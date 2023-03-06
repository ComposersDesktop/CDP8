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
 License aint with the CDP System; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 *
 */
/* floatsam version*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <grain.h>
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

#define FSECSIZE (256)
#define maxtime scalefact

#ifdef unix
#define round(x) lround((x))
#endif

static int  create_grain_sndbufs(dataptr dz);
static void establish_grain_envelope_windowsize_in_samps(dataptr dz);
static int create_rrr_sndbufs(dataptr dz);
static int  check_grain_consistency(dataptr dz);
static int  get_reorder_sequence(char *str,dataptr dz);
static int  get_grain_ratios(int special,char *filename,dataptr dz);
static int  get_two_grain_ratios(char *filename,dataptr dz);
static int  get_grain_synctimes(char *filename,dataptr dz);
static int  generate_samp_windowsizer(dataptr dz);
static int  create_ssss_sndbufs(dataptr dz);

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
//  int is_spec = FALSE;
    dz->extra_bufcnt = -1;  /* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
    dz->bptrcnt = 0;
    dz->bufcnt  = 0;
    switch(dz->process) {
    case(GRAIN_COUNT):    
    case(GRAIN_GET):
    case(GRAIN_ASSESS):
        dz->extra_bufcnt = 0;   dz->bufcnt = 2;     
        break;
    case(GRAIN_OMIT):     
    case(GRAIN_DUPLICATE): 
    case(GRAIN_REORDER):
    case(GRAIN_REPITCH):  
    case(GRAIN_TIMEWARP):
    case(GRAIN_REVERSE):
        dz->extra_bufcnt = 2;   dz->bufcnt = 3;     
        break;
    case(GRAIN_RERHYTHM):    
    case(GRAIN_REMOTIF):
    case(GRAIN_POSITION):    
    case(GRAIN_ALIGN):          
        dz->extra_bufcnt = 3;   dz->bufcnt = 3;     
        break;
    case(RRRR_EXTEND):          
    case(SSSS_EXTEND):          
        dz->extra_bufcnt = 0;   dz->bufcnt = 2;     
        break;
    case(GREV):
        dz->extra_bufcnt = 0;
        if(dz->mode == GREV_TSTRETCH || dz->mode == GREV_PUT)
            dz->bufcnt = 3;
        else
            dz->bufcnt = 2;
        break;
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
    dz->ptr_cnt    = -1;        /* base constructor...process */
    dz->array_cnt  = -1;
    dz->iarray_cnt = -1;
    dz->larray_cnt = -1;
    switch(dz->process) {
    case(GRAIN_COUNT):
    case(GRAIN_ASSESS):
        dz->array_cnt = 2; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 4; dz->fptr_cnt = 0; break;
    case(GRAIN_REORDER):
        dz->array_cnt = 3; dz->iarray_cnt = 1; dz->larray_cnt = 2; dz->ptr_cnt = 4; dz->fptr_cnt = 0; break;
    case(GRAIN_RERHYTHM):   
    case(GRAIN_REPITCH):
    case(GRAIN_REMOTIF):
        dz->array_cnt = 4; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 4; dz->fptr_cnt = 0; break;
    case(GRAIN_ALIGN):  
    case(GRAIN_GET):        
    case(GRAIN_POSITION):
        dz->array_cnt = 4; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 4; dz->fptr_cnt = 0; break;
    case(GRAIN_OMIT):       
    case(GRAIN_DUPLICATE):  
    case(GRAIN_TIMEWARP):
        dz->array_cnt = 3; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 4; dz->fptr_cnt = 0; break;
    case(GRAIN_REVERSE):
        dz->array_cnt = 3; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 4; dz->fptr_cnt = 0; break;
    case(RRRR_EXTEND):
        dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
    case(SSSS_EXTEND):
        dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 1; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
    case(GREV):
        if(dz->mode == GREV_PUT) {
            dz->array_cnt = 4; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;
        } else {
            dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0;
        }
        break;
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
    case(GRAIN_ASSESS):
    case(GRAIN_COUNT):     setup_process_logic(SNDFILES_ONLY,       SCREEN_MESSAGE,     NO_OUTPUTFILE,  dz);    break;
    case(GRAIN_OMIT):      setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GRAIN_DUPLICATE): setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GRAIN_REORDER):   setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GRAIN_REPITCH):   setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GRAIN_RERHYTHM):  setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GRAIN_REMOTIF):   setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GRAIN_TIMEWARP):  setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GRAIN_REVERSE):   setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GRAIN_POSITION):  setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GRAIN_ALIGN):     setup_process_logic(TWO_SNDFILES,        UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GRAIN_GET):       setup_process_logic(SNDFILES_ONLY,       TO_TEXTFILE,        TEXTFILE_OUT,   dz);    break;
    case(RRRR_EXTEND):      setup_process_logic(SNDFILES_ONLY,      UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(SSSS_EXTEND):      setup_process_logic(SNDFILES_ONLY,      UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(GREV):
        if(dz->mode==GREV_GET)
            setup_process_logic(SNDFILES_ONLY,      TO_TEXTFILE,        TEXTFILE_OUT,   dz);    
        else    
            setup_process_logic(SNDFILES_ONLY,      UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    
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
    case(GRAIN_ALIGN):
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
    mode = 0;
    switch(process) {
    case(GRAIN_COUNT):      case(GRAIN_OMIT):       case(GRAIN_REPITCH):    
    case(GRAIN_TIMEWARP):   case(GRAIN_GET):        case(GRAIN_DUPLICATE):  
    case(GRAIN_RERHYTHM):   case(GRAIN_ALIGN):      case(GRAIN_POSITION):   
    case(GRAIN_REORDER):    case(GRAIN_REMOTIF):    case(GRAIN_REVERSE):
                exit_status = set_internalparam_data("diiiiiiiiidddiiiii",ap);      break;
    case(GRAIN_ASSESS): 
        exit_status = set_internalparam_data("00ddd0diiiiiiiiidddiiiii",ap);        break;
    case(RRRR_EXTEND):
        exit_status = set_internalparam_data("di",ap);
        break;
    case(SSSS_EXTEND):
        break;
    case(GREV):
        exit_status = set_internalparam_data("i",ap);
        break;
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
    /*int exit_status = FINISHED;*/
    aplptr ap = dz->application;

    switch(ap->special_data) {
    case(GRAIN_REORDER_STRING): return get_reorder_sequence(str,dz);
    case(GRAIN_PITCH_RATIOS):
    case(GRAIN_TIME_RATIOS):    return get_grain_ratios((int)ap->special_data,str,dz);
    case(GRAIN_TWO_RATIOS):     return get_two_grain_ratios(str,dz);
    case(GRAIN_TIMINGS):        return get_grain_synctimes(str,dz);
        break;
    default:
        sprintf(errstr,"Unknown special_data type: read_special_data()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);   /* NOTREACHED */
}

/********************************* GET_REORDER_SEQUENCE ***************************/

int get_reorder_sequence(char *str,dataptr dz)
{

    char minval = 'z' + 1;
    char maxval = -1;
    char nextval, separator = ':';
    int OK  = FALSE;
    int  n, setcnt = strlen(str);
    for(n=0;n<setcnt;n++) {
        if(str[n]==':' || str[n]=='-') {
            separator = str[n];
            OK = TRUE;
            break;
        }
    }
    if(!OK) {
        sprintf(errstr,"Reorder sequence does not contain a separator\n");
        return(DATA_ERROR);
    }
    if(str[setcnt-2] != separator) {        /* Check for separator */
        sprintf(errstr,"Penultimate character in Reorder sequence should be the separator.\n");
        return(DATA_ERROR);
    }
    str[setcnt-2] = str[setcnt-1];      /* Eliminate separator */
    setcnt--;
    for(n=0;n<setcnt;n++) {             /* check all vals are ascii */
        if(!isalpha(str[n])) {
            sprintf(errstr,"Reorder sequence contains non-alphabetic characters\n");
            return(DATA_ERROR);
        }
    }
    nextval = str[setcnt-1];            /* Store final value as value-to-step-to */ 
    if(islower(nextval)) {              /* Standardising on uppercase */
        nextval += CAPITALISE;      
        nextval -= ALPHABASE;           /* and converting to numbers starting at zero */
    }
    dz->iparam[GR_REOCNT] = (int)(setcnt-1);/* Permute-set is all vals except last [which is value to step to] */
    for(n=0;n<dz->iparam[GR_REOCNT];n++) {
        if(islower(str[n]))             /* In the Permute set */
            str[n] += CAPITALISE;       /* Standardise on uppercase */
        str[n] -= ALPHABASE;            /* Then convert to numbers starting at zero */
        if(str[n] < minval)
            minval = str[n];            /* Also finding max and min values */
        if(str[n] > maxval)
            maxval = str[n];
    }                                   /* Check that nextval is an advance through sound */
    if((dz->iparam[GR_REOSTEP] = (int)(nextval - str[0])) <= 0) {
        sprintf(errstr,"Reorder sequence does not advance (last entry [%c] <= first [%c])\n",
        nextval+ALPHABASE,str[0]+ALPHABASE);
        return(DATA_ERROR);
    }                                    /* Reolen is max no. of grains to store before permute possible */
    dz->iparam[GR_REOLEN] = (int)(maxval - minval) + 1;
    if((dz->iparray[GR_REOSET] = (int *)malloc(dz->iparam[GR_REOCNT] * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store reorder set.\n");
        return(MEMORY_ERROR);
    }                                    /* Store the permutation set */
    for(n=0;n<dz->iparam[GR_REOCNT];n++) /* Offsetting it so that lowest numeric value = 0 */
        dz->iparray[GR_REOSET][n] = (int)(str[n] - minval);  

    if((dz->extrabuf = 
    (float **)realloc((char *)dz->extrabuf,(dz->iparam[GR_REOLEN]+SPLBUF_OFFSET) * sizeof(float *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate reorder set.\n");
        return(MEMORY_ERROR);            /* Create storage location for each stored grain */
    }                                    /* [in addition to splice stores] */
    if((dz->lparray[GR_ARRAYLEN] = (int *)malloc((dz->iparam[GR_REOLEN]) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create grain stores.\n");
        return(MEMORY_ERROR);           /* Create storage location for each stored grain arraylength */
    }
    if((dz->lparray[GR_THIS_LEN] = (int *)malloc((dz->iparam[GR_REOLEN]) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store grain lengths.\n");
        return(MEMORY_ERROR);           /* Create storage location for each stored grain length */
    }
    for(n=0;n<dz->iparam[GR_REOLEN];n++) {
        dz->lparray[GR_ARRAYLEN][n] = NOMINAL_LENGTH;
        if((dz->extrabuf[n+SPLBUF_OFFSET] = (float *)malloc(NOMINAL_LENGTH * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create extra grain buffers.\n");
            return(MEMORY_ERROR);       /* Give each grainstore a nominal length, which will be */
        }                               /* realloced, as required, during permutation process */
    }

    return(FINISHED);
}

/********************************* GET_GRAIN_RATIOS ***************************/

int get_grain_ratios(int special,char *filename,dataptr dz)
{
    FILE *fp;
    char temp[200], *p;
    double fratio;
    int ratiocnt = 0;
    /*double max_transpos = GR_MAX_TRANSPOS * SEMITONES_PER_OCTAVE;*/
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        if(*p == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&fratio)) {
            if(fratio < dz->application->min_special || fratio > dz->application->max_special) {
                sprintf(errstr,"Ratio (%lf) out of range (%lf - %lf)\n",
                fratio,dz->application->min_special,dz->application->max_special);
                return(DATA_ERROR);
            }
            if(special==GRAIN_PITCH_RATIOS)
                fratio = pow(2.0,fratio/SEMITONES_PER_OCTAVE);
            if(!ratiocnt)
                dz->parray[GR_RATIO] = (double *)malloc((ratiocnt+1)*sizeof(double));
            else
                dz->parray[GR_RATIO] =(double *)realloc((char *)dz->parray[GR_RATIO],(ratiocnt+1)*sizeof(double));
            dz->parray[GR_RATIO][ratiocnt] = fratio;
            ratiocnt++;
        }
    }
    if(ratiocnt==0) {
        sprintf(errstr,"No data in file '%s'.\n",filename);
        return(DATA_ERROR);
    }
    dz->iparam[GR_RATIOCNT] = ratiocnt;
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
        fflush(stdout);
    }
    return(FINISHED);
}

/********************************* GET_TWO_GRAIN_RATIOS ***************************/

int get_two_grain_ratios(char *filename,dataptr dz)
{
    FILE *fp;
    char temp[200], *p;
    double fratio;
    int ratiocnt = 0;
    int is_pitch = 1;
    /*double max_transpos = GR_MAX_TRANSPOS * SEMITONES_PER_OCTAVE;*/
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        if(*p == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&fratio)) {
            if(is_pitch) {
                if(fratio < dz->application->min_special || fratio > dz->application->max_special) {
                    sprintf(errstr,"Pitch-ratio (%lf) out of range (%lf - %lf)\n",
                    fratio,dz->application->min_special,dz->application->max_special);
                    return(DATA_ERROR);
                }
                fratio = pow(2.0,fratio/SEMITONES_PER_OCTAVE);
            } else {
                if(fratio < dz->application->min_special2 || fratio > dz->application->max_special2) {
                    sprintf(errstr,"Time-ratio (%lf) out of range (%lf - %lf)\n", fratio,
                    dz->application->min_special2,dz->application->max_special2);
                    return(DATA_ERROR);
                }
            }
            is_pitch = !is_pitch;
            if(!ratiocnt)
                dz->parray[GR_RATIO] = (double *)malloc((ratiocnt+1)*sizeof(double));
            else
                dz->parray[GR_RATIO] =(double *)realloc((char *)dz->parray[GR_RATIO],(ratiocnt+1)*sizeof(double));
            dz->parray[GR_RATIO][ratiocnt] = fratio;
            ratiocnt++;
        }
    }
    if(ratiocnt==0) {
        sprintf(errstr,"No data in file '%s'.\n",filename);
        return(DATA_ERROR);
    }
    if(ODD(ratiocnt)) {
        sprintf(errstr,"Pitch and time ratios not paired correctly.\n");
        return(DATA_ERROR);
    }
    dz->iparam[GR_RATIOCNT] = ratiocnt;
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
        fflush(stdout);
    }
    return(FINISHED);
}

/********************************* GET_GRAIN_SYNCTIMES ***************************/

int get_grain_synctimes(char *filename,dataptr dz)
{
    FILE *fp;
    char temp[200], *p;
    double synctime;
    int synccnt = 0;
    double lasttime = -1.0;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        if(*p == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&synctime)) {
            if(synctime < 0.0) {
                sprintf(errstr,"Invalid sync time %lf (<0.0)\n",synctime);
                return(DATA_ERROR);
            }
            if(synctime < lasttime) {
                sprintf(errstr,"Sync times out of sequence (%lf %lf)\n",lasttime,synctime);
                return(DATA_ERROR);
            }
            lasttime = synctime;
            
            synccnt++;
        }
    }
    if(synccnt==0) {
        sprintf(errstr,"No data in file '%s'.\n",filename);
        return(DATA_ERROR);
    }
    if(fseek(fp,0,0)<0) {
        sprintf(errstr,"Seek to start failed on file '%s'.\n",filename);
        return(SYSTEM_ERROR);
    }
    if((dz->parray[GR_SYNCTIME] = (double *)malloc(synccnt*sizeof(double)))==NULL) {
        sprintf(errstr,"Insufficient memory to store grain sync times.\n");
        return(MEMORY_ERROR);
    }
    synccnt = 0;
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(get_float_from_within_string(&p,&synctime)) {
            dz->parray[GR_SYNCTIME][synccnt] = synctime;
            synccnt++;
        }
    }
    if(dz->process == GREV)
        dz->itemcnt = (int)synccnt;
    else
        dz->iparam[GR_SYNCCNT] = (int)synccnt;
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
        fflush(stdout);
    }
    return(FINISHED);
}



/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/


/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)    
{
    int n, m;
    switch(dz->process) {
    case(GRAIN_COUNT):      case(GRAIN_REPITCH):    case(GRAIN_OMIT):       
    case(GRAIN_TIMEWARP):   case(GRAIN_GET):        case(GRAIN_RERHYTHM):   
    case(GRAIN_DUPLICATE):  case(GRAIN_ALIGN):      case(GRAIN_POSITION):   
    case(GRAIN_REMOTIF):    case(GRAIN_REORDER):    case(GRAIN_REVERSE):
    case(GRAIN_ASSESS):
        return grain_preprocess(GR_GATE,dz);
    case(RRRR_EXTEND):
        if(dz->mode != 1) {
            if(dz->param[RRR_START] >= dz->param[RRR_END]) {
                sprintf(errstr,"ERROR: Start and End times for the material to be processed are incompatible.\n");
                return(DATA_ERROR);
            }
        }
        if(dz->mode < 2) {
            if(dz->brksize[RRR_SLOW]) {     //  Check data continuity, and find time of end of data, or last 1.0 (no change) value in file
                if(dz->brk[RRR_SLOW][1] != 1.0 && dz->vflag[0] == 0) {
                    fprintf(stderr,"WARNING: First Slowing param not 1.0: will cause discontinuity where iteration starts.\n");
                    fflush(stdout);
                }
                if(dz->brk[RRR_SLOW][(dz->brksize[RRR_SLOW] * 2) - 1] != 1.0 && dz->vflag[1] == 0) {
                    fprintf(stdout,"WARNING: Last Slowing params not 1.0: will cause discontinuity where iteration ends.\n");
                    fflush(stdout);
                }
                dz->maxtime = 0.0;
                for(n=0,m=1;n <dz->brksize[RRR_SLOW];n++,m+=2) {
                    if(dz->brk[RRR_SLOW][m] < 1.0) {
                        sprintf(errstr,"SLOWING PARAMETER CANNOT BE LESS THAN 1.0\n");
                        return DATA_ERROR;
                    }
                    if(dz->brk[RRR_SLOW][m] == 1.0)
                        dz->maxtime = dz->brk[RRR_SLOW][m-1];
                }
                if(dz->maxtime == 0.0)
                    dz->maxtime = dz->brk[RRR_SLOW][(dz->brksize[RRR_SLOW] * 2) - 2];
            } else  {
                if(dz->param[RRR_SLOW] != 1.0) {
                    fprintf(stdout,"WARNING: SLOWING PARAMETER MUST BE IN A BRKFILE. DEFAULTING TO NO SLOWING.\n");
                    fflush(stdout);
                }
                dz->param[RRR_SLOW] = 1.0;
            }
            if(dz->brksize[RRR_REGU]) {
                if(!dz->brksize[RRR_SLOW]) {
                    sprintf(errstr,"REGULARITY PARAMETER CANNOT BE USED WITHOUT THE SLOWING PARAMETER.\n");
                    return DATA_ERROR;
                }
            } else if(dz->param[RRR_REGU] != 0.0) {
                sprintf(errstr,"REGULARITY PARAMETER, IF NOT IN A BRKFILE, MUST BE SET TO 0.0\n");
                return DATA_ERROR;
            }
        }
        dz->param[RRR_RANGE] = pow(2.0,dz->param[RRR_RANGE]);   /* convert transposition range from octaves to ratio */
        if(dz->param[RRR_RANGE] > 1.0)
            dz->param[RRR_RANGE] = 1.0/dz->param[RRR_RANGE];
        break;
    case(SSSS_EXTEND):
        dz->iparam[SSS_DUR]      = (int)round(dz->param[SSS_DUR] * dz->infile->srate) * dz->infile->channels;
        dz->iparam[NOISE_MINFRQ] = (int)round((double)dz->infile->srate / (double)dz->param[NOISE_MINFRQ]);
        dz->iparam[MIN_NOISLEN]  = (int)round(dz->param[MIN_NOISLEN] * MS_TO_SECS * dz->infile->srate) * dz->infile->channels;
        dz->iparam[MAX_NOISLEN]  = (int)round(dz->param[MIN_NOISLEN] * dz->infile->srate) * dz->infile->channels;
        dz->tempsize = dz->iparam[SSS_DUR];
        if(!dz->vflag[0])
            dz->tempsize += dz->insams[0];
        break;
    case(GREV):
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

    if(dz->process!=GRAIN_COUNT && dz->process!=GRAIN_ASSESS)
        display_virtual_time(0L,dz);

    switch(dz->process) {
    case(GRAIN_COUNT):   case(GRAIN_OMIT):      case(GRAIN_DUPLICATE):
    case(GRAIN_REORDER): case(GRAIN_REPITCH):   case(GRAIN_RERHYTHM):
    case(GRAIN_REMOTIF): case(GRAIN_TIMEWARP):  case(GRAIN_GET):
    case(GRAIN_POSITION):case(GRAIN_REVERSE):
        if((exit_status = process_grains(dz))<0)
            return(exit_status);
        break;
    case(GRAIN_ALIGN):
        if((exit_status = process_grains(dz))<0) /* Process as GRAIN_GET,but with no text output */
            return(exit_status);
        if((exit_status = swap_to_otherfile_and_readjust_counters(dz))<0)
            return(exit_status);
        if((exit_status = zero_sound_buffers(dz))<0)
            return(exit_status);
        dz->process = GRAIN_POSITION;
        if((exit_status = grain_preprocess(GR_GATE2,dz))<0)
            return(exit_status);
        if((exit_status = process_grains(dz))<0)
            return(exit_status);
        break;
    case(RRRR_EXTEND):
        switch(dz->mode) {
        case(0):
            exit_status = timestretch_iterative(dz);
            break;
        case(1):
            exit_status = timestretch_iterative2(dz);
            break;
        case(2):
            exit_status = timestretch_iterative3(dz);
            break;
        }
        if(exit_status <0)
            return(exit_status);
        break;
    case(SSSS_EXTEND):
        if((exit_status = grab_noise_and_expand(dz))<0)
            return(exit_status);
        break;
    case(GRAIN_ASSESS):
        return assess_grains(dz);
    case(GREV):
        return grev(dz);
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
    int exit_status;
    int n;
    double brkmax, brkmin, ratio;

    handle_pitch_zeros(dz);
    switch(dz->process) {
    case(GRAIN_OMIT):   return check_grain_consistency(dz);
    case(GRAIN_ASSESS):
        dz->param[GR_GATE]    =  GR_GATE_DEFAULT;
        dz->param[GR_MINTIME] = (GRAIN_SPLICELEN + GRAIN_SAFETY) * MS_TO_SECS * 2.0;
        dz->param[GR_WINSIZE] = 50.0;
        if((dz->vflag = (char *)malloc(sizeof(char))) == NULL) {
            sprintf(errstr,"Insufficient memory for internal flags.\n");
            return(MEMORY_ERROR);
        }
        dz->vflag[0] = 0;
        if((dz->brksize = (int *)malloc(4 * sizeof(int))) == NULL) {
            sprintf(errstr,"Insufficient memory for internal flags.\n");
            return(MEMORY_ERROR);
        }
        for(n=0;n<4;n++)
            dz->brksize[n] = 0;
        break;
    case(RRRR_EXTEND):
        if(dz->mode == 2) {
            dz->tempsize = dz->insams[0];
            break;
        }
        if(dz->mode == 1) {
            dz->param[RRR_WSIZENU] = LOW_RRR_SIZE/3.0;  /* 15 ms/2 = 5ms */
            generate_samp_windowsizer(dz);
            ratio = dz->param[RRR_GRSIZ] * MS_TO_SECS * dz->param[RRR_GET] * (dz->param[RRR_STRETCH] - 1.0);
        } else {
            ratio = fabs(dz->param[RRR_END] - dz->param[RRR_START]) * (dz->param[RRR_STRETCH] - 1.0);
        }
        dz->tempsize = (int)round(dz->infile->srate * ratio);
        dz->tempsize *= dz->infile->channels;
        dz->tempsize += dz->insams[0];
        break;
    case(GREV):
        dz->iparam[GREV_SAMP_WSIZE] =  (int)round(dz->param[GREV_WSIZE] * MS_TO_SECS * (double)dz->infile->srate);
        dz->param[GREV_TROFRAC] /= 2.0; /* include averaging factor here */
        switch(dz->mode) {
        case(GREV_DELETE):
        case(GREV_OMIT): /* convert how many to delete, to how many to keep NB GREV_DEL = GREV_KEEP */ 
            if(dz->brksize[GREV_DEL]) {
                if((exit_status = get_maxvalue_in_brktable(&brkmax,GREV_DEL,dz)) < 0)
                    return(exit_status);
            } else
                brkmax = (double)dz->iparam[GREV_DEL];
            if(dz->iparam[GREV_OUTOF] <=  (int)round(brkmax)) {
                sprintf(errstr,"CANNOT DELETE %d OUT OF %d.\n",(int)round(brkmax),dz->iparam[GREV_OUTOF]);
                return(DATA_ERROR);
            }
            if(dz->brksize[GREV_DEL]) {
                for(n = 1; n < dz->brksize[GREV_KEEP];n+=2)
                    dz->brk[GREV_KEEP][n] = (double)dz->iparam[GREV_OUTOF] - dz->brk[GREV_DEL][n];
            } else
                dz->iparam[GREV_KEEP] = dz->iparam[GREV_OUTOF] - dz->iparam[GREV_DEL];
            break;
        }
        switch(dz->mode) {
        case(GREV_REVERSE):
        case(GREV_OMIT):
        case(GREV_PUT):
            dz->tempsize = dz->insams[0];
            break;
        case(GREV_REPEAT):
            if(dz->brksize[GREV_REPETS]) {
                if((exit_status = get_maxvalue_in_brktable(&brkmax,GREV_REPETS,dz)) < 0)
                    return(exit_status);
            } else
                brkmax = (double)dz->iparam[GREV_REPETS];
            dz->tempsize = (int)round(dz->insams[0] * brkmax);
            break;
        case(GREV_DELETE):
            if(dz->brksize[GREV_DEL]) {
                if((exit_status = get_minvalue_in_brktable(&brkmin,GREV_DEL,dz)) < 0)
                    return(exit_status);
            } else
                brkmin = (double)dz->iparam[GREV_DEL];
            ratio = (double)dz->iparam[GREV_OUTOF] - brkmin;
            ratio /=  (double)dz->iparam[GREV_OUTOF];
            dz->tempsize = (int)round(dz->insams[0] * ratio);
            break;
        case(GREV_TSTRETCH):
            if(dz->brksize[GREV_TSTR]) {        /* actually, the maximum number to DELETE */
                if((exit_status = get_maxvalue_in_brktable(&brkmax,GREV_TSTR,dz)) < 0)
                    return(exit_status);
            } else
                brkmax = (double)dz->param[GREV_TSTR];
            dz->tempsize = (int)round(dz->insams[0] * brkmax);
            break;
        }   
        break;
    }
    return(FINISHED);
}

/***************************** CHECK_GRAIN_CONSISTENCY ******************************/

int check_grain_consistency(dataptr dz)
{
    int exit_status;
    int maxval;
    double dmaxval;
    switch(dz->process) {
    case(GRAIN_OMIT):       /* Check grains-to-KEEP vals against OUT-OF */
        if(dz->brksize[GR_KEEP]) {
            if((exit_status = get_maxvalue_in_brktable(&dmaxval,GR_KEEP,dz))<0)
                return(exit_status);
            maxval = round(dmaxval);
        } else
            maxval = dz->iparam[GR_KEEP];
        if(maxval > dz->iparam[GR_OUT_OF]) {
            sprintf(errstr,"A value of %d grains-to-keep out of each %d is impossible.\n",maxval,(int)dz->iparam[GR_OUT_OF]);
            return(DATA_ERROR);
        }
        break;
    default:
        sprintf(errstr,"Unknown case in check_grain_consistency()\n");
        return(PROGRAM_ERROR);
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
    case(GRAIN_COUNT):      case(GRAIN_REPITCH):    case(GRAIN_OMIT):
    case(GRAIN_TIMEWARP):   case(GRAIN_GET):        case(GRAIN_RERHYTHM):
    case(GRAIN_DUPLICATE):  case(GRAIN_ALIGN):      case(GRAIN_POSITION):
    case(GRAIN_REMOTIF):    case(GRAIN_REORDER):    case(GRAIN_REVERSE):
    case(GRAIN_ASSESS):
        return create_grain_sndbufs(dz);
    case(RRRR_EXTEND):
        return create_rrr_sndbufs(dz);
    case(SSSS_EXTEND):
        return create_ssss_sndbufs(dz);
    case(GREV):
        return create_sndbufs(dz);
    default:
        sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);   /* NOTREACHED */
}

/*************************** CREATE_GRAIN_SNDBUFS **************************/

int create_grain_sndbufs(dataptr dz)
{
    int bigbufsize, zz;
    int n;
//TW
//  int modulus = FSECSIZE;
    int modulus = FSECSIZE * dz->infile->channels;
    if(dz->sbufptr == 0 || dz->sampbuf==0) {
        sprintf(errstr,"buffer pointers not allocated: create_grain_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = (int) (size_t ) Malloc(-1);

    dz->buflen = bigbufsize / sizeof(float);

    dz->buflen /= dz->bufcnt;
    if((zz = (int)round(dz->param[GR_BLEN] * dz->infile->srate)) > dz->buflen)
        dz->buflen = zz;
    dz->iparam[GR_WSIZE_SAMPS] = 0;
    if(dz->vflag[GR_ENVTRACK] && !flteq(dz->param[GR_WINSIZE],0.0)) {
        /* may be able to eliminate this ...?? */
        establish_grain_envelope_windowsize_in_samps(dz);   
        modulus = dz->iparam[GR_WSIZE_SAMPS];
    }
//TW most importantly, modulo-channels calc now moved AFTER "dz->buflen /= dz->bufcnt"
// so divided-buffers still multiples of chans
    if((dz->buflen  = (dz->buflen/modulus) * modulus)<=0) {
        dz->buflen  = modulus;
    }

    if((dz->bigbuf = (float *)malloc((size_t)(dz->buflen * dz->bufcnt)* sizeof(float))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->bufcnt;n++)
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    return(FINISHED);
}

/*************************** CREATE_SSSS_SNDBUFS **************************/

int create_ssss_sndbufs(dataptr dz)
{
    int bigsize, bigbufsize;
    if(dz->sbufptr == 0 || dz->sampbuf==0) {
        sprintf(errstr,"buffer pointers not allocated: create_grain_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = (int)(size_t) Malloc(-1);
    dz->buflen = bigbufsize/sizeof(float);
    bigbufsize = dz->buflen * sizeof(float);
    bigsize = dz->insams[0] * sizeof(float);
    if(bigsize < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(MEMORY_ERROR);
    }
    if(bigbufsize + bigsize < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->bigbuf = (float *)malloc((size_t)(bigbufsize + bigsize))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
    dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->insams[0];
    dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
    return(FINISHED);
}

/*************************** ESTABLISH_GRAIN_ENVELOPE_WINDOWSIZE_IN_SAMPS **************************/

void establish_grain_envelope_windowsize_in_samps(dataptr dz)
{
    int seccnt;

    dz->iparam[GR_WSIZE_SAMPS] = 
    (int)round(dz->param[GR_WINSIZE] * (double)(dz->infile->srate * dz->infile->channels));

//TW retain old buffering protocol, for now
    if((seccnt = dz->iparam[GR_WSIZE_SAMPS]/F_SECSIZE)*F_SECSIZE != dz->iparam[GR_WSIZE_SAMPS]) {
        seccnt++;
        dz->iparam[GR_WSIZE_SAMPS] = (int)(seccnt * F_SECSIZE);
    }
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if     (!strcmp(prog_identifier_from_cmdline,"count"))          dz->process = GRAIN_COUNT;
    else if(!strcmp(prog_identifier_from_cmdline,"omit"))           dz->process = GRAIN_OMIT;
    else if(!strcmp(prog_identifier_from_cmdline,"duplicate"))      dz->process = GRAIN_DUPLICATE;
    else if(!strcmp(prog_identifier_from_cmdline,"reorder"))        dz->process = GRAIN_REORDER;
    else if(!strcmp(prog_identifier_from_cmdline,"repitch"))        dz->process = GRAIN_REPITCH;
    else if(!strcmp(prog_identifier_from_cmdline,"rerhythm"))       dz->process = GRAIN_RERHYTHM;
    else if(!strcmp(prog_identifier_from_cmdline,"remotif"))        dz->process = GRAIN_REMOTIF;
    else if(!strcmp(prog_identifier_from_cmdline,"timewarp"))       dz->process = GRAIN_TIMEWARP;
    else if(!strcmp(prog_identifier_from_cmdline,"find"))           dz->process = GRAIN_GET;
    else if(!strcmp(prog_identifier_from_cmdline,"reposition"))     dz->process = GRAIN_POSITION;
    else if(!strcmp(prog_identifier_from_cmdline,"align"))          dz->process = GRAIN_ALIGN;
    else if(!strcmp(prog_identifier_from_cmdline,"reverse"))        dz->process = GRAIN_REVERSE;
    else if(!strcmp(prog_identifier_from_cmdline,"assess"))         dz->process = GRAIN_ASSESS;
    else if(!strcmp(prog_identifier_from_cmdline,"r_extend"))       dz->process = RRRR_EXTEND;
    else if(!strcmp(prog_identifier_from_cmdline,"noise_extend"))   dz->process = SSSS_EXTEND;
    else if(!strcmp(prog_identifier_from_cmdline,"grev"))           dz->process = GREV;
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
    "\nUSAGE: grain NAME (mode) infile (infile2) outfile parameters:\n"
    "\n"
    "where NAME can be any one of\n"
    "\n"
    "    GRAIN OPERATIONS ON A SINGLE SOUND FILE\n\n"
    "count       omit        repitch     timewarp\n"
    "find        duplicate   rerhythm    reverse\n"
    "reposition  reorder     remotif     assess\n" 
    "r_extend         grev        noise_extend\n"
    "\n"
    "    GRAIN OPERATIONS ON TWO SOUND FILES\n\n"
    "align\n"
    "\n"
    "Type 'grain count' for more info on count option.. ETC.\n");
    return(USAGE_ONLY);
}


/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    double min_graintime = (GRAIN_SPLICELEN + GRAIN_SAFETY) * MS_TO_SECS * 2.0;
    if(!strcmp(str,"count")) {
        fprintf(stdout,
        "COUNT GRAINS FOUND IN A SOUND (AT GIVEN GATE & MINHOLE VALUES)\n\n"
        "USAGE:\n"
        "grain count infile [-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n"
        "LEN      maximum time between grains\n"
        "GATE     required signal level for grain to be seen: Range 0-1: default 1\n"
        "MINHOLE  min duration of inter-grain holes:\n"
        "         Min value (default) %.3lf\n"
        "-t       Gate level tracks signal level, found with windowsize winsize(msecs)\n"
        "         Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x       ignore the last grain in the source.\n\n"
        "Gate may vary over time.\n",
        min_graintime);
    } else if(!strcmp(str,"omit")) {
        fprintf(stdout,
        "OMIT A PROPORTION OF GRAINS FROM GRAINY-SOUND\n\n"
        "USAGE:\n"
        "grain omit inf outf keep out-of [-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n"
        "KEEP     number of grains to keep from each set of 'out_of' grains\n"
        "OUT_OF   'keep' grains retained from start of each set of 'out_of' grains\n"
        "LEN      maximum time between grains\n"
        "GATE     required signal level for grain to be seen: Range 0-1: default 1\n"
        "MINHOLE  min duration of inter-grain holes:\n"
        "         Min value (default) %.3lf\n"
        "-t       Gate level tracks signal level, found with windowsize winsize(msecs)\n"
        "         Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x       ignore the last grain in the source.\n\n"
        "Gate may vary over time.\n"
        "Keep may vary over time, but must not exceed 'out_of'.\n",
        min_graintime);
    } else if(!strcmp(str,"duplicate")) {
        fprintf(stdout,
        "DUPLICATE GRAINS IN GRAINY-SOUND\n\n"
        "USAGE:\n"
        "grain duplicate infil outfil N [-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n"
        "N        number of repetitions of each grain\n"
        "LEN      maximum time between grains\n"
        "GATE     required signal level for grain to be seen: Range 0-1: default 1\n"
        "MINHOLE  min duration of inter-grain holes:\n"
        "         Min value (default) %.3lf\n"
        "-t       Gate level tracks signal level, found with windowsize winsize(msecs)\n"
        "         Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x       ignore the last grain in the source.\n\n"
        "N and Gate may vary over time.\n",
        min_graintime);
    } else if(!strcmp(str,"reorder")) {
        fprintf(stdout,
        "REORDER GRAINS IN GRAINY-SOUND\n\n"
        "USAGE:\n"
        "grain reorder infil outfil code [-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n"
        "CODE     a string such as 'adb:c' indicating how grains are to be reordered.\n"
        "         The example means use grains 1 (a), 4 (d) and 2 (b) in sequence,\n"
        "         then begin this grain-jumping pattern again, BUT start at grain3 (c).\n"
        "         Continue to advance in this fashion until no grains left in infile.\n"
        "         The ':' is obligatory.\n" 
        "LEN      maximum time between grains\n"
        "GATE     required signal level for grain to be seen: Range 0-1: default 1\n"
        "MINHOLE  min duration of inter-grain holes:\n"
        "         Min value (default) %.3lf\n"
        "-t       Gate level tracks signal level, found with windowsize winsize(msecs)\n"
        "         Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x       ignore the last grain in the source.\n\n"
        "Gate may vary over time.\n",
        min_graintime);
    } else if(!strcmp(str,"repitch")) {
        fprintf(stdout,
        "REPITCH GRAINS IN GRAINY-SOUND\n\n"
        "USAGE:\n"
        "grain repitch mode infile outfile transpfile\n\t\t[-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n"
        "MODES are:-\n"
        "(1)   Repitch each grain in turn, without repeating any grains:\n"
        "      on reaching end of transposition list, cycle back to its start.\n"
        "(2)   Play grain at each transposed pitch, before proceeding to next grain.\n\n"
        "TRANSPFILE is a file listing transpositions as (+ve or -ve) semitone shifts.\n"
        "           Max transposition is %.0lf octaves up or down\n\n"
        "LEN        maximum time between grains\n"
        "GATE       required signal level for grain to be seen: Range 0-1:default 1\n"
        "MINHOLE    min duration of inter-grain holes:\n"
        "           Min value (default) %.3lf\n"
        "-t         Gate level tracks signal level,found with windowsize winsize(msecs)\n"
        "           Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x         ignore the last grain in the source.\n\n"
        "Gate may vary over time.\n",
        GR_MAX_TRANSPOS,min_graintime);
    } else if(!strcmp(str,"rerhythm")) {
        fprintf(stdout,
        "CHANGE RHYTHM OF GRAINS IN GRAINY-SOUND\n\n"
        "USAGE:\n"
        "grain rerhythm mode infile outfile multfile\n\t\t[-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n"
        "MODES are:-\n"
        "(1) Lengthen or shorten each grain in turn, without repeating any grains:\n"
        "    on reaching end of time-multipliers list, cycle back to its start.\n"
        "(2) Play grain at each specified retiming, before proceeding to next grain.\n\n"
        "MULTFILE is a file listing duration-multipliers to change the duration\n"
        "         between one grain onset and the next grain onset.\n"
        "         Max multiplier is %.0lf : Min multiplier is %lf\n"
        "         if any inter-grain time reduced below MINGRAINTIME (%.3lf)\n"
        "         it will be set to MINGRAINTIME.\n\n" 
        "LEN      maximum time between grains\n"
        "GATE     required signal level for grain to be seen: Range 0-1: default 1\n"
        "MINHOLE  min duration of inter-grain holes:\n"
        "         Min value (default) %.3lf\n"
        "-t       Gate level tracks signal level,found with windowsize winsize(msecs)\n"
        "         Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x       ignore the last grain in the source.\n\n"
        "Gate may vary over time.\n",
        GR_MAX_TSTRETCH,GR_MIN_TSTRETCH,min_graintime,min_graintime);
    } else if(!strcmp(str,"remotif")) {
        fprintf(stdout,
        "CHANGE PITCH AND RHYTHM OF GRAINS IN GRAINY-SOUND\n\n"
        "USAGE:\n"
        "grain remotif mode infile outfile transpmultfile\n\t\t[-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n"
        "MODES are:-\n"
        "(1) Transform each grain in turn, without repeating any grains:\n"
        "    on reaching end of transpmultfile-data, cycle back to its start.\n"
        "(2) Transform grain in each specified way, before proceeding to next grain.\n\n"
        "TRANSPMULTFILE is a file containing transposition\\time-multiplier PAIRS.\n"
        "               Transpositions are (+ve or -ve) semitone shifts.\n"
        "               Max transposition is %.0lf octaves up or down.\n"
        "               Time-multipliers change the duration\n"
        "               between one grain onset and the next grain onset.\n"
        "               Max multiplier is %.0lf : Min multiplier is %lf\n"
        "               if any inter-grain time reduced below MINGRAINTIME (%.3lf)\n"
        "               it will be set to MINGRAINTIME.\n\n" 
        "LEN      maximum time between grains\n"
        "GATE     required signal level for grain to be seen: Range 0-1: default 1\n"
        "MINHOLE  min duration of inter-grain holes:\n"
        "         Min value (default) %.3lf\n"
        "-t       Gate level tracks signal level,found with windowsize winsize(msecs)\n"
        "         Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x       ignore the last grain in the source.\n\n"
        "Gate may vary over time.\n",
        GR_MAX_TRANSPOS,GR_MAX_TSTRETCH,GR_MIN_TSTRETCH,min_graintime,min_graintime);
    } else if(!strcmp(str,"timewarp")) {
        fprintf(stdout,
        "STRETCH (OR SHRINK) DURATION OF GRAINY-SOUND\n"
        "    WITHOUT STRETCHING GRAINS THEMSELVES\n\n"
        "USAGE:\n"
        "grain timewarp infile outfile timestretch-ratio\n\t\t[-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n" 
        "TIMESTRETCH-RATIO is degree of stretching/shrinking of intergrain time.\n"
        "                  a value of 2 doubles the intergrain time.\n"
        "                  a value of .5 halves the intergrain time.\n"
        "                  Max stretch is %.0lf : Min shrink is %lf\n"
        "                  if any inter-grain time reduced below MINGRAINTIME (%.3lf)\n"
        "                  it will be set to MINGRAINTIME.\n\n" 
        "LEN      maximum time between grains\n"
        "GATE     required signal level for grain to be seen: Range 0-1: default 1\n"
        "MINHOLE  min duration of inter-grain holes:\n"
        "         Min value (default) %.3lf\n"
        "-t       Gate level tracks signal level,found with windowsize winsize(msecs)\n"
        "         Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x       ignore the last grain in the source.\n\n"
        "Timestretch-ratio and Gate may vary over time.\n"
        "Times in brkpnt files refer to INFILE time.\n",
        GR_MAX_TSTRETCH,GR_MIN_TSTRETCH,min_graintime,min_graintime);
    } else if(!strcmp(str,"reverse")) {
        fprintf(stdout,
        "REVERSE ORDER OF GRAINS IN A GRAINY-SOUND\n"
        "   WITHOUT REVERSING GRAINS THEMSELVES\n\n"
        "USAGE:\n"
        "grain reverse infil outfil [-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n" 
        "LEN      maximum time between grains\n"
        "GATE     required signal level for grain to be seen: Range 0-1: default 1\n"
        "MINHOLE  min duration of inter-grain holes:\n"
        "         Min value (default) %.3lf\n"
        "-t       Gate level tracks signal level,found with windowsize winsize(msecs)\n"
        "         Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x       ignore the last grain in the source.\n\n"
        "Gate may vary over time.\n",min_graintime);
    } else if(!strcmp(str,"find")) {
        fprintf(stdout,
        "LOCATE TIMINGS OF GRAIN-ONSETS IN A GRAINY-SOUND\n\n"
        "USAGE:\n"
        "grain find infil out-textfil [-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n"
        "OUT-TEXTFILE will contain a list of grain-onset timings, in seconds.\n"
        "LEN          maximum time between grains\n"
        "GATE         min signal level for grain to be seen: Range 0-1: default 1\n"
        "MINHOLE      min duration of inter-grain holes:\n"
        "             Min value (default) %.3lf\n"
        "-t           Gate tracks signal level,found with windowsize winsize(msecs)\n"
        "             Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x           ignore the last grain in the source.\n\n"
        "Gate may vary over time.\n",
        min_graintime);
    } else if(!strcmp(str,"reposition")) {
        fprintf(stdout,
        "REPOSITION GRAIN-ONSETS IN A GRAINY-SOUND\n\n"
        "USAGE:\n"
        "grain reposition infile outfile timefile offset\n\t\t[-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n"
        "TIMEFILE  must contain a list of grain-onset timings, in seconds.\n"
        "          if any inter-grain time reduced below MINGRAINTIME (%.3lf)\n"
        "          it will be set to MINGRAINTIME.\n" 
        "OFFSET    add this value (secs) to ALL grain timings.\n"
        "LEN       maximum time between grains\n"
        "GATE      min signal level for grain to be seen: Range 0-1: default 1\n"
        "MINHOLE   min duration of inter-grain holes:\n"
        "          Min value (default) %.3lf\n"
        "-t        Gate level tracks signal level,found with windowsize winsize(msecs)\n"
        "          Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x        ignore the last grain in the source.\n\n"
        "Gate may vary over time.\n",
        min_graintime,min_graintime);
    } else if(!strcmp(str,"align")) {
        fprintf(stdout,
        "SYNCHRONISE GRAIN-ONSETS IN 2nd GRAINY-SOUND\n"
        "            WITH THOSE IN THE 1st.\n"
        "USAGE:\n"
        "grain align infile1 infile2 outfile offset gate2\n\t\t[-blen] [-lgate] [-hminhole] [-twinsize] [-x]\n\n"
        "INFILE1 provides grain-onset times.\n"
        "INFILE2 provides the actual grains to be retimed.\n"
        "OFFSET  add this value (secs) to ALL grain timings.\n"
        "GATE2   min signal level to register grain in File2: Range 0-1: default 1\n"
        "LEN     maximum time between grains\n"
        "GATE    min signal level to register grain in File1: Range 0-1: default 1\n"
        "MINHOLE min duration of inter-grain holes:\n"
        "        Min value (default) %.3lf\n"
        "-t      Gate level tracks signal level,found with windowsize winsize(msecs)\n"
        "        Range(0.0 - infiledur): 0.0 turns off tracking)\n"
        "-x      ignore the last grain in the source.\n\n"
        "Gate and Gate2 may vary over time.\n",
        min_graintime);
    } else if(!strcmp(str,"assess")) {
        fprintf(stdout,
        "ASSESS BEST GATE VALUE TO FIND MAXIMUM NUMBER OF GRAINS IN FILE.\n"
        "USAGE:  grain assess infile\n\n");
    } else if(!strcmp(str,"r_extend")) {
        fprintf(stdout,
        "TIME-STRETCH' NATURAL ITERATIVE SOUNDS LIKE THE ROLLED 'rrr' IN SPEECH.\n"
        "IN MODE 1 you mark where iterative part of sound is.\n"
        "USAGE: \n"
        "grain r_extend 1 inf outf stt end te pr rep get asc psc rit reg [-s] [-e]\n"
        "     STT      Time of start of iterated material within source.\n"
        "     END      Time of end of iterated material within source.\n"
        "\n"
        "IN MODE 2 process attempts to find it, using envelope tracing.\n"
        "USAGE: \n"
        "grain r_extend 2 inf outf gate usz te pr rep get asc psc skp rit reg [-s] [-e]\n"
        "     GATE     min level in src for env tracing to kick in (0-1).\n"
        "     USZ      size of unit searched for, in ms (15 for rolled rr).\n"
        "     SKP      number of found units to skip before processing.\n"
        "\n"
        "TE       How much to time-extend the marked (or found) material.\n"
        "PR       Guesstimate pitch-range of iteration in src - in octaves (try 1).\n"
        "REP      Iterated material extended by reusing individual segments\n"
        "         'repets' is no. of adjacent copies of any seg you allow (try 1-2)\n"
        "GET      Guesstimate no. of iterations expected: listen to snd (range 2-1000).\n"
        "         In MODE 2, MINIMUM no of repeats you expect to find.\n"
        "ASC      Ouput segs made to vary randomly in amplitude. (Range 0-1)\n"
        "         Value multiplies orig seg amp by rand val in range 1 to (1-N).\n"
        "PSC      Output segs made to vary randomly in pitch (Range 0-24)\n"
        "         Val transposes pitch by randval in range -N to +N semitones.\n"
        "RIT      Slowing factor (grain-turnover rate slowed by inserting silence).\n"
        "         1.0 = no slowing. Otherwise vals MUST BE IN A BRKPOINT FILE.\n"
        "         Brkpoint times refer to time RELATIVE TO START OF ITERATED SEG.\n"
        "         When used, number of interations will be extended (or curtailed)\n"
        "         to allow the rit(s) to play out.\n"
        "REG      Force grain rate to regular. (Only where grains slowed by \"rit\").\n"
        "         1 = completely regular. But only happens once intest grain\n"
        "         shorter than average separation caused by \"rit\".\n"
        "-s       Don't keep start of sound before the iterative-extension.\n"
        "-e       Don't keep end of sound after the iterative-extension.\n"
        "IN MODE 3 Sound is edited into its start, each iterate unit, and its end.\n"
        "using params  STT  END  and PR\n");
    } else if(!strcmp(str,"noise_extend")) {
        fprintf(stdout,
        "FIND AND TIME-STRETCH' NOISE COMPONENT IN SOUND.\n"
        "USAGE: \n"
        "grain noise_extend inf outf duration minfrq mindur maxdur [-x]\n"
        "DURATION   Duration of noise (part of) output file.\n"
        "MINFRQ     lowest 'frequency' (Hz) acceptable as noise (try %.1f)\n"
        "MINDUR     Minimum duration of signal (in ms) acceptable as noise source.\n"
        "MAXDUR     Maximum duration of signal (in secs) acceptable as noise source.\n"
        "-x         Keep only the extended noise: default keep rest of input src too.\n",NOIS_MIN_FRQ);
    } else if(!strcmp(str,"grev")) {
        fprintf(stdout,
        "FIND AND MANIPULATE 'GRAINS', USING ENVELOPE TROUGHS AND ZERO-CROSSINGS.\n"
        "This process locates elements of sound by searching for troughs in envelope.\n"
        "It doesn't need a clear attack to recognise 'grain' in sound, and is\n"
        "more appropriate for e.g. separating syllables in speech.\n\n"
        "(REVERSE)     USAGE: grain grev 1 inf outf wsiz trof gpcnt\n"
        "(REPEAT)      USAGE: grain grev 2 inf outf wsiz trof gpcnt repets\n"
        "(DELETE)      USAGE: grain grev 3 inf outf wsiz trof gpcnt keep outof\n"
        "(OMIT)        USAGE: grain grev 4 inf outf wsiz trof gpcnt keep outof\n"
        "(TIMESTRETCH) USAGE: grain grev 5 inf outf wsiz trof gpcnt tstretch\n"
        "(GET)         USAGE: grain grev 6 inf out_timesfile wsiz trof gpcnt\n"
        "(PUT)         USAGE: grain grev 7 inf outf in_timesfile wsiz trof gpcnt\n"
        "WSIZ    sizeof window in ms, determines size of grains to find.\n"
        "TROF    acceptable trough height, relative to adjacent peaks (range >0 - <1)\n"
        "GPCNT   number of grains to treat as a unit in the operations.\n"
        "REPETS  number of repetitions of each unit.\n"
        "KEEP | OUTOF  number of units to keep, e.g. 3 out of 5.\n"
        "TSTRETCH amount to timestretch output (grains NOT streched) (Range .01 - 100).\n"
        "GET gets grain-pos to (text) timesfile: PUT puts grains at times in timesfile\n"
        "DELETE removes specified units: OMIT replaces them with silence,\n"
        "GPCNT, REPETS, KEEP and TSTRETCH can vary over time,\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
    if(!strcmp(str1,"count") || !strcmp(str1,"assess"))     /*RWD added assess */
        return(CONTINUE);
    else
        sprintf(errstr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
    return(FINISHED);
}

/*************************** CREATE_RRR_SNDBUFS **************************/

int create_rrr_sndbufs(dataptr dz)
{
    int n;
    int bigbufsize, secfactor, seccnt;

    if(dz->sbufptr == 0 || dz->sampbuf==0) {
        sprintf(errstr,"buffer pointers not allocated: create_grain_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = (dz->insams[0] + 2) * 2; /* in and out bufs, plus wrap-around point */

    if(dz->mode == 1) {
        secfactor = max((int)(dz->iparam[RRR_SAMP_WSIZENU]),F_SECSIZE) * 2;
        if((seccnt = bigbufsize/secfactor) * secfactor != bigbufsize)
            seccnt++;
        bigbufsize = seccnt * secfactor;
    }
    if(bigbufsize < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->bigbuf = (float *) malloc((size_t)(bigbufsize * sizeof(float)))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(MEMORY_ERROR);
    }
    bigbufsize /= dz->bufcnt;
    dz->buflen = bigbufsize;
    for(n=0;n<dz->bufcnt;n++)
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
                /* These are larger than the theoretically possible maximum, i.e. a peak every 2 samples !! */
    if((dz->parray[0] = (double *)malloc(((dz->insams[0]/2) + 2) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create peak store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[0] = (int *)malloc(((dz->insams[0]/2) + 2) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create peak store.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/**************************** GENERATE_SAMP_WINDOWSIZER *************************
 *
 * unadjusted_envwindow_sampsize too small should NOW be trapped by range settings!!
 */

int generate_samp_windowsizer(dataptr dz)
{
    int unadjusted_envwindow_sampsize, j, k;
    int srate = dz->infile->srate;
    dz->iparam[RRR_SAMP_WSIZENU] = (int)(SECSIZE/sizeof(float));
    unadjusted_envwindow_sampsize = round(dz->param[RRR_WSIZENU] * MS_TO_SECS * (double)srate);
    /* FOLLOWING POSSIBLY UNNECESSARY WITH NEW WINSIZE MINIMUM */
    if(unadjusted_envwindow_sampsize < dz->iparam[RRR_SAMP_WSIZENU]) {
        k = dz->iparam[RRR_SAMP_WSIZENU];
        while(unadjusted_envwindow_sampsize<k)
            k /= 2;
        j = k * 2;
        if(j - unadjusted_envwindow_sampsize > k - unadjusted_envwindow_sampsize)
            dz->iparam[RRR_SAMP_WSIZENU] = (int)k;
        else
            dz->iparam[RRR_SAMP_WSIZENU] = (int)j;
    }
    if(unadjusted_envwindow_sampsize > dz->iparam[RRR_SAMP_WSIZENU]) {
        k = round((double)unadjusted_envwindow_sampsize/(double)dz->iparam[RRR_SAMP_WSIZENU]);
        dz->iparam[RRR_SAMP_WSIZENU] = (int)(dz->iparam[RRR_SAMP_WSIZENU] * k);
    }
    return(FINISHED);
}

