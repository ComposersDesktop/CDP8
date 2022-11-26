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



/* 'MODIFY' FUNCTIONS */

int     granula_pconsistency(dataptr dz);
int     lobit_pconsistency(dataptr dz);
int     scrub_pconsistency(dataptr dz);
int     crossmod_pconsistency(dataptr dz);

int     granula_preprocess(dataptr dz);
int     vtrans_preprocess(dataptr dz);
int     scrub_preprocess(dataptr dz);
int     create_rm_sintab(dataptr dz);
int     sausage_preprocess(dataptr dz);
//TW NEW
int     stack_preprocess(dataptr dz);

int     granula_process(dataptr dz);
int     loudness_process(dataptr dz);
int     dopan(dataptr dz);
int     mirroring(dataptr dz);
void    mirror_panfile(dataptr dz);
int     narrow_sound(dataptr dz);
int     process_varispeed(dataptr dz);
int     do_reversing(dataptr dz);
int     lobit_process(dataptr dz);
int     do_scrubbing(dataptr dz);
int     ring_modulate(dataptr dz);
int     cross_modulate(dataptr dz);
int     generate_sintable(dataptr dz);
//TW NEW
int     do_stack(dataptr dz);

int     create_modspeed_buffers(dataptr dz);
int     create_delay_buffers(dataptr dz);
int     create_reversing_buffers(dataptr dz);
int     create_shred_buffers(dataptr dz);
int     create_scrub_buffers(dataptr dz);
int     create_crossmod_buffers(dataptr dz);
int     create_sausage_buffers(dataptr dz);

void    print_message_flush(char *str);
void    print_warning_flush(char *str);
//TW NEW
int             do_convolve(dataptr dz);
int             convolve_preprocess(dataptr dz);
int             do_shudder(dataptr dz);
int             findpan(dataptr dz);
