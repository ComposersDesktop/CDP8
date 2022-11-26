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
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <pnames.h>
#include <modeno.h>
#include <arrays.h>
#include <flags.h>
#include <blur.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <blur.h>

#define FIRST_ACCESS_OF_RANDTABLE_REACHES_END_FIRST   (1)
#define SECOND_ACCESS_OF_RANDTABLE_REACHES_END_FIRST  (2)
#define BOTH_ACCESSES_OF_RANDTABLE_REACH_END_TOGETHER (0)

static int  randomly_select_blokstokeep_and_sort_in_ascending_order(dataptr dz);
static int  delete_unwanted_bloks(dataptr dz);
static int  insert(int k,int n,int *iarray,int permcnt);
static int  shuflup(int n,int * iarray,int permcnt);
static int  read_domain_samps(int *reached_end_of_file,int *ssampsread,int windows_wanted,dataptr dz);
static int  copy_domain_to_domainbuf(int *reached_end_of_infile,int *ssampsread,int bufwlen,int permcnt,
                                     int shuf_d_windows,float *dbufptr,float *dbufend,dataptr dz);
static int  copywindows_from_domain_to_image(float *domainbuf,float * dbufend,dataptr dz);
static int      get_randplace(int *randh,int *rande,dataptr dz);
static int  get_other_randplace(int *order,int randh,int rande,int *randh2,int *rande2,dataptr dz);
static int  chorusing(int cc,int cc2,int randend,int randend2,int order,dataptr dz);
static int  do_afix_ffix(int *cc,int *cc2,int *vc,dataptr dz);
static int  do_a(int cc,int vc,int randend,dataptr dz);
static int  do_fvar(int cc,int vc,int randend,dataptr dz);
static int  do_afix_f(int *cc,int *cc2,int *vc,dataptr dz);
static int  do_a_ffix(int *cc,int *cc2,int *vc,dataptr dz);
static int  do_a_f(int *cc,int *cc2,int *vc,dataptr dz);
static int  do_afix(int cc,int vc,int randend, dataptr dz);
static int  do_ffix(int cc,int vc,int randend,dataptr dz);
static int  getnewfrq_choru(int cc,int vc,dataptr dz);
static int  getnewfrq2_choru(int cc,int vc,dataptr dz);
static int  get_sbufposition_corresponding_to_frq(int *channo,double frq,dataptr dz);
static int  advance_to_starttime(float **inputbuf_end,int *samps_read,int *drnk_bufno,dataptr dz);
static int  copy_window_to_outbuf(dataptr dz);
static int  get_randstep(int *step,dataptr dz);
static int  adjust_buffers(int *step,float **inputbuf_end,int *samps_read,int *drnk_bufno, dataptr dz);
static int  flush_outbuf(dataptr dz);
static int  invert_step_back_to_step_fwd_at_start_of_file(int *step,int reflect_off_end,dataptr dz);
static int  place_sbufptr_in_bigbuf_range(int *bigbufs_baktrak,dataptr dz);
static int  invert_step_back_to_step_fwd(int *step,float *orig_sbuf,int *samps_read,
                                         float **inputbuf_end, int reflect_off_end,int drnk_bufno,dataptr dz);
static int  baktrak_to_correct_bigbuf(int bigbufs_baktrak,int *samps_read,float **inputbuf_end,
                                      int *drnk_bufno,dataptr dz);
static int  invert_step_fwd_to_step_back_at_end_of_file(int *step,int reflect_off_start,int drnk_bufno,dataptr dz);
static int  advance_to_correct_bigbuf(float **inputbuf_end,int *samps_read,int *drnk_bufno,dataptr dz);
static int  invert_step_fwd_to_step_back(int *step,float *orig_sbuf,int *samps_read,
                                         float **inputbuf_end,int reflect_off_start,int drnk_bufno,dataptr dz);
static int  read_first_inbuf(dataptr dz);
static int  wrap_samps(unsigned int wrapsamps,float **insbuf,dataptr dz);
static int  copy_1st_window(dataptr dz);

/********************************** SPECAVRG **********************************/

int specavrg(dataptr dz)
{
    int exit_status;
    int cc, vc, k=0, n, m, q;
    if(dz->brksize[AVRG_AVRG]) {
        dz->iparam[AVRG_AVRGSPAN] = dz->iparam[AVRG_AVRG]/2;
        dz->iparam[AVRG_AVRG]     = (dz->iparam[AVRG_AVRGSPAN] * 2) + 1; /* always odd */
    }
    if(dz->iparam[AVRG_AVRGSPAN]>0) {
        if((exit_status = get_amp(dz->flbufptr[0],dz))<0)
            return(exit_status);
        for(cc = 0, vc = 0; cc < dz->iparam[AVRG_AVRGSPAN]; cc++, vc+=2) {
            dz->amp[cc] = 0.0f;
            k = cc + dz->iparam[AVRG_AVRGSPAN] + 1;                         /* SMALL AVERAGE AT BOTTOM */
            for(n = 0, m = 0; n < k; n++, m += 2)
                dz->amp[cc] = (float)(dz->amp[cc] + dz->flbufptr[0][m]);
            dz->amp[cc] = (float)(dz->amp[cc]/(double)(k));
        }
        q = dz->clength - dz->iparam[AVRG_AVRGSPAN];
        m = dz->iparam[AVRG_AVRGSPAN] * 2;
        for(cc = dz->iparam[AVRG_AVRGSPAN], vc = m; cc < q; cc++, vc += 2) {
            dz->amp[cc] = 0.0f;                                                             /* TRUE AVERAGE */
            for(n = vc - m;n <= vc + m; n += 2)
                dz->amp[cc] = (float)(dz->amp[cc] + dz->flbufptr[0][n]);
            dz->amp[cc] = (float)(dz->amp[cc]/(double)dz->iparam[AVRG_AVRG]);
        }
        for(cc = q, vc = k*q; cc < dz->clength; cc++, vc+=2) {
            dz->amp[cc] = 0.0f;
            k = cc - dz->iparam[AVRG_AVRGSPAN];                             /* SMALL AVERAGE AT TOP */
            for(n = k, m = k*2; n < dz->clength; n++, m += 2)
                dz->amp[cc] = (float)(dz->amp[cc] + dz->flbufptr[0][m]);
            dz->amp[cc] = (float)(dz->amp[cc]/(double)(dz->clength-k));
        }
        if((exit_status = put_amp(dz->flbufptr[0],dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/************************** SPECSUPR *****************************/

int specsupr(dataptr dz)
{
    int exit_status;
    chvptr quietest, loudest;
    int invtrindex, vc;
    if(dz->iparam[SUPR_INDX]>(dz->clength/2)) {         /* IF MORE CHANS TO SUPPRESS THAN TO KEEP */
        invtrindex = dz->clength - dz->iparam[SUPR_INDX];
        if((exit_status = initialise_ring_vals(invtrindex,BIGAMP,dz))<0) /* PRESET RING VALS TO MAX */
            return(exit_status);
        for(vc = 0; vc < dz->wanted; vc += 2) {                 /* IF QUIETER THAN PREVIOUS CHANS, STORE */
            if((exit_status = if_one_of_quietest_chans_store_in_ring(vc,dz))<0)
                return(exit_status);
            dz->flbufptr[0][vc] = 0.0f;                                     /* ZERO ALL CHANNELS AS WE GO */
        }
        quietest = dz->ringhead;
        do {                                                                                    /* REINSERT AMP OF QUIETEST CHANS ONLY */
            dz->flbufptr[0][quietest->loc] = quietest->val;
        } while((quietest = quietest->next)!=dz->ringhead);
    } else {                                                                                    /* IF MORE CHANS TO KEEP THAN TO SUPPRESS */
        if((exit_status = initialise_ring_vals(dz->iparam[SUPR_INDX],-BIGAMP,dz))<0)
            return(exit_status);                                            /* PRESET RING VALS TO MIN */
        for(vc = 0; vc < dz->wanted; vc += 2) {
            if((exit_status = if_one_of_loudest_chans_store_in_ring(vc,dz))<0)
                return(exit_status);                                    /* IF LOUDER THAN PREVIOUS CHANS, STORE */
        }
        loudest = dz->ringhead;
        do {
            dz->flbufptr[0][loudest->loc] = 0.0f;           /* ZERO AMPLITUDE IN LOUDEST CHANNELS ONLY */
        } while((loudest = loudest->next)!=dz->ringhead);
    }
    return(FINISHED);
}

/************************ SPECSCAT *****************************/

int specscat(dataptr dz)
{
    int exit_status;
    double pre_totalamp = 0.0, post_totalamp;
    if(dz->brksize[SCAT_CNT])
        dz->iparam[SCAT_THISCNT] = dz->iparam[SCAT_CNT];
    if(dz->brksize[SCAT_BLOKSIZE]) {
        dz->iparam[SCAT_BLOKS_PER_WINDOW] = dz->clength/dz->iparam[SCAT_BLOKSIZE];
        if((dz->iparam[SCAT_BLOKS_PER_WINDOW]*dz->iparam[SCAT_BLOKSIZE])!=dz->clength)
            dz->iparam[SCAT_BLOKS_PER_WINDOW]++;
    }
    if((dz->brksize[SCAT_CNT] || dz->brksize[SCAT_BLOKSIZE])
       &&  dz->iparam[SCAT_CNT] >= dz->iparam[SCAT_BLOKS_PER_WINDOW]) {
        sprintf(errstr,"Blokcnt exceeds number of blocks per window at %.4lf secs\n",
                dz->time - dz->frametime);
        return(USER_ERROR);
    }

    if(!dz->vflag[SCAT_NO_NORM]) {
        if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
            return(exit_status);
    }
    if(dz->vflag[SCAT_RANDCNT])
        dz->iparam[SCAT_THISCNT] = (int)((drand48() * dz->iparam[SCAT_CNT]) + 1.0); /* TRUNCATE */
    if((exit_status = randomly_select_blokstokeep_and_sort_in_ascending_order(dz))<0)
        return(exit_status);
    if((exit_status = delete_unwanted_bloks(dz))<0)
        return(exit_status);
    if(!dz->vflag[SCAT_NO_NORM]) {
        if((exit_status = get_totalamp(&post_totalamp,dz->flbufptr[0],dz->wanted))<0)
            return(exit_status);
        if((exit_status = normalise(pre_totalamp,post_totalamp,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/********** RANDOMLY_SELECT_BLOKSTOKEEP_AND_SORT_IN_ASCENDING_ORDER **********
 *
 * (1)  preset all SCAT_KEEP array vals to max.
 * (2)  insert blokno between 0 and SCAT_BLOKS_PER_WINDOW in 1st position  in SCAT_KEEP array.
 * (3)  insert blokno between 0 and SCAT_BLOKS_PER_WINDOW in next position in SCAT_KEEP array.
 * (4)  ensure its a NEW value
 * (5)  if it's not a new value, try again.
 * (6)  order the existing new values
 */

int randomly_select_blokstokeep_and_sort_in_ascending_order(dataptr dz)
{
    int exit_status;
    int n, m, k, keepcnt, bad_value, inserted;
    for(n=0;n<dz->clength+1;n++)
        dz->iparray[SCAT_KEEP][n] = dz->clength+1;                              /* 1 */
    k = (int)(drand48() * dz->iparam[SCAT_BLOKS_PER_WINDOW]);/* TRUNC */
    dz->iparray[SCAT_KEEP][0] = k;                                                      /* 2 */
    keepcnt = 1;
    for(n=1;n<dz->iparam[SCAT_THISCNT];n++) {
        k = (int)(drand48() * dz->iparam[SCAT_BLOKS_PER_WINDOW]);/* TRUNC */    /* 3 */
        bad_value = 0;
        inserted  = 0;
        for(m=0;m<keepcnt;m++) {                                                                /* 4 */
            if(k==dz->iparray[SCAT_KEEP][m]) {
                bad_value = 1;
                break;
            }
        }                                                                                                               /* 5 */
        if(bad_value) {
            n--;
            continue;
        }
        for(m=0;m<keepcnt;m++) {                                                                /* 6 */
            if(k<dz->iparray[SCAT_KEEP][m]) {
                if((exit_status = insert(k,m,dz->iparray[SCAT_KEEP],keepcnt))<0)
                    return(exit_status);
                inserted = 1;
                break;
            }
        }
        if(!inserted)
            dz->iparray[SCAT_KEEP][keepcnt] = k;
        keepcnt++;
    }
    if(keepcnt!=dz->iparam[SCAT_THISCNT]) {
        sprintf(errstr,"Error in perm arithmetic.randomly_select_blokstokeep_and_sort_in_ascending_order()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** DELETE_UNWANTED_BLOKS *****************************/

int delete_unwanted_bloks(dataptr dz)
{
    int this_blok = 0;              /* INDEX TO BLKS TO RETAIN */
    int n, c_end, vc_end;
    int cc = 0, vc;
    for(n = 0; n < dz->iparam[SCAT_BLOKS_PER_WINDOW]; n++) {
        if(n==dz->iparray[SCAT_KEEP][this_blok]) {                              /* If keeping it */
            if((cc += dz->iparam[SCAT_BLOKSIZE]) >= dz->clength)/* Jump over it, and, if at window end, break */
                break;
            if(++this_blok >= dz->iparam[SCAT_THISCNT]) {           /* If at end of kept bloks */
                vc = cc * 2;
                while(vc < dz->wanted) {                                                /* zero any remaining chans */
                    dz->flbufptr[0][AMPP] = 0.0F;
                    vc += 2;
                }
                break;
            }
        } else {                                                                                                /* if NOT keeping it */
            if((c_end = cc + dz->iparam[SCAT_BLOKSIZE]) >= dz->clength)
                c_end = dz->clength;                                                    /* mark end of blok */
            vc = cc * 2;
            vc_end = c_end * 2;
            while(vc < vc_end) {                                                            /* zero it */
                dz->flbufptr[0][AMPP] = 0.0F;
                vc += 2;
            }
            if(vc >= dz->wanted)                                                            /* If at window end, break */
                break;
        }
    }
    return(FINISHED);
}

/****************************** INSERT ***************************/

int insert(int k,int n,int *iarray,int permcnt)
{
    int exit_status;
    if((exit_status = shuflup(n,iarray,permcnt))<0)
        return(exit_status);
    iarray[n] = k;
    return(FINISHED);
}

/****************************** SHUFLUP ***************************/

int shuflup(int n,int * iarray,int permcnt)
{
    int z = permcnt;
    while(z > n) {
        iarray[z] = iarray[z-1];
        z--;
    }
    return(FINISHED);
}

/**************************** SPECSPREAD ***************************/

int specspread(dataptr dz)
{
    int exit_status;
    int cc, vc;
    double specenv_amp, ampdiff, pre_totalamp, post_totalamp;
    rectify_window(dz->flbufptr[0],dz);
    if((exit_status = extract_specenv(0,0,dz))<0)
        return(exit_status);
    if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    for( cc = 0 ,vc = 0; cc < dz->clength; cc++, vc += 2) {
        if((exit_status = getspecenvamp(&specenv_amp,(double)dz->flbufptr[0][FREQ],0,dz))<0)
            return(exit_status);
        ampdiff              = specenv_amp - dz->flbufptr[0][AMPP];
        dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] + (ampdiff * dz->param[SPREAD_SPRD]));
    }
    if((exit_status = get_totalamp(&post_totalamp,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    if((exit_status = normalise(pre_totalamp,post_totalamp,dz))<0)
        return(exit_status);
    return(FINISHED);
}

/******************************** SPECSHUFFLE *****************************/

int specshuffle(dataptr dz)
{
    int   exit_status;
    int  permcnt = 0, ssampsread;
    int  shuf_d_windows = dz->iparam[SHUF_DMNCNT] * dz->iparam[SHUF_GRPSIZE];
    int  bufwlen = dz->buflen/dz->wanted;
    float *domainbuf, *dbufptr, *dbufend;
    int   reached_end_of_infile = FALSE;
    int  min_windows_to_get = min(shuf_d_windows+1,bufwlen);
    dz->flbufptr[0] = dz->bigfbuf;
    dz->flbufptr[1] = dz->flbufptr[2];
    if(sloom)
        dz->total_samps_read = 0;
    if((domainbuf = (float *)malloc((size_t)(shuf_d_windows * (dz->wanted * sizeof(float)))))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for domain buffer.\n");
        return(MEMORY_ERROR);
    }
    dbufptr = domainbuf;
    dbufend = domainbuf + (shuf_d_windows * dz->wanted);

    if((exit_status = read_domain_samps(&reached_end_of_infile,&ssampsread,min_windows_to_get,dz))<0)
        return(exit_status);
    if(exit_status == FINISHED) {
        sprintf(errstr,"Insufficient data in soundfile to do this shuffle.\n");
        return(GOAL_FAILED);
    }

    copy_1st_window(dz);
    for(;;) {
        dbufptr = domainbuf;
        if((exit_status = copy_domain_to_domainbuf
            (&reached_end_of_infile,&ssampsread,bufwlen,permcnt,shuf_d_windows,dbufptr,dbufend,dz))<0)
            return(exit_status);
        if(exit_status==FINISHED)
            break;
        copywindows_from_domain_to_image(domainbuf,dbufend,dz);
        permcnt++;
    }
    if(dz->flbufptr[1] > dz->flbufptr[2])
        return write_samps(dz->flbufptr[2],(dz->flbufptr[1] - dz->flbufptr[2]),dz);
    return(FINISHED);

}

/********************** READ_DOMAIN_SAMPS *******************/

int read_domain_samps(int *reached_end_of_file,int *ssampsread,int windows_wanted,dataptr dz)
{
    int exit_status;

    if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
        return exit_status;
    if(dz->ssampsread < windows_wanted * dz->wanted)
        return(FINISHED);
    *ssampsread = dz->ssampsread;
    if(dz->ssampsread < dz->buflen)
        *reached_end_of_file = TRUE;
    return(CONTINUE);
}

/********************** COPY_DOMAIN_TO_DOMAINBUF *******************/

int copy_domain_to_domainbuf
(int *reached_end_of_infile,int *ssampsread,int bufwlen,int permcnt,int shuf_d_windows,
 float *dbufptr,float *dbufend,dataptr dz)
{
    int exit_status;
    int n, windows_needed;
    float *data_end;
    if(reached_end_of_infile)
        data_end = dz->bigfbuf + *ssampsread;
    else
        data_end = dz->flbufptr[2];
    for(n=0;n<shuf_d_windows;n++) {
        if(dbufptr >= dbufend) {
            sprintf(errstr,"Error in buffer arithmetic: specshuffle()\n");
            return(PROGRAM_ERROR);
        }
        if(dz->flbufptr[0] >= data_end) {
            if(!(*reached_end_of_infile)) {
                windows_needed = min(shuf_d_windows-n,bufwlen);
                if((exit_status = read_domain_samps(reached_end_of_infile,ssampsread,windows_needed,dz))<0)
                    return(exit_status);
                if(*reached_end_of_infile)
                    data_end = dz->bigfbuf + *ssampsread;
                dz->flbufptr[0] = dz->bigfbuf;
            } else
                exit_status = FINISHED;
            if(exit_status==FINISHED) {
                if(permcnt==0) {
                    sprintf(errstr,"Insufficient data in soundfile to do this shuffle.\n");
                    return(GOAL_FAILED);
                }
                return(FINISHED);
            }
        }
        memmove((char *)dbufptr,(char *)dz->flbufptr[0],(size_t)(dz->wanted * sizeof(float)));
        dbufptr        += dz->wanted;
        dz->flbufptr[0] += dz->wanted;
    }
    return(CONTINUE);
}

/************************** COPYWINDOWS_FROM_DOMAIN_TO_IMAGE ********************************/

int copywindows_from_domain_to_image(float *domainbuf,float * dbufend,dataptr dz)
{
    int exit_status;
    int n, m;
    float *srcptr;
    for(n=0;n<dz->iparam[SHUF_IMGCNT];n++) {
        srcptr  = domainbuf + (dz->iparray[SHUF_MAP][n] * dz->iparam[SHUF_GRPSIZE] * dz->wanted);
        for(m=0;m < dz->iparam[SHUF_GRPSIZE]; m++) {
            if(srcptr >= dbufend) {
                sprintf(errstr,"Error in buffer arithmetic: copywindows_from_domain_to_image()\n");
                return(PROGRAM_ERROR);
            }
            if(dz->flbufptr[1] >= dz->flbufptr[3]) {
                if((exit_status = write_exact_samps(dz->flbufptr[2],dz->buflen,dz))<0)
                    return(exit_status);
                dz->flbufptr[1] = dz->flbufptr[2];
            }
            memmove((char *)dz->flbufptr[1],(char *)srcptr,(size_t)(dz->wanted * sizeof(float)));
            srcptr         += dz->wanted;
            dz->flbufptr[1] += dz->wanted;
        }
    }
    return(FINISHED);
}

/************************* SPECCHORUS ***************************/

int specchorus(dataptr dz)
{
    int exit_status;
    //TW cc2 is never set for modes other than CH_AMP_FRQ, CH_AMP_FRQ_UP, CH_AMP_FRQ_DN
    int cc, cc2 = 0, order = 0;
    int randhere = 0, randhere2 = 0, randend = 0, randend2 = 0;
    double pre_totalamp = 0.0, post_totalamp;
    if((exit_status = get_randplace(&randhere,&randend,dz))<0)
        return(exit_status);
    cc = randhere;
    if(dz->mode == CH_AMP_FRQ || dz->mode == CH_AMP_FRQ_UP || dz->mode == CH_AMP_FRQ_DN) {
        if((exit_status = get_other_randplace(&order,randhere,randend,&randhere2,&randend2,dz))<0)
            return(exit_status);
        cc2 = randhere2;
    }
    if(dz->iparam[CHORU_SPRTYPE]!=F_VAR && dz->iparam[CHORU_SPRTYPE]!=F_FIX) {
        if((exit_status = get_totalamp(&pre_totalamp,dz->flbufptr[0],dz->wanted))<0)
            return(exit_status);
    }
    if((exit_status = chorusing(cc,cc2,randend,randend2,order,dz))<0)
        return(exit_status);
    if(dz->iparam[CHORU_SPRTYPE]!=F_VAR && dz->iparam[CHORU_SPRTYPE]!=F_FIX) {
        if((exit_status = get_totalamp(&post_totalamp,dz->flbufptr[0],dz->wanted))<0)
            return(exit_status);
        if((exit_status = normalise(pre_totalamp,post_totalamp,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/************************** GET_RANDPLACE *************************/

int     get_randplace(int *randh,int *rande,dataptr dz)
{
    *randh = (int)(drand48() * dz->iparam[CHORU_RTABSIZE]);
    if((*rande = *randh + dz->clength) >= dz->iparam[CHORU_RTABSIZE])
        *rande = dz->iparam[CHORU_RTABSIZE];
    return(FINISHED);
}

/************************** GET_OTHER_RANDPLACE **************************/

int get_other_randplace(int *order,int randh,int rande,int *randh2,int *rande2,dataptr dz)
{
    int firstlen, firstlen2;
    *randh2 = (int)(drand48() * dz->iparam[CHORU_RTABSIZE]);
    if((*rande2 = *randh2 + dz->clength) > dz->iparam[CHORU_RTABSIZE])
        *rande2 = dz->iparam[CHORU_RTABSIZE];
    firstlen  = rande   - randh;
    firstlen2 = *rande2 - *randh2;
    if(firstlen<firstlen2)  /* 1 */
        *order = FIRST_ACCESS_OF_RANDTABLE_REACHES_END_FIRST;
    else if(firstlen2<firstlen)     /* 2 */
        *order = SECOND_ACCESS_OF_RANDTABLE_REACHES_END_FIRST;
    else
        *order = BOTH_ACCESSES_OF_RANDTABLE_REACH_END_TOGETHER;                         /* 3 */
    return(FINISHED);
}

/******************************* CHORUSING *****************************/

int chorusing(int cc,int cc2,int randend,int randend2,int order,dataptr dz)
{
    int exit_status;
    int vc;
    switch(dz->iparam[CHORU_SPRTYPE]) {
    case(A_FIX):
        vc = 0;
        if((exit_status = do_afix(cc,vc,randend,dz))<0)
            return(exit_status);
        break;
    case(F_FIX):
        vc = 1;
        memset((char *)dz->windowbuf[0],0,(size_t)(dz->wanted * sizeof(float)));
        if((exit_status = do_ffix(cc,vc,randend,dz))<0)
            return(exit_status);
        memmove((char *)dz->flbufptr[0],(char *)dz->windowbuf[0],(size_t)(dz->wanted * sizeof(float)));
        break;
    case(AF_FIX):
        vc = 0;
        memset((char *)dz->windowbuf[0],0,(size_t)(dz->wanted * sizeof(float)));
        switch(order) {
        case(BOTH_ACCESSES_OF_RANDTABLE_REACH_END_TOGETHER):
            while(cc<randend) {
                if((exit_status = do_afix_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc = cc2 = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_afix_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        case(FIRST_ACCESS_OF_RANDTABLE_REACHES_END_FIRST):
            while(cc<randend) {
                if((exit_status = do_afix_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc  = 0;
            while(cc2 < randend2) {
                if((exit_status = do_afix_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc2 = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_afix_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        case(SECOND_ACCESS_OF_RANDTABLE_REACHES_END_FIRST):
            while(cc2<randend2) {
                if((exit_status = do_afix_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc2  = 0;
            while(cc < randend) {
                if((exit_status = do_afix_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_afix_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        }
        memmove((char *)dz->flbufptr[0],(char *)dz->windowbuf[0],(size_t)(dz->wanted * sizeof(float)));
        break;
    case(A_VAR):
        vc = 0;
        if((exit_status = do_a(cc,vc,randend,dz))<0)
            return(exit_status);
        break;
    case(F_VAR):
        vc = 1;
        memset((char *)dz->windowbuf[0],0,(size_t)(dz->wanted * sizeof(float)));
        if((exit_status = do_fvar(cc,vc,randend,dz))<0)
            return(exit_status);
        memmove((char *)dz->flbufptr[0],(char *)dz->windowbuf[0],(size_t)(dz->wanted * sizeof(float)));
        break;
    case(A_FIX_F_VAR):
        vc = 0;
        memset((char *)dz->windowbuf[0],0,(size_t)(dz->wanted * sizeof(float)));
        switch(order) {
        case(BOTH_ACCESSES_OF_RANDTABLE_REACH_END_TOGETHER):
            while(cc<randend) {
                if((exit_status = do_afix_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc = cc2 = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_afix_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        case(FIRST_ACCESS_OF_RANDTABLE_REACHES_END_FIRST):
            while(cc<randend) {
                if((exit_status = do_afix_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc  = 0;
            while(cc2 < randend2) {
                if((exit_status = do_afix_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc2 = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_afix_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        case(SECOND_ACCESS_OF_RANDTABLE_REACHES_END_FIRST):
            while(cc2<randend2) {
                if((exit_status = do_afix_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc2  = 0;
            while(cc < randend) {
                if((exit_status = do_afix_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_afix_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        default:
            sprintf(errstr,"Impossible order value %d: chorusing()\n",order);
            return(PROGRAM_ERROR);
        }
        memmove((char *)dz->flbufptr[0],(char *)dz->windowbuf[0],(size_t)(dz->wanted * sizeof(float)));
        break;
    case(A_VAR_F_FIX):
        vc = 0;
        memset((char *)dz->windowbuf[0],0,(size_t)(dz->wanted * sizeof(float)));
        switch(order) {
        case(BOTH_ACCESSES_OF_RANDTABLE_REACH_END_TOGETHER):
            while(cc<randend) {
                if((exit_status = do_a_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc = cc2 = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_a_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        case(FIRST_ACCESS_OF_RANDTABLE_REACHES_END_FIRST):
            while(cc<randend) {
                if((exit_status = do_a_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc  = 0;
            while(cc2 < randend2) {
                if((exit_status = do_a_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc2 = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_a_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        case(SECOND_ACCESS_OF_RANDTABLE_REACHES_END_FIRST):
            while(cc2<randend2) {
                if((exit_status = do_a_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc2  = 0;
            while(cc < randend) {
                if((exit_status = do_a_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_a_ffix(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        default:
            sprintf(errstr,"Impossible (2) order value %d: chorusing()\n",order);
            return(PROGRAM_ERROR);
        }
        memmove((char *)dz->flbufptr[0],(char *)dz->windowbuf[0],(size_t)(dz->wanted * sizeof(float)));
        break;
    case(AF_VAR):
        vc = 0;
        memset((char *)dz->windowbuf[0],0,(size_t)(dz->wanted * sizeof(float)));
        switch(order) {
        case(BOTH_ACCESSES_OF_RANDTABLE_REACH_END_TOGETHER):
            while(cc<randend)  {
                if((exit_status = do_a_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc = cc2 = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_a_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        case(FIRST_ACCESS_OF_RANDTABLE_REACHES_END_FIRST):
            while(cc<randend) {
                if((exit_status = do_a_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc  = 0;
            while(cc2 < randend2) {
                if((exit_status = do_a_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc2 = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_a_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        case(SECOND_ACCESS_OF_RANDTABLE_REACHES_END_FIRST):
            while(cc2<randend2) {
                if((exit_status = do_a_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc2  = 0;
            while(cc < randend) {
                if((exit_status = do_a_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            cc = 0;
            while(vc < dz->wanted) {
                if((exit_status = do_a_f(&cc,&cc2,&vc,dz))<0)
                    return(exit_status);
            }
            break;
        default:
            sprintf(errstr,"Impossible (3) order value %d: chorusing()\n",order);
            return(PROGRAM_ERROR);
        }
        memmove((char *)dz->flbufptr[0],(char *)dz->windowbuf[0],(size_t)(dz->wanted * sizeof(float)));
        break;
    default:
        sprintf(errstr,"Impossible sprtype value %d in chorusing()\n",dz->iparam[CHORU_SPRTYPE]);
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/******************************* DO_AFIX_FFIX *********************/

int do_afix_ffix(int *cc,int *cc2,int *vc,dataptr dz)
{
    int exit_status;
    dz->flbufptr[0][*vc] = (float)(dz->flbufptr[0][*vc] * dz->parray[CHORU_RTABA][*cc]);
    (*cc)++;
    (*vc)++;
    if((exit_status = getnewfrq_choru(*cc2,*vc,dz))<0)
        return(exit_status);
    (*cc2)++;
    (*vc)++;
    return(FINISHED);                                       /* number in flbufptr[0] */
}

/*************************** DO_A ****************************/

int do_a(int cc,int vc,int randend,dataptr dz)
{
    while(cc < randend) {
        dz->flbufptr[0][vc] = (float)(pow(dz->param[CHORU_AMPR],dz->parray[CHORU_RTABA][cc]) * dz->flbufptr[0][vc]);
        cc++;
        vc += 2;
    }
    cc = 0;
    while(vc < dz->wanted) {
        dz->flbufptr[0][vc] = (float)(pow(dz->param[CHORU_AMPR],dz->parray[CHORU_RTABA][cc]) * dz->flbufptr[0][vc]);
        cc++;
        vc += 2;
    }
    return(FINISHED);
}

/***************************** DO_FVAR *****************************/

int do_fvar(int cc,int vc,int randend,dataptr dz)
{
    int exit_status;
    while(cc < randend) {
        if((exit_status = getnewfrq2_choru(cc,vc,dz))<0)
            return(exit_status);
        cc++;
        vc += 2;
    }
    cc = 0;
    while(vc < dz->wanted) {
        if((exit_status = getnewfrq2_choru(cc,vc,dz))<0)
            return(exit_status);
        cc++;
        vc += 2;
    }
    return(FINISHED);
}

/******************************* DO_AFIX_F *********************/

int do_afix_f(int *cc,int *cc2,int *vc,dataptr dz)
{
    int exit_status;
    dz->flbufptr[0][*vc] = (float)(dz->flbufptr[0][*vc] * dz->parray[CHORU_RTABA][*cc]);
    (*cc)++;
    (*vc)++;
    if((exit_status = getnewfrq2_choru(*cc2,*vc,dz))<0)
        return(exit_status);
    (*cc2)++;
    (*vc)++;
    return(FINISHED);
}

/************************* DO_A_FFIX ***************************/

int do_a_ffix(int *cc,int *cc2,int *vc,dataptr dz)
{
    int exit_status;
    dz->flbufptr[0][*vc] = (float)(dz->flbufptr[0][*vc] * pow(dz->param[CHORU_AMPR],dz->parray[CHORU_RTABA][*cc]));
    (*cc)++;
    (*vc)++;
    if((exit_status = getnewfrq_choru(*cc2,*vc,dz))<0)
        return(exit_status);
    (*cc2)++;
    (*vc)++;
    return(FINISHED);
}

/************************* DO_A_F ***************************/

int do_a_f(int *cc,int *cc2,int *vc,dataptr dz)
{
    int exit_status;
    dz->flbufptr[0][*vc] = (float)(dz->flbufptr[0][*vc] * pow(dz->param[CHORU_AMPR],dz->parray[CHORU_RTABA][*cc]));
    (*cc)++;
    (*vc)++;
    if((exit_status = getnewfrq_choru(*cc2,*vc,dz))<0)
        return(exit_status);
    (*cc2)++;
    (*vc)++;
    return(FINISHED);
}

/***************************** DO_AFIX *************************/

int do_afix(int cc,int vc,int randend, dataptr dz)
{
    while(cc < randend)     {
        dz->flbufptr[0][vc] = (float)(dz->flbufptr[0][vc] * dz->parray[CHORU_RTABA][cc]);
        cc++;
        vc += 2;
    }
    cc = 0;
    while(vc < dz->wanted) {
        dz->flbufptr[0][vc] = (float)(dz->flbufptr[0][vc] * dz->parray[CHORU_RTABA][cc]);
        cc++;
        vc += 2;
    }
    return(FINISHED);
}

/*************************** DO_FFIX *************************/

int do_ffix(int cc,int vc,int randend,dataptr dz)
{
    int exit_status;
    while(cc < randend) {
        if((exit_status = getnewfrq_choru(cc,vc,dz))<0)
            return(exit_status);
        cc++;
        vc += 2;
    }
    cc = 0;
    while(vc < dz->wanted) {
        if((exit_status = getnewfrq_choru(cc,vc,dz))<0)
            return(exit_status);
        cc++;
        vc += 2;
    }
    return(FINISHED);
}

/************************* GETNEWFRQ_CHORU ****************************
 *
 * (1)  If scatter takes frq > current chan, do the scatter     downwards instead.
 * (1)  If scatter takes frq < current chan, do the scatter upwards instead.
 * (3)  Otherwise, keep originally calculated value.
 */

int getnewfrq_choru(int cc,int vc,dataptr dz)
{
    int exit_status;
    float amp;
    double newval;
    int new_chan;
    newval = dz->flbufptr[0][vc] * dz->parray[CHORU_RTABF][cc];
    if(newval > dz->nyquist || newval < dz->halfchwidth)
        return(FINISHED);
    if((exit_status = get_sbufposition_corresponding_to_frq(&new_chan,newval,dz))<0)
        return(exit_status);
    if((amp = dz->flbufptr[0][vc-1]) > dz->windowbuf[0][new_chan]) {
        dz->windowbuf[0][new_chan++] = amp;
        dz->windowbuf[0][new_chan]   = (float)newval;
    }
    return(FINISHED);
}

/************************** GETNEWFRQ2_CHORU *************************/

int getnewfrq2_choru(int cc,int vc,dataptr dz)
{
    int exit_status;
    float amp;
    double newval, newscat = pow(dz->param[CHORU_FRQR],dz->parray[CHORU_RTABF][cc]);
    int new_vc;
    newval = dz->flbufptr[0][vc] * newscat;
    if(newval > dz->nyquist || newval < dz->halfchwidth)
        return(FINISHED);
    if((exit_status = get_sbufposition_corresponding_to_frq(&new_vc,newval,dz))<0)
        return(exit_status);
    if((amp = dz->flbufptr[0][vc-1]) > dz->windowbuf[0][new_vc]) {
        dz->windowbuf[0][new_vc++] = amp;
        dz->windowbuf[0][new_vc]   = (float)newval;
    }
    return(FINISHED);
}

/****************************** GET_SBUFPOSITION_CORRESPONDING_TO_FRQ ****************************/

int get_sbufposition_corresponding_to_frq(int *channo,double frq,dataptr dz)
{
    frq  += dz->halfchwidth;
    *channo = (int)(frq/dz->chwidth);/* TRUNCATE */ /* find channel number */
    *channo = min(dz->clength-1,max(0,*channo));    /* Outside range TRAP */
    (*channo) *= 2;
    return(FINISHED);                                       /* number in flbufptr[0] */
}

/**************************** SPECNOISE ***************************/

int specnoise(dataptr dz)
{
    int exit_status;
    int vc;
    double totalamp;
    if((exit_status = get_totalamp(&totalamp,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    totalamp /= dz->clength;
    for(vc = 0; vc < dz->wanted; vc += 2)
        dz->flbufptr[0][vc] =
            (float)(((totalamp - dz->flbufptr[0][vc]) * dz->param[NOISE_NOIS]) + dz->flbufptr[0][vc]);
    return(FINISHED);
}

/****************************** SPECDRUNK ***************************/

int specdrunk(dataptr dz)
{
    int exit_status;
    int samps_read=0, step;
    float *inputbuf_end;
    int drnk_bufno = -1;
    dz->time = 0.0f;
    if((exit_status = advance_to_starttime(&inputbuf_end,&samps_read,&drnk_bufno,dz))<0)
        return(exit_status);
    dz->flbufptr[1] = dz->flbufptr[2];
    for(;;) {
        if((exit_status = copy_window_to_outbuf(dz))<0)
            return(exit_status);
        if((dz->time = (float)(dz->time + dz->frametime)) >= dz->param[DRNK_DUR])
            break;
        if((exit_status = get_randstep(&step,dz))<0)
            return(exit_status);
        dz->flbufptr[0] += step * dz->wanted;
        if(dz->flbufptr[0] < dz->bigfbuf || dz->flbufptr[0] >= inputbuf_end) {
            if((exit_status = adjust_buffers(&step,&inputbuf_end,&samps_read,&drnk_bufno,dz))<0)
                return(exit_status);
        }
    }
    if((exit_status = flush_outbuf(dz))<0)
        return(exit_status);
    return(FINISHED);
}

/****************************** ADVANCE_TO_STARTTIME ***************************/

int advance_to_starttime(float **inputbuf_end,int *samps_read,int *drnk_bufno,dataptr dz)
{
    int exit_status;
    double  time_read = 0.0, previous_time_read = 0.0, time_in_buf;
    int     startwindow_in_buf, w_to_buf;
    while(time_read <= dz->param[DRNK_STIME]) {
        if((exit_status = read_samps(dz->bigfbuf,dz)) < 0) {
            sprintf(errstr,"No data found in input analysis file.\n");
            return(SYSTEM_ERROR);
        }
        if(dz->ssampsread == 0) {
            sprintf(errstr,"No data found in input analysis file.\n");
            return(DATA_ERROR);
        }
        *samps_read = dz->ssampsread;

        (*drnk_bufno)++;
        w_to_buf                   = dz->ssampsread/dz->wanted;
        previous_time_read = time_read;
        time_read         += (w_to_buf * dz->frametime);
    }
    *inputbuf_end      = dz->bigfbuf + dz->ssampsread;
    time_in_buf        = dz->param[DRNK_STIME] - previous_time_read;
    startwindow_in_buf = round(time_in_buf/dz->frametime);
    dz->flbufptr[0]     = dz->bigfbuf + (startwindow_in_buf * dz->wanted);
    return(FINISHED);
}

/****************************** COPY_WINDOW_TO_OUTBUF ***************************/

int copy_window_to_outbuf(dataptr dz)
{
    int exit_status;
    int vc;
    for(vc = 0; vc < dz->wanted; vc++)
        dz->flbufptr[1][vc] = dz->flbufptr[0][vc];
    if((dz->flbufptr[1] += dz->wanted) >= dz->flbufptr[3]) {
        if((exit_status = write_exact_samps(dz->flbufptr[2],dz->buflen,dz))<0)
            return(exit_status);
        dz->flbufptr[1] = dz->flbufptr[2];
    }
    return(FINISHED);
}

/****************************** GET_RANDSTEP ***************************/

int get_randstep(int *step,dataptr dz)
{
    switch(dz->vflag[DRNK_NO_ZEROSTEPS]) {
    case(FALSE):
        *step  = (int)(drand48() * dz->iparam[DRNK_TWICERANGE]);                        /* TRUNCATE */
        *step -= dz->iparam[DRNK_RANGE];
        break;
    case(TRUE):
        do {
            *step  = (int)(drand48() * dz->iparam[DRNK_TWICERANGE]);                /* TRUNCATE */
            *step -= dz->iparam[DRNK_RANGE];
        } while(*step==0);
        break;
    default:
        sprintf(errstr,"unknown case in get_randstep()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** ADJUST_BUFFERS ***********************/

int adjust_buffers(int *step,float **inputbuf_end,int *samps_read,int *drnk_bufno, dataptr dz)
{
    //TW UPDATED
    int exit_status;
    int bigbufs_baktrak = 0;
    int baksamps, total_samps_read, seektest;
    int reflect_off_start = FALSE, reflect_off_end = FALSE;
    if(*drnk_bufno < 0) {
        sprintf(errstr,"ADJUST_BUFFERS START bufno = %d (less than zero)\n",*drnk_bufno);
        return(PROGRAM_ERROR);
    }
    do {
        if(dz->flbufptr[0] < dz->bigfbuf) {                     /* IF OFF START OF BUFFER */
            if(*drnk_bufno == 0)    {               /* IF OFF START OF FILE */
                if((exit_status = invert_step_back_to_step_fwd_at_start_of_file(step,reflect_off_end,dz))<0)
                    return(exit_status);
                reflect_off_start = TRUE;
            } else {                                                                /* ELSE NOT OFF START OF FILE */
                if((exit_status = place_sbufptr_in_bigbuf_range(&bigbufs_baktrak,dz))<0)
                    return(exit_status);
                baksamps = (bigbufs_baktrak * dz->buflen) + *samps_read;
                if((seektest = sndseekEx(dz->ifd[0],-baksamps,1)) <= 0) {       /* IF FELL OF FILE START */
                    if(seektest<0) {
                        sprintf(errstr,"sndseekEx() failed.\n");
                        return(SYSTEM_ERROR);
                    }
                    if((exit_status = invert_step_back_to_step_fwd
                        (step,dz->flbufptr[0],samps_read,inputbuf_end,reflect_off_end,*drnk_bufno,dz))<0)
                        return(exit_status);
                    reflect_off_start = TRUE;
                } else {
                    if((exit_status = baktrak_to_correct_bigbuf(bigbufs_baktrak,samps_read,inputbuf_end,drnk_bufno,dz))<0)
                        return(exit_status);
                    reflect_off_start = FALSE;
                }
            }
        } else
            reflect_off_start = FALSE;
        total_samps_read = min(dz->insams[0],(*drnk_bufno+1) * dz->buflen);

        if(dz->flbufptr[0] >= *inputbuf_end) {                          /* IF OFF END OF BUFFER */
            if(total_samps_read >= dz->insams[0]) { /* IF OFF END OF FILE */
                if((exit_status = invert_step_fwd_to_step_back_at_end_of_file(step,reflect_off_start,*drnk_bufno,dz))<0)
                    return(exit_status);
                reflect_off_end = TRUE;
            } else {                                                                        /* ELSE NOT END OF FILE */
                if((exit_status = advance_to_correct_bigbuf(inputbuf_end,samps_read,drnk_bufno,dz))<0)
                    return(exit_status);
                if(dz->flbufptr[0] >= *inputbuf_end) {          /* IF FELL OFF END OF FILE */
                    if((exit_status = invert_step_fwd_to_step_back
                        (step,dz->flbufptr[0],samps_read,inputbuf_end,reflect_off_start,*drnk_bufno,dz))<0)
                        return(exit_status);
                    reflect_off_end = TRUE;
                } else
                    reflect_off_end = FALSE;
            }
        } else
            reflect_off_end = FALSE;
    } while(reflect_off_end);
    if(*drnk_bufno < 0) {
        sprintf(errstr,"ADJUST_BUFFERS END bufno = %d (less than zero)\n",*drnk_bufno);
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** FLUSH_OUTBUF ***************************/

int flush_outbuf(dataptr dz)
{
    int exit_status;
    int samps_to_write;
    if(dz->flbufptr[1] != dz->flbufptr[2]) {
        samps_to_write = dz->flbufptr[1] - dz->flbufptr[2];
        if((exit_status = write_samps(dz->flbufptr[2],samps_to_write,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/****************************** INVERT_STEP_BACK_TO_STEP_FWD_AT_START_OF_FILE ***********************/

int invert_step_back_to_step_fwd_at_start_of_file(int *step,int reflect_off_end,dataptr dz)
{
    if(reflect_off_end) {
        sprintf(errstr,"Step turned out to be too large.\n");
        return(DATA_ERROR);
    }
    *step = -(*step);                       /* REFLECT FROM START OF FILE */
    dz->flbufptr[0] += (*step) * 2 * dz->wanted;
    return(FINISHED);
}

/****************************** PLACE_SBUFPTR_IN_BIGBUF_RANGE ***********************/

int place_sbufptr_in_bigbuf_range(int *bigbufs_baktrak,dataptr dz)
{
    while(dz->flbufptr[0] < dz->bigfbuf) {          /* GO BACK THROUGH BUFS */
        dz->flbufptr[0] += dz->big_fsize;
        (*bigbufs_baktrak)++;
    }
    return(FINISHED);
}

/****************************** INVERT_STEP_BACK_TO_STEP_FWD  ***********************/

int invert_step_back_to_step_fwd
(int *step,float *orig_sbuf,int *samps_read,float **inputbuf_end, int reflect_off_end,int drnk_bufno,dataptr dz)
{
    int exit_status;
    if(reflect_off_end) {
        sprintf(errstr,"Step turned out ot be too large.\n");
        return(DATA_ERROR);
    }
    if(sndseekEx(dz->ifd[0],drnk_bufno * dz->buflen,0)<0) {
        sprintf(errstr,"Seek problem in invert_step_back_to_step_fwd().\n");
        return(SYSTEM_ERROR);
    }
    if((exit_status = read_samps(dz->bigfbuf,dz)) < 0) {
        sprintf(errstr,"Problem reading buffer:in invert_step_back_to_step_fwd().\n");
        return(SYSTEM_ERROR);
    }
    if(dz->ssampsread == 0) {
        sprintf(errstr,"Problem reading buffer:in invert_step_back_to_step_fwd().\n");
        return(DATA_ERROR);
    }
    *samps_read = dz->ssampsread;
    *inputbuf_end  = dz->bigfbuf + dz->ssampsread;
    *step              = -(*step);          /* REVERSE STEP */
    dz->flbufptr[0] = orig_sbuf + ((*step) * 2 * dz->wanted);
    return(FINISHED);
}

/****************************** BAKTRAK_TO_CORRECT_BIGBUF ***********************/

int baktrak_to_correct_bigbuf(int bigbufs_baktrak,int *samps_read,float **inputbuf_end,int *drnk_bufno,dataptr dz)
{
    int exit_status;
    *drnk_bufno -= bigbufs_baktrak;
    if((exit_status = read_samps(dz->bigfbuf,dz)) < 0) {
        sprintf(errstr,"Problem reading buffer in baktrak_to_correct_bigbuf().\n");
        return(SYSTEM_ERROR);
    }
    if(dz->ssampsread == 0) {
        sprintf(errstr,"Problem reading buffer in baktrak_to_correct_bigbuf().\n");
        return(DATA_ERROR);
    }
    *samps_read = dz->ssampsread;
    *inputbuf_end = dz->bigfbuf + dz->ssampsread;
    return(FINISHED);
}

/****************************** INVERT_STEP_FWD_TO_STEP_BACK_AT_END_OF_FILE ***********************/

int invert_step_fwd_to_step_back_at_end_of_file(int *step,int reflect_off_start,int drnk_bufno,dataptr dz)
{
    if(reflect_off_start) {         /* IF OFF END OF FILE */
        sprintf(errstr,"Step turned out ot be too large.\n");
        return(DATA_ERROR);              /* REFLECTED FROM BOTH ENDS !! */
    }
    *step      = -(*step);                  /* REFLECT FROM END OF FILE */
    dz->flbufptr[0]  += (*step) * 2 * dz->wanted;
    if(dz->flbufptr[0] < dz->bigfbuf && drnk_bufno == 0) {
        sprintf(errstr,"Step turned out ot be too large.\n");
        return(DATA_ERROR);             /* REFLECTED FROM BOTH ENDS !! */
    }
    return(FINISHED);
}

/****************************** ADVANCE_TO_CORRECT_BIGBUF ***********************/

int advance_to_correct_bigbuf(float **inputbuf_end,int *samps_read,int *drnk_bufno,dataptr dz)
{
    int exit_status;
    if(*drnk_bufno < 0) {
        sprintf(errstr,"advance_to_correct_bigbuf: drnk_bufno = %d (less than zero)\n",*drnk_bufno);
        return(PROGRAM_ERROR);
    }
    while(dz->flbufptr[0] >= *inputbuf_end) {       /* ADVANCE ALONG FILE */
        if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
            return(exit_status);
        *samps_read = dz->ssampsread;
        if(dz->ssampsread == 0)
            break;
        *inputbuf_end = dz->bigfbuf + dz->ssampsread;
        (*drnk_bufno)++;
        dz->flbufptr[0] -= dz->big_fsize;
    }
    return(FINISHED);
}

/****************************** INVERT_STEP_FWD_TO_STEP_BACK ***********************/

int invert_step_fwd_to_step_back
(int *step,float *orig_sbuf,int *samps_read,float **inputbuf_end,int reflect_off_start,int drnk_bufno,dataptr dz)
{
    int exit_status;
    if(reflect_off_start) {
        sprintf(errstr,"Step turned out ot be too large.\n");
        return(DATA_ERROR);             /* REFLECTED FROM BOTH ENDS!! */
    }
    if(sndseekEx(dz->ifd[0],drnk_bufno * dz->buflen,0)<0) {
        sprintf(errstr,"Seek problem in invert_step_fwd_to_step_back().\n");
        return(SYSTEM_ERROR);
    }
    if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)  {
        sprintf(errstr,"Problem reading buffer in invert_step_fwd_to_step_back().\n");
        return exit_status;
    }
    if(dz->ssampsread == 0) {
        sprintf(errstr,"Problem reading buffer in invert_step_fwd_to_step_back().\n");
        return(PROGRAM_ERROR);
    }
    *samps_read = dz->ssampsread;
    *inputbuf_end = dz->bigfbuf + dz->ssampsread;
    *step             = -(*step);   /* REVERSE STEP */
    dz->flbufptr[0] = orig_sbuf + ((*step) * 2 * dz->wanted);
    return(FINISHED);
}

/****************************** SPECWEAVE ***************************/

int specweave(dataptr dz)
{
    int exit_status;
    int vc;
    int samps_to_write, n = 0;
    float *insbuf  = dz->flbufptr[0];
    float *outsbuf = dz->flbufptr[2];
    unsigned int wrapsamps = dz->iparam[WEAVE_BAKTRAK] * dz->wanted;
    if((exit_status = read_first_inbuf(dz))<0)
        return(exit_status);
    for(;;) {
        for(vc = 0; vc < dz->wanted; vc++)
            outsbuf[vc] = insbuf[vc];
        insbuf += dz->iparray[WEAVE_WEAV][n] * dz->wanted;
        if(insbuf < dz->bigfbuf) {
            sprintf(errstr,"Backtracking failure in specweave().\n");
            return(PROGRAM_ERROR);
        }
        if(insbuf >= dz->flbufptr[1]) {
            if((exit_status = wrap_samps(wrapsamps,&insbuf,dz))<0)
                return(exit_status);
            if(exit_status == FINISHED) {
                outsbuf += dz->wanted;
                break;
            }
        }
        if(++n>=dz->itemcnt)
            n = 0;
        if((outsbuf += dz->wanted)>= dz->flbufptr[3]) { /* outbuf full */
            if((exit_status = write_exact_samps(dz->flbufptr[2],dz->buflen,dz))<0)
                return(exit_status);
            outsbuf = dz->flbufptr[2];
        }
    }
    if(outsbuf != dz->flbufptr[2]) {
        samps_to_write = outsbuf - dz->flbufptr[2];
        if((exit_status = write_samps(dz->flbufptr[2],samps_to_write,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/****************************** READ_FIRST_INBUF ***************************/

int read_first_inbuf(dataptr dz)
{
    int exit_status;
    if((exit_status = read_samps(dz->flbufptr[0],dz)) < 0) {
        sprintf(errstr,"Failed to read data from infile.\n");
        return(exit_status);
    }
    dz->flbufptr[1]  = dz->flbufptr[0] + dz->ssampsread;    /* mark current end of inbuf */
    return(FINISHED);
}

/****************************** WRAP_SAMPS ***************************/

int wrap_samps(unsigned int wrapsamps,float **insbuf,dataptr dz)
{
    int exit_status;
    memmove((char *)dz->bigfbuf,(char *)dz->flbufptr[4],(size_t)(wrapsamps * sizeof(float)));
    if((exit_status = read_samps(dz->flbufptr[0],dz)) < 0)
        return exit_status;
    if(dz->ssampsread == 0)
        return(FINISHED);                                               /* no more data in infile */
    dz->flbufptr[1] = dz->flbufptr[0] + dz->ssampsread;                     /* mark current end of inbuf */
    if((*insbuf -= dz->big_fsize) >= dz->flbufptr[1])/* must have weaved off end of infile */
        return(FINISHED);
    return(CONTINUE);
}

/*************************** COPY_1ST_WINDOW ***********************/

int copy_1st_window(dataptr dz)
{
    memmove((char *)dz->flbufptr[1],(char *)dz->flbufptr[0],(size_t)(dz->wanted * sizeof(float)));
    dz->flbufptr[0] += dz->wanted;
    dz->flbufptr[1] += dz->wanted;
    return(FINISHED);
}
