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

//nmix.c
//Oct 2009 updated to use portsf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <sys/timeb.h>
#include "portsf.h"

#ifdef unix
/* in portsf.lib */
extern int stricmp(const char *a, const char *b);
#endif

//usage: nmix [-oOFFSET]  [-f] infile1 infile2 outfile
void usage() {
    printf("\nCDP MCTOOLS: NMIX V2.0.1 (c) RWD,CDP 1999,2009\n"
           "mix two multi-channel files\n"
           "usage:\n"
           "nmix [-d][-f][-oOFFSET] infile1 infile2 outfile\n"
           "     -d      : apply TPDF dither (16bit format output only).\n"
           "     -f      : set output sample type to floats.\n"
           "                Default: outfile type is that of infile 1\n"
           "     -oOFFSET:  start infile2 at OFFSET seconds.\n"
           "Files must have the same channel count.\n"
           "WAVE-EX files must have the same speaker layout\n");

    exit(0);
}

#define BUFLEN (1024)

int main(int argc, char *argv[])
{
    int i,ifd1 = -1,ifd2=-1,ofd=-1;
    int force_floats = 0;
    int do_dither = 0;
    float ampfac = 0.0;
    long len_in1, len_in2,outlen;
    double offset = 0.0;
    long offsetframes = 0,framecount = 0;
    long chans;
    long sr;
    long in1_sofar,in2_sofar;
    int halfsec;
    MYLONG peaktime;
    PSF_PROPS props_in1,props_in2;
    float *frame_in1 = NULL, *frame_in2 = NULL, *outframe = NULL;
    PSF_CHPEAK *peaks = NULL;

    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("2.0.1\n");
        return 0;
    }

    if(argc < 4)
        usage();

    while(argv[1][0]=='-'){
        char *valstr;
        double val;
        switch(argv[1][1]){
        case 'd':
            do_dither = 1;
            break;
        case('f'):
            force_floats = 1;
            break;
        case('o'):
            if(argv[1][2]=='\0'){
                fprintf(stderr,"-o flag needs a number\n");
                exit(1);
            }
            valstr = &argv[1][2];
            val=atof(valstr);
            if(val < 0.0){

                fprintf(stderr,"offset must be >= 0\n");
                exit(1);
            }
            if(val > 0.0)
                offset = val;
            break;

        default:
            fprintf(stderr,"incorrect flag option %c\n",argv[1][1]);
            exit(1);
        }
        argc--; argv++;
    }

    if(argc < 4)
        usage();
    if(force_floats && do_dither)
        fprintf(stderr,"Warning: dither option ignored for floats format.\n");

    psf_init();

    //open soundfiles: no auto-rescaling
    if((ifd1 = psf_sndOpen(argv[1],&props_in1,0)) < 0){
        fprintf(stderr,"Cannot open soundfile %s\n",argv[1]);
        goto cleanup;
    }

    if((ifd2 = psf_sndOpen(argv[2],&props_in2,0)) < 0){
        fprintf(stderr,"Cannot open soundfile %s \n",argv[2]);
        goto cleanup;
    }

    if(props_in1.chans != props_in2.chans){
        fprintf(stderr,"files do not have the same number of channels\n");
        goto cleanup;
    }

    if(props_in1.srate != props_in2.srate){
        fprintf(stderr,"files do not have the same sample rate\n");
        goto cleanup;

    }

    if(props_in1.chformat != props_in2.chformat){
        fprintf(stderr,"files do not have the same channel format\n");
        goto cleanup;
    }


    sr = props_in1.srate;
    chans  = props_in1.chans;
    ampfac = 0.5f;                                  //just mixing 2 files...
    if(offset > 0.0){
        offsetframes = (long) (offset * (double) sr);
    }
    /* now we can set up frame arrays */

    frame_in1 = (float *) calloc(chans, sizeof(float));
    frame_in2 = (float *) calloc(chans, sizeof(float));
    outframe = (float *) calloc(chans, sizeof(float));

    if(frame_in1==NULL || frame_in2==NULL || outframe==NULL ){
        puts("\nno memory for frame buffers");
        goto cleanup;
    }

    len_in1 = psf_sndSize(ifd1);
    if(len_in1==0){
        fprintf(stderr,"infile %s is empty!\n",argv[1]);
        goto cleanup;
    }
    len_in2 = psf_sndSize(ifd2);
    if(len_in2==0){
        fprintf(stderr,"infile %s is empty!\n",argv[2]);
        goto cleanup;
    }
    if(len_in1 < 0){
        fprintf(stderr,"system problem: cannot read size of infile %s\n",argv[1]);
        goto cleanup;
    }

    if(len_in2 < 0){
        fprintf(stderr,"system problem: cannot read size of infile %s\n",argv[2]);
        goto cleanup;
    }



    outlen = len_in2 + offsetframes;
    if(len_in1 > outlen)
        outlen = len_in1;

#ifdef _DEBUG
    printf("DEBUG: outfile size expected to be %d frames\n",outlen);
#endif
    //setup PEAK data
    peaks = (PSF_CHPEAK *) calloc(props_in1.chans,sizeof(PSF_CHPEAK));
    if(peaks==NULL){
        puts("nmix: error: no memory for internal PEAK buffer\n");
        goto cleanup;
    }
    if(force_floats)
        props_in1.samptype = PSF_SAMP_IEEE_FLOAT;


    if(!is_legalsize(outlen,&props_in1)){
        fprintf(stderr,"error: outfile size exceeds capacity of format.\n");
        return 1;
    }


    if((ofd = psf_sndCreate(argv[3],&props_in1,0,0,PSF_CREATE_RDWR)) < 0){
        fprintf(stderr,"unable to create outfile %s\n",argv[3]);
        goto cleanup;
    }
    if(do_dither)
        psf_sndSetDither(ofd,PSF_DITHER_TPDF);
    halfsec = sr / 2;
    //OK now we can do it....
    printf("\nmixing....\n");

    in1_sofar = in2_sofar = 0;
    if(offsetframes > 0){
        for(i=0;i < offsetframes; i++){
            int got,j;
            got = psf_sndReadFloatFrames(ifd1,frame_in1,1);
            if(got != 1){
                fprintf(stderr,"error reading from infile 1\n");
                goto cleanup;
            }
            for(j=0;j < chans; j++) {
                frame_in1[j] *= ampfac;
            }
            if((psf_sndWriteFloatFrames(ofd,frame_in1,1)) !=1){
                fprintf(stderr,"\nerror writing to outfile\n");
                goto cleanup;
            }
            if(framecount % halfsec==0)
                printf("%.2lf secs\r",(double)framecount / (double) sr);

            framecount++;
        }
    }
    in1_sofar = offsetframes;
    //now we are mixing two files...
    for(i= offsetframes; i < outlen; i++){
        int got, j;
        //clear frame blocks
        memset(frame_in1,0, chans * sizeof(float));
        memset(frame_in2,0, chans * sizeof(float));
        //if we  have data, fill frame and scale
        if(in1_sofar < len_in1){
            got = psf_sndReadFloatFrames(ifd1,frame_in1,1);
            if(got != 1){
                fprintf(stderr,"\nerror reading from infile 1\n");
                goto cleanup;
            }
            in1_sofar++;
            for(j=0;j < chans; j++)
                frame_in1[j] *= ampfac;
        }


        if(in2_sofar < len_in2){
            got = psf_sndReadFloatFrames(ifd2,frame_in2,1);
            if(got != 1){
                fprintf(stderr,"\nerror reading from infile 1\n");
                goto cleanup;
            }

            in2_sofar++;

            for(j=0;j < chans; j++)
                frame_in2[j] *= ampfac;

        }

        //mix and write
        for(j=0;j < chans; j++) {
            outframe[j] = frame_in1[j] + frame_in2[j];

        }
        if(psf_sndWriteFloatFrames(ofd,outframe,1) < 0){
            fprintf(stderr,"\nerror writing to outfile\n");
            goto cleanup;
        }
        if(framecount % halfsec==0) {
            printf("%.2lf secs\r",(double)framecount / (double) sr);
            fflush(stdout);
        }
        framecount++;
    }
    printf("%.4lf secs\nWritten %ld sample frames to %s\n",(double)framecount / (double)sr,framecount,argv[3]);
    if(psf_sndReadPeaks( ofd,peaks,&peaktime)){
        printf("PEAK values:\n");
        for(i=0; i < chans; i++){
            double val, dbval;
            val = (double) peaks[i].val;

            if(val > 0.0){
                dbval = 20.0 * log10(val);
                printf("CH %d: %.6f (%.2lfdB) at frame %u:\t%.4f secs\n",i,
                       val,dbval,(unsigned int) peaks[i].pos,(double)peaks[i].pos / (double) props_in1.srate);
            }
            else{
                printf("CH %d: %.6f (-infdB) at frame %u:\t%.4f secs\n",i,
                       val,(unsigned int) peaks[i].pos,(double)peaks[i].pos / (double) props_in1.srate);
            }
        }
    }



 cleanup:
    if(ifd1 >=0)
        psf_sndClose(ifd1);
    if(ifd2 >=0)
        psf_sndClose(ifd2);
    if(ofd >=0)
        psf_sndClose(ofd);
    if(peaks)
        free(peaks);
    if(frame_in1 != NULL)
        free(frame_in1);

    if(frame_in2 != NULL)
        free(frame_in2);
    if(outframe != NULL)
        free(outframe);


    psf_finish();
    return 0;
}
