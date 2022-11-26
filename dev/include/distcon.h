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



/************************ DISTORT *************************/
/* floatsam version*/
/* CONSTANTS FOR DISTORT */

/*#define CLIPMAX           ((short)24000)*/
#define FCLIPMAX                (0.7325f)
#define SINETABLEN  (1024)
#define TABLEN      (1024.0)
#define PULSEWIDTH  (0.001)             /* secs */
#define MAXWIDTH        (6.0)

/* CONSTANTS FOR DISTORTA, DISTORTTH, DISTORTF */

#define MAXWAVELEN   (.5)               /* seconds */

/* CONSTANTS FOR DISTORTH */

/* CONSTANTS FOR DISTORTF */

#define MIN_SCALE               (2.0)

/* CONSTANTS FOR DISTORT_RPT */

#define MULTIPLY                (0)
#define INTERPOLATE             (1)
#define GRP_MULTIPLY    (2)

/* CONSTANTS FOR DISTORT_DEL */

#define LOSE                    (0)
#define KEEP                    (1)

/* CONSTANTS FOR DISTORT_PCH */

#define MAXOCTVAR               (8.0)
#define DEFAULT_RSTEP   (64)

#define MINWAVELEN              (8.0)           /* samples */

#define DISTORT_MAX_POWFAC              (40.0)
#define DISTORTE_MIN_EXPON              (0.02)
#define DISTORTE_MAX_EXPON              (50.0)
#define DISTORTH_MAX_PRESCALE   (200.0)
#define CYCLIM_DFLTFRQ  (1000.0)
