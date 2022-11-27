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
#include <texture.h>
#include <globcon.h>
#include <arrays.h>

#include <osbind.h>

static void rndpermm(int k,int pindex,int **permm,dataptr dz);
static void insert(int m,int t,int pindex,int **permm,dataptr dz);
static void prefix(int m,int pindex,int **permm,dataptr dz);
static void shuflup(int k,int pindex, int **permm,dataptr dz);

/******************************** DOPERM *********************************
 *
 * Either deliver next item in a permuted set, or (if set exhausted)
 * generate a randon permutation of the set and deliver its first
 * element.
 *
 * (1)  If this permset does not have the same length as the last...
 * (2)    Set a new permset length.
 * (3)    If a permset already exists (i.e. this is not first)
 *        Free the permset space, and malloc a new space of correct size,
 * (4)    Create a random permutation of elements into the permset.
 * (5)    Set the pointer-to-the-set to 0.
 * (6)    Set the size of the previous perm (which will be this one,
 *        next time!) to this permset size.
 * (6a) Whether or not a new perm set has been set up...
 * (7)    Get the value of the next item in the current permset,
 *        incrementing the set pointer in the process.
 * (8)    If this has the same value as the previous-one-output-from-doperm
 *        increment the repetition counter.
 * (9)    Otherwise set the repetition counter to 1.
 * (10)   If the set pointer has run beyond the permset.
 * (11)     reset the pointer to 0.
 * (12)     generate a new random perm (of same length).
 * (13) Continue this process if the number of permissible repetitions
 *      is exceeded.
 * (14) Set the value for lastpermval, for next call.
 * (15) Return the value.
 */

int doperm(int k,int pindex,int *val,dataptr dz)
{
    int i, OK;
    int **permm = dz->tex->perm;
    if(pindex >= PERMCNT) {
        sprintf(errstr,"doperm(): Perm index %d too big (max = %d)\n",pindex,PERMCNT);
        return(PROGRAM_ERROR);
    }
    if(k <= 1) {
        if(k>=0) {
            *val = 0;
            return(FINISHED);
        } else {
            sprintf(errstr,"doperm(): Invalid perm count %d\n",k);
            return(PROGRAM_ERROR);
        }
    }
    if((k*dz->iparray[TXRPT][pindex])!=dz->iparray[TXLASTPERMLEN][pindex]) {        /* 1 */
        dz->iparray[TXPERMLEN][pindex] = (int)(k * dz->iparray[TXRPT][pindex]); /* 2 */
        if(permm[pindex]   != (int *)0)                                                                                 /* 3 */
            free(permm[pindex]);
        if((permm[pindex] = (int *)malloc(dz->iparray[TXPERMLEN][pindex] * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for permutation %d\n",pindex+1);
            return(MEMORY_ERROR);
        }
        rndpermm(k,pindex,permm,dz);                                                                                    /* 4 */
        dz->iparray[TXPERMINDEX][pindex]   = 0;                                                                 /* 5 */
        dz->iparray[TXLASTPERMLEN][pindex] = dz->iparray[TXPERMLEN][pindex];    /* 6 */
    }
    do {                                            /* 6a */
        OK = 1;
        i = *(permm[pindex] + (dz->iparray[TXPERMINDEX][pindex]++));                    /* 7 */
        if(i==dz->iparray[TXLASTPERMVAL][pindex]) {                                                             /* 8 */
            dz->iparray[TXREPETCNT][pindex]++;
            if(dz->iparray[TXREPETCNT][pindex]>dz->iparray[TXRPT][pindex]) {
                dz->iparray[TXREPETCNT][pindex] = dz->iparray[TXRPT][pindex];
                OK = 0;
            }
        } else {
            dz->iparray[TXREPETCNT][pindex]=1;
        }
        if(dz->iparray[TXPERMINDEX][pindex]>=dz->iparray[TXPERMLEN][pindex]) {  /* 10 */
            dz->iparray[TXPERMINDEX][pindex] = 0;                                                           /* 11 */
            rndpermm(k,pindex,permm,dz);                                                                            /* 12 */
        }
    }while(!OK);                                                                                                                            /* 13 */
    dz->iparray[TXLASTPERMVAL][pindex] = i;                                                                         /* 14 */
    *val = i;
    return(FINISHED);                                                                                                                       /* 15 */
}

/*************************** RNDPERMM *******************************
 *
 * Produce a permutation of k objects and store it in permutation-store
 * number 'pindex'.
 *
 * (1)  permlen is the number of objects (k) times the number of repetitions
 *          permitted (rpt[pindex]) = N.
 * (2)  This is the efficient algorithm for distributing N objects into
 *              a random perm.
 * (3)  As we really only have k objects, we take the value%rpt in each
 *              permutation location.
 *              e.g. 3 objects repeated 3 times would give us a random perm of
 *              nine objects such as
 *              5 6 2 8 3 0 1 7 4
 *              applying %3 to this we get
 *              2 0 2 2 0 0 1 1 1
 *              i.e. a perm of 3 objects with no more than 3 consecutive repets
 *              of any one object!!
 */

void rndpermm(int k,int pindex,int **permm,dataptr dz)
{
    int n, t;
    for(n=0;n<dz->iparray[TXPERMLEN][pindex];n++) {         /* 1 */
        t = (int)(drand48() * (double)(n+1));           /* 2 */
        if(t==n) {
            prefix(n,pindex,permm,dz);
        } else {
            insert(n,t,pindex,permm,dz);
        }
    }
    for(n=0;n<dz->iparray[TXPERMLEN][pindex];n++)           /* 3 */
        *(permm[pindex]+n) = (int)(*(permm[pindex]+n) % k);
}

/***************************** INSERT **********************************
 *
 * Insert the value m AFTER the T-th element in permm[pindex].
 */

void insert(int m,int t,int pindex,int **permm,dataptr dz)
{
    shuflup(t+1,pindex,permm,dz);
    *(permm[pindex]+t+1) = m;
}

/***************************** PREFIX ************************************
 *
 * Insert the value m at start of the permutation permm[pindex].
 */

void prefix(int m,int pindex,int **permm,dataptr dz)
{
    shuflup(0,pindex,permm,dz);
    *permm[pindex] = m;
}

/****************************** SHUFLUP ***********************************
 *
 * move set members in permm[pindex] upwards, starting from element k.
 */

void shuflup(int k,int pindex, int **permm,dataptr dz)
{
    int n, *i;
    int z = dz->iparray[TXPERMLEN][pindex] - 1;
    i = permm[pindex] + z;
    for(n = z;n > k;n--) {
        *i = *(i-1);
        i--;
    }
}
