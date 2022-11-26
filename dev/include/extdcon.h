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



/******************************** EXTEND ******************************/

/* CONSTANTS FOR ZIGZAG */
#define ZIG_SPLICELEN     (15.0)
#define ZIG_MIN_UNSPLICED (1.0)
#define ZIG_EXPON                 (1.0)
#define MIN_ZIGSPLICE     (1.0)
#define MAX_ZIGSPLICE     (5000.0)
#define DEFAULT_LPSTEP    (200.0)
/* CONSTANTS FOR ITERATE */
#define ITER_MAXPSHIFT          (12.0)
#define ITER_MAX_DELAY          (100.0)
#define ITER_UPSCALE            (65536.0)
#define ITER_DNSHIFT            (16)
#define ITER_SAFETY             (512)
#define DEFAULT_ITER_GAIN       (0.0)
#define ST_INTP_SHIFT           (3)
#define MN_INTP_SHIFT           (4)
#define FIXED_AMP                       (10)  /* added to flags above gives flags below */
#define FIXA_MONO                       (11)
#define FIXA_STEREO             (12)
#define FIXA_ST_INTP_SHIFT      (13)
#define FIXA_MN_INTP_SHIFT      (14)
/* CONSTANTS FOR DRUNKWALK */
#define DRNK_GRAIN              (.002)
#define DRNK_DEFAULT_GSTEP      (10.0)
#define DRNK_DEFAULT_CLOKTIK (4.0)
#define DRNK_SPLICE                     (15.0)
#define DRNK_MIN_SPLICELEN       (1.0)
#define DRNK_MAX_SPLICELEN (5000.0)
#define MAX_DRNKTIK                     (32767.0)       /* arbitrary */
#define DEFAULT_MIN_DRNKTIK     (10.0)
#define DEFAULT_MAX_DRNKTIK (30.0)
#define DRNK_MAX_OVERLAP        (0.99)
/*TW March 2004 */
/* FORM DOUBLETS */
#define SPLICEDUR (5 * MS_TO_SECS)

#define ITX_SPLICELEN   (256)

#define ITERATE_EXTEND_BUFSIZE (512 * 512)
