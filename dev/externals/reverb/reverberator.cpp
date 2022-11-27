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
 
// reverberator.cpp: implementation of the reverberator class.
// see also tapdelay.cpp
//TODO: simple delay for use with tapped delayline
// NB blame VC6 for horrible code, new returns null...
//////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#ifdef _DEBUG
#include <assert.h>
#endif
#include "reverberator.h"
#ifndef PI
# define PI 3.141592653589793238462643
#endif
#ifndef TWOPI
#define TWOPI (2.0 * PI)
#endif

#define ROOT2O2 (0.7071067811965475244)
#define SPN 1.65436e-24			   //RWD: use a proper MSVC version...?

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//for example: 
//const deltap taps[4] = {{0.1,0.9},{0.25,.75},{0.5,.5},{0.618,.25}};
//NB this accepts a zero taptime

tapdelay::tapdelay(const deltap *taps,unsigned int ctaps,double sr) : dtaps(taps)
{
	tapbuf = 0;
	ntaps = ctaps;
	buflen = 0;
	reader = 0;
	tapptr = 0;
	tapcoeff = 0;
	srate = sr;
	output_gain = 1.0;
	max_gain = 1.0;
}
#define SAFETY (2)
bool tapdelay::create(void)
{
#ifdef _DEBUG
	assert(tapbuf==0);
	assert(ntaps > 0);
#endif
	if(tapbuf)
		return false;				//or some informative errorval...
	if(ntaps <=0)
		return false;
	buflen = (unsigned int)((dtaps[ntaps-1].pos * srate) + SAFETY);
	tapbuf = new double[buflen];
	if(!tapbuf)
		return false;
	memset(tapbuf,0,buflen*sizeof(double));
	
	tapptr = new unsigned int[ntaps];
	if(!tapptr){
		delete [] tapbuf; tapbuf = 0;
		return false;
	}
	tapcoeff = new double[ntaps];	
	if(!tapcoeff) {
		delete [] tapbuf; tapbuf = 0;
		delete [] tapptr; tapptr = 0;
		return false;
	}
	//copy taps data, and get scalefac from total amps
	double sum = 0.0;
	for(unsigned int i = 0;i < ntaps;i++){
		//RWD.10.98 allow zero taptime, for input mix control...
		if(dtaps[i].pos == 0.0)
			tapptr[i] = 0;
		else
			tapptr[i] = buflen - (int)(dtaps[i].pos * srate);	
			
		tapcoeff[i] = dtaps[i].val;
		sum += fabs(tapcoeff[i]);
#ifdef _DEBUG
		assert(tapptr[i] < buflen);
#endif
	}
	//some idiot might give us empty taps! */
	if(sum == 0.0){
		delete [] tapbuf; tapbuf = 0;
		delete [] tapptr; tapptr = 0;
		delete [] tapcoeff; tapcoeff = 0;
		return false;
	}
	max_gain = (double)ntaps / sum;
	one_over_ntaps = 1.0 / (double)ntaps;
	return true;
}

double tapdelay::get_maxgain(void)
{
	return max_gain;
}

void tapdelay::set_gain(double gain)
{
	output_gain = gain;
}

double tapdelay::tick(double insam)
{
	double val = 0.0;
	tapbuf[reader++] = insam;
	if(reader == buflen)
		reader = 0;
	for(unsigned int i = 0;i < ntaps;i++) {
		val += 	((tapbuf[tapptr[i]++]) * (tapcoeff[i]));
		if(tapptr[i] == buflen)
			tapptr[i] = 0;
	}
	val *= one_over_ntaps;
	return val * output_gain;

}

tapdelay::~tapdelay()
{
	delete [] tapbuf;
	delete [] tapptr;
	delete [] tapcoeff;
}


delay::delay(unsigned int len, double gain)
{
	delbuf= 0;
	dgain = gain;
	ptr = 0;
	buflen = len;
}

delay::~delay()
{
	delete delbuf;
}

bool delay::create(void)
{
	 if(buflen <= 0)
		 return false;
	 if(delbuf)
		 return false;

	 delbuf = new double[buflen];
	 if(!delbuf) {		
		 return false;
	 }
	 memset(delbuf,0,buflen*sizeof(double));
	 return true;
}

double delay::tick(double insam)
{
	double val;
	val = delbuf[ptr];
	delbuf[ptr++] = insam;
	if(ptr==buflen)
		ptr = 0;
	return  val*dgain;
}



//////////////////////////////////////////////////////////////////////
// allpassfilter 
//////////////////////////////////////////////////////////////////////
//global helper functions: really belong in a new cdplib...except it's nice to have proper bool retval
//straight from Csound: nreverb (vdelay.c)
static bool prime( int val )
{
    int i, last;
	
    last = (int)sqrt( (double)val );
    for ( i = 3; i <= last; i+=2 ) {
      if ( (val % i) == 0 ) return false;
    }
    return true;
}

unsigned int to_prime(double dur,double sr)
{
		unsigned int time = (unsigned int) (dur * sr);
		if(time % 2== 0) time += 1;
		while(!prime(time)) 
			time +=2;
		return time;
}
#define VMODMAX (0.5)
//standard allpass, with optional pre_delay
allpassfilter::allpassfilter(long sr,unsigned int buflen, double ingain,unsigned int predelay = 0)
{
    vmod                = 0.1;
    vfreq               = 0.5; 
	rvbuf1				= 0;
	pre_dline			= 0;
	writer1 = reader1	= 0;
    sinelfo             = 0;
    noiselfo            = 0;
    srate               = sr;
	rvblen				=  buflen + (unsigned int)(buflen * VMODMAX);
	gain				= ingain;
	pre_delay			= predelay;
    
}

bool allpassfilter::create(void)
{
	if((gain < -1.0) || (gain > 1.0))		//do I want negative gain vals?
		return false;
	rvbuf1 = new double[rvblen];	
	if(!rvbuf1 ){
#ifdef _DEBUG
		printf("\nallpass: failed to allocate buffer size %d",rvblen);
#endif		
		return false;
	}
				
	if(pre_delay > 0){
		pre_dline = new delay(pre_delay,1.0);
		if(!pre_dline->create()){
#ifdef _DEBUG
		    printf("\nallpass: can't create pre_dline" );
#endif		
		    delete []rvbuf1; rvbuf1 = 0; 
		    return false;
	    }
	}
	memset(rvbuf1,0,rvblen*sizeof(double));
    sinelfo = new fastlfo();
    sinelfo->init((double) srate,0.0,0,1);
    sinelfo->set_WaveType(LFO_SINE);
    sinelfo->set_freq(vfreq);
    sinelfo->set_mod(1.0);
    noiselfo = new fastlfo();
    noiselfo->init((double) srate,0.0,0,1);
    noiselfo->set_WaveType(LFO_RAND_GAUSS);
    noiselfo->set_freq(vfreq * 2.5);
    noiselfo->set_mod(1.0);

	return true;
}

void allpassfilter::set_vmod(double amount)
{
    double numod = VMODMAX;
    if(amount < numod)
        numod = amount;
    vmod = numod;
}

void allpassfilter::set_vfreq(double freq)
{
    vfreq = freq;
    sinelfo->set_freq(vfreq);
    noiselfo->set_freq(vfreq * 2.5);
}

/* TODO: optimize this:  separate  funcs for pre-dline and plain */
double allpassfilter::tick(double insamp)
{
	double output, input; 
	input = insamp;
	output = (-gain) * input;
	output += rvbuf1[reader1++];
	input += gain * output;

	if(pre_dline)
		rvbuf1[writer1++] = pre_dline->tick(input);
	else
		rvbuf1[writer1++] = input;
	if(reader1 == rvblen)
		reader1 = 0;
	if(writer1 == rvblen)
		writer1 = 0;

	return output;
}
/* TODO: implement this! */
double allpassfilter::vtick(double insamp)
{
	double output, input; 
	input = insamp;
	output = (-gain) * input;
	output += rvbuf1[reader1++];
	input += gain * output;

	if(pre_dline)
		rvbuf1[writer1++] = pre_dline->tick(input);
	else
		rvbuf1[writer1++] = input;
	if(reader1 == rvblen)
		reader1 = 0;
	if(writer1 == rvblen)
		writer1 = 0;

	return output;
}



allpassfilter::~allpassfilter()
{
	delete [] rvbuf1;
	delete pre_dline;
}

//////////////////////////////////////////////////////////////////////
//// nested_allpass
//overall delay = ap1.pre_delay + ap2.pre_deay + ap1.length + ap2.length + post_delay
nested_allpass::nested_allpass(double srate,double lpfreq, unsigned int outertime, unsigned int time1, unsigned int time2,
							   double gain, double gain1, double gain2, 
							   unsigned int delay1 = 0, unsigned int delay2 = 0)
							   
{
	outer_gain		= gain;
	outer_time		= outertime;
	ap1_gain		= gain1;
	ap2_gain		= gain2;	
	ap1_length		= time1;
	ap2_length		= time2;
	ap1_delay		= delay1;
	ap2_delay		= delay2;
	buf				= 0;
	if(ap2_gain == 0.0)
		ap2_length = 0;
	ap1				= ap2	= 0;		
	writer = reader = 0;	
	lp = 0;
	lpfreq_ = lpfreq;
	sr_ = srate;

}

nested_allpass::~nested_allpass()
{	
	delete [] buf;
	delete ap1;
	delete ap2;
	//RWD
	delete lp;
}

bool nested_allpass::create(void)
{
	if(outer_gain < -1.0 || outer_gain > 1.0)
		return false;	
	if(outer_time <= 0)
		return false;
	buf = new double[outer_time];
	if(!buf)
		return false;
	memset(buf,0,outer_time * sizeof(double));
	if(ap1_length > 0 && ap1_gain > 0.0){
		ap1 = new allpassfilter((long)sr_,ap1_length,ap1_gain,ap1_delay);
		if(!ap1->create()){
			delete [] buf; buf = 0;
			return false;
		}
	}
	
	if(ap2_length >0 && ap2_gain > 0.0){
		ap2 = new allpassfilter((long)sr_,ap2_length,ap2_gain,ap2_delay);
		if(!ap2->create()){
			delete ap1;	 ap1 = 0;
			delete [] buf;  buf = 0;
			return false;
		}
	}
	// RWD experiment: internal absorption damping
	lp = new onepole(lpfreq_,sr_,LOW_PASS);
	if(!lp){
		delete ap1;	 ap1 = 0;
		delete ap2;	 ap2 = 0;
		delete [] buf;  buf = 0;
		return false;
	}
	damping_ = false;
	return true;
}

//messy casts - sort this out later...
double nested_allpass::tick(double insam)
{

	double output, input,nest_out; 
	input = insam;
	output = (-outer_gain) * input;
	output += buf[reader++];
	input += outer_gain * output;
	//input is to two allpasses, and the post_delay, which acts as the overall allpass
	if(!ap2) {	//no second allpass, may be one or no first allpass
		if(ap1)			
			nest_out =  ap1->tick(input);
		else
			//behave as normal allpass
			nest_out =  input;
	}
	else		
		nest_out = 	 ap2->tick(ap1->tick(input));

	//RWD experiment!
	if(damping_)
	   buf[writer++] = lp->tick(nest_out);
	else
		buf[writer++] = nest_out;
	//wrap indices
	if(writer == outer_time)
		writer = 0;
	if(reader == outer_time)
		reader = 0;
			
	return output;
}


//////////////////////////////////////////////////////////////////////
// lpcomb 
//////////////////////////////////////////////////////////////////////

lpcomb::lpcomb(unsigned int buflen, double fbgain, double lpcoeff, double pre = 1.0)
{
	combbuf = 0;
	lp = 0;
	gain = fbgain;
	lpgain = lpcoeff;
	rvblen = buflen;
	prescale  = pre;
	reader1 = writer1 = 0;
}

bool lpcomb::create(void)
{
	if(prescale <= 0.0) 
		return false;
	if((gain < -1.0 ) || (gain > 1.0))		//do I want negative gain vals?
		return false;

	combbuf = new double[rvblen];
	if(!combbuf)
		return false;
	lp = new lowpass1(lpgain);
	if(!lp){
		delete [] combbuf; combbuf = 0;
		return false;
	}
	memset(combbuf,0,rvblen*sizeof(double));
	return true;
}

 //wet output only, unscaled
double lpcomb::tick(double insam)
{
	double input,output,lpsig;
	input = insam * prescale;
	
	output = combbuf[reader1++];		 //do gain scaling outside
	lpsig = lp->tick(output);

	combbuf[writer1++] = input + (gain * lpsig) ;	//feedback scaled output
	

	if(writer1 == rvblen)
		writer1 = 0;
	if(reader1 == rvblen)
		reader1 = 0;
	return  output;
}

lpcomb::~lpcomb()
{
	delete [] combbuf;
	delete lp;
}
 
 //////////////////////////////////////////////////////////////////////
// lowpass 	: perhaps a bit ott as a class, but it allows fairly independent fiddling
//////////////////////////////////////////////////////////////////////
lowpass1::lowpass1(double filtgain)
{
	temp = output = 0.0;
	gain = filtgain;
}

lowpass1::~lowpass1()
{
}
 //IIR lowpass
inline double lowpass1::tick(double insam)
{	 
	output  = temp;
	temp = insam + (output * gain);
	return output;

}
// NB allows zero frequency: straight pass-through
onepole::onepole(double freq,double srate, int low_high)
{
	if(freq == 0.0){
		a1 = 1.0;
		a2 = 0.0;
	}
	else {
		b = 2.0 - cos(TWOPI * (freq/srate));
		a2 = sqrt((b*b) - 1.0) - b;
		a1 = 1.0 + a2;
		if(low_high==HIGH_PASS)
			a2 = -a2;
	}
	lastout = 0.0;
}

onepole::~onepole()
{
	 //nothing to do
}

double onepole::tick(double input)
{
	double output;
	output  =  a1*input - a2*lastout;
	lastout =  output;
	return output;
}

//uses code from CDP eq.c

tonecontrol::tonecontrol(double frq,double dbfac,int type,double srate)
{
	freq = frq;
	boost = dbfac;
	sr = srate;
	tc_type = type;
	x1 = x2 = y1 = y2 = 0.0;
	a0 = a1 = a2 = b1 = b2 = 0.0;
}

tonecontrol::~tonecontrol()
{
   //nothing to do;
}

bool tonecontrol::create(void)
{
	if(sr <= 0.0)
		return false;
	if(freq <= 0.0)
		return false;
	//normalize freq
	freq /= sr;
	switch(tc_type){
	case(LOW_SHELVE):
		lshelve();
		break;
	case(HIGH_SHELVE):
		hshelve();
		break;
	default:
		return false;
		break;
	}
	return true;
}

double tonecontrol::tick(double input)
{
	double out;

	if(boost== 0.0)
		return input;

	out = ((a0 * input) + (a1 * x1) + (a2 * x2) - (b1 * y1) - (b2 * y2));
	x2 = x1;
	x1 = input;
	y2 = y1;
	y1 = out;
	return out;
}

void tonecontrol::lshelve(void)
{
	double a, A, F, tmp, b0, recipb0, asq, F2, gamma2, siggam2, gam2p1;
	double gamman, gammad, ta0, ta1, ta2, tb0, tb1, tb2, aa1, ab1;

	a = tan(PI * (freq - 0.25));	/* Warp factor */ 
	asq = a * a;
	A = pow(10.0, boost/20.0);	/* Cvt dB to factor */
	if((boost < 6.0) && (boost > -6.0)) F = sqrt(A);
	else if (A > 1.0) F = A/sqrt(2.0);
	else F = A * sqrt(2.0);
	  /* If |boost/cut| < 6dB, then doesn't make sense to use 3dB point.
	     Use of root makes bandedge at half the boost/cut amount
	  */

	F2 = F * F;
	tmp = A * A - F2;
	if(fabs(tmp) <= SPN) gammad = 1;
	else gammad = pow( (F2 - 1)/ tmp, 0.25);	/* Fourth root */
	gamman = sqrt(A) * gammad;

   /* Once for the numerator */

	gamma2 = gamman * gamman;
	gam2p1 = 1 + gamma2;
	siggam2 = 2 * ROOT2O2 * gamman;
	
	ta0 = gam2p1 + siggam2;
	ta1 = -2 * (1 - gamma2);
	ta2 = gam2p1 - siggam2;

   /* And again for the denominator */

	gamma2 = gammad * gammad;
	gam2p1 = 1 + gamma2;
	siggam2 = 2 * ROOT2O2 * gammad;
	
	tb0 = gam2p1 + siggam2;
	tb1 = -2 * (1 - gamma2);
	tb2 = gam2p1 - siggam2;

   /* Now bilinear transform to proper centre frequency */

	aa1 = a * ta1;
	a0 = ta0 + aa1 + asq * ta2;
	a1 = 2 * a * (ta0 + ta2) + (1 + asq) * ta1;
	a2 = asq * ta0 + aa1 + ta2;

	ab1 = a * tb1;
	b0 = tb0 + ab1 + asq * tb2;
	b1 = 2 * a * (tb0 + tb2) + (1 + asq) * tb1;
	b2 = asq * tb0 + ab1 + tb2;
	
   /* Normalise b0 to 1.0 for realisability */

	recipb0 = 1 / b0;
	a0 *= recipb0;
	a1 *= recipb0;
	a2 *= recipb0;
	b1 *= recipb0;
	b2 *= recipb0;

}

void tonecontrol::hshelve(void)
{
	double a, A, F, tmp, b0, recipb0, asq, F2, gamma2, siggam2, gam2p1;
	double gamman, gammad, ta0, ta1, ta2, tb0, tb1, tb2, aa1, ab1;

	a = tan(PI * (freq - 0.25));	/* Warp factor */ 
	asq = a * a;
	A = pow(10.0, boost/20.0);	/* Cvt dB to factor */
	if(boost < 6.0 && boost > -6.0) F = sqrt(A);
	else if (A > 1.0) F = A/sqrt(2.0);
	else F = A * sqrt(2.0);
	  /* If |boost/cut| < 6dB, then doesn't make sense to use 3dB point.
	     Use of root makes bandedge at half the boost/cut amount
	  */

	F2 = F * F;
	tmp = A * A - F2;
	if(fabs(tmp) <= SPN) gammad = 1;
	else gammad = pow( (F2 - 1)/ tmp, 0.25);	/* Fourth root */
	gamman = sqrt(A) * gammad;

   /* Once for the numerator */

	gamma2 = gamman * gamman;
	gam2p1 = 1 + gamma2;
	siggam2 = 2 * ROOT2O2 * gamman;
	
	ta0 = gam2p1 + siggam2;
	ta1 = -2 * (1 - gamma2);
	ta2 = gam2p1 - siggam2;

	ta1 = -ta1;
	
   /* And again for the denominator */

	gamma2 = gammad * gammad;
	gam2p1 = 1 + gamma2;
	siggam2 = 2 * ROOT2O2 * gammad;
	
	tb0 = gam2p1 + siggam2;
	tb1 = -2 * (1 - gamma2);
	tb2 = gam2p1 - siggam2;

	tb1 = -tb1;

   /* Now bilinear transform to proper centre frequency */

	aa1 = a * ta1;
	a0 = ta0 + aa1 + asq * ta2;
	a1 = 2 * a * (ta0 + ta2) + (1 + asq) * ta1;
	a2 = asq * ta0 + aa1 + ta2;

	ab1 = a * tb1;
	b0 = tb0 + ab1 + asq * tb2;
	b1 = 2 * a * (tb0 + tb2) + (1 + asq) * tb1;
	b2 = asq * tb0 + ab1 + tb2;
	
   /* Normalise b0 to 1.0 for realisability */

	recipb0 = 1 / b0;
	a0 *= recipb0;
	a1 *= recipb0;
	a2 *= recipb0;
	b1 *= recipb0;
	b2 *= recipb0;

}


/***************** GARDNER DIFFUSERS ***************/

// NOTE: the published Gardner designs are here modified by adding a onepole lp filter
// in the internal outer feedback loop of each nested allpass filter. 
// When combined with the overall feedback filter, this will of course reduce the reverb decay time.
// The option exists to bypass this overall feedback filter, and rely solely on the internal ones.


// NOTE: to get an infinite reverb, all feedback filters have to be bypassed by setting lp_freq to 0.0
// otherwise, the output will still decay eventually !

small_diffuser::small_diffuser(unsigned int pre_delay, 
							   const NESTDATA *apdata1,
							   const NESTDATA *apdata2,double fb_gain,double lp_freq,double srate){

	  ap1_data = *apdata1;
	  ap2_data = *apdata2;
	  predelay_time = pre_delay;
	  lpgain = fb_gain;
	  lpfreq = lp_freq;
	  ap1 = ap2 = 0;
	  lp1 = 0;
	  predelay = 0;
	  out1 = out2 = 0.0;
	  sr = srate;
	  damping_ = false;
}

small_diffuser::~small_diffuser()
{
	delete ap1;
	delete ap2;
	delete predelay;	
	delete lp1;
}

bool small_diffuser::create(void)
{
	if(ap1_data.time1 == 0 || ap1_data.gain1 <= 0.0){
#ifdef _DEBUG
		printf("\ndiffuser: bad parameters(1)");
#endif		
		return false;
	}
	if(ap1_data.time2 ==0 || ap1_data.gain2 <= 0.0){
#ifdef _DEBUG
		printf("\ndiffuser: bad parameters(2)");
#endif		
		return false;
	}
	if(ap2_data.time1 == 0 || ap2_data.gain1 < 0.0){
#ifdef _DEBUG
		printf("\ndiffuser: bad parameters(3)");
#endif		
		return false;
	}
	if(ap2_data.time2 ==0 || ap2_data.gain2 < 0.0){
#ifdef _DEBUG
		printf("\ndiffuser: bad parameters(4)");
#endif		
		return false;
	}

	if(sr <=0.0){
#ifdef _DEBUG
		printf("\ndiffuser: bad srate parameter)");
#endif		
		return false;
	}
	if(lpfreq <0.0){
#ifdef _DEBUG
		printf("\ndiffuser: bad freq parameter)");
#endif		
		return false;
	}

	ap1 = new nested_allpass(sr,lpfreq,ap1_data.time1,ap1_data.time2,ap1_data.time3,
							ap1_data.gain1,ap1_data.gain2,ap1_data.gain3);
	if(!ap1->create()){
#ifdef _DEBUG
		printf("\ndiffuser: can't create first allpass");
#endif		
		return false;
	}
	if(ap2_data.gain1 != 0.0){	 //allow ap to eliminate second block
		ap2 = new nested_allpass(sr,lpfreq,ap2_data.time1,ap2_data.time2,ap2_data.time3,
							ap2_data.gain1,ap2_data.gain2,ap2_data.gain3);
		if(!ap2->create()){
#ifdef _DEBUG
			printf("\ndiffuser: can't create second allpass");
#endif		
			delete ap1; ap1 = 0;
			return false;
		}
	}
	if(predelay_time > 0){
		predelay = new delay(predelay_time,1.0);
		if(!predelay->create()){
#ifdef _DEBUG
			printf("\ndiffuser: can't create predelay");
#endif		
			delete ap1;	ap1 = 0;
			delete ap2;	ap2 = 0;
			return false;
		}
	}
	// NB if lpfreq == 0, onepole is no-op - returns input
	lp1 = new onepole(lpfreq,sr,LOW_PASS);
	if(lp1 == 0){
		delete [] predelay; predelay = 0;
		delete ap1;	ap1 = 0;
		delete ap2;	ap2 = 0;
		return false;
	}
	ap1->set_damping(damping_);
	ap2->set_damping(damping_);
	return true;
}


double small_diffuser::tick(double insam) 
{
	double filter_out;	
	double lp_in;	
	double output;
	double ip;
	// may only contain one nested allpass filter:
	// input to filter is either out1 or out2
	if(ap2)
		lp_in = out2;	   //= both in series
	else
		lp_in = out1;
	// RWD NOTE: if the damping_ mechanism is preferred, this filter can be omitted and ...
	filter_out =  lp1->tick(lp_in);
	// ... just use this				 
	//	filter_out = lp_in ;
	
	ip = insam + lpgain * filter_out;	
	if(predelay)
		ip = predelay->tick(ip);
	out1 = ap1->tick( ip);
	if(ap2){
		out2 = ap2->tick(out1);
		output =  (out1 + out2)  * 0.5;
	}
	else
		output =  out1;
	return  output;
}

//post-delay almost certainly doe not need to be prime...
medium_diffuser::medium_diffuser(double post_delay,const NESTDATA *apdata1,
					const NESTDATA *apdata2,
					double gain,
					double lp_freq,
					double srate)
{
	ap1_data = *apdata1;
	ap2_data = *apdata2;
	postdelay_time = to_prime(post_delay,srate);
	md_gain = gain;
	lpfreq = lp_freq;
	ap1 = ap2 = 0;
	ap3 = 0;
	lp1 = 0;
	delay1 = delay2 = postdelay = 0;
	sr = srate;
	out1 = out2 =out3 = 0.0;
	damping_  = false;
}

medium_diffuser::~medium_diffuser()
{	
		delete ap1;	
		delete ap2;	
		delete ap3;	
		delete lp1;	
		delete delay1;	
		delete delay2;
}


bool medium_diffuser::create(void)
{
	if(ap1_data.time1 == 0 || ap1_data.gain1 <= 0.0){
#ifdef _DEBUG
		printf("\nmedium_diffuser: bad parameters(1)");
#endif		
		return false;
	}
	if(ap1_data.time2 ==0 || ap1_data.gain2 <= 0.0){
#ifdef _DEBUG
		printf("\nmedium diffuser: bad parameters(2)");
#endif		
		return false;
	}
	if(ap2_data.time1 == 0 || ap2_data.gain1 < 0.0){
#ifdef _DEBUG
		printf("\nmedium diffuser: bad parameters(3)");
#endif		
		return false;
	}
	if(ap2_data.time2 ==0 || ap2_data.gain2 < 0.0){
#ifdef _DEBUG
		printf("\nmedium diffuser: bad parameters(4)");
#endif		
		return false;
	}
	if(sr <=0.0){
#ifdef _DEBUG
		printf("\nmedium diffuser: bad srate parameter)");
#endif		
		return false;
	}
	if(lpfreq <0.0){
#ifdef _DEBUG
		printf("\nmedium diffuser: bad freq parameter)");
#endif		
		return false;
	}

	ap1 = new nested_allpass(sr,lpfreq,ap1_data.time1,ap1_data.time2,ap1_data.time3,
							ap1_data.gain1,ap1_data.gain2,ap1_data.gain3);
	if(!ap1->create()){
#ifdef _DEBUG
		printf("\nmedium diffuser: can't create first diffuser");
#endif		
		return false;
	}
	ap2 = new nested_allpass(sr,lpfreq,ap2_data.time1,ap2_data.time2,ap2_data.time3,
							ap2_data.gain1,ap2_data.gain2,ap2_data.gain3);
	if(!ap2->create()){
#ifdef _DEBUG
		printf("\nmedium diffuser: can't create second diffuser");
#endif	
		delete ap1; ap1 = 0;
		return false;
	}

	ap3 = new allpassfilter((long)sr,to_prime(0.03,sr),0.5,to_prime(0.005,sr));
	if(!ap3->create()){
#ifdef _DEBUG
		printf("\nmedium diffuser: can't create internal allpass filter");
#endif
		delete ap1; ap1 = 0;
		delete ap2; ap2 = 0;
		return false;
	}


	if(postdelay_time > 0){
		postdelay = new delay(postdelay_time,1.0);
		if(!postdelay->create()){
#ifdef _DEBUG
			printf("\nmedium diffuser: can't create postdelay");
#endif		

			delete ap1; ap1 = 0;
			delete ap2; ap2 = 0;
			delete ap3; ap3 = 0;
			return false;
		}
	}

	//internal fixed delays
	delay1 = new delay(to_prime(/*0.007*/0.067,sr),1.0);
	if(!delay1->create()){
#ifdef _DEBUG
		printf("\nmedium diffuser: can't create internal delay1");
#endif
		delete ap1; ap1 = 0;
		delete ap2; ap2 = 0;
		delete ap3; ap3 = 0;
		delete [] postdelay; postdelay = 0;
		return false;
	}
		delay2 = new delay(to_prime(0.015,sr),1.0);
	if(!delay2->create()){
#ifdef _DEBUG
		printf("\nmedium diffuser: can't create internal delay2");
#endif
		delete ap1; ap1 = 0;
		delete ap2; ap2 = 0;
		delete ap3; ap3 = 0;
		delete [] postdelay; postdelay = 0;
		delete delay1; delay1 = 0;
		return false;
	}
	
	lp1 = new onepole(lpfreq,sr,LOW_PASS);
	if(lp1 == 0){
		delete ap1; ap1 = 0;
		delete ap2; ap2 = 0;
		delete ap3; ap3 = 0;
		delete [] postdelay; postdelay = 0;
		delete delay1; delay1 = 0;
		delete delay2; delay2 = 0;
		return false;
	}
	if(lpfreq == 0.0)
		damping_ = false;
	ap1->set_damping(damping_);
	ap2->set_damping(damping_);
	return true;
}

double medium_diffuser::tick(double insam)
{
	double filter_out;
	double output;

	//feedback from postdelay, thru lp filter
	// RWD NOTE: if the damping_ mechanism is preferred, this filter can be omitted and...
	filter_out = md_gain * lp1->tick(postdelay->tick(out3));
	// ... just use this
	//filter_out = (float)(md_gain * postdelay->tick(out3));
	
	//first nested-allpass, takes input plus feedback
	out1 = ap1->tick(insam + filter_out);
	//inner allpass with predelay, followed by plain delay
	out2 = delay1->tick(ap3->tick(out1));
	//second nested-allpass, takes direct input
	out3 = ap2->tick(insam + md_gain * delay2->tick(out2));
	output = 0.5 * (out1 + out2 + out3);
	return output;
}


large_diffuser::large_diffuser(const NESTDATA *apdata1,
					const NESTDATA *apdata2,
					double gain,
					double lp_freq,
					double srate)
{
	ap1_data = *apdata1;
	ap2_data = *apdata2;
	ld_gain = gain;
	lpfreq = lp_freq;
	ap1 = ap2 = 0;
	ap3 = ap4 = 0;
	lp1 = 0;
	delay1 = delay2 = delay3 = delay4 = 0;
	sr = srate;
	out1 = out2 =out3 = 0.0;
	damping_ = false;
}

large_diffuser::~large_diffuser()
{	
	delete ap1;	
	delete ap2;
	delete ap3;
	delete ap4;
 	delete lp1;
	delete delay1;
	delete delay2;
	delete delay3;
	delete delay4;
}

bool large_diffuser::create(void)
{
	if(ap1_data.time1 == 0 || ap1_data.gain1 <= 0.0){
#ifdef _DEBUG
		printf("\nmedium_diffuser: bad parameters(1)");
#endif		
		return false;
	}
	if(ap1_data.time2 ==0 || ap1_data.gain2 <= 0.0){
#ifdef _DEBUG
		printf("\nmedium diffuser: bad parameters(2)");
#endif		
		return false;
	}
	if(ap2_data.time1 == 0 || ap2_data.gain1 < 0.0){
#ifdef _DEBUG
		printf("\nmedium diffuser: bad parameters(3)");
#endif		
		return false;
	}
	if(ap2_data.time2 ==0 || ap2_data.gain2 < 0.0){
#ifdef _DEBUG
		printf("\nmedium diffuser: bad parameters(4)");
#endif		
		return false;
	}
	if(sr <=0.0){
#ifdef _DEBUG
		printf("\nmedium diffuser: bad srate parameter)");
#endif		
		return false;
	}
	if(lpfreq <0.0){
#ifdef _DEBUG
		printf("\nmedium diffuser: bad freq parameter)");
#endif		
		return false;
	}
	ap1 = new nested_allpass(sr,lpfreq,ap1_data.time1,ap1_data.time2,ap1_data.time3,
							ap1_data.gain1,ap1_data.gain2,ap1_data.gain3);
	if(!ap1->create()){
#ifdef _DEBUG
		printf("\nlarge diffuser: can't create first nested_allpass");
#endif		
		return false;
	}
	ap2 = new nested_allpass(sr,lpfreq,ap2_data.time1,ap2_data.time2,ap2_data.time3,
							ap2_data.gain1,ap2_data.gain2,ap2_data.gain3);
	if(!ap2->create()){
#ifdef _DEBUG
		printf("\nlarge diffuser: can't create second netsed_allpass");
#endif	
		cleanup();
		return false;
	}

	ap3 = new allpassfilter((long) sr,to_prime(0.008,sr),0.3);
	if(!ap3->create()){
#ifdef _DEBUG
		printf("\nlarge diffuser: can't create first internal allpass filter");
#endif
		cleanup();
		return false;
	}

	ap4 = new allpassfilter((long)sr,to_prime(0.012,sr),0.3);
	if(!ap4->create()){
#ifdef _DEBUG
		printf("\nlarge diffuser: can't create second internal allpass filter");
#endif
		cleanup();
		return false;
	}
	//internal fixed delays
	delay1 = new delay((unsigned int)(0.004 * sr),1.0);
	if(!delay1->create()){
#ifdef _DEBUG
		printf("\nlarge diffuser: can't create internal delay1");
#endif
		cleanup();
		return false;
	}
		delay2 = new delay((unsigned int)(0.017 * sr),1.0);
	if(!delay2->create()){
#ifdef _DEBUG
		printf("\nlarge diffuser: can't create internal delay2");
#endif
		cleanup();
		return false;
	}
	delay3 = new delay((unsigned int)(0.031 * sr),1.0);
	if(!delay3->create()){
#ifdef _DEBUG
		printf("\nlarge diffuser: can't create internal delay3");
#endif
		cleanup();
		return false;
	}
	delay4 = new delay((unsigned int)(0.003 * sr),1.0);
	if(!delay4->create()){
#ifdef _DEBUG
		printf("\nlarge diffuser: can't create internal delay4");
#endif
		cleanup();
		return false;
	}
	
	lp1 = new onepole(lpfreq,sr,LOW_PASS);
	if(lp1 == 0){
		cleanup();
		return false;
	}
	if(lpfreq == 0.0)
		damping_ = false;
	
	ap1->set_damping(damping_);
	ap2->set_damping(damping_);
	return true;
}
void  large_diffuser::cleanup()
{
	delete lp1; lp1 = 0;
	delete delay4; delay4 = 0;
	delete delay3; delay3 = 0;
	delete delay2; delay2 = 0;
	delete delay1; delay1 = 0;
	delete ap4; ap4 = 0;
	delete ap3; ap3 = 0;
	delete ap2; ap2 = 0;
	delete ap1; ap1 = 0;
}

// NOTE: the three output taps from the diffuser can lead to discrete slap-back echoes in the output.
// possibly authentic, but may not suit all purposes. See rmverb.cpp: some delay lengths reduced from Gardner.
// This is possible because delays are discrete: they do not share single delay line buffer.
// true also of medium diffuser, but not so apparent as delays are smaller
double large_diffuser::tick(double insam)
{
	double filter_out;
	double output;
	//feedback from lpfilter
	// RWD NOTE: if the damping_ mechanism is preferred, this filter can be omitted and ...
	filter_out = ld_gain * lp1->tick(out3);		
	// ... just use this
	//filter_out = (float)(ld_gain * out3);			  

	//first tap, from two plain allpasses, and trailing delay
	out1 = delay1->tick(ap4->tick(ap3->tick(insam + filter_out)));
	//second tap, from first nested allpass and delays at entrance and exit
	out2 = delay3->tick(ap1->tick(delay2->tick(out1)));
	//third tap, from second nested-allpass with leading delay
	out3 = ap2->tick(delay4->tick(out2));
	//out3 also goes into lpfilter... see above
	//prescribed scale factors from Gardner
	output = (out1 * 0.34) + (out2 * 0.14) + (out3 * 0.14);
	// this seems to give relatively soft reverb tail:  could try this:
	//output = (float)((out1 * 0.7) + (out2 * 0.25) + (out3 * 0.25));
	return  output;
}


moorer::moorer(const MOORERDATA *pdata,double reverbtime,double damping,double srate)
{
	mdata		= *pdata;
	reverb_time = reverbtime;
	dampfac		= damping;
	sr			= srate;
	comb1 = comb2 = comb3 = comb4 = comb5 = comb6 = 0;
	ap1 = ap2 = ap3 =  ap4  = 0;
	out = 0.0;
}

moorer::~moorer()
{
	delete comb1;
	delete comb2;
	delete comb3;
	delete comb4;
	delete comb5;
	delete comb6;
	delete ap1;
	delete ap2;		
	delete ap3;
	delete ap4;
}

bool moorer::create(void)
{
	fgain1 = 0.42 * dampfac;
	fgain2 = 0.46 * dampfac;
	fgain3 = 0.48 * dampfac;
	fgain4 = 0.49 * dampfac;
	fgain5 = 0.50 * dampfac;
	fgain6 = 0.52 * dampfac;
	scalefac = (1.0 / 6.0);	
	cgain1 = exp(log(0.001)*(mdata.ctime1/reverb_time)) * (1. - fgain1);
	cgain2 = exp(log(0.001)*(mdata.ctime2/reverb_time)) * (1. - fgain2);
	cgain3 = exp(log(0.001)*(mdata.ctime3/reverb_time)) * (1. - fgain3);
	cgain4 = exp(log(0.001)*(mdata.ctime4/reverb_time)) * (1. - fgain4);
	cgain5 = exp(log(0.001)*(mdata.ctime5/reverb_time)) * (1. - fgain5);
	cgain6 = exp(log(0.001)*(mdata.ctime6/reverb_time)) * (1. - fgain6);

	comb1 = new lpcomb(to_prime(mdata.ctime1,sr),cgain1,fgain1,1.0);
	comb2 = new lpcomb(to_prime(mdata.ctime2,sr),cgain2,fgain2,1.0);
	comb3 = new lpcomb(to_prime(mdata.ctime3,sr),cgain3,fgain3,1.0);
	comb4 = new lpcomb(to_prime(mdata.ctime4,sr),cgain4,fgain4,1.0);
	comb5 = new lpcomb(to_prime(mdata.ctime5,sr),cgain5,fgain5,1.0);
	comb6 = new lpcomb(to_prime(mdata.ctime6,sr),cgain6,fgain6,1.0);
	ap1		= new allpassfilter((long)sr,to_prime(mdata.atime1,sr),-0.6,0);
	ap2		= new allpassfilter((long)sr,to_prime(mdata.atime2,sr),0.4,0);
	ap3		= new allpassfilter((long)sr,to_prime(mdata.atime3,sr),-0.61,0);
	ap4		= new allpassfilter((long)sr,to_prime(mdata.atime4,sr),0.39,0);

	if(!
		(comb1->create()
		&& comb2->create()
		&& comb3->create()
		&& comb4->create()
		&& comb5->create()
		&& comb6->create()
		&& ap1->create()
		&& ap2->create()
		&& ap3->create()
		&& ap4->create()
		)) {
		cleanup();
		return false;
	}
	return true;
}

double moorer::tick(double insam)
{
	out =  comb1->tick(insam) +
		  comb2->tick(insam) +
		  comb3->tick(insam) +
		  comb4->tick(insam) +
		  comb5->tick(insam) +
		  comb6->tick(insam)
		  ;
	out *= scalefac;
	out = ap4->tick(ap3->tick(ap2->tick(ap1->tick(out))));
	return out;
}

void moorer::cleanup()
{
	delete comb6; comb6 = 0;
	delete comb5; comb5 = 0;
	delete comb4; comb4 = 0;
	delete comb3; comb3 = 0;
	delete comb2; comb2 = 0;
	delete comb1; comb1 = 0;
	delete ap4; ap4 = 0;
	delete ap3; ap3 = 0;
	delete ap2; ap2 = 0;
	delete ap1; ap1 = 0;
}


/* vmtdelay *******************************/
vcomb4::vcomb4()
{
	dl_buf = 0;
	dl_length = 0;
	dl_input_index = 0;
	dl_srate = 0.0;
	
}

vcomb4::~vcomb4()
{
	if(dl_buf && dl_length > 0)
		delete [] dl_buf;
		
}

bool vcomb4::init(long srate,double length_secs)
{
	unsigned long len_frames = (unsigned long)(srate * length_secs );	
	if(len_frames == 0)
		return false;
	/*  round upwards to allow for interpolation */
	len_frames++;
	/* reject init  if already created*/
	/* TODO: more sexy error checking/reporting... */
	if(dl_buf) 
		return false;
	try {
		dl_buf = new double[len_frames];
	}
	catch(...){
		return false;
	}
	/* for VC 6 */
	if(dl_buf == 0)
		return false;
	for(unsigned long i = 0; i < len_frames;i++)
		dl_buf[i] = 0.0;

	dl_length = len_frames;
	dl_input_index = 0;
	dl_srate = srate;
	gain = 0.5;
	return true;
}

bool vcomb4::init(const MOORERDATA *p_mdata,double reverbtime,double damping,double sr)
{
	if(!init((long) sr,reverbtime))
		return false;
    return true;
}

double vcomb4::tick(double vdelay_1,double vdelay_2,double vdelay_3,double vdelay_4,double feedback,double input)
{
	unsigned long base_readpos1,base_readpos2,base_readpos3,base_readpos4; 
	unsigned long next_index1,next_index2,next_index3,next_index4; 
	double vlength,dlength,frac;
	double readpos1,readpos2,readpos3,readpos4;
	double* buf = dl_buf;
	
	dlength = (double) dl_length;					/* get maxlen, save a cast later on */
	/* read pointer is vlength ~behind~ write pointer */
	/* tap1*/
	vlength = dlength -  (vdelay_1  * dl_srate);
	vlength = vlength < dlength ? vlength : dlength;	  /* clip vdelay to max del length */
	readpos1 = dl_input_index + vlength;
	base_readpos1 = (unsigned long) ( readpos1);				  
	if(base_readpos1 >= dl_length)					  /* wrap dl indices */
		base_readpos1 -= dl_length;
	next_index1 = base_readpos1 + 1;
	if(next_index1 >= dl_length)
		next_index1 -= dl_length;
	/*tap2*/
	vlength = dlength -  (vdelay_2  * dl_srate);
	vlength = vlength < dlength ? vlength : dlength;
	readpos2 = dl_input_index + vlength;
	base_readpos2 = (unsigned long) ( readpos2);				  
	if(base_readpos2 >= dl_length)					  /* wrap dl indices */
		base_readpos2 -= dl_length;
	next_index2 = base_readpos2 + 1;
	if(next_index2 >= dl_length)
		next_index2 -= dl_length;
	/*tap3*/
	vlength = dlength -  (vdelay_3  * dl_srate);
	vlength = vlength < dlength ? vlength : dlength;
	readpos3 = dl_input_index + vlength;
	base_readpos3 = (unsigned long) ( readpos3);				  
	if(base_readpos3 >= dl_length)					  /* wrap dl indices */
		base_readpos3 -= dl_length;
	next_index3 = base_readpos3 + 1;
	if(next_index3 >= dl_length)
		next_index3 -= dl_length;
	/* tap4 */
	vlength = dlength -  (vdelay_4  * dl_srate);
	vlength = vlength < dlength ? vlength : dlength;
	readpos4 = dl_input_index + vlength;
	base_readpos4 = (unsigned long) ( readpos4);				  
	if(base_readpos4 >= dl_length)					  /* wrap dl indices */
		base_readpos4 -= dl_length;
	next_index4 = base_readpos4 + 1;
	if(next_index4 >= dl_length)
		next_index4 -= dl_length;


	double outsum = 0.0;
	fb1 *= feedback;
	fb2 *= feedback;
	fb3 *= feedback;
	fb4 *= feedback;
	/* basic interp of variable delay pos */
	frac = readpos1 - (int) readpos1;	
	outsum += fb1 * (buf[base_readpos1]+((buf[next_index1] - buf[base_readpos1]) * frac));
	frac = readpos2 - (int) readpos2;	
	outsum += fb2 * (buf[base_readpos2]+((buf[next_index2] - buf[base_readpos2]) * frac));
	frac = readpos3 - (int) readpos3;	
	outsum += fb3 * (buf[base_readpos3]+((buf[next_index3] - buf[base_readpos3]) * frac));
	frac = readpos4 - (int) readpos4;	
	outsum += fb4 * (buf[base_readpos4]+((buf[next_index4] - buf[base_readpos4]) * frac));
	/* how do we scale all this?*/
	input += gain * (outsum);	
	/* add in new sample + fraction of ouput, unscaled, for minimum decay at max feedback */
	buf[dl_input_index++] = input   + outsum * 0.25;
	if(dl_input_index == dl_length)
		dl_input_index = 0;
	return  input + outsum * 0.25;

}