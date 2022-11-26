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



/******************* SPEC ***************/

#define SPEC_MINFRQ                             (10.0)
#define PITCHZERO                               (SPEC_MINFRQ/2.0)

#define SIGNOIS_MAX                             (1000.0)

/* Constant vals for SPEC ARPE */

#define DOWNRAMP                (1)
#define SIN                     (2)
#define SAW                             (3)
#define UPRAMP                  (4)
#define ARPE_TABSIZE    (1024)

#define AP_NORMAL                                                                               (0)
#define AP_SUSTAIN_PITCH                                                                (1)
#define AP_LIMIT_SUSTAIN                                                                (2)
#define AP_SUSTAIN_PITCH_AND_LIMIT_SUSTAIN                              (3)
#define AP_NONLIN                                                                               (4)
#define AP_NONLIN_AND_SUSTAIN_PITCH                                             (5)
#define AP_NONLIN_AND_LIMIT_SUSTAIN                                             (6)
#define AP_NONLIN_AND_SUSTAIN_PITCH_AND_LIMIT_SUSTAIN   (7)

/* constants for SPEC CHORD */

#define CHD_NORMAL      (0)
/* change JAN 1998 ****
#define CHD_FULLER      (1)
*/
/* change JAN 1998 ****/
#define CHD_LESSFUL     (1)

/* Constant vals for SPEC CHORUS */

#define A_FIX        (1)        /* 0001 */
#define A_VAR        (2)        /* 0010 */
#define F_FIX        (4)        /* 0100 */
#define F_VAR        (8)        /* 1000 */
#define AF_FIX       (5)        /* 0101 */
#define AF_VAR      (10)        /* 1010 */
#define A_FIX_F_VAR  (9)        /* 1001 */
#define A_VAR_F_FIX  (6)        /* 0110 */
#define IS_A         (3)        /* 0011 */
#define IS_F        (12)        /* 1100 */

#define BOTH_ACCESSES_OF_RANDTABLE_REACH_END_TOGETHER (0)
#define FIRST_ACCESS_OF_RANDTABLE_REACHES_END_FIRST   (1)
#define SECOND_ACCESS_OF_RANDTABLE_REACHES_END_FIRST  (2)

/* constants for SPEC CROSS */

#define CROS_FULL   (0)
#define CROS_INTERP     (1)

/* constant values for SPEC FMNTSEE */

#define SPACER          (4)             /* gap left between formants in pseudosnd-display */

/* constant values for FOCUS and REPORT */

#define MAXPKCNT                (16)            /* max number of peaks for FOCU */
#define MAXSTABIL               (4097)          /* max value of stabl in FOCU */

/* constants for SPEC FREEZE */

#define FRZ_BEFORE      (-1)
#define FRZ_LIMIT       (0)
#define FRZ_AFTER       (1)

/* constants for SPEC GLIS */

#define GLIS_SHIFT_DEFAULT (50.0)
#define GLIS_REFERENCE_FRQ (dz->chwidth * 2.0)

/* Constant vals for SPEC MORPH */

#define MPH_COSTABSIZE (512)

/* constant values for SPEC P_APPROX */

#define BLOKCNT         (4)

/* constant values for P_FIX */

#define IS_HIPASS   (1)
#define IS_LOPASS   (2)
#define IS_BANDPASS (3)

/* constant values for P_INVERT and SPEC P_RANDOMISE */

#define MAXINTRANGE     (96.0)

/* constant values for SPEC P_VIBRATO */

#define P_TABSIZE       (256)

/* Constant values for SPEC PITCH and SPEC TRACK */

#define EIGHTH_TONE      (1.014545335)  /* tuning tolerance for data-reduction in pitch brkpnt files */
#define SILENCE_RATIO                   (80.0)  /* 80dB signal-to-noise ratio */
#define BLIPLEN                         (2)     /* Max no adj pitched windows needed to confirm a pitch */
#define MAXIMI                                  (8)             /* Number of loudest-spectral-peaks-per-window to test */
#define ACCEPTABLE_MATCH                (5)             /* Number of MAXIMI which must be harmonics */
#define PARTIALS_IN_TEST_TONE   (6)
#define PARTIAL_DECIMATION              (.25)
#define CHANSCAN                                (8)             /* Number of adjacent chans to search for a frq that */
                                                                                /* should be in current channel */
#define PEAKSCAN                                (2)
#define MAXHARM                                 (8)     /* Max harmonic than can be lowest (signif) partial */
                                                                        /* in a pitched sound */

/* constant values for SPEC REPITCH */

#define DEFAULT_FRAMETIME  (0.002F)             /* default for REPITCH with no binary files in */

/* Constant vals for SPEC SPLIT */

#define ASCTOINT                         (-48)
#define DO_AMPLITUDE_CHANGE        (3)  /* 0011 */
#define DO_RAMPED_AMPLITUDE    (2)      /* 0010 */
#define DO_TRANSPOSITION           (4)  /* 0100 */
#define DO_ADD_TO_SPECTRUM         (8)  /* 1000 */

/* constant values for SPEC WARP */

#define WARP_MININT     (1.0)   /* min interval for pitch to be marked as moving: semitones */
#define CHANSPAN        (4)             /* frq can lie +- 4 channels outside the expected channel for that frq */
#define BLOKCNT         (4)
#define WCHTOP          (4)
#define WCHBOT          (5)

/* constants usd in get_statechanges() */

#define IS_FRQ          (0)
#define IS_PITCH        (1)



#define DEFAULT_NYQUIST (10000) /* Fudge to allow brkpnt pitchdata to be tested */

#define DEFAULT_OCTBANDS                (10)    /* octvu */

/************** CONSTANTS FOR DEFINING RANGES *****************/

#define CHORU_MAXAMP                    (1028.0)
#define CHORU_MINAMP                    (1.0)
#define CHORU_MAXFRQ                    (4.0)
#define CHORU_MINFRQ                    (1.0)
#define OCT_MAX_HMOVE                   (4096.0)                        /* frq ratio for 12 octs  OCT   */
#define MINFWINDOW                              (0.2)                           /* minimum frq window for PEAK (semitones) */
#define MINTSTR                                 (0.0001)                        /* min timestretch val for TSTRETCH */
#define MAXTSTR                                 (10000.0)                       /* max timestretch val for TSTRETCH */
#define MAXPRANGE                               (96.0)                          /* max pitch-variation in WARP */
#define MAXSMOOTH                               (100.0)                         /* max number of smoothings in P_FIX : arbitrary!! */
#define MAXGLISRATE                             (0.0625)                        /* approx 1/16th octave per window */

#define DEFAULT_STABILITY               (9)                                     /* default value of stabl in FOCU */
#define DEFAULT_TWINDOW         (.1)
#define DEFAULT_FWINDOW         (6.0)
#define DEFAULT_SRANGE                  (0.1 * SECS_TO_MS)      /* time (secs) to scan pitch-contour. Rise by semitone  */
                                                                                                        /* over SRANGE  indicates rising pitch-contour                  */
#define DEFAULT_PRANGE                  (0.38)                          /* >third_tone: pitch mistuning allowed in approx_pitch */
#define DEFAULT_TRANGE                  (0.05 * SECS_TO_MS)
#define DEFAULT_NOISEGAIN               (2.0)
#define DEFAULT_ARPE_SUSTAIN    (3.0)
#define DEFAULT_ARPE_AMPLIF             (10.0)
#define DEFAULT_AVRG                    (13.0)
#define DEFAULT_CHORU_AMPSPREAD (2.0)
#define DEFAULT_CHORU_FRQSPREAD (1.1)
#define DEFAULT_MAX_DRUNK_STEP  (32.0)
#define DEFAULT_EXAG                    (2.0)
#define DEFAULT_FILT_FRQ                (CONCERT_A)
#define DEFAULT_Q                               (20.0)
#define DEFAULT_OCTAVE_BWIDTH   (0.333333)                      /* Third Octave Band */
#define DEFAULT_GLISRATE                (1.0)
#define DEFAULT_DURATION                (2.0)
#define DEFAULT_GRABTIME                (duration/2.0)
#define DEFAULT_WINDOW_STEP             (16.0)
#define DEFAULT_TIME_STEP               (DEFAULT_WINDOW_STEP * frametime * SECS_TO_MS)
#define DEFAULT_STEP                    (0.25)
#define DEFAULT_SLEW                    (1.0001)
#define DEFAULT_VIBRATO_FRQ             (8.0)
#define DEFAULT_VIBRATO_RANGE   (0.333333)
#define DEFAULT_TRACE                   (16.0)
#define DEFAULT_BLUR                    (16.0)
#define DEFAULT_OCT_TRANSPOS    (2.0)

#define CONCERT_A                               (440.0)
#define CONCERT_A_MIDI                  (69.0)

/******* SPEC TRNSF : TRNSP ******/

#define MIN_TRANSPOS                    (0.00383)       /* 8 octaves dn */
#define MAX_TRANSPOS                    (256.0)         /* 8 octaves up */
#define DEFAULT_TRANSPOS                (1.0)

#define ARPE_MAX_AMPL   (10000.0)
#define ARPE_MIN_NONLIN (0.02)
#define ARPE_MAX_NONLIN (50.0)
#define CL_MAX_GAIN             (40.0)
#define PH_MAX_GAIN             (40.0)
#define FILT_MAX_PG             (10000.0)
#define GAIN_MAX_GAIN   (10000.0)
#define FORM_MAX_GAIN   (40.0)
#define MPH_MIN_EXP             (0.02)
#define MPH_MAX_EXP             (50.0)
#define OCT_MAX_BREI    (50.0)
#define PEX_MAX_RANG    (128.0)
#define PR_MIN_SLEW             (0.02)
#define PR_MAX_SLEW             (50.0)
#define PSEE_MAX_SCF    (10000.0)
#define PW_MIN_DRED             (.00002)
#define PW_MAX_DRED             (12.0)
#define PLUK_MAX_GAIN   (1000.0)
#define STR_MIN_EXP             (0.02)
#define STR_MAX_EXP             (50.0)
#define VOCO_MAX_GAIN   (10.0)
#define WAVER_MIN_EXP   (0.02)
#define WAVER_MAX_EXP   (50.0)
#define ACCU_MIN_DECAY  (0.001)

#define SPEC_MAXOCTS    (8.0)

int is_harmonic(int *iratio,double frq1, double frq2);
