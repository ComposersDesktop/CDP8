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
 
//props.h
// latest stab at complete sfprops def including new 32bit float sound format
// needs linking with sfsys97.lib for that to happen...
// RWD SEP97: c++ version

#ifndef __PROPS_INCLUDED__
#define __PROPS_INCLUDED__
//RWD: NB this does not include a TEXT filetype TW-style...
#ifdef __cplusplus
extern "C" {
#endif
#include <sfsys.h>
#ifdef __cplusplus
}
#endif

#ifndef __AFX_H__

# ifndef FALSE
#  define FALSE false
# endif
# ifndef TRUE
#  define TRUE true
# endif
# ifndef BOOL
#  define BOOL bool
# endif
#endif



//typedef enum {wave,analysis,formant,transposition,pitch} wavetype;
//typedef enum {SHORT8,SHORT16,FLOAT32}					sampletype;
//typedef enum format {WAVE,AIFF}							fileformat;

typedef enum {
	UNKNOWN = 0,		//RWD added
	TEXTFILE,			//(1)	
	SNDFILE,			//(2)
	ANALFILE,			//(3)
	PITCHFILE,			//(4)
	TRANSPOSFILE,		//(5)
	FORMANTFILE			//(6)
} tw_filetype; 					//TW version used in SPEC, GSPEC


#ifdef NOTDEF

typedef struct sfprops 
{
	long		srate;
	long		chans;
	wavetype	type;
	sampletype	samptype;		//RWD 07:97
	fileformat	format;			//RWD.04.98
	long		origsize;
	long		origrate;
	long		origchans;		/* pitch, formant,transpos only */
	int			specenvcnt;		/* formants only */	
	float		arate;
	int			winlen;			// aka Mlen
	int			decfac;			// aka Dfac
	//double chan_width;		//see, for example, spechoru.c
	//double half_chan_width;
} SFPROPS;

#endif

// must be decleared elsewhere for Cpp progs! 
//TW version from specg2.h

#ifdef __cplusplus
struct fileprops {		// may not need this anywhere else...
	long	srate;
	long   	channels;
	tw_filetype filetype;		 		
	long   	stype;
	long   	origstype; 		/* float files ony from here downwars */
	long   	origrate;	
	long   	origchans;	/* pitch, formant,transpos only */
	int	specenvcnt;	/* formants only */
	float  	arate;
	int    	Mlen;
	int    	Dfac;
	fileprops();		  //constructor from spec.cpp
	virtual ~fileprops();
	const fileprops& operator=(const fileprops &);
	int operator==(const fileprops &) const;
};

typedef struct fileprops *fileptr;		  // probably embed whole struct (or class?)

//these are only semi_object-oriented...should be derived from sffile class...
//TODO: reconvert read funcs to use pointer to props
BOOL sfheadread(int fd,SFPROPS &props);
BOOL sndheadread(int fd,SFPROPS &props);
BOOL sfwave_headwrite(int fd,const SFPROPS &props);		//const...
#endif

extern char *props_errstr;

#endif
