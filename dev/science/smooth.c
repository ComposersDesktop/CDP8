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

/*
 * Take a set of amp-frq data in the correct range,
 * and use linear interpolation to replot curve to cover
 * every 1/64 tone over whole range.
 * Where, frq ratio between adjacent chans becomes < 1/64 tone
 * use chan centre frqs.
 */

#define PSEUDOCENT  (1.0009)    // approx 1/64th of a tone

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <filetype.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <globcon.h>
#include <cdpmain.h>
#include <math.h>
#include <mixxcon.h>
#include <osbind.h>
#include <science.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>

int sloom = 0;
int sloombatch = 0;
const char* cdp_version = "6.1.0";

char errstr[2400];  //RWD need this
//CDP LIB REPLACEMENTS
static int allocate_output_array(double **outarray, int *outcnt, double chwidth, double halfchwidth, double nyquist);
//static int get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static int handle_the_special_data(FILE *fpi, double **parray, int *itemcnt);
static int test_the_special_data(double *parray, int itemcnt, double nyquist);
static int smooth(double *parray,int itemcnt,double *outarray,int *outcnt,double chwidth,double halfchwidth,double nyquist,int arraycnt);
static int dove(double *parray,int itemcnt,double *outarray,int outcnt);
static int read_value_from_array(double *outval,double thisfrq,double *brk,double **brkptr,int brksize,double *firstval,double *lastval,double *lastind,int *brkinit);
static int  get_float_with_e_from_within_string(char **str,double *val);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    FILE *fpi, *fpo;
    int k, OK = 0, dovetail = 0;
    int exit_status, srate, pointcnt, clength;
    double chwidth, halfchwidth, nyquist, *parray, *outarray;
    int itemcnt, outcnt, arraycnt;
    char temp[200], temp2[200];
    if(argc < 5 || argc > 6) {
        usage1();   
        return(FAILED);
    }
    if((fpi = fopen(argv[1],"r")) == NULL) {
        fprintf(stderr,"CANNOT OPEN DATAFILE %s\n",argv[1]);
        return(FAILED);
    }
    if((exit_status = handle_the_special_data(fpi,&parray,&itemcnt))<0)
        return(FAILED);
    arraycnt = itemcnt/2;
    if(sscanf(argv[3],"%d",&pointcnt) != 1) {
        fprintf(stderr,"CANNOT READ POINTCNT (%s)\n",argv[3]);
        return(FAILED);
    }
    OK = 0;
    k = 1;
    while(k < pointcnt) {
        k *= 2;
        if(k == pointcnt){
            OK= 1;
            break;
        }
    }
    if(!OK) {
        fprintf(stderr,"ANALYSIS POINTS PARAMETER MUST BE A POWER OF TWO.\n");
        return(FAILED);
    }
    if(sscanf(argv[4],"%d",&srate) != 1) {
        fprintf(stderr,"CANNOT READ SAMPLE RATE (%s)\n",argv[3]);
        return(FAILED);
    }
    if(srate < 44100 || BAD_SR(srate)) {
        fprintf(stderr,"INVALID SAMPLE RATE ENTERED (44100,48000,88200,96000 only).\n");
        return(DATA_ERROR);
    }
    if(argc == 6) {
        if(strcmp(argv[5],"-s")) {
            fprintf(stderr,"UNKNOWN FINAL PARAMETER (%s).\n",argv[5]);
            return(DATA_ERROR);
        }
        dovetail = 1;
    }
    clength  = (pointcnt + 2)/2;
    nyquist  = (double)(srate / 2);     
    chwidth  = nyquist/(double)clength;
    halfchwidth = chwidth / 2;

    if((exit_status = test_the_special_data(parray,itemcnt,nyquist))<0)
        return(FAILED);
    if((exit_status = allocate_output_array(&outarray,&outcnt,chwidth,halfchwidth,nyquist))<0)
        return(FAILED);
    if((fpo = fopen(argv[2],"w")) == NULL) {
        fprintf(stderr,"CANNOT OPEN OUTPUT DATAFILE %s\n",argv[2]);
        return(FAILED);
    }
    if((exit_status = smooth(parray,itemcnt,outarray,&outcnt,chwidth,halfchwidth,nyquist,arraycnt)) < 0)
        return(FAILED);
    if(dovetail) {
        if((exit_status = dove(parray,itemcnt,outarray,outcnt)) < 0)
            return(FAILED);
    }
    k = 0;
    for(k = 0;k < outcnt;k+=2) {
        sprintf(temp,"%lf",outarray[k]);
        sprintf(temp2,"%lf",outarray[k+1]);
        strcat(temp2,"\n");
        strcat(temp,"\t");
        strcat(temp,temp2);
        fputs(temp,fpo);
    }
    if(fclose(fpo)<0) {
        fprintf(stderr,"WARNING: Failed to close output data file %s.\n",argv[2]);
    }
    return(SUCCEEDED);
}


/******************************** USAGE1 ********************************/

int usage1(void)
{
    fprintf(stderr,
    "USAGE: smooth datafile outdatafile pointcnt srate [-s]\n"
    "\n"
    "Generate smoothed curve from input data re spectrum.\n"
    "\n"
    "DATAFILE     textfile of frq/amp pairs.\n"
    "OUTDATAFILE  textfile of smoothed data.\n"
    "POINTCNT     no of analysis points required in handling data.\n"
    "SRATE        samplerate of eventual sound output.\n"
    "-s           if spectral amp falls to zero before the nyquist,\n"
    "             keep zeroed end of spectrum at zero.\n."
    "\n");
    return(USAGE_ONLY);
}

/************************** HANDLE_THE_SPECIAL_DATA **********************************/

int handle_the_special_data(FILE *fpi, double **parray, int *itemcnt)
{
    int cnt, linecnt;
    double *p, dummy;
    char temp[200], *q;
    cnt = 0;
    p = &dummy;
    while(fgets(temp,200,fpi)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_with_e_from_within_string(&q,p))
            cnt++;
    }
    if(ODD(cnt)) {
        fprintf(stderr,"Data not paired correctly in input data file\n");
        return(DATA_ERROR);
    }
    if(cnt == 0) {
        fprintf(stderr,"No data in input data file\n");
        return(DATA_ERROR);
    }
    if((*parray = (double *)malloc(cnt * sizeof(double)))==NULL) {
        fprintf(stderr,"INSUFFICIENT MEMORY for input data.\n");
        return(MEMORY_ERROR);
    }
    fseek(fpi,0,0);
    linecnt = 1;
    p = *parray;
    while(fgets(temp,200,fpi)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_with_e_from_within_string(&q,p)) {
            p++;
        }
        linecnt++;
    }
    if(fclose(fpi)<0) {
        fprintf(stderr,"WARNING: Failed to close input data file.\n");
    }
    *itemcnt = cnt;
    return(FINISHED);
}

/************************** TEST_THE_SPECIAL_DATA **********************************/

int test_the_special_data(double *parray, int itemcnt, double nyquist)
{
    double *p;
    int isamp = 0, linecnt = 1;
    p = parray;
    while(linecnt <= itemcnt/2) {
        if(isamp) {
            if(*p < 0.0 || *p > 1.0) {
                fprintf(stderr,"Amp (%lf) out of range (0-1) at line %d in datafile.\n",*p,linecnt);
                return(DATA_ERROR);
            }
        } else {
            if(*p < 0.0 || *p > nyquist) {
                fprintf(stderr,"Frq (%lf) out of range (0 - %lf) at line %d in datafile.\n",*p,nyquist,linecnt);
                return(DATA_ERROR);
            }
        }
        if(!isamp)
            linecnt++;
        isamp = !isamp;
        p++;
    }
    return(FINISHED);
}

/************************** ALLOCATE_OUTPUT_ARRAY **********************************/

int allocate_output_array(double **outarray,int *outcnt,double chwidth,double halfchwidth,double nyquist)
{
    int outsize = 1;  // Count initial halfwidht channel
    double lastlofrq, lofrq = halfchwidth;
    while(lofrq <= nyquist) {
        outsize++;
        lastlofrq = lofrq;  
        lofrq *= PSEUDOCENT;
        if(lofrq - lastlofrq >= chwidth)    //  Once chan separation is > chwidth
            lofrq = lastlofrq + chwidth;    //  Just set 1 value per channel
    }
    outsize += 64;  //  SAFETY
    outsize *= 2;
    if((*outarray = (double *)malloc(outsize * sizeof(double)))==NULL) {
        fprintf(stderr,"INSUFFICIENT MEMORY for input data.\n");
        return(MEMORY_ERROR);
    }
    *outcnt = outsize;
    return( FINISHED);
}

/************************** SMOOTH **********************************/

int smooth(double *parray,int itemcnt,double *outarray,int *outcnt,double chwidth,double halfchwidth,double nyquist,int arraycnt)
{
    int exit_status;
    int k = 0, hi = 0;
    double lastlofrq, lofrq = halfchwidth, thisamp;
    double *brkptr = parray;
    int brksize = itemcnt/2;
    double firstval,lastval,lastind;
    int brkinit = 0;
    outarray[k] = 0.0;
    outarray[k+1] = 0.0;
    k = 2;
    while(lofrq < nyquist) {
        if(k >= *outcnt) {
            fprintf(stderr,"OUT OF MEMORY IN OUTPUT ARRAY.\n");
            return(MEMORY_ERROR);
        }
        if((exit_status = read_value_from_array(&thisamp,lofrq,parray,&brkptr,brksize,&firstval,&lastval,&lastind,&brkinit))<0)
            return(exit_status);
        outarray[k] = lofrq;
        outarray[k+1] = thisamp;
        lastlofrq = lofrq;  
        lofrq *= PSEUDOCENT;
        if(lofrq - lastlofrq >= chwidth)    //  Once chan separation is < PSEUDOCENT
            lofrq = lastlofrq + chwidth;    //  Just set 1 value per channel
        k += 2;
    }
    outarray[k++] = nyquist;
    outarray[k++] = 0.0;
    *outcnt = k;
    if(outarray[0] == 0.0 && outarray[2] == 0.0) {      // Eliminate any duplication of zero start frq
        k = 0;
        hi = 2;
        while(hi < *outcnt) {
            outarray[k] = outarray[hi];
            k++;
            hi++;
        }
        *outcnt = k;
    }   
    return(FINISHED);
}

/************************** DOVE **********************************/

int dove(double *parray,int itemcnt,double *outarray,int outcnt)
{
    double *amp, *frq, *inarrayend = parray + itemcnt, *outarrayend = outarray + outcnt, tailstartfrq;
    amp = inarrayend - 1;
    if(!flteq(*amp,0.0))        // Input spectrum never falls to zero
        return(FINISHED);

    while(amp > parray) {       //  Search back from end of (orig) spectrum, until spectral vals are >0
        if(!flteq(*amp,0.0))
            break;
        amp -= 2;
    }
    if(amp <= parray)           //  (original) spectrum is everywhere zero
        return(FINISHED);
    frq = amp - 1;
    tailstartfrq = *frq;        //  Note frq at which (original) spectrum finally falls to zero

    frq = outarray;             //  Parse frq vals in output array
    while(frq < outarrayend) {
        if(*frq >= tailstartfrq) {
            amp = frq - 1;      //  Note where outfrq exceeds tailstartfrq of input, and step to amp BEFORE it
            break;
        }
        frq += 2;
    }
    if(frq >= outarrayend) {    //  Output frq never exceeds input frq: should be impossible
        fprintf(stderr,"DOVETAILING FAILED.\n");
        return(PROGRAM_ERROR);
    }
    while(amp < outarrayend) {  //  Search for point where output first touches zero
        if(flteq(*amp,0.0))
            break;
        amp += 2;
    }
    if(amp >= outarrayend) {
        fprintf(stderr,"NO DOVETAILING OCCURED.\n");
    }
    while(amp < outarrayend) {  //  Set tail to zero
        *amp = 0.0;
        amp += 2;
    }
    return(FINISHED);
}

/**************************** READ_VALUE_FROM_ARRAY *****************************/

int read_value_from_array(double *outval,double thisfrq,double *brk,double **brkptr,int brksize,double *firstval,double *lastval,double *lastind,int *brkinit)
{
    double *endpair, *p, val;
    double hival, loval, hiind, loind;
    if(!(*brkinit)) {
        *brkptr   = brk;
        *firstval = *(brk+1);
        endpair   = brk + ((brksize-1)*2);
        *lastind  = *endpair;
        *lastval  = *(endpair+1);
        *brkinit = 1;
    }
    p = *brkptr;
    if(thisfrq <= *brk) {
        *outval = *firstval;
        return(FINISHED);
    } else if(thisfrq >= *lastind) {
        *outval = *lastval;
        return(FINISHED);
    } 
    if(thisfrq > *p) {
        while(*p < thisfrq)
            p += 2;
    } else {
        while(*p >=thisfrq)
            p -= 2;
        p += 2;
    }
    hival  = *(p+1);
    hiind  = *p;
    loval  = *(p-1);
    loind  = *(p-2);
    val    = (thisfrq - loind)/(hiind - loind);
    val   *= (hival - loval);
    val   += loval;
    *outval = val;
    *brkptr = p;
    return(FINISHED);
}

/************************** GET_FLOAT_WITH_E_FROM_WITHIN_STRING **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

int  get_float_with_e_from_within_string(char **str,double *val)
{
    char   *p, *valstart;
    int    decimal_point_cnt = 0, has_digits = 0, has_e = 0, lastchar = 0;
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
        else if(*p == 'e') {
            if(has_e || !has_digits)
                return(FALSE);
            has_e = 1;
        } else if(*p == '-') {
            if(!has_e || (lastchar != 'e'))
                return(FALSE);
        } else if(*p == '.') {
            if(has_e || (++decimal_point_cnt>1))
                return(FALSE);
        } else
            return(FALSE);
        lastchar = *p;
        p++;
    }
    if(!has_digits || sscanf(valstart,"%lf",val)!=1)
        return(FALSE);
    *str = p;
    return(TRUE);
}

