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
 
//diffuse.h
//definition of classes for small, meduim large dense reverberators
// based on gardner, using nested allpasses
#ifndef __DIFFUSE_INCLUDED__
#define __DIFFUSE_INCLUDED__

#include "reverberator.h"
#include "tapdelay.h"

typedef struct nested_allpass_data{
	unsigned int time1;
	unsigned int time2;
	unsigned int time3;
	double gain1;
	double gain2;
	double gain3;
}
NESTDATA;


class small_diffuser {
public:
	small_diffuser(unsigned int pre_delay,
					const NESTDATA *apdata1,
					const NESTDATA *apdata2,double lp_gain,double lp_coeff);
	virtual ~small_diffuser();
	bool create(void);
	float tick(float insam);
	//void clear(void);

private:
	NESTDATA ap1_data,ap2_data;
	nested_allpass *ap1,*ap2;
	delay *predelay;
	lowpass1 *lp1;
	unsigned int predelay_time;
	double lpgain,lpcoeff;
	float out1,out2;
};


#endif
