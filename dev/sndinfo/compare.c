/*
 * Copyright (c) 1983-2013 Trevor Wishart and Composers Desktop Project Ltd
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



#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <cdpmain.h>
#include <sndinfo.h>
#include <processno.h>
#include <modeno.h>
#include <pnames.h>
#include <filetype.h>
#include <flags.h>
#include <float.h>
#include <time.h>
#include <sfsys.h>
#include <string.h>
#include <math.h>

#define SPACECNT    (32)

static int  try_header(int chans,double inverse_sr,dataptr dz);
static void make_time_display(char temp[],char timestr[],double secs);
static int  get_and_set_float_maxmina(float *flbuf,dataptr dz);
static int  get_and_set_float_maxmin(float *buf,int chans, double inverse_sr,dataptr dz);

static int  compare_sndfile_properties(char temp[],dataptr dz);
static int  compare_analfile_properties(dataptr dz);
static int  read_both_the_files(int samesize,dataptr dz);
static int  read_samps_for_cdiff(dataptr dz);
static void compare_samps(char temp[],int *badmatch,int *limitcnt,double threshold,dataptr dz);
static void anal_file_time(char temp[],int n,dataptr dz);
static void snd_file_time(char temp[],int n,dataptr dz);
static int  sfdiff_process(char temp[],int samesize,dataptr dz);
static int  compare_infile_sizes(int *same_size,dataptr dz);

static int  do_sfdiff(char temp[],dataptr dz);
static int  prntprops(SFPROPS *prop, dataptr dz);
static int  get_float_maxmin(float *buf,int chans, double inverse_sr,dataptr dz);
/*RWD 6:2001 */
static int getpeakdata(int ifd, float *peakval,SFPROPS *props);

static int do_zcross_ratio(dataptr dz);

/****************************** DO_SNDINFO ******************************/

int do_sndinfo(dataptr dz)
{
    int exit_status;
    float *flbuf;
    float *buf;
    double sr = (double)dz->infile->srate;
    int chans = dz->infile->channels, maxpos;
    double other_sr = (double)dz->otherfile->srate;
    int other_chans = dz->otherfile->channels;
    double inverse_sr = 1.0, secs;
    int thisspace, m;
    float *chanmax;
    int ftype = dz->infile->filetype;
    int is_a_text_file = FALSE;
    int n, k, j;
    fileptr fptr = dz->infile;
    char timestr[64];
    char temp[200];
    int holesize = 0, holesamp = 0;
    int maxholesize = 0;
    int maxholesamp = 0;
    double maxholelen, maxholetime;
    double threshold, level = 0.0;
    int orig_ifd0;
    SFPROPS props = {0};
    infileptr ifp;
    double maxamp, maxloc;
    int maxrep;
    int getmax = 0, getmaxinfo = 0;

    if(sr > 0.0)
        inverse_sr = 1.0/sr;
    switch(dz->process) {

    case(INFO_TIMESUM):
        secs = 0.0;
        for(n=0;n<dz->infilecnt;n++) {
            secs += (double)(dz->insams[n]/chans) * inverse_sr;
            secs -= dz->param[TIMESUM_SPLEN] * MS_TO_SECS;
        }
        sprintf(errstr,"TOTAL TIME ");
        make_time_display(temp,timestr,secs);
        strcat(errstr,timestr);
        strcat(errstr,"\n");
        print_outmessage(errstr);
        break;

    case(INFO_TIMELIST):    
        if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store data on file.\n");
            return(MEMORY_ERROR);
        }
        secs = (double)(dz->insams[0]/chans) * inverse_sr;
        sprintf(errstr,"%s",dz->wordstor[0]);
        thisspace = SPACECNT - strlen(dz->wordstor[0]);
        thisspace = max(thisspace,2);
        for(m=0;m<thisspace;m++)
            strcat(errstr,".");
        make_time_display(temp,timestr,secs);
        strcat(errstr,timestr);
        strcat(errstr,"\n");
        print_outmessage(errstr);
//TW UPDATE
        if(sloom) {
            sprintf(errstr,"%lf\n",secs);
            if(fputs(errstr,dz->fp)<0) {
                sprintf(errstr,"Cannot output data to file\n");
                return(SYSTEM_ERROR);
            }
        }

        for(n=1;n<dz->infilecnt;n++) {
            if((exit_status = readhead(ifp,dz->ifd[n],dz->wordstor[n],&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0) {
                sprintf(errstr,"Cannot re-read file %d header.\n",n+1);
                return(PROGRAM_ERROR);
            }
            copy_to_fileptr(ifp,dz->infile);
            secs = (double)(dz->insams[n]/dz->infile->channels)/(double)dz->infile->srate;
            sprintf(errstr,"%s",dz->wordstor[n]);
            thisspace = SPACECNT - strlen(dz->wordstor[n]);
            thisspace = max(thisspace,2);
            for(m=0;m<thisspace;m++)
                strcat(errstr,".");
            make_time_display(temp,timestr,secs);
            strcat(errstr,timestr);
            strcat(errstr,"\n");
            print_outmessage(errstr);
//TW UPDATE
            if(sloom) {
                sprintf(errstr,"%lf\n",secs);
                if(fputs(errstr,dz->fp)<0) {
                    sprintf(errstr,"Cannot output data to file\n");
                    return(SYSTEM_ERROR);
                }
            }

        }
        break;
    case(INFO_LOUDLIST):    
        orig_ifd0 = dz->ifd[0];
        for(m=0;m<dz->infilecnt;m++) {
            float fmaxamp = 0.0f;       /*RWD 6:2001 */
            int gotpeak = 0;
            /*RWD 6:2001 try to read PEAK chunk too! */
            if(getpeakdata(dz->ifd[m],&fmaxamp,&props)){
                level = fmaxamp;
                gotpeak = 1;
            }
            
            else {
                /* RWD impossible to report int values without lot of hoohaa */
                /* just look as if its floatsam file, for now */
                if(sndgetprop(dz->ifd[m],"fmaxamp",(char *)&fmaxamp,sizeof(float)) < 0 ) {
                    dz->samps_left = dz->insams[m];             
                    dz->ifd[0] = dz->ifd[m];
                    while(dz->samps_left) {
                        if((exit_status = read_samps(dz->bigbuf,dz))<0)
                            return(exit_status);
                        for(n=0;n<dz->ssampsread;n++)
                            fmaxamp = (float)max(fmaxamp,fabs(dz->bigbuf[n]));
                    }
                }
            }
            if(!gotpeak) /*RWD 6:2001 */                
                level = (double)fmaxamp/F_MAXSAMP;

            sprintf(errstr,"%s\t",dz->wordstor[m]);
            sprintf(timestr,"%lf\n",level);
            strcat(errstr,timestr);
            print_outmessage(errstr);
            if(fputs(timestr,dz->fp)<0) {
                sprintf(errstr,"Cannot output data to file\n");
                return(SYSTEM_ERROR);
            }
            dz->ifd[0] = orig_ifd0;
        }
        break;


    case(INFO_TIMEDIFF):
        sprintf(errstr,"DIFFERENCE IS ");
        secs  = (dz->insams[0]/chans) * inverse_sr;
        secs -= (dz->insams[1]/other_chans)/other_sr;
        secs  = fabs(secs);
        make_time_display(temp,timestr,secs);
        strcat(errstr,timestr);
        strcat(errstr,"\n");
        print_outmessage(errstr);
        break;
    
    case(INFO_SFLEN):
        switch(dz->infile->filetype) {
        case(SNDFILE):
            print_outmessage("A soundfile\n");
            sprintf(errstr,"DURATION: ");
            secs = (double)(dz->insams[0]/dz->infile->channels) * inverse_sr;
            make_time_display(temp,timestr,secs);
            strcat(errstr,timestr);
            sprintf(temp,"samples %d\n",dz->insams[0]);
            strcat(errstr,temp);
            break;
        case(ANALFILE):
            print_outmessage("An analysis data file\n");
            sprintf(errstr,"DURATION: ");
            secs = (double)dz->wlength * dz->frametime;
            make_time_display(temp,timestr,secs);
            strcat(errstr,timestr);
            sprintf(temp,"windows %d : floats %d\n",dz->wlength,dz->insams[0]);
            strcat(errstr,temp);
            break;
        case(PITCHFILE):
            print_outmessage("A binary pitch data file\n");
            sprintf(errstr,"DURATION: ");
            secs = (double)dz->insams[0] * dz->frametime;
            make_time_display(temp,timestr,secs);
            strcat(errstr,timestr);
            sprintf(temp,"windows %d : floats %d\n",dz->insams[0],dz->insams[0]);
            strcat(errstr,temp);
            break;
        case(TRANSPOSFILE):
            print_outmessage("A binary transposition data file\n");
            sprintf(errstr,"DURATION: ");
            secs = (double)dz->insams[0] * dz->frametime;
            make_time_display(temp,timestr,secs);
            strcat(errstr,timestr);
            sprintf(temp,"windows %d : floats %d\n",dz->insams[0],dz->insams[0]);
            strcat(errstr,temp);
            break;
        case(ENVFILE):
            print_outmessage("A binary envelope file\n");
            sprintf(errstr,"DURATION: ");
            secs = (double)dz->insams[0] * dz->infile->window_size * MS_TO_SECS;
            make_time_display(temp,timestr,secs);
            strcat(errstr,timestr);
            sprintf(temp,"windows %d : floats %d\n",dz->wlength,dz->insams[0]);
            strcat(errstr,temp);
            break;
        case(FORMANTFILE):
            print_outmessage("A formant data file\n");
            sprintf(errstr,"DURATION: ");
            dz->wlength  = (dz->insams[0]/dz->specenvcnt) - DESCRIPTOR_DATA_BLOKS;
            secs = (double)dz->wlength * dz->frametime;
            make_time_display(temp,timestr,secs);
            strcat(errstr,timestr);
            n = dz->insams[0] - (dz->specenvcnt * DESCRIPTOR_DATA_BLOKS);
            sprintf(temp,"windows %d : floats %d\n",dz->wlength,n);
            strcat(errstr,temp);
            break;
        default:
            sprintf(errstr,"This process only works with soundfiling-system files.\n");
            return(DATA_ERROR);
        }
        print_outmessage(errstr);
        break;

    case(INFO_LOUDCHAN):
        if(dz->infile->channels==MONO) {
            sprintf(errstr,"This program does not works with MONO files.\n");
            return(GOAL_FAILED);
        }
        if((chanmax = (float *)malloc(chans *sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store channel maximi.\n");
            return(MEMORY_ERROR);
        }
        for(m=0;m<chans;m++)
            chanmax[m] = 0;
        while(dz->samps_left) {
            if((exit_status = read_samps(dz->bigbuf,dz))<0)
                return(exit_status);
            for(n=0;n<dz->ssampsread;n+=chans) {
                for(m=0;m<chans;m++)
                    chanmax[m] = (float)max(chanmax[m],(float) fabs(dz->bigbuf[n+m]));
            }
        }
        maxpos = 0;
        for(m=1;m<chans;m++) {
            if(chanmax[m] > chanmax[maxpos])
                maxpos = m;
        }
        sprintf(errstr,"LOUDEST CHANNEL is %d\n",maxpos+1);                         
        print_outmessage(errstr);
        free(chanmax);
        break;

    case(INFO_SAMPTOTIME):
        if(!dz->vflag[CHAN_GROUPED])
            dz->iparam[INFO_SAMPS] /= chans;
        secs = dz->iparam[INFO_SAMPS] * inverse_sr;
        make_time_display(temp,timestr,secs);
        sprintf(errstr,"TIME %s",timestr);
        strcat(errstr,"\n");
        print_outmessage(errstr);
        break;

    case(INFO_TIMETOSAMP):
        n = round(dz->param[INFO_TIME] * sr);
        if(!dz->vflag[CHAN_GROUPED])
            n *= chans;
        sprintf(errstr,"SAMPLE %d\n",n);
        print_outmessage(errstr);
        break;

    case(INFO_PROPS):
        errstr[0] = ENDOFSTR;
        switch(ftype) {
        case(SNDFILE):      
            sprintf(errstr,"A SOUND file.\n");                  break;
        case(ANALFILE): 
            sprintf(errstr,"An ANALYSIS file.\n");              break;
        case(PITCHFILE):
            sprintf(errstr,"A binary PITCH file.\n");           break;
        case(TRANSPOSFILE): 
            sprintf(errstr,"A binary TRANSPOSITION file.\n");   break;
        case(FORMANTFILE):
            sprintf(errstr,"A FORMANT data file.\n");           break;
        case(ENVFILE):  
            sprintf(errstr,"A binary ENVELOPE file.\n");        break;
        default:            
            if(is_a_textfile_type(ftype))    {
                sprintf(errstr,"A TEXT data file.\n");              
                is_a_text_file = TRUE;
            } else {                                
                print_outmessage("UNKNOWN input file type.\n");
                return(FINISHED);
            }
            break;
        }
        print_outmessage(errstr);
        if(is_a_text_file) {
            sprintf(errstr,"Contains %d LINES\n",dz->linecnt);
            print_outmessage(errstr);
            sprintf(errstr,"Contains %d WORDS\n",dz->all_words);
            print_outmessage(errstr);
            if(ftype==MIXFILE) {
                sprintf(errstr,"Could be a MIXFILE with output channel cnt %d\n",dz->out_chans);
                print_outmessage(errstr);
            } else if(ftype==SYNCLIST) {
                sprintf(errstr,"Could be a LIST OF SOUNDFILES TO SYNCHRONISE.\n");
                print_outmessage(errstr);
            } else if(ftype==SNDLIST) {
                sprintf(errstr,"Could be a LIST OF SOUNDFILES.\n");
                print_outmessage(errstr);
            }
            if(dz->tempsize >0 || (dz->extrabrkno >= 0  && dz->brksize[dz->extrabrkno] > 0)) {
                sprintf(errstr,"Could be a BREAKPOINT FILE:\n");
                print_outmessage(errstr);
                sprintf(errstr,"length %d minval %lf maxval %lf  duration %lf\n",
                    dz->numsize/2,dz->minbrk,dz->maxbrk,dz->duration);
                print_outmessage(errstr);
            }
            if(dz->numsize > 0) {
                sprintf(errstr,"Could be a NUMBER LIST:\n");
                print_outmessage(errstr);
                sprintf(errstr,"length %d minval %lf maxval %lf\n",dz->numsize,dz->minnum,dz->maxnum);
                print_outmessage(errstr);
            }
        } else {
            sprintf(errstr,"samples: ............ %d\n",dz->insams[0]);
            print_outmessage(errstr);
            if(ftype!=SNDFILE) {
                switch(ftype) {
                case(ANALFILE): 
                    sprintf(errstr,"file type: .......... ANALYSIS DATA\n");                break;
                case(PITCHFILE):
                    sprintf(errstr,"file type: .......... PITCH DATA (binary)\n");          break;
                case(TRANSPOSFILE): 
                    sprintf(errstr,"file type: .......... TRANSPOSITION DATA (binary)\n");  break;
                case(FORMANTFILE):
                    sprintf(errstr,"file type: .......... FORMANT DATA (binary)\n");        break;
                case(ENVFILE):  
                    sprintf(errstr,"file type: .......... ENVELOPE DATA (binary)\n");       break;
                }
                print_outmessage(errstr);
//TW UPDATE: moved srate & chans, as ENVFILES have srate and channels
                sprintf(errstr,"sample rate: ........ %d\n",fptr->srate);
                print_outmessage(errstr);
                sprintf(errstr,"channels: ........... %d\n",fptr->channels);
                print_outmessage(errstr);
                if(fptr->filetype==ENVFILE) {
                    sprintf(errstr,"window size: ........ %f ms\n",fptr->window_size);
                    print_outmessage(errstr);
                    return(FINISHED);               
                }
                if(fptr->filetype==FORMANTFILE) {
                    sprintf(errstr,"formant envelope cnt: %d\n",fptr->specenvcnt);
                    print_outmessage(errstr);
                }
                /*RWD*/
//TW NOW REDUNDANT: flagging changed
//              if(fptr->filetype!=SNDFILE){
                sprintf(errstr,"original sampsize: .. %d\n",fptr->origstype);
                print_outmessage(errstr);
                sprintf(errstr,"original sample rate: %d\n",fptr->origrate);
                print_outmessage(errstr);
                sprintf(errstr,"analysis rate: ...... %f\n",fptr->arate);
                print_outmessage(errstr);
                sprintf(errstr,"analysis window len:  %d\n",fptr->Mlen);
                print_outmessage(errstr);
                sprintf(errstr,"decimation factor: .. %d\n",fptr->Dfac);
                print_outmessage(errstr);
//              }
                if(fptr->filetype==PITCHFILE || fptr->filetype==TRANSPOSFILE) {
                    sprintf(errstr,"original channels: .. %d\n",fptr->origchans);
                    print_outmessage(errstr);
                }
            } else {
                sprintf(errstr,"file type: ........... SOUND\n");
                print_outmessage(errstr);
                sprintf(errstr,"sample rate: ........ %d\n",fptr->srate);
                print_outmessage(errstr);
                sprintf(errstr,"channels: ........... %d\n",fptr->channels);
                print_outmessage(errstr);
            }
            if((exit_status = prntprops(&props,dz))<0)
                return(exit_status);
        }
        break;
    case(INFO_MAXSAMP):
        if(is_a_text_input_filetype(dz->infile->filetype)) {
            sprintf(errstr,"This is a text file.\n");
            return(GOAL_FAILED);
        }
        /* RWD 2022 this hangs if user supplies anal file */
        if(dz->infile->filetype == ANALFILE){
            sprintf(errstr,"This is an anlysis file.\n");
            return(GOAL_FAILED);
        }
        flbuf = (float *)dz->bigbuf;
        buf   = dz->bigbuf;
        if(!dz->vflag[FORCE_SCAN] && try_header(chans,inverse_sr,dz)==FINISHED)
            break;
        else {
            if(dz->infile->filetype != SNDFILE)  {
                if((exit_status = get_and_set_float_maxmina(flbuf,dz))<0)
                        return(exit_status);                                
            } else {
                if((exit_status = get_and_set_float_maxmin(buf,chans,inverse_sr,dz))<0)
                    return(exit_status);
            }
        }
        break;
    case(INFO_MAXSAMP2):
        buf   = dz->bigbuf;
        if((exit_status = get_float_maxmin(buf,chans,inverse_sr,dz))<0)
            return(exit_status);
        break;
    
    case(INFO_PRNTSND):
        dz->ssampsread = 0;
        dz->iparam[PRNT_START] = round(dz->param[PRNT_START] * sr) * chans;
        dz->iparam[PRNT_END]   = round(dz->param[PRNT_END] * sr)  * chans;
        if(dz->iparam[PRNT_START] >= dz->iparam[PRNT_END]) {
            sprintf(errstr,"Incompatible start and end times.\n");
            return(DATA_ERROR);
        }
        if(dz->iparam[PRNT_START] >= dz->insams[0]) {
            sprintf(errstr,"Start time is at end of file: can't proceed.\n");
            return(DATA_ERROR);
        }
        
        dz->total_samps_read = 0;
        for(;;) {
            if((exit_status = read_samps(dz->bigbuf,dz))<0)
                return(exit_status);
            if(dz->iparam[PRNT_START] > dz->ssampsread) {
                dz->iparam[PRNT_START] -= dz->ssampsread;
                dz->iparam[PRNT_END]   -= dz->ssampsread;
            } else
                break;
        }
        k = 0;
        j = (dz->total_samps_read - dz->ssampsread + dz->iparam[PRNT_START])/chans;
        display_virtual_time(0L,dz);
        for(n=dz->iparam[PRNT_START];n<dz->iparam[PRNT_END];n+=chans,k+=chans,j++) {
            fprintf(dz->fp,"[%d]\t",j);
            for(m=0;m<chans;m++)
                fprintf(dz->fp,"%.6f\t",(float) dz->bigbuf[n+m]);               /*RWD 12:2003 was %d */
            fprintf(dz->fp,"\n");
            if(n >= dz->ssampsread) {
                if((exit_status = read_samps(dz->bigbuf,dz))<0)
                    return(exit_status);
//TW CHANGED to samps
                display_virtual_time(k,dz);
                dz->iparam[PRNT_END] -= dz->ssampsread;
                n = 0;
            }
        }
//TW CHANGED to samps
        display_virtual_time(k,dz);
        break;
    case(INFO_FINDHOLE):
        threshold = dz->param[HOLE_THRESH] * (double)F_MAXSAMP;
        while(dz->samps_left) {
            if((exit_status = read_samps(dz->bigbuf,dz))<0)
                return(exit_status);
            for(n = 0; n < dz->ssampsread; n++)  {
                if(holesize==0) {
                    if(fabs(dz->bigbuf[n])<threshold) {
                        holesize = 1;
                        holesamp = dz->total_samps_read - dz->ssampsread + n;
                    }
                } else {
                    if(fabs(dz->bigbuf[n])<threshold)
                        holesize++;
                    else {
                        if(holesize > maxholesize) {
                            maxholesize = holesize;
                            maxholesamp = holesamp;
                        }
                        holesize = 0;
                    }
                }
            }
        }
        maxholelen  = (double)(maxholesize/chans) * inverse_sr;
        maxholetime = (double)(maxholesamp/chans) * inverse_sr;
        sprintf(errstr,"Maximum holesize is %.6lf at time %.6lf\n",maxholelen,maxholetime);
        print_outmessage(errstr);
        break;
    case(INFO_CDIFF):
    case(INFO_DIFF):
        if((exit_status = do_sfdiff(temp,dz))<0)
            return(exit_status);
        break;
    case(ZCROSS_RATIO):
        if((exit_status = do_zcross_ratio(dz))<0)
            return(exit_status);
        break;
    default:
        sprintf(errstr,"Unknown process in do_sndinfo()\n");
        return(PROGRAM_ERROR);
    }
    fflush(stdout);
    return(FINISHED);
}

/************************** TRY_HEADER ************************/

int try_header(int chans,double inverse_sr,dataptr dz)
{
    int pos_repeats, neg_repeats, repeats, maxloc, maxamp = -1;
    float maxpfamp, maxnfamp;
    double gain, dbgain;
    float newmaxamp = 0.0f;
    if(dz->infile->filetype != SNDFILE) {
//TW SUBSTITUTED SNDgetprop
        if( sndgetprop(dz->ifd[0],"maxpfamp",(char *)&maxpfamp,sizeof(float)) < 0 )
            return(CONTINUE);
        if( sndgetprop(dz->ifd[0],"maxnfamp",(char *)&maxnfamp,sizeof(float)) < 0 )
            return(CONTINUE);
        if(sndgetprop(dz->ifd[0],"maxprep",(char *)&pos_repeats,sizeof(float)) < 0 )
            return(CONTINUE);
        if(sndgetprop(dz->ifd[0],"maxnrep",(char *)&neg_repeats,sizeof(int)) < 0 )
            return(CONTINUE);
        sprintf(errstr,"maximum float value: %f\n",maxpfamp);
        print_outmessage(errstr);
        sprintf(errstr,"occurs: ............ %d times\n",pos_repeats);
        print_outmessage(errstr);
        sprintf(errstr,"minimum float value: %f\n",maxnfamp);
        print_outmessage(errstr);
        sprintf(errstr,"occurs: ............ %d times\n",neg_repeats);
        print_outmessage(errstr);
    } else {
        if( sndgetprop(dz->ifd[0],"maxamp",(char *)&maxamp,sizeof(int)) < 0         /* old files */
        &&  sndgetprop(dz->ifd[0],"maxpfamp",(char *)&newmaxamp,sizeof(float)) < 0) /* new files */
            return(CONTINUE);
        if( sndgetprop(dz->ifd[0],"maxloc",(char *)&maxloc,sizeof(int)) < 0 )
            return(CONTINUE);
        if( sndgetprop(dz->ifd[0],"maxrep",(char *)&repeats,sizeof(int)) < 0 )
            return(CONTINUE);
        if(maxamp>=0)
            newmaxamp = (float)(maxamp/(double)MAXSAMP);
        sprintf(errstr,"maximum abs value:.. %f\n",newmaxamp);
        print_outmessage(errstr);
        sprintf(errstr,"at: ................ %d samples\n",maxloc);
        print_outmessage(errstr);
        sprintf(errstr,"time: .............. %.4lf secs\n",(double)(maxloc/chans) *inverse_sr);
        print_outmessage(errstr);
        sprintf(errstr,"repeated: .......... %d times\n",repeats);
        print_outmessage(errstr);
        if(newmaxamp>=0.0) {
            gain   = 1.0/(double)maxamp;
            dbgain = log10(gain) * 20.0;
            sprintf(errstr,"max possible gain:.. %.4lf\n",gain);
            print_outmessage(errstr);
            sprintf(errstr,"max dB gain:........ %.4lf\n",dbgain);
            print_outmessage(errstr);
        }
    }
    return(FINISHED);
}

/************************** MAKE_TIME_DISPLAY ************************/

void make_time_display(char temp[],char timestr[],double secs)
{
    int mins = 0, hrs = 0;
    if(secs > 60.0) {
        mins  = round(floor(secs/60.0));
        secs -= (double)(mins * 60);
    }
    if(mins > 60) {
        hrs   = round(floor((double)mins/60.0));
        mins -= hrs * 60;
    }           
    timestr[0] = ENDOFSTR;
    if(hrs) {       
        sprintf(temp,"%d hrs ",hrs);
        strcat(timestr,temp);
    }
    if(mins) {
        sprintf(temp,"%d mins ",mins);
        strcat(timestr,temp);
    }
    sprintf(temp,"%.6lf secs ",secs);
    strcat(timestr,temp);
}

/************************** GET_AND_SET_FLOAT_MAXMIN ************************/
/* RWD assumes analysis ?*/
int get_and_set_float_maxmina(float *flbuf,dataptr dz)
{
    int exit_status;
    double maxpdamp = DBL_MIN;
    double maxndamp = DBL_MAX;
    float maxpfamp, maxnfamp;
    int   maxprep = 1, maxnrep = 1;
    int n;
    dz->samps_left = dz->insams[0];
    if(sndseekEx(dz->ifd[0],0,0)<0) {
        sprintf(errstr,"sndseek() failed. get_and_set_float_maxmin()\n");
        return(SYSTEM_ERROR);
    }
    while(dz->samps_left > 0) {
        if((exit_status = read_samps(flbuf,dz))<0)
            return(exit_status);
        for(n=0;n<dz->ssampsread;n++) {
            if(flbuf[n] < maxpdamp)     /* IF < max, ignore */
                ; 
            else if(flbuf[n] > maxpdamp) {  
                maxpdamp = flbuf[n];    /* IF > max, reset max and count */
                maxprep  = 1;
            } else                      /* else must be EQUAL to max; incr counter */
                maxprep++;
    
            if(flbuf[n] > maxndamp)
                ; 
            else if(flbuf[n] < maxndamp) {
                maxndamp = flbuf[n];
                maxnrep  = 1;
            } else
                maxnrep++;
        }
    }
    maxpfamp = (float)maxpdamp;
    maxnfamp = (float)maxndamp;
//TW cannot reset header of a read-in file
//  if( sndputprop(dz->ifd[0],"maxpfamp",(char *)&maxpfamp,sizeof(float)) < 0 )
//      fprintf(stdout,"WARNING: Cannot write maximum float value to header\n");
//  if(sndputprop(dz->ifd[0],"maxprep",(char *)&maxprep,sizeof(long)) < 0 )
//      fprintf(stdout,"WARNING: Cannot write maxfloat repeats-count to header\n");
//  if( sndputprop(dz->ifd[0],"maxnfamp",(char *)&maxnfamp,sizeof(float)) < 0 )
//      fprintf(stdout,"WARNING: Cannot write mimimum float value to header\n");
//  if(sndputprop(dz->ifd[0],"maxnrep",(char *)&maxnrep,sizeof(long)) < 0 )
//      fprintf(stdout,"WARNING: Cannot write minfloat repeats-count to header\n");

    sprintf(errstr,"maximum float value: %f\n",maxpfamp);
    print_outmessage(errstr);
    sprintf(errstr,"occurs: ...........  %d times\n",maxprep);
    print_outmessage(errstr);
    sprintf(errstr,"minimum float value: %f\n",maxnfamp);
    print_outmessage(errstr);
    sprintf(errstr,"occurs: ...........  %d times\n",maxnrep);
    print_outmessage(errstr);
    return(FINISHED);
}

/************************** GET_AND_SET_SHORT_MAXMIN ************************/

int get_and_set_float_maxmin(float *buf,int chans, double inverse_sr,dataptr dz)
{
    int exit_status;
    /*int maxamp = MINSAMP, mag;*/
    float maxamp = F_MINSAMP, mag;
    int maxloc = 0;
    int maxrep = 1;
    int n;
    double gain = 0.0, dbgain = 0.0;
    while(dz->samps_left) {
        if((exit_status = read_samps(buf,dz))<0)
            return(exit_status);
        for(n=0;n<dz->ssampsread;n++) {
            mag = (float) fabs(buf[n]);
            if(mag > maxamp) {
                maxloc = dz->total_samps_read - dz->ssampsread + n;
                maxamp = mag;
                maxrep = 1;
            } else if(mag==maxamp)
                maxrep++;
        }
    }
//TW Cannot alter heqader of an input file
//  if(sndputprop(dz->ifd[0],"maxamp",(char *)&maxamp,sizeof(float)) < 0 )
//  if(sndputprop(dz->ifd[0],"maxpfamp",(char *)&maxamp,sizeof(float)) < 0 )
//      fprintf(stdout,"WARNING: Cannot write maximum value to header.\n");
//  if(sndputprop(dz->ifd[0],"maxloc",(char *)&maxloc,sizeof(long)) < 0 )
//      fprintf(stdout,"WARNING: Cannot write location of maximum value to header.\n");
//  if(sndputprop(dz->ifd[0],"maxrep",(char *)&maxrep,sizeof(long)) < 0 )
//      fprintf(stdout,"WARNING: Cannot write maximum repeats to header.\n");

    if(maxamp>0.0f) {
        gain   = F_MAXSAMP/(double)maxamp;
        dbgain = log10(gain) * 20.0;
    }
    sprintf(errstr,"maximum abs value:.. %f\n",maxamp);
    print_outmessage(errstr);
    sprintf(errstr,"at: ................ %d samples\n",maxloc);
    print_outmessage(errstr);
    sprintf(errstr,"time: .............. %.4lf secs\n",(double)(maxloc/chans) *inverse_sr);
    print_outmessage(errstr);
    sprintf(errstr,"repeated: .......... %d times\n",maxrep);
    print_outmessage(errstr);
    if(maxamp>0.0f) {
        sprintf(errstr,"max possible gain:.. %.4lf\n",gain);
        print_outmessage(errstr);
        sprintf(errstr,"max dB gain:........ %.4lf\n",dbgain);
        print_outmessage(errstr);
    }
    return(FINISHED);
}

/************************ COMPARE_INFILE_SIZES ***********************/

int compare_infile_sizes(int *samesize,dataptr dz)
{   
    if(dz->insams[0] != dz->insams[1]) {
        print_outmessage_flush("Files are not same size.\n");
        if(!dz->vflag[IGNORE_LENDIFF])
            return(FINISHED);
        else {
            dz->insams[0] = min(dz->insams[0],dz->insams[1]);
            *samesize = FALSE;  
        }
    }
    dz->samps_left = dz->insams[0];
    return(CONTINUE);
}

/************************** COMPARE_SNDFILE_PROPERTIES *********************/

int compare_sndfile_properties(char temp[],dataptr dz)
{
    errstr[0] = ENDOFSTR;
    if(dz->infile->srate != dz->otherfile->srate) {
        sprintf(errstr,"Files don't have same sample rate.\n");
        return(FINISHED);
    }
    if(dz->infile->channels != dz->otherfile->channels) {
        sprintf(errstr,"Files don't have same number of channels. ");
        if(dz->vflag[IGNORE_CHANDIFF]) {
            sprintf(temp,"1st has %d : 2nd has %d\n",dz->infile->channels,dz->otherfile->channels);
            strcat(errstr,temp);
        } else {
            strcat(errstr,"\n");
            return(FINISHED);
        }
        print_outmessage(errstr);
    }
    return(CONTINUE);
}

/*************************** SFDIFF_PROCESS **************************/

int sfdiff_process(char temp[],int samesize,dataptr dz)
{
    int exit_status;
    int limitcnt = 0, badmatch = 0;
    double threshold = dz->param[SFDIFF_THRESH] /* * (double)MAXSAMP*/;

    display_virtual_time(0L,dz);
    if(sndseekEx(dz->ifd[0],0,0)<0) {
        sprintf(errstr,"sndseekEx() problem in sfdiff_process()\n");
        return(SYSTEM_ERROR);
    }
    if(dz->process==INFO_DIFF && sndseekEx(dz->ifd[1],0,0)<0) {
        sprintf(errstr,"sndseekEx() problem 2 in sfdiff_process()\n");
        return(SYSTEM_ERROR);
    }
    while(dz->samps_left > 0) {
        if(dz->process==INFO_CDIFF) {
            if((exit_status = read_samps_for_cdiff(dz))<0)
                return(exit_status);
        } else {
            if((exit_status = read_both_the_files(samesize,dz))<0)
                return(exit_status);
        }
        compare_samps(temp,&badmatch,&limitcnt,threshold,dz);
        display_virtual_time(dz->total_samps_read,dz);
        fflush(stdout);
    }
    if(limitcnt == 0) {
        if(dz->process==INFO_CDIFF)
            sprintf(errstr,"Channels are IDENTICAL");
        else {
            if(samesize)
                sprintf(errstr,"Files are IDENTICAL");
            else
                sprintf(errstr,"Files are otherwise IDENTICAL");
        }
        if(dz->process==INFO_CDIFF) {
            if(dz->param[SFDIFF_THRESH] > 0) {
                sprintf(temp," to within %lf\n",dz->param[SFDIFF_THRESH]);
                strcat(errstr,temp);
            }else 
                strcat(errstr,"\n");
        } else {
            switch(dz->infile->filetype) {
            case(SNDFILE):
                if(dz->param[SFDIFF_THRESH] > 0) {
                    sprintf(temp," to within %lf\n",dz->param[SFDIFF_THRESH]);
                    strcat(errstr,temp);
                } else 
                    strcat(errstr,"\n");
                break;
            default:
                if(dz->param[SFDIFF_THRESH] > 0.0) {
                    sprintf(temp," to within %lf\n",dz->param[SFDIFF_THRESH]);
                    strcat(errstr,temp);
                } else
                    strcat(errstr,"\n");
                break;
            }
        }

    } else if(badmatch) {
        if(limitcnt < dz->iparam[SFDIFF_CNT])
            strcat(errstr," : as far as end of file.\n");
        else
            strcat(errstr," : comparison stopped here.\n");
    }
    return(FINISHED);
}

/************************ READ_BOTH_THE_FILES *******************************/

int read_both_the_files(int samesize,dataptr dz)
{   
    int samps_read2;
    if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], /*dz->bigbufsize*/dz->buflen,dz->ifd[0],0)) < 0) {
        sprintf(errstr, "Can't read samps from 1st soundfile\n");
        return(SYSTEM_ERROR);
    }
    if((samps_read2 = fgetfbufEx(dz->sampbuf[1], dz->buflen,dz->ifd[1],0)) < 0) {
        sprintf(errstr, "Can't read samps from 2nd soundfile\n");
        return(SYSTEM_ERROR);
    }
    if((dz->ssampsread != samps_read2)) {
        if(samesize) {
            sprintf(errstr, "Anomaly in soundfile read.\n");
            return(PROGRAM_ERROR);
        } else
            dz->ssampsread = min(dz->ssampsread,samps_read2);
    }
    dz->total_samps_read += dz->ssampsread;
    dz->samps_left       -= dz->ssampsread;
    return(FINISHED);
}

/************************ READ_SAMPS_FOR_CDIFF *******************************/

int read_samps_for_cdiff(dataptr dz)
{   
    int n, m;
    if((dz->ssampsread = fgetfbufEx(dz->sampbuf[2], dz->buflen * 2,dz->ifd[0],0)) < 0) {
        sprintf(errstr, "Can't read samples from soundfile\n");
        return(SYSTEM_ERROR);
    }
    
    for(n=0,m=0;m<dz->ssampsread;n++,m+=2) {
        dz->sampbuf[0][n] = dz->sampbuf[2][m];
        dz->sampbuf[1][n] = dz->sampbuf[2][m+1];
    }
    dz->total_samps_read += dz->ssampsread;
    dz->samps_left       -= dz->ssampsread;
    return(FINISHED);
}

/**************************** COMPARE_SAMPS ***************************/

void compare_samps(char temp[],int *badmatch,int *limitcnt,double threshold,dataptr dz)
{   
    int   n;
    int   sampsread = dz->ssampsread;
    switch(dz->infile->filetype) {
    case(SNDFILE):
        if(dz->process==INFO_CDIFF)
            sampsread /= STEREO;
        for(n=0;n<sampsread;n++) {
            if(fabs(dz->sampbuf[0][n]-dz->sampbuf[1][n]) > threshold) { 
                if(*badmatch==0) {
                    sprintf(errstr,"Badmatch from ");
                    snd_file_time(temp,n,dz);
                    *badmatch = 1;
                    if(++(*limitcnt) >= dz->iparam[SFDIFF_CNT]) {
                        dz->samps_left = 0;  /* force calling loop to end */
                        return;
                    }
                }
            } else {
                if(*badmatch) {
                    strcat(errstr," as far as ");
                    snd_file_time(temp,n,dz);
                    strcat(errstr,"\n");
                    print_outmessage(errstr);
                    *badmatch = 0;
                }
            }
        }
        break;
    case(ANALFILE):
        dz->ssampsread = sampsread;
        for(n=0;n<dz->ssampsread;n+=2) {
            if(fabs(dz->flbufptr[0][n]-dz->flbufptr[1][n]) > threshold) {   
                if(*badmatch==0) {
                    sprintf(errstr,"Bad frq match from ");
                    anal_file_time(temp,n,dz);
                    *badmatch = 1;
                    if(++(*limitcnt) >= dz->iparam[SFDIFF_CNT]) {
                        dz->samps_left = 0;
                        return;
                    }
                }
            } else {
                if(*badmatch) {
                    strcat(errstr," as far as ");
                    anal_file_time(temp,n,dz);
                    strcat(errstr,"\n");
                    print_outmessage(errstr);
                    *badmatch = 0;
                }
            }
        }       
        if(*badmatch) {
            strcat(errstr," as far as ");
            anal_file_time(temp,n,dz);
            strcat(errstr,"\n");
            print_outmessage(errstr);
            *badmatch = 0;
        }
        for(n=1;n<dz->ssampsread;n+=2) {
            if(fabs(dz->flbufptr[0][n]-dz->flbufptr[1][n]) > threshold) {   
                if(*badmatch==0) {
                    sprintf(errstr,"Bad amp match from ");
                    anal_file_time(temp,n,dz);
                    *badmatch = 1;
                    if(++(*limitcnt) >= dz->iparam[SFDIFF_CNT]) {
                        dz->samps_left = 0;
                        return;
                    }
                }
            } else {
                if(*badmatch) {
                    strcat(errstr," as far as ");
                    anal_file_time(temp,n,dz);
                    strcat(errstr,"\n");
                    print_outmessage(errstr);
                    *badmatch = 0;
                }
            }
        }       
        if(*badmatch) {
            strcat(errstr," to buffer end ");
            anal_file_time(temp,n,dz);
            strcat(errstr,"\n");
            print_outmessage(errstr);
            *badmatch = 0;
        }
        break;
    default:    /* float files */
        dz->ssampsread = sampsread;
        for(n=0;n<dz->ssampsread;n++) {
            if(fabs(dz->flbufptr[0][n]-dz->flbufptr[1][n]) > threshold) {   
                if(*badmatch==0) {
                    sprintf(errstr,"Badmatch from ");
                    anal_file_time(temp,n,dz);
                    *badmatch = 1;
                    if(++(*limitcnt) >= dz->iparam[SFDIFF_CNT]) {
                        dz->samps_left = 0;
                        return;
                    }
                }
            } else {
                if(*badmatch) {
                    strcat(errstr," as far as ");
                    anal_file_time(temp,n,dz);
                    strcat(errstr,"\n");
                    print_outmessage(errstr);
                    *badmatch = 0;
                }
            }
        }       
        break;
    }
}

/***************************** ANAL_FILE_TIME *************************/

void anal_file_time(char temp[],int n,dataptr dz)
{   
    int this_channel;
    double this_time;
    int  wanted;
    int this_sample = (dz->total_samps_read - dz->ssampsread) + n;
    int this_window;
    switch(dz->infile->filetype) {
    case(ANALFILE):
        wanted = dz->infile->Mlen + 2;
        this_window  = this_sample/wanted;   /* TRUNCATE */
        this_sample  = this_sample%wanted;   /* sample count from window base */
        this_channel = this_sample/2;
        this_time    = (double)this_window/(double)dz->infile->arate;
        sprintf(temp,"  channel %d: window %d: time %.4lf ",this_channel,this_window,this_time);
        break;
    case(PITCHFILE):
    case(TRANSPOSFILE):
        this_window = this_sample;
        this_time   = (double)this_window/(double)dz->infile->arate;
        sprintf(temp,"  window %d: time %.4lf ",this_window,this_time);
        break;
    case(ENVFILE):
        this_window = this_sample;
        this_time   = (double)this_window * dz->infile->window_size * MS_TO_SECS;
        sprintf(temp,"  window %d: time %.4lf ",this_window,this_time);
        break;
    case(FORMANTFILE):
        this_sample -= dz->specenvcnt * DESCRIPTOR_DATA_BLOKS;
        this_window  = this_sample/dz->specenvcnt; /* TRUNCATE */
        this_channel = this_sample%dz->specenvcnt;
        this_time   = (double)this_window/(double)dz->infile->arate;
        sprintf(temp,"  channel %d: window %d: time %.4lf ",this_channel,this_window,this_time);
        break;
    }
    strcat(errstr,temp);
}

/**************************** SND_FILE_TIME ***************************/

void snd_file_time(char temp [],int n,dataptr dz)
{   
    int   this_sample;
    double this_time, sr = (double)dz->infile->srate;
    if(dz->process==INFO_CDIFF) {
        this_sample = ((dz->total_samps_read - dz->ssampsread)/STEREO) + n;
        this_time   = (double)this_sample/sr;
    } else {
        this_sample = (dz->total_samps_read - dz->ssampsread) + n;
        this_time   = (double)this_sample/(sr * (double)dz->infile->channels);
    }
    sprintf(temp,"sample %d: time %.4lf ",this_sample,this_time);
    strcat(errstr,temp);
}

/**************************** COMPARE_ANALFILE_PROPERTIES ***************************/

int compare_analfile_properties(dataptr dz)
{
    errstr[0] = ENDOFSTR;
    switch(dz->infile->filetype) {
    case(ENVFILE):
        if(dz->infile->window_size != dz->otherfile->window_size) {
            sprintf(errstr,"Files do not have same WINDOW_SIZE.\n");
            return(FINISHED);
        }
        break;
    case(FORMANTFILE):
        if(dz->infile->specenvcnt != dz->otherfile->specenvcnt) {
            sprintf(errstr,"Files do not have same FORMANT WINDOW CNT.\n");
            return(FINISHED);
        }
        break;
    case(PITCHFILE):
    case(TRANSPOSFILE):
        if(dz->infile->origchans != dz->otherfile->origchans) {
            sprintf(errstr,"Files do not have same ORIGINAL NO. OF CHANNELS.\n");
            return(FINISHED);
        }
        break;
    }
    if(dz->infile->stype != dz->otherfile->stype) {
        sprintf(errstr,"Files do not have same SAMPLE TYPE.\n");
        return(FINISHED);
    }
    if(dz->infile->srate != dz->otherfile->srate) {
        sprintf(errstr,"Files do not have same SAMPLING RATE.\n");
        return(FINISHED);
    }
    if(dz->infile->channels != dz->otherfile->channels) {
        sprintf(errstr,"Files do not have same NO. OF CHANNELS.\n");
        return(FINISHED);
    }
    if(dz->infile->filetype!=ENVFILE) {
        if(dz->infile->origstype != dz->otherfile->origstype) {
            sprintf(errstr,"Files do not have same ORIGINAL SAMPLE TYPE.\n");
            return(FINISHED);
        }
        if(dz->infile->origrate != dz->otherfile->origrate) {
            sprintf(errstr,"Files do not have same ORIGINAL SAMPLING RATE.\n");
            return(FINISHED);
        }
        if(dz->infile->arate != dz->otherfile->arate) {
            sprintf(errstr,"Files do not have same ANALYSIS RATE.\n");
            return(FINISHED);
        }
        if(dz->infile->Mlen != dz->otherfile->Mlen) {
            sprintf(errstr,"Files do not have same ANALYSIS WINDOW LENGTH.\n");
            return(FINISHED);
        }
        if(dz->infile->Dfac != dz->otherfile->Dfac) {
            sprintf(errstr,"Files do not have same DECIMATION FACTOR.\n");  
            return(FINISHED);   
        }
    }
    return(CONTINUE);
}

/********************************* DO_SFDIFF ***********************************/

int do_sfdiff(char temp[],dataptr dz)
{
    int exit_status;
    int samesize = TRUE;

    switch(dz->process) {
    case(INFO_CDIFF):
        if(dz->infile->channels!=2) {
            sprintf(errstr,"Process only works with STEREO files.\n");
            return(DATA_ERROR);
        }
        break;
    default:
        if(is_a_text_input_filetype(dz->infile->filetype)
        || is_a_text_input_filetype(dz->otherfile->filetype)) {
            sprintf(errstr,"Program only works with soundfiling system files.\n");
            return(DATA_ERROR);
        }
        if(dz->infile->filetype != dz->otherfile->filetype) {
            print_outmessage_flush("Files are not of the same type.\n");
            return(FINISHED);
        }
        if(compare_infile_sizes(&samesize,dz)!=CONTINUE)
            return(FINISHED);

        switch(dz->infile->filetype) {      
        case(SNDFILE):
            if(compare_sndfile_properties(temp,dz)!=CONTINUE) {
                print_outmessage_flush(errstr);
                return(FINISHED);
            }
            break; 
        default:
            if(compare_analfile_properties(dz)!=CONTINUE) {
                print_outmessage_flush(errstr);
                return(FINISHED);
            }
            dz->flbufptr[0] = dz->sampbuf[0];
            dz->flbufptr[1] = dz->sampbuf[1];
            break;
        }
        break; 
    }
    if((exit_status = sfdiff_process(temp,samesize,dz))<0)
        return(exit_status);

    print_outmessage_flush(errstr);
    return(FINISHED);
}

/********************************* PRNTPROPS ***********************************/
/* RWD much adapted to report PEAK data if available */
int prntprops(SFPROPS *props,dataptr dz)
{
    char *prop;
    time_t thistime;
    char *p;
    CHPEAK peaks[2000];
    int chans, peaktime;

    if((prop = (char *)malloc(max(sizeof(unsigned int),sizeof(double))))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to get properties.\n");
        return(MEMORY_ERROR);
    }

    /*RWD find PEAK chunk as priority */
    
    if(snd_headread(dz->ifd[0],props)){
        if((*props).type == wt_wave) {
            switch(props->samptype){      /*RWD Nov 2003 show sample type now we have so many possibilities! */
            case(SHORT16):
                sprintf(errstr,"sample type:  16bit\n");
                break;
            case(FLOAT32):
                sprintf(errstr,"sample type:  32bit floats\n");
                break;          
            case(INT2424):
                sprintf(errstr,"sample type:  24bit packed\n");
                break;
            case(SHORT8):
                sprintf(errstr,"sample type:  8bit\n");
                break;
            case(INT2432):
                sprintf(errstr,"sample type:  24bit in 32bit frames\n");
                break;
            case(INT_32):
                sprintf(errstr,"sample type:  32bit integer\n");
                break;
            default:
                sprintf(errstr,"sample type unknown (custom format)\n");
                break;
            }
            print_outmessage(errstr);
            chans = (*props).chans; 
            if(sndreadpeaks(dz->ifd[0],chans,peaks,&peaktime)>0){
                int i;
                thistime = (time_t) peaktime;
                sprintf(errstr,"PEAK data at: %s\n",ctime(&thistime));
                print_outmessage(errstr);
                for(i=0;i < chans;i++){
                    sprintf(errstr,"CH %d:\tamp = %.4f (%.2f dB)\tFrame %u\n",
                        i+1,peaks[i].value,20 * log10(peaks[i].value),peaks[i].position); 
                    print_outmessage(errstr);
                }
            }
            else {
                sprintf(errstr,"No PEAK chunk in this file");
                print_outmessage(errstr);
            }
        }
    }

    if(sndgetprop(dz->ifd[0], "date", prop, sizeof(unsigned int))>=0
    || sndgetprop(dz->ifd[0], "DATE", prop, sizeof(unsigned int))>=0) {
        thistime = (time_t) *(int*)prop;
        p = ctime(&thistime);
        
        sprintf(errstr,"Date: %s\n",p);
        print_outmessage(errstr);
    }
    fflush(stdout);
    return(FINISHED);
}       

/************************** GET_SHORT_MAXMIN ************************/

int get_float_maxmin(float *buf,int chans, double inverse_sr,dataptr dz)
{
    int exit_status;
    /*int maxamp = MINSAMP;*/
    float maxamp = F_MINSAMP,mag;
    int maxloc = 0;
    int maxrep = 1;
    int n;
    double gain = 0.0, dbgain = 0.0;
    double convertor = (double)(chans * dz->infile->srate);
    int startsamp = round(dz->param[MAX_STIME] * convertor);
    int endsamp = round(dz->param[MAX_ETIME] * convertor);
    int last_total_samps_read;
    int startsrch, endsrch;
    dz->total_samps_read = 0;   /* REDUNDANT ? */
    while(dz->samps_left) {
        last_total_samps_read = dz->total_samps_read;
        if((exit_status = read_samps(buf,dz))<0)
            return(exit_status);
        if(dz->total_samps_read <= startsamp)
            continue;
        else
            startsrch = max(0,startsamp - last_total_samps_read);
        if(dz->total_samps_read > endsamp) {
            if((endsrch = endsamp - last_total_samps_read) < 0)
                break;
        } else
            endsrch = dz->ssampsread;
        for(n=startsrch;n<endsrch;n++) {
            mag = (float) fabs(buf[n]);
            if(mag > maxamp) {
                maxloc = last_total_samps_read + n;
                maxamp = mag;
                maxrep = 1;
            } else if(mag==maxamp)
                maxrep++;
        }
    }
    if(maxamp>0.0f) {
        gain   = (double)F_MAXSAMP/(double)maxamp;
        dbgain = log10(gain) * 20.0;
    }
    sprintf(errstr,"maximum abs value:.. %f\n",maxamp);
    print_outmessage(errstr);
    sprintf(errstr,"at: ................ %d samples\n",maxloc);
    print_outmessage(errstr);
    sprintf(errstr,"time: .............. %.4lf secs\n",(double)(maxloc/chans) *inverse_sr);
    print_outmessage(errstr);
    sprintf(errstr,"repeated: .......... %d times\n",maxrep);
    print_outmessage(errstr);
    if(maxamp>0.0f) {
        sprintf(errstr,"max possible gain:.. %.4lf\n",gain);
        print_outmessage(errstr);
        sprintf(errstr,"max dB gain:........ %.4lf\n",dbgain);
        print_outmessage(errstr);
    }
    return(FINISHED);
}

/*RWD 6:2001 read PEAK chunk */
int getpeakdata(int ifd, float *peakval,SFPROPS *props)
{
    CHPEAK peaks[16];
    int chans, peaktime;
    float maxamp = 0.0f;
    
    if(snd_headread(ifd,props)){
        chans = (*props).chans; 
        if(sndreadpeaks(ifd,chans,peaks,&peaktime)){
            int i;
            for(i=0;i < chans;i++)
                maxamp = max(maxamp,peaks[i].value);
            *peakval = maxamp;
            return 1;
        }       
    }
    /* sf_headread error; but can't do much about it */
    return 0;
}

/************************** DO_ZCROSS_RATIO ************************/

int do_zcross_ratio(dataptr dz)
{
    int exit_status;
    int chans = dz->infile->channels, *phase, k, m;
    int n, *cnt, cntsum = 0, zc0, zc1, lastzc0, lastzc1, bufstart, bufend, bufskip, skipsamps, totsams, total_samps_processed;
    double zc_ratio;
    float *buf = dz->sampbuf[0];
    double srate = dz->infile->srate;
    if((cnt = (int *)malloc(chans * sizeof(int)))==NULL) {
        sprintf(errstr,"NO MEMORY TO STORE ZERO-CROSSING COUNTERS.\n");
        return(MEMORY_ERROR);
    }
    if((phase = (int *)malloc(chans * sizeof(int)))==NULL) {
        sprintf(errstr,"NO MEMORY TO STORE PHASE MARKERS.\n");
        return(MEMORY_ERROR);
    }
    dz->iparam[ZC_START] = (int)round(dz->param[ZC_START] * srate) * chans;
    bufend               = (int)round(dz->param[ZC_END]   * srate) * chans;
    totsams = bufend - dz->iparam[ZC_START];
    dz->tempsize = totsams;
    bufstart = dz->iparam[ZC_START] % dz->buflen;
    if((bufskip = dz->iparam[ZC_START] / dz->buflen) > 0) {
        skipsamps = bufskip * dz->buflen;
        sndseekEx(dz->ifd[0],skipsamps,0);
        bufend   -= skipsamps;
    }
    if((exit_status = read_samps(buf,dz)) < 0)
        return(exit_status);
    if(dz->ssampsread < chans * 2) {
        sprintf(errstr,"INSUFFICIENT DATA TO COUNT ZERO-CROSSINGS.\n");
        return(DATA_ERROR);
    }
    total_samps_processed = 0;
    for(m=0,n = bufstart;m<chans;m++,n++) {
        if(buf[n] > 0)
            phase[m] = 1;
        else if(buf[n] < 0)
            phase[m] = -1;
        else
            phase[m] = 0;
        cnt[m] = 0;
    }
    k = bufstart + chans;
    while(bufend > dz->buflen)  {
        for(n = k; n < dz->ssampsread;n+=chans) {
            for(m=0;m<chans;m++) {
                switch(phase[m]) {
                case(0):
                    if(buf[n+m] > 0)
                        phase[m] = 1;
                    else if(buf[n+m] < 0)
                        phase[m] = -1;
                    break;
                case(1):
                    if(buf[n+m] < 0) {
                        cnt[m]++;
                        phase[m] = -1;
                    }
                    break;
                case(-1):
                    if(buf[n+m] > 0) {
                        cnt[m]++;
                        phase[m] = 1;
                    }
                    break;
                }
            }
        }
        total_samps_processed += (dz->ssampsread - k);
        display_virtual_time(total_samps_processed,dz);
        if((exit_status = read_samps(buf,dz)) < 0)
            return(exit_status);
        bufend -= dz->buflen;
        k = 0;
    }
    if(bufend > 0) {
        for(n = k; n < bufend;n+=chans) {
            for(m=0;m<chans;m++) {
                switch(phase[m]) {
                case(0):
                    if(buf[n+m] > 0)
                        phase[m] = 1;
                    else if(buf[n+m] < 0)
                        phase[m] = -1;
                    break;
                case(1):
                    if(buf[n+m] < 0) {
                        cnt[m]++;
                        phase[m] = -1;
                    }
                    break;
                case(-1):
                    if(buf[n+m] > 0) {
                        cnt[m]++;
                        phase[m] = 1;
                    }
                    break;
                }
            }
        }
        total_samps_processed += (bufend - k);
        display_virtual_time(total_samps_processed,dz);
    }
    for(m=0;m<chans;m++)
        cntsum += cnt[m];
    zc_ratio = (double)cntsum/(double)totsams;
    zc1 = 32768;
    zc0 = (int)round(zc_ratio * zc1);
    do {
        lastzc0 = zc0;
        lastzc1 = zc1;
        zc1 /= 2;
        zc0 = (int)round(zc_ratio * zc1);
    } while(zc0 > 0 && zc1 >= 64);
    zc0 = lastzc0;
    zc1 = lastzc1;
    while(EVEN(zc0)) {
        zc0 /= 2;
        zc1 /= 2;
    }
    fprintf(stdout,"INFO: Proportion of zero-crossings = %lf or approx %d:%d\n",zc_ratio,zc0,zc1);
    fflush(stdout);
    return FINISHED;
}
