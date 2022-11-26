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



/* floatsam version*/
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <cdpmain.h>
#include <filetype.h>
#include <globcon.h>
#include <formants.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <special.h>
#include <speccon.h>

#include <sfsys.h>
#ifdef _DEBUG
#include <assert.h>
#endif

#include <string.h>             /*RWD */

#if defined unix || defined _MSC_VER
#define round(x) lround((x))
#endif

static int  get_pitch_or_transpos_binarydata_from_first_file(float **thisdata,dataptr dz);
static int  read_pitch_or_transposition_brkvals(char *filename,double **brktable,int *brksize);
static int  store_numbers(char *filename,dataptr dz);
static int  store_wordlist(char *filename,dataptr dz);
static int  store_brktable(char *filename,dataptr dz);
static int  open_checktype_getsize_and_compareheader(char *filename,int fileno,fileptr *fp2,dataptr dz);
//TW NEW
//static int  headcompare_and_update(int fd,char *filename,int *this_filetype,fileptr *fp2,dataptr dz);
static int  headcompare_and_update(int fd,char *filename,int *this_filetype,fileptr *fp2,int fileno,dataptr dz);

static int  set_otherfile_header(int fd,char *filename,dataptr dz);
//TW NEW
//static int  check_later_filetype_valid_for_process(char *filename,int file_type,dataptr dz);
static int  check_later_filetype_valid_for_process(char *filename,int file_type,int fileno,dataptr dz);

static int  check_for_data_in_later_file(char *filename,int filetype,int fileno,dataptr dz);
static int  read_brkvals_from_other_file(FILE *fp,char *filename,dataptr dz);
static int  read_dB_brkvals_from_other_file(FILE *fp,char *filename,dataptr dz);
static int  setup_valid_wlength(dataptr dz);
static int  get_pitch_or_transpos_binarydata_from_other_file(int max_arraysize,int fileno,
                                                             float **thisdata,fileptr fp2,dataptr dz);
static int  getsize_and_get_brkpntdata_from_other_pitch_or_transpos_file
(char *filename,double minval, double maxval,int which_type, dataptr dz);
static int  establish_minwindowlength_or_minduration_in_secs(double *brktable,int brksize,char *filename,dataptr dz);
static int  establish_duration_in_secs(double *duration,double *brktable,int brksize,char *filename);
#ifdef NOTDEF
static int  check_for_pitch_zeros(char *filename,float *pitcharray,int wlen);
#endif
static int  test_brkvals(double minval,double maxval,float *floatarray,int arraysize);
static int  inform_infile_structure(fileptr fp1,fileptr fp2,dataptr dz);

/************************************************************************************/
/************************************************************************************/
/*        READING THE FIRST FILE : WHERE DATA IS KNOWN FROM PARSING PROCESS         */
/************************************************************************************/
/************************************************************************************/

/************************** OPEN_FIRST_INFILE *****************************/

int open_first_infile(char *filename,dataptr dz)
{
    int exit_status;
    int arraycnt;
    switch(dz->infile->filetype) {
        case(ANYFILE):
            break;
        case(SNDFILE):      case(ANALFILE):     case(PITCHFILE):
        case(TRANSPOSFILE): case(FORMANTFILE):  case(ENVFILE):
            if((dz->ifd[0] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
                sprintf(errstr,"Failure to open file %s for input.\n",filename);
                return(SYSTEM_ERROR);
            }
            
#ifdef _DEBUG
            assert(dz->insams[0] >0);
            
#endif
            /* RWD Feb 2007 workaround for close/reopen problem */
#ifdef _WIN32
            sndseekEx(dz->ifd[0],0,0);
#endif
            switch(dz->infile->filetype) {
                case(PITCHFILE):
                    if((dz->pitches = (float *)malloc((size_t) (dz->insams[0] * sizeof(float))))==NULL) {
                        sprintf(errstr,"INSUFFICIENT MEMORY to store binary pitch data.\n");
                        return(MEMORY_ERROR);
                    }
                    if((exit_status = get_pitch_or_transpos_binarydata_from_first_file(&(dz->pitches),dz))<0)
                        return(exit_status);
                    break;
                case(TRANSPOSFILE):
                    if((dz->transpos = (float *)malloc((size_t)(dz->insams[0] * sizeof(float))))==NULL) {
                        sprintf(errstr,"INSUFFICIENT MEMORY to store binary transposition data.\n");
                        return(MEMORY_ERROR);
                    }
                    if((exit_status = get_pitch_or_transpos_binarydata_from_first_file(&(dz->transpos),dz))<0)
                        return(exit_status);
                    break;
                case(FORMANTFILE):
                    if((exit_status = initialise_specenv(&arraycnt,dz))<0)
                        return(exit_status);
                    break;
            }
            
            break;
        case(TEXTFILE):
            if((exit_status = read_pitch_or_transposition_brkvals(filename,&(dz->temp),&(dz->tempsize)))<0)
                return(exit_status);
            break;
        case(BRKFILE):
        case(DB_BRKFILE):
        case(UNRANGED_BRKFILE):
            if((exit_status = store_brktable(filename,dz))<0)
                return(exit_status);
            if(dz->input_data_type!=ALL_FILES && dz->input_data_type!=ANY_NUMBER_OF_ANY_FILES)
                dz->infilecnt = 0;
            break;
        case(NUMLIST):
            if((exit_status = store_numbers(filename,dz))<0)
                return(exit_status);
            dz->infilecnt = 0;
            break;
        case(SNDLIST):
        case(SYNCLIST):
        case(MIXFILE):
        case(LINELIST):
        case(WORDLIST):
            if((exit_status = store_wordlist(filename,dz))<0)
                return(exit_status);
            if(dz->input_data_type!=ALL_FILES && dz->input_data_type!=ANY_NUMBER_OF_ANY_FILES)
                dz->infilecnt = 0;
            break;
        default:
            sprintf(errstr,"Unknown filetype: open_first_infile()\n");
            return(PROGRAM_ERROR);
    }
    if(dz->infilecnt>0 && dz->infile->filetype!=TEXTFILE)
        dz->samps_left = dz->insams[0];
    return(FINISHED);
}

/********************* GET_PITCH_OR_TRANSPOS_BINARYDATA_FROM_FIRST_FILE *************************/

int get_pitch_or_transpos_binarydata_from_first_file(float **thisdata,dataptr dz)
{
    int /*seccnt,*/ samps_read;
    int samps_to_read = dz->insams[0];
    unsigned int arraysize;
    
    if((*thisdata = (float *)malloc((size_t)(samps_to_read * sizeof(float))))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store binary pitch or transposition data.\n");
        return(MEMORY_ERROR);
    }
    if((samps_read  = fgetfbufEx((*thisdata), samps_to_read,dz->ifd[0],0))<0) {
        sprintf(errstr,"Sound read error.\n");
        return(SYSTEM_ERROR);
    }
    if(samps_read!= dz->insams[0]) {
        sprintf(errstr,"Problem reading data: get_pitch_or_transpos_binarydata_from_first_file()\n");
        return(PROGRAM_ERROR);
    }
    arraysize = samps_read;
    if((*thisdata = (float *)realloc((char *)(*thisdata),(arraysize + 1) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate binary pitch or transposition data.\n");   /* 1 float for safety */
        return(MEMORY_ERROR);
    }
    (*thisdata)[arraysize] = 0.0f;  /* SAFETY POSITION */
    return(FINISHED);
}

/************************* READ_PITCH_OR_TRANSPOSITION_BRKVALS *****************************/

int read_pitch_or_transposition_brkvals(char *filename,double **brktable,int *brksize)
{
    int arraysize = (*brksize) * 2;
    double *p, d;
    int n = 0,m, final_size;
    char temp[200], *q;
    FILE *fp;
    
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr, "Can't open file %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    
#ifdef IS_CDPMAIN_TESTING
    if(*brksize <= 0) {
        sprintf(errstr,"Array size not set: read_pitch_or_transposition_brkvals()\n");
        return(PROGRAM_ERROR);
    }
#endif
    
    if((*brktable = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store pitch or transposition brktable data.\n");
        return(MEMORY_ERROR);
    }
    p = *brktable;
    while(fgets(temp,200,fp)==temp) {    /* READ AND TEST BRKPNT VALS */
        q = temp;
        while(get_float_from_within_string(&q,&d)) {
            
#ifdef IS_CDPMAIN_TESTING
            if(n >= arraysize) {
                sprintf(errstr,"More values found than indicated by parse.\n");
                return(PROGRAM_ERROR);
            }
#endif
            
            *p = d;
            p++;
            n++;
        }
    }
    
#ifdef IS_CDPMAIN_TESTING
    if(n!=arraysize) {
        sprintf(errstr,"Count of pitch or transposition values does not tally with parse.\n");
        return(PROGRAM_ERROR);
    }
#endif
    
    final_size = n;
    if((*brktable)[0] != 0.0)   /* Allow space for a value at time zero, if there isn't one */
        final_size = n + 2;
    if((*brktable = (double *)realloc((char *)(*brktable),final_size * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate pitch or transposition brktable data.\n");
        return(MEMORY_ERROR);
    }
    if(final_size != n) {       /* Force a value at time zero, if there isn't one */
        for(m=n-1;m>=0;m--)
            (*brktable)[m+2] = (*brktable)[m];
        (*brktable)[0] = 0.0;
        (*brktable)[1] = (*brktable)[3];
    }
    *brksize = final_size/2;
    return(FINISHED);
}

/*************************** STORE_NUMBERS ***********************/

int store_numbers(char *filename,dataptr dz)
{
    double *p, d;
    char   temp[200], *q;
    int   n = 0;
    FILE   *fp;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Failed to open file %s for input.\n",filename);
        return(DATA_ERROR);
    }
    
#ifdef IS_CDPMAIN_TESTING
    if(dz->numsize<=0) {
        sprintf(errstr,"Accounting problem in numfile transfer: store_numbers()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->array_cnt < 1) {
        sprintf(errstr,"No double array available for number list: get_numtable_from_framework()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->parray[0] != NULL) {
        sprintf(errstr,"Double array for number list already allocated: get_numtable_from_framework()\n");
        return(PROGRAM_ERROR);
    }
#endif
    
    if((dz->parray[0] = (double *) malloc(dz->numsize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to allocate numbers array.\n");
        return(MEMORY_ERROR);
    }
    p = dz->parray[0];
    while(fgets(temp,200,fp)!=NULL) {
        q = temp;
        while(get_float_from_within_string(&q,&d)) {
            
#ifdef IS_CDPMAIN_TESTING
            if(n >= dz->numsize) {
                sprintf(errstr,"Number of input numbers exceeds parsed size: file %s\n",filename);
                return(PROGRAM_ERROR);
            }
#endif
            
            *p = d;
            p++;
            n++;
        }
    }
    if(n != dz->numsize) {
        sprintf(errstr,"Number of input numbers != parsed size: file %s\n",filename);
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/************************* STORE_WORDLIST *****************************/

int store_wordlist(char *filename,dataptr dz)
{
    char temp[200],*p,*q;
    int n;
    int total_wordcnt = 0;
    int wordcnt_in_line;
    int line_cnt = 0;
    FILE *fp;
    
#ifdef IS_CDPMAIN_TESTING
    if(dz->linecnt <= 0) {
        sprintf(errstr,"Accounting problem: get_wordlist()\n");
        return(PROGRAM_ERROR);
    }
#endif
    
    if((dz->wordstor = (char **)malloc(dz->all_words * sizeof(char *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for wordstores.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->all_words;n++)        /* initialise, for testing and safe freeing */
        dz->wordstor[n] = NULL;
    
    if((dz->wordcnt  = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for line wordcounts.\n");
        return(MEMORY_ERROR);
    }
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Failed to open file %s for input.\n",filename);
        return(DATA_ERROR);
    }
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        if(is_an_empty_line_or_a_comment(p))
            continue;
        
#ifdef IS_CDPMAIN_TESTING
        if(line_cnt >= dz->linecnt) {
            sprintf(errstr,"Too many lines found: does not tally with parsing data.\n");
            return(PROGRAM_ERROR);
        }
#endif
        
        wordcnt_in_line = 0;
        while(get_word_from_string(&p,&q)) {
            
#ifdef IS_CDPMAIN_TESTING
            if(total_wordcnt >= dz->all_words) {
                sprintf(errstr,"Error in word accounting: store_wordlist_without_comments()\n");
                return(PROGRAM_ERROR);
            }
#endif
            
            if((dz->wordstor[total_wordcnt] = (char *)malloc((strlen(q) + 1) * sizeof(char)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY for wordstor %d\n",total_wordcnt+1);
                return(MEMORY_ERROR);
            }
            strcpy(dz->wordstor[total_wordcnt],q);
            total_wordcnt++;
            wordcnt_in_line++;
        }
        dz->wordcnt[line_cnt] = wordcnt_in_line;
        line_cnt++;
    }
    /* NEW : APRIL 2009 !! */
    dz->linecnt = line_cnt;
    
#ifdef IS_CDPMAIN_TESTING
    if(dz->linecnt != line_cnt) {
        sprintf(errstr,"Count of lines does not tally with parsing data.\n");
        return(PROGRAM_ERROR);
    }
    if(dz->all_words != total_wordcnt) {
        sprintf(errstr,"Count of words does not tally with parsing data.\n");
        return(PROGRAM_ERROR);
    }
#endif
    
    if(dz->infile->filetype==SNDLIST) {     /* reorganise snds in sndlist onto separate lines */
        if(dz->linecnt != dz->all_words) {
            if((dz->wordcnt  = (int *)realloc(dz->wordcnt,dz->all_words * sizeof(int)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate word counts.\n");
                return(MEMORY_ERROR);
            }
            dz->linecnt = dz->all_words;
            for(n=0;n<dz->linecnt;n++)
                dz->wordcnt[n] = 1;
        }
    }
    //TW NOT SURE WHETHER YOU REMOVED fclose(fp) FOR A PURPOSE from the code set you have
    //OR I ADDED IT TO THE CODESET MORE RECENTLY, due to its omission throwing up a problem ???
    fclose(fp);
    return(FINISHED);
}

/*************************** STORE_BRKTABLE ***********************/

int store_brktable(char *filename,dataptr dz)
{
    double *p, d, *brktable;
    int arraysize;
    char temp[200], *q;
    int n = 0,m, final_size;
    FILE *fp;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Failed to open file %s for input.\n",filename);
        return(DATA_ERROR);
    }
    
#ifdef IS_CDPMAIN_TESTING
    if(dz->extrabrkno < 0) {
        sprintf(errstr,"invalid index for extrabrkno: get_brktable()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->brksize[dz->extrabrkno] <= 0) {
        sprintf(errstr,"invalid size for brktable: get_brktable()\n");
        return(PROGRAM_ERROR);
    }
#endif
    
    arraysize = dz->brksize[dz->extrabrkno] * 2;
    if((dz->brk[dz->extrabrkno] = (double*) malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store input brktable data.\n");
        return(MEMORY_ERROR);
    }
    brktable  = dz->brk[dz->extrabrkno];
    
    p = brktable;
    while(fgets(temp,200,fp)!=NULL) {
        q = temp;
        while(get_float_from_within_string(&q,&d)) {
            
#ifdef IS_CDPMAIN_TESTING
            if(n >= arraysize) {
                sprintf(errstr,"Too much data to put in allocated array: Accounting error.\n");
                return(PROGRAM_ERROR);
            }
#endif
            
            *p = d;
            p++;
            n++;
        }
    }
    
#ifdef IS_CDPMAIN_TESTING
    if(n != arraysize) {
        sprintf(errstr,"Count of brkfile data does not tally with parse: file %s\n",filename);
        return(PROGRAM_ERROR);
    }
#endif
    
    if(brktable[0] != 0.0)  { /* Allow space for a value at time zero, if there isn't one */
        final_size = n + 2;
        if((dz->brk[dz->extrabrkno] = (double *)realloc
            ((char *)dz->brk[dz->extrabrkno],final_size * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to reallocate input brktable data.\n");
            return(MEMORY_ERROR);
        }
        brktable  = dz->brk[dz->extrabrkno];
        for(m=n-1;m>=0;m--)
            brktable[m+2] = brktable[m];
        brktable[0] = 0.0;
        brktable[1] = brktable[3];
        dz->brksize[dz->extrabrkno]++;
    }
    return(FINISHED);
}

/************************************************************************************/
/************************************************************************************/
/*      READING SUBSEQUENT FILES : WHERE DATA NOT KNOWN FROM PARSING PROCESS        */
/************************************************************************************/
/************************************************************************************/


/***************************** HANDLE_OTHER_INFILE **************************/

int handle_other_infile(int fileno,char *filename,dataptr dz)   /* fileno = 1,2,3.*/
{
    int     exit_status, current_status;
    int     wlengthf, arraysize;
    double  maxval;
    int     arraycnt;
    fileptr fp2 = NULL;
#ifdef IS_CDPMAIN_TESTING
    if(fileno < 1 || fileno>dz->infilecnt) {
        sprintf(errstr,"Attempted to open infile not accounted for: handle_other_infile()\n");
        return(PROGRAM_ERROR);
    }
#endif
    
    if((exit_status = open_checktype_getsize_and_compareheader(filename,fileno,&fp2,dz))<0)
        return(exit_status);
    current_status = exit_status;
    
    switch(dz->input_data_type) {
        case(TWO_SNDFILES):
        case(MANY_SNDFILES):
        case(ONE_OR_MANY_SNDFILES):
            //TW NEW
        case(ONE_OR_MORE_SNDSYS):
            
        case(SNDFILE_AND_BRKFILE):
        case(SNDFILE_AND_UNRANGED_BRKFILE):
        case(SNDFILE_AND_DB_BRKFILE):
        case(SNDFILE_AND_ENVFILE):
        case(TWO_ANALFILES):
        case(THREE_ANALFILES):
        case(MANY_ANALFILES):
            
#ifdef IS_CDPMAIN_TESTING
            if(current_status!=FINISHED) {
                sprintf(errstr,"%s is not a valid data file for this application.\n",filename);
                return(USER_ERROR);
            }
#endif
            
            break;
            
        case(ANAL_AND_FORMANTS):    /* formant data is read in param_preprocess, once buffers setup */
        case(PITCH_AND_FORMANTS):   /* formant data is read in param_preprocess, once buffers setup */
            
#ifdef IS_CDPMAIN_TESTING
            if(current_status!=FINISHED) {
                sprintf(errstr,"%s is not a valid data file for this application.\n",filename);
                return(DATA_ERROR);
            }
#endif
            /* set processing length to min of input files */
            if((wlengthf = (dz->insams[fileno]/dz->infile->specenvcnt) - DESCRIPTOR_DATA_BLOKS)<=0) {
                sprintf(errstr,"No data in formant file %s\n",filename);
                return(DATA_ERROR);
            }
            dz->wlength = min(dz->wlength,wlengthf);
            dz->descriptor_samps = dz->infile->specenvcnt * DESCRIPTOR_DATA_BLOKS ;
            if((exit_status = initialise_specenv(&arraycnt,dz))<0)
                return(exit_status);
            break;
            //TW NEW CASE
        case(PFE):                  /* formant data is read in param_preprocess, once buffers setup */
            if(fileno==1) {
                /* set processing length to min of input files */
                if((wlengthf = (dz->insams[fileno]/dz->infile->specenvcnt) - DESCRIPTOR_DATA_BLOKS)<=0) {
                    sprintf(errstr,"No data in formant file %s\n",filename);
                    return(DATA_ERROR);
                }
                dz->wlength = min(dz->wlength,wlengthf);
                dz->descriptor_samps = dz->infile->specenvcnt * DESCRIPTOR_DATA_BLOKS;
                if((exit_status = initialise_specenv(&arraycnt,dz))<0)
                    return(exit_status);
            }
            break;
            
        case(ANAL_WITH_PITCHDATA):
            switch(current_status) {
                case(FINISHED):
                    /* pitchdata derived from analdata: hence lengths must be equal */
                    if((int)(dz->insams[fileno]) != dz->wlength) {
                        sprintf(errstr,"Pitchfile (%d vals) and analysisfile (%d windows) are not same length\n",dz->insams[fileno],dz->wlength);
                        return(DATA_ERROR);
                    }
                    arraysize = dz->wlength;
                    if((exit_status = get_pitch_or_transpos_binarydata_from_other_file(arraysize,fileno,&(dz->pitches),fp2,dz))<0)
                        return(exit_status);
                    break;
                case(CONTINUE):
                    if((exit_status = getsize_and_get_brkpntdata_from_other_pitch_or_transpos_file
                        (filename,-2.0,dz->nyquist+1.0,NO_SPECIAL_TYPE,dz))<0)
                        return(exit_status);
                    break;
            }
            break;
            
        case(ANAL_WITH_TRANSPOS):
            
#ifdef IS_CDPMAIN_TESTING
            if(current_status!=FINISHED) {
                sprintf(errstr,"%s is not a valid data file for this application.\n",filename);
                return(USER_ERROR);
            }
#endif
            
            arraysize = max(dz->wlength,dz->insams[1]);
            /* transposdata will be adjusted to size of analdata */
            if((exit_status = get_pitch_or_transpos_binarydata_from_other_file(arraysize,fileno,&(dz->transpos),fp2,dz))<0)
                return(exit_status);
            break;
            
        case(PITCH_AND_PITCH):
            switch(current_status) {
                case(FINISHED):             /* THIS file is a binary file, so binary data header available */
                    if((exit_status = setup_valid_wlength(dz))<0)
                        return(exit_status);
                    arraysize  = max(dz->wlength,dz->insams[1]);
                    if((exit_status = get_pitch_or_transpos_binarydata_from_other_file(arraysize,fileno,&(dz->pitches2),fp2,dz))<0)
                        return(exit_status);
                    break;
                case(CONTINUE):                             /* Implies input file is NOT a binary file */
                    if(dz->infile->filetype == TEXTFILE) {  /* Implies NEITHER file is a binary file */
                        if(dz->outfiletype  == PITCH_OUT) {
                            sprintf(errstr,"Can't generate binary output data from exclusively brkpntfile input.\n");
                            return(DATA_ERROR);
                        }
                        maxval = DEFAULT_NYQUIST;   /* nyquist is NOT defined */
                    } else
                        maxval = dz->nyquist;   /* nyquist is defined by 1st input file */
                    if((exit_status = getsize_and_get_brkpntdata_from_other_pitch_or_transpos_file
                        (filename,-1.0,maxval,NO_SPECIAL_TYPE,dz))<0)
                        return(exit_status);
                    break;
            }
            /****
             if((exit_status = check_for_pitch_zeros(filename,dz->pitches2,dz->wlength))<0)
             return(exit_status);
             ****/
            break;
            
        case(PITCH_AND_TRANSPOS):
        case(TRANSPOS_AND_TRANSPOS):
            switch(current_status) {
                case(FINISHED):             /* THIS file is a binary file, so binary data header available */
                    if((exit_status = setup_valid_wlength(dz))<0)
                        return(exit_status);
                    arraysize  = max(dz->wlength,dz->insams[1]);
                    switch(dz->input_data_type) {
                        case(PITCH_AND_TRANSPOS):
                            if((exit_status = get_pitch_or_transpos_binarydata_from_other_file
                                (arraysize,fileno,&(dz->transpos),fp2,dz))<0)
                                return(exit_status);
                            break;
                        case(TRANSPOS_AND_TRANSPOS):
                            if((exit_status = get_pitch_or_transpos_binarydata_from_other_file
                                (arraysize,fileno,&(dz->transpos2),fp2,dz))<0)
                                return(exit_status);
                            break;
                    }
                    break;
                case(CONTINUE):                             /* Implies input file is NOT a binary file */
                    if(dz->infile->filetype == TEXTFILE) {  /* Implies NEITHER file is a binary file */
                        if(dz->outfiletype  == PITCH_OUT) {
                            sprintf(errstr,"Can't generate binary output data from exclusively brkpntfile input.\n");
                            return(DATA_ERROR);
                        }
                    }
                    if((exit_status = getsize_and_get_brkpntdata_from_other_pitch_or_transpos_file
                        (filename,MIN_TRANSPOS,MAX_TRANSPOS,NO_SPECIAL_TYPE,dz))<0)
                        return(exit_status);
                    break;
            }
            break;
        case(ALL_FILES):
            if(dz->process==INFO_DIFF)
                break;
        case(ANY_NUMBER_OF_ANY_FILES):
            if(dz->process==HOUSE_BUNDLE || dz->process==ADDTOMIX || dz->process==BATCH_EXPAND || dz->process==MIX_MODEL)
                break;
        default:
            sprintf(errstr,"File '%s' has invalid data-type for this application.\n",filename);
            return(DATA_ERROR);
    }
    return(FINISHED);
}

/************************** OPEN_CHECKTYPE_GETSIZE_AND_COMPAREHEADER *****************************/

int open_checktype_getsize_and_compareheader(char *filename,int fileno,fileptr *fp2,dataptr dz)
{
    int exit_status;
    FILE *fp;
    int filetype;
    
    if((dz->ifd[fileno] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
        switch(dz->input_data_type) {
            case(ANAL_WITH_PITCHDATA):
            case(PITCH_AND_PITCH):
            case(PITCH_AND_TRANSPOS):
            case(TRANSPOS_AND_TRANSPOS):
                //TW NEW
                return(CONTINUE);
            case(ANY_NUMBER_OF_ANY_FILES):
                if(dz->process == ADDTOMIX || dz->process == BATCH_EXPAND) {
                    sprintf(errstr,"cannot open input file %s to read data.\n",filename);
                    return(DATA_ERROR);
                }
                
                return(CONTINUE);
            case(SNDFILE_AND_BRKFILE):
            case(SNDFILE_AND_UNRANGED_BRKFILE):
                if((fp = fopen(filename,"r"))==NULL) {  /* test for a brkpnt file */
                    sprintf(errstr, "Can't open file %s to read data.\n",filename);
                    return(DATA_ERROR);
                }
                return read_brkvals_from_other_file(fp,filename,dz);
            case(SNDFILE_AND_DB_BRKFILE):
                if((fp = fopen(filename,"r"))==NULL) {  /* test for a brkpnt file */
                    sprintf(errstr, "Can't open file %s to read data.\n",filename);
                    return(DATA_ERROR);
                }
                return read_dB_brkvals_from_other_file(fp,filename,dz);
            case(TWO_SNDFILES):
            case(MANY_SNDFILES):
            case(ONE_OR_MANY_SNDFILES):
            case(SNDFILE_AND_ENVFILE):
            case(TWO_ANALFILES):
            case(THREE_ANALFILES):
            case(MANY_ANALFILES):
            case(ANAL_WITH_TRANSPOS):
            case(ANAL_AND_FORMANTS):
            case(PITCH_AND_FORMANTS):
                //TW NEW CASES
            case(ONE_OR_MORE_SNDSYS):
            case(PFE):
                sprintf(errstr,"cannot open input file %s to read data.\n",filename);
                return(DATA_ERROR);
                
            case(ALL_FILES):
                if(dz->process==INFO_DIFF) {
                    if((fp = fopen(filename,"r"))==NULL)
                        sprintf(errstr, "Can't open file %s to read data.\n",filename);
                    else
                        sprintf(errstr,"This process does not work with text files [%s].\n",filename);
                    return(DATA_ERROR);
                }
                /* fall thro */
            case(NO_FILE_AT_ALL):
            case(SNDFILES_ONLY):
            case(ANALFILE_ONLY):
            case(ENVFILES_ONLY):
            case(PITCHFILE_ONLY):
            case(FORMANTFILE_ONLY):
            case(BRKFILES_ONLY):
            case(DB_BRKFILES_ONLY):
            case(MIXFILES_ONLY):
            case(SNDLIST_ONLY):
            case(SND_OR_MIXLIST_ONLY):
            case(SND_SYNC_OR_MIXLIST_ONLY):
                sprintf(errstr,"Should be impossible to call func open_checktype_getsize_and_compareheader()\n");
                return(PROGRAM_ERROR);
            default:
                sprintf(errstr,"open_checktype_getsize_and_compareheader() not defined for this filetype\n");
                return(PROGRAM_ERROR);
        }
    }
    if(dz->has_otherfile) {
        if((exit_status = set_otherfile_header(dz->ifd[fileno],filename,dz))<0)
            return(exit_status);
        filetype = dz->otherfile->filetype;
    } else {
        //TW NEW
        //      if((exit_status = headcompare_and_update(dz->ifd[fileno],filename,&filetype,fp2,dz))<0)
        if((exit_status = headcompare_and_update(dz->ifd[fileno],filename,&filetype,fp2,fileno,dz))<0)
            return(exit_status);
    }
    
    if((dz->insams[fileno] = sndsizeEx(dz->ifd[fileno]))<0) {       /* FIND SIZE OF FILE */
        sprintf(errstr, "Can't read size of input file %s.\n"
                "open_checktype_getsize_and_compareheader()\n",filename);
        return(PROGRAM_ERROR);
    }
    if((exit_status = check_for_data_in_later_file(filename,filetype,fileno,dz))<0)
        return(exit_status);
    
    return(FINISHED);
}

/***************************** HEADCOMPARE_AND_UPDATE *******************************/

//TW NEW
//int headcompare_and_update(int fd,char *filename,int *this_filetype,fileptr *fp2,dataptr dz)
int headcompare_and_update(int fd,char *filename,int *this_filetype,fileptr *fp2,int fileno,dataptr dz)
{
    int exit_status;
    fileptr fp1 = dz->infile;
    double maxamp, maxloc;
    int maxrep;
    int getmax = 0, getmaxinfo = 0;
    infileptr ifp;
    if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store data on later infile. (1)\n");
        return(MEMORY_ERROR);
    }
    if((*fp2 = (fileptr)malloc(sizeof(struct fileprops)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store data on later infile. (2)\n");
        return(MEMORY_ERROR);
    }
    if((exit_status = readhead(ifp,fd,filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
        return(exit_status);
    copy_to_fileptr(ifp,*fp2);
    if((exit_status = check_later_filetype_valid_for_process(filename,(*fp2)->filetype,fileno,dz))<0)
        return(exit_status);
    
    if(fp1->filetype==TEXTFILE) {
        *this_filetype = (*fp2)->filetype;
        return(FINISHED);
    }
    switch(dz->input_data_type) {
        case(TWO_SNDFILES):
        case(MANY_SNDFILES):
        case(ONE_OR_MANY_SNDFILES): /* compare properties common to all short and float files except envfiles */
            if((*fp2)->filetype != fp1->filetype) {
                sprintf(errstr,"Incompatible file type in input file %s.\n",filename);
                return(DATA_ERROR);
            }
            break;
    }
    switch(dz->input_data_type) {
            /***
             compare specifically formant properties : possible future usage *
             case(FORMANTS_AND_FORMANTS):
             if((*fp2)->specenvcnt != fp1->specenvcnt) {
             sprintf(errstr,"Incompatible specenvcnt in input file %s.\n",filename);
             return(DATA_ERROR):
             }
             dz->specenvcnt = fp1->specenvcnt;
             * fall thro *
             ***/
        case(PITCH_AND_FORMANTS):   /* compare properties not in analfiles */
        case(PITCH_AND_PITCH):
        case(PITCH_AND_TRANSPOS):
        case(TRANSPOS_AND_TRANSPOS):
        case(PITCH_OR_TRANSPOS):
            if((*fp2)->origchans != fp1->origchans) {
                sprintf(errstr,"Incompatible original-channel-count in input file %s.\n",filename);
                return(DATA_ERROR);
            }
            /* fall thro */
        case(ANAL_AND_FORMANTS):    /* compare properties common to all float files except envfiles */
        case(ANAL_WITH_PITCHDATA):
        case(ANAL_WITH_TRANSPOS):
        case(TWO_ANALFILES):
        case(THREE_ANALFILES):
        case(MANY_ANALFILES):
            if((*fp2)->origstype != fp1->origstype) {
                sprintf(errstr,"Incompatible original-sample-type in input file %s.\n",filename);
                return(DATA_ERROR);
            }
            if((*fp2)->origrate != fp1->origrate) {
                sprintf(errstr,"Incompatible original-sample-rate in input file %s.\n",filename);
                return(DATA_ERROR);
            }
            if((*fp2)->arate != fp1->arate) {
                sprintf(errstr,"Incompatible analysis-sample-rate in input file %s.\n",filename);
                return(DATA_ERROR);
            }
            if((*fp2)->Mlen != fp1->Mlen) {
                sprintf(errstr,"Incompatible analysis-window-length in input file %s.\n",filename);
                return(DATA_ERROR);
            }
            if((*fp2)->Dfac != fp1->Dfac) {
                sprintf(errstr,"Incompatible decimation factor in input file %s.\n",filename);
                return(DATA_ERROR);
            }
            /* fall thro */
        case(TWO_SNDFILES):
        case(MANY_SNDFILES):
        case(ONE_OR_MANY_SNDFILES): /* compare properties common to all short and float files except envfiles */
            if((*fp2)->srate != fp1->srate) {
                sprintf(errstr,"Incompatible sample-rate in input file %s.\n",filename);
                return(DATA_ERROR);
            }
            
            switch(dz->input_data_type) {
                case(TWO_ANALFILES):
                case(THREE_ANALFILES):
                case(MANY_ANALFILES):
                case(TWO_SNDFILES):
                case(MANY_SNDFILES):
                case(ONE_OR_MANY_SNDFILES):
                    if((*fp2)->channels != fp1->channels) {
                        if(!(dz->process == MOD_LOUDNESS) && !(dz->mode == 6)) {
                            sprintf(errstr,"Incompatible channel-count in input file %s.\n",filename);
                            return(DATA_ERROR);
                        }
                    }
                    break;
                case(ANAL_AND_FORMANTS):    /* compare properties and derived properties */
                case(ANAL_WITH_PITCHDATA):
                case(ANAL_WITH_TRANSPOS):
                    if((*fp2)->origchans != fp1->channels) {
                        sprintf(errstr,"Incompatible (original) channel-count in input file %s.\n",filename);
                        return(DATA_ERROR);
                    }
            }
            break;
        case(SNDFILE_AND_ENVFILE):
            break;
        case(ANY_NUMBER_OF_ANY_FILES):
            //TW NEW CASE
        case(ONE_OR_MORE_SNDSYS):
            break;
            //TW NEW CASE
        case(PFE):
            if(fileno==1) {
                if((*fp2)->origchans != fp1->origchans) {
                    sprintf(errstr,"Incompatible original-channel-count in formant file %s.\n",filename);
                    return(DATA_ERROR);
                }
                if((*fp2)->origstype != fp1->origstype) {
                    sprintf(errstr,"Incompatible original-sample-type in formant file %s.\n",filename);
                    return(DATA_ERROR);
                }
                if((*fp2)->origrate != fp1->origrate) {
                    sprintf(errstr,"Incompatible original-sample-rate in formant file %s.\n",filename);
                    return(DATA_ERROR);
                }
                if((*fp2)->arate != fp1->arate) {
                    sprintf(errstr,"Incompatible analysis-sample-rate in formant file %s.\n",filename);
                    return(DATA_ERROR);
                }
                if((*fp2)->Mlen != fp1->Mlen) {
                    sprintf(errstr,"Incompatible analysis-window-length in formant file %s.\n",filename);
                    return(DATA_ERROR);
                }
                if((*fp2)->Dfac != fp1->Dfac) {
                    sprintf(errstr,"Incompatible decimation factor in formant file %s.\n",filename);
                    return(DATA_ERROR);
                }
                if((*fp2)->srate != fp1->srate) {
                    sprintf(errstr,"Incompatible sample-rate in formant file %s.\n",filename);
                    return(DATA_ERROR);
                }
                /* NO LONGER POSSIBLE WITH NEW SFSYS
                 if((*fp2)->stype != fp1->stype) {
                 sprintf(errstr,"Incompatible sample type in formant file %s.\n",filename);
                 return(DATA_ERROR);
                 }
                 */
                fp1->specenvcnt = (*fp2)->specenvcnt;
                dz->specenvcnt  = fp1->specenvcnt;
            } else {
                //NEW
                if((*fp2)->srate != fp1->srate) {
                    sprintf(errstr,"Incompatible sample-rate (%d) in envelope file %s (must be %d).\n",
                            (*fp2)->srate,filename,fp1->srate);
                    return(DATA_ERROR);
                }
                //NEW TO HERE
            }
            *this_filetype = (*fp2)->filetype;
            return(FINISHED);
        default:
            sprintf(errstr,"Inaccessible case in  headcompare_and_update()\n");
            return(PROGRAM_ERROR);
    }
    /* ADD ANY ADDITIONAL PROPERTIES TO INFILE PROP LIST */
    switch(dz->input_data_type) {
        case(PITCH_AND_FORMANTS):
        case(ANAL_AND_FORMANTS):    /* NB: origchans, if needed, will be set at headwrite stage */
            fp1->specenvcnt = (*fp2)->specenvcnt;
            dz->specenvcnt  = fp1->specenvcnt;
            break;
    }
    *this_filetype = (*fp2)->filetype;
    return(FINISHED);
}       

/***************************** SET_OTHERFILE_HEADER *******************************
 *
 * Set up an independent header.
 */

int set_otherfile_header(int fd,char *filename,dataptr dz)
{
    int exit_status;
    double maxamp, maxloc;
    int maxrep;
    int getmax = 0, getmaxinfo = 0;
    infileptr ifp;
    if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store data on other file.\n");
        return(MEMORY_ERROR);
    }
    if((exit_status = readhead(ifp,fd,filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
        return(exit_status);
    copy_to_fileptr(ifp,dz->otherfile);
    if((exit_status = check_later_filetype_valid_for_process(filename,dz->otherfile->filetype,0,dz))<0)
        return(exit_status);
    return(FINISHED);
}       

/***************************** CHECK_LATER_FILETYPE_VALID_FOR_PROCESS *******************************/

//TW NEW
//int check_later_filetype_valid_for_process(char *filename,int file_type,dataptr dz)
int check_later_filetype_valid_for_process(char *filename,int file_type,int fileno,dataptr dz)
{
    switch(dz->input_data_type) {
        case(NO_FILE_AT_ALL):
        case(ALL_FILES):
        case(ANY_NUMBER_OF_ANY_FILES):
            break;
        case(TWO_SNDFILES):
        case(MANY_SNDFILES):
        case(ONE_OR_MANY_SNDFILES):
            if(file_type != SNDFILE) {
                sprintf(errstr,"%s is not a sound file.\n",filename);
                return(DATA_ERROR);
            }
            break;
        case(SNDFILE_AND_ENVFILE):
            if(file_type != ENVFILE) {
                sprintf(errstr,"%s is not an envelope file.\n",filename);
                return(DATA_ERROR);
            }
            break;
        case(TWO_ANALFILES):
        case(THREE_ANALFILES):
        case(MANY_ANALFILES):
            if(file_type != ANALFILE) {
                sprintf(errstr,"%s is not an analysis file.\n",filename);
                return(DATA_ERROR);
            }
            break;
        case(PITCH_AND_PITCH):
        case(ANAL_WITH_PITCHDATA):
            if(file_type != PITCHFILE) {
                sprintf(errstr,"%s is not a pitch file.\n",filename);
                return(DATA_ERROR);
            }
            break;
        case(PITCH_AND_TRANSPOS):
        case(TRANSPOS_AND_TRANSPOS):
        case(ANAL_WITH_TRANSPOS):
            if(file_type != TRANSPOSFILE) {
                sprintf(errstr,"%s is not a transposition file.\n",filename);
                return(DATA_ERROR);
            }
            break;
        case(ANAL_AND_FORMANTS):
        case(PITCH_AND_FORMANTS):
            /*  case(FORMANTS_AND_FORMANTS): */
            if(file_type != FORMANTFILE) {
                sprintf(errstr,"%s is not a formants file.\n",filename);
                return(DATA_ERROR);
            }
            break;
            //TW NEW CASE
        case(ONE_OR_MORE_SNDSYS):
            break;
            //TW NEW CASE
        case(PFE):
            if(fileno==1) {
                if(file_type != FORMANTFILE) {
                    sprintf(errstr,"%s is not a formants file.\n",filename);
                    return(DATA_ERROR);
                }
            } else if (file_type != ENVFILE) {
                sprintf(errstr,"%s is not an envelope file.\n",filename);
                return(DATA_ERROR);
            }
            break;
        default:
            sprintf(errstr,"Impossible input_data_type: check_later_filetype_valid_for_process()\n");
            return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/************************************ CHECK_FOR_DATA_IN_LATER_FILE *****************************************/

int check_for_data_in_later_file(char *filename,int filetype,int fileno,dataptr dz)
{
    int clength;
    
    //TW NEW
    //  if(dz->input_data_type == ANY_NUMBER_OF_ANY_FILES) {
    if(dz->input_data_type == ANY_NUMBER_OF_ANY_FILES || dz->input_data_type == ONE_OR_MORE_SNDSYS) {
        if(dz->insams[fileno]==0) {
            sprintf(errstr, "File %s contains no data.\n",filename);
            return(DATA_ERROR);
        }
        return(FINISHED);
    }
    
    switch(filetype) {
        case(FORMANTFILE):
            
#ifdef IS_CDPMAIN_TESTING
            
            if(fileno >= 2) {
                sprintf(errstr,"check_for_data_in_later_file()\n"
                        "only works (so far) for ONE input formantfile accompanying 1st file.\n");
                return(PROGRAM_ERROR);
            }
            if(dz->infile->filetype  == SNDFILE || dz->infile->filetype  == FORMANTFILE) {
                sprintf(errstr,"check_for_data_in_later_file()\n"
                        "Not yet able to deal with FORMANTFILE accompanying FORMANTFILE or SNDFILE.\n");
                return(PROGRAM_ERROR);
            }
#endif
            
            if(dz->insams[fileno] <= (int)(DESCRIPTOR_DATA_BLOKS * dz->infile->specenvcnt)) {
                sprintf(errstr, "File %s contains no formant data.\n",filename);
                return(DATA_ERROR);
            }
            switch(dz->infile->filetype) {
                case(FORMANTFILE):
                case(SNDFILE):
                case(ENVFILE):
                    break;
                case(ANALFILE):
                    clength = (dz->infile->channels)/2;             /* channels per window */
                    if(dz->infile->specenvcnt > clength+1) {
                        sprintf(errstr,"Formant data incompatible with existing channel data.\n"
                                "check_for_data_in_later_file()\n");
                        return(PROGRAM_ERROR);
                    }
                    break;
                case(PITCHFILE):
                case(TRANSPOSFILE):
                    clength = (dz->infile->origchans)/2;            /* channels per window */
                    if(dz->infile->specenvcnt > clength+1) {
                        sprintf(errstr,"Formant data incompatible with existing channel data.\n"
                                "check_for_data_in_later_file()\n");
                        return(PROGRAM_ERROR);
                    }
                    break;
                default:
                    sprintf(errstr,"Unknown case for original filetype combining with FORMATFILE\n");
                    return(PROGRAM_ERROR);
            }
            break;
        default:
            if(dz->insams[fileno]==0) {
                sprintf(errstr, "File %s contains no data.\n",filename);
                return(DATA_ERROR);
            }
            break;
    }
    return(FINISHED);
}

/************************* READ_BRKVALS_FROM_OTHER_FILE *****************************
 *
 * This only reads 0-1 range brkdata from 2nd file of a pair,
 * and only where extrabrkno has been established for this infile_type.
 * i.e. SNDFILE_AND_BRKFILE at Jan 16: 1998.
 */

int read_brkvals_from_other_file(FILE *fp,char *filename,dataptr dz)
{
    double maxval = 0.0;
    double minval = 0.0;
    int arraysize = BIGARRAY;
    double *p, lasttime = 0.0;
    int  istime = TRUE;
    int n = 0;
    char temp[200], *q;
    int paramno = dz->extrabrkno;
    switch(dz->input_data_type) {
        case(SNDFILE_AND_BRKFILE):
            maxval = 1.0;
            minval = 0.0;
            break;
        case(SNDFILE_AND_UNRANGED_BRKFILE):
            maxval = (double)MAXSAMP;
            minval = 0.0;
            break;
    }
    if(paramno<0) {
        sprintf(errstr,"extrabrk not allocated: read_brkvals_from_other_file()\n");
        return(PROGRAM_ERROR);
    }
    if((dz->brk[paramno] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for input brktable data.\n");
        return(MEMORY_ERROR);
    }
    p = dz->brk[paramno];
    while(fgets(temp,200,fp)==temp) {    /* READ AND TEST BRKPNT VALS */
        q = temp;
        while(get_float_from_within_string(&q,p)) {
            if(istime) {
                if(p==dz->brk[paramno]) {
                    if(*p < 0.0) {
                        sprintf(errstr,"First timeval(%lf) in brkpntfile %s is less than zero.\n",*p,filename);
                        return(DATA_ERROR);
                    }
                } else {
                    if(*p <= lasttime) {
                        sprintf(errstr,"Times (%lf & %lf) in brkpntfile %s are not in increasing order.\n",
                                lasttime,*p,filename);
                        return(DATA_ERROR);
                    }
                }
                lasttime = *p;
            } else {
                if(*p < minval || *p > maxval) {
                    sprintf(errstr,"Brkpntfile value (%lf) out of range (%.0lf - %.0lf)\n",*p,minval,maxval);   /*RWD 4:12:2003 added *p arg */
                    return(DATA_ERROR);
                }
            }
            istime = !istime;
            p++;
            if(++n >= arraysize) {
                arraysize += BIGARRAY;
                if((dz->brk[paramno] = (double *)realloc((char *)(dz->brk[paramno]),arraysize * sizeof(double)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate input brktable data.\n");
                    return(MEMORY_ERROR);
                }
                p = dz->brk[paramno] + n;
            }
        }
    }
    if(n < 4) {
        sprintf(errstr,"Not enough data in brkpnt file %s\n",filename);
        return(DATA_ERROR);
    }
    if(ODD(n)) {
        sprintf(errstr,"Data not paired correctly in brkpntfile %s\n",filename);
        return(DATA_ERROR);
    }
    if((dz->brk[paramno] = (double *)realloc((char *)(dz->brk[paramno]),n * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate input brktable data.\n");
        return(MEMORY_ERROR);
    }
    dz->brksize[paramno] = n/2;
    dz->otherfile->filetype = BRKFILE;
    return(FINISHED);
}

/************************* READ_DB_BRKVALS_FROM_OTHER_FILE *****************************
 *
 * This only reads dB range brkdata from 2nd file of a pair, converting vals to normalised range..
 * and only where extrabrkno has been established for this infile_type.
 * i.e. SNDFILE_AND_DB_BRKFILE at Jan 16: 1998.
 */

int read_dB_brkvals_from_other_file(FILE *fp,char *filename,dataptr dz)
{
    int exit_status;
    double maxval = 0.0;
    double minval = MIN_DB_ON_16_BIT;
    int arraysize = BIGARRAY;
    double *p, lasttime = 0.0;
    int  istime = TRUE;
    int n = 0;
    char temp[200], *q;
    int paramno = dz->extrabrkno;
    if(paramno<0) {
        sprintf(errstr,"extrabrk not allocated: read_dB_brkvals_from_other_file()\n");
        return(PROGRAM_ERROR);
    }
    if((dz->brk[paramno] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for input brktable data.\n");
        return(MEMORY_ERROR);
    }
    p = dz->brk[paramno];
    while(fgets(temp,200,fp)==temp) {    /* READ AND TEST BRKPNT VALS */
        q = temp;
        while(get_float_from_within_string(&q,p)) {
            if(istime) {
                if(p==dz->brk[paramno]) {
                    if(*p < 0.0) {
                        sprintf(errstr,"First timeval(%lf) in brkpntfile %s is less than zero.\n",*p,filename);
                        return(DATA_ERROR);
                    }
                } else {
                    if(*p <= lasttime) {
                        sprintf(errstr,"Times (%lf & %lf) in brkpntfile %s are not in increasing order.\n",
                                lasttime,*p,filename);
                        return(DATA_ERROR);
                    }
                }
                lasttime = *p;
            } else {
                if(*p < minval || *p > maxval) {
                    sprintf(errstr,"dB brkpntfile value (%lf) out of range (%.0lf = %.0lf)\n",
                            *p,minval,maxval);          /*RWD 4:12:2003 added args */
                    return(DATA_ERROR);
                }
                if((exit_status = convert_dB_at_or_below_zero_to_gain(p))<0)
                    return(exit_status);
            }
            istime = !istime;
            p++;
            if(++n >= arraysize) {
                arraysize += BIGARRAY;
                if((dz->brk[paramno] = (double *)realloc((char *)(dz->brk[paramno]),arraysize * sizeof(double)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate input brktable data.\n");
                    return(MEMORY_ERROR);
                }
                p = dz->brk[paramno] + n;
            }
        }
    }
    if(n < 4) {
        sprintf(errstr,"No data in brkpnt file %s\n",filename);
        return(DATA_ERROR);
    }
    if(ODD(n)) {
        sprintf(errstr,"Data not paired correctly in brkpntfile %s\n",filename);
        return(DATA_ERROR);
    }
    if((dz->brk[paramno] = (double *)realloc((char *)(dz->brk[paramno]),n * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate input brktable data.\n");
        return(MEMORY_ERROR);
    }
    dz->brksize[paramno] = n/2;
    dz->otherfile->filetype = DB_BRKFILE;
    return(FINISHED);
}

/********************************** SETUP_VALID_WLENGTH ************************************/

int setup_valid_wlength(dataptr dz)
{
    if(dz->infile->filetype == TEXTFILE) {
        if(dz->duration <= 0.0) {
            sprintf(errstr,"duration of first file not set: setup_valid_wlength()\n");
            return(PROGRAM_ERROR);
        }
        dz->wlength = round(dz->duration/dz->frametime); /* wlength of file1 */
    }
    dz->wlength = min(dz->wlength,dz->insams[1]);
    return(FINISHED);
}

/********************* GET_PITCH_OR_TRANSPOS_BINARYDATA_FROM_OTHER_FILE *************************/

int get_pitch_or_transpos_binarydata_from_other_file(int max_arraysize,int fileno,float **thisdata,fileptr fp2,dataptr dz)
{
    int exit_status;
    int  /*seccnt,*/ samps_to_read, samps_read, actual_arraysize;
    float lastval;
    fileptr fp1 = dz->infile;
    
#ifdef IS_CDPMAIN_TESTING
    if(fileno >= 2) {
        sprintf(errstr,"get_pitch_or_transpos_binarydata_from_other_file() does not work YET for > 2 infiles\n");
        return(PROGRAM_ERROR);
    }
#endif
    
    samps_to_read = max_arraysize;
    
    max_arraysize = samps_to_read;
    if((*thisdata = (float *)malloc((size_t)(samps_to_read * sizeof(float))))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for 2nd pitch or transposition array.\n");
        return(MEMORY_ERROR);
    }
    if((samps_read  = fgetfbufEx((*thisdata), samps_to_read,dz->ifd[fileno],0))<0) {
        sprintf(errstr,"Sound read error.\n");
        return(SYSTEM_ERROR);
    }
    if(samps_read != dz->insams[fileno]) {
        sprintf(errstr,"Problem reading data: get_pitch_or_transpos_binarydata_from_other_file()\n");
        return(PROGRAM_ERROR);
    }
    actual_arraysize = dz->insams[fileno];
    switch(dz->input_data_type) {
        case(PITCH_AND_PITCH):
            if(dz->infile->filetype!=TEXTFILE)     /* force data to be min length of two input files */
                actual_arraysize = min(actual_arraysize,dz->wlength);
            else {
                dz->nyquist = (double)fp2->origrate/2.0;
                dz->wlength = actual_arraysize;
            }
            //TW NEW
            //      if((exit_status = test_brkvals(-1.0,dz->nyquist,*thisdata,actual_arraysize))<0)
            if((exit_status = test_brkvals(NOT_SOUND,dz->nyquist,*thisdata,actual_arraysize))<0)
                return(exit_status);
            if(dz->infile->filetype==TEXTFILE) {   /* 1st input is brkpnt, 2nd is binary */
                if((exit_status = inform_infile_structure(fp1,fp2,dz))<0)
                    return(exit_status);
                if((exit_status = convert_brkpntdata_to_window_by_window_array
                    (dz->temp,dz->tempsize,&dz->pitches,dz->wlength,dz->frametime))<0)
                    return(exit_status);
            }
            break;
        case(PITCH_AND_TRANSPOS):
            if(dz->infile->filetype!=TEXTFILE)     /* force data to be min length of two input files */
                actual_arraysize = min(actual_arraysize,dz->wlength);
            else
                dz->wlength = actual_arraysize;
            if((exit_status = test_brkvals(MIN_TRANSPOS,MAX_TRANSPOS,*thisdata,actual_arraysize))<0)
                return(exit_status);
            if(dz->infile->filetype==TEXTFILE) {   /* 1st input is brkpnt, 2nd is binary */
                if((exit_status = inform_infile_structure(fp1,fp2,dz))<0)
                    return(exit_status);
                if((exit_status = convert_brkpntdata_to_window_by_window_array
                    (dz->temp,dz->tempsize,&dz->pitches,dz->wlength,dz->frametime))<0)
                    return(exit_status);
            }
            break;
        case(TRANSPOS_AND_TRANSPOS):
            if(dz->infile->filetype!=TEXTFILE)     /* force data to be min length of two input files */
                actual_arraysize = min(actual_arraysize,dz->wlength);
            else
                dz->wlength = actual_arraysize;
            if((exit_status = test_brkvals(MIN_TRANSPOS,MAX_TRANSPOS,*thisdata,actual_arraysize))<0)
                return(exit_status);
            if(dz->infile->filetype==TEXTFILE) {   /* 1st input is brkpnt, 2nd is binary */
                if((exit_status = inform_infile_structure(fp1,fp2,dz))<0)
                    return(exit_status);
                if((exit_status = convert_brkpntdata_to_window_by_window_array
                    (dz->temp,dz->tempsize,&dz->transpos,dz->wlength,dz->frametime))<0)
                    return(exit_status);
            }
            break;
        case(ANAL_WITH_TRANSPOS):
            /* force data to be length of 1st input files */
            lastval = (*thisdata)[actual_arraysize-1];
            
#ifdef IS_CDPMAIN_TESTING
            if(max_arraysize < dz->wlength) {
                sprintf(errstr,"Error in establishing arraysize: get_pitch_or_transpos_binarydata_from_other_file()\n");
                return(PROGRAM_ERROR);
            }
#endif 
            
            while(actual_arraysize < dz->wlength) {
                (*thisdata)[actual_arraysize] = lastval;
                actual_arraysize++;
            }
            break;
        case(ANAL_WITH_PITCHDATA):
            /* data is length of 1st input files */
            actual_arraysize = max_arraysize;
            break;
        default:
            sprintf(errstr,"Invalid case in get_pitch_or_transpos_binarydata_from_other_file()\n");
            return(PROGRAM_ERROR);
    }
    if((*thisdata = (float *)realloc((char *)(*thisdata),(actual_arraysize + 1) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate 2nd table of pitch or transposition data.\n");
        return(MEMORY_ERROR);
    }
    (*thisdata)[actual_arraysize] = 0.0f;   /* SAFETY POSITION */
    return(FINISHED);
}

/********************** GETSIZE_AND_GET_BRKPNTDATA_FROM_OTHER_PITCH_OR_TRANSPOS_FILE *******************/

int getsize_and_get_brkpntdata_from_other_pitch_or_transpos_file
(char *filename,double minval, double maxval,int which_type, dataptr dz)
{
    int exit_status;
    FILE *fp;
    double *brktable = NULL;
    /*int  init = 1, istime = 1;*/
    int /* n = 0,*/ brksize;
    int wlen;
    float timestep;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr, "Can't open brkpntfile %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    if((exit_status=read_and_test_pitch_or_transposition_brkvals
        (fp,filename,&brktable,&brksize,which_type,minval,maxval))<0) {
        if(brktable != NULL)
            free(brktable);
        return(exit_status);
    }
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
        fflush(stdout);
    }
    switch(dz->input_data_type) {
            
#ifdef IS_CDPMAIN_TESTING
        case(ANAL_WITH_TRANSPOS):
            sprintf(errstr,"Inaccesible case: getsize_and_get_brkpntdata_from_other_pitch_or_transpos_file()\n");
            return(PROGRAM_ERROR);
#endif
            
        case(ANAL_WITH_PITCHDATA):
            break;  /* files are same length: so minwindowlength = wlength */
        default:
            if((exit_status = establish_minwindowlength_or_minduration_in_secs(brktable,brksize,filename,dz))<0) {
                free(brktable);
                return(exit_status);
            }
            break;
    }
    
    if(dz->wlength==0) { /* Neither input binary: use default_frametime & convert both files to bin form */
        wlen     = round(dz->duration/DEFAULT_FRAMETIME);
        timestep = DEFAULT_FRAMETIME;
    } else {
        wlen     = dz->wlength;
        timestep = dz->frametime;
    }
    
    switch(dz->input_data_type) {
        case(ANAL_WITH_PITCHDATA):  /* First file is binary: convert second */
            if((exit_status = convert_brkpntdata_to_window_by_window_array(brktable,brksize,&dz->pitches,wlen,timestep))<0) {
                free(brktable);
                return(exit_status);
            }
            break;
        case(PITCH_AND_PITCH):
            if(dz->wlength==0) {    /* Neither input is binary: convert first file */
                if((exit_status = convert_brkpntdata_to_window_by_window_array
                    (dz->temp,dz->tempsize,&dz->pitches,wlen,timestep))<0)
                    return(exit_status);
            }                       /* Convert second file */
            if((exit_status = convert_brkpntdata_to_window_by_window_array(brktable,brksize,&(dz->pitches2),wlen,timestep))<0) {
                free(brktable);
                return(exit_status);
            }
            break;
        case(PITCH_AND_TRANSPOS):
            if(dz->wlength==0) {    /* Neither input is binary: convert first file */
                if((exit_status = convert_brkpntdata_to_window_by_window_array
                    (dz->temp,dz->tempsize,&dz->pitches,wlen,timestep))<0)
                    return(exit_status);
            }                       /* Convert second file */
            if((exit_status = convert_brkpntdata_to_window_by_window_array
                (brktable,brksize,&dz->transpos,wlen,timestep))<0) {
                free(brktable);
                return(exit_status);
            }
            break;
        case(TRANSPOS_AND_TRANSPOS):
            if(dz->wlength==0) {    /* Neither input is binary: convert first file */
                if((exit_status = convert_brkpntdata_to_window_by_window_array
                    (dz->temp,dz->tempsize,&dz->transpos,wlen,timestep))<0)
                    return(exit_status);
            }                       /* Convert second file */
            if((exit_status = convert_brkpntdata_to_window_by_window_array
                (brktable,brksize,&dz->transpos2,wlen,timestep))<0) {
                free(brktable);
                return(exit_status);
            }
            break;
    }
    free(brktable);
    return(FINISHED);
}

/************************* ESTABLISH_MINWINDOWLENGTH_OR_MINDURATION_IN_SECS *****************************/

int establish_minwindowlength_or_minduration_in_secs(double *brktable,int brksize,char *filename,dataptr dz)
{
    int exit_status;
    double thisduration;
    int thislen;
    if((exit_status = establish_duration_in_secs(&thisduration,brktable,brksize,filename))<0)
        return(exit_status);
    if(dz->infile->filetype == TEXTFILE) {                      /* 1st file was a brkpntfile */
        
#ifdef IS_CDPMAIN_TESTING
        if(dz->duration <= 0.0) {
            sprintf(errstr,"duration not set: establish_minwindowlength_or_minduration_in_secs()\n");
            return(PROGRAM_ERROR);
        }
#endif
        
        dz->duration = min(thisduration,dz->duration);
    }
    /* both files are brkpnt: hence wlength remains zero */
    else {        /* 1st file was a binary file: so frametime is established */
        
#ifdef IS_CDPMAIN_TESTING
        if(dz->wlength <=0) {
            sprintf(errstr,"wlength not set: establish_minwindowlength_or_minduration_in_secs()\n");
            return(PROGRAM_ERROR);
        }
#endif
        
        thislen = round(thisduration/dz->frametime);
        dz->wlength = min(dz->wlength,thislen);
    }
    return(FINISHED);
}

/************************* ESTABLISH_DURATION_IN_SECS *****************************/

int establish_duration_in_secs(double *duration,double *brktable,int brksize,char *filename)
{
    if((*duration = *(brktable + (brksize * 2) - 2))<=0.0) {
        sprintf(errstr,"No significant data in file %s\n",filename);
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/************************ CHECK_FOR_PITCH_ZEROS ***************************/
#ifdef NOTDEF
int check_for_pitch_zeros(char *filename,float *pitcharray,int wlen)
{
    int n;
    for(n=0;n<wlen;n++) {
        if(pitcharray[n] < FLTERR) {
            sprintf(errstr,"File %s contains unpitched windows: cannot proceed.\n",filename);
            return(DATA_ERROR);
        }
    }
    return(FINISHED);
}
#endif
/******************************* TEST_BRKVALS ****************************/

int test_brkvals(double minval,double maxval,float *floatarray,int arraysize)
{
    float *p = floatarray;
    float *pend = floatarray + arraysize;
    while(p < pend) {
        if(*p < minval) {
            sprintf(errstr,"Value (%lf) too small (minval = %lf) in further file.\n",*p,minval);
            return(DATA_ERROR);
        }
        if(*p > maxval) {
            sprintf(errstr,"Value (%lf) too large (maxval = %lf) in further file.\n",*p,maxval);
            return(DATA_ERROR);
        }
        p++;
    }
    return(FINISHED);
}

/******************************* INFORM_INFILE_STRUCTURE ****************************/

int inform_infile_structure(fileptr fp1,fileptr fp2,dataptr dz)
{
    if(dz->infilecnt > 2) {
        sprintf(errstr,"Input processes for 1 textfile & >1 nontextfile are NOT DEFINED\n"
                "Hence 2 later nontextfiles are not header_compared\n");
        return(PROGRAM_ERROR);
    }
    switch(dz->input_data_type) {
        case(PITCH_AND_PITCH):
        case(PITCH_AND_TRANSPOS):
        case(TRANSPOS_AND_TRANSPOS):
            fp1->filetype  = fp2->filetype; /* Setup infile properties here */
            fp1->origchans = fp2->origchans;
            fp1->origstype = fp2->origstype;
            fp1->origrate  = fp2->origrate;
            fp1->arate     = fp2->arate;
            fp1->Mlen      = fp2->Mlen;
            fp1->Dfac      = fp2->Dfac;
            fp1->srate     = fp2->srate;
            fp1->channels  = fp2->channels;
            fp1->stype     = fp2->stype;
            /* Setup other header-based parameters here */
            dz->nyquist    = fp1->origrate/2.0;
            dz->frametime  = (float)(1.0/fp1->arate);
            break;
        default:
            sprintf(errstr,"No process defined taking this textfiletype and this sndfiletype.\n");
            return(PROGRAM_ERROR);
    }
    return(FINISHED);
}
