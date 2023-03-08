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
 License aint with the CDP System; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 *
 */
/* floatsam vesion */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <grain.h>
#include <cdpmain.h>

#define maxtime scalefact
#include <sfsys.h>
#include <osbind.h>
#include <grain.h>
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif
#ifdef unix
#define round(x) lround((x))
#endif

static int  copygrain_from_elsewhere(int len,int k,int *obufposition,dataptr dz);
static int store_the_grain_time(int grainstart,int *graincnt,int crosbuf,double samptotime,int init,dataptr dz);
static int  retime_grain(int ibufpos,int thisgap,int grainstart,int origgap,int bufno,int *obufpos,
            int chans,int crosbuf,int *grainadjusted,int store_end,int is_first_grain,dataptr dz);
static int  keep_non_omitted_grains(int ibufpos,int bufno,int grainstart,int *graincnt,
            int *obufpos,int crosbuf,int chans,int is_first_grain,dataptr dz);
static int  do_the_reordered_grains(int ibufpos,int bufno,int grainstart,int *graincnt,
            int *obufpos,int crosbuf,int chans,int is_first_grain,int is_last_grain,dataptr dz);
static int  synchronise_the_grain(int ibufpos,int *graincnt,int bufno,int gapcnt,int *obufpos,
            int grainstart,int crosbuf,int chans,int *grainadjusted,int is_first_grain,dataptr dz);
static int  do_final_reorder_grains(int graincnt,int *obufpos,dataptr dz);
//TW REVISED
static int  output_final_grain(int grainstart,int start_splice,int bufno,int splicestart_bufno,int is_first_grain,
            int *obufpos,int grainlen,int crosbuf,dataptr dz);
static int  do_rerhythm_grain(int ibufpos,int *graincnt,int bufno,int gapcnt,int *obufpos,int grainstart,
            int crosbuf,int chans,int *grainadjusted,int is_first_grain,dataptr dz);
static void swap_array_adresses_and_lens_for_reordr(int n,int m,dataptr dz);
static int  copy_grain_to_buf(int ibufpos,int bufno,int grainstart,int crosbuf,int chans,int n,dataptr dz);
//TW REVISED
static int  retime_pre_firstgrain_material
            (int new_grainstart,int *obufposition,int chans,int crosbuf,dataptr dz);
static int  output_grain_link(int thisgap,int abs_halfsplice,int *obufposition,dataptr dz);
static int  copy_start_of_grain(int grainstart,int start_splice,int bufno,int splicestart_bufno,
            int *obufpos,int crosbuf,dataptr dz);
static int  read_dn_halfsplice(int *obufposition,int len,dataptr dz);
static int  create_an_upsplice_from_pregrain_material(int obufpos,int chans,dataptr dz);
static int  duplicate_grain(int ibufpos,int bufno,int gapcnt,int *obufpos,int grainstart,
            int crosbuf,int chans,int *grainadjusted,int is_first_grain,dataptr dz);
static int  prepare_final_grain(int ibufpos,int *start_splice,int bufno,int *splicestart_bufno,
            int grainstart,int *grainlen,int crosbuf,int chans,dataptr dz);
static int  read_up_halfsplice(int *obufposition,int len,dataptr dz);
static int  store_up_halfsplice(int storeno,int start,int bufno,int chans,int splicelen,dataptr dz);
static int  store_dn_halfsplice(int start,int bufno,int chans,int splicelen,dataptr dz);
//TW REVISED
static int  duplicate_last_grain(int ibufpos,int bufno,int *obufpos,int grainstart,
            int crosbuf,int chans,int is_first_grain,dataptr dz);
static int  store_upsplice_of_next_grain(int ibufpos,int bufno,int abs_halfsplice,int chans,dataptr dz);
static int  copy_last_grain_to_buf(int ibufpos,int bufno,int grainstart,int crosbuf,int chans,int n,dataptr dz);
static int  copy_up_halfsplice_to_grainstore(int len,int storeno,int storelen,int *storepos,dataptr dz);
static int  copy_dn_halfsplice_to_grainstore(int len,int storeno,int storelen,int storepos,dataptr dz);
static int  copy_midgrain_to_store(int mid_grainlen,int bufno,int storelen,int grainstart,
            int *storepos,int crosbuf,int storeno,dataptr dz);;
static int  put_grain_into_store(int is_last_grain,int ibufpos,int bufno,int grainstart,
            int crosbuf,int chans,int graincnt,dataptr dz);
static int  save_abs_sampletime_of_grain(int grainstart,int *graincnt,int crosbuf,dataptr dz);
static int  do_seek_and_read(int *seekbufs, int *seeksamps,int grainno,dataptr dz);
static int  test_buffer_overflows(float *obuf,int *obufpos,int *ibufpos,int n,int maxlen,int *crosbuf,dataptr dz);
static int  do_the_reversing_process(int graincnt,int *obufposition,int chans,dataptr dz);
static int  output_up_halfsplice(int *ibufpos,int *obufpos,int chans,int splicelen,int *crosbuf,dataptr dz);
static int  output_dn_halfsplice(int *ibufpos,int *obufpos,int chans,int splicelen,int *crosbuf,dataptr dz);
static int  copy_midgrain_to_output(int mid_grainlen,int *ibufpos,int *obufpos,int *crosbuf,int chans,dataptr dz);
static int  insert_EOF_sampletime_in_samptime_list(int graincnt,dataptr dz);
static void adjust_for_last_grain(int *graincnt,int *grainpos,dataptr dz);
static int  clear_outbuf(dataptr dz);
static int  output_whole_grain(int n,int *obufpos,int *seeksamps,int *seekbufs,
            int halfsplice,int abs_splicelen,int chans,dataptr dz);
static int  adjust_firstgrain_upsplice(int *ibufpos,int *startsplice,int n,int chans);
static int  do_grain_repitching(int *actual_grainlen,int bufno,int grainstart,
            int new_grainlen,int orig_grainlen,int chans,int crosbuf,double pichratio,dataptr dz);
static int  test_for_bufcros(int *thishere,float **b,int bufno,int crosbuf,dataptr dz);
static int  save_nextgrain_upsplice_elsewhere(int ibufpos,int bufno,int abs_halfsplice,int halfsplice,
            int chans,dataptr dz);
static int  copygrain_from_grainbuf_to_outbuf(int *obufpos,int grainlen,dataptr dz);
static int  retrieve_upsplice_for_nextgrain(int abs_halfsplice,dataptr dz);
static int  save_origgrain_length_and_store_nextgrains_upsplice(int *orig_grainlen,int ibufpos,int bufno,
            int grainstart,int abs_halfsplice,int halfsplice,int crosbuf,int chans,int is_last_grain,dataptr dz);
//TW REVISED
static int  create_repitched_and_retimed_grain(int bufno,int grainstart,int orig_grainlen,
            int *new_grainlen,double pichratio,double timeratio,int *obufpos,int *is_first_grain,
            int *grainadjusted,int abs_halfsplice,int chans,int crosbuf,dataptr dz);
static int  repitch_main_body_of_grain(int bufno,int grainstart,int new_grainlen,int orig_grainlen,
            double pichratio,int chans,int crosbuf,int halfsplice,int abs_halfsplice,dataptr dz);
//TW REVISED
static int  retime_main_body_of_grain(int grainstart,int bufno,double pichratio,
            double timeratio,int chans,int crosbuf,int *grainadjusted,
            int orig_grainlen,int *new_grainlen,dataptr dz);
static int  repitching_process(int ibufpos,int bufno,int grainstart,int abs_halfsplice,int halfsplice,
            int *graincnt,int *obufpos,int *orig_grainlen,int *new_grainlen,int *grainadjusted,
            int *is_first_grain,int is_last_grain,int chans,int crosbuf,dataptr dz);
static int  repitching_and_retiming_process(int ibufpos,int bufno,int grainstart,int abs_halfsplice,
            int halfsplice,int *graincnt,int *obufpos,int *orig_grainlen,int *new_grainlen,
            int *grainadjusted,int *is_first_grain,int is_last_grain,int chans,int crosbuf,dataptr dz);
static int  do_in_situ_halfsplice_on_stored_grain(int bufpos,int chans,int splicelen,int grainstorelen,dataptr dz);
static int locate_zero_crossings(int local_minima_cnt,dataptr dz);
static int which_minimum_to_eliminate(int minsegstartpos,int *pos,int maxseg,int segcnt,int *eliminate_pos,dataptr dz);
static int eliminate_excess_minima(int *local_minima_cnt,int *pos,dataptr dz);
static int find_all_local_minima(int peakcnt,int *local_minima_cnt,dataptr dz);
static int find_all_positive_peaks(int startsearch,int endsearch,int *peakcnt,dataptr dz);
static int eliminate_spurious_minima(int *local_minima_cnt,int *minimum_element_len,dataptr dz);
static void hhshuflup(int k,int setlen,int *perm);
static void hhprefix(int m,int setlen,int *perm);
static void hhinsert(int m,int t,int setlen,int *perm);
static void do_repet_restricted_perm(int *arr, int *perm, int arrsiz, int allowed, int endval);
static int rand_ints_with_restricted_repeats(int element_cnt,int max_elements_needed,int arrsiz,int fullperms,int **pattern,dataptr dz);
static int  extract_rrr_env_from_sndfile(int paramno,dataptr dz);
static void get_rrrenv_of_buffer(int samps_to_process,int envwindow_sampsize,float **envptr,float *buffer);
static float getmaxsampr(int startsamp, int sampcnt,float *buffer);

static int do_envgrain_write(int startsearch,int endsearch,int *last_total_samps_read,int *obufpos,dataptr dz);
static int do_envgrain_zerowrite(int startsearch,int endsearch,int *obufpos,dataptr dz);
static int do_envgrain_addwrite(int startsearch,int endsearch,int *last_total_samps_read,int *obufpos,dataptr dz);
static int do_envgrain_zerowrite_dblbuf(int startsearch,int endsearch,int *obufpos,dataptr dz);

/************************** DO_THE_GRAIN **********************/

int do_the_grain(int ibufpos,int *graincnt,int bufno,int gapcnt,int *obufpos,int grainstart,
int crosbuf,int chans,int *grainadjusted,double samptotime,
int *is_first_grain,dataptr dz)
{
    int    exit_status;
    int    is_last_grain = FALSE;
    int   newgapcnt, orig_grainlen, new_grainlen;
    int    abs_splicelen  = dz->iparam[GR_ABS_SPLICELEN];
    int    abs_halfsplice = abs_splicelen/2;      /* guaranteed to fall on stereo boundary */
    int    halfsplice = dz->iparam[GR_SPLICELEN]/2;
    int store_end;
    static int init = 1;

    switch(dz->process) {
    case(GRAIN_COUNT):
        (*graincnt)++;
        break;
    case(GRAIN_GET):
    case(GRAIN_ALIGN):
        if((exit_status = store_the_grain_time(grainstart,graincnt,crosbuf,samptotime,init,dz))<0)
            return(exit_status);
        init = 0;
        break;
    case(GRAIN_REPITCH):
        if((exit_status = repitching_process(ibufpos,bufno,grainstart,abs_halfsplice,halfsplice,graincnt,
        obufpos,&orig_grainlen,&new_grainlen,grainadjusted,is_first_grain,is_last_grain,chans,crosbuf,dz))<0)
            return(exit_status);
        if((exit_status = retrieve_upsplice_for_nextgrain(abs_halfsplice,dz))<0)
            return(exit_status);
        break;
    case(GRAIN_REMOTIF):
        if((exit_status = repitching_and_retiming_process(ibufpos,bufno,grainstart,abs_halfsplice,halfsplice,graincnt,
        obufpos,&orig_grainlen,&new_grainlen,grainadjusted,is_first_grain,is_last_grain,chans,crosbuf,dz))<0)
            return(exit_status);
        if((exit_status = retrieve_upsplice_for_nextgrain(abs_halfsplice,dz))<0)
            return(exit_status);
        break;
    case(GRAIN_RERHYTHM):
        if((exit_status = do_rerhythm_grain
        (ibufpos,graincnt,bufno,gapcnt,obufpos,grainstart,crosbuf,chans,grainadjusted,*is_first_grain,dz))<0)
            return(exit_status);
        break;
    case(GRAIN_OMIT):
        if((exit_status = keep_non_omitted_grains
        (ibufpos,bufno,grainstart,graincnt,obufpos,crosbuf,chans,*is_first_grain,dz))<0)
            return(exit_status);
        break;
    case(GRAIN_REORDER):
        if((exit_status = do_the_reordered_grains
        (ibufpos,bufno,grainstart,graincnt,obufpos,crosbuf,chans,*is_first_grain,is_last_grain,dz))<0)
            return(exit_status);
        break;
    case(GRAIN_DUPLICATE):
        if((exit_status = duplicate_grain
        (ibufpos,bufno,gapcnt,obufpos,grainstart,crosbuf,chans,grainadjusted,*is_first_grain,dz))<0)
            return(exit_status);
        break;
    case(GRAIN_TIMEWARP):
        newgapcnt = round((double)(gapcnt/chans) * dz->param[GR_TSTRETCH]) * chans;
        store_end = TRUE;
        if((exit_status = retime_grain
        (ibufpos,newgapcnt,grainstart,gapcnt,bufno,obufpos,chans,crosbuf,grainadjusted,store_end,*is_first_grain,dz))<0)
            return(exit_status);
        break;
    case(GRAIN_REVERSE):
        if(*is_first_grain) {
            fprintf(stdout,"INFO: Searching for grains\n");
            fflush(stdout);
        }
        if((exit_status = save_abs_sampletime_of_grain(grainstart,graincnt,crosbuf,dz))<0)
            return(exit_status);
        break;
    case(GRAIN_POSITION):
        if((exit_status = synchronise_the_grain
        (ibufpos,graincnt,bufno,gapcnt,obufpos,grainstart,crosbuf,chans,grainadjusted,*is_first_grain,dz))<0)
            return(exit_status);
        if(exit_status!=CONTINUE)
            return(FINISHED);
        break;
    default:
        sprintf(errstr,"Unknown case in do_the_grain()\n");
        return(PROGRAM_ERROR);
    }
    *is_first_grain = FALSE;
    return(CONTINUE);
}

/************************** STORE_THE_GRAIN_TIME **********************/

int store_the_grain_time(int grainstart,int *graincnt,int crosbuf,double samptotime,int init,dataptr dz)
{
    int previous_total_ssampsread = dz->total_samps_read - dz->ssampsread;
    int sampcnt = grainstart + previous_total_ssampsread;
    if(init) {
        if(*graincnt == 0) {
            sprintf(errstr,"No grains found.\n");
            return(GOAL_FAILED);
        }
        if((dz->parray[GR_SYNCTIME] = (double *)malloc(*graincnt * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store sync times.\n");
            return(MEMORY_ERROR);
        }
        dz->iparam[GR_SYNCCNT] = *graincnt;
        *graincnt = 0;
    } else if(dz->parray[GR_SYNCTIME] == NULL) {
        sprintf(errstr,"No grains found.\n");
        return(GOAL_FAILED);
    }
    if(crosbuf)
        sampcnt -= dz->buflen;
    dz->parray[GR_SYNCTIME][(*graincnt)++] = (double)sampcnt * samptotime;
    if(*graincnt > dz->iparam[GR_SYNCCNT]) {
        sprintf(errstr,"Error in memory asignment for storing grain times.\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/************************* KEEP_NON_OMITTED_GRAINS *************************/

int keep_non_omitted_grains
(int ibufpos,int bufno,int grainstart,int *graincnt,int *obufpos,int crosbuf,int chans,int is_first_grain,dataptr dz)
{
    int  exit_status;
    int start_splice;
    int  abs_splicelen  = dz->iparam[GR_ABS_SPLICELEN];
    int  abs_halfsplice = abs_splicelen/2;    /* guaranteed to fall on stereo boundary */
    int  halfsplice = dz->iparam[GR_SPLICELEN]/2;
    int  splicestart_bufno = bufno;

                                            /* Write previously stored up_splice to THIS grain */
    if(*graincnt<dz->iparam[GR_KEEP] &&!is_first_grain) {
        if((exit_status = read_up_halfsplice(obufpos,abs_halfsplice,dz))<0)
            return(exit_status);
    }
    start_splice = ibufpos - abs_halfsplice;    /* ALWAYS KEEP up_splice to NEXT grain */
    if(start_splice < 0) {
        splicestart_bufno = !bufno;
        start_splice += dz->buflen;
    }
    if((exit_status = store_up_halfsplice(1,start_splice,splicestart_bufno,chans,halfsplice,dz))<0)
        return(exit_status);

    if(*graincnt<dz->iparam[GR_KEEP]) {
        start_splice -= abs_halfsplice;
        if(start_splice < 0) {
            splicestart_bufno = !bufno;
            start_splice += dz->buflen;
        }
        if((exit_status = store_dn_halfsplice(start_splice,splicestart_bufno,chans,halfsplice,dz))<0)
            return(exit_status);
        if((exit_status = copy_start_of_grain(grainstart,start_splice,bufno,splicestart_bufno,obufpos,crosbuf,dz))<0)
            return(exit_status);
        if((exit_status = read_dn_halfsplice(obufpos,abs_halfsplice,dz))<0)
            return(exit_status);
    }
    if(++(*graincnt)>=dz->iparam[GR_OUT_OF])
        *graincnt = 0;
    return(FINISHED);
}

/************************* DO_THE_REORDERED_GRAINS *************************/

int do_the_reordered_grains(int ibufpos,int bufno,int grainstart,int *graincnt,
int *obufpos,int crosbuf,int chans,int is_first_grain,int is_last_grain,dataptr dz)
{
    int exit_status;
    int n, m, k;
    int len;
    int  abs_halfsplice = dz->iparam[GR_ABS_SPLICELEN]/2;   /* guaranteed to fall on stereo boundary */

    if(*graincnt > dz->iparam[GR_REOLEN]) {             /* WE'RE BEYOND PERMUTE-SET: LOOKING AT GRAINS NOT USED */
        if(*graincnt == dz->iparam[GR_REOSTEP]-1) {     /* IF looking at very last unused grain */
            if((exit_status = store_upsplice_of_next_grain(ibufpos,bufno,abs_halfsplice,chans,dz))<0)
                return(exit_status);/* store startsplice of nextgrain, found at end of final skipped grain */
            *graincnt = -1;         /* and reset grain counter to indicate end of skipped-over grains */
        }                           /* Otherwise, completely ignore skipped over grain */

    } else if(*graincnt == dz->iparam[GR_REOLEN]) {     /* WE'VE REACHED THE END OF THE PERMUTABLE SET */
        for(n=0;n<dz->iparam[GR_REOCNT];n++) {
            k   = dz->iparray[GR_REOSET][n];                        /* write permuted set to outbuf */
            len = dz->lparray[GR_THIS_LEN][k];
            if((exit_status = copygrain_from_elsewhere(len,k,obufpos,dz))<0)
                return(exit_status);
        } 
        if(dz->iparam[GR_REOSTEP] < dz->iparam[GR_REOLEN]) {           /* IF NEXT PERMUTE SET BEGINS BEFORE END OF THIS */
            for(n=0,m=dz->iparam[GR_REOSTEP];m < dz->iparam[GR_REOLEN];n++,m++) 
                swap_array_adresses_and_lens_for_reordr(n,m,dz);        /* recycle the buffer storage */
            *graincnt = dz->iparam[GR_REOLEN] - dz->iparam[GR_REOSTEP]; /* & reduce graincnt to no. of reused grains */
            if((exit_status = put_grain_into_store(is_last_grain,ibufpos,bufno,grainstart,crosbuf,chans,*graincnt,dz))<0)
                return(exit_status);                                    /* Store the current grain */
        } else if(dz->iparam[GR_REOSTEP] == dz->iparam[GR_REOLEN]) {   /* IF IT BEGINS AT END OF THIS ONE */
            *graincnt = 0;                                              /* reset grain counter to zero, for next set */
            if((exit_status = put_grain_into_store(is_last_grain,ibufpos,bufno,grainstart,crosbuf,chans,*graincnt,dz))<0)
                return(exit_status);                                /* Store the current grain */
        } else if (dz->iparam[GR_REOSTEP] == dz->iparam[GR_REOLEN]+1) {/* IF IT BEGINS AT 1st GRAIN BEYOND THIS SET */
            if((exit_status = store_upsplice_of_next_grain(ibufpos,bufno,abs_halfsplice,chans,dz))<0)
                return(exit_status);/* store startsplice of nextgrain, found at end of final skipped grain */
            *graincnt = -1;         /* and reset grain counter to indicate end of skipped-over grains */
        }                                                              /* IF IT BEGINS BEYOND THAT : DO NOTHING */

    } else {                                            /* WE'RE AT START OF, OR WITHIN, PERMUTABLE SET */
        if(is_first_grain) {
            if((exit_status = create_an_upsplice_from_pregrain_material(*obufpos,chans,dz))<0)
                return(exit_status);                                /* Generate upsplice to 1st grain, of correct size */
            memset((char *)dz->sampbuf[2],0,(size_t)dz->buflen * sizeof(float));
            *obufpos = 0;
        }
        if((exit_status = put_grain_into_store(is_last_grain,ibufpos,bufno,grainstart,crosbuf,chans,*graincnt,dz))<0)
            return(exit_status);                                    /* Store the current grain */
    }
    (*graincnt)++;
    return(FINISHED);
}

/*********************** SYNCHRONISE_THE_GRAIN ***************************/

int synchronise_the_grain(int ibufpos,int *graincnt,int bufno,int gapcnt,int *obufpos,
int grainstart,int crosbuf,int chans,int *grainadjusted,int is_first_grain,dataptr dz)
{
    int exit_status;
    int newgapcnt;
    int store_end = TRUE;
    if(*graincnt >= dz->iparam[GR_SYNCCNT])
        return(FINISHED);
    newgapcnt = round(dz->parray[GR_SYNCTIME][*graincnt]);
    if(*graincnt==0) {
        if((exit_status = retime_pre_firstgrain_material(newgapcnt,obufpos,chans,crosbuf,dz))<0)
            return(exit_status);
        (*graincnt)++;
    }
    newgapcnt = round(dz->parray[GR_SYNCTIME][*graincnt]);
    if((exit_status = retime_grain(ibufpos,newgapcnt,grainstart,gapcnt,
    bufno,obufpos,chans,crosbuf,grainadjusted,store_end,is_first_grain,dz))<0)
        return(exit_status);
    (*graincnt)++;
    return(CONTINUE);
}


/************************* DEAL_WITH_LAST_GRAINS *************************/
//TW REVISED
int deal_with_last_grains
(int ibufpos,int bufno,int *graincnt,int grainstart,int *grainadjusted,
int *obufpos,int crosbuf,int chans,double samptotime,int *is_first_grain,dataptr dz)
{
    int    exit_status;
    int    is_last_grain = TRUE;
    int    abs_splicelen  = dz->iparam[GR_ABS_SPLICELEN];
    int    abs_halfsplice = abs_splicelen/2;      /* guaranteed to fall on stereo boundary */
    int    halfsplice = dz->iparam[GR_SPLICELEN]/2;
    int   n, start_splice, grainlen, orig_grainlen, new_grainlen;
    int splicestart_bufno;
    switch(dz->process) {
    case(GRAIN_COUNT):
        if(!dz->vflag[LOSE_LAST_GRAIN])
            (*graincnt)++;
        sprintf(errstr,"%d grains found at this gate level.\n",*graincnt);
        break;
    case(GRAIN_GET):
        if(!dz->vflag[LOSE_LAST_GRAIN]) {
            if((exit_status = store_the_grain_time(grainstart,graincnt,crosbuf,samptotime,0,dz))<0)
                return(exit_status);
        }
        for(n=0;n < *graincnt;n++)
            fprintf(dz->fp,"%lf\n",dz->parray[GR_SYNCTIME][n]);
        break;
    case(GRAIN_REVERSE):
        if((exit_status = save_abs_sampletime_of_grain(grainstart,graincnt,crosbuf,dz))<0)
            return(exit_status);
        if(*graincnt <= 1) {
            sprintf(errstr,"No grains found.\n");
            return(DATA_ERROR);
        }
        fprintf(stdout,"INFO: Reversing grain order\n");
        fflush(stdout);
        if((exit_status = do_the_reversing_process(*graincnt,obufpos,chans,dz))<0)
            return(exit_status);
        break;
    case(GRAIN_ALIGN):
        if(!dz->vflag[LOSE_LAST_GRAIN]) {
            if((exit_status = store_the_grain_time(grainstart,graincnt,crosbuf,samptotime,0,dz))<0)
                return(exit_status);
        }
        if(dz->iparam[GR_SYNCCNT] != *graincnt)
            dz->iparam[GR_SYNCCNT] = *graincnt;
        break;
    case(GRAIN_REORDER):
        if(!dz->vflag[LOSE_LAST_GRAIN]) {
            if((exit_status = do_the_reordered_grains
            (ibufpos,bufno,grainstart,graincnt,obufpos,crosbuf,chans,*is_first_grain,is_last_grain,dz))<0)
                return(exit_status);
        }
        if((exit_status = do_final_reorder_grains(*graincnt,obufpos,dz))<0)
            return(exit_status);
        break;
    case(GRAIN_RERHYTHM):
    case(GRAIN_TIMEWARP):
    case(GRAIN_POSITION):
        if(!dz->vflag[LOSE_LAST_GRAIN]) {
            if((exit_status = prepare_final_grain
            (ibufpos,&start_splice,bufno,&splicestart_bufno,grainstart,&grainlen,crosbuf,chans,dz))<0)
                return(exit_status);
            if(exit_status==FINISHED)
                break;
            if((exit_status = output_final_grain
//TW REVISED
            (grainstart,start_splice,bufno,splicestart_bufno,*is_first_grain,obufpos,grainlen,crosbuf,dz))<0)
                return(exit_status);
        }
        break;
    case(GRAIN_OMIT):
        if(!dz->vflag[LOSE_LAST_GRAIN]
        && *graincnt<dz->iparam[GR_KEEP]) {
            if((exit_status = prepare_final_grain
            (ibufpos,&start_splice,bufno,&splicestart_bufno,grainstart,&grainlen,crosbuf,chans,dz))<0)
                return(exit_status);
            if(exit_status==FINISHED)
                break;
            if((exit_status = output_final_grain
//TW REVISED
            (grainstart,start_splice,bufno,splicestart_bufno,*is_first_grain,obufpos,grainlen,crosbuf,dz))<0)
                return(exit_status);
        }
        break;
    case(GRAIN_DUPLICATE):
        if(!dz->vflag[LOSE_LAST_GRAIN]) {
        if((exit_status = duplicate_last_grain
//TW REVISED
        (ibufpos,bufno,obufpos,grainstart,crosbuf,chans,*is_first_grain,dz))<0)
            return(exit_status);
        }
        break;
    case(GRAIN_REPITCH):
        if(!dz->vflag[LOSE_LAST_GRAIN]) {
            if((exit_status = repitching_process(ibufpos,bufno,grainstart,abs_halfsplice,halfsplice,graincnt,
            obufpos,&orig_grainlen,&new_grainlen,grainadjusted,is_first_grain,is_last_grain,chans,crosbuf,dz))<0)
                return(exit_status);
        }
        break;
    case(GRAIN_REMOTIF):
        if(!dz->vflag[LOSE_LAST_GRAIN]) {
            if((exit_status = repitching_and_retiming_process(ibufpos,bufno,grainstart,abs_halfsplice,halfsplice,graincnt,
            obufpos,&orig_grainlen,&new_grainlen,grainadjusted,is_first_grain,is_last_grain,chans,crosbuf,dz))<0)
                return(exit_status);
        }
        break;
    default:
        sprintf(errstr,"Unknown case in deal_with_last_grains()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/************************* DO_FINAL_REORDER_GRAINS *************************/

int do_final_reorder_grains(int graincnt,int *obufpos,dataptr dz)
{
    int exit_status;
    int n, k;
    int len;
    if(graincnt > 0 && graincnt <= dz->iparam[GR_REOCNT]) { /* There are grains still to permute */
        for(n=0;n<dz->iparam[GR_REOCNT];n++) {
            if((k = dz->iparray[GR_REOSET][n])>=graincnt)   /* If required grain doesn't exist, finish */
                return(FINISHED);
            len = dz->lparray[GR_THIS_LEN][k];
            if((exit_status = copygrain_from_elsewhere(len,k,obufpos,dz))<0)
                return(exit_status);
        }
    }
    return(FINISHED);
}

/************************* DO_RERHYTHM_GRAIN *************************/

int do_rerhythm_grain(int ibufpos,int *graincnt,int bufno,int gapcnt,
int *obufpos,int grainstart,int crosbuf,int chans,int *grainadjusted,int is_first_grain,dataptr dz)
{
    int exit_status;
    int newgapcnt, n;
    int store_end;
    double *ratio = dz->parray[GR_RATIO];
    switch(dz->mode) {
    case(GR_NO_REPEATS):
        store_end = TRUE;
        newgapcnt = round((double)(gapcnt/chans) * ratio[*graincnt]) * chans;
        if((exit_status = retime_grain
        (ibufpos,newgapcnt,grainstart,gapcnt,bufno,obufpos,chans,crosbuf,grainadjusted,store_end,is_first_grain,dz))<0)
            return(exit_status);
        if(++(*graincnt)>=dz->iparam[GR_RATIOCNT])
            *graincnt = 0;
        break;
    case(GR_REPEATS):
        store_end = FALSE;
        if(is_first_grain && dz->iparam[GR_RATIOCNT]>1) {
            if((exit_status = create_an_upsplice_from_pregrain_material(*obufpos,chans,dz))<0)
                return(exit_status);
        }
        for(n=0; n<dz->iparam[GR_RATIOCNT]-1; n++) {
            newgapcnt = round((double)(gapcnt/chans) * ratio[n]) * chans;
            if((exit_status = retime_grain
            (ibufpos,newgapcnt,grainstart,gapcnt,bufno,obufpos,chans,crosbuf,grainadjusted,store_end,is_first_grain,dz))<0)
                return(exit_status);
            is_first_grain = FALSE;
        }
        store_end = TRUE;
        newgapcnt = round((double)(gapcnt/chans) * ratio[n]) * chans;
        if((exit_status = retime_grain
        (ibufpos,newgapcnt,grainstart,gapcnt,bufno,obufpos,chans,crosbuf,grainadjusted,store_end,is_first_grain,dz))<0)
            return(exit_status);
        break;
    default:
        sprintf(errstr,"Unknown case in do_rerhythm_grain()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/************************* DUPLICATE_GRAIN *************************/

int duplicate_grain(int ibufpos,int bufno,int gapcnt,int *obufpos,int grainstart,
int crosbuf,int chans,int *grainadjusted,int is_first_grain,dataptr dz)
{
    int exit_status;
    int newgapcnt = gapcnt, n;
    int store_end = FALSE;
    int dupl = (int)dz->iparam[GR_DUPLS];
    if(is_first_grain && dupl>1) {
        if((exit_status = create_an_upsplice_from_pregrain_material(*obufpos,chans,dz))<0)
            return(exit_status);
    }
    for(n=0; n<dupl-1; n++) {
        if((exit_status = retime_grain
        (ibufpos,newgapcnt,grainstart,gapcnt,bufno,obufpos,chans,crosbuf,grainadjusted,store_end,is_first_grain,dz))<0)
            return(exit_status);
        is_first_grain = FALSE;
    }
    store_end = TRUE;
    if((exit_status = retime_grain
    (ibufpos,newgapcnt,grainstart,gapcnt,bufno,obufpos,chans,crosbuf,grainadjusted,store_end,is_first_grain,dz))<0)
        return(exit_status);
    return(FINISHED);
}

/************************* DUPLICATE_LAST_GRAIN *************************/

//TW REVISED
int duplicate_last_grain(int ibufpos,int bufno,int *obufpos,int grainstart,
int crosbuf,int chans,int is_first_grain,dataptr dz)
{
    int exit_status;
    int /*newgapcnt = gapcnt,*/ n;
    /*int store_end = FALSE;*/
    int start_splice, grainlen;
    int splicestart_bufno;
    int dupl = (int)dz->iparam[GR_DUPLS];
    if(is_first_grain && dupl>1) {
        if((exit_status = create_an_upsplice_from_pregrain_material(*obufpos,chans,dz))<0)
            return(exit_status);
    }
    if((exit_status = prepare_final_grain
    (ibufpos,&start_splice,bufno,&splicestart_bufno,grainstart,&grainlen,crosbuf,chans,dz))<0)
        return(exit_status);
    if(exit_status==FINISHED)
        return(FINISHED);
    for(n=0; n<dupl; n++) {
//TW REVISED
    if((exit_status = output_final_grain
        (grainstart,start_splice,bufno,splicestart_bufno,is_first_grain,obufpos,grainlen,crosbuf,dz))<0)
            return(exit_status);
        is_first_grain = FALSE;
    }
    return(FINISHED);
}

/************************** COPYGRAIN_FROM_ELSEWHERE ******************************/

int copygrain_from_elsewhere(int len,int k,int *obufposition,dataptr dz)
{
    int exit_status;
    register int n, obufpos = *obufposition;
    float *obuf = dz->sampbuf[2];
    float *buf = dz->extrabuf[k+SPLBUF_OFFSET];
    for(n=0;n<len;n++) {
        obuf[obufpos++] = buf[n];
        if(obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;
        }
    }
    *obufposition = obufpos;
    return(FINISHED);
}

/************************* SWAP_ARRAY_ADRESSES_AND_LENS_FOR_REORDR *************************
 *
 * The arrays still needed are now pointed to by the lower-indexed lparrays.
 * The arrays NO intER required get swapped, to be pointed to by the higher-indexed lparrays,
 * hence are overwritten as the process proceeds.
 *
 * This works (I hope) even where an index is swapped over more than once!!
 */

void swap_array_adresses_and_lens_for_reordr(int n,int m,dataptr dz)
{
    int n_bufno = n + SPLBUF_OFFSET;
    int m_bufno = m + SPLBUF_OFFSET;
    float *tempadr = dz->extrabuf[n_bufno];
    int temp_arraylen = dz->lparray[GR_ARRAYLEN][n];
    int templen       = dz->lparray[GR_THIS_LEN][n];
    dz->extrabuf[n_bufno] = dz->extrabuf[m_bufno];
    dz->extrabuf[m_bufno] = tempadr;
    dz->lparray[GR_ARRAYLEN][n] = dz->lparray[GR_ARRAYLEN][m];
    dz->lparray[GR_ARRAYLEN][m] = temp_arraylen;
    dz->lparray[GR_THIS_LEN][n] = dz->lparray[GR_THIS_LEN][m];
    dz->lparray[GR_THIS_LEN][m] = templen;
}

/************************* COPY_GRAIN_TO_BUF *************************/

int copy_grain_to_buf(int ibufpos,int bufno,int grainstart,int crosbuf,int chans,int storeno,dataptr dz)
{
    int exit_status;
    int grainlen = ibufpos - grainstart;
    int storepos = 0, mid_grainlen;
    int abs_splicelen = dz->iparam[GR_ABS_SPLICELEN];
    int abs_halfsplice = abs_splicelen/2;
    int halfsplice = dz->iparam[GR_SPLICELEN]/2;
    int splicestart;
    int  grainbufno = storeno + SPLBUF_OFFSET;
    int splicestart_bufno = bufno;
    if(crosbuf)
        grainlen += dz->buflen;
    if(grainlen > dz->lparray[GR_ARRAYLEN][storeno]) {
        if((dz->extrabuf[grainbufno] = (float *)realloc(dz->extrabuf[grainbufno],grainlen * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to enlarge grain store.\n");
            return(MEMORY_ERROR);
        }
        dz->lparray[GR_ARRAYLEN][storeno] = grainlen;
    }
    dz->lparray[GR_THIS_LEN][storeno] = grainlen;
    if((exit_status = copy_up_halfsplice_to_grainstore(abs_halfsplice,grainbufno,grainlen,&storepos,dz))<0)
        return(exit_status);
    splicestart = ibufpos - abs_halfsplice;
    if(splicestart < 0) {
        splicestart += dz->buflen;
        splicestart_bufno = !bufno;
    }
    if((exit_status = store_up_halfsplice(1,splicestart,splicestart_bufno,chans,halfsplice,dz))<0)
        return(exit_status);
    splicestart -= abs_halfsplice;
    if(splicestart < 0) {
        splicestart += dz->buflen;
        splicestart_bufno = !bufno;
    }
    if((exit_status = store_dn_halfsplice(splicestart,splicestart_bufno,chans,halfsplice,dz))<0)
        return(exit_status);

    mid_grainlen = grainlen - dz->iparam[GR_ABS_SPLICELEN];
    if((exit_status = copy_midgrain_to_store(mid_grainlen,bufno,grainlen,grainstart,&storepos,crosbuf,grainbufno,dz))<0)
        return(exit_status);

    if((exit_status = copy_dn_halfsplice_to_grainstore(abs_halfsplice,grainbufno,grainlen,storepos,dz))<0)
        return(exit_status);
    return(FINISHED);
}

/************************* COPY_LAST_GRAIN_TO_BUF *************************/

int copy_last_grain_to_buf(int ibufpos,int bufno,int grainstart,int crosbuf,int chans,int storeno,dataptr dz)
{
    int  exit_status;
    int storepos = 0, mid_grainlen;
    int splicelen, abs_splicelen = dz->iparam[GR_ABS_SPLICELEN];
    int abs_halfsplice = abs_splicelen/2;
    int splicestart;
    int  splicestart_bufno = bufno;
    int  grainbufno = storeno + SPLBUF_OFFSET;
    int storelen;
    int grainlen = ibufpos - grainstart;
    if(crosbuf)
        grainlen += dz->buflen;
    if((storelen = grainlen + abs_halfsplice)<abs_splicelen)
        storelen = abs_splicelen;       /* grainstore must store up_hsplice & dn_hsplice even if grain to short */
    if(storelen > dz->lparray[GR_ARRAYLEN][storeno]) {
        if((dz->extrabuf[grainbufno] = (float *)realloc(dz->extrabuf[grainbufno],storelen * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to enlarge grain store.\n");
            return(MEMORY_ERROR);
        }
        dz->lparray[GR_ARRAYLEN][storeno] = storelen;
    }
    dz->lparray[GR_THIS_LEN][storeno] = storelen;
    if((exit_status = copy_up_halfsplice_to_grainstore(abs_halfsplice,grainbufno,storelen,&storepos,dz))<0)
        return(exit_status);

    if((mid_grainlen = grainlen - abs_halfsplice) < 0)
        mid_grainlen = 0;               /* HENCE mid_grainlen >= 0 */

    abs_splicelen = min(grainlen,abs_halfsplice);
    splicelen     = abs_splicelen/chans;
                                    /* HENCE abs_splicelen <= grainlen */
    splicestart = ibufpos - abs_splicelen;  /* splicestart >= grainstart, BECAUSE abs_splicelen <= grainlen */
    if(splicestart < 0) {
        splicestart += dz->buflen;
        splicestart_bufno = !bufno;
    }
    if((exit_status = store_dn_halfsplice(splicestart,splicestart_bufno,chans,splicelen,dz))<0)
        return(exit_status);            /* forces an abs_halfsplice length unit, regardless of input splicelen */ 
    if(mid_grainlen > 0) {
        if((exit_status = 
        copy_midgrain_to_store(mid_grainlen,bufno,storelen,grainstart,&storepos,crosbuf,grainbufno,dz))<0)
            return(exit_status);
    }
    if((exit_status = copy_dn_halfsplice_to_grainstore(abs_halfsplice,grainbufno,storelen,storepos,dz))<0)
        return(exit_status);
    return(FINISHED);
}

/************************ RETIME_PRE_FIRSTGRAIN_MATERIAL ***************************/

int retime_pre_firstgrain_material
(int new_grainstart,int *obufposition,int chans,int crosbuf,dataptr dz)
{
    int exit_status;
    /*float *b    = dz->sampbuf[bufno];*/     /* RWD: not used */
    float *obuf = dz->sampbuf[2];
    double *splicetab = dz->parray[GR_SPLICETAB];
    int excess, n, k;
    int m;
    int obufpos = *obufposition;
    int  abs_splicelen = dz->iparam[GR_ABS_SPLICELEN]/2; 
    int  splicelen = abs_splicelen/chans;
    if(crosbuf) {
        sprintf(errstr,"1st grain starts beyond end of sound buffer: Can't proceed.\n");
        return(DATA_ERROR);
    }
    excess = new_grainstart - obufpos;
    if(excess > 0) {
        memmove((char *)dz->extrabuf[2],(char *)obuf,(size_t)dz->buflen * sizeof(float));
        if(excess >= dz->buflen) {
            memset((char *)obuf,0,(size_t)dz->buflen * sizeof(float));
            while(excess >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                excess -= dz->buflen;
            }
        }
        for(n=0;n<excess;n++)
            obuf[n] = 0;
        for(m=0;m < obufpos;m++) {
            obuf[n++] = dz->extrabuf[2][m];
            if(n >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                n = 0;
            }
        }
        new_grainstart = n;
    } else if(excess < 0) {
        for(n=0, m = excess; n < new_grainstart; n++, m++)
            obuf[n] = obuf[m];      /* Move data backwards */
        while(n < obufpos)
            obuf[n++] = 0;          /* rezero samples beyond new_grainstart */
        k = 0;
        for(n=0;n<splicelen;n++) {  /* Put splice on start of file */
            for(m=0;m<chans;m++) {
                obuf[k] = (float) /*round*/ ((double)obuf[k] * splicetab[n]);
                k++;
            }
            if(k >= dz->buflen) {
                sprintf(errstr,"Buffer accounting problem: retime_pre_firstgrain_material()\n");
                return(PROGRAM_ERROR);
            }
        }
    }
    *obufposition = new_grainstart;     /* reset obuf pointer to (new) END of pre-grain material */
    return(FINISHED);
}

/************************* OUTPUT_GRAIN_LINK *************************/

int output_grain_link(int thisgap,int abs_halfsplice,int *obufposition,dataptr dz)
{
    int exit_status;
    int n, m;
    float *obuf = dz->sampbuf[2];
    int obufpos = *obufposition;
    for(n = 0, m = -(thisgap - abs_halfsplice); n < thisgap; n++,m++) {
        if(n < abs_halfsplice)
            obuf[obufpos] = dz->extrabuf[0][n];
        else
            obuf[obufpos] = 0;
        if(++obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;
        }
    }
    *obufposition = obufpos;
    return(FINISHED);
}           

/************************* COPY_START_OF_GRAIN *************************/

int copy_start_of_grain
(int grainstart,int start_splice,int bufno,int splicestart_bufno,int *obufpos,int crosbuf,dataptr dz)
{
    int exit_status;
    if(crosbuf) {
        if(splicestart_bufno != bufno) {
            if((exit_status = copygrain(grainstart,start_splice,!bufno,obufpos,dz))<0)
                return(exit_status);
        } else {
            if((exit_status = crosbuf_grain_type3(grainstart,start_splice,bufno,obufpos,dz))<0)
                return(exit_status);
        }
    } else {
        if((exit_status = copygrain(grainstart,start_splice,bufno,obufpos,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/************************** PREPARE_FINAL_GRAIN ******************************/

int prepare_final_grain(int ibufpos,int *start_splice,int bufno,int *splicestart_bufno,
int grainstart,int *grainlen,int crosbuf,int chans,dataptr dz)
{
    int exit_status;
    int abs_splicelen  = dz->iparam[GR_ABS_SPLICELEN];
    int abs_halfsplice = abs_splicelen/2;     /* guaranteed to fall on stereo boundary */
    int halfsplice     = abs_halfsplice/chans;
    *grainlen = ibufpos - grainstart;
    if(crosbuf)
        *grainlen += dz->buflen;
    if(*grainlen < abs_splicelen) {
        fprintf(stdout,"INFO: Final grain omitted: too short\n");
        fflush(stdout);
        return(FINISHED);
    }
    *start_splice = ibufpos - abs_halfsplice;
    *splicestart_bufno = bufno;
    if(*start_splice < 0) {
        *splicestart_bufno = !bufno;
        *start_splice += dz->buflen;
    }
    if((exit_status = store_dn_halfsplice(*start_splice,*splicestart_bufno,chans,halfsplice,dz))<0)
        return(exit_status);
    return(CONTINUE);
}

/************************** OUTPUT_FINAL_GRAIN ******************************/

//TW REVISED
int output_final_grain(int grainstart,int start_splice,int bufno,int splicestart_bufno,int is_first_grain,
int *obufpos,int grainlen,int crosbuf,dataptr dz)
{
    int exit_status;
    int  abs_splicelen  = dz->iparam[GR_ABS_SPLICELEN];
    int  abs_halfsplice = abs_splicelen/2;    /* guaranteed to fall on stereo boundary */
    if(!is_first_grain) {
        if((exit_status = read_up_halfsplice(obufpos,abs_halfsplice,dz))<0)
            return(exit_status);
    }
    if(grainlen > 0) {
        if((exit_status = copy_start_of_grain(grainstart,start_splice,bufno,splicestart_bufno,obufpos,crosbuf,dz))<0)
            return(exit_status);
    }
    if((exit_status = read_dn_halfsplice(obufpos,abs_halfsplice,dz))<0)
        return(exit_status);
    return(FINISHED);
}

/**************************** READ_DN_HALFSPLICE ****************************/

int read_dn_halfsplice(int *obufposition,int len,dataptr dz)
{
    int exit_status;
    int n, j = 0;
    float *b    = dz->extrabuf[0];
    float *obuf = dz->sampbuf[2];
    int obufpos = *obufposition;
    for(n=0;n < len;n++) {
        obuf[obufpos++] = b[j++];
        if(obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;        
        }
    }
    *obufposition = obufpos;
    return(FINISHED);
}

/**************************** READ_UP_HALFSPLICE ****************************/

int read_up_halfsplice(int *obufposition,int len,dataptr dz)
{
    int exit_status;
    int n, j = 0;
    float *b    = dz->extrabuf[1];
    float *obuf = dz->sampbuf[2];
    int obufpos = *obufposition;
    for(n=0;n < len;n++) {
        obuf[obufpos++] = b[j++];
        if(obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;
        }
    }
    *obufposition = obufpos;
    return(FINISHED);
}

/**************************** COPY_UP_HALFSPLICE_TO_GRAINSTORE ****************************/

int copy_up_halfsplice_to_grainstore(int len,int storeno,int storelen,int *storepos,dataptr dz)
{
    char *goaladdress = (char *)dz->extrabuf[storeno];
    if(len > storelen)  {
        sprintf(errstr,"Buffer anomaly: copy_up_halfsplice_to_grainstore()\n");
        return(PROGRAM_ERROR);
    }
    memmove(goaladdress,(char *)dz->extrabuf[1],len * sizeof(float));
    *storepos = len;
    return(FINISHED);
}

/**************************** COPY_DN_HALFSPLICE_TO_GRAINSTORE ****************************/

int copy_dn_halfsplice_to_grainstore(int len,int storeno,int storelen,int storepos,dataptr dz)
{
    char *goaladdress = (char *)(dz->extrabuf[storeno] + storepos);
    if(storepos + len > storelen)  {
        sprintf(errstr,"Buffer anomaly: copy_dn_halfsplice_to_grainstore()\n");
        return(PROGRAM_ERROR);
    }
    memmove(goaladdress,(char *)dz->extrabuf[0],len * sizeof(float));
    return(FINISHED);
}

/**************************** CREATE_AN_UPSPLICE_FROM_PREGRAIN_MATERIAL ****************************/

int create_an_upsplice_from_pregrain_material(int obufpos,int chans,dataptr dz)
{
    int n, k;
    int m, j = 0, empty_space;
    double ratio;
    float *obuf      = dz->sampbuf[2];
    float *splicebuf = dz->extrabuf[1];
    int abs_halfsplice = dz->iparam[GR_ABS_SPLICELEN]/2;
    int halfsplice = abs_halfsplice/chans;
    int splicelen;
    double *splicetab = dz->parray[GR_SPLICETAB];
    if(obufpos >= abs_halfsplice) {
        k = obufpos - abs_halfsplice;
        for(n=0;n<halfsplice;n++) {
            for(m=0;m<chans;m++)
                splicebuf[j++] = (float) /*round*/ ((double)obuf[k++] * splicetab[n]);
            if(k >= dz->buflen) {
                sprintf(errstr,"Error in buffer accounting: create_an_upsplice_from_pregrain_material()\n");
                return(PROGRAM_ERROR);
            }
        }
    } else {
        splicelen = obufpos/chans;
        empty_space = abs_halfsplice - obufpos;
        for(n=0;n<empty_space;n++)
            splicebuf[j++] = 0;
        k = 0;
        for(n=0;n<splicelen;n++) {
            ratio = (double)n/(double)splicelen;
            for(m=0;m<chans;m++)
                splicebuf[j++] = (float) /*round*/ ((double)obuf[k++] * ratio);
            if(k >= dz->buflen) {
                sprintf(errstr,"Error in buffer accounting: create_an_upsplice_from_pregrain_material()\n");
                return(PROGRAM_ERROR);
            }
        }
    }
    return(FINISHED);
}

/************************* RETIME_GRAIN *************************/

int retime_grain(int ibufpos,int thisgap,int grainstart,int origgap,int bufno,
int *obufpos,int chans,int crosbuf,int *grainadjusted,int store_end,int is_first_grain,dataptr dz)
{
    int  exit_status;
    int grainend, start_splice, gapchange;
    int  abs_splicelen  = dz->iparam[GR_ABS_SPLICELEN];
    int  abs_halfsplice = abs_splicelen/2;    /* guaranteed to fall on stereo boundary */
    int  halfsplice = dz->iparam[GR_SPLICELEN]/2;
    int  splicestart_bufno = bufno;

    start_splice = ibufpos - abs_halfsplice;
    if(start_splice < 0) {
        splicestart_bufno = !bufno;
        start_splice += dz->buflen;
    }
    if(!is_first_grain) {
        if((exit_status = read_up_halfsplice(obufpos,abs_halfsplice,dz))<0)
            return(exit_status);
    }
    if(store_end) {
        if((exit_status = store_up_halfsplice(1,start_splice,splicestart_bufno,chans,halfsplice,dz))<0)
            return(exit_status);
    }
    if((gapchange = thisgap - origgap)>=0) {
        start_splice -= abs_halfsplice;
        if(start_splice < 0) {
            splicestart_bufno = !bufno;
            start_splice += dz->buflen;
        }
        if((exit_status = store_dn_halfsplice(start_splice,splicestart_bufno,chans,halfsplice,dz))<0)
            return(exit_status);
        if((exit_status = copy_start_of_grain(grainstart,start_splice,bufno,splicestart_bufno,obufpos,crosbuf,dz))<0)
            return(exit_status);
        if((exit_status = output_grain_link(abs_halfsplice+gapchange,abs_halfsplice,obufpos,dz))<0)
            return(exit_status);
    } else {
        if(thisgap < dz->iparam[GR_ABS_SPLICEX2]) {
            grainend = grainstart + dz->iparam[GR_ABS_SPLICEX2];
            (*grainadjusted)++;
        } else 
            grainend = grainstart + thisgap;
        start_splice = grainend - abs_splicelen; /* NB Ensures 1 grain stops before other starts */
        splicestart_bufno = bufno;
        if(crosbuf) {
            if(start_splice >= dz->buflen)
                start_splice -= dz->buflen;
            else
                splicestart_bufno = !bufno;
        }
        if((exit_status = store_dn_halfsplice(start_splice,splicestart_bufno,chans,halfsplice,dz))<0)
            return(exit_status);
        if((exit_status = copy_start_of_grain(grainstart,start_splice,bufno,splicestart_bufno,obufpos,crosbuf,dz))<0)
            return(exit_status);
        if((exit_status = output_grain_link(abs_halfsplice,abs_halfsplice,obufpos,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/**************************** STORE_UP_HALFSPLICE ***************************
 *
 * GIVE IT bufno where splice STARTS!!!
 */

int store_up_halfsplice(int storeno,int start,int bufno,int chans,int splicelen,dataptr dz)
{
    int   n, remain;
    int    m;
    float  *b    = dz->sampbuf[bufno];
    float  *obuf = dz->extrabuf[storeno];
    int   k = start, j = 0;
    double *splicetab = dz->parray[GR_SPLICETAB];
    int  true_halfsplice = dz->iparam[GR_SPLICELEN]/2;
    if((remain = true_halfsplice - splicelen)<0) {
        sprintf(errstr,"Invalid splicelen in store_up_halfsplice()\n");
        return(PROGRAM_ERROR);
    } else if(remain==0) {
        for(n=0;n<splicelen;n++) {
            for(m=0;m<chans;m++)
                obuf[j++] = (float) /*round*/ ((double)b[k++] * splicetab[n]);
            if(k >= dz->buflen) {
                b = dz->sampbuf[!bufno];
                k = 0;
            }
        }
    } else {
        for(n=0;n<remain;n++) {
            for(m=0;m<chans;m++)
                obuf[j++] = 0.0;
        }
        for(n=0;n<splicelen;n++) {
            for(m=0;m<chans;m++)
                obuf[j++] = (float) /*round*/ ((double)(b[k++] * n)/(double)splicelen);
            if(k >= dz->buflen) {
                b = dz->sampbuf[!bufno];
                k = 0;
            }
        }
    }
    return(FINISHED);
}

/**************************** STORE_DN_HALFSPLICE ***************************
 *
 * GIVE IT bufno where splice STARTS!!!
 */

int store_dn_halfsplice(int start,int bufno,int chans,int splicelen,dataptr dz)
{
    int   n, k = start, j = 0;
    int    m;
    float  *b    = dz->sampbuf[bufno];
    float  *obuf = dz->extrabuf[0];
    double *splicetab = dz->parray[GR_SPLICETAB];
    int    true_halfsplice = dz->iparam[GR_SPLICELEN]/2;
    int    remain = true_halfsplice - splicelen;
    if(remain < 0) {
        sprintf(errstr,"Invalid splicelen: store_dn_halfsplice()\n");
        return(PROGRAM_ERROR);
    } else if(remain==0) {
        for(n=splicelen-1;n>=0;n--) {
            for(m=0;m<chans;m++)
                obuf[j++] = (float) /*round*/ ((double)b[k++] * splicetab[n]);
            if(k >= dz->buflen) {
                b = dz->sampbuf[!bufno];
                k = 0;
            }
        }
    } else {
        for(n=splicelen-1;n>=0;n--) {
            for(m=0;m<chans;m++)
                obuf[j++] = (float) /*round*/ ((double)(b[k++] * n)/(double)splicelen);
            if(k >= dz->buflen) {
                b = dz->sampbuf[!bufno];
                k = 0;
            }
        }
        for(n=0;n<remain;n++) {
            for(m=0;m<chans;m++)
                obuf[j++] = 0.0;
        }
    }
    return(FINISHED);
}

/**************************** COPY_MIDGRAIN_TO_STORE ***************************/

int copy_midgrain_to_store
(int mid_grainlen,int bufno,int storelen,int grainstart,int *storepos,int crosbuf,int storeno,dataptr dz)
{
    float *b, *b2;
    int /* j = *storepos,*/ partbuf, partbuf_samps;
    if(mid_grainlen  + *storepos > storelen) {
        sprintf(errstr,"Buffer anomaly: copy_midgrain_to_store()\n");
        return(PROGRAM_ERROR);
    }
    if(crosbuf) {
        if(grainstart + mid_grainlen <= dz->buflen) {
            b2 = dz->sampbuf[!bufno] + grainstart;
            b  = dz->extrabuf[storeno] + *storepos;
            memmove((char *)b,(char *)b2,mid_grainlen * sizeof(float));
            *storepos += mid_grainlen;
        } else {
            partbuf = dz->buflen - grainstart;
            partbuf_samps = partbuf;
            b2 = dz->sampbuf[!bufno] + grainstart;
            b  = dz->extrabuf[storeno] + *storepos;
            memmove((char *)b,(char *)b2,partbuf_samps * sizeof(float));
            *storepos += partbuf;
            partbuf = mid_grainlen - partbuf;
            partbuf_samps = partbuf;
            b2 = dz->sampbuf[bufno];
            b  = dz->extrabuf[storeno] + *storepos;
            memmove((char *)b,(char *)b2, partbuf_samps * sizeof(float));
            *storepos += partbuf;
        }
    } else {
        b2 = dz->sampbuf[bufno] + grainstart;
        b  = dz->extrabuf[storeno] + *storepos;
        memmove((char *)b,(char *)b2,mid_grainlen * sizeof(float));
        *storepos += mid_grainlen;
    }
    return(FINISHED);
}

/**************************** STORE_UPSPLICE_OF_NEXT_GRAIN ***************************/

int store_upsplice_of_next_grain(int ibufpos,int bufno,int abs_halfsplice,int chans,dataptr dz)
{
    int  splicestart_bufno = bufno;
    int halfsplice = abs_halfsplice/chans;
    int splicestart = ibufpos - abs_halfsplice;
    if(splicestart < 0) {
        splicestart += dz->buflen;
        splicestart_bufno = !bufno;
    }
    return store_up_halfsplice(1,splicestart,splicestart_bufno,chans,halfsplice,dz);
}


/**************************** PUT_GRAIN_INTO_STORE ***************************/

int put_grain_into_store
(int is_last_grain,int ibufpos,int bufno,int grainstart,int crosbuf,int chans,int graincnt,dataptr dz)
{
    if(is_last_grain)
        return copy_last_grain_to_buf(ibufpos,bufno,grainstart,crosbuf,chans,graincnt,dz);
    return copy_grain_to_buf(ibufpos,bufno,grainstart,crosbuf,chans,graincnt,dz);
}

/**************************** SAVE_ABS_SAMPLETIME_OF_GRAIN ***************************/

int save_abs_sampletime_of_grain(int grainstart,int *graincnt,int crosbuf,dataptr dz)
{
    int samps_read_before_thisbuf = dz->total_samps_read - dz->ssampsread;
    dz->lparray[GR_ABS_POS][*graincnt] = grainstart + samps_read_before_thisbuf;
    if(crosbuf)
        dz->lparray[GR_ABS_POS][*graincnt] -= dz->buflen;
    (*graincnt)++;
    if(*graincnt >= dz->iparam[GR_ARRAYSIZE]) {
        dz->iparam[GR_ARRAYSIZE] += BIGARRAY;
        if((dz->lparray[GR_ABS_POS] =
        (int *)realloc(dz->lparray[GR_ABS_POS],dz->iparam[GR_ARRAYSIZE] * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to enlarge positions store.\n");
            return(MEMORY_ERROR);
        }
    }
    return(FINISHED);
}

/**************************** DO_THE_REVERSING_PROCESS ***************************/

int do_the_reversing_process(int graincnt,int *obufposition,int chans,dataptr dz)
{
    int  exit_status;
    int obufpos = 0, seeksamps = 0, n, seekbufs = 0;
    int  abs_splicelen = dz->iparam[GR_ABS_SPLICELEN];
    int  abs_halfsplice = abs_splicelen/2;
    int  halfsplice = abs_halfsplice/chans;
    int *grainpos;

    if((exit_status = insert_EOF_sampletime_in_samptime_list(graincnt,dz))<0)
        return(exit_status);
    grainpos = dz->lparray[GR_ABS_POS];
    for(n=0;n<graincnt;n++)                             /* adjust all grain-times to include start of splice */
        grainpos[n] -= abs_halfsplice;  
    adjust_for_last_grain(&graincnt,grainpos,dz);
    if((exit_status = clear_outbuf(dz))<0)
        return(exit_status);
//TW UPDATE
/* AUGUST 2002 : go back to start of source and read first buffer (to reset dz->ssampsread) */
    if(sndseekEx(dz->ifd[0],0,0) < 0) { 
        sprintf(errstr,"seek error at start of do_the_reversing_process()\n");
        return(SYSTEM_ERROR);
    }
    if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
        return(exit_status);
    
    if((exit_status = do_seek_and_read(&seekbufs,&seeksamps,graincnt-1,dz))<0)
        return(exit_status);                            /* seek to a buffer containing (WHOLE) final grain */
    for(n=graincnt-1;n>=0;n--) {
        if((exit_status = output_whole_grain
        (n,&obufpos,&seeksamps,&seekbufs,halfsplice,abs_splicelen,chans,dz))<0)
            return(exit_status);
    }
    *obufposition = obufpos;
    return(FINISHED);
}

/**************************** DO_SEEK_AND_READ ***************************/

int do_seek_and_read(int *seekbufs,int *seeksamps,int grainno,dataptr dz)
{
    int exit_status;
    int new_seekbufs = dz->lparray[GR_ABS_POS][grainno]/dz->buflen;
    if(new_seekbufs != *seekbufs) {
        *seekbufs = new_seekbufs;
        if((sndseekEx(dz->ifd[0],(*seekbufs) * dz->buflen,0))<0) {
            sprintf(errstr,"seek error in do_seek_and_read()\n");
            return(SYSTEM_ERROR);
        }
        if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
            return(exit_status);
        *seeksamps = *seekbufs * dz->buflen;
    }
    return(FINISHED);
}

/**************************** OUTPUT_UP_HALFSPLICE ****************************/

int output_up_halfsplice(int *ibufpos,int *obufpos,int chans,int splicelen,int *crosbuf,dataptr dz)
{
    int    exit_status;
    int   n, remain;
    int    m;
    float  *b    = dz->sampbuf[0];
    float  *obuf = dz->sampbuf[2];
    int   i = *ibufpos;
    int   j = *obufpos;
    double *splicetab = dz->parray[GR_SPLICETAB];
    int  true_halfsplice = dz->iparam[GR_SPLICELEN]/2;
    if((remain = true_halfsplice - splicelen)<0) {
        sprintf(errstr,"Invalid splicelen in output_up_halfsplice()\n");
        return(PROGRAM_ERROR);
    } else if(remain==0) {
        for(n=0;n<splicelen;n++) {
            for(m=0;m<chans;m++)
                obuf[j++] = (float) /*round*/((double)b[i++] * splicetab[n]);
            if((exit_status = test_buffer_overflows(obuf,&j,&i,n,splicelen-1,crosbuf,dz))<0)
                return(exit_status);
        }
    } else {
        for(n=0;n<remain;n++) {
            for(m=0;m<chans;m++)
                obuf[j++] = 0;
            if(j >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                j = 0;
            }
        }
        for(n=0;n<splicelen;n++) {
            for(m=0;m<chans;m++)
                obuf[j++] = (float) /*round*/ ((double)(b[i++] * n)/(double)splicelen);
            if((exit_status = test_buffer_overflows(obuf,&j,&i,n,splicelen-1,crosbuf,dz))<0)
                return(exit_status);
        }
    }
    *ibufpos = i;
    *obufpos = j;
    return(FINISHED);
}

/**************************** OUTPUT_DN_HALFSPLICE ****************************/

int output_dn_halfsplice(int *ibufpos,int *obufpos,int chans,int splicelen,int *crosbuf,dataptr dz)
{
    int    exit_status;
    int   n, k, remain;
    int    m;
    float  *b    = dz->sampbuf[0];
    float  *obuf = dz->sampbuf[2];
    int   i = *ibufpos;
    int   j = *obufpos;
    double *splicetab = dz->parray[GR_SPLICETAB];
    int  true_halfsplice = dz->iparam[GR_SPLICELEN]/2;
    if((remain = true_halfsplice - splicelen)<0) {
        sprintf(errstr,"Invalid splicelen in output_dn_halfsplice()\n");
        return(PROGRAM_ERROR);
    } else if(remain==0) {
        for(n=splicelen-1,k=0;n>=0;n--,k++) {
            for(m=0;m<chans;m++)
                obuf[j++] = (float) /*round */((double)b[i++] * splicetab[n]);
            if((exit_status = test_buffer_overflows(obuf,&j,&i,k,splicelen-1,crosbuf,dz))<0)
                return(exit_status);
        }
    } else {
        for(n=splicelen-1,k=0;n>=0;n--,k++) {
            for(m=0;m<chans;m++)
                obuf[j++] = (float) /*round*/ ((double)(b[i++] * n)/(double)splicelen);
            if((exit_status = test_buffer_overflows(obuf,&j,&i,k,splicelen-1,crosbuf,dz))<0)
                return(exit_status);
        }
        for(n=0;n<remain;n++) {
            for(m=0;m<chans;m++)
                obuf[j++] = 0;
            if(j >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                j = 0;      
            }
        }
    }
    *ibufpos = i;
    *obufpos = j;
    return(FINISHED);
}

/**************************** DO_IN_SITU_HALFSPLICE_ON_STORED_GRAIN ****************************/

int do_in_situ_halfsplice_on_stored_grain(int bufpos,int chans,int splicelen,int grainstorelen,dataptr dz)
{
    int   n, k, remain;
    int    m;
    float  *obuf = dz->extrabuf[2];
    double *splicetab = dz->parray[GR_SPLICETAB];
    int    true_halfsplice = dz->iparam[GR_SPLICELEN]/2;
    int    abs_halfsplice = true_halfsplice * chans;
    if(bufpos + abs_halfsplice > grainstorelen) {
        sprintf(errstr,"Grainstore buffer overflow: do_in_situ_halfsplice_on_stored_grain()\n");
        return(PROGRAM_ERROR);
    }
    if((remain = true_halfsplice - splicelen)<0) {
        sprintf(errstr,"Invalid splicelen in do_in_situ_halfsplice_on_stored_grain()\n");
        return(PROGRAM_ERROR);
    } else if(remain==0) {
        for(n=splicelen-1,k=0;n>=0;n--,k++) {
            for(m=0;m<chans;m++) {
                obuf[bufpos] = (float) /*round*/((double)obuf[bufpos] * splicetab[n]);
                bufpos++;
            }
        }
    } else {
        for(n=splicelen-1,k=0;n>=0;n--,k++) {
            for(m=0;m<chans;m++) {
                obuf[bufpos] = (float) /*round*/ ((double)(obuf[bufpos] * n)/(double)splicelen);
                bufpos++;
            }
        }
        for(n=0;n<remain;n++) {
            for(m=0;m<chans;m++)
                obuf[bufpos++] = 0.0;
        }
    }
    return(FINISHED);
}

/**************************** TEST_BUFFER_OVERFLOWS ****************************/

int test_buffer_overflows(float *obuf,int *obufpos,int *ibufpos,int n,int maxlen,int *crosbuf,dataptr dz)
{
    int exit_status;
    if(*obufpos >= dz->buflen) {
        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
            return(exit_status);
        *obufpos = 0;
    }
    if(*ibufpos >= dz->ssampsread && n < maxlen) {
        if(*crosbuf) {
            sprintf(errstr,"double crosbuf: test_buffer_overflows()\n");
            return(PROGRAM_ERROR);
        }
        if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
            return(exit_status);
        *crosbuf = TRUE;
        *ibufpos = 0;
    }
    return(FINISHED);
}

/**************************** COPY_MIDGRAIN_TO_OUTPUT ***************************/

int copy_midgrain_to_output(int mid_grainlen,int *ibufpos,int *obufpos,int *crosbuf,int chans,dataptr dz)
{
    int   exit_status;
    int  n;
    int   m;
    float *b = dz->sampbuf[0], *obuf = dz->sampbuf[2];
    int  i = *ibufpos;
    int  j = *obufpos;
    mid_grainlen /= chans;
    for(n=0; n<mid_grainlen;n++) {
        for(m=0;m<chans;m++)
            obuf[j++] = b[i++];
        if((exit_status = test_buffer_overflows(obuf,&j,&i,n,mid_grainlen-1,crosbuf,dz))<0)
            return(exit_status);
    }
    *obufpos = j;
    *ibufpos = i;
    return(FINISHED);
}

/**************************** INSERT_EOF_SAMPLETIME_IN_SAMPTIME_LIST ***************************/

int insert_EOF_sampletime_in_samptime_list(int graincnt,dataptr dz)
{
    if(graincnt+1 > dz->iparam[GR_ARRAYSIZE]) {
        if((dz->lparray[GR_ABS_POS] = 
        (int *)realloc(dz->lparray[GR_ABS_POS],(graincnt+1) * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to enlarge positions store.\n");
            return(MEMORY_ERROR);
        }
    }
    dz->lparray[GR_ABS_POS][graincnt] = dz->insams[0];
    return(FINISHED);
}

/**************************** ADJUST_FOR_LAST_GRAIN ***************************/

void adjust_for_last_grain(int *graincnt,int *grainpos,dataptr dz)
{
    int lastgap;
    if(dz->vflag[LOSE_LAST_GRAIN])                      /* get rid of last grain if flagged */
        (*graincnt)--;
    else if((lastgap = grainpos[*graincnt] - grainpos[(*graincnt)-1]) < dz->iparam[GR_ABS_SPLICELEN]) {
        fprintf(stdout,"WARNING: Loosing last grain: too short.\n");
        fflush(stdout);
        (*graincnt)--;
    } else if(lastgap > dz->buflen) {                   /* truncate last grain if too int */
        fprintf(stdout,"WARNING: Last grain truncated: too int.\n");
        fflush(stdout);
        grainpos[*graincnt] = grainpos[(*graincnt)-1] + dz->buflen;
    }
}

/**************************** CLEAR_OUTBUF ***************************/

int clear_outbuf(dataptr dz)
{
    memset((char *)dz->sampbuf[2],0,dz->buflen * sizeof(float));
    return(FINISHED);
}

/**************************** OUTPUT_WHOLE_GRAIN ***************************/

int output_whole_grain(int n,int *obufpos,int *seeksamps,int *seekbufs,
int halfsplice,int abs_splicelen,int chans,dataptr dz)
{
    int  exit_status;
    int  crosbuf = FALSE;
    int ibufpos;
    int mid_grainlen;
    int startsplice;
    int endsplice;
    startsplice = endsplice = halfsplice;
    if((ibufpos = dz->lparray[GR_ABS_POS][n] - *seeksamps)<0) { /* IF current grain NOT in current buffer */
        if(*seeksamps==0) {                                     /* IF at start of file */
            if((exit_status = adjust_firstgrain_upsplice(&ibufpos,&startsplice,n,chans))<0)
                return(exit_status);
        } else {                                                /* ELSE: change current buffer */
            if((exit_status = do_seek_and_read(seekbufs,seeksamps,n,dz))<0)
                return(exit_status);
            ibufpos = dz->lparray[GR_ABS_POS][n] - *seeksamps;
        }
    }
    if((exit_status = output_up_halfsplice(&ibufpos,obufpos,chans,startsplice,&crosbuf,dz))<0)
        return(exit_status);
    mid_grainlen = (dz->lparray[GR_ABS_POS][n+1] - dz->lparray[GR_ABS_POS][n]) - abs_splicelen;
    if(crosbuf) {
        (*seekbufs)++;
        (*seeksamps) += dz->buflen;
    }
    crosbuf = FALSE;
    if(mid_grainlen > 0) {
        if((exit_status = copy_midgrain_to_output(mid_grainlen,&ibufpos,obufpos,&crosbuf,chans,dz))<0)
            return(exit_status);
        if(crosbuf) {
            (*seekbufs)++;
            (*seeksamps) += dz->buflen;
        }
    }
    crosbuf = FALSE;
    if((exit_status = output_dn_halfsplice(&ibufpos,obufpos,chans,endsplice,&crosbuf,dz))<0)
        return(exit_status);
    if(crosbuf) {
        (*seekbufs)++;
        (*seeksamps) += dz->buflen;
    }
    return(FINISHED);
}

/**************************** ADJUST_FIRSTGRAIN_UPSPLICE ***************************/

int adjust_firstgrain_upsplice(int *ibufpos,int *startsplice,int n,int chans)
{
    if(n!=0) {                      /* MUST be 1st grain */
        sprintf(errstr,"Grain accounting error: adjust_firstgrain_upsplice()\n");
        return(PROGRAM_ERROR);
    }
    *startsplice += *ibufpos/chans; /* shorten startsplice by deficit of [stereo] samples */
    *ibufpos = 0;                   /* reset output read to start of file */
    return(FINISHED);
}

/************************* DO_GRAIN_REPITCHING *************************/
/* RWD 4:2002 : NB modified to allow n-channels */
int do_grain_repitching(int *actual_grainlen,int bufno,int grainstart,int new_grainlen,int orig_grainlen,
int chans,int crosbuf,double pichratio,dataptr dz)
{
    int exit_status;
    int m;
    int k = 0, here, thishere;
    double dpoint = 0.0, frac;
//TW CHANGED
//  float /*thisval[2],*/ nextval[2], diff[2];
    float /*thisval[2],*/ nextval, diff;
    float *b, *b2, *grainbuf = dz->extrabuf[2];
    float *thisval;

    thisval = (float *) malloc(chans * sizeof(float));
    if(thisval==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to enlarge positions store.\n");
        return MEMORY_ERROR;
    }
    if(crosbuf)
        b = dz->sampbuf[!bufno];
    else
        b = dz->sampbuf[bufno];
    orig_grainlen /= chans;
    while(k < new_grainlen) {
        here = (int)dpoint; /* truncate */
        frac = dpoint - (double)here;
        thishere = (here*chans);
        thishere += grainstart;
        if((exit_status = test_for_bufcros(&thishere,&b,bufno,crosbuf,dz))<0) {
            free(thisval);
            return(exit_status);
        }
        if(exit_status == TRUE)     /* changed buffer */
            grainstart -= dz->buflen;
        for(m=0;m<chans;m++)
            thisval[m] = b[thishere+m];
        b2 = b;
        here++;
        thishere = (here*chans);
        thishere += grainstart;
        if((exit_status = test_for_bufcros(&thishere,&b2,bufno,crosbuf,dz))<0) {
            free(thisval);
            return(exit_status);
        }
        for(m=0;m<chans;m++) {
//TW MODIFIED
            nextval = b2[thishere+m];
            diff    = nextval - thisval[m];
            grainbuf[k++] = (float) /*round*/(((double)diff * frac) + thisval[m]);
        }
        if((dpoint += pichratio) > orig_grainlen)
            break; 
    }
    *actual_grainlen = k;
    free(thisval);
    return(FINISHED);
}

/************************* TEST_FOR_BUFCROS *************************/

int test_for_bufcros(int *thishere,float **b,int bufno,int crosbuf,dataptr dz)
{
    if(*thishere >= dz->buflen) {
        if(crosbuf==FALSE) {
            sprintf(errstr,"Buffer accounting problem: test_for_bufcros()\n");
            return(PROGRAM_ERROR);
        }
        *b = dz->sampbuf[bufno];
        *thishere -= dz->buflen;
        return(TRUE);
    }
    return(FALSE);
}

/************************* SAVE_NEXTGRAIN_UPSPLICE_ELSEWHERE *************************/

int save_nextgrain_upsplice_elsewhere
(int ibufpos,int bufno,int abs_halfsplice,int halfsplice,int chans,dataptr dz)
{
    int splicestart_bufno = bufno;
    int start_splice = ibufpos - abs_halfsplice;
    if(start_splice < 0) {
        splicestart_bufno = !bufno;
        start_splice += dz->buflen;
    }
    return store_up_halfsplice(0,start_splice,splicestart_bufno,chans,halfsplice,dz);
}

/************************* COPYGRAIN_FROM_GRAINBUF_TO_OUTBUF *************************/

int copygrain_from_grainbuf_to_outbuf(int *obufpos,int grainlen,dataptr dz)
{
    int exit_status;
    int remain  = dz->buflen - *obufpos;
    float *there = dz->sampbuf[2] + *obufpos;
    float *here  = dz->extrabuf[2];
    while(grainlen >= remain) {
        memmove((char *)there,(char *)here,remain * sizeof(float));
        if((exit_status = write_samps(dz->sampbuf[2],dz->buflen,dz))<0)
            return(exit_status);
        *obufpos = 0;
        there  = dz->sampbuf[2];
        here  += remain;
        grainlen -= remain;
        remain = dz->buflen;
    }
    if(grainlen) {
        memmove((char *)there,(char *)here,grainlen * sizeof(float));
        *obufpos += grainlen;
    }
    return(FINISHED);
}

/************************* RETRIEVE_UPSPLICE_FOR_NEXTGRAIN *************************/

int retrieve_upsplice_for_nextgrain(int abs_halfsplice,dataptr dz)
{
    memmove((char *)dz->extrabuf[1],(char *)dz->extrabuf[0],abs_halfsplice * sizeof(float));
    return(FINISHED);
}

/************************* RETIME_MAIN_BODY_OF_GRAIN *************************/

//TW REVISED
int retime_main_body_of_grain(int grainstart,int bufno,double pichratio,
double timeratio,int chans,int crosbuf,int *grainadjusted,
int orig_grainlen,int *new_grainlen,dataptr dz)
{
    int    exit_status;
    int    abs_splicelen  = dz->iparam[GR_ABS_SPLICELEN];
    int    abs_halfsplice = abs_splicelen/2;      /* guaranteed to fall on stereo boundary */
    int    halfsplice = dz->iparam[GR_SPLICELEN]/2;

    *new_grainlen = round((double)(orig_grainlen/chans) * timeratio) * chans;
    if(*new_grainlen < dz->iparam[GR_ABS_SPLICEX2]) {
        *new_grainlen = dz->iparam[GR_ABS_SPLICEX2];
        (*grainadjusted)++;
    }
    *new_grainlen -= abs_halfsplice;
    orig_grainlen -= abs_halfsplice;
    if((exit_status = repitch_main_body_of_grain(bufno,grainstart,*new_grainlen,orig_grainlen,
    pichratio,chans,crosbuf,halfsplice,abs_halfsplice,dz))<0)
        return(exit_status);
    return(FINISHED);
}

/************************* REPITCH_MAIN_BODY_OF_GRAIN *************************/

int repitch_main_body_of_grain(int bufno,int grainstart,int new_grainlen,int orig_grainlen,
double pichratio,int chans,int crosbuf,int halfsplice,int abs_halfsplice,dataptr dz)
{
    int exit_status;
    int  actual_grainlen;
    int   splicelen;
    int  splicestart;
    float *grainbuf;
    if(new_grainlen > dz->iparam[GR_STORESIZE]) {
        if((dz->extrabuf[2] = (float *)realloc(dz->extrabuf[2],new_grainlen * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to enlarge grain store.\n");
            return(MEMORY_ERROR);
        }
        dz->iparam[GR_STORESIZE] = (int)new_grainlen;
    }
    grainbuf = dz->extrabuf[2];
    memset((char *)grainbuf,0,new_grainlen * sizeof(float));
    if((exit_status = do_grain_repitching
    (&actual_grainlen,bufno,grainstart,new_grainlen,orig_grainlen,chans,crosbuf,pichratio,dz))<0)
        return(exit_status);
    splicelen   = halfsplice;
    splicestart = actual_grainlen - abs_halfsplice;
    if(splicestart < 0) {
        splicelen += splicestart/chans;
        splicestart = 0;
    }
    return do_in_situ_halfsplice_on_stored_grain(splicestart,chans,splicelen,new_grainlen,dz);
}

/************************* CREATE_REPITCHED_AND_RETIMED_GRAIN *************************/

//TW REVISED
int create_repitched_and_retimed_grain
(int bufno,int grainstart,int orig_grainlen,int *new_grainlen,
double pichratio,double timeratio,int *obufpos,int *is_first_grain,
int *grainadjusted,int abs_halfsplice,int chans,int crosbuf,dataptr dz)
{
    int exit_status;
    if((exit_status = retime_main_body_of_grain(grainstart,bufno,pichratio,timeratio,
    chans,crosbuf,grainadjusted,orig_grainlen,new_grainlen,dz))<0)
        return(exit_status);
    if(!(*is_first_grain)) {
        if((exit_status = read_up_halfsplice(obufpos,abs_halfsplice,dz))<0)
            return(exit_status);
    }
    *is_first_grain = FALSE;
    return copygrain_from_grainbuf_to_outbuf(obufpos,*new_grainlen,dz);
}

/************************* SAVE_ORIGGRAIN_LENGTH_AND_STORE_NEXTGRAINS_UPSPLICE *************************/

int save_origgrain_length_and_store_nextgrains_upsplice(int *orig_grainlen,int ibufpos,int bufno,
int grainstart,int abs_halfsplice,int halfsplice,int crosbuf,int chans,int is_last_grain,dataptr dz)
{
    *orig_grainlen = ibufpos - grainstart;
    if(crosbuf)
        *orig_grainlen += dz->buflen;
    if((*orig_grainlen - abs_halfsplice)<0) {
        if(!is_last_grain) {
            sprintf(errstr,"Anomalous too short grain: save_origgrain_length_and_store_nextgrains_upsplice()\n");
            return(PROGRAM_ERROR);
        } else {
            fprintf(stdout,"WARNING: Last grain omitted: too short.\n");
            return(CONTINUE);
        }   
    }
    return save_nextgrain_upsplice_elsewhere(ibufpos,bufno,abs_halfsplice,halfsplice,chans,dz);
}

/************************* REPITCHING_PROCESS *************************/

int repitching_process(int ibufpos,int bufno,int grainstart,int abs_halfsplice,int halfsplice,int *graincnt,
int *obufpos,int *orig_grainlen,int *new_grainlen,int *grainadjusted,
int *is_first_grain,int is_last_grain,int chans,int crosbuf,dataptr dz)
{
    int exit_status;
    double timeratio = 1.0, pichratio;
    int n;
    if(*is_first_grain && (exit_status = create_an_upsplice_from_pregrain_material(*obufpos,chans,dz))<0)
        return(exit_status);
    if((exit_status = save_origgrain_length_and_store_nextgrains_upsplice
    (orig_grainlen,ibufpos,bufno,grainstart,abs_halfsplice,halfsplice,crosbuf,chans,is_last_grain,dz))<0)
        return(exit_status);
    if(exit_status==CONTINUE)   /* last grain too short */
        return(FINISHED);
    switch(dz->mode) {
    case(GR_REPEATS):
        for(n=0;n<dz->iparam[GR_RATIOCNT];n++) {
            pichratio  = dz->parray[GR_RATIO][n];
            if((exit_status = create_repitched_and_retimed_grain
            (bufno,grainstart,*orig_grainlen,new_grainlen,pichratio,timeratio,obufpos,
            is_first_grain,grainadjusted,abs_halfsplice,chans,crosbuf,dz))<0)
                return(exit_status);
        }
        break;
    case(GR_NO_REPEATS):
        pichratio  = dz->parray[GR_RATIO][*graincnt];
        if((exit_status = create_repitched_and_retimed_grain
        (bufno,grainstart,*orig_grainlen,new_grainlen,pichratio,timeratio,obufpos,
        is_first_grain,grainadjusted,abs_halfsplice,chans,crosbuf,dz))<0)
            return(exit_status);
        if(++(*graincnt)>=dz->iparam[GR_RATIOCNT])
            *graincnt = 0;
        break;
    default:
        sprintf(errstr,"Unknown case: repitching_process()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/************************* REPITCHING_AND_RETIMING_PROCESS *************************/

int repitching_and_retiming_process(int ibufpos,int bufno,int grainstart,int abs_halfsplice,
int halfsplice,int *graincnt,int *obufpos,int *orig_grainlen,int *new_grainlen,int *grainadjusted,
int *is_first_grain,int is_last_grain,int chans,int crosbuf,dataptr dz)
{
    int exit_status;
    double timeratio, pichratio;
    int n;
    if(*is_first_grain && (exit_status = create_an_upsplice_from_pregrain_material(*obufpos,chans,dz))<0)
        return(exit_status);
    if((exit_status = save_origgrain_length_and_store_nextgrains_upsplice
    (orig_grainlen,ibufpos,bufno,grainstart,abs_halfsplice,halfsplice,crosbuf,chans,is_last_grain,dz))<0)
        return(exit_status);
    if(exit_status==CONTINUE)   /* last grain too short */
        return(FINISHED);
    switch(dz->mode) {
    case(GR_REPEATS):
        for(n=0;n<dz->iparam[GR_RATIOCNT];n+=2) {
            pichratio  = dz->parray[GR_RATIO][n];
            timeratio  = dz->parray[GR_RATIO][n+1];
            if((exit_status = create_repitched_and_retimed_grain
            (bufno,grainstart,*orig_grainlen,new_grainlen,pichratio,timeratio,obufpos,
            is_first_grain,grainadjusted,abs_halfsplice,chans,crosbuf,dz))<0)
                return(exit_status);
        }
        break;
    case(GR_NO_REPEATS):
        pichratio  = dz->parray[GR_RATIO][(*graincnt)++];
        timeratio  = dz->parray[GR_RATIO][(*graincnt)++];
        if((exit_status = create_repitched_and_retimed_grain
        (bufno,grainstart,*orig_grainlen,new_grainlen,pichratio,timeratio,obufpos,
        is_first_grain,grainadjusted,abs_halfsplice,chans,crosbuf,dz))<0)
            return(exit_status);
        if(*graincnt >= dz->iparam[GR_RATIOCNT])
            *graincnt = 0;
        break;
    default:
        sprintf(errstr,"Unknown case: repitching_and_retiming_process()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** TIMESTRETCH_ITERATIVE ****************************/

int timestretch_iterative(dataptr dz)
{
    int exit_status, do_slow = 0, do_regu;
    float *ibuf = dz->sampbuf[0];
    float *obuf = dz->sampbuf[1];
//  double *peak = dz->parray[0]; 
    int   *pos  = dz->lparray[0];
    int peakcnt, startsearch, endsearch, local_minima_cnt, minimum_element_len;
    int stretchable_len, required_segsection_len, total_segsection_length;
    int element_cnt, max_elements_needed;
    int arrsiz, fullperms, patternsize;
    int *pattern;
    int finished;
    int n, k=0, outpos, startseg, startpos, endpos, seglen;
    int abspos, new_abspos;
    int ascatter = 0, pscatter = 0, jitter = 0;
    double gain = 1.0, trans = 1.0, d, part, time, diff;
    float val, nextval;
    double z = (dz->param[RRR_END] - dz->param[RRR_START]) * dz->param[RRR_REPET];
    double starttime_of_iter, davg_step;
    int total_slolen, total_slo_incr, slo_incr, min_step, j, avg_step, gap, maxsamp = 0;
    int *seg_step = NULL, *seg_len = NULL;
    int thiselementcnt, regusegscnt, okcnt, *seg_ok = NULL;

    if(dz->vflag[0] == 0)
        z += dz->param[RRR_START];
    if(dz->vflag[1] == 0)
        z += dz->duration - dz->param[RRR_END];
//2010
    dz->tempsize = (int)round(z * dz->infile->srate) * dz->infile->channels;
    fprintf(stdout,"INFO: Generating output.\n");
    fflush(stdout);
    if(sloom)
        display_virtual_time(0,dz);
    if(dz->brksize[RRR_ASCAT] || !flteq(dz->param[RRR_ASCAT],0.0)) {
        ascatter = 1;
        jitter = 1;
    }
    if(dz->brksize[RRR_PSCAT] || !flteq(dz->param[RRR_PSCAT],0.0)) {
        pscatter = 1;
        jitter = 1;
    }
    if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
        return(exit_status);        
    dz->sampbuf[0][dz->insams[0]] = dz->sampbuf[0][dz->insams[0] - 1];  /* wrap around point for interpolation */
    startsearch = (int)round(dz->param[RRR_START] * dz->infile->srate);
    endsearch   = (int)round(dz->param[RRR_END] * dz->infile->srate);

    /* FIND ALL POSITIVE PEAKS : always look only at +ve vals, so all zero-crossings eventually found will be from +ve to -ve */

    peakcnt = 0;
    if((exit_status = find_all_positive_peaks(startsearch,endsearch,&peakcnt,dz)) < 0)
        return(exit_status);

    /* FIND ALL POSITIVE-PEAK MINIMA : overwriting the arrays peak-vals & peak-position with minima-vals & minima-positions */

    local_minima_cnt = 0;
    if((exit_status = find_all_local_minima(peakcnt,&local_minima_cnt,dz)) < 0)
        return(exit_status);

    /* ELIMINATE SPURIOUS MINIMA */

    if((exit_status = eliminate_spurious_minima(&local_minima_cnt,&minimum_element_len,dz)) < 0)
        return (exit_status);

    fprintf(stdout,"INFO: Original number of segments found = %d\n",local_minima_cnt - 1);
    fflush(stdout);

    /* CHECK MINIMA FOUND AGAINST INPUT ESTIMATE */

    if((local_minima_cnt - 1) >= 2 * dz->iparam[RRR_GET]) {
        if((exit_status = eliminate_excess_minima(&local_minima_cnt,pos,dz)) < 0)
            return (exit_status);
        fprintf(stdout,"INFO: Reduced to = %d\n",local_minima_cnt - 1);
        fflush(stdout);
    }
    /* SEARCH FOR ZERO CROSSINGS AFTER MINIMA */

    if((exit_status = locate_zero_crossings(local_minima_cnt,dz)) < 0)
        return (exit_status);

    /* CALCULATE HOW int STRETCHED SECTION SHOULD BE */

    stretchable_len = pos[local_minima_cnt-1] - pos[0];
    required_segsection_len = (int)round((double)stretchable_len * dz->param[RRR_STRETCH]);

    /* CALCULATE HOW MANY ELEMENTS TO USE */

    element_cnt = local_minima_cnt - 1;
    max_elements_needed = (int)round(ceil((double)required_segsection_len/(double)minimum_element_len));
    max_elements_needed *= 2;   /* required no of elements is set to a value greater than possibly required */

    /* CALCULATE HOW MANY PERMUTATIONS NEEDED */

    arrsiz = element_cnt * dz->iparam[RRR_REPET];   /* if elements can be repeated N  times, perm-array can contain N copies of values */
    fullperms = max_elements_needed / arrsiz;
    if(fullperms * arrsiz < max_elements_needed)    /* total number of permutations to generate must include any part permutation */
        fullperms++;
    patternsize = fullperms * arrsiz;

    if((pattern = (int *)malloc(patternsize * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory to generate segment permutation.\n");
        return(MEMORY_ERROR);
    }

    /* GENERATE RANDOM PATTERN */

    if((exit_status = rand_ints_with_restricted_repeats(element_cnt,max_elements_needed,arrsiz,fullperms,&pattern,dz)) < 0)
        return(exit_status);

    if(dz->brksize[RRR_SLOW]) {
        maxsamp = (int)round(dz->maxtime * (double)dz->infile->srate);
        if((seg_step = (int *)malloc(element_cnt * sizeof(int)))==NULL) {
            sprintf(errstr,"Insufficient memory to store steps between segment if slowed.\n");
            return(MEMORY_ERROR);
        }
        if(dz->brksize[RRR_REGU]) {
            if((seg_len = (int *)malloc(element_cnt * sizeof(int)))==NULL) {
                sprintf(errstr,"Insufficient memory to store orig segment lengths, if slowed.\n");
                return(MEMORY_ERROR);
            }
            if((seg_ok = (int *)malloc(element_cnt * sizeof(int)))==NULL) {
                sprintf(errstr,"Insufficient memory to store segment flags, if regularised.\n");
                return(MEMORY_ERROR);
            }
        }
    }

    /* COPY SOUND START TO OUTBUF */

    outpos = 0;
    if(dz->vflag[0] == 0) {
        memcpy((char *)obuf,(char *)ibuf,pos[0] * sizeof(float));
        outpos = pos[0];
    }
    /* GENERATE OUTPUT SEQUENCE OF ELEMENTS */
    total_segsection_length = 0;
    finished = 0;
    time = 0.0;
    abspos = dz->total_samps_written + outpos;
    if(dz->brksize[RRR_SLOW])
        maxsamp += abspos;
    starttime_of_iter = (double)abspos/(double)dz->infile->srate;

    do {
        for(n=0; n < patternsize; n++) {
            if(n % element_cnt == 0) {
                do_slow = 0;
                do_regu = 0;
                if(dz->brksize[RRR_SLOW]) {
                    time  = (double)abspos/(double)dz->infile->srate;
                    time -= starttime_of_iter;
                    if((exit_status = read_value_from_brktable(time,RRR_SLOW,dz))<0)
                        return(exit_status);
                    if(dz->brksize[RRR_REGU] > 0) {
                        if((exit_status = read_value_from_brktable(time,RRR_REGU,dz))<0)
                            return(exit_status);
                    }
                }
                if(dz->param[RRR_SLOW] > 1.0) {                                             //  If segments are to be slowed (by intervening silence)
                    do_slow = 1;
                    while(do_slow) {
                        thiselementcnt = min(element_cnt,patternsize-n);                    //  SAFETY (should be whole number of element_cnts in patternsize)
                        total_slolen = (int)round(stretchable_len * dz->param[RRR_SLOW]);  //  Sample duration of all segs, once slowed by intervening gaps
                        total_slo_incr = total_slolen - stretchable_len;                    //  Total added silence, in samples
                        slo_incr = (int)round((double)total_slo_incr/(double)thiselementcnt);  //  Silence to be inserted after to each element
                        if(slo_incr <= 0) {
                            do_slow = 0;
                            break;
                        }                                                                   //  Regularisation of rhythm of output cannot be done until there is
                        if(dz->param[RRR_REGU] > 0.0) {                                     //  maniupulable silence between segments, which only occues after SLOW applied
                            do_regu = 1;
                            while(do_regu) {
                                total_slo_incr = slo_incr * thiselementcnt;                 //  Total added silence, after accounting for rounding
                                min_step = dz->insams[0] + 1;
                                avg_step = 0;
                                for(j = 0,k=n; j < thiselementcnt;j++,k++){
                                    startseg = pattern[k];
                                    startpos = pos[startseg];
                                    endpos   = pos[startseg+1];
                                    seglen = endpos - startpos;
                                    seg_len[j]  = seglen;                                   //  Find actual length of each segment
                                    seg_step[j] = seg_len[j] + slo_incr;                    //  Find step between start of one seg & next (when intervening silence added) BEFORE REGULARISING 
                                    seg_ok[j]   = 1;                                        //  Mark as a valid segment to regularise
                                    min_step  = min(min_step,seg_step[j]);                  //  Find the minimim step
                                    avg_step += seg_step[j];                                //  Find the average step
                                }
                                regusegscnt = 0;
                                okcnt = thiselementcnt;
                                davg_step = (double)avg_step/(double)okcnt;                 //  Once regularised, step between seg entries will be set to the average value 
                                for(j=0; j < thiselementcnt; j++) {                         //  If length of a segment is > average, this can't be done
                                    if(seg_len[j] > davg_step) {                            //  So mark any such segment as not valid for the averaging process
                                        seg_ok[j] = 0;
                                        avg_step -= seg_len[j];                             //  and remove it from the calculation of the average step
                                        okcnt--;
                                        davg_step = (double)avg_step/(double)okcnt;
                                    } else
                                        regusegscnt++;                                      //  Count all the averagable segments
                                }   
                                if(regusegscnt < 2)                                         //  If less than 2 valid segs, no averaging can be done
                                    do_regu = 0;
                                break;
                            }
                            if(do_regu) {
                                avg_step = (int)round((double)avg_step/(double)regusegscnt);
                                for(j=0; j < thiselementcnt; j++) {                         //  For all valid segments
                                    if(seg_ok[j])                                           //  (partially) adjust step between it and next to the average step 
                                        seg_step[j] = ((int)round((avg_step - seg_step[j]) * dz->param[RRR_REGU])) + seg_step[j];
                                }
                            }
                        }
                        break;
                    }
                    if(do_slow) {
                        if(do_regu) {
                            for(j=0; j < thiselementcnt; j++)                               //  Find the gap left after each segment
                                seg_step[j] -= seg_len[j];
                        } else {                                                            //  If seg_entries NOT to be regularised
                            for(j=0; j < thiselementcnt; j++)                               //  Set silent gap between segs all to same val
                                seg_step[j] = slo_incr;
                        }
                    }
                } 
            }
            startseg = pattern[n];
            startpos = pos[startseg];
            endpos   = pos[startseg+1];
            seglen = endpos - startpos;
            if(jitter) {
                time = (double)abspos/(double)dz->infile->srate;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return(exit_status);
                k = startpos;
                d = (double)startpos;
                part = 0.0;
                if(ascatter) {          /* generate random value for seg gain, within given range */
                    gain = drand48() * dz->param[RRR_ASCAT];
                    gain = 1.0 - gain;
                }
                if(pscatter) {
                    trans = (drand48() * 2.0) - 1.0;
                    if(trans >= 0.0)    /* randomly chose up or down transposition */
                        trans = 1.0;
                    else
                        trans = -1.0;   /* generate random semitone step within given range */
                    trans *= (drand48() * dz->param[RRR_PSCAT]);
                    trans /= 12;        /* convert to transposition ratio = step in sample-reading */
                    trans  = pow(2.0,trans);
                }
                while(k < endpos) {
                    val     = ibuf[k];
                    nextval = ibuf[k+1];
                    diff    = nextval - val;
                    z = val + (diff * part);
                    z *= gain;
                    d   += trans;
                    k    = (int)d;             /* TRUNCATE */
                    part = d - (double)k; 
                    obuf[outpos] = (float)z;
                    if(++outpos >= dz->buflen) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        outpos = 0;
                    }
                }
            } else {
                for(k = startpos; k < endpos; k++) {
                    obuf[outpos] = ibuf[k];
                    if(++outpos >= dz->buflen) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        outpos = 0;
                    }
                }
            }
            new_abspos = dz->total_samps_written + outpos;
            if(do_slow) {
                k = n % element_cnt;
                gap = seg_step[k];
                for(j=0;j<gap;j++) {
                    obuf[outpos] = 0;
                    if(++outpos >= dz->buflen) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        outpos = 0;
                    }
                }
                new_abspos = dz->total_samps_written + outpos;
            }
            abspos = new_abspos;

            //  If reach end of slowing curve, but still patterns left, exit pattern generation

            if(dz->brksize[RRR_SLOW] && abspos >= maxsamp)
                finished = 1;
            
            //  If reach end of patterns

            if((total_segsection_length += seglen) >= required_segsection_len || (n == patternsize - 1)) {  //  Should be equivalent!!

                //  If not at end of slowing pattern, generate more random perms, and restart loop (n = 0)
                
                if(dz->brksize[RRR_SLOW] && abspos < maxsamp) {
                    if((exit_status = rand_ints_with_restricted_repeats(element_cnt,max_elements_needed,arrsiz,fullperms,&pattern,dz)) < 0)
                        return(exit_status);
                } else
                    finished = 1;
            }
            if(finished)
                break;
        }
    } while(!finished);
    if(!finished) {
        sprintf(errstr,"Insufficient segments generated at output stage.\n");
        return(PROGRAM_ERROR);
    }
    /* COPY SOUND END TO OUTBUF */

    if(dz->vflag[1] == 0) {
        for(n = pos[local_minima_cnt - 1]; n < dz->insams[0];n++) {
            obuf[outpos] = ibuf[n];
            if(++outpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                /*memset((char *)obuf,0,dz->bigbufsize);*/
                memset(obuf,0,sizeof(float)* dz->buflen);   /*RWD ?? */
                outpos = 0;
            }
        }
    }
    if(outpos > 0) {
        if((exit_status = write_samps(obuf,outpos,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/****************************** RAND_INTS_WITH_RESTRICTED_REPEATS ****************************/

int rand_ints_with_restricted_repeats(int element_cnt,int max_elements_needed,int arrsiz,int fullperms,int **pattern,dataptr dz)
{
    int n, m, i, k=0, j;
    int endcnt, endval, allowed, checkpart;
    int patterncnt = 0;
    int *arr, *arr2, *perm;

    if((arr = (int *)malloc(arrsiz * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory for permutation array 1.\n");
        return(MEMORY_ERROR);
    }
    if((perm = (int *)malloc(arrsiz * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory for permutation array 2.\n");
        return(MEMORY_ERROR);
    }
    if((arr2 = (int *)malloc(dz->iparam[RRR_REPET] * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory for permutation array 3.\n");
        return(MEMORY_ERROR);
    }
    n = 0;
    for(j=0;j<dz->iparam[RRR_REPET];j++) {      /* fill array with REPET copies of values. */
        for(i=0;i<element_cnt;i++)              /* this set can be permd AS A WHOLE, as repet adjacent copies of any val */
            arr[n++] = i;                       /* which might arise in perming this set, are allowed */
    }
    endcnt = 0;                                 /* number of items repeated at end of previous perm */
    endval = -1;                                /* value (possibly repeated) at end of previous perm (for first perm set it to a val not in perm */
                                                /* initially this is just the 'startval' fixed by the user */
    allowed = dz->iparam[RRR_REPET];            /* number of permissible repetitions of this val at start of 1st perm */
    checkpart = arrsiz - dz->iparam[RRR_REPET]; /* items at end of array to test for repetitions */
    n = 0;
    while(n < fullperms) {
        do_repet_restricted_perm(arr,perm,arrsiz,allowed,endval);
        j = 0;
        for(m = 0;m <arrsiz;m++) {
            (*pattern)[patterncnt] = arr[perm[m]];
            patterncnt++;
            if(m >= checkpart)              /* save last checkable stretch of perm */
                arr2[j++] = arr[perm[m]];
        }
        if(n < fullperms -1) {
            j--;
            endval = arr2[j--];                 /* note the val at end of perm */
            endcnt = 1;                         /* and count it */
            for(k = j; k>= 0; k--) {            
                if(arr2[k] == endval)           /* check adjacent vals, for repetition of value: count */
                    endcnt++;
                else                            /* if no more repetitions, finish counting */
                    break;
            }
            allowed = dz->iparam[RRR_REPET] - endcnt;           /* get number of permissible repets at start of next perm */
        }
        n++;
    }
    return FINISHED;
}

/****************************** DO_REPET_RESTRICTED_PERM ****************************/

void do_repet_restricted_perm(int *arr, int *perm, int arrsiz, int allowed, int endval)
{
    int n, t;
    int checklen = allowed + 1;
    int done = 0;
    while(!done) {
        for(n=0;n<arrsiz;n++) {
            t = (int)(drand48() * (double)(n+1)); /* TRUNCATE */
            if(t==n)
                hhprefix(n,arrsiz,perm);
            else
                hhinsert(n,t,arrsiz,perm);
        }
        if(checklen <= 0)
            break;
        for(n=0;n<checklen;n++) {
            if(arr[perm[n]] == endval) {    /* if this is val (repeated) at end of last perm */
                if(allowed == 0)            /* if repetition not allowed, force a new perm val */
                    break;
                else                        /* else, repetitions still allowed */
                    allowed--;              /* decrement number of permissible further repets */ 
            } else {
                done = 1;                   /* if this is not val at end of last perm */
                break;                      /* perm is OK */
            }
        }
    }
}

/***************************** HHINSERT **********************************
 *
 * Insert the value m AFTER the T-th element in perm[].
 */

void hhinsert(int m,int t,int setlen,int *perm)
{
    hhshuflup(t+1,setlen,perm);
    perm[t+1] = m;
}

/***************************** HHPREFIX ************************************
 *
 * Insert the value m at start of the permutation perm[].
 */

void hhprefix(int m,int setlen,int *perm)
{
    hhshuflup(0,setlen,perm);
    perm[0] = m;
}

/****************************** HHSHUFLUP ***********************************
 *
 * move set members in perm[] upwards, starting from element k.
 */

void hhshuflup(int k,int setlen,int *perm)
{
    int n, *i;
    int z = setlen - 1;
    i = (perm+z);
    for(n = z;n > k;n--) {
        *i = *(i-1);
        i--;
    }
}

/****************************** ELIMINATE_SPURIOUS_MINIMA ***********************************/

int eliminate_spurious_minima(int *local_minima_cnt,int *minimum_element_len,dataptr dz)
{
    int exit_status;
    int *seg;
    int *segpos, badseg;
    int n, m, segcnt = (*local_minima_cnt) - 1, temp,eliminate_pos;
    double *peak = dz->parray[0];
    int *pos = dz->lparray[0];
    int maxseg, minseg, minsegstartpos;
    if((seg = (int *)malloc(segcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory (A) to check for spurious local minima\n");
        return(MEMORY_ERROR);
    }
    if((segpos = (int *)malloc(segcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory (B) to check for spurious local minima\n");
        return(MEMORY_ERROR);
    }
    for(;;) {                               /* RECURSIVELY ELIMINATE TOO-SHORT GAPS */
        for(n = 0; n < segcnt; n++)
            seg[n] = pos[n+1] - pos[n];
        for(n = 0; n < segcnt; n++)
            segpos[n] = n;
        for(n=0;n < segcnt-1; n++) {        /* BUBBLE SORT THE GAPS BETWEEN LOCAL MINIMA TO DESCENDING SIZE ORDER */
            for(m=n+1;m < segcnt; m++) {
                if(seg[n] < seg[m]) {
                    temp = seg[n];
                    seg[n] = seg[m];
                    seg[m] = temp;
                    temp = segpos[n];       /* AND KEEP TRACK OF WHERE THESE NEWLY ORDERED GAPS ARE */
                    segpos[n] = segpos[m];
                    segpos[m] = temp;
                }
            }
        }
        maxseg = seg[0];
        minseg = seg[segcnt-1];
        badseg = 0;                 /* Compare minimum seg with maximum seg */
        if((double)minseg < (double)maxseg * dz->param[RRR_RANGE]) {
            badseg = 1;
        }
        if(!badseg) {               /* No more anomalous segs found, exit loop */
            *minimum_element_len = minseg;
            break;
        }
        if(segcnt-1 < 3) {
            sprintf(errstr,"Insufficient valid local minima found during elimination of spurious minima. Are the search times incorrect??\n");
            return(DATA_ERROR);
        }
        minsegstartpos = segpos[segcnt-1];  /* Find positions of minimum seg start */

        if((exit_status = which_minimum_to_eliminate(minsegstartpos,pos,maxseg,segcnt,&eliminate_pos,dz)) < 0)
            return(exit_status);
        for(n=eliminate_pos ; n < (*local_minima_cnt)-1;n++) {
            peak[n] = peak[n+1];            /* Eliminate unwanted val, by moving values in array above it down 1 position */
            pos[n]  = pos[n+1];             /* If BAD minimum is at current end of array, it just gets ignored as result of "local_minima_cnt--" */
        }
        (*local_minima_cnt)--;              /* Reduce count of minima, and count of segs between minima */
        segcnt--;
    }
    return(FINISHED);
}

/****************************** FIND_ALL_POSITIVE_PEAKS ***********************************/

int find_all_positive_peaks(int startsearch,int endsearch,int *peakcnt,dataptr dz)
{
    double *peak = dz->parray[0], thispeak;
    int *pos  = dz->lparray[0];
    float *ibuf = dz->sampbuf[0];
    int thissamp = startsearch, thispos;
    while(ibuf[thissamp] <= 0) {        /* skip values below zero */
        if(++thissamp >= endsearch)
            break;
    }
    if(thissamp >= endsearch) {
        sprintf(errstr,"Cannot locate any peaks in the signal. Are the search times incorrect??\n");
        return(DATA_ERROR);
    }
    thispeak = ibuf[thissamp];
    thispos = thissamp;
    thissamp++;
    while(thissamp < endsearch) {
        if(ibuf[thissamp] >= 0.0) {
            if(ibuf[thissamp] > thispeak) { /* search for (positive) peak val */
                thispeak = ibuf[thissamp];  
                thispos  = thissamp;
            }
        } else {
            peak[*peakcnt] = thispeak;      /* once signal becomes -ve3, store last found peak */
            pos[*peakcnt] = thispos;
            (*peakcnt)++;
            while(ibuf[thissamp] < 0) {     /* then skip over -ve part of signal */
                if(++thissamp >= endsearch)
                    break;
            }
            thispeak = ibuf[thissamp];      /* once dignal is +ve again, set up an initial value for peak */
            thispos  = thissamp;
        }
        thissamp++;
    }
    if(*peakcnt > 0) {                      /* check for peak found near end, before signal goes -ve once more */
        if((thispos != pos[(*peakcnt)-1]) && (thispeak > 0.0)) {
            peak[*peakcnt] = thispeak;
            pos[*peakcnt] = thispos;
            (*peakcnt)++;
        }
    }
    if(*peakcnt < 3) {
        sprintf(errstr,"Insufficient signal peaks found. Are the search times incorrect??\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/****************************** FIND_ALL_LOCAL_MINIMA ***********************************/

int find_all_local_minima(int peakcnt,int *local_minima_cnt,dataptr dz)
{
    int thispeak;
    double *peak = dz->parray[0];
    int *pos = dz->lparray[0];
/*  double peakmin = peak[0];*/
    int finished = 0;
    *local_minima_cnt = 0;
    thispeak = 1;
    while(thispeak < peakcnt) {
        while(peak[thispeak] <= peak[thispeak-1]) { /* while peaks are falling, look for local peak minimum */
            if(++thispeak >= peakcnt) {
                finished = 1;
                break;
            }
        }
        if(finished)
            break;
        peak[*local_minima_cnt] = peak[thispeak-1]; /* store value and position of local mimimum */
        pos[*local_minima_cnt]  = pos[thispeak-1];
        (*local_minima_cnt)++;
        while(peak[thispeak] >= peak[thispeak-1]) { /* skip over rising sequence of peaks */
            if(++thispeak >= peakcnt) {
                break;
            }
        }
    }
    if(*local_minima_cnt < 3) {
        sprintf(errstr,"Insufficient local minima found in inital search. Are the search times incorrect??\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/****************************** LOCATE_ZERO_CROSSINGS ***********************************/

int locate_zero_crossings(int local_minima_cnt,dataptr dz)
{
    int finished = 0;
    int n;
    float  *ibuf = dz->sampbuf[0];
    double *peak = dz->parray[0];
    int   *pos  = dz->lparray[0];
    for(n=0;n<local_minima_cnt;n++) {
        while (peak[n] >= 0.0) {                /* advance position from minimum +ve peak until value crosses zero */
            if(++pos[n] >= dz->insams[0]) {
                finished = 1;
                if(peak[n] > 0.0) {             /* if end of file does not go to zero, Warn */
                    fprintf(stdout,"WARNING: End_of_sound segment doesn't fall to zero level, & may cause clicks in output. (Dovetail end of sound?)\n");
                    fflush(stdout);
                }
                break;
            }
            peak[n] = ibuf[pos[n]];
        }
        if(finished)
            break;
    }
    return(FINISHED);
}

/****************************** WHICH_MINIMUM_TO_ELIMINATE ***********************************
 *
 * Too short segment can be eliminated by  deleting minimum at its start, or at its end.
 * Eliminating the minimum will make the preceding (or following) segment larger.
 * Deduce which of the two minima to delete.
 */

int which_minimum_to_eliminate(int minsegstartpos,int *pos,int maxseg,int segcnt,int *eliminate_pos,dataptr dz)
{
    int newpreseg, newpostseg, later_seg, prior_seg;
    double ratio, ratio2, presegratio=0, postsegratio=0;

    if(minsegstartpos == 0) {                       /* if minseg is at start of sequence */
        if(pos[2] - pos[0] > maxseg)                /* losing end min of seg makes next seg bigger: if bigger than maxseg, eliminate start min instead */
            *eliminate_pos = minsegstartpos;        /* effectively erasing the first seg */
        else                                        /* else lose end min, making following seg larger */
            *eliminate_pos = minsegstartpos + 1;
    } else if(minsegstartpos + 1 == segcnt) {       /* if minseg is at end of sequence */
        if(pos[minsegstartpos+1] - pos[minsegstartpos-1] > maxseg)
            *eliminate_pos = minsegstartpos + 1;    /* if losing start min of seg makes prior seg > maxseg, eliminate end min instead */
        else                                        /* else element start min, making previous seg larger */
            *eliminate_pos = minsegstartpos;
                                                    /* ELSE we're not dealing with segs at ends of sequence */
    } else {
        newpreseg  = (pos[minsegstartpos+1] - pos[minsegstartpos-1]);   /* Find length new seg created by eliminating start min of shortest seg */
        newpostseg = (pos[minsegstartpos+2] - pos[minsegstartpos]);     /* Find length new seg created by eliminating end min of shortest seg */
        if(newpreseg > maxseg || newpostseg > maxseg) { /* if either new seg is > maxseg */
            if(newpostseg <= maxseg)                /* If ONLY preseg inter than maxseg, choose to make postseg, elim end min */
                *eliminate_pos = minsegstartpos + 1;
            else if(newpreseg <= maxseg)            /* If ONLY postseg > maxseg, choose to make postseg, elim start min */
                *eliminate_pos = minsegstartpos;
                                                    /* else if BOTH > maxseg, choose smaller */
            else if(newpreseg > newpostseg)         /* if new preseg is larger, keep postseg, eliminate end min */
                *eliminate_pos = minsegstartpos + 1;
            else                                    /* if new preseg is smaller, keep preseg, eliminate start min */
                *eliminate_pos = minsegstartpos;
        } else {                /* both preseg and postseg are less than maxseg, choose seg that tallies best with local seg environment */
            if(segcnt <=3) {    /* with only 3 segs, any deletion affects (increases sizeof) maxseg */
                                /* Thus newly created seg, wherever it is, will be > sizeof orig maxseg, and will have been dealt with above */
                sprintf(errstr,"Programming error xxxx.\n");
                return(PROGRAM_ERROR);
            }
                                /* find the maximum size ratio between new newpreseg and its adjacent segs */
            later_seg = pos[minsegstartpos+2] - pos[minsegstartpos+1];
            ratio = (double)newpreseg/(double)later_seg;
            if(ratio < 1.0)
                ratio  = 1.0/ratio;
            if(minsegstartpos - 2 > 0) {
                prior_seg = pos[minsegstartpos-1] - pos[minsegstartpos-2];
                ratio2 = (double)newpreseg/(double)prior_seg;
                if(ratio2 < 1.0)
                    ratio2 = 1.0/ratio2;
                ratio = max(ratio,ratio2);
            }
            presegratio = ratio;
                    /* find the maximum size ratio between new newpostseg and its adjacent segs */
            ratio = 0.0;
            prior_seg = pos[minsegstartpos] - pos[minsegstartpos-1];
            ratio = (double)newpostseg/(double)prior_seg;
            if(ratio < 1.0)
                ratio  = 1.0/ratio;
            if(minsegstartpos + 3 < segcnt) {
                later_seg = pos[minsegstartpos+3] - pos[minsegstartpos+2];
                ratio2 = (double)newpostseg/(double)later_seg;
                if(ratio2 < 1.0)
                    ratio2 = 1.0/ratio2;
                ratio = max(ratio,ratio2);
            }
            postsegratio = ratio;
        }
        if(postsegratio < presegratio)  /* if newpostseg makes better sense, eliminate end min */
            *eliminate_pos = minsegstartpos + 1;
        else                            /* else newporeseg makes better sense, elminate start min */
            *eliminate_pos = minsegstartpos;
    }
    return FINISHED;
}

/****************************** ELIMINATE_EXCESS_MINIMA ***********************************
 *
 * If no of segments found is >> anticipated segments, Group segments together in 'best' arrangement.
 */

int eliminate_excess_minima(int *local_minima_cnt,int *pos,dataptr dz)
{
    int n, m, j, bestgroup = 0;                    /* if found-segs is exact multiple of anticipated-segs, bestgrouping starts at position 0 */
    int more_than_possible = dz->insams[0];        /* i.e. too large to be reached by calculation */
    int segcnt = (*local_minima_cnt) - 1;          /* Number of segs found e.g. 19 */
    int grouping_cnt = segcnt/dz->iparam[RRR_GET]; /* How many segs to join-as-a-group, to make no of segs tally with anticipated value */
                                                    /* if get=4  19/4 --> 4 by integer truncation */
    int remnant = (int)(segcnt - (grouping_cnt * dz->iparam[RRR_GET])); /* How many segments are then spare at start or end of sequence of segs */
                                                                        /* e.g. 19 - 16 = 3 */
    int diff, mindiff = more_than_possible;
    int grouped_len, max_grouped_len, min_grouped_len;
    for(n=0;n<remnant;n++) {                    /* for all possible sets of consecutive segments i.e. in example 0-15, 1-16, 2-17, 3-18 */
        grouped_len = 0;                        /* starting at 0,1,2,3 respectively */
        max_grouped_len = 0;
        min_grouped_len = more_than_possible;
        for(m=n;m < segcnt;m+=grouping_cnt) {
            for(j=0;j<grouping_cnt;j++)         /* combine consecutive segs in groups of 'grouping_cnt', summing lengths */
                grouped_len += pos[m+j];        /* find max and min lengths of new grouped-segments */
            max_grouped_len = max(max_grouped_len,grouped_len);
            min_grouped_len = min(min_grouped_len,grouped_len);
        }                                       /* Find range of the new lengths */
        diff = max_grouped_len - min_grouped_len; 
        if(diff < mindiff)  
            bestgroup = n;                      /* look for set of grouped-segments with lowest range */
    }
    for(n= 0,m=bestgroup;m < *local_minima_cnt;n++,m+=grouping_cnt)
        pos[n] = pos[m];                        /* group orig segs by overwriting intermediate min-positions */
    *local_minima_cnt = n;
    return(FINISHED);
}

/************************* TIMESTRETCH_ITERATIVE2 *******************************/

int timestretch_iterative2(dataptr dz)
{
    int exit_status;
/*  double maxenv = 10000.0;*/
    int n, m, k, rrr_cnt, rrr_start=0, peakwidth;
    int *trofpnt;
    int trofpntcnt = 0, lasttrofpntcnt = 0;
    float lastenval;
    int could_be_rrr_flap, gotrrr = 0;
    int envcnt;
    fprintf(stdout,"INFO: Searching file envelope.\n");
    fflush(stdout);
    rrr_cnt = -dz->iparam[RRR_SKIP];    /* Number of iterate units to skip before utilising any of them */
    if(((envcnt = dz->insams[0]/dz->iparam[RRR_SAMP_WSIZENU]) * dz->iparam[RRR_SAMP_WSIZENU])!=dz->insams[0])
        envcnt++;
    if((dz->env=(float *)malloc((envcnt+20) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
        return(MEMORY_ERROR);
    }
    if((exit_status = extract_rrr_env_from_sndfile(RRR_SAMP_WSIZENU,dz))<0)
        return(exit_status);
    if((trofpnt = (int *)malloc(envcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY TO ANALYSE ENVELOPE.\n");
        return(MEMORY_ERROR);
    }
    trofpnt[0] = 0;
    lastenval = dz->env[0];
    n = 1;
    while(n < envcnt) {         /* GET FIRST ENVELOPE TROUGH */
        if(dz->env[n] > lastenval) {
            trofpnt[0] = 0;
            lastenval = dz->env[n];
            n++;
            break;
        } else if (dz->env[n] < lastenval) {
            trofpnt[0] = n;
            lastenval = dz->env[n];
            n++;
            break;
        }
        lastenval = dz->env[n];
        n++;
    }
    if(n >= envcnt) {
        sprintf(errstr,"NO PEAKS FOUND IN ENVELOPE\n");
        return(GOAL_FAILED);
    }
    while(n < envcnt) {         /* GET ENVELOPE TROUGHS */
        if(dz->env[n] > lastenval) {
            trofpntcnt = lasttrofpntcnt + 1;
        } else if (dz->env[n] < lastenval) {
            trofpnt[trofpntcnt] = n;
            lasttrofpntcnt = trofpntcnt;
        }
        lastenval = dz->env[n];
        n++;
    }
    if(trofpntcnt < 2) {
        sprintf(errstr,"NO SIGNIFICANT PEAKS FOUND IN ENVELOPE\n");
        return(GOAL_FAILED);
    }
    for(m = 0, n=1;n<trofpntcnt;m++,n++) {
        peakwidth = trofpnt[n] - trofpnt[m];
        if(peakwidth > 2 && peakwidth < 6) {            /* IF PEAK WIDTH IS WITHIN LIMITS FOR AN ITERATE-UNIT */
            could_be_rrr_flap = 0;
            for(k = trofpnt[m]; k < trofpnt[n]; k++) {  /* AND PEAK IS ABOVE GATE */
                if(dz->env[k] > dz->param[RRR_GATE]) {
                    could_be_rrr_flap = 1;
                    break;
                }
            }
            if(could_be_rrr_flap) {
                if(rrr_cnt == 0)                        /* IF (skipped unwanted flaps &) NO FLAPS HERE YET */
                    rrr_start = m;                      /* MARK START OF A POSSIBLE FLAP */
                rrr_cnt++;                              /* COUNT FLAPS */
            }
        } else {
            could_be_rrr_flap = 0;                      /* MARK NON-FLAP */
        }
        if (!could_be_rrr_flap) {
            if(rrr_cnt >= dz->iparam[RRR_GET]) {        /* IF END OF FLAPS, AND WE HAVE ENOUGH ADJACENT FLAPS */
                                                        /*  SET PARAMS FOR ZER-CROSSING SEARCH */
                dz->iparam[RRR_START] = trofpnt[rrr_start] * dz->iparam[RRR_SAMP_WSIZENU];
                dz->iparam[RRR_START] -= dz->iparam[RRR_SAMP_WSIZENU]/ 2;   /* Start search before first flap segment */
                dz->iparam[RRR_START] = max(0,dz->iparam[RRR_START]);
                dz->iparam[RRR_END] = trofpnt[m] * dz->iparam[RRR_SAMP_WSIZENU];;
                dz->iparam[RRR_END] += dz->iparam[RRR_SAMP_WSIZENU]/ 2; /* End search after last flap segment */
                dz->iparam[RRR_END] = min(dz->insams[0],dz->iparam[RRR_END]);
                dz->param[RRR_START] = (double)dz->iparam[RRR_START]/dz->infile->srate;
                dz->param[RRR_END]   = (double)dz->iparam[RRR_END]/dz->infile->srate;
//              dz->iparam[RRR_GET] = rrr_cnt;
// EXPERIMENTAL!! FORCES zero-cross algo to get better result
// if algo finds 8 zcs and there are really rrr_cnt=4, as 8 >= 2 * 4 , it groups the zcs in pairs to give 4 zcs
// However, if algo finds 7 zcs and there are really rrr_cnt=4
// 7 < (2*4), so it doesn't group the zcs in twos, so 7 zcs are kept, probalby too many...
// With this mod 7 > 2*(rrr_cnt-1 = 3) so it groups the number of zcs in 2s (and throws 1 away) to give 3 zcs: better result
                dz->iparam[RRR_GET] = rrr_cnt - 1;
                gotrrr = 1;
                break;
            } else {
                rrr_cnt = -dz->iparam[RRR_SKIP];
            }
        }
    }
    if(gotrrr == 0) {
        sprintf(errstr,"NO ITERATIVE LOCATION FOUND\n");
        return(GOAL_FAILED);
    } else {
        fprintf(stdout,"INFO: searching between %.04lf and %.04lf secs: where %d peaks found\n",dz->param[RRR_START],dz->param[RRR_END],rrr_cnt);
        fflush(stdout);
    }
    return timestretch_iterative(dz);
}

/************************* EXTRACT_RRR_ENV_FROM_SNDFILE *******************************/

int extract_rrr_env_from_sndfile(int paramno,dataptr dz)
{
    int n;
    float *envptr;
    int bufcnt;
    if(((bufcnt = dz->insams[0]/dz->buflen)*dz->buflen)!=dz->insams[0])
        bufcnt++;
    envptr = dz->env;
    for(n = 0; n < bufcnt; n++) {
        if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Can't read samples from soundfile: extract_rrr_env_from_sndfile()\n");
            return(SYSTEM_ERROR);
        }
        if(sloom)
            display_virtual_time(dz->total_samps_read,dz);
        get_rrrenv_of_buffer(dz->ssampsread,dz->iparam[paramno],&envptr,dz->sampbuf[0]);
    }
    dz->envend = envptr;
    return(FINISHED);
}

/************************* GET_RRRENV_OF_BUFFER *******************************/

void get_rrrenv_of_buffer(int samps_to_process,int envwindow_sampsize,float **envptr,float *buffer)
{
    int  start_samp = 0;
    float *env = *envptr;
    while(samps_to_process >= envwindow_sampsize) {
        *env++       = getmaxsampr(start_samp,envwindow_sampsize,buffer);
        start_samp  += envwindow_sampsize;
        samps_to_process -= envwindow_sampsize;
    }
    if(samps_to_process)    /* Handle any final short buffer */
        *env++ = getmaxsampr(start_samp,samps_to_process,buffer);
    *envptr = env;
}

/*************************** GETMAXSAMPR ******************************/

float getmaxsampr(int startsamp, int sampcnt,float *buffer)
{
    int  i, endsamp = startsamp + sampcnt;
    float thisval, thismaxsamp = 0.0f;
    for(i = startsamp; i<endsamp; i++) {
        if((thisval = (float)fabs(buffer[i]))>thismaxsamp)
            thismaxsamp = thisval;
    }
    return(thismaxsamp);
}

/********************************** GRAB_NOISE_AND_EXPAND **********************************
 * 
 * Locate noise, then expand it by random-reads from zero-cross to zero-cross
 */

int grab_noise_and_expand(dataptr dz)
{
    int exit_status;
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
    int phase, initialphase, isnoise = -1, finished = 0;
    int j, k, n, m, waveset_cnt;
    int lastzcross = 0, sampstart, sampend, len, temp, orig_buflen;
    int brkpntcnt = 0, got_noise = 0;
    double maxsamp, gate = dz->param[SSS_GATE];
    int abovegate = 0;
    if((dz->lparray[0] = (int *)malloc(dz->insams[0] * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY.\n");
        return(DATA_ERROR);
    }
    fprintf(stdout,"INFO: Searching for noise.\n");
    fflush(stdout);
    orig_buflen = dz->buflen;
    dz->buflen = dz->insams[0];
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);
    else if(dz->ssampsread <= 0) {
        sprintf(errstr,"Failed to read sound from input file\n");
        return(DATA_ERROR);
    }
    dz->buflen = orig_buflen;

    /* ESTABLISH INITIAL PHASE OF SIGNAL */

    n = 0;
    while(ibuf[n]==0) {
        if(++n >= dz->ssampsread) {
            sprintf(errstr,"FAILED TO FIND ANY WAVECYCLES IN FILE.\n");
            return(DATA_ERROR);
        }
    }
    if(ibuf[n] > 0)
        initialphase = 1;
    else
        initialphase = -1;

    for(;;) {

        /* FIND A WAVECYCLE */
        maxsamp = 0.0;
        if(initialphase == 1) {
            while(ibuf[n] > 0) {
                maxsamp = max(maxsamp,ibuf[n]);
                if(++n >= dz->ssampsread) {
                    finished = 1;
                    break;
                }
            }
            while(ibuf[n] <= 0) {
                maxsamp = max(maxsamp,-ibuf[n]);
                if(++n >= dz->ssampsread) {
                    finished = 1;
                    break;
                }
            }
        } else {
            while(ibuf[n] < 0) {
                maxsamp = max(maxsamp,-ibuf[n]);
                if(++n >= dz->ssampsread) {
                    finished = 1;
                    break;
                }
            }
            while(ibuf[n] >= 0) {
                maxsamp = max(maxsamp,ibuf[n]);
                if(++n >= dz->ssampsread) {
                    finished = 1;
                    break;
                }
            }
        }
        if(finished)
            break;
        if(maxsamp < gate) 
            isnoise = 0;
        else  {
            abovegate = 1;
            dz->lparray[0][brkpntcnt] = n;
            /* MEASURE WAVE-CYCLE LENGTH, AND TEST FOR NOISE */
            /* IF SIGNAL SWITCHES FROM NOISE  to NOT-NOISE or vice versa, STORE THAT POSITION */
            if(dz->lparray[0][brkpntcnt] - lastzcross < dz->iparam[NOISE_MINFRQ]) {
                if(brkpntcnt == 0)      /* if noise-start pos, move to search for noise-end position */
                    brkpntcnt = 1;
                else if(dz->lparray[0][1] - dz->lparray[0][0] >= dz->iparam[MAX_NOISLEN]) {
                    got_noise = 1;
                    break;
                }
                isnoise = 1;
            } else {
                if(isnoise == 1) {      /* if at end of noise ... */
                        /* if enough noise present ... break */
                    if(dz->lparray[0][1] - dz->lparray[0][0] > dz->iparam[MIN_NOISLEN]) {
                        got_noise = 1;
                        break;
                    } else {                /* if NOT ENOUGH noise present, delete noise pos data, start again */
                        dz->lparray[0][0] = dz->lparray[0][1];
                        brkpntcnt = 0;
                    }
                }
                isnoise = 0;
            }
        }
        lastzcross = n; /* store position of last waveset end... */
    }

    /* CHECK THAT ANY NOISE : non-NOISE SWITCHES FOUND */

    if(!abovegate) {
        sprintf(errstr,"NO SIGNAL IS ABOVE THE GATE LEVEL\n");
        return(GOAL_FAILED);
    }
    if(!got_noise) {
        if(gate > 0)
            sprintf(errstr,"NO NOISE FOUND WITH GATE-LEVEL %lf\n",gate);
        else
            sprintf(errstr,"NO NOISE FOUND\n");
        return(GOAL_FAILED);
    }
    fprintf(stdout,"INFO: Generating output.\n");
    fflush(stdout);
    sampstart = dz->lparray[0][0];
    sampend   = dz->lparray[0][1];
    waveset_cnt = 0;
    if(ibuf[sampstart] > 0) {
        initialphase = 1;
        phase = 1;
        dz->lparray[0][waveset_cnt++] = sampstart;
    } else if (ibuf[sampstart] < 0) {
        initialphase  = -1;
        phase  = -1;
        dz->lparray[0][waveset_cnt++] = sampstart;
    } else
        phase = 0;
            /* STORE ZERO-CROSS-PAIR POSITIONS */
    for(n = sampstart+1;n < sampend; n++) {
        switch(phase) {
        case(0):
            if(ibuf[n] > 0) {
                phase = 1;
                initialphase = 1;
                dz->lparray[0][waveset_cnt++] = n;
            } else if(ibuf[n] < 0) {
                phase = -1;
                initialphase = -1;
                dz->lparray[0][waveset_cnt++] = n;
            }
            break;
        case(1):
            if(ibuf[n] < 0) {
                if(initialphase == -1)
                    dz->lparray[0][waveset_cnt++] = n;
                phase = -1;
            }
            break;
        case(-1):
            if(ibuf[n] > 0) {
                if(initialphase == 1)
                    dz->lparray[0][waveset_cnt++] = n;
                phase = 1;
            }
            break;
        }
    }
    j = 0;
    if(!dz->vflag[0]) {
        for(n=0;n<sampstart;n++) {
            obuf[j++] = ibuf[n];
            if(j >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                j = 0;
            }
        }
    }
    for(;;) {
        n = (int)floor(drand48() * waveset_cnt);    /* RANDOM READS TO AND FROM ZERO-CROSSINGS */
        do {
            m = (int)floor(drand48() * waveset_cnt);
        } while (m == n);
        if((len = dz->lparray[0][m] - dz->lparray[0][n]) < 0) {
            temp = m;
            m = n;
            n = temp;
            len  = -len;
        }
        if(j + len >= dz->iparam[SSS_DUR])      /* HALT AT A ZERO CROSSING */
            break;
        for(k= dz->lparray[0][n]; k < dz->lparray[0][m];k++) {
            obuf[j++] = ibuf[k];
            if(j >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                dz->iparam[SSS_DUR] -= dz->buflen;
                j = 0;
            }
        }
    }
    if(!dz->vflag[0]) {
        for(n=sampend;n<dz->insams[0];n++) {
            obuf[j++] = ibuf[n];
            if(j >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                j = 0;
            }
        }
    }
    if(j > 0) {
        if((exit_status = write_samps(obuf,j,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/************************** GREV **********************/

int grev(dataptr dz)
{
    int exit_status, finished, start_negative;
    int n, j=0, k, minpeakloc, envcnt, last_total_samps_read, startsearch, endsearch, obufpos;
    int lastobufpos, step, expansion, lastgrainlen=0, nu_gp_dur;
    double maxsamp0, maxsamp1, peakav, minpeakav, time;
    int firsttrof, up, gotmaxsamp0, crossed_zero_to_positive, crossed_zero_to_negative, gp=0, read_brk = 0;
    float *e, *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
    int *pa;
    double convertor = 1.0 / dz->infile->srate;

    if(((envcnt = dz->insams[0]/dz->iparam[GREV_SAMP_WSIZE]) * dz->iparam[GREV_SAMP_WSIZE])!=dz->insams[0])
        envcnt++;
    if((dz->env=(float *)malloc((envcnt + 12) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
        return(MEMORY_ERROR);
    }
    e = dz->env;
    if((pa =(int *)malloc((envcnt + 12) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for peak positions array.\n");
        return(MEMORY_ERROR);
    }
    if((exit_status = extract_rrr_env_from_sndfile(GREV_SAMP_WSIZE,dz))<0)  /* Get envel of whole sound */
        return(exit_status);
    dz->total_samps_read = 0;
    display_virtual_time(0,dz);
    envcnt = dz->envend - dz->env;
    n = 0;
    k = 0;
    pa[k++] = 0;
    while(flteq(e[n],e[0])) {
        n++;
        if(n >= envcnt) {
            sprintf(errstr,"NO PEAKS IN THE FILE\n");
            return(GOAL_FAILED);
        }
    }
    if(e[n] < e[0]) {
        firsttrof = 1;
        up = -1;
    } else {
        firsttrof = 0;
        up = 1;
    }

    /* KEEP ONLY THE PEAKS AND TROUGHS OF THE ENVELOPE, AND THEIR LOCATIONS */

    while (n <envcnt) { /* store peaks and troughs only */
        switch(up) {
        case(1):
            if(e[n] < e[n-1]) {
                dz->env[k] = dz->env[n-1]; 
                pa[k]  = (n-1) * dz->iparam[GREV_SAMP_WSIZE];
                k++;
                up = -1;
            }
            break;
        case(-1):
            if(e[n] > e[n-1]) {
                dz->env[k] = dz->env[n-1]; 
                pa[k]  = (n-1) * dz->iparam[GREV_SAMP_WSIZE];
                k++;
                up = 1;
            }
            break;
        }
        n++;
    }
    if((envcnt = k) <= 3) {
        sprintf(errstr,"INSUFFICIENT PEAKS IN THE FILE.\n");
        return(GOAL_FAILED);
    }

    /* KEEP ONLY THE (DEEP ENOUGH) TROUGHS OF THE ENVELOPE */

    switch(firsttrof) {
    case(0):        /* if trof at start */
        k = 1;      /* set item at 0 NOT to be overwritten (as it is first trof) (set k=1) */
        j = 1;      /* search for good trofs between peaks, from (j=)1 */
        break;
    case(1):        /* if trof not at start */
        k = 0;      /* set item at 0 to be overwritten by 1st trof found (k=0) */
        j = 0;      /* search for good trofs between peaks, from (j=)0 */
        break;
    }
    for(n=j;n<envcnt-2;n++) {
        peakav = dz->env[n] + dz->env[n+2];
        if(peakav * dz->param[GREV_TROFRAC] >= dz->env[n+1]) {  /* NB TROF_FAC alreday PRE-MULTIPLIED by 2.0 */
            pa[k]  = pa[n+1];
            k++;
        }
    }
    if((envcnt = k) <= 3) {
        sprintf(errstr,"INSUFFICIENT VALID TROUGHS IN THE FILE.\n");
        return(GOAL_FAILED);
    }

    /* SEARCH WAVEFORM FOR ZERO_CROSSING AT MORE ACCURATE TROUGH */

    fprintf(stdout,"INFO: Number of grains found = %d\n",envcnt);
    fflush(stdout);
    if((sndseekEx(dz->ifd[0],0,0))<0) {
        sprintf(errstr,"seek error 1\n");
        return(SYSTEM_ERROR);
    }
    last_total_samps_read = 0;
    k = (int)round((double)dz->iparam[GREV_SAMP_WSIZE] * 0.5); /* search around size of envel window */
    startsearch = max(pa[0] - k, 0);
    endsearch   = min(pa[0] + k,dz->insams[0]);
    dz->total_samps_read = 0;
    while(startsearch > dz->buflen) {
        dz->total_samps_read += dz->buflen;
        startsearch -= dz->buflen;
    }
    if(dz->total_samps_read > 0) {
        if((sndseekEx(dz->ifd[0],dz->total_samps_read,0))<0) {
            sprintf(errstr,"seek error 2\n");
            return(SYSTEM_ERROR);
        }
        last_total_samps_read = dz->total_samps_read;
        endsearch   -= last_total_samps_read;
    }
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);
    n = 0;
    finished = 0;
    while(n<envcnt) {
        maxsamp0 = 0.0;
        maxsamp1 = 0.0;
        gotmaxsamp0 = 0;
        minpeakav = HUGE_VAL;
        minpeakloc = -1;
        j = startsearch;
        crossed_zero_to_positive = 0;
        crossed_zero_to_negative = 0;
        if(ibuf[j] <= 0)
            start_negative = 1;
        else
            start_negative = 0;
        do {
            if(j >= dz->ssampsread) {
                last_total_samps_read = dz->total_samps_read;
                endsearch -= dz->buflen;
                j         -= dz->buflen;
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                if(dz->ssampsread == 0) {
                    finished = 1;
                    break;
                }
            }
            if(!crossed_zero_to_negative) {     /* before signal crosses to negative */
                if(start_negative) {
                    if(ibuf[j] <= 0.0) {
                        j++;
                        continue;
                    }
                    start_negative = 0;
                }
                if(!gotmaxsamp0) {              /* First time only, look for first maxsamp */
                    if(ibuf[j] > maxsamp0)      /* (after first time, it gets val passed back from 2nd maxsamp */
                        maxsamp0 = ibuf[j];
                } 
                if (ibuf[j] < 0.0) {            /* if not crossed zero to -ve, look for, and mark, zero-cross to -ve */
                    crossed_zero_to_negative = j + last_total_samps_read;
                    gotmaxsamp0 = 1;
                }
            } else if (ibuf[j] >= 0) {      /* if crossed zero to neg and we're now crossing back to +ve */
                crossed_zero_to_positive = 1;
                if(ibuf[j] > maxsamp1)      /* look for 2nd maxsamp */
                    maxsamp1 = ibuf[j];
            } else if (crossed_zero_to_positive) {  /* having crossed from -ve to +ve, we're now -ve again, in a new cycle */
                if((peakav = maxsamp0 + maxsamp1) < minpeakav) {
                    minpeakav = peakav;
                    minpeakloc = crossed_zero_to_negative;
                }
                maxsamp0 = maxsamp1;
                crossed_zero_to_positive = 0;
                crossed_zero_to_negative = 0;
            }
            j++;
        } while(j < endsearch || minpeakloc < 0);

        if(minpeakloc < 0) {    
            if (finished) {     /* deal with endcases where waveform fails to cross zero (twice) */
                if(crossed_zero_to_negative > 0)
                    pa[n++] = crossed_zero_to_negative;
                envcnt = n;
                break;
            } else {
                sprintf(errstr,"FAILED TO FIND ONE OF THE LOCAL MINIMA.\n");
                return(PROGRAM_ERROR);
            }
        }
        pa[n] = minpeakloc;
        n++;
        startsearch = max(pa[n] - k, 0);
        endsearch   = min(pa[n] + k,dz->insams[0]);
        if(startsearch >= dz->total_samps_read) {
            while(startsearch >= dz->total_samps_read) {
                last_total_samps_read = dz->total_samps_read;
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                if(last_total_samps_read >= dz->total_samps_read) {
                    envcnt = n;
                    break;
                }
            }
        }
        startsearch -= last_total_samps_read;
        endsearch   -= last_total_samps_read;
        while(startsearch < 0) {    /* very tiny windows may cause backtracking in file */
            last_total_samps_read -= dz->buflen;
            if((sndseekEx(dz->ifd[0],last_total_samps_read,0))<0) {
                sprintf(errstr,"seek error 3\n");
                return(SYSTEM_ERROR);
            }
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            dz->total_samps_read = last_total_samps_read + dz->ssampsread;
            startsearch += dz->buflen;
            endsearch += dz->buflen;
        }
    }
    if((sndseekEx(dz->ifd[0],0,0))<0) {
        sprintf(errstr,"seek error 4\n");
        return(SYSTEM_ERROR);
    }
    dz->total_samps_read = 0;
    last_total_samps_read = 1;  /* Value 1 forces first seek and read */
    obufpos = 0;
    switch(dz->mode) {
    case(GREV_REVERSE):
        if(!dz->brksize[GREV_GPCNT])
            gp = dz->iparam[GREV_GPCNT];
        for(n = envcnt - (2 * gp); n>0; n-=gp) {
            startsearch = pa[n];
            if(dz->brksize[GREV_GPCNT]) {
                time = (double)startsearch * convertor;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return(exit_status);
                gp = (int)round(dz->param[GREV_GPCNT]);
            }
            endsearch   = pa[n + gp];
            if((exit_status = do_envgrain_write(startsearch,endsearch,&last_total_samps_read,&obufpos,dz))<0)
                return(exit_status);
        }
        break;
    case(GREV_REPEAT):
        if(!dz->brksize[GREV_GPCNT])
            gp = dz->iparam[GREV_GPCNT];
        if(dz->brksize[GREV_REPETS] || dz->brksize[GREV_GPCNT])
            read_brk = 1;
        for(n = 0; n<=(envcnt-gp); n+=gp) {
            startsearch = pa[n];
            if(read_brk) {
                time = (double)startsearch * convertor;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return(exit_status);
            }
            if(dz->brksize[GREV_GPCNT])
                gp = (int)round(dz->param[GREV_GPCNT]);
            if(dz->brksize[GREV_REPETS])
                dz->iparam[GREV_REPETS] = (int)round(dz->param[GREV_REPETS]);
            endsearch   = pa[n + gp];
            for(k = 0;k < dz->iparam[GREV_REPETS];k++) {
                if((exit_status = do_envgrain_write(startsearch,endsearch,&last_total_samps_read,&obufpos,dz))<0)
                    return(exit_status);
            }
        }
        break;
    case(GREV_DELETE):
        if(!dz->brksize[GREV_GPCNT])
            gp = dz->iparam[GREV_GPCNT];
        if(dz->brksize[GREV_KEEP] || dz->brksize[GREV_GPCNT])
            read_brk = 1;
        for(n = 0; n<=(envcnt-gp); n+=gp) {
            startsearch = pa[n];
            if(read_brk) {
                time = (double)startsearch * convertor;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return(exit_status);
            }
            if(dz->brksize[GREV_GPCNT])
                gp = (int)round(dz->param[GREV_GPCNT]);
            if(dz->brksize[GREV_KEEP])
                dz->iparam[GREV_KEEP] = (int)round(dz->param[GREV_KEEP]);
            endsearch   = pa[n + gp];
            if((n % dz->iparam[GREV_OUTOF]) < dz->iparam[GREV_KEEP]) {
                if((exit_status = do_envgrain_write(startsearch,endsearch,&last_total_samps_read,&obufpos,dz))<0)
                    return(exit_status);
            }
        }
        break;
    case(GREV_OMIT):
        if(!dz->brksize[GREV_GPCNT])
            gp = dz->iparam[GREV_GPCNT];
        if(dz->brksize[GREV_KEEP] || dz->brksize[GREV_GPCNT])
            read_brk = 1;
        for(n = 0; n<=(envcnt-gp); n+=gp) {
            startsearch = pa[n];
            if(read_brk) {
                time = (double)startsearch * convertor;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return(exit_status);
            }
            if(dz->brksize[GREV_GPCNT])
                gp = (int)round(dz->param[GREV_GPCNT]);
            if(dz->brksize[GREV_KEEP])
                dz->iparam[GREV_KEEP] = (int)round(dz->param[GREV_KEEP]);
            endsearch   = pa[n + gp];
            if((n % dz->iparam[GREV_OUTOF]) < dz->iparam[GREV_KEEP]) {
                if((exit_status = do_envgrain_write(startsearch,endsearch,&last_total_samps_read,&obufpos,dz))<0)
                    return(exit_status);
            } else {
                if((exit_status = do_envgrain_zerowrite(startsearch,endsearch,&obufpos,dz))<0)
                    return(exit_status);
            }
        }
        break;
    case(GREV_TSTRETCH):
        if(!dz->brksize[GREV_GPCNT])
            gp = dz->iparam[GREV_GPCNT];
        if(dz->brksize[GREV_TSTR] || dz->brksize[GREV_GPCNT])
            read_brk = 1;
        for(n = 0; n<=envcnt-1; n++) {
            startsearch = pa[n];
            if(read_brk) {
                time = (double)startsearch * convertor;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return(exit_status);
            }
            if(dz->brksize[GREV_GPCNT])
                gp = (int)round(dz->param[GREV_GPCNT]);
            endsearch   = pa[n + 1];
            lastobufpos = obufpos + dz->total_samps_written;
            if((exit_status = do_envgrain_addwrite(startsearch,endsearch,&last_total_samps_read,&obufpos,dz))<0)
                return(exit_status);
            step = obufpos + dz->total_samps_written - lastobufpos;
            expansion = (int)round((double)step * dz->param[GREV_TSTR]) - step;
            if(expansion > 0) {
                if((exit_status = do_envgrain_zerowrite_dblbuf(0,expansion,&obufpos,dz))<0)
                    return(exit_status);
            } else 
                obufpos += expansion;
        }
        break;
    case(GREV_GET):
        if(!dz->brksize[GREV_GPCNT])
            gp = dz->iparam[GREV_GPCNT];
        else
            read_brk = 1;
        
        for(n = 0; n<=(envcnt-gp); n+=gp) {
            startsearch = pa[n];
            if(read_brk) {
                time = (double)startsearch * convertor;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return(exit_status);
            }
            if(dz->brksize[GREV_GPCNT])
                gp = (int)round(dz->param[GREV_GPCNT]);
            fprintf(dz->fp,"%d\n",startsearch);
        }
        break;
    case(GREV_PUT):
        if(!dz->brksize[GREV_GPCNT])
            gp = dz->iparam[GREV_GPCNT];
        else
            read_brk = 1;
        for(n = 0; n<=(envcnt-gp); n+=gp) {
            startsearch = pa[n];
            if(read_brk) {
                time = (double)startsearch * convertor;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return(exit_status);
            }
            if(dz->brksize[GREV_GPCNT])
                gp = (int)round(dz->param[GREV_GPCNT]);
            endsearch   = pa[n + gp];
            if(n > 0) {
                if(n >= dz->itemcnt)
                    break;
                nu_gp_dur = (int)round(dz->parray[GR_SYNCTIME][n] - dz->parray[GR_SYNCTIME][n-1]);
                obufpos += (nu_gp_dur - lastgrainlen);
                if(obufpos <= -dz->buflen) {
                    sprintf(errstr,"BACKTRACK TOO LARGE\n");
                    return(GOAL_FAILED);
                }
            }
            if((exit_status = do_envgrain_write(startsearch,endsearch,&last_total_samps_read,&obufpos,dz))<0)
                return(exit_status);
            lastgrainlen = endsearch - startsearch;
        }
        break;
    }
    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/************************** DO_ENVGRAIN_WRITE **********************/

int do_envgrain_write(int startsearch,int endsearch,int *last_total_samps_read,int *obufpos,dataptr dz)
{
    int exit_status;
    int step, n, m, limit = dz->buflen;
    float *ibuf = dz->sampbuf[0], *obuf  = dz->sampbuf[1];
    if(startsearch > dz->total_samps_read || startsearch < *last_total_samps_read) {
        step = (startsearch / dz->buflen) * dz->buflen; 
        if((sndseekEx(dz->ifd[0],step,0))<0) {
            sprintf(errstr,"seek error 5\n");
            return(SYSTEM_ERROR);
        }
        *last_total_samps_read = step;
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        dz->total_samps_read = *last_total_samps_read + dz->ssampsread; 
    }
    startsearch -= *last_total_samps_read;
    endsearch   -= *last_total_samps_read;
    m = *obufpos;
    if(dz->mode == GREV_PUT) {
        limit *= 2;
        while(m >= limit) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            memcpy((char *)dz->sampbuf[1],(char *)dz->sampbuf[2],dz->buflen * sizeof(float));
            memset((char *)dz->sampbuf[2],0,dz->buflen * sizeof(float));
            m -= dz->buflen;
        }
    }
    for(n=startsearch;n <endsearch;n++) {
        if(n >= dz->buflen) {
            *last_total_samps_read = dz->total_samps_read;
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            n = 0;
            endsearch -= dz->buflen;
        }
        obuf[m++] = ibuf[n];
        if(m >= limit) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            if(dz->mode == GREV_PUT) {      /* deals with bufptr < 0 */
                memcpy((char *)dz->sampbuf[1],(char *)dz->sampbuf[2],dz->buflen * sizeof(float));
                memset((char *)dz->sampbuf[2],0,dz->buflen * sizeof(float));
                m = dz->buflen;
            } else 
                m = 0;
        }
    }
    *obufpos = m;
    return(FINISHED);
}

/************************** DO_ENVGRAIN_ZEROWRITE **********************/

int do_envgrain_zerowrite(int startsearch,int endsearch,int *obufpos,dataptr dz)
{
    int exit_status;
    int n, m;
    float *obuf = dz->sampbuf[1];
    m = *obufpos;
    for(n=startsearch;n <endsearch;n++) {
        if(m >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            m = 0;
        }
        obuf[m++] = 0.0;
    }
    *obufpos = m;
    return(FINISHED);
}

/************************** DO_ENVGRAIN_ZEROWRITE_DBLBUF **********************/

int do_envgrain_zerowrite_dblbuf(int startsearch,int endsearch,int *obufpos,dataptr dz)
{
    int exit_status;
    int n, m;
    float *obuf = dz->sampbuf[1], *obuf2 = dz->sampbuf[2];
    m = *obufpos;
    for(n=startsearch;n <endsearch;n++) {
        if(m >= dz->buflen * 2) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            memcpy((char *)obuf,(char *)obuf2,dz->buflen * sizeof(float));
            memset((char *)obuf2,0,dz->buflen * sizeof(float));
            m -= dz->buflen;
        }
        obuf[m++] = 0.0;
    }
    *obufpos = m;
    return(FINISHED);
}

/************************** DO_ENVGRAIN_ADDWRITE **********************/

int do_envgrain_addwrite(int startsearch,int endsearch,int *last_total_samps_read,int *obufpos,dataptr dz)
{
    int exit_status;
    int step, n, m;
    float *ibuf = dz->sampbuf[0], *obuf  = dz->sampbuf[1], *obuf2 = dz->sampbuf[2];
    if(*obufpos < 0) {
        sprintf(errstr,"GRAIN TOO LARGE TO BACKTRACK IN BUFFER.\n");
        return(GOAL_FAILED);
    }
    if(startsearch > dz->total_samps_read || startsearch < *last_total_samps_read) {
        step = (startsearch / dz->buflen) * dz->buflen; 
        if((sndseekEx(dz->ifd[0],step,0))<0) {
            sprintf(errstr,"seek error 6\n");
            return(SYSTEM_ERROR);
        }
        *last_total_samps_read = step;
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        dz->total_samps_read = *last_total_samps_read + dz->ssampsread; 
    }
    startsearch -= *last_total_samps_read;
    endsearch   -= *last_total_samps_read;
    m = *obufpos;
    for(n=startsearch;n <endsearch;n++) {
        if(n >= dz->buflen) {
            *last_total_samps_read = dz->total_samps_read;
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            n = 0;
            endsearch -= dz->buflen;
        }
        obuf[m++] += ibuf[n];
        if(m >= dz->buflen * 2) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            memcpy((char *)obuf,(char *)obuf2,dz->buflen * sizeof(float));
            memset((char *)obuf2,0,dz->buflen * sizeof(float));
            m -= dz->buflen;
        }
    }
    *obufpos = m;
    return(FINISHED);
}

/****************************** TIMESTRETCH_ITERATIVE ****************************/

int timestretch_iterative3(dataptr dz)
{
    int   exit_status;
    float *obuf = dz->sampbuf[0];
    int  *pos = dz->lparray[0];
    int  peakcnt, startsearch, endsearch, local_minima_cnt, minimum_element_len;
    int  n,z = 0, outpos, samps_to_write;
    char  *outfilename;
    
    int namelen = strlen(dz->wordstor[0]);
    if((outfilename = (char *)malloc(namelen + 4))==NULL) {
        sprintf(errstr,"Insufficient memory\n");
        return(MEMORY_ERROR);
    }
    strcpy(outfilename,dz->wordstor[0]);
    fprintf(stdout,"INFO: Generating output.\n");
    fflush(stdout);
    if(sloom)
        display_virtual_time(0,dz);
    if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
        return(exit_status);        
    dz->sampbuf[0][dz->insams[0]] = dz->sampbuf[0][dz->insams[0] - 1];  /* wrap around point for interpolation */
    startsearch = (int)round(dz->param[RRR_START] * dz->infile->srate);
    endsearch   = (int)round(dz->param[RRR_END] * dz->infile->srate);

    /* FIND ALL POSITIVE PEAKS : always look only at +ve vals, so all zero-crossings eventually found will be from +ve to -ve */

    peakcnt = 0;
    if((exit_status = find_all_positive_peaks(startsearch,endsearch,&peakcnt,dz)) < 0)
        return(exit_status);

    /* FIND ALL POSITIVE-PEAK MINIMA : overwriting the arrays peak-vals & peak-position with minima-vals & minima-positions */

    local_minima_cnt = 0;
    if((exit_status = find_all_local_minima(peakcnt,&local_minima_cnt,dz)) < 0)
        return(exit_status);

    /* ELIMINATE SPURIOUS MINIMA */

    if((exit_status = eliminate_spurious_minima(&local_minima_cnt,&minimum_element_len,dz)) < 0)
        return (exit_status);

    fprintf(stdout,"INFO: Original number of iterated segments found = %d\n",local_minima_cnt - 1);
    fflush(stdout);

    /* CHECK MINIMA FOUND AGAINST INPUT ESTIMATE */

    if((local_minima_cnt - 1) >= 2 * dz->iparam[RRR_GET]) {
        if((exit_status = eliminate_excess_minima(&local_minima_cnt,pos,dz)) < 0)
            return (exit_status);
        fprintf(stdout,"INFO: Reduced to = %d\n",local_minima_cnt - 1);
        fflush(stdout);
    }
    /* SEARCH FOR ZERO CROSSINGS AFTER MINIMA */

    if(local_minima_cnt > 999) {
        sprintf(errstr,"Found more than 999 segments. Process terminated.\n");
        return(GOAL_FAILED);
    }

    if((exit_status = locate_zero_crossings(local_minima_cnt,dz)) < 0)
        return (exit_status);

    /* COPY SOUND START TO OUTBUF */

    outpos = 0;
    if (pos[0] > 0) {
        fprintf(stdout,"INFO: Cutting start of sound\n");
        fflush(stdout);
    }
    if((exit_status = write_samps(obuf,pos[0],dz))<0)
        return(exit_status);
    outpos = pos[0];
    for (n = 1; n < local_minima_cnt; n++) {
        if((exit_status = headwrite(dz->ofd,dz))<0) {
            return(exit_status);
        }
        if(sndcloseEx(dz->ofd) < 0) {
            fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
            fflush(stdout);
        }
        z++;
        strcpy(outfilename,dz->wordstor[0]);
        if(!sloom)
            insert_new_number_at_filename_end(outfilename,n,0);
        else
            insert_new_number_at_filename_end(outfilename,n,1);

        if((exit_status = create_sized_outfile(outfilename,dz))<0) {
            sprintf(errstr,"WARNING: Can't create output soundfile %s\n",outfilename);
            return(SYSTEM_ERROR);
        }
        if(n==1) {
            fprintf(stdout,"INFO: Cutting each iterated segment\n");
            fflush(stdout);
        }
        obuf = dz->sampbuf[0] + outpos;
        samps_to_write = pos[n] - pos[n-1];
        if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
            return(exit_status);
        outpos = pos[n];
    }
    if((exit_status = headwrite(dz->ofd,dz))<0) {
        free(outfilename);
        return(exit_status);
    }
    if(sndcloseEx(dz->ofd) < 0) {
        fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
        fflush(stdout);
    }
    z++;
    if(pos[n-1] < dz->insams[0]) {
        fprintf(stdout,"INFO: Cutting end of sound\n");
        fflush(stdout);
        strcpy(outfilename,dz->wordstor[0]);
        if(!sloom)
            insert_new_number_at_filename_end(outfilename,n,0);
        else
            insert_new_number_at_filename_end(outfilename,n,1);

        if((exit_status = create_sized_outfile(outfilename,dz))<0) {
            sprintf(errstr,"WARNING: Can't create output soundfile %s\n",outfilename);
            return(SYSTEM_ERROR);
        }
        obuf = dz->sampbuf[0] + outpos;
        samps_to_write = dz->insams[0] - pos[n-1];
        if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
            return(exit_status);
        z++;
    }
    fprintf(stdout,"INFO: Total no of outfiles = %d\n",z);
    fflush(stdout);
    return(FINISHED);
}

