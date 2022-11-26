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
#include <modify.h>
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

#include <srates.h>

//TW UPDATES
#include <arrays.h>
#include <special.h>
#include <limits.h>

int sintab_pconsistency(dataptr dz);

//TW UPDATES
static int read_stackdata(char *str,dataptr dz);
static double semitone_to_ratio(double val);
static int convolve_pconsistency(dataptr dz);
static int create_convolve_bufs(dataptr dz);
static int read_loudness(char *str,dataptr dz);
static int setup_and_init_brktable_constants(dataptr dz);
/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

static int  create_granula_buffers(dataptr dz);
static int  grab_an_appropriate_block_of_memory(int bufdivisor,dataptr dz);

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
//  int is_spec = FALSE;
    dz->extra_bufcnt = -1;  /* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
    dz->bptrcnt = 0;
    dz->bufcnt  = 0;
    switch(dz->process) {
    case(MOD_LOUDNESS):         dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
    case(MOD_SPACE):
        switch(dz->mode) {
        case(MOD_PAN):          dz->extra_bufcnt = 0;   dz->bufcnt = 3;     break;
        case(MOD_MIRROR):       dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
        case(MOD_MIRRORPAN):    dz->extra_bufcnt = 0;   dz->bufcnt = 0;     break;
        case(MOD_NARROW):       dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
        default:
            sprintf(errstr,"Unknown case for MOD_SPACE in establish_bufptrs_and_extra_buffers()\n");
            return(PROGRAM_ERROR);
        }
        break;              
//TW NEW CASES
    case(SCALED_PAN):           dz->extra_bufcnt = 0;   dz->bufcnt = 3;     break;
    case(FIND_PANPOS):          dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;

    case(MOD_PITCH):
        switch(dz->mode) {
        case(MOD_TRANSPOS):             dz->extra_bufcnt = 0;   dz->bufcnt = 9;     break;
        case(MOD_TRANSPOS_SEMIT):       dz->extra_bufcnt = 0;   dz->bufcnt = 9;     break;
        case(MOD_TRANSPOS_INFO):        dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
        case(MOD_TRANSPOS_SEMIT_INFO):  dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
        case(MOD_ACCEL):                dz->extra_bufcnt = 0;   dz->bufcnt = 9;     break;
        case(MOD_VIBRATO):              dz->extra_bufcnt = 0;   dz->bufcnt = 9;     break;
        default:
            sprintf(errstr,"Unknown case for MOD_PITCH in establish_bufptrs_and_extra_buffers()\n");
            return(PROGRAM_ERROR);
        }
        break;              
    case(MOD_REVECHO):
        switch(dz->mode) {
        case(MOD_STADIUM):      dz->extra_bufcnt = 0;   dz->bufcnt = 4;     break;
        default:                dz->extra_bufcnt = 0;   dz->bufcnt = 2;     break;
        }
        break;
    case(MOD_RADICAL):
        switch(dz->mode) {
//TW SIMPLIFIED
//      case(MOD_REVERSE):      dz->extra_bufcnt = 0;   dz->bufcnt = 2;     break;
        case(MOD_REVERSE):      dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
        case(MOD_LOBIT):
        case(MOD_LOBIT2):       dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
        case(MOD_SCRUB):        dz->extra_bufcnt = 0;   dz->bufcnt = 2;     break;
        case(MOD_SHRED):        dz->extra_bufcnt = 0;   dz->bufcnt = 2;     break;
        case(MOD_RINGMOD):      dz->extra_bufcnt = 0;   dz->bufcnt = 1;     break;
        case(MOD_CROSSMOD):     dz->extra_bufcnt = 0;   dz->bufcnt = 3;     break;
        }
        break;
    case(BRASSAGE):             dz->extra_bufcnt = 1;   dz->bufcnt = 4;     break;
    case(SAUSAGE):              dz->extra_bufcnt = 1;   dz->bufcnt = 2;     break; /* ADD MORE LATER */
    case(SIN_TAB):              dz->extra_bufcnt = 0;   dz->bufcnt = 0;     break; /* ADD MORE LATER */
//TW NEW CASES
    case(STACK):                dz->extra_bufcnt = 0;   dz->bufcnt = 2;     break;
    case(CONVOLVE):             dz->extra_bufcnt = 0;   dz->bufcnt = 3;     break;
    case(SHUDDER):              dz->extra_bufcnt = 0;   dz->bufcnt = 4;     break;

/* TEMPORARY TEST ROUTINE */
    case(WORDCNT):              dz->extra_bufcnt = 0;   dz->bufcnt = 0;     break;
/* TEMPORARY TEST ROUTINE */
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
    dz->ptr_cnt    = -1;        //base constructor...process
    dz->array_cnt  = -1;
    dz->iarray_cnt = -1;
    dz->larray_cnt = -1;
    switch(dz->process) {
    case(MOD_LOUDNESS): dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
    case(MOD_SPACE):    dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
//TW UPDATES
    case(SCALED_PAN):   dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
    case(FIND_PANPOS):  dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;

    case(SIN_TAB):      dz->array_cnt=1; dz->iarray_cnt=0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
    case(MOD_PITCH):
        switch(dz->mode) {
        case(MOD_VIBRATO): dz->array_cnt=1;dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
        default:           dz->array_cnt=0;dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
        }
        break;
    case(MOD_REVECHO):
        switch(dz->mode) {
        case(MOD_STADIUM): dz->array_cnt=5;dz->iarray_cnt=0; dz->larray_cnt=1; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;        
        default:           dz->array_cnt=1;dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
        }
        break;
    case(MOD_RADICAL):
        switch(dz->mode) {
        case(MOD_REVERSE): dz->array_cnt=0;dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
        case(MOD_LOBIT):
        case(MOD_LOBIT2):  dz->array_cnt=0;dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
        case(MOD_SCRUB):   dz->array_cnt=2;dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
        case(MOD_SHRED):   dz->array_cnt=0;dz->iarray_cnt=1; dz->larray_cnt=2; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
        case(MOD_RINGMOD): dz->array_cnt=1;dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
        case(MOD_CROSSMOD): dz->array_cnt=0;dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
        }
        break;
    case(BRASSAGE): dz->array_cnt = 3; dz->iarray_cnt = 1; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 4; break;
    case(SAUSAGE):  dz->array_cnt = 3; dz->iarray_cnt = 2; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 4; break;
/* TEMPORARY TEST ROUTINE */
    case(WORDCNT):  dz->array_cnt = 0; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
/* TEMPORARY TEST ROUTINE */
//TW UPDATES
    case(STACK):    dz->array_cnt = 2; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
    case(CONVOLVE): dz->array_cnt = 1; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;
    case(SHUDDER):  dz->array_cnt = 6; dz->iarray_cnt = 0; dz->larray_cnt = 0; dz->ptr_cnt = 0; dz->fptr_cnt = 0; break;

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
    case(MOD_LOUDNESS):       
        switch(dz->mode) {
        case(LOUDNESS_GAIN):    
        case(LOUDNESS_DBGAIN):
        case(LOUDNESS_NORM):
        case(LOUDNESS_SET):
        case(LOUD_PROPOR):    
        case(LOUD_DB_PROPOR):
        case(LOUDNESS_PHASE):   
            setup_process_logic(SNDFILES_ONLY,  EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
        case(LOUDNESS_BALANCE): 
            setup_process_logic(TWO_SNDFILES,   EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
        case(LOUDNESS_LOUDEST): 
            setup_process_logic(MANY_SNDFILES,  OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
        case(LOUDNESS_EQUALISE): 
            setup_process_logic(MANY_SNDFILES,  UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
        default:
            sprintf(errstr,"Unknown case for MOD_LOUDNESS in assign_process_logic()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(MOD_SPACE):          
        switch(dz->mode) {
        case(MOD_PAN):          setup_process_logic(SNDFILES_ONLY,  UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
        case(MOD_MIRROR):       setup_process_logic(SNDFILES_ONLY,  EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
        case(MOD_MIRRORPAN):    setup_process_logic(UNRANGED_BRKFILE_ONLY,TO_TEXTFILE,  TEXTFILE_OUT,   dz);    break;
        case(MOD_NARROW):       setup_process_logic(SNDFILES_ONLY,  EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;
        default:
            sprintf(errstr,"Unknown case for MOD_SPACE in assign_process_logic()\n");
            return(PROGRAM_ERROR);
        }
        break;
//TW UPDATES
    case(SCALED_PAN):       setup_process_logic(SNDFILES_ONLY,  UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);        break;
    case(FIND_PANPOS):      setup_process_logic(SNDFILES_ONLY,  SCREEN_MESSAGE,     NO_OUTPUTFILE,  dz);        break;

    case(MOD_PITCH):          
        switch(dz->mode) {
        case(MOD_TRANSPOS):            setup_process_logic(SNDFILES_ONLY,UNEQUAL_SNDFILE,SNDFILE_OUT,   dz);    break;
        case(MOD_TRANSPOS_SEMIT):      setup_process_logic(SNDFILES_ONLY,UNEQUAL_SNDFILE,SNDFILE_OUT,   dz);    break;
        case(MOD_TRANSPOS_INFO):       setup_process_logic(SNDFILES_ONLY,SCREEN_MESSAGE, NO_OUTPUTFILE, dz);    break;
        case(MOD_TRANSPOS_SEMIT_INFO): setup_process_logic(SNDFILES_ONLY,SCREEN_MESSAGE, NO_OUTPUTFILE, dz);    break;
        case(MOD_ACCEL):               setup_process_logic(SNDFILES_ONLY,UNEQUAL_SNDFILE,SNDFILE_OUT,   dz);    break;
        case(MOD_VIBRATO):             setup_process_logic(SNDFILES_ONLY,UNEQUAL_SNDFILE,SNDFILE_OUT,   dz);    break;
        default:
            sprintf(errstr,"Unknown case for MOD_PITCH in assign_process_logic()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(MOD_REVECHO):    setup_process_logic(SNDFILES_ONLY,        UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(MOD_RADICAL):
        switch(dz->mode) {
        case(MOD_REVERSE): setup_process_logic(SNDFILES_ONLY,       EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;      
        case(MOD_LOBIT):
        case(MOD_LOBIT2):  setup_process_logic(SNDFILES_ONLY,       EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;      
        case(MOD_SCRUB):   setup_process_logic(SNDFILES_ONLY,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;      
        case(MOD_SHRED):   setup_process_logic(SNDFILES_ONLY,       EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;      
        case(MOD_RINGMOD): setup_process_logic(SNDFILES_ONLY,       EQUAL_SNDFILE,      SNDFILE_OUT,    dz);    break;      
        case(MOD_CROSSMOD): setup_process_logic(TWO_SNDFILES,       UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;      
        }
        break;
    case(BRASSAGE):       setup_process_logic(SNDFILES_ONLY,        UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(SAUSAGE):        setup_process_logic(MANY_SNDFILES,        UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(SIN_TAB):        setup_process_logic(NO_FILE_AT_ALL,       TO_TEXTFILE,        TEXTFILE_OUT,   dz);    break;
/* TEMPORARY TEST ROUTINE */
    case(WORDCNT):        setup_process_logic(WORDLIST_ONLY,        TO_TEXTFILE,        NO_OUTPUTFILE,  dz);    break;
/* TEMPORARY TEST ROUTINE */
//TW UPDATES
    case(STACK):          setup_process_logic(SNDFILES_ONLY,        UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(CONVOLVE):       setup_process_logic(TWO_SNDFILES,         UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
    case(SHUDDER):        setup_process_logic(SNDFILES_ONLY,        UNEQUAL_SNDFILE,    SNDFILE_OUT,    dz);    break;
 
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
    case(MOD_RADICAL):
        if(dz->mode==MOD_CROSSMOD)
            dz->has_otherfile = TRUE;
        else 
            dz->has_otherfile = FALSE;
        break;
    case(MOD_LOUDNESS):
        if(dz->mode==LOUDNESS_BALANCE)
            dz->has_otherfile = TRUE;
        else 
            dz->has_otherfile = FALSE;
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
    case(MOD_LOUDNESS):         exit_status = set_internalparam_data("",ap);        break;      
    case(MOD_SPACE):            exit_status = set_internalparam_data("",ap);        break;      
//TW UPDATES
    case(SCALED_PAN):           exit_status = set_internalparam_data("",ap);        break;      
    case(FIND_PANPOS):          exit_status = set_internalparam_data("",ap);        break;      

    case(MOD_PITCH):
        switch(mode) {
        case(MOD_ACCEL):        exit_status = set_internalparam_data(  "iiid",ap);  break;
        case(MOD_VIBRATO):      exit_status = set_internalparam_data( "iiiid",ap);  break;
        default:                exit_status = set_internalparam_data("iiiiid",ap);  break;
        }       
        break;
    case(MOD_REVECHO):
        switch(mode) {
        case(MOD_DELAY):        exit_status = set_internalparam_data("0ddidi",ap);  break;
        case(MOD_VDELAY):       exit_status = set_internalparam_data( "ddidi",ap);  break;
        case(MOD_STADIUM):      exit_status = set_internalparam_data( "ii"   ,ap);  break;
        default:
            sprintf(errstr,"Unknown case for MOD_REVECHO in set_legal_internalparam_structure()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(MOD_RADICAL):
        switch(mode) {
        case(MOD_REVERSE):      exit_status = set_internalparam_data("iiii",ap);            break;
        case(MOD_SHRED):        exit_status = set_internalparam_data("iiiiiiiiiiiiiii",ap); break;
        case(MOD_SCRUB):        exit_status = set_internalparam_data("ididdii",ap);         break;
        case(MOD_LOBIT):
        case(MOD_LOBIT2):       exit_status = set_internalparam_data("iiii",ap);            break;
        case(MOD_RINGMOD):      exit_status = set_internalparam_data("",ap);                break;
        case(MOD_CROSSMOD):     exit_status = set_internalparam_data("",ap);                break;
        default:
            sprintf(errstr,"Unknown case for MOD_RADICAL in set_legal_internalparam_structure()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(BRASSAGE):
        switch(mode) {
        case(GRS_PITCHSHIFT):   exit_status = set_internalparam_data("0000ddddiiiiiiiiiiiiiiiiii",ap);  break;
        case(GRS_TIMESTRETCH):  exit_status = set_internalparam_data("0000ddddiiiiiiiiiiiiiiiiii",ap);  break;
        case(GRS_REVERB):       exit_status = set_internalparam_data( "000ddddiiiiiiiiiiiiiiiiii",ap);  break;
        case(GRS_SCRAMBLE):     exit_status = set_internalparam_data( "000ddddiiiiiiiiiiiiiiiiii",ap);  break;
        case(GRS_GRANULATE):    exit_status = set_internalparam_data("0000ddddiiiiiiiiiiiiiiiiii",ap);  break;
        case(GRS_BRASSAGE):     exit_status = set_internalparam_data(    "ddddiiiiiiiiiiiiiiiiii",ap);  break;
        case(GRS_FULL_MONTY):   exit_status = set_internalparam_data(    "ddddiiiiiiiiiiiiiiiiii",ap);  break;
        default:
            sprintf(errstr,"Unknown case for BRASSAGE in set_legal_internalparam_structure()\n");
            return(PROGRAM_ERROR);
        }
        break;                         
    case(SAUSAGE):      exit_status = set_internalparam_data("ddddiiiiiiiiiiiiiiiiii",ap);  break;
    case(SIN_TAB):      exit_status = set_internalparam_data("",ap);                        break;
//TW UPDATES
    case(STACK):        exit_status = set_internalparam_data("d",ap);                       break;
    case(CONVOLVE):     exit_status = set_internalparam_data("",ap);                        break;
    case(SHUDDER):      exit_status = set_internalparam_data("",ap);                        break;

/* TEMPORARY TEST ROUTINE */
    case(WORDCNT):      exit_status = set_internalparam_data("",ap);                        break;
/* TEMPORARY TEST ROUTINE */
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
//TW UPDATE
    case(STACKDATA):    return read_stackdata(str,dz);
    case(LOUDNESS):     return read_loudness(str,dz);
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
    int exit_status = FINISHED;
#ifdef MULTICHAN
    int n;
#endif
    switch(dz->process) {
    case(MOD_LOUDNESS):
        return(FINISHED);           
    case(MOD_RADICAL):
        switch(dz->mode) {
        case(MOD_SHRED):    return shred_preprocess(dz);
        case(MOD_SCRUB):    return scrub_preprocess(dz);
        case(MOD_RINGMOD):  return create_rm_sintab(dz);
        default:            return(FINISHED);           
        }
        break;
    case(MOD_SPACE):        return modspace_preprocess(dz);

//TW UPDATES
    case(SCALED_PAN):   return scaledpan_preprocess(dz);
    case(FIND_PANPOS):  return FINISHED;

    case(MOD_PITCH):    return vtrans_preprocess(dz);
    case(MOD_REVECHO):  return delay_preprocess(dz);
    case(BRASSAGE):
        /* RWD: create outfile here  - different chan count from infile */
#ifdef MULTICHAN
        n = dz->infile->channels;
        dz->infile->channels = dz->outfile->channels;
#endif
        if(dz->mode==GRS_REVERB || dz->mode==GRS_BRASSAGE || dz->mode == GRS_FULL_MONTY){               
            if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
                return(exit_status);
        }
#ifdef MULTICHAN
        dz->infile->channels = n;
#endif
        return FINISHED;
    case(SAUSAGE):      return sausage_preprocess(dz);
    case(SIN_TAB):      return FINISHED;
//TW UPDATES
    case(STACK):        return stack_preprocess(dz);
    case(CONVOLVE):
        if(dz->mode == CONV_TVAR)
            return convolve_preprocess(dz);
        break;
    case(SHUDDER):      return FINISHED;

/* TEMPORARY TEST ROUTINE */
    case(WORDCNT):          
        return FINISHED;        
/* TEMPORARY TEST ROUTINE */
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
    /*int exit_status = FINISHED;*/

    if(dz->process==BRASSAGE || dz->process==SAUSAGE)
        display_virtual_time(0L,dz);

    switch(dz->process) {
    case(MOD_SPACE):
        switch(dz->mode) {
        case(MOD_PAN):       return dopan(dz);
        case(MOD_MIRROR):    return mirroring(dz);
        case(MOD_MIRRORPAN): mirror_panfile(dz);    return(FINISHED);
        case(MOD_NARROW):    return narrow_sound(dz);
        default:
            sprintf(errstr,"Unknown mode for MOD_SPACE in groucho_process_file()\n");
            return(PROGRAM_ERROR);
        }
        break;
//TW UPDATES
    case(SCALED_PAN):   return dopan(dz);
    case(FIND_PANPOS):  return findpan(dz);

    case(MOD_PITCH):    return process_varispeed(dz);
    case(MOD_REVECHO):  
        switch(dz->mode) {
        case(MOD_STADIUM):  return do_stadium(dz);
        case(MOD_DELAY):
        case(MOD_VDELAY):   return do_delay(dz);
        default:
            sprintf(errstr,"Unknown mode for MOD_REVECHO in groucho_process_file()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(MOD_RADICAL):  
        switch(dz->mode) {
        case(MOD_REVERSE):  return do_reversing(dz);
        case(MOD_SHRED):    return shred_process(dz);
        case(MOD_SCRUB):    return do_scrubbing(dz);
        case(MOD_LOBIT):
        case(MOD_LOBIT2):   return lobit_process(dz);
        case(MOD_RINGMOD):  return ring_modulate(dz);
        case(MOD_CROSSMOD): return cross_modulate(dz);
        default:
            sprintf(errstr,"Unknown mode for MOD_RADICAL in groucho_process_file()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(MOD_LOUDNESS): return loudness_process(dz);
    case(BRASSAGE):     return granula_process(dz);
    case(SAUSAGE):      return granula_process(dz);
    case(SIN_TAB):      return generate_sintable(dz);
//TW UPDATES
    case(STACK):        return do_stack(dz);
    case(CONVOLVE):     return do_convolve(dz);
    case(SHUDDER):      return do_shudder(dz);

    default:
        sprintf(errstr,"Unknown case in process_file()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);   /* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
/*  int exit_status = FINISHED;*/
    handle_pitch_zeros(dz);
    switch(dz->process) {
    case(MOD_SPACE):    return modspace_pconsistency(dz);
//TW UPDATE
    case(SCALED_PAN):   
        if(dz->infile->channels!=MONO) {
            sprintf(errstr,"ERROR: SCALED PAN only works with MONO input files.\n");
            return(DATA_ERROR);
        }
        return FINISHED;

    case(MOD_REVECHO):
        if(dz->mode==MOD_STADIUM)
            return stadium_pconsistency(dz);
        return FINISHED;
    case(MOD_RADICAL):
        switch(dz->mode) {
        case(MOD_LOBIT):    return lobit_pconsistency(dz);
        case(MOD_SHRED):    return shred_pconsistency(dz);
        case(MOD_SCRUB):    return scrub_pconsistency(dz);
        case(MOD_CROSSMOD): return crossmod_pconsistency(dz);
        default:         return FINISHED;
        }
        break;
    case(BRASSAGE):
    case(SAUSAGE):      return granula_pconsistency(dz);
    case(SIN_TAB):      return sintab_pconsistency(dz);
//TW UPDATES
    case(STACK):        return FINISHED;
    case(CONVOLVE):     return convolve_pconsistency(dz);
    case(SHUDDER):      return FINISHED;
    case(FIND_PANPOS):  return FINISHED;

/* TEMPORARY TEST ROUTINE */
    case(WORDCNT):      return FINISHED;        
/* TEMPORARY TEST ROUTINE */
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
    case(MOD_LOUDNESS):         
        return create_sndbufs(dz);
    case(MOD_SPACE):            
        if(dz->mode!=MOD_MIRRORPAN)
            return create_sndbufs(dz);
        return(FINISHED);
//TW UPDATE
    case(SCALED_PAN):   return create_sndbufs(dz);

    case(MOD_PITCH):    return create_modspeed_buffers(dz);
    case(MOD_REVECHO):  return create_delay_buffers(dz);
    case(MOD_RADICAL):
        switch(dz->mode) {
        case(MOD_REVERSE):  return create_reversing_buffers(dz);
        case(MOD_SHRED):    return create_shred_buffers(dz);
        case(MOD_SCRUB):    return create_scrub_buffers(dz);
        case(MOD_LOBIT):
        case(MOD_LOBIT2):   return create_sndbufs(dz);
        case(MOD_RINGMOD):  return create_sndbufs(dz);
        case(MOD_CROSSMOD): return create_crossmod_buffers(dz);
        }
        break;
    case(BRASSAGE):     return create_granula_buffers(dz);
    case(SAUSAGE):      return create_sausage_buffers(dz);
    case(SIN_TAB):      return FINISHED;
//TW UPDATES
    case(FIND_PANPOS):  return create_sndbufs(dz);
    case(STACK):        return create_sndbufs(dz);
    case(CONVOLVE):     return create_convolve_bufs(dz);
    case(SHUDDER):      return create_sndbufs(dz);

/* TEMPORARY TEST ROUTINE */
    case(WORDCNT):          
        return(FINISHED);
/* TEMPORARY TEST ROUTINE */
    default:
        sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/*************************** CREATE_GRANULA_BUFFERS **************************/

#ifndef MULTICHAN

int create_granula_buffers(dataptr dz)
{
    int   exit_status,enough_memory;
    int   convert_to_stereo = FALSE, overall_size = 0, bufdivisor = 0;
    float *tailend;
    int  stereo_buflen = 0, stereo_bufxs = 0, outbuflen, bbb_size;
//TW All buffers are in floats, so this not needed
//  int   lfactor = sizeof(int)/sizeof(float);
    if(dz->iparray[GRS_FLAGS][G_SPACE_FLAG])
        convert_to_stereo = TRUE;                           
    if((dz->extrabuf[GRS_GBUF] = (float *)malloc(dz->iparam[GRS_GLBUF_SMPXS] * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create grain buffer.\n");     /* GRAIN BUFFER */
        return(MEMORY_ERROR);
    }
                /* CALCULATE NUMBER OF BUFFER CHUNKS REQUIRED : bufdivisor */

    if(dz->iparam[GRS_CHANNELS]>0)
        bufdivisor += 2;                
//TW All buffers are in floats
//  bufdivisor += 1 + 1 + lfactor;      
    bufdivisor += 3;        
    if(convert_to_stereo)
//TW All buffers are in floats
//      bufdivisor += 1 + lfactor;      
        bufdivisor += 2;        

    enough_memory = 0;
    if((exit_status = grab_an_appropriate_block_of_memory(bufdivisor,dz))<0)
        return(exit_status);
    bbb_size = /*dz->bigbufsize*/dz->buflen;
    while(!enough_memory) {
                    /* CALCULATE AND ALLOCATE TOTAL MEMORY REQUIRED : overall_size */
        if ((overall_size = /*dz->bigbufsize*/dz->buflen * bufdivisor) < 0) {       /* overflow */
            sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
            return(MEMORY_ERROR);
        }
        if ((overall_size += (dz->iparam[GRS_BUF_SMPXS] + dz->iparam[GRS_LBUF_SMPXS])) < 0) {       /* overflow */
            sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
            return(MEMORY_ERROR);
        }
        if(dz->iparam[GRS_CHANNELS])                
            overall_size += 2 * dz->iparam[GRS_BUF_SMPXS];  
        if(overall_size<0) {                                                 /* overflow */
            sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n"); 
            return(MEMORY_ERROR);
        }
        if((dz->bigbuf=(float *)malloc(overall_size * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
            return(MEMORY_ERROR);
        }
                    /* SET SIZE OF inbuf, outbuf, AND Lbuf (FOR CALCS IN LONGS) */
        outbuflen   = dz->buflen;
        if(convert_to_stereo)                   
            outbuflen *= 2;
        if(dz->iparam[GRS_CHANNELS]) {
            stereo_buflen = dz->buflen * 2;
            stereo_bufxs  = dz->iparam[GRS_BUF_SMPXS] * 2;      /**** CHANGED MAY 1998 ****/
        }                                        
        dz->iparam[GRS_LONGS_BUFLEN] = outbuflen;           
        if(dz->iparam[GRS_LBUF_SMPXS] <= dz->iparam[GRS_LONGS_BUFLEN])                                      
            enough_memory = 1;
        else {
            if((/*dz->bigbufsize*/dz->buflen += bbb_size) < 0) {    /* overflow */
                sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
                return(MEMORY_ERROR);
            }
        }
    }
                /* DIVIDE UP ALLOCATED MEMORY IN SPECIALISED BUFFERS */ 
                                                                    
    if(dz->iparam[GRS_CHANNELS]) {                      
        dz->sampbuf[GRS_SBUF] = dz->bigbuf;                                  
        dz->sampbuf[GRS_BUF]  = dz->sampbuf[GRS_SBUF] + stereo_buflen + stereo_bufxs;            
    } else                                               
        dz->sampbuf[GRS_BUF]     = dz->bigbuf;                      /* buf: complete input buffer with wrap-around space */                 
    dz->sbufptr[GRS_BUF]   = dz->sampbuf[GRS_BUF] + dz->buflen;                /* bufend:   start of wrap-around area */
    tailend                = dz->sbufptr[GRS_BUF] + dz->iparam[GRS_BUF_SMPXS]; /* tailend:  end of wrap-around area */
    dz->sampbuf[GRS_IBUF]  = dz->sampbuf[GRS_BUF] + dz->iparam[GRS_BUF_SMPXS]; /* ibuf: input buffer 'base' after wrap-around begins */                  
    dz->fptr[GRS_LBUF]     = /*(int *)*/tailend;                                   /* Lbuf: buffer for calculations */
    dz->fptr[GRS_LBUFEND]  = dz->fptr[GRS_LBUF] + dz->iparam[GRS_LONGS_BUFLEN];/* Lbufend:  start of Lbuf wrap-around area */ 
    dz->fptr[GRS_LTAILEND] = dz->fptr[GRS_LBUFEND] + dz->iparam[GRS_LBUF_SMPXS];/* Ltailend: end of Lbuf wrap-around area */
    dz->fptr[GRS_LBUFMID]  = dz->fptr[GRS_LBUF]    + dz->iparam[GRS_LBUF_SMPXS];/* Lbufmid:  Lbuf 'base' after wrap-around begins */
    dz->sampbuf[GRS_OBUF]  = (dz->fptr[GRS_LTAILEND]);                  /* obuf:     output buffer */

                                /* INITIALISE BUFFERS */

    memset((char *)dz->bigbuf,0,overall_size * sizeof(float));
    return(FINISHED);
}

#else

int create_granula_buffers(dataptr dz)
{
    int   exit_status,enough_memory;
    int   spatialise_output = FALSE, overall_size = 0, bufdivisor = 0, chans = dz->infile->channels;
    float *tailend;
    int  multichan_buflen = 0, multichan_bufxs = 0, grainbuflen, bbb_size;
    if(dz->iparray[GRS_FLAGS][G_SPACE_FLAG])
        spatialise_output = TRUE;                           
    if((dz->extrabuf[GRS_GBUF] = (float *)malloc(dz->iparam[GRS_GLBUF_SMPXS] * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create grain buffer.\n");     /* GRAIN BUFFER */
        return(MEMORY_ERROR);
    }
                /* CALCULATE NUMBER OF BUFFER CHUNKS REQUIRED : bufdivisor */

    if(dz->iparam[GRS_CHANNELS]>0)
        bufdivisor += chans;                
    bufdivisor += 3;        

// MARCH 2010
//  if(spatialise_output)
        bufdivisor += dz->out_chans;

    enough_memory = 0;
    if((exit_status = grab_an_appropriate_block_of_memory(bufdivisor,dz))<0)
        return(exit_status);
    bbb_size = dz->buflen;
    while(!enough_memory) {
                    /* CALCULATE AND ALLOCATE TOTAL MEMORY REQUIRED : overall_size */
        if ((overall_size = dz->buflen * bufdivisor) < 0) {     /* overflow */
            sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
            return(MEMORY_ERROR);
        }
        if ((overall_size += (dz->iparam[GRS_BUF_SMPXS] + dz->iparam[GRS_LBUF_SMPXS])) < 0) {       /* overflow */
            sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
            return(MEMORY_ERROR);
        }
        if(dz->iparam[GRS_CHANNELS])                
            overall_size += chans * dz->iparam[GRS_BUF_SMPXS];  
        if(overall_size<0) {                                                 /* overflow */
            sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n"); 
            return(MEMORY_ERROR);
        }
        if((dz->bigbuf=(float *)malloc(overall_size * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
            return(MEMORY_ERROR);
        }
                    /* SET SIZE OF inbuf, outbuf, AND Lbuf (FOR CALCS IN LONGS) */
    
        grainbuflen   = dz->buflen * dz->iparam[GRS_INCHANS];   
        if(dz->iparam[GRS_CHANNELS]) {
            multichan_buflen = dz->buflen * chans;
            multichan_bufxs  = dz->iparam[GRS_BUF_SMPXS] * chans;       /**** CHANGED MAY 1998 ****/
        }                                        
        dz->iparam[GRS_LONGS_BUFLEN] = grainbuflen;         
        if(dz->iparam[GRS_LBUF_SMPXS] <= dz->iparam[GRS_LONGS_BUFLEN])                                      
            enough_memory = 1;
        else {
            if((dz->buflen += bbb_size) < 0) {  /* overflow */
                sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
                return(MEMORY_ERROR);
            }
        }
    }
                /* DIVIDE UP ALLOCATED MEMORY IN SPECIALISED BUFFERS */ 
                                                                    
    if(dz->iparam[GRS_CHANNELS]) {                      
        dz->sampbuf[GRS_SBUF] = dz->bigbuf;                                  
        dz->sampbuf[GRS_BUF]  = dz->sampbuf[GRS_SBUF] + multichan_buflen + multichan_bufxs;          
    } else                                               
        dz->sampbuf[GRS_BUF]     = dz->bigbuf;                      /* buf: complete input buffer with wrap-around space */                 
    dz->sbufptr[GRS_BUF]   = dz->sampbuf[GRS_BUF] + dz->buflen;                /* bufend:   start of wrap-around area */
    tailend                = dz->sbufptr[GRS_BUF] + dz->iparam[GRS_BUF_SMPXS]; /* tailend:  end of wrap-around area */
    dz->sampbuf[GRS_IBUF]  = dz->sampbuf[GRS_BUF] + dz->iparam[GRS_BUF_SMPXS]; /* ibuf: input buffer 'base' after wrap-around begins */                  
    dz->fptr[GRS_LBUF]     = /*(int *)*/tailend;                                   /* Lbuf: buffer for calculations */
    dz->fptr[GRS_LBUFEND]  = dz->fptr[GRS_LBUF] + dz->iparam[GRS_LONGS_BUFLEN];/* Lbufend:  start of Lbuf wrap-around area */ 
    dz->fptr[GRS_LTAILEND] = dz->fptr[GRS_LBUFEND] + dz->iparam[GRS_LBUF_SMPXS];/* Ltailend: end of Lbuf wrap-around area */
    dz->fptr[GRS_LBUFMID]  = dz->fptr[GRS_LBUF]    + dz->iparam[GRS_LBUF_SMPXS];/* Lbufmid:  Lbuf 'base' after wrap-around begins */
    dz->sampbuf[GRS_OBUF]  = (dz->fptr[GRS_LTAILEND]);                  /* obuf:     output buffer */

                                /* INITIALISE BUFFERS */

    memset((char *)dz->bigbuf,0,overall_size * sizeof(float));
    return(FINISHED);
}

#endif

/*  INPUT BUFFER :-     
 *
 *  |-----------BUFLEN-----------|
 *
 *  buf      ibuf          bufend    tailend
 *  |_________|__________________|buf_smpxs|  ..... (obuf->)
 *                               /
 *  |buf_smpxs|           <<-COPY_________/
 *
 *            |-----------BUFLEN-----------|
 *
 *
 *
 *  OUTPUT LONGS BUFFER:-
 *
 *  Lbuf       Lbufmid    Lbufend
 *  |____________|_______________|_Lbuf_smpxs_|
 *                               /
 *  |_Lbuf_smpxs_|         <<-COPY___________/  
 *
 */

/*************************** GRAB_AN_APPROPRIATE_BLOCK_OF_MEMORY **************************/

#ifndef MULTICHAN

int grab_an_appropriate_block_of_memory(int bufdivisor,dataptr dz)
{
    int this_bloksize, standard_block = (int) ((long)Malloc(-1));
    int  sector_blok;
    standard_block /= sizeof(float);
    this_bloksize = standard_block;
    dz->buflen = 0;
    while(dz->buflen <= 0){
        if((dz->buflen = this_bloksize)< 0) {   /* arithmetic overflow */
            sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
            return(MEMORY_ERROR);
        }
        this_bloksize += standard_block;
                /* CALCULATE SIZE OF BUFFER REQUIRED : dz->bigbufsize */

        dz->buflen -= dz->iparam[GRS_BUF_SMPXS] + dz->iparam[GRS_LBUF_SMPXS];   /* Allow for overflow areas */
        if(dz->iparam[GRS_CHANNELS])                
            dz->buflen -= 2 * dz->iparam[GRS_BUF_SMPXS];    /* Allow for overflow space in additional stereo inbuf */
        dz->buflen /= bufdivisor;                           /* get unit buffersize */
        sector_blok = F_SECSIZE;                            /* Read and write buf sizes must be multiples of SECSIZE ... for now */
        if(dz->iparam[GRS_CHANNELS])                        /* If reading stereo: 2* SECSIZE reduces to single mono SECSIZE */
            sector_blok *= 2;                               /* So dz->bigbufsize must be a multiple of (2 * SECSIZE) */
        dz->buflen = (dz->buflen/sector_blok) * sector_blok;
    }
    return(FINISHED);
}

#else

int grab_an_appropriate_block_of_memory(int bufdivisor,dataptr dz)
{
    int this_bloksize, standard_block = (int)Malloc(-1);
    int  sector_blok, chans = dz->infile->channels;
    standard_block /= sizeof(float);
    this_bloksize = standard_block;
    dz->buflen = 0;
    while(dz->buflen <= 0){
        if((dz->buflen = this_bloksize)< 0) {   /* arithmetic overflow */
            sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
            return(MEMORY_ERROR);
        }
        this_bloksize += standard_block;
                /* CALCULATE SIZE OF BUFFER REQUIRED : dz->bigbufsize */

        dz->buflen -= dz->iparam[GRS_BUF_SMPXS] + dz->iparam[GRS_LBUF_SMPXS];   /* Allow for overflow areas */
        if(dz->iparam[GRS_CHANNELS])                
            dz->buflen -= chans * dz->iparam[GRS_BUF_SMPXS];    /* Allow for overflow space in additional multichannel inbuf */
        dz->buflen /= bufdivisor;                           /* get unit buffersize */
        sector_blok = F_SECSIZE;                            /* Read and write buf sizes must be multiples of SECSIZE ... for now */
        if(dz->iparam[GRS_CHANNELS])                        /* If reading multichan: chans * SECSIZE reduces to single mono SECSIZE */
            sector_blok *= chans;                               /* So dz->bigbufsize must be a multiple of (chans * SECSIZE) */
        dz->buflen = (dz->buflen/sector_blok) * sector_blok;
    }
    return(FINISHED);
}

#endif

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if     (!strcmp(prog_identifier_from_cmdline,"loudness"))       dz->process = MOD_LOUDNESS;
    else if(!strcmp(prog_identifier_from_cmdline,"space"))          dz->process = MOD_SPACE;
//TW UPDATES
    else if(!strcmp(prog_identifier_from_cmdline,"scaledpan"))      dz->process = SCALED_PAN;
    else if(!strcmp(prog_identifier_from_cmdline,"findpan"))        dz->process = FIND_PANPOS;

    else if(!strcmp(prog_identifier_from_cmdline,"speed"))          dz->process = MOD_PITCH;
    else if(!strcmp(prog_identifier_from_cmdline,"revecho"))        dz->process = MOD_REVECHO;
    else if(!strcmp(prog_identifier_from_cmdline,"radical"))        dz->process = MOD_RADICAL;
    else if(!strcmp(prog_identifier_from_cmdline,"brassage"))       dz->process = BRASSAGE;
    else if(!strcmp(prog_identifier_from_cmdline,"sausage"))        dz->process = SAUSAGE;
//TW UPDATE (originally missing from cmdlinr list
    else if(!strcmp(prog_identifier_from_cmdline,"spaceform"))      dz->process = SIN_TAB;
//TW UPDATES
    else if(!strcmp(prog_identifier_from_cmdline,"stack"))          dz->process = STACK;
    else if(!strcmp(prog_identifier_from_cmdline,"convolve"))       dz->process = CONVOLVE;
    else if(!strcmp(prog_identifier_from_cmdline,"shudder"))        dz->process = SHUDDER;

/* TEMPORARY TEST ROUTINE */
    else if(!strcmp(prog_identifier_from_cmdline,"wordcnt"))        dz->process = WORDCNT;
/* TEMPORARY TEST ROUTINE */
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        return(USAGE_ONLY);
    }
    /*RWD 9:2001 need retval */
    return FINISHED;
}

/********************************************************************************************/
/********************************** FORMERLY IN usage.c *************************************/
/********************************************************************************************/

/******************************** USAGE1 ********************************/

int usage1(void)
{
    sprintf(errstr,
    "USAGE: modify NAME (mode) infile outfile (datafile) parameters\n"
    "\n"
    "where NAME can be any one of\n"
    "\n"
    "loudness   space     speed     revecho\n"
//TW UPDATE
//  "brassage   sausage   radical\n"
    "brassage     sausage     radical     spaceform\n"
//TW UPDATE
    "scaledpan    findpan    convolve    stack       shudder\n"
    "\n"
    "Type 'modify  brassage'  for more info on modify  brassage option... ETC.\n");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/
 /*RWD 9:2001 usage message changes for loudness */
int usage2(char *str)
{
    if (!strcmp(str,"loudness")) {       
        fprintf(stdout,
        "ADJUST LOUDNESS OF A SOUNDFILE\n\n"
        "USAGE: modify loudness 1 infile         outfile gain\n"
        "OR:    modify loudness 11-12 infile     outfile gain\n"
        "OR:    modify loudness 2 infile         outfile gain\n"        /* NORMALISE IF NESS */
        "OR:    modify loudness 3 infile         outfile [-llevel]\n"       /* NORMALISE IF NESS */
        "OR:    modify loudness 4 infile         outfile [-llevel]\n"       /* FORCE TO SET LEVEL */
        "OR:    modify loudness 5 infile infile2 outfile\n"                 /* BALANCE */
        "OR:    modify loudness 6 infile outfile\n"                         /* INVERT PHASE */
        "OR:    modify loudness 7 infile infile2 etc.\n"                    /* FIND LOUDEST */
        "OR:    modify loudness 8 infile infile2 etc. outfile\n\n"          /* NORMALISE TO LOUDEST */
        "WHERE MODES ARE\n"
        "1) GAIN:         adjust level by factor GAIN.\n"
        "2) dBGAIN:       adjust level by GAIN dB. Range +-96.0\n"
        "3) NORMALISE:    force level (if ness) to max possible, or to LEVEL given.\n"
        "4) FORCE LEVEL:  force level to maximum possible, or to LEVEL given.\n"
        "5) BALANCE:      force max level of file1 to max level of file 2.\n"
        "6) INVERT PHASE: Invert phase of the sound.\n"
        "7) FIND LOUDEST: find loudest file.\n"
        "8) EQUALISE:     force all files to level of loudest file.\n"
        "11) PROPORTIONAL:    adjust level with envelope stretched to dur of sound.\n"
        "12) PROPORTIONAL dB: adjust level with dB envelope stretched to dur of sound.\n"
        "                 Infiles are rescaled in input order, with output names as\n" 
//TW    "                 outfile, outfile1, outfile2, outfile3 etc.\n"     
        "                 outfile0, outfile1, outfile2, outfile3 etc.\n"        
        );
    } else if(!strcmp(str,"space")) {        
        fprintf(stdout,
        "CREATE OR ALTER DISTRIBUTION OF SOUND IN STEREO SPACE\n\n"
        "USAGE: modify space 1 infile outfile pan [-pprescale]\n"
        "OR:    modify space 2 infile outfile\n"
        "OR:    modify space 3 infile outfile\n"        
        "OR:    modify space 4 infile outfile narrowing\n\n"
        "WHERE MODES ARE\n"
        "1) PAN:         Position or move mono sound in a stereo field.\n"
        "                Prescale reduces inlevel to avoid clipping (default 0.7)\n"
        "2) MIRROR:      Invert stereo positions in a stereo file.\n"
        "3) MIRRORPAN:   Invert stereo positions in a pan data file.\n"
        "4) NARROW:      Narrow the stereo image of a sound.\n"
        "                NARROWING range, -1 to 1\n"
        "                1   leaves stereo image as it is.\n"
        "                .5  narrows stereo image by half.\n"
        "                0   converts stereo image to mono.\n"
        "                -ve vals work similarly, but also invert stereo image.\n");
//TW UPDATES
    } else if(!strcmp(str,"spaceform")) {        
        fprintf(stdout,
        "CREATE A SINUSOIDAL SPATIAL DISTRIBUTION DATA FILE\n\n"
        "USAGE: modify spaceform outpanfile cyclelen width dur quantisation phase\n\n"
        "CYCLELEN     is the duration of one complete sinusoidal pan cycle\n"
        "WIDTH        is the width of the pan (from 0 to full width,1)\n"
        "DUR          is the duration of the output file.\n" 
        "QUANTISATION is time step between successive space-position specifications.\n" 
        "PHASE        is the angular position at which the pan starts.\n" 
        "             0 is full left, 360 is full right.\n" 
        "cyclelen and width may vary over time\n");
    } else if(!strcmp(str,"scaledpan")) {        
        fprintf(stdout,
        "DISTRIBUTE SOUND IN STEREO SPACE, SCALING PAN DATA TO SNDFILE DURATION\n\n"
        "USAGE: modify scaledpan infile outfile pan [-pprescale]\n\n"
        "PAN      breakpoint file of time, position pairs.\n"
        "         Positions sound in a stereo field, from -1 (Left) to 1 (Right) or beyond.\n"
        "PRESCALE reduces input level to avoid clipping (default 0.7)\n");
    } else if(!strcmp(str,"findpan")) {      
        fprintf(stdout,
        "FIND STEREO-PAN POSITION OF A SOUND IN A STEREO FILE\n\n"
        "USAGE: modify findpan infile time\n\n"
        "Process assumes file contains a sound which has previously been panned to a position\n"
        "in the stereo field. The process will give misleading results if this is not the case.\n");
    } else if(!strcmp(str,"stack")) {        
        fprintf(stdout,
        "CREATE A MIX WHICH STACKS TRANPOSED VERSIONS OF SOURCE ON ONE ANOTHER\n\n"
        "USAGE: modify stack infile outfile transpos count lean atk-offset gain dur [-s] [-n]\n\n"
        "TRANSPOS   when numeric, is (semitone) transposition between successive copies\n"
        "           or (as a file) a set of transposition values for each stack component.\n"
        "COUNT      is the number of copies in the stack.\n"
        "LEAN       is the loudness of the highest component, relative to the lowest.\n" 
        "ATK_OFFSET is time at which attack of sound occurs.\n" 
        "GAIN       is an overall amplifying factor on the ouptut sound.\n"
        "DUR        how much of the output to make (a proportion, from 0 to 1).\n"
        "-s         see the relative levels of the layers in the stack.\n"
        "-n         Normalise the output.\n");
    } else if(!strcmp(str,"convolve")) {         
        fprintf(stdout,
        "CONVOLVE THE FIRST SOUND WITH THE SECOND\n\n"
        "USAGE: modify convolve 1 infile1 infile2 outfile\n"
        "OR:    modify convolve 2 infile1 infile2 outfile tranposfile\n\n"
        "TRANSPOSFILE is textfile of time / semitone-transposition pairs\n"
        "infile2 must NOT be longer than infile1\n"
        "and both files must have the same channel count.\n");
    } else if(!strcmp(str,"shudder")) {      
        fprintf(stdout,
        "SHUDDER A STEREO FILE\n\n"
        "USAGE: modify shudder infile outfile\n"
        "         starttime  frq  scatter  stereo_spread  mindepth  maxdepth  minwidth  maxwidth [-b]\n\n"
        "START_TIME    is time when Shuddering will begin.\n"
        "FREQUENCY     is (average) frequency of the shuddering.\n"
        "SCATTER       randomises the shudder events, in time. (0 - 1)\n"
        "STEREO_SPREAD positions the shudder events in space. (0 - 1)\n"
        "DEPTH         amplitude of shudders (each gets randval btwn MIN & MAX) Range 0-1\n"
        "EVENT WIDTH   durations of shudder events (each gets randval btwn MIN & MAX)\n"
        "-b            Balance average level of the stereo channels\n");
    } else if(!strcmp(str,"speed")) {        
        fprintf(stdout,
        "CHANGE THE SPEED & PITCH OF THE SRC SOUND.\n\n"
        "USAGE: modify speed 1 infile outfile     speed             [-o]\n"
        "OR:    modify speed 2 infile outfile     semitone-transpos [-o]\n"
        "OR:    modify speed 3 infile outtextfile speed             [-o]\n"
        "OR:    modify speed 4 infile outtextfile semitone-transpos [-o]\n"
        "OR:    modify speed 5 infile outfile     accel  goaltime   [-sstarttime]\n"
        "OR:    modify speed 6 infile outfile    vibrate vibdepth\n"
        "WHERE MODES ARE\n"
        "1)  Vary speed/pitch of a sound.\n"
        "2)  Vary speed/pitch by constant (fractional) no. of semitones.\n"
        "3)  Get information on varying speed in a time-changing manner.\n"
        "4)  Get info on time-variable speedchange in semitones.\n"
        "    -o  brkpnt times read as outfile times (default: as infile times).\n"
        "5)  Accelerate or decelerate a sound.\n"
        "    ACCEL:     multiplication of speed reached by GOALTIME.\n"
        "    GOALTIME:  time in OUTPUT file at which accelerated speed reached.\n"
        "               If infile not exhausted there, it continues to accel.\n"
        "               If insound finishes before GOALTIME reached,\n"
        "               outfile won't reach specified acceleration value.\n"
        "    STARTTIME: time in input/output file at which accel begins.\n"
        "6)  Add vibrato to a sound.\n"
        "    VIBRATE:   is rate of vibrato shaking in cycles-per-second.\n"
        "    VIBDEPTH:  is vibrato depth in (possibly fractional) semitones.\n"
        "    both of these may vary in time.\n"); 
    } else if(!strcmp(str,"revecho")) {      
        fprintf(stdout,
        "CREATE REVERB, ECHO, OR RESONANCE AROUND SOUND\n\n"
        "USAGE: modify revecho 1 infl outfl delay mix feedback tail [-pprescale] [-i]\n"
        "OR:    modify revecho 2 infl outfl delay mix feedback\n"
        "                                   lfomod lfofreq lfophase lfodelay\n"
        "                                   tail [-pprescale] [-sseed]\n"
        "OR:    modify revecho 3 infl outfl [-ggain] [-rroll_off] [-ssize] [-ecount] [-n]\n\n"
        "WHERE MODES ARE\n"
        "1) STANDARD DELAY with feedback, & mix (0=dry) of original & delayed signal.\n"
        "2) VARYING DELAY  with low frequency oscillator varying delay time.\n"
        "3) STADIUM ECHO   create stadium P.A. type echos.\n\n"
        "DELAY     Delay time, in milliseconds.\n"
        "MIX       amount of delayed signal in final mix: 0 gives 'dry' result.\n"
        "FEEDBACK  produces resonance related to delay time (with short times).\n"
        "TAIL      is time to allow delayed signal to decay to zero.\n"
        "PRESCALE  prescales input level, to avoid overload.\n"
        "-i        inverts the dry signal (for phasing effects).\n"
        "LFOMOD    is the depth of delay-variation sweep.\n"
        "LFOFREQ   is the freq of the sweep: -ve vals give random oscillations.\n"
        "LFOPHASE  is the start phase of the sweep.\n"
        "LFODELAY  is the time before the seep begins.\n"
        "SEED      Nonzero value gives reproducible output (with same seed)\n"
        "          where random oscillations are used.\n"
        "GAIN      to apply to input signal: Default is %lf\n"
        "SIZE      multiplies average time between echoes: (Default time 0.1 secs).\n"
        "ROLL_OFF  rate of loss of level across stadium (default 1)\n"
        "COUNT     number of stadium echoes: Default (max) is %d\n"
        "-n        Normalise the output, preventing overload,\n",STAD_PREGAIN_DFLT,MAX_ECHOCNT);
    } else if(!strcmp(str,"radical")) {      
        fprintf(stdout,
        "RADICAL CHANGES TO THE SOUND.\n\n"
        "USAGE: modify radical 1 infile outfile\n"
        "OR:    modify radical 2 infile outfile repeats chunklen [-sscatter] [-n]\n"
        "OR:    modify radical 3 infile outfile dur [-ldown] [-hup] [-sstart] [-eend] [-f]\n"
        "OR:    modify radical 4 infile outfile  bit_resolution  srate_division\n"
        "OR:    modify radical 5 infile outfile  modulating-frq\n"
        "OR:    modify radical 6 infile1 infile2 outfile\n\n"
        "OR:    modify radical 7 infile1 outfile bit_resolution\n\n"
        "MODES ARE\n"
        "1) REVERSE:         sound plays backwards.\n"
        "2) SHRED:           sound is shredded, within its existing duration.\n"
        "                    REPEATS   no. of repeats of shredding process.\n"
        "                    CHUNKLEN  average length of chunks to cut & permute.\n"
        "                    SCATTER   randomisation of cuts (0 to K): default 1.\n"
        "                    where K = total no. of chunks (snd-duration/chunklen).\n"
        "                    If scatter = 0, reorders without shredding.\n"
        "                    NB:  chunklen * scatter MUST be < program's snd buffer.\n"
        "                    NB2: If Input sound > internal buffer len,\n"
        "                    each buffer of sound shredded independently.\n"
        "                    -n flag gives a slightly smoother output\n"
        "3) SCRUB BACK & FORTH: as if handwinding over a tape-head.\n"
        "                    DUR       is minimum length of outfile required.\n"
        "                    DOWN      is lowest downward transposition (semitones).\n"
        "                    UP        is highest upward transposition (semitones).\n"
        "                    START     scrubs starts before time 'start' secs.\n"
        "                    END       scrubs end after time 'end' secs.\n"
        "                    -f        single forwards scrub only (ignores \"DUR\")\n"
        "4) LOSE RESOLUTION: sound converted to lower srate, or bit-resolution.\n"
        "                    BIT_RESOLUTION  range(1 - 16): default 16-bit.\n"
        "                    SRATE_DIVISION  range(1-256): default 1 (normal)\n"
        "                    entered value will be rounded to a power of 2.\n"
        "5) RING MODULATE:   against input modulating frequency, creating sidebands.\n"
        "6) CROSS MODULATE:  Two infiles are multiplied, creating complex sidebands.\n"
        "                    Any combo of mono & stereo files works but,\n"
        "                    files with more channels must have same channel count.\n"
        "7) QUANTISE:        sound converted to specific bit-resolution (mid-rise).\n"
        "                    BIT_RESOLUTION  range(1 - 16): default 16-bit.\n");
    } else if(!strcmp(str,"brassage")) {         /* BRASSAGE = GRANULA */
        fprintf(stdout, /* NB has to use direct fprintf as 2 screenfulls */
        "GRANULAR RECONSTITUTION OF SOUNDFILE\n\n"
        "MODES ARE...\n"
        "1) PITCHSHIFT      4) SCRAMBLE\n"
        "2) TIMESTRETCH     5) GRANULATE\n"
        "3) REVERB          6) BRASSAGE\n"
        "                   7) FULL MONTY\n"
//TW temporary cmdline restriction
        "USAGE:\n"
        "modify brassage 1 infile outfile pitchshift\n"
        "modify brassage 2 infile outfile velocity\n"
        "modify brassage 3 infile outfile density pitch amp [-rrange]\n"
        "modify brassage 4 infile outfile grainsize [-rrange]\n"
        "modify brassage 5 infile outfile density \n\n"
        "modify brassage 6 infile outfile velocity density grainsize pitchshft amp space\n"
        "          bsplice esplice\n"
        "          [-rrange] [-jjitter] [-loutlength] [-cchannel] [-x] [-n]\n\n"
        "modify brassage 7 infile outfile velocity density hvelocity hdensity\n"
        "          grainsize  pitchshift  amp  space  bsplice  esplice\n"
        "          hgrainsize hpitchshift hamp hspace hbsplice hesplice\n"
        "          [-rrange] [-jjitter] [-loutlength] [-cchannel] [-x] [-n]\n"
        "\n"

#ifdef IS_PC
        "MORE??? ----- (hit keyboard)\n");
        while(!kbhit())
            ;
        if(kbhit()) {
#else
        "\n");
#endif

            fprintf(stdout,
            "VELOCITY:  speed of advance in infile, relative to outfile. (>=0)\n"
            "           This is inverse of timestretch, (& permits infinite timestretch).\n"
            "DENSITY:   amount of grain overlap (>0 : <1 leaves intergrain silence)\n"
            "           Extremely small values will cease to perform predictably.\n"
            "GRAINSIZE: grainsize in MS (must be > 2 * splicelen) (Default %.0lf)\n"
            "PITCHSHIFT:is pitchshift in +|- (fractions of) semitones.\n"
            "AMP:       is gain on grains (range 0-1)             (Default 1.0)\n"
            "           use only if you want amp to vary (over a range &/or in time)\n"
            "BSPLICE:   length of startsplices on grains,in MS (Default %.0lf)\n"
            "ESPLICE:   length of endsplices   on grains,in MS (Default %.0lf)\n"
            "SPACE:     set stereo position in outputfile. 0=L,1=R    (Range 0-1).\n"
#ifndef MULTICHAN
            "           Space flag on STEREO input, mixes it to mono before acting.\n"
#else
            "           Set space param to zero, to force multichan output.\n"
            "           Otherwise multichan input mixes to mono\n"
            "           (or selects 1 channel of your choice - see below)\n"
            "           before generating a spatialised stereo output.\n"
#endif
            "RANGE:     of search for nextgrain, before infile 'now'  (Default 0 MS).\n"
            "JITTER:    Randomisation of grain position (Range 0-1) Default (%.2lf).\n"
            "OUTLENGTH: maximum outfile length (if end of data not reached).\n"
            "           Set to zero (Default) for this parameter to be ignored.\n"
            "           BUT if VELOCITY is ANYWHERE 0: OUTLENGTH must be given.\n"
#ifndef MULTICHAN
            "CHANNEL    Extract & work on just 1 channel of stereo snd: Range(1-2).\n"
#else
            "CHANNEL    Extract/work on 1 chan of multichan snd: Range(1-chancnt) OR\n"
            "           (For -ve values) e.g. val -N, spatialise to N-channel space.\n"
#endif
            "           Set to zero (Default) for this parameter to be ignored.\n"
            "-x:        do exponential splices           (Default: linear).\n"
            "-n:        no interpolation for pitch vals, (quick but dirty).\n"
            "\n"
            "HVELOCITY,HDENSITY,HGRAINSIZE,HPITCHSHIFT,HAMP,HBSPLICE,HESPLICE,HSPACE\n"
            "allow a range of values to be specified for any of these params. e.g. With\n"
            "PITCHSHIFT & HPITCHSHIFT set, random pitchshift chosen between these limits.\n"
            "AND NB PITCHSHIFT & HPITCHSHIFT can both vary through time.\n\n"
            "All params, except OUTLENGTH and CHANNEL, can vary through time.\n",
            GRS_DEFAULT_GRAINSIZE,GRS_DEFAULT_SPLICELEN,GRS_DEFAULT_SPLICELEN,GRS_DEFAULT_SCATTER);

#ifdef IS_PC
        }
#endif

     } else if (!strcmp(str,"sausage")) {
        fprintf(stdout, 
        "GRANULAR RECONSTITUTION OF SEVERAL SOUNDFILES SCRAMBLED TOGETHER.\n\n"
//TW temporary cmdline restriction
        "USAGE       (name of outfile must NOT end with a '1')\n"
        "modify sausage infile [infile2 ...] outfile velocity density\n"
        "  hvelocity hdensity grainsize  pitchshift  amp  space  bsplice  esplice\n"
        "  hgrainsize hpitchshift hamp hspace hbsplice hesplice\n"
        "  [-rrange] [-jjitter] [-loutlength] [-cchannel] [-d] [-x] [-n]\n\n"
        "VELOCITY:  speed of advance in infiles, relative to outfile. (>=0)\n"
        "           inverse of timestretch, (permits infinite timestretch).\n"
        "DENSITY:   grain overlap (>0 : <1 leaves intergrain silence)\n"
        "           Extremely small values don't perform predictably.\n"
        "GRAINSIZE: grainsize in MS (must be > 2 * splicelen) (Default %.0lf)\n"
        "PITCHSHIFT:pitchshift in +|- (fractions of) semitones.\n"
        "AMP:       gain on grains (range 0-1) (Default 1.0)\n"
        "           use if amp variation required (over a range &/or in time)\n"
        "BSPLICE:   grain-startsplice length,in MS (Default %.0lf)\n"
        "ESPLICE:   grain-endsplice length,in MS (Default %.0lf)\n"
        "SPACE:     stereo position in outputfile. 0=L,1=R    (Range 0-1).\n"
#ifndef MULTICHAN
        "           Space flag on STEREO input, mixes to mono before acting.\n"
#else
        "           Set all space params to zero, to force multichan output.\n"
        "           Otherwise multichan input mixes to mono\n"
        "           (or selects 1 channel of your choice - see below)\n"
        "           before generating a spatialised stereo output.\n"
#endif
        "RANGE:     of search for nextgrain, before infile 'now'  (Default 0 MS).\n"
        "JITTER:    Randomisation of grain position (Range 0-1) Default (%.2lf).\n"
        "OUTLENGTH: max outfile length (if end of data not reached).\n"
        "           Set to zero (Default) to ignore.\n"
        "           BUT if VELOCITY is ANYWHERE 0: OUTLENGTH must be given.\n"
#ifndef MULTICHAN
        "CHANNEL:   work on just 1 channel of stereo snd: Range(1-2).\n"
#else
        "CHANNEL:   work on just 1 chan of multichan snd: Range(1-chancnt).\n"
        "           or (for -ve vals) e.g val -N, spatialise to N-channel space.\n"
#endif
        "           Set to zero (Default) to ignore.\n"
        "-x:        do exponential splices           (Default: linear).\n"
        "-n:        no interpolation for pitch vals, (quick but dirty).\n"
        "\n"
        "HVELOCITY,HDENSITY,HGRAINSIZE,HPITCHSHIFT,HAMP,HBSPLICE,HESPLICE,HSPACE\n"
        "allow a range of values to be specified for any of these params. e.g. With\n"
        "PITCHSHIFT & HPITCHSHIFT set, random pitchshift chosen between these limits.\n"
        "AND NB PITCHSHIFT & HPITCHSHIFT can both vary through time.\n\n"
        "All params, except OUTLENGTH and CHANNEL, can vary through time.\n",
        GRS_DEFAULT_GRAINSIZE,GRS_DEFAULT_SPLICELEN,GRS_DEFAULT_SPLICELEN,GRS_DEFAULT_SCATTER);
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

/******************************** SINTAB_PCONSISTENCY  ********************************/

int sintab_pconsistency(dataptr dz)
{
    int exit_status;
    double val;
    if(dz->brksize[SIN_FRQ]) {
        if((exit_status = get_maxvalue(SIN_FRQ,&val,dz))<0)
            return exit_status;
    } else
        val = dz->param[SIN_FRQ];
//TW SIN_FRQ parameter is actually the duration of a cycle = 1/frq. already
//  val = 1.0/val;
    val /= 2.0;
    if (val <= dz->param[SIN_QUANT]) {
        sprintf(errstr,"You must reduce the quantisation time to handle the maximum frequency you've specified.\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
    return(FINISHED);
}

/******************************** READ_STACKDATA ********************************/

#define SMALLARRAY 16

int read_stackdata(char *str,dataptr dz)
{
    aplptr ap = dz->application;
    int do_file = 0;
    int arraysize = SMALLARRAY;
    double *p, dummy = 0.0;
    char temp[200], *q;
    int n = 0, m;
    FILE *fp;

    if(!sloom) {                
    /* NEW CMDLINE convention: filenames CAN start with numbers: */
    /* BUT can't BE numbers in their entirety (e.g. 123.456) */
        if(value_is_numeric(str)) {
            if(sscanf(str,"%lf",&dummy)!=1) {
                sprintf(errstr,"Failed to read a numeric value [%s] for transposition\n",str);
                return(DATA_ERROR);
            }
        } else {
//TW Now traps bad pathnames, and names starting with '-', only
            if(file_has_invalid_startchar(str)) {
                sprintf(errstr,"Invalid characters in filename '%s'\n",str);
                return(DATA_ERROR);
            }
            do_file = 1;
        }   
    } else {                    /* TK convention, all numeric values are preceded by NUMERICVAL_MARKER */
        if(str[0]==NUMERICVAL_MARKER) {      
            str++;              
            if(strlen(str)<=0 || sscanf(str,"%lf",&dummy)!=1) {
                sprintf(errstr,"Invalid parameter value (%s) encountered.\n",str);
                return(DATA_ERROR);
            }
        } else 
            do_file = 1;
    }
    if(do_file) {
        if((fp = fopen(str,"r"))==NULL) {
            sprintf(errstr, "Can't open datafile %s to read data.\n",str);
            return(DATA_ERROR);
        }
        if((dz->parray[STACK_TRANS] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for transposition data in file %s.\n",str);
            return(MEMORY_ERROR);
        }
        p = dz->parray[STACK_TRANS];
        while(fgets(temp,200,fp)==temp) {
            q = temp;
            if(*q == ';')   //  Allow comments in file
                continue;
            while(get_float_from_within_string(&q,p)) {
                if(*p < ap->min_special || *p > ap->max_special) {
                    sprintf(errstr,"Transposition value no %ld (%lf) out of range (%lf to %lf)\n",
                        n+1L,*p,ap->min_special,ap->max_special);
                    return(DATA_ERROR);
                }
                *p = semitone_to_ratio(*p);
                p++;
                if(++n >= arraysize) {
                    arraysize += SMALLARRAY;
                    if((dz->parray[STACK_TRANS] = 
                    (double *)realloc((char *)(dz->parray[STACK_TRANS]),arraysize * sizeof(double)))==NULL) {
                        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate transposition data from file %s.\n",str);
                        return(MEMORY_ERROR);
                    }
                    p = dz->parray[STACK_TRANS] + n;        
                }
            }
        }       
        if(n == 0) {
            sprintf(errstr,"No data in file %s\n",str);
            return(DATA_ERROR);
        }
        if((dz->parray[STACK_TRANS] = (double *)realloc((char *)(dz->parray[STACK_TRANS]),n * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to reallocate transposition data from file %s.\n",str);
            return(MEMORY_ERROR);
        }
        if(fclose(fp)<0) {
            fprintf(stdout,"WARNING: Failed to close transposition file %s.\n",str);
            fflush(stdout);
        }
        dz->itemcnt = n;
        p = dz->parray[STACK_TRANS];        /* sort : smallest at foot */
        for(n=0;n<dz->itemcnt-1;n++) {
            for(m=1;m<dz->itemcnt;m++) {
                if(p[m] < p[n]) {
                    dummy = p[n];
                    p[n]  = p[m];
                    p[m]  = dummy;
                }
            }
        }
    } else  {
        if(dummy > ap->max_special || dummy < ap->min_special) {
            sprintf(errstr,"Transposition value (%lf) out of range (%lf to %lf)\n",
            dummy,ap->min_special,ap->max_special);
                return(DATA_ERROR);
        }
        dummy = semitone_to_ratio(dummy);
        if((dz->parray[STACK_TRANS] = (double *)malloc(sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for transposition data\n");
            return(MEMORY_ERROR);
        }
        dz->parray[STACK_TRANS][0] = dummy;
        dz->itemcnt = 0;
    }
    return(FINISHED);
}

/******************************** SEMITONE_TO_RATIO ********************************/

double semitone_to_ratio(double val) {
    val /= 12.0;
    val = pow(2.0,val);
    return val;
}

/******************************** CONVOLVE_PCONSISTENCY ********************************/

int convolve_pconsistency(dataptr dz)
{
    int exit_status;
    double minval;
    switch(dz->mode) {
    case(CONV_NORMAL):
        if(dz->insams[1] > dz->insams[0]) {
            sprintf(errstr,"Convolving file cannot be larger than file to be convolved.\n");
            return(DATA_ERROR);
        }
        break;
    case(CONV_TVAR):
        if(dz->brksize[CONV_TRANS]) {
            if((exit_status = get_minvalue_in_brktable(&minval,CONV_TRANS,dz))<0)
                return(exit_status);
        } else
            minval = dz->param[CONV_TRANS];
        if(((double)dz->insams[1]/minval) > (double)(dz->insams[0])) {
            sprintf(errstr,"Convolving file, at lowest transposition, cannot be larger than file to be convolved.\n");
            return(DATA_ERROR);
        }
        break;
    }
    return(FINISHED);
}

/******************************** CREATE_CONVOLVE_BUFS ********************************/

//TW ADDED AND CONVERTED
int create_convolve_bufs(dataptr dz)
{
    int bigbufsize, seccnt, cfile_len, cfile_size = dz->insams[1] * sizeof(float),total_bufsize;
    double limit;
    int sum;
    int fsecbytesize = F_SECSIZE * sizeof(float);

    if(cfile_size < 0) {
        sprintf(errstr,"Size of convolving file will cause numerical overflow in calculating required memory space.\n");
        return(MEMORY_ERROR);
    }
    seccnt = cfile_size/fsecbytesize;
    if(seccnt * fsecbytesize < cfile_size)
        seccnt++;
    cfile_size = seccnt * fsecbytesize;
    if(cfile_size < 0) {
        sprintf(errstr,"Size of convolving file will cause numerical overflow in calculating required memory space.\n");
        return(MEMORY_ERROR);
    }
    if((bigbufsize = (int)((long)Malloc(-1))) < cfile_size)
        bigbufsize = cfile_size;
    else {
        seccnt = bigbufsize/fsecbytesize;
        if(seccnt * fsecbytesize < bigbufsize)
            seccnt++;
        bigbufsize = seccnt * fsecbytesize;
    }
    if(bigbufsize < 0) {
        sprintf(errstr,"Insufficient memory for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    cfile_size += (dz->infile->channels * sizeof(float));   /* additional space for wrap_around points */
    if(cfile_size < 0) {
        sprintf(errstr,"Size of convolving file will cause numerical overflow in calculating required memory space.\n");
        return(MEMORY_ERROR);
    }
    if((total_bufsize = bigbufsize * 2) < 0) {
        sprintf(errstr,"Insufficient memory for sound buffers.\n");
        return(MEMORY_ERROR);
    }
#if 0
    if((total_bufsize += cfile_size) < total_bufsize) {
        sprintf(errstr,"Size of convolving file will cause numerical overflow in calculating required memory space.\n");
        return(MEMORY_ERROR);
    }
#endif
    dz->buflen = bigbufsize/sizeof(float);
    if((dz->bigbuf = (float *)Malloc(total_bufsize)) <= 0) {
        sprintf(errstr,"Insufficient memory for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    cfile_len = cfile_size/sizeof(float);
    dz->sampbuf[0] = dz->bigbuf;
    dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
    dz->sampbuf[2] = dz->sampbuf[1] + cfile_len;
    memset(dz->bigbuf,0,((dz->buflen * 2) + cfile_len) * sizeof(float));
    sum = dz->buflen + dz->insams[1]; /* total no. of vals convolving buffer of 1st file with all 2nd file */
    if((dz->tempsize = dz->insams[0] + dz->insams[1]) < 0)
        dz->tempsize = INT_MAX;
    limit = floor((double) INT_MAX/(double)sizeof(double));
    if((double)sum > limit) {
        sprintf(errstr,"Size of convolving file will cause numerical overflow in calculating required memory space.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc(sum * sizeof(double)))==NULL) {
        sprintf(errstr,"Insufficent memory for convolve calculations buffer.\n");
        return(MEMORY_ERROR);
    }
    memset((char *)dz->parray[0],0,sum * sizeof(double));
    return(FINISHED);
}

/******************************** READ_LOUDNESS ********************************/

int read_loudness(char *str,dataptr dz)
{
    int istime, exit_status;
    int arraysize = BIGARRAY;
    double *p, lasttime = 0;
    char temp[200], *q;
    int n = 0;
    FILE *fp;

    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr, "Can't open datafile %s to read data.\n",str);
        return(DATA_ERROR);
    }
    if((exit_status = setup_and_init_brktable_constants(dz)) < 0)
        return(exit_status);
    if((dz->brk[LOUD_GAIN] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"Cannot create memory for loudness breaktable\n");
        return(MEMORY_ERROR);
    }
    p = dz->brk[LOUD_GAIN];
    istime = 1;
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,p)) {
            if(istime) {
                if(*p < 0.0) {
                    sprintf(errstr,"INVALID TIME (%lf) IN LOUDNESS ENVELOPE DATA.\n",*p);
                    return(DATA_ERROR);
                }
                if(n == 0) {
                    if(!flteq(*p,0.0)) {
                        sprintf(errstr,"LOUDNESS ENVELOPE MUST BEGIN AT TIME ZERO.\n");
                        return(DATA_ERROR);
                    }
                } else if(*p <= lasttime) {
                    sprintf(errstr,"TIMES DO NOT ADVANCE IN LOUDNESS ENVELOPE (AT TIME %lf)\n",*p);
                    return(DATA_ERROR);
                }
                lasttime = *p;
            } else {
                switch(dz->mode) {
                case(LOUD_PROPOR):
                    if(*p < 0.0) {
                        sprintf(errstr,"INVALID LOUDNESS VALUE (%lf) IN LOUDNESS ENVELOPE DATA.\n",*p);
                        return(DATA_ERROR);
                    }
                    break;
                case(LOUD_DB_PROPOR):
                    *p = dbtogain(*p);
                    break;
                }
            }
            p++;
            if(++n >= arraysize) {
                arraysize += BIGARRAY;
                if((dz->brk[LOUD_GAIN] = (double *)realloc((char *)(dz->brk[LOUD_GAIN]),arraysize * sizeof(double)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate loudness data from file %s.\n",str);
                    return(MEMORY_ERROR);
                }
                p = dz->brk[LOUD_GAIN] + n;     
            }
            istime = !istime;
        }
    }
    if((dz->brk[LOUD_GAIN] = (double *)realloc((char *)(dz->brk[LOUD_GAIN]),n * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate loudness data from file %s.\n",str);
        return(MEMORY_ERROR);
    }
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close loudness file %s.\n",str);
        fflush(stdout);
    }
    if(n == 0) {
        sprintf(errstr,"No data in file %s\n",str);
        return(DATA_ERROR);
    }
    if(!EVEN(n)) {
        sprintf(errstr,"LOUDNESS ENVELOPE DATA INCORRECTLY PAIRED IN FILE %s.\n",str);
        return(DATA_ERROR);
    }
    dz->brksize[0] = n/2;
    return(FINISHED);
}

/******************************** SETUP_AND_INIT_BRKTABLE_CONSTANTS ********************************/

int setup_and_init_brktable_constants(dataptr dz)
{
    if((dz->brk      = (double **)malloc(sizeof(double *)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 1\n");
        return(MEMORY_ERROR);
    }
    if((dz->brkptr   = (double **)malloc(sizeof(double *)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 6\n");
        return(MEMORY_ERROR);
    }
    if((dz->brksize  = (int    *)malloc(sizeof(int)    ))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 2\n");
        return(MEMORY_ERROR);
    }
    if((dz->firstval = (double  *)malloc(sizeof(double)  ))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 3\n");
        return(MEMORY_ERROR);                                                 
    }
    if((dz->lastind  = (double  *)malloc(sizeof(double)  ))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 4\n");
        return(MEMORY_ERROR);
    }
    if((dz->lastval  = (double  *)malloc(sizeof(double)  ))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 5\n");
        return(MEMORY_ERROR);
    }
    if((dz->brkinit  = (int     *)malloc(sizeof(int)     ))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 7\n");
        return(MEMORY_ERROR);
    }
    dz->brk[0]     = NULL;
    dz->brkptr[0]  = NULL;
    dz->brkinit[0] = 0;
    dz->brksize[0] = 0;
    return(FINISHED);
}
