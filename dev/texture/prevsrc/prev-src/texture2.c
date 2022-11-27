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



/* floatsam version: no changes */
#include <stdio.h>
#include <stdlib.h>
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

/***************************** GENERATING TEXTURES WITH NO HFIELD(S) ******************************/

static int  setup_motif_params
(noteptr phrasenote,double tsettime,double *thisamp,double ampstep,double *phraseamp,
 int phrno,double rangemult,noteptr thisnote,noteptr tsetnote,
 double multiplier,double timeadjust,dataptr dz);
static void gen_orn_pitch(noteptr thisnote,noteptr tsetnote,noteptr phrasenote,
                          float phrlpitch,float phrfirstpitch,dataptr dz);
static void gen_mtf_pitch(noteptr thisnote,noteptr tsetnote,noteptr phrasenote,float phrfirstpitch);
static int  dec_centre(double tsetpitch,float *val,int gpsize,double gprlow,double gprange,dataptr dz);
static int  pscat(double range,double bottom,int pindex,double *val,dataptr dz);
static int  initialise_phrases
(int **phrnotecnt,double **phraseamp,double **phrange,noteptr **phrlastnote,dataptr dz);
static int  do_ev_pch
(double *thispitch,double thistime,double *pptop,double *ppbot,dataptr dz);
static int  get_gpparams
(double thistime,double thispitch,unsigned char dectypecnt,double *gprange,double *gprlow,int *gpsize,
 double *gpdense,double pptop,double ppbot,unsigned char dectypestor,int mingrpsize,dataptr dz);
static int orientrange
(double thispitch,double pptop, double ppbot,double *gprange, double *gprlow,
 unsigned char dectypestor,unsigned char dectypecnt,dataptr dz);
static void squeezrange(double thispitch,double *gprange,double *gprlow, double pptop, double ppbot);

/**************************** DO_TEXTURE *****************************/

int do_texture(dataptr dz)
{
    int exit_status;
    noteptr tsetnote = dz->tex->timeset->firstnote;
    unsigned int texflag     = dz->tex->txflag;
    unsigned char dectypestor = dz->tex->dectypstor;
    unsigned char dectypecnt  = dz->tex->dectypcnt;
    unsigned char amptypestor = dz->tex->amptypstor;
    unsigned char amptypecnt  = dz->tex->amptypcnt;
    int ampdirected           = dz->tex->ampdirectd;
    motifptr tset                     = dz->tex->timeset;
    motifptr *phrase                  = dz->tex->phrase;
    int phrcount                      = dz->tex->phrasecnt;
    int *phrnotecnt=NULL;
    double *phraseamp=NULL, *phrange=NULL;
    noteptr thisnote=NULL, nextnote=NULL, phrasenote=NULL, nextevent=NULL, *phrlastnote=NULL, *shadow=NULL;
    float phrlpitch=0.0, phrfirstpitch=0.0;
    unsigned char thisinstr, amptype=0;
    double  tsettime=0.0, thistime=0.0, thispitch=0.0, ampstep=0.0, thepitch=0.0, thisamp=0.0, thisdur=0.0;
    double  rangemult=0.0, gpdense=0.0, gprange=0.0, gprlow=0.0, multiplier=0.0;
    double timeadjust = 0.0; /* default */
    int phrno=0, n=0, shadowsize=0, gpsize=0, mingrpsize = 0;
    double pptop=0.0, ppbot=0.0;
    int shadindex = 0;

    dz->iparam[SPINIT] = 0;
    if(texflag & IS_CLUMPED) {
        if((dz->tex->phrasecnt > 0)
           && (exit_status = initialise_phrases(&phrnotecnt,&phraseamp,&phrange,&phrlastnote,dz))<0)
            return(exit_status);

        if(texflag & IS_GROUPS)
            mingrpsize = 1;

        if((exit_status= make_shadow(tset,&shadowsize,&shadow))<0)
            return(exit_status);

        if(texflag & IS_DECOR) {
            setup_decor(&pptop,&ppbot,&shadindex,&tsetnote,dz);
        } else if(texflag & IS_MOTIFS)  {
            if((exit_status = set_motifs(phrcount,phrase,phrnotecnt,phraseamp,phrange,phrlastnote))<0)
                return(exit_status);
        } else if(texflag & IS_ORNATE) {
            if((exit_status = set_motifs(phrcount,phrase,phrnotecnt,phraseamp,phrange,phrlastnote))<0)
                return(exit_status);
            if(dz->vflag[WHICH_CHORDNOTE]==DECOR_HIGHEST)
                tsetnote = gethipitch(tsetnote,&shadindex);
        }
    }

    while(tsetnote!=(noteptr)0) {
        tsettime  = (double)tsetnote->ntime;
        thistime  = tsettime;
        if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
            return(exit_status);
        if(texflag & IS_DECOR)
            thispitch = tsetnote->pitch;
        if(texflag & IS_ORN_OR_MTF) {
            if((exit_status =
                setup_motif_or_ornament(thistime,&multiplier,&phrno,&phrasenote,phrase,dz))<0)
                return(exit_status);
        }
        if(!(texflag & IS_ORN_OR_DEC)) {
            if((exit_status = do_ev_pch(&thispitch,thistime,&pptop,&ppbot,dz))<0)
                return(exit_status);
            tsetnote->pitch = (float)thispitch;
        }
        if((exit_status = do_amp_instr_dur(&thisamp,&thisinstr,&thisdur,tsetnote,thistime,dz))<0)
            return(exit_status);
        if(texflag & IS_CLUMPED) {
            if(texflag & IS_MOTIFS) {
                if((exit_status = getmtfdur(tsetnote,phrasenote,&thisdur,multiplier,dz))<0)
                    return(exit_status);
                tsetnote->dur = (float)thisdur;
            }
            if(texflag & IS_ORN_OR_MTF) {
                gpsize = phrnotecnt[phrno];
                phrfirstpitch = phrasenote->pitch;
            } else {
                if((exit_status = get_gpparams(thistime,thispitch,dectypecnt,
                                               &gprange,&gprlow,&gpsize,&gpdense,pptop,ppbot,dectypestor,mingrpsize,dz))<0)
                    return(exit_status);
            }
            if(texflag & IS_DECOR) {
                if((exit_status = position_and_size_decoration(&thistime,tsettime,gpdense,&gpsize,dz))<0)
                    return(exit_status);
            } else
                gpsize--;
            if(texflag & IS_ORNATE) {
                phrlpitch = (phrlastnote[phrno])->pitch;
                if((exit_status = setup_ornament
                    (&timeadjust,&thistime,&gpsize,phrlastnote,multiplier,&phrasenote,phrno,dz))<0)
                    return(exit_status);
            }
            if(!(texflag & IS_DECOR))
                nextnote = tsetnote->next;
            if(gpsize>0) {                                          /* 3b */
                if(texflag & IS_ORN_OR_MTF) {
                    if((exit_status = orn_or_mtf_amp_setup
                        (ampdirected,phrange,phrno,thisamp,gpsize,&rangemult,&ampstep,&amptype,amptypestor,amptypecnt,dz))<0)
                        return(exit_status);
                }
                if(texflag & IS_MOTIFS) {
                    if((exit_status = set_motif_amp
                        (tsetnote,&thisamp,gpsize,ampstep,phrasenote,rangemult,phraseamp,phrno,amptype))<0)
                        return(exit_status);
                } else if(texflag & IS_ORNATE) {
                    if((exit_status = set_ornament_amp
                        (phraseamp,phrlastnote,&thisamp,phrasenote,phrno,tsetnote,ampstep,rangemult,gpsize,dz))<0)
                        return(exit_status);
                } else if(texflag & IS_GROUPS) {
                    if((exit_status = set_group_amp
                        (tsetnote,&thisamp,&amptype,&ampstep,gpsize,amptypecnt,amptypestor,dz))<0)
                        return(exit_status);
                } else if(texflag & IS_DECOR) {
                    if((exit_status = set_decor_amp
                        (ampdirected,&thisamp,&ampstep,gpsize,&amptype,amptypecnt,amptypestor,dz))<0)
                        return(exit_status);
                }
                if(dz->iparam[TEX_GPSPACE]!=IS_STILL) {
                    if((exit_status = init_group_spatialisation(tsetnote,shadindex,shadow,shadowsize,dz))<0)
                        return(exit_status);
                }
                thisnote  = tsetnote;
                if(texflag & IS_DECOR)
                    nextnote = tsetnote->next;
                else if(texflag & IS_ORNATE) {
                    if(!dz->vflag[IS_PRE])
                        phrasenote = phrasenote->next;
                }
                if(texflag & IS_ORN_OR_DEC)
                    nextevent = getnextevent_to_decorate(tsetnote,&shadindex,dz);
                for(n=0;n<gpsize;n++) {
                    if((exit_status = make_new_note(&thisnote))<0)
                        return(exit_status);
                    if(texflag & IS_DEC_OR_GRP) {
                        if((exit_status = set_group_params
                            (tsetnote,thisnote,gpdense,ampstep,&thisamp,&thistime,thisdur,dz))<0)
                            return(exit_status);
                        if((exit_status = do_grp_ins(tsetnote->instr,&(thisnote->instr),dz))<0)
                            return(exit_status);
                        thisnote->motioncentre = tsetnote->motioncentre;
                        if(texflag & IS_DECOR) {
                            if((exit_status = dec_centre
                                (thispitch,&(thisnote->pitch),gpsize,gprlow,gprange,dz))<0)
                                return(exit_status);
                        } else {
                            if((exit_status = pscat(gprange,gprlow,PM_GPPICH,&thepitch,dz))<0)
                                return(exit_status);
                            thisnote->pitch = (float)thepitch;
                        }
                        thisnote->ntime  = (float)thistime;
                    } else {
                        if((exit_status = check_next_phrasenote_exists(&phrasenote,texflag,dz))<0)
                            return(exit_status);
                        if((exit_status = setup_motif_params(phrasenote,thistime,&thisamp,ampstep,phraseamp,phrno,
                                                             rangemult,thisnote,tsetnote,multiplier,timeadjust,dz))<0)
                            return(exit_status);
                        if(texflag & IS_MOTIFS) {
                            gen_mtf_pitch(thisnote,tsetnote,phrasenote,phrfirstpitch);
                        } else {
                            gen_orn_pitch(thisnote,tsetnote,phrasenote,phrlpitch,phrfirstpitch,dz);
                            phrasenote = phrasenote->next;
                        }
                    }
                    if((exit_status = setspace(tsetnote,thisnote,gpsize,dz))<0)
                        return(exit_status);
                }
                thisnote->next = nextnote;
                if(nextnote!=(noteptr)0)
                    nextnote->last = thisnote;
                if(texflag & IS_ORN_OR_DEC)
                    tsetnote = nextevent;

            } else if(texflag & IS_ORN_OR_DEC)
                tsetnote = getnextevent_to_decorate(tsetnote,&shadindex,dz);

        } else
            tsetnote = tsetnote->next;

        if(texflag & IS_MTF_OR_GRP) {
            tsetnote = nextnote;
            shadindex++;
        }
    }
    if((texflag & IS_DECOR) && dz->vflag[DISCARD_ORIGLINE]) {
        if((exit_status = erase_shadow(shadowsize,shadow,tset))<0)
            return(exit_status);
    }
    if(texflag & IS_CLUMPED)
        return arrange_notes_in_timeorder(tset);
    return(FINISHED);
}

/**************************** SETUP_MOTIF_PARAMS ****************************/

int setup_motif_params
(noteptr phrasenote,double tsettime,double *thisamp,double ampstep,double *phraseamp, int phrno,
 double rangemult,noteptr thisnote,noteptr tsetnote,double multiplier,double timeadjust,dataptr dz)
{
    double notetime, ampdif;
    notetime = getnotetime(phrasenote,tsettime,multiplier,timeadjust,dz);
    *thisamp  += ampstep;
    ampdif    =(phraseamp[phrno] - phrasenote->amp)*rangemult;/* e */
    return do_mtf_params(thisnote,*thisamp,phrasenote,tsetnote,ampdif,notetime,multiplier,dz);
}

/**************************** GEN_ORN_PITCH ****************************/

void gen_orn_pitch
(noteptr thisnote,noteptr tsetnote,noteptr phrasenote,float phrlpitch,float phrfirstpitch,dataptr dz)
{
    if(dz->vflag[IS_PRE])
        thisnote->pitch = (float)(tsetnote->pitch + phrasenote->pitch - phrlpitch);
    else
        thisnote->pitch = (float)(tsetnote->pitch + phrasenote->pitch - phrfirstpitch);
}

/**************************** GEN_MTF_PITCH ****************************/

void gen_mtf_pitch(noteptr thisnote,noteptr tsetnote,noteptr phrasenote,float phrfirstpitch)
{
    thisnote->pitch = (float)(tsetnote->pitch + phrasenote->pitch - phrfirstpitch);
    thisnote->pitch = (float)octadjust((double)thisnote->pitch);
}

/************************* DEC_CENTRE ******************************
 *
 * Centre a decoration on the decorated pitch.
 *
 * (1)  For groupsize 1 or 2, k=1
 *              For groupsize 3 or 4, k=2
 *              For groupsize 5 or 6, k=3 etc
 * (2)  Scatter pitches as normally.
 * (3)  If range is NOT one-sided, proceed..otherwise return in normal way.
 * (4)  Find ratio of upper half of range to lower.
 * (5)  Find distance from decorated note.
 * (6)  Save the current value of allowed repetitions.
 *              THese are number of times a note occurs above the decorated pitch,
 *              before it can do so again (same for below).
 * (7)  If the allowed repetitions are bigger than k, replace it by k.
 *              This means that if we allow 3 repetitions but there are only
 *              1,2,3 or 4 notes, we force, 1 repetition for 2 notes, 2 repetitions
 *              for 3 or 4 notes etc. but allow 3 repetitions for 5,6 etc notes.
 * (8)  Select above or below decorated pitch.
 * (9)  If meant to be above note, but is below it, put note in other
 *              half of range, with appropriate adjustment for relative sizes
 *              of half-ranges.
 * (10) If meant to be below note, but is above it, put note in other
 *              half of range, with appropriate adjustment for relative sizes
 *              of half-ranges.
 * (11) Restore the repetcnt value!!
 */

int dec_centre(double tsetpitch,float *val,int gpsize,double gprlow,double gprange,dataptr dz)
{
    int exit_status;
    double a, q, w, gprhi = gprlow + gprange, ulratio;
    int s, save, k = (gpsize+1)/2;                                                          /* 1 */
    if((exit_status = pscat(gprange,gprlow,PM_GPPICH,&a,dz))<0) /* 2 */
        return(exit_status);
    if((w=tsetpitch-gprlow)>FLTERR && !flteq(tsetpitch,gprhi)) {/* 3 */
        ulratio = (gprhi-tsetpitch)/w;                                                  /* 4 */
        q       = a-tsetpitch;                                                                  /* 5 */
        save    = dz->iparray[TXREPETCNT][PM_DECABV];                   /* 6 */
        if(dz->iparray[TXREPETCNT][PM_DECABV]>k)                                /* 7 */
            dz->iparray[TXREPETCNT][PM_DECABV] = k;
        if((exit_status = doperm((int)2,PM_DECABV,&s,dz))<0)                    /* 8 */
            return(exit_status);
        if(s && q<0.0)                                                                                  /* 9 */
            a = tsetpitch - (q*ulratio);
        if(!s && q>0.0)                                                                                 /* 10 */
            a = tsetpitch - (q/ulratio);
        dz->iparray[TXREPETCNT][PM_DECABV] = save;                              /* 11 */
        if(a<MIDIBOT || a>MIDITOP) {
            sprintf(errstr,"TEXTURE: Problem in dec_centre()\n");
            return(PROGRAM_ERROR);
        }
    }
    *val = (float)a;
    return(FINISHED);
}

/***************************** PSCAT ************************************
 *
 * A weighted version of pscatx().
 *
 * Select a random value within 'range', by permuting one of BANDCNT UNequal
 * ranges, and selecting a random value within the chosen range. The upper
 * and lower band are half as wide as the inner 3 bands, ensuring that
 * values within these outer bands are squeezed into a narrow-range-of-values
 * near the boundaries of the total range.
 */

int pscat(double range,double bottom,int pindex,double *val,dataptr dz)
{
    int exit_status;
    double bandbottom, bandwidth, x;
    int k;
    if((exit_status = doperm((int)BANDCNT,pindex,&k,dz))<0)
        return(exit_status);
    bandwidth = range/(double)LAYERCNT;
    switch(k) {
    case(0):
        bandbottom = 0.0;
        break;
    case(BANDCNT-1):
        bandbottom = (LAYERCNT-1) * bandwidth;
        break;
    default:
        bandbottom = bandwidth + ((double)((k-1) * 2) * bandwidth);
        bandwidth *= 2.0;
        break;
    }
    x  = (drand48() * bandwidth);
    x += bandbottom;
    *val = x + bottom;
    return(FINISHED);
}

/**************************** INITIALISE_PHRASES *******************************/

int initialise_phrases(int **phrnotecnt,double **phraseamp,double **phrange,noteptr **phrlastnote,dataptr dz)
{
    if((*phrnotecnt = (int *)malloc(dz->tex->phrasecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for phrase notecnt array.\n");
        return(MEMORY_ERROR);
    }
    if((*phraseamp = (double *)malloc(dz->tex->phrasecnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for phrase amplitude array.\n");
        return(MEMORY_ERROR);
    }
    if((*phrange = (double *)malloc(dz->tex->phrasecnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for phrase range array.\n");
        return(MEMORY_ERROR);
    }
    if((*phrlastnote = (noteptr *)malloc(dz->tex->phrasecnt * sizeof(noteptr)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for phrase lastnote array.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/********************** DO_EV_PCH *******************************
 *
 * Set pitch of event (not group etc.).
 */

int do_ev_pch
(double *thispitch,double thistime,double *pptop,double *ppbot,dataptr dz)
{
    int exit_status;
    if((exit_status = getvalue(TEXTURE_MAXPICH,TEXTURE_MINPICH,thistime,PM_PITCH,thispitch,dz))<0)
        return(exit_status);
    if(*thispitch > MIDITOP || *thispitch < MIDIBOT) {
        sprintf(errstr,"TEXTURE: pitch [%.1f] out of midirange at time %.2f\n",*thispitch,thistime);
        return(DATA_ERROR);
    }
    *pptop = dz->param[TEXTURE_MAXPICH];
    *ppbot = dz->param[TEXTURE_MINPICH];
    return(FINISHED);
}

/************************** GET_GPPARMAS **************************
 *
 * Get the parameters for a group of notes.
 *
 * (1)  Get gprange in normal way.
 * (2)  Don't adjust range.
 * (3)  For decorations, the range may be oriented, above, below
 *              or centred on the initial note. So range is adjusted here.
 */

int get_gpparams(double thistime,double thispitch,unsigned char dectypecnt,
                 double *gprange,double *gprlow,int *gpsize,double *gpdense,double pptop,double ppbot,
                 unsigned char dectypestor,int mingrpsize,dataptr dz)
{
    int exit_status;
    if((exit_status = getvalue(TEX_GPRANGHI,TEX_GPRANGLO,thistime,PM_GPRANG,gprange,dz))<0)
        return(exit_status);                                                                                            /* 1 */
    if(!dectypecnt)
        squeezrange(thispitch,gprange,gprlow,pptop,ppbot);
    else {
        if((exit_status = orientrange
            (thispitch,pptop,ppbot,gprange,gprlow,dectypestor,dectypecnt,dz))<0)/* 3 */
            return(exit_status);
    }
    if((exit_status = igetvalue(TEX_GPSIZEHI,TEX_GPSIZELO,thistime,PM_GPSIZE,gpsize,dz))<0)
        return(PROGRAM_ERROR);

    if(*gpsize < mingrpsize) {
        sprintf(errstr,"Impossible GROUP SIZE value [%d]\n",*gpsize);
        return(PROGRAM_ERROR);
    }
    return get_density_val(thistime,gpdense,dz);
}

/***************************  ORIENTRANGE ******************************
 *
 */

int orientrange
(double thispitch,double pptop, double ppbot,double *gprange, double *gprlow,
 unsigned char dectypestor,unsigned char dectypecnt,dataptr dz)
{
    int exit_status;
    unsigned char dectype;
    double top;
    int k;
    if((exit_status = doperm((int)dectypecnt,PM_ORIENT,&k,dz))<0)
        return(exit_status);
    if((exit_status = gettritype(k,dectypestor,&dectype))<0)
        return(exit_status);
    switch(dectype) {
    case(0):                        /* centred range */
        squeezrange(thispitch,gprange,gprlow,pptop,ppbot);
        dz->vflag[DECCENTRE] = TRUE;
        break;
    case(1):                        /* range above note */
        top      = min((thispitch + *gprange),pptop);
        *gprlow  = max((top - *gprange),ppbot);
        *gprange = top - *gprlow;
        dz->vflag[DECCENTRE] = FALSE;
        break;
    case(2):                        /* range below note */
        top      = thispitch;
        *gprlow  = max((top - *gprange),ppbot);
        *gprange = top - *gprlow;
        dz->vflag[DECCENTRE] = FALSE;
        break;
    default:
        sprintf(errstr,"TEXTURE: Problem in orientrange()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/***************************  SQUEEZRANGE ******************************
 *
 * Adjust range to lie within texture limits.
 *
 * (2)  Top of range is at current pitch plus half range. If this is
 *      above current top-of-texture range (pptop) move top down.
 * (3)  Bottom of range is gprange below top. If this is below
 *      bottom-of-texture range (ppbot), move gprangelo up.
 * (4)  Recalculate the true gprange within true limits.
 */

void squeezrange(double thispitch,double *gprange,double *gprlow, double pptop, double ppbot)
{
    double top;
    top     = min((thispitch + (*gprange/2.0)),pptop);              /* 2 */
    *gprlow  = max((top-*gprange),ppbot);                                   /* 3 */
    *gprange = top - *gprlow;                                                               /* 4 */
}
