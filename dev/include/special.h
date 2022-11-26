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



/******************* SPECIAL DATA TYPES : GROUCHO  ***************************************/

/* DISTORT */
#define NO_SPECIAL_TYPE                 (0)
#define DISTORT_ENVELOPE                (1)             /* FILE */
#define HARMONIC_DISTORT                (2)             /* FILE */
#define SHUFFLE_DATA                    (3)             /* STRING */
/* EXTEND */
#define ZIGDATA                                 (4)              /* FILE */
/* TEXTURE */
#define TEX_NOTEDATA                    (6)      /* FILE */
/* GRAIN */
#define GRAIN_REORDER_STRING    (7)              /* STRING */
#define GRAIN_PITCH_RATIOS              (8)              /* FILE */
#define GRAIN_TIME_RATIOS               (9)              /* FILE */
#define GRAIN_TWO_RATIOS                (10)     /* FILE */
#define GRAIN_TIMINGS                   (11)     /* FILE */
/* ENVEL */
#define ENV_CREATEFILE                  (12)     /* FILE */
#define ENV_TRIGGER_RAMP                (13)     /* FILE */
/* MIX */
#define SNDFILENAME                             (14)     /* STRING */
/* FILTERS */
#define FILTERBANK                              (15)     /* FILE */
#define TIMEVARYING_FILTERBANK  (16)     /* FILE */
/* EXTEND */
#define ATTACK_STREAM                   (41)     /* FILE */
/* HOUSEKEEP */
#define BY_HAND                                 (42)     /* FILE */
/* HF_PERM */
#define DELPERM                                 (43)     /* FILE */
#define DELPERM2                                (44)     /* FILE */
/* SFEDIT */
#define SWITCH_TIMES                    (45)     /* FILE */
#define MANY_SWITCH_TIMES               (46)     /* FILE */
/* REPITCH */
#define PITCH_SPECTRUM                  (47)     /* FILE */
#define ZERO_INSERTTIMES                (48)     /* FILE */
#define PITCH_VOWELS                    (49)     /* FILE or STRING */
/* SUBMIX */
#define GRIDDED_MIX                             (50)     /* FILE */
/* REPITCH */
#define PITCH_CREATE                    (51)     /* FILE */
/* SUBMIX */
#define AUTO_MIX                                (52)     /* FILE */
/* SFEDTSUBMIX */
#define MANYCUTS                                (53)     /* FILE */
/* STACK */
#define STACKDATA                               (54)     /* FILE or VAL */
/* DISTORT PULSE */
#define PULSE_ENVELOPE                  (55)     /* FILE */
/* SEQUENCER */
#define SEQUENCER_VALUES                (56)     /* FILE */
/* CLICK */
#define CLICKTRACK                              (57)     /* FILE */
/* SYLLABS */
#define SYLLTIMES                               (58)     /* FILE */
/* JOIN_SEQ */
#define JOINSEQ                                 (59)     /* FILE */
/* BATCH_EXPAND */
#define BATCH                                   (60)     /* FILE */
/* BATCH_EXPAND */
#define INBTWN_RATIOS                   (61)     /* FILE */
/* PROPORTIONAL LOUDNESS */
#define LOUDNESS                                (62)     /* FILE */
/* JOIN_SEQDYN */
#define JOINSEQDYN                              (63)     /* FILE */
/* ENVSYN */
#define ENVSYN_ENVELOPE                 (64)     /* FILE */
/* SEQUENCER2 */
#define SEQUENCER2_VALUES               (65)     /* FILE */
/* MULTI_SYN */
#define CHORD_SYN                               (66)     /* FILE */
/* FLTBANKV2 */
#define TIMEVARY2_FILTERBANK    (67)     /* FILE */

/******************* SPECIAL DATA TYPES : SPEC ***************************************/


#define TRANSPOS_RATIO_OR_CONSTANT      (17) /* FILE OR VAL */
#define TRANSPOS_OCTAVE_OR_CONSTANT     (18) /* FILE OR VAL */
#define TRANSPOS_SEMIT_OR_CONSTANT      (19) /* FILE OR VAL */
#define SPECSPLI_DATA                           (20) /* FILE */
                                                                                                                        /*      Data for the spec split prog specifying filter
                                                                                                                        bands and what happens to them
                                                                                                                        Original data format crazy, better as grafix-in */
#define FRQ_OR_FRQSET                           (21) /* FILE OR VAL */
                                                                                                                        /* frqs for Spec tune in mode that takes frq data */
#define PITCH_OR_PITCHSET                       (22) /* FILE OR VAL */  /* midipitches for Spec tune in mode that takes midi */
#define FILTER_FRQS                                     (23) /* FILE */                 /* Filter frqs for spec greq in single bandwidth mode */
#define FILTER_BWS_AND_FRQS                     (24) /* FILE */                 /* Filter frqs and bws for spec greq in variable bwidth mode */
#define SEMIT_TRANSPOS_SET                      (25) /* FILE */                 /* semitone transposition vals for spec chord */
#define FREEZE_DATA                                     (26) /* FILE */                 /* Freeze points (with flags) for spec freeze - better grafik */
#define SPEC_SHUFFLE_DATA                       (27) /* STRING */               /* Strings demo-ing shuffling for spec shuffle */
#define WEAVE_DATA                                      (28) /* FILE */                 /* Number seq defining weave for spec weave - better grafix */
#define INTERVAL_MAPPING                        (29) /* FILE OR VAL */  /* Map interval-->interval for spec pinvert */
#define PITCHQUANTISE_SET                       (30) /* FILE */                 /* Midi pitchvals for quantising pitch on, for spec pquantise */
#define OUT_PFILE                                       (31) /* STRING */               /* Causes process to OPEN 2nd outfile - pitchdata file */
#define OUT_PBRKFILE                            (32) /* STRING */               /* Causes process to OPEN 2nd outfile - pitch brkpnt(text)file */


/* especially for handling infile in SPEC REPITCH */

#define PITCH_BINARY_OR_BRKPNT1         (33)
#define PITCH_BINARY_OR_BRKPNT2         (34)
#define TRANSPOS_BINARY_OR_BRKPNT1      (35)
#define TRANSPOS_BINARY_OR_BRKPNT2      (36)

/* EDIT */
#define EXCISE_TIMES                            (37)     /* FILE */
/* INFO */
#define NOTE_REPRESENTATION                     (38)     /* STRING */
#define INTERVAL_REPRESENTATION         (39)     /* STRING */

#define FREEZE2_DATA                            (40) /* FILE */         /* Hold points & durs for spec freeze2 */

/********** SPECIAL DATA VALS ************/

#define MIN_HARMONIC    (2.0)
#define MAX_HARMONIC    (1024.0)
#define MIN_HARM_AMP    (1.0/(double)MAXSHORT)
#define MAX_HARM_AMP    (32.0)

/*RWD 6:2001 purged decls of local statics */

int  read_shuffle_data(int dmnparam,int imgparam,int mapparam,char *str,dataptr dz);
int  read_new_filename(char *str,dataptr dz);
int  read_env_ramp_brk(char *str,dataptr dz);
