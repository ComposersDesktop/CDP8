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
 * Insert a column of figures, in a textfile,
 * into an existing file
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>

FILE *fp2, *fp1, *fpout;

#define PROG    "PUTCOL"
#define NEW_COL_SPACE (128)

#define ENDOFSTR '\0'
#define NEWLINE  '\n'
#define TAB      '\t'

void    usage(void), logo(void);
char    *skip_column(char *,int,char *);
int     sloom = 0;
int     sloombatch = 0;

int             ignore_s_and_e = 0, ignore_c = 0;

char    errstr[400];
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
    char temp[400], temp2[400], *p, c;
    int n, skip = 0, column, cnt=0, replace = 0, end_col = 0, keep = 1, outlinecnt, newlen;
    char **line = (char **)0;
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
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
    }
    if(!strcmp(argv[argc-1],"-ec")) {
        ignore_c = 1;
        argc--;
    }
    if(argc<6 || argc>7)
        usage();

    if(!sloom) {
        if(!strcmp(argv[3],argv[2]) || !strcmp(argv[3],argv[1])) {
            fprintf(stdout,"ERROR: Cannot use same file for input and ouput.\n");
            fflush(stdout);
            usage();
        }
        if((fpout = fopen(argv[3],"w"))==NULL) {
            fprintf(stdout,"ERROR: Cannot open file '%s'\n",argv[3]);
            fflush(stdout);
            usage();
        }
    }
    if((fp1 = fopen(argv[1],"r"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open file '%s'\n",argv[1]);
        fflush(stdout);
        usage();
    }
    if((fp2 = fopen(argv[2],"r"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open file '%s'\n",argv[2]);
        fflush(stdout);
        usage();
    }
    if(sscanf(argv[4],"%d",&column)!=1) {
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
    if(argc==7 && sscanf(argv[6],"%d",&skip)!=1) {
        fprintf(stdout,"ERROR: Cannot read skip value.\n");
        fflush(stdout);
        usage();
    }
    if(skip<0) {
        skip = -skip;
        keep = 0;
    }
    if(*argv[5]++!='-') {
        fprintf(stdout,"ERROR: Unknown parameter %s.\n",--argv[5]);
        fflush(stdout);
        usage();
    }
    switch(*argv[5]) {
    case('r'): replace = 1; break;
    case('i'):      break;
    default:
        fprintf(stdout,"ERROR: Unknown flag -%c\n",*argv[5]);
        fflush(stdout);
        usage();
    }
    outlinecnt = 0;
    if(skip > 0) {
        if(keep) {
            if((line = (char **)malloc(skip * sizeof(char *)))==NULL) {
                fprintf(stdout,"ERROR: Out of memory\n");
                fflush(stdout);
            }
        }
        for(n=0;n<skip;n++) {
            if(fgets(temp,400,fp2)!=temp) {
                fprintf(stdout,"ERROR: Failed to get skippable line %d in file '%s'\n",n+1,argv[1]);
                fflush(stdout);
                exit(1);
            }
            if(keep) {
                if((line[outlinecnt] = (char *)malloc((strlen(temp)+2) * sizeof(char)))==NULL) {
                    fprintf(stdout,"ERROR: Out of memory\n");
                    fflush(stdout);
                }
                line[outlinecnt][0] = ENDOFSTR;
                strcat(line[outlinecnt],temp);
                outlinecnt++;
            }
        }
    }
    while(fgets(temp,400,fp2)!=NULL) {
        /*RWD*/ p = temp;
        /*RWD*/ while(isspace(*p)) p++;
        /*and*/ if(*p=='\n' || *p == ENDOFSTR){
            if((line = (char **)realloc((char *)line,(outlinecnt+1) * sizeof(char *)))==NULL) {
                fprintf(stdout,"ERROR: Out of memory\n");
                fflush(stdout);
            }
            if((line[outlinecnt] = (char *)malloc(2 * sizeof(char)))==NULL) {
                fprintf(stdout,"ERROR: Out of memory\n");
                fflush(stdout);
            }
            line[outlinecnt][0] = ENDOFSTR;
            outlinecnt++;
            continue;
        }
        if((ignore_s_and_e && (!strcmp(temp,"e\n") || !strcmp(temp,"e") || !strcmp(temp,"s\n") || !strncmp(temp,";",1)))
           || (ignore_c && !strncmp(temp,";",1))) {
            if(line==(char **)0) {
                if((line = (char **)malloc((outlinecnt+1) * sizeof(char *)))==NULL) {
                    fprintf(stdout,"ERROR: Out of memory\n");
                    fflush(stdout);
                }
            } else {
                if((line = (char **)realloc((char *)line,(outlinecnt+1) * sizeof(char *)))==NULL) {
                    fprintf(stdout,"ERROR: Out of memory\n");
                    fflush(stdout);
                }
            }
            if((line[outlinecnt] = (char *)malloc((strlen(temp)+2) * sizeof(char)))==NULL) {
                fprintf(stdout,"ERROR: Out of memory\n");
                fflush(stdout);
            }
            line[outlinecnt][0] = ENDOFSTR;
            strcat(line[outlinecnt],temp);
            outlinecnt++;
            continue;
        }
        cnt++;
        end_col = 0;
        for(n=0;n<column;n++) {
            if(*p==ENDOFSTR) {
                fprintf(stdout,"ERROR: Not enough columns in line %d in origfile.\n",cnt);
                fflush(stdout);
                exit(1);
            }
            p = skip_column(p,cnt,argv[1]);
        }
        if(*p==ENDOFSTR) {
            if(replace) {
                fprintf(stdout,"ERROR: Not enough columns in line %d in origfile.\n",cnt);
                fflush(stdout);
                exit(1);
            } else {
                p--;    /* HANG ONTO NEWLINE */
                if(*p != NEWLINE)
                    p++;
                end_col = 1;
            }
        }
        c = *p;
        *p = ENDOFSTR;
        if(line==(char **)0) {
            if((line = (char **)malloc((outlinecnt+1) * sizeof(char *)))==NULL) {
                fprintf(stdout,"ERROR: Out of memory\n");
                fflush(stdout);
            }
        } else {
            if((line = (char **)realloc((char *)line,(outlinecnt+1) * sizeof(char *)))==NULL) {
                fprintf(stdout,"ERROR: Out of memory\n");
                fflush(stdout);
            }
        }
        if((line[outlinecnt] = (char *)malloc((strlen(temp)+2+NEW_COL_SPACE) * sizeof(char)))==NULL) {
            fprintf(stdout,"ERROR: Out of memory\n");
            fflush(stdout);
        }
        line[outlinecnt][0] = ENDOFSTR;
        strcat(line[outlinecnt],temp);
        if(end_col)
            strcat(line[outlinecnt],"\t");
        *p = c;
        if(replace) {
            p = skip_column(p,cnt,argv[1]);
            if(*p==ENDOFSTR) {
                p--;            /* KEEP NEWLINE !!! */
                if(*p != NEWLINE)
                    p++;
                end_col = 1;
            }
        }
        strcpy(temp2,p);
        if(fgets(temp,400,fp1)==NULL) {
            fprintf(stdout,"ERROR: No more vals in file %s at line %d\n",argv[1],cnt);
            fflush(stdout);
            break;
        }
        p = temp + strlen(temp);
        p--;    /* DELETE NEWLINE */
        if(*p != NEWLINE)
            p++;
        if(!end_col)
            *p++ = TAB;
        *p = ENDOFSTR;
        strcat(temp,temp2);
        newlen = strlen(line[outlinecnt]) + strlen(temp) + 2;
        if((line[outlinecnt] = (char *)realloc((char *)line[outlinecnt],newlen * sizeof(char)))==NULL) {
            fprintf(stdout,"ERROR: Memory problem.\n");
            fflush(stdout);
        }
        strcat(line[outlinecnt],temp);
        outlinecnt++;
    }
    if(sloom) {
        if((fpout = fopen(argv[3],"w"))==NULL) {
            fprintf(stdout,"ERROR: Cannot reopen file '%s'\n",argv[3]);
            fflush(stdout);
            usage();
        }
    }
    for(n=0;n<outlinecnt;n++) {
        if(!sloom && !sloombatch) {
            strcat(line[n],"\n");
            fputs(line[n],fpout);
        } else
            fprintf(stdout,"INFO: %s\n",line[n]);
    }
    fflush(stdout);
    fclose(fp2);
    fclose(fp1);
    return 0;
}

/******************************** USAGE *****************************/

void usage(void)
{
    if(!sloom) {
        logo();
        fprintf(stderr,"USAGE : putcol columnfile intofile outfile colno -r|-i [[-]skiplines] [-e]\n\n");
        fprintf(stderr,"COLUMNFILE  File with NEW column.\n");
        fprintf(stderr,"INTOFILE    File INTO which the new column goes.\n");
        fprintf(stderr,"OUTFILE     Resultant file.\n");
        fprintf(stderr,"COLNO       POSITION of the column to be added.\n");
        fprintf(stderr,"-r          REPLACE original column.\n");
        fprintf(stderr,"-i          INSERT new column amongst original columns.\n");
        fprintf(stderr,"SKIPLINES   SKIP OVER skiplines before inserting column.\n");
        fprintf(stderr,"-SKIPLINES  THROW AWAY skiplines: then insert column.\n");
        fprintf(stderr,"-e          Ignore lines which contain only 'e' or 's'.\n");
        fprintf(stderr,"            and comment lines starting with ';'\n");
        fprintf(stderr,"            (skipped lines should still be counted).\n");
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
        if(*p==ENDOFSTR)  return(p);
    }
    while(isspace(*p)) {
        p++;
        if(*p==ENDOFSTR)  return(p);
    }
    return(p);
}

/******************************** LOGO() **********************************/

void logo(void)
{   printf("\t    ***************************************************\n");
    printf("\t    *           COMPOSERS DESKTOP PROJECT             *\n");
    printf("\t                    %s $Revision: 1.3 $       \n",PROG);
    printf("\t    *                                                 *\n");
    printf("\t    *    Insert a column of figures, in a textfile,   *\n");
    printf("\t    *              into an existing file.             *\n");
    printf("\t    *                                                 *\n");
    printf("\t    *                by TREVOR WISHART                *\n");
    printf("\t    ***************************************************\n\n");
}
