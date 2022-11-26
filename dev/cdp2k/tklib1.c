/* Release 8 */
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
// AUGUST 21: 2005

#include <stdio.h>
#include <stdlib.h>
#include <osbind.h>
#include <structures.h>
#include <tkglobals.h>
#include <sfsys.h>
#include <cdparams.h>
#include <globcon.h>
#include <localcon.h>
#include <processno.h>
#include <modeno.h>
#include <filetype.h>
#include <formants.h>
#include <pnames.h>
#include <sndinfo.h>
#include <headread.h>
#include <house.h>
#include <pvoc.h>
#include <distort1.h>
#include <ctype.h>
#include <string.h>
#include <logic.h>
#include <math.h>
#include <float.h>
//TW NEW
#include <vowels2.h>

/* WARNING: These function assumes brktables are INTERPOLATED LINEARLY */
/* If not, function needs to take SRATE as a parameter, and compare sample-by-sample */

#ifdef unix
#define round(x) lround((x))
#endif

#define EQUIVALENT      (0)     /* brktables are identical */
#define FIRST_LESS      (1)
#define FIRST_GREATER   (2)
#define CROSSED         (-1)    /* brktable vals cross */
#define INVALID         (-1)    /* at least 1 of brktables has only 1 val or doesn't exist */

static int      value_greater_than_brkvals(double val,double *brk,int brksize,int repel);
static int      value_less_than_brkvals(double val,double *brk,int brksize,int repel);
static int      brk_compare(double *brk1,int brksize1,double *brk2,int brksize2,int repel);
static int      establish_orientation(double val1, double val2,int repel);
static double   establish_current_time(double t1,double t2);
static double   establish_value_at_current_time(double thistime,double *t,double *brk,double *v);
static double   interpval(double thistime,double lastval, double nextval, double lasttime, double nexttime);
int getpeakdata(int ifd, float *peakval,int *peakpos,SFPROPS props);

/*
#define TEST_HEADREAD 1
*/
#ifdef TEST_HEADREAD
static int headread_check(SFPROPS props,char *filename,int ifd);
#endif

#define AN_HOUR     (3600.0)
#define A_MINUTE    (60.0)

/**************************** ITEMS FORMERLY IN FILETYPE **************************/

char *get_last_slash(char *filename,int filenamelen);

/**************************** ITEMS FORMERLY IN VALIDATE **************************/

void validate(int applicno,int *valid);

/***************************** GET_WORD_FROM_STRING ****************************
 *
 * Read a word in a larger string.
 *
 * p returns address reached after word read.
 * q returns address of word.
 *
 */

int get_word_from_string(char **p,char **q)
{
    while(isspace(**p))     /* SKIP LEADING SPACE */
        (*p)++;
    *q = *p;                /* SET IN_STRING POINTER */
    if(**p==ENDOFSTR)
        return(0);          /* END OF BIG_STRING: NO WORD THIS TIME */
    while(!isspace(**p)) {
        if(**p==ENDOFSTR)
            return(1);      /* WORD FOUND: END OF BIG_STRING */
        (*p)++;
    }
    **p = ENDOFSTR;         /* WORD FOUND,BUT NOT END OF BIG_STRING, SO ENDMARKED:  */
    (*p)++;                 /* GO TO NEXT CHARACTER */
    return(1);
}

/************************** GET_FLOAT_FROM_WITHIN_STRING **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

int  get_float_from_within_string(char **str,double *val)
{
    char   *p, *valstart;
    int    decimal_point_cnt = 0, has_digits = 0;
    p = *str;
    while(isspace(*p))
        p++;
    valstart = p;   
    switch(*p) {
    case('-'):                      break;
    case('.'): decimal_point_cnt=1; break;
    default:
        if(!isdigit(*p))
            return(FALSE);
        has_digits = TRUE;
        break;
    }
    p++;        
    while(!isspace(*p) && *p!=NEWLINE && *p!=ENDOFSTR) {
        if(isdigit(*p))
            has_digits = TRUE;
        else if(*p == '.') {
            if(++decimal_point_cnt>1)
                return(FALSE);
        } else
            return(FALSE);
        p++;
    }
    if(!has_digits || sscanf(valstart,"%lf",val)!=1)
        return(FALSE);
    *str = p;
    return(TRUE);
}

/**************************** FLTEQ *******************************/

int flteq(double f1,double f2)
{
    double upperbnd, lowerbnd;
    upperbnd = f2 + FLTERR;     
    lowerbnd = f2 - FLTERR;     
    if((f1>upperbnd) || (f1<lowerbnd))
        return(FALSE);
    return(TRUE);
}

//TW NEW, FOR FLOATPOINT DATA
/**************************** FLTEQ *******************************/

int smpflteq(double f1,double f2)
{
    double upperbnd, lowerbnd;
    upperbnd = f2 + SMP_FLTERR;     
    lowerbnd = f2 - SMP_FLTERR;     
    if((f1>upperbnd) || (f1<lowerbnd))
        return(FALSE);
    return(TRUE);
}

/*********************************** SWAP *************************/

void swap(double *d1, double *d2)
{
    double dd;
    dd  = *d1;
    *d1 = *d2;
    *d2 = dd;
}

/*********************************** IISWAP *************************/

void iiswap(int *d1, int *d2)
{
    int dd;
    dd  = *d1;
    *d1 = *d2;
    *d2 = dd;
}

/************************** MIDITOHZ ************************/

double miditohz(double val)
{
    val += 3.0;
    val /= 12.0;
    val  = pow((double)2.0,val);
    val *= LOW_A;
    return(val);
}    

/************************** HZTOMIDI *****************************/

int hztomidi(double *midi,double hz)
{   
    double val;
    if(hz< FLTERR) {
        sprintf(errstr,"Sent a 0 Hz value to hztomidi()\n");
        return(PROGRAM_ERROR);
    }
    val  = hz;
    val /= LOW_A;
    val  = LOG2(val);
    val *= 12.0;
    val -= 3.0;
    *midi = val;
    return(FINISHED);
}

/************************** UNCHECKED_HZTOMIDI *****************************/

double unchecked_hztomidi(double hz)
{
    double val;
    val  = hz;
    val /= LOW_A;
    val  = LOG2(val);
    val *= 12.0;
    val -= 3.0;
    return(val);
}

/********************** GET_LEVEL ****************************/
 
int get_level(char *thisword,double *level)
{
    if(is_dB(thisword)) {
        if(!get_leveldb(thisword,level)) {
            sprintf(errstr,"Failed to get dB level: get_level()\n");
            return(PROGRAM_ERROR);
        }       
    } else {
        if(sscanf(thisword,"%lf",level)!=1) {
            sprintf(errstr,"Failed to get level: get_level()\n");
            return(PROGRAM_ERROR);
        }       
    }
    return(FINISHED);
}

/************************************* IS_DB ******************************/

int is_dB(char *str)
{
    char *p = str + strlen(str);
        p -= 2;
    if(strcmp(p,"dB") && strcmp(p,"DB") && strcmp(p,"db"))
        return(FALSE);
    return(TRUE);
}

/**************************** GET_LEVELDB ********************************/

int get_leveldb(char *str,double *val)
{
    int is_neg = 0;
    if(sscanf(str,"%lf",val)!=1)
        return(FALSE);  
    *val = max(*val,MIN_DB_ON_16_BIT);
//TW NEW
    *val = min(*val,MAX_DB_ON_16_BIT);
    if(flteq(*val,0.0)) {
        *val = 1.0;
        return(TRUE);
    }
    if(*val<0.0) {
        *val = -(*val);
        is_neg = 1;
    }
    *val /= 20.0;
    *val = pow(10.0,*val);
    if(is_neg)
        *val = 1.0/(*val);
    return(TRUE);
}

/**************************** DBTOGAIN ********************************/

double dbtogain(double val)
{
    int is_neg = FALSE;
    if(val <= MIN_DB_ON_16_BIT)
        return(0.0);
    if(flteq(val,0.0))
        return(1.0);
    if(val<0.0) {
        val = -val;
        is_neg = TRUE;
    }
    val /= 20.0;
    val = pow(10.0,val);
    if(is_neg)
        val = 1.0/val;
    return(val);
}

/***************************** IS_AN_EMPTY_LINE_OR_A_COMMENT **************************/

int is_an_empty_line_or_a_comment(char *p)
{
    while(isspace(*p))
        p++;
    if(*p==';' || *p=='\n' || *p==ENDOFSTR)
        return(1);
    return(0);
}


/***************************** IS_A_COMMENT **************************/

int is_a_comment(char *p)
{
    while(isspace(*p))
        p++;
    if(*p==';')
        return(1);
    return(0);
}

/***************************** IS_AN_EMPTY_LINE **************************/

int is_an_empty_line(char *p)
{
    while(isspace(*p))
        p++;
    if(*p=='\n' || *p==ENDOFSTR)
        return(1);
    return(0);
}

/******************************* FLOAT_ARRAY ******************************
 *
 * Generate float array of required size.
 */

int float_array(float **q,int size)
{
    if((*q = (float *)malloc(size * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create array of floats.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/******************************* CONVERT_TIME_AND_VALS_TO_SAMPLECNTS *******************************/

int convert_time_and_vals_to_samplecnts(int paramno,dataptr dz)
{
    double *p, *pend;
    if(dz->brksize==NULL) {
        sprintf(errstr,"brksize array not initialised:convert_time_and_vals_to_samplecnts()\n");
        return(PROGRAM_ERROR);
    } 
    if(dz->brksize[paramno]) {
        if(dz->brk==NULL) {
            sprintf(errstr,"brk not initialised:convert_time_and_vals_to_samplecnts()\n");
            return(PROGRAM_ERROR);
        } 
        p    = dz->brk[paramno];
        pend = dz->brk[paramno] + (dz->brksize[paramno] * 2);
        while(p < pend) {
            *p = (double)round(*p * (double)dz->infile->srate);
            p++;
        }
        dz->iparam[paramno] = round(*(dz->brk[paramno] + 1));   /* initialise */
    } else
        dz->iparam[paramno] = round(dz->param[paramno] * (double)dz->infile->srate);
    dz->is_int[paramno] = TRUE;
    return FINISHED;
}

/************************** CONVERT_TIME_TO_SAMPLECNTS *******************************
 *
 * Converts time(s) to (stereo-pairs) sample-cnt.
 */

int convert_time_to_samplecnts(int paramno,dataptr dz)
{
    double *p, *pend;
    if(dz->brksize==NULL) {
        sprintf(errstr,"brksize not initialised:convert_time_to_samplecnts()\n");
        return(PROGRAM_ERROR);
    } 
    if(dz->brksize[paramno]) {
        if(dz->brk==NULL) {
            sprintf(errstr,"brk not initialised:convert_time_to_samplecnts()\n");
            return(PROGRAM_ERROR);
        } 
        p    = dz->brk[paramno];
        pend = dz->brk[paramno] + (dz->brksize[paramno] * 2);
        while(p < pend) {
            *p = (double)round(*p * (double)dz->infile->srate);
            p += 2;
        }
    }
    return(FINISHED);
}

/*************************************** BRK_COMPARE *****************************************/

int brk_compare(double *brk1,int brksize1,double *brk2,int brksize2,int repel)
{
    int cnt1 = 0, cnt2 = 0;
    double *t1 = brk1, *t2 = brk2;
    double *v1 = brk1 + 1,  *v2 = brk2 + 1;
    double val1, val2, thistime;
    int orientation = EQUIVALENT;

    if(brksize1<2 || brksize2<2)
        return(INVALID);

    while((cnt1 < brksize1) && (cnt2 < brksize2)) {
        thistime = establish_current_time(*t1,*t2);
        if(*t1 > thistime) {
            val1 = establish_value_at_current_time(thistime,t1,brk1,v1);
            val2 = *v2;
            if((orientation = establish_orientation(val1,val2,repel))!=EQUIVALENT)
                break;
            else if(repel)
                return(CROSSED);
            if(++cnt2 < brksize2) {
                v2 += 2;
                t2 += 2;
            }
        } else if (*t2 > thistime) {
            val2 = establish_value_at_current_time(thistime,t2,brk2,v2);
            val1 = *v1;
            if((orientation = establish_orientation(val1,val2,repel))!=EQUIVALENT)
                break;
            else if(repel)
                return(CROSSED);
            if(++cnt1 < brksize1) {
                v1 += 2;
                t1 += 2;
            }
        } else {
            val1 = *v1;
            val2 = *v1;
            if((orientation = establish_orientation(val1,val2,repel))!=EQUIVALENT)
                break;
            else if(repel)
                return(CROSSED);
            if(++cnt1 < brksize1) {
                v1 += 2;
                t1 += 2;
            }
            if(++cnt2 < brksize2) {
                v2 += 2;
                t2 += 2;
            }
        }
    }
    while((cnt1 < brksize1) && (cnt2 < brksize2)) {
        thistime = establish_current_time(*t1,*t2);
        if(*t1 > thistime) {
            val1 = establish_value_at_current_time(thistime,t1,brk1,v1);
            val2 = *v2;
            switch(repel) {
            case(TRUE):
                switch(orientation) {
                case(FIRST_GREATER):  if(val2 >= val1-FLTERR)   return(CROSSED);    break;
                case(FIRST_LESS):     if(val1 >= val2-FLTERR)   return(CROSSED);    break;
                }
                break;
            case(FALSE):
                switch(orientation) {
                case(FIRST_GREATER):  if(val2 > val1)   return(CROSSED);    break;
                case(FIRST_LESS):     if(val1 > val2)   return(CROSSED);    break;
                }
                break;
            }
            cnt2++;
            v2 += 2;
            t2 += 2;
        } else if (*t2 > thistime) {
            val2 = establish_value_at_current_time(thistime,t2,brk2,v2);
            val1 = *v1;
            switch(repel) {
            case(TRUE):
                switch(orientation) {
                case(FIRST_GREATER):  if(val2 >= val1-FLTERR)   return(CROSSED);    break;
                case(FIRST_LESS):     if(val1 >= val2-FLTERR)   return(CROSSED);    break;
                }
                break;
            case(FALSE):
                switch(orientation) {
                case(FIRST_GREATER):  if(val2 > val1)   return(CROSSED);    break;
                case(FIRST_LESS):     if(val1 > val2)   return(CROSSED);    break;
                }
                break;
            }
            cnt1++;
            v1 += 2;
            t1 += 2;
        } else {
            val1 = *v1;
            val2 = *v1;
            switch(repel) {
            case(TRUE):
                switch(orientation) {
                case(FIRST_GREATER):  if(val2 >= val1-FLTERR)   return(CROSSED);    break;
                case(FIRST_LESS):     if(val1 >= val2-FLTERR)   return(CROSSED);    break;
                }
                break;
            case(FALSE):
                switch(orientation) {
                case(FIRST_GREATER):  if(val2 > val1)   return(CROSSED);    break;
                case(FIRST_LESS):     if(val1 > val2)   return(CROSSED);    break;
                }
                break;
            }
            cnt1++;
            cnt2++;
            v1 += 2;
            t1 += 2;
            v2 += 2;
            t2 += 2;
        }
    }
    return(orientation);    
}

/*************************** ESTABLISH_ORIENTATION **************************/

int establish_orientation(double val1, double val2,int repel)
{
    if(repel) {
        if(val1 < val2-FLTERR)
            return(FIRST_LESS);
        else if (val2 < val1-FLTERR)
            return(FIRST_GREATER);
    } else {
        if(val1 < val2)
            return(FIRST_LESS);
        else if (val2 < val1)
            return(FIRST_GREATER);
    }
    return(EQUIVALENT);         
}

/************************** ESTABLISH_CURRENT_TIME **************************/

double establish_current_time(double t1,double t2)
{
    if(t1 <= t2)
        return(t1);
    else
        return(t2);
}

/******************** ESTABLISH_VALUE_AT_CURRENT_TIME ***********************/

double establish_value_at_current_time(double thistime,double *t,double *brk,double *v)
{
    double *lastt, val, lastval, lasttime;
    if((lastt = t - 2) < brk)
        val = *v;
    else {
        lastval  = *(v - 2);
        lasttime = *(lastt);
        val = interpval(thistime,lastval,*v,lasttime,*t);
    }
    return(val);
}

/***************************** INTERPVAL *****************************/

double interpval(double thistime,double lastval, double nextval, double lasttime, double nexttime)
{
    double timestep = nexttime - lasttime;
    double valstep  = nextval - lastval;
    double thisstep = thistime - lasttime;
    double timeratio = thisstep/timestep;
    valstep *= timeratio;
    return(lastval + valstep);
}

/***************************** VALUE_GREATER_THAN_BRKVALS *****************************/

int value_greater_than_brkvals(double val,double *brk,int brksize,int repel)
{
    double *p = brk;
    double *pend = p + (brksize*2);
    p++;
    if(repel) {
        while(p < pend) {
            if(*p > val-FLTERR)
                return(FALSE);
            p += 2;
        }
    } else {
        while(p < pend) {
            if(*p > val)
                return(FALSE);
            p += 2;
        }
    }
    return(TRUE);
}

/***************************** VALUE_LESS_THAN_BRKVALS *****************************/

int value_less_than_brkvals(double val,double *brk,int brksize,int repel)
{
    double *p = brk;
    double *pend = p + (brksize*2);
    p++;
    if(repel) {
        while(p < pend) {
            if(*p < val+FLTERR)
                return(FALSE);
            p += 2;
        }
    } else {
        while(p < pend) {
            if(*p < val)
                return(FALSE);
            p += 2;
        }
    }
    return(TRUE);
}

/***************************** FIRST_PARAM_NOT_LESS_THAN_SECOND *****************************/

int first_param_not_less_than_second(int paramno1,int paramno2,dataptr dz)
{
    int exit_status;
    int repel = FALSE;
    if(dz->brksize[paramno1]) {
        if(dz->brksize[paramno2]) {
            if((exit_status = brk_compare
            (dz->brk[paramno1],dz->brksize[paramno1],dz->brk[paramno2],dz->brksize[paramno2],repel))<0) {
                sprintf(errstr,"Incompatible parameter values in brktables.\n");
                return(FALSE);
            }
            if(exit_status == FIRST_LESS) {
                sprintf(errstr,"Incompatible parameter values in brktables.\n");
                return(FALSE);
            }
        } else {
            if(value_greater_than_brkvals(dz->param[paramno2],dz->brk[paramno1],dz->brksize[paramno1],repel)) {
                sprintf(errstr,"Incompatible parameter values somewhere in brktable.\n");
                return(FALSE);
            }
        }
    } else if(dz->brksize[paramno2]) {
        if(value_less_than_brkvals(dz->param[paramno1],dz->brk[paramno2],dz->brksize[paramno2],repel)) {
            sprintf(errstr,"Incompatible parameter values somewhere in brktable.\n");
            return(FALSE);
        }
    } else {
        if(dz->param[paramno2] > dz->param[paramno1]) {
            sprintf(errstr,"Incompatible parameter values.\n");
            return(FALSE);
        }
    }
    return(TRUE);
}

/**************************** FIRST_PARAM_GREATER_THAN_SECOND *****************************/

int first_param_greater_than_second(int paramno1,int paramno2,dataptr dz)
{
    int exit_status;
    int repel = TRUE;
    if(dz->brksize[paramno1]) {
        if(dz->brksize[paramno2]) {
            if((exit_status = brk_compare
            (dz->brk[paramno1],dz->brksize[paramno1],dz->brk[paramno2],dz->brksize[paramno2],repel))<0) {
                sprintf(errstr,"Incompatible parameter values in brktables.\n");
                return(FALSE);
            }
            if(exit_status == FIRST_LESS) {
                sprintf(errstr,"Incompatible parameter values in brktables.\n");
                return(FALSE);
            }
        } else {
            if(!value_less_than_brkvals(dz->param[paramno2],dz->brk[paramno1],dz->brksize[paramno1],repel)) {
                sprintf(errstr,"Incompatible parameter values somewhere in brktable.\n");
                return(FALSE);
            }
        }
    } else if(dz->brksize[paramno2]) {
        if(!value_greater_than_brkvals(dz->param[paramno1],dz->brk[paramno2],dz->brksize[paramno2],repel)) {
            sprintf(errstr,"Incompatible parameter values somewhere in brktable.\n");
            return(FALSE);
        }
    } else {
        if(dz->param[paramno2] > dz->param[paramno1]-FLTERR) {
            sprintf(errstr,"Incompatible parameter values.\n");
            return(FALSE);
        }
    }
    return(TRUE);
}

/***************************** ESTABLISH_ADDITIONAL_BRKTABLE **************************/

int establish_additional_brktable(dataptr dz)
{
    int brkcnt = dz->application->total_input_param_cnt;
    if(dz->extrabrkno >= 0) {
        sprintf(errstr,"extra brktable already exists: establish_additional_brktable()\n");
        return(PROGRAM_ERROR);
    }
    dz->extrabrkno = brkcnt;              
    brkcnt++;           /* extra brktable for internal brkpntfile data */
    if(dz->extrabrkno > 0) {
        if((dz->brk      = (double **)realloc(dz->brk      ,brkcnt * sizeof(double *)))==NULL
        || (dz->brkptr   = (double **)realloc(dz->brkptr   ,brkcnt * sizeof(double)  ))==NULL
        || (dz->brksize  = (int    *)realloc(dz->brksize  ,brkcnt * sizeof(int)    ))==NULL
        || (dz->firstval = (double  *)realloc(dz->firstval ,brkcnt * sizeof(double)  ))==NULL
        || (dz->lastind  = (double  *)realloc(dz->lastind  ,brkcnt * sizeof(double)  ))==NULL
        || (dz->lastval  = (double  *)realloc(dz->lastval  ,brkcnt * sizeof(double)  ))==NULL
        || (dz->brkinit  = (int     *)realloc(dz->brkinit  ,brkcnt * sizeof(int)     ))==NULL) {
            sprintf(errstr,"establish_additional_brktable(): 1\n");
            return(MEMORY_ERROR);
        }
    } else {
        if((dz->brk      = (double **)malloc(brkcnt * sizeof(double *)))==NULL
        || (dz->brkptr   = (double **)malloc(brkcnt * sizeof(double)  ))==NULL
        || (dz->brksize  = (int    *)malloc(brkcnt * sizeof(int)    ))==NULL
        || (dz->firstval = (double  *)malloc(brkcnt * sizeof(double)  ))==NULL
        || (dz->lastind  = (double  *)malloc(brkcnt * sizeof(double)  ))==NULL
        || (dz->lastval  = (double  *)malloc(brkcnt * sizeof(double)  ))==NULL
        || (dz->brkinit  = (int     *)malloc(brkcnt * sizeof(int)     ))==NULL) {
            sprintf(errstr,"establish_additional_brktable(): 2\n");
            return(MEMORY_ERROR);
        }
    }
    dz->brk[dz->extrabrkno]       = (double *)0;
    dz->brkptr[dz->extrabrkno]    = (double *)0;
    dz->brksize[dz->extrabrkno]   = 0;
    dz->brkinit[dz->extrabrkno]   = 0;
    return(FINISHED);
}

/**************************** GET_CHANNEL_CORRESPONDING_TO_FRQ ***************************/

int get_channel_corresponding_to_frq(int *chan,double thisfrq,dataptr dz)
{   
    if(dz->chwidth <= 0.0) {
        sprintf(errstr,"chwidth not set in get_channel_corresponding_to_frq()\n");
        return(PROGRAM_ERROR);
    }
    if(thisfrq < 0.0) {
        sprintf(errstr,"-ve frequency in get_channel_corresponding_to_frq()\n");
        return(PROGRAM_ERROR);
    }
    if(thisfrq > dz->nyquist) {
        sprintf(errstr,"frequency beyond nyquist in get_channel_corresponding_to_frq()\n");
        return(PROGRAM_ERROR);
    }
    *chan = (int)((fabs(thisfrq) + dz->halfchwidth)/dz->chwidth);  /* TRUNCATE */
    if(*chan >= dz->clength) {
        sprintf(errstr,"chan (%d) beyond clength-1 (%d) returned: get_channel_corresponding_to_frq()\n",
        *chan,(dz->clength)-1);
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/**************************** RESET_FILEDATA_COUNTERS **************************/

void reset_filedata_counters(dataptr dz)
{
    dz->total_samps_written = 0;
    dz->samps_left = dz->insams[0];
    dz->total_samps_read = 0;
}

/**********************************************************************************/
/**************************** ITEMS FORMERLY IN FILETYPE **************************/
/**********************************************************************************/

/****************************** IS_A_VALID_INPUT_FILETYPE *********************************/

int is_a_valid_input_filetype(int filetype)
{
    switch(filetype) {
    case(SNDFILE):
    case(ANALFILE):
    case(PITCHFILE):
    case(TRANSPOSFILE):
    case(FORMANTFILE):
    case(ENVFILE):
    case(TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(DB_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(DB_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(NUMLIST_OR_WORDLIST):
    case(SNDLIST_OR_SYNCLIST_LINELIST_OR_WORDLIST):
    case(SNDLIST_OR_SYNCLIST_OR_WORDLIST):
    case(SNDLIST_OR_LINELIST_OR_WORDLIST):
    case(SNDLIST_OR_WORDLIST):
    case(MIXLIST_OR_LINELIST_OR_WORDLIST):
    case(MIXLIST_OR_WORDLIST):
    case(SYNCLIST_OR_LINELIST_OR_WORDLIST):
    case(SYNCLIST_OR_WORDLIST):
    case(LINELIST_OR_WORDLIST):
    case(WORDLIST):
        return(TRUE);
    }
    return(FALSE);
}

/****************************** IS_A_TEXT_INPUT_FILETYPE *********************************/

int is_a_text_input_filetype(int filetype)
{
    if(filetype & IS_A_TEXTFILE)
        return(TRUE);
    return(FALSE);
}

/****************************** COULD_BE_TRANSPOS_AND_OR_PITCH_INFILETYPE *********************************/

int could_be_transpos_and_or_pitch_infiletype(int filetype)
{
    if(filetype & IS_A_TRANSPOS_BRKFILE || filetype & IS_A_PITCH_BRKFILE)
        return(TRUE);
    return(FALSE);
}

/****************************** IS_BRKFILE_INFILETYPE *********************************/

int is_brkfile_infiletype(int filetype)
{
    if(filetype & IS_A_BRKFILE)
        return(TRUE);
    return(FALSE);
}

/****************************** IS_NUMLIST_INFILETYPE *********************************/

int is_numlist_infiletype(int filetype)
{
    if(filetype & IS_A_NUMLIST)
        return(TRUE);
    return(FALSE);
}

/****************************** IS_NUMLIST_INFILETYPE *********************************/

int is_transpos_infiletype(int filetype)
{
    if(filetype & IS_A_TRANSPOS_BRKFILE)
        return(TRUE);
    return(FALSE);
}

/****************************** IS_PITCH_INFILETYPE *********************************/

int is_pitch_infiletype(int filetype)
{
    if(filetype & IS_A_PITCH_BRKFILE)
        return(TRUE);
    return(FALSE);
}

/****************************** IS_SNDLIST_ONLY_INFILETYPE *********************************/

int is_sndlist_only_infiletype(int filetype)
{
    switch(filetype) {
    case(SNDLIST_OR_LINELIST_OR_WORDLIST):
    case(SNDLIST_OR_WORDLIST):
        return(TRUE);
    }
    return(FALSE);
}

/******************************* FILE_HAS_INVALID_STARTCHAR ****************************/
/* RWD 4:2002 need this until readdata is fixed (reading numbers as text filenames) */

int file_has_invalid_startchar(char *filename)
{
    char *p;
    int filenamelen = strlen(filename);
//TW AGREED TO DELETION n, has_point
    if(filenamelen <= 0)
        return 1;
    p = filename + filenamelen - 1;
    switch(filename[0]) {
    case(ENDOFSTR): 
        return 1;
    case('.'):
        if(filenamelen <3)
            return 1;
        switch(filename[1]) {
        case('/'):                                  /* allow ".\" 'current directory' shorthand */
        case('\\'):
            p = get_last_slash(filename,filenamelen);
            p++;
//TW MODIFIED TO ALLOW NUMERIC FILENAMES
//          if(strlen(p) <=0 || isdigit(*p) || (*p == '-'))
            if(strlen(p) <=0 || (*p == '-'))
                return 1;
            break;
        case('.'):
            if(filenamelen <4)
                return 1;
            switch(filename[2]) {
            case('/'):                              /* allow "..\ 'other directory' shorthand */
            case('\\'):
                p = get_last_slash(filename,filenamelen);
                p++;
//TW MODIFIED TO ALLOW NUMERIC FILENAMES
//              if(strlen(p) <=0  || isdigit(*p) || (*p == '-'))
                if(strlen(p) <=0 || (*p == '-'))
                    return 1;
                break;
            default:
                return 1;
            }
            break;
        default:
            return 1;
        }
        break;
//TW MODIFIED TO ALLOW NUMERIC FILENAMES, but stop '-' (which playcmd may interpet as flag)
    case('-'):
        return 1;
    }
    return 0;
}

/******************************* GET_LAST_SLASH ****************************/

char *get_last_slash(char *filename,int filenamelen)
{
    char *p = filename;
    p += filenamelen - 1;
    while(p >= filename) {
        if(*p == '/' || *p == '\\')
            return p;
        p--;
    }
    return p;   /* NOTREACHED */
}

/******************************* DERIVED_FILETYPE ****************************/

int derived_filetype(int filetype)
{
    switch(filetype) {
    case(PITCHFILE):
    case(TRANSPOSFILE):
    case(FORMANTFILE):
        return(TRUE);
    }
    return(FALSE);
}

/******************************* FILE_HAS_RESERVED_EXTENSION ****************************/

int file_has_reserved_extension(char *filename)
{
    char *p = strrchr(filename, '.');
    if(p == NULL)
        return 0;
    p++;
    if(!_stricmp(p,"frq") 
    || !_stricmp(p,"for") 
    || !_stricmp(p,"trn") 
    || !_stricmp(p,"evl") 
    || !_stricmp(p,"ana") 
    || !_stricmp(p,"wav") 
    || !_stricmp(p,"aiff") 
    || !_stricmp(p,"aifc") 
    || !_stricmp(p,"aif"))
        return 1;
    while(*p != ENDOFSTR) { /* numeric extensions are patches */
        if(!isdigit(*p))
            return 0;
        p++;
    }
    return 1;
}

/****************************** IS_A_TEXTFILE_TYPE *******************************/

int is_a_textfile_type(int filetype)    /* after type conversion: not at input */
{
    if(filetype==BRKFILE
    || filetype==DB_BRKFILE
    || filetype==UNRANGED_BRKFILE
    || filetype==NUMLIST
    || filetype==SNDLIST
    || filetype==SYNCLIST
    || filetype==MIXFILE
    || filetype==LINELIST
    || filetype==WORDLIST)
        return(TRUE);
    return(FALSE);
}

/***************************** FILE_HAS_INVALID_EXTENSION ****************************************/

int file_has_invalid_extension(char *filename)
{
    char *p;
    int len = strlen(filename);
    if(len <=0)
        return 0;
    p = strrchr(filename, '.');
    if(p == 0)
        return 0;
    p++;
    if(*p == '\\' || *p == '/')     /*  allow .\abc OR ../abc */
        return 0;
    while(*p != ENDOFSTR) {
        if(!isdigit(*p))            /* .123 is a patchfile : not permitted */
            return 0;
        p++;
    }
    return 1;
}

/**********************************************************************************/
/*********************** ITEMS FORMERLY IN DEFAULTS.C tklib2 **********************/
/**********************************************************************************/

/****************************** INITIALISE_PARAM_VALUES *********************************/
/*RWD NB we ignore infilesize and use insams ONLY */
int initialise_param_values(int process,int mode,int channels,double nyquist,float frametime,
    int insams,int srate,int wanted,int linecnt,double duration,double *default_val,int filetype,aplptr ap)
{
    int    n;
    int    clength       = (int)(wanted/2);
    double chwidth       = nyquist/(double)(clength-1);
    double halfchwidth   = chwidth/2.0;
    double sr            = (double)srate;

        /******************************* SPEC *******************************/
        /******************************* SPEC *******************************/
        /******************************* SPEC *******************************/

    switch(process) {
    case(ACCU):
        default_val[ACCU_DINDEX]    = 1.0;
        default_val[ACCU_GINDEX]    = 0.0;
        break;
    case(ALT):      
        break;
    case(ARPE):
        default_val[ARPE_WTYPE]     = SIN;
        default_val[ARPE_ARPFRQ]    = 1.0;

        if(mode==ABOVE_BOOST || mode==ONCE_ABOVE)
            default_val[ARPE_PHASE] = 0.51;
        else
            default_val[ARPE_PHASE] = 0.0;



        default_val[ARPE_LOFRQ]     = PITCHZERO;
        default_val[ARPE_HIFRQ]     = nyquist;
        default_val[ARPE_HBAND]     = nyquist/(double)(clength-1);
        default_val[ARPE_AMPL]      = DEFAULT_ARPE_AMPLIF;
        if(mode==ON
        || mode==BOOST
        || mode==BELOW_BOOST
        || mode==ABOVE_BOOST) {
            default_val[ARPE_NONLIN]= 1.0;
            default_val[ARPE_SUST]  = DEFAULT_ARPE_SUSTAIN;
        }
        break;
    case(AVRG):
        default_val[AVRG_AVRG]      = DEFAULT_AVRG;
        break;
    case(BARE):
        break;
    case(BLUR):
        default_val[BLUR_BLURF]     = DEFAULT_BLUR;
        break;
    case(BLTR):
        default_val[BLUR_BLURF]     = DEFAULT_BLUR;
        default_val[BLTR_TRACE]     = DEFAULT_TRACE;
        break;
    case(BRIDGE):
        default_val[BRG_OFFSET]     = 0.0;
        default_val[BRG_SF2]        = 0.0;
        default_val[BRG_SA2]        = 0.0;
        default_val[BRG_EF2]        = 1.0;
        default_val[BRG_EA2]        = 1.0;
        default_val[BRG_STIME]      = 0.0;
        default_val[BRG_ETIME]      = frametime * 2.0;
        break;
    case(CHANNEL):
        default_val[CHAN_FRQ]       = CONCERT_A;
        break;
    case(CHORD):
    case(MULTRANS):
        default_val[CHORD_HIFRQ]    = nyquist;
        default_val[CHORD_LOFRQ]    = PITCHZERO;
        break;
    case(CHORUS):
        if(mode==CH_AMP
        || mode==CH_AMP_FRQ 
        || mode==CH_AMP_FRQ_UP 
        || mode==CH_AMP_FRQ_DN)
            default_val[CHORU_AMPR] = DEFAULT_CHORU_AMPSPREAD;
        if(mode==CH_FRQ
        || mode==CH_FRQ_UP
        || mode==CH_FRQ_DN
        || mode==CH_AMP_FRQ
        || mode==CH_AMP_FRQ_UP
        || mode==CH_AMP_FRQ_DN)
            default_val[CHORU_FRQR] = DEFAULT_CHORU_FRQSPREAD;
        break;
    case(CLEAN):
        switch(mode) {
        case(FROMTIME):
        case(ANYWHERE):
            default_val[CL_SKIPT]   = 0.0;
            break;
        case(FILTERING):
            default_val[CL_FRQ]     = PITCHZERO;
            break;
        }
        default_val[CL_GAIN]        = DEFAULT_NOISEGAIN;
        break;
    case(CROSS):
        default_val[CROS_INTP]      = 1.0;
        break;
    case(CUT):
        default_val[CUT_STIME]      = 0.0;
        default_val[CUT_ETIME]      = duration;
        break;
    case(DIFF):
        default_val[DIFF_CROSS]     = 1.0;
        break;
    case(DRUNK):
        default_val[DRNK_RANGE]     = DEFAULT_MAX_DRUNK_STEP;
        default_val[DRNK_STIME]     = 0.0;
        default_val[DRNK_DUR]       = DEFAULT_DURATION;
        break;
    case(EXAG):
        default_val[EXAG_EXAG]      = DEFAULT_EXAG;
        break;
    case(FILT):
        switch(mode) {
        case(F_HI): 
        case(F_HI_NORM):
            default_val[FILT_FRQ1]  = SPEC_MINFRQ;
            default_val[FILT_QQ]    = DEFAULT_Q; 
            break;
        case(F_LO): 
        case(F_LO_NORM):
            default_val[FILT_FRQ1]  = DEFAULT_FILT_FRQ;
            default_val[FILT_QQ]    = DEFAULT_Q; 
            break;
        case(F_HI_GAIN):
            default_val[FILT_FRQ1]  = SPEC_MINFRQ;
            default_val[FILT_QQ]    = DEFAULT_Q; 
            default_val[FILT_PG]    = 1.0;
            break;
        case(F_LO_GAIN):
            default_val[FILT_FRQ1]  = DEFAULT_FILT_FRQ;
            default_val[FILT_QQ]    = DEFAULT_Q; 
            default_val[FILT_PG]    = 1.0;
            break;
        case(F_BND):
        case(F_BND_NORM):
        case(F_NOTCH):
        case(F_NOTCH_NORM):
            default_val[FILT_FRQ1]  = SPEC_MINFRQ;
            default_val[FILT_FRQ2]  = DEFAULT_FILT_FRQ;
            default_val[FILT_QQ]    = DEFAULT_Q; 
            break;
        case(F_BAND_GAIN):  
        case(F_NOTCH_GAIN):
            default_val[FILT_FRQ1]  = SPEC_MINFRQ;
            default_val[FILT_FRQ2]  = DEFAULT_FILT_FRQ;
            default_val[FILT_QQ]    = DEFAULT_Q; 
            default_val[FILT_PG]    = 1.0;
            break;
        }
        break;
    case(FMNTSEE):  
    case(FORMANTS): 
    case(FORMSEE):  
        break;
    case(FOCUS):
        default_val[FOCU_PKCNT]     = (double)MAXPKCNT;
        default_val[FOCU_BW]        = DEFAULT_OCTAVE_BWIDTH;
        default_val[FOCU_LOFRQ]     = PITCHZERO;
        default_val[FOCU_HIFRQ]     = nyquist;
        default_val[FOCU_STABL]     = DEFAULT_STABILITY;
        break;
    case(FOLD):
        default_val[FOLD_LOFRQ]     = PITCHZERO;
        default_val[FOLD_HIFRQ]     = nyquist;
        break;
    case(FORM):
        default_val[FORM_FTOP]      = nyquist;
        default_val[FORM_FBOT]      = PITCHZERO;
        default_val[FORM_GAIN]      = 1.0;
        break;
    case(FREEZE):   
    case(FREEZE2):  
        break;
    case(FREQUENCY):
        default_val[FRQ_CHAN]       = 0;
        break;
    case(GAIN):
        default_val[GAIN_GAIN]      = 1.0;
        break;
    case(GLIDE):
        default_val[GLIDE_DUR]      = DEFAULT_DURATION;
        break;
    case(GLIS):
        default_val[GLIS_RATE]      = DEFAULT_GLISRATE;
        default_val[GLIS_HIFRQ]     = nyquist;
        if(mode==INHARMONIC)
            default_val[GLIS_SHIFT] = GLIS_SHIFT_DEFAULT;
        break;                                                           
    case(GRAB):
        default_val[GRAB_FRZTIME]   = duration/2;
        break;
    case(GREQ):
        break;
    case(INVERT):
        break;
    case(LEAF):
        default_val[LEAF_SIZE]      = 1.0;
        break;
    case(LEVEL):
        break;
    case(MAGNIFY):
        default_val[MAG_FRZTIME]    = duration/2;
        default_val[MAG_DUR]        = DEFAULT_DURATION;
        break;
    case(MAKE):
        break;
    case(MAX):
        break;
    case(MEAN):
        default_val[MEAN_LOF]       = PITCHZERO;
        default_val[MEAN_HIF]       = nyquist;
        default_val[MEAN_CHAN]      = clength;
        break;
    case(MORPH):
        default_val[MPH_ASTT]       = 0.0;
        default_val[MPH_AEND]       = frametime * 2;
        default_val[MPH_FSTT]       = 0.0;
        default_val[MPH_FEND]       = frametime * 2;
        default_val[MPH_AEXP]       = 1.0;
        default_val[MPH_FEXP]       = 1.0;
        default_val[MPH_STAG]       = 0.0;
        break;
    case(NOISE):
        default_val[NOISE_NOIS]     = 0.0;
        break;
    case(OCT):
        default_val[OCT_HMOVE]      = DEFAULT_OCT_TRANSPOS; 
        default_val[OCT_BREI]       = 0.0;
        break;
    case(OCTVU):
        default_val[OCTVU_TSTEP]    = DEFAULT_TIME_STEP;
        default_val[OCTVU_FUND]     = max(halfchwidth,exp(log(nyquist/ROOT_2)/(double)DEFAULT_OCTBANDS));
        break;
    case(P_APPROX):
        default_val[PA_PRANG]       = DEFAULT_PRANGE;
        default_val[PA_TRANG]       = DEFAULT_TRANGE;
        default_val[PA_SRANG]       = DEFAULT_SRANGE;
        break;
    case(P_CUT):
        default_val[PC_STT]         = 0.0;
        default_val[PC_END]         = duration;
        break;
    case(P_EXAG):
        switch(mode) {
        case(RANGE_ONLY_TO_P):
        case(RANGE_ONLY_TO_T):
        case(R_AND_C_TO_P):
        case(R_AND_C_TO_T):
            default_val[PEX_RANG]   = 1.0;
            /* fall thro */
        default:
            default_val[PEX_MEAN]   = MIDIMIN;
            default_val[PEX_CNTR]   = 0.0;
        }
        break;
    case(P_FIX):
        default_val[PF_SCUT]        = 0.0;
        default_val[PF_ECUT]        = duration - FLTERR;
        default_val[PF_LOF]         = PITCHZERO;
        default_val[PF_HIF]         = nyquist;
        default_val[PF_SMOOTH]      = 1.0;
        default_val[PF_SMARK]       = SPEC_MINFRQ;
        default_val[PF_EMARK]       = SPEC_MINFRQ;
        break;
    case(P_HEAR):
        default_val[PH_GAIN]        = 1.0;
        break;
    case(P_INFO):
        break;
    case(P_INVERT):
        hztomidi(&(default_val[PI_MEAN]),SPEC_MINFRQ);
        default_val[PI_TOP]         = MIDIMAX;
        hztomidi(&(default_val[PI_BOT]),SPEC_MINFRQ);
        break;
    case(P_QUANTISE):   
        break;
    case(P_RANDOMISE):
        default_val[PR_MXINT]       = 1.0;
        default_val[PR_TSTEP]       = (frametime * SECS_TO_MS) + FLTERR;
        default_val[PR_SLEW]        = DEFAULT_SLEW;
        break;
    case(P_SEE):
// TW Default Scaling value altered to give range 0-1 out
//      default_val[PSEE_SCF]       = 1.0;
        default_val[PSEE_SCF]       = 1.0/nyquist;
        break;
    case(P_SMOOTH):
        default_val[PS_TFRAME]      = (frametime * SECS_TO_MS) + FLTERR;
        hztomidi(&(default_val[PS_MEAN]),SPEC_MINFRQ);
        break;
//TW NEW CASES
    case(P_INSERT):
    case(P_SINSERT):
    case(P_PTOSIL):
    case(P_NTOSIL):
    case(P_SYNTH):
    case(ANALENV):
    case(P_BINTOBRK):
    case(MAKE2):
    case(P_INTERP):
        break;
    case(P_VOWELS):
        default_val[PV_HWIDTH] = V_HWIDTH;
        default_val[PV_CURVIT] = CURVIT;
        default_val[PV_PKRANG] = PEAK_RANGE;
        default_val[PV_FUNBAS] = FUNDAMENTAL_BASE;
        default_val[PV_OFFSET] = 0.0;
        break;
    case(VFILT):
        default_val[PV_HWIDTH] = V_HWIDTH;
        default_val[PV_CURVIT] = CURVIT;
        default_val[PV_PKRANG] = PEAK_RANGE;
        default_val[VF_THRESH] = 0.5;
        break;
    case(P_GEN):
        default_val[PGEN_SRATE] = 48000;
        default_val[PGEN_CHANS_INPUT] = 1024;
        default_val[PGEN_WINOVLP_INPUT] = 3;
        break;

    case(P_TRANSPOSE):
        default_val[PT_TVAL]        = 0.0;
        break;
    case(P_VIBRATO):
        default_val[PV_FRQ]         = DEFAULT_VIBRATO_FRQ;
        default_val[PV_RANG]        = DEFAULT_VIBRATO_RANGE;
        break;
    case(P_WRITE):
        default_val[PW_DRED]        = LOG2(EIGHTH_TONE) * SEMITONES_PER_OCTAVE;
        break;
    case(P_ZEROS):      
        break;
    case(PEAK):
        default_val[PEAK_CUTOFF]    = SPEC_MINFRQ;
        default_val[PEAK_TWINDOW]   = DEFAULT_TWINDOW;
        default_val[PEAK_FWINDOW]   = DEFAULT_FWINDOW;
        break;
    case(PICK):
        default_val[PICK_FUND]      = SPEC_MINFRQ;
        default_val[PICK_LIN]       = SPEC_MINFRQ;
        default_val[PICK_CLAR]      = 1.0;
        break;
    case(PITCH):
        default_val[PICH_RNGE]      = 1.0;
        default_val[PICH_VALID]     = (double)BLIPLEN;
        default_val[PICH_SRATIO]    = SILENCE_RATIO;
        default_val[PICH_MATCH]     = (double)ACCEPTABLE_MATCH;
        default_val[PICH_HILM]      = nyquist/MAXIMI;
        default_val[PICH_LOLM]      = SPEC_MINFRQ;
        if(mode==PICH_TO_BRK)
            default_val[PICH_DATAREDUCE]= LOG2(EIGHTH_TONE) * SEMITONES_PER_OCTAVE;
        break;
    case(PLUCK):
        default_val[PLUK_GAIN]      = 1.0;
        break;
    case(PRINT):
        default_val[PRNT_STIME]     = 0.0;
        default_val[PRNT_WCNT]      = 1.0;
        break;
    case(REPITCH):      
        break;
    case(REPITCHB):
        default_val[RP_DRED]        = LOG2(EIGHTH_TONE) * SEMITONES_PER_OCTAVE;
        break;
    case(REPORT):
        default_val[REPORT_PKCNT]   = (double)MAXPKCNT;
        default_val[REPORT_LOFRQ]   = PITCHZERO;
        default_val[REPORT_HIFRQ]   = nyquist;
        default_val[REPORT_STABL]   = DEFAULT_STABILITY;
        break;
    case(SCAT):
        default_val[SCAT_CNT]       = max((double)(clength/4),16.0);
        default_val[SCAT_BLOKSIZE]  = chwidth;
        break;
    case(SHIFT):
        default_val[SHIFT_SHIF]     = 0.0;
        default_val[SHIFT_FRQ1]     = halfchwidth/2.0;
        default_val[SHIFT_FRQ2]     = nyquist - (halfchwidth/2.0);
        break;
    case(SHIFTP):
        default_val[SHIFTP_FFRQ]    = CONCERT_A;
        default_val[SHIFTP_SHF1]    = 0.0;
        default_val[SHIFTP_SHF2]    = 0.0;
        default_val[SHIFTP_DEPTH]   = 1.0;
        break;
    case(SHUFFLE):
        default_val[SHUF_GRPSIZE]   = 1.0;
        break;
    case(SPLIT):        
        break;
    case(SPREAD):
        default_val[SPREAD_SPRD]    = 1.0;
        break;
    case(STEP):
        default_val[STEP_STEP]      = DEFAULT_STEP;
        break;
    case(STRETCH):
        default_val[STR_FFRQ]       = CONCERT_A;
        default_val[STR_SHIFT]      = 1.4;
        default_val[STR_EXP]        = 1.0;
        default_val[STR_DEPTH]      = 1.0;
        break;
    case(SUM):
        default_val[SUM_CROSS]      = 1.0;
        break;
    case(SUPR):
        default_val[SUPR_INDX]      = DEFAULT_TRACE;
        break;
    case(S_TRACE):
        default_val[TRAC_INDX]      = DEFAULT_TRACE;
        default_val[TRAC_LOFRQ]     = PITCHZERO;
        default_val[TRAC_HIFRQ]     = nyquist;
        break;
    case(TRACK):
        default_val[TRAK_PICH]      = SPEC_MINFRQ;
        default_val[TRAK_RNGE]      = 1.0;
        default_val[TRAK_VALID]     = (double)BLIPLEN;
        default_val[TRAK_SRATIO]    = SILENCE_RATIO;
        default_val[TRAK_HILM]      = nyquist/MAXIMI;
        if(mode==TRK_TO_BRK)
            default_val[TRAK_DATAREDUCE]= LOG2(EIGHTH_TONE) * SEMITONES_PER_OCTAVE;
        break;
    case(TRNSF):
        default_val[TRNSF_HIFRQ]    = nyquist;
        default_val[TRNSF_LOFRQ]    = PITCHZERO;
        break;
    case(TRNSP):
        default_val[TRNSP_HIFRQ]    = nyquist;
        default_val[TRNSP_LOFRQ]    = PITCHZERO;
        break;
    case(TSTRETCH):
        default_val[TSTR_STRETCH]   = 1.0;
        break;
    case(TUNE):
        default_val[TUNE_FOC]       = 1.0;
        default_val[TUNE_CLAR]      = 1.0;
        default_val[TUNE_INDX]      = DEFAULT_TRACE;
        default_val[TUNE_BFRQ]      = SPEC_MINFRQ;
        break;
    case(VOCODE):
        default_val[VOCO_LOF]       = PITCHZERO;
        default_val[VOCO_HIF]       = nyquist;
        default_val[VOCO_GAIN]      = 1.0;
        break;
    case(WARP):
        default_val[WARP_PRNG]      = DEFAULT_PRANGE;
        default_val[WARP_TRNG]      = DEFAULT_TRANGE;
        default_val[WARP_SRNG]      = DEFAULT_SRANGE;
        break;
    case(WAVER):
        default_val[WAVER_VIB]      = DEFAULT_VIBRATO_FRQ;
        default_val[WAVER_STR]      = 1.0;
        default_val[WAVER_LOFRQ]    = PITCHZERO;
        if(mode==WAVER_SPECIFIED)
            default_val[WAVER_EXP]  = 1.0;
        break;
    case(WEAVE):    
        break;
    case(WINDOWCNT):
        break;
    case(LIMIT):
        default_val[LIMIT_THRESH]   = 0.0;
        break;

        /******************************* GROUCHO *******************************/
        /******************************* GROUCHO *******************************/
        /******************************* GROUCHO *******************************/

    case(DISTORT):
        default_val[DISTORT_POWFAC]     = 2.0;
        break;
    case(DISTORT_ENV):
        default_val[DISTORTE_CYCLECNT]  = 1.0;
        default_val[DISTORTE_TROF]      = 0.0;
        default_val[DISTORTE_EXPON]     = 1.0;
        break;
    case(DISTORT_AVG):
        default_val[DISTORTA_CYCLECNT]  = 2.0;
        default_val[DISTORTA_MAXLEN]    = MAXWAVELEN;
        default_val[DISTORTA_SKIPCNT]   = 0.0;
        break;
    case(DISTORT_OMT):
        default_val[DISTORTO_OMIT]      = 1.0;
        default_val[DISTORTO_KEEP]      = 2.0;
        break;
    case(DISTORT_MLT):
    case(DISTORT_DIV):
        default_val[DISTORTM_FACTOR]    = 2.0;
        break;
    case(DISTORT_HRM):
        default_val[DISTORTH_PRESCALE]  = 1.0;
        break;
    case(DISTORT_FRC):
        default_val[DISTORTF_SCALE]     = MIN_SCALE;
        default_val[DISTORTF_AMPFACT]   = 1.0;
        default_val[DISTORTF_PRESCALE]  = 1.0;
        break;
    case(DISTORT_REV):
        default_val[DISTORTR_CYCLECNT]  = 1.0;
        break;
    case(DISTORT_SHUF):
        default_val[DISTORTS_CYCLECNT]  = 1.0;
        default_val[DISTORTS_SKIPCNT]   = 0.0;
        break;
    case(DISTORT_RPTFL):
        default_val[DISTRPT_CYCLIM]     = CYCLIM_DFLTFRQ;
        /* fall thro */
    case(DISTORT_RPT):
    case(DISTORT_RPT2):
        default_val[DISTRPT_MULTIPLY]   = 2.0;
        default_val[DISTRPT_CYCLECNT]   = 1.0;
        default_val[DISTRPT_SKIPCNT]    = 0.0;
        break;
    case(DISTORT_INTP):
        default_val[DISTINTP_MULTIPLY]  = 2.0;
        default_val[DISTINTP_SKIPCNT]   = 0.0;
        break;
    case(DISTORT_DEL):
        default_val[DISTDEL_CYCLECNT]   = 2.0;
        default_val[DISTDEL_SKIPCNT]    = 0.0;
        break;
    case(DISTORT_RPL):
        default_val[DISTRPL_CYCLECNT]   = 2.0;
        default_val[DISTRPL_SKIPCNT]    = 0.0;
        break;
    case(DISTORT_TEL):
        default_val[DISTTEL_CYCLECNT]   = 2.0;
        default_val[DISTTEL_SKIPCNT]    = 0.0;
        break;
    case(DISTORT_FLT):
        default_val[DISTFLT_LOFRQ_CYCLELEN] = SPEC_MINFRQ;
        default_val[DISTFLT_HIFRQ_CYCLELEN] = nyquist;
        default_val[DISTFLT_SKIPCNT]          = 0.0;
        break;
    case(DISTORT_INT):
        break;
    case(DISTORT_CYCLECNT):
        break;
    case(DISTORT_PCH):
        default_val[DISTPCH_OCTVAR]     = 1.0;
        default_val[DISTPCH_CYCLECNT]   = DEFAULT_RSTEP;
        default_val[DISTPCH_SKIPCNT]    = 0.0;
        break;
    case(DISTORT_OVERLOAD):
        if(mode==OVER_SINE) {
            default_val[DISTORTER_FRQ]  = CONCERT_A;
        }
        default_val[DISTORTER_MULT]     = DFLT_DISTORTER_MULT;
        default_val[DISTORTER_DEPTH]    = DFLT_DISTORTER_DEPTH;
        break;
//TW NEW CASE
    case(DISTORT_PULSED):
        default_val[PULSE_STARTTIME]    = 0.0;  /* starttime of impulses in input-sound */
        default_val[PULSE_DUR]          = 1.0;  /* duration of impulse stream */
        default_val[PULSE_FRQ]          = 10.0; /* frq of impulses */
        default_val[PULSE_FRQRAND]      = 0.0;  /* randomisation frq of impulses, in semitones */
        default_val[PULSE_TIMERAND]     = 0.0;  /* randomisation of pulse shape, timewise */
        default_val[PULSE_SHAPERAND]    = 0.0;  /* randomisation of pulse shape, ampwise  */
        if(mode==PULSE_SYNI)
            default_val[PULSE_WAVETIME] = 1.0;  /* number of wavesets to cycle-around, within impulse [synth2 option only] */
        else
            default_val[PULSE_WAVETIME] = .02;  /* duration of wavesets to cycle-around, within impulse [synth option only] */
        default_val[PULSE_TRANSPOS]     = 0.0;  /* transposition envelope of material inside impulse */
        default_val[PULSE_PITCHRAND]    = 0.0;  /* randomisation of transposition envelope */
        break;

    case(ZIGZAG):
        default_val[ZIGZAG_START]       = 0.0;
        default_val[ZIGZAG_END]         = duration;
        default_val[ZIGZAG_DUR]         = duration * 2.0;
        default_val[ZIGZAG_MIN]         = ((ZIG_SPLICELEN * 2) + ZIG_MIN_UNSPLICED) * MS_TO_SECS;
        default_val[ZIGZAG_SPLEN]       = ZIG_SPLICELEN;
        if(mode==ZIGZAG_SELF) {
            default_val[ZIGZAG_MAX]     = min(2.0,duration - (2 * ZIG_SPLICELEN * MS_TO_SECS));
            default_val[ZIGZAG_RSEED]   = 0.0;
        }
        break;
    case(LOOP):
        default_val[LOOP_OUTDUR]        = duration;
        default_val[LOOP_REPETS]        = 2;
        default_val[LOOP_START]         = 0.0;
        default_val[LOOP_LEN]           = max(DEFAULT_LPSTEP,((ZIG_SPLICELEN * 2) + ZIG_MIN_UNSPLICED));
        if(mode==LOOP_ALL)
            default_val[LOOP_STEP]      = max(DEFAULT_LPSTEP/2,((ZIG_SPLICELEN * 2) + ZIG_MIN_UNSPLICED));
        else
            default_val[LOOP_STEP]      = 0.0;
        default_val[LOOP_SPLEN]         = ZIG_SPLICELEN;
        default_val[LOOP_SRCHF]         = 0.0;
        break;
    case(SCRAMBLE):
        switch(mode) {
        case(SCRAMBLE_RAND):
            default_val[SCRAMBLE_MIN]   = ((2 * ZIG_SPLICELEN) + ZIG_MIN_UNSPLICED) * MS_TO_SECS;
            default_val[SCRAMBLE_MAX]   = max((duration/4.0),default_val[SCRAMBLE_MIN]);
            break;
        case(SCRAMBLE_SHRED):
            default_val[SCRAMBLE_LEN]   = max((duration/4.0),((2 * ZIG_SPLICELEN) + ZIG_MIN_UNSPLICED) * MS_TO_SECS);
            default_val[SCRAMBLE_SCAT]  = 0.0;
            break;
        default:
            sprintf(errstr,"Unknown mode for SCRAMBLE: in initialise_param_values()\n");
            return(PROGRAM_ERROR);
        }
        default_val[SCRAMBLE_DUR]       = duration;
        default_val[SCRAMBLE_SPLEN]     = ZIG_SPLICELEN;
        default_val[SCRAMBLE_SEED]      = 0.0;
        break;
    case(ITERATE):
        switch(mode) {
        case(ITERATE_DUR):
            default_val[ITER_DUR]       = duration * 2.0;
            break;
        case(ITERATE_REPEATS): 
            default_val[ITER_REPEATS]   = 2.0;
            break;
        default:
            sprintf(errstr,"Unknown mode for ITERATE: in initialise_param_values()\n");
            return(PROGRAM_ERROR);
        }
        default_val[ITER_DELAY]         = duration;
        default_val[ITER_RANDOM]        = 0.0;
        default_val[ITER_PSCAT]         = 0.0;
        default_val[ITER_ASCAT]         = 0.0;
        default_val[ITER_FADE]          = 0.0;
        default_val[ITER_RSEED]         = 0.0;
        default_val[ITER_GAIN]          = DEFAULT_ITER_GAIN; /* 0.0 */ 
        break;
    case(ITERATE_EXTEND):
        switch(mode) {
        case(ITERATE_DUR):     
            default_val[ITER_DUR]       = duration * 2.0;
            break;
        case(ITERATE_REPEATS): 
            default_val[ITER_REPEATS]   = 2.0;
            break;
        default:
            sprintf(errstr,"Unknown mode for ITERATE_EXTEND: in initialise_param_values()\n");
            return(PROGRAM_ERROR);
        }
        default_val[ITER_DELAY]         = duration;
        default_val[ITER_RANDOM]        = 0.0;
        default_val[ITER_PSCAT]         = 0.0;
        default_val[ITER_ASCAT]         = 0.0;
        default_val[CHUNKSTART]         = 0.0;
        default_val[CHUNKEND]           = duration;
        default_val[ITER_LGAIN]         = 1.0;
        default_val[ITER_RRSEED]        = 0.0;
        break;
    case(DRUNKWALK):
        default_val[DRNK_TOTALDUR]      = duration;
        default_val[DRNK_LOCUS]         = 0.0;
        default_val[DRNK_AMBITUS]       = min(1.0,duration);
        default_val[DRNK_GSTEP]         = DRNK_GRAIN * DRNK_DEFAULT_GSTEP;
        default_val[DRNK_CLOKTIK]       = DRNK_SPLICE * MS_TO_SECS * DRNK_DEFAULT_CLOKTIK;
        default_val[DRNK_MIN_DRNKTIK]   = DEFAULT_MIN_DRNKTIK;
        default_val[DRNK_MAX_DRNKTIK]   = DEFAULT_MAX_DRNKTIK;

        default_val[DRNK_SPLICELEN]     = DRNK_SPLICE;
        default_val[DRNK_CLOKRND]       = 0.0;
        default_val[DRNK_OVERLAP]       = 0.0;
        default_val[DRNK_RSEED]         = 0.0;
        if(mode==HAS_SOBER_MOMENTS) {
            default_val[DRNK_MIN_PAUS]  = 0.25;
            default_val[DRNK_MAX_PAUS]  = min(1.5,duration + FLTERR);
        }
        break;

    case(SIMPLE_TEX):   case(TIMED):    case(GROUPS):   case(TGROUPS):
    case(DECORATED):    case(PREDECOR): case(POSTDECOR):
    case(ORNATE):       case(PREORNATE):case(POSTORNATE):
    case(MOTIFS):       case(MOTIFSIN): case(TMOTIFS):  case(TMOTIFSIN):
        default_val[TEXTURE_DUR]        = TEXTURE_DEFAULT_DUR;

        switch(process) {
        case(SIMPLE_TEX):   case(GROUPS):   case(MOTIFS):   case(MOTIFSIN):
            default_val[TEXTURE_PACK]   = DENSITY_DEFAULT;
            break;
        case(TIMED):        case(TGROUPS):      case(TMOTIFS):      case(TMOTIFSIN):
        case(DECORATED):    case(PREDECOR):     case(POSTDECOR):    
        case(ORNATE):       case(PREORNATE):    case(POSTORNATE):
            default_val[TEXTURE_SKIP]   = DEFAULT_SKIP;
            break;
        default:
            sprintf(errstr,"Unknown process in initialise_param_values()\n");
            return(PROGRAM_ERROR);
        }
        default_val[TEXTURE_SCAT]       = 0.0;
        default_val[TEXTURE_TGRID]      = 0.0;
        default_val[TEXTURE_INSLO]      = 1.0;
        default_val[TEXTURE_INSHI]      = 1.0;
        default_val[TEXTURE_MAXAMP]     = 64.0;
        default_val[TEXTURE_MINAMP]     = 64.0;
        default_val[TEXTURE_MAXDUR]     = min(ap->hi[TEXTURE_MAXDUR],TEXTURE_MAX_DUR);
        default_val[TEXTURE_MINDUR]     = max(ap->lo[TEXTURE_MINDUR],TEXTURE_MIN_DUR);
        default_val[TEXTURE_MAXPICH]    = DEFAULT_PITCH;
        default_val[TEXTURE_MINPICH]    = DEFAULT_PITCH;
        default_val[TEX_PHGRID]         = 0.0;
        default_val[TEX_GPSPACE]        = (double)IS_STILL;
        default_val[TEX_GRPSPRANGE]     = 0.0;
        default_val[TEX_AMPRISE]        = 0.0;
        default_val[TEX_AMPCONT]        = 0.0;
        default_val[TEX_GPSIZELO]       = DEFAULT_GPSIZE;
        default_val[TEX_GPSIZEHI]       = DEFAULT_GPSIZE;
        default_val[TEX_GPPACKLO]       = DENSITY_DEFAULT * SECS_TO_MS;
        default_val[TEX_GPPACKHI]       = DENSITY_DEFAULT * SECS_TO_MS;
        if(mode==TEX_NEUTRAL) { /* midipitches */
            default_val[TEX_GPRANGLO]   = DEFAULT_PITCH - HALF_OCTAVE;
            default_val[TEX_GPRANGHI]   = DEFAULT_PITCH + HALF_OCTAVE;
        } else { /* notes of hfield */
            default_val[TEX_GPRANGLO]   = DEFAULT_HF_GPRANGE;
            default_val[TEX_GPRANGHI]   = DEFAULT_HF_GPRANGE;
        }
        default_val[TEX_MULTLO]         = 1.0;
        default_val[TEX_MULTHI]         = 1.0;
        default_val[TEX_DECPCENTRE]     = (double)DEC_CENTRED;
        default_val[TEXTURE_ATTEN]      = 1.0;
        default_val[TEXTURE_POS]        = TEX_CENTRE;
        default_val[TEXTURE_SPRD]       = MAX_SPREAD;
        default_val[TEXTURE_SEED]       = 0.0;
        for(n=0;n<TEXTURE_SEED;n++) {
            if(default_val[n] < 0.0) {
                sprintf(errstr,"parameter %d has been preset to a -ve val: invalid in TEXTURE\n",n+1);
                return(PROGRAM_ERROR);
            }
        }
        break;

    case(GRAIN_COUNT):      case(GRAIN_OMIT):       case(GRAIN_DUPLICATE):
    case(GRAIN_REORDER):    case(GRAIN_REPITCH):    case(GRAIN_RERHYTHM):
    case(GRAIN_REMOTIF):    case(GRAIN_TIMEWARP):   case(GRAIN_POSITION):
    case(GRAIN_ALIGN):      case(GRAIN_GET):        case(GRAIN_REVERSE):
        default_val[GR_BLEN]            = min(GR_MINDUR,duration);
        default_val[GR_GATE]            =  GR_GATE_DEFAULT;
        default_val[GR_MINTIME]         = (GRAIN_SPLICELEN + GRAIN_SAFETY) * MS_TO_SECS * 2.0;
        default_val[GR_WINSIZE]         = 50.0;         
        switch(process) {
        case(GRAIN_OMIT):       
            default_val[GR_KEEP]        = 1.0;
            default_val[GR_OUT_OF]      = 2.0;
            break;
        case(GRAIN_DUPLICATE):  
            default_val[GR_DUPLS]       = 2.0;
            break;
        case(GRAIN_TIMEWARP):
            default_val[GR_TSTRETCH]    = 1.0;
            break;
        case(GRAIN_POSITION):
            default_val[GR_OFFSET]      = 0.0;
            break;
        case(GRAIN_ALIGN):
            default_val[GR_OFFSET]      = 0.0;
            default_val[GR_GATE2]       = GR_GATE_DEFAULT;
            break;
        }
        break;
    case(RRRR_EXTEND):
        if(mode == 1) {
            default_val[RRR_GATE]    = 0.0;
            default_val[RRR_GRSIZ]   = LOW_RRR_SIZE;
            default_val[RRR_SKIP]    = 0;
            default_val[RRR_AFTER]   = 0.0;
            default_val[RRR_TEMPO]   = 1.0;
            default_val[RRR_AT]      = 0;
        } else {
            default_val[RRR_START]   = 0.0;
            default_val[RRR_END]     = duration;
        }
        default_val[RRR_SLOW]    = 1.0;
        default_val[RRR_REGU]    = 0.0;
        default_val[RRR_RANGE]   = 1.0;
        default_val[RRR_GET]     = 3.0;
        if(mode != 2) {
            default_val[RRR_STRETCH] = 2.0;
            default_val[RRR_REPET]   = 2.0;
            default_val[RRR_ASCAT]   = 0.0;
            default_val[RRR_PSCAT]   = 0.0;
        }
        break;
    case(SSSS_EXTEND):
        default_val[SSS_DUR]      = 10;
        default_val[NOISE_MINFRQ] = NOIS_MIN_FRQ;
        default_val[MIN_NOISLEN]  = 50.0;
        default_val[MAX_NOISLEN]  = min(1.0,duration);
        default_val[SSS_GATE]     = 0.0;
        break;
    case(ENV_CREATE):
    case(ENV_BRKTOENV):
    case(ENV_DBBRKTOENV):
    case(ENV_IMPOSE):
//TW NEW CASE
    case(ENV_PROPOR):
    case(ENV_REPLACE):
        default_val[ENV_WSIZE]          = ENV_DEFAULT_WSIZE;
        break;
    case(ENV_EXTRACT):
        default_val[ENV_WSIZE]          = ENV_DEFAULT_WSIZE;
        if(mode==ENV_BRKFILE_OUT)
            default_val[ENV_DATAREDUCE] = ENV_DEFAULT_DATAREDUCE;
        break;
    case(ENV_ENVTOBRK):
    case(ENV_ENVTODBBRK):
        default_val[ENV_DATAREDUCE]     = ENV_DEFAULT_DATAREDUCE;
        break;
    case(ENV_WARPING):
    case(ENV_REPLOTTING):
        default_val[ENV_WSIZE]          = ENV_DEFAULT_WSIZE;
        /* fall thro */
    case(ENV_RESHAPING):
        switch(mode) {
        case(ENV_NORMALISE):
        case(ENV_REVERSE):
        case(ENV_CEILING):
            break;
        case(ENV_DUCKED):
            default_val[ENV_GATE]       = ENV_DEFAULT_GATE;
            default_val[ENV_THRESHOLD]  = ENV_DEFAULT_THRESH;
            break;
        case(ENV_EXAGGERATING):
            default_val[ENV_EXAG]       = ENV_DEFAULT_EXAGG;
            break;
        case(ENV_ATTENUATING):
            default_val[ENV_ATTEN]      = ENV_DEFAULT_ATTEN;
            break;
        case(ENV_LIFTING):
            default_val[ENV_LIFT]       = ENV_DEFAULT_LIFT;
            break;
        case(ENV_FLATTENING):
            default_val[ENV_FLATN]      = ENV_DEFAULT_FLATN;
            break;
        case(ENV_TSTRETCHING):
            default_val[ENV_TSTRETCH]   = ENV_DEFAULT_TSTRETCH;
            break;
        case(ENV_GATING):
            default_val[ENV_GATE]       = ENV_DEFAULT_GATE;
            default_val[ENV_SMOOTH]     = 0.0;
            break;
        case(ENV_INVERTING):
            default_val[ENV_GATE]       = ENV_DEFAULT_GATE;
            default_val[ENV_MIRROR]     = ENV_DEFAULT_MIRROR;
            break;
        case(ENV_LIMITING):
            default_val[ENV_LIMIT]      = 1.0;
            default_val[ENV_THRESHOLD]  = ENV_DEFAULT_GATE;
            break;
        case(ENV_CORRUGATING):
            default_val[ENV_TROFDEL]    = ENV_DEFAULT_TROFDEL;
            /* fall thro */
        case(ENV_PEAKCNT):
            default_val[ENV_PKSRCHWIDTH]= ENV_DEFAULT_PKSRCHWIDTH;
            break;
        case(ENV_EXPANDING):
            default_val[ENV_GATE]       = ENV_DEFAULT_GATE;
            default_val[ENV_THRESHOLD]  = ENV_DEFAULT_THRESH;
            default_val[ENV_SMOOTH]     = 0.0;
            break;
        case(ENV_TRIGGERING):
            default_val[ENV_GATE]       = ENV_DEFAULT_GATE;
            default_val[ENV_TRIGRISE]   = ENV_DEFAULT_TRIGRISE;
            default_val[ENV_TRIGDUR]    = ENV_DEFAULT_TRIGDUR;
            break;
        default:
            sprintf(errstr,"Unknown case for ENVWARP,RESHAPING or REPLOTTING: initialise_param_values()\n");
            return(PROGRAM_ERROR);
        }                 
        if(process==ENV_REPLOTTING)
            default_val[ENV_DATAREDUCE] = ENV_DEFAULT_DATAREDUCE;
        break;
    case(ENV_DOVETAILING):
        default_val[ENV_STARTTRIM]      = 0.0;
        default_val[ENV_ENDTRIM]        = 0.0;
        default_val[ENV_STARTTYPE]      = ENVTYPE_EXP;
        default_val[ENV_ENDTYPE]        = ENVTYPE_EXP;
        default_val[ENV_TIMETYPE]       = ENV_TIMETYPE_SECS;
        break;
    case(ENV_CURTAILING):
        default_val[ENV_STARTTIME]      = 0.0;
        default_val[ENV_ENDTIME]        = duration;
        default_val[ENV_ENVTYPE]        = ENVTYPE_EXP;
        default_val[ENV_TIMETYPE]       = ENV_TIMETYPE_SECS;
        break;
    case(ENV_SWELL):
        default_val[ENV_PEAKTIME]       = duration/2.0;
        default_val[ENV_ENVTYPE]        = ENVTYPE_EXP;
        break;
    case(ENV_ATTACK):
        switch(mode) {
        case(ENV_ATK_GATED):
            default_val[ENV_ATK_GATE]   = 0.5;
            break;
        case(ENV_ATK_TIMED):
        case(ENV_ATK_XTIME):
            default_val[ENV_ATK_ATTIME] = min(duration/2.0,0.5);
            break;
        case(ENV_ATK_ATMAX):
            break;
        default:
            sprintf(errstr,"Unknown case for ENV_ATTACK: initialise_param_values()\n");
            return(PROGRAM_ERROR);
        }
        default_val[ENV_ATK_GAIN]       = ENV_DEFAULT_ATK_GAIN;
        default_val[ENV_ATK_ONSET]      = ENV_DEFAULT_ATK_ONSET;
        default_val[ENV_ATK_TAIL]       = ENV_DEFAULT_ATK_TAIL;
        default_val[ENV_ATK_ENVTYPE]    = ENVTYPE_EXP;
        break;
    case(ENV_PLUCK):
        default_val[ENV_PLK_ENDSAMP]    = 0.0;
        default_val[ENV_PLK_WAVELEN]    = round(sr/ENV_PLK_FRQ_MAX);
        default_val[ENV_PLK_CYCLEN]     = ENV_PLK_CYCLEN_DEFAULT;
        default_val[ENV_PLK_DECAY]      = ENV_PLK_DECAY_DEFAULT;
        break;
    case(ENV_TREMOL):
        default_val[ENV_TREM_FRQ]       = ENV_TREM_DEFAULT_FRQ;
        default_val[ENV_TREM_DEPTH]     = ENV_TREM_DEFAULT_DEPTH;
        default_val[ENV_TREM_AMP]       = 1.0;
        break;
    case(ENV_DBBRKTOBRK):
    case(ENV_BRKTODBBRK):
        break;

    case(MIX):
        default_val[MIX_ATTEN]          = 1.0;
        /* fall thro */
    case(MIXMAX):
        default_val[MIX_START]          = 0.0;
        default_val[MIX_END]            = duration;
        break;
    case(MIXTWO):       
        default_val[MIX_STAGGER]        = 0.0;
        default_val[MIX_SKIP]           = 0.0;
        default_val[MIX_SKEW]           = 1.0;
        default_val[MIX_STTA]           = 0.0;
        default_val[MIX_DURA]           = 32767.0;
        break;
//TW NEW CASE
    case(MIXMANY):
        break;
    case(MIXBALANCE):
        default_val[MIX_STAGGER]        = 0.5;
        default_val[MIX_SKIP]           = 0.0;
        default_val[MIX_SKEW]           = duration;
        break;
    case(MIXCROSS):
        default_val[MCR_STAGGER]        = 0.0;
        default_val[MCR_BEGIN]          = 0.0;
        default_val[MCR_END]            = 0.0;
        if(mode==MCCOS)
            default_val[MCR_POWFAC]     = 1.0;
        break;
    case(MIXINBETWEEN):
        switch(mode) {
        case(INBI_COUNT):
            default_val[INBETW]         = (double)DEFAULT_BETWEEN;
            break;
        case(INBI_RATIO):
            default_val[INBETW]         = 0.5;
            break;
        default:
            sprintf(errstr,"Unknown mode for MIXINBETWEEN: initialise_param_values()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(CYCINBETWEEN):
        default_val[INBETW]         = (double)DEFAULT_BETWEEN;
        default_val[BTWN_HFRQ]      = (double)DEFAULT_BTWNFRQ;
        break;
    case(MIXTEST):
    case(MIXFORMAT):
    case(MIXDUMMY):
    case(MIXINTERL):
    case(MIXSYNC):
//TW NEW CASE
    case(ADDTOMIX):
        break;
    case(MIXSYNCATT):
        default_val[MSY_WFAC]           = (double)MIN_WINFAC;
        break;
    case(MIXGAIN):
        default_val[MIX_GAIN]           = 1.0;
        default_val[MSH_STARTLINE]      = 1.0;
        default_val[MSH_ENDLINE]        = (double)linecnt;
        break;
    case(MIXTWARP):
        switch(mode) {
        case(MTW_REVERSE_T):  case(MTW_REVERSE_NT):
        case(MTW_FREEZE_T):   case(MTW_FREEZE_NT):
        case(MTW_TIMESORT):
            break;
        case(MTW_SCATTER):
            default_val[MTW_PARAM]      = 0.5;
            break;
        case(MTW_DOMINO):
            default_val[MTW_PARAM]      = 0.0;
            break;
        case(MTW_ADD_TO_TG):
        case(MTW_CREATE_TG_1):  case(MTW_CREATE_TG_2):  case(MTW_CREATE_TG_3):  case(MTW_CREATE_TG_4):
        case(MTW_ENLARGE_TG_1): case(MTW_ENLARGE_TG_2): case(MTW_ENLARGE_TG_3): case(MTW_ENLARGE_TG_4):
            default_val[MTW_PARAM]      = 0.1;
            break;
        }
        if(mode!=MTW_TIMESORT) {
            default_val[MSH_STARTLINE]  = 1.0;
            default_val[MSH_ENDLINE]    = (double)linecnt;
        }
        break;
    case(MIXSWARP):
        switch(mode) {
        case(MSW_TWISTALL):
            break;
        case(MSW_TWISTONE):
            default_val[MSW_TWLINE]     = 1.0;
            break;
        case(MSW_NARROWED):
            default_val[MSW_NARROWING]  = 0.5;
            break;
        case(MSW_LEFTWARDS):
        case(MSW_RIGHTWARDS):
        case(MSW_RANDOM):
        case(MSW_RANDOM_ALT):
            default_val[MSW_POS1]       = PAN_LEFT;
            default_val[MSW_POS2]       = PAN_RIGHT;
            break;
        case(MSW_FIXED):
            default_val[MSW_POS1]       = PAN_CENTRE;
            break;
        }
        if(mode!=MSW_TWISTALL && mode!=MSW_TWISTONE) {
            default_val[MSH_STARTLINE]  = 1.0;
            default_val[MSH_ENDLINE]    = (double)linecnt;
        }
        break;
    case(MIXSHUFL):
        default_val[MSH_STARTLINE]      = 1.0;
        default_val[MSH_ENDLINE]        = (double)linecnt;
        break;
//TW NEW CASES
    case(MIX_ON_GRID):
        break;
    case(AUTOMIX):
        default_val[0] = 1.0;
        break;
    case(MIX_PAN):
        default_val[PAN_PAN]            = 0.0;
        break;
    case(MIX_AT_STEP):
        default_val[MIX_STEP]           = 0.0;
        break;
    case(SHUDDER):
        default_val[SHUD_STARTTIME]     = 0.0;
        default_val[SHUD_FRQ]           = 6.0;
        default_val[SHUD_SCAT]          = 1.0;
        default_val[SHUD_SPREAD]        = 1.0;
        default_val[SHUD_MINDEPTH]      = 0.0;
        default_val[SHUD_MAXDEPTH]      = 1.0;
        default_val[SHUD_MINWIDTH]      = 0.02;
        default_val[SHUD_MAXWIDTH]      = 0.2;
        break;

    case(EQ):
        switch(mode) {
        case(FLT_PEAKING):
            default_val[FLT_BW]         = FLT_DEFAULT_BW;
            /* fall thro */
        default:
            default_val[FLT_BOOST]      = 0.0; /* dB */
            default_val[FLT_ONEFRQ]     = FLT_DEFAULT_FRQ;
            default_val[FLT_PRESCALE]   = 1.0;
//TW NEW PARAM
            default_val[FILT_TAIL]      = 1.0;
            break;
        }
        break;
    case(LPHP):
        default_val[FLT_GAIN]           = FLT_DEFAULT_LOHI_ATTEN;
        default_val[FLT_PRESCALE]       = FLT_DEFAULT_LPHP_PRESCALE;    /* as in orig prog */
        switch(mode) {
        case(FLT_HZ):
            default_val[FLT_PASSFRQ]    = FLT_DEFAULT_LOHIPASS;
            default_val[FLT_STOPFRQ]    = FLT_DEFAULT_LOHISTOP;
            break;
        case(FLT_MIDI):
            default_val[FLT_PASSFRQ]    = FLT_DEFAULT_LOHIPASSMIDI;
            default_val[FLT_STOPFRQ]    = FLT_DEFAULT_LOHISTOPMIDI;
            break;
        }
//TW NEW PARAM
        default_val[FILT_TAIL]      = 1.0;
        break;
    case(FSTATVAR):
        default_val[FLT_Q]              = 1.0/FLT_DEFAULT_Q;    /* parameter is actually 1/Q */
        default_val[FLT_GAIN]           = 1.0;
        default_val[FLT_ONEFRQ]         = FLT_DEFAULT_FRQ;
//TW NEW PARAM
        default_val[FILT_TAIL]      = 1.0;
        break;
    case(FLTBANKN):
        default_val[FLT_Q]              = FLT_DEFAULT_Q;
        default_val[FLT_GAIN]           = 1.0;
//TW NEW PARAM
        default_val[FILT_TAIL]      = 1.0;
        /* fall thro */
    case(FLTBANKC):
        default_val[FLT_LOFRQ]          = FLT_DEFAULT_LOFRQ;
        default_val[FLT_HIFRQ]          = FLT_DEFAULT_HIFRQ;
        switch(mode) {
        case(FLT_LINOFFSET):
            default_val[FLT_OFFSET]     = FLT_DEFAULT_OFFSET;
            break;
        case(FLT_EQUALSPAN):
            default_val[FLT_INCOUNT]    = FLT_DEFAULT_INCOUNT;
            break;
        case(FLT_EQUALINT):
            default_val[FLT_INTSIZE]    = FLT_DEFAULT_INTSIZE;
            break;
        }
        default_val[FLT_RANDFACT]       = 0.0;
        break;
    case(FLTBANKU):     
        default_val[FLT_Q]              = FLT_DEFAULT_Q;
        default_val[FLT_GAIN]           = 1.0;
//TW NEW PARAM
        default_val[FILT_TAIL]          = 1.0;
        break;
    case(FLTBANKV):
        default_val[FLT_Q]              = FLT_DEFAULT_Q;
        default_val[FLT_GAIN]           = 1.0;
        default_val[FLT_HARMCNT]        = FLT_DEFAULT_HCNT;
        default_val[FLT_ROLLOFF]        = FLT_DEFAULT_ROLLOFF;
        default_val[FILT_TAILV]         = 1.0;
        break;
    case(FLTBANKV2):
        default_val[FLT_Q]              = FLT_DEFAULT_Q;
        default_val[FLT_GAIN]           = 1.0;
        default_val[FILT_TAILV]         = 1.0;
        break;
    case(FLTSWEEP):
        default_val[FLT_Q]              = 1.0/FLT_DEFAULT_Q;
        default_val[FLT_GAIN]           = 1.0; 
        default_val[FLT_LOFRQ]          = FLT_DEFAULT_LOFRQ;
        default_val[FLT_HIFRQ]          = FLT_DEFAULT_HIFRQ;
        default_val[FLT_SWPFRQ]         = FLT_DEFAULT_SWPFRQ;
        default_val[FLT_SWPPHASE]       = FLT_DEFAULT_SWPPHASE;
//TW NEW PARAM
        default_val[FILT_TAIL]          = 1.0;
        break;
    case(FLTITER):
        default_val[FLT_Q]              = FLT_DEFAULT_Q;
        default_val[FLT_GAIN]           = 1.0;
        default_val[FLT_DELAY]          = FLT_DEFAULT_ITERDELAY;
        default_val[FLT_OUTDUR]         = duration * 2.0;
        default_val[FLT_PRESCALE]       = 0.0;
        default_val[FLT_RANDDEL]        = 0.0;
        default_val[FLT_PSHIFT]         = 0.0;
        default_val[FLT_AMPSHIFT]       = 0.0;
        break;
    case(ALLPASS):      
        default_val[FLT_GAIN]           = 1.0;
        default_val[FLT_DELAY]          = FLT_DEFAULT_ALLPASSDELAY;
        default_val[FLT_PRESCALE]       = 1.0;
//TW NEW PARAM
        default_val[FILT_TAIL]          = 1.0;
        break;

    case(MOD_LOUDNESS):
        switch(mode) {
        case(LOUDNESS_GAIN):
            default_val[LOUD_GAIN]      = 1.0;
            break;
        case(LOUDNESS_DBGAIN):
            default_val[LOUD_GAIN]      = 0.0;
            break;
        case(LOUDNESS_NORM):
        case(LOUDNESS_SET):
//TW NEW DEFAULT
//          default_val[LOUD_LEVEL]     = 1.0;
            default_val[LOUD_LEVEL]     = 0.9;
            break;
        }
        break;

    case(MOD_SPACE):
        switch(mode) {
        case(MOD_PAN):
            default_val[PAN_PAN]        = 0.0;
            default_val[PAN_PRESCALE]   = PAN_PRESCALE_DEFAULT;
            break;
        case(MOD_NARROW):
            default_val[NARROW]         = 1.0;
            break;
        }
        break;
//TW NEW CASES
        case(SCALED_PAN):
        default_val[PAN_PAN]        = 0.0;
        default_val[PAN_PRESCALE]   = PAN_PRESCALE_DEFAULT;
        break;
    case(FIND_PANPOS):
        default_val[PAN_PAN]        = 0.0;
        break;

    case(MOD_PITCH):
        switch(mode) {
        case(MOD_TRANSPOS):
        case(MOD_TRANSPOS_INFO):
            default_val[VTRANS_TRANS]   = 1.0;
            break;
        case(MOD_TRANSPOS_SEMIT):
        case(MOD_TRANSPOS_SEMIT_INFO):
            default_val[VTRANS_TRANS]   = 0.0;
            break;
        case(MOD_ACCEL):
            default_val[ACCEL_ACCEL]    = 1.0;
// TW NEW DEFAULT
//          default_val[ACCEL_GOALTIME] = 1.0;
            default_val[ACCEL_GOALTIME] = min(1.0,duration);
            default_val[ACCEL_STARTTIME]= 0.0;
            break;
        case(MOD_VIBRATO):
            default_val[VIB_FRQ]        = DEFAULT_VIB_FRQ;
            default_val[VIB_DEPTH]      = DEFAULT_VIB_DEPTH;
            break;
        }
        break;
    case(MOD_REVECHO):
        switch(mode) {
        case(MOD_STADIUM):
            default_val[STAD_PREGAIN]   = STAD_PREGAIN_DFLT;
            default_val[STAD_ROLLOFF]   = 1.0;
            default_val[STAD_SIZE]      = 1.0;
//TW NEW DEFAULT AFTER increasing number of possible echoes
//          default_val[STAD_ECHOCNT]   = MAX_ECHOCNT;
            default_val[STAD_ECHOCNT]   = REASONABLE_ECHOCNT;
            break;
        case(MOD_DELAY):
        case(MOD_VDELAY):
            default_val[DELAY_LFOMOD]   = 0.0;
            default_val[DELAY_LFOFRQ]   = 0.0;
            default_val[DELAY_LFOPHASE] = 0.0;
            default_val[DELAY_LFODELAY] = 0.0;
            default_val[DELAY_DELAY]    = DEFAULT_DELAY; /* milliseconds */
            default_val[DELAY_MIX]      = 0.5;
            default_val[DELAY_FEEDBACK] = 0.0;
            default_val[DELAY_TRAILTIME]= 0.0;
            default_val[DELAY_PRESCALE] = 1.0;
            if(mode==MOD_VDELAY)
                default_val[DELAY_SEED] = 0.0;
            break;
        }
        break;
    case(MOD_RADICAL):
        switch(mode) {
        case(MOD_REVERSE):  
            break;
        case(MOD_SHRED):
            default_val[SHRED_CNT]      = 1.0;
            default_val[SHRED_CHLEN]    = max((duration/8.0),((double)(SHRED_SPLICELEN * 3)/(double)srate));
            default_val[SHRED_SCAT]     = 1.0;
            break;
        case(MOD_SCRUB):
            default_val[SCRUB_TOTALDUR]     = duration;
            default_val[SCRUB_MINSPEED]     = SCRUB_MINSPEED_DEFAULT;
            default_val[SCRUB_MAXSPEED]     = SCRUB_MAXSPEED_DEFAULT;
            default_val[SCRUB_STARTRANGE]   = 0.0;
            default_val[SCRUB_ESTART]       = duration;
            break;
        case(MOD_LOBIT):
            default_val[LOBIT_BRES]     = (double)MAX_BIT_DIV;
            default_val[LOBIT_TSCAN]    = 1.0;
            break;
        case(MOD_LOBIT2):
            default_val[LOBIT_BRES]     = (double)MAX_BIT_DIV;
            break;
        case(MOD_RINGMOD):
            default_val[RM_FRQ]         = (double)MIDDLE_C_MIDI;
            break;
        case(MOD_CROSSMOD):
            break;
        }
        break;
    case(BRASSAGE):
        default_val[GRS_VELOCITY]       = 1.0;
        switch(mode) {
        case(GRS_REVERB):
            default_val[GRS_DENSITY]    = GRS_DEFAULT_REVERB_DENSITY;
            break;
        default:
            default_val[GRS_DENSITY]    = GRS_DEFAULT_DENSITY;
            break;
        }
        default_val[GRS_HVELOCITY]      = 1.0;
        default_val[GRS_HDENSITY]       = GRS_DEFAULT_DENSITY;
        default_val[GRS_GRAINSIZE]      = GRS_DEFAULT_GRAINSIZE;
        default_val[GRS_PITCH]          = 0.0;
        default_val[GRS_AMP]            = 1.0;
        switch(mode) {
        case(GRS_REVERB):
            default_val[GRS_SPACE]      = 0.0;
            break;
        default:
            default_val[GRS_SPACE]      = 0.5;
            break;
        }
        default_val[GRS_BSPLICE]        = GRS_DEFAULT_SPLICELEN;
        default_val[GRS_ESPLICE]        = GRS_DEFAULT_SPLICELEN;
        default_val[GRS_HGRAINSIZE]     = GRS_DEFAULT_GRAINSIZE;
        default_val[GRS_HPITCH]         = 0.0;
        default_val[GRS_HAMP]           = 1.0;
        switch(mode) {
        case(GRS_REVERB):
            default_val[GRS_HSPACE]     = 1.0;
            break;
        default:
            default_val[GRS_HSPACE]     = 0.5;
            break;
        }
        default_val[GRS_HBSPLICE]       = GRS_DEFAULT_SPLICELEN;
        default_val[GRS_HESPLICE]       = GRS_DEFAULT_SPLICELEN;
        switch(mode) {
        case(GRS_BRASSAGE):
        case(GRS_FULL_MONTY):
            default_val[GRS_SCATTER]    = GRS_DEFAULT_SCATTER;
            default_val[GRS_OUTLEN]     = 0.0;
            default_val[GRS_CHAN_TO_XTRACT] = 0.0;
            /* fall thro */
        case(GRS_REVERB):
        case(GRS_SCRAMBLE):
            default_val[GRS_SRCHRANGE]  = 0.0;
            break;
        }
        break;
    case(SAUSAGE):
        default_val[GRS_VELOCITY]       = 1.0;
        default_val[GRS_DENSITY]        = GRS_DEFAULT_DENSITY;
        default_val[GRS_HVELOCITY]      = 1.0;
        default_val[GRS_HDENSITY]       = GRS_DEFAULT_DENSITY;
        default_val[GRS_GRAINSIZE]      = GRS_DEFAULT_GRAINSIZE;
        default_val[GRS_PITCH]          = 0.0;          
        default_val[GRS_AMP]            = 1.0;
        default_val[GRS_SPACE]          = 0.5;  
        default_val[GRS_BSPLICE]        = GRS_DEFAULT_SPLICELEN;
        default_val[GRS_ESPLICE]        = GRS_DEFAULT_SPLICELEN;
        default_val[GRS_HGRAINSIZE]     = GRS_DEFAULT_GRAINSIZE;
        default_val[GRS_HPITCH]         = 0.0;
        default_val[GRS_HAMP]           = 1.0;
        default_val[GRS_HSPACE]         = 0.5;
        default_val[GRS_HBSPLICE]       = GRS_DEFAULT_SPLICELEN;
        default_val[GRS_HESPLICE]       = GRS_DEFAULT_SPLICELEN;
        default_val[GRS_SCATTER]        = GRS_DEFAULT_SCATTER;
        default_val[GRS_OUTLEN]         = 0.0;
        default_val[GRS_CHAN_TO_XTRACT] = 0.0;
        default_val[GRS_SRCHRANGE]      = 0.0;
        break;
/* TEMPORARY TEST ROUTINE */
    case(WORDCNT):
        break;
/* TEMPORARY TEST ROUTINE */


    case(PVOC_ANAL):
        default_val[PVOC_CHANS_INPUT]   = (double)DEFAULT_PVOC_CHANS;
        default_val[PVOC_WINOVLP_INPUT] = (double)DEFAULT_WIN_OVERLAP;
        break;
    case(PVOC_SYNTH):
        break;
    case(PVOC_EXTRACT):
        default_val[PVOC_CHANS_INPUT]   = (double)DEFAULT_PVOC_CHANS;
        default_val[PVOC_WINOVLP_INPUT] = (double)DEFAULT_WIN_OVERLAP;
        default_val[PVOC_CHANSLCT_INPUT]= 0.0;
        default_val[PVOC_LOCHAN_INPUT]  = 0.0;
        default_val[PVOC_HICHAN_INPUT]  = 0.0;
        break;

    case(EDIT_CUT):
        default_val[CUT_CUT]            = 0.0;
        switch(mode) {
        case(EDIT_SECS):    default_val[CUT_END] = duration;                    break;
        case(EDIT_SAMPS):   default_val[CUT_END] = (double)insams;              break;
        case(EDIT_STSAMPS): default_val[CUT_END] = (double)(insams/channels);   break;
        }
        default_val[CUT_SPLEN]          = EDIT_SPLICELEN;
        break;
    case(EDIT_CUTEND):
        default_val[CUT_CUT]            = 0.0;
        default_val[CUT_SPLEN]          = EDIT_SPLICELEN;
        break;
    case(MANY_ZCUTS):
        break;
    case(EDIT_ZCUT):
        default_val[CUT_CUT]            = 0.0;
        switch(mode) {
        case(EDIT_SECS):    default_val[CUT_END] = duration;                    break;
        case(EDIT_SAMPS):   default_val[CUT_END] = (double)(insams);            break;
        case(EDIT_STSAMPS): default_val[CUT_END] = (double)(insams/channels);   break;
        }
        break;
    case(EDIT_EXCISE):
        default_val[CUT_CUT]            = 0.0;
        switch(mode) {
        case(EDIT_SECS):    default_val[CUT_END] = duration;                    break;
        case(EDIT_SAMPS):   default_val[CUT_END] = (double)(insams);            break;
        case(EDIT_STSAMPS): default_val[CUT_END] = (double)(insams/channels);   break;
        }
        default_val[CUT_SPLEN]          = EDIT_SPLICELEN;
        break;
    case(EDIT_EXCISEMANY):
    case(INSERTSIL_MANY):
        default_val[CUT_SPLEN]          = EDIT_SPLICELEN;
        break;
    case(EDIT_INSERT):
        default_val[CUT_CUT]            = 0.0;
        default_val[CUT_SPLEN]          = EDIT_SPLICELEN;
        default_val[INSERT_LEVEL]       = 1.0;
        break;
//TW NEW CASE
    case(EDIT_INSERT2):
        default_val[CUT_CUT]            = 0.0;
        default_val[CUT_END]            = 0.0;
        default_val[CUT_SPLEN]          = EDIT_SPLICELEN;
        default_val[INSERT_LEVEL]       = 1.0;
        break;

    case(EDIT_INSERTSIL):
        default_val[CUT_CUT]            = 0.0;
        switch(mode) {
        case(EDIT_SECS):    default_val[CUT_END] = duration;                    break;
        case(EDIT_SAMPS):   default_val[CUT_END] = (double)(insams);            break;
        case(EDIT_STSAMPS): default_val[CUT_END] = (double)(insams/channels);   break;
        }
        default_val[CUT_SPLEN]          = EDIT_SPLICELEN;
        break;
    case(JOIN_SEQ):
        default_val[MAX_LEN]            = 32767;
        /* fall thro */
    case(EDIT_JOIN):
    case(JOIN_SEQDYN):
        default_val[CUT_SPLEN]          = EDIT_SPLICELEN;
        break;
    case(HOUSE_COPY):
        switch(mode) {
        case(COPYSF):   break;
        case(DUPL):         default_val[COPY_CNT] = 1.0;                        break;
        }
        break;
    case(HOUSE_DEL):
        break;
    case(HOUSE_CHANS):
        switch(mode) {
        case(HOUSE_CHANNEL):
        case(HOUSE_ZCHANNEL):
            default_val[CHAN_NO] = 1.0;
            break;
        case(HOUSE_CHANNELS):
        case(STOM):
        case(MTOS):
            break;
        }
        break;
    case(HOUSE_BUNDLE):
        break;
    case(HOUSE_SORT):
        switch(mode) {
        case(BY_DURATION):
        case(BY_LOG_DUR):
            default_val[SORT_SMALL] = SORT_SMALL_DEFAULT;
            default_val[SORT_LARGE] = SORT_LARGE_DEFAULT;
            if(mode==BY_DURATION)
                default_val[SORT_STEP] = SORT_STEP_DEFAULT;
            else
                default_val[SORT_STEP] = SORT_LSTEP_DEFAULT;
            break;
        }
        break;
    case(HOUSE_SPEC):
        switch(mode) {
        case(HOUSE_REPROP):
            default_val[HSPEC_CHANS] = (double)channels;
            if(filetype==SNDFILE)
                default_val[HSPEC_TYPE] = (double)SAMP_SHORT;
            else
            /*RWD what's going on here ? */
                default_val[HSPEC_TYPE] = (double)SAMP_FLOAT;
            /* fall thro */
        case(HOUSE_RESAMPLE):
            default_val[HSPEC_SRATE] = (double)srate;
            break;
        default:
            break;
        }
        break;
    case(HOUSE_EXTRACT):
        switch(mode) {
        case(HOUSE_CUTGATE):
            default_val[CUTGATE_SPLEN]      = CUTGATE_SPLICELEN;
            default_val[CUTGATE_SUSTAIN]    = 0.0;
            /* fall thro */
        case(HOUSE_ONSETS):
            default_val[CUTGATE_GATE]       = 0.0;
            default_val[CUTGATE_ENDGATE]    = 0.0;
            default_val[CUTGATE_THRESH]     = 0.0;
            default_val[CUTGATE_BAKTRAK]    = 0.0;
            default_val[CUTGATE_INITLEVEL]  = 0.0;
            default_val[CUTGATE_MINLEN]     = min(duration,2.0 * CUTGATE_SPLICELEN * MS_TO_SECS);
            default_val[CUTGATE_WINDOWS] = 0.0;
            break;
        case(HOUSE_CUTGATE_PREVIEW):
            break;
        case(HOUSE_TOPNTAIL):
            default_val[TOPN_GATE]   = 0.0;
            default_val[TOPN_SPLEN]  = CUTGATE_SPLICELEN;
            break;
        case(HOUSE_RECTIFY):
            default_val[RECTIFY_SHIFT] = 0.0;
            break;
        }
        break;
    case(TOPNTAIL_CLICKS):
        default_val[TOPN_GATE]   = 0.0;
        default_val[TOPN_SPLEN]  = TOPNTAIL_SPLICELEN;
        break;
    case(HOUSE_BAKUP):
    case(HOUSE_GATE):
        default_val[GATE_ZEROS]     = 4;
        break;
//UP TO HERE
  case(HOUSE_DISK):
        break;
    case(INFO_PROPS):
    case(INFO_SFLEN):
    case(INFO_TIMELIST):
    case(INFO_LOUDLIST):
    case(INFO_TIMEDIFF):
    case(INFO_LOUDCHAN):
//TW    break;
    case(INFO_MAXSAMP):
        break;
    case(INFO_MAXSAMP2):
        default_val[MAX_STIME]  = 0.0;
        default_val[MAX_ETIME]  = duration;
        break;
    case(INFO_TIMESUM):
        default_val[TIMESUM_SPLEN]  = TIMESUM_DEFAULT_SPLEN;
        break;  
    case(INFO_SAMPTOTIME):
        default_val[INFO_SAMPS]     = 0.0;
        break;
    case(INFO_TIMETOSAMP):
        default_val[INFO_TIME]      = 0.0;
        break;
    case(INFO_FINDHOLE):
        default_val[HOLE_THRESH]    = 0.0;
        break;  
    case(INFO_DIFF):
    case(INFO_CDIFF):
        default_val[SFDIFF_THRESH]  = 0.0;
        default_val[SFDIFF_CNT]     = 1.0;
        break;  
    case(INFO_PRNTSND):
        default_val[PRNT_START]     = 0.0;
        default_val[PRNT_END]       = duration;
        break;  
    case(INFO_MUSUNITS):
        switch(mode) {
        case(MU_MIDI_TO_FRQ):
        case(MU_MIDI_TO_NOTE):
            default_val[0]  = MIDDLE_C_MIDI;
            break;
        case(MU_FRQ_TO_MIDI):
        case(MU_FRQ_TO_NOTE):
            default_val[0]  = CONCERT_A;
            break;
        case(MU_FRQRATIO_TO_SEMIT):
        case(MU_FRQRATIO_TO_INTVL):
        case(MU_FRQRATIO_TO_OCTS):
        case(MU_FRQRATIO_TO_TSTRETH):
            default_val[0]  = 2.0;
            break;
        case(MU_SEMIT_TO_FRQRATIO):
        case(MU_SEMIT_TO_OCTS):
        case(MU_SEMIT_TO_INTVL):
        case(MU_SEMIT_TO_TSTRETCH):
            default_val[0]  = 12.0;
            break;
        case(MU_OCTS_TO_FRQRATIO):
        case(MU_OCTS_TO_SEMIT):
        case(MU_OCTS_TO_TSTRETCH):
            default_val[0]  = 1.0;
            break;
        case(MU_TSTRETCH_TO_FRQRATIO):
        case(MU_TSTRETCH_TO_SEMIT):
        case(MU_TSTRETCH_TO_OCTS):
        case(MU_TSTRETCH_TO_INTVL):
            default_val[0]  = 1.0;
            break;
        case(MU_GAIN_TO_DB):
            default_val[0]  = 1.0;
            break;
        case(MU_DB_TO_GAIN):
            default_val[0]  = 0.0;
            break;
        case(MU_NOTE_TO_FRQ):
        case(MU_NOTE_TO_MIDI):
        case(MU_INTVL_TO_FRQRATIO):
        case(MU_INTVL_TO_TSTRETCH):
            break;
/*** NOW OPERATES THRO THE MUSIC CALCULATOR ***/
        }
        break;
    case(SYNTH_WAVE):
        default_val[SYN_TABSIZE]= (double)WAVE_TABSIZE;
        default_val[SYN_FRQ]    = CONCERT_A;
        /* fall thro */
    case(SYNTH_NOISE):
        default_val[SYN_AMP]    = 1.0;
        /* fall thro */
    case(SYNTH_SIL):
        default_val[SYN_SRATE]  = (double)WAVE_DEFAULT_SR;
        default_val[SYN_CHANS]  = 1;
        default_val[SYN_DUR]    = 1.0;
        break;
    case(MULTI_SYN):
        default_val[SYN_TABSIZE]= (WAVE_TABSIZE * 16);
        default_val[SYN_AMP]    = 0.3;
        default_val[SYN_SRATE]  = (double)WAVE_DEFAULT_SR;
        default_val[SYN_CHANS]  = 1;
        default_val[SYN_DUR]    = 4.0;
        break;
    case(SYNTH_SPEC):
        default_val[SS_DUR]      = 1.0;
        default_val[SS_CENTRFRQ] = CONCERT_A;
        default_val[SS_SPREAD]   = 0.0;
        default_val[SS_FOCUS]    = 1.0;
        default_val[SS_FOCUS2]   = 1.0;
        default_val[SS_TRAND]    = 0.0;
        default_val[SS_SRATE]    = 44100;
        break;
    case(RANDCUTS):
        default_val[RC_CHLEN]  = max((duration/8.0),((double)(SHRED_SPLICELEN * channels * 3)/(double)srate));
        default_val[RC_SCAT]   = 1.0;
        break;
    case(RANDCHUNKS):
        default_val[CHUNKCNT]  = 2;
        default_val[MINCHUNK]  = MINOUTDUR;
        default_val[MAXCHUNK]  = duration;
        break;
    case(TWIXT):
    case(SPHINX):
        default_val[IS_SPLEN]  = 2.7;
        default_val[IS_WEIGHT] = 1;
        default_val[IS_SEGCNT] = 1;
        break;
//TW NEW CASES
    case(EDIT_CUTMANY):
        default_val[CM_SPLICELEN] = EDIT_SPLICELEN;
        break;
    case(STACK):
        default_val[STACK_CNT]    = 3;
        default_val[STACK_LEAN]   = 1.0;
        default_val[STACK_OFFSET] = 0.0;
        default_val[STACK_GAIN]   = 1.0;
        default_val[STACK_DUR]    = 1.0;
        break;

    case(SIN_TAB):
        default_val[SIN_FRQ]   = 1.0;
        default_val[SIN_AMP]   = 1;
        default_val[SIN_DUR]   = 10;
        default_val[SIN_QUANT] = .01;
        default_val[SIN_PHASE] = 0;
        break;
    case(ACC_STREAM):
        default_val[ACC_ATTEN] = 1.0;
        break;
    case(HF_PERM1):
    case(HF_PERM2):
        default_val[HP1_SRATE]    = 44100;
        default_val[HP1_ELEMENT_SIZE] = 1.0;
        default_val[HP1_GAP_SIZE] = 1.0;
        default_val[HP1_GGAP_SIZE]= 1.0;
        default_val[HP1_MINSET]   = 1.0;
        default_val[HP1_BOTNOTE]  = 0.0;
        default_val[HP1_BOTOCT]   = 0.0;
        default_val[HP1_TOPNOTE]  = 0.0;
        default_val[HP1_TOPOCT]   = 1.0;
        default_val[HP1_SORT1]    = 0;
        break;
    case(DEL_PERM):
        default_val[DP_SRATE]  = 44100;
        default_val[DP_DUR]    = 1.0;
        default_val[DP_CYCCNT] = 4;
        break;
    case(DEL_PERM2):
        default_val[DP_CYCCNT] = 4;
        break;
//TW NEW CASES
    case(NOISE_SUPRESS):
        default_val[NOISE_SPLEN] = 2.0;
        default_val[NOISE_MINFRQ] = 6000.0;
        default_val[MIN_NOISLEN] = 0.0;
        default_val[MIN_TONELEN] = 15.0;
        break;
    case(TIME_GRID):
        default_val[GRID_COUNT]  = 4.0;
        default_val[GRID_WIDTH]  = 0.1;
        default_val[GRID_SPLEN]  = 3.0;
        break;
    case(SEQUENCER2):
        default_val[SEQ_SPLIC]  = 2.0;
        /* fall thro */
    case(SEQUENCER):
        default_val[SEQ_ATTEN]  = 1.0;
        break;
    case(CONVOLVE):
        if(mode==CONV_TVAR)
            default_val[CONV_TRANS]  = 0.0;
        break;
    case(BAKTOBAK):
        default_val[BTOB_CUT]    = .015;
        default_val[BTOB_SPLEN]  = 15.0;
        break;
    case(CLICK):
        if(mode == CLICK_BY_LINE) {
            default_val[CLIKSTART]  = 1.0;
        } else {
            default_val[CLIKSTART]  = 0.0;
        }
        default_val[CLIKOFSET]  = 1.0;
        default_val[CLIKEND]  = 32767.0;
        break;
    case(DOUBLETS):
        default_val[SEG_DUR]    = 0.1;
        default_val[SEG_REPETS] = 2.0;
        break;
    case(SYLLABS):
        default_val[SYLLAB_DOVETAIL]    = 5.0;
        default_val[SYLLAB_SPLICELEN]   = 2.0;
        break;
    case(MAKE_VFILT):
    case(MIX_MODEL):
        break;
    case(BATCH_EXPAND):
        default_val[BE_INFILE]   = 1;
        default_val[BE_OUTFILE]  = 1;
        default_val[BE_PARAM]    = 1;
        break;
    case(ENVSYN):
        default_val[ENVSYN_WSIZE]       = 50.0;
        default_val[ENVSYN_DUR]         = 60.0;
        default_val[ENVSYN_CYCLEN]      = 1.0;
        default_val[ENVSYN_STARTPHASE]  = 0.0;
        if(mode != ENVSYN_USERDEF) {
            default_val[ENVSYN_TROF]        = 0.0;
            default_val[ENVSYN_EXPON]       = 1.0;
        }
        break;
    case(HOUSE_GATE2):
        default_val[GATE2_DUR]      = 20.0;
        default_val[GATE2_ZEROS]    = 4.0;
        default_val[GATE2_LEVEL]    = 0.0;
        default_val[GATE2_SPLEN]    = 2.0;
        default_val[GATE2_FILT]     = 2.0;
        break;
    case(GRAIN_ASSESS):
        break;
    case(ZCROSS_RATIO):
        default_val[ZC_START]   = 0.0;
        default_val[ZC_END]     = duration;
        break;
    case(GREV):
        default_val[GREV_WSIZE]      = 5;
        default_val[GREV_TROFRAC]    = .2;
        default_val[GREV_GPCNT]      = 1.0;
        switch(mode) {
        case(GREV_TSTRETCH):
            default_val[GREV_TSTR]   = 2.0;
            break;
        case(GREV_DELETE):
        case(GREV_OMIT):
            default_val[GREV_KEEP]   = 63;
            default_val[GREV_OUTOF]  = 64;
            break;
        case(GREV_REPEAT):
            default_val[GREV_REPETS] = 2;
            break;
        }
        break;
    default:
        sprintf(errstr,"Unknown case: initialise_param_values()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/**********************************************************************************/
/*********************** ITEMS FORMERLY IN FORMTYPE.C tklib2 **********************/
/**********************************************************************************/

/**************************** DEAL_WITH_FORMANTS *************************/

int deal_with_formants(int process,int mode,int channels,aplptr ap)
{
    int exit_status;
    set_formant_flags(process,mode,ap);
    if(ap->formant_flag) {
        if((exit_status = establish_formant_band_ranges(channels,ap))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/**************************** SET_FORMANT_FLAGS *************************/

void set_formant_flags(int process,int mode,aplptr ap)
{
    switch(process) {
    case(CHORD):    ap->formant_qksrch = TRUE;  ap->formant_flag = TRUE;    break;
    case(FOCUS):    ap->formant_qksrch = TRUE;  ap->formant_flag = TRUE;    break;
    case(FORMANTS): ap->formant_qksrch = TRUE;  ap->formant_flag = TRUE;    break;
    case(FORMSEE):  ap->formant_qksrch = TRUE;  ap->formant_flag = TRUE;    break;
    case(GLIS):     ap->formant_qksrch = TRUE;  ap->formant_flag = TRUE;    break;
    case(REPORT):   ap->formant_qksrch = TRUE;  ap->formant_flag = TRUE;    break;
    case(SPREAD):   ap->formant_qksrch = TRUE;  ap->formant_flag = TRUE;    break;
    case(TRNSF):    ap->formant_qksrch = TRUE;  ap->formant_flag = TRUE;    break;
    case(VOCODE):   ap->formant_qksrch = TRUE;  ap->formant_flag = TRUE;    break;
    case(FORM):     ap->formant_qksrch = TRUE;                              break;
    case(OCT):      ap->formant_qksrch = TRUE;                              break;
    }
}

/**************************** ESTABLISH_FORMANT_BAND_RANGE *************************/

int establish_formant_band_ranges(int channels,aplptr ap)
{
    int clength = channels/2;
    int clength_less_one = clength - 1;
    if(clength_less_one < 0) {
        sprintf(errstr,"Invalid call to process formants: establish_formant_band_ranges()\n");
        return(PROGRAM_ERROR);
    }
    if(clength_less_one < LOW_OCTAVE_BANDS) {
        fprintf(stdout,"WARNING: Too few analysis channels for pitchwise formant extraction.\n");
        fflush(stdout);
        ap->no_pichwise_formants = TRUE;
    } else
        ap->max_pichwise_fbands = MAX_BANDS_PER_OCT;
    ap->max_freqwise_fbands = (clength_less_one)/2;
    return(FINISHED);
}

/**********************************************************************************/
/*********************** ITEMS FORMERLY IN INTYPE.C tklib2 ************************/
/**********************************************************************************/

// TW assign_input_process_type REMOVED: REDUNDANT

/******************************* DOES_PROCESS_ACCEPT_CONFLICTING_SRATES *******************************/

int does_process_accept_conflicting_srates(int process)
{
    if(process==GRAIN_ALIGN
    || process==ENV_IMPOSE
//TW NEW CASE
    || process==ENV_PROPOR
    || process==ENV_REPLACE)
        return(TRUE);
    return(FALSE);
}

/**********************************************************************************/
/*********************** ITEMS FORMERLY IN MAXMODE.C tklib2 ***********************/
/**********************************************************************************/

/************************************ GET_MAXMODE ************************************/

int get_maxmode(int process)
{
    int maxmode;
    switch(process) {
    case(ACCU):             maxmode = 0;    break;
    case(ALT):              maxmode = 2;    break;
    case(ARPE):             maxmode = 8;    break;
    case(AVRG):             maxmode = 0;    break;
    case(BARE):             maxmode = 0;    break;
    case(BLTR):             maxmode = 0;    break;
    case(BLUR):             maxmode = 0;    break;
    case(BRIDGE):           maxmode = 6;    break;
    case(CHANNEL):          maxmode = 0;    break;
    case(CHORD):            maxmode = 0;    break;
    case(CHORUS):           maxmode = 7;    break;
    case(CLEAN):            maxmode = 4;    break;
    case(CROSS):            maxmode = 0;    break;
    case(CUT):              maxmode = 0;    break;
    case(DIFF):             maxmode = 0;    break;
    case(DRUNK):            maxmode = 0;    break;
    case(EXAG):             maxmode = 0;    break;
    case(FILT):             maxmode = 12;   break;
    case(FMNTSEE):          maxmode = 0;    break;
    case(FOCUS):            maxmode = 0;    break;
    case(FOLD):             maxmode = 0;    break;
    case(FORM):             maxmode = 2;    break;
    case(FORMANTS):         maxmode = 0;    break;
    case(FORMSEE):          maxmode = 0;    break;
    case(FREEZE):           maxmode = 3;    break;
    case(FREEZE2):          maxmode = 0;    break;
    case(FREQUENCY):        maxmode = 0;    break;
    case(GAIN):             maxmode = 0;    break;
    case(GLIDE):            maxmode = 0;    break;
    case(GLIS):             maxmode = 3;    break;
    case(GRAB):             maxmode = 0;    break;
    case(GREQ):             maxmode = 2;    break;
    case(INVERT):           maxmode = 2;    break;
    case(LEAF):             maxmode = 0;    break;
    case(LEVEL):            maxmode = 0;    break;
    case(MAGNIFY):          maxmode = 0;    break;
    case(MAKE):             maxmode = 0;    break;
    case(MAX):              maxmode = 0;    break;
    case(MEAN):             maxmode = 8;    break;
    case(MORPH):            maxmode = 2;    break;
    case(NOISE):            maxmode = 0;    break;
    case(OCT):              maxmode = 3;    break;
    case(OCTVU):            maxmode = 0;    break;
    case(P_APPROX):         maxmode = 2;    break;
    case(P_CUT):            maxmode = 3;    break;
    case(P_EXAG):           maxmode = 6;    break;
    case(P_FIX):            maxmode = 0;    break;
    case(P_HEAR):           maxmode = 0;    break;
    case(P_INFO):           maxmode = 0;    break;
    case(P_INVERT):         maxmode = 2;    break;
    case(P_QUANTISE):       maxmode = 2;    break;
    case(P_RANDOMISE):      maxmode = 2;    break;
    case(P_SEE):            maxmode = 2;    break;
//TW NEW CASES
    case(P_INSERT):         maxmode = 2;    break;
    case(P_SINSERT):        maxmode = 2;    break;
    case(P_PTOSIL):         maxmode = 0;    break;
    case(P_NTOSIL):         maxmode = 0;    break;
    case(P_SYNTH):          maxmode = 0;    break;
    case(P_VOWELS):         maxmode = 0;    break;
    case(VFILT):            maxmode = 0;    break;
    case(P_GEN):            maxmode = 0;    break;
    case(ANALENV):          maxmode = 0;    break;
    case(P_BINTOBRK):       maxmode = 0;    break;
    case(MAKE2):            maxmode = 0;    break;
    case(P_INTERP):         maxmode = 2;    break;

    case(P_SMOOTH):         maxmode = 2;    break;
    case(P_TRANSPOSE):      maxmode = 0;    break;
    case(P_VIBRATO):        maxmode = 2;    break;
    case(P_WRITE):          maxmode = 0;    break;
    case(P_ZEROS):          maxmode = 0;    break;
    case(PEAK):             maxmode = 0;    break;
    case(PICK):             maxmode = 5;    break;
    case(PITCH):            maxmode = 2;    break;
    case(PLUCK):            maxmode = 0;    break;
    case(PRINT):            maxmode = 0;    break;
    case(REPITCH):          maxmode = 3;    break;
    case(REPITCHB):         maxmode = 3;    break;
    case(REPORT):           maxmode = 4;    break;
    case(SCAT):             maxmode = 0;    break;
    case(SHIFT):            maxmode = 5;    break;
    case(SHIFTP):           maxmode = 6;    break;
    case(SHUFFLE):          maxmode = 0;    break;
    case(SPLIT):            maxmode = 0;    break;
    case(SPREAD):           maxmode = 0;    break;
    case(STEP):             maxmode = 0;    break;
    case(STRETCH):          maxmode = 2;    break;
    case(SUM):              maxmode = 0;    break;
    case(SUPR):             maxmode = 0;    break;
    case(S_TRACE):          maxmode = 4;    break;
    case(TRACK):            maxmode = 2;    break;
    case(TRNSF):            maxmode = 4;    break;
    case(TRNSP):            maxmode = 4;    break;
    case(TSTRETCH):         maxmode = 2;    break;
    case(TUNE):             maxmode = 2;    break;
    case(VOCODE):           maxmode = 0;    break;
    case(WARP):             maxmode = 0;    break;
    case(WAVER):            maxmode = 2;    break;
    case(WEAVE):            maxmode = 0;    break;
    case(WINDOWCNT):        maxmode = 0;    break;
    case(LIMIT):            maxmode = 0;    break;
    case(MULTRANS):         maxmode = 0;    break;

    case(DISTORT):          maxmode = 8;    break;
    case(DISTORT_ENV):      maxmode = 4;    break;
    case(DISTORT_AVG):      maxmode = 0;    break;
    case(DISTORT_OMT):      maxmode = 0;    break;
    case(DISTORT_MLT):      maxmode = 0;    break;
    case(DISTORT_DIV):      maxmode = 0;    break;
    case(DISTORT_HRM):      maxmode = 0;    break;
    case(DISTORT_FRC):      maxmode = 0;    break;
    case(DISTORT_REV):      maxmode = 0;    break;
    case(DISTORT_SHUF):     maxmode = 0;    break;
    case(DISTORT_RPTFL):
    case(DISTORT_RPT2):
    case(DISTORT_RPT):      maxmode = 0;    break;
    case(DISTORT_INTP):     maxmode = 0;    break;
    case(DISTORT_DEL):      maxmode = 3;    break;
    case(DISTORT_RPL):      maxmode = 0;    break;
    case(DISTORT_TEL):      maxmode = 0;    break;
    case(DISTORT_FLT):      maxmode = 3;    break;
    case(DISTORT_INT):      maxmode = 2;    break;
    case(DISTORT_CYCLECNT): maxmode = 0;    break;
    case(DISTORT_PCH):      maxmode = 0;    break;
    case(DISTORT_OVERLOAD): maxmode = 2;    break;
//TW NEW CASE
    case(DISTORT_PULSED):   maxmode = 3;    break;

    case(ZIGZAG):           maxmode = 2;    break;
    case(LOOP):             maxmode = 3;    break;
    case(SCRAMBLE):         maxmode = 2;    break;
    case(ITERATE):          maxmode = 2;    break;
    case(ITERATE_EXTEND):   maxmode = 2;    break;
    case(DRUNKWALK):        maxmode = 2;    break;

    case(SIMPLE_TEX):   case(TIMED):
    case(DECORATED):case(PREDECOR): case(POSTDECOR):
    case(ORNATE):   case(PREORNATE):case(POSTORNATE):
    case(MOTIFS):   case(TMOTIFS):
    case(GROUPS):   case(TGROUPS):
                            maxmode = 5;    break;
    case(MOTIFSIN):
    case(TMOTIFSIN):        maxmode = 4;    break;

    case(GRAIN_REPITCH):
    case(GRAIN_RERHYTHM):
    case(GRAIN_REMOTIF):    maxmode = 2;    break;

    case(GRAIN_COUNT):      case(GRAIN_OMIT):       case(GRAIN_TIMEWARP):
    case(GRAIN_GET):        case(GRAIN_DUPLICATE):  case(GRAIN_ALIGN):
    case(GRAIN_POSITION):   case(GRAIN_REORDER):    case(GRAIN_REVERSE):
                            maxmode = 0;    break;
    case(RRRR_EXTEND):      maxmode = 3;    break;
    case(SSSS_EXTEND):      maxmode = 0;    break;
    case(ENV_CREATE):   case(ENV_EXTRACT):
                            maxmode = 2;    break;
    case(ENV_IMPOSE):   case(ENV_REPLACE):
                            maxmode = 4;    break;
    case(ENV_WARPING):  case(ENV_RESHAPING): case(ENV_REPLOTTING):
                            maxmode = 16;   break;
    case(ENV_CURTAILING):
                            maxmode = 6;    break;
    case(ENV_BRKTOENV):   case(ENV_ENVTOBRK):   case(ENV_ENVTODBBRK):
    case(ENV_DBBRKTOENV): case(ENV_DBBRKTOBRK): case(ENV_BRKTODBBRK):
//TW NEW CASE
//  case(ENV_SWELL):      case(ENV_PLUCK):
    case(ENV_SWELL):      case(ENV_PLUCK):      case(ENV_PROPOR):
                            maxmode = 0;    break;

    case(ENV_DOVETAILING):  maxmode = 2;    break;
    case(ENV_TREMOL):       maxmode = 2;    break;
    case(ENV_ATTACK):       maxmode = 4;    break;

    case(MIX):
    case(MIXTEST):
    case(MIXFORMAT):
    case(MIXGAIN):          maxmode = 0;    break;
    case(MIXDUMMY):         maxmode = 3;    break;
    case(MIXTWO):           maxmode = 0;    break;
//TW NEW CASE
    case(MIXMANY):          maxmode = 0;    break;
    case(MIXBALANCE):       maxmode = 0;    break;

    case(MIXCROSS):         maxmode = 2;    break;
    case(MIXINTERL):        maxmode = 0;    break;
    case(MIXINBETWEEN):     maxmode = 2;    break;
    case(CYCINBETWEEN):     maxmode = 0;    break;
    case(MIXSYNC):          maxmode = 3;    break;
    case(MIXSYNCATT):       maxmode = 0;    break;
    case(MIXMAX):           maxmode = 3;    break;
    case(MIXTWARP):         maxmode = 16;   break;
    case(MIXSWARP):         maxmode = 8;    break;
    case(MIXSHUFL):         maxmode = 7;    break;
//TW NEW CASES
    case(MIX_ON_GRID):      maxmode = 0;    break;
    case(AUTOMIX):          maxmode = 0;    break;
    case(MIX_PAN):          maxmode = 0;    break;
    case(MIX_AT_STEP):      maxmode = 0;    break;
    case(ADDTOMIX):         maxmode = 0;    break;

    case(EQ):               maxmode = 3;    break;
    case(LPHP):             maxmode = 2;    break;
    case(FSTATVAR):         maxmode = 4;    break;
    case(FLTBANKN):         maxmode = 6;    break;
    case(FLTBANKC):         maxmode = 6;    break;
    case(FLTBANKU):         maxmode = 2;    break;
    case(FLTBANKV):         maxmode = 2;    break;
    case(FLTBANKV2):        maxmode = 2;    break;
    case(FLTITER):          maxmode = 2;    break;
    case(FLTSWEEP):         maxmode = 4;    break;
    case(ALLPASS):          maxmode = 2;    break;

    case(MOD_LOUDNESS):     maxmode = 12;   break;
    case(MOD_SPACE):        maxmode = 4;    break;
//TW NEW CASES
    case(SCALED_PAN):       maxmode = 0;    break;
    case(FIND_PANPOS):      maxmode = 0;    break;

    case(MOD_PITCH):        maxmode = 6;    break;
    case(MOD_REVECHO):      maxmode = 3;    break;
    case(MOD_RADICAL):      maxmode = 7;    break;  
    case(BRASSAGE):         maxmode = 7;    break;
    case(SAUSAGE):          maxmode = 0;    break;

    case(PVOC_ANAL):        maxmode = 3;    break;
    case(PVOC_SYNTH):       maxmode = 0;    break;
    case(PVOC_EXTRACT):     maxmode = 0;    break;

    case(WORDCNT):          maxmode = 0;    break;

    case(EDIT_CUT):         maxmode = 3;    break;
    case(EDIT_CUTEND):      maxmode = 3;    break;
    case(EDIT_ZCUT):        maxmode = 2;    break;
    case(MANY_ZCUTS):       maxmode = 2;    break;
    case(EDIT_EXCISE):      maxmode = 3;    break;
    case(EDIT_EXCISEMANY):  maxmode = 3;    break;
    case(INSERTSIL_MANY):   maxmode = 3;    break;
    case(EDIT_INSERT):      maxmode = 3;    break;
//TW NEW CASE
    case(EDIT_INSERT2):     maxmode = 3;    break;
    case(EDIT_INSERTSIL):   maxmode = 3;    break;
    case(EDIT_JOIN):
    case(JOIN_SEQDYN):
    case(JOIN_SEQ):         maxmode = 0;    break;

    case(HOUSE_COPY):       maxmode = 2;    break;
    case(HOUSE_DEL):        maxmode = 0;    break;
    case(HOUSE_CHANS):      maxmode = 5;    break;
//TW NEW CASES REPLACING BRACKETED-OUT CODE (Dump & Recover abandoned)
    case(HOUSE_BAKUP):      maxmode = 0;    break;
    case(HOUSE_GATE):

    case(HOUSE_BUNDLE):     maxmode = 5;    break;
    case(HOUSE_SORT):       maxmode = 6;    break;
    case(HOUSE_SPEC):       maxmode = 3;    break;
    case(HOUSE_EXTRACT):    maxmode = 6;    break;
    case(TOPNTAIL_CLICKS):  maxmode = 0;    break;
    case(HOUSE_DISK):       maxmode = 0;    break;

    case(INFO_PROPS):       maxmode = 0;    break;
    case(INFO_SFLEN):       maxmode = 0;    break;
    case(INFO_TIMELIST):    maxmode = 0;    break;
    case(INFO_LOUDLIST):    maxmode = 0;    break;
    case(INFO_TIMESUM):     maxmode = 0;    break;
    case(INFO_TIMEDIFF):    maxmode = 0;    break;
    case(INFO_SAMPTOTIME):  maxmode = 0;    break;
    case(INFO_TIMETOSAMP):  maxmode = 0;    break;
    case(INFO_MAXSAMP):     maxmode = 0;    break;
    case(INFO_MAXSAMP2):    maxmode = 0;    break;
    case(INFO_LOUDCHAN):    maxmode = 0;    break;
    case(INFO_FINDHOLE):    maxmode = 0;    break;
    case(INFO_DIFF):        maxmode = 0;    break;
    case(INFO_CDIFF):       maxmode = 0;    break;
    case(INFO_PRNTSND):     maxmode = 0;    break;
    case(INFO_MUSUNITS):    maxmode = 31;   break;

    case(SYNTH_WAVE):       maxmode = 4;    break;
    case(MULTI_SYN):        maxmode = 3;    break;
    case(SYNTH_NOISE):      maxmode = 0;    break;
    case(SYNTH_SIL):        maxmode = 0;    break;
    case(SYNTH_SPEC):       maxmode = 0;    break;

    case(RANDCUTS):         maxmode = 0;    break;
    case(RANDCHUNKS):       maxmode = 0;    break;
    case(SIN_TAB):          maxmode = 0;    break;
    case(ACC_STREAM):       maxmode = 0;    break;

    case(HF_PERM1):         maxmode = 4;    break;
    case(HF_PERM2):         maxmode = 4;    break;
    case(DEL_PERM):         maxmode = 0;    break;
    case(DEL_PERM2):        maxmode = 0;    break;
    case(TWIXT):            maxmode = 4;    break;
    case(SPHINX):           maxmode = 3;    break;
//TW NEW CASES
    case(EDIT_CUTMANY):     maxmode = 3;    break;
    case(STACK):            maxmode = 0;    break;
    case(SHUDDER):          maxmode = 0;    break;
    case(NOISE_SUPRESS):    maxmode = 0;    break;
    case(TIME_GRID):        maxmode = 0;    break;
    case(SEQUENCER2):
    case(SEQUENCER):        maxmode = 0;    break;
    case(CONVOLVE):         maxmode = 2;    break;
    case(BAKTOBAK):         maxmode = 0;    break;
    case(CLICK):            maxmode = 2;    break;
    case(DOUBLETS):         maxmode = 0;    break;
    case(SYLLABS):          maxmode = 3;    break;
    case(MAKE_VFILT):       maxmode = 0;    break;
    case(MIX_MODEL):        maxmode = 0;    break;
    case(BATCH_EXPAND):     maxmode = 2;    break;
    case(ENVSYN):           maxmode = 4;    break;
    case(HOUSE_GATE2):      maxmode = 0;    break;
    case(GRAIN_ASSESS):     maxmode = 0;    break;
    case(ZCROSS_RATIO):     maxmode = 0;    break;
    case(GREV):             maxmode = 7;    break;
    default:
        sprintf(errstr,"Unknown case [%d] in get_maxmode()\n",process);
        return(-1);
    }
    return(maxmode);
}

/**********************************************************************************/
/*********************** ITEMS FORMERLY IN RANGES.C tklib2 ************************/
/**********************************************************************************/

/****************************** GET_PARAM_RANGES *********************************/

int get_param_ranges
(int process,int mode,int total_params,double nyquist,float frametime,float arate,int srate,
int wlength,int insams,int channels,int wanted,
int filetype,int linecnt,double duration,aplptr ap)
{
    int exit_status;
    if((exit_status = setup_input_param_range_stores(total_params,ap))<0)
        return(exit_status);
    return set_param_ranges(process,mode,nyquist,frametime,arate,srate,wlength,
                            insams,channels,wanted,filetype,linecnt,duration,ap);
}       
    
/****************************** SETUP_INPUT_PARAM_RANGE_STORES *********************************/

int setup_input_param_range_stores(int total_params,aplptr ap)
{
    if((ap->lo = (double *)malloc((total_params) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: range lo limits\n");
        return(MEMORY_ERROR);
    }
    if((ap->hi = (double *)malloc((total_params) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: range hi limits\n");
        return(MEMORY_ERROR);
    }
    return FINISHED;  /*RWD 9:2001 */
}


/****************************** SET_PARAM_RANGES *********************************/
/*RWD we ignore infilesize and use insams */
int set_param_ranges
(int process,int mode,double nyquist,float frametime,float arate,int srate,
int wlength,int insams,int channels,int wanted,
int filetype,int linecnt,double duration,aplptr ap)
{
    int    n;
    double infilesize_in_samps = 0.0;
    int    clength     = (int)(wanted/2);
    double chwidth     = nyquist/(double)(clength-1);
    double halfchwidth = chwidth/2.0;
    double sr          = (double)srate;

    switch(filetype) {
    case(SNDFILE):  
        infilesize_in_samps = (double)(insams/channels);    break;
    case(ENVFILE):
//TW THIS NOW SEEMS BOTH CRAZY and (fortunately) REDUNDANT !!!
        infilesize_in_samps = duration;                     break;
    }
    switch(process) {
    case(ACCU):
            /* variants */
        ap->lo[ACCU_DINDEX] = ACCU_MIN_DECAY;
        ap->hi[ACCU_DINDEX] = 1.0;
        ap->lo[ACCU_GINDEX] = -MAXGLISRATE/frametime; /* 1/16th octaves per window : guess!! */
        ap->hi[ACCU_GINDEX] = MAXGLISRATE/frametime;  /*     converted to 8vas per second    */
        break;
    case(ALT):
        break;
    case(ARPE):
            /* params */
        switch(mode) {
        case(ABOVE_BOOST):
        case(ONCE_ABOVE):
            ap->lo[ARPE_WTYPE]  = (double)(DOWNRAMP);   /* 1 */
            ap->hi[ARPE_WTYPE]  = (double)(SAW);        /* 3 */ /* i.e. NOT upramp */
            break;
        case(BELOW_BOOST):    
        case(ONCE_BELOW):     
            ap->lo[ARPE_WTYPE]  = (double)(SIN);        /* 2 */ /* i.e. NOT downramp */
            ap->hi[ARPE_WTYPE]  = (double)(UPRAMP);     /* 4 */
            break;
        default:
            ap->lo[ARPE_WTYPE]  = (double)(DOWNRAMP);   /* 1 */
            ap->hi[ARPE_WTYPE]  = (double)(UPRAMP);     /* 4 */
            break;
        }
        ap->lo[ARPE_ARPFRQ]     = 0.0;
        ap->hi[ARPE_ARPFRQ]     = arate;
            /* options */
        ap->lo[ARPE_PHASE]      = 0.0;
        ap->hi[ARPE_PHASE]      = 1.0;
        ap->lo[ARPE_LOFRQ]      = PITCHZERO;
        ap->hi[ARPE_LOFRQ]      = nyquist;
        ap->lo[ARPE_HIFRQ]      = PITCHZERO;
        ap->hi[ARPE_HIFRQ]      = nyquist;
        ap->lo[ARPE_HBAND]      = chwidth;
        ap->hi[ARPE_HBAND]      = nyquist;
        ap->lo[ARPE_AMPL]       = 0.0;
        ap->hi[ARPE_AMPL]       = ARPE_MAX_AMPL;
        switch(mode) {
        case(ON): case(BOOST): case(BELOW_BOOST): case(ABOVE_BOOST):
                /* variants */
            ap->lo[ARPE_SUST]   = 1.0;          /* 1 */
            ap->hi[ARPE_SUST]   = wlength;
            ap->lo[ARPE_NONLIN] = ARPE_MIN_NONLIN;
            ap->hi[ARPE_NONLIN] = ARPE_MAX_NONLIN;
            break;
        }
        break;
    case(AVRG):
            /* params */
        ap->lo[AVRG_AVRG]       = (double)2.0;      /* 2 */
        ap->hi[AVRG_AVRG]       = (double)clength;
        break;
    case(BARE):
        break;
    case(BLTR):
            /* params */
        ap->lo[BLUR_BLURF]      = 1.0;          /* 1 */ 
        ap->hi[BLUR_BLURF]      = (double)wlength;
        ap->lo[BLTR_TRACE]      = 1.0;          /* 1 */ 
        ap->hi[BLTR_TRACE]      = (double)clength;
        break;
    case(BLUR):
            /* params */
        ap->lo[BLUR_BLURF]      = 1.0;          /* 1 */ 
        ap->hi[BLUR_BLURF]      = (double)wlength;
        break;
    case(BRIDGE):
        if(insams <= 0) {
            sprintf(errstr,"insams not set: set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }
        ap->lo[BRG_OFFSET]      = 0.0;
        ap->hi[BRG_OFFSET]      = (wlength - 2) * frametime;
        ap->lo[BRG_SF2]         = 0.0;
        ap->hi[BRG_SF2]         = 1.0;
        ap->lo[BRG_SA2]         = 0.0;
        ap->hi[BRG_SA2]         = 1.0;
        ap->lo[BRG_EF2]         = 0.0;
        ap->hi[BRG_EF2]         = 1.0;
        ap->lo[BRG_EA2]         = 0.0;
        ap->hi[BRG_EA2]         = 1.0;
        ap->lo[BRG_STIME]       = 0.0;
        ap->hi[BRG_STIME]       = (wlength - 2) * frametime;
        ap->lo[BRG_ETIME]       = frametime * 2.0;
        ap->hi[BRG_ETIME]       = wlength * frametime;
        break;
    case(CHANNEL):
        ap->lo[CHAN_FRQ]        = 0.0;
        ap->hi[CHAN_FRQ]        = nyquist;
        break;
    case(CHORD):
    case(MULTRANS):
            /* variants */
        ap->lo[CHORD_LOFRQ]     = PITCHZERO;
        ap->hi[CHORD_LOFRQ]     = nyquist;
        ap->lo[CHORD_HIFRQ]     = PITCHZERO;
        ap->hi[CHORD_HIFRQ]     = nyquist;
        break;
    case(CHORUS):
            /* params */
        ap->lo[CHORU_AMPR]      = CHORU_MINAMP;
        ap->hi[CHORU_AMPR]      = CHORU_MAXAMP;
        ap->lo[CHORU_FRQR]      = CHORU_MINFRQ;
        ap->hi[CHORU_FRQR]      = CHORU_MAXFRQ;
        break;
    case(CLEAN):
        switch(mode) {
        case(FROMTIME):
        case(ANYWHERE):
            ap->lo[CL_SKIPT]    = 0.0;
            ap->hi[CL_SKIPT]    = (wlength - 1) * frametime;
            break;
        case(FILTERING):
            ap->lo[CL_FRQ]      = PITCHZERO;
            ap->hi[CL_FRQ]      = nyquist - halfchwidth;
            break;
        }
        ap->lo[CL_GAIN]         = 1.0;
        ap->hi[CL_GAIN]         = CL_MAX_GAIN;
        break;
    case(CROSS):
        ap->lo[CROS_INTP]       = 0.0;
        ap->hi[CROS_INTP]       = 1.0;
        break;
    case(CUT):
        ap->lo[CUT_STIME]       = 0.0;
        ap->hi[CUT_STIME]       = (wlength - 1) * frametime;
        ap->lo[CUT_ETIME]       = frametime;
        ap->hi[CUT_ETIME]       = wlength * frametime;
        break;
    case(DIFF):
        ap->lo[DIFF_CROSS]      = 0.0;
        ap->hi[DIFF_CROSS]      = 1.0;
        break;
    case(DRUNK):
        ap->lo[DRNK_RANGE]      = 1.0;
        ap->hi[DRNK_RANGE]      = (double)(wlength/2);
        ap->lo[DRNK_STIME]      = 0.0;
        ap->hi[DRNK_STIME]      = wlength * frametime;
        ap->lo[DRNK_DUR]        = frametime;
        ap->hi[DRNK_DUR]        = BIG_TIME;
        break;
    case(EXAG):
            /* params */
        ap->lo[EXAG_EXAG]       = 0.001;
        ap->hi[EXAG_EXAG]       = 1000.0;
        break;
    case(FILT):
            /* params */
        ap->lo[FILT_FRQ1]       = SPEC_MINFRQ;
        ap->hi[FILT_FRQ1]       = nyquist;
        ap->lo[FILT_FRQ2]       = SPEC_MINFRQ;
        ap->hi[FILT_FRQ2]       = nyquist;
        ap->lo[FILT_QQ]         = 1.0;
        ap->hi[FILT_QQ]         = nyquist;
        ap->lo[FILT_PG]         = 0.0;
        ap->hi[FILT_PG]         = FILT_MAX_PG;
        break;
    case(FMNTSEE):
        break;
    case(FOCUS):
            /* params */
        ap->lo[FOCU_PKCNT]      = 1.0;
        ap->hi[FOCU_PKCNT]      = (double)MAXPKCNT;
        ap->lo[FOCU_BW]         = OCTAVES_PER_SEMITONE;
        ap->hi[FOCU_BW]         = LOG2(nyquist/halfchwidth);
            /* options */
        ap->lo[FOCU_LOFRQ]      = PITCHZERO;
        ap->hi[FOCU_LOFRQ]      = nyquist;
        ap->lo[FOCU_HIFRQ]      = PITCHZERO;
        ap->hi[FOCU_HIFRQ]      = nyquist;
        ap->lo[FOCU_STABL]      = 2.0;
        ap->hi[FOCU_STABL]      = (double)MAXSTABIL;
        break;
    case(FOLD):
            /* params */
        ap->lo[FOLD_LOFRQ]      = PITCHZERO;
        ap->hi[FOLD_LOFRQ]      = nyquist;
        ap->lo[FOLD_HIFRQ]      = PITCHZERO;
        ap->hi[FOLD_HIFRQ]      = nyquist;
        break;
    case(FORM):
        ap->lo[FORM_FTOP]       = PITCHZERO;
        ap->hi[FORM_FTOP]       = nyquist;
        ap->lo[FORM_FBOT]       = PITCHZERO;
        ap->hi[FORM_FBOT]       = nyquist;
        ap->lo[FORM_GAIN]       = FLTERR;
        ap->hi[FORM_GAIN]       = FORM_MAX_GAIN;
        break;
    case(FORMANTS):
        break;
    case(FORMSEE):
        break;
    case(FREEZE):
    case(FREEZE2):
        break;
    case(FREQUENCY):
        ap->lo[FRQ_CHAN]        = 0.0;
        ap->hi[FRQ_CHAN]        = (double)(clength - 1);
        break;
    case(GAIN):
            /* params */
        ap->lo[GAIN_GAIN]       = 0.0;
        ap->hi[GAIN_GAIN]       =  GAIN_MAX_GAIN;
        break;
    case(GLIDE):
        ap->lo[GLIDE_DUR]       = frametime * 2.0;
        ap->hi[GLIDE_DUR]       = BIG_TIME;
        break;
    case(GLIS):
            /* params */
        ap->lo[GLIS_RATE]       = -8.0/frametime;
        ap->hi[GLIS_RATE]       =  8.0/frametime;
        ap->lo[GLIS_SHIFT]      = chwidth;
        ap->hi[GLIS_SHIFT]      = nyquist/2;
            /* variants */
        ap->lo[GLIS_HIFRQ]      = PITCHZERO;
        ap->hi[GLIS_HIFRQ]      = nyquist;
        break;
    case(GRAB):
        ap->lo[GRAB_FRZTIME]    = 0.0;
        ap->hi[GRAB_FRZTIME]    = duration + 1.0;
        break;
    case(GREQ):
        break;
    case(INVERT):
        break;
    case(LEAF):
        ap->lo[LEAF_SIZE]       = 1;
        ap->hi[LEAF_SIZE]       = wlength;
        break;
    case(LEVEL):
        break;
    case(MAGNIFY):
        ap->lo[MAG_FRZTIME]     = 0.0;
        ap->hi[MAG_FRZTIME]     = duration;
        ap->lo[MAG_DUR]         = frametime * 2.0;
        ap->hi[MAG_DUR]         = BIG_TIME;
        break;
    case(MAKE):     
        break;
    case(MAX):  
        break;
    case(MEAN):
        ap->lo[MEAN_LOF]        = PITCHZERO;
        ap->hi[MEAN_LOF]        = nyquist - halfchwidth;
        ap->lo[MEAN_HIF]        = PITCHZERO + chwidth;
        ap->hi[MEAN_HIF]        = nyquist;
        ap->lo[MEAN_CHAN]       = 2;
        ap->hi[MEAN_CHAN]       = clength;
        break;
    case(MORPH):
        ap->lo[MPH_STAG]        = 0.0;
        ap->hi[MPH_STAG]        = (wlength-1) * frametime;
        ap->lo[MPH_ASTT] = ap->lo[MPH_FSTT] = 0.0;
        ap->hi[MPH_ASTT] = ap->hi[MPH_FSTT] = (wlength - 1) * frametime;
        ap->lo[MPH_AEND] = ap->lo[MPH_FEND] = frametime * 2;
        ap->hi[MPH_AEND] = ap->hi[MPH_FEND] = BIG_TIME;
        ap->lo[MPH_AEXP] = ap->lo[MPH_FEXP] = MPH_MIN_EXP;
        ap->hi[MPH_AEXP] = ap->hi[MPH_FEXP] = MPH_MAX_EXP;
        break;
    case(NOISE):
        ap->lo[NOISE_NOIS]      =  0.0;
        ap->hi[NOISE_NOIS]      =  1.0;
        break;
    case(OCT):
        ap->lo[OCT_HMOVE]       = 2.0;
        ap->hi[OCT_HMOVE]       = OCT_MAX_HMOVE;
        ap->lo[OCT_BREI]        = 0.0;
        ap->hi[OCT_BREI]        = OCT_MAX_BREI;
        break;
    case(OCTVU):
        ap->lo[OCTVU_TSTEP]     = (double)round(frametime * SECS_TO_MS);
        ap->hi[OCTVU_TSTEP]     = (double)round(wlength * frametime * SECS_TO_MS);
        ap->lo[OCTVU_FUND]      = halfchwidth;
        ap->hi[OCTVU_FUND]      = nyquist/ROOT_2;
        break;
    case(P_APPROX):
        ap->lo[PA_PRANG]        = 0.0;
        ap->hi[PA_PRANG]        = MAXINTRANGE;
        ap->lo[PA_TRANG]        = frametime * SECS_TO_MS;
        ap->hi[PA_TRANG]        = wlength * frametime * SECS_TO_MS;
        ap->lo[PA_SRANG]        = frametime * BLOKCNT * 2 * SECS_TO_MS;
        ap->hi[PA_SRANG]        = wlength * frametime * SECS_TO_MS;
        break;
    case(P_CUT):
        ap->lo[PC_STT]          = 0.0;
        ap->hi[PC_STT]          = (wlength - 1) * frametime;
        ap->lo[PC_END]          = frametime;
        ap->hi[PC_END]          = wlength * frametime;
        break;
    case(P_EXAG):
        ap->lo[PEX_MEAN]        = MIDIMIN;
        ap->hi[PEX_MEAN]        = MIDIMAX;
        ap->lo[PEX_RANG]        = 0.0;
        ap->hi[PEX_RANG]        = PEX_MAX_RANG;
        ap->lo[PEX_CNTR]        = 0.0;
        ap->hi[PEX_CNTR]        = 1.0;
        break;
    case(P_FIX):
        ap->lo[PF_SCUT]         = 0.0;
        ap->hi[PF_SCUT]         = wlength * frametime;
        ap->lo[PF_ECUT]         = 0.0;
        ap->hi[PF_ECUT]         = wlength * frametime;
        ap->lo[PF_HIF]          = PITCHZERO;
        ap->hi[PF_HIF]          = nyquist;
        ap->lo[PF_LOF]          = PITCHZERO;
        ap->hi[PF_LOF]          = nyquist;
        ap->lo[PF_SMOOTH]       = 1.0;
        ap->hi[PF_SMOOTH]       = (double)(MAXSMOOTH);
        ap->lo[PF_SMARK]        = SPEC_MINFRQ;
        ap->hi[PF_SMARK]        = nyquist;
        ap->lo[PF_EMARK]        = SPEC_MINFRQ;
        ap->hi[PF_EMARK]        = nyquist;
        break;
    case(P_HEAR):
        ap->lo[PH_GAIN]         = FLTERR;
        ap->hi[PH_GAIN]         = PH_MAX_GAIN;
        break;
    case(P_INFO):     
        break;
    case(P_INVERT):
        ap->lo[PI_MEAN]         = MIDIMIN;
        ap->hi[PI_MEAN]         = MIDIMAX;
        ap->lo[PI_TOP]          = MIDIMIN;
        ap->hi[PI_TOP]          = MIDIMAX;
        ap->lo[PI_BOT]          = MIDIMIN;
        ap->hi[PI_BOT]          = MIDIMAX;
        break;
    case(P_QUANTISE): 
        break;
    case(P_RANDOMISE):
        ap->lo[PR_MXINT]        = 0.0;
        ap->hi[PR_MXINT]        = MAXINTRANGE;
        ap->lo[PR_TSTEP]        = frametime * SECS_TO_MS;
        ap->hi[PR_TSTEP]        = wlength * frametime * SECS_TO_MS;
        ap->lo[PR_SLEW]         = -(PR_MAX_SLEW);
        ap->hi[PR_SLEW]         = PR_MAX_SLEW;
        break;
    case(P_SEE):
        ap->lo[PSEE_SCF]        = FLTERR;
        ap->hi[PSEE_SCF]        = PSEE_MAX_SCF;
        break;
    case(P_SMOOTH):
        ap->lo[PS_TFRAME]       = frametime * SECS_TO_MS;
        ap->hi[PS_TFRAME]       = wlength * frametime * SECS_TO_MS;
        ap->lo[PS_MEAN]         = MIDIMIN;
        ap->hi[PS_MEAN]         = MIDIMAX;
        break;
//TW NEW CASES
    case(P_SYNTH):
    case(P_INSERT):
    case(P_SINSERT):
    case(P_PTOSIL):
    case(P_NTOSIL):
    case(ANALENV):
    case(MAKE2):
    case(P_INTERP):
    case(P_BINTOBRK):
        break;
    case(P_VOWELS):
        ap->lo[PV_HWIDTH] = .01;
        ap->hi[PV_HWIDTH] = 10.0;
        ap->lo[PV_CURVIT] = .1;
        ap->hi[PV_CURVIT] = 10;
        ap->lo[PV_PKRANG] = 0;
        ap->hi[PV_PKRANG] = 1;
        ap->lo[PV_FUNBAS] = 0;
        ap->hi[PV_FUNBAS] = 1;
        ap->lo[PV_OFFSET] = 0.0;
        ap->hi[PV_OFFSET] = 1.0;
        break;
    case(VFILT):
        ap->lo[PV_HWIDTH] = .01;
        ap->hi[PV_HWIDTH] = 10.0;
        ap->lo[PV_CURVIT] = .1;
        ap->hi[PV_CURVIT] = 10;
        ap->lo[PV_PKRANG] = 0;
        ap->hi[PV_PKRANG] = 1;
        ap->lo[VF_THRESH] = 0.0;
        ap->hi[VF_THRESH] = 1.0;
        break;
    case(P_GEN):
        ap->lo[PGEN_SRATE] = 16000;
        ap->hi[PGEN_SRATE] = /*48000*/96000;    /*RWD 4:05 */
        ap->lo[PGEN_CHANS_INPUT]    = (double)2;
        ap->hi[PGEN_CHANS_INPUT]    = (double)MAX_PVOC_CHANS;
        ap->lo[PGEN_WINOVLP_INPUT]  = (double)1;
        ap->hi[PGEN_WINOVLP_INPUT]  = (double)4;
        break;

    case(P_TRANSPOSE):
        ap->lo[PT_TVAL]     = -MAXINTRANGE;
        ap->hi[PT_TVAL]     = MAXINTRANGE;
        break;
    case(P_VIBRATO):
        ap->lo[PV_FRQ]          = 0.0;
        ap->hi[PV_FRQ]          = 1.0/frametime;
        ap->lo[PV_RANG]         = 0.0;
        ap->hi[PV_RANG]         = MAXINTRANGE;
        break;
    case(P_WRITE):
        ap->lo[PW_DRED]         = PW_MIN_DRED;
        ap->hi[PW_DRED]         = PW_MAX_DRED;
        break;
    case(P_ZEROS):  
        break;
    case(PEAK):
        ap->lo[PEAK_CUTOFF]     = SPEC_MINFRQ;
        ap->hi[PEAK_CUTOFF]     = nyquist - halfchwidth;
        ap->lo[PEAK_TWINDOW]    = frametime;
        ap->hi[PEAK_TWINDOW]    = (double)wlength * frametime;
        ap->lo[PEAK_FWINDOW]    = MINFWINDOW;
        ap->hi[PEAK_FWINDOW]    = LOG2(nyquist/SPEC_MINFRQ) * SEMITONES_PER_OCTAVE;
        break;
    case(PICK):
            /* params */
        ap->lo[PICK_FUND]       = SPEC_MINFRQ;
        ap->hi[PICK_FUND]       = nyquist;
        if(mode==PIK_DISPLACED_HARMS)
            ap->lo[PICK_LIN]    = -nyquist;
        else
            ap->lo[PICK_LIN]    = SPEC_MINFRQ;
        ap->hi[PICK_LIN]        = nyquist;
            /* options */
        ap->lo[PICK_CLAR]       =  0.0;
        ap->hi[PICK_CLAR]       =  1.0;
        break;
    case(PITCH):
        ap->lo[PICH_RNGE]       = 0.0;
        ap->hi[PICH_RNGE]       = 6.0;
        ap->lo[PICH_VALID]      = 0.0;
        ap->hi[PICH_VALID]      = (double)wlength;
        ap->lo[PICH_SRATIO]     = 0.0;
        ap->hi[PICH_SRATIO]     = SIGNOIS_MAX;
        ap->lo[PICH_MATCH]      = 1.0;
        ap->hi[PICH_MATCH]      = (double)MAXIMI;
        ap->lo[PICH_HILM]       = SPEC_MINFRQ;
        ap->hi[PICH_HILM]       = nyquist/MAXIMI;
        ap->lo[PICH_LOLM]       = SPEC_MINFRQ;
        ap->hi[PICH_LOLM]       = nyquist/MAXIMI;
        if(mode==PICH_TO_BRK) {
            ap->lo[PICH_DATAREDUCE] = PW_MIN_DRED;
            ap->hi[PICH_DATAREDUCE] = PW_MAX_DRED;
        }
        break;
    case(PLUCK):
            /* params */
        ap->lo[PLUK_GAIN]       = 0.0;
        ap->hi[PLUK_GAIN]       =  PLUK_MAX_GAIN;
        break;
    case(PRINT):
        ap->lo[PRNT_STIME]      = 0.0;
        ap->hi[PRNT_STIME]      = (wlength - 1) * frametime;
        ap->lo[PRNT_WCNT]       = 1.0;
        ap->hi[PRNT_WCNT]       = wlength;
        break;
    case(REPITCH):  
        break;
    case(REPITCHB):
        ap->lo[RP_DRED]         = PW_MIN_DRED;
        ap->hi[RP_DRED]         = PW_MAX_DRED;
        break;
    case(REPORT):
            /* params */
        ap->lo[REPORT_PKCNT]    = 1.0;
        ap->hi[REPORT_PKCNT]    = (double)MAXPKCNT;
            /* options */
        ap->lo[REPORT_LOFRQ]    = PITCHZERO;
        ap->hi[REPORT_LOFRQ]    = nyquist;
        ap->lo[REPORT_HIFRQ]    = PITCHZERO;
        ap->hi[REPORT_HIFRQ]    = nyquist;
        ap->lo[REPORT_STABL]    = 2.0;
        ap->hi[REPORT_STABL]    = (double)MAXSTABIL;
        break;
    case(SCAT):
            /* params */
        ap->lo[SCAT_CNT]        =  1.0;
        ap->hi[SCAT_CNT]        =  (double)clength;
            /* options */
        ap->lo[SCAT_BLOKSIZE]   =  chwidth;
        ap->hi[SCAT_BLOKSIZE]   =  nyquist/2.0;
        break;
    case(SHIFT):
            /* params */
        ap->lo[SHIFT_SHIF]      =  -nyquist;
        ap->hi[SHIFT_SHIF]      =  nyquist;
        ap->lo[SHIFT_FRQ1]      =  halfchwidth/2.0;
        ap->hi[SHIFT_FRQ1]      =  nyquist - (halfchwidth/2.0);
        ap->lo[SHIFT_FRQ2]      =  halfchwidth/2.0;
        ap->hi[SHIFT_FRQ2]      =  nyquist - (halfchwidth/2.0);
        break;
    case(SHIFTP):
            /* params */
        ap->lo[SHIFTP_FFRQ]     = PITCHZERO;
        ap->hi[SHIFTP_FFRQ]     = nyquist;
        ap->lo[SHIFTP_SHF1]     = LOG2(SPEC_MINFRQ/nyquist) * SEMITONES_PER_OCTAVE;
        ap->hi[SHIFTP_SHF1]     = LOG2(nyquist/SPEC_MINFRQ) * SEMITONES_PER_OCTAVE;
        ap->lo[SHIFTP_SHF2]     = LOG2(SPEC_MINFRQ/nyquist) * SEMITONES_PER_OCTAVE;
        ap->hi[SHIFTP_SHF2]     = LOG2(nyquist/SPEC_MINFRQ) * SEMITONES_PER_OCTAVE;
            /* variants */
        ap->lo[SHIFTP_DEPTH]    = 0.0;
        ap->hi[SHIFTP_DEPTH]    = 1.0;
        break;
    case(SHUFFLE):
        ap->lo[SHUF_GRPSIZE]    = 1.0;
        ap->hi[SHUF_GRPSIZE]    = 32767.0;
        break;
    case(SPLIT): 
        break;
    case(SPREAD):
            /* options */
        ap->lo[SPREAD_SPRD]     = 0.0;
        ap->hi[SPREAD_SPRD]     = 1.0;
        break;
    case(STEP):
            /* params */
        ap->lo[STEP_STEP]       = 2.0 * frametime;
        ap->hi[STEP_STEP]       = wlength * frametime;
        break;
    case(STRETCH):
            /* params */
        ap->lo[STR_FFRQ]        = PITCHZERO;
        ap->hi[STR_FFRQ]        = nyquist;
        ap->lo[STR_SHIFT]       = SPEC_MINFRQ/nyquist;
        ap->hi[STR_SHIFT]       = nyquist/SPEC_MINFRQ;
        ap->lo[STR_EXP]         = STR_MIN_EXP;
        ap->hi[STR_EXP]         = STR_MAX_EXP;
            /* variants */
        ap->lo[STR_DEPTH]       = 0.0;
        ap->hi[STR_DEPTH]       = 1.0;
        break;
    case(SUM):
        ap->lo[SUM_CROSS]       = 0.0;
        ap->hi[SUM_CROSS]       = 1.0;
        break;
    case(SUPR):
            /* params */
        ap->lo[SUPR_INDX]       = 1.0;
        ap->hi[SUPR_INDX]       = (double)clength;
        break;
    case(S_TRACE):
            /* params */
        ap->lo[TRAC_INDX]       = 1.0;
        ap->hi[TRAC_INDX]       = (double)clength;
        ap->lo[TRAC_LOFRQ]      = PITCHZERO;
        ap->hi[TRAC_LOFRQ]      = nyquist;
        ap->lo[TRAC_HIFRQ]      = PITCHZERO;
        ap->hi[TRAC_HIFRQ]      = nyquist;
        break;
    case(TRACK):
        ap->lo[TRAK_PICH]       = SPEC_MINFRQ;
        ap->hi[TRAK_PICH]       = nyquist/MAXIMI;
        ap->lo[TRAK_RNGE]       = 0.0;
        ap->hi[TRAK_RNGE]       = 6.0;
        ap->lo[TRAK_VALID]      = 1.0;
        ap->hi[TRAK_VALID]      = (double)wlength;
        ap->lo[TRAK_SRATIO]     = 0.0;
        ap->hi[TRAK_SRATIO]     = SIGNOIS_MAX;;
        ap->lo[TRAK_HILM]       = SPEC_MINFRQ + FLTERR;
        ap->hi[TRAK_HILM]       = nyquist/MAXIMI;
        if(mode==TRK_TO_BRK) {
            ap->lo[TRAK_DATAREDUCE] = PW_MIN_DRED;
            ap->hi[TRAK_DATAREDUCE] = PW_MAX_DRED;
        }
        break;
    case(TRNSF):
            /* variants */
        ap->lo[TRNSF_HIFRQ]     = PITCHZERO;
        ap->hi[TRNSF_HIFRQ]     = nyquist;
        ap->lo[TRNSF_LOFRQ]     = PITCHZERO;
        ap->hi[TRNSF_LOFRQ]     = nyquist;
        break;
    case(TRNSP):
            /* variants */
        ap->lo[TRNSP_HIFRQ]     = PITCHZERO;
        ap->hi[TRNSP_HIFRQ]     = nyquist;
        ap->lo[TRNSP_LOFRQ]     = PITCHZERO;
        ap->hi[TRNSP_LOFRQ]     = nyquist;
        break;
    case(TSTRETCH):
        ap->lo[TSTR_STRETCH]    = MINTSTR;
        ap->hi[TSTR_STRETCH]    = MAXTSTR;
        break;
    case(TUNE):
            /* options */
        ap->lo[TUNE_FOC]        = 0.0;
        ap->hi[TUNE_FOC]        = 1.0;
        ap->lo[TUNE_CLAR]       = 0.0;
        ap->hi[TUNE_CLAR]       = 1.0;
            /* variants */
        ap->lo[TUNE_INDX]       = 1.0;
        ap->hi[TUNE_INDX]       = clength;
        ap->lo[TUNE_BFRQ]       = MINPITCH;
        ap->hi[TUNE_BFRQ]       = nyquist;
        break;
    case(VOCODE):
        ap->lo[VOCO_LOF]        = PITCHZERO;
        ap->hi[VOCO_LOF]        = nyquist;
        ap->lo[VOCO_HIF]        = PITCHZERO;
        ap->hi[VOCO_HIF]        = nyquist;
        ap->lo[VOCO_GAIN]       = FLTERR;
        ap->hi[VOCO_GAIN]       = VOCO_MAX_GAIN;
        break;
    case(WARP):
        ap->lo[WARP_PRNG]       = 0.0;
        ap->hi[WARP_PRNG]       = MAXPRANGE;
        ap->lo[WARP_TRNG]       = frametime * SECS_TO_MS;
        ap->hi[WARP_TRNG]       = wlength * frametime * SECS_TO_MS;
        ap->lo[WARP_SRNG]       = (BLOKCNT * 2) * frametime * SECS_TO_MS;
        ap->hi[WARP_SRNG]       = (wlength - (BLOKCNT * 2)) * frametime * SECS_TO_MS;
        break;
    case(WAVER):
            /* params */
        ap->lo[WAVER_VIB]       = 1.0/(wlength * frametime);
        ap->hi[WAVER_VIB]       = 0.5/frametime;
        ap->lo[WAVER_STR]       = 1.0;
        ap->hi[WAVER_STR]       = nyquist/SPEC_MINFRQ;
        ap->lo[WAVER_LOFRQ]     = PITCHZERO;
        ap->hi[WAVER_LOFRQ]     = nyquist; 
        ap->lo[WAVER_EXP]       =  WAVER_MIN_EXP;
        ap->hi[WAVER_EXP]       =  WAVER_MAX_EXP;
        break;
    case(WEAVE):    
        break;
    case(WINDOWCNT):
        break;
    case(LIMIT):
        ap->lo[LIMIT_THRESH]    =  0.0;
        ap->hi[LIMIT_THRESH]    =  1.0;
        break;
                    /*********************** GROUCHO ***************************/

    case(DISTORT):
        ap->lo[DISTORT_POWFAC]      = FLTERR;
        ap->hi[DISTORT_POWFAC]      = DISTORT_MAX_POWFAC;
        break;
    case(DISTORT_ENV):
        ap->lo[DISTORTE_CYCLECNT]   = 1.0;
        ap->hi[DISTORTE_CYCLECNT]   = 1000.0;
        ap->lo[DISTORTE_TROF]       = 0.0;
        ap->hi[DISTORTE_TROF]       = 1.0;
        ap->lo[DISTORTE_EXPON]      = DISTORTE_MIN_EXPON;
        ap->hi[DISTORTE_EXPON]      = DISTORTE_MAX_EXPON;
        break;
    case(DISTORT_AVG):
        ap->lo[DISTORTA_CYCLECNT]   = 2.0;
        ap->hi[DISTORTA_CYCLECNT]   = MAX_CYCLECNT;
        ap->lo[DISTORTA_MAXLEN]     = MINWAVELEN/nyquist;
        ap->hi[DISTORTA_MAXLEN]     = 1.0;
        ap->lo[DISTORTA_SKIPCNT]    = 0.0;
        ap->hi[DISTORTA_SKIPCNT]    = MAX_CYCLECNT;
        break;
    case(DISTORT_OMT):
        ap->lo[DISTORTO_OMIT]       = 1.0;
        ap->hi[DISTORTO_OMIT]       = MAX_CYCLECNT;
        ap->lo[DISTORTO_KEEP]       = 2.0;
        ap->hi[DISTORTO_KEEP]       = MAX_CYCLECNT + 1.0;
        break;
    case(DISTORT_MLT):
    case(DISTORT_DIV):
        ap->lo[DISTORTM_FACTOR]     = 2.0;
        ap->hi[DISTORTM_FACTOR]     = 16.0;
        break;
    case(DISTORT_HRM):
        ap->lo[DISTORTH_PRESCALE]   = FLTERR;
        ap->hi[DISTORTH_PRESCALE]   = DISTORTH_MAX_PRESCALE;
        break;
    case(DISTORT_FRC):
        ap->lo[DISTORTF_SCALE]      = MIN_SCALE;
        ap->hi[DISTORTF_SCALE]      = MAXWAVELEN * sr;
        ap->lo[DISTORTF_AMPFACT]    = 1.0/(double)MAXSHORT;
        ap->hi[DISTORTF_AMPFACT]    = (double)MAXSHORT;
        ap->lo[DISTORTF_PRESCALE]   = 1.0/(double)MAXSHORT;
        ap->hi[DISTORTF_PRESCALE]   = (double)MAXSHORT;
        break;
    case(DISTORT_REV):
        ap->lo[DISTORTR_CYCLECNT]   = 1.0;
        ap->hi[DISTORTR_CYCLECNT]   = MAX_CYCLECNT;
        break;
    case(DISTORT_SHUF):
        ap->lo[DISTORTS_CYCLECNT]   = 1.0;
        ap->hi[DISTORTS_CYCLECNT]   = MAX_CYCLECNT;
        ap->lo[DISTORTS_SKIPCNT]    = 0.0;
        ap->hi[DISTORTS_SKIPCNT]    = MAX_CYCLECNT;
        break;
    case(DISTORT_RPTFL):
        ap->lo[DISTRPT_CYCLIM]      = 440.0;
        ap->hi[DISTRPT_CYCLIM]      = nyquist;
        /* fall thro */
    case(DISTORT_RPT):
    case(DISTORT_RPT2):
        ap->lo[DISTRPT_MULTIPLY]    = 2.0;
        ap->hi[DISTRPT_MULTIPLY]    = BIG_VALUE;
        ap->lo[DISTRPT_CYCLECNT]    = 1.0;
        ap->hi[DISTRPT_CYCLECNT]    = MAX_CYCLECNT;
        ap->lo[DISTRPT_SKIPCNT]     = 0.0;
        ap->hi[DISTRPT_SKIPCNT]     = MAX_CYCLECNT;
        break;
    case(DISTORT_INTP):
        ap->lo[DISTINTP_MULTIPLY]   = 2.0;
        ap->hi[DISTINTP_MULTIPLY]   = BIG_VALUE;
        ap->lo[DISTINTP_SKIPCNT]    = 0.0;
        ap->hi[DISTINTP_SKIPCNT]    = MAX_CYCLECNT;
        break;
    case(DISTORT_DEL):
        ap->lo[DISTDEL_CYCLECNT]    = 2.0;
        ap->hi[DISTDEL_CYCLECNT]    = MAX_CYCLECNT;
        ap->lo[DISTDEL_SKIPCNT]     = 0.0;
        ap->hi[DISTDEL_SKIPCNT]     = MAX_CYCLECNT;
        break;
    case(DISTORT_RPL):
        ap->lo[DISTRPL_CYCLECNT]    = 2.0;
        ap->hi[DISTRPL_CYCLECNT]    = MAX_CYCLECNT;
        ap->lo[DISTRPL_SKIPCNT]     = 0.0;
        ap->hi[DISTRPL_SKIPCNT]     = MAX_CYCLECNT;
        break;
    case(DISTORT_TEL):
        ap->lo[DISTTEL_CYCLECNT]    = 2.0;
        ap->hi[DISTTEL_CYCLECNT]    = MAX_CYCLECNT;
        ap->lo[DISTTEL_SKIPCNT]     = 0.0;
        ap->hi[DISTTEL_SKIPCNT]     = MAX_CYCLECNT;
        break;
    case(DISTORT_FLT):
        ap->lo[DISTFLT_LOFRQ_CYCLELEN] = SPEC_MINFRQ;
        ap->hi[DISTFLT_LOFRQ_CYCLELEN] = nyquist;
        ap->lo[DISTFLT_HIFRQ_CYCLELEN] = SPEC_MINFRQ;
        ap->hi[DISTFLT_HIFRQ_CYCLELEN] = nyquist;
        ap->lo[DISTFLT_SKIPCNT]        = 0.0;
        ap->hi[DISTFLT_SKIPCNT]        = MAX_CYCLECNT;
        break;
    case(DISTORT_INT):
        break;
    case(DISTORT_CYCLECNT):
        break;
    case(DISTORT_PCH):
        ap->lo[DISTPCH_OCTVAR]      = FLTERR;
        ap->hi[DISTPCH_OCTVAR]      = MAXOCTVAR;
        ap->lo[DISTPCH_CYCLECNT]    = 2.0;
        ap->hi[DISTPCH_CYCLECNT]    = MAX_CYCLECNT;
        ap->lo[DISTPCH_SKIPCNT]     = 0.0;
        ap->hi[DISTPCH_SKIPCNT]     = MAX_CYCLECNT;
        break;
    case(DISTORT_OVERLOAD):
        if(mode==OVER_SINE) {
            ap->lo[DISTORTER_FRQ]   = 0.01;
            ap->hi[DISTORTER_FRQ]   = nyquist;
        }
        ap->lo[DISTORTER_MULT]      = 0.0;
        ap->hi[DISTORTER_MULT]      = 1.0;
        ap->lo[DISTORTER_DEPTH]     = 0.0;
        ap->hi[DISTORTER_DEPTH]     = 1.0;
        break;
//TW NEW CASE
    case(DISTORT_PULSED):
        ap->lo[PULSE_STARTTIME] = 0.0;  /* starttime of impulses in input-sound */
        if(mode == PULSE_SYNI)
            ap->hi[PULSE_STARTTIME] = (double)round(duration * sr); /* starttime of impulses in input-sound, in samples */
        else
            ap->hi[PULSE_STARTTIME] = duration; /* starttime of impulses in input-sound */
        ap->lo[PULSE_DUR]       = (double)(ENDBIT_SPLICE * 2.0)/(double)srate;  /* duration of impulse stream */
        if(mode == PULSE_IMP)
            ap->hi[PULSE_DUR]   = duration; /* duration of impulse stream */
        else
            ap->hi[PULSE_DUR]   = AN_HOUR;
        ap->lo[PULSE_FRQ]       = .1;   /* frq of impulses */
        ap->hi[PULSE_FRQ]       = 50.0; /* frq of impulses */
        ap->lo[PULSE_FRQRAND]   = 0.0;  /* randomisation of pulse frq, semitones */
        ap->hi[PULSE_FRQRAND]   = 12.0; /* randomisation of pulse frq, semitones */
        ap->lo[PULSE_TIMERAND]  = 0.0;  /* randomisation of pulse shape, timewise */
        ap->hi[PULSE_TIMERAND]  = 1.0;  /* randomisation of pulse shape, timewise */
        ap->lo[PULSE_SHAPERAND] = 0.0;  /* randomisation of pulse shape, ampwise  */
        ap->hi[PULSE_SHAPERAND] = 1.0;  /* randomisation of pulse shape, ampwise  */
        if(mode == PULSE_SYNI) {
            ap->lo[PULSE_WAVETIME]  = 1.0;      /* no. of wavesets to cycle-around, within impulse [synth2 option only] */
            ap->hi[PULSE_WAVETIME]  = 256.0;    /* no. of wavesets to cycle-around, within impulse [synth2 option only] */
        } else {
            ap->lo[PULSE_WAVETIME]  = .001; /* duration of wavesets to cycle-around, within impulse [synth option only] */
            ap->hi[PULSE_WAVETIME]  = 4;    /* duration of wavesets to cycle-around, within impulse [synth option only] */
        }
        ap->lo[PULSE_TRANSPOS]  = -12.0;/* transposition envelope of material inside impulse */
        ap->hi[PULSE_TRANSPOS]  = 12.0; /* transposition envelope of material inside impulse */
        ap->lo[PULSE_PITCHRAND] = 0.0;  /* semitone range of randomisation of transposition envelope */
        ap->hi[PULSE_PITCHRAND] = 12.0; /* semitone range of randomisation of transposition envelope */
        break;

    case(ZIGZAG):
        ap->lo[ZIGZAG_START]        = 0.0;
        ap->hi[ZIGZAG_START]        = duration - (ZIG_SPLICELEN * MS_TO_SECS);
        ap->lo[ZIGZAG_END]          = ((ZIG_SPLICELEN * 2) + ZIG_MIN_UNSPLICED) * MS_TO_SECS;
        ap->hi[ZIGZAG_END]          = duration;
        ap->lo[ZIGZAG_DUR]          = duration + FLTERR;
        ap->hi[ZIGZAG_DUR]          = BIG_TIME;
        ap->lo[ZIGZAG_MIN]          = ((ZIG_SPLICELEN * 2) + ZIG_MIN_UNSPLICED) * MS_TO_SECS;
        ap->hi[ZIGZAG_MIN]          = duration - (2 * ZIG_SPLICELEN * MS_TO_SECS);
        ap->lo[ZIGZAG_SPLEN]        = MIN_ZIGSPLICE;
        ap->hi[ZIGZAG_SPLEN]        = MAX_ZIGSPLICE;
        if(mode==ZIGZAG_SELF) {
            ap->lo[ZIGZAG_MAX]      = ((ZIG_SPLICELEN * 2) + ZIG_MIN_UNSPLICED) * MS_TO_SECS;
            ap->hi[ZIGZAG_MAX]      = duration - (2 * ZIG_SPLICELEN * MS_TO_SECS);
            ap->lo[ZIGZAG_RSEED]    = 0.0;
            ap->hi[ZIGZAG_RSEED]    = MAXSHORT;
        }
        break;
    case(LOOP):
        ap->lo[LOOP_OUTDUR]         = 0.0;
        ap->hi[LOOP_OUTDUR]         = BIG_TIME;
        ap->lo[LOOP_REPETS]         = 1;
        ap->hi[LOOP_REPETS]         = BIG_VALUE;
        ap->lo[LOOP_START]          = 0.0;
        ap->hi[LOOP_START]          = duration - (2 * ZIG_SPLICELEN * MS_TO_SECS);
/* OLD
        ap->lo[LOOP_LEN]            = (ZIG_SPLICELEN * 2) + ZIG_MIN_UNSPLICED;
 NEW */
        ap->lo[LOOP_LEN]            = (1.0/sr) * SECS_TO_MS;
        ap->hi[LOOP_LEN]            = (duration * SECS_TO_MS) - (2 * ZIG_SPLICELEN);

        
        if(mode==LOOP_ALL)
            ap->lo[LOOP_STEP]       = (1.0/sr) * SECS_TO_MS;
        else
            ap->lo[LOOP_STEP]       = 0.0;
        ap->hi[LOOP_STEP]           = duration * SECS_TO_MS;
        ap->lo[LOOP_SPLEN]          = MIN_ZIGSPLICE;
        ap->hi[LOOP_SPLEN]          = MAX_ZIGSPLICE;
        ap->lo[LOOP_SRCHF]          = 0.0;
        ap->hi[LOOP_SRCHF]          = duration * SECS_TO_MS;
        break;
    case(SCRAMBLE):
        switch(mode) {
        case(SCRAMBLE_RAND):
            ap->lo[SCRAMBLE_MIN]    = ((2 * ZIG_SPLICELEN) + ZIG_MIN_UNSPLICED) * MS_TO_SECS;
            ap->hi[SCRAMBLE_MIN]    = duration - (ZIG_SPLICELEN * MS_TO_SECS);
            ap->lo[SCRAMBLE_MAX]    = ((2 * ZIG_SPLICELEN) + ZIG_MIN_UNSPLICED) * MS_TO_SECS;
            ap->hi[SCRAMBLE_MAX]    = duration - (ZIG_SPLICELEN * MS_TO_SECS);
            break;
        case(SCRAMBLE_SHRED):
            ap->lo[SCRAMBLE_LEN]    = (4 * ZIG_SPLICELEN) * MS_TO_SECS;
            ap->hi[SCRAMBLE_LEN]    = (duration/2.0) - (ZIG_SPLICELEN * MS_TO_SECS);
            if(ap->hi[SCRAMBLE_LEN] <= ap->lo[SCRAMBLE_LEN]) {
                sprintf(errstr,"infile too short for this process.\n");
                return(DATA_ERROR);
            }
            ap->lo[SCRAMBLE_SCAT]   = 0.0;
            ap->hi[SCRAMBLE_SCAT]   = BIG_VALUE;
            break;
        default:
            sprintf(errstr,"Unknown mode for SCRAMBLE: in set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }
        ap->lo[SCRAMBLE_DUR]        = ((2 * ZIG_SPLICELEN) + ZIG_MIN_UNSPLICED) * MS_TO_SECS;
        ap->hi[SCRAMBLE_DUR]        = (double)BIG_TIME;
        ap->lo[SCRAMBLE_SPLEN]      = MIN_ZIGSPLICE;
        ap->hi[SCRAMBLE_SPLEN]      = MAX_ZIGSPLICE;
        ap->lo[SCRAMBLE_SEED]       = 0.0;
        ap->hi[SCRAMBLE_SEED]       = MAXSHORT;
        break;
    case(ITERATE):
        switch(mode) {
        case(ITERATE_DUR):
            ap->lo[ITER_DUR]        = duration;
            ap->hi[ITER_DUR]        = BIG_TIME;
            break;
        case(ITERATE_REPEATS):
            ap->lo[ITER_REPEATS]    = 1.0;
            ap->hi[ITER_REPEATS]    = BIG_VALUE;
            break;
        default:
            sprintf(errstr,"Unknown mode for ITERATE: in set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }
        ap->lo[ITER_DELAY]          = FLTERR;
        ap->hi[ITER_DELAY]          = ITER_MAX_DELAY;
        ap->lo[ITER_RANDOM]         = 0.0;
        ap->hi[ITER_RANDOM]         = 1.0;
        ap->lo[ITER_PSCAT]          = 0.0;
        ap->hi[ITER_PSCAT]          = ITER_MAXPSHIFT;   
        ap->lo[ITER_ASCAT]          = 0.0;
        ap->hi[ITER_ASCAT]          = 1.0;
        ap->lo[ITER_FADE]           = 0.0;
        ap->hi[ITER_FADE]           = 1.0;
        ap->lo[ITER_GAIN]           = 0.0;
        ap->hi[ITER_GAIN]           = 1.0;
        ap->lo[ITER_RSEED]          = 0.0;
        ap->hi[ITER_RSEED]          = MAXSHORT;
        break;
    case(ITERATE_EXTEND):
        switch(mode) {
        case(ITERATE_DUR):
            ap->lo[ITER_DUR]        = duration;
            ap->hi[ITER_DUR]        = BIG_TIME;
            break;
        case(ITERATE_REPEATS):
            ap->lo[ITER_REPEATS]    = 1.0;
            ap->hi[ITER_REPEATS]    = BIG_VALUE;
            break;
        default:
            sprintf(errstr,"Unknown mode for ITERATE_EXTEND: in set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }
        ap->lo[ITER_DELAY]          = FLTERR;
        ap->hi[ITER_DELAY]          = ITER_MAX_DELAY;
        ap->lo[ITER_RANDOM]         = 0.0;
        ap->hi[ITER_RANDOM]         = 1.0;
        ap->lo[ITER_PSCAT]          = 0.0;
        ap->hi[ITER_PSCAT]          = ITER_MAXPSHIFT;   
        ap->lo[ITER_ASCAT]          = 0.0;
        ap->hi[ITER_ASCAT]          = 1.0;
        ap->lo[CHUNKSTART]          = 0.0;
        ap->hi[CHUNKSTART]          = duration;
        ap->lo[CHUNKEND]            = 0.0;
        ap->hi[CHUNKEND]            = duration;
        ap->lo[ITER_LGAIN]           = 0.25;
        ap->hi[ITER_LGAIN]           = 4.0;
        ap->lo[ITER_RRSEED]          = 0.0;
        ap->hi[ITER_RRSEED]          = MAXSHORT;
        break;
    case(DRUNKWALK):
        ap->lo[DRNK_TOTALDUR]       = 0.0;
        ap->hi[DRNK_TOTALDUR]       = BIG_VALUE;
        ap->lo[DRNK_LOCUS]          = 0.0;
        ap->hi[DRNK_LOCUS]          = duration;
        ap->lo[DRNK_AMBITUS]        = 0.0;
        ap->hi[DRNK_AMBITUS]        = duration;
        ap->lo[DRNK_GSTEP]          = 0.0;
        ap->hi[DRNK_GSTEP]          = duration;
        ap->lo[DRNK_CLOKTIK]        = DRNK_SPLICE * MS_TO_SECS * 2.0;
        ap->hi[DRNK_CLOKTIK]        = duration; /* ???? */
        ap->lo[DRNK_MIN_DRNKTIK]    = 1.0;
        ap->hi[DRNK_MIN_DRNKTIK]    = BIG_VALUE;
        ap->lo[DRNK_MAX_DRNKTIK]    = 1.0;
        ap->hi[DRNK_MAX_DRNKTIK]    = BIG_VALUE;
        ap->lo[DRNK_SPLICELEN]      = DRNK_MIN_SPLICELEN;
        ap->hi[DRNK_SPLICELEN]      = min(DRNK_MAX_SPLICELEN,(duration/2.0) * SECS_TO_MS);
        ap->lo[DRNK_CLOKRND]        = 0.0;
        ap->hi[DRNK_CLOKRND]        = 1.0;
        ap->lo[DRNK_OVERLAP]        = 0.0;
        ap->hi[DRNK_OVERLAP]        = DRNK_MAX_OVERLAP;
        ap->lo[DRNK_RSEED]          = 0.0;
        ap->hi[DRNK_RSEED]          = MAXSHORT;
        if(mode==HAS_SOBER_MOMENTS) {
            ap->lo[DRNK_MIN_PAUS]   = FLTERR;
            ap->hi[DRNK_MIN_PAUS]   = duration + FLTERR;
            ap->lo[DRNK_MAX_PAUS]   = FLTERR;
            ap->hi[DRNK_MAX_PAUS]   = duration + FLTERR;
        }
        break;

    case(SIMPLE_TEX):   case(TIMED):    case(GROUPS):   case(TGROUPS):
    case(DECORATED):    case(PREDECOR): case(POSTDECOR):
    case(ORNATE):       case(PREORNATE):case(POSTORNATE):
    case(MOTIFS):       case(MOTIFSIN): case(TMOTIFS):  case(TMOTIFSIN):
        ap->lo[TEXTURE_DUR]         = TEXTURE_MIN_DUR;
        ap->hi[TEXTURE_DUR]         = BIG_TIME;
        switch(process) {
        case(SIMPLE_TEX):   case(GROUPS):   case(MOTIFS):   case(MOTIFSIN):
            ap->lo[TEXTURE_PACK]    = 1.0/sr;
            ap->hi[TEXTURE_PACK]    = MAX_PACKTIME;         
            break;
        case(DECORATED):    case(PREDECOR):     case(POSTDECOR):
        case(ORNATE):       case(PREORNATE):    case(POSTORNATE):
        case(TIMED):        case(TGROUPS):      case(TMOTIFS):      case(TMOTIFSIN):
            ap->lo[TEXTURE_SKIP]    = FLTERR;
            ap->hi[TEXTURE_SKIP]    = TEXTURE_MAX_SKIP;
            break;
        default:
            sprintf(errstr,"Unknown process in set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }
        ap->lo[TEXTURE_SCAT]        = 0.0;
        ap->hi[TEXTURE_SCAT]        = MAX_SCAT_TEXTURE;
        ap->lo[TEXTURE_TGRID]       = 0.0;
        ap->hi[TEXTURE_TGRID]       = TEXTURE_MAX_TGRID;
        ap->lo[TEXTURE_INSLO]       = 1.0;
        ap->hi[TEXTURE_INSLO]       = (double)SF_MAXFILES;
        ap->lo[TEXTURE_INSHI]       = 1.0;
        ap->hi[TEXTURE_INSHI]       = (double)SF_MAXFILES;
        ap->lo[TEXTURE_MAXAMP]      = MIDIBOT;
        ap->hi[TEXTURE_MAXAMP]      = MIDITOP; /* default 64 */
        ap->lo[TEXTURE_MINAMP]      = MIDIBOT;
        ap->hi[TEXTURE_MINAMP]      = MIDITOP; /* default 64 */
        ap->lo[TEXTURE_MAXDUR]      = (TEXTURE_SPLICELEN + TEXTURE_SAFETY) * MS_TO_SECS;
        ap->hi[TEXTURE_MAXDUR]      = BIG_TIME; 
        ap->lo[TEXTURE_MINDUR]      = (TEXTURE_SPLICELEN + TEXTURE_SAFETY) * MS_TO_SECS;
        ap->hi[TEXTURE_MINDUR]      = BIG_TIME; 
        ap->lo[TEXTURE_MAXPICH]     = MIDIBOT;
        ap->hi[TEXTURE_MAXPICH]     = MIDITOP;
        ap->lo[TEXTURE_MINPICH]     = MIDIBOT;
        ap->hi[TEXTURE_MINPICH]     = MIDITOP;
        if(process == SIMPLE_TEX) {
            ap->lo[TEX_PHGRID]          = 0;
            ap->hi[TEX_PHGRID]          = 64;
        } else {
            ap->lo[TEX_PHGRID]          = 0.0;
            ap->hi[TEX_PHGRID]          = TEXTURE_MAX_PHGRID;
        }
        ap->lo[TEX_GPSPACE]         = (double)IS_STILL;
        ap->hi[TEX_GPSPACE]         = (double)IS_CONTRARY;
        ap->lo[TEX_GRPSPRANGE]      = 0.0; /* default */
        ap->hi[TEX_GRPSPRANGE]      = 1.0;
        ap->lo[TEX_AMPRISE]         = 0.0;  /* default */
        ap->hi[TEX_AMPRISE]         = MIDITOP; 
        ap->lo[TEX_AMPCONT]         = 0.0; /* default */
        if(process==ORNATE      || process==PREORNATE   || process==POSTORNATE 
        || process==DECORATED   || process==PREDECOR    || process==POSTDECOR)
            ap->hi[TEX_AMPCONT]     = 8.0;
        else
            ap->hi[TEX_AMPCONT]     = 6.0;
        ap->lo[TEX_GPSIZELO]        = 1.0;
        ap->hi[TEX_GPSIZELO]        = BIG_VALUE;
        ap->lo[TEX_GPSIZEHI]        = 1.0;
        ap->hi[TEX_GPSIZEHI]        = BIG_VALUE;
        ap->lo[TEX_GPPACKLO]        = (double)SECS_TO_MS/sr;
        ap->hi[TEX_GPPACKLO]        = MAX_PACKTIME  * SECS_TO_MS;
        ap->lo[TEX_GPPACKHI]        = (double)SECS_TO_MS/sr;
        ap->hi[TEX_GPPACKHI]        = MAX_PACKTIME  * SECS_TO_MS;
        if(mode==TEX_NEUTRAL) { /* midipitches */
            ap->lo[TEX_GPRANGLO]    = MIDIBOT;  /* default 60 */
            ap->hi[TEX_GPRANGLO]    = MIDITOP;
            ap->lo[TEX_GPRANGHI]    = MIDIBOT;  /* default 60 */ 
            ap->hi[TEX_GPRANGHI]    = MIDITOP;
        } else { /* notes of hfield */
            ap->lo[TEX_GPRANGLO]    = 1.0;  /* default 8 */
            ap->hi[TEX_GPRANGLO]    = MAX_GPRANGE;
            ap->lo[TEX_GPRANGHI]    = 1.0;  /* default 8 */ 
            ap->hi[TEX_GPRANGHI]    = MAX_GPRANGE;
        }
        ap->lo[TEX_MULTLO]          = MIN_MULT; /* default 1 */
        ap->hi[TEX_MULTLO]          = MAX_MULT;
        ap->lo[TEX_MULTHI]          = MIN_MULT; /* default 1 */
        ap->hi[TEX_MULTHI]          = MAX_MULT;
        ap->lo[TEX_DECPCENTRE]      = (double)DEC_CENTRED; /* default 0 */
        ap->hi[TEX_DECPCENTRE]      = (double)DEC_C_A_B;
        ap->lo[TEXTURE_ATTEN]       = FLTERR;
        ap->hi[TEXTURE_ATTEN]       = 1.0;
        ap->lo[TEXTURE_POS]         = TEX_LEFT;
        ap->hi[TEXTURE_POS]         = TEX_RIGHT;
        ap->lo[TEXTURE_SPRD]        = 0.0;
        ap->hi[TEXTURE_SPRD]        = 1.0;
        ap->lo[TEXTURE_SEED]        = 0.0;
        ap->hi[TEXTURE_SEED]        = MAXSHORT;

        for(n=0;n<TEXTURE_SEED;n++) {
            if(n == TEXTURE_SCAT && process == SIMPLE_TEX)
                continue;
            if(ap->lo[n] <0.0 || ap->hi[n] <0.0) {
                sprintf(errstr,"Range limit of parameter %d has -ve value: invalid in TEXTURE\n",n+1);
                return(PROGRAM_ERROR);
            }
        }
        break;

    case(GRAIN_COUNT):      case(GRAIN_OMIT):       case(GRAIN_DUPLICATE):
    case(GRAIN_REORDER):    case(GRAIN_REPITCH):    case(GRAIN_RERHYTHM):
    case(GRAIN_REMOTIF):    case(GRAIN_TIMEWARP):   case(GRAIN_POSITION):
    case(GRAIN_ALIGN):      case(GRAIN_GET):        case(GRAIN_REVERSE):
        ap->lo[GR_BLEN]             = min(duration,GR_MINDUR);
        ap->hi[GR_BLEN]             = duration;
        ap->lo[GR_GATE]             = 0.0;
        ap->hi[GR_GATE]             = 1.0;
        /* min GR_MINTIME size guarantees grainsize cannot be less than 2 * splicelen */
        /*  internal algorithms depend on this fact !! */
        ap->lo[GR_MINTIME]          = (GRAIN_SPLICELEN + GRAIN_SAFETY) * MS_TO_SECS * 2.0;
        ap->hi[GR_MINTIME]          = duration;
        ap->lo[GR_WINSIZE]          = 0.0;
        ap->hi[GR_WINSIZE]          = duration * SECS_TO_MS;
        switch(process) {
        case(GRAIN_OMIT):       
            ap->lo[GR_KEEP]         = 1.0;
            ap->hi[GR_KEEP]         = GR_MAX_OUT_OF - 1.0;
            ap->lo[GR_OUT_OF]       = 2.0;
            ap->hi[GR_OUT_OF]       = GR_MAX_OUT_OF;
            break;
        case(GRAIN_DUPLICATE):  
            ap->lo[GR_DUPLS]        = 1.0;
            ap->hi[GR_DUPLS]        = BIG_VALUE;
            break;
        case(GRAIN_TIMEWARP):
            ap->lo[GR_TSTRETCH]     = GR_MIN_TSTRETCH;
            ap->hi[GR_TSTRETCH]     = GR_MAX_TSTRETCH;
            break;
        case(GRAIN_POSITION):
            ap->lo[GR_OFFSET]       = 0.0;
            ap->hi[GR_OFFSET]       = GR_MAX_OFFSET;
            break;
        case(GRAIN_ALIGN):
            ap->lo[GR_OFFSET]       = 0.0;
            ap->hi[GR_OFFSET]       = GR_MAX_OFFSET;
            ap->lo[GR_GATE2]        = 0.0;
            ap->hi[GR_GATE2]        = 1.0;
            break;
        }
        break;
    case(RRRR_EXTEND):
        if(mode == 1) {
            ap->lo[RRR_GATE]   = 0.0;
            ap->hi[RRR_GATE]   = 1.0;
            ap->lo[RRR_GRSIZ]  = LOW_RRR_SIZE;
            ap->hi[RRR_GRSIZ]  = SECS_TO_MS;
            ap->lo[RRR_SKIP]   = 0;
            ap->hi[RRR_SKIP]   = 8;
            ap->lo[RRR_AFTER]   = 0.0;
            ap->hi[RRR_AFTER]   = duration;
            ap->lo[RRR_TEMPO]   = LOW_RRR_SIZE * MS_TO_SECS;
            ap->hi[RRR_TEMPO]   = A_MINUTE;
            ap->lo[RRR_AT]      = 0;
            ap->hi[RRR_AT]      = AN_HOUR;
        } else {
            ap->lo[RRR_START]   = 0.0;
            ap->hi[RRR_START]   = duration;
            ap->lo[RRR_END]     = 0.0;
            ap->hi[RRR_END]     = duration;
        }
        ap->lo[RRR_SLOW]    = 1.0;
        ap->hi[RRR_SLOW]    = 100;
        ap->lo[RRR_REGU]    = 0.0;
        ap->hi[RRR_REGU]    = 1.0;
        ap->lo[RRR_RANGE]   = 0.0;
        ap->hi[RRR_RANGE]   = 4.0;
        ap->lo[RRR_GET]     = 2.0;
        ap->hi[RRR_GET]     = 100.0;
        if (mode != 2) {
            ap->lo[RRR_STRETCH] = 1.0;
            ap->hi[RRR_STRETCH] = 32767.0;
            ap->lo[RRR_REPET]   = 1.0;
            ap->hi[RRR_REPET]   = 32767.0;
            ap->lo[RRR_ASCAT]   = 0.0;
            ap->hi[RRR_ASCAT]   = 1.0;
            ap->lo[RRR_PSCAT]   = 0.0;
            ap->hi[RRR_PSCAT]   = 24.0;
        }
        break;
    case(SSSS_EXTEND):
        ap->lo[SSS_DUR] = .01;
        ap->hi[SSS_DUR] = 32767.0;
        ap->lo[NOISE_MINFRQ] = MIN_SUPRESS;
        ap->hi[NOISE_MINFRQ] = nyquist;
        ap->lo[MIN_NOISLEN] = 0.0;
        ap->hi[MIN_NOISLEN] = 50.0;
        ap->lo[MAX_NOISLEN] = 0.0;
        ap->hi[MAX_NOISLEN] = duration;
        ap->lo[SSS_GATE]    = 0.0;
        ap->hi[SSS_GATE]    = 1.0;
        break;
    case(ENV_CREATE):
        ap->lo[ENV_WSIZE]           = ENV_MIN_WSIZE;                /* MSECS */
        ap->hi[ENV_WSIZE]           = ENV_MAX_WSIZE;
        break;
    case(ENV_BRKTOENV):
    case(ENV_DBBRKTOENV):
    case(ENV_IMPOSE):
//TW NEW CASE
    case(ENV_PROPOR):
    case(ENV_REPLACE):
        ap->lo[ENV_WSIZE]           = ENV_MIN_WSIZE;                /* MSECS */
        ap->hi[ENV_WSIZE]           = duration * SECS_TO_MS;
        break;
    case(ENV_EXTRACT):
        ap->lo[ENV_WSIZE]           = ENV_MIN_WSIZE;                /* MSECS */
        ap->hi[ENV_WSIZE]           = duration * SECS_TO_MS;
        if(mode==ENV_BRKFILE_OUT) {
            ap->lo[ENV_DATAREDUCE]  = 0.0;
            ap->hi[ENV_DATAREDUCE]  = 1.0;
        }
        break;
    case(ENV_ENVTOBRK):
    case(ENV_ENVTODBBRK):
        ap->lo[ENV_DATAREDUCE]      = 0.0;
        ap->hi[ENV_DATAREDUCE]      = 1.0;
        break;
    case(ENV_WARPING):
    case(ENV_REPLOTTING):
        ap->lo[ENV_WSIZE]           = ENV_MIN_WSIZE;                /* MSECS */
        ap->hi[ENV_WSIZE]           = duration * SECS_TO_MS;
        /* fall thro */
    case(ENV_RESHAPING):
        switch(mode) {
        case(ENV_NORMALISE):
        case(ENV_REVERSE):
        case(ENV_CEILING):
            break;
        case(ENV_DUCKED):
            ap->lo[ENV_GATE]        = 0.0;
            ap->hi[ENV_GATE]        = 1.0;
            ap->lo[ENV_THRESHOLD]   = 0.0;
            ap->hi[ENV_THRESHOLD]   = 1.0;
            break;
        case(ENV_EXAGGERATING):
            ap->lo[ENV_EXAG]        = MIN_ENV_EXAG;
            ap->hi[ENV_EXAG]        = MAX_ENV_EXAG;
            break;
        case(ENV_ATTENUATING):
            ap->lo[ENV_ATTEN]       = MIN_ENV_ATTEN;
            ap->hi[ENV_ATTEN]       = MAX_ENV_ATTEN;
            break;
        case(ENV_LIFTING):
            ap->lo[ENV_LIFT]        = 0.0;
            ap->hi[ENV_LIFT]        = 1.0;
            break;
        case(ENV_FLATTENING):
            ap->lo[ENV_FLATN]       = 2.0;
            ap->hi[ENV_FLATN]       = MAX_ENV_FLATN;
            break;
        case(ENV_TSTRETCHING):
            ap->lo[ENV_TSTRETCH]    = MIN_ENV_TSTRETCH;
            ap->hi[ENV_TSTRETCH]    = MAX_ENV_TSTRETCH;
            break;
        case(ENV_GATING):
            ap->lo[ENV_GATE]        = 0.0;
            ap->hi[ENV_GATE]        = 1.0;
            ap->lo[ENV_SMOOTH]      = 0.0;
            ap->hi[ENV_SMOOTH]      = MAX_ENV_SMOOTH;
            break;
        case(ENV_INVERTING):
            ap->lo[ENV_GATE]        = 0.0;
            ap->hi[ENV_GATE]        = 1.0;
            ap->lo[ENV_MIRROR]      = 0.0;
            ap->hi[ENV_MIRROR]      = 1.0;
            break;
        case(ENV_LIMITING):
            ap->lo[ENV_LIMIT]       = 0.0;
            ap->hi[ENV_LIMIT]       = 1.0;
            ap->lo[ENV_THRESHOLD]   = 0.0;
            ap->hi[ENV_THRESHOLD]   = 1.0;
            break;
        case(ENV_CORRUGATING):
            ap->lo[ENV_TROFDEL]     = 1.0;
            ap->hi[ENV_TROFDEL]     = BIG_VALUE;
            /* fall thro */
        case(ENV_PEAKCNT):
            ap->lo[ENV_PKSRCHWIDTH] = 2.0;
            ap->hi[ENV_PKSRCHWIDTH] = BIG_VALUE;
            break;
        case(ENV_EXPANDING):
            ap->lo[ENV_GATE]        = 0.0;
            ap->hi[ENV_GATE]        = 1.0;
            ap->lo[ENV_THRESHOLD]   = 0.0;
            ap->hi[ENV_THRESHOLD]   = 1.0;
            ap->lo[ENV_SMOOTH]      = 0.0;
            ap->hi[ENV_SMOOTH]      = MAX_ENV_SMOOTH;
            break;
        case(ENV_TRIGGERING):
            ap->lo[ENV_GATE]        = 0.0;
            ap->hi[ENV_GATE]        = 1.0;
            ap->lo[ENV_TRIGRISE]    = 1.0/(double)MAXSHORT;
            ap->hi[ENV_TRIGRISE]    = 1.0;
            ap->lo[ENV_TRIGDUR]     = ENV_MIN_WSIZE;
            ap->hi[ENV_TRIGDUR]     = duration * SECS_TO_MS;
            break;
        default:
            sprintf(errstr,"Unknown case for ENVWARP,RESHAPING or REPLOTTING: set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }                 
        switch(process) {
        case(ENV_WARPING):
            ap->lo[ENV_WSIZE]       = ENV_MIN_WSIZE;                /* MSECS */
            ap->hi[ENV_WSIZE]       = duration * SECS_TO_MS;
            break;
        case(ENV_REPLOTTING):
            ap->lo[ENV_WSIZE]       = ENV_MIN_WSIZE;                /* MSECS */
            ap->hi[ENV_WSIZE]       = duration * SECS_TO_MS;
            ap->lo[ENV_DATAREDUCE]  = 0.0;
            ap->hi[ENV_DATAREDUCE]  = 1.0;
        }
        break;
    case(ENV_DOVETAILING):
        ap->lo[ENV_STARTTRIM]       = 0.0;
        ap->hi[ENV_STARTTRIM]       = duration;
        ap->lo[ENV_ENDTRIM]         = 0.0;
        ap->hi[ENV_ENDTRIM]         = duration;
        ap->lo[ENV_STARTTYPE]       = (double)ENVTYPE_LIN;
        ap->hi[ENV_STARTTYPE]       = (double)ENVTYPE_EXP;
        ap->lo[ENV_ENDTYPE]         = (double)ENVTYPE_LIN;
        ap->hi[ENV_ENDTYPE]         = (double)ENVTYPE_EXP;
        ap->lo[ENV_TIMETYPE]        = (double)ENV_TIMETYPE_SECS;
        ap->hi[ENV_TIMETYPE]        = (double)ENV_TIMETYPE_STSMPS;
        break;
    case(ENV_CURTAILING):
        ap->lo[ENV_STARTTIME]       = 0.0;
        ap->hi[ENV_STARTTIME]       = duration;
        ap->lo[ENV_ENDTIME]         = 0.0;
        ap->hi[ENV_ENDTIME]         = duration;
        ap->lo[ENV_ENVTYPE]         = (double)ENVTYPE_LIN;
        ap->hi[ENV_ENVTYPE]         = (double)ENVTYPE_EXP;
        ap->lo[ENV_TIMETYPE]        = (double)ENV_TIMETYPE_SECS;
        ap->hi[ENV_TIMETYPE]        = (double)ENV_TIMETYPE_STSMPS;
        break;
    case(ENV_SWELL):
        ap->lo[ENV_PEAKTIME]        = ENV_MIN_WSIZE * MS_TO_SECS;
        ap->hi[ENV_PEAKTIME]        = duration - (ENV_MIN_WSIZE * MS_TO_SECS);
        ap->lo[ENV_ENVTYPE]         = (double)ENVTYPE_LIN;
        ap->hi[ENV_ENVTYPE]         = (double)ENVTYPE_EXP;
        break;
    case(ENV_ATTACK):
        switch(mode) {
        case(ENV_ATK_GATED):
            ap->lo[ENV_ATK_GATE]    = 0.0;
            ap->hi[ENV_ATK_GATE]    = 1.0;
            break;
        case(ENV_ATK_TIMED):
        case(ENV_ATK_XTIME):
            ap->lo[ENV_ATK_ATTIME]  = 0.0;
            ap->hi[ENV_ATK_ATTIME]  = duration - (ENV_MIN_ATK_ONSET * MS_TO_SECS);
            break;
        case(ENV_ATK_ATMAX):
            break;
        }
        ap->lo[ENV_ATK_GAIN]        = 1.0;
        ap->hi[ENV_ATK_GAIN]        = (double)MAXSHORT;
        ap->lo[ENV_ATK_ONSET]       = ENV_MIN_ATK_ONSET;
        ap->hi[ENV_ATK_ONSET]       = (double)BIG_TIME;
        ap->lo[ENV_ATK_TAIL]        = ENV_MIN_ATK_ONSET;
        ap->hi[ENV_ATK_TAIL]        = (duration * SECS_TO_MS) - ENV_MIN_ATK_ONSET;
        ap->lo[ENV_ATK_ENVTYPE]     = ENVTYPE_LIN;
        ap->hi[ENV_ATK_ENVTYPE]     = ENVTYPE_EXP;
        break;
    case(ENV_PLUCK):
        ap->lo[ENV_PLK_ENDSAMP]     = 0.0;
        ap->hi[ENV_PLK_ENDSAMP]     = round(min(ENV_PLK_ENDSAMP_MAXTIME,duration)) * srate * channels;
        ap->lo[ENV_PLK_WAVELEN]     = round(sr/ENV_PLK_FRQ_MAX);
        ap->hi[ENV_PLK_WAVELEN]     = round(sr/ENV_PLK_FRQ_MIN);
        ap->lo[ENV_PLK_CYCLEN]      = ENV_PLK_CYCLEN_MIN;
        ap->hi[ENV_PLK_CYCLEN]      = ENV_PLK_CYCLEN_MAX;
        ap->lo[ENV_PLK_DECAY]       = ENV_PLK_DECAY_MIN;
        ap->hi[ENV_PLK_DECAY]       = ENV_PLK_DECAY_MAX;
        break;
    case(ENV_TREMOL):
        ap->lo[ENV_TREM_FRQ]        = 0.0;
        ap->hi[ENV_TREM_FRQ]        = ENV_TREM_MAXFRQ;
        ap->lo[ENV_TREM_DEPTH]      = 0.0;
        ap->hi[ENV_TREM_DEPTH]      = 1.0;
        ap->lo[ENV_TREM_AMP]        = 0.0;
        ap->hi[ENV_TREM_AMP]        = 1.0;
        break;
    case(ENV_DBBRKTOBRK):
    case(ENV_BRKTODBBRK):
        break;

    case(MIX):
        ap->lo[MIX_ATTEN]           = 0.0;
        ap->hi[MIX_ATTEN]           = 1.0;
        /* fall thro */
    case(MIXMAX):
        ap->lo[MIX_START]           = 0.0;
        ap->hi[MIX_START]           = duration;
        ap->lo[MIX_END]             = 0.0;
        ap->hi[MIX_END]             = duration;
        break;
    case(MIXCROSS):
        ap->lo[MCR_STAGGER]         = 0.0;
        ap->hi[MCR_STAGGER]         = duration;
        ap->lo[MCR_BEGIN]           = 0.0;
        ap->hi[MCR_BEGIN]           = duration;
        ap->lo[MCR_END]             = 0.0;
        ap->hi[MCR_END]             = BIG_TIME;
        if(mode==MCCOS) {
            ap->lo[MCR_POWFAC]      = MIN_MCR_POWFAC;
            ap->hi[MCR_POWFAC]      = MAX_MCR_POWFAC;
        }
        break;
    case(MIXTWO):
        ap->lo[MIX_STAGGER]         = 0.0;
        ap->hi[MIX_STAGGER]         = duration;
        ap->lo[MIX_SKIP]            = 0.0;
        ap->hi[MIX_SKIP]            = BIG_TIME;
        ap->lo[MIX_SKEW]            = 1.0/(double)MAXSHORT;
        ap->hi[MIX_SKEW]            = (double)MAXSHORT;
        ap->lo[MIX_DURA]            = 0.0;
        ap->hi[MIX_DURA]            = 32767.0;
        ap->lo[MIX_STTA]            = 0.0;
        ap->hi[MIX_STTA]            = 32767.0;
        break;
//TW NEW CASE
    case(MIXMANY):
        break;
    case(MIXBALANCE):
        ap->lo[MIX_STAGGER]         = 0.0;
        ap->hi[MIX_STAGGER]         = 1.0;
        ap->lo[MIX_SKIP]            = 0.0;
        ap->hi[MIX_SKIP]            = duration;
        ap->lo[MIX_SKEW]            = 0.0;
        ap->hi[MIX_SKEW]            = duration;
        break;
    case(MIXINBETWEEN):
        switch(mode) {
        case(INBI_COUNT):
            ap->lo[INBETW]          = 1.0;
            ap->hi[INBETW]          = (double)MAXBETWEEN;
            break;
        case(INBI_RATIO):   /* TW param read as special data */
            break;
        default:
            sprintf(errstr,"Unknown mode for MIXINBETWEEN: set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(CYCINBETWEEN):
        ap->lo[INBETW]          = 1.0;
        ap->hi[INBETW]          = (double)MAXBETWEEN;
        ap->lo[BTWN_HFRQ]      = (double)10.0;
        ap->hi[BTWN_HFRQ]      = (double)nyquist;
        break;
    case(MIXTEST):
    case(MIXFORMAT):
    case(MIXDUMMY):
    case(MIXINTERL):
    case(MIXSYNC):
//TW NEW CASE
    case(ADDTOMIX):
        break;
    case(MIXSYNCATT):
        ap->lo[MSY_WFAC]            = (double)MIN_WINFAC;
        ap->hi[MSY_WFAC]            = (double)MAX_WINFAC;
        break;
    case(MIXGAIN):         
        ap->lo[MIX_GAIN]            = 0.0;
        ap->hi[MIX_GAIN]            = (double)MAXSHORT;
        ap->lo[MSH_ENDLINE]         = 1.0;
        ap->hi[MSH_ENDLINE]         = (double)linecnt;
        ap->lo[MSH_STARTLINE]       = 1.0;
        ap->hi[MSH_STARTLINE]       = (double)linecnt;
        break;
    case(MIXTWARP):
        switch(mode) {
        case(MTW_TIMESORT):
        case(MTW_REVERSE_T):
        case(MTW_REVERSE_NT):
        case(MTW_FREEZE_T):
        case(MTW_FREEZE_NT):
            break;
        case(MTW_SCATTER):
            ap->lo[MTW_PARAM]       = 0.0;
            ap->hi[MTW_PARAM]       = 1.0;
            break;
        case(MTW_DOMINO):
            ap->lo[MTW_PARAM]       = -BIG_TIME;
            ap->hi[MTW_PARAM]       = BIG_TIME;
            break;
        case(MTW_ADD_TO_TG):
            ap->lo[MTW_PARAM]       = -BIG_TIME;
            ap->hi[MTW_PARAM]       = BIG_TIME;
            break;
        case(MTW_CREATE_TG_1):  case(MTW_CREATE_TG_2): case(MTW_CREATE_TG_3):   case(MTW_CREATE_TG_4):
        case(MTW_ENLARGE_TG_1): case(MTW_ENLARGE_TG_2): case(MTW_ENLARGE_TG_3): case(MTW_ENLARGE_TG_4):
            ap->lo[MTW_PARAM]       = 0.0;
            ap->hi[MTW_PARAM]       = BIG_TIME;
            break;
        default:
            sprintf(errstr,"Unknown mode for MIXTWARP: in set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }
        if(mode != MTW_TIMESORT) {
            ap->lo[MSH_ENDLINE]     = 1.0;
            ap->hi[MSH_ENDLINE]     = (double)linecnt;
            ap->lo[MSH_STARTLINE]   = 1.0;
            ap->hi[MSH_STARTLINE]   = (double)linecnt;
        }
        break;
    case(MIXSWARP):
        switch(mode) {
        case(MSW_TWISTALL):
            break;
        case(MSW_TWISTONE):
            ap->lo[MSW_TWLINE]      = 1.0;
            ap->hi[MSW_TWLINE]      = (double)linecnt;
            break;
        case(MSW_NARROWED):
            ap->lo[MSW_NARROWING]   = 0.0;
            ap->hi[MSW_NARROWING]   = 1.0;
            break;
        case(MSW_LEFTWARDS): 
        case(MSW_RIGHTWARDS): 
        case(MSW_RANDOM): 
        case(MSW_RANDOM_ALT):
            ap->lo[MSW_POS2]        = PAN_LEFT;
            ap->hi[MSW_POS2]        = PAN_RIGHT;
            /* fall thro */
        case(MSW_FIXED):
            ap->lo[MSW_POS1]        = PAN_LEFT;
            ap->hi[MSW_POS1]        = PAN_RIGHT;
            break;
        default:
            sprintf(errstr,"Unknown mode for MIXSWARP: in set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }
        if(mode!=MSW_TWISTALL && mode!=MSW_TWISTONE) {
            ap->lo[MSH_ENDLINE]     = 1.0;
            ap->hi[MSH_ENDLINE]     = (double)linecnt;
            ap->lo[MSH_STARTLINE]   = 1.0;
            ap->hi[MSH_STARTLINE]   = (double)linecnt;
        }
        break;
    case(MIXSHUFL):        
        ap->lo[MSH_ENDLINE]         = 1.0;
        ap->hi[MSH_ENDLINE]         = (double)linecnt;
        ap->lo[MSH_STARTLINE]       = 1.0;
        ap->hi[MSH_STARTLINE]       = (double)linecnt;
        break;
//TW NEW CASES
    case(MIX_ON_GRID):
        break;
    case(AUTOMIX):
        ap->lo[0]           = 0.0;
        ap->hi[0]           = 256.0;
        break;
    case(MIX_PAN):
        ap->lo[PAN_PAN]     = -MAX_PANNING;
        ap->hi[PAN_PAN]     = MAX_PANNING;
        break;
    case(MIX_AT_STEP):
        ap->lo[MIX_STEP]    = 0.0;
        ap->hi[MIX_STEP]    = 10000.0;
        break;
    case(SHUDDER):
        ap->lo[SHUD_STARTTIME]      = 0.0;
        ap->hi[SHUD_STARTTIME]      = duration;
        ap->lo[SHUD_FRQ]            = 0.1;
        ap->hi[SHUD_FRQ]            = 100.0;
        ap->lo[SHUD_SCAT]           = 0.0;
        ap->hi[SHUD_SCAT]           = 1.0;
        ap->lo[SHUD_SPREAD]         = 0.0;
        ap->hi[SHUD_SPREAD]         = 1.0;
        ap->lo[SHUD_MINDEPTH]       = 0.0;
        ap->hi[SHUD_MINDEPTH]       = 1.0;
        ap->lo[SHUD_MAXDEPTH]       = 0.0;
        ap->hi[SHUD_MAXDEPTH]       = 1.0;
        ap->lo[SHUD_MINWIDTH]       = 0.02;
        ap->hi[SHUD_MINWIDTH]       = duration;
        ap->lo[SHUD_MAXWIDTH]       = 0.02;
        ap->hi[SHUD_MAXWIDTH]       = duration;
        break;

    case(EQ):
        switch(mode) {
        case(FLT_PEAKING):
            ap->lo[FLT_BW]          = 0.001;
            ap->hi[FLT_BW]          = nyquist;
            /* fall thro */
        default:
            ap->lo[FLT_BOOST]       = FLT_MINDBGAIN;
            ap->hi[FLT_BOOST]       = FLT_MAXDBGAIN;
            ap->lo[FLT_ONEFRQ]      = FLT_MINFRQ;
            ap->hi[FLT_ONEFRQ]      = FLT_MAXFRQ;
            ap->lo[FLT_PRESCALE]    = FLT_MINEQPRESCALE;
            ap->hi[FLT_PRESCALE]    = FLT_MAXEQPRESCALE;
//TW NEW PARAM
            ap->lo[FILT_TAIL]       = 0.0;
            ap->hi[FILT_TAIL]       = FLT_TAIL;
            break;
        }
        break;
    case(LPHP):
        ap->lo[FLT_GAIN]            = MIN_DB_ON_16_BIT;
        ap->hi[FLT_GAIN]            = 0.0;
        ap->lo[FLT_PRESCALE]        = FLT_MINEQPRESCALE;
        ap->hi[FLT_PRESCALE]        = FLT_MAXEQPRESCALE;
//TW NEW PARAM
        ap->lo[FILT_TAIL]           = 0.0;
        ap->hi[FILT_TAIL]           = FLT_TAIL;
        switch(mode) {
        case(FLT_HZ):
            ap->lo[FLT_PASSFRQ]     = FLT_MINFRQ;
            ap->hi[FLT_PASSFRQ]     = FLT_MAXFRQ;
            ap->lo[FLT_STOPFRQ]     = FLT_MINFRQ;
            ap->hi[FLT_STOPFRQ]     = FLT_MAXFRQ;
            break;
        case(FLT_MIDI):
            ap->lo[FLT_PASSFRQ]     = unchecked_hztomidi(FLT_MINFRQ);
            ap->hi[FLT_PASSFRQ]     = MIDIMAX;
            ap->lo[FLT_STOPFRQ]     = unchecked_hztomidi(FLT_MINFRQ);
            ap->hi[FLT_STOPFRQ]     = MIDIMAX;
            break;
        default:
            sprintf(errstr,"Unknown case for LPHP: set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(FSTATVAR):
        ap->lo[FLT_Q]               = MIN_ACUITY;   /* parametere is acutrally 1/Q */
        ap->hi[FLT_Q]               = MAX_ACUITY;
        ap->lo[FLT_GAIN]            = FLT_MINGAIN;
        ap->hi[FLT_GAIN]            = FLT_MAXGAIN;
        ap->lo[FLT_ONEFRQ]          = FLT_MINFRQ;
        ap->hi[FLT_ONEFRQ]          = FLT_MAXFRQ;
//TW NEW PARAM
        ap->lo[FILT_TAIL]           = 0.0;
        ap->hi[FILT_TAIL]           = FLT_TAIL;
        break;
    case(FLTBANKN):     
        ap->lo[FLT_Q]               = MINQ;
        ap->hi[FLT_Q]               = MAXQ;
        ap->lo[FLT_GAIN]            = FLT_MINGAIN;
        ap->hi[FLT_GAIN]            = FLT_MAXGAIN;
//TW NEW PARAM
        ap->lo[FILT_TAIL]           = 0.0;
        ap->hi[FILT_TAIL]           = FLT_TAIL;
        /* fall thro */
    case(FLTBANKC):
        ap->lo[FLT_LOFRQ]           = FLT_MINFRQ;
        ap->hi[FLT_LOFRQ]           = FLT_MAXFRQ;
        ap->lo[FLT_HIFRQ]           = FLT_MINFRQ;
        ap->hi[FLT_HIFRQ]           = FLT_MAXFRQ;
        switch(mode) {
        case(FLT_LINOFFSET):
            ap->lo[FLT_OFFSET]      = -FLT_MAXFRQ;
            ap->hi[FLT_OFFSET]      = FLT_MAXFRQ;
            break;
        case(FLT_EQUALSPAN):
            ap->lo[FLT_INCOUNT]     = 1.0;
            ap->hi[FLT_INCOUNT]     = (double)FLT_MAX_FILTERS;
            break;
        case(FLT_EQUALINT):
            ap->lo[FLT_INTSIZE]     = FLT_MININT;
            ap->hi[FLT_INTSIZE]     = FLT_MAXINT;
            break;
        }
        ap->lo[FLT_RANDFACT]        = 0.0;
        ap->hi[FLT_RANDFACT]        = 1.0;
        break;
    case(FLTBANKU):     
        ap->lo[FLT_Q]               = MINQ;
        ap->hi[FLT_Q]               = MAXQ;
        ap->lo[FLT_GAIN]            = FLT_MINGAIN;
        ap->hi[FLT_GAIN]            = FLT_MAXGAIN;
        ap->lo[FILT_TAIL]           = 0.0;
        ap->hi[FILT_TAIL]           = FLT_TAIL;
        break;
    case(FLTBANKV):
        ap->lo[FLT_Q]               = MINQ;
        ap->hi[FLT_Q]               = MAXQ;
        ap->lo[FLT_GAIN]            = FLT_MINGAIN;
        ap->hi[FLT_GAIN]            = FLT_MAXGAIN;
        ap->lo[FLT_HARMCNT]         = 1.0;
        ap->hi[FLT_HARMCNT]         = FLT_MAXHARMS;
        ap->lo[FLT_ROLLOFF]         = MIN_DB_ON_16_BIT;
        ap->hi[FLT_ROLLOFF]         = 0.0;
        ap->lo[FILT_TAILV]          = 0.0;
        ap->hi[FILT_TAILV]          = FLT_TAIL;
        break;
    case(FLTBANKV2):
        ap->lo[FLT_Q]               = MINQ;
        ap->hi[FLT_Q]               = MAXQ;
        ap->lo[FLT_GAIN]            = FLT_MINGAIN;
        ap->hi[FLT_GAIN]            = FLT_MAXGAIN;
        ap->lo[FILT_TAILV]          = 0.0;
        ap->hi[FILT_TAILV]          = FLT_TAIL;
        break;
    case(FLTSWEEP):
        ap->lo[FLT_Q]               = MIN_ACUITY;
        ap->hi[FLT_Q]               = MAX_ACUITY;
        ap->lo[FLT_GAIN]            = FLT_MINGAIN;
        ap->hi[FLT_GAIN]            = FLT_MAXGAIN;
        ap->lo[FLT_LOFRQ]           = FLT_MINFRQ;
        ap->hi[FLT_LOFRQ]           = FLT_MAXFRQ;
        ap->lo[FLT_HIFRQ]           = FLT_MINFRQ;
        ap->hi[FLT_HIFRQ]           = FLT_MAXFRQ;
        ap->lo[FLT_SWPFRQ]          = 0.0;
        ap->hi[FLT_SWPFRQ]          = FLT_MAXSWEEP;
        ap->lo[FLT_SWPPHASE]        = 0.0;
        ap->hi[FLT_SWPPHASE]        = 1.0;
//TW NEW PARAM
        ap->lo[FILT_TAIL]           = 0.0;
        ap->hi[FILT_TAIL]           = FLT_TAIL;
        break;
    case(FLTITER):
        ap->lo[FLT_Q]               = MINQ;
        ap->hi[FLT_Q]               = MAXQ;
        ap->lo[FLT_GAIN]            = FLT_MINGAIN;
        ap->hi[FLT_GAIN]            = FLT_MAXGAIN;
        ap->lo[FLT_DELAY]           = FLTERR;
        ap->hi[FLT_DELAY]           = FLT_MAXDELAY;
        ap->lo[FLT_OUTDUR]          = duration;
        ap->hi[FLT_OUTDUR]          = BIG_TIME;
        ap->lo[FLT_PRESCALE]        = FLT_MINPRESCALE;
        ap->hi[FLT_PRESCALE]        = FLT_MAXPRESCALE;
        ap->lo[FLT_RANDDEL]         = 0.0;
        ap->hi[FLT_RANDDEL]         = 1.0;
        ap->lo[FLT_PSHIFT]          = 0.0;
        ap->hi[FLT_PSHIFT]          = FLT_MAX_PSHIFT;   /* semitones */
        ap->lo[FLT_AMPSHIFT]        = 0.0;
        ap->hi[FLT_AMPSHIFT]        = 1.0;
        break;
    case(ALLPASS):      
        ap->lo[FLT_GAIN]            = -1.0;
        ap->hi[FLT_GAIN]            = 1.0;
        ap->lo[FLT_DELAY]           = (1.0/sr) * SECS_TO_MS;
        ap->hi[FLT_DELAY]           = (duration/2.0) * SECS_TO_MS;
        ap->lo[FLT_PRESCALE]        = FLT_MINPRESCALE;
        ap->hi[FLT_PRESCALE]        = FLT_MAXPRESCALE;
//TW NEW PARAM
        ap->lo[FILT_TAIL]           = 0.0;
        ap->hi[FILT_TAIL]           = FLT_TAIL;
        break;

    case(MOD_LOUDNESS):
        switch(mode) {
        case(LOUDNESS_GAIN):
            ap->lo[LOUD_GAIN]       = 0.0;
            ap->hi[LOUD_GAIN]       = (double)MAXSHORT;
            break;
        case(LOUDNESS_DBGAIN):
            ap->lo[LOUD_GAIN]       = MIN_DB_ON_16_BIT;
            ap->hi[LOUD_GAIN]       = /*0.0*/MAX_DB_ON_16_BIT;
            break;
        case(LOUDNESS_NORM):
        case(LOUDNESS_SET):
            ap->lo[LOUD_LEVEL]      = 1.0/(double)MAXSHORT;
            ap->hi[LOUD_LEVEL]      = 1.0;
            break;
        }
        break;

    case(MOD_SPACE):
        switch(mode) {
        case(MOD_PAN):
            ap->lo[PAN_PAN]         = -MAX_PANNING;
            ap->hi[PAN_PAN]         = MAX_PANNING;
            ap->lo[PAN_PRESCALE]    = 1.0/(double)MAXSHORT;
            ap->hi[PAN_PRESCALE]    = 2.0;
            break;
        case(MOD_NARROW):
            ap->lo[NARROW]          = -1.0;
            ap->hi[NARROW]          = 1.0;
            break;
        }
        break;
//TW NEW CASES
        case(SCALED_PAN):
        ap->lo[PAN_PAN]         = -MAX_PANNING;
        ap->hi[PAN_PAN]         = MAX_PANNING;
        ap->lo[PAN_PRESCALE]    = 1.0/(double)MAXSHORT;
        ap->hi[PAN_PRESCALE]    = 2.0;
        break;
    case(FIND_PANPOS):
        ap->lo[PAN_PAN]         = 0.0;
        ap->hi[PAN_PAN]         = duration;
        break;

    case(MOD_PITCH):
        switch(mode) {
        case(MOD_TRANSPOS):
        case(MOD_TRANSPOS_INFO):
            ap->lo[VTRANS_TRANS]    = MIN_TRANSPOS;
            ap->hi[VTRANS_TRANS]    = MAX_TRANSPOS;
            break;
        case(MOD_TRANSPOS_SEMIT):
        case(MOD_TRANSPOS_SEMIT_INFO):
            ap->lo[VTRANS_TRANS]    = EIGHT_8VA_DOWN;
            ap->hi[VTRANS_TRANS]    = EIGHT_8VA_UP;
            break;
        case(MOD_ACCEL):
            ap->lo[ACCEL_ACCEL]     = MIN_ACCEL;
            ap->hi[ACCEL_ACCEL]     = MAX_ACCEL;
            ap->lo[ACCEL_GOALTIME]  = MINTIME_ACCEL;
            ap->hi[ACCEL_GOALTIME]  = duration;
            ap->lo[ACCEL_STARTTIME] = 0.0;
            ap->hi[ACCEL_STARTTIME] = duration - MINTIME_ACCEL;
            break;
        case(MOD_VIBRATO):
            ap->lo[VIB_FRQ]         = 0.0;
            ap->hi[VIB_FRQ]         = MAX_VIB_FRQ;
            ap->lo[VIB_DEPTH]       = 0.0;
            ap->hi[VIB_DEPTH]       = EIGHT_8VA_UP;
            break;
        }
        break;

    case(MOD_REVECHO):
        switch(mode) {
        case(MOD_STADIUM):
            ap->lo[STAD_PREGAIN]    = 1.0/(double)MAXSHORT;
            ap->hi[STAD_PREGAIN]    = 1.0;
            ap->lo[STAD_ROLLOFF]    = 1.0/(double)MAXSHORT;
            ap->hi[STAD_ROLLOFF]    = 1.0;
            ap->lo[STAD_SIZE]       = (DFLT_STAD_DELTIME * 2.0)/srate;
            ap->hi[STAD_SIZE]       = MAX_STAD_DELAY;   /* arbitrary */
            ap->lo[STAD_ECHOCNT]    = (double)2;
            ap->hi[STAD_ECHOCNT]    = (double)MAX_ECHOCNT;
            break;
        case(MOD_DELAY):
        case(MOD_VDELAY):
            ap->lo[DELAY_LFOMOD]    = 0.0;
            ap->hi[DELAY_LFOMOD]    = 1.0;
            ap->lo[DELAY_LFOFRQ]    = -50.0;
            ap->hi[DELAY_LFOFRQ]    = 50.0;
            ap->lo[DELAY_LFOPHASE]  = 0.0;
            ap->hi[DELAY_LFOPHASE]  = 1.0;
            ap->lo[DELAY_LFODELAY]  = 0.0;
            ap->hi[DELAY_LFODELAY]  = duration;
            ap->lo[DELAY_DELAY]     = (1.0/sr) * SECS_TO_MS;
            ap->hi[DELAY_DELAY]     = MAX_DELAY;
            ap->lo[DELAY_MIX]       = 0.0;
            ap->hi[DELAY_MIX]       = 1.0;
            ap->lo[DELAY_FEEDBACK]  = -1.0;
            ap->hi[DELAY_FEEDBACK]  = 1.0;
            ap->lo[DELAY_TRAILTIME] = 0.0;
            ap->hi[DELAY_TRAILTIME] = 30.0;
            ap->lo[DELAY_PRESCALE]  = 1.0/(double)MAXSHORT;
            ap->hi[DELAY_PRESCALE]  = 1.0;
            if(mode==MOD_VDELAY) {
                ap->lo[DELAY_SEED]  = 0.0;
                ap->hi[DELAY_SEED]  = 256.0;
            }
            break;
        }
        break;

    case(MOD_RADICAL):
        switch(mode) {
        case(MOD_REVERSE):
            break;
        case(MOD_SHRED):
            ap->lo[SHRED_CNT]       = 1.0;
            ap->hi[SHRED_CNT]       = (double)MAXSHRED;
            ap->lo[SHRED_CHLEN]     = (double)((SHRED_SPLICELEN  * 3)/(double)srate);
            ap->hi[SHRED_CHLEN]     = (duration/2)-FLTERR;
            ap->lo[SHRED_SCAT]      = 0.0;
            ap->hi[SHRED_SCAT]      = (double)MAX_SHR_SCATTER;
            break;
        case(MOD_SCRUB):
            ap->lo[SCRUB_TOTALDUR]      = (double)channels/(double)srate;
            ap->hi[SCRUB_TOTALDUR]      = BIG_TIME;
            ap->lo[SCRUB_MINSPEED]      = SCRUB_SPEED_MIN;
            ap->hi[SCRUB_MINSPEED]      = SCRUB_SPEED_MAX;
            ap->lo[SCRUB_MAXSPEED]      = SCRUB_SPEED_MIN;
            ap->hi[SCRUB_MAXSPEED]      = SCRUB_SPEED_MAX;
            ap->lo[SCRUB_STARTRANGE]    = 0.0;
            ap->hi[SCRUB_STARTRANGE]    = duration - ((double)channels/(double)srate);
            ap->lo[SCRUB_ESTART]        = (double)channels/(double)srate;
            ap->hi[SCRUB_ESTART]        = duration;
            break;
        case(MOD_LOBIT):
            ap->lo[LOBIT_BRES]      = 1.0;
            ap->hi[LOBIT_BRES]      = (double)MAX_BIT_DIV;
            ap->lo[LOBIT_TSCAN]     = 1.0;
            ap->hi[LOBIT_TSCAN]     = (double)MAX_SRATE_DIV;
            break;
        case(MOD_LOBIT2):
            ap->lo[LOBIT_BRES]      = 1.0;
            ap->hi[LOBIT_BRES]      = (double)MAX_BIT_DIV;
            break;
        case(MOD_RINGMOD):
            ap->lo[RM_FRQ]          = MIN_RING_MOD_FRQ;
            ap->hi[RM_FRQ]          = nyquist;
            break;
        case(MOD_CROSSMOD): 
            break;
        }
        break;

    case(BRASSAGE):
        ap->lo[GRS_VELOCITY]        = 0.0;
        ap->hi[GRS_VELOCITY]        = GRS_MAX_VELOCITY;
        switch(mode) {
        case(GRS_REVERB):
            ap->lo[GRS_DENSITY]     = GRS_DEFAULT_DENSITY;
            ap->hi[GRS_DENSITY]     = GRS_MAX_DENSITY;
            break;
        case(GRS_GRANULATE):
            ap->lo[GRS_DENSITY]     = FLTERR;
            ap->hi[GRS_DENSITY]     = GRS_DEFAULT_DENSITY;
            break;
        default:
            ap->lo[GRS_DENSITY]     = FLTERR;
            ap->hi[GRS_DENSITY]     = GRS_MAX_DENSITY;
            break;
        }
        ap->lo[GRS_HVELOCITY]       = 0.0;
        ap->hi[GRS_HVELOCITY]       = MAX_GRS_HVELOCITY;
        ap->lo[GRS_HDENSITY]        = (1.0/sr);
        ap->hi[GRS_HDENSITY]        = (double)MAXSHORT/2.0;
        switch(mode) {
        case(GRS_PITCHSHIFT):   
        case(GRS_TIMESTRETCH):  
        case(GRS_SCRAMBLE): 
            ap->lo[GRS_GRAINSIZE]   = GRS_DEFAULT_SPLICELEN * 2.0;
            ap->lo[GRS_HGRAINSIZE]  = GRS_DEFAULT_SPLICELEN * 2.0;
        default:
            ap->lo[GRS_GRAINSIZE]   = GRS_MIN_SPLICELEN * 2.0;
            ap->lo[GRS_HGRAINSIZE]  = GRS_MIN_SPLICELEN * 2.0;
        }
        ap->hi[GRS_GRAINSIZE]       = (infilesize_in_samps/sr) * SECS_TO_MS;
        ap->hi[GRS_HGRAINSIZE]      = (infilesize_in_samps/sr) * SECS_TO_MS;
        switch(mode) {
        case(GRS_REVERB):
            ap->lo[GRS_PITCH]       = -GRS_DEFAULT_REVERB_PSHIFT;
            ap->hi[GRS_PITCH]       = GRS_DEFAULT_REVERB_PSHIFT;
            break;
        default:
            ap->hi[GRS_PITCH]       = LOG2(nyquist/MIN_LIKELY_PITCH) * SEMITONES_PER_OCTAVE;
            ap->lo[GRS_PITCH]       = -(ap->hi[GRS_PITCH]);
            break;
        }
        ap->lo[GRS_AMP]             = 0.0;
        ap->hi[GRS_AMP]             = 1.0;
        ap->lo[GRS_SPACE]           = 0.0;
        ap->hi[GRS_SPACE]           = 16.0;
        ap->lo[GRS_BSPLICE]         = GRS_MIN_SPLICELEN;
        ap->hi[GRS_BSPLICE]         = ap->hi[GRS_GRAINSIZE]/2.0;
        ap->lo[GRS_ESPLICE]         = GRS_MIN_SPLICELEN;
        ap->hi[GRS_ESPLICE]         = ap->hi[GRS_GRAINSIZE]/2.0;
//TW UPDATE
//      ap->hi[GRS_HPITCH]          = LOG2(nyquist/MIN_LIKELY_PITCH);
        ap->hi[GRS_HPITCH]          = LOG2(nyquist/MIN_LIKELY_PITCH) * SEMITONES_PER_OCTAVE;
        ap->lo[GRS_HPITCH]          = -(ap->hi[GRS_HPITCH]);
        ap->lo[GRS_HAMP]            = 0.0;
        ap->hi[GRS_HAMP]            = 1.0;
        ap->lo[GRS_HSPACE]          = 0.0;
        ap->hi[GRS_HSPACE]          = 16.0;
        ap->lo[GRS_HBSPLICE]        = GRS_MIN_SPLICELEN;
        ap->hi[GRS_HBSPLICE]        = ap->hi[GRS_GRAINSIZE]/2.0;
        ap->lo[GRS_HESPLICE]        = GRS_MIN_SPLICELEN;
        ap->hi[GRS_HESPLICE]        = ap->hi[GRS_GRAINSIZE]/2.0;
        switch(mode) {
        case(GRS_REVERB):
            ap->lo[GRS_SRCHRANGE]   = 0.0;
            ap->hi[GRS_SRCHRANGE]   = GRS_DEFAULT_REVERB_SRCHRANGE;
            break;
        case(GRS_SCRAMBLE):
        case(GRS_BRASSAGE):
        case(GRS_FULL_MONTY):
            ap->lo[GRS_SRCHRANGE]   = 0.0;
            ap->hi[GRS_SRCHRANGE]   = (duration * 2.0) * SECS_TO_MS;
            break;
        }
        switch(mode) {
        case(GRS_BRASSAGE):
        case(GRS_FULL_MONTY):
            ap->lo[GRS_SCATTER]         = 0.0;
            ap->hi[GRS_SCATTER]         = 1.0;
            ap->lo[GRS_OUTLEN]          = 0.0;
            ap->hi[GRS_OUTLEN]          = BIG_TIME;
            ap->lo[GRS_CHAN_TO_XTRACT]  = -16.0;
            ap->hi[GRS_CHAN_TO_XTRACT]  = (double)16.0;
            break;
        }
        break;
    case(SAUSAGE):
        ap->lo[GRS_VELOCITY]        = 0.0;
        ap->hi[GRS_VELOCITY]        = GRS_MAX_VELOCITY;
        ap->lo[GRS_DENSITY]         = FLTERR;
        ap->hi[GRS_DENSITY]         = GRS_MAX_DENSITY;
        ap->lo[GRS_HVELOCITY]       = 0.0;
        ap->hi[GRS_HVELOCITY]       = MAX_GRS_HVELOCITY;
        ap->lo[GRS_HDENSITY]        = (1.0/sr);
        ap->hi[GRS_HDENSITY]        = (double)MAXSHORT/2.0;
        ap->lo[GRS_GRAINSIZE]       = GRS_MIN_SPLICELEN * 2.0;
        ap->lo[GRS_HGRAINSIZE]      = GRS_MIN_SPLICELEN * 2.0;
        ap->hi[GRS_GRAINSIZE]       = (infilesize_in_samps/sr) * SECS_TO_MS;
        ap->hi[GRS_HGRAINSIZE]      = (infilesize_in_samps/sr) * SECS_TO_MS;
        ap->hi[GRS_PITCH]           = LOG2(nyquist/MIN_LIKELY_PITCH) * SEMITONES_PER_OCTAVE;
        ap->lo[GRS_PITCH]           = -(ap->hi[GRS_PITCH]);
        ap->lo[GRS_AMP]             = 0.0;
        ap->hi[GRS_AMP]             = 1.0;
        ap->lo[GRS_SPACE]           = 0.0;
        ap->hi[GRS_SPACE]           = 16.0;
        ap->lo[GRS_BSPLICE]         = GRS_MIN_SPLICELEN;
        ap->hi[GRS_BSPLICE]         = ap->hi[GRS_GRAINSIZE]/2.0;
        ap->lo[GRS_ESPLICE]         = GRS_MIN_SPLICELEN;
        ap->hi[GRS_ESPLICE]         = ap->hi[GRS_GRAINSIZE]/2.0;
//TW REVISION
//      ap->hi[GRS_HPITCH]          = LOG2(nyquist/MIN_LIKELY_PITCH);
        ap->hi[GRS_HPITCH]          = LOG2(nyquist/MIN_LIKELY_PITCH) * SEMITONES_PER_OCTAVE;
        ap->lo[GRS_HPITCH]          = -(ap->hi[GRS_HPITCH]);
        ap->lo[GRS_HAMP]            = 0.0;
        ap->hi[GRS_HAMP]            = 1.0;
        ap->lo[GRS_HSPACE]          = 0.0;
        ap->hi[GRS_HSPACE]          = 16.0;
        ap->lo[GRS_HBSPLICE]        = GRS_MIN_SPLICELEN;
        ap->hi[GRS_HBSPLICE]        = ap->hi[GRS_GRAINSIZE]/2.0;
        ap->lo[GRS_HESPLICE]        = GRS_MIN_SPLICELEN;
        ap->hi[GRS_HESPLICE]        = ap->hi[GRS_GRAINSIZE]/2.0;
        ap->lo[GRS_SRCHRANGE]       = 0.0;
        ap->hi[GRS_SRCHRANGE]       = (duration * 2.0) * SECS_TO_MS;
        ap->lo[GRS_SCATTER]         = 0.0;
        ap->hi[GRS_SCATTER]         = 1.0;
        ap->lo[GRS_OUTLEN]          = 0.0;
        ap->hi[GRS_OUTLEN]          = BIG_TIME;
        ap->lo[GRS_CHAN_TO_XTRACT]  = -16.0;
        ap->hi[GRS_CHAN_TO_XTRACT]  = 16.0;
        break;
    case(PVOC_ANAL):
        ap->lo[PVOC_CHANS_INPUT]    = (double)2;
        ap->hi[PVOC_CHANS_INPUT]    = (double)MAX_PVOC_CHANS;
        ap->lo[PVOC_WINOVLP_INPUT]  = (double)1;
        ap->hi[PVOC_WINOVLP_INPUT]  = (double)4;
        break;
    case(PVOC_SYNTH):
        break;
    case(PVOC_EXTRACT):
        ap->lo[PVOC_CHANS_INPUT]    = (double)2;
        ap->hi[PVOC_CHANS_INPUT]    = (double)MAX_PVOC_CHANS;
        ap->lo[PVOC_WINOVLP_INPUT]  = (double)1;
        ap->hi[PVOC_WINOVLP_INPUT]  = (double)4;
        ap->lo[PVOC_CHANSLCT_INPUT] = 0.0;
        ap->hi[PVOC_CHANSLCT_INPUT] = 2.0;
        ap->lo[PVOC_LOCHAN_INPUT]   = 0.0;
        ap->hi[PVOC_LOCHAN_INPUT]   = (double)(MAX_PVOC_CHANS/2);
        ap->lo[PVOC_HICHAN_INPUT]   = 0.0;
        ap->hi[PVOC_HICHAN_INPUT]   = (double)(MAX_PVOC_CHANS/2);
        break;

/* TEMPORARY TEST ROUTINE */
    case(WORDCNT):
        break;
/* TEMPORARY TEST ROUTINE */

    case(EDIT_CUT):
        ap->lo[CUT_CUT]         = 0.0;
        switch(mode) {
        case(EDIT_SECS):    ap->hi[CUT_CUT] = duration;                break;
        case(EDIT_SAMPS):   ap->hi[CUT_CUT] = (double)(insams);        break;
        case(EDIT_STSAMPS): ap->hi[CUT_CUT] = (double)(insams/channels); break;
        }
        ap->lo[CUT_END]         = 0.0;
        switch(mode) {
        case(EDIT_SECS):    ap->hi[CUT_END] = duration;                 break;
        case(EDIT_SAMPS):   ap->hi[CUT_END] = (double)(insams);        break;
        case(EDIT_STSAMPS): ap->hi[CUT_END] = (double)(insams/channels); break;
        }
        ap->lo[CUT_SPLEN]       = 0.0;
        ap->hi[CUT_SPLEN]       = EDIT_SPLICEMAX;
        break;
    case(EDIT_CUTEND):
        ap->lo[CUT_CUT]         = 0.0;
        switch(mode) {
        case(EDIT_SECS):    ap->hi[CUT_CUT] = duration;                  break;
        case(EDIT_SAMPS):   ap->hi[CUT_CUT] = (double)(insams);          break;
        case(EDIT_STSAMPS): ap->hi[CUT_CUT] = (double)(insams/channels); break;
        }
        ap->lo[CUT_SPLEN]       = 0.0;
        ap->hi[CUT_SPLEN]       = EDIT_SPLICEMAX;
        break;
    case(MANY_ZCUTS):
        break;
    case(EDIT_ZCUT):
        ap->lo[CUT_CUT]         = 0.0;
        switch(mode) {
        case(EDIT_SECS):    ap->hi[CUT_CUT] = duration;                  break;
        case(EDIT_SAMPS):   ap->hi[CUT_CUT] = (double)(insams);          break;
        case(EDIT_STSAMPS): ap->hi[CUT_CUT] = (double)(insams/channels); break;
        }
        ap->lo[CUT_END]         = 0.0;
        switch(mode) {
        case(EDIT_SECS):    ap->hi[CUT_END] = duration;                  break;
        case(EDIT_SAMPS):   ap->hi[CUT_END] = (double)(insams);          break;
        case(EDIT_STSAMPS): ap->hi[CUT_END] = (double)(insams/channels); break;
        }
        break;
    case(EDIT_EXCISE):
        ap->lo[CUT_CUT]         = 0.0;
        switch(mode) {
        case(EDIT_SECS):    ap->hi[CUT_CUT] = duration;                  break;
        case(EDIT_SAMPS):   ap->hi[CUT_CUT] = (double)(insams);          break;
        case(EDIT_STSAMPS): ap->hi[CUT_CUT] = (double)(insams/channels); break;
        }
        ap->lo[CUT_END]         = 0.0;
        switch(mode) {
        case(EDIT_SECS):    ap->hi[CUT_END] = duration;                  break;
        case(EDIT_SAMPS):   ap->hi[CUT_END] = (double)(insams);          break;
        case(EDIT_STSAMPS): ap->hi[CUT_END] = (double)(insams/channels); break;
        }
        ap->lo[CUT_SPLEN]       = 0.0;
        ap->hi[CUT_SPLEN]       = EDIT_SPLICEMAX;
        break;
    case(EDIT_EXCISEMANY):
    case(INSERTSIL_MANY):
        ap->lo[CUT_SPLEN]       = 0.0;
        ap->hi[CUT_SPLEN]       = EDIT_SPLICEMAX;
        break;
    case(EDIT_INSERT):
        ap->lo[CUT_CUT]         = 0.0;
        switch(mode) {
        case(EDIT_SECS):    ap->hi[CUT_CUT] = duration;                  break;
        case(EDIT_SAMPS):   ap->hi[CUT_CUT] = (double)(insams);          break;
        case(EDIT_STSAMPS): ap->hi[CUT_CUT] = (double)(insams/channels); break;
        }
        ap->lo[CUT_SPLEN]       = 0.0;
        ap->hi[CUT_SPLEN]       = EDIT_SPLICEMAX;
        ap->lo[INSERT_LEVEL]    = FLTERR;
        ap->hi[INSERT_LEVEL]    = EDIT_MAXGAIN;
        break;
//TW NEW CASE
        case(EDIT_INSERT2):
        ap->lo[CUT_CUT]         = 0.0;
        ap->hi[CUT_CUT]         = duration;
        ap->lo[CUT_END]         = 0.0;
        ap->hi[CUT_END]         = duration;
        ap->lo[CUT_SPLEN]       = 0.0;
        ap->hi[CUT_SPLEN]       = EDIT_SPLICEMAX;
        ap->lo[INSERT_LEVEL]    = FLTERR;
        ap->hi[INSERT_LEVEL]    = EDIT_MAXGAIN;
        break;

    case(EDIT_INSERTSIL):
        ap->lo[CUT_CUT]         = 0.0;
        switch(mode) {
        case(EDIT_SECS):    ap->hi[CUT_CUT] = duration;                  break;
        case(EDIT_SAMPS):   ap->hi[CUT_CUT] = (double)(insams);          break;
        case(EDIT_STSAMPS): ap->hi[CUT_CUT] = (double)(insams/channels); break;
        }
        ap->lo[CUT_END]         = 0.0;
        switch(mode) {
        case(EDIT_SECS):    ap->hi[CUT_END] = duration;                  break;
        case(EDIT_SAMPS):   ap->hi[CUT_END] = (double)(insams);          break;
        case(EDIT_STSAMPS): ap->hi[CUT_END] = (double)(insams/channels); break;
        }
        ap->lo[CUT_SPLEN]       = 0.0;
        ap->hi[CUT_SPLEN]       = EDIT_SPLICEMAX;
        break;
    case(JOIN_SEQ):
        ap->lo[MAX_LEN]         = 1;
        ap->hi[MAX_LEN]         = 32767;
        /* fall thro */
    case(EDIT_JOIN):
    case(JOIN_SEQDYN):
        ap->lo[CUT_SPLEN]       = 0.0;
        ap->hi[CUT_SPLEN]       = EDIT_SPLICEMAX;
        break;
    case(HOUSE_COPY):
        switch(mode) {
        case(COPYSF):
            break;
        case(DUPL):
            ap->lo[COPY_CNT] = 1.0;
            ap->hi[COPY_CNT] = (double)MAXDUPL;
            break;
        }
        break;
    case(HOUSE_DEL):
        break;
    case(HOUSE_CHANS):
        switch(mode) {
        case(HOUSE_CHANNEL):
            ap->lo[CHAN_NO] = 1.0;
            ap->hi[CHAN_NO] = (double)channels;
            break;
        case(HOUSE_ZCHANNEL):
            ap->lo[CHAN_NO] = 1.0;
            ap->hi[CHAN_NO] = (double)channels;
            break;
        case(HOUSE_CHANNELS):
        case(STOM):
        case(MTOS):
            break;
        }
        break;
    case(HOUSE_BUNDLE):
        break;
    case(HOUSE_SORT):
        switch(mode) {
        case(BY_DURATION):
        case(BY_LOG_DUR):
            ap->lo[SORT_SMALL]  = MIN_SORT_DUR;
            ap->hi[SORT_SMALL]  = MAX_SORT_DUR;
            ap->lo[SORT_LARGE]  = MIN_SORT_DUR;
            ap->hi[SORT_LARGE]  = MAX_SORT_DUR;
            if(mode==BY_DURATION) {
                ap->lo[SORT_STEP] = MIN_SORT_STEP;
                ap->hi[SORT_STEP] = MAX_SORT_STEP;
            } else {
                ap->lo[SORT_STEP] = MIN_SORT_LSTEP;
                ap->hi[SORT_STEP] = MAX_SORT_LSTEP;
            }
            break;
        }
        break;
    case(HOUSE_SPEC):
        switch(mode) {
        case(HOUSE_REPROP):
            ap->lo[HSPEC_CHANS] = (double)1;
            ap->hi[HSPEC_CHANS] = (double)MAX_SNDFILE_OUTCHANS;
            ap->lo[HSPEC_TYPE]  = (double)SAMP_SHORT;
            ap->hi[HSPEC_TYPE]  = (double)SAMP_FLOAT;
            /* fall thro */
        case(HOUSE_RESAMPLE):
            ap->lo[HSPEC_SRATE] = (double)16000;
            ap->hi[HSPEC_SRATE] = (double)/*48000*/96000;         /*RWD 4:05 */
            break;
        default:
            break;
        }
        break;
    case(HOUSE_EXTRACT):
        switch(mode) {
        case(HOUSE_CUTGATE):
            ap->lo[CUTGATE_SPLEN]   = CUTGATE_MINSPLICE;
            ap->hi[CUTGATE_SPLEN]   = CUTGATE_MAXSPLICE;
            ap->lo[CUTGATE_SUSTAIN] = 0.0;
            ap->hi[CUTGATE_SUSTAIN] = (double)CUTGATE_MAX_SUSTAIN;
            /* fall thro */
        case(HOUSE_ONSETS):
            ap->lo[CUTGATE_GATE]    = 0.0;
            ap->hi[CUTGATE_GATE]    = 1.0;
            ap->lo[CUTGATE_ENDGATE] = 0.0;
            ap->hi[CUTGATE_ENDGATE] = 1.0;
            ap->lo[CUTGATE_THRESH]  = 0.0;
            ap->hi[CUTGATE_THRESH]  = 1.0;
            ap->lo[CUTGATE_BAKTRAK] = 0.0;
            ap->hi[CUTGATE_BAKTRAK] = (double)CUTGATE_MAXBAKTRAK;
            ap->lo[CUTGATE_INITLEVEL]= 0.0;
            ap->hi[CUTGATE_INITLEVEL]= 1.0;
            ap->lo[CUTGATE_MINLEN]  = 0.0;
            ap->hi[CUTGATE_MINLEN]  = duration;
            ap->lo[CUTGATE_WINDOWS] = 0.0;
            ap->hi[CUTGATE_WINDOWS] = (double)CUTGATE_MAXWINDOWS;
            break;
        case(HOUSE_CUTGATE_PREVIEW):
            break;
        case(HOUSE_TOPNTAIL):
            ap->lo[TOPN_GATE]   = 0.0;
            ap->hi[TOPN_GATE]   = 1.0;
// TW Mar 21: 2011
            ap->lo[TOPN_SPLEN]  = 0;
            ap->hi[TOPN_SPLEN]  = CUTGATE_MAXSPLICE;
            break;
        case(HOUSE_RECTIFY):
            ap->lo[RECTIFY_SHIFT] = -1.0;
            ap->hi[RECTIFY_SHIFT] = 1.0;
            break;
        }
        break;
    case(TOPNTAIL_CLICKS):
        ap->lo[TOPN_GATE]   = 0.0;
        ap->hi[TOPN_GATE]   = 1.0;
        ap->lo[TOPN_SPLEN]  = 0.0;
        ap->hi[TOPN_SPLEN]  = CUTGATE_MAXSPLICE;
        break;
    case(HOUSE_BAKUP):
        break;
    case(HOUSE_GATE):
        ap->lo[GATE_ZEROS]      = 1;
        ap->hi[GATE_ZEROS]      = 33;
        break;

    case(HOUSE_DISK):
        break;
    case(INFO_PROPS):
    case(INFO_SFLEN):
    case(INFO_TIMELIST):
    case(INFO_LOUDLIST):
    case(INFO_TIMEDIFF):
    case(INFO_LOUDCHAN):
    case(INFO_MAXSAMP):
        break;
    case(INFO_MAXSAMP2):
        ap->lo[MAX_STIME]   = 0.0;
        ap->hi[MAX_STIME]   = duration;
        ap->lo[MAX_ETIME]   = 0.0;
        ap->hi[MAX_ETIME]   = duration;
        break;
    case(INFO_TIMESUM):
        ap->lo[TIMESUM_SPLEN]   = 0.0;
        ap->hi[TIMESUM_SPLEN]   = MAX_TIMESUM_SPLEN;
        break;  
    case(INFO_SAMPTOTIME):
        ap->lo[INFO_SAMPS]      = 0.0;
        ap->hi[INFO_SAMPS]      = insams;
        break;
    case(INFO_TIMETOSAMP):
        ap->lo[INFO_TIME]       = 0.0;
        ap->hi[INFO_TIME]       = duration;
        break;
    case(INFO_FINDHOLE):
        ap->lo[HOLE_THRESH] = 0.0;
        ap->hi[HOLE_THRESH] = 1.0;
        break;  
    case(INFO_CDIFF):
        ap->lo[SFDIFF_CNT]      = 1.0;
        ap->hi[SFDIFF_CNT]      = (double)MAX_SFDIFF_CNT;
        ap->lo[SFDIFF_THRESH]   = 0.0;
        ap->hi[SFDIFF_THRESH]   = 1.0;
        break;  
    case(INFO_DIFF):
        ap->lo[SFDIFF_CNT]      = 1.0;
        ap->hi[SFDIFF_CNT]      = (double)MAX_SFDIFF_CNT;
        ap->lo[SFDIFF_THRESH]   = 0.0;
        if(filetype==PITCHFILE)
            ap->hi[SFDIFF_THRESH] = nyquist;
        else if(filetype==TRANSPOSFILE)
            ap->hi[SFDIFF_THRESH] = nyquist/SPEC_MINFRQ;
        else
            ap->hi[SFDIFF_THRESH] = 1.0;
        break;  
    case(INFO_PRNTSND):
        ap->lo[PRNT_START]  = 0.0;
        ap->hi[PRNT_START]  = duration;
        ap->lo[PRNT_END]    = 0.0;
        ap->hi[PRNT_END]    = duration;
        break;  
    case(INFO_MUSUNITS):
        switch(mode) {
        case(MU_MIDI_TO_FRQ):
        case(MU_MIDI_TO_NOTE):
            ap->lo[0]   = MIDIMIN;
            ap->hi[0]   = MIDIMAX;
            break;
        case(MU_FRQ_TO_MIDI):
        case(MU_FRQ_TO_NOTE):
            ap->lo[0]   = miditohz(MIDIMIN);
            ap->hi[0]   = miditohz(MIDIMAX);
            break;
        case(MU_FRQRATIO_TO_SEMIT):
        case(MU_FRQRATIO_TO_INTVL):
        case(MU_FRQRATIO_TO_OCTS):
        case(MU_FRQRATIO_TO_TSTRETH):
            ap->lo[0]   = pow(2.0,-MAX_OCTSHIFT);
            ap->hi[0]   = pow(2.0,MAX_OCTSHIFT);
            break;
        case(MU_SEMIT_TO_FRQRATIO):
        case(MU_SEMIT_TO_OCTS):
        case(MU_SEMIT_TO_INTVL):
        case(MU_SEMIT_TO_TSTRETCH):
            ap->lo[0]   = -(MAX_OCTSHIFT * SEMITONES_PER_OCTAVE);
            ap->hi[0]   = MAX_OCTSHIFT * SEMITONES_PER_OCTAVE;
            break;
        case(MU_OCTS_TO_FRQRATIO):
        case(MU_OCTS_TO_SEMIT):
        case(MU_OCTS_TO_TSTRETCH):
            ap->lo[0]   = -MAX_OCTSHIFT;
            ap->hi[0]   = MAX_OCTSHIFT;
            break;
        case(MU_TSTRETCH_TO_FRQRATIO):
        case(MU_TSTRETCH_TO_SEMIT):
        case(MU_TSTRETCH_TO_OCTS):
        case(MU_TSTRETCH_TO_INTVL):
            ap->lo[0]   = pow(2.0,-MAX_OCTSHIFT);
            ap->hi[0]   = pow(2.0,MAX_OCTSHIFT);
            break;
        case(MU_GAIN_TO_DB):
            ap->lo[0]   = 0.0;
            ap->hi[0]   = MU_MAXGAIN;
            break;
        case(MU_DB_TO_GAIN):
            ap->lo[0]   = MIN_DB_ON_16_BIT;
            ap->hi[0]   = log10(MU_MAXGAIN) * 20.0;
            break;
        case(MU_NOTE_TO_FRQ):
        case(MU_NOTE_TO_MIDI):
        case(MU_INTVL_TO_FRQRATIO):
        case(MU_INTVL_TO_TSTRETCH):
            break;
        case(MU_DELAY_TO_FRQ):
        case(MU_DELAY_TO_MIDI):
            ap->lo[0]   = MU_MIN_DELAY;
            ap->hi[0]   = MU_MAX_DELAY;
            break;
        case(MU_MIDI_TO_DELAY):
            ap->lo[0]   = MIDIMIN;
            ap->hi[0]   = MIDIMAX;
            break;
        case(MU_FRQ_TO_DELAY):
            ap->lo[0]   = SECS_TO_MS/MU_MAX_DELAY;
            ap->hi[0]   = SECS_TO_MS/MU_MIN_DELAY;
            break;
        case(MU_DELAY_TO_TEMPO):
            ap->lo[0]   = MU_MINTEMPO_DELAY;
            ap->hi[0]   = MU_MAXTEMPO_DELAY;
            break;
        case(MU_TEMPO_TO_DELAY):
            ap->lo[0]   = MU_MIN_TEMPO;
            ap->hi[0]   = MU_MAX_TEMPO;
            break;
        case(MU_NOTE_TO_DELAY):
            break;
        default:
// TW UPDATE
//          sprintf(errstr,"Unknown mode for MUSUNITS: initialise_param_values()\n");
            sprintf(errstr,"Unknown mode for MUSUNITS: set_param_ranges()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(SYNTH_WAVE):
        ap->lo[SYN_TABSIZE]= (double)WAVE_TABSIZE;
        ap->hi[SYN_TABSIZE]= (double)(WAVE_TABSIZE * 16);
        ap->lo[SYN_FRQ] = MIN_SYNTH_FRQ;
        ap->hi[SYN_FRQ] = MAX_SYNTH_FRQ;
        /* fall thro */
    case(SYNTH_NOISE):
        ap->lo[SYN_AMP] = 0.0;
        ap->hi[SYN_AMP] = 1.0;
        ap->lo[SYN_SRATE] = 16000.0;
        ap->hi[SYN_SRATE] = 192000.0;   /*RWD wayhay! */
        ap->lo[SYN_CHANS] = 1.0;
        ap->hi[SYN_CHANS] = 16.0;
        ap->lo[SYN_DUR]   = MIN_SYN_DUR;
        ap->hi[SYN_DUR]   = MAX_SYN_DUR;
        break;
    case(SYNTH_SIL):
        ap->lo[SYN_SRATE] = 16000.0;
        ap->hi[SYN_SRATE] = 192000.0;
        ap->lo[SYN_CHANS] = 1.0;
        ap->hi[SYN_CHANS] = 16.0;
        ap->lo[SYN_DUR]   = FLTERR;
        ap->hi[SYN_DUR]   = MAX_SYN_DUR;
        break;
    case(MULTI_SYN):
        ap->lo[SYN_TABSIZE]= (double)WAVE_TABSIZE;
        ap->hi[SYN_TABSIZE]= (double)(WAVE_TABSIZE * 16);
        ap->lo[SYN_AMP] = 0.0;
        ap->hi[SYN_AMP] = 1.0;
        ap->lo[SYN_SRATE] = 16000.0;
        ap->hi[SYN_SRATE] = /*48000.0*/96000.0; /*RWD 4:05 */
        ap->lo[SYN_CHANS] = 1.0;
        ap->hi[SYN_CHANS] = 16.0;
        ap->lo[SYN_DUR]   = MIN_SYN_DUR;
        ap->hi[SYN_DUR]   = MAX_SYN_DUR;
        break;
    case(SYNTH_SPEC):
        ap->lo[SS_DUR]      = 0.1;
        ap->hi[SS_DUR]      = 32767;
        ap->lo[SS_SRATE]    = 16000;
        ap->hi[SS_SRATE]    = /*48000*/96000;   /*RWD 4:05 */   /*RWD push this to 192000 as well????? */
        ap->lo[SS_CENTRFRQ] = SPEC_MINFRQ;
        ap->hi[SS_CENTRFRQ] = ap->hi[SS_SRATE]/2.0;
        ap->lo[SS_SPREAD]   = 0.0;
        ap->hi[SS_SPREAD]   = ap->hi[SS_SRATE]/4.0;
        ap->lo[SS_FOCUS]    = 0.0;
        ap->hi[SS_FOCUS]    = 1.0;
        ap->lo[SS_FOCUS2]   = 0.0;
        ap->hi[SS_FOCUS2]   = 1.0;
        ap->lo[SS_TRAND]    = 0.0; 
        ap->hi[SS_TRAND]    = 1.0; 
        break;
    case(RANDCUTS):
        ap->lo[RC_CHLEN]  = (double)((SHRED_SPLICELEN * channels * 3)/(double)srate);
        ap->hi[RC_CHLEN]  = (duration/2)-FLTERR;
        ap->lo[RC_SCAT]   = 0.0;
        ap->hi[RC_SCAT]   = (double)MAX_SHR_SCATTER;
        break;
    case(RANDCHUNKS):
        ap->lo[CHUNKCNT]  = 2;
        ap->hi[CHUNKCNT]  = 999;
        ap->lo[MINCHUNK]  = MINOUTDUR;
        ap->hi[MINCHUNK]  = duration;
        ap->lo[MAXCHUNK]  = MINOUTDUR;
        ap->hi[MAXCHUNK]  = duration;
        break;
    case(TWIXT):
    case(SPHINX):
        ap->lo[IS_SPLEN]  = 2;
        ap->hi[IS_SPLEN]  = 15;
        ap->lo[IS_WEIGHT] = 1;
        ap->hi[IS_WEIGHT] = 10;
        ap->lo[IS_SEGCNT] = 1;
        ap->hi[IS_SEGCNT] = 10000;
        break;
//TW NEW CASES
    case(EDIT_CUTMANY):
        ap->lo[CM_SPLICELEN] = 0.0;
        ap->hi[CM_SPLICELEN] = EDIT_SPLICEMAX;
        break;
    case(STACK):
        ap->lo[STACK_CNT]    = 2;
        ap->hi[STACK_CNT]    = 32;
        ap->lo[STACK_LEAN]   = .01;
        ap->hi[STACK_LEAN]   = 100.0;
        ap->lo[STACK_OFFSET] = 0.0;
        ap->hi[STACK_OFFSET] = duration;
        ap->lo[STACK_GAIN]   = 0.1;
        ap->hi[STACK_GAIN]   = 10.0;
        ap->lo[STACK_DUR]    = 0.0;
        ap->hi[STACK_DUR]    = 1.0;
        break;

    case(SIN_TAB):
        ap->lo[SIN_FRQ]   = .01;
        ap->hi[SIN_FRQ]   = 100;
        ap->lo[SIN_AMP]   = 0;
        ap->hi[SIN_AMP]   = 1;
        ap->lo[SIN_DUR]   = 0;
        ap->hi[SIN_DUR]   = 32767;
        ap->lo[SIN_QUANT] = .005;
        ap->hi[SIN_QUANT] = 1;
        ap->lo[SIN_PHASE] = 0;
        ap->hi[SIN_PHASE] = 360;
        break;
    case(ACC_STREAM):
        ap->lo[ACC_ATTEN] = 0.0;
        ap->hi[ACC_ATTEN] = 1.0;
        break;
    case(HF_PERM1):
    case(HF_PERM2):
        ap->lo[HP1_SRATE]   = 16000;
        ap->hi[HP1_SRATE]   = /*48000*/96000;       /*RWD 4:05 */
        ap->lo[HP1_ELEMENT_SIZE] = 0.04;
        ap->hi[HP1_ELEMENT_SIZE] = 10.0;
        ap->lo[HP1_GAP_SIZE]  = 0.02;
        ap->hi[HP1_GAP_SIZE]  = 10.0;
        ap->lo[HP1_GGAP_SIZE] = 0.02;
        ap->hi[HP1_GGAP_SIZE] = 10.0;
        ap->lo[HP1_MINSET]  = 1.0;
        ap->hi[HP1_MINSET]  = MAX_HFPERMSET;
        ap->lo[HP1_BOTNOTE] = 0;
        ap->hi[HP1_BOTNOTE] = 11;
        ap->lo[HP1_BOTOCT]  = -4;
        ap->hi[HP1_BOTOCT]  = 4;
        ap->lo[HP1_TOPNOTE] = 0;
        ap->hi[HP1_TOPNOTE] = 11;
        ap->lo[HP1_TOPOCT]  = -4;
        ap->hi[HP1_TOPOCT]  = 4;
        ap->lo[HP1_SORT1]     = 0;
        ap->hi[HP1_SORT1]     = 4;
        break;
    case(DEL_PERM):
        ap->lo[DP_SRATE]  = 16000;
        ap->hi[DP_SRATE]  = /*48000*/96000;         /*RWD 4:05 */
        ap->lo[DP_DUR]    = 0.1;
        ap->hi[DP_DUR]    = 10.0;
        ap->lo[DP_CYCCNT] = 2;
        ap->hi[DP_CYCCNT] = 16;
        break;
    case(DEL_PERM2):
        ap->lo[DP_CYCCNT] = 2;
        ap->hi[DP_CYCCNT] = 16;
        break;
//TW NEW CASES
    case(NOISE_SUPRESS):
        ap->lo[NOISE_SPLEN] = 0.0;
        ap->hi[NOISE_SPLEN] = 50.0;
        ap->lo[NOISE_MINFRQ] = MIN_SUPRESS;
        ap->hi[NOISE_MINFRQ] = nyquist;
        ap->lo[MIN_NOISLEN] = 0.0;
        ap->hi[MIN_NOISLEN] = 50.0;
        ap->lo[MIN_TONELEN] = 0.0;
        ap->hi[MIN_TONELEN] = 1000.0;
        break;
    case(TIME_GRID):
        ap->lo[GRID_COUNT]  = 2.0;
        ap->hi[GRID_COUNT]  = 32.0;
        ap->lo[GRID_WIDTH]  = 0.002;
        ap->hi[GRID_WIDTH]  = 10.0;
        ap->lo[GRID_SPLEN]  = 2.0;
        ap->hi[GRID_SPLEN]  = 1000.0;
        break;
    case(SEQUENCER2):
        ap->lo[SEQ_SPLIC]  = 2.0;
        ap->hi[SEQ_SPLIC]  = 200.0;
        /* fall thro */
    case(SEQUENCER):
        ap->lo[SEQ_ATTEN]  = 0.0;
        ap->hi[SEQ_ATTEN]  = 1.0;
        break;
    case(CONVOLVE):
        if(mode==CONV_TVAR) {
            ap->lo[CONV_TRANS]  = -48.0;
            ap->hi[CONV_TRANS]  = 48.0;
        }
        break;
    case(BAKTOBAK):
        ap->lo[BTOB_CUT]    = 0.0;
        ap->hi[BTOB_CUT]    = duration;
        ap->lo[BTOB_SPLEN]  = 0.01;
        ap->hi[BTOB_SPLEN]  = min(5.0,duration) * SECS_TO_MS;
        break;
    case(CLICK):
        if(mode == CLICK_BY_LINE) {
            ap->lo[CLIKSTART]   = 1.0;
            ap->hi[CLIKSTART]   = 32767.0;
            ap->lo[CLIKEND]     = 2.0;
            ap->hi[CLIKEND]     = 32767.0;
        } else {
            ap->lo[CLIKSTART]   = 0.0;
            ap->hi[CLIKSTART]   = 32767.0;
            ap->lo[CLIKEND]     = 0.0;
            ap->hi[CLIKEND]     = 32767.0;
        }
        ap->lo[CLIKOFSET]   = 1.0;
        ap->hi[CLIKOFSET]   = 32767.0;
        break;
    case(DOUBLETS):
        ap->lo[SEG_DUR]    = SPLICEDUR * 2.0;
        ap->hi[SEG_DUR]    = max(ap->lo[SEG_DUR],min(10.0,duration));
        ap->lo[SEG_REPETS] = 2.0;
        ap->hi[SEG_REPETS] = 32.0;
        break;
    case(SYLLABS):
        ap->lo[SYLLAB_DOVETAIL]    = 0.0;
        ap->hi[SYLLAB_DOVETAIL]    = 20.0;
        ap->lo[SYLLAB_SPLICELEN]    = 1.0;
        ap->hi[SYLLAB_SPLICELEN]    = 20.0;
        break;
    case(MAKE_VFILT):
    case(MIX_MODEL):
        break;
    case(BATCH_EXPAND):
        ap->lo[BE_INFILE]   = 1;
        ap->hi[BE_INFILE]   = 40;
        ap->lo[BE_OUTFILE]  = 1;
        ap->hi[BE_OUTFILE]  = 40;
        ap->lo[BE_PARAM]    = 1;
        ap->hi[BE_PARAM]    = 40;
        break;
    case(ENVSYN):
        ap->lo[ENVSYN_WSIZE]        = ENV_MIN_WSIZE;
        ap->hi[ENVSYN_WSIZE]        = ENV_MAX_WSIZE;
        ap->lo[ENVSYN_DUR]          = ENV_MIN_WSIZE * MS_TO_SECS * 2.1; /* .1 is safety factor */
        ap->hi[ENVSYN_DUR]          = 32767.0;
        ap->lo[ENVSYN_CYCLEN]       = ENV_MIN_WSIZE * MS_TO_SECS * 2.1; /* .1 is safety factor */;
        ap->hi[ENVSYN_CYCLEN]       = ap->hi[ENVSYN_DUR];
        ap->lo[ENVSYN_STARTPHASE]   = 0.0;
        ap->hi[ENVSYN_STARTPHASE]   = 1.0;
        if(mode != ENVSYN_USERDEF) {
            ap->lo[ENVSYN_TROF]     = 0.0;
            ap->hi[ENVSYN_TROF]     = 1.0;
            ap->lo[ENVSYN_EXPON]    = DISTORTE_MIN_EXPON;
            ap->hi[ENVSYN_EXPON]    = DISTORTE_MAX_EXPON;
        }
        break;
    case(HOUSE_GATE2):
        ap->lo[GATE2_DUR]   = ((double)channels/(double)srate) * SECS_TO_MS;
        ap->hi[GATE2_DUR]   = 1.0 * SECS_TO_MS;
        ap->lo[GATE2_ZEROS] = 0.3;
        ap->hi[GATE2_ZEROS] = 1.0 * SECS_TO_MS;
        ap->lo[GATE2_LEVEL] = 0.0;
        ap->hi[GATE2_LEVEL] = (double)F_MAXSAMP;
        ap->lo[GATE2_SPLEN] = 0.0;
        ap->hi[GATE2_SPLEN] = 50.0;
        ap->lo[GATE2_FILT]  = ((double)channels/(double)srate) * SECS_TO_MS;
        ap->hi[GATE2_FILT]  = 4.0;
        break;
    case(GRAIN_ASSESS):
        break;
    case(ZCROSS_RATIO):
        ap->lo[ZC_START]        = 0.0;
        ap->hi[ZC_START]        = duration;
        ap->lo[ZC_END]          = 0.0;
        ap->hi[ZC_END]          = duration;
        break;
    case(GREV):
        ap->lo[GREV_WSIZE]      = (8.0/srate) * SECS_TO_MS;
        ap->hi[GREV_WSIZE]      = (duration/3.0) * SECS_TO_MS;
        ap->lo[GREV_TROFRAC]    = FLTERR;
        ap->hi[GREV_TROFRAC]    = 1.0 - FLTERR;
        ap->lo[GREV_GPCNT]      = 1.0;
        ap->hi[GREV_GPCNT]      = 100.0;
        switch(mode) {
        case(GREV_TSTRETCH):
            ap->lo[GREV_TSTR]   = .01;
            ap->hi[GREV_TSTR]   = 100;
            break;
        case(GREV_DELETE):
        case(GREV_OMIT):
            ap->lo[GREV_KEEP]   = 1;
            ap->hi[GREV_KEEP]   = 255;
            ap->lo[GREV_OUTOF]  = 2;
            ap->hi[GREV_OUTOF]  = 256;
            break;
        case(GREV_REPEAT):
            ap->lo[GREV_REPETS] = 1;
            ap->hi[GREV_REPETS] = 100;
            break;
        }
        break;
    default:
        sprintf(errstr,"Unknown case: get_param_ranges()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/**********************************************************************************/
/*********************** ITEMS FORMERLY IN HEADERS.C tklib3 ***********************/
/**********************************************************************************/

/***************************** READHEAD *******************************
 *
 * Header reader for sndfiles,envfiles,analfiles etc..
 */

int readhead(infileptr inputfile,int ifd,char *filename,double *maxamp,double *maxloc, int *maxrep,int getmax,int needmaxinfo)
{
    float peakval;
    int peakpos;
    SFPROPS props = {0};   // RWD NB on 64bit, alignment important here!
    int isenv = 0;
    int os;
    
    if(!snd_headread(ifd,&props)) {
        fprintf(stdout,"INFO: Failure to read properties, in %s\n",filename);
        fflush(stdout);
        return(DATA_ERROR);
    }
    inputfile->srate = props.srate;
    inputfile->channels = props.chans;

    switch(props.samptype) {
    case(SHORT16):  inputfile->stype = SAMP_SHORT;  break;
    case(FLOAT32):  inputfile->stype = SAMP_FLOAT;  break;
    case(SHORT8):   inputfile->stype = SAMP_BYTE;   break;
    default:    
        /* remaining symbols have same int value */ 
        inputfile->stype = (int)props.samptype; /* RWD April 2005 */
        break;
    }
//TW TEMPORARY SUBSTITUTION
    inputfile->filetype = SNDFILE;
    if(sndgetprop(ifd,"is an envelope",(char *) &isenv,sizeof(int)) >= 0)
        inputfile->filetype = ENVFILE;
    else if(sndgetprop(ifd,"is a formant file",(char *) &isenv,sizeof(int)) >= 0)
        inputfile->filetype = FORMANTFILE;
    else if(sndgetprop(ifd,"is a pitch file",(char *) &isenv,sizeof(int)) >= 0)
        inputfile->filetype = PITCHFILE;
    else if(sndgetprop(ifd,"is a transpos file",(char *) &isenv,sizeof(int)) >= 0)
        inputfile->filetype = TRANSPOSFILE;
    else if(sndgetprop(ifd,"original sampsize",(char *) &os,sizeof(int)) > 0)
        inputfile->filetype = ANALFILE;

/* THIS STILL DOESN'T WORK !!!!
    switch(props.type) {
    case(wt_wave):          inputfile->filetype = SNDFILE;      break;
    case(wt_analysis):      inputfile->filetype = ANALFILE;     break;
    case(wt_formant):       inputfile->filetype = FORMANTFILE;  break;
    case(wt_transposition): inputfile->filetype = TRANSPOSFILE; break;
    case(wt_pitch):         inputfile->filetype = PITCHFILE;    break;
    case(wt_binenv):        inputfile->filetype = ENVFILE;      break;
    default:
        fprintf(stdout,"INFO: Unknown file type,\n");
        return(DATA_ERROR);
    }
*/
    switch(inputfile->filetype) {
    case(SNDFILE):
        if(needmaxinfo) {
            *maxrep = (int)-1;  /* flags that maxinfo not got, unless it's got, below */
            *maxloc = (int)0;
            *maxamp = 0.0;
            if(getmax) {
                if(getpeakdata(ifd,&peakval,&peakpos,props) > 0) {
                    *maxamp = (double)peakval;
                    *maxloc = (double)peakpos;
                    *maxrep = 0;
                }
            }
        }
#ifdef TEST_HEADREAD
        if(headread_check(props,filename,ifd)<0)
            return(PROGRAM_ERROR);
#endif
        return(FINISHED);
    case(ENVFILE):
// TW NOT WORKING
//      inputfile->window_size = props.window_size;
        if(sndgetprop(ifd,"window size",(char *)&(inputfile->window_size),sizeof(float)) < 0) {
            fprintf(stdout,"Cannot read original window size: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
#ifdef TEST_HEADREAD
        if(headread_check(props,filename,ifd)<0)
            return(PROGRAM_ERROR);
#endif
        return(FINISHED);

    case(FORMANTFILE):
        inputfile->specenvcnt = props.specenvcnt;
        /* fall through */
    case(PITCHFILE):
    case(TRANSPOSFILE):
        if(inputfile->channels != 1) {
            sprintf(errstr,"Channel count not equal to 1 in %s: readhead()\n"
            "Implies failure to write correct header in another program.\n",
            filename);
            return(PROGRAM_ERROR);
        }
// TW NOT WORKING
//      inputfile->origchans = props.origchans;
// TW SUBSTITUTE FOR NOW
        if(sndgetprop(ifd,"orig channels",(char *)&(inputfile->origchans),sizeof(int)) < 0) {
            fprintf(stdout,"Cannot read original channels count: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }

        /* fall through */
    case(ANALFILE):
// TW NOT WORKING
//      inputfile->origstype = props.origsize;
//      inputfile->origrate = props.origrate;
//      inputfile->arate    = props.arate;
//      inputfile->Mlen     = props.winlen;
//      inputfile->Dfac     = props.decfac;
// TW SUBSTITUTE FOR NOW
        if(sndgetprop(ifd,"original sampsize",(char *)&(inputfile->origstype),sizeof(int)) < 0) {
            fprintf(stdout,"Cannot read original sample type: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(sndgetprop(ifd,"original sample rate",(char *)&(inputfile->origrate),sizeof(int)) < 0) {
            fprintf(stdout,"Cannot read original sample rate: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(sndgetprop(ifd,"arate",(char *)&(inputfile->arate),sizeof(float)) < 0) {
            fprintf(stdout,"Cannot read analysis rate: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(sndgetprop(ifd,"analwinlen",(char *)&(inputfile->Mlen),sizeof(int)) < 0) {
            fprintf(stdout,"Cannot read analysis window length: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(sndgetprop(ifd,"decfactor",(char *)&(inputfile->Dfac),sizeof(int)) < 0) {
            fprintf(stdout,"Cannot read original decimation factor: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }


#ifdef TEST_HEADREAD
        if(headread_check(props,filename,ifd)<0)
            return(PROGRAM_ERROR);
#endif
        break;
    }
    return(FINISHED);
}

/******************************* IS_A_PITCHFILE ****************************/

int is_a_pitchfile(int fd)
{
    int pppp = 1;
    if(sndgetprop(fd,"is a pitch file", (char *)&pppp, sizeof(int)) < 0)
        return(FALSE);
    return(TRUE);
}   

/******************************* IS_A_TRANSPOSFILE ****************************/

int is_a_transposfile(int fd)
{
    int tttt = 1;
    if(sndgetprop(fd,"is a transpos file", (char *)&tttt, sizeof(int)) < 0)
        return(FALSE);
    return(TRUE);
}   

/******************************* IS_A_FORMANTFILE ****************************/

int is_a_formantfile(int fd)
{   
    int ffff = 1;
    if(sndgetprop(fd,"is a formant file", (char *)&ffff, sizeof(int)) < 0)
        return(FALSE);
    return(TRUE);
}   

/************************************* REDEFINE_TEXTFILE_TYPES ******************************/

int redefine_textfile_types(dataptr dz)
{
    switch(dz->infile->filetype) {
    case(POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):   case(MAKE_VFILT):
            dz->infile->filetype = NUMLIST;
            break;
        case(P_GEN):
            dz->infile->filetype = POSITIVE_BRKFILE;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_PROPOR):
                dz->infile->filetype = POSITIVE_BRKFILE;
                break;
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        default:
            dz->infile->filetype = UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST;
            break;
        }
        break;
    case(POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):  case(MAKE_VFILT):
            dz->infile->filetype = NUMLIST;
            break;
        case(P_GEN):
            dz->infile->filetype = POSITIVE_BRKFILE;
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_PROPOR):
                dz->infile->filetype = POSITIVE_BRKFILE;
                break;
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        default:
            dz->infile->filetype = UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
            break;
        }
        break;
    case(PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):  case(MAKE_VFILT):
            dz->infile->filetype = NUMLIST;
            break;
        case(ENV_IMPOSE):
//TW NEW CASE
        case(ENV_PROPOR):
            dz->infile->filetype = POSITIVE_BRKFILE;
            break;
        case(P_GEN):
            dz->infile->filetype = POSITIVE_BRKFILE;
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_PROPOR):
                dz->infile->filetype = POSITIVE_BRKFILE;
                break;
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        default:
            dz->infile->filetype = PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST;
            break;
        }
        break;
    case(PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):   case(MAKE_VFILT):
            dz->infile->filetype = NUMLIST;
            break;
        case(ENV_IMPOSE):
//TW NEW CASE
        case(ENV_PROPOR):
            dz->infile->filetype = POSITIVE_BRKFILE;
            break;
        case(P_GEN):
            dz->infile->filetype = POSITIVE_BRKFILE;
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_PROPOR):
                dz->infile->filetype = POSITIVE_BRKFILE;
                break;
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        default:
            dz->infile->filetype = PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST;
            break;
        }
        break;

    
    }

    if(dz->process==INFO_DIFF) {
        sprintf(errstr,"This process does not work with textfiles.\n");
        return(GOAL_FAILED);
    } else if(dz->process==HOUSE_COPY) {
        dz->infile->filetype = WORDLIST;
        return(FINISHED);
    } else if(dz->input_data_type==NO_FILE_AT_ALL) {
        dz->infile->filetype = WORDLIST; /* dummy */
        return(FINISHED);
    }

    switch(dz->infile->filetype) {
    case(TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(REPITCH):
        case(REPITCHB):
            dz->infile->filetype = TEXTFILE;
            return(FINISHED);
        }
        /* fall thro */
    case(NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(ENV_REPLOTTING):
        case(ENV_BRKTOENV):
        case(ENV_BRKTODBBRK):
        case(INFO_PROPS):
            dz->infile->filetype = BRKFILE;
            return(FINISHED);
        case(MOD_SPACE):
            if(dz->mode==MOD_MIRRORPAN) {
                dz->infile->filetype = UNRANGED_BRKFILE;
                return(FINISHED);
            }
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_PROPOR):
                dz->infile->filetype = POSITIVE_BRKFILE;
                return(FINISHED);
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                return(FINISHED);
            }
            break;
        }
        /* fall thro */
    case(NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(INFO_PROPS):
            dz->infile->filetype = NUMLIST;
            return(FINISHED);
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):  case(MAKE_VFILT):
            dz->infile->filetype = NUMLIST;
            return(FINISHED);
        }
        /* fall thro */
    case(LINELIST_OR_WORDLIST):
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(REPITCH):
        case(REPITCHB):
            dz->infile->filetype = TEXTFILE;
            return(FINISHED);
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        }
        /* fall tro */
    case(NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(ENV_REPLOTTING):
        case(ENV_BRKTOENV):
        case(ENV_BRKTODBBRK):
        case(INFO_PROPS):
            dz->infile->filetype = BRKFILE;
            return(FINISHED);
        case(MOD_SPACE):
            if(dz->mode==MOD_MIRRORPAN) {
                dz->infile->filetype = UNRANGED_BRKFILE;
                return(FINISHED);
            }
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_PROPOR):
                dz->infile->filetype = POSITIVE_BRKFILE;
                break;
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        }
        /* fall thro */
    case(NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):  case(MAKE_VFILT): 
            dz->infile->filetype = NUMLIST;
            return(FINISHED);
        case(INFO_PROPS):
            dz->infile->filetype = NUMLIST;
            return(FINISHED);
        }
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(DB_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(ENV_DBBRKTOENV):
        case(ENV_DBBRKTOBRK):
        case(INFO_PROPS):
            dz->infile->filetype = DB_BRKFILE;
            return(FINISHED);
        case(MOD_SPACE):
            if(dz->mode==MOD_MIRRORPAN) {
                dz->infile->filetype = UNRANGED_BRKFILE;
                return(FINISHED);
            }
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        }
        /* fall thro */
    case(NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):  case(MAKE_VFILT):
            dz->infile->filetype = NUMLIST;
            return(FINISHED);
        }
        /* fall thro */
    case(LINELIST_OR_WORDLIST):
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(INFO_PROPS):
        case(HOUSE_SORT):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(DB_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(ENV_DBBRKTOENV):
        case(ENV_DBBRKTOBRK):
        case(INFO_PROPS):
            dz->infile->filetype = DB_BRKFILE;
            return(FINISHED);
        case(MOD_SPACE):
            if(dz->mode==MOD_MIRRORPAN) {
                dz->infile->filetype = UNRANGED_BRKFILE;
                return(FINISHED);
            }
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        }
        /* fall thro */
    case(NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):  case(MAKE_VFILT):
            dz->infile->filetype = NUMLIST;
            return(FINISHED);
        }
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
            dz->infile->filetype = WORDLIST;    
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(REPITCH):
        case(REPITCHB):
            dz->infile->filetype = TEXTFILE;
            return(FINISHED);
        }
        /* fall thro */
    case(UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(INFO_PROPS):
            dz->infile->filetype = UNRANGED_BRKFILE;
            return(FINISHED);
        case(MOD_SPACE):
            if(dz->mode==MOD_MIRRORPAN) {
                dz->infile->filetype = UNRANGED_BRKFILE;
                return(FINISHED);
            }
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        }
    /* fall thro */
    case(NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):   case(MAKE_VFILT): 
            dz->infile->filetype = NUMLIST;
            return(FINISHED);
        }
        /* fall thro */
    case(LINELIST_OR_WORDLIST):
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT): 
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(REPITCH):
        case(REPITCHB):
            dz->infile->filetype = TEXTFILE;
            return(FINISHED);
        }
        /* fall thro */
    case(UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(INFO_PROPS):
            dz->infile->filetype = UNRANGED_BRKFILE;
            return(FINISHED);
        case(MOD_SPACE):
            if(dz->mode==MOD_MIRRORPAN) {
                dz->infile->filetype = UNRANGED_BRKFILE;
                return(FINISHED);
            }
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        }
        /* fall thro */
    case(NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):  case(MAKE_VFILT):
            dz->infile->filetype = NUMLIST;
            return(FINISHED);
        }
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(MIXLIST_OR_LINELIST_OR_WORDLIST):
        /* fall thro */
    case(MIXLIST_OR_WORDLIST):
        switch(dz->process) {
        case(MIX):
        case(MIXMAX):
        case(MIXTEST):
        case(MIXGAIN):
        case(MIXSHUFL):
        case(MIXTWARP):
        case(MIXSWARP):
        case(MIXSYNC):
        case(MIXSYNCATT):
//TW NEW CASES
        case(ADDTOMIX):
        case(MIX_MODEL):
        case(MIX_PAN):
        case(MIX_AT_STEP):

        case(INFO_PROPS):
            dz->infile->filetype = MIXFILE;
            return(FINISHED);
        }
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(SNDLIST_OR_LINELIST_OR_WORDLIST): /* implies different srates */
        /* fall thro */
    case(SNDLIST_OR_WORDLIST):
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(SNDLIST_OR_SYNCLIST_OR_WORDLIST):
        switch(dz->process) {
        case(MIXDUMMY):
        case(INFO_PROPS):
            dz->infile->filetype = SNDLIST;
            return(FINISHED);
        case(MIXSYNC):
        case(MIXSYNCATT):
            dz->infile->filetype = SYNCLIST;
            return(FINISHED);
        }
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(SNDLIST_OR_SYNCLIST_LINELIST_OR_WORDLIST):    /* list of snds AT SAME SRATE */
        switch(dz->process) {
        case(MIXDUMMY):
        case(INFO_PROPS):
            dz->infile->filetype = SNDLIST;
            return(FINISHED);
        case(MIXSYNC):
        case(MIXSYNCATT):
            dz->infile->filetype = SYNCLIST;
            return(FINISHED);
        }
            /* fall thro */
    case(LINELIST_OR_WORDLIST):
            /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT): 
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(SYNCLIST_OR_LINELIST_OR_WORDLIST):   /* i.e. can't be a sndlist */
        switch(dz->process) {
        case(MIXSYNCATT): 
        case(INFO_PROPS):
            dz->infile->filetype = SYNCLIST;    
            return(FINISHED);
        }
        /* fall thro */
    case(LINELIST_OR_WORDLIST):
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(SYNCLIST_OR_WORDLIST):                /* i.e. can't be a sndlist */
        switch(dz->process) {
        case(MIXSYNCATT): 
        case(INFO_PROPS):
            dz->infile->filetype = SYNCLIST;    
            return(FINISHED);
        }
            /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT): 
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(LINELIST_OR_WORDLIST):
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(REPITCH):
        case(REPITCHB):
            dz->infile->filetype = TEXTFILE;
            return(FINISHED);
        case(INFO_PROPS):
            dz->infile->filetype = UNRANGED_BRKFILE;
            return(FINISHED);
        case(MOD_SPACE):
            if(dz->mode==MOD_MIRRORPAN) {
                dz->infile->filetype = UNRANGED_BRKFILE;
                return(FINISHED);
            }
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_PROPOR):
                dz->infile->filetype = POSITIVE_BRKFILE;
                break;
            }
            break;
        }
        /* fall thro */
    case(NUMLIST_OR_LINELIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):   case(MAKE_VFILT):
            dz->infile->filetype = NUMLIST;
            return(FINISHED);
        }
        /* fall thro */
    case(LINELIST_OR_WORDLIST):
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }

    switch(dz->infile->filetype) {
    case(TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(REPITCH):
        case(REPITCHB):
            dz->infile->filetype = TEXTFILE;
            return(FINISHED);
        case(INFO_PROPS):
            dz->infile->filetype = UNRANGED_BRKFILE;
            return(FINISHED);
        case(MOD_SPACE):
            if(dz->mode==MOD_MIRRORPAN) {
                dz->infile->filetype = UNRANGED_BRKFILE;
                return(FINISHED);
            }
            break;
        case(MOD_LOUDNESS):
            switch(dz->mode) {
            case(LOUD_PROPOR):
                dz->infile->filetype = POSITIVE_BRKFILE;
                break;
            case(LOUD_DB_PROPOR):
                dz->infile->filetype = DB_BRKFILE;
                break;
            }
            break;
        }
        /* fall thro */
    case(NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(HF_PERM1): case(HF_PERM2): case(DEL_PERM): case(DEL_PERM2):   case(MAKE_VFILT):
            dz->infile->filetype = NUMLIST;
            return(FINISHED);
        }
        /* fall thro */
    case(WORDLIST):
        switch(dz->process) {
        case(WORDCNT):
        case(HOUSE_DISK):
        case(HOUSE_BUNDLE):
        case(HOUSE_SORT):
        case(INFO_PROPS):
        case(INFO_SFLEN):
        case(INFO_MAXSAMP):
        case(INFO_MAXSAMP2):
        case(UTILS_GETCOL):
        case(UTILS_PUTCOL):
        case(UTILS_JOINCOL):
        case(UTILS_COLMATHS):
        case(UTILS_COLMUSIC):
        case(UTILS_COLRAND):
        case(UTILS_COLLIST):
        case(UTILS_COLGEN):
        case(BATCH_EXPAND):
            dz->infile->filetype = WORDLIST;
            return(FINISHED);
        default:
            sprintf(errstr,"type_conversion not done for this process: redefine_textfile_types()\n");
            return(PROGRAM_ERROR);
        }
    }
    sprintf(errstr,"Unknown input textfile type: redefine_textfile_types()\n");
    return(PROGRAM_ERROR);
}

/************************************* SOUND_LOOM_IN_USE ******************************/

int sound_loom_in_use(int *argc, char **argv[]) {
    char *p;
    if(*argc>1) {
        if(!strcmp((*argv)[1],"#")) {
            (*argv)++;
            (*argc)--;
            return 1;
        } else if (!strcmp((*argv)[1],"##")) {
            p = (*argv)[0];
            (*argc)--;
            (*argv)++;
            (*argv)[0] = p;
            return 2;
        }
    }
    return 0;
}

/************************ print_outmessage **********************/

void print_outmessage(char *str) {

    if(!sloom && !sloombatch)
        fprintf(stdout,"%s",str);
    else
        fprintf(stdout,"INFO: %s",str);
}

/************************ print_outmessage_flush **********************/

void print_outmessage_flush(char *str) {

    if(!sloom && !sloombatch)
        fprintf(stdout,"%s",str);
    else {
        fprintf(stdout,"INFO: %s",str);
        fflush(stdout);
    }
}

/********************** PRINT_OUTWARNING_FLUSH ***************************/

void print_outwarning_flush(char *str)
{
    if(!sloom && !sloombatch)
        fprintf(stdout,"%s",str);
    else {
        fprintf(stdout,"WARNING: %s",str);
        fflush(stdout);
    }
}

/**************************STRGETFLOAT **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

int  strgetfloat(char **str,double *val)
{   char *p, *q, *end;
    double numero;
    int point, valid;
    for(;;) {
        point = 0;
        p = *str;
        while(isspace(*p))
            p++;
        q = p;
        if(!isdigit(*p) && *p != '.' && *p!='-')
            return(0);
        if(*p == '.'|| *p == '-') {
            if(*p == '-') {
                p++;
            } else {
                point++;
                    p++;
            }
        }
        for(;;) {
            if(*p == '.') {
                if(point)
                    return(0);
                else {
                    point++;
                    p++;
                    continue;
                }
            }
            if(isdigit(*p)) {
                p++;
                continue;
            } else {
                if(!isspace(*p) && *p!=ENDOFSTR)
                    return(0);
                else {
                    end = p;
                    p = q;
                    valid = 0;
                    while(p!=end) {
                        if(isdigit(*p))
                            valid++;
                        p++;
                    }
                    if(valid) {
                        if(sscanf(q,"%lf",&numero)!=1)
                            return(0);
                        *val = numero;
                       *str = end;
                        return(1);
                    }
                    return(0);
                }
            }
        }
    }
    return(0);              /* NOTREACHED */
}

//TW: NEW FUNCTIONS BELOW

/********************************** STRGETFLOAT_WITHIN_STRING ***********************************
 *
 * Differs from strgetfloat, in that float in string need not be terminated by space or endofstring
 */

int  strgetfloat_within_string(char **str,double *val)
{   char *p, *q, c;
    int point = 0, digit = 0;
    p = *str;
    while(isspace(*p))
        p++;
    q = p;  
    for(;;) {
        if(*p == '-') {
            if (p != q)
                return(0);  /* negative sign only at start */
        } else if (*p == '.') {
            if(point)
                return(0);  /* only one decimal point */
            point++;
        } else if(isdigit(*p))
            digit++;
        else 
            break;
        p++;
    }
    if(!digit)
        return(0);
    c = *p;
    *p = ENDOFSTR;
    if(sscanf(q,"%lf",val)!=1) {
        *p = c;
        return(0);
    }
    *p = c;
    *str = p;
    return(1);
}

/********************* VALUE_IS_NUMERIC *********************/

int value_is_numeric(char *str)
{   char *p, *q, *end;
    int point, valid;
    for(;;) {
        point = 0;
        p = str;
        while(isspace(*p))
            p++;
        q = p;  
        if(!isdigit(*p) && *p != '.' && *p!='-')
            return(0);
        if(*p == '.'|| *p == '-') {
            if(*p == '-') {
                p++;
            } else {
                point++;
                p++;
            }
        }
        for(;;) {
            if(*p == '.') {
                if(point)
                    return(0);
                else {
                    point++;
                    p++;
                    continue;
                }
            }
            if(isdigit(*p)) {
                p++;
                continue;
            } else {
                if(!isspace(*p) && *p!=ENDOFSTR)
                    return(0);
                else {
                    end = p;
                    p = q;
                    valid = 0;
                    while(p!=end) {
                        if(isdigit(*p))
                            valid++;
                        p++;
                    }
                    if(valid)
                        return(1);
                    return(0);
                }
            }
        }
    }
    return(0);              /* NOTREACHED */
}

#ifdef TEST_HEADREAD
int headread_check(SFPROPS props,char *filename,int ifd)
{
    infileptr inputfile;
    int dummy;
    if((inputfile = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
        sprintf(errstr,"Insufficient memory for headread_check on file %s.\n",filename);
        return(MEMORY_ERROR);
    }
    if(sndgetprop(ifd,"sample rate", (char *)&(inputfile->srate), sizeof(int)) < 0){
        fprintf(stdout,"INFO: Cannot read sample rate in file: %s\n",filename);
        fflush(stdout);
        return(DATA_ERROR);
    }
    if(props.srate != inputfile->srate) {
        fprintf(stdout,"INFO: snd_headread gives srate %ld: correct val = %ld: file %s\n",props.srate,inputfile->srate,filename);
        fflush(stdout);
    }
    if(sndgetprop(ifd,"channels", (char *)&(inputfile->channels), sizeof(int)) < 0){
        fprintf(stdout,"INFO: Cannot read channel count in file: %s\n",filename);
        fflush(stdout);
        return(DATA_ERROR);
    }
    if(props.chans != inputfile->channels) {
        fprintf(stdout,"INFO: snd_headread gives channels %ld: correct val = %ld: file %s\n",props.chans,inputfile->channels,filename);
        fflush(stdout);
    }
    inputfile->filetype = SNDFILE;
    if(sndgetprop(ifd,"is an envelope",(char *)&dummy,sizeof(int)) >= 0)
        inputfile->filetype = ENVFILE;
    else if(sndgetprop(ifd,"is a pitch file",(char *)&dummy,sizeof(int)) >= 0)
        inputfile->filetype = PITCHFILE;
    else if(sndgetprop(ifd,"is a transpos file",(char *)&dummy,sizeof(int)) >= 0)
        inputfile->filetype = TRANSPOSFILE;
    else if(sndgetprop(ifd,"is a formant file",(char *)&dummy,sizeof(int)) >= 0)
        inputfile->filetype = FORMANTFILE;
    else if(sndgetprop(ifd,"original sampsize", (char *)&(inputfile->origstype), sizeof(int)) >= 0)
        inputfile->filetype = ANALFILE;
    
    switch(inputfile->filetype) {

    case(SNDFILE):
        return(FINISHED);
    case(ENVFILE):
        if(sndgetprop(ifd,"window size", (char *)&(inputfile->window_size), sizeof(float)) < 0){
            fprintf(stdout,"INFO: Cannot read envelope sindow size in file: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(props.window_size != inputfile->window_size) {
            fprintf(stdout,"INFO: snd_headread gives window_size %f: correct val = %f: file %s\n",props.window_size,inputfile->window_size,filename);
            fflush(stdout);
        }
        return(FINISHED);

    case(FORMANTFILE):
        if(sndgetprop(ifd,"specenvcnt", (char *)&(inputfile->specenvcnt), sizeof(int)) < 0){
            fprintf(stdout,"INFO: Cannot read specenvcnt in file: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(props.specenvcnt != inputfile->specenvcnt) {
            fprintf(stdout,"INFO: snd_headread gives specenvcnt %ld: correct val = %ld: file %s\n",props.specenvcnt,inputfile->specenvcnt,filename);
            fflush(stdout);
        }
        /* fall through */
    case(PITCHFILE):
    case(TRANSPOSFILE):
        if(sndgetprop(ifd,"orig channels", (char *)&(inputfile->origchans), sizeof(int)) < 0){
            fprintf(stdout,"INFO: Cannot read origchans in file: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(props.origchans != inputfile->origchans) {
            fprintf(stdout,"INFO: snd_headread gives origchans %ld: correct val = %ld: file %s\n",props.origchans,inputfile->origchans,filename);
            fflush(stdout);
        }
        /* fall through */
    case(ANALFILE):
        if(sndgetprop(ifd,"original sampsize", (char *)&(inputfile->origstype), sizeof(int)) < 0){
            fprintf(stdout,"INFO: Cannot read original sample type in file: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(sndgetprop(ifd,"original sample rate", (char *)&(inputfile->origrate), sizeof(int)) < 0){
            fprintf(stdout,"INFO: Cannot read original sample rate in file: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(sndgetprop(ifd,"arate",(char *)&(inputfile->arate),sizeof(float)) < 0){
            fprintf(stdout,"INFO: Cannot read analysis rate in file: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(sndgetprop(ifd,"analwinlen",(char *)&(inputfile->Mlen),sizeof(int)) < 0){
            fprintf(stdout,"INFO: Cannot read analysis window length in file: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(sndgetprop(ifd,"decfactor",(char *)&(inputfile->Dfac),sizeof(int)) < 0) {
            fprintf(stdout,"INFO: Cannot read decimation factor in file: %s\n",filename);
            fflush(stdout);
            return(DATA_ERROR);
        }
        if(props.origsize != inputfile->origstype) {
            fprintf(stdout,"INFO: snd_headread gives origstype %ld: correct val = %ld: file %s\n",props.origsize,inputfile->origstype,filename);
            fflush(stdout);
        }
        if(props.origrate != inputfile->origrate) {
            fprintf(stdout,"INFO: snd_headread gives origrate %ld: correct val = %ld: file %s\n",props.origrate,inputfile->origrate,filename);
            fflush(stdout);
        }
        if(props.arate != inputfile->arate) {
            fprintf(stdout,"INFO: snd_headread gives arate %f: correct val = %f: file %s\n",props.arate,inputfile->arate,filename);
            fflush(stdout);
        }
        if(props.winlen != inputfile->Mlen) {
            fprintf(stdout,"INFO: snd_headread gives Mlen %d: correct val = %d: file %s\n",props.winlen,inputfile->Mlen,filename);
            fflush(stdout);
        }
        if(props.decfac != inputfile->Dfac) {
            fprintf(stdout,"INFO: snd_headread gives Dfac %d: correct val = %d: file %s\n",props.decfac,inputfile->Dfac,filename);
            fflush(stdout);
        }
        break;
    }
    return(FINISHED);
}
#endif

/***************************************** COPY_TO_INFILEPTR ***********************************/

void copy_to_fileptr(infileptr i, fileptr f)
{
f->filetype     = i->filetype;
f->srate        = i->srate;
f->stype        = i->stype;
f->origstype    = i->origstype;
f->origrate     = i->origrate;
f->channels     = i->channels;
f->origchans    = i->origchans;
f->specenvcnt   = i->specenvcnt;
f->Mlen         = i->Mlen;
f->Dfac         = i->Dfac;
f->arate        = i->arate;
f->window_size  = i->window_size;
}

/***************************************** GETPEAKDATA ***********************************/

//TW get peak val and position
int getpeakdata(int ifd, float *peakval,int *peakpos,SFPROPS props)
{
    CHPEAK peaks[16];
    int chans, peaktime, maxpos = -1, pos_set = 0;
    float maxamp = -2.0f;
    
    chans = props.chans; 
    if(sndreadpeaks(ifd,chans,peaks,&peaktime)){
        int i;
        for(i=0;i < chans;i++) {
            if(peaks[i].value > maxamp) {
                maxamp = peaks[i].value;
                maxpos = peaks[i].position;
                pos_set = 1;
            } else if(smpflteq(peaks[i].value,maxamp)) {
                if(pos_set && (peaks[i].position < (unsigned int)maxpos))
                    maxpos = peaks[i].position;
            }
        }
        if(maxpos < 0) {
            return 0;
        } else {
            *peakval = maxamp;
            *peakpos = maxpos;
        }
        return 1;
    }
    /* sf_headread error; but can't do much about it */
    return 0;
}

