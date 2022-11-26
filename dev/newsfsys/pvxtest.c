//
//  pvxtest.c


#include <stdio.h>
#include <stdlib.h>
#include "sfsys.h"

float ampmax(float* buffer, int samps)
{
    double amp = 0.0;
    int iamp;
    for (iamp=0; iamp < samps ;iamp += 2) {
        float thisamp = buffer[iamp];
        amp =  max(amp, thisamp);
    }
    return amp;
}

int main(int argc, char **argv)
{
    SFPROPS props;
    int ifd, ofd, i, bufcount, sampswanted, got,put;
    int numsamps,numframes, pos;
    float *buf = NULL;
    float *framesbuf = NULL;
    float *framebufptr = NULL;
    int frames_per_buf = 10;
    int framecount = 0;
    int filesize = 0;
    int bytedatasize;
    
    if(argc< 3){
        fprintf(stderr, "usage: pvxtest sfile.pvx outfile.ana\n");
        return 1;
    }
    if(sflinit("pvxtest")){
        fprintf(stderr,"pvxtest: unable to initialize CDP Sound Filing System\n");
        return 1;
    }
    if((ifd = sndopenEx(argv[1],0,CDP_OPEN_RDONLY)) < 0){
        fprintf(stderr,"pvxtest: unable to open infile %s: %s\n",argv[1], sferrstr());
        return 1;
    }
    else {
        fprintf(stderr,"pvxtest: file opened OK, ifd = %d\n",ifd );
    }
    //read file size if various ways...
    filesize = sndsizeEx(ifd);
    fprintf(stderr,"got filesize = %d\n",filesize);
    
    
    if(!snd_headread(ifd,&props)){
        fprintf(stderr,"pvxtest: unable to read infile header: %s\n",sferrstr());
        return 1;
    }
    //read file size if various ways...
    filesize = sndsizeEx(ifd);
    fprintf(stderr,"got filesize = %d\n",filesize);
    /* do we have correct props? */
    fprintf(stderr, "props.srate: = %d\nprops.chans: %d\n",
            props.srate,props.chans);
    fprintf(stderr, "props.origsize: %d\nprops.origrate:%d\nprops.origchans:%d (pitch/fmt/trans only\n",
            props.origsize, props.origrate,props.origchans);
    fprintf(stderr,"props.arate:%f\nprops.winlen:%d\nprops.decfac :%d\n",
            props.arate,props.winlen,props.decfac);
    
    framesbuf = malloc(props.chans * frames_per_buf);
    if(framesbuf == NULL){
        fprintf(stderr, "alloc failure\n");
        return 1;
    }

    props.type = wt_analysis;
    props.format = WAVE;
    props.samptype = FLOAT32;
    fprintf(stderr,"setting output chans to %d\n",props.chans);
    //ofd = sndcreat_ex(argv[2],-1,&props,SFILE_CDP,CDP_CREATE_NORMAL);
    ofd = sndcreat_formatted(argv[2],-1,SAMP_FLOAT,props.chans,props.srate,CDP_CREATE_NORMAL);
    if(ofd < 0){
        fprintf(stderr,"pvxtest: unable to open outfile %s: %s\n",argv[2], sferrstr());
        return 1;
    }
#ifndef SINGLEFRAME
    sampswanted = props.chans;
    buf = (float *) malloc(sizeof(float) * sampswanted);
    if(buf == NULL){
        fprintf(stderr,"failed to allocate buf of %d samps\n",sampswanted);
        return -1;
    }
    bufcount = 0;
    //for(i=0;i < 20;i++){
    for(i=0; /* i < numframes */; i++){
        got = fgetfbufEx(buf, sampswanted,ifd,0);
        if(got <= 0)
            break;
        if(i==0 & got == sampswanted){
            fprintf(stderr,"%d: read %d samps, max = %f\n",bufcount,got, ampmax(buf,got));
        }
        put = fputfbufEx(buf,sampswanted,ofd);
        if(put < 0){
            fprintf(stderr,"error writing to oputfile, block %d\n",i);
        }
        bufcount++;
    }
    fprintf(stderr,"written %d frames\n",bufcount);
#else
    sampswanted = props.chans * frames_per_buf;
    for(i=0;;i++){
        got = fgetfbufEx(framesbuf,sampswanted,ifd,0);
        if(got <= 0)
            break;
        put = fputfbufEx(framesbuf,got,ifd);
        if(put < 0){
            fprintf(stderr,"error writing to oputfile, block %d\n",i);
        }
        framecount += got / props.chans;
    }
    fprintf(stderr, "processed %d frames\n",framecount);
#endif
    
    //must set all CDP analysis props before closing
    fprintf(stderr, "adding prop orig sampsize: %d\n",props.origsize);
    if(sndputprop(ofd,"original sampsize",(char *) &(props.origsize),sizeof(int)) < 0){
        fprintf(stderr,"Failure to write original sample size %s\n", (char*)&(props.origsize));
    }
    fprintf(stderr, "adding prop orig srate: %d\n",props.origrate);
    if(sndputprop(ofd,"original sample rate",(char *) &(props.origrate),sizeof(int)) < 0){
        fprintf(stderr,"Failure to write original sample size\n");
    }
    if(sndputprop(ofd,"arate",(char*) &props.arate,sizeof(float)) < 0){
        fprintf(stderr,"Failure to write original sample size\n");
    }
    if(sndputprop(ofd,"analwinlen",(char*)  &props.winlen,sizeof(int)) < 0){
        fprintf(stderr,"Failure to write original sample size\n");
    }
    if(sndputprop(ofd,"decfactor",(char*) &props.decfac,sizeof(int)) < 0){
        fprintf(stderr,"Failure to write original sample size\n");
    }
#ifdef _DEBUG
    {
        // just check a couple...
        int got_origsize = 0,got_origrate = 0;
        if(sndgetprop(ofd,"original sampsize",(char*) &got_origsize,sizeof(int)) < 0)
            fprintf(stderr,"failed to recapture orig sampsize\n");
        fprintf(stderr,"got_origsize = %d\n",got_origsize);
        
        if(sndgetprop(ofd,"original sample rate",(char*) &got_origrate,sizeof(int)) < 0)
            fprintf(stderr,"failed to recapture orig samprate\n");
        fprintf(stderr,"got_origrate = %d\n",got_origrate);
    }
#endif

    fprintf(stderr,"pvxtest: calling sndcloseEx(infile) (%d)\n",ifd);
    sndcloseEx(ifd);
    fprintf(stderr,"pvxtest: calling sndcloseEx(outfile) (%d)\n",ofd);
    sndcloseEx(ofd);
    return 0;
}
