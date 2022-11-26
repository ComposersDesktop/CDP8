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



#define SFD_BLEEP       (0)
#define SFD_HEADERS     (1)
#define SFD_SOUND       (2)
#define SFD_SHORT       (3)

#define DUMPFILE_CHANNELS   (2)
#define DUMPFILE_SRATE      (48000)

#define SFD_CYCLECNT       (512)        /* number of square wave cycles in bleep */
#define SFD_HALFCYCLE           (50)    /* square-wave is 50 * 2 samples long    */
                                                                        /* at 44100,stereo = 882 Hz for .58 secs */
#define SFD_SQUAREAMP        (8000)     /* amplitude of square-wave bleep        */
#define SFD_HEADSIZE  (SECSIZE * 2)     /* bytesize of sndfile header in dumpfile*/
#define SFD_HALFHEADSIZE  (SECSIZE)     /* half size of sndfile header, in bytes */
#define SFD_HEADCOPIES        (100)     /* number of bakup copies of header      */
#define SFD_DUMPDATSIZE ((SFD_CYCLECNT*SFD_HALFCYCLE*2*sizeof(short))+(SFD_HEADSIZE*SFD_HEADCOPIES))
#define TW_MAGIC    (271828182)         /* e */

#define FIRST_HALF_SF_MAGIC             (30244)
#define SECOND_HALF_SF_MAGIC    (5522)
#define FIRST_HALF_TW_MAGIC             (-15146)
#define SECOND_HALF_TW_MAGIC    (4147)

#define SFD_HEADSEARCHSIZE  ((HEADSIZE*HEADCOPIES)+SECSIZE)

#define SFD_COPY_HEADER_WRITTEN (0)
#define SFD_HEADER_SIZE_DEFINED (0)

#define SFD_NAMESIZE            (64)

#define BUFFER_START    (0)
#define BUFFER_END              (1)
#define SFD_INBUF               (2)

#define SFD_DATAPTR             (0)
#define SFD_DATAEND             (1)
#define SFD_OUTBUF              (2)
#define SFD_OUTBUFEND   (3)

extern  char    *headend, *hdataend;
extern int      sfd_outfilesize;
extern int              magicsize;
extern int              crcsize;
extern int              sizesize;
extern char             sfd_outfilename[];
extern int              short_secsize;

/* SFREC1.C     MAIN    */

int  read_bytes_aa(char *bbuf,int bytes_to_read,dataptr dz);
int  write_bytes_recover
        (char *bbuf,int bytes_to_write,int total_bytes_left,int *bytes_written,dataptr dz);
void um(char *,char *,int,int,dataptr dz);

/* SFREC2.C     WRITE SOUND     */

int relocate_search_position(int *bufpos,int *buffer_full,short *snd_data_end,dataptr dz);
int align_sound_to_buffer(int *buffer_full,int *bufpos,int cnt,int *total_bytes_left,dataptr dz);
int fill_rest_of_buf(int refill_sects,int *buffer_full,dataptr dz);
int adjust_alignment(int bufpos,int buffer_full,short **bptr,dataptr dz);

/* SFREC3.C     WRITE SOUND     */

int write_snd_rec(int sfd_outfilesize,int *buffer_full,int *total_bytes_left,short **snd_data_end,dataptr dz);

/* SFREC4.C     LOCATE HEADER   */

int locate_header(int *bufpos,headptr *head_variant,int *buffer_full,int *total_bytes_left,dataptr dz);
int get_true_header(headptr *head_variant,char *headstore,int *sfd_outfilesize,char *sfd_outfilename,dataptr dz);

/* SFREC5.C     VERIFY HEADER   */

int recover_header(char *headstore,int *sfd_outfilesize,char *sfd_outfilename,dataptr dz);

/************************************************************************
 ************************ BUFFER MANIPULATION ***************************
 ************************************************************************
 *
 *                      STAGGERED BUFFERING
 *                      -------------------
 *
 *       ____________extended_buffer_size_______________
 *      |                                               |
 *      |                                               |
 *      |       ..............buffer_size...............|
 *      |       :                                       :
 *      |       :             INPUT BUFFER              :
 *      |_______|_______|_______|_______|_______|_______|
 *              |       |       |       |               |                       O  = outbuf
 *         O    I                                                               OE                      I  = inbuf
 *         |              OUTPUT BUFFER                            |            OE = outbufend
 *         |                                       |
 *         |_______|_______buffer_size_____|_______|remnant
 *         :    :                                                                  :    :
 *         :....:                                                                  :....:
 *            ^                                                                  |
 *            |                                                                  V
 *            |___move remnant to start of outbuf____|
 *
 */
