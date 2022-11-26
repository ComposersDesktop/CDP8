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
#include <math.h>
#include <time.h>
#include <memory.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <cdpmain.h>
#include <modify.h>
#include <modeno.h>
#include <pnames.h>
#include <flags.h>
#include <arrays.h>
#include <sfsys.h>
#include <cdplib.h>
#include <osbind.h>

/*RWD January 2009:
   delay:
       replaced domono and dostereo with domulti  for full m/c support
       corrected trailtime calc  (counts frames, not samples )
   vdelay:
        replaced do_vdelay with do_vdelay_multi, ditto
        replaced triangle lfo with sine lfo
        revised  random mod to use modulated  sine osc
        randfac revised to use 1/f  noise (average of successive  rands)
   stadium:
        added error return if inout more than 2 chans
*/

#ifdef unix
#define round(x) lround((x))
#endif

#define DELAY_BUFLEN    (4096)
#define ROOTLEN_DELBUF  (2)
#define ROOTLEN_VDELBUF (4)

/*RWD January 2009 */
static int  domulti(int trailtime,int delay,double gain,double fdbk,double fdfrwd,double prescale,dataptr dz);
int do_vdelay_multi(int trailtime,int delay,double gain,double fdbk,double fdfrwd,double prescale,
double modlen,double lfophase,int mdelay,dataptr dz);
static int  count(int num,int *counter);
static double randfac(double length,double phase,int delay,int *ikindex,int *counter,double *rout,double *diff);
static int domultiv(int trailtime,int delay,double prescale,dataptr dz);

static int  setup_stadium(dataptr dz);
static int  generate_delays(dataptr dz);
static void generate_gains(dataptr dz);

#ifndef M_PI
#define M_PI  (3.1415926535897932)
#endif

#ifndef M_TWOPI
#define M_TWOPI (2.0 * M_PI)
#endif

typedef  struct soscil_
{
    double phase;
    double sr;   
    double incr;
    double val;
    unsigned int count;
    unsigned int delay;
} SOSCIL;

SOSCIL*  new_soscil(double srate,double startphase, double  freq,unsigned int delay)
{
    SOSCIL*  osc = (SOSCIL*) malloc(sizeof(SOSCIL));
    if(osc==NULL)
        return NULL;
    osc->sr = srate;
    osc->phase = M_TWOPI * startphase;
    osc->incr =  M_TWOPI/srate * freq;
    osc->delay = delay;
    osc->count = 0;
    osc->val = sin(osc->phase);
    return osc;
}

double  soscil_tick(SOSCIL* osc)
{
    if(osc->count >= osc->delay){
        osc->phase+= osc->incr;
        if(osc->phase >= M_PI)
            osc->phase -= M_TWOPI;
        osc->val = sin(osc->phase);
    }
    else
        osc->count++;
    return osc->val;
}

double  soscil_tickf(SOSCIL* osc,double freqmod)
{
    double nuincr;
    if(osc->count >= osc->delay){
        nuincr =  osc->incr * freqmod;
        osc->phase+= nuincr;
        if(osc->phase >= M_PI)
            osc->phase -= M_TWOPI;
        osc->val = sin(osc->phase);
    }
    else
        osc->count++;
    return osc->val;
}

/********************************** DELAY_PREPROCESS *****************************/

int delay_preprocess(dataptr dz)
{
    int exit_status;      /*RWD*/

    double sr;
    switch(dz->mode) {
    case(MOD_DELAY):
        dz->param[DELAY_INVERT] = 1.0;
        if(dz->vflag[DELAY_INVERT_FLAG])
            dz->param[DELAY_INVERT] = -1.0;
        break;
    case(MOD_VDELAY):
        sr = (double)dz->infile->srate;
        if(dz->param[DELAY_LFOFRQ]<0.0)
            dz->iparam[DELAY_LFOFLAG] = 0;
        else
            dz->iparam[DELAY_LFOFLAG] = 1;
        dz->param[DELAY_MODLEN] = 0.0;
        if(dz->param[DELAY_LFOFRQ] !=0.0)
            dz->param[DELAY_MODLEN] = sr/fabs(dz->param[DELAY_LFOFRQ]);
        dz->iparam[DELAY_MDELAY] = (int)(sr*dz->param[DELAY_LFODELAY]);
        break;
        /*RWD create outfile here in this case: props changed */
    case(MOD_STADIUM):
        /* create stereo outfile here! */
        /* RWD 4:2002  now we can open outfile with corect params! */
        dz->infile->channels = STEREO;  /* ARRGH! */
        if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
            return(exit_status);
        break;
    }
    return(FINISHED);
}

/********************************** CREATE_DELAY_BUFFERS *****************************/

int create_delay_buffers(dataptr dz)    /* USES small bufs to allow max space for delay buffer */
{
    int bigbufsize;

    if(dz->mode==MOD_STADIUM)
        return create_stadium_buffers(dz);
/* MULTICHAN 2009 --> */
//  bigbufsize = DELAY_BUFLEN * 2;
    bigbufsize = DELAY_BUFLEN * dz->infile->channels * 2;
    if((dz->bigbuf = (float *)malloc(bigbufsize * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    bigbufsize /= 2;
    dz->buflen = bigbufsize;
    dz->sampbuf[0] = dz->bigbuf;
    dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
    return(FINISHED);
}


/********************************** DO_DELAY *****************************/

int do_delay(dataptr dz)
{
    int exit_status = FINISHED;
    double rescale, gain, fdbk, fdfrwd, prescale;
    double sr = (double)dz->infile->srate;
    int chans = dz->infile->channels, root_delay = 0;
    int trailtime, delay, delbuflen;
    time_t longtime;
    delay = round(dz->param[DELAY_DELAY] * sr * MS_TO_SECS);
/* nb delaysamps is per channel here    */
/*  fdfrwd = ((1.0 - dz->param[DELAY_MIX])*dz->param[DELAY_INVERT]);    */
    if((exit_status = read_values_from_all_existing_brktables(0.0,dz))<0)
        return(exit_status);                        /* read value 1 bloklength ahead */
    fdfrwd = 1.0 - dz->param[DELAY_MIX];
    gain = dz->param[DELAY_MIX];
    fdbk    =  dz->param[DELAY_FEEDBACK];
    rescale = (1.0 / (fdbk + 1.0)); /* i/p compensation */
    prescale = dz->param[DELAY_PRESCALE] * rescale;
    fdfrwd /= rescale;
    trailtime = round(dz->param[DELAY_TRAILTIME] * sr);
    switch(dz->mode) {
    case(MOD_DELAY):
        /* RWD Jan 2009:  trailtime counts frames, not samples */
    /*  if(chans==STEREO) 
            trailtime *= 2;
    */
        delay *= chans;
        root_delay = ROOTLEN_DELBUF;
        root_delay *= chans;        /*RWD Jan 2009 */
        /*RWD */
        fdfrwd *= dz->param[DELAY_INVERT];
        break;
    case(MOD_VDELAY):
        delay *= chans;
        root_delay = ROOTLEN_VDELBUF;
        root_delay *= chans;
//TW added, following RWD's modifications above
        fdfrwd *= dz->param[DELAY_INVERT];
        break;
    }
    delbuflen = delay + root_delay;
    if((dz->parray[DELAY_BUF] = (double *)calloc(delbuflen,sizeof(double)))==NULL){
       sprintf(errstr,"INSUFFICIENT MEMORY for main delay buffer.");
       return(MEMORY_ERROR);
    }

    switch(dz->mode) {
    case(MOD_DELAY):
        if(dz->brksize[DELAY_MIX] > 0 || dz->brksize[DELAY_FEEDBACK] > 0)
            exit_status = domultiv(trailtime,delay,prescale,dz);
        else
            exit_status = domulti(trailtime,delay,gain,fdbk,fdfrwd,prescale,dz);
        break;
    case(MOD_VDELAY):
        if(dz->iparam[DELAY_SEED]==0) {
            time(&longtime);
            dz->iparam[DELAY_SEED] = (int)longtime;
        }
#if defined(_WIN32) || defined(__SC__)
        srand(dz->iparam[DELAY_SEED]);
#else
        srand48(dz->iparam[DELAY_SEED]);
#endif
        dz->param[DELAY_MODRANGE] = 0.0;        
        if(dz->param[DELAY_LFOMOD]>0.0) {
            dz->param[DELAY_MODRANGE] = (double) (delay/chans) * dz->param[DELAY_LFOMOD];
            if(dz->param[DELAY_MODRANGE] >= (double)(delay/chans))      /* never exactly 100% mod */
                dz->param[DELAY_MODRANGE] = (double)(delay/chans) - 1.0;
        }
        /*RWD Jan 2009 */
        exit_status = do_vdelay_multi(trailtime,delay,gain,fdbk,fdfrwd,prescale,
                        dz->param[DELAY_MODLEN],dz->param[DELAY_LFOPHASE],dz->iparam[DELAY_MDELAY],dz);
        break;
    }
    return(exit_status);    
}

/************************************* DOMONO ************************************/

int domono(int trailtime,int delay,double gain,double fdbk,double fdfrwd,double prescale,dataptr dz)
{
    int exit_status;
    register int i; 
    double  input,output;
    int ipptr=0,opptr=0;
    float   *inbuf  = dz->sampbuf[0];
    float   *outbuf = dz->sampbuf[1];
    double  *delbuf = dz->parray[DELAY_BUF];
    while(dz->samps_left > 0) {
        if((exit_status = read_samps(inbuf,dz))<0)
            return(exit_status);
        for(i=0;i<dz->ssampsread;i++) {
            input = (double)inbuf[i] * prescale;
            output = (input * fdfrwd) + (delbuf[opptr] * gain);
            delbuf[ipptr] = delbuf[opptr++] * fdbk; 
            delbuf[ipptr++] += input;
            outbuf[i] = (float) output;
            if(ipptr >= delay) 
                ipptr -= delay;
            if(opptr >= delay) 
                opptr -= delay;
            if(ipptr < 0 || opptr < 0) {
                sprintf(errstr,"Internal error, ipptr=%d,opptr=%d\n",ipptr,opptr);
                return(PROGRAM_ERROR);
            }
        }
        if(dz->ssampsread == dz->buflen) {     /* If full buffer, write it */
            if((exit_status = write_samps(outbuf,dz->ssampsread,dz))<0)
                return(exit_status); 
        }
    }
    i = dz->ssampsread % dz->buflen;        /* i = position in buffer if not written, or 0 if just written */
            /* now do trailer   */
    while(trailtime>0) {
        for(;i<dz->buflen;i++) {
            output=(delbuf[opptr] * gain);
            delbuf[ipptr++] = delbuf[opptr++] *fdbk;
            outbuf[i] = (float)output;
            if(ipptr >= delay) 
                ipptr -= delay;
            if(opptr >= delay) 
                opptr -= delay;
            if(--trailtime <=0)     /* if got to end of trailer, break out */
                break;
        }
        if(trailtime > 0) {         /* if not at end of trailer, but at end of buf,  write buf */
            if((exit_status = write_samps(outbuf,dz->buflen,dz))<0)
                return(exit_status); 
            i = 0;                  /* and reset bufptr to 0 */
        }
    }       /* if no trailer or at end of trailer, and not at start of buf, write rest of buf */
    if(i>0 && (exit_status = write_samps(outbuf, i, dz))<0)
        return(exit_status); 
    return(FINISHED);
}                   

/************************************* DOSTEREO ************************************/

int dostereo(int trailtime,int delay,double gain,double fdbk,double fdfrwd,double prescale,dataptr dz)
{
    int exit_status;
    int     i;  
    double  Linput,Rinput;
    double  Loutput,Routput;
    int Lipptr = 0,Ripptr = 1;
    int Lopptr = 0,Ropptr = 1;
    double  *delbuf = dz->parray[DELAY_BUF];
    float   *inbuf = dz->sampbuf[0];
    float   *outbuf= dz->sampbuf[1];

    while(dz->samps_left > 0) {
        if((exit_status = read_samps(inbuf,dz))<0)
            return(exit_status);
        for(i=0;i<(dz->ssampsread-1);i+=2) {
            Linput = inbuf[i]*prescale;
            Rinput = inbuf[i+1]*prescale;
            Loutput=(Linput*fdfrwd) + (delbuf[Lopptr]*gain);
            Routput=(Rinput*fdfrwd) + (delbuf[Ropptr]*gain);
            delbuf[Lipptr] = delbuf[Lopptr]*fdbk;
            Lopptr+=2;
            delbuf[Ripptr] = delbuf[Ropptr]*fdbk;
            Ropptr+=2;
            delbuf[Lipptr]+=Linput;
            delbuf[Ripptr]+=Rinput;
            Lipptr+=2;
            Ripptr+=2;
            outbuf[i] = (float) Loutput;
            outbuf[i+1] = (float) Routput;

            if(Lipptr >= delay)     Lipptr -= delay;
            if(Ripptr >= delay)     Ripptr -= delay;
            if(Lopptr >= delay)     Lopptr -= delay;
            if(Ropptr >= delay)     Ropptr -= delay;
        }
        if(dz->ssampsread == dz->buflen) {
            if((exit_status = write_samps(outbuf,dz->ssampsread,dz))<0)
                return(exit_status); 
        }
    }
    i = dz->ssampsread % dz->buflen;
            /* now do trailer   */
    while(trailtime>0) {
        for(;i<dz->buflen-1;i+=2) {
            Loutput = delbuf[Lopptr]*gain;
            Routput = delbuf[Ropptr]*gain;
            delbuf[Lipptr] = delbuf[Lopptr]*fdbk;
            delbuf[Ripptr] = delbuf[Ropptr]*fdbk;
            Lopptr+=2;
            Ropptr+=2;
            Lipptr+=2;
            Ripptr+=2;
            outbuf[i] = (float) Loutput;
            outbuf[i+1] = (float) Routput;
            if(Lipptr >= delay) Lipptr -= delay;
            if(Ripptr >= delay) Ripptr -= delay;
            if(Lopptr >= delay) Lopptr -= delay;
            if(Ropptr >= delay) Ropptr -= delay;
            if(--trailtime <=0)
                break;
        }
        if(trailtime > 0) {
            if((exit_status = write_samps(outbuf,dz->buflen,dz))<0)
                return(exit_status); 
            i = 0;
        }
    }
    if(i>0 && (exit_status = write_samps(outbuf,i ,dz))<0)
        return(exit_status); 
    return(FINISHED);
}

/*RWD Jan 2009  multichannel version */
int domulti(int trailtime,int delay,double gain,double fdbk,double fdfrwd,double prescale,dataptr dz)
{
    int exit_status;
    int     i,j;        
    double* inputs;
    double* outputs;    
    int* lipptr;
    int*  lopptr;
    int chans = dz->infile->channels;
    int adjust = chans-1;
    double  *delbuf = dz->parray[DELAY_BUF];
    float   *inbuf = dz->sampbuf[0];
    float   *outbuf= dz->sampbuf[1];

    inputs  = (double*) malloc(chans * sizeof(double));
    outputs = (double*) malloc(chans * sizeof(double));
    lipptr  = (int*)   malloc(chans * sizeof(int));
    lopptr  = (int*)   malloc(chans * sizeof(int));
    for(i=0;i < chans;i++){
        inputs[i] = outputs[i] = 0.0;
        lipptr[i] = lopptr[i] = i;
    }

    while(dz->samps_left > 0) {
        if((exit_status = read_samps(inbuf,dz))<0)
            return(exit_status);
        for(i=0;i < dz->ssampsread-adjust; i+=chans) {
            for(j=0;j < chans;j++){
                inputs[j] = inbuf[i+j]*prescale;                
                outputs[j] =(inputs[j]*fdfrwd) + (delbuf[lopptr[j]] * gain);                
                delbuf[lipptr[j]] = delbuf[lopptr[j]]*fdbk;
                lopptr[j] += chans;             
                delbuf[lipptr[j]] += inputs[j];             
                lipptr[j] += chans;             
                outbuf[i+j] = (float) outputs[j];               
                if(lipptr[j] >= delay)  
                    lipptr[j] -= delay;             
                if(lopptr[j] >= delay) 
                    lopptr[j] -= delay;             
            }
        }
        if(dz->ssampsread == dz->buflen) {
            if((exit_status = write_samps(outbuf,dz->ssampsread,dz))<0)
                return(exit_status); 
        }
    }
    i = dz->ssampsread % dz->buflen;
            /* now do trailer   */
    while(trailtime>0) {
        for(;i<dz->buflen-adjust;i+=chans) {
            for(j=0;j < chans;j++){
                outputs[j] = delbuf[lopptr[j]]*gain;                
                delbuf[lipptr[j]] = delbuf[lopptr[j]] * fdbk;               
                lopptr[j]+=chans;               
                lipptr[j]+=chans;               
                outbuf[i+j] = (float) outputs[j];               
                if(lipptr[j] >= delay) lipptr[j] -= delay;              
                if(lopptr[j] >= delay) lopptr[j] -= delay;              
            }
            if(--trailtime <=0)
                break;
        }
        if(trailtime > 0) {
            if((exit_status = write_samps(outbuf,dz->buflen,dz))<0)
                return(exit_status); 
            i = 0;
        }
    }
    if(i>0 && (exit_status = write_samps(outbuf,i ,dz))<0)
        return(exit_status); 
    free(inputs);
    free(outputs);
    free(lipptr);
    free(lopptr);

    return(FINISHED);
}

/*TW 2010: variable feedback, feedfrwd version */
int domultiv(int trailtime,int delay,double prescale,dataptr dz)
{
    int exit_status;
    int     i,j;        
    double* inputs;
    double* outputs;    
    int* lipptr;
    int*  lopptr;
    int chans = dz->infile->channels;
    int adjust = chans-1;
    double  *delbuf = dz->parray[DELAY_BUF];
    float   *inbuf = dz->sampbuf[0];
    float   *outbuf= dz->sampbuf[1];
    int vstep = 128 * chans, samps_processed = 0;
    double srate = (double)dz->infile->srate, thistime;
    double gain, fdbk, fdfrwd;
    if((exit_status = read_values_from_all_existing_brktables(0.0,dz))<0)
        return(exit_status);
    fdfrwd = 1.0 - dz->param[DELAY_MIX];
    gain   = dz->param[DELAY_MIX];
    fdbk   = dz->param[DELAY_FEEDBACK];

    inputs  = (double*) malloc(chans * sizeof(double));
    outputs = (double*) malloc(chans * sizeof(double));
    lipptr  = (int*)   malloc(chans * sizeof(int));
    lopptr  = (int*)   malloc(chans * sizeof(int));
    for(i=0;i < chans;i++){
        inputs[i] = outputs[i] = 0.0;
        lipptr[i] = lopptr[i] = i;
    }

    while(dz->samps_left > 0) {
        if((exit_status = read_samps(inbuf,dz))<0)
            return(exit_status);
        for(i=0;i < dz->ssampsread-adjust; i+=chans) {
            if(samps_processed % vstep == 0) {
                thistime = (double)(samps_processed/chans)/srate;
                if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
                    return(exit_status);
                fdfrwd = 1.0 - dz->param[DELAY_MIX];
                gain   = dz->param[DELAY_MIX];
                fdbk   = dz->param[DELAY_FEEDBACK];
            }
            for(j=0;j < chans;j++){
                inputs[j] = inbuf[i+j]*prescale;                
                outputs[j] =(inputs[j]*fdfrwd) + (delbuf[lopptr[j]] * gain);                
                delbuf[lipptr[j]] = delbuf[lopptr[j]]*fdbk;
                lopptr[j] += chans;             
                delbuf[lipptr[j]] += inputs[j];             
                lipptr[j] += chans;             
                outbuf[i+j] = (float) outputs[j];               
                if(lipptr[j] >= delay)  
                    lipptr[j] -= delay;             
                if(lopptr[j] >= delay) 
                    lopptr[j] -= delay;             
            }
            samps_processed += chans;
        }
        if(dz->ssampsread == dz->buflen) {
            if((exit_status = write_samps(outbuf,dz->ssampsread,dz))<0)
                return(exit_status); 
        }
    }
    i = dz->ssampsread % dz->buflen;
            /* now do trailer   */
    while(trailtime>0) {
        for(;i<dz->buflen-adjust;i+=chans) {
            for(j=0;j < chans;j++){
                outputs[j] = delbuf[lopptr[j]]*gain;                
                delbuf[lipptr[j]] = delbuf[lopptr[j]] * fdbk;               
                lopptr[j]+=chans;               
                lipptr[j]+=chans;               
                outbuf[i+j] = (float) outputs[j];               
                if(lipptr[j] >= delay) lipptr[j] -= delay;              
                if(lopptr[j] >= delay) lopptr[j] -= delay;              
            }
            if(--trailtime <=0)
                break;
        }
        if(trailtime > 0) {
            if((exit_status = write_samps(outbuf,dz->buflen,dz))<0)
                return(exit_status); 
            i = 0;
        }
    }
    if(i>0 && (exit_status = write_samps(outbuf,i ,dz))<0)
        return(exit_status); 
    free(inputs);
    free(outputs);
    free(lipptr);
    free(lopptr);

    return(FINISHED);
}
 
/************************************* DO_VDELAY_MULTI ************************************/

int do_vdelay_multi(int trailtime,int delay,double gain,double fdbk,double fdfrwd,double prescale,
double modlen,double lfophase,int mdelay,dataptr dz)
{
    int exit_status;
    float  *inbuf = dz->sampbuf[0];
    float  *obuf  = dz->sampbuf[1];
    int     i,j,chans =  dz->infile->channels;
    int adjust = chans-1;
    int    *ipptr;
    int    *opptr;
    int    optr1, optr2;
    int    framedelay = delay/chans;
    double  val, offset;
    double  *input;
    double  *output;
    double  *delbuf = dz->parray[DELAY_BUF];
    double  *dinterp;
    double  rout = 0.0, diff = 0.0;
    int     ikindex= 0, counter = 0;
    SOSCIL*  osc;

    ipptr = (int*) malloc(chans * sizeof(int));
    opptr = (int*) malloc(chans * sizeof(int));
    input = (double*) malloc(chans * sizeof(double));
    output = (double*) malloc(chans * sizeof(double));
    dinterp = (double*) malloc(chans * sizeof(double));
    for(j=0;j < chans;j++){
        ipptr[j] = opptr[j] = j;        
        dinterp[j] = 0.0;
        input[j] = output[j] = 0.0;
    }

    osc = new_soscil(dz->infile->srate,lfophase,dz->param[DELAY_LFOFRQ],(unsigned int) mdelay);
    if(osc==NULL){
        return MEMORY_ERROR;
    }

    while(dz->samps_left > 0) {

        if((exit_status = read_samps(inbuf,dz))<0)
            return(exit_status);
        for(i = 0; i < dz->ssampsread-adjust; i += chans) {
            if(dz->iparam[DELAY_LFOFLAG])           
                val = 0.5 + (0.5 * soscil_tick(osc));
            else  {
                val = randfac(modlen,lfophase,mdelay,&ikindex,&counter,&rout,&diff);
                val = 0.5 + (0.5 * soscil_tickf(osc,val));
            }
            /*  same fac for all chans */
            offset = dz->param[DELAY_MODRANGE] * val;
            for(j=0;j < chans;j++){
                input[j] = ((double) (inbuf[i+j])) * prescale;
                if(ipptr[j] >= framedelay)  
                    ipptr[j] -= framedelay;
                if(opptr[j] >= framedelay)  
                    opptr[j] -= framedelay;
                optr1 = opptr[j] + (int) offset;
                if(optr1 >= framedelay) 
                    optr1 -= framedelay;
                optr2 = optr1 + 1;
                if(optr2 >= framedelay) 
                    optr2 -= framedelay;
                dinterp[j] =(double) delbuf[optr1*chans +j] + 
                    (fmod(offset,1.0)*((double)delbuf[optr2*chans +j] - (double)delbuf[optr1*chans +j]));               
                output[j] = (input[j] * fdfrwd) + (dinterp[j] * gain);
                delbuf[ipptr[j]*chans+j] = (float)(dinterp[j] * fdbk); 
                delbuf[ipptr[j]*chans +j] += (float)input[j] ;
                ipptr[j]++;
                obuf[i+j] = (float)output[j];
                opptr[j]++;                
            }           
        }
        if(dz->ssampsread == dz->buflen) {     /* If full buffer, write it */
            if((exit_status = write_samps(obuf,dz->ssampsread,dz))<0)
                return(exit_status); 
        }
    }

    i = dz->ssampsread % dz->buflen;        /* i = position in buffer if not written, or 0 if just written */
            /* now do trailer   */
    while(trailtime>0) {
        for(; i < dz->buflen-adjust; i+= chans) {
            if(dz->iparam[DELAY_LFOFLAG]) 
             val = 0.5 + (0.5 * soscil_tick(osc));
            else  {
                val = randfac(modlen,lfophase,mdelay,&ikindex,&counter,&rout,&diff);
                val = 0.5 + (0.5 * soscil_tickf(osc,val));
            }

            offset = dz->param[DELAY_MODRANGE] * val;
            for(j=0; j < chans; j++){
                if(ipptr[j] >= framedelay) 
                    ipptr[j] -= framedelay;
                if(opptr[j] >= framedelay) 
                    opptr[j] -= framedelay;
                optr1 = opptr[j] + (int) offset;
                if(optr1 >= framedelay) 
                    optr1 -= framedelay;
                optr2 = optr1 +1;
                if(optr2 >= framedelay) 
                    optr2 -= framedelay;
                dinterp[j] =(double) delbuf[optr1*chans +j] + 
                    (fmod(offset,1.0)*((double)delbuf[optr2*chans+j] - (double)delbuf[optr1*chans+j]));
                output[j] = (dinterp[j] * gain);
                delbuf[ipptr[j]*chans+j] = (float)(dinterp[j] * fdbk);
                ipptr[j]++;
                obuf[i+j] = (float)output[j];
                opptr[j]++;
            }
            if(--trailtime <=0)     /* if got to end of trailer, break out */
                break;
        }   /* for loop */
        if(trailtime > 0) {         /* if not at end of trailer, but at end of buf,  write buf */
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status); 
            i = 0;                  /* and reset bufptr to 0 */
        }
    }       
    free(osc);
    free(ipptr);
    free(opptr);
    free(input);
    free(output);
    free(dinterp);
    /* if no trailer or at end of trailer, and not at start of buf, write rest of buf */
    if(i>0 && (exit_status = write_samps(obuf,i ,dz))<0)
        return(exit_status); 
    return(FINISHED);
}




/************************************* COUNT ************************************/

int count(int num,int *counter)
{
    if(*counter<=num) {
        num -= (*counter);
        (*counter)++;
        return(num);
    }
    return 0;
}

/************************************* RANDFAC ************************************/

double randfac(double length,double phase,int delay,int *ikindex,int *counter,double *rout,double *diff)
{
    double temp;
    volatile double dummy;
    if(!count(delay,counter)) {
        if(*ikindex==0) {       /* start of cycle */
            dummy = drand48();  /* kludge to kickstart drand48() */
            temp  = drand48();
            // *diff  = *rout - temp;
            /* RWD january 2009 : extended  to  use 1/f random vals */
            *diff = *rout - (0.5 * (dummy + temp));
        }
        *rout -= (*diff)/length;
        (*ikindex)++;
        if(*ikindex>=(int)length) 
            *ikindex = 0;
        return *rout;
    }
    return phase;
}

/************************** STADIUM_PCONSISTENCY ****************************/

int stadium_pconsistency(dataptr dz)
{
    int exit_status;
    dz->itemcnt = dz->infile->channels;
    dz->infile->channels = STEREO;  /* for output!! */
    if((exit_status = setup_stadium(dz))<0)
        return(exit_status);
    if((exit_status = generate_delays(dz))<0)
        return(exit_status);
    dz->iparam[STAD_MAXDELAY] = dz->lparray[STAD_DELAY][dz->iparam[STAD_ECHOCNT]-1];
    generate_gains(dz);
    return(FINISHED);
}

/*************************** SETUP_STADIUM  ***************************/

int setup_stadium(dataptr dz)
{
//TW UPDATE TO 100 possible echoes
//  double *tdelay, *gain, *pan;
    double *tdelay, *gain, *pan, top, bot, interval;
    int n;

    if((dz->parray[STAD_TDELAY] = malloc(MAX_ECHOCNT * sizeof(double)))==NULL
    || (dz->parray[STAD_GAIN]   = malloc(MAX_ECHOCNT * sizeof(double)))==NULL
    || (dz->parray[STAD_GAINL]  = malloc(MAX_ECHOCNT * sizeof(double)))==NULL
    || (dz->parray[STAD_GAINR]  = malloc(MAX_ECHOCNT * sizeof(double)))==NULL
    || (dz->parray[STAD_PAN]    = malloc(MAX_ECHOCNT * sizeof(double)))==NULL) {
        sprintf(errstr,"Insufficient memory for stadium arrays.\n");
        return(MEMORY_ERROR);
    }
    tdelay = dz->parray[STAD_TDELAY];
    gain   = dz->parray[STAD_GAIN];
    pan    = dz->parray[STAD_PAN];
//TW UPDATE TO 10 possible echoes
    if(dz->iparam[STAD_ECHOCNT] <= REASONABLE_ECHOCNT) {
        tdelay[0]  = 0.000000;  gain[0]  = 1.000000;    pan[0]  = -1;
        tdelay[1]  = 0.131832;  gain[1]  = 1.000000;    pan[1]  =  1;
        tdelay[2]  = 0.215038;  gain[2]  = 0.354813;    pan[2]  = .5;
        tdelay[3]  = 0.322274;  gain[3]  = 0.354813;    pan[3]  = -.5;
        tdelay[4]  = 0.414122;  gain[4]  = 0.251189;    pan[4]  = 0;
        tdelay[5]  = 0.504488;  gain[5]  = 0.125893;    pan[5]  = .7;
        tdelay[6]  = 0.637713;  gain[6]  = 0.125893;    pan[6]  = -.7;
        tdelay[7]  = 0.730468;  gain[7]  = 0.063096;    pan[7]  = .3;
        tdelay[8]  = 0.808751;  gain[8]  = 0.063096;    pan[8]  = -.3;
        tdelay[9]  = 0.910460;  gain[9]  = 0.031623;    pan[9]  = .15;
        tdelay[10] = 1.027041;  gain[10] = 0.031623;    pan[10] = -.15;
        tdelay[11] = 1.132028;  gain[11] = 0.012589;    pan[11] = .85;
        tdelay[12] = 1.244272;  gain[12] = 0.012589;    pan[12] = -.85;
        tdelay[13] = 1.336923;  gain[13] = 0.005012;    pan[13] = .4;
        tdelay[14] = 1.427700;  gain[14] = 0.005012;    pan[14] = -.4;
        tdelay[15] = 1.528503;  gain[15] = 0.002512;    pan[15] = .6;
        tdelay[16] = 1.618661;  gain[16] = 0.002512;    pan[16] = -.6;
        tdelay[17] = 1.715413;  gain[17] = 0.002512;    pan[17] = .225;
        tdelay[18] = 1.814730;  gain[18] = 0.002512;    pan[18] = -.225;
        tdelay[19] = 1.914843;  gain[19] = 0.002512;    pan[19] = .775;
        tdelay[20] = 2.000258;  gain[20] = 0.002512;    pan[20] = -.775;
        tdelay[21] = 2.135363;  gain[21] = 0.002512;    pan[21] = .125;
        tdelay[22] = 2.230554;  gain[22] = 0.002512;    pan[22] = -.125;
    } else {
        top = log(1.0);
        bot = log(.0001);
        interval = (top - bot)/(dz->iparam[STAD_ECHOCNT] - 1);
        gain[0] = 1.0;
        for(n=1;n<(dz->iparam[STAD_ECHOCNT] - 1);n++) {
            top -= interval;
            gain[n] = exp(top);
        }
        gain[n] = .0001;
        for(n=0;n<dz->iparam[STAD_ECHOCNT];n++) {
            switch(n) {
            case(0):    tdelay[0] = 0.000000;  pan[0] = -1;     break;
            case(1):    tdelay[1] = 0.131832;  pan[1] = 1;      break;
            case(2):    tdelay[2] = 0.215038;  pan[2] = .5;     break;
            case(3):    tdelay[3] = 0.322274;  pan[3] = -.5;    break;
            case(4):    tdelay[4] = 0.414122;  pan[4] = 0;      break;
            case(5):    tdelay[5] = 0.504488;  pan[5] = .7;     break;
            case(6):    tdelay[6] = 0.637713;  pan[6] = -.7;    break;
            case(7):    tdelay[7] = 0.730468;  pan[7] = .3;     break;
            case(8):    tdelay[8] = 0.808751;  pan[8] = -.3;    break;
            case(9):    tdelay[9] = 0.910460;  pan[9] = .15;    break;
            case(10):   tdelay[10] = 1.027041; pan[10] = -.15;  break;
            case(11):   tdelay[11] = 1.132028; pan[11] = .85;   break;
            case(12):   tdelay[12] = 1.244272; pan[12] = -.85;  break;
            case(13):   tdelay[13] = 1.336923; pan[13] = .4;    break;
            case(14):   tdelay[14] = 1.427700; pan[14] = -.4;   break;
            case(15):   tdelay[15] = 1.528503; pan[15] = .6;    break;
            case(16):   tdelay[16] = 1.618661; pan[16] = -.6;   break;
            case(17):   tdelay[17] = 1.715413; pan[17] = .225;  break;
            case(18):   tdelay[18] = 1.814730; pan[18] = -.225; break;
            case(19):   tdelay[19] = 1.914843; pan[19] = .775;  break;
            case(20):   tdelay[20] = 2.000258; pan[20] = -.775; break;
            case(21):   tdelay[21] = 2.135363; pan[21] = .125;  break;
            case(22):   tdelay[22] = 2.230554; pan[22] = -.125; break;
            case(23):   tdelay[23] = 2.290000; pan[23] = -.65;  break;
            case(24):   tdelay[24] = 2.372498; pan[24] = .65;   break;
            case(25):   tdelay[25] = 2.445365; pan[25] = .35;   break;
            case(26):   tdelay[26] = 2.571897; pan[26] = -.35;  break;
            case(27):   tdelay[27] = 2.627656; pan[27] = -.1;   break;
            case(28):   tdelay[28] = 2.695809; pan[28] = .1;    break;
            case(29):   tdelay[29] = 2.758350; pan[29] = -.8;   break;
            case(30):   tdelay[30] = 2.880984; pan[30] = .8;    break;
            case(31):   tdelay[31] = 2.975910; pan[31] = -.55;  break;
            case(32):   tdelay[32] = 3.043894; pan[32] = .55;   break;
            case(33):   tdelay[33] = 3.143261; pan[33] = -.25;  break;
            case(34):   tdelay[34] = 3.268126; pan[34] = .25;   break;
            case(35):   tdelay[35] = 3.366093; pan[35] = -.45;  break;
            case(36):   tdelay[36] = 3.480461; pan[36] = .45;   break;
            case(37):   tdelay[37] = 3.577198; pan[37] = .05;   break;
            case(38):   tdelay[38] = 3.666871; pan[38] = -.9;   break;
            case(39):   tdelay[39] = 3.779061; pan[39] = -.05;  break;
            case(40):   tdelay[40] = 3.852884; pan[40] = .9;    break;
            case(41):   tdelay[41] = 3.966519; pan[41] = -.425; break;
            case(42):   tdelay[42] = 4.043514; pan[42] = .425;  break;
            case(43):   tdelay[43] = 4.147562; pan[43] = .75;   break;
            case(44):   tdelay[44] = 4.241538; pan[44] = -.75;  break;
            case(45):   tdelay[45] = 4.358808; pan[45] = .275;  break;
            case(46):   tdelay[46] = 4.470554; pan[46] = -.275; break;
            case(47):   tdelay[47] = 4.540000; pan[47] = -.625; break;
            case(48):   tdelay[48] = 4.612729; pan[48] = .625;  break;
            case(49):   tdelay[49] = 4.693888; pan[49] = .375;  break;
            case(50):   tdelay[50] = 4.750479; pan[50] = -.375; break;
            case(51):   tdelay[51] = 4.834122; pan[51] = -.116; break;
            case(52):   tdelay[52] = 4.894456; pan[52] = .116;  break;
            case(53):   tdelay[53] = 4.978112; pan[53] = -.825; break;
            case(54):   tdelay[54] = 5.107199; pan[54] = .825;  break;
            case(55):   tdelay[55] = 5.217554; pan[55] = -.535; break;
            case(56):   tdelay[56] = 5.282567; pan[56] = .535;  break;
            case(57):   tdelay[57] = 5.387678; pan[57] = -.265; break;
            case(58):   tdelay[58] = 5.493642; pan[58] = .265;  break;
            case(59):   tdelay[59] = 5.610009; pan[59] = -.475; break;
            case(60):   tdelay[60] = 5.720302; pan[60] = .475;  break;
            case(61):   tdelay[61] = 5.818331; pan[61] = .11;   break;
            case(62):   tdelay[62] = 5.911186; pan[62] = -.77;  break;
            case(63):   tdelay[63] = 6.013019; pan[63] = -.11;  break;
            case(64):   tdelay[64] = 6.089382; pan[64] = .77;   break;
            case(65):   tdelay[65] = 6.192071; pan[65] = -.475; break;
            case(66):   tdelay[66] = 6.288516; pan[66] = .475;  break;
            case(67):   tdelay[67] = 6.403337; pan[67] = .725;  break;
            case(68):   tdelay[68] = 6.474009; pan[68] = -.725; break;
            case(69):   tdelay[69] = 6.613701; pan[69] = .235;  break;
            case(70):   tdelay[70] = 6.710554; pan[70] = -.235; break;
            case(71):   tdelay[71] = 6.800000; pan[71] = -.685; break;
            case(72):   tdelay[72] = 6.933349; pan[72] = .685;  break;
            case(73):   tdelay[73] = 7.032532; pan[73] = .315;  break;
            case(74):   tdelay[74] = 7.122216; pan[74] = -.315; break;
            case(75):   tdelay[75] = 7.194122; pan[75] = -.136; break;
            case(76):   tdelay[76] = 7.265021; pan[76] = .136;  break;
            case(77):   tdelay[77] = 7.358764; pan[77] = -.635; break;
            case(78):   tdelay[78] = 7.445276; pan[78] = .635;  break;
            case(79):   tdelay[79] = 7.539293; pan[79] = -.435; break;
            case(80):   tdelay[80] = 7.609978; pan[80] = .435;  break;
            case(81):   tdelay[81] = 7.710640; pan[81] = -.765; break;
            case(82):   tdelay[82] = 7.831968; pan[82] = .765;  break;
            case(83):   tdelay[83] = 7.933759; pan[83] = -.575; break;
            case(84):   tdelay[84] = 8.031102; pan[84] = .575;  break;
            case(85):   tdelay[85] = 8.120963; pan[85] = .215;  break;
            case(86):   tdelay[86] = 8.230907; pan[86] = -.692; break;
            case(87):   tdelay[87] = 8.322181; pan[87] = -.215; break;
            case(88):   tdelay[88] = 8.426997; pan[88] = .692;  break;
            case(89):   tdelay[89] = 8.531689; pan[89] = -.435; break;
            case(90):   tdelay[90] = 8.607012; pan[90] = .435;  break;
            case(91):   tdelay[91] = 8.715148; pan[91] = .675;  break;
            case(92):   tdelay[92] = 8.803142; pan[92] = -.675; break;
            case(93):   tdelay[93] = 8.951037; pan[93] = .265;  break;
            case(94):   tdelay[94] = 9.018695; pan[94] = -.265; break;
            case(95):   tdelay[95] = 9.107151; pan[95] = -.555; break;
            case(96):   tdelay[96] = 9.240962; pan[96] = .555;  break;
            case(97):   tdelay[97] = 9.312251; pan[97] = .333;  break;
            case(98):   tdelay[98] = 9.422676; pan[98] = -.333; break;
            case(99):   tdelay[99] = 9.514122; pan[99] = 0.0;   break;
            case(100):  tdelay[100] = 9.637713;  pan[100] = -1;     break;
            case(101):  tdelay[101] = 9.730468;  pan[101] = 1;      break;
            case(102):  tdelay[102] = 9.808751;  pan[102] = .5;     break;
            case(103):  tdelay[103] = 9.910460;  pan[103] = -.5;    break;
            case(104):  tdelay[104] = 10.027041;  pan[104] = 0;     break;
            case(105):  tdelay[105] = 10.132028;  pan[105] = .7;    break;
            case(106):  tdelay[106] = 10.244272;  pan[106] = -.7;   break;
            case(107):  tdelay[107] = 10.336923;  pan[107] = .3;    break;
            case(108):  tdelay[108] = 10.427700;  pan[108] = -.3;   break;
            case(109):  tdelay[109] = 10.528503;  pan[109] = .15;   break;
            case(110):  tdelay[110] = 10.618661; pan[110] = -.15;   break;
            case(111):  tdelay[111] = 10.715413; pan[111] = .85;    break;
            case(112):  tdelay[112] = 10.814730; pan[112] = -.85;   break;
            case(113):  tdelay[113] = 10.914843; pan[113] = .4;     break;
            case(114):  tdelay[114] = 11.000258; pan[114] = -.4;    break;
            case(115):  tdelay[115] = 11.135363; pan[115] = .6;     break;
            case(116):  tdelay[116] = 11.230554; pan[116] = -.6;    break;
            case(117):  tdelay[117] = 11.290000; pan[117] = .225;   break;
            case(118):  tdelay[118] = 11.372498; pan[118] = -.225;  break;
            case(119):  tdelay[119] = 11.445365; pan[119] = .775;   break;
            case(120):  tdelay[120] = 11.571897; pan[120] = -.775;  break;
            case(121):  tdelay[121] = 11.627656; pan[121] = .125;   break;
            case(122):  tdelay[122] = 11.695809; pan[122] = -.125;  break;
            case(123):  tdelay[123] = 11.758350; pan[123] = -.65;   break;
            case(124):  tdelay[124] = 11.880984; pan[124] = .65;    break;
            case(125):  tdelay[125] = 11.975910; pan[125] = .35;    break;
            case(126):  tdelay[126] = 12.043894; pan[126] = -.35;   break;
            case(127):  tdelay[127] = 12.143261; pan[127] = -.1;    break;
            case(128):  tdelay[128] = 12.268126; pan[128] = .1;     break;
            case(129):  tdelay[129] = 12.366093; pan[129] = -.8;    break;
            case(130):  tdelay[130] = 12.480461; pan[130] = .8;     break;
            case(131):  tdelay[131] = 12.577198; pan[131] = -.55;   break;
            case(132):  tdelay[132] = 12.666871; pan[132] = .55;    break;
            case(133):  tdelay[133] = 12.779061; pan[133] = -.25;   break;
            case(134):  tdelay[134] = 12.852884; pan[134] = .25;    break;
            case(135):  tdelay[135] = 12.966519; pan[135] = -.45;   break;
            case(136):  tdelay[136] = 13.043514; pan[136] = .45;    break;
            case(137):  tdelay[137] = 13.147562; pan[137] = .05;    break;
            case(138):  tdelay[138] = 13.241538; pan[138] = -.9;    break;
            case(139):  tdelay[139] = 13.358808; pan[139] = -.05;   break;
            case(140):  tdelay[140] = 13.470554; pan[140] = .9;     break;
            case(141):  tdelay[141] = 13.540000; pan[141] = -.425;  break;
            case(142):  tdelay[142] = 13.612729; pan[142] = .425;   break;
            case(143):  tdelay[143] = 13.693888; pan[143] = .75;    break;
            case(144):  tdelay[144] = 13.750479; pan[144] = -.75;   break;
            case(145):  tdelay[145] = 13.834122; pan[145] = .275;   break;
            case(146):  tdelay[146] = 13.894456; pan[146] = -.275;  break;
            case(147):  tdelay[147] = 13.978112; pan[147] = -.625;  break;
            case(148):  tdelay[148] = 14.107199; pan[148] = .625;   break;
            case(149):  tdelay[149] = 14.217554; pan[149] = .375;   break;
            case(150):  tdelay[150] = 14.282567; pan[150] = -.375;  break;
            case(151):  tdelay[151] = 14.387678; pan[151] = -.116;  break;
            case(152):  tdelay[152] = 14.493642; pan[152] = .116;   break;
            case(153):  tdelay[153] = 14.610009; pan[153] = -.825;  break;
            case(154):  tdelay[154] = 14.720302; pan[154] = .825;   break;
            case(155):  tdelay[155] = 14.818331; pan[155] = -.535;  break;
            case(156):  tdelay[156] = 14.911186; pan[156] = .535;   break;
            case(157):  tdelay[157] = 15.013019; pan[157] = -.265;  break;
            case(158):  tdelay[158] = 15.089382; pan[158] = .265;   break;
            case(159):  tdelay[159] = 15.192071; pan[159] = -.475;  break;
            case(160):  tdelay[160] = 15.288516; pan[160] = .475;   break;
            case(161):  tdelay[161] = 15.403337; pan[161] = .11;    break;
            case(162):  tdelay[162] = 15.474009; pan[162] = -.77;   break;
            case(163):  tdelay[163] = 15.613701; pan[163] = -.11;   break;
            case(164):  tdelay[164] = 15.710554; pan[164] = .77;    break;
            case(165):  tdelay[165] = 15.800000; pan[165] = -.475;  break;
            case(166):  tdelay[166] = 15.933349; pan[166] = .475;   break;
            case(167):  tdelay[167] = 16.032532; pan[167] = .725;   break;
            case(168):  tdelay[168] = 16.122216; pan[168] = -.725;  break;
            case(169):  tdelay[169] = 16.194122; pan[169] = .235;   break;
            case(170):  tdelay[170] = 16.265021; pan[170] = -.235;  break;
            case(171):  tdelay[171] = 16.358764; pan[171] = -.685;  break;
            case(172):  tdelay[172] = 16.445276; pan[172] = .685;   break;
            case(173):  tdelay[173] = 16.539293; pan[173] = .315;   break;
            case(174):  tdelay[174] = 16.609978; pan[174] = -.315;  break;
            case(175):  tdelay[175] = 16.710640; pan[175] = -.136;  break;
            case(176):  tdelay[176] = 16.831968; pan[176] = .136;   break;
            case(177):  tdelay[177] = 16.933759; pan[177] = -.635;  break;
            case(178):  tdelay[178] = 17.031102; pan[178] = .635;   break;
            case(179):  tdelay[179] = 17.120963; pan[179] = -.435;  break;
            case(180):  tdelay[180] = 17.230907; pan[180] = .435;   break;
            case(181):  tdelay[181] = 17.322181; pan[181] = -.765;  break;
            case(182):  tdelay[182] = 17.426997; pan[182] = .765;   break;
            case(183):  tdelay[183] = 17.531689; pan[183] = -.575;  break;
            case(184):  tdelay[184] = 17.607012; pan[184] = .575;   break;
            case(185):  tdelay[185] = 17.715148; pan[185] = .215;   break;
            case(186):  tdelay[186] = 17.803142; pan[186] = -.692;  break;
            case(187):  tdelay[187] = 17.951037; pan[187] = -.215;  break;
            case(188):  tdelay[188] = 18.018695; pan[188] = .692;   break;
            case(189):  tdelay[189] = 18.107151; pan[189] = -.435;  break;
            case(190):  tdelay[190] = 18.240962; pan[190] = .435;   break;
            case(191):  tdelay[191] = 18.312251; pan[191] = .675;   break;
            case(192):  tdelay[192] = 18.422676; pan[192] = -.675;  break;
            case(193):  tdelay[193] = 18.514122; pan[193] = .265;   break;
            case(194):  tdelay[194] = 18.637713; pan[194] = -.265;  break;
            case(195):  tdelay[195] = 18.730468; pan[195] = -.555;  break;
            case(196):  tdelay[196] = 18.808751; pan[196] = .555;   break;
            case(197):  tdelay[197] = 18.910460; pan[197] = .333;   break;
            case(198):  tdelay[198] = 19.000000; pan[198] = -.333;  break;
            case(199):  tdelay[199] = 19.131832; pan[199] = 0.0;    break;
            case(200):  tdelay[200] = 19.215038;  pan[200] = -1;    break;
            case(201):  tdelay[201] = 19.322274;  pan[201] = 1;     break;
            case(202):  tdelay[202] = 19.414122;  pan[202] = .5;    break;
            case(203):  tdelay[203] = 19.504488;  pan[203] = -.5;   break;
            case(204):  tdelay[204] = 19.637713;  pan[204] = 0;     break;
            case(205):  tdelay[205] = 19.730468;  pan[205] = .7;    break;
            case(206):  tdelay[206] = 19.808751;  pan[206] = -.7;   break;
            case(207):  tdelay[207] = 19.910460;  pan[207] = .3;    break;
            case(208):  tdelay[208] = 20.027041;  pan[208] = -.3;   break;
            case(209):  tdelay[209] = 20.132028;  pan[209] = .15;   break;
            case(210):  tdelay[210] = 20.244272; pan[210] = -.15;   break;
            case(211):  tdelay[211] = 20.336923; pan[211] = .85;    break;
            case(212):  tdelay[212] = 20.427700; pan[212] = -.85;   break;
            case(213):  tdelay[213] = 20.528503; pan[213] = .4;     break;
            case(214):  tdelay[214] = 20.618661; pan[214] = -.4;    break;
            case(215):  tdelay[215] = 20.715413; pan[215] = .6;     break;
            case(216):  tdelay[216] = 20.814730; pan[216] = -.6;    break;
            case(217):  tdelay[217] = 20.914843; pan[217] = .225;   break;
            case(218):  tdelay[218] = 21.000258; pan[218] = -.225;  break;
            case(219):  tdelay[219] = 21.135363; pan[219] = .775;   break;
            case(220):  tdelay[220] = 21.230554; pan[220] = -.775;  break;
            case(221):  tdelay[221] = 21.290000; pan[221] = .125;   break;
            case(222):  tdelay[222] = 21.372498; pan[222] = -.125;  break;
            case(223):  tdelay[223] = 21.445365; pan[223] = -.65;   break;
            case(224):  tdelay[224] = 21.571897; pan[224] = .65;    break;
            case(225):  tdelay[225] = 21.627656; pan[225] = .35;    break;
            case(226):  tdelay[226] = 21.695809; pan[226] = -.35;   break;
            case(227):  tdelay[227] = 21.758350; pan[227] = -.1;    break;
            case(228):  tdelay[228] = 21.880984; pan[228] = .1;     break;
            case(229):  tdelay[229] = 21.975910; pan[229] = -.8;    break;
            case(230):  tdelay[230] = 22.043894; pan[230] = .8;     break;
            case(231):  tdelay[231] = 22.143261; pan[231] = -.55;   break;
            case(232):  tdelay[232] = 22.268126; pan[232] = .55;    break;
            case(233):  tdelay[233] = 22.366093; pan[233] = -.25;   break;
            case(234):  tdelay[234] = 22.480461; pan[234] = .25;    break;
            case(235):  tdelay[235] = 22.577198; pan[235] = -.45;   break;
            case(236):  tdelay[236] = 22.666871; pan[236] = .45;    break;
            case(237):  tdelay[237] = 22.779061; pan[237] = .05;    break;
            case(238):  tdelay[238] = 22.852884; pan[238] = -.9;    break;
            case(239):  tdelay[239] = 22.966519; pan[239] = -.05;   break;
            case(240):  tdelay[240] = 23.043514; pan[240] = .9;     break;
            case(241):  tdelay[241] = 23.147562; pan[241] = -.425;  break;
            case(242):  tdelay[242] = 23.241538; pan[242] = .425;   break;
            case(243):  tdelay[243] = 23.358808; pan[243] = .75;    break;
            case(244):  tdelay[244] = 23.470554; pan[244] = -.75;   break;
            case(245):  tdelay[245] = 23.540000; pan[245] = .275;   break;
            case(246):  tdelay[246] = 23.612729; pan[246] = -.275;  break;
            case(247):  tdelay[247] = 23.693888; pan[247] = -.625;  break;
            case(248):  tdelay[248] = 23.750479; pan[248] = .625;   break;
            case(249):  tdelay[249] = 23.834122; pan[249] = .375;   break;
            case(250):  tdelay[250] = 23.894456; pan[250] = -.375;  break;
            case(251):  tdelay[251] = 23.978112; pan[251] = -.116;  break;
            case(252):  tdelay[252] = 24.107199; pan[252] = .116;   break;
            case(253):  tdelay[253] = 24.217554; pan[253] = -.825;  break;
            case(254):  tdelay[254] = 24.282567; pan[254] = .825;   break;
            case(255):  tdelay[255] = 24.387678; pan[255] = -.535;  break;
            case(256):  tdelay[256] = 24.493642; pan[256] = .535;   break;
            case(257):  tdelay[257] = 24.610009; pan[257] = -.265;  break;
            case(258):  tdelay[258] = 24.720302; pan[258] = .265;   break;
            case(259):  tdelay[259] = 24.818331; pan[259] = -.475;  break;
            case(260):  tdelay[260] = 24.911186; pan[260] = .475;   break;
            case(261):  tdelay[261] = 25.013019; pan[261] = .11;    break;
            case(262):  tdelay[262] = 25.089382; pan[262] = -.77;   break;
            case(263):  tdelay[263] = 25.192071; pan[263] = -.11;   break;
            case(264):  tdelay[264] = 25.288516; pan[264] = .77;    break;
            case(265):  tdelay[265] = 25.403337; pan[265] = -.475;  break;
            case(266):  tdelay[266] = 25.474009; pan[266] = .475;   break;
            case(267):  tdelay[267] = 25.613701; pan[267] = .725;   break;
            case(268):  tdelay[268] = 25.710554; pan[268] = -.725;  break;
            case(269):  tdelay[269] = 25.800000; pan[269] = .235;   break;
            case(270):  tdelay[270] = 25.933349; pan[270] = -.235;  break;
            case(271):  tdelay[271] = 26.032532; pan[271] = -.685;  break;
            case(272):  tdelay[272] = 26.122216; pan[272] = .685;   break;
            case(273):  tdelay[273] = 26.194122; pan[273] = .315;   break;
            case(274):  tdelay[274] = 26.265021; pan[274] = -.315;  break;
            case(275):  tdelay[275] = 26.358764; pan[275] = -.136;  break;
            case(276):  tdelay[276] = 26.445276; pan[276] = .136;   break;
            case(277):  tdelay[277] = 26.539293; pan[277] = -.635;  break;
            case(278):  tdelay[278] = 26.609978; pan[278] = .635;   break;
            case(279):  tdelay[279] = 26.710640; pan[279] = -.435;  break;
            case(280):  tdelay[280] = 26.831968; pan[280] = .435;   break;
            case(281):  tdelay[281] = 26.933759; pan[281] = -.765;  break;
            case(282):  tdelay[282] = 27.031102; pan[282] = .765;   break;
            case(283):  tdelay[283] = 27.120963; pan[283] = -.575;  break;
            case(284):  tdelay[284] = 27.230907; pan[284] = .575;   break;
            case(285):  tdelay[285] = 27.322181; pan[285] = .215;   break;
            case(286):  tdelay[286] = 27.426997; pan[286] = -.692;  break;
            case(287):  tdelay[287] = 27.531689; pan[287] = -.215;  break;
            case(288):  tdelay[288] = 27.607012; pan[288] = .692;   break;
            case(289):  tdelay[289] = 27.715148; pan[289] = -.435;  break;
            case(290):  tdelay[290] = 27.803142; pan[290] = .435;   break;
            case(291):  tdelay[291] = 27.951037; pan[291] = .675;   break;
            case(292):  tdelay[292] = 28.018695; pan[292] = -.675;  break;
            case(293):  tdelay[293] = 28.107151; pan[293] = .265;   break;
            case(294):  tdelay[294] = 28.240962; pan[294] = -.265;  break;
            case(295):  tdelay[295] = 28.312251; pan[295] = -.555;  break;
            case(296):  tdelay[296] = 28.422676; pan[296] = -.333;  break;
            case(297):  tdelay[297] = 28.514122; pan[297] = 0.0;    break;
            case(298):  tdelay[298] = 28.637713;  pan[298] = -1;    break;
            case(299):  tdelay[299] = 28.730468;  pan[299] = 1;     break;
            case(300):  tdelay[300] = 28.808751;  pan[300] = .5;    break;
            case(301):  tdelay[301] = 28.910460;  pan[301] = -.5;   break;
            case(302):  tdelay[302] = 29.027041;  pan[302] = 0;     break;
            case(303):  tdelay[303] = 29.132028;  pan[303] = .7;    break;
            case(304):  tdelay[304] = 29.244272;  pan[304] = -.7;   break;
            case(305):  tdelay[305] = 29.336923;  pan[305] = .3;    break;
            case(306):  tdelay[306] = 29.427700;  pan[306] = -.3;   break;
            case(307):  tdelay[307] = 29.528503;  pan[307] = .15;   break;
            case(308):  tdelay[308] = 29.618661; pan[308] = -.15;   break;
            case(309):  tdelay[309] = 29.715413; pan[309] = .85;    break;
            case(310):  tdelay[310] = 29.814730; pan[310] = -.85;   break;
            case(311):  tdelay[311] = 29.914843; pan[311] = .4;     break;
            case(312):  tdelay[312] = 30.000258; pan[312] = -.4;    break;
            case(313):  tdelay[313] = 30.135363; pan[313] = .6;     break;
            case(314):  tdelay[314] = 30.230554; pan[314] = -.6;    break;
            case(315):  tdelay[315] = 30.290000; pan[315] = .225;   break;
            case(316):  tdelay[316] = 30.372498; pan[316] = -.225;  break;
            case(317):  tdelay[317] = 30.445365; pan[317] = .775;   break;
            case(318):  tdelay[318] = 30.571897; pan[318] = -.775;  break;
            case(319):  tdelay[319] = 30.627656; pan[319] = .125;   break;
            case(320):  tdelay[320] = 30.695809; pan[320] = -.125;  break;
            case(321):  tdelay[321] = 30.758350; pan[321] = -.65;   break;
            case(322):  tdelay[322] = 30.880984; pan[322] = .65;    break;
            case(323):  tdelay[323] = 30.975910; pan[323] = .35;    break;
            case(324):  tdelay[324] = 31.043894; pan[324] = -.35;   break;
            case(325):  tdelay[325] = 31.143261; pan[325] = -.1;    break;
            case(326):  tdelay[326] = 31.268126; pan[326] = .1;     break;
            case(327):  tdelay[327] = 31.366093; pan[327] = -.8;    break;
            case(328):  tdelay[328] = 31.480461; pan[328] = .8;     break;
            case(329):  tdelay[329] = 31.577198; pan[329] = -.55;   break;
            case(330):  tdelay[330] = 31.666871; pan[330] = .55;    break;
            case(331):  tdelay[331] = 31.779061; pan[331] = -.25;   break;
            case(332):  tdelay[332] = 31.852884; pan[332] = .25;    break;
            case(333):  tdelay[333] = 31.966519; pan[333] = -.45;   break;
            case(334):  tdelay[334] = 32.043514; pan[334] = .45;    break;
            case(335):  tdelay[335] = 32.147562; pan[335] = .05;    break;
            case(336):  tdelay[336] = 32.241538; pan[336] = -.9;    break;
            case(337):  tdelay[337] = 32.358808; pan[337] = -.05;   break;
            case(338):  tdelay[338] = 32.470554; pan[338] = .9;     break;
            case(339):  tdelay[339] = 32.540000; pan[339] = -.425;  break;
            case(340):  tdelay[340] = 32.612729; pan[340] = .425;   break;
            case(341):  tdelay[341] = 32.693888; pan[341] = .75;    break;
            case(342):  tdelay[342] = 32.750479; pan[342] = -.75;   break;
            case(343):  tdelay[343] = 32.834122; pan[343] = .275;   break;
            case(344):  tdelay[344] = 32.894456; pan[344] = -.275;  break;
            case(345):  tdelay[345] = 32.978112; pan[345] = -.625;  break;
            case(346):  tdelay[346] = 33.107199; pan[346] = .625;   break;
            case(347):  tdelay[347] = 33.217554; pan[347] = .375;   break;
            case(348):  tdelay[348] = 33.282567; pan[348] = -.375;  break;
            case(349):  tdelay[349] = 33.387678; pan[349] = -.116;  break;
            case(350):  tdelay[350] = 33.493642; pan[350] = .116;   break;
            case(351):  tdelay[351] = 33.610009; pan[351] = -.825;  break;
            case(352):  tdelay[352] = 33.720302; pan[352] = .825;   break;
            case(353):  tdelay[353] = 33.818331; pan[353] = -.535;  break;
            case(354):  tdelay[354] = 33.911186; pan[354] = .535;   break;
            case(355):  tdelay[355] = 34.013019; pan[355] = -.265;  break;
            case(356):  tdelay[356] = 34.089382; pan[356] = .265;   break;
            case(357):  tdelay[357] = 34.192071; pan[357] = -.475;  break;
            case(358):  tdelay[358] = 34.288516; pan[358] = .475;   break;
            case(359):  tdelay[359] = 34.403337; pan[359] = .11;    break;
            case(360):  tdelay[360] = 34.474009; pan[360] = -.77;   break;
            case(361):  tdelay[361] = 34.613701; pan[361] = -.11;   break;
            case(362):  tdelay[362] = 34.710554; pan[362] = .77;    break;
            case(363):  tdelay[363] = 34.800000; pan[363] = -.475;  break;
            case(364):  tdelay[364] = 34.933349; pan[364] = .475;   break;
            case(365):  tdelay[365] = 35.032532; pan[365] = .725;   break;
            case(366):  tdelay[366] = 35.122216; pan[366] = -.725;  break;
            case(367):  tdelay[367] = 35.194122; pan[367] = .235;   break;
            case(368):  tdelay[368] = 35.265021; pan[368] = -.235;  break;
            case(369):  tdelay[369] = 35.358764; pan[369] = -.685;  break;
            case(370):  tdelay[370] = 35.445276; pan[370] = .685;   break;
            case(371):  tdelay[371] = 35.539293; pan[371] = .315;   break;
            case(372):  tdelay[372] = 35.609978; pan[372] = -.315;  break;
            case(373):  tdelay[373] = 35.710640; pan[373] = -.136;  break;
            case(374):  tdelay[374] = 35.831968; pan[374] = .136;   break;
            case(375):  tdelay[375] = 35.933759; pan[375] = -.635;  break;
            case(376):  tdelay[376] = 36.031102; pan[376] = .635;   break;
            case(377):  tdelay[377] = 36.120963; pan[377] = -.435;  break;
            case(378):  tdelay[378] = 36.230907; pan[378] = .435;   break;
            case(379):  tdelay[379] = 36.322181; pan[379] = -.765;  break;
            case(380):  tdelay[380] = 36.426997; pan[380] = .765;   break;
            case(381):  tdelay[381] = 36.531689; pan[381] = -.575;  break;
            case(382):  tdelay[382] = 36.607012; pan[382] = .575;   break;
            case(383):  tdelay[383] = 36.715148; pan[383] = .215;   break;
            case(384):  tdelay[384] = 36.803142; pan[384] = -.692;  break;
            case(385):  tdelay[385] = 36.951037; pan[385] = -.215;  break;
            case(386):  tdelay[386] = 37.088695; pan[386] = .692;   break;
            case(387):  tdelay[387] = 37.240962; pan[387] = -.435;  break;
            case(388):  tdelay[388] = 37.312251; pan[388] = .435;   break;
            case(389):  tdelay[389] = 37.422676; pan[389] = .675;   break;
            case(390):  tdelay[390] = 37.514122; pan[390] = -.675;  break;
            case(391):  tdelay[391] = 37.637713; pan[391] = .265;   break;
            case(392):  tdelay[392] = 37.730468; pan[392] = -.265;  break;
            case(393):  tdelay[393] = 37.808751; pan[393] = -.555;  break;
            case(394):  tdelay[394] = 37.910460; pan[394] = .555;   break;
            case(395):  tdelay[395] = 38.000000; pan[395] = .333;   break;
            case(396):  tdelay[396] = 38.131832; pan[396] = -.333;  break;
            case(397):  tdelay[397] = 38.215038; pan[397] = 0.0;    break;
            case(398):  tdelay[398] = 38.322274;  pan[398] = -1;    break;
            case(399):  tdelay[399] = 38.414122;  pan[399] = 1;     break;
            case(400):  tdelay[400] = 38.504488;  pan[400] = .5;    break;
            case(401):  tdelay[401] = 38.637713;  pan[401] = -.5;   break;
            case(402):  tdelay[402] = 38.730468;  pan[402] = 0;     break;
            case(403):  tdelay[403] = 38.808751;  pan[403] = .7;    break;
            case(404):  tdelay[404] = 38.910460;  pan[404] = -.7;   break;
            case(405):  tdelay[405] = 39.027041;  pan[405] = .3;    break;
            case(406):  tdelay[406] = 39.132028;  pan[406] = -.3;   break;
            case(407):  tdelay[407] = 39.244272;  pan[407] = .15;   break;
            case(408):  tdelay[408] = 39.336923; pan[408] = -.15;   break;
            case(409):  tdelay[409] = 39.427700; pan[409] = .85;    break;
            case(410):  tdelay[410] = 39.528503; pan[410] = -.85;   break;
            case(411):  tdelay[411] = 39.618661; pan[411] = .4;     break;
            case(412):  tdelay[412] = 39.715413; pan[412] = -.4;    break;
            case(413):  tdelay[413] = 39.814730; pan[413] = .6;     break;
            case(414):  tdelay[414] = 39.914843; pan[414] = -.6;    break;
            case(415):  tdelay[415] = 40.000258; pan[415] = .225;   break;
            case(416):  tdelay[416] = 40.135363; pan[416] = -.225;  break;
            case(417):  tdelay[417] = 40.230554; pan[417] = .775;   break;
            case(418):  tdelay[418] = 40.290000; pan[418] = -.775;  break;
            case(419):  tdelay[419] = 40.372498; pan[419] = .125;   break;
            case(420):  tdelay[420] = 40.445365; pan[420] = -.125;  break;
            case(421):  tdelay[421] = 40.571897; pan[421] = -.65;   break;
            case(422):  tdelay[422] = 40.627656; pan[422] = .65;    break;
            case(423):  tdelay[423] = 40.695809; pan[423] = .35;    break;
            case(424):  tdelay[424] = 40.758350; pan[424] = -.35;   break;
            case(425):  tdelay[425] = 40.880984; pan[425] = -.1;    break;
            case(426):  tdelay[426] = 40.975910; pan[426] = .1;     break;
            case(427):  tdelay[427] = 41.043894; pan[427] = -.8;    break;
            case(428):  tdelay[428] = 41.143261; pan[428] = .8;     break;
            case(429):  tdelay[429] = 41.268126; pan[429] = -.55;   break;
            case(430):  tdelay[430] = 41.366093; pan[430] = .55;    break;
            case(431):  tdelay[431] = 41.480461; pan[431] = -.25;   break;
            case(432):  tdelay[432] = 41.577198; pan[432] = .25;    break;
            case(433):  tdelay[433] = 41.666871; pan[433] = -.45;   break;
            case(434):  tdelay[434] = 41.779061; pan[434] = .45;    break;
            case(435):  tdelay[435] = 41.852884; pan[435] = .05;    break;
            case(436):  tdelay[436] = 41.966519; pan[436] = -.9;    break;
            case(437):  tdelay[437] = 42.043514; pan[437] = -.05;   break;
            case(438):  tdelay[438] = 42.147562; pan[438] = .9;     break;
            case(439):  tdelay[439] = 42.241538; pan[439] = -.425;  break;
            case(440):  tdelay[440] = 42.358808; pan[440] = .425;   break;
            case(441):  tdelay[441] = 42.470554; pan[441] = .75;    break;
            case(442):  tdelay[442] = 42.540000; pan[442] = -.75;   break;
            case(443):  tdelay[443] = 42.612729; pan[443] = .275;   break;
            case(444):  tdelay[444] = 42.693888; pan[444] = -.275;  break;
            case(445):  tdelay[445] = 42.750479; pan[445] = -.625;  break;
            case(446):  tdelay[446] = 42.834122; pan[446] = .625;   break;
            case(447):  tdelay[447] = 42.894456; pan[447] = .375;   break;
            case(448):  tdelay[448] = 42.978112; pan[448] = -.375;  break;
            case(449):  tdelay[449] = 43.107199; pan[449] = -.116;  break;
            case(450):  tdelay[450] = 43.217554; pan[450] = .116;   break;
            case(451):  tdelay[451] = 43.282567; pan[451] = -.825;  break;
            case(452):  tdelay[452] = 43.387678; pan[452] = .825;   break;
            case(453):  tdelay[453] = 43.493642; pan[453] = -.535;  break;
            case(454):  tdelay[454] = 43.610009; pan[454] = .535;   break;
            case(455):  tdelay[455] = 43.720302; pan[455] = -.265;  break;
            case(456):  tdelay[456] = 43.818331; pan[456] = .265;   break;
            case(457):  tdelay[457] = 43.911186; pan[457] = -.475;  break;
            case(458):  tdelay[458] = 44.013019; pan[458] = .475;   break;
            case(459):  tdelay[459] = 44.089382; pan[459] = .11;    break;
            case(460):  tdelay[460] = 44.192071; pan[460] = -.77;   break;
            case(461):  tdelay[461] = 44.288516; pan[461] = -.11;   break;
            case(462):  tdelay[462] = 44.403337; pan[462] = .77;    break;
            case(463):  tdelay[463] = 44.474009; pan[463] = -.475;  break;
            case(464):  tdelay[464] = 44.613701; pan[464] = .475;   break;
            case(465):  tdelay[465] = 44.710554; pan[465] = .725;   break;
            case(466):  tdelay[466] = 44.800000; pan[466] = -.725;  break;
            case(467):  tdelay[467] = 44.933349; pan[467] = .235;   break;
            case(468):  tdelay[468] = 45.032532; pan[468] = -.235;  break;
            case(469):  tdelay[469] = 45.122216; pan[469] = -.685;  break;
            case(470):  tdelay[470] = 45.194122; pan[470] = .685;   break;
            case(471):  tdelay[471] = 45.265021; pan[471] = .315;   break;
            case(472):  tdelay[472] = 45.358764; pan[472] = -.315;  break;
            case(473):  tdelay[473] = 45.445276; pan[473] = -.136;  break;
            case(474):  tdelay[474] = 45.539293; pan[474] = .136;   break;
            case(475):  tdelay[475] = 45.609978; pan[475] = -.635;  break;
            case(476):  tdelay[476] = 45.710640; pan[476] = .635;   break;
            case(477):  tdelay[477] = 45.831968; pan[477] = -.435;  break;
            case(478):  tdelay[478] = 45.933759; pan[478] = .435;   break;
            case(479):  tdelay[479] = 46.031102; pan[479] = -.765;  break;
            case(480):  tdelay[480] = 46.120963; pan[480] = .765;   break;
            case(481):  tdelay[481] = 46.230907; pan[481] = -.575;  break;
            case(482):  tdelay[482] = 46.322181; pan[482] = .575;   break;
            case(483):  tdelay[483] = 46.426997; pan[483] = .215;   break;
            case(484):  tdelay[484] = 46.531689; pan[484] = -.692;  break;
            case(485):  tdelay[485] = 46.607012; pan[485] = -.215;  break;
            case(486):  tdelay[486] = 46.715148; pan[486] = .692;   break;
            case(487):  tdelay[487] = 46.803142; pan[487] = -.435;  break;
            case(488):  tdelay[488] = 46.951037; pan[488] = .435;   break;
            case(489):  tdelay[489] = 47.018695; pan[489] = .675;   break;
            case(490):  tdelay[490] = 47.107151; pan[490] = -.675;  break;
            case(491):  tdelay[491] = 47.240962; pan[491] = .265;   break;
            case(492):  tdelay[492] = 47.312251; pan[492] = -.265;  break;
            case(493):  tdelay[493] = 47.422676; pan[493] = -.555;  break;
            case(494):  tdelay[494] = 47.514122; pan[494] = .555;   break;
            case(495):  tdelay[495] = 47.637713; pan[495] = .333;   break;
            case(496):  tdelay[496] = 47.730468; pan[496] = -.333;  break;
            case(497):  tdelay[497] = 47.808751; pan[497] = 0.0;    break;
            case(498):  tdelay[498] = 47.910460; pan[498] = -1;     break;
            case(499):  tdelay[499] = 48.027041; pan[499] = 1;      break;
            case(500):  tdelay[500] = 48.132028; pan[500] = .5;     break;
            case(501):  tdelay[501] = 48.244272; pan[501] = -.5;    break;
            case(502):  tdelay[502] = 48.336923; pan[502] = 0;      break;
            case(503):  tdelay[503] = 48.427700; pan[503] = .7;     break;
            case(504):  tdelay[504] = 48.528503; pan[504] = -.7;    break;
            case(505):  tdelay[505] = 48.618661; pan[505] = .3;     break;
            case(506):  tdelay[506] = 48.715413; pan[506] = -.3;    break;
            case(507):  tdelay[507] = 48.814730; pan[507] = .15;    break;
            case(508):  tdelay[508] = 48.914843; pan[508] = -.15;   break;
            case(509):  tdelay[509] = 49.000258; pan[509] = .85;    break;
            case(510):  tdelay[510] = 49.135363; pan[510] = -.85;   break;
            case(511):  tdelay[511] = 49.230554; pan[511] = .4;     break;
            case(512):  tdelay[512] = 49.290000; pan[512] = -.4;    break;
            case(513):  tdelay[513] = 49.372498; pan[513] = .6;     break;
            case(514):  tdelay[514] = 49.445365; pan[514] = -.6;    break;
            case(515):  tdelay[515] = 49.571897; pan[515] = .225;   break;
            case(516):  tdelay[516] = 49.627656; pan[516] = -.225;  break;
            case(517):  tdelay[517] = 49.695809; pan[517] = .775;   break;
            case(518):  tdelay[518] = 49.758350; pan[518] = -.775;  break;
            case(519):  tdelay[519] = 49.880984; pan[519] = .125;   break;
            case(520):  tdelay[520] = 49.975910; pan[520] = -.125;  break;
            case(521):  tdelay[521] = 50.043894; pan[521] = -.65;   break;
            case(522):  tdelay[522] = 50.143261; pan[522] = .65;    break;
            case(523):  tdelay[523] = 50.268126; pan[523] = .35;    break;
            case(524):  tdelay[524] = 50.366093; pan[524] = -.35;   break;
            case(525):  tdelay[525] = 50.480461; pan[525] = -.1;    break;
            case(526):  tdelay[526] = 50.577198; pan[526] = .1;     break;
            case(527):  tdelay[527] = 50.666871; pan[527] = -.8;    break;
            case(528):  tdelay[528] = 50.779061; pan[528] = .8;     break;
            case(529):  tdelay[529] = 50.852884; pan[529] = -.55;   break;
            case(530):  tdelay[530] = 50.966519; pan[530] = .55;    break;
            case(531):  tdelay[531] = 51.043514; pan[531] = -.25;   break;
            case(532):  tdelay[532] = 51.147562; pan[532] = .25;    break;
            case(533):  tdelay[533] = 51.241538; pan[533] = -.45;   break;
            case(534):  tdelay[534] = 51.358808; pan[534] = .45;    break;
            case(535):  tdelay[535] = 51.470554; pan[535] = .05;    break;
            case(536):  tdelay[536] = 51.540000; pan[536] = -.9;    break;
            case(537):  tdelay[537] = 51.612729; pan[537] = -.05;   break;
            case(538):  tdelay[538] = 51.693888; pan[538] = .9;     break;
            case(539):  tdelay[539] = 51.750479; pan[539] = -.425;  break;
            case(540):  tdelay[540] = 51.834122; pan[540] = .425;   break;
            case(541):  tdelay[541] = 51.894456; pan[541] = .75;    break;
            case(542):  tdelay[542] = 51.978112; pan[542] = -.75;   break;
            case(543):  tdelay[543] = 52.107199; pan[543] = .275;   break;
            case(544):  tdelay[544] = 52.217554; pan[544] = -.275;  break;
            case(545):  tdelay[545] = 52.282567; pan[545] = -.625;  break;
            case(546):  tdelay[546] = 52.387678; pan[546] = .625;   break;
            case(547):  tdelay[547] = 52.493642; pan[547] = .375;   break;
            case(548):  tdelay[548] = 52.610009; pan[548] = -.375;  break;
            case(549):  tdelay[549] = 52.720302; pan[549] = -.116;  break;
            case(550):  tdelay[550] = 52.818331; pan[550] = .116;   break;
            case(551):  tdelay[551] = 52.911186; pan[551] = -.825;  break;
            case(552):  tdelay[552] = 53.013019; pan[552] = .825;   break;
            case(553):  tdelay[553] = 53.089382; pan[553] = -.535;  break;
            case(554):  tdelay[554] = 53.192071; pan[554] = .535;   break;
            case(555):  tdelay[555] = 53.288516; pan[555] = -.265;  break;
            case(556):  tdelay[556] = 53.403337; pan[556] = .265;   break;
            case(557):  tdelay[557] = 53.474009; pan[557] = -.475;  break;
            case(558):  tdelay[558] = 53.613701; pan[558] = .475;   break;
            case(559):  tdelay[559] = 53.710554; pan[559] = .11;    break;
            case(560):  tdelay[560] = 53.800000; pan[560] = -.77;   break;
            case(561):  tdelay[561] = 53.933349; pan[561] = -.11;   break;
            case(562):  tdelay[562] = 54.032532; pan[562] = .77;    break;
            case(563):  tdelay[563] = 54.122216; pan[563] = -.475;  break;
            case(564):  tdelay[564] = 54.194122; pan[564] = .475;   break;
            case(565):  tdelay[565] = 54.265021; pan[565] = .725;   break;
            case(566):  tdelay[566] = 54.358764; pan[566] = -.725;  break;
            case(567):  tdelay[567] = 54.445276; pan[567] = .235;   break;
            case(568):  tdelay[568] = 54.539293; pan[568] = -.235;  break;
            case(569):  tdelay[569] = 54.609978; pan[569] = -.685;  break;
            case(570):  tdelay[570] = 54.710640; pan[570] = .685;   break;
            case(571):  tdelay[571] = 54.831968; pan[571] = .315;   break;
            case(572):  tdelay[572] = 54.933759; pan[572] = -.315;  break;
            case(573):  tdelay[573] = 55.031102; pan[573] = -.136;  break;
            case(574):  tdelay[574] = 55.120963; pan[574] = .136;   break;
            case(575):  tdelay[575] = 55.230907; pan[575] = -.635;  break;
            case(576):  tdelay[576] = 55.322181; pan[576] = .635;   break;
            case(577):  tdelay[577] = 55.426997; pan[577] = -.435;  break;
            case(578):  tdelay[578] = 55.531689; pan[578] = .435;   break;
            case(579):  tdelay[579] = 55.607012; pan[579] = -.765;  break;
            case(580):  tdelay[580] = 55.715148; pan[580] = .765;   break;
            case(581):  tdelay[581] = 55.803142; pan[581] = -.575;  break;
            case(582):  tdelay[582] = 55.951037; pan[582] = .575;   break;
            case(583):  tdelay[583] = 56.018695; pan[583] = .215;   break;
            case(584):  tdelay[584] = 56.107151; pan[584] = -.692;  break;
            case(585):  tdelay[585] = 56.240962; pan[585] = -.215;  break;
            case(586):  tdelay[586] = 56.312251; pan[586] = .692;   break;
            case(587):  tdelay[587] = 56.422676; pan[587] = -.435;  break;
            case(588):  tdelay[588] = 56.514122; pan[588] = .435;   break;
            case(589):  tdelay[589] = 56.637713; pan[589] = .675;   break;
            case(590):  tdelay[590] = 56.730468; pan[590] = -.675;  break;
            case(591):  tdelay[591] = 56.808751; pan[591] = .265;   break;
            case(592):  tdelay[592] = 56.910460; pan[592] = -.265;  break;
            case(593):  tdelay[593] = 57.027041; pan[593] = -.555;  break;
            case(594):  tdelay[594] = 57.132028; pan[594] = .555;   break;
            case(595):  tdelay[595] = 57.244272; pan[595] = .333;   break;
            case(596):  tdelay[596] = 57.336923; pan[596] = -.333;  break;
            case(597):  tdelay[597] = 57.427700; pan[597] = 0.0;    break;
            case(598):  tdelay[598] = 57.528503;  pan[598] = -1;    break;
            case(599):  tdelay[599] = 57.618661;  pan[599] = 1;     break;
            case(600):  tdelay[600] = 57.715413;  pan[600] = .5;    break;
            case(601):  tdelay[601] = 57.814730;  pan[601] = -.5;   break;
            case(602):  tdelay[602] = 57.914843;  pan[602] = 0;     break;
            case(603):  tdelay[603] = 58.000258;  pan[603] = .7;    break;
            case(604):  tdelay[604] = 58.135363;  pan[604] = -.7;   break;
            case(605):  tdelay[605] = 58.230554;  pan[605] = .3;    break;
            case(606):  tdelay[606] = 58.290000;  pan[606] = -.3;   break;
            case(607):  tdelay[607] = 58.372498;  pan[607] = .15;   break;
            case(608):  tdelay[608] = 58.445365; pan[608] = -.15;   break;
            case(609):  tdelay[609] = 58.571897; pan[609] = .85;    break;
            case(610):  tdelay[610] = 58.627656; pan[610] = -.85;   break;
            case(611):  tdelay[611] = 58.695809; pan[611] = .4;     break;
            case(612):  tdelay[612] = 58.758350; pan[612] = -.4;    break;
            case(613):  tdelay[613] = 58.880984; pan[613] = .6;     break;
            case(614):  tdelay[614] = 58.975910; pan[614] = -.6;    break;
            case(615):  tdelay[615] = 59.043894; pan[615] = .225;   break;
            case(616):  tdelay[616] = 59.143261; pan[616] = -.225;  break;
            case(617):  tdelay[617] = 59.268126; pan[617] = .775;   break;
            case(618):  tdelay[618] = 59.366093; pan[618] = -.775;  break;
            case(619):  tdelay[619] = 59.480461; pan[619] = .125;   break;
            case(620):  tdelay[620] = 59.577198; pan[620] = -.125;  break;
            case(621):  tdelay[621] = 59.666871; pan[621] = -.65;   break;
            case(622):  tdelay[622] = 59.779061; pan[622] = .65;    break;
            case(623):  tdelay[623] = 59.852884; pan[623] = .35;    break;
            case(624):  tdelay[624] = 59.966519; pan[624] = -.35;   break;
            case(625):  tdelay[625] = 60.043514; pan[625] = -.1;    break;
            case(626):  tdelay[626] = 60.147562; pan[626] = .1;     break;
            case(627):  tdelay[627] = 60.241538; pan[627] = -.8;    break;
            case(628):  tdelay[628] = 60.358808; pan[628] = .8;     break;
            case(629):  tdelay[629] = 60.470554; pan[629] = -.55;   break;
            case(630):  tdelay[630] = 60.540000; pan[630] = .55;    break;
            case(631):  tdelay[631] = 60.612729; pan[631] = -.25;   break;
            case(632):  tdelay[632] = 60.693888; pan[632] = .25;    break;
            case(633):  tdelay[633] = 60.750479; pan[633] = -.45;   break;
            case(634):  tdelay[634] = 60.834122; pan[634] = .45;    break;
            case(635):  tdelay[635] = 60.894456; pan[635] = .05;    break;
            case(636):  tdelay[636] = 60.978112; pan[636] = -.9;    break;
            case(637):  tdelay[637] = 61.107199; pan[637] = -.05;   break;
            case(638):  tdelay[638] = 61.217554; pan[638] = .9;     break;
            case(639):  tdelay[639] = 61.282567; pan[639] = -.425;  break;
            case(640):  tdelay[640] = 61.387678; pan[640] = .425;   break;
            case(641):  tdelay[641] = 61.493642; pan[641] = .75;    break;
            case(642):  tdelay[642] = 61.610009; pan[642] = -.75;   break;
            case(643):  tdelay[643] = 61.720302; pan[643] = .275;   break;
            case(644):  tdelay[644] = 61.818331; pan[644] = -.275;  break;
            case(645):  tdelay[645] = 61.911186; pan[645] = -.625;  break;
            case(646):  tdelay[646] = 62.013019; pan[646] = .625;   break;
            case(647):  tdelay[647] = 62.089382; pan[647] = .375;   break;
            case(648):  tdelay[648] = 62.192071; pan[648] = -.375;  break;
            case(649):  tdelay[649] = 62.288516; pan[649] = -.116;  break;
            case(650):  tdelay[650] = 62.403337; pan[650] = .116;   break;
            case(651):  tdelay[651] = 62.474009; pan[651] = -.825;  break;
            case(652):  tdelay[652] = 62.613701; pan[652] = .825;   break;
            case(653):  tdelay[653] = 62.710554; pan[653] = -.535;  break;
            case(654):  tdelay[654] = 62.800000; pan[654] = .535;   break;
            case(655):  tdelay[655] = 62.933349; pan[655] = -.265;  break;
            case(656):  tdelay[656] = 63.032532; pan[656] = .265;   break;
            case(657):  tdelay[657] = 63.122216; pan[657] = -.475;  break;
            case(658):  tdelay[658] = 63.194122; pan[658] = .475;   break;
            case(659):  tdelay[659] = 63.265021; pan[659] = .11;    break;
            case(660):  tdelay[660] = 63.358764; pan[660] = -.77;   break;
            case(661):  tdelay[661] = 63.445276; pan[661] = -.11;   break;
            case(662):  tdelay[662] = 63.539293; pan[662] = .77;    break;
            case(663):  tdelay[663] = 63.609978; pan[663] = -.475;  break;
            case(664):  tdelay[664] = 63.710640; pan[664] = .475;   break;
            case(665):  tdelay[665] = 63.831968; pan[665] = .725;   break;
            case(666):  tdelay[666] = 63.933759; pan[666] = -.725;  break;
            case(667):  tdelay[667] = 64.031102; pan[667] = .235;   break;
            case(668):  tdelay[668] = 64.120963; pan[668] = -.235;  break;
            case(669):  tdelay[669] = 64.230907; pan[669] = -.685;  break;
            case(670):  tdelay[670] = 64.322181; pan[670] = .685;   break;
            case(671):  tdelay[671] = 64.426997; pan[671] = .315;   break;
            case(672):  tdelay[672] = 64.531689; pan[672] = -.315;  break;
            case(673):  tdelay[673] = 64.607012; pan[673] = -.136;  break;
            case(674):  tdelay[674] = 64.715148; pan[674] = .136;   break;
            case(675):  tdelay[675] = 64.803142; pan[675] = -.635;  break;
            case(676):  tdelay[676] = 64.951037; pan[676] = .635;   break;
            case(677):  tdelay[677] = 65.018695; pan[677] = -.435;  break;
            case(678):  tdelay[678] = 65.107151; pan[678] = .435;   break;
            case(679):  tdelay[679] = 65.240962; pan[679] = -.765;  break;
            case(680):  tdelay[680] = 65.312251; pan[680] = .765;   break;
            case(681):  tdelay[681] = 65.422676; pan[681] = -.575;  break;
            case(682):  tdelay[682] = 65.514122; pan[682] = .575;   break;
            case(683):  tdelay[683] = 65.637713; pan[683] = .215;   break;
            case(684):  tdelay[684] = 65.730468; pan[684] = -.692;  break;
            case(685):  tdelay[685] = 65.808751; pan[685] = -.215;  break;
            case(686):  tdelay[686] = 65.910460; pan[686] = .692;   break;
            case(687):  tdelay[687] = 66.000000; pan[687] = -.435;  break;
            case(688):  tdelay[688] = 66.131832; pan[688] = .435;   break;
            case(689):  tdelay[689] = 66.215038; pan[689] = .675;   break;
            case(690):  tdelay[690] = 66.322274; pan[690] = -.675;  break;
            case(691):  tdelay[691] = 66.414122; pan[691] = .265;   break;
            case(692):  tdelay[692] = 66.504488; pan[692] = -.265;  break;
            case(693):  tdelay[693] = 66.637713; pan[693] = -.555;  break;
            case(694):  tdelay[694] = 66.730468; pan[694] = .555;   break;
            case(695):  tdelay[695] = 66.808751; pan[695] = .333;   break;
            case(696):  tdelay[696] = 66.910460; pan[696] = -.333;  break;
            case(697):  tdelay[697] = 67.027041; pan[697] = 0.0;    break;
            case(698):  tdelay[698] = 67.132028; pan[698] = -1;     break;
            case(699):  tdelay[699] = 67.244272; pan[699] = 1;      break;
            case(700):  tdelay[700] = 67.336923; pan[700] = .5;     break;
            case(701):  tdelay[701] = 67.427700; pan[701] = -.5;    break;
            case(702):  tdelay[702] = 67.528503; pan[702] = 0;      break;
            case(703):  tdelay[703] = 67.618661; pan[703] = .7;     break;
            case(704):  tdelay[704] = 67.715413; pan[704] = -.7;    break;
            case(705):  tdelay[705] = 67.814730; pan[705] = .3;     break;
            case(706):  tdelay[706] = 67.914843; pan[706] = -.3;    break;
            case(707):  tdelay[707] = 68.000258; pan[707] = .15;    break;
            case(708):  tdelay[708] = 68.135363; pan[708] = -.15;   break;
            case(709):  tdelay[709] = 68.230554; pan[709] = .85;    break;
            case(710):  tdelay[710] = 68.290000; pan[710] = -.85;   break;
            case(711):  tdelay[711] = 68.372498; pan[711] = .4;     break;
            case(712):  tdelay[712] = 68.445365; pan[712] = -.4;    break;
            case(713):  tdelay[713] = 68.571897; pan[713] = .6;     break;
            case(714):  tdelay[714] = 68.627656; pan[714] = -.6;    break;
            case(715):  tdelay[715] = 68.695809; pan[715] = .225;   break;
            case(716):  tdelay[716] = 68.758350; pan[716] = -.225;  break;
            case(717):  tdelay[717] = 68.880984; pan[717] = .775;   break;
            case(718):  tdelay[718] = 68.975910; pan[718] = -.775;  break;
            case(719):  tdelay[719] = 69.043894; pan[719] = .125;   break;
            case(720):  tdelay[720] = 69.143261; pan[720] = -.125;  break;
            case(721):  tdelay[721] = 69.268126; pan[721] = -.65;   break;
            case(722):  tdelay[722] = 69.366093; pan[722] = .65;    break;
            case(723):  tdelay[723] = 69.480461; pan[723] = .35;    break;
            case(724):  tdelay[724] = 69.577198; pan[724] = -.35;   break;
            case(725):  tdelay[725] = 69.666871; pan[725] = -.1;    break;
            case(726):  tdelay[726] = 69.779061; pan[726] = .1;     break;
            case(727):  tdelay[727] = 69.852884; pan[727] = -.8;    break;
            case(728):  tdelay[728] = 69.966519; pan[728] = .8;     break;
            case(729):  tdelay[729] = 70.043514; pan[729] = -.55;   break;
            case(730):  tdelay[730] = 70.147562; pan[730] = .55;    break;
            case(731):  tdelay[731] = 70.241538; pan[731] = -.25;   break;
            case(732):  tdelay[732] = 70.358808; pan[732] = .25;    break;
            case(733):  tdelay[733] = 70.470554; pan[733] = -.45;   break;
            case(734):  tdelay[734] = 70.540000; pan[734] = .45;    break;
            case(735):  tdelay[735] = 70.612775; pan[735] = .05;    break;
            case(736):  tdelay[736] = 70.697583; pan[736] = -.9;    break;
            case(737):  tdelay[737] = 70.750379; pan[737] = -.05;   break;
            case(738):  tdelay[738] = 70.834122; pan[738] = .9;     break;
            case(739):  tdelay[739] = 70.894456; pan[739] = -.425;  break;
            case(740):  tdelay[740] = 70.978112; pan[740] = .425;   break;
            case(741):  tdelay[741] = 71.107199; pan[741] = .75;    break;
            case(742):  tdelay[742] = 71.217554; pan[742] = -.75;   break;
            case(743):  tdelay[743] = 71.282567; pan[743] = .275;   break;
            case(744):  tdelay[744] = 71.387678; pan[744] = -.275;  break;
            case(745):  tdelay[745] = 71.493642; pan[745] = -.625;  break;
            case(746):  tdelay[746] = 71.610009; pan[746] = .625;   break;
            case(747):  tdelay[747] = 71.720302; pan[747] = .375;   break;
            case(748):  tdelay[748] = 71.818331; pan[748] = -.375;  break;
            case(749):  tdelay[749] = 71.911186; pan[749] = -.116;  break;
            case(750):  tdelay[750] = 72.013019; pan[750] = .116;   break;
            case(751):  tdelay[751] = 72.089382; pan[751] = -.825;  break;
            case(752):  tdelay[752] = 72.192071; pan[752] = .825;   break;
            case(753):  tdelay[753] = 72.288516; pan[753] = -.535;  break;
            case(754):  tdelay[754] = 72.403337; pan[754] = .535;   break;
            case(755):  tdelay[755] = 72.474009; pan[755] = -.265;  break;
            case(756):  tdelay[756] = 72.613701; pan[756] = .265;   break;
            case(757):  tdelay[757] = 72.710554; pan[757] = -.475;  break;
            case(758):  tdelay[758] = 72.800000; pan[758] = .475;   break;
            case(759):  tdelay[759] = 72.933349; pan[759] = .11;    break;
            case(760):  tdelay[760] = 73.032532; pan[760] = -.77;   break;
            case(761):  tdelay[761] = 73.122216; pan[761] = -.11;   break;
            case(762):  tdelay[762] = 73.194122; pan[762] = .77;    break;
            case(763):  tdelay[763] = 73.265021; pan[763] = -.475;  break;
            case(764):  tdelay[764] = 73.358764; pan[764] = .475;   break;
            case(765):  tdelay[765] = 73.445276; pan[765] = .725;   break;
            case(766):  tdelay[766] = 73.539293; pan[766] = -.725;  break;
            case(767):  tdelay[767] = 73.609978; pan[767] = .235;   break;
            case(768):  tdelay[768] = 73.710640; pan[768] = -.235;  break;
            case(769):  tdelay[769] = 73.831968; pan[769] = -.685;  break;
            case(770):  tdelay[770] = 73.933759; pan[770] = .685;   break;
            case(771):  tdelay[771] = 74.031102; pan[771] = .315;   break;
            case(772):  tdelay[772] = 74.120963; pan[772] = -.315;  break;
            case(773):  tdelay[773] = 74.230907; pan[773] = -.136;  break;
            case(774):  tdelay[774] = 74.322181; pan[774] = .136;   break;
            case(775):  tdelay[775] = 74.426997; pan[775] = -.635;  break;
            case(776):  tdelay[776] = 74.531689; pan[776] = .635;   break;
            case(777):  tdelay[777] = 74.607012; pan[777] = -.435;  break;
            case(778):  tdelay[778] = 74.715148; pan[778] = .435;   break;
            case(779):  tdelay[779] = 74.803142; pan[779] = -.765;  break;
            case(780):  tdelay[780] = 74.951037; pan[780] = .765;   break;
            case(781):  tdelay[781] = 75.018695; pan[781] = -.575;  break;
            case(782):  tdelay[782] = 75.107151; pan[782] = .575;   break;
            case(783):  tdelay[783] = 75.240962; pan[783] = .215;   break;
            case(784):  tdelay[784] = 75.312251; pan[784] = -.692;  break;
            case(785):  tdelay[785] = 75.422676; pan[785] = -.435;  break;
            case(786):  tdelay[786] = 75.514122; pan[786] = .435;   break;
            case(787):  tdelay[787] = 75.637713; pan[787] = .675;   break;
            case(788):  tdelay[788] = 75.730468; pan[788] = -.675;  break;
            case(789):  tdelay[789] = 75.808751; pan[789] = .265;   break;
            case(790):  tdelay[790] = 75.910460; pan[790] = -.265;  break;
            case(791):  tdelay[791] = 76.027041; pan[791] = -.555;  break;
            case(792):  tdelay[792] = 76.132028; pan[792] = .555;   break;
            case(793):  tdelay[793] = 76.244272; pan[793] = .333;   break;
            case(794):  tdelay[794] = 76.336923; pan[794] = -.333;  break;
            case(795):  tdelay[795] = 76.427700; pan[795] = 0.0;    break;
            case(796):  tdelay[796] = 76.528503; pan[796] = -1;     break;
            case(797):  tdelay[797] = 76.618661; pan[797] = 1;      break;
            case(798):  tdelay[798] = 76.715413; pan[798] = .5;     break;
            case(799):  tdelay[799] = 76.814730; pan[799] = -.5;    break;
            case(800):  tdelay[800] = 76.914843; pan[800] = 0;      break;
            case(801):  tdelay[801] = 77.000258; pan[801] = .7;     break;
            case(802):  tdelay[802] = 77.135363; pan[802] = -.7;    break;
            case(803):  tdelay[803] = 77.230554; pan[803] = .3;     break;
            case(804):  tdelay[804] = 77.290000; pan[804] = -.3;    break;
            case(805):  tdelay[805] = 77.372498; pan[805] = .15;    break;
            case(806):  tdelay[806] = 77.445365; pan[806] = -.15;   break;
            case(807):  tdelay[807] = 77.571897; pan[807] = .85;    break;
            case(808):  tdelay[808] = 77.627656; pan[808] = -.85;   break;
            case(809):  tdelay[809] = 77.695809; pan[809] = .4;     break;
            case(810):  tdelay[810] = 77.758350; pan[810] = -.4;    break;
            case(811):  tdelay[811] = 77.880984; pan[811] = .6;     break;
            case(812):  tdelay[812] = 77.975910; pan[812] = -.6;    break;
            case(813):  tdelay[813] = 78.043894; pan[813] = .225;   break;
            case(814):  tdelay[814] = 78.143261; pan[814] = -.225;  break;
            case(815):  tdelay[815] = 78.268126; pan[815] = .775;   break;
            case(816):  tdelay[816] = 78.366093; pan[816] = -.775;  break;
            case(817):  tdelay[817] = 78.480461; pan[817] = .125;   break;
            case(818):  tdelay[818] = 78.577198; pan[818] = -.125;  break;
            case(819):  tdelay[819] = 78.666871; pan[819] = -.65;   break;
            case(820):  tdelay[820] = 78.779061; pan[820] = .65;    break;
            case(821):  tdelay[821] = 78.852884; pan[821] = .35;    break;
            case(822):  tdelay[822] = 78.966519; pan[822] = -.35;   break;
            case(823):  tdelay[823] = 79.043514; pan[823] = -.1;    break;
            case(824):  tdelay[824] = 79.147562; pan[824] = .1;     break;
            case(825):  tdelay[825] = 79.241538; pan[825] = -.8;    break;
            case(826):  tdelay[826] = 79.358808; pan[826] = .8;     break;
            case(827):  tdelay[827] = 79.470554; pan[827] = -.55;   break;
            case(828):  tdelay[828] = 79.540000; pan[828] = .55;    break;
            case(829):  tdelay[829] = 79.612729; pan[829] = -.25;   break;
            case(830):  tdelay[830] = 79.693888; pan[830] = .25;    break;
            case(831):  tdelay[831] = 79.750479; pan[831] = -.45;   break;
            case(832):  tdelay[832] = 79.834122; pan[832] = .45;    break;
            case(833):  tdelay[833] = 79.894456; pan[833] = .05;    break;
            case(834):  tdelay[834] = 79.978112; pan[834] = -.9;    break;
            case(835):  tdelay[835] = 80.107199; pan[835] = -.05;   break;
            case(836):  tdelay[836] = 80.217554; pan[836] = .9;     break;
            case(837):  tdelay[837] = 80.282567; pan[837] = -.425;  break;
            case(838):  tdelay[838] = 80.387678; pan[838] = .425;   break;
            case(839):  tdelay[839] = 80.493642; pan[839] = .75;    break;
            case(840):  tdelay[840] = 80.610009; pan[840] = -.75;   break;
            case(841):  tdelay[841] = 80.720302; pan[841] = .275;   break;
            case(842):  tdelay[842] = 80.818331; pan[842] = -.275;  break;
            case(843):  tdelay[843] = 80.911186; pan[843] = -.625;  break;
            case(844):  tdelay[844] = 81.013019; pan[844] = .625;   break;
            case(845):  tdelay[845] = 81.089382; pan[845] = .375;   break;
            case(846):  tdelay[846] = 81.192071; pan[846] = -.375;  break;
            case(847):  tdelay[847] = 81.288516; pan[847] = -.116;  break;
            case(848):  tdelay[848] = 81.403337; pan[848] = .116;   break;
            case(849):  tdelay[849] = 81.474009; pan[849] = -.825;  break;
            case(850):  tdelay[850] = 81.613701; pan[850] = .825;   break;
            case(851):  tdelay[851] = 81.710554; pan[851] = -.535;  break;
            case(852):  tdelay[852] = 81.800000; pan[852] = .535;   break;
            case(853):  tdelay[853] = 81.933349; pan[853] = -.265;  break;
            case(854):  tdelay[854] = 82.032532; pan[854] = .265;   break;
            case(855):  tdelay[855] = 82.122216; pan[855] = -.475;  break;
            case(856):  tdelay[856] = 82.194122; pan[856] = .475;   break;
            case(857):  tdelay[857] = 82.265021; pan[857] = .11;    break;
            case(858):  tdelay[858] = 82.358764; pan[858] = -.77;   break;
            case(859):  tdelay[859] = 82.445276; pan[859] = -.11;   break;
            case(860):  tdelay[860] = 82.539293; pan[860] = .77;    break;
            case(861):  tdelay[861] = 82.609978; pan[861] = -.475;  break;
            case(862):  tdelay[862] = 82.710640; pan[862] = .475;   break;
            case(863):  tdelay[863] = 82.831968; pan[863] = .725;   break;
            case(864):  tdelay[864] = 82.933759; pan[864] = -.725;  break;
            case(865):  tdelay[865] = 83.031102; pan[865] = .235;   break;
            case(866):  tdelay[866] = 83.120963; pan[866] = -.235;  break;
            case(867):  tdelay[867] = 83.230907; pan[867] = -.685;  break;
            case(868):  tdelay[868] = 83.322181; pan[868] = .685;   break;
            case(869):  tdelay[869] = 83.426997; pan[869] = .315;   break;
            case(870):  tdelay[870] = 83.531689; pan[870] = -.315;  break;
            case(871):  tdelay[871] = 83.607012; pan[871] = -.136;  break;
            case(872):  tdelay[872] = 83.715148; pan[872] = .136;   break;
            case(873):  tdelay[873] = 83.803142; pan[873] = -.635;  break;
            case(874):  tdelay[874] = 83.951037; pan[874] = .635;   break;
            case(875):  tdelay[875] = 84.018695; pan[875] = -.435;  break;
            case(876):  tdelay[876] = 84.160962; pan[876] = .435;   break;
            case(877):  tdelay[877] = 84.312251; pan[877] = -.765;  break;
            case(878):  tdelay[878] = 84.422676; pan[878] = .765;   break;
            case(879):  tdelay[879] = 84.514122; pan[879] = -.575;  break;
            case(880):  tdelay[880] = 84.637713; pan[880] = .575;   break;
            case(881):  tdelay[881] = 84.730468; pan[881] = .215;   break;
            case(882):  tdelay[882] = 84.808751; pan[882] = -.692;  break;
            case(883):  tdelay[883] = 84.910460; pan[883] = -.215;  break;
            case(884):  tdelay[884] = 85.000000; pan[884] = .692;   break;
            case(885):  tdelay[885] = 85.131832; pan[885] = -.435;  break;
            case(886):  tdelay[886] = 85.215038; pan[886] = .435;   break;
            case(887):  tdelay[887] = 85.322274; pan[887] = .675;   break;
            case(888):  tdelay[888] = 85.414122; pan[888] = -.675;  break;
            case(889):  tdelay[889] = 85.504488; pan[889] = .265;   break;
            case(890):  tdelay[890] = 85.637713; pan[890] = -.265;  break;
            case(891):  tdelay[891] = 85.730468; pan[891] = -.555;  break;
            case(892):  tdelay[892] = 85.808751; pan[892] = .555;   break;
            case(893):  tdelay[893] = 85.910460; pan[893] = .333;   break;
            case(894):  tdelay[894] = 86.027041; pan[894] = -.333;  break;
            case(895):  tdelay[895] = 86.132028; pan[895] = 0.0;    break;
            case(896):  tdelay[896] = 86.244272;  pan[896] = -1;    break;
            case(897):  tdelay[897] = 86.336923;  pan[897] = 1;     break;
            case(898):  tdelay[898] = 86.427700;  pan[898] = .5;    break;
            case(899):  tdelay[899] = 86.528503;  pan[899] = -.5;   break;
            case(900):  tdelay[900] = 86.618661;  pan[900] = 0;     break;
            case(901):  tdelay[901] = 86.715413;  pan[901] = .7;    break;
            case(902):  tdelay[902] = 86.814730;  pan[902] = -.7;   break;
            case(903):  tdelay[903] = 86.914843;  pan[903] = .3;    break;
            case(904):  tdelay[904] = 87.000258;  pan[904] = -.3;   break;
            case(905):  tdelay[905] = 87.135363;  pan[905] = .15;   break;
            case(906):  tdelay[906] = 87.230554; pan[906] = -.15;   break;
            case(907):  tdelay[907] = 87.290000; pan[907] = .85;    break;
            case(908):  tdelay[908] = 87.372498; pan[908] = -.85;   break;
            case(909):  tdelay[909] = 87.445365; pan[909] = .4;     break;
            case(910):  tdelay[910] = 87.571897; pan[910] = -.4;    break;
            case(911):  tdelay[911] = 87.627656; pan[911] = .6;     break;
            case(912):  tdelay[912] = 87.695809; pan[912] = -.6;    break;
            case(913):  tdelay[913] = 87.758350; pan[913] = .225;   break;
            case(914):  tdelay[914] = 87.880984; pan[914] = -.225;  break;
            case(915):  tdelay[915] = 87.975910; pan[915] = .775;   break;
            case(916):  tdelay[916] = 88.043894; pan[916] = -.775;  break;
            case(917):  tdelay[917] = 88.143261; pan[917] = .125;   break;
            case(918):  tdelay[918] = 88.268126; pan[918] = -.125;  break;
            case(919):  tdelay[919] = 88.366093; pan[919] = -.65;   break;
            case(920):  tdelay[920] = 88.480461; pan[920] = .65;    break;
            case(921):  tdelay[921] = 88.577198; pan[921] = .35;    break;
            case(922):  tdelay[922] = 88.666871; pan[922] = -.35;   break;
            case(923):  tdelay[923] = 88.779061; pan[923] = -.1;    break;
            case(924):  tdelay[924] = 88.852884; pan[924] = .1;     break;
            case(925):  tdelay[925] = 88.966519; pan[925] = -.8;    break;
            case(926):  tdelay[926] = 89.043514; pan[926] = .8;     break;
            case(927):  tdelay[927] = 89.147562; pan[927] = -.55;   break;
            case(928):  tdelay[928] = 89.241538; pan[928] = .55;    break;
            case(929):  tdelay[929] = 89.358808; pan[929] = -.25;   break;
            case(930):  tdelay[930] = 89.470554; pan[930] = .25;    break;
            case(931):  tdelay[931] = 89.540000; pan[931] = -.45;   break;
            case(932):  tdelay[932] = 89.612729; pan[932] = .45;    break;
            case(933):  tdelay[933] = 89.693888; pan[933] = .05;    break;
            case(934):  tdelay[934] = 89.750479; pan[934] = -.9;    break;
            case(935):  tdelay[935] = 89.834122; pan[935] = -.05;   break;
            case(936):  tdelay[936] = 89.894456; pan[936] = .9;     break;
            case(937):  tdelay[937] = 89.978112; pan[937] = -.425;  break;
            case(938):  tdelay[938] = 90.107199; pan[938] = .425;   break;
            case(939):  tdelay[939] = 90.217554; pan[939] = .75;    break;
            case(940):  tdelay[940] = 90.282567; pan[940] = -.75;   break;
            case(941):  tdelay[941] = 90.387678; pan[941] = .275;   break;
            case(942):  tdelay[942] = 90.493642; pan[942] = -.275;  break;
            case(943):  tdelay[943] = 90.610009; pan[943] = -.625;  break;
            case(944):  tdelay[944] = 90.720302; pan[944] = .625;   break;
            case(945):  tdelay[945] = 90.818331; pan[945] = .375;   break;
            case(946):  tdelay[946] = 90.911186; pan[946] = -.375;  break;
            case(947):  tdelay[947] = 91.013019; pan[947] = -.116;  break;
            case(948):  tdelay[948] = 91.089382; pan[948] = .116;   break;
            case(949):  tdelay[949] = 91.192071; pan[949] = -.825;  break;
            case(950):  tdelay[950] = 91.288516; pan[950] = .825;   break;
            case(951):  tdelay[951] = 91.403337; pan[951] = -.535;  break;
            case(952):  tdelay[952] = 91.474009; pan[952] = .535;   break;
            case(953):  tdelay[953] = 91.613701; pan[953] = -.265;  break;
            case(954):  tdelay[954] = 91.710554; pan[954] = .265;   break;
            case(955):  tdelay[955] = 91.800000; pan[955] = -.475;  break;
            case(956):  tdelay[956] = 91.933349; pan[956] = .475;   break;
            case(957):  tdelay[957] = 92.032532; pan[957] = .11;    break;
            case(958):  tdelay[958] = 92.122216; pan[958] = -.77;   break;
            case(959):  tdelay[959] = 92.194122; pan[959] = -.11;   break;
            case(960):  tdelay[960] = 92.265021; pan[960] = .77;    break;
            case(961):  tdelay[961] = 92.358764; pan[961] = -.475;  break;
            case(962):  tdelay[962] = 92.445276; pan[962] = .475;   break;
            case(963):  tdelay[963] = 92.539293; pan[963] = .725;   break;
            case(964):  tdelay[964] = 92.609978; pan[964] = -.725;  break;
            case(965):  tdelay[965] = 92.710640; pan[965] = .235;   break;
            case(966):  tdelay[966] = 92.831968; pan[966] = -.235;  break;
            case(967):  tdelay[967] = 92.933759; pan[967] = -.685;  break;
            case(968):  tdelay[968] = 93.031102; pan[968] = .685;   break;
            case(969):  tdelay[969] = 93.120963; pan[969] = .315;   break;
            case(970):  tdelay[970] = 93.230907; pan[970] = -.315;  break;
            case(971):  tdelay[971] = 93.322181; pan[971] = -.136;  break;
            case(972):  tdelay[972] = 93.426997; pan[972] = .136;   break;
            case(973):  tdelay[973] = 93.531689; pan[973] = -.635;  break;
            case(974):  tdelay[974] = 93.607012; pan[974] = .635;   break;
            case(975):  tdelay[975] = 93.715148; pan[975] = -.435;  break;
            case(976):  tdelay[976] = 93.803142; pan[976] = .435;   break;
            case(977):  tdelay[977] = 93.951037; pan[977] = -.765;  break;
            case(978):  tdelay[978] = 94.018695; pan[978] = .765;   break;
            case(979):  tdelay[979] = 94.107151; pan[979] = -.575;  break;
            case(980):  tdelay[980] = 94.240962; pan[980] = .575;   break;
            case(981):  tdelay[981] = 94.312251; pan[981] = .215;   break;
            case(982):  tdelay[982] = 94.422676; pan[982] = -.692;  break;
            case(983):  tdelay[983] = 94.514122; pan[983] = -.215;  break;
            case(984):  tdelay[984] = 94.637713; pan[984] = .692;   break;
            case(985):  tdelay[985] = 94.730468; pan[985] = -.435;  break;
            case(986):  tdelay[986] = 94.808751; pan[986] = .435;   break;
            case(987):  tdelay[987] = 94.933759; pan[987] = .675;   break;
            case(988):  tdelay[988] = 95.013019; pan[988] = -.675;  break;
            case(989):  tdelay[989] = 95.107151; pan[989] = .265;   break;
            case(990):  tdelay[990] = 95.240962; pan[990] = -.265;  break;
            case(991):  tdelay[991] = 95.312251; pan[991] = -.555;  break;
            case(992):  tdelay[992] = 95.422676; pan[992] = .555;   break;
            case(993):  tdelay[993] = 95.514122; pan[993] = .333;   break;
            case(994):  tdelay[994] = 95.637713; pan[994] = -.333;  break;
            case(995):  tdelay[995] = 95.730468; pan[995] = 0.0;    break;
            case(996):  tdelay[996] = 95.831968; pan[996] = -1;     break;
            case(997):  tdelay[997] = 95.933759; pan[997] = 1;      break;
            case(998):  tdelay[998] = 96.000000; pan[998] = .475;   break;
            case(999):  tdelay[999] = 96.103567; pan[999] = -.475;  break;
            }
        }
    }
    return(FINISHED);
}

/************************** GENERATE_DELAYS ****************************/

int generate_delays(dataptr dz)
{
    int    n;
    int    eccnt = dz->iparam[STAD_ECHOCNT];
    int   *delay;
    double *tdelay = dz->parray[STAD_TDELAY];
    double sr = (double)dz->infile->srate;
    /*RWD Jan 2009 - trap files with more than  2 chans! */
    if(dz->itemcnt > 2){
        sprintf(errstr,"File has more than two channels.\n");
        return GOAL_FAILED;

    }
    if((dz->lparray[STAD_DELAY] = malloc(MAX_ECHOCNT * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory for stadium arrays.\n");
        return(MEMORY_ERROR);
    }
    delay = dz->lparray[STAD_DELAY];
    for(n=0;n<eccnt;n++) {
        if((delay[n] = (int)round(tdelay[n] * sr * dz->param[STAD_SIZE]) * STEREO)<0) {
            sprintf(errstr,"Delay time too long.\n");
            return(GOAL_FAILED);
        }
    }
    return(FINISHED);
}

/************************** GENERATE_GAINS ****************************/

void generate_gains(dataptr dz)
{
    int n;
    double pregain = dz->param[STAD_PREGAIN];
    double *gain   = dz->parray[STAD_GAIN];
    double *gainl  = dz->parray[STAD_GAINL];
    double *gainr  = dz->parray[STAD_GAINR];
    double *pan    = dz->parray[STAD_PAN];
    double rolloff = dz->param[STAD_ROLLOFF];
    int    eccnt   = dz->iparam[STAD_ECHOCNT];
    if(rolloff<1.0) {
        for(n=0;n<eccnt;n++)
            gain[n] = pow(gain[n],rolloff);
    }
    for(n=0;n<eccnt;n++) {
        if(pan[n] < 0.0) {  /* LEFTWARDS */
            gainl[n] = gain[n] * pregain;
            gainr[n] = gain[n] * pregain * (1.0+pan[n]);
        } else {           /* RIGHTWARDS */
            gainl[n] = gain[n] * pregain * (1.0-pan[n]);
            gainr[n] = gain[n] * pregain;
        }
    }
}

/******************************** CREATE_STADIUM_BUFFERS **********************************/

#define IBUF    (0)
#define OBUF    (1)
#define OBUFEND (2)
#define PADSTRT (3)

int create_stadium_buffers(dataptr dz)
{
    int bigbufsize,bufwanted;
    int obuflen;
    int cnt;
    float *ibufend;
    int overflowlen;
    dz->iparam[STAD_OVFLWSZ] = dz->iparam[STAD_MAXDELAY] * sizeof(float) ;
    if(dz->itemcnt == 1)    /* storing channel-count of infile */
        cnt = 3;
    else
        cnt = 4;
    overflowlen  = dz->iparam[STAD_MAXDELAY];
    bigbufsize = (int)(long) Malloc(-1);
// TW modified overflow check, to include dz->iparam[STAD_OVFLWSZ]
    if((bufwanted = bigbufsize + dz->iparam[STAD_OVFLWSZ]) < 0) {
        sprintf(errstr,"Insufficient memory for delay buffer & snd buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->bigbuf = (float *) malloc(bufwanted))==NULL){
        sprintf(errstr,"Unable to acquire %d samps for delay and sound buffers.\n",bufwanted);
        return MEMORY_ERROR;
    }
    bigbufsize /= cnt;
    dz->buflen  = bigbufsize/sizeof(float);
    obuflen     = dz->buflen * STEREO;
    dz->sampbuf[IBUF]    = dz->bigbuf;
    ibufend              = dz->sampbuf[IBUF] + (dz->buflen * dz->itemcnt);
    dz->sampbuf[OBUF]    = ibufend;
    dz->sampbuf[OBUFEND] = dz->sampbuf[OBUF] + obuflen;
    dz->sampbuf[PADSTRT] = dz->sampbuf[OBUF] + overflowlen;
    memset((char *)dz->bigbuf,0,(dz->buflen * sizeof(float) * cnt) + dz->iparam[STAD_OVFLWSZ]);
    return(FINISHED);
}

/******************************** DO_STADIUM **********************************/

int do_stadium(dataptr dz)
{
    int exit_status;
    int n, m, chans = dz->itemcnt; /* stores channel-cnt of infile */
    double sum, maxinsamp = 0.0, maxoutsamp = 0.0, normaliser = 1.0;
    int icnt, ocnt, sampcnt = 0, samps_to_write, samps_left = 0, k;
    int   *delay  = dz->lparray[STAD_DELAY];
    double *gainl  = dz->parray[STAD_GAINL];
    double *gainr  = dz->parray[STAD_GAINR];
    /*int   ibuflen = dz->buflen;*/
    int   obuflen = dz->buflen * STEREO;
    float  *ibuf   = dz->sampbuf[IBUF];
    float  *obuf   = dz->sampbuf[OBUF];
    int    eccnt   = dz->iparam[STAD_ECHOCNT];
    
    if(dz->vflag[0]) {
        if(sloom) {
            fprintf(stdout,"INFO: Checking output level\n");
            fflush(stdout);
        }
        while((dz->ssampsread  = fgetfbufEx(ibuf,(dz->buflen * dz->itemcnt),dz->ifd[0],0)) > 0) {
            if(chans >= 2) {
                sum = 0.0;
                for(n = 0,m = 0; n < dz->ssampsread; n+=chans, m++) {
                    for(k=0;k<chans;k++)
                        sum += ibuf[n+k];
                    ibuf[m] = (float)(sum/(double)chans);
                    if(fabs(ibuf[m]) > maxinsamp)
                        maxinsamp = fabs(ibuf[m]);
                }
                dz->ssampsread /= chans;
            } else {
                for(n = 0; n < dz->ssampsread; n++) {
                    if(fabs(ibuf[n]) > maxinsamp)
                        maxinsamp = fabs(ibuf[n]);
                }
            }
            for(n=0;n<eccnt;n++) {
                for(icnt=0,ocnt=0;icnt<dz->ssampsread;icnt++,ocnt+=STEREO) {
                    sampcnt = ocnt + delay[n];
                    obuf[sampcnt] += (float) /*round*/(ibuf[icnt] * gainl[n]);
                    sampcnt++;
                    obuf[sampcnt] += (float) /*round*/(ibuf[icnt] * gainr[n]);
                }
            }
            sampcnt++;
            if((samps_to_write = min(obuflen,sampcnt))>0) {
                for(k=0;k < samps_to_write;k++) {
                    if(fabs(obuf[k]) > maxoutsamp)
                        maxoutsamp = fabs(obuf[k]);
                }
            }
            samps_left = sampcnt - obuflen;
            memcpy((char *)obuf,(char *)dz->sampbuf[OBUFEND],dz->iparam[STAD_OVFLWSZ]);
            memset((char *)dz->sampbuf[PADSTRT],0,obuflen * sizeof(float));
        }
        if(dz->ssampsread < 0) {
            sprintf(errstr,"Sound read error.\n");
            return(SYSTEM_ERROR);
        }
        if(samps_left > 0) {
            for(k=0;k < samps_left;k++) {
                if(fabs(obuf[k]) > maxoutsamp)
                    maxoutsamp = fabs(obuf[k]);
            }
        }
        if(flteq(maxinsamp,0.0) || flteq(maxoutsamp,0.0)) {
            sprintf(errstr,"No significant level in input file.\n");
            return DATA_ERROR;
        }
        if(maxoutsamp > 0.95)
            normaliser = 0.95/maxoutsamp;
        else if(maxoutsamp < maxinsamp)
            normaliser = maxoutsamp/maxinsamp;
        memset((char *)dz->bigbuf,0,(dz->buflen * sizeof(float) * 3) + dz->iparam[STAD_OVFLWSZ]);
        sndseekEx(dz->ifd[0],0,0);
        reset_filedata_counters(dz);
        if(sloom) {
            fprintf(stdout,"INFO: Generating stadium reverb.\n");
            fflush(stdout);
        }
    }
    while((dz->ssampsread  = fgetfbufEx(ibuf,(dz->buflen * dz->itemcnt),dz->ifd[0],0)) > 0) {
        if(chans >= 2) {
            for(n = 0,m = 0; n < dz->ssampsread; n+=chans, m++) {
                sum = 0.0;
                for(k=0;k<chans;k++)
                    sum += ibuf[n+k];
                ibuf[m] = (float)(sum/(double)chans);
            }
            dz->ssampsread /= chans;
        }
        for(n=0;n<eccnt;n++) {
            for(icnt=0,ocnt=0;icnt<dz->ssampsread;icnt++,ocnt+=STEREO) {
                sampcnt = ocnt + delay[n];
                obuf[sampcnt] += (float) /*round*/(ibuf[icnt] * gainl[n]);
                sampcnt++;
                obuf[sampcnt] += (float) /*round*/(ibuf[icnt] * gainr[n]);
            }
        }
        sampcnt++;
        if((samps_to_write = min(obuflen,sampcnt))>0) {
            if(dz->vflag[0]) {
                for(k=0;k < samps_to_write;k++)
                    obuf[k] = (float)(obuf[k] * normaliser);
            }
            if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
                return(exit_status);
        }
        samps_left = sampcnt - obuflen;
        memcpy((char *)obuf,(char *)dz->sampbuf[OBUFEND],dz->iparam[STAD_OVFLWSZ]);
        memset((char *)dz->sampbuf[PADSTRT],0,obuflen * sizeof(float));
    }
    if(dz->ssampsread < 0) {
        sprintf(errstr,"Sound read error.\n");
        return(SYSTEM_ERROR);
    }
    if(samps_left > 0) {
        if(dz->vflag[0]) {
            for(k=0;k < samps_left;k++)
                obuf[k] = (float)(obuf[k] * normaliser);
        }
        return write_samps(obuf,samps_left,dz);
    }
    return(FINISHED);
}

//TW UPDATE : NEW FUNCTIONS (converted for flotsams)
/******************************** DO_CONVOLVE **********************************/
int do_convolve(dataptr dz)
{
    int exit_status, chans = dz->infile->channels, k;
    int n = 0, m = 0, dl, yy, ynext, nn, mm, samps_read, maxwrite, tt = 0;
    double *dbuf, scaler, maxval = 0.0, d, cbufval, clumpcnt = (double)(dz->insams[1]/chans);
    double timeincr = 1.0/(double)dz->infile->srate, thistime;
    float *cbuf, *ibuf, *obuf;
    int cfile_len = dz->insams[1], samps_to_write;
    unsigned int dblovflw = cfile_len * sizeof(double);
    dz->total_samps_read = 0;
    if((samps_read  = fgetfbufEx(dz->sampbuf[1],dz->insams[1],dz->ifd[1],0)) < 0) {     
        sprintf(errstr,"Can't read samps from soundfile to be convolved.\n");
        return(SYSTEM_ERROR);
    }
    dz->total_samps_read += samps_read;
    if((samps_read  = fgetfbufEx(dz->sampbuf[0],dz->buflen,dz->ifd[0],0)) < 0) {        
        sprintf(errstr,"Can't read samps from convolving soundfile.\n");
        return(SYSTEM_ERROR);
    }
    if(samps_read == 0) {
        sprintf(errstr,"No data found in soundfile to be convolved.\n");
        return(SYSTEM_ERROR);
    }
    dz->ssampsread = samps_read;
    dz->total_samps_read += samps_read;
    print_outmessage_flush("Calculating scaling factor. (This is a SLOW process)\n");
    do {
        for(k=0;k<chans;k++) {
            ibuf = dz->sampbuf[0] + k;
            dbuf = dz->parray[0] + k;
            switch(dz->mode) {
            case(CONV_NORMAL):
                for(n = k; n < dz->ssampsread; n+=chans,tt++) {
                    nn = n - k;
                    cbuf = dz->sampbuf[1] + k;
                    for(m = k; m < dz->insams[1]; m+=chans) {
                        mm = m - k;
                        dbuf[nn+mm] += *ibuf * *cbuf;
                        cbuf += chans;
                    }
                    ibuf += chans;
                    if(tt % dz->infile->srate == 0)
                        display_virtual_time(tt,dz);
                }
                break;
            case(CONV_TVAR):
                for(n = k, thistime = 0.0; n < dz->ssampsread; n+=chans, thistime += timeincr,tt++) {
                    nn = n - k;
                    if(dz->brksize[CONV_TRANS] && !(nn%256)) {
                        if((exit_status = read_value_from_brktable(thistime,CONV_TRANS,dz))<0)
                            return(exit_status);
                    }
//TW Omission corrected DEC 2002
                    cbuf = dz->sampbuf[1] + k;
                    for(m = k, d = 0.0; d <= clumpcnt; m+=chans, d+= dz->param[CONV_TRANS]) {
                        mm = m - k;
                        d += max(dz->param[CONV_TRANS],0.0);
                        dl = (int)floor(d);
                        yy = dl * chans;
                        ynext = yy+chans;
                        cbufval = (double)cbuf[yy] + ((d - (double)dl) * (double)(cbuf[ynext] - cbuf[yy]));
                        dbuf[nn+mm] += *ibuf * cbufval;
                    }
                    ibuf += chans;
                    if(tt % dz->infile->srate == 0)
                        display_virtual_time(tt,dz);
                }
                break;
            }
        }
        maxwrite = (n-chans) + (m-chans) + 1;
        dbuf = dz->parray[0];
        if (samps_read >= dz->buflen) {
            for(n = 0; n < dz->ssampsread; n++)
                maxval = max(maxval,fabs(dbuf[n]));
//          display_virtual_time(dz->total_samps_read,dz);
            memset((char *)dbuf,0,dz->buflen * sizeof(double));
            memcpy((char *)dbuf,(char *)(dbuf + dz->buflen),dblovflw);
            memset((char *)(dbuf + dz->buflen),0,dblovflw);
            maxwrite -= dz->buflen;
            if((samps_read  = fgetfbufEx(dz->sampbuf[0],dz->buflen,dz->ifd[0],0)) < 0) {        
                sprintf(errstr,"Can't read samps from convolving soundfile.\n");
                return(SYSTEM_ERROR);
            }
            dz->ssampsread = samps_read;
            dz->total_samps_read += samps_read;
            display_virtual_time(dz->total_samps_read,dz);
        } else {
            if(maxwrite > 0) {
                for(n = 0; n < maxwrite; n++)
                    maxval = max(maxval,fabs(dbuf[n]));
            }
            break;
        }               
    } while(samps_read > 0);
    dbuf = dz->parray[0];
    scaler = F_MAXSAMP/maxval;
    memset((char *)dbuf,0,((dz->buflen + cfile_len) * sizeof(double)));
    dz->total_samps_read = 0;
    display_virtual_time(0,dz);
    print_outmessage_flush("Doing the convolution. (This is an even SLOWER process)\n");
    sndseekEx(dz->ifd[0],0,0);
    if((samps_read  = fgetfbufEx(dz->sampbuf[0],dz->buflen,dz->ifd[0],0)) < 0) {        
        sprintf(errstr,"Can't read samps from convolving soundfile.\n");
        return(SYSTEM_ERROR);
    }
    dz->ssampsread = samps_read;
    dz->total_samps_read += samps_read;
    if(samps_read == 0) {
        sprintf(errstr,"No data found in soundfile to be convolved, on 2nd pass.\n");
        return(SYSTEM_ERROR);
    }
    obuf = dz->sampbuf[2];
    tt = 0;
    do {
        for(k=0;k<chans;k++) {
            ibuf = dz->sampbuf[0] + k;
            dbuf = dz->parray[0] + k;
            switch(dz->mode) {
            case(CONV_NORMAL):
                for(n = k; n < dz->ssampsread; n+=chans,tt++) {
                    nn = n - k;
                    cbuf = dz->sampbuf[1] + k;
                    for(m = k; m < dz->insams[1]; m+=chans) {
                        mm = m - k;
                        dbuf[nn+mm] += *ibuf * *cbuf;
                        cbuf += chans;
                    }
                    ibuf += chans;
                    if(tt % dz->infile->srate == 0)
                        display_virtual_time(tt,dz);
                }
                break;
            case(CONV_TVAR):
                for(n = k, thistime = 0.0; n < dz->ssampsread; n+=chans, thistime += timeincr,tt++) {
                    nn = n - k;
                    if(dz->brksize[CONV_TRANS] && !(nn%256)) {
                        if((exit_status = read_value_from_brktable(thistime,CONV_TRANS,dz))<0)
                            return(exit_status);
                    }
//TW Omission corrected DEC 2002
                    cbuf = dz->sampbuf[1] + k;
                    for(mm = 0, d = 0.0; d <= clumpcnt; mm+=chans, d+= dz->param[CONV_TRANS]) {
                        d += max(dz->param[CONV_TRANS],0.0);
                        dl = (int)floor(d);
                        yy = dl * chans;
                        ynext = yy+chans;
                        cbufval = (double)cbuf[yy] + ((d - (double)dl) * (double)(cbuf[ynext] - cbuf[yy]));
                        dbuf[nn+mm] += *ibuf * cbufval;
                    }
                    ibuf += chans;
                    if(tt % dz->infile->srate == 0)
                        display_virtual_time(tt,dz);
                }
                break;
            }
        }
        maxwrite = (n-chans) + (m-chans) + 1;
        dbuf = dz->parray[0];
        if(samps_read >= dz->buflen) {
            for(n=0;n<dz->ssampsread;n++)
                obuf[n] = (float)(dbuf[n] * scaler);
            if((exit_status = write_samps(dz->sampbuf[2],samps_read,dz))<0)
                return(exit_status);
//          display_virtual_time(dz->total_samps_written,dz);
            memset((char *)dz->parray[0],0,dz->buflen * sizeof(double));
            memcpy((char *)dz->parray[0],(char *)(dz->parray[0] + dz->buflen),dblovflw);
            memset((char *)(dz->parray[0] + dz->buflen),0,dblovflw);
            maxwrite -= dz->buflen;
            if((samps_read = fgetfbufEx(dz->sampbuf[0],dz->buflen,dz->ifd[0],0)) < 0) {     
                sprintf(errstr,"Can't read samps from convolving soundfile.\n");
                return(SYSTEM_ERROR);
            }
            dz->ssampsread = samps_read;
            dz->total_samps_read += samps_read;
            display_virtual_time(dz->total_samps_read,dz);
        } else {
            m = 0;
            while(maxwrite > 0) {
                samps_to_write = min(maxwrite,dz->buflen);
                for(n=0;n<samps_to_write;n++) {
                    obuf[n] = (float)(dbuf[m++] * scaler);
                }
                if((exit_status = write_samps(dz->sampbuf[2],samps_to_write,dz))<0)
                    return(exit_status);
                display_virtual_time(dz->total_samps_written,dz);
                maxwrite -= samps_to_write;
            }
            break;
        }
    } while(samps_read > 0);
    return(FINISHED);
}

/******************************** CONVOLVE_PREPROCESS **********************************/

int convolve_preprocess(dataptr dz)
{
    double *p, *brkend;
    if(dz->brksize[CONV_TRANS]) {
        brkend = dz->brk[CONV_TRANS] + (2 * dz->brksize[CONV_TRANS]);
        p = dz->brk[CONV_TRANS] + 1;
        while(p < brkend) {
            *p = pow(2.0,(*p/SEMITONES_PER_OCTAVE));
            p += 2;
        }
    } else
        dz->param[CONV_TRANS] = pow(2.0,(dz->param[CONV_TRANS]/SEMITONES_PER_OCTAVE));
    return(FINISHED);
}
