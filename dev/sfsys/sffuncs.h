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



/* sffuncs.h : private routines and structures */
/*RWD Dec 2009 changed various fields in sf_direct to int or unsigned int */
#ifndef __CDP_SFFUNCS_INCLUDED__
#define __CDP_SFFUNCS_INCLUDED__
/*
 *      Various dummy definitions which did things on the Atari, but must be
 *      removed for other platforms
 */
#define STDARGS /**/
#define SFCALLS /**/

/* RWD.5.99: are these now too small for WIN32? */
#define MAXSFNAME       (255)   /* maximum length of SFfile's name */
#define MAXPREFIX       (50)    /* maximum length of a filename prefix */

#ifndef WIN32
typedef long long __int64;
#endif

/*
 *      Structure definitions
 */
/*RWD.5.99 alignment problems with targetname[] ? */
struct sf_direct {
    int     flags;                  /* flags about the file (see below) */
    unsigned int length;            /* size of file in bytes */
    long    index;                  /* index of header sector on disk */
    long    seclen;                 /* number of sectors (inc headers) */
    char    name[MAXSFNAME+1];      /* the name of the file */
    int     samptype;               /* the type of samples */
    int     nochans;                /* the number of channels */
    int     samprate;               /* the sampling rate */

/* sfsysEx */
    int     origwordsize;   /*recognize 8bit sfiles            */
    int     is_shortcut;
    char    targetname[256];        /*_MAX_PATH      = 255*/
    fileformat      fmt;
};

/*
 *      structure for various information about the soundfiling system
 */
struct sfdata {
    long version;
    long base_sf_fd;        /* lowest possible sf filedescriptor */
    long limit_sf_fd;       /* limit of sf filedescriptors */
    long base_snd_fd;       /* lowest possible snd filedescriptor */
    long limit_snd_fd;      /* limit of snd filedescriptors */
};

/*
 *      the format of a raw disk configuration block - mostly a fudge!
 */
#define MAXPARTS        (64)    /* maximum number of SFS partitions */

struct partinfo {               /* information about a partition */
    short   unit_no;        /* SCSI unit number (log|ctrl) */
    long    phys_start;     /* phys. address of first sector(ie sf_start) */
    long    sf_start;       /* logical address of first sector */
    long    sf_end;         /* logical address of last valid sec in part. */
};

struct rdskcfg {
    long    disksize;       /* number of sectors available for the sfs */
    short   npart;          /* number of partitions */
    struct partinfo pinfo[MAXPARTS];        /* information on the partitions */
};

/*
 *      First special routines
 */

extern char *sfgetbigbuf(int *secsize); /* create a big memory buffer */

extern int sfdata(struct sfdata *, int size);   /* get information about sfsys */

/*
 *      The dummy prefix stuff
 */
extern int sfsetprefix(char *path);
extern void sfgetprefix(char *path);
extern int rdiskcfg(struct rdskcfg *);
/*
 *      The main (DEPRECATED) sf functions
 */

extern int sfdir(int SFCALLS (*func)(struct sf_direct *filedetails), int flags);
extern int sfopen(const char *name);
extern int sfcreat(const char *name, int sizein, int *sizeout);
extern int sfclose(int sfd);
extern int sfunlink(int sfd);
//extern long sfrename(int sfd, const char *newname);
//extern long sf_makepath(char path[], const char* sfname);


extern int SFCALLS STDARGS sfread(int sfd, char *buf, int cnt);
extern int SFCALLS STDARGS sfwrite(int sfd, char *buf, int cnt);

extern int sfseek(int sfd, int dist, int whence);
#ifdef FILE64_WIN
extern __int64 sfsize(int sfd);
extern int sfadjust(int sfd, __int64 delta);
#else
extern __int64 sfsize(int sfd);
extern int sfadjust(int sfd, __int64 delta);
#endif
extern int sfgetprop(int sfd, const char *propname, char *dest, int lim);
extern int sfputprop(int sfd, char *propname, char *src, int size);
extern int sfrmprop(int sfd, char *propname);
extern int sfdirprop(int sfd, int (*func)(char *propname, int propsize));

/*RWD OCT97 temp fudge to deal with 8bit sffiles...not to be used in applications! */

extern int      sfgetwordsize(int sfd);

/*recognise shortcuts (aliasses) in WIN32*/
/* this ~should~ be portable...*/
/* return -1 for error, 0 for false, 1 for true */
int sf_is_shortcut(int sfd,char *name);
/* this is portable */
/* return -1 for error, 0 otherwise */
int sfformat(int sfd,fileformat *pfmt);

/*support for mixed int formats*/
int sf_getcontainersize(int sfd);
int sf_getvalidbits(int sfd);

/********** SFSYS 98 extended routines to support live streaming, header adjstments *******/
/* creates full header; format cannot be changed later! */
/* return -1 for error, else sfd*/
#ifdef FILE64_WIN
int sfcreat_formatted(const char *name, __int64 size, __int64 *outsize,int channels,
                      int srate, int stype,cdp_create_mode mode);

#else
int sfcreat_formatted(const char *name, __int64 size, __int64 *outsize,int channels,
                      int srate, int stype,cdp_create_mode mode);
#endif
/* re-entrant version of above, for GUI apps. Used in GrainMill. */
/* keeps the input sfd; returns 1 for success, 0 for error */
/* NB a snd~ version will be created asap! */
#ifdef FILE64_WIN
int sfrecreat_formatted(int sfd, __int64 size, __int64 *outsize,int channels,int srate, int stype);
#else
int sfrecreat_formatted(int sfd, __int64 size, __int64 *outsize,int channels,int srate, int stype);
#endif
/*return -1 for error, 0 for no peak info, 1 for success*/
/* returns raw ANSI peaktime from header: use ctime() etc to decode */
int sfreadpeaks(int sfd,int channels,CHPEAK peakdata[],int *peaktime);
/* currently, we have to track PEAK data explicitly, and update header at the end.
 * eventually, there will be a frame-append function that updates PEAK info automatcally */
/* return -1 for error, 0 for success */
int sfputpeaks(int sfd,int channels,const CHPEAK peakdata[]);

/* new version of sfopen, controlling access */
int sfopenEx(const char *name,unsigned int access);
const char* sf_getfilename(int ifd);
extern int SFCALLS STDARGS sfread_buffered(int sfd, char *buf, int cnt);
extern int SFCALLS STDARGS sfwrite_buffered(int sfd, char *buf, int cnt);

#ifdef FILE64_WIN
extern __int64 sfseek_buffered(int sfd, __int64 dist, int whence);
int sfcreat_ex(const char *name, __int64 size, __int64 *outsize,
               SFPROPS *props,int min_header,cdp_create_mode mode);
#else
extern __int64 sfseek_buffered(int sfd, __int64 dist, int whence);

int
sfcreat_ex(const char *name, __int64 size, __int64 *outsize,
           SFPROPS *props,int min_header,cdp_create_mode mode);
#endif
int sfgetchanmask(int sfd);

int sf_getchanformat(int sfd, channelformat *chformat);

/* read sfile props into new structure, all in one go */
extern int sf_headread(int fd,SFPROPS *props);

#define SF_MAGIC        (0x15927624)    /* value of _sfmagic() */
#define SF_CMAGIC       (0x27182835)    /* magic number for configuration */
#define SF_UNLKMAGIC    (0x46689275)    /* magic value for rdiskunlck() */

#define MSBFIRST        (1)
#define LSBFIRST        (1)
/* RWD extended set of symbols ! Now works for Mac universal Binary build */
#if defined(__I86__) || defined(_X86_) || defined(__x86_64) || defined(__i386__) || defined(__i486__) || defined(_IBMR2)
#undef MSBFIRST
#elif defined(M68000) || defined(__sgi) || defined (__POWERPC__)
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
//static char * REV3BYTES(char *samp_24);
extern int sampsize[];

#ifdef MSBFIRST
#define REVDATAINFILE(f)        ((f)->filetype == riffwav || (f)->filetype == wave_ex)
#else
#define REVDATAINFILE(f)        (((f)->filetype == eaaiff) || ((f)->filetype==aiffc))
#endif

#endif
