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



#include <pnames.h>

/*
 * 1)   When compiling validate library for CDPARSE, ALL condit defines must be ON!!
 */

#ifdef ENVEL_COMPILE
#define IS_GROUCHO_COMPILE              (1)
#include <envel.h>
#endif

#ifdef DISTORT_COMPILE
#define IS_GROUCHO_COMPILE              (1)
#include <distort.h>
#endif

#ifdef EXTEND_COMPILE
#define IS_GROUCHO_COMPILE              (1)
#include <extend.h>
#endif

#ifdef TEXTURE_COMPILE
#define IS_GROUCHO_COMPILE              (1)
#include <texture.h>
#endif

#ifdef GRAIN_COMPILE
#define IS_GROUCHO_COMPILE              (1)
#include <grain.h>
#endif

#ifdef MIX_COMPILE
#define IS_GROUCHO_COMPILE              (1)
#include <mix.h>
int  set_up_mix(dataptr dz);
#endif

#ifdef FILTER_COMPILE
#define IS_GROUCHO_COMPILE              (1)
#include <filters.h>
#endif

#ifdef MODIFY_COMPILE
#define IS_GROUCHO_COMPILE              (1)
#include <modify.h>
#endif


#ifdef SPEC_SIMPLE_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <simple.h>
#endif

#ifdef SPEC_STRETCH_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <stretch.h>
#endif

#ifdef SPEC_PITCH_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <pitch.h>
#endif

#ifdef SPEC_HIGHLIGHT_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <highlight.h>
#endif

#ifdef SPEC_FOCUS_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <focus.h>
#endif

#ifdef SPEC_BLUR_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <blur.h>
#endif

#ifdef SPEC_STRANGE_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <strange.h>
#endif

#ifdef SPEC_MORPH_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <morph.h>
#endif

#ifdef SPEC_REPITCH_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <repitch.h>
#endif

#ifdef SPEC_FORMANTS_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <fmnts.h>
#endif

#ifdef SPEC_COMBINE_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <combine.h>
#endif

#ifdef SPEC_INFO_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <specinfo.h>
#endif

#ifdef SPEC_PINFO_COMPILE
#define IS_SPEC_COMPILE         (1)
#include <specpinfo.h>
#endif

#ifdef PVOC_COMPILE
#define IS_GROUCHO_COMPILE      (1)
#include <pvoc.h>
#endif

#ifdef EDIT_COMPILE
#define IS_GROUCHO_COMPILE      (1)
#include <edit.h>
#endif

#ifdef HOUSEKEEP_COMPILE
#define IS_GROUCHO_COMPILE      (1)
#include <house.h>
#endif

#ifdef SNDINFO_COMPILE
#define IS_GROUCHO_COMPILE      (1)
#include <sndinfo.h>
#endif

#ifdef SYNTHESIS_COMPILE
#define IS_GROUCHO_COMPILE      (1)
#include <synth.h>
#endif


#ifdef UTILS_COMPILE
#define IS_GROUCHO_COMPILE      (1)
#include <utils.h>
#endif
