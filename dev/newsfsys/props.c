/*
 * Copyright (c) 1983-2013 Martin Atkins, Richard Dobson and Composers Desktop Project Ltd
 * http://people.bath.ac.uk/masrwd
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



/*props.c*/
/*builtin- C version of props.cpp*/
/*RWD.5.99 private version to support WAVE_EX*/
/*RWD.5.99 NB NO SUPPORT FOR LONG INT FORMATS YET!*/
/*RWD Feb 2014 release 7, added 6.1 speaker format */
#include <stdio.h>
#include <stdlib.h>
#include <sfsys.h>
#include "sffuncs.h"
#include "chanmask.h"

char *props_errstr;

int sf_headread(int fd,SFPROPS *props)
{
    int srate,chans,samptype,origsize = 0,origrate=0,origchans = 0,dummy;
    float arate = (float)0.0;
    int wlen=0,dfac=0,specenvcnt = 0,checksum=0;
    channelformat chformat;

    props->window_size = 0.0f;
    props_errstr = NULL;
    if(props==NULL)
        return 0;
    if(fd <0){
        props_errstr = "Cannot read Soundfile: Bad Handle";
        return 0;
    }
    if(sfgetprop(fd,"sample rate", (char *)&srate, sizeof(int)) < 0) {
        props_errstr = "Failure to read sample rate";
        return 0;
    }
    if(sfgetprop(fd,"channels", (char *)&chans, sizeof(int)) < 0) {
        props_errstr ="Failure to read channel data";
        return 0;
    }
    if(sfgetprop(fd,"sample type", (char *)&samptype, sizeof(int)) < 0){
        props_errstr = "Failure to read sample size";
        return 0;
    }

    props->srate = srate;
    props->chans = chans;
    props->type = wt_wave;

    if(sf_getchanformat(fd,&chformat) < 0){
        props_errstr = "cannot find channel format information";
        return 0;
    }
    props->chformat = chformat;

    switch(samptype){
    case(SAMP_SHORT):
        props->samptype = SHORT16;
        break;
    case(SAMP_FLOAT):
        props->samptype = FLOAT32;
        break;
    case(SAMP_LONG):
        props->samptype = INT_32;
        break;
    case(SAMP_2432):
        props->samptype = INT2432;
        break;
    case(SAMP_2424):
        props->samptype = INT2424;
        break;
    case(SAMP_2024):
        props->samptype = INT2024;
        break;
    case(SAMP_MASKED):
        props->samptype = INT_MASKED;
        break;
    default:
        props_errstr = "unrecognised integer sample format";
        return 0;
        break;
    }

    /*now elaborate the format...*/
    if(chformat > STDWAVE && chformat != MC_BFMT){
        long chmask = sfgetchanmask(fd);

        /*we want to know the channel config, if any, and the brand of file*/
        if(chmask < 0) {
            props_errstr = "internal error: cannot read channel mask";
            return 0;
        }
        /*RWD Jan 2007 accept chancount > numbits set in speakermask */
        switch(chmask){
            /*check all cross-platform formats first...*/
        case(SPKRS_UNASSIGNED):
            props->chformat = MC_STD;
            break;
        case(SPKRS_MONO):
            if(props->chans==1)
                props->chformat = MC_MONO;
            else
                props->chformat = MC_WAVE_EX;   /*something weird...*/
            break;
        case(SPKRS_STEREO):
            if(props->chans==2)
                props->chformat = MC_STEREO;
            else
                props->chformat = MC_WAVE_EX;   /*something weird...*/
            break;
        case(SPKRS_GENERIC_QUAD):
            if(props->chans==4)
                props->chformat = MC_QUAD;
            else
                props->chformat = MC_WAVE_EX;   /*something weird...*/
            break;
        case(SPKRS_SURROUND_LCRS):
            if(props->chans==4)
                props->chformat = MC_LCRS;
            else
                props->chformat = MC_WAVE_EX;   /*something weird...*/
            break;
        case(SPKRS_DOLBY5_1):
            if(props->chans==6)
                props->chformat = MC_DOLBY_5_1;
            else
                props->chformat = MC_WAVE_EX;   /*something weird...*/
            break;
        case(SPKRS_SURR_6_1):
            props->chformat = MC_SURR_6_1;
            break;
        case(SPKRS_SURR_7_1):
            props->chformat = MC_SURR_7_1;
            break;
        case(SPKRS_CUBE):
            props->chformat = MC_CUBE;
            break;

        default:
            props->chformat = MC_WAVE_EX;   /*something weird...    */
            break;
        }
    }

    if(props->samptype==FLOAT32) {
        /*we know we have floats: is it spectral file?*/
        if(sfgetprop(fd,"original sampsize",(char *)&origsize, sizeof(int))<0){
            props_errstr = "Failure to read original sample size";
        }
        if(sfgetprop(fd,"original sample rate",(char *)&origrate,sizeof(int))<0){
            props_errstr = "Failure to read original sample rate";
        }
        if(sfgetprop(fd,"arate",(char *)&arate,sizeof(float)) < 0){
            props_errstr = "Failure to read analysis sample rate";
        }
        if(sfgetprop(fd,"analwinlen",(char *)&wlen,sizeof(int)) < 0){
            props_errstr = "Failure to read analysis window length";
        }
        if(sfgetprop(fd,"decfactor",(char *)&dfac,sizeof(int)) < 0){
            props_errstr = "Failure to read decimation factor";
        }
        checksum = origsize + origrate + wlen + dfac + (int)arate;
        if(checksum==0) {       /*its a wave file, or an envelope file*/
            int isenv = 0;

            float winsize;
            if(sfgetprop(fd,"is an envelope",(char *) &isenv,sizeof(int)) < 0){
                props_errstr = "Failure to read envelope property";
            }
            if(isenv){
                /*its an envelope file, get window size*/
                if(sfgetprop(fd,"window size",(char *)&winsize,sizeof(/*double*/float)) < 0) {
                    props_errstr = "Error reading window size in envelope file";
                    return 0;
                }
                props->window_size = /*(float)*/ winsize;
                props->type = wt_binenv;
            }


            /*return 1;*/
            /*tell props what brand of file this is - e.g WAVE_EX*/
            return (sfformat(fd,&props->format) == 0);

        }
        else {
            if(props_errstr==NULL){ /*its a good spectral file*/
                props->origsize = origsize;
                props->origrate = origrate;
                props->arate = arate;
                props->winlen = wlen;    /*better be wlen+2 ?*/
                props->decfac = dfac;
                /*props.chans = (wlen+2)/2;             //?? */
                if(sfgetprop(fd,"is a pitch file", (char *)&dummy, sizeof(int))>=0)
                    props->type = wt_pitch;
                else if(sfgetprop(fd,"is a transpos file", (char *)&dummy, sizeof(int))>=0)
                    props->type = wt_transposition;
                else if(sfgetprop(fd,"is a formant file", (char *)&dummy, sizeof(int))>=0)
                    props->type = wt_formant;
                else
                    props->type = wt_analysis;
            } else
                return 0;       /*somehow, got a bad analysis file...*/
        }
        /* get any auxiliary stuff for control file*/
        /*adapted from TW's function in speclibg.cpp*/
        switch(props->type){
        case(wt_formant):
            if(sfgetprop(fd,"specenvcnt",(char *)&specenvcnt,sizeof(int)) < 0){
                props_errstr = "Failure to read formant size in formant file";
                return 0;
            }
            props->specenvcnt = specenvcnt;

        case(wt_pitch):
        case(wt_transposition):
            if(props->chans != 1){ /*RWD: this makes old-style files illegal!*/
                props_errstr =  "Channel count does not equal to 1 formant,pitch or transposition file";
                return 0;
            }
            if(sfgetprop(fd,"orig channels", (char *)&origchans, sizeof(int)) < 0) {
                props_errstr =  "Failure to read original channel data in formant,pitch or transposition file";
                return 0;
            }
            props->origchans = origchans;
            break;
        default:
            break;
        }
        /*for a spectral or binenv file, we say standard channel format*/
        props->chformat = STDWAVE;      /*might be MC_STD in due course!*/

    }
    /*return 1;*/
    /*tell props what brand of file this is - e.g WAVE_EX*/
    return (sfformat(fd,&props->format) == 0);
}



int snd_headread(int fd,SFPROPS *props)
{
    int srate,chans,samptype,origsize = 0,origrate = 0, origchans = 0,dummy;
    float arate = (float)0.0;
    int wlen=0,dfac=0,specenvcnt = 0,checksum=0;
    channelformat chformat;
    props->window_size = 0.0f;

    props_errstr = NULL;
    if(props==NULL)
        return 0;
    if(fd <0){
        props_errstr = "Bad Soundfile Handle";
        return 0;
    }
    if(sndgetprop(fd,"sample rate", (char *)&srate, sizeof(int)) < 0) {
        props_errstr = "Failure to read sample rate";
        return 0;
    }
    if(sndgetprop(fd,"channels", (char *)&chans, sizeof(int)) < 0) {
        props_errstr ="Failure to read channel data";
        return 0;
    }
    if(sndgetprop(fd,"sample type", (char *)&samptype, sizeof(int)) < 0){
        props_errstr = "Failure to read sample size";
        return 0;
    }

    props->srate = srate;
    props->chans = chans;
    props->type = wt_wave;

    if(snd_getchanformat(fd,&chformat) < 0){
        props_errstr = "cannot find channel format information";
        return 0;
    }
    props->chformat = chformat;

    switch(samptype){
    case(SAMP_SHORT):
        props->samptype = SHORT16;
        break;
    case(SAMP_FLOAT):
        props->samptype = FLOAT32;
        break;
    case(SAMP_LONG):
        props->samptype = INT_32;
        break;
    case(SAMP_2432):
        props->samptype = INT2432;
        break;
    case(SAMP_2424):
        props->samptype = INT2424;
        break;
    case(SAMP_2024):
        props->samptype = INT2024;
        break;
    case(SAMP_MASKED):
        props->samptype = INT_MASKED;
        break;
    default:
        props_errstr = "unrecognised integer sample format";
        return 0;
        break;
    }



    /*now elaborate the format..*/
    if(chformat > STDWAVE && chformat != MC_BFMT){
        int chmask = sndgetchanmask(fd);

        /*we want to know the channel config, if any, and the brand of file     */
        if(chmask < 0)
            return 0;
        switch(chmask){
            /*check all cross-platform formats first...*/
        case(SPKRS_UNASSIGNED):
            props->chformat = MC_STD;
            break;
        case(SPKRS_MONO):
            props->chformat = MC_MONO;
            break;
        case(SPKRS_STEREO):
            props->chformat = MC_STEREO;
            break;
        case(SPKRS_GENERIC_QUAD):
            props->chformat = MC_QUAD;
            break;
        case(SPKRS_SURROUND_LCRS):
            props->chformat = MC_LCRS;
            break;
        case(SPKRS_DOLBY5_1):
            props->chformat = MC_DOLBY_5_1;
            break;
        case(SPKRS_SURR_6_1):
            props->chformat = MC_SURR_6_1;
            break;
        case(SPKRS_SURR_7_1):
            props->chformat = MC_SURR_7_1;
            break;
        case(SPKRS_CUBE):
            props->chformat = MC_CUBE;
            break;
        default:
            props->chformat = MC_WAVE_EX;   /*could be something weird from WAVE-EX...*/
            break;
        }
    }

    if(props->samptype==FLOAT32 || props->samptype== SAMP_LONG) {
        /*we know we have floats: is it spectral file?*/
        if(sndgetprop(fd,"original sampsize",(char *)&origsize, sizeof(int))<0){
            props_errstr = "Failure to read original sample size";
        }
        if(sndgetprop(fd,"original sample rate",(char *)&origrate,sizeof(int))<0){
            props_errstr = "Failure to read original sample rate";
        }
        if(sndgetprop(fd,"arate",(char *)&arate,sizeof(float)) < 0){
            props_errstr = "Failure to read analysis sample rate";
        }
        if(sndgetprop(fd,"analwinlen",(char *)&wlen,sizeof(int)) < 0){
            props_errstr = "Failure to read analysis window length";
        }
        if(sndgetprop(fd,"decfactor",(char *)&dfac,sizeof(int)) < 0){
            props_errstr = "Failure to read decimation factor";
        }
        /*TODO: find a way to guarantee unique checksums...*/
        checksum = origsize + origrate + wlen + dfac + (int)arate;
        if(checksum==0) {       /*its a wave file*/
            int isenv = 0;
            float winsize;
            if(sndgetprop(fd,"is an envelope",(char *) &isenv,sizeof(int)) < 0){
                props_errstr = "Error looking for envelope property";
            }
            if(isenv){
                /*its an envelope file, get window size*/

                if(sndgetprop(fd,"window size",(char *)&winsize,sizeof(float)) < 0) {
                    props_errstr = "Error reading window size in envelope file";
                    return 0;
                }
                props->window_size = /*(float)*/ winsize;

                props->type = wt_binenv;
                /*must be floatsams, in this case*/
                props->samptype = /*SAMP_FLOAT;*/ FLOAT32; /* RWD July 2004 */
                /*and it won't (yet) be anything other than a standard file*/
                props->chformat = STDWAVE;
            }
            /*tell props what brand of file this is - e.g WAVE_EX*/
            return (snd_fileformat(fd,&props->format) == 0);
        }
        else {
            if(props_errstr==NULL){ /*its a good spectral file*/
                /*must be floatsams*/
                props->samptype= FLOAT32;

                props->origsize = origsize;
                props->origrate = origrate;
                props->arate = arate;
                props->winlen = wlen;
                props->decfac = dfac;

                if(sndgetprop(fd,"is a pitch file", (char *)&dummy, sizeof(int))>=0)
                    props->type = wt_pitch;
                else if(sndgetprop(fd,"is a transpos file", (char *)&dummy, sizeof(int))>=0)
                    props->type = wt_transposition;
                else if(sndgetprop(fd,"is a formant file", (char *)&dummy, sizeof(int))>=0)
                    props->type = wt_formant;
                else
                    props->type = wt_analysis;
            } else
                return 0;       /*somehow, got a bad analysis file...*/
        }
        /* get any auxiliary stuff for control file */
        /* adapted from TW's function in speclibg.cpp */
        switch(props->type){
        case(wt_formant):
            if(sndgetprop(fd,"specenvcnt",(char *)&specenvcnt,sizeof(int)) < 0){
                props_errstr = "Failure to read formant size in formant file";
                return 0;
            }
            props->specenvcnt = specenvcnt;
            /*      break; */ /*RWD July 2004*/
        case(wt_pitch):
        case(wt_transposition):
            if(props->chans != 1){
                props_errstr =  "Channel count not equal to 1 in transposition file";
                return 0;
            }
            if(sndgetprop(fd,"orig channels", (char *)&origchans, sizeof(int)) < 0) {
                props_errstr =  "Failure to read original channel data from transposition file";
                return 0;
            }
            props->origchans = origchans;
            break;
        default:
            break;
        }
        props->chformat = STDWAVE;
    }
    /*tell props what brand of file this is - e.g WAVE_EX*/
    return (snd_fileformat(fd,&props->format) == 0);
}
