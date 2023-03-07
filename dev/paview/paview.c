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
static  int     open_in(char*);
static  int     get_big_buf(void);
static  int     make_textfile(int sonogram,char *filename);
static  int     write_output(int outlen,double *outbuf,double convertor,int sonogram,FILE *fp);


float *bigbuf, *sampbuf;

int Mlen;
float arate, frametime;
int buflen;             /* buffer length in samps           */
int  ifd;                   /* input soundfile descriptor       */
int  channels, expanded;    /* number of channels of input      */
int startwin, endwin, startchan, endchan;
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
    unsigned int start = hz1000();
    char filename[200];
    double starttime, endtime;
    int sonogram;

    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    /* initialise SFSYS */
    if( sflinit("paview") < 0 ) {
        fprintf(stdout,"ERROR: Cannot initialise soundfile system.\n");
        fflush(stdout);
        return tidy_up(3,start);
    }
    if(argc!=7)  {
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
    if(sscanf(argv[4],"%d",&sonogram)!=1) {
        fprintf(stdout,"ERROR: cannot read sonogram flag.\n");
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
    /* open input file */
    if(open_in(argv[1]) < 0)
        return tidy_up(2,start);

    startwin = (int)floor(starttime/frametime);
    endwin   = (int)ceil(endtime/frametime);

    strcpy(filename,argv[1]);

    /* get biggest buffer */
    if(get_big_buf() == 0)
        return tidy_up(1,start);
        
    /* max soundfiles */
    if(make_textfile(sonogram,filename)<0)
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
    if((ifd = sndopenEx(name,0,CDP_OPEN_RDONLY)) < 0)   {
        fprintf(stdout,"ERROR: Cannot open file: %s\n\t",name);
        fflush(stdout);
        return(-1);
    }

    /*  get channels        */

    if(sndgetprop(ifd, "channels", (char*)&channels, sizeof(int)) < 0) {
        fprintf(stdout,"ERROR: Cannot get channel count of %s\n",name);
        fflush(stdout);
        return(-1);
    }
    /*  get frametime       */

    if(sndgetprop(ifd,"arate",(char *)&arate,sizeof(float)) < 0){
        fprintf(stdout,"ERROR: Failure to read analysis sample rate, in %s\n",name);
        fflush(stdout);
        return(-1);
    }
    frametime  = (float)(1.0/arate);
    return 0;
}

/**************************** GET_BIG_BUF ****************************
 *
 * allocates memory for the biggest possible buffer 
 */

int get_big_buf()
{
    size_t i;

    if((bigbuf=(float*)Malloc(((channels * 2) + F_SECSIZE) * sizeof(float))) == NULL) {
        fprintf(stdout,"ERROR: Failed to allocate float buffer.\n");
        fflush(stdout);
        return 0;
    }
    i = ((size_t)bigbuf+sizeof(float)-1)/sizeof(float)*sizeof(float);   /* align bigbuf to word boundary */
    bigbuf = (float*)i;
    memset((char *)bigbuf,0,(channels * 2) * sizeof(float));
    sampbuf = bigbuf + channels;
    buflen = channels;
    /* allow for wrap-around */
    return 1;
}

/**************************** MAKE TEXTFILE *****************************/

int make_textfile(int sonogram,char *filename)
{
    FILE *fp;
    int samplen, zoomfact, total_windowsgot_got, samps_remain;
    int got, sampsgot, windowsgot, bufcnt, wingrpcnt;
    int channelstoshow, chanscale, ch_outlen, w_outlen, done_display, obufcnt, changrpcnt;
    float *rbuf;
    double *outbuf;
    double channelmax;
    int n, displayed_channel_cnt, channel_cnt, totchans;
    double convertor, zscale;
    
    expanded = 0;
    if(sonogram) {
        w_outlen = VIEW_WIDTH;
        ch_outlen = VIEW_HEIGHT;
        convertor = COLOR_SPECTRUM / 3.0; /* converts range 0 --> 3 TO range of colours */
    } else  {
        w_outlen = EFFECTIVE_VIEW_WIDTH;
        ch_outlen = HALF_VIEW_HEIGHT;
        convertor = HALF_VIEW_HEIGHT / 6.0; /* converts range 0 --> 3 TO range 0 -> quarter-view-heigth */
    }


    if((outbuf = (double *)malloc(ch_outlen * sizeof(double))) == NULL) {
        fprintf(stdout, "ERROR: Insufficient memory to store output values.\n");
        fflush(stdout);
        return -1;
    }
    if((samplen  = sndsizeEx(ifd))<0) {     /* FIND SIZE OF FILE */
        fprintf(stdout, "ERROR: Can't read size of input file %s.\n",filename);
        fflush(stdout);
        return -1;
    }
    if((fp = fopen("cdptest00.txt","w"))==NULL) {
        fprintf(stdout, "ERROR: Failed to open the Sound Loom display file 'cdptest00.txt'\n");
        fflush(stdout);
        return -1;
    }
    zoomfact = max(1L,(int)ceil((double)(endwin - startwin)/w_outlen));
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

    zscale = (double)channelstoshow / (double)ch_outlen;
    if(zscale < .5)
        expanded = (int)round((double)ch_outlen / (double)channelstoshow);

    chanscale = (int)ceil((double)channelstoshow / (double)ch_outlen);  /* ratio of channels to available pixels */

    memset((char *)outbuf,0, ch_outlen * sizeof(double));
    done_display = 0;

    wingrpcnt = 0;                                  /* cnt of set-of-windows to be zoomed together */
    while((got = fgetfbufEx(sampbuf,buflen,ifd,1)) > 0 ) {
        sampsgot = samps_remain;                    /* Take into account any wrapped around values */
        sampsgot += got;
        windowsgot = sampsgot/channels;
        samps_remain = sampsgot - (windowsgot * channels);
        bufcnt = 0;                                 /* cnt in buffer we're reading from (which includes wrap-around) */
        for(n=0;n<windowsgot;n++) {                 /* for every window */
            total_windowsgot_got++;                 /* count the windows */
            if(total_windowsgot_got <= startwin) {  /* if not at 1st window to display, continue */
                bufcnt  += channels;
                continue;
            }
            else if(total_windowsgot_got > endwin) {            /* if beyond last window to display, quit */
                done_display = 1;
                break;
            }
            obufcnt = 0;                            /* count in buffer of output values */
            changrpcnt = 0;                         /* count of set-of-channels to group together for display */
            channelmax = 0.0;
            for(channel_cnt=0;channel_cnt < startchan;channel_cnt++)
                bufcnt += 2;
            for(displayed_channel_cnt = 0; displayed_channel_cnt < channelstoshow; displayed_channel_cnt++) {
                if(rbuf[bufcnt] > channelmax)       /* for each channel to display */   
                    channelmax = rbuf[bufcnt];      /* find maximum value amongst group of channels */
                if(++changrpcnt >= chanscale) {     /* once whole group has been surveyed */
                    if(channelmax > outbuf[obufcnt])/* transfer maxvalue to output, if it's larger than val alreasdy there */
                        outbuf[obufcnt] = channelmax;   /* (we are also possibly averaging over windows, by zoomfact) */
                    obufcnt++;                      /* move to next outbuffer location */
                    channelmax = 0.0;               /* reset channelmax to min */
                    changrpcnt = 0;                 /* start counting next group */
                }
                bufcnt += 2;                        /* move up to next amplitude value */
            }
            if(changrpcnt > 0) {                    /* last channels may not be a full group: if not, output their val here */
                if(channelmax > outbuf[obufcnt])
                    outbuf[obufcnt] = channelmax;
                obufcnt++;
                channelmax = 0.0;
                changrpcnt = 0;
            }
            channel_cnt += displayed_channel_cnt;
            while(channel_cnt < totchans) {
                bufcnt += 2;
                channel_cnt++;
            }
            wingrpcnt++;
            if(wingrpcnt >= zoomfact) {         /* if we have looked at enough windows (in the zoom) */
                if(write_output(ch_outlen,outbuf,convertor,sonogram,fp) < 0) {
                    fclose(fp);
                    fprintf(stdout, "ERROR: Failed to complete data write to Sound Loom display file 'cdptest00.txt'\n");
                    fflush(stdout);
                    return -1;
                }
                memset((char *)outbuf,0, ch_outlen * sizeof(double));
                wingrpcnt = 0;                      /* reset the obuf, and the window counter */
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
    if(wingrpcnt > 0) {                             /* last windows looed at did not form a complete zoom set */
        if(write_output(ch_outlen,outbuf,convertor,sonogram,fp) < 0) {
            fclose(fp);
            fprintf(stdout, "ERROR: Failed to complete data write to Sound Loom display file 'cdptest00.txt'\n");
            fflush(stdout);
            return -1;
        }
    }
    fclose(fp);
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

int write_output(int outlen,double *outbuf,double convertor,int sonogram,FILE *fp)
{
    int z, x, zob=0;
    int channelstoshow; 
    char temp[2000];
    char temp2[64];
    temp[0] = '\0';
    if(expanded) {
        channelstoshow = endchan - startchan;
        for(z = 0;z < channelstoshow; z++) {        /* output trhe results */
            outbuf[z] *= 999.0;
            outbuf[z] += 1.0;                   /* Range    0-1       -->> 1 to 1000  */
            outbuf[z] = log10(outbuf[z]);       /* Range    1 to 1000 -->> 1 to 3  */
            outbuf[z] *= convertor;
            if(sonogram) {
                zob  = (int)round(outbuf[z]);
            }
            for(x = 0; x < expanded; x++) {
                if(sonogram) {
                    sprintf(temp2,"%d",zob);
                } else {
                    sprintf(temp2,"%lf",outbuf[z]);
                }
                strcat(temp2," ");
                strcat(temp,temp2);
            }
        }
    } else {
        for(z = 0;z < outlen; z++) {        /* output trhe results */
            outbuf[z] *= 999.0;
            outbuf[z] += 1.0;                   /* Range    0-1       -->> 1 to 1000  */
            outbuf[z] = log10(outbuf[z]);       /* Range    1 to 1000 -->> 1 to 3  */
            outbuf[z] *= convertor;             /* For Sonogram: Range    1 to 3      -->> 0 to 0 to max colour  */
                                                /* FOr Spectrum: Range    1 to 3      -->> 0 to half of HALF_VIEW_HEIGHT  */
            if(sonogram) {
                sprintf(temp2,"%d",(int)round(outbuf[z]));
            } else {
                sprintf(temp2,"%lf",outbuf[z]);
            }
            strcat(temp2," ");
            strcat(temp,temp2);
        }
    }
    if(fprintf(fp,"%s\n",temp) < 1)
        return -1;
    return 1;
}
