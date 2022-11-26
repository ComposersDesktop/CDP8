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



#ifndef _CDP_HOUSE_H_INCLUDED
#define _CDP_HOUSE_H_INCLUDED
#define IS_GROUCHO_COMPILE      (1)

#define MAXDUPL                         (999)
#define COPYDEL_OVERMAX         (10)
#define SORT_SMALL_DEFAULT  (1.0)
#define SORT_LARGE_DEFAULT  (16.0)
#define SORT_STEP_DEFAULT   (1.0)
#define SORT_LSTEP_DEFAULT  (2.0)
#define MIN_SORT_DUR        (0.005)
#define MAX_SORT_DUR        (120.0)
#define MIN_SORT_STEP       (0.005)
#define MAX_SORT_STEP       (60.0)
#define MIN_SORT_LSTEP      (1.414)
#define MAX_SORT_LSTEP      (8.0)
#define CUTGATE_SPLICELEN   (15.0)
#define CUTGATE_MINSPLICE   (2.0)
#define CUTGATE_MAXSPLICE   (200.0)
#define CUTGATE_MAX_SUSTAIN (64)
#define CUTGATE_MAXBAKTRAK      (64)
#define CUTGATE_MAXWINDOWS      (64)
#define TOPNTAIL_SPLICELEN      (2)
#define DUMPFILE_CHANNELS   (2)
#define DUMPFILE_SRATE      (48000)

#define BLEEP   (0)
#define HEADERS (1)
#define SOUND   (2)
#define SHORT   (3)

#define CYCLECNT          (512) /* number of square wave cycles in bleep */
#define HALFCYCLE          (50) /* square-wave is 50 * 2 samples long    */
#define SQUAREAMP        (8000) /* amplitude of square-wave bleep        */
#define HEADSIZE  (SECSIZE * 2) /* bytesize of sndfile header in dumpfile*/
#define HALFHEADSIZE  (SECSIZE) /* half size of sndfile header, in bytes */
#define HEADCOPIES        (100) /* number of bakup copies of header      */
#define DUMPDATSIZE ((CYCLECNT*HALFCYCLE*2*sizeof(short))+(HEADSIZE*HEADCOPIES))
#define TW_MAGIC    (271828182) /* e */

#define FIRST_HALF_SF_MAGIC             (30244)
#define SECOND_HALF_SF_MAGIC    (5522)
#define FIRST_HALF_TW_MAGIC             (-15146)
#define SECOND_HALF_TW_MAGIC    (4147)

#define OK_COUNT                (.25)   /* min fraction header_copies to recover    */
#define OK_MARKER               (.33)   /* frac of a header_marker OK  to validate  */
#define OK_SAME                 (.5)    /* frac of header to be same, to be a copy  */

#define HEADSEARCHSIZE  ((HEADSIZE*HEADCOPIES)+SECSIZE)

#define COPY_HEADER_WRITTEN (0)
#define HEADER_SIZE_DEFINED (0)

#define SHSECSIZE                       (SECSIZE/sizeof(short))

// TW NEW
//#define WHOLESECT(x)      (((x)/SHSECSIZE)*SHSECSIZE==(x))
#define WHOLESECT(x)        (((x)/(int)SHSECSIZE)*(int)SHSECSIZE==(x))

#define NAMESIZE            (64)
#define FILESIZE_SIZE           (sizeof(int))
#define MAGIC_SIZE                      (sizeof(int))
#define CRC_SIZE                        (sizeof(int))


// TW NEW
//#define SFREC_SHIFT_MIN               (-100)
//#define SFREC_SHIFT_MAX               (100)
#define SFREC_SHIFT_MIN         (-10)
#define SFREC_SHIFT_MAX         (10)



typedef unsigned char   uchar;

int             pconsistency_clean(dataptr dz);

int     create_respec_buffers(dataptr dz);
int             create_clean_buffers(dataptr dz);
int     create_bakup_buffers(dataptr dz);
int     create_recover_buffer(dataptr dz);

int     sort_preprocess(dataptr dz);
int     respec_preprocess(dataptr dz);

int             do_duplicates(dataptr dz);
int             do_deletes(dataptr dz);
int     do_channels(dataptr dz);
int     do_bundle(dataptr dz);
int     do_file_sorting(dataptr dz);
int     do_respecification(dataptr dz);
int             process_clean(dataptr dz);
int     sfdump(dataptr dz);
int     sfrecover(dataptr dz);
int     sum32(register int, register uchar *, int);

//TW NEW
int             check_available_diskspace(dataptr dz);
int             house_bakup(dataptr dz);
int             house_gate(dataptr dz);
int             batch_expand(dataptr dz);
int             house_gate2(dataptr dz);

#define BAKUP_GAP (1.0)

#endif
