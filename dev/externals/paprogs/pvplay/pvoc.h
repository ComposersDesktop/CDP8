/*
 * Copyright (c) 1983-2020  Composers Desktop Project Ltd
 * http://www.composersdesktop.com
 * This file is part of the CDP System.
 * The CDP System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *
 * The CDP System is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with the CDP System; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
 
/* Functions in PVOC.C */

void 	logo(void);
void	warpse(float*,float*,int,double);
void	usage(void);
void	hamming(float*,int,int);
float	*float_array(int);
void	malerr(char*,int);
void	kaiser_(int*,float*,int*,int*,float*);


/* Functions in MXFFT.C */
#ifndef USE_FFTW
void fft_(float *, float *,int,int,int,int);
void fftmx(float *,float *,int,int,int,int,int,int *,float *,float *,float *,float *,int *,int[]);
void reals_(float *,float *,int,int);
#endif

#ifndef min
#define min(a,b)	(((a)<(b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)	(((a)>(b)) ? (a) : (b))
#endif
