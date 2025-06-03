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
 
/*
 *  recsf.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <string.h>

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

#if defined unix || defined linux
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

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    // Guardar configuración terminal actual
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
#endif



#ifndef min
#define min(x,y)  ((x) < (y) ? (x) : (y))
#endif

#define N_BFORMATS (10)
static const int bformats[N_BFORMATS] = {2,3,4,5,6,7,8,9,11,16};


//#define FRAMES_PER_BUFFER (4096)
// want nice big buffer for recording!
#define RINGBUF_NFRAMES    (32768)
#define NUM_WRITES_PER_BUFFER   (4)
#define DEFAULT_SRATE (44100)
#define DEFAULT_STYPE (1)
#define DEFAULT_CHANS (2)
#define DBRANGE (64)

enum { ARG_PROGNAME, ARG_OUTFILE, ARG_DUR };

#ifdef _WIN32
HANDLE ghEvent;
#endif


typedef struct {
    PaUtilRingBuffer ringbuf;
    PaStream *stream;
    float *ringbufData;
    char  peakstr[(DBRANGE/2)+1];
    PaTime startTime;
    PaTime lastTime;
#ifdef WIN32
    void *hThread;
    HANDLE hTimer;
    HANDLE hTimerCount;
#endif
#ifdef unix
    pthread_t hThread;
#endif
    double peak;
    unsigned long frames_written;
    unsigned long frames_to_write; // for optional duration arg
    unsigned long current_frame;
    int srate;
    int chans;
    int flag;
    int ofd;
    int finished;
    int showlevels;
} psfdata;

static int file_recording;
static psfdata *g_pdata = NULL;  // for timer interrupt routine


int show_devices(void);

void playhandler(int sig)
{
    if(sig == SIGINT)
        file_recording = 0;
}


#ifdef unix
void alarm_wakeup (int i)
{
    struct itimerval tout_val;
    
    signal(SIGALRM,alarm_wakeup);
    
    
    if(file_recording && g_pdata->stream) {
        double dBpeak  = (int)( 20.0 * log10(sqrt(g_pdata->peak)));
        int dBmin = -DBRANGE;
        int i;
        for(i=0;i < DBRANGE/2;i++){
            g_pdata->peakstr[i] = dBpeak > dBmin+(i*2) ? '*' : '.';
        }
        //printf("%.4f secs\r",(float)(g_pdata->frames_played /(double) g_pdata->srate));
        g_pdata->lastTime = Pa_GetStreamTime(g_pdata->stream ) - g_pdata->startTime;
        //printf("%.2f secs\r", g_pdata->lastTime);
        if(g_pdata->showlevels)
            printf("%.2f secs\t\t\%s\r", g_pdata->lastTime,g_pdata->peakstr );
        else
            printf("%.2f secs\r", g_pdata->lastTime);
        fflush(stdout);
    }
    tout_val.it_interval.tv_sec = 0;
    tout_val.it_interval.tv_usec = 0;
    tout_val.it_value.tv_sec = 0;
    tout_val.it_value.tv_usec = 250000;
    
    setitimer(ITIMER_REAL, &tout_val,0);
    
}
#endif


void finishedCallback(void *userData)
{
    psfdata *pdata = (psfdata*) userData;
    //printf("stream finished!\n");
    pdata->finished = 1;
    file_recording = 0;
}


#ifdef WIN32

VOID CALLBACK TimerCallback(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
    psfdata *pdata = (psfdata*) lpParam;
    
    if(file_recording && pdata->stream) {
        //printf("%.4f secs\r",(float)(g_pdata->frames_played /(double) g_pdata->srate));
        double dBpeak  = (int)( 20.0 * log10(sqrt(pdata->peak)));
        int dBmin = -DBRANGE;
        int dBval = dBmin;
        int i;
        for(i=0;i < DBRANGE/2;i++) {
            pdata->peakstr[i] = dBpeak > dBmin+(i*2) ? '*' : '.';
        }
        pdata->lastTime = Pa_GetStreamTime(pdata->stream ) - pdata->startTime;
        printf("%.2f secs\t\t%s\r", pdata->lastTime,pdata->peakstr );
        fflush(stdout);
        SetEvent(ghEvent);
    }
    else
        printf("\n");
}

#endif

// TODO: implement optional duration arg


#ifdef unix
static int threadFunctionWriteFile(void* ptr)
#else
static unsigned int __stdcall threadFunctionWriteFile(void* ptr)
#endif
{
    psfdata* pData = (psfdata*)ptr;

    /* Mark thread started */
    pData->flag = 0;

    while (1) {
        ring_buffer_size_t elementsInBuffer = PaUtil_GetRingBufferReadAvailable(&pData->ringbuf);
        if(file_recording == 0){
            //write out whatever remains in ring buffer
            void* ptr[2] = {0};
            ring_buffer_size_t sizes[2] = {0};

            //printf("flushing ring buffer...\n");

            ring_buffer_size_t elementsRead = PaUtil_GetRingBufferReadRegions(&pData->ringbuf, elementsInBuffer, ptr + 0, sizes + 0, ptr + 1, sizes + 1);
            if (elementsRead > 0) {
                int i;

                for (i = 0; i < 2 && ptr[i] != NULL; ++i)  {
                    unsigned long towrite = sizes[i];
                    if(pData->frames_to_write){
                        if(pData->frames_written + towrite > pData->frames_to_write)
                            towrite = pData->frames_to_write - pData->frames_written;
                    }
                    if(psf_sndWriteFloatFrames(pData->ofd,(float*) ptr[i],towrite) != towrite) {
                        printf("File %d write error\n",pData->ofd);
                        pData->flag = 0;
                        break;
                    }
                    pData->frames_written += towrite;
                    if(pData->frames_written == pData->frames_to_write){
                        //printf("recording done\n");
                        pData->flag = 1;
                        file_recording = 0;
                        break;
                    }
                }
                PaUtil_AdvanceRingBufferReadIndex(&pData->ringbuf, elementsRead);
            }

            break;
        }
        if ( (elementsInBuffer >= pData->ringbuf.bufferSize / NUM_WRITES_PER_BUFFER) || pData->flag ) {
            void* ptr[2] = {0};
            ring_buffer_size_t sizes[2] = {0};
            
            /* By using PaUtil_GetRingBufferReadRegions, we can read directly from the ring buffer */
            ring_buffer_size_t elementsRead = PaUtil_GetRingBufferReadRegions(&pData->ringbuf, elementsInBuffer, ptr + 0, sizes + 0, ptr + 1, sizes + 1);
            if (elementsRead > 0) {
                int i;

                for (i = 0; i < 2 && ptr[i] != NULL; ++i)  {
                    unsigned long towrite = sizes[i];
                    if(pData->frames_to_write){
                        if(pData->frames_written + towrite > pData->frames_to_write)
                            towrite = pData->frames_to_write - pData->frames_written;
                    }
                    if(psf_sndWriteFloatFrames(pData->ofd,(float*) ptr[i],towrite) != towrite) {
                        printf("File %d write error\n",pData->ofd);
                        pData->flag = 0;
                        break;
                    }
                    pData->frames_written += towrite;
                    if(pData->frames_written == pData->frames_to_write){
                        //printf("recording done\n");
                        pData->flag = 1;
                        file_recording = 0;
                        break;
                    }
                }
                PaUtil_AdvanceRingBufferReadIndex(&pData->ringbuf, elementsRead);
            }
            
            if (pData->flag) {
                break;
            }
        }
        /* Sleep a little while... */
        Pa_Sleep(10);
    }
    return 0;
}

#ifdef unix
// TODO: unix return type should be void*
typedef int (*threadfunc)(void*);
#endif
#ifdef WIN32
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
// for sake of completion - curently NOT USED
#if 0
static int stopThread( psfdata* pdata )
{
    // RWD: just called when all data played; must be called before StopStream
    //   pdata->flag = 1;
    /* Wait for thread to stop */
       while (pdata->flag == 0) {
           Pa_Sleep(10);
       }
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

static int recordCallback( const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData )
{
    psfdata *data = (psfdata*) userData;
    const float *rptr = (const float*) inputBuffer;
    double peak = 0.0, val;
    unsigned int i;
    
    (void) outputBuffer; /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    data->current_frame += PaUtil_WriteRingBuffer(&data->ringbuf, rptr, framesPerBuffer);

    // simple level meter!
    val = rptr[0];
    val  =  val * val;
    peak = val;
    for(i=0;i < framesPerBuffer * data->chans ;i++){
        val = rptr[i];
        val  =  val * val;
        peak =  peak < val ? val : peak;
    }
    data->peak = peak;
    if(data->flag) {
        return paComplete; // or paAbort?
    }
    return paContinue;
}

void usage(void)
{
        printf("usage: recsf [-BN][-cN][-dN][-hN][-i][-p][-rN][-tN] outfile [dur]\n"
               "outfile:   output file in WAVE,AIFF or AIFC formats,\n"
               "           determined by the file extension.\n"
               "           use extension .amb to create a B-Format file.\n"
               "-BN    :   set memory buffer size to N frames (default: %d).\n"
               "              N must be a power of 2 (e.g 4096, 8192, 16384 etc).\n"
               "-cN    :   set channels to N (default: 2).\n"
               "-hN    :   set hardware buffer size to N frames (default: set by device)\n"
               "-p     :   suppress running peak level indicator\n"
               "-rN    :   set sample rate to N Hz (default: 44100)\n"
               "-tN    :   set sample type to N (default: 1)\n"
               "           0 :  16 bits\n"
               "           1 :  24 bits\n"
               "           2 :  32bit floats\n"
               "dur    :   optional fixed duration for outfile\n"
               "             (overriden by Ctrl-C)\n"
               "           Otherwise, use Ctrl_C to terminate recording.\n"
               "-dN    :   use input device N\n"
               "-i     :   start recording immediately (default: wait for keypress)\n"
               ,RINGBUF_NFRAMES);
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

int main(int argc,char **argv)
{
    PaStreamParameters inputParameters;
#ifdef _WIN32
    /* portaudio sets default channel masks we can't use; we must do all this to set default mask = 0! */
    PaWinDirectSoundStreamInfo directSoundStreamInfo;
    //    PaWinMmeStreamInfo winmmeStreamInfo;

#endif
    PaDeviceInfo *devinfo = NULL;
    PaStream *stream = NULL;
//    PaStreamCallback *callback = recordCallback;
    PaError err = paNoError;
    psfdata sfdata;
    PSF_CHPEAK *fpeaks = NULL;
    PSF_PROPS props;
    const char* outfilename = NULL;
    MYLONG lpeaktime;
    int waitkey = 1;
    int showlevels = 1;
    double duration = 0.0;
    int i;
    int res;
    int samptype = DEFAULT_STYPE;
    char* ext = NULL;
    PaDeviceIndex device;
    unsigned long ringframelen = RINGBUF_NFRAMES;  // length of ring buffer in m/c frames
    unsigned long frames_per_buffer = 0;
    unsigned int frameBlocksize = 0;
    double maxdurFrames = 0.0;
    unsigned long LmaxdurFrames = 0;
    unsigned long LmaxdurSecs = 0;
    unsigned long LmaxdurMins = 0;

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


    props.chans = DEFAULT_CHANS;
    props.srate = DEFAULT_SRATE;
    props.samptype = samptype;


    signal(SIGINT,playhandler);

    sfdata.ringbufData = NULL;
    sfdata.frames_to_write = 0;
    printf("RECSF: multi-channel record to file. v 1.1.0 RWD,CDP 2013\n");
    file_recording = 0;
    err = Pa_Initialize();
    if( err != paNoError ) {
        printf("Failed to initialize Portaudio.\n");
        Pa_Terminate();
        return 1;
    }
    device =  Pa_GetDefaultInputDevice();

    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("1.1.0\n");
        return 0;
    }
    if(argc < ARG_DUR) {
        printf("insufficient args\n");
        usage();
        show_devices();
        Pa_Terminate();
        return 1;
    }
    while(argv[1][0]=='-'){
        int err = 0;
        unsigned long userbuflen = 0;
        switch(argv[1][1]){
            case 'c':
                if(argv[1][2]=='\0'){
                    printf("-c flag requires parameter\n");
                    err++;
                    break;
                }
                props.chans = atoi(&(argv[1][2]));
                if(props.chans < 1){
                    printf("bad value %d for channels\n",props.chans);
                    err++;
                }
                break;
            case 'd':
                if(argv[1][2]=='\0'){
                    printf("-d flag requires parameter\n");
                    err++;
                    break;
                }
                device = atoi(&(argv[1][2]));
                break;
            case 'h':
                if(argv[1][2]=='\0'){
                    printf("-h flag requires parameter\n");
                    err++;
                    break;
                }
                frames_per_buffer = atoi(&(argv[1][2]));
                if(frames_per_buffer < 32){
                    printf("-h value too small; must be >= 32\n");
                    err++;
                }
                break;
            case 'i':
                waitkey = 0;
                break;
            case 'p':
                showlevels = 0;
                break;
            case 'r':
                if(argv[1][2]=='\0'){
                    printf("-r flag requires parameter\n");
                    err++;
                    break;
                }
                props.srate = atoi(&(argv[1][2]));
                if(props.srate <= 0){
                    printf("bad value %d for srate\n",props.srate);
                    err++;
                }
                break;
            case 't':
                if(argv[1][2]=='\0'){
                    printf("-t flag requires parameter\n");
                    err++;
                    break;
                }
                samptype = atoi(&(argv[1][2]));
                if(samptype < 0 || samptype > 2){
                    printf("bad value %d for sample type\n",samptype);
                    err++;
                }
                break;
            case 'B':
                if(argv[1][2]=='\0'){
                    printf("-B flag requires parameter\n");
                    err++;
                    break;
                }
                ringframelen = atoi(&(argv[1][2]));
                if(ringframelen < 1024){
                    printf("-B: buffer size must be >=1024\n");
                    err++;
                }
                userbuflen = NextPowerOf2(ringframelen);
                if(userbuflen != ringframelen){
                    printf("-B: buffer size must be power of 2 size\n");
                    Pa_Terminate();
                    return 1;
                }
                
                break;
            default:
                printf("unrecognised flag option\n");
                err++;
                break;
        }
        if(err){
            Pa_Terminate();
            return 1;
        }
        argv++;  argc--;
    }

    if(argc < ARG_DUR || argc > ARG_DUR+1) {
        usage();
        show_devices();
        Pa_Terminate();
        return 1;
    }
    outfilename = argv[ARG_OUTFILE];
    switch(samptype){
        case 0:
            props.samptype = PSF_SAMP_16;
            frameBlocksize = 2;
            break;
        case 1:
            props.samptype = PSF_SAMP_24;
            frameBlocksize = 3;
            break;
        case 2:
            props.samptype = PSF_SAMP_IEEE_FLOAT;
            frameBlocksize = 4;
            break;
        default:
            printf("stype must be 0,1 or 2\n");
            Pa_Terminate();
            return 1;
    }
    // find max recording time, with safety margin
    frameBlocksize *= props.chans;
    maxdurFrames = (pow(2.0,32.0) / frameBlocksize) - 1000;
    LmaxdurFrames = (unsigned long) maxdurFrames;
    LmaxdurSecs =  LmaxdurFrames / props.srate;
    LmaxdurMins =  LmaxdurSecs / 60;
    printf("Max recording time: %lu mins, %lu secs\n",LmaxdurMins,LmaxdurSecs - (LmaxdurMins * 60));

    if(argc==ARG_DUR+1){
        duration = atof(argv[ARG_DUR]);
        if(duration <=0.0){
            printf("duration must be positive!\n");
            Pa_Terminate();
            return 1;
        }
        if(duration > (double) LmaxdurSecs){
            printf("specified duration too long for file format.\n");
            Pa_Terminate();
            return 1;
        }
        sfdata.frames_to_write = (unsigned long) (duration * props.srate);
    }
    ext = strrchr(outfilename,'.');
    if(ext && stricmp(ext,".amb")==0) {
        int matched = 0;
        for(i=0;i < N_BFORMATS;i++) {
            if(props.chans == bformats[i]){
                matched = 1;
                break;
            }
        }
        if(!matched){
            printf("WARNING: No Bformat definition for %d-channel file.\n",props.chans);
        }
        props.format = PSF_WAVE_EX;
        props.chformat = MC_BFMT;
    }
    else {
        // we must be strictly correct with WAVE formats!
        props.chformat = props.chans > 2 ? MC_STD : STDWAVE;
        props.format = psf_getFormatExt(outfilename);
        if(props.chans > 2 || props.samptype > PSF_SAMP_16 || props.srate > 48000)
            props.format = PSF_WAVE_EX;

    }
    sfdata.ofd = psf_sndCreate(outfilename,&props,0,0,PSF_CREATE_RDWR);
    if(sfdata.ofd < 0){
        printf("Sorry - unable to create outfile\n");
        goto error;
    }

    fpeaks = (PSF_CHPEAK *) calloc(props.chans,sizeof(PSF_CHPEAK));
    if(fpeaks==NULL){
        puts("no memory for PEAK data\n");
        goto error;
    }
    if(props.srate > 48000)
        ringframelen <<= 1;
    printf("File buffer size = %ld\n",ringframelen);
    // NB ring buffer sized for decoded data, hence outchans here; otherwise inchans = outchans
    sfdata.ringbufData = (float *) malloc(ringframelen * sizeof(float) * props.chans);
    /* From now on, recordedSamples is initialised. */
    if( sfdata.ringbufData == NULL )   {
        puts("Could not allocate play buffer.\n");
        goto error;
    }
    // number of elements has to be a power of two, so each element has to be a full m/c frame
    if (PaUtil_InitializeRingBuffer(&sfdata.ringbuf, sizeof(float) * props.chans, ringframelen , sfdata.ringbufData) < 0) {
        puts("Could not initialise play buffer.\n");
        goto error;
    }
    // scale frame buf to sample rate
    if(props.srate > 48000)
         frames_per_buffer *= 2;
    if(frames_per_buffer > 0)
        printf("Audio buffer size = %lu frames\n",frames_per_buffer);

    sfdata.chans      = props.chans;
    sfdata.frames_written  = 0;
    sfdata.current_frame = 0;
    sfdata.srate = props.srate;
    sfdata.peakstr[DBRANGE/2] = '\0';
    sfdata.finished = 0;
    sfdata.showlevels = showlevels;
    g_pdata = &sfdata;

    inputParameters.device = device;   /*Pa_GetDefaultOutputDevice(); */ /* default output device */
    inputParameters.channelCount = props.chans;
    inputParameters.sampleFormat = paFloat32;             /* 32 bit floating point output */
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    devinfo = (PaDeviceInfo *) Pa_GetDeviceInfo(device);
#ifndef WIN32
    if(devinfo){
        printf("Using device %d: %s\n",device,devinfo->name);
    }
#endif

#ifdef WIN32
    if(devinfo) {
        int apitype = devinfo->hostApi;
        const PaHostApiInfo *apiinfo = Pa_GetHostApiInfo(apitype);
        printf("Using device %d: %s:\n",device,devinfo->name);

        if(apiinfo->type  == paDirectSound ){
            printf("(DS)\n");
            /* set this IF we are using Dsound device. */
            directSoundStreamInfo.size = sizeof(PaWinDirectSoundStreamInfo);
            directSoundStreamInfo.hostApiType = paDirectSound;
            directSoundStreamInfo.version = 2;
            directSoundStreamInfo.flags = paWinDirectSoundUseChannelMask;
            directSoundStreamInfo.channelMask = 0;
            inputParameters.hostApiSpecificStreamInfo = &directSoundStreamInfo;
        }
        else if(apiinfo->type == paASIO)
            printf("(ASIO)\n");
        // else
        //    printf("API unknown!);
    }

#endif
    // TODO; move this up to before file is created?
    err = Pa_IsFormatSupported(&inputParameters, NULL,props.srate);
    if(err != paNoError){
        printf("Selected device does not support this format.\n");
        goto error;
    }

    err = Pa_OpenStream(
                        &stream,
                        &inputParameters,
                        NULL,  /* No output */
                        props.srate,
                        frames_per_buffer,
                        paClipOff,
                        recordCallback,
                        &sfdata );

    if( err != paNoError ) {
        printf("Unable to open output device for %d-channel file.\n",props.chans);
        goto error;
    }
    err =  Pa_SetStreamFinishedCallback( stream, finishedCallback );
    if( err != paNoError ) {
        printf("Internal error: unable to set finish callback\n");
        goto error;
    }
    sfdata.stream = stream;



    file_recording = 1;
    if(waitkey){
        printf("Press any key to start:\n");
        while (!kbhit()){
            if(!file_recording)  //check for instant CTRL-C
             goto error;
        };
    #ifdef _WIN32
        if(kbhit())
         _getch();            //prevent display of char
    #endif
    }

    // set up timer
#ifdef unix
    setitimer(ITIMER_REAL, &tout_val,0);
    signal(SIGALRM,alarm_wakeup); /* set the Alarm signal capture */
#endif
    /* Start the file reading thread */
    sfdata.startTime = Pa_GetStreamTime(stream );
    err = startThread(&sfdata, threadFunctionWriteFile);
    if( err != paNoError ) goto error;

#ifdef WIN32
    if(!CreateTimerQueueTimer(&sfdata.hTimer, hTimerQueue,
                              (WAITORTIMERCALLBACK) TimerCallback, &sfdata,200,200,0)) {
        printf("failed to start timer (3).\n");
        return 1;
    }
#endif
    err = Pa_StartStream( stream );
    if( err != paNoError )
        goto error;

    printf("Hit CTRL-C to stop.\n");

    while((!sfdata.finished) && file_recording){
        // nothing to do!
        Pa_Sleep(10);
    }
    // note to programmer: any bug in audio buffer arithmetic will likely cause crash here!
    err = Pa_StopStream( stream );
    if( err != paNoError ) {
        printf("Error stopping stream\n");
        goto error;
    }

    // need to stop thread explicitly?

    err = Pa_CloseStream( stream );
    if( err != paNoError ) {
        printf("Error closing stream\n");
        goto error;
    }
#ifdef WIN32
    CloseHandle(ghEvent);
    DeleteTimerQueue(hTimerQueue);
#endif
    printf("%.2f secs\n",(float)(sfdata.lastTime));
    fflush(stdout);
    printf("Recording finished.\n");
    res = psf_sndReadPeaks(sfdata.ofd,fpeaks,(MYLONG *) &lpeaktime);
    if(res==0) {
        printf("no PEAK data in this soundfile\n");
    }
    else if(res < 0){
        printf("Error reading PEAK data\n");
        goto error;
    }
    else{
        // creation time not available until file closed; don't need it here!
        printf("PEAK data:\n");
        for(i=0;i < sfdata.chans;i++){
            if(fpeaks[i].val > 0.0){
                double dBval = 20.0 * log10(fpeaks[i].val);
                printf("CH %d: %.4f (%.2lfdB) at frame %u: \t%.4f secs\n",
                   i,fpeaks[i].val,dBval,fpeaks[i].pos,(double)(fpeaks[i].pos / (double) props.srate));
            }
            else {
                printf("CH %d: %.4f (-infdB)at frame %u: \t%.4f secs\n",
                   i,fpeaks[i].val,fpeaks[i].pos,(double)(fpeaks[i].pos / (double) props.srate));

            }
        }
    }

error:
    Pa_Terminate();

//#ifdef _WIN32
//    CloseHandle(ghEvent);
//    DeleteTimerQueue(hTimerQueue);
//#endif
    if( sfdata.ringbufData )
        free(sfdata.ringbufData);
        sfdata.ringbufData = NULL;
    if(sfdata.ofd >=0)
        psf_sndClose(sfdata.ofd);
    if(fpeaks)
        free(fpeaks);

    psf_finish();
    return 0;
}



int show_devices(void)
{
//    int i,j;
    PaDeviceIndex numDevices,p;
    const    PaDeviceInfo *pdi;
#ifdef _WIN32
    const PaHostApiInfo* api_info;
    const char *apiname;
#endif
    PaError  err;
    int nInputDevices = 0;

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
        
        //#ifdef _WIN32
        //          /*RWD: skip, info on inputs */
        //          if(pdi->maxOutputChannels == 0)
        //              continue;
        //#endif
        nInputDevices++;
        if( p == Pa_GetDefaultInputDevice() )
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
    return 0;
}

