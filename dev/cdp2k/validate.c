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



/* floatsam version */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/*RWD*/
#include <memory.h>
#include <structures.h>
#include <globcon.h>
#include <tkglobals.h>
#include <processno.h>
#include <filetype.h>



void validate(int applicno,int *valid);

/******************************* ALLOCATE_AND_INITIALISE_VALIDITY_FLAGS *******************************/

int allocate_and_initialise_validity_flags(int **valid,int *validcnt)
{
    int k = sizeof(int) * CHARBITSIZE;
    *validcnt = (MAX_PROCESS_NO + k - 1)/k;
    if((*valid = (int *)malloc((*validcnt) * sizeof(int)))==NULL) {
        sprintf(errstr,"ERROR: INSUFFICIENT MEMORY for validation flags\n");
        return(MEMORY_ERROR);
    }
    memset((char *)(*valid),0,(*validcnt) * sizeof(int));
    return(FINISHED);
}

/******************************* VALIDATE *******************************/

void validate(int process,int *valid)
{
    int k = sizeof(int) * CHARBITSIZE;
    int flagno = (process - 1) / k; /* TRUNCATE */  /* e.g. 36 -> flag 2 (counted from zero)  */
    int bitno  = (process - 1) % k;                 /* e.g. 36 -> bit 3  (counted from zero) */
    int mask = 1;
    if(bitno > 0)
        mask <<= bitno;
    valid[flagno] |= mask;
}

/******************************* VALID_APPLICATION *******************************/

int valid_application(int process,int *valid)
{
    int k = sizeof(int) * CHARBITSIZE;
    int flagno = (process - 1) / k; /* TRUNCATE */  /* e.g. 36 -> flag 2 (counted from zero) */
    int bitno  = (process - 1) % k;                 /* e.g. 36 -> bit 3  (counted from zero) */
    int mask = 1;
    if(bitno > 0)
        mask <<= bitno;
    return(valid[flagno] & mask);
}

/******************************* ESTABLISH_APPLICATION_VALIDITIES *******************************/

int establish_application_validities(int filetype,int channels,int *valid)
{
    /* VALID REGARDLESS OF INFILE TYPE */
    validate(ENV_CREATE,valid);
    validate(MIXFORMAT,valid);
    validate(HOUSE_DISK,valid); 
    validate(HOUSE_BUNDLE,valid);   
    validate(INFO_PROPS,valid);
    validate(INFO_SFLEN,valid);
    validate(INFO_MAXSAMP,valid);
    validate(INFO_MAXSAMP2,valid);
    validate(INFO_DIFF,valid);
    validate(INFO_MUSUNITS,valid);
    validate(SYNTH_WAVE,valid);
    validate(MULTI_SYN,valid);
    validate(SYNTH_NOISE,valid);
    validate(SYNTH_SIL,valid);
    validate(HOUSE_DEL,valid);
//TW NEW CASES
    validate(P_GEN,valid);  
    validate(CLICK,valid);  
    validate(ENVSYN,valid);
    validate(SIN_TAB,valid);
    validate(SYNTH_SPEC,valid);
    /* VALIDITY DEPENDS ON INFILE TYPE */
    switch(filetype) {
    case(ANALFILE):

        validate(HOUSE_COPY,valid); 
        validate(ACCU,valid);
        validate(ALT,valid);
        validate(ARPE,valid);
        validate(AVRG,valid);
        validate(BARE,valid);
        validate(BLTR,valid);
        validate(BLUR,valid);
        validate(BRIDGE,valid);
        validate(CHANNEL,valid);
        validate(CHORD,valid);
        validate(CHORUS,valid);
        validate(CLEAN,valid);
        validate(CROSS,valid);
        validate(CUT,valid);
        validate(DIFF,valid);
        validate(DRUNK,valid);
        validate(EXAG,valid);
        validate(FILT,valid);
        validate(FOCUS,valid);
        validate(FOLD,valid);
        validate(FORM,valid);
        validate(FORMANTS,valid);
        validate(FORMSEE,valid);
        validate(FREEZE,valid);
        validate(FREEZE2,valid);
        validate(FREQUENCY,valid);
        validate(GAIN,valid);
        validate(GLIDE,valid);
        validate(GLIS,valid);
        validate(GRAB,valid);
        validate(GREQ,valid);
        validate(INVERT,valid);
        validate(LEAF,valid);
        validate(LEVEL,valid);
        validate(LIMIT,valid);
        validate(MAGNIFY,valid);
        validate(MAX,valid);
        validate(MEAN,valid);
        validate(MORPH,valid);
        validate(MULTRANS,valid);
        validate(NOISE,valid);
        validate(OCT,valid);
        validate(OCTVU,valid);
        validate(PEAK,valid);
        validate(PICK,valid);
        validate(PITCH,valid);
        validate(PLUCK,valid);
        validate(PRINT,valid);
        validate(REPORT,valid);
        validate(SCAT,valid);
        validate(SHIFT,valid);
        validate(SHIFTP,valid);
        validate(SHUFFLE,valid);    
        validate(SPLIT,valid);      
        validate(SPREAD,valid); 
        validate(STEP,valid);       
        validate(STRETCH,valid);    
        validate(SUM,valid);        
        validate(SUPR,valid);       
        validate(S_TRACE,valid);        
/***
        validate(TRACK,valid);      
***/
        validate(TRNSF,valid);
        validate(TRNSP,valid);      
        validate(TSTRETCH,valid);   
        validate(TUNE,valid);       
        validate(VOCODE,valid); 
/***
        validate(WARP,valid);       
***/
        validate(WAVER,valid);      
        validate(WEAVE,valid);      
        validate(WINDOWCNT,valid);  
        validate(PVOC_SYNTH,valid); 
//TW NEW CASES
        validate(ANALENV,valid);
        validate(VFILT,valid);

        break;
    case(PITCHFILE):
        validate(HOUSE_COPY,valid); 
        validate(MAKE,valid);       
        validate(P_FIX,valid);      
        validate(P_APPROX,valid);   
        validate(P_INVERT,valid);   
        validate(P_QUANTISE,valid);  
        validate(P_RANDOMISE,valid); 
        validate(P_SMOOTH,valid);    
//TW NEW CASES
        validate(P_SYNTH,valid);
        validate(P_VOWELS,valid);
        validate(P_INSERT,valid);
        validate(P_SINSERT,valid);
        validate(P_PTOSIL,valid);
        validate(P_NTOSIL,valid);
        validate(MAKE2,valid);
        validate(P_INTERP,valid);

        validate(P_VIBRATO,valid);      
        validate(P_EXAG,valid);     
        validate(P_TRANSPOSE,valid);    
        validate(P_HEAR,valid);     
        validate(P_CUT,valid);      
        validate(P_INFO,valid);     
        validate(P_SEE,valid);       
        validate(P_WRITE,valid);        
        validate(P_ZEROS,valid);        
        validate(REPITCH,valid);    
        validate(REPITCHB,valid);
        validate(P_BINTOBRK,valid);
        break;
    case(TRANSPOSFILE):
        validate(HOUSE_COPY,valid); 
        validate(P_SEE,valid);
        validate(P_WRITE,valid);
        validate(REPITCH,valid);
        validate(REPITCHB,valid);
        break;
    case(FORMANTFILE):
        validate(HOUSE_COPY,valid); 
        validate(FMNTSEE,valid);
        break;
    case(SNDFILE):
//TW NEW CASES
        validate(TOPNTAIL_CLICKS,valid);    
        validate(DEL_PERM2,valid);  
        validate(HOUSE_BAKUP,valid);    
        validate(HOUSE_GATE,valid); 
        validate(EDIT_CUTMANY,valid);   
        validate(STACK,valid);  
        validate(SHUDDER,valid);    

        validate(RANDCUTS,valid);
        validate(RANDCHUNKS,valid);
        validate(HOUSE_COPY,valid); 
        validate(HOUSE_CHANS,valid);    
        if(channels==MONO) {
            validate(DISTORT,valid);
            validate(DISTORT_ENV,valid);
            validate(DISTORT_AVG,valid);
            validate(DISTORT_OMT,valid);
            validate(DISTORT_MLT,valid);
            validate(DISTORT_DIV,valid);
            validate(DISTORT_HRM,valid);
            validate(DISTORT_FRC,valid);
            validate(DISTORT_REV,valid);
            validate(DISTORT_SHUF,valid);
            validate(DISTORT_RPT,valid);
            validate(DISTORT_RPT2,valid);
            validate(DISTORT_RPTFL,valid);
            validate(DISTORT_INTP,valid);
            validate(DISTORT_DEL,valid);
            validate(DISTORT_RPL,valid);
            validate(DISTORT_TEL,valid);
            validate(DISTORT_FLT,valid);
            validate(DISTORT_INT,valid);
            validate(DISTORT_CYCLECNT,valid);
            validate(DISTORT_PCH,valid);
            validate(DISTORT_OVERLOAD,valid);
//TW NEW CASES
            validate(DISTORT_PULSED,valid);
            validate(NOISE_SUPRESS,valid);
            validate(CYCINBETWEEN,valid);
            validate(GREV,valid);
        }
        validate(ZIGZAG,valid);
        validate(LOOP,valid);
        validate(SCRAMBLE,valid);
        validate(ITERATE,valid);
        validate(ITERATE_EXTEND,valid);
        validate(DRUNKWALK,valid);
//TW NEW CASES
        validate(TIME_GRID,valid);
        validate(SEQUENCER2,valid);
        validate(SEQUENCER,valid);
        validate(CONVOLVE,valid);
        validate(BAKTOBAK,valid);

        if(channels==MONO) {
            validate(SIMPLE_TEX,valid);
            validate(GROUPS,valid);
            validate(MOTIFS,valid);
            validate(MOTIFSIN,valid);
            validate(DECORATED,valid);
            validate(PREDECOR,valid);
            validate(POSTDECOR,valid);
            validate(ORNATE,valid);
            validate(PREORNATE,valid);
            validate(POSTORNATE,valid);
            validate(TIMED,valid);
            validate(TGROUPS,valid);
            validate(TMOTIFS,valid);
            validate(TMOTIFSIN,valid);
            validate(PVOC_ANAL,valid);  
            validate(PVOC_EXTRACT,valid);
            validate(RRRR_EXTEND,valid);
            validate(SSSS_EXTEND,valid);
        }
        validate(GRAIN_COUNT,valid);
        validate(GRAIN_OMIT,valid);
        validate(GRAIN_DUPLICATE,valid);
        validate(GRAIN_REORDER,valid);
        validate(GRAIN_REPITCH,valid);
        validate(GRAIN_RERHYTHM,valid);
        validate(GRAIN_REMOTIF,valid);
        validate(GRAIN_TIMEWARP,valid);
        validate(GRAIN_GET,valid);
        validate(GRAIN_POSITION,valid);
        validate(GRAIN_ALIGN,valid);
        validate(GRAIN_REVERSE,valid);
        validate(ENV_EXTRACT,valid);
        validate(ENV_IMPOSE,valid);
//TW NEW CASE
        validate(ENV_PROPOR,valid);

        validate(ENV_REPLACE,valid);
        validate(ENV_WARPING,valid);
        validate(ENV_DOVETAILING,valid);
        validate(ENV_CURTAILING,valid);
        validate(ENV_SWELL,valid);
        validate(ENV_ATTACK,valid);
        if(channels==1)
            validate(ENV_PLUCK,valid);
        validate(ENV_TREMOL,valid);
        validate(MIXTWO,valid);
//TW NEW CASES
        validate(MIX_AT_STEP,valid);
        validate(MIXMANY,valid);

        validate(MIXBALANCE,valid);
        validate(MIXCROSS,valid);
        if(channels==1)
            validate(MIXINTERL,valid);
        validate(MIXINBETWEEN,valid);
        validate(EQ,valid);     
        validate(LPHP,valid);       
        validate(FSTATVAR,valid);   
        validate(FLTBANKN,valid);  
        validate(FLTBANKC,valid);
        validate(FLTBANKU,valid);   
        validate(FLTBANKV,valid);   
        validate(FLTBANKV2,valid);  
        validate(FLTITER,valid);    
        validate(FLTSWEEP,valid);   
        validate(ALLPASS,valid);    
        validate(MOD_LOUDNESS,valid);   
        validate(MOD_SPACE,valid);  
//TW NEW CASE
        validate(SCALED_PAN,valid); 

        validate(MOD_PITCH,valid);  
        validate(MOD_REVECHO,valid);    
        validate(MOD_RADICAL,valid);    
        validate(BRASSAGE,valid);   
        validate(SAUSAGE,valid);    
        if(channels==MONO) {
            validate(EDIT_ZCUT,valid);
            validate(MANY_ZCUTS,valid);
        }
        validate(EDIT_CUT,valid);   
        validate(EDIT_CUTEND,valid);    
        validate(EDIT_EXCISE,valid);    
        validate(EDIT_EXCISEMANY,valid);    
        validate(INSERTSIL_MANY,valid); 
        validate(EDIT_INSERT,valid);    
//TW NEW CASE
        validate(EDIT_INSERT2,valid);   

        validate(EDIT_INSERTSIL,valid); 
        validate(EDIT_JOIN,valid);
        validate(JOIN_SEQ,valid);   
        validate(JOIN_SEQDYN,valid);
        validate(HOUSE_COPY,valid); 
        validate(HOUSE_SPEC,valid); 
        validate(HOUSE_EXTRACT,valid);  
        validate(INFO_TIMELIST,valid);
        validate(INFO_LOUDLIST,valid);
        validate(INFO_TIMESUM,valid);
        validate(INFO_TIMEDIFF,valid);
        validate(INFO_SAMPTOTIME,valid);
        validate(INFO_TIMETOSAMP,valid);
        validate(INFO_LOUDCHAN,valid);
        validate(INFO_FINDHOLE,valid);
        validate(INFO_CDIFF,valid);
        validate(INFO_PRNTSND,valid);
        validate(TWIXT,valid);
        validate(SPHINX,valid);
        validate(MIXDUMMY,valid);
//TW NEW CASES
        validate(MIX_ON_GRID,valid);
        validate(AUTOMIX,valid);
        validate(DOUBLETS,valid);
        validate(SYLLABS,valid);
        validate(HOUSE_GATE2,valid);
        if(channels==2)
            validate(FIND_PANPOS,valid);
        validate(GRAIN_ASSESS,valid);
        validate(ZCROSS_RATIO,valid);
        validate(ACC_STREAM,valid);
        break;

    case(ENVFILE):
        validate(HOUSE_COPY,valid); 
        validate(ENV_RESHAPING,valid);
        validate(ENV_ENVTOBRK,valid);
        validate(ENV_ENVTODBBRK,valid);
        break;
    case(SNDLIST_OR_SYNCLIST_LINELIST_OR_WORDLIST):    /* list of snds AT SAME SRATE */
    case(SNDLIST_OR_SYNCLIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid);
        validate(MIXSYNC,valid);
        validate(MIXSYNCATT,valid);
        validate(HOUSE_SORT,valid); 
        validate(WORDCNT,valid);
        break;
    case(SNDLIST_OR_LINELIST_OR_WORDLIST):   /* list of snds NOT ALL at same srate */
    case(SNDLIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(HOUSE_SORT,valid); 
        validate(WORDCNT,valid);
        break;
    case(MIXLIST_OR_LINELIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(MIX,valid);
        validate(MIX_MODEL,valid);
        validate(MIXTEST,valid);
        validate(MIXMAX,valid);
        validate(MIXTWARP,valid);
        validate(MIXSWARP,valid);
        validate(MIXGAIN,valid);
        validate(MIXSHUFL,valid);
        validate(MIXSYNC,valid);
        validate(MIXSYNCATT,valid);
//TW NEW CASES
        validate(ADDTOMIX,valid);

        validate(WORDCNT,valid);
//TW NEW CASES
        validate(MIX_PAN,valid);

        break;
    case(MIXLIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(MIX,valid);
        validate(MIX_MODEL,valid);
        validate(MIXTEST,valid);
        validate(MIXMAX,valid);
        validate(MIXTWARP,valid);
        validate(MIXSWARP,valid);
        validate(MIXGAIN,valid);
        validate(MIXSHUFL,valid);
        validate(MIXSYNC,valid);
        validate(MIXSYNCATT,valid);
//TW NEW CASE
        validate(ADDTOMIX,valid);

        validate(WORDCNT,valid);
//TW NEW CASE
        validate(MIX_PAN,valid);
        
        break;
    case(SYNCLIST_OR_LINELIST_OR_WORDLIST):  /* i.e. NOT a sndlist */
        validate(HOUSE_COPY,valid); 
        validate(MIXSYNCATT,valid);
        validate(HOUSE_SORT,valid); 
        validate(WORDCNT,valid);
        break;
    case(SYNCLIST_OR_WORDLIST):              /* i.e. NOT a sndlist */
        validate(HOUSE_COPY,valid); 
        validate(MIXSYNCATT,valid);
        validate(HOUSE_SORT,valid); 
        validate(WORDCNT,valid);
        break;
    case(LINELIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(HOUSE_SORT,valid); 
        validate(WORDCNT,valid);
        validate(BATCH_EXPAND,valid);
        break;
    case(WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(HOUSE_SORT,valid); 
        validate(WORDCNT,valid);
        break;
    case(TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(REPITCH,valid);
        validate(REPITCHB,valid);
        validate(ENV_REPLOTTING,valid);
        validate(ENV_BRKTOENV,valid);
        validate(ENV_BRKTODBBRK,valid);
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(REPITCH,valid);
        validate(REPITCHB,valid);
        validate(ENV_REPLOTTING,valid);
        validate(ENV_BRKTOENV,valid);
        validate(ENV_BRKTODBBRK,valid);
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(REPITCH,valid);
        validate(REPITCHB,valid);
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(REPITCH,valid);
        validate(REPITCHB,valid);
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(REPITCH,valid);
        validate(REPITCHB,valid);
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(REPITCH,valid);
        validate(REPITCHB,valid);
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        validate(HF_PERM1,valid);   
        validate(HF_PERM2,valid);   
        validate(DEL_PERM,valid);
        validate(DEL_PERM2,valid);
        validate(ENV_IMPOSE,valid); 
        /* fall thro */
    case(PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(REPITCH,valid);
        validate(REPITCHB,valid);
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        validate(HF_PERM1,valid);   
        validate(HF_PERM2,valid);   
        validate(DEL_PERM,valid);   
        validate(DEL_PERM2,valid);  
        validate(ENV_IMPOSE,valid); 
        /* fall thro */
    case(PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(REPITCH,valid);
        validate(REPITCHB,valid);
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(ENV_REPLOTTING,valid);
        validate(ENV_BRKTOENV,valid);
        validate(ENV_BRKTODBBRK,valid);
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(DB_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(ENV_DBBRKTOENV,valid);
        validate(ENV_DBBRKTOBRK,valid);
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        validate(HF_PERM1,valid);   
        validate(HF_PERM2,valid);   
        validate(DEL_PERM,valid);   
        validate(DEL_PERM2,valid);  
        /* fall thro */
    case(UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(MOD_SPACE,valid);  
        validate(WORDCNT,valid);
        break;
    case(NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(NUMLIST_OR_WORDLIST):
        validate(HOUSE_COPY,valid); 
        validate(WORDCNT,valid);
        validate(HF_PERM1,valid);
        validate(HF_PERM2,valid);
        validate(DEL_PERM,valid);
        validate(DEL_PERM2,valid);
        validate(MAKE_VFILT,valid);
        break;
    default:
        sprintf(errstr,"Unknown filetype [%d]: establish_application_validities()\n",filetype);
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

