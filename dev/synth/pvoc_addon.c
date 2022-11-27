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

/* RWD 22/02/2018  replaced hamming with vonhann window for smooth frame transition. */

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <globcon.h>
#include <tkglobals.h>
#include <pnames.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <cdpmain.h>
#include <pvoc.h>
#include <string.h>
#include <sfsys.h>
#include <pvoc.h>

static int  outfloats(float *nextOut, float *maxsampl,float *minsample,int *num_overflows,int todo, dataptr dz);
static int  pvoc_float_array(int nnn,float **ptr);
static int  sndwrite_header(float R,dataptr dz);
static void hamming(float *win,int winLen,int even);
static void vonhann(float *win,int winLen,int even);
static int  pvoc_time_display(int nI,unsigned int samps_so_far,int srate,int *samptime,dataptr dz);

static void reset_file_params_for_analout(int orig_proctype,int orig_srate,
            int orig_chans,int orig_stype,dataptr dz);

/****************************** PVOC_OUT *************************
 *  
 *  1) This code assumes there is an output analysis file from a previous process.
 *  2) It assumes that the NAME of that file (orig_outfilename) is NOT the name of the new file being created.
 *      In fact its name have a HIGHER numeric index, so when it is deleted there are no gaps
 *      in the numbering of VALID outfiles (which run from 0 upwards).
 *  3) root_outfname is the base outfilename, without its numeric add-on
 *     'jj' is the humeric extension which will be added to that root name.
 *  4) any INPUT file(s) to previous process MUST BE CLOSED before calling this function
 *  5) floats_out = number of float samples in analysis file
 *  6) samps_so_far is counter of TOTAL floats written out to any output files so far,
 *      and is accumulated to the write-count sent to the timer-display bar by PVOC.
 *      It does not affect counters in writing of file data.
 *      Time-display for the process containing this function should be calculated as fraction of dz->tempsize,
 *      which itself must hold the total of ALL the samps written out to ALL the files over the whole process.
 *  7) the original analysis file (input to this function) is closed AND DELETED by this process.
 *  8) mxfft() must be included in compilation, for this to work.
 */

int pvoc_out
(int floats_out,unsigned int *samps_so_far,char *orig_outfilename, char *root_outname, int jj, dataptr dz)
{
    int exit_status;
    int orig_proctype, orig_chans, orig_stype;
    int orig_srate;
    int orig_infilecnt  = dz->infilecnt;
    int len = strlen(orig_outfilename);
    char filename[256];
    int stype;
    /*RWD 2004 */
    char pfname[_MAX_PATH];
    //Create enough space to remember filename
    if(dz->wordstor == NULL) {
        if((dz->wordstor = (char **)malloc(sizeof(char *)))==NULL)
            return(MEMORY_ERROR);
        dz->wordstor[0] = NULL;
        if((dz->wordstor[0] = (char *)malloc((len + 1) * sizeof(char *)))==NULL)
            return(MEMORY_ERROR);
    } else {
        if(strlen(dz->wordstor[0]) < (unsigned int)len) {
            if((dz->wordstor[0] = (char *)realloc((char *)dz->wordstor[0],(len + 1) * sizeof(char *)))==NULL)
                return(MEMORY_ERROR);
        }
    }
    strcpy(dz->wordstor[0],orig_outfilename);   /* save name of file in dz->wordstor[0]: in this case, no malloc reqd */

    strcpy(filename,root_outname);              /* create new outfilename, for sound output */
//TW REVISION new protocol : always pass outfile extension to cmdline
    insert_new_number_at_filename_end(filename,jj,0);
    dz->infilecnt = 2;                      /* tell system we have 1 inputfile */
    /*RWD Feb 2004 write full anal props so we can inspect etc */
#ifdef _DEBUG
    if(sndputprop(dz->ofd,"original sampsize", (char *)&(dz->outfile->origstype), sizeof(int)) < 0){
            sprintf(errstr,"Failure to write original sample size. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        if(sndputprop(dz->ofd,"original sample rate", (char *)&(dz->outfile->origrate), sizeof(int)) < 0){
            sprintf(errstr,"Failure to write original sample rate. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        if(sndputprop(dz->ofd,"arate",(char *)&(dz->outfile->arate),sizeof(float)) < 0){
            sprintf(errstr,"Failed to write analysis sample rate. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        if(sndputprop(dz->ofd,"analwinlen",(char *)&(dz->outfile->Mlen),sizeof(int)) < 0){
            sprintf(errstr,"Failure to write analysis window length. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        if(sndputprop(dz->ofd,"decfactor",(char *)&(dz->outfile->Dfac),sizeof(int)) < 0){
            sprintf(errstr,"Failure to write decimation factor. headwrite()\n");
            return(PROGRAM_ERROR);
        }
#endif  
/* set output file as input file */
    sndcloseEx(dz->ofd);
    dz->ofd = -1;
    if(dz->ifd == NULL)
        allocate_filespace(dz);
    if((dz->ifd[0] = sndopenEx(orig_outfilename,0,CDP_OPEN_RDONLY)) < 0) {
        sprintf(errstr,"Cannot reopen temporary analysis file '%s' to generate sound data:%s\n", orig_outfilename,sferrstr());
        return(SYSTEM_ERROR);
    }
    *samps_so_far += dz->total_samps_written;
    reset_file_params_for_sndout(&orig_proctype,&orig_srate,&orig_chans,&orig_stype,dz);

    if(dz->floatsam_output==1)
        stype = SAMP_FLOAT;
    else
        stype = SAMP_SHORT;
    dz->true_outfile_stype = stype;
    if((dz->ofd = sndcreat_formatted(filename,-1,stype,
            1,(int)round(dz->param[SS_SRATE]),CDP_CREATE_NORMAL)) < 0) {
        sprintf(errstr,"Cannot open output intermediate soundfile %s\n", filename);
        return(DATA_ERROR);
    }
    dz->total_samps_written = 0L;
    dz->insams[0] = floats_out;
    /*RWD need retval - may run out of memory! */

    if((exit_status = pvoc_process_addon(*samps_so_far,dz)) < 0)
//TW Bare messages break sound Loom messaging system
        return exit_status;
    samps_so_far += dz->total_samps_written;

    /*RWD Feb 2004 */
    if(dz->ifd[0] >= 0){
        strcpy(pfname,snd_getfilename(dz->ifd[0]));
        if(sndcloseEx(dz->ifd[0]) < 0) {
            sprintf(errstr,"Can't close intermediate analfile %s\n",orig_outfilename);
            return(SYSTEM_ERROR);
        }
    }
    dz->ifd[0] = -1;
    if(remove(/*orig_outfilename*/pfname)<0) {
        fprintf(stdout,"WARNING: Can't remove temporary analysis file %s\n",/*orig_outfilename*/pfname);
        fflush(stdout);
    }
    sndcloseEx(dz->ofd);
    reset_file_params_for_analout(orig_proctype,orig_srate,orig_chans,orig_stype,dz);
    dz->infilecnt    = orig_infilecnt;
    return(FINISHED);
}

/****************************** HAMMING ******************************/
#if 0
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
#endif
/*RWD 22/03/18 maybe need to use this? */
void vonhann(float *win,int winLen,int even)
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
    else{    *(win) = 1.0f;
        for (i=1; i<=winLen; i++)
            *(win+i) =(float)(.5 + .5 *cos(ftmp*(double)i));
    }
}


/****************************** FLOAT_ARRAY ******************************/

int pvoc_float_array(int nnn,float **ptr)
{   /* set up a floating point array length nnn. */
    *ptr = (float *) calloc(nnn,sizeof(float));
    if(*ptr==NULL){
        sprintf(errstr,"pvoc: insufficient memory\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/****************************** PVOC_PROCESS_ADDON ******************************/

int pvoc_process_addon(unsigned int samps_so_far,dataptr dz)
{
    int exit_status;
    int num_overflows = 0;
    int samptime = SAMP_TIME_STEP;
    float   *input,         /* pointer to start of input buffer */
            *output,        /* pointer to start of output buffer */
            *anal,          /* pointer to start of analysis buffer */
            *syn,           /* pointer to start of synthesis buffer */
//          *banal,         /* pointer to anal[1] (for FFT calls) */
            *bsyn,          /* pointer to syn[1]  (for FFT calls) */
//          *nextIn,        /* pointer to next empty word in input */
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
            maxsample = 0.0, minsample = 0.0, biggest;

    int     M = 0,          /* length of analWindow impulse response */
            D = 0,          /* decimation factor */
            I = 0,          /* interpolation factor (default will be I=D)*/
            pvoc_chans = dz->infile->channels - 2,
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
            //RoverTwoPi,     /* R/D divided by 2*Pi */
            TwoPioverR,     /* 2*Pi divided by R/I */
            sum,            /* scale factor for renormalizing windows */
            //rIn,            /* decimated sampling rate */
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
    float   arate;      /* sample rate for header on stdout if -A */

    /*RWD*/
    float F = 0.0f;

#if defined(__SC__) && defined(SOFT_FP)
    extern int _8087;
    _8087 = 0;
#endif

    {
        char *mem;

        if((mem = malloc(64*SECSIZE)) == 0
//TW dz->snd_ifd no longer exists
           /* RWD Nov 2011 not allowed to call sndsetbuf these days! */
    /*  ||sndsetbuf(dz->ifd[0], mem, 64) < 0 */ ) {        /*RWD*/
            sprintf(errstr, "pvoc: Can't set big input buffer\n");
            return(MEMORY_ERROR);
        }
    }

    isr      = dz->infile->origrate;
    arate    = dz->infile->arate;
    M        = dz->infile->Mlen;
    D        = dz->infile->Dfac;
    R        = ((float) D * arate);

    if(flteq(R,0.0)) {
        sprintf(errstr,"Problem: zero sampling rate\n");
        return(DATA_ERROR);
    }

    N2 = pvoc_chans / 2;

    F = /*(int)*/(R /(float)pvoc_chans);

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

    if((exit_status = sndwrite_header(R,dz))<0)
        return(exit_status);

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

    if((exit_status = pvoc_float_array(M+Mf,&analWindow))<0)
        return(exit_status);
    analWindow += (analWinLen = M/2);

    //hamming(analWindow,analWinLen,Mf);
    vonhann(analWindow,analWinLen,Mf);
    
    
    for (i = 1; i <= analWinLen; i++)
        *(analWindow - i) = *(analWindow + i - Mf);

    if (M > pvoc_chans) {
        if (Mf)
        *analWindow *=(float)
        ((double)pvoc_chans*sin((double)PI*.5/pvoc_chans)/(double)(PI*.5));
        for (i = 1; i <= analWinLen; i++) 
            *(analWindow + i) *=(float)
            ((double)pvoc_chans * sin((double) (PI*(i+.5*Mf)/pvoc_chans)) / (PI*(i+.5*Mf)));  /*RWD*/
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

    if((exit_status = pvoc_float_array(M+Mf,&synWindow))<0)
        return(exit_status);
    synWindow += (synWinLen = M/2);

    if (M <= pvoc_chans){
        //hamming(synWindow,synWinLen,Mf);
        vonhann(synWindow,synWinLen,Mf);
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
        //hamming(synWindow,synWinLen,Mf);
        vonhann(synWindow,synWinLen,Mf);
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

    if((exit_status = pvoc_float_array(ibuflen,&input))<0)
        return(exit_status);

//  nextIn = input;

    /* set up output buffer:  nextOut always points to the next word
        to be shifted out.  The shift is simulated by writing the
        value to the standard output and then setting that word
        of the buffer to zero.  When nextOut reaches the end of
        the buffer, it jumps back to the beginning.  */

    if((exit_status = pvoc_float_array(obuflen,&output))<0)
        return(exit_status);

    nextOut = output;
    /* set up analysis buffer for (N/2 + 1) channels: The input is real,
        so the other channels are redundant. oldInPhase is used
        in the conversion to remember the previous phase when
        calculating phase difference between successive samples. */

    if((exit_status = pvoc_float_array(pvoc_chans+2,&anal))<0)
        return(exit_status);

    if((exit_status = pvoc_float_array(N2+1,&oldInPhase))<0)
        return(exit_status);
    if((exit_status = pvoc_float_array(N2+1,&maxAmp))<0)
        return(exit_status);
    if((exit_status = pvoc_float_array(N2+1,&avgAmp))<0)
        return(exit_status);
    if((exit_status = pvoc_float_array(N2+1,&avgFrq))<0)
        return(exit_status);
    if((exit_status = pvoc_float_array(N2+1,&env))<0)
        return(exit_status);

    /* set up synthesis buffer for (pvoc_chans/2 + 1) channels: (This is included
        only for clarity.)  oldOutPhase is used in the re-
        conversion to accumulate angle differences (actually angle
        difference per second). */

    if((exit_status = pvoc_float_array(NO+2,&syn))<0)
        return(exit_status);
    bsyn = syn + 1;

    if((exit_status = pvoc_float_array(NO2+1,&oldOutPhase))<0)
        return(exit_status);

    /* initialization: input time starts negative so that the rightmost
        edge of the analysis filter just catches the first non-zero
        input samples; output time is always T times input time. */

    outCount = 0;
    //rIn  = (float)(R/(float)D);
    rOut = (float)(R/(float)I);
    //RoverTwoPi = (float)(rIn/TWOPI);
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
            if ((rv = fgetfloat((anal+i),dz->ifd[0])) <= 0){
                goto epilog;
            }
        }
#else
//TW snd_ibuf no longer exists
        if((i = fgetfbufEx(anal, pvoc_chans+2, dz->ifd[0],0)) < 0) {       /*RWD*/
            sfperror("pvoc: read error: ");
            return(PROGRAM_ERROR);
        }
        if(i < pvoc_chans+2)
            goto epilog;
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

        if((exit_status = reals_(syn,bsyn,NO2,2))<0)
            return(exit_status);
        if((exit_status = fft_(syn,bsyn,1,NO2,1,2))<0)
            return(exit_status);

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
            fputfloat(nextOut,dz->ofd);
            *(nextOut++) = 0.;
            if (nextOut >= (output + obuflen))
                nextOut -= obuflen;
            outCount++;
        }
#else
        for (i = 0; i < IOi;) { /* shift out next IOi values */
            int j;
            int todo = min(IOi-i, output+obuflen-nextOut);
                if((exit_status = outfloats(nextOut,&maxsample,&minsample,&num_overflows,todo, dz))<0)
                return(exit_status);
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


        if(nI > samptime && (exit_status = pvoc_time_display(nI,samps_so_far,isr,&samptime,dz))<0)
            return(exit_status);
    }   /* End of main while loop */

    nMaxOut = endsamp;
    while (outCount <= nMaxOut){
#ifdef SINGLE_SAMP
        outCount++;
        fputfloat(nextOut++,dz->ofd);
        if (nextOut >= (output + obuflen))
            nextOut -= obuflen;
#else
        int todo = min(nMaxOut-outCount, output+obuflen-nextOut);
        if(todo == 0)
            break;
        if((exit_status = outfloats(nextOut,&maxsample,&minsample,&num_overflows,todo, dz))<0)
            return(exit_status);
        outCount += todo;
        nextOut += todo;
        if (nextOut >= (output + obuflen))
            nextOut -= obuflen;
#endif
    }
    if((exit_status = pvoc_time_display((int)endsamp,samps_so_far,isr,&samptime,dz))<0)
        return(exit_status);

epilog:

#ifndef NOOVERCHK
    if(num_overflows > 0) {
        biggest =  maxsample;
        if(-minsample > maxsample)
            biggest = -minsample;
        fprintf(stdout, "WARNING: %d samples overflowed, and were clipped\n",num_overflows);
        fprintf(stdout, "WARNING: maximum sample was %f  :  minimum sample was %f\n",maxsample,minsample);
        fprintf(stdout, "WARNING: You should reduce source level to avoid clipping: use gain of <= %lf\n",1.0/biggest);
        fflush(stdout);
    }

#endif
    return(FINISHED);
}

/*MCA
 *  Convert floats -> shorts explicitly, since we are compiled with
 *  hardware FP(probably), and the sound filing system is not!
 *  (even without this, it should be more efficient!)
 */
//TW NO CONVERSION TO FLOATS NEEDED 2002
int outfloats(float *nextOut,float *maxsample,float *minsample,int *num_overflows,int todo, dataptr dz)
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
            sprintf(errstr, "pvoc: can't allocate output buffer\n");
            return(MEMORY_ERROR);
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

//  if(fputfbufEx(sbuf, todo, ofd) < todo) {
    if(todo > 0) {
        if(write_samps(sbuf, todo, dz) < 0) {
            sfperror("pvoc: write error");
            return(SYSTEM_ERROR);;
        }
    }
//TW UPDATE
    return FINISHED;
}

/************************************ SNDWRITE_HEADER ************************************/
 /*RWD changed dz->ofd to to dz->ofd */
int sndwrite_header(float R,dataptr dz)
{
    int Nchans = 1;
    int isr = (int)R;
    if(sndputprop(dz->ofd,"sample rate",(char *)&isr,sizeof(int)) < 0 )
        fprintf(stdout,"WARNING: failed to write sample rate\n");
    if(sndputprop(dz->ofd,"channels",(char *)&Nchans,sizeof(int)) < 0 )
        fprintf(stdout,"WARNING: failed to write channel data\n");
    fflush(stdout);
    return(FINISHED);
}

/*********************************** PVOC_TIME_DISPLAY ***********************************/

int pvoc_time_display(int nI,unsigned int samps_so_far,int srate,int *samptime,dataptr dz)
{
    int true_chans, true_srate;
    true_chans = dz->infile->channels;
    true_srate = dz->infile->srate;
    dz->infile->channels = 1;
    dz->infile->srate    = srate;
    display_virtual_time(nI + samps_so_far,dz); 
    dz->infile->channels = true_chans;
    dz->infile->srate    = true_srate;
    *samptime += SAMP_TIME_STEP;
    return(FINISHED);
}

/*void get_name_of_intermediate_file(char *filename,char *nufilename)
{
    char temp[256];
    char *p;
    strcpy(nufilename,filename);
    p = nufilename + strlen(nufilename) - 1;
    while(p > nufilename) {
        if(*p == '/' || *p == '\\' || *p == ':') {
            p = nufilename;
            break;
        }
        if(*p == '.') {
            p--;
            c = *p;
            c += 2;
            *p = c;
            return;
        }
        p--;
    }
    if(p == nufilename) {
        p = nufilename + strlen(nufilename) - 1;
        c = *p;
        c += 2;
        *p = c;
    }
}


*/

void reset_file_params_for_sndout
(int *orig_proctype,int *orig_srate,int *orig_chans,int *orig_stype,dataptr dz) {
    int stype;
    if(dz->floatsam_output==1 )
        stype= SAMP_FLOAT;
    else
        stype = SAMP_SHORT;
    dz->true_outfile_stype = stype;
    
    *orig_proctype = dz->process_type;
    *orig_srate    = dz->outfile->srate;
    *orig_chans    = dz->outfile->channels;
    *orig_stype   = dz->outfile->stype;
    dz->process_type = UNEQUAL_SNDFILE;
    dz->outfile->srate    = dz->infile->srate    = dz->iparam[SS_SRATE];
    dz->outfile->channels = MONO;
    dz->outfile->stype    = /*  dz->outfile->stype = */ stype;    /* RWD: should that be infile->stype? */
}

void reset_file_params_for_analout
(int orig_proctype,int orig_srate,int orig_chans,int orig_stype,dataptr dz) {
    dz->process_type = orig_proctype;
    dz->outfile->srate    = dz->infile->srate    = orig_srate;
    dz->outfile->channels = orig_chans;
    dz->outfile->stype    = /* dz->outfile->stype = */ orig_stype;

}
