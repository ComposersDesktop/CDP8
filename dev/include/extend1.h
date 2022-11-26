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



/*
 *      2nd header for EXTEND group of sndprgs - graphics compatible version - FUNCTIONS
 */

#define EXTEND1_H_RCSID "$Id$"
/*
 * $Log$
 */
/* RWD 6:2001 commented out funcs declare static to ap_extend.c, extprepro.c */
void    initialise_random_sequence(int seed_flagno,int seed_paramno,dataptr dz);
int     iterate_preprocess(dataptr dz);
int             convert_time_and_vals_to_samplecnts(int paramno,dataptr dz);
int     create_drunk_buffers(dataptr dz);
int     drunk_preprocess(dataptr dz);

int     zigzag(dataptr dz);
int     do_iteration(dataptr dz);
int     do_loops(int *thisstart,int *lastend,int *outbuf_space,int obufno,int splbufno,dataptr dz);
int     do_drunken_walk(dataptr dz);
int     accent_stream(dataptr dz);
int             do_sequence(dataptr dz);
int             do_sequence2(dataptr dz);
int             do_btob(dataptr dz);
/*TW March 2004 */
int             do_doubling(dataptr dz);
int             extend_by_insplice(dataptr dz);
