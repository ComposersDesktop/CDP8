/*
 * Copyright (c) 1983-2023 Richard Dobson and Composers Desktop Project Ltd
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
 
//fmdcode.c  : decode .amb file to various speaker layouts
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <portsf.h>
#include "fmdcode.h"

#ifdef unix
/* in portsf.lib */
extern int stricmp(const char *a, const char *b);
#endif

/*
 Channel order is WXYZ,RSTUV,KLMNOPQ
 
 The number of channels defines the order of the soundfield:
 2 channel = W+Y =  "Mid-Side" 
 3 channel = h   = 1st order horizontal
 4 channel = f   = 1st order 3-D
 5 channel = hh  = 2nd order horizontal
 6 channel = fh  = 2nd order horizontal + 1st order height (formerly
                                                            called 2.5 order)
 7 channel = hhh = 3rd order horizontal
 8 channel = fhh = 3rd order horizontal + 1st order height
 9 channel = ff  = 2nd order 3-D
 11 channel = ffh = 3rd order horizontal + 2nd order height
 16 channel = fff = 3rd order 3-D
 
 
 Horizontal   Height  Soundfield   Number of    Channels
 order        order       type      channels    
 1           0         horizontal     3         WXY
 1           1        full-sphere     4         WXYZ
 2           0         horizontal     5         WXY....UV
 2           1         mixed-order    6         WXYZ...UV
 2           2        full-sphere     9         WXYZRSTUV
 3           0         horizontal     7         WXY....UV.....PQ
 3           1         mixed-order    8         WXYZ...UV.....PQ
 3           2         mixed-order   11         WXYZRSTUV.....PQ
 3           3        full-sphere    16         WXYZRSTUVKLMNOPQ

 */


enum {ARG_PROGNAME, ARG_INFILE,ARG_OUTFILE, ARG_LAYOUT,ARG_NARGS};
#define N_BFORMATS (10)
enum {FM_MONO,FM_STEREO,FM_SQUARE,FM_QUAD,FM_PENT,DM_5_0,DM_5_1,FM_HEX,FM_OCT1,FM_OCT2,FM_CUBE,FM_QUADCUBE,FM_NLAYOUTS};


//static const int bformats[N_BFORMATS] = {2,3,4,5,6,7,8,9,11,16};
static const int layout_chans[] = {1,2,4,4,5,5,6,6,8,8,8,8};

void usage(void)
{
   printf(
    "usage: fmdcode [-x][-w] infile outfile layout\n"
    "       -w    :   write plain WAVE outfile format\n"
    "                (.wav default - use generic wavex format).\n"
    "       -x    : write std WAVEX speaker positions to header\n"
    "               (applies to compatible layouts only; requires .wav extension).\n"
    "   layout    : one of the choices below.\n"
    "       Output channel order is anticlockwise from centre front\n"
    "            except where indicated.\n"
    "   Layouts indicated with * are compatible with WAVEX speaker position order. \n"
    "   Available speaker layouts:\n"
    "   1    :  *  mono (= W signal only)\n"
    "   2    :  *  stereo (quasi mid/side, = W +- Y)\n"
    "   3    :     square\n"
    "   4    :  *  quad FL,FR,RL,RR order\n"
    "   5    :     pentagon\n"
    "   6    :  *  5.0 surround (WAVEX order)\n"
    "   7    :  *  5.1 surround (WAVEX order, silent LFE)\n"
    "   8    :     hexagon\n"
    "   9    :     octagon 1 (front pair, 45deg)\n"
    "   10   :     octagon 2 (front centre speaker)\n"
    "   11   :     cube (as 3, low-high interleaved. Csound-compatible.)\n"
    "   12   :  *  cube (as 4, low quad followed by high quad).\n"
    " NOTE: no shelf filters or NF compensation used.\n");
}

int main(int argc,char *argv[])
{
    int i,ifd, ofd;
    int layout,inorder = 1;
    int got,halfsec;
    unsigned int framepos;
    int inchans,outchans;
    int outsize;
    int write_speakerpositions = 0;
    MYLONG peaktime;
    psf_channelformat chformat = MC_STD;
    psf_format  outformat;
    char *sfname;
    float *frame = NULL;
    fmhcopyfunc copyfunc;
    fmhdecodefunc decodefunc = NULL;
    int write_wavex = 1;
    ABFSAMPLE abfsample;
    PSF_PROPS props;
    PSF_CHPEAK *peaks = NULL;
    float abfframe[16];

    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("1.0b\n");
        return 0;
    }
    
    if(argc < 3){
        printf("\nCDP MCTOOLS: FMDCODE v 1.0beta: RWD,CDP 2009\n"
                "Plain multi-layout decoder for .amb files.\n"
                "Regular layouts use standard Furse-Malham in-phase coefficients.\n" 
                "5.x surround coefficients (maxre) from David Moore.\n");
        usage();
        return 1;
    }
    while(argv[1][0] =='-'){        
        switch(argv[1][1]){
        case('w'):
            write_wavex = 0;
            break;
        case 'x':
            write_speakerpositions = 1;
            break;
        default:
            fprintf(stderr,"fmdcode: error: illegal flag option %s\n",argv[1]);
            return 1;
        }
        argc--; argv++;
    }
    if(argc < ARG_NARGS){
        
        usage();
        return 1;
    }
    if(psf_init()) {        
        printf("failed to init psfsys\n");
        exit(1);
    }
    
    sfname = argv[ARG_OUTFILE];
    layout = atoi(argv[ARG_LAYOUT]);
    if(layout < 1 || layout > FM_NLAYOUTS+1){
        printf("Unsupported layout type.\n");
        return 1;
    }
    
    
    ifd = psf_sndOpen(argv[ARG_INFILE],&props,0);
    if(ifd < 0){
        fprintf(stderr,"unable toopen infile %s\n",argv[ARG_INFILE]);
        return 1;
    }
    inchans = props.chans;
    if(inchans > 4) {
        inorder = 2;
        printf("%d-channel input: performing 2nd-order decode.\n",inchans);
    }
    outsize = psf_sndSize(ifd);
    if(outsize <= 0){
        fprintf(stderr,"fmdcode: infile is empty!\n");
        psf_sndClose(ifd);
        return 1;
    }
    
    switch(inchans){
    case 3:
        copyfunc = fmhcopy_3;
        break;
    case 4:
        copyfunc = fmhcopy_4;       
        break;
    case 5:
        copyfunc = fmhcopy_5;
        break;
    case 6:
        copyfunc = fmhcopy_6;
        break;
    case 7:
        copyfunc = fmhcopy_7;
        break;
    case 8:
        copyfunc = fmhcopy_8;
        break;
    case 9:
        copyfunc = fmhcopy_9;
        break;
    case 11:
        copyfunc = fmhcopy_11;
        break;
    case 16:
        copyfunc = fmhcopy_16;
        break;
    default:
        printf("file has unsupported number of channels (%d)\n",inchans);
        psf_sndClose(ifd);
        return 1;
    }
    //FM_MONO,FM_STEREO,FM_SQUARE,FM_PENT,FM_SURR,FM_SURR6,FM_HEX,FM_OCT1,FM_OCT2,FM_CUBE
    switch(layout-1){
        case FM_MONO:
            printf("Decoding to Mono\n");
            decodefunc = fm_i1_mono;
            if(write_wavex && write_speakerpositions)
                chformat = MC_MONO;
            break;
        case FM_STEREO:
            printf("Decoding to Stereo\n");
            decodefunc = fm_i1_stereo;
            if(write_wavex && write_speakerpositions)
                chformat = MC_STEREO;
            break;
        case FM_SQUARE:
            printf("Decoding to Square\n");
            if(inorder == 1)
               decodefunc = fm_i1_square;
            else
               decodefunc = fm_i2_square; 
            break;
        case FM_QUAD:
            printf("Decoding to quad surround (WAVEX order)\n");
            if(inorder == 1)
                decodefunc = fm_i1_quad;
            else
                decodefunc = fm_i2_quad;
            if(write_wavex && write_speakerpositions)
                chformat = MC_QUAD;
            break;
        case FM_PENT:
            printf("Decoding to pentagon\n");
            if(inorder==1)
                decodefunc = fm_i1_pent;
            else
                decodefunc = fm_i2_pent;
            break;

        case DM_5_0:
            printf("Decoding to 5.0 surround (David Moore)\n");
            if(inorder==1)
                decodefunc = dm_i1_surr;
            else
                decodefunc = dm_i2_surr;
            if(write_wavex && write_speakerpositions)
                chformat = MC_SURR_5_0;
            break;
        case DM_5_1:
            printf("Decoding to  5.1 surround (David Moore)\n");
            if(inorder==1)
                decodefunc = dm_i1_surr6;
            else
                decodefunc = dm_i2_surr6;
            if(write_wavex && write_speakerpositions)
                chformat = MC_DOLBY_5_1;
            break;
        case FM_HEX:
            printf("Decoding to Hexagon\n");
            if(inorder==1)
                decodefunc = fm_i1_hex;
            else
                decodefunc = fm_i2_hex;
            break;
        case FM_OCT1:
            printf("Decoding to Octagon 1\n");
            if(inorder==1)
                decodefunc = fm_i1_oct1;
            else
                decodefunc = fm_i2_oct1;
            break;
        case FM_OCT2:
            printf("Decoding to Octagon 2\n");
            if(inorder==1)
                decodefunc = fm_i1_oct2;
            else
                decodefunc = fm_i2_oct2;
            break;
        case FM_CUBE:
            printf("Decoding to Cube (FM interleaved)\n");
            if(inorder==1)
                decodefunc = fm_i1_cube;
            else
                decodefunc = fm_i2_cube;
            break; 
        case FM_QUADCUBE:
            printf("Decoding to Octagon 1 (WAVEX order)\n");
            if(inorder==1)
                decodefunc = fm_i1_cubex;
            else
                decodefunc = fm_i2_cubex;
            if(write_wavex && write_speakerpositions)
                chformat = MC_CUBE;
            break;
    }
    outformat = psf_getFormatExt(sfname);
    if(outformat >= PSF_AIFF){
        if(write_speakerpositions)
            printf("Warning: -x requires .wav format\n");
    }
    outchans = layout_chans[layout-1];
    frame = malloc(sizeof(float) * outchans);
    if(frame==NULL){
        puts("No Memory!\n");
        return 1;
    }
    props.chformat = STDWAVE;
    props.chans = outchans;
    if(!is_legalsize(outsize,&props)){
        fprintf(stderr,"error: outfile size exceeds capacity of format.\n");
        return 1;
    }
    
    /*TODO: set speaker pos when we can */
    if(write_wavex){
        props.chformat = chformat;
        props.format = PSF_WAVE_EX;
    }

    ofd = psf_sndCreate(sfname,&props,0,0,PSF_CREATE_RDWR);
    if(ofd < 0){
        fprintf(stderr,"can't create outfile %s\n",sfname);
        psf_sndClose(ifd);
        return 1;
    }
    peaks = (PSF_CHPEAK*)  malloc(sizeof(PSF_CHPEAK) * outchans);
    memset(peaks,0,sizeof(PSF_CHPEAK) * outchans);

    halfsec = props.srate / 2;
    framepos = 0;
    printf("\ndecoding:\n");
    while((got = psf_sndReadFloatFrames(ifd,abfframe,1))==1){
        memset(&abfsample,0,sizeof(ABFSAMPLE));
        copyfunc(&abfsample,abfframe);
        decodefunc(&abfsample,frame,1);            
        if(0 > psf_sndWriteFloatFrames(ofd,frame,1)){
            fprintf(stderr,"error writing to outfile\n");
            psf_sndClose(ifd);
            psf_sndClose(ofd);
            return 1;
        }

        if((framepos % halfsec) == 0){
            printf("%.2lf secs\r",(double) framepos / (double) props.srate);
            fflush(stdout);
        }
        framepos++;
    }

    if(got != 0){
        fprintf(stderr,"warning: not all data was read\n");
    }
    printf("\n%.4lf secs\nWritten %d frames to %s\n",(double)framepos / (double) props.srate,framepos,sfname);
    
    if(psf_sndReadPeaks( ofd,peaks,&peaktime)){
        printf("PEAK values:\n");
        for(i=0; i < outchans; i++){
            double val, dbval;
            val = (double) peaks[i].val;
            
            if(val > 0.0){
                dbval = 20.0 * log10(val);
                printf("CH %d: %.6f (%.2lfdB) at frame %u:\t%.4f secs\n",i,
                       val,dbval,(unsigned int)peaks[i].pos,(double)peaks[i].pos / (double) props.srate);
            }
            else{
                printf("CH %d: %.6f (-infdB) at frame %u:\t%.4f secs\n",i,
                       val,(unsigned int)peaks[i].pos,(double)peaks[i].pos / (double) props.srate); 
            }
        }
    }
    printf("\n");
    psf_sndClose(ifd);
    psf_sndClose(ofd);
    if(peaks){
        free(peaks);
    }
    psf_finish();
    return 0;
}

