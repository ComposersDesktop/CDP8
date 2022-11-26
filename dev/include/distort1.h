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
#define DFLT_DISTORTER_MULT             (.01)
#define DFLT_DISTORTER_DEPTH    (1.0)
#define DISTORTER_TABLEN                (1028)
#define ENDBIT_SPLICE                   (256.0)
#define PULSE_FRQLIM                    (6000.0)
#define PULSE_DBLIM                             (-60)
#define ORIG_PULSENV    (0)
#define PULSENV                 (1)
#define PULSTRN                 (2)

int     process_with_swapped_bufs_on_single_half_cycles(dataptr dz);
int             process_with_swapped_bufs_on_full_cycles(dataptr dz);
int             process_on_single_buf_with_phase_dependence(dataptr dz);
int     process_with_swapped_buf_to_swapped_outbufs(dataptr dz);
int             process_with_swapped_bufs_on_full_cycles_with_optional_prescale(dataptr dz);
int             process_with_swapped_bufs_on_full_cycles_with_newsize_output_and_skipcycles
                        (float *outbuf,int skip_param,dataptr dz);
int             process_cyclecnt(dataptr dz);
int     two_infiles_interleave_process(dataptr dz);
int     two_infiles_resize_process(dataptr dz);
int     distort_pitch(dataptr dz);

int             skip_initial_cycles(int *current_pos,int *current_buf,int phase,int skip_paramno,dataptr dz);
int             get_initial_phase(int *initial_phase,dataptr dz);

int     do_distort(int tthis,int is_last,int *lastzero,float *cyclemax,dataptr dz);
int     distort_env(int *current_buf,int initial_phase,int *current_pos,int *buffer_overrun,int *cnt,dataptr dz);
int     distort_rev(int *current_buf,int initial_phase,int *current_pos,int *buffer_overrun,int *cnt,dataptr dz);
int             distort_avg(int *current_buf,int inital_phase,int *outbufcnt,int *inbufcnt,int *cnt,dataptr dz);
int     distort_omt(int *inbufcnt,int inital_phase,dataptr dz);
int     mdistort(int is_last,int *lastzero,int *endsample,int *output_phase,int current_buf,float *cyclemax,
                int *no_of_half_cycles,int *startindex,int *startmakrer,int *endindex,dataptr dz);
int     distorth(int *bufpos,int phase,int *last_total_samps_read,int *current_buf,dataptr dz);
int     distortf(int *bufpos,int phase,int *last_total_samps_read,int *current_buf,dataptr dz);
int     distort_shuf(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,dataptr dz);
int     distort_rpt(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,
                                int cyclecnt,int *lastcycle_len,int *lastcycle_start,int *last_bufcros,dataptr dz);
int             distort_del(int *current_buf,int *current_pos_in_buf,int phase,int *obufpos,int *cnt,dataptr dz);
int             distort_del_with_loudness(int *current_buf,int phase,int *obufpos,int *current_pos_in_buf,int *cnt,dataptr dz);
int             distort_rpl(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,dataptr dz);
int     distort_tel(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,dataptr dz);
int     distort_flt(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,dataptr dz);
int     two_infiles_interleave_process(dataptr dz);
int     two_infiles_resize_process(dataptr dz);
int     reset_distorte_modes(dataptr dz);

void    prescale(int current_buf,int prescale_param,dataptr dz);
int             cop_out(int i,int j,int last_total_bytes_read,dataptr dz);
int     change_buf(int *current_buf,int *buffer_overrun,float **buf,dataptr dz);
int             change_buff(float **b,int *cycleno_in_group_at_bufcros,int *current_buf,dataptr dz);
int     get_full_cycle(float *b,int *buffer_overrun,int *current_buf,int initial_phase,int *current_pos,
                int cyclecnt_param,dataptr dz);
int     output_val(float value,int *obufpos,dataptr dz);
int     distort_overload(dataptr dz);
int             overload_preprocess(dataptr dz);
int             preprocess_pulse(dataptr dz);
int             do_pulsetrain(dataptr dz);
int     distort_rpt_frqlim(int *current_buf,int initial_phase,int *obufpos,int *current_pos_in_buf,int *cnt,
                                int cyclecnt,dataptr dz);
