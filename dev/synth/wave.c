/*
 * Copyright (c) 1983-2023 Trevor Wishart and Composers Desktop Project Ltd
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

/* RWD 22/02/2018  changed sndfile interleave code to eliminate glitch on ch2
   only suffix number to outfile name for sloom 
 */

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <cdpmain.h>
#include <synth.h>
#include <processno.h>
#include <modeno.h>
#include <pnames.h>
#include <flags.h>
#include <arrays.h>
#include <math.h>
#include <logic.h>
#include <speccon.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>         /*RWD*/
//TW UPDATE
#include <limits.h>
#include <filetype.h>

static int  gentable(int tabsize,dataptr dz);
static int  gen_wave(int sampdur,double sr,dataptr dz);
static int  gen_silence(int sampdur,dataptr dz);
static int  gen_noise(int sampdur,double sr,dataptr dz);
static int  update_wave_params(double *ampstep,double *frqstep,double *lastamp,double *lastfrq,double inverse_sr,
            int *samptime, int *nextsamptime,int blokstep,dataptr dz);
static double getval(double *tab,int i,double fracstep,double amp);
static int advance_in_table(int *i,double convertor,double *step,double *fracstep,
double *amp,double *frq,double dtabsize,double *ampstep,double *frqstep,int *samptime,int *nextsamptime,
double *lastamp,double *lastfrq,double inverse_sr,int blokstep,dataptr dz);

void set_band_limits(int clength,int *supertight, double *bwidth, int *minchan, int *centrchan,
    double *hfbwidth, int *lo_n, int *hi_n, double *offset, double hfchwidth, dataptr dz);
int do_stereo_specsynth(dataptr dz);
//TW UPDATE
static int gen_clicktrack(dataptr dz);
static int gen_chord(int sampdur,dataptr dz);

/******************************* DO_SYNTH ******************************/

int do_synth(dataptr dz)
{
    int exit_status;
    int sampdur = 0;
    double sr = 0.0;

//TW UPDATES
    if(dz->process == CLICK)
        dz->infile->channels = 1;
    else {
        dz->infile->channels = dz->iparam[SYN_CHANS];
        dz->infile->srate    = dz->iparam[SYN_SRATE];
        sr = (double)dz->infile->srate;
        sampdur = round(sr * dz->param[SYN_DUR]) * dz->iparam[SYN_CHANS];
        dz->tempsize = sampdur;
    }
    if(dz->floatsam_output)
        dz->infile->stype = SAMP_FLOAT;
    else
        dz->infile->stype = SAMP_SHORT;

    switch(dz->process) {
    case(SYNTH_SIL):    return gen_silence(sampdur,dz);
    case(SYNTH_NOISE):  return gen_noise(sampdur,sr,dz);
//TW UPDATES
    case(CLICK):        return gen_clicktrack(dz);
    }
    if((dz->parray[SYNTH_TAB] = (double *)malloc((dz->iparam[SYN_TABSIZE]+1) * sizeof(double)))==NULL) {
        sprintf(errstr,"Insufficient memory for wave table.\n");
        return(MEMORY_ERROR);
    }
    if(dz->process == MULTI_SYN)
        dz->mode =  WAVE_SINE;
    if((exit_status = gentable(dz->iparam[SYN_TABSIZE],dz))<0)
        return(exit_status);
    if(dz->process == MULTI_SYN)
        return gen_chord(sampdur,dz);
    return gen_wave(sampdur,sr,dz);
}

/******************************* GENTABLE ******************************/

int gentable(int tabsize,dataptr dz)
{   
    int n, m, k0, k1;
    double minstep, step;
    double *tab = dz->parray[SYNTH_TAB];
    switch(dz->mode) {
    case(WAVE_SINE):                
        step = (2.0 * PI)/(double)tabsize;
        for(n=0;n<tabsize;n++)
            tab[n] = sin(step * (double)n) * F_MAXSAMP;
        tab[tabsize] = 0.0;
        break;
    case(WAVE_SQUARE):              
        k0 = tabsize/2;
        for(n=0;n<k0;n++)
            tab[n] = F_MAXSAMP;
        for(n=k0;n<=tabsize;n++)
            tab[n] = -F_MAXSAMP;
        break;
    case(WAVE_SAW):                 
        k0 = tabsize/4;
        k1 = tabsize/2;
        minstep = F_MAXSAMP/(double)k0;
        tab[0] = 0.0;
        for(n=1;n<=k0;n++)
            tab[n] = tab[n-1] + minstep;
        for(n=k0,m=k0-1;n<k1;n++,m--)
            tab[n] = tab[m];
        for(n=k1,m=0;n<tabsize;n++,m++)
            tab[n] = -tab[m];
        tab[n] = 0.0;
        break;
    case(WAVE_RAMP):                    
        k0 = tabsize/2;
        minstep = F_MAXSAMP/(double)k0;
        tab[0] = F_MAXSAMP;
        for(n=1;n<k0;n++)
            tab[n] = tab[n-1] - minstep;
        tab[k0] = 0.0;
        for(n=k0+1,m=k0-1;n<=tabsize;n++,m--)
            tab[n] = -tab[m];
        break;
    default:
        sprintf(errstr,"Unknown case in gentable()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** GEN_WAVE *************************/

int gen_wave(int sampdur,double sr,dataptr dz)
{
    int exit_status;
    double inverse_sr = 1.0/sr;
    double step;
    double dtabsize = (double)dz->iparam[SYN_TABSIZE];
    double convertor = dtabsize * inverse_sr;
    int m, i, k;
    int chans = dz->infile->channels;
    double timestep = 10.0 * MS_TO_SECS, ampstep, frqstep, lastamp, lastfrq, amp, frq;
    int blokstep   = round(timestep * sr);
    int samptime = 0, nextsamptime = blokstep;
    double *tab = dz->parray[SYNTH_TAB];
    double fracstep = 0.0, val;
    int do_start = 1;
    int synth_splicelen = SYNTH_SPLICELEN * chans;
    int total_samps = 0;
    int startj = 0, endj = SYNTH_SPLICELEN;
    int total_samps_left = sampdur;
    int endsplicestart = sampdur - synth_splicelen;
    int todo;
    
    step = 0.0;
    i = 0;

    if(sampdur < (synth_splicelen * 2) + chans) {
        fprintf(stdout,"ERROR: Specified output duration is less then available splicing length.\n");
        return(DATA_ERROR);
    }

    if((exit_status = read_values_from_all_existing_brktables(0.0,dz))<0)
        return(exit_status);
    if((exit_status = 
    update_wave_params(&ampstep,&frqstep,&lastamp,&lastfrq,inverse_sr,&samptime,&nextsamptime,blokstep,dz))<0)
        return(exit_status);
    amp = lastamp;
    frq = lastfrq;
    while(total_samps_left > 0) {
        if(total_samps_left/dz->buflen <= 0)
            todo = total_samps_left;
        else
            todo = dz->buflen;
        m = 0;
        while(m < todo) {
            val = getval(tab,i,fracstep,amp);
            if(do_start) {
                val *= (startj++/(double)SYNTH_SPLICELEN); 
                if(startj >= SYNTH_SPLICELEN)
                    do_start = 0;
            }
            if(total_samps >= endsplicestart)
                val *= (endj--/(double)SYNTH_SPLICELEN);
            for(k=0;k<chans;k++) {
//              dz->bigbuf[m++] = (short)round(val);
                dz->bigbuf[m++] = (float)val;
                total_samps++;
            }
            if((exit_status = advance_in_table(&i,convertor,&step,&fracstep,&amp,&frq,dtabsize,
            &ampstep,&frqstep,&samptime,&nextsamptime,&lastamp,&lastfrq,inverse_sr,blokstep,dz))<0)
                return(exit_status);
        }
        if(todo) {
            if((exit_status = write_samps(dz->bigbuf,todo,dz))<0)
                return(exit_status);
        }
        total_samps_left -= dz->buflen;
    }
    return(FINISHED);
}
  
/****************************** GEN_SILENCE *************************/

int gen_silence(int sampdur,dataptr dz)
{
    int exit_status;
    int n, bufcnt = sampdur/dz->buflen;
    int remain = sampdur - (bufcnt * dz->buflen);
    for(n=0;n<dz->buflen;n++)
        dz->bigbuf[n] = (float)0;
    for(n=0;n<bufcnt;n++) {
        if((exit_status = write_samps(dz->bigbuf,dz->buflen,dz))<0)
            return(exit_status);
    }
    if(remain)
        return write_samps(dz->bigbuf,remain,dz);
    return FINISHED;
}
  
/****************************** GEN_NOISE *************************/

int gen_noise(int sampdur,double sr,dataptr dz)
{
    int exit_status;
    int n;
    int k, chans = dz->infile->channels;
    double amp, lastamp,lastfrq,ampstep,frqstep;
    double inverse_sr = 1.0/sr;
    double timestep = 10.0 * MS_TO_SECS;
    int blokstep = round(timestep * sr);
    int samptime = 0, nextsamptime = blokstep;
    int total_samps = 0;
    int synth_splicelen = SYNTH_SPLICELEN * chans, startj = 0, endj = SYNTH_SPLICELEN;
    int endsplicestart = sampdur - synth_splicelen;
    int do_start = 1;
    int total_samps_left = sampdur;
    int todo;
    double val;

    if(sampdur < (synth_splicelen * 2) + chans) {
        fprintf(stdout,"ERROR: Specified output duration is less then available splicing length.\n");
        return(DATA_ERROR);
    }

    if((exit_status = read_values_from_all_existing_brktables(0.0,dz))<0)
        return(exit_status);
    if((exit_status = 
    update_wave_params(&ampstep,&frqstep,&lastamp,&lastfrq,inverse_sr,&samptime,&nextsamptime,blokstep,dz))<0)
        return(exit_status);
    amp = lastamp;
    while(total_samps_left > 0) {
        if(total_samps_left/dz->buflen <= 0)
            todo = total_samps_left;
        else
            todo = dz->buflen;
        for(n=0;n<todo;n+=chans,total_samps += chans) {
            val = ((drand48() * 2.0) - 1.0) * amp;
            if(do_start) {
                val *= startj++/(double)SYNTH_SPLICELEN; 
                if(startj >= SYNTH_SPLICELEN)
                    do_start = 0;
            }
            if(total_samps >= endsplicestart)
                val *= endj--/(double)SYNTH_SPLICELEN; 
            for(k=0;k<chans;k++)
                dz->bigbuf[n+k] = (float)val;
            amp += ampstep;
            if(++samptime >= nextsamptime) {
                if((exit_status = update_wave_params
                (&ampstep,&frqstep,&lastamp,&lastfrq,inverse_sr,&samptime,&nextsamptime,blokstep,dz))<0)
                    return(exit_status);
                amp = lastamp;
            }
        }
        if(todo) {
            if((exit_status = write_samps(dz->bigbuf,todo,dz))<0)
                return(exit_status);
        }
        total_samps_left -= dz->buflen;
    }
    return(FINISHED);
}
  
/****************************** GETVAL *************************/

double getval(double *tab,int i,double fracstep,double amp)
{
    double diff, val  = tab[i];
    
    diff = tab[i+1] - val;
    val += diff * fracstep;
    val *= amp;
    return(val);
}

/****************************** UPDATE_WAVE_PARAMS *************************/

int update_wave_params(double *ampstep,double *frqstep,double *lastamp,double *lastfrq,double inverse_sr,
    int *samptime, int *nextsamptime,int blokstep,dataptr dz)
{   
    int exit_status;
    double thistime = (double)(*nextsamptime) * inverse_sr;
    double ratio;
    *lastfrq = dz->param[SYN_FRQ];
    *lastamp = dz->param[SYN_AMP];
    *ampstep = 0.0;
    *frqstep = 1.0;
    if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
        return(exit_status);
    if(dz->brksize[SYN_AMP])
        *ampstep = (dz->param[SYN_AMP] - *lastamp)/(double)blokstep;

    if(dz->brksize[SYN_FRQ]) {
        ratio    = dz->param[SYN_FRQ]/(*lastfrq);
        *frqstep = pow(10.0,log10(ratio)/(double)blokstep);
    }

    *samptime      = *nextsamptime;
    *nextsamptime += blokstep;
    return(FINISHED);
}

/****************************** ADVANCE_IN_TABLE *************************/

static int advance_in_table(int *i,double convertor,double *step,double *fracstep,
double *amp,double *frq,double dtabsize,double *ampstep,double *frqstep,int *samptime,int *nextsamptime,
double *lastamp,double *lastfrq,double inverse_sr,int blokstep,dataptr dz)
{
    int exit_status;

    *step += *frq * convertor;
    while(*step >= dtabsize)
        *step -= dtabsize;
    *i = (int)(*step); /* TRUNCATE */
    *fracstep = *step - (double)(*i);
    *amp += *ampstep;
    *frq *= *frqstep;
    if(++(*samptime) >= *nextsamptime) {
        if((exit_status = update_wave_params
        (ampstep,frqstep,lastamp,lastfrq,inverse_sr,samptime,nextsamptime,blokstep,dz))<0)
            return(exit_status);
        *amp = *lastamp;
        *frq = *lastfrq;
    }
    return(FINISHED);
}

/****************************** DO_STEREO_SPECSYNTH *************************/

#define SAFETY (10)

//TW CONVERTED
int do_stereo_specsynth(dataptr dz)
{
    int exit_status;
    double hfchwidth = dz->chwidth/2.0;
    float *bufend;
    double focusrange = 0.0, basefrq, frq, *frqq, bwidth = 0.0, hfbwidth, offset = 0.0, totalamp;
    double thisfocus, ampscaler, amp, wobble, dcf, hfchansprd;
    int minchan = 0, supertight, lo_n = 0, hi_n = 0, wcnt, jj, n, m, k;
    int remainder, samps_written;
    int clength = dz->wanted/2, centrchan = 0;
    int orig_process_type;
    char outfnam[256], outfilename[256];
    unsigned int samps_so_far;
    int floats_out =  dz->wlength * dz->wanted;
    int samps_read1, samps_read2, samps_read;
    int OK = 1;
    int orig_process, orig_proctype, orig_chans, orig_stype;
    int orig_srate;
//TW UPDATE CORRECTION
    int  overlap = (dz->infile->channels - 2)/dz->infile->Dfac;
    int snd_floats_out = floats_out/overlap;
    extern int sloom;
    
    dz->tempsize = (floats_out + snd_floats_out) * 2;

    if((frqq = (double *)malloc(clength * sizeof(double)))==NULL) {
        sprintf(errstr,"No memory for initial-frequency storage-array.\n");
        return(MEMORY_ERROR);
    }
    bufend = dz->bigfbuf + dz->big_fsize;

    if(!dz->brksize[SS_FOCUS2] && !dz->brksize[SS_FOCUS])
        focusrange = dz->param[SS_FOCUS2] - dz->param[SS_FOCUS];

    /* SET BAND LIMITS */

    if(!dz->brksize[SS_SPREAD] && !dz->brksize[SS_CENTRFRQ])
        set_band_limits(clength,&supertight,&bwidth,&minchan,&centrchan,
            &hfbwidth,&lo_n,&hi_n,&offset,hfchwidth,dz);

    outfnam[0] = ENDOFSTR;
    strcat(outfnam,dz->wordstor[0]);
//TW REVISION for new protocol: always pass file-extension on cmdline
    if(sloom)
        delete_filename_lastchar(outfnam);  /* axe trailing zero on outfilename */
    samps_so_far = 0;

    for(jj=1;jj<3;jj++) {
        strcpy(outfilename,outfnam);
//TW REVISION for new protocol: always pass file-extension on cmdline
        insert_new_number_at_filename_end(outfilename,jj+2,0);
        orig_process_type = dz->process_type;
        dz->process_type = BIG_ANALFILE;
        dz->outfiletype = ANALFILE_OUT;
        if((exit_status = create_sized_outfile(outfilename,dz)) < 0) {
            sprintf(errstr,"Cannot open temporary analysis file '%s' to generate sound data:%s\n", outfilename,sferrstr());
            return(SYSTEM_ERROR);
        }
        // dz->outfiletype = NO_OUTPUTFILE;  /* TW */ Thought I was, for safety, resetting to its original val (changed above) but did it wrongly, but not necessary.
        dz->total_windows = 0;
        dz->time = (float)0.0;
        dz->flbufptr[0] = dz->bigfbuf;
        for(wcnt = 0; wcnt < dz->wlength; wcnt++) {
            memset((char *)dz->flbufptr[0],0,dz->wanted * sizeof(float));
            if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
                return(exit_status);
            if(dz->brksize[SS_FOCUS2] || dz->brksize[SS_FOCUS])
                focusrange = dz->param[SS_FOCUS2] - dz->param[SS_FOCUS];
            if(dz->brksize[SS_SPREAD] || dz->brksize[SS_CENTRFRQ])
                set_band_limits(clength,&supertight,&bwidth,&minchan,&centrchan,
                    &hfbwidth,&lo_n,&hi_n,&offset,hfchwidth,dz);
            hfchansprd = (hi_n - lo_n)/2.0;
            totalamp = 0.0;
            for(n=lo_n, m = minchan; n<hi_n; n++,m++) {
                basefrq = ((double)n * bwidth) + offset;
                if(dz->total_windows==0) {                                  /* for 1st window, establish a channel frq */
                    frq = (drand48() * bwidth) + basefrq;
                    frqq[n] = frq;
                } else if(dz->param[SS_TRAND] > 0.0) {                  /* if chan frq wobbles thro time, vary it */
                    wobble = ((drand48() * 2.0) - 1.0)/2.0;
                    frq = frqq[n] + (wobble * bwidth * dz->param[SS_TRAND]);
                } else                                                      /* if not, retain frq from first window */
                    frq = frqq[n];
                dcf = (double)abs(centrchan - m)/hfchansprd;    /* fraction of chan-sprd we are away from centre */

                thisfocus = (drand48() * focusrange) + dz->param[SS_FOCUS];
                thisfocus = 1.0/thisfocus;
                dcf = pow(dcf,thisfocus);                                   /* squeeze or unsqueeze cosin distrib */
                amp = (cos(dcf * PI) + 1.0)/2.0;                            /* cosin distrib */
                totalamp += amp;
                k = m * 2;
                if(dz->total_windows)
                    dz->flbufptr[0][k] = (float)amp;
                dz->flbufptr[0][++k]   = (float)frq;
            }
            if(dz->total_windows) {
                ampscaler = (double)SPECSYN_MAXAMP/totalamp;
                for(n=lo_n, k = minchan*2; n<hi_n; n++,k+=2)
                    dz->flbufptr[0][k] = (float)(dz->flbufptr[0][k] * ampscaler);
            }
            if((dz->flbufptr[0] += dz->wanted) >= bufend) {
                if((exit_status = write_samps_no_report(dz->bigfbuf,dz->buflen,&samps_written,dz)) < 0)
                    return(exit_status);
                display_virtual_time(dz->total_samps_written + samps_so_far,dz);
                dz->flbufptr[0] = dz->bigfbuf;
            }
            dz->time = (float)(dz->time + dz->frametime);
            dz->total_windows++;
        }
        if((remainder = dz->flbufptr[0] - dz->bigfbuf) > 0) {
            if((exit_status = write_samps_no_report(dz->bigfbuf,remainder,&samps_written,dz)) < 0)
                return(exit_status);
            display_virtual_time(dz->total_samps_written + samps_so_far,dz);
        }
        dz->process_type = orig_process_type;

/* SEE NOTES IN pvoc_addon.c in PVOC directory */
        if((exit_status = pvoc_out(floats_out,&samps_so_far,outfilename,outfnam,jj,dz))<0)
            return(exit_status);
    }
//TW NEW: merge the 2 output files
    reset_file_params_for_sndout(&orig_proctype,&orig_srate,&orig_chans,&orig_stype,dz);
    for(jj=0;jj<2;jj++) {
        strcpy(outfilename,outfnam);
        insert_new_number_at_filename_end(outfilename,jj+1,0);
        if((dz->ifd[jj] = sndopenEx(outfilename,0,CDP_OPEN_RDONLY)) < 0) {
            sprintf(errstr,"Cannot reopen mono file '%s' to convert to stereo :%s\n", outfilename,sferrstr());
            return(SYSTEM_ERROR);
        }
    }
    dz->insams[0] *= 2;
    dz->process_type = EQUAL_SNDFILE;
    dz->infile->channels = dz->outfile->channels = STEREO;
    dz->infile->filetype = SNDFILE; /* forces formatted sndfile creation */
    strcpy(outfilename,outfnam);
    if(dz->floatsam_output)
        dz->infile->stype = SAMP_FLOAT;
    else
        dz->infile->stype = SAMP_SHORT;
    dz->infile->srate = round(dz->param[SS_SRATE]);
    dz->outfiletype = SNDFILE_OUT;
    orig_process = dz->process;
    dz->process = EDIT_CUT; /* any process creating a standard sound outfile */
    dz->infile->srate = (int)round(dz->param[SS_SRATE]);
    if(sloom)                                                       /* TW 24/2/18 */
        insert_new_number_at_filename_end(outfilename,0,0);
    if((exit_status = create_sized_outfile(outfilename,dz)) < 0) {
        sprintf(errstr,"Cannot open temporary soundfile '%s' to generate sound data:%s\n", outfilename,sferrstr());
        return(SYSTEM_ERROR);
    }
    if((exit_status = reset_peak_finder(dz))<0)
        return(exit_status);
    dz->insams[0] /= 2;
    OK = 1;
    while(OK) {
        memset((char *)dz->bigfbuf,0,dz->buflen * sizeof(float));
        memset((char *)dz->flbufptr[2],0,dz->buflen * sizeof(float));
        memset((char *)dz->flbufptr[3],0,dz->buflen * sizeof(float));
        if((samps_read1 = fgetfbufEx(dz->flbufptr[2],dz->buflen,dz->ifd[0],0))<=0) {
            if(samps_read1 < 0) {
                sprintf(errstr,"Sample read failed: ");
                for(jj=0;jj<2;jj++) {
                    char pfname[_MAX_PATH];
                    strcpy(pfname, snd_getfilename(dz->ifd[jj]));
                    sndcloseEx(dz->ifd[jj]);
                    if(remove(/*outfilename*/pfname)<0) {
                        fprintf(stdout,"WARNING: Tempfile %s not removed: ",outfilename);
                        fflush(stdout);
                    }
                }
                strcat(errstr,"\n");
                return(SYSTEM_ERROR);
            } else
                break;
        }
        if((samps_read2 = fgetfbufEx(dz->flbufptr[3],dz->buflen,dz->ifd[1],0))<=0) {
            if(samps_read2< 0) {
                sprintf(errstr,"Sample read failed: ");
                for(jj=0;jj<2;jj++) {
                    char pfname[_MAX_PATH];
                    strcpy(pfname, snd_getfilename(dz->ifd[jj]));
                    sndcloseEx(dz->ifd[jj]);
                    if(remove(/*outfilename*/pfname)<0) {
                        fprintf(stdout,"WARNING: Tempfile %s not removed: ",outfilename);
                        fflush(stdout);
                    }
                }
                strcat(errstr,"\n");
                return(SYSTEM_ERROR);
            } else
                break;
        }
        if((samps_read = min(samps_read1,samps_read2)) <= 0)
            break;
#ifdef NOTDEF
        for(n = 0,k=0; n < samps_read-1;n++,k+=2) {
            dz->bigfbuf[k] = dz->flbufptr[2][n];
            dz->bigfbuf[k+1] = dz->flbufptr[3][n];
        }
        dz->bigfbuf[k] = dz->flbufptr[2][n];
#else
        for(n = 0,k=0; n < samps_read;n++,k+=2) {
            dz->bigfbuf[k] = dz->flbufptr[2][n];
            dz->bigfbuf[k+1] = dz->flbufptr[3][n];
        }
#endif
        /* last samp remains in situ */
        if((exit_status = write_samps(dz->bigfbuf,samps_read * 2,dz))<0) {
            sprintf(errstr,"Sample write failed: ");
            for(jj=0;jj<2;jj++) {
                char pfname[_MAX_PATH];
                strcpy(pfname, snd_getfilename(dz->ifd[jj]));
                sndcloseEx(dz->ifd[jj]);
                if(remove(/*outfilename*/pfname)<0) {
                    fprintf(stdout,"WARNING: Tempfile %s not removed: ",outfilename);
                    fflush(stdout);
                }
            }
            strcat(errstr,"\n");
            return(SYSTEM_ERROR);
        }
    }
    for(jj=0;jj<2;jj++) {
        /*RWD Feb 2004 */
        char pfname[_MAX_PATH];
        strcpy(pfname, snd_getfilename(dz->ifd[jj]));
        sndcloseEx(dz->ifd[jj]);
        if(remove(/*outfilename*/pfname)<0) {
            fprintf(stdout,"WARNING: Tempfile %s not removed: ",outfilename);
            fflush(stdout);
        }
    }
    dz->process = orig_process; 
// In order to correctly write header, process_type should remain EQUAL_SNDFILE
//  dz->process_type = OTHER_PROCESS;
    return(FINISHED);
}



/****

With cosinusoidal energy distribution
-------------------------------------

DCF(dist-from-centre-frq) = fabs(frq - centre-frq)          (range 0 - 1)
                            -------------------
                            half-interval-spread

To focus energy towards centre, we squeeze values towards position 0
--------------------------------------------------------------------

DCF^focus

if(focus > 1) squeezes
if(focus < 1) braodens


ampscaler = (cos(CDF*PI) + 1)/2.0;

****/

/****************************** SET_BAND_LIMITS *************************/

void set_band_limits(int clength,int *supertight, double *bwidth, int *minchan, int *centrchan, 
    double *hfbwidth, int *lo_n, int *hi_n, double *offset, double hfchwidth, dataptr dz)

{
    int chancnt;
    double hifrq, lofrq, frqsprd;
    int lochan, hichan, chanspreadmin;

    if(dz->vflag[SS_PICHSPRD]) {
        if(dz->param[SS_SPREAD] < 1.0)
            dz->param[SS_SPREAD] = 1.0/dz->param[SS_SPREAD];
        hifrq = min(dz->param[SS_CENTRFRQ] * dz->param[SS_SPREAD],dz->nyquist);
        lofrq = max(dz->param[SS_CENTRFRQ] / dz->param[SS_SPREAD],SPEC_MINFRQ);
    } else {
        hifrq = min(dz->param[SS_CENTRFRQ] + dz->param[SS_SPREAD],dz->nyquist);
        lofrq = max(dz->param[SS_CENTRFRQ] - dz->param[SS_SPREAD],SPEC_MINFRQ);
    }
    frqsprd = hifrq - lofrq;
    /* ESTABLISH WHICH CHANS INVOLVED */

    lochan = (int)floor((lofrq + hfchwidth)/dz->chwidth) - 1;
    hichan = (int)floor((hifrq + hfchwidth)/dz->chwidth) - 1;
//TW UPDATES
    lochan = max(lochan,0);
    hichan = max(hichan,0);

    chancnt = hichan - lochan + 1;
    chanspreadmin = (CHANUP * 2) + 1;
    if (chancnt < chanspreadmin) {
        chancnt = chanspreadmin;
        *bwidth = frqsprd/chanspreadmin;
        *centrchan = (int)floor((dz->param[SS_CENTRFRQ] + hfchwidth)/dz->chwidth) - 1;
        if(*centrchan - CHANUP < 0)
            *minchan = 0;
        else if (*centrchan + CHANUP > clength)
            *minchan = clength - chanspreadmin;
        else
            *minchan = *centrchan - CHANUP;
        *centrchan = *minchan + CHANUP;
        *supertight = 1;
    } else {
        *bwidth = dz->chwidth;
        *minchan = lochan;
        *supertight = 0;
    }
    *hfbwidth = (*bwidth)/2.0;

    if(*supertight) {
        *lo_n = 0;
        *hi_n = chancnt;
        *offset = lofrq;
    } else {
        *lo_n   = lochan;
        *hi_n   = hichan;
        *centrchan = (lochan + hichan)/2;
        *offset = -(*hfbwidth);
    }
}

//TW UPDATE: New Function
/****************************** GEN_CLICKTRACK *************************/

#define LINE_TIMES  (0)

int gen_clicktrack(dataptr dz)
{
    double *clikdata = dz->parray[0], *clik = dz->parray[1];
    double amp, tottime;
    int n, m, j = 0, bufsamptime, bufsampendtime, samptime, tcnt = 0;
    float *buf = dz->sampbuf[0];
    int k, exit_status;
    int from_start = 1;
    int to_end = 1;
    double click_ofset = 0.0, starttime_offset = 0.0;

    if(dz->mode == CLICK_BY_LINE) {
        if(dz->iparam[CLIKOFSET] > 0)
            click_ofset = dz->parray[2][dz->iparam[CLIKOFSET]];
        dz->param[CLIKSTART] = dz->parray[2][dz->iparam[CLIKSTART]] - click_ofset;
        if(dz->iparam[CLIKEND] + 1 == dz->iparam[CLICKTIME])
            dz->iparam[CLIKEND]++;
        else
            to_end = 0;
        dz->param[CLIKEND]   = dz->parray[2][dz->iparam[CLIKEND]] - click_ofset;
    }
    if(sloom) {
        tottime = dz->param[CLIKEND] - dz->param[CLIKSTART];
        if((dz->tempsize = (int)round(tottime * CLICK_SRATE)) < 0) {
            fprintf(stdout,"WARNING: File is too long to give accurate progress display on Progress Bar.\n");
            fflush(stdout);
            dz->tempsize = INT_MAX;
        }
    }
    if(!flteq(dz->param[CLIKSTART],0.0))
        from_start = 0;
    if(dz->iparam[CLIKOFSET] > 0) {
        click_ofset = dz->parray[2][dz->iparam[CLIKOFSET]];
        if(!from_start)
            starttime_offset = dz->param[CLIKSTART] + click_ofset;
        dz->param[CLIKEND]   += click_ofset;
        fprintf(stdout,"INFO: MUSIC STARTS %lf secs AFTER CLICKS\n",click_ofset);
        fflush(stdout);
    } else
        starttime_offset = dz->param[CLIKSTART];
    for(n=dz->itemcnt-2;n >=0;n-=2) {   /* throw away data beyond endtime of generating clicks */

        if((dz->parray[0][n] < dz->param[CLIKEND]) || flteq(dz->parray[0][n],dz->param[CLIKEND])) {
            dz->itemcnt = n+2;
            break;
        }
    }
    if(!to_end)
        dz->itemcnt -= 2;
    if(!from_start) {                   /* throw away data before starttime of generating clicks */
        while(dz->parray[0][0] < starttime_offset) {
            dz->parray[0]+=2;
            if((dz->itemcnt-=2) <= 0) {
                sprintf(errstr,"No click data found between the specified times.\n");
                return(DATA_ERROR);
            }
        }
        for(n=0;n < dz->itemcnt;n+=2)   /* and adjust data time values so generation starts at outfile starttime 0.0 */
            dz->parray[0][n] -= starttime_offset;
    }
    if(dz->vflag[LINE_TIMES]) {
        tcnt = 0;                       /* skip over times-of-lines prior to start of click-generation */
        while(dz->parray[2][tcnt] < starttime_offset) {
            if(++tcnt >= dz->iparam[CLICKTIME]) {
                sprintf(errstr,"Anomaly in data about timing of data lines.\n");
                return(PROGRAM_ERROR);
            }
        }
    }
    clikdata = dz->parray[0];
    memset((char *)buf,0,dz->buflen * sizeof(float));
    for(n=0,m=1;n<dz->itemcnt;n+=2,m+=2) {
        samptime = (int)round(clikdata[n] * dz->infile->srate);
        if(dz->vflag[LINE_TIMES] && (tcnt < dz->iparam[CLICKTIME])) {
            if(flteq(dz->parray[2][tcnt],(clikdata[n] + starttime_offset))) {
                fprintf(stdout,"INFO: LINE  %d  at  %lf\n",tcnt+1,(dz->parray[2][tcnt] - click_ofset));
                fflush(stdout);
                tcnt++;
            } else if(dz->parray[2][tcnt] < (clikdata[n] + starttime_offset))
                tcnt++;                 /* skips lines which start with unaccented beat */
        }
        bufsamptime = samptime - dz->total_samps_written;
        while(bufsamptime >= dz->buflen) {
            if((exit_status = write_samps(dz->bigbuf,dz->buflen,dz))<0)
                return(exit_status);
            memset((char *)buf,0,dz->buflen * sizeof(float));
            bufsamptime = samptime - dz->total_samps_written;
        }
        bufsampendtime = bufsamptime + CLICKLEN + 1;
        amp = clikdata[m];
        k = 0;
        if(bufsampendtime <= dz->buflen) {
            for(j = bufsamptime; j < bufsampendtime; j++)
                buf[j] = (float)((double)clik[k++] * amp);
        } else {
            for(j = bufsamptime; j < dz->buflen; j++)
                buf[j] = (float)((double)clik[k++] * amp);
            if((exit_status = write_samps(dz->bigbuf,dz->buflen,dz))<0)
                return(exit_status);
            memset((char *)buf,0,dz->buflen * sizeof(float));
            j = 0;
            while(k <= CLICKLEN)
                buf[j++] = (float)((double)clik[k++] * amp);
        }
    }
    if(j > 0) {
        if((exit_status = write_samps(dz->bigbuf,j,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/****************************** GEN_CHORD *************************/

#define JITTER_RANGE (1.0)
#define JITTERSTEP (8)

int gen_chord(int sampdur,dataptr dz)
{
    int exit_status;
    double dtabsize = (double)dz->iparam[SYN_TABSIZE];
    int m, n, k, kk;
    int chans = dz->infile->channels;
    double amp = dz->param[SYN_AMP];
    double *tab = dz->parray[SYNTH_TAB];
    double val;
    int do_start = 1, jittercnt = 0;
    int synth_splicelen = SYNTH_SPLICELEN * chans;
    int total_samps = 0;
    int startj = 0, endj = SYNTH_SPLICELEN, this_chordend, this_chordstart, thischord_notecnt;
    int total_samps_left = sampdur;
    int endsplicestart = sampdur - synth_splicelen;
    int todo = 0, total_samps_done;
    
    double *step     = dz->parray[1];
    double *pos      = dz->parray[2];
    double *fracstep = dz->parray[3];
    int   *samppos  = dz->lparray[0];

    double *jitter=NULL, *lastjitter=NULL, *thisjitstep=NULL, *thisjitter=NULL;

    if ((jitter = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        fprintf(stdout,"WARNING: Insufficient memory to add jitter.\n");
        fflush(stdout);
    } else if ((thisjitter = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        fprintf(stdout,"WARNING: Insufficient memory (2) to add jitter.\n");
        fflush(stdout);
    } else if ((lastjitter = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        fprintf(stdout,"WARNING: Insufficient memory (3) to add jitter.\n");
        fflush(stdout);
    } else if ((thisjitstep = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        fprintf(stdout,"WARNING: Insufficient memory (4) to add jitter.\n");
        fflush(stdout);
    } else {
        for(n=0;n<dz->itemcnt;n++) {
            jitter[n] = step[n] * (pow(SEMITONE_INTERVAL,JITTER_RANGE) - 1.0); /* Jitter within 0.5 of a semitone */
            lastjitter[n] = 0.0;
            thisjitter[n] = ((drand48() * 2.0) - 1.0) * jitter[n];  /* +- within jitter range */
            thisjitstep[n] = thisjitter[n]/(double)JITTERSTEP;
        }
    }
    if(sampdur < (synth_splicelen * 2) + chans) {
        fprintf(stdout,"ERROR: Specified output duration is less then available splicing length.\n");
        return(DATA_ERROR);
    }
    if(dz->ringsize > 1) {
        total_samps_left = (int)round((double)(sampdur/chans)/(double)dz->ringsize) * chans;
        total_samps_done = 0;
        m = 0;
        this_chordend = -1;
        for(kk=0; kk < dz->ringsize; kk++) {
            do_start = 1;
            startj = 0;
            endj = SYNTH_SPLICELEN;
            if(kk == dz->ringsize - 1)
                total_samps_left = sampdur - total_samps_done;
            endsplicestart = total_samps_left - synth_splicelen;
            this_chordend++;
            this_chordstart = this_chordend;
            while(step[this_chordend] > 0) {
                if(++this_chordend >= dz->itemcnt)
                    break;
            }
            thischord_notecnt = this_chordend - this_chordstart;
            todo = min(m + total_samps_left,dz->buflen);
            total_samps = 0;
            while(total_samps_left > 0) {
                while(m < todo) {
                    val = 0.0;
                    for(n=this_chordstart;n < this_chordend;n++)
                        val += getval(tab,samppos[n],fracstep[n],amp);
                    val /= (double)thischord_notecnt;
                    if(do_start) {
                        val *= (startj++/(double)SYNTH_SPLICELEN); 
                        if(startj >= SYNTH_SPLICELEN)
                            do_start = 0;
                    }
                    if(total_samps >= endsplicestart)
                        val *= (endj--/(double)SYNTH_SPLICELEN);
                    for(k=0;k<chans;k++) {
                        dz->bigbuf[m++] = (float)val;
                        total_samps++;
                        total_samps_done++;
                    }
                    if(jitter != NULL) {
                        if(++jittercnt >= JITTERSTEP) {
                            for(n=this_chordstart;n<this_chordend;n++) {
                                lastjitter[n] = thisjitter[n];
                                thisjitter[n] = ((drand48() * 2.0) - 1.0) * jitter[n];  /* +- within jitter range */
                                thisjitstep[n] = (thisjitter[n] - lastjitter[n])/(double)JITTERSTEP;
                            }
                            jittercnt = 0;
                        }
                    }
                    for(n=this_chordstart;n<this_chordend;n++) {
                        pos[n] += step[n];
                        if(jitter != NULL)
                            pos[n] += thisjitstep[n];
                        while(pos[n] >= dtabsize)
                            pos[n] -= dtabsize;
                        samppos[n] = (int)pos[n];
                        fracstep[n] = pos[n] - (double)samppos[n];
                    }
                }
                total_samps_left -= todo;
                if(m >= dz->buflen) {
                    if((exit_status = write_samps(dz->bigbuf,dz->buflen,dz))<0)
                        return(exit_status);
                    todo = min(total_samps_left,dz->buflen);
                    m = 0;
                }
            } 
        }
        if(todo > 0) {
            if((exit_status = write_samps(dz->bigbuf,todo,dz))<0)
                return(exit_status);
        }
    } else {
        while(total_samps_left > 0) {
            if(total_samps_left/dz->buflen <= 0)
                todo = total_samps_left;
            else
                todo = dz->buflen;
            m = 0;
            while(m < todo) {
                val = 0.0;
                for(n=0;n < dz->itemcnt;n++)
                    val += getval(tab,samppos[n],fracstep[n],amp);
                val /= (double)dz->itemcnt;
                if(do_start) {
                    val *= (startj++/(double)SYNTH_SPLICELEN); 
                    if(startj >= SYNTH_SPLICELEN)
                        do_start = 0;
                }
                if(total_samps >= endsplicestart)
                    val *= (endj--/(double)SYNTH_SPLICELEN);
                for(k=0;k<chans;k++) {
                    dz->bigbuf[m++] = (float)val;
                    total_samps++;
                }
                if(jitter != NULL) {
                    if(++jittercnt >= JITTERSTEP) {
                        for(n=0;n<dz->itemcnt;n++) {
                            lastjitter[n] = thisjitter[n];
                            thisjitter[n] = ((drand48() * 2.0) - 1.0) * jitter[n];  /* +- within jitter range */
                            thisjitstep[n] = (thisjitter[n] - lastjitter[n])/(double)JITTERSTEP;
                        }
                        jittercnt = 0;
                    }
                }
                for(n=0;n<dz->itemcnt;n++) {
                    pos[n] += step[n];
                    if(jitter != NULL)
                        pos[n] += thisjitstep[n];
                    while(pos[n] >= dtabsize)
                        pos[n] -= dtabsize;
                    samppos[n] = (int)pos[n];
                    fracstep[n] = pos[n] - (double)samppos[n];
                }
            }
            if((exit_status = write_samps(dz->bigbuf,todo,dz))<0)
                return(exit_status);
            total_samps_left -= dz->buflen;
        }
    }
    return(FINISHED);
}
