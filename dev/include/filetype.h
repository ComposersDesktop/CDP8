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



/** WARNING **
 *
 * If new filetypes are added..
 * FUNCTIONS  in filetype.c must be upgraded to take account of these!!
 */

/***************************** NEW FILE HEADER VALUES, ALSO USEFUL FOR FLAGGING IN GUI *************
                                                                                                                                                                   TPNdU LNSSMFS|
                                                                                                                                                                   rioBn iunyilp|
                                                                                                                                                                   atr r nmdnxoe|
                                                                                                                                                                   ncm n eb c ac|
                                                                                                                                                                   shl g  r   t |
                                                                                                                                                                           d                    */
#define BRKFILE                                                                                                                 (8192)  /* 0010000000000000     */
#define DB_BRKFILE                                                                                                              (4096)  /* 0001000000000000     */
#define UNRANGED_BRKFILE                                                                                                (2048)  /* 0000100000000000     */
#define NUMLIST                                                                                                                 (256)   /* 0000000100000000     */
#define SNDLIST                                                                                                                 (128)   /* 0000000010000000     */
#define SYNCLIST                                                                                                                (64)    /* 0000000001000000     */
#define MIXFILE                                                                                                         (32)    /* 0000000000100000     */
#define LINELIST                                                                                                                (512)   /* 0000001000000000     */
#define SNDFILE                                                                                                                 (1)             /* 0000000000000001     */
#define FLT_SNDFILE                                                                                                             (17)    /* 0000000000010001     */
#define ANALFILE                                                                                                                (24)    /* 0000000000011000     */
#define PITCHFILE                                                                                                               (25)    /* 0000000000011001     */
#define TRANSPOSFILE                                                                                                    (26)    /* 0000000000011010     */
#define FORMANTFILE                                                                                                             (27)    /* 0000000000011011     */
#define ENVFILE                                                                                                                 (16)    /* 0000000000010000     */
#define TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST    (41728) /* 1010001100000000     */
#define TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST                                (41216) /* 1010000100000000     */
#define TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST    (49920) /* 1100001100000000     */
#define TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST                                (49408) /* 1100000100000000     */
#define TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST (35584) /* 1000101100000000     */
#define TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST                             (35072) /* 1000100100000000     */
#define NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST                                (8960)  /* 0010001100000000     */
#define NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST                                                    (8448)  /* 0010000100000000     */
#define DB_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST                                   (4864)  /* 0001001100000000     */
#define DB_BRKFILE_OR_NUMLIST_OR_WORDLIST                                                               (4352)  /* 0001000100000000     */
#define PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST                                (17152) /* 0100001100000000     */
#define PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST                                                    (16640) /* 0100000100000000     */
#define PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST               (18176) /* 0100011100000000     */
#define PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST                                   (17664) /* 0100010100000000     */
#define UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST                             (2816)  /* 0000101100000000     */
#define UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST                                                 (2304)  /* 0000100100000000     */
#define NUMLIST_OR_LINELIST_OR_WORDLIST                                                                 (768)   /* 0000001100000000     */
#define NUMLIST_OR_WORDLIST                                                                                             (256)   /* 0000000100000000     */
#define SNDLIST_OR_SYNCLIST_LINELIST_OR_WORDLIST                                                (704)   /* 0000001011000000     */
#define SNDLIST_OR_SYNCLIST_OR_WORDLIST                                                                 (192)   /* 0000000011000000     */
#define SNDLIST_OR_LINELIST_OR_WORDLIST                                                                 (640)   /* 0000001010000000     */
#define SNDLIST_OR_WORDLIST                                                                                             (128)   /* 0000000010000000     */
#define MIXLIST_OR_LINELIST_OR_WORDLIST                                                                 (544)   /* 0000001000100000     */
#define MIXLIST_OR_WORDLIST                                                                                             (32)    /* 0000000000100000     */
#define SYNCLIST_OR_LINELIST_OR_WORDLIST                                                                (576)   /* 0000001001000000     */
#define SYNCLIST_OR_WORDLIST                                                                                    (64)    /* 0000000001000000     */
#define LINELIST_OR_WORDLIST                                                                                    (512)   /* 0000001000000000     */
#define WORDLIST                                                                                                                (288)   /* 0000000100100000     */
#define POSITIVE_BRKFILE                                                                                                (1024)  /* 0000010000000000 */
#define POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST                             (1792)  /* 0000011100000000     */
#define POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST                                                 (1280)  /* 0000010100000000     */


/**** VALUES ONLY USED AFTER redefine_textfile_type... *********/

#define ANYFILE                 (65535) /* 1111111111111111 (probably redundant)*/
#define TEXTFILE                (96)    /* 0000000001100000 */

/*********** NEW FLAG RECOGNITION CONSTANTS **************/
                                                                                                /* Operation
                                                                                                with program's
                                                                                                filetype flag */
#define IS_A_PITCH_BRKFILE              (16384)                         /* &=   TRUE */         /* 0100000000000000     */
#define IS_A_TRANSPOS_BRKFILE   (32768)                         /* &=   TRUE */         /* 1000000000000000     */
#define IS_A_POSITIVE_BRKFILE   POSITIVE_BRKFILE        /* &=   TRUE */         /* 1000000000000000     */
#define IS_A_NORMD_BRKFILE              BRKFILE                         /* &=   TRUE */
#define IS_A_DB_BRKFILE                 DB_BRKFILE                      /* &=   TRUE */
#define IS_AN_UNRANGED_BRKFILE  (3072)                          /* &=   TRUE */         /* 0000110000000000     */
#define IS_A_NUMLIST                    NUMLIST                         /* &=   TRUE */
#define IS_A_SNDLIST                    SNDLIST                         /* &=   TRUE */
#define IS_A_SYNCLIST                   SYNCLIST                        /* &=   TRUE */
#define IS_A_MIXFILE                MIXFILE                     /* &=   TRUE  && !WORDLIST */
#define IS_A_LINELIST                   LINELIST                        /* &=   TRUE */
#define IS_A_SNDFILE                    SNDFILE                         /* ==            */
#define IS_A_FLT_SNDFILE                FLT_SNDFILE                     /* ==            */
#define IS_AN_ANALFILE                  ANALFILE                        /* ==            */
#define IS_A_PITCHFILE                  PITCHFILE                       /* ==            */
#define IS_A_TRANSPOSFILE               TRANSPOSFILE            /* ==            */
#define IS_A_FORMANTFILE                FORMANTFILE                     /* ==            */
#define IS_AN_ENVFILE                   ENVFILE                         /* ==            */
#define IS_A_SNDSYSTEM_FILE             (31)                            /* &=   TRUE                                                    0000000000011111 */
#define IS_A_PTF_BINFILE                (27)                            /* &=   > ANALFILE                                              0000000000011011 */
#define IS_A_TEXTFILE                   (65504)                         /* &=   TRUE                                                    1111111111100000 */
#define IS_A_SNDLIST_FIXEDSR    (192)                           /* &=   IS_A_SNDLIST_FIXEDSR                    0000000011000000 */
#define IS_A_NUMBER_LINELIST    (768)                           /* &=   IS_A_NUMBER_LINELIST                    0000001100000000 */
#define IS_MIX_OR_SNDLST_FIXSR  (224)                           /* &=   MIXFILE||IS_A_SNDLIST_FIXEDSR   0000000011100000 */
#define IS_A_BRKFILE                    (64512)                         /* &=   TRUE                                                    1111110000000000 */
#define IS_A_P_OR_T_BRK                 (39152)                         /* &=   TRUE                                                    1100000000000000 */

/* NOT USED, IF USED, CHANGE BIT 6 (counting from 1 at LEFT, to 1 */
#define IS_AN_ENV_OR_DB_BRK             (12288)                         /* &=   TRUE                                                    0011000000000000 */

#define POSSIBLY_FILENAMES              (928)                           /* &=   TRUE                                                    0000001110100000 */
/* ANYFILE                              NO TEST */
#define IS_ANYFILE                              (65535)                         /* &=   TRUE                                                    1111111111111111 */

#define IS_NOT_PURE_NUMS_TEXT   (65248)                         /* &=   TRUE                                                    1111111011100000 */
#define IS_NOT_PURENUMS_OR_MIX_TEXT     (65216)                 /* &=   TRUE                                                    1111111011000000 */
#define IS_NOT_MIX_TEXT                 (65472)                         /* &=   TRUE                                                    1111111111000000 */

#define IS_ANALDERIVED                  (8)                                     /* &=   TRUE                                                    0000000000001000 */




#define DEFAULT_FILENAME        "#test"

#define ONE_NONSND_FILE                 (-100)  /* flag */
