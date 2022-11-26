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



/**************************** INTERNAL VARIANT-FLAGS ******************************/

#define ACCU_DECAY              (0)     /* accu */
#define ACCU_GLIS               (1)     /* accu */

#define ARPE_IS_NONLIN  (0)     /* arpe */
#define ARPE_IS_SUST    (1)     /* arpe */
#define ARPE_TRKFRQ             (2)     /* arpe */
#define ARPE_KEEP_SUST  (3)     /* arpe */

#define BARE_LESS_BODY  (0)     /* bare */

#define CHORD_FTOP              (0)     /* chord */
#define CHORD_FBOT              (1)     /* chord */
#define CHORD_BODY              (2)     /* chord */

#define CROSS_INTP              (0)     /* cross */

#define DIFF_IS_CROSS   (0)     /* diff */
#define DIFF_SUBZERO    (1)     /* diff */

#define DRNK_NO_ZEROSTEPS       (0)     /* drunk */

#define FMNT_VIEW               (0)     /* fmntsee */

#define FOCUS_STABLE    (0)     /* focus */

#define FOLD_WITH_BODY  (0)     /* fold */

#define FSEE_DISPLAY    (0) /* formsee */

#define GLIS_FTOP               (0)     /* glis */

#define GREQ_NOTCH              (0)     /* greq */

#define MEAN_ZEROED             (0)     /* mean */

#define PF_STARTCUT             (0)     /* pitchfix */
#define PF_ENDCUT               (1)
#define PF_LOPASS               (2)
#define PF_HIPASS               (3)
#define PF_IS_SMOOTH    (4)
#define PF_IS_SMARK             (5)
#define PF_IS_EMARK             (6)
#define PF_TWOW                 (7)
#define PF_INTERP               (8)

#define PI_IS_MEAN              (0)     /* p_invert */
#define PI_IS_TOP               (1)
#define PI_IS_BOT               (2)

#define PQ_OCTDUPL              (0)     /* pquantise */

#define PR_IS_SLEW              (0)     /* prandomise */

#define PS_MEANP                (0)     /* psmooth */
#define PS_HOLD                 (1)     /* psmooth */

#define PEAK_HEAR               (0)     /* peak */

#define PICH_ALTERNATIVE_METHOD (0)     /* pitch */
#define KEEP_PITCH_ZEROS                (1) /* pitch, track */


#define SCAT_RANDCNT    (0)     /* scat */
#define SCAT_NO_NORM    (1)     /* scat */

#define SHIFT_LOG               (0)     /* shift */

#define SHP_IS_DEPTH    (0)     /* shiftp  */

#define STR_IS_DEPTH    (0)     /* stretch */

#define SUM_IS_CROSS    (0)     /*  sum */

#define TRACE_RETAIN    (0)     /*trace */

#define TRNSF_FTOP              (0)     /* trnsf */
#define TRNSF_FBOT              (1)     /* trnsf */
#define TRNSF_BODY              (2)     /* trsnf */

#define TRNSP_FTOP              (0)     /* trnsp */
#define TRNSP_FBOT              (1)     /* trnsp */
#define TRNSP_BODY              (2)     /* trnsp */

#define TUNE_TRACE      (0)     /* tune */
#define TUNE_FBOT               (1)     /* tune */

#define IS_OCTVU_FUND   (0)     /* octvu */

#define IS_BRG_START    (0)     /* bridge */
#define IS_BRG_END              (1)     /* bridge */

/* DISTORT_ENV */
#define DISTORTE_IS_TROF        (0)
#define DISTORTE_IS_EXPON       (1)

/* DISTORT_MLT */
#define DISTORTM_SMOOTH         (0)

/* DISTORT_DIV */
#define DISTORTD_INTERP         (0)

/* DISTORT_HRM */
/* DISTORT_FRC */
#define IS_PRESCALED            (0)

/* DISTORT_AVG  */
/* DISTORT_SHUF */
/* DISTORT_RPT  */
/* DISTORT_DEL  */
/* DISTORT_RPL  */
/* DISTORT_TEL  */
/* DISTORT_FLT  */
#define IS_DISTORT_SKIP         (0)

/* DISTORT_TEL */
#define IS_DISTTEL_AVG          (1)

/* ZIGZAG */
#define IS_ZIG_RSEED            (0)

/* LOOP */
#define IS_KEEP_START           (0)

/* SCRAMBLE */
#define IS_SCRAMBLE_RSEED       (0)
#define IS_SCR_KEEP_START       (1)
#define IS_SCR_KEEP_END         (2)

/* ITERATE */
#define IS_ITER_DELAY           (0)
#define IS_ITER_RAND            (1)
#define IS_ITER_PSCAT           (2)
#define IS_ITER_ASCAT           (3)
#define IS_ITER_FADE            (4)
#define IS_ITER_GAIN            (5)
#define IS_ITER_RSEED           (6)

/* DRUNKWALK */
#define IS_DRNK_CLOKRND         (0)
#define IS_DRNK_OVERLAP         (1)
#define IS_DRNK_RSEED           (2)

/* FILTERS */
#define FLT_DBLFILT                     (0)
#define DROP_OUT_AT_OVFLOW      (1)
/* FLTITER */
#define FLT_PINTERP_OFF         (1)
#define FLT_EXPDEC                      (2)
#define FLT_NONORM                      (3)

/* ALLPASS */
#define FLT_LINDELAY            (0)

/* GRAIN DUPL,REORODER,REPITCH,REMOTIF ONLY */
#define GR_ENVTRACK                     (0)
#define LOSE_LAST_GRAIN         (1)

/* MIX */
#define ALTERNATIVE_MIX         (0)

/* MIXSHUFL */
#define MSH_NOCHECK                     (0)

/* SYNCATT */
#define MSY_POWMETHOD           (0)

/* BRASSAGE */
#define GRS_NO_DECCEL           (0)
#define GRS_EXPON                       (1)
#define GRS_NO_INTERP           (2)

/* TEXTURE */
#define IS_TEX_RSEED            (0)
        /* FLAGS only used in certain texture processes */
#define TEX_IGNORE_DUR   (1)
#define DONT_KEEP_MTFDUR (2) /* motif note durs same as firstnote, not as motif-durs */
#define FIXED_STEP       (2) /* dec & gp note event-spacing equal */
#define INS_TO_SCATTER   (3) /* flags instrument values are scattered:     ORN & DEC only */
#define FORCEHI                  (4) /* Force orns, decs to highest note of chord */
#define WHICH_CHORDNOTE  (4) /* A duplicate name for FORCEHI flag : see NOTE BELOW */
#define FORCE_EVERY              (5) /* Force orns onto ALL notes of chord */
#define DISCARD_ORIGLINE (6) /* after decoration: discard orig line :           DEC only */

#define CYCLIC_TEXFLAG   (2)
#define PERM_TEXFLAG     (3)


#define TOTAL_POSSIBLE_USER_FLAGS (7) /* COUNT OF FLAGS ABOVE */

/* MOD_PITCH */
#define VTRANS_OUTTIMES         (0)
/* MOD_REVECHO */
#define DELAY_INVERT_FLAG       (0)
/* EDIT_INSERT */
#define INSERT_OVERWRITE        (0)
#define ACCEPT_SILENT_END       (1)
/* EDIT_SPLICE */
#define SPLICE_START            (0)
#define SPLICE_END                      (1)
/* RANDCHUNKS */
#define LINEAR                          (0)
#define FROMSTART                       (1)
/* HOUSE_COPY */
#define IGNORE_COPIES           (0)
/* HOUSE_DEL */
#define ALL_COPIES                      (0)
/* HOUSE_CHANS */
#define CHAN_INVERT_PHASE       (0)
/* HOUSE_SORT */
#define DONT_SHOW_DURS          (0)
/* HOUSE_EXTRACT */
#define STOP_ON_SAMENAME        (0)

#define NO_STT_TRIM                     (0)
#define NO_END_TRIM                     (1)
#define STT_TRIM                        (0)
#define END_TRIM                        (1)
/* SNDINFO */
/* samptotime & timetosamp */
#define CHAN_GROUPED            (0)
/* sfdiff */
#define IGNORE_LENDIFF          (0)
#define IGNORE_CHANDIFF         (1)
/* maxsamp */
#define FORCE_SCAN                      (0)

/* HOUSE RECOVER */
#define SFREC_PHASESHIFT        (0)

/* SYNTH */
#define SYNTH_FLOAT                     (0)

/* HF_PERM1 */

#define HP1_MINONLY                     (0)
#define HP1_SPAN                        (1)
#define HP1_DENS                        (2)
#define HP1_ELIMOCTS            (3)

/* SYNTH_SPEC */
#define SS_PICHSPRD                     (0)

/* TWIXT */
#define IS_NOTCYCL                      (0)

/* DISTORT_PULSE */
#define PULSE_KEEPSTART (0)     /* keep start of input sound before start of impulse stream */
#define PULSE_KEEPEND   (1)     /* keep tail of input sound on end of impulse stream */

/* EDIT : SUPPRESS NOISE */
#define GET_NOISE       (0)

/* STACK */
#define STACK_SEE (0)

/* SHUDDER */
#define SHUD_BALANCE (0)
#define SHUD_ATK         (1)

/* DOUBLETS */
#define NO_TIME_EXPAND  (0)

/* SYLLABS */
#define SYLLAB_PAIRS (0)
