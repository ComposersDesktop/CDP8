/*pvfileio.h: header file for PVOC_EX file format */
/*
 * Copyright (c) 2000,2022 Richard Dobson and Composers Desktop Project Ltd
 * http://www.rwdobson.com
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
/* Initial Version 0.1 RWD 25:5:2000 all rights reserved: work in progress! */
/* NB a special version of this file is used in Csound - do not auto-replace! */

#ifndef __PVFILEIO_H_INCLUDED
#define __PVFILEIO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#include <windows.h>
#endif

#include <stdint.h>
#include "wavdefs.h"
#include "pvdefs.h"
/* need to skip these defs for osx, use above headers */
#if 0
#ifndef WIN32
    
#ifndef _WIN32
    typedef  unsigned short WORD;
    typedef unsigned int DWORD;
#endif

typedef struct _GUID 
{ 
    DWORD           Data1;
    WORD            Data2; 
    WORD            Data3; 
    unsigned char   Data4[8]; 
} GUID;


typedef struct /*waveformatex */{ 
    WORD  wFormatTag; 
    WORD  nChannels; 
    DWORD nSamplesPerSec; 
    DWORD nAvgBytesPerSec; 
    WORD  nBlockAlign; 
    WORD  wBitsPerSample; 
    WORD  cbSize; 
} WAVEFORMATEX; 
#endif

typedef struct pvoc_data {              /* 32 bytes*/
    WORD    wWordFormat;                /* pvoc_wordformat */
    WORD    wAnalFormat;                /* pvoc_frametype */
    WORD    wSourceFormat;              /* WAVE_FORMAT_PCM or WAVE_FORMAT_IEEE_FLOAT*/
    WORD    wWindowType;                /* pvoc_windowtype */
    DWORD   nAnalysisBins;              /* implicit FFT size = (nAnalysisBins-1) * 2*/
    DWORD   dwWinlen;                   /* analysis winlen,samples, NB may be <> FFT size */
    DWORD   dwOverlap;                  /* samples */
    DWORD   dwFrameAlign;               /* usually nAnalysisBins * 2 * sizeof(float) */
    float   fAnalysisRate;
    float   fWindowParam;               /* default 0.0f unless needed */
} PVOCDATA;

typedef struct {
    WAVEFORMATEX    Format;              /* 18 bytes:  info for renderer as well as for pvoc*/
    union {                              /* 2 bytes*/
        WORD wValidBitsPerSample;        /*  as per standard WAVE_EX: applies to renderer*/
        WORD wSamplesPerBlock;          
        WORD wReserved;                  
                                        
    } Samples;
    DWORD           dwChannelMask;      /*  4 bytes : can be used as in standrad WAVE_EX */
                                        
    GUID            SubFormat;          /* 16 bytes*/
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;


typedef struct {
    WAVEFORMATEXTENSIBLE wxFormat;       /* 40 bytes*/
    DWORD dwVersion;                     /* 4 bytes*/
    DWORD dwDataSize;                    /* 4 bytes: sizeof PVOCDATA data block */ 
    PVOCDATA data;                       /* 32 bytes*/
} WAVEFORMATPVOCEX;                      /* total 80 bytes*/
#endif

    /* Renderer information: source is presumed to be of this type */
typedef enum pvoc_sampletype {STYPE_16,STYPE_24,STYPE_32,STYPE_IEEE_FLOAT} pv_stype;
    
/* at least VC++ will give 84 for sizeof(WAVEFORMATPVOCEX), so we need our own version*/
#define SIZEOF_FMTPVOCEX (80)
/* for the same reason:*/
#define SIZEOF_WFMTEX (18)
#define PVX_VERSION     (1)
/******* the all-important PVOC GUID 

 {8312B9C2-2E6E-11d4-A824-DE5B96C3AB21}

**************/

extern  const GUID KSDATAFORMAT_SUBTYPE_PVOC;


/* pvoc file handling functions */
    /* NB: frame count always disregards chans */
const char *pvoc_errorstr(void);
int32_t init_pvsys(void);
int32_t  pvoc_createfile(const char *filename, 
                     DWORD fftlen,DWORD overlap, DWORD chans,
                     DWORD format,int32_t srate,
                     pv_stype stype,pv_wtype wtype,float wparam,float *fWindow,DWORD dwWinlen);
    int32_t pvoc_getpvxprops(int32_t ifd, PVOCDATA *data);
int32_t pvoc_openfile(const char *filename,PVOCDATA *data,WAVEFORMATEX *fmt);
int32_t pvoc_closefile(int32_t ofd);
int32_t pvoc_putframes(int32_t ofd,const float *frame,int32_t numframes);
int32_t pvoc_getframes(int32_t ifd,float *frames,DWORD nframes);
    /* framecount is raw count, if stereo, calc framecount / 2 for stereo-frame value, e.g. to calc duration */
int32_t pvoc_framecount(int32_t ifd);
int32_t pvoc_rewind(int32_t ifd,int32_t skip_first_frame);      /* RWD 14:4:2001 */
int32_t pvoc_framepos(int32_t ifd);                         /* RWD Jan 2014 */
int32_t pvoc_seek_mcframe(int32_t ifd, int32_t offset, int32_t mode);
int32_t pvsys_release(void);
    /* tell pvsys to update pvocdata on close; return 0 for success */
int32_t pvoc_set_needsupdate(int32_t ifd);
    //returns 0 for success
    int32_t pvoc_canupdate(int32_t ifd);
    //ONLY when creating new file using 2-stages.called last thing before file close.
//    int32_t pvoc_updateprops(int32_t ifd, const PVOCDATA *data);
    int32_t pvoc_update_closefile(int ofd, const PVOCDATA *data,const WAVEFORMATEXTENSIBLE *wfx);
    int32_t pvoc_getdatasize_bytes(int32_t fd); // mainly for benefit of dirsf etc
#ifdef __cplusplus
}
#endif

#endif
