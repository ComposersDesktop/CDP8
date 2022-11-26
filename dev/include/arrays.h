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



        /*******@@@@@@@@@@@@@@****** ARRAY NAMES ********@@@@@@@@@@@@@@********/

/************************* (double) VARIABLE ARRAY NAMES **********************/

/*************** SPEC ***********/

#define ARPE_TAB                        (0)

#define CHORU_RTABA                     (0)
#define CHORU_RTABF                     (1)

#define GLIDE_INF                       (0)

#define INV_AMPRATIO            (0)

#define MPH_COS                         (0)     /* morph */

#define OCTVU_ENERGY            (0)

#define PA_AVPICH                       (0)     /* p_approx */

#define PI_INTMAP                       (0)     /* p_invert */

#define PQ_QSET                         (0)     /* p_quantise */

#define PV_SIN                          (0)     /* p_vibrato */

#define PW_BRK                          (0)     /* p_write */

#define PEAK_BAND                       (0)     /* peak */
#define PEAK_MAXI                       (1)

#define PICH_PRETOTAMP          (0)     /* pitch && track */
#define PICH_PBRK                       (1)

#define PICH_SPEC                       (0)

#define RP_TBRK                         (0)     /* repitch */

#define SHIFT_CHTOP             (0)
#define SHIFT_CHMID             (1)

#define TSTR_PBUF                       (0)
#define TSTR_QBUF                       (1)
/* the 2 above are also names for bufptrs  */
/* the 1 below is ONLY a name for a bufptr */
#define TSTR_PEND                       (2)

#define WARP_P2                         (0)
#define WARP_AVP                        (1)

#define WAVER_CHFBOT            (0)

/* DISTORT */
#define DISTORT_SIN                     (0)

/* DISTORT_ENV */
#define DISTORTE_ENV            (0)     /* array AND ptr */
#define DISTORTE_ENVEND         (1)     /* ptr only */

/* DISTORT_HRM */
#define DISTORTH_AMP            (0)

#define ENV_CREATE_INTIME       (0)
#define ENV_CREATE_INLEVEL      (1)
#define ENV_CREATE_TIME         (2)
#define ENV_CREATE_LEVL         (3)

/* ENVEL */
#define ENV_SINETAB                     (0)

/* ZIGZAG : LOOP */
#define ZIGZAG_SPLICE           (0)

/* DRUNKWALK */
#define DRNK_SPLICETAB          (0)

/* FILTERS */
#define FLT_FRQ                         (0)
#define FLT_AMP                         (1)
#define FLT_AMPL                        (2)
#define FLT_A                           (3)
#define FLT_B                           (4)
#define FLT_Y                           (5)
#define FLT_Z                           (6)
#define FLT_D                           (7)
#define FLT_E                           (8)
#define FLT_COSW                        (9)
#define FLT_SINW                        (10)
#define FLT_WW                          (11)

#define FLT_FBRK                        (12)
#define FLT_INAMP                       (13)
#define FLT_INFRQ                       (14)
#define FLT_LASTFVAL            (15)
#define FLT_LASTAVAL            (16)
#define FLT_FINCR                       (17)
#define FLT_AINCR                       (18)

#define FLT_HBRK                        (19)
#define HARM_FRQ_CALC           (20)
#define HARM_AMP_CALC           (21)

/* FSTATVAR, FLTSWEEP */
#define FLT_DLS                         (0)
#define FLT_DBS                         (1)
#define FLT_DHS                         (2)
#define FLT_DNS                         (3)

/* EQ */
#define FLT_XX1                         (0)
#define FLT_XX2                         (1)
#define FLT_YY1                         (2)
#define FLT_YY2                         (3)

/* ALLPASS */
#define FLT_DELBUF1                     (0)
#define FLT_DELBUF2                     (1)

/* LPHP */
/*RWD 4:2000 redefined to deal with nchan files */
#define FLT_DEN1                        (0)
#define FLT_DEN2                        (1)
#define FLT_CN                          (2)

#ifdef NOTDEF
#define FLT_S1                          (3)
#define FLT_S2                          (4)
#define FLT_E1                          (5)
#define FLT_E2                          (6)
#define FLT_S1S                         (7)
#define FLT_E1S                         (8)
#define FLT_S2S                         (9)
#define FLT_E2S                         (10)
#else
#define FLT_S1_BASE                     (3)
#define FLT_S2_BASE                     (4)
#define FLT_E1_BASE                     (5)
#define FLT_E2_BASE                     (6)
#define FLT_LPHP_ARRAYS_PER_FILTER (4)
#endif
/* GRAIN */
#define GR_ENVEL                        (0)
#define GR_ENVSTEP                      (1)
#define GR_SPLICETAB            (2)

/* GRAIN REPITCH, RERHYTHM, REMOTIF */
#define GR_RATIO                        (3)

/* GRAIN REPOSITION, FIND & ALIGN */
#define GR_SYNCTIME                     (3)

/* SYNC */
#define MSY_TIMESTOR            (0)
#define MSY_STARTSRCH           (1)
#define MSY_ENDSRCH                     (2)

/* MIX TIMEWARP */
#define MTW_TIMESTOR            (0)
#define MTW_TIMEDIFF            (1)

/* MIX SPACEWARP */
#define MSW_TIMESTOR            (0)

/* MIX GAIN */
#define MIX_LLEVELSTOR          (0)
#define MIX_RLEVELSTOR          (1)

/* MIX MERGE */
#define MCR_COSTABLE            (0)

/* BRASSAGE & SAUSAGE */
#define GRS_BSPLICETAB          (0)
#define GRS_ESPLICETAB          (1)
#define GRS_NORMFACT            (2)

/* TEXTURE */
#define TEX_PITCHES                     (0)

/* MODIFY */
#define VIB_SINTAB                      (0)
#define DELAY_BUF                       (0)

#define STAD_TDELAY                     (0)
#define STAD_GAIN                       (1)
#define STAD_PAN                        (2)
#define STAD_GAINL                      (3)
#define STAD_GAINR                      (4)

#define SCRUB_SIN                       (0)
#define SCRUB_READTAB           (1)

#define RM_SINTAB                       (0)
                /* EDIT JOIN */
#define SPLICE_UP                       (0)
#define SPLICE_DN                       (1)

                /* SYNTH */
#define SYNTH_TAB                       (0)

                /* UTILS */
#define COL                                     (0)

/************************ INTEGER VARIABLE ARRAY NAMES **************************/

/*** SPEC *****/

#define ARPE_KEEP                       (0)

#define CL_MARK                         (0)  /* clean */

#define FRZ_TIMETYPE            (0)

#define GLIDE_ZERO                      (0)

#define OCTVU_CHBTOP            (0)
#define OCTVU_CHBBOT            (1)

#define PA_CHANGE                       (0)     /* p_approx */

#define SCAT_KEEP                       (0)

#define SHIFT_OVER              (0)
#define SHIFT_DONE              (1)

#define SHUF_MAP                        (0)

#define TUNE_LOUD                       (0)

#define WARP_CHANGE                     (0)

#define WEAVE_WEAV                      (0)

#define MEAN_LOC1                       (0)
#define MEAN_LOC2                       (1)

/* DISTORT_HRM */
#define DISTORTH_HNO            (0)

/* DISTORT_SHUF */
#define DISTORTS_MAP            (0)

/* ENVEL */
#define ENV_SLOPETYPE           (0)

/* LOOP : SCRAMBLE */
#define ZIGZAG_PLAY                     (0)

/* SCRAMBLE */
#define SCRAMBLE_PERM           (1)

/* GRAIN REORDER only */
#define GR_REOSET                       (0)

/* MIX SYNC */
#define MSY_CHANS                       (0)
#define MSY_ENVPOS                      (1)

/* BRASSAGE & SAUSAGE */
#define GRS_FLAGS                       (0)
/* SAUSAGE */
#define SAUS_PERM                       (1)

/* TEXTURE */
#define TXPERMINDEX             (0)
#define TXPERMLEN               (1)
#define TXLASTPERMLEN           (2)
#define TXLASTPERMVAL           (3)
#define TXREPETCNT                      (4)
#define TXRPT                           (5)

/* MOD_RADICAL */
#define SHR_PERM                        (0)

/************************** LONG VARIABLE ARRAY NAMES ****************************/

/**** SPEC *****/

#define FRZ_SEGTIME                     (0)
#define FRZ_FRZTIME                     (1)

#define PICK_BFLG                       (0)

#define PLUK_BFLG                       (0)

/* DISTORT_AVG */
#define DISTORTA_CYCLEN         (0)
#define DISTORTA_STARTCYC       (1)

/* DISTORT_FRC */
#define DISTORTF_CYCLEN         (0)
#define DISTORTF_STARTCYC       (1)

/* DISTORT_SHUF */
#define DISTORTS_STARTCYC       (0)

/* DISTORT_DEL */
#define DISTDEL_STARTCYC        (0)
#define DISTDEL_CYCLEVAL        (1)

/* DISTORT_RPL */
#define DISTRPL_STARTCYC        (0)
#define DISTRPL_CYCLEVAL        (1)

/* DISTORT_TEL */
#define DISTTEL_STARTCYC        (0)
#define DISTTEL_CYCLEVAL        (1)

/* ZIGZAG : LOOP : SCRAMBLE */
#define ZIGZAG_TIMES            (0)

/* SCRAMBLE */
#define SCRAMBLE_CHUNKPTR       (1)
#define SCRAMBLE_CHUNKLEN       (2)

/* FILTERS */
#define FLT_SAMPTIME            (0)

/* GRAIN REORDER */
#define GR_ARRAYLEN                     (0)
#define GR_THIS_LEN                     (1)

/* GRAIN REVERSE */
#define GR_ABS_POS                      (0)

/* MIX SYNC */
#define MSY_PEAKSAMP            (0)
#define MSY_SAMPSIZE            (1)
#define MSY_ENVEL                       (2)

/* BRASSAGE & SAUSAGE */
#define GRS_LBUF                        (0)
#define GRS_LBUFEND                     (1)
#define GRS_LBUFMID                     (2)
#define GRS_LTAILEND            (3)

/* MOD_REVECHO */
#define STAD_DELAY                      (0)

/* MOD_RADICAL */
#define SHR_CHUNKPTR            (0)
#define SHR_CHUNKLEN            (1)

/* EDIT EXCISE(MANY) */
#define CUT_STTSAMP                     (0)
#define CUT_STTSPLI                     (1)
#define CUT_ENDSAMP                     (2)
#define CUT_ENDSPLI                     (3)

/* HOUSE EXTRACT */
#define CUTGATE_STIME           (0)
#define CUTGATE_ETIME           (1)

/************************* EXTRA SHORT BUFS **********************/

/* ZIGZAG */
#define ZIGZAG_SPLBUF           (0)

/************************* INTERNAL POINTERS (dz->ptr) ***********************/

/* GRAIN */
#define GR_GATEVALS                     (0)
#define GR_GATEBRKEND           (1)
#define GR_ENVEND               (2)
#define GR_ESTEPEND             (3)

/************* FLOAT ARRAY NAMES FOR SPEC PITCH, TRACK, and P_HEAR ****************/

#define CHBOT                           (0)
#define TESTPAMP                        (1)
#define TOTPAMP                         (2)

#define SIN_TABLE                       (0)

#define STACK_TRANS                     (0)
#define STACK_AMP                       (1)

/************* FLOAT ARRAY NAMES FOR SHUDDER ****************/

#define SHUD_TIMES                      (0)
#define SHUD_LEVEL                      (1)
#define SHUD_WIDTH                      (2)
#define SHUD_DEPTH                      (3)
#define SHUD_ENV0                       (4)
#define SHUD_ENV1                       (5)

/*TW March 2004 */
/* ENVSYN */
#define ENVSYN_ENV                      (0)     /* array AND ptr */
#define ENVSYN_ENVEND           (1)     /* ptr only */
