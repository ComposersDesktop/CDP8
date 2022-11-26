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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sfsys.h>
#include <math.h>
#include <osbind.h>

char errstr[2400];

#define MONO        (1)
#define STEREO      (2)
#define ENDOFSTR    ('\0')

#define LEAVESPACE  (10*1024)

void splice_multiline_string(char *str,char *prefix);
const char* cdp_version = "7.1.0";
/*RWD Jan 2014 updated getdrivefreespace(), corrected INFO output format for unsigned long */
/************************************ CHECK_AVAILABLE_DISKSPACE ********************************/

int main(int argc,char *argv[])
{
    double srate, secs, mins, hrs;
    unsigned int outsamps = 0, orig_outsamps;
    int m, k, kk, spacecnt;
    char temp[800];
    unsigned int freespace = getdrivefreespace(/*"temp"*/".") - LEAVESPACE;
    unsigned int start = hz1000();

    if((argc==2) && strcmp(argv[1],"--version")==0) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }

    //start = hz1000(); 
    if(argc!=2)  {
        fprintf(stdout,"ERROR: Cannot run this process.\n");
        fflush(stdout);
//TW UPDATE
        while(!(hz1000() - start))
            ;
        return 1;
    }
    if(sscanf(argv[1],"%lf",&srate)!=1) {
        fprintf(stdout,"ERROR: Cannot read sample rate.\n");
        fflush(stdout);
//TW UPDATE
        while(!(hz1000() - start))
            ;
        return 1;
    }

    sprintf(errstr,"AVAILABLE DISK SPACE\n");
    sprintf(temp,"%u",freespace);
    strcat(errstr,temp);
    spacecnt = 13 - strlen(temp);
    for(k=0;k<spacecnt;k++)
        strcat(errstr," ");
    strcat(errstr,"bytes\n");
    splice_multiline_string(errstr,"INFO:");
    fflush(stdout);
    kk = 0;
    while (kk < 4) {
        errstr[0] = ENDOFSTR;
        switch(kk) {
        case(0): outsamps = freespace/2;    break;
        case(1): outsamps = freespace/3;    break;
        case(2): outsamps = freespace/4;    break;
        case(3): outsamps = freespace/sizeof(float);    break;
        }
        sprintf(temp,"%d",outsamps);
        strcat(errstr,temp);
        spacecnt = 13 - strlen(temp);
        for(k=0;k<spacecnt;k++)
            strcat(errstr," ");
        switch(kk) {
        case(0): strcat(errstr,"16-bit samples\n"); break;
        case(1): strcat(errstr,"24-bit samples\n"); break;
        case(2): strcat(errstr,"32-bit samples\n"); break;
        case(3): strcat(errstr,"float  samples\n"); break;
        }
        orig_outsamps = outsamps;
        for(m=MONO;m<=STEREO;m++) {
            switch(m) {
            case(MONO):
                outsamps = orig_outsamps;
                sprintf(temp,"IN MONO    : ");
                strcat(errstr,temp);
                break;
            case(STEREO):
                outsamps /= 2;
                sprintf(temp,"IN STEREO : ");
                strcat(errstr,temp);
                break;
            }
            secs  = (double)outsamps/srate;
            mins  = floor(secs/60.0);
            secs -= mins * 60.0;
            hrs   = floor(mins/60.0);
            mins -= hrs * 60.0;
            if(hrs > 0.0) {
                sprintf(temp,"%.0lf",hrs);
                strcat(errstr,temp);
                spacecnt = 3 - strlen(temp);
                for(k=0;k<spacecnt;k++)
                    strcat(errstr," ");
                strcat(errstr,"hours ");
            } else {
                for(k=0;k<4 + (int)strlen("hours");k++)
                    strcat(errstr," ");
            }
            if(mins > 0.0) {
                sprintf(temp,"%.0lf",mins);
                strcat(errstr,temp);
                spacecnt = 3 - strlen(temp);
                for(k=0;k<spacecnt;k++)
                    strcat(errstr," ");
                strcat(errstr,"mins ");
            } else {
                for(k=0;k<4 + (int)strlen("mins");k++)
                    strcat(errstr," ");
            }
            sprintf(temp,"%.3lf",secs);
            strcat(errstr,temp);
            spacecnt = 7 - strlen(temp);
            for(k=0;k<spacecnt;k++)
                strcat(errstr," ");
            strcat(errstr,"secs\n");
        }
        splice_multiline_string(errstr,"INFO:");
        fflush(stdout);
        kk++;
    }
//TW UPDATE
    while(!(hz1000() - start))
        ;
    return 0;
}

/****************************** SPLICE_MULTILINE_STRING ******************************/

void splice_multiline_string(char *str,char *prefix)
{
    char *p, *q, c;
    p = str;
    q = str;
    while(*q != ENDOFSTR) {
        while(*p != '\n' && *p != ENDOFSTR)
            p++;
        c = *p;
        *p = ENDOFSTR;
        fprintf(stdout,"%s %s\n",prefix,q);
        *p = c;
        if(*p == '\n')
             p++;
        while(*p == '\n') {
            fprintf(stdout,"%s \n",prefix);
            p++;
        }
        q = p;
        p++;
    }
}
