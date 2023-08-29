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
 
/* sfprops.c: display primary soundfile properties 
 * uses sfsysEx.lib: recognizes WAVE-EX formats
 * part of the CDP MCTOOLS suite
 */
//RWD.6.99 use new funcs to report 24bit formats, etc
//last update: 22.6.99 enumerate PEAK chans from 1, link with latest sfsysEx
/* Nov28 2001: rebuild with sfsysEx recognizing stupid QuickTime AIFC floats with 16bit size! */
/*Dec 2005 support .amb extension */
/*April 2006: build with updated sfsys to read PEAK chunk after data chunk (Thanks to Sony!)*/
/* OCT 2009 TODO: sort out 64bit platform issues (peaktime etc) */
/* FEB 2010: decl filesize etc as unsigned to read huge file sizes! */
/* Nov 2013 added MC_SURR_6_1 */
/* March 2023: add flags for plain numeric output, as helper for TV scripts */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <sfsys.h>
#include <chanmask.h>

#ifdef unix

int stricmp(const char *a, const char *b)
{
    while(*a != '\0' && *b != '\0') {
        int ca = islower(*a) ? toupper(*a) : *a;
        int cb = islower(*b) ? toupper(*b) : *b;
        
        if(ca < cb)
            return -1;
        if(ca > cb)
            return 1;
        
        a++;
        b++;
    }
    if(*a == '\0' && *b == '\0')
        return 0;
    if(*a != '\0')
        return 1;
    return -1;
}
#endif

typedef struct wave_ex_speaker {
    int mask;
    char name[28];
} WAVE_EX_SPEAKER;


static const WAVE_EX_SPEAKER speakers[NUM_SPEAKER_POSITIONS] = {
    {SPEAKER_FRONT_LEFT,"Front Left"},
    {SPEAKER_FRONT_RIGHT,"Front Right"},
    {SPEAKER_FRONT_CENTER,"Front Centre"},
    {SPEAKER_LOW_FREQUENCY,"Low Frequency"},
    {SPEAKER_BACK_LEFT,"Back Left"},
    {SPEAKER_BACK_RIGHT,"Back Right"},
    {SPEAKER_FRONT_LEFT_OF_CENTER,"Front Centre-Left"},
    {SPEAKER_FRONT_RIGHT_OF_CENTER,"Front Centre-Right"},
    {SPEAKER_BACK_CENTER,"Back Centre"},
    {SPEAKER_SIDE_LEFT,"Side Left"},
    {SPEAKER_SIDE_RIGHT,"Side Right"},
    {SPEAKER_TOP_CENTER,"Top Centre"},
    {SPEAKER_TOP_FRONT_LEFT,"Front Top Left"},
    {SPEAKER_TOP_FRONT_CENTER,"Front Top Centre"},
    {SPEAKER_TOP_FRONT_RIGHT,"Front Top Right"},
    {SPEAKER_TOP_BACK_LEFT,"Back Top Left"},
    {SPEAKER_TOP_BACK_CENTER,"Back Top Centre"},
    {SPEAKER_TOP_BACK_RIGHT,"Back Top Right"}
};

void usage(){
    fprintf(stderr,"CDP MCTOOLS: SFPROPS v2.2.1 (c) RWD,CDP,1999,2009,2010,2013,2023\n"
                    "Display soundfile details, with WAVE-EX speaker positions\n"
                    "\n\tusage: sfprops [-c | -d | -r] infile\n");
    fprintf(stderr,"\tflag options for single data report\n");
    fprintf(stderr,"\t-c: print number of channels in file\n");
    fprintf(stderr,"\t-d: print duration (secs) of file\n");
    fprintf(stderr,"\t-r: print sample rate of file\n");
}

 

int main(int argc, char **argv)
{

    int i,ifd;
    unsigned int nframes,filesize; /* FEB 2010 */
    double srate;
    SFPROPS props = {0};
    const char *name = NULL;
    CHPEAK *peaks = NULL;
    int res,peaktime,speakermask = 0;
    float fmaxamp;
    int lmaxamp;
    unsigned int do_singleflag = 0;
    unsigned int wantsr = 0, wantdur = 0,wantchans = 0;
    
    if(argc< 2){
        usage();
        exit(1);
    }
    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("2.2.1.\n");
        return 0;
    }
    //RWD new, for TV scripting
    if(argc>2 && argv[1][0] == '-'){
        unsigned char uiflag = 0;
        if(argv[1][1] == '\0'){
            fprintf(stderr,"no flag option specified\n");
            return 0;
        }
        else {
            do_singleflag = 1;
            uiflag = argv[1][1];
            switch(uiflag){
                case 'c':
                    wantchans = 1;
                    break;
                case 'r':
                    wantsr = 1;
                    break;
                case 'd':
                    wantdur = 1;
                    break;
                default:
                    fprintf(stderr,"unrecognised flag %s\n",argv[1]);
                    return 0;
                    break;
            }
        }
        argv++;
        argc--;
    }

    if(sflinit("sfprops")){
        if(do_singleflag)
            printf("0\n");
        else
            fprintf(stderr,"SFPROPS: unable to initialize CDP Sound Filing System\n");
        exit(1);
    }
    if((ifd = sndopenEx(argv[1],0,CDP_OPEN_RDONLY)) < 0){
        if(do_singleflag)
            printf("0\n");
        else
            fprintf(stderr,"SFPROPS: unable to open infile %s: %s\n",argv[1], sferrstr());
        exit(1);
    }

    if(!snd_headread(ifd,&props)){
        if(do_singleflag)
            printf("0\n");
        else
            fprintf(stderr,"SFPROPS: unable to read infile header\n");
        exit(1);
    }

    srate = (double) props.srate;
    if(do_singleflag && wantsr){
        printf("%u\n",props.srate);
        sndcloseEx(ifd);
        return 0;
    }
    filesize = sndsizeEx(ifd);
    nframes = filesize/ props.chans;        /*assume soundfile for now */
    if(do_singleflag && wantdur){
        float dur = nframes / srate;
        printf("%f\n",dur);
        sndcloseEx(ifd);
        return 0;
    }
    if(do_singleflag && wantchans){
        printf("%u\n",props.chans);
        sndcloseEx(ifd);
        return 0;
    }
    name = snd_getfilename(ifd);
    printf("Properties of %s:\n",name);
    printf("File type:  ");
    switch(props.type){
    case(wt_wave):
        printf("soundfile\nFormat         :  ");

        switch(props.format){
        case(WAVE):
            printf("Standard WAVE format\n");
            break;
        case(WAVE_EX):
            printf("MS WAVE-FORMAT-EXTENSIBLE\n");
            printf("SPEAKER CONFIGURATION:\t");

            switch(props.chformat){
            //case(MC_STD):
            //  printf("unassigned (generic format)");
            //  break;
            case(MC_MONO):
                printf("Mono\n");
                break;
            case(MC_STEREO):
                printf("Stereo\n");
                break;
            case(MC_QUAD):
                printf("Rectangular Quad\n");
                break;
            case(MC_LCRS):
                printf("Quad Surround\n");
                break;
            case(MC_BFMT):
                printf("Ambisonic B-Format\n");
                break;
            case(MC_DOLBY_5_1):
                printf("5.1 (Dolby) Surround\n");
                break;
            case(MC_SURR_6_1):
                printf("6.1 Surround\n");
                break;
            case(MC_SURR_7_1):
                printf("7.1 Surround\n");
                break;
            case(MC_CUBE):
                printf("Cube Surround\n");
                break;
            default:
                printf("Special speaker assignments\n");
                break;
            }
            speakermask = sndgetchanmask(ifd);
            if(speakermask < 0)
                fprintf(stderr,"Unable to read speakermask from WAVE_EX header\n");
            else {
                //try to figure out the mask
                int assigned = 0;
                int shift = 0;
                int this_channel = 1;
                printf("Speaker Mask = %d (0x%x)\n",speakermask,(unsigned int) speakermask);
                while(assigned < props.chans){
                    if(speakers[shift].mask & speakermask){
                        printf("Channel %d: %s\n",this_channel++,speakers[shift].name);
                        assigned++;
                    }
                    if(++shift == NUM_SPEAKER_POSITIONS)
                        break;
                }
                if(assigned < props.chans)
                    printf("Remaining channels not assigned speaker locations.\n");

            }
            break;
                
        case(AIFF):
            printf("AIFF format\n");
            break;
        case(AIFC):
            printf("AIFC format\n");
            break;
        default:
            printf("unknown format\n");
            break;
        }
        printf("Sample Rate    :  %d\n",props.srate);
        printf("Channels       :  %d\n",props.chans);
        printf("Sample Frames  :  %d\n",nframes);
        printf("sample type:   :  ");
        switch(props.samptype){
        case(SHORT8):
            printf("8-bit\n");
            break;
        case(SHORT16):
            printf("16-bit\n");
            break;
        case(FLOAT32):
            printf("32bit floats\n");
            break;
        case(INT_32):
            printf("32bit (integer)\n");
            break;
        case(INT2424):
            printf("24bit (packed)\n");
            break;
        case(INT2432):
            printf("24bit in 32bit words\n");
            break;
        case(INT2024):
            printf("20bit in 24bit words\n");
            break;
        case(INT_MASKED):
            printf("non-standard WAVE_EX format\n");
            //fet more info....
            break;
        default:
            printf("sorry: don't recognize word format!\n");
            break;

        }
        printf("duration       :  %.4lf secs\n",(double)nframes / srate);
        //this is the problem: we never decided what type maxamp is, officially!
        if(props.samptype==FLOAT32){
            if(sndgetprop(ifd,"maxamp",(char *)&fmaxamp,sizeof(float)) == sizeof(float)){
                printf("CDP maxamp     :  %.4lf\n",fmaxamp);    
            }
        }
        else{
            if(sndgetprop(ifd,"maxamp",(char *)&lmaxamp,sizeof(int)) == sizeof(int)){
                printf("CDP maxamp     :  %d\n",lmaxamp);   
            }
        }
        peaks = (CHPEAK *) calloc(props.chans,sizeof(CHPEAK));
        if(peaks==NULL){
            puts("sfprops: no memory for fpeak data buffer\n");
            exit(1);
        }
        else{
            time_t thistime;
            res = sndreadpeaks(ifd,props.chans,peaks, (int *) &peaktime);
            thistime = (time_t) peaktime;
            if(res ==0)
                printf("no peak data in this infile\n");
            else if(res < 0){
                fprintf(stderr,"sfprops: WARNING: error reading infile peak data\n");
                
            }
            else {      
                printf("PEAK data:\n\tcreation time: %s\n", ctime(&thistime));
                //for(i=0;i < props.chans; i++)
                //  printf("CH %d: %.4lf at frame %d:\t%.4lf secs\n",
                //  i+1,peaks[i].value,peaks[i].position,(double) (peaks[i].position) / srate);
                for(i=0; i < props.chans; i++){
                    double val, dbval;
                    val = (double) peaks[i].value;
                    
                    if(val > 0.0){
                        dbval = 20.0 * log10(val);
                        printf("CH %d: %.6f (%.2lfdB) at frame %9d:\t%.4f secs\n",i,
                               val,dbval,peaks[i].position,(double)(peaks[i].position / (double) srate));
                    }
                    else{
                        printf("CH %d: %.6f (-infdB)  \t\t:\t%.4f secs\n",
                               i,
                               val, (double)(peaks[i].position / (double) srate)); 
                    }
                }
            }
        }
        
        break;
    case(wt_analysis):
        printf("CDP pvoc analysis file.\n");
        printf("Channel Format:       Amplitude,Frequency\n");
        printf("Orig rate:            %d\n",props.origrate);
        printf("Analysis Window Size: %d\n",props.winlen);
        printf("Analysis channels:    %d\n",props.chans/2);
        printf("Window Overlap:       %d\n",props.decfac);
        printf("Analysis rate:        %.4f\n",props.arate);
        printf("Frame count:          %d\n",filesize / (props.chans));
        printf("Data size (floats):   %d\n",filesize);
        printf("Duration (secs):      %.3lf\n",
            ((filesize / props.chans) * props.decfac) / (double) props.origrate);
        break;
    case(wt_formant):
        printf("CDP formant data\n");
        printf("Specenvcnt:           %d\n",props.specenvcnt);
        printf("Analysis Window Size: %d\n",props.winlen);
        printf("Channels:             %d\n",props.chans);
        printf("Frame count:          %d\n",filesize);
        printf("Orig rate:            %d\n",props.origrate);        
        printf("Window Overlap:       %d\n",props.decfac);
        printf("Analysis rate:        %.4f\n",props.arate);
        printf("Orig Chans:          %d\n",props.origchans);        
        printf("Duration (secs):      %.3lf\n",(filesize /props.specenvcnt)/ props.arate );
        break;
    case(wt_transposition):
        printf("CDP transposition data\n");
        printf("Orig sample rate:     %d\n",props.origrate);
        printf("Analysis Window Size: %d\n",props.winlen);
        printf("Channels:             %d\n",props.chans);
        printf("Window Overlap:       %d\n",props.decfac);
        printf("Analysis rate:        %.4f\n",props.arate);
        printf("Orig Chans:          %d\n",props.origchans);
        printf("Data size (floats):   %d\n",filesize);
        printf("Duration (secs):      %.3lf\n",
            ((filesize / props.chans) * props.decfac) / (double) props.origrate);
        break;
    case(wt_pitch):
        printf("CDP pitch data\n");
        printf("Orig sample rate:     %d\n",props.origrate);
        printf("Analysis Window Size: %d\n",props.winlen);
        printf("Channels:             %d\n",props.chans);
        printf("Window Overlap:       %d\n",props.decfac);
        printf("Analysis rate:        %.4f\n",props.arate);
        printf("Orig Chans:          %d\n",props.origchans);
        printf("Data size (floats):   %d\n",filesize);
        printf("Duration (secs):      %.3lf\n",
            ((filesize / props.chans) * props.decfac) / (double) props.origrate);
        break;
    case(wt_binenv):
        printf("CDP binary envelope data\n");
        printf("envelope window size: %f msecs\n",props.window_size);
        printf("sample rate:          %d\n",props.srate);
        printf("Channels:             %d\n",props.chans);
        printf("Points:               %d\n",filesize);
        printf("Duration (secs):      %.3f\n",filesize * (props.window_size * 0.001));
        break;
    default:
        printf("internal error! unlisted soundfile type\n");
        break;
    }

    sndcloseEx(ifd);
//  sffinish();
    return 0;
}



