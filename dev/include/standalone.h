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
/* These are new programs which don't require libs to be recompiled
   but don't act differently to standard CDP programs */

#define SPECTOVF2   89
#define MTON        90
#define FLUTTER     91
#define SETHARES    92
#define MCHSHRED    93
#define MCHZIG      94
#define MCHSTEREO   95

#define MIXMULTI    256
#define ANALJOIN    257
#define PTOBRK      258
#define PSOW_STRETCH    259
#define PSOW_DUPL       260
#define PSOW_DEL        261
#define PSOW_STRFILL    262

#define ONEFORM_GET     263
#define ONEFORM_PUT     264
#define ONEFORM_COMBINE 265
#define PSOW_FREEZE     266
#define PSOW_CHOP       267
#define PSOW_INTERP     268
#define NEWGATE         269
#define PSOW_FEATURES   270
#define PSOW_SYNTH      271
#define PSOW_IMPOSE     272
#define PSOW_SPLIT      273
#define PSOW_SPACE      274
#define PSOW_INTERLEAVE 275
#define PSOW_REPLACE    276
#define PSOW_EXTEND     277
#define PSOW_LOCATE     278
#define PSOW_CUT        279
#define SPEC_REMOVE     280
#define PSOW_EXTEND2    281
#define PREFIXSIL       282
#define STRANS          283
#define PSOW_REINF      284
#define PARTIALS_HARM   285
#define SPECROSS        286
#define MCHITER         287

/* These are new programs which behave differently to standard CDP progs */
/* These numbers follow on consecutively from values processno.h */

#define TAPDELAY    353
#define RMRESP      354
#define RMVERB      355

/* simil: these are multichannel toolkit programs */

#define ABFPAN      900
#define ABFPAN2     901
#define CHANNELX    902
#define CHORDER     903
#define CHXFORMAT   904
#define COPYSFX     905
#define FMDCODE     906
#define INTERLX     907
#define NJOIN       908
#define NMIX        909
#define RMSINFO     910
#define SFEXPROPS   911
#define CHXFORMATM  912
#define CHXFORMATG  913
#define NJOINCH     914
#define ABFPAN2P    915

/* These are new programs which behave exactly as standard CDP progs */
/* These numbers follow on consecutively from values processno.h */

#define LUCIER_GETF 356
#define LUCIER_GET  357
#define LUCIER_PUT  358
#define LUCIER_DEL  359
#define SPECLEAN    360
#define SPECTRACT   361
#define PHASE       362
#define FEATURES    363
#define BRKTOPI     364
#define SPECSLICE   365
#define FOFEX_EX    366
#define FOFEX_CO    367
#define GREV_EXTEND 368
#define PEAKFIND    369
#define CONSTRICT   370
#define EXPDECAY    371 
#define PEAKCHOP    372 
#define MCHANPAN    373 
#define TEX_MCHAN   374 
#define MANYSIL     375
#define RETIME      376
#define SPECTOVF    377
#define HOVER       378
#define MULTIMIX    379
#define FRAME       380
#define SEARCH      381
#define MCHANREV    382
#define WRAPPAGE    383
// Intervening numbers are in science.h 

#define SPEKTRUM    384
#define SPEKVARY    385
#define SPEKFRMT    386
#define TS_OSCIL    387
#define TS_TRACE    388
#define SPECAV      389
#define SPECANAL    390
#define SPECSPHINX  391
#define SUPERACCU   392
#define PARTITION   393
#define SPECGRIDS   394
#define GLISTEN     395
#define TUNEVARY    396
#define ISOLATE     397
#define REJOIN      398
#define PANORAMA    399
#define TREMOLO     400
#define ECHO        401
#define PACKET      402
#define SYNTHESIZER 403
#define TAN_ONE     404
#define TAN_TWO     405
#define TAN_SEQ     406
#define TAN_LIST    407
#define SPECTWIN    408

#define TRANSIT     409
#define TRANSITF    410
#define TRANSITD    411
#define TRANSITFD   412
#define TRANSITS    413
#define TRANSITL    414
#define CANTOR      415
#define SHRINK      416
#define NEWTEX      417
#define CERACU      418
#define MADRID      419
#define SHIFTER     420
#define FRACTURE    421
#define SUBTRACT    422
#define SPEKLINE    423
#define SPECMORPH   424
#define SPECMORPH2  425
#define NEWDELAY    426
#define FILTRAGE    427
#define ITERLINE    428
#define ITERLINEF   429
#define SPECVU      430
#define SPECRAND    431
#define SPECSQZ     432
#define HOVER2      433
#define SELFSIM     434
#define ITERFOF     435
#define PULSER      436
#define PULSER2     437
#define PULSER3     438
#define CHIRIKOV    439
#define MULTIOSC    440
#define SYNFILT     441
#define STRANDS     442
#define REFOCUS     443
#define UNKNOT      444
#define RHYMORPH    445
#define RHYMORPH2   446
#define CHANPHASE   447
#define SILEND      448
#define SPECULATE   449
#define SPECTUNE    450
#define REPAIR      451
#define DISTSHIFT   452
#define QUIRK       453
#define ROTOR       454
#define DISTCUT     455
#define ENVCUT      456
#define DISTWARP    457
#define SPECFOLD    458
#define BROWNIAN    459
#define SPIN        460
#define SPINQ       461
#define CRUMBLE     462
#define TESSELATE   463
#define MULTISYN    464
#define PHASOR      465
#define CRYSTAL     466
#define WAVEFORM    467
#define DVDWIND     468
#define CASCADE     469
#define SYNSPLINE   470
#define FRACTAL     471
#define FRACSPEC    472
#define SPLINTER    473
#define REPEATER    474
#define VERGES      475
#define MOTOR       476
#define STUTTER     477
#define SCRUNCH     478
#define IMPULSE     479
#define TWEET       480
#define BOUNCE      481
#define SORTER      482
#define SPECFNU     483
#define FLATTEN     484
#define WAVMEDIAN   485
#define DISTORTT    486
#define SPECTSTR    487
#define DISTMARK    488
#define FTURANAL    489
#define FTURSYNTH   490
#define DISTBRIGHT  491
#define DISTDOUBLE  492
#define SEGSBKWD    493
#define SEGSZIG     494
#define SPIKE       495
#define DISTREP     496
#define TOSTEREO    497
#define SUPPRESS    498
#define CALTRAIN    499
#define SPECENV     500
#define PAIREX      501
#define NUCLICK     502
#define CLIP        503
#define SPECEX      504
#define ONSET       505
#define MATRIX      506
#define TRANSPART   507
#define SPECINVNU   508
#define SPECCONV    509
#define SPECSND     510
#define SPECFRAC    511
#define ENVSPEAK    512
#define EXTSPEAK    513
#define ENVSCULPT   514
#define TREMENV     515
#define DCFIX       516
//#define   TOPTAIL     497 //  nOT USEFUL: Tops and tails only where ALL channels fall below gate
// MAX_PROCESS_NO should be defiend as the maximum number above 
#ifdef MAX_PROCESS_NO
#undef MAX_PROCESS_NO
#define MAX_PROCESS_NO DCFIX
#else
#define MAX_PROCESS_NO DCFIX
#endif

#define MIX_MULTI   (ROTOR) // 1110000000000000 filetype

// These numbers follow on consecutively from values in special.h 
#define TAPDELAY_DATA   68
#define TAPDELAY_OPTION 69
#define P_BRK_DATA      70
#define SYNTHBANK       71
#define TIMEVARYING_SYNTHBANK   72
#define PSOW_REINFORCEMENT      73
#define PSOW_INHARMONICS        74
#define FOFEX_EXCLUDES  75
#define FOFBANK_INFO    76
#define MCHANDATA       77
#define MANYSIL_DATA    78
#define RETIME_DATA     79
#define FRAMEDATA       80
#define IDEAL_DATA      81
#define MCHANDATA2      82
#define RETIME_FNAM     83
#define WRAP_FOCUS      84
#define ANTIPHON        85
#define CROSSPAN        86
#define RETEMPO_DATA    87
#define RETIME_MASK     88
#define OCHANDATA       89
#define FLUTTERDATA     90
#define CHANXDATA       91
#define CHORDATA        92
#define MZIGDATA        93
/* #define TS_HARM      94  in science.h */
#define TUNING          95
#define TUNELOW_DATA    96
#define ISOLATES        97
#define ISOGROUPS       98
#define ISOSLICES       99
#define ISOSYLLS        100
#define PANOLSPKRS      101
#define PAK_TIMES       102
#define SYN_PARTIALS    103
#define SHRFOC          104
#define NTEX_TRANPOS    105
#define CYCLECNTS       106
#define MAD_SEQUENCE    107
#define SHFCYCLES       108
#define ENVSERIES       109
#define SPEKLDATA       110
#define MPEAKS          111
#define ITERTRANS       112
#define ITERTRANSF      113
#define SYN_SPEK        114
#define SYN_FILTERBANK  115
#define COUTHREADS      116
#define RHYMORLINK      117
#define TUNINGLIST      118
#define ROTORDAT        119
#define TESSELATION     120
#define SCOREDATA       121
#define CRYSTALDAT      122
#define CASCLIPS        123
#define FRACSHAPE       124
#define REPEATDATA      125
#define VERGEDATA       126
#define MOTORDATA       127
#define SPSYNCDAT       -1
#define FFILT           -2
#define HFIELD          -3
#define HFIELD_OR_ZERO  -4
#define SEGMENTLIST     -5
#define MARKLIST        -6
#define FTURDATA        -7
#define SPIKEDATA       -8
#define SPACEDATA       -9
#define MATRIX_DATA     -10
#define XSPK_PATTERN    -11
#define XSPK_CUTS       -12
#define XSPK_CUTPAT     -13
#define XSPK_CUTARG     -14
#define XSPK_CUPATA     -15
#define RHYTHM          -16
// these numbers follow on consecutively DOWNwards from vals in tkglobals.h 
#define FILE_OR_ZERO    -7
#define FNAM_STRING     -8
// these numbers follow on consecutively from vals in cdparams.h 
#define OPTIONAL_FILE   25

#define SPEED_OF_SOUND (346.5)

/* parameters for PSOW_FREEZE */
#define PS_TIME (1)
#define PS_DUR  (2)
#define PS_SEGS (3)
#define PS_DENS (4)
#define PS_TRNS (5)
#define PS_RAND (6)
#define PS_GAIN (7)
/* parameters for PSOW_INTERP */
#define PS_SDUR (0)
#define PS_IDUR (1)
#define PS_EDUR (2)
#define PS_VIBFRQ    (3)
#define PS_VIBDEPTH  (4)
#define PS_TREMFRQ   (5)
#define PS_TREMDEPTH (6)
/* parameters for PSOW_FEATURES */
#define PSF_SEGS (1)
#define PSF_TRNS (2)
#define PSF_SPEC (5)
#define PSF_RAND (6)
#define PSF_GAIN (7)
#define PSF_SUB (8)
#define PSF_SUBGAIN (9)
#define PSF_FOFSTR (10)
/* parameters for PSOW_IMPOSE */
#define PS_DEPTH     (1)
#define PS_WSIZE     (2)
#define PS_GATE      (3)
#define PS_SCNT      (4)
#define PS_ENTRYCNT  (5)
#define PS_WORDCNT   (6)
#define PS_TIMESLOTS (7)
#define PS_FRQ_INDEX (8)
#define PS_TIMES_CNT (9)
/* parameters for PSOW_SPLIT */
#define PS_SUBNO     (1)
#define PS_UTRNS     (2)
#define PS_ATTEN     (3)
/* parameters for PSOW_SPACE */
#define PS_SEPAR     (2)
#define PS_RELEV     (3)
#define PS_RELV2     (4)
/* parameters for PSOW_INTERLEAVE */
#define PS_GCNT      (2)
#define PS_BIAS      (3)
#define PS_WEIGHT    (5)
/* ARRAY NAMES FOR for PSOW_SYNTH */
#define PS_OSCFRQ   (0)
#define PS_OSCAMP   (1)
#define PS_INFRQ    (2)
#define PS_INAMP    (3)
#define PS_FBRK     (4)
/* parameters for PSOW_EXTEND */
#define PSE_VFRQ    (4)
#define PSE_VDEP    (5)
#define PSE_TRNS    (6)
#define PSE_GAIN    (7)
/* parameters for PSOW_EXTEND2 */
#define PS2_VFRQ    (3)
#define PS2_VDEP    (4)
#define PS2_NUJ     (5)

#define PS_SAMPTIME (0)

/* parameters for PSOW_REINF */
#define ISTR        (1)

/* parameters for SPECROSS */
#define PICH_THRESH     (6)
#define SPCMPLEV        (7)
#define SPECHINT        (8)
    /* internal */
#define PICH_PICH2      (9)
/* flags for SPECROSS */
#define AMP_SCALING     (0)
#define STABLE_PITCH    (1)

/* params for LUCIER GET */
#define MIN_ROOM_DIMENSION  (0)
#define ROLLOFF_INTERVAL    (1)
/* extra param for LUCIER GETF */
#define LUCIER_CUT          (2)
/* params for LUCIER IMPOSE */
#define RESON_CNT           (0)
#define RES_EXTEND_ATTEN    (1)
/* params for LUCIER SUPPRESS */
#define SUPR_COEFF      (0)

#define MAX_OUTCHAN (64)

#define NEW_DEFAULT_NOISEGAIN (1.2)

#define FORM_WIDTH      (0)
#define FORM_ACCURACY   (1)
#define FORM_PKCNT      (2)
#define FORM_GATE       (3)
#define FORM_RATIO      (4)
#define FORM_FWIDTH     (5)

#define FOFEX_FOFSAMPLE_SIZE (32)

/* modes for FOFEX_CO */
#define FOF_SINGLE  (0)
#define FOF_SUM     (1)
#define FOF_LOSUM   (2)
#define FOF_MIDSUM  (3)
#define FOF_HISUM   (4)
#define FOF_LOHI    (5)
#define FOF_TRIPLE  (6)
#define FOF_MEASURE (7)

#define FOF_NORM    (0)
#define FOF_UNWIN   (0)

#define LOCALMAX_WINDOW (10)
#define LOCALMIN_WINDOW_DIV (3)
#define LOCALPEAK_DECIMATE (5)

/* params for PEAKCHOP */
#define PKCH_WSIZE  (0)
#define PKCH_WIDTH  (1)
#define PKCH_SPLICE (2)
#define PKCH_TEMPO  (3)
#define PKCH_GAIN   (4)
#define PKCH_GATE   (5)
#define PKCH_SKEW   (6)
#define PKCH_SCAT   (7)
#define PKCH_NORM   (8)
#define PKCH_REPET  (9)
#define PKCH_MISS   (10)

#define PKCH_RISE   (1)
#define PKCH_DECDUR (2)
#define PKCH_STEEP  (3)
#define PKCH_ZSTT   (4)
#define PKCH_ZEND   (5)
#define PKCH_RATIO  (6)

/* params for TEX_MCHAN */
#define TEXTURE_OUTCHANS    (12)

#define SVF_RANGE   (0)
#define SVF_MATCH   (1)
#define SVF_LOLM    (2)
#define SVF_HILM    (3)

   /********************************************************/
   /* ELEMENTS IN PLAYLIST FOR MAKING MULTICHANNEL TEXTURE */
   /********************************************************/

struct soundoutmchan {
float *inbuf;
unsigned long ibufcnt;
unsigned long st_sstttime;
unsigned long st_sendtime;
unsigned long obufpos;
long st_splicpos;
double ibufpos;
double step;
double lgain;
double rgain;
int    lchan;
int    rchan;
struct soundoutmchan *next;
struct soundoutmchan *last;
};

typedef struct soundoutmchan *sndoutmchptr;

/* Retime */

#define SIL_SPLICELEN   (0)
#define MM  (0)
#define RETIME_WIDTH    (1)
#define RETIME_SPLICE   (2)

#define SILMIN  (0.5)

#define BEAT_AT         (1)
#define BEAT_CNT        (2)
#define BEAT_REPEATS    (3)
#define BEAT_SILMIN     (4)

#define RETIME_SYNCAT   (4)

/* Mchanrev*/

#define REV_OCHANS 4
#define REV_CENTRE 5
#define REV_SPREAD 6

#define STRING_E    (26)


/******* WRAPPAGE PARAMS, FLAGS ETC.******/

#define WRAP_OUTCHANS       (0)
#define WRAP_SPREAD         (1)
#define WRAP_DEPTH          (2)
#define WRAP_VELOCITY       (3)
#define WRAP_HVELOCITY      (4)
#define WRAP_DENSITY        (5)
#define WRAP_HDENSITY       (6)
#define WRAP_GRAINSIZE      (7)
#define WRAP_HGRAINSIZE     (8)
#define WRAP_PITCH          (9)
#define WRAP_HPITCH         (10)
#define WRAP_AMP            (11)
#define WRAP_HAMP           (12)
#define WRAP_BSPLICE        (13)
#define WRAP_HBSPLICE       (14)
#define WRAP_ESPLICE        (15)
#define WRAP_HESPLICE       (16)
#define WRAP_SRCHRANGE      (17)
#define WRAP_SCATTER        (18)
#define WRAP_OUTLEN         (19)
#define WRAP_BUFXX          (20)

#define WRAP_PRANGE         (21)
#define WRAP_SPRANGE        (22)
#define WRAP_VRANGE         (23)
#define WRAP_DRANGE         (24)
#define WRAP_INCHANS        (25)
#define WRAP_BUF_SMPXS      (26)
#define WRAP_BUF_XS         (27)
#define WRAP_GLBUF_SMPXS    (28)
#define WRAP_LBUF_SMPXS     (29)
#define WRAP_LBUF_XS        (30)
#define WRAP_IS_BTAB        (31)
#define WRAP_IS_ETAB        (32)
#define WRAP_CHANNELS       (33)
#define WRAP_ORIG_SMPSIZE   (34)
#define WRAP_SAMPS_IN_INBUF (35)
#define WRAP_ARANGE         (36)
#define WRAP_GRANGE         (37)
#define WRAP_BRANGE         (38)
#define WRAP_ERANGE         (39)
#define WRAP_LONGS_BUFLEN   (40)
#define WRAP_ARRAYSIZE      (41)

#define WRAP_BSPLICETAB     (0)
#define WRAP_ESPLICETAB     (1)
#define WRAP_NORMFACT       (2)
#define WRAP_CENTRE         (3)

#define WRAP_FLAGS          (0)

#define WRAP_LBUF           (0)
#define WRAP_LBUFEND        (1)
#define WRAP_LBUFMID        (2)
#define WRAP_LTAILEND       (3)

#define WRAP_PERM           (1)

/* #define WRAP_BUF  (1) */
/* #define WRAP_IBUF (2) */
/* extrabuf_names */
#define WRAP_GBUF   (0)
/* bitflags */
#define WRAP_VARIABLE_HI_BOUND  (8)
#define WRAP_HI_BOUND           (4)
#define WRAP_VARIABLE_VAL       (2)
#define WRAP_FIXED_VAL          (1)
/* defaults and internal counters */
#define WRAP_MIN_GRSAMPSIZE     (4)     /* smaalest grainsize in (multichan-group=1) samps */
#define WRAP_MAX_VFLAGS         (3)
#define WRAP_DEFAULT_DENSITY    (2.0)
#define WRAP_MAX_DENSITY        (16384.0)
#define WRAP_MAX_VELOCITY       (32767.0)
#define WRAP_DEFAULT_GRAINSIZE        (50.0)
#define WRAP_DEFAULT_REVERB_PSHIFT    (0.33)
#define WRAP_DEFAULT_REVERB_SRCHRANGE (50.0)    /* ??? */
#define WRAP_DEFAULT_SPLICELEN        (5.0)     /* ??? */
#define WRAP_MIN_SPLICELEN            (1.0)     /* ??? */
#define WRAP_DEFAULT_SCATTER          (0.5)     /* ??? */
#define WRAP_DEFAULT_REVERB_DENSITY   (8.0)     /* ??? */

#define WRAP_VELOCITY_FLAG  (0)
#define WRAP_DENSITY_FLAG   (1)
#define WRAP_GRAINSIZE_FLAG (2)
#define WRAP_PITCH_FLAG     (3)
#define WRAP_AMP_FLAG       (4)
#define WRAP_SPACE_FLAG     (5)
#define WRAP_BSPLICE_FLAG   (6)
#define WRAP_ESPLICE_FLAG   (7)
#define WRAP_RANGE_FLAG     (8)
#define WRAP_SCATTER_FLAG   (9)

#define WRAP_MIN_LIKELY_PITCH   (10.0)

#define WRAP_SFLAGCNT  (8)  /* structured flags */
#define WRAP_FLAGCNT   (2)  /* other flags */

#define WRAP_EXPON  (0)
#define WRAP_OTIME  (1)

#define FIXED       (1)
#define VARIABLE    (2)
#define RANGED      (5)
#define RANGE_VLO   (6)
#define RANGE_VHI   (9)
#define RANGE_VHILO (10)
/* granula */
/* multichannel 2009
#define BOTH_CHANNELS (-1)
#define ALL_CHANNELS  (-1)
*/
/* param names for SPECTOVF2 */
#define SVF2_HILM   0
#define SVF2_PKNG   1
#define SVF2_CTOF   2
#define SVF2_WNDR   3
#define SVF2_PSST   4
#define SVF2_TSTP   5
#define SVF2_SGNF   6
#define SVF2_LOLM   7
#define SVF2_WSIZ   8

/*  params for SETHARES */
#define M_WINSIZE   0
#define M_PEAKING   1
#define M_AMPFLOOR  2
#define M_LOFRQ     3
#define M_HIFRQ     4
#define M_INTUNE    5

/* MCHSHRED */
/* PARAMS */
#define MSHR_CNT            (0)  /* int    */
#define MSHR_CHLEN          (1)  /* double */
#define MSHR_SCAT           (2)  /* double */
#define MSHR_OUTCHANS       (3)  /* int    */
/* internal */
#define MSHR_SPLEN          (4)  /* int */
#define MSHR_LAST_BUFLEN    (5)  /* int */
#define MSHR_LAST_CHCNT     (6)  /* int */
#define MSHR_LAST_SCAT      (7)  /* int */
#define MSHR_UNITLEN        (8)  /* int */
#define MSHR_RAWLEN         (9)  /* int */
#define MSHR_ENDRAWLEN      (10) /* int */
#define MSHR_SCATGRPCNT     (11) /* int */
#define MSHR_RANGE          (12) /* int */
#define MSHR_CHCNT_LESS_ONE (13) /* int */
#define MSHR_ENDSCAT        (14) /* int */
#define MSHR_ENDRANGE       (15) /* int */
#define MSHR_CHCNT          (16) /* int */
/* ARRAYS */
#define MSHR_PERM           (0)     /* int array */
#define MSHR_PERMCH         (1)     /* int array */
#define MSHR_CHUNKPTR       (0)     /* long array */
#define MSHR_CHUNKLEN       (1)     /* long array */
#define MSHR_ROTATE         (0)     /* float array */
/* CONSTANTS */
#define MSHR_SPLICELEN      (256)
#define MSHR_MAX_SCATTER    (8)
#define MSHR_MAX            (10000)
 
/*MCHZIG */
#define MZIG_START  (0)
#define MZIG_END    (1)
#define MZIG_DUR    (2)
#define MZIG_MIN    (3)
#define MZIG_OCHANS (4)
#define MZIG_SPLEN  (5)
#define MZIG_MAX    (6)
#define MZIG_RSEED  (7)
/* constants */
#define MZIG_SPLICELEN     (15.0)
#define MZIG_MIN_UNSPLICED (1.0)
#define MZIG_EXPON         (1.0)
#define MMIN_ZIGSPLICE     (1.0)
#define MMAX_ZIGSPLICE     (5000.0)
#define MDEFAULT_LPSTEP    (200.0)

/*MCHITER */
#define MITER_OCHANS    (0)
#define MITER_DUR       (1)
#define MITER_REPEATS   (1)
#define MITER_DELAY     (2)
#define MITER_RANDOM    (3)
#define MITER_PSCAT     (4)
#define MITER_ASCAT     (5)
#define MITER_FADE      (6)
#define MITER_GAIN      (7)
#define MITER_RSEED     (8)
/* internal params */
#define MITER_STEP      (9)
#define MITER_MSAMPDEL  (10)
#define MITER_DO_SCALE  (11)
#define MITER_PROCESS   (12)

/* ISOLATE */
/* params */
#define ISO_THRON   0
#define ISO_THROFF  1
#define ISO_SPL 2
#define ISO_DOV 3
#define ISO_MIN 3
#define ISO_LEN 4
/* modes */
#define ISO_SEGMNT  0
#define ISO_GROUPS  1
#define ISO_THRESH  2
#define ISO_SLICED  3
#define ISO_OVRLAP  4

/* PANORAMA */
/* params */
#define PANO_LCNT 0
#define PANO_LWID 1
#define PANO_SPRD 2
#define PANO_OFST 3
#define PANO_CNFG 4
#define PANO_RAND 5

/* TREMOLO  && TREMENV */
/* params */
#define TREMOLO_FRQ 0
#define TREMOLO_DEP 1
#define TREMOLO_WIN 2
#define TREMOLO_AMP 2
#define TREMOLO_SQZ 3
/* sintable size */
#define TREMOLO_TABSIZE 4096

/* ECHO */
/* params */
#define ECHO_DELAY  0
#define ECHO_ATTEN  1
#define ECHO_DUR    2
#define ECHO_RAND   3
#define ECHO_CUT    4

/* PACKET */
/* params */
#define PAK_DUR 0
#define PAK_SQZ 1
#define PAK_CTR 2

/* SYNTHESIZE */
/* params */
#define SYNTHSRAT (0)
#define SYNTH_DUR (1)
#define SYNTH_FRQ (2)
#define SYNTH_SQZ (3)
#define SYNTH_CTR (4)

#define SYNTH_DAMP  (3)
#define SYNTH_K     (4)
#define SYNTH_B     (5)

#define SYNTH_ATK       (3)
#define SYNTH_EATK      (4)
#define SYNTH_DEC       (5)
#define SYNTH_EDEC      (6)
#define SYNTH_ATOH      (7)
#define SYNTH_GTOW      (8)
#define SYNTH_RAND      (9)
#define SYNTH_FLEVEL    (10)

/* sintable size */
#define SYNTH_TABSIZE   131072
#define SYNTH_CHANS (3)
#define SYNTH_MAX   (4)
#define SYNTH_RATE  (5)
#define SYNTH_RISE  (6)
#define SYNTH_FALL  (7)
#define SYNTH_STDY  (8)
#define SYNTH_SPLEN (9)
#define SYNTH_NUM   (10)
#define SYNTH_EFROM (11)
#define SYNTH_ETIME (12)
#define SYNTH_CTO   (13)
#define SYNTH_CTIME (14)
#define SYNTH_STYPE (15)
#define SYNTH_RSPEED (16)

/* TANGENT */
/* params */
#define TAN_DUR     (0)
#define TAN_STEPS   (1)
#define TAN_MANG    (2)
#define TAN_SKEW    (2)
#define TAN_DEC     (3)
#define TAN_FBAL    (4)
#define TAN_FOCUS   (5)
#define TAN_JITTER  (6)
#define TAN_SLOW    (7)

/* SPECTWIN */
/* params */
#define TWIN_FRQINTP    (0)
#define TWIN_ENVINTP    (1)

/* TRANSIT */
/* params */
#define TRAN_FOCUS  (0) /*  Centre of motion */
#define TRAN_DUR    (1) /*  Duration */
#define TRAN_STEPS  (2) /*  Number of events in stream */
#define TRAN_MAXA   (3) /*  Maxangle, or (mode4) maxdistance */
#define TRAN_DEC    (4) /*  Gain from 1 event to next, away from centre */
#define TRAN_FBAL   (5) /*  Progreesive mix-in of filtered sound, away from centre */
#define TRAN_THRESH (6) /*  Gain at which gain-decrement starts to increase */
#define TRAN_DECLIM (7) /*  Mix level of decrement after it dtarts to increase */
#define TRAN_MINLEV (8) /*  Minimum gain level for event to end */
#define TRAN_MAXDUR (9) /*  Maximum duration to curtail event */
/* modes */
#define GLANCING    (0)
#define EDGEWISE    (1)
#define CROSSING    (2)
#define CLOSE       (3)
#define CENTRAL     (4)
/* arrays */
#define TR_SPKRPAIR (0)
#define TR_SNDFILE  (1)
#define TR_LEVEL    (0)
#define TR_POSITION (1)
#define TR_TIME     (2)
#define TR_FLTMIX   (3)

/* CANTOR */
/* params */
#define CA_HOLEN    (0)
#define CA_HOLEDIG  (1)
#define CA_TRIGLEV  (2)
#define CA_WOBBLES  (2)
#define CA_WOBDEC   (3)
#define CA_SPLEN    (3)
#define CA_MAXDUR   (4)

/* SHRINK */
/* params */
#define SHR_TIME    (0)
#define SHR_INK     (1)
#define SHR_GAP     (2)
#define SHR_WSIZE   (2)
#define SHR_CNTRCT  (3)
#define SHR_DUR     (4)
#define SHR_AFTER   (4)
#define SHR_SPLEN   (5)
#define SHR_SMALL   (6)
#define SHR_MIN     (7)
#define SHR_RAND    (8)
#define SHR_GATE    (9)
#define SHR_LEN     (10)
#define SHR_SKEW    (11)
/* modes */
#define SHRM_START  (0)
#define SHRM_END    (1)
#define SHRM_CENTRE (2)
#define SHRM_TIMED  (3)
#define SHRM_FINDMX (4)
#define SHRM_LISTMX (5)
/* flags */
#define SHRNK_NORM   (0)
#define SHRNK_INVERT (1)
#define SHRNK_EVENLY (2)
#define SHR_SUPPRESS (3)
/* double arrays */
#define LOCAL_PK_VAL  (0)
#define SHR_INPUT_PKS (1)
#define SHR_INVTGAPS  (2)
/* long arrays */
#define SHR_PEAKS     (0)
#define SHR_TROFS     (1)
#define LOCAL_PK_AT   (2)
/* TINY WINDOW, TO FIND MINIMUM */
#define TINY_WSIZE  (8)

/* spacebox */

#define SB_LRRAND       1
#define SB_FBRAND       2
#define SB_ROTATE       3
#define SB_SUPERSPACE   4
#define SB_SUPERSPACE2  5
#define SB_SUPERSPACE3  6
#define SB_SUPERSPACE4  7
#define SB_LR           8
#define SB_FB           9
#define SB_FRAMESWITCH  10
#define SB_TRIROT1      11
#define SB_TRIROT2      12
#define SB_ANTITRIROT1  13
#define SB_ANTITRIROT2  14

/* NEWTEX */
/* params */
#define NTEX_DUR    (0)
#define NTEX_CHANS  (1)
#define NTEX_MAX    (2)
#define NTEX_RATE   (3)
#define NTEX_STYPE  (4)
#define NTEX_DEL    (5)
#define NTEX_LOC    (6)
#define NTEX_AMB    (7)
#define NTEX_GST    (8)
#define NTEX_SPLEN  (9)
#define NTEX_NUM    (10)
#define NTEX_EFROM  (11)
#define NTEX_ETIME  (12)
#define NTEX_CTO    (13)
#define NTEX_CTIME  (14)
#define NTEX_RSPEED (15)
/* constants */
#define NTEX_OBUFSIZE (512 * 512)

/* CERACU */
#define CER_MINDUR  (0)
#define CER_OCHANS  (1)
#define CER_CUTOFF  (2)
#define CER_DELAY   (3)
#define CER_DSTEP   (4)
/* FLAGS */
#define CER_OVERRIDE (0)
#define CER_LINEAR   (1)

/* MADRID */
#define MAD_DUR    (0)
#define MAD_CHANS  (1)
#define MAD_STRMS  (2)
#define MAD_DELF   (3)
#define MAD_STEP   (4)
#define MAD_RAND   (5)
#define MAD_SEED   (6)
/* FLAGS */
#define MAD_GAPS   (0)
#define MAD_LINEAR (1)
#define MAD_INPERM (2)
#define MAD_INRAND (3)

/* SHIFTER */
#define SHF_CYCDUR (0)
#define SHF_OUTDUR (1)
#define SHF_OCHANS (2)
#define SHF_SUBDIV (3)
#define SHF_LINGER (4)
#define SHF_TRNSIT (5)
#define SHF_LBOOST (6)
/* flags */
#define SHF_ZIG (0)
#define SHF_RND (1)
#define SHF_LIN (2)

/* FRACTURE */
#define FRAC_CHANS   (0)
#define FRAC_STRMS   (1)
#define FRAC_PULSE   (2)
#define FRAC_DEPTH   (3)
#define FRAC_STACK   (4)
#define FRAC_CENTRE  (5)
#define FRAC_FRONT   (6)
#define FRAC_MDEPTH  (7)
#define FRAC_ROLLOFF (8)
#define FRAC_INRND   (9)
#define FRAC_OUTRND (10)
#define FRAC_SCAT   (11)
#define FRAC_LEVRND (12)
#define FRAC_ENVRND (13)
#define FRAC_STKRND (14)
#define FRAC_PCHRND (15)
#define FRAC_SEED   (16)
#define FRAC_MIN    (17)
#define FRAC_MAX    (18)
#define FRAC_ATTEN  (19)
#define FRAC_ZPOINT   (20)
#define FRAC_CONTRACT (21)
#define FRAC_FPOINT   (22)
#define FRAC_FFACTOR  (23)
#define FRAC_FFREQ    (24)
#define FRAC_UP       (25)
#define FRAC_DN       (26)
#define FRAC_GAIN     (27)

/* SPECMORPH */
// params
#define NMPH_STAG       (0)
#define NMPH_ASTT       (1)
#define NMPH_AEND       (2)
#define NMPH_AEXP       (3)
#define NMPH_APKS       (4)
#define NMPH_OCNT       (5)
#define NMPH_RAND       (6)
// modes
#define NMPH_LINE   (0)
#define NMPH_COSIN  (1)
#define NMPH_LINEX  (2)
#define NMPH_COSINX (3)
#define NMPH_LINET  (4)
#define NMPH_COSINT (5)
#define NMPH_GETTEX (6)
#define NMPH_LINTEX (7)
#define NMPH_COSTEX (8)

/* SPECTUNE */
#define ST_MATCH    0
#define ST_LOPCH    1
#define ST_HIPCH    2
#define ST_STIME    3
#define ST_ETIME    4
#define ST_INTUN    5
#define ST_WNDWS    6
#define ST_NOISE    7

#define ST_INTUN5   3           //  different param numbering for mode 4
#define ST_WNDWS5   4
#define ST_NOISE5   5

#define ST_CNT      0

#define ST_ACCEPTABLE_MATCH 5

/* DISTCUT */
#define DCUT_CNT    0
#define DCUT_STP    1
#define DCUT_EXP    2
#define DCUT_LIM    3

/* ENVCUT */
#define ECUT_CNT    0
#define ECUT_STP    1
#define ECUT_ATT    2
#define ECUT_EXP    3
#define ECUT_LIM    4

/* PHASOR */
#define PHASOR_STREAMS  0
#define PHASOR_FRQ      1
#define PHASOR_SHIFT    2
#define PHASOR_OCHANS   3
#define PHASOR_OFFSET   4

/* SPECSYNC */
#define IBUF    0   
#define OBUF    1   
#define OEND    2   
#define IHERE   3
#define INEXT   4
#define OHERE   5
#define SYNCBUFSCNT 8

/* SORTER */
#define SORTER_SIZE     0
#define SORTER_SEED     1
#define SORTER_SMOOTH   2
#define SORTER_OMIDI    3
#define SORTER_IMIDI    4
#define SORTER_META     5

/* MODES FOR MATRIX */
#define MATRIX_MAKE     0
#define MATRIX_USE      1
/* PNAMES FOR MATRIX */
#define MATRIX_CHANS    0
#define MATRIX_OVLAP    1
/* PNAMES FOR FRACTAL */
#define FRACDEPTH       0
#define FRACSPLICE      1
/* PNAMES FOR ENVSPEAK */
#define ESPK_WINSZ  0
#define ESPK_SPLEN  1
#define ESPK_OFFST  2
#define ESPK_REPET  3
#define ESPK_GAIN   4
#define ESPK_WHICH  5
/* FLAG NAMES EXTSPEAK */
#define XSPK_TRANSPOSE  0
#define XSPK_ENV        1
#define XSPK_KEEP       2
#define XSPK_ORIGSZ     3
#define XSPK_INJECT     4
#define XSPK_RAND       5
/* PNAMES NAMES EXTSPEAK */
#define XSPK_WINSZ  0
#define XSPK_SPLEN  1
#define XSPK_OFFST  2
#define XSPK_N      3
#define XSPK_GAIN   4
#define XSPK_SEED   5
#define MAX_PATN    100
