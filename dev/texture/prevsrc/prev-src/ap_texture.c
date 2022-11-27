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

/* 03/2020: TW/RWD: corrected usage message argument lists */

/* floatsam version: no changes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <texture.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
#include <mixxcon.h>
#include <flags.h>
#include <speccon.h>
#include <arrays.h>
#include <special.h>
#include <formants.h>
#include <sfsys.h>
#include <osbind.h>

#include <srates.h>

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

static int  texture_pconsistency(dataptr dz);

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

static int  get_the_notedatafile(char *filename,dataptr dz);

/***************************************************************************************/
/****************************** FORMERLY IN aplinit.c **********************************/
/***************************************************************************************/

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
    /*int is_spec = FALSE;*/
    dz->bptrcnt = 0;
    dz->bufcnt  = 0;
    switch(dz->process) {
    case(SIMPLE_TEX):
    case(TIMED):
    case(GROUPS):
    case(TGROUPS):
    case(DECORATED):
    case(PREDECOR):
    case(POSTDECOR):
    case(ORNATE):
    case(PREORNATE):
    case(POSTORNATE):
    case(MOTIFS):
    case(TMOTIFS):
    case(MOTIFSIN):
    case(TMOTIFSIN):
        dz->extra_bufcnt = 0;   dz->bufcnt = 1;
        break;
    default:
        sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
        return(PROGRAM_ERROR);
    }
    return establish_groucho_bufptrs_and_extra_buffers(dz);
}

/***************************** SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS **************************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
    int n;
    dz->ptr_cnt    = -1;            /* base constructor...process */
    dz->array_cnt  = -1;
    dz->iarray_cnt = -1;
    dz->larray_cnt = -1;
    switch(dz->process) {
    case(SIMPLE_TEX):
    case(TIMED):
    case(GROUPS):
    case(TGROUPS):
    case(DECORATED):
    case(PREDECOR):
    case(POSTDECOR):
    case(ORNATE):
    case(PREORNATE):
    case(POSTORNATE):
    case(MOTIFS):
    case(TMOTIFS):
    case(MOTIFSIN):
    case(TMOTIFSIN):
        dz->array_cnt = 1; dz->iarray_cnt = 6; dz->larray_cnt = 0; dz->ptr_cnt = 0;     dz->fptr_cnt = 0;
        break;
    }

    /*** WARNING ***
         ANY APPLICATION DEALING WITH A NUMLIST INPUT: MUST establish AT LEAST 1 double array: i.e. dz->array_cnt = at least 1
         **** WARNING ***/


    if(dz->array_cnt < 0 || dz->iarray_cnt < 0 || dz->larray_cnt < 0 || dz->ptr_cnt < 0 || dz->fptr_cnt < 0) {
        sprintf(errstr,"array_cnt not set in setup_internal_arrays_and_array_pointers()\n");
        return(PROGRAM_ERROR);
    }

    if(dz->array_cnt > 0) {
        if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
            return(MEMORY_ERROR);
        }
        for(n=0;n<dz->array_cnt;n++)
            dz->parray[n] = NULL;
    }
    if(dz->iarray_cnt > 0) {
        if((dz->iparray = (int     **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
            return(MEMORY_ERROR);
        }
        for(n=0;n<dz->iarray_cnt;n++)
            dz->iparray[n] = NULL;
    }
    if(dz->larray_cnt > 0) {
        if((dz->lparray = (int    **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
            return(MEMORY_ERROR);
        }
        for(n=0;n<dz->larray_cnt;n++)
            dz->lparray[n] = NULL;
    }
    if(dz->ptr_cnt > 0)   {
        if((dz->ptr     = (double  **)malloc(dz->ptr_cnt  * sizeof(double *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for internal pointer arrays.\n");
            return(MEMORY_ERROR);
        }
        for(n=0;n<dz->ptr_cnt;n++)
            dz->ptr[n] = NULL;
    }
    if(dz->fptr_cnt > 0)   {
        if((dz->fptr = (float **)malloc(dz->fptr_cnt * sizeof(float *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for internal float-pointer arrays.\n");
            return(MEMORY_ERROR);
        }
        for(n=0;n<dz->fptr_cnt;n++)
            dz->fptr[n] = NULL;
    }
    return(FINISHED);
}

/****************************** ASSIGN_PROCESS_LOGIC *********************************/

int assign_process_logic(dataptr dz)
{
    switch(dz->process) {
    case(SIMPLE_TEX):
    case(TIMED):
    case(GROUPS):
    case(TGROUPS):
    case(DECORATED):
    case(PREDECOR):
    case(POSTDECOR):
    case(ORNATE):
    case(PREORNATE):
    case(POSTORNATE):
    case(MOTIFS):
    case(TMOTIFS):
    case(MOTIFSIN):
    case(TMOTIFSIN):
        setup_process_logic(ONE_OR_MANY_SNDFILES,       UNEQUAL_SNDFILE,        SNDFILE_OUT,    dz);
        break;
    default:
        sprintf(errstr,"Unknown process: assign_process_logic()\n");
        return(PROGRAM_ERROR);
        break;
    }
    if(dz->has_otherfile) {
        switch(dz->input_data_type) {
        case(ALL_FILES):
        case(TWO_SNDFILES):
        case(SNDFILE_AND_ENVFILE):
        case(SNDFILE_AND_BRKFILE):
        case(SNDFILE_AND_UNRANGED_BRKFILE):
        case(SNDFILE_AND_DB_BRKFILE):
            break;
        case(MANY_SNDFILES):
            if(dz->process==INFO_TIMELIST)
                break;
            /* fall thro */
        default:
            sprintf(errstr,"Most processes accepting files with different properties\n"
                    "can only take 2 sound infiles.\n");
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/***************************** SET_LEGAL_INFILE_STRUCTURE **************************
 *
 * Allows 2nd infile to have different props to first infile.
 */

void set_legal_infile_structure(dataptr dz)
{
    switch(dz->process) {
    default:
        dz->has_otherfile = FALSE;
        break;
    }
}

/***************************************************************************************/
/****************************** FORMERLY IN internal.c *********************************/
/***************************************************************************************/

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
    int exit_status = FINISHED;
    switch(process) {
    case(SIMPLE_TEX):       case(TIMED):    case(GROUPS):   case(TGROUPS):
    case(DECORATED):        case(PREDECOR): case(POSTDECOR):
    case(ORNATE):           case(PREORNATE):case(POSTORNATE):
    case(MOTIFS):           case(TMOTIFS):  case(MOTIFSIN): case(TMOTIFSIN):
        exit_status = set_internalparam_data("iiidddi",ap);                     break;
    default:
        sprintf(errstr,"Unknown process in set_legal_internalparam_structure()\n");
        return(PROGRAM_ERROR);
    }
    return(exit_status);
}

/********************************************************************************************/
/********************************** FORMERLY IN specialin.c *********************************/
/********************************************************************************************/

/********************** READ_SPECIAL_DATA ************************/

int read_special_data(char *str,dataptr dz)
{
    /*      int exit_status = FINISHED;*/
    aplptr ap = dz->application;
    switch(ap->special_data) {
    case(TEX_NOTEDATA):                     return get_the_notedatafile(str,dz);
    default:
        sprintf(errstr,"Unknown special_data type: read_special_data()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);       /* NOTREACHED */
}

/***************************** GET_THE_NOTEDATAFILE ****************************/

int get_the_notedatafile(char *filename,dataptr dz)
{
    if((dz->fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Failed to open notedata file %s\n",filename);
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)
{
    /*      int exit_status = FINISHED;*/

    switch(dz->process) {
    case(SIMPLE_TEX):       case(TIMED):    case(GROUPS):   case(TGROUPS):
    case(DECORATED):        case(PREDECOR): case(POSTDECOR):
    case(ORNATE):           case(PREORNATE):case(POSTORNATE):
    case(MOTIFS):           case(TMOTIFS):  case(MOTIFSIN): case(TMOTIFSIN):
        return texture_preprocess(dz);
    default:
        sprintf(errstr,"PROGRAMMING PROBLEM: Unknown process in param_preprocess()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);       /* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN procgrou.c **********************************/
/********************************************************************************************/

/**************************** GROUCHO_PROCESS_FILE ****************************/

int groucho_process_file(dataptr dz)   /* FUNCTIONS FOUND IN PROCESS.C */
{
    int exit_status = FINISHED;

    switch(dz->process) {
    case(SIMPLE_TEX):       case(TIMED):    case(GROUPS):   case(TGROUPS):
    case(DECORATED):        case(PREDECOR): case(POSTDECOR):
    case(ORNATE):           case(PREORNATE):case(POSTORNATE):
    case(MOTIFS):           case(MOTIFSIN): case(TMOTIFS):  case(TMOTIFSIN):
        if((exit_status = make_texture(dz))<0)
            return(exit_status);
        if((exit_status = produce_texture_sound(dz))<0)
            return(exit_status);
        break;
    default:
        sprintf(errstr,"Unknown case in process_file()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
    /*      int exit_status = FINISHED;*/
    handle_pitch_zeros(dz);
    switch(dz->process) {
    case(SIMPLE_TEX): case(TIMED):          case(GROUPS):   case(TGROUPS):
    case(DECORATED):  case(PREDECOR):       case(POSTDECOR):
    case(ORNATE):     case(PREORNATE):      case(POSTORNATE):
    case(MOTIFS):     case(TMOTIFS):        case(MOTIFSIN): case(TMOTIFSIN):
        return texture_pconsistency(dz);
    }
    return(FINISHED);
}

/***************************** PREPARE_TEXTURE_PARAMETERS ****************************/

int texture_pconsistency(dataptr dz)
{
    double *p, *end;
    int OK = TRUE;
    if(dz->brksize[TEXTURE_INSLO]) {
        p = dz->brk[TEXTURE_INSLO] + 1;
        end = dz->brk[TEXTURE_INSLO] + (dz->brksize[TEXTURE_INSLO] * 2);
        while(p < end) {
            if(*p  > (double)dz->infilecnt) {
                OK = FALSE;
                break;
            }
            p += 2;
        }
    } else if(dz->iparam[TEXTURE_INSLO] > dz->infilecnt)
        OK  = FALSE;
    if(!OK) {
        sprintf(errstr,"FIRST SND-IN-LIST TO USE > count of files entered: cannot proceed.\n");
        return(DATA_ERROR);
    }
    OK = TRUE;
    if(dz->brksize[TEXTURE_INSHI]) {
        p = dz->brk[TEXTURE_INSHI] + 1;
        end = dz->brk[TEXTURE_INSHI] + (dz->brksize[TEXTURE_INSHI] * 2);
        while(p < end) {
            if(*p  > (double)dz->infilecnt) {
                OK = FALSE;
                break;
            }
            p += 2;
        }
    } else if(dz->iparam[TEXTURE_INSHI] > dz->infilecnt)
        OK  = FALSE;
    if(!OK) {
        sprintf(errstr,"LAST SND-IN-LIST TO USE > count of files entered: cannot proceed.\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}



/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
    switch(dz->process) {
    case(SIMPLE_TEX):
    case(TIMED):            case(GROUPS):           case(TGROUPS):
    case(DECORATED):        case(PREDECOR):         case(POSTDECOR):
    case(ORNATE):           case(PREORNATE):        case(POSTORNATE):
    case(MOTIFS):           case(TMOTIFS):          case(MOTIFSIN):
    case(TMOTIFSIN):
        return create_sndbufs(dz);
    default:
        sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);       /* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if     (!strcmp(prog_identifier_from_cmdline,"simple"))         dz->process = SIMPLE_TEX;
    else if(!strcmp(prog_identifier_from_cmdline,"grouped"))        dz->process = GROUPS;
    else if(!strcmp(prog_identifier_from_cmdline,"motifs"))         dz->process = MOTIFS;
    else if(!strcmp(prog_identifier_from_cmdline,"motifsin"))       dz->process = MOTIFSIN;
    else if(!strcmp(prog_identifier_from_cmdline,"decorated"))      dz->process = DECORATED;
    else if(!strcmp(prog_identifier_from_cmdline,"predecor"))       dz->process = PREDECOR;
    else if(!strcmp(prog_identifier_from_cmdline,"postdecor"))      dz->process = POSTDECOR;
    else if(!strcmp(prog_identifier_from_cmdline,"ornate"))         dz->process = ORNATE;
    else if(!strcmp(prog_identifier_from_cmdline,"preornate"))      dz->process = PREORNATE;
    else if(!strcmp(prog_identifier_from_cmdline,"postornate"))     dz->process = POSTORNATE;
    else if(!strcmp(prog_identifier_from_cmdline,"timed"))                  dz->process = TIMED;
    else if(!strcmp(prog_identifier_from_cmdline,"tgrouped"))               dz->process = TGROUPS;
    else if(!strcmp(prog_identifier_from_cmdline,"tmotifs"))                dz->process = TMOTIFS;
    else if(!strcmp(prog_identifier_from_cmdline,"tmotifsin"))              dz->process = TMOTIFSIN;
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        return(USAGE_ONLY);
    }

    return FINISHED;
}

/********************************************************************************************/
/********************************** FORMERLY IN usage.c *************************************/
/********************************************************************************************/

/******************************** USAGE1 ********************************/

int usage1(void)
{
    fprintf(stdout, /* HAs to use fprintf because of kbhit */
            "\n"
            "USAGE:\ntexture NAME (mode) infile (infile2..etc) outfile notedata params:\n"
            "\n"
            "\nwhere NAME can be any one of\n"
            "\n"
            "\tTEXTURE MADE FROM ONE OR SEVERAL SOUND FILES\n"
            "\n"
            "simple   grouped    decorated    ornate       motifs\n"
            "                    predecor     preornate    motifsin\n"
            "                    postdecor    postornate\n"
            "\n"
            "\tTIMED TEXTURES MADE FROM ONE OR SEVERAL SOUND FILES\n"
            "\n"
            "timed     tmotifs   tmotifsin   tgrouped\n"
            "\n"
            "ORNAMENTS & MOTIFS   have user-specified pitch-shapes.\n"
            "DECORATIONS & GROUPS have random pitch-shapes.\n"
            "\n"
            "Type 'texture simple' for more info on simple texture option.... etc.\n"
            "\n"
#ifdef IS_PC
            "MORE??? ----- (hit keyboard)\n"
            "\n");

    while(!kbhit())
        ;
    if(kbhit()) {
#else
        "\n");
#endif
    fprintf(stdout,
            "\n"
            "*********************************************************************\n"
            "SOME SPECIAL PARAMETER VALUES\n"
            "*********************************************************************\n"
            "GPSPACE: spatialisation of event-groups\n"
            "\n"
            "     0:still            1:scattered(default) 2:towards-texture-centre\n"
            "     3:away-from-centre 4:follow-texmotion   5:contrary-to-motion\n"
            "     4 & 5 only function if spatial position varies in time..\n"
            "*********************************************************************\n"
            "CONTOUR: amplitude contour of groups\n"
            "\n"
            "     0:mixed(default) 1:cresc     2:flat   3:decresc    4:c_or_fl\n"
            "     5:c_or_d         6:d_or_fl  [7:directed_to_event   8:dir_or_flat]\n"
            "*********************************************************************\n"
            "CENTRING: how decoration pitches centre on decorated line pitches\n"
            "\n"
            "     0: centred(default) 1: above       2: below\n"
            "     3: c_and_a          4: c_and_b     5: a_and_b     6: c_and_a_and_b\n"
            "     In all cases except 0, pitchrange shifted to tally with line pitch\n\n"
            "\n"
            "*********************************************************************\n"
            "       ALL OTHER PARAMS, except outdur, MAY VARY IN TIME.\n"
            "*********************************************************************\n"
            "\n"

#ifdef IS_PC
            "MORE??? ----- (hit keyboard)\n");
}
_getch();
while(!kbhit())
    ;
if(kbhit()) {
#else
    "\n");
#endif

fprintf(stdout,
        "\n"
        "*********************************************************************\n"
        "HARMONIC SETS   use only the pitches specified.\n"
        "\n"
        "HARMONIC FIELDS duplicate the specified pitches in all 8vas.\n"
        "*********************************************************************\n"
        "NOTEDATA is in a textfile containing.....\n"
        "*********************************************************************\n"
        "     assumed MIDI 'pitch' of each input snd, specified on 1st line.\n"
        "\n"
        "   FOLLOWED BY, where ness,  NOTELIST(S), SPECIFIED THUS:-\n"
        "\n"
        "     #N (where N = no. of notes in notelist: follows by N lines of...)\n"
        "     time(SECS)   infile_no    pitch(MIDI)    amp(MIDI)   dur(SECS)\n"
        "\n"
        "where times within motif must increase (or remain same, during chords)\n"
        "*********************************************************************\n"
        "NOTELISTS REPRESENT: IN THIS ORDER..\n"
        "*********************************************************************\n"
        "Notelist of notes in any timed, ornamented or decorated line in texture.\n"
        "Notelist of notes in any harmonic field(s) or set(s) specified.\n"
        "      (For more than one  hfield(hset),\n"
        "      data should specify chords, placed at appropriae times.)\n"
        "Notelist(s) of notes in any ornament(s) or motif(s) specified.\n"
        "*********************************************************************\n");

#ifdef IS_PC
}
#endif
return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"simple")) {
        fprintf(stdout,
                "USAGE:\n"
                "texture simple mode infile [infile2...] outfile notedata outdur packing scatter\n"
                "        tgrid sndfirst sndlast  mingain maxgain  mindur maxdur  minpich maxpich\n"
                "             [-aatten] [-pposition] [-sspread] [-rseed] [-w -c -p]\n"
                "MODES:-\n"
                "1)  ON A GIVEN HARMONIC-FIELD        3)  ON A GIVEN HARMONIC-SET\n"
                "2)  ON CHANGING HARMONIC-FIELDS      4)  ON CHANGING HARMONIC-SETS\n"
                "5)  NONE\n"
                "notedata: infofile, insnd 'pitches',harmonic fields pitches etc.\n"
                "outdur:           (min) duration of outfile\n"
                "packing:          (average) time between event onsets.\n"
                "scatter:          randomisation of event-onsets (0 - %.0lf)\n"
                "tgrid:            minstep(MS) quantised timegrid(for grp starttimes)(default 0)\n"
                "sndfirst,sndlast: 1st,last snd to use,from list of insnds(range:1 - no.of snds)\n"
                "mingain,maxgain:  minimum & maximum level of input events (1-127:default 64,64)\n"
                "mindur,maxdur:    minimum & maximum duration of events in texture\n"
                "minpich,maxpich:  minimum & maximum pitch (MIDI VALUE):\n"
                "atten:            overall attenuation of the output\n"
                "position:         centre of output sound-image (0(Left) 1(Right): default 0.5)\n"
                "spread:           spatial-spread of texture events (0 - 1(full-spread))\n"
                "seed:             same seed-no: same output on rerun(dflt 0: differs each time)\n"
                "-w:               always play whole input-sound (ignoring duration values).\n"
                "-c:               Choose files cyclically in listed order (ignore 'sndfirst','last').\n"
                "-p:               Random permute each cycle (only when -c flag set).\n",MAX_SCAT_TEXTURE);
    } else if(!strcmp(str,"timed")) {
        fprintf(stdout,
                "TIMED TEXTURE:  USAGE:\n"
                "texture timed mode infile [infile2...]  outfile  notedata     outdur skiptime\n"
                "             sndfirst sndlast mingain maxgain mindur maxdur minpitch maxpitch\n"
                "             [-aatten] [-pposition] [-sspread] [-rseed] [-w]\n"
                "MODES:-\n"
                "1)  ON A GIVEN HARMONIC-FIELD        3)  ON A GIVEN HARMONIC-SET\n"
                "2)  ON CHANGING HARMONIC-FIELDS      4)  ON CHANGING HARMONIC-SETS\n"
                "5)  NONE\n"
                "notedata: infofile, insnd 'pitches',texture timing,harmonic fields pitches etc.\n"
                "outdur:            (min) duration of outfile\n"
                "skiptime:          time between repetitions of timing motif in notedata\n"
                "sndfirst,sndlast:  1st,last snd to use,from list of insnds(range:1 - all)\n"
                "mingain,maxgain:   min & max level of input events (1-127:default 64,64)\n"
                "mindur,maxdur:     min & max duration of events in texture\n"
                "minpitch,maxpitch: min & max pitch (MIDI VALUE)\n"
                "atten:             overall attenuation of the output\n"
                "position:          centre of output sound-image (0(Left) 1(Right):default 0.5)\n"
                "spread:            spatial-spread of texture events (0 - 1(full-spread))\n"
                "seed:             same seed-no: same output on rerun(dflt 0: differs each time)\n"
                "-w:                always play whole input-sound (ignoring duration values).\n");
    } else if(!strcmp(str,"grouped")) {
        fprintf(stdout,
                "TEXTURE OF EVENT-GROUPS: USAGE: texture grouped mode infile [infile2..] outfile\n"
                "notedata outdur packing scatter tgrid\n"
                "sndfirst sndlast mingain maxgain mindur maxdur minpitch maxpitch phgrid gpspace\n"
                "gpsprange amprise contour gpsizelo gpsizehi gppaklo gppakhi gpranglo gpranghi\n"
                "                   [-aatten] [-pposition] [-sspread] [-rseed] [-w] [-d] [-i]\n"
                "MODES:-\n"
                "1)ON HARMONIC-FIELD 2)CHANGING HFLDS 3)HARMONIC-SET 4)CHANGING HSETS 5)NONE\n"
                "notedata:   infofile, 'pitch' of insnds, harmonic fields etc.\n"
                "outdur:     (min) duration of outfile\n"
                "packing:    (average) time between group onsets.\n"
                "scatter:    randomisation of event-onsets (0 - %.0lf)\n"
                "tgrid:      minstep(MS) quantised timegrid(for grp starttimes)(default 0)\n"
                "sndfirst,sndlast: 1st,last snd to use, from list of insnds(range:1 - all)\n"
                "mingain,maxgain:  min & max level of input events (1-127:default 64,64)\n"
                "mindur,maxdur:    min & max duration of events in texture\n"
                "minpitch,maxpitch:min & max pitch (MIDI VALUE)\n"
                "phgrid:     a timegrid (MS) applying WITHIN the groups\n"
                "gpspace:    spatialisation of event-groups (Range 0-5: default 1)\n"
                "gpsprange:  spatial range of event-groups (Range 0-1: default 1)\n"
                "amprise:    amplitude change within groups: (0-127: default 0)\n"
                "contour:    amplitude contour of groups (Range 0-6: default 0)\n"
                /* NOTE : CARE all_types is 7 AND NOT 8 here */
                "gpsizelo,gpsizehi:smallest,largest no. of events in groups\n"
                "gppaklo,gppakhi:  shortest,longest time between event-onsets in groups (MS)\n"
                "gpranglo,gpranghi:min,max pitchrange grps OR (hfields) no.of hf-notes range\n"
                "atten:      overall attenuation of output\n"
                "position:   centre of output sound-image (0(Left) 1(Right): default 0.5)\n"
                "spread:     spatial-spread of texture events (0 - 1(full-spread))\n"
                "seed:       same seed-no: same output on rerun(dflt 0: differs each time)\n"
                "-w:         always play whole input-sound (ignore maxdur,mindur vals).\n"
                "-d:             fixed timestep between groupnotes.\n"
                "-i:         each group not confined to a fixed instr (default:fixed)\n",MAX_SCAT_TEXTURE);
    } else if(!strcmp(str,"tgrouped")) {
        fprintf(stdout,
                "TIMED TEXTURE OF EVENT-GROUPS: USAGE: texture tgrouped mode infile [infile2..]\n"
                "outfile notedata outdur skip sndfirst sndlast mingain maxgain mindur maxdur\n"
                "minpitch maxpitch phgrid gpspace gpsprange amprise contour gpsizelo gpsizehi\n"
                "gppacklo gppackhi gpranglo gpranghi\n"
                "                   [-aatten] [-pposition] [-sspread] [-rseed] [-w] [-d] [-i]\n"
                "MODES:-\n"
                "1)  ON A GIVEN HARMONIC-FIELD        3)  ON A GIVEN HARMONIC-SET\n"
                "2)  ON CHANGING HARMONIC-FIELDS      4)  ON CHANGING HARMONIC-SETS\n"
                "5)  NONE\n"
                "notedata: infofile, 'pitch' of insnds, timings for timed textures, hfields etc\n"
                "outdur:           (min) duration of outfile\n"
                "skip:             time between repetitions of timing motif in notedata\n"
                "sndfirst,sndlast: 1st,last snd to use from list of insnds(range:1 - all)\n"
                "mingain,maxgain:  min & max level of input events (1-127:default 64,64)\n"
                "mindur,maxdur:    min & max duration of events in texture\n"
                "minpitch,maxpitch:min & max pitch (MIDI VALUE)\n"
                "phgrid:           timegrid (MS) applying WITHIN the groups\n"
                "gpspace:          spatialisation of event-groups (Range 0-5: default 1)\n"
                "gpsprange:        spatial range of event-groups (Range 0-1: default 1)\n"
                "amprise:          amplitude change within groups: (0-127: default 0)\n"
                "contour:          amplitude contour of groups (Range 0-6: default 0)\n"
                /* NOTE : CARE all_types is 7 AND NOT 8 here */
                "gpsizelo,gpsizehi:smallest & largest numbers of events in groups\n"
                "gppacklo,gppackhi:shortest & longest time between event-onsets in groups (MS)\n"
                "gpranglo,gpranghi:min,max pitchrange grps OR (hfields) no.of hf-notes range\n"
                "atten:            overall attenuation of output\n"
                "position:         centre of output sound-image (0(Left) 1(Right): default 0.5)\n"
                "spread:           spatial-spread of texture events (0 - 1(full-spread))\n"
                "seed:             same seed-no: same output on rerun(dflt 0: differs each time)\n"
                "-w:               always play whole input-sound (ignoring duration values).\n"
                "-d:                            fixed timestep between groupnotes.\n"
                "-i:               each group not confined to a fixed instr (default:fixed)\n");
    } else if(!strcmp(str,"decorated") || !strcmp(str,"predecor") || !strcmp(str,"postdecor")) {
        fprintf(stdout,
                "TEXTURE WITH DECORATIONS:USAGE:texture decorated|predecor|postdecor mode infile\n"
                "[infile2..] outfile notedata outdur skiptime sndfirst sndlast mingain maxgain\n"
                "mindur maxdur phgrid gpspace gpsprange amprise contour gpsizlo gpsizhi\n"
                "gppaklo gppakhi gpranglo gpranghi centring\n"
                "              [-aatten] [-ppos] [-ssprd] [-rseed] [-w] [-d] [-i] [-h] [-e] [-k]\n"
                "MODES:-\n"
                "1)ON HARMONIC-FIELD 2)CHANGING HFLDS 3)HARMONIC-SET 4)CHANGING HSETS 5)NONE\n"
                "notedata:         infofile, 'pitch' of insnds, decoration shape etc.\n"
                "outdur:           (min) duration outfile\n"
                "skiptime:         time between repets of motif-to-decorate in notedata\n"
                "sndfirst,sndlast: 1st,last snd to use, from input snds (range 1 - all)\n"
                "mingain,maxgain:  min & max gain on input events (MIDI)\n"
                "mindur,maxdur:    min & max duration of events in texture\n"
                "phgrid:           timegrid (MS) applying WITHIN decors\n"
                "gpspace:          spatialisation decor-groups (Range 0-5: default 1)\n"
                "gpsprange:        spatial range decor-groups (Range 0-1: default 1)\n"
                "amprise:          amplitude change within decors: (0-127: default 0)\n"
                "contour:          amplitude contour of decors (Range 0-8: default 0)\n"
                "gpsizlo,gpsizhi:  smallest,largest no. events in decors\n"
                "gppaklo,gppakhi:  shortest,longest time between event-onsets in decors(MS)\n"
                "gpranglo,gpranghi:min,max pitchrange decors OR (hfields) no.of hf-notes range\n"
                "centring:         how decor pitches centre on line pitches(Range 0-7:default 0)\n"
                "atten:            overall attenuation of output\n"
                "pos:              centre of output sound-image (-1(Left) 1(Right): default 0)\n"
                "sprd:             spatial-spread of texture events (0 - 1(full-spread))\n"
                "seed:             same seed-no: same output on rerun(dflt 0: differs each time)\n"
                "-w: play all insnd(ignore min,maxdur) -h: dec TOPnote chord:(dflt:1st listed)\n"
                "-d: fixed timestep btwn decornotes    -e: dec all notes of chords\n"
                "-i: instrs vary in decor(dflt:fixed)  -k: discard orig line, after decor\n");
    } else if(!strcmp(str,"ornate") || !strcmp(str,"preornate") || !strcmp(str,"postornate")) {
        fprintf(stdout,
                "TEXTURE WITH ORNAMENTS: USAGE:\n"
                "texture ornate|preornate|postornate mode infile [infile2...] outfile notedata\n"
                "outdur skiptime sndfirst sndlast   mingain maxgain  mindur maxdur\n"
                "phgrid gpspace gpsprange  amprise  contour   multlo   multhi\n"
                "          [-aatten] [-pposition] [-sspread] [-rseed] [-w] [-d] [-i] [-h] [-e]\n"
                "MODES:-\n"
                "1)ON HARMONIC-FIELD 2)CHANGING HFLDS 3)HARMONIC-SET 4)CHANGING HSETS 5)NONE\n"
                "notedata:   infofile, 'pitch' of insnds, notes in ornaments, hfields etc.\n"
                "outdur:           (min) duration of outfile\n"
                "skiptime:         time between repetitions of motif-to-ornament in notedata\n"
                "sndfirst,sndlast: 1st,last snd to use, from input snds (range 1 - all)\n"
                "mingain,maxgain:  min & max level of input events (1-127:default 64,64)\n"
                "mindur,maxdur:    min & max duration of events in texture\n"
                "phgrid:           a timegrid (MS) applying WITHIN the ornaments\n"
                "gpspace:          spatialisation of event-groups (Range 0-5: default 1)\n"
                "gpsprange:        spatial range of event-groups (Range 0-1: default 1)\n"
                "amprise:          amplitude change within ornaments: (0-127: default 0)\n"
                "contour:          amplitude contour of groups (Range 0-8: default 0)\n"
                "multlo, multhi:   min & max multiplier of total input duration of motif\n"
                "atten:            overall attenuation of the output\n"
                "position:         centre of output sound-image (0(Left) 1(Right):default 0.5)\n"
                "spread:           spatial-spread of texture events (0 - 1(full-spread))\n"
                "seed:             same seed-no: same output on rerun(dflt 0: differs each time)\n"
                "-w:               always play whole input-sound (ignoring duration values).\n"
                "-d:               ornament notes all have same duration as ornamented note\n"
                "-i:               orns not confined to instr of ornd-note (default:same note)\n"
                "-h:               orns on highest note of any chord:(default:1st note listed)\n"
                "-e:               ornaments on all notes of any chord.\n");
    } else if(!strcmp(str,"motifsin")) {
        fprintf(stdout,
                "TEXTURE OF MOTIFS, FORCED ONTO A HARMONIC FIELD: USAGE:\n"
                "texture motifsin mode infile [infile2..] outfile notedata outdur packing\n"
                "scatter tgrid sndfirst sndlast mingain maxgain\n"
                "minpitch maxpitch phgrid gpspace gpsprange amprise  contour multlo multhi\n"
                "                [-aatten] [-pposition] [-sspread] [-rseed] [-w] [-d] [-i]\n"
                "MODES:-\n"
                "1) ON HARMONIC-FIELD 2) CHANGING HFLDS 3) HARMONIC-SET 4) CHANGING HSETS\n"
                "notedata: infofile,'pitch' of insnds, timings for timed textures, hfields etc\n"
                "outdur:             (min) duration of outfile\n"
                "packing:            (average) time between motif onsets.\n"
                "scatter:            randomisation of event-onsets (0 - %.0lf)\n"
                "tgrid:              minstep(MS) quantised timegrid (for mtf starttimes)(dflt 0)\n"
                "sndfirst,sndlast:   1st,last snd to use, from input snds (range 1 - all)\n"
                "mingain,maxgain:    min & max level of input events (1-127:default 64,64)\n"
                "minpitch,maxpitch:  min & max pitch (MIDI VALUE)\n"
                "phgrid:             a timegrid (MS) applying WITHIN the motifs\n"
                "gpspace:            spatialisation of event-groups (Range 0-5: default 1)\n"
                "gpsprange:          spatial range of event-groups (Range 0-1: default 1)\n"
                "amprise:            amplitude change within motifs: (0-127: default 0)\n"
                "contour:            amplitude contour of groups (Range 0-6: default 0)\n"
                /* NOTE : CARE all_types is 7 AND NOT 8 here */
                "multlo, multhi:     min & max multiplier of total input duration of motif\n"
                "atten:              overall attenuation of the output\n"
                "position:           centre of output sound-image (0(Left) 1(Right):default 0.5)\n"
                "spread:             spatial-spread of texture events (0 - 1(full-spread))\n"
                "seed:             same seed-no: same output on rerun(dflt 0: differs each time)\n"
                "-w:                 always play whole input-sound (ignoring duration values).\n"
                "-d                  notes within any one motif all have same duration.\n"
                "-i:                 motif not each confined to fixed instr (default:fixed)\n",MAX_SCAT_TEXTURE);
    } else if(!strcmp(str,"tmotifsin")) {
        fprintf(stdout,
                "TIMED TEXTURE OF MOTIFS, FORCED ONTO A HARMONIC FIELD: USAGE:\n"
                "texture tmotifsin mode infile [infile2...] outfile notedata sndfirst sndlast\n"
                "mingain maxgain minpich maxpich phgrid gpspace gpsprange\n"
                "amprise contour multlo multhi [-aatten] [-ppos] [-sspread] [-rseed] [-w] [-d]\n"
                "MODES:-\n"
                "1) ON HARMONIC-FIELD 2) CHANGING HFLDS 3) HARMONIC-SET 4) CHANGING HSETS\n"
                "notedata:    infofile, 'pitch' of insnds,motifs,texture timings,hfields etc..\n"
                "outdur:           (min) duration of outfile\n"
                "skiptime:         time between repetitions of timing motif in notedata\n"
                "sndfirst,sndlast: 1st,last snd to use, from input snds (range 1 - all)\n"
                "mingain,maxgain:  minimum & maximum level of input events (1-127:default 64,64)\n"
                "minpich,maxpich:  min & max pitch (MIDI VALUE)\n"
                "phgrid:           a timegrid (MS) applying WITHIN the motifs\n"
                "gpspace:          spatialisation of event-groups (Range 0-5: default 1)\n"
                "gpsprange:        spatial range of event-groups (Range 0-1: default 1)\n"
                "amprise:          amplitude change within motifs: (0-127: default 0)\n"
                "contour:          amplitude contour of groups (Range 0-6: default 0)\n"
                /* NOTE : CARE all_types is 7 AND NOT 8 here */
                "multlo, multhi:   min & max multiplier of total input duration of motif\n"
                "atten:            overall attenuation of the output\n"
                "pos:              centre of output sound-image (0(Left) 1(Right):default 0.5)\n"
                "spread:           spatial-spread of texture events (0 - 1(full-spread))\n"
                "seed:             same seed-no: same output on rerun(dflt 0: differs each time)\n"
                "-w:               always play whole input-sound (ignoring duration values).\n"
                "-d:               notes in any one motif all have same duration as timing note.\n"
                "-i:               motifs not each confined to a fixed instrument (default: fixed)\n");
    } else if(!strcmp(str,"motifs")) {
        fprintf(stdout,
                "TEXTURE OF MOTIFS, (1ST NOTES ONLY, FORCED ONTO HARMONIC FIELD/SET, IF USED)\n"
                "USAGE: texture motifs mode infile [infile2...] outfile notedata outdur packing\n"
                "scatter tgrid sndfirst sndlast mingain maxgain minpich maxpich\n"
                "phgrid gpspace gpsprange amprise contour multlo multhi\n"
                "                 [-aatten] [-pposition] [-sspread] [-rseed] [-w] [-d]\n"
                "MODES:-\n"
                "1)ON HARMONIC-FIELD 2)CHANGING HFLDS 3)HARMONIC-SET 4)CHANGING HSETS 5)NONE\n"
                "notedata: infofile, 'pitch' of insnds, timings for timed textures,hfields etc.\n"
                "outdur:             (min) duration of outfile\n"
                "packing:            (average) time between motif onsets.\n"
                "scatter:            randomisation of event-onsets (0 - %.0lf)\n"
                "tgrid:              minstep(MS) quantised timegrid (for mtf starttimes)(dflt 0)\n"
                "sndfirst,sndlast:   1st,last snd to use, from input snds (range 1 - all)\n"
                "mingain,maxgain:    min & max level of input events (1-127:default 64,64)\n"
                "minpich,maxpich:    min & max pitch (MIDI VALUE)\n"
                "phgrid:             a timegrid (MS) applying WITHIN the motifs\n"
                "gpspace:            spatialisation of event-groups (Range 0-5: default 1)\n"
                "gpsprange:          spatial range of event-groups (Range 0-1: default 1)\n"
                "amprise:            amplitude change within motifs: (0-127: default 0)\n"
                "contour:            amplitude contour of groups (Range 0-6: default 0)\n"
                /* NOTE : CARE all_types is 7 AND NOT 8 here */
                "multlo, multhi:     min & max multiplier of total input duration of motif\n"
                "atten:              overall attenuation of the output\n"
                "position:           centre of output sound-image (0(Left) 1(Right):default 0.5)\n"
                "spread:             spatial-spread of texture events (0 - 1(full-spread))\n"
                "seed:               same seed-no: same output on rerun(dflt 0: differs each time)\n"
                "-w:                 always play whole input-sound (ignoring duration values).\n"
                "-d                  notes of any one motif all have same duration.\n"
                "-i:                 motif not each confined to fixed instr (default:fixed)\n",MAX_SCAT_TEXTURE);
    } else if(!strcmp(str,"tmotifs")) {
        fprintf(stdout,
                "TIMED TEXTURE OF MOTIFS, THEMSELVES NOT FORCED ONTO HARMONIC FIELD: USAGE:\n"
                "texture tmotifs mode infile [infile2...] outfile notedata outdur skip\n"
                "sndfirst sndlast mingain maxgain minpitch maxpitch phgrid\n"
                "gpspace gpsprange amprise contour multlo multhi\n"
                "                    [-aatten] [-pposition] [-sspread] [-rseed] [-w] [-d]\n"
                "MODES:-\n"
                "1)ON HARMONIC-FIELD 2)CHANGING HFLDS 3)HARMONIC-SET 4)CHANGING HSETS 5)NONE\n"
                "notedata:  infofile, 'pitch' of insnds,texture timings,motifs,hfields etc....\n"
                "outdur:             (min) duration of outfile\n"
                "skip:               time between repetitions of timing motif in notedata\n"
                "sndfirst,sndlast:   1st,last snd to use, from input snds (range 1 - all)\n"
                "mingain,maxgain:    min & max level of input events (1-127:default 64,64)\n"
                "minpitch,maxpitch:  min & max pitch (MIDI VALUE)\n"
                "phgrid:             a timegrid (MS) applying WITHIN the motifs\n"
                "gpspace:            spatialisation of event-groups (Range 0-5: default 1)\n"
                "gpsprange:          spatial range of event-groups (Range 0-1: default 1)\n"
                "amprise:            amplitude change within motifs: (0-127: default 0)\n"
                "contour:            amplitude contour of groups (Range 0-6: default 0)\n"
                /* NOTE : CARE all_types is 7 AND NOT 8 here */
                "multlo, multhi:     min & max multiplier of total input duration of motif\n"
                "atten:              overall attenuation of the output\n"
                "position:           centre of output sound-image (0(Left) 1(Right):default 0.5)\n"
                "spread:             spatial-spread of texture events (0 - 1(full-spread))\n"
                "seed:             same seed-no: same output on rerun(dflt 0: differs each time)\n"
                "-w:                 always play whole input-sound (ignoring duration values).\n"
                "-d:                 motif notes all have same duration as ornamented note\n"
                "-i:                 motif not each confined to fixed instr (default:fixed)\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
    sprintf(errstr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
    return(FINISHED);
}
