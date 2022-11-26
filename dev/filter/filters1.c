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



#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <filters.h>
#include <cdpmain.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <filters.h>
#include <logic.h>

static int    readsamps_with_wrap(dataptr dz);
static double get_gain(dataptr dz);
static double get_pshift(dataptr dz);
static int   get_next_writestart(int write_start,dataptr dz);
static int    iterfilt(int write_start,int *write_end,int *atten_index,
                int thisbuf,double orig_maxsamp,int splicelen,dataptr dz);
static void   reset_filters(dataptr dz);
static void   scale_input(dataptr dz);
static void   amp_smooth(dataptr dz);
static void   do_attenuation(int atten_index, float *ib2,dataptr dz);
static void   filter_normalise(float *thisbuf,double orig_maxsamp,dataptr dz);
static double    get_maxsamp(float *thisbuf,dataptr dz);
static int    do_shift_gain_and_copy_to_outbuf(float *ibuf,float *obuf,int chans,double step,
                    double thisgain,int *write_pos,dataptr dz);
static int    do_iterated_filtering(int atten_index,int write_start,int *write_end,
                    int thisbuf,double orig_maxsamp,int splicelen,dataptr dz);
static void   do_end_splice(float *ibuf,int seglen_in_mono,int splicelen,dataptr dz);

/****************************** ITERATING_FILTER *************************/

int iterating_filter(dataptr dz)
{
    int exit_status;
    int tail;
    float *obuf     = dz->sampbuf[FLT_OUTBUF];
    float *ovflwbuf = dz->sampbuf[FLT_OVFLWBUF];
    int write_start = 0, write_end;
    int atten_index = 2, remaining_dur = dz->iparam[FLT_OUTDUR];
    int thisbuf = 0;
    double orig_maxsamp;
    int splicelen = round(FLT_SPLICELEN * MS_TO_SECS * (double)dz->infile->srate);
    splicelen = min(splicelen,dz->iparam[FLT_INMSAMPSIZE]/2 );     /*RWD 2 signifies ???? */
    if((exit_status = readsamps_with_wrap(dz))<0)
        return(exit_status);
    if(!flteq(dz->param[FLT_PRESCALE],1.0))
        scale_input(dz);
    display_virtual_time(0L,dz);
    if(dz->vflag[FLT_EXPDEC])
        amp_smooth(dz);
    orig_maxsamp = get_maxsamp(dz->sampbuf[0],dz);

    memmove((char *)obuf,(char *)dz->sampbuf[0],(size_t)(dz->insams[0] * sizeof(float)));
    write_start = get_next_writestart(write_start,dz);
    write_end = dz->insams[0];
    while(write_start < remaining_dur) {
        while(write_start >= dz->buflen) {  /* 'while' allows blank buffers to be copied (where delay > filesize) */
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            tail = write_end - dz->buflen;
            memset((char *)obuf,0,(size_t)(dz->buflen * sizeof(float)));
            if(tail > 0) {
                memmove((char *)obuf,(char *)ovflwbuf,tail * sizeof(float));
                memset((char *)ovflwbuf,0,(size_t) (dz->iparam[FLT_OVFLWSIZE] * sizeof(float)));
            }
            write_start -= dz->buflen;
            write_end   -= dz->buflen;
            remaining_dur -= dz->buflen;
        }
        if((exit_status= iterfilt(write_start,&write_end,&atten_index,thisbuf,orig_maxsamp,splicelen,dz))<0)
            return(exit_status);
        thisbuf = !thisbuf;   /* swap between buf0 and buf1 */
        write_start = get_next_writestart(write_start,dz);
    }
    if(write_end-1 > 0)
        return write_samps(obuf,write_end-1,dz);
    return FINISHED;
}

/*************************** READSAMPS_WITH_WRAP **************************/

int readsamps_with_wrap(dataptr dz)
{
    int exit_status;
    int j;
    int n;
    float *buf = dz->sampbuf[0];
    if((exit_status = read_samps(dz->sampbuf[0],dz)) < 0) {
//TW MESSAGE TEXT CORRECTED
        sprintf(errstr, "Can't read samples from input soundfile: readsamps_with_wrap()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->ssampsread!=dz->insams[0]) {
        sprintf(errstr, "Failed to read all of source file: readsamps_with_wrap()\n");
        return(PROGRAM_ERROR);
    }
    j = dz->ssampsread;
    for(n=0;n<dz->infile->channels;n++) {
        buf[j] = 0.0f;
        j++;            /* INSERT GUARD POINTS FOR INTERPOLATION */
    }
    return(FINISHED);
}

/******************************** GET_GAIN *****************************/

double get_gain(dataptr dz)
{
    double scatter;
    scatter  = drand48() * dz->param[FLT_AMPSHIFT];
    scatter  = 1.0 - scatter;
    return scatter;
}

/******************************** GET_PSHIFT *****************************/

double get_pshift(dataptr dz)
{
    double scatter;
    scatter = (drand48() * 2.0) - 1.0;
    if(scatter >= 0.0) {
        scatter *= dz->param[FLT_PSHIFT];
        return(pow(2.0,scatter));
    }
    scatter = -scatter;
    scatter *= dz->param[FLT_PSHIFT];
    scatter = pow(2.0,scatter);
    scatter = 1.0/scatter;
    return(scatter);
}

/*************************** GET_NEXT_WRITESTART ****************************/

int get_next_writestart(int write_start,dataptr dz)
{
    int this_step;
    double scatter;  
    scatter = ((drand48() * 2.0) - 1.0) * dz->param[FLT_RANDDEL];
    scatter += 1.0;
    this_step = (int)round((double)dz->iparam[FLT_MSAMPDEL] * scatter);
    this_step *= dz->infile->channels;
    write_start += this_step;
    return(write_start);
}    

/******************************* ITERFILT *****************************/

int iterfilt(int write_start,int *write_end,int *atten_index,int thisbuf,double orig_maxsamp,int splicelen,dataptr dz)
{
    int exit_status;
    int wr_end;
    if((exit_status = do_iterated_filtering(*atten_index,write_start,&wr_end,thisbuf,orig_maxsamp,splicelen,dz))<0)
        return(exit_status);
    (*atten_index)++;
    *write_end =  max(wr_end,*write_end);
    return(FINISHED);
}

/***************************** RESET_FILTERS *************************/

void reset_filters(dataptr dz)
{
    int n, chno, k;
    int chans = dz->infile->channels;
    for(n=0;n<dz->iparam[FLT_CNT];n++) {
        for(chno=0;chno<chans;chno++) {
            k = (n*chans)+chno;
            if(dz->vflag[FLT_DBLFILT]) {
                dz->parray[FLT_D][k]  = 0.0;
                dz->parray[FLT_E][k]    = 0.0;
            }
            dz->parray[FLT_Y][k]  = 0.0;
            dz->parray[FLT_Z][k]  = 0.0;
        }
    }
}

/******************************* SCALE_INPUT ****************************/

void scale_input(dataptr dz)
{
    int i;
    float *b  = dz->sampbuf[0];
    int  end = dz->insams[0];
    if(dz->iparam[FLT_DOFLAG]!=U_MONO && dz->iparam[FLT_DOFLAG]!=U_STEREO)
        end = dz->insams[0] + dz->infile->channels;
    for(i=0; i < end; i++)
        b[i] = (float)(b[i] * dz->param[FLT_PRESCALE]);
}

/*************************** AMP_SMOOTH ************************
 *
 * expomential attenuation to input ensures everything remains equally loud as it decays!!
 * attenuation when max no of signals overlayed (endatten)  is 1.0/dz->iparam[FLT_MAXOVERLAY].
 * No of samples before this max overlay (y=attencnt)  
 * .....is dz->iparam[FLT_MSAMPDEL] * (dz->iparam[FLT_MAXOVERLAY]-1).
 * If sample by sample atten is x then
 * pow(x,y) = endatten.
 * Hence x  = pow(endatten,1/y);
 */

void amp_smooth(dataptr dz)
{
//  int k = 1;
    register int i = 0, n;
    register int m;
    double atten, endatten;
    int attencnt;
    int chans = dz->infile->channels;
    attencnt = dz->iparam[FLT_MSAMPDEL] * (dz->iparam[FLT_MAXOVERLAY]-1);
    endatten = 1.0/(double)dz->iparam[FLT_MAXOVERLAY];
    dz->param[FLT_SAMPATTEN] = pow(endatten,(1.0/(double)attencnt));
    atten = 1.0;
    for(n = 0;n < dz->insams[0]; n++) {
        for(m = 0; m < chans; m++) {
            dz->sampbuf[0][i] = (float)((double)dz->sampbuf[0][i] * atten);
            i++;
        }
        atten *= dz->param[FLT_SAMPATTEN];
    }
}

/*************************** DO_ATTENUATION ************************/

void do_attenuation(int atten_index, float *ib2,dataptr dz)
{
    double thisgain;
//  register int n, j = 0;
    register int n;
//  int chans = dz->infile->channels;
    int atten_factor = min(atten_index,dz->iparam[FLT_MAXOVERLAY]);
    thisgain = pow(dz->param[FLT_SAMPATTEN],(double)(atten_factor * dz->iparam[FLT_MSAMPDEL]));
    for (n = 0; n < dz->insams[0]; n++) {
        ib2[n] = (float)((double)ib2[n] * thisgain);
        //      n++;      /*RWD 12/20: can't be right to increment this twice */
    }
}

/************************** FILTER_NORMALISE ************************/

void filter_normalise(float *thisbuf,double orig_maxsamp,dataptr dz)
{
    int n;
    double maxsamp = get_maxsamp(thisbuf,dz);
    double thisnorm = orig_maxsamp/maxsamp;
    for (n = 0; n < dz->insams[0]; n++)
        thisbuf[n] = (float) ((double)thisbuf[n] * thisnorm);
}

/************************** GET_MAXSAMP ************************/

double get_maxsamp(float *thisbuf,dataptr dz)
{
    int n;
    double  maxsamp = F_MINSAMP, minsamp = F_MAXSAMP;
    for (n = 0; n < dz->insams[0]; n++) {
        maxsamp = max(thisbuf[n],maxsamp);
        minsamp = min(thisbuf[n],minsamp);
    }
    maxsamp = max(fabs(maxsamp),fabs(minsamp));
    return(maxsamp);
}   

/**************************** DO_ITERATED_FILTERING ***************************/

int do_iterated_filtering(int atten_index,int write_start,int *write_end,
        int thisbuf,double orig_maxsamp,int splicelen,dataptr dz)
{
    int exit_status;
    register int  n;
    int   write_pos = write_start;
    float  *ibuf = dz->sampbuf[thisbuf];
    float  *ibuf2= dz->sampbuf[!thisbuf];
    float  *obuf =  dz->sampbuf[FLT_OUTBUF];
    int    chans = dz->infile->channels;
    double *ampl = dz->parray[FLT_AMPL];
    double *a    = dz->parray[FLT_A];
    double *b    = dz->parray[FLT_B];
    double *y    = dz->parray[FLT_Y];
    double *z    = dz->parray[FLT_Z];
    double *d    = dz->parray[FLT_D];
    double *z1   = dz->parray[FLT_E];
    double step     = 1.0;
    double thisgain = 1.0;
    int    total_samps = dz->iparam[FLT_INMSAMPSIZE]*chans;
    if(dz->param[FLT_PSHIFT] > 0.0)     
        step = get_pshift(dz);
    if(dz->param[FLT_AMPSHIFT] > 0.0)       
        thisgain = get_gain(dz);

    reset_filters(dz);
    for (n = 0; n < total_samps; n+=chans)
        io_filtering(ibuf,ibuf2,chans,n,a,b,y,z,d,z1,ampl,dz);
    if(!dz->vflag[FLT_NONORM])
        filter_normalise(ibuf2,orig_maxsamp,dz);
    if(dz->vflag[FLT_EXPDEC])
        do_attenuation(atten_index,ibuf2,dz);
    do_end_splice(ibuf2,dz->iparam[FLT_INMSAMPSIZE],splicelen,dz);
    if((exit_status = do_shift_gain_and_copy_to_outbuf(ibuf2,obuf,chans,step,thisgain,&write_pos,dz))<0)
        return(exit_status);

    memset((char *)ibuf,0,(size_t)(dz->iparam[FLT_INFILESPACE] * sizeof(float)));
    *write_end = write_pos;
    return(FINISHED);
}

/*************************** DO_SHIFT_GAIN_AND_COPY_TO_OUTBUF *******************************/

int do_shift_gain_and_copy_to_outbuf(float *ibuf,float *obuf,int chans,double step,
                                    double thisgain,int *write_pos,dataptr dz)
{
    float  val, nextval, diff;
    double d = 0.0, part = 0.0, input;
    int n = 0, z;
    register int j = *write_pos;
    register int  chno;
    int total_samps = dz->iparam[FLT_INMSAMPSIZE] * chans;

    switch(dz->iparam[FLT_DOFLAG]) {
    case(ITER_MONO): 
    case(ITER_STEREO):
        while(n < total_samps) { 
            for(chno=0;chno<chans;chno++)
                obuf[j++] += (float)(ibuf[n+chno] * thisgain);
            n+=chans;
        }
        break;
    case(MN_SHIFT):
        while(n < dz->iparam[FLT_INMSAMPSIZE]) { 
            obuf[j++] += (float) (ibuf[n] * thisgain);
            d       += step;
            n        = round(d);        /* ROUND */
        }
        break;
    case(ST_SHIFT):
        while (n < dz->iparam[FLT_INMSAMPSIZE]) { 
            for(chno = 0; chno < chans; chno++) {
                z = (n * chans) + chno;
                obuf[j++] += (float)(ibuf[z] * thisgain);
            }
            d   += step;
            n   = round(d);
        }
        break;
    case(MN_FLT_INTP_SHIFT):
        while(n < dz->iparam[FLT_INMSAMPSIZE]) {
            val      = ibuf[n++];
            nextval  = ibuf[n];
            diff     = nextval - val;
            input    = (double)val + ((double)diff * part);
            obuf[j++] += (float) (input * thisgain);
            d       += step;
            n        = (int)d;          /* TRUNCATE */
            part     = d - (double)n; 
        }
        break;
    case(ST_FLT_INTP_SHIFT):
        while(n < dz->iparam[FLT_INMSAMPSIZE]) {
            for(chno = 0; chno < chans; chno++) {
                z = (n * chans) + chno;
                val       = ibuf[z];
                nextval   = ibuf[z+chans];
                diff      = nextval - val;
                input     = (double)val + ((double)diff * part);
                obuf[j++] += (float) (input * thisgain);
            }
            d   += step;
            n    = (int)d;                      /* TRUNCATE */
            part = d - (double)n;
        }
        break;
    case(U_MONO): 
    case(U_STEREO):
        while(n < total_samps) {
            for(chno=0;chno<chans;chno++) 
                obuf[j++] += ibuf[n+chno];
            n+= chans;
        }
        break;
    case(U_MN_SHIFT):
        while(n < dz->iparam[FLT_INMSAMPSIZE]) { 
            obuf[j++] = ibuf[n];
            d      += step;
            n       = round(d);     /* ROUND */
        }
        break;
    case(U_ST_SHIFT):
        while(n < dz->iparam[FLT_INMSAMPSIZE]) { 
            for(chno = 0; chno < chans; chno++) {
                z = (n * chans) + chno;
                obuf[j++] += ibuf[z];
            }
            d += step;
            n  = round(d);      /* ROUND */
        }
        break;
    case(U_MN_INTP_SHIFT):
        while(n < dz->iparam[FLT_INMSAMPSIZE]) {
            val      = ibuf[n++];
            nextval  = ibuf[n];
            diff     = nextval - val;
            input    = (double)val + ((double)diff * part);
            obuf[j]  = (float) (input + (double)obuf[j]);
            j++;
            d        += step;
            n         = (int)d;             /* TRUNCATE */
            part      = d - (double)n; 
        }
        break;
    case(U_ST_INTP_SHIFT):
        while(n < dz->iparam[FLT_INMSAMPSIZE]) {
            for(chno = 0; chno < chans; chno++) {
                z = (n * chans) + chno;
                val       = ibuf[z];
                nextval   = ibuf[z+chans];
                diff      = nextval - val;
                input     = (double)val + ((double)diff * part);
                obuf[j]   = (float)(input + (double)obuf[j]);
                j++;
            }
            d   += step;
            n    = (int)d;             /* TRUNCATE */
            part = d - (double)n;
        }
        break;
    default:
        sprintf(errstr,"Unknown case in do_shift_gain_and_copy_to_outbuf()\n");
        return(PROGRAM_ERROR);
    }
    *write_pos = j;
    return(FINISHED);
}

/*************************************** DO_END_SPLICE **************************************/

void do_end_splice(float *ibuf,int seglen_in_mono,int splicelen,dataptr dz)
{
    int n, m, sampno, chnno, chans = dz->infile->channels;
    double splicestep = 1.0/(double)splicelen;
    double splicefact = 0.0;
    for(n=((seglen_in_mono*chans)-chans), m=0; m<splicelen; n-=chans ,m++) {
        for(chnno = 0;chnno<chans;chnno++) {
            sampno = n + chnno;
            ibuf[sampno] = (float) ((double)ibuf[sampno] * splicefact);
        }
        splicefact += splicestep;
    }
}

/*************************************** MAKE_VFILT_DATA **************************************/

int make_vfilt_data(dataptr dz)
{
    int n,k,m,j;
    char *q, outname[200], line[200], temp[200];
    double d;
    FILE *fp;
    int *thislinecnt;
    if((thislinecnt = (int *)malloc(dz->linecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"No memory to distinguish lines in input data.\n");
        return(MEMORY_ERROR);
    }
    if((fp = fopen(dz->wordstor[0],"r"))==NULL) {
        sprintf(errstr,"Failed to reopen file %s for checking.\n",dz->wordstor[0]);
        return(DATA_ERROR);
    }
    n = 0;
    while(fgets(temp,200,fp)!=NULL) {
        q = temp;
        thislinecnt[n] = 0;
        while(get_float_from_within_string(&q,&d)) {
            thislinecnt[n]++;
        }
        n++;
    }
    fclose(fp);
    j = 0;
    for(n = 0; n < dz->linecnt; n++) {
        strcpy(outname,dz->wordstor[1]);
        if(sloom) {
            replace_filename_extension(outname,"");
            insert_new_number_at_filename_end(outname,n,1);
        } else {
            replace_filename_extension(outname,".txt");
            insert_new_number_at_filename_end(outname,n,0);
        }
        if((fp = fopen(outname,"w"))==NULL) {
            sprintf(errstr,"Cannot open file %s to store filter data\n",outname);
            return(SYSTEM_ERROR);
        }
        strcpy(line,"0");
        k = j;
        for(m=0;m<thislinecnt[n];m++) {
            strcat(line,"\t");
            sprintf(temp,"%lf",dz->parray[0][j++]);
            strcat(line,temp);
            strcat(line,"\t1");
        }
        fprintf(fp,"%s\n",line);
        strcpy(line,"10000");
        for(m=0;m<thislinecnt[n];m++) {
            strcat(line,"\t");
            sprintf(temp,"%lf",dz->parray[0][k++]);
            strcat(line,temp);
            strcat(line,"\t1");
        }
        fprintf(fp,"%s\n",line);
        fclose(fp);
    }
    dz->process_type = OTHER_PROCESS;
    return FINISHED;
}
