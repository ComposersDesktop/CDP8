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
#include <sndinfo.h>
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

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  interval_to_semitones(char *str,dataptr dz);
static int  readnote_as_midi(char *str,dataptr dz);

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
    int exit_status;
    dz->extra_bufcnt = -1;  /* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
    dz->bptrcnt = 0;
    dz->bufcnt  = 0;
    switch(dz->process) {
    case(INFO_PROPS):           dz->extra_bufcnt = 0;   dz->bufcnt = 0;                         break;
    case(INFO_SFLEN):           dz->extra_bufcnt = 0;   dz->bufcnt = 0;                         break;
    case(INFO_TIMELIST):        dz->extra_bufcnt = 0;   dz->bufcnt = 0;                         break;
    case(INFO_TIMESUM):         dz->extra_bufcnt = 0;   dz->bufcnt = 0;                         break;
    case(INFO_TIMEDIFF):        dz->extra_bufcnt = 0;   dz->bufcnt = 0;                         break;
    case(INFO_SAMPTOTIME):      dz->extra_bufcnt = 0;   dz->bufcnt = 0;                         break;
    case(INFO_TIMETOSAMP):      dz->extra_bufcnt = 0;   dz->bufcnt = 0;                         break;
    case(INFO_MAXSAMP):
    case(INFO_MAXSAMP2):        dz->extra_bufcnt = 0;   dz->bufcnt = 1;                         break;
    case(INFO_LOUDCHAN):        dz->extra_bufcnt = 0;   dz->bufcnt = 1;                         break;
    case(INFO_FINDHOLE):        dz->extra_bufcnt = 0;   dz->bufcnt = 1;                         break;
    case(INFO_DIFF):            dz->extra_bufcnt = 0;   dz->bufcnt = 2; dz->bptrcnt = 2;        break;
    case(INFO_CDIFF):           dz->extra_bufcnt = 0;   dz->bufcnt = 4;                         break;
    case(INFO_PRNTSND):         dz->extra_bufcnt = 0;   dz->bufcnt = 1;                         break;
    case(INFO_MUSUNITS):        dz->extra_bufcnt = 0;   dz->bufcnt = 0;                         break;
    case(INFO_LOUDLIST):        dz->extra_bufcnt = 0;   dz->bufcnt = 1;                         break;
    case(ZCROSS_RATIO):         dz->extra_bufcnt = 0;   dz->bufcnt = 1;                         break;
        
    default:
        sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
        return(PROGRAM_ERROR);
    }

    if(dz->extra_bufcnt < 0) {
        sprintf(errstr,"bufcnts have not been set: establish_bufptrs_and_extra_buffers()\n");
        return(PROGRAM_ERROR);
    }
    if((dz->process==HOUSE_SPEC && dz->mode==HOUSE_CONVERT) || dz->process==INFO_DIFF) {
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
    case(INFO_PROPS):      
        dz->array_cnt=1; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; 
        break;
    case(INFO_SFLEN):       case(INFO_TIMELIST):    case(INFO_TIMESUM):     case(INFO_TIMEDIFF):    
    case(INFO_SAMPTOTIME):  case(INFO_TIMETOSAMP):  case(INFO_MAXSAMP):     case(INFO_LOUDCHAN):    
    case(INFO_FINDHOLE):    case(INFO_DIFF):        case(INFO_CDIFF):       case(INFO_PRNTSND):
    case(INFO_MUSUNITS):    case(INFO_LOUDLIST):    case(INFO_MAXSAMP2):    case(ZCROSS_RATIO):   
        dz->array_cnt=0; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; 
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
    case(INFO_PROPS):           setup_process_logic(ALL_FILES,      OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_SFLEN):           setup_process_logic(ALL_FILES,      OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
//TW UPDATE
    case(INFO_TIMELIST):
        if(sloom)
            setup_process_logic(MANY_SNDFILES,  TO_TEXTFILE,        TEXTFILE_OUT,   dz);
        else
            setup_process_logic(MANY_SNDFILES,  OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    
        break;
    case(INFO_TIMESUM):         setup_process_logic(MANY_SNDFILES,  OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_TIMEDIFF):        setup_process_logic(TWO_SNDFILES,   OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_SAMPTOTIME):      setup_process_logic(SNDFILES_ONLY,  OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_TIMETOSAMP):      setup_process_logic(SNDFILES_ONLY,  OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_MAXSAMP):
    case(INFO_MAXSAMP2):        setup_process_logic(ALL_FILES,      OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_LOUDCHAN):        setup_process_logic(SNDFILES_ONLY,  OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_FINDHOLE):        setup_process_logic(SNDFILES_ONLY,  OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_DIFF):            setup_process_logic(ALL_FILES,      OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_CDIFF):           setup_process_logic(SNDFILES_ONLY,  OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_PRNTSND):         setup_process_logic(SNDFILES_ONLY,  TO_TEXTFILE,        TEXTFILE_OUT,   dz);    break;
    case(INFO_MUSUNITS):        setup_process_logic(NO_FILE_AT_ALL, OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;
    case(INFO_LOUDLIST):        setup_process_logic(MANY_SNDFILES,  TO_TEXTFILE,        TEXTFILE_OUT,   dz);    break;
    case(ZCROSS_RATIO):         setup_process_logic(SNDFILES_ONLY,  OTHER_PROCESS,      NO_OUTPUTFILE,  dz);    break;

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
            if(dz->process==INFO_TIMELIST || dz->process==INFO_LOUDLIST)
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
    case(INFO_DIFF):
    case(INFO_TIMEDIFF):
    case(INFO_TIMELIST):
    case(INFO_LOUDLIST):
        dz->has_otherfile = TRUE;
        break;
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
    case(INFO_PROPS):       case(INFO_SFLEN):       case(INFO_TIMELIST):    case(INFO_TIMESUM):
    case(INFO_TIMEDIFF):    case(INFO_SAMPTOTIME):  case(INFO_TIMETOSAMP):  case(INFO_MAXSAMP):
    case(INFO_LOUDCHAN):    case(INFO_FINDHOLE):    case(INFO_DIFF):        case(INFO_CDIFF):
    case(INFO_PRNTSND):     case(INFO_MUSUNITS):    case(INFO_LOUDLIST):    case(INFO_MAXSAMP2):
    case(ZCROSS_RATIO):
                            exit_status = set_internalparam_data("",ap);        break;
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
    case(NOTE_REPRESENTATION):      return readnote_as_midi(str,dz);
    case(INTERVAL_REPRESENTATION):  return interval_to_semitones(str,dz);
    default:
        sprintf(errstr,"Unknown special_data type: read_special_data()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);   /* NOTREACHED */
}

/********************** INTERVAL_TO_SEMITONES **********************/

int interval_to_semitones(char *str,dataptr dz)
{   
    char *p, *q;
    double d = 0, direction = 1.0;
    p = str + strlen(str) - 1;

    switch(*p) {
    case('u'):    case('U'):    d = .5;  break;
    case('d'):    case('D'):    d = -.5; break;
    }
    q = str;
    if(*q=='-') {
        direction = -1.0;
        q++;
    }
    if     (!strncmp(q,"m2",2))  dz->scalefact = (1.0+d)*direction;  
    else if(!strncmp(q,"2",1))   dz->scalefact = (2.0+d)*direction;  
    else if(!strncmp(q,"m3",2))  dz->scalefact = (3.0+d)*direction;
    else if(!strncmp(q,"3",1))   dz->scalefact = (4.0+d)*direction;
    else if(!strncmp(q,"4",1))   dz->scalefact = (5.0+d)*direction;
    else if(!strncmp(q,"#4",2))  dz->scalefact = (6.0+d)*direction;
    else if(!strncmp(q,"5",1))   dz->scalefact = (7.0+d)*direction;
    else if(!strncmp(q,"m6",2))  dz->scalefact = (8.0+d)*direction;
    else if(!strncmp(q,"6",1))   dz->scalefact = (9.0+d)*direction;
    else if(!strncmp(q,"m7",2))  dz->scalefact = (10.0+d)*direction;
    else if(!strncmp(q,"7",1))   dz->scalefact = (11.0+d)*direction;
    else if(!strncmp(q,"8",1))   dz->scalefact = (12.0+d)*direction;
    else if(!strncmp(q,"m9",2))  dz->scalefact = (13.0+d)*direction;
    else if(!strncmp(q,"9",1))   dz->scalefact = (14.0+d)*direction;
    else if(!strncmp(q,"m10",3)) dz->scalefact = (15.0+d)*direction;
    else if(!strncmp(q,"10",2))  dz->scalefact = (16.0+d)*direction;
    else if(!strncmp(q,"11",2))  dz->scalefact = (17.0+d)*direction;
    else if(!strncmp(q,"#11",3)) dz->scalefact = (18.0+d)*direction;
    else if(!strncmp(q,"12",2))  dz->scalefact = (19.0+d)*direction;
    else if(!strncmp(q,"m13",3)) dz->scalefact = (20.0+d)*direction;
    else if(!strncmp(q,"13",2))  dz->scalefact = (21.0+d)*direction;
    else if(!strncmp(q,"m14",3)) dz->scalefact = (22.0+d)*direction;
    else if(!strncmp(q,"14",2))  dz->scalefact = (23.0+d)*direction;
    else if(!strncmp(q,"15",2))  dz->scalefact = (24.0+d)*direction;
    else {
        sprintf(errstr,"Unknown interval.\n");
        return(GOAL_FAILED);
    } return FINISHED;
}

/************************ READNOTE_AS_MIDI *************************/

int readnote_as_midi(char *str,dataptr dz)
{
    int oct, isquarter = 0;
    int flat = 0, sharp = 0, accidental = 0, quarter = 0;
    char *p = str + strlen(str) - 1;

    if(!isdigit(*p)) {
        if(*p=='u' || *p=='d') {
            sprintf(errstr,"'u' or 'd' come BEFORE 8va-number in note-representation.\n");
            return(GOAL_FAILED);
        } else {
            sprintf(errstr,"Invalid note-representation.\n");
            return(GOAL_FAILED);
        }
    }
    if(*(str+1)=='b') {
        flat = 1;
        accidental = 1;
    } else {
        if(*(str+1)=='#') {
            sharp = 1;
            accidental = 1;
        }
    }
    if(*(str+1+accidental)=='u') {
        quarter = 1;
        isquarter = 1;
    } else {
        if(*(str+1+accidental)=='d') {
            quarter = -1;
            isquarter = 1;
        }
    }
    if(sscanf(str+1+accidental+isquarter,"%d",&oct)!=1) {
        sprintf(errstr,"Note octave not specified.\n");
        return(GOAL_FAILED);
    }
    switch(*str) {
    case('c'): case('C'): dz->scalefact = 0.0; break;
    case('d'): case('D'): dz->scalefact = 2.0; break;
    case('e'): case('E'): dz->scalefact = 4.0; break;
    case('f'): case('F'): dz->scalefact = 5.0; break;
    case('g'): case('G'): dz->scalefact = 7.0; break;
    case('a'): case('A'): dz->scalefact = 9.0; break;
    case('b'): case('B'): dz->scalefact = 11.0; break;
    default:
        sprintf(errstr,"Unknown note '%c'\n",*str);
        return(GOAL_FAILED);
    }
    if(flat)    dz->scalefact -= 1.0;
    if(sharp)   dz->scalefact += 1.0;
    oct     += 5;
    dz->scalefact += ((double)oct     * 12.0);    
    dz->scalefact += ((double)quarter * 0.5);
    return(FINISHED);
}


/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)    
{
    double  temp;

    switch(dz->process) {
    case(INFO_PROPS):       case(INFO_SFLEN):       case(INFO_TIMELIST):
    case(INFO_TIMESUM):     case(INFO_TIMEDIFF):    case(INFO_SAMPTOTIME):
    case(INFO_TIMETOSAMP):  case(INFO_LOUDCHAN):
    case(INFO_FINDHOLE):    case(INFO_DIFF):        case(INFO_CDIFF):
    case(INFO_PRNTSND):     case(INFO_MUSUNITS):    case(INFO_LOUDLIST):    
    case(INFO_MAXSAMP):
        break;
    case(ZCROSS_RATIO):
        if(flteq(dz->param[ZC_START],dz->param[ZC_END])) {
            sprintf(errstr,"Start and Endtime of search are too close.\n");
            return(DATA_ERROR);
        }
        if(dz->param[ZC_START] < 0.0)
            dz->param[ZC_START] = 0.0;
        if(dz->param[ZC_END] > dz->duration)
            dz->param[ZC_END] = dz->duration;
        if(dz->param[ZC_START] > dz->param[ZC_END]) {
            temp = dz->param[ZC_START];
            dz->param[ZC_START] = dz->param[ZC_END];
            dz->param[ZC_END] = temp;
        }
        break;
    case(INFO_MAXSAMP2):
        if(flteq(dz->param[MAX_ETIME],dz->param[MAX_STIME])) {
            sprintf(errstr,"Start and Endtime of search are too close.\n");
            return(DATA_ERROR);
        }
        if(dz->param[MAX_ETIME] < dz->param[MAX_STIME]) {
            temp = dz->param[MAX_ETIME];
            dz->param[MAX_ETIME] = dz->param[MAX_STIME];
            dz->param[MAX_STIME] = temp;
        }
        break;
    default:
        sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN procgrou.c **********************************/
/********************************************************************************************/

/**************************** GROUCHO_PROCESS_FILE ****************************/

int groucho_process_file(dataptr dz)   /* FUNCTIONS FOUND IN PROCESS.C */
{   
    switch(dz->process) {
    case(INFO_PROPS):       case(INFO_SFLEN):       case(INFO_TIMELIST):    case(INFO_TIMESUM): 
    case(INFO_TIMEDIFF):    case(INFO_SAMPTOTIME):  case(INFO_TIMETOSAMP):  case(INFO_MAXSAMP):
    case(INFO_LOUDCHAN):    case(INFO_FINDHOLE):    case(INFO_DIFF):        case(INFO_CDIFF):
    case(INFO_PRNTSND):     case(INFO_LOUDLIST):    case(INFO_MAXSAMP2):    case(ZCROSS_RATIO):
        return do_sndinfo(dz);
    case(INFO_MUSUNITS):
        return do_musunits(dz);
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
    handle_pitch_zeros(dz);
    return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
    int exit_status;
    int orig_chancnt = 0;
    switch(dz->process) {
    case(INFO_PROPS):       case(INFO_TIMESUM):     case(INFO_SAMPTOTIME):      case(INFO_TIMETOSAMP):
    case(INFO_SFLEN):       case(INFO_TIMELIST):    case(INFO_TIMEDIFF):        case(INFO_MUSUNITS):
        break;
    case(INFO_MAXSAMP):     case(INFO_FINDHOLE):    case(INFO_PRNTSND):   case(INFO_MAXSAMP2):
    case(INFO_LOUDCHAN):    case(INFO_LOUDLIST):
    case(ZCROSS_RATIO):
        return create_sndbufs(dz);
    case(INFO_DIFF):        case(INFO_CDIFF):
        if(dz->infile->filetype == ANALFILE) {
            orig_chancnt = dz->infile->channels;
            dz->infile->channels = 1;
        }
        if ((exit_status = create_sndbufs(dz))<0)
            return exit_status;
        if(dz->infile->filetype == ANALFILE)
            dz->infile->channels = orig_chancnt;
        return FINISHED;
    default:
        sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if     (!strcmp(prog_identifier_from_cmdline,"props"))      dz->process = INFO_PROPS;
    else if(!strcmp(prog_identifier_from_cmdline,"len"))        dz->process = INFO_SFLEN;
    else if(!strcmp(prog_identifier_from_cmdline,"lens"))       dz->process = INFO_TIMELIST;
    else if(!strcmp(prog_identifier_from_cmdline,"sumlen"))     dz->process = INFO_TIMESUM;
    else if(!strcmp(prog_identifier_from_cmdline,"timediff"))   dz->process = INFO_TIMEDIFF;
    else if(!strcmp(prog_identifier_from_cmdline,"smptime"))    dz->process = INFO_SAMPTOTIME;
    else if(!strcmp(prog_identifier_from_cmdline,"timesmp"))    dz->process = INFO_TIMETOSAMP;
    else if(!strcmp(prog_identifier_from_cmdline,"maxsamp"))    dz->process = INFO_MAXSAMP;
    else if(!strcmp(prog_identifier_from_cmdline,"maxsamp2"))   dz->process = INFO_MAXSAMP2;
    else if(!strcmp(prog_identifier_from_cmdline,"loudchan"))   dz->process = INFO_LOUDCHAN;
    else if(!strcmp(prog_identifier_from_cmdline,"findhole"))   dz->process = INFO_FINDHOLE;
    else if(!strcmp(prog_identifier_from_cmdline,"diff"))       dz->process = INFO_DIFF;
    else if(!strcmp(prog_identifier_from_cmdline,"chandiff"))   dz->process = INFO_CDIFF;
    else if(!strcmp(prog_identifier_from_cmdline,"prntsnd"))    dz->process = INFO_PRNTSND;
    else if(!strcmp(prog_identifier_from_cmdline,"units"))      dz->process = INFO_MUSUNITS;
    else if(!strcmp(prog_identifier_from_cmdline,"maxi"))       dz->process = INFO_LOUDLIST;
    else if(!strcmp(prog_identifier_from_cmdline,"zcross"))     dz->process = ZCROSS_RATIO;
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
    "USAGE: sndreport NAME (mode) infile(s) (outfile) (parameters)\n"
    "\n"
    "where NAME can be any one of\n"
    "\n"
    "props     len        lens      sumlen      timediff\n"
    "smptime   timesmp    maxsamp   maxsamp2    loudchan    findhole\n"
    "diff      chandiff   prntsnd   units       maxi        zcross\n"
    "\n"
    "Type 'sndreport timediff'  for more info on info timediff option... ETC.\n");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"props")) {
        sprintf(errstr,
        "DISPLAY PROPERTIES OF A SNDFILING-SYSTEM FILE\n\n"
        "USAGE: sndreport props infile\n");
    } else if(!strcmp(str,"len")) {
        sprintf(errstr,
        "DISPLAY DURATION OF A SNDFILING-SYSTEM FILE\n\n"
        "USAGE: sndreport len infile\n");
    } else if(!strcmp(str,"lens")) {
        sprintf(errstr,
        "LIST DURATIONS OF SEVERAL SNDFILING-SYSTEM FILES\n\n"
        "USAGE: sndreport lens infile [infile2..]\n");
    } else if(!strcmp(str,"maxi")) {
        sprintf(errstr,
        "LIST LEVELS OF SEVERAL SOUNDFILES\n\n"
//TW UPDATE
        "USAGE: sndreport maxi infile infile2 [infile3..] outfile\n");
    } else if(!strcmp(str,"sumlen")) {
        sprintf(errstr,
        "SUM DURATIONS OF SEVERAL SNDFILING-SYSTEM FILES\n\n"
        "USAGE: sndreport sumlen infile infile2 [infile3..] [-ssplicelen]\n\n"
        "      SPLICELEN is in milliseconds. (Default: 15ms)\n");
    } else if(!strcmp(str,"timediff")) {
        sprintf(errstr,
        "FIND DIFFERENCE IN DURATION OF TWO SOUND FILES\n\n"
        "USAGE: sndreport timediff infile1 infile2\n");
    } else if(!strcmp(str,"smptime")) {
        sprintf(errstr,
        "CONVERT SAMPLE COUNT TO TIME IN SOUNDFILE\n\n"
        "USAGE: sndreport smptime infile samplecnt [-g]\n\n"
        "-g   sample count is count of GROUPED samples\n"
        "     e.g. stereo file: sample-PAIRS counted.\n");
    } else if(!strcmp(str,"timesmp")) {
        sprintf(errstr,
        "CONVERT TIME TO SAMPLE COUNT IN SOUNDFILE\n\n"
        "USAGE: sndreport timesmp infile time [-g]\n\n"
        "-g   sample count is count of GROUPED samples\n"
        "     e.g. stereo file: sample-PAIRS counted.\n");
    } else if(!strcmp(str,"maxsamp")) {
        sprintf(errstr,
        "FIND MAXIMUM SAMPLE IN SOUNDFILE OR BINARY DATA FILE\n\n"
        "USAGE: sndreport maxsamp infile [-f]\n"
        "-f   Force file to be scanned\n"
        "     (Ignore any header info about max sample)\n");
    } else if(!strcmp(str,"maxsamp2")) {
        sprintf(errstr,
        "FIND MAXIMUM SAMPLE WITHIN TIMERANGE IN SOUNDFILE\n\n"
        "USAGE: sndreport maxsamp2 infile start end\n\n"
        "start  starttime of search in file\n"
        "end    endtime of search in file\n");
    } else if(!strcmp(str,"loudchan")) {
        sprintf(errstr,
        "FIND LOUDEST CHANNEL IN A STEREO SOUNDFILE\n\n"
        "USAGE: sndreport loudchan infile\n");
    } else if(!strcmp(str,"findhole")) {
        sprintf(errstr,
        "FIND LARGEST LOW LEVEL HOLE IN A SOUNDFILE\n\n"
        "USAGE: sndreport findhole infile [-tthreshold]\n\n"
        "THRESHOLD  hole only if level falls and stays below threshold (default: 0).\n");
    } else if(!strcmp(str,"diff")) {
        sprintf(errstr,
        "COMPARE 2 SOUND,ANALYSIS,PITCH,TRANSPOSITION,ENVELOPE OR FORMANT FILES\n\n"
        "USAGE: sndreport diff infile1 infile2 [-tthreshold] [-ncnt] [-l] [-c]\n\n"
        "THRESHOLD  max permissible difference in data values:\n"
        "CNT        MAX NUMBER of differences to accept (default 1).\n"
        "-l         continue, even if files are not same LENGTH\n"
        "-c         continue, even if (snd)files don't have same no of CHANNELS\n\n"
        "This process works with binary (non-text) files only.\n");
    } else if(!strcmp(str,"chandiff")) {
        sprintf(errstr,
        "COMPARE CHANNELS IN A STEREO SOUNDFILE\n\n"
        "USAGE: sndreport chandiff infile [-tthreshold] [-ncnt]\n\n"
        "THRESHOLD  max permissible difference in data values.\n"
        "CNT        MAX NUMBER of differences to accept (default 1).\n"
        "   NB: The output sample display is counted in sample-pairs.\n");
    } else if(!strcmp(str,"prntsnd")) {
        sprintf(errstr,
        "PRINT SOUND SAMPLE DATA TO A TEXTFILE\n\n"
        "USAGE: sndreport prntsnd infile outtextfile starttime endtime\n\n"
        "CARE!!! large quantities of data.\n");
    } else if(!strcmp(str,"units")) {
        fprintf(stdout,
        "CONVERT BETWEEN DIFFERENT UNITS\n\n"
        "USAGE: sndreport units mode value\n\n"
        "                           MODES ARE\n\n"
        "       PITCH                  INTERVAL                       SPEED\n"
        "       -----                  --------                       -----\n"
        "(1) MIDI to FRQ   (7)  FRQ RATIO  to SEMITONES (16) FRQ RATIO  to TIME RATIO\n"
        "(2) FRQ  to MIDI  (8)  FRQ RATIO  to INTERVAL  (17) SEMITONES  to TIME RATIO\n"
        "(3) NOTE to FRQ   (9)  INTERVAL   to FRQ RATIO (18) OCTAVES    to TIME RATIO\n"
        "(4) NOTE to MIDI  (10) SEMITONES to FRQ RATIO  (19) INTERVAL   to TIME RATIO\n"
        "(5) FRQ to NOTE   (11) OCTAVES    to FRQ RATIO (20) TIME RATIO to FRQ RATIO\n"
        "(6) MIDI to NOTE  (12) OCTAVES   to SEMITONES  (21) TIME RATIO to SEMITONES\n"
        "                  (13) FRQ RATIO to OCTAVES    (22) TIME RATIO to OCTAVES\n"
        "                  (14) SEMITONES to OCTAVES    (23) TIME RATIO to INTERVAL\n"
        "                  (15) SEMITONES to INTERVAL\n"
        "\n"
        "                                LOUDNESS\n"
        "                                --------\n"
        "                  (24) GAIN FACTOR to DB GAIN\n"
        "                  (25) DB GAIN     to GAIN FACTOR\n"
        "\n"
        "NOTE REPRESENTATION ..... A1 = A in octave 1\n"
        "                          Ebu4 is E flat,   + (Up) quartertone  in octave 4\n"
        "                          F#d-2 is F sharp, - (Dn) quartertone, in octave -2\n\n"
        "INTERVAL REPRESENTATION.. 3 = a 3rd     -m3 = minor 3rd DOWN\n"
        "                          m3u = minor 3rd + (Up) quartertone\n"
        "                          #4d = tritone   - (Dn) quartertone\n"
        "                          15  = a fifteenth (max permissible interval)\n");
    } else if(!strcmp(str,"zcross")) {
        sprintf(errstr,
        "DISPLAY FRACTION OF ZERO-CROSSINGS IN FILE\n\n"
        "USAGE: sndreport ZCROSS infile [-sstarttime -eendtime]\n");
    } else
        sprintf(errstr,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
    if(!strcmp(str1,"props"))           return(CONTINUE);
    else if(!strcmp(str1,"len"))        return(CONTINUE);
    else if(!strcmp(str1,"timediff"))   return(CONTINUE);
    else if(!strcmp(str1,"smptime"))    return(CONTINUE);
    else if(!strcmp(str1,"timesmp"))    return(CONTINUE);
    else if(!strcmp(str1,"loudchan"))   return(CONTINUE);
    else if(!strcmp(str1,"maxsamp"))    return(CONTINUE);
    else if(!strcmp(str1,"maxsamp2"))   return(CONTINUE);
    else if(!strcmp(str1,"findhole"))   return(CONTINUE);
    else if(!strcmp(str1,"chandiff"))   return(CONTINUE);
    else
        sprintf(errstr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}


/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
    return(FINISHED);
}
