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

/* Sept 05: TW fix for formants put */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <filetype.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>

#define round(x) lround((x))

#define CHAN_SRCHRANGE_F    (4)

static int  set_specenv_frqs(int arraycnt,dataptr dz);
static int  setup_octaveband_steps(double **interval,dataptr dz);
static int  setup_low_octave_bands(int arraycnt,dataptr);

/************************** INITIALISE_SPECENV *********************
 *
 *  MAR 1998: not sure if the follwoing comment is relevant any more
 *  but I wont risk changing it at this stage.
 *
 *  WANTED and CLENGTH are calculated from scratch here, as dz->wanted
 *  gets set equal to dz->specenvcnt for calculations on formant data,
 *  while dz->clength may not yet be set!!      
 */

int initialise_specenv(int *arraycnt,dataptr dz)
{
    int wanted,clength;
    switch(dz->infile->filetype) {
    case(ANALFILE): 
        wanted  = dz->infile->channels; 
        break;
    case(FORMANTFILE):
    case(PITCHFILE):
    case(TRANSPOSFILE):
        wanted  = dz->infile->origchans;
        break;
    default:
        sprintf(errstr,"Unknown original filetype: initialise_specenv()\n");
        return(PROGRAM_ERROR);
    }
    clength = wanted/2;
    *arraycnt   = clength + 1;
    if((dz->specenvfrq = (float *)malloc((*arraycnt) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant frq array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->specenvpch = (float *)malloc((*arraycnt) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant pitch array.\n");
        return(MEMORY_ERROR);
    }
    /*RWD  zero the data */
    memset(dz->specenvpch,0,*arraycnt * sizeof(float));

    if((dz->specenvamp = (float *)malloc((*arraycnt) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant aplitude array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->specenvtop = (float *)malloc((*arraycnt) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant frq limit array.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/************************** READ_FORMANTBAND_DATA_AND_SETUP_FORMANTS *********************/

int read_formantband_data_and_setup_formants(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
    int exit_status;
    int max_fbands = 0, arraycnt;
    aplptr ap = dz->application;
    if((exit_status = establish_formant_band_ranges(dz->infile->channels,ap))<0)
        return(exit_status);

    if(!sloom) {
        if(strlen((*cmdline)[0])<3) {
            sprintf(errstr,"Formant parameter missing on cmdline.\n");
            return(USAGE_ONLY);
        }
        if(!strncmp((*cmdline)[0],"-f",2))
            dz->specenv_type = FREQWISE_FORMANTS;           
        else if(!strncmp((*cmdline)[0],"-p",2))
            dz->specenv_type = PICHWISE_FORMANTS;           
        else {
            sprintf(errstr,"Formant flag missing on cmdline.\n");
            return(USAGE_ONLY);
        }
        if(dz->specenv_type==PICHWISE_FORMANTS && ap->no_pitchwise_formants) {
            sprintf(errstr,"-p flag can't be used: Too few analysis data channels.\n");
            return(DATA_ERROR);
        }
        (*cmdline)[0] += 2;
        if(sscanf((*cmdline)[0],"%d",&(dz->formant_bands))!=1) {
            sprintf(errstr,"Cannot read count of formant_bands.\n");
            return(USAGE_ONLY);
        }
        (*cmdline)++;
        (*cmdlinecnt)--;
    } else {
        if((*cmdline)[0][0]!=NUMERICVAL_MARKER) {   /* TK  convention, values are preceded by '@' */
            sprintf(errstr,"Invalid value for Formant bands switch sent from TK\n");
            return(DATA_ERROR);
        }
        ((*cmdline)[0])++;
        switch((*cmdline)[0][0]) {
        case('0'):  dz->specenv_type = PICHWISE_FORMANTS;   break;          
        case('1'):  dz->specenv_type = FREQWISE_FORMANTS;   break;          
        default:
            sprintf(errstr,"Bad value (%c) for Formant Flag sent from TK\n",((*cmdline)[0][0]));
            return(DATA_ERROR);
        }
        (*cmdline)++;
        (*cmdlinecnt)--;

        if((*cmdline)[0][0]!=NUMERICVAL_MARKER) {   /* TK  convention, values are preceded by '@' */
            sprintf(errstr,"Invalid value for Formant bands sent from TK\n");
            return(DATA_ERROR);
        }
        ((*cmdline)[0])++;
        if(sscanf((*cmdline)[0],"%d",&(dz->formant_bands))!=1) {
            sprintf(errstr,"No formant band count sent from TK.\n");
            return(USAGE_ONLY);
        }
        (*cmdline)++;
        (*cmdlinecnt)--;
    }
    switch(dz->specenv_type) {
    case(PICHWISE_FORMANTS): max_fbands = ap->max_pichwise_fbands;  break;
    case(FREQWISE_FORMANTS): max_fbands = ap->max_freqwise_fbands;  break;
    }
    if(dz->formant_bands < ap->min_fbands) {
        sprintf(errstr,"Too few formant_bands requested: min for this file is %d\n",ap->min_fbands);
        return(DATA_ERROR);
    }
    if(dz->formant_bands > max_fbands) {
        sprintf(errstr,"Too many formant_bands requested: max for this file is %d\n",max_fbands);
        return(DATA_ERROR);
    }
    if((exit_status = initialise_specenv(&arraycnt,dz))<0)
        return(exit_status);
    if((exit_status = set_specenv_frqs(arraycnt,dz))<0)
        return(exit_status);
    dz->descriptor_samps = dz->infile->specenvcnt * DESCRIPTOR_DATA_BLOKS ;
    return(FINISHED);
}

/************************ SET_SPECENV_FRQS ************************
 *
 * FREQWISE BANDS = number of channels for each specenv point
 * PICHWISE BANDS  = number of points per octave
 */

int set_specenv_frqs(int arraycnt,dataptr dz)
{
    int exit_status;
    double frqstep, thisfrq, nextfrq, bandbot;
    double *interval;
    int m, n, k = 0, cc;
    switch(dz->specenv_type) {
    case(FREQWISE_FORMANTS):
        dz->specenvamp[0] = (float)0.0;
        dz->specenvfrq[0] = (float)1.0;
        dz->specenvpch[0] = (float)log10(dz->specenvfrq[0]);
        dz->specenvtop[0] = (float)dz->halfchwidth;
        frqstep = dz->halfchwidth * (double)dz->formant_bands;
        thisfrq = dz->specenvtop[0];
        k = 1;
        while((nextfrq = thisfrq + frqstep) < dz->nyquist) {
            if(k >= arraycnt) {
                sprintf(errstr,"Formant array too small: set_specenv_frqs()\n");
                return(PROGRAM_ERROR);
            }
            dz->specenvfrq[k] = (float)nextfrq;
            dz->specenvpch[k] = (float)log10(dz->specenvfrq[k]);
            nextfrq          += frqstep;
            dz->specenvtop[k] = (float)min(dz->nyquist,nextfrq);
            thisfrq           = nextfrq;
            k++;
        }
        break;
    case(PICHWISE_FORMANTS):
        if((exit_status = setup_octaveband_steps(&interval,dz))<0)
            return(exit_status);
        if((exit_status = setup_low_octave_bands(arraycnt,dz))<0)
            return(exit_status);
        k  = TOP_OF_LOW_OCTAVE_BANDS;
        cc = CHAN_ABOVE_LOW_OCTAVES;
        while(cc <= dz->clength) {
            m = 0;
            if((bandbot = dz->chwidth * (double)cc) >= dz->nyquist)
                break;
            for(n=0;n<dz->formant_bands;n++) {
                if(k >= arraycnt) {
                    sprintf(errstr,"Formant array too small: set_specenv_frqs()\n");
                    return(PROGRAM_ERROR);
                }
                dz->specenvfrq[k]   = (float)(bandbot * interval[m++]);
                dz->specenvpch[k]   = (float)log10(dz->specenvfrq[k]);
                dz->specenvtop[k++] = (float)(bandbot * interval[m++]);
            }   
            cc *= 2;        /* 8-16: 16-32: 32-64 etc as 8vas, then split into bands */
        }
        break;
    }
    dz->specenvfrq[k] = (float)dz->nyquist;
    dz->specenvpch[k] = (float)log10(dz->nyquist);
    dz->specenvtop[k] = (float)dz->nyquist;
    dz->specenvamp[k] = (float)0.0;
    k++;
    dz->infile->specenvcnt = k;
    return(FINISHED);
}

/************************* SETUP_OCTAVEBAND_STEPS ************************/

int setup_octaveband_steps(double **interval,dataptr dz)
{
    double octave_step;
    int n = 1, m = 0, halfbands_per_octave = dz->formant_bands * 2;
    if((*interval = (double *)malloc(halfbands_per_octave * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing interval array for formants.\n");
        return(MEMORY_ERROR);
    }
    while(n < halfbands_per_octave) {
        octave_step   = (double)n++/(double)halfbands_per_octave;
        (*interval)[m++] = pow(2.0,octave_step);
    }
    (*interval)[m] = 2.0;
    return(FINISHED);
}

/************************ SETUP_LOW_OCTAVE_BANDS ***********************
 *
 * Lowest PVOC channels span larger freq steps and therefore we must
 * group them in octave bands, rather than in anything smaller.
 */

int setup_low_octave_bands(int arraycnt,dataptr dz)
{
    int n;
    if(arraycnt < LOW_OCTAVE_BANDS) {
        sprintf(errstr,"Insufficient array space for low_octave_bands\n");
        return(PROGRAM_ERROR);
    }
    for(n=0;n<LOW_OCTAVE_BANDS;n++) {
        switch(n) {
        case(0):
            dz->specenvfrq[0] = (float)1.0;                 /* frq whose log is 0 */
            dz->specenvpch[0] = (float)0.0;
            dz->specenvtop[0] = (float)dz->chwidth;         /* 8VA wide bands */
            break;
        case(1):
            dz->specenvfrq[1] = (float)(dz->chwidth * 1.5); /* Centre Chs 1-2 */    
            dz->specenvpch[1] = (float)log10(dz->specenvfrq[1]);
            dz->specenvtop[1] = (float)(dz->chwidth * 2.0);
            break;
        case(2):
            dz->specenvfrq[2] = (float)(dz->chwidth * 3.0); /* Centre Chs 2-4 */
            dz->specenvpch[2] = (float)log10(dz->specenvfrq[2]);
            dz->specenvtop[2] = (float)(dz->chwidth * 4.0);
            break;
        case(3):
            dz->specenvfrq[3] = (float)(dz->chwidth * 6.0); /* Centre Chs 4-8 */    
            dz->specenvpch[3] = (float)log10(dz->specenvfrq[3]);
            dz->specenvtop[3] = (float)(dz->chwidth * 8.0);
            break;
        default:
            sprintf(errstr,"Insufficient low octave band setups in setup_low_octave_bands()\n");
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/**************************** GETSPECENVAMP *************************/

int getspecenvamp(double *thisamp,double thisfrq,int storeno,dataptr dz)
{
    double pp, ratio, ampdiff;
    float *ampstore;
    int z = 1;
    if(thisfrq<0.0) {   /* NOT SURE THIS IS CORRECT */
        *thisamp = 0.0; /* SHOULD WE PHASE INVERT & RETURN A -ve AMP ?? */
        return(FINISHED);   
    }
    if(thisfrq<=1.0)
        pp = 0.0;
    else
        pp = log10(thisfrq); 
    while( dz->specenvpch[z] < pp){
        z++;
        /*RWD may need to trap on size of array? */
        if(z == dz->infile->specenvcnt -1)
            break;
    }
    switch(storeno) {
    case(0):    
        ampstore = dz->specenvamp;  break;
    case(1):    
        ampstore = dz->specenvamp2; break;
    default:
        sprintf(errstr,"Unknown storenumber in getspecenvamp()\n");
        return(PROGRAM_ERROR);
    }
    ratio    = (pp - dz->specenvpch[z-1])/(dz->specenvpch[z] - dz->specenvpch[z-1]);
    ampdiff  = ampstore[z] - ampstore[z-1];
    *thisamp = ampstore[z-1] + (ampdiff * ratio);
    *thisamp = max(0.0,*thisamp);
    return(FINISHED);
}

/********************** EXTRACT_SPECENV *******************/

int extract_specenv(int bufptr_no,int storeno,dataptr dz)    /* bufptr_no = flbufptr number (usually 0) */
{
    int    n, cc, vc, specenvcnt_less_one;
    int    botchan, topchan;
    double botfreq, topfreq;
    double bwidth_in_chans;
    float *ampstore;
    switch(storeno) {
    case(0):    ampstore = dz->specenvamp;  break;
    case(1):    ampstore = dz->specenvamp2; break;
    default:
        sprintf(errstr,"Unknown storenumber in extract_specenv()\n");
        return(PROGRAM_ERROR);
    }
    specenvcnt_less_one = dz->infile->specenvcnt - 1;
    vc = 0;
    switch(dz->deal_with_chan_data) {
    case(RECTIFY_CHANNEL_FRQ_ORDER):
        for(n=0;n<dz->infile->specenvcnt;n++)
            ampstore[n] = (float)0.0;
        topfreq = 0.0f;
        n = 0;
        while(n < specenvcnt_less_one) {
            botfreq  = topfreq;
            botchan  = (int)((botfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
            botchan -= CHAN_SRCHRANGE_F;
            botchan  =  max(botchan,0);
            topfreq  = dz->specenvtop[n];
            topchan  = (int)((topfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
            topchan += CHAN_SRCHRANGE_F;
            topchan  =  min(topchan,dz->clength);
            for(cc = botchan,vc = botchan * 2; cc < topchan; cc++,vc += 2) {
                if(dz->flbufptr[bufptr_no][FREQ] >= botfreq && dz->flbufptr[bufptr_no][FREQ] < topfreq)
                    ampstore[n] = (float)(ampstore[n] + dz->flbufptr[bufptr_no][AMPP]);
            }
            bwidth_in_chans   = (double)(topfreq - botfreq)/dz->chwidth;
            ampstore[n] = (float)(ampstore[n]/bwidth_in_chans);
            n++;
        }
        break;
    case(IGNORE_ACTUAL_CHAN_FRQS):
        for(n=1;n<specenvcnt_less_one;n++)
            ampstore[n] = (float)0.0;
        ampstore[0] = (float)(dz->flbufptr[bufptr_no][0]/2.0); /* dz->halfchwidth WIDE CHANNEL */
        cc = 1;
        vc = 2;
        n  = 1;                  /* OMIT LOWEST CHANNEL */
        topfreq = dz->halfchwidth;
        while(cc < dz->clength  && n < specenvcnt_less_one) {
            botfreq  = topfreq;
            topfreq += dz->chwidth;
            if(topfreq < dz->specenvtop[n]) {
                if(botfreq >= dz->specenvtop[n-1])  /* WHOLLY IN SPECENVBAND  */
                    ampstore[n]=(float)(ampstore[n] + dz->flbufptr[bufptr_no][AMPP]);
                else                                /* PARTLY IN SPECENVBAND  */
                    ampstore[n]=(float)(ampstore[n]+(dz->flbufptr[bufptr_no][AMPP]/2.0));
                cc++;   
                vc += 2;
            } else if(botfreq < dz->specenvtop[n]) { /* PARTLY IN SPECENVBAND  */
                ampstore[n]=(float)(ampstore[n] + (dz->flbufptr[bufptr_no][AMPP]/2.0));
                cc++;   
                vc += 2;
            } else {                                /* NOT IN SPECENVBAND     */
                bwidth_in_chans = (dz->specenvtop[n]-dz->specenvtop[n-1])/dz->chwidth;
                ampstore[n]   = (float)(ampstore[n]/bwidth_in_chans);
                n++;                                /* GO TO NEXT SPECENVBAND */
            }
        }
        break;
    case(NOT_SET):           //RWD: add default action too...
        sprintf(errstr,"DEAL_WITH_CHAN_DATA flag not set in extract_specenv()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/********************* EXTRACT_SPECENV_OVER_PARTIALS ***********************/

int extract_specenv_over_partials(int *specenvcnt,double thispitch,int bufptr_no,dataptr dz)
{                                            /* bufptr_no = flbufptr number (usually 0) */
    int exit_status;
    int n = 0, k, cc, vc, lastk = 0;
    for(n=0;n<=dz->clength;n++)
        dz->specenvamp[n] = 0.0F;
    n = 0;
    dz->specenvfrq[n]   = 1.0F;
    dz->specenvpch[n++] = 0.0F;
    for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) {
        if((exit_status = is_harmonic(&k,dz->flbufptr[bufptr_no][FREQ],thispitch))<0)
            return(exit_status);
        if(exit_status==FALSE)
            continue;
        if(k==lastk) {
            if(dz->flbufptr[bufptr_no][AMPP] > dz->specenvamp[n]) {
                dz->specenvamp[n] = dz->flbufptr[bufptr_no][AMPP];                  /* GET LOUDEST PARTIAL */
                dz->specenvfrq[n] = (float)max(1.0f,dz->flbufptr[bufptr_no][FREQ]);  /*    NEAR HARMONIC    */
                dz->specenvpch[n] = (float)log10(dz->specenvfrq[n]);
            }
        } else {
            n++;
            dz->specenvamp[n] = dz->flbufptr[bufptr_no][AMPP];  /* KEEP AMP & FRQ OF PARTIAL */
            dz->specenvfrq[n] = (float)max(1.0f,dz->flbufptr[bufptr_no][FREQ]);
            dz->specenvpch[n] = (float)log10(dz->specenvfrq[n]);
            lastk = k;
        }
    }
    n++;
    dz->specenvamp[n]   = 0.0F;
    dz->specenvfrq[n]   = (float)dz->nyquist;
    dz->specenvpch[n] = (float)log10(dz->specenvfrq[n]);
    n++;
    *specenvcnt = n;
    return(FINISHED);
}   

/**************************** IS_HARMONIC *************************
 *
 * WARNING: Checks if FIRST FREQ is harmonic of 2nd: NOT VICE VERSA!!
 */

int is_harmonic(int *iratio,double frq1, double frq2)
{
    double ratio, intvl;
    if(frq2 < FLTERR) {
        sprintf(errstr,"Zero frequency submitted as fundamental to is_harmonic()\n");
        return(PROGRAM_ERROR);
    }
    ratio = frq1/frq2;
    *iratio =  round(ratio);
    if(ratio > *iratio)
        intvl = ratio/(double)(*iratio);
    else
        intvl = (double)(*iratio)/ratio;
    if(intvl > SEMITONE_INTERVAL)
        return(FALSE);
    return(TRUE);
}
                                                    
/************************** INITIALISE_SPECENV2 *********************/

int initialise_specenv2(dataptr dz)
{
    int wanted,clength;
    int n;
    switch(dz->infile->filetype) {
    case(ANALFILE): 
        wanted  = dz->infile->channels; 
        break;
    case(FORMANTFILE):
    case(PITCHFILE):
    case(TRANSPOSFILE):
        wanted  = dz->infile->origchans;
        break;
    default:
        sprintf(errstr,"Unknown original filetype: initialise_specenv2()\n");
        return(PROGRAM_ERROR);
    }
    clength = wanted/2;
    if((dz->specenvamp2 = (float *)malloc((clength+1) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for second formant amplitude array.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->infile->specenvcnt;n++)
        dz->specenvamp2[n]  = 0.0f;
    return(FINISHED);
}

/******************** WRITE_FORMANT_DESCRIPTOR **********************/

int write_formant_descriptor(float **fptr1,float *fptr2,dataptr dz)
{
    int fsamps_to_write = dz->descriptor_samps/DESCRIPTOR_DATA_BLOKS;
    *fptr1 = fptr2;
    memmove((char *)(*fptr1),(char *)dz->specenvpch,(size_t)(fsamps_to_write * sizeof(float)));
    *fptr1 += dz->infile->specenvcnt;
    memmove((char *)(*fptr1),(char *)dz->specenvtop,(size_t)(fsamps_to_write * sizeof(float)));
    *fptr1 += dz->infile->specenvcnt;
    return(FINISHED);
}

/******************** SETUP_FORMANT_PARAMS **********************/

int setup_formant_params(int fd,dataptr dz)
{
    int samps_read;
    /*RWD what the f**** are these? */
    int fsamps_to_read = dz->descriptor_samps/DESCRIPTOR_DATA_BLOKS;
    
    /*RWD need dz->bigbuflen2 ? */
    if(dz->buflen2 <= 0) {
//      sprintf(errstr,"bigbufsize2 not established: setup_formant_params()\n");
        sprintf(errstr,"buflen2 not established: setup_formant_params()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->buflen2 < dz->descriptor_samps) {
//      sprintf(errstr,"bigbufsize2 smaller than descriptor_samps: setup_formant_params()\n");
        sprintf(errstr,"buflen2 smaller than descriptor_samps: setup_formant_params()\n");
        return(PROGRAM_ERROR);
    }
    if((samps_read = fgetfbufEx(dz->flbufptr[2], dz->buflen2,fd,0)) < dz->descriptor_samps) {
        if(samps_read<0) {
            sprintf(errstr,"Sound read error.\n");
            return(SYSTEM_ERROR);
        }
        sprintf(errstr,"No data in formantfile.\n");
        return(DATA_ERROR);
    }
    /*TW Sept 05*/
    dz->ssampsread = samps_read;

    dz->flbufptr[1] = dz->flbufptr[2];
    memmove((char *)dz->specenvpch,(char *)dz->flbufptr[1],(size_t)(fsamps_to_read * sizeof(float)));
    dz->flbufptr[1] += dz->infile->specenvcnt;
    memmove((char *)dz->specenvtop,(char *)dz->flbufptr[1],(size_t)(fsamps_to_read * sizeof(float)));
    dz->flbufptr[1] += dz->infile->specenvcnt;
    return(FINISHED);
}

/****************************** EXTRACT_FORMANT_PEAKS2 ***************************/

int extract_formant_peaks2(int sl1param,int *thispkcnt,double lofrq_limit,double hifrq_limit,dataptr dz)
{
    int n;
    int  falling = 0, last_channel_was_in_range = 1;
    int *fppk = dz->stable->fpk[dz->iparam[sl1param]];
    *thispkcnt = 0;
    n = 1;
    while(n<dz->infile->specenvcnt) {
        if(dz->specenvfrq[n] < lofrq_limit) {
            last_channel_was_in_range = 0;
            n++;
            continue;
        }
        if(dz->specenvamp[n] > dz->specenvamp[n-1])
            falling = 0;
        else {
            if(!falling) {
                if(last_channel_was_in_range)
                    fppk[(*thispkcnt)++] = n-1;
                falling = 1;
            }
        }
        last_channel_was_in_range = 1;
        if(dz->specenvfrq[n] > hifrq_limit)
            break;
        n++;
    }
    return(FINISHED);
}

