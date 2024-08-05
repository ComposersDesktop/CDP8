/* RWD small mods for newsfsys, no need for FILE_AMB_SUPPORT symbol */
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
 *      portable sfsys - Unix version of sfdir, etc
 */

/*
 *      $Log: sfdir.c%v $
 * Revision 1.1  1994/10/31  15:41:38  martin
 * Initial revision
 *
 */
/*RWD OCT97: rebuild under CDP97 to recognise *.ana as analysis file*/
/* RWD Dec 2009 restored fix to fmt info code! NB some sf_direct fields now int or unsigned int*/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <osbind.h>
#include <sfsys.h>      /*RWD: don't want local copies of this!*/
#include "sffuncs.h"
#if defined(_SGI_SOURCE) || defined unix
#include <dirent.h>
#else
#include "scandir.h"
#endif

#if defined WIN32 || defined linux
static int
selfn(const struct dirent *d)
#else
    static int
    selfn(const struct dirent *d)
#endif
{
    char *dotp = strrchr(d->d_name, '.');

    if(dotp == 0)
        return 0;
    if(_stricmp(dotp, ".aif") == 0
       ||_stricmp(dotp, ".aiff") == 0
       ||_stricmp(dotp, ".wav") == 0
       /*RWD.1.99 add AIFC - just how many xtensions can there be?*/
       || _stricmp(dotp,".afc") == 0                   /*official Apple*/
       || _stricmp(dotp,".aifc") ==0                   /* unix?*/
       || _stricmp(dotp,".aic") == 0                   /* lots of other people...*/
       ||_stricmp(dotp, ".amb") == 0
       ||_stricmp(dotp, ".wxyz") == 0
       /*RWD OCT97;JULY98: recognise TW filetypes*/
       || _stricmp(dotp,".ana") == 0
#ifdef ENABLE_PVX
       || _stricmp(dotp,".pvx") == 0
#endif
       || _stricmp(dotp,".frq") == 0
       || _stricmp(dotp,".fmt") == 0          /* lose this in time */
       || _stricmp(dotp,".for") == 0
       || _stricmp(dotp,".env") == 0          /* lose this in time */
       || _stricmp(dotp,".evl") == 0
       || _stricmp(dotp,".trn") == 0
       || _stricmp(dotp,".lnk") == 0    /*RWD98 VERY experimental ; may be wrong place to do it, or wrong way...*/
       )

        return 1;
    return 0;
}

int
sfdir(int SFCALLS (*func)(struct sf_direct *filedetails), int flags)
{
    struct dirent **filelist = 0;
    int numfiles, file;
    int fdes;
    int rc = SFDIR_NOTFOUND;
    struct sf_direct dir;

    if((numfiles = scandir(".", &filelist, selfn, alphasort)) < 0)
        return SFDIR_ERROR;

    if(numfiles == 0)
        return SFDIR_NOTFOUND;
    if(filelist == 0)
        abort();           /*RWD: ouch!*/

    for(file = 0; file < numfiles; file++) {
        struct dirent *f = filelist[file];
        if((fdes = sfopenEx(f->d_name,CDP_OPEN_RDONLY)) >= 0){
            dir.flags = 0;
            dir.length = (unsigned int) sfsize(fdes);    /*RWD 2007 */
            dir.index = 0;
            dir.seclen = dir.length>>LOGSECSIZE;
#if defined _WIN32
            dir.is_shortcut = sf_is_shortcut(fdes,dir.targetname);
            if(!dir.is_shortcut)
                dir.targetname[0] = '\0';
#endif
            sfformat(fdes,&(dir.fmt));
            strncpy(dir.name, f->d_name, MAXSFNAME-1);
            dir.name[MAXSFNAME-1] = '\0';
            if(sfgetprop(fdes, "sample type", (char *)&dir.samptype, sizeof(int)) != sizeof(int))
                dir.samptype = -1;
            if(sfgetprop(fdes, "sample rate", (char *)&dir.samprate, sizeof(int)) != sizeof(int))
                dir.samprate = -1;
            if(sfgetprop(fdes, "channels", (char *)&dir.nochans, sizeof(int)) != sizeof(int))
                dir.nochans = -1;
            dir.origwordsize = sfgetwordsize(fdes);
            if(dir.origwordsize==8)
                dir.length /= 2;
            sfclose(fdes);

            if(func(&dir)) {
                rc = SFDIR_FOUND;
                break;
            }
        } else {
            if(sferrno() != ESFNOTFOUND) {
                rc = SFDIR_ERROR;
                /*RWD.9.98      for dirsf*/
                dir.fmt = FMT_UNKNOWN;
                /*break;*/

            }
            dir.flags = SFFLG_IOERROR;
            dir.length = 0xffffffff; /* RWD Aug 2009: unsigned equiv of -1 . need to improve on this system ere long */
            dir.index = 0;
            dir.seclen = -1;
            strncpy(dir.name, f->d_name, MAXSFNAME-1);
            dir.name[MAXSFNAME-1] = '\0';
            dir.samptype = -1;
            dir.samprate = -1;
            dir.nochans = -1;

            if(func(&dir)) {
                rc = SFDIR_FOUND;
                break;
            }
        }
    }

    for(file = 0; file < numfiles; file++)
        free(filelist[file]);
    free(filelist);
    return rc;
}

int
rdiskcfg(struct rdskcfg *cfg)
{
    cfg->disksize = 1024*1024;      /* we lie! */
    cfg->npart = 1;
    cfg->pinfo[0].unit_no = 0;
    cfg->pinfo[0].phys_start = 0;
    cfg->pinfo[0].sf_start = 0;
    cfg->pinfo[0].sf_end = 1024*1024-1;
    return 0;
}
