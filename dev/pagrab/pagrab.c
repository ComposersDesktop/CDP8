/*
 * Copyright (c) 1983-2023 Trevor Wishart and Composers Desktop Project Ltd
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




/**************************************************************************
 *
 * Convert snd data to text format suitable for Pview in Sound Loom
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osbind.h>
#include <math.h>
#include <float.h>
#include <float.h>
#include <sfsys.h>
#include <cdplib.h>

#define VIEW_WIDTH          900     /* width of sloom display */
#define VIEW_HEIGHT         300     /* heigth of sloom display */
#define HALF_VIEW_HEIGHT    150     /* half heigth of sloom display */
#define COLOR_SPECTRUM       45     /* number of individual colours supported */

#define EFFECTIVE_VIEW_WIDTH (VIEW_WIDTH - (VIEW_HEIGHT * 2))

#define F_SECSIZE   (256)

/*  Functions   */

static  int     tidy_up(int,unsigned int);
static  int     open_in(char*), open_out(char *);
static  int     get_big_buf(void);
static  int     write_output(int obufcnt);
static  int     write_window(int totchans,float *rbuf,int bufcnt);
static  int     make_outfile(int del,char *filename);

float *bigbuf, *sampbuf, *osambuf;

float arate, frametime;
int buflen, obuflen;        /* buffer length in samps           */
int  ifd, ofd;              /* input soundfile descriptor       */
int channels;               /* number of channels of input      */
int startwin, endwin, startchan, endchan, outfilesize, obufcnt;
char outfilename[200];

int srate, origstype, origrate, stype;
int Mlen, Dfac, del;

SFPROPS sfprops = {0};
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
    unsigned int start = hz1000();
    double starttime, endtime;

    /* initialise SFSYS */
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if( sflinit("paview") < 0 ) {
        fprintf(stdout,"ERROR: Cannot initialise soundfile system.\n");
        fflush(stdout);
        return tidy_up(3,start);
    }
    if(argc!=8)  {
        fprintf(stdout,"ERROR: Wrong number of arguments.\n");
        fflush(stdout);
        return tidy_up(2,start);
    }
    if(sscanf(argv[2],"%lf",&starttime)!=1) {
        fprintf(stdout,"ERROR: cannot read start time.\n");
        fflush(stdout);
        return tidy_up(2,start);
    }
    if(sscanf(argv[3],"%lf",&endtime)!=1) {
        fprintf(stdout,"ERROR: cannot read end time.\n");
        fflush(stdout);
        return tidy_up(2,start);
    }
    if(sscanf(argv[4],"%s",outfilename)!=1) {
        fprintf(stdout,"ERROR: cannot outfilename.\n");
        fflush(stdout);
        return tidy_up(2,start);
    }
    if(sscanf(argv[5],"%d",&startchan)!=1) {
        fprintf(stdout,"ERROR: cannot read sonogram flag.\n");
        fflush(stdout);
        return tidy_up(2,start);
    }
    if(sscanf(argv[6],"%d",&endchan)!=1) {
        fprintf(stdout,"ERROR: cannot read sonogram flag.\n");
        fflush(stdout);
        return tidy_up(2,start);
    }
    if(sscanf(argv[7],"%d",&del)!=1) {
        fprintf(stdout,"ERROR: cannot read deletion flag.\n");
        fflush(stdout);
        return tidy_up(2,start);
    }
    /* open input file */
    if(open_in(argv[1]) < 0)
        return tidy_up(2,start);

    startwin = (int)floor(starttime/frametime);
    endwin   = (int)ceil(endtime/frametime);

    if(open_out(argv[4]) < 0)
        return tidy_up(2,start);

    /* get biggest buffer */
    if(get_big_buf() == 0)
        return tidy_up(1,start);
        
    if(make_outfile(del,outfilename)<0)
        return tidy_up(1,start);

    /* tidy up */
    return tidy_up(0,start);
}

/**************************** OPEN_IN ****************************
 *
 * opens input soundfile and gets header 
 */

int open_in(char *name) /* opens input soundfile and gets header */
{
    if( (ifd = sndopenEx(name,0,CDP_OPEN_RDONLY)) < 0 ) {
        fprintf(stdout,"ERROR: Cannot open file: %s\n\t",name);
        fflush(stdout);
        return(-1);
    }
    if(snd_headread(ifd,&sfprops)<=0) {
        fprintf(stdout,"ERROR: Cannot read file properties: %s\n\t",name);
        fflush(stdout);
        return(-1);
    }

    channels = sfprops.chans;
    arate = sfprops.arate;
    origrate = sfprops.origrate;
    Mlen = sfprops.winlen;
    Dfac = sfprops.decfac;

    frametime  = (float)(1.0/ sfprops.arate);
    if(sfprops.type != wt_analysis){
        fprintf(stdout,"ERROR: %s is not an analysis file\n",name);
        fflush(stdout);
        return(-1);
    }
    return 0;
}

/**************************** OPEN_OUT ****************************
 *
 * opens input soundfile and gets header 
 */

int open_out(char *name)    /* opens input soundfile and gets header */
{
    outfilesize = (endwin - startwin) * channels;
    if((ofd = sndcreat_ex(name,outfilesize,&sfprops,0,CDP_CREATE_NORMAL)) < 0) {
        fprintf(stdout,"ERROR: Cannot open output file %s\n",name);
        fflush(stdout);
        return(-1);
    }
    return 0;
}

/**************************** GET_BIG_BUF ****************************
 *
 * allocates memory for the biggest possible buffer 
 */

int get_big_buf(void)
{
    size_t i;

    if((bigbuf=(float*)Malloc(((channels * 4) + F_SECSIZE) * sizeof(float))) == NULL) {
        fprintf(stdout,"ERROR: Failed to allocate float buffer.\n");
        fflush(stdout);
        return 0;
    }
    i = ((size_t)bigbuf+sizeof(float)-1)/sizeof(float)*sizeof(float);   /* align bigbuf to word boundary */
    bigbuf = (float*)i;
    memset((char *)bigbuf,0,(channels * 4) * sizeof(float));
    sampbuf = bigbuf + channels;
    /* allow for wrap-around */
    buflen = channels;
    obuflen = (int)ceil((double)channels / (double)F_SECSIZE);
    obuflen *= F_SECSIZE;
    osambuf = sampbuf + channels;
    /* allow for wrap-around */
    return 1;
}

/**************************** MAKE TEXTFILE *****************************/

int make_outfile(int del,char *filename)
{
    int samplen, total_windowsgot_got, samps_remain;
    int got, sampsgot, windowsgot, bufcnt;
    int channelstoshow, done_display /*, changrpcnt*/;
    float *rbuf;
//    double channelmax;
    int n, displayed_channel_cnt, channel_cnt, totchans;
    
    if((samplen  = sndsizeEx(ifd))<0) {     /* FIND SIZE OF FILE */
        fprintf(stdout, "ERROR: Can't read size of input file %s.\n",filename);
        fflush(stdout);
        return -1;
    }
    total_windowsgot_got = 0;
    samps_remain = 0;
    rbuf = sampbuf;
    totchans = channels/2;
    if((channelstoshow = endchan - startchan) == 0) {
        if(endchan != totchans)
            endchan++;
        else if(startchan !=0)
            startchan--;
        channelstoshow++;
    }

    memset((char *)osambuf,0, obuflen * sizeof(float));
    done_display = 0;

    obufcnt = 0;                                    /* count in buffer of output values */
    while((got = fgetfbufEx(sampbuf,buflen,ifd,1)) > 0 ) {
        sampsgot = samps_remain;                    /* Take into account any wrapped around values */
        sampsgot += got;
        windowsgot = sampsgot/channels;
        samps_remain = sampsgot - (windowsgot * channels);
        bufcnt = 0;                                 /* cnt in buffer we're reading from (which includes wrap-around) */
        for(n=0;n<windowsgot;n++) {                 /* for every window */
            total_windowsgot_got++;                 /* count the windows */
            if(total_windowsgot_got <= startwin) {  /* if not at 1st window to display, continue */
                if(del)
                    if(write_window(totchans,rbuf,bufcnt)<0)
                        return -1;
                bufcnt += channels;
                continue;
            }
            else if(total_windowsgot_got > endwin) {            /* if beyond last window to display, quit */
                if(del) {
                    if(write_window(totchans,rbuf,bufcnt)<0)
                        return -1;
                    bufcnt += channels;
                    continue;
                } else {
                    done_display = 1;
                    break;
                }
            }
//            changrpcnt = 0;                         /* count of set-of-channels to group together for display */
//            channelmax = 0.0;
            for(channel_cnt=0;channel_cnt < startchan;channel_cnt++) {
                if(del) {
                    osambuf[obufcnt++] = rbuf[bufcnt++];
                    osambuf[obufcnt++] = rbuf[bufcnt++];
                    if(obufcnt >= obuflen) {
                        if(write_output(obufcnt) < 0)
                            return -1;
                    }
                } else {
                    osambuf[obufcnt++] = 0.0;
                    bufcnt++;
                    osambuf[obufcnt++] = rbuf[bufcnt++];
                    if(obufcnt >= obuflen) {
                        if(write_output(obufcnt) < 0)
                            return -1;
                    }
                }
            }
            for(displayed_channel_cnt = 0; displayed_channel_cnt < channelstoshow; displayed_channel_cnt++) {
                if(del) {
                    osambuf[obufcnt++] = 0.0;
                    bufcnt++;
                    osambuf[obufcnt++] = rbuf[bufcnt++];
                    if(obufcnt >= obuflen) {
                        if(write_output(obufcnt) < 0)
                            return -1;
                    }
                } else {
                    osambuf[obufcnt++] = rbuf[bufcnt++];
                    osambuf[obufcnt++] = rbuf[bufcnt++];
                    if(obufcnt >= obuflen) {
                        if(write_output(obufcnt) < 0)
                            return -1;
                    }
                }
            }
            channel_cnt += displayed_channel_cnt;
            while(channel_cnt < totchans) {
                if(del) {
                    osambuf[obufcnt++] = rbuf[bufcnt++];
                    osambuf[obufcnt++] = rbuf[bufcnt++];
                    if(obufcnt >= obuflen) {
                        if(write_output(obufcnt) < 0)
                            return -1;
                    }
                } else {
                    osambuf[obufcnt++] = 0.0;
                    bufcnt++;
                    osambuf[obufcnt++] = rbuf[bufcnt++];
                    if(obufcnt >= obuflen) {
                        if(write_output(obufcnt) < 0)
                            return -1;
                    }
                }
                channel_cnt++;
            }
        }
        if(done_display)                            /* if reached end of windows to display, quit */
            break;

        if(samps_remain) {                          /* if there are extra samples beyond end of windows */
            rbuf = sampbuf - samps_remain;          /* wrap them around */
            memcpy((char *)rbuf,(char *)(rbuf + (windowsgot * channels)),samps_remain * sizeof(float));
        } else
            rbuf = sampbuf;
    }
    if(obufcnt >= 0) {
        if(write_output(obufcnt) < 0)
            return -1;
    }
    sndcloseEx(ofd);
    return(0);
}

/**************************** TIDY_UP ****************************
 *
 * Exit, freeing buffers and closing files where necessary.
 */

int tidy_up(int where,unsigned int start)
{
    switch(where) {
    case 0:
        Mfree(bigbuf);
    case 1:
        sndcloseEx(ifd);
    case 2:
//      sffinish();
    default:
        break;
    }
    while(!(hz1000() - start))
        ;
    return(1);
}
    
/**************************** WRITE_OUTPUT ****************************/

int write_output(int samps_written)
{
    if(fputfbufEx(osambuf,samps_written,ofd) < 0) {
        fprintf(stdout, "ERROR: Failed to complete data write to analysis output file.\n");
        if(sndunlink(ofd) < 0)
            fprintf(stdout, "ERROR: Can't set output analysis for deletion.\n");
        if(sndcloseEx(ofd) < 0)
            fprintf(stdout, "WARNING: Can't close output analysis.\n");
        fflush(stdout);
        return -1;
    }
    memset((char *)osambuf,0, obuflen * sizeof(float));
    obufcnt = 0;
    return 0;
}

/**************************** WRITE_WINDOW ****************************/

int write_window(int totchans,float *rbuf,int bufcnt)
{
    int channel_cnt;
    for(channel_cnt=0;channel_cnt < totchans;channel_cnt++) {
        osambuf[obufcnt++] = rbuf[bufcnt++];
        osambuf[obufcnt++] = rbuf[bufcnt++];
        if(obufcnt >= obuflen) {
            if(write_output(obufcnt) < 0)
                return -1;
        }
    }
    return 0;
}

