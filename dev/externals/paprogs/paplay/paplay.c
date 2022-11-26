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

/* paplay.c
 * Play a soundfile using the Portable Audio api for Max OS X using Core Audio.
 *
 • new version using threads and ring buffer, with thanks to Robert Bielik
*/

/* Dec 2005 support 3ch B-Format, b-format files via portsf */
/* OCT 2009 much revised, extended b-format decoding options */
/*     set ch mask to zero on Windows mme/ds.
       TODO: decide whether/how to enable user to pass chmask of file to driver!
*/
/* Apr 2011 build with new portaudio stable v 19 */
/* July 2012 added "to" option, memory playback mode */
/* August 2012  restored -x flag */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <time.h>

#ifdef _WIN32
#define _WIN32_WINNT 0x0500
#include <conio.h>
#include <windows.h>
#include <process.h>
// NB: need to have PAWIN_USE_WDMKS_DEVICE_INFO defined
#include "pa_win_ds.h"
#endif

#ifdef unix

#include <sys/types.h>
#include <sys/timeb.h>
#endif

#include <signal.h>

#ifdef unix
#include <aaio.h>
#endif

#ifdef MAC
#include <libkern/OSAtomic.h>
#endif

#ifdef unix
#include <sys/time.h>
#include <pthread.h>
/* in portsf.lib */
extern int stricmp(const char *a, const char *b);
#endif

#include "portaudio.h"
#include "pa_ringbuffer.h"
#include "pa_util.h"
#include "portsf.h"
#include "fmdcode.h"



#ifndef min
#define min(x,y)  ((x) < (y) ? (x) : (y))
#endif

// TODO: scale by sample rate


#define FRAMES_PER_BUFFER (4096)
#define RINGBUF_NFRAMES    (32768)
#define NUM_WRITES_PER_BUFFER   (4)


enum {FM_MONO,FM_STEREO,FM_SQUARE,FM_QUAD,FM_PENT,DM_5_0,DM_5_1,FM_HEX,FM_OCT1,FM_OCT2,FM_CUBE,FM_QUADCUBE,FM_NLAYOUTS};

#ifdef _WIN32
HANDLE ghEvent;
#endif


typedef struct {
    PaUtilRingBuffer ringbuf;
    PaStream *stream;
    PaTime startTime;
    PaTime lastTime;
    float *ringbufData;
    float *inbuf;   //for decoding and other tx; used as temp working buffer for read thread func.
    /* for memory read func */
    float *membuf;
    float** orderptr; // for chan mapping;
#ifdef _WIN32
    void *hThread;
    HANDLE hTimer;
    HANDLE hTimerCount;
#else
#ifdef unix
    pthread_t hThread;
#endif
#endif
    fmhdecodefunc decodefunc;
    fmhcopyfunc copyfunc;
    double gain;
    unsigned long frames_played;
    unsigned long current_frame;
    unsigned long from_frame;
    unsigned long to_frame;
    unsigned long memFramelength;
    unsigned long memFramePos;
    unsigned long inbuflen;
    int srate;
    int inchans;
    int outchans;
    int flag;
    int ifd;
#ifdef DEBUG
    int ofd;
#endif
    int do_decode;
    int do_mapping;
    int play_looped;
    int finished;
} psfdata;

static int file_playing;
static psfdata *g_pdata = NULL;  // for timer interrupt routine

void playhandler(int sig)
{
    if(sig == SIGINT)
        file_playing = 0;
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
    //    printf("stream finished!\n");
    pdata->finished = 1;
    file_playing = 0;


}


#ifdef _WIN32

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
/* thread func for file playback */
/* writes decoded data to ring buffer */
/* size of element is full m/c frame */

// TODO: unix thread should return void*
#ifdef unix
static  int  threadFunctionReadFromRawFile(void* ptr)
#else
    static unsigned int __stdcall threadFunctionReadFromRawFile(void* ptr)
#endif
{
    psfdata* pdata = (psfdata*)ptr;

    /* Mark thread started */
    pdata->flag = 0;

    while (1) {
        ring_buffer_size_t nElementsProcessed = 0;
        ring_buffer_size_t elementsInBuffer = PaUtil_GetRingBufferWriteAvailable(&pdata->ringbuf);

        //memset(pdata->inbuf,0,pdata->inbuflen * sizeof(float) * pdata->inchans);

        if (elementsInBuffer >= pdata->ringbuf.bufferSize / 4) {
            void* ptr[2] = {NULL};
            ring_buffer_size_t sizes[2] = {0};

            /* By using PaUtil_GetRingBufferWriteRegions, we can write directly into the ring buffer */
            PaUtil_GetRingBufferWriteRegions(&pdata->ringbuf, elementsInBuffer, &ptr[0], &sizes[0], &ptr[1], &sizes[1]);

            if (file_playing)  {
                ring_buffer_size_t itemsReadFromFile;
                int i,j;
                int framesread = 0;
                // we work with frames, = constant across inchans and outchans
                itemsReadFromFile = 0;
                for(i = 0; i < 2 && (ptr[i] != NULL); i++) {
                    // NB ringbuf is sized by m/c frames
                    int frameswanted = sizes[i];

                    pdata->current_frame = psf_sndTell(pdata->ifd);
                    // read up to end frame if requested
                    if(pdata->to_frame < pdata->current_frame + frameswanted)
                        frameswanted = pdata->to_frame - pdata->current_frame;

                    if(frameswanted > 0){
                        framesread = psf_sndReadFloatFrames(pdata->ifd,pdata->inbuf,frameswanted);
                        if(framesread < 0){ // read error!
                            printf("Error reading soundfile\n");
                            pdata->flag = 1;
                            file_playing = 0;
                            break;  // just out of for loop - need to return instead?
                        }
                    }
                    if(framesread == 0){
                        /* EOF. EITHER: finish, or rewind if looping playback*/
                        if(pdata->play_looped){
                            if(psf_sndSeek(pdata->ifd,pdata->from_frame,PSF_SEEK_SET)){
                                printf("Error looping soundfile\n");
                                pdata->flag = 1;
                                file_playing = 0;
                                break;
                            }
                            // sizes[1] especially may well = 0
                            if(frameswanted==0)
                                break;
                            framesread = psf_sndReadFloatFrames(pdata->ifd,pdata->inbuf,frameswanted);
                            if(framesread < 0){ // read error!
                                printf("Error reading soundfile\n");
                                pdata->flag = 1;
                                file_playing = 0;
                                break;
                            }
                        }
                        else {
                            // we must watch the ring buffer to make sure all data has been rendered,
                            // over several callback blocks
                            //printf("End of data. playing = %d:\n", file_playing);
                            //printf("\telements remaining = %d\n",elementsInBuffer);
                            if(elementsInBuffer == pdata->ringbuf.bufferSize)  {
                                pdata->flag = 1;
                                break;
                                //return 0;
                            }
                        }
                    }
                    else {
                        // ringbuf calcs always relative to outchans
                        itemsReadFromFile += framesread;

                        // now ready to apply decoding or other processing

                        if(pdata->gain != 1.0){
                            for(j = 0; j < framesread * pdata->inchans; j++)
                                pdata->inbuf[j]  = (float)(pdata->inbuf[j] * pdata->gain);
                        }
                        if(pdata->do_mapping) {
                            int k;
                            float val;
                            float *buf = ptr[i];
                            float *pchan;
                            for(j=0; j < framesread; j++){
                                for(k=0; k < pdata->outchans; k++){
                                    pchan = pdata->orderptr[k];
                                    if(pchan == NULL)
                                        val = 0.0f;
                                    else
                                        val = *(pchan + j * pdata->inchans);
                                    buf[j*pdata->outchans + k] = val;
                                }
                            }
                        }
                        else if(pdata->do_decode) {
                            ABFSAMPLE abfsamp;

                            for(j=0; j < framesread; j++){
                                // single frame only
                                pdata->copyfunc(&abfsamp,pdata->inbuf + (j * pdata->inchans));
                                /* BIG TODO: update funcs to process large frame buffer! */
                                /* NB: ring buffer is effectively defined as raw bytes */
#ifdef unix
                                pdata->decodefunc(&abfsamp,ptr[i] + (j * pdata->outchans * sizeof(float)), 1);
#else
                                // suppress VC++ complaint of unknown size
                                pdata->decodefunc(&abfsamp, (float*)(ptr[i]) + (j * pdata->outchans), 1);

#endif
                                nElementsProcessed++;
                            }
                        }
                        else {  // inchans = outchans
                            memcpy(ptr[i],pdata->inbuf,framesread * sizeof(float) * pdata->inchans);
                            nElementsProcessed += framesread;
                        }
                    }
                }
                //                assert(nElementsProcessed == itemsReadFromFile);
                if(framesread)
                    PaUtil_AdvanceRingBufferWriteIndex(&pdata->ringbuf, itemsReadFromFile);


            }
            else {
                // this code is activated on Ctrl-C. Can do immediate finish by setting flag
                //               printf("file done\n");
                pdata->flag = 1;
                break;
            }
        }
        //  else {
        //      printf("ringbuf size = %d, elements remaining = %d, playing = %d\n",pdata->ringbuf.bufferSize,elementsInBuffer,file_playing);
        //  }

        /* Sleep a little while... */
        Pa_Sleep(10);
    }
    return 0;
}



#ifdef unix
// TODO: unix return type should be void*
typedef int (*threadfunc)(void*);
#endif
#ifdef _WIN32
typedef unsigned int (__stdcall *threadfunc)(void*);
#endif


/* Start up a new thread for given function */
static PaError startThread( psfdata* pdata, threadfunc fn )
{
    pdata->flag = 1;
#ifdef _WIN32
    pdata->hThread = (void*)_beginthreadex(NULL, 0, fn, pdata, 0, NULL);
    if (pdata->hThread == NULL)
        return paUnanticipatedHostError;
    /* Wait for thread to startup */
    while (pdata->flag) {
        Pa_Sleep(10);
    }
    /* Set file thread to a little higher prio than normal */
    SetThreadPriority(pdata->hThread, THREAD_PRIORITY_ABOVE_NORMAL);
#else

#if defined(__APPLE__) || defined(__GNUC__)
    if(pthread_create(&pdata->hThread,NULL,(void*) fn,pdata))
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
    // RWD: just called when all data played; must be called before StopStream
    //   pdata->flag = 1;
    /* Wait for thread to stop */
    //   while (pdata->flag) {
    //       Pa_Sleep(10);
    //   }
#ifdef _WIN32
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
    int framesToPlay = sampsToPlay/data->outchans;
    int played;
    /* outbuf may be NULL on initial startup */
    if(out == NULL)
        return paContinue;

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

    /* outbuf may be NULL on initial startup */
    if(out == NULL)
        return paContinue;

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
        int j;
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
        //printf("\nnewcallback - complete\n");
        //fflush(stdout);
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


int show_devices(void);

#define N_BFORMATS (10)
//static const int bformats[N_BFORMATS] = {2,3,4,5,6,7,8,9,11,16};
//static const int layout_chans[] = {1,2,4,4,5,5,6,6,8,8,8,8};

enum {FLAG_B = 0, FLAG_BM, FLAG_D, FLAG_G, FLAG_H, FLAG_I, FLAG_L, FLAG_M, FLAG_NFLAGS};


void usage(void){
#ifdef WIN32
    printf("usage:\n  paplay [-BN][-dN][-gN]-hN][-i][-l][-b[N]|-m[S]][-u][-x] infile [from] [to] \n"
#else
           printf("usage:\n  paplay [-BN][-dN][-gN]-hN][-i][-l][-b[N]|-m[S]][-u] infile [from] [to]\n"
#endif
                  "       -dN  : use output Device N.\n"
                  "       -gN  : apply gain factor N to input.\n"
                  "       -BN  : set memory buffer size to N frames (default: %d).\n"
                  "                N must be a power of 2 (e.g 4096, 8192, 16384 etc).\n"
                  "       -hN  : set hardware blocksize to N frames (32 < N <= BN/4).\n"
                  "               (N recommended to be a power of two size. Default: from soundcard).\n"
                  "               Where set, buffer sizes are doubled internally for sample rates > 48KHz.\n"
                  "       -i   : play immediately (do not wait for keypress).\n"
                  "       -l   : loop file continuously, from start-time to end-time.\n"
                  "       -m[S]: render using channel map string S.\n"
                  "                Use -m without parameter for usage.\n"
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
                  "paplay reads PEAK chunk if present to rescale over-range floatsam files.\n\n"
                  ,RINGBUF_NFRAMES);
           }

        void mapusage(void)
        {
            printf( "  PAPLAY channel map mode -mS: Map arbitrary infile channels to output channels\n\n"
                    "  Order string S = any combination of characters a-z inclusive.\n"
                    "  Infile channels are mapped in order as a=1,b=2...z=26\n"
                    "  (For example: channels in a 4-channel file are represented by the\n"
                    "    characters abcd; any other character is an error).\n"
                    "  Characters must be lower case, and may be used more than once.\n"
                    "  Duplicate characters duplicate the corresponding input channel.\n"
                    "  The zero character (0) may be used to set a silent channel.\n"
                    "  A maximum of 26 channels is supported for both input and output.\n"
                    "  The number of characters in S sets the number of output channels,\n"
                    "    which must be supported by the selected device.\n"
                    "  NB: -m cannot be combined with -b (B-Format decoding).\n"
                    "  If channel mapping is used, a B-format file (AMB) \n"
                    "    will be rendered without decoding.\n"
                    );
        }

    /*******************************************************************/
    int main(int argc,char **argv)
    {
        PaStreamParameters outputParameters;
#ifdef _WIN32
        /* portaudio sets default channel masks we can't use; we must do all this to set default mask = 0! */
        PaWinDirectSoundStreamInfo directSoundStreamInfo;
        //    PaWinMmeStreamInfo winmmeStreamInfo;

#endif
        PaDeviceInfo *devinfo = NULL;
        PaStream *stream = NULL;
        PaStreamCallback *playcallback = paplayCallback;
        PaError err = paNoError;
        psfdata sfdata;
        PSF_CHPEAK *fpeaks = NULL;
        PSF_PROPS props;
        // ABFSAMPLE abfsample;
        // ABFSAMPLE *abf_frame = NULL;
        int res,inorder = 1;
        time_t in_peaktime = 0;
        MYLONG lpeaktime;
        int waitkey = 1;
        //int play_looped = 0;
        double fromdur = 0.0;
        double totime  = 0.0;
        int ifd = -1,from_frame = 0, to_frame = 0;
        unsigned int i=0;
        PaDeviceIndex device;
        long filesize;
        //long framesize = 0;   // Apr 2010 to be scaled by sample rate
        unsigned long ringframelen = RINGBUF_NFRAMES;  // length of ring buffer in m/c frames
        unsigned long frames_per_buffer = 0;
        unsigned long nFramesToPlay = 0;
        double gain = 1.0;
        unsigned int inchans,outchans;
        int layout = 4; // default, quad decode */
        fmhcopyfunc copyfunc = NULL;
        fmhdecodefunc decodefunc = NULL;
        // vars for chorder facilit
        char* argstring = NULL;
        unsigned int rootchar = 'a';
        unsigned int maxchar = 'z';
        unsigned int  nchars=0,nzeros = 0;
        unsigned int max_inchar = 0;
        unsigned int flags[FLAG_NFLAGS] = {0};
#ifdef WIN32
        int do_speakermask = 0;
        int speakermask = 0;
#endif
        int do_updatemessages = 1;
#ifdef unix
        struct itimerval tout_val;
        tout_val.it_interval.tv_sec = 0;
        tout_val.it_interval.tv_usec = 0;
        tout_val.it_value.tv_sec = 0;
        tout_val.it_value.tv_usec = 200000;
#endif

#ifdef _WIN32
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
        /* CDP version number */
        if(argc==2 && (stricmp(argv[1],"--version")==0)){
            printf("3.0.1\n");
            return 0;
        }

        signal(SIGINT,playhandler);

        sfdata.inbuf = NULL;
        sfdata.membuf = NULL;
        sfdata.memFramelength = 0;
        sfdata.memFramePos = 0;
        sfdata.ringbufData = NULL;
        sfdata.orderptr = NULL;
        sfdata.do_mapping = 0;

        printf("PAPLAY: play multi-channel soundfile. V3.0.1 (c)  RWD,CDP 2012, 2013\n");
        file_playing = 0;
        err = Pa_Initialize();
        if( err != paNoError ) {
            printf("Failed to initialize Portaudio.\n");
            return 1;
        }
        device =  Pa_GetDefaultOutputDevice();

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
            case 'm':
                if(flags[FLAG_M]){
                    printf("error: multiple -m flags\n");
                    Pa_Terminate();
                    return 1;
                }
                if(argv[1][2]=='\0'){
                    mapusage();
                    Pa_Terminate();
                    return 1;
                }
                argstring = &argv[1][2];
                nchars = strlen(argstring);
                if(nchars > 26) {
                    printf("error: -m order list too long.\n");
                    Pa_Terminate();
                    return 1;
                }
                if(nchars < 1){
                    printf("error: -m must specify at least one channel.\n");
                    Pa_Terminate();
                    return 1;

                }
                flags[FLAG_M] = 1;
                break;

                // RWD Feb 2011: need to support dummy -u flag for compatibility with pvplay
                // will we actually need to implement it?
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
        // this just cheks flag usage. We render amb files undecoded, if using channel mapping
        if(flags[FLAG_B] && flags[FLAG_M]){
            printf("Sorry: cannot use -m with -b\n");
            Pa_Terminate();
            return 1;
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
        if(psf_init()){
            printf("Error initializing psfsys\n");
            goto error;
        }
        /* allow auto rescaling of overrange floats via PEAK chunk */
        ifd = psf_sndOpen(argv[1],&props,1);
        if(ifd < 0){
            printf("unable to open soundfile %s\n",argv[1]);
            goto error;
        }
        filesize = psf_sndSize(ifd);
        if(filesize ==0){
            printf("Soundfile is empty!\n");
            goto error;
        }

#if WIN32
        if(props.format == PSF_WAVE_EX){
            if(do_speakermask){
                if(flags[FLAG_B] || flags[FLAG_M]){
                    printf("-x flag ignored if -B or -m used\n");
                }
                else {
                    int mask = psf_speakermask(ifd);
                    if(mask < 0){
                        printf("could not read speaker mask. Using 0\n");
                        speakermask = 0;
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
                printf("end time must be later than from time\n");
                goto error;
            }
            nFramesToPlay = to_frame - from_frame;
            //printf("playing %d frames\n",(int) nFramesToPlay);
        }

        outchans = inchans = props.chans;
        /* if using order string, outchans may change arbitrarily;
           later on we set inchans to outchans,
           to emulate a plain outchans-sized file
        */
        if(flags[FLAG_M]){
            outchans = nchars;
        }

        if(props.chformat==MC_BFMT){
            if(flags[FLAG_M]){
                printf("-m specified: not decoding B-Format file.\n");
                flags[FLAG_B] = 0;
            }
            else
                flags[FLAG_B] = 1;
        }

        // we will not have both do_channel_mapping and do_decode toigether
        if(flags[FLAG_B] == 1){
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
                fprintf(stderr,"unsupported channel count (%u) for B-format file.\n",inchans);
                return 1;
            }

            switch(layout-1){
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
        }



        printf("opened %s: %ld frames, %u channels\n",argv[1],filesize,inchans);
        fpeaks = (PSF_CHPEAK *) calloc(inchans,sizeof(PSF_CHPEAK));
        if(fpeaks==NULL){
            puts("no memory for PEAK data\n");
            goto error;
        }
        //lpeaktime = (int) in_peaktime;
        res = psf_sndReadPeaks(ifd,fpeaks,(MYLONG *) &lpeaktime);
        if(res==0) {
            printf("no PEAK data in this soundfile\n");
        }
        else if(res < 0){
            printf("Error reading PEAK data\n");
            goto error;
        }
        else{
            in_peaktime = (time_t) lpeaktime;
            printf("PEAK data:\ncreation time: %s",ctime(&in_peaktime));

            for(i=0;i < inchans;i++){
                printf("CH %d: %.4f at frame %lu: \t%.4f secs\n",
                       i,fpeaks[i].val,fpeaks[i].pos,(double)(fpeaks[i].pos / (double) props.srate));
            }
        }
        // if doing channel map,  precompute into mem buffer
        if(nFramesToPlay <= ringframelen){
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
            // set up ring buffer  NB must be power of 2 size
            if(props.srate > 48000)
                ringframelen <<= 1;

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
            printf("File buffer size = %ld\n",ringframelen);

        }

        // worst case, ring buffer is empty! So need enough space
        // NB inchans may well be > outchans
        sfdata.inbuf = (float *) PaUtil_AllocateMemory(ringframelen * sizeof(float) * inchans);
        if(sfdata.inbuf==NULL){
            puts("No memory for read buffer\n");
            goto error;
        }
        sfdata.inbuflen = ringframelen;

        if(flags[FLAG_M]) {
            max_inchar = rootchar;

            sfdata.orderptr = malloc(outchans * sizeof(float*));
            for(i=0;i < nchars;i++){
                unsigned int thischar = argstring[i];
                //          printf("reading char %c (%d)\n",thischar,thischar);
                unsigned int chindex;
                if(thischar != '0' && (thischar < rootchar || thischar > maxchar)){
                    printf("illegal character in order string: %c\n",thischar);
                    goto error;
                }
                if(thischar =='0'){
                    //            printf("setting channel %d to zero.\n",i);
                    sfdata.orderptr[i] = NULL;
                    nzeros++;
                }
                else{
                    if(thischar > max_inchar)
                        max_inchar = thischar;
                    chindex = thischar - rootchar;
                    sfdata.orderptr[i] = sfdata.inbuf + chindex;
                }
            }
            if(nzeros==nchars)
                printf("Warning: -m order string is all zeros - a silent file will be made!\n");
            else{
                /* count inclusively! */
                if(props.chans < (int)(max_inchar - rootchar + 1)){
                    printf("File has %d channels; order string defines non-existent channels.\n",props.chans);
                    printf("For this file, maximum character is %c\n",rootchar + props.chans -1);
                    goto error;
                }
            }
            outchans = nchars;
            sfdata.do_mapping = 1;

            // final alloc when infile is read, below
        }

        if(props.srate > 48000) {
            frames_per_buffer *= 2;
            if(frames_per_buffer)
                printf("Audio buffer size = %lu\n",frames_per_buffer);
        }
        sfdata.ifd          = ifd;
        sfdata.inchans      = inchans;
        sfdata.outchans     = outchans;
        sfdata.frames_played  = 0;
        sfdata.gain         = gain;
        sfdata.copyfunc     = copyfunc;
        sfdata.decodefunc   = decodefunc;
        sfdata.do_decode    = flags[FLAG_B];
        sfdata.from_frame   = from_frame;
        if(to_frame > 0)
            sfdata.to_frame = to_frame;
        else {
            sfdata.to_frame = 0;
        }
        sfdata.current_frame = 0;
        sfdata.play_looped  = flags[FLAG_L];

        sfdata.srate = props.srate;

        sfdata.finished = 0;
        g_pdata = &sfdata;

#ifdef DEBUG
        {
            PSF_PROPS outprops = props;
            outprops.chans = outchans;
            sfdata.ofd = psf_sndCreate("debug.wav",&outprops,0,0,PSF_CREATE_RDWR);
            if(sfdata.ofd == -1){
                printf("Sorry - unable to create debug outfile\n");
            }
        }
#endif

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

#ifdef _WIN32

#ifdef NOTDEF
        if(devinfo){
            if(apiinfo->type  == paMME ){
                winmmeStreamInfo.size = sizeof(winmmeStreamInfo);
                winmmeStreamInfo.hostApiType = paMME;
                winmmeStreamInfo.version = 1;
                winmmeStreamInfo.flags = paWinMmeUseChannelMask;
                winmmeStreamInfo.channelMask = 0;
                outputParameters.hostApiSpecificStreamInfo = &winmmeStreamInfo;
# ifdef _DEBUG
                printf("WIN DEBUG: WinMME device channel mask set to 0.\n");
# endif
            }
        }
#endif

        if(devinfo) {
            int apitype = devinfo->hostApi;
            const PaHostApiInfo *apiinfo = Pa_GetHostApiInfo(apitype);
            //  if(do_speakermask)
            //      printf("Using file channel format %d\n",speakermask);
            printf("Using device %d: %s ",device,devinfo->name);

            if(apiinfo->type  == paDirectSound ){
                printf("(DS)\n");
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
            else
                printf("\n");
        }
#endif

        err = Pa_OpenStream(
                            &stream,
                            NULL,  /* No input */
                            &outputParameters, /* As above. */
                            props.srate,
                            frames_per_buffer,
                            paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                            playcallback,
                            &sfdata );



        if( err != paNoError ) {
            printf("Unable to open output device for %u-channel file.\n",outchans);
            goto error;
        }
        err =  Pa_SetStreamFinishedCallback( stream, finishedCallback );
        if( err != paNoError ) {
            printf("Unable to set finish callback\n");
            goto error;
        }
        sfdata.stream = stream;

        file_playing = 1; // need this for c/l test below!

        if(waitkey){
            printf("Press any key to start:\n");
            while (!kbhit()){
                if(!file_playing)        //check for instant CTRL-C
                    goto error;
            };
#ifdef _WIN32
            if(kbhit())
                _getch();                        //prevent display of char
#endif
        }

        // should this go in read thread func too?
        if(from_frame > 0){
            if(psf_sndSeek(sfdata.ifd,from_frame,PSF_SEEK_SET))     {
                printf("Error setting start position\n");
                goto error;
            }
            sfdata.current_frame = from_frame;
        }

        /* if small block, read it all into memory
         * if doing channel map, do it here rather in callback; we must set inchans to outchans */
        if(sfdata.memFramelength > 0){
            if(flags[FLAG_M]) {
                unsigned int i,j;
                int k;

                float **orderptr = NULL;
                float val;
                float *tempframebuf = malloc(sizeof(float) * sfdata.memFramelength * props.chans);
                nzeros = 0;
                if(tempframebuf==NULL){
                    puts("no memory for audio data\n");
                    goto error;
                }
                orderptr = malloc(outchans * sizeof(float*));
                for(i=0;i < nchars;i++){
                    unsigned int thischar = argstring[i];
                    unsigned int chindex;
                    if(thischar != '0' && (thischar < rootchar || thischar > maxchar)){
                        printf("illegal character in order string: %c\n",thischar);
                        goto error;
                    }
                    if(thischar =='0'){
                        orderptr[i] = NULL;
                        nzeros++;
                    }
                    else{
                        if(thischar > max_inchar)
                            max_inchar = thischar;
                        chindex = thischar - rootchar;
                        orderptr[i] = tempframebuf + chindex;
                    }
                }
                if(nzeros==nchars)
                    printf("Warning: -m order string is all zeros - a silent file will be made!\n");
                else {
                    /* count inclusively! */
                    if(props.chans < (int)(max_inchar - rootchar + 1)){
                        printf("File has %d channels; order string defines non-existent channels.\n",props.chans);
                        printf("For this file, maximum character is %c\n",rootchar + props.chans -1);
                        goto error;
                    }
                }
                if(psf_sndReadFloatFrames(sfdata.ifd,tempframebuf,sfdata.memFramelength) != sfdata.memFramelength) {
                    printf("Error reading soundfile into memory\n");
                    goto error;
                }


                for(j=0;j < sfdata.memFramelength; j++) {
                    for(k = 0; k < sfdata.outchans; k++) {
                        // find requested input chan
                        float *pchan = orderptr[k];
                        if(pchan == NULL)
                            val = 0.0f;
                        else
                            val = *(pchan + j * props.chans);
                        // and write to stream memory buffer
                        sfdata.membuf[j * sfdata.outchans + k] = val;
                    }
                }
                // done with these now
                free(tempframebuf);
                free(orderptr);
                sfdata.inchans = sfdata.outchans;
            }
            else {  // no fancy stuff, just read straight into mem buffer
                if(psf_sndReadFloatFrames(sfdata.ifd,sfdata.membuf,sfdata.memFramelength)
                   != sfdata.memFramelength) {
                    printf("Error reading soundfile into memory\n");
                    goto error;
                }
            }
        }

        // set up timer
#ifdef unix
        if(do_updatemessages) {
            setitimer(ITIMER_REAL, &tout_val,0);
            signal(SIGALRM,alarm_wakeup); /* set the Alarm signal capture */
        }
#endif

#ifdef _WIN32
        // not sure of the best position for this
        //if(!CreateTimerQueueTimer(&sfdata.hTimer, hTimerQueue,
        //    (WAITORTIMERCALLBACK) TimerCallback, &sfdata,250,250,0)) {
        //    printf("failed to start timer (3).\n");
        //    return 1;
        // }
#endif
        /* Start the file reading thread */
        sfdata.startTime = Pa_GetStreamTime(stream );

        if(sfdata.memFramelength == 0){

            err = startThread(&sfdata, threadFunctionReadFromRawFile);
            if( err != paNoError ) goto error;
        }
        else {
            sfdata.flag = 0;
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
        if( err != paNoError )
            goto error;

        printf("Hit CTRL-C to stop.\n");
        while(!sfdata.finished && file_playing){
            // nothing to do!
            Pa_Sleep(10);
        }
        // note to programmer: any bug in audio buffer arithmetic will likely cause crash here!
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
        printf("\r%.2f secs\n",(float)(sfdata.lastTime));
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
        if(sfdata.orderptr)
            free(sfdata.orderptr);
        if(ifd >=0)
            psf_sndClose(ifd);
        if(fpeaks)
            free(fpeaks);
#ifdef DEBUG
        if(sfdata.ofd >=0)
            psf_sndClose(sfdata.ofd);
#endif
        psf_finish();

        return 0;
    }

    int show_devices(void)
    {
        PaDeviceIndex numDevices,p;
        const    PaDeviceInfo *pdi;
#ifdef _WIN32
        const PaHostApiInfo* api_info;
        const char *apiname;
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
        for( p = 0; p < numDevices; p++ )
            {
                pdi = Pa_GetDeviceInfo( p );

                //#ifdef _WIN32
                //                      /*RWD: skip, info on inputs */
                //                      if(pdi->maxOutputChannels == 0)
                //                              continue;
                //#endif
                nOutputDevices++;

                if( p == Pa_GetDefaultOutputDevice() )
                    printf("*");
                else
                    printf(" ");
#ifdef WIN32
                api_info = Pa_GetHostApiInfo(pdi->hostApi);
                apiname = api_info->name;
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
