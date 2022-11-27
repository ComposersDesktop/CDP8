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



/* floatsam version : no changes */
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <texture.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

/**** ALL FUNCTIONS WHICH ARE GLOBAL TO ALL PROCESS FUNCTIONS TYPES ***/

/* Functions which are global to all texture applications.
 */
static void orn_subzero(double *thistime,noteptr phrlastnote,noteptr *phrasenote,
                        double *timeadjust,double multiplier,int *gpsize);
static int  getmtfampstep
(double amp,double framp,double *ampstep,unsigned char *amptype,
 unsigned char amptypestor,unsigned char amptypecnt,int gpsize,dataptr dz);
static int  space_gpnote(noteptr thisnote,int gpsize,dataptr dz);
static int  sp_scatter(noteptr mnote,dataptr dz);
static int  pre_place(noteptr thisnote,int gpsize,dataptr dz);
static int  post_place(noteptr thisnote,int gpsize,dataptr dz);
static int  read_direction(int shi,noteptr *shadow,int shadowsize);

static int  do_ev_ins(unsigned char *thisinstr,double thistime,dataptr dz);
static int  do_ev_amp(double *thisamp,double thistime,dataptr dz);
static int  do_ev_dur(double *thisdur,double thistime,dataptr dz);
static int  getdirampstep
(double amp,unsigned char amptypecnt,double *ampstep,int gpsize,dataptr dz);
static int  getampstep
(double *ampstep,double amp,unsigned char *amptype,int gpsize,unsigned char amptypecnt,
 unsigned char amptypestor,dataptr dz);
static int  get_dec_or_orn_pos(dataptr dz);

static int  subzero(double *thistime,double gpdense,int *gpsize);

static noteptr reverse_search_for_time(noteptr start,double reftime);
static void    move_notelist_item(noteptr a,noteptr b);
static double  setup_first_notelist_time(noteptr *base);
static int     reset_mtfhead(motifptr mtf);
static int         post_place(noteptr thisnote,int gpsize,dataptr dz);
static int     getmtfdirampstep
(double amp,double framp,unsigned char amptypecnt,double *ampstep,int gpsize,dataptr dz);
static void cyclicperm(dataptr dz);
static void hinsert(int m,int t,int *perm,int permlen);
static void hprefix(int m,int *perm,int permlen);
static void hshuflup(int k,int *perm,int permlen);

/***************************** OCTADJUST *****************************
 *
 * Check for range overflow of pitch.
 */

double octadjust(double thispitch)
{
    if(thispitch>MIDITOP)
        thispitch -= SEMITONES_PER_OCTAVE;
    if(thispitch<MIDIBOT)
        thispitch += SEMITONES_PER_OCTAVE;
    return(thispitch);
}

/***************************** DO_AMP_INSTR_DUR ************************************/

int do_amp_instr_dur
(double *thisamp,unsigned char *thisinstr,double *thisdur,noteptr tsetnote,double thistime,dataptr dz)
{
    int exit_status;
    if((exit_status = do_ev_amp(thisamp,thistime,dz))<0)
        return(exit_status);
    tsetnote->amp = (float)(*thisamp);
    if((exit_status = do_ev_ins(thisinstr,thistime,dz))<0)
        return(exit_status);
    tsetnote->instr = *thisinstr;
    if((exit_status = do_ev_dur(thisdur,thistime,dz))<0)
        return(exit_status);
    tsetnote->dur = (float)(*thisdur);
    return(FINISHED);
}

/********************** DO_EV_INS *******************************
 *
 * Set instrument of event (not group etc.).
 *
 * (0)  If instruments in ornament or decoration are to be relative to
 *      input instr, return this input instrument.
 * (1)  If there is a pre-existing timed set (pretimed = 1).
 * (2)  Negative value causes original value to be replaced
 *      by orig + abs(neg value).
 *      Zero value, by default, causes orig value to be retained.
 */

int do_ev_ins(unsigned char *thisinstr,double thistime,dataptr dz)
{
    int exit_status;
    int instrval;
    if(dz->process == SIMPLE_TEX && dz->vflag[CYCLIC_TEXFLAG]) {
        if(dz->itemcnt == dz->infilecnt)
            dz->itemcnt = 0;
        if(dz->vflag[PERM_TEXFLAG]) {
            if(dz->itemcnt == 0)
                cyclicperm(dz);
            *thisinstr = (unsigned char)dz->peakno[dz->itemcnt];
        } else {
            *thisinstr = (unsigned char)dz->itemcnt;
        }
        dz->itemcnt++;
    } else {
        if((exit_status = igetvalue(TEXTURE_INSHI,TEXTURE_INSLO,thistime,PM_INSNO,&instrval,dz))<0)
            return(exit_status);
        *thisinstr = (unsigned char)instrval;
    }
    return(FINISHED);
}

/********************** DO_EV_AMP ********************************/

int do_ev_amp(double *thisamp,double thistime,dataptr dz)
{
    int exit_status;
    if((exit_status = getvalue(TEXTURE_MAXAMP,TEXTURE_MINAMP,thistime,PM_AMP,thisamp,dz))<0)
        return(exit_status);

    *thisamp *= dz->param[TEXTURE_ATTEN];

    if(*thisamp>MIDITOP + 0.5) {
        sprintf(errstr,"TEXTURE: amplitude [%.1f] out of midirange at time %.2f\n",*thisamp,thistime);
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/********************** DO_EV_DUR ********************************/

int do_ev_dur(double *thisdur,double thistime,dataptr dz)
{
    int exit_status;
    if((exit_status = getvalue(TEXTURE_MAXDUR,TEXTURE_MINDUR,thistime,PM_DUR,thisdur,dz))<0)
        return(exit_status);
    return(FINISHED);                                       /* 4 */
}

/************************ MAKE_SHADOW ****************************
 *
 * Store addresses of original tset notes.
 */

int make_shadow(motifptr tset,int *shadowsize,noteptr **shadow)
{
    noteptr *sh, thisnote = tset->firstnote;
    int n;
    *shadowsize     = 0;
    while(thisnote!=(noteptr)0) {
        (*shadowsize)++;
        thisnote = thisnote->next;
    }
    if((sh = *shadow = (noteptr *)malloc((*shadowsize) * sizeof(noteptr)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for shadow.\n");
        return(MEMORY_ERROR);
    }
    thisnote = tset->firstnote;
    n = 0;
    while(thisnote!=(noteptr)0) {
        sh[n++]  = thisnote;
        thisnote = thisnote->next;
    }
    return(FINISHED);
}

/************************ ERASE_SHADOW ****************************
 *
 * Erase original tset from resultant list.
 *
 * (0)  Assuming shadow starts at start of tset, if first note of
 *      shadow is deleted, del_note automatically resets motifptr (tset)
 *      to next note (and so on).
 * (1)  In case shadow started AFTER start of tset (should be impossible!!)
 *      reset pointer to start of remaining motif !
 */

int erase_shadow(int shadowsize,noteptr *shadow,motifptr tset)
{
    int n;
    noteptr thisnote;
    for(n=0;n<shadowsize;n++)                                       /* 0 */
        del_note(shadow[n],tset);
    if((thisnote = tset->firstnote->last)!=(noteptr)0) {    /* 1 */
        sprintf(errstr,"TEXTURE: erase_shadow(): First note of shadow NOT first note of tset.\n"
                "This should never happen\n");
        return(PROGRAM_ERROR);
        /**** REMOVED FOLLOWING AUTO-CORRECTION CODE : OCT 1997 *****
              while(thisnote->last!=(noteptr)0)
              thisnote = thisnote->last;
              tset->firstnote = thisnote;
        ****/
    }
    return(FINISHED);
}

/************************ SETUP_DECOR ****************************/

void setup_decor(double *pptop,double *ppbot,int *shadindex,noteptr *tsetnote,dataptr dz)
{
    *pptop = MIDITOP;
    *ppbot = MIDIBOT;
    if(dz->vflag[WHICH_CHORDNOTE]==DECOR_HIGHEST)
        *tsetnote = gethipitch(*tsetnote,shadindex);
}

/**************************** SETUP_ORNAMENT *********************************/

int setup_ornament
(double *timeadjust,double *thistime,int *gpsize,noteptr *phrlastnote,
 double multiplier,noteptr *phrasenote,int phrno,dataptr dz)
{
    int exit_status;
    double origtime;
    if((exit_status = get_dec_or_orn_pos(dz))<0)
        return(exit_status);
    *timeadjust = 0.0;
    if(dz->vflag[IS_PRE]) {
        origtime = *thistime;
        if((*thistime -= (phrlastnote[phrno])->ntime * multiplier)<0.0) {
            *thistime = origtime;
            orn_subzero(thistime,phrlastnote[phrno],phrasenote,timeadjust,multiplier,gpsize);
        }
    }
    return(FINISHED);
}

/**************************** SET_MOTIF_AMP *********************************/

int set_motif_amp
(noteptr tsetnote,double *thisamp,int gpsize,double ampstep,noteptr phrasenote,double rangemult,
 double *phraseamp,int phrno,unsigned char amptype)
{
    double ampdif;
    if(amptype==CRESC) {
        *thisamp -= (double)(gpsize) * ampstep;
        ampdif    = (phraseamp[phrno] - phrasenote->amp) * rangemult;
        tsetnote->amp = (float)(*thisamp - ampdif);
    }
    return(FINISHED);
}

/**************************** SET_ORNAMENT_AMP *********************************/

int set_ornament_amp
(double *phraseamp,noteptr *phrlastnote,double *thisamp,
 noteptr phrasenote,int phrno,noteptr tsetnote,
 double ampstep,double rangemult,int gpsize,dataptr dz)
{
    double ampdif;
    if(dz->vflag[IS_PRE])
        ampdif=(phraseamp[phrno]-phrlastnote[phrno]->amp) * rangemult;
    else
        ampdif=(phraseamp[phrno]-phrasenote->amp) * rangemult;
    tsetnote->amp -= (float)ampdif;
    if(ampstep>0.0) /* CRESCENDO */
        *thisamp -= (double)(gpsize+1) * ampstep;
    return(FINISHED);
}

/**************************** CHECK_NEXT_PHRASENOTE_EXISTS *********************************/

int check_next_phrasenote_exists(noteptr *phrasenote,int texflag,dataptr dz)
{
    if(texflag & IS_MOTIFS) {
        if((*phrasenote = (*phrasenote)->next)==(noteptr)0) {
            sprintf(errstr,"TEXTURE: Accounting Problem in check_next_phrasenote_exists()\n");
            return(PROGRAM_ERROR);
        }
    } else if (texflag & IS_ORNATE){
        if(*phrasenote==(noteptr)0) {
            sprintf(errstr,"TEXTURE: 1st Accounting Problem in check_next_phrasenote_exists()\n");
            return(PROGRAM_ERROR);
        }
        if(dz->vflag[IS_PRE] && ((*phrasenote)->next==(noteptr)0)) {
            sprintf(errstr,"TEXTURE: 2nd Accounting Problem in check_next_phrasenote_exists()\n");
            return(PROGRAM_ERROR);
        }
    }
    return(CONTINUE);
}

/************************* GETNEXTEVENT_TO_DECORATE ***************************
 *
 * Get the next event in list, i.e. skip over simultaneous events
 * (e.g. chords).
 *
 * (0)  If flagged as 2, ornaments/decorations are attached to EVERY note
 *      so the next event is just the next note in the tset.
 *      OTHERWISE we search for the next event AT A NEW TIME.
 * (1)  If flagged, force a search for the highest note of a simultaneous
 *      group (chord). Hence ornament/dec attaches to highest note.
 *      Otherwise, ornament attached to first listed note of chord in input
 *      data.
 */

noteptr getnextevent_to_decorate(noteptr tsetnote,int *shaddoindex,dataptr dz)
{
    if(dz->vflag[WHICH_CHORDNOTE]==DECOR_EVERY)                                             /* 0 */
        return(tsetnote->next);
    while(tsetnote->next!=(noteptr)0) {
        tsetnote = tsetnote->next;
        (*shaddoindex)++;
        if(!flteq((double)tsetnote->ntime,(double)tsetnote->last->ntime)) {
            if(dz->vflag[WHICH_CHORDNOTE]==DECOR_HIGHEST)
                tsetnote = gethipitch(tsetnote,shaddoindex);    /* 1 */
            return(tsetnote);
        }
    }
    return((noteptr)0);
}

/************************** GETNOTETIME ********************************
 *
 * Get actual time of note, from time of tset note, and time of motif
 * or ornament note.
 * (a)  Time is set by an offset from the tset-note, THROUGHOUT each
 *      motif-placement. The offset for each note is its position in the
 *      original note (which have been set relative to zero by set_mtfs())
 *      times the multiplier, with an adjustment if we're actually using
 *      less than the whole motif for some reason.
 * (b)  Time may need to be quantised.
 * (c)  Time of note is this tset-note + offset.
 */

double getnotetime(noteptr phrasenote,double thistime,double multiplier,double timeadjust,dataptr dz)
{
    int qunits;
    double timestep = ((double)phrasenote->ntime - timeadjust) * (double)multiplier; /* a */
    double thisgrid;

    if(dz->param[TEX_PHGRID]>0.0) {
        /* NEW  2000 */
        thisgrid = dz->param[TEX_PHGRID] * MS_TO_SECS;

        qunits    = round(timestep/thisgrid);           /* b */
        timestep  = (double)(qunits*thisgrid);
    }
    return(thistime + timestep);                            /* c */
}

/************************ GETMTFDUR **********************************/

int getmtfdur(noteptr tsetnote,noteptr phrasenote,double *dur,double multiplier,dataptr dz)
{
    if(dz->vflag[DONT_KEEP_MTFDUR])
        *dur = tsetnote->dur;
    else
        *dur = phrasenote->dur * multiplier;
    if(flteq(*dur,0.0)) {
        sprintf(errstr,"TEXTURE: Problem 3 in getmtfdur(): Zero duration\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/***************************** ORN_OR_MTF_AMP_SETUP ************************************/

int orn_or_mtf_amp_setup
(int ampdirected,double *phrange,int phrno,double thisamp,int gpsize,double *rangemult,double *ampstep,
 unsigned char *amptype,unsigned char amptypestor,unsigned char amptypecnt,dataptr dz)
{
    int exit_status;
    double kk;
    if(phrange[phrno]>=thisamp) {
        kk = (double)phrange[phrno]*((double)(gpsize+1)/(double)gpsize);
        *rangemult = (double)thisamp / kk;
        *ampstep   = 0.0;
    } else {
        if(ampdirected) {
            if((exit_status = getmtfdirampstep(thisamp,phrange[phrno],amptypecnt,ampstep,gpsize,dz))<0)
                return(exit_status);
        } else {
            if((exit_status = getmtfampstep(thisamp,phrange[phrno],ampstep,amptype,amptypestor,amptypecnt,gpsize,dz))<0)
                return(exit_status);
        }
        *rangemult = 1.0;
    }
    return(FINISHED);
}

/**************************** DO_MTF_PARAMS *******************************/

int do_mtf_params
(noteptr thisnote,double thisamp,noteptr phrasenote,noteptr tsetnote,
 double ampdif,double notetime,double multiplier,dataptr dz)
{
    int exit_status;
    double duration;
    thisnote->amp   = (float)(thisamp - ampdif);
    if((exit_status = getmtfdur(tsetnote,phrasenote,&duration,multiplier,dz))<0)
        return(exit_status);
    thisnote->dur = (float)duration;
    if((exit_status = do_grp_ins(tsetnote->instr,&(thisnote->instr),dz))<0)
        return(exit_status);
    thisnote->motioncentre = tsetnote->motioncentre;
    thisnote->ntime  = (float)notetime;
    return(FINISHED);
}

/***************************** SETUP_MOTIF_OR_ORNAMENT ************************************/

int setup_motif_or_ornament
(double thistime,double *multiplier,int *phrno,noteptr *phrasenote,motifptr *phrase,dataptr dz)
{
    int exit_status;
    if((exit_status = getvalue(TEX_MULTHI,TEX_MULTLO,thistime,PM_MULT,multiplier,dz))<0)
        return(exit_status);
    if((exit_status = doperm((int)dz->tex->phrasecnt,PM_WHICH,phrno,dz))<0)
        return(exit_status);
    *phrasenote    = phrase[*phrno]->firstnote;
    return(FINISHED);
}

/****************************** POSITION_AND_SIZE_DECORATION **********************************/

int position_and_size_decoration
(double *thistime,double tsettime,double gpdense,int *gpsize,dataptr dz)
{
    int exit_status;
    if((exit_status = get_dec_or_orn_pos(dz))<0)
        return(exit_status);
    if(dz->vflag[IS_PRE]) {
        if((*thistime -= gpdense * (double)((*gpsize) + 1))< -gpdense) {
            *thistime = tsettime;
            if((exit_status = subzero(thistime,gpdense,gpsize))<0)
                return(exit_status);
        }
    }
    return(FINISHED);
}

/****************************** SET_DECOR_AMP **********************************/

int set_decor_amp
(int ampdirected,double *thisamp,double *ampstep,int gpsize,
 unsigned char *amptype,unsigned char amptypecnt,unsigned char amptypestor,dataptr dz)
{
    int exit_status;
    if(ampdirected) {
        if((exit_status = getdirampstep(*thisamp,amptypecnt,ampstep,gpsize,dz))<0)
            return(exit_status);
    } else  {
        if((exit_status = getampstep(ampstep,*thisamp,amptype,gpsize,amptypecnt,amptypestor,dz))<0)
            return(exit_status);
    }
    if(*ampstep>0.0)
        *thisamp -= (double)(gpsize+1) * (*ampstep);
    return(FINISHED);
}

/****************************** SET_GROUP_AMP **********************************/

int set_group_amp
(noteptr tsetnote,double *thisamp,unsigned char *amptype, double *ampstep,int gpsize,
 unsigned char amptypecnt,unsigned char amptypestor,dataptr dz)
{
    int exit_status;
    if((exit_status = getampstep(ampstep,*thisamp,amptype,gpsize,amptypecnt,amptypestor,dz))<0)
        return(exit_status);
    if(*amptype==CRESC) {
        *thisamp -= (double)(gpsize) * (*ampstep);
        tsetnote->amp = (float)(*thisamp);
    }
    return(FINISHED);
}

/* LOCAL TO THESE GLOBALS */

/****************************** ORN_SUBZERO *******************************
 *
 * Readjust the groupsize and the pointer to first note of phrase, where
 * otherwise phrase would start before zero.
 *
 * (1)  Note the endtime of the phrase.
 * (2)  For each note in the phrase.
 * (3)  Find the duration of the phrase FROM THIS NOTE, by subtracting time of
 *      current note from end (total) time, and adjusting by multiplier.
 * (4)  If the duration of phrase from here, would put it's start after
 *      zero-time, break from loop.
 * (5)  Count number of notes rejected, and go to next phrase note.
 * (6)  If we're at end of phrase, none of it's notes fall after zero time,
 *      so return a gpsize of 0.
 * (7)  Otherwise reset the event time (mifitime) to start of the group or
 *      part-group which 'thisnote' points to.
 * (8)  Resert the phrase pointer to point at this note of the phrase.
 * (9)  When note times are finally calculated, timeadjust will reset all
 *      the note times WITHIN the ornament to run from the note from
 *      which we're starting, rather than the true start, by subtracting
 *      'timeadjust'.
 * (10) Return a size for the group reduced by the number of rejected notes.
 */

void orn_subzero
(double *thistime,noteptr phrlastnote,noteptr *phrasenote,double *timeadjust,double multiplier,int *gpsize)
{
    double endtime = phrlastnote->ntime;            /* 1 */
    double phrdur = 0.0;
    int cnt = 0;
    noteptr thisnote = *phrasenote;
    while(thisnote!=(noteptr)0) {                           /* 2 */
        phrdur = (endtime - thisnote->ntime) * multiplier;      /* 3 */
        if(*thistime - phrdur >= 0.0)                   /* 4 */
            break;
        cnt++;                                                                  /* 5 */
        thisnote = thisnote->next;
    }
    if(thisnote==(noteptr)0) {
        *gpsize = 0;                                                    /* 6 */
        return;
    } else {
        *thistime -= phrdur;                                    /* 7 */
        *phrasenote = thisnote;                                 /* 8 */
        *timeadjust = thisnote->ntime;                  /* 9 */
    }
    *gpsize -= cnt;
    return;                                                                         /* 10 */
}

/*************************** GETMTFAMPSTEP ******************************
 *
 * Select flat, cresc or decresc, from available types, for motifs.
 *
 * (0)  If the amplitude is bigger than the size of amprise and the
 *      range of the phrase, taken together, amprise is OK as it is.
 * (00) Otherwise, amprise (set as ampstep at this point) is the amount
 *      of the total amplitude left when the amplitude range of the phrase
 *      has been subtracted. This ensures that no note of the phrase
 *      can possibly end up below zero.
 * (1)  IF the cresc will be from zero, clrzamp is 1, and this makes
 *      the amplitude steps a little smaller, so that 1 of the notes
 *      in sequence cannot be at amplitude zero.
 *      OTHERWISE, clrzamp = 0,  and ampstep is as normal.
 */

int getmtfampstep(double amp,double framp,double *ampstep,unsigned char *amptype,
                  unsigned char amptypestor,unsigned char amptypecnt,int gpsize,dataptr dz)
{
    int exit_status;
    int clrzamp = 0;
    int k;
    if((exit_status = doperm((int)amptypecnt,PM_GPCNTR,&k,dz))<0)
        return(exit_status);
    if((exit_status=gettritype(k,amptypestor,amptype))<0)
        return(exit_status);
    if(*amptype==FLAT) {            /**** FLAT *****/
        *ampstep =0.0;
    } else {
        if(amp>(dz->param[TEX_AMPRISE] + framp)) /* 0 */
            *ampstep = dz->param[TEX_AMPRISE];
        else {
            *ampstep = amp - framp;                 /* 00 */
            clrzamp = 1;
        }
        *ampstep /= (double)(gpsize+clrzamp);           /* 1 */
        if(*amptype==DECRESC)           /** DECRESCENDO **/
            *ampstep = -(*ampstep);
    }
    return(FINISHED);
}

/************************************************************************
 *
 * Calculating spatial orientation of motifs,groups,ornaments,decorations.
 */

/**************************** SETSPACE *******************************/

int setspace(noteptr tsetnote,noteptr thisnote,int gpsize,dataptr dz)
{
    int exit_status;
    if(dz->iparam[TEX_GPSPACE]!=IS_STILL) {
        if((exit_status = space_gpnote(thisnote,gpsize,dz))<0)
            return(exit_status);
    } else
        thisnote->spacepos = tsetnote->spacepos;
    return(FINISHED);
}

/***************************** SPACE_GPNOTE *******************************
 *
 * (1)  OUTSIDE THE GROUP- OR PHRASE-MAKING LOOP
 *      (a) Need to know gpsize, dectype(is_post?), gpsprange
 *          (which MUST be +ve!).
 *      (b) Need to run 'init_group_spatialisation()' to establish spinit, & hence
 *          range modifications etc.
 *
 *  Then this will work INSIDE the loop.
 *
 * (1)  NB !!!!! is_pre is defaulted to ZERO. This means that groups
 *      or motifs which CANNOT BE pre- or post- are defaulted to
 *      post (which means the groups start on the tset notes, rather
 *      leading up to them).
 */

int space_gpnote(noteptr thisnote,int gpsize,dataptr dz)
{
    int exit_status;
    if(dz->iparam[TEX_GPSPACE]==IS_SCATTER) {
        if((exit_status = sp_scatter(thisnote,dz))<0)
            return(exit_status);
    } else {
        if(dz->vflag[IS_PRE]) {                         /* 1 */
            if((exit_status = pre_place(thisnote,gpsize,dz))<0)
                return(exit_status);
        } else {
            if((exit_status = post_place(thisnote,gpsize,dz))<0)
                return(exit_status);
        }
    }
    dz->iparam[SPINIT] = 0;         /* disable initialisation after first call to group */
    return(FINISHED);
}

/**************************** SP_SCATTER *************************
 *
 * scatter group note around position of main event.
 */

int sp_scatter(noteptr mnote,dataptr dz)
{
    int exit_status;
    double sptop = 1.0, spbot= 0.0, hfthissprange,val;
    if(dz->param[THISSPRANGE]<=0.0) {
        mnote->spacepos = (float)dz->param[TPOSITION];
        return(FINISHED);
    }
    if(dz->iparam[SPINIT]) {
        hfthissprange = dz->param[THISSPRANGE]/(double)2.0;
        spbot = max((dz->param[TPOSITION] - hfthissprange),0.0);
        sptop = min((dz->param[TPOSITION] + hfthissprange),1.0);
        dz->param[THISSPRANGE] = sptop - spbot;
    }
    if((exit_status = pscatx(dz->param[THISSPRANGE],spbot,PM_GPSPAC,&val,dz))<0)
        return(exit_status);
    mnote->spacepos      = (float)val;
    return(FINISHED);
}

/************** MACROS FOR RANGE REORIENTATIONS AND TRUNCATIONS **************
 *
 *      r = group spatial range         p = position of event
 *      c = centre of motion            d = direction of motion
 *
 * (1)  If position to left  of centre, reverse range.
 * (2)  If position to right of centre, reverse range.
 * (3)  If no motion, zero the range.
 * (4)  If motion is right to left reverse range.
 * (5)  If motion is left to right reverse range.
 * (6)  avoid centre crossing
 * (7)  avoid right-edge crossing
 * (8)  avoid left-edge crossing
 * (9)  avoid edge crossing
 */

#define TURNOUTWARD(r,p,c) if((p)-(c)<0.0)  (r) = -(r);                                         /* 1 */
#define TURNINWARD(r,p,c)  if((p)-(c)>0.0)  (r) = -(r);                                         /* 2 */
#define CHECKSTIL(r,d)     if((d)==0) (r) = 0.0;                                                        /* 3 */
#define FOLLOW(r,d)                CHECKSTIL(r,d) else { if((d)<0) (r) = -(r); }        /* 4 */
#define CONTRARY(r,d)      CHECKSTIL(r,d) else { if((d)>0) (r) = -(r); }        /* 5 */
#define SET_PRE_INRANGE(r,p,c) if(p<c) r = min(p,r); else r = max(p-1.0,r);


#define FREECENTRE(r,p,c)  if((r)>fabs((p)-(c))) (r)=fabs((p)-(c));                     /* 6 */
#define FREEREDGE(r,p)     if((p)+(r)>1.0) (r) = 1.0 - (p);                                     /* 7 */
#define FREELEDGE(r,p)     if((p)+(r)<0.0) (r) = -(p);                                          /* 8 */
#define FREEEDGES(r,p)     if((r)>=0.0) { FREEREDGE((r),(p)) } else { FREELEDGE((r),(p)) }
/* 9 */

#define FREEREDGE_PRE(r,p) if((p)-(r)>1.0) (r) = (p) - 1.0;                                     /* 7 */
#define FREELEDGE_PRE(r,p) if((p)-(r)<0.0) (r) = (p);                                           /* 8 */
#define FREEEDGES_PRE(r,p) if((r)>=0.0) { FREELEDGE_PRE((r),(p)) } else { FREEREDGE_PRE((r),(p)) }
/* 9 */
/************************* PRE_PLACE *****************************
 *
 * place-in-space ornaments or decorations BEFORE notes, or motifs or groups
 * at texture points.
 *
 * is_pre = 1.
 */

int pre_place(noteptr thisnote,int gpsize,dataptr dz)
{
    if(dz->iparam[SPINIT])
        dz->iparam[SPCNT] = 0;

    switch(dz->iparam[TEX_GPSPACE]) {
    case(IS_STILL):
        dz->param[THISSPRANGE] = 0.0;
        break;
    case(IS_INWARD):
        if(dz->iparam[SPINIT]) {
            TURNINWARD(dz->param[THISSPRANGE],dz->param[TPOSITION],dz->param[CPOS])
                SET_PRE_INRANGE(dz->param[THISSPRANGE],dz->param[TPOSITION],dz->param[CPOS])

                }
        break;
    case(IS_OUTWARD):
        if(dz->iparam[SPINIT]) {
            FREECENTRE(dz->param[THISSPRANGE],dz->param[TPOSITION],dz->param[CPOS])
                TURNOUTWARD(dz->param[THISSPRANGE],dz->param[TPOSITION],dz->param[CPOS])
                }
        break;
    case(IS_FOLLOWING):
        if(dz->iparam[SPINIT]) {
            FOLLOW(dz->param[THISSPRANGE],dz->iparam[DIRECTION])
                FREEEDGES_PRE(dz->param[THISSPRANGE],dz->param[TPOSITION])
                }
        break;
    case(IS_CONTRARY):
        if(dz->iparam[SPINIT]) {
            CONTRARY(dz->param[THISSPRANGE],dz->iparam[DIRECTION])
                FREEEDGES_PRE(dz->param[THISSPRANGE],dz->param[TPOSITION])
                }
        break;
    default:
        sprintf(errstr,"TEXTURE: Invalid space-type %d in pre_place()\n",dz->iparam[TEX_GPSPACE]);
        return(PROGRAM_ERROR);
    }
    thisnote->spacepos = (float)(dz->param[TPOSITION] - dz->param[THISSPRANGE]
                                 + ((dz->param[THISSPRANGE]/(double)gpsize)*(double)(dz->iparam[SPCNT])));
    dz->iparam[SPCNT]++;
    return(FINISHED);
}

/************************* POST_PLACE *****************************
 *
 * place-in-space ornaments or decorations AFTER notes, or motifs or groups
 * at texture points.
 *
 * is_pre = 0, (or no ornament).
 */

int post_place(noteptr thisnote,int gpsize,dataptr dz)
{
    if(dz->iparam[SPINIT])
        dz->iparam[SPCNT] = 1;
    switch(dz->iparam[TEX_GPSPACE]) {
    case(IS_STILL):
        dz->param[THISSPRANGE] = 0.0;
        break;
    case(IS_INWARD):
        if(dz->iparam[SPINIT]) {
            FREECENTRE(dz->param[THISSPRANGE],dz->param[TPOSITION],dz->param[CPOS])
                TURNINWARD(dz->param[THISSPRANGE],dz->param[TPOSITION],dz->param[CPOS])
                }
        break;
    case(IS_OUTWARD):
        if(dz->iparam[SPINIT]) {
            TURNOUTWARD(dz->param[THISSPRANGE],dz->param[TPOSITION],dz->param[CPOS])
                FREEEDGES(dz->param[THISSPRANGE],dz->param[TPOSITION])
                }
        break;
    case(IS_FOLLOWING):
        if(dz->iparam[SPINIT]) {
            FOLLOW(dz->param[THISSPRANGE],dz->iparam[DIRECTION])
                FREEEDGES(dz->param[THISSPRANGE],dz->param[TPOSITION])
                }
        break;
    case(IS_CONTRARY):
        if(dz->iparam[SPINIT]) {
            CONTRARY(dz->param[THISSPRANGE],dz->iparam[DIRECTION])
                FREEEDGES(dz->param[THISSPRANGE],dz->param[TPOSITION])
                }
        break;
    default:
        sprintf(errstr,"TEXTURE: Invalid space-type %d in post_place()\n",dz->iparam[TEX_GPSPACE]);
        return(PROGRAM_ERROR);
    }
    thisnote->spacepos = (float)(dz->param[TPOSITION] +
                                 ((dz->param[THISSPRANGE]/(double)gpsize)*(double)(dz->iparam[SPCNT])));
    dz->iparam[SPCNT]++;

    return(FINISHED);
}

/************************** GPSP_INIT ***********************************
 *
 * Initialise the space-param for each group, before calling the spatialise
 * routines. 'spinit' causes initialisation of range etc elsewhere (in
 * pre_place() or post_place() ).
 */

int init_group_spatialisation(noteptr tsetnote,int shaddoindex,noteptr *shadow,int shadowsize,dataptr dz)
{
    int exit_status;
    dz->iparam[SPINIT] = 1;
    dz->param[TPOSITION] = tsetnote->spacepos;
    dz->param[CPOS]      = tsetnote->motioncentre;
    if(dz->brksize[TEX_GRPSPRANGE]) { /* TABLE VALUES MUST BE 0 - 1 */
        if((exit_status = read_value_from_brktable((double)tsetnote->ntime,TEX_GRPSPRANGE,dz))<0)
            return(exit_status);
    }
    if(IS_DIRECTED(dz->iparam[TEX_GPSPACE]))
        dz->iparam[DIRECTION] = read_direction(shaddoindex,shadow,shadowsize);
    dz->param[THISSPRANGE] = dz->param[TEX_GRPSPRANGE];
    return(FINISHED);
}

/**************************** READ_DIRECTION ****************************
 *
 * Read direction of motion from spatial position stored in note.
 *
 * NB: as the tset is in the process of being modified, we cannot
 * refer directly to the tset linked list. We can only access the original
 * tset through it's 'shadow'.
 *
 * (0)  Save the time of the current note as 'now'.
 * (1)  Save position of current note as 'here'.
 * (2)  Whilever we're not at the end of the tset (shadow).
 * (a)  Note time of next note, store as 'then'.
 * (b)  If 'then' is later than 'now'.
 *              Note position of new note as 'there'.
 *              Set as OK and break out of loop.
 * (3)  If it's not OK (i.e. we were at, or came to the end of the tset
 *              (shadow))....
 * (a)  Go back to where we started.
 * (b)  Call this position 'there'.
 * (4)  While ever we're not at beginning of tset...
 * (a)  Note time of previous note, store as 'then'.
 * (b)  If 'then' is 'earlier' than 'now'...
 *              Note position of new note as 'here'.
 *              Set as OK and break out of loop.
 * (5)  If still not OK, then there is no more than 1 (non-simultaneous)
 *              event in the tset, so therefore no motion, so return 0.
 * (6)  If there>here, left-to-right motion, return 1.
 * (7)  If here>there, right-to-left motion, return -1.
 * (8)  Otherwise, no motion, return 0.
 */

int read_direction(int shi,noteptr *shadow,int shadowsize)
{
    unsigned char OK;
    float here, there = 0.0;
    double now, then;
    int startshi = shi;
    now  = (shadow[shi])->ntime;                    /* 0 */
    here = (shadow[shi])->motioncentre;             /* 1 */
    OK = 0;
    while(++shi<shadowsize) {                       /* 2 */
        then = (shadow[shi])->ntime;            /* a */
        if(then>(now+FLTERR)) {                 /* b */
            there = (shadow[shi])->motioncentre;
            OK = 1;
            break;
        }
    }
    if(!OK) {                                       /* 3 */
        shi = startshi;                         /* a */
        there = here;                           /* b */
        while(--shi>=0) {                       /* 4 */
            then = (shadow[shi])->ntime;            /* a */
            if(then<(now-FLTERR)) {         /* b */
                here = (shadow[shi])->motioncentre;
                OK = 1;
                break;
            }
        }
    }
    if(!OK)
        return(0);                              /* 5 */
    if(there>here)                          /* 6 */
        return(1);
    if(there<here)                          /* 7 */
        return(-1);
    return(0);                                      /* 8 */
}

/*************************** GETAMPSTEP ******************************
 *
 * Select flat, cresc or decresc, from available types.
 *
 * (1)  IF the cresc will be from zero, clrzamp is 1, and this makes
 *      the amplitude steps a little smaller, so that 1st note in sequence
 *      will not be at amplitude zero.
 *      OTHERWISE, clrzamp = 0,  and ampstep is as normal.
 */

int getampstep
(double *ampstep,double amp,unsigned char *amptype,int gpsize,unsigned char amptypecnt,
 unsigned char amptypestor,dataptr dz)
{
    int exit_status;
    int clrzamp = 0, k;
    /*int texflag = dz->tex->txflag;*/
    if((exit_status = doperm((int)amptypecnt,PM_GPCNTR,&k,dz))<0)
        return(exit_status);
    if((exit_status = gettritype(k,amptypestor,amptype))<0)
        return(exit_status);

    if(*amptype==FLAT)              /**** FLAT *****/
        *ampstep =0.0;
    else {
        *ampstep = dz->param[TEX_AMPRISE]; /* param with <|> option */

        if(*ampstep>=amp) {
            clrzamp  = 1;
            *ampstep = amp;
        }
        *ampstep /= (double)(gpsize + clrzamp);         /* 1 */
        if(*amptype==DECRESC)           /** DECRESCENDO **/
            *ampstep = -(*ampstep);
    }
    return(FINISHED);
}

/*************************** GETDIRAMPSTEP ******************************
 *
 * (1)  If there are two amplitude types, in THIS CASE, these can only
 *              be DIRECTED or FLAT. So...
 *   (2)  Choose flat or directed via perm.
 *   (3)  and if it's flat, return an ampstep of zero.
 *        Otherwise, we proceed as in the normal case of a directed amp.
 * (4)  Set initial ampstep to amprise.
 * (5)  IF the cresc will be from zero, or decresc to zero,  clrzamp is 1,
 *              and this makes the amplitude steps a little smaller, so that 1st note
 *              of cresc (or last note of decresc) will not be at amplitude zero.
 *              OTHERWISE, clrzamp = 0,  and ampstep is as normal.
 * (6)  Divide ampstep by number of members in group.
 * (7)  If it's a post-ornament, ampstep is negative.
 */

int getdirampstep
(double amp,unsigned char amptypecnt,double *ampstep,int gpsize,dataptr dz)
{
    int exit_status;
    int k, clrzamp = 0;
    if(amptypecnt==2) {                                                                     /* 1 */
        if((exit_status= doperm((int)2,PM_GPCNTR,&k,dz))<0)     /* 2 */
            return(exit_status);
        if(k==0) {
            *ampstep = 0.0;
            return(FINISHED);                                                       /* 3 */
        }
    }
    *ampstep = dz->param[TEX_AMPRISE];      /* param with <|> option */                             /* 4 */
    if(*ampstep>=amp) {                                                                     /* 5 */
        clrzamp = 1;
        *ampstep = amp;
    }
    *ampstep /= (double)(gpsize + clrzamp);                         /* 6 */
    if(!dz->vflag[IS_PRE])                                                          /* 7 */
        *ampstep = -(*ampstep);

    return(FINISHED);
}

/*************************** GETMTFDIRAMPSTEP *******************************
 *
 *      Intelligent guesswork!! Nov 4: 1997
 */

int getmtfdirampstep(double amp,double framp,unsigned char amptypecnt,double *ampstep,int gpsize,dataptr dz)
{
    int exit_status;
    int k, clrzamp = 0;
    if(amptypecnt==2) {                                                                     /* 1 */
        if((exit_status= doperm((int)2,PM_GPCNTR,&k,dz))<0)     /* 2 */
            return(exit_status);
        if(k==0) {
            *ampstep = 0.0;
            return(FINISHED);                                                       /* 3 */
        }
    }
    if(amp>(dz->param[TEX_AMPRISE] + framp))
        *ampstep = dz->param[TEX_AMPRISE];
    else {
        *ampstep = amp - framp;
        clrzamp = 1;
    }
    *ampstep /= (double)(gpsize + clrzamp);                         /* 6 */
    if(!dz->vflag[IS_PRE])                                                          /* 7 */
        *ampstep = -(*ampstep);

    return(FINISHED);
}

/*************************  GET_DEC_OR_ORN_POS ***************************
 *
 * Decuptype flags are selected and shifted down into the range 0-3
 * If its 01 (1), it's a post-ornament, so set 'is_pre' = FALSE.
 * If its 10 (2), it's a pre_ornament,  so set 'is_pre' = TRUE
 * If its 00 (0), it's not set, so we select 'is_pre' at random using doperm on 2 (output 0S or 1).
 *
 * NB: If we are using a group or motif, rather than a decoration
 *      or ornament, is_pre is BY DEFAULT set to 0.
 */

int get_dec_or_orn_pos(dataptr dz)
{
    int exit_status;
    int k;
    unsigned int dectype = dz->tex->txflag & GET_DECORNPOS;
    switch(dectype) {
    case(1): dz->vflag[IS_PRE] = TRUE;      break;
    case(2): dz->vflag[IS_PRE] = FALSE;     break;
    default:
        if((exit_status = doperm((int)2,PM_ORNPOS,&k,dz))<0)
            return(exit_status);
        dz->vflag[IS_PRE] = (char)k;
        break;
    }
    if(dz->vflag[IS_PRE] > 1) {
        sprintf(errstr,"Error from perm in  get_dec_or_orn_pos()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/**************************** SUBZERO ***********************************
 *
 * If start of a decoration falls before time zero, adjust its size
 * and hence starttime(-minus-gpdense) appropriately.
 */

int subzero(double *thistime,double gpdense,int *gpsize)
{
    int n = -1;
    while(*thistime>=0.0) {
        *thistime -= gpdense;
        n++;
    }
    if(n<0) {
        sprintf(errstr,"TEXTURE: Problem in subzero()\n");
        return(PROGRAM_ERROR);
    }
    *gpsize = n;
    return(FINISHED);
}

/*************************** GETHIPITCH *********************************#
 *
 * Find highest pitch at current time.
 */

noteptr gethipitch(noteptr tsetnote,int *shaddoindex)
{
    noteptr hinote, thisnote;
    double hipitch;
    int zz = *shaddoindex;
    hinote = thisnote = tsetnote;
    hipitch = (double)hinote->pitch;
    while(thisnote->next!=(noteptr)0) {
        if(thisnote->next->ntime > thisnote->ntime + FLTERR)
            return(hinote);
        thisnote = thisnote->next;
        zz++;
        if((double)thisnote->pitch > hipitch) {
            hipitch = (double)thisnote->pitch;
            hinote  = thisnote;
            (*shaddoindex) = zz;
        }
    }
    return(hinote);
}

/****************************** DO_GRP_INS **********************************
 *
 * Choose instr-no within group/decoration/motif or ornament.
 * Values of instr-no range are preset when instr is chosen for the
 * tset, in do_ev_ins().
 */

int do_grp_ins(unsigned char thisinstr,unsigned char *val,dataptr dz)
{
    int exit_status;
    int irange, ival;
    if(dz->vflag[INS_TO_SCATTER]) {
        if((irange = (int)(dz->iparam[TEXTURE_INSHI] - dz->iparam[TEXTURE_INSLO])) < 0) {
            iswap((int *)&(dz->iparam[TEXTURE_INSHI]),(int *)&(dz->iparam[TEXTURE_INSLO]));
            irange = -irange;
        }
        if(irange==0)
            *val = (unsigned char)dz->iparam[TEXTURE_INSLO];
        else {
            if((exit_status = doperm(irange+1,PM_INSNO,&ival,dz))<0)        /* range INCLUSIVE for ints */
                return(exit_status);
            *val = (unsigned char)(ival     + dz->iparam[TEXTURE_INSLO]);
        }
    } else
        *val = thisinstr;
    return(FINISHED);
}

/***************************** PSCATX ************************************
 *
 * Select a random value within 'range', by permuting one of BANDCNT equal
 * ranges, and selecting a random value within the chosen range.
 */

int pscatx(double range,double bottom,int pindex,double *val,dataptr dz)
{
    int k, exit_status;
    double bandwidth, bandbottom, x;
    if((exit_status = doperm((int)BANDCNT,pindex,&k,dz))<0)
        return(exit_status);
    bandwidth       = range/(double)BANDCNT;
    bandbottom      = (double)k * bandwidth;
    x  = (drand48() * bandwidth);
    x += bandbottom;
    *val = x + bottom;
    return(FINISHED);
}

/*************************** DEL_NOTE ************************************/

void del_note(noteptr thisnote,motifptr thismotif)
{
    if(thisnote->next!=(noteptr)0)
        thisnote->next->last = thisnote->last;          /* 1 */
    if(thisnote->last!=(noteptr)0)
        thisnote->last->next = thisnote->next;          /* 2 */
    else
        thismotif->firstnote = thisnote->next;          /* 3 */
    free(thisnote);                                 /* 4 */
}

/************************** TIMESORT ****************************
 *
 * Timesorts notes in a list
 *
 * (1)  Move the item with earliest time to start of list, and set this
 *      (minimum) time as maximum time also.
 *      For each item in the list....
 *   (2)   If this item is earlier than current latest item (maxtime)..
 *     (3)    Find where it should be in the list.
 *     (4)    move it to that location.
 *   (5)   Otherwise, set this item's time as latest time(maxtime).
 * (6)  Reset motifhead to start of notelist!!
 */

int arrange_notes_in_timeorder(motifptr mtf)
{
    noteptr base, here, there, mark;
    double thistime, maxtime;
    if((base = mtf->firstnote)==(noteptr)0)
        return(FINISHED);
    maxtime = setup_first_notelist_time(&base);                     /* 1 */
    here = base;
    while(here!=(noteptr)0) {
        if((thistime = here->ntime) < maxtime) {                /* 2 */
            if((mark = here->last)==(noteptr)0) {
                sprintf(errstr,"Problem (1) in arrange_notes_in_timeorder()\n");
                return(PROGRAM_ERROR);
            }
            if((there = reverse_search_for_time(here,thistime))==(noteptr)0) {
                sprintf(errstr,"Timing problem in arrange_notes_in_timeorder()\n");
                return(PROGRAM_ERROR);
            }                                               /* 3 */
            move_notelist_item(here,there);                 /* 4 */
            here = mark;
        } else
            maxtime = here->ntime;                  /* 5 */
        here = here->next;
    }
    return reset_mtfhead(mtf);                                      /* 6 */
}

/************************* RESET_MTFHEAD ***************************
 *
 * Reset pointer to start of notelist.
 */

int reset_mtfhead(motifptr mtf)
{
    noteptr here = mtf->firstnote;
    if(mtf->firstnote==NULL) {
        sprintf(errstr,"Problem in reset_mrfhead()\n");
        return(PROGRAM_ERROR);
    }
    while(here->last!=(noteptr)0)
        here = here->last;
    mtf->firstnote = here;

    return(FINISHED);
}

/************************** SETUP_FIRST_NOTELIST_TIME ***************************
 *
 * Put the earliest timed item in a list of notes into start of list.
 */

double setup_first_notelist_time(noteptr *base)
{
    noteptr firstnote, here;
    double  mintime, thistime;
    firstnote = here = *base;
    mintime   = (*base)->ntime;
    while(here!=(noteptr)0) {
        if((thistime = here->ntime)<mintime) {
            firstnote = here;
            mintime = thistime;
        }
        here = here->next;
    }
    if(firstnote!=(*base)) {
        move_notelist_item(firstnote,(*base));
        *base = firstnote;
    }
    return(mintime);
}

/************************* MOVE_NOTELIST_ITEM ***************************
 *
 * Move an item in a notelist from location a to BEFORE location b.
 */

void move_notelist_item(noteptr a,noteptr b)
{
    if(a->last!=(noteptr)0)
        a->last->next = a->next;
    if(a->next!=(noteptr)0)
        a->next->last = a->last;
    a->next = b;
    a->last = b->last;
    if(b->last!=(noteptr)0)
        b->last->next = a;
    b->last = a;
}

/************************** REVERSE_SEARCH_FOR_TIME *****************************
 *
 * Reverse search note-list using a time as index, and return address
 * of note whose time is AFTER the input time.
 * WILL RETURN (noteptr)0 IF REFTIME IS OFF END OF LIST.
 */

noteptr reverse_search_for_time(noteptr start,double reftime)
{
    noteptr here = start;
    while(here->ntime >= reftime) {
        if(here->last == (noteptr)0)
            return(here);
        here = here->last;
    }
    return(here->next);
}

/************************** ISWAP *****************************/

void iswap(int *a,int *b)
{
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

/****************************** SET_GROUP_PARAMS **********************************/

int set_group_params
(noteptr tsetnote,noteptr thisnote,double gpdense,double ampstep,double *thisamp,double *thistime,double thisdur,dataptr dz)
{
    int exit_status;
    double thisgpdense, time_left;
    *thisamp += ampstep;
    thisnote->amp   = (float)*thisamp;
    thisnote->dur   = (float)thisdur;
    if(dz->vflag[FIXED_STEP])
        *thistime += gpdense;
    else {
        if((exit_status = get_density_val(*thistime,&thisgpdense,dz))<0)
            return(exit_status);
        if((dz->tex->txflag & IS_DECOR) && dz->vflag[IS_PRE]) {
            if((time_left = tsetnote->ntime - *thistime)<0.0) {
                sprintf(errstr,"Error in assumptions: set_group_params()\n");
                return(PROGRAM_ERROR);
            }
            time_left *= 0.66;
            thisgpdense = min(time_left,thisgpdense);
        }
        *thistime += thisgpdense;
    }
    return(FINISHED);
}

/***************************** GETTRITYPE *****************************
 *
 * Choose a type, from up to three available types in bit-flag.
 * e.g.
 * Choose an amplitude type (flat,cresc,decresc) from 3 available types.
 * Choose a decoration-orientation type (above,below,mid) from 3 types.
 *
 * (1)  For each type in the bitflag.
 * (2)  If this option is flagged, count it.
 * (3)  IF this option is chosen, return its bit-number.
 * (4)  Advance the bitflag mask.
 */

int gettritype(int k,unsigned stor,unsigned char *val)
{
    int v = 0, mask = 1, n;
    k++;
    for(n=0;n<3;n++) {                              /* 1 */
        if(mask & stor)                         /* 2 */
            v++;
        if(k==v) {                              /* 3 */
            *val = (unsigned char)n;
            return(FINISHED);
        }
        mask <<= 1;                             /* 4 */
    }
    sprintf(errstr,"TEXTURE: Problem in gettritype()\n");
    return(PROGRAM_ERROR);
}

/***************************  GET_DENSITY_VAL ******************************/

int get_density_val(double thistime,double *gpdense,dataptr dz)
{
    int exit_status;
    if((exit_status = getvalue(TEX_GPPACKHI,TEX_GPPACKLO,thistime,PM_GPDENS,gpdense,dz))<0)
        return(PROGRAM_ERROR);

    /* NEW MAR 2000 */
    *gpdense *= MS_TO_SECS;

    if(dz->param[TEX_PHGRID]>0.0)
        *gpdense = quantise(*gpdense,dz->param[TEX_PHGRID]);

    return FINISHED;
}

/**************************** SET_MOTIFS *******************************
 *
 * initialise all parameters of input motifs to be used for MOTIFS or
 * ORNAMENTS.
 *
 * (1)For each input motif (called a 'phrase').
 *    phrnotecnt[n] = 0;
 * (2)  Initialise count of number of notes to 0, maximum amplitude
 *      of phrase to 0.0, and lowest pitch to MIDITOP.
 * (3)  Go through each note of the motif.
 * (4)  Add up the number of notes in it.
 * (5)  Find it's lowest pitch.
 * (6)  Find it's loudest note (and set as phraseamp).
 * (7)  Establish dynamic range of phrase.
 * (8)  Transpose the motif into it's lowest possible register.
 * (9)  Ensure motif starts at zero time.
 * (10) Store addresses of last notes in phrases.
 */

int set_motifs
(int phrcount,motifptr *phrase,int *phrnotecnt,double *phraseamp,double *phrange,noteptr *phrlastnote)
{
    int exit_status;
    int n;
    double minpitch, mintime, minamp;
    noteptr thisnote, lastnote = (noteptr)0;
    for(n=0;n<phrcount;n++) {                                               /* 1 */
        if((exit_status = arrange_notes_in_timeorder(phrase[n]))<0)
            return(exit_status);
        phrnotecnt[n] = 0;
        phraseamp[n] = 0.0;                                                     /* 2 */
        minamp   = DBL_MAX;
        minpitch = MIDITOP;
        mintime  = DBL_MAX;
        thisnote = phrase[n]->firstnote;                        /* 3 */
        while(thisnote!=(noteptr)0) {
            phrnotecnt[n]++;                                                        /* 4 */
            if(thisnote->pitch<minpitch)
                minpitch = thisnote->pitch;                     /* 5 */
            if(thisnote->ntime<mintime)
                mintime  = thisnote->ntime;                     /* 5 */
            if((double)thisnote->amp>phraseamp[n])
                phraseamp[n] = (double)thisnote->amp;/* 6 */
            if(thisnote->amp<minamp)
                minamp = thisnote->amp;                         /* 6 */
            thisnote = thisnote->next;
        }
        if(minamp  == DBL_MAX || mintime == DBL_MAX) {
            sprintf(errstr,"Error parsing motif: set_motifs()\n");
            return(PROGRAM_ERROR);
        }
        thisnote = phrase[n]->firstnote;
        phrange[n] = phraseamp[n] - minamp;                     /* 7 */
        while(thisnote!=(noteptr)0) {
            thisnote->pitch = (float)(thisnote->pitch - (minpitch + MIDIBOT));      /* 8 */
            thisnote->ntime = (float)(thisnote->ntime - mintime);                           /* 9 */
            lastnote = thisnote;                                                                                            /* 10 */
            thisnote = thisnote->next;
        }
        if((phrlastnote[n] = lastnote) == (noteptr)0) {
            sprintf(errstr,"Zero length phrase encountered: set_motifs()\n");
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

void cyclicperm(dataptr dz)
{
    int n, t;
    do {
        for(n=0;n<dz->infilecnt;n++) {
            t = (int)floor(drand48() * (n+1));
            if(t==n) {
                hprefix(n,dz->peakno,dz->infilecnt);
            } else {
                hinsert(n,t,dz->peakno,dz->infilecnt);
            }
        }
    } while(dz->peakno[0] == dz->lastpeakno[dz->infilecnt - 1]);    // Avoid repetitions at perm boundaries.
    for(n=0;n<dz->infilecnt;n++)
        dz->lastpeakno[n] = dz->peakno[n];
}

void hinsert(int m,int t,int *perm,int permlen)
{
    hshuflup(t+1,perm,permlen);
    perm[t+1] = m;
}

void hprefix(int m,int *perm,int permlen)
{
    hshuflup(0,perm,permlen);
    perm[0] = m;
}

void hshuflup(int k,int *perm,int permlen)
{
    int n, *i;
    int z = permlen - 1;
    i = perm+z;
    for(n = z;n > k;n--) {
        *i = *(i-1);
        i--;
    }
}

#ifndef round

int round(double a)
{
    return (int)floor(a + 0.5);
}
#endif
