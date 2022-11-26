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

#define EDIT_SPLICELEN  (15.0)
#define EDIT_SPLICEMAX  (5000.0)
#define EDIT_MAXGAIN    (128.0)
#define MINOUTDUR               (0.05)
#define MIN_SUPRESS             (1000)

int     create_edit_buffers(dataptr dz);
int     create_twixt_buffers(dataptr dz);
int     edit_pconsistency(dataptr dz);
int     edit_preprocess(dataptr dz);
int     twixt_preprocess(dataptr dz);
int     sphinx_preprocess(dataptr dz);
int     do_cut(dataptr dz);
int     do_zcut(dataptr dz);
int     do_excise(dataptr dz);
int     do_insert(dataptr dz);
int     do_insertsil_many(dataptr dz);
int     do_joining(dataptr dz);
int     do_patterned_joining(dataptr dz);       /*TW March 2004 */
int             do_randcuts(dataptr dz);
int             do_randchunks(dataptr dz);
int             do_twixt(dataptr dz);
int     get_switchtime_data(char *filename,dataptr dz);
int     get_multi_switchtime_data(char *filename,dataptr dz);
int     cut_many(dataptr dz);
int     do_noise_suppression(dataptr dz);
int     do_syllabs(dataptr dz);                         /*TW March 2004 */
int             do_many_zcuts(dataptr dz);
#define SPLICELEN (dz->ringsize)        /* dz->ringsize being used to store SPLICELEN */
                                                                        /* danger, don't use RING functions anywhere in sfedit functions !! */
