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
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <modeno.h>
#include <arrays.h>
#include <distort.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>

static int do_shuffle(float *inbuf,int *obufpos,dataptr dz);
static int do_shuffle_crosbuf(int current_buf,int *obufpos,int cycleno_in_group_at_bufcros,dataptr dz);

/**************************** DISTORT_SHUF ******************************
 *
 * Distort file by shuffling GROUPS of cycles.
 */

int distort_shuf
(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,dataptr dz)
{
	int exit_status;
	register int i = *current_pos_in_buf;
	register int n;
	int cycleno_in_group_at_bufcros = -1;
	int cyclecnt = dz->iparam[DISTORTS_CYCLECNT] * dz->iparam[DISTORTS_DMNCNT];
	float *inbuf  = dz->sampbuf[*current_buf];
	for(n=0;n<cyclecnt;n++) {
		dz->lparray[DISTORTS_STARTCYC][n] = i;
		switch(initial_phase) {
		case(1):
			while(inbuf[i]>=0.0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			while(inbuf[i]<=0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			break;
		case(-1):
			while(inbuf[i]<=0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			while(inbuf[i]>=0) {
				if(++i >= dz->ssampsread) {
					if((exit_status = change_buff(&inbuf,&cycleno_in_group_at_bufcros,current_buf,dz))!=CONTINUE)
						return(exit_status);
					cycleno_in_group_at_bufcros = n;
					i = 0;
				}
			}
			break;
		}
	}
	dz->lparray[DISTORTS_STARTCYC][n] = i;
	if(cycleno_in_group_at_bufcros >= 0)
		exit_status = do_shuffle_crosbuf(*current_buf,obufpos,cycleno_in_group_at_bufcros,dz);
	else
		exit_status = do_shuffle(inbuf,obufpos,dz);
	if(exit_status<0)
		return(exit_status);
	*current_pos_in_buf = i;
	(*cnt)++;
	return(CONTINUE);
}

/*************************** DO_SHUFFLE **************************/

int do_shuffle(float *inbuf,int *obufpos,dataptr dz)
{
	/*RWD need this! */
	int exit_status;

	float *outbuf = dz->sampbuf[2];
	register int outbufpos = *obufpos, k, m, n;
	int  grpbase, thiscycle;
	for(n=0;n<dz->iparam[DISTORTS_IMGCNT];n++) {
		grpbase = dz->iparray[DISTORTS_MAP][n] * dz->iparam[DISTORTS_CYCLECNT];
		for(m=0;m<dz->iparam[DISTORTS_CYCLECNT];m++) {
			thiscycle = grpbase + m;
			k = dz->lparray[DISTORTS_STARTCYC][thiscycle];
			while(k < dz->lparray[DISTORTS_STARTCYC][thiscycle+1]) {
				outbuf[outbufpos] = inbuf[k++];
				if(++outbufpos >= dz->buflen) {
					/* RWD 4:2002 added exit_status check */
					exit_status = write_samps(outbuf,dz->buflen,dz);

					if(exit_status < 0)
						return exit_status;

					outbufpos = 0;
				}
			}		
		}
	}
	*obufpos = outbufpos;
	return(FINISHED);
}

/********************** DO_SHUFFLE_CROSBUF *************************/

int do_shuffle_crosbuf(int current_buf,int *obufpos,int cycleno_in_group_at_bufcros,dataptr dz)
{
	int exit_status;	/*RWD 4:2002 */
	float *outbuf = dz->sampbuf[2];
	register int k, m, n, outbufpos = *obufpos;
	int  grpbase, thiscycle;
	float *inbuf;
	for(n=0;n<dz->iparam[DISTORTS_IMGCNT];n++) {
		grpbase = dz->iparray[DISTORTS_MAP][n] * dz->iparam[DISTORTS_CYCLECNT];
		for(m=0;m<dz->iparam[DISTORTS_CYCLECNT];m++) {
			thiscycle = grpbase + m;
			k = dz->lparray[DISTORTS_STARTCYC][thiscycle];
			if(thiscycle == cycleno_in_group_at_bufcros) {
				inbuf= dz->sampbuf[!current_buf];
				while(k < dz->buflen) {
					outbuf[outbufpos] = inbuf[k++];
					if(++outbufpos >= dz->buflen) {
						/* RWD 4:2002 added error check */
						exit_status = write_samps(outbuf,dz->buflen,dz);
						if(exit_status < 0)
							return exit_status;
						outbufpos = 0;
					}
				}
				inbuf = dz->sampbuf[current_buf];
				k = 0;
			} else {
				if(thiscycle < cycleno_in_group_at_bufcros)
					inbuf = dz->sampbuf[!current_buf];
				else
					inbuf = dz->sampbuf[current_buf];
			}
			while(k < dz->lparray[DISTORTS_STARTCYC][thiscycle+1]) {
				outbuf[outbufpos] = inbuf[k++];
				if(++outbufpos >= dz->buflen) {
					/*RWD as above */
					exit_status = write_samps(outbuf,dz->buflen,dz);
					if(exit_status < 0)
						return exit_status;

					outbufpos = 0;
				}
			}		
		}
	}
	*obufpos = outbufpos;
	return(FINISHED);
}
