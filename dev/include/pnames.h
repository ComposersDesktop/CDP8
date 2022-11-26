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



                                                        /** VARIABLE NAMES **/

/************************************ SPEC *************************************/
/* RWD floats version */
/* ACCU */
#define ACCU_DINDEX             (0)
#define ACCU_GINDEX             (1)

/* ARPE */
#define ARPE_WTYPE              (0)     /* int */
#define ARPE_ARPFRQ             (1)
#define ARPE_PHASE              (2)
#define ARPE_LOFRQ              (3)
#define ARPE_HIFRQ              (4)
#define ARPE_HBAND              (5)
#define ARPE_AMPL               (6)
#define ARPE_NONLIN             (7)
#define ARPE_SUST               (8)      /* int */
        /* internal */
#define ARPE_LAST_TABPOS (9)
#define ARPE_LASTARPFRQ  (10)
#define ARPE_SUSFLAG    (11) /* int */
#define ARPE_WAVETABPOS  (12) /* int */

/* AVRG */
#define AVRG_AVRG               (0)     /* int */
        /* internal */
#define AVRG_AVRGSPAN   (1)     /* int */


/* BLTR & BLUR */
#define BLUR_BLURF              (0)     /* int */
/* BLTR */
#define BLTR_TRACE              (1)     /* int */
#define BLTR_MAXTRACE   (2)     /* int */

/* BRIDGE */
#define BRG_OFFSET              (0)
#define BRG_SF2                 (1)
#define BRG_SA2                 (2)
#define BRG_EF2                 (3)
#define BRG_EA2                 (4)
#define BRG_STIME               (5)
#define BRG_ETIME               (6)
        /* internal */
#define BRG_TFAC                (7)
#define BRG_FSTEP               (8)
#define BRG_ASTEP               (9)
#define BRG_TSTEP               (10)
#define BRG_SWIN                (11) /* int */
#define BRG_INTPEND             (12) /* int */
#define BRG_STARTIS             (13) /* int */
#define BRG_TAILIS              (14) /* int */

/* CHANNEL */
#define CHAN_FRQ                (0)

/* CHORD */
#define CHORD_LOFRQ             (0)
#define CHORD_HIFRQ             (1)

/* CHORUS */
#define CHORU_AMPR              (0)
#define CHORU_FRQR              (1)
        /* internal */
#define CHORU_RTABSIZE  (2)     /* int */
#define CHORU_SPRTYPE   (3)     /* int */

/* CLEAN */
#define CL_FRQ                  (0)
#define CL_SKIPT                (0)     /* NB : same param storage location as CL_FRQ: alternatives !! */
#define CL_GAIN                 (1)
        /* internal */
#define CL_SKIPW                (2)     /* int */

/* CROSS */
#define CROS_INTP               (0)     /* cross */

/* CUT */
#define CUT_STIME               (0)
#define CUT_ETIME               (1)

/* DIFF */
#define DIFF_CROSS              (0)

/* DRUNK */
#define DRNK_RANGE              (0)      /* int */
#define DRNK_STIME              (1)
#define DRNK_DUR                (2)
        /* internal */
#define DRNK_TWICERANGE (3)     /* int */

/* EXAG */
#define EXAG_EXAG               (0)

/* FILT */
#define FILT_FRQ1               (0)
#define FILT_FRQ2               (1)
#define FILT_QQ                 (2)
#define FILT_PG                 (3)

/* FMNTSEE */
        /* internal */
#define FMNTSEE_MAX             (0)

/* FOCUS */
#define FOCU_PKCNT              (0) /* int */
#define FOCU_BW                 (1)
#define FOCU_LOFRQ              (2)
#define FOCU_HIFRQ              (3)
#define FOCU_STABL              (4)     /* int */
        /* internal */
#define FOCU_BRATIO_DN  (5)
#define FOCU_BRATIO_UP  (6)
#define FOCU_SL1                (7)     /* int */

/* FOLD */
#define FOLD_LOFRQ              (0)
#define FOLD_HIFRQ              (1)
        /* internal */
#define FOLD_HICHAN             (2)     /* int */
#define FOLD_LOCHAN             (3)     /* int */
#define FOLD_HICHANTOP  (4)
#define FOLD_LOCHANBOT  (5)

/* FORM */
#define FORM_FBOT               (0)
#define FORM_FTOP               (1)
#define FORM_GAIN               (2)

/* FORMANTS */
   /* internal */
//TW RENAMED
#define FMNT_SAMPS_TO_WRITE  (0)        /* int */

/* FORMSEE */
        /* internal */
#define FSEE_FMAX               (0)
#define FSEE_FCNT               (1)     /* int */

/* FREQUENCY */
#define FRQ_CHAN                (0)     /* int */

/* GAIN */
#define GAIN_GAIN               (0)

/* GLIDE */
#define GLIDE_DUR               (0)

/* GLIS */
#define GLIS_RATE                       (0)
#define GLIS_SHIFT                      (1)
#define GLIS_HIFRQ                      (2)
        /* internal */
#define GLIS_CONVERTOR          (3)
#define GLIS_HALF_SHIFT         (4)
#define GLIS_REFPITCH           (5)
#define GLIS_HALF_REFPITCH      (6)
#define GLIS_FRQTOP_TOP         (7)
#define GLIS_ROLL_OFF           (8)
#define GLIS_BASEFRQ            (9)

/* GRAB */
#define GRAB_FRZTIME    (0)     /* as magnify */

/* LEAF */
#define LEAF_SIZE               (0)     /* int */

/* MAGNIFY */
#define MAG_FRZTIME     (0)     /* as grab */
#define MAG_DUR                 (1)

/* MEAN */
#define MEAN_LOF                (0)
#define MEAN_HIF                (1)
#define MEAN_CHAN               (2)     /* int */
        /* internal */
#define MEAN_TOP                (3)     /* int */
#define MEAN_BOT                (4)     /* int */

/* MORPH */
#define MPH_ASTT                (0)
#define MPH_AEND                (1)
#define MPH_FSTT                (2)
#define MPH_FEND                (3)
#define MPH_AEXP                (4)
#define MPH_FEXP                (5)
#define MPH_STAG                (6)
          /* internal */
#define MPH_ASTTW               (7)      /* int */
#define MPH_AENDW               (8)  /* int */
#define MPH_FSTTW               (9)  /* int */
#define MPH_FENDW               (10) /* int */
#define MPH_STAGW               (11) /* int */

/* NOISE */
#define NOISE_NOIS              (0)

/* OCT */
#define OCT_HMOVE               (0)      /* int */
#define OCT_BREI                (1)

/* OCTVU */
#define OCTVU_TSTEP             (0)
#define OCTVU_FUND              (1)
        /* internal */
#define OCTVU_BBBTOP    (2)                              /* bottom-band bandtop */
#define OCTVU_TBLOK             (3)     /* int */

/* P_APPROX */
#define PA_PRANG                (0)
#define PA_TRANG                (1)
#define PA_SRANG                (2)

/* P_CUT */
#define PC_STT                  (0)
#define PC_END                  (1)

/* P_EXAG */
#define PEX_MEAN                (0)
#define PEX_RANG                (1)
#define PEX_CNTR                (2)

/* P_FIX */
#define PF_SCUT                 (0)
#define PF_ECUT                 (1)
#define PF_LOF                  (2)
#define PF_HIF                  (3)
#define PF_SMOOTH               (4) /* int */
#define PF_SMARK                (5)
#define PF_EMARK                (6)
        /* internal */
#define PF_SCUTW        (7) /* int */
#define PF_ECUTW        (8) /* int */
#define PF_ISCUT        (9) /* int */
#define PF_ISFILTER     (10) /* int */

/* P_HEAR */
#define PH_GAIN                 (0)

/* P_INFO */
        /* internal */
#define PINF_MEAN               (0)
#define PINF_MAX                (1)
#define PINF_MIN                (2)
#define PINF_MAXPOS             (3)
#define PINF_MINPOS             (4)

/* P_INVERT */
#define PI_MEAN                 (0)
#define PI_BOT                  (1)
#define PI_TOP                  (2)

/* P_RANDOMISE */
#define PR_MXINT                (0)
#define PR_TSTEP                (1)
#define PR_SLEW                 (2)
        /* internal */
#define PR_NEGATIV_SLEW (3)

/* P_SEE */
#define PSEE_SCF                (0)

/* P_SMOOTH */
#define PS_TFRAME               (0)
#define PS_MEAN                 (1)

/* P_TRANSPOSE */
#define PT_TVAL                 (0)

/* P_VIBRATO */
#define PV_FRQ                  (0)
#define PV_RANG                 (1)

/* P_WRITE */
#define PW_DRED                 (0)

/* PEAK */
#define PEAK_CUTOFF             (0)
#define PEAK_TWINDOW    (1)
#define PEAK_FWINDOW    (2)
        /* internal */
#define PEAK_ENDTIME    (3)
#define PEAK_TGROUP             (4)     /* int */
#define PEAK_BANDCNT    (5)     /* int */

/* PICK */
#define PICK_FUND               (0)
#define PICK_LIN                (1)
#define PICK_CLAR               (2)
        /* internal */
#define PICK_LONGPOW2   (3)     /* int */
#define PICK_DIVMASK    (4)     /* int */

/* PITCH */
#define PICH_RNGE               (0)
#define PICH_VALID              (1)     /* int */
#define PICH_SRATIO             (2)
#define PICH_MATCH              (3) /* int */
#define PICH_LOLM               (4)
#define PICH_HILM               (5)
#define PICH_DATAREDUCE (6)
        /* internal */
#define PICH_PICH               (7)

/* PLUCK */
#define PLUK_GAIN               (0)
        /* internal */
#define PLUK_LONGPOW2   (1)     /* int */
#define PLUK_DIVMASK    (2)     /* int */

/* PRINT */
#define PRNT_STIME              (0)
#define PRNT_WCNT               (1)     /* int */
        /* internal */
#define PRNT_STARTW             (2)     /* int */
#define PRNT_ENDW               (3)     /* int */

/* REPITCHB */
#define RP_DRED                 (0)

/* REPORT */
#define REPORT_PKCNT                    (0) /* int */
#define REPORT_LOFRQ                    (1)
#define REPORT_HIFRQ                    (2)
#define REPORT_STABL                    (3)     /* int */
        /* internal */
#define REPORT_SL1                              (4)     /* int */
#define REPORT_LAST_PKCNT_HERE  (5) /* int */

/* SCAT */
#define SCAT_CNT                                (0)     /* int */
#define SCAT_BLOKSIZE                   (1)     /* only later round to INT converting frqspan->chan_cnt: special case */
        /* internal */
#define SCAT_THISCNT                    (2)     /* int */
#define SCAT_BLOKS_PER_WINDOW   (3)     /* int */

/* SHIFT */
#define SHIFT_SHIF              (0)
#define SHIFT_FRQ1              (1)
#define SHIFT_FRQ2              (2)

/* SHIFTP */
#define SHIFTP_FFRQ             (0)
#define SHIFTP_SHF1             (1)
#define SHIFTP_SHF2             (2)
#define SHIFTP_DEPTH    (3)
        /* internal */
#define SHIFTP_FDCNO    (4)     /* int */
#define SHIFTP_S1L1             (5)
#define SHIFTP_1LS1             (6)
#define SHIFTP_S2L1             (7)
#define SHIFTP_1LS2             (8)
#define SHIFTP_NS1              (9)
#define SHIFTP_NS2              (10)

/* SHUFFLE */
#define SHUF_GRPSIZE    (0)     /* int */
        /* internal */
#define SHUF_DMNCNT             (1)     /* int */
#define SHUF_IMGCNT             (2)     /* int */
#define SHUF_I_WANTED   (3)     /* int */
#define SHUF_D_WANTED   (4)     /* int */
#define SHUF_I_BSZ              (5)     /* int */
#define SHUF_D_BSZ              (6)     /* int */

/* SPREAD */
#define SPREAD_SPRD             (0)

/* STEP */
#define STEP_STEP               (0)     /* only later round to INT converting time->windowcnt: special case */

/* STRETCH */
#define STR_FFRQ                (0)
#define STR_SHIFT               (1)
#define STR_EXP                 (2)
#define STR_DEPTH               (3)
        /* internal */
#define STR_FDCNO               (4)     /* int */
#define STR_SL1                 (5)
#define STR_NSHIFT              (6)

/* SUM */
#define SUM_CROSS               (0)

/* SUPR */
#define SUPR_INDX               (0) /* int */

/* TRACE */
#define TRAC_INDX               (0) /* int */
#define TRAC_LOFRQ              (1)
#define TRAC_HIFRQ              (2)

/* TRACK */
#define TRAK_PICH               (0)
#define TRAK_RNGE               (1)
#define TRAK_VALID              (2) /* int */
#define TRAK_SRATIO             (3)
#define TRAK_HILM               (4)
#define TRAK_DATAREDUCE (5)
        /* internal */
#define TRAK_LOLM               (6)

/* TRNSF */
#define TRNSF_LOFRQ             (0)
#define TRNSF_HIFRQ             (1)

/* TRSNP */
#define TRNSP_LOFRQ             (0)
#define TRNSP_HIFRQ             (1)

/* TSTRETCH */
#define TSTR_STRETCH    (0)
        /* internal */
#define TSTR_TOTIME             (1)
#define TSTR_ARRAYSIZE  (2)     /* int */

/* TUNE */
#define TUNE_FOC                (0)
#define TUNE_CLAR               (1)
#define TUNE_INDX               (2) /* int */
#define TUNE_BFRQ               (3)

/* VOCODE */
#define VOCO_LOF                (0)
#define VOCO_HIF                (1)
#define VOCO_GAIN               (2)

/* WARP */
#define WARP_PRNG               (0)
#define WARP_TRNG               (1)
#define WARP_SRNG               (2)
                /* internal */
#define WARP_WREAD              (3)     /* int */
#define WARP_PART_INBUF (4)     /* int */
#define WARP_ATEND              (5)     /* int */

/* WAVER */
#define WAVER_VIB               (0)
#define WAVER_STR               (1)
#define WAVER_LOFRQ             (2)
#define WAVER_EXP               (3)
        /* internal */
#define WAVER_BOTCHAN   (4) /* int */
#define WAVER_STRCHANS  (5) /* int */

/* WEAVE */
        /* internal */
#define WEAVE_BAKTRAK   (0)     /* int */

/* LIMIT */
#define LIMIT_THRESH    (0)


/******************************* GROUCHO ********************************/

/************ NAMES FOR PARAMS, ,OPTIONS, VARIANTS, INTERNAL PARAMS ************/

/* DISTORT */
#define DISTORT_POWFAC          (0)
        /* internal */
#define DISTORT_PULSE           (1) /* int */

/* DISTORT_ENV */
#define DISTORTE_CYCLECNT       (0)     /* int */
        /* param OR variant */
#define DISTORTE_TROF           (1)
        /* variant */
#define DISTORTE_EXPON          (2)
        /* internal */
#define ONE_LESS_TROF           (3)

/* DISTORT_AVG */
#define DISTORTA_CYCLECNT       (0)     /* int */
        /* option */
#define DISTORTA_MAXLEN         (1)
        /* variant */
#define DISTORTA_SKIPCNT        (2)
        /* internal */
#define DISTORTA_CYCBUFLEN      (3)     /* int */

/* DISTORT_OMT */
#define DISTORTO_OMIT           (0)     /* int */
#define DISTORTO_KEEP           (1)     /* int */

/* DISTORT_MLT */
/* DISTORT_DIV */
#define DISTORTM_FACTOR         (0)     /* int */
/* DISTORT_MLT flag name only */
#define DISTORTM_SMOOTH_FLAG (1)
/* DISTORT_DIV flag name only */
#define DISTORTD_INTERP_FLAG (1)

/* DISTORT_HRM */
#define DISTORTH_PRESCALE       (0)
        /* internal */
#define DISTORTH_CYCBUFLEN      (1)     /* int */
#define DISTORTH_HCNT           (2)     /* int */
#define FOLDOVER_WARNING        (3)     /* int */

/* DISTORT_FRC */
#define DISTORTF_SCALE          (0)     /* int */
#define DISTORTF_AMPFACT        (1)
#define DISTORTF_PRESCALE       (2)
        /* internal */
#define DISTORTF_CYCBUFLEN      (3)     /* int */

/* DISTORT_REV */
#define DISTORTR_CYCLECNT       (0)     /* int */

/* DISTORT_SHUF */
#define DISTORTS_CYCLECNT       (0)     /* int */
#define DISTORTS_SKIPCNT        (1)     /* int */
        /* internal */
#define DISTORTS_DMNCNT         (2)     /* int */
#define DISTORTS_IMGCNT         (3)     /* int */

/* DISTORT_RPT , DISTORT_RPTFL */
#define DISTRPT_MULTIPLY                (0) /* int */
#define DISTRPT_CYCLECNT                (1) /* int */
#define DISTRPT_SKIPCNT                 (2) /* int */
#define DISTRPT_CYCLIM                  (3)     /* float */
/* DISTORT_INTP */
#define DISTINTP_MULTIPLY               (0) /* int */
#define DISTINTP_SKIPCNT                (1) /* int */

/* DISTORT_DEL */
#define DISTDEL_CYCLECNT                (0) /* int */
#define DISTDEL_SKIPCNT                 (1) /* int */

/* DISTORT_RPL */
#define DISTRPL_CYCLECNT                (0) /* int */
#define DISTRPL_SKIPCNT                 (1) /* int */

/* DISTORT_TEL */
#define DISTTEL_CYCLECNT                (0) /* int */
#define DISTTEL_SKIPCNT                 (1) /* int */
#define DISTTEL_AVG_FLAG                (2)     /* flagname only */

/* DISTORT_FLT */
#define DISTFLT_LOFRQ_CYCLELEN  (0)
#define DISTFLT_HIFRQ_CYCLELEN  (1)
#define DISTFLT_SKIPCNT                 (2) /* int */
        /* internal ??  */
#define DISTFLT_SRATE                   (3)
#define DISTFLT_NYQUIST                 (4)

/* DISTORT_PCH */
#define DISTPCH_OCTVAR                  (0)
#define DISTPCH_CYCLECNT                (1) /* int */
#define DISTPCH_SKIPCNT                 (2) /* int */

/* DISTORT_OVERLOAD */
#define DISTORTER_MULT                  (0)
#define DISTORTER_DEPTH                 (1)
#define DISTORTER_FRQ                   (2)

/************ NAMES FOR PARAMS, ,OPTIONS, VARIANTS, INTERNAL PARAMS ************/

/* CREATE, EXTRACT, WARPING, REPLOTTING, CONVERT */
#define ENV_WSIZE               (0)             /* MSECS */
/* SWELL */
#define ENV_PEAKTIME    (0)     /* double OR int : depends on MODE */
/* DOVETAILING */
#define ENV_STARTTRIM   (0)     /* double OR int : depends on MODE */
#define ENV_ENDTRIM             (1)     /* double OR int : depends on MODE */
#define ENV_STARTTYPE   (2) /* int */
#define ENV_ENDTYPE             (3) /* int */
/* CURTAILING */
#define ENV_STARTTIME   (0)     /* double : but will pre-convert?? to int, depending on ENV_TIMETYPE */
#define ENV_ENDTIME             (1)     /* double : but will pre-convert?? to int, depending on ENV_TIMETYPE */
/* CURTAILING AND SWELL */
#define ENV_ENVTYPE         (2) /* int */

/* ATTACK */
#define ENV_ATK_GATE    (0)
#define ENV_ATK_ATTIME  (0)
#define ENV_ATK_GAIN    (1)
#define ENV_ATK_ONSET   (2)
#define ENV_ATK_TAIL    (3)

/* DOVETAILING, CURTAILING */
#define ENV_TIMETYPE    (4) /* int */

/* ATTACK */
#define ENV_ATK_ENVTYPE (4) /* int */


/* WARPING_RESHAPING,REPLOTTING */
#define ENV_EXAG                (1)
#define ENV_ATTEN               (1)
#define ENV_LIFT                (1)
#define ENV_FLATN               (1)     /* int */
#define ENV_GATE                (1)
#define ENV_TROFDEL             (1)     /* int */
#define ENV_LIMIT               (1)
#define ENV_AVERAGE             (1)     /* int */
#define ENV_TSTRETCH    (1)

/* various */
#define ENV_THRESHOLD   (2)
#define ENV_MIRROR              (2)
/* korrug */
#define ENV_PKSRCHWIDTH (2)     /* int */
/* trigger */
#define ENV_TRIGRISE    (2)

/* trigger */
#define ENV_TRIGDUR             (3)
/* expand and gate */
#define ENV_SMOOTH              (3)     /* int */

/* EXTRACT, REPLOTTING, CONVERT : OPTION */
#define ENV_DATAREDUCE  (4)

/* INTERNAL */
#define ENV_SAMP_WSIZE  (5)     /* int */                  /*RWD*/
#define ENV_ATK_TIME    (6)     /* int */

/* PLUCK */
#define ENV_PLK_ENDSAMP (0)
#define ENV_PLK_WAVELEN (1)
#define ENV_PLK_CYCLEN  (4)
#define ENV_PLK_DECAY   (5)
/* INTERNAL for PLUCK */
#define ENV_PLK_OBUFLEN         (6)
#define ENV_PLK_OWRAPLEN        (7)

/* TREMOL */
#define ENV_TREM_FRQ        (0)
#define ENV_TREM_DEPTH      (1)
#define ENV_TREM_AMP        (2)

/************ NAMES FOR PARAMS, ,OPTIONS, VARIANTS, INTERNAL PARAMS ************/

/* ZIGZAG */
#define ZIGZAG_START  (0)
#define ZIGZAG_END        (1)
#define ZIGZAG_DUR        (2)
#define ZIGZAG_MIN        (3)
#define ZIGZAG_SPLEN  (4)
#define ZIGZAG_MAX        (5)
#define ZIGZAG_RSEED  (6)
/* internal */
#define ZIG_SPLICECNT (7)
#define ZIG_SPLBYTES  (8)
#define ZIG_SPLSAMPS  (9)
#define ZIG_RUNSTOEND (10)

/* LOOP */
#define LOOP_OUTDUR     (0)
#define LOOP_REPETS     (1)     /* int */
#define LOOP_START      (2)
#define LOOP_LEN        (3)
#define LOOP_STEP       (4)
#define LOOP_SPLEN      (5)
#define LOOP_SRCHF      (6)
/* flag only : only for names */
#define FROM_START_FLAG (7)
/* internal */
/* as ZIGZAG (7) */
/* as ZIGZAG (8) */
/* as ZIGZAG (9) */
/* as ZIGZAG (10)*/

/* SCRAMBLE */
#define SCRAMBLE_MIN     (0)
#define SCRAMBLE_LEN     (0) /* different mode */
#define SCRAMBLE_MAX     (1)
#define SCRAMBLE_SCAT    (1) /* different mode */
#define SCRAMBLE_DUR     (2)
#define SCRAMBLE_SPLEN   (3)
#define SCRAMBLE_SEED    (4) /* int */
/* flag only : only for names : DO NOT CONFUSE WITH INTERNAL FLAG NAMES !!!! */
#define FROM_START_SCRAMBLE (5)
#define TO_END_SCRAMBLE         (6)
/* internal */
#define SCRAMBLE_OUTLEN  (5) /* int */
#define SCRAMBLE_CHCNT   (6) /* int */
/* as ZIGZAG (7) */
/* as ZIGZAG (8) */
/* as ZIGZAG (9) */
/* as ZIGZAG (10)*/

#define ITER_DUR                (0)
#define ITER_REPEATS    (0)
#define ITER_DELAY              (1)
#define ITER_RANDOM             (2)
#define ITER_PSCAT              (3)
#define ITER_ASCAT              (4)
#define ITER_FADE               (5)
#define ITER_GAIN               (6)
#define CHUNKSTART              (5)
#define CHUNKEND                (6)
#define ITER_RSEED              (7)
/* internal params */
#define ITER_STEP               (8)
#define ITER_MSAMPDEL   (10)
#define ITER_DO_SCALE   (11)
#define ITER_PROCESS    (12)

#define ITER_LGAIN      (7)
#define ITER_RRSEED (8)
#define ITER_SSTEP  (9)

/* DRUNKWALK */
#define DRNK_TOTALDUR    (0)    /* d */
#define DRNK_LOCUS               (1)    /* D */
#define DRNK_AMBITUS     (2)    /* D */
#define DRNK_GSTEP               (3)    /* D */
#define DRNK_CLOKTIK     (4)    /* D */
#define DRNK_MIN_DRNKTIK (5)    /* I */
#define DRNK_MAX_DRNKTIK (6)    /* I */
/* option */
#define DRNK_SPLICELEN   (7)    /* d */
/* flagged */
#define DRNK_CLOKRND     (8)    /* D */
#define DRNK_OVERLAP     (9)    /* D */
#define DRNK_RSEED               (10)   /* i */
#define DRNK_MIN_PAUS    (11)   /* D */
#define DRNK_MAX_PAUS    (12)   /* D */
/* internal params */
#define DRNK_LGRAIN               (13)  /* int */
#define DRNK_DWNSHIFT     (14)  /* int */
#define DRNK_MAXHOLD      (15)  /* int */
#define DRNK_MAX_OVLAP    (16)  /* int */
#define DRNK_DRNKTIK_RANG (17)  /* int */
#define DRNK_PAUS_RANG    (18)  /* int */
#define DRNK_LAST_LOCUS   (19)  /* int */

/******************* PARAMS ********************/

#define FLT_Q           (0)
/* filteq */
#define FLT_BW          (0)
#define FLT_GAIN        (1)
/* eq */
#define FLT_BOOST       (1)
#define FLT_LOFRQ       (2)
/* eq, fstatvar */
#define FLT_ONEFRQ      (2)
/* lphp */
#define FLT_PASSFRQ     (2)
/* allpass, fltiter */
#define FLT_DELAY       (2)
/* allpass */
#define FLT_HIFRQ       (3)
/* fltiter */
#define FLT_OUTDUR      (3)
/* lphp */
#define FLT_STOPFRQ     (3)

/* fltbankn */
#define FLT_INCOUNT     (4)     /* CARE!! don't confuse with FLT_CNT */
#define FLT_INTSIZE     (4)
#define FLT_OFFSET      (4)
/* fltsweep */
#define FLT_SWPFRQ      (4)
//TW NEW
#define FILT_TAIL       (5)
#define FILT_TAILV      (3)
/*----------- options ------------*/
//TW NEW, NUMBERING MODIFIED
/* fltbnkhv */
#define FLT_HARMCNT     (4)     /* int */
#define FLT_ROLLOFF     (5)
#define FLT_PRESCALE (6)
/* fltbankn */
#define FLT_RANDFACT (6)
/* fltsweep */
#define FLT_SWPPHASE (6)
/* fltiter */
#define FLT_RANDDEL  (7)
#define FLT_PSHIFT       (8)
#define FLT_AMPSHIFT (9)
/*----------- internal -----------*/
//TW NEW, NUMBERING MODIFIED
#define FLT_WORDCNT             (8)
#define FLT_ENTRYCNT    (9)             /* int */       /* no of entries on a fltbankv line */
#define FLT_INV_SR              (10)                                    /* inverse of srate */
#define FLT_CNT                 (11)    /* int */  /* number of filters */
#define FLT_SAMS                (12)    /* int */  /* Q-(or delay-)brkpnt_segment sample-counter */
#define FLT_OVFLW               (13)    /* int */  /* overflows counter */
#define FLT_BLOKSIZE    (14)    /* int */  /* bloksize before filter-val altered */
#define FLT_BLOKCNT             (15)    /* int */  /* counter inside filter block */
#define FLT_Q_INCR              (16)                       /* samp-to-samp increment of Q in current Q-blok */

/* allpass */
//TW NEW, NUMBERING MODIFIED
#define FLT_D_INCR              (10)                                    /* samp-to-samp increment of delay in current delay-blok */
#define FLT_MAXDELSAMPS (11)    /* int */   /* max delay in (stereo-pairs-counted-as-1) samples */
#define FLT_DELBUFPOS   (15)    /* int */       /* current position for writing to delay buffer */
#define FLT_PHSAMS              (16)    /* int */       /* phasing strength brkpnt counter */
#define FLT_PH_INCR         (17)                                /* samp-to-samp increment of strength in current strength-blok */

/* eq */
//TW NEW, NUMBERING MODIFIED
#define FLT_A0                  (11)                            /* filter coeff for eq */
#define FLT_A1                  (12)                            /* filter coeff for eq */
#define FLT_A2                  (14)                            /* filter coeff for eq */
#define FLT_B1                  (15)                            /* filter coeff for eq */
#define FLT_B2                  (16)                            /* filter coeff for eq */

/* lphp */
//TW NEW, NUMBERING MODIFIED
#define FLT_MUL                   (10)
#define FLT_INTERNAL_GAIN (12)

/* time-varying filter frq-amps */
//TW NEW, NUMBERING MODIFIED
#define FLT_FSAMS               (17)    /* int */  /* timed-filter-'brkpnt'_segment sample-counter */
#define FLT_F_INCR              (18)                       /* samp-to-samp increment of Filt frq in current flt-blok */
/* above 2 also used in FSTATVAR */
#define FLT_TIMESLOTS   (19)    /* int */  /* number of timeslotes for changing filter */
#define FLT_TIMES_CNT   (20)    /* int */  /* runnig counter of tvary-filter timeslots */
#define FLT_FRQ_INDEX   (21)    /* int */  /* base index for next set of filter-data, tvarying filter */
#define FLT_A_INCR              (22)                       /* samp-to-samp increment of Filt amp in current flt-blok */
#define HRM_ENTRYCNT    (23)
#define HRM_WORDCNT             (24)
#define FLT_TIMESTEP    (25)
#define FLT_TOTALTIME   (26)

/* fltsweep */
//TW NEW, NUMBERING MODIFIED
#define FLT_CYCPOS      (17)                       /* position in sweep-wave cycle */
#define FLT_SWINCR              (18)                       /* samp-to-samp increment of sweepfrq in current sfrq blok */
/* NB This has to be at 18, because FSTATVAR also uses it */
#define FLT_QFAC                (19)                       /* Q derived factor entering into filter calcs */
#define FLT_SWSAMS              (20)                       /* sweepfrq-brkpnt segment sample-counter */
#define FLT_LOINCR              (21)                       /* samp-to-samp increment of lofrq in current lofrq blok */
#define FLT_LOSAMS              (22)                       /* lofrq-brkpnt segment sample-counter */
#define FLT_HIINCR              (23)                       /* samp-to-samp increment of hifrq in current hifrq blok */
#define FLT_HISAMS              (24)                       /* hifrq-brkpnt segment sample-counter */
/* fltiter */
//TW NEW, NUMBERING MODIFIED
#define FLT_INMSAMPSIZE (14)    /* int */  /* input size counted in (stereopair = 1) samples */
#define FLT_MSAMPDEL    (15)    /* int */  /* iteration delay counted ditto */
#define FLT_SMPDEL              (16)    /* int */  /* iteration delay counted in standard (short) samples */
#define FLT_DOFLAG              (17)    /* int */  /* internal flag for fltiter */
#define FLT_OVFLWSIZE   (18)    /* int */  /* size of overflow buffer for fltiter */
#define FLT_INFILESPACE (19)    /* int */  /* size of input buffers for fltiter */
#define FLT_WRITESTART  (20)    /* int */  /* position in buffer to write next filtered copy, for fltiter */
#define FLT_MAXOVERLAY  (21)    /* int */  /* maximum superimpositions of infile, for fltiter */
#define FLT_SAMPATTEN   (22)                       /* sample-by-sample attenuation */

/************ NAMES FOR PARAMS, ,OPTIONS, VARIANTS, INTERNAL PARAMS ************/

/* params : PARTICULAR */
/* OMIT */
#define GR_KEEP                         (0)  /* I */
#define GR_OUT_OF                       (1)  /* i */
/* DUPLICATE */
#define GR_DUPLS                        (0)  /* I */
/* TIMEWARP */
#define GR_TSTRETCH                     (0)  /* D */
/* POSITION, ALIGN */
#define GR_OFFSET                       (0)  /* d */
/* ALIGN */
#define GR_GATE2                        (1)  /* D */
/*---------------------*/
/* options/flags with params : UNIVERSAL: MUST BE SO, for preprocessing to work!!! */
#define GR_BLEN                         (2)  /* d */
#define GR_GATE                         (3)  /* I */
#define GR_MINTIME                      (4)      /* d */
#define GR_WINSIZE                      (5)      /* d */
/*---------------------*/
/* flag name ONLY */
#define LOSE_LAST_GRAIN_FLAG (6)  /* 0 */
/*---------------------*/
/* internal : UNIVERSAL */
#define GR_NGATE                        (6)
#define GR_MINHOLE                      (7)  /* int */
/*#define       GR_WSIZE_BYTES          (7)*/    /* int */
#define GR_WSIZE_SAMPS          (8)                                                /* RWD */
#define GR_WINCNT                       (9)      /* int */
#define GR_SPLICELEN            (10)  /* int */
#define GR_SPLUS1                       (11) /* int */
#define GR_ABS_SPLICELEN        (12) /* int */
#define GR_ABS_SPLICEX2         (13) /* int */
#define GR_THISBRKSAMP          (14) /* int */
#define GR_NEXTBRKSAMP          (15) /* int */
#define GR_NEXTGATE                     (16)
#define GR_FGATE                        (17)
#define GR_GATESTEP                     (18)
#define GR_UP                           (19) /* int */
#define GR_TESTLIM                      (20) /* int */
/*---------------------*/
/* internal : PARTICULAR */
/* reposition, [find?] & align only */
#define GR_SYNCCNT                      (21) /* int */
/* reverse */
#define GR_ARRAYSIZE            (21) /* int */
/* reordr only */
#define GR_REOSTEP                      (21) /* int */
#define GR_REOCNT                       (22) /* int */
#define GR_REOLEN                       (23) /* int */
/* repitch, rerhythm, remotif only */
#define GR_RATIOCNT                     (21) /* int */
/* repitch: remotif only */
#define GR_STORESIZE            (22) /* int */

/************************** MIX PARAMS *********************/

/* inbetween */
#define INBETW                  (0)
/* cyc_inbetween */
#define BTWN_HFRQ               (1)
/* cross */
#define MCR_STAGGER             (0)
#define MCR_BEGIN               (1)
#define MCR_END                 (2)
#define MCR_POWFAC              (3)
        /* internal */
#define MCR_CONTOUR             (4)     /* int */
#define MCR_INDEX               (5)     /* int */
#define MCR_CROSFACT    (6)

/* merge */
#define MIX_STAGGER             (0)
#define MIX_SKIP                (1)
#define MIX_SKEW                (2)
#define MIX_STTA                (3)
#define MIX_DURA                (4)
        /* internal */
#define MIX2_GAIN1              (5)
#define MIX2_GAIN2              (6)

/* twarp */
#define MTW_PARAM                       (0)
/* sync */
#define MSY_WFAC                        (0)     /* int */
        /* flagname only */
#define MSY_POWMETHOD_FLAG      (1)
        /* internal */
#define MSY_SRFAC                       (1)     /* int */
/* swarp */
#define MSW_TWLINE                      (0)
#define MSW_NARROWING           (0)
#define MSW_POS1                        (0)
#define MSW_POS2                        (1)
/* gain */
#define MIX_GAIN                        (0)
/* shuffle, twarp, swarp, gain */
#define MSH_STARTLINE           (2)
#define MSH_ENDLINE                     (3)
#define MSH_NOCHECK_FLAG        (4)

/* real mix */
#define MIX_START                       (0)
#define MIX_END                         (1)
#define MIX_ATTEN                       (2)
        /* flagname only */
#define ALTERNATIVE_MIX_FLAG    (2)
        /* internal */
#define MIX_STRTPOS                             (2)     /* int */
#define MIX_STRTPOS_IN_ACTION   (3)     /* int */
#define MIX_TOTAL_ACTCNT                (4)     /* int */

/* FLAGS */

/* mix */
#define ALTERNATIVE_MIX         (0)
/* mixshufl */
#define MSH_NOCHECK                     (0)

#define MSY_POWMETHOD           (0)

/* ARRAYS (double) */

/* sync */
#define MSY_TIMESTOR            (0)
#define MSY_STARTSRCH           (1)
#define MSY_ENDSRCH                     (2)
/* timewarp */
#define MTW_TIMESTOR            (0)
#define MTW_TIMEDIFF            (1)
/* spacewarp */
#define MSW_TIMESTOR            (0)
/* gain */
#define MIX_LLEVELSTOR          (0)
#define MIX_RLEVELSTOR          (1)
/* merge */
#define MCR_COSTABLE            (0)

/* ARRAYS (long) */

/* sync */
#define MSY_PEAKSAMP            (0)
#define MSY_SAMPSIZE            (1)
#define MSY_ENVEL                       (2)

/* ARRAYS (int) */

/* sync */
#define MSY_CHANS                       (0)
#define MSY_ENVPOS                      (1)

/******* PARAMS ******/
/* brassage */
#define GRS_VELOCITY            (0)
#define GRS_DENSITY                     (1)
#define GRS_HVELOCITY           (2)
#define GRS_HDENSITY            (3)
#define GRS_GRAINSIZE           (4)
#define GRS_PITCH                       (5)
#define GRS_AMP                         (6)
#define GRS_SPACE                       (7)
#define GRS_BSPLICE                     (8)
#define GRS_ESPLICE                     (9)
#define GRS_HGRAINSIZE          (10)
#define GRS_HPITCH                      (11)
#define GRS_HAMP                        (12)
#define GRS_HSPACE                      (13)
#define GRS_HBSPLICE            (14)
#define GRS_HESPLICE            (15)
#define GRS_SRCHRANGE           (16)
#define GRS_SCATTER                     (17)
#define GRS_OUTLEN                      (18)
#define GRS_CHAN_TO_XTRACT      (19)
/* flagnames only */
//#define GRS_NO_DECCEL_FLAG    (20)
//#define GRS_EXPON_FLAG                (21)
//#define GRS_NO_INTERP_FLAG    (22)
/* internal */
#define GRS_PRANGE                      (20)
#define GRS_SPRANGE                     (21)
#define GRS_VRANGE                      (22)
#define GRS_DRANGE                      (23)
#define GRS_INCHANS                     (24)
#define GRS_OUTCHANS            (25)
#define GRS_BUF_SMPXS           (26)
#define GRS_BUF_XS                      (27)
#define GRS_GLBUF_SMPXS         (28)
#define GRS_LBUF_SMPXS          (29)
#define GRS_LBUF_XS                     (30)
#define GRS_IS_BTAB                     (31)
#define GRS_IS_ETAB                     (32)
#define GRS_CHANNELS            (33)
#define ORIG_SMPSIZE            (34)
#define SAMPS_IN_INBUF          (35)
#define GRS_ARANGE                      (36)
#define GRS_GRANGE                      (37)
#define GRS_BRANGE                      (38)
#define GRS_ERANGE                      (39)
#define GRS_LONGS_BUFLEN        (40)
#define GRS_ARRAYSIZE           (41)

/************ NAMES FOR PARAMS, ,OPTIONS, VARIANTS, INTERNAL PARAMS ************/

#define TEXTURE_DUR                     (0)
#define TEXTURE_PACK            (1)     /* for applics with no prior timed set of notes */
#define TEXTURE_SKIP            (1)     /* for applics with a timed set already */
#define TEXTURE_SCAT            (2)
#define TEXTURE_TGRID           (3)
#define TEXTURE_INSLO           (4)       /* int */
#define TEXTURE_INSHI           (5)   /* int */
#define TEXTURE_MINAMP          (6)
#define TEXTURE_MAXAMP          (7)
#define TEXTURE_MINDUR          (8)
#define TEXTURE_MAXDUR          (9)
#define TEXTURE_MINPICH         (10)
#define TEXTURE_MAXPICH         (11)

#define TEX_PHGRID                      (12)
#define TEX_GPSPACE                     (13)  /* int */
#define TEX_GRPSPRANGE          (14) /* current spatial range of group */
#define TEX_AMPRISE                     (15)
#define TEX_AMPCONT                     (16)  /* int */
#define TEX_GPSIZELO            (17)  /* int */
#define TEX_GPSIZEHI            (18)  /* int */
#define TEX_GPPACKLO            (19)
#define TEX_GPPACKHI            (20)
#define TEX_GPRANGLO            (21)
#define TEX_GPRANGHI            (22)
#define TEX_MULTLO                      (23)
#define TEX_MULTHI                      (24)
#define TEX_DECPCENTRE          (25)  /* int */
        /* options or variants */
#define TEXTURE_ATTEN           (26)
#define TEXTURE_POS                     (27)
#define TEXTURE_SPRD            (28)
#define TEXTURE_SEED            (29)

#define TEX_IGNOREDUR_FLAG        (30)
#define DONT_KEEP_MTFDUR_FLAG (31)
#define INS_TO_SCATTER_FLAG       (32)
#define FORCEHI_FLAG              (33)
#define FORCE_EVERY_FLAG          (34)
#define DISCARD_ORIGLINE_FLAG (35)

/* internal params */
#define SPINIT                  (30) /* int */ /* flags spatialisation of motif beginning */
#define SPCNT                   (31) /* int */ /* count of spatialised motif items */
#define DIRECTION               (32) /* int */ /* spatial direction of motif */
#define CPOS                    (33)               /* central position of a motion at time x */
#define TPOSITION               (34)               /* spatial position of note in tset */
#define THISSPRANGE             (35)               /* current spatial range */
#define TEX_MAXOUT              (36) /* float */ /* abs max output level */             /*RWD*/

/* PVOC */

#define PVOC_CHANS_INPUT                (0)
#define PVOC_WINOVLP_INPUT              (1)
#define PVOC_CHANSLCT_INPUT             (2)
#define PVOC_LOCHAN_INPUT               (3)
#define PVOC_HICHAN_INPUT               (4)
#define PVOC_CHANS                              (5)
#define PVOC_WIN_OVERLAP                (6)
#define PVOC_MINSAMP                    (7)
#define PVOC_MAXSAMP                    (8)
#define PVOC_SELECTED_CHAN              (9)
#define PVOC_PARTIAL_RESYNTH    (10)
#define PVOC_AF_PAIR_LO                 (11)
#define PVOC_AF_PAIR_HI                 (12)
#define PVOC_ANAL_ONLY                  (13)
#define PVOC_ENVOUT_ONLY                (14)
#define PVOC_MAGOUT_ONLY                (15)
#define PVOC_SYNTH_ONLY                 (16)

/* MOD_LOUDNESS */

#define LOUD_GAIN                               (0)
#define LOUD_LEVEL                              (1)

/* MOD_SPACE */

#define PAN_PAN                                 (0)
#define PAN_PRESCALE                    (1)

#define NARROW                                  (0)

/* MOD_PITCH */

        /* strans, vtrans */
#define VTRANS_TRANS                    (0)
        /* accel */
#define ACCEL_ACCEL                             (0)
#define ACCEL_GOALTIME                  (1)
#define ACCEL_STARTTIME                 (2)
        /* vibrato */
#define VIB_FRQ                                 (0)
#define VIB_DEPTH                               (1)
        /* internal */
        /* strans, accel, vibrato */
#define VTRANS_HBUFSIZE                 (3)
        /* vtrans */
#define UNITSIZE                                (1)
#define UNIT_BUFLEN                             (2)
#define UNITS_READ                              (3)
#define TOTAL_UNITS_READ                (4)
#define UNITS_RD_PRE_THISBUF    (5)
#define VTRANS_SR                               (6)

        /* MOD_REVECHO */
#define DELAY_DELAY                             (0)
#define DELAY_MIX                               (1)
#define DELAY_FEEDBACK                  (2)
#define DELAY_LFOMOD                    (3)
#define DELAY_LFOFRQ                    (4)
#define DELAY_LFOPHASE                  (5)
#define DELAY_LFODELAY                  (6)
#define DELAY_TRAILTIME                 (7)
#define DELAY_PRESCALE                  (8)
#define DELAY_SEED                              (9)     /* int */
                /* flags only */
#define DELAY_INVERT_FLAGNAME   (10)
                /* internal */
#define DELAY_INVERT                    (10)
#define DELAY_MODRANGE                  (11)
#define DELAY_LFOFLAG                   (12) /* int */
#define DELAY_MODLEN                    (13)
#define DELAY_MDELAY                    (14) /* int */
                /* stadium */
#define STAD_PREGAIN                    (0)
#define STAD_ROLLOFF                    (1)
#define STAD_SIZE                               (2)
#define STAD_ECHOCNT                    (3) /* int */
                /* internal */
#define STAD_MAXDELAY                   (4)
#define STAD_OVFLWSZ                    (5)

/* MOD_RADICAL */
        /* reverse */
        /* internal */
#define REV_BUFCNT                              (0) /* int */
/*#define REV_RBYTES                            (1)*/   /* int */                         /*RWD*/
#define REV_RSAMPS                              (1)
#define REV_SECOFFSET                   (2) /* int */
#define REV_WRAPBYTES                   (3)        /* int */                      /*RWD NOT USED NOW*/

        /* SHRED */
#define SHRED_CNT                               (0)  /* int */
#define SHRED_CHLEN                             (1)
#define SHRED_SCAT                              (2)
        /* internal */
#define SHRED_SPLEN                             (3)  /* int */
#define SHRED_HSPLEN                    (4)  /* int */
#define SHRED_SPLENPOW2                 (5)  /* int */
#define SHR_LAST_BUFLEN                 (6)  /* int */
#define SHR_LAST_CHCNT                  (7)  /* int */
#define SHR_LAST_SCAT                   (8)  /* int */
#define SHR_UNITLEN                             (9)  /* int */
#define SHR_RAWLEN                              (10) /* int */
#define SHR_ENDRAWLEN                   (11) /* int */
#define SHR_SCATGRPCNT                  (12) /* int */
#define SHR_RANGE                               (13) /* int */
#define SHR_CHCNT_LESS_ONE              (14) /* int */
#define SHR_ENDSCAT                             (15) /* int */
#define SHR_ENDRANGE                    (16) /* int */
#define SHR_CHCNT                               (17) /* int */
        /* SCRUB */
#define SCRUB_TOTALDUR                  (0)
#define SCRUB_MINSPEED                  (1)
#define SCRUB_MAXSPEED                  (2)
#define SCRUB_STARTRANGE                (3)
#define SCRUB_ESTART                    (4)
        /* internal */
#define SCRUB_ENDRANGE                  (5)      /* int */
#define SCRUB_SPEEDRANGE                (6)
#define SCRUB_LENGTH                    (7)      /* int */
#define SCRUB_SININTEGRAL               (8)
#define SCRUB_TABINC                    (9)
#define SCRUB_DROPOUT                   (10) /* int */
#define SCRUB_BUFCNT                    (11) /* int */
        /* LOBIT */
#define LOBIT_BRES                              (0)
#define LOBIT_TSCAN                             (1)
        /* internal */
#define LOBIT_HF_BRES                   (2)
#define LOBIT_BRES_SHIFT                (3)
#define LOBIT_HF_TSCAN                  (4)
#define LOBIT_TSCAN_SHIFT               (5)
        /* RINGMOD */
#define RM_FRQ                                  (0)

        /* EDIT */
#define CUT_CUT                                 (0)
#define CUT_END                                 (1)
#define CUT_SPLEN                               (2)
#define INSERT_LEVEL                    (3)

        /* internal */
#define END_SPLICE_ADDR                 (4)  /* int */  /* join ONLY */
#define CUT_BYTECUT                             (4)  /* int */
#define CUT_BYTEEND                             (5)      /* int */
/* #define CUT_BYTELEN                          (6) */ /* int */
#define CUT_LEN                         (6)                                              /*RWD*/
#define CUT_HLFSPLEN                    (7)  /* int */
#define CUT_BYTESPLEN                   (8)  /* int */
#define CUT_BUFXS                               (9)  /* int */
#define CUT_BYTEBUFXS                   (10) /* int */
#define CUT_BUFOFFSET                   (11) /* int */
#define CUT_BYTEBUFOFFSET               (12) /* int */
#define CUT_BUFREMNANT                  (13) /* int */
#define CUT_BUFCNT                              (14) /* int */
#define CUT_BUFCNT2                             (15) /* int */
#define CUT_SECCNT                              (16) /* int */
#define CUT_SECCNT2                             (17) /* int */
#define CUT_BYTEREMAIN                  (18) /* int */
#define CUT_SECSREMAIN                  (19) /* int */
#define CUT_SECSREMAIN2                 (20) /* int */
#define CUT_SMPSREMAIN                  (21) /* int */
#define CUT_GOES_TO_END                 (22) /* int */
#define CUT_NO_END                              (23) /* int */
#define CUT_NO_STT                              (24) /* int */
#define EXCISE_CNT                              (25) /* int */
#define SMALLBUFSIZ                             (26) /* int */

/* RANDCUTS */

#define RC_CHLEN                (0)
#define RC_SCAT                 (1)
#define RC_CHCNT                (2)     /* internal */
#define RC_SCATGRPCNT   (3)     /* internal */
#define RC_ENDSCAT              (4) /* internal */
#define RC_RANGE                (5) /* internal */
#define RC_ENDRANGE             (6) /* internal */
#define RC_UNITLEN              (7) /* internal */

        /* RANDCHUNKS ONLY */
#define CHUNKCNT                                (0)
#define MINCHUNK                                (1)
#define MAXCHUNK                                (2)

/* HOUSE_COPY */
#define COPY_CNT                                (0)  /* int */

/* HOUSE_CHANS */
#define CHAN_NO                                 (0)  /* int */

/* HOUSE_SORT */
#define SORT_SMALL                              (0)
#define SORT_LARGE                              (1)
#define SORT_STEP                               (2)
/* internal */
#define SORT_LENCNT                             (3)     /* int */

/* HOUSE_SPEC */
#define HSPEC_SRATE                             (0)     /* int */
#define HSPEC_CHANS                             (1)     /* int */
#define HSPEC_TYPE                              (2)     /* int */
/* internal */
#define RSMP_OBUFSIZE                   (3)     /* int */
#define RSMP_OBUFLEN                    (4)     /* int */
#define RSMP_HISR                               (5)     /* int */
#define RSMP_QIK                                (6)     /* int */

/* HOUSE_EXTRACT */
#define RECTIFY_SHIFT                   (0)     /* int */
#define TOPN_GATE                               (0)     /* int */
#define TOPN_SPLEN                              (1)
        /* internal */
#define TOPN_NGATE                              (2)     /* int */
#define LAST_TOTALSAMPS_READ    (3)     /* int */

#define CUTGATE_GATE            (0)     /* int */
#define CUTGATE_SPLEN           (1)     /* int */
#define CUTGATE_ENDGATE         (2)     /* int */
#define CUTGATE_THRESH          (3)     /* int */
#define CUTGATE_SUSTAIN         (4)     /* int */
#define CUTGATE_BAKTRAK         (5)     /* int */
#define CUTGATE_INITLEVEL       (6)     /* int */
#define CUTGATE_MINLEN          (7)
        /* internal or not */
#define CUTGATE_WINDOWS         (8)     /* int */
        /* internal */
#define CUTGATE_NUMSECS         (9)      /* int */
#define CUTGATE_NSEC            (10) /* int */
#define CUTGATE_NBUFS           (11) /* int */
//#define CUTGATE_BYTEBLOK      (12) /* int */
#define CUTGATE_SAMPBLOK        (12) /* int */
#define CUTGATE_SHRTBLOK        (13) /* int */
#define CUTGATE_TYPFACT         (14) /* int */
#define CUTGATE_SPLCNT          (15) /* int */

//TW : REDUNDANT ITEMS REMOVED

/* HOUSE RECOVER */
#define DUMP_OK_CNT                     (0)
#define DUMP_OK_PROP            (1)
#define DUMP_OK_SAME            (2)
#define SFREC_SHIFT                     (3)
//TW : REDUNDANT ITEM REMOVED
/* internal recover */
//TW : RENUMBERED PLUS SOME REMOVED
#define SFD_OUTFILESIZE         (4)     /* i */
#define HEADER_CNT                      (5)     /* i */
#define OK_HEADER_CNT           (6) /* i */
#define TRUE_HEADER_CNT         (7) /* i */
#define HEADER_VARIANT_CNT      (8) /* i */
#define CRC_AT                          (9) /* i */
#define OK_CORRESPONDENCE       (10) /* i */
#define UNKNOWN_CNT                     (11) /* i */
#define RENAME_CNT                      (12) /* i */


/* SNDINFO */
#define TIMESUM_SPLEN           (0)
#define INFO_SAMPS                      (0)     /* int */
#define INFO_TIME                       (0)
#define HOLE_THRESH                     (0)     /* int */
#define SFDIFF_THRESH           (0)
#define SFDIFF_CNT                      (1) /* int */
#define PRNT_START                      (0)
#define PRNT_END                        (1)

/* MUSUNITS */
#define MUSUNIT                         (0)

/* SYNTH */
#define SYN_SRATE                       (0)
#define SYN_CHANS                       (1)
#define SYN_DUR                         (2)
#define SYN_FRQ                         (3)
#define SYN_AMP                         (4)
#define SYN_TABSIZE                     (5)

/* SYNTH_SPEC */
#define SS_DUR                          (0)
#define SS_CENTRFRQ                     (1)
#define SS_SPREAD                       (2)
#define SS_FOCUS                        (3)
#define SS_FOCUS2                       (4)
#define SS_TRAND                        (5)
#define SS_SRATE                        (6)

/* UTILS */

#define U0                                      (0)
#define U1                                      (1)

/* SINTAB */

#define SIN_FRQ                         (0)
#define SIN_AMP                         (1)
#define SIN_DUR                         (2)
#define SIN_QUANT                       (3)
#define SIN_PHASE                       (4)

/* ACC_STREAM */

#define ACC_ATTEN                       (0)

/* INFO_MAXSAMP2 */

#define MAX_STIME                       (0)
#define MAX_ETIME                       (1)

/* HF_PERM1 */

#define HP1_SRATE                       (0)
#define HP1_ELEMENT_SIZE        (1)
#define HP1_GAP_SIZE            (2)
#define HP1_GGAP_SIZE           (3)
#define HP1_MINSET                      (4)
#define HP1_BOTNOTE                     (5)
#define HP1_BOTOCT                      (6)
#define HP1_TOPNOTE                     (7)
#define HP1_TOPOCT                      (8)
#define HP1_SORT1                       (9)
        /* internal */
#define HP1_HFCNT                       (10)

/* DEL_PERM */
/* DEL_PERM2 */

#define DP_SRATE        (0)
#define DP_DUR          (1)
#define DP_CYCCNT       (2)
        /* internal */
#define DP_NOTECNT      (3)
#define DP_PERMCNT      (4)

/* TWIXT */

#define IS_SPLEN        (0)
#define IS_SEGCNT       (1)
#define IS_WEIGHT       (2)
        /* internal */
#define IS_SPLICETIME   (3)
#define IS_SHSECSIZE    (4)

//TW NEW -->>

/* P_VOWELS */
#define PV_HWIDTH       (0)
#define PV_CURVIT       (1)
#define PV_PKRANG       (2)
#define PV_FUNBAS       (3)
#define PV_OFFSET       (4)
/* VFILT */
#define VF_THRESH       (3)
/* HOUSE_GATE */
#define GATE_ZEROS      (0)
/* P_GEN */
#define PGEN_SRATE      (0)
#define PGEN_CHANS_INPUT (1)
#define PGEN_WINOVLP_INPUT (2)
/* CUT_MANY */
#define CM_SPLICELEN (0)
/* internal */
//TW REVISED FOR FLTS
//#define CM_SPLICEBYTES (1)
#define CM_SPLICE_TOTSAMPS (1)
#define CM_SPLICESAMPS (2)
#define CM_SPLICEINCR  (3)
/* STACK */
#define STACK_CNT               (0)
#define STACK_LEAN              (1)
#define STACK_OFFSET    (2)
#define STACK_GAIN              (3)
#define STACK_DUR               (4)
/* internal */
#define STACK_MINTRANS  (5)
/* DISTORT_PULSED */
#define PULSE_STARTTIME (0)     /* starttime of impulses in input-sound */
#define PULSE_DUR               (1)     /* duration of impulse stream */
#define PULSE_FRQ               (2)     /* frq of impulses */
#define PULSE_FRQRAND   (3)     /* frq of impulses */
#define PULSE_TIMERAND  (4)     /* randomisation of pulse shape, timewise */
#define PULSE_SHAPERAND (5)     /* randomisation of pulse shape, ampwise  */
#define PULSE_WAVETIME  (6)     /* duration of wavcycle-group to cycle round, within impulse [synth option only] */
#define PULSE_TRANSPOS  (7)     /* transposition envelope of material inside impulse */
#define PULSE_PITCHRAND (8)     /* randomisation of transposition envelope */
/* internal */
#define PULSE_ENVSIZE   (9)     /* no of paired vals in envelope of impulse */
#define PULSE_TRNSIZE   (10) /* no of paired vals in transposition envelope of impulse */
/* noise suppress */
#define NOISE_SPLEN     (0)
#define NOISE_MINFRQ (1)
#define MIN_NOISLEN     (2)
#define MIN_TONELEN     (3)
/* time grid */
#define GRID_COUNT      (0)
#define GRID_WIDTH      (1)
#define GRID_SPLEN      (2)
/* convolve */
#define CONV_TRANS      (0)
/* baktobak */
#define BTOB_CUT        (0)
#define BTOB_SPLEN      (1)
/* sequencer */
#define SEQ_ATTEN       (0)
#define SEQ_SPLIC       (1)
/* SHUDDER */
#define SHUD_STARTTIME  (0)
#define SHUD_FRQ                (1)
#define SHUD_SCAT               (2)
#define SHUD_SPREAD             (3)
#define SHUD_MINDEPTH   (4)
#define SHUD_MAXDEPTH   (5)
#define SHUD_MINWIDTH   (6)
#define SHUD_MAXWIDTH   (7)
/* MIX AT STEP */
#define MIX_STEP                (0)
/* CLICKS */
#define CLIKSTART (0)
#define CLIKEND   (1)
#define CLIKOFSET (2)
#define CLICKTIME (3)

/* DOUBLETS */
#define SEG_DUR (0)
#define SEG_REPETS (1)
/* internal */
#define SEGLEN (2)
/* SYLLABS */
#define SYLLAB_SPLICELEN        (0)     /* NB must be same paramno as edit_cutmany CM_SPLICELEN */
#define SYLLAB_DOVETAIL (1)
/* BATCH_EXPAND */
#define BE_INFILE       (0)
#define BE_OUTFILE      (1)
#define BE_PARAM        (2)
/* JOIN_SEQ */
#define MAX_LEN (3)
/* ENVSYN */
#define ENVSYN_WSIZE            (0)
#define ENVSYN_DUR                      (1)
#define ENVSYN_CYCLEN           (2)
#define ENVSYN_STARTPHASE       (3)
#define ENVSYN_TROF                     (4)
#define ENVSYN_EXPON            (5)
/* INEXTEND */
#define INEXTEND_STT            (0)
#define INEXTEND_END            (1)
#define INEXTEND_REP            (2)
#define INEXTEND_SPL            (3)
/* RRRR_EXTEND */
#define RRR_START       (0)
#define RRR_GATE        (0)
#define RRR_END         (1)
#define RRR_GRSIZ       (1)
#define RRR_STRETCH     (2)
#define RRR_GET     (3)
#define RRR_RANGE       (4)
#define RRR_REPET       (5)
#define RRR_ASCAT       (6)
#define RRR_PSCAT       (7)
#define RRR_SKIP        (8)
#define RRR_AFTER       (9)
#define RRR_TEMPO       (10)
#define RRR_AT          (11)
/* internal */
#define RRR_WSIZE       (12)
#define RRR_SAMP_WSIZE  (13)
/* VERSION 8+ */
#define RRR_SLOW    (9)
#define RRR_REGU    (10)
/* internal */
#define RRR_WSIZENU    (11)
#define RRR_SAMP_WSIZENU    (12)
/* HOUSE_GATE2 */
#define GATE2_DUR       (0)
#define GATE2_ZEROS (1)
#define GATE2_LEVEL (2)
#define GATE2_SPLEN (3)
#define GATE2_FILT      (4)
/* ZCROSS_RATIO */
#define ZC_START        (0)
#define ZC_END          (1)
/* SSSS_EXTEND */
#define SSS_DUR                 (0)
#define  MAX_NOISLEN    (3)
#define  SSS_GATE        (4)
/* GREV */
#define GREV_WSIZE              (0)
#define GREV_TROFRAC    (1)
#define GREV_GPCNT              (2)
#define GREV_TSTR               (3)
#define GREV_REPETS             (3)
#define GREV_KEEP               (3)
#define GREV_DEL                (3)
#define GREV_OUTOF              (4)
/* internal */
#define GREV_SAMP_WSIZE (5)
