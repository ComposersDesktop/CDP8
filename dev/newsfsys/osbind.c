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
 *  emulation of Misc Atari OS routines
 */

/*
 * Revision 1.2  1995/02/01  10:54:37  martin
 * Added CDP_MEMORY_BBSIZE to control Malloc(-1) result
 *
 * Revision 1.1  1994/10/31  15:46:36  martin
 * Initial revision
 *
 */
 /*RWD 16/1/96 changed hz200 to GetSystemTime() from GetLocalTime() */
 /*RWD 7/9/96 fixed bug in getdrivefreespace() */
#ifdef _WIN32
#include <windows.h>
#include <time.h>
#endif
#include <stdlib.h>
#include <osbind.h>
#if defined __GNUWIN32__ || defined unix
#include <ctype.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef __GNUWIN32__
#include <io.h>
#endif

#ifdef unix
#include <string.h>
#include <sys/param.h>
#include <sys/mount.h>
#endif
#ifdef linux
#include <stdint.h>
#include <sys/vfs.h>
#endif

/*RWD May 2005: has to be a signed value sadly!*/
/* RWD Jan 2014 not any longer... */
#define BIGFILESIZE (0xFFFFFFFFLU)


void *
Malloc(long size)
{
    char *bbs;
    unsigned long res;

    if(size != -1)
        return malloc(size);
    /* this is a request for the maximum block size */

    if((bbs = getenv("CDP_MEMORY_BBSIZE")) == NULL)
        return (void *)(1024*1024);
    res = atol(bbs);
    if(res < 10)
        res = 100;
    else if(res > 20*1024)
        res = 20 * 1024;
    res *= 1024;
    return (void *)res;
}

void
Mfree(void *ptr)
{
    free(ptr);
}

/************** WINDOWS ************************/

#ifdef _WIN32
unsigned int
hz200(void)
{
    unsigned long secs, ticks;
    SYSTEMTIME st;

    GetLocalTime(&st);
    ticks = st.wMilliseconds/(1000/200);
    secs = (st.wHour*60 + st.wMinute) * 60 + st.wSecond;

    return (unsigned int)(secs*200 + ticks);
}
/*TW*/
unsigned int
hz1000(void)
{
    unsigned long secs, ticks;
    SYSTEMTIME st;

    GetLocalTime(&st);
    ticks = st.wMilliseconds;
    secs = (st.wHour*60 + st.wMinute) * 60 + st.wSecond;
    return (unsigned int)( secs*1000 + ticks);
}

unsigned int
getdrivefreespace(const char *path)
{
    DWORD sectorspercluster;
    DWORD bytespersector;
    DWORD freeclusters;
    DWORD totclusters;
    __int64 freesectors;    /*RWD Jan 2014 */

    char pbuf[4];

    if(path[0] != '\0' && path[1] == ':') {
        pbuf[0] = path[0];
        pbuf[1] = ':';
        pbuf[2] = '\\';
        pbuf[3] = '\0';     /*RWD*/
        path = pbuf;
    } else
        path = 0;

    if(!GetDiskFreeSpace(path, &sectorspercluster, &bytespersector, &freeclusters, &totclusters))
        return 0;

    freesectors = (__int64)freeclusters * sectorspercluster;
    if(freesectors > BIGFILESIZE/bytespersector)
        return BIGFILESIZE; /* next line would overflow!! */
    return (unsigned int)(freesectors * bytespersector);
}

char *
legalfilename(char *filename)
{
    char *lastdot = strrchr(filename, '.');
    char *lastfs = strrchr(filename, '\\');
    DWORD maxcl;
    char path[4];
    char fstname[10];

    if(lastdot == 0 || lastfs == 0 || lastdot <= lastfs || filename[1] != ':')
        return "Illegal filename: internal error";
    path[0] = filename[0];
    path[1] = ':';
    path[2] = '\\';
    path[3] = '\0';

    if(!GetVolumeInformation(path,
                    (LPTSTR)0, 0,   /* volume name */
                    (LPDWORD)0, /* volume serial number */
                    &maxcl,     /* maximum component length */
                    (LPDWORD)0, /* filesystem flags */
                    fstname, 10 /* file system type name */
            ))
        return "Illegal filename: can't get volume information";

    if(strcmp(fstname, "FAT") == 0 && maxcl == 12) {/* don't know what vfat under windows4 will look like! */
        if(strchr(lastfs+1, '.') != lastdot)
            return "Illegal filename: FAT filenames cannot have more than 1 dot";
        if(strlen(lastdot+1) > 3)
            return "Illegal filename: FAT filenames cannot have more than 3 letters after dot";
        if(lastdot - (lastfs+1) > 8)
            return "Illegal filename: FAT filenames cannot have more than 8 letters before dot";
        return 0;
    }
    if(strlen(lastfs+1) > maxcl)
        return "Illegal filename: filename component is too long";
    return 0;
}

#endif

/***************************** OS X (at least)******************/

#ifdef unix
unsigned int
hz200(void)
{
    struct timeval tv;

    if(gettimeofday(&tv, (struct timezone *)0) < 0)
        abort();

    return (unsigned int)(tv.tv_sec*200 + tv.tv_usec/(1000000/200));
}
unsigned int
hz1000(void)
{
    struct timeval tv;

    if(gettimeofday(&tv, (struct timezone *)0) < 0)
        abort();

    return (unsigned int)(tv.tv_sec*1000 + tv.tv_usec/(1000000/1000));

}

unsigned int
getdrivefreespace(const char *path)
{
    int ret;
    uint64_t avail = 0; /*RWD Jan 2014 */
    struct statfs diskstat = {0};
    /* RWD we ignore path, and use current dir; cannot use non-existent file path anyway */
    ret = statfs(".",&diskstat);
    if(ret==0){
        if(diskstat.f_bfree > (uint64_t) BIGFILESIZE / diskstat.f_bsize)
            avail = BIGFILESIZE;
        else
            avail = diskstat.f_bsize * diskstat.f_bfree;
    }   
    return (unsigned int) avail;
}

int
_stricmp(const char *a, const char *b)
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
_strnicmp(const char *a, const char *b, size_t length)
{
    size_t len = length;

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


char *
_fullpath(char *buf, const char *filename, size_t maxlen)
{
    int mybuf = 0;

    if(filename[0] == '/') {
        if((buf = malloc(strlen(filename) + 1)) == 0)
            return (char *)filename;
        strcpy(buf, filename);
        return buf;
    }
    if(buf == 0) {
        mybuf++;
        maxlen = 2 * /*PATH_MAX*/ MAXPATHLEN;
        if((buf = malloc(maxlen)) == 0)
            return NULL;
    }

    if(getcwd(buf, maxlen) == NULL)
        return (char *)filename;

    if(strlen(buf) + strlen(filename) + 2 >= maxlen)
        return (char *)filename;

    strcat(buf, "/");
    strcat(buf, filename);

    if(mybuf)
        buf = realloc(buf, strlen(buf)+1);
    return buf;
}

char *
legalfilename(char *filename)
{
    return 0;
}
#endif


/* for both WIN32 and unix! */

void
initrand48(void)
{
    srand(time((time_t *)0));
}

double
drand48(void)
{
    return (double)rand()/(double)RAND_MAX;
}

