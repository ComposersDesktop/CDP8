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



/* floatsam version: no changes */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <texture.h>
#include <cdpmain.h>

#include <sfsys.h>

/********************* GETTING DATA VALUES VIA RAND PERMUTATIONS *************/



static int  delete_all_other_motifs(motifptr thismotif,dataptr dz);
static int  delete_motifs_before(motifptr thismotif,dataptr dz);
static int  delete_motifs_beyond(motifptr thismotif);

/***************************** TEXTURE 1 ********************************
 * (1)  Generating random permutations of sets of values, and delivering
 *              the next value from one of these.
 * (2)  Scattering values within a range.
 */

/*************************** MAKE_TEXTURE *********************************/

int make_texture(dataptr dz)
{
    int exit_status;
    unsigned int texflag = dz->tex->txflag;
    display_virtual_time(0,dz);
    if(texflag & ISHARM) {
        if(texflag & IS_CLUMPED) {
            if((exit_status = do_clumped_hftexture(dz))<0)
                return(exit_status);
        } else {
            if((exit_status = do_simple_hftexture(dz))<0)
                return(exit_status);
        }
    } else {
        if((exit_status = do_texture(dz))<0)
            return(exit_status);
    }
    if((exit_status = delete_all_other_motifs(dz->tex->timeset,dz))<0)
        return(exit_status);
    return(FINISHED);
}

/****************************** GETVALUE **********************************
 *
 * Get value of a parameter, either from table, or fixed value.
 */

int getvalue(int paramhi,int paramlo,double thistime,int z,double *val,dataptr dz)
{
    int exit_status;
    double range;
    int is_swap = 0;
    if(dz->is_int[paramhi] || dz->is_int[paramlo]) {
        sprintf(errstr,"getvalue() called on integer parameter %d or %d\n",paramhi,paramlo);
        return(PROGRAM_ERROR);
    }
    if(dz->brk[paramhi]) {
        if((exit_status = read_value_from_brktable(thistime,paramhi,dz))<0)
            return(exit_status);
    }
    if(dz->brk[paramlo]) {
        if((exit_status = read_value_from_brktable(thistime,paramlo,dz))<0)
            return(exit_status);
    }
    if((range = dz->param[paramhi] - dz->param[paramlo])<0.0) {
        swap(&dz->param[paramhi],&dz->param[paramlo]);
        range = -range;
        is_swap = 1;
    }
    if(flteq(range,0.0)) {
        *val = dz->param[paramhi];
        if(is_swap)
            swap(&dz->param[paramhi],&dz->param[paramlo]);
        return(FINISHED);
    }
    if((exit_status = pscatx(range,dz->param[paramlo],z,val,dz))<0)
        return(exit_status);
    if(is_swap)
        swap(&dz->param[paramhi],&dz->param[paramlo]);
    return(FINISHED);
}

/****************************** IGETVALUE **********************************
 *
 * Get integer value of a parameter, either from table, or fixed value.
 */

int igetvalue(int paramhi,int paramlo,double thistime,int z,int *ival,dataptr dz)
{
    int   exit_status;
    int  irange;
    int   a, is_swap = 0;

    if(!dz->is_int[paramhi] || !dz->is_int[paramlo]) {
        sprintf(errstr,"igetvalue() called on non-integer parameter %d or %d\n",paramhi,paramlo);
        return(PROGRAM_ERROR);
    }
    if(dz->brk[paramhi]) {
        if((exit_status = read_value_from_brktable(thistime,paramhi,dz))<0)
            return(exit_status);
    }
    if(dz->brk[paramlo]) {
        if((exit_status = read_value_from_brktable(thistime,paramlo,dz))<0)
            return(exit_status);
    }

    if((irange = dz->iparam[paramhi] - dz->iparam[paramlo])<0) {
        iiswap(&dz->iparam[paramhi],&dz->iparam[paramlo]);
        irange = -irange;
        is_swap = 1;
    }
    if(irange==0) {
        *ival = (int)dz->iparam[paramlo];
        if(is_swap)
            iiswap(&dz->iparam[paramhi],&dz->iparam[paramlo]);
        return(FINISHED);
        /* OCT 1997 replace ****
           } else if((exit_status = doperm(irange,z,&a,dz))<0)
        */
        /* BY */
    } else if((exit_status = doperm(irange+1,z,&a,dz))<0)
        /* because the range of an integer variable between(say) 3 & 7 is INCLUSIVELY 5, not 7-3) */
        return(exit_status);
    a = (int)(a + dz->iparam[paramlo]);
    *ival = a;
    if(is_swap)
        iiswap(&dz->iparam[paramhi],&dz->iparam[paramlo]);
    return(FINISHED);
}

/*************************** DELETE_ALL_OTHER_MOTIFS **********************************
 *
 * Deletes all other motifs in motif list.
 */

int delete_all_other_motifs(motifptr thismotif,dataptr dz)
{
    int exit_status;
    motifptr here = thismotif->next;
    if((exit_status = delete_motifs_before(thismotif,dz))<0)
        return(exit_status);
    if(here!=(motifptr)0) {
        if((exit_status = delete_motifs_beyond(here))<0)
            return(exit_status);
    }
    dz->tex->motifhead = thismotif;
    return(exit_status);
}

/********************** DELETE_MOTIFS_BEYOND ******************************
 *
 * Deletes motif-space from this space onwards.
 */

int delete_motifs_beyond(motifptr thismotif)
{
    motifptr start = thismotif;
    while(thismotif->next!=(motifptr)0)
        thismotif = thismotif->next;
    while(thismotif!=start) {
        thismotif = thismotif->last;
        free(thismotif->next);
    }
    thismotif->next = (motifptr)0;
    return(FINISHED);
}

/*********************** DELETE_MOTIFS_BEFORE ***************************
 *
 * Kill all motifs BEFORE this one in the motif list.
 */

int delete_motifs_before(motifptr thismotif,dataptr dz)
{
    motifptr here = dz->tex->motifhead;
    while(here!=thismotif) {
        if((here = here->next)==(motifptr)0) {
            sprintf(errstr,"Problem in delete_motifs_before()\n");
            return(PROGRAM_ERROR);
        }
        delete_notes_here_and_beyond(here->last->firstnote);
        free(here->last);
        here->last = (motifptr)0;
    }
    dz->tex->motifhead = thismotif;
    return(FINISHED);
}
