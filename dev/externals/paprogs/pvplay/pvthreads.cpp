
/*
 * Copyright (c) 1983-2020 Richard Dobson and Composers Desktop Project Ltd
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

/*  pvthreads.c: thread functions for pvplay */
/* Jan 2014: completed pvx support */

#include "pvplay.h"
#include <assert.h>

extern int file_playing;
extern psfdata *g_pdata;

__inline void stereo_interls(const float *in_l,const float *in_r,float *out,long insize)
{
    long i;
    const float *pfl_l,*pfl_r;
    float*pfl_o;
    pfl_o = out;
    pfl_l = in_l;
    pfl_r = in_r;

    for(i=insize;i;--i){
        *pfl_o++ = *pfl_l++;
        *pfl_o++ = *pfl_r++;
    }
}

/* soundfile */

/* writes decoded data to ring buffer */
/* size of element is full m/c frame */
// TODO: unix thread should return void*
#ifdef unix
/*int*/void*  threadFunctionReadFromRawFile(void* ptr)
#else
unsigned int __stdcall threadFunctionReadFromRawFile(void* ptr)
#endif
{
    psfdata* pdata = (psfdata*)ptr;
    
    /* Mark thread started */
    pdata->flag = 0;
    //printf("thread: from_frame = %d, to_frame = %d, current_Frame = %d\n",pdata->from_frame, pdata->to_frame, pdata->current_frame);
    while (1) {
        ring_buffer_size_t nElementsProcessed = 0;
        ring_buffer_size_t elementsInBuffer = PaUtil_GetRingBufferWriteAvailable(&pdata->ringbuf);
           
        if (elementsInBuffer >= pdata->ringbuf.bufferSize / 4) {
            void* ptr[2] = {NULL};
            ring_buffer_size_t sizes[2] = {0};
            
            /* By using PaUtil_GetRingBufferWriteRegions, we can write directly into the ring buffer */
            PaUtil_GetRingBufferWriteRegions(&pdata->ringbuf, elementsInBuffer, &ptr[0], &sizes[0], &ptr[1], &sizes[1]);
            
            if (file_playing)  {
                ring_buffer_size_t itemsReadFromFile;
                int i,j;
                int framesread = 0;
                int sampswanted = 0;
                int sampsread = 0;
                // we work with frames, = constant across inchans and outchans
                itemsReadFromFile = 0;
                
                for(i = 0; i < 2 && (ptr[i] != NULL); i++) {
                    // NB ringbuf is sized by m/c frames
                    int frameswanted = sizes[i];
                    
                    pdata->current_frame = sndtellEx(pdata->ifd) / pdata->inchans;
                    // read up to end frame if requested
                    if(pdata->to_frame < pdata->current_frame + frameswanted)
                        frameswanted = pdata->to_frame - pdata->current_frame;
                    
                    if(frameswanted > 0){
                        sampswanted = frameswanted * pdata->inchans;
                        sampsread = fgetfbufEx(pdata->inbuf,sampswanted,pdata->ifd,0);
                        framesread = sampsread / pdata->inchans;
                        
                        if(framesread < 0){ // read error!
                            printf("Error reading soundfile: %s ifd = %d\n", sferrstr(),pdata->ifd);
                            pdata->flag = 1;
                            file_playing = 0;
                            break;  // just out of for loop - need to return instead?
                        }
                    }
                    if(framesread == 0){
                        /* EOF. EITHER: finish, or rewind if looping playback*/
                        if(pdata->play_looped){
                            if(sndseekEx(pdata->ifd,pdata->from_frame * pdata->inchans,SEEK_SET) < 0){
                                printf("Error looping soundfile\n");
                                pdata->flag = 1;
                                file_playing = 0;
                                break;
                            }
                            // sizes[1] especially may well = 0
                            if(frameswanted==0)
                                break;
                            sampswanted = frameswanted * pdata->inchans;
                            sampsread = fgetfbufEx(pdata->inbuf,sampswanted,pdata->ifd,0);
                            framesread = sampsread / pdata->inchans;
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
                        // no channel mapping in pvplay
                        if(pdata->do_decode) {
                            ABFSAMPLE abfsamp;
                            
                            for(j=0; j < framesread; j++){
                                // single frame only
                                pdata->copyfunc(&abfsamp,pdata->inbuf + (j * pdata->inchans));
                                /* BIG TODO: update funcs to process large frame buffer! */
                                /* NB: ring buffer is effectively defined as raw bytes */
                                pdata->decodefunc(&abfsamp, (float*)(ptr[i]) + (j * pdata->outchans), 1);
                                nElementsProcessed++;
                            }
                        }
                        else {  // inchans = outchans
                            memcpy(ptr[i],pdata->inbuf,framesread * sizeof(float) * pdata->inchans);
                            nElementsProcessed += framesread;
                        }
                    }
                }
                PaUtil_AdvanceRingBufferWriteIndex(&pdata->ringbuf, itemsReadFromFile);
            }
            else {
                // this code is activated on Ctrl-C. Can do immediate finish by setting flag
                //    printf("file done\n");
                pdata->flag = 1;  
                break;
            }
        }
        //  else {
        //      printf("ringbuf size = %d, elements remaining = %d, playing = %d\n",pdata->ringbuf.bufferSize,elementsInBuffer,file_playing);
        //  }

        /* Sleep a little while...! */
        Pa_Sleep(10);
    }
    return 0;
}

/*  versions for .ana file (always mono only) */

#ifdef unix
/*int*/void*  threadFunctionReadFromAnalFile(void* ptr)
#else
unsigned int __stdcall threadFunctionReadFromAnalFile(void* ptr)
#endif
{
    psfdata* pdata = (psfdata*)ptr;
    
    /* Mark thread started */
    pdata->flag = 0;
    
    while (1) {
        ring_buffer_size_t nElementsProcessed = 0;
        ring_buffer_size_t elementsInBuffer = PaUtil_GetRingBufferWriteAvailable(&pdata->ringbuf);
        
        if (elementsInBuffer >= pdata->ringbuf.bufferSize / 4) {
            void* ptr[2] = {NULL};
            ring_buffer_size_t sizes[2] = {0};
            
            //read and analyse  as many frames as resynth space is available for, into inbuf
            // then distribute to ringbuf sections as necessary
            //printf("from_frame = %d,to_frame = %d\n",pdata->from_frame,pdata->to_frame);
            int framestoget = elementsInBuffer / pdata->fftsize;
            //assert(framestoget); // must be at least one, or ring buffer is too small!
            if(framestoget==0)
                break;
            float *pbuf = pdata->inbuf;
            int i, got = 0;
            int elementstowrite = 0;
            
            if (file_playing)  {
                /* find out where we are ... */
                long framepos1 = sndtellEx(pdata->ifd) / pdata->anal_framesize;
                
                for(i = 0;i < framestoget;i++){ 
                    /* read one frame*/
                    got = fgetfbufEx(pdata->analframe1,pdata->anal_framesize,pdata->ifd,0);
                    if(got == -1) {
                        fprintf(stderr,"\nError reading from sfile\n");
                        file_playing  = 0;
                        break;
                    }
                    if(got != pdata->anal_framesize) {
                        if(got > 0) {
                            fprintf(stderr,"Infile error: incomplete analysis frame encountered\n");
                            file_playing  = 0;
                            break;
                        }
                    }
                    framepos1++;
                    
                    if(got && (framepos1 <= pdata->to_frame)) {
                        long samps = pdata->pv_l->process_frame(pdata->analframe1,pbuf,PVOC_AMP_FREQ);
                        elementstowrite += samps;
                        pbuf += samps;
                    }
                    
                    if (got==0 || framepos1 >= pdata->to_frame){
                        if(pdata->play_looped) {
                            long pos;
                            pos = sndseekEx(pdata->ifd,pdata->from_frame * pdata->anal_framesize,SEEK_SET);
                            if(pos < 0){
                                fprintf(stderr,"Error rewinding frame loop.\n");
                                file_playing  = 0;
                                break;
                            }
                        }
                        else if(elementsInBuffer == pdata->ringbuf.bufferSize)  {
                            //printf("buffer empty!\n");
                            pdata->flag = 1;
                            break;
                        }
                    }  
                }
                
                //apply gain to inbuf first, then copy to ringbuf
                if(pdata->gain != 1.0) {
                    for(i=0;i < elementstowrite;i++)
                        pdata->inbuf[i]  = (float)(pdata->inbuf[i] * pdata->gain);
                }
                /* By using PaUtil_GetRingBufferWriteRegions, we can write directly into the ring buffer */
                PaUtil_GetRingBufferWriteRegions(&pdata->ringbuf, elementsInBuffer, &ptr[0], &sizes[0], &ptr[1], &sizes[1]);
                pbuf = pdata->inbuf;
                //printf("ring buffer sizes : %d:%d",(int)sizes[0], (int)sizes[1]);
                for(i = 0; i < 2; i++) {
                    if(ptr[i]) {
                        int frameswanted = sizes[i];
                        memcpy(ptr[i],pbuf,frameswanted * sizeof(float) * pdata->inchans);
                        pbuf += frameswanted * pdata->inchans;
                        nElementsProcessed += frameswanted;
                    }
                }
                if(elementstowrite)
                    PaUtil_AdvanceRingBufferWriteIndex(&pdata->ringbuf, elementstowrite);
            } // file_playing
            else {
                // this code is activated on Ctrl-C. Can do immediate finish by setting flag
                //               printf("file done\n");
                pdata->flag = 1;  
                break;
            }
        }
        /* Sleep a little while... */
        Pa_Sleep(5);
    }    
    return 0;
}

// version for pvx format, just mono and stereo supported for now
#ifdef unix
/*int*/void*  threadFunctionReadFromPVXFile(void* ptr)
#else
unsigned int __stdcall threadFunctionReadFromPVXFile(void* ptr)
#endif
{
    psfdata* pdata = (psfdata*)ptr;

    /* Mark thread started */
    pdata->flag = 0;

    while (1) {
        ring_buffer_size_t nElementsProcessed = 0;
        ring_buffer_size_t elementsInBuffer = PaUtil_GetRingBufferWriteAvailable(&pdata->ringbuf);

        // we remember that an element is a frame (mono or stereo)
        if (elementsInBuffer >= pdata->ringbuf.bufferSize / 4) {
            void* ptr[2] = {NULL};
            ring_buffer_size_t sizes[2] = {0};

            //read and analyse  as many frames as resynth space is available for, into inbuf
            // then distribute to ringbuf sections as necessary
            //printf("from_frame = %d,to_frame = %d\n",pdata->from_frame,pdata->to_frame);
            /* pvx : file framecount is of single frames - need enough space to synth a single mc analysis frame */
            /* but to_frame and from_frame are m/c frame count */
            int framestoget = (elementsInBuffer  / pdata->fftsize);

            if(framestoget==0)
                break;

            int i, got = 0;
            int elementstowrite = 0;

            if(pdata->inchans==1) {
                float *pbuf = pdata->inbuf;
                if (file_playing)  {
                    /* find out where we are ...multi-chan frame count */
                    long framepos = pvoc_framepos(pdata->pvfile) / pdata->inchans; // NB
                    if(framepos < 0){
                        fprintf(stderr,"\nError reading file frame position\n");
                        file_playing  = 0;
                        break;  
                    }
                    for(i = 0;i < framestoget;i++){ 
                        /* read one frame*/
                        got = pvoc_getframes(pdata->pvfile,pdata->analframe1,1);
                        if(got == -1) {
                            fprintf(stderr,"\nError reading from sfile\n");
                            file_playing  = 0;
                            break;
                        }

                        framepos++;

                        if(got && (framepos <= pdata->to_frame)) {
                            long samps = pdata->pv_l->process_frame(pdata->analframe1,pbuf,PVOC_AMP_FREQ);
                            elementstowrite += samps;
                            pbuf += samps;
                        }

                        if (got==0 || framepos >= pdata->to_frame){
                            if(pdata->play_looped) {
                                if(pvoc_seek_mcframe(pdata->pvfile,pdata->from_frame,SEEK_SET)) {
                                    fprintf(stderr,"Error rewinding frame loop.\n");
                                    file_playing  = 0;
                                    break;
                                }
                            }
                            else if(elementsInBuffer == pdata->ringbuf.bufferSize)  {
                                //printf("buffer empty!\n");  
                                pdata->flag = 1;
                                break;
                            }
                        }
                    }  // framestoget

                    //apply gain to inbuf first, then copy to ringbuf
                    if(pdata->gain != 1.0) {
                        for(i=0;i < elementstowrite;i++)
                            pdata->inbuf[i]  = (float)(pdata->inbuf[i] * pdata->gain);
                    }
                    /* By using PaUtil_GetRingBufferWriteRegions, we can write directly into the ring buffer */
                    PaUtil_GetRingBufferWriteRegions(&pdata->ringbuf, elementsInBuffer, &ptr[0], &sizes[0], &ptr[1], &sizes[1]);
                    pbuf = pdata->inbuf;
                    //printf("ring buffer sizes : %d:%d",(int)sizes[0], (int)sizes[1]);
                    for(i = 0; i < 2; i++) {        
                        if(ptr[i]) {
                            int frameswanted = sizes[i];
                            memcpy(ptr[i],pbuf,frameswanted * sizeof(float) * pdata->inchans);
                            pbuf += frameswanted * pdata->inchans;
                            nElementsProcessed += frameswanted;
                        }
                    }
                    if(elementstowrite)
                        PaUtil_AdvanceRingBufferWriteIndex(&pdata->ringbuf, elementstowrite);
                } // file_playing
                else {
                    // this code is activated on Ctrl-C. Can do immediate finish by setting flag
                    //               printf("file done\n");
                    pdata->flag = 1;  
                    break;
                }
            }  //inchans==1
            else if (pdata->inchans==2) {
                float *pbuf = pdata->outbuf_l;
                float *pbuf_r = pdata->outbuf_r;
                // framestoget is count of m/c analysis frames to read
                if(file_playing){
                    /* find out where we are ...multi-chan frame count */
                    long framepos = pvoc_framepos(pdata->pvfile) / pdata->inchans; // NB
                    if(framepos < 0){
                        fprintf(stderr,"\nError reading file frame position\n");
                        file_playing  = 0;
                        break;
                    }
                    for(i = 0;i < framestoget;i++){
                        /* read one stereo frame*/
                        got = pvoc_getframes(pdata->pvfile,pdata->analframe1,1);
                        if(got == -1) {
                            fprintf(stderr,"\nError reading ch 1 from pvx file\n");
                            file_playing  = 0;
                            break;
                        }
                        if(got){
                            got = pvoc_getframes(pdata->pvfile,pdata->analframe2,1);
                            if(got == -1) {
                                fprintf(stderr,"\nError reading ch 2 from pvx file, frame %lu\n",framepos);
                                file_playing  = 0;
                                break;
                            }
                        }
                        framepos++;
                        if(got && (framepos <= pdata->to_frame)) {
                            // each call returns <overlap> samples
                            long samps = pdata->pv_l->process_frame(pdata->analframe1,pbuf,PVOC_AMP_FREQ);
                            pbuf += samps;
                            samps = pdata->pv_r->process_frame(pdata->analframe2,pbuf_r,PVOC_AMP_FREQ);
                            pbuf_r += samps;
                            
                            elementstowrite += samps; // element is a stereo sample frame
                        }
                        if (got==0 || framepos >= pdata->to_frame){
                            if(pdata->play_looped) {
                                //pos = sndseekEx(pdata->ifd,pdata->from_frame * pdata->anal_framesize,SEEK_SET);
                                //if(pvoc_rewind(pdata->pvfile,1)){  /* 1 = skip empty frame */
                                if(pvoc_seek_mcframe(pdata->pvfile,pdata->from_frame,SEEK_SET)) {
                                    fprintf(stderr,"Error rewinding frame loop.\n");
                                    file_playing  = 0;
                                    break;
                                }
                            }
                            else if(elementsInBuffer == pdata->ringbuf.bufferSize)  {
                                //printf("buffer empty!\n");
                                pdata->flag = 1;
                                break;
                            }
                        }
                    }  // framestogeet
                    assert(elementstowrite <= elementsInBuffer);
                    //apply gain to inbuf first, then copy to ringbuf
                    pbuf = pdata->outbuf_l;
                    pbuf_r = pdata->outbuf_r;
                    stereo_interls(pbuf,pbuf_r,pdata->inbuf,elementstowrite);
                    
                    if(pdata->gain != 1.0) {
                        for(i=0;i < elementstowrite * pdata->inchans;i++)
                            pdata->inbuf[i]  = (float)(pdata->inbuf[i] * pdata->gain);
                    }
                    /* By using PaUtil_GetRingBufferWriteRegions, we can write directly into the ring buffer */
                    PaUtil_GetRingBufferWriteRegions(&pdata->ringbuf, elementsInBuffer, &ptr[0], &sizes[0], &ptr[1], &sizes[1]);
                    pbuf = pdata->inbuf;
                    //printf("ring buffer sizes : %d:%d",(int)sizes[0], (int)sizes[1]);
                    for(i = 0; i < 2; i++) {
                        if(ptr[i]) {
                            int frameswanted = sizes[i];
                            memcpy(ptr[i],pbuf,frameswanted * sizeof(float) * pdata->inchans);
                            pbuf += frameswanted * pdata->inchans;
                            nElementsProcessed += frameswanted;
                        }
                    }
                    if(elementstowrite)
                        PaUtil_AdvanceRingBufferWriteIndex(&pdata->ringbuf, elementstowrite);
                } // file_playing
                else {
                    pdata->flag = 1;
                    break;
                }
            }
        }
        /* Sleep a little while... */
        Pa_Sleep(5);
    }
    return 0;
}
