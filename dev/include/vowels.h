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



#define VOWEL_EE        (1)
#define VOWEL_I         (2)
#define VOWEL_AI        (3)
#define VOWEL_E         (4)
#define VOWEL_A         (5)
#define VOWEL_AR        (6)
#define VOWEL_O         (7)
#define VOWEL_OR        (8)
#define VOWEL_U         (9)
#define VOWEL_XX        (10)
#define VOWEL_X         (11)
#define VOWEL_OO        (12)
#define VOWEL_OA        (13)
#define VOWEL_AII       (14)    /* scottish 'ai' as in educAted */
#define VOWEL_UU        (15)    /* scottish edUcated, or 'you' : near german Uber */
#define VOWEL_N         (16)    /* 'n' */
#define VOWEL_UI        (17)    /* scottish 'could' */
#define VOWEL_M         (18)    /* 'm' */
#define VOWEL_R         (19)    /* 'r' on neutral vowel */
#define VOWEL_TH        (20)    /* 'r' on neutral vowel */

/* heed  */
#define EE_FORMANT1     (300)
#define EE_FORMANT2     (2600)
#define EE_FORMANT3     (2800)
#define EE_F2ATTEN              (0.85)
#define EE_F3ATTEN              (0.85)

/* hid   */
#define I_FORMANT1              (360)
#define I_FORMANT2              (2100)
#define I_FORMANT3              (2300)
#define I_F2ATTEN               (1.0)
#define I_F3ATTEN               (1.0)

/* scottish educAted  */
#define AII_FORMANT1    (500)
#define AII_FORMANT2    (2250)
#define AII_FORMANT3    (2450)
#define AII_F2ATTEN     (.92)
#define AII_F3ATTEN     (0.25)

/* laid  */
#define AI_FORMANT1     (465)
#define AI_FORMANT2     (2250)
#define AI_FORMANT3     (2450)
#define AI_F2ATTEN              (.92)
#define AI_F3ATTEN              (0.25)

/* head  */
#define E_FORMANT1              (570)
#define E_FORMANT2              (1970)
#define E_FORMANT3              (2450)
#define E_F2ATTEN               (1.0)
#define E_F3ATTEN               (0.75)

/* had   */
#define A_FORMANT1              (750)
#define A_FORMANT2              (1550)
#define A_FORMANT3              (2450)
#define A_F2ATTEN               (1.0)
#define A_F3ATTEN               (0.85)

/* hard  */
#define AR_FORMANT1     (680)
#define AR_FORMANT2     (1100)
#define AR_FORMANT3     (2450)
#define AR_F2ATTEN              (1.0)
#define AR_F3ATTEN              (0.92)

/* hod */
#define O_FORMANT1              (600)
#define O_FORMANT2              (900)
#define O_FORMANT3              (2450)
#define O_F2ATTEN               (1.0)
#define O_F3ATTEN               (0.5)

/* horde */
#define OR_FORMANT1     (450)
#define OR_FORMANT2     (860)
#define OR_FORMANT3     (2300)
#define OR_F2ATTEN              (1.0)
#define OR_F3ATTEN              (0.82)

/* coke (North England) */
#define OA_FORMANT1     (410)
#define OA_FORMANT2     (810)
#define OA_FORMANT3     (2300)
#define OA_F2ATTEN              (1.0)
#define OA_F3ATTEN              (0.82)

/* hood : mud (North England) */
#define U_FORMANT1              (380)
#define U_FORMANT2              (850)
#define U_FORMANT3              (2100)
#define U_F2ATTEN               (1.0)
#define U_F3ATTEN               (0.6)

/* scottish 'you'  */
#define UU_FORMANT1     (380)
#define UU_FORMANT2     (850)
#define UU_FORMANT3     (2800)
#define UU_F2ATTEN              (0.85)
#define UU_F3ATTEN              (0.85)

/* scottish could  */
#define UI_FORMANT1     (380)
#define UI_FORMANT2     (1000)
#define UI_FORMANT3     (2450)
#define UI_F2ATTEN              (.92)
#define UI_F3ATTEN              (0.25)

/* mood */
#define OO_FORMANT1     (300)
#define OO_FORMANT2     (850)
#define OO_FORMANT3     (2100)
#define OO_F2ATTEN              (1.0)
#define OO_F3ATTEN              (0.6)

/* (hub : South England) */
#define XX_FORMANT1     (720)
#define XX_FORMANT2     (1240)
#define XX_FORMANT3     (2300)
#define XX_F2ATTEN              (0.85)
#define XX_F3ATTEN              (0.3)

/* herd  */
#define X_FORMANT1              (580)
#define X_FORMANT2              (1380)
#define X_FORMANT3              (2450)
#define X_F2ATTEN               (1.0)
#define X_F3ATTEN               (0.5)

/* n  */
#define N_FORMANT1              (580)
#define N_FORMANT2              (1380)
#define N_FORMANT3              (2625)
#define N_F2ATTEN               (0.0)
#define N_F3ATTEN               (0.1)

/* m  */
#define M_FORMANT1              (580)
#define M_FORMANT2              (850)
#define M_FORMANT3              (2100)
#define M_F2ATTEN               (0.0)
#define M_F3ATTEN               (0.0)

/* r  */
#define R_FORMANT1              (880)
#define R_FORMANT2              (2100)
#define R_FORMANT3              (2450)
#define R_F2ATTEN               (0.2)
#define R_F3ATTEN               (0.0)

/* th  */
#define TH_FORMANT1     (400)
#define TH_FORMANT2     (1000) /* 880 */
#define TH_FORMANT3     (2450)
#define TH_F2ATTEN              (0.3) /* 0.3 */
#define TH_F3ATTEN              (0.2)
