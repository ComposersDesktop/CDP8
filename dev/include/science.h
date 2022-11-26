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



/* CONSTANTS FOR SCIENCE PROGS */

/* PROCESS NUMBERS */

#ifndef SPEKTRUM
#define SPEKTRUM        384
#endif
#ifndef SPEKVARY
#define SPEKVARY        385
#endif
#ifndef SPEKFRMT
#define SPEKFRMT        386
#endif
#ifndef TS_OSCIL
#define TS_OSCIL        387
#endif
#ifndef TS_TRACE
#define TS_TRACE        388
#endif
#ifndef SPECAV
#define SPECAV          389
#endif
#ifndef SPECANAL
#define SPECANAL        390
#endif
/* PARAMETERS */

/* MAX_PROCESS_NO should be defiend as the maximum process OVERALL */
#ifdef MAX_PROCESS_NO
#undef MAX_PROCESS_NO
#define MAX_PROCESS_NO PARTITION
#else
#define MAX_PROCESS_NO PARTITION
#endif


#define SPEKPOINTS      0
#define SPEKSRATE       1
#define SPEKDUR         2
#define SPEKHARMS       3
#define SPEKBRITE       4
#define SPEKRANDF       5
#define SPEKRANDA       6
#define SPEKSPRED       7
#define SPEKGAIN        8
#define SPEKTYPE        9
#define SPEKWIDTH       10
#define SPEKMXASP       11
#define SPEKZOOM        12

#define SPEKDATLO       (5)
#define SPEKDATHI       (6)
#define SPEKSPKLO       (7)
#define SPEKSPKHI       (8)
#define SPEKMAX         (9)
#define SPEKWARP        (10)
#define SPEKAWARP       (11)

/* PARAMETERS */

#define TS_TSTRETCH      0
#define TS_OMAXDUR       1
#define TS_FRQ           1
#define TS_HALFRANGE 2
#define TS_TMAXDUR       3

/* ARRAYS */

#define TS_DATA          0
#define TS_HARMONICS 1
#define TS_SINETAB   2

/* CONSTANTS */

#define TS_SINTABSIZE 4096
#define TS_MAXLEVEL     0.9
#define TS_MAXTSTRETCH 10000
#define TS_MAXOCT 16
#define TS_MAXRANGE             48      /*      Max halfrange of pitch variation of time-series from mean freq: in semitones */
#define TS_DFLTRANGE    12      /*      Default halfrange of ditto */
#define TS_MINFRQ       16.0

#define SPEKSR          44100
#define SPEKFADE        12              /* Number of windows over which spectrum fades in and out */


/*      INPUT FILE LOGIC */

#define NUMLIST_ONLY    (34)

/* SPECIAL DATA */

#define TS_HARM         94

/* SPECANAL */

#define PA_DEFAULT_PVOC_CHANS   (1024)
#define PA_VERY_BIG_INT                 (100000000)
#define PA_MAX_PVOC_CHANS               (16380)
#define PA_PVOC_CONSTANT_A              (8.0)

#define FILTR_DUR       (0)
#define FILTR_CNT       (1)
#define FILTR_MMIN      (2)
#define FILTR_MMAX      (3)
#define FILTR_DIS       (4)
#define FILTR_RND       (5)
#define FILTR_AMIN      (6)
#define FILTR_ARND      (7)
#define FILTR_ADIS      (8)
#define FILTR_STEP      (9)
#define FILTR_SRND      (10)
#define FILTR_SEED      (11)

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

#define PLS_DUR          0
#define PLS_PITCH        1
#define PLS_TRANSP       1
#define PLS_MINRISE      2
#define PLS_MAXRISE      3
#define PLS_MINSUS       4
#define PLS_MAXSUS   5
#define PLS_MINDECAY 6
#define PLS_MAXDECAY 7
#define PLS_SPEED        8
#define PLS_SCAT         9
#define PLS_EXP          10
#define PLS_EXP2         11
#define PLS_PSCAT        12
#define PLS_ASCAT        13
#define PLS_OCT          14
#define PLS_BEND         15
#define PLS_SEED         16
#define PLS_SRATE        17
#define PLS_CNT          18

/* CHIRIKOV */

#define CHIR_DUR        0
#define CHIR_FRQ        1
#define CHIR_DAMP       2
#define CHIR_SRATE      3
#define CHIR_SPLEN      4
#define CHIR_PMIN       3
#define CHIR_PMAX       4
#define CHIR_STEP       5
#define CHIR_RAND       6

/* MULTIOSC */

#define MOSC_DUR        0
#define MOSC_FRQ1       1
#define MOSC_FRQ2       2
#define MOSC_AMP2       3
#define MOSC_FRQ3       4
#define MOSC_AMP3       5
#define MOSC_FRQ4       6
#define MOSC_AMP4       7
#define MOSC_SRATE      8
#define MOSC_SPLEN      9

/* SYNFILT */

#define SYNFLT_SRATE    0
#define SYNFLT_CHANS    1
#define SYNFLT_Q                2
#define SYNFLT_HARMCNT  3
#define SYNFLT_ROLLOFF  4
#define SYNFLT_SEED             5

/* STRANDS */
#define STRAND_DUR              0       //      OUTPUT DURATION
#define STRAND_BANDS    1       //      NUMBER OF BANDS
#define STRAND_THRDS    2       //      NUMBER OF THREADS PER BAND
#define STRAND_TSTEP    3       //      TIME-STEP BETWEEN OUTPUT DATA POINTS
#define STRAND_BOT              4       //      LOWEST PITCH OF ALL BANDS
#define STRAND_TOP              5       //      HIGHEST PITCH OF ALL BANDS
#define STRAND_TWIST    6       //      SPEED OF OSCILLATION OF THREAD-PITCHES
#define STRAND_RAND             7       //      RANDOMISATION SPEED BETWEEN BANDS
#define STRAND_SCAT             8       //      SCATTER OF SPEED AMONGST THREADS
#define STRAND_VAMP             9       //      WAVY VORTEX SIZE
#define STRAND_VMIN             10      //      WAVY VORTEX MINIMUM SPEED
#define STRAND_VMAX             11      //      WAVY VORTEX MAXIMUM SPEED
#define STRAND_TURB             12      //      TURBULENCE SETTING
#define STRAND_SEED             13      //      SEED FOR RANDOM PROCESSES
#define STRAND_GAP              14      //      MINIMUM PITCH INTERVAL BETWEEN BANDS
#define STRAND_MINB             15      //      MINIMUM PITCH-WIDTH OF BANDS
#define STRAND_3D               16      //      DEPTH-MOTION TYPE

/* REFOCUS */
#define REFOC_DUR               0
#define REFOC_BANDS             1
#define REFOC_RATIO             2
#define REFOC_TSTEP             3
#define REFOC_RAND              4
#define REFOC_OFFSET    5
#define REFOC_END               6
#define REFOC_XCPT              7
#define REFOC_SEED              8

/* UNKNOT */
#define KNOT_PATREP             0
#define KNOT_COMBOREP   1
#define KNOT_ALLREP             2
#define KNOT_UNKNOTREP  3
#define KNOT_GOALREP    4
#define KNOT_SPACETYP   5
#define KNOT_CHANA              6
#define KNOT_CHANB              7
#define KNOT_MIN                8
#define KNOT_CLIP               9

/* RHYMORPH : RHYMORPH2 */
#define RHYM_PATREP             0
#define RHYM_MORPHREP   1
#define RHYM_GOALREP    2
#define RHYM_STEPS              3
#define RHYM_RESPACE    4
