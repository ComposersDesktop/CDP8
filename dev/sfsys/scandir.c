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
 *      portable sfsys - NT version of scandir
 */

static char *rcsid = "$Id: scandir.c%v 1.2 1994/10/31 15:56:04 martin Exp $";
/*
 *      $Log: scandir.c%v $
 * Revision 1.2  1994/10/31  15:56:04  martin
 * Fix local copy of rcsid string
 *
 * Revision 1.1  1994/10/31  15:46:36  martin
 * Initial revision
 *
 */

#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "scandir.h"
static char *scandir_h_rcsid = SCANDIR_H_RCSID;

#define ARRAY_INCR      (10)

#ifdef _WIN32

int
scandir(const char *dir,
        struct dirent ***namelist,
        int (*selfn)(const struct dirent *d),
        int (*srtfn)(const struct dirent * const *d1,
                     const struct dirent * const *d2))
{
    struct dirent **names = (struct dirent **)malloc(1);
    int names_size = 0;
    int names_seen = 0;
    HANDLE shndl;
    struct dirent f;
    WIN32_FIND_DATA filedata;
    char pattern[_MAX_PATH];

    if(names == 0)
        return -1;
    strcpy(pattern, dir);
    strcat(pattern, "\\*.*");

    if((shndl = FindFirstFile(pattern, &filedata)) == INVALID_HANDLE_VALUE) {
        if(GetLastError() == ERROR_FILE_NOT_FOUND)
            return 0;
        return -1;
    }

    for(;;) {
        strcpy(f.d_name, filedata.cFileName);
        if(selfn(&f)) {
            if(names_seen >= names_size) {
                names_size += ARRAY_INCR;
                if((names = (struct dirent **) realloc(names, names_size*sizeof(struct dirent *))) == 0) {
                    FindClose(shndl);
                    return -1;
                }
            }
            if((names[names_seen] = (struct dirent *)malloc(sizeof(struct dirent))) == 0) {
                FindClose(shndl);
                return -1;
            }
            *names[names_seen++] = f;
        }

        if(!FindNextFile(shndl, &filedata)) {
            if(GetLastError() != ERROR_NO_MORE_FILES) {
                FindClose(shndl);
                return -1;
            }
            break;
        }
    }

    FindClose(shndl);
    qsort(names, names_seen, sizeof(struct dirent *),
          (int (*)(const void *, const void *))srtfn);
    *namelist = names;
    return names_seen;
}
#endif

#ifdef unix
int
scandir(const char *dir,
        struct dirent ***namelist,
        int (*selfn)(const struct dirent *d),
        int (*srtfn)(const struct dirent * const *d1,
                     const struct dirent * const *d2))
{
    DIR *dirfd;
    struct dirent *f;
    struct dirent **names = (struct dirent **)malloc(1);
    int names_size = 0;
    int names_seen = 0;

    if(names == 0)
        return -1;
    if((dirfd = opendir(".")) == NULL)
        return -1;

    while((f = readdir(dirfd)) != NULL) {
        if(!selfn(f))
            continue;
        if(names_seen >= names_size) {
            names_size += ARRAY_INCR;
            if((names = realloc(names, names_size*sizeof(struct dirent *))) == 0) {
                closedir(dirfd);
                return -1;
            }
        }
        if((names[names_seen] = (struct dirent *)malloc(sizeof(struct dirent))) == 0) {
            closedir(dirfd);
            return -1;
        }
        *names[names_seen++] = *f;
    }
    closedir(dirfd);
    qsort(names, names_seen, sizeof(struct dirent *),
          (int (*)(const void *, const void *))srtfn);
    *namelist = names;
    return names_seen;
}

#endif



int
alphasort(const struct dirent * const *d1,
          const struct dirent * const *d2)
{
    return strcmp((*d1)->d_name, (*d2)->d_name);
}
