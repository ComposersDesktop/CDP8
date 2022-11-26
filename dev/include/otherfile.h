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



Type of further main files required:
3rd item sent back from CDPARAMS......

#define FILES_NONE                                      (0)
#define FILES_SND                                       (1)
#define FILES_SND_MONO                          (2)
#define FILES_SND_SAME_CHANS            (3)
#define FILES_SND_MONO_ANY_SRATE        (4)
#define FILES_SND_ANY_SRATE                     (5)
#define FILES_ANAL                                      (6)
#define FILES_A_AND_PCH                         (7)
#define FILES_A_AND_TRNS                        (8)
#define FILES_A_AND_FMNT                        (9)
#define FILES_P_AND_FMNT                        (10)
#define FILES_ENV                                       (11)
#define FILES_BRK                                       (12)
#define FILES_DB_BRK                            (13)
#define FILES_TRNS_OR_BRK_TRNS          (14)
#define FILES_PCH_OR_BRK_PCH            (15)


Filetype data (originally sent from CDPARSE)
which you have stored for each file on the user interface.

#define SNDFILE                  (10)
#define ANALFILE                 (11)
#define PITCHFILE                (12)
#define TRANSPOSFILE     (13)
#define FORMANTFILE              (14)
#define ENVFILE                  (15)
#define TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST            (16)
#define TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST                                        (17)
#define TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST            (18)
#define TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST                                        (19)
#define TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST         (20)
#define TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST                                     (21)
#define NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST                                        (22)
#define NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST                                                            (23)
#define DB_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST                                           (24)
#define DB_BRKFILE_OR_NUMLIST_OR_WORDLIST                                                                       (25)
#define PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST                                        (26)
#define PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST                                                            (27)
#define UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST                                     (28)
#define UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST                                                         (29)
#define NUMLIST_OR_LINELIST_OR_WORDLIST                                                                         (30)
#define NUMLIST_OR_WORDLIST                                                                                                     (31)
#define SNDLIST_OR_SYNCLIST_LINELIST_OR_WORDLIST                                                        (32)
#define SNDLIST_OR_SYNCLIST_OR_WORDLIST                                                                         (33)
#define SNDLIST_OR_LINELIST_OR_WORDLIST                                                                         (34)
#define SNDLIST_OR_WORDLIST                                                                                                     (35)
#define MIXLIST_OR_LINELIST_OR_WORDLIST                                                                         (36)
#define MIXLIST_OR_WORDLIST                                                                                                     (37)
#define SYNCLIST_OR_LINELIST_OR_WORDLIST                                                                        (38)
#define SYNCLIST_OR_WORDLIST                                                                                            (39)
#define LINELIST_OR_WORDLIST                                                                                            (40)
#define WORDLIST                                                                                                                        (41)


HOW THIS WORKS.........
-----------------------

        case(FILES_NONE):
                break;
        case(FILES_SND):
                filetype of further file(s) = SNDFILE;
                srate ==
                break;
        case(FILES_SND_MONO):
                filetype of further file(s) = SNDFILE;
                srate ==
                channels = 1;
                break;
        case(FILES_SND_SAME_CHANS):
                filetype of further file(s) = SNDFILE;
                srate ==
                channels ==
                break;
        case(FILES_SND_MONO_ANY_SRATE):
                filetype of further file(s) = SNDFILE;
                channels = 1;
                break;
        case(FILES_SND_ANY_SRATE):
                filetype of further file(s) = SNDFILE;
                break;
        case(FILES_ANAL):
                filetype of further file(s) = ANALFILE;
                srate ==
                channels ==
                arate  flteq()
                origstype ==
                origrate ==
                Mlen ==
                Dfac ==
                break;
        case(FILES_A_AND_PCH):
                filetype of further file(s) = PITCHFILE;
                srate ==
                channels(file1) == origchans(file2)     <<<----- NB:!!!!***
                arate  flteq()
                origstype ==
                origrate ==
                Mlen ==
                Dfac ==
                break;
        case(FILES_A_AND_TRNS):
                filetype of further file(s) = TRANSPOSFILE;
                srate ==
                channels(file1) == origchans(file2)     <<<----- NB:!!!!***
                arate  flteq()
                origstype ==
                origrate ==
                Mlen ==
                Dfac ==
                break;
        case(FILES_A_AND_FMNT):
                filetype of further file(s) = FORMANTFILE;
                srate ==
                channels(file1) == origchans(file2)     <<<----- NB:!!!!***
                arate  flteq()
                origstype ==
                origrate ==
                Mlen ==
                Dfac ==
                break;
        case(FILES_P_AND_FMNT):
                filetype of further file(s) = FORMANTFILE;
                srate ==
                origchans(file1) == origchans(file2)    <<<----- NB: DIFFERENT AGAIN !!!!***
                arate  flteq()
                origstype ==
                origrate ==
                Mlen ==
                Dfac ==
                break;
        case(FILES_ENV):
                filetype of further file(s) = ENVFILE;
                break;
        case(FILES_BRK):
                filetype of further file(s) =
                                   TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST
                                || NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST;
                break;
        case(FILES_DB_BRK):
                filetype of further file(s) = DB_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || DB_BRKFILE_OR_NUMLIST_OR_WORDLIST;
                break;
        case(FILES_TRNS_OR_BRK_TRNS):
                filetype of further file(s) =
                                   TRANSPOSFILE
                                || TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST
                                || TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST
                                || TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST
                                || PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST;
                break;
        case(FILES_PCH_OR_BRK_PCH):
                filetype of further file(s) =
                                   PITCHFILE
                                || TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST
                                || TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST
                                || TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST
                                || TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST
                                || PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST;
                break;

        }

/**************************** FLTEQ *******************************/

#define FLTERR  (0.000002)
#define FALSE   (0)
#define TRUE    (1)

int flteq(double f1,double f2)
{
        double upperbnd, lowerbnd;
        upperbnd = f2 + FLTERR;
        lowerbnd = f2 - FLTERR;
        if((f1>upperbnd) || (f1<lowerbnd))
                return(FALSE);
        return(TRUE);
}
