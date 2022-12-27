/*
 * Copyright (c) 1983-2013 Richard Dobson and Composers Desktop Project Ltd
 * http://people.bath.ac.uk/masrwd
 * http://www.composersdesktop.com
 * This file is part of the CDP System.
 * The CDP System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *
 * The CDP System is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with the CDP System; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
 
/*chanmask.h*/
#ifndef __CHANMASK_H_INCLUDED
#define __CHANMASK_H_INCLUDED

#define NUM_SPEAKER_POSITIONS (18)


#define SPEAKER_FRONT_LEFT              0x1
#define SPEAKER_FRONT_RIGHT             0x2
#define SPEAKER_FRONT_CENTER            0x4
#define SPEAKER_LOW_FREQUENCY           0x8
#define SPEAKER_BACK_LEFT               0x10
#define SPEAKER_BACK_RIGHT              0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER    0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER   0x80
#define SPEAKER_BACK_CENTER             0x100
#define SPEAKER_SIDE_LEFT               0x200
#define SPEAKER_SIDE_RIGHT              0x400
#define SPEAKER_TOP_CENTER              0x800
#define SPEAKER_TOP_FRONT_LEFT          0x1000
#define SPEAKER_TOP_FRONT_CENTER        0x2000
#define SPEAKER_TOP_FRONT_RIGHT         0x4000
#define SPEAKER_TOP_BACK_LEFT           0x8000
#define SPEAKER_TOP_BACK_CENTER         0x10000
#define SPEAKER_TOP_BACK_RIGHT          0x20000
/* RWD 2022 use latest version from MS */
#define SPEAKER_RESERVED                0x7FFC0000
#define SPEAKER_ALL                     0x80000000

/* my extras*/
#define SPKRS_UNASSIGNED    (0)
#define SPKRS_MONO          (0x00000004)  /* RWD 11:20 correction, was 0x01! */
#define SPKRS_STEREO        (0x00000003)
#define SPKRS_GENERIC_QUAD  (0x00000033)
#define SPKRS_SURROUND_LCRS (0x00000107)
#define SPKRS_DOLBY5_1      (0x0000003f)
#define SPKRS_SURR_5_0      (0x00000037)    /* March 2008 */
#define SPKRS_SURR_6_1      (0x0000013f)    /* RELEASE7 2013 */
#define SPKRS_SURR_7_1      (0x000000ff)    /* OCT 2009 */
#define SPKRS_CUBE          (0x0002d033)
#define SPKRS_ACCEPT_ALL    (0xffffffff)     /*???? no use for a file*/
#endif
