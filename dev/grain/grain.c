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
/*floatsm version */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <arrays.h>
#include <grain.h>
#include <cdpmain.h>

#include <sfsys.h>

static int  grains(int *ibufposition,int bufno,int *crosbuf,int *inithole,int *grainstart,int * is_first_grain,
            int *gapcnt,int *holecnt,int *obufpos,int *graincnt,int chans,int *grainadjusted,
            double samptotime,double **env,double **envstep,double *envel_val,int winsize,dataptr dz);
static int  read_gate(int abs_ipos,int chans,dataptr dz);
static double get_grainstart_time
            (int ibufpos,int grainstart,int abs_ipos,int crosbuf,double samptotime,dataptr dz);
static int  adjust_gate(double *gate,double *ngate,int abs_ipos,int winsize,
            double **env,double **envstep,double *envel_val,dataptr dz);
static void   swap_infile_and_otherfile(dataptr dz); 
static int count_grains_for_assess(int *graincnt,dataptr dz);

/**************************** PROCESS_GRAINS *****************************/

int process_grains(dataptr dz)
{
    int exit_status;
    int  ibufpos = 0, obufpos = 0, graincnt = 0, gapcnt = 0, holecnt = 0, grainadjusted = 0;
    int   bufno = 0, crosbuf = 0, inithole = TRUE;
    int   is_first_grain = TRUE;
    int   chans = dz->infile->channels;
    double samptotime = 1.0/(double)(dz->infile->srate * chans);
    int  grainstart = 0, winsize = dz->iparam[GR_WSIZE_SAMPS];
    double *env     = dz->parray[GR_ENVEL];
    double *envstep = dz->parray[GR_ENVSTEP];
    double envel_val;
    int grain_get_flag = 1;
    int real_process = 0;
    int firstpass = 1;

    display_virtual_time(0,dz);
    while(grain_get_flag == 1) {
        if(firstpass) {
            if((dz->process == GRAIN_GET) || (dz->process == GRAIN_ALIGN)) {
                real_process = dz->process;
                dz->process = GRAIN_COUNT;
                fprintf(stdout,"INFO: Counting the grains.\n");
                fflush(stdout);
            } else {
                grain_get_flag = 0;
            }
        } else {
            if(graincnt == 0) {
                sprintf(errstr,"ERROR: No grains were found.\n");
                return(GOAL_FAILED);
            }
            fprintf(stdout,"INFO: Getting the grains.\n");
            fflush(stdout);
            if(sndseekEx(dz->ifd[0],0,0)<0) {
                sprintf(errstr,"Failed to seek to start of file.");
                return(SYSTEM_ERROR);
            }
            dz->total_samps_read = 0;
            dz->samps_left = dz->insams[0];
            display_virtual_time(0,dz);
            dz->process = real_process;
            env     = dz->parray[GR_ENVEL];
            envstep = dz->parray[GR_ENVSTEP];
            ibufpos = 0;
            obufpos = 0;
            gapcnt  = 0;
            holecnt = 0;
            bufno   = 0;
            crosbuf = 0;
            grainadjusted = 0;
            inithole       = TRUE;
            is_first_grain = TRUE;
            grain_get_flag = 0;
            if(sndseekEx(dz->ifd[0],0,0)<0) {
                sprintf(errstr,"ERROR: seek failed.\n");
                return(SYSTEM_ERROR);
            }
        }
        while(dz->samps_left > 0) {
            if((exit_status = read_samps(dz->sampbuf[bufno],dz))<0)
                return(exit_status);
            if((exit_status = grains(&ibufpos,bufno,&crosbuf,&inithole,&grainstart,&is_first_grain,
                &gapcnt,&holecnt,&obufpos,&graincnt,chans,&grainadjusted,samptotime,&env,&envstep,&envel_val,winsize,dz))<0)
                return(exit_status);
            if(exit_status!=CONTINUE)
                break;
            bufno = !bufno;
        }
        //TW REVISED
        if((exit_status = deal_with_last_grains(ibufpos,bufno,&graincnt,grainstart,
            &grainadjusted, &obufpos,crosbuf,chans,samptotime,&is_first_grain,dz))<0)
            return(exit_status);
        if(dz->outfiletype==SNDFILE_OUT && dz->process != GRAIN_ALIGN && obufpos!=0
            && (exit_status = write_samps(dz->sampbuf[2],obufpos ,dz))<0)
            return(exit_status);
        if(grainadjusted>0) {
            fprintf(stdout,"WARNING: %d output grains too small: size adjusted\n",grainadjusted);
            fflush(stdout);
        }
        firstpass = 0;
    }
    
    return(FINISHED);
}

/**************************** GRAINS *****************************/

int grains(int *ibufposition,int bufno,int *crosbuf,int *inithole,int *grainstart,int *is_first_grain,
int *gapcnt,int *holecnt,int *obufpos,int *graincnt,int chans,int *grainadjusted,
double samptotime,double **env,double **envstep,double *envel_val,int winsize,dataptr dz)
{
    int exit_status;
    register int ibufpos, abs_ipos;
    int in_grain, m;
    double gate, ngate;
    double thistime;
    float *obuf = dz->sampbuf[2];
    float *b    = dz->sampbuf[bufno];
    int has_snd_output = FALSE;
    int samps_read_before_thisbuf = dz->total_samps_read - dz->ssampsread;
    if(dz->outfiletype==SNDFILE_OUT && dz->process != GRAIN_ALIGN)
        has_snd_output = TRUE;
    for(ibufpos = 0,abs_ipos = samps_read_before_thisbuf; ibufpos < dz->ssampsread; ibufpos+=chans,abs_ipos+=chans)  {
        if(dz->ptr[GR_GATEVALS]!=NULL && (exit_status = read_gate(abs_ipos,chans,dz))<0)
            return(exit_status);
        gate  = dz->param[GR_GATE]  * F_MAXSAMP;
        ngate = dz->param[GR_NGATE] * F_MAXSAMP;
        if(dz->iparam[GR_WSIZE_SAMPS] > 0) {   /* envelope tracking */
            if((exit_status = adjust_gate(&gate,&ngate,abs_ipos,winsize,env,envstep,envel_val,dz))<0)
                return(exit_status);
        }
        in_grain = FALSE;
        for(m=0;m<chans;m++) {
            if(b[ibufpos+m]>gate || b[ibufpos+m]<ngate)
                in_grain = TRUE;
        }
        if(in_grain) {
            if(*inithole) {
                *grainstart = ibufpos; 
                *inithole   = 0;
            }
            if(*holecnt) {
                if(*holecnt >= dz->iparam[GR_MINHOLE]) {
                    thistime = get_grainstart_time(ibufpos,*grainstart,abs_ipos,*crosbuf,samptotime,dz);
                    if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
                        return(exit_status);
                    if((exit_status = do_the_grain(ibufpos,graincnt,bufno,*gapcnt,obufpos,*grainstart,
                    *crosbuf,chans,grainadjusted,samptotime,is_first_grain,dz))<0)
                        return(exit_status);
                    if(exit_status!=CONTINUE) {
                        *ibufposition = ibufpos;
                        return(FINISHED);   /* grsync ran out of sync-times */
                    }
                    *grainstart = ibufpos;
                    *gapcnt  = 0;
                    *crosbuf = 0;
                }
            }
            *holecnt = 0;
        } else {
            if(*inithole) {
                if(has_snd_output) {
                    for(m=0;m<chans;m++)
                        obuf[(*obufpos)++] = b[ibufpos+m];
                    if(*obufpos >= dz->buflen) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        *obufpos = 0;       
                    }
                }
                continue;
            }
            *holecnt += chans;
        }
        *gapcnt += chans;
    }
    *ibufposition = ibufpos;
    if(*crosbuf==TRUE) {      /* If crosbuf STILL set */
        if(dz->ssampsread == dz->buflen) {  /* Reached end of buffer while doing crosbuf grain */
            if(dz->process == GRAIN_COUNT && dz->itemcnt == GRAIN_ASSESS) {
                if(dz->vflag[0] == 0) {
                    fprintf(stdout,"WARNING: Encountered grain too large for buffer: counting grains so far\n");
                    dz->vflag[0] = 1;
                }
            } else
                fprintf(stdout,"WARNING: Encountered grain too large for buffer: writing file so far\n");
            fflush(stdout);
        }
        return(FINISHED);                   /* Else reached end of file while doing crosbuf grain */
    }
    if(dz->samps_left)      /* UNLESS we've reached end of src */
        *crosbuf = TRUE;    /* mark crossing into next buffer */
    return(CONTINUE);
}

/************************** COPYGRAIN ******************************/

int copygrain(int start,int end,int bufno,int *obufposition,dataptr dz)
{
    int exit_status;
    float *b = dz->sampbuf[bufno];
    float *obuf = dz->sampbuf[2];
    register int n, obufpos = *obufposition;
    for(n=start;n<end;n++) {
        obuf[obufpos++] = b[n];
        if(obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;        
        }
    }
    *obufposition = obufpos;
    return(FINISHED);
}

/************************ CROSBUF_GRAIN_TYPE3 **********************/

int crosbuf_grain_type3(int grainstart,int grainend,int bufno,int *obufpos,dataptr dz)
{
    int exit_status;
    if((exit_status = copygrain(grainstart,dz->buflen,!bufno,obufpos,dz))<0)
        return(exit_status);
    return copygrain(0L,grainend,bufno,obufpos,dz);
}

/***************************** CHECK_GRAIN_CONSISTENCY ******************************/

int check_grain_consistency(dataptr dz)
{
    int exit_status;
    int maxval;
    double dmaxval;
    switch(dz->process) {
    case(GRAIN_OMIT):       /* Check grains-to-KEEP vals against OUT-OF */
        if(dz->brksize[GR_KEEP]) {
            if((exit_status = get_maxvalue_in_brktable(&dmaxval,GR_KEEP,dz))<0)
                return(exit_status);
            maxval = round(dmaxval);
        } else
            maxval = dz->iparam[GR_KEEP];
        if(maxval > dz->iparam[GR_OUT_OF]) {
            sprintf(errstr,"A value of %d grains-to-keep out of each %d is impossible.\n",maxval,(int)dz->iparam[GR_OUT_OF]);
            return(DATA_ERROR);
        }
        break;
    default:
        sprintf(errstr,"Unknown case in check_grain_consistency()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/***************************** READ_GATE ******************************/

int read_gate(int abs_ipos,int chans,dataptr dz)
{
    double sampstep;
    if(abs_ipos >= dz->iparam[GR_NEXTBRKSAMP]) {
        dz->iparam[GR_THISBRKSAMP] = dz->iparam[GR_NEXTBRKSAMP];
        dz->param[GR_GATE]         = dz->param[GR_NEXTGATE];
        dz->param[GR_NGATE]        = -(dz->param[GR_GATE]);

        dz->iparam[GR_NEXTBRKSAMP] = round(*(dz->ptr[GR_GATEVALS]++));
        dz->param[GR_NEXTGATE]     = *(dz->ptr[GR_GATEVALS]++);

        if((dz->iparam[GR_TESTLIM] = dz->iparam[GR_NEXTBRKSAMP] - (GR_INTERPLIMIT * chans)) < dz->iparam[GR_THISBRKSAMP])
            dz->iparam[GR_TESTLIM] = dz->iparam[GR_NEXTBRKSAMP];

        sampstep = (double)((dz->iparam[GR_NEXTBRKSAMP] - dz->iparam[GR_THISBRKSAMP])/chans);

        dz->param[GR_GATESTEP] = (double)(dz->param[GR_NEXTGATE] - dz->param[GR_GATE])/sampstep;
        if(dz->param[GR_GATESTEP]>0.0)
            dz->iparam[GR_UP] = TRUE;
        else
            dz->iparam[GR_UP] = FALSE;

    } else {
        dz->param[GR_GATE] += dz->param[GR_GATESTEP];
        if(abs_ipos > dz->iparam[GR_TESTLIM]) {           /* allowing for arithmetic rounding */
            if(dz->iparam[GR_UP]==TRUE) {
                if(dz->param[GR_GATE] > dz->param[GR_NEXTGATE])
                    dz->param[GR_GATE] = dz->param[GR_NEXTGATE];
            } else {
                if(dz->param[GR_GATE] < dz->param[GR_NEXTGATE])
                    dz->param[GR_GATE] = dz->param[GR_NEXTGATE];
            }       
        }
        dz->param[GR_NGATE] = -(dz->param[GR_GATE]);
    }
    return(FINISHED);
}

/**************************** GET_GRAINSTART_TIME ***************************/

double get_grainstart_time(int ibufpos,int grainstart,int abs_ipos,int crosbuf,double samptotime,dataptr dz)
{
    double thistime;
    int abs_grainstart;
    int grainlen = ibufpos - grainstart;
    if(crosbuf) 
        grainlen += dz->buflen;
    abs_grainstart = abs_ipos - grainlen; 
    thistime = (double)abs_grainstart * samptotime;
    return thistime;
}

/******************* SWAP_TO_OTHERFILE_AND_READJUST_COUNTERS *******************/

int swap_to_otherfile_and_readjust_counters(dataptr dz)
{
    swap_infile_and_otherfile(dz);                 /* Now process 2nd file AS IF it were infile. */
    dz->infile->channels = dz->otherfile->channels;
    dz->infile->srate    = dz->otherfile->srate;
    dz->infile->stype    = dz->otherfile->stype;
    reset_filedata_counters(dz);        /* initalise all counters before starting 2nd processing */
    if(dz->iparam[GR_WSIZE_SAMPS] > 0) {                         /* Free envelope storage arrays */
        free(dz->parray[GR_ENVEL]);
        free(dz->parray[GR_ENVSTEP]);
    }
    return(FINISHED);
}

/***************************** SWAP_INFILE_AND_OTHERFILE **************************
 *
 * SWAPPING (rather than simply replacing ifd0 by ifd1) ensures that both files
 * are closed properly in finish().
 */

void swap_infile_and_otherfile(dataptr dz)
{
    int fd;
    int filesize;
    int insamples;
    fd                = dz->ifd[0];
    dz->ifd[0]        = dz->ifd[1];
    dz->ifd[1]        = fd;

    filesize          = dz->insams[0];
    dz->insams[0] = dz->insams[1];
    dz->insams[1] = filesize;

    insamples         = dz->insams[0];
    dz->insams[0]     = dz->insams[1];
    dz->insams[1]     = insamples;
}

/**************************** ADJUST_GATE ***************************/

int adjust_gate(double *gate,double *ngate,int abs_ipos,int winsize,
double **env,double **envstep,double *envel_val,dataptr dz)
{
    int cnt;
    if(abs_ipos==0)
        *envel_val = **env;
    else if((cnt = abs_ipos%winsize)==0) {
        (*env)++;
        (*envstep)++;
        if(*env >= dz->ptr[GR_ENVEND] || *envstep >= dz->ptr[GR_ESTEPEND]) {
            sprintf(errstr,"Envelope array overflow: adjust_gate()\n");
            return(PROGRAM_ERROR);
        }
        *envel_val = **env;
    } else
        *envel_val += **envstep;
    *gate  *= *envel_val;
    *ngate = -(*gate);
    return(FINISHED);
}

/*************************** ASSESS_GRAINS **************************/

int assess_grains(dataptr dz) 
{
    int exit_status;
    int lastgraincnt = 0, graincnt = 0;
    int firsttime = 1;
    double step = 0.1;
    double bestgate;
    double hi_error = 1.0 -FLTERR;

    dz->process = GRAIN_COUNT;
    dz->itemcnt = GRAIN_ASSESS; /*n temporary storage */
    dz->param[GR_GATE]  = GR_GATE_DEFAULT;
    dz->param[GR_NGATE] = -(dz->param[GR_GATE]);
    bestgate = dz->param[GR_GATE];
    fprintf(stdout,"INFO: Scanning file\n");
    fflush(stdout);
    step = 0.1;
    firsttime = 1;
    if((exit_status = count_grains_for_assess(&graincnt,dz)) < 0) {
        dz->process = GRAIN_ASSESS;
        return(exit_status);
    }
    dz->param[GR_GATE]  += step;
    dz->param[GR_NGATE] = -(dz->param[GR_GATE]);
    while(fabs(step) > .00002) {
        lastgraincnt = graincnt;
        graincnt = 0;
        if((exit_status = count_grains_for_assess(&graincnt,dz)) < 0) {
            dz->process = GRAIN_ASSESS;
            return(exit_status);
        }
        if(graincnt >= lastgraincnt) {
            firsttime = 0;
            bestgate = dz->param[GR_GATE];
            dz->param[GR_GATE]  += step;
            if(dz->param[GR_GATE] >= hi_error || dz->param[GR_GATE] < 0.0) {
                dz->param[GR_GATE]  -= step;
                step *= 0.1;
                dz->param[GR_GATE]  += step;
                firsttime = 1;
            }
        } else if(firsttime) {
            graincnt = lastgraincnt;
            step = -step;
            if((dz->param[GR_GATE] += (step * 2.0)) < 0.0) {
                break;
            }
            firsttime = 0;
        } else {
            dz->param[GR_GATE]  -= step;
            bestgate = dz->param[GR_GATE];
            graincnt = lastgraincnt;
            step *= 0.1;
            dz->param[GR_GATE]  += step;
            firsttime = 1;
        }
        dz->param[GR_NGATE] = -dz->param[GR_GATE];
    }
    if(lastgraincnt == 0)
        sprintf(errstr,"Unable to find any grains in this sound.\n");
    else
        sprintf(errstr,"Maximum grains found = %d at gate value %lf and windowlen 50ms\n",lastgraincnt,bestgate);
    fflush(stdout);
    dz->process = GRAIN_ASSESS;
    return(FINISHED);
}

/*************************** COUNT_GRAINS_FOR_ASSESS **************************/

int count_grains_for_assess(int *graincnt,dataptr dz)
{
    int exit_status;
    int  ibufpos = 0, obufpos = 0, gapcnt = 0, holecnt = 0, grainadjusted = 0;
    int   bufno = 0, crosbuf = 0, inithole = TRUE;
    int   is_first_grain = TRUE;
    int   chans = dz->infile->channels;
    double samptotime = 1.0/(double)(dz->infile->srate * chans);
    int  grainstart=0, winsize = dz->iparam[GR_WSIZE_SAMPS];
    double *env     = dz->parray[GR_ENVEL];
    double *envstep = dz->parray[GR_ENVSTEP];
    double envel_val;
    sndseekEx(dz->ifd[0],0,0);
    reset_filedata_counters(dz);
    display_virtual_time(0,dz);
    while(dz->samps_left > 0) {
        if((exit_status = read_samps(dz->sampbuf[bufno],dz)<0))
            return(exit_status);
        if((exit_status = grains(&ibufpos,bufno,&crosbuf,&inithole,&grainstart,&is_first_grain,
        &gapcnt,&holecnt,&obufpos,graincnt,chans,&grainadjusted,samptotime,&env,&envstep,&envel_val,winsize,dz))<0)
            return(exit_status);
        if(exit_status!=CONTINUE)
            break;
        bufno = !bufno;
    }
    if((exit_status = deal_with_last_grains(ibufpos,bufno,graincnt,grainstart,
    &grainadjusted, &obufpos,crosbuf,chans,samptotime,&is_first_grain,dz))<0)
        return(exit_status);
    return(FINISHED);
}
