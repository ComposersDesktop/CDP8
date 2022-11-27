/*
 * Copyright (c) 1983-2020 Richard Dobson and Composers Desktop Project Ltd
 * http://www.rwdobson.com
 * http://www.composersdesktop.com
 * This file is part of the CDP System.
 * The CDP System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *
 * The CDP System is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with the CDP System; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */


/*pvoc.cpp*/
/*  simple class wrapper for (modified) CARL pvoc; supports realtime streaming. (c) Richard Dobson 2000,2014 */

/* includes correction to sinc function code from Dan Timis, June 2000 */
/* RWD 12:2000: define PVOC_NORM_PHASE to use slower normalized phase calcs */
/* NB: optional FFTW code is for v 2.1.5 */

#include <math.h>
#include <stdlib.h>
#include <memory.h>
extern "C" {
#ifdef USE_FFTW
# include <rfftw.h>
# else
void fft_(float *, float *,int,int,int,int);
void fftmx(float *,float *,int,int,int,int,int,int *,float *,float *,float *,float *,int *,int[]);
void reals_(float *,float *,int,int);
#endif
}
#include "pvpp.h"

#ifndef PI
#define PI (3.141592653589793)
#endif
#define TWOPI (2.0 * PI)

#if defined _WIN32 && defined _MSC_VER
#pragma message ("using assembler round()") 
__inline static int round(double fval)
{
    int result;
    _asm{
        fld     fval
        fistp   result
        mov     eax,result
    }
    return result;
}

#else
#define round(x) lround((x))
#endif


phasevocoder::phasevocoder()
{

    input       =  NULL;
    output      = NULL;
    anal        = NULL;
    syn         =  NULL;        /* pointer to start of synthesis buffer */
    banal       =  NULL;        /* pointer to anal[1] (for FFT calls) */
    bsyn        =  NULL;        /* pointer to syn[1]  (for FFT calls) */
    nextIn      =  NULL;    /* pointer to next empty word in input */
    nextOut     =  NULL;    /* pointer to next empty word in output */
    analWindow  =  NULL;    /* pointer to center of analysis window */
    synWindow   =  NULL;    /* pointer to center of synthesis window */
    maxAmp      =  NULL;    /* pointer to start of max amp buffer */
    avgAmp      =  NULL;    /* pointer to start of avg amp buffer */
    avgFrq      =  NULL;    /* pointer to start of avg frq buffer */
    env     =  NULL;        /* pointer to start of spectral envelope */
    i0      =  NULL;        /* pointer to amplitude channels */
    i1      =  NULL;        /* pointer to frequency channels */
    oi      =  NULL;        /* pointer to old phase channels */
    oldInPhase      =  NULL;    /* pointer to start of input phase buffer */
    oldOutPhase     =  NULL;    /* pointer to start of output phase buffer */
    m = n = 0;

    N = 0;      /* number of phase vocoder channels (bands) */
    M = 0;      /* length of analWindow impulse response */
    L = 0;      /* length of synWindow impulse response */
    D = 0;      /* decimation factor (default will be M/8) */
    I = 0;      /* interpolation factor (default will be I=D)*/
    W = -1;     /* filter overlap factor (determines M, L) */
    //F = 0;        /* fundamental frequency (determines N) */
    //F2 = 0;       /* F/2 */
    /*RWD */
    Fexact = 0.0f;
    analWinLen = 0, /* half-length of analysis window */
    synWinLen = 0;  /* half-length of synthesis window */

    sampsize = 0;   /* sample size for output file */
    outCount = 0;   /* number of samples written to output */
    ibuflen= 0; /* length of input buffer */
    obuflen= 0; /* length of output buffer */
    nI = 0;     /* current input (analysis) sample */
    nO= 0;      /* current output (synthesis) sample */
    nMaxOut= 0; /* last output (synthesis) sample */
    nMin = 0;   /* first input (analysis) sample */
    nMax = 100000000;   /* last input sample (unless EOF) */
/***************************** 6:2:91  OLD CODE **************
                        long    origsize;
*******************************NEW CODE **********************/
    origsize = 0;   /* sample type of file analysed */
    ch = 0;
    ifd =  ofd = -1;
    beta = 6.8f;    /* parameter for Kaiser window */
    real = 0.0f;        /* real part of analysis data */
    imag= 0.0f;     /* imaginary part of analysis data */
    mag= 0.0f;      /* magnitude of analysis data */
    phase= 0.0f;        /* phase of analysis data */
    angleDif= 0.0f; /* angle difference */
    RoverTwoPi= 0.0f;   /* R/D divided by 2*Pi */
    TwoPioverR= 0.0f;   /* 2*Pi divided by R/I */
    sum= 0.0f;      /* scale factor for renormalizing windows */
    ftot = 0.0f,    /* scale factor for calculating statistics */
    rIn= 0.0f;      /* decimated sampling rate */
    rOut= 0.0f;     /* pre-interpolated sampling rate */
    invR= 0.0f;     /* 1. / srate */
    time= 0.0f;     /* nI / srate */
    tvx0 = 0.0f;
    tvx1 = 0.0f;
    tvdx = 0.0f;
    tvy0 = tvy1 = 0.0f;
    tvdy = 0.0f;
    frac = 0.0f;
    warp = 0.0f;    /* spectral envelope warp factor */
    R = 0.0f;       /* input sampling rate */
    P = 1.0f;       /* pitch scale factor */
    Pinv= 0.0f;     /* 1. / P */
    T = 1.0f;       /* time scale factor ( >1 to expand)*/
    //Mlen,
    Mf = 0;     /* flag for even M */
    Lf = 0;     /* flag for even L */
    //Dfac,
    flag = 0;       /* end-of-input flag */
    C = 0;      /* flag for resynthesizing even or odd chans */
    Ci = 0;     /* flag for resynthesizing chans i to j only */
    Cj = 0;     /* flag for resynthesizing chans i to j only */
    CC = 0;     /* flag for selected channel resynthesis */
    X = 0;      /* flag for magnitude output */
    E = 0;      /* flag for spectral envelope output */
    tvflg = 0;  /* flag for time-varying time-scaling */
    tvnxt = 0;
    tvlen = 0;
    timecheckf= 0.0f;
    K = H = 0;  /* default window is Hamming */
    Nchans = 0;
    NO2 = 0;
    vH = 0;                     /* RWD set to 1 to set von Hann window */
    bin_index = 0;
    m_mode = PVPP_NOT_SET;
    synWindow_base = NULL;
    analWindow_base = NULL;
};

/* use when decfac needs specifying directly: cuts out other options */
bool phasevocoder::init(long outsrate,long fftlen,long winlen,long decfac,float scalefac,
                        pvoc_scaletype stype,pvoc_windowtype wtype,pvocmode mode)
{       
    N = fftlen;  
    M = N*2;          /* RWD make double-window the default  */
    D = decfac;
    if(scalefac <=0.0)
        return false;
    
    switch(stype){
    case PVOC_S_TIME:
        T = scalefac;
        P = 1.0f;
        break;
    case PVOC_S_PITCH:
        T = P = scalefac;
        break;
    default:
        T = P = 1.0f;
        break;
    }
    switch(wtype){
    case PVOC_HANN:
        H = 1;
        break;
    case PVOC_KAISER:
        K = 1;
        break;
    default:
        /* for now, anything else just sets Hamming window! */
        break;
    }
    if(N <=0)
        return false;
    if(D < 0)
        return false;
    if(M < 0)
        return false;

    /*for now */
    if(!(mode == PVPP_OFFLINE || mode == PVPP_STREAMING) )
        return false;
    m_mode  = mode;

    isr = outsrate;
    R       = srate = (float) outsrate;
    N       = N  + N%2; /* Make N even */
    N2      = N / 2;
//  if (N2 > 16384){
//      return false;
//  }

//  F       = (int)((float) R / N);
    Fexact  = (float)R / (float)N;      /* RWD */
//  F2      = F / 2;
    if(winlen > 0)
        M   = winlen;
    Mf      = 1 - M%2;

    L       =  (L != 0 ? L : M);
    Lf      = 1 - L%2;
    ibuflen = 4 * M;
    obuflen = 4 * L;


    if (W == -1)
        W = (int)(3.322 * log10((double)(4. * N) / M));/* cosmetic */

    if (Cj == 0)
        Cj = N2;
    if (Cj > N2)
        Cj = N2;
    if (Ci < 0)
        Ci = 0;
    D = (int)((D != 0 ? D : M/(8.0*(T > 1.0 ? T : 1.0))));

    if (D == 0){
        //fprintf(stderr,"pvoc: warning - T greater than M/8 \n");
        D = 1;
    }

    I = (int)(I != 0 ? I : (float) T*D );

    T = ((float) I / D);

    if (P != 1.)
        P = T;

    NO = (int)((float) N / P);  /* synthesis transform will be NO points */
    NO = NO + NO%2;             /* make NO even */

    NO2 = NO / 2;

    P = ((float) N / NO);       /* ideally, N / NO = I / D = pitch change */
    Pinv = (float)(1.0/ P);

    if (warp == -1.)
        warp = P;
    if ((E == 1) && (warp == 0.))
        warp = 1.0f;


    //if ((P != 1.) && (P != T))
    //   fprintf(stderr,"pvoc: warning P=%f not equal to T=%f\n",P,T);

    IO = (int)((float) I / P);

    nMax -= nMin;

    /*RWD need this to get sum setup for synth window! */
    /* NB vonHann window also available now */

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


    analWindow_base = new float[M+Mf];
    analWindow = analWindow_base + (analWinLen = M/2);
#ifdef USE_FFTW 
    in_fftw_size = N;
    out_fftw_size = NO;
    forward_plan = rfftwnd_create_plan_specific(1,&in_fftw_size, 
        FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE | FFTW_IN_PLACE,
        (fftw_real*) analWindow_base,1,NULL,1);
    inverse_plan = rfftwnd_create_plan(1,&out_fftw_size, FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE | FFTW_IN_PLACE);
#endif

    if (K)  
        kaiser(analWindow_base,M+Mf,beta);          /* ??? or just M?*/
    else if (vH)
        vonhann(analWindow,analWinLen,Mf);
    else
        hamming(analWindow,analWinLen,Mf);


    for (i = 1; i <= analWinLen; i++)
        *(analWindow - i) = *(analWindow + i - Mf);

    if (M > N) {
        if (Mf)
        *analWindow *=(float)( (double)N * sin((double)PI*.5/N) /(double)( PI*.5));
        for (i = 1; i <= analWinLen; i++) 
            *(analWindow + i) *=(float)
            ((double)N * sin((double) (PI*(i+.5*Mf)/N)) / (PI*(i+.5*Mf)));  /* D.T. 2000*/
        for (i = 1; i <= analWinLen; i++)
            *(analWindow - i) = *(analWindow + i - Mf);
    }

    sum = 0.0f;
    for (i = -analWinLen; i <= analWinLen; i++)
        sum += *(analWindow + i);

    sum = (float)(2.0 / sum);       /*factor of 2 comes in later in trig identity*/

    for (i = -analWinLen; i <= analWinLen; i++)
        *(analWindow + i) *= sum;

    /* set up synthesis window:  For the minimal mean-square-error
        formulation (valid for N >= M), the synthesis window
        is identical to the analysis window (except for a
        scale factor), and both are even in length.  If N < M,
        then an interpolating synthesis window is used. */

    synWindow_base = new float[L+Lf];
    synWindow = synWindow_base + (synWinLen = L/2);
#ifdef USE_FFTW
    Ninv = (float) (1.0 / N);
#endif
    if (M <= N){
        
        if(K)
            kaiser(synWindow_base,L+Lf,beta);
        else if(vH)
            vonhann(synWindow,synWinLen,Lf);
        else
            hamming(synWindow,synWinLen,Lf);
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Lf);

        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;

        sum = 0.0f;
        for (i = -synWinLen; i <= synWinLen; i+=I)
            sum += *(synWindow + i) * *(synWindow + i);

        sum = (float)(1.0/ sum);
#ifdef USE_FFTW
        sum *= Ninv;
#endif
        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;
    }
    else {
        if(K)
            kaiser(synWindow_base,L+Lf,beta);
        else if(vH)
            vonhann(synWindow,synWinLen,Lf);
        else
            hamming(synWindow,synWinLen,Lf);
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Lf);

        if (Lf)
            *synWindow *= (float)((double)IO * sin((double) (PI*.5/IO)) / (double)(PI*.5));
        for (i = 1; i <= synWinLen; i++) 
                *(synWindow + i) *=(float)
                ((double)IO * sin((double) (PI*(i+.5*Lf)/IO)) /(double) (PI*(i+.5*Lf)));
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Lf);

        sum = (float)(1.0/sum);
#ifdef USE_FFTW
        sum *= Ninv;
#endif
        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;
    }


    
    try{

        /* set up input buffer:  nextIn always points to the next empty
        word in the input buffer (i.e., the sample following
        sample number (n + analWinLen)).  If the buffer is full,
        then nextIn jumps back to the beginning, and the old
        values are written over. */

        input = new float[ibuflen];

        nextIn = input;

        /* set up output buffer:  nextOut always points to the next word
        to be shifted out.  The shift is simulated by writing the
        value to the standard output and then setting that word
        of the buffer to zero.  When nextOut reaches the end of
        the buffer, it jumps back to the beginning.  */
        output =    new float [obuflen];

        nextOut =   output;


        /* set up analysis buffer for (N/2 + 1) channels: The input is real,
        so the other channels are redundant. oldInPhase is used
        in the conversion to remember the previous phase when
        calculating phase difference between successive samples. */

        anal    =   new float[N+2];
        banal   =   anal + 1;

        oldInPhase =    new float[N2+1];
        maxAmp =    new float[N2+1];
        avgAmp =    new float[N2+1];
        avgFrq =    new float[N2+1];
        env =       new float[N2+1];

        /* set up synthesis buffer for (N/2 + 1) channels: (This is included
        only for clarity.)  oldOutPhase is used in the re-
        conversion to accumulate angle differences (actually angle
        difference per second). */

        syn =       new float[NO+2];
        bsyn = syn + 1;
        oldOutPhase =   new float[NO2+1];
    }
    catch(...){
        if(synWindow_base){
            delete [] synWindow_base;
            synWindow_base = 0;
        }
        if(analWindow_base){
            delete [] analWindow_base;
            analWindow_base = 0;

        }
        if(input) {
            delete [] input;
            input = 0;
        }

        if(output) {
            delete [] output;
            output = 0;
        }
        if(anal) {
            delete [] anal;
            anal = 0;
        }
        if(oldInPhase) {
            delete [] oldInPhase;
            oldInPhase = 0;
        }
        if(maxAmp){
            delete [] maxAmp;
            maxAmp = 0;
        }
        if(avgAmp) {
            delete [] avgAmp;
            avgAmp = 0;
        }
        if(avgFrq) {
            delete [] avgFrq;
            avgFrq = 0;
        }
        if(env){
            delete [] env;
            env= 0;
        }
        if(syn){
            delete [] syn;
            syn = 0;
        }
        if(oldOutPhase){
            delete [] oldOutPhase;
            oldOutPhase = 0;
        }
        return false;
    }

    outCount = 0;
    rIn = ((float) R / D);
    rOut = ((float) R / I);
    invR =((float) 1. / R);
    RoverTwoPi = (float)(rIn / TWOPI);
    TwoPioverR = (float)(TWOPI / rOut);
    nI = -(analWinLen / D) * D; /* input time (in samples) */
    nO = (long)((float) T/P * nI);  /* output time (in samples) */
    Dd = analWinLen + nI + 1;   /* number of new inputs to read */
    Ii = 0;             /* number of new outputs to write */
    IOi = 0;
    flag = 1;

    for(i=0;i < ibuflen;i++) {
        input[i] = 0.0f;
        output[i] = 0.0f;
    }
    for(i=0;i < NO+2;i++)
        syn[i] = 0.0f;
    for(i=0;i < N+2;i++)
        anal[i] = 0.0f;
    for(i=0;i < NO2+1;i++)
        oldOutPhase[i] = 0.0f;
    for(i=0;i < N2+1;i++)
        oldInPhase[i] = maxAmp[i] = avgAmp[i] = avgFrq[i] = env[i] = 0.0f;
    return true;
}

/* closer to PVOC command-line format; we will need a full array of settings ere long */
/* e.g to install a custom window...*/
bool phasevocoder::init(long outsrate,long fftlen,pvoc_overlapfac ofac,float scalefac,
                        pvoc_scaletype stype,pvoc_windowtype wtype,pvocmode mode)
{
    N   = fftlen;
    D   = 0;
    //D = N/4;      /* one problem - when M is larger, overlap is larger too */
    switch(ofac){   
    case PVOC_O_W0:
        W = 0;
        break;
    case PVOC_O_W1:
        W = 1;
        break;
    case PVOC_O_W2:
        W = 2;
        break;
    case PVOC_O_W3:
        W = 3;
        break;
    default:
        W = 1;       /* double-window option */
        break;
    }
    switch(wtype){
    case PVOC_HANN:
        H = 1;      
        break;
    case PVOC_KAISER:
        K = 1;      
        break;
    default:
        /* for now, anything else just sets Hamming window! */
        break;
    }


    if(scalefac <=0.0)
        return false;
    switch(stype){
    case PVOC_S_TIME:
        T = scalefac;
        P = 1.0f;
        break;
    case PVOC_S_PITCH:
        T = P = scalefac;
        break;
    default:
        T = P = 1.0f;
        break;
    }


    if(N <=0)
        return false;
    if(D < 0)
        return false;
    /*for now */
    if(!(mode == PVPP_OFFLINE || mode == PVPP_STREAMING) )
        return false;
    m_mode  = mode;

    isr = outsrate;
    R       = srate = (float) outsrate;
    N       = N  + N%2; /* Make N even */
    N2      = N / 2;
//  if (N2 > 16384){
//      return false;
//  }

//  F       = (int)((float) R / N);
    Fexact  = (float)R / (float)N;      /* RWD */
//  F2      = F / 2;
    if (W != -1)
        /* cannot be used with M; but we don't touch M anyway here */
        switch(W){
        case 0: M = 4*N;
            break;
        case 1: M = 2*N;        /* RWD: we want this one, most of the time */
            break;
        case 2: M = N;
            break;
        case 3: M = N2;
            break;
        default:
            break;
        }

    M       = (M != 0 ? M : N*2);      /* RWD double-window as default */
    Mf      = 1 - M%2;

    L       =  M;
    Lf      = 1 - L%2;
    ibuflen = 4 * M;
    obuflen = 4 * L;

    if (W == -1)
        W = (int)(3.322 * log10((double)(4. * N) / M));/* cosmetic */
    
    if (Cj == 0)
        Cj = N2;
    if (Cj > N2)
        Cj = N2;
    if (Ci < 0)
        Ci = 0;
    D = (int)((D != 0 ? D : M/(8.0*(T > 1.0 ? T : 1.0))));

    if (D == 0){
        //fprintf(stderr,"pvoc: warning - T greater than M/8 \n");
        D = 1;
    }

    I = (int)(I != 0 ? I : (float) T*D );

    T = ((float) I / D);

    if (P != 1.)
        P = T;

    NO = (int)((float) N / P);  /* synthesis transform will be NO points */
    NO = NO + NO%2;             /* make NO even */

    NO2 = NO / 2;

    P = ((float) N / NO);       /* ideally, N / NO = I / D = pitch change */
    Pinv = (float)(1.0/ P);

    if (warp == -1.)
        warp = P;
    if ((E == 1) && (warp == 0.))
        warp = 1.0f;


    //if ((P != 1.) && (P != T))
    //   fprintf(stderr,"pvoc: warning P=%f not equal to T=%f\n",P,T);

    IO = (int)((float) I / P);

    nMax -= nMin;
    /*RWD need this to get sum setup for synth window! */
    /* NB vonHann window also available now */

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


    analWindow_base = new float[M+Mf];
    analWindow = analWindow_base + (analWinLen = M/2);


    if (K)  
        kaiser(analWindow_base,M+Mf,beta);          /* ??? or just M?*/
    else if (H)
        vonhann(analWindow,analWinLen,Mf);
    else
        hamming(analWindow,analWinLen,Mf);

    for (i = 1; i <= analWinLen; i++)
        *(analWindow - i) = *(analWindow + i - Mf);

    if (M > N) {
        if (Mf)
        *analWindow *=(float)( (double)N * sin((double)PI*.5/N) /(double)( PI*.5));
        for (i = 1; i <= analWinLen; i++) 
            *(analWindow + i) *=(float)
            ((double)N * sin((double) (PI*(i+.5*Mf)/N)) / (PI*(i+.5*Mf)));  /* D.T 2000*/
        for (i = 1; i <= analWinLen; i++)
            *(analWindow - i) = *(analWindow + i - Mf);
    }

    sum = 0.0f;
    for (i = -analWinLen; i <= analWinLen; i++)
        sum += *(analWindow + i);

    sum = (float)(2.0 / sum);       /*factor of 2 comes in later in trig identity*/

    for (i = -analWinLen; i <= analWinLen; i++)
        *(analWindow + i) *= sum;

    /* set up synthesis window:  For the minimal mean-square-error
        formulation (valid for N >= M), the synthesis window
        is identical to the analysis window (except for a
        scale factor), and both are even in length.  If N < M,
        then an interpolating synthesis window is used. */

    synWindow_base = new float[L+Lf];
    synWindow = synWindow_base + (synWinLen = L/2);
#ifdef USE_FFTW
    Ninv = (float) (1.0 / N);
#endif

    if (M <= N){
        if(K)
            kaiser(synWindow_base,L+Lf,beta);
        else if(H)
            vonhann(synWindow,synWinLen,Lf);
        else
            hamming(synWindow,synWinLen,Lf);

        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Lf);

        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;

        sum = 0.0f;
        /* RWD ??? this jumps by overlap samps thru table*/
        for (i = -synWinLen; i <= synWinLen; i+=I)
            sum += *(synWindow + i) * *(synWindow + i);

        sum = (float)(1.0/ sum);
#ifdef USE_FFTW
        sum *= Ninv;
#endif
        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;
    }
    else {
        if(K)
            kaiser(synWindow_base,L+Lf,beta);
        else if(H)
            vonhann(synWindow,synWinLen,Lf);
        else
            hamming(synWindow,synWinLen,Lf);
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Lf);

        if (Lf)
            *synWindow *= (float)((double)IO * sin((double) (PI*.5/IO)) / (double)(PI*.5));
        for (i = 1; i <= synWinLen; i++) 
                *(synWindow + i) *=(float)
                ((double)IO * sin((double) (PI*(i+.5*Lf)/IO)) /(double) (PI*(i+.5*Lf)));
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Lf);

        sum = (float)(1.0/sum);
#ifdef USE_FFTW
        sum *= Ninv;
#endif
        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;
    }

#ifdef USE_FFTW 
    in_fftw_size = N;
    out_fftw_size = NO;
    forward_plan = rfftwnd_create_plan_specific(1,&in_fftw_size, 
        FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE | FFTW_IN_PLACE,
        (fftw_real*) analWindow_base,1,NULL,1);
    inverse_plan = rfftwnd_create_plan_specific(1,&out_fftw_size, 
        FFTW_COMPLEX_TO_REAL, FFTW_ESTIMATE | FFTW_IN_PLACE,
        (fftw_real*) synWindow_base,1,NULL,1);

#endif

    try{

        /* set up input buffer:  nextIn always points to the next empty
        word in the input buffer (i.e., the sample following
        sample number (n + analWinLen)).  If the buffer is full,
        then nextIn jumps back to the beginning, and the old
        values are written over. */

        input = new float[ibuflen];

        nextIn = input;

        /* set up output buffer:  nextOut always points to the next word
        to be shifted out.  The shift is simulated by writing the
        value to the standard output and then setting that word
        of the buffer to zero.  When nextOut reaches the end of
        the buffer, it jumps back to the beginning.  */
        output =    new float [obuflen];

        nextOut =   output;


        /* set up analysis buffer for (N/2 + 1) channels: The input is real,
        so the other channels are redundant. oldInPhase is used
        in the conversion to remember the previous phase when
        calculating phase difference between successive samples. */

        anal    =   new float[N+2];
        banal   =   anal + 1;

        oldInPhase =    new float[N2+1];
        maxAmp =    new float[N2+1];
        avgAmp =    new float[N2+1];
        avgFrq =    new float[N2+1];
        env =       new float[N2+1];

        /* set up synthesis buffer for (N/2 + 1) channels: (This is included
        only for clarity.)  oldOutPhase is used in the re-
        conversion to accumulate angle differences (actually angle
        difference per second). */

        syn =       new float[NO+2];
        bsyn = syn + 1;
        oldOutPhase =   new float[NO2+1];
    }
    catch(...){
        if(synWindow_base){
            delete [] synWindow_base;
            synWindow_base = 0;
        }
        if(analWindow_base){
            delete [] analWindow_base;
            analWindow_base = 0;

        }
        if(input) {
            delete [] input;
            input = 0;
        }

        if(output) {
            delete [] output;
            output = 0;
        }
        if(anal) {
            delete [] anal;
            anal = 0;
        }
        if(oldInPhase) {
            delete [] oldInPhase;
            oldInPhase = 0;
        }
        if(maxAmp){
            delete [] maxAmp;
            maxAmp = 0;
        }
        if(avgAmp) {
            delete [] avgAmp;
            avgAmp = 0;
        }
        if(avgFrq) {
            delete [] avgFrq;
            avgFrq = 0;
        }
        if(env){
            delete [] env;
            env= 0;
        }
        if(syn){
            delete [] syn;
            syn = 0;
        }
        if(oldOutPhase){
            delete [] oldOutPhase;
            oldOutPhase = 0;
        }
        return false;
    }

    outCount = 0;
    rIn = ((float) R / D);
    rOut = ((float) R / I);
    invR =((float) 1. / R);
    RoverTwoPi = (float)(rIn / TWOPI);
    TwoPioverR = (float)(TWOPI / rOut);
    nI = -(analWinLen / D) * D; /* input time (in samples) */
    nO = (long)((float) T/P * nI);  /* output time (in samples) */
    Dd = analWinLen + nI + 1;   /* number of new inputs to read */
    Ii = 0;             /* number of new outputs to write */
    IOi = 0;
    flag = 1;

    for(i=0;i < ibuflen;i++) {
        input[i] = 0.0f;
        output[i] = 0.0f;
    }
    for(i=0;i < NO+2;i++)
        syn[i] = 0.0f;
    for(i=0;i < N+2;i++)
        anal[i] = 0.0f;
    for(i=0;i < NO2+1;i++)
        oldOutPhase[i] = 0.0f;
    for(i=0;i < N2+1;i++)
        oldInPhase[i] = maxAmp[i] = avgAmp[i] = avgFrq[i] = env[i] = 0.0f;
    return true;
}





phasevocoder::~phasevocoder()
{
    if(synWindow_base)
        delete [] synWindow_base;
    if(analWindow_base)
        delete [] analWindow_base;
    if(input)
        delete [] input;
    if(output)
        delete [] output;
    if(anal)
        delete [] anal;
    if(oldInPhase)
        delete [] oldInPhase;
    if(maxAmp)
        delete [] maxAmp;
    if(avgAmp)
        delete [] avgAmp;
    if(avgFrq)
        delete [] avgFrq;
    if(env)
        delete [] env;
    if(syn)
        delete [] syn;  
    if(oldOutPhase)
        delete [] oldOutPhase;

#ifdef USE_FFTW
    rfftwnd_destroy_plan(forward_plan);
    rfftwnd_destroy_plan(inverse_plan);
#endif

}

void phasevocoder::hamming(float *win,int winLen,int even)
{
    double Pi,ftmp;
    int i;

/***********************************************************
                    Pi = (double)((double)4.*atan((double)1.));
***********************************************************/
    Pi = (double)PI;
    ftmp = Pi/winLen;

    if (even) {
        for (i=0; i<winLen; i++)
        *(win+i) = (float)(.54 + .46*cos(ftmp*((double)i+.5)));
        *(win+winLen) = 0.0;}
    else{   
        *(win) = 1.0;
        for (i=1; i<=winLen; i++)
        *(win+i) = (float)(.54 + .46*cos(ftmp*(double)i));
    }

}


double phasevocoder::besseli( double x)
{
    double ax, ans;
    double y;

    if (( ax = fabs( x)) < 3.75)     {
    y = x / 3.75;
    y *= y;
    ans = (1.0 + y * ( 3.5156229 +
              y * ( 3.0899424 +
                y * ( 1.2067492 +
                      y * ( 0.2659732 +
                        y * ( 0.360768e-1 +
                          y * 0.45813e-2))))));
    }
    else {
    y = 3.75 / ax;
    ans = ((exp ( ax) / sqrt(ax))
        * (0.39894228 +
           y * (0.1328592e-1 +
            y * (0.225319e-2 +
             y * (-0.157565e-2 +
                  y * (0.916281e-2 +
                   y * (-0.2057706e-1 +
                    y * (0.2635537e-1 +
                         y * (-0.1647633e-1 +
                          y * 0.392377e-2)))))))));
    }
    return ans;
}

//courtesy of Csound....

void phasevocoder::kaiser(float *win,int len,double Beta)
{
    float *ft = win;
    double i,xarg = 1.0;    //xarg = amp scalefactor
    for (i = -len/2.0 + .1 ; i < len/2.0 ; i++)
        *ft++ = (float) (xarg *
              besseli((Beta * sqrt(1.0-pow((2.0*i/(len - 1)), 2.0)))) /
              besseli( Beta));
   // assymetrical hack: sort out first value!
   win[0] = win[len-1];
}

void phasevocoder::vonhann(float *win,int winLen,int even)
{
    float Pi,ftmp;
    int i;

    Pi = (float)PI;
    ftmp = Pi/winLen;

    if (even) {
        for (i=0; i<winLen; i++)
        *(win+i) = (float)(.5 + .5 *cos(ftmp*((double)i+.5)));
        *(win+winLen) = 0.0f;
    }
    else{   *(win) = 1.0f;
        for (i=1; i<=winLen; i++)
        *(win+i) =(float)(.5 + .5 *cos(ftmp*(double)i));
    }   
}



long phasevocoder::process_frame(float *anal,float *outbuf,pvoc_frametype frametype)
{

    /*RWD vars */
    int n;
    long written;
    float *obufptr;

    /* reconversion: The magnitude and angle-difference-per-second in syn
        (assuming an intermediate sampling rate of rOut) are
        converted to real and imaginary values and are returned in syn.
        This automatically incorporates the proper phase scaling for
        time modifications. */
    
    if (NO <= N){
        for (i = 0; i < NO+2; i++)
            *(syn+i) = *(anal+i);
    }
    else {
        for (i = 0; i <= N+1; i++)
            *(syn+i) = *(anal+i);
        for (i = N+2; i < NO+2; i++)
            *(syn+i) = 0.0f;
    }

    if(frametype==PVOC_AMP_PHASE){
        for(i=0, i0=syn, i1=syn+1; i<= NO2; i++, i0+=2,  i1+=2){
            mag = *i0;
            phase = *i1;
            *i0 = (float)((double)mag * cos((double)phase));
            *i1 = (float)((double)mag * sin((double)phase));
        }
    }
    else if(frametype == PVOC_AMP_FREQ){
        for(i=0, i0=syn, i1=syn+1; i<= NO2; i++, i0+=2,  i1+=2){
            mag = *i0;
            /* keep phase wrapped within +- TWOPI */
            /* this could possibly be spread across several frame cycles, as the problem does not
                develop for a while */
            double angledif, the_phase;
            angledif = TwoPioverR * (*i1  - ((float) i * /*F*/Fexact));
            the_phase = *(oldOutPhase + i) +angledif;
            if(i== bin_index)
                the_phase = (float) fmod(the_phase,TWOPI);
            *(oldOutPhase + i) = (float) the_phase;
            phase = (float) the_phase;


            *i0 = (float)((double)mag * cos((double)phase));
            *i1 = (float)((double)mag * sin((double)phase));
        }
    }
#ifdef PVOC_NORM_PHASE
    if(++bin_index == NO2+1)
        bin_index = 0;
#endif

    /* else it must be PVOC_COMPLEX */
    if (P != 1.)
        for (i = 0; i < NO+2; i++)
            *(syn+i) *= Pinv;
    
    /* synthesis: The synthesis subroutine uses the Weighted Overlap-Add
            technique to reconstruct the time-domain signal.  The (N/2 + 1)
            phase vocoder channel outputs at time n are inverse Fourier
            transformed, windowed, and added into the output array.  The
            subroutine thinks of output as a shift register in which
            locations are referenced modulo obuflen.  Therefore, the main
            program must take care to zero each location which it "shifts"
            out (to standard output). The subroutines reals and fft
            together perform an efficient inverse FFT.  */


# ifdef USE_FFTW
    rfftwnd_one_complex_to_real(inverse_plan,(fftw_complex * )syn,NULL);
# else
    reals_(syn,bsyn,NO2,2);
    fft_(syn,bsyn,1,NO2,1,2);
#endif
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

    obufptr = outbuf;   /*RWD */
    written = 0;
    for (i = 0; i < IOi;){  /* shift out next IOi values */
        int j;
        int todo = MIN(IOi-i, output+obuflen-nextOut);
        /*outfloats(nextOut, todo, ofd);*/
        /*copy data to external buffer */
        for(n=0;n < todo;n++)
            *obufptr++ = nextOut[n];
        written += todo;

        i += todo;
        outCount += todo;
        for(j = 0; j < todo; j++)
            *nextOut++ = 0.0f;
        if (nextOut >= (output + obuflen))
            nextOut -= obuflen;
    }

    if (flag)   /* flag means do this operation only once */
        if ((nI > 0) && (Dd < D)){  /* EOF detected */
            flag = 0;
            nMax = nI + analWinLen - (D - Dd);
        }


    /*  D = some_function(nI);      for variable time-scaling */
    /*  rIn = ((float) R / D);      for variable time-scaling */
    /*  RoverTwoPi =  rIn / TwoPi;  for variable time-scaling */

    nI += D;                /* increment time */
    nO += IO;

    /* Dd = D except when the end of the sample stream intervenes */
    /* RWD handle offline and streaming separately - 
        can't count an infinite number of real-time samples! */
    if(m_mode == PVPP_OFFLINE)
        Dd = MIN(D, MAX(0L, D+nMax-nI-analWinLen));   /*  CARL */
    else
        Dd = D;                                       /* RWD */

    if (nO > (synWinLen + I))
        Ii = I;
    else
        if (nO > synWinLen)
            Ii = nO - synWinLen;
        else {
            Ii = 0;
            for (i=nO+synWinLen; i<obuflen; i++)
                if (i > 0)
                    *(output+i) = 0.0f;
        }
    IOi = (int)((float) Ii / P);



    return written;

}


/*RWD arrgh! */
long phasevocoder::process_frame(float *anal,short *outbuf,pvoc_frametype frametype)
{

    /*RWD vars */
    int n;
    long written;
    short *obufptr;

        /* reconversion: The magnitude and angle-difference-per-second in syn
        (assuming an intermediate sampling rate of rOut) are
        converted to real and imaginary values and are returned in syn.
        This automatically incorporates the proper phase scaling for
        time modifications. */

    if (NO <= N){
        for (i = 0; i < NO+2; i++)
            *(syn+i) = *(anal+i);
    }
    else {
        for (i = 0; i <= N+1; i++) 
            *(syn+i) = *(anal+i);
        for (i = N+2; i < NO+2; i++) 
            *(syn+i) = 0.0f;
    }
    if(frametype != PVOC_COMPLEX){      /* assume AMP_FREQ otherwise, for now */
        for(i=0, i0=syn, i1=syn+1; i<= NO2; i++, i0+=2,  i1+=2){
            mag = *i0;
            *(oldOutPhase + i) += *i1 - ((float) i * /*F*/ Fexact);
            phase = *(oldOutPhase + i) * TwoPioverR;
            *i0 = (float)((double)mag * cos((double)phase));
            *i1 = (float)((double)mag * sin((double)phase));
        }
    }
    if (P != 1.)
        for (i = 0; i < NO+2; i++)
            *(syn+i) *= Pinv;

    /* synthesis: The synthesis subroutine uses the Weighted Overlap-Add
            technique to reconstruct the time-domain signal.  The (N/2 + 1)
            phase vocoder channel outputs at time n are inverse Fourier
            transformed, windowed, and added into the output array.  The
            subroutine thinks of output as a shift register in which 
            locations are referenced modulo obuflen.  Therefore, the main
            program must take care to zero each location which it "shifts"
            out (to standard output). The subroutines reals and fft
            together perform an efficient inverse FFT.  */


#ifdef USE_FFTW
    rfftwnd_one_complex_to_real(inverse_plan,(fftw_complex * )syn,NULL);
#else
    reals_(syn,bsyn,NO2,2);
    fft_(syn,bsyn,1,NO2,1,2);
#endif
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

    obufptr = outbuf;   /*RWD */
    written = 0;
    for (i = 0; i < IOi;){  /* shift out next IOi values */
        int j;
        int todo = MIN(IOi-i, output+obuflen-nextOut);
        
        /*copy data to external buffer */
        for(n=0;n < todo;n++)
            *obufptr++ = (short) round(32767.0 * nextOut[n]);
        written += todo;

        i += todo;
        outCount += todo;
        for(j = 0; j < todo; j++)
            *nextOut++ = 0.0f;
        if (nextOut >= (output + obuflen))
            nextOut -= obuflen;
    }
                
    if (flag)   /* flag means do this operation only once */
        if ((nI > 0) && (Dd < D)){  /* EOF detected */
            flag = 0;
            nMax = nI + analWinLen - (D - Dd);
        }


    /*  D = some_function(nI);      for variable time-scaling */
    /*  rIn = ((float) R / D);      for variable time-scaling */
    /*  RoverTwoPi =  rIn / TwoPi;  for variable time-scaling */

    nI += D;                /* increment time */
    nO += IO;

    /* Dd = D except when the end of the sample stream intervenes */

    Dd = MIN(D, MAX(0L, D+nMax-nI-analWinLen));


    if (nO > (synWinLen + I))
        Ii = I;
    else
        if (nO > synWinLen)
            Ii = nO - synWinLen;
        else {
            Ii = 0;
            for (i=nO+synWinLen; i<obuflen; i++)
                if (i > 0)
                    *(output+i) = 0.0f;
        }
    IOi = (int)((float) Ii / P);



    return written;

}
/* trying to get clean playback when looping! */
void phasevocoder::reset_phases(void)
{
    int i;
    if(oldInPhase){
        for(i=0;i < N2+1;i++)
            oldInPhase[i] = 0.0f;
    }
    if(oldOutPhase){
        for(i=0;i < NO2+1;i++)
            oldOutPhase[i] = 0.0f;
    }
    if(syn){
        for(i=0;i < NO+2;i++)
            syn[i] = 0.0f;
    }
    
    
#ifdef _DEBUG
    //printf("\nnI = %d; nO = %d; j = %d; k = %d\n",nI,nO,j,k);
#endif

    /* these seem definitely necessary, but don't solve everything... */
    nI = -(analWinLen / D) * D; /* input time (in samples) */
    nO = (long)((float) T/P * nI);  /* output time (in samples) */

}


/* we don't read in a single sample, a la pvoc, but just output an empty first frame*/
/* we assume final block zero-padded if necessary */
long phasevocoder::generate_frame(float *fbuf,float *outanal,long samps,pvoc_frametype frametype)
{
    
    /*sblen = decfac = D */
    //static int sblen = 0;
    int got, tocp;
    float *fp,*ofp;

    got = samps;     /*always assume */
    if(got < Dd)
        Dd = got;

    fp = fbuf;
    

    tocp = MIN(got, input+ibuflen-nextIn);
    got -= tocp;
    while(tocp-- > 0)
        *nextIn++ = *fp++; 

    if(got > 0) {
        nextIn -= ibuflen;
        while(got-- > 0)
            *nextIn++ = *fp++;
    }
    if (nextIn >= (input + ibuflen))
        nextIn -= ibuflen;

    if (nI > 0)
        for (i = Dd; i < D; i++){   /* zero fill at EOF */
            *(nextIn++) = 0.0f;
            if (nextIn >= (input + ibuflen))
                nextIn -= ibuflen;
        }

    /* analysis: The analysis subroutine computes the complex output at
        time n of (N/2 + 1) of the phase vocoder channels.  It operates
        on input samples (n - analWinLen) thru (n + analWinLen) and
        expects to find these in input[(n +- analWinLen) mod ibuflen].
        It expects analWindow to point to the center of a
        symmetric window of length (2 * analWinLen +1).  It is the
        responsibility of the main program to ensure that these values
        are correct!  The results are returned in anal as succesive
        pairs of real and imaginary values for the lowest (N/2 + 1)
        channels.   The subroutines fft and reals together implement
        one efficient FFT call for a real input sequence.  */


    for (i = 0; i < N+2; i++)
        *(anal + i) = 0.0f; /*initialize*/

    j = (nI - analWinLen - 1 + ibuflen) % ibuflen;  /*input pntr*/

    k = nI - analWinLen - 1;            /*time shift*/
    while (k < 0)
        k += N;
    k = k % N;
    for (i = -analWinLen; i <= analWinLen; i++) {
        if (++j >= ibuflen)
            j -= ibuflen;
        if (++k >= N)
            k -= N;
        *(anal + k) += *(analWindow + i) * *(input + j);
    }
#ifdef USE_FFTW
    rfftwnd_one_real_to_complex(forward_plan,(fftw_real*) anal,NULL);
            //reals_(anal,banal,N2,-2);
#else
    fft_(anal,banal,1,N2,1,-2);
    reals_(anal,banal,N2,-2);
#endif
    /* conversion: The real and imaginary values in anal are converted to
        magnitude and angle-difference-per-second (assuming an 
        intermediate sampling rate of rIn) and are returned in
        anal. */
    if(frametype == PVOC_AMP_PHASE){
        /* PVOCEX uses plain (wrapped) phase format, ref Soundhack */
        for(i=0,i0=anal,i1=anal+1,oi=oldInPhase; i <= N2; i++,i0+=2,i1+=2, oi++){
            real = *i0;
            imag = *i1;
            *i0 =(float) sqrt((double)(real * real + imag * imag));
            /* phase unwrapping */
            /*if (*i0 == 0.)*/
            if(*i0 < 1.0E-10f)
                //angleDif = 0.0f;
                phase = 0.0f;

            else {
#ifdef OLDCALC
                rratio = atan((double)imag/(double)real);
                if(real<0.0) {
                    if(imag<0.0)
                        rratio -= PI;
                    else
                        rratio += PI;
                }
#else
                rratio = atan2((double)imag,(double)real);
#endif
                /*angleDif  = (phase = (float)rratio) - *oi;
                *oi = phase;
                */
                phase = (float)rratio;
            }

            *i1 = phase;
        }
    }
    if(frametype==PVOC_AMP_FREQ){
        for(i=0,i0=anal,i1=anal+1,oi=oldInPhase; i <= N2; i++,i0+=2,i1+=2, oi++){
            real = *i0;
            imag = *i1;
            *i0 =(float) sqrt((double)(real * real + imag * imag));
            /* phase unwrapping */
            /*if (*i0 == 0.)*/
            if(*i0 < 1.0E-10f)
                angleDif = 0.0f;

            else {
#ifdef OLDCALC
/*RWD 12.99: slower with Pentium Pro build, but otherwise faster??!?? */
                rratio = atan((double)imag/(double)real);
                if(real<0.0) {
                    if(imag<0.0)
                        rratio -= PI;
                    else
                        rratio += PI;
                }
#else
/*RWD98*/       rratio = atan2((double)imag,(double)real);
#endif
                angleDif  = (phase = (float)rratio) - *oi;
                *oi = phase;
            }
            if (angleDif > PI)
                angleDif = (float)(angleDif - TWOPI);
            if (angleDif < -PI)
                angleDif = (float)(angleDif + TWOPI);

            /* add in filter center freq.*/
            *i1 = angleDif * RoverTwoPi + ((float) i * /*F*/Fexact);
        }
    }
    /* else must be PVOC_COMPLEX */
    fp = anal;
    ofp = outanal;
    for(i=0;i < N+2;i++)
        *ofp++ = *fp++;

    nI += D;                /* increment time */
    nO += IO;
    /* deal with offline and streaming differently */
    if(m_mode== PVPP_OFFLINE)
        Dd = MIN(D, MAX(0L, D+nMax-nI-analWinLen));     /* CARL */
    else
        Dd = D;                         /* RWD */

    if (nO > (synWinLen + I))
        Ii = I;
    else
        if (nO > synWinLen)
            Ii = nO - synWinLen;
        else {
            Ii = 0;
            for (i=nO+synWinLen; i<obuflen; i++)
                if (i > 0)
                    *(output+i) = 0.0f;
        }
    IOi = (int)((float) Ii / P);


    return D;
}

