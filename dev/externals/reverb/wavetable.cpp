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
 
// Jan 2009  extended random lfo to support gaussian distribution


//////////////////////////////////////////////////////////////////////

#include <math.h>
#include <memory.h>
#include <time.h>
#include "wavetable.h"
#include <assert.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifndef PI
#define PI (3.141592634)
#endif

#ifndef TWOPI
#define TWOPI (2.0 * PI)
#endif

#define LAMBDA (10.0)


/* rand stuff from Csound*/
static const long RANDINT_MAX = 2147483647L;        // 2^^31 - 1 */
static const long BIPOLAR = 0x4fffffff;
static const long RIA = 16807;
static const long RIM = 0x7fffffffL;
static const double dv2_31 = 1.0 / 2147483648.0;
static const long MAXLEN = 0x1000000L;
static const long PHMASK = 0x0ffffffL;
static const double FMAXLEN = (double)(0x1000000L);
// for tpdf - bit of a hack, but seems close enough!
static const long TPDFSHIFT = 0x18000000;
// for gauss, trial and error!
static const long BISHIFT = 0x16000000;



fastlfo::fastlfo()
{
	param.freq = 0.0;
	param.modrange = 0.0;
	m_cur_WaveType = LFO_SINE;
	curfreq = param.freq = 0.0;
	curphase = 0.0;
	incr = 0.0;
	tf = 0;
	kicvt = 0;
	offset= 0.0;
	b_sync = false;
	tpdf = false;
    gauss = false;
    /* for csrand */
    csseed = 12345;
}

double      fastlfo::csrand()  
{ 
    long rval;
    rval =  randint31(csseed);
    csseed = rval;
    return (double) rval / (double) RANDINT_MAX;
}


long fastlfo::init(double srate, double normphase, long seedval, unsigned long ksmps)
{
	long seed;

	m_srate = srate / ksmps;
	twopiovrsr = TWOPI / m_srate;	
	m_inv_srate = 1.0 / m_srate;
	set_WaveType(LFO_SINE);
	phs = (long)(MAXLEN * normphase);
	curphase = TWOPI * normphase;
	if(seedval == 0)
		seed = (long) time(NULL);
	else
		seed = seedval;
	rand = randint31(seed);
	rand = randint31(rand);	
    num1 = 0;
    // get 2nd randon val for diff calc
    // TODO: check this! why << 1 ?
	rand = randint31(rand);
	//num2 = (double)(rand<<1) * dv2_31;
    num2 = (double)((long)((unsigned)rand<<1)-BIPOLAR) * dv2_31;

	dfdmax = (num2 - num1) / FMAXLEN;
	kicvt = (long)((double)MAXLEN / m_srate);
	lastval = 0.0;
	ksamps = ksmps;
	if(ksamps==0)
		ksamps++;
	kcount = 1;
	b_sync = false;
	curphs = (long)(normphase * MAXLEN);
	return seed;
}


// we keep current wavetype; no reset of random values ;
/* phase ranged 0 -1 */
void fastlfo::reset(double phase)
{ 
	curphase  = TWOPI * phase;
	phs =  (long)(MAXLEN * phase);
	curphs = phs; // assume phase offset = 0 here? 
	lastval = 0.0;
	b_sync = false;
	kcount = 1;
}



#define OSC_WRAPPHASE  if(curphase >= TWOPI) curphase -= TWOPI;	\
						if(curphase < 0.0) curphase += TWOPI
// input range 0-1
void fastlfo::sync_phase(double phase,double phaseoffset,double phaseincr)
{
	if(b_sync) {
		curphase = phase * TWOPI;
		offset = phaseoffset * TWOPI;
		incr = phaseincr * TWOPI;
		phs_incr = (long)(phaseincr*ksamps * MAXLEN);
		phs = (long)(phase * MAXLEN);
		phs &= PHMASK;
		phs_offset = (long)(phaseoffset * MAXLEN);
		curphs = phs + phs_offset;
		curphs &= PHMASK;
	}
}

inline double fastlfo::sinetick(void)
{
	double val;
	double thisphase = curphase + offset;	
	while(thisphase >= TWOPI)
		thisphase -= TWOPI;
	if(kcount==ksamps){
		if(!b_sync){
			val = sin(thisphase);
			if(curfreq != param.freq){
				curfreq = param.freq;
				incr = twopiovrsr * param.freq;		
			}
		}
		else
			val = sin(thisphase);
		curphase += incr;
		//OSC_WRAPPHASE;
		if(curphase >= TWOPI) 
			curphase -= TWOPI;	
		if(curphase < 0.0) 
			curphase += TWOPI;
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return lastval;
}

inline double fastlfo::tritick(void)
{
	double val;
	double thisphase = curphase + offset;	
	while(thisphase >= TWOPI)
		thisphase -= TWOPI;
	if(kcount==ksamps){
		if(!b_sync){
			if(curfreq != param.freq){
				curfreq = param.freq;
				incr = twopiovrsr * param.freq;		
			}
		}
		if(thisphase <= PI)	{
			val =   (4.0 * (thisphase * (1.0 / TWOPI) )) - 1.0;
		}
		else {
			val =  3.0 - 4.0 * (thisphase * (1.0 / TWOPI) );
		}
		curphase += incr;
		OSC_WRAPPHASE;
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return 	lastval;
	
}

inline double fastlfo::squaretick(void)
{
	double val;
	double thisphase = curphase + offset;	
	while(thisphase >= TWOPI)
		thisphase -= TWOPI;
	if(kcount==ksamps){
		if(!b_sync){
			if(curfreq != param.freq){
				curfreq = param.freq;
				incr = twopiovrsr * param.freq;		
			}
		}
		if(thisphase <= PI)
			val = 1.0;
		else
			val = -1;
		curphase += incr;
		OSC_WRAPPHASE;
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return 	lastval;
	
}

double fastlfo::sawuptick(void)
{
	double val;
	double thisphase = curphase + offset;	
	while(thisphase >= TWOPI)
		thisphase -= TWOPI;

	if(kcount==ksamps){
		if(!b_sync){
			if(curfreq != param.freq){
				curfreq = param.freq;
				incr = twopiovrsr * param.freq;		
			}
		}
		val =  (2.0 * (thisphase * (1.0 / TWOPI) )) - 1.0;
		curphase += incr;
		OSC_WRAPPHASE;
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return 	lastval;
	
}

inline double fastlfo::sawdowntick(void)
{
	double val;
	double thisphase = curphase + offset;	
	while(thisphase >= TWOPI)
		thisphase -= TWOPI;

	if(kcount==ksamps){
		if(!b_sync){
			if(curfreq != param.freq){
				curfreq = param.freq;
				incr = twopiovrsr * param.freq;		
			}
		}
		val =  1.0 - 2.0 * (thisphase * (1.0 / TWOPI) );
		curphase += incr;
		OSC_WRAPPHASE;
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return 	lastval;
}


// some redundant code with phs and curphs here, but keep both for now!


inline double fastlfo::randomtick(void)
{
	double val;
	double dval = 0.0;
    int i;

	if(kcount==ksamps){
		if(b_sync){
			val = num1 +(double)(curphs * dfdmax);
			phs += phs_incr;
			curphs += phs_incr;
			if(curphs >= MAXLEN){
				long r =  randint31(rand);
				if(tpdf){
					long rr = randint31(r);
                    long ri;
                    unsigned long sum = r + rr;
                    ri = (long)(sum/2);                       
                    r =  ri - TPDFSHIFT;
				}
                else if(gauss){
                    int nrands = 16;
                    long ri = 0;
                    long rr[16];
                    rr[0] = rand;
                    for(i = 1; i < nrands; i++)
                        rr[i] = randint31(rr[i-1]);
                    for(i=0;i < nrands;i++)
                        ri += rr[i]>>4;                        
                    r = ri - BISHIFT;    
                }
                else if(biexp){
                    long rr = r;
                    double dbiexp = 0.0;
                                    
                    dbiexp = 2.0 * (double)rr * dv2_31;
                    while(dbiexp==0.0 || dbiexp == 2.0){                    
                        rr = randint31(rr);                                            
                    }                
                    if(dbiexp > 1.0)
                        dval = -log(2.0-dbiexp) / LAMBDA;
                    if(dbiexp < 1.0)
                        dval = log(dbiexp) / LAMBDA;
                    if( dval > 0.9)
                        dval = 0.9;
                    if(dval < -0.9)
                        dval = -0.9;                                        
                    r =   ((unsigned long)(dval/dv2_31) +BIPOLAR)>>1;                                       
                }
				curphs &= PHMASK;
				rand = r;
				num1 = num2;
				num2 = (double)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;
				dfdmax = (num2-num1) / FMAXLEN;
			}
			if(phs >= MAXLEN)
				phs &= PHMASK;
		}
		else{           
            val = num1 +(double)(phs * dfdmax);
            phs +=(long) (param.freq * kicvt);
            if(phs >= MAXLEN){
                long r =  randint31(rand);
                assert(r>0);
                if(tpdf){
                    long rr = randint31(r);
                    long ri;
                    unsigned long sum = r + rr;
                    ri = (long)(sum/2);                   
                    r =  ri - TPDFSHIFT;                     
                }
                else if(gauss){
                    int nrands = 16;
                    long ri = 0;
                    long rr[16];
                    rr[0] = rand;
                    for(i = 1;i < nrands;i++)
                        rr[i]= randint31(rr[i-1]);
                    for(i=0;i < nrands;i++)
                        ri += rr[i]>>4;                        
                    r = ri - BISHIFT;                        
                }
                else if(biexp){
                    long rr = r;
                    double dbiexp = 0.0;
                                    
                    dbiexp = 2.0 * (double)rr * dv2_31;
                    while(dbiexp==0.0 || dbiexp == 2.0){                    
                        rr = randint31(rr);                                            
                    }                
                    if(dbiexp > 1.0)
                        dval = -log(2.0-dbiexp) / LAMBDA;
                    if(dbiexp < 1.0)
                        dval = log(dbiexp) / LAMBDA;
                    if( dval > 0.9)
                        dval = 0.9;
                    if(dval < -0.9)
                        dval = -0.9;                                        
                    r =   ((unsigned long)(dval/dv2_31) +BIPOLAR)>>1;
                    /* assert(r>0);
                      assert(r < RANDINT_MAX);
                    */
                }            
                phs &= PHMASK;
                rand = r;
                num1 = num2;
                num2 = (double)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;
                /* NB makes uniform rand not really uniform, more a sort of rounded distr
                 * so maybe not ideal for strict plain uniform distr - still while though!
                */
                dfdmax = (num2-num1) / FMAXLEN;                            
            }
		}
        if(gauss)
            val *= 1.85;
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return 	lastval;
}

inline double fastlfo::rand_tpdf_tick(void)
{
	double val;

	if(kcount==ksamps){
		if(b_sync){
			val = num1 +(double)(curphs * dfdmax);
			phs += phs_incr;
			curphs += phs_incr;
			if(curphs >= MAXLEN){
				long r =  randint31(rand);
				
				long rr = randint31(r);
                long ri;
                long sum = r + rr;
                ri = (long)(sum/2);                       
                r =  ri - TPDFSHIFT;
				                              
				curphs &= PHMASK;
				rand = r;
				num1 = num2;
				num2 = (double)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;
				dfdmax = (num2-num1) / FMAXLEN;
			}
			if(phs >= MAXLEN)
				phs &= PHMASK;
		}
		else{           
            val = num1 +(double)(phs * dfdmax);
            /* need this test for audiorate noise; but is it the right test for everywhere? */
            /* time-varying noise rate? */
            if(ksamps > 1)
                phs +=(long) (param.freq * kicvt + 0.5);
            else
                phs = MAXLEN;
            if(phs >= MAXLEN){
                long r =  randint31(rand);                                
                long rr = randint31(r);
                long rsum;
#ifdef _DEBUG
                double d1,d2,dtpdf;
                d1 = ((2.0 *(double)r)  / (double) RANDINT_MAX) - 1.0;
                d2 = ((2.0 *(double)rr) / (double) RANDINT_MAX) - 1.0;
                dtpdf = d1 + d2;
                dtpdf *= 0.5;
#endif
                rand = rr;
                /* bipolar value  =  2*r - 1  or 2* (r - 0.5)*/
                long bipolard2 = 0x40000000;
                r  = (r - bipolard2);
                rr = (rr- bipolard2);
                rsum = r + rr;
                double dsum = (double)rsum  / (double) RANDINT_MAX ;                                                                                          
                phs &= PHMASK; 
                num1 = num2;
                num2 = dsum;
                dfdmax = (num2-num1) / FMAXLEN;                            
            }
		}        
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return 	lastval;
}

inline double fastlfo::rand_gauss_tick(void)
{
	double val;
    int i;

	if(kcount==ksamps){
		if(b_sync){
			val = num1 +(double)(curphs * dfdmax);
			phs += phs_incr;
			curphs += phs_incr;
			if(curphs >= MAXLEN){
				long r =  randint31(rand);				                
                int nrands = 16;
                long ri = 0;
                long rr[16];

                rr[0] = rand;
                for(i = 1; i < nrands; i++)
                    rr[i] = randint31(rr[i-1]);
                for(i=0;i < nrands;i++)
                    ri += rr[i]>>4;                        
                r = ri - BISHIFT;    
                                
				curphs &= PHMASK;
				rand = r;
				num1 = num2;
				num2 = (double)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;
				dfdmax = (num2-num1) / FMAXLEN;
			}
			if(phs >= MAXLEN)
				phs &= PHMASK;
		}
		else{           
            val = num1 +(double)(phs * dfdmax);
            if(ksamps > 1)
                phs +=(long) (param.freq * kicvt + 0.5);
            else
                phs = MAXLEN;
            
            if(phs >= MAXLEN){
                long r =  randint31(rand);                                                
                int nrands = 16;
                long ri = 0;
                long rr[16];

                rr[0] = rand;
                for(i = 1;i < nrands;i++)
                    rr[i]= randint31(rr[i-1]);
                for(i=0;i < nrands;i++)
                    ri += rr[i]>>4;                        
                r = ri - BISHIFT;                        
                                         
                phs &= PHMASK;
                rand = r;
                num1 = num2;
                num2 = (double)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;                
                dfdmax = (num2-num1) / FMAXLEN;                            
            }
		}
        
        val *= 1.85;
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return 	lastval;
}

inline double fastlfo::rand_exp_tick(void)
{
	double val;
	double dval = 0.0;

	if(kcount==ksamps){
		if(b_sync){
			val = num1 +(double)(curphs * dfdmax);
			phs += phs_incr;
			curphs += phs_incr;
			if(curphs >= MAXLEN){
				long r =  randint31(rand);				                
                long rr = r;
                double dbiexp = 0.0;
                                    
                dbiexp = 2.0 * (double)rr * dv2_31;
                while(dbiexp==0.0 || dbiexp == 2.0){                    
                    rr = randint31(rr);                                            
                }                
                if(dbiexp > 1.0)
                    dval = -log(2.0-dbiexp) / LAMBDA;
                if(dbiexp < 1.0)
                    dval = log(dbiexp) / LAMBDA;
                if( dval > 0.9)
                    dval = 0.9;
                if(dval < -0.9)
                    dval = -0.9;                                        
                r =   ((unsigned long)(dval/dv2_31) +BIPOLAR)>>1;                                       
                
				curphs &= PHMASK;
				rand = r;
				num1 = num2;
				num2 = (double)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;
				dfdmax = (num2-num1) / FMAXLEN;
			}
			if(phs >= MAXLEN)
				phs &= PHMASK;
		}
		else{           
            val = num1 +(double)(phs * dfdmax);
            if(ksamps > 1)
                phs +=(long) (param.freq * kicvt + 0.5);
            else
                phs = MAXLEN;            
            if(phs >= MAXLEN){
                long r =  randint31(rand);                                
                long rr = r;
                double dbiexp = 0.0;
                                    
                dbiexp = 2.0 * (double)rr * dv2_31;
                while(dbiexp==0.0 || dbiexp == 2.0){                    
                    rr = randint31(rr);                                            
                }                
                if(dbiexp > 1.0)
                    dval = -log(2.0-dbiexp) / LAMBDA;
                if(dbiexp < 1.0)
                    dval = log(dbiexp) / LAMBDA;
                if( dval > 0.9)
                    dval = 0.9;
                if(dval < -0.9)
                    dval = -0.9;                                        
                r =   ((unsigned long)(dval/dv2_31) +BIPOLAR)>>1;
                                                                  
                phs &= PHMASK;
                rand = r;
                num1 = num2;
                num2 = (double)((long)((unsigned)r<<1)-BIPOLAR) * dv2_31;
                
                dfdmax = (num2-num1) / FMAXLEN;                            
            }
		}        
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return 	lastval;
}


inline double fastlfo::randhtick(void)
{
	double val;
	
	if(kcount==ksamps){
		if(b_sync){
			val = num1;
			phs += phs_incr;
			curphs += phs_incr;
			if(curphs >= MAXLEN){
				long r = randint31(rand);
				if(tpdf){
					long rr = randint31(r);
                    long ri;
                    unsigned long sum = r + rr;
                    ri = (long)(sum/2);                       
                    r =  ri - TPDFSHIFT;
				}
				rand = r;
				num1 = (double)((long)((unsigned)r<<1) - BIPOLAR) * dv2_31;
				curphs &= PHMASK;
			}
			if(phs >= MAXLEN)
				phs &= PHMASK;
		}
		else {
			val = num1;
            if(ksamps > 1)
                phs +=(long) (param.freq * kicvt + 0.5);
            else
                phs = MAXLEN;
			
			if(phs >= MAXLEN){
				long r = randint31(rand);
				if(tpdf){
					long rr = randint31(r);
                    long ri;
                    unsigned long sum = r + rr;
                    ri = (long)(sum/2);                       
                    r =  ri - TPDFSHIFT;
				}
				phs &= PHMASK;
				rand = r;
				num1 = (double)((long)((unsigned)r<<1) - BIPOLAR) * dv2_31;
			}
		}
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return 	lastval;
}


inline double fastlfo::randh_tpdf_tick(void)
{
	double val;
	
	if(kcount==ksamps){
		if(b_sync){
			val = num1;
			phs += phs_incr;
			curphs += phs_incr;
			if(curphs >= MAXLEN){
				long r = randint31(rand);
				
				long rr = randint31(r);
                long ri;
                unsigned long sum = r + rr;
                ri = (long)(sum/2);                       
                r =  ri - TPDFSHIFT;
				
				rand = r;
				num1 = (double)((long)((unsigned)r<<1) - BIPOLAR) * dv2_31;
				curphs &= PHMASK;
			}
			if(phs >= MAXLEN)
				phs &= PHMASK;
		}
		else {
			val = num1;			
			if(ksamps > 1)
                phs +=(long) (param.freq * kicvt + 0.5);
            else
                phs = MAXLEN;
			if(phs >= MAXLEN){
				long r = randint31(rand);
				
				long rr = randint31(r);
                long ri;
                unsigned long sum = r + rr;
                ri = (long)(sum/2);                       
                r =  ri - TPDFSHIFT;
				
				phs &= PHMASK;
				rand = r;
				num1 = (double)((long)((unsigned)r<<1) - BIPOLAR) * dv2_31;
			}
		}
		lastval = val * param.modrange;
		kcount = 1;
	}
	else
		kcount++;
	return 	lastval;
}

/* return positive value between 0 and RANDINT_MAX */
inline long fastlfo::randint31(long seed31)
{
	unsigned long rilo, rihi;

    rilo = RIA * (long)(seed31 & 0xFFFF);
    rihi = RIA * (long)((unsigned long)seed31 >> 16);
    rilo += (rihi & 0x7FFF) << 16;
    if (rilo > (unsigned long) RIM) {
      rilo &= RIM;
      ++rilo;
    }
    rilo += rihi >> 15;
    if (rilo > (unsigned long) RIM) {
      rilo &= RIM;
      ++rilo;
    }
    return (long)rilo;
}

bool fastlfo::set_WaveType(LfoWaveType type)		
{	
	switch(type){
	case(LFO_SINE):
            tf = &fastlfo::sinetick;
		break;
	case(LFO_TRIANGLE):
		tf = &fastlfo::tritick;
		break;
	case(LFO_SQUARE):
		tf = &fastlfo::squaretick;
		break;
	case(LFO_SAWUP):
		tf = &fastlfo::sawuptick;
		break;
	case(LFO_SAWDOWN):
		tf = &fastlfo::sawdowntick;
		break;
	case(LFO_RANDOM):
		tf = &fastlfo::randomtick;
		break;
       case(LFO_RAND_TPDF):
		tf = &fastlfo::rand_tpdf_tick;
		break;
       case(LFO_RAND_GAUSS):
		tf = &fastlfo::rand_gauss_tick;
		break;
       case(LFO_RAND_EXP):
		tf = &fastlfo::rand_exp_tick;
		break;
	case(LFO_RND_SH):
		tf = &fastlfo::randhtick;
		break;
       case(LFO_RANDH_TPDF):
		tf = &fastlfo::randh_tpdf_tick;
		break;
	default:
		return false;
	}
	m_cur_WaveType = type;	
	return true;
}

