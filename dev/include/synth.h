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

#define WAVE_TABSIZE    (256)
#define WAVE_DEFAULT_SR (44100)
#define MAX_SYN_DUR             (7200.0)        /* 2 hrs */
#define MIN_SYN_DUR             (0.04)          /* Assumes min samprate = 16000 and max chans = 2 */
                                                                        /* Otherwise, internal trap */
#define MAX_SYNTH_FRQ   (22000)         /* lowest conceivable nyquist */
#define MIN_SYNTH_FRQ   (0.1)           /*arbitrary */

#define SYNTH_SPLICELEN (256)

int do_synth(dataptr dz);
int do_stereo_specsynth(dataptr dz);

#define CHANUP (4)                              /* half-band of channels over which same frq can occur */
//TW: total amp of 1.0 seems rational, but causes distorttion on resynth
//#define SPECSYN_MAXAMP (1.0)  /* maximum total amp of a spectrum : guess */
#define SPECSYN_MAXAMP (0.5)    /* maximum total amp of a spectrum : guess */
//#define SPECSYN_SRATE  (48000)
//#define SPECSYN_MLEN   (4098)
 /*RWD more economical ones! */
#define SPECSYN_SRATE  (44100)
#define SPECSYN_MLEN   (1026)
#define CLICK_SRATE (48000)
#define CLICKLEN (20)
#define CLICKAMP1 (1.0)
#define CLICKAMP2 (.5)
#define CLICKAMP3 (.2)
