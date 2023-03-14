//
//  wavdefs.h
//  

#ifndef wavdefs_h
#define wavdefs_h

# ifndef _WIN32
typedef unsigned short WORD;
typedef unsigned int DWORD;

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
#endif

#ifdef _WIN32
#include <windows.h>
#include <mmreg.h>
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
    WAVEFORMATEXTENSIBLE wxFormat;       /* 40 bytes*/
    DWORD dwVersion;                     /* 4 bytes*/
    DWORD dwDataSize;                    /* 4 bytes: sizeof PVOCDATA data block */
    PVOCDATA data;                       /* 32 bytes*/
} WAVEFORMATPVOCEX;                      /* total 80 bytes*/

/* return -1 for error in fd, id can be >=0  */
int get_pvxfd(int fd,PVOCDATA *pvxdata);

#endif /* wavdefs_h */
