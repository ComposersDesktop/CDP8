/*
 * Copyright (c) 1983-2013 Trevor Wishart and Composers Desktop Project Ltd
 * http://www.trevorwishart.co.uk
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
 * get Nth column from a file of columned data
 * (skipping M lines at start of file)
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#define PROG     "GETCOL"
#define ENDOFSTR '\0'

FILE    *fp1, *fpout;
char    *skip_column(char *,int,char *);
char    *skip_column_wanted(char *,int,char *);
void    usage(void), logo(void);

int             ignore_s_and_e = 0, ignore_c = 0;
int             sloom = 0;
int             sloombatch = 0;
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
    char temp[2000], *p, *q;
    int n, skip=0, column, cnt=0;

    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    /* VERSION 3 */
    if(argc < 2)
        usage();
    if(!strcmp(argv[1],"#")) {
        sloom = 1;
        argv++;
        argc--;
    } else if (!strcmp(argv[1],"##")) {
        p = argv[0];
        argc--;
        argv++;
        argv[0] = p;
        sloombatch = 1;
    }




    if(!strcmp(argv[argc-1],"-e")) {
        ignore_s_and_e = 1;
        argc--;
    } else  if(!strcmp(argv[argc-1],"-ec")) {
        ignore_c = 1;
        argc--;
    }
    if(argc<4 || argc>5)
        usage();
    if((fp1 = fopen(argv[1],"r"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open file '%s'\n",argv[1]);
        fflush(stdout);
        usage();
    }
    if((fpout = fopen(argv[2],"w"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open file '%s'\n",argv[2]);
        fflush(stdout);
        usage();
    }
    if(sscanf(argv[3],"%d",&column)!=1) {
        fprintf(stdout,"ERROR: Cannot read column value.\n");
        fflush(stdout);
        usage();
    }
    if(column<1) {
        fprintf(stdout,"ERROR: Column value cannot be less than 1.\n");
        fflush(stdout);
        usage();
    }
    column--;
    if(argc==5 && sscanf(argv[4],"%d",&skip)!=1) {
        fprintf(stdout,"ERROR: Cannot read skip value.\n");
        fflush(stdout);
        usage();
    }
    for(n=0;n<skip;n++) {
        if(fgets(temp,200,fp1)!=temp) {
            fprintf(stdout,"ERROR: Failed to get skippable line %d in file '%s'\n",
                    n+1,argv[1]);
            fflush(stdout);
            exit(1);
        }
        q = temp;                               /* IGNORE EMPTY LINES */
        p = temp;
        while(*p != ENDOFSTR) {
            if(isspace(*p))
                q++;
            p++;
        }
        if(p==q)
            n--;
    }
    while(fgets(temp,2000,fp1)!=NULL) {
        cnt++;
        p = temp;
        /*RWD*/ while(isspace(*p)) p++;
        /*and*/ if(*p=='\n' || *p==ENDOFSTR){
            if(!sloom && !sloombatch)
                fputs("\n",fpout);
            else
                fprintf(stdout,"INFO: \n");
            continue;
        }
        if(ignore_s_and_e && (!strcmp(temp,"e\n") || !strcmp(temp,"e") || !strcmp(temp,"s\n") || !strncmp(temp,";",1)))
            continue;
        if(ignore_c && !strncmp(temp,";",1))
            continue;
        for(n=0;n<column;n++) {
            if(*p==ENDOFSTR) {
                fprintf(stdout,"ERROR: Not enough columns in line %d.\n",cnt);
                fflush(stdout);
                exit(1);
            }
            p = skip_column(p,cnt,argv[1]);
        }
        q = p;
        if(*p==ENDOFSTR) {
            fprintf(stdout,"ERROR: Not enough columns in line %d.\n",cnt);
            fflush(stdout);
            exit(1);
        }
        p = skip_column_wanted(p,cnt,argv[1]);
        if(*p==ENDOFSTR) {
            p--;    /* DELETE NEWLINE */
            if(*p!='\n')
                p++;
        }
        *p = ENDOFSTR;
        if(!sloom && !sloombatch) {
            fputs(q,fpout);
            fputs("\n",fpout);
        } else
            fprintf(stdout,"INFO: %s\n",q);
    }
    fflush(stdout);
    return 0;
}

/******************************** USAGE *****************************/


void usage(void)
{
    if(!sloom && !sloombatch) {
        logo();
        fprintf(stderr,"USAGE : getcol infile outfile colno [skiplines] [-e]\n\n");
        fprintf(stderr,"skiplines  Ignores this number of lines at start of file.\n");
        fprintf(stderr,"-e         Ignores lines with a single 'e' or 's' (as in CSound scores)\n");
        fprintf(stderr,"           and comment lines starting with ';'\n");
        fprintf(stderr,"           (skipped lines should still be counted).\n");
    }
    exit(1);
}

/*************************** SKIP_COLUMN **************************/

char *skip_column(char *p,int cnt,char *argv)
{
    if(*p==ENDOFSTR) {
        fprintf(stdout,"ERROR: Not enough columns in line %d file %s\n",cnt,argv);
        fflush(stdout);
        exit(0);
    }
    while(!isspace(*p)) {
        p++;
        if(*p==ENDOFSTR)
            return(p);
    }
    while(isspace(*p)) {
        p++;
        if(*p==ENDOFSTR)
            return(p);
    }
    return(p);
}

/*************************** SKIP_COLUMN_WANTED **************************/

char *skip_column_wanted(char *p,int cnt,char *argv)
{
    if(*p==ENDOFSTR) {
        fprintf(stdout,"ERROR: Not enough columns in line %d file %s\n",cnt,argv);
        fflush(stdout);
        exit(0);
    }
    while(!isspace(*p++)) {
        if(*p==ENDOFSTR)
            return(p);
    }
    return(p);
}

/******************************** LOGO() **********************************/

void logo(void)
{
    printf("\t    ***************************************************\n");
    printf("\t    *            COMPOSERS DESKTOP PROJECT            *\n");
    printf("\t                    %s $Revision: 1.2 $\n",PROG);
    printf("\t    *                                                 *\n");
    printf("\t    *   get Nth column from a file of columned data   *\n");
    printf("\t    *      (skipping M lines at start of file).       *\n");
    printf("\t    *                                                 *\n");
    printf("\t    *                by TREVOR WISHART                *\n");
    printf("\t    ***************************************************\n\n");
}
