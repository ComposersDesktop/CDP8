/*
 * Copyright (c) 1983-2023 Martin Atkins and Composers Desktop Project Ltd
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
 *  Sound Filing System - Library
 *
 *  wildcard - sound file wildcards
 *
 *  Copyright M. C. Atkins, April 1987
 *  All Rights Reserved.
 */

//static char *rcsid = "$Id: wildcard.c%v 1.1 1994/10/31 16:49:56 martin Exp $";
/*
 *  $Log: wildcard.c%v $
 * Revision 1.1  1994/10/31  16:49:56  martin
 * Initial revision
 *
 */

#include <stdlib.h>
#include <string.h>
#include <sfsys.h>
#include "sffuncs.h"
#include "wildcard.h"
//static char *wildcard_h_rcsid = WILDCARD_H_RCSID;

struct wildcard {
    int code;
    struct wildcard *next;
    int arglen;
    char arg[1];
};

int
iswildcard(char *filename)
{
    while(*filename != '\0') {
        switch(*filename++) {
        case '?':
        case '*':
        case '[':
        case '\\':
            return 1;
        }
    }
    return 0;
}

void
freeargv(char **a)
{
    char **ap = a;

    while(*ap != 0)
        free(*ap++);
    free(a);
}

struct wildcard *
mkwildcard(int code, char *str)
{
    int len = (str != 0) ? strlen(str) : 0;
    struct wildcard *res = (struct wildcard *)
                malloc(sizeof(struct wildcard) + len);
    if(res == 0)
        return 0;
    res->code = code;
    res->next = 0;
    res->arglen = len;
    if(str != 0)
        strcpy(res->arg, str);
    return res;
}

void
wildfree(struct wildcard *wld)
{
    struct wildcard *next;

    while(wld != 0) {
        next = wld->next;
        free(wld);
        wld = next;
    }
}

struct wildcard *
wildcomp(char *str)
{
    struct wildcard *res = 0;
    struct wildcard **thisp = &res;
    char buf[400];
    char *bp = buf;

    while(str[0] != '\0') {
        switch(str[0]) {
        case '?':
        case '*':
            if(bp != buf) {
                *bp++ = '\0';
                if((*thisp = mkwildcard('A', buf)) == 0)
                    return 0;
                thisp = &(*thisp)->next;
                bp = buf;
            }
            if((*thisp = mkwildcard(*str, 0)) == 0)
                return 0;
            thisp = &(*thisp)->next;
            break;
        case '\\':
            if(str[1] == '\0')
                *bp++ = '\\';
            else
                *bp++ = *++str;
            break;
        case '[':
            if(bp != buf) {
                *bp++ = '\0';
                if((*thisp = mkwildcard('A', buf)) == 0)
                    return 0;
                thisp = &(*thisp)->next;
                bp = buf;
            }
            while(*++str != ']') {
                switch(str[0]) {
                case '\0':
                    wildfree(res);
                    return 0;
                case '-':
                    if(bp == buf || str[1] == ']' || str[1] == '\0')
                        *bp++ = '-';
                    else {
                        int from = bp[-1] + 1;
                        int to = str[1];

                        while(from <= to)
                            *bp++ = (char) from++;
                        str++;
                    }
                    break;
                case '\\':
                    if(str[1] == '\0')
                        *bp++ = '\\';
                    else
                        *bp++ = *++str;
                    break;
                default:
                    *bp++ = *str;
                    break;
                }
            }
            *bp++ = '\0';
            if((*thisp = mkwildcard('[', buf)) == 0)
                return 0;
            thisp = &(*thisp)->next;
            bp = buf;
            break;
        default:
            *bp++ = *str;
            break;
        }
        str++;
    }

    if(bp != buf) {
        *bp++ = '\0';
        if((*thisp = mkwildcard('A', buf)) == 0)
            return 0;
    }
    return res;
}

static int SFCALLS dofile(struct sf_direct *p);
static char **matcharr;
static char **nxtmatch;
static struct wildcard *pat;

char **
wildmatchall(struct wildcard *wild)
{
    if((matcharr = (char **)malloc(MAXMATCH * sizeof(char *))) == 0)
        return 0;
    nxtmatch = &matcharr[0];
    pat = wild;

    if(sfdir(dofile, SFDIR_USED) != SFDIR_NOTFOUND) {/* an error occured */
        char **t = matcharr;

        while(t < nxtmatch)
            free(*t++);
        free(matcharr);
        return 0;
    }
    *nxtmatch++ = 0;
    if((matcharr = (char **)realloc(matcharr, (nxtmatch-matcharr)*sizeof(char *))) == 0)
        return 0;
    return matcharr;
}

static int SFCALLS
dofile(struct sf_direct *p)
{
    char *res;

    if(wildmatch(p->name, pat)) {
        if((res = malloc(strlen(p->name) + 1)) == 0
         ||nxtmatch >= &matcharr[MAXMATCH])
            return 1;
        strcpy(res, p->name);
        *nxtmatch++ = res;
    }
    return 0;
}

char **
wildexpand(char *filename)
{
    struct wildcard *wld;
    char **res;
    char prefix[MAXPREFIX];
    char buf[MAXPREFIX + 100];

    if(filename[0] != '\\') {
        sfgetprefix(prefix);
        if(prefix[0] != '\0') {     /* prefix is set */
            char *fp = prefix;
            char *tp = buf;

            do {
                *tp++ = *fp;
                if(*fp == '\\')
                    *tp++ = '\\';
            } while(*fp++ != 0);
                
            strcat(buf, "\\\\");
            strcat(buf, filename);
            filename = buf;
        }
    }
    if((wld = wildcomp(filename)) == 0)
        return 0;

    res = wildmatchall(wld);
    wildfree(wld);
    return res;
}

int SFCALLS
wildmatch(char *filename, struct wildcard *wild)
{
    char *cp = filename;
    int len;

    while(*cp != '\0' && wild != 0) {
        switch(wild->code) {
        case 'A':
            if(strncmp(cp, wild->arg, wild->arglen) != 0)
                return 0;
            cp += wild->arglen;
            break;
        case '?':
            cp++;
            break;
        case '[':
            if(strchr(wild->arg, *cp) == 0)
                return 0;
            cp++;
            break;
        case '*':
            len = 0;
            do {
                if(wildmatch(&cp[len], wild->next))
                    return 1;
            } while(cp[len++] != 0);
            return 0;
        }
        wild = wild->next;
    }
    return wild == 0 && *cp == 0;
}
