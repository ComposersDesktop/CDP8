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
#include <stdlib.h>
#include <sfsys.h>
#include <osbind.h>

static int advance_func(int *current_pos_in_buf,dataptr dz);

/**************************** DISTORT_OMT ******************************
 *
 * keep 'keep' cycles, omit 'omit' cycles.
 */

int distort_omt(int *current_pos_in_buf,int inital_phase,dataptr dz)
{
	int exit_status;
	register int n;
	int i = *current_pos_in_buf;
	float *b = dz->sampbuf[0];
	int keep = dz->iparam[DISTORTO_KEEP] - dz->iparam[DISTORTO_OMIT];
	switch(inital_phase) {
	case(1):
		for(n=0;n<keep;n++) {
			while(b[i]>=0.0) {
				if((exit_status = advance_func(&i,dz))!=CONTINUE)
					return(exit_status);
			}
			while(b[i]<=0.0) {
				if((exit_status = advance_func(&i,dz))!=CONTINUE)
					return(exit_status);
			}
		}
		for(n=0;n<dz->iparam[DISTORTO_OMIT];n++) {
			while(b[i]>=0.0) {
				b[i] = 0;
				if((exit_status = advance_func(&i,dz))!=CONTINUE)
					return(exit_status);
			}
			while(b[i]<=0.0) {
				b[i] = 0.0;
				if((exit_status = advance_func(&i,dz))!=CONTINUE)
					return(exit_status);
			}
		}
		break;
	case(-1):
		for(n=0;n<keep;n++) {
			while(b[i]<=0.0) {
				if((exit_status = advance_func(&i,dz))!=CONTINUE)
					return(exit_status);
			}
			while(b[i]>=0.0) {
				if((exit_status = advance_func(&i,dz))!=CONTINUE)
					return(exit_status);
			}
		}
		for(n=0;n<dz->iparam[DISTORTO_OMIT];n++) {
			while(b[i]<=00) {
				b[i] = 0.0;
				if((exit_status = advance_func(&i,dz))!=CONTINUE)
					return(exit_status);
			}
			while(b[i]>=0.0) {
				b[i] = 0.0;
				if((exit_status = advance_func(&i,dz))!=CONTINUE)
					return(exit_status);
			}
		}
		break;
	}
	*current_pos_in_buf = i;
	return(CONTINUE);
}

/******************************** ADVANCE_FUNC ***************************/

int advance_func(int *current_pos_in_buf,dataptr dz)
{
	int exit_status;
	if(++(*current_pos_in_buf) >= dz->ssampsread) {
		if(dz->ssampsread > 0) {
			if((exit_status = write_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
				return(exit_status);
		}
		if(dz->samps_left <= 0)
			return(FINISHED);
		if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
			return(exit_status);
		*current_pos_in_buf = 0;
	}
	return(CONTINUE);
}
