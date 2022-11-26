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



/* FORMANT PARAMS */
#define FREQWISE_FORMANTS                       (0)
#define PICHWISE_FORMANTS                       (1)
#define TOP_OF_LOW_OCTAVE_BANDS         (4)
#define CHAN_ABOVE_LOW_OCTAVES          (8)
#define IGNORE_ACTUAL_CHAN_FRQS         (1)
#define RECTIFY_CHANNEL_FRQ_ORDER       (2)
#define MAX_BANDS_PER_OCT                       (12)    /* semitone is max width */

#define FBAND_DEFAULT                           (4)             /* default no of formant bands */

#define FMNT_BUFMULT    (512)

/* FORMANTS */
int  initialise_specenv(int *arraycnt,dataptr dz);
int  write_formant_descriptor(float **fptr1,float *fptr2,dataptr dz);
int  initialise_specenv2(dataptr dz);
int  get_channel_corresponding_to_frq(int *chan,double thisfrq,dataptr dz);
int      establish_formant_band_ranges(int channels,aplptr ap);
int  read_formantband_data_and_setup_formants(char ***cmdline,int *cmdlinecnt,dataptr dz);
int  initialise_specenv(int *arraycnt,dataptr dz);
int  extract_specenv(int bufptr_no,int storeno,dataptr dz);
int  getspecenvamp(double *thisamp,double thisfrq,int storeno,dataptr dz);
int  extract_specenv_over_partials(int *specenvcnt,double thispitch,int bufptr_no,dataptr dz);
int  setup_formant_params(int fd,dataptr dz);
void print_formant_params_to_screen(dataptr dz);
