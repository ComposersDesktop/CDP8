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



/*alias.c: WIN32 only */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alias.h"
#ifndef _MAX_PATH
#define _MAX_PATH (255)
#endif


#ifdef TESTING
int main(int argc, char **argv)
{

    char linkfile[_MAX_PATH];
    char path[_MAX_PATH];
    char desc[_MAX_PATH];
    char tmp[_MAX_PATH];
    char *dotp = NULL, *targetname;

    if(argc < 2){
        printf("\nusage: alias file.lnk");
        return 1;
    }
    linkfile[0] = path[0] = desc[0] = tmp[0] = '\0';

    strcpy(linkfile,argv[1]);
    targetname = linkfile;
    /*does name have .lnk extension?*/
    dotp = strrchr(linkfile, '.');
    if(dotp == NULL || (dotp != NULL && _stricmp(dotp, ".lnk") != 0)) {
        //add .lnk extension
        strcpy(tmp,linkfile);
        strcat(tmp,".lnk");                     /*TODO: check strlen(linkfile) !*/
        targetname = tmp;
    }
    /* check if the file exists             */
    if(!fileExists(targetname)) {
        printf("\ncannot find a link file with that name!");
        return 1;
    }
    /*and get the target    */
    if(!getAliasInfo(targetname,path,desc)){
        printf("\nfailed to get link info: is it a real shortcut?");
        return 1;
    }

    printf("\nPath = %s\nDescr = %s\n",path,desc);

    return 0;
}
#endif

/*return 0 for failure, 1 for success*/
int getAliasName(const char *name, char *newname)
{
    char *dotp = NULL, *targetname =  (char *) name;
    char tmp[_MAX_PATH],desc[_MAX_PATH];
    if(name==NULL || newname==NULL)
        return 0;
    tmp[0] = '\0';
    dotp = strrchr(targetname, '.');
    if(dotp == NULL || (dotp != NULL && _stricmp(dotp, ".lnk") != 0)) {
        /*add .lnk extension    */
        strcpy(tmp,name);
        strcat(tmp,".lnk");                     /*TODO: check strlen(linkfile) !*/
        targetname = tmp;
    }
    /* check if the file exists             */
    if(!fileExists(targetname)) {
        newname=NULL;
        return 0;
    }
    /*and get the target    */
    if(!getAliasInfo(targetname,newname,desc)){
        newname[0] = '\0';
        return 0;
    }

    return 1;
}
