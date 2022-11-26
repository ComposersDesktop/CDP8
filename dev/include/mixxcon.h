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



/*********************************** MIX ***********************************/

#define MIX_ACTION_OFF          (0)
#define MIX_ACTION_ON           (1)
#define MIX_ACTION_REFILL       (2)

/*
defined elsewhere....
#define MONO                             (1)
#define STEREO                           (2)
*/
#define MONO_TO_LEFT             (3)
#define MONO_TO_RIGHT            (4)
#define MONO_TO_CENTRE           (5)
#define MONO_TO_STEREO           (6)
#define STEREO_MIRROR            (7)
#define STEREO_TO_CENTRE         (8)
#define STEREO_TO_LEFT           (9)
#define STEREO_TO_RIGHT          (10)
#define STEREO_PANNED            (11)
#define STEREOLEFT_TO_LEFT       (12)
#define STEREOLEFT_TO_RIGHT      (13)
#define STEREOLEFT_PANNED        (14)
#define STEREORIGHT_TO_LEFT      (15)
#define STEREORIGHT_TO_RIGHT (16)
#define STEREORIGHT_PANNED       (17)

#define MIX_MAXLINE             (7)
#define MIX_MIDLINE             (5)
#define MIX_MINLINE             (4)
#define MIX_NAMEPOS             (0)
#define MIX_TIMEPOS             (1)
#define MIX_CHANPOS             (2)
#define MIX_LEVELPOS    (3)
#define MIX_PANPOS              (4)
#define MIX_RLEVELPOS   (5)
#define MIX_RPANPOS             (6)

#define POSITIONS               (32)
#define HALF_POSTNS     (16)

#define DEFAULT_DECIMAL_REPRESENTATION  (5)
#define MIN_WINFAC              (1)
#define MAX_WINFAC              (32)

#define MAX_MCR_POWFAC  (8.0)
#define MIN_MCR_POWFAC  (1.0/MAX_MCR_POWFAC)

#define MCR_LINEAR              (0)
#define MCR_COSIN               (1)
#define MCR_SKEWED              (2)

#define MCR_TABLEN              (512)

#define MAX_MI_OUTCHANS (1000)
#define INT_TO_ASCII    (48)

#define DEFAULT_BETWEEN (8)
#define MAXBETWEEN      (999)

#define MINPAN                  (-32767.0)
#define MAXPAN                  (32767.0)
#define OBUFMIX                 (0)
#define STEREOBUF               (1)
#define IBUFMIX                 (2)

#define PAN_LEFT                (-1.0)
#define PAN_RIGHT               (1.0)
#define PAN_CENTRE              (0.0)

#define SAMPLE_RATE_DIVIDE      (28000) /* divides possible samprates int hi_sr and lo_sr */
#define DEFAULT_BTWNFRQ (4000.0)
