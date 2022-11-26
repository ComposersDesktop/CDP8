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



/******************* OUTPUT FILE TYPES ***************************************/

#define NO_OUTPUTFILE                      (0)
#define SNDFILE_OUT                        (1)
#define ANALFILE_OUT                       (2)
#define PITCH_OUT                          (3)
#define TRANSPOS_OUT                       (4)
#define FORMANTS_OUT                       (5)
#define TEXTFILE_OUT                       (6)
#define ENVFILE_OUT                                (7)
#define BRKFILE_OUT                                (8)

/******************* PROCESS TYPES ***************************************/

#define UNEQUAL_SNDFILE                         (0)
#define EQUAL_ANALFILE                      (1)
#define MAX_ANALFILE                        (2)
#define MIN_ANALFILE                        (3)
#define BIG_ANALFILE                        (4)
#define PSEUDOSNDFILE                       (5)
#define TO_TEXTFILE                                 (6)
#define ANAL_TO_FORMANTS                    (7)
#define ANAL_TO_PITCH                       (8)
#define PITCH_TO_PITCH                      (9)
#define PITCH_TO_BIGPITCH                   (10)
#define PITCH_TO_ANAL                       (11)
#define PITCH_TO_PSEUDOSND                  (12)
#define EQUAL_FORMANTS                      (13)
#define SCREEN_MESSAGE                      (14)
#define EQUAL_SNDFILE                           (15)
#define EQUAL_ENVFILE                           (16)
#define UNEQUAL_ENVFILE                         (17)
#define CREATE_ENVFILE                          (18)
#define EXTRACT_ENVFILE                         (19)
#define OTHER_PROCESS                           (20)

/************************** INPUT PROCESS TYPE *****************************************/

#define ANY_NUMBER_OF_ANY_FILES (-3)
#define ALL_FILES                               (-2)
#define NO_FILE_AT_ALL                  (-1)
#define SNDFILES_ONLY                   (0)             /* Default */
#define ANALFILE_ONLY                   (1)
#define TWO_ANALFILES                   (2)
#define THREE_ANALFILES                 (3)
#define MANY_ANALFILES                  (4)
#define FORMANTFILE_ONLY                (5)
#define PITCHFILE_ONLY                  (6)
#define PITCH_OR_TRANSPOS               (7)
#define ANAL_AND_FORMANTS               (8)
#define PITCH_AND_FORMANTS              (9)
#define PITCH_AND_PITCH                 (10)
#define PITCH_AND_TRANSPOS              (11)
#define TRANSPOS_AND_TRANSPOS   (12)
#define ANAL_WITH_PITCHDATA             (13)
#define ANAL_WITH_TRANSPOS              (14)
#define TWO_SNDFILES                    (15)
#define MANY_SNDFILES                   (16)
#define ENVFILES_ONLY                   (17)
#define BRKFILES_ONLY                   (18)    /* NB ONLY WORKS with a SINGLE brkfile */
#define DB_BRKFILES_ONLY                (19)
#define SNDFILE_AND_ENVFILE             (20)
#define SNDFILE_AND_BRKFILE             (21)
#define SNDFILE_AND_DB_BRKFILE  (22)
#define MIXFILES_ONLY                   (23)
#define SNDLIST_ONLY                    (24)
#define SND_OR_MIXLIST_ONLY             (25)
#define SND_SYNC_OR_MIXLIST_ONLY (26)
#define WORDLIST_ONLY                   (27)
#define ONE_OR_MANY_SNDFILES    (28)
#define UNRANGED_BRKFILE_ONLY   (29)
#define TWO_WORDLISTS                   (30)
#define SNDFILE_AND_UNRANGED_BRKFILE    (31)
//TW NEW
#define PFE                                             (32)
#define ONE_OR_MORE_SNDSYS          (33)
