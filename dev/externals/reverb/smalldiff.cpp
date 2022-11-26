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
 
//smalldiff.cpp
//implements diffuse reverberator for small rooms, based on gardner nested allpasses
#include "reverberator.h"

small_diffuser::small_diffuser(unsigned int pre_delay, 
							   const NESTDATA *apdata1,
							   const NESTDATA *apdata2,double fb_gain,double lp_coeff){

	  ap1_data = *apdata1;
	  ap2_data = *apdata2;
	  predelay_time = pre_delay;
	  lpgain = fb_gain;
	  lpcoeff = lp_coeff;
	  ap1 = ap2 = 0;
	  lp1 = 0;
	  predelay = 0;
	  out1 = out2 = 0.0f;
}

small_diffuser::~small_diffuser()
{
	delete ap1;
	delete ap2;
	if(predelay)
		delete predelay;
	delete lp1;
}

bool small_diffuser::create(void)
{
	if(ap1_data.time1 == 0 || ap1_data.gain1 <= 0.0)
		return false;
	if(ap1_data.time2 ==0 || ap1_data.gain2 <= 0.0)
		return false;
	if(ap2_data.time1 == 0 || ap2_data.gain1 < 0.0)
		return false;
	if(ap2_data.time2 ==0 || ap2_data.gain2 < 0.0)
		return false;
	ap1 = new nested_allpass(ap1_data.time1,ap1_data.time2,ap1_data.time3,
							ap1_data.gain1,ap1_data.gain2,ap1_data.gain3,ap1_data.delay1,ap1_data.delay2);
	if(!ap1->create())
		return false;
	if(ap2_data.gain1 != 0.0){	 //allow ap to eliminate second block
		ap2 = new nested_allpass(ap2_data.time1,ap2_data.time2,ap2_data.time3,
							ap2_data.gain1,ap2_data.gain2,ap2_data.gain3,ap2_data.delay1,ap2_data.delay2);
		if(!ap2->create())
			return false;
	}
	if(predelay_time > 0){
		predelay = new delay(predelay_time,1.0);
		if(!predelay->create())
			return false;
	}
	if(lpcoeff > 0.0)
		lp1 = new lowpass1(lpcoeff);
	return true;
}


float small_diffuser::tick(float insam) 
{
	float ip = insam;
	double temp;
	if(lp1)
		temp = lp1->tick((out2 + out1)*0.5f);
	else
		temp = (out2 + out1)*0.5f;
	ip += (float)(lpgain * temp);	
	if(predelay)
		ip = predelay->tick(ip);
	out1 = ap1->tick(ip);
	if(ap2)
		out2 = ap2->tick(out1);
	return (out1 + out2)  /** 0.5f*/;
}
