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



                                                                /** MODE NAMES **/

/*********************************** SPEC ***********************************/

/* MODES for ALT */

#define DELETE_ODD  (0)
#define DELETE_EVEN (1)

/* MODES for ARPE */

#define ON                      (0)
#define BOOST           (1)
#define BELOW_BOOST (2)
#define ABOVE_BOOST (3)
#define BELOW           (4)
#define ABOVE           (5)
#define ONCE_BELOW      (6)
#define ONCE_ABOVE      (7)

/* MODES for BRIDGE */

#define BRG_NO_NORMALISE         (0)
#define BRG_NORM_TO_MIN      (1)
#define BRG_NORM_TO_FILE1    (2)
#define BRG_NORM_TO_FILE2    (3)
#define BRG_NORM_FROM_1_TO_2 (4)
#define BRG_NORM_FROM_2_TO_1 (5)


/* MODES for CHORUS */

#define CH_AMP            (0)
#define CH_FRQ            (1)
#define CH_FRQ_UP         (2)
#define CH_FRQ_DN         (3)
#define CH_AMP_FRQ        (4)
#define CH_AMP_FRQ_UP (5)
#define CH_AMP_FRQ_DN (6)

/* MODES for CLEAN */

#define FROMTIME          (0)
#define ANYWHERE          (1)
#define FILTERING         (2)
#define COMPARING         (3)

/* MODES for FILT */

#define F_HI              (0)
#define F_HI_NORM         (1)
#define F_LO              (2)
#define F_LO_NORM         (3)
#define F_HI_GAIN         (4)
#define F_LO_GAIN         (5)
#define F_BND             (6)
#define F_BND_NORM        (7)
#define F_NOTCH           (8)
#define F_NOTCH_NORM  (9)
#define F_BAND_GAIN       (10)
#define F_NOTCH_GAIN  (11)

/* MODES for FORM */

#define FORM_REPLACE  (0)
#define FORM_IMPOSE   (1)

/* MODES for FREEZE */

#define FRZ_AMP         (0)
#define FRZ_FRQ         (1)
#define FRZ_AMP_AND_FRQ (2)

/* MODES for GLIS */

#define SHEPARD           (0)
#define INHARMONIC    (1)
#define SELFGLIS      (2)

/* MODES for GREQ */

#define GR_ONEBAND        (0)
#define GR_MULTIBAND  (1)

/* MODES for INVERT */

#define INV_NORMAL        (0)
#define INV_KEEPAMP   (1)

/* MODES for MEAN */

#define MEAN_AMP_AND_PICH (0)
#define MEAN_AMP_AND_FRQ  (1)
#define AMP1_MEAN_PICH    (2)
#define AMP1_MEAN_FRQ     (3)
#define AMP2_MEAN_PICH    (4)
#define AMP2_MEAN_FRQ     (5)
#define MAXAMP_MEAN_PICH  (6)
#define MAXAMP_MEAN_FRQ   (7)

/* MODES for MORPH */

#define MPH_LINE          (0)
#define MPH_COSIN     (1)

/* MODES for OCT */

#define OCT_UP            (0)
#define OCT_DN            (1)
#define OCT_DN_BASS   (2)

/* MODES FOR P_APPROX,    P_INVERT, P_QUANTISE,
                         P_RANDOMISE, P_SMOOTH, P_VIBRATO */

#define PICH_OUT          (0)
#define TRANSP_OUT    (1)

/* MODES for P_CUT */

#define PCUT_START_ONLY (0)
#define PCUT_END_ONLY   (1)
#define PCUT_BOTH           (2)

/* MODES for P_EXAG */

#define RANGE_ONLY_TO_P   (0)
#define RANGE_ONLY_TO_T   (1)
#define CONTOUR_ONLY_TO_P (2)
#define CONTOUR_ONLY_TO_T (3)
#define R_AND_C_TO_P      (4)
#define R_AND_C_TO_T      (5)

/* MODES for P_SEE */

#define SEE_PITCH     (0)
#define SEE_TRANSPOS  (1)

/* MODES for P_TRANSPOSE */

#define PTR_RATIO         (0)
#define PTR_SEMITONES (1)

/* MODES for PICK */

#define PIK_HARMS                       (0)
#define PIK_OCTS                        (1)
#define PIK_ODD_HARMS           (2)
#define PIK_LINEAR                      (3)
#define PIK_DISPLACED_HARMS     (4)

/* MODES for PITCH */

#define PICH_TO_BIN       (0)
#define PICH_TO_BRK       (1)

/* MODES for REPORT */

#define FRQ_ORDERED_TIMED   (0)
#define AMP_ORDERED_TIMED   (1)
#define FRQ_ORDERED_UNTIMED (2)
#define AMP_ORDERED_UNTIMED (3)

/* MODES for REPITCH and REPITCHB */

#define PPT                       (0)
#define PTP                       (1)
#define TTT                       (2)
#define PPT_TO_BRK        (3)
#define PTP_TO_BRK        (4)
#define TTT_TO_BRK        (5)


/* MODES for SHIFT */

#define SHIFT_ALL         (0)
#define SHIFT_ABOVE       (1)
#define SHIFT_BELOW       (2)
#define SHIFT_BETWEEN (3)
#define SHIFT_OUTSIDE (4)

/* MODES for SHIFTP */

#define P_OCT_UP                 (0)
#define P_OCT_DN                 (1)
#define P_OCT_UP_AND_DN  (2)
#define P_SHFT_UP                (3)
#define P_SHFT_DN                (4)
#define P_SHFT_UP_AND_DN (5)

/* MODES for STRETCH */

#define STRETCH_ABOVE (0)
#define STRETCH_BELOW (1)

/* MODES for TRACE */

#define TRC_ALL           (0)
#define TRC_ABOVE         (1)
#define TRC_BELOW         (2)
#define TRC_BETWEEN       (3)

/* MODES for TRACK */

#define TRK_TO_BIN        (0)
#define TRK_TO_BRK        (1)

/* MODES for TRNSF and TRNSP */

#define TRNS_RATIO        (0)
#define TRNS_OCT          (1)
#define TRNS_SEMIT        (2)
#define TRNS_BIN          (3)

/* MODES for TUNE */

#define TUNE_FRQ          (0)
#define TUNE_MIDI         (1)

/* MODES for WAVER */

#define WAVER_STANDARD  (0)
#define WAVER_SPECIFIED (1)

/* MODES for TSTRETCH */

#define TSTR_NORMAL  (0)
#define TSTR_LENGTH  (1)

/******** MODE NUMBERS FOR INDIVIDUAL DISTORT PROGRAMS *********/

/* MODES FOR DISTORT */

#define DISTORT_SQUARE_FIXED            (0)
#define DISTORT_SQUARE                          (1)
#define DISTORT_TRIANGLE_FIXED          (2)
#define DISTORT_TRIANGLE                        (3)
#define DISTORT_INVERT_HALFCYCLE        (4)
#define DISTORT_CLICK                           (5)
#define DISTORT_SINE                            (6)
#define DISTORT_EXAGG                           (7)

/* MODES FOR DISTORT_ENV */
#define DISTORTE_RISING         (0)
#define DISTORTE_FALLING        (1)
#define DISTORTE_TROFFED        (2)
#define DISTORTE_USERDEF        (3)
                /* internal */
#define DISTORTE_LINRISE    (4)
#define DISTORTE_LINFALL    (5)
#define DISTORTE_LINTROF    (6)
#define DISTORTE_RISING_TR  (7)
#define DISTORTE_FALLING_TR (8)
#define DISTORTE_LINRISE_TR (9)
#define DISTORTE_LINFALL_TR (10)

/* MODES FOR DISTORT_DEL */

#define DELETE_IN_STRICT_ORDER  (0)     /* Original DIVIDE      */
#define KEEP_STRONGEST                  (1)     /* Original flag -1     */
#define DELETE_WEAKEST                  (2)     /* Original flag 0      */

/* MODES FOR DISTORT_FLT */

#define DISTFLT_HIPASS          (0)
#define DISTFLT_LOPASS          (1)
#define DISTFLT_BANDPASS        (2)

/* MODES FOR DISTORT_INT */

#define DISTINT_INTRLV          (0)      /* interleave: original type 1 */
#define DISTINT_RESIZE          (1)      /* resize:     original type 2 */

/* MODES FOR DISTORT_OVERLOAD */

#define OVER_NOISE                      (0)
#define OVER_SINE                       (1)

/* MODES FOR DISTORT_PULSE */

#define PULSE_IMP                       (0)
#define PULSE_SYN                       (1)
#define PULSE_SYNI                      (2)

/******** MODE NUMBERS FOR INDIVIDUAL ENVELOPE PROGRAMS *********/

/* IMPOSE,REPLACE */
#define ENV_SNDFILE_IN          (0)
#define ENV_ENVFILE_IN          (1)
#define ENV_BRKFILE_IN          (2)
#define ENV_DB_BRKFILE_IN       (3)
/* CREATE,EXTRACT */
#define ENV_ENVFILE_OUT (0)
#define ENV_BRKFILE_OUT (1)
/* WARPING,RESHAPING,REPLOTTING */
#define ENV_NORMALISE   (0)
#define ENV_REVERSE             (1) /* from warp */

#define ENV_EXAGGERATING (2)
#define ENV_ATTENUATING (3) /* from warp */
#define ENV_LIFTING             (4) /* from warp */
#define ENV_TSTRETCHING (5)

#define ENV_FLATTENING  (6)

#define ENV_GATING              (7)


#define ENV_INVERTING   (8)
#define ENV_LIMITING    (9)

#define ENV_CORRUGATING (10)

#define ENV_EXPANDING   (11)

#define ENV_TRIGGERING  (12)
#define ENV_CEILING     (13)
#define ENV_DUCKED              (14)
#define ENV_PEAKCNT     (15)
/* DOVETAILING */
#define DOVE            (0)
#define DOVEDBL         (1)
/* CURTAILING */
#define ENV_START_AND_END       (0)
#define ENV_START_AND_DUR       (1)
#define ENV_START_ONLY      (2)
#define ENV_START_AND_END_D     (3)
#define ENV_START_AND_DUR_D     (4)
#define ENV_START_ONLY_D    (5)
/* ATTACK */
#define ENV_ATK_GATED       (0)
#define ENV_ATK_TIMED       (1)
#define ENV_ATK_XTIME       (2)
#define ENV_ATK_ATMAX       (3)
/* TREMOL */
#define ENV_TREM_LIN        (0)
#define ENV_TREM_LOG        (1)

/******** MODE NUMBERS FOR INDIVIDUAL EXTEND PROGRAMS *********/

/* MODES FOR ZIGZAG */
#define ZIGZAG_SELF             (0)
#define ZIGZAG_USER             (1)

/* MODES FOR LOOP */
#define LOOP_ALL                (0)
#define LOOP_OUTLEN             (1)
#define LOOP_RPTS               (2)

/* MODES FOR SCRAMBLE */
#define SCRAMBLE_RAND   (0)
#define SCRAMBLE_SHRED  (1)

/* MODES FOR ITERATE */
#define ITERATE_DUR             (0)
#define ITERATE_REPEATS (1)

/* MODES FOR DRUNKWALK */
#define TOTALLY_PISSED    (0)
#define HAS_SOBER_MOMENTS (1)

/******************* MODES ********************/

/* fltbanku,fltbankv,fltiter, lphp */
#define FLT_HZ                  (0)
#define FLT_MIDI                (1)
/* fltbankn, fltbankc */
#define FLT_HARMONIC    (0)
#define FLT_ALTERNATE   (1)
#define FLT_SUBHARM         (2)
#define FLT_LINOFFSET   (3)
#define FLT_EQUALSPAN   (4)
#define FLT_EQUALINT    (5)
/* fltsweep, fstatvar */
#define FSW_HIGH        (0)
#define FSW_LOW         (1)
#define FSW_BAND        (2)
#define FSW_NOTCH       (3)
/* eq */
#define FLT_LOSHELF             (0)
#define FLT_HISHELF             (1)
#define FLT_PEAKING             (2)
/* allpass */
#define FLT_PHASESHIFT  (0)
#define FLT_PHASER              (1)

/******** MODE NUMBERS FOR INDIVIDUAL GRAIN PROGRAMS *********/

/* repitch */
/* rerhythm */
/* remotif */
#define GR_NO_REPEATS   (0)
#define GR_REPEATS              (1)

/******** MODE NUMBERS FOR INDIVIDUAL MIX PROGRAMS *************/

/* MIXINBETWEEN */
#define INBI_COUNT                      (0)
#define INBI_RATIO                      (1)

/* MIXSYNC */
#define MIX_SYNCMID                     (0)
#define MIX_SYNCEND                     (1)
#define MIX_SYNCSTT                     (2)

/* MIXMAX */
#define MIX_LEVEL_ONLY          (0)
#define MIX_CLIPS_ONLY          (1)
#define MIX_LEVEL_AND_CLIPS     (2)

/* MIXTWARP */
#define MTW_TIMESORT            (0)
#define MTW_REVERSE_T           (1)
#define MTW_REVERSE_NT          (2)
#define MTW_FREEZE_T            (3)
#define MTW_FREEZE_NT           (4)
#define MTW_SCATTER                     (5)
#define MTW_DOMINO                      (6)
#define MTW_ADD_TO_TG           (7)
#define MTW_CREATE_TG_1         (8)
#define MTW_CREATE_TG_2         (9)
#define MTW_CREATE_TG_3         (10)
#define MTW_CREATE_TG_4         (11)
#define MTW_ENLARGE_TG_1        (12)
#define MTW_ENLARGE_TG_2        (13)
#define MTW_ENLARGE_TG_3        (14)
#define MTW_ENLARGE_TG_4        (15)

/* MIXSHUFL */
#define MSH_DUPLICATE           (0)
#define MSH_REVERSE_N           (1)
#define MSH_SCATTER                     (2)
#define MSH_FIXED_N                     (3)
#define MSH_OMIT                        (4)
#define MSH_OMIT_ALT            (5)
#define MSH_DUPL_AND_RENAME     (6)

/* MIXSWARP */
#define MSW_FIXED                       (0)
#define MSW_NARROWED            (1)
#define MSW_LEFTWARDS           (2)
#define MSW_RIGHTWARDS          (3)
#define MSW_RANDOM                      (4)
#define MSW_RANDOM_ALT          (5)
#define MSW_TWISTALL            (6)
#define MSW_TWISTONE            (7)

/* MIXCROSS */
#define MCLIN                           (0)
#define MCCOS                           (1)

/* MIXDUMMY */
#define MD_TOGETHER                     (0)
#define MD_FOLLOW                       (1)


/******************* MODES ********************/

/* MOD_LOUDNESS */
#define LOUDNESS_GAIN           (0)
#define LOUDNESS_DBGAIN         (1)
#define LOUDNESS_NORM           (2)
#define LOUDNESS_SET            (3)
#define LOUDNESS_BALANCE        (4)
#define LOUDNESS_PHASE          (5)
#define LOUDNESS_LOUDEST        (6)
#define LOUDNESS_EQUALISE       (7)
/* TW March 2004 modes 8 & 9 are crypto modes for the tremolo program */
#define LOUD_PROPOR                     (10)
#define LOUD_DB_PROPOR          (11)

/* MOD_SPACE */
#define MOD_PAN                 (0)        /* name: PAN */
#define MOD_MIRROR              (1)        /* name: MIRROR */
#define MOD_MIRRORPAN   (2)        /* name: MIRROR */
#define MOD_NARROW              (3)        /* name: NARROW */
/* MOD_PITCH */
#define MOD_TRANSPOS            (0)        /* name: SPEED CHANGE */
#define MOD_TRANSPOS_SEMIT      (1)        /* name: SPEEDCHANGE IN SEMITONES */
#define MOD_TRANSPOS_INFO       (2)        /* name: VARISPEED INFO */
#define MOD_TRANSPOS_SEMIT_INFO (3)  /* name: VARISPEED SEMITONES INFO */
#define MOD_ACCEL                       (4)        /* name: ACCELERATE */
#define MOD_VIBRATO                     (5)        /* name: VIBRATO */
/* MOD_REVECHO */
#define MOD_DELAY               (0)        /* name: DELAY */
#define MOD_VDELAY              (1)        /* name: VDELAY */
#define MOD_STADIUM             (2)        /* name: STADIUM */
/* BRASSAGE */
#define GRS_PITCHSHIFT          (0)
#define GRS_TIMESTRETCH         (1)
#define GRS_REVERB                  (2)
#define GRS_SCRAMBLE            (3)
#define GRS_GRANULATE           (4)
#define GRS_BRASSAGE            (5)
#define GRS_FULL_MONTY          (6)
/* SAUSAGE */
#define S_RANDSPACE_GIVENP      (0)        /* name: RANDOM POSITION */
#define S_GIVENSPACE_GIVENP     (1)        /* name: GIVEN POSITIONS */
#define S_CYCLESPACE_GIVENP     (2)        /* name: CYCLING POSITIONS */
#define S_RANDSPACE_CYCLEP      (3)        /* name: RAND POS + CYCLING PITCH */
#define S_GIVENSPACE_CYCLEP     (4)        /* name: GIVN POS + CYCLING PITCH */
#define S_CYCLESPACE_CYCLEP     (5)        /* name: CYCL POS + CYCLING PITCH */
/* RADICAL */
#define MOD_REVERSE             (0)        /* name: REVERSE */
#define MOD_SHRED               (1)        /* name: SHRED */
#define MOD_SCRUB               (2)        /* name: SCRUB */
#define MOD_LOBIT               (3)        /* name: LOWER RESOLUTION */
#define MOD_RINGMOD             (4)        /* name: RING MODULATE */
#define MOD_CROSSMOD    (5)        /* name: CROSS MODULATE */
#define MOD_LOBIT2              (6)        /* name: LOWER RESOLUTION : NEW ALGO */

/******** MODE NUMBERS FOR INDIVIDUAL TEXTURE PROGRAMS *********/

#define TEX_HFIELD                      (0)
#define TEX_HFIELDS                     (1)
#define TEX_HSET                        (2)
#define TEX_HSETS                       (3)
#define TEX_NEUTRAL                     (4)

/******** MODE NUMBERS FOR PVOC_ANAL *********/

#define STANDARD_ANAL   (0)
#define ENVEL_ONLY              (1)
#define MAG_ONLY                (2)

/******** MODE NUMBERS FOR EDIT *********/

#define EDIT_SECS               (0)
#define EDIT_SAMPS              (1)
#define EDIT_STSAMPS    (2)

/******** MODE NUMBERS HOUSE_COPY *******/
#define COPYSF                  (0)
#define DUPL                    (1)

/******** MODE NUMBERS HOUSE_CHANNELS *******/
#define HOUSE_CHANNEL   (0)
#define HOUSE_CHANNELS  (1)
#define HOUSE_ZCHANNEL  (2)
#define STOM                    (3)
#define MTOS                    (4)

/******** MODE NUMBERS HOUSE_BUNDLE *******/
#define BUNDLE_ALL              (0)
#define BUNDLE_NONTEXT  (1)
#define BUNDLE_TYPE             (2)
#define BUNDLE_SRATE    (3)
#define BUNDLE_CHAN             (4)

/******** MODE NUMBERS HOUSE_SORT *******/

#define BY_FILETYPE             (0)
#define BY_SRATE                (1)
#define BY_DURATION             (2)
#define BY_LOG_DUR              (3)
#define IN_DUR_ORDER    (4)
#define FIND_ROGUES             (5)

/******** MODE NUMBERS HOUSE_SPEC *******/

#define HOUSE_RESAMPLE  (0)
#define HOUSE_CONVERT   (1)
#define HOUSE_REPROP    (2)

/******** MODE NUMBERS HOUSE_EXTRACT *******/

#define HOUSE_CUTGATE                   (0)
#define HOUSE_CUTGATE_PREVIEW   (1)
#define HOUSE_TOPNTAIL                  (2)
#define HOUSE_RECTIFY                   (3)
#define HOUSE_BYHAND                    (4)
#define HOUSE_ONSETS                    (5)

//TW REDUNDANT MODES REMOVED
/******** MODE NUMBERS INFO_MUSUNITS *******/

#define MU_MIDI_TO_FRQ                  (0)
#define MU_FRQ_TO_MIDI                  (1)
#define MU_NOTE_TO_FRQ                  (2)
#define MU_NOTE_TO_MIDI                 (3)
#define MU_FRQ_TO_NOTE                  (4)
#define MU_MIDI_TO_NOTE                 (5)
#define MU_FRQRATIO_TO_SEMIT    (6)
#define MU_FRQRATIO_TO_INTVL    (7)
#define MU_INTVL_TO_FRQRATIO    (8)
#define MU_SEMIT_TO_FRQRATIO    (9)
#define MU_OCTS_TO_FRQRATIO             (10)
#define MU_OCTS_TO_SEMIT                (11)
#define MU_FRQRATIO_TO_OCTS             (12)
#define MU_SEMIT_TO_OCTS                (13)
#define MU_SEMIT_TO_INTVL               (14)
#define MU_FRQRATIO_TO_TSTRETH  (15)
#define MU_SEMIT_TO_TSTRETCH    (16)
#define MU_OCTS_TO_TSTRETCH             (17)
#define MU_INTVL_TO_TSTRETCH    (18)
#define MU_TSTRETCH_TO_FRQRATIO (19)
#define MU_TSTRETCH_TO_SEMIT    (20)
#define MU_TSTRETCH_TO_OCTS             (21)
#define MU_TSTRETCH_TO_INTVL    (22)
#define MU_GAIN_TO_DB                   (23)
#define MU_DB_TO_GAIN                   (24)
#define MU_DELAY_TO_FRQ                 (25)
#define MU_DELAY_TO_MIDI                (26)
#define MU_FRQ_TO_DELAY                 (27)
#define MU_MIDI_TO_DELAY                (28)
#define MU_NOTE_TO_DELAY                (29)
#define MU_TEMPO_TO_DELAY               (30)
#define MU_DELAY_TO_TEMPO               (31)

/* SYNTHESIS */
#define WAVE_SINE               (0)
#define WAVE_SQUARE             (1)
#define WAVE_SAW                (2)
#define WAVE_RAMP               (3)

/* HF_PERM1 */

#define HFP_SNDOUT              (0)
#define HFP_SNDSOUT             (1)
#define HFP_TEXTOUT             (2)
#define HFP_MIDIOUT             (3)

/* TWIXT */

#define IN_SEQUENCE             (0)
#define RAND_REORDER    (1)
#define RAND_SEQUENCE   (2)
#define TRUE_EDIT               (3)

//NEW TW
/* P_INTERP */

#define PI_GLIDE                (0)
#define PI_SUSTAIN              (1)

/* CONVOLVED */
#define CONV_NORMAL             (0)
#define CONV_TVAR               (1)

/* CLICK */
#define CLICK_BY_TIME   (0)
#define CLICK_BY_LINE   (1)
/*TW March 2004 */
/* BATCH_EXPAND */
#define ONE_PARAM       (0)
#define MANY_PARAMS     (1)

/* ENVSYN */
#define ENVSYN_RISING   (0)
#define ENVSYN_FALLING  (1)
#define ENVSYN_TROFFED  (2)
#define ENVSYN_USERDEF  (3)

/* MODES FOR GREV */
#define GREV_REVERSE    (0)
#define GREV_REPEAT             (1)
#define GREV_DELETE             (2)
#define GREV_OMIT               (3)
#define GREV_TSTRETCH   (4)
#define GREV_GET                (5)
#define GREV_PUT                (6)
