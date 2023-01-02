/* RWD version to support pvx files */
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
 *      Sound Filing System - Buffered sound system, open/close, etc.
 *
 *      Portable version
 *
 *      Copyright M. C. Atkins, 1986, 1987, 1993; RWD 2007,2014,2022
 *      All Rights Reserved.
 */
/* RWD: old sfsys functions DEPRECATED and emptied */
/* RWD Dec 2019  fixed SndCloseEx for int64 comps */
/* RWD 2022 added PVX support */
#ifdef _DEBUG
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sfsys.h>
#include "sffuncs.h"
#ifdef unix
#include <unistd.h>
#endif
#ifdef ENABLE_PVX
# ifdef unix
#  include "wavdefs.h"
#  include "pvdefs.h"
# endif
# include "pvfileio.h"
# ifdef _DEBUG
#  include <assert.h>
# endif
#endif
#ifdef _WIN32
#ifdef _MSC_VER
static __inline int cdp_round(double fval)
{
    int result;
    asm{
        fld     fval
        fistp   result
        mov     eax,result /* RWD: need this line? */
    }
    return result;
}
#else
# ifndef __GNUWIN32__
static int cdp_round(double val)
{
    int k;
    k = (int)(fabs(val)+0.5);
    if(val < 0.0)
        k = -k;
    return k;
}
# endif
#endif
#endif

/*
 *      This is the main structure describing a procom file
 */
struct sndfile {
    int     flags;              /* various flags */
    int     fd;                 /* SFfile/gemfile file descripter */
#ifdef ENABLE_PVX
    int pvxid;
#endif
    char    *buffer;           /* pointer to the buffer */
    char    *next;             /* next entry in buffer to access */
    int     bufsiz;             /* the size of the buffer (in sectors) */
    int     remain;             /* number of samples remaining in buffer */
    char    *endbuf;           /* end of buffer (for short bufs at eof) */
    int     samptype;           /* the type of samples in the file */
    int     lastread;           /* number of bytes obtained from last read */
    int     true_bufsize;               /*for 24bit alignment, etc: adjust buffer downwards*/
    int     scale_floats;
    double  fscalefac;
};

typedef union {
    int lsamp;
    /*float fsamp;                  //any use for this?   */
    unsigned char bytes[4];
} SND_SAMP;


/*
 *      values for sndfile.flags
 */
#define SNDFLG_WRITTEN  (1)     /* the buffer must be flushed back to disk */
#define SNDFLG_LASTWR   (4)     /* last i/o was a write (for sndtell) */
#define SNDFLG_TRUNC    (8)     /* truncate when closed */
#define SNDFLG_USERBUF  (32)    /* the buffer was supplied by the user */
#define SNDFLG_ATEND    (64)    /* the last i/o went over the end of the file */

/*
 *      Various constants/global data
 */
#define SNDFDBASE       (4000)          /* sndfd's allocated from here */
#define SNDBUFSIZE      (64)


/*
 *      global storage
 */
/*
 *      The array of open sndfiles
 */
//#if defined ENABLE_PVX
//static struct sndfile *sndfiles[MAXSNDFILES];
//#else
static struct sndfile *sndfiles[MAXSNDFILES];
//#endif
/*
 *      mapping table from sample type, to size
 */

/* the possible sample types are now:

        SAMP_SHORT      (0)             * 16-bit short integers *
        SAMP_FLOAT      (1)             * 32-bit (IEEE) floating point *
        SAMP_BYTE       (2)             //recognize 8bit soundfiles?
        SAMP_LONG       (3)             // four most likely formats
        SAMP_2424       (4)
        SAMP_2432       (5)
        SAMP_2024       (6)
        SAMP_MASKED (7)         //some weird WAVE_EX format!

*/

/*RWD.6.99 set this as container-size
 *private, but sfsys.c needs to have it
 */



int sampsize[] = {

/* SAMP_SHORT */        sizeof(short),
/* SAMP_FLOAT */        sizeof(float),
/* SAMP_BYTE  */        sizeof(unsigned char),
/* SAMP_LONG  */        sizeof(int),
/* SAMP_2424  */        3 * sizeof(unsigned char),
/* SAMP_2432  */        sizeof(int),
/* SAMP_2024  */        3 * sizeof(unsigned char),
/* dummy */             0                 /*for masked formats, have to calc from 1st principles...*/
};

/*
 *      Utility routines
 */
#define getmem(type)    ((type *)malloc(sizeof(type)))
extern int _rsf_getmaxpeak(int sfd,float *peak);
extern int _rsf_getbitmask(int sfd);

/*
 *      initialisation/termination routines
 */
static void
rsndallclose(void)
{
    int fd;

    for(fd = 0; fd < MAXSFNAME; fd++)
        if(sndfiles[fd] != 0)
            sndcloseEx(fd + SNDFDBASE);  //RWD was plain sndclose()
#if defined ENABLE_PVX
    pvsys_release();
#endif
}

static void
rsndinit()
{
    register int fd;
    static int inited = 0;

    if(inited != 0)
        return;
    inited++;
    for(fd = 0; fd < MAXSFNAME; fd++)
        sndfiles[fd] = 0;
    atexit(rsndallclose);       //RWD if we need to be reentrant, we can't use this!
}

/*
 *      map a sndfd onto a sndfiles index,
 *      return 0 on failure
 */

/*RWD I want a public version of this, for use in ASSERT etc*/

static int
mapsndfd(int *sfdp)
{
        *sfdp -= SNDFDBASE;

        if(*sfdp < 0 || *sfdp >= MAXSNDFILES) {
                rsferrno = ESFBADADDR;
                rsferrstr = "sfd is not in valid range";
                return 0;
        }
        if(sndfiles[*sfdp] == 0) {
                rsferrno = ESFNOTOPEN;
                rsferrstr = "sndfile is not open (bad sndfd)";
                return 0;
        }
        return 1;
}

/*
 *      do the underlying seek
 * NOTE
 *      seeks to the end of an sffile are rounded down!!
 */
static int
doseek(struct sndfile *sf, int dist, int whence)
{
        return sfseek(sf->fd, dist, whence);
}

/*RWD Feb 2010 ??? */
//static long
//doseekEx(struct sndfile *sf, long dist, int whence)
//{
//      return sfseek_buffered(sf->fd, dist, whence);
//}
static __int64 doseekEx(struct sndfile *sf, __int64 dist, int whence)
{
    return sfseek_buffered(sf->fd, dist, whence);
}

/*
 *      sndfilbuf - fill a buffer, for reading
 *      return true, if an error occurs
 */
static int
sndfilbuf(struct sndfile *sfd)
{
    long want /* = SECSIZE * sfd->bufsiz*/ ;          /*RWD: almost a const!*/
    long got;
    int rc = 0;

    if(sfd->flags & SNDFLG_ATEND)
        return 0;
    want = sfd->true_bufsize;
    /*only difference - supports other sample sizes*/
    got = sfread_buffered(sfd->fd, sfd->buffer, want);
    if(got < 0) {
        rc = 1;
        got = 0;
    }
    if(got < want)
        sfd->flags |= SNDFLG_ATEND;
    sfd->lastread = got;                                    /*bytes of <size>samps*/
    sfd->flags &= ~(SNDFLG_WRITTEN|SNDFLG_LASTWR);
    sfd->endbuf = &sfd->buffer[got];
/*RWD NB all snd funcs think in SAMPLES, of 2 types: SHORTS or FLOATS; all 8bit stuff hidden in sf_calls*/
    sfd->remain = got / sampsize[sfd->samptype];   /*RWD : adjust for 8bit files...*/
    sfd->next = sfd->buffer;
    return rc;
}

/*
 *      sndflsbuf - flush a buffer, after writing
 *      return true, if an error occurs
 */
static int
sndflsbuf(struct sndfile *sfd)
{
    int toput = sfd->endbuf - sfd->buffer;
    int put;

    if(sfd->next == sfd->buffer)
        return 0;
    if(sfd->flags & SNDFLG_ATEND)
        return 0;

    sfd->next = sfd->buffer;
    sfd->flags &= ~SNDFLG_WRITTEN;
    sfd->endbuf = sfd->buffer;

    if(!(sfd->flags&SNDFLG_LASTWR) ) {
        if(doseek(sfd, -sfd->lastread, 1) < 0) {
            sfd->flags |= SNDFLG_LASTWR;
            sfd->remain = (sfd->bufsiz<<LOGSECSIZE) / sampsize[sfd->samptype];
            return 1;
        }
    }
    sfd->flags |= SNDFLG_LASTWR;
    sfd->remain = (sfd->bufsiz<<LOGSECSIZE) / sampsize[sfd->samptype];

    toput = (toput+SECSIZE-1)&~(SECSIZE-1);
    if((put = sfwrite(sfd->fd, sfd->buffer, toput)) < 0)
        return 1;
    if(put < toput) {
        sfd->flags |= SNDFLG_ATEND;
        sfd->remain = 0;
    }
    return 0;
}


static int
sndflsbufEx(struct sndfile *sfd)
{
    int toput = sfd->endbuf - sfd->buffer;
    int put;

    if(sfd->next == sfd->buffer)
        return 0;
    if(sfd->flags & SNDFLG_ATEND)
        return 0;

    sfd->next = sfd->buffer;
    sfd->flags &= ~SNDFLG_WRITTEN;
    sfd->endbuf = sfd->buffer;

    if(!(sfd->flags&SNDFLG_LASTWR) ) {
        if(doseekEx(sfd,(__int64) -sfd->lastread, 1) < 0) {
            sfd->flags |= SNDFLG_LASTWR;
            sfd->remain = sfd->true_bufsize / sampsize[sfd->samptype];
            return 1;
        }
    }
    sfd->flags |= SNDFLG_LASTWR;
    sfd->remain = sfd->true_bufsize / sampsize[sfd->samptype];
    if((put = sfwrite_buffered(sfd->fd, sfd->buffer, toput)) < 0)
        return 1;

    if(put < toput) {
        sfd->flags |= SNDFLG_ATEND;
        sfd->remain = 0;
    }
    return 0;
}


/*
 *      free the sample buffer, if it wasn't supplied by a sndsetbuf
 */
static void
freesndbuf(int fd)
{
    if(sndfiles[fd]->flags & SNDFLG_USERBUF)
        return;
    free(sndfiles[fd]->buffer);
}

/*
 *      free the memory for an open SFfile
 *      used on last close, and failure of creat
 */
static void
freesndfd(int fd)
{
    freesndbuf(fd);
    free((char *)sndfiles[fd]);
    sndfiles[fd] = 0;
    return;
}

/*
 *      Try to find a snd file descripter
 *      returns -1, if there aren't any
 */
static int
findsndfd()
{
    register int fd = 0;

    while(sndfiles[fd] != 0)
        if(++fd >= MAXSNDFILES) {
            rsferrno = ESFNOSFD;
            rsferrstr = "Too many sndfiles are already open";
            return -1;
        }

    rsferrstr = "no memory to open sndfile";
    rsferrno = ESFNOMEM;

    if((sndfiles[fd] = getmem(struct sndfile)) == 0)
        return -1;
    if((sndfiles[fd]->buffer = (char*) malloc(SNDBUFSIZE*SECSIZE)) == 0) {
        free((char *)sndfiles[fd]);
        sndfiles[fd] = 0;
        return -1;
    }
#ifdef ENABLE_PVX
    sndfiles[fd]->pvxid = -1;
#endif
    sndfiles[fd]->flags = 0;
    sndfiles[fd]->bufsiz = SNDBUFSIZE;
    sndfiles[fd]->lastread = 0;
    sndfiles[fd]->endbuf = sndfiles[fd]->next = sndfiles[fd]->buffer;
    sndfiles[fd]->true_bufsize = SNDBUFSIZE * SECSIZE;      /*may get rounded down for 24bit formats*/
    return fd;
}

/*
 *      The user-accessable routines
 */

/*
 *      open a sndfile
 */

int
sndopenEx(const char *fn,int auto_scale, int access)
{
    register struct sndfile *sf;
    int fd;
#ifdef ENABLE_PVX
    PVOCDATA pvxdata;
    int rc;
#endif
    rsndinit();

    if((fd = findsndfd()) < 0)
        return -1;
    sf = sndfiles[fd];

    if((sf->fd = sfopenEx(fn,access)) < 0) {
        freesndfd(fd);
        return -1;
    }
    
    /* RWD TODO: add pvx support here_ sfopenEx can produce one */
#ifdef ENABLE_PVX
    if(sf_ispvx(sf->fd)){
        if((rc = get_pvxfd(sf->fd ,&pvxdata )) < 0){
# ifdef _DEBUG
            fprintf(stderr,"sndopenEx: bad sf id\n");
# endif
            return -1;
        }
        else if (rc >= 0){
# ifdef _DEBUG
            fprintf(stderr,"sndopenEx: file %s is a pvx file!\n", fn);
# endif
            return fd + SNDFDBASE;
        }
    }
    // else its a soundfile or a .ana file, carry on  as usual
#endif
    /* all the rest just for standard CDP files */
    sf->remain = 0;
    if(sfgetprop(sf->fd, "sample type",
                (char *)&sf->samptype, sizeof(int)) != sizeof(int)) {
        rsferrno = ESFNOSTYPE;
        rsferrstr = "no sample type defined";
        freesndfd(fd);
        return -1;
    }
    /*RWD only used to rescale floatsams when read into shorts (eg for playback)*/
    sf->scale_floats = 0;
    sf->fscalefac = 1.0;
    if(auto_scale && (sf->samptype==SAMP_FLOAT)){
        float fac = 0.0f;
        /* RWD 4:2002: I would prefer to rely on PEAK only for this now... */
        if(sfgetprop(sf->fd,"maxamp",(char *)&fac,sizeof(float)) == sizeof(float)){
            if(fac > sf->fscalefac){
                sf->scale_floats = 1;
                sf->fscalefac = 0.99995 / (double) fac;
            }
        }
        /*get it from PEAK data if available*/
        else if (_rsf_getmaxpeak(sf->fd,&fac) > 0){
            if(fac > sf->fscalefac){
                sf->scale_floats = 1;
                sf->fscalefac = 0.99995 / (double) fac;
            }
        }
                /*Could also provide a set_autoscale() func, legal only before first read (like sndsetbuf)*/
    }

    /*adjust internal buffer if reading 24bit formats*/
    if(sf->samptype==SAMP_2424 || sf->samptype == SAMP_2024)
        sf->true_bufsize = (sf->true_bufsize / sampsize[sf->samptype]) * sampsize[sf->samptype];

    /* need superfluous seek to overcome daft OS problems reopening just-closed file */
    if(sndseekEx(fd+SNDFDBASE,0,0) != 0)
        return -1;
    return fd+SNDFDBASE;
}

/* RWD PVX note: this is the form called by CDP for ANAL files */
/* Also, NB stype  is according to the SAMP_* list, gets converted by sampsize[] for sf routines as here */
int
sndcreat_formatted(const char *fn, int size, int stype,int channels,
                                   int srate,cdp_create_mode mode)
{
    register struct sndfile *sf;
    int fd;

    rsndinit();

    if((fd = findsndfd()) < 0)
        return -1;
    sf = sndfiles[fd];

    sf->flags |= SNDFLG_TRUNC;
    sf->samptype = stype;

    /*RWD NB sampsize[]  - no slot for 24bit size yet*/
    /* ALSO: at this stage, CDP still assumes 32bits if an analysis file, no "original sampsize" info yet -
       we have to rely on sfputprop("original sampsize" being set by program
     */
    /* if pvx file, sets sffile->pvxfileno (which may be 0 ), but returns sf id (which may be 1) */
    if((sf->fd = sfcreat_formatted(fn, size*sampsize[stype], (__int64 *)0,channels,srate,
                    stype,mode)) < 0) {
        freesndfd(fd);
        return -1;
    }
    
    // now need to set sf->pvxid, if this is a pvx file.
    sf->remain = sf->true_bufsize / sampsize[stype];
    return fd+SNDFDBASE;
}

/* RWD PVX: Trouble is, we need to use this one, to get props */
/* means just one change (???!!!???) inside cdp2k, mainfunc.c */
int
sndcreat_ex(const char *name, int size,SFPROPS *props,int min_header,cdp_create_mode mode)
{
        register struct sndfile *sf;
        int fd;
        int smpsize;
        rsndinit();

        if((fd = findsndfd()) < 0)
                return -1;
        sf = sndfiles[fd];

        sf->flags |= SNDFLG_TRUNC;

        /* RWD.6.99 write 24bit formats, etc */
        switch(props->samptype){
        case(FLOAT32):
                sf->samptype = SAMP_FLOAT;
                break;
        case(SHORT16):
                sf->samptype = SAMP_SHORT;
                break;
        case(INT_32):
                sf->samptype = SAMP_LONG;
                break;
        case(INT2424):
                sf->samptype = SAMP_2424;
                /*adjust sndbuffer with 3-byte alignment*/
                sf->true_bufsize = (sf->true_bufsize / sampsize[sf->samptype]) * sampsize[sf->samptype];
                break;
        case(INT2024):
                sf->samptype = SAMP_2024;
                /*adjust sndbuffer with 3-byte alignment*/
                sf->true_bufsize = (sf->true_bufsize / sampsize[sf->samptype]) * sampsize[sf->samptype];
                break;
        case(SAMP_MASKED):
                /*don't know how to do this yet...*/
                rsferrno = ESFBADPARAM;
                rsferrstr = "cannot write masked sample type";
                return -1;
        default:
                rsferrno = ESFBADPARAM;
                rsferrstr = "unknown sample type";
                return -1;
                break;

        }

        /*sf->samptype = (props->samptype == FLOAT32 ? SAMP_FLOAT : SAMP_SHORT);*/
        smpsize = sampsize[sf->samptype];

        /*need to return to outsize: size / smpsize - check this...*/

    if((sf->fd = sfcreat_ex(name, (__int64)size *smpsize, (__int64 *)0,
                        props,min_header,mode)) < 0) {
                freesndfd(fd);
                return -1;
        }
        sf->remain = sf->true_bufsize / smpsize;
        return fd+SNDFDBASE;
}

/*
 *      close a sndfile
 */

int
sndcloseEx(int fd)
{
    register struct sndfile *sf;
    int rc = 0;
    __int64 length, pos;

    if(!mapsndfd(&fd))
        return -1;
    sf = sndfiles[fd];
#ifdef ENABLE_PVX
    if(sf_ispvx(sf->fd)){
      //  if(sf_ispvx(sf->fd,0) >=0) {
# ifdef _DEBUG
            fprintf(stderr,"calling sfclose with fd %d\n",sf->fd);
# endif
            rc =  sfclose(sf->fd);   // I think..nothing else to do for pvx file!
            if(rc < 0) {
# ifdef _DEBUG
                fprintf(stderr,"sfclose error, rc = %d\n",rc);
# endif
                sfperror("sndcloseEx");
            }
            freesndfd(fd);
            return rc;
      //  }
    }
#endif

    if(sf->flags&SNDFLG_TRUNC) {
        length = sfsize(sf->fd);
        pos = sndtellEx(fd + SNDFDBASE); //RWD 12-12-19 make sure of 64bit ints in calcs
        pos *= sampsize[sf->samptype];
        if(sf->flags  & SNDFLG_WRITTEN )        /* should never exec */
            sndflsbufEx(sf);                /* rsfsize does it!  */
        if((rc = sfadjust(sf->fd,pos-length)) < 0) {
            rsferrno = ESFWRERR;
            rsferrstr = "can't truncate SFfile";
        }
    } else if(sf->flags  & SNDFLG_WRITTEN )
        sndflsbufEx(sf);
    rc |= sfclose(sf->fd);

    freesndfd(fd);
    return rc;
}


/*RWD.6.98 needed for Release98!*/
int sndunlink(int sndfd)
{
    //int size;
    struct sndfile *sf;
    /* RWD PVX TODO: hopefully wouldn't need to perform this hack on pvx file....? */
    (void)sndseekEx(sndfd,0,0);     /*RWD.7.98 hack to fix bug when closing empty file*/
    if(!mapsndfd(&sndfd))
        return -1;

    sf = sndfiles[sndfd];
    return sfunlink(sf->fd);
}


/*
 *      Return the size (in samples) of a sndfile
 * NOTE
 *      the first sndseek will flush buffer if necessary
 */

int
sndsizeEx(int sfd)
{
    int oldpos;
    int size;

    if((oldpos = sndtellEx(sfd)) < 0)         /* RWD: pvx handling done */
        return -1;
    if((size = sndseekEx(sfd, 0L, 2)) < 0) {
        size = -1;
        rsferrno = ESFNOSEEK;
        rsferrstr = "can't seek to end of file to find size";
    }
    if(sndseekEx(sfd, oldpos, 0) < 0) {
        rsferrno = ESFNOSEEK;
        rsferrstr = "can't restore position after finding file size";
        return -1;
    }
    return size;
}

/*
 *      seek in a sndfile  (of SHORTS or FLOATS)
 */
int
sndseek(int fd, int dist, int whence)
{
        //return sndseekEx(fd,dist,whence);
        return -1;
}

//RWD NB PVX: returns new file pos (in samples), or -1 for error
int
sndseekEx(int fd, int dist, int whence)
{
    register struct sndfile *sf;
    long bufsize;
    __int64 secpos;
    __int64 newpos = 0;
    __int64 gotpos;

    if(!mapsndfd(&fd))
        return -1;
    sf = sndfiles[fd];
#ifdef ENABLE_PVX
    if(sf_ispvx(sf->fd)){       // RWD TODO: shoukld add test for error return
        PVOCDATA pvxdata;
        // we must return currentframepos as raw floatsams
        int frameoffset;
        int curframe;
        int goalframe = 0;
        int rc = 0;
        int pvxid;
        pvxid = get_pvxfd(sf->fd,&pvxdata);
# ifdef _DEBUG
        assert(pvxid >= 0);
        
        // eventually we will code this permanently...but is this the best place to put this test?
        if(dist > 0)
            assert(dist % (pvxdata.nAnalysisBins * 2) == 0);
# endif
        frameoffset = dist / (pvxdata.nAnalysisBins * 2);  //NB: always mono stream in CDP
        
        switch(whence){
                //set: go to "dist" samples: must be int multiple of  framesize
                // seems never to be called for CDP ana routines, except for dist= 0
        case SEEK_SET:
            
            rc = pvoc_seek_mcframe(pvxid, frameoffset,SEEK_SET);
            //or: if dist==0, use pvoc_rewind()?
            if(rc < 0){
                return rc;
            }
            rc =  pvoc_framepos(pvxid) * (pvxdata.nAnalysisBins * 2);
            break;
            //tell - Ok to use pvoc_framepos, OR can use sndtellEx below
        case SEEK_CUR:
                // not directly handled in pvfileio.c yet
                rc = pvoc_seek_mcframe(pvxid, frameoffset,SEEK_CUR);
                if(rc < 0){
                    return rc;
                }
                rc =  pvoc_framepos(pvxid) * (pvxdata.nAnalysisBins * 2);
                break;
            //end: step back dist samps if negative. We do not allow setting beyond EOF.
        case SEEK_END:
                rc = pvoc_seek_mcframe(pvxid, frameoffset,SEEK_END);
                if(rc < 0){
                    return rc;
                }
                rc =  pvoc_framepos(pvxid);     //frames
                rc  *= pvxdata.nAnalysisBins * 2;    // samps. caller has to compute nframes
                break;
        }
        return rc;
    }
#endif
    // std CDP files from here
        bufsize = sf->true_bufsize;

        if((sf->flags & SNDFLG_WRITTEN) && sndflsbufEx(sf))
                return -1;
        switch(whence) {
        case 0:
                newpos = dist;           /*in SAMPLES: NB must deal with 8bit files!*/
                break;
        case 1:
                newpos = sndtellEx(fd+SNDFDBASE) + dist;
                break;
        case 2:
                newpos = sfsize(sf->fd);           /*size-specific:SHORTSAMS, from datachunksize*/
                newpos = (newpos / sampsize[sf->samptype]) + dist;              /*make non-size-specific*/
                break;
        }
        if(newpos < 0)
                newpos = 0;
        newpos *= sampsize[sf->samptype];               /* byte offset */ /*restore size-specific for doseek*/
        /*RWD.6.99 still need to do this - not a SECSIZE calc, but just cdp_round to our buffersize*/
        secpos = (newpos/bufsize)*bufsize;              /* cdp_round down */

/*RWD 2007: NB for FILE64_WIN doseekEx takes and returns __int64 */
        if((gotpos = doseekEx(sf, secpos, 0)) < 0)/*NB seek might be truncated */        /*gotpos = non-size-specific*/
                return -1;

        sf->flags &= ~SNDFLG_ATEND;

        if(sndfilbuf(sf)) {                     /* if sndfilbuf fails... */
                sf->next = sf->buffer;
                sf->remain = 0;
                return -1;
        }

        if(gotpos < secpos) {
                newpos -= sf->remain;     /*RWD 2007 !!! remain supposed to count ~samples~ */
                sf->remain = 0;
        }
        sf->next = &sf->buffer[(unsigned long)newpos % bufsize];
        newpos /= sampsize[sf->samptype];
        sf->remain = (sf->endbuf - sf->next)/sampsize[sf->samptype];
        return (long) newpos;
}

/*
 *      Where are we in the present sndfile
 * NOTE
 *      This does not need to flush buffers, etc
 */
int
sndtell(int fd)
{
        return -1;

}

/*RWD 2007 FIXME for 64bit reads: */
/* doseekEx must return unsigned long, or __int64 value */
/* RWD PVX: this is called only once in all of CDP7/8: pvthreads.cpp, to get current framepos */
/* direct equivalent is pvoc_framepos() */
/* but sfsys expects (float) sample pos as for (mono) .ana file */
/* don't want to clone all the sf calls here (dealing with SECSIZE etc */
int
sndtellEx(int fd)
{
    struct sndfile *sf;
        //long off;
    __int64  off;

    if(!mapsndfd(&fd))      //strips SNDFDBASE: fd = 0
        return -1;
    sf = sndfiles[fd];        //sf-fd = 1000
    
#ifdef ENABLE_PVX
    if(sf_ispvx(sf->fd)){
        int pvxid;
        PVOCDATA pvxdata;
        
        pvxid = get_pvxfd(sf->fd,&pvxdata);
        if(pvxid >=0){
        // we must return currentframepos as raw floatsams
            int32_t framepos;
            int32_t nsamps;
            //TODO: sf->fd is sf element, offset by SFDBASE= 1000
            // need to remove that first - how?
            framepos = pvoc_framepos(pvxid);
            if(framepos < 0){
# ifdef _DEBUG
                fprintf(stderr,"bad return %d from pvoc_framepos\n",framepos);
# endif
                return -1;
            }
            nsamps = framepos * (pvxdata.nAnalysisBins * 2); //of course, 1 bin  = [amp,freq] duplet
            return (int) nsamps;
        }
    }
#endif
    // std CDP files from here
    
    if((off = doseekEx(sf, 0L, 1)) < 0) {      /*NB: must return cnt of samples * sizeof(type=SHORTS or FLOATS)*/
        rsferrno = ESFNOSEEK;
        rsferrstr = "can't seek to find present position in file";
        return -1;
    }
    if(sf->flags & SNDFLG_LASTWR)                   /*RWD.5.1.99 fails for short sndfiles within a buffer*/
        off += sf->true_bufsize;

    if(sf->flags & SNDFLG_ATEND)
        off = sfsize(sf->fd);      /*bytes, of SHORTS or FLOATS samps*/
    off /= sampsize[sf->samptype];

    return (int)(off - ( (sf->remain < 0) ? 0 : sf->remain));
}

/*
 *      let the user supply a larger buffer
 */


/*RWD.6.99 NB bufsize is in SECTORS - may need new version for new formats...*/
int
sndsetbuf(int sfd, char *buf, int bufsize)
{
        struct sndfile *sf;

        if(!mapsndfd(&sfd))
                return -1;
        sf = sndfiles[sfd];
        if(sf->remain != 0) {
                rsferrno = ESFLOSEDATA;
                rsferrstr = "sndsetbuf would lose data";
                return -1;
        }
        freesndbuf(sfd);
        sf->buffer = buf;
        sf->bufsiz = bufsize;
        sf->true_bufsize = sf->bufsiz << LOGSECSIZE;
        sf->flags |= SNDFLG_USERBUF;
        return 0;
}

/*
 *      snd I/O routines
 */
/*
 *      fgetfloat - read the next float
 */
// RWD TODO: looks like several programs still call this!

int
fgetfloat(float *fp, int sfd)
{
        register struct sndfile *sfp;

        if(!mapsndfd(&sfd))
                return -1;
        sfp = sndfiles[sfd];
        if(sfp->remain < 0)
                return 0;
        if(sfp->remain-- == 0) {
                if(sndfilbuf(sfp))
                        return -1;
                if(sfp->remain-- <= 0)
                        return 0;
        }
        if(sfp->samptype == SAMP_FLOAT) {
                *fp = *(float *)sfp->next;
                sfp->next += sizeof(float);
        } else {
                *fp = (float)(*(short *)sfp->next) / (float)MAXSHORT;
                sfp->next += sizeof(short);
        }
        return 1;
}

int
fgetfloatEx(float *fp, int sfd,int expect_floats)
{
    register struct sndfile *sfp;
    long lword = 0;
    long mask;               /*we need signed ints*/
    SND_SAMP ssamp;

    if(!mapsndfd(&sfd))
        return -1;
    sfp = sndfiles[sfd];
    if(sfp->remain < 0)
        return 0;
    if(sfp->remain-- == 0) {
        if(sndfilbuf(sfp))
            return -1;
        if(sfp->remain-- <= 0)
            return 0;
        }
        ssamp.lsamp = 0;
        switch(sfp->samptype){
        case(SAMP_FLOAT):
            *fp = *(float *)sfp->next;
            sfp->next += sizeof(float);
            break;
        case(SAMP_SHORT):
            *fp = (float)(*(short *)sfp->next) / (float)MAXSHORT;
            sfp->next += sizeof(short);
            break;
        case(SAMP_LONG):
            if(expect_floats){
                *fp = *(float *)sfp->next;
                sfp->next += sizeof(float);
            }
            else{
                *fp = (float)((double) (*(int *)sfp->next) / (float)MAXINT);
                sfp->next += sizeof(int);
            }
            break;
        case(SAMP_2432):
            /*mask the word first*/
            lword =  *(long *)sfp->next;
            lword &= 0xffffff00;
            *fp = (float)((double)lword / MAXINT);
            sfp->next += sizeof(int);
            break;
        case(SAMP_2024):        /*need to mask it?*/
            ssamp.lsamp = 0;
            mask = _rsf_getbitmask(sfp->fd);
            if(mask==0)
                return -1;
#ifdef LSBFIRST
            ssamp.bytes[1] = sfp->next[0];
            ssamp.bytes[2] = sfp->next[1];
            ssamp.bytes[3] = sfp->next[2];
#else
            ssamp.bytes[0] = sfp->next[0];
            ssamp.bytes[1] = sfp->next[1];
            ssamp.bytes[2] = sfp->next[2];
#endif
            *fp = (float)((double)(ssamp.lsamp & mask) / MAXINT);
            sfp->next += 3;
            break;
        case(SAMP_2424):
#ifdef LSBFIRST
            ssamp.bytes[1] = sfp->next[0];
            ssamp.bytes[2] = sfp->next[1];
            ssamp.bytes[3] = sfp->next[2];
#else
            ssamp.bytes[0] = sfp->next[0];
            ssamp.bytes[1] = sfp->next[1];
            ssamp.bytes[2] = sfp->next[2];
#endif
            *fp = (float)((double)(ssamp.lsamp) / MAXINT);
            sfp->next += 3;
            break;
        default:
            rsferrno = ESFBADPARAM;
            rsferrstr = "attempted to read unknown sample format";
            return -1;                      /*do others later...*/
            break;
        }
        return 1;
}




/*
 *      fputfloat - write the next float
 */
int
fputfloat(float *fp, int sfd)
{
        register struct sndfile *sfp;

        if(!mapsndfd(&sfd))
                return -1;
        sfp = sndfiles[sfd];
        if(sfp->flags&SNDFLG_ATEND)
                return 0;
        sfp->flags |= SNDFLG_WRITTEN;
        /* sndtell checks this*/
        sfp->flags |= SNDFLG_LASTWR;

        if(sfp->samptype == SAMP_FLOAT) {
                *(float *)sfp->next = *fp;
                sfp->next += sizeof(float);
        } else {
                /* *(short *)sfp->next = (short) floor(0.5 + *fp * MAXSHORT);*/
                *(short *)sfp->next = (short) cdp_round( *fp * MAXSHORT);
                sfp->next += sizeof(short);
        }
        if(sfp->next > sfp->endbuf)
                sfp->endbuf = sfp->next;
        if(--sfp->remain == 0) {
                if(sndflsbuf(sfp))
                        return -1;
        }
        return 1;
}

/*RWD.7.99 replace floor calc with cdp_round(): more accurate!*/
int
fputfloatEx(float *fp, int sfd)
{
    register struct sndfile *sfp;
    SND_SAMP ssamp;

    if(!mapsndfd(&sfd))
        return -1;
    sfp = sndfiles[sfd];
    if(sfp->flags&SNDFLG_ATEND)
        return 0;
    sfp->flags |= SNDFLG_WRITTEN;
    /* sndtell checks this*/
    sfp->flags |= SNDFLG_LASTWR;
    ssamp.lsamp = 0;
    switch(sfp->samptype){
    case(SAMP_FLOAT):
        *(float *)sfp->next = *fp;
        sfp->next += sizeof(float);
        break;
    case(SAMP_SHORT):
        /* *(short *)sfp->next = (short)floor(0.5 + *fp * MAXSHORT);*/
        *(short *)sfp->next = (short) cdp_round( *fp * MAXSHORT);
        sfp->next += sizeof(short);
        break;
    case(SAMP_2024):
        /*ssamp.lsamp = (int) floor(0.5 + *fp * MAXINT);*/
        ssamp.lsamp = (int) cdp_round(*fp * MAXINT);
#ifdef LSBFIRST
        sfp->next[0] = ssamp.bytes[1] & 0xf0;
        sfp->next[1] = ssamp.bytes[2];
        sfp->next[2] = ssamp.bytes[3];
#else
        sfp->next[0] = ssamp.bytes[0] & 0xf0;
        sfp->next[1] = ssamp.bytes[1];
        sfp->next[2] = ssamp.bytes[2];
#endif
        sfp->next += 3;
        break;

    case(SAMP_2424):
        /*ssamp.lsamp = (int) floor(0.5 + *fp * MAXINT);*/
        ssamp.lsamp = (int) cdp_round(*fp * MAXINT);
#ifdef LSBFIRST
        sfp->next[0] = ssamp.bytes[1];
        sfp->next[1] = ssamp.bytes[2];
        sfp->next[2] = ssamp.bytes[3];
#else
        sfp->next[0] = ssamp.bytes[0];
        sfp->next[1] = ssamp.bytes[1];
        sfp->next[2] = ssamp.bytes[2];
#endif
        sfp->next += 3;
        break;
    case(SAMP_LONG):
        /* *(long *)sfp->next = (long) floor(0.5 + *fp * MAXINT);*/
        *(int *)sfp->next = (int) cdp_round(*fp * MAXINT);
        sfp->next += sizeof(int);
        break;
    case(SAMP_2432):
        /*ssamp.lsamp = (int) floor(0.5 + *fp * MAXINT);*/
        ssamp.lsamp = (int) cdp_round(*fp * MAXINT);
        ssamp.bytes[0] = 0;
        *(int *)sfp->next = ssamp.lsamp;
        sfp->next += sizeof(int);
        break;
    default:
        rsferrno = ESFBADPARAM;
        rsferrstr = "attempted to write unknown sample format";
        return -1;                      /*do others later...*/
        break;
    }
    if(sfp->next > sfp->endbuf)
        sfp->endbuf = sfp->next;
    if(--sfp->remain == 0) {
        if(sndflsbufEx(sfp))
            return -1;
    }
    return 1;
}



/*
 *      fgetfbuf - get a sequence of float-sams
 */

/*RWD.6.99 probably the tidiest way of dealing with old floatsam files
* expect_float must be non-zero to trigger
*/
int
fgetfbufEx(float *fp, int n, int sfd,int expect_floats)
{
    register struct sndfile *sfp;
    int chunk;
    int cnt = 0;
#ifdef ENABLE_PVX
    int rc;
#endif
    if(!mapsndfd(&sfd))
        return -1;
    sfp = sndfiles[sfd];
    
#ifdef ENABLE_PVX
    rc = sf_ispvx(sfp->fd);
    if(rc < 0){
# ifdef _DEBUG
        fprintf(stderr,"fgetfbufEx: bad sf id\n");
# endif
        return cnt;
    }

    else if(rc){
        PVOCDATA pvxdata;
        int n_frames = 0;
        int binsamps;
        int pvxfd = get_pvxfd(sfp->fd,&pvxdata);
        // we absolutely have to confirm n is exact multiple of nAnalysisBins * 2)
        binsamps = pvxdata.nAnalysisBins * 2;
        if((n % binsamps) != 0){
# ifdef _DEBUG
            fprintf(stderr,"fgetfbufEx: buffersize(%d) must be multiple of analysis frame size\n",n);
# endif
            cnt = 0;
        }
        n_frames = n / binsamps;
        // test sfp->samptype too?
        cnt = pvoc_getframes(pvxfd , fp, n_frames);
        if(cnt <= 0)
            cnt = 0; // EOF or error
        else
            cnt *= binsamps;
        return cnt;
    }
#endif
    // std CDP files from here
    if(sfp->samptype==SAMP_FLOAT || ((sfp->samptype==INT_32) && expect_floats)){
        while(cnt < n) {
            if(sfp->remain == 0) {
                if(sndfilbuf(sfp) || sfp->remain == 0)
                    return cnt;
            }
            chunk = n - cnt;
            if(chunk > sfp->remain)
                chunk = sfp->remain;
            memcpy((char *)fp, sfp->next, chunk*sizeof(float));
            sfp->remain -= chunk;
            sfp->next += chunk*sizeof(float);
            cnt += chunk;
            fp += chunk;
        }
    }
    else{
        sfd += SNDFDBASE;
        while(cnt < n && (fgetfloatEx(fp++, sfd,expect_floats) > 0))
            cnt++;
            /*return cnt;*/
    }
    return cnt;
}
/*
 *      fputfbuf - put a sequence of float-sams
 */
int
fputfbuf(float *fp, int n, int sfd)
{
    register struct sndfile *sfp;
    int chunk;
    int cnt = 0;

    if(!mapsndfd(&sfd))
        return -1;
    sfp = sndfiles[sfd];
    if(sfp->samptype == SAMP_SHORT) {
        sfd += SNDFDBASE;
        while(cnt < n && fputfloat(fp++, sfd) > 0)
            cnt++;
        return cnt;
    }
    while(cnt < n) {
        sfp->flags |= SNDFLG_WRITTEN;
        /* sndtell checks this*/
        sfp->flags |= SNDFLG_LASTWR;
        chunk = n - cnt;
        if(chunk > sfp->remain)
            chunk = sfp->remain;
        memcpy(sfp->next, (char *)fp, chunk*sizeof(float));
        sfp->remain -= chunk;
        sfp->next += chunk*sizeof(float);
        if(sfp->next > sfp->endbuf)
            sfp->endbuf = sfp->next;
        cnt += chunk;
        fp += chunk;
        if(sfp->remain == 0) {
            if(sndflsbuf(sfp))
                return -1;
            if(sfp->remain == 0)
                return cnt;
        }
    }
    return cnt;
}
#ifdef ENABLE_PVX
// not used yet...
int snd_getpvxfno(int sfd)
{
    register struct sndfile *sfp;

    if(!mapsndfd(&sfd))
        return -1;
    sfp = sndfiles[sfd];

    return sfp->pvxid;
}
#endif

int
fputfbufEx(float *fp, int n, int sfd)
{
    register struct sndfile *sfp;
    int chunk;
    int cnt = 0;
#ifdef ENABLE_PVX
    int rc;
#endif
    if(!mapsndfd(&sfd))
        return -1;
    sfp = sndfiles[sfd];
#ifdef ENABLE_PVX
    rc = sf_ispvx(sfp->fd);
    if(rc < 0)
        return -1;
    else if(rc) {
        PVOCDATA pvxdata;
        int n_frames = 0;
        int binsamps;
        int rc;
        int pvxid = -1;
        
        pvxid = get_pvxfd(sfp->fd,&pvxdata);
        if(pvxid < 0){
# ifdef _DEBUG
            fprintf(stderr,"fputfbufEx: bad pvx id %d\n",pvxid);
# endif
            return cnt;
        }
        else {
            // we absolutely have to confirm n is exact multiple of nAnalysisBins * 2)
        
# ifdef _DEBUG
            assert(pvxid >= 0);
            assert(pvxdata.nAnalysisBins > 0);
# endif
            binsamps = pvxdata.nAnalysisBins * 2;
            if((n % binsamps) != 0){
# ifdef _DEBUG
                fprintf(stderr,"fputfbufEx: buffersize(%d) must be multiple of analysis frame size\n",n);
# endif
                cnt = 0;
            }
            n_frames = n / binsamps;
            // test sfp->samptype too?
            cnt = pvoc_putframes(pvxid, fp, n_frames);
            if(cnt <= 0)
                cnt = 0; // EOF or error
            else
                cnt *= binsamps;
            return binsamps * n_frames;
        }
    }
    
#endif
    // std CDP files from here
    
    if(sfp->samptype == SAMP_SHORT) {
        sfd += SNDFDBASE;
        while(cnt < n && fputfloatEx(fp++, sfd) > 0)
            cnt++;
        return cnt;
    }

    if(sfp->samptype == SAMP_FLOAT){
        while(cnt < n) {
            sfp->flags |= SNDFLG_WRITTEN;
            /* sndtell checks this*/
            sfp->flags |= SNDFLG_LASTWR;
            chunk = n - cnt;
            if(chunk > sfp->remain)
                chunk = sfp->remain;
            memcpy(sfp->next, (char *)fp, chunk*sizeof(float));
            sfp->remain -= chunk;
            sfp->next += chunk*sizeof(float);
            if(sfp->next > sfp->endbuf)
                sfp->endbuf = sfp->next;
            cnt += chunk;
            fp += chunk;
            if(sfp->remain == 0) {
                if(sndflsbufEx(sfp))
                    return -1;
                if(sfp->remain == 0)
                    return cnt;
            }
        }
    }

    else {
        sfd += SNDFDBASE;
        while(cnt < n && fputfloatEx(fp++, sfd) > 0)
            cnt++;
        return cnt;
    }
    return cnt;
}

/* RWD.7.99 use cdp_round() instead of floor(): later, try shoft/truncate and/or dithering! */
int
fgetshortEx(short *sp, int sfd,int expect_floats)
{
    struct sndfile *sfp;
    int lword = 0;
    int mask;
    SND_SAMP ssamp;

    if(!mapsndfd(&sfd))
        return -1;
    sfp = sndfiles[sfd];
    if(sfp->remain < 0)
        return 0;
    if(sfp->remain-- == 0) {
        if(sndfilbuf(sfp))
            return -1;
        if(sfp->remain-- <= 0)
            return 0;
    }
    ssamp.lsamp = 0;
    switch(sfp->samptype){
    case(SAMP_FLOAT):
        if(sfp->scale_floats==1)
            /* *sp = (short)(floor(0.5 + (sfp->fscalefac * (*(float *)sfp->next * MAXSHORT))));*/
            *sp = (short) cdp_round((sfp->fscalefac * (*(float *)sfp->next * MAXSHORT)));
        else
            /* *sp = (short)floor(0.5 + *(float *)sfp->next * MAXSHORT);*/
            *sp = (short) cdp_round( *(float *)sfp->next * MAXSHORT);

        sfp->next += sizeof(float);
        break;
    case(SAMP_SHORT):
        *sp = *(short *)sfp->next;
        sfp->next += sizeof(short);
        break;
    case(SAMP_LONG):
        if(expect_floats){
            if(sfp->scale_floats==1)
                /* *sp = (short)(floor(0.5 + (sfp->fscalefac * (*(float *)sfp->next * MAXSHORT))));*/
                *sp = (short) cdp_round((sfp->fscalefac * (*(float *)sfp->next * MAXSHORT)));
            else
                /* *sp = (short)floor(0.5 + *(float *)sfp->next * MAXSHORT);*/
                *sp = (short) cdp_round( *(float *)sfp->next * MAXSHORT);

            sfp->next += sizeof(float);
        }
        else{
            *sp = (short)( (*(int *)sfp->next) >> 16);
            sfp->next += sizeof(int);
        }
        break;
    case(SAMP_2432):
        /*mask the word first*/
        lword =  *(int *)sfp->next;
        lword &= 0xffffff00;
        *sp = (short)(lword >> 16);
        sfp->next += sizeof(int);
        break;
    case(SAMP_2024):        /*need to mask it?*/
        mask = _rsf_getbitmask(sfp->fd);
        if(mask==0)
            return -1;
        ssamp.bytes[1] = sfp->next[0];
        ssamp.bytes[2] = sfp->next[1];
        ssamp.bytes[3] = sfp->next[2];
        *sp = (short)((ssamp.lsamp & mask) >> 16);
        sfp->next += 3;
        break;
    case(SAMP_2424):
        ssamp.bytes[1] = sfp->next[0];
        ssamp.bytes[2] = sfp->next[1];
        ssamp.bytes[3] = sfp->next[2];
        *sp = (short)(ssamp.lsamp >> 16);
        sfp->next += 3;
        break;
    default:
        rsferrno = ESFBADPARAM;
        rsferrstr = "attempted to read unknown sample format";
        return -1;                      /*do others later...*/
        break;
    }
    return 1;
}

int
fputshortEx(short *sp, int sfd)
{
    register struct sndfile *sfp;
    SND_SAMP ssamp;

    if(!mapsndfd(&sfd))
        return -1;
    sfp = sndfiles[sfd];
    if(sfp->flags&SNDFLG_ATEND)
        return 0;
    sfp->flags |= SNDFLG_WRITTEN;
    /* sndtell checks this*/
    sfp->flags |= SNDFLG_LASTWR;
    ssamp.lsamp = 0;
    switch(sfp->samptype){
    case(SAMP_FLOAT):
        *(float *)sfp->next = (float)*sp / (float)MAXSHORT;
        sfp->next += sizeof(float);
        break;
    case(SAMP_SHORT):
        *(short *)sfp->next = *sp;
        sfp->next += sizeof(short);
        break;
    case(SAMP_2024):
        /*no need to mask; 16 bits anyway!*/
    case(SAMP_2424):
        ssamp.lsamp = (int) ((*sp) << 16);
        sfp->next[0] = ssamp.bytes[1];
        sfp->next[1] = ssamp.bytes[2];
        sfp->next[2] = ssamp.bytes[3];
        sfp->next += 3;
        break;
    case(SAMP_LONG):
    case(SAMP_2432):
        ssamp.lsamp = (int) ((*sp) << 16);
        *(int *)sfp->next = ssamp.lsamp;
        sfp->next += sizeof(int);
        break;
    default:
        rsferrno = ESFBADPARAM;
        rsferrstr = "attempted to write unknown sample format";
        return -1;                      /*do others later...*/
        break;
    }
    if(sfp->next > sfp->endbuf)
        sfp->endbuf = sfp->next;
    if(--sfp->remain == 0) {
        if(sndflsbufEx(sfp))
            return -1;
    }
    return 1;
}

int
fgetsbufEx(short *sp, int n, int sfd,int expect_floats)
{
    register struct sndfile *sfp;
    int chunk;
    int cnt = 0;

    if(!mapsndfd(&sfd))
        return -1;
    sfp = sndfiles[sfd];

    switch(sfp->samptype){
    case(SAMP_SHORT):
        while(cnt < n) {
            if(sfp->remain <= 0) {                 /*RWD.6.99 was == 0*/
                if(sndfilbuf(sfp) || (sfp->remain == 0))
                    return cnt;
            }
            chunk = n - cnt;
            if(chunk > sfp->remain)
                chunk = sfp->remain;
            memcpy((char *)sp, sfp->next, chunk*sizeof(short));
            sfp->remain -= chunk;
            sfp->next += chunk*sizeof(short);
            cnt += chunk;
            sp += chunk;
        }
        break;
    default:
        sfd += SNDFDBASE;
        while(cnt < n && fgetshortEx(sp++, sfd,expect_floats) > 0)
            cnt++;
        break;
    }
    return cnt;
}

int
fputsbufEx(short *sp, int n, int sfd)
{
    register struct sndfile *sfp;
    int chunk;
    int cnt = 0;

    if(!mapsndfd(&sfd))
        return -1;
    sfp = sndfiles[sfd];

    switch(sfp->samptype){
    case(SAMP_SHORT):
        while(cnt < n) {
            sfp->flags |= SNDFLG_WRITTEN;

            /* sndtell checks this*/
            sfp->flags |= SNDFLG_LASTWR;
            chunk = n - cnt;
            if(chunk > sfp->remain)
                chunk = sfp->remain;
            memcpy(sfp->next, (char *)sp, chunk*sizeof(short));
            sfp->remain -= chunk;
            sfp->next += chunk*sizeof(short);
            if(sfp->next > sfp->endbuf)
                sfp->endbuf = sfp->next;
            cnt += chunk;
            sp += chunk;
            if(sfp->remain == 0) {
                if(sndflsbufEx(sfp))
                    return -1;
                if(sfp->remain == 0)
                    return cnt;
            }
        }
        break;
    default:
        sfd += SNDFDBASE;
        while(cnt < n && fputshortEx(sp++, sfd) > 0)
            cnt++;
        break;
    }
    return cnt;
}



/*
 *      The property stuff, for sndfiles
 */
int
sndgetprop(int sfd, char *prop, char *dest, int lim)
{
    if(!mapsndfd(&sfd))
        return -1;
    return sfgetprop(sndfiles[sfd]->fd, prop, dest, lim);
}

int
sndputprop(int sfd, char *prop, char *src, int size)
{
    if(!mapsndfd(&sfd))
        return -1;
    if(strcmp(prop, "sample type") == 0) {
        rsferrno = ESFNOSTYPE;
        rsferrstr = "can't change sample type on sndfile";
        return -1;
    }
    return sfputprop(sndfiles[sfd]->fd, prop, src, size);
}

int
sndrmprop(int sfd, char *prop)
{
    if(!mapsndfd(&sfd))
        return -1;
    if(strcmp(prop, "sample type") == 0) {
        rsferrno = ESFNOSTYPE;
        rsferrstr = "can't remove sample type on sndfile";
        return -1;
    }
    return sfrmprop(sndfiles[sfd]->fd, prop);
}

int
snddirprop(int sfd, int (*func)(char *propname, int propsize))
{
    if(!mapsndfd(&sfd))
        return -1;
    return sfdirprop(sndfiles[sfd]->fd, func);
}


/*RWD OCT97*/
int sndgetwordsize(int sfd)
{
    if(!mapsndfd(&sfd))
        return -1;
    return sfgetwordsize(sndfiles[sfd]->fd);
}

/*recognise shortcuts in WIN32*/
#if defined _WIN32
/*TODO: add arg for full targetname*/
int snd_is_shortcut(int sfd)
{
    if(!mapsndfd(&sfd))
        return -1;
    return sf_is_shortcut(sndfiles[sfd]->fd,NULL);
}
#endif

/*RWD.5.99*/
int snd_fileformat(int sfd, fileformat *pfmt)
{
    if(!mapsndfd(&sfd))
        return -1;
    /* RWD 2022: this should now recognise pvx file as PVOCEX */
    return sfformat(sndfiles[sfd]->fd,pfmt);
}

int sndgetchanmask(int sfd)
{
    if(!mapsndfd(&sfd))
        return -1;

    return sfgetchanmask(sndfiles[sfd]->fd);
}




int snd_getchanformat(int sfd, channelformat *chformat)
{
    if(!mapsndfd(&sfd))
        return -1;

    return sf_getchanformat(sndfiles[sfd]->fd,chformat);

}


int sndreadpeaks(int sfd,int channels,CHPEAK peakdata[],int *peaktime)
{
    if(!mapsndfd(&sfd))
        return -1;

    return sfreadpeaks(sndfiles[sfd]->fd,(int) channels,peakdata,peaktime);
}



int sndputpeaks(int sfd,int channels,const CHPEAK peakdata[])
{
    if(!mapsndfd(&sfd))
        return -1;

    return sfputpeaks(sndfiles[sfd]->fd,channels,peakdata);
}

const char * snd_getfilename(int sfd)
{
    if(!mapsndfd(&sfd))
        return NULL;

    return sf_getfilename(sndfiles[sfd]->fd);
}

extern int sf_makepath(char path[], const char* sfname);

int snd_makepath(char path[], const char* sfname)
{
    return sf_makepath(path,sfname);
}

