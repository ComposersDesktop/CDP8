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
 
//NB it is most important to avoid periodicities in these
// using rmresp, best to place source and listener near opposite corners, or both near one corner 
//preset early reflections - could get these from a brkfile, of course
//except small room ; seems best to have src and listener same end of room, to get even earlier reflections
//11 8 3.3
//0.9
//5
//9 3 1.75
//9 5.7 1.29
//normed to 0.5

deltap smalltaps[] = {
	{0.0082,0.500000},
	{0.0121,0.204198},
	{0.0134,0.169094},
	{0.0145,0.143638},
	{0.0170,0.093391},
	{0.0180,0.084478},
	{0.0200,0.067530},
	{0.0219,0.063096},
	{0.0226,0.053176},
	{0.0233,0.044833},
	{0.0237,0.048586},
	{0.0243,0.046058},
	{0.0250,0.043713},
	{0.0256,0.037388},
	{0.0261,0.044475},
	{0.0265,0.034818},
	{0.0271,0.033360},
	{0.0276,0.035772},
	{0.0281,0.034382},
	{0.0285,0.030051},
	{0.0287,0.033059},
	{0.0301,0.027091},
	{0.0304,0.026513},
	{0.0306,0.026200},
	{0.0309,0.023001},
	{0.0319,0.024114},
	{0.0326,0.020657},
	{0.0335,0.021782},
	{0.0340,0.019019},
	{0.0356,0.017387},
	{0.0398,0.017155},
	{0.0408,0.014690},
	{0.0412,0.014424},
	{0.0416,0.014160},
	{0.0425,0.012175},
	{0.0429,0.011971},
	{0.0439,0.011468},
	{0.0451,0.010854}
	};

//dim: 21 16 5
// refl: 0.95
//nrooms 5
//listenerpos
//17 8 2
//srcpos:
//6 9.35 2
//RWD NB does sound worse with with only 32 taps, or refl-order 3
// this is the 'no-compromise' reverb!
deltap mediumtaps[] = 
{
	{0.0332,0.993303},
	{0.0353,0.834879},
	{0.0377,0.729745},
	{0.0447,0.494138},
	{0.0548,0.345328},
	{0.0561,0.313134},
	{0.0570,0.319440},
	{0.0577,0.296282},
	{0.0583,0.290651},
	{0.0598,0.276075},
	{0.0615,0.274630},
	{0.0625,0.240116},
	{0.0627,0.251368},
	{0.0641,0.240392},
	{0.0644,0.226004},
	{0.0684,0.200374},
	{0.0690,0.218340},
	{0.0700,0.201354},
	{0.0713,0.194250},
	{0.0718,0.191280},
	{0.0728,0.176802},
	{0.0740,0.171020},
	{0.0752,0.165815},
	{0.0770,0.166316},
	{0.0778,0.147079},
	{0.0780,0.154272},
	{0.0791,0.149852},
	{0.0816,0.148066},
	{0.0825,0.137700},
	{0.0826,0.130403},
	{0.0836,0.134167},
	{0.0862,0.132653} ,
	{0.0870,0.117789},
	{0.0871,0.123637},
	{0.0881,0.120782},
	{0.0913,0.106847},
	{0.0929,0.114356},
	{0.0937,0.106863},
	{0.0946,0.104723},
    {0.0975,0.103831}
	
};

//normed to 0.5:
//43 31 19
//0.95
//7
//7.6 15 2
//30 23.7 6

deltap largetaps[] = 
{
	{0.0729,0.500000},
	{0.0758,0.439456},
	{0.0975,0.265770},
	{0.0997,0.241550},
	{0.1151,0.190794},
	{0.1162,0.187246},
	{0.1180,0.172387},
	{0.1247,0.154486},
	{0.1320,0.137708},
	{0.1330,0.135754},
	{0.1344,0.139864},
	{0.1346,0.125902},
	{0.1360,0.129780},
	{0.1404,0.115604},
	{0.1449,0.114371},
	{0.1464,0.112074},
	{0.1477,0.115801},
	{0.1492,0.107884},
	{0.1540,0.096167}
};

//Moorer
deltap longtaps[18] = {
					{0.0043,0.841},
					{0.0215,0.504},
					{0.0225,0.491},
					{0.0268,0.379},
					{0.0270,0.380},
					{0.0298,0.346},
					{0.0458,0.289},
					{0.0485,0.272},
					{0.0572,0.192},
					{0.0587,0.193},
					{0.0595,0.217},
					{0.0612,0.181},
					{0.0707,0.180},
					{0.0708,0.181},
					{0.0726,0.176},
					{0.0741,0.142},
					{0.0753,0.167},
					{0.0797,0.134}
};


deltap shorttaps[] = 
{
					{0.0099,1.02},			  //my times!
					{0.0154,0.818},
					{0.0179,0.635},
					{0.0214,0.719},
					{0.0309,0.267},
					{0.0596,0.242}
					};

