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



/******************************** TEXTURE ******************************/

/* CONSTANTS FOR TEXTURE */
#define TEXTURE_MIN_DUR   (0.01)
#define TEXTURE_MAX_DUR   (1.0)
#define MAX_PACKTIME      (60.0)
#define DENSITY_DEFAULT   (0.1)
#define DEFAULT_SKIP      (0.25)
#define MAX_SCAT_TEXTURE  (10.0)
#define EIGHT_8VA_UP      (96.0)
#define EIGHT_8VA_DOWN    (-96.0)
#define DEFAULT_TEXDUR    (4.0)
#define DEFAULT_TEXDENS   (10.0)
#define DEFAULT_TEXSCAT   (0.5)
#define MIN_MULT                  (0.01)
#define MAX_MULT                  (100.0)
#define TEXTURE_SPLICELEN (15.0)        /* 15 MS */
#define TEXTURE_SAFETY    (1.0)         /* 1 MS: min snd-dur between splices */
#define TEXTURE_DEFAULT_DUR (10.0)
#define DEFAULT_PITCH     (60.0)        /* middle C */
#define DEFAULT_HF_GPRANGE        (8.0)
#define DEFAULT_GPSIZE    (4.0)
#define HALF_OCTAVE               (6.0)
#define TEX_CENTRE        (0.5)
#define TEX_LEFT                  (0.0)
#define TEX_RIGHT             (1.0)
#define MAX_SPREAD                (1.0)

#define MAX_GPRANGE               (128.0)

#define TEXTURE_SEPARATOR ('#') /* Character in notedata file to indicate next motif */

/************************* MAX ELEMENT-REPETITIONS ALLOWED IN PERMS *************************/

#define SPACE_REPETS   (2)      /* for param PM_SPACE   */
#define PITCH_REPETS   (1)      /* for param PM_PITCH   */
#define AMP_REPETS         (1)  /* for param PM_AMP             */
#define DUR_REPETS         (1)  /* for param PM_DUR             */
#define INSNO_REPETS   (2)      /* for param PM_INSNO   */
#define GPRANG_REPETS  (1)      /* for param PM_GPRANG  */
#define GPSIZE_REPETS  (1)      /* for param PM_GPSIZE  */
#define GPDENS_REPETS  (1)      /* for param PM_GPDENS  */
#define GPCNTR_REPETS  (2)      /* for param PM_GPCNTR  */
#define GPPICH_REPETS  (1)      /* for param PM_GPPICH  */
#define ORNPOS_REPETS  (2)      /* for param PM_ORNPOS  */
#define GPSPAC_REPETS  (1)      /* for param PM_GPSPAC  */
#define ORIENT_REPETS  (1)      /* for param PM_ORIENT  */
#define DECABV_REPETS  (1)      /* for param PM_DECABV  */
#define MULT_REPETS    (1)      /* for param PM_MULT    */
#define WHICH_REPETS   (1)      /* for param PM_WHICH   */
#define GPPICH2_REPETS (1)      /* for param PM_GPPICH2 */

/* CONSTANTS connected with these same permutation functions */

#define PM_SPACE        (0)             /* L/R decision on events   :                           Oldname NOUGHT   */
#define PM_PITCH        (1)             /* PITCH              of events :                               Oldname ONE              */
#define PM_AMP          (2)             /* AMPLITUDE      of events :                           Oldname TWO              */
#define PM_DUR          (3)             /* DURATION       of events :                           Oldname THREE    */
#define PM_INSNO        (4)             /* INSTR_NO       of events :                           Oldname FOUR     */
#define PM_GPRANG       (5)             /* RANGE                  of group  :                   Oldname FIVE     */
#define PM_GPSIZE       (6)             /* SIZE                   of group  :                   Oldname SIX              */
#define PM_GPDENS       (7)             /* DENSITY (notedur)  of group  :                       Oldname SEVEN    */
#define PM_GPCNTR       (8)             /* SELECT AMP CONTOUR of group  :                       Oldname EIGHT    */
#define PM_GPPICH       (9)             /* PITCH OF NOTE    in a group  :                       Oldname NINE     */
#define PM_ORNPOS       (10)    /* PRE- or POST-      ornament  :                       Oldname TEN              */
#define PM_GPSPAC       (11)    /* SPATIALISE NOTE  in a group  :                       Oldname ELEVEN   */
#define PM_ORIENT       (12)    /* SELECT RANGE ORIENTATION of decoration   Oldname TWELVE   */
#define PM_DECABV       (13)    /* SELECT ABOVE/BELOW of centred dec. notes Oldname THIRTEEN */
#define PM_MULT         (14)    /* MULTIPLIER for duration motifs/ornmnts   Oldname FOURTEEN */
#define PM_WHICH        (15)    /* Select which motif to use as m or ornmnt Oldname FIFTEEN  */
#define PM_GPPICH2      (16)    /* as 9 for other half of centred HF  group Oldname SIXTEEN  */

#define DECOR_FIRST             (0)
#define DECOR_HIGHEST   (1)
#define DECOR_EVERY             (2)

/************************* TEXTURE PARAMETERS ******************************/

/* SIMPLE_TEX           (0)                00000000 00000000 */
#define ISPRE_DECORORN  (1)             /* -------- -------1 */
#define ISPOST_DECORORN (2)             /* -------- ------1- */
#define IS_HS           (4)             /* -------- -----1-- */
#define ISMANY_HFLDS    (8)             /* -------- ----1--- */
#define IS_GROUPS       (16)    /* -------- ---1---- */
#define IS_DECOR        (32)    /* -------- --1----- */
#define IS_MOTIFS       (64)    /* -------- -1------ */
#define IS_ORNATE       (128)   /* -------- 1------- */
#define MOTIF_IN_HF             (256)   /* -------1 -------- */
#define ISTIMED                 (512)   /* ------1- -------- */
#define ISHARM          (1024)  /* -----1-- -------- */

#define GET_DECORNPOS   (3)             /* -------- ------11 */

#define IS_CLUMPED      (0xF0)  /* -------- 1111---- */
#define IS_DEC_OR_GRP   (0x30)  /* -------- --11---- */
#define IS_MTF_OR_GRP   (0x50)  /* -------- -1-1---- */
#define IS_ORN_OR_DEC   (0xA0)  /* -------- 1-1----- */
#define ORN_DEC_OR_TIMED (0x2A0)/* ------1- 1-1----- */
#define IS_ORNDEC_OR_MTF (0xE0) /* -------- 111----- */
#define IS_ORN_OR_MTF   (0xC0)  /* -------- 11------ */


extern  int texflag;            /* bitflag for type of texture */


#define MAXNAMECNT      (9)

/* AMPLITUDE TYPES SPECIFIED BY USER */

#define IS_MIXED                         (0)
#define IS_CRESC                         (1)
#define IS_FLAT                          (2)
#define IS_DECRESC                       (3)
#define IS_FLAT_AND_CRESC        (4)
#define IS_CRESC_AND_DECRESC (5)
#define IS_FLAT_AND_DECRESC      (6)
#define IS_DIRECTIONAL           (7)
#define IS_DIREC_OR_FLAT         (8)

/* AMPLITUDE TYPES RETURNED FROM GETAMPTYPE */

#define FLAT    (0)
#define CRESC   (1)
#define DECRESC (2)

#define IS_STILL                (0)
#define IS_SCATTER              (1)
#define IS_INWARD               (2)
#define IS_OUTWARD              (3)
#define IS_FOLLOWING    (4)
#define IS_CONTRARY             (5)
#define IS_DIRECTED(x)  ((x)==IS_FOLLOWING || (x)==IS_CONTRARY)

        /* INTERNAL FLAGS */
#define INTERNAL_FLAGS_CNT                (2) /* COUNT OF FLAGS BELOW */

#define IS_PRE           (7) /* flags decorations precede decd notes (default) POST */
#define DECCENTRE                (8) /* flags decorations etc are centred on tset-pitch  */




#define PERMCNT                 (17)    /* number of permutation options, see texture0.h */

/* For more readable code  WHICH_CHORDNOTE */
/* Externally (to user) FORCEHI flag selects HIGH (as against FIRST) note */
/* Internally that flag can have 3 vals (FIRST,HIGHEST,EVERY..NOTE) */
/* So the NEW, more informative, name is used internally */

/******* GLOBAL VALUES ****/

#define MIDITOP         (127.0)            /* Maximum pitch midi note     */
#define MIDIBOT         (1.0)              /* Minimum pitch midi note     */

#define BANDCNT         (5) /* no.of range-bands each param divided into, for randperms */
#define LAYERCNT        (8) /* for weighted bands, there are 8 equal layers
                                                        band 1 = layer 1:
                                                        band 2 = layers 2 & 3:
                                                        band 3 = layers 4 & 5:
                                                        band 4 = layers 6 & 7:
                                                        band 5 = layer 8:
                                                */
#define LOW_A           (6.875)            /* Frequency of A below MIDI 0 */


/* HOW DECORATION PITCHES CENTRE ON DECORATED LINE PITCHES */

#define DEC_CENTRED     (0)
#define DEC_ABOVE               (1)
#define DEC_BELOW               (2)
#define DEC_C_A                 (3)
#define DEC_C_B                 (4)
#define DEC_A_B                 (5)
#define DEC_C_A_B               (6)

#define TEXTURE_MAX_SKIP                (100.0)
#define TEXTURE_MAX_TGRID               (10000.0)
#define TEXTURE_MAX_PHGRID              (1000.0)
