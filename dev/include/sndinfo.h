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



#define IS_GROUCHO_COMPILE      (1)

#define MAX_TIMESUM_SPLEN               (2000.0)        /* ms */
#define TIMESUM_DEFAULT_SPLEN   (15.0)          /* ms */
#define MAX_SFDIFF_CNT                  (1000)
#define MU_MAXGAIN                              (128.0)
#define MAX_OCTSHIFT                    (8.0)

int     do_sndinfo(dataptr dz);
int     do_musunits(dataptr dz);
