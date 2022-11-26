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
 
/* fmhfuncs.c */
#include "fmdcode.h"

/* TODO: expand to handle numframes frames? */

void fmhcopy_3(ABFSAMPLE* abf,const float*buf)
{
    abf->W = *buf++;
    abf->X = *buf++;
    abf->Y = *buf;
}

void fmhcopy_4(ABFSAMPLE* abf,const float*buf)
{
    abf->W = *buf++;
    abf->X = *buf++;
    abf->Y = *buf++;
    abf->Z = *buf;
}

void fmhcopy_5(ABFSAMPLE* abf,const float*buf)
{
    abf->W = *buf++;
    abf->X = *buf++;
    abf->Y = *buf++;
    abf->U = *buf++;
    abf->V = *buf;
}

void fmhcopy_6(ABFSAMPLE* abf,const float*buf)
{
    abf->W = *buf++;
    abf->X = *buf++;
    abf->Y = *buf++;
    abf->Z = *buf++;
    abf->U = *buf++;
    abf->V = *buf;
}
// following discard 3rd order chans
void fmhcopy_7(ABFSAMPLE* abf,const float*buf)
{
    abf->W = *buf++;
    abf->X = *buf++;
    abf->Y = *buf++;
    abf->U = *buf++;
    abf->V = *buf;
}

void fmhcopy_8(ABFSAMPLE* abf,const float*buf)
{
    abf->W = *buf++;
    abf->X = *buf++;
    abf->Y = *buf++;
    abf->Z = *buf++;
    abf->U = *buf++;
    abf->V = *buf;
}
// these identical for 2nd order horiz max, but may be expanded later!
void fmhcopy_9(ABFSAMPLE* abf,const float*buf)
{
    abf->W = *buf++;
    abf->X = *buf++;
    abf->Y = *buf++;
    abf->Z = *buf++;
    abf->R = *buf++;
    abf->S = *buf++;
    abf->T = *buf++;
    abf->U = *buf++;
    abf->V = *buf;
}

void fmhcopy_11(ABFSAMPLE* abf,const float*buf)
{
    abf->W = *buf++;
    abf->X = *buf++;
    abf->Y = *buf++;
    abf->Z = *buf++;
    abf->R = *buf++;
    abf->S = *buf++;
    abf->T = *buf++;
    abf->U = *buf++;
    abf->V = *buf;
}
void fmhcopy_16(ABFSAMPLE* abf,const float*buf)
{
    abf->W = *buf++;
    abf->X = *buf++;
    abf->Y = *buf++;
    abf->Z = *buf++;
    abf->R = *buf++;
    abf->S = *buf++;
    abf->T = *buf++;
    abf->U = *buf++;
    abf->V = *buf;
}

/********** DECODE FUNCS *************/
/* TODO: complete support for numframes > 1 */

void fm_i1_mono(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
    int i;	
	float *p_out = outbuf;
	double aw;
    for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.7071;
		*p_out++ = (float) aw;  		  
	}
}

void fm_i1_stereo(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
    int i;	
	float *p_out = outbuf;
	double aw,ay;
    for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.7071;
		ay = (double) inbuf->Y * 0.5;		
		
		*p_out++ = (float) (aw +  ay);  
		*p_out++ = (float) (aw  - ay);  
	}
}

void fm_i1_square(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.35355;
		ax = (double) inbuf->X * 0.17677;
		ay = (double) inbuf->Y * 0.17677;		
		
		*p_out++ = (float) (aw + ax + ay);  //FL
		*p_out++ = (float) (aw - ax + ay);  //RL
		*p_out++ = (float) (aw - ax - ay);  //RR
		*p_out++ = (float) (aw + ax - ay);  //FR
	}
}
void fm_i2_square(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;		
	float *p_out = outbuf;	
	double aw,ax,ay,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.3536;
		ax = (double) inbuf->X * 0.2434;
		ay = (double) inbuf->Y * 0.2434;		
		av = (double) inbuf->V * 0.0964;
		*p_out++ = (float) (aw + ax + ay + av);  //FL
		*p_out++ = (float) (aw - ax + ay - av ); //RL
		*p_out++ = (float) (aw - ax - ay + av);  //RR
		*p_out++ = (float) (aw + ax - ay - av);  //FR
	}
}
/* ditto, RLRL layout for WAVEX */
void fm_i1_quad(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.35355;
		ax = (double) inbuf->X * 0.17677;
		ay = (double) inbuf->Y * 0.17677;		
		
		*p_out++ = (float) (aw + ax + ay);  //FL
        *p_out++ = (float) (aw + ax - ay);  //FR
		*p_out++ = (float) (aw - ax + ay);  //RL
		*p_out++ = (float) (aw - ax - ay);  //RR
		
	}
}
void fm_i2_quad(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;		
	float *p_out = outbuf;	
	double aw,ax,ay,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.3536;
		ax = (double) inbuf->X * 0.2434;
		ay = (double) inbuf->Y * 0.2434;		
		av = (double) inbuf->V * 0.0964;
		*p_out++ = (float) (aw + ax + ay + av);  //FL
        *p_out++ = (float) (aw + ax - ay - av);  //FR
		*p_out++ = (float) (aw - ax + ay - av ); //RL
		*p_out++ = (float) (aw - ax - ay + av);  //RR
		
	}
}


//front pair angle 72deg
void fm_i1_pent(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.2828;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		
		*p_out++ = (float) (aw + (ax*0.1618) + (ay*0.1176));  //FL
		*p_out++ = (float) (aw - (ax*0.0618) + (ay*0.1902));  
		*p_out++ = (float) (aw - (ax*0.2));  
		*p_out++ = (float) (aw - (ax*0.0618) - (ay*0.1902));  
        *p_out++ = (float) (aw + (ax*0.1618) - (ay*0.1176)); //FR
	}
}

void fm_i2_pent(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;	
	double aw,ax,ay,au,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.2828;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;	
        au = (double) inbuf->U;
        av = (double) inbuf->V;
		
		*p_out++ = (float) (aw + (ax*0.2227) + (ay*0.1618) + (au*0.0238) + (av * 0.0733));  
		*p_out++ = (float) (aw - (ax*0.0851) + (ay*0.2619) - (au*0.0624) - (av * 0.0453));  
		*p_out++ = (float) (aw - (ax*0.2753)               + (au * 0.0771)              );  
		*p_out++ = (float) (aw - (ax*0.0851) - (ay*0.2619) - (au*0.0624) + (av * 0.0453));  
        *p_out++ = (float) (aw + (ax*0.2227) - (ay*0.1618) + (au*0.0238) - (av * 0.0733));
	}
}

/* FMH only defines 1st order decode */ 
void fm_i1_surr(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		/* TODO: fix this order! */
		*p_out++ = (float) ((aw * 0.169)  + (ax*0.0797) + (ay * 0.0891));   //L
		*p_out++ = (float) ((aw * 0.1635) + (ax*0.0923));                   //C    ///???
		*p_out++ = (float) ((aw * 0.169)  - (ax*0.0797) - (ay * 0.0891));   //R    ///????
		*p_out++ = (float) ((aw * 0.4563) - (ax*0.1259) + (ay * 0.1543));   //LS
        *p_out++ = (float) ((aw * 0.4563) - (ax*0.1259) - (ay * 0.1543));   //RS
	}
}
/* from Bruce Wiggins via Csound */
void fm_i2_surr(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay,au,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		au = (double) inbuf->U;
        av = (double) inbuf->V;
        
		*p_out++ = (float) ((aw * 0.405) + (ax*0.32) + (ay * 0.31)  + (au * 0.085) + (av * 0.125));   //L
        *p_out++ = (float) ((aw * 0.405) + (ax*0.32) - (ay * 0.31)  + (au * 0.085) - (av * 0.125));   //R
		*p_out++ = (float) ((aw * 0.085) + (ax*0.04)                + (au * 0.045)               );   //C
		*p_out++ = (float) ((aw * 0.635) - (ax*0.335) + (ay * 0.28) - (au * 0.08)  + (av * 0.08));    //LS
        *p_out++ = (float) ((aw * 0.635) - (ax*0.335) - (ay * 0.28) - (au * 0.08)  - (av * 0.08));    //RS
	}
}

/* 5.1 versions - silent LFE */
/* FMH only defines 1st order decode */ 
void fm_i1_surr6(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		
		*p_out++ = (float) ((aw * 0.169)  + (ax*0.0797) + (ay * 0.0891));   //L
		*p_out++ = (float) ((aw * 0.1635) + (ax*0.0923));                   //C
		*p_out++ = (float) ((aw * 0.169)  - (ax*0.0797) - (ay * 0.0891));   //R
        *p_out++ = 0.0f;                                                    //LFE
		*p_out++ = (float) ((aw * 0.4563) - (ax*0.1259) + (ay * 0.1543));   //LS
        *p_out++ = (float) ((aw * 0.4563) - (ax*0.1259) - (ay * 0.1543));   //RS
	}
}
/* from Bruce Wiggins via Csound */
void fm_i2_surr6(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay,au,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		au = (double) inbuf->U;
        av = (double) inbuf->V;
        
		*p_out++ = (float) ((aw * 0.405) + (ax*0.32) + (ay * 0.31)  + (au * 0.085) + (av * 0.125));   //L
        *p_out++ = (float) ((aw * 0.405) + (ax*0.32) - (ay * 0.31)  + (au * 0.085) - (av * 0.125));   //R
		*p_out++ = (float) ((aw * 0.085) + (ax*0.04)                + (au * 0.045)               );   //C
        *p_out++ = 0.0f;                                                                              //LFE
		*p_out++ = (float) ((aw * 0.635) - (ax*0.335) + (ay * 0.28) - (au * 0.08)  + (av * 0.08));    //LS
        *p_out++ = (float) ((aw * 0.635) - (ax*0.335) - (ay * 0.28) - (au * 0.08)  - (av * 0.08));    //RS
	}
}

// 1st order 5.0
void dm_i1_surr(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		
        
		*p_out++ = (float) ((aw * 0.4597)  + (ax*0.4536) + (ay * 0.3591));   //L
        *p_out++ = (float)  ((aw * 0.4597)  + (ax*0.4536) - (ay * 0.3591));  //R 
		*p_out++ = 0.0f;                                                     //C
		
		*p_out++ = (float) ((aw * 0.5662) - (ax*0.3681) + (ay * 0.4606));    //LS
        *p_out++ = (float) ((aw * 0.5662) - (ax*0.3681) - (ay * 0.4606));    //RS
    }
}
//1st order 5.1
void dm_i1_surr6(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		
        
		*p_out++ = (float) ((aw * 0.4597)  + (ax*0.4536) + (ay * 0.3591));   //L
        *p_out++ = (float)  ((aw * 0.4597)  + (ax*0.4536) - (ay * 0.3591));  //R 
		*p_out++ = 0.0f;                                                     //C
		*p_out++ = 0.0f;                                                     //LFE
		*p_out++ = (float) ((aw * 0.5662) - (ax*0.3681) + (ay * 0.4606));    //LS
        *p_out++ = (float) ((aw * 0.5662) - (ax*0.3681) - (ay * 0.4606));    //RS
    }
}
// 2nd order 5.0
void dm_i2_surr(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay,au,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		au = (double) inbuf->U;
        av = (double) inbuf->V;
        
		*p_out++ = (float) ((aw * 0.3314)  + (ax*0.4097) + (ay * 0.3487) + (au * 0.0828) + (av*0.1489));  //L
        *p_out++ = (float) ((aw * 0.3314)  + (ax*0.4097) - (ay * 0.3487) + (au * 0.0828) - (av*0.1489));  //R 
		*p_out++ = (float) ((aw * 0.0804)  + (ax * 0.1327));                                              //C
		*p_out++ = (float) ((aw * 0.6025) - (ax*0.3627) + (ay * 0.4089) - (au * 0.0567));                 //LS
        *p_out++ = (float) ((aw * 0.6025) - (ax*0.3627) - (ay * 0.4089) - (au * 0.0567));                 //RS
    }
}
// 2nd order 5.1
void dm_i2_surr6(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay,au,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		au = (double) inbuf->U;
        av = (double) inbuf->V;
        
		*p_out++ = (float) ((aw * 0.3314)  + (ax*0.4097) + (ay * 0.3487) + (au * 0.0828) + (av*0.1489));  //L
        *p_out++ = (float) ((aw * 0.3314)  + (ax*0.4097) - (ay * 0.3487) + (au * 0.0828) - (av*0.1489));  //R 
		*p_out++ = (float) ((aw * 0.0804)  + (ax * 0.1327));                                              //C
		*p_out++ = 0.0f;                                                                                  //LFE
		*p_out++ = (float) ((aw * 0.6025) - (ax*0.3627) + (ay * 0.4089) - (au * 0.0567));                 //LS
        *p_out++ = (float) ((aw * 0.6025) - (ax*0.3627) - (ay * 0.4089) - (au * 0.0567));                 //RS
    }
}


void fm_i1_hex(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.2357;
		ax = (double) inbuf->X * 0.1443;
		ay = (double) inbuf->Y;		
		
		*p_out++ = (float) (aw + ax + (ay * 0.0833));  //FL
		*p_out++ = (float) (aw      + (ay * 0.1667));  //SL
		*p_out++ = (float) (aw - ax + (ay * 0.0833));  //RL
		*p_out++ = (float) (aw - ax - (ay * 0.0833));  //RR
        *p_out++ = (float) (aw      - (ay * 0.1667));  //SR
        *p_out++ = (float) (aw + ax - (ay * 0.0833));  //FR
	}
}
void fm_i2_hex(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;	
	double aw,ax,ay,au,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.2357;
		ax = (double) inbuf->X * 0.1987;
		ay = (double) inbuf->Y;
		au = (double) inbuf->U;
        av = (double) inbuf->V * 0.0556;
		
		*p_out++ = (float) (aw + ax + (ay * 0.1147) + (au * 0.0321) + av);  //FL
		*p_out++ = (float) (aw      + (ay * 0.2294) - (au * 0.0643)     );  //SL
		*p_out++ = (float) (aw - ax + (ay * 0.1147) + (au * 0.0321) - av);  //RL
		*p_out++ = (float) (aw - ax - (ay * 0.1147) + (au * 0.0321) + av);  //RR
        *p_out++ = (float) (aw      - (ay * 0.2294) - (au * 0.0643)     );  //SR
        *p_out++ = (float) (aw + ax - (ay * 0.1147) + (au * 0.0321) - av);  //FR
	}
}

void fm_i1_oct1(const ABFSAMPLE *inbuf,float*outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.1768;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		
		*p_out++ = (float) (aw + (ax * 0.1155) + (ay * 0.0478));  
		*p_out++ = (float) (aw + (ax * 0.0478) + (ay * 0.1155));  
		*p_out++ = (float) (aw - (ax * 0.0478) + (ay * 0.1155));  
		*p_out++ = (float) (aw - (ax * 0.1155) + (ay * 0.0478));  
        *p_out++ = (float) (aw - (ax * 0.231)  - (ay * 0.0957));  
        *p_out++ = (float) (aw - (ax * 0.0478) - (ay * 0.1155));  
        *p_out++ = (float) (aw + (ax * 0.0478) - (ay * 0.1155));  
        *p_out++   = (float) (aw + (ax * 0.1155) - (ay * 0.0478));  
	}
}
void fm_i2_oct1(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;	
	double aw,ax,ay,au,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.17677;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;
		au = (double) inbuf->U * 0.03417;
		av = (double) inbuf->V * 0.03417;
        
		*p_out++ = (float) (aw + (ax * 0.15906) + (ay * 0.06588) + au + av);  
		*p_out++ = (float) (aw + (ax * 0.06588) + (ay * 0.15906) - au + av);  
		*p_out++ = (float) (aw - (ax * 0.06588) + (ay * 0.15906) - au - av);  
		*p_out++ = (float) (aw - (ax * 0.15906) + (ay * 0.06588) + au - av);  
        *p_out++ = (float) (aw - (ax * 0.15906) - (ay * 0.06588) + au + av);  
        *p_out++ = (float) (aw - (ax * 0.06588) - (ay * 0.15906) - au + av);  
        *p_out++ = (float) (aw + (ax * 0.06588) - (ay * 0.15906) - au - av);  
        *p_out++ = (float) (aw + (ax * 0.15906) - (ay * 0.06588) + au - av);  
	}
}

void fm_i1_oct2(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.1768;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;		
		
		*p_out++ = (float) (aw + (ax * 0.125)                 );  
		*p_out++ = (float) (aw + (ax * 0.0884) + (ay * 0.0884));  
		*p_out++ = (float) (aw                 + (ay * 0.125) );  
		*p_out++ = (float) (aw - (ax * 0.0884) + (ay * 0.0884));  
        *p_out++ = (float) (aw - (ax * 0.125)                 );  
        *p_out++ = (float) (aw - (ax * 0.0884) - (ay * 0.0884));  
        *p_out++ = (float) (aw                 - (ay * 0.125) );  
        *p_out++ = (float) (aw + (ax * 0.0884) - (ay * 0.0884));  
	}
}
void fm_i2_oct2(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;	
	double aw,ax,ay,au,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.1768;
		ax = (double) inbuf->X;
		ay = (double) inbuf->Y;
		au = (double) inbuf->U * 0.0482;
		av = (double) inbuf->V * 0.0482;
        
		*p_out++ = (float) (aw + (ax * 0.1721)                 + au     );  
		*p_out++ = (float) (aw + (ax * 0.1217) + (ay * 0.1217)      + av);  
		*p_out++ = (float) (aw                 + (ay * 0.1721) - au     );  
		*p_out++ = (float) (aw - (ax * 0.1217) + (ay * 0.1217)      - av);  
        *p_out++ = (float) (aw - (ax * 0.1721)                 + au     );  
        *p_out++ = (float) (aw - (ax * 0.1217) - (ay * 0.1217)      + av);  
        *p_out++ = (float) (aw                 - (ay * 0.1721) - au     );  
        *p_out++ = (float) (aw + (ax * 0.1217) - (ay * 0.1217)      - av);  
	}
}

/* csound order; low/high anti-clockwise. 
FMH page order, 4 low folowed by 4 high , clockwise! */
void fm_i1_cube(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay,az;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.17677;
		ax = (double) inbuf->X * 0.07216;
		ay = (double) inbuf->Y * 0.07216;
		az = (double) inbuf->Z * 0.07216;
		
		*p_out++ = (float) (aw + ax + ay - az);  // FL low
		*p_out++ = (float) (aw + ax + ay + az);  // FL hi
        
		*p_out++ = (float) (aw - ax + ay - az);  // RL low
		*p_out++ = (float) (aw - ax + ay + az);  //    hi
        
        *p_out++ = (float) (aw - ax - ay - az);  // RR low
        *p_out++ = (float) (aw - ax - ay + az);  //   hi
        
        *p_out++ = (float) (aw + ax - ay - az);  // FR low
        *p_out++ = (float) (aw + ax - ay + az);  //    hi
	}
}
void fm_i2_cube(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;	
	double aw,ax,ay,az,as,at,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.1768;
		ax = (double) inbuf->X * 0.114;
		ay = (double) inbuf->Y * 0.114;
        az = (double) inbuf->Z * 0.114;
        as = (double) inbuf->S * 0.0369;
        at = (double) inbuf->T * 0.0369;
		av = (double) inbuf->V * 0.0369;
        
		*p_out++ = (float) (aw + ax + ay - az - as - at + av); //FL low 
		*p_out++ = (float) (aw + ax + ay + az + as + at + av); //   hi 
		
        *p_out++ = (float) (aw - ax + ay - az + as - at - av); //RL low
		*p_out++ = (float) (aw - ax + ay + az - as + at - av);  
        
        *p_out++ = (float) (aw - ax - ay - az + as + at + av); // RR low
        *p_out++ = (float) (aw - ax - ay + az - as - at + av);  
        
        *p_out++ = (float) (aw + ax - ay - az - as + at - av);  // FR low
        *p_out++ = (float) (aw + ax - ay + az + as - at - av);   
	}
}
/* ditto, wavex order */
/* Front L, front R, Back L, Back R; top Front L, Top Fr R, Top Back L, Top back R */
void fm_i1_cubex(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;
	double aw,ax,ay,az;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.17677;
		ax = (double) inbuf->X * 0.07216;
		ay = (double) inbuf->Y * 0.07216;
		az = (double) inbuf->Z * 0.07216;
		
		*p_out++ = (float) (aw + ax + ay - az);  // FL low
        *p_out++ = (float) (aw + ax - ay - az);  // FR low
        *p_out++ = (float) (aw - ax + ay - az);  // RL low
        *p_out++ = (float) (aw - ax - ay - az);  // RR low
        
		*p_out++ = (float) (aw + ax + ay + az);  // FL hi
        *p_out++ = (float) (aw + ax - ay + az);  // FR hi
		*p_out++ = (float) (aw - ax + ay + az);  // RL hi
        *p_out++ = (float) (aw - ax - ay + az);  // RR hi
	}
}
void fm_i2_cubex(const ABFSAMPLE *inbuf,float *outbuf,unsigned int numframes)
{
	int i;	
	float *p_out = outbuf;	
	double aw,ax,ay,az,as,at,av;
	
	for(i=0;i < numframes; i++){
		aw = (double) inbuf->W * 0.1768;
		ax = (double) inbuf->X * 0.114;
		ay = (double) inbuf->Y * 0.114;
        az = (double) inbuf->Z * 0.114;
        as = (double) inbuf->S * 0.0369;
        at = (double) inbuf->T * 0.0369;
		av = (double) inbuf->V * 0.0369;
        
		*p_out++ = (float) (aw + ax + ay - az - as - at + av);  // FL low
        *p_out++ = (float) (aw + ax - ay - az - as + at - av);  // FR low
        *p_out++ = (float) (aw - ax + ay - az + as - at - av);  // RL low
        *p_out++ = (float) (aw - ax - ay - az + as + at + av);  // RR low
		
        *p_out++ = (float) (aw + ax + ay + az + as + at + av);  // FL  hi 
		*p_out++ = (float) (aw + ax - ay + az + as - at - av);  // FR  hi
		*p_out++ = (float) (aw - ax + ay + az - as + at - av);  // RL  hi 
        *p_out++ = (float) (aw - ax - ay + az - as - at + av);  // RR  hi 
	}
}


#ifdef NOTDEF
void bfdcode4(float *inbuf,long numframes)
{
	int i;	
	float *p_buf = inbuf;	
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		aw = (double)(p_buf[0]) ;
		ax = (double)(p_buf[1]) * 0.707;
		ay = (double)(p_buf[2]) * 0.707;		
		//decode frame
		*p_buf++ = (float)(0.3333 * (aw + ax + ay)); //FL
		*p_buf++ = (float)(0.3333 * (aw + ax - ay)); //FR
		*p_buf++ = (float)(0.3333 * (aw - ax + ay)); //RL
		*p_buf++ = (float)(0.3333 * (aw - ax - ay));  //RR
	}
}


/* handle 3ch in to 4ch out! */

void bfdcode324(float *inbuf,float*outbuf,long numframes)
{
	int i;	
	float *p_buf = inbuf;
	float * p_outbuf = outbuf;
	double aw,ax,ay;
	
	for(i=0;i < numframes; i++){
		int j;
		aw = (double)(*p_buf++) ;
		ax = (double)(*p_buf++) * 0.707;
		ay = (double)(*p_buf++) * 0.707;
        
		//decode frame
		*p_outbuf++ = (float)(0.3333 * (aw + ax + ay));
		*p_outbuf++ = (float)(0.3333 * (aw + ax - ay));
		*p_outbuf++ = (float)(0.3333 * (aw - ax + ay));
		*p_outbuf++ = (float)(0.3333 * (aw - ax - ay));
	}
}
#endif
