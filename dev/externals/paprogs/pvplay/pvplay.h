/*
 * Copyright (c) 1983-2020 Richard Dobson and Composers Desktop Project Ltd
 * http://www.rwdobson.com
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
 
//
//  pvplay.h
//  
//
//  Created by Archer on 28/01/2013.
//  Copyright (c) 2013 __MyCompanyName__. All rights reserved.
//

#ifndef _pvthreads_h
#define _pvthreads_h

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>
#include <signal.h>
#include <assert.h>

#include "portaudio.h"
//#include "porttime.h"
#include "pa_ringbuffer.h"
#include "pa_util.h"

#ifdef _WIN32
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <conio.h>
#include <process.h>
#include "pa_win_wmme.h"
#include "pa_win_ds.h"
#endif


#ifdef unix
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <pthread.h>
#include <ctype.h>
/* in portsf.lib */
int stricmp(const char *a, const char *b);
int strnicmp(const char *a, const char *b, const int length);

#endif

#ifdef CPLONG64

#define MYLONG int
#else
#define MYLONG long
#endif


/* need this to avoid clashes of GUID defs, etc */
#ifdef _WIN32
#ifndef _WINDOWS
#define _WINDOWS
#endif
#endif



/* sfsys just for analysis files! */
#ifdef __cplusplus
extern "C" {	
#endif
#include "sfsys.h"
#include "fmdcode.h"
#include "pvfileio.h"
#ifdef __cplusplus
}
#endif

#include "pvpp.h"

#if defined MAC || defined unix
#include <aaio.h>
#endif

#ifdef MAC
#include <libkern/OSAtomic.h>
#endif

#ifndef min
#define min(x,y)  ((x) < (y) ? (x) : (y))
#endif

#ifdef _WIN32
extern HANDLE ghEvent;
#endif

// paplayt
//typedef struct {
//	int sfd;
//    int chans;
//	psf_buffer *outbuf;
//	unsigned long frames_played;
//	int last_buffer; /* init to 1; at detected eof send one extra buffer to flush, and set this to 0 */
//} psfdata;


typedef struct {
	float *buffer;
	long nframes;
	MYLONG used;
}
psf_buffer;

typedef struct {
    PaUtilRingBuffer ringbuf;
    PaStream *stream;
    PaTime startTime;
    PaTime lastTime;
	float *ringbufData;
    float *inbuf;   //for decoding and other tx; used as temp working buffer for read thread func.
    /* for memory read func */
    float *membuf;
/* analysis file vars */
    // pv objects
    phasevocoder *pv_l;
    phasevocoder *pv_r;
    float *analframe1;
    float *analframe2;
    int anal_framesize; // in floats
    int fftsize;        // sample frames from resynthesis of analysis frame(s)
    float *outbuf_l;    // stereo pvx files - needed?
    float *outbuf_r;
    
//    float** orderptr; // for chan mapping;
#ifdef _WIN32
    void *hThread;
    HANDLE hTimer;
    HANDLE hTimerCount;
#endif
  
#ifdef unix
    pthread_t hThread;
#endif
    fmhdecodefunc decodefunc;
    fmhcopyfunc copyfunc;
    double gain;
	unsigned long frames_played;  // for .ana, .pvx files, these are analysis frames
    unsigned long from_frame;
    unsigned long to_frame;
    unsigned long memFramelength;
    unsigned long memFramePos;
    unsigned long inbuflen;
    unsigned long current_frame;
    int srate;
    int inchans;
    int outchans;
    int flag;
	int ifd; // soundfiles, .ana files
    int pvfile; // pvx file
    pvoc_frametype inframetype;
#ifdef DEBUG
    int ofd;
#endif
    int do_decode;
//    int do_mapping;
    int play_looped;
    int finished;
} psfdata;

#ifdef NOTDEF
typedef struct {
    PaStream *stream;
    PaTime starttime /* = Pa_GetStreamTime(stream)*/;
} ptimedata;
#endif

//extern int file_playing;
//extern psfdata *g_pdata = NULL;  // for timer interrupt routine

void stereo_interls(const float *in_l,const float *in_r,float *out,long insize);
int show_devices(void);


/* handlers */
void playhandler(int sig);
void finishedCallback(void *userData);
#ifdef unix
void alarm_wakeup (int i);
#endif
#ifdef _WIN32
VOID CALLBACK TimerCallback(PVOID lpParam, BOOLEAN TimerOrWaitFired);
#endif




/* playback thread functions */


#ifdef unix
// TODO: unix return type should be void*
typedef void* (*threadfunc)(void*);
void*  threadFunctionReadFromRawFile(void* ptr);
void*  threadFunctionReadFromAnalFile(void* ptr);
void*  threadFunctionReadFromPVXFile(void* ptr);
#endif


#ifdef _WIN32
typedef unsigned int (__stdcall *threadfunc)(void*);
// soundfile
unsigned int __stdcall threadFunctionReadFromRawFile(void* ptr);
unsigned int __stdcall threadFunctionReadFromAnalFile(void* ptr);
unsigned int __stdcall threadFunctionReadFromPVXFile(void* ptr);
#endif


#endif
