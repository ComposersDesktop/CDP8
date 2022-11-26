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
 

// reverberator.h: interface for the reverberator class.
// RWD.9.98 now contains all my delay/allpass etc  ugs
//RWD 03.20  corrected def of tick function to double everywhere
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_REVERBERATOR_H__9DE45E43_83BA_11D1_96D4_444553540000__INCLUDED_)
#define AFX_REVERBERATOR_H__9DE45E43_83BA_11D1_96D4_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "wavetable.h"

enum {LOW_PASS,HIGH_PASS};
enum {LOW_SHELVE,HIGH_SHELVE};

unsigned int to_prime(double dur,double sr);



typedef struct __tap
{
	double pos;
	double val;
}
deltap;

typedef struct nested_allpass_data{
	unsigned int time1;
	unsigned int time2;
	unsigned int time3;
	unsigned int delay1;
	unsigned int delay2;
	double gain1;
	double gain2;
	double gain3;
}
NESTDATA;

typedef struct moorer_data{
	double ctime1;
	double ctime2;
	double ctime3;
	double ctime4;
	double ctime5;
	double ctime6;
	double atime1;
	double atime2;
	double atime3;
	double atime4;
}
MOORERDATA;
//abstract base class for all processors

class cdp_processor 
{
public:
	cdp_processor() {}
	virtual ~cdp_processor(){}
	//not all processes will need to use this
	virtual bool create() {return true;}
//	virtual float tick(float insam)	{return insam;}
    virtual double tick(double insam) = 0;
};


//could have a method create_taps(uint ntaps, float maxtime)  ... ?
class tapdelay 
{
public:
	tapdelay(const deltap *taps,unsigned int ctaps,double sr);
	bool	create(void);
	double  get_maxgain(void);
	void set_gain(double gain);
	double	tick(double insam);
	virtual ~tapdelay();
private:
	double *tapbuf;
	double srate;
	double one_over_ntaps;
	double max_gain;		/* get rms value of whole array */
	double output_gain;
	unsigned int ntaps;
	unsigned int buflen;
	unsigned int *tapptr;
	double *tapcoeff;
	unsigned int reader;
	const deltap *dtaps;
};

//basic feedthrough delay (independent of srate) - could be base class?
class delay
{
public:
	delay(unsigned int len, double gain);
	virtual ~delay();
	bool create(void);
	double tick(double insam);
private:
	double *delbuf;
	unsigned int buflen;
	double dgain;
	unsigned int ptr;
};



class allpassfilter  
{
public:
	allpassfilter(long sr,unsigned int time, double ingain,unsigned int predelay);
	virtual ~allpassfilter();
	bool create(void);    
    void set_vfreq(double freq);
    void set_vmod(double amount);
	double tick(double insam);
    double vtick(double insam);

private:
	double gain;
	double *rvbuf1;			 //CDP allpass uses 2 buffers!
    double vfreq,vmod;
    fastlfo *sinelfo;
    fastlfo *noiselfo;
	unsigned int rvblen, writer1, reader1;
	delay *pre_dline;
	unsigned int pre_delay;
    long srate;
};

///////////////// DOUBLE-NESTED ALLPASS
///  set either or both gain2, gain3 to zero to disable the respective allpass
////	
class onepole;

class  nested_allpass
{
public:
	nested_allpass(double srate,double lpfreq,unsigned int outertime, 
					unsigned int time1,
					unsigned int time2,
					double gain, 
					double gain1,
					double gain2, 
					unsigned int delay1, 
					unsigned int delay2);
	virtual ~nested_allpass();
	bool create(void);
	double tick(double insam);
    void  set_vfreq(double freq);
    void set_vmod(double amount);
    double vtick(double  insam);
	void set_damping(bool damping) { damping_ = damping;}
	bool get_damping() const { return damping_;}
private:
	
	double srate_, outer_gain, ap1_gain,ap2_gain;
	unsigned int ap1_length,ap2_length;
	unsigned int outer_time,ap1_delay,ap2_delay;
	allpassfilter *ap1, *ap2;
	double *buf;
	unsigned int writer,reader;
	// RWD experimental - internal lf damping
	onepole* lp; 
	bool damping_;
	double sr_,lpfreq_;

};


//TODO: comb filter (with lopass), and tapped delay

//basic lowpass for combs

class lowpass1
{
public:
	lowpass1(double filtgain);
	virtual ~lowpass1();
	double tick(double insam);
private:
	double temp,gain,output;	
};


class onepole
{
public:
	onepole(double freq,double srate,int low_high);
	virtual ~onepole();
	double tick(double input);
private:
	double a1,a2,b,lastout;
};

class tonecontrol
{
public:
	tonecontrol(double frq,double dbfac,int type,double srate);
	virtual ~tonecontrol();
	bool create(void);
	double tick(double input);
private:
	double freq, boost, sr,a0, a1, a2, b1, b2;
	double x1,x2,y1,y2;
	int tc_type;
	void lshelve(void);
	void hshelve(void);
};

class lpcomb
{
public:
	lpcomb(unsigned int time, double ingain, double lpcoeff, double prescale);
	virtual ~lpcomb();
	bool create(void);
	double tick(double insam);
private:
	double prescale, gain, lpgain;
	double *combbuf;
	unsigned int rvblen, reader1,writer1;
	lowpass1 *lp;

};


class small_diffuser : public cdp_processor
{
public:
	small_diffuser(unsigned int pre_delay,
					const NESTDATA *apdata1,
					const NESTDATA *apdata2,double fb_gain,double lp_freq,double srate);
	virtual ~small_diffuser();
	bool create(void);
	double tick(double insam);
	void set_damping(bool damping) { damping_ = damping; }
private:
	NESTDATA ap1_data,ap2_data;
	nested_allpass *ap1,*ap2;
	delay *predelay;
	//lowpass1 *lp1;
	onepole *lp1;
	unsigned int predelay_time;
	double lpgain,lpfreq,sr;
	double out1,out2;
	bool damping_;
};

//no public output from postdelay, so internal only
class medium_diffuser : public cdp_processor
{
public:
	medium_diffuser(double post_delay,const NESTDATA *apdata1,
					const NESTDATA *apdata2,
					double gain,
					double lp_freq,
					double srate);
	virtual ~medium_diffuser();
	bool create(void);
	double tick(double insam);
	void set_damping(bool damping) { damping_ = damping;	}
private:
	NESTDATA ap1_data,ap2_data;
	nested_allpass *ap1,*ap2;
	allpassfilter *ap3;				//no public access to this, for now...
	onepole *lp1;
	delay *delay1,*delay2,*postdelay;			
	double md_gain,lpfreq,sr;
	double out1,out2,out3;
	unsigned int postdelay_time;
	bool damping_;
};


class large_diffuser : public cdp_processor
{
public:
	large_diffuser(const NESTDATA *apdata1,
					const NESTDATA *apdata2,
					double gain,
					double lp_freq,
					double srate);
	virtual ~large_diffuser();
	bool create(void);
	double tick(double insam);
	void set_damping(bool damping) { damping_ = damping; }
private:
	void cleanup();
	NESTDATA ap1_data,ap2_data;
	nested_allpass *ap1,*ap2;
	allpassfilter *ap3,*ap4;				//no public access to this, for now...
	onepole *lp1;
	delay *delay1,*delay2,*delay3,*delay4;			
	double ld_gain,lpfreq,sr;
	double out1,out2,out3;
	bool damping_;
};

class moorer : public cdp_processor
{
public:
	moorer(const MOORERDATA *p_mdata,double reverbtime,double damping,double sr);
	virtual ~moorer();
	bool create(void);
	double tick(double insam);
private:
	void cleanup();
	lpcomb *comb1,*comb2,*comb3,*comb4,*comb5,*comb6;
	allpassfilter *ap1,*ap2,*ap3,*ap4;
	MOORERDATA mdata;
	double cgain1,cgain2,cgain3,cgain4,cgain5,cgain6;
	double fgain1,fgain2,fgain3,fgain4,fgain5,fgain6;	
	double reverb_time,dampfac,sr;
	double out,scalefac;
};

/* vmtdelay ****************/
class vcomb4 
{
public:
	vcomb4();
	virtual ~vcomb4();
	bool init(long srate,double length_secs);
	bool init(const MOORERDATA *p_mdata,double reverbtime,double damping,double sr);
	double tick(double vdelay_1,double vdelay_2,double vdelay_3,double vdelay_4,double feedback,double input);
	void setfgains(double fg1,double fg2,double fg3,double fg4) {fb1 = fg1;fb2 = fg2;fb3 = fg3;fb4 = fg4;}
private:
	double*			dl_buf;
	unsigned long	dl_length;
	unsigned  long	dl_input_index;
	double			dl_srate;
	double			fb1,fb2,fb3,fb4;
//	lowpass1		lp1,lp2,lp3,lp4;
	double		gain;
};


#endif // !defined(AFX_REVERBERATOR_H__9DE45E43_83BA_11D1_96D4_444553540000__INCLUDED_)
