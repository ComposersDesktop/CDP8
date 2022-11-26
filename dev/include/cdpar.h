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



#define CHECKBUTTON                     (0)
#define SWITCHED                        (1)
#define LINEAR                          (2)
#define LOG                                     (3)
#define PLOG                            (4)
#define FILE_OR_VAL                     (5)
#define FILENAME                        (6)
#define NUMERIC                         (7)
#define GENERIC_FILENAME        (8)
#define STRING_A                        (10)
#define STRING_B                        (11)
#define STRING_C                        (12)
#define STRING_D                        (13)
#define TIMETYPE                        (14)
#define SRATE                           (15)
#define TWOFAC                          (16)
#define WAVETYPE                        (17)
#define POWTWO                          (18)

typedef struct applic *aplptr;

int  cdparams(int process,int mode,int filetype,int infilesize,int insams,int srate,
                int channels,int wanted,int wlength,int linecnt,float arate,float frametime,
                double nyquist,double duration);
int   parse_indata(int arc,char *argv[],int *process,int *mode,int *infilecnt,int *filetype,int *infilesize,
                int *insams,int *srate,int *channels,int *wanted,int *wlength,int *linecnt,
                float *arate,float *frametime,double *nyquist,double *duration);
int  does_process_accept_conflicting_srates(int process);
int  get_param_names(int process,int mode,int total_params,int user_params,aplptr ap);
int  setup_flagnames(int process,int mode,int bare_flags,aplptr ap);
int  establish_display(int process,int mode,int total_params,float frametime,double duration,aplptr ap);

int  set_legal_param_structure(int application_no,int mode, aplptr ap);
int      set_legal_option_and_variant_structure(int process,int mode,aplptr ap);
int  deal_with_formants(int process,int mode,int channels,aplptr ap);
int  deal_with_special_data(int process,int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap);
int  get_param_ranges(int process,int mode,int total_params,double nyquist,float frametime,
                float arate,int srate,int wlength,int insams,
                int channels,int wanted,int filetype,int linecnt,double duration,aplptr ap);
