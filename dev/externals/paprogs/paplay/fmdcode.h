/*
 * Copyright (c) 1983-2013 Richard Dobson and Composers Desktop Project Ltd
 * http://people.bath.ac.uk/masrwd
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
 
/* fmdcode.h */

/*
 Channel order is WXYZ,RSTUV,KLMNOPQ
 
 The number of channels defines the order of the soundfield:
 2 channel = UHJ 
 3 channel = h   = 1st order horizontal
 4 channel = f   = 1st order 3-D
 5 channel = hh  = 2nd order horizontal
 6 channel = fh  = 2nd order horizontal + 1st order height (formerly
                                                            called 2.5 order)
 7 channel = hhh = 3rd order horizontal
 8 channel = fhh = 3rd order horizontal + 1st order height
 9 channel = ff  = 2nd order 3-D
 11 channel = ffh = 3rd order horizontal + 2nd order height
 16 channel = fff = 3rd order 3-D
 
 
 Horizontal   Height  Soundfield   Number of    Channels
 order 	      order 	  type      channels 	
 1 	         0 	       horizontal 	  3 	    WXY
 1 	         1 	      full-sphere 	  4 	    WXYZ
 2 	         0 	       horizontal 	  5 	    WXY....UV
 2 	         1 	       mixed-order    6 	    WXYZ...UV
 2 	         2 	      full-sphere     9 	    WXYZRSTUV
 3           0 	       horizontal 	  7 	    WXY....UV.....PQ
 3 	         1         mixed-order 	  8 	    WXYZ...UV.....PQ
 3 	         2 	       mixed-order 	 11 	    WXYZRSTUV.....PQ
 3 	         3 	      full-sphere 	 16 	    WXYZRSTUVKLMNOPQ
 */

typedef struct abf_samp {
	float W;
	float X;
	float Y;
	float Z;
    float R;
    float S;
    float T;
	float U;
    float V;
} ABFSAMPLE;

typedef void (*fmhcopyfunc)(ABFSAMPLE*,const float*);

typedef void (*fmhdecodefunc)(const ABFSAMPLE*, float*,unsigned int);
//void bfdcode4(float *inbuf,long numframes);
void fmhcopy_3(ABFSAMPLE* abf,const float*buf);
void fmhcopy_4(ABFSAMPLE* abf,const float*buf);
void fmhcopy_5(ABFSAMPLE* abf,const float*buf);
void fmhcopy_6(ABFSAMPLE* abf,const float*buf);
void fmhcopy_7(ABFSAMPLE* abf,const float*buf);
void fmhcopy_8(ABFSAMPLE* abf,const float*buf);
void fmhcopy_9(ABFSAMPLE* abf,const float*buf);
void fmhcopy_11(ABFSAMPLE* abf,const float*buf);
void fmhcopy_16(ABFSAMPLE* abf,const float*buf);


// i1 = inphase 1st order, etc
void fm_i1_mono(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i1_stereo(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i1_square(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i2_square(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i1_quad(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i2_quad(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i1_pent(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i2_pent(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i1_surr(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i2_surr(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes);
void fm_i1_surr6(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i2_surr6(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes);
void dm_i1_surr(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes);
void dm_i1_surr6(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes);
void dm_i2_surr(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes);
void dm_i2_surr6(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes);
void fm_i1_hex(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i2_hex(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i1_oct1(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i2_oct1(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i1_oct2(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i2_oct2(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i1_cube(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i2_cube(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i1_cubex(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
void fm_i2_cubex(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes);
