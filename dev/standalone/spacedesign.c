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
    spacedesign stom    genericoutfilename convergestarttime convergeendtime convergeposition 
    spacedesign mtos    genericoutfilename divergestarttime  divergeendtime  divergeposition 
    spacedesign rotate  genericoutfilename starttime         endtime         startpos    
                        endpos  edgetimes   width   clokwise? lingerfactor loudnessdepth loudnesslead  attenuation   filterfrq
                        entry   entry       entry   radiobuts entry        entry         entry         entry
                        -1 to 1 increasing  time/               >0 && <1     >= 1       0 - 1           >0 to 10 ?? 10 - 10000
                                times       width                           dflt 1

                edgetimes are times at which rotation reaches extremal left or right point
                MUST be > starttime
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

//TW UPDATE
//#include <ctype.h>

/* RWD Jan 2014 corrected spelling mistoke in usage */

#define ENDOFSTR ('\0')
#ifndef PI
#define PI  (3.141592654)
#endif

#define LINGRATIO (0.222222222) /* 2 9ths = proprtion of space not in linger */
#define FLTERR (0.000002)

static void strip_file_extension(char *filename);
static int stom(char *filename1,char *filename2,float starttime,float endttime,float endpos);
static int mtos(char *filename1,char *filename2,float starttime,float endtime,float startpos);
static void usage(int mode);
static int read_rotation_data(FILE *fpr,float **rotdat);
static int read_width_data(FILE *fpr,float **widdat);
static int  strgetfloat(char **str,double *val);
static int rotate(char *fnam1,char *fnam2,char *fnam3,float starttime,float endtime,float startpos,float endpos,
            int clokwise,int rotcnt,float *rotdat,int widcnt,float *widdat,float linger,float depth,
            float lead,float atten,int isfilt,float warp);
static int read_value_from_brktable(float *val,float *brk, int brksize, float thistime,int init);
static int flteq(double f1,double f2);
static int isnumeric(char *str);
const char* cdp_version = "7.1.0";

int main(int argc, char *argv[])
{
    int mode = 0, exit_status = 1, rotcnt = 0, widcnt=0, clokwise, isfilt;
    float starttime, endtime, endpos, startpos=0.0f, *rotdat = NULL,  *widdat = NULL, dummy;
    float linger, depth, lead, atten, warp;
    FILE *fp, *fpr, *fpw;
    char filename[400], filename1[400], filename2[400], filename3[400] ,errword[10], errword2[12], errword3[8];
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if(argc < 2) {
        usage(0);
        return 0;
    }
    if(!strcmp(argv[1],"stom")) {
        mode = 1;
    } else if (!strcmp(argv[1],"mtos")) {
        mode = 2;
    } else if (!strcmp(argv[1],"rotate")) {
        mode = 3;
    }
    switch(mode) {
    case(1):
    case(2):
        if(argc != 6) {
            usage(mode);
            return 0;
        }
        break;
    case(3):
        if(argc != 16) {
            usage(mode);
            return 0;
        }
        break;
    default:
        fprintf(stdout,"ERROR: Unknown mode '%s': spacedesign()\n",argv[1]);
        fflush(stdout);
        return 0;
    }
    switch(mode) {
    case(1):    strcpy(errword,"converge"); strcpy(errword2,"end");     strcpy(errword3,"stom");    break;
    case(2):    strcpy(errword,"diverge");  strcpy(errword2,"start");   strcpy(errword3,"mtos");    break;
    case(3):    strcpy(errword,"rotate");   strcpy(errword2,"start");   strcpy(errword3,"rotate");  break;
    }
    switch(mode) {
    case(1):
    case(2):
    case(3):
        if(sscanf(argv[2],"%s",filename)!=1) {
            fprintf(stdout,"ERROR: Failed to read outfilename: spacedesign\n");
            fflush(stdout);
            return 0;
        }
        if(sscanf(argv[3],"%f",&starttime)!=1 || starttime < 0.0) {
            fprintf(stdout,"ERROR: bad %s starttime parameter: spacedesign %s\n",errword,errword3);
            fflush(stdout);
            return 0;
        }       
        if(sscanf(argv[4],"%f",&endtime)!=1 || endtime <= starttime) {
            fprintf(stdout,"ERROR: bad %s endtime parameter: spacedesign %s\n",errword,errword3);
            fflush(stdout);
            return 0;
        }       
        if(sscanf(argv[5],"%f",&endpos)!=1 || (endpos < -1.0 || endpos > 1.0)) {
            fprintf(stdout,"ERROR: bad %s %s position parameter: spacedesign %s\n",errword,errword2,errword3);
            fflush(stdout);
            return 0;
        }
        if(mode==2 || mode==3)
            startpos = endpos;
        break;
    }
    switch(mode) {
    case(1):
    case(2):
    case(3):
        strip_file_extension(filename);
        strcpy(filename1,filename);
        strcat(filename1,"1");
        strcat(filename1,".txt");
        strcpy(filename2,filename);
        strcat(filename2,"2");
        strcat(filename2,".txt");
        strcpy(filename3,filename);
        strcat(filename3,"3");
        strcat(filename3,".txt");
        if((fp = fopen(filename1,"r"))!=NULL) {
            fprintf(stdout,"ERROR: file %s already exists: spacedesign()\n",filename1);
            fclose(fp);
            fflush(stdout);
            return 0;
        }
        if((fp = fopen(filename2,"r"))!=NULL) {
            fprintf(stdout,"ERROR: file %s already exists: spacedesign()\n",filename2);
            fclose(fp);
            fflush(stdout);
            return 0;
        }
        break;
    }
    switch(mode) {
    case(3):
        if(sscanf(argv[6],"%f",&endpos)!=1 || (endpos < -1.0 || endpos > 1.0)) {
            fprintf(stdout,"ERROR: bad end position parameter: spacedesign %s\n",errword3);
            fflush(stdout);
            return 0;
        }
        if((fpr = fopen(argv[7],"r"))==NULL) {
            fprintf(stdout,"ERROR: Bad edgetimes data: spacedesign %s\n",errword3);
            fflush(stdout);
            return 0;
        }
        if((rotcnt = read_rotation_data(fpr,&rotdat))<=0)
            return 0;
        if(*rotdat <= starttime) {
            fprintf(stdout,"ERROR: First edgetime <= rotation starttime: spacedesign %s\n",errword3);
            fflush(stdout);
            return 0;
        }
        if(isnumeric(argv[8])) {
            sscanf(argv[8],"%f",&dummy);
            if (dummy < 0.0  || dummy > 1.0) {
                fprintf(stdout,"ERROR: Bad width value (range 0 - 1): spacedesign %s\n",errword3);
                fflush(stdout);
                return 0;
            }
            widcnt = 1;
            if((widdat = (float *)malloc(4 * sizeof(float)))==NULL) {
                fprintf(stdout,"ERROR: Cannot allocate memory for width data: spacedesign %s\n",errword3);
                fflush(stdout);
                return 0;
            }
            *widdat = 0.0f;
            *(widdat+1) = dummy;
            *(widdat+2) = 10000.0f;
            *(widdat+3) = dummy;
        } else if((fpw = fopen(argv[8],"r"))==NULL) {
            fprintf(stdout,"ERROR: Bad width data: spacedesign %s\n",errword3);
            fflush(stdout);
            return 0;
        } else if((widcnt = read_width_data(fpw,&widdat))<=0)
            return 0;
        if(sscanf(argv[9],"%d",&clokwise)!=1 || (clokwise != 1 && clokwise != 0)) {
            fprintf(stdout,"ERROR: bad direction value: 1 for clockwise, 0 for anticlockwise\n");
            fflush(stdout);
            return 0;
        }
        if(sscanf(argv[10],"%f",&linger)!=1 || linger >= 1.0 || linger <= 0.0) {
            fprintf(stdout,"ERROR: bad linger value: range >0 to <1\n");
            fflush(stdout);
            return 0;
        }
        if(sscanf(argv[11],"%f",&depth)!=1 || depth < 1.0) {
            fprintf(stdout,"ERROR: bad depth value:  must be >= 1\n");
            fflush(stdout);
            return 0;
        }
        if(sscanf(argv[12],"%f",&lead)!=1 || lead < 0.0 || lead > 1.0) {
            fprintf(stdout,"ERROR: bad lead value:  range 0-1\n");
            fflush(stdout);
            return 0;
        }
        if(sscanf(argv[13],"%f",&atten)!=1 || atten <= 0.0 || atten >= 10) {
            fprintf(stdout,"ERROR: bad attenuation value:  range >0-10\n");
            fflush(stdout);
            return 0;
        }
        if(sscanf(argv[14],"%d",&isfilt)!=1 || (isfilt != 0 && isfilt != 1)) {
            fprintf(stdout,"ERROR: bad filter flag (%d)\n",isfilt);
            fflush(stdout);
            return 0;
        }
        if(isfilt) {
            if((fp = fopen(filename3,"r"))!=NULL) {
                fprintf(stdout,"ERROR: file %s already exists: spacedesign()\n",filename3);
                fclose(fp);
                fflush(stdout);
                return 0;
            }
        }
        if((sscanf(argv[15],"%f",&warp)!=1) || warp < 1 || warp > 20) {
            fprintf(stdout,"ERROR: bad warp value (%f)\n",warp);
            fflush(stdout);
            return 0;
        }
        break;
    }
    switch(mode) {
    case(1): 
        exit_status = stom(filename1,filename2,starttime,endtime,endpos);
        break;
    case(2): 
        exit_status = mtos(filename1,filename2,starttime,endtime,startpos);
        break;
    case(3):
        exit_status = 
        rotate(filename1,filename2,filename3,starttime,endtime,startpos,endpos,clokwise,rotcnt,rotdat,
            widcnt,widdat,linger,depth,lead,atten,isfilt,warp);
        break;
    }
    return exit_status;
}

int stom(char *filename1,char *filename2,float starttime,float endtime,float endpos) 
{
    FILE *fp1, *fp2;
    char valstr[400];
    int OK = 1, n;
    double gap, step, q;
    double leftmove, rightmove, cospos, leftpos, rightpos, thistime;
    if((fp1 = fopen(filename1,"w"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open file %s: spacedesign()\n",filename1);
        fflush(stdout);
        return -1;
    }
    if((fp2 = fopen(filename2,"w"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open file %s: spacedesign()\n",filename2);
        fclose(fp1);
        if(remove(filename1)<0)
            fprintf(stdout, "ERROR: Can't remove file %s.\n",filename1);
        fflush(stdout);
        return -1;
    }
    while(OK) {
        strcpy(valstr,"0.0 -1.0\n");
        if(fputs(valstr,fp1)<0) {
            OK = 0;
            break;
        }
        strcpy(valstr,"0.0 1.0\n");
        if(fputs(valstr,fp2)<0) {
            OK = 0;
            break;
        }
        if(starttime > 0.0) {
            sprintf(valstr,"%f",starttime);
            strcat(valstr," -1.0\n");
            if(fputs(valstr,fp1)<0) {
                OK = 0;
                break;
            }
            sprintf(valstr,"%f",starttime);
            strcat(valstr," 1.0\n");
            if(fputs(valstr,fp2)<0) {
                OK = 0;
                break;
            }
        }
        gap = endtime - starttime;
        leftmove  = endpos + 1.0;
        rightmove = endpos - 1.0;
        step = gap/8.0;
        thistime = starttime;
        for(n=1; n<8;n++) {
            thistime += step;
            q = (double)n/8.0;
            cospos = cos(q * PI);
            cospos = (1 - cospos)/2.0;
// weight the cosinus with the straight line, to make curve less abrupt
            cospos = (q + (2.0 * cospos)) / 3.0;
            leftpos  = (cospos * leftmove)  - 1.0;
            rightpos = (cospos * rightmove) + 1.0;
            sprintf(valstr,"%f %f\n",thistime,(float)leftpos);
            if(fputs(valstr,fp1)<0) {
                OK = 0;
                break;
            }
            sprintf(valstr,"%f %f\n",thistime,(float)rightpos);
            if(fputs(valstr,fp2)<0) {
                OK = 0;
                break;
            }
        }
        if(!OK)
            break;
        sprintf(valstr,"%f %f\n",endtime,endpos);
        if(fputs(valstr,fp1)<0) {
            OK = 0;
            break;
        }
        if(fputs(valstr,fp2)<0) {
            OK = 0;
            break;
        }
        sprintf(valstr,"10000 %f\n",endpos);
        if(fputs(valstr,fp1)<0) {
            OK = 0;
            break;
        }
        if(fputs(valstr,fp2)<0)
            OK = 0;
        break;
    }
    fclose(fp1);
    fclose(fp2);
    if(!OK) {
        if(remove(filename1)<0)
            fprintf(stdout, "ERROR: Can't remove file %s.\n",filename1);
        if(remove(filename2)<0)
            fprintf(stdout, "ERROR: Can't remove file %s.\n",filename2);
        fflush(stdout);
        return 0;
    }
    return 1;
}

int mtos(char *filename1,char *filename2,float starttime,float endtime,float startpos) 
{
    FILE *fp1, *fp2;
    char valstr[400];
    int OK = 1, n;
    double gap, step, q;
    double leftmove, rightmove, cospos, leftpos, rightpos, thistime;
    if((fp1 = fopen(filename1,"w"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open file %s: spacedesign()\n",filename1);
        fflush(stdout);
        return -1;
    }
    if((fp2 = fopen(filename2,"w"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open file %s: spacedesign()\n",filename2);
        fclose(fp1);
        if(remove(filename1)<0)
            fprintf(stdout, "ERROR: Can't remove file %s.\n",filename1);
        fflush(stdout);
        return -1;
    }
    while(OK) {
        sprintf(valstr,"0.0 %f\n",startpos);
        if(fputs(valstr,fp1)<0) {
            OK = 0;
            break;
        }
        if(fputs(valstr,fp2)<0) {
            OK = 0;
            break;
        }
        if(starttime > 0.0) {
            sprintf(valstr,"%f %f\n",starttime,startpos);
            if(fputs(valstr,fp1)<0) {
                OK = 0;
                break;
            }
            if(fputs(valstr,fp2)<0) {
                OK = 0;
                break;
            }
        }
        gap = endtime - starttime;
        leftmove  = -(1.0 + startpos);
        rightmove = 1.0 - startpos;
        step = gap/8.0;
        thistime = starttime;
        for(n=1; n<8;n++) {
            thistime += step;
            q = (double)n/8.0;
            cospos = cos(q * PI);
            cospos = (1 - cospos)/2.0;
// weight the cosinus with the straight line, to make curve less abrupt
            cospos = (q + (2.0 * cospos)) / 3.0;
            leftpos  = (cospos * leftmove)  + startpos;
            rightpos = (cospos * rightmove) + startpos;
            sprintf(valstr,"%f %f\n",thistime,(float)leftpos);
            if(fputs(valstr,fp1)<0) {
                OK = 0;
                break;
            }
            sprintf(valstr,"%f %f\n",thistime,(float)rightpos);
            if(fputs(valstr,fp2)<0) {
                OK = 0;
                break;
            }
        }
        sprintf(valstr,"%f -1.0\n",endtime);
        if(fputs(valstr,fp1)<0) {
            OK = 0;
            break;
        }
        sprintf(valstr,"%f 1.0\n",endtime);
        if(fputs(valstr,fp2)<0) {
            OK = 0;
            break;
        }
        sprintf(valstr,"10000 -1\n");
        if(fputs(valstr,fp1)<0) {
            OK = 0;
            break;
        }
        sprintf(valstr,"10000 1\n");
        if(fputs(valstr,fp2)<0)
            OK = 0;
        break;
    }
    fclose(fp1);
    fclose(fp2);
    if(!OK) {
        if(remove(filename1)<0)
            fprintf(stdout, "ERROR: Can't remove file %s.\n",filename1);
        if(remove(filename2)<0)
            fprintf(stdout, "ERROR: Can't remove file %s.\n",filename2);
        fflush(stdout);
        return 0;
    }
    return 1;
}

int rotate(char *fnam1,char *fnam2,char *fnam3,float starttime,float endtime,float startpos,float endpos,int clokwise,
           int rotcnt,float *rotdat,int widcnt,float *widdat,float linger,float depth,float lead,float atten,int isfilt,float warp)
{
    FILE *fp1, *fp2 ,*fp3=NULL;
    char valstr[200];
    int init = 0, n, OK = 1, right=0,  doit;
    float width, *p, edgetime, last_edgetime, val[8];
    float depdiff = (float)((depth - 1.0)/(depth + 1.0));
    float last_edgepos, edgepos, postlingpos, prelingpos, postlingtime, prelingtime, leadval, midtime;
    double lingwidth, timestep, lingstep, leadstep, leadtime, frontwarp=0.0, rearwarp=0.0, step;
    double newtimestep, time_extend, timeratio, thisgap, newlen;

    int rightwards, iswarp, k;
    if(flteq(warp,1.0))
        iswarp = 0;
    else {
        iswarp = 1;
        frontwarp = 2.0/(double)(warp + 1.0);
        rearwarp  = (2.0 * warp)/(double)(warp + 1.0);
    }
    if((fp1 = fopen(fnam1,"w"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open file %s to write space data: spacedesign()\n",fnam1);
        fflush(stdout);
        return 0;
    }
    if((fp2 = fopen(fnam2,"w"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open file %s to write loudness data: spacedesign()\n",fnam2);
        fflush(stdout);
        fclose(fp1);
        if(remove(fnam1)<0)
            fprintf(stdout, "ERROR: Can't remove file %s.\n",fnam1);
        fflush(stdout);
        return 0;
    }
    if(isfilt) {
        if((fp3 = fopen(fnam3,"w"))==NULL) {
            fprintf(stdout,"ERROR: Cannot open file %s to write loudness data: spacedesign()\n",fnam3);
            fflush(stdout);
            fclose(fp1);
            if(remove(fnam1)<0)
                fprintf(stdout, "ERROR: Can't remove file %s.\n",fnam1);
            fflush(stdout);
            fclose(fp2);
            if(remove(fnam2)<0)
                fprintf(stdout, "ERROR: Can't remove file %s.\n",fnam2);
            fflush(stdout);
            return 0;
        }
    }
    while(OK) {
        sprintf(valstr,"0.0 %f\n",atten);
        if(fputs(valstr,fp2)<0) {
            OK = 0;
            break;
        }
        if(isfilt) {
            sprintf(valstr,"0.0 1.0\n");
            if(fputs(valstr,fp3)<0) {
                OK = 0;
                break;
            }
        }
        sprintf(valstr,"0.0 %f\n",startpos);
        val[0] = 0.0f;
        val[1] = startpos;
        if(starttime > 0.0) {
            if(fputs(valstr,fp1)<0) {
                OK = 0;
                break;
            }
            val[0] = starttime;
            val[1] = startpos;
            sprintf(valstr,"%f %f\n",starttime,atten);
            if(fputs(valstr,fp2)<0) {
                OK = 0;
                break;
            }
            if(isfilt) {
                sprintf(valstr,"%f 1.0\n",starttime);
                if(fputs(valstr,fp3)<0) {
                    OK = 0;
                    break;
                }
            }
        }
        n = 0;
        p = rotdat;
        edgetime = *p;
        last_edgetime = starttime;
        last_edgepos = startpos;
        while(edgetime < endtime) {
            if(edgetime > starttime) {
                init = read_value_from_brktable(&width,widdat,widcnt,edgetime,init);
                edgepos = width;
                if(n==0) {
                    if(clokwise) {
                        edgepos = -edgepos;
                        right = 0;
                    } else
                        right = 1;
                } else if(right) {
                    edgepos = -edgepos;
                    right = 0;
                } else
                    right = 1;
                width = edgepos - last_edgepos;                     /* find width of pan from last edgepos */
                lingwidth = (width * LINGRATIO)/2.0;                /* find proprtion of width taken up by each linger */
                postlingpos = (float)(last_edgepos + lingwidth);    /* find position at end of linger after last edgepos */
                prelingpos  = (float)(edgepos      - lingwidth);    /* find position at start of linger before next edgepos */
                timestep = edgetime - last_edgetime;                /* find timestep from edge to edge */
                lingstep = (timestep * linger)/2.0;                 /* find proprtion of time used in each linger */
                postlingtime = (float)(last_edgetime + lingstep);   /* find time of end of linger after last edgepos */
                prelingtime  = (float)(edgetime      - lingstep);   /* find time of start of linger before next edgepos */
                midtime = (float)((edgetime + last_edgetime)/2.0);
                sprintf(valstr,"%f %f\n",postlingtime,postlingpos);
                val[2] = postlingtime;
                val[3] = postlingpos;
                val[4] = prelingtime;
                val[5] = prelingpos;
                val[6] = edgetime;
                val[7] = edgepos;
                leadstep = (timestep/2.0) * lead;                   /* find time to maxmin val within 1st half of timestep */
                leadtime = last_edgetime + leadstep;                /* find time of maxmin */
                rightwards = 0;                     
                if(edgepos > last_edgepos)                          /* is soundmoving left or right */
                    rightwards = 1;
                if(flteq(last_edgetime,starttime)) {
                    leadval = atten;
                    sprintf(valstr,"%f %f\n",midtime,leadval);
                } else {
                    if(rightwards == clokwise) {                            /* assign min or max according to clokwise/anti */
                        leadval = (float)(atten * (1.0 - depdiff));
                        sprintf(valstr,"%f %f\n",midtime,leadval);
                    } else {
                        leadval = (float)(atten * (1.0 + depdiff));
                        sprintf(valstr,"%f %f\n",leadtime,leadval);
                    }
                }
                if(fputs(valstr,fp2)<0) {
                    OK = 0;
                    break;
                }
                if(rightwards == clokwise) {
                    if (iswarp) {
                        k = 2;
                        if(flteq(last_edgetime,starttime)) {    /* adjust initial set */
                            newtimestep = timestep * rearwarp;
                            time_extend = (newtimestep - timestep)/2.0;
                            newtimestep = timestep + time_extend;
                            timeratio = newtimestep/timestep;
                            while(k < 6) {
                                thisgap = val[k] - last_edgetime;
                                thisgap *= timeratio;
                                val[k] = (float)(last_edgetime + thisgap);
                                k += 2;
                            }
                        } else {
                            while(k < 6) {                  /* for rear motion, adjust two linger times */
                                step = midtime - val[k];
                                step *= rearwarp;
                                val[k] = (float)(midtime - step);
                                k += 2;
                            }
                        }
                    }
                    for(k=2;k<6;k+=2) {
                        sprintf(valstr,"%f %f\n",val[k],val[k+1]);
                        if(fputs(valstr,fp1)<0) {
                            OK = 0;
                            break;
                        }
                    }
                } else {
                    if (iswarp) {
                        k = 0;
                        while(k < 8) {                  /* for front motion, adjust four linger times */
                            step = midtime - val[k];
                            step *= frontwarp;
                            val[k] = (float)(midtime - step);
                            k += 2;
                        }
                        if(flteq(last_edgetime,starttime)) {    /* adjust initial set */
                            step = val[0] - last_edgetime;
                            for(k=0;k<8;k+=2)
                                val[k] = (float)(val[k] - step);
                            newlen = val[6] - val[0];
                            timeratio = (newlen + step)/newlen;
                            for(k=0;k<8;k+=2) {
                                thisgap = val[k] - last_edgetime;
                                thisgap *= timeratio;
                                val[k] = (float)(last_edgetime + thisgap);
                            }
                        }
                    }
                    for(k=0;k<8;k+=2) {
                        sprintf(valstr,"%f %f\n",val[k],val[k+1]);
                        if(fputs(valstr,fp1)<0) {
                            OK = 0;
                            break;
                        }
                    }
                }
                if(OK == 0)
                    break;
                if(isfilt) {
                    doit = 0;
                    if(flteq(last_edgetime,starttime)) {        /* at startup, no level changes */
                        if(!flteq(leadtime,last_edgetime)) {
                            sprintf(valstr,"%f 1.0\n",leadtime);
                            doit = 1;
                        }
                    } else if(rightwards == clokwise) {         /* if different , filter in middle */
                        sprintf(valstr,"%f 0.0\n",midtime);
                        doit = 1;
                    } else {
                        sprintf(valstr,"%f 1.0\n",leadtime);    /* if close, filter off at max-level point */
                        doit = 1;
                    }
                    if(doit) {
                        if(fputs(valstr,fp3)<0) {
                            OK = 0;
                            break;
                        }
                    }
                }
                last_edgetime = edgetime;
                last_edgepos  = edgepos;
            }
            if(++n >= rotcnt)
                break;
            p++;
            edgetime = *p;
            val[0] = last_edgetime;
            val[1] = last_edgepos;
        }
        if(!OK)
            break;
        if((last_edgetime < endtime) && !flteq((double)endtime,(double)last_edgetime)) {
            edgetime = endtime;
            edgepos  = endpos;                                  /* convert with 0-1 --> position cal -1 to 1 */
            rightwards = 0;                     
            if(edgepos > last_edgepos)
                rightwards = 1;
            if(rightwards == clokwise) {
                last_edgetime = val[6];
            } else {
                midtime = (float)((edgetime + last_edgetime)/2.0);
                step = midtime - last_edgetime;
                if(iswarp)
                    step *= frontwarp;
                last_edgetime = (float)(last_edgetime + step);
                if(last_edgetime >= edgetime) {
                    OK = 0;                 
                } else {
                    val[0] = last_edgetime;
                    val[1] = last_edgepos;
                }
            }
            if(OK) {
                width = edgepos - last_edgepos;                     /* find width of pan from last TRUE edgepos */
                lingwidth = (width * LINGRATIO)/2.0;                /* find proprtion of width taken up by each linger */
                postlingpos = (float)(last_edgepos + lingwidth);    /* find position at end of linger after last edgepos */
                prelingpos  = (float)(edgepos      - lingwidth);    /* find position at start of linger before next edgepos */
                timestep = edgetime - last_edgetime;                /* find timestep from edge to edge */
                lingstep = (timestep * linger)/2.0;                 /* find proprtion of time used in each linger */
                postlingtime = (float)(last_edgetime + lingstep);   /* find time of end of linger after last edgepos */
                prelingtime  = (float)(edgetime      - lingstep);   /* find time of start of linger before next edgepos */
                val[2] = postlingtime;
                val[3] = postlingpos;
                val[4] = prelingtime;
                val[5] = prelingpos;
                val[6] = edgetime;
                val[7] = edgepos;
                if(rightwards == clokwise) {
                    for(k=2;k<8;k+=2) {
                        sprintf(valstr,"%f %f\n",val[k],val[k+1]);
                        if(fputs(valstr,fp1)<0) {
                            OK = 0;
                            break;
                        }
                    }
                } else {
                    for(k=0;k<8;k+=2) {
                        sprintf(valstr,"%f %f\n",val[k],val[k+1]);
                        if(fputs(valstr,fp1)<0) {
                            OK = 0;
                            break;
                        }
                    }
                }
                if(OK == 0)
                    break;
                leadstep = (timestep/2.0) * lead;                   /* find time to maxmin val within 1st half of timestep */
                leadtime = last_edgetime + leadstep;                /* find time of maxmin */
                sprintf(valstr,"%f %f\n",leadtime,atten);
                if(fputs(valstr,fp2)<0) {
                    OK = 0;
                    break;
                }
                if(isfilt) {
                    if(!flteq(leadtime,last_edgetime)) {
                        sprintf(valstr,"%f 1.0\n",leadtime);
                        if(fputs(valstr,fp3)<0) {
                            OK = 0;
                            break;
                        }
                    }
                }
            }
        }
        sprintf(valstr,"10000 %f\n",endpos);
        if(fputs(valstr,fp1)<0) {
            OK = 0;
            break;
        }
        sprintf(valstr,"10000 %f\n",atten);
        if(fputs(valstr,fp2)<0) {
            OK = 0;
            break;
        }
        if(isfilt) {
            sprintf(valstr,"10000 1.0\n");
            if(fputs(valstr,fp3)<0) {
                OK = 0;
                break;
            }
        }
        break;
    }
    fclose(fp1);
    fclose(fp2);
    if(isfilt)
        fclose(fp3);
    if(!OK) {
        if(remove(fnam1)<0)
            fprintf(stdout, "ERROR: Can't remove file %s.\n",fnam1);
        if(remove(fnam2)<0)
            fprintf(stdout, "ERROR: Can't remove file %s.\n",fnam2);
        if(isfilt) {
            if(remove(fnam3)<0)
                fprintf(stdout, "ERROR: Can't remove file %s.\n",fnam3);
        }
        fflush(stdout);
        return 0;
    }
    return 1;
}

void strip_file_extension(char *filename)
{
    char *p = filename;
    while(*p != ENDOFSTR) {
        if(*p == '.') {
            *p = ENDOFSTR;
            return;
        }
        p++;
    }
    return;
}

int read_rotation_data(FILE *fpr,float **rotdat)
{
    char *q, temp[200];
    float *p;
    double d;
    double lasttime = -1.0;
    int arraysize = 100, n = 0;
    if((*rotdat = (float *)malloc(arraysize * sizeof(float)))==NULL) {
        fprintf(stdout,"ERROR: Insufficient memory to store rotation data\n");
        fflush(stdout);
        return -1;
    }
    p = *rotdat;
    while(fgets(temp,200,fpr)==temp) {
        q = temp;
        while(strgetfloat(&q,&d)) {
            if(d <= lasttime) {
                fprintf(stdout,"ERROR: Times not increasing (at %lf) in rotation data file\n",d);
                fflush(stdout);
                return -1;
            }
            lasttime = d;
            *p = (float)d;
            p++;
            n++;
            if(n >= arraysize) {
                arraysize += 100;
                if((*rotdat = (float *)realloc((char *)(*rotdat),arraysize * sizeof(float)))==NULL) {
                    fprintf(stdout,"ERROR: Insufficient memory to expand store of rotation data\n");
                    fflush(stdout);
                    return -1;
                }
            }
        }
    }       
    return n;
}

int read_width_data(FILE *fpr,float **widdat)
{
    char *q, temp[200];
    float *p;
    double d;
    double lasttime = -1.0;
    int istime = 1;
    int arraysize = 100, n = 0;
    if((*widdat = (float *)malloc(arraysize * sizeof(float)))==NULL) {
        fprintf(stdout,"ERROR: Insufficient memory to store rotation data\n");
        fflush(stdout);
        return -1;
    }
    p = *widdat;
    while(fgets(temp,200,fpr)==temp) {
        q = temp;
        while(strgetfloat(&q,&d)) {
            switch(istime) {
            case(1):
                if(d <= lasttime) {
                    fprintf(stdout,"ERROR: Times not increasing (at %lf) in rotation data file\n",d);
                    fflush(stdout);
                    return -1;
                }
                lasttime = d;
                break;
            case(0):
                if(d > 1 || d < 0) {
                    fprintf(stdout,"ERROR: Width data %lf out of range 0-1 in width data file\n",d);
                    fflush(stdout);
                    return -1;
                }
                break;
            }
            istime = !istime;
            *p = (float)d;
            p++;
            n++;
            if(n >= arraysize) {
                arraysize += 100;
                if((*widdat = (float *)realloc((char *)(*widdat),arraysize * sizeof(float)))==NULL) {
                    fprintf(stdout,"ERROR: Insufficient memory to expand store of rotation data\n");
                    fflush(stdout);
                    return -1;
                }
            }
        }
    }       
    if(n&1) {
        fprintf(stdout,"ERROR: Width data not correctly paired\n");
        fflush(stdout);
        return -1;
    }
    return n;
}

/**************************STRGETFLOAT **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

int  strgetfloat(char **str,double *val)
{   char *p, *q, *end;
    double numero;
    int point, valid;
    for(;;) {
        point = 0;
        p = *str;
        while(isspace(*p))
            p++;
        q = p;  
        if(!isdigit(*p) && *p != '.' && *p!='-')
            return(0);
        if(*p == '.'|| *p == '-') {
            if(*p == '-') {
                p++;
            } else {
                point++;
                    p++;
            }
        }
        for(;;) {
            if(*p == '.') {
                if(point)
                    return(0);
                else {
                    point++;
                    p++;
                    continue;
                }
            }
            if(isdigit(*p)) {
                p++;
                continue;
            } else {
                if(!isspace(*p) && *p!=ENDOFSTR)
                    return(0);
                else {
                    end = p;
                    p = q;
                    valid = 0;
                    while(p!=end) {
                        if(isdigit(*p))
                            valid++;
                        p++;
                    }
                    if(valid) {
                        if(sscanf(q,"%lf",&numero)!=1)
                            return(0);
                        *val = numero;
                       *str = end;
                        return(1);
                    }
                    return(0);
                }
            }
        }
    }
    return(0);              /* NOTREACHED */
}

/**************************** READ_VALUE_FROM_BRKTABLE *****************************/

int read_value_from_brktable(float *val,float *brk, int brksize, float thistime,int init)
{
    float *endpair, *p;
    double hival, loval, hiind, loind, thisval;
    static float *brkptr, firstval, finaltime, lastval;
    if(!init) {
        brkptr = brk;
        firstval = *(brkptr+1);
        endpair  = brkptr + ((brksize-1)*2);
        finaltime  = *endpair;
        lastval  = *(endpair+1);
    }
    p = brkptr;
    if(thistime <= *brk) {
        *val = firstval;
        return 1;
    } else if(thistime >= finaltime) {
        *val = lastval;
        return 1;
    } 
    if(thistime > *p) {
        while(*p < thistime)
            p += 2;
    } else {
        while((*p = thistime))
            p -= 2;
        p += 2;
    }
    hival  = *(p+1);
    hiind  = *p;
    loval  = *(p-1);
    loind  = *(p-2);
    thisval = (thistime - loind)/(hiind - loind);
    thisval   *= (float)(hival - loval);
    thisval   += loval;
    *val = (float)thisval;
    brkptr = p;
    return 1;
}

void usage(int mode)
{
    switch(mode) {
    case(0):
        fprintf(stdout,
        "ERROR: USAGE: spacedesign mode outfile params\n"
        "\n"
        "MODE     stom, mtos, rotate\n"
        "STOM     collapses stereo image to mono, gradually\n"
        "MTOS     spreads from mono image to stereo\n"
        "ROTATE   rotates the source\n"
        "\n"
        "Outputs are various control files for enveloping or mixing the source.\n"
        "\n"
        "This program is usually accessed through an interface on the Sound Loom.\n");
        break;
    case(1):
        fprintf(stdout,
        "ERROR: USAGE: spacedesign stom generic_outfilename starttime endtime endpos\n"
        "\n"
        "STARTTIME    time at which convergence to mono begins.\n"
        "ENDTIME      time at which convergence to mono end.\n"
        "ENDPOS       final position of mono image in stereo field.\n");
        break;
    case(2):
        fprintf(stdout,
        "ERROR: USAGE: spacedesign mtos generic_outfilename starttime endtime startpos\n"
        "\n"
        "STARTTIME    time at which divergence to stereo begins.\n"
        "ENDTIME      time at which divergence to stereo is complete.\n"
        "ENDPOS       initial position of mono image in stereo field.\n");
        break;
    case(3):
        fprintf(stdout,
        "ERROR: USAGE: spacedesign rotate generic_outfilename starttime endtime startpos endpos\n"
        "edgetimes width clokwise(0/1) linger loudnessdepth loudnesslead attenuation isfilt warp\n"
        "\n"
        "STARTTIME     time at which rotation begins.\n"
        "ENDTIME       time at which rotation ends.\n"
        "STARTPOS      position of mono image in stereo field when rotation starts.\n"
        "ENDPOS        position of mono image in stereo field when rotation ends.\n"
        "EDGETIMES     file of (increasing) times at which rotation is at (alternate) edges of space.\n"
        "WIDTH         width of rotating circle (0-1) (try 0.9).\n"
        "CLOKWISE      rotate clockwise(1) or anticlockwise(0).\n"
        "LINGER        linger at edges of space (0-1) (try 0.15).\n"
        "LOUDNESSDEPTH difference in level between front and rear of circle (>=1) (1 = no difference).\n"
        "LOUDNESSLEAD  loudest moment precedes central point of rotation by (secs).\n"
        "ATTENUATION   overall attenuation.\n"
        "ISFILT         (0 or 1) create a file to control balance of src and a filtered version of it?\n"
        "WARP          (1-20) greater warp increases proportion of time \"behind\" plane of loudspeakers.\n"); /*RWD was 'plain' */
        break;
    }
    fflush(stdout);
}

/**************************** FLTEQ *******************************/

int flteq(double f1,double f2)
{
    double upperbnd, lowerbnd;
    upperbnd = f2 + FLTERR;
    lowerbnd = f2 - FLTERR;
    if((f1>upperbnd) || (f1<lowerbnd))
        return(0);
    return(1);
}

int isnumeric(char *str)
{
    char *p = str;
    int pntcnt = 0;
    int valcnt = 0;
    int cnt = 0;
    while(*p != ENDOFSTR) {
        cnt++;
        if(isdigit(*p)) {
            valcnt++;
            p++;
            continue;
        }
        if(*p == '.') {
            pntcnt++;
            if(pntcnt > 1)
                return 0;
            p++;
            continue;
        }
        if(*p == '-') {
            if(cnt != 1)
                return 0;
            p++;
            continue;
        }
        return 0;
    }
    if(valcnt <= 0)
        return 0;
    return 1;
}
