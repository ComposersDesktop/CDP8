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



/* PEDITAUD
 *
 * Hear a pitchfile being edited in the PDISPLAY routine in SoundLoom.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osbind.h>
#include <math.h>
#include <float.h>
#include <float.h>
#include <sfsys.h>
#include <cdplib.h>
#include <pvoc.h>
#include <paudition.h>

char errstr[2400];

#define NOT_PITCH (-1.0)

#define ENDOFSTR                ('\0')
#define ANALFILE_OUT            (1)
#define SNDFILE_OUT             (0)
#define WINDOW_AMP              (0.5)
#define PARTIALS_IN_TEST_TONE   (6)
#define PARTIAL_DECIMATION      (.25)
#define VERY_TINY_AMP           (0.00000000000000000001)    /* 10^-20 */
#define FLTERR                  (0.000002)

static int headwrite(int ofd,int origstype,int origrate,float arate,int Mlen,int Dfac);
static int get_data_from_pitchfile(float **pitches,int infilesize,int ifd);
static int setup_amplitudes(int clength,double halfchwidth,double chwidth,float **testpamp,float **totpamp,float **chbot);
static void do_finish(int n,int ifd,int ofd,float *bigbuf) ;
static int make_buffer(float **bigbuf,int *big_fsize, int wanted);
static int gen_testdata(int ofd,float *bigbuf,float *bigbufend,int big_fsize,
    int wlength,int clength,int wanted,int *total_samps_written,float *pitches,
    double chwidth,double halfchwidth,float *chbot,double nois_chanamp,float *testpamp,float *totpamp,
    double nyquist,double pre_totalamp);
static void gen_silence_in_sampbuf(float *sampbuf,double chwidth,double halfchwidth,int clength,
    float *chbot,double nois_chanamp);
static void gen_nois_in_sampbuf(float *sampbuf,double chwidth,double halfchwidth,int clength,
    float *chbot,double nois_chanamp);
static void gen_tone_in_sampbuf(double thisfrq,float *sampbuf,float *testpamp,float *totpamp,
    double nyquist,double pre_totalamp,double chwidth,double halfchwidth,int wanted);
static int pvoc_process_addon(int ifd,int ofd,int chans,int origrate,float arate,int Mlen,int Dfac);
static void hamming(float *win,int winLen,int even);
static int pvoc_float_array(int nnn,float **ptr);
/*static int lcm(int a, int b);*/
static void preset_sampbuf(float *sampbuf,double halfchwidth, int clength, float *chbot);
static void normalise(double post_totalamp,double pre_totalamp,int wanted,float *sampbuf);
static int flteq(double f1,double f2);
static int outfloats(float *nextOut,float *maxsample,float *minsample,int *num_overflows,int todo, int ofd);

#define MONO (1)
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
    float *pitches, *testpamp, *totpamp, *chbot;
    float *bigbuf = NULL, *bigbufend;
    int big_fsize, total_samps_written = 0;
    int ofd = -1, ifd;
    int wlength;
    int origchans, origstype, origrate;
    float arate;
    int Mlen, Dfac;
    int srate, chans;
    double  chwidth, halfchwidth, nyquist, nois_chanamp, gain = 1.0, pre_totalamp;
    int   wanted, clength;

    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if(argc != 12) {
        fprintf(stdout,"ERROR: Incorrect call to program which writes the sound.\n");
        fflush(stdout);
        return 1;
    }
    /* initialise SFSYS */
    if( sflinit("paudition") < 0 ) {
        fprintf(stdout,"ERROR: Cannot initialise soundfile system.\n");
        fflush(stdout);
        return 1;
    }

    /* open input file */
    if((ifd = sndopenEx(argv[1],0,CDP_OPEN_RDONLY))<0)  {
        fprintf(stdout,"INFO: Cannot open intermediate pitch data file: %s\n",argv[1]);
        fflush(stdout);
        do_finish(0,ifd,ofd,bigbuf);
    }
    wlength = sndsizeEx(ifd);

    if(sscanf(argv[4],"%d",&origchans)!=1) {
        fprintf(stdout,"ERROR: Cannot read original-channels data,\n");
        fflush(stdout);
        do_finish(1,ifd,ofd,bigbuf);
        return 1;
    }
    if(sscanf(argv[5],"%d",&origstype)!=1) {
        fprintf(stdout,"ERROR: Cannot read original-sample-type data,\n");
        fflush(stdout);
        do_finish(1,ifd,ofd,bigbuf);
        return 1;
    }
    if(sscanf(argv[6],"%d",&origrate)!=1) {
        fprintf(stdout,"ERROR: Cannot read original-sample-rate data,\n");
        fflush(stdout);
        do_finish(1,ifd,ofd,bigbuf);
        return 1;
    }
    if(sscanf(argv[7],"%f",&arate)!=1) {
        fprintf(stdout,"ERROR: Cannot read analysis-rate data,\n");
        fflush(stdout);
        do_finish(1,ifd,ofd,bigbuf);
        return 1;
    }
    if(sscanf(argv[8],"%d",&Mlen)!=1) {
        fprintf(stdout,"ERROR: Cannot read Mlen data,\n");
        fflush(stdout);
        do_finish(1,ifd,ofd,bigbuf);
        return 1;
    }
    if(sscanf(argv[9],"%d",&Dfac)!=1) {
        fprintf(stdout,"ERROR: Cannot read Decimation Factor data,\n");
        fflush(stdout);
        do_finish(1,ifd,ofd,bigbuf);
        return 1;
    }
    if(sscanf(argv[10],"%d",&srate)!=1) {
        fprintf(stdout,"ERROR: Cannot read sample-rate data,\n");
        fflush(stdout);
        do_finish(1,ifd,ofd,bigbuf);
        return 1;
    }
    if(sscanf(argv[11],"%d",&chans)!=1) {
        fprintf(stdout,"ERROR: Cannot read channel data,\n");
        fflush(stdout);
        do_finish(1,ifd,ofd,bigbuf);
        return 1;
    }
//TW COMMENT: this is temporary analysis file

    if((ofd = sndcreat_formatted(argv[2],-1,SAMP_FLOAT,
            chans,srate,CDP_CREATE_NORMAL)) < 0) {
        fprintf(stdout,"ERROR: Can't create file %s (It may already exist)\n", argv[2]);
        fflush(stdout);
        do_finish(1,ifd,ofd,bigbuf);
        return 1;
    }

    wanted  = origchans ;       /* floatsams to read */
    clength = wanted / 2;       /* channels to process */
    nyquist     = (double)origrate/2.0;
    chwidth     = nyquist/(double)(clength - 1);
    halfchwidth = chwidth/2.0;

    if(!get_data_from_pitchfile(&pitches,wlength,ifd)) {
        do_finish(2,ifd,ofd,bigbuf);
        return 1;
    }

    sndcloseEx(ifd);

    pre_totalamp = WINDOW_AMP * gain;
    nois_chanamp = pre_totalamp/(double)clength;
    if(!setup_amplitudes(clength,halfchwidth,chwidth,&testpamp,&totpamp,&chbot)) {
        do_finish(3,ifd,ofd,bigbuf);
        return 1;
    }

    if(!make_buffer(&bigbuf,&big_fsize,wanted)) {
        do_finish(3,ifd,ofd,bigbuf);
        return 1;
    }
    bigbufend = bigbuf + big_fsize;

    if(!gen_testdata(ofd,bigbuf,bigbufend,big_fsize,wlength,clength,wanted,&total_samps_written,
        pitches,chwidth,halfchwidth,chbot,nois_chanamp,testpamp,totpamp,nyquist,pre_totalamp)) {
        do_finish(4,ifd,ofd,bigbuf);
        return 1;
    }
    chans = origchans;
    if (!headwrite(ofd,origstype,origrate,arate,Mlen,Dfac)) {

        fprintf(stdout,"ERROR: Failed to write valid header to output file.\n");
        fflush(stdout);
        do_finish(4,ifd,ofd,bigbuf);
        return 1;
    }
    Mfree(bigbuf);
    sndcloseEx(ofd);

    if((ifd = sndopenEx(argv[2],0,CDP_OPEN_RDONLY))<0)  {
        fprintf(stdout,"INFO: Cannot open intermediate analysis data file: %s\n",argv[2]);
        fflush(stdout);
        do_finish(0,ifd,ofd,bigbuf);
        return 1;
    }
//TW COMMENT as this is a temporary sndfile: may as well leave it as SAMP_SHORT
    if((ofd = sndcreat_formatted(argv[3],-1,SAMP_SHORT,
            MONO,origrate,CDP_CREATE_NORMAL)) < 0) {
        fprintf(stdout,"ERROR: Can't create output soundfile %s\n", argv[3]);
        fflush(stdout);
        do_finish(5,ifd,ofd,bigbuf);
        return 1;
    }
    if(!pvoc_process_addon(ifd,ofd,chans,origrate,arate,Mlen,Dfac)) {
        do_finish(6,ifd,ofd,bigbuf);
        return 1;
    }
    do_finish(6,ifd,ofd,bigbuf);
    return 1;
}

/***************************** HEADWRITE *******************************/

int headwrite(int ofd,int origstype,int origrate,float arate,int Mlen,int Dfac)
{
    if(sndputprop(ofd,"original sampsize", (char *)&origstype, sizeof(int)) < 0){
        fprintf(stdout,"ERROR: Failure to write original sample size. headwrite()\n");
        fflush(stdout);
        return(0);
    }
    if(sndputprop(ofd,"original sample rate", (char *)&origrate, sizeof(int)) < 0){
        fprintf(stdout,"ERROR: Failure to write original sample rate. headwrite()\n");
        fflush(stdout);
        return(0);
    }
    if(sndputprop(ofd,"arate",(char *)&arate,sizeof(float)) < 0){
        fprintf(stdout,"ERROR: Failed to write analysis sample rate. headwrite()\n");
        fflush(stdout);
        return(0);
    }
    if(sndputprop(ofd,"analwinlen",(char *)&Mlen,sizeof(int)) < 0){
        fprintf(stdout,"ERROR: Failure to write analysis window length. headwrite()\n");
        fflush(stdout);
        return(0);
    }
    if(sndputprop(ofd,"decfactor",(char *)&Dfac,sizeof(int)) < 0){
        fprintf(stdout,"ERROR: Failure to write decimation factor. headwrite()\n");
        fflush(stdout);
        return(0);
    }
    return(1);
}

/********************* GET_DATA_FROM_PITCHFILE *************************/


int get_data_from_pitchfile(float **pitches,int wanted,int ifd)
{
    int samps_read;
    if((*pitches = (float *)malloc(wanted * sizeof(float)))==NULL) {
        fprintf(stdout,"ERROR: Insufficient memory to reread pitchdata.\n");
        fflush(stdout);
        return 0;
    }
    if((samps_read = fgetfbufEx(*pitches, wanted,ifd,0))<0) {
        fprintf(stdout,"ERROR: Cannot reread pitchdata.\n");
        fflush(stdout);
        return 0;
    }
    if(samps_read!=wanted) {
        fprintf(stdout,"ERROR: Problem rereading pitch data.\n");
        fflush(stdout);
        return 0;
    }
    return(1);
}

/****************************** SETUP_AMPLITUDES ***************/

int setup_amplitudes(int clength,double halfchwidth,double chwidth,float **testpamp,float **totpamp,float **chbot)
{   
    int n, cc;
    if((*testpamp = (float *)malloc(PARTIALS_IN_TEST_TONE * sizeof(float)))==NULL) {
        fprintf(stdout,"ERROR: Insufficient memory A.\n");
        fflush(stdout);
        return 0;
    }
    if((*totpamp  = (float *)malloc(PARTIALS_IN_TEST_TONE * sizeof(float)))==NULL) {
        fprintf(stdout,"ERROR: Insufficient memory B.\n");
        fflush(stdout);
        return 0;
    }
    (*totpamp)[0] = (*testpamp)[0] = 0.5f;
    for(n = 1; n < PARTIALS_IN_TEST_TONE; n++)    /* ACTUAL PARTIAL AMPS */
        (*testpamp)[n] = (float)((*testpamp)[n-1] * PARTIAL_DECIMATION);
    for(n = 1; n < PARTIALS_IN_TEST_TONE; n++)    /* SUM OF PARTIAL AMPS */
        (*totpamp)[n] = (float)((*totpamp)[n-1] + (*testpamp)[n]);
    if((*chbot    = (float *)malloc(clength * sizeof(float)))==NULL) {
        fprintf(stdout,"ERROR: Insufficient memory C.\n");
        fflush(stdout);
        return 0;
    }
    (*chbot)[0] = 0.0f;
    (*chbot)[1] = (float)halfchwidth;
    for(cc = 2 ;cc < clength; cc++)
        (*chbot)[cc] = (float)((*chbot)[cc-1] + chwidth);
    return 1;
}

/****************************** DO_FINISH *********************************/

void do_finish(int n,int ifd,int ofd,float *bigbuf) 
{
    switch(n) {
    case(2):
        sndcloseEx(ofd);
    case(1):
        sndcloseEx(ifd);
    case(0):
//      sffinish();
        break;
    case(4):
        Mfree(bigbuf);
    case(3):
        sndcloseEx(ofd);
//      sffinish();
        break;
    case(6):
        sndcloseEx(ofd);
    case(5):
        sndcloseEx(ifd);
//      sffinish();
        break;
    }
}

/****************************** MAKE_BUFFER *********************************/

int make_buffer(float **bigbuf,int *big_fsize, int wanted)
{
    size_t bigbufsize  = (size_t) Malloc(-1);
    *big_fsize   = bigbufsize/sizeof(float);
    if((*big_fsize  = ((*big_fsize)/wanted) * wanted)<=0) {
        fprintf(stdout,"ERROR: Insufficient memory E.\n");
        fflush(stdout);
        return 0;
    }
    if((*bigbuf = (float*)Malloc(*big_fsize * sizeof(float)))==NULL) {
        fprintf(stdout,"ERROR: Insufficient memory F.\n");
        fflush(stdout);
        return 0;
    }
    memset((char *)(*bigbuf),0,(*big_fsize * sizeof(float)));
    return 1;
}

/************************* GEN_TESTDATA *************************/

int gen_testdata(int ofd,float *bigbuf,float *bigbufend,int big_fsize,
int wlength,int clength,int wanted,int *total_samps_written,float *pitches,
double chwidth,double halfchwidth,float *chbot,double nois_chanamp,float *testpamp,float *totpamp,
double nyquist,double pre_totalamp)
{
    double thisfrq;
    int n = 0, samps_to_write, samps_written;
    float *sampbuf = bigbuf;
    int cc, vc;
    while(n < wlength) {
        thisfrq = (double)pitches[n];
        if(thisfrq < NOT_PITCH)     /* NO SOUND FOUND : GENERATE SILENCE */
            gen_silence_in_sampbuf(sampbuf,chwidth,halfchwidth,clength,chbot,nois_chanamp);
        else if(thisfrq < 0.0)      /* NO PITCH FOUND : GENERATE NOISE */
            gen_nois_in_sampbuf(sampbuf,chwidth,halfchwidth,clength,chbot,nois_chanamp);
        else {              /* GENERATE TESTTONE AT FOUND PITCH */
            preset_sampbuf(sampbuf,halfchwidth,clength,chbot);
            gen_tone_in_sampbuf(thisfrq,sampbuf,testpamp,totpamp,nyquist,pre_totalamp,chwidth,halfchwidth,wanted);
        }
        if(n==0) {
            for(cc = 0, vc = 0; cc < clength; cc++, vc += 2)
                sampbuf[vc] = 0.0f;
        }
        if((sampbuf += wanted) >= bigbufend) {
            if((samps_written = fputfbufEx(bigbuf,big_fsize,ofd))<0) {
                fprintf(stdout,"ERROR: Can't write to analysis file.\n");
                fflush(stdout);
                return 0;
            }
            *total_samps_written += big_fsize;
            memset((char *)bigbuf,0,big_fsize * sizeof(float));
            sampbuf = bigbuf;
        }
        n++;
    }
    if(sampbuf != bigbuf) {
        samps_to_write = (sampbuf - bigbuf);
        *total_samps_written += samps_to_write;
        if((samps_written = fputfbufEx(bigbuf,samps_to_write,ofd))<0) {
            fprintf(stdout,"ERROR: Can't write to analysis file.\n");
            fflush(stdout);
            return 0;
        }
    }
    return 1;
}

/*************************** GEN_SILENCE_IN_SAMPBUF *********************/

void gen_silence_in_sampbuf(float *sampbuf,double chwidth,double halfchwidth,int clength,
        float *chbot,double nois_chanamp)
{
    int cc, vc, clength_less_1 = clength - 1;
    sampbuf[0] = (float)nois_chanamp;
    sampbuf[1] = (float)(drand48() * halfchwidth);
    for(cc = 1, vc = 2; cc < clength_less_1; cc++, vc += 2) {
        sampbuf[vc+1] = (float)((drand48() * chwidth) + chbot[cc]);
        sampbuf[vc]   = 0.0f;
    }
    sampbuf[vc+1] = (float)((drand48() * halfchwidth) + chbot[cc]);
    sampbuf[vc]   = 0.0f;
}

/*************************** GEN_NOIS_IN_SAMPBUF *********************/

void gen_nois_in_sampbuf(float *sampbuf,double chwidth,double halfchwidth,int clength,
        float *chbot,double nois_chanamp)
{
    int cc, vc, clength_less_1 = clength - 1;
    sampbuf[0] = (float)nois_chanamp;
    sampbuf[1] = (float)(drand48() * halfchwidth);
    for(cc = 1, vc = 2; cc < clength_less_1; cc++, vc += 2) {
        sampbuf[vc+1] = (float)((drand48() * chwidth) + chbot[cc]);
        sampbuf[vc]   = (float)nois_chanamp;
    }
    sampbuf[vc+1] = (float)((drand48() * halfchwidth) + chbot[cc]);
    sampbuf[vc]   = (float)nois_chanamp;
}

/********************** GEN_TONE_IN_SAMPBUF ***************************/

void gen_tone_in_sampbuf(double thisfrq,float *sampbuf,float *testpamp,float *totpamp,
    double nyquist,double pre_totalamp,double chwidth,double halfchwidth,int wanted)
{
    int cc, vc, m, ampadjusted = 0;
    int lastvc = -1;
    double post_totalamp;
    for(m=0;m<PARTIALS_IN_TEST_TONE;m++) {
        cc = (int)((thisfrq + halfchwidth)/chwidth);
        if((vc = cc)!=0)
            vc = cc * 2;
        if(vc != lastvc) {
            sampbuf[vc]   = testpamp[m];
            sampbuf[vc+1] = (float)thisfrq;
        }
        lastvc = vc;
        if((thisfrq = thisfrq * 2.0) > nyquist) {
            post_totalamp = (double)totpamp[m];
            normalise(post_totalamp,pre_totalamp,wanted,sampbuf);
            ampadjusted = 1;
            break;
        }
    }
    if(!ampadjusted) {
        post_totalamp = (double)totpamp[PARTIALS_IN_TEST_TONE-1];
        normalise(post_totalamp,pre_totalamp,wanted,sampbuf);
    }
}

/******************************** NORMALISE ***************************/

void normalise(double post_totalamp,double pre_totalamp,int wanted,float *sampbuf)
{
    double normaliser;
    int vc;
    if(post_totalamp < VERY_TINY_AMP)
        return;
    normaliser = pre_totalamp/post_totalamp;
    for(vc = 0; vc < wanted; vc += 2) {
        if(sampbuf[vc] > 0.0f)
            sampbuf[vc] = (float)(sampbuf[vc] * normaliser);
    }
}

/****************************** HAMMING ******************************/

void hamming(float *win,int winLen,int even)
{
    float Pi,ftmp;
    int i;

/***********************************************************
                    Pi = (float)((double)4.*atan((double)1.));
***********************************************************/
    Pi = (float)PI;
    ftmp = Pi/winLen;

    if (even) {
        for (i=0; i<winLen; i++)
        *(win+i) = (float)((double).54 + (double).46*cos((double)(ftmp*((float)i+.5))));
        *(win+winLen) = 0.0f;}
    else{   *(win) = 1.0f;
        for (i=1; i<=winLen; i++)
        *(win+i) =(float)((double).54 + (double).46*cos((double)(ftmp*(float)i)));
    }
    return;
}

/****************************** FLOAT_ARRAY ******************************/

int pvoc_float_array(int nnn,float **ptr)
{   /* set up a floating point array length nnn. */
    *ptr = (float *) calloc(nnn,sizeof(float));
    if(*ptr==NULL){
        fprintf(stdout,"ERROR: insufficient memory for PVOC\n");
        fflush(stdout);
        return(0);
    }
    return(1);
}

/****************************** PVOC_PROCESS_ADDON ******************************/

int pvoc_process_addon(int ifd,int ofd,int chans,int origrate,float arate,int Mlen,int Dfac)
{
    int num_overflows = 0;
    float   *input,         /* pointer to start of input buffer */
            *output,        /* pointer to start of output buffer */
            *anal,          /* pointer to start of analysis buffer */
            *syn,           /* pointer to start of synthesis buffer */
            *banal,         /* pointer to anal[1] (for FFT calls) */
            *bsyn,          /* pointer to syn[1]  (for FFT calls) */
            *nextIn,        /* pointer to next empty word in input */
            *nextOut,       /* pointer to next empty word in output */
            *analWindow,    /* pointer to center of analysis window */
            *synWindow,     /* pointer to center of synthesis window */
            *maxAmp,        /* pointer to start of max amp buffer */
            *avgAmp,        /* pointer to start of avg amp buffer */
            *avgFrq,        /* pointer to start of avg frq buffer */
            *env,           /* pointer to start of spectral envelope */
            *i0,            /* pointer to amplitude channels */
            *i1,            /* pointer to frequency channels */
            *oldInPhase,    /* pointer to start of input phase buffer */
            *oldOutPhase,   /* pointer to start of output phase buffer */
            maxsample = 0.0, minsample = 0.0;

    int     M = 0,          /* length of analWindow impulse response */
            D = 0,          /* decimatin factor */
            I = 0,          /* interpolation factor (default will be I=D)*/
            pvoc_chans = chans - 2,
            analWinLen,     /* half-length of analysis window */
            synWinLen;      /* half-length of synthesis window */

    int outCount,       /* number of samples written to output */
            ibuflen,        /* length of input buffer */
            obuflen,        /* length of output buffer */
            nI = 0,         /* current input (analysis) sample */
            nO,             /* current output (synthesis) sample */
            nMaxOut;        /* last output (synthesis) sample */
    int isr,            /* sampling rate */
            endsamp = VERY_BIG_INT;

    float   mag,            /* magnitude of analysis data */
            phase,          /* phase of analysis data */
//            RoverTwoPi,     /* R/D divided by 2*Pi */
            TwoPioverR,     /* 2*Pi divided by R/I */
            F = (float)0,   /* fundamental frequency (determines pvoc_chans) */
            sum,            /* scale factor for renormalizing windows */
//            rIn,            /* decimated sampling rate */
            rOut,           /* pre-interpolated sampling rate */
            R;              /* input sampling rate */

    int     i,j,k,      /* index variables */
            Dd,         /* number of new inputs to read (Dd <= D) */
            Ii,         /* number of new outputs to write (Ii <= I) */
            N2,         /* pvoc_chans/2 */
            NO,         /* synthesis NO = pvoc_chans / P */
            NO2,        /* NO/2 */
            IO,         /* synthesis IO = I / P */
            IOi,        /* synthesis IOi = Ii / P */
            Mf = 0,     /* flag for even M */
#ifdef SINGLE_SAMP
            rv,         /* return value from fgetfloat */
#endif
            flag = 0;   /* end-of-input flag */

#if defined(__SC__) && defined(SOFT_FP)
    extern int _8087;
    _8087 = 0;
#endif

    {
        char *mem;

        if((mem = malloc(64*SECSIZE)) == 0
         ||sndsetbuf(ifd, mem, 64) < 0) {
            fprintf(stdout, "ERROR: Can't set big input buffer for PVOC.\n");
            return(0);
        }
    }

    isr      = origrate;
    M        = Mlen;
    D        = Dfac;
    R        = ((float) D * arate);

    if(flteq(R,0.0)) {
        fprintf(stdout, "ERROR: Problem: zero sampling rate in PVOC\n");
        return(0);
    }

    N2 = pvoc_chans / 2;

    F = (float)(R /(float)pvoc_chans);

    Mf = 1 - M%2;

    if (M < 7) {
        fprintf(stdout,"WARNING: analWindow impulse response is too small\n");
        fflush(stdout);
    }
    ibuflen = 4 * M;
    obuflen = 4 * M;

    I   = D;
    NO  = pvoc_chans;   /* synthesis transform will be NO points */
    NO2 = NO/2;
    IO  = I;

    /* set up analysis window: The window is assumed to be symmetric
        with M total points.  After the initial memory allocation,
        analWindow always points to the midpoint of the window
        (or one half sample to the right, if M is even); analWinLen
        is half the true window length (rounded down). Any low pass
        window will work; a Hamming window is generally fine,
        but a Kaiser is also available.  If the window duration is
        longer than the transform (M > N), then the window is
        multiplied by a sin(x)/x function to meet the condition:
        analWindow[Ni] = 0 for i != 0.  In either case, the
        window is renormalized so that the phase vocoder amplitude
        estimates are properly scaled.  The maximum allowable
        window duration is ibuflen/2. */

    if(!pvoc_float_array(M+Mf,&analWindow))
        return(0);
    analWindow += (analWinLen = M/2);

    hamming(analWindow,analWinLen,Mf);

    for (i = 1; i <= analWinLen; i++)
        *(analWindow - i) = *(analWindow + i - Mf);

    if (M > pvoc_chans) {
        if (Mf)
        *analWindow *=(float)
        ((double)pvoc_chans*sin((double)PI*.5/pvoc_chans)/(double)(PI*.5));
        for (i = 1; i <= analWinLen; i++) 
            *(analWindow + i) *=(float)
            ((double)pvoc_chans * sin((double) (PI*(i+.5*Mf)/pvoc_chans) / (PI*(i+.5*Mf))));
        for (i = 1; i <= analWinLen; i++)
            *(analWindow - i) = *(analWindow + i - Mf);
    }

    sum = 0.0f;
    for (i = -analWinLen; i <= analWinLen; i++)
        sum += *(analWindow + i);

    sum = (float)(2.0/sum);     /*factor of 2 comes in later in trig identity*/

    for (i = -analWinLen; i <= analWinLen; i++)
        *(analWindow + i) *= sum;

    /* set up synthesis window:  For the minimal mean-square-error
        formulation (valid for N >= M), the synthesis window
        is identical to the analysis window (except for a
        scale factor), and both are even in length.  If N < M,
        then an interpolating synthesis window is used. */

    if(!pvoc_float_array(M+Mf,&synWindow))
        return(0);
    synWindow += (synWinLen = M/2);

    if (M <= pvoc_chans){
        hamming(synWindow,synWinLen,Mf);
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Mf);

        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;

        sum = 0.0f;
        for (i = -synWinLen; i <= synWinLen; i+=I)
            sum += *(synWindow + i) * *(synWindow + i);

        sum = (float)(1.0/ sum);

        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;
    } else {
        hamming(synWindow,synWinLen,Mf);
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Mf);

        if (Mf)
            *synWindow *= (float)((double)IO * sin((double) (PI*.5/IO)) / (double)(PI*.5));
        for (i = 1; i <= synWinLen; i++) 
            *(synWindow + i) *=(float)
            ((double)IO * sin((double) (PI*(i+.5*Mf)/IO)) /(double) (PI*(i+.5*Mf)));
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Mf);

        sum = (float)(1.0/sum);

        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;
    }
      
    /* set up input buffer:  nextIn always points to the next empty
        word in the input buffer (i.e., the sample following
        sample number (n + analWinLen)).  If the buffer is full,
        then nextIn jumps back to the beginning, and the old
        values are written over. */

    if(!pvoc_float_array(ibuflen,&input))
        return(0);

    nextIn = input;

    /* set up output buffer:  nextOut always points to the next word
        to be shifted out.  The shift is simulated by writing the
        value to the standard output and then setting that word
        of the buffer to zero.  When nextOut reaches the end of
        the buffer, it jumps back to the beginning.  */

    if(!pvoc_float_array(obuflen,&output))
        return(0);

    nextOut = output;
    /* set up analysis buffer for (N/2 + 1) channels: The input is real,
        so the other channels are redundant. oldInPhase is used
        in the conversion to remember the previous phase when
        calculating phase difference between successive samples. */

    if(!pvoc_float_array(pvoc_chans+2,&anal))
        return(0);
    banal = anal + 1;

    if(!pvoc_float_array(N2+1,&oldInPhase))
        return(0);
    if(!pvoc_float_array(N2+1,&maxAmp))
        return(0);
    if(!pvoc_float_array(N2+1,&avgAmp))
        return(0);
    if(!pvoc_float_array(N2+1,&avgFrq))
        return(0);
    if(!pvoc_float_array(N2+1,&env))
        return(0);

    /* set up synthesis buffer for (pvoc_chans/2 + 1) channels: (This is included
        only for clarity.)  oldOutPhase is used in the re-
        conversion to accumulate angle differences (actually angle
        difference per second). */

    if(!pvoc_float_array(NO+2,&syn))
        return(0);
    bsyn = syn + 1;

    if(!pvoc_float_array(NO2+1,&oldOutPhase))
        return(0);

    /* initialization: input time starts negative so that the rightmost
        edge of the analysis filter just catches the first non-zero
        input samples; output time is always T times input time. */

    outCount = 0;
//    rIn  = (float)(R/(float)D);
    rOut = (float)(R/(float)I);
//    RoverTwoPi = (float)(rIn/TWOPI);
    TwoPioverR = (float)(TWOPI/rOut);
    nI = -(analWinLen / D) * D; /* input time (in samples) */
    nO = nI;                    /* output time (in samples) */
    Dd = analWinLen + nI + 1;   /* number of new inputs to read */
    Ii = 0;             /* number of new outputs to write */
    IOi = 0;
    flag = 1;

    /* main loop:  If endsamp is not specified it is assumed to be very large
        and then readjusted when fgetfloat detects the end of input. */

    while(nI < (endsamp + analWinLen)){
#ifdef SINGLE_SAMP
        for (i = 0; i < pvoc_chans+2; i++){     /* synthesis only */
            if ((rv = fgetfloat((anal+i),ifd)) <= 0){
                return 1;
            }
        }
#else
        if((i = fgetfbufEx(anal, pvoc_chans+2, ifd,0)) < 0) {
            sfperror("pvoc: read error: ");
            return(0);
        }
        if(i < pvoc_chans+2)
            return 1;
#endif

    /* reconversion: The magnitude and angle-difference-per-second in syn
        (assuming an intermediate sampling rate of rOut) are
        converted to real and imaginary values and are returned in syn.
        This automatically incorporates the proper phase scaling for
        time modifications. */

        if (NO <= pvoc_chans){
            for (i = 0; i < NO+2; i++)
                *(syn+i) = *(anal+i);
        } else {
            for (i = 0; i <= pvoc_chans+1; i++) 
                *(syn+i) = *(anal+i);
            for (i = pvoc_chans+2; i < NO+2; i++) 
                *(syn+i) = 0.0f;
        }
        
        for(i=0, i0=syn, i1=syn+1; i<= NO2; i++,i0+=2,i1+=2) {
            mag = *i0;
            *(oldOutPhase + i) += *i1 - ((float) i * F);
            phase = *(oldOutPhase + i) * TwoPioverR;
            *i0 = (float)((double)mag * cos((double)phase));
            *i1 = (float)((double)mag * sin((double)phase));
        }

    /* synthesis: The synthesis subroutine uses the Weighted Overlap-Add
        technique to reconstruct the time-domain signal.  The (pvoc_chans/2 + 1)
        phase vocoder channel outputs at time n are inverse Fourier
        transformed, windowed, and added into the output array.  The
        subroutine thinks of output as a shift register in which 
        locations are referenced modulo obuflen.  Therefore, the main
        program must take care to zero each location which it "shifts"
        out (to standard output). The subroutines reals and fft
        together perform an efficient inverse FFT.  */

        if(reals_(syn,bsyn,NO2,2)<0) {
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return 0;
        }
        if(fft_(syn,bsyn,1,NO2,1,2)<0) {
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return 0;
        }
        j = nO - synWinLen - 1;
        while (j < 0)
            j += obuflen;
        j = j % obuflen;

        k = nO - synWinLen - 1;
        while (k < 0)
            k += NO;
        k = k % NO;

        for (i = -synWinLen; i <= synWinLen; i++) { /*overlap-add*/
            if (++j >= obuflen)
                j -= obuflen;
            if (++k >= NO)
                k -= NO;
            *(output + j) += *(syn + k) * *(synWindow + i);
        }

#ifdef SINGLE_SAMP
        for (i = 0; i < IOi; i++) { /* shift out next IOi values */
            fputfloat(nextOut,ofd);
            *(nextOut++) = 0.;
            if (nextOut >= (output + obuflen))
                nextOut -= obuflen;
            outCount++;
        }
#else
        for (i = 0; i < IOi;) { /* shift out next IOi values */
            int j;
            int todo = min(IOi-i, output+obuflen-nextOut);
            if(!outfloats(nextOut,&maxsample,&minsample,&num_overflows,todo, ofd))
                return(0);
            i += todo;
            outCount += todo;
            for(j = 0; j < todo; j++)
                *nextOut++ = 0.0f;
            if (nextOut >= (output + obuflen))
                nextOut -= obuflen;
        }
#endif
                    
        if(flag                             /* flag means do this operation only once */
        && (nI > 0) && (Dd < D)) {          /* EOF detected */
            flag = 0;
            endsamp = nI + analWinLen - (D - Dd);
        }

        nI += D;                /* increment time */
        nO += IO;
                                /* Dd = D except when the end of the sample stream intervenes */
        Dd = min(D, max(0, D+endsamp-nI-analWinLen));

        if (nO > (synWinLen + I))
            Ii = I;
        else if (nO > synWinLen)
            Ii = nO - synWinLen;
        else {
            Ii = 0;
            for (i=nO+synWinLen; i<obuflen; i++) {
                if (i > 0)
                    *(output+i) = 0.0f;
            }
        }
        IOi = Ii;


    }   /* End of main while loop */

    nMaxOut = endsamp;
    while (outCount <= nMaxOut){
#ifdef SINGLE_SAMP
        outCount++;
        fputfloat(nextOut++,ofd);
        if (nextOut >= (output + obuflen))
            nextOut -= obuflen;
#else
        int todo = min(nMaxOut-outCount, output+obuflen-nextOut);
        if(todo == 0)
            break;
        if(!outfloats(nextOut,&maxsample,&minsample,&num_overflows,todo, ofd))
            return(0);
        outCount += todo;
        nextOut += todo;
        if (nextOut >= (output + obuflen))
            nextOut -= obuflen;
#endif
    }
    return(1);
}

/************************************ OUTFLOATS ************************************
 *TW 2002 no longer need to do conversion to floats: this func is now just checking for clipping
 *
 *  ORIGINALLY (MCA) Converted floats -> shorts explicitly, since we are compiled with
 *  hardware FP(probably), and the sound filing system is not!
 *  (even without this, it should be more efficient!)
 */
int outfloats(float *nextOut,float *maxsample,float *minsample,int *num_overflows,int todo, int ofd)
{
    static float *sbuf = 0;
    static int sblen = 0;
    float *sp;
    int cnt;
    float val;

    if(sblen < todo) {
        if(sbuf != 0)
            free(sbuf);
        if((sbuf = (float *)malloc(todo*sizeof(float))) == 0) {
            fprintf(stdout, "ERROR: PVOC can't allocate output buffer\n");
            fflush(stdout);
            return(0);
        }
        sblen = todo;
    }

    sp = sbuf;
#ifdef NOOVERCHK
    for(cnt = 0; cnt < todo; cnt++)
        *sp++ = *nextOut++;
#else
    for(cnt = 0; cnt < todo; cnt++) {
        val = *nextOut++;
        if(val >= 1.0 || val <= -1.0) {
            (*num_overflows)++;
            if(val > 0.0f) {
                if(val > *maxsample)
                    *maxsample = val;
                val = 1.0f;
            }
            if(val < 0.0f) {
                if(val < *minsample)
                    *minsample = val;
                val = -1.0f;
            }
        }
        *sp++ = val;
    }
#endif

    if(fputfbufEx(sbuf, todo, ofd) < todo) {
        fprintf(stdout,"ERROR: PVOC write error");
        fflush(stdout);
        return(0);
    }
    return(1);
}

/**************************** PRESET_SAMPBUF ******************************
 *
 * Preset all frqs to centre of channels.
 */

void preset_sampbuf(float *sampbuf,double halfchwidth, int clength, float *chbot)
{
    int cc, vc , clength_less_1 = clength -1;
    sampbuf[1] = (float)(halfchwidth/2.0);
    for(cc = 1, vc = 2; cc < clength_less_1; cc++, vc += 2)
        sampbuf[vc+1] = (float)(chbot[cc] + halfchwidth);
    sampbuf[vc+1]=(float)(chbot[clength_less_1]+(halfchwidth/2.0));
}

/**************************** FLTEQ *******************************/

int flteq(double f1,double f2)
{   double upperbnd, lowerbnd;
    upperbnd = f2 + FLTERR;     
    lowerbnd = f2 - FLTERR;     
    if((f1>upperbnd) || (f1<lowerbnd))
    return(0);
    return(1);
}

