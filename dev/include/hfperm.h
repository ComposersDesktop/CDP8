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



#define MAX_HFPERMSET (48.0)
#define HFP_OUTLEVEL (0.8)
#define HFP_TABSIZE (2048)
#define HFP_MAXJITTER (15.0)
#define HP1_SPLICELEN (15.0)

int do_hfperm(dataptr dz);

#define ROOT            (0)
#define TOP                     (1)
#define PCLASS          (2)
#define CHORDTYPE       (3)
#define CHORD_1         (4)

#define MAXSPAN_LAST    (0)     /* means nothing for chords */
#define MAXSPAN_FIRST   (1)     /* means nothing for chords */

/* both means nothing for chords OR density */
#define DENSE_SORT      (0)     /* sort by DENSITY */
#define OTHER_SORT      (1)     /* for ROOT & TOP, sort by INTSTAK : for PCLASS, sort by MAXINT then DENSITY */

int gen_dp_output(dataptr dz);
