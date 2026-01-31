/*
 * Copyright (c) 1983-2020 Trevor Wishart and Composers Desktop Project Ltd
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
/* CONSTANTS FOR SCIENCE PROGS */

/* PROCESS NUMBERS */

#ifndef SPEKTRUM
#define SPEKTRUM    384
#endif
#ifndef SPEKVARY    
#define SPEKVARY    385
#endif
#ifndef SPEKFRMT    
#define SPEKFRMT    386
#endif
#ifndef TS_OSCIL    
#define TS_OSCIL    387
#endif
#ifndef TS_TRACE    
#define TS_TRACE    388
#endif
#ifndef SPECAV      
#define SPECAV      389
#endif
#ifndef SPECANAL    
#define SPECANAL    390
#endif
/* PARAMETERS */

/* MAX_PROCESS_NO should be defiend as the maximum process OVERALL */
#ifdef MAX_PROCESS_NO
#undef MAX_PROCESS_NO
#define MAX_PROCESS_NO TWEET
#else
#define MAX_PROCESS_NO TWEET
#endif


#define SPEKPOINTS  0
#define SPEKSRATE   1
#define SPEKDUR     2
#define SPEKHARMS   3
#define SPEKBRITE   4
#define SPEKRANDF   5   
#define SPEKRANDA   6
#define SPEKSPRED   7
#define SPEKGAIN    8
#define SPEKTYPE    9
#define SPEKWIDTH   10
#define SPEKMXASP   11
#define SPEKZOOM    12

#define SPEKDATLO   (5)
#define SPEKDATHI   (6)
#define SPEKSPKLO   (7)
#define SPEKSPKHI   (8)
#define SPEKMAX     (9)
#define SPEKWARP    (10)
#define SPEKAWARP   (11)

/* PARAMETERS */

#define TS_TSTRETCH  0
#define TS_OMAXDUR   1
#define TS_FRQ       1
#define TS_HALFRANGE 2
#define TS_TMAXDUR   3

/* ARRAYS */

#define TS_DATA      0
#define TS_HARMONICS 1
#define TS_SINETAB   2

/* CONSTANTS */

#define TS_SINTABSIZE 4096
#define TS_MAXLEVEL 0.9
#define TS_MAXTSTRETCH 10000
#define TS_MAXOCT 16
#define TS_MAXRANGE     48  /*  Max halfrange of pitch variation of time-series from mean freq: in semitones */
#define TS_DFLTRANGE    12  /*  Default halfrange of ditto */
#define TS_MINFRQ   16.0

#define SPEKSR      44100
#define SPEKFADE    12      /* Number of windows over which spectrum fades in and out */


/*  INPUT FILE LOGIC */

#define NUMLIST_ONLY    (34)    

/* SPECIAL DATA */
    
#define TS_HARM     94

/* SPECANAL */

#define PA_DEFAULT_PVOC_CHANS   (1024)
#define PA_VERY_BIG_INT         (100000000)
#define PA_MAX_PVOC_CHANS       (32768)    //RWD 2025 was 16380
#define PA_PVOC_CONSTANT_A      (8.0)

#define FILTR_DUR   (0)
#define FILTR_CNT   (1)
#define FILTR_MMIN  (2)
#define FILTR_MMAX  (3)
#define FILTR_DIS   (4)
#define FILTR_RND   (5)
#define FILTR_AMIN  (6)
#define FILTR_ARND  (7)
#define FILTR_ADIS  (8)
#define FILTR_STEP  (9)
#define FILTR_SRND  (10)
#define FILTR_SEED  (11)

/* ITERFOF */

#define ITF_DEL  0
#define ITF_DUR  1
#define ITF_PRND 2
#define ITF_AMPC 3
#define ITF_TRIM 4
#define ITF_TRBY 5
#define ITF_SLOP 6
#define ITF_RAND 7
#define ITF_VMIN 8
#define ITF_VMAX 9
#define ITF_DMIN 10
#define ITF_DMAX 11
#define ITF_SEED1 12
#define ITF_GMIN 12
#define ITF_GMAX 13
#define ITF_UFAD 14
#define ITF_FADE 15
#define ITF_GAPP 16
#define ITF_PORT 17
#define ITF_PINT 18
#define ITF_SEED2 19

/* PULSER */

#define PLS_DUR      0
#define PLS_PITCH    1
#define PLS_TRANSP   1
#define PLS_MINRISE  2
#define PLS_MAXRISE  3
#define PLS_MINSUS   4
#define PLS_MAXSUS   5
#define PLS_MINDECAY 6
#define PLS_MAXDECAY 7
#define PLS_SPEED    8
#define PLS_SCAT     9
#define PLS_EXP      10
#define PLS_EXP2     11
#define PLS_PSCAT    12
#define PLS_ASCAT    13
#define PLS_OCT      14
#define PLS_BEND     15
#define PLS_SEED     16
#define PLS_SRATE    17
#define PLS_CNT      18
#define PLS_WIDTH    17

/* CHIRIKOV */

#define CHIR_DUR    0
#define CHIR_FRQ    1
#define CHIR_DAMP   2
#define CHIR_SRATE  3
#define CHIR_SPLEN  4
#define CHIR_PMIN   3
#define CHIR_PMAX   4
#define CHIR_STEP   5
#define CHIR_RAND   6

/* MULTIOSC */

#define MOSC_DUR    0
#define MOSC_FRQ1   1
#define MOSC_FRQ2   2
#define MOSC_AMP2   3
#define MOSC_FRQ3   4
#define MOSC_AMP3   5
#define MOSC_FRQ4   6
#define MOSC_AMP4   7
#define MOSC_SRATE  8
#define MOSC_SPLEN  9

/* SYNFILT */

#define SYNFLT_SRATE    0
#define SYNFLT_CHANS    1
#define SYNFLT_Q        2
#define SYNFLT_HARMCNT  3
#define SYNFLT_ROLLOFF  4
#define SYNFLT_SEED     5

/* STRANDS */
#define STRAND_DUR      0   //  OUTPUT DURATION
#define STRAND_BANDS    1   //  NUMBER OF BANDS
#define STRAND_THRDS    2   //  NUMBER OF THREADS PER BAND
#define STRAND_TSTEP    3   //  TIME-STEP BETWEEN OUTPUT DATA POINTS
#define STRAND_BOT      4   //  LOWEST PITCH OF ALL BANDS
#define STRAND_TOP      5   //  HIGHEST PITCH OF ALL BANDS
#define STRAND_TWIST    6   //  SPEED OF OSCILLATION OF THREAD-PITCHES
#define STRAND_RAND     7   //  RANDOMISATION SPEED BETWEEN BANDS
#define STRAND_SCAT     8   //  SCATTER OF SPEED AMONGST THREADS
#define STRAND_VAMP     9   //  WAVY VORTEX SIZE
#define STRAND_VMIN     10  //  WAVY VORTEX MINIMUM SPEED
#define STRAND_VMAX     11  //  WAVY VORTEX MAXIMUM SPEED
#define STRAND_TURB     12  //  TURBULENCE SETTING
#define STRAND_SEED     13  //  SEED FOR RANDOM PROCESSES
#define STRAND_GAP      14  //  MINIMUM PITCH INTERVAL BETWEEN BANDS
#define STRAND_MINB     15  //  MINIMUM PITCH-WIDTH OF BANDS
#define STRAND_3D       16  //  DEPTH-MOTION TYPE

/* REFOCUS */
#define REFOC_DUR       0
#define REFOC_BANDS     1
#define REFOC_RATIO     2
#define REFOC_TSTEP     3
#define REFOC_RAND      4
#define REFOC_OFFSET    5
#define REFOC_END       6
#define REFOC_XCPT      7
#define REFOC_SEED      8

/* UNKNOT */
#define KNOT_PATREP     0
#define KNOT_COMBOREP   1
#define KNOT_ALLREP     2
#define KNOT_UNKNOTREP  3
#define KNOT_GOALREP    4
#define KNOT_SPACETYP   5
#define KNOT_CHANA      6
#define KNOT_CHANB      7
#define KNOT_MIN        8
#define KNOT_CLIP       9

/* RHYMORPH : RHYMORPH2 */
#define RHYM_PATREP     0
#define RHYM_MORPHREP   1
#define RHYM_GOALREP    2
#define RHYM_STEPS      3
#define RHYM_RESPACE    4

/* ROTOR */
#define ROT_CNT     0
#define ROT_PMIN    1
#define ROT_PMAX    2
#define ROT_NSTEP   3
#define ROT_PCYC    4
#define ROT_TCYC    5
#define ROT_PHAS    6
#define ROT_DUR     7
#define ROT_GSTEP   8
#define ROT_DOVE    9

/* BROWNIAN */
#define BRCHANS 0
#define BRDUR   1
#define BRATT   2
#define BRDEC   3
#define BRPLO   4
#define BRPHI   5
#define BRPSTT  6
#define BRSSTT  7
#define BRPSTEP 8
#define BRSSTEP 9
#define BRTICK  10
#define BRSEED  11
#define BRASTEP 12
#define BRAMIN  13
#define BRASLP  14
#define BRDSLP  15

/* SPIN */
#define SPNRATE  0
#define SPNOCHNS 1
#define SPNOCNTR 2
#define SPNDOPL  3
#define SPNXBUF  4
#define SPNBOOST 5
#define SPNATTEN 6
#define SPNCMIN  7
#define SPNCMAX  8

/* CRUMBLE */
#define CRSTART  0
#define CRSTEP1  1
#define CRSTEP2  2
#define CRSTEP3  3
#define CRORIENT 4
#define CRSIZE   5
#define CRRAND   6
#define CRISCAT  7
#define CROSCAT  8
#define CROSTR   9
#define CRPSCAT  10
#define CRSEED   11
#define CRSPLICE 12
#define CRTAIL   13
#define CRDUR    14

/* tesselate */
#define TESS_CHANS  0
#define TESS_PHRAS  1
#define TESS_DUR    2
#define TESS_TYP    3
#define TESS_FROM   4

/* crystal */
#define CRY_ROTA    (0)
#define CRY_ROTB    (1)
#define CRY_TWIDTH  (2)
#define CRY_TSTEP   (3)
#define CRY_DUR     (4)
#define CRY_PLO     (5)
#define CRY_PHI     (6)
#define CRY_FPASS   (7)
#define CRY_FSTOP   (8)
#define CRY_FATT    (9)
#define CRY_FPRESC  (10)
#define CRY_FSLOPE  (11)    //  warps curve of mix-balance of normal & filtered version of sound (depth cue)
#define CRY_SSLOPE  (12)    //  warps curve of mix-balance of transposed & original version of sound (proximity cue)
// internal
#define CRY_CNT     (13)
#define CRY_MUL     (14)

//LIMIT OR DEFAULT VALS

#define CRY_ROT_MIN     (-10.0)
#define CRY_ROT_MAX     (10.0)
#define CRY_TSTEP_MIN   (0.01)
#define CRY_TSTEP_MAX   (3600)
#define CRY_DUR_MAX     (7200)
#define CRY_LOP_MIN     (0)
#define CRY_LOP_MAX     (127)
#define CRY_TW_MIN      (0.01)
#define CRY_TW_MAX      (3600)
#define CRY_FATT_DFLT   (-60)
#define CRY_FPRESC_DFLT (0.9)
#define CRY_PASSBAND    (400)
#define CRY_STOPBAND    (800)
#define MIN_FSLOPE      (0.1)
#define MAX_FSLOPE      (10.0)
#define MIN_SSLOPE      (0.1)
#define MAX_SSLOPE      (10.0)

#define CRYS_DEPTH_ATTEN    (0.7)   //  Makes transit from normal to filtered follow convex arc (so filt increases quickly-slowly as depth increases)
#define CRYS_PROX_ATTEN     (0.7)   //  Makes transit from normal to stacked follow convex arc (stacking increases quickly-slowly as proximity increases)

// WAVEFORM
#define WF_TIME (0)
#define WF_CNT  (1)
#define WF_DUR  (1)
#define WF_BAL  (2)

//CASCADE
#define CAS_CLIP     (0)
#define CAS_ECHO     (1)
#define CAS_MAXCLIP  (2)
#define CAS_MAXECHO  (3)
#define CAS_RAND     (4)
#define CAS_SEED     (5)
#define CAS_SHREDNO  (6)
#define CAS_SHREDCNT (7)

#define CAS_SHREDSRC (0)
#define CAS_LINEAR   (1)
#define CAS_UPNORMAL (2)

#define SPLIN_SRATE (0)
#define SPLIN_DUR   (1)
#define SPLIN_FRQ   (2)
#define SPLIN_CNT   (3)
#define SPLIN_INTP  (4)
#define SPLIN_SEED  (5)
#define SPLIN_MCNT  (6)
#define SPLIN_MINTP (7)
#define SPLIN_DRIFT (8)
#define SPLIN_DRVEL (9)
//  FRACTAL
#define FRAC_MAXDUR     7200.0      //  secs = 2 hours
#define FRAC_MAXFRACT   1000        //  max possible fractalisation
#define FRAC_MINTRNS    -12.0       //  semitones
#define FRAC_MAXTRNS    12.0
#define FRAC_MINMIDI    0.0         //  MIDI pitch
#define FRAC_MAXMIDI    127.0
//  SPLINTER
#define SPL_TIME    (0)
#define SPL_WCNT    (1)
#define SPL_SHRCNT  (2)
#define SPL_OCNT    (3)
#define SPL_PULS1   (4)
#define SPL_PULS2   (5)
#define SPL_ECNT    (6)
#define SPL_SCURVE  (7)
#define SPL_PCURVE  (8)
#define SPL_FRQ     (9)
#define SPL_DUR     (9)
#define SPL_RND     (10)
#define SPL_SHRND   (11)

#define SHRCNT_DFLT 8           //  Default count of selected-unit repeats  over which shrinkage takes place
#define OCNT_DFLT   8           //  Default count of splinters (mode 1) prior to unshrinking/ (mode 2) post shrinking
#define PULS_DFLT   10          //  Default pulse speed of splinters
#define FREQ_DFLT   6000.0      //  Typical frq  of splinters (really wavelen = srate/6000 = c.8 samples at 44100)

//  REPEATER
#define REP_ACCEL   0
#define REP_WARP    1
#define REP_FADE    2
#define REP_RAND    3
#define REP_TRNSP   4
#define REP_SEED    5
#define REPSPLEN    5.0         //  length of splices in mS

//  VERGES
#define VRG_TRNSP   0
#define VRG_CURVE   1
#define VRG_DUR     2

#define VRG_DFLT_DUR    100.0   //  Millisecond duration of verges
#define VRG_DFLT_CURVE  1.0     //  Gliss curvature
#define VRG_DFLT_TRNSP  5       //  Default transposition   

//  MOTOR
#define MOT_DUR     0
#define MOT_FRQ     1
#define MOT_PULSE   2
#define MOT_FRATIO  3
#define MOT_PRATIO  4
#define MOT_SYM     5
#define MOT_FRND    6
#define MOT_PRND    7
#define MOT_JIT     8
#define MOT_TREM    9
#define MOT_SYMRND  10
#define MOT_EDGE    11
#define MOT_BITE    12
#define MOT_VARY    13
#define MOT_SEED    14

#define MOT_FXDSTP  0
#define MOT_CYCLIC  1

#define MOT_FRQ_DFLT    20.0
#define MOT_PULSE_DFLT  0.5
#define MOT_SPLICE      3.0     //  default mS splicelen for inner events
#define MOT_EXPDECAY    4.0     //  default exponential decay of sub-events with decay-tails
#define MOT_DOVE        5.0     //  mS dovetailing of separated syllabs

//  STUTTER
#define STUT_DUR    0
#define STUT_JOIN   1
#define STUT_SIL    2
#define STUT_SILMIN 3
#define STUT_SILMAX 4
#define STUT_SEED   5
#define STUT_TRANS  6
#define STUT_ATTEN  7
#define STUT_BIAS   8
#define STUT_MINDUR 9

#define STUT_MIN    50.0    //  Minimum size of any cut-element
#define STUT_SPLICE 3.0     //  default mS splicelen for segments
#define STUT_DOVE   5.0     //  mS dovetailing of separated segments
#define STUT_SILDIV 20      //  Accuracy of porportion of-silence
                            //  20 means, accurate to 1 20th, so val in range 0 to 1 is approximated to nearest 20th
//  flags
#define STUT_PERM   0
#define STUT_MAX_JOIN 8

//  SCRAMBLE
#define SCR_DUR   0
#define SCR_SEED  1
#define SCR_CNT   2
#define SCR_TRNS  3
#define SCR_ATTEN 4

//  IMPULSE
#define IMP_DUR     0
#define IMP_PICH    1
#define IMP_CHIRP   2
#define IMP_SLOPE   3
#define IMP_CYCS    4
#define IMP_LEV     5
#define IMP_GAP     6
#define IMP_SRATE   7
#define IMP_CHANS   8

//  TWEET
#define TWT_PDAT    0
#define TWT_MIN     1
#define TWT_PKCNT   2
#define TWT_CHIRP   3

//  SPECFNU
#define MIN_SYLLAB_DUR   (0.08)
#define MIN_PEAKTROF_GAP (0.08)
#define SPEC_MIDIMIN        (4)     //  c 10Hz, lower pitches difficult to repesent in spectrum
#define RANDPITCHMAX        (96)    //  Range of possible random variation of pitch (semitones)
#ifndef MIDIMAX
#define MIDIMAX             (127)
#endif
#define MAXFILTVALS       12        //  Max number of filter pitches output by F_MAKEFILT

// Modes
#define F_NARROW    0   //  == user 1
#define F_SQUEEZE   1   //  == user 2
#define F_INVERT    2   //  == user 3
#define F_ROTATE    3   //  == user 4
#define F_NEGATE    4   //  == user 5
#define F_SUPPRESS  5   //  == user 6
#define F_MAKEFILT  6   //  == user 7
#define F_MOVE      7   //  == user 8
#define F_MOVE2     8   //  == user 9
#define F_ARPEG     9   //  == user 10
#define F_OCTSHIFT  10  //  == user 11  is_coloring
#define F_TRANS     11  //  == user 12  is_coloring
#define F_FRQSHIFT  12  //  == user 13  is_coloring
#define F_RESPACE   13  //  == user 14  is_coloring
#define F_PINVERT   14  //  == user 15  is_coloring
#define F_PEXAGG    15  //  == user 16  is_coloring
#define F_PQUANT    16  //  == user 17  is_coloring
#define F_PCHRAND   17  //  == user 18  is_coloring
#define F_RAND      18  //  == user 19  is_coloring
#define F_SEE       19  //  == user 20
#define F_SEEPKS    20  //  == user 21
#define F_SYLABTROF 21  //  == user 22
#define F_SINUS     22  //  == user 23

//  PARAM NAMES

//  param names NARROW, SQUEEZE, SUPPRESS, INVERT, ROTATE, NEGATE
//  param names RECOLOR = F_TRANS,F_FRQSHIFT,F_RESPACE,F_RAND
#define FGAIN       2
//  param names F_NARROW
#define NARROWING   0
#define NARSUPRES   3
//  param names F_SQUEEZE
#define SQZFACT     0
#define SQZAT       1
//  param names F_INVERT
#define FVIB        0
//  param names F_ROTATE
#define RSPEED      0
//  param names F_SUPPRESS
#define SUPRF       0
//  param names F_MAKEFILT
#define FPKCNT      0
#define FBELOW      2
//  param names F_MOVE & F_MOVE2
#define FMOVE1      0
#define FMOVE2      1
#define FMOVE3      2
#define FMOVE4      3
#define FMVGAIN     4
//  param names F_SYLABTROF
#define FMINSYL     0
#define FMINPKG     1
//  param names F_ARPEG
#define FARPRATE    0
//  param names RECOLOR = F_TRANS,F_FRQSHIFT,F_RESPACE,F_RAND
#define COL_LO      3
#define COL_HI      4
#define COLRATE     5
#define COLLOPCH    6
#define COLHIPCH    7
//  param names RECOLOR = F_OCTSHIFT
#define COLINT      0
//  param names RECOLOR = F_TRANS, F_FRQSHIFT, F_RESPACE, F_RAND, F_PSHIFT
#define COLFLT      0
//  param names RECOLOR = F_PEXAGG
#define EXAGRANG    1
//  param names RECOLOR = F_PCHRAND
#define FPRMAXINT   0
#define FSLEW       1
//  param names F_SINUS
#define F_SINING    0
#define F_AMP1      3
#define F_AMP2      4
#define F_AMP3      5
#define F_AMP4      6
#define F_QDEP1     7
#define F_QDEP2     8
#define F_QDEP3     9
#define F_QDEP4     10

// FLAGNAMES

//  flagnames NARROW
#define ZEROTOP     0
#define NRW_FUND    1
#define NRW_SW      2
#define NRW_XNH     3
#define NRW_KHM     4
#define NRW_EXI     5
//  flagnames SQUEEZE
#define AT_TROFS    0
#define SQZ_FUND    1
#define SQZ_SW      2
#define SQZ_XNH     3
#define SQZ_KHM     4
#define SQZ_EXI     5
//  flagnames NEGATE
#define FLATT       0
#define NEG_SW      1
#define NEG_XNH     2
#define NEG_KHM     3
#define NEG_EXI     4
//  flagnames MAKEFILT
#define KEEPAMP     0
#define KEEPINV     1
#define FLT_FUND    2
#define FLT_SW      3
//  flagnames MOVE
#define MOV_ZEROTOP 0
#define MOV_SW      1
#define MOV_XNH     2
#define MOV_KHM     3
#define MOV_EXI     4
/// flagnames MOVE2
#define MOV_ZEROTOP 0
#define MOV_SW      1
#define MOV2_NRW    2
#define MOV2_XNH    3
#define MOV2_KHM    4
#define MOV2_EXI    5
//  flagnames INVERT
#define INVERT_SW   0
#define INVERT_XNH  1   //  Exclude non harmonics
#define INVERT_KHM  2   //  Suppress all harmonics
#define INVERT_EXI  3   //  Smooth_away unpitched-data
//  flagnames ROTATE
#define ROTATE_SW   0
#define ROTATE_XNH  1
#define ROTATE_KHM  2
#define ROTATE_EXI  3
//  flagnames SUPPRESS
#define SUPPRESS_SW 0
#define SUPPRESS_XNH 1
//  flagnames SEE
#define SEE_SW      0
//  flagnames SEEPKS    
#define SEEPKS_SW   0
//  flagnames RECOLOR
#define RECOLOR_SW  0
#define RECOLOR_XNH 1
#define RECOLOR_EXI 2
#define RECOLOR_DWN 3
#define RECOLOR_CYC 4
#define RECOLOR_FIL 5
//  F_PQUANTISE + F_PCHRAND
#define Q_ORNAMENTS 5
#define Q_NOSMOOTH  6
//  F_PCHRAND
#define NO_RESHAPE  7
//  F_PEXAGG
#define EXAG_HITIE  5
#define EXAG_LOTIE  6
#define EXAG_MIDTIE 7
#define ONLY_ABOVE  8
#define ONLY_BELOW  9
//  F_SYLABTROF
#define PKS_ONLY    0
#define PKS_TROFS   1
//  F_SINUS
#define FSIN_SW     0
#define FSIN_FUND   1
#define FSIN_EXI    2
#define F_SMOOTH    3
//  BOUNCE
#define BNC_NUMBER  0
#define BNC_STTSTEP 1
#define BNC_SHORTEN 2
#define BNC_ENDLEV  3
#define BNC_LEVWRP  4
#define BNC_MINDUR  5
