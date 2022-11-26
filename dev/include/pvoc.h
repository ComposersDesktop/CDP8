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



#define IS_GROUCHO_COMPILE      (1)

/* Functions in TTPVOC.C */

#define DEFAULT_PVOC_CHANS      (1024)
#define DEFAULT_WIN_OVERLAP     (3)
#define VERY_BIG_INT            (100000000)
#define MAX_PVOC_CHANS          (16380)
#define PVOC_CONSTANT_A         (8.0)
#define SAMP_TIME_STEP          (2000)

/* Functions in MXFFT.C */

int fft_(float *, float *,int,int,int,int);
int fftmx(float *,float *,int,int,int,int,int,int *,float *,float *,float *,float *,int *,int[]);
int reals_(float *,float *,int,int);
