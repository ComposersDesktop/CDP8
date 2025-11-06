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

/*
 *  Sound Filing System - resident library interface header
 *
 *  Copyright M. C. Atkins, 1987, 1988, 1989, 1990, RWD revisions 1997,1998,199,2000,2001
 *  All Rights Reserved.
 *
 *  Portable version for WindowsNT, DOS and Unix
 */
#ifndef __CDP_SFSYS_INCLUDED__
#define __CDP_SFSYS_INCLUDED__


/*
 *  $Log: sfsys.h%v $
 * Revision 4.1  1994/10/31  15:19:34  martin
 * Starting with RCS
 *
 */

/*
 *  This version of sfsys.h was introduced with version 2.33 of sfsys.prg
 *
 *  ver 3.04    31/ 1/93 MCA - changes introducing license control
 *  ver 3.05    25/ 5/93 MCA - add sfdata
 */

/*  RWD: SFSYSEX version (c) CDP 2000,2001 */
/* NB  this now defines only the snd~ level routines.
   all (deprecated) sf~ routines  now in sffuncs.h
   MAY BE REMOVED ALTOGETHER IN TIME!
*/

/* RWD March 2008 added MC_SURR_5_0 */

/* RWD Aug 2009 sfsys64, restored this, needed by dirsf */
/* NB: sfsys64: sf_direct.length now unsigned, so will report a 4GB file size */

/* Feb 2010: discover if we are  64 bit */
#ifdef __GNUC__ 
# ifdef __LP64__
#   define CDPLONG64
# endif
#endif
#ifdef ENABLE_PVX
#include "wavdefs.h"
#endif

extern int _sfverno;
#define sfverno()   (_sfverno)      /* sfsys version number */


/* SFSYSEX: new structures; may be extended even further... */
typedef enum {wt_wave,wt_analysis,wt_formant,wt_transposition,wt_pitch,wt_binenv} wavetype;
typedef enum {SHORT8,SHORT16,FLOAT32,INT_32,INT2424,INT2432,INT2024,INT_MASKED}     sampletype;
/* very likely we will support .au and .snd too, and maybe even raw */
#ifdef ENABLE_PVX
// could have some knock-on effects. Alternative may be just to flag pvx file as WAVE_EX
typedef enum wt_format {FMT_UNKNOWN = -1,WAVE,WAVE_EX,AIFF,AIFC,PVOCEX}        fileformat;
#else
typedef enum wt_format {FMT_UNKNOWN = -1,WAVE,WAVE_EX,AIFF,AIFC}        fileformat;
#endif
typedef enum  {CDP_CREATE_NORMAL,CDP_CREATE_TEMPORARY,CDP_CREATE_RDONLY} cdp_create_mode;
/* MC_WAVE_EX is a temporary one to cover abstruse infile formats! */
typedef enum { STDWAVE,MC_STD,MC_MONO,MC_STEREO,MC_QUAD,MC_LCRS,MC_BFMT,MC_DOLBY_5_1,MC_SURR_5_0,MC_SURR_6_1,MC_SURR_7_1,MC_CUBE,MC_WAVE_EX } channelformat;

/* the main format structure, to be used for all new programs! */
/* this may expand to include new things, possibly PVOC-EX too */
/* NB any new fields should be added at the bottom ONLY!. Best of all is to define a new
 * structure, with SFPROPS as the first element
 */
typedef struct sfprops 
{
    int     srate;
    int     chans;
    wavetype    type;
    sampletype  samptype;       
    fileformat  format;         
    channelformat chformat;     
    int     origsize;       /*RWD 3:2000 needs UPDATING in pvoc to register all new sample types */
    int     origrate;
    int     origchans;      /* pitch, formant,transpos only */
    int     specenvcnt;     /* formants only */
    float   arate;
    int     winlen;                 /*aka Mlen*/
    int     decfac;                 /* aka Dfac*/
    /*RWD.6.99 for envelope files*/
    float   window_size;
} SFPROPS;




/* SFSYSEX PEAK chunk support: use CHPEAK to avoid likely name clashes (e.g. Release 4!)  */
typedef struct chpeak {
    float value;            /* absolute value: 0dBFS = 1.0  */
    unsigned int position;  /* in (muti-channel)sample frames */
} CHPEAK;


/*
 *  This is the Sound Filing System initialisation routine, which must be
 *  called before doing ANYTHING else with the SFSystem.
 *  It returns 0 on success, or 1 on failure.
 */
/* the name is not used! */
extern int sflinit(const char *progname);

/*
 *  Print a description of the most-recent error on stderr.
 *  calling this only makes sense after an error-return from
 *  a SFSystem routine.
 */
extern void  sfperror(const char *msg);

/*
 *  return the string that would be printed by sfperror.
 */
extern char *  sferrstr(void);

/*
 *  return the numeric error indication.
 */
extern int  sferrno(void);   //RWD Oct 2025 addewd void


extern int sfinit(void);     /* deprecated */
#define sfexit exit      /* deprecated */

/*
 *  the access routine, etc
 */
extern void  sffinish(void);

/* PEAK  support, all sample formats:
 * ability to write full, minimum or PEAK-only header
 */
/* header-size flags*/
#define SFILE_CDP       (2)
#define SFILE_PEAKONLY  (1)
#define SFILE_MINIMUM   (0)

/* support opening read-only files - e.g from CD, also multiple opens */
/* All new progs should use RDONLY wherever possible, when opening existing files */
#define CDP_OPEN_RDONLY     (0)
#define CDP_OPEN_RDWR       (1)

/* return 0 for true */
int sndreadpeaks(int sfd,int channels,CHPEAK peakdata[],int *peaktime);
int sndputpeaks(int sfd,int channels,const CHPEAK peakdata[]);

/*
 *  Next routines for buffered access to files
 */ 
int snd_getchanformat(int sfd, channelformat *chformat);
const char * snd_getfilename(int ifd);                                                               
int sndgetchanmask(int sfd);
extern int sndopen(const char *name);
/* sfsysEx extended version - supports all formats.
  * requires use of all snd~Ex functions */
extern int sndopenEx(const char *name, int auto_scale,int access);

extern int sndcreat(const char *name, int size, int stype);
/* sfsysEx extended version*/
extern int sndcreat_formatted(const char *fn, int size, int stype,
                              int channels,int srate,cdp_create_mode mode);  /*SFSYS98*/

/*need this for WAVE_EX,etc*/
int
sndcreat_ex(const char *name, int size,SFPROPS *props,int min_header,cdp_create_mode mode);

/* read sfile props into new structure, all in one go */
extern int snd_headread(int fd,SFPROPS *props);

extern int sndclose(int sfd);
extern int sndsize(int sfd);
extern int sndseek(int sfd, int dist, int whence);
extern int sndtell(int sfd);
extern int sndsetbuf(int sfd, char *buf, int bufsize);
extern int snd_makepath(char path[],const char* sfname);
/*RWD OCT97: as above, for sndfiles*/
extern int sndgetwordsize(int sfd);

/* set sndfile for deletion */
extern int sndunlink(int sndfd);                    
/*recognise shortcuts in WIN32 - ~should~ be portable to unix/mac alias
 */
int snd_is_shortcut(int sfd);
/*#endif*/

/* calls sfformat internally */
int snd_fileformat(int sfd, fileformat *pfmt);

/*
 *  float-oriented operations : DEPRECATED;  see below for extended versions 
 */
extern int fgetfloat(float *fp, int sfd);
extern int fputfloat(float *fp, int sfd);
extern int fgetfbuf(float *fp, int count, int sfd);
extern int fputfbuf(float *fp, int count, int sfd);
                                                                             
/*
 *  short-oriented operations  : DEPRECATED;
 */
extern int fgetshort(short *sp, int sfd);
extern int fputshort(short *sp, int sfd);
extern int fgetsbuf(short *sp, int count, int sfd);
extern int fputsbuf(short *sp, int count, int sfd);

extern int sndgetprop(int sfd, char *propname, char *dest, int lim);
extern int sndputprop(int sfd, char *propname, char *src, int size);
extern int sndrmprop(int sfd, char *propname);
extern int snddirprop(int sfd, int (*func)(char *propname, int propsize));


/* sfsysEx extended versions, supporting all formats */
/* set expect_floats if reading from a type-1 32bit file known to be floats */
/* it is best not to mix: e.g don;t use fgetfloatEx with sndtell() */ 
extern int fgetfloatEx(float *fp, int sfd,int expect_floats);
extern int fgetfbufEx(float *fp, int count, int sfd,int expect_floats);
extern int fputfloatEx(float *fp, int sfd);
extern int fputfbufEx(float *fp, int count, int sfd);
extern int sndcloseEx(int sfd);

extern int fgetshortEx(short *sp, int sfd,int expect_floats);
extern int fgetsbufEx(short *sp, int count, int sfd,int expect_floats);
extern int fputshortEx(short *sp, int sfd);
extern int fputsbufEx(short *sp, int count, int sfd);

extern int sndseekEx(int sfd, int dist, int whence);    /*uses buffered sf_routines*/
extern int sndsizeEx(int sfd);
extern int sndtellEx(int sfd);
extern int sf_getpvxfno(int sfd);     /* RWD: pvsys has separate list of files, so CDP file ids won't apply directly. */
/*
 *  definitions for use with sfdir
 */
/*
 *  values for the flags field
 */
#define SFFLG_EMPTY     (1) /* the SFile is empty */
#define SFFLG_LAST      (2) /* the SFile is the last on the disk */
#define SFFLG_SAMPLE    (4) /* sample type was set (only sfdir) */
#define SFFLG_ISOPEN    (8) /* file is open (internal use only) */
#define SFFLG_TODELETE  (16)    /* file to be deleted (ditto) */
#define SFFLG_IOERROR   (32)    /* sfdir only - error reading file information */

/*
 *  selectors for sfdir
 */
#define SFDIR_EMPTY (1) /* call func with empty SFiles */
#define SFDIR_USED  (2) /* call func with non-empty SFiles */
#define SFDIR_SAMPLE    (4) /* call func with (non-empty) sample SFiles */

#define SFDIR_ALL   (3) /* call func with all SFiles */

#define SFDIR_IGPREFIX  (16)    /* show files irrespective of prefix */

#define SFDIR_FOUND (0) /* function accepted file */
#define SFDIR_NOTFOUND  (1) /* no file was accepted */
#define SFDIR_ERROR (-1)    /* and error occured during sfdir */

/*
 *  Other useful constants
 */
/* NB in time, these will be REMOVED! */
#define SECSIZE     (512)       /* bytes/sector on the disk */
#define LOGSECSIZE  (9)     /* log2(ditto)  */

#ifdef SFFUNCS
#define SF_MAGIC        (0x15927624)    /* value of _sfmagic() */
#define SF_CMAGIC       (0x27182835)    /* magic number for configuration */
#define SF_UNLKMAGIC    (0x46689275)    /* magic value for rdiskunlck() */
#endif

#define SF_MAXFILES (1000)      /* max no. of SFfiles open at once */
#define MAXSNDFILES (1000)      /* max no. of sndfiles open at once */

/* RWD  reduced property size, to assist bad unix progs to read soundifles! */
/* also, we are reducing dependency on CDP props, through new file formats */
#define PROPSIZE    (800)       /* size of the property area in hdrs */
//#ifndef _WINNT_ 
#ifndef MSC_VER               
# ifdef MAXSHORT
#  undef MAXSHORT
# endif
# define MAXSHORT   (32767.0)   /* maxint for shorts (as a float);  used EVERYWHERE! */
# ifdef MAXLONG
#  undef MAXLONG
# endif
/* NOT USED in CDP */
# define MAXLONG  (2147483648.0)      /* RWD 18:March 2008,  was 214748367 */
# ifdef MAXINT
#  undef MAXINT
# endif
# define MAXINT MAXLONG
#endif
/* NB: not used in sfsys or CDP */
#define MAX24BIT  (8388607.0)

/*
 *  sample types
 */
#define SAMP_SHORT  (0)     /* 16-bit short integers */
#define SAMP_FLOAT  (1)     /* 32-bit (IEEE) floating point */
/* sfsysEx Extended type support */
#define SAMP_BYTE   (2)     /*recognize 8bit soundfiles*/
#define SAMP_LONG   (3)     /* four most likely formats*/
#define SAMP_2424   (4)     
#define SAMP_2432   (5)     /* illegal for AIFF, and probably for WAVE too , but what the...*/
#define SAMP_2024   (6)     /* curioisity value: used by Alesis ADAT */
#define SAMP_MASKED (7)     /*some weird WAVE_EX format!*/


/*
 *  Generate the program registration number
 */
#define PROGRAM_NUMBER(pn)  unsigned long _SF_prognum = (pn)

/*
 *  Values for sferrno
 */
#define EBASEERR    (100)

#define ESFNOSPACE      (EBASEERR+ 0)
#define ESFNOTOPEN      (EBASEERR+ 1)
#define ESFRDERR        (EBASEERR+ 2)
#define ESFWRERR        (EBASEERR+ 3)
#define ESFNOSFD        (EBASEERR+ 4)
#define ESFNOTFOUND     (EBASEERR+ 5)
#define ESFNOMEM        (EBASEERR+ 6)
#define ESFDUPFNAME     (EBASEERR+ 7)
#define ESFBADMAG       (EBASEERR+ 8)
#define ESFNOCONFIG     (EBASEERR+ 9)
#define ESFNOTINIT      (EBASEERR+11)
#define ESFCONSIST      (EBASEERR+12)
#define ESFNOSTYPE      (EBASEERR+13)
#define ESFBADRATE      (EBASEERR+14)
#define ESFBADNCHANS    (EBASEERR+15)
#define ESFBADPARAM     (EBASEERR+16)
#define ESFNOSEEK       (EBASEERR+17)
#define ESFBADSRCDST    (EBASEERR+18)
#define ESFLOSEDATA     (EBASEERR+19)
#define ESFBADADDR      (EBASEERR+20)
#define ESFPTHTOLONG    (EBASEERR+21)
#define ESFILLSFNAME    (EBASEERR+22)
#define ESFPREFTOLONG   (EBASEERR+23)
#define ESFLOCKED       (EBASEERR+24)
#define ESFILLREGNAM    (EBASEERR+25)
#define ESFNOSTRMR      (EBASEERR+26)
#define ESFHARDCONFIG   (EBASEERR+27)
#define ESFUNDERRUN     (EBASEERR+28)
#define ESFBUFFOPER     (EBASEERR+29)
#define ESFSETBUFFER    (EBASEERR+30)
#define ESFOVERRUN      (EBASEERR+31)
#define ESFNOTPLAYING   (EBASEERR+32)
#define ESFPLAYING      (EBASEERR+33)
#define ESFUSESDMA      (EBASEERR+34)
#define ESFRECING       (EBASEERR+35)
#define ESFNOTRECING    (EBASEERR+36)
#define ESFREADONLY     (EBASEERR+37)

#define ELASTERR        (ESFREADONLY)

/*
 *  Atari things we we will also have to emulate
 */
void *Malloc(long size);
void Mfree(void *);

/*CDP98: now a func, so can round negative numbers properly!*/
/* NB in WIN32 sfsys.c: this is implemented in asssembler for great speed! */
//RWD Oct 2025: should no longer be needed as lround() now available across platforms
extern int cdp_round(double val);
/* unix math.h does not include these macros */
#ifndef __cplusplus
#ifndef min
#define min(x,y)    ( ((x)>(y)) ? (y) : (x) )
#endif

#ifndef max
#define max(x,y)    ( ((x)>(y)) ? (x) : (y) )
#endif
#endif
/*
 *  Globals internal to the porting library
 */
extern int rsferrno;
extern char *rsferrstr;
extern char *props_errstr;

/*RWD: yet to be tested: only for 32bit platforms */
#define IS_DENORMAL(f) (((*(unsigned int*)&f)&0x7f800000)==0)
/* use like this:
if (IS_DENORMAL(x)) x = 0.0f;
*/

int sndrewind(int sfd);
# ifdef ENABLE_PVX
/* PVOCEX file access */
/* return 0=false, 1 = true */
int sf_ispvx(int sfd);
/* return -1 for error in fd, id can be >=0  */
int get_pvxfd(int fd,PVOCDATA *pvxdata);
# endif
#endif      
