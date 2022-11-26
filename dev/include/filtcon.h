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



/***************************** FILTERS ******************************/

#define BSIZE                   (128)
#define MINQ                    (0.001)
#define MAXQ                    (10000.0)
#define MIN_ACUITY              (1.0/MAXQ)
#define MAX_ACUITY              (1.0)
#define FLT_MINGAIN             (0.001)
#define FLT_MAXGAIN             (10000.0)
#define MINMIDI                 (0.0)
#define MAXMIDI                 (127.0)
//RWD was a whopping 10Hz!
#define FLT_MINFRQ              (0.1)
#define FLT_MAXFRQ              ((double)srate/2.0)

#define FLT_MAX_FILTERS (2000)

#define MINFILTAMP              (0.001) /* Filter can't have zero gain (causes divide by zero) */
                                                                /* This is near enough to zero to allow the maths to work */
                                                                /* without the filter component biting */

#define FLT_MAX_PSHIFT  (48.0)  /* 4 8vas in semitones */

/* FLITITER */

#define ITER_MONO               (1)
#define ITER_STEREO             (2)
#define MN_SHIFT                (3)
#define ST_SHIFT                (4)
#define ST_FLT_INTP_SHIFT       (5)
#define MN_FLT_INTP_SHIFT       (6)
#define FIXED_AMP       (10) /* Converts flags above to flags below */
#define U_MONO                  (11)
#define U_STEREO        (12)
#define U_MN_SHIFT              (13)
#define U_ST_SHIFT              (14)
#define U_ST_INTP_SHIFT (15)
#define U_MN_INTP_SHIFT (16)

#define MIN_SHORT               (-32768)
#define MAX_SHORT               (32767)
#define FLT_SAFETY      (512)

#define FLT_OUTBUF      (2)
#define FLT_OVFLWBUF    (3)

/* FLTSWEEP */

#define FLT_MINSWEEP    (0.0001)
#define FLT_MAXSWEEP    (200.0)

/* FSTATVAR */

#define FLT_DEFAULT_FRQ         (440.0)
#define FLT_DEFAULT_LOFRQ       (100.0)
#define FLT_DEFAULT_HIFRQ       (4000.0)
#define FLT_DEFAULT_SWPFRQ      (0.5)
#define FLT_MININT                      (.25)           /* 1/4 of a semitone: min step between filters */
#define FLT_MAXINT                      (8.0 * SEMITONES_PER_OCTAVE)

#define FLT_MINPRESCALE         (-1.0)
#define FLT_MAXPRESCALE         (1.0)

#define FLT_MAXEQPRESCALE       (200.0)
#define FLT_MINEQPRESCALE       (1.0/FLT_MAXEQPRESCALE)

#define FLT_MAXHARMS            (200.0)

#define FLT_DEFAULT_BW                   (200.0)
#define FLT_DEFAULT_Q                    (20.0)
#define FLT_DEFAULT_OFFSET               (40.0)
#define FLT_DEFAULT_INCOUNT              (8.0)
#define FLT_DEFAULT_INTSIZE              (5.0)
#define FLT_DEFAULT_HCNT                 (1.0)
#define FLT_DEFAULT_ROLLOFF              (-6.0)
#define FLT_DEFAULT_ITERDELAY    (1.0)
#define FLT_DEFAULT_ALLPASSDELAY (10.0)  /* MS */
#define FLT_DEFAULT_SWPPHASE     (0.25)

#define FLT_MINDBGAIN                    (MIN_DB_ON_16_BIT)
#define FLT_MAXDBGAIN                    (36.0) /* arbitrary!! */

#define FLT_DEFAULT_LOHIPASS     (1000.0)
#define FLT_DEFAULT_LOHISTOP     (600.0)

#define FLT_DEFAULT_LOHIPASSMIDI (84.0)
#define FLT_DEFAULT_LOHISTOPMIDI (72.0)

#define FLT_DEFAULT_LOHI_ATTEN   (-60.0)

#define FLT_LBF                                         (200)
#define FLT_DEFAULT_LPHP_PRESCALE       (0.9)

#define FLT_MAXDELAY                    (32767.0)

#define FLT_SPLICELEN                           (15.0)

#define FLT_TAIL                                        (20.0)  /* 1 second allowed for filter to die to zero */
