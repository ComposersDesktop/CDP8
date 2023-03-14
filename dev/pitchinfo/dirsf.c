/*
 * Copyright (c) 1983-2023 Martin Atkins, Richard Dobson and Composers Desktop Project Ltd
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
 *  Sound Filing System Utility - Directory
 *
 *  Copyright M. C. Atkins, 1986, 1987
 *  All Rights Reserved.
 */
/* RWD identify B-Format files */

/*
 *  $Log: dirsf.c%v $


 * Revision 4.1 1.99 RWD full sfsysEx version: support all formats

 * Revision 4.0 8.98 RWD
 * add -h and -? flags, adapt -d usage for Win32
 * link with sfsys97 for all cdp extensions


 * Revision 3.5  1994/11/12  20:09:02  martin
 * Fix rcs string
 *
 * Revision 3.4  1994/10/31  16:52:56  martin
 * Make rcs version number consistent with released versions
 *
 * Revision 1.1  1994/10/31  16:49:56  martin
 * Initial revision
 *
 */
/*****

 * RWD OCT97: Revision 3.6: under CDP97 (sfsys97.lib) recompile to recognize 8bit sfiles
 * RWD FEB98: recognise WIN32 shortcuts. Gives name of shortcut (with .lnk extension),
 * but statistics of the aliased file. Don't know what to do if the shortcut name's unrelated to the
 * target - not enough room on a console for two filenames! 
 * RWD.9.98 removed -p option (for WIN32) - Atari-death!
 * RWD.6.99 link with sfsysEx, report many more formats!
 * RWD 8.09 fixed report of length > 2GB (sfsysEx64) 
 * JAN 2013  rebuild with fixed getsfsysadtl bug in sfsys (sfsys2010)
 *****/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#if defined(_WIN32) || defined(__SC__)
#include <direct.h>
#include <ctype.h>
#endif
#ifdef unix
#include <unistd.h>
#endif
#include <sfsys.h>     
#include "sffuncs.h"
#include "wildcard.h"
static char *VERSION = "DIRSF: Revision 7.0 (c) 1999,2005,2014 CDP";
char    cdp_ver_str[]="~CDPVER~dirsf $Revision: 7.0 1999,2005,2009,2014 $";

char longhdr[] = "Bytes\t\tChannel\tSample\tRate\tSeconds\tfmt Name\n\n";
/* char posnhdr[] = "Flags\tFirst\tSectors\tName\n\n"; */
/* char sdisk[]   = "\t%d\t\t---- start of disk: device %d sector %d\n"; */

int SFCALLS lpr(struct sf_direct *);
/*int SFCALLS pospr(struct sf_direct *);*/
int SFCALLS pr(struct sf_direct *);
int SFCALLS tpr(struct sf_direct *);
int preflg = 0;     /* all prefixes flag */
int prefix_set = 0; /* there currently is a prefix set */

struct filename {
    struct wildcard *pat;   /* non-zero => pattern */
    char *pic;      /* argv string */
    struct filename *next;  /* chained up */
};

struct filename *filelist = 0;

struct rdskcfg cfg;
struct partinfo *pi = &cfg.pinfo[0];

#define FSTVERWPREF (0x201L)

void
usage()
{
    fprintf(stderr,
#ifdef ATARI
        "usage: dirsf [-g][-l or -p or -t] [-d prefix] [filespec ...]\n");
#else
        "\n%s\nusage: dirsf [-l or -t] [-d path] [filespec ...]\n",VERSION);
#endif
#ifdef ATARI
    fprintf(stderr, "\tg - list files with all prefixes\n");
#endif
    fprintf(stderr, "\tl - list names only\n"); 
    fprintf(stderr, "\tt - list modification dates and times\n");
#ifdef ATARI
    fprintf(stderr, "\td - list only files with the given prefix\n");
#else
    fprintf(stderr, "\td - list files in directory path\n");
#endif
    exit(1);
}

void
isprefix()
{
    char buf[MAXPREFIX];

    sfgetprefix(buf);

    prefix_set = (buf[0] != '\0');
}

void
mkfilelist(char **argv)
{
    struct filename **thisp = &filelist;

    while(*argv != 0) {
        if((*thisp = (struct filename *)
                malloc(sizeof(struct filename))) == 0) {
            fprintf(stderr, "dirsf: can't get memory for file list\n");
            exit(1);
        }
        (*thisp)->next = 0;
        (*thisp)->pic = *argv;
        if(!iswildcard(*argv))
            (*thisp)->pat = 0;
        else if(((*thisp)->pat = wildcomp(*argv)) == 0) {
            fprintf(stderr, "dirsf: can't compile pattern %s\n", *argv);
            exit(1);
        }
        thisp = &(*thisp)->next;
        argv++;
    }
}

PROGRAM_NUMBER(0x94589004);

int
main(int argc, char *argv[])
{
    int rc = SFDIR_NOTFOUND; /* a guess, to suppress warning */
    int option = 0;
    int cded = 0;       /* did we cd? */    
#if defined(_WIN32) || defined(__SC__)
    char opath[MAXPREFIX];
    int olddrive;
    int newdrive;
    char *dircp;
#endif

    if(sflinit("dirsf")) {
        sfperror("dirsf: initialisation");
        exit(1);
    }
    if(sfverno() < FSTVERWPREF)
        fprintf(stderr, "cdfs: warning, installed sfsys does not support prefixes\n");
    isprefix();
    if(argc > 1 && strcmp(argv[1], "-g") == 0) {
        preflg = SFDIR_IGPREFIX;
        argc--;
        argv++;
    }
    if(argc > 1 && argv[1][0] == '-')
        switch(argv[1][1]) {
        case 'l':       
        case 't':
            option = argv[1][1];
            argc--;
            argv++;
            break;
        case 'd':
            break;
        case 'h':           //RWD 7.98
        case '?':
            usage();
        default:
            usage();
    }
    if(argc > 1 && strcmp(argv[1], "-d") == 0) {
        if(argc < 3)
            usage(); 
#if defined(unix)
        cded++;
        if(chdir(argv[2]) < 0) {
            fprintf(stderr, "dirsf: can't change directory\n");
            exit(1);
        }
#elif defined(_WIN32) || defined(__SC__)
        olddrive = _getdrive();
        if(argv[2][1] == ':') {
            newdrive = toupper(argv[2][0]) - 'A' + 1;
            dircp = &argv[2][2];
        } else {
            newdrive = olddrive;
            dircp = &argv[2][0];
        }
        if(_chdrive(newdrive) < 0
         ||_getcwd(opath, MAXPREFIX) == NULL
         ||((dircp[0] != '\0') ? _chdir(dircp) : 0) < 0) {
            fprintf(stderr, "dirsf: error changing current directory\n");
            exit(1);
        }                                                
        cded++;
#else
#error "Unknown system"
#endif
        argc -= 2;
        argv += 2;
    }
    if(argc > 1)
        mkfilelist(&argv[1]);
        
    switch(option) {
    case 0:
        printf("Sfsys Version %x.%02x\n", (unsigned int)(sfverno()>>8), (unsigned int)(sfverno()&0xff));
        puts( longhdr);
        rc = sfdir(lpr, SFDIR_USED|preflg);
        break;
    case 'l':
        rc = sfdir(pr, SFDIR_USED|preflg);
        break;
    case 't':
        rc = sfdir(tpr, SFDIR_USED|preflg);
        break;
    default:
        usage();
    }

#if defined(_WIN32) || defined(__SC__)
    if(cded) {
        if(_chdir(opath) < 0
         ||_chdrive(olddrive) < 0)
            fprintf(stderr, "dirsf: can't restore current path\n");
    }
#endif
    if(rc != SFDIR_NOTFOUND) {
        sfperror("dirsf");
        exit(1);
    }
//  sffinish();
    return 0;
}

/*
 *  do we want this file?
 */
int
dofile(char *name)
{
    char *basename;
    struct filename *this;

    if(filelist == 0)
        return 1;
    if((basename = strrchr(name, '\\')) != 0)
        basename++;
    else
        basename = name;

    for(this = filelist; this != 0; this = this->next) {
        if(this->pat != 0) {
            if(wildmatch(prefix_set ? basename : name, this->pat))
                return 1;
        } else if(strcmp(this->pic, basename) == 0)
                return 1;
    }
    return 0;
}

/*
 *  Just print the filename
 */
int SFCALLS
pr(struct sf_direct *dirp)
{
#ifdef NOTDEF
    if(dofile(dirp->name))
        printf("%s\n", dirp->name);
#else
    if(dofile(dirp->name)) {
#ifdef _WIN32
        if(dirp->is_shortcut)
            printf("%s\t\t%s\n",dirp->name,dirp->targetname);
        else
#endif
        printf("%s\n", dirp->name);
    }
#endif
    return 0;
}

/*
 *  Get the date/time for a file
 */
int
getdateprop(char *name, unsigned int *timep)
{
    int rc;
    int sfd = sndopenEx(name,0,CDP_OPEN_RDONLY);
    
    if(sfd < 0)
        return 0;
    rc = sndgetprop(sfd, "DATE", (char *)timep, sizeof(int)) == sizeof(int);
    sndcloseEx(sfd);
    return rc;
}

/*
 *  Print date/time and name
 */
int SFCALLS
tpr(struct sf_direct *dirp)
{
    time_t time;
    unsigned int cdptime;
    char *times;
    
    if(!dofile(dirp->name))
        return 0;
    
    if(getdateprop(dirp->name, &cdptime)) {
            time = (time_t) cdptime;
        times = ctime(&time);
        times[24] = '\0';
        printf("%-26s\t", times);
    } else
        printf("%-26s\t", "-");
#ifdef _WIN32   
    if(dirp->is_shortcut)
        printf("%s\t%s\n",dirp->name,dirp->targetname);
    else
#endif
        printf("%s\n", dirp->name);
    return 0;
}

/*
 *  print long information
 */
int SFCALLS
lpr(struct sf_direct *p)
{
    int nosec = 0;
    int scale;

    if(!dofile(p->name))
        return 0;
    if(p->flags == SFFLG_IOERROR)     /* RWD Aug 2009 solve the unsigned issue here */
        printf("%d\t",-1);
    else
        printf("%9u\t", p->length);    /*RWD Aug 2009: print as unsigned! */
    switch(p->nochans) {
    case '1':
        printf("mono\t");
        break;
    case '2':
        printf("stereo\t");
        break;
    default:
        if(p->nochans < 0) {
            printf("-\t");
            nosec++;
        } else
            printf("%d\t", p->nochans);
        break;
    }
    switch(p->samptype) {
    case SAMP_SHORT:
        if(p->origwordsize==8)
            printf("8bit \t");
        else
            printf("short\t");
        scale = sizeof(short); //RWD: keep until I fully define SAMP_BYTE 
        break;
    case SAMP_FLOAT:
        printf("float\t");
        scale = sizeof(float);
        break;
    case SAMP_LONG:
        printf("long\t");
        scale = sizeof(long);
        break;
    case SAMP_2424:
        printf("24(p) \t");
        scale = 3;
        break;
    case SAMP_2432:
        printf("24:32\t");
        scale = sizeof(long);
        break;
    case SAMP_2024:
        printf("20:24\t");
        scale = 3;
        break;
    default:
        if(p->samptype < 0)
            printf("-\t");
        else
            printf("?\t");
        nosec++;
        break;
    }
    if(p->samprate > 0)
        printf("%5d\t", p->samprate);
    else {
        printf("-\t");
        nosec++;
    }
    if(nosec)
        printf("-\t");
    else {
        float time = (float)p->length;
        if(p->origwordsize==8)
             time *= 2.0f;

        time /= (float)(scale * p->nochans * p->samprate);
        printf("%7.3f\t", time);
    }
    if(p->fmt== WAVE)
        printf(" W  ");
    else if(p->fmt== WAVE_EX) {
        channelformat fmt = -1;
        int sfd;
        sfd = sndopenEx(p->name,0,CDP_OPEN_RDONLY);
        if(sfd >=0){
            snd_getchanformat(sfd, &fmt);
            if(fmt >=0){
                if(fmt==MC_BFMT)
                    printf(" WB ");
                else
                    printf(" WX ");
            }
            sndcloseEx(sfd);

        }
        else
            printf(" WX ");
    }
    else if(p->fmt==AIFF)
        printf(" A  ");
    else if(p->fmt==AIFC)       
        printf(" AC ");
#ifdef ENABLE_PVX
    else if(p->fmt==PVOCEX)
        printf(" PX ");
#endif
    else    //FMT_UNKNOWN
        printf(" ?  ");

    printf("%s\n", p->name);
    return 0;
}
