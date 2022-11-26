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
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <localcon.h>
#include <special.h>
#include <processno.h>
#include <modeno.h>
#include <stdio.h>
#include <sfsys.h>

static int  establish_special_data_type(int process,int mode, aplptr ap);
static int  setup_special_data(int process,int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap);
static int  setup_special_data_names(int process,int mode,aplptr ap);

/****************************** DEAL_WITH_SPECIAL_DATA *********************************/

int deal_with_special_data(int process,int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap)
{
    int exit_status;
    if((exit_status = establish_special_data_type(process,mode,ap))<0)
        return(exit_status);
    if(ap->special_data) {
        if((exit_status = setup_special_data(process,mode,srate,duration,nyquist,wlength,channels,ap))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/****************************** ESTABLISH_SPECIAL_DATA_TYPE *********************************/

int establish_special_data_type(int process,int mode,aplptr ap)
{
    ap->special_data = 0;
    switch(process) {

            /*************************** SPEC ****************************/
            /*************************** SPEC ****************************/
            /*************************** SPEC ****************************/

    case(CHORD):                    ap->special_data = SEMIT_TRANSPOS_SET;              break;
    case(FREEZE):                   ap->special_data = FREEZE_DATA;                     break;
    case(FREEZE2):                  ap->special_data = FREEZE2_DATA;                    break;
    case(GREQ):
        switch(mode) {
        case(GR_ONEBAND):   ap->special_data = FILTER_FRQS;         break;
        case(GR_MULTIBAND): ap->special_data = FILTER_BWS_AND_FRQS; break;
        default:
            sprintf(errstr,"Unknown mode in establish_special_data_type()\n");
            return(PROGRAM_ERROR);
        }
        break;                                                                  
    case(P_INVERT):                 ap->special_data = INTERVAL_MAPPING;                break;
    case(P_QUANTISE):               ap->special_data = PITCHQUANTISE_SET;               break;
//TW NEW CASES
    case(P_SYNTH):                  ap->special_data = PITCH_SPECTRUM;                  break;
    case(P_VOWELS):
    case(VFILT):                    ap->special_data = PITCH_VOWELS;                    break;
    case(P_GEN):                    ap->special_data = PITCH_CREATE;                    break;
    case(P_INSERT):                 ap->special_data = ZERO_INSERTTIMES;                break;
    case(P_SINSERT):                ap->special_data = ZERO_INSERTTIMES;                break;
    case(MIX_ON_GRID):              ap->special_data = GRIDDED_MIX;                     break;
    case(AUTOMIX):                  ap->special_data = AUTO_MIX;                        break;

    case(PITCH):
        if(!sloom) {
            switch(mode) {
            case(PICH_TO_BIN):  ap->special_data = OUT_PFILE;           break;
            case(PICH_TO_BRK):  ap->special_data = OUT_PBRKFILE;        break;
            default:
                sprintf(errstr,"Unknown mode in establish_special_data_type()\n");
                return(PROGRAM_ERROR);
            }
        }
        break;                                                                  
    case(SHUFFLE):                  ap->special_data = SHUFFLE_DATA;                    break;
    case(SPLIT):                    ap->special_data = SPECSPLI_DATA;                   break;
    case(TRACK):
        if(!sloom) {
            switch(mode) {
            case(TRK_TO_BIN):   ap->special_data = OUT_PFILE;           break;
            case(TRK_TO_BRK):   ap->special_data = OUT_PBRKFILE;        break;
            default:
                sprintf(errstr,"Unknown mode in establish_special_data_type()\n");
                return(PROGRAM_ERROR);
            }
        }
        break;
    case(TRNSF):
    case(TRNSP):
        switch(mode) {
        case(TRNS_RATIO):   ap->special_data = TRANSPOS_RATIO_OR_CONSTANT;      break;
        case(TRNS_OCT):     ap->special_data = TRANSPOS_OCTAVE_OR_CONSTANT;     break;
        case(TRNS_SEMIT):   ap->special_data = TRANSPOS_SEMIT_OR_CONSTANT;      break;
        case(TRNS_BIN):     break;
        default:
            sprintf(errstr,"Unknown mode in establish_special_data_type()\n");
            return(PROGRAM_ERROR);
        }
        break;
    case(TUNE):
        switch(mode) {
        case(TUNE_FRQ):     ap->special_data = FRQ_OR_FRQSET;                   break;
        case(TUNE_MIDI):    ap->special_data = PITCH_OR_PITCHSET;               break;
        default:
            sprintf(errstr,"Unknown mode in establish_special_data_type()\n");
            return(PROGRAM_ERROR);
        }
        break;  
    case(WEAVE):                    ap->special_data = WEAVE_DATA;                      break;
    case(MULTRANS):                 ap->special_data = SEMIT_TRANSPOS_SET;              break;


            /*************************** GROUCHO ****************************/
            /*************************** GROUCHO ****************************/
            /*************************** GROUCHO ****************************/

    case(DISTORT_ENV):  
        if(mode==DISTORTE_USERDEF)  ap->special_data = DISTORT_ENVELOPE;                break;
    case(ENVSYN):
        if(mode==ENVSYN_USERDEF)    ap->special_data = ENVSYN_ENVELOPE;                 break;
    case(DISTORT_HRM):              ap->special_data = HARMONIC_DISTORT;                break;
    case(DISTORT_SHUF):             ap->special_data = SHUFFLE_DATA;                    break;
//TW NEW CASE
    case(DISTORT_PULSED):           ap->special_data = PULSE_ENVELOPE;                  break;
    case(ZIGZAG):       
        if(mode==ZIGZAG_USER)       ap->special_data = ZIGDATA;                         break;

    case(SIMPLE_TEX):       
    case(GROUPS):           
    case(ORNATE): 
    case(PREORNATE):    
    case(POSTORNATE):       
    case(MOTIFS): 
    case(MOTIFSIN):         
    case(DECORATED): 
    case(PREDECOR): 
    case(POSTDECOR):        
    case(TIMED):            
    case(TGROUPS):          
    case(TMOTIFS): 
    case(TMOTIFSIN):                ap->special_data = TEX_NOTEDATA;                    break;

    case(GRAIN_REORDER):            ap->special_data = GRAIN_REORDER_STRING;            break;
    case(GRAIN_REPITCH):            ap->special_data = GRAIN_PITCH_RATIOS;              break;
    case(GRAIN_RERHYTHM):           ap->special_data = GRAIN_TIME_RATIOS;               break;
    case(GRAIN_REMOTIF):            ap->special_data = GRAIN_TWO_RATIOS;                break;
    case(GRAIN_POSITION):           ap->special_data = GRAIN_TIMINGS;                   break;
    case(GREV):
        if(mode==GREV_PUT)
            ap->special_data = GRAIN_TIMINGS;                   
        break;
    case(ENV_CREATE):
        if(mode==ENV_ENVFILE_OUT 
        || mode==ENV_BRKFILE_OUT)   ap->special_data = ENV_CREATEFILE;                  break;
    case(ENV_WARPING):
    case(ENV_REPLOTTING):
    case(ENV_RESHAPING): 
        if(mode==ENV_TRIGGERING)    ap->special_data = ENV_TRIGGER_RAMP;                break;
    case(MIXINBETWEEN):             
        if(mode==INBI_RATIO)
            ap->special_data = INBTWN_RATIOS;
        break;
    case(MIXSHUFL): 
        if(mode==MSH_DUPL_AND_RENAME) ap->special_data=SNDFILENAME;                     break;

    case(FLTBANKU):    
    case(FLTITER):                  ap->special_data = FILTERBANK;                      break;

    case(FLTBANKV):                 ap->special_data = TIMEVARYING_FILTERBANK;          break;
    case(FLTBANKV2):                ap->special_data = TIMEVARY2_FILTERBANK;            break;
    case(INSERTSIL_MANY):
    case(EDIT_EXCISEMANY):          ap->special_data = EXCISE_TIMES;                    break;
    case(INFO_MUSUNITS):
        if(mode==MU_NOTE_TO_FRQ
        || mode==MU_NOTE_TO_MIDI)   ap->special_data = NOTE_REPRESENTATION;
        else if(mode==MU_INTVL_TO_TSTRETCH
             || mode==MU_INTVL_TO_FRQRATIO) 
                                    ap->special_data = INTERVAL_REPRESENTATION;         break;
    case(HOUSE_DEL):                ap->special_data = SNDFILENAME;                     break;
    case(HOUSE_EXTRACT):
        if(mode==HOUSE_BYHAND)  
                                    ap->special_data = BY_HAND;                         break;
    case(ACC_STREAM):               ap->special_data = ATTACK_STREAM;                   break;
    case(DEL_PERM):                 ap->special_data = DELPERM;                         break;
    case(DEL_PERM2):                ap->special_data = DELPERM2;                        break;
    case(TWIXT):                    ap->special_data = SWITCH_TIMES;                    break;
    case(SPHINX):                   ap->special_data = MANY_SWITCH_TIMES;               break;
//TW NEW CASES
    case(MANY_ZCUTS):
    case(EDIT_CUTMANY):             ap->special_data = MANYCUTS;                        break;
    case(STACK):                    ap->special_data = STACKDATA;                       break;
    case(SEQUENCER):                ap->special_data = SEQUENCER_VALUES;                break;
    case(SEQUENCER2):               ap->special_data = SEQUENCER2_VALUES;               break;
    case(CLICK):                    ap->special_data = CLICKTRACK;                      break;
    case(SYLLABS):                  ap->special_data = SYLLTIMES;                       break;
    case(JOIN_SEQ):                 ap->special_data = JOINSEQ;                         break;
    case(JOIN_SEQDYN):              ap->special_data = JOINSEQDYN;                      break;
    case(BATCH_EXPAND):             ap->special_data = BATCH;                           break;
    case(MOD_LOUDNESS):
        if(mode == LOUD_PROPOR || mode == LOUD_DB_PROPOR)
                                    ap->special_data = LOUDNESS;                        break;
    case(MULTI_SYN):                ap->special_data = CHORD_SYN;                       break;
    }
    return(FINISHED);
}

/************************ SETUP_SPECIAL_DATA *********************/

int setup_special_data(int process,int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap)
{
    int exit_status;

    if((exit_status = setup_special_data_ranges(mode,srate,duration,nyquist,wlength,channels,ap))<0)
        return(exit_status);
    if((exit_status = setup_special_data_names(process,mode,ap))<0)
        return(exit_status);
    return(FINISHED);
}

/************************ SETUP_SPECIAL_DATA_RANGES *********************/

int setup_special_data_ranges(int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap)
{
    int exit_status;

    switch(ap->special_data) {

                /************************ SPEC *********************/
                /************************ SPEC *********************/
                /************************ SPEC *********************/

    case(TRANSPOS_RATIO_OR_CONSTANT):
        ap->special_range       = TRUE;
        ap->min_special         = MIN_TRANSPOS;
        ap->max_special         = MAX_TRANSPOS;
        ap->default_special     = DEFAULT_TRANSPOS;
        break;
    case(TRANSPOS_OCTAVE_OR_CONSTANT):
        ap->special_range       = TRUE;
        ap->min_special         = LOG2(MIN_TRANSPOS);
        ap->max_special         = LOG2(MAX_TRANSPOS);
        ap->default_special     = LOG2(DEFAULT_TRANSPOS);
        break;
    case(TRANSPOS_SEMIT_OR_CONSTANT):
        ap->special_range       = TRUE;
        ap->min_special         = 12.0 * LOG2(MIN_TRANSPOS);
        ap->max_special         = 12.0 * LOG2(MAX_TRANSPOS);
        ap->default_special     = 12.0 * LOG2(DEFAULT_TRANSPOS);
        break;
    case(SEMIT_TRANSPOS_SET):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 12.0 * LOG2(MIN_TRANSPOS);
        ap->max_special         = 12.0 * LOG2(MAX_TRANSPOS);
        ap->default_special     = 12.0 * LOG2(DEFAULT_TRANSPOS);
        break;
    case(FRQ_OR_FRQSET):
        ap->special_range       = TRUE;
        ap->min_special         = MINPITCH;         /* NEW */
        ap->max_special         = nyquist;
        ap->default_special     = CONCERT_A;
        break;
    case(PITCH_OR_PITCHSET):    
        ap->special_range       = TRUE;
        ap->min_special         = MIDIBOT;
        if((exit_status = hztomidi(&(ap->max_special),nyquist))<0)
            return(exit_status);
        ap->default_special     = CONCERT_A_MIDI;
        break;
    case(FILTER_FRQS):          /* actually 1 bwdth and many frqs */
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = OCTAVES_PER_SEMITONE;
        ap->max_special         = LOG2(nyquist - SPEC_MINFRQ);
        ap->other_special_range = TRUE;
        ap->min_special2        = SPEC_MINFRQ;
        ap->max_special2        = (nyquist * 2.0)/3.0;
        break;
    case(FILTER_BWS_AND_FRQS):  /* actually frq followed by bwd, in pairs */    
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = SPEC_MINFRQ;
        ap->max_special         = (nyquist * 2.0)/3.0;
        ap->other_special_range = TRUE;
        ap->min_special2        = OCTAVES_PER_SEMITONE;
        ap->max_special2        = LOG2(nyquist - SPEC_MINFRQ);
        break;
    case(PITCHQUANTISE_SET):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = MIDIMIN;
        ap->max_special         = MIDIMAX;
        break;
    case(INTERVAL_MAPPING):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = -MAXINTRANGE;
        ap->max_special         = MAXINTRANGE;
        break;
    case(SPECSPLI_DATA):
        ap->data_in_file_only   = TRUE;
        break;
    case(FREEZE_DATA):
    case(FREEZE2_DATA):
        ap->data_in_file_only   = TRUE;
        break;
    case(WEAVE_DATA):
        ap->data_in_file_only   = TRUE;
        ap->min_special         = (double)(-(wlength-2));
        ap->max_special         = (double)(wlength-1);
        break;
    case(OUT_PFILE):
    case(OUT_PBRKFILE):
        if(!sloom)
            ap->data_in_file_only   = TRUE;
        break;
//TW NEW CASES
    case(PITCH_SPECTRUM):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0.0;
        ap->max_special         = 1.0;
        break;
    case(PITCH_VOWELS):
        ap->data_in_file_only   = VOWEL_STRING;
        break;
    case(PITCH_CREATE):
        ap->data_in_file_only   = TRUE;
        break;
    case(ZERO_INSERTTIMES):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0.0;
        ap->max_special         = duration;
        if(mode==1)
            ap->max_special *= 48000.0;     /* Times maximum samplerate : kludgy */
        break;

                    /************************ SPEC & GROUCHO ***********************/

    case(SHUFFLE_DATA):
        ap->data_in_file_only   = SHUFFLE_STRING;
        break;

                    /************************ GROUCHO ***********************/
                    /************************ GROUCHO ***********************/
                    /************************ GROUCHO ***********************/

    case(DISTORT_ENVELOPE):
    case(ENVSYN_ENVELOPE):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0.0;
        ap->max_special         = 1.0;
        break;
//TW NEW CASE
    case(PULSE_ENVELOPE):
        if(mode == 0) {
            ap->data_in_file_only   = TRUE;
            ap->special_range       = TRUE;
            ap->min_special         = 0.0;
            ap->max_special         = 1.0;
        } else {
            ap->data_in_file_only   = FALSE;
            ap->special_range       = TRUE;
            ap->min_special         = 0.0;
            ap->max_special         = 1.0;
            ap->default_special     = 0.0;
        }
        break;
    case(HARMONIC_DISTORT):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = MIN_HARMONIC;
        ap->max_special         = MAX_HARMONIC;
        ap->other_special_range = TRUE;
        ap->min_special2        = MIN_HARM_AMP;
        ap->max_special2        = MAX_HARM_AMP;
        break;
    case(ZIGDATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = ZIG_SPLICELEN * MS_TO_SECS * 3;
        ap->max_special         = duration;
        break;
    case(TEX_NOTEDATA):
        ap->data_in_file_only   = TRUE;
        break;
    case(GRAIN_REORDER_STRING):
        ap->data_in_file_only   = REORDER_STRING;
        break;
    case(GRAIN_PITCH_RATIOS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->max_special         = GR_MAX_TRANSPOS * SEMITONES_PER_OCTAVE;
        ap->min_special         = -ap->max_special;
        break;
    case(GRAIN_TIME_RATIOS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = GR_MIN_TSTRETCH;
        ap->max_special         = GR_MAX_TSTRETCH;
        break;
    case(GRAIN_TWO_RATIOS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->max_special         = GR_MAX_TRANSPOS * SEMITONES_PER_OCTAVE;
        ap->min_special         = -(ap->max_special);
        ap->other_special_range = TRUE;
        ap->min_special2        = GR_MIN_TSTRETCH;
        ap->max_special2        = GR_MAX_TSTRETCH;
        break;
//TW NEW CASE
    case(SEQUENCER_VALUES):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->max_special         = GR_MAX_TRANSPOS * SEMITONES_PER_OCTAVE;
        ap->min_special         = -(ap->max_special);
        ap->other_special_range = TRUE;
        ap->min_special2        = 0.0;
        ap->max_special2        = 7200.0;   /* arbitrary : 2 hours */
        break;
    case(SEQUENCER2_VALUES):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->max_special         = MIDIMAX;
        ap->min_special         = MIDIMIN;
        ap->other_special_range = TRUE;
        ap->min_special2        = 0.0;
        ap->max_special2        = 7200.0;   /* arbitrary : 2 hours */
        break;
    case(GRAIN_TIMINGS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = GRAIN_SPLICELEN * MS_TO_SECS;
        ap->max_special         = MAX_FIRSTGRAIN_TIME;
        break;
    case(ENV_CREATEFILE):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0.0;
        ap->max_special         = BIG_TIME;
        ap->other_special_range = TRUE;
        ap->min_special2        = 0.0;
        ap->max_special2        = 1.0;
        break;
    case(ENV_TRIGGER_RAMP):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0.0;
        ap->max_special         = duration;
        ap->other_special_range = TRUE;
        ap->min_special2        = 0.0;
        ap->max_special2        = 1.0;
        break;
    case(SNDFILENAME):
        ap->data_in_file_only   = SNDFILENAME_STRING;
        break;
    case(FILTERBANK):
    case(TIMEVARYING_FILTERBANK):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        switch(mode) {
        case(FLT_HZ):
            ap->min_special     = FLT_MINFRQ;
            ap->max_special     = FLT_MAXFRQ;
            break;
        case(FLT_MIDI):
            ap->min_special     = unchecked_hztomidi(FLT_MINFRQ);
            ap->max_special     = MIDIMAX;
            break;
        }
        ap->other_special_range = TRUE;
        ap->min_special2        = FLT_MINGAIN;
        ap->max_special2        = FLT_MAXGAIN;
        break;
    case(TIMEVARY2_FILTERBANK):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(EXCISE_TIMES):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        switch(mode) {
        case(EDIT_SAMPS):
            ap->min_special     = 0.0;
            ap->max_special     = (int)((duration * (double)srate) + 1.0) * MAX_SNDFILE_OUTCHANS;
            break;                      /* ROUND UP, times OVERKILL as this func doesn't know chancnt!! */
        case(EDIT_STSAMPS):
            ap->min_special     = 0.0;
            ap->max_special     = (int)((duration * (double)srate) + 1.0);  /* ROUND UP */
            break;
        case(EDIT_SECS):
            ap->min_special     = 0.0;
            ap->max_special     = duration;
            break;
        }
        break;
    case(NOTE_REPRESENTATION):
        ap->data_in_file_only   = NOTE_STRING;
        break;
    case(INTERVAL_REPRESENTATION):
        ap->data_in_file_only   = INTERVAL_STRING;
        break;
    case(ATTACK_STREAM):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0.0;
        ap->max_special         = MAX_SYN_DUR;  /* 2 hours */
        break;
    case(BY_HAND):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0.0;
        ap->max_special         = (int)round(duration * (double)(srate * channels));
        ap->other_special_range = TRUE;
        ap->min_special2        = MINSAMP;
        ap->max_special2        = MAXSAMP;
        break;
    case(DELPERM):
    case(DELPERM2):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = -MIDIMAX/2;
        ap->max_special         = MIDIMAX/2;
        ap->other_special_range = TRUE;
        ap->min_special2        = 0.05;
        ap->max_special2        = .95;
        break;
    case(SWITCH_TIMES):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0.0;
        ap->max_special         = duration;
        break;
    case(MANY_SWITCH_TIMES):
//TW NEW CASE
    case(MANYCUTS):
        ap->data_in_file_only   = TRUE;
        break;
//TW NEW CASES
    case(GRIDDED_MIX):
    case(AUTO_MIX):
        ap->data_in_file_only   = TRUE;
        break;
    case(STACKDATA):
        ap->special_range       = TRUE;
        ap->min_special         = -60.0;
        ap->max_special         = 60.0;
        ap->default_special     = 12.0;
        break;
    case(CLICKTRACK):
    case(SYLLTIMES):
    case(JOINSEQ):
    case(JOINSEQDYN):
    case(BATCH):
    case(INBTWN_RATIOS):
    case(LOUDNESS):
        ap->data_in_file_only   = TRUE;
        break;
    case(CHORD_SYN):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        switch(mode) {
        case(0):
            hztomidi(&ap->min_special,1.0/MAX_SYN_DUR);
            ap->max_special         = 127;
            break;
        case(1):
            ap->min_special     = 1.0/MAX_SYN_DUR;
            ap->max_special     = miditohz(127.0);
            break;
        break;
        }
        break;
    default:
        sprintf(errstr,"Unknown special_data type: setup_special_data_ranges()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/************************ SETUP_SPECIAL_NAMES *********************/

int setup_special_data_names(int process,int mode,aplptr ap)
{
    switch(ap->special_data) {

                /************************ SPEC *********************/
    case(TRANSPOS_RATIO_OR_CONSTANT):   ap->special_data_name   = "TRANSPOSITION_RATIO";            break;
    case(TRANSPOS_OCTAVE_OR_CONSTANT):  ap->special_data_name   = "TRANSPOSITION_IN_OCTAVES";       break;
    case(TRANSPOS_SEMIT_OR_CONSTANT):   ap->special_data_name   = "TRANSPOSITION_IN_SEMITONES";     break;
    case(SEMIT_TRANSPOS_SET):           ap->special_data_name   = "TRANSPOSITION_IN_SEMITONES";     break;
    case(FRQ_OR_FRQSET):                ap->special_data_name   = "FREQUENCY_SET";                  break;
    case(PITCH_OR_PITCHSET):            ap->special_data_name   = "PITCH_SET";                      break;
    case(FILTER_FRQS):                  ap->special_data_name   = "BANDWIDTH_&_FILTER_FRQS";        break;
    case(FILTER_BWS_AND_FRQS):          ap->special_data_name   = "FILTER_FRQS_&_BANDWIDTHS";       break;
    case(PITCHQUANTISE_SET):            ap->special_data_name   = "QUANTISING_PITCHES";             break;
    case(INTERVAL_MAPPING):             ap->special_data_name   = "INTERVALS_FOR_MAPPING";          break;
    case(SPECSPLI_DATA):                ap->special_data_name   = "SPLITTING_DATA";                 break;
    case(FREEZE_DATA):                  ap->special_data_name   = "FREEZE_DATA";                    break;
    case(FREEZE2_DATA):                 ap->special_data_name   = "HOLD_DATA";                      break;
    case(WEAVE_DATA):                   ap->special_data_name   = "WEAVE_DATA";                     break;
//TW NEW CASES
    case(PITCH_SPECTRUM):               ap->special_data_name   = "PARTIAL_LEVELS";                 break;
    case(PITCH_VOWELS):                 ap->special_data_name   = "VOWELS";                         break;
    case(PITCH_CREATE):                 ap->special_data_name   = "PITCH_DATA";                     break;
    case(ZERO_INSERTTIMES):             ap->special_data_name   = "INSERTION_TIMEPAIRS";            break;

        /************************ SPEC & GROUCHO ***********************/
    case(SHUFFLE_DATA):                 ap->special_data_name   = "SHUFFLE_DATA";                   break;
                /************************ GROUCHO ***********************/
    case(DISTORT_ENVELOPE):             ap->special_data_name   = "USER_DISTORT_ENVELOPE";          break;
    case(ENVSYN_ENVELOPE):              ap->special_data_name   = "USER_ENVELOPE";                  break;
//TW NEW CASE
    case(PULSE_ENVELOPE):               ap->special_data_name   = "IMPULSE_ENVELOPE";               break;
    case(HARMONIC_DISTORT):             ap->special_data_name   = "HARMONIC-NO:AMPLITUDE_PAIRS";    break;
    case(ZIGDATA):                      ap->special_data_name   = "ZIGZAG_TIMES";                   break;
    case(TEX_NOTEDATA):                 ap->special_data_name   = "NOTE_DATA";                      break;
    case(GRAIN_REORDER_STRING):         ap->special_data_name   = "CODE_FOR_REORDERING";            break;
    case(GRAIN_PITCH_RATIOS):           ap->special_data_name   = "SEMITONE_SHIFT_EACH_GRAIN";      break;
    case(GRAIN_TIME_RATIOS):            ap->special_data_name   = "DURATION_MULTIPLIER_EACH_GRAIN"; break;
    case(GRAIN_TWO_RATIOS):             ap->special_data_name   = "PSHIFT&DUR-MULTIPLY_EACH_GRAIN"; break;
//TW NEW CASE
    case(SEQUENCER_VALUES):             ap->special_data_name   = "TIME,_SEMITONE_SHIFT_&_LEVEL";   break;
    case(SEQUENCER2_VALUES):            ap->special_data_name   = "SOUND-NO,_TIME,_MIDIPITCH,_LEVEL,_DUR";   break;
    case(GRAIN_TIMINGS):                ap->special_data_name   = "GRAIN_TIMINGS";                  break;
    case(ENV_CREATEFILE):               ap->special_data_name   = "TIMES_&_(e)LEVELS_FOR_ENVELOPE"; break;
    case(ENV_TRIGGER_RAMP):             ap->special_data_name   = "TIMES_&_LEVELS_OF_RAMP";         break;
    case(SNDFILENAME):
        switch(process) {
        case(MIXSHUFL):                 ap->special_data_name   = "NEW_FILENAME";                   break;
        case(HOUSE_DEL):                ap->special_data_name   = "GENERIC_FILENAME";               break;
        }
        break;                  
    case(FILTERBANK):
        switch(mode) {
        case(FLT_HZ):                   ap->special_data_name   = "FRQ_AND_AMP_OF_FILTERS";         break;
        case(FLT_MIDI):                 ap->special_data_name   = "PITCH(MIDI)_&_AMP_OF_FILTERS";   break;
        }
        break;
    case(TIMEVARYING_FILTERBANK):
    case(TIMEVARY2_FILTERBANK):
        switch(mode) {
        case(FLT_HZ):                   ap->special_data_name   = "TIMELIST_OF_FRQS&AMPS_OF_FILTS"; break;
        case(FLT_MIDI):                 ap->special_data_name   = "TIMELIST_OF_PICHS&AMPS_OF_FILTS";break;
        }
        break;
    case(EXCISE_TIMES):                 ap->special_data_name   = "START_&_END_TIMES_FOR_EXCISIONS";break;
    case(NOTE_REPRESENTATION):          ap->special_data_name   = "NOTE_NAME";                      break;
    case(INTERVAL_REPRESENTATION):      ap->special_data_name   = "INTERVAL_NAME";                  break;
    case(ATTACK_STREAM):                ap->special_data_name   = "TIMES_OF_EVENTS";                break;
    case(BY_HAND):                      ap->special_data_name   = "SAMPLE_NO_AND_SAMPLE_VALUE";     break;
    case(DELPERM):                      ap->special_data_name   = "DELAY_PERMUTATION";              break;
    case(DELPERM2):                     ap->special_data_name   = "TRANSPOS_+_DELAY_PERMUTATION";   break;
    case(SWITCH_TIMES):                 ap->special_data_name   = "SWITCHING_TIMES";                break;
    case(MANY_SWITCH_TIMES):            ap->special_data_name   = "SWITCHING_TIMES";                break;
// TW NEW CASES
    case(MANYCUTS):                     ap->special_data_name   = "SEGMENT_START_AND_END_TIMES";    break;
    case(GRIDDED_MIX):                  ap->special_data_name   = "MIX_GRID";                       break;
    case(AUTO_MIX):                     ap->special_data_name   = "MIX_BALANCE_FUNCTION";           break;
    case(STACKDATA):                    ap->special_data_name   = "TRANSPOSITION_DATA";             break;
    case(CLICKTRACK):                   ap->special_data_name   = "CLICKTRACK_DATA";                break;
    case(SYLLTIMES):                    ap->special_data_name   = "SYLLABLE_TIMES";                 break;
    case(JOINSEQ):                      ap->special_data_name   = "FILE_SEQUENCE";                  break;
    case(JOINSEQDYN):                   ap->special_data_name   = "FILE_SEQUENCE_WITH_LOUDNESSES";  break;
    case(BATCH):                        ap->special_data_name   = "NEW_PARAMETER_VALUES";           break;
    case(INBTWN_RATIOS):                ap->special_data_name   = "MIX_RATIOS";                     break;
    case(LOUDNESS):                     ap->special_data_name   = "LOUDNESS_ENVELOPE";              break;
    case(CHORD_SYN):
        switch(mode) {
        case(0):                        ap->special_data_name   = "MIDI_PITCHES_OF_CHORD_NOTES";    break;
        case(1):                        ap->special_data_name   = "FRQUENCIES_OF_CHORD_NOTES";      break;
        }
        break;
    default:
        sprintf(errstr,"Unknown special_data type: setup_special_data_names()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

