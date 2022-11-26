/*
 * Copyright (c) 1983-2020 Trevor Wishart and Composers Desktop Project Ltd
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



/* floatsam version */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <processno.h>
#include <logic.h>
#include <modeno.h>
#include <special.h>
#include <cdparams.h>
#include <globcon.h>

static int  set_param_data(aplptr ap, int special_data,int maxparamcnt,int paramcnt,char *paramlist);
static int  set_vflgs(aplptr ap,char *optflags,int optcnt,char *optlist,
                      char *varflags,int vflagcnt, int vparamcnt,char *varlist);

/****************************** SET_LEGAL_PARAM_STRUCTURE *********************************/

int set_legal_param_structure(int process,int mode, aplptr ap)
{
    /*  |                             |m| |        | */
    /*  |                             |a| |        | */
    /*  |                             |x| |        | */
    /*  |                             |p|p|        | */
    /*  |                             |a|a|        | */
    /*  |  special-data               |r|r| param  | */
    /*  |                             |a|a|  list  | */
    /*  |                             |m|m|        | */
    /*  |                             |c|c|        | */
    /*  |                             |n|n|        | */
    /*  |                             |t|t|        | */
    
    switch(process) {
        case(ACCU):     return set_param_data(ap,0                        ,0,0,""      );
        case(ALT):      return set_param_data(ap,0                        ,0,0,""      );
        case(ARPE):     return set_param_data(ap,0                        ,2,2,"iD"    );
        case(AVRG):     return set_param_data(ap,0                        ,1,1,"I"     );
        case(BARE):     return set_param_data(ap,0                        ,0,0,""      );
        case(BLTR):     return set_param_data(ap,0                        ,2,2,"II"    );
        case(BLUR):     return set_param_data(ap,0                        ,1,1,"I"     );
        case(BRIDGE):   return set_param_data(ap,0                        ,0,0,""      );
        case(CHANNEL):  return set_param_data(ap,0                        ,1,1,"d"     );
        case(CHORD):    return set_param_data(ap,SEMIT_TRANSPOS_SET       ,0,0,""      );
        case(CHORUS):
            switch(mode) {
                case(CH_AMP):
                    return set_param_data(ap,0                        ,2,1,"D0"    );
                case(CH_FRQ):
                case(CH_FRQ_UP):
                case(CH_FRQ_DN):
                    return set_param_data(ap,0                        ,2,1,"0D"    );
                case(CH_AMP_FRQ):
                case(CH_AMP_FRQ_UP):
                case(CH_AMP_FRQ_DN):
                    return set_param_data(ap,0                        ,2,2,"DD"    );
            }
            break;
        case(CLEAN):
            switch(mode) {
                case(FROMTIME):
                case(ANYWHERE):
                case(FILTERING):
                    return set_param_data(ap,0                        ,1,1,"d"     );
                case(COMPARING):
                    return set_param_data(ap,0                        ,1,0,"0"     );
            }
            break;
        case(CROSS):    return set_param_data(ap,0                        ,0,0,""      );
        case(CUT):      return set_param_data(ap,0                        ,2,2,"dd"    );
        case(DIFF):     return set_param_data(ap,0                        ,0,0,""      );
        case(DRUNK):    return set_param_data(ap,0                        ,3,3,"idd"   );
        case(EXAG):     return set_param_data(ap,0                        ,1,1,"D"     );
        case(FILT):
            switch(mode) {
                case(F_HI):
                case(F_HI_NORM):
                case(F_LO):
                case(F_LO_NORM):
                    return set_param_data(ap,0                        ,4,2,"D0D0"  );
                case(F_HI_GAIN):
                case(F_LO_GAIN):
                    return set_param_data(ap,0                        ,4,3,"D0Dd"  );
                case(F_BND):
                case(F_BND_NORM):
                case(F_NOTCH):
                case(F_NOTCH_NORM):
                    return set_param_data(ap,0                        ,4,3,"DDD0"  );
                case(F_BAND_GAIN):
                case(F_NOTCH_GAIN):
                    return set_param_data(ap,0                        ,4,4,"DDDd"  );
            }
            break;
        case(FMNTSEE):  return set_param_data(ap,0                        ,0,0,""      );
        case(FOCUS):    return set_param_data(ap,0                        ,2,2,"iD"    );
        case(FOLD):     return set_param_data(ap,0                        ,2,2,"DD"    );
        case(FORM):     return set_param_data(ap,0                        ,0,0,""      );
        case(FORMANTS): return set_param_data(ap,0                        ,0,0,""      );
        case(FORMSEE):  return set_param_data(ap,0                        ,0,0,""      );
        case(FREEZE):   return set_param_data(ap,FREEZE_DATA              ,0,0,""      );
        case(FREEZE2):  return set_param_data(ap,FREEZE2_DATA             ,0,0,""      );
        case(FREQUENCY):return set_param_data(ap,0                        ,1,1,"i"     );
        case(GAIN):     return set_param_data(ap,0                        ,1,1,"D"     );
        case(GLIDE):    return set_param_data(ap,0                        ,1,1,"d"     );
        case(GLIS):
            switch(mode) {
                case(SHEPARD):
                case(SELFGLIS):
                    return set_param_data(ap,0                        ,2,1,"D0"    );
                case(INHARMONIC):
                    return set_param_data(ap,0                        ,2,2,"Dd"    );
            }
            break;
        case(GRAB):     return set_param_data(ap,0                        ,1,1,"d"     );
        case(GREQ):
            switch(mode) {
                case(GR_ONEBAND):
                    return set_param_data(ap,FILTER_FRQS              ,0,0,""      );
                case(GR_MULTIBAND):
                    return set_param_data(ap,FILTER_BWS_AND_FRQS      ,0,0,""      );
            }
            break;
        case(INVERT):   return set_param_data(ap,0                        ,0,0,""      );
        case(LEAF):     return set_param_data(ap,0                        ,1,1,"i"     );
        case(LEVEL):    return set_param_data(ap,0                        ,0,0,""      );
        case(MAGNIFY):  return set_param_data(ap,0                        ,2,2,"dd"    );
        case(MAKE):     return set_param_data(ap,0                        ,0,0,""      );
        case(MAX):      return set_param_data(ap,0                        ,0,0,""      );
        case(MEAN):     return set_param_data(ap,0                        ,0,0,""      );
        case(MORPH):    return set_param_data(ap,0                        ,6,6,"dddddd");
        case(NOISE):    return set_param_data(ap,0                        ,1,1,"D"     );
        case(OCT):
            switch(mode) {
                case(OCT_UP):
                case(OCT_DN):
                    return set_param_data(ap,0                        ,2,1,"i0"    );
                case(OCT_DN_BASS):
                    return set_param_data(ap,0                        ,2,2,"iD"    );
            }
            break;
        case(OCTVU):    return set_param_data(ap,0                        ,1,1,"d"     );
        case(P_APPROX): return set_param_data(ap,0                        ,0,0,""      );
        case(P_CUT):
            switch(mode) {
                case(PCUT_START_ONLY):
                    return set_param_data(ap,0                        ,2,1,"D0"    );
                case(PCUT_END_ONLY):
                    return set_param_data(ap,0                        ,2,1,"0D"    );
                case(PCUT_BOTH):
                    return set_param_data(ap,0                        ,2,2,"DD"    );
            }
            break;
        case(P_EXAG):
            switch(mode) {
                case(RANGE_ONLY_TO_P):
                case(RANGE_ONLY_TO_T):
                    return set_param_data(ap,0                        ,3,2,"DD0"   );
                case(CONTOUR_ONLY_TO_P):
                case(CONTOUR_ONLY_TO_T):
                    return set_param_data(ap,0                        ,3,2,"D0D"   );
                case(R_AND_C_TO_P):
                case(R_AND_C_TO_T):
                    return set_param_data(ap,0                        ,3,3,"DDD"   );
            }
            break;
        case(P_FIX):    return set_param_data(ap,0                        ,0,0,""      );
        case(P_HEAR):   return set_param_data(ap,0                        ,0,0,""      );
        case(P_INFO):   return set_param_data(ap,0                        ,0,0,""      );
        case(P_INVERT): return set_param_data(ap,INTERVAL_MAPPING         ,0,0,""      );
        case(P_QUANTISE):   return set_param_data(ap,PITCHQUANTISE_SET    ,0,0,""      );
        case(P_RANDOMISE):  return set_param_data(ap,0                    ,2,2,"DD"    );
        case(P_SEE):
            switch(mode) {
                case(SEE_PITCH):
                    return set_param_data(ap,0                        ,1,1,"d"     );
                case(SEE_TRANSPOS):
                    return set_param_data(ap,0                        ,1,0,"0"     );
            }
            break;
        case(P_SMOOTH):     return set_param_data(ap,0                    ,1,1,"D"     );
            //TW NEW
        case(P_SYNTH):      return set_param_data(ap,PITCH_SPECTRUM       ,0,0,""      );
        case(P_VOWELS):     return set_param_data(ap,PITCH_VOWELS         ,5,5,"dddDD" );
        case(VFILT):        return set_param_data(ap,PITCH_VOWELS         ,4,4,"dddd"  );
        case(P_GEN):        return set_param_data(ap,PITCH_CREATE         ,1,1,"i"     );
        case(P_INSERT):     return set_param_data(ap,ZERO_INSERTTIMES     ,0,0,""      );
        case(P_SINSERT):    return set_param_data(ap,ZERO_INSERTTIMES     ,0,0,""      );
        case(P_PTOSIL):     return set_param_data(ap,0                    ,0,0,""      );
        case(P_NTOSIL):     return set_param_data(ap,0                    ,0,0,""      );
        case(ANALENV):      return set_param_data(ap,0                    ,0,0,""      );
        case(P_BINTOBRK):   return set_param_data(ap,0                    ,0,0,""      );
        case(MAKE2):        return set_param_data(ap,0                    ,0,0,""      );
        case(P_INTERP):     return set_param_data(ap,0                    ,0,0,""      );
            
        case(P_TRANSPOSE):  return set_param_data(ap,0                    ,1,1,"d"     );
        case(P_VIBRATO):    return set_param_data(ap,0                    ,2,2,"DD"    );
        case(P_WRITE):  return set_param_data(ap,0                        ,0,0,""      );
        case(P_ZEROS):  return set_param_data(ap,0                        ,0,0,""      );
        case(PEAK):     return set_param_data(ap,0                        ,0,0,""      );
        case(PICK):
            switch(mode) {
                case(PIK_HARMS):
                case(PIK_OCTS):
                case(PIK_ODD_HARMS):
                    return set_param_data(ap,0                        ,2,1,"d0"    );
                case(PIK_LINEAR):
                case(PIK_DISPLACED_HARMS):
                    return set_param_data(ap,0                        ,2,2,"dd"    );
            }
            break;
        case(PITCH):
            if(!sloom) {
                switch(mode) {
                    case(PICH_TO_BIN):
                        return set_param_data(ap,OUT_PFILE              ,0,0,""      );
                    case(PICH_TO_BRK):
                        return set_param_data(ap,OUT_PBRKFILE           ,0,0,""      );
                }
            } else {
                return set_param_data(ap,0                                  ,0,0,""      );
            }
            break;
        case(PLUCK):    return set_param_data(ap,0                        ,1,1,"D"     );
        case(PRINT):    return set_param_data(ap,0                        ,1,1,"d"     );
        case(REPITCH):  return set_param_data(ap,0                        ,0,0,""      );
        case(REPITCHB): return set_param_data(ap,0                        ,0,0,""      );
        case(REPORT):   return set_param_data(ap,0                        ,1,1,"i"     );
        case(SCAT):     return set_param_data(ap,0                        ,1,1,"I"     );
        case(SHIFT):
            switch(mode) {
                case(SHIFT_ALL):
                    return set_param_data(ap,0                        ,3,1,"D00"   );
                case(SHIFT_ABOVE):
                case(SHIFT_BELOW):
                    return set_param_data(ap,0                        ,3,2,"DD0"   );
                case(SHIFT_BETWEEN):
                case(SHIFT_OUTSIDE):
                    return set_param_data(ap,0                        ,3,3,"DDD"   );
            }
            break;
        case(SHIFTP):
            switch(mode) {
                case(P_OCT_UP):
                case(P_OCT_DN):
                case(P_OCT_UP_AND_DN):
                    return set_param_data(ap,0                        ,3,1,"D00"   );
                case(P_SHFT_UP):
                case(P_SHFT_DN):
                    return set_param_data(ap,0                        ,3,2,"DD0"   );
                case(P_SHFT_UP_AND_DN):
                    return set_param_data(ap,0                        ,3,3,"DDD"   );
            }
            break;
        case(SHUFFLE):  return set_param_data(ap,SHUFFLE_DATA             ,1,1,"i"     );
        case(SPLIT):    return set_param_data(ap,SPECSPLI_DATA            ,0,0,""      );
        case(SPREAD):   return set_param_data(ap,0                        ,0,0,""      );
        case(STEP):     return set_param_data(ap,0                        ,1,1,"d"     );
        case(STRETCH):  return set_param_data(ap,0                        ,3,3,"ddd"   );
        case(SUM):      return set_param_data(ap,0                        ,0,0,""      );
        case(SUPR):     return set_param_data(ap,0                        ,1,1,"I"     );
        case(S_TRACE):
            switch(mode) {
                case(TRC_ALL):
                    return set_param_data(ap,0                        ,3,1,"I00"   );
                case(TRC_ABOVE):
                    return set_param_data(ap,0                        ,3,2,"ID0"   );
                case(TRC_BELOW):
                    return set_param_data(ap,0                        ,3,2,"I0D"   );
                case(TRC_BETWEEN):
                    return set_param_data(ap,0                        ,3,3,"IDD"   );
            }
            break;
        case(TRACK):
            switch(mode) {
                case(TRK_TO_BIN):
                    return set_param_data(ap,OUT_PFILE                ,1,1,"d"     );
                case(TRK_TO_BRK):
                    return set_param_data(ap,OUT_PBRKFILE             ,1,1,"d"     );
            }
            break;
        case(TRNSF):
            switch(mode) {
                case(TRNS_RATIO):
                    return set_param_data(ap,TRANSPOS_RATIO_OR_CONSTANT ,0,0,""      );
                case(TRNS_OCT):
                    return set_param_data(ap,TRANSPOS_OCTAVE_OR_CONSTANT,0,0,""      );
                case(TRNS_SEMIT):
                    return set_param_data(ap,TRANSPOS_SEMIT_OR_CONSTANT ,0,0,""      );
                case(TRNS_BIN):
                    return set_param_data(ap,0                          ,0,0,""      );
            }
            break;
        case(TRNSP):
            switch(mode) {
                case(TRNS_RATIO):
                    return set_param_data(ap,TRANSPOS_RATIO_OR_CONSTANT ,0,0,""      );
                case(TRNS_OCT):
                    return set_param_data(ap,TRANSPOS_OCTAVE_OR_CONSTANT,0,0,""      );
                case(TRNS_SEMIT):
                    return set_param_data(ap,TRANSPOS_SEMIT_OR_CONSTANT ,0,0,""      );
                case(TRNS_BIN):
                    return set_param_data(ap,0                          ,0,0,""      );
            }
            break;
        case(TSTRETCH): return set_param_data(ap,0                        ,1,1,"D"     );
        case(TUNE):
            switch(mode) {
                case(TUNE_FRQ):
                    return set_param_data(ap,FRQ_OR_FRQSET            ,0,0,""      );
                case(TUNE_MIDI):
                    return set_param_data(ap,PITCH_OR_PITCHSET        ,0,0,""      );
            }
            break;
        case(VOCODE):   return set_param_data(ap,0                        ,0,0,""      );
        case(WARP):     return set_param_data(ap,0                        ,0,0,""      );
        case(WAVER):
            switch(mode) {
                case(WAVER_STANDARD):
                    return set_param_data(ap,0                        ,4,3,"DDd0"  );
                case(WAVER_SPECIFIED):
                    return set_param_data(ap,0                        ,4,4,"DDdd"  );
            }
            break;
        case(WEAVE):    return set_param_data(ap,WEAVE_DATA               ,0,0,""      );
        case(WINDOWCNT):return set_param_data(ap,0                        ,0,0,""      );
        case(LIMIT):    return set_param_data(ap,0                        ,1,1,"D"     );
        case(MULTRANS): return set_param_data(ap,SEMIT_TRANSPOS_SET       ,0,0,""      );
            /*  |                             |m| |        | */
            /*  |                             |a| |        | */
            /*  |                             |x| |        | */
            /*  |                             |p|p|        | */
            /*  |                             |a|a|        | */
            /******** GROUCHO ********/       /*  |  special-data             |r|r| param  | */
            /*  |                         |a|a|  list  | */
            /*  |                         |m|m|        | */
            /*  |                         |c|c|        | */
            /*  |                         |n|n|        | */
            /*  |                         |t|t|        | */
        case(DISTORT):
            switch(mode) {
                case(DISTORT_EXAGG):
                    return set_param_data(ap,0                        ,1,1,"D"    );
                default:      return set_param_data(ap,0                          ,1,0,"0"    );
            }
            break;
        case(DISTORT_ENV):
            switch(mode) {
                case(DISTORTE_USERDEF):     /* 1 param + 0 variants: 2 INACTIVEs force internalparams to start at param[3] */
                    return set_param_data(ap,DISTORT_ENVELOPE         ,3,1,"I00"  );
                case(DISTORTE_TROFFED):     /* 2 params + 1 variant trof = param[1]: internals start at param[3] */
                    return set_param_data(ap,0                        ,2,2,"ID"   );
                    /* 1 param + 2 variants trof = param[1]: internals start at param[3] */
                default:       return set_param_data(ap,0                         ,1,1,"I"    );
            }
            break;
        case(DISTORT_AVG): return set_param_data(ap,0                         ,1,1,"I"    );
        case(DISTORT_OMT): return set_param_data(ap,0                         ,2,2,"Ii"   );
            
        case(DISTORT_MLT): return set_param_data(ap,0                         ,1,1,"I"    );
        case(DISTORT_DIV): return set_param_data(ap,0                         ,1,1,"I"    );
        case(DISTORT_HRM): return set_param_data(ap,HARMONIC_DISTORT          ,0,0,""     );
        case(DISTORT_FRC): return set_param_data(ap,0                         ,2,2,"ID"   );
        case(DISTORT_REV): return set_param_data(ap,0                         ,1,1,"I"    );
        case(DISTORT_SHUF):return set_param_data(ap,SHUFFLE_DATA              ,0,0,""     );
        case(DISTORT_RPTFL):
        case(DISTORT_RPT2):
        case(DISTORT_RPT): return set_param_data(ap,0                         ,1,1,"I"    );
        case(DISTORT_INTP):return set_param_data(ap,0                         ,1,1,"I"    );
        case(DISTORT_DEL): return set_param_data(ap,0                         ,1,1,"I"    );
        case(DISTORT_RPL): return set_param_data(ap,0                         ,1,1,"I"    );
        case(DISTORT_TEL): return set_param_data(ap,0                         ,1,1,"I"    );
        case(DISTORT_FLT):
            switch(mode) {
                case(DISTFLT_HIPASS):
                    return set_param_data(ap,0                        ,2,1,"D0"   );
                case(DISTFLT_LOPASS):
                    return set_param_data(ap,0                        ,2,1,"0D"   );
                case(DISTFLT_BANDPASS):
                    return set_param_data(ap,0                        ,2,2,"DD"   );
            }
            break;
        case(DISTORT_INT): return set_param_data(ap,0                         ,0,0,""     );
        case(DISTORT_CYCLECNT):
            return set_param_data(ap,0                        ,0,0,""     );
        case(DISTORT_PCH): return set_param_data(ap,0                         ,1,1,"D"    );
        case(DISTORT_OVERLOAD):
            switch(mode) {
                case(OVER_NOISE):
                    return set_param_data(ap,0                        ,3,2,"DD0"  );
                case(OVER_SINE):
                    return set_param_data(ap,0                        ,3,3,"DDD"  );
            }
            break;
        case(DISTORT_PULSED):
            switch(mode) {
                case(PULSE_IMP):  return set_param_data(ap,PULSE_ENVELOPE        ,9,8,"ddDddd0Dd");
                case(PULSE_SYN):  return set_param_data(ap,PULSE_ENVELOPE        ,9,9,"ddDddddDd");
                case(PULSE_SYNI): return set_param_data(ap,PULSE_ENVELOPE        ,9,9,"idDdddiDd");
            }
            break;
        case(ZIGZAG):
            switch(mode) {
                case(ZIGZAG_SELF):
                    return set_param_data(ap,0                        ,4,4,"dddd" );
                case(ZIGZAG_USER):
                    return set_param_data(ap,ZIGDATA                      ,4,0,"0000" );
                default:
                    sprintf(errstr,"Unknown mode (%d) : ZIGZAG: set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(LOOP):
            switch(mode) {
                case(LOOP_ALL):
                    return set_param_data(ap,0                        ,5,3,"00ddd");
                case(LOOP_OUTLEN):
                    return set_param_data(ap,0                        ,4,3,"d0dd" );
                case(LOOP_RPTS):
                    return set_param_data(ap,0                        ,4,3,"0idd" );
                default:
                    sprintf(errstr,"Unknown mode (%d) : LOOP: set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(SCRAMBLE):    return set_param_data(ap,0                         ,3,3,"ddd"  );
        case(ITERATE):
            switch(mode) {
                case(ITERATE_DUR):
                    return set_param_data(ap,0                        ,1,1,"d"    );
                case(ITERATE_REPEATS):
                    return set_param_data(ap,0                        ,1,1,"i"    );
                default:
                    sprintf(errstr,"Unknown mode (%d) : ITERATE: set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(ITERATE_EXTEND):
            switch(mode) {
                case(ITERATE_DUR):
                    return set_param_data(ap,0                        ,8,8,"dDDDDddd" );
                case(ITERATE_REPEATS):
                    return set_param_data(ap,0                        ,8,8,"iDDDDddd" );
                default:
                    sprintf(errstr,"Unknown mode (%d) : ITERATE_EXTEND: set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(DRUNKWALK):
            switch(mode) {
                case(TOTALLY_PISSED):
                    return set_param_data(ap,0                        ,7,5,"dDDDD00");
                case(HAS_SOBER_MOMENTS):
                    return set_param_data(ap,0                         ,7,7,"dDDDDII");
                default:
                    sprintf(errstr,"Unknown mode (%d) : DRUNKWALK: set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(SIMPLE_TEX):  return set_param_data(ap,TEX_NOTEDATA              ,26,13,"dDDDIIDDDDDDI0000000000000");
        case(GROUPS):      return set_param_data(ap,TEX_NOTEDATA              ,26,23,"dDDDIIDDDDDDDiDDiIIDDDD000");
        case(ORNATE): case(PREORNATE):  case(POSTORNATE):
            return set_param_data(ap,TEX_NOTEDATA             ,26,15,"dD00IIDDDD00DiDDi000000DD0");
        case(MOTIFS): case(MOTIFSIN):
            return set_param_data(ap,TEX_NOTEDATA             ,26,17,"dDDDIIDD00DDDiDDi000000DD0");
        case(DECORATED): case(PREDECOR): case(POSTDECOR):
            return set_param_data(ap,TEX_NOTEDATA             ,26,20,"dD00IIDDDD00DiDDiIIDDDD00i");
        case(TIMED):       return set_param_data(ap,TEX_NOTEDATA              ,26,10,"dD00IIDDDDDD00000000000000");
        case(TGROUPS):     return set_param_data(ap,TEX_NOTEDATA              ,26,21,"dD00IIDDDDDDDiDDiIIDDDD000");
        case(TMOTIFS): case(TMOTIFSIN):
            return set_param_data(ap,TEX_NOTEDATA             ,26,15,"dD00IIDD00DDDiDDi000000DD0");
        case(GRAIN_COUNT):     return set_param_data(ap,0                     ,2,0,"00"       );
        case(GRAIN_OMIT):      return set_param_data(ap,0                     ,2,2,"Ii"       );
        case(GRAIN_DUPLICATE): return set_param_data(ap,0                     ,2,1,"I0"       );
        case(GRAIN_REORDER):   return set_param_data(ap,GRAIN_REORDER_STRING  ,2,0,"00"       );
        case(GRAIN_REPITCH):   return set_param_data(ap,GRAIN_PITCH_RATIOS    ,2,0,"00"       );
        case(GRAIN_RERHYTHM):  return set_param_data(ap,GRAIN_TIME_RATIOS     ,2,0,"00"       );
        case(GRAIN_REMOTIF):   return set_param_data(ap,GRAIN_TWO_RATIOS      ,2,0,"00"       );
        case(GRAIN_TIMEWARP):  return set_param_data(ap,0                     ,2,1,"D0"       );
        case(GRAIN_REVERSE):   return set_param_data(ap,0                     ,2,0,"00"       );
        case(GRAIN_GET):       return set_param_data(ap,0                     ,2,0,"00"       );
        case(GRAIN_POSITION):  return set_param_data(ap,GRAIN_TIMINGS         ,2,1,"d0"       );
        case(GRAIN_ALIGN):     return set_param_data(ap,0                     ,2,2,"dD"       );
        case(ENV_CREATE):
            switch(mode) {
                case(ENV_ENVFILE_OUT): return set_param_data(ap,ENV_CREATEFILE    ,4,1,"d000"    );
                case(ENV_BRKFILE_OUT): return set_param_data(ap,ENV_CREATEFILE    ,4,0,"0000"    );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for ENV_CREATE in set_legal_param_structure()\n",mode);
                    return (PROGRAM_ERROR);
            }
            break;
        case(ENV_EXTRACT):     return set_param_data(ap,0                     ,4,1,"d000"     );
            
        case(ENV_WARPING):
        case(ENV_REPLOTTING):
            switch(mode) {
                case(ENV_NORMALISE):
                case(ENV_REVERSE):
                case(ENV_CEILING):
                    return set_param_data(ap,0                     ,4,1,"d000"     );
                    
                case(ENV_EXAGGERATING):
                case(ENV_ATTENUATING):
                case(ENV_LIFTING): return set_param_data(ap,0                     ,4,2,"dD00"     );
                    
                case(ENV_DUCKED):
                case(ENV_INVERTING):
                case(ENV_LIMITING):return set_param_data(ap,0                     ,4,3,"dDD0"     );
                    
                case(ENV_TSTRETCHING): return set_param_data(ap,0                 ,4,2,"dd00"     );
                case(ENV_FLATTENING):  return set_param_data(ap,0                 ,4,2,"dI00"     );
                case(ENV_GATING):      return set_param_data(ap,0                 ,4,3,"dD0i"     );
                case(ENV_CORRUGATING): return set_param_data(ap,0                 ,4,3,"dII0"     );
                case(ENV_EXPANDING):   return set_param_data(ap,0                 ,4,4,"dDDi"     );
                case(ENV_TRIGGERING):  return set_param_data(ap,ENV_TRIGGER_RAMP  ,4,4,"dDdd"     );
                case(ENV_PEAKCNT):     return set_param_data(ap,0                 ,4,2,"d0I0"     );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for ENV_WARPING or REPLOTTING in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(ENV_RESHAPING):
            switch(mode) {
                case(ENV_NORMALISE):
                case(ENV_REVERSE):
                case(ENV_CEILING):
                    return set_param_data(ap,0                     ,4,0,"0000"     );
                    
                case(ENV_EXAGGERATING):
                case(ENV_ATTENUATING):
                case(ENV_LIFTING): return set_param_data(ap,0                     ,4,1,"0D00"     );
                    
                case(ENV_DUCKED):
                case(ENV_INVERTING):
                case(ENV_LIMITING):return set_param_data(ap,0                     ,4,2,"0DD0"     );
                    
                case(ENV_TSTRETCHING): return set_param_data(ap,0                 ,4,1,"0d00"     );
                case(ENV_FLATTENING):  return set_param_data(ap,0                 ,4,1,"0I00"     );
                case(ENV_GATING):      return set_param_data(ap,0                 ,4,2,"0D0i"     );
                case(ENV_CORRUGATING): return set_param_data(ap,0                 ,4,2,"0II0"     );
                case(ENV_EXPANDING):   return set_param_data(ap,0                 ,4,3,"0DDi"     );
                case(ENV_TRIGGERING):  return set_param_data(ap,ENV_TRIGGER_RAMP  ,4,3,"0Ddd"     );
                case(ENV_PEAKCNT):     return set_param_data(ap,0                 ,4,1,"00I0"     );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for ENV_RESHAPING in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            //TW NEW
        case(ENV_PROPOR):           return set_param_data(ap,0                ,4,0,"0000"     );    break;
        case(ENV_IMPOSE):
            switch(mode) {
                case(ENV_ENVFILE_IN):
                case(ENV_DB_BRKFILE_IN):
                case(ENV_BRKFILE_IN): return set_param_data(ap,0                  ,4,0,"0000"     );
                case(ENV_SNDFILE_IN): return set_param_data(ap,0                  ,4,1,"d000"     );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for ENV_IMPOSE in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(ENV_REPLACE):
            switch(mode) {
                case(ENV_ENVFILE_IN):     return set_param_data(ap,0                  ,4,0,"0000"     );
                case(ENV_BRKFILE_IN):
                case(ENV_DB_BRKFILE_IN):
                case(ENV_SNDFILE_IN):     return set_param_data(ap,0                  ,4,1,"d000"     );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for ENV_REPLACE in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(ENV_DOVETAILING):
            switch(mode) {
                case(DOVE):        return set_param_data(ap,0                     ,4,4,"ddii"     );
                case(DOVEDBL):     return set_param_data(ap,0                     ,4,2,"dd00"     );
            }
            break;
        case(ENV_SWELL):       return set_param_data(ap,0                     ,4,2,"d0i0"     );
        case(ENV_ATTACK):
            switch(mode) {
                case(ENV_ATK_GATED): return set_param_data(ap,0                   ,4,4,"dddd"     );
                case(ENV_ATK_TIMED): return set_param_data(ap,0                   ,4,4,"dddd"     );
                case(ENV_ATK_XTIME): return set_param_data(ap,0                   ,4,4,"dddd"     );
                case(ENV_ATK_ATMAX): return set_param_data(ap,0                   ,4,3,"0ddd"     );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for ENV_ATTACK in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(ENV_CURTAILING):
            switch(mode) {
                case(ENV_START_AND_END):
                case(ENV_START_AND_DUR):    return set_param_data(ap,0               ,4,3,"ddi0"     );
                case(ENV_START_ONLY):       return set_param_data(ap,0               ,4,2,"d0i0"     );
                case(ENV_START_AND_END_D):
                case(ENV_START_AND_DUR_D):  return set_param_data(ap,0               ,4,2,"dd00"     );
                case(ENV_START_ONLY_D):     return set_param_data(ap,0               ,4,1,"d000"     );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for ENV_CURTAILING in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            
        case(ENV_ENVTOBRK):    return set_param_data(ap,0                     ,4,0,"0000"     );
        case(ENV_ENVTODBBRK):  return set_param_data(ap,0                     ,4,0,"0000"     );
        case(ENV_BRKTOENV):    return set_param_data(ap,0                     ,4,1,"d000"     );
        case(ENV_DBBRKTOENV):  return set_param_data(ap,0                     ,4,1,"d000"     );
        case(ENV_DBBRKTOBRK):  return set_param_data(ap,0                     ,4,0,"0000"     );
        case(ENV_BRKTODBBRK):  return set_param_data(ap,0                     ,4,0,"0000"     );
        case(ENV_PLUCK):       return set_param_data(ap,0                     ,4,2,"ii00"     );
        case(ENV_TREMOL):      return set_param_data(ap,0                     ,4,3,"DDD0"     );
            
        case(MIX):             return set_param_data(ap,0                     ,0,0,""         );
        case(MIXTWO):          return set_param_data(ap,0                     ,0,0,""         );
            //TW NEW
        case(MIXMANY):         return set_param_data(ap,0                     ,0,0,""         );
        case(MIXBALANCE):      return set_param_data(ap,0                     ,0,0,""         );
        case(MIXCROSS):        return set_param_data(ap,0                     ,0,0,""         );
        case(MIXINTERL):       return set_param_data(ap,0                     ,0,0,""         );
        case(MIXINBETWEEN):
            switch(mode) {
                case(INBI_COUNT):  return set_param_data(ap,0                     ,1,1,"i"        );
                case(INBI_RATIO):  return set_param_data(ap,INBTWN_RATIOS         ,1,0,"0"        );
                default:
                    sprintf(errstr,"Unknown mode (%d) for MIXINBETWEEN in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(MIXTEST):         return set_param_data(ap,0                     ,0,0,""         );
        case(MIXMAX):          return set_param_data(ap,0                     ,0,0,""         );
        case(MIXFORMAT):       return set_param_data(ap,0                     ,0,0,""         );
        case(MIXDUMMY):        return set_param_data(ap,0                     ,0,0,""         );
        case(MIXSYNC):         return set_param_data(ap,0                     ,0,0,""         );
        case(MIXSYNCATT):      return set_param_data(ap,0                     ,0,0,""         );
        case(MIXGAIN):         return set_param_data(ap,0                     ,2,1,"d0"       );
        case(MIXTWARP):
            switch(mode) {
                case(MTW_REVERSE_T):    case(MTW_REVERSE_NT):
                case(MTW_FREEZE_T):     case(MTW_FREEZE_NT):    case(MTW_TIMESORT):
                    return set_param_data(ap,0                     ,2,0,"00"       );
                case(MTW_DOMINO):       case(MTW_ADD_TO_TG):    case(MTW_SCATTER):
                case(MTW_CREATE_TG_1):  case(MTW_CREATE_TG_2):  case(MTW_CREATE_TG_3):  case(MTW_CREATE_TG_4):
                case(MTW_ENLARGE_TG_1): case(MTW_ENLARGE_TG_2): case(MTW_ENLARGE_TG_3): case(MTW_ENLARGE_TG_4):
                    return set_param_data(ap,0                     ,2,1,"d0"       );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MIXTWARP in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(MIXSWARP):
            switch(mode) {
                case(MSW_TWISTALL):return set_param_data(ap,0                     ,2,0,"00"       );
                case(MSW_TWISTONE):return set_param_data(ap,0                     ,2,1,"i0"       );
                    
                case(MSW_FIXED):
                case(MSW_NARROWED):return set_param_data(ap,0                     ,2,1,"d0"       );
                    
                case(MSW_LEFTWARDS):
                case(MSW_RIGHTWARDS):
                case(MSW_RANDOM):
                case(MSW_RANDOM_ALT):
                    return set_param_data(ap,0                     ,2,2,"dd"       );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MIXSWARP in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(MIXSHUFL):
            switch(mode) {
                case(MSH_DUPLICATE):
                case(MSH_REVERSE_N):
                case(MSH_SCATTER):
                case(MSH_FIXED_N):
                case(MSH_OMIT):
                case(MSH_OMIT_ALT):
                    return set_param_data(ap,0                     ,2,0,"00"       );
                case(MSH_DUPL_AND_RENAME):
                    return set_param_data(ap,SNDFILENAME           ,2,0,"00"       );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MIXSHUFL in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            
        case(EQ):
            switch(mode) {
                case(FLT_LOSHELF):
                case(FLT_HISHELF): return set_param_data(ap,0                     ,5,2,"0dd00"    );
                case(FLT_PEAKING): return set_param_data(ap,0                     ,5,3,"ddd00"    );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for EQ in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(LPHP):            return set_param_data(ap,0                     ,5,3,"0ddd0"    );
        case(FSTATVAR):        return set_param_data(ap,0                     ,5,3,"DdD00"    );
        case(FLTBANKN):
            switch(mode) {
                case(FLT_HARMONIC):
                case(FLT_ALTERNATE):
                case(FLT_SUBHARM): return set_param_data(ap,0                     ,5,4,"Dddd0"    );
                case(FLT_EQUALSPAN):return set_param_data(ap,0                    ,5,5,"Ddddi"    );
                case(FLT_LINOFFSET):return set_param_data(ap,0                    ,5,5,"Ddddd"    );
                case(FLT_EQUALINT):return set_param_data(ap,0                     ,5,5,"Ddddd"    );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for FLTBANKN in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(FLTBANKC):
            switch(mode) {
                case(FLT_HARMONIC):
                case(FLT_ALTERNATE):
                case(FLT_SUBHARM): return set_param_data(ap,0                     ,6,2,"00dd00"    );
                case(FLT_EQUALSPAN):return set_param_data(ap,0                    ,6,3,"00ddi0"    );
                case(FLT_LINOFFSET):return set_param_data(ap,0                    ,6,3,"00ddd0"    );
                case(FLT_EQUALINT):return set_param_data(ap,0                     ,6,3,"00ddd0"    );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for FLTBANKC in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(FLTBANKU):        return set_param_data(ap,FILTERBANK            ,5,2,"Dd000"    );
        case(FLTBANKV):        return set_param_data(ap,TIMEVARYING_FILTERBANK,3,2,"Dd0"      );
        case(FLTBANKV2):       return set_param_data(ap,TIMEVARY2_FILTERBANK  ,3,2,"Dd0"      );
        case(FLTITER):         return set_param_data(ap,FILTERBANK            ,6,4,"dddd00"   );
        case(FLTSWEEP):        return set_param_data(ap,0                     ,5,5,"DdDDD"    );
        case(ALLPASS):         return set_param_data(ap,0                     ,5,2,"0dD00"    );
            
        case(MOD_LOUDNESS):
            switch(mode) {
                    //      case(LOUDNESS_GAIN):    return set_param_data(ap,0                ,2,1, "d0");
                case(LOUDNESS_GAIN):    return set_param_data(ap,0                ,2,1, "D0");
                    //      case(LOUDNESS_DBGAIN):  return set_param_data(ap,0                ,2,1, "d0");
                case(LOUDNESS_DBGAIN):  return set_param_data(ap,0                ,2,1, "D0");
                case(LOUDNESS_NORM):    return set_param_data(ap,0                ,1,0, "0");
                case(LOUDNESS_SET):     return set_param_data(ap,0                ,1,0, "0");
                case(LOUDNESS_BALANCE): return set_param_data(ap,0                ,2,0, "00");
                case(LOUDNESS_PHASE):   return set_param_data(ap,0                ,0,0, "");
                case(LOUDNESS_LOUDEST): return set_param_data(ap,0                ,2,0, "00");
                case(LOUDNESS_EQUALISE): return set_param_data(ap,0               ,2,0, "00");
                case(LOUD_PROPOR):      return set_param_data(ap,LOUDNESS         ,2,0, "00");
                case(LOUD_DB_PROPOR):   return set_param_data(ap,LOUDNESS         ,2,0, "00");
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MOD_LOUDNESS in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(MOD_SPACE):
            switch(mode) {
                case(MOD_PAN):          return set_param_data(ap,0                ,1,1, "D");
                case(MOD_MIRROR):       return set_param_data(ap,0                ,2,0, "00");
                case(MOD_MIRRORPAN):    return set_param_data(ap,0                ,2,0, "00");
                case(MOD_NARROW):       return set_param_data(ap,0                ,2,1, "d0");
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MOD_SPACE in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            //TW NEW
        case(SCALED_PAN):           return set_param_data(ap,0                ,1,1, "D");
        case(MOD_PITCH):
            switch(mode) {
                case(MOD_ACCEL):        return set_param_data(ap,0                ,2,2, "dd");
                case(MOD_VIBRATO):      return set_param_data(ap,0                ,2,2, "DD");
                default:                return set_param_data(ap,0                ,1,1, "D");
            }
            break;
        case(MOD_REVECHO):
            switch(mode) {
                case(MOD_DELAY):        return set_param_data(ap,0                ,8,4,"dDD0000d");
                case(MOD_VDELAY):       return set_param_data(ap,0                ,8,8,"dddddddd");
                case(MOD_STADIUM):      return set_param_data(ap,0                ,0,0,"");
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MOD_REVECHO in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(MOD_RADICAL):
            switch(mode) {
                case(MOD_REVERSE):      return set_param_data(ap,0                ,0,0,"");
                case(MOD_SHRED):        return set_param_data(ap,0                ,2,2,"id");
                case(MOD_SCRUB):        return set_param_data(ap,0                ,1,1,"d");
                case(MOD_LOBIT):        return set_param_data(ap,0                ,2,2,"ii");
                case(MOD_LOBIT2):       return set_param_data(ap,0                ,2,1,"i0");
                case(MOD_RINGMOD):      return set_param_data(ap,0                ,1,1,"D");
                case(MOD_CROSSMOD):     return set_param_data(ap,0                ,0,0,"");
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MOD_RADICAL in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(BRASSAGE):
            switch(mode) {
                case(GRS_PITCHSHIFT):   return set_param_data(ap,0                ,16,1, "00000D0000000000");
                case(GRS_TIMESTRETCH):  return set_param_data(ap,0                ,16,1, "D000000000000000");
                case(GRS_REVERB):       return set_param_data(ap,0                ,16,3, "0D000DD000000000");
                case(GRS_SCRAMBLE):     return set_param_data(ap,0                ,16,1, "0000D00000000000");
                case(GRS_GRANULATE):    return set_param_data(ap,0                ,16,1, "0D00000000000000");
                case(GRS_BRASSAGE):     return set_param_data(ap,0                ,16,8, "DD00DDDDDD000000");
                case(GRS_FULL_MONTY):   return set_param_data(ap,0                ,16,16,"DDDDDDDDDDDDDDDD");
                default:
                    sprintf(errstr,"Unknown mode (%d)  for BRASSAGE in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(SAUSAGE):         return set_param_data(ap,0                      ,16,16,"DDDDDDDDDDDDDDDD");
            
        case(PVOC_ANAL):       return set_param_data(ap,0                     ,0,0,""         );
        case(PVOC_SYNTH):      return set_param_data(ap,0                     ,2,0,"00"       );
        case(PVOC_EXTRACT):    return set_param_data(ap,0                     ,0,0,""         );
            
            /* TEMPORARY TEST ROUTINE */
        case(WORDCNT):         return set_param_data(ap,0                     ,0,0,""         );
            /* TEMPORARY TEST ROUTINE */
            
        case(EDIT_CUT):
            switch(mode) {
                case(EDIT_SECS):   return set_param_data(ap,0                     ,2,2,"dd"       );
                case(EDIT_SAMPS):
                case(EDIT_STSAMPS):return set_param_data(ap,0                     ,2,2,"ii"       );
                default:
                    sprintf(errstr,"Unknown mode (%d) for EDIT_CUT in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            //TW NEW
        case(EDIT_CUTMANY): return set_param_data(ap,MANYCUTS                 ,1,1,"d"       );
        case(STACK):        return set_param_data(ap,STACKDATA                ,5,5,"idddd"   );
            
        case(EDIT_CUTEND):
            switch(mode) {
                case(EDIT_SECS):   return set_param_data(ap,0                     ,2,1,"d0"       );
                case(EDIT_SAMPS):
                case(EDIT_STSAMPS):return set_param_data(ap,0                     ,2,1,"i0"       );
                default:
                    sprintf(errstr,"Unknown mode (%d) for EDIT_CUTEND in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(EDIT_ZCUT):
            switch(mode) {
                case(EDIT_SECS):   return set_param_data(ap,0                     ,2,2,"dd"       );
                case(EDIT_SAMPS):
                case(EDIT_STSAMPS):return set_param_data(ap,0                     ,2,2,"ii"       );
                default:
                    sprintf(errstr,"Unknown mode (%d) for EDIT_ZCUT in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(MANY_ZCUTS):      return set_param_data(ap,MANYCUTS              ,0,0,""         );
        case(EDIT_EXCISE):
            switch(mode) {
                case(EDIT_SECS):   return set_param_data(ap,0                     ,2,2,"dd"       );
                case(EDIT_SAMPS):
                case(EDIT_STSAMPS):return set_param_data(ap,0                     ,2,2,"ii"       );
                default:
                    sprintf(errstr,"Unknown mode (%d) for EDIT_EXCISE in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            
        case(EDIT_EXCISEMANY): return set_param_data(ap,EXCISE_TIMES          ,2,0,"00"       );
        case(INSERTSIL_MANY):  return set_param_data(ap,EXCISE_TIMES          ,2,0,"00"       );
            
        case(EDIT_INSERT):
            switch(mode) {
                case(EDIT_SECS):   return set_param_data(ap,0                     ,2,1,"d0"       );
                case(EDIT_SAMPS):
                case(EDIT_STSAMPS):return set_param_data(ap,0                     ,2,1,"i0"       );
                default:
                    sprintf(errstr,"Unknown mode (%d) for EDIT_INSERT in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            //TW NEW
        case(EDIT_INSERT2):    return set_param_data(ap,0                     ,2,2,"dd"       );
            
        case(EDIT_INSERTSIL):
            switch(mode) {
                case(EDIT_SECS):   return set_param_data(ap,0                     ,2,2,"dd"       );
                case(EDIT_SAMPS):
                case(EDIT_STSAMPS):return set_param_data(ap,0                     ,2,2,"ii"       );
                default:
                    sprintf(errstr,"Unknown mode (%d) for EDIT_INSERTSIL in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(EDIT_JOIN):       return set_param_data(ap,0                     ,2,0,"00"       );
            
        case(HOUSE_COPY):
            switch(mode) {
                case(COPYSF):      return set_param_data(ap,0                     ,1,0,"0"        );
                case(DUPL):        return set_param_data(ap,0                     ,1,1,"i"        );
                default:
                    sprintf(errstr,"Unknown mode (%d) for HOUSE_COPY in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(HOUSE_DEL):       return set_param_data(ap,SNDFILENAME           ,0,0,""         );
        case(HOUSE_CHANS):
            switch(mode) {
                case(HOUSE_CHANNEL):    return set_param_data(ap,0                ,1,1,"i"        );
                case(HOUSE_CHANNELS):   return set_param_data(ap,0                ,0,0,""         );
                case(HOUSE_ZCHANNEL):   return set_param_data(ap,0                ,1,1,"i"        );
                case(STOM):             return set_param_data(ap,0                ,0,0,""         );
                case(MTOS):             return set_param_data(ap,0                ,0,0,""         );
                default:
                    sprintf(errstr,"Unknown mode (%d) for HOUSE_CHANS in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(HOUSE_BUNDLE):    return set_param_data(ap,0                     ,0,0,""         );
        case(HOUSE_SORT):
            switch(mode) {
                case(BY_FILETYPE):      return set_param_data(ap,0                ,3,0,"000"      );
                case(BY_SRATE):         return set_param_data(ap,0                ,3,0,"000"      );
                case(BY_DURATION):      return set_param_data(ap,0                ,3,3,"ddd"      );
                case(BY_LOG_DUR):       return set_param_data(ap,0                ,3,3,"ddd"      );
                case(IN_DUR_ORDER):     return set_param_data(ap,0                ,3,0,"000"      );
                case(FIND_ROGUES):      return set_param_data(ap,0                ,3,0,"000"      );
                default:
                    sprintf(errstr,"Unknown mode (%d) for HOUSE_SORT in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(HOUSE_SPEC):
            switch(mode) {
                case(HOUSE_RESAMPLE): return set_param_data(ap,0                  ,1,1,"i"        );
                default:              return set_param_data(ap,0                  ,0,0,""         );
            }
            break;
        case(HOUSE_EXTRACT):
            switch(mode) {
                case(HOUSE_RECTIFY):  return set_param_data(ap,0                  ,1,1,"d"        );
                case(HOUSE_CUTGATE):
                case(HOUSE_CUTGATE_PREVIEW):
                case(HOUSE_TOPNTAIL): return set_param_data(ap,0                  ,0,0,""         );
                case(HOUSE_ONSETS):   return set_param_data(ap,0                  ,9,7,"d0dd0iddi");
                case(HOUSE_BYHAND):   return set_param_data(ap,BY_HAND            ,0,0,""         );
                default:
                    sprintf(errstr,"Unknown mode (%d) for HOUSE_EXTRACT in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(TOPNTAIL_CLICKS):    return set_param_data(ap,0                  ,2,2,"dd"       );
            //TW NEW: REPLACING BRACKETED OUT CODE (Dump & Recover abandoned)
        case(HOUSE_BAKUP):        return set_param_data(ap,0                  ,0,0,""         );
        case(HOUSE_GATE):         return set_param_data(ap,0                  ,0,0,""         );
            
        case(HOUSE_DISK):         return set_param_data(ap,0                  ,0,0,""         );
            
        case(INFO_PROPS):         return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_SFLEN):         return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_TIMELIST):      return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_LOUDLIST):      return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_TIMESUM):       return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_TIMEDIFF):      return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_SAMPTOTIME):    return set_param_data(ap,0                  ,1,1,"i"        );
        case(INFO_TIMETOSAMP):    return set_param_data(ap,0                  ,1,1,"d"        );
        case(INFO_MAXSAMP):       return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_MAXSAMP2):      return set_param_data(ap,0                  ,2,2,"dd"       );
        case(INFO_LOUDCHAN):      return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_FINDHOLE):      return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_DIFF):          return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_CDIFF):         return set_param_data(ap,0                  ,0,0,""         );
        case(INFO_PRNTSND):       return set_param_data(ap,0                  ,2,2,"dd"       );
        case(INFO_MUSUNITS):
            switch(mode) {
                case(MU_MIDI_TO_FRQ):           case(MU_FRQ_TO_MIDI):       case(MU_FRQRATIO_TO_SEMIT):
                case(MU_FRQRATIO_TO_INTVL):     case(MU_SEMIT_TO_FRQRATIO): case(MU_OCTS_TO_FRQRATIO):
                case(MU_OCTS_TO_SEMIT):         case(MU_SEMIT_TO_OCTS):     case(MU_SEMIT_TO_INTVL):
                case(MU_FRQRATIO_TO_TSTRETH):   case(MU_SEMIT_TO_TSTRETCH): case(MU_OCTS_TO_TSTRETCH):
                case(MU_TSTRETCH_TO_FRQRATIO):  case(MU_TSTRETCH_TO_SEMIT): case(MU_TSTRETCH_TO_OCTS):
                case(MU_TSTRETCH_TO_INTVL):     case(MU_GAIN_TO_DB):        case(MU_DB_TO_GAIN):
                case(MU_FRQRATIO_TO_OCTS):      case(MU_MIDI_TO_NOTE):      case(MU_FRQ_TO_NOTE):
                case(MU_FRQ_TO_DELAY):          case(MU_MIDI_TO_DELAY):     case(MU_TEMPO_TO_DELAY):
                case(MU_DELAY_TO_FRQ):          case(MU_DELAY_TO_MIDI):     case(MU_DELAY_TO_TEMPO):
                    return set_param_data(ap,0                  ,1,1,"d"        );
                case(MU_NOTE_TO_FRQ):
                case(MU_NOTE_TO_MIDI):
                case(MU_NOTE_TO_DELAY):
                    return set_param_data(ap,NOTE_REPRESENTATION      ,0,0,""         );
                case(MU_INTVL_TO_FRQRATIO):
                case(MU_INTVL_TO_TSTRETCH):
                    return set_param_data(ap,INTERVAL_REPRESENTATION  ,0,0,""         );
                default:
                    sprintf(errstr,"Unknown mode (%d) for MUSUNITS in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(SYNTH_WAVE):       return set_param_data(ap,0                    ,4,4,"iidD"     );
        case(MULTI_SYN):        return set_param_data(ap,CHORD_SYN            ,4,3,"iid0"     );
        case(SYNTH_NOISE):      return set_param_data(ap,0                    ,4,3,"iid0"     );
        case(SYNTH_SIL):        return set_param_data(ap,0                    ,4,3,"iid0"     );
        case(SYNTH_SPEC):       return set_param_data(ap,0                    ,7,7,"dDDDDDd"  );
        case(RANDCUTS):         return set_param_data(ap,0                    ,2,2,"dd"       );
        case(RANDCHUNKS):       return set_param_data(ap,0                    ,2,2,"id"       );
        case(SIN_TAB):          return set_param_data(ap,0                    ,5,5,"DDddd"    );
        case(ACC_STREAM):       return set_param_data(ap,ATTACK_STREAM        ,1,1,"D"        );
        case(HF_PERM1):
            switch(mode) {
                case(HFP_SNDOUT):   return set_param_data(ap,0                    ,10,10,"idddiiiiii");
                case(HFP_SNDSOUT):  return set_param_data(ap,0                    ,10,9, "idd0iiiiii");
                case(HFP_TEXTOUT):  return set_param_data(ap,0                    ,10,6, "0000iiiiii");
                case(HFP_MIDIOUT):  return set_param_data(ap,0                    ,10,6, "0000iiiiii");
                default:
                    sprintf(errstr,"Unknown mode (%d) for HF_PERM1 in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
        case(HF_PERM2):
            switch(mode) {
                case(HFP_SNDOUT):   return set_param_data(ap,0                    ,10,6,"idddi0000i");
                case(HFP_SNDSOUT):  return set_param_data(ap,0                    ,10,5,"idd0i0000i");
                case(HFP_TEXTOUT):  return set_param_data(ap,0                    ,10,2,"0000i0000i");
                case(HFP_MIDIOUT):  return set_param_data(ap,0                    ,10,2,"0000i0000i");
                default:
                    sprintf(errstr,"Unknown mode (%d) for HF_PERM2 in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(DEL_PERM):         return set_param_data(ap,DELPERM              ,3,3,"idi");
        case(DEL_PERM2):        return set_param_data(ap,DELPERM2             ,3,1,"00i");
        case(TWIXT):
            switch(mode) {
                case(TRUE_EDIT):     return set_param_data(ap,SWITCH_TIMES        ,2,1,"d0");
                case(IN_SEQUENCE):   return set_param_data(ap,SWITCH_TIMES        ,2,1,"d0");
                case(RAND_REORDER):  return set_param_data(ap,SWITCH_TIMES        ,2,2,"di");
                case(RAND_SEQUENCE): return set_param_data(ap,SWITCH_TIMES        ,2,2,"di");
                default:
                    //          sprintf(errstr,"Unknown mode (%d) for TWIXT in set_vflgs()\n",mode);
                    sprintf(errstr,"Unknown mode (%d) for TWIXT in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(SPHINX):
            switch(mode) {
                case(IN_SEQUENCE):   return set_param_data(ap,MANY_SWITCH_TIMES   ,2,1,"d0");
                case(RAND_REORDER):  return set_param_data(ap,MANY_SWITCH_TIMES   ,2,2,"di");
                case(RAND_SEQUENCE): return set_param_data(ap,MANY_SWITCH_TIMES   ,2,2,"di");
                default:
                    //          sprintf(errstr,"Unknown mode (%d) for SPHINX in set_vflgs()\n",mode);
                    sprintf(errstr,"Unknown mode (%d) for SPHINX in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            //TW NEW
        case(MIX_ON_GRID):      return set_param_data(ap,GRIDDED_MIX          ,0,0,"");
        case(AUTOMIX):          return set_param_data(ap,AUTO_MIX             ,1,1,"D");
        case(NOISE_SUPRESS):    return set_param_data(ap,0                    ,4,4,"dddd");
        case(TIME_GRID):        return set_param_data(ap,0                    ,3,3,"iDD");
        case(SEQUENCER2):       return set_param_data(ap,SEQUENCER2_VALUES    ,1,1,"d");
        case(SEQUENCER):        return set_param_data(ap,SEQUENCER_VALUES     ,1,1,"d");
        case(CONVOLVE):
            switch(mode) {
                case(CONV_NORMAL):  return set_param_data(ap,0                    ,1,0,"0");
                case(CONV_TVAR):    return set_param_data(ap,0                    ,1,1,"D");
                default:
                    sprintf(errstr,"Unknown mode (%d) for CONVOLVE in set_legal_param_structure()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            
        case(BAKTOBAK):         return set_param_data(ap,0                    ,2,2,"dd");
        case(ADDTOMIX):         return set_param_data(ap,0                    ,0,0,"");
        case(MIX_PAN):          return set_param_data(ap,0                    ,1,1,"D");
        case(SHUDDER):          return set_param_data(ap,0                    ,8,8,"dDDDDDDD");
        case(MIX_AT_STEP):      return set_param_data(ap,0                    ,1,1,"d");
        case(FIND_PANPOS):      return set_param_data(ap,0                    ,1,1,"d");
        case(CLICK):            return set_param_data(ap,CLICKTRACK           ,0,0,"");
        case(DOUBLETS):         return set_param_data(ap,0                    ,2,2,"Di");
        case(SYLLABS):          return set_param_data(ap,SYLLTIMES            ,2,2,"dd");
        case(JOIN_SEQ):         return set_param_data(ap,JOINSEQ              ,2,0,"00");
        case(JOIN_SEQDYN):      return set_param_data(ap,JOINSEQDYN           ,2,0,"00");
        case(MAKE_VFILT):       return set_param_data(ap,0                    ,0,0,"");
        case(BATCH_EXPAND):     return set_param_data(ap,BATCH                ,3,3,"iii");
        case(MIX_MODEL):        return set_param_data(ap,0                    ,0,0,"");
        case(CYCINBETWEEN):     return set_param_data(ap,0                    ,2,2,"id");
        case(ENVSYN):
            switch(mode) {
                case(ENVSYN_USERDEF):
                    return set_param_data(ap,ENVSYN_ENVELOPE          ,6,4,"ddDd00"  );
                default:       return set_param_data(ap,0                         ,6,6,"ddDdDD"  );
            }
            break;
        case(RRRR_EXTEND):
            switch(mode) {
                case(0):            return set_param_data(ap,0                    ,11,10,"dddidiDD0DD");
                case(1):            return set_param_data(ap,0                    ,11,11,"dddidiDDiDD");
                case(2):            return set_param_data(ap,0                    ,11,4, "dd0id000000");
            }
            break;
        case(SSSS_EXTEND):      return set_param_data(ap,0                    ,4,4,"dddd");
        case(HOUSE_GATE2):      return set_param_data(ap,0                    ,5,5,"ddddd");
        case(GRAIN_ASSESS):     return set_param_data(ap,0                    ,2,0,"00"       );
        case(ZCROSS_RATIO):     return set_param_data(ap,0                    ,0,0,"00"       );
        case(GREV):
            switch(mode) {
                case(0):            return set_param_data(ap,0                    ,5,3,"ddI00");
                case(1):            return set_param_data(ap,0                    ,5,4,"ddII0");
                case(2):            return set_param_data(ap,0                    ,5,5,"ddIIi");
                case(3):            return set_param_data(ap,0                    ,5,5,"ddIIi");
                case(4):            return set_param_data(ap,0                    ,5,4,"ddID0");
                case(5):            return set_param_data(ap,0                    ,5,3,"ddI00");
                case(6):            return set_param_data(ap,GRAIN_TIMINGS        ,5,3,"ddI00");
            }
            break;
            /*  |                         |m| |        | */
            /*  |                         |a| |        | */
            /*  |                         |x| |        | */
            /*  |                         |p|p|        | */
            /*  |                         |a|a|        | */
            /*  |  special-data           |r|r| param  | */
            /*  |                         |a|a|  list  | */
            /*  |                         |m|m|        | */
            /*  |                         |c|c|        | */
            /*  |                         |n|n|        | */
            /*  |                         |t|t|        | */
        default:
            sprintf(errstr,"Unknown process (%d) in set_legal_param_structure()\n",process);
            return(PROGRAM_ERROR);
    }
    return(FINISHED);
}                                                           

/************************** SET_LEGAL_OPTION_AND_VARIANT_STRUCTURE ***********/

int set_legal_option_and_variant_structure(int process,int mode,aplptr ap)                                        
{
    /*|      | |        |        |v| |       */
    /*|      |o|        |        |f|v|       */
    /*|option|p| option |variant |l|p|variant*/
    /*|flags |t|  list  | flags  |a|a|list   */
    /*|      |c|        |        |g|r|       */
    /*|      |n|        |        |c|a|     */
    /*|      |t|        |        |n|m|     */
    /*|      | |        |        |t|s|       */
    switch(process) {
        case(ACCU):        return set_vflgs(ap,""      ,0,""      ,"dg"    ,2,2,"DD"    );
        case(ALT):         return set_vflgs(ap,""      ,0,""      ,"x"     ,1,0,"0"     );
        case(ARPE):
            switch(mode) {
                case(ON): case(BOOST): case(ABOVE_BOOST): case(BELOW_BOOST):
                    return set_vflgs(ap,"plhba", 5,"dDDDD" ,"NsTK"  ,4,2,"dI00"  );
                    //TW JULY 2006
                case(ABOVE): case(BELOW):
                    return set_vflgs(ap,"plhba", 5,"dDD0D" ,""     ,0,0,""      );
                default:       return set_vflgs(ap,"plhba", 5,"dDDDD" ,""      ,0,0,""      );
            }
            break;
        case(AVRG):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(BARE):        return set_vflgs(ap,""      ,0,""      ,"x"     ,1,0,"0"     );
        case(BLTR):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(BLUR):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(BRIDGE):      return set_vflgs(ap,"abcde" ,5,"ddddd" ,"fg"    ,2,2,"dd"    );
        case(CHANNEL):     return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(CHORD):       return set_vflgs(ap,""      ,0,""      ,"btx"   ,3,2,"DD0"   );
        case(CHORUS):      return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(CLEAN):       return set_vflgs(ap,"g"     ,1,"d"     ,""      ,0,0,""      );
        case(CROSS):       return set_vflgs(ap,""      ,0,""      ,"i"     ,1,1,"D"     );
        case(CUT):         return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(DIFF):        return set_vflgs(ap,""      ,0,""      ,"ca"    ,2,1,"D0"    );
        case(DRUNK):       return set_vflgs(ap,""      ,0,""      ,"z"     ,1,0,"0"     );
        case(EXAG):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(FILT):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(FMNTSEE):     return set_vflgs(ap,""      ,0,""      ,"v"     ,1,0,"0"     );
        case(FOCUS):       return set_vflgs(ap,"bt"    ,2,"DD"    ,"s"     ,1,1,"i"     );
        case(FOLD):        return set_vflgs(ap,""      ,0,""      ,"x"     ,1,0,"0"     );
        case(FORM):        return set_vflgs(ap,"lhg"   ,3,"ddd"   ,""      ,0,0,""      );
        case(FORMANTS):    return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(FORMSEE):     return set_vflgs(ap,""      ,0,""      ,"s"     ,1,0,"0"     );
        case(FREEZE):      return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(FREEZE2):     return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(FREQUENCY):   return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(GAIN):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(GLIDE):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(GLIS):        return set_vflgs(ap,""      ,0,""      ,"t"     ,1,1,"d"     );
        case(GRAB):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(GREQ):        return set_vflgs(ap,""      ,0,""      ,"r"     ,1,0,"0"     );
        case(INVERT):      return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(LEAF):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(LEVEL):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(MAGNIFY):     return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(MAKE):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(MAX):         return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(MEAN):        return set_vflgs(ap,"lhc"   ,3,"ddi"   ,"z"     ,1,0,"0"     );
        case(MORPH):       return set_vflgs(ap,"s"     ,1,"d"     ,""      ,0,0,""      );
        case(NOISE):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(OCT):         return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(OCTVU):       return set_vflgs(ap,""      ,0,""      ,"f"     ,1,1,"d"     );
        case(P_APPROX):    return set_vflgs(ap,"pts"   ,3,"DDD"   ,""      ,0,0,""      );
        case(P_CUT):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_EXAG):      return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_FIX):       return set_vflgs(ap,""      ,0,""      ,"rxlhsbewi",9,7,"ddddidd00");
        case(P_HEAR):      return set_vflgs(ap,"g"     ,1,"d"     ,""      ,0,0,""      );
        case(P_INFO):      return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_INVERT):    return set_vflgs(ap,""      ,0,""      ,"mbt"   ,3,3,"Ddd"   );
        case(P_QUANTISE):  return set_vflgs(ap,""      ,0,""      ,"o"     ,1,0,"0"     );
        case(P_RANDOMISE): return set_vflgs(ap,""      ,0,""      ,"s"     ,1,1,"d"     );
        case(P_SEE):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_SMOOTH):    return set_vflgs(ap,""      ,0,""      ,"ph"    ,2,1,"D0"    );
            //TW NEW
        case(P_SYNTH):     return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(VFILT):
        case(P_VOWELS):    return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_GEN):       return set_vflgs(ap,"co"    ,2,"ii"    ,""      ,0,0,""      );
        case(P_INSERT):    return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_SINSERT):   return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_PTOSIL):    return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_NTOSIL):    return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(ANALENV):     return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_BINTOBRK):  return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(MAKE2):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_INTERP):    return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
            
        case(P_TRANSPOSE): return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_VIBRATO):   return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(P_WRITE):     return set_vflgs(ap,"d"     ,1,"d"     ,""      ,0,0,""      );
        case(P_ZEROS):     return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(PEAK):        return set_vflgs(ap,"ctf"   ,3,"ddd"   ,"h"     ,1,0,"0"     );
        case(PICK):        return set_vflgs(ap,"c"     ,1,"D"     ,""      ,0,0,""      );
        case(PITCH):
            switch(mode) {
                case(PICH_TO_BIN):
                    return set_vflgs(ap,"tgsnlh",6,"dididd"  ,"az"  ,2,0,"00"    );
                case(PICH_TO_BRK):
                    return set_vflgs(ap,"tgsnlhd",7,"dididdd","a"   ,1,0,"0"     );
            }
            break;
        case(PLUCK):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(PRINT):       return set_vflgs(ap,"w"     ,1,"i",     ""      ,0,0,""      );
        case(REPITCH):     return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(REPITCHB):    return set_vflgs(ap,"d"     ,1,"d"     ,""      ,0,0,""      );
        case(REPORT):      return set_vflgs(ap,"bts"   ,3,"DDi"   ,""      ,0,0,""      );
        case(SCAT):        return set_vflgs(ap,"b"     ,1,"D"     ,"rn"    ,2,0,"00"    );
        case(SHIFT):       return set_vflgs(ap,""      ,0,""      ,"l"     ,1,0,"0"     );
        case(SHIFTP):      return set_vflgs(ap,""      ,0,""      ,"d"     ,1,1,"D"     );
        case(SHUFFLE):     return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(SPLIT):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(SPREAD):      return set_vflgs(ap,"s"     ,1,"D"     ,""      ,0,0,""      );
        case(STEP):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(STRETCH):     return set_vflgs(ap,""      ,0,""      ,"d"     ,1,1,"D"     );
        case(SUM):         return set_vflgs(ap,""      ,0,""      ,"c"     ,1,1,"D"     );
        case(SUPR):        return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(S_TRACE):
            switch(mode) {
                case(TRC_ALL): return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
                default:       return set_vflgs(ap,""      ,0,""      ,"r"     ,1,0,"0"     );
            }
            break;
        case(TRACK):
            switch(mode) {
                case(TRK_TO_BIN):
                    return set_vflgs(ap,"tgsh"  ,4,"didd"  ,""      ,0,0,""      );
                case(TRK_TO_BRK):
                    return set_vflgs(ap,"tgshd" ,5,"diddd" ,""      ,0,0,""      );
            }
            break;
        case(TRNSF):       return set_vflgs(ap,""      ,0,""      ,"lhx"   ,3,2,"DD0"   );
        case(TRNSP):       return set_vflgs(ap,""      ,0,""      ,"lhx"   ,3,2,"DD0"   );
        case(TSTRETCH):    return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(TUNE):        return set_vflgs(ap,"fc"    ,2,"DD"    ,"tb"    ,2,2,"ID"    );
        case(VOCODE):      return set_vflgs(ap,"lhg"   ,3,"ddd"   ,""      ,0,0,""      );
        case(WARP):        return set_vflgs(ap,"pts"   ,3,"DDD"   ,""      ,0,0,""      );
        case(WAVER):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(WEAVE):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(WINDOWCNT):   return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(LIMIT):       return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(MULTRANS):    return set_vflgs(ap,""      ,0,""      ,"btx"   ,3,2,"DD0"   );
            
            /************* GROUCHO **************/
            /************* GROUCHO **************/
            /************* GROUCHO **************/
            
        case(DISTORT):     return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(DISTORT_ENV):
            switch(mode) {
                case(DISTORTE_RISING):
                case(DISTORTE_FALLING):     /* 1 param, 2 variants: param[1] = trof */
                    return set_vflgs(ap,""      ,0,""      ,"te"    ,2,2,"DD"    );
                case(DISTORTE_TROFFED):     /* 2 params, 1 variant: param[1] = trof */
                    return set_vflgs(ap,""      ,0,""      ,"e"     ,1,1,"D"     );
                case(DISTORTE_USERDEF):
                    return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
            }
            break;
        case(DISTORT_AVG): return set_vflgs(ap,"m"     ,1,"d"     ,"s"     ,1,1,"i"     );
        case(DISTORT_OMT): return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(DISTORT_MLT): return set_vflgs(ap,""      ,0,""      ,"s"     ,1,0,"0"     );
        case(DISTORT_DIV): return set_vflgs(ap,""      ,0,""      ,"i"     ,1,0,"0"     );
        case(DISTORT_HRM): return set_vflgs(ap,""      ,0,""      ,"p"     ,1,1,"d"     );
        case(DISTORT_FRC): return set_vflgs(ap,""      ,0,""      ,"p"     ,1,1,"d"     );
        case(DISTORT_REV): return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(DISTORT_SHUF):return set_vflgs(ap,"c"     ,1,"I"     ,"s"     ,1,1,"i"     );
        case(DISTORT_RPT2):
        case(DISTORT_RPT): return set_vflgs(ap,"c"     ,1,"I"     ,"s"     ,1,1,"i"     );
        case(DISTORT_RPTFL): return set_vflgs(ap,"c"   ,1,"I"     ,"sf"    ,2,2,"id"    );
        case(DISTORT_INTP):return set_vflgs(ap,""      ,0,""      ,"s"     ,1,1,"i"     );
        case(DISTORT_DEL): return set_vflgs(ap,""      ,0,""      ,"s"     ,1,1,"i"     );
        case(DISTORT_RPL): return set_vflgs(ap,""      ,0,""      ,"s"     ,1,1,"i"     );
        case(DISTORT_TEL): return set_vflgs(ap,""      ,0,""      ,"sa"    ,2,1,"i0"    );
        case(DISTORT_FLT): return set_vflgs(ap,""      ,0,""      ,"s"     ,1,1,"i"     );
        case(DISTORT_INT): return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(DISTORT_CYCLECNT):
            return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
        case(DISTORT_PCH): return set_vflgs(ap,"cs"    ,2,"Ii"    ,""      ,0,0,""      );
        case(DISTORT_OVERLOAD):
            return set_vflgs(ap,""      ,0,""      ,""      ,0,0,""      );
            //TW NEW
        case(DISTORT_PULSED):
            return set_vflgs(ap,""      ,0,""      ,"se"    ,2,0,"00"    );
            
        case(ZIGZAG):
            switch(mode) {
                case(ZIGZAG_SELF):
                    return set_vflgs(ap,"sm"    ,2,"dd"   ,"r"       ,1,1,"i"     );
                case(ZIGZAG_USER):
                    return set_vflgs(ap,"s"     ,1,"d"     ,""       ,0,0,""      );
                default:
                    sprintf(errstr,"Unknown mode (%d) : ZIGZAG: set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(LOOP):
            switch(mode) {
                case(LOOP_ALL):
                    return set_vflgs(ap,"ws"    ,2,"dd"   ,"b"       ,1,0,"0"     );
                case(LOOP_OUTLEN):
                case(LOOP_RPTS):
                    return set_vflgs(ap,"lws"   ,3,"ddd"  ,"b"       ,1,0,"0"     );
                default:
                    sprintf(errstr,"Unknown mode (%d) : LOOP: set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(SCRAMBLE):    return set_vflgs(ap,"w"     ,1,"d"    ,"sbe"     ,3,1,"i00"   );
        case(ITERATE):     return set_vflgs(ap,""      ,0,""     ,"drpafgs" ,7,7,"DDDDDdi");
        case(ITERATE_EXTEND): return set_vflgs(ap,""      ,0,""     ,"s" ,1,1,"i");
        case(DRUNKWALK):
            switch(mode) {
                case(TOTALLY_PISSED):
                    return set_vflgs(ap,"s"     ,1,"d"    ,"cor"     ,3,3,"DDi"   );
                case(HAS_SOBER_MOMENTS):
                    return set_vflgs(ap,"s"     ,1,"d"    ,"corlh"   ,5,5,"DDiDD" );
                default:
                    sprintf(errstr,"Unknown mode (%d) : DRUNKWALK: set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            
        case(DECORATED): case(PREDECOR):  case(POSTDECOR):
            return set_vflgs(ap,"aps"   ,3,"DDD"   ,"rwdihek",7,1,"i000000");
        case(ORNATE):    case(PREORNATE): case(POSTORNATE):
            return set_vflgs(ap,"aps"   ,3,"DDD"   ,"rwdihe" ,6,1,"i00000");
        case(MOTIFS):    case(TMOTIFS):
        case(MOTIFSIN):  case(TMOTIFSIN):
            return set_vflgs(ap,"aps"   ,3,"DDD"   ,"rwdi"   ,4,1,"i000"  );
        case(GROUPS):    case(TGROUPS):
            return set_vflgs(ap,"aps"   ,3,"DDD"   ,"rwdi"   ,4,1,"i000"  );
        case(SIMPLE_TEX):case(TIMED):
            return set_vflgs(ap,"aps"   ,3,"DDD"   ,"rwcp"     ,4,1,"i000"    );
            
        case(GRAIN_COUNT):      case(GRAIN_OMIT):       case(GRAIN_REPITCH):    case(GRAIN_TIMEWARP):
        case(GRAIN_GET):        case(GRAIN_DUPLICATE):  case(GRAIN_RERHYTHM):   case(GRAIN_ALIGN):
        case(GRAIN_POSITION):   case(GRAIN_REORDER):    case(GRAIN_REMOTIF):    case(GRAIN_REVERSE):
            return set_vflgs(ap,"blh"    ,3,"dDd"    ,"tx"     ,2,1,"d0"    );
            
        case(ENV_EXTRACT):
            switch(mode) {
                case(ENV_ENVFILE_OUT):
                    return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
                case(ENV_BRKFILE_OUT):
                    return set_vflgs(ap,"d" ,1,"d"     ,""       ,0,0,""      );
                default:
                    sprintf(errstr,"Unknown mode (%d) for ENV_EXTRACT: set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(ENV_CREATE):      return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(ENV_WARPING):     return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(ENV_RESHAPING):   return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(ENV_REPLOTTING):  return set_vflgs(ap,"d" ,1,"d"     ,""       ,0,0,""      );
        case(ENV_IMPOSE):      return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
            //TW NEW
        case(ENV_PROPOR):      return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(ENV_REPLACE):     return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(ENV_DOVETAILING): return set_vflgs(ap,"t" ,1,"i"     ,""       ,0,0,""      );
        case(ENV_CURTAILING):  return set_vflgs(ap,"t" ,1,"i"     ,""       ,0,0,""      );
        case(ENV_ENVTOBRK):    return set_vflgs(ap,"d" ,1,"d"     ,""       ,0,0,""      );
        case(ENV_ENVTODBBRK):  return set_vflgs(ap,"d" ,1,"d"     ,""       ,0,0,""      );
        case(ENV_BRKTOENV):    return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(ENV_DBBRKTOENV):  return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(ENV_DBBRKTOBRK):  return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(ENV_BRKTODBBRK):  return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(ENV_SWELL):       return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(ENV_ATTACK):      return set_vflgs(ap,"t" ,1,"i"     ,""       ,0,0,""      );
        case(ENV_PLUCK):       return set_vflgs(ap,"ad",2,"id"    ,""       ,0,0,""      );
        case(ENV_TREMOL):      return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
            
        case(MIX):             return set_vflgs(ap,"seg",3,"ddd"  ,"a"      ,1,0,"0"     );
        case(MIXTWO):          return set_vflgs(ap,"sjkbe",5,"ddddd",""     ,0,0,""      );
            //TW NEW
        case(MIXMANY):         return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(MIXBALANCE):      return set_vflgs(ap,"kbe",3,"Ddd"  ,""       ,0,0,""      );
        case(MIXINTERL):       return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(MIXINBETWEEN):    return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(MIXCROSS):
            switch(mode) {
                case(MCLIN):       return set_vflgs(ap,"sbe" ,3,"ddd" ,""       ,0,0,""      );
                case(MCCOS):       return set_vflgs(ap,"sbep",4,"dddd",""       ,0,0,""      );
            }
            break;
        case(MIXTEST):         return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(MIXMAX):          return set_vflgs(ap,"se",2,"dd"    ,""       ,0,0,""      );
        case(MIXFORMAT):       return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(MIXDUMMY):        return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(MIXSYNC):         return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
        case(MIXSYNCATT):      return set_vflgs(ap,"w" ,1,"i"     ,"p"      ,1,0,"0"     );
        case(MIXTWARP):
            switch(mode) {
                case(MTW_TIMESORT):return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
                default:           return set_vflgs(ap,"se",2,"ii"    ,""       ,0,0,""      );
            }
        case(MIXSWARP):
            switch(mode) {
                case(MSW_TWISTALL):
                case(MSW_TWISTONE):return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""      );
                default:           return set_vflgs(ap,"se",2,"ii"    ,""       ,0,0,""      );
            }
            break;
        case(MIXGAIN):         return set_vflgs(ap,"se",2,"ii"    ,""       ,0,0,""      );
        case(MIXSHUFL):
            switch(mode) {
                case(MSH_DUPL_AND_RENAME):
                    return set_vflgs(ap,"se",2,"ii"    ,"x"      ,1,0,"0"     );
                default:
                    return set_vflgs(ap,"se",2,"ii"    ,""       ,0,0,""      );
            }
            break;
            
            /*RWD May 2004 added t and a "d" to all filters ... correct? */
        case(EQ):              return set_vflgs(ap,"ts"  ,2,"dd"    ,""       ,0,0,""    );
        case(LPHP):            return set_vflgs(ap,"ts"  ,2,"dd"    ,""       ,0,0,""    );
        case(FSTATVAR):        return set_vflgs(ap,"t"   ,1,"d"     ,""       ,0,0,""    );
        case(FLTBANKN):        return set_vflgs(ap,"ts"  ,2,"dd"    ,"d"      ,1,0,"0"   );
        case(FLTBANKC):        return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(FLTBANKU):        return set_vflgs(ap,"t"   ,1,"d"     ,"d"      ,1,0,"0"   );
        case(FLTBANKV):        return set_vflgs(ap,"thr" ,3,"did"   ,"don"    ,3,0,"000" );
        case(FLTBANKV2):       return set_vflgs(ap,"t"   ,1,"d"     ,"dn"     ,2,0,"00"  );
        case(FLTITER):         return set_vflgs(ap,"srpa",4,"ddDD"  ,"dien"   ,4,0,"0000");
        case(FLTSWEEP):        return set_vflgs(ap,"tp"  ,2,"dd"    ,""       ,0,0,""    );
        case(ALLPASS):         return set_vflgs(ap,"ts"  ,2,"dd"    ,"l"      ,1,0,"0"   );
            
        case(MOD_LOUDNESS):
            switch(mode) {
                case(LOUDNESS_GAIN):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(LOUDNESS_DBGAIN):  return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(LOUDNESS_NORM):    return set_vflgs(ap,"l"   ,1,"d"     ,""       ,0,0,""    );
                case(LOUDNESS_SET):     return set_vflgs(ap,"l"   ,1,"d"     ,""       ,0,0,""    );
                case(LOUDNESS_BALANCE): return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(LOUDNESS_PHASE):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(LOUDNESS_LOUDEST): return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(LOUDNESS_EQUALISE): return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(LOUD_PROPOR):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(LOUD_DB_PROPOR):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MOD_LOUDNESS in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(MOD_SPACE):
            switch(mode) {
                case(MOD_PAN):          return set_vflgs(ap,"p"   ,1,"d"     ,""       ,0,0,""    );
                case(MOD_MIRROR):       return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(MOD_MIRRORPAN):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(MOD_NARROW):       return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MOD_SPACE in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            //TW NEW
        case(SCALED_PAN):           return set_vflgs(ap,"p"   ,1,"d"     ,""       ,0,0,""    );
        case(MOD_PITCH):
            switch(mode) {
                case(MOD_ACCEL):      return set_vflgs(ap,"s"   ,1,"d"     ,""       ,0,0,""    );
                case(MOD_VIBRATO):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                default:              return set_vflgs(ap,""    ,0,""      ,"o"      ,1,0,"0"   );
            }
            break;
        case(MOD_REVECHO):
            switch(mode) {
                case(MOD_DELAY):      return set_vflgs(ap,"p"   ,1,"d"     ,"i"      ,1,0,"0"   );
                case(MOD_VDELAY):     return set_vflgs(ap,"p"   ,1,"d"     ,"s"      ,1,1,"i"   );
                case(MOD_STADIUM):    return set_vflgs(ap,"grse",4,"dddi"  ,"n"      ,1,0,"0"   );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MOD_REVECHO in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(MOD_RADICAL):
            switch(mode) {
                case(MOD_REVERSE):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(MOD_SHRED):      return set_vflgs(ap,"s"   ,1,"d"     ,"n"      ,1,0,"0"   );
                case(MOD_SCRUB):      return set_vflgs(ap,"hlse",4,"dddd"  ,"f"      ,1,0,"0"   );
                case(MOD_LOBIT):
                case(MOD_LOBIT2):     return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(MOD_RINGMOD):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(MOD_CROSSMOD):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for MOD_RADICAL in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(BRASSAGE):
            switch(mode) {
                case(GRS_PITCHSHIFT): return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(GRS_TIMESTRETCH):return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
                case(GRS_REVERB):     return set_vflgs(ap,"r"   ,1,"D"     ,""       ,0,0,""    );
                case(GRS_SCRAMBLE):   return set_vflgs(ap,"r"   ,1,"D"     ,""       ,0,0,""    );
                case(GRS_GRANULATE):  return set_vflgs(ap,""    ,0,""      ,"d"      ,1,0,"0"   );
                case(GRS_BRASSAGE):   return set_vflgs(ap,"rjlc",4,"DDdi"  ,"dxn"    ,3,0,"000" );
                case(GRS_FULL_MONTY): return set_vflgs(ap,"rjlc",4,"DDdi"  ,"dxn"    ,3,0,"000" );
                default:
                    sprintf(errstr,"Unknown mode (%d)  for BRASSAGE in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(SAUSAGE):         return set_vflgs(ap,"rjlc",4,"DDdi"  ,"dxn"    ,3,0,"000"  );
            
        case(PVOC_ANAL):       return set_vflgs(ap,"co"  ,2,"ii"    ,""       ,0,0,""     );
        case(PVOC_SYNTH):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(PVOC_EXTRACT):    return set_vflgs(ap,"co"  ,2,"ii"    ,"dlh"    ,3,3,"iii"  );
            
            /* TEMPORARY TEST ROUTINE */
        case(WORDCNT):         return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
            /* TEMPORARY TEST ROUTINE */
            
            
        case(EDIT_CUT):        return set_vflgs(ap,"w"   ,1,"d"     ,""       ,0,0,""     );
            //TW NEW
        case(EDIT_CUTMANY):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(STACK):           return set_vflgs(ap,""    ,0,""      ,"sn"     ,2,0,"00"   );
            
        case(EDIT_CUTEND):     return set_vflgs(ap,"w"   ,1,"d"     ,""       ,0,0,""     );
        case(MANY_ZCUTS):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(EDIT_ZCUT):       return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(EDIT_EXCISE):     return set_vflgs(ap,"w"   ,1,"d"     ,""       ,0,0,""     );
        case(EDIT_EXCISEMANY): return set_vflgs(ap,"w"   ,1,"d"     ,""       ,0,0,""     );
        case(INSERTSIL_MANY):  return set_vflgs(ap,"w"   ,1,"d"     ,""       ,0,0,""     );
        case(EDIT_INSERT):     return set_vflgs(ap,"wl"  ,2,"dd"    ,"o"      ,1,0,"0"    );
            //TW NEW
        case(EDIT_INSERT2):    return set_vflgs(ap,"wl"  ,2,"dd"    ,""       ,0,0,""     );
        case(EDIT_INSERTSIL):  return set_vflgs(ap,"w"   ,1,"d"     ,"os"     ,2,0,"00"    );
        case(EDIT_JOIN):       return set_vflgs(ap,"w"   ,1,"d"     ,"be"     ,2,0,"00"   );
        case(HOUSE_COPY):
            switch(mode) {
                case(COPYSF):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
                case(DUPL):
                    if(!sloom)
                        return set_vflgs(ap,""    ,0,""      ,"i"      ,1,0,"0"    );
                    else
                        return set_vflgs(ap,""    ,0,""      ,""       ,0,0,"0"    );
                default:
                    sprintf(errstr,"Unknown mode (%d) : for HOUSE_COPY in set_vflgs()\n", mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(HOUSE_DEL):       return set_vflgs(ap,""    ,0,""      ,"a"      ,1,0,"0"     );
        case(HOUSE_CHANS):
            switch(mode) {
                case(HOUSE_CHANNEL):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
                case(HOUSE_CHANNELS):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
                case(HOUSE_ZCHANNEL):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
                case(STOM):             return set_vflgs(ap,""    ,0,""      ,"p"      ,1,0,"0"    );
                case(MTOS):             return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
                default:
                    sprintf(errstr,"Unknown mode (%d) for HOUSE_CHANS in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(HOUSE_BUNDLE):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(HOUSE_SORT):
            switch(mode) {
                case(BY_FILETYPE):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
                case(BY_SRATE):         return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
                case(BY_DURATION):      return set_vflgs(ap,""    ,0,""      ,"l"      ,1,0,"0"    );
                case(BY_LOG_DUR):       return set_vflgs(ap,""    ,0,""      ,"l"      ,1,0,"0"    );
                case(IN_DUR_ORDER):     return set_vflgs(ap,""    ,0,""      ,"l"      ,1,0,"0"    );
                case(FIND_ROGUES):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
                default:
                    sprintf(errstr,"Unknown mode (%d) for HOUSE_SORT in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(HOUSE_SPEC):
            switch(mode) {
                case(HOUSE_RESAMPLE): return set_vflgs(ap,""   ,0,""    ,""       ,0,0,""     );
                case(HOUSE_CONVERT):  return set_vflgs(ap,""   ,0,""    ,""       ,0,0,""     );
#ifdef NOTDEF
                case(HOUSE_REPROP):   return set_vflgs(ap,"sct",3,"iii" ,""       ,0,0,""     );
#else
                    /*RWD May 2005 removed -t option */
                case(HOUSE_REPROP):   return set_vflgs(ap,"sc",2,"ii" ,""       ,0,0,""     );
#endif
                default:
                    sprintf(errstr,"Unknown mode (%d) for HOUSE_SPEC in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(HOUSE_EXTRACT):
            switch(mode) {
                case(HOUSE_CUTGATE):
                    if(sloom)
                        return set_vflgs(ap,"gsethbilw",9,"diddiiddi","",0,0,"0");
                    else
                        return set_vflgs(ap,"gsethbilw",9,"diddiiddi","n",1,0,"0");
                case(HOUSE_CUTGATE_PREVIEW):
                    return set_vflgs(ap,""   ,0,""    ,""       ,0,0,""     );
                case(HOUSE_ONSETS):   return set_vflgs(ap,""   ,0,""    ,""       ,0,0,""     );
                case(HOUSE_TOPNTAIL): return set_vflgs(ap,"gs" ,2,"dd" ,"be"      ,2,0,"00"   );
                case(HOUSE_RECTIFY):  return set_vflgs(ap,""   ,0,""    ,""       ,0,0,""     );
                case(HOUSE_BYHAND):   return set_vflgs(ap,""   ,0,""    ,""       ,0,0,""     );
                default:
                    sprintf(errstr,"Unknown mode (%d) for HOUSE_SPEC in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(TOPNTAIL_CLICKS):  return set_vflgs(ap,""   ,0,""      ,"be"     ,2,0,"00"   );
        case(HOUSE_BAKUP):      return set_vflgs(ap,""   ,0,""      ,""       ,0,0,""     );
        case(HOUSE_GATE):       return set_vflgs(ap,"z"  ,1,"i"     ,""       ,0,0,""     );
            
        case(HOUSE_DISK):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
            
        case(INFO_PROPS):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(INFO_SFLEN):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(INFO_TIMELIST):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(INFO_LOUDLIST):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(INFO_TIMESUM):    return set_vflgs(ap,"s"   ,1,"d"     ,""       ,0,0,""     );
        case(INFO_TIMEDIFF):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(INFO_SAMPTOTIME): return set_vflgs(ap,""    ,0,""      ,"g"      ,1,0,"0"    );
        case(INFO_TIMETOSAMP): return set_vflgs(ap,""    ,0,""      ,"g"      ,1,0,"0"    );
        case(INFO_MAXSAMP):    return set_vflgs(ap,""    ,0,""      ,"f"      ,1,0,"0"    );
        case(INFO_MAXSAMP2):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(INFO_LOUDCHAN):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(INFO_FINDHOLE):   return set_vflgs(ap,"t"   ,1,"d"     ,""       ,0,0,""     );
        case(INFO_DIFF):       return set_vflgs(ap,"tn"  ,2,"di"    ,"lc"     ,2,0,"00"   );
        case(INFO_CDIFF):      return set_vflgs(ap,"tn"  ,2,"di"    ,""       ,0,0,""     );
        case(INFO_PRNTSND):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(INFO_MUSUNITS):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
            
        case(MULTI_SYN):       return set_vflgs(ap,"at"  ,2,"di"    ,""       ,0,0,""     );
        case(SYNTH_WAVE):      return set_vflgs(ap,"at"  ,2,"Di"    ,"f"      ,1,0,"0"    );
        case(SYNTH_NOISE):     return set_vflgs(ap,"a"   ,1,"D"     ,"f"      ,1,0,"0"    );
        case(SYNTH_SIL):       return set_vflgs(ap,""    ,0,""      ,"f"      ,1,0,"0"    );
        case(SYNTH_SPEC):      return set_vflgs(ap,""    ,0,""      ,"p"      ,1,0,"0"    );
        case(RANDCUTS):        return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(RANDCHUNKS):      return set_vflgs(ap,"m"   ,1,"d"     ,"ls"     ,2,0,"00"   );
        case(SIN_TAB):         return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(ACC_STREAM):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""     );
        case(HF_PERM1):        return set_vflgs(ap,""    ,0,""      ,"msao"   ,4,0,"0000" );
        case(HF_PERM2):        return set_vflgs(ap,""    ,0,""      ,"msao"   ,4,0,"0000" );
        case(DEL_PERM):        return set_vflgs(ap,""    ,0,""      ,""       ,0,0,"0000" );
        case(DEL_PERM2):       return set_vflgs(ap,""    ,0,""      ,""       ,0,0,"0000" );
        case(TWIXT):
            switch(mode) {
                case(TRUE_EDIT):     return set_vflgs(ap,""  ,0,""      ,""       ,0,0,""     );
                case(IN_SEQUENCE):   return set_vflgs(ap,"w" ,1,"i"     ,"r"      ,1,0,"0"    );
                case(RAND_REORDER):  return set_vflgs(ap,"w" ,1,"i"     ,"r"      ,1,0,"0"    );
                case(RAND_SEQUENCE): return set_vflgs(ap,"w" ,1,"i"     ,"r"      ,1,0,"0"    );
                default:
                    sprintf(errstr,"Unknown mode (%d) for TWIXT in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
        case(SPHINX):
            switch(mode) {
                case(IN_SEQUENCE):   return set_vflgs(ap,"w" ,1,"i"     ,"r"      ,1,0,"0"    );
                case(RAND_REORDER):  return set_vflgs(ap,"w" ,1,"i"     ,"r"      ,1,0,"0"    );
                case(RAND_SEQUENCE): return set_vflgs(ap,"w" ,1,"i"     ,"r"      ,1,0,"0"    );
                default:
                    sprintf(errstr,"Unknown mode (%d) for SPHINX in set_vflgs()\n",mode);
                    return(PROGRAM_ERROR);
            }
            break;
            //TW NEW
        case(MIX_ON_GRID):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(AUTOMIX):       return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(NOISE_SUPRESS): return set_vflgs(ap,""    ,0,""      ,"n"      ,1,0,"0"   );
        case(TIME_GRID):     return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(SEQUENCER2):    return set_vflgs(ap,""    ,0,""      ,"s"      ,1,1,"d"   );
        case(SEQUENCER):     return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(CONVOLVE):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(BAKTOBAK):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(ADDTOMIX):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(MIX_PAN):       return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(SHUDDER):       return set_vflgs(ap,""    ,0,""      ,"b"      ,1,0,"0"   );
        case(MIX_AT_STEP):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(FIND_PANPOS):   return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""    );
        case(CLICK):
            switch(mode) {
                case(CLICK_BY_TIME): return set_vflgs(ap,"sez" ,3,"ddi"   ,"t"      ,1,0,"0"   );
                case(CLICK_BY_LINE): return set_vflgs(ap,"sez" ,3,"iii"   ,"t"      ,1,0,"0"   );
            }
            break;
        case(DOUBLETS):        return set_vflgs(ap,""    ,0,""      ,"s"      ,1,0,"0"     );
        case(SYLLABS):         return set_vflgs(ap,""    ,0,""      ,"p"      ,1,0,"0"     );
        case(JOIN_SEQDYN):     return set_vflgs(ap,"w"   ,1,"i"     ,"be"     ,2,0,"00"    );
        case(JOIN_SEQ):        return set_vflgs(ap,"wm"  ,2,"di"    ,"be"     ,2,0,"00"    );
        case(MAKE_VFILT):      return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""      );
        case(BATCH_EXPAND):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""      );
        case(MIX_MODEL):       return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""      );
        case(CYCINBETWEEN):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""      );
        case(ENVSYN):          return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""      );
        case(RRRR_EXTEND):
            switch(mode) {
                case(0):           return set_vflgs(ap,""    ,0,""      ,"se"     ,2,0,"00"    );
                case(1):           return set_vflgs(ap,""    ,0,""      ,"se"     ,2,0,"00"    );
                case(2):           return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""      );
            }
            break;
        case(SSSS_EXTEND):     return set_vflgs(ap,"g"   ,1,"d"     ,"x"      ,1,0,"0"     );
        case(HOUSE_GATE2):     return set_vflgs(ap,""    ,0,""      ,"s"      ,1,0,"0"     );
        case(GRAIN_ASSESS):    return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""      );
        case(ZCROSS_RATIO):    return set_vflgs(ap,"se"  ,2,"dd"    ,""       ,0,0,""      );
        case(GREV):            return set_vflgs(ap,""    ,0,""      ,""       ,0,0,""      );
            
            /*|      | |        |         |v| |       */
            /*|      |o|        |         |f|v|       */
            /*|option|p| option |variant  |l|p|variant*/
            /*|flags |t|  list  | flags   |a|a|list   */
            /*|      |c|        |         |g|r|       */
            /*|      |n|        |         |c|a|       */
            /*|      |t|        |         |n|m|       */
            /*|      | |        |         |t|s|       */
    }
    sprintf(errstr,"Unknown process %d: set_vflgs()\n",process);
    return(PROGRAM_ERROR);
}

/****************************** SET_VFLGS *********************************/

int set_vflgs
(aplptr ap,char *optflags,int optcnt,char *optlist,char *varflags,int vflagcnt, int vparamcnt,char *varlist)
{
    ap->option_cnt   = (char) optcnt;           /*RWD added cast */
    if(optcnt) {
        if((ap->option_list = (char *)malloc((size_t)(optcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for option_list\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->option_list,optlist);
        if((ap->option_flags = (char *)malloc((size_t)(optcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for option_flags\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->option_flags,optflags);
    }
    ap->vflag_cnt = (char) vflagcnt;
    ap->variant_param_cnt = (char) vparamcnt;
    if(vflagcnt) {
        if((ap->variant_list  = (char *)malloc((size_t)(vflagcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for variant_list\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->variant_list,varlist);
        if((ap->variant_flags = (char *)malloc((size_t)(vflagcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for variant_flags\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->variant_flags,varflags);
    }
    return(FINISHED);
}

/****************************** SET_PARAM_DATA *********************************/

int set_param_data(aplptr ap, int special_data,int maxparamcnt,int paramcnt,char *paramlist)
{
    ap->special_data   = (char)special_data;
    ap->param_cnt      = (char)paramcnt;
    ap->max_param_cnt  = (char)maxparamcnt;
    if(ap->max_param_cnt>0) {
        if((ap->param_list = (char *)malloc((size_t)(ap->max_param_cnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for param_list\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->param_list,paramlist);
    }
    return(FINISHED);
}

