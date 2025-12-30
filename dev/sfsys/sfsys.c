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



/* sfsys 64bit (sfsys2010):  handle  files up to 4GB : fpos_t/fread/POS64 version */
/* PC requires FILE64_WIN  defined at compiler level */
/*
 *      portable sfsys replacement
 */
/*RWD.6.5.99 test version for PEAK chunk, etc...
*RWD.7.99 delete created file on error in format, etc
*RWD Jan 2004: fixed WAVE_EX GUID reading on big-endian
*/

/* Nov 2005:  allow any channel count for B-Format files */
/* April 2006: correct bug recoignising .ambi extension in gettypefrom name98() */
/* but we don't really need to preserve this anyway...could just remove */

/*April 2006: revise rdwavhdr to scan all chunks, so can pick up PEAK chunk after data chunk,
   as done by SoundForge! */


/* Oct 2006: changed defs of WORD and DWORD to unsigned, can we support 4GB files???? */
/* RWD 2009: first version of 64bit file handling for 4GB files.*/
/* RWD May 2011: hacked rdwavhdr to accept dastardly Wavelab files with 20byte fmt chunk */
/* PS: and also even more dastardly PT plain wave files with 40byte fmt chunk! */
/* RWD Jan 2013 fixed bug in getsfsysadtl, return correct padded size */
/* RWD Nov 2013 RELEASE 7: added MC_SURR_6_1 */
/* still TODO: replace tmpnam with mkstemp for CDP temporary files for GUI progs (see below)
 * Or: leave it to the GUI progs to sort out! */
/* RWD Feb 2014: converted to ints for x64 building */
/* RWD MAR 2015 finished (?) adoption of default value for CDP_SOUND_EXT, eliminated various compiler warnings */
#ifdef __GNUC__
# if (defined(__LP64__) || defined(_LP64))
# define CDPLONG64
# endif
#endif
#ifdef CDPLONG64
# define CDPLONG int
#else
# define CDPLONG long
#endif


//static char *rcsid = "$Id: sfsys.c%v 4.0 1998/17/02 00:47:05 martin Exp $";

/*pstring for AIFC      - includes the pad byte*/
static const char aifc_floatstring[10] = { 0x08,'F','l','o','a','t',0x20,'3','2',0x00};
static const char aifc_notcompressed[16] = {0x0e,'n','o','t',0x20,'c','o','m','p','r','e','s','s','e','d',0x00};
/*
 *      global state
 */
int _sfverno = 0x0710;                     //RWD: remember to update this!

/* NB: private flag! */
#define SFILE_ANAL  (3)     /* RWD Nov 2009: no PEAK,CLUE chunk for analysis files */

#ifdef DEBUG_MAC
#include <errno.h>
static void parse_errno(int err)
{

    switch(err){
        case(EBADF):
            printf("fd is not valid descriptor\n");
            break;
        case(EINVAL):
            printf("fd is socket, not a file\n");
            printf("or: file not open for writing\n");
            break;
        default:
            printf("unrecognised value for err: %d\n",err);
            break;
    }

}
#endif

/*
 *      $Log: sfsys.c%v $
 * Revision 2.2  1994/12/13  00:47:05  martin
 * Add declaration for Unix
 *
 * Revision 2.1  1994/10/31  15:49:10  martin
 * New version number to differentiate from versions already shipped
 *
 * Revision 1.1  1994/10/31  15:41:38  martin
 * Initial revision
 *
 */
/*  RWD July 1997: Revision 2.3: add support for new WAV float format
 RWD OCT 97: IMPORTANT FIX: DEAL WITH EXTRA 2 BYTES IN NEW WAVEFORMATEX CHUNK
 We MUST READ wave files correctly; it is not wrong, as such, to continue to write them
 the old way, (at least, for straight PCM format), but it is nevertheless inconsistent
 with modern practice.
 also: completed support for 8bit infiles       (read,seek,dirsf)
        added initial support of minimal 'fact' chunk in WAVE files for non-PCM formats
 to use: #define CDP97
 to use Win32 file functions, define CDP99
 */
#include <stdio.h>
#ifdef unix
#include <unistd.h>
# ifdef __MAC__
# include <sys/syslimits.h>
# endif
#endif

#include <stdlib.h>
#include <math.h>
#include <string.h>

#if  /* defined CDP99 && */ defined _WIN32
#include <windows.h>
#endif

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(_WIN32) || defined(__SC__) || defined  (__GNUWIN32__)
#include <io.h>
extern char*   _fullpath (char*, const char*, size_t);
#endif
#include <errno.h>
#include <stdio.h>
#include <time.h>
#ifdef _DEBUG
#include <assert.h>
#endif
#include <sfsys.h>              /*RWD: don't want local copies of this!*/
/*RWD April 2005 */
#include "sffuncs.h"

#include <osbind.h>
#ifdef _WIN32
#include "alias.h"
#endif

int CDP_COM_READY = 0;   /*global flag, ah well...(define in alias.h will access func??)*/


#include <chanmask.h>
/* RWD oct 2022 to eliminate pointer aliasing in AIFF srate handling */
#include "ieee80.h"

//static char *sfsys_h_rcsid = SFSYS_H_RCSID;




/*
 *      The portability definitions come first
 */
#ifdef unix
#define _O_BINARY       (0)
#define _O_RDWR         O_RDWR
#define _O_RDONLY       O_RDONLY
#define _O_CREAT        O_CREAT
#define _O_TRUNC        O_TRUNC
#define _O_EXCL         O_EXCL
#define _S_IWRITE       S_IWRITE
#define _S_IREAD        S_IREAD

#define chsize  ftruncate
#endif



#if !defined(_WIN32) && !defined(__GNUC__)
#define __inline        /**/
#endif

#if defined __MAC__
#define _MAX_PATH (PATH_MAX)
#endif

#if defined MAC || defined __MAC__ || defined linux
int getAliasName(char *filename,char *newpath)
{
    return 1;
}
#endif
#ifndef min
#define min(a,b)        (((a) < (b)) ? (a) : (b) )
#endif

#ifndef WORD
typedef unsigned short WORD;
#endif

#ifndef DWORD
# ifdef CDPLONG64
typedef unsigned int DWORD;
# else
typedef unsigned long DWORD;
# endif
#endif

#ifdef NOTDEF
// moved to sffuncs.h
#define MSBFIRST        (1)
#define LSBFIRST        (1)
/*RWD May 2007:  revise defines to recognise both forms of MAC (__PPC__) */
#if defined(__I86__) || defined(_X86_) || defined(__i386__) || defined(__i486__) || defined(_IBMR2) || defined(__LITTLE_ENDIAN__)
#undef MSBFIRST
#elif defined(M68000) || defined(__sgi) || defined (__ppc__) || defined(__BIG_ENDIAN__)
#undef LSBFIRST
#else
#error  "Unknown byte order for this processor"
#endif

#if defined(MSBFIRST) && defined(LSBFIRST)
#error  "Internal: can't be both MSB and LSB"
#endif

#define REVDWBYTES(t)   ( (((t)&0xff) << 24) | (((t)&0xff00) << 8) | (((t)&0xff0000) >> 8) | (((t)>>24) & 0xff) )
#define REVWBYTES(t)    ( (((t)&0xff) << 8) | (((t)>>8) &0xff) )

/*RWD.6.99 REV3BYTES is a function*/
static char * REV3BYTES(char *samp_24);
extern int sampsize[];

#ifdef MSBFIRST
#define REVDATAINFILE(f)        ((f)->filetype == riffwav || (f)->filetype == wave_ex)
#else
#define REVDATAINFILE(f)        (((f)->filetype == eaaiff) || ((f)->filetype==aiffc))
#endif

#endif
/*
 *      Sfsys-related definitions
 */
#define SFDBASE (1000)
#define ALLOC(s)        ((s *)malloc(sizeof(s)))

#define TAG(a,b,c,d)    ( ((a)<<24) | ((b)<<16) | ((c)<<8) | (d) )

/*
 *      Wave format-specific stuff
 */
#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM         (0x0001)
#endif
#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT  (0x0003)
#endif
#ifndef WAVE_FORMAT_EXTENSIBLE
#define WAVE_FORMAT_EXTENSIBLE (65534)
#endif

#define CURRENT_PEAK_VERSION (1)
#define sizeof_WFMTEX (40)


#ifdef linux
#define POS64(x) (x.__pos)
#else
#define POS64(x) (x)
#endif

typedef union {
    DWORD lsamp;
    float fsamp;
    unsigned char bytes[4];
} SND_SAMP;


struct fmtchunk {
        WORD    formattag;
        WORD    channels;
        DWORD   samplespersec;
        DWORD   avgbytespersec;
        WORD    blockalign;
};

#ifndef _WIN32
typedef struct _GUID
{
    DWORD               Data1;
    unsigned short       Data2;
    unsigned short       Data3;
    unsigned char        Data4[8];
} GUID;

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

typedef struct {
    WAVEFORMATEX    Format;
    union {
        WORD wValidBitsPerSample;       /* bits of precision  */
        WORD wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
        WORD wReserved;                 /* If neither applies, set to */
                                        /* zero. */
    } Samples;
    DWORD           dwChannelMask;      /* which channels are */
                                        /* present in stream  */
    GUID            SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;

const static GUID  KSDATAFORMAT_SUBTYPE_PCM = {0x00000001,0x0000,0x0010,
                                                                                                {0x80,
                                                                                                0x00,
                                                                                                0x00,
                                                                                                0xaa,
                                                                                                0x00,
                                                                                                0x38,
                                                                                                0x9b,
                                                                                                0x71}};

const static GUID  KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {0x00000003,0x0000,0x0010,
                                                                                                {0x80,
                                                                                                0x00,
                                                                                                0x00,
                                                                                                0xaa,
                                                                                                0x00,
                                                                                                0x38,
                                                                                                0x9b,
                                                                                                0x71}};

//B-FORMAT!
// {00000001-0721-11d3-8644-C8C1CA000000}
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
struct cuepoint {
        DWORD   name;
        DWORD   position;
        DWORD   incchunkid;
        DWORD   chunkoffset;
        DWORD   blockstart;
        DWORD   sampleoffset;
};

/*
 *      aiff format-specific stuff
 */

struct aiffchunk {
        DWORD tag;
        DWORD size;
    fpos_t offset;
        char *buf;
        struct aiffchunk *next;
};

/*
 *      Property storage structures
 */
#define PROPCNKSIZE     (2000)

struct property {
        char *name;
        char *data;
        int size;
        struct property *next;
};

/*
 *      Common declarations
 */
enum sndfiletype {
        unknown_wave,
        riffwav,
        eaaiff,
        aiffc,                                   //RWD sfsys98
        wave_ex,                                //RWD.5.99
        cdpfile                                 //RWD sfsys98
};


//typedef struct chpeak {
//      float value;
//      unsigned long position;
//} CHPEAK;

//RWD.6.99 NOTE: because of the slight possibility of 32bit int formats, from both file formats,
// we need an explicit indicator of sample type. We choose to use the fmtchunk.formattag WAVE-style
//for this, even for AIFF formats. Eventually, AIF-C will be used for all float AIFF files


struct sf_file {
        char *filename;
        enum sndfiletype filetype;
        int refcnt;
#if defined CDP99 && defined _WIN32
        HANDLE fileno;
#else
    FILE* fileno;
#endif
        int infochanged;
        int todelete;
        int readonly;
        DWORD mainchunksize;
    fpos_t fmtchunkoffset;
        WAVEFORMATEXTENSIBLE fmtchunkEx;
        int bitmask;
    fpos_t datachunkoffset;
#ifdef FILE64_WIN
        __int64_t datachunksize;
        __int64_t sizerequested;
#else
/* typedef from long long */
    __int64_t datachunksize;
        __int64_t sizerequested;
#endif
        int extrachunksizes;
        struct aiffchunk *aiffchunks;
        int proplim;
    fpos_t propoffset;
        int propschanged;
        int curpropsize;
        struct property *props;
        DWORD curpos;
    fpos_t factchunkoffset;
        int header_set;                 //streaming: disallow header updates
        int is_shortcut;
        //RWD.6.5.99
        time_t peaktime;
    fpos_t peakchunkoffset;
        CHPEAK *peaks;
        channelformat chformat;
        int min_header;
};

/*
 *      for later!!
 *      I think all we have to do is keep an fd open for each file open, and share
 *      the other information - we don't have to worry about being thread-safe, etc!!!
 */
struct sf_openfile {
        struct sf_file *file;
#if defined CDP99 && defined _WIN32
        HANDLE fileno;
#else
    FILE* fileno;
#endif

        DWORD curpos;
};

/*
 *      Values for sizerequested
 */
#define ES_EXIST        (-2)

#define LEAVESPACE      (10*1024)               /* file space that must be left */ //RWD? align to cluster size?



/*
 *      internal state
 */
int rsferrno = 0;
char *rsferrstr = "no previous error";

static struct sf_file *sf_files[SF_MAXFILES];
//static enum sndfiletype gettypefromname(const char *path);              //RWD98: lets declare this here
static enum sndfiletype gettypefromname98(const char *path);
static enum sndfiletype gettypefromfile(struct sf_file *f);
static int wrwavhdr98(struct sf_file *f, int channels, int srate, int stype);
static int wraiffhdr98(struct sf_file *f, int channels, int srate, int stype);
static int wavupdate98(time_t thistime,struct sf_file *f);
static int      aiffupdate(time_t thistime,struct sf_file *f);
static int aiffupdate98(time_t thistime,struct sf_file *f);

//private, but used by snd routines
unsigned int _rsf_getmaxpeak(int sfd,float *peak);
/*RWD 3:2000  to add analysis properties internally */
static int  addprop(struct sf_file *f, char *propname, char *src, int size);

static int compare_guids(const GUID *gleft, const GUID *gright)
{
        const char *left = (const char *) gleft, *right = (const char *) gright;
        return !memcmp(left,right,sizeof(GUID));
}


static int check_guid(struct sf_file *f)
{
//expects a GUID to be loaded already,
        //at present, we only need to know if this is b-format or not.
        //but might as well validate floatsam format against nBits , jic
        if(f->filetype != wave_ex)                      //should be an assert, but this is NOT a console lib!
                return 1;
        f->chformat = MC_STD;
        //we elabotate on possible std assignments later...
        if(compare_guids(&(f->fmtchunkEx.SubFormat),&(KSDATAFORMAT_SUBTYPE_PCM)))
                return 0;

        if(compare_guids(&(f->fmtchunkEx.SubFormat),&(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)))
                if(f->fmtchunkEx.Format.wBitsPerSample == 32)
                        return 0;

        if(compare_guids(&(f->fmtchunkEx.SubFormat),&(SUBTYPE_AMBISONIC_B_FORMAT_PCM))) {
                f->chformat = MC_BFMT;
                return 0;
        }

        if(compare_guids(&(f->fmtchunkEx.SubFormat),&(SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT)))  {
                if(f->fmtchunkEx.Format.wBitsPerSample == 32){
                        f->chformat = MC_BFMT;
                        return 0;
                }
        }

        return 1;
}




#if defined CDP99 && defined _WIN32
#ifdef FILE64_WIN
static int w_ch_size(HANDLE fh,__int64_t size)
#else
static int w_ch_size(HANDLE fh,DWORD size)
#endif
{
#ifdef FILE64_WIN
        LARGE_INTEGER li;
        li.QuadPart =  size;
        li.LowPart = SetFilePointer(fh,li.LowPart,&li.HighPart,FILE_BEGIN);
        if(li.LowPart != 0xFFFFFFFF && GetLastError() == NO_ERROR){
                if (!SetEndOfFile(fh))
                        return -1;
   }
   else
           return -1;
#else
        if(SetFilePointer(fh,(long) size,NULL,FILE_BEGIN)== 0xFFFFFFFF
                || !SetEndOfFile(fh))
                return -1;
#endif
        return 0;
}
#endif

/*
 *      read/write routines
 */

/* RWD 2007 executive decision not to allow cnt >= 2GB! */

#if defined CDP99 && defined _WIN32
static __inline int
doread(struct sf_file *f, char *buf, int cnt)
{
        DWORD done = 0, count = (DWORD) cnt;

        if(f->fileno == INVALID_HANDLE_VALUE)
                return 1;

        if(!ReadFile(f->fileno, buf, count,&done,NULL)){
# ifdef _DEBUG
                LPVOID lpMsgBuf;
                FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                GetLastError(),
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                (LPTSTR) &lpMsgBuf,
                                0,
                                NULL
                );
#  ifndef WINDOWS
                fprintf(stderr,(const char *)lpMsgBuf);
#  else
                MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );
#  endif
                LocalFree( lpMsgBuf );
# endif
                return 1;
        }
        return done != count;
}

#else
//RWD NB returns ERROR if requested byte-count not read
static __inline int
doread(struct sf_file *f, char *buf, int cnt)
{

        int done = fread(buf,sizeof(char),cnt,f->fileno);
        return done != cnt;

}
#endif


static int
read_w_lsf(WORD *wp, struct sf_file *f)
{
        WORD w;
        if(doread(f, (char *)&w, sizeof(WORD)))
                return 1;
#ifdef MSBFIRST
        *wp = REVWBYTES(w);
#else
        *wp = w;
#endif
        return 0;
}

static int
read_w_msf(WORD *wp, struct sf_file *f)
{
        WORD w;
        if(doread(f, (char *)&w, sizeof(WORD)))
                return 1;
#ifdef LSBFIRST
        *wp = REVWBYTES(w);
#else
        *wp = w;
#endif
        return 0;
}

static int
read_dw_lsf(DWORD *dwp, struct sf_file *f)
{
        DWORD dw;
        if(doread(f, (char *)&dw, sizeof(DWORD)))
                return 1;
#ifdef MSBFIRST
        *dwp = REVDWBYTES(dw);
#else
        *dwp = dw;
#endif
        return 0;
}

static int
read_dw_msf(DWORD *dwp, struct sf_file *f)
{
        DWORD dw;
        if(doread(f, (char *)&dw, sizeof(DWORD)))
                return 1;
#ifdef LSBFIRST
        *dwp = REVDWBYTES(dw);
#else
        *dwp = dw;
#endif
        return 0;
}


#if defined CDP99 && defined _WIN32
static __inline int
dowrite(struct sf_file *f, char *buf, int cnt)
{
        DWORD done = 0, count = (DWORD)cnt;
        if(!WriteFile(f->fileno, buf, count,&done,NULL))
                return 1;
        return done != count;

}


#else

static __inline int
dowrite(struct sf_file *f, char *buf, int cnt)
{
        int done = fwrite(buf,sizeof(char),cnt,f->fileno);
        return done != cnt;

}
#endif

static int
write_w_lsf(WORD w, struct sf_file *f)
{
#ifdef MSBFIRST
        w = REVWBYTES(w);
#endif
        return dowrite(f, (char *)&w, sizeof(WORD));
}

static int
write_w_msf(WORD w, struct sf_file *f)
{
#ifdef LSBFIRST
        w = REVWBYTES(w);
#endif
        return dowrite(f, (char *)&w, sizeof(WORD));
}

static int
write_dw_lsf(DWORD dw, struct sf_file *f)
{
#ifdef MSBFIRST
        dw = REVDWBYTES(dw);
#endif
        return dowrite(f, (char *)&dw, sizeof(DWORD));
}

static int
write_dw_msf(DWORD dw, struct sf_file *f)
{
#ifdef LSBFIRST
        dw = REVDWBYTES(dw);
#endif
        return dowrite(f, (char *)&dw, sizeof(DWORD));
}

//RWD.6.5.99 write peak data
/* NB position values are of frames, so int good for 2GB anyway */
static int write_peak_lsf(int channels, struct sf_file *f)
{
        int i;
        DWORD peak[2];
        SND_SAMP ssamp;
    
#ifdef _DEBUG
        if(f->peaks==NULL){
                printf("\nerror: attempt to write uninitialized peak data");
                return 1;
        }
#endif
        for(i=0; i < channels; i++){
                // /*long*/ DWORD  *pdw;
                // pdw = (/*long*/DWORD *) &(f->peaks[i].value);    /* RWD replaced with union */
                ssamp.fsamp = f->peaks[i].value;
                //peak[0] = *pdw;
                peak[0] = ssamp.lsamp;
                peak[1] = f->peaks[i].position;
#ifdef MSBFIRST
                peak[0] = REVDWBYTES(peak[0]);
                peak[1]  = REVDWBYTES(peak[1]);
#endif
                if(dowrite(f,(char *) peak, 2 * sizeof(DWORD)))
                        return 1;

        }
        return 0;

}

static int read_peak_lsf(int channels, struct sf_file *f)
{
        int i;
        DWORD peak[2];
        SND_SAMP ssamp;
#ifdef _DEBUG
        if(f->peaks==NULL){
                printf("\nerror: attempt to write uninitialized peak data");
                return 1;
        }
#endif
        for(i=0;i < channels; i++){
                if(doread(f,(char *)peak,2 * sizeof(DWORD)))
                        return 1;
#ifdef MSBFIRST
                peak[0] = REVDWBYTES(peak[0]);
                peak[1]  = REVDWBYTES(peak[1]);
#endif
                ssamp.lsamp = peak[0];
                //f->peaks[i].value = *(float *) &(peak[0]);          /* RWD TODO: replaced with union */
                f->peaks[i].value = ssamp.fsamp;
                f->peaks[i].position = peak[1];
        }
        return 0;
}

static int write_peak_msf(int channels, struct sf_file *f)
{
        int i;
        DWORD peak[2];
        SND_SAMP ssamp;
        for(i=0; i < channels; i++){
                // /*long*/DWORD  *pdw;
                //pdw = (/*long*/DWORD *) &(f->peaks[i].value);      /* RWD replaced with union */
                ssamp.fsamp = f->peaks[i].value;
                // peak[0] = *pdw;
                peak[0] = ssamp.lsamp;
                peak[1] = f->peaks[i].position;
#ifdef LSBFIRST
                peak[0] = REVDWBYTES(peak[0]);
                peak[1]  = REVDWBYTES(peak[1]);
#endif
                if(dowrite(f,(char *) peak, 2 * sizeof(DWORD)))
                        return 1;

        }
        return 0;

}

static int read_peak_msf(int channels, struct sf_file *f)
{
        int i;
        DWORD peak[2];
        SND_SAMP ssamp;
        for(i=0;i < channels; i++){
                if(doread(f,(char *)peak,2 * sizeof(DWORD)))
                        return 1;
#ifdef LSBFIRST
                peak[0] = REVDWBYTES(peak[0]);
                peak[1]  = REVDWBYTES(peak[1]);
#endif
                ssamp.lsamp = peak[0];
                //f->peaks[i].value = *(float *) &(peak[0]);        /* RWD replaced with union */
                f->peaks[i].value = ssamp.fsamp;
                f->peaks[i].position = peak[1];
        }
        return 0;
}
/*
 *      Fudge Apple extended format, for sample rates
 */
/* RWD Oct 2022 removed old "TABLE_IEEE754" code */
/* now using compact Csound code */



#ifdef DW_EX_OLDCODE
static int
read_ex_todw(DWORD *dwp, struct sf_file *f)
{
        double neg = 1.0;
        WORD exp;
        /*unsigned long*/DWORD ms_sig;
        double res;
        char buf[10];   /* for 80-bit extended float */

        if(doread(f, buf, 10))
                return 1;
# ifdef LSBFIRST
        exp = REVWBYTES(*(WORD *)&buf[0]);
        ms_sig = (/*unsigned long*/DWORD)REVDWBYTES(*(DWORD *)&buf[2]);   /* RWD TODO replace with union */
# else
        exp = *(WORD *)&buf[0];
        ms_sig = (/*unsigned long*/DWORD)*(DWORD *)&buf[2];               /* RWD TODO replace with union */
# endif
        if(exp & 0x8000) {
                exp &= ~0x8000;
                neg = -1.0;
        }
        exp -= 16382;
        res = (double)ms_sig/TWOPOW32;
        res = neg*ldexp(res, exp);
        *dwp = (DWORD)(res+0.5);
        return 0;
}

static int
write_dw_toex(DWORD dw, struct sf_file *f)
{
    double val = (double)dw;
    int neg = 0;
    /*unsigned long*/DWORD mant;
    int exp;
    char buf[10];
    
    if(val < 0.0) {
        val = -val;
        neg++;
    }
    mant = (/*unsigned long*/DWORD)(frexp(val, &exp) * TWOPOW32 + 0.5);
    exp += 16382;
    if(neg)
        exp |= 0x8000;
# ifdef LSBFIRST
    *(WORD *)&buf[0] = REVWBYTES(exp);              /* RWD TODO replace all with union? */
    *(DWORD *)&buf[2] = REVDWBYTES((DWORD)mant);
# else
    *(WORD *)&buf[0] = exp;
    *(DWORD *)&buf[2] = mant;
# endif
    *(DWORD *)&buf[6] = 0;
    return dowrite(f, buf, 10);
}
#else
/* use Csound funcs */
static int
read_ex_todw(DWORD *dwp, struct sf_file *f)
{
    double Csound_res = 0.0;
    char buf[10];   /* for 80-bit extended float */

    if(doread(f, buf, 10))
        return 1;
    Csound_res = ieee_80_to_double((unsigned char *) buf);
    *dwp = (DWORD) Csound_res;
    return 0;
}

static int
write_dw_toex(DWORD dw, struct sf_file *f)
{
    double val = (double)dw;
    char buf[10];
    double_to_ieee_80(val,(unsigned char *) buf);
    return dowrite(f,buf,10);
}


#endif  // DW_EX_OLDCODE


/*
 *      wave file-format specific routines
 */

static int
getsfsyscue(struct sf_file *f)
{
        DWORD cnt;
        int rc = 0;
        struct cuepoint cue;

        if(read_dw_lsf(&cnt, f))
                return -1;
        while(cnt-- > 0) {
                if(read_dw_msf(&cue.name, f)
                 ||read_dw_lsf(&cue.position, f)
                 ||read_dw_msf(&cue.incchunkid, f)
                 ||read_dw_lsf(&cue.chunkoffset, f)
                 ||read_dw_lsf(&cue.blockstart, f)
                 ||read_dw_lsf(&cue.sampleoffset, f))
                        return -1;
                if(cue.name == TAG('s','f','i','f'))
                        rc = 1;
        }
        return rc;
}

/*
 *      extended properties are as follows:
 *
 *      property name '\n'
 *      property value '\n'
 *      ...
 *      '\n'
 */
static int
xtoi(int ch)
{
        if(ch >= '0' && ch <= '9')
                return ch - '0';
        if(ch >= 'A' && ch <= 'F')
                return ch - 'A' + 10;
        if(ch >= 'a' && ch <= 'f')
                return ch - 'a' + 10;
        return 0;
}

static int
itox(int i)
{
        static char trans[] = "0123456789ABCDEF";
        return trans[i&0x0f];
}

static void
parseprops(struct sf_file *f, char *data)
{
        char *cp = data;
        char *ep, *evp;
        int cnt;
        struct property **ppp = &f->props;
        struct property *np;

        f->curpropsize = 0;

        while(cp-data < f->proplim && *cp != '\n') {
                if((ep = strchr(cp, '\n')) == 0
                 ||(evp = strchr(ep+1, '\n')) == 0
                 ||((evp-ep-1)&1))
                        return;
                if((np = ALLOC(struct property)) == 0
                 ||(np->name = (char *) malloc(ep-cp+1)) == 0
                 ||(np->data = (char *) malloc((evp-ep-1)/2)) == 0)
                        return;
                np->size = (evp-ep-1)/2;
                np->next = 0;
                memcpy(np->name, cp, ep-cp);
                np->name[ep-cp] = '\0';
                for(cnt = 0; cnt < np->size; cnt++)
                        np->data[cnt] = (xtoi((ep+1)[2*cnt])<<4) + xtoi((ep+1)[2*cnt+1]);
                *ppp = np;
                ppp = &np->next;
                f->curpropsize += strlen(np->name) + 1 + np->size + 1;

                cp = evp+1;
        }
}

static int
writeprops(struct sf_file *f)
{
        char *obuf, *op;
        int cnt;
        struct property *p = f->props;

        if((obuf = (char *) malloc(f->proplim)) == 0) {
                rsferrno = ESFNOMEM;
                rsferrstr = "No memory to write properties with";
                return -1;
        }
        op = obuf;

        while(p != 0) {
                strcpy(op, p->name);
                op += strlen(p->name);
                *op++ = '\n';
                for(cnt = 0; cnt < p->size; cnt++) {
                        *op++ = itox(p->data[cnt]>>4);
                        *op++ = itox(p->data[cnt]);
                }
                *op++ = '\n';
                if(op-obuf >= f->proplim)                 //RWD.1.99 this is really an assert test...
                        abort();
                p = p->next;
        }
        while(op < &obuf[f->proplim])
                *op++ = '\n';
        /*RWD 2007: we rely on all props being within first 2GB of file! */
#if defined CDP99 && defined _WIN32
        if (SetFilePointer(f->fileno,(LONG)f->propoffset,NULL,FILE_BEGIN) == 0xFFFFFFFF
#else

        if(fseeko(f->fileno, POS64(f->propoffset), SEEK_SET)
#endif
         ||dowrite(f, obuf, f->proplim)) {
                rsferrno = ESFWRERR;
                rsferrstr = "Write error writing new property values";
                return -1;
        }
        free(obuf);
        return 0;
}

static int
getsfsysadtl(struct sf_file *f, int adtllen)
{
        DWORD tag, size;
        DWORD name;
    fpos_t bytepos;
        char *propspace;
        char buf[1];

        while(adtllen > 0) {
                if(read_dw_msf(&tag, f)
                 ||read_dw_lsf(&size, f))
                        return -1;
                switch(tag) {
                case TAG('n','o','t','e'):
                        if(read_dw_msf(&name, f))
                                return -1;
                        if(name != TAG('s','f','i','f')
                         ||(int)(POS64(f->propoffset)) >= 0
                         ||(propspace = (char *) malloc(size-sizeof(DWORD))) == 0) {

#if defined CDP99 && defined _WIN32
                                if(SetFilePointer(f->fileno,(LONG)((size-sizeof(DWORD)+1)&~1),NULL,FILE_CURRENT)== 0xFFFFFFFF)
#else
                                if(fseek(f->fileno, (size-sizeof(DWORD)+1)&~1, SEEK_CUR))
#endif
                                        return -1;
                                break;
                        }
                        f->proplim = size-sizeof(DWORD);
#if defined CDP99 && defined _WIN32
            if((f->propoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF
              ||doread(f, propspace, size-sizeof(DWORD)))
                return -1;
#else
                        if(fgetpos(f->fileno,&bytepos)
              ||doread(f, propspace, size-sizeof(DWORD)))
                                return -1;
            f->propoffset = bytepos;
#endif
                        parseprops(f, propspace);
                        free(propspace);
                        if(size&1)
                                doread(f, buf, 1);
                        break;
                default:
#if defined CDP99 && defined _WIN32
                        if(SetFilePointer(f->fileno, (size+1)&~1,NULL,FILE_CURRENT) == 0xFFFFFFFF)
                return -1;
#else
                        if(fseek(f->fileno, (size+1)&~1, SEEK_CUR) < 0)
                                return -1;
#endif
                        break;
                }
                adtllen -= 2*sizeof(DWORD) + ((size+1)&~1);     /* RWD Jan 2013 */
        }
        return 0;
}

//RWD.7.99 TODO: set f->min_header here?

/*RWD 2007: MUST use DWORD to enure we get sizes up to 4GB */
/* BUT: if we seek to end of data chunk, need 64bit seek */
static int
rdwavhdr(struct sf_file *f)
{
        DWORD tag, size;
        int fmtseen = 0;
        int dataseen  = 0;      /*RWD April 2006: try to read PEAK chunk after data (boo hiss Sony! )*/
        int err = 0;            /*  "" */
        int gotsfsyscue = 0;
        DWORD factsize = 0;
        DWORD peak_version;
#ifdef FILE64_WIN
        LARGE_INTEGER pos64;   /* pos64.QuadPart is __int64_t */
#endif
    fpos_t bytepos;

        //WAVEFORMATEXTENSIBLE *pFmtEx;
        if(read_dw_msf(&tag, f)
         ||read_dw_lsf(&size, f)
         ||tag != TAG('R','I','F','F')) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "File is not a RIFF file";
                return 1;
        }
        if(size < 4) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "File data size is too small";
                return 1;
        }
        if(read_dw_msf(&tag, f)
         ||tag != TAG('W','A','V','E')) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "File is not a wave RIFF file";
                return 1;
        }
        f->filetype = riffwav;          //might be wave_ex
        f->mainchunksize = size;
        f->extrachunksizes = 0;
        f->proplim = 0;
        f->props = 0;
        POS64(f->propoffset) = (unsigned) -1;
        POS64(f->factchunkoffset) = (unsigned) -1;
        //datachunkoffset now initialized in allocsffile

        //RWD.6.99 do I need to do the AIFF getout for bad sizes here too?

        for(;;) {
                if(read_dw_msf(&tag, f)
                        ||read_dw_lsf(&size,f)){
                        /*RWD April 2006 TODO: detect EOF! */
                        err++;
                        goto ioerror;
                }
                switch(tag) {
                case TAG('f','m','t',' '):
                        //RWD: must deal with possibility of WAVEFORMATEX extra cbSize word
#if defined CDP99 && defined _WIN32
                        if((f->fmtchunkoffset = SetFilePointer(f->fileno, 0L,NULL, FILE_CURRENT))== 0xFFFFFFFF
              ||read_w_lsf(&f->fmtchunkEx.Format.wFormatTag, f)
              ||read_w_lsf(&f->fmtchunkEx.Format.nChannels, f)
              ||read_dw_lsf(&f->fmtchunkEx.Format.nSamplesPerSec, f)
              ||read_dw_lsf(&f->fmtchunkEx.Format.nAvgBytesPerSec, f)
              ||read_w_lsf(&f->fmtchunkEx.Format.nBlockAlign, f) ) {
                                err++;
                                goto ioerror;
            }
#else
                        if(fgetpos(f->fileno, &bytepos)
                         ||read_w_lsf(&f->fmtchunkEx.Format.wFormatTag, f)
                         ||read_w_lsf(&f->fmtchunkEx.Format.nChannels, f)
                         ||read_dw_lsf(&f->fmtchunkEx.Format.nSamplesPerSec, f)
                         ||read_dw_lsf(&f->fmtchunkEx.Format.nAvgBytesPerSec, f)
                         ||read_w_lsf(&f->fmtchunkEx.Format.nBlockAlign, f) ) {
                                err++;
                                goto ioerror;
            }
            f->fmtchunkoffset = bytepos;
#endif

                        switch(f->fmtchunkEx.Format.wFormatTag) {
                        case WAVE_FORMAT_PCM:
                        case WAVE_FORMAT_IEEE_FLOAT:                    //RWD 07:97
                        case WAVE_FORMAT_EXTENSIBLE:                    //RWD.5.99
                                if(read_w_lsf(&f->fmtchunkEx.Format.wBitsPerSample, f)) {
                                        err++;
                                        goto ioerror;
                                }
                                //RWD.6.99 set f->fmtchunkEx.Samples.wValidBitsPerSample to this as default
                                // will only be changed by a WAVE_EX header
                                f->fmtchunkEx.Samples.wValidBitsPerSample = f->fmtchunkEx.Format.wBitsPerSample;
                                //deal with things such as 20bits per sample...
                                //this covers standard WAVE, WAVE-EX may adjust again
                                if(f->fmtchunkEx.Format.wBitsPerSample !=
                                           ((f->fmtchunkEx.Format.nBlockAlign / f->fmtchunkEx.Format.nChannels) * 8)){
                                        //deduce the container size
                                        f->fmtchunkEx.Format.wBitsPerSample =
                                           ((f->fmtchunkEx.Format.nBlockAlign / f->fmtchunkEx.Format.nChannels) * 8);

                                }


                                switch(f->fmtchunkEx.Format.wBitsPerSample) {
                                case 32:                /* floats or longs*/
                                        break;
                                case(24):
                                case 16:                /* shorts */
                                case 8:                 /* byte -> short mappping */
/*RWD 07:97*/           if(!(f->fmtchunkEx.Format.wFormatTag == WAVE_FORMAT_PCM  ||
                                                        f->fmtchunkEx.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)){
                                                rsferrno = ESFNOTFOUND;
                                                rsferrstr = "can't open Sfile : Format mismatch";
                                                return 1;
                                        }
                                        break;
                                default:
                                        rsferrno = ESFNOTFOUND;
                                        rsferrstr = "can't open Sfile - unsupported format";
                                        return 1;
                                }
                                //now catch any extra bytes, and parse WAVE_EX if we have it...
                                if(size > 16){
                                        unsigned short cbSize;
                                        if(read_w_lsf(&cbSize,f))
                                                goto ioerror;
                    /* RWD May 2011. Effing Steinberg Wavelab writes a 20byte fmt chunk! */
                    /* Feb 2012: AND Pro Tools writes a 40byte fmt chunk! */
                    if(cbSize==0){
                        int wordstoskip = (size-18) / sizeof(WORD);
                        int skip;
                        unsigned short dummy;
                        for(skip = 0; skip < wordstoskip; skip++){
                            if(read_w_lsf(&dummy,f))
                                goto ioerror;
                        }
                    }
                                        else{
                                                if(f->fmtchunkEx.Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE){
                                                        rsferrno = ESFNOTFOUND;
                                                        rsferrstr = "unexpected extra format information - not a wave file";
                                                        return 1;
                                                }
                                                if(cbSize != 22){
                                                        rsferrno = ESFNOTFOUND;
                                                        rsferrstr = "unexpected extra format information in WAVE_EX file!";
                                                        return 1;
                                                }
                                                f->filetype = wave_ex;
                                                if(read_w_lsf(&(f->fmtchunkEx.Samples.wValidBitsPerSample),f)
                                                        ||
                                                        read_dw_lsf(&(f->fmtchunkEx.dwChannelMask),f)){
                                                        err++;
                                                        goto ioerror;
                                                }
                                                //get the GUID
                                                /*RWD TODO: need byte-reverse code in hers! */
                                                if(doread(f,(char *) &(f->fmtchunkEx.SubFormat),sizeof(GUID))) {
                                                        err++;
                                                        goto ioerror;
                                                }
#ifdef MSBFIRST
                                                f->fmtchunkEx.SubFormat.Data1 = REVDWBYTES(f->fmtchunkEx.SubFormat.Data1);
                                                f->fmtchunkEx.SubFormat.Data2 = REVWBYTES(f->fmtchunkEx.SubFormat.Data2);
                                                f->fmtchunkEx.SubFormat.Data3 = REVWBYTES(f->fmtchunkEx.SubFormat.Data3);
#endif
                                                if(check_guid(f)){
                                                        rsferrno = ESFNOTFOUND;
                                                        rsferrstr = "unrecognized WAV_EX subformat";
                                                        return 1;
                                                }


                                                //we don't try to ID the spkr-config here: do that in the sf/snd open fucntions
                                        }
                                }
                                break;
                        default:
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "can't open Sfile - unsupported format";
                                return 1;
                        }
                        //set the bitmask here - covers plain WAVE too
                        if(f->fmtchunkEx.Samples.wValidBitsPerSample
                                < f->fmtchunkEx.Format.wBitsPerSample){
                                /*long*/ int mask = 0xffffffff;
                                /*long*/ int shift = 32 - f->fmtchunkEx.Format.wBitsPerSample;
                                f->bitmask = mask << (shift + (f->fmtchunkEx.Format.wBitsPerSample
                                        - f->fmtchunkEx.Samples.wValidBitsPerSample));
                        }

                        fmtseen++;
                        break;
                case TAG('L','I','S','T'):
                        if(read_dw_msf(&tag, f))
                                goto ioerror;
                        switch(tag) {
                        case TAG('w','a','v','l'):
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "Can't open SFfile - no support for list chunks yet!";
                                return 1;
                        case TAG('a','d','t','l'):
                                if(getsfsysadtl(f, size-sizeof(DWORD)) < 0)     {
                                        err++;
                                        goto ioerror;
                                }
                                break;
                        default:
#if defined CDP99 && defined _WIN32
                                if(SetFilePointer(f->fileno, size-sizeof(DWORD),NULL, FILE_CURRENT) == 0xFFFFFFFF)
#else
                POS64(bytepos) = size-sizeof(DWORD);
                                if(fseeko(f->fileno, /*size-sizeof(DWORD)*/POS64(bytepos), SEEK_CUR) < 0)
#endif
                                {
                                        err++;
                                        goto ioerror;
                                }
                                break;
                        }
                        break;

//read fact chunk (not needed for std PCM files...but we use it anyway in sfsys97!
                case TAG('f','a','c','t'):
#if defined CDP99 && defined _WIN32
                        if((f->factchunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
#else
                        if(fgetpos(f->fileno,&f->factchunkoffset))
#endif
                        {
                                err++;
                                goto ioerror;
                        }
                        if(read_dw_lsf(&factsize,f))  {
                                err++;
                                goto ioerror;
                        }
                        break;                                                  //RWD: need to update extrachunksizes?


                        //RWD.5.99 read PEAK chunk
                case TAG('P','E','A','K'):
                        f->peaks = (CHPEAK *) calloc(f->fmtchunkEx.Format.nChannels,sizeof(CHPEAK));
                        if(f->peaks == NULL){
                                rsferrno = ESFNOMEM;
                                rsferrstr = "No memory for peak data";
                                return 1;
                        }
                        if(read_dw_lsf(&peak_version,f)) {
                                err++;
                                goto ioerror;
                        }
                        switch(peak_version){
                        case(CURRENT_PEAK_VERSION):
                                if(read_dw_lsf((DWORD*) &(f->peaktime),f)){
                                        err++;
                                        goto ioerror;
                                }
/* RWD 2007:  PEAK chunk is after data chunk in some naff but otherwise legal files,
  so ordinary lseek no good */

#if defined CDP99 && defined _WIN32
# ifdef FILE64_WIN
                                pos64.QuadPart = 0;
                                pos64.LowPart = SetFilePointer(f->fileno, 0L, &pos64.HighPart, FILE_CURRENT);
                                if(pos64.LowPart == 0xFFFFFFFF  && GetLastError() != NO_ERROR){
                                        err++;
                                        goto ioerror;
                                }
                                else {
                                        /* WAVE and AIFF have to fit inside unsigned int */
                                        f->peakchunkoffset = (unsigned int) pos64.QuadPart;
                                        if(read_peak_lsf(f->fmtchunkEx.Format.nChannels,f))     {
                                                err++;
                                                goto ioerror;
                                        }
                                }
# else
                                if((f->peakchunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF
                                        || read_peak_lsf(f->fmtchunkEx.Format.nChannels,f))     {
                                        err++;
                                        goto ioerror;
                                }

# endif

#else
                                if(fgetpos(f->fileno,&f->peakchunkoffset)
                                        || read_peak_lsf(f->fmtchunkEx.Format.nChannels,f))     {
                                        err++;
                                        goto ioerror;
                                }
#endif
                                break;
                        default:
#ifdef _DEBUG
                                fprintf(stderr,"\nunknown PEAK version!");
#endif
                                free(f->peaks);
                                f->peaks = NULL;
                                break;
                        }
                        break;

                case TAG('d','a','t','a'):
#if defined CDP99 && defined _WIN32
/*  datachunk MUST be within 2GB! */

                        if((f->datachunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
            {
                                err++;
                                goto ioerror;
                        }
            f->datachunksize = size;
            if(f->fmtchunkEx.Format.wBitsPerSample == 8)
                f->datachunksize *= 2;
#else
                        if(fgetpos(f->fileno,&f->datachunkoffset) )
                        {
                                err++;
                                goto ioerror;
                        }
                        f->datachunksize = size;
                        if(f->fmtchunkEx.Format.wBitsPerSample == 8)
                                f->datachunksize *= 2;
#endif
                        /* skip over data chunk; to try to read later chunks */

                        size = (size+1)&~1;
#if defined CDP99 && defined _WIN32
# ifdef FILE64_WIN
                        pos64.QuadPart = (__int64) size;
                        pos64.LowPart = SetFilePointer(f->fileno, pos64.LowPart,&pos64.HighPart,FILE_CURRENT);
                        if(pos64.LowPart == 0xFFFFFFFF && GetLastError() != NO_ERROR){
                                err++;
                                goto ioerror;
                        }

# else
                        if(SetFilePointer(f->fileno, size, NULL,FILE_CURRENT) == 0xFFFFFFFF)
                        {
                                err++;
                                goto ioerror;
                        }
# endif
#else
            /* expect >2GB data chunk, so must use fseeko etc */
            POS64(bytepos) = POS64(f->datachunkoffset) + size;
            if(fsetpos(f->fileno, &bytepos))
            {
                err++;
                goto ioerror;
            }

#endif
                        dataseen++;
                        break;
                case TAG('c','u','e',' '):
                        if((gotsfsyscue = getsfsyscue(f)) < 0) {
                                err++;
                                goto ioerror;
                        }
                        break;
                default:
                        size = (size+1)&~1;
                        /* we trust that only the data chunk will be huge! */
#if defined CDP99 && defined _WIN32
                        if(SetFilePointer(f->fileno, size, NULL,FILE_CURRENT) == 0xFFFFFFFF)
                        {
                                err++;
                                goto ioerror;
                        }
#else
                        if(fseek(f->fileno, size, SEEK_CUR) < 0)
                        {
                                err++;
                                goto ioerror;
                        }
#endif
                        f->extrachunksizes += size + 2*sizeof(DWORD);      //RWD: anonymous chunks - we will copy these one day...
                        break;
                }
        }
        /* NOTREACHED */

ioerror:
        if(fmtseen && dataseen)
                return 0;

        rsferrno = ESFRDERR;
        rsferrstr = "read error (or file too short) reading wav header";
        return 1;

}

#if 0
static int
wrwavhdr(struct sf_file *f)
{
        struct cuepoint cue;
        int extra = 0;          //RWD CDP97
#ifdef linux
        fpos_t bytepos = {0};
#else
    fpos_t bytepos = 0;
#endif
    WORD cbSize = 0x0000;    //RWD for WAVEFORMATEX
        f->mainchunksize = 0;   /* we don't know the full size yet! */
        f->extrachunksizes = 0;
        extra =  sizeof(WORD);

        if(write_dw_msf(TAG('R','I','F','F'), f)
         ||write_dw_lsf(0, f)
         ||write_dw_msf(TAG('W','A','V','E'), f))
                goto ioerror;

        f->fmtchunkEx.Format.wFormatTag = WAVE_FORMAT_PCM;
        f->fmtchunkEx.Format.nChannels = 1;
        f->fmtchunkEx.Format.nSamplesPerSec = 44100;
        f->fmtchunkEx.Format.nAvgBytesPerSec = 4*44100;
        f->fmtchunkEx.Format.nBlockAlign = 2;
        f->fmtchunkEx.Format.wBitsPerSample = 16;

        if(write_dw_msf(TAG('f','m','t',' '), f)
      ||write_dw_lsf(16 + extra, f)                             //RWD CDP97: size = 18 to include cbSize field
#if defined CDP99 && defined _WIN32
          ||((f->fmtchunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
      ||write_w_lsf(f->fmtchunkEx.Format.wFormatTag, f)
      ||write_w_lsf(f->fmtchunkEx.Format.nChannels, f)
      ||write_dw_lsf(f->fmtchunkEx.Format.nSamplesPerSec, f)
      ||write_dw_lsf(f->fmtchunkEx.Format.nAvgBytesPerSec, f)
      ||write_w_lsf(f->fmtchunkEx.Format.nBlockAlign, f)
      ||write_w_lsf(f->fmtchunkEx.Format.wBitsPerSample, f)
      ||write_w_lsf(cbSize,f)                                                   //RWD , sorry, HAVE to allow for this!
      )
                  goto ioerror;
#else
      ||(fgetpos(f->fileno, &bytepos))
      ||write_w_lsf(f->fmtchunkEx.Format.wFormatTag, f)
          ||write_w_lsf(f->fmtchunkEx.Format.nChannels, f)
          ||write_dw_lsf(f->fmtchunkEx.Format.nSamplesPerSec, f)
          ||write_dw_lsf(f->fmtchunkEx.Format.nAvgBytesPerSec, f)
          ||write_w_lsf(f->fmtchunkEx.Format.nBlockAlign, f)
          ||write_w_lsf(f->fmtchunkEx.Format.wBitsPerSample, f)
          ||write_w_lsf(cbSize,f)                                                       //RWD , sorry, HAVE to allow for this!
     )
        goto ioerror;

    f->fmtchunkoffset = bytepos;
#endif
        /* don't need to put a fact chunk */

        /*
         *      add the cue point/note chunk for properties
         */
//RWD TODO: add switch to skip writing this extra stuff!
        if(write_dw_msf(TAG('c','u','e',' '), f)
         ||write_dw_lsf(sizeof(struct cuepoint) + sizeof(DWORD), f) )
                goto ioerror;

        cue.name = TAG('s','f','i','f');
        cue.position = 0;
        cue.incchunkid = TAG('d','a','t','a');
        cue.chunkoffset = 0;
        cue.blockstart = 0;
        cue.sampleoffset = 0;
        if(write_dw_lsf(1, f)   /* one cue point */
         ||write_dw_msf(cue.name, f)
         ||write_dw_lsf(cue.position, f)
         ||write_dw_msf(cue.incchunkid, f)
         ||write_dw_lsf(cue.chunkoffset, f)
         ||write_dw_lsf(cue.blockstart, f)
         ||write_dw_lsf(cue.sampleoffset, f) )
                goto ioerror;

        /*... add a LIST chunk of type 'adtl'... */

        if(write_dw_msf(TAG('L','I','S','T'), f)
         ||write_dw_lsf(sizeof(DWORD) + 3*sizeof(DWORD) + PROPCNKSIZE, f)
         ||write_dw_msf(TAG('a','d','t','l'), f) )
                goto ioerror;

        /*      add the property-space note chunk  */
        if(write_dw_msf(TAG('n','o','t','e'), f)
         ||write_dw_lsf(sizeof(DWORD) + PROPCNKSIZE, f)
         ||write_dw_msf(TAG('s','f','i','f'), f)
#if defined CDP99 && defined _WIN32
         ||((f->propoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
         ||SetFilePointer(f->fileno, (long)PROPCNKSIZE,NULL, FILE_CURRENT) == 0xFFFFFFFF )
        goto ioerror;
#else
         ||fgetpos(f->fileno, &bytepos)
         ||fseek(f->fileno, PROPCNKSIZE, SEEK_CUR) )
                goto ioerror;
    f->propoffset = bytepos;
#endif
        f->propschanged = 1;
        f->proplim = PROPCNKSIZE;

        /*
         *      and add the data chunk
         */
        f->datachunksize = 0;

        if( write_dw_msf(TAG('d','a','t','a'), f)
         ||write_dw_lsf(0, f)
#if defined CDP99 && defined _WIN32
          ||((f->datachunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF))
        goto ioerror;
#else
          ||(fgetpos(f->fileno, &bytepos)) )
                goto ioerror;
# ifndef linux
    f->datachunkoffset = (__int64) POS64(bytepos);
# else
        POS64(f->datachunkoffset) = POS64(bytepos);
# endif
#endif
        return 0;
        /* NOTREACHED */

ioerror:
        rsferrno = ESFWRERR;
        rsferrstr = "write error writing wav header";
        return 1;
}
#endif

/********** SFSYS98 version ****************
 *  this RECEIVES format data from calling function,
 * to create the required header
 */
static int
wrwavhdr98(struct sf_file *f, int channels, int srate, int stype)
{
        struct cuepoint cue;
        int extra = 0;
        int wordsize;
#ifdef linux
        fpos_t bytepos = {0};
#else
    fpos_t bytepos = 0;
#endif
    WORD cbSize = 0x0000;    //RWD for WAVEFORMATEX, FLOAT FORMAT ONLY
        f->mainchunksize = 0;   /* we don't know the full size yet! */
        f->extrachunksizes = 0;

        //we will not use sffuncs for 24bit files!
        if(stype >= SAMP_MASKED){
                rsferrstr = "this verson cannot write files in requested format";
                return 1;
        }

        //wordsize = (stype == SAMP_FLOAT ?  sizeof(float) : sizeof(short));
        wordsize = sampsize[stype];
        if(stype== SAMP_FLOAT)
                extra =  sizeof(WORD);

        if(write_dw_msf(TAG('R','I','F','F'), f)
         ||write_dw_lsf(0, f)
         ||write_dw_msf(TAG('W','A','V','E'), f))
                goto ioerror;

        f->fmtchunkEx.Format.wFormatTag = stype == SAMP_FLOAT ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
        f->fmtchunkEx.Format.nChannels = (unsigned short)channels;
        f->fmtchunkEx.Format.nSamplesPerSec = srate;
        f->fmtchunkEx.Format.nAvgBytesPerSec = wordsize * channels * srate;
        f->fmtchunkEx.Format.nBlockAlign = (unsigned short) ( wordsize * channels);

        //for standard WAVE, we set wBitsPerSample to = validbits
        if(stype==SAMP_2024)
                f->fmtchunkEx.Format.wBitsPerSample = 20;
        else if(stype==SAMP_2432)
                f->fmtchunkEx.Format.wBitsPerSample = 24;
        else
                f->fmtchunkEx.Format.wBitsPerSample = (short)(8 * wordsize);
        //need this for wavupdate98()
    f->fmtchunkEx.Samples.wValidBitsPerSample = f->fmtchunkEx.Format.wBitsPerSample;
        if(write_dw_msf(TAG('f','m','t',' '), f)
         ||write_dw_lsf(16 + extra, f)                          //RWD CDP97: size = 18 to include cbSize field
#if defined CDP99 && defined _WIN32
         ||((f->fmtchunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
     ||write_w_lsf(f->fmtchunkEx.Format.wFormatTag, f)
     ||write_w_lsf(f->fmtchunkEx.Format.nChannels, f)
     ||write_dw_lsf(f->fmtchunkEx.Format.nSamplesPerSec, f)
     ||write_dw_lsf(f->fmtchunkEx.Format.nAvgBytesPerSec, f)
     ||write_w_lsf(f->fmtchunkEx.Format.nBlockAlign, f)
     ||write_w_lsf(f->fmtchunkEx.Format.wBitsPerSample, f)
     )
                goto ioerror;
#else
         ||fgetpos(f->fileno,&bytepos)
         ||write_w_lsf(f->fmtchunkEx.Format.wFormatTag, f)
         ||write_w_lsf(f->fmtchunkEx.Format.nChannels, f)
         ||write_dw_lsf(f->fmtchunkEx.Format.nSamplesPerSec, f)
         ||write_dw_lsf(f->fmtchunkEx.Format.nAvgBytesPerSec, f)
         ||write_w_lsf(f->fmtchunkEx.Format.nBlockAlign, f)
         ||write_w_lsf(f->fmtchunkEx.Format.wBitsPerSample, f)
        )
                goto ioerror;
    f->fmtchunkoffset = bytepos;
#endif
        if(stype == SAMP_FLOAT) {
                if(write_w_lsf(cbSize,f))
                        goto ioerror;
        }

        //RWD.6.5.99 ADD the PEAK chunk
        if((f->min_header >= SFILE_PEAKONLY) && f->peaks){
                int i,size;
                DWORD now = 0;
                for(i=0;i < channels; i++){
                        f->peaks[i].value = 0.0f;
                        f->peaks[i].position = 0;
                }
                size = 2 * sizeof(DWORD) + channels * sizeof(CHPEAK);
                if(write_dw_msf(TAG('P','E','A','K'),f)
          || write_dw_lsf(size,f)
#if defined CDP99 && defined _WIN32
                  || ((f->peakchunkoffset = SetFilePointer(f->fileno,0L,NULL, FILE_CURRENT))== 0xFFFFFFFF)
          || write_dw_lsf(CURRENT_PEAK_VERSION,f)
          || write_dw_lsf(now,f)
          || write_peak_lsf(channels,f))
                        goto ioerror;
#else
                        || fgetpos(f->fileno,&bytepos)
                        || write_dw_lsf(CURRENT_PEAK_VERSION,f)
                        || write_dw_lsf(now,f)
                        || write_peak_lsf(channels,f))

                        goto ioerror;

        f->peakchunkoffset = bytepos;
#endif
        }


        if(f->min_header >= SFILE_CDP){
        /*
         *      add the cue point/note chunk for properties
         */
        /* RWD Nov 2009: don't need cue for analysis files */
        if(f->min_header==SFILE_CDP){
            //RWD TODO: add switch to skip writing this extra stuff!
            if(write_dw_msf(TAG('c','u','e',' '), f)
                ||write_dw_lsf(sizeof(struct cuepoint) + sizeof(DWORD), f) )
                goto ioerror;

            cue.name = TAG('s','f','i','f');
            cue.position = 0;
            cue.incchunkid = TAG('d','a','t','a');
            cue.chunkoffset = 0;
            cue.blockstart = 0;
            cue.sampleoffset = 0;
            if(write_dw_lsf(1, f)       /* one cue point */
               ||write_dw_msf(cue.name, f)
               ||write_dw_lsf(cue.position, f)
               ||write_dw_msf(cue.incchunkid, f)
               ||write_dw_lsf(cue.chunkoffset, f)
               ||write_dw_lsf(cue.blockstart, f)
               ||write_dw_lsf(cue.sampleoffset, f) )
                goto ioerror;
        }
                /*... add a LIST chunk of type 'adtl'... */
                if(write_dw_msf(TAG('L','I','S','T'), f)
                ||write_dw_lsf(sizeof(DWORD) + 3*sizeof(DWORD) + PROPCNKSIZE, f)
                ||write_dw_msf(TAG('a','d','t','l'), f) )
                        goto ioerror;

                /*      add the property-space note chunk  */
                if(write_dw_msf(TAG('n','o','t','e'), f)
                ||write_dw_lsf(sizeof(DWORD) + PROPCNKSIZE, f)
                ||write_dw_msf(TAG('s','f','i','f'), f)
#if defined CDP99 && defined _WIN32
                ||((f->propoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
                ||SetFilePointer(f->fileno, (long)PROPCNKSIZE,NULL, FILE_CURRENT) == 0xFFFFFFFF )
            goto ioerror;
#else
                ||fgetpos(f->fileno, &bytepos)
                ||fseek(f->fileno, (long)PROPCNKSIZE, SEEK_CUR) < 0)
            goto ioerror;
        f->propoffset = bytepos;
#endif

                f->propschanged = 1;
                f->proplim = PROPCNKSIZE;
        }
        /*
         *      and add the data chunk
         */
        f->datachunksize = 0;

        if(write_dw_msf(TAG('d','a','t','a'), f)
          ||write_dw_lsf(0, f)
#if defined CDP99 && defined _WIN32
          ||((f->datachunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF))
        goto ioerror;
#else
          ||fgetpos(f->fileno, &bytepos))
        goto ioerror;
    f->datachunkoffset = bytepos;
#endif

        f->header_set = 1;                      //later, may allow update prior to first write ?
        return 0;
        /* NOTREACHED */

ioerror:
        rsferrno = ESFWRERR;
        rsferrstr = "write error writing formatted wav header";
        return 1;
}


//wave-ex special
static int
wrwavex(struct sf_file *f, SFPROPS *props)
{
        struct cuepoint cue;
        //int extra = 0;
        int wordsize;
    WORD validbits,cbSize = 22;
    GUID guid;
        GUID *pGuid = NULL;
        //int fmtsize = sizeof_WFMTEX;
        fpos_t bytepos;
        POS64(bytepos) = 0;

        f->mainchunksize = 0;   /* we don't know the full size yet! */
        f->extrachunksizes = 0;
        POS64(f->propoffset) = 0;
        if(props->chformat==STDWAVE){
                rsferrno = ESFBADPARAM;
                rsferrstr = "std wave format requested for WAVE-EX file!";
                return 1;
        }

        if(props->samptype == FLOAT32){
                pGuid = (GUID *)  &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;

        }
        else{
                pGuid =(GUID *) &KSDATAFORMAT_SUBTYPE_PCM;
        }
        //NB AIFF and AIFF-C don't support masked formats - just write the required larger container size
        switch(props->samptype){
        case(INT_32):
        case(FLOAT32):
        case(INT2432):
                wordsize = sizeof(/*long*/int);
                break;
        case(INT2424):
        case(INT2024):
                wordsize = 3;
                break;
        case(SHORT16):
                wordsize= sizeof(short);
                break;
        default:
                //don't accept SHORT8, can't deal with INT_MASKED yet - no info in SFPROPS yet!
                rsferrno = ESFBADPARAM;
                rsferrstr = "unsupported sample format requested for WAVE-EX file";
                return 1;
        }
        if(props->samptype==INT2432)
                validbits = (WORD)24;
        else if(props->samptype==INT2024)
                validbits = (WORD)20;
        else
                validbits = (WORD)( 8 * wordsize);              //deal with more masks in due course...

        if(write_dw_msf(TAG('R','I','F','F'), f)
         ||write_dw_lsf(0, f)
         ||write_dw_msf(TAG('W','A','V','E'), f))
                goto ioerror;
        /* MC_STD,MC_GENERIC,MC_LCRS,MC_BFMT,MC_DOLBY_5_1,
              MC_SURR_5_0,MC_SURR_7_1,MC_CUBE,MC_WAVE_EX
    */

        f->fmtchunkEx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        f->fmtchunkEx.Format.nChannels = (unsigned short) props->chans;
        f->fmtchunkEx.Format.nSamplesPerSec = props->srate;
        f->fmtchunkEx.Format.nAvgBytesPerSec = wordsize * props->chans * props->srate;
        f->fmtchunkEx.Format.nBlockAlign = (unsigned short) ( wordsize * props->chans);
        f->fmtchunkEx.Format.wBitsPerSample = (WORD)(wordsize * 8);
    f->fmtchunkEx.Samples.wValidBitsPerSample = validbits;
    /*RWD Jan 30 2007: permit mask bits < nChans */
        switch(props->chformat){
        case(MC_STD):
                f->fmtchunkEx.dwChannelMask = SPKRS_UNASSIGNED;
                break;
        case(MC_MONO):
                if(props->chans /*!=*/ < 1){     /*RWD Jan 30 2007 */
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "conflicting channel configuration for WAVE-EX file";
                        return 1;
                }
                f->fmtchunkEx.dwChannelMask = SPKRS_MONO;
                break;
        case(MC_STEREO):
                if(props->chans /*!=*/ < 2){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "conflicting channel configuration for WAVE-EX file";
                        return 1;
                }
                f->fmtchunkEx.dwChannelMask = SPKRS_STEREO;
                break;
        case(MC_QUAD):
                if(props->chans /*!=*/ < 4){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "conflicting channel configuration for WAVE-EX file";
                        return 1;
                }

                f->fmtchunkEx.dwChannelMask = SPKRS_GENERIC_QUAD;
                break;
        case(MC_BFMT):
// Nov 2005: now supporting many channel counts for B-Format!
#ifdef NOTDEF
                if(props->chans != 4){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "conflicting channel configuration for WAVE-EX file";
                        return 1;
                }
#endif
                f->fmtchunkEx.dwChannelMask = SPKRS_UNASSIGNED;
                pGuid = props->samptype==FLOAT32 ?(GUID *) &SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT :(GUID *) &SUBTYPE_AMBISONIC_B_FORMAT_PCM;
                break;
        case(MC_LCRS):
                if(props->chans /*!=*/< 4){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "conflicting channel configuration for WAVE-EX file";
                        return 1;
                }

                f->fmtchunkEx.dwChannelMask = SPKRS_SURROUND_LCRS;
                break;
        case(MC_DOLBY_5_1):
                if(props->chans /*!=*/ < 6){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "conflicting channel configuration for WAVE-EX file";
                        return 1;
                }

                f->fmtchunkEx.dwChannelMask = SPKRS_DOLBY5_1;
                break;
    /*March 2008 */
    case(MC_SURR_5_0):
                if(props->chans /*!=*/ < 5){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "conflicting channel configuration for WAVE-EX file";
                        return 1;
                }

                f->fmtchunkEx.dwChannelMask = SPKRS_SURR_5_0;
                break;
        /* Nov 2013 */

        case(MC_SURR_6_1):
                if(props->chans < 7){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "conflicting channel configuration for WAVE-EX file";
                        return 1;
                }
                f->fmtchunkEx.dwChannelMask = SPKRS_SURR_6_1;
                break;
    /* OCT 2009 */
    case(MC_SURR_7_1):
                if(props->chans /*!=*/ < 8){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "conflicting channel configuration for WAVE-EX file";
                        return 1;
                }

                f->fmtchunkEx.dwChannelMask = SPKRS_SURR_7_1;
                break;
    case(MC_CUBE):
                if(props->chans /*!=*/ < 8){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "conflicting channel configuration for WAVE-EX file";
                        return 1;
                }

                f->fmtchunkEx.dwChannelMask = SPKRS_CUBE;
                break;
        default:
                rsferrno = ESFBADPARAM;
                rsferrstr = "unsupported channel configuration for WAVE-EX file";
                return 1;
                break;


        }


        if(write_dw_msf(TAG('f','m','t',' '), f)
         ||write_dw_lsf(sizeof_WFMTEX, f)                               //RWD CDP97: size = 18 to include cbSize field
#if defined CDP99 && defined _WIN32
         ||((f->fmtchunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
     ||write_w_lsf(f->fmtchunkEx.Format.wFormatTag, f)
     ||write_w_lsf(f->fmtchunkEx.Format.nChannels, f)
     ||write_dw_lsf(f->fmtchunkEx.Format.nSamplesPerSec, f)
     ||write_dw_lsf(f->fmtchunkEx.Format.nAvgBytesPerSec, f)
     ||write_w_lsf(f->fmtchunkEx.Format.nBlockAlign, f)
     ||write_w_lsf(f->fmtchunkEx.Format.wBitsPerSample, f)
     ||write_w_lsf(cbSize,f)
     )
                goto ioerror;
#else
         ||fgetpos(f->fileno, &bytepos)
         ||write_w_lsf(f->fmtchunkEx.Format.wFormatTag, f)
         ||write_w_lsf(f->fmtchunkEx.Format.nChannels, f)
         ||write_dw_lsf(f->fmtchunkEx.Format.nSamplesPerSec, f)
         ||write_dw_lsf(f->fmtchunkEx.Format.nAvgBytesPerSec, f)
         ||write_w_lsf(f->fmtchunkEx.Format.nBlockAlign, f)
         ||write_w_lsf(f->fmtchunkEx.Format.wBitsPerSample, f)
         ||write_w_lsf(cbSize,f)
        )
                goto ioerror;
    f->fmtchunkoffset = bytepos;

#endif
        //don't need fact chunk unless actually a compressed format...

   guid = *pGuid;
#ifdef MSBFIRST
   guid.Data1 = REVDWBYTES(guid.Data1);
   guid.Data2 = REVWBYTES(guid.Data2);
   guid.Data3 = REVWBYTES(guid.Data3);
#endif
   if(write_w_lsf(f->fmtchunkEx.Samples.wValidBitsPerSample,f)
                ||      write_dw_lsf(f->fmtchunkEx.dwChannelMask,f)
                || dowrite(f,(char *) &guid,sizeof(GUID))                 /* RWD NB deal with byte-reversal sometime! */
                )
                goto ioerror;

    // ADD the PEAK chunk
        if((f->min_header>=SFILE_PEAKONLY) && f->peaks){
                int i,size;
                DWORD now = 0;
                for(i=0;i < props->chans; i++){
                        f->peaks[i].value = 0.0f;
                        f->peaks[i].position = 0;
                }
                size = 2 * sizeof(DWORD) + props->chans * sizeof(CHPEAK);
                if(write_dw_msf(TAG('P','E','A','K'),f)
          || write_dw_lsf(size,f)
#if defined CDP99 && defined _WIN32
          || ((f->peakchunkoffset = SetFilePointer(f->fileno,0L,NULL, FILE_CURRENT))== 0xFFFFFFFF)
          || write_dw_lsf(CURRENT_PEAK_VERSION,f)
          || write_dw_lsf(now,f)
          || write_peak_lsf(props->chans,f))

            goto ioerror;
#else
                        || fgetpos(f->fileno, &bytepos)
                        || write_dw_lsf(CURRENT_PEAK_VERSION,f)
                        || write_dw_lsf(now,f)
                        || write_peak_lsf(props->chans,f))

                        goto ioerror;
        f->peakchunkoffset = bytepos;
#endif
        }


        if(f->min_header>=SFILE_CDP){
        /*
         *      add the cue point/note chunk for properties
         */
//RWD TODO: add switch to skip writing this extra stuff!
                if(write_dw_msf(TAG('c','u','e',' '), f)
                ||write_dw_lsf(sizeof(struct cuepoint) + sizeof(DWORD), f) )
                        goto ioerror;

                cue.name = TAG('s','f','i','f');
                cue.position = 0;
                cue.incchunkid = TAG('d','a','t','a');
                cue.chunkoffset = 0;
                cue.blockstart = 0;
                cue.sampleoffset = 0;
                if(write_dw_lsf(1, f)   /* one cue point */
                ||write_dw_msf(cue.name, f)
                ||write_dw_lsf(cue.position, f)
                ||write_dw_msf(cue.incchunkid, f)
                ||write_dw_lsf(cue.chunkoffset, f)
                ||write_dw_lsf(cue.blockstart, f)
                ||write_dw_lsf(cue.sampleoffset, f) )
                        goto ioerror;

                /*... add a LIST chunk of type 'adtl'... */

                if(write_dw_msf(TAG('L','I','S','T'), f)
                 ||write_dw_lsf(sizeof(DWORD) + 3*sizeof(DWORD) + PROPCNKSIZE, f)
                ||write_dw_msf(TAG('a','d','t','l'), f) )
                        goto ioerror;

                /*      add the property-space note chunk  */
                if(write_dw_msf(TAG('n','o','t','e'), f)
                ||write_dw_lsf(sizeof(DWORD) + PROPCNKSIZE, f)
                ||write_dw_msf(TAG('s','f','i','f'), f)
#if defined CDP99 && defined _WIN32
                ||((f->propoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
                ||SetFilePointer(f->fileno, (long)PROPCNKSIZE,NULL, FILE_CURRENT) == 0xFFFFFFFF )
            goto ioerror;
#else
                ||fgetpos(f->fileno, &bytepos)
                ||fseek(f->fileno, (long)PROPCNKSIZE, SEEK_CUR) < 0)
                        goto ioerror;
        f->propoffset = bytepos;

#endif
                f->propschanged = 1;
                f->proplim = PROPCNKSIZE;
        }

        /*
         *      and add the data chunk
         */
        f->datachunksize = 0;

        if(write_dw_msf(TAG('d','a','t','a'), f)
         ||write_dw_lsf(0, f)
#if defined CDP99 && defined _WIN32
          ||((f->datachunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF))
        goto ioerror;
#else
          ||fgetpos(f->fileno, &bytepos))

                goto ioerror;
    f->datachunkoffset = bytepos;
#endif
        f->header_set = 1;                      //later, may allow update prior to first write ?
        return 0;
        /* NOTREACHED */

ioerror:
        rsferrno = ESFWRERR;
        rsferrstr = "error writing wav_ex header";
        return 1;
}





/*
 *      aiff routines
 */

//RWD98 BUG, SOMEWHERE: THIS READS THE SSND CHUNK, AND GETS SIZE = REMAIN; BUT REMAIN SHOULD BE SIZE + 8
// SO, EITHER BUG IN THIS CODE, OR IN WRAIFFHDR... SO CANNOT USE WINDOWS DWORD 'COS UNSIGNED....
//RWD.6.99 NOTE: some tests require the Fomat field to be set, even though this is aiff
//RWD.7.99: accept masked AIFF formats: container size is always next intergral number of bytes
static int
rdaiffhdr(struct sf_file *f)
{
        DWORD /*long */ tag, size = 0, remain = 0, appltag; //RWD.04.98 can't be unsigned until the size anomalies are resolved
        DWORD peaktime; /* RWD Jan 2005 */
    int commseen = 0, ssndseen = 0;
        DWORD numsampleframes;
        DWORD ssnd_offset, ssnd_blocksize;
        fpos_t bytepos;
        POS64(bytepos) = 0;
        char *propspace;
        DWORD peak_version;
        // WARNING! The file can have the wrong size (e.g. from sox)
        if(read_dw_msf(&tag, f)
         ||read_dw_msf(&remain, f)
         ||tag != TAG('F','O','R','M')) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "File is not an AIFF file";
                return 1;
        }
        if(remain < 3*sizeof(DWORD)) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "File data size is too small";
                return 1;
        }
        if(read_dw_msf(&tag, f)
         ||tag != TAG('A','I','F','F')) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "File does not include an AIFF form";
                return 1;
        }
        f->mainchunksize = remain;
        f->extrachunksizes = 0;
        POS64(f->propoffset) = -1;
        f->aiffchunks = 0;
        remain -= sizeof(DWORD);

        while(remain > 0) {
                if(read_dw_msf(&tag, f)
                        ||read_dw_msf(&size,f)){
                        if(ssndseen && commseen){               //RWD accept the file anyway if we have enough
                                remain = 0;
                                break;
                        }
                        else
                                goto ioerror;
                }
                remain -= 2*sizeof(DWORD);
                switch(tag) {
                case TAG('C','O','M','M'):
                        if(size != 18) {
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "AIFF COMM chunk of incorrect size";
                                return 1;
                        }
#if defined CDP99 && defined _WIN32
                        if((f->fmtchunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF
              ||read_w_msf(&f->fmtchunkEx.Format.nChannels, f)
              ||read_dw_msf(&numsampleframes, f)
              ||read_w_msf(&f->fmtchunkEx.Format.wBitsPerSample, f)
              ||read_ex_todw(&f->fmtchunkEx.Format.nSamplesPerSec, f) )
                                goto ioerror;
#else
                        if(fgetpos(f->fileno, &bytepos)
                          ||read_w_msf(&f->fmtchunkEx.Format.nChannels, f)
                          ||read_dw_msf(&numsampleframes, f)
                          ||read_w_msf(&f->fmtchunkEx.Format.wBitsPerSample, f)
                          ||read_ex_todw(&f->fmtchunkEx.Format.nSamplesPerSec, f) )
                                goto ioerror;
            f->fmtchunkoffset = bytepos;
#endif
            /*RWD Trevor uses srate of zero for envel files! */
            /* nSamples... is unsigned anyway, so dont bother with this one any more... */
#ifdef NOTDEF
                        if(f->fmtchunkEx.Format.nSamplesPerSec < 0) {
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "Unknown AIFF sample rate";
                                return 1;
                        }
#endif
                        /*RWD.7.99 we now read 32bit in standard AIFF as LONGS
                        * we rely on the extra properties to tell if it's an analysis file */
                        f->fmtchunkEx.Format.wFormatTag = WAVE_FORMAT_PCM;
                        //fill in other info
                        f->fmtchunkEx.Samples.wValidBitsPerSample = f->fmtchunkEx.Format.wBitsPerSample;
                        //we have to deduce blockalign, and hence containersize
                        switch(f->fmtchunkEx.Samples.wValidBitsPerSample){
                        case(32):
                                f->fmtchunkEx.Format.nBlockAlign = sizeof(/*long*/int);
                                break;
                        case(20):
                        case(24):
                                f->fmtchunkEx.Format.nBlockAlign = 3;
                                break;
                        case(16):
                                f->fmtchunkEx.Format.nBlockAlign = sizeof(short);
                                break;
                        case(8):
                                f->fmtchunkEx.Format.nBlockAlign = sizeof(char);
                                break;
                        default:
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "unsupported sample size in aiff file";
                                return 1;
                        }
                        f->fmtchunkEx.Format.nBlockAlign *= f->fmtchunkEx.Format.nChannels;
                        //should do avgBytesPerSec too...
                        f->fmtchunkEx.dwChannelMask = 0;
                        remain -= 18;
                        commseen++;
                        break;

                        //RWD.5.99 read PEAK chunk
                case TAG('P','E','A','K'):
                        f->peaks = (CHPEAK *) calloc(f->fmtchunkEx.Format.nChannels,sizeof(CHPEAK));
                        if(f->peaks == NULL){
                                rsferrno = ESFNOMEM;
                                rsferrstr = "No memory for peak data";
                                return 1;
                        }
                        if(read_dw_msf(&peak_version,f))
                                goto ioerror;
                        switch(peak_version){
                        case(CURRENT_PEAK_VERSION):
                                if(read_dw_msf(&peaktime,f)) /* RWD Jan 2005 for DevCPP */
                                        goto ioerror;
                                f->peaktime = (time_t) peaktime;
#if defined CDP99 && defined _WIN32
                                if((f->peakchunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF
                   || read_peak_msf(f->fmtchunkEx.Format.nChannels,f))
                                        goto ioerror;
#else
                                if(fgetpos(f->fileno, &bytepos)
                                || read_peak_msf(f->fmtchunkEx.Format.nChannels,f))
                                        goto ioerror;
                f->peakchunkoffset = bytepos;
#endif
                                break;
                        default:
#ifdef _DEBUG
                                fprintf(stderr,"\nunknown PEAK version!");
#endif
                                free(f->peaks);
                                f->peaks = NULL;
                                break;
                        }
                        remain -= 2 * sizeof(DWORD) + (sizeof(CHPEAK) * f->fmtchunkEx.Format.nChannels);
                        break;


                case TAG('S','S','N','D'):
                        if(read_dw_msf(&ssnd_offset, f)
                         ||read_dw_msf(&ssnd_blocksize, f)
#if defined CDP99 && defined _WIN32
                         ||(f->datachunkoffset = SetFilePointer(f->fileno, 0L,NULL,FILE_CURRENT)) ==0xFFFFFFFF
                         ||     SetFilePointer(f->fileno, ((size+1)&~1) - 2 *sizeof(DWORD),NULL,FILE_CURRENT) ==0xFFFFFFFF)
                goto ioerror;
#else
             || fgetpos(f->fileno, &bytepos))
                 goto ioerror;
            f->datachunkoffset = bytepos;
            POS64(bytepos) += ((size+1)&~1) - 2 *sizeof(DWORD);
                        /*fseek(f->fileno, ((size+1)&~1) - 2 *sizeof(DWORD), SEEK_CUR) < 0)     */
            if(fsetpos(f->fileno,&bytepos))
                                goto ioerror;

#endif
            remain -= 2 * sizeof(DWORD); /* RWD Apr 2011 need this */
            /* RWD MAR 2015 eliminate warning, ssnd_offset is unsigned */
                        if(/* ssnd_offset < 0 || */ ssnd_offset > ssnd_blocksize) {
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "Funny offset in AIFF SSND chunk";
                                return 1;
                        }
                        POS64(f->datachunkoffset) += ssnd_offset;
                        ssndseen++;
                        remain -= (size+1)&~1;          //RWD98 BUG!!! remain can get less than size...
                        break;

                case 0:
                        rsferrno = ESFNOTFOUND;
                        rsferrstr = "Illegal zero tag in aiff chunk";
                        return 1;

                case TAG('A','P','P','L'):
                        if(size < sizeof(DWORD)
                         ||read_dw_msf(&appltag,f) )
                                goto ioerror;
                        if(appltag == TAG('s','f','i','f')) {
                                if(POS64(f->propoffset) >= 0
                                 ||(propspace = (char *) malloc(size - sizeof(DWORD))) == 0
#if defined CDP99 && defined _WIN32
                                 ||(f->propoffset = SetFilePointer(f->fileno, 0L,NULL,FILE_CURRENT))==0xFFFFFFFF
                 ||doread(f, propspace, size-sizeof(DWORD)) )
                                        goto ioerror;
#else
                                 ||fgetpos(f->fileno, &bytepos)
                                 ||doread(f, propspace, size-sizeof(DWORD)) )
                                        goto ioerror;
                POS64(f->propoffset) = POS64(bytepos);
#endif
                                f->proplim = size - sizeof(DWORD);
                                parseprops(f, propspace);
                                if(size&1)
                                        doread(f, propspace, 1);
                                free(propspace);
                                break;
                        } else {
#if defined CDP99 && defined _WIN32
                                if(SetFilePointer(f->fileno,-4L,NULL,FILE_CURRENT) == 0xFFFFFFFF)
#else
                                if(fseek(f->fileno, -4L, SEEK_CUR) < 0)
#endif
                                        goto ioerror;
                        }
                        /* FALLTHROUGH */

                default:
/* RWD MAR 2015: size is unsigned, eliminate warning! */
#ifdef NOTDEF
                        if(size < 0  /* || size > 100*1024 */) {  /* RWD Apr 2011 */
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "Silly size for unknown AIFF chunk";
                                return 1;
                        }
#endif
                        if(ssndseen) {
                                struct aiffchunk **cpp, *cp;

                                for(cpp = &f->aiffchunks; *cpp != 0; cpp = &(*cpp)->next)
                                        ;
                                if((*cpp = cp = ALLOC(struct aiffchunk)) == 0
                                 ||(cp->buf = (char *) malloc((size+1)&~1)) == 0) {
                                        rsferrno = ESFNOMEM;
                                        rsferrstr = "No memory for aiff chunk storage";
                                        return 1;
                                }
                                cp->tag = tag;
                                cp->size = size;
                                cp->next = 0;
#if defined CDP99 && defined _WIN32
                                if((cp->offset = SetFilePointer(f->fileno,0L,NULL,FILE_CURRENT)) == 0xFFFFFFFF
                   ||doread(f, cp->buf, (size+1)&~1))
                                        goto ioerror;
#else
                                if(fgetpos(f->fileno,&bytepos)
                                 ||doread(f, cp->buf, (size+1)&~1))
                                        goto ioerror;
                cp->offset = bytepos;
#endif
                        } else {
#if defined CDP99 && defined _WIN32
                                if(SetFilePointer(f->fileno,(long)((size+1)&~1),NULL,FILE_CURRENT) == 0xFFFFFFFF)
#else
                                if(fseeko(f->fileno, (size+1)&~1, SEEK_CUR) < 0)
#endif
                                        goto ioerror;
                        }

                        f->extrachunksizes += ((size+1)&~1) + 2*sizeof(DWORD);
                        remain -= (size+1)&~1;
                        break;

                }
        }

        if(!commseen) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "AIFF format error: no COMM chunk found";
                return 1;
        }
        if(!ssndseen) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "AIFF format error: no SSND chunk found";
                return 1;
        }

        f->datachunksize = numsampleframes * f->fmtchunkEx.Format.nChannels;
        switch(f->fmtchunkEx.Format.wBitsPerSample) {
        case 32:                /* floats */
                f->datachunksize *= 4;
                break;
        case 20:
        case 24:
                f->datachunksize *= 3;
                break;
        case 16:                /* shorts */
                f->datachunksize *= 2;
                break;
        case 8:                 /* byte -> short mappping */
                f->datachunksize *= 2;  /* looks like short samples! */
                break;
        default:
                rsferrno = ESFNOTFOUND;
                rsferrstr = "can't open aiff file - unsupported wordsize";
                return 1;
        }

#if defined CDP99 && defined _WIN32
        if(SetFilePointer(f->fileno,f->datachunkoffset,NULL,FILE_BEGIN)==0xFFFFFFFF)
#else
    bytepos = f->datachunkoffset;
        if(fsetpos(f->fileno, &bytepos) < 0)
#endif
                goto ioerror;
        return 0;
        /* NOTREACHED */

ioerror:
        rsferrno = ESFRDERR;
        rsferrstr = "read error (or file too short) reading AIFF header";
        return 1;
}


/*AIF-C*/

#define AIFC_VERSION_1 (0xA2805140)
static int
rdaifchdr(struct sf_file *f)
{
        DWORD tag, size = 0, remain = 0, appltag;
        int commseen = 0, ssndseen = 0,fmtverseen = 0;
        DWORD numsampleframes, aifcver,ID_compression;
        DWORD ssnd_offset, ssnd_blocksize;
        char *propspace;
        DWORD peak_version;
    DWORD peaktime;
    fpos_t bytepos;

        if(read_dw_msf(&tag, f)
         ||read_dw_msf(&remain, f)
         ||tag != TAG('F','O','R','M')) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "File is not an AIFF file";
                return 1;
        }
        if(remain < 3*sizeof(DWORD)) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "File data size is too small";
                return 1;
        }
        if(read_dw_msf(&tag, f)
         ||tag != TAG('A','I','F','C')) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "File does not include an AIFC form";
                return 1;
        }
        f->mainchunksize = remain;
        f->extrachunksizes = 0;
        POS64(f->propoffset) = -1;
        f->aiffchunks = 0;

        //start by assuming integer format:
        f->fmtchunkEx.Format.wFormatTag = WAVE_FORMAT_PCM;


        remain -= sizeof(DWORD);

        while(remain > 0) {
                if(read_dw_msf(&tag, f)
                 ||read_dw_msf(&size,f)){
                        if(ssndseen && commseen){               //RWD accept the file anyway if we have enough
                                remain = 0;
                                break;
                        }
                        else
                                goto ioerror;
                }
                remain -= 2*sizeof(DWORD);
                switch(tag) {
                case TAG('F','V','E','R'):
                        if(size != 4){
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "bad aif-c FVER chunk";
                                return 1;
                        }

                        if(read_dw_msf(&aifcver,f) || aifcver != AIFC_VERSION_1){
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "bad aif-c Version";
                                return 1;
                        }
                        remain -= sizeof(DWORD);
                        fmtverseen++;
                        break;
                case TAG('C','O','M','M'):
                        if(size < 22) {
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "AIFC COMM chunk of incorrect size";
                                return 1;
                        }
#if defined CDP99 && defined _WIN32
                        if((f->fmtchunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF
               ||read_w_msf(&f->fmtchunkEx.Format.nChannels, f)
               ||read_dw_msf(&numsampleframes, f)
               ||read_w_msf(&f->fmtchunkEx.Format.wBitsPerSample, f)
               ||read_ex_todw(&f->fmtchunkEx.Format.nSamplesPerSec, f) )
                                goto ioerror;
#else
                        if(fgetpos(f->fileno, &bytepos)
                         ||read_w_msf(&f->fmtchunkEx.Format.nChannels, f)
                         ||read_dw_msf(&numsampleframes, f)
                         ||read_w_msf(&f->fmtchunkEx.Format.wBitsPerSample, f)
                         ||read_ex_todw(&f->fmtchunkEx.Format.nSamplesPerSec, f) )
                                goto ioerror;
            f->fmtchunkoffset = bytepos;
#endif
            /*RWD: Trevor uses srate of zero for envel files! */
/* RWD MAR 2015: so elimianet code to avoid warning, as above */
#ifdef NOTDEF
                        if(f->fmtchunkEx.Format.nSamplesPerSec < 0) {
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "Unknown AIFC sample rate";
                                return 1;
                        }
#endif
                        if(read_dw_msf(&ID_compression,f))
                                goto ioerror;
                        if( !(
                                (ID_compression == TAG('N','O','N','E'))
                                  || (ID_compression == TAG('F','L','3','2'))   //Csound
                                  || (ID_compression == TAG('f','l','3','2'))   //Apple
/*  used in Steinberg 24bit SDIR files */
                  || (ID_compression == TAG('i','n','2','4'))
                                                  )){
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "Unknown AIFC compression type";
                                return 1;
                        }
                        //set sample type in sfinfo
                        if((ID_compression== TAG('F','L','3','2'))
                                || ID_compression == TAG('f','l','3','2')){

/*Nov 2001: F***** Quicktime writes size = 16, for floats! */
                                        if(f->fmtchunkEx.Format.wBitsPerSample != 32){
                                                if(f->fmtchunkEx.Format.wBitsPerSample != 16){
                                                        rsferrno = ESFNOTFOUND;
                                                        rsferrstr = "error in AIFC header: samples not 32bit in floats file ";
                                                        return 1;
                                                }
                                                else
                                                        f->fmtchunkEx.Format.wBitsPerSample = 32;
                                        }
                                        f->fmtchunkEx.Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
                        }


            /* RWD 06/01/09  precautionary, to validate 'in24'  24bit files  */
            if(ID_compression == TAG('i','n','2','4')) {
                                if(f->fmtchunkEx.Format.wBitsPerSample != 24){
                                        rsferrstr = "error in AIFC header: sample size not 24bit in <in24> file ";
                                        return 1;
                                }
                        }


                        //no ambiguity here with 32bit format.

                        //fill in other info
                        f->fmtchunkEx.Samples.wValidBitsPerSample = f->fmtchunkEx.Format.wBitsPerSample;
                        f->fmtchunkEx.dwChannelMask = 0;
                        //we have to deduce blockalign, and hence containersize
                        switch(f->fmtchunkEx.Samples.wValidBitsPerSample){
                        case(32):
                                f->fmtchunkEx.Format.nBlockAlign = sizeof(/*long*/int);
                                break;
                        case(20):
                        case(24):
                                f->fmtchunkEx.Format.nBlockAlign = 3;
                                break;
                        case(16):
                                f->fmtchunkEx.Format.nBlockAlign = sizeof(short);
                                break;
                        case(8):
                                f->fmtchunkEx.Format.nBlockAlign = sizeof(char);
                                break;
                        default:
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "unsupported sample size in aiff file";
                                return 1;
                        }
                        f->fmtchunkEx.Format.nBlockAlign *= f->fmtchunkEx.Format.nChannels;
                        //should do avgBytesPerSec too...

                        /*RWD 23:10:2000 can get odd sizes... */

                        //skip past pascal string
#if defined CDP99 && defined _WIN32
                        if(SetFilePointer(f->fileno, ((size+1)&~1) - 22,NULL,FILE_CURRENT) ==0xFFFFFFFF)
#else
                        if(fseek(f->fileno,((size+1)&~1)        - 22,SEEK_CUR) < 0)
#endif
                                goto ioerror;
                        remain -= (size+1)&~1;
                        commseen++;
                        break;

                        //RWD.5.99 read PEAK chunk
                case TAG('P','E','A','K'):
                        f->peaks = (CHPEAK *) calloc(f->fmtchunkEx.Format.nChannels,sizeof(CHPEAK));
                        if(f->peaks == NULL){
                                rsferrno = ESFNOMEM;
                                rsferrstr = "No memory for peak data";
                                return 1;
                        }
                        if(read_dw_msf(&peak_version,f))
                                goto ioerror;
                        switch(peak_version){
                        case(CURRENT_PEAK_VERSION):
                                if(read_dw_msf(&peaktime,f))
                                        goto ioerror;
                                f->peaktime = (time_t) peaktime;
#if defined CDP99 && defined _WIN32
                                if((f->peakchunkoffset = SetFilePointer(f->fileno, 0L, NULL, FILE_CURRENT)) == 0xFFFFFFFF
                  || read_peak_msf(f->fmtchunkEx.Format.nChannels,f))
                                        goto ioerror;
#else
                                if(fgetpos(f->fileno, &bytepos)
                                  || read_peak_msf(f->fmtchunkEx.Format.nChannels,f))
                                        goto ioerror;
                f->peakchunkoffset = bytepos;
#endif
                                break;
                        default:
#ifdef _DEBUG
                                fprintf(stderr,"\nunknown PEAK version!");
#endif
                                free(f->peaks);
                                f->peaks = NULL;
                                break;
                        }
                        remain -= 2 * sizeof(DWORD) + (sizeof(CHPEAK) * f->fmtchunkEx.Format.nChannels);
                        break;

                case TAG('S','S','N','D'):
                        if(read_dw_msf(&ssnd_offset, f)
                         ||read_dw_msf(&ssnd_blocksize, f)
#if defined CDP99 && defined _WIN32
                         ||(f->datachunkoffset = SetFilePointer(f->fileno, 0L,NULL,FILE_CURRENT)) ==0xFFFFFFFF
                         ||     SetFilePointer(f->fileno, ((size+1)&~1) - 2 *sizeof(DWORD),NULL,FILE_CURRENT) ==0xFFFFFFFF)
               goto ioerror;
            if(ssnd_offset < 0 || ssnd_offset > ssnd_blocksize) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "Funny offset in AIFC SSND chunk";
                return 1;
            }
            f->datachunkoffset += ssnd_offset;
#else
                         ||fgetpos(f->fileno, &bytepos) )
                goto ioerror;
            f->datachunkoffset = bytepos;
            POS64(bytepos) = ((size+1)&~1) - 2 *sizeof(DWORD);
            if(fseeko(f->fileno,POS64(bytepos), SEEK_CUR) < 0)
                                goto ioerror;
                        /* RWD MAR 2015 ssnd_offset unsigned, eliminate compiler warning */
                        if(/* ssnd_offset < 0 || */ ssnd_offset > ssnd_blocksize) {
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "Funny offset in AIFC SSND chunk";
                                return 1;
                        }
                        POS64(f->datachunkoffset) += ssnd_offset;
#endif
                        ssndseen++;
                        remain -= (size+1)&~1;          //RWD98 BUG!!! remain can get less than size...
                        break;

                case 0:
                        rsferrno = ESFNOTFOUND;
                        rsferrstr = "Illegal zero tag in aifc chunk";
                        return 1;

                case TAG('A','P','P','L'):
                        if(size < sizeof(DWORD)
                         ||read_dw_msf(&appltag,f) )
                                goto ioerror;
                        if(appltag == TAG('s','f','i','f')) {
                                if(POS64(f->propoffset) >= 0
                                 ||(propspace = (char *) malloc(size - sizeof(DWORD))) == 0
#if defined CDP99 && defined _WIN32
                                 ||(f->propoffset = SetFilePointer(f->fileno, 0L,NULL,FILE_CURRENT))==0xFFFFFFFF
                 ||doread(f, propspace, size-sizeof(DWORD)) )
                                        goto ioerror;
#else
                                 ||fgetpos(f->fileno, &bytepos)
                                 ||doread(f, propspace, size-sizeof(DWORD)) )
                                        goto ioerror;
                f->propoffset = bytepos;
#endif
                                f->proplim = size - sizeof(DWORD);
                                parseprops(f, propspace);
                                if(size&1)
                                        doread(f, propspace, 1);
                                free(propspace);
                                break;
                        } else {
#if defined CDP99 && defined _WIN32
                                if(SetFilePointer(f->fileno,-4L,NULL,FILE_CURRENT) == 0xFFFFFFFF)
#else
                                if(fseek(f->fileno, -4L, SEEK_CUR) < 0)
#endif
                                        goto ioerror;
                        }
                        /* FALLTHROUGH */

                default:
            /* RWD MAR 2015 eliminate compiler warning; bit of an arbitrary exclusion anyway? */
                        if(/* size < 0 || */ size > 100*1024) {
                                rsferrno = ESFNOTFOUND;
                                rsferrstr = "Silly size for unknown AIFC chunk";
                                return 1;
                        }

                        if(ssndseen) {
                                struct aiffchunk **cpp, *cp;

                                for(cpp = &f->aiffchunks; *cpp != 0; cpp = &(*cpp)->next)
                                        ;
                                if((*cpp = cp = ALLOC(struct aiffchunk)) == 0
                                 ||(cp->buf = (char *) malloc((size+1)&~1)) == 0) {
                                        rsferrno = ESFNOMEM;
                                        rsferrstr = "No memory for aifc chunk storage";
                                        return 1;
                                }
                                cp->tag = tag;
                                cp->size = size;
                                cp->next = 0;
#if defined CDP99 && defined _WIN32
                                if((cp->offset = SetFilePointer(f->fileno,0L,NULL,FILE_CURRENT)) == 0xFFFFFFFF
                  ||doread(f, cp->buf, (size+1)&~1))
                                        goto ioerror;
#else
                                if(fgetpos(f->fileno, &bytepos)
                                  ||doread(f, cp->buf, (size+1)&~1))
                                        goto ioerror;
                cp->offset = bytepos;
#endif
                        } else {
#if defined CDP99 && defined _WIN32
                                if(SetFilePointer(f->fileno,(long)((size+1)&~1),NULL,FILE_CURRENT) == 0xFFFFFFFF)
                    goto ioerror;
#else
                                if(fgetpos(f->fileno, &bytepos))
                    goto ioerror;
                POS64(bytepos) += (size+1)&~1;
                if(fsetpos(f->fileno, &bytepos))
                                        goto ioerror;
#endif
                        }

                        f->extrachunksizes += ((size+1)&~1) + 2*sizeof(DWORD);
                        remain -= (size+1)&~1;
                        break;

                }
        }

        if(!commseen) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "AIFC format error: no COMM chunk found";
                return 1;
        }
        if(!ssndseen) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "AIFC format error: no SSND chunk found";
                return 1;
        }
        if(!fmtverseen) {
                rsferrno = ESFNOTFOUND;
                rsferrstr = "AIFC format error: no FVER chunk found";
                return 1;
        }

        f->datachunksize = numsampleframes * f->fmtchunkEx.Format.nChannels;
        switch(f->fmtchunkEx.Format.wBitsPerSample) {
        case 32:                /* floats */
                f->datachunksize *= 4;
                break;
        case 16:                /* shorts */
                f->datachunksize *= 2;
                break;
        case 20:
        case 24:
                f->datachunksize *= 3;
                break;
        case 8:                 /* byte -> short mappping */
                f->datachunksize *= 2;  /* looks like short samples! */
                break;
        default:
                rsferrno = ESFNOTFOUND;
                rsferrstr = "can't open Sfile - unsupported wordsize";
                return 1;
        }

#if defined CDP99 && defined _WIN32
        if(SetFilePointer(f->fileno,f->datachunkoffset,NULL,FILE_BEGIN)==0xFFFFFFFF)
#else
    POS64(bytepos) = POS64(f->datachunkoffset);
        if(fsetpos(f->fileno, &bytepos) < 0)
#endif
                goto ioerror;
        return 0;
        /* NOTREACHED */

ioerror:
        rsferrno = ESFRDERR;
        rsferrstr = "read error (or file too short) reading AIFC header";
        return 1;
}


#if 0
static int
wraiffhdr(struct sf_file *f)
{
    fpos_t bytepos;

        f->mainchunksize = 0;
        if(write_dw_msf(TAG('F','O','R','M'), f)
         ||write_dw_msf(0, f)
         ||write_dw_msf(TAG('A','I','F','F'), f) )
                goto ioerror;

        f->fmtchunkEx.Format.nChannels = 1;
        f->fmtchunkEx.Format.nSamplesPerSec = 44100;
        f->fmtchunkEx.Format.wBitsPerSample = 16;
        f->aiffchunks = 0;

        if(write_dw_msf(TAG('C','O','M','M'), f)
         || write_dw_msf(18, f)
#if defined CDP99 && defined _WIN32
         || (f->fmtchunkoffset = SetFilePointer(f->fileno,0L,NULL,FILE_CURRENT))==0xFFFFFFFF
     || write_w_msf(f->fmtchunkEx.Format.nChannels, f)
     || write_dw_msf(0, f)                      /* num sample frames */
     || write_w_msf(f->fmtchunkEx.Format.wBitsPerSample, f)
     || write_dw_toex(f->fmtchunkEx.Format.nSamplesPerSec, f) )
        goto ioerror;
#else
         ||fgetpos(f->fileno, &bytepos)
         || write_w_msf(f->fmtchunkEx.Format.nChannels, f)
         || write_dw_msf(0, f)                  /* num sample frames */
         || write_w_msf(f->fmtchunkEx.Format.wBitsPerSample, f)
         || write_dw_toex(f->fmtchunkEx.Format.nSamplesPerSec, f) )
                goto ioerror;
    f->fmtchunkoffset = bytepos;
#endif
        if(write_dw_msf(TAG('A','P','P','L'), f)
         || write_dw_msf(sizeof(DWORD) + PROPCNKSIZE, f)
         || write_dw_msf(TAG('s','f','i','f'), f)
#if defined CDP99 && defined _WIN32
         || (f->propoffset = SetFilePointer(f->fileno, 0L,NULL,FILE_CURRENT))==0xFFFFFFFF
         || SetFilePointer(f->fileno,(long)PROPCNKSIZE,NULL,FILE_CURRENT) == 0xFFFFFFFF)
        goto ioerror;
#else
         || fgetpos(f->fileno, &bytepos)
         || fseek(f->fileno, (long)PROPCNKSIZE, SEEK_CUR) < 0)
                goto ioerror;
    f->propoffset = bytepos;
#endif
        if(write_dw_msf(TAG('S','S','N','D'), f)
         || write_dw_msf(0, f)
         || write_dw_msf(0, f)          /* offset */
         || write_dw_msf(0, f)          /* blocksize */
#if defined CDP99 && defined _WIN32
         || (f->datachunkoffset = SetFilePointer(f->fileno,0L,NULL,FILE_CURRENT)) == 0xFFFFFFFF)
        goto ioerror;
#else
         || fgetpos(f->fileno, &bytepos) )
                goto ioerror;
    f->datachunkoffset = bytepos;
#endif
        f->propschanged = 1;
        f->proplim = PROPCNKSIZE;
        f->datachunksize = 0;
        f->extrachunksizes = 0;

        return 0;
        /* NOTREACHED */
ioerror:
        rsferrno = ESFWRERR;
        rsferrstr = "write error writing aiff header";
        return 1;
}
#endif

/*********** sfsys98 extension **********/

static int
wraiffhdr98(struct sf_file *f,int channels,int srate,int stype)
{
    fpos_t bytepos;

        if(stype >= SAMP_MASKED)
                return 1;
        rsferrstr = NULL;
        f->mainchunksize = 0;
        if(write_dw_msf(TAG('F','O','R','M'), f)
         || write_dw_msf(0, f)
         || write_dw_msf(TAG('A','I','F','F'), f) )
                goto ioerror;

        f->fmtchunkEx.Format.nChannels = (short)channels;
        f->fmtchunkEx.Format.nSamplesPerSec = srate;
        switch(stype){
        case(SAMP_SHORT):
                f->fmtchunkEx.Format.wBitsPerSample = 16;
                f->fmtchunkEx.Format.nBlockAlign = sizeof(short) * f->fmtchunkEx.Format.nChannels;
                break;
        case(SAMP_FLOAT):         //need to keep this for now...
        case(SAMP_LONG):
                f->fmtchunkEx.Format.wBitsPerSample = 32;
                f->fmtchunkEx.Format.nBlockAlign = sizeof(/*long*/int) * f->fmtchunkEx.Format.nChannels;
                break;
        case(SAMP_2024):
                f->fmtchunkEx.Format.wBitsPerSample = 20;
                f->fmtchunkEx.Format.nBlockAlign = 3 * f->fmtchunkEx.Format.nChannels;
                break;
        case(SAMP_2424):
                f->fmtchunkEx.Format.wBitsPerSample = 24;
                f->fmtchunkEx.Format.nBlockAlign = 3 * f->fmtchunkEx.Format.nChannels;
                break;
        //NB 24/32 not allowed in AIFF
        //case SAMP_MASKED: supported by AIFF, inside nearest integral byte-size
        default:
                rsferrstr = "sample format not supported in AIFF files";
                goto ioerror;   //something we don't know about!


        }
        f->aiffchunks = 0;

        if(write_dw_msf(TAG('C','O','M','M'), f)
         || write_dw_msf(18, f)
#if defined CDP99 && defined _WIN32
         || (f->fmtchunkoffset = SetFilePointer(f->fileno,0L,NULL,FILE_CURRENT))==0xFFFFFFFF
     || write_w_msf(f->fmtchunkEx.Format.nChannels, f)
     || write_dw_msf(0, f)                      /* num sample frames */
     || write_w_msf(f->fmtchunkEx.Format.wBitsPerSample, f)
     || write_dw_toex(f->fmtchunkEx.Format.nSamplesPerSec, f) )
       goto ioerror;
#else
         || fgetpos(f->fileno, &bytepos)
         || write_w_msf(f->fmtchunkEx.Format.nChannels, f)
         || write_dw_msf(0, f)                  /* num sample frames */
         || write_w_msf(f->fmtchunkEx.Format.wBitsPerSample, f)
         || write_dw_toex(f->fmtchunkEx.Format.nSamplesPerSec, f) )
                goto ioerror;
    f->fmtchunkoffset = bytepos;
#endif
        //RWD.6.5.99 ADD the PEAK chunk
        if((f->min_header >= SFILE_PEAKONLY) && f->peaks){
                int i,size;
                DWORD now = 0;
                for(i=0;i < channels; i++){
                        f->peaks[i].value = 0.0f;
                        f->peaks[i].position = 0;
                }
                size = 2 * sizeof(DWORD) + channels * sizeof(CHPEAK);
                if(write_dw_msf(TAG('P','E','A','K'),f)
          || write_dw_msf(size,f)
#if defined CDP99 && defined _WIN32
                  || ((f->peakchunkoffset = SetFilePointer(f->fileno,0L,NULL, FILE_CURRENT))== 0xFFFFFFFF)
          || write_dw_msf(CURRENT_PEAK_VERSION,f)
          || write_dw_msf(now,f)
          || write_peak_msf(channels,f))
                        goto ioerror;
#else
          || fgetpos(f->fileno, &bytepos)
                  || write_dw_msf(CURRENT_PEAK_VERSION,f)
                  || write_dw_msf(now,f)
                  || write_peak_msf(channels,f))
                        goto ioerror;
        f->peakchunkoffset = bytepos;
#endif
        }

        if(f->min_header >= SFILE_CDP){

                if(write_dw_msf(TAG('A','P','P','L'), f)
                  || write_dw_msf(sizeof(DWORD) + PROPCNKSIZE, f)
                  || write_dw_msf(TAG('s','f','i','f'), f)
#if defined CDP99 && defined _WIN32
                  || (f->propoffset = SetFilePointer(f->fileno, 0L,NULL,FILE_CURRENT))==0xFFFFFFFF
                  || SetFilePointer(f->fileno,(long)PROPCNKSIZE,NULL,FILE_CURRENT) == 0xFFFFFFFF)
            goto ioerror;
#else
                  || fgetpos(f->fileno, &bytepos)
                  || fseek(f->fileno, (long)PROPCNKSIZE, SEEK_CUR) < 0)
                        goto ioerror;
        f->propoffset = bytepos;
#endif
        }

        if(write_dw_msf(TAG('S','S','N','D'), f)
          || write_dw_msf(0, f)
          || write_dw_msf(0, f)         /* offset */
          || write_dw_msf(0, f)         /* blocksize */
#if defined CDP99 && defined _WIN32
          || (f->datachunkoffset = SetFilePointer(f->fileno,0L,NULL,FILE_CURRENT)) == 0xFFFFFFFF)
        goto ioerror;
#else
          || fgetpos(f->fileno, &bytepos) )
                goto ioerror;
    f->datachunkoffset = bytepos;
#endif
        if(f->min_header >= SFILE_CDP){
                f->propschanged = 1;
                f->proplim = PROPCNKSIZE;
        }
        f->datachunksize = 0;
        f->extrachunksizes = 0;
        f->header_set = 1;
        return 0;
        /* NOTREACHED */
ioerror:
        rsferrno = ESFWRERR;
        if(rsferrstr == NULL)
                rsferrstr = "error writing aiff header";
        return 1;
}

/*RWD 22:6:2000 now use 'fl32', so the new Cubase will read it!*/
static int
wraifchdr(struct sf_file *f,int channels,int srate,int stype)
{
        DWORD aifcver = AIFC_VERSION_1;
        DWORD ID_compression;
    fpos_t bytepos;
        //assume 32bit floats, but we may be asked to use aifc for integer formats too
        char *str_compressed = (char *) aifc_floatstring;
        int pstring_size = 10;
        rsferrstr = NULL;
        if(stype >= SAMP_MASKED)
                return 1;
        /*RWD Sept 2000: was "ff32!" .*/
        if(stype==SAMP_FLOAT)
                ID_compression = TAG('f','l','3','2');
        /* RWD 06-01-09 TODO:  add "in24"  type? */
        else {
                ID_compression = TAG('N','O','N','E');
                pstring_size = 16;
                str_compressed = (char *) aifc_notcompressed;
        }

        f->mainchunksize = 0;
        if(write_dw_msf(TAG('F','O','R','M'), f)
         ||write_dw_msf(0, f)
         ||write_dw_msf(TAG('A','I','F','C'), f) )
                goto ioerror;

        f->fmtchunkEx.Format.nChannels = (short)channels;
        f->fmtchunkEx.Format.nSamplesPerSec = srate;
        switch(stype){
        case(SAMP_SHORT):
                f->fmtchunkEx.Format.wBitsPerSample = 16;
                f->fmtchunkEx.Format.nBlockAlign = sizeof(short) * f->fmtchunkEx.Format.nChannels;

                break;
        case(SAMP_FLOAT):
        case(SAMP_LONG):
                f->fmtchunkEx.Format.wBitsPerSample = 32;
                f->fmtchunkEx.Format.nBlockAlign = sizeof(/*long*/int) * f->fmtchunkEx.Format.nChannels;

                break;
        case(SAMP_2024):
                f->fmtchunkEx.Format.wBitsPerSample = 20;
                f->fmtchunkEx.Format.nBlockAlign = 3 * f->fmtchunkEx.Format.nChannels;

        case(SAMP_2424):
                f->fmtchunkEx.Format.wBitsPerSample = 24;
                f->fmtchunkEx.Format.nBlockAlign = 3 * f->fmtchunkEx.Format.nChannels;

                break;
        //NB 2432 format not allowed in AIF file!
        default:
                rsferrstr = "requested sample format not supported by AIFF-C";
                goto ioerror;   //something we don't know about!


        }
        f->aiffchunks = 0;
        //write FVER chunk
        if(write_dw_msf(TAG('F','V','E','R'),f)
                || write_dw_msf(4,f)
                || write_dw_msf(aifcver,f))
                goto ioerror;

        //extended COMM chunk...22 bytes plus size of pascal string, rounded
        if(write_dw_msf(TAG('C','O','M','M'), f)
          || write_dw_msf(22 + pstring_size, f)
#if defined CDP99 && defined _WIN32
          || (f->fmtchunkoffset = SetFilePointer(f->fileno,0L,NULL,FILE_CURRENT))==0xFFFFFFFF
      || write_w_msf(f->fmtchunkEx.Format.nChannels, f)
      || write_dw_msf(0, f)                     /* num sample frames */
      || write_w_msf(f->fmtchunkEx.Format.wBitsPerSample, f)
      || write_dw_toex(f->fmtchunkEx.Format.nSamplesPerSec, f) )
                goto ioerror;
#else
          || fgetpos(f->fileno, &bytepos)
          || write_w_msf(f->fmtchunkEx.Format.nChannels, f)
          || write_dw_msf(0, f)                 /* num sample frames */
          || write_w_msf(f->fmtchunkEx.Format.wBitsPerSample, f)
          || write_dw_toex(f->fmtchunkEx.Format.nSamplesPerSec, f) )
                goto ioerror;
    f->fmtchunkoffset = bytepos;
#endif
        //now the special bits...
        if(write_dw_msf(ID_compression,f))
                goto ioerror;
        //the dreaded pascal string...
        if(dowrite(f,str_compressed,pstring_size))
                goto ioerror;
        //RWD.6.5.99 ADD the PEAK chunk
        if((f->min_header >= SFILE_PEAKONLY) && f->peaks){
                int i,size;
                DWORD now = 0;
                for(i=0;i < channels; i++){
                        f->peaks[i].value = 0.0f;
                        f->peaks[i].position = 0;
                }
                size = 2 * sizeof(DWORD) + channels * sizeof(CHPEAK);
                if(write_dw_msf(TAG('P','E','A','K'),f)
          || write_dw_msf(size,f)
#if defined CDP99 && defined _WIN32
                  || ((f->peakchunkoffset = SetFilePointer(f->fileno,0L,NULL, FILE_CURRENT))== 0xFFFFFFFF)
          || write_dw_msf(CURRENT_PEAK_VERSION,f)
          || write_dw_msf(now,f)
          || write_peak_msf(channels,f))
                        goto ioerror;
#else
                  || fgetpos(f->fileno, &bytepos)
                  || write_dw_msf(CURRENT_PEAK_VERSION,f)
                  || write_dw_msf(now,f)
                  || write_peak_msf(channels,f))
                        goto ioerror;
        f->peakchunkoffset = bytepos;
#endif
        }

        if(f->min_header >= SFILE_CDP){
                if(write_dw_msf(TAG('A','P','P','L'), f)
                  || write_dw_msf(sizeof(DWORD) + PROPCNKSIZE, f)
                  || write_dw_msf(TAG('s','f','i','f'), f)
#if defined CDP99 && defined _WIN32
                  || (f->propoffset = SetFilePointer(f->fileno, 0L,NULL,FILE_CURRENT))==0xFFFFFFFF
                  || SetFilePointer(f->fileno,(long)PROPCNKSIZE,NULL,FILE_CURRENT) == 0xFFFFFFFF)
            goto ioerror;
#else
                  || fgetpos(f->fileno, &bytepos)
                  || fseek(f->fileno, (long)PROPCNKSIZE, SEEK_CUR) < 0)
                        goto ioerror;
        f->propoffset = bytepos;
#endif
        }

        if(write_dw_msf(TAG('S','S','N','D'), f)
          ||write_dw_msf(0, f)
          ||write_dw_msf(0, f)          /* offset */
          ||write_dw_msf(0, f)          /* blocksize */
#if defined CDP99 && defined _WIN32
          ||(f->datachunkoffset = SetFilePointer(f->fileno,0L,NULL,FILE_CURRENT)) == 0xFFFFFFFF)
        goto ioerror;
#else
          ||fgetpos(f->fileno, &bytepos) )
                goto ioerror;
    f->datachunkoffset = bytepos;
#endif
        if(f->min_header >= SFILE_CDP){
                f->propschanged = 1;
                f->proplim = PROPCNKSIZE;
        }
        f->datachunksize = 0;
        f->extrachunksizes = 0;
        f->header_set = 1;
        return 0;
        /* NOTREACHED */
ioerror:
        rsferrno = ESFWRERR;
        if(rsferrstr==NULL)
                rsferrstr = "error writing aiff-c header";
        return 1;
}




/*
 *      Initialization routines
 */
#if 0
int
sfinit()
{
        return sflinit("%no name app%");
}
#endif
            
static void
rsffinish(void)
{
        int i;

        for(i = 0; i < SF_MAXFILES; i++)
                if(sf_files[i] != 0)
                        sfclose(i+SFDBASE);

#ifdef _WIN32
                if(CDP_COM_READY){
                        COMclose();
                        CDP_COM_READY = 0;
                }
#endif
}

int
sflinit(const char *name)
{
        int i;

        for(i = 0; i < SF_MAXFILES; i++)
                sf_files[i] = 0;
        atexit(rsffinish);

        if(sizeof(DWORD) != 4 || sizeof(WORD) != 2) {
                rsferrno = ESFBADPARAM;
                rsferrstr = "internal: sizeof(WORD) != 2 or sizeof(DWORD) != 4";
                return -1;
        }
#ifdef _WIN32
//alternative is to set CDP_COM_READY entirely in shortcuts.c
#ifdef _DEBUG
        assert(!CDP_COM_READY);
#endif
        CDP_COM_READY = COMinit();         //need COM to read shortcuts
#endif
        return 0;
}

/*
 *      Misc other stuff
 */
#if 0
void
sffinish()
{
        /* leave everything to atexit! */
}
#endif
            
char *
sfgetbigbuf(int *secsize)
{
        char *mem = (char *) malloc(100*SECSIZE);

        *secsize = (mem == 0) ? 0 : 100;
        return mem;
}

void
sfperror(const char *s)
{
        if(s == 0)
                s = "sound filing system";
        if(*s != '\0')
                fprintf(stderr, "%s: %s\n", s, rsferrstr);
        else
                fprintf(stderr, "%s\n", rsferrstr);
}

char *
sferrstr(void)
{
        return rsferrstr;
}

int
sferrno(void)
{
        return rsferrno;
}

int
sfsetprefix(char *path)
{
        /* the set prefix call is simply ignored - for now */
        return 0;
}

void
sfgetprefix(char *path)
{
        path[0] = '\0';         /* signal that no prefix is set */
}

/*
 *      allocate/de-allocate file numbers
 */
static int
allocsffile(char *filename)
{
        int i;
        int first_i = -1;
/*#if defined CDP97 && defined _WIN32*/
        int refcnt98 = 1;                        //RWD incr refcnt for THIS file, if we have previously opened it
/*#endif*/
        for(i = 0; i < SF_MAXFILES; i++)
                if(sf_files[i] == 0) {
                        if(first_i < 0)
                                first_i = i;
                }

                //RWD98 excluding the return ~seems~ to be all thats needed to get multiple opens!
                else if(_stricmp(sf_files[i]->filename, filename) == 0) {/* not quite right! */
                        sf_files[i]->refcnt++;
                        refcnt98++;                //for THIS file
                        //return i;
                }

        if(first_i < 0) {
                rsferrno = ESFNOSFD;
                rsferrstr = "Too many Sfiles are open";
                free(filename);
                return -1;
        }
        if((sf_files[first_i] = ALLOC(struct sf_file)) == 0) {
                rsferrno = ESFNOMEM;
                rsferrstr = "No memory for open SFfile";
                free(filename);
                return -1;
        }

        sf_files[first_i]->refcnt = refcnt98;

        //sf_files[first_i]->refcnt = 1;          //RWD.6.98 restore this  to restore old behaviour

        sf_files[first_i]->filename = filename;
        sf_files[first_i]->props = 0;
        sf_files[first_i]->proplim = 0;
        sf_files[first_i]->curpropsize = 0;
        sf_files[first_i]->propschanged = 0;
        sf_files[first_i]->aiffchunks = 0;
        sf_files[first_i]->peaktime = 0;
        POS64(sf_files[first_i]->peakchunkoffset) = 0;
        POS64(sf_files[first_i]->factchunkoffset) = 0;
        POS64(sf_files[first_i]->datachunkoffset) = 0;
        sf_files[first_i]->peaks = NULL;
        sf_files[first_i]->bitmask = 0xffffffff;
        sf_files[first_i]->fmtchunkEx.dwChannelMask = 0;
        sf_files[first_i]->chformat = STDWAVE;
        sf_files[first_i]->min_header = SFILE_CDP;
        return first_i;
}

static void
freesffile(int i)
{
        struct property *pp = sf_files[i]->props;
        struct aiffchunk *ap = sf_files[i]->aiffchunks;

        while(pp != /* 0 */ NULL) {
                struct property *pnext = pp->next;
                free(pp->name);
                free(pp->data);
                free(pp);
                pp = pnext;
        }
        while(ap != /* 0 */ NULL) {
                struct aiffchunk *anext = ap->next;
                free(ap->buf);
                free(ap);
                ap = anext;
        }
        free(sf_files[i]->filename);
        //RWD.6.5.99
        if(sf_files[i]->peaks != NULL)
                free(sf_files[i]->peaks);

        free(sf_files[i]);
        sf_files[i] = /* 0 */ NULL;
}

#ifdef unix
#define PATH_SEP        '/'
#else
#define PATH_SEP        '\\'
#endif

//RWD: the environment var code prevents use of a defined analysis file extension
//in addition to CDP_SOUND_EXT ...

//RWD98 now declared static at top of file

/*RWD for DevCPp*/
#ifndef _MAX_PATH
#define _MAX_PATH (255)
#endif

#if 0
static enum sndfiletype
gettypefromname(const char *path)
{
#ifdef NOTDEF
        char *eos = &path[strlen(path)];        /* points to the null byte */
        char *lastsl = strrchr(path, PATH_SEP);
#else
        //RWD98: use hackable local copy of path, to check for WIN32 shortcut
        //this bit general, though
        char *eos, *lastsl;
        char copypath[_MAX_PATH];
        int len;
        copypath[0] = '\0';
        strcpy(copypath,path);
        len = strlen(copypath);
        eos =  &copypath[len];
        lastsl = strrchr(copypath,PATH_SEP);
#endif

        if(lastsl == 0)
                //abort();
                return unknown_wave;               //RWD.1.99


#ifdef _WIN32
        //it it a shortcut?
        if(_stricmp(eos-4, ".lnk")==0) {
                copypath[len-4] = '\0'; //cut away link extension
                eos -= 4;       //step past the ext, we should be left with a kosher sfilename
        }
#endif
#ifdef NOTDEF
        if(eos-4 > lastsl && _stricmp(eos-4, ".wav") == 0)
#endif
        //CDP97: recognise .ana as signifying analysis file
        if(eos-4 > lastsl && (_stricmp(eos-4, ".wav") == 0 || _stricmp(eos-4, ".ana") == 0))
                return riffwav;
        else if(eos-4 > lastsl && _stricmp(eos-4, ".aif") == 0 )
                return eaaiff;
        else if(eos-5 > lastsl && _stricmp(eos-5, ".aiff") == 0)
                return eaaiff;
        else if(eos-4 > lastsl && _stricmp(eos-4, ".afc") == 0)         //RWD.1.99 for AIF-C
                return aiffc;
        else if(eos-4 > lastsl && _stricmp(eos-4, ".aic") == 0 )
                return aiffc;
        else if(eos-5 > lastsl && _stricmp(eos-5, ".aifc") == 0)        //anyone using this?
                return aiffc;


        return unknown_wave;
}
#endif

/********* sfsy98 alternative... ********/

static enum sndfiletype
gettypefromname98(const char *path)
{
#ifdef NOTDEF
        char *eos = &path[strlen(path)];        /* points to the null byte */
        char *lastsl = strrchr(path, PATH_SEP);
#else
        //RWD98: use hackable local copy of path, to check for WIN32 shortcut
        //this bit general, though
        char *eos, *lastsl;
        char copypath[_MAX_PATH];
        int len;
        copypath[0] = '\0';
        strcpy(copypath,path);
        len = strlen(copypath);
        eos =  &copypath[len];
        lastsl = strrchr(copypath,PATH_SEP);
#endif

        if(lastsl == 0)
                //abort();
                return unknown_wave;             //RWD.1.99


#ifdef _WIN32
        //it it a shortcut?
        if(_stricmp(eos-4, ".lnk")==0) {
                copypath[len-4] = '\0'; //cut away link extension
                eos -= 4;       //step past the ext, we should be left with a kosher sfilename
        }
#endif

        if(eos-4 > lastsl && _stricmp(eos-4, ".wav") == 0)
                return riffwav;
        else if(eos-4 > lastsl && _stricmp(eos-4, ".aif") == 0)
                return eaaiff;
        else if(eos-5 > lastsl && _stricmp(eos-5, ".aiff") == 0)
                return eaaiff;
        //Recognize AIF-C files: use separate sndfiletype for this?
        else if(eos-4 > lastsl && _stricmp(eos-4,".afc") == 0)
                return aiffc;
        else if(eos-4 > lastsl && _stricmp(eos-4,".aic") == 0)
                return aiffc;
        else if(eos-5 > lastsl && _stricmp(eos-5,".aifc") == 0)
                return aiffc;
/* FILE_AMB_SUPPORT */
        else if(eos-4 > lastsl && _stricmp(eos-4, ".amb") == 0)
                return riffwav;
        else if(eos-5 > lastsl && _stricmp(eos-5, ".ambi") == 0) //RWD April 2006 was -4 !
                return riffwav;
        else if(eos-5 > lastsl && _stricmp(eos-5, ".wxyz") == 0)
                return riffwav;

        //CDP97: recognise .ana as signifying analysis file     - find out later whether wav or aiff
        /* 4:2001 added revised extensions for and evl; lose fmt and env in time */
        else if(_stricmp(eos-4, ".ana") == 0                      //analysis file
                        || _stricmp(eos-4,".fmt") == 0                    //formant file
                        || _stricmp(eos-4,".for") == 0
                        || _stricmp(eos-4,".frq") == 0                    // pitch file
                        || _stricmp(eos-4,".env") == 0                    // binary envelope
                        || _stricmp(eos-4,".evl") == 0
                        || _stricmp(eos-4,".trn") == 0 )                          // transposition file

                return cdpfile;
        return unknown_wave;
}

//if a cdpfile - what format is it?
//RWD TODO: rewrite this with error retval
static enum sndfiletype
gettypefromfile(struct sf_file *f)
{
        DWORD tag1,tag2,size;
        enum sndfiletype type = unknown_wave;

        if(read_dw_msf(&tag1, f) || read_dw_lsf(&size, f)       || read_dw_msf(&tag2,f)) {
#if defined CDP99 && defined _WIN32
                if(SetFilePointer(f->fileno,0,NULL,FILE_BEGIN)==0xFFFFFFFF)
#else
                if(fseek(f->fileno,0,SEEK_SET) < 0)
#endif
                        return unknown_wave;
        }
        else if(tag1 == TAG('R','I','F','F')  && tag2 == TAG('W','A','V','E')){
                        type = riffwav;
        }

        else if(tag1 == TAG('F','O','R','M') && tag2 == TAG('A','I','F','F')){
                        type = eaaiff;
        }
        //RWD.1.99 support aifc files as well
        else if(tag1 == TAG('F','O','R','M') && tag2 == TAG('A','I','F','C')){
                        type = aiffc;
        }

#if defined CDP99 && defined _WIN32
        if(SetFilePointer(f->fileno,0,NULL,FILE_BEGIN)==0xFFFFFFFF)
#else
        if(fseek(f->fileno,0,SEEK_SET) < 0)
#endif
                        return unknown_wave;
        return type;
}




//RWD.6.98 when tested, add shortcuts code...
static char *
mksfpath(const char *name)
{
        char *errormsg;
        char *path = _fullpath(NULL, name, 0);
    enum sndfiletype filetype = unknown_wave; //RWD 2015

        if(path == NULL) {
                rsferrno = ESFBADPARAM;
                rsferrstr = "can't find full path for soundfile - bad drive?";
//#ifdef unix
//        printf("realpath failed:errno = %d:%s\n",errno,strerror(errno));
//#endif
                return NULL;
        }

#ifdef _WIN32
        //if its a shortcut, strip off the link extension: sfopen will try normal open first
        {
                int len;
                char *eos;
                len = strlen(path);
                eos = &path[len-4];
                if(_stricmp(eos,".lnk")==0)
                        path[len-4] = '\0';
        }
#endif
/* RWD March 2014 make this optional! */
    filetype = gettypefromname98(path);

        if( filetype == unknown_wave) {
                char *newpath;
                char *ext;
                char *ext_default = "wav";
// RWD MAR 2015 we may have unset CDP_SOUND_EXT, but not removed it completely!
                if((ext = getenv("CDP_SOUND_EXT")) == NULL || strlen(ext) == 0 ) {
            //rsferrno = ESFBADPARAM;
                        //rsferrstr = "unknown sound file type - extension not set";
                        //free(path);
                        //return NULL;
                        ext = ext_default;
                }
                if(_stricmp(ext, "wav") != 0
                 &&_stricmp(ext, "aif") != 0
                 &&_stricmp(ext, "aiff") != 0
                 &&_stricmp(ext,"afc") != 0                        //Apple...
                 &&_stricmp(ext,"aic") != 0                        //Csound uses this form
                 &&_stricmp(ext,"aifc") !=0) {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "unknown sound file type - bad CDP_SOUND_EXT setting";
                        free(path);
                        return NULL;
                }

                if((newpath = (char *) malloc(strlen(path) + strlen(ext) + 2)) == 0) {
                        rsferrno = ESFNOMEM;
                        rsferrstr = "can't get memory for full path of soundfile";
                        free(path);
                        return NULL;
                }

                strcpy(newpath, path);
                strcat(newpath, ".");
                strcat(newpath, ext);
                free(path);
                path = newpath;
        }
        if((errormsg = legalfilename(path)) != 0) {
                rsferrno = ESFBADPARAM;
                rsferrstr = errormsg;
                free(path);
                return NULL;
        }
        return path;
}

/*
 *      public sf routines
 */
//RWD.6.98 TODO when tested, add all the file-sharing code
// best to #ifdef the revised function in as a block...
int
sfopen(const char *name)
{
        int i, rc;
        struct sf_file *f;
        char *sfpath;
#if defined CDP99 && defined _WIN32
        DWORD access = GENERIC_WRITE |  GENERIC_READ;     //assumeed for first open (eg for maxsamp...)
                //seems I need to set write sharing so some other process can write...
        DWORD sharing = FILE_SHARE_READ;           //for first open
#else
    char *faccess = "r+";
#endif
#ifdef _WIN32
        char newpath[_MAX_PATH];
        newpath[0] = '\0';
#endif
        if((sfpath = mksfpath(name)) == NULL)
                return -1;

        if((i = allocsffile(sfpath)) < 0)
                return -1;
        f = sf_files[i];
//#ifdef NOTDEF
        //this may not be needed after all: can't really display a file while is is being written to...
        if(f->refcnt > 1) {
# if defined CDP99 && defined _WIN32
                access = GENERIC_READ;
                sharing = FILE_SHARE_WRITE | FILE_SHARE_READ;    //repeat opens MUST allow first open to write!
# else
                //faccess = "r";   /*RWD 2010 allow this ?? */
                rsferrno = ESFNOTOPEN;
                rsferrstr = "Can't open file more than once - yet!";
                freesffile(i);
                return -1;
# endif
        }
//#endif
        f->readonly = 0;
#ifdef _WIN32
        f->is_shortcut = 0;
#endif
        //first, try normal open as rd/wr
#if defined CDP99 && defined _WIN32
        if((f->fileno = CreateFile(f->filename,access,sharing,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL)) ==  INVALID_HANDLE_VALUE)
#else
        if((f->fileno = fopen(f->filename, faccess)) == NULL)
#endif

        {
#if defined CDP99 && defined _WIN32
                DWORD w_errno = GetLastError();
#endif
                rsferrno = ESFNOTFOUND;
#if defined CDP99 && defined _WIN32
                if(w_errno != ERROR_FILE_NOT_FOUND){
#else
                if(errno != ENOENT){    //won't exist if its actually a shortcut
#endif
#if defined CDP99 && defined _WIN32
                        if(w_errno == ERROR_INVALID_NAME) {
#else
                        if(errno == EINVAL) {
#endif
                                rsferrstr = "Illegal filename";
                                freesffile(i);
                                return -1;
                        }
#if defined CDP99 && defined _WIN32
                if(w_errno != ERROR_ACCESS_DENIED) {
#else
                        if(errno != EACCES) {
#endif
                                rsferrstr = "SFile not found";
                                freesffile(i);
                                return -1;
                        }
                }
        }

/* block bnelow is ONLY for Windows */
#ifdef _WIN32
        //try a shortcut to rd/wr file...
# ifdef CDP99
        if(f->fileno == INVALID_HANDLE_VALUE){
# else
        if(f->fileno == NULL){
# endif
                if(
                        (CDP_COM_READY) &&
                        (getAliasName(f->filename,newpath))     &&
# ifdef CDP99
                        ((f->fileno = CreateFile(newpath,access,sharing,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL)) ==  INVALID_HANDLE_VALUE)
# else
                        ((f->fileno = open(newpath, _O_BINARY|_O_RDWR) ) < 0)
# endif
                        ) {
                        //good link, but still no open...
# ifdef CDP99
                DWORD w_errno = GetLastError();
                rsferrno = ESFNOTFOUND;
                if(w_errno == ERROR_INVALID_NAME) {
# else
                rsferrno = ESFNOTFOUND;
                if(errno == EINVAL) {
# endif
                                rsferrstr = "Illegal filename";
                                freesffile(i);
                                return -1;
                        }
# ifdef CDP99
                if(w_errno != ERROR_ACCESS_DENIED) {
# else
                if(errno != EACCES) {
# endif
                                rsferrstr = "SFile not found";
                                freesffile(i);
                                return -1;
                        }
                }
        }
#endif

/* "normal" file open code here */
        //must be rdonly, try normal open or shortcut
#if defined CDP99 && defined _WIN32
        if(f->fileno== INVALID_HANDLE_VALUE){
#else
        if(f->fileno==NULL){
#endif
                if(
#ifdef CDP99
                        ((f->fileno = CreateFile(f->filename, GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,NULL)) ==  INVALID_HANDLE_VALUE)
#else
                        ((f->fileno = fopen(f->filename, "r")) == NULL)
#endif
#ifdef _WIN32
                        && (!((CDP_COM_READY) || (getAliasName(f->filename,newpath)))
# ifdef CDP99
                                || ((f->fileno = CreateFile(newpath, GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,NULL)) ==  INVALID_HANDLE_VALUE)
# else
                                || ((f->fileno = open(newpath, _O_BINARY|_O_RDONLY) ) < 0)
# endif
                                )
#endif
                                ){

                        rsferrstr = "SFile not found";
                        freesffile(i);
                        return -1;
                }
                f->readonly = 1;
        }


#ifdef _WIN32
        if(strlen(newpath) >0) {
                f->filename[0] = '\0';
                f->is_shortcut = 1;
                strcpy(f->filename,newpath);      //filename will be freed eventually; don't copy pointers
        }
#endif


        switch(f->filetype = gettypefromfile(f)) {
        case riffwav:
                rc = rdwavhdr(f);
                break;
        case eaaiff:
                rc = rdaiffhdr(f);
                break;
        case aiffc:
                rc = rdaifchdr(f);
                break;
        default:
                rsferrno = ESFNOSTYPE;
                rsferrstr = "Internal error: can't find file type";
                rc = 1;
        }
        if(rc) {
                freesffile(i);
                return -1;
        }
        f->infochanged = 0;
        f->todelete = 0;
        f->sizerequested = ES_EXIST;
        f->curpos = 0;
        return i+SFDBASE;
}

//RWD.9.98 new version to control access

int
sfopenEx(const char *name, unsigned int access)
{
        int i, rc;
        struct sf_file *f;
        char *sfpath;

        char newpath[_MAX_PATH];
        newpath[0] = '\0';

        if((sfpath = mksfpath(name)) == NULL)
                return -1;

        if((i = allocsffile(sfpath)) < 0)
                return -1;
        f = sf_files[i];

        f->readonly = 0;

        f->is_shortcut = 0;

        if(access== CDP_OPEN_RDONLY){
#if defined CDP99 && defined _WIN32
                        if(((f->fileno = CreateFile(f->filename, GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,NULL)) ==  INVALID_HANDLE_VALUE)
#else
                if(((f->fileno = fopen(f->filename, "r")) == NULL)
#endif
                        && ((!CDP_COM_READY)
                                || (!getAliasName(f->filename,newpath))
#if defined CDP99 && defined _WIN32
                                || ((f->fileno = CreateFile(newpath, GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,NULL)) ==  INVALID_HANDLE_VALUE)
#else
                                || ((f->fileno = fopen(newpath, "r") ) == NULL)
#endif
                                )){

                        rsferrstr = "SFile not found";
                        freesffile(i);
                        return -1;
                }
                f->readonly = 1;
        }
        else{
        // normal open as rd/wr
#if defined CDP99 && defined _WIN32
                DWORD w_errno;
                if((f->fileno = CreateFile(f->filename, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,NULL)) ==  INVALID_HANDLE_VALUE) {
#else
                if((f->fileno = fopen(f->filename, "r+")) == NULL )      {
#endif
                        rsferrno = ESFNOTFOUND;
#if defined CDP99 && defined _WIN32
                        w_errno= GetLastError();
                        if(w_errno != ERROR_FILE_NOT_FOUND){
#else
                        if(errno != ENOENT){    //won't exist if its actually a shortcut
#endif
#if defined CDP99 && defined _WIN32
                                if(w_errno == ERROR_INVALID_NAME) {
#else
                                if(errno == EINVAL) {
#endif
                                        rsferrstr = "Illegal filename";
                                        freesffile(i);
                                        return -1;
                                }
#if defined CDP99 && defined _WIN32
                                if(w_errno != ERROR_ACCESS_DENIED) {
#else
                                if(errno != EACCES) {
#endif
                                        rsferrstr = "SFile not found";
                                        freesffile(i);
                                        return -1;
                                }
                        }
                }

                //try a shortcut to rd/wr file...
# ifdef CDP99
        if(f->fileno == INVALID_HANDLE_VALUE){
# else
        if(f->fileno == NULL){
# endif
                        if(
                                (CDP_COM_READY) &&
                                (getAliasName(f->filename,newpath))     &&
#if defined CDP99 && defined _WIN32
                                ((f->fileno = CreateFile(newpath, GENERIC_READ | GENERIC_WRITE,
                                        FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_READONLY,NULL)) ==  INVALID_HANDLE_VALUE)
#else
                                ((f->fileno = fopen(newpath, "r+") ) == NULL)
#endif
                                ) {
                                //good link, but still no open...
                                rsferrno = ESFNOTFOUND;
#if defined CDP99 && defined _WIN32
                                if(w_errno == ERROR_INVALID_NAME) {
#else
                                if(errno == EINVAL) {
#endif
                                        rsferrstr = "Illegal filename";
                                        freesffile(i);
                                        return -1;
                                }
#if defined CDP99 && defined _WIN32
                                if(w_errno != ERROR_ACCESS_DENIED) {
#else
                                if(errno != EACCES) {
#endif
                                        rsferrstr = "SFile not found";
                                        freesffile(i);
                                        return -1;
                                }
                        }
                }


#if defined CDP99 && defined _WIN32
                if(f->fileno== INVALID_HANDLE_VALUE){
#else
                if(f->fileno == NULL){
#endif
                        rsferrstr = "SFile not found";
                        freesffile(i);
                        return -1;
                }

        }
#ifdef  _WIN32
        if(strlen(newpath) >0) {
                f->filename[0] = '\0';
                f->is_shortcut = 1;
                strcpy(f->filename,newpath);      //filename will be freed eventually; don't copy pointers
        }
#endif


        switch(f->filetype = gettypefromfile(f)) {

        case riffwav:
                rc = rdwavhdr(f);
                break;
        case eaaiff:
                rc = rdaiffhdr(f);
                break;
        case aiffc:
                rc = rdaifchdr(f);
                break;
        default:
                rsferrno = ESFNOSTYPE;
                rsferrstr = "Internal error: can't find file type";
                rc = 1;
        }
        if(rc) {
                freesffile(i);
                return -1;
        }
        f->infochanged = 0;
        f->todelete = 0;
        f->sizerequested = ES_EXIST;
        f->curpos = 0;
        return i+SFDBASE;
}



static struct sf_file *
findfile(int sfd)
{
        sfd -= SFDBASE;
        if(sfd < 0 || sfd >= SF_MAXFILES || sf_files[sfd] == 0) {
                rsferrno = ESFNOTOPEN;
                rsferrstr = "soundfile descriptor does not refer to an open soundfile";
                return 0;
        }
        return sf_files[sfd];
}


static int
comparewithlist(const char *list, const char *name)
{
        size_t len = strlen(name);
        for(;;) {
                if(_strnicmp(list, name, len) == 0
                 &&(list[len] == '\0' || list[len] == ','))
                        return 1;
                if((list = strchr(list, ',')) == 0)
                        break;
                list++;
        }
        return 0;
}

#if defined CDP99 && defined _WIN32
static HANDLE doopen(const char *name, const char *origname,cdp_create_mode mode)
{
        char *ovrflg;
        HANDLE rc;
        DWORD access,sharing,attrib,w_errno;
        access = GENERIC_READ | GENERIC_WRITE;
        sharing = FILE_SHARE_READ;
        attrib = FILE_ATTRIBUTE_NORMAL;
        if(mode==CDP_CREATE_TEMPORARY){
                sharing = 0;
                attrib = FILE_ATTRIBUTE_TEMPORARY
                        | FILE_ATTRIBUTE_HIDDEN
                        | FILE_FLAG_DELETE_ON_CLOSE;
        }
        if(mode==CDP_CREATE_RDONLY)
                attrib = FILE_ATTRIBUTE_READONLY;

        if((rc = CreateFile(name, access,sharing,NULL,CREATE_NEW,attrib,NULL)) !=  INVALID_HANDLE_VALUE)
                return rc;

        w_errno = GetLastError();
        if(!(w_errno == ERROR_FILE_EXISTS || w_errno==ERROR_ALREADY_EXISTS))
                return rc;
        if(mode==CDP_CREATE_NORMAL){
                if((ovrflg = getenv("CDP_OVERWRITE_FILE")) == 0)
                        return rc;
                if(strcmp(ovrflg, "*") != 0
                        &&!comparewithlist(ovrflg, origname))
                        return rc;
                return  CreateFile(name, access,sharing,NULL,CREATE_ALWAYS,attrib,NULL);
        }
        else
                return rc;
}
#else
static FILE* doopen(const char *name, const char *origname,cdp_create_mode mode)
{
        char *ovrflg;
        FILE* fp = NULL;
        //RWD set modeflags here to allow setting a temporary file in CDP97
        //int exclmode,truncmode;
        //exclmode = (_O_BINARY|_O_RDWR|_O_CREAT|_O_EXCL );
        //truncmode = (_O_BINARY|_O_RDWR|_O_TRUNC);
        char *fmode = "w+x";
#ifdef _WIN32
        if(mode==CDP_CREATE_TEMPORARY){
                exclmode |=  /*_O_SHORT_LIVED*/_O_TEMPORARY;      //create as temporary, if poss no flush to disk
        }
#else
/* TODO: replace with mkstemp, maybe use origname as part of template? */
    /* RWD MAR 2015, need to eliminate call to tmpnam,
     * without having to alloc new memory for modifiable name for mkstemp() */
    /* only the old GUI programs (GrainMill) ask for a temporary filename, anyway... */
        //if(mode==CDP_CREATE_TEMPORARY)
        //      name = tmpnam(NULL);

#endif
        if(mode==CDP_CREATE_RDONLY){
          //exclmode = (_O_BINARY|_O_RDONLY|_O_CREAT|_O_EXCL );
          //truncmode = (_O_BINARY|_O_RDONLY|_O_TRUNC);
          fmode = "r+x";
        }

        if((fp = fopen(name,fmode ))!= NULL)
                return fp;
    if(errno != EEXIST)
                return fp;
        if((ovrflg = getenv("CDP_OVERWRITE_FILE")) == 0)
                return fp;
        if(strcmp(ovrflg, "*") != 0
         &&!comparewithlist(ovrflg, origname))
                return fp;
    // allow overwriting (I hope...) */
        return fopen(name,"w+");
}

#endif
//RWD98 BIG TODO: new function sfcreateEx(const char *name, long size, long *outsize, SFPROPS *pProps,int min_header)
//RWD.5.99 NB of course, we CANNOT write a peak chunk here, as we have no format information
//RWD.6.99 also, just to be consistent with old programs, we reject any aifc formats

/* RWD Aug 2010 hope we can eliminate this altogether! */

#ifdef NOTDEF
int
sfcreat(const char *name, int size, int *outsize)
{
        int i, rc;
        struct sf_file *f;
        char *sfpath;
    char *ext_default = "wav";
#if defined CDP99 && defined _WIN32
        DWORD w_errno;
#endif
        unsigned long freespace = getdrivefreespace(name) - LEAVESPACE;

        if((sfpath = mksfpath(name)) == NULL)
                return -1;
        //RWD.5.99 we rely on f->peaks being NULL
        if((i = allocsffile(sfpath)) < 0)
                return -1;
        f = sf_files[i];
        //RWD: this is OK, as it tells us we cannot CREATE more than one file of the same name, or one already opened
        if(f->refcnt > 1) {
                rsferrno = ESFNOTOPEN;
                rsferrstr = "Can't open file more than once - yet!";
                freesffile(i);
                return -1;
        }
#ifdef NOTDEF
        if((f->fileno = doopen(f->filename, name)) < 0) {
#endif
#ifdef CDP99
        if((f->fileno = doopen(f->filename, name,CDP_CREATE_NORMAL)) == INVALID_HANDLE_VALUE) {

#else
        if((f->fileno = doopen(f->filename, name,CDP_CREATE_NORMAL)) < 0) {
#endif
#if defined CDP99 && defined _WIN32

                w_errno = GetLastError();
                switch(w_errno){
                case ERROR_INVALID_NAME:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Illegal filename";
                        break;
                case ERROR_FILE_EXISTS:
                case ERROR_ALREADY_EXISTS:
                        rsferrno = ESFDUPFNAME;
                        rsferrstr = "Can't create SFile, already exists";
                        break;
                case ERROR_ACCESS_DENIED:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, permission denied";
                        break;
                default:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Internal error";

                }

#else
                switch(errno) {
                case EINVAL:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Illegal filename";
                        break;
                case EEXIST:
                        rsferrno = ESFDUPFNAME;
                        rsferrstr = "Can't create SFile, already exists";
                        break;
                case EACCES:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, permission denied";
                        break;
                default:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Internal error";
                }
#endif
                freesffile(i);
                return -1;
        }

        if(size < 0)
                f->sizerequested = freespace;
        else if((unsigned int)size >= freespace) {
                rsferrno = ESFNOSPACE;
                rsferrstr = "Not enough space on Disk to create sound file";
                freesffile(i);
                return -1;
        } else
                f->sizerequested = size&~1;             /*RWD Nov 2001 No 24bit support in this func, so keep ~1 */

        f->readonly = 0;
        f->header_set = 0;

        //RWD.5.99 HOPE WE WE DON'T TRY TO CREATE FROM A SHORTCUT!
//#ifdef NOTDEF
//      switch(f->filetype = gettypefromname(f->filename)) {
//#endif
        switch(f->filetype = gettypefromname98(f->filename)) {


/******* RWD.7.98 all we have to do to write a requested format is to fill in the data in f->fmtchunk
 ******* and get wrwavhdr() to read this in! *****/
                char *ext;

        case riffwav:
                rc = wrwavhdr(f);
                break;
        case eaaiff:
                rc = wraiffhdr(f);
                break;
                //RWD temporary
        case aiffc:
                rsferrno = ESFNOSTYPE;
                rsferrstr = "This version canot write AIF-C soundfiles";
                return -1;
                break;
        case cdpfile:
            /* RWD MAR 2015 added test for empty string */
                if((ext = getenv("CDP_SOUND_EXT")) == NULL || strlen(ext) == 0) {
                        //rsferrno = ESFBADPARAM;
                        //rsferrstr = "unknown sound file type - extension not set";
                        //return -1;
            ext = ext_default;
                }
                if(_stricmp(ext, "wav") == 0){
                        rc=wrwavhdr(f);
                        f->filetype = riffwav;
                }
                else if(_stricmp(ext, "aif") == 0 || _stricmp(ext, "aiff") == 0){
                        rc=wraiffhdr(f);
                        f->filetype= eaaiff;
                }

                else {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "unknown sound file type - bad CDP_SOUND_EXT setting";
                        return -1;
                }
                break;
        default:
                rsferrno = ESFNOSTYPE;
                rsferrstr = "Internal error: can't find filetype";
                rc = 1;
        }
        if(rc) {
                freesffile(i);
                return -1;
        }

        f->datachunksize = 0;
        f->infochanged = 0;
        f->todelete = 0;
        if(outsize != 0)
                *outsize = (int) f->sizerequested;
        f->curpos = 0;
        return i+SFDBASE;
}
#endif

/********* SFSY98 extension: supply format info for streaming, etc      *******/
//RWD.6.99 supports all new legal formats, except WAVE_EX (use sfcreat_ex)
//RWD.1.99 added mode arg to create temporary file in Current Directory

/*RWD 2007: change size params to __int64 */
#ifdef  FILE64_WIN
int
sfcreat_formatted(const char *name, __int64_t size, __int64_t *outsize,int channels,
                                  int srate, int stype,cdp_create_mode mode)
#else
int sfcreat_formatted(const char *name,  __int64_t size,  __int64_t *outsize,int channels,
                                  int srate, int stype,cdp_create_mode mode)

#endif
{
        int i, rc;
        struct sf_file *f;
        char *sfpath;
/* RWD March 2014 */
    char *ext_default = "wav";
/*RWD 2007 */
#ifdef FILE64_WIN
        /*unsigned long*/__int64_t freespace = getdrivefreespace(name) - LEAVESPACE;
#else
    __int64_t freespace = getdrivefreespace(name) - LEAVESPACE;
#endif
        if((sfpath = mksfpath(name)) == NULL)
                return -1;

        if((i = allocsffile(sfpath)) < 0)
                return -1;
        f = sf_files[i];
        //RWD: this is OK, as it tells us we cannot CREATE more than one file of the same name, or one already opened
        if(f->refcnt > 1) {
                rsferrno = ESFNOTOPEN;
                rsferrstr = "Can't open file more than once - yet!";
                freesffile(i);
                return -1;
        }
#if defined CDP99 && defined _WIN32
        if((f->fileno = doopen(f->filename, name,mode)) == INVALID_HANDLE_VALUE) {
                DWORD w_errno = GetLastError();
#else
        if((f->fileno = doopen(f->filename, name,mode)) == NULL) {
#endif

#if defined CDP99 && defined _WIN32
                switch(w_errno) {
                case ERROR_INVALID_NAME:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Illegal filename";
                        break;
                case ERROR_FILE_EXISTS:
                case ERROR_ALREADY_EXISTS:
                        rsferrno = ESFDUPFNAME;
                        rsferrstr = "Can't create SFile, already exists";
                        break;
                case ERROR_ACCESS_DENIED:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, permission denied";
                        break;
                default:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Internal error";
                }

#else
                switch(errno) {
                case EINVAL:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Illegal filename";
                        break;
                case EEXIST:
                        rsferrno = ESFDUPFNAME;
                        rsferrstr = "Can't create SFile, already exists";
                        break;
                case EACCES:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, permission denied";
                        break;
                default:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Internal error";
                }
#endif
                freesffile(i);
                return -1;
        }

        if(size < 0)
                f->sizerequested = freespace;
        else if(size >= freespace) {
                rsferrno = ESFNOSPACE;
                rsferrstr = "Not enough space on Disk to create sound file";
//RWD.7.99
#if defined CDP99 && defined _WIN32
                CloseHandle(f->fileno);
                DeleteFile(f->filename);
#else
                fclose(f->fileno);
                remove(f->filename);
#endif
                freesffile(i);
                return -1;
        } else
                f->sizerequested = /*size&~1*/ size;     /* RWD NOV 2001 NO ROUNDING! We have 24bit samples now! */

        f->readonly = 0;
        f->header_set = 0;

        ///RWD.6.5.99 prepare peak storage
        f->peaks = (CHPEAK *) calloc(channels, sizeof(CHPEAK));
        if(f->peaks==NULL){
                rsferrno = ESFNOMEM;
                rsferrstr = "No memory to create peak data storage";
//RWD.7.99
#if defined CDP99 && defined _WIN32
                CloseHandle(f->fileno);
                DeleteFile(f->filename);
#else
                fclose(f->fileno);
                remove(f->filename);
#endif

                freesffile(i);
                return -1;
        }


        switch(f->filetype = gettypefromname98(f->filename)) {
                char *ext;
/******* RWD.7.98 all we have to do to write a requested format is to fill in the data in f->fmtchunk
 ******* and get wrwavhdr() to read this in! *****/
        case riffwav:
                rc = wrwavhdr98(f,channels,srate,stype);
                break;
        case eaaiff:
                //make sure AIFF format is legal!
                if(stype==SAMP_2432){
                        //reject here, as can't tell caller
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "requested sample type illegal for AIFF files";
                        return -1;
                }
                if(stype==SAMP_FLOAT){
                        //we now require AIFC for float formats
                        f->filetype = aiffc;
                        rc = wraifchdr(f,channels,srate,stype);
                }
                else
                        rc = wraiffhdr98(f,channels,srate,stype);
                break;
                //RWD temporary
        case aiffc:
                //make sure AIFF format is legal!
                if(stype==SAMP_2432){
                        //reject here, as can't tell caller
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "requested sample type illegal for AIFF files";
                        return -1;
                }

                rc = wraifchdr(f,channels,srate,stype);
                break;
        case cdpfile:
            /* RWD MAR 2015 as above */
                if((ext = getenv("CDP_SOUND_EXT")) == NULL || strlen(ext) == 0) {
                        //rsferrno = ESFBADPARAM;
                        //rsferrstr = "unknown sound file type - extension not set";
//RWD.7.99
//#if defined CD/P99 && defined _WIN32
//                      CloseHandle(f->fileno);
//                      DeleteFile(f->filename);
//#else
//                      fclose(f->fileno);
//                      remove(f->filename);
//#endif
//                      return -1;
            ext = ext_default;
                }
        f->min_header = SFILE_ANAL; /*RWD Nov 2009: but we don't want PEAK, CUE for analysis files! */
        if(f->peaks){
            free(f->peaks);
            f->peaks = NULL;
        }
                if(_stricmp(ext, "wav") == 0){
                        rc = wrwavhdr98(f,channels,srate,stype);           /*RWD 5:2003*/
                        f->filetype = riffwav;
                }
                else if(_stricmp(ext, "aif") == 0 || _stricmp(ext, "aiff") == 0){
                        if(stype==SAMP_FLOAT){
                        //we now require AIFC for float formats
                                f->filetype = aiffc;
                                rc = wraifchdr(f,channels,srate,stype);
                        }
                        else{
                                rc=wraiffhdr98(f,channels,srate,stype);
                                f->filetype= eaaiff;
                        }
                }
                //RWD.1.99 temporary
                else if(_stricmp(ext, "aic") == 0
                                || _stricmp(ext, "afc") == 0
                                || _stricmp(ext, "aifc") == 0){
                        rc=wraifchdr(f,channels,srate,stype);
                        f->filetype= aiffc;
                }
                else {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "unknown sound file type - bad CDP_SOUND_EXT setting";
//RWD.7.99
#if defined CDP99 && defined _WIN32
                        CloseHandle(f->fileno);
                        DeleteFile(f->filename);
#else
                        fclose(f->fileno);
                        remove(f->filename);
#endif


                        return -1;
                }
                break;

        default:
                rsferrno = ESFNOSTYPE;
                rsferrstr = "Internal error: can't find filetype";
                rc = 1;
        }
        if(rc) {
//RWD.7.99
#if defined CDP99 && defined _WIN32
                CloseHandle(f->fileno);
                DeleteFile(f->filename);
#else
                fclose(f->fileno);
                remove(f->filename);
#endif

                freesffile(i);
                return -1;
        }

        f->datachunksize = 0;
        f->infochanged = 0;
        f->todelete = 0;
        if(outsize != 0)
                *outsize = (unsigned int) f->sizerequested;
        f->curpos = 0;
        return i+SFDBASE;
}


//special version for wave-ex
//props is both in and out

#ifdef FILE64_WIN
int
sfcreat_ex(const char *name, __int64_t size, __int64_t *outsize,SFPROPS *props,int min_header,cdp_create_mode mode)

#else
int
sfcreat_ex(const char *name, __int64_t size, __int64_t *outsize,SFPROPS *props,int min_header,cdp_create_mode mode)
#endif
{
        int i, rc;
        int stype = -1;
        struct sf_file *f;
        char *sfpath;
    char *ext_default = "wav"; /* RWD March 2014 */
        unsigned long freespace = getdrivefreespace(name) - LEAVESPACE;

        if((sfpath = mksfpath(name)) == NULL)
                return -1;
        if(min_header < SFILE_MINIMUM || min_header > SFILE_CDP){
                rsferrno =  ESFBADPARAM;
                rsferrstr = "bad min_header spec";
                return -1;
        }

        if((i = allocsffile(sfpath)) < 0)
                return -1;
        f = sf_files[i];
        //RWD: this is OK, as it tells us we cannot CREATE more than one file of the same name, or one already opened
        if(f->refcnt > 1) {
                rsferrno = ESFNOTOPEN;
                rsferrstr = "Can't open file more than once - yet!";
                freesffile(i);
                return -1;
        }
        //can only minimise header for wavefile!
        if(props->type == wt_wave)
                f->min_header = min_header;
        //reject analysis formats if not floatsams
        else{
                if(props->samptype != FLOAT32){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "Analysis data must be floats";
                        freesffile(i);
                return -1;


                }
        }

        switch(props->samptype){
        case (SHORT16):
                stype = SAMP_SHORT;
                break;
        case(FLOAT32):
                stype = SAMP_FLOAT;
                break;
        case(INT_32):
                stype = SAMP_LONG;
                break;
        case(INT2424):
                stype = SAMP_2424;
                break;
        case(INT2024):
                stype = SAMP_2024;
                break;
        case(INT2432):
                stype = SAMP_2432;
                break;
        default:
                rsferrno =  ESFBADPARAM;
                rsferrstr = "unsupported sample type";  //add speaker mask stuff ere long, if WAVE_EX
                freesffile(i);
                return -1;
                break;
        }


#if defined CDP99 && defined _WIN32
        if((f->fileno = doopen(f->filename, name,mode)) == INVALID_HANDLE_VALUE) {
                DWORD w_errno = GetLastError();
#else
        if((f->fileno = doopen(f->filename, name,mode)) == NULL) {
#endif

#if defined CDP99 && defined _WIN32
                switch(w_errno) {
                case ERROR_INVALID_NAME:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Illegal filename";
                        break;
                case ERROR_FILE_EXISTS:
                case ERROR_ALREADY_EXISTS:
                        rsferrno = ESFDUPFNAME;
                        rsferrstr = "Can't create SFile, already exists";
                        break;
                case ERROR_ACCESS_DENIED:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, permission denied";
                        break;
                default:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Internal error";
                }

#else
                switch(errno) {
                case EINVAL:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Illegal filename";
                        break;
                case EEXIST:
                        rsferrno = ESFDUPFNAME;
                        rsferrstr = "Can't create SFile, already exists";
                        break;
                case EACCES:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, permission denied";
                        break;
                default:
                        rsferrno = ESFNOTOPEN;
                        rsferrstr = "Can't create SFile, Internal error";
                }
#endif
                freesffile(i);
                return -1;
        }

        if(size < 0)
                f->sizerequested = freespace;
        else if(size >= freespace) {
                rsferrno = ESFNOSPACE;
                rsferrstr = "Not enough space on Disk to create sound file";
//RWD.7.99
#if defined CDP99 && defined _WIN32
                CloseHandle(f->fileno);
                DeleteFile(f->filename);
#else
                fclose(f->fileno);
                remove(f->filename);
#endif

                freesffile(i);
                return -1;
        } else
                f->sizerequested = (size/* + 1*/)/* &~1*/;      /*RWD use size+1) to get 16bit pad for 24bit files */

        f->readonly = 0;
        f->header_set = 0;

        ///RWD.6.5.99 prepare peak storage: wave and binary envelope are OK
        if(props->type == wt_wave || props->type== wt_binenv){
                f->peaks = (CHPEAK *) calloc(props->chans, sizeof(CHPEAK));
                if(f->peaks==NULL){
                        rsferrno = ESFNOMEM;
                        rsferrstr = "No memory to create peak data storage";
//RWD.7.99
#if defined CDP99 && defined _WIN32
                        CloseHandle(f->fileno);
                        DeleteFile(f->filename);
#else
                        fclose(f->fileno);
                        remove(f->filename);
#endif

                        freesffile(i);
                        return -1;
                }
        }

        switch(f->filetype = gettypefromname98(f->filename)) {
                char *ext;
/******* RWD.7.98 all we have to do to write a requested format is to fill in the data in f->fmtchunk
 ******* and get wrwavhdr() to read this in! *****/
        case riffwav:
                if(props->chformat >= MC_STD){
                        f->filetype = wave_ex;
                        props->format = WAVE_EX;
                        rc = wrwavex(f, props);
                }
                else {
                        props->chformat = STDWAVE;
                        props->format = WAVE;
                        rc = wrwavhdr98(f,props->chans,props->srate,stype);
                }
                break;
        //TODO: get the aiff extended formats sorted!
        case eaaiff:
                //for now, we have to IGNORE chformat requests
                props->chformat = STDWAVE;
                props->format = AIFF;
                //make sure AIFF format is legal!
                if(stype==SAMP_2432){
                        stype = SAMP_2424;
                        props->samptype = INT2424;
                }
                if(stype==SAMP_FLOAT){
                        //we now require AIFC for float formats
                        props->format = AIFC;
                        f->filetype = aiffc;
                        rc = wraifchdr(f,props->chans,props->srate,stype);
                }
                else
                        rc = wraiffhdr98(f,props->chans,props->srate,stype);
                break;
        case aiffc:
                props->format = AIFC;
                //make sure AIFF format is legal!
                if(stype==SAMP_2432){
                        stype = SAMP_2424;
                        props->samptype = INT2424;
                }

                rc = wraifchdr(f,props->chans,props->srate,stype);
                break;
        case cdpfile:
            /* RWD MAR 2015 as above */
                if((ext = getenv("CDP_SOUND_EXT")) == NULL || strlen(ext) == 0) {
//                      rsferrno = ESFBADPARAM;
//                      rsferrstr = "unknown sound file type - extension not set";
//RWD.7.99
//#if defined CDP99 && defined _WIN32
//                      CloseHandle(f->fileno);
//                      DeleteFile(f->filename);
//#else
//                      fclose(f->fileno);
//                      remove(f->filename);
//#endif
//                      return -1;
            ext = ext_default;
                }

                if(_stricmp(ext, "wav") == 0){
                        if(props->chformat > MC_STD){
                                f->filetype = wave_ex;
                                props->format = WAVE_EX;
                                rc = wrwavex(f, props);
                        }
                        else {
                                props->chformat = STDWAVE;
                                props->format = WAVE;

                                rc = wrwavhdr98(f,props->chans,props->srate,stype);
                                f->filetype = riffwav;
                        }
                }
                else if(_stricmp(ext, "aif") == 0 || _stricmp(ext, "aiff") == 0){
                        props->chformat = STDWAVE;
                        props->format = AIFF;
                        if(stype==SAMP_FLOAT){
                                //we now require AIFC for float formats
                                props->format = AIFC;
                                f->filetype = aiffc;
                                rc = wraifchdr(f,props->chans,props->srate,stype);
                        }
                        else {
                                rc = wraiffhdr98(f,props->chans,props->srate,stype);
                                f->filetype= eaaiff;
                        }
                }
                //RWD.1.99 temporary
                else if(_stricmp(ext, "aic") == 0
                        || _stricmp(ext, "afc") == 0
                        || _stricmp(ext, "aifc") == 0){
                        props->format = AIFC;
                        f->filetype = aiffc;
                        rc = wraifchdr(f,props->chans,props->srate,stype);

                }
                else {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "unknown sound file type - bad CDP_SOUND_EXT setting";
//RWD.7.99
#if defined CDP99 && defined _WIN32
                        CloseHandle(f->fileno);
                        DeleteFile(f->filename);
#else
                        fclose(f->fileno);
                        remove(f->filename);
#endif
                        return -1;
                }
                break;

        default:
                rsferrno = ESFNOSTYPE;
                rsferrstr = "Internal error: can't find filetype";
                rc = 1;
        }
        if(rc) {
//RWD.7.99
#if defined CDP99 && defined _WIN32
                CloseHandle(f->fileno);
                DeleteFile(f->filename);
#else
                fclose(f->fileno);
                remove(f->filename);
#endif
                freesffile(i);
                return -1;
        }

        f->datachunksize = 0;
        f->infochanged = 0;
        f->todelete = 0;
        if(outsize != 0)
                *outsize = f->sizerequested;
        f->curpos = 0;

        if(props->type==wt_analysis){
                /*Write all pvoc properties*/
                if(addprop(f,"original sampsize",(char *)&(props->origsize), sizeof(/*long*/int))<0){
                        rsferrstr = "Failure to write original sample size";
                        return -1;
        }
        if(addprop(f,"original sample rate",(char *)&(props->origrate),sizeof(/*long*/int))<0){
                        rsferrstr = "Failure to write original sample rate";
        }
        if(addprop(f,"arate",(char *)&(props->arate),sizeof(float)) < 0){
                        rsferrstr = "Failure to write analysis sample rate";
        }
        if(addprop(f,"analwinlen",(char *)&(props->winlen),sizeof(int)) < 0){
                        rsferrstr = "Failure to write analysis window length";
        }
        if(addprop(f,"decfactor",(char *)&(props->decfac),sizeof(int)) < 0){
                        rsferrstr = "Failure to write decimation factor";
        }
        }
        return i+SFDBASE;
}






//RWD.1.99 new func to enable current sfile (e.g temporary) to be reopened for writing
//return 1 for success, 0 for error
//new channel/sample formats are accepted, but the cdp_create_mode cannot be changed, nor the file format
//(without  alot mre jiggery-pokery...)
//BIG QUESTION: ALLOW THIS ONLY FOR TEMPORARY FILES?
//note we do not call freesfile[] here, as the file is owned externally
//current dependency: GRAINMILL
//will need sndrecreat_formatted eventually, NB set buffer size for 24bit formats!
#ifdef FILE64_WIN
int
sfrecreat_formatted(int sfd, __int64_t size, __int64_t *outsize,int channels,
                                  int srate, int stype)

#else
int
sfrecreat_formatted(int sfd, __int64_t size, __int64_t *outsize,int channels,
                                  int srate, int stype)
#endif
{
        int rc;
        struct sf_file *f;
    char *ext_default = "wav";
        __int64_t freespace;

        //might as well validate the params
        if(channels < 1 || srate <= 0)
                return 0;
        //ho hum, need stype as a typedef...
        // we're not interested in 8-bit stuff!
        if((stype >= SAMP_MASKED) || stype == SAMP_BYTE)
                return 0;
        if((f = findfile(sfd)) == 0)
                return 0;
        //RWD: this is OK, as it tells us we cannot CREATE more than one file of the same name, or one already opened
        if(f->refcnt > 1) {
                rsferrno = ESFNOTOPEN;
                rsferrstr = "Can't (re)create file more than once!";
                return 0;
        }
        if(f->readonly == 1)
                return 0;

#if defined CDP99 && defined _WIN32
        if(w_ch_size(f->fileno, 0L) < 0) {
#else
        if(ftruncate(fileno(f->fileno), 0) < 0) {
#endif
                rsferrno = ESFWRERR;
                rsferrstr = "write error resetting file";
                return 0;
        }


        freespace = getdrivefreespace(f->filename) - LEAVESPACE;
        if(size < 0)
                f->sizerequested = freespace;
        else if((unsigned long)size >= freespace) {
                rsferrno = ESFNOSPACE;
                rsferrstr = "Not enough space on Disk to create sound file";
                return 0;
        } else
                f->sizerequested = /*size&~1*/ size;  /*RWD Nov 2001: accept 24bit samples */

        f->readonly = 0;
        f->header_set = 0;

        //RWD.6.5.99 : accept PEAKS for now, but may need to forbid unless wave or binenv, as above
        if(f->peaks){
                free(f->peaks);
                f->peaks = (CHPEAK *) calloc(channels,sizeof(CHPEAK));
                if(f->peaks==NULL){
                        rsferrno = ESFNOMEM;
                        rsferrstr = "No memory for peak data";
                        return 0;
                }

        }

        //we don't change f->min_header...
        switch(f->filetype) {
                char *ext;
        case riffwav:
                rc = wrwavhdr98(f,channels,srate,stype);
                break;
        case eaaiff:
                //make sure AIFF format is legal!
                if(stype==SAMP_2432){
                        //reject here, as can't tell caller
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "requested sample type illegal for AIFF files";
                        return -1;
                }
                if(stype==SAMP_FLOAT){
                        //we now require AIFC for float formats
                        f->filetype = aiffc;
                        rc = wraifchdr(f,channels,srate,stype);
                }
                else
                        rc = wraiffhdr98(f,channels,srate,stype);
                break;

        case aiffc:
                //make sure AIFF format is legal!
                if(stype==SAMP_2432){
                        //reject here, as can't tell caller
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "requested sample type illegal for AIFF files";
                        return -1;
                }

                rc = wraifchdr(f,channels,srate,stype);
                break;
        case cdpfile:
            /* RWD MAR 2015 as above */
                if((ext = getenv("CDP_SOUND_EXT")) == NULL || strlen(ext) == 0) {
//                      rsferrno = ESFBADPARAM;
//                      rsferrstr = "unknown sound file type - extension not set";
//                      rc = 1;
            ext = ext_default;
                }
                if(_stricmp(ext, "wav") == 0){
                        rc = wrwavhdr98(f,channels,srate,stype);
                        f->filetype = riffwav;
                }
                else if(_stricmp(ext, "aif") == 0 || _stricmp(ext, "aiff") == 0){
                        if(stype==SAMP_FLOAT){
                                //we now require AIFC for float formats
                                f->filetype = aiffc;
                                rc = wraifchdr(f,channels,srate,stype);
                        }
                        else {
                                rc = wraiffhdr98(f,channels,srate,stype);
                                f->filetype= eaaiff;
                        }
                }
                //RWD.1.99 temporary
                else if(_stricmp(ext, "aic") == 0
                        || _stricmp(ext, "afc") == 0
                        || _stricmp(ext, "aifc") == 0){
                        f->filetype = aiffc;
                        rc = wraifchdr(f,channels,srate,stype);
                }
                else {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "unknown sound file type - bad CDP_SOUND_EXT setting";
                        rc = 1;
                }
                break;

        default:
                rsferrno = ESFNOSTYPE;
                rsferrstr = "Internal error: can't find filetype";
                rc = 1;
        }
        if(rc) {
                return 0;
        }

        f->datachunksize = 0;
        f->infochanged = 0;
        f->todelete = 0;
        if(outsize != 0)
                *outsize = f->sizerequested;
        f->curpos = 0;
        return 1;
}




//RWD OCT97
int
sfgetwordsize(int sfd)
{
        struct sf_file *f;

        if((f = findfile(sfd)) == 0)
                return -1;
        return f->fmtchunkEx.Format.wBitsPerSample;
}

#ifdef FILE64_WIN
__int64_t sfgetdatasize(int sfd)
#else
__int64
sfgetdatasize(int sfd)
#endif
{
        struct sf_file *f;

        if((f = findfile(sfd)) == 0)
                return -1;
        return f->datachunksize;
}

#if defined CDP97 && defined _WIN32
int sf_is_shortcut(int sfd,char *name)
{
        struct sf_file *f;
        if((f = findfile(sfd)) == 0)
                return -1;
        if(f->is_shortcut){
                if(name != NULL)
                        strcpy(name,f->filename);
        }
        return f->is_shortcut;
}
#endif

/*RWD: nb cnt arg is seen as count of BYTES to fill (expecting cnt/sizeof(sample) words);
 f->curpos expected to contain bytes of SHORTS or FLOATS
*/
int
sfread(int sfd, char *buf, int cnt)
{
        struct sf_file *f;
        short *sp;
        DWORD *dp;
        __int64_t remain;
        int i;
        //int got = 0;

        if((f = findfile(sfd)) == 0)
                return -1;

        cnt = cnt & ~(SECSIZE-1);
        /*RWD OCT97: here,  remain IS size-specific, so curpos must be, too*/
        if((remain = (int)(f->datachunksize - f->curpos)) < 0)
                remain = 0;


        if(cnt > remain)
                cnt = remain;
        if(cnt == 0)
                return 0;
        if(f->fmtchunkEx.Format.wBitsPerSample == 8)
                cnt /= 2;                                               /*see below...*/
        if(doread(f, buf, cnt)) {                       /*bytes, bytecnt*/
                rsferrno = ESFRDERR;
                rsferrstr = "Read error";
                return -1;
        }

        switch(f->fmtchunkEx.Format.wBitsPerSample) {
        case 8:
                if(f->filetype == riffwav || f->filetype ==wave_ex) {
                        for(i = cnt-1; i >= 0; i--)
                                ((short *)buf)[i] = (buf[i]-128)<<8;
                } else if((f->filetype == eaaiff) || (f->filetype==aiffc)) {            //RWD.1.99
                        for(i = cnt-1; i >= 0; i--)
                                ((short *)buf)[i] = ((signed char *)buf)[i];
                } else
                        abort();                                 //RWD ouch!
                cnt *= 2;                                        // restored from above
                break;
        case 16:
                if(REVDATAINFILE(f)) {
                        sp = (short *)buf;
                        for(i = cnt/sizeof(short); i > 0; i--) {
                                *sp = REVWBYTES(*sp);
                                sp++;
                        }
                }
                break;
        case 32:
                if(REVDATAINFILE(f)) {
                        dp = (DWORD *)buf;
                        for(i = cnt/sizeof(DWORD); i > 0; i--) {
                                *dp = REVDWBYTES(*dp);
                                dp++;
                        }
                }
                break;
        default:
//              abort();                                                 // ouch again!
                //RW.6.99
                rsferrno = ESFBADPARAM;
                rsferrstr = "cannot read unsupported sample type";
                return -1;

        }
        f->curpos += cnt;               //assumes pos in SHORTS or FLOATS buffer
        return cnt;
}
//RWD: the original func - no support for new formats, but must eliminate abort() call!
/* RWD 2007 NB : these funcs all return -1 for error, so are forced to handle only signed longs */
/* therefore: NOT ready for 4GB files ! */
int
sfwrite(int sfd, char *outbuf, int cnt)
{
        struct sf_file *f;
        int i;
    __int64_t remain;
        short *ssp, *sdp;
        DWORD *dsp, *ddp;
        char *buf = outbuf;

        if((f = findfile(sfd)) == 0)
                return -1;

        if(f->readonly) {
                rsferrno = ESFREADONLY;
                rsferrstr = "Can't write to read only file";
                return -1;
        }

        if(f->fmtchunkEx.Format.wBitsPerSample == 8) {
                rsferrno = ESFREADONLY;
                rsferrstr = "Can't write to 8bits/sample files";
                return -1;
        }

        cnt = cnt & ~(SECSIZE-1);
        if(f->sizerequested >= 0) {                     /* creating file - explicit size */
                if((remain = f->sizerequested - f->curpos) < 0)
                        remain = 0;
                if(cnt > (int) remain)
                        cnt = (int) remain;
        } else if(f->sizerequested == ES_EXIST) {       /* existing file - can't change size */
                if((remain = f->datachunksize - f->curpos) < 0)
                        remain = 0;
                if(cnt > (int) remain)
                        cnt = (int) remain;
        }

        if(cnt == 0)
                return 0;
        if(REVDATAINFILE(f)) {
                if((buf = (char *) malloc(cnt)) == 0) {
                        rsferrno = ESFWRERR;
                        rsferrstr = "Write error: can't allocate byte swap buffer";
                        return -1;
                }
                switch(f->fmtchunkEx.Format.wBitsPerSample) {
                case 16:
                        ssp = (short *)outbuf;
                        sdp = (short *)buf;
                        for(i = cnt/sizeof(short); i > 0; i--) {
                                *sdp = REVWBYTES(*ssp);
                                ssp++;
                                sdp++;
                        }
                        break;
                case 32:
                        dsp = (DWORD *)outbuf;
                        ddp = (DWORD *)buf;
                        for(i = cnt/sizeof(DWORD); i > 0; i--) {
                                *ddp = REVDWBYTES(*dsp);
                                dsp++;
                                ddp++;
                        }
                        break;
                default:
#ifdef NOTDEF
                        abort();
#endif
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "cannot write unsupported sample format";
                        return -1;



                }
        }
                                                        /* else creating file - max size */
        if(dowrite(f, buf, cnt)) {
                rsferrno = ESFWRERR;
                rsferrstr = "Write error";
                if(buf != outbuf)
                        free(buf);
                return -1;
        }
        f->curpos += cnt;
        if(f->curpos > f->datachunksize) {
                f->datachunksize = f->curpos;
                f->infochanged = 1;
        }
        if(buf != outbuf)
                free(buf);
        return cnt;
}

/* RWD: OBSOLETE - NOT IN USE NOW! */
int
sfseek(int sfd, int dist, int whence)
{
    struct sf_file *f;
    unsigned int newpos = 0u;
    unsigned int size;
    fpos_t bytepos;

    if((f = findfile(sfd)) == 0)
        return -1;
    /*RWD 2007 added casts to silence compiler! */
    size = (unsigned int) sfsize(sfd);              /* NB Can't fail! */  //RWD OCT97: NB: assumes file of SHORTS or FLOATS
    switch(whence) {
    case 0:
                newpos = dist;
                break;
        case 1:
                newpos = f->curpos + dist;
                break;
        case 2:
                newpos = size + dist;
                break;
        default:
                rsferrno = ESFBADPARAM;
                rsferrstr = "illegal whence value in sfseek";
                break;
        }
    /* RWD MAR 2015 just to eliminate compiler warning */
        //if(newpos < 0)
        //      newpos = 0;
        if(newpos > size)
                newpos = size;

        newpos &= ~(SECSIZE-1);
        f->curpos = newpos;             //still size-specific here...

//RWD OCT97 must seek correctly in 8bit files
        if(f->fmtchunkEx.Format.wBitsPerSample==8)
                newpos /= 2;



#if defined _WIN32
    newpos += f->datachunkoffset;
        if(SetFilePointer(f->fileno, newpos,NULL, FILE_BEGIN) != newpos) {
#else
    POS64(bytepos) = newpos + POS64(f->datachunkoffset);
        if(fsetpos(f->fileno, &bytepos) ) {
#endif
                rsferrno = ESFRDERR;                      //RWD CDP97
                rsferrstr = "Seek error";
                return -1;
        }
        return f->curpos;
}

static char * REV3BYTES(char *samp_24){
        //trick here: just exchange the outer bytes!
        char temp = samp_24[0];
        *samp_24 = samp_24[2];
        samp_24[2] = temp;
        return samp_24;
}
/* special buffered sf_routines for new sample sizes */
int
sfread_buffered(int sfd, char *buf, int lcnt)
{
    struct sf_file *f;
    short *sp;
    DWORD *dp;
#ifdef FILE64_WIN
    __int64_t remain,i;
    __int64_t cnt = lcnt;  /*RWD 2007: lcnt used some places below */
    long containersize;
#else
    __int64_t remain;
     __int64_t cnt = lcnt;
    int i,containersize;
#endif

        //int got = 0;

        if((f = findfile(sfd)) == 0)
                return -1;

        //RWD OCT97: here,  remain IS size-specific, so curpos must be, too
        if((remain = (__int64) (f->datachunksize - f->curpos)) < 0)
                remain = 0;

#ifdef FILE64_WIN
        if((__int64) cnt > remain)
                cnt = (unsigned int) remain;
#else
        if(cnt > remain)
                cnt =  remain;
#endif
        if(cnt == 0)
                return 0;
        if(f->fmtchunkEx.Format.wBitsPerSample == 8)
                cnt /= 2;                                               //see below...
        if(doread(f, buf, (int) cnt)) {                 //bytes, bytecnt
                rsferrno = ESFRDERR;
                rsferrstr = "Read error";
                return -1;
        }
        containersize = 8 * (f->fmtchunkEx.Format.nBlockAlign /         f->fmtchunkEx.Format.nChannels);
        switch(containersize) {
        case 8:
                if(f->filetype == riffwav || f->filetype ==wave_ex) {
                        for(i = cnt-1; i >= 0; i--)
                                ((short *)buf)[i] = (buf[i]-128)<<8;
                } else if((f->filetype == eaaiff) || (f->filetype==aiffc)) {            //RWD.1.99
                        for(i = cnt-1; i >= 0; i--)
                                ((short *)buf)[i] = ((signed char *)buf)[i];
                } else
                        abort();                                 //RWD ouch!
                cnt *= 2;                                        // restored from above
                break;
        case 16:
                if(REVDATAINFILE(f)) {
                        sp = (short *)buf;
                        for(i = cnt/sizeof(short); i > 0; i--) {
                                *sp = REVWBYTES(*sp);
                                sp++;
                        }
                }
                break;

        case(24):
                if(REVDATAINFILE(f)) {
                        char *p_byte = buf;
                        for(i = cnt/3; i > 0; i--) {
                                p_byte = REV3BYTES(p_byte);
                                p_byte += 3;
                        }
                }
                break;

        case 32:
                if(REVDATAINFILE(f)) {
                        dp = (DWORD *)buf;
                        for(i = cnt/sizeof(DWORD); i > 0; i--) {
                                *dp = REVDWBYTES(*dp);
                                dp++;
                        }
                }
                break;
        default:
#ifdef NOTDEF
                abort();                                                 // ouch again!
#endif
                rsferrno = ESFBADPARAM;
                rsferrstr = "unsupported sample format";
                return -1;


        }
        f->curpos += (DWORD) cnt;               //assumes pos in SHORTS or FLOATS buffer
        return (int) cnt;
}
//RWD.7.99 TODO: use blockalign to decide what size word to write
int
sfwrite_buffered(int sfd, char *outbuf, int lcnt)
{
    struct sf_file *f;

        //unsigned int remain, cnt = lcnt;  /* RWD Feb 2010 */
    __int64_t remain;
    __int64_t cnt = lcnt;

        short *ssp, *sdp;
        DWORD *dsp, *ddp;
        char *buf = outbuf;
        int containersize;

        if((f = findfile(sfd)) == 0)
                return -1;

        if(f->readonly) {
                rsferrno = ESFREADONLY;
                rsferrstr = "Can't write to read only file";
                return -1;
        }

        if(f->fmtchunkEx.Format.wBitsPerSample == 8) {
                rsferrno = ESFREADONLY;
                rsferrstr = "Can't write to 8bits/sample files";
                return -1;
        }

        //cnt = cnt & ~(SECSIZE-1);
        if(f->sizerequested >= 0) {                     /* creating file - explicit size */
                if((remain = f->sizerequested - f->curpos) < 0)
                        remain = 0;
                if(cnt > remain)
                        cnt = remain;
        } else if(f->sizerequested == ES_EXIST) {       /* existing file - can't change size */
                if((remain = f->datachunksize - f->curpos) < 0)
                        remain = 0;
                if(cnt > remain)
                        cnt = remain;
        }

        if(cnt == 0)
                return 0;

        containersize = 8 * (f->fmtchunkEx.Format.nBlockAlign /         f->fmtchunkEx.Format.nChannels);

        if(REVDATAINFILE(f)) {
        int i;  /* RWD Feb 2010: should be OK as signed; too risky unsigned in a loop! */
                if((buf = (char *) malloc((size_t) cnt)) == 0) {  /*RWD 2007 */
                        rsferrno = ESFWRERR;
                        rsferrstr = "Write error: can't allocate byte swap buffer";
                        return -1;
                }
                switch(containersize) {
                        char *p_byte;
                        char *p_buf;
                case 16:
                        ssp = (short *)outbuf;
                        sdp = (short *)buf;
                        for(i = cnt/sizeof(short); i > 0; i--) {
                                *sdp = REVWBYTES(*ssp);
                                ssp++;
                                sdp++;
                        }
                        break;
                case 32:
                        dsp = (DWORD *)outbuf;
                        ddp = (DWORD *)buf;
                        for(i = cnt/sizeof(DWORD); i > 0; i--) {
                                *ddp = REVDWBYTES(*dsp);
                                dsp++;
                                ddp++;
                        }
                        break;
                //case 20:
                case 24:
                        p_byte = outbuf;
                        p_buf = buf;
                        for(i= cnt/3; i > 0; i--){
                                p_buf[0] = p_byte[2];
                                p_buf[1] = p_byte[1];
                                p_buf[2] = p_byte[0];

                                p_byte += 3;
                                p_buf += 3;
                        }

                        break;
                default:
                        //abort();
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "unsupported sample format";
                        if(buf != outbuf)
                                free(buf);
                        return -1;
                        break;
                }
        }
                                                        /* else creating file - max size */
        if(dowrite(f, buf, (int) cnt)) {
                rsferrno = ESFWRERR;
                rsferrstr = "Write error";
                if(buf != outbuf)
                        free(buf);
                return -1;
        }
        f->curpos += (DWORD) cnt;
        if(f->curpos > (DWORD) f->datachunksize) {
                f->datachunksize = f->curpos;
                f->infochanged = 1;
        }
        if(buf != outbuf)
                free(buf);
        return (int) cnt;
}



#ifdef FILE64_WIN
/* RWD 2007 remember dist can be negative... */
__int64
sfseek_buffered(int sfd, __int64_t dist, int whence)
{
        struct sf_file *f;
        /*unsigned long*/__int64_t newpos = 0;
        __int64_t i64size;
        LARGE_INTEGER pos64;

        if((f = findfile(sfd)) == 0)
                return -1;

        i64size = sfsize(sfd);          /* NB Can't fail! */  //RWD OCT97: NB: assumes file of SHORTS or FLOATS

        switch(whence) {
        case 0:
                newpos = dist;
                break;
        case 1:
                newpos = (__int64) f->curpos + dist;
                break;
        case 2:
                newpos =  i64size + dist;
                break;
        default:
                rsferrno = ESFBADPARAM;
                rsferrstr = "illegal whence value in sfseek";
                break;
        }
        if(newpos < 0)
                newpos = 0;
        if(newpos > i64size)
                newpos = i64size;

        f->curpos = (unsigned int) newpos;              //still size-specific here...

//RWD OCT97 must seek correctly in 8bit files
        if(f->fmtchunkEx.Format.wBitsPerSample==8)
                newpos /= 2;

        newpos += f->datachunkoffset;
//#endif

#if defined _WIN32
        pos64.QuadPart = newpos;


        pos64.LowPart = SetFilePointer(f->fileno, pos64.LowPart,&pos64.HighPart, FILE_BEGIN);
        if(pos64.LowPart==0xFFFFFFFF && GetLastError() != NO_ERROR){
                 /* != newpos) { */
                rsferrno = ESFRDERR;                      //RWD CDP97
                rsferrstr = "Seek error";
                return -1;
        }
        if(pos64.QuadPart != newpos){
                rsferrno = ESFRDERR;
                rsferrstr = "Seek error";
                return -1;
        }
#else
        if(lseek(f->fileno, newpos, SEEK_SET) != newpos) {
                rsferrno = ESFRDERR;                      //RWD CDP97
                rsferrstr = "Seek error";
                return -1;
        }
#endif
        return f->curpos;
}


#else
__int64
sfseek_buffered(int sfd, __int64_t dist, int whence)
{
        struct sf_file *f;
        __int64_t newpos = 0;    // allow max seek distance 2GB
        __int64_t size;
    fpos_t bytepos;

        if((f = findfile(sfd)) == 0)
                //return -1;
        return 0xFFFFFFFF;
        size = sfsize(sfd);             /* NB Can't fail! */  //RWD OCT97: NB: assumes file of SHORTS or FLOATS
        switch(whence) {
        case 0:
                newpos = dist;
                break;
        case 1:
                newpos = f->curpos + dist;
                break;
        case 2:
                newpos = size + dist;
                break;
        default:
                rsferrno = ESFBADPARAM;
                rsferrstr = "illegal whence value in sfseek";
                break;
        }
        if(newpos < 0)
                newpos = 0;
        if(newpos > size)
                newpos = size;

        f->curpos = (DWORD) newpos;             //still size-specific here...

//RWD OCT97 must seek correctly in 8bit files
        if(f->fmtchunkEx.Format.wBitsPerSample==8)
                newpos /= 2;

        //

//#endif

#if defined _WIN32
    newpos += f->datachunkoffset;
        if(SetFilePointer(f->fileno, newpos,NULL, FILE_BEGIN) != newpos) {
#else
    POS64(bytepos) = (DWORD) newpos + POS64(f->datachunkoffset);
        if(fsetpos(f->fileno, &bytepos)) {
#endif
                rsferrno = ESFRDERR;                      //RWD CDP97
                rsferrstr = "Seek error";
                return -1;

        }
        return (__int64) f->curpos;
}

#endif


//RWD: this may not be sufficient: we may want to distinguish betweeen
// float sfiles and analfiles....
static int
wavupdate(struct sf_file *f)
{
    unsigned long seekdist;
    fpos_t bytepos;
        switch(f->fmtchunkEx.Format.wBitsPerSample) {
        case 8:
                f->fmtchunkEx.Format.nAvgBytesPerSec = f->fmtchunkEx.Format.nBlockAlign = f->fmtchunkEx.Format.nChannels;
                f->datachunksize /= 2;
                break;
        case 16:
                f->fmtchunkEx.Format.nAvgBytesPerSec = f->fmtchunkEx.Format.nBlockAlign = f->fmtchunkEx.Format.nChannels * 2;
                break;
        case 32:
                f->fmtchunkEx.Format.nAvgBytesPerSec = f->fmtchunkEx.Format.nBlockAlign = f->fmtchunkEx.Format.nChannels * 4;
                f->fmtchunkEx.Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;               //RWD 07:97
                //RWD TODO: update 'fact' chunk with num-samples
                break;
        default:
                abort();
        }
        f->fmtchunkEx.Format.nAvgBytesPerSec *= f->fmtchunkEx.Format.nSamplesPerSec;

        //add space for fact chunk ?
        //WARNING: THIS HAS NOT BEEN FULLY TESTED!
        // maxsamp needed this...writes directly to existing header on disk
        //f->extrachunksizes = 3*sizeof(DWORD);

        f->mainchunksize = POS64(f->datachunkoffset) + (DWORD) f->datachunksize - 2*sizeof(DWORD) + f->extrachunksizes;
#if defined _WIN32
        if(SetFilePointer(f->fileno, 4L,NULL, FILE_BEGIN)== 0xFFFFFFFF
#else
        if(fseek(f->fileno, 4L, SEEK_SET) < 0
#endif
         || write_dw_lsf(f->mainchunksize, f)
#if defined _WIN32
         || SetFilePointer(f->fileno, f->fmtchunkoffset,NULL, FILE_BEGIN)== 0xFFFFFFFF
     || write_w_lsf(f->fmtchunkEx.Format.wFormatTag, f)
     || write_w_lsf(f->fmtchunkEx.Format.nChannels, f)
     || write_dw_lsf(f->fmtchunkEx.Format.nSamplesPerSec, f)
     || write_dw_lsf(f->fmtchunkEx.Format.nAvgBytesPerSec, f)
     || write_w_lsf(f->fmtchunkEx.Format.nBlockAlign, f)
     || write_w_lsf(f->fmtchunkEx.Format.wBitsPerSample, f)){
                rsferrno = ESFWRERR;
                rsferrstr = "Write error: Can't update format data";
                return -1;
        }
#else
         || fsetpos(f->fileno, &f->fmtchunkoffset)
         || write_w_lsf(f->fmtchunkEx.Format.wFormatTag, f)
         || write_w_lsf(f->fmtchunkEx.Format.nChannels, f)
         || write_dw_lsf(f->fmtchunkEx.Format.nSamplesPerSec, f)
         || write_dw_lsf(f->fmtchunkEx.Format.nAvgBytesPerSec, f)
         || write_w_lsf(f->fmtchunkEx.Format.nBlockAlign, f)
         || write_w_lsf(f->fmtchunkEx.Format.wBitsPerSample, f)){
                rsferrno = ESFWRERR;
                rsferrstr = "Write error: Can't update format data";
                return -1;
        }
#endif
         //RWD OCT97: the extra cbSize field SHOULD be there, = 0
         //fact chunk contains size in samples
        if(f->fmtchunkEx.Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT){
                if(POS64(f->factchunkoffset) > 0){        /*RWD 01:2004 */
#if defined _WIN32
                        if(SetFilePointer(f->fileno,f->factchunkoffset,NULL,FILE_BEGIN)== 0xFFFFFFFF
#else
                        if(fsetpos(f->fileno,&f->factchunkoffset)
#endif
                                ||write_dw_lsf((DWORD)(f->datachunksize / (f->fmtchunkEx.Format.wBitsPerSample / sizeof(char))),f)){  /*RWD 2007 */
                                rsferrno = ESFWRERR;
                                rsferrstr = "Write error: Can't update fact chunk for floatsam data";
                                return -1;
                        }
                }
        }

#if defined _WIN32
    if(SetFilePointer(f->fileno, f->datachunkoffset - sizeof(DWORD),NULL, FILE_BEGIN)== 0xFFFFFFFF
#else
    seekdist = POS64(f->datachunkoffset) - sizeof(DWORD);
    POS64(bytepos) = seekdist;
    if(fsetpos(f->fileno, &bytepos)
#endif
         ||write_dw_lsf((DWORD) f->datachunksize, f) ) {   /*RWD 2007 */
                rsferrno = ESFWRERR;
                rsferrstr = "Write error: Can't update data size";
                return -1;
        }
        return 0;
}

 /******* SFSY98 VERSION*******
  ******* main format data already there - just update durations
  */

//RWD.5.99 TODO: need to be more clever with extended formats:
//must distinguish 32bit int and float
//this is used when header has already been set
static int
wavupdate98(time_t thistime, struct sf_file *f)
{
    fpos_t bytepos;
        if(! f->header_set)
                return -1;

        if(f->fmtchunkEx.Format.wBitsPerSample == 8)
                f->datachunksize /= 2;

        f->mainchunksize = POS64(f->datachunkoffset) + (DWORD) f->datachunksize - 2*sizeof(DWORD) + f->extrachunksizes;
#if defined _WIN32
        if(SetFilePointer(f->fileno, 4L,NULL, FILE_BEGIN)== 0xFFFFFFFF
#else
        if(fseek(f->fileno, 4L, SEEK_SET) < 0
#endif
                                ||write_dw_lsf(f->mainchunksize, f)){
                rsferrno = ESFWRERR;
                rsferrstr = "SFSY98: Write error: Can't update datachunk size";
                return -1;
        }
#ifdef NOTDEF
        if(f->infochanged){
#ifdef _WIN32
                if(SetFilePointer(f->fileno, f->fmtchunkoffset,NULL, FILE_BEGIN)== 0xFFFFFFFF
#else
                if(fsetpos(f->fileno, &f->fmtchunkoffset)
#endif
                                ||write_w_lsf(f->fmtchunkEx.Format.wFormatTag, f)
                                ||write_w_lsf(f->fmtchunkEx.Format.nChannels, f)
                                ||write_dw_lsf(f->fmtchunkEx.Format.nSamplesPerSec, f)
                                ||write_dw_lsf(f->fmtchunkEx.Format.nAvgBytesPerSec, f)
                                ||write_w_lsf(f->fmtchunkEx.Format.nBlockAlign, f)
                                ||write_w_lsf(f->fmtchunkEx.Format.wBitsPerSample, f)){
                        rsferrno = ESFWRERR;
                        rsferrstr = "SFSYS98: Write error: Can't update chunk sizes or info chunk";
                        return -1;
                }
        }
#endif
        if(f->fmtchunkEx.Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT){
         //fact chunk contains size in samples
                //RWD.5.99 may not be using it...
                if(POS64(f->factchunkoffset) > 0){
#if defined _WIN32
                        if(SetFilePointer(f->fileno,f->factchunkoffset,NULL,FILE_BEGIN)== 0xFFFFFFFF
#else
                        if(fsetpos(f->fileno,&f->factchunkoffset)
#endif
                                || write_dw_lsf((DWORD)(f->datachunksize / (f->fmtchunkEx.Format.wBitsPerSample / sizeof(char))),f))    {
                                rsferrno = ESFWRERR;
                                rsferrstr = "SFSYS98: Write error: Can't update fact chunk for floatsam file";
                                return -1;
                        }
                }
        }

        //RWD.6.5.99 update peak chunk

        if((f->min_header >= SFILE_PEAKONLY) && f->peaks){
#if defined _WIN32
                if(SetFilePointer(f->fileno,f->peakchunkoffset + sizeof(DWORD),NULL,FILE_BEGIN)== 0xFFFFFFFF
#else
        fpos_t target = f->peakchunkoffset;
           POS64(target) += sizeof(DWORD);
                //if(lseek(f->fileno,f->peakchunkoffset + sizeof(DWORD),SEEK_SET) < 0
        if(fsetpos(f->fileno,&target)
#endif
                        || write_dw_lsf((DWORD) thistime,f)
                        || write_peak_lsf(f->fmtchunkEx.Format.nChannels,f)) {

                        rsferrno = ESFWRERR;
                        rsferrstr = "SFSYS98: Write error: Can't update peak chunk";
                        return -1;
                }

        }




#if defined _WIN32
         if(SetFilePointer(f->fileno, f->datachunkoffset - sizeof(DWORD),NULL, FILE_BEGIN)== 0xFFFFFFFF
        ||write_dw_lsf((DWORD) f->datachunksize, f) ) {
#else
    bytepos = f->datachunkoffset;
    POS64(bytepos) -= sizeof(DWORD);
    //if(lseek(f->fileno, f->datachunkoffset - sizeof(DWORD), SEEK_SET) < 0
        //              ||write_dw_lsf((DWORD) f->datachunksize, f) ) {
    if(fsetpos(f->fileno, &bytepos)
                        ||write_dw_lsf((DWORD) f->datachunksize, f) ) {
#endif
                rsferrno = ESFWRERR;
                rsferrstr = "SFSYS98: Write error: Can't update chunk sizes or info chunk";
                return -1;
        }
        return 0;
}


static int
aiffupdate(time_t thistime,struct sf_file *f)
{
        DWORD numsampleframes;
    fpos_t bytepos;
        struct aiffchunk *ap = f->aiffchunks;
        int commafterdata = 0;

        switch(f->fmtchunkEx.Format.wBitsPerSample) {
        case 8:
                f->datachunksize /= 2;
                numsampleframes = (DWORD) f->datachunksize;   /* RWD 2007 added DWORD casts */
                break;
        case 16:
                numsampleframes = (DWORD) f->datachunksize / 2;
                break;
        case 32:
                numsampleframes = (DWORD) f->datachunksize / 4;
                break;
        default:
                abort();
        }
        numsampleframes /= f->fmtchunkEx.Format.nChannels;

        f->mainchunksize = sizeof(DWORD) + 2*sizeof(DWORD) + 26 + 2*sizeof(DWORD) + (DWORD)(f->datachunksize + f->extrachunksizes);

#if defined _WIN32
        if(SetFilePointer(f->fileno,4L,NULL,FILE_BEGIN) == 0xFFFFFFFF
       ||write_dw_msf((DWORD) POS64(f->mainchunksize), f)
       ||SetFilePointer(f->fileno,(long)(POS64(f->datachunkoffset) - 3*sizeof(DWORD)),NULL,FILE_BEGIN) == 0xFFFFFFFF
       ||write_dw_msf((DWORD)(f->datachunksize + 2*sizeof(DWORD)), f) ) {
                rsferrno = ESFWRERR;
                rsferrstr = "Write error: Can't update main or data chunk size";
                return -1;
        }
#else
    bytepos = f->datachunkoffset;
    POS64(bytepos) -= 3 * sizeof(DWORD);
        if(fseek(f->fileno, 4L, SEEK_SET)
         ||write_dw_msf((DWORD) f->mainchunksize, f)
         ||fsetpos(f->fileno, &bytepos)
         ||write_dw_msf((DWORD)(f->datachunksize + 2*sizeof(DWORD)), f) ) {
                rsferrno = ESFWRERR;
                rsferrstr = "Write error: Can't update main or data chunk size";
                return -1;
        }
#endif
        if(POS64(f->fmtchunkoffset) > POS64(f->datachunkoffset)) {
                commafterdata++;
                POS64(f->fmtchunkoffset) = POS64(f->datachunkoffset) + (DWORD)( (f->datachunksize+1)&~1);
        }
        //RWD NB no support for AIFC here...
#ifdef NOTDEF
        else
                f->fmtchunkoffset -= 2*sizeof(DWORD);
#endif

#if defined _WIN32
        if(SetFilePointer(f->fileno,f->fmtchunkoffset,NULL,FILE_BEGIN) == 0xFFFFFFFF){
#else
        if(fsetpos(f->fileno, &f->fmtchunkoffset) ) {
#endif
                rsferrno = ESFWRERR;
                rsferrstr = "Write error: Can't seek to COMM chunk";
                return -1;
        }

        if(
#ifdef NOTDEF
                write_dw_msf(TAG('C','O','M','M'), f)
         ||write_dw_msf(18, f)
         ||
#endif
           write_w_msf(f->fmtchunkEx.Format.nChannels, f)
         ||write_dw_msf(numsampleframes, f)
         ||write_w_msf(f->fmtchunkEx.Format.wBitsPerSample, f)
         ||write_dw_toex(f->fmtchunkEx.Format.nSamplesPerSec, f) ) {
                rsferrno = ESFWRERR;
                rsferrstr = "Write error: Can't update COMM chunk";
                return -1;
        }


                //RWD.6.5.99 update peak chunk

        if((f->min_header >= SFILE_PEAKONLY) && f->peaks){
#if defined _WIN32
                if(SetFilePointer(f->fileno,f->peakchunkoffset + sizeof(DWORD),NULL,FILE_BEGIN)== 0xFFFFFFFF
#else
           bytepos = f->peakchunkoffset;
           POS64(bytepos) += sizeof(DWORD);
                if(fsetpos(f->fileno,&bytepos)
#endif
                        || write_dw_msf((DWORD) thistime,f)
                        || write_peak_msf(f->fmtchunkEx.Format.nChannels,f)) {

                        rsferrno = ESFWRERR;
                        rsferrstr = "SFSYS98: Write error: Can't update peak chunk";
                        return -1;
                }

        }


        if(!commafterdata) {
#if defined _WIN32
                if(SetFilePointer(f->fileno,(long)(f->datachunkoffset + (f->datachunksize+1)&~1),NULL,FILE_BEGIN)==0xFFFFFFFF) {
#else
            bytepos = f->datachunkoffset;
            POS64(bytepos) += (f->datachunksize+1)&~1;
                if(fsetpos(f->fileno, &bytepos) ) {
#endif
                        rsferrno = ESFWRERR;
                        rsferrstr = "Can't seek to end, to update extra chunks";
                        return -1;
                }
        }

        for(; ap != 0; ap = ap->next) {
                if(POS64(ap->offset) < POS64(f->datachunkoffset))
                        continue;
                if(write_dw_msf(ap->tag, f)
                 ||write_dw_msf(ap->size, f)
                 ||dowrite(f, ap->buf, (ap->size+1)&~1) ) {
                        rsferrno = ESFWRERR;
                        rsferrstr = "Can't update extra chunk";
                        return -1;
                }
        }
        return 0;
}


static int
aiffupdate98(time_t thistime,struct sf_file *f)
{
    DWORD numsampleframes = 0;
#ifdef linux
    fpos_t bytepos = {0};
#else
  fpos_t bytepos = 0;
#endif
    struct aiffchunk *ap = f->aiffchunks;
    int commafterdata = 0;

    if(! f->header_set)
        return -1;              
    if(f->infochanged){
        switch(f->fmtchunkEx.Format.wBitsPerSample) {
            case 8:
        f->datachunksize /= 2;
        numsampleframes = (DWORD) f->datachunksize;
        break;
      case 16:
                        numsampleframes = (DWORD) f->datachunksize/2;
                        break;
                case 20:                //NB not allowed by AIFF - no header field for masked sizes
                case 24:
                        numsampleframes = (DWORD) f->datachunksize / 3;
                        break;
                case 32:
                        numsampleframes = (DWORD) f->datachunksize/4;
                        break;
                default:
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "cannot update with unknown sample format";
                        return -1;
                        break;
                }
        }
        // else if bitspersample== 8....  ?
        numsampleframes /= f->fmtchunkEx.Format.nChannels;
        /* RWD 8.99 this may be wrong...*/
        /*f->mainchunksize = sizeof(DWORD) + 2*sizeof(DWORD) + 26 + 2*sizeof(DWORD) + f->datachunksize + f->extrachunksizes;
         */
        f->mainchunksize = POS64(f->datachunkoffset) + (DWORD) f->datachunksize - 2 * sizeof(DWORD);

#if defined defined _WIN32
        if(SetFilePointer(f->fileno,4L,NULL,FILE_BEGIN) == 0xFFFFFFFF
#else
        bytepos = f->datachunkoffset;
        POS64(bytepos) -= 3*sizeof(DWORD);
        if(fseek(f->fileno, 4L, SEEK_SET) < 0
#endif
         ||write_dw_msf(f->mainchunksize, f)
#if defined defined _WIN32
         ||SetFilePointer(f->fileno,(long)(f->datachunkoffset - 3*sizeof(DWORD)),NULL,FILE_BEGIN) == 0xFFFFFFFF
#else
//       ||lseek(f->fileno, f->datachunkoffset - 3*sizeof(DWORD), SEEK_SET) < 0
     ||fsetpos(f->fileno, &bytepos)
#endif
         ||write_dw_msf((DWORD) f->datachunksize + 2*sizeof(DWORD), f) ) {
                rsferrno = ESFWRERR;
                rsferrstr = "Write error: Can't update main or data chunk size";
                return -1;
        }

        if(POS64(f->fmtchunkoffset) > POS64(f->datachunkoffset)) {
                commafterdata++;
                POS64(f->fmtchunkoffset) = POS64(f->datachunkoffset) + (DWORD)((f->datachunksize+1)&~1);   /*RWD 2007 */
        }
        //RWD don't want to do this - might be AIFC - don't need to anyway!
#ifdef NOTDEF
        else
                f->fmtchunkoffset -= 2*sizeof(DWORD);
#endif
#if defined defined _WIN32
        if(SetFilePointer(f->fileno,f->fmtchunkoffset,NULL,FILE_BEGIN) == 0xFFFFFFFF){
#else
        if(fsetpos(f->fileno, &f->fmtchunkoffset) ) {
#endif
                rsferrno = ESFWRERR;
                rsferrstr = "Write error: Can't seek to COMM chunk";
                return -1;
        }
        //strictly, we should check the new format and redo the AIFC stuff,
        //OR reject if infochanged
        //at least, disallow format conversion in AIFC!

        if(
#ifdef NOTDEF
                write_dw_msf(TAG('C','O','M','M'), f)
         ||write_dw_msf(18, f)
         ||
#endif
           write_w_msf(f->fmtchunkEx.Format.nChannels, f)
         ||write_dw_msf(numsampleframes, f)
         ||write_w_msf(f->fmtchunkEx.Format.wBitsPerSample, f)
         ||write_dw_toex(f->fmtchunkEx.Format.nSamplesPerSec, f) ) {
                rsferrno = ESFWRERR;
                rsferrstr = "Write error: Can't update COMM chunk";
                return -1;
        }


    // update peak chunk

        if((f->min_header >= SFILE_PEAKONLY) && f->peaks){
#if defined defined _WIN32
                if(SetFilePointer(f->fileno,f->peakchunkoffset + sizeof(DWORD),NULL,FILE_BEGIN)== 0xFFFFFFFF
#else
        bytepos = f->peakchunkoffset;
        POS64(bytepos) += sizeof(DWORD);
                if(fsetpos(f->fileno,&bytepos)
#endif
                        || write_dw_msf((DWORD) thistime,f)
                        || write_peak_msf(f->fmtchunkEx.Format.nChannels,f)) {

                        rsferrno = ESFWRERR;
                        rsferrstr = "SFSYS98: Write error: Can't update peak chunk";
                        return -1;
                }

        }


        if(!commafterdata) {
#if defined CDP99 && defined _WIN32
                if(SetFilePointer(f->fileno,(long)(f->datachunkoffset + (f->datachunksize+1)&~1),NULL,FILE_BEGIN)==0xFFFFFFFF) {
#else
        bytepos = f->datachunkoffset;
        POS64(bytepos) += (f->datachunksize+1)&~1;
       // if(lseek(f->fileno, f->datachunkoffset + (f->datachunksize+1)&~1, SEEK_SET) < 0) {
        if(fsetpos(f->fileno, &bytepos) ) {
#endif
                        rsferrno = ESFWRERR;
                        rsferrstr = "Can't seek to end, to update extra chunks";
                        return -1;
                }
        }
        /* RWD: NB if we have these, they will start correctly after a pad byte,
         * and we will just trust that these chunks are kosher!*/
        if(ap){
                for(; ap != 0; ap = ap->next) {
                        if(POS64(ap->offset) < POS64(f->datachunkoffset))
                                continue;
                        if(write_dw_msf(ap->tag, f)
                        ||write_dw_msf(ap->size, f)
                        ||dowrite(f, ap->buf, (ap->size+1)&~1) ) {
                                rsferrno = ESFWRERR;
                                rsferrstr = "Can't update extra chunk";
                                return -1;
                        }
                }
        }
        /*RWD AIFF requires pad byte at end, for 8bit and 24bit sample types*/
        else {
#ifdef FILE64_WIN
                __int64_t size = f->datachunkoffset +  (f->datachunksize+1)&~1;
#else
                DWORD size = POS64(f->datachunkoffset) +  ((f->datachunksize+1)&~1);
#endif
#if defined CDP99 && defined _WIN32
                if(w_ch_size(f->fileno, size) < 0) {
#else
                if(ftruncate(fileno(f->fileno), (off_t) size) < 0) {
#endif
                        rsferrno = ESFWRERR;
                        rsferrstr = "write error truncating file";
                        return -1;
                }
        }

        return 0;
}




int
sfclose(int sfd)
{
        struct sf_file *f;
        int rc = 0;

        if((f = findfile(sfd)) == 0){     //RWD: may already have been closed... see errrmsg
                rsferrstr = "SFSYS: close: bad file ID (already closed?)";
                return -1;
        }

        if((f->infochanged || f->propschanged) && !f->todelete && !f->readonly) {
                unsigned int cdptime;
                time_t now = time(0);
                cdptime = (unsigned int) now;
                //RWD.5.99
                if((f->min_header >= SFILE_CDP) && POS64(f->propoffset) > 0)
                        sfputprop(sfd, "DATE", /* (char *)&now */ (char *) &cdptime, sizeof(/*long*/int));

                switch(f->filetype) {
                case riffwav:
                case wave_ex:
                        if(f->header_set)
                                rc = wavupdate98(now,f);          //RWD.6.5.99
                        else
                                rc = wavupdate(f);
                        break;
                case eaaiff:
                case aiffc:
                        if(f->header_set)
                                rc = aiffupdate98(now,f);
                        else
                                rc = aiffupdate(now,f);                  //RWD.6.5.99
                        break;

                default:
                        //abort();
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "SFSYS98: aif-c files not supported";
                        return -1;
                        break;
                }
                f->propschanged = 1;
        }

        if(f->min_header >= SFILE_CDP && POS64(f->propoffset) >= 0 && f->propschanged && !f->todelete && !f->readonly)
                if(writeprops(f) < 0)
                        rc = -1;

#if defined _WIN32
        if(!CloseHandle(f->fileno)) {
#else
        if(fclose(f->fileno) < 0) {
#endif
                rsferrno = ESFWRERR;
                rsferrstr = "write error: system had trouble closing file";
                rc = -1;
        }

        if(f->todelete
                &&  f->filename != NULL
#if defined _WIN32
         && !DeleteFile(f->filename)){

#else
     && remove(f->filename) < 0) {

#endif
                rsferrno = ESFWRERR;
                rsferrstr = "can't remove soundfile";
                rc = -1;
        }
        freesffile(sfd-SFDBASE);

        return rc;
}


//return true size in bytes: of SHORTS or FLOATS file

__int64
sfsize(int sfd)
{
        struct sf_file *f;
        if((f = findfile(sfd)) == 0)
                return -1;
        return (__int64)(f->sizerequested == ES_EXIST ? f->datachunksize : f->sizerequested);
}


#ifdef FILE64_WIN
int
sfadjust(int sfd, __int64_t delta)
{
        struct sf_file *f;
        __int64_t newsize;

        if(delta > 0) {
                rsferrno = ESFBADPARAM;
                rsferrstr = "Can't extend a soundfile";
                return -1;
        }
        if((f = findfile(sfd)) == 0)
                return -1;

        if(f->readonly) {
                rsferrno = ESFREADONLY;
                rsferrstr = "can't adjust size of read-only file";
                return -1;
        }

        f->infochanged = 1;

        switch(f->sizerequested) {
        case ES_EXIST:
                if(f->datachunksize + delta < 0) {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "can't make soundfile with negative size";
                        return -1;
                }
                f->datachunksize += delta;
                break;
        default:
                if(f->sizerequested + delta < 0) {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "can't make soundfile with negative size";
                        return -1;
                }
                if(f->sizerequested + delta >= f->datachunksize)
                        return 0;
                f->sizerequested += delta;
                f->datachunksize = f->sizerequested;
                break;
        }
        newsize = f->datachunkoffset;
        if(f->fmtchunkEx.Format.wBitsPerSample == 8)
                newsize += f->datachunksize/2;
        else
                newsize += f->datachunksize;

#if defined defined _WIN32
        if(w_ch_size(f->fileno, newsize) < 0) {
#else
        if(chsize(f->fileno, newsize) < 0) {
#endif
                rsferrno = ESFWRERR;
                rsferrstr = "write error truncating file";
                return -1;
        }
        return 0;               /* is this right? */
}

#else
int
sfadjust(int sfd, __int64_t delta)
{
        struct sf_file *f;
        __int64_t newsize;

#ifdef _DEBUG
    fprintf(stder,"in sfadjust()\n");
#endif
        if(delta > 0) {
                rsferrno = ESFBADPARAM;
                rsferrstr = "Can't extend a soundfile";
                return -1;
        }
        if((f = findfile(sfd)) == 0)
                return -1;

        if(f->readonly) {
                rsferrno = ESFREADONLY;
                rsferrstr = "can't adjust size of read-only file";
                return -1;
        }

        f->infochanged = 1;

        switch(f->sizerequested) {
        case ES_EXIST:
                if( f->datachunksize + delta < 0) {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "can't make soundfile with negative size";
                        return -1;
                }
                f->datachunksize += delta;
                break;
        default:
                if(f->sizerequested + delta < 0) {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "can't make soundfile with negative size";
                        return -1;
                }
                if(f->sizerequested + delta >= f->datachunksize)
                        return 0;
                f->sizerequested += delta;
                f->datachunksize = f->sizerequested;
                break;
        }
        newsize = (__int64) POS64(f->datachunkoffset);
        if(f->fmtchunkEx.Format.wBitsPerSample == 8)
                newsize += f->datachunksize/2;
        else
                newsize += f->datachunksize;

#if defined _WIN32
        if(w_ch_size(f->fileno, newsize) < 0) {
#else
        if(chsize(fileno(f->fileno), newsize) < 0) {
#endif
                rsferrno = ESFWRERR;
                rsferrstr = "write error truncating file";
                return -1;
        }
        return 0;               /* is this right? */
}
#endif

int
sfunlink(int sfd)
{
        struct sf_file *f;

        if((f = findfile(sfd)) == 0)
                return -1;
        if(f->readonly) {
                rsferrno = ESFREADONLY;
                rsferrstr = "can't delete read-only file";
                return -1;
        }
        f->todelete = 1;
        return 0;
}

int
sfrename(int sfd, const char *newname)
{
        struct sf_file *f;
        char *path = NULL;
        if((f = findfile(sfd)) == 0)
                return -1;
        if(f->readonly) {
                rsferrno = ESFREADONLY;
                rsferrstr = "can't rename read-only file";
                return -1;
        }
        if(f->filename == 0) {
                rsferrno = ESFNOMEM;
                rsferrstr = "Couldn't remember name of file";
                return -1;
        }
        path = mksfpath(newname);
        if(path==NULL){
                rsferrno = ESFBADPARAM;
                rsferrstr = "can't rename file changing its type";
                return -1;
        }

        if(f->filetype != gettypefromname98(path)) {
                rsferrno = ESFBADPARAM;
                rsferrstr = "can't rename file changing its type";
                return -1;
        }

        if(rename(f->filename, path) != 0) {
                rsferrno = ESFDUPFNAME;
                rsferrstr = "file already exists, or can't rename across devices";
                return -1;
        }
        free(f->filename);
        f->filename = _fullpath(NULL, path, 0);
        return 0;
}

/*
 *      Property access routines
 */
//RWD.6.99 mega-special case for sample type now!
int
sfgetprop(int sfd, const char *propname, char *dest, int lim)
{
        int lret =  SAMP_MASKED;                //RWD.6.99
        int res,containersize;
        struct sf_file *f;
        struct property *pp;

        if((f = findfile(sfd)) == 0)
                return -1;

        if(strcmp(propname, "channels") == 0)
                lret = f->fmtchunkEx.Format.nChannels;
        else if(strcmp(propname, "sample rate") == 0)
                lret = f->fmtchunkEx.Format.nSamplesPerSec;
        else if(strcmp(propname, "sample type") == 0) {
                //RWD.6.99 lets accept all formats!
                //is this good for AIFF?
                containersize = 8 * (f->fmtchunkEx.Format.nBlockAlign /         f->fmtchunkEx.Format.nChannels);
                switch(containersize){
                case(32):
                        if(f->fmtchunkEx.Format.wFormatTag== WAVE_FORMAT_IEEE_FLOAT
                                || (f->fmtchunkEx.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE
                                                && (
                                                        (compare_guids(&(f->fmtchunkEx.SubFormat),&(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)))
                                                        ||
                                                         (compare_guids(&(f->fmtchunkEx.SubFormat),&(SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT)))
                                                        )
                                        )
                                        )       {
                                lret =  SAMP_FLOAT;
                                break;
                        }
                        else{
                                //int format: may be masked
                                switch(f->fmtchunkEx.Samples.wValidBitsPerSample){
                                case(32):
                                        lret = SAMP_LONG;
                                        break;
                                case(24):
                                        lret = SAMP_2432;
                                        break;
                                default:
                                        lret = SAMP_MASKED;
                                        break;

                                }
                        }
                        break;
                case(24):
                        switch(f->fmtchunkEx.Samples.wValidBitsPerSample){
                        case(24):
                                lret = SAMP_2424;
                                break;
                        case(20):
                                lret = SAMP_2024;
                                break;
                        default:
                                lret = SAMP_MASKED;
                                break;

                        }
                        break;
                case(16):
                case(8):
                        lret = SAMP_SHORT;
                        break;
                default:
                        break;
                }

        } else {
                if(f->min_header < SFILE_CDP){
                   rsferrno = ESFNOTFOUND;
                   rsferrstr = "no CDP properties in minimum header";
                   return -1;
                }

                if(POS64(f->propoffset) >= 0)
                        for(pp = f->props; pp != 0; pp = pp->next) {
                                if(strcmp(propname, pp->name) == 0) {
                                        res = min(pp->size, lim);
                                        memcpy(dest, pp->data, res);
                                        return res;
                                }
                        }
                rsferrno = ESFNOTFOUND;
                rsferrstr = "Property not defined in file";
                return -1;
        }
        /*
         *      return a standard property as a long
         */
        res = min(lim, sizeof(/*long*/int));
        memcpy(dest, &lret, res);
        return res;
}

static int
getlong(int *dest, char *src, int size, int error)
{
        if(size != sizeof(/*long*/int)) {
                rsferrno = error;
                rsferrstr = "Bad size for standard property";
                return -1;
        }
        memcpy(dest, src, sizeof(/*long*/int));
        return 0;
}

//RWD.6.99 trap attempts to update primary properties of streamable file
// ATARI trapped, but ATARI ~could~ support 32bit longs, but who cares?
int
sfputprop(int sfd, char *propname, char *src, int size)
{
         int data;
        struct sf_file *f;
        struct property *pp, **ppp;
        char *np;

        if((f = findfile(sfd)) == 0)
                return -1;
        if(f->readonly) {
                rsferrno = ESFREADONLY;
                rsferrstr = "can't set property in a read-only file";
                return -1;
        }

        if(strcmp(propname, "channels") == 0) {
                if(getlong(&data, src, size, ESFBADNCHANS) < 0)
                        return -1;
                //RWD.6.99
                if(f->header_set && (data != (int)f->fmtchunkEx.Format.nChannels)){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "not allowed to alter existing property";
                        return -1;

                }
                f->fmtchunkEx.Format.nChannels = (short)data;
                f->infochanged = 1;
                return 0;
        } else if(strcmp(propname, "sample rate") == 0) {

                if(getlong(&data, src, size, ESFBADRATE) < 0)
                        return -1;
                //RWD.6.99
                if(f->header_set && (data != (int) f->fmtchunkEx.Format.nSamplesPerSec)){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "not allowed to alter existing property";
                        return -1;
                }
                f->fmtchunkEx.Format.nSamplesPerSec = data;
                f->infochanged = 1;
                return 0;
        } else if(strcmp(propname, "sample type") == 0) {
                if(f->header_set){
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "not allowed to alter existing property";
                        return -1;
                }
                if(getlong(&data, src, size, ESFBADPARAM) < 0)
                        return -1;
#ifdef SFSYS_UNBUFFERED
                if(data != SAMP_SHORT && data != SAMP_FLOAT) {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "Only short samples supported, as yet!";
                        return -1;
                }
#else
                if(data < SAMP_SHORT && data >= SAMP_MASKED) {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "cannot change to unsupported sample type";
                        return -1;
                }

#endif
                if(f->fmtchunkEx.Format.wBitsPerSample == 8) {
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "Can't change sample type when accessing 8 bits/sample file";
                        return -1;
                }
#ifdef SFSYS_UNBUFFERED
                f->fmtchunkEx.Format.wBitsPerSample = (data == SAMP_SHORT) ? 16 : 32;
#else
                switch(data){
                case(SAMP_LONG):
                case(SAMP_FLOAT):
                case(SAMP_2432):
                        f->fmtchunkEx.Format.wBitsPerSample = 32;
                        break;
                case(SAMP_2424):
                case(SAMP_2024):
                        f->fmtchunkEx.Format.wBitsPerSample = 24;
                        break;
                case(SAMP_SHORT):
                        f->fmtchunkEx.Format.wBitsPerSample = 16;
                        break;
                case(SAMP_BYTE):
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "8-bit files not supportewd for writing";
                        return -1;
                default:
                        rsferrno = ESFBADPARAM;
                        rsferrstr = "cannot set unsupported sample type";
                        return -1;
                        break;

                }

#endif

                f->infochanged = 1;
                return 0;
        }

        /*
         *      now deal with extended attributes
         */

        if((f->min_header < SFILE_CDP) || POS64(f->propoffset) < 0) {
                rsferrno = ESFNOSPACE;
                rsferrstr = "No property chunk in this .wav file";
                return -1;
        }

        for(pp = f->props; pp != 0; pp = pp->next) {
                if(strcmp(propname, pp->name) == 0) {
                        if(f->curpropsize + 2*(size - pp->size) > f->proplim) {
                                rsferrno = ESFNOSPACE;
                                rsferrstr = "No space in extended properties for bigger property data";
                                return -1;
                        }
                        if((np = (char *) malloc(size)) == 0) {
                                rsferrno = ESFNOMEM;
                                rsferrstr = "No memory for bigger property data";
                                return -1;
                        }
                        memcpy(np, src, size);
                        free(pp->data);
                        pp->data = np;
                        pp->size = size;
                        f->curpropsize -= 2*(size - pp->size);
                        f->propschanged = 1;
                        return 0;
                }
        }

        /* adding new property */
        if(f->curpropsize + (signed int)strlen(propname) + 2 + 2*size > f->proplim) {
                rsferrno = ESFNOSPACE;
                rsferrstr = "No space in extended properties for new property data";
                return -1;
        }
        for(ppp = &f->props; *ppp != 0; ppp = &(*ppp)->next)
                ;
        if((*ppp = ALLOC(struct property)) == 0
         ||((*ppp)->name = (char *) malloc(strlen(propname)+1)) == 0
         ||((*ppp)->data = (char *) malloc(size)) == 0) {
                rsferrno = ESFNOMEM;
                rsferrstr = "No memory for bigger property data";
                return -1;
        }
        strcpy((*ppp)->name, propname);
        memcpy((*ppp)->data, src, size);
        (*ppp)->size = size;
        (*ppp)->next = 0;
        f->curpropsize += strlen(propname) + 2 + 2*size;
        f->propschanged = 1;
        return 0;
}


//RWD.6.5.99 not used internally by header routines, return 0 for true
int sfputpeaks(int sfd,int channels,const CHPEAK peakdata[])
{
        int i;
        struct sf_file *f;

        if((f = findfile(sfd)) == 0)
                return -1;
        if(f->readonly) {
                rsferrno = ESFREADONLY;
                rsferrstr = "can't set property in a read-only file";
                return -1;
        }

        if(f->min_header < SFILE_PEAKONLY){
           rsferrno = ESFNOSPACE;
                rsferrstr = "no space for peak data in minimum header";
                return -1;


        }

        if(f->peaks==NULL){
                rsferrno = ESFREADONLY;
                rsferrstr = "peak data not initialized";
                return -1;

        }
        for(i=0;i < channels; i++){
                f->peaks[i].value = peakdata[i].value;
                f->peaks[i].position = peakdata[i].position;
        }
        return 0;
}
//NB this one not used internally by header routines, return 1 for true
int sfreadpeaks(int sfd,int channels,CHPEAK peakdata[],int *peaktime)
{

        int i;
        struct sf_file *f;

        if((f = findfile(sfd)) == 0)
                return -1;

        if(f->peaks==NULL){                //NOT an error: just don't have the chunk
                *peaktime = 0;
                return 0;

        }
        *peaktime = (int) f->peaktime;
        for(i=0;i < channels; i++){
                peakdata[i].value = f->peaks[i].value;
                peakdata[i].position = f->peaks[i].position;
        }
        return 1;
}


int
sfrmprop(int sfd, char *propname)
{
        struct sf_file *f;
        struct property **ppp;

        if((f = findfile(sfd)) == 0)
                return -1;

        if(f->readonly) {
                rsferrno = ESFREADONLY;
                rsferrstr = "can't remove property from a read-only file";
                return -1;
        }

        if(strcmp(propname, "channels") == 0
         ||strcmp(propname, "sample rate") == 0
         ||strcmp(propname, "sample type") == 0) {
                rsferrno = ESFBADPARAM;
                rsferrstr = "Cannot remove standard property";
                return -1;
        }

        if(f->min_header < SFILE_CDP){
                rsferrno = ESFNOTFOUND;
                rsferrstr = "minimum header - no CDP properties present";
                return -1;
        }


        if(POS64(f->propoffset) >= 0)
                for(ppp = &f->props; *ppp != 0; ppp = &(*ppp)->next)
                        if(strcmp((*ppp)->name, propname) == 0) {
                                struct property *p = *ppp;
                                f->curpropsize -= strlen(propname) + 2 + p->size;
                                f->propschanged = 1;
                                free(p->name);
                                free(p->data);
                                *ppp = p->next;
                                free(p);
                                return 0;
                        }

        rsferrno = ESFNOTFOUND;
        rsferrstr = "Property not found";
        return -1;
}

int
sfdirprop(int sfd, int (*func)(char *propname, int propsize))
{
        struct sf_file *f;
        struct property *pp;

        if((f = findfile(sfd)) == 0)
                return -1;

        if(func("channels", sizeof(/*long*/ int)) != 0
         ||func("sample type", sizeof(/*long*/ int)) != 0
         ||func("sample rate", sizeof(/*long*/ int)) != 0)
                return SFDIR_FOUND;

        for(pp = f->props; pp != 0; pp = pp->next)
                if(func(pp->name, pp->size) != 0)
                        return SFDIR_FOUND;
        return SFDIR_NOTFOUND;
}
//CDP98


#if defined _WIN32 && defined _MSC_VER
long cdp_round(double fval)
{
    int result;
        _asm{
                fld     fval
                fistp   result
                mov     eax,result
        }
        return (long) result;
}

#else
# if defined __GNUWIN32__
// TODO: learn and use gcc asm syntax!
long cdp_round(double fval)
{
    int k;
        k = (int)(fabs(fval)+0.5);
        if(fval < 0.0)
            k = -k;
        return (long) k;
}

# endif

#endif


int sfformat(int sfd, fileformat *pfmt)
{
        struct sf_file *f;
        if(pfmt==NULL)
                return 0;
        if((f = findfile(sfd)) == 0)
                return -1;

        if(f->filetype ==  riffwav)
                *pfmt = WAVE;
        else if(f->filetype == wave_ex)
                *pfmt = WAVE_EX;
        else if(f->filetype== eaaiff)
                *pfmt = AIFF;
        else if(f->filetype ==aiffc)
                *pfmt = AIFC;
        else
                return -1;

        return 0;
}

int sfgetchanmask(int sfd)
{
        struct sf_file *f;
        //int mask = 0;   //default is generic (unassigned)
        if((f = findfile(sfd)) ==NULL)
                return -1;

        if(f->filetype==wave_ex)
                return f->fmtchunkEx.dwChannelMask;

        return 0;
}
//private, but used by sndsystem
int _rsf_getbitmask(int sfd)
{
        struct sf_file *f;

        if((f = findfile(sfd)) ==NULL)
                return 0;

        return f->bitmask;


}
int sf_getchanformat(int sfd, channelformat *chformat)
{
        struct sf_file *f;
        if((f = findfile(sfd)) ==NULL)
                return -1;
        if(chformat==NULL)
                return -1;
        *chformat = f->chformat;
        return 0;

}

const char* sf_getfilename(int sfd)
{
        struct sf_file *f;

        if((f = findfile(sfd)) ==NULL)
                return NULL;

        return (const char *) f->filename;

}

int sf_getcontainersize(int sfd)
{
        struct sf_file *f;

        if((f = findfile(sfd)) ==NULL)
                return -1;

        return (int) f->fmtchunkEx.Format.wBitsPerSample;
}


int sf_getvalidbits(int sfd)
{

        struct sf_file *f;

        if((f = findfile(sfd)) ==NULL)
                return -1;

        return (int) f->fmtchunkEx.Samples.wValidBitsPerSample;



}

//keep this as private func for now, but used by snd routines
// may have pos >2GB so need unsigned retval
unsigned int _rsf_getmaxpeak(int sfd,float *peak)
{
        struct sf_file *f;
        int i;
        double peakval = 0.0;
        if((f = findfile(sfd)) ==NULL)
                return 0xFFFFFFFF;

        if(f->peaks == NULL)
                return 0;

        for(i=0; i < f->fmtchunkEx.Format.nChannels; i++)
                peakval = max(peakval,(double)(f->peaks[i].value));


        *peak = (float) peakval;
        return 1;

}


/*RWD */
int   addprop(struct sf_file *f, char *propname, char *src, int size)
{
        struct property /* *pp,*/ **ppp;
        /*char *np;*/

        if(f->curpropsize + (signed int)strlen(propname) + 2 + 2*size > f->proplim) {
                rsferrno = ESFNOSPACE;
                rsferrstr = "No space in extended properties for new property data";
                return -1;
        }
        for(ppp = &f->props; *ppp != 0; ppp = &(*ppp)->next)
                ;
        if((*ppp = ALLOC(struct property)) == 0
         ||((*ppp)->name = (char *) malloc(strlen(propname)+1)) == 0
         ||((*ppp)->data = (char *) malloc(size)) == 0) {
                rsferrno = ESFNOMEM;
                rsferrstr = "No memory for bigger property data";
                return -1;
        }
        strcpy((*ppp)->name, propname);
        memcpy((*ppp)->data, src, size);
        (*ppp)->size = size;
        (*ppp)->next = 0;
        f->curpropsize += strlen(propname) + 2 + 2*size;
        f->propschanged = 1;
        return 0;
}

int sf_makepath(char path[], const char* sfname)
{
        char* fullname = mksfpath(sfname);
        if(fullname==NULL)
                return -1;
        strcpy(path,fullname);
        return 0;
}
