/*
 * Copyright (c) 1983-2013 Richard Dobson and Composers Desktop Project Ltd
 * http://people.bath.ac.uk/masrwd
 * http://www.composersdesktop.com
 * This file is part of the CDP System.
 * The CDP System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * The CDP System is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with the CDP System; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* channelx.c */
//RWD MCTOOLS, portsf version Oct 2009
// nothing special, but use PEAK chunk (reading it where available)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <ctype.h>
#include <portsf.h>

#ifndef _MAX_PATH
#define _MAX_PATH (1024)
#endif
#define MAXFILES 16

#ifdef unix
/* in portsf.lib */
extern int stricmp(const char *a, const char *b);
#endif


void usage(void);
//RWD.7.99 extended to get file extension from base name
// so can be used without CDP_SOUND_EXT
static void getbody(char*,char*,char *);

void
usage(void)
{
    fprintf(stderr,"\nCDP MCTOOLS: CHANNELX V1.6 (c) RWD, CDP 2010\n");
    fprintf(stderr,
            "Extract all channels from m/c file\n"
            "usage: channelx [-oBASENAME] infile chan_no [chan_no...]\n"
            //"\nusage:  channelx [-oBASENAME] infile"
            "  -oBASENAME = base name (with extension) of outfiles (appended *_cN for ch N)\n"
            "(NB: Channels of WAVE-EX files are written as standard soundfiles)\n"
            );
}

#if defined(unix)
#define SEP     '/'
#else
#define SEP     '\\'
#endif

void getbody(char *filename,char *body,char *ext)
{
    char *fn, *sl;

    if((sl = strrchr(filename, SEP)) != 0)
        sl++;
    else
        sl = filename;

    for(fn = sl; *fn != '.' && *fn != '\0'; ) {
        *body++ = *fn++;
    }
    *body++ = '\0';
    //save extension
    while(*fn != '\0')
        *ext++ = *fn++;
    *ext = '\0';
}





int main(int argc,char *argv[])
{

    int ifd = 0;                            /* Infile descriptor */
    int ofd[MAXFILES];                      /* Outfile descriptors */
    int i;                      /* Assorted indexes */
    int inchans=1;                          /* Infile channels */
    char filename[_MAX_PATH];       /* Infilename */
    int select[MAXFILES];           /* Flags for each channel to MAXFILES */
    int lastchan = 0;                       /* Maximum Requested channels */
    int numchans = 0;                       /* number of channels extracted */
    char body[100];                         /* Infile name prefix */
    char ext[8];
    char outext[8];                         /* output extension: to change .amb to .wav */
    char *p_dot;
    char ofile[_MAX_PATH];          /* Raw outfile name */
    PSF_PROPS inprops;
    int nframes =0, total_frames = 0;
    float *frame = NULL;
    int writefiles = 0;                     /* try to keep working even if one file fails...*/


    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("1.6.\n");
        return 0;
    }
    if(argc<2) {
        usage();
        return 1;
    }

    if(psf_init()) {
        fprintf(stderr,"Startup failure.\n");
        return 1;
    }

    body[0] = '\0';
    ext[0] = '\0';

    while(argv[1] != 0 && argv[1][0] == '-') {
        if(strncmp(argv[1], "-o", 2) == 0) {
            if(body[0] != '\0')
                usage();
            strcpy(body, &argv[1][2]);
        }
        argv++;
        argc--;
    }
    if(argc < 2) {
        usage();
        return 1;
    }

    if(argv[1] != 0) {
        sprintf(filename,"%s",argv[1]);
        if((ifd = psf_sndOpen(filename,&inprops,0)) < 0) {
            fprintf(stderr,"Channelx: Cannot open input file %s\n",argv[1]);
            return 1;
        }
    }
    argv++;

    if(inprops.chans > MAXFILES){
        fprintf(stderr,"Channelx: infile has too many channels!\n");
        psf_sndClose(ifd);
        return 1;
    }
    inchans = inprops.chans;
    nframes = psf_sndSize(ifd);
    if(nframes < 0){
        fprintf(stderr,"Channelx: error reading infile size\n");
        psf_sndClose(ifd);
        return 1;
    }
    if(nframes==0){
        fprintf(stderr,"Channelx: infile contains no data!\n");
        psf_sndClose(ifd);
        return 1;
    }
    nframes /= inchans;
    //always create a standard file!
    if(inprops.format == PSF_WAVE_EX) {
        inprops.format = PSF_STDWAVE;
        inprops.chformat = STDWAVE;
    }
    //that's all we need to know, now set one output channel
    inprops.chans = 1;

    frame = calloc(inchans,sizeof(float));
    if(frame == NULL){
        puts("Channelx: no memory for internal sample data\n");
        psf_sndClose(ifd);
        return 1;
    }

    for(i=0;i<MAXFILES;i++){
        select[i] = 0;
        ofd[i] = -1;
    }


    while(argv[1] != 0) {
        int chn;

        if(sscanf(argv[1],"%d",&chn) < 1){
            fprintf(stderr,
                    "Channelx: illegal channel argument %s\n\n",argv[1]);
            goto cleanup;
        }

        if((chn > inchans) || (chn < 1)){
            fprintf(stderr,"Channelx: channel argument out of range\n\n");
            goto cleanup;
        }
        //check for duplicates
        if(select[chn-1]==0){
            select[chn-1] = 1;
            if(chn > lastchan)
                lastchan = chn;
            numchans++;
        }
        else
            fprintf(stderr,"Channelx: WARNING: duplicate channel number %d\n",chn);

        argv++;
        argc--;
    }
    if(numchans < 1) {              //RWD
        fprintf(stderr, "channelx: must extract at least one channel\n");
        goto cleanup;
    }

    writefiles = numchans;

    if(body[0] == '\0')
        getbody(filename,body,ext);

    else if((p_dot = strrchr(body,'.')) != NULL){
        //extension set in body arg
        *p_dot++ = '\0';
        sprintf(ext,".%s",p_dot);
    }
    //trap ambisonic file; single channels must be wav
    strcpy(outext,ext);
    if(stricmp(outext,".amb")==0)
        strcpy(outext,".wav");
    for(i=0;i<inchans;i++){
        if(select[i]){
            sprintf(ofile, "%s_c%d%s", body, i+1,outext);


            if((ofd[i] = psf_sndCreate(ofile,&inprops,0,0,PSF_CREATE_RDWR)) < 0) {
                fprintf(stderr,"Channelx: Failed to create %s\n\n",ofile);
                goto cleanup;
            }
        }
    }

    printf("Extracting %d channels from '%s':\n%d sample frames\n",numchans,filename,nframes);

    //stopwatch(1);
    for(i=0;i < nframes; i++){
        int got;
        if(writefiles==0)
            goto cleanup;

        if((got = psf_sndReadFloatFrames(ifd,frame,1)) < 1){
            if(got < 0)
                fprintf(stderr,"error reading data from infile, after %d frames\n",i);
            //sndunlink all outfiles?
            else
                break;
            //goto cleanup;
        }

        for(i=1;i<lastchan+1;i++){
            if(select[i-1]){
                if((psf_sndWriteFloatFrames(ofd[i-1],frame +(i-1),1))<0){
                    fprintf(stderr,"Channelx: Write [channel %d] failed.\n\n",i);
                    //RWD
                    //sndunlink(ofd[i-1]);
                    writefiles--;
                    if(writefiles==0)
                        break;
                    else
                        continue;       //may be other files we can still write to...
                }
            }
        }
        total_frames++;
        if((total_frames % (inprops.srate/2)) == 0){
            printf("%.2lf secs\r",(double)total_frames / (double)inprops.srate);
            fflush(stdout);
        }
    }
    printf("%.4lf secs\n",(double)total_frames / (double)inprops.srate);
    printf("%d files created.\n",writefiles);

 cleanup:
    psf_sndClose(ifd);

    for(i=0;i < MAXFILES; i++){
        if(ofd[i] >= 0)
            psf_sndClose(ofd[i]);
    }


    free(frame);
    psf_finish();
    return 0;
}
