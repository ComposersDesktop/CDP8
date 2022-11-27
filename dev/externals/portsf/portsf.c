/* Copyright (c) 1999-2009 Richard Dobson
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation
 files (the "Software"), to deal in the Software without
 restriction, including without limitation the rights to use,
 copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following
 conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.
 */

/* Changes Oct 28 2002: support overwrite mode (seek-back->read->wrtite) */
/* fix bug in floats WAVE header reading! */
/* more comprehensive use of assert in _DEBUG mode (still not complete ....) */
/* sundry tidy-ups inspired by Intel compiler messages! */
/* changes Apr 27 2003: fix reversal bug in wave format writing on big-endian platforms */
/* changes Aug 2003: added basic TPDF dither to 16bit output */
/* Nov 2003: fixed aiffsize bug in aiff write func: 8 bytes too big! */
/* Jan 2004 fixed error reading from WAVE_EX file */

/*Oct 19 2006: cleared up some memory leaks on SFFILE (psf_sndOpen etc)*/
/* July 2009: revised to use fget/setpos, 4GB file support, 64bit platforms
 Move to 2LSB TPDF dither
 */

/* POST BOOK!*/
/* OCT 2009 ADDED MC_CUBE, completed bformat support */
/* fixed bad omission in byteswaaping wavex elements! */
/* added recognition of speaker layouts */
/* corrected absent assignment of chmask on little-endian */

/* Apr 2010 upped MAXFILES to 256! */
/* Aug 2010, and now to 512! */

/* Aug 2012 TODO (?):  add running check for 4GB limit to writeFrame funcs */
/* Aug 2012 corrected SPKRS_MONO value (portsf.h) */
/* Nov 2013: added SPKRS_6_1 to list */
/* Mar 2020: fix fault parsing floats amb files! */

#include <stdio.h>
#ifdef unix
#include <unistd.h>
#endif
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "ieee80.h"
#ifdef _DEBUG
#include <assert.h>
#endif

#include <portsf.h>

#ifndef DBGFPRINTF
# ifdef _DEBUG
# define DBGFPRINTF(a) fprintf a
# else
# define DBGFPRINTF(a)
# endif
#endif

#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif
#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE (8)
#endif
#ifndef WIN32
#include <ctype.h>
int stricmp(const char *a, const char *b);
int strnicmp(const char *a, const char *b, const int length);
#endif

#ifdef linux
#define POS64(x) (x.__pos)
#else
#define POS64(x) (x)
#endif


/* probably no good for 64bit platforms */
#define REVDWBYTES(t)   ( (((t)&0xff) << 24) | (((t)&0xff00) << 8) | (((t)&0xff0000) >> 8) | (((t)>>24) & 0xff) )
#define REVWBYTES(t)    ( (((t)&0xff) << 8) | (((t)>>8) &0xff) )
#define TAG(a,b,c,d)    ( ((a)<<24) | ((b)<<16) | ((c)<<8) | (d) )
/* RW changed 15:10:2002 - one less! */
/* RWD Dec 2009 changed back again for bit-accurate scaling */
#define MAX_16BIT  (32768.0)
#define MAX_32BIT  (2147483648.0)

#define AIFC_VERSION_1 (0xA2805140)
/*pstring for AIFC  - includes the pad byte*/
static const char aifc_floatstring[10] = { 0x08,'F','l','o','a','t',0x20,'3','2',0x00};
static const char aifc_notcompressed[16] = {0x0e,'n','o','t',0x20,'c','o','m','p','r','e','s','s','e','d',0x00};

static float trirand();
static psf_channelformat get_speakerlayout(DWORD chmask,DWORD chans);
static double inv_randmax  = 1.0 / RAND_MAX;
static const float dclip16 = (float)(32767.0/32768.0);
static const float dclip24 = (float)(8388607.0/8388608.0);
static const float dclip32 = (float)(2147483647.0/2147483648.0);

/* we need the standard Windows defs, when compiling on other platforms.
 <windows.h> defines _INC_WINDOWS
 */
#ifndef _INC_WINDOWS

#define WAVE_FORMAT_PCM         (0x0001)

typedef union {
    int lsamp;
    float fsamp;
    unsigned char bytes[4];
} SND_SAMP;

typedef struct _GUID 
{ 
    unsigned int      Data1;
    unsigned short    Data2;
    unsigned short    Data3;
    unsigned char     Data4[8];
} GUID; 


typedef struct  {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    
} WAVEFORMAT;


typedef struct { 
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
} WAVEFORMATEX; 
#endif


/* basic support for WAVE_FORMAT_EXTENSIBLE */

typedef struct {
    WAVEFORMATEX    Format;             /* 18 bytes */
    union {
        WORD wValidBitsPerSample;       /* bits of precision  */
        WORD wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
        WORD wReserved;                 /* If neither applies, set to */
        /* zero. */
    } Samples;
    DWORD    dwChannelMask;             /* which channels are */
    /* present in stream  */
    GUID     SubFormat;
} WAVEFORMATEXTENSIBLE;

/* sizeof(WAVEFORMATEXTENSIBLE) gives size plus alignment padding; not good here */
/* size = 18 + 2 + 4 + 16 */
#define sizeof_WFMTEX  (40)

/* std WAVE-EX GUIDS from <ksmedia.h> */
static const GUID  KSDATAFORMAT_SUBTYPE_PCM = {0x00000001,0x0000,0x0010,
    {0x80,
        0x00,
        0x00,
        0xaa,
        0x00,
        0x38,
        0x9b,
        0x71}};

static const GUID  KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {0x00000003,0x0000,0x0010,
    {0x80,
        0x00,
        0x00,
        0xaa,
        0x00,
        0x38,
        0x9b,
        0x71}};

static const GUID SUBTYPE_AMBISONIC_B_FORMAT_PCM = { 0x00000001, 0x0721, 0x11d3, 
    { 0x86,
        0x44,
        0xc8,
        0xc1,
        0xca,
        0x0,
        0x0,
        0x0 } };


static const GUID SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT = { 0x00000003, 0x0721, 0x11d3, 
    { 0x86,
        0x44,
        0xc8,
        0xc1,
        0xca,
        0x0,
        0x0,
        0x0 } };

#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT  (0x0003)
#endif
#ifndef WAVE_FORMAT_EXTENSIBLE
#define WAVE_FORMAT_EXTENSIBLE  (0xfffe)
#endif
/*RWD Feb 2010 */
#define WARNSTRING_SIZE (64)
/******** the private structure holding all sfile stuff */
enum lastop {PSF_OP_READ,PSF_OP_WRITE};

typedef struct psffile {
    FILE            *file;
    char            *filename;
    DWORD           curframepos;    /* for read operations */
    DWORD           nFrames;        /* multi-channel sample frames */
    int             isRead;         /* how we are using it */
    int             clip_floats;
    int             rescale;
    float           rescale_fac;
    psf_format      riff_format;
    /*int               isSeekable;*/    /* any use ? */
    int             is_little_endian;
    psf_stype       samptype;       /* = nBlockAlign / nChannels */
    fpos_t          dataoffset;     /* = sizeof(header) */
    fpos_t          fmtoffset;
    fpos_t          peakoffset;
    WAVEFORMATEXTENSIBLE fmt;           /* serves all WAVE,AIFF support.*/
    psf_channelformat chformat;
    PSF_CHPEAK      *pPeaks;
    time_t          peaktime;
    fpos_t          lastwritepos;
    int             lastop;         /* last op was read or write? */
    int             dithertype;
    /* RWD Feb 2010 ; to warn user of ill-formed files*/
    int             illformed;
    char            warnstring[WARNSTRING_SIZE];
} PSFFILE;


static int compare_guids(const GUID *gleft, const GUID *gright)
{
    const char *left = (const char *) gleft, *right = (const char *) gright;
    return !memcmp(left,right,sizeof(GUID));
}



#define psf_maxfiles (512)

/* could make this dynamically allocated, via psf_init, one day, if it matters. */

static PSFFILE *psf_files[psf_maxfiles];

/* return 0 for success, non-zero for error */
int psf_init(void)
{
    int i;
    
    for(i=0;i < psf_maxfiles;i++)
        psf_files[i] = NULL;
    /* do any other inits we need.... */
    return 0;
}

/* return zero for success, non-zero for error*/
static int psf_release_file(PSFFILE *psff)
{
    int rc = 0;
#ifdef _DEBUG
    assert(psff);
#endif
    
    if(psff->file){
        rc = fclose(psff->file);
        if(rc)
            return rc;
        psff->file = NULL;
    }
    if(psff->filename){
        free(psff->filename);
        psff->filename = NULL;
    }
    if(psff->pPeaks) {
        free(psff->pPeaks);
        psff->pPeaks = NULL;
    }
    return rc;
}

/* return zero for success, non-zero for error*/
int psf_finish(void)
{
    int i,rc = 0;
    for(i=0;i < psf_maxfiles;i++) {
        if(psf_files[i]!= NULL){
#ifdef _DEBUG
            printf("sfile %s not closed: closing.\n",psf_files[i]->filename);
#endif
            rc = psf_release_file(psf_files[i]);
            /* an alternative is to continue, and write error info to a logfile */
            if(rc)
                return rc;
            
        }
        free(psf_files[i]);
        psf_files[i] = NULL;
    }
    return rc;
}


/* thanks to the SNDAN programmers for this! */
/* return 0 for big-endian machine, 1 for little-endian machine*/
/* probably no good for 16bit swapping though */
static int byte_order()                 
{                           
    int   one = 1;
    char* endptr = (char *) &one;
    return (*endptr);
}


static void fmtSwapBytes(PSFFILE *sfdat)
{
    WAVEFORMATEX  *pfmt = (WAVEFORMATEX *) &(sfdat->fmt.Format);
    
    pfmt->wFormatTag    = (WORD) REVWBYTES(pfmt->wFormatTag);
    pfmt->nChannels     = (WORD) REVWBYTES(pfmt->nChannels);
    pfmt->nSamplesPerSec    = REVDWBYTES(pfmt->nSamplesPerSec);
    pfmt->nAvgBytesPerSec   = REVDWBYTES(pfmt->nAvgBytesPerSec);
    pfmt->nBlockAlign   = (WORD) REVWBYTES(pfmt->nBlockAlign);
    pfmt->wBitsPerSample    = (WORD) REVWBYTES(pfmt->wBitsPerSample);
}

static void fmtExSwapBytes(PSFFILE *sfdat)
{
    WAVEFORMATEXTENSIBLE  *pfmtEx =  &(sfdat->fmt);
    WAVEFORMATEX          *pfmt   = &(pfmtEx->Format);
    
    pfmt->wFormatTag    = (WORD) REVWBYTES(pfmt->wFormatTag);
    pfmt->nChannels     = (WORD) REVWBYTES(pfmt->nChannels);
    pfmt->nSamplesPerSec    = REVDWBYTES(pfmt->nSamplesPerSec);
    pfmt->nAvgBytesPerSec   = REVDWBYTES(pfmt->nAvgBytesPerSec);
    pfmt->nBlockAlign   = (WORD) REVWBYTES(pfmt->nBlockAlign);
    pfmt->wBitsPerSample    = (WORD) REVWBYTES(pfmt->wBitsPerSample);
    pfmt->cbSize            = (WORD) REVWBYTES(pfmt->cbSize);
    // OCT 09: missing!
    pfmtEx->Samples.wValidBitsPerSample = (WORD) REVWBYTES(pfmtEx->Samples.wValidBitsPerSample);
    pfmtEx->dwChannelMask     = (DWORD) REVDWBYTES(pfmtEx->dwChannelMask);
    /* we swap numeric fields of GUID, but not the char string */
    pfmtEx->SubFormat.Data1 = REVDWBYTES(pfmtEx->SubFormat.Data1);
    pfmtEx->SubFormat.Data2 = (WORD) REVWBYTES(pfmtEx->SubFormat.Data2);
    pfmtEx->SubFormat.Data3 = (WORD) REVWBYTES(pfmtEx->SubFormat.Data3);
}

static int check_guid(PSFFILE *sfdat)
{
    /* expects a GUID to be loaded already into sfdat.*/
    if(sfdat->riff_format != PSF_WAVE_EX)
        return 1;
    
    if(compare_guids(&(sfdat->fmt.SubFormat),&(KSDATAFORMAT_SUBTYPE_PCM))){
        switch(sfdat->fmt.Format.wBitsPerSample){
            case(16):
                sfdat->samptype = PSF_SAMP_16;
                break;
            case(24):
                /* only support packed format for now */
                if((sfdat->fmt.Format.nBlockAlign / sfdat->fmt.Format.nChannels) != 3){
                    sfdat->samptype = PSF_SAMP_UNKNOWN;
                    return 1;
                }
                sfdat->samptype = PSF_SAMP_24;
                break;
            case(32):
                sfdat->samptype = PSF_SAMP_32;
                break;
            default:
                sfdat->samptype = PSF_SAMP_UNKNOWN;
                return 1;
        }
        return 0;
    }
    if(compare_guids(&(sfdat->fmt.SubFormat),&(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)))
        if(sfdat->fmt.Format.wBitsPerSample == 32) {
            sfdat->samptype = PSF_SAMP_IEEE_FLOAT;
            return 0;
        }
    /* add other recognised GUIDs here... */
    if(compare_guids(&(sfdat->fmt.SubFormat),&(SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT)))
        if(sfdat->fmt.Format.wBitsPerSample == 32) {
            sfdat->samptype = PSF_SAMP_IEEE_FLOAT;
            sfdat->chformat = MC_BFMT;
            return 0;
        }
    if(compare_guids(&(sfdat->fmt.SubFormat),&(SUBTYPE_AMBISONIC_B_FORMAT_PCM))) {
        switch(sfdat->fmt.Format.wBitsPerSample){
            case(16):
                sfdat->samptype = PSF_SAMP_16;
                break;
            case(24):
                /* only support packed format for now */
                if((sfdat->fmt.Format.nBlockAlign / sfdat->fmt.Format.nChannels) != 3){
                    sfdat->samptype = PSF_SAMP_UNKNOWN;
                    return 1;
                }
                sfdat->samptype = PSF_SAMP_24;
                break;
            case(32):
                sfdat->samptype = PSF_SAMP_32;
                break;
            default:
                sfdat->samptype = PSF_SAMP_UNKNOWN;
                return 1;
        }
        sfdat->chformat = MC_BFMT;
        return 0;
    }
    return 1;
}
/* return actual validbits */
static int psf_bitsize(psf_stype type)
{
    int size = 0;
    switch(type){
        case(PSF_SAMP_16):
            size = 16;
            break;
        case (PSF_SAMP_24):
            size = 24;
            break;
        case(PSF_SAMP_32):
        case(PSF_SAMP_IEEE_FLOAT):
            size = 32;
            break;
        default:
            break;
    }
    return size;
}
/* return sample size in bytes */
static int psf_wordsize(psf_stype type)
{
    int size = 0;
    switch(type){
        case(PSF_SAMP_16):
            size = 2;
            break;
        case (PSF_SAMP_24):
            size = 3;
            break;
        case(PSF_SAMP_32):
        case(PSF_SAMP_IEEE_FLOAT):
            size = 4;
            break;
        default:
            break;
    }
    return size;
    
}


#if defined _WIN32 && defined _MSC_VER
/* fast convergent rounding */
__inline long psf_round(double fval)
{
    int result;
    _asm{
        fld fval
        fistp   result
        mov eax,result
    }
    return result;
}

#else
/* slow convergent rounding ! */
/* TODO: implement IEEE round-to-even */
long psf_round(double val);

long psf_round(double val)
{
    long k;
    k = (long)(fabs(val)+0.5);
    if(val < 0.0)
        k = -k;
    return k;
}
#endif

#ifndef WIN32
int stricmp(const char *a, const char *b)
{
    while(*a != '\0' && *b != '\0') {
        int ca = islower(*a) ? toupper(*a) : *a;
        int cb = islower(*b) ? toupper(*b) : *b;
        
        if(ca < cb)
            return -1;
        if(ca > cb)
            return 1;
        
        a++;
        b++;
    }
    if(*a == '\0' && *b == '\0')
        return 0;
    if(*a != '\0')
        return 1;
    return -1;
}

int
strnicmp(const char *a, const char *b, const int length)
{
    int len = length;
    
    while(*a != '\0' && *b != '\0') {
        int ca = islower(*a) ? toupper(*a) : *a;
        int cb = islower(*b) ? toupper(*b) : *b;
        
        if(len-- < 1)
            return 0;
        
        if(ca < cb)
            return -1;
        if(ca > cb)
            return 1;
        
        a++;
        b++;
    }
    if(*a == '\0' && *b == '\0')
        return 0;
    if(*a != '\0')
        return 1;
    return -1;
}
#endif

/* create a new soundfile, from input props, or with default format  if props==NULL */
/* current default = sr 44100, ch 1, WAVE, 16bit */
/* could have func to define a new default format...*/

static PSFFILE *psf_newFile(const PSF_PROPS *props)
{
    PSFFILE *sfdat;
    
    if(props){
        if(props->srate <=0)
            return NULL;
        if(props->chans <=0)
            return NULL;
        /* NO support for PSF_SAMP_8 yet...*/
        if(props->samptype < PSF_SAMP_16 || props->samptype > PSF_SAMP_IEEE_FLOAT)
            return NULL;
        if(props->format    <= PSF_FMT_UNKNOWN || props->format > PSF_AIFC)
            return NULL;
        if(props->chformat < STDWAVE || props->chformat > MC_WAVE_EX)
            return NULL;
    }
    
    
    sfdat = (PSFFILE *) malloc(sizeof(PSFFILE));
    if(sfdat==NULL)
        return sfdat;
    
    POS64(sfdat->lastwritepos)      = 0;
    sfdat->file         = NULL;
    sfdat->filename         = NULL;
    sfdat->nFrames          = 0;
    sfdat->curframepos      = 0;
    sfdat->isRead           = 1;                /* OK. who knows?    */
    /* or use platform default format.... */
    sfdat->riff_format      = props ? props->format : PSF_STDWAVE;      /* almost certainly! */
    /*sfdat->isSeekable     = 1;*/
    sfdat->clip_floats      = 1;
    sfdat->rescale          = 0;
    sfdat->rescale_fac      = 1.0f;
    sfdat->is_little_endian = byte_order();
    sfdat->samptype         = props ? props->samptype : PSF_SAMP_16;        /* reasonable...     */
    POS64(sfdat->dataoffset)        = 0;
    POS64(sfdat->fmtoffset)     = 0;
    POS64(sfdat->peakoffset)        = 0;
    sfdat->chformat         = props ? props->chformat : STDWAVE;
    /*setup Format */
    if(props)
        sfdat->fmt.Format.wFormatTag  = (WORD) (props->samptype == PSF_SAMP_IEEE_FLOAT ?  WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM);
    else
        sfdat->fmt.Format.wFormatTag  = WAVE_FORMAT_PCM;
    sfdat->fmt.Format.nChannels       = (WORD) (props ?  props->chans : 1);
    sfdat->fmt.Format.nSamplesPerSec  = props ? props->srate : 44100;
    sfdat->fmt.Format.nBlockAlign     = (WORD) (props  ?  sfdat->fmt.Format.nChannels * psf_wordsize(props->samptype) : sfdat->fmt.Format.nChannels * sizeof(short));
    sfdat->fmt.Format.wBitsPerSample  = (WORD) (props ?  psf_bitsize(props->samptype)  : sizeof(short) * BITS_PER_BYTE);
    sfdat->fmt.Format.nAvgBytesPerSec = sfdat->fmt.Format.nSamplesPerSec
    *sfdat->fmt.Format.nChannels
    * (sfdat->fmt.Format.wBitsPerSample / BITS_PER_BYTE);
    sfdat->pPeaks           = NULL;
    sfdat->peaktime         = 0;
    sfdat->fmt.Format.cbSize = 0;
    /* set initial defaults for WAVE-EX stuff; may change */
    /* but nobody should look at these fields unless we have a real WAVE-EX file anyway... */
    sfdat->fmt.dwChannelMask = SPKRS_UNASSIGNED;
    sfdat->fmt.Samples.wValidBitsPerSample  = sfdat->fmt.Format.wBitsPerSample;
    /* 0 should be a guaranteed non-valid GUID! */
    memset((char *) &(sfdat->fmt.SubFormat),0,sizeof(GUID));
    
    if(props && (props->format == PSF_WAVE_EX)) {
        sfdat->fmt.Format.cbSize = 22;
        /* NB we will set the GUID from wFormatTag in waveExWriteHeader() */
        /* should really flag an error if user sets this */
        if(sfdat->chformat==STDWAVE)
            sfdat->chformat = MC_STD;
        
        /* set wavex speaker mask */
        /* TODO: support custom speaker masks, wordsizes, etc */
        switch(sfdat->chformat){
            case MC_MONO:
                if(props->chans != 1){
                    //rsferrstr = "conflicting channel configuration for WAVE-EX file";
                    free(sfdat);
                    return NULL;
                }
                sfdat->fmt.dwChannelMask = SPKRS_MONO;
                break;
            case MC_STEREO:
                if(props->chans != 2){
                    //rsferrstr = "conflicting channel configuration for WAVE-EX file";
                    free(sfdat);
                    return NULL;
                }
                sfdat->fmt.dwChannelMask = SPKRS_STEREO;
                break;
            case MC_QUAD:
                if(props->chans != 4){
                    //rsferrstr = "conflicting channel configuration for WAVE-EX file";
                    free(sfdat);
                    return NULL;
                }
                sfdat->fmt.dwChannelMask = SPKRS_GENERIC_QUAD;
                break;
            case MC_LCRS:
                if(props->chans != 4){
                    free(sfdat);
                    return NULL;
                }
                sfdat->fmt.dwChannelMask = SPKRS_SURROUND_LCRS;
                break;
                
            case MC_DOLBY_5_1:
                if(props->chans != 6){
                    //rsferrstr = "conflicting channel configuration for WAVE-EX file";
                    free(sfdat);
                    return NULL;
                }
                sfdat->fmt.dwChannelMask = SPKRS_DOLBY5_1;
                break;
            case MC_SURR_5_0:
                if(props->chans != 5){
                    //rsferrstr = "conflicting channel configuration for WAVE-EX file";
                    free(sfdat);
                    return NULL;
                }
                sfdat->fmt.dwChannelMask = SPKRS_SURR_5_0;
                break;
            case MC_SURR_6_1:
                if(props->chans != 7){
                    //rsferrstr = "conflicting channel configuration for WAVE-EX file";
                    free(sfdat);
                    return NULL;
                }
                sfdat->fmt.dwChannelMask = SPKRS_6_1;
                break;
            case MC_SURR_7_1:
                if(props->chans != 8){
                    //rsferrstr = "conflicting channel configuration for WAVE-EX file";
                    free(sfdat);
                    return NULL;
                }
                sfdat->fmt.dwChannelMask = SPKRS_7_1;
                break;
            case MC_CUBE:
                if(props->chans != 8){
                    //rsferrstr = "conflicting channel configuration for WAVE-EX file";
                    free(sfdat);
                    return NULL;
                }
                sfdat->fmt.dwChannelMask = SPKRS_CUBE;
                break;
            default:
                /*MC_STD, MC_BFMT */
                sfdat->fmt.dwChannelMask = SPKRS_UNASSIGNED;
                break;
        }
    }
    /* no dither, by default */
    sfdat->dithertype = PSF_DITHER_OFF;
    sfdat->illformed = 0;                   /*RWD Feb 2010 */
    memset(sfdat->warnstring,0,WARNSTRING_SIZE);
    return sfdat;
}

int psf_getWarning(int sfd,const char** warnstring)
{
    int retval = 0;
    PSFFILE *sfdat;
    if(sfd < 0 || sfd > psf_maxfiles)
        retval = PSF_E_BADARG;
    sfdat  = psf_files[sfd];
    
    if(sfdat->illformed==0)
        retval = 0;
    else{
        *warnstring = sfdat->warnstring;
        retval =  1;
    }
    return retval;
}
/* complete header before closing file; return PSF_E_NOERROR[= 0] on success */
static int wavUpdate(PSFFILE *sfdat)
{
    DWORD riffsize,datasize;
    fpos_t bytepos;
#ifdef _DEBUG
    assert(sfdat);
    assert(sfdat->file);
    assert(POS64(sfdat->dataoffset) != 0);
#endif      
    POS64(bytepos) = sizeof(int);
    if((fsetpos(sfdat->file,&bytepos))==0) {
        riffsize = (sfdat->nFrames * sfdat->fmt.Format.nBlockAlign) +  (MYLONG) POS64(sfdat->dataoffset);
        riffsize -= 2 * sizeof(DWORD);
        if(!sfdat->is_little_endian)
            riffsize = REVDWBYTES(riffsize);
        if(fwrite((char *) &riffsize,sizeof(int),1,sfdat->file) != 1)
            return PSF_E_CANT_WRITE;
    }
    else
        return PSF_E_CANT_SEEK;
    if(sfdat->pPeaks){
        if(POS64(sfdat->peakoffset)==0)
            return PSF_E_BADARG;
        
        /*do byterev if necessary...*/
        if((fsetpos(sfdat->file,&sfdat->peakoffset))==0){
            /*set current time*/
            DWORD *pblock;
            int i;
            time_t now = time(0);
            if(!sfdat->is_little_endian){
                now = REVDWBYTES(now);
                pblock = (DWORD *) (sfdat->pPeaks);
                for(i=0;i < sfdat->fmt.Format.nChannels * 2; i++)
                    pblock[i] = REVDWBYTES(pblock[i]);
            }
            if((fwrite((char*)&now,sizeof(DWORD),1,sfdat->file)) != 1)
                return PSF_E_CANT_WRITE;
            
            if((fwrite((char *) (sfdat->pPeaks),sizeof(PSF_CHPEAK),sfdat->fmt.Format.nChannels,sfdat->file))
               != sfdat->fmt.Format.nChannels )
                return PSF_E_CANT_WRITE;
        }
        else
            return PSF_E_CANT_SEEK;
    }
    POS64(bytepos) = POS64(sfdat->dataoffset) -  sizeof(int);
    if((fsetpos(sfdat->file,&bytepos))==0) {
        datasize = sfdat->nFrames * sfdat->fmt.Format.nBlockAlign;
        if(!sfdat->is_little_endian)
            datasize = REVDWBYTES(datasize);
        if(fwrite((char *) & datasize,sizeof(DWORD),1,sfdat->file) != 1)
            return PSF_E_CANT_WRITE;
    }
    if(fseek(sfdat->file,0,SEEK_END)){
        /*DBGFPRINTF((stderr,"wavUpdate: error reseeking to end of file\n"));*/
        return PSF_E_CANT_SEEK;
    }
    
    return PSF_E_NOERROR;
}

/* ditto for AIFF... */

/* NB: the AIFF spec is unclear on type of size field. We decide on unsigned long (DWORD) here;
 on the principle that a COMM chunk with an unsigned long nSampleFrames really needs the
 chunk size to be unsigned long too!.
 
 */
static int aiffUpdate(PSFFILE *sfdat)
{
    DWORD aiffsize,datasize,rev_datasize,frames;
    fpos_t bytepos,filesize;
    unsigned char pad = 0x00;
    
    if(sfdat==NULL || sfdat->file== NULL)
        return PSF_E_BADARG;
    
    if(POS64(sfdat->dataoffset)  == 0)
        return PSF_E_BADARG;
    POS64(bytepos) = sizeof(int);
    if((fsetpos(sfdat->file,&bytepos))==0) {
        /* RWD 26:10:2002 */
        aiffsize = (sfdat->nFrames * sfdat->fmt.Format.nBlockAlign)
        + (MYLONG) POS64(sfdat->dataoffset);
        // need to count any needed pad byte
        aiffsize += (aiffsize % 2);
        // deduct 8 bytes for FORM<size>
        aiffsize -= 2 * sizeof(DWORD);
        if(sfdat->is_little_endian)
            aiffsize = REVDWBYTES(aiffsize);
        if(fwrite((char *) &aiffsize,sizeof(DWORD),1,sfdat->file) != 1)
            return PSF_E_CANT_WRITE;
    }
    else
        return PSF_E_CANT_SEEK;
    POS64(bytepos)  = POS64(sfdat->fmtoffset) + sizeof(WORD);
    if((fsetpos(sfdat->file,&bytepos))==0) {
        frames = sfdat->nFrames;
        if(sfdat->is_little_endian)
            frames = REVDWBYTES(frames);
        if(fwrite((char *) &frames,sizeof(DWORD),1,sfdat->file) != 1)
            return PSF_E_CANT_WRITE;
    }
    else
        return PSF_E_CANT_SEEK;
    if(sfdat->pPeaks){
        if(POS64(sfdat->peakoffset)==0)
            return PSF_E_BADARG;
        
        /*do byterev if necessary...*/
        if((fsetpos(sfdat->file,&sfdat->peakoffset))==0){
            /*set current time*/
            DWORD *pblock;
            int i;
            time_t now = time(0);
            if(sfdat->is_little_endian){
                now = REVDWBYTES(now);
                pblock = (DWORD *) (sfdat->pPeaks);
                for(i=0;i < sfdat->fmt.Format.nChannels * 2; i++)
                    pblock[i] = REVDWBYTES(pblock[i]);
            }
            if((fwrite((char*)&now,sizeof(DWORD),1,sfdat->file)) != 1)
                return PSF_E_CANT_WRITE;
            
            if((fwrite((char *) (sfdat->pPeaks),sizeof(PSF_CHPEAK),sfdat->fmt.Format.nChannels,sfdat->file))
               != sfdat->fmt.Format.nChannels )
                return PSF_E_CANT_WRITE;
        }
        else
            return PSF_E_CANT_SEEK;
    }
    POS64(bytepos) = POS64(sfdat->dataoffset) - (3 * sizeof(int));
    if((fsetpos(sfdat->file,&bytepos))==0) {
        datasize = sfdat->nFrames * sfdat->fmt.Format.nBlockAlign;
        datasize += 2* sizeof(DWORD);   /* add offset and blocksize fields */
        rev_datasize = datasize; /* preserve this for the seek later on */
        if(sfdat->is_little_endian)
            rev_datasize = REVDWBYTES(datasize);
        if(fwrite((char *) & rev_datasize,sizeof(DWORD),1,sfdat->file) != 1)
            return PSF_E_CANT_WRITE;
    }
    else
        return PSF_E_CANT_SEEK;
    /* datachunk needs added pad byte if odd, not included in saved chunksize*/
    POS64(bytepos) = POS64(sfdat->dataoffset) + datasize;
    if((fsetpos(sfdat->file,&bytepos))){
        return PSF_E_CANT_SEEK;
    }
    if(fgetpos(sfdat->file,&filesize))
        return PSF_E_CANT_SEEK;
#ifdef _DEBUG   
    assert(POS64(filesize) == POS64(bytepos));
#endif  
    if(POS64(filesize) % 2)
        if(fwrite(&pad,sizeof(unsigned char),1,sfdat->file) != 1)
            return PSF_E_CANT_WRITE;
    
    return PSF_E_NOERROR;
}


/* internal write func: return 0 for success */
static int wavDoWrite(PSFFILE *sfdat, const void* buf, DWORD nBytes)
{
    
    DWORD written = 0;
    if(sfdat==NULL || buf==NULL)
        return PSF_E_BADARG;
    
    if(sfdat->file==NULL)
        return PSF_E_CANT_WRITE;
    
    if((written = fwrite(buf,sizeof(char),nBytes,sfdat->file)) != nBytes) {
        DBGFPRINTF((stderr, "wavDoWrite: wanted %d got %d.\n",
                    (int) nBytes,(int) written));
        return PSF_E_CANT_WRITE;
    }
    sfdat->lastop  = PSF_OP_WRITE;
    return PSF_E_NOERROR;
}

static int wavDoRead(PSFFILE *sfdat, void* buf, DWORD nBytes)
{
    
    DWORD got = 0;
    if(sfdat==NULL || buf==NULL)
        return PSF_E_BADARG;
    
    if(sfdat->file==NULL)
        return PSF_E_CANT_READ;
    
    if((got = fread(buf,sizeof(char),nBytes,sfdat->file)) != nBytes) {
        DBGFPRINTF((stderr, "wavDoRead: wanted %d got %d.\n",
                    (int) nBytes,(int) got));
        return PSF_E_CANT_READ;
    }
    sfdat->lastop = PSF_OP_READ;
    return PSF_E_NOERROR;
    
}

/* write PEAK chunk if we have the data */
static int wavWriteHeader(PSFFILE *sfdat)
{
    DWORD tag,size;
    WORD cbSize = 0;
    WAVEFORMATEX *pfmt;
    PSF_CHPEAK *peaks;
    fpos_t bytepos;
#ifdef _DEBUG
    assert(sfdat);
    assert(sfdat->file);
    assert(sfdat->riff_format == PSF_STDWAVE);
    assert(sfdat->nFrames == 0);
    assert(!sfdat->isRead);
    assert(sfdat->fmt.Format.nChannels != 0);
#endif
    
    /*clear pPeaks array*/
    if(sfdat->pPeaks)
        memset((char *)sfdat->pPeaks,0,sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels);
    
    
    tag = TAG('R','I','F','F');
    size = 0;
    if(!sfdat->is_little_endian)
        size = REVDWBYTES(size);
    else
        tag = REVDWBYTES(tag);
    
    
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    
    tag = TAG('W','A','V','E');
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    
    pfmt = &(sfdat->fmt.Format);
    
    tag = TAG('f','m','t',' ');
    size = sizeof(WAVEFORMAT);
    if(sfdat->samptype==PSF_SAMP_IEEE_FLOAT)
        size += sizeof(WORD);         /* for cbSize: WAVEOFRMATEX */
    if(!sfdat->is_little_endian){
        size = REVDWBYTES(size);
        fmtSwapBytes(sfdat);
    }
    else
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    if(fgetpos(sfdat->file,&bytepos))
        return PSF_E_CANT_SEEK;
    sfdat->fmtoffset = bytepos;
    
    if(wavDoWrite(sfdat,(char *)pfmt,sizeof(WAVEFORMAT)))
        return PSF_E_CANT_WRITE;
    /*add cbSize if floatsams */
    if(sfdat->samptype==PSF_SAMP_IEEE_FLOAT)
        if(wavDoWrite(sfdat,(char *)&cbSize,sizeof(WORD)))
            return PSF_E_CANT_WRITE;
    /* reswap it all */
    if(!sfdat->is_little_endian){
        fmtSwapBytes(sfdat);
    }
    
    if(sfdat->pPeaks){
        DWORD version = 1, now = 0;
        peaks = sfdat->pPeaks;
        
        tag = TAG('P','E','A','K');
        size = 2 * sizeof(DWORD) + sizeof(PSF_CHPEAK) * pfmt->nChannels;
        if(!sfdat->is_little_endian){
            size = REVDWBYTES(size);
            version  = REVDWBYTES(version);
        }
        else
            tag = REVDWBYTES(tag);
        
        if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *)&version,sizeof(DWORD)))
            return PSF_E_CANT_WRITE;
        if(fgetpos(sfdat->file,&bytepos))
            return PSF_E_CANT_SEEK;
        sfdat->peakoffset = bytepos;  /*we need to update time*/
        
        if(wavDoWrite(sfdat,(char *) &now,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *) peaks, sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels))
            return PSF_E_CANT_WRITE;
    }
    tag = TAG('d','a','t','a');
    size = 0;
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    if(fgetpos(sfdat->file,&bytepos))
        return PSF_E_CANT_SEEK;
    sfdat->dataoffset = bytepos;
    return PSF_E_NOERROR;
}


static int waveExWriteHeader(PSFFILE *sfdat)
{
    DWORD tag,size;
    WAVEFORMATEXTENSIBLE *pfmt;
    PSF_CHPEAK *peaks;
    GUID *pGuid = NULL;
    fpos_t bytepos;
#ifdef _DEBUG
    assert(sfdat);
    assert(sfdat->file);
    assert(sfdat->chformat > STDWAVE);
    assert(sfdat->nFrames==0);
    assert(!sfdat->isRead);
    assert(sfdat->fmt.Format.nChannels != 0);
#endif
    /*clear pPeaks array*/
    if(sfdat->pPeaks)
        memset((char *)sfdat->pPeaks,0,sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels);
    /* complete WAVE-EX format fields: */
    if(sfdat->chformat==MC_BFMT){
        if(sfdat->samptype== PSF_SAMP_IEEE_FLOAT){
            pGuid = (GUID *)  &SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT;
        }
        else{
            pGuid =(GUID *) &SUBTYPE_AMBISONIC_B_FORMAT_PCM;
        }
        
    }else {
        if(sfdat->fmt.Format.wFormatTag==  WAVE_FORMAT_IEEE_FLOAT){
            pGuid = (GUID *)  &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        }
        else{
            pGuid =(GUID *) &KSDATAFORMAT_SUBTYPE_PCM;
        }
    }
    sfdat->fmt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    memcpy((char *) &(sfdat->fmt.SubFormat),(char *)pGuid,sizeof(GUID));
    tag = TAG('R','I','F','F');
    size = 0;
    if(!sfdat->is_little_endian)
        size = REVDWBYTES(size);
    else
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    tag = TAG('W','A','V','E');
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    pfmt = &(sfdat->fmt);
    tag = TAG('f','m','t',' ');
    size = sizeof_WFMTEX;
    if(!sfdat->is_little_endian){
        size = REVDWBYTES(size);
        fmtExSwapBytes(sfdat);
    }
    else
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    if(fgetpos(sfdat->file,&bytepos))
        return PSF_E_CANT_SEEK;
    sfdat->fmtoffset = bytepos;
    /* write fmt elementwise, to avoid C alignment traps with WORD */
    /* 16byte format...*/
    if(wavDoWrite(sfdat,(char *)pfmt,sizeof(WAVEFORMAT))
       /** cbSize... */
       ||wavDoWrite(sfdat,(char *) &(pfmt->Format.cbSize),sizeof(WORD))
       /* validbits... */
       ||wavDoWrite(sfdat,(char *) &(pfmt->Samples.wValidBitsPerSample),sizeof(WORD))
       /* ChannelMask .... */
       ||wavDoWrite(sfdat,(char *) &(pfmt->dwChannelMask),sizeof(DWORD))
       /*  and the GUID */
       ||wavDoWrite(sfdat,(char *) &(pfmt->SubFormat),sizeof(GUID)))
        return PSF_E_CANT_WRITE;
    /* reswap it all */
    if(!sfdat->is_little_endian){
        fmtExSwapBytes(sfdat);
    }
    if(sfdat->pPeaks){
        DWORD version = 1, now = 0;
        
        peaks = sfdat->pPeaks;
        tag = TAG('P','E','A','K');
        size = 2 * sizeof(DWORD) + sizeof(PSF_CHPEAK) * pfmt->Format.nChannels;
        if(!sfdat->is_little_endian){
            size = REVDWBYTES(size);
            version  = REVDWBYTES(version);
        }
        else
            tag = REVDWBYTES(tag);
        if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *)&version,sizeof(DWORD)))
            return PSF_E_CANT_WRITE;
        if(fgetpos(sfdat->file,&bytepos))
            return PSF_E_CANT_SEEK;
        sfdat->peakoffset = bytepos;  /*we need to update time*/
        if(wavDoWrite(sfdat,(char *) &now,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *) peaks, sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels))
            return PSF_E_CANT_WRITE;
    }
    
    tag = TAG('d','a','t','a');
    size = 0;
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    if(fgetpos(sfdat->file,&bytepos))
        return PSF_E_CANT_SEEK;
    sfdat->dataoffset = bytepos;
    return PSF_E_NOERROR;
}

static int aiffWriteHeader(PSFFILE *sfdat)
{
    DWORD tag,size;
    PSF_CHPEAK *peaks;
    DWORD dwData,offset,blocksize;
    unsigned char ieee[10];
    WORD wData;
    fpos_t bytepos;
#ifdef _DEBUG
    assert(sfdat);
    assert(sfdat->file);
    assert(sfdat->chformat == STDWAVE);
    assert(sfdat->nFrames==0);
    assert(!sfdat->isRead);
    assert(sfdat->riff_format == PSF_AIFF);
    assert(sfdat->fmt.Format.nChannels != 0);
#endif
    
    /*clear pPeaks array*/
    if(sfdat->pPeaks)
        memset((char *)sfdat->pPeaks,0,sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels);
    
    tag = TAG('F','O','R','M');
    size = 0;
    
    if(sfdat->is_little_endian) {
        size = REVDWBYTES(size);
        tag = REVDWBYTES(tag);
    }
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    tag = TAG('A','I','F','F');
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    
    tag = TAG('C','O','M','M');
    size = 18;
    if(sfdat->is_little_endian){
        size = REVDWBYTES(size);
        tag = REVDWBYTES(tag);
    }
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    if(fgetpos(sfdat->file,&bytepos))
        return PSF_E_CANT_SEEK;
    sfdat->fmtoffset = bytepos;
    
    wData = sfdat->fmt.Format.nChannels;
    if(sfdat->is_little_endian)
        wData = (WORD) REVWBYTES(wData);
    if(wavDoWrite(sfdat,(char *)&wData,sizeof(WORD)))
        return PSF_E_CANT_WRITE;
    
    dwData = 0;         /* nFrames */
    if(wavDoWrite(sfdat,(char *)&dwData,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    
    wData = sfdat->fmt.Format.wBitsPerSample;
    if(sfdat->is_little_endian)
        wData = (WORD) REVWBYTES(wData);
    if(wavDoWrite(sfdat,(char *)&wData,sizeof(WORD)))
        return PSF_E_CANT_WRITE;
    double_to_ieee_80((double)sfdat->fmt.Format.nSamplesPerSec,ieee);
    
    if(wavDoWrite(sfdat,ieee,10))
        return PSF_E_CANT_WRITE;
    if(sfdat->pPeaks){
        DWORD version = 1, now = 0;
        peaks = sfdat->pPeaks;
        
        tag = TAG('P','E','A','K');
        size = 2 * sizeof(DWORD) + sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels;
        if(sfdat->is_little_endian){
            size = REVDWBYTES(size);
            version  = REVDWBYTES(version);
            tag = REVDWBYTES(tag);
        }
        if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *)&version,sizeof(DWORD)))
            return PSF_E_CANT_WRITE;
        if(fgetpos(sfdat->file,&bytepos))
            return PSF_E_CANT_SEEK;
        sfdat->peakoffset = bytepos;  /*we need to update time*/
        
        if(wavDoWrite(sfdat,(char *) &now,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *) peaks, sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels))
            return PSF_E_CANT_WRITE;
    }
    
    tag = TAG('S','S','N','D');
    size = offset = blocksize = 0;
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&offset,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&blocksize,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    if(fgetpos(sfdat->file,&bytepos))
        return PSF_E_CANT_SEEK;
    sfdat->dataoffset = bytepos;
    return PSF_E_NOERROR;
}


static int aifcWriteHeader(PSFFILE *sfdat)
{
    DWORD tag,size;
    PSF_CHPEAK *peaks;
    DWORD dwData,offset,blocksize,aifcver = AIFC_VERSION_1,ID_compression;
    /*assume 32bit floats, but we may be asked to use aifc for integer formats too*/
    char *str_compressed = (char *) aifc_floatstring;
    int pstring_size = 10;
    unsigned char ieee[10];
    WORD wData;
    fpos_t bytepos;
    
#ifdef _DEBUG
    assert(sfdat);
    assert(sfdat->file);
    assert(sfdat->nFrames==0);
    assert(!sfdat->isRead);
    assert(sfdat->riff_format == PSF_AIFC);
    assert(sfdat->fmt.Format.nChannels != 0);
#endif
    
    if(sfdat->samptype==PSF_SAMP_IEEE_FLOAT)
        ID_compression = TAG('f','l','3','2');
    else {
        ID_compression = TAG('N','O','N','E');
        pstring_size = 16;
        str_compressed = (char *) aifc_notcompressed;
    }
    /*clear pPeaks array*/
    if(sfdat->pPeaks)
        memset((char *)sfdat->pPeaks,0,sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels);
    
    tag = TAG('F','O','R','M');
    size = 0;
    
    if(sfdat->is_little_endian) {
        size = REVDWBYTES(size);
        tag = REVDWBYTES(tag);
    }
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    tag = TAG('A','I','F','C');
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    
    tag = TAG('F','V','E','R');
    size = sizeof(DWORD);
    if(sfdat->is_little_endian){
        size = REVDWBYTES(size);
        tag = REVDWBYTES(tag);
        aifcver = REVDWBYTES(aifcver);
    }
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&aifcver,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    
    tag = TAG('C','O','M','M');
    size = 22 + pstring_size;
    if(sfdat->is_little_endian){
        size = REVDWBYTES(size);
        tag = REVDWBYTES(tag);
    }
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    if(fgetpos(sfdat->file,&bytepos))
        return PSF_E_CANT_SEEK;
    sfdat->fmtoffset = bytepos;
    
    wData = sfdat->fmt.Format.nChannels;
    if(sfdat->is_little_endian)
        wData = (WORD) REVWBYTES(wData);
    if(wavDoWrite(sfdat,(char *)&wData,sizeof(WORD)))
        return PSF_E_CANT_WRITE;
    
    dwData = 0;         /* nFrames */
    if(wavDoWrite(sfdat,(char *)&dwData,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    
    wData = sfdat->fmt.Format.wBitsPerSample;
    if(sfdat->is_little_endian)
        wData = (WORD) REVWBYTES(wData);
    if(wavDoWrite(sfdat,(char *)&wData,sizeof(WORD)))
        return PSF_E_CANT_WRITE;
    double_to_ieee_80((double)sfdat->fmt.Format.nSamplesPerSec,ieee);
    
    if(wavDoWrite(sfdat,ieee,10))
        return PSF_E_CANT_WRITE;
    /*AIFC bits */
    if(sfdat->is_little_endian)
        ID_compression = REVDWBYTES(ID_compression);
    if(wavDoWrite(sfdat,(char *)&ID_compression,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    if(wavDoWrite(sfdat,str_compressed,pstring_size))
        return PSF_E_CANT_WRITE;
    
    if(sfdat->pPeaks){
        DWORD version = 1, now = 0;
        
        peaks = sfdat->pPeaks;
        tag = TAG('P','E','A','K');
        size = 2 * sizeof(DWORD) + sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels;
        if(sfdat->is_little_endian){
            size = REVDWBYTES(size);
            version  = REVDWBYTES(version);
            tag = REVDWBYTES(tag);
        }
        if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *)&version,sizeof(DWORD)))
            return PSF_E_CANT_WRITE;
        if(fgetpos(sfdat->file,&bytepos))
            return PSF_E_CANT_SEEK;
        sfdat->peakoffset = bytepos;  /*we need to update time*/
        
        if(wavDoWrite(sfdat,(char *) &now,sizeof(DWORD))
           || wavDoWrite(sfdat,(char *) peaks, sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels))
            return PSF_E_CANT_WRITE;
    }
    
    tag = TAG('S','S','N','D');
    size = offset = blocksize = 0;
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(wavDoWrite(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&size,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&offset,sizeof(DWORD))
       || wavDoWrite(sfdat,(char *)&blocksize,sizeof(DWORD)))
        return PSF_E_CANT_WRITE;
    if(fgetpos(sfdat->file,&bytepos))
        return PSF_E_CANT_SEEK;
    sfdat->dataoffset = bytepos;
    return PSF_E_NOERROR;
}

/* create soundfile. return descriptor, or some PSF_error value < 0 */
/* supported clipping or non-clipping of floats to 0dbFS, 
 minimum header (or PEAK), and RDWR or RDONLY (but last not implemented yet!) */
/* we expect full format info to be set in props */
/* I want to offer share-read access (easy with WIN32), but can't with  ANSI! */
/* possible TODO:  enforce non-destructive by e.g. rejecting create on existing file */
int psf_sndCreate(const char *path,const PSF_PROPS *props,int clip_floats,int minheader, int mode)
{       
    int i,rc = PSF_E_UNSUPPORTED;
    psf_format fmt;
    PSFFILE *sfdat;
    char *fmtstr = "wb+";   /* default is READ+WRITE */
    /*  disallow props = NULL here, until/unless I can offer mechanism to set default props via psf_init() */
    if(path == NULL || props == NULL)
        return PSF_E_BADARG;
    
    for(i=0; i < psf_maxfiles; i++) {
        if(psf_files[i] == NULL)
            break;
    }
    if(i==psf_maxfiles)
        return PSF_E_TOOMANYFILES;
    
    sfdat = psf_newFile(props);
    if(sfdat == NULL)
        return PSF_E_NOMEM;
    
    sfdat->clip_floats = clip_floats;
    fmt = psf_getFormatExt(path);
    if(fmt==PSF_FMT_UNKNOWN)
        return PSF_E_UNSUPPORTED;
    if(sfdat->samptype == PSF_SAMP_UNKNOWN)
        return PSF_E_BADARG;
    
    sfdat->filename = (char *) malloc(strlen(path)+1);
    if(sfdat->filename==NULL) {
        DBGFPRINTF((stderr, "wavOpenWrite: no memory for filename\n"));
        return PSF_E_NOMEM;
    }
    if(!minheader){
        sfdat->pPeaks = (PSF_CHPEAK *) malloc(sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels);
        if(sfdat->pPeaks==NULL){
            DBGFPRINTF((stderr, "wavOpenWrite: no memory for peak data\n"));
            return PSF_E_NOMEM;
        }
    }
    /*switch (mode).... */
    if(mode==PSF_CREATE_WRONLY)
        fmtstr = "wb";
    /* deal with CREATE_TEMPORARY later on! */
    if((sfdat->file = fopen(path,fmtstr))  == NULL) {
        DBGFPRINTF((stderr, "wavOpenWrite: cannot create '%s'\n", path));
        return PSF_E_CANT_OPEN;
    }
    
    strcpy(sfdat->filename, path);
    sfdat->isRead = 0;
    sfdat->nFrames = 0;
    /* force aif f/p data to go to aifc format */
    if(sfdat->samptype==PSF_SAMP_IEEE_FLOAT && fmt==PSF_AIFF){
        DBGFPRINTF((stderr, "Warning: writing floating point data in AIFC format\n"));
        fmt= PSF_AIFC;
    }
    /* .wav extension can be either std WAVE or WAVE-EX */
    if(fmt==PSF_STDWAVE){
        if(props->format==PSF_WAVE_EX)
            fmt = PSF_WAVE_EX;
    }
    sfdat->riff_format = fmt;
    
    switch(fmt){
        case(PSF_STDWAVE):
            rc = wavWriteHeader(sfdat);
            break;
        case(PSF_AIFF):
            
            rc = aiffWriteHeader(sfdat);
            break;
        case(PSF_AIFC):
            
            rc = aifcWriteHeader(sfdat);
            break;
        case (PSF_WAVE_EX):
            rc = waveExWriteHeader(sfdat);
            break;
        default:
            sfdat->riff_format = PSF_FMT_UNKNOWN;
            /* RAW? */
            break;
    }
    if(rc < PSF_E_NOERROR)
        return rc;
    psf_files[i] = sfdat;
    return i;
}

/* snd close:  automatically completes PEAK data when writing */
/* return 0 for success */
int psf_sndClose(int sfd)
{
    int rc = PSF_E_NOERROR;
    PSFFILE *sfdat;
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    sfdat  = psf_files[sfd];
#ifdef _DEBUG       
    assert(sfdat->file);
    assert(sfdat->filename);
#endif
    if(sfdat==NULL || sfdat->file==NULL)
        return PSF_E_BADARG;
    if(!sfdat->isRead){
        switch(sfdat->riff_format){
            case(PSF_STDWAVE):
            case(PSF_WAVE_EX):
                rc = wavUpdate(sfdat);
                break;
            case(PSF_AIFF):
            case(PSF_AIFC):
                rc = aiffUpdate(sfdat);
                break;
            default:
                rc = PSF_E_CANT_CLOSE;
                break;
        }
    }
    if(psf_release_file(sfdat))
        rc = PSF_E_CANT_CLOSE;
    else {
        free(sfdat);
        psf_files[sfd]= NULL;
    }
    return rc;
}

/* write floats (multi-channel) framebuf to whichever target format. tracks PEAK data.*/ 
/* bend over backwards not to modify source data */
/* returns nFrames, or errval < 0 */
int psf_sndWriteFloatFrames(int sfd, const float *buf, DWORD nFrames)
{
    int chans,lsamp;
    DWORD i;
    int j,do_reverse;
    const float *pbuf = buf;
    float fsamp,absfsamp;
    int do_shift = 1;
    PSFFILE *sfdat;
    SND_SAMP s_samp;
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    
    sfdat  = psf_files[sfd];
    
#ifdef _DEBUG       
    assert(sfdat->file);
    assert(sfdat->filename);
#endif
    
    if(buf==NULL)
        return PSF_E_BADARG;
    if(nFrames == 0)
        return nFrames;
    if(sfdat->isRead)
        return PSF_E_FILE_READONLY;
    chans = sfdat->fmt.Format.nChannels;
    
    switch(sfdat->riff_format){
        case(PSF_STDWAVE):
        case(PSF_WAVE_EX):
            do_reverse = (sfdat->is_little_endian ? 0 : 1 );
            do_shift = 1;
            break;
        case(PSF_AIFF):
        case(PSF_AIFC):
            do_reverse = (sfdat->is_little_endian ? 1 : 0 );
            do_shift = 0;
            break;
        default:
            return PSF_E_UNSUPPORTED;
    }
    if(sfdat->lastop  == PSF_OP_READ)
        fflush(sfdat->file);
    switch(sfdat->samptype){
        case(PSF_SAMP_IEEE_FLOAT):
            if(do_reverse){
                for(i=0; i < nFrames; i++){
                    for(j=0;j < chans; j++) {
                        fsamp = *pbuf++;
                        if(sfdat->clip_floats){
                            fsamp = min(fsamp,1.0f);
                            fsamp = max(fsamp,-1.0f);
                        }
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        // lsamp = * (int *) pbuf++;
                        s_samp.fsamp = fsamp;
                        lsamp = s_samp.lsamp;
                        lsamp = REVDWBYTES(lsamp);
                        if(wavDoWrite(sfdat,(char *) &lsamp,sizeof(int))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            else {
                for(i=0; i < nFrames; i++, pbuf += chans){
                    for(j=0;j < chans; j++) {
                        fsamp = pbuf[j];
                        if(sfdat->clip_floats){
                            fsamp = min(fsamp,1.0f);
                            fsamp = max(fsamp,-1.0f);
                        }
                        absfsamp = (float)fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                    }
                }
                if(wavDoWrite(sfdat,(char *)buf,nFrames * chans * sizeof(float))){
                    DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                    return PSF_E_CANT_WRITE;
                }
            }
            break;
        case(PSF_SAMP_16):
            /* TODO: optimise all this with func pointers etc */
            if(do_reverse){
                short ssamp;
                for(i=0; i < nFrames; i++){
                    for(j=0;j < chans; j++) {
                        fsamp = *buf++;
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip16);
                        fsamp = max(fsamp,-dclip16);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        if(sfdat->dithertype == PSF_DITHER_TPDF)
                            ssamp = (short) psf_round(fsamp * 32766.0 + 2.0 * trirand());
                        else
                            ssamp = (short) psf_round(fsamp * MAX_16BIT);
                        ssamp = (short) REVWBYTES(ssamp);
                        if( wavDoWrite(sfdat,(char *) &ssamp,sizeof(short))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            else {
                short ssamp;
                for(i=0; i < nFrames; i++, buf += chans){
                    for(j=0;j < chans; j++) {
                        fsamp = buf[j];
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip16);
                        fsamp = max(fsamp,-dclip16);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        if(sfdat->dithertype == PSF_DITHER_TPDF)
                            ssamp = (short) psf_round(fsamp * 32766.0 + 2.0 * trirand());
                        else
                            ssamp = (short) psf_round(fsamp * MAX_16BIT);
                        
                        if(wavDoWrite(sfdat,(char *) &ssamp,sizeof(short))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            break;
        case(PSF_SAMP_24):
            if(do_reverse){
                for(i=0; i < nFrames; i++){
                    for(j=0;j < chans; j++) {
                        fsamp = *buf++;
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip24);
                        fsamp = max(fsamp,-dclip24);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        lsamp = psf_round(fsamp * (MAX_32BIT));
                        lsamp = REVDWBYTES(lsamp);
                        if(do_shift){
                            if(sfdat->is_little_endian)
                                lsamp >>= 8;
                            else
                                lsamp <<= 8;
                        }
                        if( wavDoWrite(sfdat,(char *) &lsamp,3)){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            else {
                for(i=0; i < nFrames; i++, buf += chans){
                    for(j=0;j < chans; j++) {
                        fsamp = buf[j];
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip24);
                        fsamp = max(fsamp,-dclip24);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        lsamp = psf_round(fsamp * (MAX_32BIT));
                        if(do_shift){
                            if(sfdat->is_little_endian)
                                lsamp >>= 8;
                            else
                                lsamp <<= 8;
                        }
                        if(wavDoWrite(sfdat,(char *) &lsamp,3)){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            break;
        case(PSF_SAMP_32):
            if(do_reverse){
                for(i=0; i < nFrames; i++){
                    for(j=0;j < chans; j++) {
                        fsamp = *buf++;
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip32);
                        fsamp = max(fsamp,-dclip32);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        lsamp = psf_round(fsamp * (MAX_32BIT));
                        lsamp = REVDWBYTES(lsamp);
                        if( wavDoWrite(sfdat,(char *) &lsamp,sizeof(int))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            else {
                for(i=0; i < nFrames; i++, buf += chans){
                    for(j=0;j < chans; j++) {
                        fsamp = buf[j];
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip32);
                        fsamp = max(fsamp,-dclip32);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        lsamp = psf_round(fsamp * (MAX_32BIT));
                        
                        if(wavDoWrite(sfdat,(char *) &lsamp,sizeof(int))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            break;
        default:
            DBGFPRINTF((stderr, "wavOpenWrite: unsupported sample format\n"));
            return PSF_E_UNSUPPORTED;
            
    }
    POS64(sfdat->lastwritepos) += nFrames;
    sfdat->curframepos = (MYLONG) POS64(sfdat->lastwritepos);
    sfdat->nFrames = max(sfdat->nFrames,(DWORD) POS64(sfdat->lastwritepos));
    /*  fflush(sfdat->file); */ /* ? may need this if reading/seeking as well as  write, etc */
    return nFrames;
    
}

int psf_sndWriteDoubleFrames(int sfd, const double *buf, DWORD nFrames)
{
    int chans,lsamp;
    DWORD i;
    int j,do_reverse;
    const double *pbuf = buf;
    float fsamp,absfsamp;
    PSFFILE *sfdat;
    int do_shift = 1;
    SND_SAMP s_samp;
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    
    sfdat  = psf_files[sfd];
#ifdef _DEBUG       
    assert(sfdat->file);
    assert(sfdat->filename);
#endif
    if(buf==NULL)
        return PSF_E_BADARG;
    if(nFrames == 0)
        return nFrames;
    if(sfdat->isRead)
        return PSF_E_FILE_READONLY;
    chans = sfdat->fmt.Format.nChannels;
    
    switch(sfdat->riff_format){
        case(PSF_STDWAVE):
        case(PSF_WAVE_EX):
            do_reverse = (sfdat->is_little_endian ? 0 : 1 );
            do_shift = 1;
            break;
        case(PSF_AIFF):
        case(PSF_AIFC):
            do_reverse = (sfdat->is_little_endian ? 1 : 0 );
            do_shift = 0;
            break;
        default:
            return PSF_E_UNSUPPORTED;
    }
    if(sfdat->lastop  == PSF_OP_READ)
        fflush(sfdat->file);
    switch(sfdat->samptype){
        case(PSF_SAMP_IEEE_FLOAT):
            if(do_reverse){
                for(i=0; i < nFrames; i++){
                    for(j=0;j < chans; j++) {
                        fsamp = (float) *pbuf++;
                        if(sfdat->clip_floats){
                            fsamp = min(fsamp,1.0f);
                            fsamp = max(fsamp,-1.0f);
                        }
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        s_samp.fsamp = fsamp;
                        lsamp = s_samp.lsamp;
                        //   lsamp = * (int *) &fsamp;
                        lsamp = REVDWBYTES(lsamp);
                        if(wavDoWrite(sfdat,(char *) &lsamp,sizeof(int))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            else {
                for(i=0; i < nFrames; i++, pbuf += chans){
                    for(j=0;j < chans; j++) {
                        fsamp = (float) pbuf[j];
                        if(sfdat->clip_floats){
                            fsamp = min(fsamp,1.0f);
                            fsamp = max(fsamp,-1.0f);
                        }
                        if(wavDoWrite(sfdat,(char*)&fsamp,sizeof(float))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                        absfsamp = (float)fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                    }
                }
            }
            break;
        case(PSF_SAMP_16):
            /* TODO: optimise all this with func pointers etc */
            if(do_reverse){
                short ssamp;
                for(i=0; i < nFrames; i++){
                    for(j=0;j < chans; j++) {
                        fsamp = (float)  *buf++;
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip16);
                        fsamp = max(fsamp,-dclip16);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        if(sfdat->dithertype == PSF_DITHER_TPDF)
                            ssamp = (short) psf_round(fsamp * 32766.0 + 2.0 * trirand());
                        else
                            ssamp = (short) psf_round(fsamp * MAX_16BIT);
                        ssamp = (short) REVWBYTES(ssamp);
                        if( wavDoWrite(sfdat,(char *) &ssamp,sizeof(short))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            else {
                short ssamp;
                for(i=0; i < nFrames; i++, buf += chans){
                    for(j=0;j < chans; j++) {
                        fsamp = (float) buf[j];
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip16);
                        fsamp = max(fsamp,-dclip16);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        if(sfdat->dithertype == PSF_DITHER_TPDF)
                            ssamp = (short) psf_round(fsamp * 32766.0 + 2.0 * trirand());
                        else
                            ssamp = (short) psf_round(fsamp * MAX_16BIT);
                        
                        if(wavDoWrite(sfdat,(char *) &ssamp,sizeof(short))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            break;
        case(PSF_SAMP_24):
            if(do_reverse){
                for(i=0; i < nFrames; i++){
                    for(j=0;j < chans; j++) {
                        fsamp =(float)  *buf++;
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip24);
                        fsamp = max(fsamp,-dclip24);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        lsamp = psf_round(fsamp * MAX_32BIT);
                        lsamp = REVDWBYTES(lsamp);
                        if(do_shift){
                            if(sfdat->is_little_endian)
                                lsamp >>= 8;
                            else
                                lsamp <<= 8;
                        }
                        if( wavDoWrite(sfdat,(char *) &lsamp,3)){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            else {
                for(i=0; i < nFrames; i++, buf += chans){
                    for(j=0;j < chans; j++) {
                        fsamp = (float)  buf[j];
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip24);
                        fsamp = max(fsamp,-dclip24);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        lsamp = psf_round(fsamp * MAX_32BIT);
                        if(do_shift){
                            if(sfdat->is_little_endian)
                                lsamp >>= 8;
                            else
                                lsamp <<= 8;
                        }
                        if(wavDoWrite(sfdat,(char *) &lsamp,3)){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            break;
        case(PSF_SAMP_32):
            if(do_reverse){
                for(i=0; i < nFrames; i++){
                    for(j=0;j < chans; j++) {
                        fsamp = (float) *buf++;
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip32);
                        fsamp = max(fsamp,-dclip32);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        lsamp = psf_round(fsamp * MAX_32BIT);
                        lsamp = REVDWBYTES(lsamp);
                        if( wavDoWrite(sfdat,(char *) &lsamp,sizeof(int))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            else {
                for(i=0; i < nFrames; i++, buf += chans){
                    for(j=0;j < chans; j++) {
                        fsamp = (float) buf[j];
                        /* clip now! we may have a flag to rescale first...one day */
                        fsamp = min(fsamp,dclip32);
                        fsamp = max(fsamp,-dclip32);
                        absfsamp = (float) fabs((double)fsamp);
                        if(sfdat->pPeaks && (sfdat->pPeaks[j].val < absfsamp)){
                            sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                            sfdat->pPeaks[j].val = absfsamp;
                        }
                        lsamp = psf_round(fsamp * MAX_32BIT);
                        
                        if(wavDoWrite(sfdat,(char *) &lsamp,sizeof(int))){
                            DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                            return PSF_E_CANT_WRITE;
                        }
                    }
                }
            }
            break;
        default:
            DBGFPRINTF((stderr, "wavOpenWrite: unsupported sample format\n"));
            return PSF_E_UNSUPPORTED;
            
    }
    POS64(sfdat->lastwritepos) += nFrames;
    /* keep this as is for now, don't optimize, work in progress, etc */
    sfdat->curframepos =  (DWORD) POS64(sfdat->lastwritepos);
    sfdat->nFrames = max(sfdat->nFrames, ((DWORD) POS64(sfdat->lastwritepos)));
    /*  fflush(sfdat->file);*/  /* ? need this if reading/seeking as well as  write, etc */
    return nFrames;
    
}


/* deprecated! Do not use. */

int psf_sndWriteShortFrames(int sfd, const short *buf, DWORD nFrames)
{
    int chans;
    DWORD i;
    int j;
    PSFFILE *sfdat;
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    
    sfdat  = psf_files[sfd];
    
#ifdef _DEBUG       
    assert(sfdat->file);
    assert(sfdat->filename);
#endif
    
    if(buf==NULL)
        return PSF_E_BADARG;
    if(nFrames == 0)
        return nFrames;
    if(sfdat->isRead)
        return PSF_E_FILE_READONLY;
    chans = sfdat->fmt.Format.nChannels;
    
    /* well, it can't be ~less~ efficient than converting twice! */
    if(!sfdat->is_little_endian){
        short ssamp;
        double fval;
        for(i=0; i < nFrames; i++){
            for(j=0;j < chans; j++) {
                ssamp = *buf++;
                fval = ((double) ssamp / MAX_16BIT);
                if(sfdat->pPeaks && (sfdat->pPeaks[j].val < (float)(fabs(fval)))){
                    sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                    sfdat->pPeaks[j].val = (float)fval;
                }
                
                ssamp = (short) REVWBYTES(ssamp);
                if(wavDoWrite(sfdat,(char *) &ssamp,sizeof(short))){
                    DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                    return PSF_E_CANT_WRITE;
                }
            }
        }
    }
    else {
        short ssamp;
        double fval;
        for(i=0; i < nFrames; i++){
            for(j=0;j < chans; j++) {
                ssamp = *buf++;
                fval = ((double) ssamp / MAX_16BIT);
                if(sfdat->pPeaks && (sfdat->pPeaks[j].val < (float)(fabs(fval)))){
                    sfdat->pPeaks[j].pos = sfdat->nFrames + i;
                    sfdat->pPeaks[j].val = (float)fval;
                }
                
                if(wavDoWrite(sfdat,(char *) &ssamp,sizeof(short))){
                    DBGFPRINTF((stderr, "wavOpenWrite: write error\n"));
                    return PSF_E_CANT_WRITE;
                }
            }
        }
    }
    POS64(sfdat->lastwritepos) += nFrames;
    sfdat->nFrames = max(sfdat->nFrames, ((DWORD) POS64(sfdat->lastwritepos)));
    fflush(sfdat->file);
    return nFrames;
}

/******** READ ***********/
static int wavReadHeader(PSFFILE *sfdat)
{
    DWORD tag,version,peaktime;
    DWORD size;
    WORD cbSize;
    fpos_t bytepos;
    
    if(sfdat==NULL || sfdat->file == NULL)
        return PSF_E_BADARG;
    
    if(wavDoRead(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoRead(sfdat,(char *) &size,sizeof(DWORD)))
        return PSF_E_CANT_READ;
    if(!sfdat->is_little_endian)
        size = REVDWBYTES(size);
    else
        tag = REVDWBYTES(tag);
    if(tag != TAG('R','I','F','F'))
        return PSF_E_NOT_WAVE;
    if(size < (sizeof(WAVEFORMAT) + 3 * sizeof(WORD)))
        return PSF_E_BAD_FORMAT;
    
    if(wavDoRead(sfdat,(char *)&tag,sizeof(DWORD)))
        return PSF_E_CANT_READ;
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(tag != TAG('W','A','V','E'))
        return PSF_E_NOT_WAVE;
    for(;;){
        if(wavDoRead(sfdat,(char *)&tag,sizeof(DWORD))
           || wavDoRead(sfdat,(char *) &size,sizeof(DWORD)))
            return PSF_E_CANT_READ;
        if(!sfdat->is_little_endian)
            size = REVDWBYTES(size);
        else
            tag = REVDWBYTES(tag);
        switch(tag){
            case(TAG('f','m','t',' ')):
                if( size < sizeof(WAVEFORMAT))
                    return PSF_E_BAD_FORMAT;
                if(size > sizeof_WFMTEX)
                    return PSF_E_UNSUPPORTED;
                if(fgetpos(sfdat->file,&bytepos))
                    return PSF_E_CANT_SEEK;
                sfdat->fmtoffset = bytepos;
                if(wavDoRead(sfdat,(char *)&(sfdat->fmt.Format),sizeof(WAVEFORMAT))){
                    return PSF_E_CANT_READ;
                }
                if(!sfdat->is_little_endian)
                    fmtSwapBytes(sfdat);
                /* calling function decides if format is supported*/
                if(size > sizeof(WAVEFORMAT)) {
                    if(wavDoRead(sfdat,(char*)&cbSize,sizeof(WORD)))
                        return PSF_E_CANT_READ;
                    if(!sfdat->is_little_endian)
                        cbSize = (WORD) REVWBYTES(cbSize);
                    if(cbSize != (WORD)0) {
                        if(sfdat->fmt.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE){
                            if(cbSize != 22)
                                return PSF_E_BAD_FORMAT;
                            sfdat->riff_format = PSF_WAVE_EX;
                        }
                    }
                    else {
                        int fmtsize = 18;
                        /* cbSize = 0: has to be 18-byte WAVEFORMATEX */
                        if((sfdat->fmt.Format.wFormatTag == WAVE_FORMAT_PCM
                            || sfdat->fmt.Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT))
                            sfdat->riff_format = PSF_STDWAVE;
                        else  /* some horribly mangled format! */
                            return PSF_E_BAD_FORMAT;
                        /* Feb 2010: hack to handle bad files with overlarge fmt chunk! */
                        while (size > fmtsize){
                            char dummy;
                            if(wavDoRead(sfdat,(char*)&dummy,sizeof(char)))
                                return PSF_E_CANT_READ;
                            fmtsize++;
                            strcpy(sfdat->warnstring,"fmt chunk too large for format");
                            sfdat->illformed = 1;
                        }
                        
                    }
                    sfdat->fmt.Format.cbSize = cbSize;
                    /* fill in as if basic Format; may change later from WAVE-EX */
                    sfdat->fmt.Samples.wValidBitsPerSample = sfdat->fmt.Format.wBitsPerSample;
                    sfdat->fmt.dwChannelMask = 0;
                    /* get rest of WAVE-EX, if we have it */
                    if(sfdat->fmt.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE){
                        WORD validbits;
                        DWORD chmask;
                        if(wavDoRead(sfdat,(char *) &validbits,sizeof(WORD)))
                            return PSF_E_CANT_READ;
                        if(!sfdat->is_little_endian)
                            sfdat->fmt.Samples.wValidBitsPerSample = (WORD) REVWBYTES(validbits);
                        if(wavDoRead(sfdat,(char *) &chmask,sizeof(DWORD)))
                            return PSF_E_CANT_READ;
                        sfdat->fmt.dwChannelMask = chmask;      /* RWD OCT 2009 ! */
                        if(!sfdat->is_little_endian)
                            sfdat->fmt.dwChannelMask = REVDWBYTES(chmask);
                        /* OCT 2009 identify speaker layout */
                        sfdat->chformat = get_speakerlayout(sfdat->fmt.dwChannelMask,sfdat->fmt.Format.nChannels);
                        if(wavDoRead(sfdat,(char *) &(sfdat->fmt.SubFormat),sizeof(GUID)))
                            return PSF_E_CANT_READ;
                        if(!sfdat->is_little_endian){
                            sfdat->fmt.SubFormat.Data1 = REVDWBYTES(sfdat->fmt.SubFormat.Data1);
                            sfdat->fmt.SubFormat.Data2 = (WORD) REVWBYTES(sfdat->fmt.SubFormat.Data2);
                            sfdat->fmt.SubFormat.Data3 = (WORD) REVWBYTES(sfdat->fmt.SubFormat.Data3);
                        }
                        /* if we get a good GUID, this sets up sfdat with samplesize info */
                        if(check_guid(sfdat))
                            return PSF_E_UNSUPPORTED;
                    }
                }
                break;
            case(TAG('P','E','A','K')):
                /* I SHOULD  report an error if this is after data chunk;
                 but I suppose innocent users (e.g. of Cubase ) will grumble... */
                if(wavDoRead(sfdat,(char  *) &version,sizeof(DWORD)))
                    return PSF_E_CANT_READ;
                if(!sfdat->is_little_endian)
                    version = REVDWBYTES(version);
                if(version != 1) {
                    DBGFPRINTF((stderr, "Unexpected version level for PEAK chunk!\n"));
                    return PSF_E_UNSUPPORTED;
                }
                if(size != (2 * sizeof(DWORD) + sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels)) {
                    DBGFPRINTF((stderr, "\nBad size for PEAK chunk\n"));
                    return PSF_E_BAD_FORMAT;
                }
                if(fgetpos(sfdat->file,&bytepos))
                    return PSF_E_CANT_SEEK;
                sfdat->peakoffset = bytepos;
                if(wavDoRead(sfdat,(char *) &peaktime,sizeof(DWORD))){
                    DBGFPRINTF((stderr,"Error reading PEAK time\n"));
                    return PSF_E_CANT_READ;
                }
                if(!sfdat->is_little_endian)
                    peaktime = REVDWBYTES(peaktime);
                sfdat->peaktime = (time_t) peaktime;
                sfdat->pPeaks = (PSF_CHPEAK *) malloc(sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels);
                if(sfdat->pPeaks==NULL){
                    DBGFPRINTF((stderr, "wavOpenWrite: no memory for peak data\n"));
                    return PSF_E_NOMEM;
                }
                if(wavDoRead(sfdat,(char *) sfdat->pPeaks,sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels)) {
                    DBGFPRINTF((stderr,"Error reading PEAK peak data\n"));
                    return PSF_E_CANT_READ;
                }
                if(!sfdat->is_little_endian){
                    DWORD *pBlock;
                    int i;
                    pBlock = (DWORD *) (sfdat->pPeaks);
                    for(i=0;i < sfdat->fmt.Format.nChannels * 2; i++)
                        pBlock[i] = REVDWBYTES(pBlock[i]);
                }
                break;
            case(TAG('d','a','t','a')):
                if(fgetpos(sfdat->file,&bytepos))
                    return PSF_E_CANT_SEEK;
                sfdat->dataoffset = bytepos;
                if(POS64(sfdat->fmtoffset)==0)
                    return PSF_E_BAD_FORMAT;
                sfdat->nFrames = size / sfdat->fmt.Format.nBlockAlign;
                /* get rescale factor if available */
                /* NB in correct format, val is always >= 0.0 */
                if(sfdat->pPeaks && POS64(sfdat->peakoffset) != 0){
                    float fac = 0.0f;
                    int i;
                    for(i=0;i < sfdat->fmt.Format.nChannels; i++)
                        fac = max(fac,sfdat->pPeaks[i].val);
                    if(fac > 1.0f)
                        sfdat->rescale_fac = 1.0f / fac;
                }
                /* set sampletype */
                switch(sfdat->fmt.Format.wFormatTag){
                    case(WAVE_FORMAT_IEEE_FLOAT):
                        sfdat->samptype = PSF_SAMP_IEEE_FLOAT;
                        if(sfdat->fmt.Format.wBitsPerSample != 32) {
                            /*return PSF_E_BAD_FORMAT;*/
                            if(sfdat->fmt.Format.wBitsPerSample == sizeof(double))
                                return PSF_E_UNSUPPORTED;           /*RWD 26:10:2002 allow possibility of doubles! */
                            else
                                return PSF_E_BAD_FORMAT;
                        }
                        sfdat->lastop  = PSF_OP_READ;
                        /* RWD 26:10:2002 IMPORTANT BUGFIX! */
                        return PSF_E_NOERROR;
                    case(WAVE_FORMAT_PCM):
                    case(WAVE_FORMAT_EXTENSIBLE):
                        switch(sfdat->fmt.Format.wBitsPerSample){
                            case(8):
                                sfdat->samptype = PSF_SAMP_8;
                                break;
                            case(16):
                                sfdat->samptype = PSF_SAMP_16;
                                break;
                            case(24):
                                sfdat->samptype = PSF_SAMP_24;
                                break;
                            case(32):
                                sfdat->samptype = PSF_SAMP_32;
                                /* RWD 03/20: fix fault parsing floats amb files */
                                if(sfdat->chformat == MC_BFMT) {
                                    if(compare_guids(&(sfdat->fmt.SubFormat),&SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT)) {
                                        sfdat->samptype = PSF_SAMP_IEEE_FLOAT;
                                    }
                                    else if(!compare_guids(&(sfdat->fmt.SubFormat),&SUBTYPE_AMBISONIC_B_FORMAT_PCM)) {
                                        sfdat->samptype = PSF_SAMP_UNKNOWN;
                                    }
                                }
                                break;
                            default:
                                sfdat->samptype = PSF_SAMP_UNKNOWN;
                                break;
                        }
                        if(sfdat->samptype == PSF_SAMP_UNKNOWN)
                            return PSF_E_UNSUPPORTED;
                        sfdat->lastop  = PSF_OP_READ;
                        return PSF_E_NOERROR;
                }
            default:
                //RWD Apr 2019
                // handle odd chunk sizes
                size = (size+1)&~1;
                /* unknown chunk - skip */
                if(fseek(sfdat->file,size,SEEK_CUR))
                    return PSF_E_CANT_READ;
                break;
        }
    }
}

static int aiffReadHeader(PSFFILE *sfdat)
{
    DWORD tag,version,peaktime,remain,offset,blocksize;
    int have_comm =0,have_ssnd =0;
    DWORD dwData,size;
    unsigned char ieee[10];
    WORD wData;
    fpos_t bytepos;
    
    if(sfdat==NULL || sfdat->file == NULL)
        return PSF_E_BADARG;
    
    if(wavDoRead(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoRead(sfdat,(char *) &remain,sizeof(DWORD)))
        return PSF_E_CANT_READ;
    if(sfdat->is_little_endian) {
        remain = REVDWBYTES(remain);
        tag = REVDWBYTES(tag);
    }
    if(tag != TAG('F','O','R','M')){
        DBGFPRINTF((stderr, "file is not AIFF: no PSF chunk\n"));
        return PSF_E_BADARG;
    }
    
    if(wavDoRead(sfdat,(char *)&tag,sizeof(DWORD)))
        return PSF_E_CANT_READ;
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(tag != TAG('A','I','F','F')){
        DBGFPRINTF((stderr, "file is not AIFF: no AIFF chunk\n"));
        return PSF_E_BADARG;
    }
    remain -= sizeof(int);
    
    
    while(remain > 0){
        if(wavDoRead(sfdat,(char *)&tag,sizeof(DWORD))
           || wavDoRead(sfdat,(char *) &size,sizeof(DWORD)))
            return PSF_E_CANT_READ;
        if(sfdat->is_little_endian) {
            size = REVDWBYTES(size);
            tag = REVDWBYTES(tag);
        }
        remain -=(int)( 2 * sizeof(DWORD));
        switch(tag){
            case(TAG('C','O','M','M')):
                if(size != 18){
                    DBGFPRINTF((stderr,"AIFF file has bad size for COMM chunk\n"));
                    return PSF_E_BAD_FORMAT;
                }
                if(fgetpos(sfdat->file,&bytepos))
                    return PSF_E_CANT_SEEK;
                sfdat->fmtoffset = bytepos;
                if(wavDoRead(sfdat,(char *)&wData,sizeof(WORD)))
                    return PSF_E_CANT_READ;
                if(sfdat->is_little_endian)
                    wData = (WORD) REVWBYTES(wData);
                sfdat->fmt.Format.nChannels = wData;
                if(wavDoRead(sfdat,(char *)&dwData,sizeof(DWORD)))
                    return PSF_E_CANT_READ;
                if(sfdat->is_little_endian)
                    dwData = REVDWBYTES(dwData);
                sfdat->nFrames = dwData;
                if(wavDoRead(sfdat,(char *)&wData,sizeof(WORD)))
                    return PSF_E_CANT_READ;
                if(sfdat->is_little_endian)
                    wData = (WORD) REVWBYTES(wData);
                sfdat->fmt.Format.wBitsPerSample = wData;
                if(wavDoRead(sfdat,ieee,10))
                    return PSF_E_CANT_READ;
                sfdat->fmt.Format.nSamplesPerSec = (DWORD)(ieee_80_to_double(ieee));
                /*we have to deduce blockalign, and hence containersize*/
                /* no support (yet) for strange wordsizes such as 20 in 24 */
                switch(sfdat->fmt.Format.wBitsPerSample){
                    case(32):
                        sfdat->fmt.Format.nBlockAlign = sizeof(int);
                        sfdat->samptype = PSF_SAMP_32;
                        break;
                    case(24):
                        sfdat->fmt.Format.nBlockAlign = 3;
                        sfdat->samptype = PSF_SAMP_24;
                        break;
                    case(16):
                        sfdat->fmt.Format.nBlockAlign = sizeof(short);
                        sfdat->samptype = PSF_SAMP_16;
                        break;
                    default:
                        DBGFPRINTF((stderr, "unsupported sample format for AIFF file\n"));
                        return PSF_E_UNSUPPORTED;
                }
                sfdat->fmt.Format.nBlockAlign = (WORD) (sfdat->fmt.Format.nBlockAlign * sfdat->fmt.Format.nChannels);
                remain -= 18;
                have_comm = 1;
                break;
            case (TAG('P','E','A','K')):
                if(!have_comm){
                    DBGFPRINTF((stderr, "AIFF file: found PEAK chunk before COMM chunk!\n"));
                    return PSF_E_BAD_FORMAT;
                }
                if(size != (2 * sizeof(DWORD) + sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels)){
                    DBGFPRINTF((stderr, "AIFF file has bad size for PEAK chunk\n"));
                    return PSF_E_BAD_FORMAT;
                }
                if(wavDoRead(sfdat,(char *)&version,sizeof(DWORD))) {
                    DBGFPRINTF((stderr,"Error reading PEAK version\n"));
                    return PSF_E_CANT_READ;
                }
                if(sfdat->is_little_endian)
                    version  = REVDWBYTES(version);
                if(version != 1){
                    DBGFPRINTF((stderr, "AIFF file has unexpected version level for PEAK chunk!\n"));
                    return PSF_E_UNSUPPORTED;
                }
                if(fgetpos(sfdat->file,&bytepos))
                    return PSF_E_CANT_SEEK;
                sfdat->peakoffset = bytepos;
                if(wavDoRead(sfdat,(char *) &peaktime,sizeof(DWORD))) {
                    DBGFPRINTF((stderr,"Error reading PEAK time\n"));
                    return PSF_E_CANT_READ;
                }
                if(sfdat->is_little_endian)
                    peaktime = REVDWBYTES(peaktime);
                sfdat->peaktime = (time_t) peaktime;
                sfdat->pPeaks = (PSF_CHPEAK *)malloc(sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels);
                if(sfdat->pPeaks==NULL){
                    DBGFPRINTF((stderr, "wavOpenWrite: no memory for peak data\n"));
                    return PSF_E_NOMEM;
                }
                if(wavDoRead(sfdat,(char *)(sfdat->pPeaks),sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels)){
                    DBGFPRINTF((stderr,"Error reading PEAK peak data\n"));
                    return PSF_E_CANT_READ;
                }
                if(sfdat->is_little_endian){
                    DWORD *pBlock;
                    int i;
                    pBlock = (DWORD *) (sfdat->pPeaks);
                    for(i=0;i < sfdat->fmt.Format.nChannels * 2; i++)
                        pBlock[i] = REVDWBYTES(pBlock[i]);
                }
                remain -= size;
                break;
            case(TAG('S','S','N','D')):
                if(wavDoRead(sfdat,(char *)&offset,sizeof(DWORD))
                   || wavDoRead(sfdat,(char *) &blocksize,sizeof(DWORD)))
                    return PSF_E_CANT_READ;
                if(sfdat->is_little_endian){
                    offset  = REVDWBYTES(offset);
                    blocksize = REVDWBYTES(blocksize);
                }
                if(fgetpos(sfdat->file,&bytepos))
                    return PSF_E_CANT_SEEK;
                sfdat->dataoffset = bytepos;
                POS64(sfdat->dataoffset) += offset;
                sfdat->nFrames = (size - 2* sizeof(DWORD))/ sfdat->fmt.Format.nBlockAlign;
                /* NB for seek: we used up 8 bytes with offset and blocksize */
                /* if we already have COMM, we could finish here! */
                /* TODO! this is still a signed int seek, no good for 4GB */
                if(fseek(sfdat->file,((size - 2* sizeof(DWORD))+1)&~1,SEEK_CUR))
                    return PSF_E_CANT_SEEK;
                have_ssnd = 1;
                remain -= (size+1)&~1;
                break;
                /*HARSH! traps linux sox error, for example */
            case(0):
                DBGFPRINTF((stderr, "AIFF file has bad main chunksize\n"));
                return PSF_E_BAD_FORMAT;
            default:
                /* skip all unknown chunks */
                if(fseek(sfdat->file,(size+1)&~1,SEEK_CUR))
                    return PSF_E_CANT_SEEK;
                remain -= (size+1)&~1;
                break;
        }
    }
    if(!(have_ssnd && have_comm)){
        DBGFPRINTF((stderr, "AIFF file has missing chunks\n"));
        return PSF_E_BAD_FORMAT;
    }
    /* we have seeked to EOF, so rewind to start of data */
    if(fsetpos(sfdat->file,&sfdat->dataoffset))
        return PSF_E_CANT_SEEK;
    sfdat->curframepos = 0;
    sfdat->riff_format = PSF_AIFF;
    /* get rescale factor if available */
    /* NB in correct format, val is always >= 0.0 */
    if(sfdat->pPeaks &&  POS64(sfdat->peakoffset) != 0){
        float fac = 0.0f;
        int i;
        for(i=0;i < sfdat->fmt.Format.nChannels; i++)
            fac = max(fac,sfdat->pPeaks[i].val);
        if(fac > 1.0f)
            sfdat->rescale_fac = 1.0f / fac;
    }
    sfdat->lastop  = PSF_OP_READ;
    return PSF_E_NOERROR;
}

static int aifcReadHeader(PSFFILE *sfdat)
{
    DWORD tag,version,peaktime,remain,offset,blocksize;
    int have_comm =0,have_ssnd =0,have_fver = 0;
    DWORD dwData,size,aifcver,ID_compression;
    unsigned char ieee[10];
    WORD wData;
    fpos_t bytepos;
    
    if(sfdat==NULL || sfdat->file == NULL)
        return PSF_E_BADARG;
    
    if(wavDoRead(sfdat,(char *)&tag,sizeof(DWORD))
       || wavDoRead(sfdat,(char *) &remain,sizeof(DWORD)))
        return PSF_E_CANT_READ;
    if(sfdat->is_little_endian) {
        remain = REVDWBYTES(remain);
        tag = REVDWBYTES(tag);
    }
    if(tag != TAG('F','O','R','M')){
        DBGFPRINTF((stderr, "file is not AIFC: no FORM chunk\n"));
        return PSF_E_BADARG;
    }
    
    if(wavDoRead(sfdat,(char *)&tag,sizeof(DWORD)))
        return PSF_E_CANT_READ;
    if(sfdat->is_little_endian)
        tag = REVDWBYTES(tag);
    if(tag != TAG('A','I','F','C')){
        DBGFPRINTF((stderr, "file is not AIFC: no AIFC chunk\n"));
        return PSF_E_BADARG;
    }
    remain -= sizeof(DWORD);
    
    
    while(remain > 0){
        if(wavDoRead(sfdat,(char *)&tag,sizeof(DWORD))
           || wavDoRead(sfdat,(char *) &size,sizeof(DWORD)))
            return PSF_E_CANT_READ;
        if(sfdat->is_little_endian) {
            size = REVDWBYTES(size);
            tag = REVDWBYTES(tag);
        }
        remain -= 2 * sizeof(DWORD);
        switch(tag){
            case(TAG('F','V','E','R')):
                if(size != sizeof(DWORD)){
                    DBGFPRINTF((stderr, "AIFC file has bad size for FVER chunk\n"));
                    return PSF_E_BAD_FORMAT;
                }
                if(wavDoRead(sfdat,(char *) &aifcver,sizeof(DWORD)))
                    return PSF_E_CANT_READ;
                if(sfdat->is_little_endian)
                    aifcver = REVDWBYTES(aifcver);
                remain-= sizeof(DWORD);
                if(aifcver != AIFC_VERSION_1)
                    return PSF_E_UNSUPPORTED;
                have_fver = 1;
                break;
            case(TAG('C','O','M','M')):
                if(size < 22) {
                    DBGFPRINTF((stderr, "AIFC file has bad size for COMM chunk\n"));
                    return PSF_E_BAD_FORMAT;
                }
                if(fgetpos(sfdat->file,&sfdat->fmtoffset))
                    return PSF_E_CANT_SEEK;
                if(wavDoRead(sfdat,(char *)&wData,sizeof(WORD)))
                    return PSF_E_CANT_READ;
                if(sfdat->is_little_endian)
                    wData = (WORD) REVWBYTES(wData);
                sfdat->fmt.Format.nChannels = wData;
                if(wavDoRead(sfdat,(char *)&dwData,sizeof(DWORD)))
                    return PSF_E_CANT_READ;
                if(sfdat->is_little_endian)
                    dwData = REVDWBYTES(dwData);
                sfdat->nFrames = dwData;
                if(wavDoRead(sfdat,(char *)&wData,sizeof(WORD)))
                    return PSF_E_CANT_READ;
                if(sfdat->is_little_endian)
                    wData = (WORD) REVWBYTES(wData);
                sfdat->fmt.Format.wBitsPerSample = wData;
                if(wavDoRead(sfdat,ieee,10))
                    return PSF_E_CANT_READ;
                sfdat->fmt.Format.nSamplesPerSec = (DWORD)(ieee_80_to_double(ieee));
                /*we have to deduce blockalign, and hence containersize*/
                /* no support for strange wordsizes such as 20 in 24 */
                switch(sfdat->fmt.Format.wBitsPerSample){
                    case(32):
                        sfdat->fmt.Format.nBlockAlign = sizeof(DWORD);
                        sfdat->samptype = PSF_SAMP_32;
                        break;
                    case(24):
                        sfdat->fmt.Format.nBlockAlign = 3;
                        sfdat->samptype = PSF_SAMP_24;
                        break;
                    case(16):
                        sfdat->fmt.Format.nBlockAlign = sizeof(short);
                        sfdat->samptype = PSF_SAMP_16;
                        break;
                    default:
                        DBGFPRINTF((stderr, "unsupported sample format for AIFC file\n"));
                        return PSF_E_UNSUPPORTED;
                }
                sfdat->fmt.Format.nBlockAlign = (WORD) (sfdat->fmt.Format.nBlockAlign * sfdat->fmt.Format.nChannels);
                if(wavDoRead(sfdat,(char *)&ID_compression,sizeof(DWORD))){
                    return PSF_E_CANT_READ;
                }
                if(sfdat->is_little_endian)
                    ID_compression = REVDWBYTES(ID_compression);
                if(!(    (ID_compression == TAG('N','O','N','E'))
                     || (ID_compression == TAG('F','L','3','2'))
                     || (ID_compression == TAG('f','l','3','2'))
                     || (ID_compression == TAG('i','n','2','4')))
                   ){
                    DBGFPRINTF((stderr, "AIFC file: unsupported compression format\n"));
                    return PSF_E_UNSUPPORTED;
                }
                /*set stype info */
                if((ID_compression== TAG('F','L','3','2'))
                   || ID_compression == TAG('f','l','3','2')){
                    if(sfdat->fmt.Format.wBitsPerSample != 32){
                        DBGFPRINTF((stderr, "AIFC file: samples not 32bit in floats file\n"));
                        return PSF_E_BAD_FORMAT;
                    }
                    else {
                        sfdat->fmt.Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
                        sfdat->samptype = PSF_SAMP_IEEE_FLOAT;
                    }
                }
                /* yes, lazy! skip past pascal string*/
                if(fseek(sfdat->file,((size-22)+1)&~1,SEEK_CUR))    /*written for documentation, not terseness!*/
                    return PSF_E_CANT_SEEK;
                remain -= (size+1)&~1;
                have_comm = 1;
                break;
            case (TAG('P','E','A','K')):
                if(!have_comm){
                    DBGFPRINTF((stderr, "\nAIFC file: found PEAK chunk before COMM chunk!\n"));
                    return PSF_E_BAD_FORMAT;
                }
                if(size != (2 * sizeof(DWORD) + sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels)){
                    DBGFPRINTF((stderr, "\nBad size for PEAK chunk\n"));
                    return PSF_E_BAD_FORMAT;
                }
                if(wavDoRead(sfdat,(char *)&version,sizeof(DWORD)))
                    return PSF_E_CANT_READ;
                if(sfdat->is_little_endian)
                    version  = REVDWBYTES(version);
                if(version != 1) {
                    DBGFPRINTF((stderr, "Unexpected version level for PEAK chunk!\n"));
                    return PSF_E_UNSUPPORTED;
                }
                if(fgetpos(sfdat->file,&bytepos))
                    return PSF_E_CANT_SEEK;
                sfdat->peakoffset = bytepos;
                if(wavDoRead(sfdat,(char *) &peaktime,sizeof(DWORD))){
                    DBGFPRINTF((stderr,"Error reading PEAK time\n"));
                    return PSF_E_CANT_READ;
                }
                if(sfdat->is_little_endian)
                    peaktime = REVDWBYTES(peaktime);
                sfdat->peaktime = (time_t) peaktime;
                sfdat->pPeaks = (PSF_CHPEAK *)malloc(sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels);
                if(sfdat->pPeaks==NULL) {
                    DBGFPRINTF((stderr, "wavOpenWrite: no memory for peak data\n"));
                    return PSF_E_NOMEM;
                }
                if(wavDoRead(sfdat,(char *)(sfdat->pPeaks),sizeof(PSF_CHPEAK) * sfdat->fmt.Format.nChannels)) {
                    DBGFPRINTF((stderr,"Error reading PEAK peak data\n"));
                    return PSF_E_CANT_READ;
                }
                if(sfdat->is_little_endian){
                    DWORD *pBlock;
                    int i;
                    pBlock = (DWORD *) (sfdat->pPeaks);
                    for(i=0;i < sfdat->fmt.Format.nChannels * 2; i++)
                        pBlock[i] = REVDWBYTES(pBlock[i]);
                }
                remain -= (size+1)&~1;
                break;
            case(TAG('S','S','N','D')):
                if(wavDoRead(sfdat,(char *)&offset,sizeof(DWORD))
                   || wavDoRead(sfdat,(char *) &blocksize,sizeof(DWORD)))
                    return PSF_E_CANT_READ;
                if(sfdat->is_little_endian){
                    offset  = REVDWBYTES(offset);
                    blocksize = REVDWBYTES(blocksize);
                }
                if(fgetpos(sfdat->file,&bytepos))
                    return PSF_E_CANT_SEEK;
                sfdat->dataoffset = bytepos;
                POS64(sfdat->dataoffset) += offset;
                sfdat->nFrames = (size - 2* sizeof(DWORD))/ sfdat->fmt.Format.nBlockAlign;
                if(fseek(sfdat->file,((size - 2* sizeof(DWORD))+1)&~1,SEEK_CUR))
                    return PSF_E_CANT_SEEK;
                have_ssnd = 1;
                remain -= (size+1)&~1;
                break;
                /* HARSH! as above */
            case(0):
                DBGFPRINTF((stderr, "AIFC file has bad main chunksize\n"));
                return PSF_E_BAD_FORMAT;
            default:
                /* skip all unknown chunks */
                if(fseek(sfdat->file,(size+1)&~1,SEEK_CUR))
                    return PSF_E_CANT_SEEK;
                remain -= (size+1)&~1;
                break;
        }
    }
    if(!(have_ssnd && have_comm && have_fver)){
        DBGFPRINTF((stderr, "AIFC file has bad format\n"));
        return PSF_E_BAD_FORMAT;
    }
    /* we have seeked (ugh) to EOF, so rewind to start of data */
    if(fsetpos(sfdat->file,&sfdat->dataoffset))
        return PSF_E_CANT_SEEK;
    sfdat->curframepos = 0;
    sfdat->riff_format = PSF_AIFC;
    /* get rescale factor if available */
    /* NB in correct format, val is always >= 0.0 */
    if(sfdat->pPeaks &&  POS64(sfdat->peakoffset) != 0){
        float fac = 0.0f;
        int i;
        for(i=0;i < sfdat->fmt.Format.nChannels; i++)
            fac = max(fac,sfdat->pPeaks[i].val);
        if(fac > 1.0f)
            sfdat->rescale_fac = 1.0f / fac;
    }
    sfdat->lastop  = PSF_OP_READ;
    return PSF_E_NOERROR;
}
/* only RDONLY access supported */
int psf_sndOpen(const char *path,PSF_PROPS *props, int rescale)
{
    int i,rc = 0;
    PSFFILE *sfdat;
    psf_format fmt;
    
    /* RWD interesting syntax issue: I need the curlies, or break doesn't work properly */
    for(i=0;i < psf_maxfiles;i++) {
        if(psf_files[i]==NULL)
            break;
    }
    if(i==psf_maxfiles){
        
        return PSF_E_TOOMANYFILES;
    }
    
    sfdat = psf_newFile(NULL);
    if(sfdat==NULL){
        return PSF_E_NOMEM;
    }
    sfdat->rescale = rescale;
    sfdat->is_little_endian = byte_order();
    fmt = psf_getFormatExt(path);
    if(!(fmt==PSF_STDWAVE || fmt==PSF_WAVE_EX || fmt==PSF_AIFF || fmt==PSF_AIFC))
        return PSF_E_BADARG;
    
    if((sfdat->file = fopen(path,"rb"))  == NULL) {
        DBGFPRINTF((stderr, "psf_sndOpen: cannot open '%s'\n", path));
        return PSF_E_CANT_OPEN;
    }
    sfdat->filename = (char *) malloc(strlen(path)+1);
    if(sfdat->filename==NULL) {
        return PSF_E_NOMEM;
    }
    strcpy(sfdat->filename, path);
    sfdat->isRead =  1;
    sfdat->nFrames = 0;
    /* no need to calc header sizes */
    switch(fmt){
        case(PSF_STDWAVE):
        case(PSF_WAVE_EX):
            rc =  wavReadHeader(sfdat);
            break;
        case(PSF_AIFF):
            /* some .aiff files may actually be aifc - esp if floats! */
        case(PSF_AIFC):
            rc = aiffReadHeader(sfdat);
            /* try AIFC if AIFF fails */
            if(rc < PSF_E_NOERROR) {
                rewind(sfdat->file);
                rc =  aifcReadHeader(sfdat);
            }
            break;
        default:
            DBGFPRINTF((stderr, "psf_sndOpen: unsupported file format\n"));
            rc =  PSF_E_UNSUPPORTED;
    }
    if(rc < PSF_E_NOERROR)
        return rc;
    /* fill props info*/
    props->srate    = sfdat->fmt.Format.nSamplesPerSec;
    props->chans    = sfdat->fmt.Format.nChannels;
    props->samptype = sfdat->samptype;
    props->chformat = sfdat->chformat;
    props->format      =  fmt;
    
    if(fmt==PSF_STDWAVE && (sfdat->riff_format == PSF_WAVE_EX))
        props->format = PSF_WAVE_EX;
    
    psf_files[i] = sfdat;
    return i;
}


int psf_sndReadFloatFrames(int sfd, float *buf, DWORD nFrames)
{
    int chans;
    DWORD framesread;
    int blocksize,lsamp;
    int temp;
    int i,do_reverse;
    short ssamp;
    float *pbuf = buf;
    float fsamp;
    PSFFILE *sfdat;
    int do_shift;
    SND_SAMP s_samp;
#ifdef _DEBUG
    static int debug = 1;
#endif
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    if(buf==NULL)
        return PSF_E_BADARG;
    if(nFrames == 0)
        return nFrames;
    sfdat  = psf_files[sfd];
#ifdef _DEBUG
    assert(sfdat);
    assert(sfdat->file);
    assert(sfdat->filename);
    /* must check our calcs! */
    assert(sfdat->curframepos <= sfdat->nFrames);
#endif
    /* how much do we have left? return immediately if none! */
    chans = sfdat->fmt.Format.nChannels;
    framesread = min(sfdat->nFrames - sfdat->curframepos,nFrames);
    if(framesread==0)
        return (long) framesread;
    
    blocksize =  framesread * chans;
    switch(sfdat->riff_format){
        case(PSF_STDWAVE):
        case(PSF_WAVE_EX):
            do_reverse = (sfdat->is_little_endian ? 0 : 1 );
            do_shift = 1;
            break;
        case(PSF_AIFF):
        case(PSF_AIFC):
            do_reverse = (sfdat->is_little_endian ? 1 : 0 );
            do_shift = 0;
            break;
        default:
            return PSF_E_UNSUPPORTED;
    }
    if(sfdat->lastop == PSF_OP_WRITE)
        fflush(sfdat->file);
    switch(sfdat->samptype){
        case(PSF_SAMP_IEEE_FLOAT):
            
            if(do_reverse){
                for(i=0;i < blocksize;i ++){
                    if(wavDoRead(sfdat,(char *)&lsamp,sizeof(int)))
                        return PSF_E_CANT_READ;
                    lsamp = REVDWBYTES(lsamp);
                    //    fsamp = * (float *)&lsamp;
                    s_samp.lsamp = lsamp;
                    fsamp = s_samp.fsamp;
                    if(sfdat->rescale)
                        fsamp *= sfdat->rescale_fac;
                    *pbuf++ = fsamp;
                }
            }
            else{
                if(wavDoRead(sfdat,(char *) buf,blocksize * sizeof(float)))
                    return PSF_E_CANT_READ;
                if(sfdat->rescale){
                    pbuf = buf;
                    for(i=0;i < blocksize; i++)
                        *pbuf++  *= sfdat->rescale_fac;
                }
            }
            break;
        case(PSF_SAMP_16):
            if(do_reverse){
                for(i = 0; i < blocksize; i++){
                    if(wavDoRead(sfdat,(char *)&ssamp,sizeof(short)))
                        return PSF_E_CANT_READ;
                    ssamp = (short) REVWBYTES(ssamp);
                    fsamp = (float)((double) ssamp / MAX_16BIT);
                    *pbuf++ = fsamp;
                }
            }
            else{
                for(i = 0; i < blocksize; i++){
                    if(wavDoRead(sfdat,(char *)&ssamp,sizeof(short)))
                        return PSF_E_CANT_READ;
                    fsamp = (float)((double) ssamp / MAX_16BIT);
                    *pbuf++ = fsamp;
                }
            }
            break;
        case(PSF_SAMP_24):
            if(do_reverse){
#ifdef _DEBUG
                if(debug){
                    printf("do_reverse: riffformat=%d do_shift = %d little_endian = %d\n",
                           sfdat->riff_format,do_shift,sfdat->is_little_endian);
                    debug = 0;
                }
#endif            
                for(i=0;i < blocksize;i++){
                    temp = 0;
                    if(wavDoRead(sfdat,(char *)&temp,3))
                        return PSF_E_CANT_READ;
                    lsamp = REVDWBYTES(temp);
                    if(do_shift)
                        lsamp <<= 8;
                    fsamp = (float)((double)(lsamp) / MAX_32BIT);
                    *pbuf++ = fsamp;
                }
            }
            else{
                for(i=0;i < blocksize;i++){
                    lsamp = 0;
                    if(wavDoRead(sfdat,(char *)&lsamp,3))
                        return PSF_E_CANT_READ;
                    if(do_shift)
                        lsamp <<= 8;
                    fsamp = (float)((double)(lsamp) / MAX_32BIT);
                    *pbuf++ = fsamp;
                }
            }
            break;
        case(PSF_SAMP_32):
            if(do_reverse){
                for(i=0;i < blocksize;i++){
                    if(wavDoRead(sfdat,(char *)&lsamp,sizeof(int)))
                        return PSF_E_CANT_READ;
                    lsamp = REVDWBYTES(lsamp);
                    fsamp = (float)((double)(lsamp) / MAX_32BIT);
                    *pbuf++ = fsamp;
                }
            }
            else{
                for(i=0;i < blocksize;i++){
                    lsamp = 0L;
                    if(wavDoRead(sfdat,(char *)&lsamp,sizeof(int)))
                        return PSF_E_CANT_READ;
                    fsamp = (float)((double)(lsamp) / MAX_32BIT);
                    *pbuf++ = fsamp;
                }
            }
            break;
        default:
            DBGFPRINTF((stderr, "psf_sndOpen: unsupported sample format\n"));
            return PSF_E_UNSUPPORTED;
    }
    sfdat->curframepos += framesread;
    
    return framesread;
}


/* read doubles version! */
int psf_sndReadDoubleFrames(int sfd, double *buf, DWORD nFrames)
{
    int chans;
    DWORD framesread;
    int blocksize,lsamp;
    int i,do_reverse;
    short ssamp;
    double *pbuf = buf;
    float fsamp;
    PSFFILE *sfdat;
    SND_SAMP s_samp;
    int do_shift;
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    if(buf==NULL)
        return PSF_E_BADARG;
    if(nFrames == 0)
        return nFrames;
    sfdat  = psf_files[sfd];
#ifdef _DEBUG
    assert(sfdat);
    assert(sfdat->file);
    assert(sfdat->filename);
    /* must check our calcs! */
    assert(sfdat->curframepos <= sfdat->nFrames);
#endif
    /* how much do we have left? return immediately if none! */
    chans = sfdat->fmt.Format.nChannels;
    framesread = min(sfdat->nFrames - sfdat->curframepos,nFrames);
    if(framesread==0)
        return (long) framesread;
    
    blocksize =  framesread * chans;
    switch(sfdat->riff_format){
        case(PSF_STDWAVE):
        case(PSF_WAVE_EX):
            do_reverse = (sfdat->is_little_endian ? 0 : 1 );
            do_shift = 1;
            break;
        case(PSF_AIFF):
        case(PSF_AIFC):
            do_reverse = (sfdat->is_little_endian ? 1 : 0 );
            do_shift = 0;
            break;
        default:
            return PSF_E_UNSUPPORTED;
    }
    if(sfdat->lastop == PSF_OP_WRITE)
        fflush(sfdat->file);
    switch(sfdat->samptype){
        case(PSF_SAMP_IEEE_FLOAT):
            if(do_reverse){
                for(i=0;i < blocksize;i ++){
                    if(wavDoRead(sfdat,(char *)&lsamp,sizeof(int)))
                        return PSF_E_CANT_READ;
                    lsamp = REVDWBYTES(lsamp);
                    //    fsamp = * (float *)&lsamp;
                    s_samp.lsamp = lsamp;
                    fsamp = s_samp.fsamp;
                    if(sfdat->rescale)
                        fsamp *= sfdat->rescale_fac;
                    *pbuf++ = (double) fsamp;
                }
            }
            else{
                for(i=0;i < blocksize;i++){
                    if(wavDoRead(sfdat,(char *) &fsamp, sizeof(float)))
                        return PSF_E_CANT_READ;
                    *pbuf++ = (double) fsamp;
                }
                if(sfdat->rescale){
                    pbuf = buf;
                    for(i=0;i < blocksize; i++)
                        *pbuf++  *= sfdat->rescale_fac;
                }
            }
            break;
        case(PSF_SAMP_16):
            if(do_reverse){
                for(i = 0; i < blocksize; i++){
                    if(wavDoRead(sfdat,(char *)&ssamp,sizeof(short)))
                        return PSF_E_CANT_READ;
                    ssamp = (short) REVWBYTES(ssamp);
                    fsamp = (float)((double) ssamp / MAX_16BIT);
                    *pbuf++ = (double) fsamp;
                }
            }
            else{
                for(i = 0; i < blocksize; i++){
                    if(wavDoRead(sfdat,(char *)&ssamp,sizeof(short)))
                        return PSF_E_CANT_READ;
                    fsamp = (float)((double) ssamp / MAX_16BIT);
                    *pbuf++ = (double) fsamp;
                }
            }
            break;
        case(PSF_SAMP_24):
            if(do_reverse){
                for(i=0;i < blocksize;i++){
                    if(wavDoRead(sfdat,(char *)&lsamp,3))
                        return PSF_E_CANT_READ;
                    lsamp = REVDWBYTES(lsamp);
                    if(do_shift)
                        lsamp <<= 8;
                    fsamp = (float)((double)(lsamp) / MAX_32BIT);
                    *pbuf++ = (double) fsamp;
                }
            }
            else{
                for(i=0;i < blocksize;i++){
                    lsamp = 0L;
                    if(wavDoRead(sfdat,(char *)&lsamp,3))
                        return PSF_E_CANT_READ;
                    if(do_shift)
                        lsamp <<= 8;
                    fsamp = (float)((double)(lsamp) / MAX_32BIT);
                    *pbuf++ = (double) fsamp;
                }
            }
            break;
        case(PSF_SAMP_32):
            if(do_reverse){
                for(i=0;i < blocksize;i++){
                    if(wavDoRead(sfdat,(char *)&lsamp,sizeof(int)))
                        return PSF_E_CANT_READ;
                    lsamp = REVDWBYTES(lsamp);
                    fsamp = (float)((double)(lsamp) / MAX_32BIT);
                    *pbuf++ = (double) fsamp;
                }
            }
            else{
                for(i=0;i < blocksize;i++){
                    lsamp = 0L;
                    if(wavDoRead(sfdat,(char *)&lsamp,sizeof(int)))
                        return PSF_E_CANT_READ;
                    fsamp = (float)((double)(lsamp) / MAX_32BIT);
                    *pbuf++ = (double) fsamp;
                }
            }
            break;
        default:
            DBGFPRINTF((stderr, "psf_sndOpen: unsupported sample format\n"));
            return PSF_E_UNSUPPORTED;
    }
    sfdat->curframepos += framesread;
    
    return framesread;
}


#ifdef _DEBUG
/* private test func to get raw file size */
/* verify sfdat->nFrames always <= true EOF position */
/* NB: size value is wrong if there is junk after data chunk! */
static fpos_t getsize(FILE* fp){
    fpos_t curpos;
    fpos_t size;
    if(fgetpos(fp,&curpos))
        return -1;
    
    if(fseek(fp,0,SEEK_END))
        return -1;
    
    if(fgetpos(fp,&size))
        return -1;
    
    if(fsetpos(fp,&curpos))
        return -1;
    
    return  size;
}
#endif

/* return size in m/c frames */
/* signed long because we want error return */
/* 64 bit: this would have to return a 64bit type (or unsigned 32bit int) IF we want to support 4GB 8bit sfiles! */
/* (which we don't) */
int psf_sndSize(int sfd)
{
    PSFFILE *sfdat;
#ifdef _DEBUG
    fpos_t size;
    DWORD framesize;
#endif
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    
    sfdat  = psf_files[sfd];
    if(sfdat==NULL)
        return PSF_E_BADARG;
#ifdef _DEBUG       
    assert(sfdat->file);
    assert(sfdat->filename);
    if((size = getsize(sfdat->file)) < 0)   {
        DBGFPRINTF((stderr, "getsize() error in psf_sndSize().\n"));
        return -1;
    }
    /* NB deceived if any pad or other junk after data chunk! */
    framesize = (DWORD)((POS64(size) - (MYLONG)(sfdat->dataoffset)) / sfdat->fmt.Format.nBlockAlign);
    assert(framesize >= (DWORD) sfdat->nFrames);
    
#endif
    
    return sfdat->nFrames;
    
}

/* returns multi-channel (frame)  position */
/* 64bit see above! */
int psf_sndTell(int sfd)
{
    fpos_t pos;
    PSFFILE *sfdat;
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    
    sfdat  = psf_files[sfd];
    if(sfdat==NULL)
        return PSF_E_BADARG;
#ifdef _DEBUG       
    assert(sfdat->file);
    assert(sfdat->filename);
#endif
    
    if(fgetpos(sfdat->file,&pos))
        return PSF_E_CANT_SEEK;
    POS64(pos) -= POS64(sfdat->dataoffset);
    POS64(pos) /= sfdat->fmt.Format.nBlockAlign;
#ifdef _DEBUG
    if(sfdat->lastop == PSF_OP_READ)
        assert(pos== sfdat->curframepos);
    else if(sfdat->lastop == PSF_OP_WRITE)
    /* RWD this will be out (but == curframepos) if lastop was a read . so maybe say >=, or test for lastop ? */
        assert(pos == sfdat->lastwritepos);
#endif
    return (int) POS64(pos);
}


int psf_sndSeek(int sfd,int offset, int mode)
{
    long byteoffset;    /* can be negative - limited to 2GB moves*/
    fpos_t data_end,pos_target,cur_pos;
    PSFFILE *sfdat;
    
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    
    sfdat  = psf_files[sfd];
    if(sfdat==NULL)
        return PSF_E_BADARG;
#ifdef _DEBUG       
    assert(sfdat->file);
    assert(sfdat->filename);
#endif
    /* RWD NB:dataoffset test only valid for files with headers! */
    if(POS64(sfdat->dataoffset)==0)
        return PSF_E_BADARG;
    /* or, it indicates a RAW file.... */
    
    byteoffset =  offset *  sfdat->fmt.Format.nBlockAlign;
    POS64(data_end) = POS64(sfdat->dataoffset) + (sfdat->nFrames * sfdat->fmt.Format.nBlockAlign);
    switch(mode){
        case PSF_SEEK_SET:
            POS64(pos_target) =  POS64(sfdat->dataoffset) + byteoffset;
            if(fsetpos(sfdat->file,&pos_target))
                return PSF_E_CANT_SEEK;
            break;
        case PSF_SEEK_END:
            /* NB can't just seek to end of file as there may be junk after data chunk! */
            POS64(pos_target) = POS64(data_end) + byteoffset;
            if(fsetpos(sfdat->file,&pos_target))
                return PSF_E_CANT_SEEK;
            break;
        case PSF_SEEK_CUR:
            /* should be safe using fseek for SEEK_END */
            /* Currently UNDECIDED whether to allow seeks beyond end of file! */
            if(fseek(sfdat->file,byteoffset,SEEK_CUR))
                return PSF_E_CANT_SEEK;
            break;
    }
    if(fgetpos(sfdat->file,&cur_pos))
        return PSF_E_CANT_SEEK;
    if(POS64(cur_pos) >= POS64(sfdat->dataoffset)){
        sfdat->curframepos = (DWORD)(POS64(cur_pos) -  POS64(sfdat->dataoffset))  / sfdat->fmt.Format.nBlockAlign;
        if(!sfdat->isRead)  {       /*RWD NEW*/
            /* we are rewinding a file open for writing */
            POS64(sfdat->lastwritepos) = sfdat->curframepos;
        }
        return PSF_E_NOERROR;
    }
    else
        return PSF_E_CANT_SEEK;
}


/* decide sfile format from the filename extension */
/* TODO: add func to get format from file header */
psf_format psf_getFormatExt(const char *path)
{
    char *lastdot;
    if(path==NULL || (strlen(path) < 4))
        return PSF_FMT_UNKNOWN;             /* TODO: support RAW data... */
    lastdot = strrchr(path,'.');
    if(lastdot==NULL)
        return PSF_FMT_UNKNOWN;
    
    if(stricmp(lastdot,".wav")==0)
        return PSF_STDWAVE;
    else if((stricmp(lastdot,".aif")==0) || stricmp(lastdot,".aiff")==0)
        return PSF_AIFF;
    else if((stricmp(lastdot,".afc")==0) || stricmp(lastdot,".aifc")==0)
        return PSF_AIFC;
    /* Ambisonic b-format files */
    else if(stricmp(lastdot,".wxyz")==0)
        return PSF_STDWAVE;
    else if(stricmp(lastdot,".amb")==0)
        return PSF_WAVE_EX;
    else
        return PSF_FMT_UNKNOWN;
    
}

/* return 0 for no PEAK data, 1 for success */
/* NB: we read PEAK data from sfdat, so we can read peaks while writing the file, before closing */
int psf_sndReadPeaks(int sfd,PSF_CHPEAK peakdata[],MYLONG *peaktime)
{
    int i,nchans;
    PSFFILE *sfdat;
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    
    sfdat  = psf_files[sfd];
    if(sfdat==NULL)
        return PSF_E_BADARG;
#ifdef _DEBUG       
    assert(sfdat->file);
    assert(sfdat->filename);
#endif
    /* TODO: we may want to have this, for RAW files, even though we won't write it */
    if(sfdat->pPeaks==NULL){           /*NOT an error: just don't have the chunk*/
        if(peaktime!= NULL)
            *peaktime = 0;
        return 0;
        
    }
    if(peaktime != NULL)
        *peaktime = (int) sfdat->peaktime;
    nchans = sfdat->fmt.Format.nChannels;
    for(i=0;i < nchans;i++){
        peakdata[i].val = sfdat->pPeaks[i].val;
        peakdata[i].pos = sfdat->pPeaks[i].pos;
    }
    return 1;
}

static float trirand(void)
{
    double r1,r2;
    r1 = (double) rand() * inv_randmax;
    r2 = (double) rand() * inv_randmax;
    
    return (float)((r1 + r2) - 1.0);
    
    
}

int psf_sndSetDither(int sfd,unsigned int dtype)
{
    PSFFILE *sfdat;;
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    
    sfdat  = psf_files[sfd];
    if(sfdat==NULL)
        return PSF_E_BADARG;
#ifdef _DEBUG       
    assert(sfdat->file);
    assert(sfdat->filename);
#endif
    if(dtype < PSF_DITHER_OFF || dtype > PSF_DITHER_TPDF || sfdat->isRead)
        return PSF_E_BADARG;
    
    sfdat->dithertype = dtype;
    
    return PSF_E_NOERROR;
    
}
/* get current dither setting */
int psf_sndGetDither(int sfd)
{
    PSFFILE *sfdat;
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    
    sfdat  = psf_files[sfd];
    if(sfdat==NULL)
        return PSF_E_BADARG;
#ifdef _DEBUG       
    assert(sfdat->file);
    assert(sfdat->filename);
#endif
    
    return sfdat->dithertype;
    
}

/* OCT 2009 */
psf_channelformat get_speakerlayout(DWORD chmask,DWORD chans)
{
    psf_channelformat chformat = MC_WAVE_EX;    // default is some weird format!
    
    /* accept chancount > numbits set in speakermask */
    switch(chmask){
            /*check all cross-platform formats first...*/
        case(SPKRS_UNASSIGNED):
            chformat = MC_STD;
            break;
        case(SPKRS_MONO):
            if(chans==1)
                chformat = MC_MONO;
            break;
        case(SPKRS_STEREO):
            if(chans==2)
                chformat = MC_STEREO;
            break;
        case(SPKRS_GENERIC_QUAD):
            if(chans==4)
                chformat = MC_QUAD;
            break;
        case(SPKRS_SURROUND_LCRS):
            if(chans==4)
                chformat = MC_LCRS;
            break;
        case(SPKRS_DOLBY5_1):
            if(chans==6)
                chformat = MC_DOLBY_5_1;
            break;
        case(SPKRS_SURR_5_0):                   /* March 2008 */
            if(chans==5)
                chformat = MC_SURR_5_0;
            break;
        case(SPKRS_6_1):
            if(chans==7)
                chformat = MC_SURR_6_1;
            break;
        case(SPKRS_7_1):
            if(chans==8)
                chformat = MC_SURR_7_1;
            break;
        case SPKRS_CUBE:
            if(chans==8)
                chformat = MC_CUBE;
            break;
        default:
            break;
    }
    return chformat;
}

/* NOV 2009 get raw speaker mask */

int psf_speakermask(int sfd)
{
    PSFFILE *sfdat;
    
    if(sfd < 0 || sfd > psf_maxfiles)
        return PSF_E_BADARG;
    
    sfdat  = psf_files[sfd];
    if(sfdat==NULL)
        return PSF_E_BADARG;
    
    return (int) sfdat->fmt.dwChannelMask;
}

/* September 2010 to check output file sizes */
#define PSF_SIZE_SAFETY (512)

int is_legalsize(unsigned long nFrames, const PSF_PROPS *props)
{
    int samplesize = 0;
    int result  = 0;
    int blocksize;
    unsigned long long bytesize;
    
    switch(props->samptype){
        case PSF_SAMP_8:
            samplesize = 1;
            break;
        case PSF_SAMP_16:
            samplesize = 2;
            break;
        case PSF_SAMP_24:
            samplesize = 3;
            break;
        case PSF_SAMP_32:
        case PSF_SAMP_IEEE_FLOAT:
            samplesize = 4;
            break;
        default:
            return result;
    }
    
    blocksize = props->chans * samplesize;
    bytesize = nFrames * blocksize;
    if( bytesize <=  0xffffffffLL - PSF_SIZE_SAFETY)
        result = 1;
    return result;
}

/* TODO: define a psf_writePeak function; probably to a single nominated channel. 
 This would be needed as soon as write is performed with random over-write activity.
 This is probably something to discourage, however!
 */


