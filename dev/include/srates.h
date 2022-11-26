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



/*rates.h: portable set of defines for sample rates on PC/SGI/FALCON */

#ifdef ATARI

#define IS_HISR(x) ((x)==49170 || (x)==48000 || (x)==44100 || \
        (x)==32780 || (x)==32000)

#define IS_LOSR(x) ((x)==24585 || (x)==24000 || (x)==22050 || (x)==19668 \
        || (x)==16390 || (x)==16000 || (x)==12292 || (x)==9834 || (x)==8195)

#define IS_FALSR(x) ((x)==49170 || (x)==32780 || (x)==24585 || (x)==19668 \
        || (x)== 16390 || (x)== 12292 || (x)== 9834 || (x)==8195)

#define IS_HIFR(x) ((x)==49170 || (x)==32780)
#define IS_LOFR(x) ((x)==24585 || (x)==19668 || (x)==16390 || \
        (x)==12292 || (x)==9834 || (x)==8195)

#define BAD_SR(x) (x!=49170 && x!=44100 && x!=32780 && x!=32000 \
        && x!=24585 && x!= 24000 && x!= 22050 && x!=19668  && x!=16390  \
        && x!=16000 && x!= 12292 && x!= 9834 && x!= 8195)

#define SR_DEFAULT (24585)
#define D_SR_DEFAULT (24585.0)

static const char *FalconRates =
        "49170, 32780, 24585, 19668, 16390, 12292, 9834 or 8195";

#else
/*RWD April 2005 added new high srates */
#define IS_HISR(x) ((x)== 96000 || (x) == 88200 || (x)==48000 || (x)==44100 || (x)==32000)
#define IS_LOSR(x) ((x)==24000 || (x)==22050 || (x)== 16000)
#define SR_DEFAULT (22050)
#define D_SR_DEFAULT (22050.0)
#define BAD_SR(x) ((x)!=44100 && (x)!=22050 && (x)!=32000 && (x)!=16000  \
        && (x)!=48000 && (x)!= 24000 && (x)!=88200 && (x)!=96000)

#endif
