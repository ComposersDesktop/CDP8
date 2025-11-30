/*
 * Copyright (c) 1983-2023 Trevor Wishart and Composers Desktop Project Ltd
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
#include <structures.h>
#include <tkglobals.h>
#include <cdpmain.h>
#include <globcon.h>
#include <processno.h>
#include <grain.h>
#include <arrays.h>
#include <sfsys.h>

//#ifdef unix 
#define round(x) lround((x))
//#endif

static int  get_grain_envelope(dataptr dz);
static int  convert_arraytimelist_to_abs_samplecnts(double *thisarray,int thissize,dataptr dz);
static int  convert_gate_time_to_abs_samplecnts(int paramno,dataptr dz);
static int  insert_limit_vals(int gate_paramno,dataptr dz);
static void do_arbitrary_param_presets(dataptr dz);
static void convert_sampletime_to_samplegap(dataptr dz);
static int  make_grain_splicetable(dataptr dz);
static void offset_synctimes(dataptr dz);
static int  readenv(int samps_to_process,int winsamps,float *maxsamp,double **winptr,dataptr dz);
static double getmaxsamp(int startsamp,float *maxsamp, int winsamps,dataptr dz);
static int  normalise_env(dataptr dz);
static int  seek_to_start_and_zero_all_counters(dataptr dz);
static void create_envsteps(dataptr dz);

/**************************** GRAIN_PREPROCESS ******************************/

int grain_preprocess(int gate_paramno,dataptr dz)
{
    int exit_status;
    int chans = dz->infile->channels;
    /* setup params for reading gate-vals */
    int splice_samplen;
    dz->ptr[GR_GATEVALS] = NULL;
    dz->itemcnt = 0;
    if(dz->iparam[GR_WSIZE_SAMPS]>0) {
        fprintf(stdout,"INFO: Extracting amplitude envelope of source.\n");
        fflush(stdout);
        if((exit_status = get_grain_envelope(dz))<0)
            return(exit_status);
        fprintf(stdout,"INFO: Processing grains.\n");
        fflush(stdout);
    }
    if(dz->brksize[gate_paramno]) {
        if((exit_status = convert_gate_time_to_abs_samplecnts(gate_paramno,dz))<0)
            return(exit_status);
        if((exit_status = insert_limit_vals(gate_paramno,dz))<0)     /* CARE - ONLY WORKS FOR FILE0 */
            return(exit_status);
        dz->brkptr[gate_paramno]   = dz->brk[gate_paramno];
        dz->ptr[GR_GATEVALS]       = dz->brk[gate_paramno];
        dz->iparam[GR_THISBRKSAMP] = round(*(dz->ptr[GR_GATEVALS]++));
        dz->param[GR_GATE]         = *(dz->ptr[GR_GATEVALS]++);
        dz->iparam[GR_NEXTBRKSAMP] = round(*(dz->ptr[GR_GATEVALS]++));
        dz->param[GR_NEXTGATE]     = *(dz->ptr[GR_GATEVALS]++);
        dz->param[GR_GATESTEP]     = (dz->param[GR_NEXTGATE] - dz->param[GR_GATE])
                                     /(double)(dz->iparam[GR_NEXTBRKSAMP]-dz->iparam[GR_THISBRKSAMP]);
        if(dz->param[GR_GATESTEP]>0.0)
        dz->iparam[GR_UP] = TRUE;
        else
            dz->iparam[GR_UP] = FALSE;
        if((dz->iparam[GR_TESTLIM] = dz->iparam[GR_NEXTBRKSAMP] - 1000)<dz->iparam[GR_THISBRKSAMP])
            dz->iparam[GR_TESTLIM] = dz->iparam[GR_NEXTBRKSAMP];
        dz->ptr[GR_GATEBRKEND] = dz->brk[gate_paramno] + (dz->brksize[gate_paramno] * 2);
        dz->brksize[gate_paramno] = 0; /* Comment: prevents programs RE-READING brktable (using TIME as index)
                                    where it calls "read_values_from_all_existing_brktables()" */
    } else
        do_arbitrary_param_presets(dz); /* otherwise program thinks param not assigned, and objects */

    dz->param[GR_NGATE] = -dz->param[GR_GATE];

    /* setup splicing params */
    dz->iparam[GR_SPLICELEN]     = round(GRAIN_SPLICELEN * MS_TO_SECS * (double)dz->infile->srate);
    if(ODD(dz->iparam[GR_SPLICELEN]))
        dz->iparam[GR_SPLICELEN]++; /* Needs to be even as we use HALF of it */
    dz->param[GR_SPLUS1]         = (double)dz->iparam[GR_SPLICELEN] + 1.0;
    dz->iparam[GR_ABS_SPLICELEN] = dz->iparam[GR_SPLICELEN] * chans;
    dz->iparam[GR_ABS_SPLICEX2]  = dz->iparam[GR_ABS_SPLICELEN] * 2;
    dz->iparam[GR_MINHOLE]       = (int)(round(dz->param[GR_MINTIME] * (double)dz->infile->srate) * chans);
    splice_samplen = (dz->iparam[GR_SPLICELEN]/2) * chans;
    if(dz->process!=GRAIN_GET && dz->process!=GRAIN_COUNT && dz->process!=GRAIN_ASSESS) {
        if((dz->extrabuf[0] = (float *)malloc(splice_samplen * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for splice buffer.\n");
            return(MEMORY_ERROR);               /* setup 1st splicebuf */
        }
        if((dz->extrabuf[1] = (float *)malloc(splice_samplen * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for 2nd splice buffer.");
            return(MEMORY_ERROR);               /* setup 2nd splicebuf */
        }
        if((exit_status = make_grain_splicetable(dz))<0)
            return(exit_status);
    }
    switch(dz->process) {
    case(GRAIN_POSITION):           /* Check offset value, converting to abs_samplecnt */
        dz->param[GR_OFFSET] = (double)(round(dz->param[GR_OFFSET] *(double)dz->infile->srate) * chans);
        if((exit_status = convert_arraytimelist_to_abs_samplecnts
        (dz->parray[GR_SYNCTIME],(int)dz->iparam[GR_SYNCCNT],dz))<0)
            return(exit_status);
        if(dz->param[GR_OFFSET] > 0.0)
            offset_synctimes(dz);               /* Add offset */
        convert_sampletime_to_samplegap(dz);    /* Convert abs_grain_samptimes to gaps between grains */
        if((dz->extrabuf[2] = (float *)malloc(dz->buflen * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for sound copying array.\n");
            return(MEMORY_ERROR);               /* setup buffer for snd copying */
        }
        break;
    case(GRAIN_REVERSE):            /* Establish array to store abs_grain_positions */
        dz->iparam[GR_ARRAYSIZE] = BIGARRAY;
        if((dz->lparray[GR_ABS_POS] = (int *)malloc(dz->iparam[GR_ARRAYSIZE] * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for grain times array.\n");
            return(MEMORY_ERROR);
        }
        break;
    case(GRAIN_REPITCH):
    case(GRAIN_REMOTIF):
        dz->iparam[GR_STORESIZE] = NOMINAL_LENGTH;
        if((dz->extrabuf[2] = (float *)malloc(dz->iparam[GR_STORESIZE] * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for sound copying buffer.\n");
            return(MEMORY_ERROR);               /* setup buffer for snd copying */
        }
    }
    return(FINISHED);
}

/**************************** OFFSET_SYNCTIMES ***************************/

void offset_synctimes(dataptr dz)
{
    int n;
    for(n=0;n<dz->iparam[GR_SYNCCNT];n++)
        dz->parray[GR_SYNCTIME][n] = (double)round(dz->parray[GR_SYNCTIME][n] + dz->param[GR_OFFSET]);
}

/******************************** GET_GRAIN_ENVELOPE *****************************/

int get_grain_envelope(dataptr dz)
{
    int exit_status;
    int n, bufcnt;
    int winsamps = dz->iparam[GR_WSIZE_SAMPS];
    double *winptr;
    float maxsamp = 0;
    if(((bufcnt = dz->insams[0]/dz->buflen)*dz->buflen)!=dz->insams[0])
        bufcnt++;   /* Find number of buffers contained in file. */ 
    if(((dz->iparam[GR_WINCNT] = 
    dz->insams[0]/dz->iparam[GR_WSIZE_SAMPS]) 
            * dz->iparam[GR_WSIZE_SAMPS])!=dz->insams[0])
        dz->iparam[GR_WINCNT]++;    /* Find number of windows contained in file. */
    if((dz->parray[GR_ENVEL]=(double *)malloc(dz->iparam[GR_WINCNT] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for envelope store.\n");
        return(MEMORY_ERROR);
    }
    dz->ptr[GR_ENVEND] = dz->parray[GR_ENVEL] + dz->iparam[GR_WINCNT];
    if((dz->parray[GR_ENVSTEP]=(double *)malloc(dz->iparam[GR_WINCNT] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for envelope step store.\n");
        return(MEMORY_ERROR);
    }
    dz->ptr[GR_ESTEPEND] = dz->parray[GR_ENVSTEP] + dz->iparam[GR_WINCNT];
    winptr = dz->parray[GR_ENVEL];

    if(sloom) {
        fprintf(stdout,"INFO: Extracting sound envelope.\n");
        fflush(stdout);
    }

    for(n = 0; n < bufcnt; n++) {
        if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
            return(exit_status);
        if(sloom)
            display_virtual_time(dz->total_samps_read,dz);
        if((exit_status = readenv(dz->ssampsread,winsamps,&maxsamp,&winptr,dz))<0)
            return(exit_status);
    }
    if((exit_status = normalise_env(dz))<0)
        return(exit_status);
    create_envsteps(dz);

    if(sloom) {
        fprintf(stdout,"INFO: Proceeding to grain extraction.\n");
        fflush(stdout);
    }
    return seek_to_start_and_zero_all_counters(dz);
}

/******************************* CONVERT_ARRAYTIMELIST_TO_ABS_SAMPLECNTS *******************************/

int convert_arraytimelist_to_abs_samplecnts(double *thisarray,int thissize,dataptr dz)
{
    double *p, *pend;
    if(thisarray==NULL) {
        sprintf(errstr,"Array not initialised in convert_arraytimelist_to_abs_samplecnts()\n");
        return(PROGRAM_ERROR);
    }
    if(thissize <= 0) {
        sprintf(errstr,"Array size invalid (<=0) in convert_arraytimelist_to_abs_samplecnts()\n");
        return(PROGRAM_ERROR);
    }
    p = thisarray;
    pend = thisarray + thissize;
    while(p < pend) {
        *p = (double)(round(*p * (double)(dz->infile->srate)) * dz->infile->channels);
        p++;
    }
    return(FINISHED);
}

/***************************** CONVERT_GATE_TIME_TO_ABS_SAMPLECNTS *******************************/

int convert_gate_time_to_abs_samplecnts(int paramno,dataptr dz)
{
    double *p, *pend;
    double lasttime = 0.0;
    if(dz->brksize==NULL) {
        sprintf(errstr,"brksize not initialised:convert_gate_time_to_abs_samplecnts()\n");
        return(PROGRAM_ERROR);
    } 
    if(dz->brksize[paramno]) {
        if(dz->brk==NULL) {
            sprintf(errstr,"brk not initialised:convert_gate_time_to_abs_samplecnts()\n");
            return(PROGRAM_ERROR);
        } 
        p    = dz->brk[paramno];
        pend = dz->brk[paramno] + (dz->brksize[paramno] * 2);
        while(p < pend) {
            *p = (double)(round(*p * (double)dz->infile->srate) * dz->infile->channels);
            if(p == dz->brk[paramno]) {
                if(*p < 0.0) {
                    sprintf(errstr,"Subzero time encountered in brkpnt file for gate.\n");
                    return(DATA_ERROR);
                }
                lasttime = *p;
            } else if(*p <= lasttime) {
                sprintf(errstr,"Times, when converted to sample-cnts, don't advance in brkfile\n");
                return(DATA_ERROR);
            }
            lasttime = *p;
            p += 2;
        }
    } else
        dz->param[paramno] = (double)(round(dz->param[paramno] * (double)dz->infile->srate) * dz->infile->channels);
    return(FINISHED);
}

/************************** INSERT_LIMIT_VALS ***************************/

int insert_limit_vals(int gate_paramno,dataptr dz)   /* CARE: ASSUMES WE'RE DEALING WITH FILE 0 */
{
    double timediff, truediff, ratio, valdiff;
    double *q, *p = dz->brk[gate_paramno];
    if(*p != 0.0) {                     /* IF NO VALUE AT ZEROTIME */
        dz->brksize[gate_paramno]++;
        if((dz->brk[gate_paramno] = (double *)realloc
        ((char *)dz->brk[gate_paramno],dz->brksize[gate_paramno] * 2 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for array limit values.\n");
            return(MEMORY_ERROR);       /* create any extra pair-location */
        }
        p = dz->brk[gate_paramno] + (dz->brksize[gate_paramno] * 2) - 1;
        q = p - 2;
        while(q >= dz->brk[gate_paramno]) { /* shuffle values up by 2 places */
            *p = *q;
            p--;
            q--;
        }
        p = dz->brk[gate_paramno]+1;        /* copy post-zerotime value into zero-time value */
        q = p + 2;
        *p-- = *q;
        *p = 0.0;                           /* insert zero-time at start */
    }
    p = dz->brk[gate_paramno] + (dz->brksize[gate_paramno] * 2) - 2;
    if(*p < dz->insams[0]) {            /* IF VALUES DON'T REACH ENDOFFILE-TIME */
        dz->brksize[gate_paramno]++;
        if((dz->brk[gate_paramno] = (double *)realloc
        ((char *)dz->brk[gate_paramno],dz->brksize[gate_paramno] * 2 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for array limit values.\n");
            return(MEMORY_ERROR);       /* create any extra pair-location */
        }
        p = dz->brk[gate_paramno] + (dz->brksize[gate_paramno] * 2) - 1;
        q = p - 2;                      /* copy endtime value into endoffile-val location */
        *p-- = *q;
        *p = dz->insams[0] + 2;         /* Put endoffile time in endoffile-time location (+2 for safety) */
    } else if (*p > dz->insams[0]) {    /* IF VALUES REACH BEYOND ENDOFFILE-TIME */
        while(*p > dz->insams[0]) {
            p -= 2;                     /* step back to value AT or before endoffile-time */
            dz->brksize[gate_paramno]--;        /* adjusting brktable size */
        }
        if(*p < dz->insams[0]) {        /* If value now reached is before endoffile-time */
            dz->brksize[gate_paramno]++;        /* readjust brktable size upwards */
            q = p + 2;                  /* create an interpolated value for endoffile-time */
            timediff = *q - *p;
            truediff = dz->insams[0] - *p;
            ratio = truediff/timediff;
            *q = (double)dz->insams[0];
            p++;
            q++;
            valdiff = *q - *p;
            valdiff *= ratio;
            *q = *p + valdiff;
        }                                /* and finally, readjust the brktable-space to its true size */
        if((dz->brk[gate_paramno] = (double *)realloc
        ((char *)dz->brk[gate_paramno],dz->brksize[gate_paramno] * 2 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to reallocate values.\n");
            return(MEMORY_ERROR);
        }
    }
    return(FINISHED);
}

/************************** DO_ARBITRARY_PARAM_PRESETS ***************************/

void do_arbitrary_param_presets(dataptr dz)
{
    dz->iparam[GR_THISBRKSAMP] = 0;
    dz->iparam[GR_NEXTBRKSAMP] = 0;
    dz->param[GR_NEXTGATE]     = 0;
    dz->param[GR_GATESTEP]     = 0.0;
    dz->iparam[GR_UP]          = TRUE;
    dz->iparam[GR_TESTLIM]     = 0;
}

/************************ CONVERT_SAMPLETIME_TO_SAMPLEGAP ***************************
 *
 * Store sample count BETWEEN grains in place of times OF grains, and check values!!
 */

void convert_sampletime_to_samplegap(dataptr dz)
{
    int n, timecnt = 0,bumcnt = 0, grain_moved = 0;
    int lval, lastlval = round(dz->parray[GR_SYNCTIME][0]);
    int mingap = dz->iparam[GR_ABS_SPLICELEN] * 2;
    int minstartgap = dz->iparam[GR_ABS_SPLICELEN];
    for(n=1;n<dz->iparam[GR_SYNCCNT];n++) {
        lval = round(dz->parray[GR_SYNCTIME][n]);
        dz->parray[GR_SYNCTIME][n] = (double)(lval - lastlval);
        lastlval = lval;
    }
    if((lval = minstartgap - round(dz->parray[GR_SYNCTIME][0]))>0) {
        if(dz->iparam[GR_SYNCCNT]>1
        && (round(dz->parray[GR_SYNCTIME][1]) - lval >= mingap)) {
            dz->parray[GR_SYNCTIME][1]   -= (double)lval;
            dz->parray[GR_SYNCTIME][0]   += (double)lval;
            fprintf(stdout,"WARNING: 1st grain moved by %lf secs (%d samps) to allow for startsplice\n",
            (double)(lval/dz->infile->channels)/(double)dz->infile->srate,lval);
            fflush(stdout);
        } else {
            dz->parray[GR_SYNCTIME][0] = (double)dz->iparam[GR_ABS_SPLICELEN];
            fprintf(stdout,"WARNING: All grain times offset by %lf secs (%d samps) to allow for startsplice\n",
            (double)(lval/dz->infile->channels)/(double)dz->infile->srate,lval);
            fflush(stdout);
        }
    }
    for(n=1;n<dz->iparam[GR_SYNCCNT];n++) {
        timecnt++;
        if((lval = mingap - round(dz->parray[GR_SYNCTIME][n]))>0) {
            bumcnt++;
            if(n>1 
            && (round(dz->parray[GR_SYNCTIME][n-1]) - lval >= mingap)) {
                dz->parray[GR_SYNCTIME][n-1] -= (double)lval;
                dz->parray[GR_SYNCTIME][n]   += (double)lval;
                grain_moved++;
            } else if (n < dz->iparam[GR_SYNCCNT]-1
            && (round(dz->parray[GR_SYNCTIME][n+1]) - lval >= mingap)) {
                dz->parray[GR_SYNCTIME][n+1] -= (double)lval;
                dz->parray[GR_SYNCTIME][n]   += (double)lval;
                grain_moved++;
            } else {
                dz->parray[GR_SYNCTIME][n] = (double)(dz->iparam[GR_ABS_SPLICELEN] * 2);
                bumcnt++;
            }
        }
    }
    if(bumcnt) {
        if(bumcnt >= timecnt/2)
            fprintf(stdout,
            "WARNING: so many graintimes altered to minimum splicelen that splicetime-frq may produce a tone.\n");
        else
            fprintf(stdout,"WARNING: (some) set of graintimes shifted to allow for splicetime.\n");
    } else if(grain_moved)
        fprintf(stdout,"WARNING: %d grain-starttimes moved to allow for splicetime.\n",grain_moved);
    fflush(stdout);
}

/*************************** MAKE_GRAIN_SPLICETABLE ***************************/

int make_grain_splicetable(dataptr dz)
{
    int n;
    int splicelen = dz->iparam[GR_SPLICELEN]/2;
    if((dz->parray[GR_SPLICETAB] = (double *)malloc(splicelen * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for splice table.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<splicelen;n++)
        dz->parray[GR_SPLICETAB][n] = (double)n/(double)splicelen;
    return FINISHED;
}

/************************* READENV ******************************
 *
 * Extract envelope values from a buffer of samples.
 */

int readenv(int samps_to_process,int winsamps,float *maxsamp,double **winptr,dataptr dz)
{
    int  startsamp = 0;
    double *env = *winptr;
    while(samps_to_process >= winsamps) {
        *env = getmaxsamp(startsamp,maxsamp,winsamps,dz);
        if(++env > dz->ptr[GR_ENVEND]) {
            sprintf(errstr,"Array overflow in readenv(): 1\n");
            return(PROGRAM_ERROR);
        }
        startsamp += winsamps;
        samps_to_process -= winsamps;
    }
    if(samps_to_process) {  /* Handle any final short buffer */
        *env = getmaxsamp(startsamp,maxsamp,winsamps,dz);
        if(++env > dz->ptr[GR_ENVEND]) {
            sprintf(errstr,"Array overflow in readenv(): 2\n");
            return(PROGRAM_ERROR);
        }
    }
    *winptr = env;
    return(FINISHED);
}

/*************************** GETMAXSAMP ******************************
 *
 * Find largest sample over window.
 *
 * NB We assume that in a stereo file,the largest sample in EITHER channel
 *    represents the max amplitude.
 */

double getmaxsamp(int startsamp,float *maxsamp, int winsamps,dataptr dz)
{
    int  i, endsamp = startsamp + winsamps;
    /*int*/float thismaxsamp = 0.0;
    /*int*/double val;
    for(i = startsamp; i<endsamp; i++) {
        val = fabs(dz->sampbuf[0][i]);
        thismaxsamp = (float) max(thismaxsamp,val);
    }
    if(thismaxsamp > *maxsamp)
        *maxsamp = thismaxsamp;
    return((double)thismaxsamp);
}

/**************************** NORMALISE_ENV *************************/

int normalise_env(dataptr dz)
{
    int n;
    double *d = dz->parray[GR_ENVEL];
    double scaler;
    double maxsamp = 0.0;
    for(n=0;n<dz->iparam[GR_WINCNT];n++) {
        maxsamp = max(maxsamp,*d);
        d++;
    }
    if(maxsamp==0.0) {
        sprintf(errstr,"Zero level in input file.\n");
        return(DATA_ERROR);
    }
    scaler = 1.0/(double)maxsamp;
    d = dz->parray[GR_ENVEL];
    for(n=0;n<dz->iparam[GR_WINCNT];n++) {
        *d *= scaler;
        d++;
    }
    return(FINISHED);
}

/*************************** SEEK_TO_START_AND_ZERO_ALL_COUNTERS *************************/

int seek_to_start_and_zero_all_counters(dataptr dz)
{
    if(sndseekEx(dz->ifd[0],0,0)<0) {
        sprintf(errstr,"seek failed: seek_to_start_and_zero_all_counters()\n");
        return(SYSTEM_ERROR);
    }
    dz->total_samps_read = 0;
    dz->samps_left = dz->insams[0];
    return(FINISHED);
}

/**************************** CREATE_ENVSTEPS *************************/

void create_envsteps(dataptr dz)
{
    int n;
    double sampstep = (double)(dz->iparam[GR_WSIZE_SAMPS]/dz->infile->channels);
    for(n=0;n<dz->iparam[GR_WINCNT]-1;n++)
        dz->parray[GR_ENVSTEP][n] = 
        (dz->parray[GR_ENVEL][n+1] - dz->parray[GR_ENVEL][n])/(double)sampstep;
    dz->parray[GR_ENVSTEP][n] = 0.0;
}
