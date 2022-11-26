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



int  set_up_mix(dataptr dz);
int  mix_syntax_check(dataptr dz);
int  mix_preprocess(dataptr dz);
int  mmix(dataptr dz);
int  mix_syntax_check(dataptr dz);
int  mix_level_check(double *normalisation,dataptr dz);
int  create_mixdummy(dataptr dz);
int      mix_twisted(dataptr dz);

int  mix_gain(dataptr dz);
int  mix_timewarp(dataptr dz);
int  mix_spacewarp(dataptr dz);
int  mix_shufl(dataptr dz);

int  do_time_manip(dataptr dz);
int  do_time_and_name_copy(dataptr dz);
int  do_name_reverse(dataptr dz);
int  do_name_and_time_reverse(dataptr dz);
int  do_name_freeze(dataptr dz);
int  do_name_and_time_freeze(dataptr dz);
int  randomise_names(dataptr dz);
int  get_maxwordsize(int **maxwordsize,dataptr dz);
int  output_mixfile_lines(int *maxwordsize,dataptr dz);
int  read_new_filename(char *filename,dataptr dz);

int  mix_twisted(dataptr dz);
int  check_new_filename(char *filename,dataptr dz);
int  mix_sync(dataptr dz);
int  mix_sync_a_mixfile(dataptr dz);
int  open_file_and_retrieve_props(int filecnt,char *filename,int *srate,dataptr dz);
int  open_file_retrieve_props_open(int filecnt,char *filename,int *srate,dataptr dz);
int  synchronise_mix_attack(dataptr dz);
int  get_level(char *thisword,double *level);
int  sync_and_output_mixfile_line
         (int n,char *filename,int max_namelen,int max_timeword,double timestor,double gain,dataptr dz);
int  retime_the_lines(double *timestor,int maxpostdec,dataptr dz);
int  timesort_mixfile(double *timestor,dataptr dz);

int  mixtwo_preprocess(dataptr dz);
int  mixtwo(dataptr dz);
//TW NEW
int  mixmany(dataptr dz);

int  mix_cross(dataptr dz);
int  mixcross_preprocess(dataptr dz);
int  mix_interl(dataptr dz);
int  mix_inbetween(dataptr dz);
int  get_inbetween_ratios(dataptr dz);

int  open_file_and_retrieve_props(int filecnt,char *filename,int *srate,dataptr dz);
int  cross_stitch(dataptr dz);

//TW NEW
int  do_automix(dataptr dz);
int  addtomix(dataptr dz);
int  panmix(dataptr dz);
int  mix_model(dataptr dz);
int reset_up_mix(dataptr dz);
