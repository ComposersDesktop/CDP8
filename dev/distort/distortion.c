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



/* floatsam version*/
#include <stdio.h>
#include <stdlib.h>
/*RWD*/
#include <memory.h>

#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <distort.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>

static int  cyccnt(int *current_pos_in_buf,int *current_buf,int initial_phase,dataptr dz);

/*************************** GET_INITIAL_PHASE **********************/

int get_initial_phase(int *initial_phase,dataptr dz)
{
	float *b = dz->sampbuf[0];
	int current_pos_in_buf = 0;
	while(b[current_pos_in_buf]==0 && current_pos_in_buf < dz->ssampsread)
		current_pos_in_buf++;
	if(current_pos_in_buf >= dz->ssampsread) {
		sprintf(errstr,"1st buffer is ALL ZEROES: Cannot proceed.\n");
		return(GOAL_FAILED);
	}
	if(b[current_pos_in_buf] > 0)
		*initial_phase = 1;
	else
		*initial_phase = -1;
	return(FINISHED);
}

/*************************** PROCESS_WITH_SWAPPED_BUFS_ON_SINGLE_HALF_CYCLES ************************/

int process_with_swapped_bufs_on_single_half_cycles(dataptr dz)  /* [distortion0] */	 
{
	int exit_status;
	int current_buf = 0;
	int lastzero = 0;
	/*int*/float cyclemax = 0.0;
	if((exit_status = read_samps(dz->sampbuf[current_buf],dz))<0)
		return(exit_status);
	current_buf = !current_buf;
	while(dz->samps_left > 0) {
		if((exit_status = read_samps(dz->sampbuf[current_buf],dz))<0)
			return(exit_status);
		current_buf = !current_buf;
		switch(dz->process) {
		case(DISTORT): 	exit_status = do_distort(current_buf,0,&lastzero,&cyclemax,dz);	break;
		default:
			sprintf(errstr,"Unknown case in process_with_swapped_bufs_on_single_half_cycles()\n");
			return(PROGRAM_ERROR);
		}
		if(exit_status<0)
			return(exit_status);
		if((exit_status = write_samps(dz->sampbuf[current_buf],dz->buflen,dz))<0)
			return(exit_status);
	}
	current_buf = !current_buf; 
	switch(dz->process) {
	case(DISTORT): 	exit_status = do_distort(current_buf,1,&lastzero,&cyclemax,dz); break;
	default:
		sprintf(errstr,"Unknown case in process_with_swapped_bufs_on_single_half_cycles()\n");
		return(PROGRAM_ERROR);
	}
	if(exit_status<0)
		return(exit_status);
	if(dz->ssampsread > 0)
		return write_samps(dz->sampbuf[current_buf],dz->ssampsread,dz);
	return FINISHED;
}

/*************************** PROCESS_WITH_SWAPPED_BUFS_ON_FULL_CYCLES ************************/

int process_with_swapped_bufs_on_full_cycles(dataptr dz)  /* [distortion2] */
{
	int exit_status;
	int current_pos_in_buf, cnt = 0;
	int  buffer_overrun, initial_phase, current_buf = 0;
	double thistime;
	display_virtual_time(0,dz);
	if((exit_status = read_samps(dz->sampbuf[current_buf],dz))<0)
		return(exit_status);
	if((exit_status = get_initial_phase(&initial_phase,dz))<0)
		return(exit_status);
	current_pos_in_buf = 0;
	do {
		thistime = (double)(dz->total_samps_read - dz->ssampsread + current_pos_in_buf)/(double)dz->infile->srate;
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
		buffer_overrun = FALSE;
		switch(dz->process) {
		case(DISTORT_ENV):	
			exit_status = distort_env(&current_buf,initial_phase,&current_pos_in_buf,&buffer_overrun,&cnt,dz);
			break;
		case(DISTORT_REV):	
			exit_status = distort_rev(&current_buf,initial_phase,&current_pos_in_buf,&buffer_overrun,&cnt,dz);
			break;
		default:
			sprintf(errstr,"Unknown case in process_with_swapped_bufs_on_full_cycles()\n");
			return(PROGRAM_ERROR);
		}
	} while(exit_status == CONTINUE);
	if(exit_status<0)
		return(exit_status);
	if(cnt==0) {
		sprintf(errstr,"source sound too short to attempt this process.\n");
		return(GOAL_FAILED);
	}
//TW DELETED IN UPDATED CODE
	if(current_pos_in_buf > 0)
		return write_samps(dz->sampbuf[current_buf],current_pos_in_buf,dz);
	return(FINISHED);
}

/***************************** PROCESS_ON_SINGLE_BUF_WITH_PHASE_DEPENDENCE *****************************/

int process_on_single_buf_with_phase_dependence(dataptr dz)
{
	int exit_status;
	int initial_phase;
	int current_pos_in_buf = 0;
	double  thistime;
	display_virtual_time(0,dz);
	if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
		return(exit_status);
	if((exit_status = get_initial_phase(&initial_phase,dz))<0)
		return(exit_status);
	do {
		thistime = (double)(dz->total_samps_read - dz->ssampsread + current_pos_in_buf)/(double)dz->infile->srate;
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
		exit_status = distort_omt(&current_pos_in_buf,initial_phase,dz);
	} while(exit_status==CONTINUE);
		;
	return(exit_status);
}

/**************** PROCESS_WITH_SWAPPED_BUFS_ON_FULL_CYCLES_WITH_NEWSIZE_OUTPUT_AND_SKIPCYCLES  *************************/

int process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles(float *outbuf,int skip_param,dataptr dz)
{
	int exit_status;
	int current_buf = 0, initial_phase, previous_cycle_crossed_bufs = FALSE;
	int obufpos = 0, current_pos_in_buf = 0;
	int cpinbf, iobufpos;
	int cnt = 0;
	double  thistime;
	int lastcycle_len = 0, lastcycle_start;

	display_virtual_time(0,dz);
	if((exit_status = read_samps(dz->sampbuf[current_buf],dz))<0)
		return(exit_status);
	if((exit_status = get_initial_phase(&initial_phase,dz))<0)
		return(exit_status);
	if(dz->vflag[IS_DISTORT_SKIP] && (exit_status = skip_initial_cycles(&current_pos_in_buf,&current_buf,initial_phase,skip_param,dz))<0)
		return(exit_status);
	do {
		thistime = (double)(dz->total_samps_read - dz->ssampsread + current_pos_in_buf)/(double)dz->infile->srate;
		if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
			return(exit_status);
		switch(dz->process) {
		case(DISTORT_AVG):
			cpinbf = (int)current_pos_in_buf;
			exit_status = distort_avg(&current_buf,initial_phase,&obufpos,&cpinbf,&cnt,dz);
			current_pos_in_buf = (int)cpinbf;
			break;
		case(DISTORT_SHUF):	
			exit_status = distort_shuf(&current_buf,initial_phase,&obufpos,&current_pos_in_buf,&cnt,dz);
			break;
		case(DISTORT_RPT):
		case(DISTORT_RPT2):
			exit_status = distort_rpt(&current_buf,initial_phase,&obufpos,&current_pos_in_buf,&cnt,
				dz->iparam[DISTRPT_CYCLECNT],&lastcycle_len,&lastcycle_start,&previous_cycle_crossed_bufs,dz);
			break;
		case(DISTORT_RPTFL):
			exit_status = distort_rpt_frqlim(&current_buf,initial_phase,&obufpos,&current_pos_in_buf,&cnt,
				dz->iparam[DISTRPT_CYCLECNT],dz);
			break;
		case(DISTORT_INTP):
			exit_status = distort_rpt(&current_buf,initial_phase,&obufpos,&current_pos_in_buf,&cnt,
		   								   1,&lastcycle_len,&lastcycle_start,&previous_cycle_crossed_bufs,dz);
			break;
		case(DISTORT_DEL):
			switch(dz->mode) {
			case(DELETE_IN_STRICT_ORDER):
				exit_status = distort_del(&current_buf,&current_pos_in_buf,initial_phase,&obufpos,&cnt,dz);
				break;
			case(KEEP_STRONGEST):
			case(DELETE_WEAKEST):
				exit_status = distort_del_with_loudness(&current_buf,initial_phase,&obufpos,&current_pos_in_buf,&cnt,dz);
				break;
			}
			break;
		case(DISTORT_RPL):
			iobufpos = (int)obufpos;
			exit_status = distort_rpl(&current_buf,initial_phase,&iobufpos,&current_pos_in_buf,&cnt,dz);
			obufpos = (int)iobufpos;
			break;
		case(DISTORT_TEL):
			iobufpos = (int)obufpos;
			cpinbf = (int)current_pos_in_buf;
			exit_status = distort_tel(&current_buf,initial_phase,&iobufpos,&cpinbf,&cnt,dz);
			obufpos = (int)iobufpos;
			current_pos_in_buf = (int)cpinbf;
			break;
		case(DISTORT_FLT):
			cnt = 1;	
			iobufpos = (int)obufpos;
			exit_status = distort_flt(&current_buf,initial_phase,&iobufpos,&current_pos_in_buf,dz);
			obufpos = (int)iobufpos;
			break;
		default:
			sprintf(errstr,"Unknown case in process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles()\n");
			return(PROGRAM_ERROR);
		}
	} while(exit_status == CONTINUE);
	if(exit_status <0)
		return(exit_status);
	if(cnt==0) {
		sprintf(errstr,"source sound too short to attempt this process.\n");
		return(GOAL_FAILED);
	}
	if(obufpos > 0)
		return write_samps(outbuf,obufpos,dz);
	return(FINISHED);
}

/***************************** PROCESS_WITH_SWAPPED_BUF_TO_SWAPPED_OUTBUFS *****************************/

int process_with_swapped_buf_to_swapped_outbufs(dataptr dz)
{
	int exit_status;
	int current_buf = 0, output_phase = 1 /*, cyclemax = 0*/;
	float cyclemax = 0.0;
	int lastzero = 0, endsample;
	int no_of_half_cycles = 0;
	int startindex, startmarker, endindex;
	display_virtual_time(0L,dz);
	if((exit_status = read_samps(dz->sampbuf[current_buf],dz))<0)
		return(exit_status);
	if(dz->ssampsread <=0) {
		sprintf(errstr,"No data found in input soundfile.\n");
		return(DATA_ERROR);
	}
	current_buf = !current_buf;
	while(dz->samps_left > 0) {
		read_samps(dz->sampbuf[current_buf],dz);
		current_buf = !current_buf;
		switch(dz->process) {
		case(DISTORT_MLT):
		case(DISTORT_DIV): 
			exit_status = mdistort(!LAST,&lastzero,&endsample,&output_phase,current_buf,&cyclemax,
								&no_of_half_cycles,&startindex,&startmarker,&endindex,dz);
			break;
		default:
			sprintf(errstr,"Unknown case in process_with_swapped_buf_to_swapped_outbufs()\n");
			return(PROGRAM_ERROR);
		}
		if(exit_status<0)
			return(exit_status);
		if((exit_status = write_samps(dz->sampbuf[current_buf+2],dz->buflen,dz))<0)
			return(exit_status);
		memset((char *)dz->sampbuf[current_buf+2],0,dz->buflen * sizeof(float));
	}
	current_buf = !current_buf; 
	switch(dz->process) {
	case(DISTORT_MLT): 
	case(DISTORT_DIV):
		exit_status = mdistort(LAST,&lastzero,&endsample,&output_phase,current_buf,&cyclemax,
							&no_of_half_cycles,&startindex,&startmarker,&endindex,dz); 
		break;
	default:
		sprintf(errstr,"Unknown case in process_with_swapped_buf_to_swapped_outbufs()\n");
		return(PROGRAM_ERROR);
	}
	if(exit_status<0)
		return(exit_status);
	if(endsample > 0) 
		return write_samps(dz->sampbuf[current_buf+2],endsample,dz);
	return(FINISHED);
}

/******** PROCESS_WITH_SWAPPED_BUFS_ON_FULL_CYCLES_WITH_OPTIONAL_PRESCALE *******/

int process_with_swapped_bufs_on_full_cycles_with_optional_prescale(dataptr dz)
{
	int exit_status;
	int initial_phase;
	int cnt = 0;
//TW FIXED -> SAMPS
	int bufpos = 0, last_total_samps_read = 0;
	int prescale_param, current_buf = 0;
	display_virtual_time(0,dz);
	if((exit_status = read_samps(dz->sampbuf[current_buf],dz))<0)
		return(exit_status);		
	switch(dz->process) {
	case(DISTORT_HRM):	prescale_param = DISTORTH_PRESCALE; break;
	case(DISTORT_FRC):	prescale_param = DISTORTF_PRESCALE; break;
	default:
		sprintf(errstr,"Unknown case: process_with_swapped_bufs_on_full_cycles_with_optional_prescale()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->vflag[IS_PRESCALED])
		prescale(current_buf,prescale_param,dz);
	if((exit_status = get_initial_phase(&initial_phase,dz))<0)
		return(exit_status);
	exit_status = CONTINUE;
	switch(dz->process) {
	case(DISTORT_HRM):
		while(exit_status==CONTINUE) {
//TW FIXED-> SAMPS
			exit_status = distorth(&bufpos,initial_phase,&last_total_samps_read,&current_buf,dz);
			cnt++;
		}
		break;
	case(DISTORT_FRC):
		while(exit_status==CONTINUE) {
//TW FIXED-> SAMPS
			exit_status = distortf(&bufpos,initial_phase,&last_total_samps_read,&current_buf,dz);
			cnt++;
		}
		break;
	}
	if(exit_status<0)
		return(exit_status);
	if(cnt <= 1) {
		sprintf(errstr,"source sound too short to attempt this process.\n");
		return(GOAL_FAILED);
	}
	if(bufpos > 0)
		return write_samps(dz->sampbuf[current_buf],bufpos,dz);
	return(FINISHED);
}

/**************************** PRESCALE *****************************/

/* prescaling added by rwd */

void prescale(int current_buf,int prescale_param,dataptr dz)
{
	int i;
	double temp;
	for(i=0;i<dz->ssampsread;i++) {
		temp = (double)dz->sampbuf[current_buf][i];
		dz->sampbuf[current_buf][i] = (float)/* round*/(temp * dz->param[prescale_param]); /*RWD added cast */
	}
}

/**************************** COP_OUT **************************/

int cop_out(int i, int j, int last_total_samps_read, dataptr dz)
{
	j = last_total_samps_read + i - j + 1;
	fprintf(stdout,"WARNING: Program assumes maximum wavelength is %lf secs.\n",MAXWAVELEN);
	fprintf(stdout,"WARNING: Wavelength too long at infile time %lf secs.\n",(double)j/(double)dz->infile->srate);
	return(GOAL_FAILED);
}

/***************************** PROCESS_FILE **************************/

int process_cyclecnt(dataptr dz)
{
	int exit_status;
	/*long i = 0;*/
	int current_buf = 0;
	int current_pos_in_buf = 0;
	int initial_phase;
	dz->itemcnt = 0;
	if((exit_status = read_samps(dz->sampbuf[current_buf],dz))<0)
		return(exit_status);
	get_initial_phase(&initial_phase,dz);
	while(cyccnt(&current_pos_in_buf,&current_buf,initial_phase,dz))
		dz->itemcnt++;
	sprintf(errstr,"%d cycles found.\n",dz->itemcnt);
	return(FINISHED);
}

/************************** CYCCNT *************************/

int cyccnt(int *current_pos_in_buf,int *current_buf,int initial_phase,dataptr dz)
{
	int exit_status;
	register int i = *current_pos_in_buf;
	float *b = dz->sampbuf[*current_buf];
	int buffer_overrun = FALSE;
	switch(initial_phase) {
	case(1):
		while(b[i]>=0) {
			if(++i >= dz->ssampsread) {
				if((exit_status = change_buf(current_buf,&buffer_overrun,&b,dz))!=CONTINUE)
					return(FINISHED);
				i = 0;
			}
		}
		while(b[i]<=0) {
			if(++i >= dz->ssampsread) {
				if((exit_status = change_buf(current_buf,&buffer_overrun,&b,dz))!=CONTINUE)
					return(FINISHED);
				i = 0;
			}
		}
		break;
	case(-1):
		while(b[i]<=0) {
			if(++i >= dz->ssampsread) {
				if((exit_status = change_buf(current_buf,&buffer_overrun,&b,dz))!=CONTINUE)
					return(FINISHED);
				i = 0;
			}
		}
		while(b[i]>=0) {
			if(++i >= dz->ssampsread) {
				if((exit_status = change_buf(current_buf,&buffer_overrun,&b,dz))!=CONTINUE)
					return(FINISHED);
				i = 0;
			}
		}
		break;
	}
	*current_pos_in_buf = i;
	return(CONTINUE);
}
