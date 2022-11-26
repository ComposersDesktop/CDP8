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



/*********************************** GRAIN ********************************/

#define GRAIN_SPLICELEN (15.0)  /* mS */
#define GRAIN_SAFETY     (1.0)  /* mS */
#define GR_INTERPLIMIT  (1000)  /* stereo-samples */
#define CAPITALISE               (-32)  /* convert lower-case to up[per-case ASCII */
#define ALPHABASE                ('A')  /* lowest ASCII value */
#define GR_MAX_TRANSPOS  (4.0)  /* max grain transposition in octaves */
#define GR_MAX_OUT_OF    (64.0) /* max fraction of omitted grains = 63/64 */
#define GR_MIN_TSTRETCH  (.001)
#define GR_MAX_TSTRETCH  (1000.0)
#define GR_GATE_DEFAULT  (0.3)
#define GR_MAX_OFFSET           (32767.0)       /* arbitrary */
#define MAX_FIRSTGRAIN_TIME     (32767.0)       /* arbitrary */
#define SPLBUF_OFFSET    (2)                    /* first 2 extrabufs are for splices */
                                                                                /* remainder store grains for reordering */
#define NOMINAL_LENGTH   (2)                    /* size of pre-malloced arrays */

#define LOW_RRR_SIZE     (15)                   /* 15 ms is approx size of fast rrr-flap */

#define NOIS_MIN_FRQ    (6000.0)
#define GR_MINDUR        (0.1)
