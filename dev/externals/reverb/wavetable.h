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
 

// wavetable.h: interface for the fast_lfo class.
// Feb 2009 added gaussian option to random
//FWD 3.20: eliminate warning of unused private var (phsmask)

//////////////////////////////////////////////////////////////////////


#if !defined(AFX_WAVETABLE_H__2F961225_3FF4_11D2_96D4_444553540000__INCLUDED_)
#define AFX_WAVETABLE_H__2F961225_3FF4_11D2_96D4_444553540000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

typedef enum lfowavetype {LFO_SINE,LFO_TRIANGLE,LFO_SAWUP,LFO_SAWDOWN,LFO_SQUARE,LFO_RANDOM,LFO_RND_SH,
LFO_RAND_TPDF,LFO_RAND_GAUSS,LFO_RAND_EXP, LFO_RANDH_TPDF,
/*LFO_RANDH_GAUSS,LFO_RANDH_EXP, */
LFO_NUM_WAVES } LfoWaveType;

enum {PVS_LFO_FREQ_LORANGE, PVS_LFO_FREQ_HIRANGE};


typedef struct lfoparam
{
	double freq;
	double modrange;
} LfoParam;



class fastlfo {
	typedef double (fastlfo:: *tickfunc)(void);
public:

	fastlfo();
	virtual ~fastlfo() {}
	long init(double srate, double normphase = 0.0,long seedval = 0,unsigned long ksmps = 1);
	double		tick(void)							{ return (this->*tf)(); }
	tickfunc	tf;
	void		set_freq(double freq)				{ param.freq = freq;	}
	double		get_freq(void)				const	{ return param.freq;	}
	void		set_mod(double mod)					{ param.modrange = mod;	}
	double		get_mod(void)				const	{ return param.modrange;}
	void		reset(double phase);
	bool		set_WaveType(LfoWaveType type);
	void		set_tpdf()              		    { tpdf = true; gauss = biexp = false;}
    void        set_white()                         {  tpdf = gauss = biexp = false;}
    void        set_biexp()                         {  tpdf = gauss = false; biexp = true;}
    void        set_flat()                          { tpdf = gauss = biexp = false;}
    void        set_gaussian()                      { tpdf = biexp = false; gauss = true;}
	void		sync_phase(double phase, double offset, double phaseincr);
	void		set_sync(bool onoff)				{ b_sync = onoff;}
    double      csrand();                   
private:
	double	sinetick(void);
	double	tritick(void);
	double	squaretick(void);
	double	sawuptick(void);
	double	sawdowntick(void);
	double	randomtick(void);
	double	randhtick(void);
    double  rand_tpdf_tick(void);
    double  rand_gauss_tick(void);
    double  rand_exp_tick(void);
    double  randh_tpdf_tick(void);
 //   double  randh_gauss_tick(void);
 //   double  randh_exp_tick(void);

	long	randint31(long seed31);

	double	m_srate, m_inv_srate;
	double twopiovrsr;
	double curfreq;
	double curphase;
	double incr;
	LfoParam	param;
	LfoWaveType  m_cur_WaveType;  
	/* vars for random oscs, with a little help from Csound!*/
	long		phs;
	long		kicvt;
//	long		phsmask;   // currently not used
	long		rand;
	double		num1,num2;
	double		dfdmax;
	/* for krate output */
	unsigned long		ksamps;
	unsigned long		kcount;
	double		lastval;
	bool	tpdf;
    bool gauss;
    bool biexp;
	// for lfo sync
	bool b_sync;
	double		offset;
	// for random oscs
	long curphs;
	long phs_incr;
	long phs_offset;
    /* for csrand */
    long csseed;
};

#endif // !defined(AFX_WAVETABLE_H__2F961225_3FF4_11D2_96D4_444553540000__INCLUDED_)


