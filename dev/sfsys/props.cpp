/*
 * Copyright (c) 1983-2013 Martin Atkins, Richard Dobson and Composers Desktop Project Ltd
 * http://people.bath.ac.uk/masrwd
 * http://www.composersdesktop.com
 *
 This file is part of the CDP System.
 
 The CDP System is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 The CDP System is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with the CDP System; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 *
 */
/*	props.cpp: adapted from speclib3.c
 *	NB this is now real C++ code, not C!	
 *	
 */
//LATEST VERSION OCT97: supercedes all previous!
//can I make this fully portable to both graphic and console apps?

//TODO: Feb 2014: replace sizeof(long) with sizeof(int) everywhere!

#include "stdafx.h"		   //for MFC: def of BOOL, TRUE, FALSE, etc
//#include <propobjs.h>
//use multi-threaded version of sfsys.lib
extern "C" {
#include <sfsys.h>
}

#include "props.h"
char *props_errstr = NULL;
//RWD TODO: also need a copy constructor...
/***************************** HEADREAD *******************************
 *
 * Read input analysis file header data.
 */
//RWD TODO: see Holub: props MUST be a pointer, not ref
BOOL sfheadread(int fd,SFPROPS &props)
{
	long srate,chans,samptype,origsize = 0,origrate=0,origchans = 0,dummy;
	float arate = (float)0.0;
	int wlen=0,dfac=0,specenvcnt = 0,checksum=0;

	props_errstr = NULL;
	if(fd <0){
		props_errstr = "Cannot read Soundfile: Bad Handle";
		return FALSE;
	}
   	if(sfgetprop(fd,"sample rate", (char *)&srate, sizeof(long)) < 0) {
		props_errstr = "Failure to read sample rate";
		return FALSE;
    }
    if(sfgetprop(fd,"channels", (char *)&chans, sizeof(long)) < 0) {
		props_errstr ="Failure to read channel data";
		return FALSE;
    }
    if(sfgetprop(fd,"sample type", (char *)&samptype, sizeof(long)) < 0){
		props_errstr = "Failure to read sample size";
		return FALSE;
    }
	if(!(samptype==SAMP_SHORT  || samptype== SAMP_FLOAT)){
			props_errstr = "unsupported sample type";
			return FALSE;
	}
	props.srate = srate;
	props.chans = chans;
    if(samptype == SAMP_SHORT || samptype == SAMP_FLOAT){	
			props.type = wt_wave;
			props.samptype = (samptype == SAMP_SHORT ? SHORT16 : FLOAT32);
	}
	
	if(props.samptype==FLOAT32) {
		//we know we have floats: is it spectral file?
    	if(sfgetprop(fd,"original sampsize",(char *)&origsize, sizeof(long))<0){
			props_errstr = "Failure to read original sample size";		
    	}
    	if(sfgetprop(fd,"original sample rate",(char *)&origrate,sizeof(long))<0){
			props_errstr = "Failure to read original sample rate";		
    	}
    	if(sfgetprop(fd,"arate",(char *)&arate,sizeof(float)) < 0){
			props_errstr = "Failure to read analysis sample rate";		
    	}
    	if(sfgetprop(fd,"analwinlen",(char *)&wlen,sizeof(int)) < 0){
			props_errstr = "Failure to read analysis window length";		
    	}
    	if(sfgetprop(fd,"decfactor",(char *)&dfac,sizeof(int)) < 0){
			props_errstr = "Failure to read decimation factor";		
    	}
		checksum = origsize + origrate + wlen + dfac + (int)arate;
		if(checksum==0)		//its a wave file
			return TRUE;		
		else {
			if(props_errstr==NULL){	//its a good spectral file
				props.origsize = origsize;
				props.origrate = origrate;
				props.arate = arate;
				props.winlen = wlen;	 //better be wlen+2 ?
				props.decfac = dfac;
			//props.chans = (wlen+2)/2;		//??
				if(sfgetprop(fd,"is a pitch file", (char *)&dummy, sizeof(long))>=0)
					props.type = wt_pitch;
				else if(sfgetprop(fd,"is a transpos file", (char *)&dummy, sizeof(long))>=0)
					props.type = wt_transposition;
				else if(sfgetprop(fd,"is a formant file", (char *)&dummy, sizeof(long))>=0)
					props.type = wt_formant;
				else
					props.type = wt_analysis;
			} else
				return FALSE;	//somehow, got a bad analysis file...
		}
		// get any auxiliary stuff for control file
		//adapted from TW's function in speclibg.cpp
		switch(props.type){
		case(wt_formant):
			if(sfgetprop(fd,"specenvcnt",(char *)&specenvcnt,sizeof(int)) < 0){
				props_errstr = "Failure to read formant size in formant file";
				return FALSE;
			}
			props.specenvcnt = specenvcnt;
			break;
		case(wt_pitch):
		case(wt_transposition):
			if(props.chans != 1){ //RWD: this makes old-style files illegal!
				props_errstr = 	"Channel count does not equal to 1 in transposition file";
    			return FALSE;
			}
			if(sfgetprop(fd,"orig channels", (char *)&origchans, sizeof(long)) < 0) {
				props_errstr = 	"Failure to read original channel data from transposition file";
				return FALSE;
			}
			props.origchans = origchans;
			break;
		default:
			break;
		}
	}			
    return TRUE;
}	    



BOOL sndheadread(int fd,SFPROPS &props)
{
	long srate,chans,samptype,origsize = 0,origrate = 0, origchans = 0,dummy;
	float arate = (float)0.0;
	int wlen=0,dfac=0,specenvcnt = 0,checksum=0;

	props_errstr = NULL;
	if(fd <0){
		props_errstr = "Bad Soundfile Handle";
		return FALSE;
	}
   	if(sndgetprop(fd,"sample rate", (char *)&srate, sizeof(long)) < 0) {
		props_errstr = "Failure to read sample rate";
		return FALSE;
    }
    if(sndgetprop(fd,"channels", (char *)&chans, sizeof(long)) < 0) {
		props_errstr ="Failure to read channel data";
		return FALSE;
    }
    if(sndgetprop(fd,"sample type", (char *)&samptype, sizeof(long)) < 0){
		props_errstr = "Failure to read sample size";
		return FALSE;
    }
	if(!(samptype==SAMP_SHORT || samptype== SAMP_FLOAT)){
			props_errstr = "unsupported sample type";
			return FALSE;
	}

	props.srate = srate;
	props.chans = chans;
    if(samptype == SAMP_SHORT || samptype== SAMP_FLOAT){	
		props.type = wt_wave;
		props.samptype = (samptype == SAMP_SHORT ? SHORT16 : FLOAT32);
	}
	if(props.samptype==FLOAT32) {
		//we know we have floats: is it spectral file?
    	if(sndgetprop(fd,"original sampsize",(char *)&origsize, sizeof(long))<0){
			props_errstr = "Failure to read original sample size";
    	}
    	if(sndgetprop(fd,"original sample rate",(char *)&origrate,sizeof(long))<0){
			props_errstr = "Failure to read original sample rate";
    	}
    	if(sndgetprop(fd,"arate",(char *)&arate,sizeof(float)) < 0){
			props_errstr = "Failure to read analysis sample rate";
    	}
    	if(sndgetprop(fd,"analwinlen",(char *)&wlen,sizeof(int)) < 0){
			props_errstr = "Failure to read analysis window length";
    	}
    	if(sndgetprop(fd,"decfactor",(char *)&dfac,sizeof(int)) < 0){
			props_errstr = "Failure to read decimation factor";
    	}
		//TODO: find a way to guarantee unique checksums...
		checksum = origsize + origrate + wlen + dfac + (int)arate;
		if(checksum==0)		//its a wave file
			return TRUE;		
		else {
			if(props_errstr==NULL){	//its a good spectral file				
				props.origsize = origsize;
				props.origrate = origrate;
				props.arate = arate;
				props.winlen = wlen;
				props.decfac = dfac;
				//props.chans = (wlen+2)/2;		//??

				if(sndgetprop(fd,"is a pitch file", (char *)&dummy, sizeof(long))>=0)
					props.type = wt_pitch;
				else if(sndgetprop(fd,"is a transpos file", (char *)&dummy, sizeof(long))>=0)
					props.type = wt_transposition;
				else if(sndgetprop(fd,"is a formant file", (char *)&dummy, sizeof(long))>=0)
					props.type = wt_formant;
				else
					props.type = wt_analysis;
			} else
				return FALSE;	//somehow, got a bad analysis file...
		}
		// get any auxiliary stuff for control file
		//adapted from TW's function in speclibg.cpp
		switch(props.type){
		case(wt_formant):
			if(sndgetprop(fd,"specenvcnt",(char *)&specenvcnt,sizeof(int)) < 0){
				props_errstr = "Failure to read formant size in formant file";
				return FALSE;
			}
			props.specenvcnt = specenvcnt;
			break;
		case(wt_pitch):
		case(wt_transposition):
			if(props.chans != 1){
				props_errstr = 	"Channel count not equal to 1 in transposition file";
    			return FALSE;
			}
			if(sndgetprop(fd,"orig channels", (char *)&origchans, sizeof(long)) < 0) {
				props_errstr = 	"Failure to read original channel data from transposition file";
				return FALSE;
			}
			props.origchans = origchans;
			break;
		default:
			break;
		}

    }
    return TRUE;
}	    
// next is for SOUND files only...
//TODO: full support for analysis and control files!
BOOL sfwave_headwrite(int fd,const SFPROPS &props)
{
	
#ifdef _DEBUG
     ASSERT(fd >= 0);
	 ASSERT(props.samptype==SHORT16 || props.samptype==FLOAT32);
	 ASSERT(props.srate > 0);
	 ASSERT(props.chans > 0);
#endif
	int srate = props.srate;
	int chans = props.chans;
	int samptype;

	if(fd <0){
		props_errstr = "Cannot write soundfile: bad Handle";
		return FALSE;
	}
	if(props.type == wt_wave)
		samptype = (props.samptype == SHORT16 ? SAMP_SHORT : SAMP_FLOAT);  //within sfsys,mostly safe assumption...
	else {
		props_errstr = "Not a wave file";
		return FALSE;
	}
		
   	if(sfputprop(fd,"sample rate", (char *)&srate, sizeof(long)) < 0) {
		props_errstr = "Failure to write sample rate";
		return FALSE;
    	}
    	if(sfputprop(fd,"channels", (char *)&chans, sizeof(long)) < 0) {
		props_errstr ="Failure to write channel data";
		return FALSE;
    	}
    	if(sfputprop(fd,"sample type", (char *)&samptype, sizeof(long)) < 0){
		props_errstr = "Failure to write sample size";
		return FALSE;
    	}
      	return TRUE;
}

fileprops::~fileprops()
{
}

fileprops::fileprops()
{
	srate 	   = 0L;		 		
	stype 	   = -1L;
	filetype   = UNKNOWN;
	channels   = 0L;
	origstype  = -1L;
	origrate   = 0L;
	origchans  = 0L;
	specenvcnt = 0;
	Mlen 	   = 0;
	Dfac 	   = 0;
	arate 	   = 0.0f;
}

//TODO: when I have unique checksums, can just compare them
const fileprops& fileprops::operator=(const fileprops &rhs)
{
	if(!(rhs == *this)){		 //use  my operator==; TODO: define operator!= as well...
			srate 		= rhs.srate;
			channels 	= rhs.channels;
			stype 		= rhs.stype;
			origstype 	= rhs.origstype;
			origrate 	= rhs.origrate;
			Mlen 		= rhs.Mlen;
			Dfac 		= rhs.Dfac;
			arate 		= rhs.arate;
			origchans 	= rhs.origchans;
			specenvcnt 	= rhs.specenvcnt;	//RWD: comes from?
	}
	return *this;
}

int fileprops::operator==(const fileprops &rhs) const
{
		  return (srate	== rhs.srate
			 && channels 	== rhs.channels
			&&	stype 		== rhs.stype
			&&	origstype 	== rhs.origstype
			&&	origrate 	== rhs.origrate
			&&	Mlen 		== rhs.Mlen
			&&	Dfac 		== rhs.Dfac
			&&	arate 		== rhs.arate
			&&	origchans 	== rhs.origchans
			&&	specenvcnt 	== rhs.specenvcnt);
}







