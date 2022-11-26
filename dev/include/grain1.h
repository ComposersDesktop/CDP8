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



/* GRAIN1.H */
/*RWD 6:2001 purged decls of local statics */

int  process_grains(dataptr dz);
int  copygrain(int start,int end,int bufno,int *obufposition,dataptr dz);
int  crosbuf_grain_type3(int grainstart,int grainend,int bufno,int *obufpos,dataptr dz);
int  do_the_grain(int ibufpos,int *graincnt,int bufno,int gapcnt,int *obufpos,int grainstart,
                                int crosbuf,int chans,int *grainadjusted,double samptotime,int *is_first_grain,dataptr dz);
//TW REVISED
int  deal_with_last_grains(int ibufpos,int bufno,int *graincnt,int grainstart,int *grainadjusted,
                                int *obufpos,int crosbuf,int chans,double samptotime,int *is_first_grain,dataptr dz);
int  swap_to_otherfile_and_readjust_counters(dataptr dz);
int timestretch_iterative(dataptr dz);
int     assess_grains(dataptr dz);
int  grain_preprocess(int gate_paramno,dataptr dz);
int  timestretch_iterative2(dataptr dz);
int  timestretch_iterative3(dataptr dz);
int  grab_noise_and_expand(dataptr dz);
int      grev(dataptr dz);
