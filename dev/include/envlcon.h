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



/*************************** ENVELOPE *************************/

#define ENV_DEFAULT_WSIZE       (50.0)  /* MSECS */

#define MIN_ENV_EXAG            (0.01)
#define MAX_ENV_EXAG            (100.0)
#define MIN_ENV_ATTEN           (0.0)
#define MAX_ENV_ATTEN           (1.0)
#define MIN_ENV_LIMIT           (0.0)
#define MAX_ENV_LIMIT           (ABSMAXSAMP)
#define MIN_ENV_THRESHOLD   (0.0)
#define MAX_ENV_THRESHOLD   (ABSMAXSAMP)
#define MIN_ENV_TSTRETCH        (.001)
#define MAX_ENV_TSTRETCH        (1000.0)
#define MAX_ENV_FLATN           (5000.0)
#define MAX_ENV_SMOOTH           (32767.0)


#define ENV_DEFAULT_EXAGG    (2.0)
#define ENV_DEFAULT_ATTEN    (0.5)
#define ENV_DEFAULT_LIFT     (0.0)
#define ENV_DEFAULT_TSTRETCH (2.0)
#define ENV_DEFAULT_FLATN        (4.0)
#define ENV_DEFAULT_GATE         (0.3)
#define ENV_DEFAULT_THRESH       (0.15)
#define ENV_DEFAULT_MIRROR       (0.3)
#define ENV_DEFAULT_TRIGDUR  (ENV_DEFAULT_WSIZE * 4.0)
#define ENV_DEFAULT_TRIGRISE (0.15)
#define ENV_DEFAULT_TROFDEL     (2.0)
#define ENV_DEFAULT_PKSRCHWIDTH (4.0)

#define ENV_DEFAULT_DATAREDUCE  (0.0002)

#define ENV_MIN_ATK_ONSET               (5.0)
#define ENV_MAX_ATK_ONSET               (32767.0)
#define ENV_DEFAULT_ATK_ONSET   (50.0)
#define ENV_ATK_SRCH                    (200.0)
#define ENV_DEFAULT_ATK_TAIL    (200.0)
#define ENV_DEFAULT_ATK_GAIN    (2.0)

#define ENV_TIMETYPE_SECS       (0)
#define ENV_TIMETYPE_SMPS       (1)
#define ENV_TIMETYPE_STSMPS     (2)

#define ENVTYPE_LIN             (0)
#define ENVTYPE_EXP             (1)
#define ENVTYPE_STEEP   (2)
#define ENVTYPE_DBL             (3)

#define ENV_PLK_FRQ_MIN                 (20.0)
#define ENV_PLK_FRQ_MAX                 (10000.0)
#define ENV_PLK_CYCLEN_MIN          (2.0)
#define ENV_PLK_CYCLEN_MAX      ((double)MAXSHORT)
#define ENV_PLK_CYCLEN_DEFAULT  (32.0)
#define ENV_PLK_DECAY_DEFAULT   (48.0)
#define ENV_PLK_DECAY_MIN               (1.0)
#define ENV_PLK_DECAY_MAX               (64.0)
#define ENV_PLK_ENDSAMP_MAXTIME (10.0)
#define ENV_PLK_ONSET_TIME      (4.0)      /* MS */

#define PLK_INBUF    (0)
#define PLK_OUTBUF   (1)
#define PLK_OBUFWRAP (2)
#define PLK_BUFEND   (3)
#define PLK_INITBUF  (4)
#define PLK_PLUKEND  (5)

#define ENV_TREM_TABSIZE                (4096)
#define ENV_TREM_MAXFRQ                 (500.0)
#define ENV_TREM_DEFAULT_FRQ    (15.0)
#define ENV_TREM_DEFAULT_DEPTH  (0.25)

#define ENV_MIN_WSIZE                   (5.0)           /* MSECS */
#define ENV_MAX_WSIZE                   (10000.0)       /* MSECS */

#define MAX_PEAK_SEPARATION             (32767)         /* Arbitrary */
