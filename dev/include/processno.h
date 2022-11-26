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



/******************* PROGRAM PROCESS NUMBERS ***************************************/

#define MIN_PROCESS_NO  (GAIN)
/***** SPECTRAL *****/
/*********** SIMPLE ********/
#define GAIN            (1)
#define LIMIT           (2)
#define BARE            (3)
#define CLEAN           (4)
#define CUT                     (5)
#define GRAB            (6)
#define MAGNIFY         (7)
/********* STRETCH ********/
#define STRETCH         (8)
#define TSTRETCH        (9)
/******* PITCH & HARMONY ****/
#define ALT                     (10)
#define OCT                     (11)
#define SHIFTP          (12)
#define TUNE            (13)
#define PICK            (14)
#define MULTRANS        (15)
#define CHORD           (16)
/********** HIGHLIGHT ********/
#define FILT            (17)
#define GREQ            (18)
#define SPLIT           (19)
#define ARPE            (20)
#define PLUCK           (21)
#define S_TRACE         (22)
#define BLTR            (23)
/***** FOCUS ********/
#define ACCU            (24)
#define EXAG            (25)
#define FOCUS           (26)
#define FOLD            (27)
#define FREEZE          (28)
#define STEP            (29)
/***** BLUR ******/
#define AVRG            (30)
#define BLUR            (31)
#define SUPR            (32)
#define CHORUS          (33)
#define DRUNK           (34)
#define SHUFFLE         (35)
#define WEAVE           (36)
#define NOISE           (37)
#define SCAT            (38)
#define SPREAD          (39)
/******* STRANGE ****/
#define SHIFT           (40)
#define GLIS            (41)
#define WAVER           (42)
#define WARP            (43)
#define INVERT          (44)
/**** MORPH ******/
#define GLIDE           (45)
#define BRIDGE          (46)
#define MORPH           (47)
/****** REPITCH ******/
#define PITCH           (48)
#define TRACK           (49)
#define P_APPROX        (50)
#define P_EXAG          (51)
#define P_INVERT        (52)
#define P_QUANTISE      (53)
#define P_RANDOMISE     (54)
#define P_SMOOTH        (55)
#define P_TRANSPOSE     (56)
#define P_VIBRATO       (57)
#define P_CUT           (58)
#define P_FIX           (59)
#define REPITCH         (60)
#define REPITCHB        (61)
#define TRNSP           (62)
#define TRNSF           (63)
/****** FORMANTS ******/
#define FORMANTS        (64)
#define FORM            (65)
#define VOCODE          (66)
#define FMNTSEE     (67)
#define FORMSEE         (68)
/**** COMBINE *****/
#define MAKE            (69)
#define SUM             (70)
#define DIFF            (71)
#define LEAF            (72)
#define MAX                     (73)
#define MEAN            (74)
#define CROSS           (75)
/********** INFO ************/
#define WINDOWCNT       (76)
#define CHANNEL         (77)
#define FREQUENCY       (78)
#define LEVEL           (79)
#define OCTVU           (80)
#define PEAK            (81)
#define REPORT          (82)
#define PRINT           (83)
/********** PITCH INFO ************/
#define P_INFO          (84)
#define P_ZEROS         (85)
#define P_SEE           (86)
#define P_HEAR          (87)
#define P_WRITE         (88)

#define TOP_OF_SPEC_PROCESSES           (P_WRITE)

#define FOOT_OF_GROUCHO_PROCESSES       (MANY_ZCUTS)

/**** NON SPECTRAL ***/

#define MANY_ZCUTS               (96) /* several cuts at zero-crossing */
#define MULTI_SYN                (97) /* synthesize fixed chord */
#define MIXBALANCE               (98) /* mix two sounds with balance function */
#define INFO_MAXSAMP2    (99) /* maxsamp over time-range */

/******************* DISTORT **********************/

#define DISTORT_CYCLECNT (100) /* cyclecnt    */
#define DISTORT          (101)   /* reshape     */
#define DISTORT_ENV      (102)   /* envelope    */
#define DISTORT_AVG      (103)   /* average     */
#define DISTORT_OMT      (104)   /* omit        */
#define DISTORT_MLT      (105)   /* multiply    */
#define DISTORT_DIV      (106)   /* divide          */
#define DISTORT_HRM      (107)   /* harmonic    */
#define DISTORT_FRC      (108)   /* fractal     */
#define DISTORT_REV      (109)   /* reverse     */
#define DISTORT_SHUF (110)       /* shuffle     */
#define DISTORT_RPT  (111)       /* repeat      */
#define DISTORT_INTP (112)       /* interpolate */
#define DISTORT_DEL  (113)       /* delete      */
#define DISTORT_RPL  (114)       /* replace     */
#define DISTORT_TEL  (115)       /* telescope   */
#define DISTORT_FLT  (116)       /* filter      */
#define DISTORT_INT  (117)       /* interact    */
#define DISTORT_PCH  (118)       /* pitch       */

/******************* EXTEND **********************/

#define ZIGZAG           (119)   /* zigzag   */
#define LOOP             (120)   /* loop     */
#define SCRAMBLE         (121)   /* scramble */
#define ITERATE          (122)   /* iterate  */
#define DRUNKWALK        (123)   /* drunk    */

/******************* TEXTURE **********************/

#define SIMPLE_TEX       (124)   /* texture  */
#define GROUPS           (125)   /* texture  */
#define DECORATED        (126)   /* texture  */
#define PREDECOR         (127)   /* texture  */
#define POSTDECOR        (128)   /* texture  */
#define ORNATE           (129)   /* texture  */
#define PREORNATE        (130)   /* texture  */
#define POSTORNATE       (131)   /* texture  */
#define MOTIFS           (132)   /* texture  */
#define MOTIFSIN         (133)   /* texture  */
#define TIMED            (134)
#define TGROUPS          (135)
#define TMOTIFS          (136)
#define TMOTIFSIN        (137)

/******************* GRAIN **********************/

#define GRAIN_COUNT         (138)
#define GRAIN_OMIT          (139)
#define GRAIN_DUPLICATE (140)
#define GRAIN_REORDER   (141)
#define GRAIN_REPITCH   (142)
#define GRAIN_RERHYTHM  (143)
#define GRAIN_REMOTIF   (144)
#define GRAIN_TIMEWARP  (145)
#define GRAIN_GET               (146)
#define GRAIN_POSITION  (147)
#define GRAIN_ALIGN     (148)
#define GRAIN_REVERSE   (149)

/******************* ENVELOPE **********************/

#define ENV_CREATE              (150)
#define ENV_EXTRACT             (151)
#define ENV_IMPOSE              (152)
#define ENV_REPLACE             (153)
#define ENV_WARPING             (154)
#define ENV_RESHAPING   (155)
#define ENV_REPLOTTING  (156)
#define ENV_DOVETAILING (157)
#define ENV_CURTAILING  (158)
#define ENV_SWELL               (159)
#define ENV_ATTACK              (160)
#define ENV_PLUCK               (161)
#define ENV_TREMOL              (162)
#define ENV_ENVTOBRK    (163)
#define ENV_ENVTODBBRK  (164)
#define ENV_BRKTOENV    (165)
#define ENV_DBBRKTOENV  (166)
#define ENV_DBBRKTOBRK  (167)
#define ENV_BRKTODBBRK  (168)

/******************* MIX **********************/

#define MIXTWO           (169)
#define MIXCROSS         (170)
#define MIXINTERL        (171)
#define MIXINBETWEEN (172)
#define MIX                      (173)
#define MIXMAX           (174)
#define MIXGAIN          (175)
#define MIXSHUFL         (176)
#define MIXTWARP         (177)
#define MIXSWARP         (178)
#define MIXSYNC          (179)
#define MIXSYNCATT       (180)
#define MIXTEST          (181)
#define MIXFORMAT        (182)
#define MIXDUMMY         (183)
#define MIXVAR           (184)

/******************* FILTER **********************/

#define EQ                       (185)
#define LPHP             (186)
#define FSTATVAR         (187)
#define FLTBANKN         (188)
#define FLTBANKC         (189)
#define FLTBANKU         (190)
#define FLTBANKV         (191)
#define FLTSWEEP         (192)
#define FLTITER          (193)
#define ALLPASS          (194)

/******************* MODIFY **********************/

#define MOD_LOUDNESS    (195)
#define MOD_SPACE               (196)
#define MOD_PITCH               (197)
#define MOD_REVECHO             (198)
#define BRASSAGE                (199)
#define SAUSAGE                 (200)
#define MOD_RADICAL             (201)

/******************* PVOC **********************/

#define PVOC_ANAL               (202)
#define PVOC_SYNTH              (203)
#define PVOC_EXTRACT    (204)

#define WORDCNT (205)   /* temporary test that parsing works */

/******************* EDIT **********************/

#define EDIT_CUT                (206)
#define EDIT_CUTEND             (207)
#define EDIT_ZCUT               (208)
#define EDIT_EXCISE             (209)
#define EDIT_EXCISEMANY (210)
#define EDIT_INSERT             (211)
#define EDIT_INSERTSIL  (212)
#define EDIT_JOIN               (213)

        /* HOUSEKEEP */
#define HOUSE_COPY              (214)
#define HOUSE_CHANS             (215)
#define HOUSE_EXTRACT   (216)
#define HOUSE_SPEC              (217)
#define HOUSE_BUNDLE    (218)
#define HOUSE_SORT              (219)
#define HOUSE_BAKUP             (220)
#define HOUSE_RECOVER   (221)
#define HOUSE_DISK              (222)

        /* SNDINFO */
#define INFO_PROPS              (223)
#define INFO_SFLEN              (224)
#define INFO_TIMELIST   (225)
#define INFO_LOUDLIST   (297)
#define INFO_TIMESUM    (226)
#define INFO_TIMEDIFF   (227)
#define INFO_SAMPTOTIME (228)
#define INFO_TIMETOSAMP (229)
#define INFO_MAXSAMP    (230)
#define INFO_LOUDCHAN   (231)
#define INFO_FINDHOLE   (232)
#define INFO_DIFF               (233)
#define INFO_CDIFF              (234)
#define INFO_PRNTSND    (235)
#define INFO_MUSUNITS   (236)

#define SYNTH_WAVE              (237)
#define SYNTH_NOISE             (238)
#define SYNTH_SIL               (239)

#define UTILS_GETCOL    (240)
#define UTILS_PUTCOL    (241)
#define UTILS_JOINCOL   (242)
#define UTILS_COLMATHS  (243)
#define UTILS_COLMUSIC  (244)
#define UTILS_COLRAND   (245)
#define UTILS_COLLIST   (246)
#define UTILS_COLGEN    (247)

#define FREEZE2                 (248)
#define HOUSE_DEL               (249)
#define UTILS_VECTORS   (250)

/******************* EDIT **********************/
#define INSERTSIL_MANY  (251)
#define RANDCUTS                (252)
#define RANDCHUNKS              (253)
/******************* MODIFY **********************/
#define SIN_TAB                 (254)
/******************* EXTEND (?) **********************/
#define ACC_STREAM              (255)
/******************* HFPERM **********************/
#define HF_PERM1                (289)
#define HF_PERM2                (290)
#define DEL_PERM                (291)
#define DEL_PERM2               (292)
/******************* SYNTH **********************/
#define SYNTH_SPEC              (293)
/******************* DISTORT **********************/
#define DISTORT_OVERLOAD (294)
/******************* EDIT **********************/
#define TWIXT                   (295)
#define SPHINX                  (296)

/* TW NEW */
/******************* 2002-3 **********************/
#define P_SYNTH         (298)
#define P_INSERT        (299)
#define P_PTOSIL        (300)
#define P_NTOSIL        (301)
#define P_SINSERT       (302)
#define ANALENV         (303)
#define MAKE2           (304)
#define P_VOWELS        (305)
#define HOUSE_DUMP              (306)
#define HOUSE_GATE              (307)
#define MIX_ON_GRID             (308)
#define P_GEN           (309)
#define P_INTERP        (310)
#define AUTOMIX                 (311)
#define EDIT_CUTMANY    (312)
#define STACK                   (313)
#define VFILT                   (314)
#define ENV_PROPOR              (315)
#define SCALED_PAN              (316)
#define MIXMANY                 (317)
#define DISTORT_PULSED  (318)    /* pulse-train */
#define NOISE_SUPRESS   (319)
#define TIME_GRID               (320)
#define SEQUENCER               (321)
#define CONVOLVE                (322)
#define BAKTOBAK                (323)
#define ADDTOMIX                (324)
#define EDIT_INSERT2    (325)
#define MIX_PAN                 (326)
#define SHUDDER                 (327)
#define MIX_AT_STEP             (328)
#define FIND_PANPOS             (329)
#define CLICK                   (330)
#define DOUBLETS                (331)
#define SYLLABS                 (332)
#define JOIN_SEQ                (333)
#define MAKE_VFILT              (334)
#define BATCH_EXPAND    (335)
#define MIX_MODEL               (336)
#define CYCINBETWEEN    (337)
#define JOIN_SEQDYN             (338)
#define ITERATE_EXTEND  (339)
#define DISTORT_RPTFL   (340)
#define TOPNTAIL_CLICKS (341)
#define P_BINTOBRK              (342)
#define ENVSYN                  (343)
#define SEQUENCER2              (344)
#define RRRR_EXTEND             (345)
#define HOUSE_GATE2             (346)
#define GRAIN_ASSESS    (347)
#define FLTBANKV2               (348)
#define DISTORT_RPT2    (349)
#define ZCROSS_RATIO    (350)
#define SSSS_EXTEND             (351)
#define GREV                    (352)
#define MAX_TEMP_PROCESS_NO     (257)

#define MAX_PROCESS_NO  (352)
