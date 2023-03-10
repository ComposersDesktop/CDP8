/*
 * Copyright (c) 1983-2023 Richard Dobson and Composers Desktop Project Ltd
 * http://www.rwdobson.com
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

/* pvplay.cpp
 * Play a soundfile or analysis file using PortAudio
 *
 * Yes yes, I know, this is iffy C++, and there are some gotos to boot. Work in progress...
 *  and I may yet redo the whole thing in plain C anyway, like paplay.
 */ 
/* OCT 2009 rebuilt with updated pvfileio.c for faster performance on G4 */
/* Feb 2010 rebuilt with new stable portaudio */
/* June 2012  new portaudio, settable audio block size */
/* Jan 2013 changed (as also paplay), at last, to use threaded portaudio ringbuffer code (etc) */
/* Jan 2014: completed pvx support */

#include "pvplay.h"

typedef enum  {PLAY_SFILE,PLAY_ANAFILE,PLAY_PVXFILE} PLAYTYPE;
enum {FM_MONO,FM_STEREO,FM_SQUARE,FM_QUAD,FM_PENT,DM_5_0,DM_5_1,FM_HEX,FM_OCT1,FM_OCT2,FM_CUBE,FM_QUADCUBE,FM_NLAYOUTS};

#define NUM_ANALFRAMES (1)
#define RINGBUF_NFRAMES    (32768)
#define NUM_WRITES_PER_BUFFER   (4)

#ifndef min
#define min(x,y)  ((x) < (y) ? (x) : (y))
#endif

#ifdef unix
#include <ctype.h>
int stricmp(const char *a, const char *b);
int strnicmp(const char *a, const char *b, const int length);
#endif

#ifdef WIN32
HANDLE ghEvent;
#endif

int file_playing;
psfdata *g_pdata = NULL;

void playhandler(int sig)
{
    if(sig == SIGINT) {
        if(file_playing) {
            file_playing = 0;
            g_pdata->flag = 1;
            Pa_Sleep(5);
        }
    }
}

#ifdef unix
void alarm_wakeup (int i)
{
    struct itimerval tout_val;
    
    signal(SIGALRM,alarm_wakeup);
    if(file_playing && g_pdata->stream) {
        //printf("%.4f secs\r",(float)(g_pdata->frames_played /(double) g_pdata->srate));
        g_pdata->lastTime = Pa_GetStreamTime(g_pdata->stream ) - g_pdata->startTime;
        printf("%.2f secs\r", g_pdata->lastTime); 
        fflush(stdout);
    }
    tout_val.it_interval.tv_sec = 0;
    tout_val.it_interval.tv_usec = 0;
    tout_val.it_value.tv_sec = 0; 
    tout_val.it_value.tv_usec = 200000; 
    setitimer(ITIMER_REAL, &tout_val,0); 
}
#endif

void finishedCallback(void *userData)
{
    psfdata *pdata = (psfdata*) userData;
    //printf("stream finished!\n");
    pdata->finished = 1;
    file_playing = 0;
}

#ifdef WIN32
VOID CALLBACK TimerCallback(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
    psfdata *pdata = (psfdata*) lpParam;
    
    if(file_playing && pdata->stream) {
        //printf("%.4f secs\r",(float)(g_pdata->frames_played /(double) g_pdata->srate));
        pdata->lastTime = Pa_GetStreamTime(pdata->stream ) - pdata->startTime;
        printf("%.2f secs\r", pdata->lastTime); 
        fflush(stdout);
    }
    SetEvent(ghEvent);
}
#endif

/* This routine will be called by the PortAudio engine when audio is needed.
** It may called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/

#ifdef NOTDEF
// need this?
static void porttimeCallback(PtTimestamp timestamp, void *userData)
{
    PtTimestamp curtime =  Pt_Time();
    double timesecs = (double) curtime * 0.001;
    printf("%.3lf\r",timesecs);
}

#endif

/* Start up a new thread for given function */
static PaError startThread( psfdata* pdata, threadfunc fn )
{
    pdata->flag = 1;
#ifdef WIN32
    pdata->hThread = (void*)_beginthreadex(NULL, 0, fn, pdata, 0, NULL);
    if (pdata->hThread == NULL) 
        return paUnanticipatedHostError;
    /* Wait for thread to startup */
    while (pdata->flag) {
        Pa_Sleep(10);
    }
    /* Set file thread to a little higher priority than normal */
    SetThreadPriority(pdata->hThread, THREAD_PRIORITY_ABOVE_NORMAL);
//#endif
    
#else
#if defined(__APPLE__) || defined(__GNUC__)
    if(pthread_create(&pdata->hThread,NULL,/*(void*)*/ fn,pdata))
        return -1;
    /* Wait for thread to startup */
    while (pdata->flag) {
        Pa_Sleep(10);
    } 
#endif
#endif
    
    return paNoError;
}
#if 0
static int stopThread( psfdata* pdata )
{
    // just called when all data played; must be called before StopStream
    //   pdata->flag = 1;
    /* Wait for thread to stop */
       while (pdata->flag==0) {
           Pa_Sleep(10);
       }
#ifdef WIN32
    CloseHandle(pdata->hThread);
    pdata->hThread = 0;
#else
#if defined(__APPLE__) || defined(__GNUC__)
    pthread_cancel(pdata->hThread);
#endif
#endif
    return paNoError;
}
#endif

static int paplayCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData )
{
    psfdata *data = (psfdata *)userData;
    ring_buffer_size_t sampsAvailable = PaUtil_GetRingBufferReadAvailable(&data->ringbuf) * data->outchans;
    ring_buffer_size_t sampsToPlay = min(sampsAvailable, (ring_buffer_size_t)(framesPerBuffer * data->outchans));
    float *out = (float*) outputBuffer;
    int framesToPlay = sampsToPlay / data->outchans;
    int played;
    
    // if framestoplay < framesPerBuffer, need to clean up the remainder
    memset(out,0,framesPerBuffer * data->outchans * sizeof(float));
    played = PaUtil_ReadRingBuffer(&data->ringbuf, out, framesToPlay);
    
    data->frames_played += played;      
    if(data->flag) {
        //printf("callback - complete\n");
        return paComplete;
    }
    return paContinue;
}

static int MemCallback(const    void *inputBuffer, void *outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags,
                       void *userData )
{
    psfdata *pdata = (psfdata *)userData;
    
    float *out = (float*) outputBuffer;
    unsigned long inSamps,outSamps, inSampPos;
    unsigned long i;
    unsigned long framesToPlay = framesPerBuffer;
    
    // if framesToPlay < framesPerBuffer, need to clean up the remainder
    memset(out,0,framesPerBuffer * pdata->outchans * sizeof(float));
    inSamps = pdata->memFramelength * pdata->inchans;
    outSamps = framesPerBuffer * pdata->inchans;
    inSampPos = pdata->memFramePos * pdata->inchans;
    
    if(pdata->play_looped){
        for(i=0;i < outSamps; i++){
            pdata->inbuf[i] = pdata->membuf[inSampPos++] * pdata->gain;
            if(inSampPos == inSamps)
                inSampPos = 0;
        }
    }
    else {
        for(i=0;i < outSamps; i++){
            pdata->inbuf[i] = pdata->membuf[inSampPos++] * pdata->gain;
            if(inSampPos == inSamps)
                break;
        }
        framesToPlay = i / pdata->inchans;
        for(; i < outSamps;i++){
            pdata->inbuf[i] = 0.0;
        }
        
    }
    // write to output
    // everything now in inbuf
    if(pdata->do_decode) {
        ABFSAMPLE abfsamp;
        unsigned long j;
        for(j=0; j < framesToPlay; j++){
            pdata->copyfunc(&abfsamp,pdata->inbuf + (j * pdata->inchans));
            pdata->decodefunc(&abfsamp,out + (j * pdata->outchans), 1);
        }
    }
    else {  // inchans = outchans
        memcpy(out,pdata->inbuf,framesToPlay * sizeof(float) * pdata->inchans);  
    }
    pdata->memFramePos = inSampPos / pdata->inchans;
    pdata->frames_played += framesToPlay;
    if(framesToPlay < framesPerBuffer)
        pdata->flag = 1;           // end of stream
    if(pdata->flag) {
        return paComplete;
    }
    return paContinue;
}

static unsigned NextPowerOf2(unsigned val)
{
    val--;
    val = (val >> 1) | val;
    val = (val >> 2) | val;
    val = (val >> 4) | val;
    val = (val >> 8) | val;
    val = (val >> 16) | val;
    return ++val;
}

#define N_BFORMATS (10)
// static const int bformats[N_BFORMATS] = {2,3,4,5,6,7,8,9,11,16};
// static const int layout_chans[] = {1,2,4,4,5,5,6,6,8,8,8,8};
/* no M flag here */
enum {FLAG_B = 0, FLAG_BM, FLAG_D, FLAG_G, FLAG_H, FLAG_I, FLAG_L, FLAG_NFLAGS};

void usage(void){
#ifdef WIN32
    printf("usage:\n  pvplay [-BN][-dN][-gN]-hN][-i][-l][-b[N]][-u][-x] infile [from] [to] \n"
#else
    printf("usage:\n  pvplay [-BN][-dN][-gN]-hN][-i][-l][-b[N]][-u] infile [from] [to]\n"
#endif
                  "       -dN  : use output Device N.\n"
                  "       -gN  : apply gain factor N to input.\n"
                  "       -BN  : set memory buffer size to N frames (default: %d).\n"
                  "                N must be a power of 2 (e.g 4096, 8192, 16384 etc).\n"
                  "       -hN  : set hardware blocksize to N frames (32 < N <= BN/4), default is set internally.\n"
                  "               (N recommended to be a power of two size).\n"
                  "            : NB: for sample rates > 48KHz, buffer sizes are doubled internally.\n"
                  "       -i   : play immediately (do not wait for keypress).\n"
                  "       -l   : loop file continuously, from start-time to end-time.\n"
                  "       -u   : suppress elapsed time updates\n"
#ifdef WIN32
                  "       -x   : Apply WAVE_EX infile channel mask to DS audio stream\n"
                  "              (ignored if -m or -b used)\n"
#endif
                  "       from : set start time (secs) for playback and looping. Default: 0.\n"
                  "       to   : set end time (secs) for playback and looping. Default: EOF.\n"
                  "       -b[N]: apply 1st-order B-Format decoding to standard soundfile.\n"
                  "              (file must have at least 3 channels)\n"
                  "               B-Format files (.amb) will be decoded automatically.\n"
                  "               N sets speaker layout to one of the following:\n"
                  "               1    :  *  mono (= W signal only)\n"
                  "               2    :  *  stereo (quasi mid/side, = W +- Y)\n"
                  "               3    :     square\n"
                  "               4    :  *  quad FL,FR,RL,RR order   (default)\n"
                  "               5    :     pentagon\n"
                  "               6    :  *  5.0 surround (WAVEX order)\n"
                  "               7    :  *  5.1 surround (WAVEX order, silent LFE)\n"
                  "               8    :     hexagon\n"
                  "               9    :     octagon 1 (front pair, 45deg)\n"
                  "              10    :     octagon 2 (front centre speaker)\n"
                  "              11    :     cube (as 3, low-high interleaved. Csound-compatible.)\n"
                  "              12    :  *  cube (as 4, low quad followed by high quad).\n"
                  "              Default decode layout is 4 (quad).\n"
                  "              NB: no decoding is performed if -m flag used.\n"
                  "  pvplay reads PEAK chunk if present to rescale over-range floatsam files.\n\n"
                  ,RINGBUF_NFRAMES);
}
           

/*******************************************************************/
int main(int argc,char **argv)
{
    PaStreamParameters outputParameters;
#ifdef WIN32
    /* portaudio sets default channel masks we can't use; we must do all this to set default mask = 0! */
    PaWinDirectSoundStreamInfo directSoundStreamInfo;
//    PaWinMmeStreamInfo winmmeStreamInfo;
#endif
    PaDeviceInfo *devinfo = NULL;
    PaStream *stream = NULL;
    PaStreamCallback *playcallback = paplayCallback;
    PaError err = paNoError;
    SFPROPS props; 
    psfdata sfdata;
    CHPEAK *fpeaks = NULL;
    //ABFSAMPLE *abf_frame = NULL;
    int res,inorder = 1;
    time_t in_peaktime = 0;
    int waitkey = 1;
    //int play_looped = 0;
    double fromdur = 0.0;
    double totime  = 0.0;
    int from_frame = 0, to_frame = 0;
    unsigned int i=0;
    long anal_nframes;
    PaDeviceIndex device;
    //long framesread;
    long filesize;
    //long framesize = 0;
    unsigned long ringframelen = RINGBUF_NFRAMES;   // length of ring buffer in m/c frames
    unsigned long frames_per_buffer = 0;            // default is to let portaudio decide
    unsigned long nFramesToPlay = 0;
    double gain = 1.0;
    unsigned int inchans = 0,outchans = 0;          // may be different if decoding B-Format
    int layout = 4;                                 // default, quad decode */
    fmhcopyfunc copyfunc = NULL;
    fmhdecodefunc decodefunc = NULL;
    /* chorder facility not included in pvplay - use paplay! */
    unsigned int flags[FLAG_NFLAGS] = {0};
#ifdef WIN32
    int speakermask = 0;
    int do_speakermask = 0;
#endif
    int do_updatemessages = 1;
#ifdef unix
    struct itimerval tout_val;    
    tout_val.it_interval.tv_sec = 0;
    tout_val.it_interval.tv_usec = 0;
    tout_val.it_value.tv_sec = 0; 
    tout_val.it_value.tv_usec = 200000;
#endif
    char* ext = 0;
    PVOCDATA *p_pvdata = NULL;
    WAVEFORMATEX *p_wfx = NULL;
    //int pvfile = -1;
    //pvoc_frametype inframetype;
    pvoc_windowtype wtype;
    
    //double oneovrsr;
    int anal_buflen,overlap,winlen;
    // for sfiles only, for now 
    //int framesize_factor = 0;
    //phasevocoder  *pv = NULL, *pv_r = NULL;
    PLAYTYPE playtype = PLAY_SFILE;
    
    
#ifdef WIN32
    HANDLE hTimerQueue;
    ghEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    if(ghEvent==NULL){
        printf("Failed to start internal timer (1).\n");
        return 1;
    }
    hTimerQueue = CreateTimerQueue();
    if(hTimerQueue == NULL){
        printf("Failed to start internal timer (2).\n");
        return 1;
    }
#endif
    
    
    /* CDP version number; now set for release 7 */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("7.1.0\n");
        return 0;
    }
    
    signal(SIGINT,playhandler);
    //sf_frames = FRAMESIZE;  // need this for pvoc playback reporting
    sfdata.inbuf = NULL;
    sfdata.membuf = NULL;
    sfdata.memFramelength = 0;
    sfdata.memFramePos = 0;
    sfdata.current_frame = 0;
    sfdata.ringbufData = NULL;
    sfdata.outbuf_l = NULL;
    sfdata.outbuf_r = NULL;
    sfdata.pvfile = -1;
    
    printf("PvPlay: play multi-channel PCM and analysis files (.ana, .pvx). V7.0.0 RWD,CDP 2014\n");
    file_playing = 0;
    err = Pa_Initialize();
    if( err != paNoError ) {
        fprintf(stderr,"Portaudio startup error.\n");
        return 1;
    }
    device =  Pa_GetDefaultOutputDevice();
    if(device < 0){
        fprintf(stderr,"Cannot find any audio output device\n");
        return 1;
    }

    if(argc < 2){
        usage();
        show_devices();
        Pa_Terminate();
            return 1;
    }

    while(argv[1][0]=='-'){
        unsigned long userbuflen;
        switch(argv[1][1]){
            case('b'):
                if(flags[FLAG_B]){
                    printf("error: multiple -b flags\n");
                    Pa_Terminate();
                    return 1;
                }
                if(argv[1][2] != '\0'){
                    layout = atoi(&argv[1][2]);
                    if(layout < 1 || layout > 12){
                        fprintf(stderr,"value for -b flag out of range.\n");
                        Pa_Terminate();
                        return 1;
                    }
                }
                flags[FLAG_B] = 1;
                break;
            case('d'):
                if(flags[FLAG_D]){
                    printf("error: multiple -d flags\n");
                    Pa_Terminate();
                    return 1;
                }
                if(argv[1][2]=='\0'){
                    printf("-d flag requires parameter\n");
                    Pa_Terminate();
                    return 1;
                }
                device = atoi(&(argv[1][2]));
                flags[FLAG_D] = 1;
                break;
            case('g'):
                if(flags[FLAG_G]){
                    printf("error: multiple -g flags\n");
                    Pa_Terminate();
                    return 1;
                }
                if(argv[1][2]=='\0'){
                    printf("-g flag requires parameter\n");
                    Pa_Terminate();
                    return 1;
                }
                gain = atof(&(argv[1][2]));
                if(gain <= 0.0){
                    fprintf(stderr,"gain value must be positive.\n");
                    Pa_Terminate();
                    return 1;
                }
                flags[FLAG_G] = 1;
                break;
            case 'h':
                if(flags[FLAG_H]){
                    printf("error: multiple -h flags\n");
                    Pa_Terminate();
                    return 1;
                }
                if(argv[1][2]=='\0'){
                    printf("-h flag requires parameter\n");
                    Pa_Terminate();
                    return 1;
                }
                frames_per_buffer = atoi(&(argv[1][2]));
                if((frames_per_buffer > 0) && (frames_per_buffer < 32)){
                    printf("-h value too small: must be at least 32.\n");
                    Pa_Terminate();
                    return 1;
                }
                flags[FLAG_H] = 1;
                break;
            case('i'):
                if(flags[FLAG_I]){
                    printf("error: multiple -i flags\n");
                    Pa_Terminate();
                    return 1;
                }
                flags[FLAG_I] = 1;
                waitkey = 0;
                break;
            case ('l'):
                flags[FLAG_L] = 1;
                break;
            case 'u':
                do_updatemessages = 0;
                break;
#ifdef WIN32
            case 'x':
                do_speakermask = 1;
                break;
#endif
            case 'B':
                if(flags[FLAG_BM]){
                    printf("error: multiple -B flags\n");
                    Pa_Terminate();
                    return 1;
                }
                if(argv[1][2]=='\0'){
                    printf("-B flag requires parameter\n");
                    Pa_Terminate();
                    return 1;
                }
                
                ringframelen = atoi(&(argv[1][2]));
                if(ringframelen <= 1024){
                    printf("-B: framesize value must be >=1024!n");
                    Pa_Terminate();
                    return 1;
                }
                userbuflen = NextPowerOf2(ringframelen);
                if(userbuflen != ringframelen){
                    printf("-B: framesize value must be power of 2 size\n");
                    Pa_Terminate();
                    return 1;
                }
                flags[FLAG_BM] = 1;
                break;
            default:
                printf("unrecognised flag option\n");
                Pa_Terminate();
                return 1;
        }
        argv++;  argc--;
    }
    
    if(frames_per_buffer > ringframelen/4){
        printf(" hardware (-h) framesize %lu too large for file buffer (-B) size %lu\n",
               frames_per_buffer,ringframelen);
        Pa_Terminate();
        return 1;
    }
    
    if(argc < 2 || argc > 4){
        usage();
        show_devices();
        Pa_Terminate();
        return 1;
    }
    
    if(argc>=3){        
        fromdur = atof(argv[2]);
        if(fromdur < 0.0){
            printf("Error: start position must be positive\n");
            Pa_Terminate();
            return 1;
        }
    }
    if(argc==4){
        totime = atof(argv[3]);
        if(totime <= fromdur){
            printf("Error: end time must be after from time\n");
            Pa_Terminate();
            return 1;
        }
    }
    
    if(!init_pvsys()){
        puts("\nUnable to start pvsys.");
        return 1;
    }
    if(sflinit("pvplay")){
        puts("\nUnable to start sfsys.");
        return 1;
    }   
    
    /* get pvocex filetype from extension */
    ext = strrchr(argv[1],'.');   
    if(ext && stricmp(ext,".pvx")==0){
        p_pvdata =  new PVOCDATA;
        if(p_pvdata==NULL){
            puts("\nNo Memory!");
            return 1;
        }
        p_wfx =  new WAVEFORMATEX;
        if(p_wfx==NULL){
            puts("\nNo Memory!");
            return 1;
        }

        sfdata.pvfile   = pvoc_openfile(argv[1],p_pvdata,p_wfx);
        if(sfdata.pvfile< 0){
            fprintf(stderr,"\npvplay: unable to open pvx file: %s",pvoc_errorstr());
            return 1;
        }
        props.srate = p_wfx->nSamplesPerSec;
        if(props.srate <=0){
            fprintf(stderr,"\nbad srate found: corrupted file?\n");
            return 1;
        }
        sfdata.srate = props.srate;
        /*stick to mono/stereo for now */
        if(p_wfx->nChannels > 2){
            fprintf(stderr,"\nSorry: can only read mono or stereo pvx files.");
            return 1;
        }
        inchans = p_wfx->nChannels;
        props.chans = inchans;
        outchans = inchans;
        /* calc size in m/c analysis frames of this analysis file */
        filesize = pvoc_framecount(sfdata.pvfile) / inchans;
        double dur = (double) filesize  / (double)(p_pvdata->fAnalysisRate);
        filesize = (long)(dur * props.srate);

        sfdata.anal_framesize = (p_pvdata->nAnalysisBins/*- 1*/) * 2; 
        sfdata.fftsize = sfdata.anal_framesize-2;
        winlen = p_pvdata->dwWinlen;
        overlap = p_pvdata->dwOverlap;
        sfdata.inframetype = (pvoc_frametype) p_pvdata->wAnalFormat;
        
//        oneovrsr = 1.0 / (double) props.srate;
        wtype = (pvoc_windowtype) p_pvdata->wWindowType;        
        
        /* adjust buffer size to multiple of overlap   */
        anal_buflen = sfdata.anal_framesize * NUM_ANALFRAMES;
        anal_buflen = (anal_buflen / overlap) * overlap;
        /* mono */
        sfdata.analframe1 = new float[sfdata.anal_framesize];
        memset(sfdata.analframe1,0,sfdata.anal_framesize * sizeof(float));

        sfdata.pv_l = new phasevocoder();
        if(!sfdata.pv_l->init(props.srate,sfdata.anal_framesize-2,winlen,overlap,1.0,PVOC_S_NONE,wtype,PVPP_STREAMING)){
            fprintf(stderr,"\nUnable to initialize pvoc Channel 1.");
            //delete sfdata.pv_l;
            //delete sfdata.analframe1;
            return 1;
        }
        if(inchans==2){
            /* need two intermediate buffers, to be interleaved into sampbuf */
            sfdata.analframe2 = new float[sfdata.anal_framesize];
            memset(sfdata.analframe2,0,sfdata.anal_framesize * sizeof(float));
            sfdata.pv_r = new phasevocoder();
            if(!sfdata.pv_r->init(props.srate,sfdata.anal_framesize-2,winlen,overlap,1.0,PVOC_S_NONE,wtype,PVPP_STREAMING)){
                fprintf(stderr,"\nUnable to initialize pvoc Channel 2.");
                //delete sfdata.pv_l;
                //delete sfdata.pv_r;
                //delete sfdata.outbuf_l;
                //delete sfdata.outbuf_r;
                //delete sfdata.analframe1;
                //delete sfdata.analframe2;
                return 1;
            }
        }
        to_frame = (pvoc_framecount(sfdata.pvfile) / inchans) - 1;
        if(fromdur > 0.0){
            //printf("SORRY: from parameter ignored: not yet supported for pvx files\n");
            from_frame = (long)(fromdur * (props.srate/overlap));
            if(from_frame >= to_frame){
                printf("Error: start frame %d is beyond end of file\n", from_frame);
                goto error;
            }
            if(pvoc_seek_mcframe(sfdata.pvfile,from_frame,SEEK_SET)){
                printf("Error: from: failed to seek to frame %d\n",from_frame);
                goto error;
            }
        } 
                
        if(totime > 0.0){
            long targetframe = (long)(totime * (props.srate/overlap));
            if(targetframe >= to_frame){
                printf("Error: end time is beyond end of file\n");
                goto error;
            }
                    
            if(targetframe <= from_frame){
                printf("Start time must be less than end time.\n");
                goto error;
            }
            to_frame = targetframe;       
            if(to_frame - from_frame < 1){
                to_frame = from_frame+1;   // maybe we can in fact freeze on a single frame!
            // but we will not use in-memory system in this case
            }
            nFramesToPlay = to_frame - from_frame;
                    
            printf("Playing %d analysis frames\n",(int) nFramesToPlay);
        } 
        playtype = PLAY_PVXFILE; 
    }

    /* soundfile or anafile*/

    else {
        /* allow auto rescaling of overrange floats via PEAK chunk */
        sfdata.ifd = sndopenEx(argv[1],1,CDP_OPEN_RDONLY);
        if(sfdata.ifd < 0) {
            printf("unable to open soundfile %s : %s\n",argv[1],sferrstr());
           return 1;
        }
        if(!snd_headread(sfdata.ifd,&props)) {
            fprintf(stderr,"\nError reading sfile header");
            fprintf(stderr,"\n%s",props_errstr);
            return 1;
        }
        inchans = props.chans;
        filesize = sndsizeEx(sfdata.ifd);  // get size in frames
        if(filesize == 0){
            printf("File is empty!\n");
            return 1;
        }
        
        bool adjusted = false; // in case we have to increase ringframelen for huge FFT size
        switch(props.type){
            case wt_analysis:
                sfdata.anal_framesize = props.chans;
                outchans = inchans = props.chans = 1;
                props.srate = props.origrate;
                anal_nframes =  filesize / sfdata.anal_framesize ;   // * chans ?
                if(anal_nframes <= 1){
                    fprintf(stderr,"Bad analysis file - not enough frames\n");
                    return 1;
                }
                sfdata.analframe1  = new float[sfdata.anal_framesize];                                      
                winlen = props.winlen;
                /* TODO: match ringbuf length to multiple of FFT size */
                sfdata.fftsize = sfdata.anal_framesize - 2;
                        
                sfdata.pv_l = new phasevocoder;
                if(!sfdata.pv_l->init(props.origrate,sfdata.anal_framesize-2,props.winlen,props.decfac,1.0,
                        PVOC_S_NONE,PVOC_HAMMING,PVPP_STREAMING)){
                    fprintf(stderr,"Error: unable to initialize pvoc engine.\n");           
                    return 1;
                }
                overlap = sfdata.pv_l->anal_overlap();
                //anal_buflen = /*FRAMESIZE * NUM_ANALFRAMES*/ anal_framesize;  // always mono
                //anal_buflen = (anal_buflen / overlap) * overlap;
                // buflen = anal_buflen;
                from_frame = (long)(fromdur * props.arate);
                if(from_frame >= anal_nframes){
                    printf("Error: start is beyond end of file\n");
                    goto error;
                }
                
                to_frame = anal_nframes -1;
                
                if(totime > 0.0){
                    to_frame = (long)(totime * props.arate + 0.5);
                    if(to_frame >= anal_nframes){
                        printf("Error: end time is beyond end of file\n");
                        goto error;
                    }
                    
                    if(to_frame <= from_frame){
                        printf("Start time must be less than end time.\n");
                        goto error;
                    }
                    
                    if(to_frame - from_frame < 1){
                        to_frame = from_frame +1;   // maybe we can in fact freeze on a single frame!
                        // but we will not use in-memory system in this case
                    }
                    nFramesToPlay = to_frame - from_frame;
                    
                    printf("Playing %d analysis frames\n",(int) nFramesToPlay);
                }
               if(from_frame > 0){
                    if(sndseekEx(sfdata.ifd,from_frame * sfdata.anal_framesize,SEEK_SET) < 0)   {
                        printf("Error setting start position to frame %d\n", from_frame);
                        printf("%s", sferrstr());
                        goto error;
                    }
                    sfdata.current_frame = from_frame;
                }
                // ensure ring buffer is large enough even for huge FFT lengths!
                adjusted = false;
                while(ringframelen < (unsigned long)(sfdata.fftsize * 4)) {
                    ringframelen <<= 1;
                    adjusted = true;
                }
                    
                //printf("from = %d, to = %d\n",from_frame, to_frame);
                playtype = PLAY_ANAFILE; 
                printf("Opened %s: %ld frames, %d channels\n",argv[1],anal_nframes,props.chans);
                if(adjusted)
                    printf("FFT frame size = %d, expanding buffer to suit...\n", sfdata.fftsize);
                break;
            case wt_wave:
                playtype = PLAY_SFILE;
                printf("Opened %s: %ld frames, %d channels\n",argv[1],filesize,props.chans);
                //framesize_factor = (long) (48000/ props.srate);
                //framesize_factor += 1;
                filesize /= inchans;
#ifdef WIN32
                if(props.format == WAVE_EX){
                    if(do_speakermask){
                        int mask;
                        if(flags[FLAG_B]){
                            printf("-x flag ignored if -B used\n");
                        }
                        else {
                            mask = sndgetchanmask(sfdata.ifd);
                            if(mask < 0){
                                printf("Could not read speaker mask. Using 0\n");
                            }
                            else 
                                speakermask = mask;
                        }
                    }
                }
#endif
                from_frame = (long)(fromdur * props.srate);
                if(from_frame >= filesize){
                    printf("Error: start is beyond end of file\n");
                    goto error;
                }
                nFramesToPlay = filesize - from_frame;
                to_frame = filesize -1;
                if(totime > 0.0){
                    to_frame = (long)(totime * props.srate);
                    if(to_frame >= filesize){
                        printf("Error: end time is beyond end of file\n");
                        goto error;
                    }
                    //printf("fromframe = %d, toframe = %d\n",(int)from_frame,(int) to_frame);
                    if(to_frame <= from_frame){
                        printf("End time must be later than from time\n");
                        goto error;
                    }
                    nFramesToPlay = to_frame - from_frame;
                    printf("Playing %d frames\n",(int) nFramesToPlay);
                }
                if(from_frame > 0){
                    if(sndseekEx(sfdata.ifd,from_frame * inchans,SEEK_SET) < 0) {
                        printf("Error setting start position\n");
                        goto error;
                    }
                    sfdata.current_frame = from_frame;
                }
                
                
                outchans = inchans = props.chans;
                if(props.chformat==MC_BFMT){
                    flags[FLAG_B] = 1;
                    if(inchans > 4) {
                        inorder = 2;
                        printf("%u-channel input: performing 2nd-order decode.\n",inchans);
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
                        fprintf(stderr,"Unsupported channel count (%u) for B-format file.\n",inchans);
                        return 1;
                    }
                    
                    switch(layout-1) {
                    case FM_MONO:
                        printf("Decoding to Mono\n");
                        decodefunc = fm_i1_mono;
                        outchans = 1;
                        break;
                    case FM_STEREO:
                        printf("Decoding to Stereo\n");
                        decodefunc = fm_i1_stereo;
                        outchans = 2;
                        break;
                    case FM_SQUARE:
                        printf("Decoding to Square\n");
                        if(inorder == 1)
                            decodefunc = fm_i1_square;
                        else
                            decodefunc = fm_i2_square;
                        outchans = 4;
                        break;
                    case FM_PENT:
                        printf("Decoding to pentagon\n");
                        if(inorder==1)
                            decodefunc = fm_i1_pent;
                        else
                            decodefunc = fm_i2_pent;
                        outchans = 5;
                        break;
                    case DM_5_0:
                        printf("Decoding to 5.0 surround (David Moore)\n");
                        if(inorder==1)
                            decodefunc = dm_i1_surr;
                        else
                            decodefunc = dm_i2_surr;
                        outchans = 5;
                        break;
                    case DM_5_1:
                        printf("Decoding to  5.1 surround (David Moore)\n");
                        if(inorder==1)
                            decodefunc = dm_i1_surr6;
                        else
                            decodefunc = dm_i2_surr6;
                        outchans = 6;
                        break;
                    case FM_HEX:
                        printf("Decoding to Hexagon\n");
                        if(inorder==1)
                            decodefunc = fm_i1_hex;
                        else
                            decodefunc = fm_i2_hex;
                        outchans = 6;
                        break;
                    case FM_OCT1:
                        printf("Decoding to Octagon 1\n");
                        if(inorder==1)
                            decodefunc = fm_i1_oct1;
                        else
                            decodefunc = fm_i2_oct1;
                        outchans = 8;
                        break;
                    case FM_OCT2:
                        printf("Decoding to Octagon 2\n");
                        if(inorder==1)
                            decodefunc = fm_i1_oct2;
                        else
                            decodefunc = fm_i2_oct2;
                        outchans = 8;
                        break;
                    case FM_CUBE:
                        printf("Decoding to Cube (FM interleaved)\n");
                        if(inorder==1)
                            decodefunc = fm_i1_cube;
                        else
                            decodefunc = fm_i2_cube;
                        outchans = 8;
                        break; 
                    case FM_QUADCUBE:
                        printf("Decoding to Octagon 1 (WAVEX order)\n");
                        if(inorder==1)
                            decodefunc = fm_i1_cubex;
                        else
                            decodefunc = fm_i2_cubex;
                        outchans = 8;
                        break;
                    default:
                        //quad
                        printf("Decoding to quad surround (WAVEX order)\n");
                        if(inorder == 1)
                            decodefunc = fm_i1_quad;
                        else
                            decodefunc = fm_i2_quad;
                        outchans = 4;
                        break;
                    }
                }  // MC_BFMT            
                
                /* just read peaks for soundfile */
                fpeaks = (CHPEAK *) calloc(inchans,sizeof(CHPEAK));
                if(fpeaks==NULL){
                    puts("No memory for PEAK data\n");
                    goto error;
                }
                int thispeaktime;
                res = sndreadpeaks(sfdata.ifd,inchans,fpeaks, &thispeaktime);
                in_peaktime = (time_t) thispeaktime;
                if(res==0){
                    printf("No PEAK data in this file\n");
                }
                if(res > 0) {
                    printf("PEAK data:\ncreation time: %s",ctime( &in_peaktime));
                    
                    for(i=0;i < inchans;i++){
                        printf("CH %d: %.4f at frame %u: \t%.4f secs\n",
                               i,fpeaks[i].value,fpeaks[i].position,(double)(fpeaks[i].position / (double) props.srate));
                    }
                }        
                break;
            default:
                fprintf(stderr,"\nSorry: Pvplay can only play soundfiles and analysis files.");
                return 1;
        }
    }

    /* memory playback mode? */
    if(playtype == PLAY_SFILE && nFramesToPlay <= ringframelen){
        sfdata.membuf =  (float *) PaUtil_AllocateMemory(nFramesToPlay * sizeof(float) * /*inchans*/ outchans);
        if( sfdata.membuf == NULL )   {
            puts("Could not allocate memory play buffer.\n");
            goto error;
        }
        sfdata.memFramelength = nFramesToPlay;
        sfdata.memFramePos = 0;
        playcallback = MemCallback;
        printf("RAM block size =  %lu\n",nFramesToPlay);
                    
    }
    else {
        // set up pcm ring buffer  NB must be power of 2 size
        if(props.srate > 48000)
            ringframelen <<= 1;
        printf("File buffer size set to %ld sample frames\n",ringframelen);
        // NB ring buffer sized for decoded data, hence outchans here; otherwise inchans = outchans
        sfdata.ringbufData = (float *) PaUtil_AllocateMemory( ringframelen * sizeof(float) * outchans); /* From now on, recordedSamples is initialised. */
        if( sfdata.ringbufData == NULL )   {
            puts("Could not allocate play buffer.\n");
            goto error;
        }                 
        // number of elements has to be a power of two, so each element has to be a full m/c frame
        if (PaUtil_InitializeRingBuffer(&sfdata.ringbuf, sizeof(float) * outchans, ringframelen , sfdata.ringbufData) < 0) {
            puts("Could not initialise play buffer.\n");
            goto error;
        }            
    } 
    
    
    /* create input side workspace buffer, used for both soundfiles and analysis files! */
    sfdata.inbuf = (float *) PaUtil_AllocateMemory(ringframelen * sizeof(float) * inchans);
    if(sfdata.inbuf==NULL){
        puts("No memory for read buffer\n");
        goto error;
    }
    sfdata.inbuflen = ringframelen;
    
    if(playtype == PLAY_PVXFILE && (inchans==2)){
        sfdata.outbuf_l = (float *) PaUtil_AllocateMemory(ringframelen * sizeof(float) );
        sfdata.outbuf_r = (float *) PaUtil_AllocateMemory(ringframelen * sizeof(float) );
        
    }
    
    if(props.srate > 48000)
        frames_per_buffer *= 2;
    if(frames_per_buffer > 0)
        printf("Audio buffer size = %lu\n",frames_per_buffer);
    
    sfdata.inchans      = inchans;
    sfdata.outchans     = outchans;
    sfdata.frames_played  = 0;
    sfdata.gain         = gain;
    sfdata.copyfunc     = copyfunc;
    sfdata.decodefunc   = decodefunc;
    sfdata.do_decode    = flags[FLAG_B];
    sfdata.from_frame   = from_frame;  // interpreted according to Play Type
    sfdata.to_frame     = to_frame;
    sfdata.play_looped  = flags[FLAG_L];
    sfdata.srate = props.srate;
    sfdata.finished = 0;
    g_pdata = &sfdata;
    
    outputParameters.device = device;   /*Pa_GetDefaultOutputDevice(); */ /* default output device */
    outputParameters.channelCount = outchans;                     
    outputParameters.sampleFormat = paFloat32;             /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    
    devinfo = (PaDeviceInfo *) Pa_GetDeviceInfo(device);
#if defined MAC || defined unix
    if(devinfo){
        printf("Using device %d: %s\n",device,devinfo->name);
    }
#endif
    
#ifdef WIN32
    
# ifdef NOTDEF
    if(devinfo){
        if(apiinfo->type  == paMME ){
            winmmeStreamInfo.size = sizeof(winmmeStreamInfo);
            winmmeStreamInfo.hostApiType = paMME; 
            winmmeStreamInfo.version = 1;
            winmmeStreamInfo.flags = paWinMmeUseChannelMask;
            winmmeStreamInfo.channelMask = 0;
            outputParameters.hostApiSpecificStreamInfo = &winmmeStreamInfo; 
#  ifdef _DEBUG
            printf("WIN DEBUG: WinMME device channel mask set to 0.\n");
#  endif
        }
    }
# endif
    /* change val to 1 if also using MME */
    if(devinfo){
        int apitype = devinfo->hostApi; // get index of api type
        const PaHostApiInfo *apiinfo =  Pa_GetHostApiInfo( apitype );
        printf("Using device %d: %s:",device,devinfo->name);
        if(apiinfo->type  == paDirectSound) {
            printf ("(DS)\n");
            /* set this IF we are using Dsound device. */
            directSoundStreamInfo.size = sizeof(PaWinDirectSoundStreamInfo);
            directSoundStreamInfo.hostApiType = paDirectSound; 
            directSoundStreamInfo.version = 2;
            directSoundStreamInfo.flags = paWinDirectSoundUseChannelMask;
            directSoundStreamInfo.channelMask = speakermask;
            outputParameters.hostApiSpecificStreamInfo = &directSoundStreamInfo;
        }
        else if(apiinfo->type == paASIO)
            printf("(ASIO)\n");
        // else
        //    printf("API unknown!);
    }
#endif    
    
    
    err = Pa_OpenStream(
            &stream,
            NULL,  /* No input */
            &outputParameters, /* As above. */
            props.srate,
            frames_per_buffer,          /* Frames per buffer. */
            paClipOff,      /* we won't output out of range samples so don't bother clipping them */
            playcallback,
            &sfdata );
    
    if( err != paNoError ){
        fprintf(stderr,"Unable to open audio device: err = %d\n", err);   
        exit(1);
    }
    err =  Pa_SetStreamFinishedCallback( stream, finishedCallback );
    if( err != paNoError ) {
        printf("Unable to set finish callback\n");
        goto error;
    }
    sfdata.stream = stream;
    file_playing = 1;

    if(waitkey){
        printf("Press any key to start:\n");
        printf("Hit CTRL-C to stop.\n");
        fflush(stdout);
        while (!kbhit()){   
            if(!file_playing)    //check for instant CTRL-C
                goto error;     
            };
#ifdef _WIN32
        if(kbhit())
            _getch();            //prevent display of char
#endif
    }

    // should this go in read thread func too?
    
    
#ifdef NOTDEF
    // TODO: any benefits in using this?
    pterror = Pt_Start(200, porttimeCallback, NULL);
    if(pterror != ptNoError){
        printf("porttime timer failed to initialise!\n");
        return 1;
    }
#endif
    
    /* if small block, read it all into memory
     NB not doing paplay channel mapping  */
    if(sfdata.memFramelength > 0){
        //if(psf_sndReadFloatFrames(sfdata.ifd,sfdata.membuf,sfdata.memFramelength) 
        if(fgetfbufEx(sfdata.membuf,sfdata.memFramelength*sfdata.inchans,sfdata.ifd,0)
           != (int) sfdata.memFramelength*sfdata.inchans) {
            printf("Error reading soundfile into memory\n");
            goto error;
        }
    }
    
    // set up timer
#ifdef unix
    if(do_updatemessages) {
        setitimer(ITIMER_REAL, &tout_val,0);
        signal(SIGALRM,alarm_wakeup); /* set the Alarm signal capture */
    }
#endif
//#ifdef _WIN32
    // not sure of the best position for this
    //if(!CreateTimerQueueTimer(&sfdata.hTimer, hTimerQueue,
    //    (WAITORTIMERCALLBACK) TimerCallback, &sfdata,250,250,0)) {
    //    printf("failed to start timer (3).\n");
    //    return 1;
    // }
//#endif
    /* Start the file reading thread */
    
    sfdata.startTime = Pa_GetStreamTime(stream );
    
    switch(playtype){
        case PLAY_SFILE:    
            if(sfdata.memFramelength == 0){
                err = startThread(&sfdata, threadFunctionReadFromRawFile);
                if( err != paNoError ) goto error;
            }
            else {
                sfdata.flag = 0;
            }
            break;
        case PLAY_ANAFILE:
            err = startThread(&sfdata, threadFunctionReadFromAnalFile);
            if( err != paNoError ) goto error;
            break;
        case PLAY_PVXFILE:
            err = startThread(&sfdata, threadFunctionReadFromPVXFile);
            if( err != paNoError ) goto error;
            break;
        default:
            printf("Internal error: no playback file type!\n");
            return 1;
            
    }
    
#ifdef _WIN32
    if(do_updatemessages){
        if(!CreateTimerQueueTimer(&sfdata.hTimer, hTimerQueue,
                                  (WAITORTIMERCALLBACK) TimerCallback, &sfdata,200,200,0)) {
            printf("failed to start timer (3).\n");
            return 1;
        }
    }
#endif
    err = Pa_StartStream( stream );
    if( err != paNoError ) {
        fprintf(stderr,"Unable to start playback.\n");
        goto error;
    }
    
    while((!sfdata.finished) && file_playing){
        // nothing to do!
        Pa_Sleep(20);
    }
    // note to the curious: any bug in audio buffer arithmetic will likely cause crash here!
    err = Pa_StopStream( stream );
    if( err != paNoError ) {
        printf("Error stopping stream\n");
        goto error;
    }
    /*  read thread should exit always, no need to Stop it here */
    
    err = Pa_CloseStream( stream ); 
    if( err != paNoError ) {
        printf("Error closing stream\n");
        goto error;
    }
    printf("%.2f secs\n",(float)(sfdata.lastTime));
    fflush(stdout);
    printf("Playback finished.\n");
error:
    Pa_Terminate();
#ifdef _WIN32
    CloseHandle(ghEvent);
    DeleteTimerQueue(hTimerQueue);
#endif
    if( sfdata.ringbufData )       
        PaUtil_FreeMemory(sfdata.ringbufData);
    
    if(sfdata.inbuf)
        PaUtil_FreeMemory(sfdata.inbuf);
    
    if(sfdata.membuf)
        PaUtil_FreeMemory(sfdata.membuf);
    
    if(sfdata.outbuf_l)
        PaUtil_FreeMemory(sfdata.outbuf_l);
    
    if(sfdata.outbuf_r)
        PaUtil_FreeMemory(sfdata.outbuf_r); 
    
    if(sfdata.ifd >= 0)
        sndcloseEx(sfdata.ifd);
    
    if(sfdata.pvfile >= 0)
        pvoc_closefile(sfdata.pvfile);
        
    if(fpeaks)
        free(fpeaks);
    
//    sffinish();
    
    pvsys_release();
    
    if(err != paNoError){
        fprintf( stderr, "An error occured while using the portaudio stream\n" ); 
        fprintf( stderr, "Error number: %d\n", err );
        fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
        return err;
    }
    return 0;
}

int show_devices(void)
{
//      int i,j;
        PaDeviceIndex numDevices,p;
        const    PaDeviceInfo *pdi;
#ifdef _WIN32
        const PaHostApiInfo* api_info;
#endif
        PaError  err;
        int nOutputDevices = 0;
        
#ifdef USE_ASIO
        printf("For ASIO multi-channel, you may need to select the highest device no.\n");      
#endif
        /*Pa_Initialize();*/
        numDevices =  Pa_GetDeviceCount();
        if( numDevices < 0 )
        {
            printf("ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
            err = numDevices;
            return err;
        }
#ifdef WIN32
        printf("Driver\tDevice\tInput\tOutput\tName\n");
#else
        printf("Device\tInput\tOutput\tName\n");
#endif
        //printf("Number of devices = %d\n", numDevices );
        for( p=0; p<numDevices; p++ )
        {
            pdi = Pa_GetDeviceInfo( p );
            nOutputDevices++;
            
            if( p == Pa_GetDefaultOutputDevice() ) 
                printf("*");
            else
                printf(" ");
#ifdef WIN32
            api_info = Pa_GetHostApiInfo(pdi->hostApi);
            const char *apiname = api_info->name;
            if(strcmp(apiname,"Windows DirectSound")==0)
                apiname = "DS ";
            printf("(%s)\t%d\t%d\t%d\t%s\n",apiname,p,
                pdi->maxInputChannels,
                pdi->maxOutputChannels,
                pdi->name); 
#else
            printf("%d\t%d\t%d\t%s\n",p,
                pdi->maxInputChannels,
                pdi->maxOutputChannels,
                pdi->name);


#endif
        }
        /*Pa_Terminate();*/
        return 0;
}

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

int
strnicmp(const char *a, const char *b, const int length)
{
    int len = length;
    
    while(*a != '\0' && *b != '\0') {
        int ca = islower(*a) ? toupper(*a) : *a;
        int cb = islower(*b) ? toupper(*b) : *b;
        
        if(len-- < 1)
            return 0;
        
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



