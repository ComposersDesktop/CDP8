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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <extend.h>
#include <sfsys.h>
#include <osbind.h>

#if defined  unix || defined __GNUC__
#define round(x) lround((x))
#else
#define round(x) cdp_round((x))
#endif

#define RANDSET     (32)  /* reduce no.of segs to small finite no, so start and end seg have chance to be chosen */
#define FORWARDS    (1)
#define BACKWARDS   (-1)

static int  setup_zigzag_splice(int paramno,dataptr dz);
static int  make_zigsplice(int paramno,dataptr dz);
static int  create_zigzag_splicebuffer(dataptr dz);
static int  generate_zigzag_table(dataptr dz);
static int  sort_zigs(dataptr dz);
static int  eliminate_n_steps(int *this_zigtime,int *next_zigtime,int **ziglistend,int *cnt,dataptr dz);
static int  eliminate_step(int *next_zigtime,int **ziglistend,int *cnt,dataptr dz);
static int  insert_extra_zig
                (int direction,int **this_zigtime,int **next_zigtime,int **ziglistend,int len,dataptr dz);
static int  split_up_steps_too_large_for_buffer(dataptr dz);
static int  update_and_check_ziglist_params(int **next_zigtime,int **this_zigtime,int *ziglistend,int direction);
static int  generate_loop_table(dataptr dz);
static int  sort_loops(dataptr dz);
static int  insert_extra_loop_element
                (int direction,int **this_zigtime,int **next_zigtime,int **ziglistend,int len,dataptr dz);
static int  generate_scramble_table(dataptr dz);
static int  scramble_rand(int arraysize,dataptr dz);
static int  preprocess_scramble_shred(dataptr dz);
static int  scramble_shred(int arraysize,dataptr dz);
static int  get_basis_lengths(int chunkcnt,int worklen,int *unit_len, int *endlen,
                      int *scatgrpcnt, int *endscat, int *range, int *endrange, dataptr dz);
static int  normal_scat(int unit_len,int endlen,int chunkcnt_less_one,dataptr dz);
static int  heavy_scat(int scatgrpcnt,int range,int endscat,int endrange,dataptr dz);
static void permute_chunks(int chunkcnt,dataptr dz);
static void insert(int n,int t,int chunkcnt,dataptr dz);
static void prefix(int n,int chunkcnt,dataptr dz);
static void shuflup(int k,int chunkcnt,dataptr dz);
static int  ptr_sort(int end,dataptr dz);
static int  get_maxvalue_of_rand(double *maxrand,dataptr dz);
static int  get_maxvalue_of_pscat(double *maxpscat,dataptr dz);
static int  get_minvalue_of_delay(double *mindelay,dataptr dz);
static void reverse_fadevals(dataptr dz);
static void set_default_gain(int mindelay_samps,dataptr dz);
static void set_default_delays(dataptr dz);
static void setup_iter_process_type(int is_unity_gain,dataptr dz);
static int  make_drnk_splicetab(dataptr dz);
static int  convert_sec_steps_to_grain_steps(dataptr dz);

/***************************** ZIGZAG_PREPROCESS ******************************/

int zigzag_preprocess(dataptr dz)
{
    int exit_status;
    int n = 0;
    if(dz->mode == ZIGZAG_SELF)
        initialise_random_sequence(IS_ZIG_RSEED,ZIGZAG_RSEED,dz);
    if((exit_status = setup_zigzag_splice(ZIGZAG_SPLEN,dz))<0)
        return(exit_status);
    if(dz->insams[0] <= dz->iparam[ZIG_SPLSAMPS] * dz->infile->channels * 2) {
        sprintf(errstr,"Infile too short for splices.\n");
        return(DATA_ERROR);
    }
    if(dz->mode == ZIGZAG_SELF) {
        if((exit_status = generate_zigzag_table(dz))<0)
            return(exit_status);
    }
    if((exit_status = sort_zigs(dz))<0)
        return(exit_status);
    if(sloom) {
        dz->tempsize = 0L;
        for(n=1;n<dz->itemcnt;n++)
            /*RWD treat tempszie as in samps */
            dz->tempsize += abs(dz->lparray[ZIGZAG_TIMES][n] - dz->lparray[ZIGZAG_TIMES][n-1]);
    }
    return(FINISHED);
}

/*********************** SETUP_ZIGZAG_SPLICE ***************************/

int setup_zigzag_splice(int paramno,dataptr dz)
{
    int exit_status;
    if((exit_status = make_zigsplice(paramno,dz))<0)
        return(exit_status);
    return create_zigzag_splicebuffer(dz);
}

/*********************** MAKE_ZIGSPLICE ***************************/

int make_zigsplice(int paramno,dataptr dz)
{
    int n;
    int framesize = dz->infile->channels;
    

    dz->iparam[ZIG_SPLICECNT] = (int)round(dz->param[paramno] * MS_TO_SECS * dz->infile->srate);
    dz->iparam[ZIG_SPLSAMPS]  = dz->iparam[ZIG_SPLICECNT] * framesize;
    if(dz->iparam[ZIG_SPLSAMPS] >= dz->buflen) {
        sprintf(errstr,"Splicelength too long for available memory.\n");
        return(GOAL_FAILED);
    }
    if((dz->parray[ZIGZAG_SPLICE] = (double *)malloc(dz->iparam[ZIG_SPLICECNT] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to make splicer buffer.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->iparam[ZIG_SPLICECNT];n++)
        dz->parray[ZIGZAG_SPLICE][n] = (double)n/(double)dz->iparam[ZIG_SPLICECNT];
    return(FINISHED);
}

/*********************** CREATE_ZIGZAG_SPLICEBUFFER ***************************/

int create_zigzag_splicebuffer(dataptr dz)
{
    if(dz->extrabuf == (float **)0) {
        sprintf(errstr,"extrabuf has not been created: create_zigzag_splicebuffer()\n");
        return(PROGRAM_ERROR);
    }

    if((dz->extrabuf[ZIGZAG_SPLBUF] = (float *)malloc(sizeof(float) * dz->iparam[ZIG_SPLSAMPS]))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to make splicing buffer.\n");
        return(MEMORY_ERROR);
    }
    memset((char *)dz->extrabuf[ZIGZAG_SPLBUF],0,sizeof(float) * dz->iparam[ZIG_SPLSAMPS]);
    return(FINISHED);
}

/***************************** GENERATE_ZIGZAG_TABLE ***************************/

int generate_zigzag_table(dataptr dz)
{
    int    OK;
    int   arraysize = BIGARRAY;
    double infiledur = (double)(dz->insams[0]/dz->infile->channels)/(double)(dz->infile->srate);
    double totaltime = dz->param[ZIGZAG_START];
    double goaltime  = dz->param[ZIGZAG_DUR] - (infiledur - dz->param[ZIGZAG_END]);
    double diff, randlen = 0.0, here  = dz->param[ZIGZAG_START];
    int direction = FORWARDS;
    if((dz->lparray[ZIGZAG_TIMES] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store times.\n");
        return(MEMORY_ERROR);
    }
    dz->lparray[ZIGZAG_TIMES][0] = 0;
    dz->itemcnt = 1;
    do {
        OK = TRUE;
        switch(direction) {
        case(FORWARDS):
            diff = min(dz->param[ZIGZAG_MAX],dz->param[ZIGZAG_END] - here);
            if(diff<=dz->param[ZIGZAG_MIN])
                OK = FALSE;
            else {
                randlen = drand48();                        /* generate segment length at random */
                randlen *= (diff - dz->param[ZIGZAG_MIN]);  /* scale it to range required */
                randlen += dz->param[ZIGZAG_MIN];           /* and add mindur */
                here = (here + randlen);
            }
            break;
        case(BACKWARDS):
            diff = min(dz->param[ZIGZAG_MAX],here - dz->param[ZIGZAG_START]);
            if(diff<=dz->param[ZIGZAG_MIN])
                OK = FALSE;
            else {
                randlen = drand48();                        /* generate segment length at random */
                randlen *= (diff - dz->param[ZIGZAG_MIN]);  /* scale it to range required */
                randlen += dz->param[ZIGZAG_MIN];           /* and add mindur */
                here = (here - randlen);
            }
            break;
        }
        direction = -direction; /* invert time-direction */
        if(!OK)
            continue;
        totaltime += randlen;
        dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = round(here * (double)dz->infile->srate) * dz->infile->channels;
        if(++dz->itemcnt >= arraysize) {
            arraysize += BIGARRAY;
            if((dz->lparray[ZIGZAG_TIMES] = 
            (int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],arraysize*sizeof(int)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
                return(MEMORY_ERROR);
            }
        }
    } while(totaltime<goaltime);
    dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = dz->insams[0];
    dz->itemcnt++;
    if(dz->itemcnt < arraysize) {
        if((dz->lparray[ZIGZAG_TIMES] = 
        (int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],dz->itemcnt*sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
            return(MEMORY_ERROR);
        }
    }
    return(FINISHED);
}

/****************************** SORT_ZIGS ************************************/

int sort_zigs(dataptr dz)
{
    int exit_status;
    int zigsize;
    int safety = round(ZIG_MIN_UNSPLICED * MS_TO_SECS * (double)dz->infile->srate) * dz->infile->channels;
    int cnt = 0, firstime = TRUE, direction, previous_direction = FORWARDS;
    int *this_zigtime = dz->lparray[ZIGZAG_TIMES];
    int *next_zigtime = dz->lparray[ZIGZAG_TIMES] + 1;
    int *ziglistend = dz->lparray[ZIGZAG_TIMES] + dz->itemcnt;
    int minzig = (dz->iparam[ZIG_SPLSAMPS] * 2) + safety;
    int file_samplen = dz->insams[0];
    double convert_to_time = 1.0/(double)dz->infile->channels/(double)dz->infile->srate;
    if(minzig > (dz->buflen/2)-1) {
        sprintf(errstr,"splicelen too long to work with available memory\n"
        "Longest splicelen available %ld msecs.\n",
round((double)(((((dz->buflen/2)-1) - safety)/2)/(double)dz->infile->channels/(double)dz->infile->srate) * SECS_TO_MS)-1);
        return(GOAL_FAILED);
    }
    if(*this_zigtime < 0 || *this_zigtime > file_samplen) {
        sprintf(errstr,"Invalid 1st zigtime %lf\n",(*this_zigtime) * convert_to_time);
        return(DATA_ERROR);
    }
    if(*(ziglistend-1) >= file_samplen) {
        *(ziglistend-1) = file_samplen;
        dz->iparam[ZIG_RUNSTOEND] = 1;
    } else
        dz->iparam[ZIG_RUNSTOEND] = 0;
    while(next_zigtime < ziglistend - dz->iparam[ZIG_RUNSTOEND]) {
        if(*next_zigtime < 0 || *next_zigtime > file_samplen) {
            sprintf(errstr,"Invalid zigtime %lf\n",(*next_zigtime) * convert_to_time);
            return(DATA_ERROR);
        }
        while((zigsize = abs(*next_zigtime - *this_zigtime)) < minzig) {
            if(++next_zigtime == ziglistend - dz->iparam[ZIG_RUNSTOEND])
                break;
        }
        if(next_zigtime - this_zigtime > 1) {
            if(dz->mode == ZIGZAG_USER) {
                sprintf(errstr,"Some zigs too short to use with specified splicelen.\n");
                return(DATA_ERROR);
            }
            eliminate_n_steps(this_zigtime,next_zigtime,&ziglistend,&cnt,dz);
            next_zigtime = this_zigtime + 1;
        }
        if(*next_zigtime > *this_zigtime)
            direction = FORWARDS;
        else
            direction = BACKWARDS;
        if(!firstime && (direction == previous_direction)) {
            if((exit_status = eliminate_step(next_zigtime,&ziglistend,&cnt,dz))<0)
                return(exit_status);
            continue;
        }
        previous_direction = direction;
        firstime = FALSE;
        this_zigtime++;
        next_zigtime++;
    }
    if(cnt>0) {
        fprintf(stdout,"WARNING: %d steps eliminated (too small relative to spliclength\n",cnt);
//TW : CAN'T SPLIT LINES SENT TO SLOOM - 'WARNING' is a flag to SLOOM - possibly my error, since updated
        fprintf(stdout,"WARNING: or moving in same direction as previous step)\n");
        fflush(stdout);
        if((dz->lparray[ZIGZAG_TIMES] =
        (int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],dz->itemcnt * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
            return(MEMORY_ERROR);
        }
    }
    if(dz->iparam[ZIG_RUNSTOEND]) {
        if(*(ziglistend-1) - *(ziglistend-2) < dz->iparam[ZIG_SPLSAMPS] + safety) {
            sprintf(errstr,"Final zig too short for splicelen.\n");
            return(GOAL_FAILED);
        }
    }
    return split_up_steps_too_large_for_buffer(dz);
}

/**************************** ELIMINATE_N_STEPS ***************************/

int eliminate_n_steps(int *this_zigtime,int *next_zigtime,int **ziglistend,int *cnt,dataptr dz)
{
    int *here  = this_zigtime + 1;
    int *there = next_zigtime;
    int elimination_cnt = next_zigtime - this_zigtime - 1;
    while(there < *ziglistend) {
        *here = *there;
        here++;
        there++;
    }
    if((dz->itemcnt -= elimination_cnt) < 2) {
        sprintf(errstr,"All zigsteps either too small for splices: or moving in same direction.\n");
        return(DATA_ERROR);
    }
    *ziglistend -= elimination_cnt;
    (*cnt) += elimination_cnt;
    return(FINISHED);
}

/***************************** ELIMINATE_STEP ***************************/

int eliminate_step(int *next_zigtime,int **ziglistend,int *cnt,dataptr dz)
{
    int *here = next_zigtime;

    while(here < *ziglistend) {
        *(here-1) = *here;
        here++;
    }
    if(--dz->itemcnt < 2) {
        sprintf(errstr,"All zigsteps either too small for splices: or moving in same direction.\n");
        return(DATA_ERROR);
    }
    (*ziglistend)--;
    (*cnt)++;
    return(FINISHED);
}

/***************************** INSERT_EXTRA_ZIG ***************************/

int insert_extra_zig(int direction,int **this_zigtime,int **next_zigtime,int **ziglistend,int len,dataptr dz)
{
    int *here;
    int zthis = *this_zigtime - dz->lparray[ZIGZAG_TIMES];
    int znext = *next_zigtime - dz->lparray[ZIGZAG_TIMES];
    dz->itemcnt++;
    if((dz->lparray[ZIGZAG_TIMES] =
    (int *)realloc((char *)dz->lparray[ZIGZAG_TIMES],dz->itemcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
        return(MEMORY_ERROR);
    }
    *this_zigtime = dz->lparray[ZIGZAG_TIMES] + zthis;
    *next_zigtime = dz->lparray[ZIGZAG_TIMES] + znext;
    *ziglistend  =  dz->lparray[ZIGZAG_TIMES] + dz->itemcnt;
    here = *ziglistend - 1;
    while(here > *next_zigtime) {
        *here = *(here-1);
        here--;
    }
    switch(direction) {
    case(FORWARDS):
        *here = **this_zigtime + len;       
        break;
    case(BACKWARDS):    
        *here = **this_zigtime - len;       
        break;
    }
    if(*here < 0.0 || (int)*here > dz->insams[0]) {
        sprintf(errstr,"Error in logic of sample arithmetic: insert_extra_zig()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/**************************** SPLIT_UP_STEPS_TOO_LARGE_FOR_BUFFER ***************************/

int split_up_steps_too_large_for_buffer(dataptr dz)
{
    int zigsize;
    int direction, n, longbufs, shortbufs;
    int safety = round(ZIG_MIN_UNSPLICED * MS_TO_SECS * (double)dz->infile->srate) * dz->infile->channels;
    int minzig = (dz->iparam[ZIG_SPLSAMPS] * 2) + safety;
    int *this_zigtime = dz->lparray[ZIGZAG_TIMES];
    int *next_zigtime = this_zigtime + 1;
    int *ziglistend = dz->lparray[ZIGZAG_TIMES] + dz->itemcnt;
    int max_effective_length = dz->buflen - F_SECSIZE;
    int pre_endbuflen, endbuflen;
    while(next_zigtime < ziglistend) {
        zigsize = *next_zigtime - *this_zigtime;
        if(zigsize > 0.0)
            direction = FORWARDS;
        else
            direction = BACKWARDS;
        zigsize = abs(zigsize);

        if((longbufs = zigsize/max_effective_length) > 0) { /* TRUNCATE */
            shortbufs    = -1;
            pre_endbuflen =  0;
            while((endbuflen = zigsize%max_effective_length) > 0 && endbuflen < minzig) {
                longbufs--;             
                if(longbufs<0) {
                    sprintf(errstr,"splicelen is too long for available memory.\n");
                    return(GOAL_FAILED);
                }
                shortbufs++;
                pre_endbuflen  = max_effective_length + endbuflen - minzig;
                if(pre_endbuflen > minzig)
                    break;
                zigsize -= minzig;
            }
            for(n = 0; n < longbufs;n++) {
                insert_extra_zig(direction,&this_zigtime,&next_zigtime,&ziglistend,max_effective_length,dz);
                update_and_check_ziglist_params(&next_zigtime,&this_zigtime,ziglistend,direction);
            }
            if(pre_endbuflen > 0) {
                insert_extra_zig(direction,&this_zigtime,&next_zigtime,&ziglistend,pre_endbuflen,dz);
                update_and_check_ziglist_params(&next_zigtime,&this_zigtime,ziglistend,direction);
            }
            if(shortbufs > 0) {
                for(n = 0; n < shortbufs; n++) {
                    insert_extra_zig(direction,&this_zigtime,&next_zigtime,&ziglistend,minzig,dz);
                    update_and_check_ziglist_params(&next_zigtime,&this_zigtime,ziglistend,direction);
                }
            }
        }
        next_zigtime++;
        this_zigtime++;
    }   
    return(FINISHED);
}

/***************************** UPDATE_AND_CHECK_ZIGLIST_PARAMS ***************************/

int update_and_check_ziglist_params(int **next_zigtime,int **this_zigtime,int *ziglistend,int direction)
{       
    int zigsize;
    (*next_zigtime)++;
    (*this_zigtime)++;
    if(*this_zigtime >= ziglistend - 1) {
        sprintf(errstr,"Error in counting logic: update_and_check_ziglist_params()\n");
        return(PROGRAM_ERROR);
    }
    zigsize = **next_zigtime - **this_zigtime;
    if((zigsize > 0.0 && direction != FORWARDS)
    || (zigsize < 0.0 && direction != BACKWARDS)) {
        sprintf(errstr,"Error in counting logic(2): update_and_check_ziglist_params()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/*********************** LOOP_PREPROCESS ***************************/

int loop_preprocess(dataptr dz)
{
    int exit_status;
    initrand48();
    if(dz->param[LOOP_LEN] <= dz->param[LOOP_SPLEN] * 2) {
        sprintf(errstr,"Loop length too short for splices.\n");
        return(DATA_ERROR);
    }
    if((exit_status = setup_zigzag_splice(LOOP_SPLEN,dz))<0)
        return(exit_status);
    if(dz->insams[0] <= dz->iparam[ZIG_SPLSAMPS] * 2) {
        sprintf(errstr,"Infile too short for splices.\n");
        return(DATA_ERROR);
    }
    if((exit_status = generate_loop_table(dz))<0)
        return(exit_status);
    return sort_loops(dz);
}

/***************************** GENERATE_LOOP_TABLE ***************************/

int generate_loop_table(dataptr dz)
{
    int searchsamps, tmpstart;
    unsigned int file_endsamp = dz->insams[0];
    int   i, arraysize = BIGARRAY;
    int   startsamp = dz->infile->channels * (int) (dz->infile->srate * dz->param[LOOP_START]);
    int   looplen   = dz->infile->channels * (int) (dz->infile->srate * dz->param[LOOP_LEN]   * MS_TO_SECS);
    int   stepsamps = dz->infile->channels * (int) (dz->infile->srate * dz->param[LOOP_STEP]  * MS_TO_SECS);
    int   lsfield   = dz->infile->channels * (int) (dz->infile->srate * dz->param[LOOP_SRCHF] * MS_TO_SECS);
    int   samplen, excess, totallen, endsamp, n;

/* NEW APR 16 --> */
    looplen += dz->iparam[ZIG_SPLSAMPS]; /* allow for splice overlaps in calcs */

    dz->itemcnt = 0;
    if((dz->lparray[ZIGZAG_TIMES] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store times.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[ZIGZAG_PLAY]  = (int  *)malloc(arraysize * sizeof(int )))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store section markers.\n");
        return(MEMORY_ERROR);
    }
    searchsamps = 0;
    tmpstart = startsamp;
    for(i = 0; i < dz->iparam[LOOP_REPETS] ;i++) {
        if(dz->itemcnt + 2 > arraysize) {
            arraysize  += BIGARRAY;
            if((dz->lparray[ZIGZAG_TIMES] = (int *)realloc(dz->lparray[ZIGZAG_TIMES],arraysize * sizeof(int)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times.\n");
                return(MEMORY_ERROR);
            }
            if((dz->iparray[ZIGZAG_PLAY]  = (int  *)realloc(dz->iparray[ZIGZAG_PLAY],arraysize * sizeof(int)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate markers.\n");
                return(MEMORY_ERROR);
            }
        }
        dz->iparray[ZIGZAG_PLAY][dz->itemcnt]  = TRUE;
        if(i==0 && dz->vflag[IS_KEEP_START])
            dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = 0;
        else
            dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = tmpstart;
        dz->itemcnt++;
        dz->iparray[ZIGZAG_PLAY][dz->itemcnt]  = FALSE;
        if((unsigned int)(dz->lparray[ZIGZAG_TIMES][dz->itemcnt] = tmpstart + looplen) >= file_endsamp) {
            dz->iparam[ZIG_RUNSTOEND] = TRUE;
            dz->itemcnt++;
            break;
        }           
        dz->itemcnt++;
        startsamp += stepsamps;
        if(lsfield)
            searchsamps = round((double) lsfield * drand48());
        tmpstart = (startsamp + searchsamps);
    }
    if(dz->iparam[ZIG_RUNSTOEND])
        dz->lparray[ZIGZAG_TIMES][dz->itemcnt-1] = file_endsamp;    /* Force read to end */
    else if(dz->mode==LOOP_OUTLEN) {
        samplen = dz->infile->channels * round(dz->infile->srate * dz->param[LOOP_OUTDUR]);
        totallen = 0;                                               /* Force to correct outlen */
        for(n=1;n<dz->itemcnt;n+=2)
            totallen += dz->lparray[ZIGZAG_TIMES][n] - dz->lparray[ZIGZAG_TIMES][n-1] - dz->iparam[ZIG_SPLSAMPS];
        if((excess = samplen - totallen) > 0) {
            endsamp = min(dz->lparray[ZIGZAG_TIMES][dz->itemcnt-1] + excess,(int)file_endsamp);
            dz->lparray[ZIGZAG_TIMES][dz->itemcnt-1] = endsamp;
            if(endsamp == (int)file_endsamp)
                dz->iparam[ZIG_RUNSTOEND] = TRUE;
        }
    }
    if((dz->lparray[ZIGZAG_TIMES] = (int *)realloc(dz->lparray[ZIGZAG_TIMES],dz->itemcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[ZIGZAG_PLAY]  = (int  *)realloc(dz->iparray[ZIGZAG_PLAY], dz->itemcnt * sizeof(int )))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate markers.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/****************************** SORT_LOOPS ************************************/

int sort_loops(dataptr dz)
{
    int zigsize;
    int direction, n, longbufs, shortbufs;
    int safety = round(ZIG_MIN_UNSPLICED * MS_TO_SECS * (double)dz->infile->srate) * dz->infile->channels;
    int minzig = (dz->iparam[ZIG_SPLSAMPS] * 2) + safety;
    int *this_zigtime = dz->lparray[ZIGZAG_TIMES];
    int *next_zigtime = this_zigtime + 1;
    int *ziglistend = dz->lparray[ZIGZAG_TIMES] + dz->itemcnt;
    int max_effective_length = dz->buflen - F_SECSIZE;
    int pre_endbuflen, endbuflen;
    int m = 0;

    while(next_zigtime < ziglistend) {
        zigsize = *next_zigtime - *this_zigtime;
        if(zigsize > 0.0)
            direction = FORWARDS;
        else
            direction = BACKWARDS;
        zigsize = abs(zigsize);

        if((longbufs = zigsize/max_effective_length) > 0) { /* TRUNCATE */
            shortbufs     = -1;
            pre_endbuflen =  0;
            while((endbuflen = zigsize%max_effective_length) > 0 && endbuflen < minzig) {
                longbufs--;             
                if(longbufs<0) {
                    sprintf(errstr,"splicelen is too long for available memory.\n");
                    return(GOAL_FAILED);
                }
                shortbufs++;
                pre_endbuflen  = max_effective_length + endbuflen - minzig;
                if(pre_endbuflen > minzig)
                    break;
                zigsize -= minzig;
            }
            for(n = 0; n < longbufs;n++) {
                insert_extra_loop_element(direction,&this_zigtime,&next_zigtime,&ziglistend,max_effective_length,dz);
                update_and_check_ziglist_params(&next_zigtime,&this_zigtime,ziglistend,direction);
            }
            if(pre_endbuflen > 0) {
                insert_extra_loop_element(direction,&this_zigtime,&next_zigtime,&ziglistend,pre_endbuflen,dz);
                update_and_check_ziglist_params(&next_zigtime,&this_zigtime,ziglistend,direction);
            }
            if(shortbufs > 0) {
                for(n = 0; n < shortbufs; n++) {
                    insert_extra_loop_element(direction,&this_zigtime,&next_zigtime,&ziglistend,minzig,dz);
                    update_and_check_ziglist_params(&next_zigtime,&this_zigtime,ziglistend,direction);
                }
            }
        }
        next_zigtime++;
        this_zigtime++;
    }
    if(sloom) {
        dz->tempsize = 0L;
        for(n=0,m=1;m<dz->itemcnt;n++,m++) {
            if(dz->iparray[ZIGZAG_PLAY][n])
                dz->tempsize += abs(dz->lparray[ZIGZAG_TIMES][m] - dz->lparray[ZIGZAG_TIMES][n]);
        }                    /* abs for SAFETY: should be unnecessary as all plays should be forwards!! */
    }
    return(FINISHED);
}

/***************************** INSERT_EXTRA_LOOP_ELEMENT ***************************/

int insert_extra_loop_element(int direction,int **this_zigtime,int **next_zigtime,int **ziglistend,int len,dataptr dz)
{
    int *here;
    int  *there, *playlistend, *nextplaytime;
    int zthis = *this_zigtime - dz->lparray[ZIGZAG_TIMES];
    int znext = *next_zigtime - dz->lparray[ZIGZAG_TIMES];
    dz->itemcnt++;
    if((dz->lparray[ZIGZAG_TIMES] =
    (int *)realloc(dz->lparray[ZIGZAG_TIMES],dz->itemcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to insert extra loop time.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[ZIGZAG_PLAY] =
    (int  *)realloc(dz->iparray[ZIGZAG_PLAY], dz->itemcnt * sizeof(int )))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to insert extra loopmarker.\n");
        return(MEMORY_ERROR);
    }
    *this_zigtime = dz->lparray[ZIGZAG_TIMES] + zthis;
    *next_zigtime = dz->lparray[ZIGZAG_TIMES] + znext;
    *ziglistend   = dz->lparray[ZIGZAG_TIMES] + dz->itemcnt;
    here = *ziglistend - 1;
    while(here > *next_zigtime) {
        *here = *(here-1);
        here--;
    }
    nextplaytime = dz->iparray[ZIGZAG_PLAY] + (*next_zigtime - dz->lparray[ZIGZAG_TIMES]);
    playlistend =  dz->iparray[ZIGZAG_PLAY] + dz->itemcnt;
    there = playlistend - 1;
    while(there > nextplaytime) {
        *there = *(there-1);
        there--;
    }
    switch(dz->process) {
    case(LOOP): /* COMMENT: this old LOOPcode may be unnecessary, as SCRAMBLE version may be OK for both */
        switch(direction) {
        case(FORWARDS):     *here = **this_zigtime + len;       *there = TRUE;      break;
        case(BACKWARDS):    *here = **this_zigtime - len;       *there = FALSE;     break;
        }
        break;
    case(SCRAMBLE):
        switch(direction) {
        case(FORWARDS):     *here = **this_zigtime + len;       break;
        case(BACKWARDS):    *here = **this_zigtime - len;       break;
        }
        *there = *(there-1);
        break;
    }
//  if(*here < 0.0 || (unsigned int)*here > dz->insams[0]) {
    if(*here < 0.0 || (int)*here > dz->insams[0]) {
        sprintf(errstr,"Error in logic of sample arithmetic: insert_extra_loop_element()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/***************************** SCRAMBLE_PREPROCESS ***************************/

int scramble_preprocess(dataptr dz)
{
    int exit_status;
    initialise_random_sequence(IS_SCRAMBLE_RSEED,SCRAMBLE_SEED,dz);
    if((exit_status = setup_zigzag_splice(SCRAMBLE_SPLEN,dz))<0)
        return(exit_status);
    if(dz->insams[0] <= dz->iparam[ZIG_SPLSAMPS] * dz->infile->channels * 2) {
        sprintf(errstr,"Infile too short for splices.\n");
        return(DATA_ERROR);
    }
    if((exit_status = generate_scramble_table(dz))<0)
        return(exit_status);
    return sort_loops(dz);
}

/***************************** GENERATE_SCRAMBLE_TABLE **************************/

int generate_scramble_table(dataptr dz)
{
    int exit_status;
    int arraysize = BIGARRAY;
    if((dz->lparray[ZIGZAG_TIMES] = (int *)malloc(arraysize*sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for times store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[ZIGZAG_PLAY] = (int  *)malloc(arraysize * sizeof(int )))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for markers store.\n");
        return(MEMORY_ERROR);
    }
    switch(dz->mode) {
    case(SCRAMBLE_RAND):
        exit_status =  scramble_rand(arraysize,dz);
        break;
    case(SCRAMBLE_SHRED):   
        if((exit_status = preprocess_scramble_shred(dz))<0)
            return(exit_status);
        exit_status =  scramble_shred(arraysize,dz);
        break;
    default:
        sprintf(errstr,"Unknown mode in generate_scramble_table()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->vflag[IS_SCR_KEEP_START])
        dz->lparray[ZIGZAG_TIMES][0] = 0;
    if(dz->vflag[IS_SCR_KEEP_END])
        dz->lparray[ZIGZAG_TIMES][dz->itemcnt-1] = dz->insams[0];
    return(exit_status);
}

/***************************** SCRAMBLE_RAND **************************/

int scramble_rand(int arraysize,dataptr dz)
{
    int lenrange = round((dz->param[SCRAMBLE_MAX] - dz->param[SCRAMBLE_MIN]) * dz->infile->srate);
    int minlen   = round(dz->param[SCRAMBLE_MIN] * dz->infile->srate);
    int infilelen = dz->insams[0];
    int thislen, outlen = 0, effective_range, thispos;
    int  finished = 0, zz;
    double thisfrac;
    dz->iparam[SCRAMBLE_OUTLEN] = round(dz->param[SCRAMBLE_DUR] * dz->infile->srate) * dz->infile->channels;
    dz->itemcnt = 0;
    while(!finished) {
        if(dz->itemcnt + 2 >= arraysize) {
            arraysize += BIGARRAY;
            if((dz->lparray[ZIGZAG_TIMES] =(int *)realloc(dz->lparray[ZIGZAG_TIMES],arraysize*sizeof(int)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
                return(MEMORY_ERROR);
            }
            if((dz->iparray[ZIGZAG_PLAY] = (int  *)realloc(dz->iparray[ZIGZAG_PLAY], arraysize*sizeof(int )))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate markers store.\n");
                return(MEMORY_ERROR);
            }
        }
        thislen  = (round(drand48() * lenrange) + minlen) * dz->infile->channels;
        effective_range = infilelen - dz->iparam[ZIG_SPLSAMPS] - thislen;
        zz =  round(drand48() * RANDSET);
        thisfrac = (double)zz/(double)RANDSET;
        thispos =  round(thisfrac * effective_range);
        if(ODD(thispos))
            thispos--;
        dz->lparray[ZIGZAG_TIMES][dz->itemcnt]  = thispos;
        dz->iparray[ZIGZAG_PLAY][dz->itemcnt++] = TRUE;
        dz->lparray[ZIGZAG_TIMES][dz->itemcnt]  = thispos + thislen + dz->iparam[ZIG_SPLSAMPS];
        dz->iparray[ZIGZAG_PLAY][dz->itemcnt++] = FALSE;
        if((outlen += thislen) >= dz->iparam[SCRAMBLE_OUTLEN])
            finished = 1;
    }
    if((dz->lparray[ZIGZAG_TIMES] = (int *)realloc(dz->lparray[ZIGZAG_TIMES],dz->itemcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[ZIGZAG_PLAY] = (int  *)realloc(dz->iparray[ZIGZAG_PLAY], dz->itemcnt * sizeof(int )))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate markers store.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/***************************** PREPROCESS_SCRAMBLE_SHRED **************************/

int preprocess_scramble_shred(dataptr dz)
{
//  double infiledur;
    int chunksamps;

    //infiledur = (double)(dz->insams[0]/dz->infile->channels)/(double)(dz->infile->srate);
    chunksamps = round(dz->param[SCRAMBLE_LEN] * dz->infile->srate) * dz->infile->channels;
    dz->iparam[SCRAMBLE_CHCNT]   = dz->insams[0]/chunksamps;    /* truncate */
    if(dz->iparam[SCRAMBLE_CHCNT]<2) {
        sprintf(errstr,"Too long chunklen not trapped earlier.\n");
        return(PROGRAM_ERROR);
    }
    dz->iparam[SCRAMBLE_OUTLEN] = round(dz->param[SCRAMBLE_DUR] * dz->infile->srate) * dz->infile->channels;

    if((dz->lparray[SCRAMBLE_CHUNKPTR] = (int *)malloc(dz->iparam[SCRAMBLE_CHCNT] * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store scramble chunk pointers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[SCRAMBLE_CHUNKLEN] = (int *)malloc(dz->iparam[SCRAMBLE_CHCNT] * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store scramble chunk lengths.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[SCRAMBLE_PERM]    = (int  *)malloc(dz->iparam[SCRAMBLE_CHCNT] * sizeof(int )))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store scramble permutation.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/******************************* SCRAMBLE_SHRED ***************************/

int scramble_shred(int arraysize,dataptr dz)
{
    int n, exit_status;
    int finished = 0;
    int chunkcnt = dz->iparam[SCRAMBLE_CHCNT];
    int worklen  = dz->insams[0];
    int total_len = 0;
    int chunkcnt_less_one = chunkcnt - 1;
    int unit_len, endlen, scatgrpcnt=0, endscat=0, range=0, endrange=0;
    int *perm = dz->iparray[SCRAMBLE_PERM];
    dz->itemcnt = 0;
    if((exit_status = get_basis_lengths(chunkcnt,worklen,&unit_len,&endlen,&scatgrpcnt,&endscat,&range,&endrange,dz))<0)
        return(exit_status);
    while(!finished)  {
        if(dz->iparam[SCRAMBLE_SCAT]) { 
            if((exit_status = heavy_scat(scatgrpcnt,range,endscat,endrange,dz))<0)
                return(exit_status);
        } else   {
            if((exit_status = normal_scat(unit_len,endlen,chunkcnt_less_one,dz))<0)
                return(exit_status);
        }
        for(n=0;n<chunkcnt_less_one;n++)            /* 2 */
            dz->lparray[SCRAMBLE_CHUNKLEN][n] = dz->lparray[SCRAMBLE_CHUNKPTR][n+1] - dz->lparray[SCRAMBLE_CHUNKPTR][n];
        dz->lparray[SCRAMBLE_CHUNKLEN][n]     = worklen - dz->lparray[SCRAMBLE_CHUNKPTR][n];        /* 2A */
        permute_chunks(chunkcnt,dz);                            /* 3 */
        for(n=0;n < chunkcnt; n++) { 
            if(dz->itemcnt + 2 >= arraysize) {
                arraysize += BIGARRAY;
                if((dz->lparray[ZIGZAG_TIMES] =(int *)realloc(dz->lparray[ZIGZAG_TIMES],arraysize*sizeof(int)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
                    return(MEMORY_ERROR);
                }
                if((dz->iparray[ZIGZAG_PLAY] = (int  *)realloc(dz->iparray[ZIGZAG_PLAY], arraysize*sizeof(int )))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate markers store.\n");
                    return(MEMORY_ERROR);
                }
            }
            dz->lparray[ZIGZAG_TIMES][dz->itemcnt]   = dz->lparray[SCRAMBLE_CHUNKPTR][perm[n]];
            dz->iparray[ZIGZAG_PLAY][dz->itemcnt]    = TRUE;
            dz->lparray[ZIGZAG_TIMES][dz->itemcnt+1] = dz->lparray[ZIGZAG_TIMES][dz->itemcnt] + dz->lparray[SCRAMBLE_CHUNKLEN][perm[n]];
/* APR 1998 */
            dz->lparray[ZIGZAG_TIMES][dz->itemcnt+1] += dz->iparam[ZIG_SPLSAMPS];
            if(dz->lparray[ZIGZAG_TIMES][dz->itemcnt+1] > worklen) {
                dz->lparray[ZIGZAG_TIMES][dz->itemcnt+1] = worklen;
                dz->lparray[ZIGZAG_TIMES][dz->itemcnt]   = worklen - dz->lparray[SCRAMBLE_CHUNKLEN][perm[n]];
            }
/* APR 1998 */
            dz->iparray[ZIGZAG_PLAY][dz->itemcnt+1]  = FALSE;
            if((total_len += dz->lparray[SCRAMBLE_CHUNKLEN][perm[n]]) > dz->iparam[SCRAMBLE_OUTLEN]) {
                finished = 1;
                break;
            }
            dz->itemcnt += 2;
        }
    }
    if((dz->lparray[ZIGZAG_TIMES] = (int *)realloc(dz->lparray[ZIGZAG_TIMES],dz->itemcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate times store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[ZIGZAG_PLAY] = (int  *)realloc(dz->iparray[ZIGZAG_PLAY], dz->itemcnt * sizeof(int )))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate markers store.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/*********************** GET_BASIS_LENGTHS ***********************/

int get_basis_lengths(int chunkcnt,int worklen,int *unit_len, int *endlen,
                      int *scatgrpcnt, int *endscat, int *range, int *endrange, dataptr dz)
{
    int excess;
    *unit_len = (int)round(worklen/chunkcnt);
    if(ODD(*unit_len))   
        (*unit_len)--;       /* Allow for stereo case */
    excess   = worklen - ((*unit_len) * chunkcnt);
    if(ODD(excess) && dz->infile->channels==2) {
        sprintf(errstr,"Problem in buffer accounting: get_basis_lengths()\n");
        return(PROGRAM_ERROR);
    }
    *endlen  = *unit_len  + excess;
    if(dz->param[SCRAMBLE_SCAT] >= 1.0) {
        dz->iparam[SCRAMBLE_SCAT] = round(dz->param[SCRAMBLE_SCAT]);
        *scatgrpcnt = (int)(chunkcnt/dz->iparam[SCRAMBLE_SCAT]);
        *endscat    = (int)(chunkcnt - ((*scatgrpcnt) * dz->iparam[SCRAMBLE_SCAT]));
        *range      = (*unit_len) * dz->iparam[SCRAMBLE_SCAT];
        *endrange   = ((*endscat - 1) * (*unit_len)) + *endlen;
    } else {
        dz->iparam[SCRAMBLE_SCAT] = 0;
    }
    return(FINISHED);
}

/************************** NORMAL_SCAT *******************************/

int normal_scat(int unit_len,int endlen,int chunkcnt_less_one,dataptr dz)
{
    double this_scatter;
    int n;
    int new_chunklen, total_len = unit_len;     
    for(n=1;n<chunkcnt_less_one;n++) {          
        this_scatter  = (drand48() - 0.5) * dz->param[SCRAMBLE_SCAT];   /* range +- .5 reduced by  *scatter */
        new_chunklen = (int)(this_scatter * (double)unit_len);
        if(ODD(new_chunklen))   
            new_chunklen--;                             /* in case stereo?? */
        dz->lparray[SCRAMBLE_CHUNKPTR][n] = total_len + new_chunklen;           /* position chunk-pointer */
        total_len  += unit_len;                         /* step to next BASE position */
    }
    this_scatter  = (drand48() - 0.5) * dz->param[SCRAMBLE_SCAT];       /* as final chunk may be unique length */
    if(this_scatter<0.0) {                              /* if scatter is -ve, scatter is frac of penultimate length */
        new_chunklen   = (int)(this_scatter * (double)unit_len);
        if(ODD(new_chunklen))   
            new_chunklen--;
        dz->lparray[SCRAMBLE_CHUNKPTR][n] = total_len - new_chunklen;
    } else {                                            /* else, scatter is frac of final length */
        new_chunklen   = (int)(this_scatter * (double)endlen);
        if(ODD(new_chunklen))   
            new_chunklen--;
        dz->lparray[SCRAMBLE_CHUNKPTR][n] = total_len + new_chunklen;
    }
    return(FINISHED);
}

/*********************** HEAVY_SCAT ***************************
 *
 * (1)  Start at the chunk (this=1) AFTER the first (which can't be moved).
 * (2)  STARTPTR marks the start of the chunk GROUP (and will be advanced
 *  by RANGE, which is length of chunk-group).
 * (3)  The loop will generate a set of positions for the chunks in
 *  a chunk-group. In the first chunkgroup the position of the
 *  first chunk (start of file) can't be moved, so loop starts at
 *  (first=) 1. Subsequemt loop passes start at 0.
 * (4)  For eveery chunk-group.
 * (5)    Set the index of the first chunk in this group (start) to the
 *    current index (this).
 * (6)    For every member of this chunk-group.
 * (7)      Generate a random-position within the chunk-grp's range
 *      and check it is not too close ( < SPLICELEN) to the others.
 *        Set a checking flag (OK).
 * (8)        Generate a position within the range, and after the startptr.
 * (9)      Compare it with all previously generated positions in this
 *      chunk-grp AND with last position of previous chunk-group!!
 *      If it's closer than SPLICELEN, set OK = 0, drop out of
        checking loop and generate another position instead.
 * (10)       If the position is OK, drop out of position generating loop..
 * (11)     Advance to next chunk in this group.
 * (12)   Once all this group is done, advance the group startpoint by RANGE.
 * (13)   After FIRST grp, all positions can by varied, so set the initial
 *    loop counter to (first=)0.
 * (14) If there are chunks left over (endscat!=0)..
 *    Follow the same procedure for chunks in end group, using the
 *    alternative variables, endscat and endrange.
 */

int heavy_scat(int scatgrpcnt,int range,int endscat,int endrange,dataptr dz)
{
    int thiss = 0, that, start, n, m, OK;           /* 1 */
    int startptr = 0;                               /* 2 */
    int minlen = (dz->iparam[ZIG_SPLSAMPS] * 2);
    int filend = dz->insams[0];
    int first = 1;                                  /* 3 */
/* TW OCT 1997 */   
    int latest_ptr = 0;
    dz->lparray[SCRAMBLE_CHUNKPTR][thiss++] = 0;
/* TW OCT 1997 */   
    for(n=0;n<scatgrpcnt;n++) {                     /* 4 */
        start = thiss;                              /* 5 */
        for(m=first;m<dz->iparam[SCRAMBLE_SCAT];m++) {                  /* 6 */
            do {                                    /* 7 */
                OK = 1;                 
                dz->lparray[SCRAMBLE_CHUNKPTR][thiss] = (int)(drand48()*range);     /* TRUNCATE */
                dz->lparray[SCRAMBLE_CHUNKPTR][thiss] += startptr;      /* 8 */
                if(ODD(dz->lparray[SCRAMBLE_CHUNKPTR][thiss]))   
                    dz->lparray[SCRAMBLE_CHUNKPTR][thiss]--;    
/* TW OCT 1997 -->*/
                if(abs(dz->lparray[SCRAMBLE_CHUNKPTR][thiss] - latest_ptr)< minlen) {
                    OK = 0;
                    continue;
                }
                for(that=start; that<thiss; that++) {
                    if(abs(dz->lparray[SCRAMBLE_CHUNKPTR][thiss] - dz->lparray[SCRAMBLE_CHUNKPTR][that])<minlen
                    || abs(filend - abs(dz->lparray[SCRAMBLE_CHUNKPTR][thiss]))< minlen) {
/* TW OCT 1997 */
                        OK = 0;                     /* 9 */
                        break;
                    }
                }
            } while(!OK);                           /* 10 */
            thiss++;                                /* 11 */
        }
/* TW OCT 1997 --> */   
        for(m=0;m<thiss;m++) {
            if(dz->lparray[SCRAMBLE_CHUNKPTR][m] > latest_ptr)
                latest_ptr = dz->lparray[SCRAMBLE_CHUNKPTR][m];
        }
/* TW OCT 1997 */   
        startptr += range;                          /* 12 */
        first = 0;                                  /* 13 */
    }

    if(endscat) {                                   /* 14 */
        start = thiss;
        for(m=0;m<endscat;m++) {
            do {
                OK = 1;
                dz->lparray[SCRAMBLE_CHUNKPTR][thiss] = (int)(drand48() * endrange);      /* TRUNCATE */
                dz->lparray[SCRAMBLE_CHUNKPTR][thiss] += startptr;
                if(ODD(dz->lparray[SCRAMBLE_CHUNKPTR][thiss]))   
                    dz->lparray[SCRAMBLE_CHUNKPTR][thiss]--;    
/* TW OCT 1997 */
                if(abs(dz->lparray[SCRAMBLE_CHUNKPTR][thiss] - latest_ptr)< minlen) {
                    OK = 0;
                    continue;
                }
/* TW OCT 1997 */
                for(that=start; that<thiss; that++) {
                    if(abs(dz->lparray[SCRAMBLE_CHUNKPTR][thiss] - dz->lparray[SCRAMBLE_CHUNKPTR][that])< minlen 
/* TW OCT 1997 */
                    || abs(filend - abs(dz->lparray[SCRAMBLE_CHUNKPTR][thiss]))< minlen) {
/* TW OCT 1997 */
                        OK = 0;
                        break;
                    }
                }
            } while(!OK);
            thiss++;
        }
    }
    return ptr_sort(thiss,dz);
}

/*************************** PERMUTE_CHUNKS ***************************/

void permute_chunks(int chunkcnt,dataptr dz)
{
    int n, t;
    for(n=0;n<chunkcnt;n++) {
        t = (int)(drand48() * (double)(n+1));    /* TRUNCATE */
        if(t==n)
            prefix(n,chunkcnt,dz);
        else
            insert(n,t,chunkcnt,dz);
    }
}

/****************************** INSERT ****************************/

void insert(int n,int t,int chunkcnt,dataptr dz)
{
    shuflup(t+1,chunkcnt,dz);
    dz->iparray[SCRAMBLE_PERM][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefix(int n,int chunkcnt,dataptr dz)
{
    shuflup(0,chunkcnt,dz);
    dz->iparray[SCRAMBLE_PERM][0] = n;
}

/****************************** SHUFLUP ****************************/

void shuflup(int k,int chunkcnt,dataptr dz)
{
    int n;
    int *i;
    int z = chunkcnt-1;
    i = &(dz->iparray[SCRAMBLE_PERM][z]);
    for(n = z; n > k; n--) {
        *i = *(i-1);
        i--;
    }
}

/************************** PTR_SORT ***************************/

int ptr_sort(int end,dataptr dz)
{
    int i,j;
    int a;
    for(j=1;j<end;j++) {
        a = dz->lparray[SCRAMBLE_CHUNKPTR][j];
        i = j-1;
        while(i >= 0 && dz->lparray[SCRAMBLE_CHUNKPTR][i] > a) {
            dz->lparray[SCRAMBLE_CHUNKPTR][i+1]=dz->lparray[SCRAMBLE_CHUNKPTR][i];
            i--;
        }
        dz->lparray[SCRAMBLE_CHUNKPTR][i+1] = a;
    }
    return(FINISHED);
}    

/**************************** ITERATE_PREPROCESS ******************************/

int iterate_preprocess(dataptr dz)
{
    int exit_status;
    double maxrand, maxpscat, mindelay, temp;
    int is_unity_gain = FALSE;
    int mindelay_samps;
    if(dz->process == ITERATE_EXTEND) {
        if(dz->iparam[ITER_RRSEED] > 0)
            srand((int)dz->iparam[ITER_RRSEED]);
        else
            initrand48();
    } else {
        if(dz->iparam[ITER_RSEED] > 0)
            srand((int)dz->iparam[ITER_RSEED]);
        else
            initrand48();
    }
    if((exit_status = get_maxvalue_of_rand(&maxrand,dz))<0)
            return(exit_status);
    if((exit_status = get_maxvalue_of_pscat(&maxpscat,dz))<0)
            return(exit_status);
    if((exit_status = get_minvalue_of_delay(&mindelay,dz))<0)
            return(exit_status);
    mindelay_samps = round(mindelay * (double)dz->infile->srate);
    if(dz->process == ITERATE_EXTEND) {
        dz->iparam[CHUNKSTART] = (int)round(dz->param[CHUNKSTART] * dz->infile->srate) * dz->infile->channels;
        dz->iparam[CHUNKEND]   = (int)round(dz->param[CHUNKEND]   * dz->infile->srate) * dz->infile->channels;
        if(dz->param[CHUNKSTART] > dz->param[CHUNKEND]) {
            temp = dz->param[CHUNKSTART];
            dz->param[CHUNKSTART] = dz->param[CHUNKEND];
            dz->param[CHUNKEND] = temp;
        }
        if(dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART] <= ITX_SPLICELEN * dz->infile->channels * 2) {
            sprintf(errstr,"FROZEN SEGMENT (%d samples) TOO SHORT FOR SPLICING (needs %d samples)\n",
                dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART],ITX_SPLICELEN * dz->infile->channels * 2);
            return(DATA_ERROR);
        }
        if(dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART] < mindelay_samps * dz->infile->channels) {
            sprintf(errstr,"FROZEN SEGMENT (%d samples) TOO SHORT FOR (MINIMUM) DELAY TIME SPECIFIED (%d samples)\n",
                dz->iparam[CHUNKEND] - dz->iparam[CHUNKSTART],mindelay_samps * dz->infile->channels);
            return(DATA_ERROR);
        }
    }
    if((dz->process != ITERATE_EXTEND) && (dz->param[ITER_GAIN]==DEFAULT_ITER_GAIN))
        set_default_gain(mindelay_samps,dz);
    set_default_delays(dz);
    if (dz->process == ITERATE_EXTEND) {
        dz->param[ITER_SSTEP] = 1.0; /* 1st sound is exact copy of orig */
    } else {
        reverse_fadevals(dz);
        dz->param[ITER_STEP] = 1.0; /* 1st sound is exact copy of orig */
    }
    if((dz->process != ITERATE_EXTEND) && flteq(dz->param[ITER_GAIN],1.0))
        is_unity_gain = TRUE;
    setup_iter_process_type(is_unity_gain,dz);
    return create_iterbufs(maxpscat,dz);
}

/*************************** GET_MAXVALUE_OF_RAND ****************************/

int get_maxvalue_of_rand(double *maxrand,dataptr dz)
{
    int exit_status;
    if(dz->brksize[ITER_RANDOM]) {
        if((exit_status = get_maxvalue_in_brktable(maxrand,ITER_RANDOM,dz))<0)
            return(exit_status);
    } else
        *maxrand = dz->param[ITER_RANDOM];
    return(FINISHED);
}

/************************* GET_MAXVALUE_OF_PSCAT ****************************/

int get_maxvalue_of_pscat(double *maxpscat,dataptr dz)
{
    int exit_status;
    if(dz->brksize[ITER_PSCAT]) {
        if((exit_status = get_maxvalue_in_brktable(maxpscat,ITER_PSCAT,dz))<0)
            return(exit_status);
    } else
        *maxpscat = dz->param[ITER_PSCAT];
    return(FINISHED);
}

/************************* GET_MINVALUE_OF_DELAY ****************************/

int get_minvalue_of_delay(double *mindelay,dataptr dz)
{
    int exit_status;
    if(dz->brksize[ITER_DELAY]) {
        if((exit_status = get_minvalue_in_brktable(mindelay,ITER_DELAY,dz))<0)
            return(exit_status);
    } else
        *mindelay = dz->param[ITER_DELAY];
    return(FINISHED);
}

/************************* REVERSE_FADEVALS ****************************/

void reverse_fadevals(dataptr dz)
{
    double *p, *pend;

    if(dz->brksize[ITER_FADE]==0)
        dz->param[ITER_FADE] = 1.0 - dz->param[ITER_FADE];
    else {
        p    = dz->brk[ITER_FADE] + 1;
        pend = dz->brk[ITER_FADE] + (dz->brksize[ITER_FADE] * 2);
        while(p < pend) {
            *p = 1.0 - *p;
            p += 2;
        }
    }
}

/************************** SET_DEFAULT_GAIN ****************************/

void set_default_gain(int mindelay_samps,dataptr dz)
{
    int inmsampsize;
    int maxoverlay_cnt;
    inmsampsize = dz->insams[0]/dz->infile->channels;
    maxoverlay_cnt = round(((double)inmsampsize/(double)mindelay_samps)+1.0);
    if(dz->vflag[IS_ITER_RAND])
        maxoverlay_cnt++;
    dz->param[ITER_GAIN]   = 1.0/(double)maxoverlay_cnt;
}

/*********************** SET_DEFAULT_DELAYS ****************************/

void set_default_delays(dataptr dz)
{
    if(dz->process==ITERATE_EXTEND) {
        if(!dz->brksize[ITER_DELAY])
            dz->iparam[ITER_MSAMPDEL] = round(dz->param[ITER_DELAY] * (double)dz->infile->srate);
        return;
    }
    if(dz->vflag[IS_ITER_DELAY]) {
        if(!dz->brksize[ITER_DELAY])
            dz->iparam[ITER_MSAMPDEL] = round(dz->param[ITER_DELAY] * (double)dz->infile->srate);
    } else
        dz->iparam[ITER_MSAMPDEL] = dz->insams[0]/dz->infile->channels; /* default */
}

/********************* SETUP_ITER_PROCESS_TYPE ********************************/

void setup_iter_process_type(int is_unity_gain,dataptr dz)
{
    if(dz->process==ITERATE_EXTEND) {
        if(dz->param[ITER_PSCAT] > 0.0) {
            if(dz->infile->channels>1)
                dz->iparam[ITER_PROCESS] = ST_INTP_SHIFT;
            else
                dz->iparam[ITER_PROCESS] = MN_INTP_SHIFT;
        } else {
            if(dz->infile->channels>1)
                dz->iparam[ITER_PROCESS] = STEREO;
            else
                dz->iparam[ITER_PROCESS] = MONO;
        }
        if(flteq(dz->param[ITER_ASCAT],0.0)) {
            dz->iparam[ITER_PROCESS] += FIXED_AMP;
        }
        return;
    }
    dz->iparam[ITER_DO_SCALE] = TRUE;
    if(dz->vflag[IS_ITER_PSCAT]) {
        if(dz->infile->channels>1)
            dz->iparam[ITER_PROCESS] = ST_INTP_SHIFT;
        else
            dz->iparam[ITER_PROCESS] = MN_INTP_SHIFT;
    } else {
        if(dz->infile->channels>1)
            dz->iparam[ITER_PROCESS] = STEREO;
        else
            dz->iparam[ITER_PROCESS] = MONO;
    }
    if(!dz->vflag[IS_ITER_ASCAT]) {
        if(is_unity_gain)
            dz->iparam[ITER_DO_SCALE] = FALSE;
        dz->iparam[ITER_PROCESS] += FIXED_AMP;
    }
}

/***************************** DRUNK_PREPROCESS *************************
 *
 * Splicelen & cloktiks have been converted to SAMPLES in pconsistency.c
 */

int drunk_preprocess(dataptr dz)
{
    int exit_status;
    dz->iparam[DRNK_LAST_LOCUS] = -1;
    initialise_random_sequence(IS_DRNK_RSEED,DRNK_RSEED,dz);
    if((exit_status = create_drunk_buffers(dz))<0)  /* 1 */
        return(exit_status);
    if((exit_status = make_drnk_splicetab(dz))<0)
        return(exit_status);
    if(dz->insams[0] <= dz->iparam[DRNK_SPLICELEN] * dz->infile->channels * 2) {
        sprintf(errstr,"Infile too short for splices.\n");
        return(DATA_ERROR);
    }
    dz->iparam[DRNK_LGRAIN] = round((double)dz->infile->srate * DRNK_GRAIN); 

    dz->iparam[DRNK_TOTALDUR]  = round(dz->param[DRNK_TOTALDUR] * (double)(dz->infile->srate * dz->infile->channels));
    if((exit_status = convert_time_and_vals_to_samplecnts(DRNK_LOCUS,dz))<0)
        return(exit_status);
    if((exit_status = convert_time_and_vals_to_samplecnts(DRNK_AMBITUS,dz))<0)
        return(exit_status);
    if((exit_status = convert_time_to_samplecnts(DRNK_CLOKRND,dz))<0)
        return(exit_status);
    if((exit_status = convert_time_to_samplecnts(DRNK_OVERLAP,dz))<0)
        return(exit_status);
    if((exit_status = get_maxvalue(DRNK_OVERLAP,&dz->param[DRNK_MAX_OVLAP],dz))<0)
        return(exit_status);
    if((exit_status = convert_time_to_samplecnts(DRNK_GSTEP,dz))<0)
        return(exit_status);
    if((exit_status = convert_sec_steps_to_grain_steps(dz))<0)
        return(exit_status);
    if(dz->mode==HAS_SOBER_MOMENTS) {
        if((exit_status = convert_time_and_vals_to_samplecnts(DRNK_MIN_PAUS,dz))<0)
            return(exit_status);
        if((exit_status = convert_time_and_vals_to_samplecnts(DRNK_MAX_PAUS,dz))<0)
            return(exit_status);
        if((exit_status = convert_time_to_samplecnts(DRNK_MIN_DRNKTIK,dz))<0)
            return(exit_status);
        if((exit_status = convert_time_to_samplecnts(DRNK_MAX_DRNKTIK,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/********************** MAKE_DRNK_SPLICETAB *********************/

int make_drnk_splicetab(dataptr dz)
{
    int n;
    if((dz->parray[DRNK_SPLICETAB] = (double *)malloc(dz->iparam[DRNK_SPLICELEN] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to make splicetable.\n");
        return(MEMORY_ERROR);
    } 
    for(n=0;n<dz->iparam[DRNK_SPLICELEN];n++)
        dz->parray[DRNK_SPLICETAB][n] = (double)n/(double)dz->iparam[DRNK_SPLICELEN];
    return(FINISHED);
 }

/************************* CONVERT_SEC_STEPS_TO_GRAIN_STEPS *********************/

int convert_sec_steps_to_grain_steps(dataptr dz)
{
    double *p, *pend;
    int lval;
    if(dz->iparam[DRNK_LGRAIN]<=0) {
        sprintf(errstr,"Invalid DRNK_LGRAIN val: convert_sec_steps_to_grain_steps()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->brksize[DRNK_GSTEP]) {
        p    = dz->brk[DRNK_GSTEP] + 1;
        pend = dz->brk[DRNK_GSTEP] + (dz->brksize[DRNK_GSTEP] * 2);
        while(p < pend) {
            lval = round(*p * (double)dz->infile->srate);
            lval = (lval+(dz->iparam[DRNK_LGRAIN]/2))/dz->iparam[DRNK_LGRAIN];
            *p   = (double)lval;
            p += 2;
        }
    } else {
        dz->iparam[DRNK_GSTEP] = round(dz->param[DRNK_GSTEP] * (double)dz->infile->srate);
        dz->iparam[DRNK_GSTEP] = (dz->iparam[DRNK_GSTEP]+(dz->iparam[DRNK_LGRAIN]/2))/dz->iparam[DRNK_LGRAIN];
        dz->param[DRNK_GSTEP]  = (double)dz->iparam[DRNK_GSTEP];
    }
    return(FINISHED);
}
