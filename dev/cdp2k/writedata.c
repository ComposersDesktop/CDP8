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
#include <processno.h>
#include <modeno.h>
#include <filtcon.h>
#include <cdpmain.h>
#include <globcon.h>
#include <logic.h>
#include <standalone.h>

#include <pnames.h>
#include <sfsys.h>
//TW ADDED
#include <limits.h>

//#ifdef unix
#define round(x) lround((x))
//#endif

static int  set_output_header_properties(dataptr dz);
static int  pt_datareduce(double **q,double sharp,double flat,double *thisarray,int *bsize);
static int  incremental_pt_datareduction(int array_no,int *bsize,double sharpness,dataptr dz);

    /* OTHER */
static int  print_screen_message(void);

/* AUGUST 2000 */
unsigned int superzargo;

/***************************** COMPLETE_OUTPUT *******************************/

int complete_output(dataptr dz)
{
    if(dz->process_type==PITCH_TO_PITCH || dz->process_type==PITCH_TO_BIGPITCH) {
        if(dz->is_transpos)
            dz->outfiletype = TRANSPOS_OUT;
        else
            dz->outfiletype = PITCH_OUT;
    }
    switch(dz->process_type) {
    case(UNEQUAL_SNDFILE):
    case(BIG_ANALFILE):
    case(UNEQUAL_ENVFILE):
    case(CREATE_ENVFILE):
    case(EXTRACT_ENVFILE):
    case(PITCH_TO_ANAL):
    case(ANAL_TO_FORMANTS):
    case(PITCH_TO_BIGPITCH):
    case(PSEUDOSNDFILE):
//TW No longer used:
//      if((exit_status = truncate_outfile(dz))<0)
//          return(exit_status);
        /* fall thro */
    case(EQUAL_ANALFILE):
    case(MAX_ANALFILE):
    case(MIN_ANALFILE):
    case(EQUAL_FORMANTS):
    case(EQUAL_SNDFILE):
    case(EQUAL_ENVFILE):
    case(PITCH_TO_PITCH):
    case(PITCH_TO_PSEUDOSND):
        return headwrite(dz->ofd,dz);
        break;
    case(SCREEN_MESSAGE):
        return print_screen_message();
    case(ANAL_TO_PITCH):
        sprintf(errstr,"ANAL_TO_PITCH process_type (%d) in complete_output()\n",dz->process_type);
        return(PROGRAM_ERROR);
    case(TO_TEXTFILE):      
        if(fclose(dz->fp)<0) {
            fprintf(stdout,"WARNING: Failed to close output textfile.\n");
            fflush(stdout);
        }
        break;
    case(OTHER_PROCESS):        
        break;
    default:
        sprintf(errstr,"Unknown process_type (%d) in complete_output()\n",dz->process_type);
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/***************************** TRUNCATE_OUTFILE *******************************/
#ifdef NOTDEF
int truncate_outfile(dataptr dz)
{
    if(sfadjust(dz->ofd,dz->total_bytes_written - dz->outfilesize)<0) {
        sprintf(errstr,"Cannot truncate output file (Is hard-disk full?).\n");
        return(SYSTEM_ERROR);
    }
    return(FINISHED);
}
#endif
/***************************** HEADWRITE *******************************/

int headwrite(int ofd,dataptr dz)
{   
    int   exit_status;
    int   isenv = 1;
    int  property_marker = 1;
//    int  samptype, srate, channels;
    float window_size = 0.0f;

    if(!(dz->process == PVOC_ANAL || dz->process == PVOC_EXTRACT || dz->process == PVOC_SYNTH)) {
        if((exit_status = set_output_header_properties(dz))<0)
            return(exit_status);
    }
    switch(dz->outfiletype) {
    case(FORMANTS_OUT):
        if(sndputprop(ofd,"specenvcnt", (char *)&(dz->outfile->specenvcnt), sizeof(int)) < 0){
            sprintf(errstr,"Failure to write specenvcnt: headwrite()\n");
            return(PROGRAM_ERROR);
        }
        /* drop through */
    case(PITCH_OUT):
    case(TRANSPOS_OUT):
        if(sndputprop(ofd,"orig channels", (char *)&(dz->outfile->origchans), sizeof(int)) < 0){
            sprintf(errstr,"Failure to write original channel data: headwrite()\n");
            return(PROGRAM_ERROR);
        }
        /* drop through */
    case(ANALFILE_OUT):
        if(sndputprop(ofd,"original sampsize", (char *)&(dz->outfile->origstype), sizeof(int)) < 0){
            sprintf(errstr,"Failure to write original sample size. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        if(sndputprop(ofd,"original sample rate", (char *)&(dz->outfile->origrate), sizeof(int)) < 0){
            sprintf(errstr,"Failure to write original sample rate. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        if(sndputprop(ofd,"arate",(char *)&(dz->outfile->arate),sizeof(float)) < 0){
            sprintf(errstr,"Failed to write analysis sample rate. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        if(sndputprop(ofd,"analwinlen",(char *)&(dz->outfile->Mlen),sizeof(int)) < 0){
            sprintf(errstr,"Failure to write analysis window length. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        if(sndputprop(ofd,"decfactor",(char *)&(dz->outfile->Dfac),sizeof(int)) < 0){
            sprintf(errstr,"Failure to write decimation factor. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        /* drop through */
    case(SNDFILE_OUT):
//TW these properties can either be created at the outset (sndcreat_formatted() 
//  in which case they are not alterable here.
//  or they can be left open (sndcreat e.g. for PITCH, or CUT for dz->otherfile)
        sndputprop(ofd,"sample rate", (char *)&(dz->outfile->srate), sizeof(int));
        sndputprop(ofd,"channels", (char *)&(dz->outfile->channels), sizeof(int));
        break;
    case(ENVFILE_OUT):  
 //       samptype = SAMP_FLOAT;
 //       srate = round(SECS_TO_MS/dz->outfile->window_size);
 //       channels = 1;
        window_size = dz->outfile->window_size;
/* JUNE 2004
        if(sndputprop(ofd,"sample rate", (char *)&srate, sizeof(int)) < 0){
            sprintf(errstr,"Failure to write sample rate. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        if(sndputprop(ofd,"channels", (char *)&channels, sizeof(int)) < 0){
            sprintf(errstr,"Failure to write channel data. headwrite()\n");
            return(PROGRAM_ERROR);
        }
*/
        if(sndputprop(ofd,"is an envelope",(char *)&isenv, sizeof(int)) < 0){
            sprintf(errstr,"Failure to write envelope property. headwrite()\n");
            return(PROGRAM_ERROR);
        }
        if(sndputprop(ofd,"window size", (char *)(&window_size), sizeof(float)) < 0) {
            sprintf(errstr,"Failure to write window size. headwrite()\n");
            return(DATA_ERROR);
        }
        break;

    default:
        sprintf(errstr,"Unknown outfile type: headwrite()\n");
        return(PROGRAM_ERROR);
    }
    switch(dz->outfiletype) {
    case(PITCH_OUT):
        if(sndputprop(ofd,"is a pitch file", (char *)&property_marker, sizeof(int)) < 0){
            sprintf(errstr,"Failure to write pitch property: headwrite()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(TRANSPOS_OUT):
        if(sndputprop(ofd,"is a transpos file", (char *)&property_marker, sizeof(int)) < 0){
            sprintf(errstr,"Failure to write transposition property: headwrite()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(FORMANTS_OUT):
        if(sndputprop(ofd,"is a formant file", (char *)&property_marker, sizeof(int)) < 0){
            sprintf(errstr,"Failure to write formant property: headwrite()\n");
            return(PROGRAM_ERROR);
        }
        break;
    }
    return(FINISHED);
}

/***************************** SET_OUTPUT_HEADER_PROPERTIES ******************************/         
 
int set_output_header_properties(dataptr dz)
{   
    switch(dz->process_type) {
    case(EQUAL_SNDFILE):
    case(UNEQUAL_SNDFILE):
    case(EQUAL_ENVFILE):
    case(UNEQUAL_ENVFILE):
        dz->outfile->channels = dz->infile->channels;
        dz->outfile->srate    = dz->infile->srate;
        dz->outfile->stype    = dz->infile->stype;
        break;
    case(EXTRACT_ENVFILE):
    case(CREATE_ENVFILE):
        break;
    case(PSEUDOSNDFILE):     /* Assumption of conversion from anal,formant,pitch or transpos filetype */
    case(PITCH_TO_PSEUDOSND):     
        dz->outfile->channels = 1;
        dz->outfile->srate    = dz->infile->origrate;
        dz->outfile->stype    = SAMP_SHORT;
        break;
    case(ANAL_TO_FORMANTS):
        dz->outfile->specenvcnt= dz->infile->specenvcnt;
        /* fall through */
    case(ANAL_TO_PITCH):
        dz->outfile->origchans = dz->infile->channels;
        dz->outfile->channels  = 1;
        dz->outfile->origstype = dz->infile->origstype;
        dz->outfile->origrate  = dz->infile->origrate;
        dz->outfile->arate     = dz->infile->arate;
        dz->outfile->Mlen      = dz->infile->Mlen;
        dz->outfile->Dfac      = dz->infile->Dfac;
        dz->outfile->srate     = dz->infile->srate;
        dz->outfile->stype     = dz->infile->stype;
        break;
    case(EQUAL_FORMANTS):
        dz->outfile->specenvcnt = dz->infile->specenvcnt;
        /* fall through */
    case(PITCH_TO_PITCH):
    case(PITCH_TO_BIGPITCH):
        dz->outfile->origchans = dz->infile->origchans;
        /* fall through */
    case(EQUAL_ANALFILE):
    case(MAX_ANALFILE):
    case(MIN_ANALFILE):
    case(BIG_ANALFILE):
    case(PITCH_TO_ANAL):
        dz->outfile->origstype = dz->infile->origstype;
        dz->outfile->origrate  = dz->infile->origrate;
        dz->outfile->arate     = dz->infile->arate;
        dz->outfile->Mlen      = dz->infile->Mlen;
        dz->outfile->Dfac      = dz->infile->Dfac;
        dz->outfile->srate     = dz->infile->srate;
        dz->outfile->stype     = dz->infile->stype;
        if(dz->process_type==PITCH_TO_ANAL)
            dz->outfile->channels = dz->infile->origchans;
        else
            dz->outfile->channels = dz->infile->channels;
        break;
    case(TO_TEXTFILE):
    case(SCREEN_MESSAGE):
        break;
    default:
        sprintf(errstr,"Unknown process: set_output_header_properties()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/***************************** WRITE_EXACT_SAMPS ******************************
 *
 * Writes a block of known length to file area of same length.
 * Checks it has all been written.
 */
#ifdef NOTDEF
int write_exact_bytes(char *buffer,int bytes_to_write,dataptr dz)
{
    int pseudo_bytes_to_write = bytes_to_write;
    int secs_to_write = bytes_to_write/SECSIZE;
    int bytes_written;
    if((secs_to_write * SECSIZE)!=bytes_to_write) {
        secs_to_write++;
        pseudo_bytes_to_write = secs_to_write * SECSIZE;
    }
    if((bytes_written = sfwrite(dz->ofd, buffer, pseudo_bytes_to_write))<0) {
        sprintf(errstr, "Can't write to output soundfile: (is hard-disk full?).\n");
        return(SYSTEM_ERROR);
    }
    if(bytes_written != bytes_to_write) {
        sprintf(errstr, "Incorrect number of bytes written\nbytes_written = %ld\n"
                        "bytes_to_write = %ld\n (is hard-disk full?).\n",
                        bytes_written,bytes_to_write);
        return(SYSTEM_ERROR);
    }
    dz->total_bytes_written += bytes_to_write;
    display_virtual_time(dz->total_bytes_written,dz);
    return(FINISHED);
}
#else
int write_exact_samps(float *buffer,int samps_to_write,dataptr dz)
{
    int i,j;
    int samps_written;
    float val;

    if(dz->needpeaks){
        for(i=0;i < samps_to_write; i += dz->outchans){
            for(j = 0;j < dz->outchans;j++){
                val = (float)fabs(buffer[i+j]);
                /* this way, posiiton of first peak value is stored */
                if(val > dz->outpeaks[j].value){
                    dz->outpeaks[j].value = val;
                    dz->outpeaks[j].position = dz->outpeakpos[j];
                }
            }
            /* count framepos */
            for(j=0;j < dz->outchans;j++)
                dz->outpeakpos[j]++;
        }
    }
    if((samps_written = fputfbufEx(buffer, samps_to_write,dz->ofd))<=0) {
        sprintf(errstr, "Can't write to output soundfile: %s\n",sferrstr());
        return(SYSTEM_ERROR);
    }
    if(samps_written != samps_to_write) {
        sprintf(errstr, "Incorrect number of samples written\nsamps_written = %d\n"
                        "samps_to_write = %d\n (is hard-disk full?).\n",
                        samps_written,samps_to_write);
        return(SYSTEM_ERROR);
    }
    dz->total_samps_written += samps_to_write;
    display_virtual_time(dz->total_samps_written,dz);
    return(FINISHED);
}

#endif


#ifdef NOTDEF
/*************************** WRITE_SAMPS ***********************/

int write_bytes(char *bbuf,int bytes_to_write,dataptr dz)
{
    int pseudo_bytes_to_write = bytes_to_write;
    int secs_to_write = bytes_to_write/SECSIZE;
    int bytes_written;
    if((secs_to_write * SECSIZE)!=bytes_to_write) {
        secs_to_write++;
        pseudo_bytes_to_write = secs_to_write * SECSIZE;
    }
    if((bytes_written = sfwrite(dz->ofd,bbuf,pseudo_bytes_to_write))<0) {
        sprintf(errstr,"Can't write to output soundfile: (is hard-disk full?).\n");
        return(SYSTEM_ERROR);
    }
    dz->total_bytes_written += bytes_to_write;
    dz->total_samps_written  = dz->total_bytes_written/sizeof(short);  /* IRRELEVANT TO SPEC */
    display_virtual_time(dz->total_bytes_written,dz);
    return(FINISHED);

}
#else
int write_samps(float *bbuf,int samps_to_write,dataptr dz)
{
    
    int samps_written;
    int i,j;
    float val;

    if(dz->needpeaks){
        for(i=0;i < samps_to_write; i += dz->outchans){
            for(j = 0;j < dz->outchans;j++){
                val = (float)fabs(bbuf[i+j]);
                /* this way, posiiton of first peak value is stored */
                if(val > dz->outpeaks[j].value){
                    dz->outpeaks[j].value = val;
                    dz->outpeaks[j].position = dz->outpeakpos[j];
                }
            }
            /* count framepos */
            for(j=0;j < dz->outchans;j++)
                dz->outpeakpos[j]++;
        }
    }
    if((samps_written = fputfbufEx(bbuf,samps_to_write,dz->ofd))<=0) {
        sprintf(errstr,"Can't write to output soundfile: %s\n",sferrstr());
        return(SYSTEM_ERROR);
    }
    dz->total_samps_written += samps_written;   
    display_virtual_time(dz->total_samps_written,dz);
    return(FINISHED);
}


#endif
/*************************** WRITE_SAMPS_NO_REPORT ***********************/
#ifdef NOTDEF
int write_bytes_no_report(char *bbuf,int bytes_to_write,dataptr dz)
{
    int pseudo_bytes_to_write = bytes_to_write;
    int secs_to_write = bytes_to_write/SECSIZE;
    int bytes_written;
    if((secs_to_write * SECSIZE)!=bytes_to_write) {
        secs_to_write++;
        pseudo_bytes_to_write = secs_to_write * SECSIZE;
    }
    if((bytes_written = sfwrite(dz->ofd,bbuf,pseudo_bytes_to_write))<0) {
        sprintf(errstr,"Can't write to output soundfile: (is hard-disk full?).\n");
        return(SYSTEM_ERROR);
    }
    dz->total_bytes_written += bytes_to_write;
    dz->total_samps_written  = dz->total_bytes_written/sizeof(short);  /* IRRELEVANT TO SPEC */
    return(FINISHED);
}
#else
int write_samps_no_report(float *bbuf,int samps_to_write,int *samps_written,dataptr dz)
{
    
    int i;
    int j;
    float val;
    if(dz->needpeaks){
        for(i=0;i < samps_to_write; i += dz->outchans){
            for(j = 0;j < dz->outchans;j++){
                val = (float)fabs(bbuf[i+j]);
                /* this way, posiiton of first peak value is stored */
                if(val > dz->outpeaks[j].value){
                    dz->outpeaks[j].value = val;
                    dz->outpeaks[j].position = dz->outpeakpos[j];
                }
            }
            /* count framepos */
            for(j=0;j < dz->outchans;j++)
                dz->outpeakpos[j]++;
        }
    }
    if((*samps_written = fputfbufEx(bbuf,samps_to_write,dz->ofd))<=0) {
        sprintf(errstr,"Can't write to output soundfile: %s\n",sferrstr());
        return(SYSTEM_ERROR);
    }
    dz->total_samps_written += samps_to_write;
    
    return(FINISHED);
}


#endif
/*********************** DISPLAY_VIRTUAL_TIME *********************/

void display_virtual_time(int samps_sent,dataptr dz)
{
    int mins;
    double  secs = 0.0;
    int startwindow, endwindow, windows_to_write, total_samps_to_write = 0, maxlen, minlen, offset;
    unsigned int zargo = 0;
    int n, display_time;
    double float_time = 0.0;
    if(sloombatch)
        return;
    if(!sloom) {
        if(dz->process==HOUSE_COPY && dz->mode==COPYSF)
            secs = (double)samps_sent/(double)(dz->infile->srate * dz->infile->channels);
        else {
            switch(dz->outfiletype) {
            case(NO_OUTPUTFILE):
                if(dz->process==MIXMAX)
                    secs = (double)samps_sent/(double)(dz->infile->srate * dz->infile->channels);
                else
                    return;
            case(SNDFILE_OUT):
                secs = (double)samps_sent/(double)(dz->infile->srate * dz->infile->channels);
                break;
            case(ANALFILE_OUT):
                secs = (double)(samps_sent/dz->wanted) * dz->frametime;
                break;
            case(PITCH_OUT):                       
            case(TRANSPOS_OUT):
                secs = (double)(samps_sent) * dz->frametime;
                break;
            case(FORMANTS_OUT):
                secs = (double)((samps_sent - dz->descriptor_samps)/dz->infile->specenvcnt) * dz->frametime;
                break;
            case(ENVFILE_OUT):
                secs = (double)(samps_sent) * dz->outfile->window_size * MS_TO_SECS;
                break;
            case(TEXTFILE_OUT):
                switch(dz->process) {
                case(INFO_PRNTSND):
                    secs = (double)samps_sent/(double)(dz->infile->srate * dz->infile->channels);
                    break;
                }   
                break;
    
            }
        }
        mins = (int)(secs/60.0);    /* TRUNCATE */
        secs -= (double)(mins * 60);
        fprintf(stdout,"\r%d min %5.2lf sec", mins, secs);
        fflush(stdout);

    } else {
        switch(dz->process_type) {
        case(EQUAL_ANALFILE):
            float_time   = min(1.0,(double)samps_sent/(double)dz->insams[0]);
            break;
        case(MAX_ANALFILE):
            switch(dz->process) {
                case(MAX):  /* file 0 is forced to be largest by program */
                    float_time = (double)samps_sent/(double)dz->insams[0];
                    break;
            default:
                maxlen = dz->insams[0];
                for(n=1;n<dz->infilecnt;n++)
                    maxlen = max(maxlen,dz->insams[n]);
                float_time = (double)samps_sent/(double)maxlen;
                break;
            }
            break;
        case(MIN_ANALFILE):
            minlen = dz->insams[0];
            for(n=1;n<dz->infilecnt;n++)
                minlen = min(minlen,dz->insams[n]);
            float_time = (double)samps_sent/(double)minlen;
            break;
        case(BIG_ANALFILE):
            switch(dz->process) {
            case(CUT):
                startwindow = (int)(dz->param[CUT_STIME]/dz->frametime);
                endwindow   = (int)(dz->param[CUT_ETIME]/dz->frametime);
                endwindow   = min(endwindow,dz->wlength);
                windows_to_write = endwindow - startwindow;
                total_samps_to_write = windows_to_write * dz->wanted;
                float_time = (double)samps_sent/(double)total_samps_to_write;
                break;
            case(GRAB):
                total_samps_to_write = dz->wanted;
                float_time = (double)samps_sent/(double)total_samps_to_write;
                break;
            case(MAGNIFY):
                windows_to_write = dz->total_windows + 1;
                total_samps_to_write = windows_to_write * dz->wanted;
                float_time = (double)samps_sent/(double)total_samps_to_write;
                break;
            case(TSTRETCH): case(SHUFFLE): case(WEAVE): case(WARP): case(FORM):     
                float_time = (double)dz->total_samps_read/(double)dz->insams[0];
                break;
            case(DRUNK):        
                total_samps_to_write = round((dz->param[DRNK_DUR]/dz->frametime) * dz->wanted);
                float_time = min((double)samps_sent/(double)total_samps_to_write,1.0);
                break;
            case(GLIDE):        
//TW            total_samps_to_write = dz->wlength * dz->sampswanted;
                total_samps_to_write = dz->wlength * dz->wanted;
                float_time = (double)samps_sent/(double)total_samps_to_write;
                break;
            case(BRIDGE):       
                offset = round(dz->param[BRG_OFFSET]/dz->frametime) * dz->wanted;
                switch(dz->iparam[BRG_TAILIS]) {
                case(0):
                case(1):    total_samps_to_write = dz->insams[0];                               break;
                case(2):    total_samps_to_write = offset + dz->insams[1];                      break;
                case(3):    total_samps_to_write = min(dz->insams[0],offset + dz->insams[1]);   break;
                }
                float_time = min(1.0,(double)samps_sent/(double)total_samps_to_write);
                break;
            case(MORPH):        
                total_samps_to_write = (dz->iparam[MPH_STAGW] * dz->wanted) + dz->insams[1];
                float_time = min(1.0,(double)samps_sent/(double)total_samps_to_write);
                break;
            case(SYNTH_SPEC):
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
            case(CLICK):
                if(samps_sent < 0)
                    samps_sent = INT_MAX;
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
            default:
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
            }
            break;
        case(ANAL_TO_FORMANTS):
            float_time = (double)dz->total_samps_read/(double)dz->insams[0];
            break;
        case(PSEUDOSNDFILE):
            switch(dz->process) {
            case(FMNTSEE):
            case(FORMSEE):
            case(LEVEL):
                float_time = (double)dz->total_samps_read/(double)dz->insams[0];
                break;
            }
            break;
        case(PITCH_TO_ANAL):
            switch(dz->process) {
            case(MAKE):
//TW NEW CASE
            case(MAKE2):
                /* windows_to_write = dz->wlength, in this case based on specenvcnt-len formant-windows */
                total_samps_to_write = dz->wlength * (dz->infile->origchans);
                float_time = min(1.0,(double)samps_sent/(double)total_samps_to_write);
                break;
//TW NEW CASES ADDED, therefore
//          case(P_HEAR):
            default:
                /* windows_to_write = dz->insams[0] as each pitch is converted to a window */
                total_samps_to_write = dz->insams[0] * dz->wanted;
                float_time = (double)samps_sent/(double)total_samps_to_write;
                break;
            }
            break;
        case(PITCH_TO_PSEUDOSND):
            total_samps_to_write = dz->insams[0];
            float_time = (double)samps_sent/(double)total_samps_to_write;
            break;
        case(UNEQUAL_SNDFILE):
            switch(dz->process) {
            case(DISTORT_AVG):  case(DISTORT_MLT):  case(DISTORT_DIV):  case(DISTORT_HRM):  case(DISTORT_FRC):
            case(DISTORT_REV):  case(DISTORT_SHUF): case(DISTORT_RPT):  case(DISTORT_INTP): case(DISTORT_DEL):
            case(DISTORT_RPL):  case(DISTORT_TEL):  case(DISTORT_FLT):  case(DISTORT_PCH):  
//TW NEW CASE
            case(DISTORT_OVERLOAD): case(DISTORT_RPT2):
            case(GRAIN_OMIT):       case(GRAIN_DUPLICATE):  case(GRAIN_REORDER):    case(GRAIN_REPITCH):
            case(GRAIN_RERHYTHM):   case(GRAIN_REMOTIF):    case(GRAIN_TIMEWARP):
            case(GRAIN_POSITION):   case(GRAIN_ALIGN):
                float_time = min(1.0,(double)dz->total_samps_read/(double)dz->insams[0]);
                break;
//TW NEW CASE
            case(DISTORT_PULSED):
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
            case(GRAIN_REVERSE):
                float_time = (double)dz->total_samps_written/(double)dz->insams[0];
                break;
            case(TOPNTAIL_CLICKS):
                float_time = min(1.0,(double)dz->total_samps_read/(double)dz->tempsize);
                break;
            case(DISTORT_INT):  
                total_samps_to_write = (dz->insams[0] + dz->insams[1]) >> 1;/* averaged,as is bytes_read !! */
                float_time = min(1.0,(double)dz->total_samps_read/(double)total_samps_to_write);
                break;
            case(HF_PERM1): case(HF_PERM2):
                zargo = superzargo + samps_sent;
                float_time = min(1.0,(double)zargo/(double)dz->tempsize);
                break;
            case(SCRAMBLE):
                total_samps_to_write = dz->iparam[SCRAMBLE_OUTLEN];
                float_time = min(1.0,(double)samps_sent/(double)total_samps_to_write);
                break;
            case(DRUNKWALK):
                total_samps_to_write = dz->iparam[DRNK_TOTALDUR];
                float_time = min(1.0,(double)samps_sent/(double)total_samps_to_write);
                break;
            case(SIMPLE_TEX):   case(TIMED):    case(GROUPS):    case(TGROUPS):     case(DECORATED):  case(PREDECOR):   
            case(POSTDECOR):    case(ORNATE):   case(PREORNATE): case(POSTORNATE):  case(MOTIFS):     case(TMOTIFS):    
            case(MOTIFSIN):     case(TMOTIFSIN):
                total_samps_to_write = round(dz->param[TEXTURE_DUR] * dz->infile->srate) * STEREO;
                float_time = min(1.0,(double)samps_sent/(double)total_samps_to_write);
                break;
            case(TEX_MCHAN):
                total_samps_to_write = round(dz->param[TEXTURE_DUR] * dz->infile->srate) * dz->iparam[TEXTURE_OUTCHANS];
                float_time = min(1.0,(double)samps_sent/(double)total_samps_to_write);
                break;
            case(ENV_CURTAILING):
                total_samps_to_write = 
                round(dz->param[ENV_ENDTIME] * dz->infile->srate) * dz->infile->channels;
                float_time = min(1.0,(double)samps_sent/(double)total_samps_to_write);
                break;
            case(ENV_IMPOSE):   /* In these cases the 'bytes_sent' may be bytes_written or bytes_read */
            case(ENV_REPLACE):  
//TW NEW CASE
            case(ENV_PROPOR):   
                float_time = (double)samps_sent/(double)dz->insams[0];
                break;
            case(ENV_PLUCK):    
                float_time = min(1.0,(double)dz->total_samps_read/(double)dz->insams[0]);
                break;
            case(EQ):           case(LPHP):     case(FSTATVAR):     case(FLTBANKN):
//TW UPDATE
            case(FLTBANKU):     case(FLTSWEEP):     case(ALLPASS):
                total_samps_to_write = dz->insams[0] + 
                (round(FLT_TAIL * (double)dz->infile->srate) * dz->infile->channels);
                float_time = (double)samps_sent/(double)total_samps_to_write;
                break;
            case(FLTBANKV):
                total_samps_to_write = dz->insams[0] + 
                ( round(dz->param[FILT_TAILV] * (double)dz->infile->srate) * dz->infile->channels);
                float_time = (double)samps_sent/(double)total_samps_to_write;
                break;
            case(FLTITER):
                total_samps_to_write = dz->iparam[FLT_OUTDUR];
                float_time = min(1.0,(double)samps_sent/(double)total_samps_to_write);
                break;
            case(MIXCROSS):
                float_time = min(1.0,(double)samps_sent/(double)(dz->iparam[MCR_STAGGER] + dz->insams[1]));
                break;
            case(MIXINTERL):
                total_samps_to_write = dz->insams[0];
                for(n=0;n<dz->infilecnt;n++)
                    total_samps_to_write = max(total_samps_to_write,dz->insams[n]);
                total_samps_to_write *= dz->infilecnt;
                float_time = min(1.0,(double)samps_sent/(double)total_samps_to_write);
                break;
            case(MIX):
            case(MIXMULTI):
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
//TW NEW CASES
            case(HOUSE_BAKUP):
            case(SEQUENCER):
            case(SEQUENCER2):
            case(RRRR_EXTEND):
            case(CONVOLVE):
            case(BAKTOBAK):
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;

            case(MOD_PITCH):
                float_time = min(1.0,(double)dz->total_samps_read/(double)dz->insams[0]);
                break;
            case(MOD_REVECHO):
                if(dz->mode==MOD_STADIUM)
                    float_time = min(1.0,(double)samps_sent/(double)(dz->insams[0] * 2));
                else 
                    float_time = min(1.0,(double)samps_sent/(double)dz->insams[0]);
                break;
            case(MOD_SPACE):
                if(dz->mode==MOD_PAN)
                    float_time = min(1.0,(double)dz->total_samps_read/(double)dz->insams[0]);
                else
                    float_time = min(1.0,(double)samps_sent/(double)dz->insams[0]);
                break;
//TW NEW CASE
            case(SCALED_PAN):
                float_time = min(1.0,(double)dz->total_samps_read/(double)dz->insams[0]);
                break;
            case(MOD_RADICAL):
                switch(dz->mode) {
                case(MOD_SCRUB):
                    float_time = min(1.0,(double)dz->total_samps_written/(double)dz->iparam[SCRUB_TOTALDUR]);
                    break;
                case(MOD_CROSSMOD):
                    float_time = (double)samps_sent/(double)dz->tempsize;
                    break;
                default:
                    float_time = min(1.0,(double)samps_sent/(double)dz->insams[0]);
                    break;
                }
                break;
            case(PVOC_SYNTH):
            case(EDIT_CUT):         case(EDIT_CUTEND):      case(EDIT_ZCUT):        case(EDIT_EXCISE):  
            case(EDIT_EXCISEMANY):  case(EDIT_INSERT):      case(EDIT_INSERTSIL):   case(EDIT_JOIN):    
            case(HOUSE_CHANS):      case(HOUSE_ZCHANNEL):   case(STOM):             case(MTOS):
            case(ZIGZAG):           case(LOOP):             case(ITERATE): /*  case(HOUSE_BAKUP): */
            case(DEL_PERM):         case(DEL_PERM2):        case(MIXTWO):           case(MIXBALANCE):
            case(HOUSE_SPEC):       case(SYNTH_SPEC):       case(JOIN_SEQ):         case(JOIN_SEQDYN):
            case(ACC_STREAM):       case(ITERATE_EXTEND):

            case(STACK):
            case(SYNTH_WAVE):
            case(SYNTH_NOISE):
            case(SYNTH_SIL):
            case(MULTI_SYN):
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
            case(DOUBLETS):
                if(samps_sent < 0)
                    samps_sent = INT_MAX;
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
            case(AUTOMIX):          case(MIXMANY):          case(EDIT_INSERT2):
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
            case(BRASSAGE):
                float_time = min(1.0,(double)dz->total_samps_read/(double)dz->insams[0]);
                break;
            case(TWIXT):
            case(SPHINX):
                float_time = min(1.0,(double)superzargo/(double)dz->tempsize);
                break;
            case(EDIT_CUTMANY):
            case(MANY_ZCUTS):
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
            case(GREV):
                float_time = min(1.0,(double)dz->total_samps_written/(double)dz->tempsize);
                break;
            default:
                float_time = min(1.0,(double)samps_sent/(double)dz->insams[0]);
                break;
            }
            break;
        case(EQUAL_SNDFILE):
            switch(dz->process) {
            case(HOUSE_CHANS):
//TW NEW CASE
            case(TIME_GRID):
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
//TW NEW CASE
            case(SHUDDER):
                float_time = min(1.0,(double)dz->total_samps_written/(double)dz->tempsize);
                break;
            case(MOD_RADICAL):
                if(dz->mode == MOD_SHRED) { /* based on number of shreds completed */
                    /*RWD NB: SHRED_CNT was in bytes --- make sure it is now in samps */
                    float_time = min(1.0,(double)samps_sent/(double)dz->iparam[SHRED_CNT]);
                }
                /* fall thro */
            default:
                float_time = min(1.0,(double)samps_sent/(double)dz->insams[0]);
                break;
            }
            break;
        case(EXTRACT_ENVFILE):  /* In these cases the 'bytes_sent' is bytes_read */
            float_time = (double)samps_sent/(double)dz->insams[0];
            break;
        case(OTHER_PROCESS):
            switch(dz->process) {
            case(HOUSE_EXTRACT):
            case(HOUSE_GATE):
                float_time = (double)samps_sent/(double)dz->insams[0];
                break;
            case(MIXINBETWEEN):
            case(CYCINBETWEEN):
                total_samps_to_write = max(dz->insams[0],dz->insams[1]);
                float_time = (double)samps_sent/(double)total_samps_to_write;
                break;
            case(HOUSE_COPY):
                switch(dz->mode) {
                case(COPYSF):
                    float_time = (double)samps_sent/(double)dz->insams[0];
                    break;
                case(DUPL):
                    superzargo += samps_sent;
                    float_time = min(1.0,(double)superzargo/(double)dz->tempsize);
                    break;
                }
                break;
    /* NEW */
            case(INFO_DIFF):
                float_time = min(1.0,(double)dz->total_samps_read/(double)dz->insams[0]);
                break;
            }
            break;
        case(TO_TEXTFILE):
            switch(dz->process) {
            case(INFO_PRNTSND):
                zargo = (dz->iparam[PRNT_END] - dz->iparam[PRNT_START]);
                float_time = min(1.0,(double)samps_sent/(double)zargo);
                break;
            case(HOUSE_EXTRACT):
                float_time = min(1.0,(double)samps_sent/(double)dz->insams[0]);
                break;
            case(GRAIN_GET):
                float_time = min(1.0,(double)dz->total_samps_read/(double)dz->insams[0]);
                break;
            }   
            break;
        case(SCREEN_MESSAGE):
            switch(dz->process) {
            case(MIXMAX):
            case(FIND_PANPOS):
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
            }   
            break;
        case(CREATE_ENVFILE):
            switch(dz->process) {
            case(ENVSYN):
                float_time = min(1.0,(double)samps_sent/(double)dz->tempsize);
                break;
            }
        }
        display_time = round(float_time * PBAR_LENGTH);
        fprintf(stdout,"TIME: %d\n",display_time);
        fflush(stdout);
    }
}

/***************************** PRINT_SCREEN_MESSAGE **************************/

int print_screen_message(void)
{
    if(!sloom && !sloombatch)           /*TW May 2001 */
        fprintf(stdout,"%s",errstr);
    else
        splice_multiline_string(errstr,"INFO:");
    fflush(stdout);
    return(FINISHED);
}

/***************************** WRITE_SAMPS_TO_ELSEWHERE ******************************
 *
 * Allows bytes to be written to files other than standard outfile.
 * No byte-count checking.
 */
#ifdef NOTDEF
int write_bytes_to_elsewhere(int ofd, char *buffer,int bytes_to_write,dataptr dz)
{
    int pseudo_bytes_to_write = bytes_to_write;
    int secs_to_write = bytes_to_write/SECSIZE;
    int bytes_written;
    if((secs_to_write * SECSIZE)!=bytes_to_write) {
        secs_to_write++;
        pseudo_bytes_to_write = secs_to_write * SECSIZE;
    }
    if((bytes_written = sfwrite(ofd, (char *)buffer, pseudo_bytes_to_write))<0) {
        sprintf(errstr, "Can't write to output soundfile:  (is hard-disk full?).\n");
        return(SYSTEM_ERROR);
    }
    dz->total_bytes_written += bytes_to_write;
    return(FINISHED);
}
#else
int write_samps_to_elsewhere(int ofd, float *buffer,int samps_to_write,dataptr dz)
{
    int i,j;
    int samps_written;
    if(dz->needotherpeaks){
        for(i=0;i < samps_to_write; i += dz->otheroutchans){
            for(j = 0;j < dz->otheroutchans;j++){
                /* this way, posiiton of first peak value is stored */
                if(buffer[i+j] > dz->outpeaks[j].value){
                    dz->otherpeaks[j].value = buffer[i+j];
                    dz->otherpeaks[j].position = dz->otherpeakpos[j];
                }
            }
            /* count framepos */
            for(j=0;j < dz->otheroutchans;j++)
                dz->otherpeakpos[j]++;
        }
    }
    
    if((samps_written = fputfbufEx(buffer, samps_to_write,ofd))<=0) {
        sprintf(errstr, "Can't write to output soundfile:  %s\n",sferrstr());
        return(SYSTEM_ERROR);
    }
    dz->total_samps_written += samps_to_write;
    return(FINISHED);
}




#endif
/****************************** WRITE_BRKFILE *************************/

int write_brkfile(FILE *fptext,int brklen,int array_no,dataptr dz)
{
    int n;
    double *p = dz->parray[array_no];
    for(n=0;n<brklen;n++) {
        fprintf(fptext,"%lf \t%lf\n",*p,*(p+1));
        p += 2;
    }
    return(FINISHED);
}

/***************** CONVERT_PCH_OR_TRANSPOS_DATA_TO_BRKPNTTABLE ***********************/

#define TONE_CENT   (.01)   /* SEMITONES */

int convert_pch_or_transpos_data_to_brkpnttable(int *brksize,float *floatbuf,float frametime,int array_no,dataptr dz)
{
    int exit_status;
    double *q;
    float *p = floatbuf;
    float *endptab = floatbuf + dz->wlength;
    int n, bsize;
    double sharpness = TONE_CENT;   /* sharpness in semitones */

    if(dz->wlength<2) {
        sprintf(errstr,"Not enough pitchdata to convert to brktable.\n");
        return(DATA_ERROR);
    }
    q = dz->parray[array_no];
    n = 0;
    *q++ = 0.0;
    *q++ = (double)*p++;
    *q++ = frametime;
    *q++ = (double)*p++;
    n = 2;
    while(endptab-p > 0) {
        *q++ = (double)n * frametime;
        *q++ = (double)*p++;
        n++;
    }
    bsize = q - dz->parray[array_no];

    if((exit_status = incremental_pt_datareduction(array_no,&bsize,sharpness,dz))<0)
        return(exit_status);
    n = bsize;

    if((dz->parray[array_no] = (double *)realloc((char *)dz->parray[array_no],n*sizeof(double)))==NULL) {
        sprintf(errstr,"convert_pch_or_transpos_data_to_brkpnttable()\n");
        return(MEMORY_ERROR);
    }
    *brksize = n/2;
    return(FINISHED);
}
    
/***************************** PT_DATAREDUCE **********************
 *
 * Reduce data on passing from pitch or transposition to brkpnt representation.
 *
 * Note, these are identical, becuase the root-pitch from which pitch-vals are measured
 * cancels out in the maths (and doesn't of course occur in the transposition calc).
 *
 * Take last 3 points, and if middle point has (approx) same value as
 * a point derived by interpolating between first and last points, then
 * ommit midpoint from brkpnt representation.
 */

int pt_datareduce(double **q,double sharp,double flat,double *thisarray,int *bsize)
{
    double interval;
    double *arrayend;
    double *p = (*q)-6, *midpair = (*q)-4, *endpair = (*q)-2;
    double startime = *p++;
    double startval = LOG2(*p++);                               /* pitch = LOG(p/root) = LOG(p) - LOG(root) */
    double midtime  = *p++;
    double midval   = LOG2(*p++);                               /* pitch = LOG(p/root) = LOG(p) - LOG(root) */
    double endtime  = *p++;
    double endval   = LOG2(*p);
    double valrange     = endval-startval;                      /* LOG(root) cancels out */
    double midtimeratio = (midtime-startime)/(endtime-startime);
    double guessval     = (valrange * midtimeratio) + startval;  /*  -LOG(root) reintroduced */

    if((interval = (guessval - midval) * SEMITONES_PER_OCTAVE) < sharp && interval > flat) { /* but cancels again */
        arrayend = thisarray + *bsize;
        while(endpair < arrayend)
            *midpair++ = *endpair++;
        (*q) -= 2;
        *bsize -= 2;
    }
    return(FINISHED);
}

/***************************** INCREMENTAL_PT_DATAREDUCTION ******************************/

int incremental_pt_datareduction(int array_no,int *bsize,double sharp,dataptr dz)
{
    int exit_status;
    double flat;
    double *q;
    double sharp_semitones = LOG2(dz->is_sharp) * SEMITONES_PER_OCTAVE; 
    double *thisarray = dz->parray[array_no];
    while(*bsize >= 6 && sharp < sharp_semitones) { 
        flat = -sharp;
        q = thisarray + 4;
        while(q < thisarray + (*bsize) - 1) {
            q +=2;
            if((exit_status = pt_datareduce(&q,sharp,flat,thisarray,bsize))<0)
                return(exit_status);
        }
        sharp *= 2.0;               /* interval-size doubles */
    }
    if(*bsize >= 6) { 
        sharp = sharp_semitones;
        flat = -sharp;
        q = thisarray + 4;
        while(q < thisarray + (*bsize) - 1) {
            q +=2;
            if((exit_status = pt_datareduce(&q,sharp,flat,thisarray,bsize))<0)
                return(exit_status);
        }
    }
    return(FINISHED);
}

/****************************** SPLICE_MULTILINE_STRING ******************************/

void splice_multiline_string(char *str,char *prefix)
{
    char *p, *q, c;
    p = str;
    q = str;
    while(*q != ENDOFSTR) {
        while(*p != '\n' && *p != ENDOFSTR)
            p++;
        c = *p;
        *p = ENDOFSTR;
        fprintf(stdout,"%s %s\n",prefix,q);
        *p = c;
        if(*p == '\n')
             p++;
        while(*p == '\n') {
            fprintf(stdout,"%s \n",prefix);
            p++;
        }
        q = p;
        p++;
    }
}
