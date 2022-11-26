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
#include <blur.h>
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
#include <ctype.h>
#include <math.h>
#include <srates.h>

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

static int      allocate_weave_buffer(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int      check_consistency_of_scat_params(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

#define RSIZE           (4)             /* Number of windowfulls of rand numbers in CHORU */
static int  avrg_preprocess(dataptr dz);
static int  chor_preprocess(dataptr dz);
static int  set_chorus_processing_type(dataptr dz);
static int  setup_randtables_for_rapid_rand_processing(dataptr dz);
static int  drnk_preprocess(dataptr dz);
static int  scat_preprocess(dataptr dz);
static int  convert_params_for_scat(dataptr dz);
static int  setup_internal_arrays_and_preset_internal_variables_for_scat(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

//static int  read_specshuffle_data(char *str,dataptr dz);
static int  read_specweave_data(char *filename,dataptr dz);
static int  check_for_viable_weave(int *maxbaktrak,dataptr dz);
static int  read_numeric_value_from_weavefile_text(char **str,int n,dataptr dz);

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
    //int is_spec = FALSE;
    dz->extra_bufcnt = -1;  /* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
    dz->bptrcnt = 0;
    dz->bufcnt  = 0;
    switch(dz->process) {
    case(AVRG):                     dz->extra_bufcnt =  0; dz->bptrcnt = 1;         /*is_spec = TRUE;*/             break;
    case(BLUR):                     dz->extra_bufcnt =  0; dz->bptrcnt = 4;         /*is_spec = TRUE;*/             break;
    case(SUPR):                     dz->extra_bufcnt =  0; dz->bptrcnt = 1;         /*is_spec = TRUE;*/             break;
    case(CHORUS):                   dz->extra_bufcnt =  1; dz->bptrcnt = 1;         /*is_spec = TRUE;*/             break;
    case(DRUNK):                    dz->extra_bufcnt =  0; dz->bptrcnt = 4;         /*is_spec = TRUE;*/             break;
    case(SHUFFLE):                  dz->extra_bufcnt =  0; dz->bptrcnt = 5;         /*is_spec = TRUE;*/             break;
    case(WEAVE):                    dz->extra_bufcnt =  0; dz->bptrcnt = 5;         /*is_spec = TRUE;*/             break;
    case(NOISE):                    dz->extra_bufcnt =  0; dz->bptrcnt = 1;         /*is_spec = TRUE;*/             break;
    case(SCAT):                     dz->extra_bufcnt =  0; dz->bptrcnt = 1;         /*is_spec = TRUE;*/             break;
    case(SPREAD):                   dz->extra_bufcnt =  0; dz->bptrcnt = 1;         /*is_spec = TRUE;*/             break;
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
    dz->ptr_cnt    = -1;            /* base constructor...process */
    dz->array_cnt  = -1;
    dz->iarray_cnt = -1;
    dz->larray_cnt = -1;
    switch(dz->process) {
    case(AVRG):     dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
    case(BLUR):     dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
    case(SUPR):     dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
    case(CHORUS):   dz->array_cnt =2;  dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
    case(DRUNK):    dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
    case(SHUFFLE):  dz->array_cnt = 0; dz->iarray_cnt =1;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
    case(WEAVE):    dz->array_cnt = 0; dz->iarray_cnt =1;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
    case(NOISE):    dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
    case(SCAT):     dz->array_cnt = 0; dz->iarray_cnt =1;  dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
    case(SPREAD):   dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;   break;
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
            sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
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
    case(AVRG):                     setup_process_logic(ANALFILE_ONLY,                      EQUAL_ANALFILE,         ANALFILE_OUT,   dz);    break;
    case(BLUR):                     setup_process_logic(ANALFILE_ONLY,                      EQUAL_ANALFILE,         ANALFILE_OUT,   dz);    break;
    case(SUPR):                     setup_process_logic(ANALFILE_ONLY,                      EQUAL_ANALFILE,         ANALFILE_OUT,   dz);    break;
    case(CHORUS):           setup_process_logic(ANALFILE_ONLY,                      EQUAL_ANALFILE,         ANALFILE_OUT,   dz);    break;
    case(DRUNK):            setup_process_logic(ANALFILE_ONLY,                      BIG_ANALFILE,           ANALFILE_OUT,   dz);    break;
    case(SHUFFLE):          setup_process_logic(ANALFILE_ONLY,                      BIG_ANALFILE,           ANALFILE_OUT,   dz);    break;
    case(WEAVE):            setup_process_logic(ANALFILE_ONLY,                      BIG_ANALFILE,           ANALFILE_OUT,   dz);    break;
    case(NOISE):            setup_process_logic(ANALFILE_ONLY,                      EQUAL_ANALFILE,         ANALFILE_OUT,   dz);    break;
    case(SCAT):                     setup_process_logic(ANALFILE_ONLY,                      EQUAL_ANALFILE,         ANALFILE_OUT,   dz);    break;
    case(SPREAD):           setup_process_logic(ANALFILE_ONLY,                      EQUAL_ANALFILE,         ANALFILE_OUT,   dz);    break;
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
    case(AVRG):                     exit_status = set_internalparam_data("i",ap);                                   break;
    case(BLUR):                     return(FINISHED);
    case(SUPR):                     return(FINISHED);
    case(CHORUS):           exit_status = set_internalparam_data("ii",ap);                                  break;
    case(DRUNK):            exit_status = set_internalparam_data("i",ap);                                   break;
    case(SHUFFLE):          exit_status = set_internalparam_data("iiiiii",ap);                              break;
    case(WEAVE):            exit_status = set_internalparam_data("i",ap);                                   break;
    case(NOISE):            return(FINISHED);
    case(SCAT):                     exit_status = set_internalparam_data("ii",ap);                                  break;
    case(SPREAD):           return(FINISHED);
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
    case(SHUFFLE_DATA):             return read_shuffle_data(SHUF_DMNCNT,SHUF_IMGCNT,SHUF_MAP,str,dz);
    case(WEAVE_DATA):                       return read_specweave_data(str,dz);
    default:
        sprintf(errstr,"Unknown special_data type: read_special_data()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);       /* NOTREACHED */
}

/***************************** READ_SPECWEAVE_DATA ****************************/

int read_specweave_data(char *filename,dataptr dz)
{
    int exit_status;
    FILE *fp;
    char temp[200], *p;
    int arraysize = BIGARRAY, cnt = 0;
    if((dz->iparray[WEAVE_WEAV] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for weave array.\n");
        return(MEMORY_ERROR);
    }
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Cannot open weave file '%s'\n",filename);
        return(DATA_ERROR);
    }
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while((exit_status = read_numeric_value_from_weavefile_text(&p,cnt,dz))==CONTINUE) {
            if(++cnt>=arraysize) {
                arraysize += BIGARRAY;
                if((dz->iparray[WEAVE_WEAV] =
                    (int *)realloc((char *)dz->iparray[WEAVE_WEAV],arraysize * sizeof(int)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate weave array.");
                    return(MEMORY_ERROR);
                }
            }
        }
        if(exit_status<0)
            return(exit_status);
    }
    if(cnt==0) {
        sprintf(errstr,"No data in weave file.\n");
        return(DATA_ERROR);
    }
    if((dz->iparray[WEAVE_WEAV] = (int *)realloc((char *)dz->iparray[WEAVE_WEAV],cnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate weave array.\n");
        return(MEMORY_ERROR);
    }
    dz->itemcnt = cnt;
    return check_for_viable_weave(&(dz->iparam[WEAVE_BAKTRAK]),dz);
}

/****************************** CHECK_FOR_VIABLE_WEAVE **************************/

int check_for_viable_weave(int *maxbaktrak,dataptr dz)
{
    int n, m;
    int weavhere   = 0;
    int pos_relative_to_this_start;
    int maxbaktrak_from_this_start;
    *maxbaktrak = 0;
    for(n=0; n<dz->itemcnt; n++) {
        if((weavhere += dz->iparray[WEAVE_WEAV][n]) < 0) {
            sprintf(errstr,"You cannot weave to before weave-start.\n");
            return(DATA_ERROR);
        }
    }
    if(weavhere <= 0) {
        sprintf(errstr,"Weave must progress aint file.\n");
        return(DATA_ERROR);
    }
    for(n=1; n<dz->itemcnt; n++) {
        pos_relative_to_this_start = 0;
        maxbaktrak_from_this_start = 0;
        for(m=n; m<dz->itemcnt; m++) {
            if((pos_relative_to_this_start += dz->iparray[WEAVE_WEAV][n]) < 0)
                maxbaktrak_from_this_start = min(pos_relative_to_this_start,maxbaktrak_from_this_start);
        }
        *maxbaktrak = min(*maxbaktrak,maxbaktrak_from_this_start);
    }
    fprintf(stdout,"INFO: Output duration approx %.2lf times that of input.\n",
            (double)dz->itemcnt/(double)weavhere);
    fflush(stdout);
    *maxbaktrak = -(*maxbaktrak);
    return(FINISHED);
}

/************************** READ_NUMERIC_VALUE_FROM_WEAVEFILE_TEXT **************************
 *
 * takes a pointer TO A POINTER to a string. If it succeeds in finding
 * a char it returns the char value (*val), and it's new position in the
 * string (*str).
 */

int read_numeric_value_from_weavefile_text(char **str,int n,dataptr dz)
{       char *p, *q, *digitstart, *end;
    int numero;
    p = *str;
    while(isspace(*p))
        p++;
    q = p;
    if(*p=='\0')
        return(FINISHED);
    if(!isdigit(*p) && *p!='-') {
        sprintf(errstr,"Invalid character ('%c') in weave file.\n",*p);
        return(DATA_ERROR);
    }
    if(*p == '-')
        p++;
    digitstart = p;
    while(isdigit(*p))
        p++;
    if(p - digitstart <= 0) {
        sprintf(errstr,"Invalid character sequence in weave file.\n");
        return(DATA_ERROR);
    }
    if(!isspace(*p) && *p!='\0') {
        sprintf(errstr,"Invalid character in weave file.\n");
        return(DATA_ERROR);
    }
    end = p;
    if(sscanf(q,"%d",&numero)!=1) {
        sprintf(errstr,"Failed to read value from weave file.\n");
        return(DATA_ERROR);
    }
    if(numero > dz->application->max_special || numero < dz->application->min_special) {
        sprintf(errstr,"Weave value %d out of range (%.0lf to %.0lf) for this file.\n",
                n+1,dz->application->min_special,dz->application->max_special);
        return(USER_ERROR);
    }
    dz->iparray[WEAVE_WEAV][n] = numero;
    *str = end;
    return(CONTINUE);
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/


/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)
{
    switch(dz->process) {
    case(AVRG):                 return avrg_preprocess(dz);
    case(SUPR):                     return setup_ring(dz);
    case(CHORUS):           return chor_preprocess(dz);
    case(DRUNK):            return drnk_preprocess(dz);
    case(SCAT):                     return scat_preprocess(dz);

    case(BLUR):     case(SHUFFLE):  case(WEAVE):    case(NOISE):    case(SPREAD):
        return(FINISHED);
    default:
        sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);       /* NOTREACHED */
}

/************************** AVRG_PREPROCESS ******************************/

int avrg_preprocess(dataptr dz)
{
    if(dz->brksize[AVRG_AVRG]==0) {
        dz->iparam[AVRG_AVRGSPAN] = dz->iparam[AVRG_AVRG]/2;
        dz->iparam[AVRG_AVRG]     = (dz->iparam[AVRG_AVRGSPAN] * 2) + 1;/* ALWAYS ODD */
    }
    return(FINISHED);
}

/************************** CHOR_PREPROCESS ******************************/

int chor_preprocess(dataptr dz)
{
    int exit_status;
    dz->iparam[CHORU_RTABSIZE] = dz->wanted * RSIZE;
    if((exit_status = set_chorus_processing_type(dz))<0)
        return(exit_status);
    return setup_randtables_for_rapid_rand_processing(dz);
}

/*************************** SET_CHORUS_PROCESSING_TYPE *************************/

int set_chorus_processing_type(dataptr dz)
{
    switch(dz->mode) {
    case(CH_AMP):
        if(dz->brksize[CHORU_AMPR]==0)   dz->iparam[CHORU_SPRTYPE] = A_FIX;
        else                                                     dz->iparam[CHORU_SPRTYPE] = A_VAR;
        break;
    case(CH_FRQ):
    case(CH_FRQ_UP):
    case(CH_FRQ_DN):
        if(dz->brksize[CHORU_FRQR]==0)   dz->iparam[CHORU_SPRTYPE] = F_FIX;
        else                                                     dz->iparam[CHORU_SPRTYPE] = F_VAR;
        break;
    case(CH_AMP_FRQ):
    case(CH_AMP_FRQ_UP):
    case(CH_AMP_FRQ_DN):
        if(dz->brksize[CHORU_AMPR]==0) {
            if(dz->brksize[CHORU_FRQR]==0)  dz->iparam[CHORU_SPRTYPE] = AF_FIX;
            else                                                    dz->iparam[CHORU_SPRTYPE] = A_FIX_F_VAR;
        } else {
            if(dz->brksize[CHORU_FRQR]==0)  dz->iparam[CHORU_SPRTYPE] = A_VAR_F_FIX;
            else                                                    dz->iparam[CHORU_SPRTYPE] = AF_VAR;
        }
        break;
    default:
        sprintf(errstr,"Unknown program mode in set_chorus_processing_type()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************** SETUP_RANDTABLES_FOR_RAPID_RAND_PROCESSING ***************/

int setup_randtables_for_rapid_rand_processing(dataptr dz)
{
    int n;
    double z;
    if(dz->iparam[CHORU_SPRTYPE] & IS_A) {
        if((dz->parray[CHORU_RTABA] = (double *)malloc(dz->iparam[CHORU_RTABSIZE] * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for chorusing amplitude random table.\n");
            return(MEMORY_ERROR);
        }
        for(n=0;n<dz->iparam[CHORU_RTABSIZE];n++) {
            z  = drand48() * 2.0;
            dz->parray[CHORU_RTABA][n] = z - 1.0;
        }
        if(dz->iparam[CHORU_SPRTYPE] & A_FIX) {
            for(n=0;n<dz->iparam[CHORU_RTABSIZE];n++)
                dz->parray[CHORU_RTABA][n] = (double)pow(dz->param[CHORU_AMPR],dz->parray[CHORU_RTABA][n]);
        }
    }
    if(dz->iparam[CHORU_SPRTYPE] & IS_F) {
        if((dz->parray[CHORU_RTABF] = (double *)malloc(dz->iparam[CHORU_RTABSIZE] * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for chorusing frq random table.\n");
            return(MEMORY_ERROR);
        }
        switch(dz->mode) {
        case(CH_FRQ_DN):
        case(CH_AMP_FRQ_DN): /* downwards only  */
            for(n=0;n<dz->iparam[CHORU_RTABSIZE];n++) {
                z  = drand48();
                dz->parray[CHORU_RTABF][n] = z - 1.0;
            }
            break;
        case(CH_FRQ_UP):
        case(CH_AMP_FRQ_UP):    /* upwards only */
            for(n=0;n<dz->iparam[CHORU_RTABSIZE];n++)
                dz->parray[CHORU_RTABF][n]  = drand48();
            break;
        default: /* normal */
            for(n=0;n<dz->iparam[CHORU_RTABSIZE];n++) {
                z  = drand48() * 2.0;
                dz->parray[CHORU_RTABF][n] = z - 1.0;
            }
            break;
        }
        if(dz->iparam[CHORU_SPRTYPE] & F_FIX) {
            for(n=0;n<dz->iparam[CHORU_RTABSIZE];n++)
                dz->parray[CHORU_RTABF][n] = pow(dz->param[CHORU_FRQR],dz->parray[CHORU_RTABF][n]);
        }
    }
    return(FINISHED);
}

/************************** DRNK_PREPROCESS ******************************/

int drnk_preprocess(dataptr dz)
{
    initrand48();
    dz->iparam[DRNK_TWICERANGE] = (dz->iparam[DRNK_RANGE] * 2) + 1;
    return(FINISHED);
}

/************************** SCAT_PREPROCESS ******************************/

int scat_preprocess(dataptr dz)
{
    int exit_status;
    if((exit_status = convert_params_for_scat(dz))<0)
        return(exit_status);
    return setup_internal_arrays_and_preset_internal_variables_for_scat(dz);
}

/************ CONVERT_PARAMS_FOR_SCAT *************
 *
 * convert frq width input to channel cnt, for bloksize
 */

int convert_params_for_scat(dataptr dz)
{
    double *p;
    int n;
    if(dz->brksize[SCAT_BLOKSIZE]==0)
        dz->iparam[SCAT_BLOKSIZE] = round(dz->param[SCAT_BLOKSIZE]/dz->chwidth);
    else {
        p = dz->brk[SCAT_BLOKSIZE] + 1;
        for(n=0;n < dz->brksize[SCAT_BLOKSIZE];n++) {
            *p /= dz->chwidth;
            p  += 2;
        }       /* AND ensure bloksize is now ROUNDED TO INT when read from brktable */
        dz->is_int[SCAT_BLOKSIZE] = 1;
    }
    return(FINISHED);
}

/************ SETUP_INTERNAL_ARRAYS_AND_PRESET_INTERNAL_VARIABLES_FOR_SCAT *********/

int setup_internal_arrays_and_preset_internal_variables_for_scat(dataptr dz)
{
    if((dz->iparray[SCAT_KEEP]
        = (int *)malloc((dz->clength+1) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for scatter keep table.\n");
        return(MEMORY_ERROR);
    }
    if(dz->brksize[SCAT_CNT]==0)
        dz->iparam[SCAT_THISCNT] = dz->iparam[SCAT_CNT];
    if(dz->brksize[SCAT_BLOKSIZE]==0) {
        dz->iparam[SCAT_BLOKS_PER_WINDOW] = dz->clength/dz->iparam[SCAT_BLOKSIZE];
        if((dz->iparam[SCAT_BLOKS_PER_WINDOW]*dz->iparam[SCAT_BLOKSIZE])!=dz->clength)
            dz->iparam[SCAT_BLOKS_PER_WINDOW]++;
    }
    if(dz->brksize[SCAT_BLOKSIZE]==0 && dz->brksize[SCAT_CNT]==0
       && dz->iparam[SCAT_CNT] >= dz->iparam[SCAT_BLOKS_PER_WINDOW]) {
        sprintf(errstr,"Block count exceeds number of blocks per window.\n");
        return(USER_ERROR);
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
    case(AVRG):             return outer_loop(dz);
    case(SUPR):             return outer_loop(dz);
    case(CHORUS):   return outer_loop(dz);
    case(NOISE):    return outer_loop(dz);
    case(SCAT):             return outer_loop(dz);
    case(SPREAD):   return outer_loop(dz);

    case(BLUR):             return spec_blur_and_bltr(dz);
    case(DRUNK):    return specdrunk(dz);
    case(SHUFFLE):  return specshuffle(dz);
    case(WEAVE):    return specweave(dz);
    default:
        sprintf(errstr,"Unknown process in procspec()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);       /* NOTREACHED */
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
        case(AVRG):             exit_status = specavrg(dz);                                                     break;
        case(CHORUS):   exit_status = specchorus(dz);                                           break;
        case(NOISE):    exit_status = specnoise(dz);                                            break;
        case(SCAT):             exit_status = specscat(dz);                                             break;
        case(SPREAD):   exit_status = specspread(dz);                                           break;
        case(SUPR):             exit_status = specsupr(dz);                                                     break;
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
    case(AVRG):
    case(SPREAD):
        return(TRUE);
    }
    return(FALSE);
}

/*********************************** SPEC_BLUR_AND_BLTR ***********************************/

int spec_blur_and_bltr(dataptr dz)
{
    int exit_status;
    int     blurcnt = 0, blurfactor = 0, samps_read, got, w_to_buf, wc;
    int     last_total_windows = 0;
    float   *ampdif, *freqdif;
    dz->time = 0.0f;
    if((exit_status = float_array(&ampdif,dz->clength))<0)
        return(exit_status);
    if((exit_status = float_array(&freqdif,dz->clength))<0)
        return(exit_status);
    dz->flbufptr[1] = dz->flbufptr[2];
    if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
        return exit_status;

    while((samps_read = dz->ssampsread) > 0) {
        got                = samps_read;
        dz->flbufptr[0] = dz->bigfbuf;
        w_to_buf       = got/dz->wanted;
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
                if(dz->total_windows + blurfactor >= dz->wlength)         /* number of windows to skip */
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
        if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
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
    for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2){
        ampdif[cc]   = dz->flbufptr[0][AMPP] - dz->amp[cc];
        ampdif[cc]  /= (float)blurfactor;
        freqdif[cc]  = dz->flbufptr[0][FREQ] - dz->freq[cc];
        freqdif[cc] /= (float)blurfactor;
    }
    for(thiswindow = *last_total_windows,j=0;thiswindow<dz->total_windows;thiswindow++,j++) {
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2){  /* BLUR amp AND freq */
            dz->flbufptr[1][AMPP] = dz->amp[cc]  + ((float)j * ampdif[cc]);
            dz->flbufptr[1][FREQ] = dz->freq[cc] + ((float)j * freqdif[cc]);
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

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
    handle_pitch_zeros(dz);
    switch(dz->process) {
    case(SCAT):                return check_consistency_of_scat_params(dz);
    }
    return(FINISHED);
}

/************ CHECK_CONSISTENCY_OF_SCAT_PARAMS *************/

int check_consistency_of_scat_params(dataptr dz)
{
    int bloksize, bpw;
    if(dz->brksize[SCAT_BLOKSIZE]==0 && dz->brksize[SCAT_CNT]==0) {
        bloksize = round(dz->param[SCAT_BLOKSIZE]/dz->chwidth);
        bpw = dz->clength/bloksize;
        if((bpw * bloksize)!=dz->clength)
            bpw++;
        if(dz->iparam[SCAT_CNT] >= bpw) {
            sprintf(errstr,"No. of blocks to keep exceeds number of blocks per window.\n");
            return(USER_ERROR);
        }
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
    case(AVRG):             case(SUPR):             case(CHORUS):
    case(NOISE):    case(SCAT):             case(SPREAD):
        return allocate_single_buffer(dz);

    case(BLUR):     case(DRUNK):    case(SHUFFLE):
        return allocate_double_buffer(dz);

    case(WEAVE):
        return allocate_weave_buffer(dz);
    default:
        sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);       /* NOTREACHED */
}

/*************************** ALLOCATE_WEAVE_BUFFER ****************************/

int allocate_weave_buffer(dataptr dz)
{
    unsigned int buffersize;
    if(dz->bptrcnt < 5) {
        sprintf(errstr,"Insufficient bufptrs established in allocate_weave_buffer()\n");
        return(PROGRAM_ERROR);
    }
    //TW CONVERTED FO FLT
    buffersize = dz->wanted * BUF_MULTIPLIER;
    dz->buflen = buffersize;
    buffersize *= 2;
    buffersize += (dz->iparam[WEAVE_BAKTRAK] * dz->wanted);
    if((dz->bigfbuf = (float*)malloc((size_t)buffersize * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize   = dz->buflen;                                               /* BIGBUF is start of baktrak buf */
    //TW ADDED
    dz->flbufptr[0]   = dz->bigfbuf + (dz->iparam[WEAVE_BAKTRAK] * dz->wanted);             /* start of input buf */
    dz->flbufptr[2] = dz->flbufptr[0] + dz->big_fsize;   /* start of outbuf (&& initially end of input buf) */
    dz->flbufptr[3] = dz->flbufptr[2] + dz->big_fsize;                                                           /* end of output buf */
    dz->flbufptr[4] = dz->flbufptr[2] - (dz->iparam[WEAVE_BAKTRAK] * dz->wanted);               /* wrap-around marker */
    return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if     (!strcmp(prog_identifier_from_cmdline,"avrg"))                   dz->process = AVRG;
    else if(!strcmp(prog_identifier_from_cmdline,"blur"))                   dz->process = BLUR;
    else if(!strcmp(prog_identifier_from_cmdline,"suppress"))               dz->process = SUPR;
    else if(!strcmp(prog_identifier_from_cmdline,"chorus"))                 dz->process = CHORUS;
    else if(!strcmp(prog_identifier_from_cmdline,"drunk"))                  dz->process = DRUNK;
    else if(!strcmp(prog_identifier_from_cmdline,"shuffle"))                dz->process = SHUFFLE;
    else if(!strcmp(prog_identifier_from_cmdline,"weave"))                  dz->process = WEAVE;
    else if(!strcmp(prog_identifier_from_cmdline,"noise"))                  dz->process = NOISE;
    else if(!strcmp(prog_identifier_from_cmdline,"scatter"))                dz->process = SCAT;
    else if(!strcmp(prog_identifier_from_cmdline,"spread"))                 dz->process = SPREAD;
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
            "\nBLURRING OPERATIONS ON A SPECTRAL FILE\n\n"
            "USAGE: blur NAME (mode) infile outfile parameters: \n"
            "\n"
            "where NAME can be any one of\n"
            "\n"
            "avrg      blur      suppress    chorus      drunk\n"
            "shuffle   weave     noise       scatter     spread\n\n"
            "Type 'blur avrg' for more info on blur avrg..ETC.\n");
    return(USAGE_ONLY);
}

int usage2(char *str)
{
    if(!strcmp(str,"avrg")) {
        fprintf(stdout,
                "blur avrg infile outfile N\n"
                "\n"
                "AVERAGE SPECTRAL ENERGY OVER N ADJACENT CHANNELS\n"
                "\n"
                "N must be ODD and <= half the -N param used in original analysis.\n"
                "\n"
                "N may vary over time.\n");
    } else if(!strcmp(str,"blur")) {
        fprintf(stdout,
                "blur blur infile outfile blurring\n"
                "\n"
                "TIME-AVERAGE THE SPECTRUM\n"
                "\n"
                "blurring   is number of windows over which to average the spectrum.\n"
                "\n"
                "blurring may vary over time. \n");
    } else if(!strcmp(str,"suppress")) {
        fprintf(stdout,
                "blur suppress infile outfile N\n"
                "\n"
                "SUPPRESS THE N LOUDEST PARTIALS (ON A WINDOW-BY-WINDOW BASIS)\n"
                "\n"
                "N   is number of spectral components to reject.\n"
                "\n"
                "N may vary over time.  \n");
    } else if(!strcmp(str,"chorus")) {
        fprintf(stdout,
                "blur chorus 1   infile outfile aspread        \n"
                "blur chorus 2-4 infile outfile         fspread\n"
                "blur chorus 5-7 infile outfile aspread fspread\n"
                "\n"
                "CHORUSING BY RANDOMISING AMPLITUDES AND/OR FREQUENCIES OF PARTIALS\n"
                "\n"
                "MODES :-\n"
                "1  Randomise partial amplitudes.\n"
                "2  Randomise partial frequencies.\n"
                "3  Randomise partial frequencies upwards only.\n"
                "4  Randomise partial frequencies downwards only.\n"
                "5  Randomise partial amplitudes AND frequencies.\n"
                "6  Randomise partial amplitudes, and frequencies upwards only.\n"
                "7  Randomise partial amplitudes, and frequencies downwards only.\n"
                "\n"
                "aspread   is maximum random scatter of partial-amps (Range 1-%.0lf)\n"
                "fspread   is maximum random scatter of partial-frqs (Range 1-%.0lf)\n"
                "\n"
                "aspread and fspread may vary over time.\n",CHORU_MAXAMP,CHORU_MAXFRQ);
    } else if(!strcmp(str,"drunk")) {
        fprintf(stdout,
                "blur drunk infile outfile range starttime duration [-z]\n\n"
                "MODIFY SOUND BY DRUNKEN WALK ALONG ANALYSIS WINDOWS.\n\n"
                "RANGE     is maximum step (in windows) for drunken walk.\n"
                "STARTTIME is time (in secs) in file to begin walk.\n"
                "DURATION  is required duration of outfile after resynthesis.\n"
                "-z        eliminates zero steps (window-repeats) in the drunken walk.\n"
                "\n");
    } else if(!strcmp(str,"shuffle")) {
        fprintf(stdout,
                "blur shuffle infile outfile domain-image grpsize\n\n"
                "SHUFFLE ORDER OF ANALYSIS WINDOWS IN FILE\n\n"
                "domain-image is 2 strings of letters, separated by a '-'\n"
                "The first string is the DOMAIN, and the 2nd, the IMAGE.\n\n"
                "e.g. 'abc-abbabcc'\n\n"
                "The DOMAIN letters represent a group of consecutive inputfile analysis_windows.\n\n"
                "e.g. 'abcd'\n\n"
                "The IMAGE is any permutation of, or selection from, these domain letters.\n"
                "(domain letters may be omitted or repeated in the image string).\n\n"
                "e.g. 'aaaaaaadaaa'\n\n"
                "Inputfile windows are shuffled according to this same mapping.\n\n"
                "grpsize Windows may be grouped in sets of GRPSIZE\n"
                "        before being shuffled (as groups).\n"
                "        In this case,each domain-letter represents a group of GRPSIZE windows.\n"
                "\n");
    } else if(!strcmp(str,"weave")) {
        fprintf(stdout,
                "blur weave infile outfile weavfile\n\n"
                "MODIFY SOUND BY WEAVING AMONGST ANALYSIS WINDOWS.\n\n"
                "WEAVFILE contains a list of integers which define successive steps (in windows)\n"
                "through the input file. Steps may be forward or backwards.\n"
                "The step sequence is repeated until end of infile is reached.\n\n"
                "Weave must obey the following rules....\n\n"
                "RULE 1:  NO step can exceed the length of the file, in windows, forwards or backwards.\n"
                "RULE 2:  NO window reached in a weave can be BEFORE startwindow of the weave.\n"
                "RULE 3:  FINAL window must be AFTER the weave startwindow.\n\n"
                "Otherwise, weave may be forward or backward, or remain at same window.\n"
                "\n");
    } else if(!strcmp(str,"noise")) {
        fprintf(stdout,
                "blur noise infile outfile noise\n"
                "\n"
                "PUT NOISE IN THE SPECTRUM\n"
                "\n"
                "noise   Range 0 (no noise in spectrum) to 1 (spectrum saturated with noise).\n"
                "\n"
                "noise may vary over time.\n");
    } else if(!strcmp(str,"scatter")) {
        fprintf(stdout,
                "blur scatter infile outfile keep [-bblocksize] [-r] [-n] \n"
                "\n"
                "RANDOMLY THIN OUT THE SPECTRUM\n"
                "\n"
                "keep      number of (randomly-chosen) blocks to keep, in each spectral window.\n"
                "blocksize is freq range of each block (default is width of 1 analysis channel).\n"
                "          (Rounded internally to a multiple of channel width).\n"
                "-r        number of blocks actually selected is randomised between 1 & keep.\n"
                "-n        Turn OFF normalisation of resulting sound.\n"
                "\n"
                "keep & blocksize may vary over time.\n");
    } else if(!strcmp(str,"spread")) {
        fprintf(stdout,
                "blur spread infile outfile -fN|-pN [-i] [-sspread]\n"
                "\n"
                "SPREAD PEAKS OF SPECTRUM, INTRODUCING CONTROLLED NOISINESS\n"
                "\n"
                "-f   extract formant envelope linear frqwise,\n"
                "     using 1 point for every N equally-spaced frequency-channels.\n"
                "-p   extract formant envelope linear pitchwise,\n"
                "     using N equally-spaced pitch-bands per octave.\n"
                "-i   quicksearch for formants (less accurate).\n"
                "\n"
                "spread    degree of spreading of spectrum (Range 0-1 : Default 1).\n"
                "\n"
                "spread may vary over time.\n");
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
