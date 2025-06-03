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
#ifdef WIN32
#include <malloc.h>
#else
#include <memory.h>
#endif
#include <ctype.h>
#include <string.h>
#include <structures.h>
#include <flags.h>
#include <speccon.h>
#include <arrays.h>
#include <special.h>
#include <formants.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
#include <stdlib.h>
#include <mixxcon.h>
#include <sfsys.h>
#include <osbind.h>
#include <math.h>
#include <filetype.h>
#include <pnames.h>
#include <srates.h>
#include <vowels.h>

#ifdef unix
#define round(x) lround((x))
#endif

/******************************************************************************/
/************************* FORMERLY IN procspec.c *****************************/
/******************************************************************************/

static int  extend_ring(int change_in_size,dataptr dz);
static int  shrink_ring(int change_in_size,dataptr dz);
static int  zero_channels_between_partials(int *vvc,double frq_close_below_next_partial,dataptr dz);
static int  eliminate_all_but_loudest_channel_near_this_partial_frq
                (int *vvc,double frq_close_above_next_partial,dataptr dz);
static int  keep_all_channels_near_partial_frq(int *vvc,double frq_close_above_next_partial,dataptr dz);
static int  do_amplitude_poll(int bot,int top,dataptr dz);

/******************************************************************************/
/************************* FORMERLY IN aplinit.c ******************************/
/******************************************************************************/

static int  establish_application(dataptr dz);
static int  application_init(dataptr dz);

static int  initialise_vflags(dataptr dz);
static int  setup_and_init_input_param_activity(dataptr dz,int tipc);
static int  setup_and_init_input_brktable_constants(dataptr dz,int tipc);
static int  setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int  initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int  mark_parameter_types(dataptr dz,aplptr ap);

static int  establish_infile_constants(dataptr dz);
static int  setup_input_param_defaultval_stores(int tipc,aplptr ap);

static int get_domain_and_image(char *str,char **domain, char **image);
static int consistent_domain_and_image(int dmncnt,int imgcnt,char *domain,char *image,dataptr dz);
static int make_map(int dmncnt,int imgcnt,int mapping,char *domain,char *image,dataptr dz);

/**************************** SETUP_PARTICULAR_APPLICATION *******************************/

int setup_particular_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)
        return(exit_status);
    ap = dz->application;
    if((exit_status = set_legal_param_structure(dz->process,dz->mode,ap))<0)
        return(exit_status);                                        /* GLOBAL LIBRARY */

    if((exit_status = set_legal_option_and_variant_structure(dz->process,dz->mode,ap))<0)
        return(exit_status);                                        /* GLOBAL LIBRARY */
    set_formant_flags(dz->process,dz->mode,ap);                     /* GLOBAL LIBRARY */

    if((exit_status = set_legal_internalparam_structure(dz->process,dz->mode,ap))<0)
        return(exit_status);                                        /* LIBRARY */
    set_legal_infile_structure(dz);                                 /* IN THIS FILE */
    assign_process_logic(dz);                                       /* IN THIS FILE */

    if((exit_status = application_init(dz))<0)                      /* IN THIS FILE */
        return(exit_status);
    return(FINISHED);
}

/****************************** ESTABLISH_APPLICATION *******************************/

int establish_application(dataptr dz)
{
    aplptr ap;
    if((dz->application = (aplptr)malloc(sizeof (struct applic)))==NULL) {
        sprintf(errstr,"establish_application()\n");
        return(MEMORY_ERROR);
    }
    ap = dz->application;
    memset((char *)ap,0,sizeof(struct applic));
    return(FINISHED);
}

/***************************** APPLICATION_INIT **************************/

int application_init(dataptr dz)
{
    int exit_status;
    int storage_cnt;
    int tipc, brkcnt;
    aplptr ap = dz->application;
    if(ap->vflag_cnt>0)
        initialise_vflags(dz);
    tipc  = ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt;
    ap->total_input_param_cnt = (char)tipc;
    if(tipc>0) {
        if((exit_status = setup_input_param_range_stores(tipc,ap))<0)
            return(exit_status);
        if((exit_status = setup_input_param_defaultval_stores(tipc,ap))<0)
            return(exit_status);
        if((exit_status = setup_and_init_input_param_activity(dz,tipc))<0)
            return(exit_status);
    }
    brkcnt = tipc;

    if(dz->input_data_type==BRKFILES_ONLY
    || dz->input_data_type==UNRANGED_BRKFILE_ONLY
    || dz->input_data_type==DB_BRKFILES_ONLY
    || dz->input_data_type==SNDFILE_AND_BRKFILE
    || dz->input_data_type==SNDFILE_AND_UNRANGED_BRKFILE
    || dz->input_data_type==SNDFILE_AND_DB_BRKFILE
    || dz->input_data_type==ALL_FILES
    || dz->input_data_type==ANY_NUMBER_OF_ANY_FILES
    || dz->outfiletype==BRKFILE_OUT) {
        dz->extrabrkno = brkcnt;
        brkcnt++;       /* extra brktable for input or output brkpntfile data */
    }
    if(brkcnt>0) {
        if((exit_status = setup_and_init_input_brktable_constants(dz,brkcnt))<0)
            return(exit_status);
    }

    if((storage_cnt = tipc + ap->internal_param_cnt)>0) {
        if((exit_status = setup_parameter_storage_and_constants(storage_cnt,dz))<0)
            return(exit_status);
        if((exit_status = initialise_is_int_and_no_brk_constants(storage_cnt,dz))<0)
            return(exit_status);
    }
    if((exit_status = mark_parameter_types(dz,ap))<0)
            return(exit_status);

    if((exit_status = establish_infile_constants(dz))<0)
        return(exit_status);
    if((exit_status = establish_bufptrs_and_extra_buffers(dz))<0)
        return(exit_status);
    if((exit_status = setup_internal_arrays_and_array_pointers(dz))<0)
        return(exit_status);
    return(FINISHED);
}

/************************* INITIALISE_VFLAGS *************************/

int initialise_vflags(dataptr dz)
{
    int n;
    if((dz->vflag  = (char *)malloc(dz->application->vflag_cnt * sizeof(char)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: vflag store,\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->application->vflag_cnt;n++)
        dz->vflag[n]  = FALSE;
    return FINISHED;
}

/***************************** SETUP_AND_INIT_INPUT_PARAM_ACTIVITY **************************/

int setup_and_init_input_param_activity(dataptr dz,int tipc)
{
    int n;
    if((dz->is_active = (char   *)malloc((size_t)tipc))==NULL) {
        sprintf(errstr,"setup_and_init_input_param_activity()\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<tipc;n++)
        dz->is_active[n] = (char)0;
    return(FINISHED);
}

/***************************** SETUP_AND_INIT_INPUT_BRKTABLE_CONSTANTS **************************/

int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt)
{
    int n;
    if((dz->brk      = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 1\n");
        return(MEMORY_ERROR);
    }
    if((dz->brkptr   = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 6\n");
        return(MEMORY_ERROR);
    }
    if((dz->brksize  = (int    *)malloc(brkcnt * sizeof(int)    ))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 2\n");
        return(MEMORY_ERROR);
    }
    if((dz->firstval = (double  *)malloc(brkcnt * sizeof(double)  ))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 3\n");
        return(MEMORY_ERROR);
    }
    if((dz->lastind  = (double  *)malloc(brkcnt * sizeof(double)  ))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 4\n");
        return(MEMORY_ERROR);
    }
    if((dz->lastval  = (double  *)malloc(brkcnt * sizeof(double)  ))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 5\n");
        return(MEMORY_ERROR);
    }
    if((dz->brkinit  = (int     *)malloc(brkcnt * sizeof(int)     ))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 7\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<brkcnt;n++) {
        dz->brk[n]     = NULL;
        dz->brkptr[n]  = NULL;
        dz->brkinit[n] = 0;
        dz->brksize[n] = 0;
    }
    return(FINISHED);
}

/********************** SETUP_PARAMETER_STORAGE_AND_CONSTANTS ********************/
/* RWD malloc changed to calloc; helps debug verison run as release! */
int setup_parameter_storage_and_constants(int storage_cnt,dataptr dz)
{
    // RWD Apr 20100 add extra element
    storage_cnt += 1;
    
    if((dz->param       = (double *)calloc(storage_cnt, sizeof(double)))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 1\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparam      = (int    *)calloc(storage_cnt, sizeof(int)   ))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 2\n");
        return(MEMORY_ERROR);
    }
    if((dz->is_int      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 3\n");
        return(MEMORY_ERROR);
    }
    if((dz->no_brk      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 5\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/************** INITIALISE_IS_INT_AND_NO_BRK_CONSTANTS *****************/

int initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz)
{
    int n;
    for(n=0;n<storage_cnt;n++) {
        dz->is_int[n] = (char)0;
        dz->no_brk[n] = (char)0;
    }
    return(FINISHED);
}

/***************************** MARK_PARAMETER_TYPES **************************/

int mark_parameter_types(dataptr dz,aplptr ap)
{
    int n, m;                           /* PARAMS */
    for(n=0;n<ap->max_param_cnt;n++) {
        switch(ap->param_list[n]) {
        case('0'):  break; /* dz->is_active[n] = 0 is default */
        case('i'):  dz->is_active[n] = (char)1; dz->is_int[n] = (char)1;dz->no_brk[n] = (char)1; break;
        case('I'):  dz->is_active[n] = (char)1; dz->is_int[n] = (char)1;                         break;
        case('d'):  dz->is_active[n] = (char)1;                         dz->no_brk[n] = (char)1; break;
        case('D'):  dz->is_active[n] = (char)1; /* normal case: double val or brkpnt file */     break;
        default:
            sprintf(errstr,"Programming error: invalid parameter type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }                               /* OPTIONS */
    for(n=0,m=ap->max_param_cnt;n<ap->option_cnt;n++,m++) {
        switch(ap->option_list[n]) {
//TW JULY 2006
        case('0'): break;
        case('i'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1; dz->no_brk[m] = (char)1; break;
        case('I'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;                          break;
        case('d'): dz->is_active[m] = (char)1;                          dz->no_brk[m] = (char)1; break;
        case('D'): dz->is_active[m] = (char)1;  /* normal case: double val or brkpnt file */     break;
        default:
            sprintf(errstr,"Programming error: invalid option type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }                               /* VARIANTS */
    for(n=0,m=ap->max_param_cnt + ap->option_cnt;n < ap->variant_param_cnt; n++, m++) {
        switch(ap->variant_list[n]) {
        case('0'): break;
        case('i'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1; dz->no_brk[m] = (char)1; break;
        case('I'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;                          break;
        case('d'): dz->is_active[m] = (char)1;                          dz->no_brk[m] = (char)1; break;
        case('D'): dz->is_active[m] = (char)1; /* normal case: double val or brkpnt file */      break;
        default:
            sprintf(errstr,"Programming error: invalid variant type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }                               /* INTERNAL */
    for(n=0,
    m=ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt; n<ap->internal_param_cnt; n++,m++) {
        switch(ap->internal_param_list[n]) {
        case('0'):  break;   /* dummy variables: variables not used: but important for internal paream numbering!! */
        case('i'):  dz->is_int[m] = (char)1;    dz->no_brk[m] = (char)1;    break;
        case('d'):                              dz->no_brk[m] = (char)1;    break;
        default:
            sprintf(errstr,"Programming error: invalid internal param type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/******************** ESTABLISH_SPEC_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_spec_bufptrs_and_extra_buffers(dataptr dz)
{
    int n;
    if(dz->extra_bufcnt) {
        if((dz->windowbuf = (float **)malloc(sizeof(float *) * dz->extra_bufcnt))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for extra float sample buffers.\n");
            return(MEMORY_ERROR);
        }
        for(n = 0; n < dz->extra_bufcnt;n++)
            dz->windowbuf[n] = NULL;
    }
    if(dz->bptrcnt) {
        if((dz->flbufptr = (float **)malloc(sizeof(float *) * dz->bptrcnt))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for float sample buffers.\n");
            return(MEMORY_ERROR);
        }
        for(n = 0;n <dz->bptrcnt; n++)
            dz->flbufptr[n] = NULL;
    }
    return(FINISHED);
}

/******************** ESTABLISH_GROUCHO_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_groucho_bufptrs_and_extra_buffers(dataptr dz)
{
    int n;
    if(dz->bufcnt < 0)  {
        sprintf(errstr,"bufcnt has not been set: establish_groucho_bufptrs_and_extra_buffers()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->extra_bufcnt < 0)    {
        sprintf(errstr,"extrs_bufcnt has not been set: establish_groucho_bufptrs_and_extra_buffers()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->bufcnt) {
        if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
            return(MEMORY_ERROR);
        }
        if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
            return(MEMORY_ERROR);
        }
        for(n = 0;n <dz->bufcnt; n++)
            dz->sampbuf[n] = dz->sbufptr[n] = (float *)0;
        dz->sampbuf[n] = (float *)0;
    }
    if(dz->extra_bufcnt) {
        if((dz->extrabuf = (float **)malloc(sizeof(float *) * (dz->extra_bufcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY establishing extra buffers.\n");
            return(MEMORY_ERROR);
        }
        if((dz->extrabufptr = (float **)malloc(sizeof(float *) * (dz->extra_bufcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY establishing extrabuffer pointers.\n");
            return(MEMORY_ERROR);
        }
        for(n = 0;n <dz->extra_bufcnt; n++)
            dz->extrabuf[n] = dz->extrabufptr[n] = (float *)0;
        dz->extrabuf[n] = (float *)0;
    }
    return(FINISHED);
}

/***************************** ESTABLISH_INFILE_CONSTANTS **************************/

int establish_infile_constants(dataptr dz)
{
    switch(dz->input_data_type) {

    case(MIXFILES_ONLY):        case(SNDLIST_ONLY):
    case(SND_OR_MIXLIST_ONLY):  case(SND_SYNC_OR_MIXLIST_ONLY):
    case(BRKFILES_ONLY):        case(DB_BRKFILES_ONLY):     case(UNRANGED_BRKFILE_ONLY):
    case(WORDLIST_ONLY):
        dz->infilecnt = ONE_NONSND_FILE;
        break;
    case(NO_FILE_AT_ALL):
        dz->infilecnt = 0;
        break;
    case(ALL_FILES):            case(SNDFILES_ONLY):        case(ENVFILES_ONLY):
    case(ANALFILE_ONLY):        case(PITCHFILE_ONLY):       case(PITCH_OR_TRANSPOS):
    case(FORMANTFILE_ONLY):
        dz->infilecnt = 1;
        break;
    case(TWO_SNDFILES):
    case(SNDFILE_AND_ENVFILE):  case(SNDFILE_AND_BRKFILE):  case(SNDFILE_AND_DB_BRKFILE):
    case(TWO_ANALFILES):        case(ANAL_WITH_PITCHDATA):  case(ANAL_WITH_TRANSPOS):
    case(ANAL_AND_FORMANTS):    case(PITCH_AND_FORMANTS):   case(PITCH_AND_PITCH):
    case(PITCH_AND_TRANSPOS):   case(TRANSPOS_AND_TRANSPOS):case(SNDFILE_AND_UNRANGED_BRKFILE):
        dz->infilecnt = 2;
        break;
    case(THREE_ANALFILES):
//TW NEW CASE
    case(PFE):
        dz->infilecnt = 3;
        break;
    case(MANY_ANALFILES):
    case(MANY_SNDFILES):
        dz->infilecnt = -2; /* flags 2 or more */
        break;
    case(ONE_OR_MANY_SNDFILES):
//TW NEW CASE
    case(ONE_OR_MORE_SNDSYS):
    case(ANY_NUMBER_OF_ANY_FILES):
        dz->infilecnt = -1; /* flags 1 or more */
        break;
    default:
        sprintf(errstr,"Unknown input_process-type: establish_infile_constants()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** SETUP_PROCESS_LOGIC *********************************/

void setup_process_logic(int input,int processing,int output,dataptr dz)
{
    dz->input_data_type = input;
    dz->process_type    = processing;
    dz->outfiletype     = output;
}

/************************* SETUP_INPUT_PARAM_DEFAULTVALS *************************/
// RWD Apr 2011 added extra element
int setup_input_param_defaultval_stores(int tipc,aplptr ap)
{
    int n;
    tipc += 1;
    if((ap->default_val = (double *)malloc(tipc * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for application default values store\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<tipc;n++)
        ap->default_val[n] = 0.0;
    return(FINISHED);
}

/******************************************************************************/
/************************* FORMERLY IN internal.c *****************************/
/******************************************************************************/

/******** SET_INTERNALPARAM_DATA ***********************/

int set_internalparam_data(const char *this_paramlist,aplptr ap)
{
    int count;
    if((count = (int)strlen(this_paramlist))>0) {
        if((ap->internal_param_list = (char *)malloc((size_t)(count+1)))==NULL) {
            sprintf(errstr,"set_internalparam_data()\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->internal_param_list,this_paramlist);
    }
    ap->internal_param_cnt = (char ) count;        /*RWD added cast */
    return(FINISHED);
}

/******************************************************************************/
/************************* FORMERLY IN sinlib.c *******************************/
/******************************************************************************/

 /************************* READ_SAMPS **************************/
//TW removed NOTDEF: not certain how to get to not-NOTDEF option

int read_samps(float *bbuf,dataptr dz)
{
    if((dz->ssampsread = fgetfbufEx(bbuf, dz->buflen,dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Can't read samples from input soundfile.\n");
        return(SYSTEM_ERROR);
    }
    dz->samps_left -= dz->ssampsread;
    dz->total_samps_read += dz->ssampsread;


    return(FINISHED);
}

/*************************** READ_VALUES_FROM_ALL_EXISTING_BRKTABLES **************************/

int read_values_from_all_existing_brktables(double thistime,dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    int n;
    for(n=0;n<ap->total_input_param_cnt;n++) {
        if(dz->brksize[n]) {
            if((exit_status = read_value_from_brktable(thistime,n,dz))<0)
                return(exit_status);
        }
    }
    return(FINISHED);
}

/**************************** READ_VALUE_FROM_BRKTABLE *****************************/

int read_value_from_brktable(double thistime,int paramno,dataptr dz)
{
    double *endpair, *p, val;
    double hival, loval, hiind, loind;
    if(!dz->brkinit[paramno]) {
        dz->brkptr[paramno]   = dz->brk[paramno];
        dz->firstval[paramno] = *(dz->brk[paramno]+1);
        endpair               = dz->brk[paramno] + ((dz->brksize[paramno]-1)*2);
        dz->lastind[paramno]  = *endpair;
        dz->lastval[paramno]  = *(endpair+1);
        dz->brkinit[paramno] = 1;
    }
    p = dz->brkptr[paramno];
    if(thistime <= *(dz->brk[paramno])) {
        dz->param[paramno] = dz->firstval[paramno];
        if(dz->is_int[paramno])
            dz->iparam[paramno] = round(dz->param[paramno]);
        return(FINISHED);
    } else if(thistime >= dz->lastind[paramno]) {
        dz->param[paramno] = dz->lastval[paramno];
        if(dz->is_int[paramno])
            dz->iparam[paramno] = round(dz->param[paramno]);
        return(FINISHED);
    }
    if(thistime > *(p)) {
        while(*(p)<thistime)
            p += 2;
    } else {
        while(*(p)>=thistime)
            p -= 2;
        p += 2;
    }
    hival  = *(p+1);
    hiind  = *p;
    loval  = *(p-1);
    loind  = *(p-2);
    val    = (thistime - loind)/(hiind - loind);
    val   *= (hival - loval);
    val   += loval;
    dz->param[paramno] = val;
    if(dz->is_int[paramno])
        dz->iparam[paramno] = round(dz->param[paramno]);
    dz->brkptr[paramno] = p;
    return(FINISHED);
}

/************************** GET_TOTALAMP ***********************/

int get_totalamp(double *totalamp,float *sbuf,int wanted)
{
    int vc;
    *totalamp = 0.0;
    for(vc = 0; vc < wanted; vc += 2)
        *totalamp += sbuf[AMPP];
    return(FINISHED);
}

/******************************************************************************/
/************************* FORMERLY IN specialin.c ****************************/
/******************************************************************************/

/************** READ_NEW_FILENAME ***********/

int read_new_filename(char *filename,dataptr dz)
{
    if(dz->wordstor!=NULL) {
        if((dz->wordstor  = (char **)realloc(dz->wordstor,(dz->all_words+1) * sizeof(char *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to reallocate word stores.\n");
            return(MEMORY_ERROR);
        }
    } else {
        if(dz->all_words!=0 || dz->extra_word!=-1) {
            sprintf(errstr,"Array allocation error: read_new_filename().\n");
            return(PROGRAM_ERROR);
        }
        if((dz->wordstor  = (char **)malloc((dz->all_words+1) * sizeof(char *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create word stores.\n");
            return(MEMORY_ERROR);
        }
    }
    dz->extra_word = dz->all_words;
    dz->all_words++;
    if((dz->wordstor[dz->extra_word] = (char *)malloc((strlen(filename)+1) * sizeof(char)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create extra word store.\n");
        return(MEMORY_ERROR);
    }
    strcpy(dz->wordstor[dz->extra_word],filename);
    return(FINISHED);
}

/**************************** READ_SHUFFLE_DATA **************************/

int read_shuffle_data(int dmnparam,int imgparam,int mapparam,char *str,dataptr dz)
{
    int exit_status;
    char *domain, *image;
    if((exit_status = get_domain_and_image(str,&domain,&image))<0)
        return(exit_status);
    dz->iparam[dmnparam] = (int)strlen(domain);
    dz->iparam[imgparam] = (int)strlen(image);
    if((exit_status = consistent_domain_and_image(dmnparam,imgparam,domain,image,dz))!=TRUE)
        return(DATA_ERROR);
    return make_map(dmnparam,imgparam,mapparam,domain,image,dz);
}

/************************** GET_DOMAIN_AND_IMAGE ********************************/

int get_domain_and_image(char *str,char **domain, char **image)
{
    char *p = str, *q = str;

    while(*p!='-' && *p != ':') {
        if(*p == ENDOFSTR) {
            sprintf(errstr,"Bad string for shuffle data: separator missing\n");
            return(DATA_ERROR);
        } else if(!isalpha(*p)) {
            sprintf(errstr,"Invalid character ('%c') in string for shuffle data.\n",*p);
            return(DATA_ERROR);
        }
        p++;
    }
    if(p-q <=0) {
        sprintf(errstr,"Bad string for shuffle data: domain missing\n");
        return(DATA_ERROR);
    }
    *p = ENDOFSTR;
    *domain = q;
    p++;
    q = p;
    while(*p!=ENDOFSTR) {
        if(!isalpha(*p)) {
            sprintf(errstr,"Invalid character ('%d') in string for shuffle data.\n",*p);
            return(DATA_ERROR);
        }
        p++;
    }
    if(p-q <=0) {
        sprintf(errstr,"Bad string for shuffle data: image missing\n");
        return(DATA_ERROR);
    }
    *image = q;
    return(FINISHED);
}
                                                                
/************************** MAKE_MAP ********************************/

int make_map(int dmncnt,int imgcnt,int mapping,char *domain,char *image,dataptr dz)
{
    int n, m, OK;
    if((dz->iparray[mapping] = (int *)malloc(dz->iparam[imgcnt] * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for map.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->iparam[imgcnt];n++) {
        OK = 0;
        for(m=0;m<dz->iparam[dmncnt];m++) {
            if(image[n] == domain[m]) {
                dz->iparray[mapping][n] = m;
                OK = 1;
                break;
            }
        }
        if(!OK) {
            sprintf(errstr,"Programming Problem: no match in make_map().\n");
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/************************** CONSISTENT_DOMAIN_AND_IMAGE ********************************/

int consistent_domain_and_image(int dmncnt,int imgcnt,char *domain,char *image,dataptr dz)
{
    int n, m, OK;
    for(n=0;n<dz->iparam[dmncnt]-1;n++) {
        for(m=n+1;m<dz->iparam[dmncnt];m++) {
            if(domain[n]==domain[m]) {
                sprintf(errstr,"Duplicated symbol [%c] in domain string.\n",domain[n]);
                return(FALSE);
            }
        }
    }
    for(n=0;n<dz->iparam[imgcnt];n++) {
        OK = 0;
        for(m=0;m<dz->iparam[dmncnt];m++) {
            if(image[n] == domain[m]) {
                OK = 1;
                break;
            }
        }
        if(!OK) {
            sprintf(errstr,"Image symbol [%c] not in domain.\n",image[n]);
            return(FALSE);
        }
    }
    return(TRUE);
}

/******************************************************************************/
/************************* FORMERLY IN preprocess.c ***************************/
/******************************************************************************/

/***************************** FORCE_VALUE_AT_ZERO_TIME ******************************/

int force_value_at_zero_time(int paramno,dataptr dz)
{
    double *p, *pend;
    double thistime,lasttime,timediff,timeratio,thisval,lastval,valdiff,val;
    int n;
    if(flteq(dz->brk[paramno][0],0.0)) {
        dz->brk[paramno][0] = 0.0;
        return(FINISHED);
    }
    if(dz->brk[paramno][0] > 0.0) {
        if((dz->brk[paramno] = (double *)realloc((char *)(dz->brk[paramno]),(dz->brksize[paramno]+1) * 2 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to reallocate to force zero-time value.\n");
            return(MEMORY_ERROR);
        }
        for(n = (dz->brksize[paramno] * 2) - 1; n>=0;n--)
            dz->brk[paramno][n+2] = dz->brk[paramno][n];

        dz->brk[paramno][0] = 0.0;
        dz->brk[paramno][1] = dz->brk[paramno][3];
        dz->brksize[paramno]++;
    } else {
        p = dz->brk[paramno];
        pend = p + (dz->brksize[paramno] * 2);
        while(*p < 0.0) {
            p += 2;
            if(p >= pend) {
                p -= 2;
                memmove((char *)(dz->brk[paramno]),(char *)p,2 * sizeof(double));
                dz->brk[paramno][0] = 0.0;
                if((dz->brk[paramno] = (double *)realloc((char *)(dz->brk[paramno]),2 * sizeof(double)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate to force zero-time value.\n");
                    return(MEMORY_ERROR);
                }
                dz->brksize[paramno] = 1;
                return(FINISHED);
            }
        }
        if(*p > 0.0) {
            thistime  = *p;
            lasttime  = *(p-2);
            timediff  = thistime - lasttime;
            timeratio = -(lasttime)/timediff;   /* lasttime is -ve */
            thisval   = *(p+1);
            lastval   = *(p-1);
            valdiff   = thisval - lastval;
            val       = lastval + (valdiff * timeratio);
            p -= 2;
            *p     = 0.0;
            *(p+1) = val;
        }
        dz->brksize[paramno] = (pend - p)/2;
        memmove((char *)(dz->brk[paramno]),(char *)p,dz->brksize[paramno] * 2 * sizeof(double));
        if((dz->brk[paramno] = (double *)realloc((char *)(dz->brk[paramno]),dz->brksize[paramno] * 2 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to reallocate to force zero-time value.\n");
            return(MEMORY_ERROR);
        }
    }
    dz->brkptr[paramno] = dz->brk[paramno];
    return(FINISHED);
}

/***************************** INITIALISE_RANDOM_SEQUENCE ***************************/

void initialise_random_sequence(int seed_flagno,int seed_paramno,dataptr dz)
{
    if(dz->vflag[seed_flagno])
        srand((int)dz->iparam[seed_paramno]);
    else
        initrand48();
}

/******************** CONVERT_MSECS_TO_SECS **********************/

int convert_msecs_to_secs(int brktableno,dataptr dz)
{
    double *p, *pend;
    if(dz->brksize[brktableno]) {
        p = dz->brk[brktableno];
        pend = p + (dz->brksize[brktableno] * 2);
        while(p < pend) {
            *p *= MS_TO_SECS;
            p+=2;
        }
    } else
        dz->param[brktableno] *= MS_TO_SECS;
    return(FINISHED);
}

/***************************** ESTABLISH_BOTTOM_FRQS_OF_CHANNELS **************/

int establish_bottom_frqs_of_channels(dataptr dz)
{
    int cc;
    if((dz->windowbuf[CHBOT] = (float *)    realloc((char *)dz->windowbuf[CHBOT],dz->clength * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for channel bottom frqs array.\n");
        return(MEMORY_ERROR);
    }
    dz->windowbuf[CHBOT][0] = 0.0f;
    dz->windowbuf[CHBOT][1] = (float)dz->halfchwidth;
    for(cc = 2 ;cc < dz->clength; cc++)
        dz->windowbuf[CHBOT][cc] = (float)(dz->windowbuf[CHBOT][cc-1] + dz->chwidth);
    return(FINISHED);
}

/***************************** ESTABLISH_TESTTONE_AMPS *************************/

int establish_testtone_amps(dataptr dz)
{
    int n;
    if((dz->windowbuf[TESTPAMP] =
    (float *)realloc((char *)dz->windowbuf[TESTPAMP],PARTIALS_IN_TEST_TONE * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for testtone amp array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->windowbuf[TOTPAMP]  =
    (float *)realloc((char *)dz->windowbuf[TOTPAMP],PARTIALS_IN_TEST_TONE * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for testtone totalamp array.\n");
        return(MEMORY_ERROR);
    }
    dz->windowbuf[TOTPAMP][0] = dz->windowbuf[TESTPAMP][0] = 1.0f;
    for(n = 1; n < PARTIALS_IN_TEST_TONE; n++)    /* ACTUAL PARTIAL AMPS */
        dz->windowbuf[TESTPAMP][n]
                = (float)(dz->windowbuf[TESTPAMP][n-1] * PARTIAL_DECIMATION);
    for(n = 1; n < PARTIALS_IN_TEST_TONE; n++)    /* SUM OF PARTIAL AMPS */
        dz->windowbuf[TOTPAMP][n] =
            (float)(dz->windowbuf[TOTPAMP][n-1] + dz->windowbuf[TESTPAMP][n]);
    return(FINISHED);
}

/************************** SETUP_RING ******************************/

int setup_ring(dataptr dz)
{
    chvptr /*last,*/ head;
    if((dz->ringhead = (chvptr)malloc(sizeof(struct chanval)))==NULL)    {
        sprintf(errstr,"INSUFFICIENT MEMORY for ring buffer.\n");
        return(MEMORY_ERROR);
    }
    head = dz->ringhead;
    head->next = head;                         /* Initialise its pointers to self-point */
    head->last = head;
//  last = head;                                /* Mark current end of ring at ringhead */
    dz->ringsize = 1;
    return(FINISHED);
}

/************ SETUP_INTERNAL_BITFLAGS *************/

int setup_internal_bitflags(int bflag_array_no,int longpow,int divmask, dataptr dz)
{
    int exit_status;
     if((exit_status = log2_of_the_number_which_is_a_power_of_2(&(dz->iparam[longpow]),sizeof(int) * CHARBITSIZE))<0)
        return(exit_status);                                                    /* sizeof(int) in bytes as pow of 2 */
    dz->iparam[divmask]  = (int)(two_to_the_power_of(dz->iparam[longpow]) - 1); /* mask to calc MOD(longpow2) */
    return init_bitflags_to_zero(bflag_array_no,dz);                            /* Create and set-to-zero bitflags */
}

/********** LOG2_OF_THE_NUMBER_WHICH_IS_A_POWER_OF_2 *********/

int log2_of_the_number_which_is_a_power_of_2(int *n,int k)
{
    int mask = 1;
    int m = 0;
    if(k < 0) {
        sprintf(errstr,"-ve number submiited to log2_of_the_number_which_is_a_power_of_2()\n");
        return(PROGRAM_ERROR);
    }
    while(!(mask & k)) {
        mask <<= 1;
        m++;
    }
    *n = m;
    return(FINISHED);
}

/*********************** TWO_TO_THE_POWER_OF **********************
 *
 * Find 2-to-power-of-k.
 */

int two_to_the_power_of(int k)
{
    int n = 1;
    n <<= k;
    return(n);
}

/***************************** INIT_BITFLAGS_TO_ZERO ***************************/

int init_bitflags_to_zero(int bflag_array_no,dataptr dz)
{
    int n;
    int flgsize = (dz->clength/(sizeof(int) * CHARBITSIZE)) + 1;
    if((dz->lparray[bflag_array_no] = (int *)malloc(flgsize * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for bitflags.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<flgsize;n++)
        dz->lparray[bflag_array_no][n] = 0;
    return(FINISHED);
}

/****************************** SETUP_STABILITY_ARRAYS_AND_CONSTANTS **************/

int setup_stability_arrays_and_constants(int stability_val,int sval_less_one,dataptr dz)
{
    int n;
    if((dz->stable = (stabptr)malloc(sizeof(struct stability)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for stability array.\n");
        return(MEMORY_ERROR);
    }
    if(dz->wlength < dz->iparam[stability_val]) {
        sprintf(errstr,"File too short to use this program option.\nTry smaller value with '-s' (stability) flag.\n");
        return(DATA_ERROR);
    }
    dz->stable->offset = dz->iparam[stability_val]/2;   /* stabl is always ODD : gives middle window of windows buffer */

    if((dz->stable->sbufstore = (float *)malloc(((dz->wanted*dz->iparam[stability_val])+1) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for stability buffer store.\n");
        return(MEMORY_ERROR);
    }
    /* baktraking windows buf */
    if((dz->stable->sbuf = (float **)malloc((dz->iparam[stability_val]+1) * sizeof(float *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for stability backtracking buffer.\n");
        return(MEMORY_ERROR);
    }
    /* ptrs to wndws within it */
    dz->stable->sbuf[0]   = dz->stable->sbufstore;
    for(n=1;n<dz->iparam[stability_val]+1;n++)
        dz->stable->sbuf[n] = dz->stable->sbuf[n-1] + dz->wanted;
    if((dz->stable->specstore =
    (float *)malloc(((dz->infile->specenvcnt * dz->iparam[stability_val])+1) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for stability spectral store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->stable->spec      = (float **)malloc((dz->iparam[stability_val]+1) * sizeof(float*)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for stability spectrum store.\n");
        return(MEMORY_ERROR);
    }
    dz->stable->spec[0]   = dz->stable->specstore;
    for(n=1;n<dz->iparam[stability_val]+1;n++)
        dz->stable->spec[n] = dz->stable->spec[n-1] + dz->infile->specenvcnt;
    if((dz->stable->fpkstore  =
    (int *)malloc(((dz->iparam[stability_val] * dz->infile->specenvcnt)+1)*sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for stability peaks store.\n");
        return(MEMORY_ERROR);
    }
    /* baktraking spectral-peaks buf */
    if((dz->stable->fpk = (int **)malloc((dz->iparam[stability_val]+1) * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for stability peaks backtrack store.\n");
        return(MEMORY_ERROR);
    }
    /* pointers to peakstores within it */
    dz->stable->fpk[0]    = dz->stable->fpkstore;
    for(n=1;n<dz->iparam[stability_val]+1;n++)
        dz->stable->fpk[n] = dz->stable->fpk[n-1] + dz->infile->specenvcnt;
    if((dz->stable->total_pkcnt  = (int *)malloc((dz->iparam[stability_val]+1) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for stability peakcount store.\n");
        return(MEMORY_ERROR);
    }
    /* cnt of total peaks found per baktrak buf */
    if((dz->stable->design_score = (int *)malloc((dz->infile->specenvcnt+1) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for stability design score store.\n");
        return(MEMORY_ERROR);
    }
    /* array for designing filter */
    if((dz->stable->des = (desptr *)malloc((dz->infile->specenvcnt+1) * sizeof(desptr)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for stability design store.\n");
        return(MEMORY_ERROR);
    }
    dz->iparam[sval_less_one] = dz->iparam[stability_val]- 1;
            /* SET SAFETY VALUES */
    dz->stable->specstore[dz->infile->specenvcnt * dz->iparam[stability_val]] = 0.0f;
    dz->stable->spec[dz->iparam[stability_val]] = (float *)0;
    dz->stable->fpkstore[dz->iparam[stability_val] * dz->infile->specenvcnt] = 0;
    dz->stable->fpk[dz->iparam[stability_val]] = (int *)0;
    dz->stable->total_pkcnt[dz->iparam[stability_val]] = 0;
    dz->stable->design_score[dz->infile->specenvcnt] = 0;  /* array for designing filter */
    dz->stable->des[dz->infile->specenvcnt] = (desptr)0;
    dz->stable->sbufstore[dz->wanted * dz->iparam[stability_val]] = 0.0f;
    dz->stable->sbuf[dz->iparam[stability_val]] = 0;
    return(FINISHED);
}

/******************************************************************************/
/************************* FORMERLY IN procgrou.c *****************************/
/******************************************************************************/

/***************************** ZERO_SOUND_BUFFERS **************************/

int zero_sound_buffers(dataptr dz)
{
    int n;
    for(n=0;n<dz->bufcnt;n++)
        memset((char *)dz->sampbuf[n],0,(size_t)(dz->buflen * sizeof(float)));
    return(FINISHED);
}

/******************************************************************************/
/************************* FORMERLY IN procspec.c *****************************/
/******************************************************************************/

/***************************** RECTIFY_WINDOW ******************************/

void rectify_window(float *flbuf,dataptr dz)
{
    int vc;
    for(vc = 0; vc < dz->wanted; vc += 2) {
        if(flbuf[FREQ] < 0.0) {
            flbuf[FREQ] = -flbuf[FREQ];
// FEB 2010 TW
//          if(dz->is_rectified==FALSE) {
//              fprintf(stdout,"WARNING: Negative frq(s) in source data: rectified.\n");
//              fflush(stdout);
//              dz->is_rectified = TRUE;
//          }
        }
    }
}

/**************************** OUTER_LOOP ****************************/

int outer_loop(dataptr dz)
{
    int exit_status;
    int samps_read, got, windows_in_buf,peakscore = 0, pitchcnt = 0;
    int  in_start_portion = TRUE, least = 0, descnt = 0;
    dz->time = 0.0f;
    if(dz->bptrcnt <= 0) {
        sprintf(errstr,"flbufptr[0] not established by outer_loop()\n");
        return(PROGRAM_ERROR);
    }
    while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
        got = samps_read;
        dz->flbufptr[0] = dz->bigfbuf;
        windows_in_buf = got/dz->wanted;
        if((exit_status = inner_loop(&peakscore,&descnt,&in_start_portion,&least,&pitchcnt,windows_in_buf,dz))<0)
            return(exit_status);
        if((exit_status = write_exact_samps(dz->bigfbuf,samps_read,dz))<0)
            return(exit_status);
    }
    if(samps_read < 0) {
        sprintf(errstr,"Sound read error.\n");
        return(SYSTEM_ERROR);
    }
    return(FINISHED);
}

/*********************** INITIALISE_WINDOW_FRQS **********************
 *
 * Give start values for channel frqs in zero-amp 1st window.
 */

int initialise_window_frqs(dataptr dz)
{   int cc, vc;
    double thisfrq = GLIS_REFERENCE_FRQ;
    if(dz->mode!=SELFGLIS) {
        do {
            cc = (int)((thisfrq + dz->halfchwidth)/dz->chwidth);    /* TRUNCATE */
            vc = cc * 2;
            dz->flbufptr[0][FREQ] = (float)thisfrq;
            thisfrq += dz->chwidth;
        } while (thisfrq < dz->nyquist);
    }
    return(FINISHED);
}

/******************************** GET_AMP_AND_FRQ ******************************/

int get_amp_and_frq(float *floatbuf,dataptr dz)
{
    int cc, vc;

    for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2){
        dz->amp[cc]  = floatbuf[AMPP];
        dz->freq[cc] = floatbuf[FREQ];
    }
    return(FINISHED);
}

/******************************** PUT_AMP_AND_FRQ ******************************/

int put_amp_and_frq(float *floatbuf,dataptr dz)
{
    int cc, vc;

    for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2){
        floatbuf[AMPP] = dz->amp[cc];
        floatbuf[FREQ] = dz->freq[cc];
    }
    return(FINISHED);
}

/**************************** NORMALISE ***************************/

int normalise(double pre_totalamp,double post_totalamp,dataptr dz)
{
    double normaliser;
    int vc;

    if(post_totalamp < VERY_TINY_VAL) {
        if(!dz->zeroset) {
            fprintf(stdout,"WARNING: Zero-amp spectral window(s) encountered: orig window(s) substituted.\n");
            fflush(stdout);
            dz->zeroset = TRUE;
        }
    } else {
        normaliser = pre_totalamp/post_totalamp;    /* REMOVED a multiplier of 0.5 : APRIL 1998 */
        for(vc = 0; vc < dz->wanted; vc += 2)
            dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * normaliser);
    }
    return(FINISHED);
}

/*************************** INITIALISE_RING_VALS ***************************/

int initialise_ring_vals(int thissize,double initial_amp,dataptr dz)
{
    int exit_status;
    chvptr tthis;
    int change_in_size;
    if((change_in_size = thissize - dz->ringsize)!=0) {
        if(change_in_size>0) {
            if((exit_status = extend_ring(change_in_size,dz))<0)    /* If ring gets bigger,   extend it */
                return(exit_status);
        } else {
             if((exit_status = shrink_ring(-change_in_size,dz))<0)  /* If ring gets smaller,  shrink it */
                return(exit_status);
        }
    }
    tthis = dz->ringhead;
    do {                          /* Enter initval at all ring locations */
        tthis->val = (float)initial_amp;
    } while ((tthis = tthis->next) != dz->ringhead);
    dz->ringsize = thissize;
    return(FINISHED);
}

/************************* EXTEND_RING *****************************/

int extend_ring(int change_in_size,dataptr dz)
{   int n;
    chvptr tthis, head, last;
    head = dz->ringhead;
    last = head->last;         /* Mark current end of ring  */
    for(n=0;n<change_in_size;n++) {            /* create new ring locations */
        if((tthis = (chvptr)malloc(sizeof(struct chanval)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create noew ring buffer location.\n");
            return(MEMORY_ERROR);
        }
        last->next = tthis;  /* Connect new location to end of ring */
        tthis->last = last;
        tthis->next = head;/* Connect new location to start of ring */
        head->last = tthis;
        last = tthis;                    /* Mark curent end of ring */
    }
    return(FINISHED);
}

/************************* SHRINK_RING *****************************/

int shrink_ring(int change_in_size,dataptr dz)
{   int n;
    chvptr tthat, head, tthis;
    head = dz->ringhead;
    tthis = head->last;
    for(n=0;n<change_in_size;n++)
        tthis = tthis->last;                 /* Find new end of ring */
    tthat = tthis->next;        /* Point to start of redundant items */
    while(tthat!=head) {
        tthat = tthat->next;               /* Delete redundant items */
        free(tthat->last);
    }
    tthis->next = head;       /* Connect new-end-of-ring to ringhead */
    head->last = tthis;
    return(FINISHED);
}

/************************** IF_ONE_OF_LOUDEST_CHANS_STORE_IN_RING *****************************/

int if_one_of_loudest_chans_store_in_ring(int vc,dataptr dz)
{
    int exit_status;
    chvptr loudest = dz->ringhead;
    do {                                          /* COMPARE LOUDNESS WITH THOSE IN RING  */
        if(dz->flbufptr[0][vc] > loudest->val) {      /* IF LOUDER */
            if((exit_status = rearrange_ring_to_allow_new_entry_and_return_entry_address(&loudest,dz))<0)
                return(exit_status);
            loudest->loc = vc;
            loudest->val = dz->flbufptr[0][vc];
            break;
        }
    } while((loudest = loudest->next)!=dz->ringhead);
    return(FINISHED);
}

/************************** IF_ONE_OF_QUIETEST_CHANS_STORE_IN_RING *****************************/

int if_one_of_quietest_chans_store_in_ring(int vc,dataptr dz)
{
    int exit_status;
    chvptr quietest = dz->ringhead;
    do {                                           /* COMPARE QUIETNESS WITH THOSE IN RING */
        if(dz->flbufptr[0][vc] < quietest->val) {   /* IF QUIETER */
            if((exit_status = rearrange_ring_to_allow_new_entry_and_return_entry_address(&quietest,dz))<0)
                return(exit_status);
            quietest->loc = vc;
            quietest->val = dz->flbufptr[0][vc];
            break;
        }
    } while((quietest = quietest->next)!=dz->ringhead);
    return(FINISHED);
}

/**************** REARRANGE_RING_TO_ALLOW_NEW_ENTRY_AND_RETURN_ENTRY_ADDRESS *************
 *
 * Shuffle the items in the list of loudest/quietest chans.
 */

int rearrange_ring_to_allow_new_entry_and_return_entry_address(chvptr *here,dataptr dz)
{
    chvptr head = dz->ringhead;
    chvptr this, prethis, prehere;
    if(*here == head->last) /* If place of insertion is lastplace in ring */
        return(FINISHED);   /* Final item in ring just gets written over. */
    if(*here==head) {       /* IF place of insertion is start of ring.... */
        dz->ringhead = head->last;   /* Move head of ring back one place. */
        *here = dz->ringhead;        /* New value will be written there,  */
        return(FINISHED);
    }              /* automatically deleting previous final item in ring. */
            
    this          = head->last;  /* OTHERWISE: unlink last item from ring */
    prethis       = this->last;
    head->last    = prethis;
    prethis->next = head;
    
    prehere       = (*here)->last;/* Reuse adr-space ('this') by splicing */
    this->last    = prehere;   /* it into ring at current position (here) */
    prehere->next = this;
    this->next    = *here;
    (*here)->last = this;
    *here = this;     /* Return address of new current position ('this') */
    return(FINISHED);
}

/****************************** CHOOSE_BITFLAG_AND_RESET_MASK_IF_NESS ****************************/

int choose_bflagno_and_reset_mask_if_ness(int *bflagno,int cc,int *mask,int longpow2,int divmask)
{
    *bflagno = cc >> longpow2;      /* bflagno chooses which bitflag */
    if(!(cc & divmask))             /* If cc divisible by bitflag length */
        *mask = 1;                  /* reset bitmask to 1, for next bitflag */
    return(FINISHED);
}

/******************* MOVE_DATA_INTO_APPROPRIATE_CHANNEL *****************
 *
 *  If there turns out to be more than one partial to a channel,
 *  keep partial that was loudest in orig sound, retaining its
 *  loudness for further comparison.
 */

int move_data_into_appropriate_channel(int vc,int truevc,float thisamp,float thisfrq,dataptr dz)
{
    if(dz->flbufptr[0][AMPP] > dz->windowbuf[0][truevc]) {
        dz->windowbuf[0][truevc++] = thisamp;
        dz->windowbuf[0][truevc]   = thisfrq;
    }
    return(FINISHED);
}

/******************* MOVE_DATA_INTO_SOME_APPROPRIATE_CHANNEL *****************
 *
 * On assumption that frq does NOT have to be in exact channel!!
 * Replace simpler idea....above....
 * which tends to see several significant pieces of data disappear
 * as they overwrite one another in the same channel... BY ....
 *
 * (1) Put in correct empty channel IF POSSIBLE
 * (2) ELSE Put in an adj empty chan
 * (3)       below
 * (4)  ELSE above
 * (5) ELSE Replace val in true chan
 * (6) ELSE Replace val in adj chan
 * (7)       below
 * (8)  ELSE above
 *
 */

int move_data_into_some_appropriate_channel(int truevc,float thisamp,float thisfrq,dataptr dz)
{
#define THISCHANSCAN    (4)

    int minscan, maxscan, n;

    if(dz->windowbuf[0][truevc] == 0.0f) {          /* 1 */
        dz->windowbuf[0][truevc++] = thisamp;
        dz->windowbuf[0][truevc]   = thisfrq;
        return(FINISHED);
    }
    minscan = max(0,truevc - (THISCHANSCAN * 2));   /* 2 */
    maxscan = min(dz->clength-1,truevc + (THISCHANSCAN * 2));   /* 3 */
    for(n=truevc-2;n>minscan;n-=2) {
        if(dz->windowbuf[0][n] == 0.0f) {
            dz->windowbuf[0][n++] = thisamp;
            dz->windowbuf[0][n]   = thisfrq;
            return(FINISHED);
        }
    }
    for(n=truevc+2;n<maxscan;n+=2) {            /* 4 */
        if(dz->windowbuf[0][n] == 0.0f) {
            dz->windowbuf[0][n++] = thisamp;
            dz->windowbuf[0][n]   = thisfrq;
            return(FINISHED);
        }
    }
    if(thisamp > dz->flbufptr[0][truevc]) { /* 5 */
        dz->windowbuf[0][truevc++] = thisamp;
        dz->windowbuf[0][truevc]   = thisfrq;
        return(FINISHED);
    }
    for(n=truevc-2;n>minscan;n-=2) {            /* 6 */
        if(thisamp > dz->windowbuf[0][n]) { /* 7 */
            dz->windowbuf[0][n++] = thisamp;
            dz->windowbuf[0][n]   = thisfrq;
            return(FINISHED);
        }
    }
    for(n=truevc+2;n<maxscan;n+=2) {            /* 8 */
        if(thisamp > dz->windowbuf[0][n]) {
            dz->windowbuf[0][n++] = thisamp;
            dz->windowbuf[0][n]   = thisfrq;
            return(FINISHED);
        }
    }
    return(FINISHED);
}

/******************** GET_AMP ********************/

int get_amp(float *floatbuf,dataptr dz)
{
    int cc, vc;

    for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
        dz->amp[cc] = floatbuf[vc];
    return(FINISHED);
}

/******************** PUT_AMP ********************/

int put_amp(float *floatbuf,dataptr dz)
{
    int cc, vc;

    for(cc = 0, vc = 0; cc < dz->clength; cc++, vc += 2)
        floatbuf[vc] = dz->amp[cc];
    return(FINISHED);
}

/*************************** ADVANCE_ONE_2FILEINPUT_WINDOW ***********************/

int advance_one_2fileinput_window(dataptr dz)
{
    dz->flbufptr[0] += dz->wanted;
    dz->flbufptr[1] += dz->wanted;
    dz->total_windows++;
    dz->time = (float)(dz->time + dz->frametime);
    return(FINISHED);
}

/***************************** SPECBARE *********************************/

int specbare(int *pitchcnt,dataptr dz)
{
    int exit_status;
    double  error_range = SPAN_FACTOR * SEMITONE_INTERVAL;  /* +or- error in finding pitch of partial */
    double pitch;
    double thisfrq /* = pitch */, frq_close_above_next_partial, frq_close_below_next_partial;
    int n = 0;
    int vc = 0;
    double  one_over_error_range = 1.0/error_range;
    double  upper_freqdiff,lower_freqdiff;

    pitch = dz->pitches[(*pitchcnt)++];
    if(pitch < 0.0)             /*   UNPITCHED WINDOW : marked in specpich */
        return(FINISHED);
    upper_freqdiff = (pitch * error_range) - pitch;
    lower_freqdiff =  pitch - (pitch * one_over_error_range);

    while(vc < dz->wanted && (thisfrq = pitch * (double)++n) < dz->nyquist) {
        frq_close_above_next_partial = thisfrq + upper_freqdiff;
        frq_close_below_next_partial = thisfrq  - lower_freqdiff;
        if((exit_status = zero_channels_between_partials(&vc,frq_close_below_next_partial,dz))<0)
            return(exit_status);
        if(vc >= dz->wanted)
            break;
        if(dz->flbufptr[0][FREQ]>frq_close_above_next_partial)
            continue;
        else if(dz->vflag[BARE_LESS_BODY]) {
            if((exit_status = eliminate_all_but_loudest_channel_near_this_partial_frq
            (&vc,frq_close_above_next_partial,dz))<0)
                return(exit_status);
        } else {
            if((exit_status = keep_all_channels_near_partial_frq(&vc,frq_close_above_next_partial,dz))<0)
                return(exit_status);
        }
    }
    return(FINISHED);
}

/************** ZERO_CHANNELS_BETWEEN_PARTIALS ****************/

int zero_channels_between_partials(int *vvc,double frq_close_below_next_partial,dataptr dz)
{
    int vc = *vvc;
    while(dz->flbufptr[0][FREQ] < frq_close_below_next_partial) {
        dz->flbufptr[0][AMPP] = 0.0f;
        if((vc += 2) >= dz->wanted)
            break;
    }
    *vvc = vc;
    return(FINISHED);
}

/******** ELIMINATE_ALL_BUT_LOUDEST_CHANNEL_NEAR_THIS_PARTIAL_FRQ *********/

int eliminate_all_but_loudest_channel_near_this_partial_frq(int *vvc,double frq_close_above_next_partial,dataptr dz)
{
    int vc = *vvc;
    double maxamp = dz->flbufptr[0][AMPP];  /* SET MAXAMP TO 1ST IN-RANGE CH */
    int k, maxloc  = vc;                    /* NOTE NO. OF THE CH THUS MARKD */
    int   firstloc = vc;                    /* MARK FIRST CH IN SET          */
    while((vc += 2) < dz->wanted) {
        if(dz->flbufptr[0][FREQ]>frq_close_above_next_partial)      /* IF BEYOND CURRENT RANGE, STOP */
            break;
        if(dz->flbufptr[0][vc]>maxamp) {        /* IF LOUDER THAN MAX,RESET MAX  */
            maxamp = dz->flbufptr[0][AMPP]; /* AND RESET MAXAMP CHANNEL NO.  */
            maxloc = vc;
        }
    }
    for(k=firstloc; k<vc; k+=2) {           /* FOR THE WHOLE SET THUS DONE  */
        if(k!=maxloc)                       /* ZERO ALL CHS OTHER THAN MAX  */
            dz->flbufptr[0][k] = 0.0f;
    }
    *vvc = vc;
    return(FINISHED);
}

/*********************** KEEP_ALL_CHANNELS_NEAR_PARTIAL_FRQ ******************/

int keep_all_channels_near_partial_frq(int *vvc,double frq_close_above_next_partial,dataptr dz)
{
    int vc = *vvc;
    while((vc += 2) < dz->wanted) {
        if(dz->flbufptr[0][FREQ]>frq_close_above_next_partial)  /* IF BEYOND CURRENT RANGE, STOP */
            break;
    }
    *vvc = vc;
    return(FINISHED);
}

/****************************** GEN_AMPLITUDE_IN_LO_HALF_FILTERBAND ***************************/

int gen_amplitude_in_lo_half_filterband(double *thisamp,double thisfrq,double filt_centre_frq,dataptr dz)
{
    double val;

    val = -LOG2(thisfrq/filt_centre_frq); /* octave distance from filter centre-frq */
    val *= dz->scalefact;                /* convert to a fraction of halfbwidth (range 0-1) &
                                            convert to range 0-PI */
    val = cos(val);                      /* get cos val between 1 & -1 */
    val += 1.0;                          /* convert range to 2 -> 0 */
    val /= 2.0;                          /* convert range to 1 -> 0 */
    *thisamp = val;
    return(FINISHED);
}

/****************************** GEN_AMPLITUDE_IN_HI_HALF_FILTERBAND ***************************/

int gen_amplitude_in_hi_half_filterband(double *thisamp,double thisfrq,double filt_centre_frq,dataptr dz)
{
    double val;

    val = LOG2(thisfrq/filt_centre_frq);     /* octave distance from filter centre-frq */
    val *= dz->scalefact;                /* convert to a fraction of halfbwidth (range 0-1) &
                                            convert to range 0-PI */
    val = cos(val);                      /* get cos val between 1 & -1 */
    val += 1.0;                          /* convert range to 2 -> 0 */
    val /= 2.0;                          /* convert range to 1 -> 0 */
    *thisamp = val;
    return(FINISHED);
}

/****************************** FILTER_BAND_TEST ***************************
 *
 * CHeck that filter values are within range - and that there ARE values there!!!
 */

int filter_band_test(dataptr dz)
{
    int OK = 0, cc;
    for(cc=0;cc<dz->clength;cc++) {
        if(dz->fsampbuf[cc]>1.0 || dz->fsampbuf[cc]<0.0)  {
            sprintf(errstr,"filter contour out of range. (item[%d] = %f): filter_band_test()\n",cc,dz->fsampbuf[cc]);
            return(PROGRAM_ERROR);
        }
        if(dz->fsampbuf[cc] > 0.0)
            OK = 1;
    }
    if(!OK && !dz->fzeroset) {
            fprintf(stdout,"WARNING: ZERO filter contour at window %d time %lf\n",
            dz->total_windows+1,dz->total_windows * dz->frametime);
            fflush(stdout);
            dz->fzeroset = TRUE;
    } else
        dz->fzeroset = FALSE;
    return(FINISHED);
}

/************************** GET_STATECHANGES ************************/

int get_statechanges(int avcnt,int scantableno,int avpitcharrayno,int statechangearrayno,
                    double min_up_interval,double min_dn_interval,int datatype,dataptr dz)
{
#define RISING  (1)
#define FALLING (2)
#define ONLEVEL (3)

    int exit_status;
    int   n, window_cnt, pblok;
    double ttime = 0.0, intvl = 0.0;
    int    *bbrk;
    if((bbrk = (int *)malloc(avcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for statechange array.\n");
        return(MEMORY_ERROR);
    }
    memset((char *)bbrk,0,avcnt * sizeof(int));
    if(dz->brksize[scantableno]) {
        if((exit_status = read_value_from_brktable(ttime,scantableno,dz))<0)
            return(exit_status);
    }
    window_cnt = round(dz->param[scantableno]/dz->frametime);
    if((pblok = round(window_cnt/BLOKCNT))<2) {
        sprintf(errstr,"scanning-contour time_scale too short for this data.\n");
        return(DATA_ERROR);
    }
    if(pblok >= avcnt) {
        sprintf(errstr,"scanning-contour time_scale too long for this data.\n");
        return(DATA_ERROR);
    }
    for(n=pblok;n<avcnt;n++) {
        switch(datatype) {
        case(IS_PITCH): intvl = dz->parray[avpitcharrayno][n] - dz->parray[avpitcharrayno][n-pblok];
            break;
        case(IS_FRQ):   intvl = dz->parray[avpitcharrayno][n]/dz->parray[avpitcharrayno][n-pblok];
            break;
        }
        if(intvl > min_up_interval)
            bbrk[n] = RISING;
        else if(intvl < min_dn_interval)
            bbrk[n] = FALLING;
        else
            bbrk[n] = ONLEVEL;
        if(dz->brksize[scantableno]) {
             /* move along by an (averaged) BLOK of windows */
            ttime += dz->frametime * BLOKCNT;
            if((exit_status = read_value_from_brktable(ttime,scantableno,dz))<0)
                return(exit_status);
            window_cnt = round(dz->param[scantableno]/dz->frametime);
            if((pblok = round(window_cnt/BLOKCNT))<2) {
                sprintf(errstr,"scanning-contour time_scale too short for this data.\n");
                return(DATA_ERROR);
            }
        }
    }
    n = 0;
     /* Set types at very start, equal to actual first type already set */
    while(!bbrk[n]) {
        if(++n >= avcnt)
            break;
    }
    if(n<avcnt) {
        n--;
        while(n>=0) {
            bbrk[n] = bbrk[n+1];
            if(--n < 0)
                break;
        }
    }
    memset(dz->iparray[statechangearrayno],0,avcnt * sizeof(int));
    dz->iparray[statechangearrayno][0] = 1; /* initial state change */
    for(n=1;n<avcnt;n++) {
        if(bbrk[n]!=bbrk[n-1])
            dz->iparray[statechangearrayno][n] = 1;
    }
    free(bbrk);
    return(FINISHED);
}

/**************************** RECTIFY_FRQS ****************************/

int rectify_frqs(float *floatbuf,dataptr dz)
{
    int vc;
    for(vc = 0; vc < dz->wanted; vc += 2) {
        if(floatbuf[FREQ] < 0.0f)
            floatbuf[FREQ] = (float)(-(floatbuf[FREQ]));
    }
    return(FINISHED);
}

/******************************* MOVE_ALONG_FORMANT_BUFFER ************************/

int move_along_formant_buffer(dataptr dz)
{
    int samps_read;
    if((dz->flbufptr[1]  += dz->infile->specenvcnt) >= dz->flbufptr[3]) {
        if((samps_read = fgetfbufEx(dz->flbufptr[2],dz->buflen2,dz->ifd[1],0))<0) {
            sprintf(errstr,"fgetfbufEx failure, move_along_formant_buffer()\n");
            return(SYSTEM_ERROR);
        }
        if(samps_read <= 0)
            return(FINISHED);
        dz->flbufptr[1] = dz->flbufptr[2];
    }
    return(CONTINUE);
}

/*************************** GET_LONGER_FILE ***********************/

int get_longer_file(dataptr dz)
{
    int file_to_keep = 1;
    if(dz->insams[0] < dz->insams[1])
        /* allows larger infile2 to be copied after infile1 finished */
            file_to_keep = 2;
    return(file_to_keep);
}

/*************************** READ_BOTH_FILES ***********************/

int read_both_files(int *windows_in_buf,int *got,dataptr dz)
{
    int samps_read, samps_read2;
    memset((char *)dz->bigfbuf,0,dz->buflen * sizeof(float));
    if((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Failed to read data from first file.\n");
        return(SYSTEM_ERROR);
    }
    memset((char *)dz->flbufptr[2],0,dz->buflen* sizeof(float));
    if((samps_read2 = fgetfbufEx(dz->flbufptr[2], dz->buflen,dz->ifd[1],0)) < 0) {
        sprintf(errstr,"Failed to read data from second file.\n");
        return(SYSTEM_ERROR);
    }
    *got = max(samps_read,samps_read2);
    dz->flbufptr[0] = dz->bigfbuf;
    dz->flbufptr[1] = dz->flbufptr[2];
    *windows_in_buf = *got/dz->wanted;
    return(FINISHED);
}

/*************************** KEEP_EXCESS_BYTES_FROM_CORRECT_FILE ***********************/
#ifdef NOTDEF
int keep_excess_bytes_from_correct_file(int *bytes_to_write,int file_to_keep,int got,int wc,dataptr dz)
{
    int floats_to_keep = got - ((wc+1) * dz->wanted);
    if(file_to_keep) {
        if(file_to_keep==2 && floats_to_keep > 0)
            memmove((char *)dz->flbufptr[0],(char *)dz->flbufptr[1],floats_to_keep * sizeof(float));
        dz->flbufptr[0] += floats_to_keep;
    }
    *bytes_to_write = (dz->flbufptr[0] - dz->bigfbuf) * sizeof(float);
    return(FINISHED);
}
#else
/*************************** KEEP_EXCESS_SAMPS_FROM_CORRECT_FILE ***********************/

int keep_excess_samps_from_correct_file(int *samps_to_write,int file_to_keep,int got,int wc,dataptr dz)
{
    int floats_to_keep = got - ((wc+1) * dz->wanted);
    if(file_to_keep) {
        if(file_to_keep==2 && floats_to_keep > 0)
            memmove((char *)dz->flbufptr[0],(char *)dz->flbufptr[1],floats_to_keep * sizeof(float));
        dz->flbufptr[0] += floats_to_keep;
    }
//TW CORRECTED
//  *samps_to_write = (dz->flbufptr[0] - dz->bigfbuf) * sizeof(float);
    *samps_to_write =  dz->flbufptr[0] - dz->bigfbuf;
    return(FINISHED);
}




#endif
/*************************** READ_EITHER_FILE ***********************/

int read_either_file(int *samps_read,int file_to_keep,dataptr dz)
{
    if(file_to_keep==2) {
        if((*samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[1],0))<0) {
            sprintf(errstr,"Failed to read samples from file 2\n");
            return(SYSTEM_ERROR);
        }
    } else {
        if((*samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0))<0) {
            sprintf(errstr,"Failed to read samples from file 1\n");
            return(SYSTEM_ERROR);
        }
    }
    return(FINISHED);
}

/****************************** SCORE_PEAKS ******************************/

int score_peaks(int *peakscore,int sl1_var,int stabl_var,dataptr dz)
{
    int n, m;
    if(dz->total_windows>dz->iparam[sl1_var]) {
        for(n = 0;n < dz->stable->total_pkcnt[dz->iparam[sl1_var]]; n++) {      /* Add in the peaks in the last buf calcd */
            dz->stable->design_score[dz->stable->fpk[dz->iparam[sl1_var]][n]]++;            /* to the existing scores!! */
            (*peakscore)++;
        }
    } else {
        for(m=0;m<dz->iparam[stabl_var];m++) {                  /* For every stored array of peaks */
            for(n = 0;n < dz->stable->total_pkcnt[m]; n++) {    /* For every peak within that store */
                dz->stable->design_score[dz->stable->fpk[m][n]]++; /* Score a point for each occurence of a specenvchan */
                (*peakscore)++;                     /* Check to see if there are ANY peaks at all */
            }
        }
    }
    return(FINISHED);
 }

 /****************************** COLLECT_SCORES ******************************
 *
 *  If any channel is used as a peak (score > 0),store channel-no and score
 *  in a design_struct, and preset the amp_total to zero (see amplitude_poll).
 *  Count the total number of such peaks.
 */

int collect_scores(int *cnt,int *descnt,dataptr dz)
{
    int n;
    *cnt = 0;
    for(n=0;n<dz->infile->specenvcnt;n++) {
        if(dz->stable->design_score[n]) {
            if(*cnt >= *descnt) {       /* If this number of design_structs don't exist, make another */
                if((dz->stable->des[*cnt] = (desptr)malloc(sizeof(struct design)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY for scoring array.\n");
                    return(MEMORY_ERROR);
                }
                (*descnt)++;
            }
            dz->stable->des[*cnt]->chan  = n;
            dz->stable->des[*cnt]->score = dz->stable->design_score[n];
            dz->stable->des[*cnt]->amp   = 0.0;
            (*cnt)++;
        }
    }
    return(FINISHED);
}

/****************************** SORT_DESIGN ***************************
 *
 * Sort peak channels into descending order ofabundance
 */
int sort_design(int no_of_design_elements,dataptr dz)
{
    int n, m, a, b;
    for(n=1;n<no_of_design_elements;n++) {
        a = dz->stable->des[n]->chan;
        b = dz->stable->des[n]->score;
        m = n-1;
        while(m >= 0 && dz->stable->des[m]->score < b) {
            dz->stable->des[m+1]->chan  = dz->stable->des[m]->chan;
            dz->stable->des[m+1]->score = dz->stable->des[m]->score;
            m--;
        }
        dz->stable->des[m+1]->chan  = a;
        dz->stable->des[m+1]->score = b;
    }
    return(FINISHED);
}

/****************************** SORT_EQUIVALENT_SCORES ******************************
 *
 * For channels with equivalent scores. Find the bottom and top of the range of
 * channels with equivalent scores. Save those below the bottom of the range as
 * acceptable peaks (they have higher scores than those in the range).
 * For the rest, select those channels having the maximum amplitude over the
 * whole timespan of the stabilise buffers.
 */

int sort_equivalent_scores(int this_pkcnt,dataptr dz)
{
    int bot, top, n;
    if(dz->stable->des[dz->itemcnt-1]->score < dz->stable->des[dz->itemcnt]->score) {
        sprintf(errstr,"Problem in peak sorting: sort_equivalent_scores()\n");
        return(PROGRAM_ERROR);
    }
    n = dz->itemcnt-1;
    while(n >= 0) {
        if(dz->stable->des[n]->score == dz->stable->des[dz->itemcnt]->score)
            n--;
        else
            break;
    }
    bot = n+1;
    n = dz->itemcnt+1;
    while(n <this_pkcnt) {
        if(dz->stable->des[n]->score == dz->stable->des[dz->itemcnt]->score)
            n++;
        else
            break;
    }
    top = n;
    for(n=0;n<bot;n++)
        dz->peakno[n] = dz->stable->des[n]->chan; /* keep peaks that have definitely higher scores */
    return do_amplitude_poll(bot,top,dz);
}

/****************************** DO_AMPLITUDE_POLL ***************************/

int do_amplitude_poll(int bot,int top,dataptr dz)
{
    int n, m, here = 0, thischan;
    int nextpeak = bot;             /* peaknumber of first of peaks still to allocate */
    double thisamp, ampmax;
    for(n=bot;n<top;n++) {              /* For every channel in the set of possible channels */
        thischan = dz->stable->des[n]->chan;
        for(m=0;m<dz->iparam[FOCU_STABL];m++)       /* sum amplitudes in that chan across all specenv buffers */
            dz->stable->des[n]->amp += dz->stable->spec[m][thischan];   /* and store in des-array assocd with channel */
    }
    while(nextpeak < dz->itemcnt) {         /* while there are still peaks to be chosen */
        ampmax = 0.0;
        for(n=bot;n<top;n++) {          /* Find maximum among these amplitude-totals */
            if((thisamp = dz->stable->des[n]->amp)>ampmax) {
                ampmax = thisamp;
                here   = n;
            }
        }
        dz->peakno[nextpeak++] = dz->stable->des[here]->chan; /* Set next peak channel to this maxamp channel */
        dz->stable->des[here]->amp = 0.0;             /* zero amptotal in chosen des, to eliminate from next search */
    }
    return(FINISHED);
}
 
/****************************** UNSCORE_PEAKS ******************************/

int unscore_peaks(int *peakscore,dataptr dz)
{
    int n;
    for(n = 0;n < dz->stable->total_pkcnt[0]; n++) {    /* Subtract score of peaks buffer no longer in use */
        dz->stable->design_score[dz->stable->fpk[0][n]]--;          /* from the existing scores!! */
        (*peakscore)--;
    }
    return(FINISHED);
}

/******************************************************************************/
/************************* FORMERLY IN pconsistency.c *************************/
/******************************************************************************/

static void setup_pitchzero(int paramno,dataptr dz);

/************************** HANDLE_PITCH_ZEROS ********************************/

void handle_pitch_zeros(dataptr dz)
{
    switch(dz->process) {
    case(ARPE):
        setup_pitchzero(ARPE_LOFRQ,dz);
        setup_pitchzero(ARPE_HIFRQ,dz);
        break;
    case(CHORD):
        setup_pitchzero(CHORD_LOFRQ,dz);
        setup_pitchzero(CHORD_HIFRQ,dz);
        break;
    case(CLEAN):
        if(dz->mode==FILTERING)
            setup_pitchzero(CL_FRQ,dz);
        break;
    case(FOCUS):
        setup_pitchzero(FOCU_LOFRQ,dz);
        setup_pitchzero(FOCU_HIFRQ,dz);
        break;
    case(FOLD):
        setup_pitchzero(FOLD_LOFRQ,dz);
        setup_pitchzero(FOLD_HIFRQ,dz);
        break;
    case(FORM):
        setup_pitchzero(FORM_FTOP,dz);
        setup_pitchzero(FORM_FBOT,dz);
        break;
    case(MEAN):
        setup_pitchzero(MEAN_LOF,dz);
        setup_pitchzero(MEAN_HIF,dz);
        break;
    case(P_FIX):
        setup_pitchzero(PF_LOF,dz);
        setup_pitchzero(PF_HIF,dz);
        break;
    case(PEAK):
        setup_pitchzero(PEAK_CUTOFF,dz);
        break;
    case(REPORT):
        setup_pitchzero(REPORT_LOFRQ,dz);
        setup_pitchzero(REPORT_HIFRQ,dz);
        break;
    case(SHIFTP):
        setup_pitchzero(SHIFTP_FFRQ,dz);
        break;
    case(STRETCH):
        setup_pitchzero(STR_FFRQ,dz);
        break;
    case(S_TRACE):
        setup_pitchzero(TRAC_LOFRQ,dz);
        setup_pitchzero(TRAC_HIFRQ,dz);
        break;
    case(TRNSF):
        setup_pitchzero(TRNSF_HIFRQ,dz);
        setup_pitchzero(TRNSF_LOFRQ,dz);
        break;
    case(TRNSP):
        setup_pitchzero(TRNSP_HIFRQ,dz);
        setup_pitchzero(TRNSP_LOFRQ,dz);
        break;
    case(TUNE):
        setup_pitchzero(TUNE_BFRQ,dz);
        break;
    case(VOCODE):
        setup_pitchzero(VOCO_LOF,dz);
        setup_pitchzero(VOCO_HIF,dz);
        break;
    case(WAVER):
        setup_pitchzero(WAVER_LOFRQ,dz);
        break;
    }
}

/****************************** SETUP_PITCHZERO *******************************/

void setup_pitchzero(int paramno,dataptr dz)
{
    double *p, *end;
    if(dz->brksize[paramno]) {
        p   = dz->brk[paramno] + 1;
        end = dz->brk[paramno] + (dz->brksize[paramno] * 2);
        while(p < end) {
            if(*p < SPEC_MINFRQ)
                *p = SPEC_MINFRQ;
            p+=2;
        }
    } else if(dz->param[paramno] < SPEC_MINFRQ)
        dz->param[paramno] = 0.0;
}

/************************************ CHECK_DEPTH_VALS *************************************/

int check_depth_vals(int param_no,dataptr dz)
{
    double *p;
    int n = 0;
    if(dz->brksize[param_no]==0) {
        if(flteq(dz->param[param_no],0.0)) {
            sprintf(errstr,
                "A non-varying depth value of zero will not change your source file.\n");
            return(DATA_ERROR);
        }
    } else {
        p = dz->brk[param_no]+1;
        n = 0;
        while(n < dz->brksize[param_no]) {
            if(!flteq(*p,0.0))
                return(FINISHED);
            p += 2;
            n++;
        }
        sprintf(errstr,
            "A non-varying depth value of zero will not change your source file.\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/*********************** CONVERT_SHIFTP_VALS ***********************/

void convert_shiftp_vals(dataptr dz)
{
    switch(dz->mode) {
    case(P_SHFT_UP_AND_DN):
        dz->param[SHIFTP_SHF1] *= OCTAVES_PER_SEMITONE;
        dz->param[SHIFTP_SHF1] = pow(2.0,dz->param[SHIFTP_SHF1]);
        dz->param[SHIFTP_SHF2] *= OCTAVES_PER_SEMITONE;
        dz->param[SHIFTP_SHF2] = pow(2.0,dz->param[SHIFTP_SHF2]);
        break;
    case(P_SHFT_DN):
        dz->param[SHIFTP_SHF1] *= OCTAVES_PER_SEMITONE;
        dz->param[SHIFTP_SHF1] = pow(2.0,dz->param[SHIFTP_SHF1]);
        break;
    case(P_SHFT_UP):
        dz->param[SHIFTP_SHF1] *= OCTAVES_PER_SEMITONE;
        dz->param[SHIFTP_SHF1] = pow(2.0,dz->param[SHIFTP_SHF1]);
        break;
    }
}

/******************************************************************************/
/************************* FORMERLY IN buffers.c ******************************/
/******************************************************************************/


/*************************** CREATE_SNDBUFS **************************/

/* 2009 MULTICHANNEL */
/* TW update Nov 2022, correct buflen for large anal chans */
int create_sndbufs(dataptr dz)
{
    int n;
    size_t bigbufsize;
    int framesize;
    framesize = F_SECSIZE * dz->infile->channels;
    if(dz->sbufptr == 0 || dz->sampbuf==0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = (size_t) Malloc(-1);
    bigbufsize /= dz->bufcnt;
    if(bigbufsize <=0)
        bigbufsize  = framesize * sizeof(float);

    dz->buflen = (int)(bigbufsize / sizeof(float));
    // NEW TW NOV 26 : 2022 -->
    if(dz->buflen < framesize)
        dz->buflen = framesize;
    else
        // <-- NEW TW NOV 26 : 2022
        dz->buflen = (dz->buflen / framesize)  * framesize;
    bigbufsize = dz->buflen * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize  * dz->bufcnt)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    for(n=0;n<dz->bufcnt;n++)
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    return(FINISHED);
}

/**************************** ALLOCATE_SINGLE_BUFFER ******************************/
//TW REVISED: buffers no longer multiples of secsize
int allocate_single_buffer(dataptr dz)
{
//  int bigbufsize;
//  int exit_status;
    unsigned int buffersize;
    if(dz->bptrcnt <= 0) {
        sprintf(errstr,"bufptr not established in allocate_single_buffer()\n");
        return(PROGRAM_ERROR);
    }
//TW
//  buffersize = dz->sampswanted * BUF_MULTIPLIER;
    buffersize = dz->wanted * BUF_MULTIPLIER;
//TW MOVED THIS LINE: to get correct value of buflen (NOT after the +1)
    dz->buflen = buffersize;
    buffersize += 1;
    if((dz->bigfbuf = (float*) malloc(buffersize * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize = dz->buflen;
    dz->bigfbuf[dz->big_fsize] = 0.0f;  /* safety value */
    return(FINISHED);
}

/**************************** ALLOCATE_DOUBLE_BUFFER ******************************/
/* RWD MUST recheck this! */

int allocate_double_buffer(dataptr dz)
{
//  int exit_status;
    unsigned int buffersize;
    if(dz->bptrcnt < 4) {
        sprintf(errstr,"Insufficient bufptrs established in allocate_double_buffer()\n");
        return(PROGRAM_ERROR);
    }
//TW REVISED: buffers don't need to be multiples of secsize
    buffersize = dz->wanted * BUF_MULTIPLIER;
    dz->buflen = buffersize;
    if((dz->bigfbuf = (float*)malloc(dz->buflen*2 * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize = dz->buflen;
    dz->flbufptr[2]  = dz->bigfbuf + dz->big_fsize;
    dz->flbufptr[3]  = dz->flbufptr[2] + dz->big_fsize;
    return(FINISHED);
}

/**************************** ALLOCATE_TRIPLE_BUFFER ******************************/

int allocate_triple_buffer(dataptr dz)
{
    unsigned int buffersize;
    if(dz->bptrcnt < 5) {
        sprintf(errstr,"Insufficient bufptrs established in allocate_triple_buffer()\n");
        return(PROGRAM_ERROR);
    }
//TW REVISED: buffers don't need to be multiples of secsize
    buffersize = dz->wanted;
    dz->buflen = buffersize;
    if((dz->bigfbuf = (float*)malloc(dz->buflen*3 * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize = dz->buflen;
    dz->flbufptr[2]  = dz->bigfbuf + dz->big_fsize;
    dz->flbufptr[3]  = dz->flbufptr[2] + dz->big_fsize;
    dz->flbufptr[4]  = dz->flbufptr[3] + dz->big_fsize;
    return(FINISHED);
}

/*************************** ALLOCATE_ANALDATA_PLUS_FORMANTDATA_BUFFER ****************************/

int allocate_analdata_plus_formantdata_buffer(dataptr dz)
{
    int exit_status;
    unsigned int buffersize;
    /*int cnt = 0;*/
    if(dz->bptrcnt < 4) {
        sprintf(errstr,"Insufficient bufptrs established in allocate_analdata_plus_formantdata_buffer()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->infile->specenvcnt==0) {
        sprintf(errstr,"specenvcnt not set: allocate_analdata_plus_formantdata_buffer()\n");
        return(PROGRAM_ERROR);
    }
    if((exit_status = calculate_analdata_plus_formantdata_buffer(&buffersize,dz))<0)
        return(exit_status);

    if((dz->bigfbuf = (float*)malloc((size_t)buffersize * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize  = /*dz->bigbufsize/sizeof(float);*/dz->buflen;
    dz->flbufptr[2] = dz->bigfbuf + dz->big_fsize;
    dz->flbufptr[3] = dz->flbufptr[2] + /*(dz->bigbufsize2/sizeof(float));*/ dz->buflen2;
    return(FINISHED);
}

/******************************* CALCULATE_ANALDATA_PLUS_FORMANTDATA_BUFFER **************************/

int calculate_analdata_plus_formantdata_buffer(unsigned int *buffersize,dataptr dz)
{
//  int exit_status;
    int cnt = 0;
    unsigned int orig_buffersize = 0;
    dz->buflen2 = 0;
//TW REVISED: no secsize-multiple restriction on buffers
    *buffersize = (dz->wanted + dz->infile->specenvcnt) * BUF_MULTIPLIER;
    while(dz->buflen2 < dz->descriptor_samps) {
        if(cnt == 0)
            orig_buffersize = *buffersize;
        else
            *buffersize += orig_buffersize;
        cnt++;
        dz->buflen  = dz->wanted * BUF_MULTIPLIER * cnt;
        dz->buflen2 = dz->infile->specenvcnt * BUF_MULTIPLIER * cnt;
    }
    return(FINISHED);
}

/******************************* ALLOCATE_SINGLE_BUFFER_PLUS_EXTRA_POINTER **************************/

int allocate_single_buffer_plus_extra_pointer(dataptr dz)
{
    int exit_status;
    if((exit_status = allocate_single_buffer(dz))<0)
        return(exit_status);
    if(dz->bptrcnt < 2) {
        sprintf(errstr,"dz->flbufptr[1] not established: allocate_larger_buffers()\n");
        return(PROGRAM_ERROR);
    }
    dz->flbufptr[1] = dz->bigfbuf + dz->big_fsize;
    return(FINISHED);
}

/******************************************************************************/
/************************* FORMERLY IN cmdline.c *****************************/
/******************************************************************************/

/************************** GET_PROCESS_AND_MODE_FROM_CMDLINE *****************************/

int get_process_and_mode_from_cmdline(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int exit_status;
    if((exit_status = get_process_no((*cmdline)[0],dz))<0)
        return(exit_status);
    (*cmdline)++;
    (*cmdlinecnt)--;
    
    if((dz->maxmode = get_maxmode(dz->process))<0)
        return(PROGRAM_ERROR);
    if(dz->maxmode > 0) {
        if(*cmdlinecnt<=0) {
            sprintf(errstr,"Too few commandline parameters.\n");
            return(USAGE_ONLY);
        }
        if((exit_status = get_mode_from_cmdline((*cmdline)[0],dz))<0)
            return(exit_status);
        (*cmdline)++;
        (*cmdlinecnt)--;
    }
    return(FINISHED);
}

/****************************** GET_MODE *********************************/

int get_mode_from_cmdline(char *str,dataptr dz)
{
    if(sscanf(str,"%d",&dz->mode)!=1) {
        sprintf(errstr,"Cannot read mode of program.\n");
        return(USAGE_ONLY);
    }
    if(dz->mode <= 0 || dz->mode > dz->maxmode) {
        sprintf(errstr,"Program mode value [%d] is out of range [1 - %d].\n",dz->mode,dz->maxmode);
        return(USAGE_ONLY);
    }
    dz->mode--;     /* CHANGE TO INTERNAL REPRESENTATION OF MODE NO */
    return(FINISHED);
}

/******************************************************************************/
/************************* FORMERLY IN usage.c ********************************/
/******************************************************************************/


/******************************** USAGE1 ********************************/

//int usage(int argc,char *argv[])
//{
//    switch(argc) {
//    case(1): return usage1();
//    case(2): return usage2(argv[1]);
//    case(3): return usage3(argv[1],argv[2]);
//    }
//    sprintf(errstr,"Incorrect call to usage()\n");
//    return(PROGRAM_ERROR);
//}

int usage(int argc, char *argv[])
{
    switch (argc) {
        case 1: return usage1();
        case 2: return usage2(argv[1]);
        case 3:
            fprintf(stderr, "Insufficient parameters on command line.\n");
            return USAGE_ONLY;
    }
    sprintf(errstr, "Incorrect call to usage()\n");
    return PROGRAM_ERROR;
}

/******************************** USAGE1 ********************************/

//TW NEW CODE BELOW
/******************************** GET_THE_VOWELS ********************************/

int get_the_vowels(char *filename,double **times,int **vowels,int *vcnt,dataptr dz)
{
    FILE *fp;
    double *t, lasttime = 0.0, endtime;
    int *v, vv;
    int only_one_vowel = 0, non_zero_start = 0;
    int arraysize = BIGARRAY, n = 0;
    char temp[200], *q, *p;
    int istime;

    if((vv = get_vowel(filename))>=0) {
        if((*times = (double *)malloc(2 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for time data.\n");
            return(MEMORY_ERROR);
        }
        if((*vowels = (int *)malloc(2 * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for vowels data.\n");
            return(MEMORY_ERROR);
        }
        (*times)[0] = 0.0;
        (*times)[1] = 0.0;
        (*vowels)[0] = vv;
        (*vowels)[1] = vv;
        return(FINISHED);
    }
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr, "Can't open textfile %s to read vowel data.\n",filename);
        return(DATA_ERROR);
    }
    if((*times = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for time data.\n");
        return(MEMORY_ERROR);
    }
    if((*vowels = (int *)malloc(arraysize * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for vowels data.\n");
        return(MEMORY_ERROR);
    }
    t = *times;
    v = *vowels;
    istime = 1;
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_word_from_string(&q,&p)) {
            if(istime) {
                if(sscanf(p,"%lf",t)!=1) {
                    sprintf(errstr,"No time for time-vowel pair %d\n",n+1);
                    return(DATA_ERROR);
                }
                if(n==0) {
                    if(*t < 0.0) {
                        sprintf(errstr,"First time is less than zero in vowel data\n");
                        return(DATA_ERROR);
                    } else if(*t > 0.0) {
                        print_outwarning_flush("FIRST TIME in vowel data IS NOT ZERO : assuming first vowel runs from time zero\n");
                        non_zero_start = 1;
                        t++;
                        *t = *(t-1);
                        *(t-1) = 0.0;
                    }
                } else {
                    if (*t <= lasttime) {
                        sprintf(errstr,"Times do not advance (from %lf to %lf at pair %d) in vowel data\n",
                        lasttime,*t,n+1);
                        return(DATA_ERROR);
                    }
                }
                lasttime = *t++;
            } else {
                if((*v = get_vowel(p))<0) {
                    sprintf(errstr,"Unrecognised vowel string %s at pair %d in vowel datafile\n",p,n+1);
                    return(DATA_ERROR);
                }
                if((n==0) && non_zero_start) {
                    v++;
                    *v = *(v-1);
                    n++;
                }
                v++;
                if(++n >= arraysize) {
                    arraysize += BIGARRAY;
                    if((*times = (double *)realloc((char *)(*times),arraysize * sizeof(double)))==NULL) {
                        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate table of vowel-times.\n");
                        return(MEMORY_ERROR);
                    }
                    if((*vowels = (int *)realloc((char *)(*vowels),arraysize * sizeof(int)))==NULL) {
                        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate table of vowels.\n");
                        return(MEMORY_ERROR);
                    }
                    t = *times + n;
                    v = *vowels + n;
                }
            }
            istime = !istime;
        }
    }
    if(n == 0) {
        sprintf(errstr,"No data in vowel datafile %s\n",filename);
        return(DATA_ERROR);
    }
    if(!istime) {
        sprintf(errstr,"data in  vowel datafile %s not paired correctly\n",filename);
        return(DATA_ERROR);
    }
    endtime = (dz->wlength + 1) * dz->frametime;
    if(n == 1) {
        only_one_vowel = 1;
        *t = endtime;
        *v = *(v-1);
        n = 2;
    } else {
        if(*(t-1) < endtime) {  /* Force a time-value beyond end of infile */
            *t = endtime;
            *v = *(v-1);
            n++;
        }
    }
    if((*times = (double *)realloc((char *)(*times),n * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate vowel-time table.\n");
        return(MEMORY_ERROR);
    }
    if((*vowels = (int *)realloc((char *)(*vowels),n * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate vowels table.\n");
        return(MEMORY_ERROR);
    }
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
        fflush(stdout);
    }
    if(only_one_vowel)
        *vcnt = 0;
    else
        *vcnt = n;
    return(FINISHED);
}

/******************************** GET_VOWEL ********************************/

int get_vowel (char *str)
{
    if     (!strcmp(str,"ee"))  return VOWEL_EE;
    else if(!strcmp(str,"i"))   return VOWEL_I;
    else if(!strcmp(str,"ai"))  return VOWEL_AI;
    else if(!strcmp(str,"aii")) return VOWEL_AII;
    else if(!strcmp(str,"e"))   return VOWEL_E;
    else if(!strcmp(str,"a"))   return VOWEL_A;
    else if(!strcmp(str,"ar"))  return VOWEL_AR;
    else if(!strcmp(str,"o"))   return VOWEL_O;
    else if(!strcmp(str,"oo"))  return VOWEL_OO;
    else if(!strcmp(str,"oa"))  return VOWEL_OA;
    else if(!strcmp(str,"or"))  return VOWEL_OR;
    else if(!strcmp(str,"u"))   return VOWEL_U;
    else if(!strcmp(str,"ui"))  return VOWEL_UI;
    else if(!strcmp(str,"uu"))  return VOWEL_UU;
    else if(!strcmp(str,"xx"))  return VOWEL_XX;
    else if(!strcmp(str,"x"))   return VOWEL_X;
    else if(!strcmp(str,"n"))   return VOWEL_N;
    else if(!strcmp(str,"m"))   return VOWEL_M;
    else if(!strcmp(str,"r"))   return VOWEL_R;
    else if(!strcmp(str,"th"))  return VOWEL_TH;
    return -1;
}

/************************** CLOSE_AND_DELETE_TEMPFILE ***************************
 *
 * The input file is a temporary file only: needs to be deleted
 *
 */

int close_and_delete_tempfile(char *newfilename,dataptr dz)
{
    /*RWD Nov 2003: use sndunlink before calling this for soundfiles! */
    if(sndunlink(dz->ifd[0])){
        fprintf(stdout, "WARNING: Can't delete temporary file %s.\n",newfilename);
        fflush(stdout);
    }

    if(sndcloseEx(dz->ifd[0]) < 0) {
        fprintf(stdout, "WARNING: Can't close temporary file %s\n",newfilename);
        fflush(stdout);
    } else {
        dz->ifd[0] = -1;
    }
#ifdef NOTDEF
    if(remove(newfilename) < 0) {
        fprintf(stdout, "WARNING: Can't delete temporary file %s.\n",newfilename);
        fflush(stdout);
    }
#endif
    return FINISHED;
}