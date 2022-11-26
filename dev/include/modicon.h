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



/******************************** MODIFY ******************************/

/*
#define MULTICHAN 1
*/
/* mod_space constants */
#define PAN_PRESCALE_DEFAULT    (0.7)
#define MAX_PANNING                             (256.0)

/* brassage internal flags */
#define G_VELOCITY_FLAG         (0)
#define G_DENSITY_FLAG          (1)
#define G_GRAINSIZE_FLAG        (2)
#define G_PITCH_FLAG            (3)
#define G_AMP_FLAG                      (4)
#define G_SPACE_FLAG            (5)
#define G_BSPLICE_FLAG          (6)
#define G_ESPLICE_FLAG          (7)
#define G_RANGE_FLAG            (8)
#define G_SCATTER_FLAG          (9)
/* brassage bufnames */
#define GRS_SBUF        (0)
#define GRS_BUF         (1)
#define GRS_IBUF        (2)
#define GRS_OBUF        (3)
/* brassage extrabuf_names */
#define GRS_GBUF        (0)
/* brassage bitflags */
#define G_VARIABLE_HI_BOUND     (8)
#define G_HI_BOUND                      (4)
#define G_VARIABLE_VAL          (2)
#define G_FIXED_VAL                     (1)
/* brassage */
#define GRS_MIN_GRSAMPSIZE      (4)             /* smaalest grainsize in (stereo-pair=1) samps */
#define GRS_MAX_VFLAGS          (3)
#define GRS_DEFAULT_DENSITY                      (2.0)           /* used to be 2.0 */
#define GRS_MAX_DENSITY                          (16384.0)
#define GRS_MAX_VELOCITY                         (32767.0)
#define GRS_DEFAULT_GRAINSIZE            (50.0)
#define GRS_DEFAULT_REVERB_PSHIFT        (0.33)
#define MIN_LIKELY_PITCH                         (10.0)
#define GRS_DEFAULT_REVERB_SRCHRANGE (50.0)     /* ??? */
#define GRS_DEFAULT_SPLICELEN            (5.0)  /* ??? */
#define GRS_MIN_SPLICELEN                        (1.0)  /* ??? */
#define GRS_DEFAULT_SCATTER                      (0.5)  /* ??? */
#define GRS_DEFAULT_REVERB_DENSITY       (8.0)  /* ??? */
/* counts of granula flags */
#define SFLAGCNT  (8)   /* structured flags */
#define FLAGCNT   (2)   /* other flags */
/* settings of granula flags */
/*
#define NOT_SET         (0) DEFINED GLOBALLY */
#define FIXED           (1)
#define VARIABLE        (2)
#define RANGED          (5)
#define RANGE_VLO       (6)
#define RANGE_VHI       (9)
#define RANGE_VHILO (10)
/* granula */
// multichannel 2009
#define BOTH_CHANNELS (-1)
#define ALL_CHANNELS  (-1)

/* strans,vtrans */
#define INBUF           (0)
#define OUTBUF          (1)
#define OBUFEND         (2)
#define TRUE_LBUF       (3)
#define L_GRDPNT        (4)
#define TRUE_RBUF       (5)
#define R_GRDPNT        (6)
#define LBUF            (7)
#define RBUF            (8)

#define MIN_ACCEL               (.0001)
#define MAX_ACCEL               (1000.0)
#define MINTIME_ACCEL   (0.01)

#define VIB_TABSIZE             (1024)
#define VIB_BLOKSIZE            (128)
#define DEFAULT_VIB_FRQ         (12.0)
#define DEFAULT_VIB_DEPTH       (0.667) /* 2/3 semitones = 1/3 tone */
#define MAX_VIB_FRQ                     (120.0)

#define DEFAULT_DELAY           (20.0)
#define MAX_DELAY                       (10000.0)

/*TW NEW*/

#define MAX_ECHOCNT             (1000)  /* used to be 23, then 100 */
#define REASONABLE_ECHOCNT      (23)

#define STAD_PREGAIN_DFLT       (0.645654)
#define DFLT_STAD_DELTIME       (0.1)
#define MAX_STAD_DELAY          (100.0)

        /* lobit */
#define MIN_SRATE_DIV           (2)
#define MAX_SRATE_DIV           (256)
#define MAX_BIT_DIV             (16)
        /* shred */
#define SHRED_SPLICELEN         (256)
#define SHRED_HSPLICELEN                (128)
#define SHRED_SPLICELEN_POW2    (8)
#define MAXSHRED                                (10000)
#define MAX_SHR_SCATTER                 (8)
        /* scrub */
#define SCRUB_SINTABSIZE        (256)
#define SCRUB_SINWARP           (.3)
#define SCRUB_SKEWRANGE         (.01)

#define SCRUB_MINSPEED_DEFAULT  (-48.0) /* 4 octaves down */
#define SCRUB_MAXSPEED_DEFAULT  (48.0)  /* 4 octaves up */
#define SCRUB_SPEED_MIN                 (-96.0) /* 8 octaves down */
#define SCRUB_SPEED_MAX                 (96.0)  /* 8 octaves up */
#define SCRUB_RANGE_REDUCE_FACTOR       (.15)

#define MIN_RING_MOD_FRQ                (0.1)
#define RING_MOD_BLOK                   (128)
#define RM_SINTABSIZE                   (512)
