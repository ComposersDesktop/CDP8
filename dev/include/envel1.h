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



int     process_envelope(dataptr dz);
//int   setup_envel_windowsize(dataptr dz);
int     read_env_ramp_brk(char *filename,dataptr dz);
//int   read_env_create_file(char *str,dataptr dz);
int     envelope_warp(float **env,float **envend,dataptr dz);
int     envreplace(float *env,float **envend,float *origenv,float *origend);
int     extract_env_from_sndfile(int *bufcnt,int *envcnt,float **env,float **envend,int fileno,dataptr dz);
int     impose_envel_on_sndfile(float *env,int envcnt,int bufcnt,int fileno,dataptr dz);
int     create_envelope(int *cnt,dataptr dz);
int     envel_preprocess(dataptr dz);
int     create_sndbufs_for_envel(dataptr dz);
int     apply_brkpnt_envelope(dataptr dz);

int     buffers_in_sndfile(int buffer_size,int fileno,dataptr dz);
int     windows_in_sndfile(int fileno,dataptr dz);

//int   create_pluck_buffers(dataptr dz);
int     envelope_pluck(dataptr dz);
int     envelope_tremol(dataptr dz);
int     generate_samp_windowsize(fileptr thisfile,dataptr dz);
int             do_grids(dataptr dz);
int             envsyn(dataptr dz);     /*TW March 2004 */
