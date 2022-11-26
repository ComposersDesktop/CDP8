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
#include <pnames.h>
#include <globcon.h>
#include <cdpmain.h>
#include <house.h>
#include <modeno.h>
#include <sfsys.h>
#include <memory.h>
#include "sfdump.h"

/*************************** HOUSE_BAKUP ******************************/

int house_bakup(dataptr dz) 
{
	int n, exit_status;
	int samps_to_write, secs_to_write, insert_samps, insert_secs;
	int spare_samps, tail, spare_samps_in_secs, samps_written = 0;
	float *buf = dz->bigbuf;
	
	insert_samps = dz->infile->srate * dz->infile->channels;	/* 1 second in samples */
	insert_samps = (int)round(BAKUP_GAP * insert_samps);
	if((insert_secs = insert_samps/F_SECSIZE) * F_SECSIZE != insert_samps)
		insert_secs++;
	insert_samps = insert_secs * F_SECSIZE;

	dz->tempsize = 0;
	for(n=0;n<dz->infilecnt;n++)
		dz->tempsize += dz->insams[n];

	for(n=0;n<dz->infilecnt;n++) {
		sndseekEx(dz->ifd[n],0,0);
		dz->samps_left = dz->insams[n];
		spare_samps = 0;
		while(dz->samps_left > 0) {
			memset((char *)dz->bigbuf,0,dz->buflen * sizeof(float));
			if((dz->ssampsread = fgetfbufEx(buf,dz->buflen,dz->ifd[n],0)) < 0) {
				sprintf(errstr,"Can't read samples from input soundfile %d\n",n+1);
				return(SYSTEM_ERROR);
			}
			dz->samps_left -= dz->ssampsread;

			samps_to_write = dz->ssampsread;	/* Default (full input buff) write all of it */
			spare_samps = dz->buflen - dz->ssampsread;
			if(spare_samps) {					/* with partially full input buff  */
				tail = spare_samps % F_SECSIZE;
				spare_samps_in_secs = spare_samps - tail;
				if(spare_samps_in_secs >= insert_samps) {	/* If empty part of buf exceeds silence-insert-dur */				
					samps_to_write = dz->ssampsread + tail + insert_samps;	/* write ALL silence from buftail */
					if(((samps_to_write/F_SECSIZE) * F_SECSIZE) != samps_to_write) {
						sprintf(errstr,"Error in program file-write logic.\n");
						return(PROGRAM_ERROR);
					}
				} else {
					samps_to_write = dz->buflen;		/* Else, write some of silence from tail of buffer */
					if((exit_status = write_samps(buf,samps_to_write,dz)) < 0)
						return(exit_status);
					dz->total_samps_written += samps_written;
					display_virtual_time(dz->total_samps_written,dz);
					memset((char *)dz->bigbuf,0,dz->buflen * sizeof(float));	
					samps_to_write = insert_samps - spare_samps;
					if((secs_to_write = samps_to_write/F_SECSIZE) * F_SECSIZE != samps_to_write)
						secs_to_write++;					/* rounding up to whole number of sectors */
					samps_to_write = secs_to_write * F_SECSIZE;
				}
			}
			if(samps_to_write > 0) {
				if((exit_status = write_samps(buf, samps_to_write,dz)) < 0)
					return(exit_status);
			}
			dz->total_samps_written += samps_written;
			display_virtual_time(dz->total_samps_written,dz);
		}
		if(spare_samps == 0) {		/* in event of last file write EXACTLY fitting into read buffer */
			memset((char *)dz->bigbuf,0,dz->buflen * sizeof(float));	
			if(insert_samps > 0) {
				if((exit_status = write_samps(buf, insert_samps,dz)) < 0)
					return(exit_status);
			}
			dz->total_samps_written += samps_written;
			display_virtual_time(dz->total_samps_written,dz);
		}
	}
	return FINISHED;
}

