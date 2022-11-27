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

#define CHARBITSIZE (8)         /* number of bits in a char                  */

struct ffit {                           /* Structure for FIT of motif to HS  */
    float transpose;
    float fit;
    struct ffit *next;
    struct ffit *last;
};

typedef struct ffit *fitptr;

/***************************** GENERATING TEXTURES WITH HFIELD(S) ******************************/
/* RWD Feb 2014: now in sfsys,h, differently! */
#ifdef MAXINT
# undef MAXINT
#endif
#define MAXINT (2147483647)

static int set_hfmotifs(double *hs,motifptr *phrase,int *phrnotecnt,double *phraseamp,
                        double *phrange, noteptr *phrlastnote,int *endhsindex,int **hfphrase,
                        int hsnotecnt,fitptr fithead,double **notestor,dataptr dz);
static int gethfphrase
(double *hs,double **notestor,int z,double lopitch,double hipitch,int **hfphrase,
 int hsnotecnt,int *phrnotecnt,motifptr *phrase,fitptr fithead);
static int shrink_to_hs
(int z,int hsnotecnt,double hipitch,double lopitch,double hstop,double hsbot,
 double *hs, int *hfphrase,motifptr *phrase,int *phrnotecnt);
static int findhfphrase
(int z,double *hs,double hstop,int hsnotecnt,double lopitch,
 double origlo,motifptr *phrase,int **hfphrase,int *phrnotecnt,fitptr fithead,double **notestor);
static int getmtfindeces
(double transpose,int z,int hsnotecnt,double *hs,motifptr *phrase,int *phrnotecnt,int **hfphrase);
static int get_fit(double *notestor,fitptr thisfit,double lopitch,double origlo,int z,
                   int hsnotecnt,int *phrnotecnt,double *hs,fitptr *newfit);
static int put_fit(fitptr thisfit,double transval,double fitval,fitptr *newfit);
static int best_fit(fitptr fithead,double *transpose);
static void free_fitlist(fitptr fithead);
static double hfadjust(double thispitch,double *hs,int x1,int x2,int texflag,int hsnotecnt);
static int convert_pitch_to_nearest_hset_pitch(double thispitch,double *hs,int hsnotecnt);
static int hfadj2(int hsi,int hfnotecnt,int hsnotecnt);
static int setup_hfield(noteptr thisnote,double **hs,int *hsnotecnt,double **hf,int *hfnotecnt,int texflag);
static int get_next_hfield
(double **hf,double **hs,double inputtime,int *hf_cnt,int *hs_cnt,int *hfnotecnt,
 noteptr *thishfnote,double *thishftime,double *nexthftime,int hfdatacnt,int *hsnotecnt,
 noteptr *thishsnote,double *thishstime,double *nexthstime,double *hft,int texflag);
static int setup_motif_note
(double thisamp,int thisinstr,double thisdur,noteptr tsetnote,noteptr phrasenote,
 double *thispitch,double thistime,int *hsindex,int hsnotecnt,double *hs,
 double *pptop,double *ppbot,int *shadindex,char *shadbits,double multiplier,dataptr dz);
static int setup_group_note
(double thisamp,int thisinstr,double thisdur,noteptr tsetnote,
 double *thispitch,double thistime,int *hsindex,int hsnotecnt,double *hs,
 double *pptop,double *ppbot,int *shadindex,char *shadbits,dataptr dz);
static int setup_ornamentation
(int *starthsi,int *endhsi,noteptr *nextevent,noteptr tsetnote,noteptr *phrasenote,
 int *shadindex, int *endhsindex,int *hfphrase,int phrno,dataptr dz);
static int generate_motifin_note_pitch
(int n,int hsindex,int starthsindex,int hsnotecnt,double thispitch,
 int *hfphrase,noteptr *thisnote,double *hs,int texflag);
static int generate_motif_note_pitch
(noteptr tsetnote,noteptr phrasenote,double phrfirstnote,noteptr thisnote);
static int generate_ornament_note_pitch
(int n,double *hs, int hsi, int endhsi,int starthsi,noteptr *thisnote,noteptr *phrasenote,
 int *hfphrase,int hfnotecnt,int hsnotecnt,dataptr dz);
static int do_ev_hfpch
(double thistime,int *hsindex,double *val,
 int hsnotecnt,double *hs,double *pptop,double *ppbot,dataptr dz);
static int getp_as_index
(double thistime,int *val,int hsnotecnt,double *hs,double *pptop,double *ppbot,dataptr dz);
static int init_shadbits(int shadowsize,int *shshsize,char **shadbits);
static int geths(noteptr thisnote,double **hs,int *hsnotecnt);
static int gethf(noteptr thisnote,double **hf,int *hfnotecnt);
static int gen_hs(double **hf,double **hs, int *hsnotecnt, int hfnotecnt);
static int chekrepeat(noteptr thisnote,double lastpitch);
static void set_shadbit(int k,char *shadbits);
static int      geths_lobnd(double thispitch,double *hs,int hsnotecnt);
static int      gethfnote(double thispitch,double *hf,int *hfnotecnt);
static int      setmtfparams
(noteptr thisnote,double thisamp,noteptr phrasenote,noteptr tsetnote,double ampdif,
 double notetime,int gpsize,double multiplier,dataptr dz);
static int      initialise_hfphrases
(int ***hfphrase,int **endhsindex,int **phrnotecnt,double **phraseamp,double **phrange,
 noteptr **phrlastnote,dataptr dz);
static int      readhftimes(noteptr firstnote, int *hfdatacnt,double **hft);
static int      init_fits(fitptr *thisfit);
static int      get_hfgpparams(double thistime,double thispitch,double *hs,int hsnotecnt,
                               int *gpsize,int *hfrange,int *hfgpranglo,int *hfgpranghi,
                               double *gprange,double *gpdense,int mingrpsize,
                               double *gprlow, int dectypecnt,unsigned char dectypestor,dataptr dz);
static int      generate_group_note(noteptr thisnote,noteptr testnote,double thistime,
                                    int hfrange,int hfgpranglo,double *hs,int *hsindex,int gpsize,dataptr dz);
static int      generate_decor_note(noteptr thisnote,noteptr tsetnote,
                                    double thispitch,double thistime,int hfrange,int hfgpranghi,int hfgpranglo,
                                    double *hs,int *hsindex,int hsnotecnt,double gprlow,double gprange,int gpsize,dataptr dz);
static void del_ghosts(int shshsize,char *shadbits,noteptr *shadow,int shadowsize,motifptr tset);
static int  geths_hibnd(double thispitch,int hsnotecnt,double *hs);
static int  geths_above(double thispitch,double *hs,int hsnotecnt);
static int  fit_unlink(fitptr thisfit);
static int  new_fit(fitptr thisfit,fitptr *newfit);
static int      getnexths(double **hs,double inputtime,int *hs_cnt,int *hsnotecnt,noteptr *thishsnote,
                          double *thishstime,double *nexthstime,int hfdatacnt,double *hft);
static int      getnexthf(double **hf,double **hs, int *hsnotecnt,double inputtime,int *hf_cnt,
                          int *hfnotecnt,noteptr *thishfnote,double *thishftime,double *nexthftime,int hfdatacnt,double *hft);
static int      hfscat(int prange,int pbottom,int permindex,int *val,dataptr dz);
static int      gethsnote(double thispitch,double *thishs,int *hsnotecnt);
static void hfsqueezrange
(double thispitch,int *hfrange,int *hfgpranghi,int *hfgpranglo,int hsnotecnt,double *hs);
static int  hforientrange(double thispitch,int *hfrange,int *hfgpranghi,int *hfgpranglo,
                          int hsnotecnt,double *hs,int dectypecnt,unsigned char dectypestor,dataptr dz);
static int  dec_hfcentre(double tsetpitch,double *hs,int hsnotecnt,
                         int hfrange,int hfgpranghi,int hfgpranglo,double gprlow,double gprange,int gpsize,
                         double *val,dataptr dz);
static int  hfscatx(int prange,int pbottom,int permindex,int *val,dataptr dz);
static int  get_first_hs(noteptr thisnote,double **hs,int *hsnotecnt);
static int  get_first_hf(noteptr thisnote,double **hf,int *hfnotecnt);
static int  setup_first_hfield
(noteptr thisnote,double **hs,int *hsnotecnt,double **hf,int *hfnotecnt,int texflag);
static int octeq(double a,double b);

/**************************** DO_SIMPLE_HFTEXTURE ****************************/

int do_simple_hftexture(dataptr dz)
{
    int exit_status;
    unsigned int texflag     = dz->tex->txflag;
    motifptr tset                     = dz->tex->timeset;
    motifptr hfmotif                  = dz->tex->hfldmotif;
    int hsindex;
    noteptr tsetnote = tset->firstnote;
    noteptr nextnote;
    double thistime, thispitch, lastpitch = -1.0;
    noteptr thishfnote = hfmotif->firstnote;
    noteptr thishsnote = hfmotif->firstnote;
    double thishftime,nexthftime,thishstime,nexthstime,thisamp,thisdur;
    unsigned char thisinstr;
    int hf_cnt = 1, hfnotecnt = 0;
    int hs_cnt = 1, hsnotecnt = 0;
    int hfdatacnt = 0;
    double *hft, pptop, ppbot;
    double *hs = (double *)0, *hf = (double *)0;
    dz->iparam[SPINIT] = 0;

    if((exit_status = readhftimes(hfmotif->firstnote,&hfdatacnt,&hft))<0)
        return(exit_status);
    thishftime = nexthftime = thishstime = nexthstime = hft[0];

    if(texflag & ISMANY_HFLDS) {
        if((exit_status = setup_first_hfield(hfmotif->firstnote,&hs,&hsnotecnt,&hf,&hfnotecnt,texflag))<0)
            return(exit_status);
    } else {
        if((exit_status = setup_hfield(hfmotif->firstnote,&hs,&hsnotecnt,&hf,&hfnotecnt,texflag))<0)
            return(exit_status);
    }
    while(tsetnote!=(noteptr)0) {                               /* 3 */
        /*repeated  = */(void) chekrepeat(tsetnote,lastpitch);
        lastpitch = tsetnote->pitch;
        thistime  = (double)tsetnote->ntime;        /* 5 */
        if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
            return(exit_status);
        if(texflag & ISMANY_HFLDS) {
            if((exit_status = get_next_hfield(&hf,&hs,thistime,&hf_cnt,&hs_cnt,
                                              &hfnotecnt,&thishfnote,&thishftime,&nexthftime,hfdatacnt,
                                              &hsnotecnt,&thishsnote,&thishstime,&nexthstime,hft,texflag))<0)
                return(exit_status);
        }
        if((exit_status = do_amp_instr_dur(&thisamp,&thisinstr,&thisdur,tsetnote,thistime,dz))<0)
            return(exit_status);
        if((exit_status = do_ev_hfpch(thistime,&hsindex,&thispitch,hsnotecnt,hs,&pptop,&ppbot,dz))<0)
            return(exit_status);
        tsetnote->pitch = (float)thispitch;
        if(exit_status == CONTINUE) {
            nextnote = tsetnote->next;
            del_note(tsetnote,tset);
            tsetnote = nextnote;
        } else
            tsetnote = tsetnote->next;
    }
    if(tset->firstnote==(noteptr)0) {
        sprintf(errstr,"The harmonic set specified does not tally with the range.\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/**************************** DO_CLUMPED_HFTEXTURE ****************************/

int do_clumped_hftexture(dataptr dz)
{
    int exit_status;
    unsigned int texflag     = dz->tex->txflag;
    unsigned char dectypestor = dz->tex->dectypstor;
    unsigned char dectypecnt  = dz->tex->dectypcnt;
    unsigned char amptypestor = dz->tex->amptypstor;
    unsigned char amptypecnt  = dz->tex->amptypcnt;
    int ampdirected           = dz->tex->ampdirectd;
    motifptr tset                     = dz->tex->timeset;
    motifptr hfmotif                  = dz->tex->hfldmotif;
    motifptr *phrase                  = dz->tex->phrase;
    int **hfphrase = NULL;
    int *endhsindex=NULL, *phrnotecnt=NULL;
    double *phraseamp=NULL, *phrange=NULL;
    unsigned char amptype = 0;
    fitptr fithead = NULL;
    noteptr tsetnote = tset->firstnote;
    noteptr thisnote=NULL, nextnote=NULL, phrasenote=NULL, nextevent=NULL, *shadow=NULL;
    double thispitch = 0.0, lastpitch = -1.0, ampstep = 0.0, ampdif = 0.0, notetime = 0.0, thisamp = 0.0, thisdur = 0.0;
    unsigned char thisinstr;
    double phrfirstnote = 0.0, thistime = 0.0, rangemult = 0.0, gprlow = 0.0;
    double timeadjust = 0.0; /* default */
    noteptr thishfnote = hfmotif->firstnote;
    noteptr thishsnote = hfmotif->firstnote;
    noteptr *phrlastnote = NULL;
    double thishftime = 0.0,nexthftime = 0.0,thishstime = 0.0,nexthstime = 0.0,tsettime = 0.0;
    int hf_cnt = 1, hfnotecnt = 0, mingrpsize = 0, shadindex = 0, shadowsize=0;
    double gprange = 0.0,gpdense = 0.0,multiplier = 0.0;
    double *notestor=NULL;
    int hfgpranglo=0,hfgpranghi=0;
    int hs_cnt = 1, hsnotecnt = 0;
    int hfdatacnt = 0, hfrange = 0, gpsize,shshsize = 0;
    int phrno = 0, n=0,  hsindex=0, starthsindex=0, hfchanged=0;
    int hsi=0, starthsi = 0, endhsi=0;
    char *shadbits = NULL;
    double *hft=NULL, pptop=0.0, ppbot=0.0;
    double *hs = (double *)0, *hf = (double *)0;

    if((dz->tex->phrasecnt > 0) && (exit_status = initialise_hfphrases
                                    (&hfphrase,&endhsindex,&phrnotecnt,&phraseamp,&phrange,&phrlastnote,dz))<0)
        return(exit_status);
    dz->iparam[SPINIT] = 0;
    if(texflag & IS_GROUPS)
        mingrpsize = 1;

    if((exit_status = readhftimes(hfmotif->firstnote,&hfdatacnt,&hft))<0)
        return(exit_status);
    thishftime = nexthftime = thishstime = nexthstime = hft[0];

    if((exit_status = make_shadow(tset,&shadowsize,&shadow))<0)
        return(exit_status);

    if((texflag & MOTIF_IN_HF) || (texflag & IS_ORNATE)) {
        if((exit_status = init_fits(&fithead))<0)
            return(exit_status);
    }
    if(texflag & IS_DECOR)
        setup_decor(&pptop,&ppbot,&shadindex,&tsetnote,dz);
    else {
        if((exit_status = init_shadbits(shadowsize,&shshsize,&shadbits))<0)
            return(exit_status);
    }
    if(texflag & ISMANY_HFLDS) {
        if((exit_status = setup_first_hfield(hfmotif->firstnote,&hs,&hsnotecnt,&hf,&hfnotecnt,texflag))<0)
            return(exit_status);
    } else {
        if((exit_status = setup_hfield(hfmotif->firstnote,&hs,&hsnotecnt,&hf,&hfnotecnt,texflag))<0)
            return(exit_status);
    }
    if((texflag & MOTIF_IN_HF) || ((texflag & IS_ORNATE) && !(texflag & ISMANY_HFLDS))) {
        if((exit_status = set_hfmotifs
            (hs,phrase,phrnotecnt,phraseamp,phrange,phrlastnote,endhsindex,hfphrase,hsnotecnt,fithead,&notestor,dz))<0)
            return(exit_status);
    } else if(texflag & IS_MOTIFS) {
        if((exit_status = set_motifs
            (dz->tex->phrasecnt,phrase,phrnotecnt,phraseamp,phrange,phrlastnote))<0)
            return(exit_status);
    }
    while(tsetnote!=(noteptr)0) {
        if(!(texflag & IS_ORNATE)) {
            /*repeated =*/ (void) chekrepeat(tsetnote,lastpitch);
            lastpitch = tsetnote->pitch;
        }
        tsettime = thistime = (double)tsetnote->ntime;
        if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
            return(exit_status);
        if(texflag & ISMANY_HFLDS) {
            if((exit_status = get_next_hfield(&hf,&hs,thistime,&hf_cnt,&hs_cnt,
                                              &hfnotecnt,&thishfnote,&thishftime,&nexthftime,hfdatacnt,
                                              &hsnotecnt,&thishsnote,&thishstime,&nexthstime,hft,texflag))<0)
                return(exit_status);
            hfchanged = exit_status;
            if(((texflag & MOTIF_IN_HF) || (texflag & IS_ORNATE)) && hfchanged) {
                if((exit_status = set_hfmotifs
                    (hs,phrase,phrnotecnt,phraseamp,phrange,phrlastnote,endhsindex,hfphrase,hsnotecnt,fithead,&notestor,dz))<0)
                    return(exit_status);
            }
        }
        if(texflag & IS_ORN_OR_MTF) {
            if((exit_status = setup_motif_or_ornament(thistime,&multiplier,&phrno,&phrasenote,phrase,dz))<0)
                return(exit_status);
        }
        if((exit_status = do_amp_instr_dur(&thisamp,&thisinstr,&thisdur,tsetnote,thistime,dz))<0)
            return(exit_status);
        if(texflag & IS_DECOR)
            thispitch = tsetnote->pitch;
        if(texflag & IS_MTF_OR_GRP) {
            if(texflag & IS_GROUPS)
                exit_status = setup_group_note
                    (thisamp,thisinstr,thisdur,tsetnote,&thispitch,
                     thistime,&hsindex,hsnotecnt,hs,&pptop,&ppbot,&shadindex,shadbits,dz);
            else
                exit_status = setup_motif_note
                    (thisamp,thisinstr,thisdur,tsetnote,phrasenote,&thispitch,
                     thistime,&hsindex,hsnotecnt,hs,&pptop,&ppbot,&shadindex,shadbits,multiplier,dz);
            if(exit_status==CONTINUE)
                continue;
        }
        if(texflag & IS_ORN_OR_MTF)
            gpsize = phrnotecnt[phrno];
        else {
            if((exit_status = get_hfgpparams
                (thistime,thispitch,hs,hsnotecnt,&gpsize,&hfrange,&hfgpranglo,&hfgpranghi,
                 &gprange,&gpdense,mingrpsize,&gprlow,dectypecnt,dectypestor,dz))<0)
                return(exit_status);
        }
        if(texflag & IS_DECOR) {
            if((exit_status = position_and_size_decoration(&thistime,tsettime,gpdense,&gpsize,dz))<0)
                return(exit_status);
        } else
            gpsize--;
        if(texflag & IS_MOTIFS)
            phrfirstnote = phrasenote->pitch;
        else if(texflag & IS_ORNATE) {
            if((hsi = convert_pitch_to_nearest_hset_pitch((double)tsetnote->pitch,hs,hsnotecnt))<0)
                gpsize = 0;                     /* note out of bounds, ornament omitted  */
            if((exit_status = setup_ornament
                (&timeadjust,&thistime,&gpsize,phrlastnote,multiplier,&phrasenote,phrno,dz))<0)
                return(exit_status);
        }
        if(!(texflag & IS_DECOR))
            nextnote  = tsetnote->next;
        if(gpsize>0) {
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
            } else if (texflag & IS_GROUPS) {
                if((exit_status = set_group_amp
                    (tsetnote,&thisamp,&amptype,&ampstep,gpsize,amptypecnt,amptypestor,dz))<0)
                    return(exit_status);
            } else if (texflag & IS_DECOR) {
                if((exit_status = set_decor_amp
                    (ampdirected,&thisamp,&ampstep,gpsize,&amptype,amptypecnt,amptypestor,dz))<0)
                    return(exit_status);
            }
            if(dz->iparam[TEX_GPSPACE]!=IS_STILL) {
                if((exit_status = init_group_spatialisation(tsetnote,shadindex,shadow,shadowsize,dz))<0)
                    return(exit_status);
            }
            thisnote  = tsetnote;
            if(texflag & MOTIF_IN_HF)
                starthsindex =  *(hfphrase[phrno]);
            else if(texflag & IS_ORNATE) {
                if((exit_status = setup_ornamentation
                    (&starthsi,&endhsi,&nextevent,tsetnote,&phrasenote,&shadindex,endhsindex,hfphrase[phrno],phrno,dz))<0)
                    return(exit_status);
            } else if(texflag & IS_DECOR) {
                nextnote = tsetnote->next;
                nextevent = getnextevent_to_decorate(tsetnote,&shadindex,dz);
            }
            for(n=0;n<gpsize;n++) {
                if((exit_status = make_new_note(&thisnote))<0)
                    return(exit_status);
                if(texflag & IS_ORN_OR_MTF) {
                    if((exit_status = check_next_phrasenote_exists(&phrasenote,texflag,dz))<0)
                        return(exit_status);
                    notetime = getnotetime(phrasenote,thistime,multiplier,timeadjust,dz);
                    thisamp += ampstep;
                    ampdif   = (phraseamp[phrno]-phrasenote->amp)*rangemult;
                    if((exit_status = setmtfparams
                        (thisnote,thisamp,phrasenote,tsetnote,ampdif,notetime,gpsize,multiplier,dz))<0)
                        return(exit_status);
                } else  {
                    if((exit_status = set_group_params
                        (tsetnote,thisnote,gpdense,ampstep,&thisamp,&thistime,thisdur,dz))<0)
                        return(exit_status);
                }
                if(texflag & MOTIF_IN_HF) {
                    if((exit_status = generate_motifin_note_pitch
                        (n,hsindex,starthsindex,hsnotecnt,thispitch,hfphrase[phrno],&thisnote,hs,texflag))<0)
                        return(exit_status);
                } else if(texflag & IS_MOTIFS) {
                    if((exit_status = generate_motif_note_pitch(tsetnote,phrasenote,phrfirstnote,thisnote))<0)
                        return(exit_status);
                } else if (texflag & IS_ORNATE) {
                    if((exit_status = generate_ornament_note_pitch
                        (n,hs,hsi,endhsi,starthsi,&thisnote,&phrasenote,hfphrase[phrno],hfnotecnt,hsnotecnt,dz))<0)
                        return(exit_status);
                } else if(texflag & IS_GROUPS) {
                    if((exit_status = generate_group_note
                        (thisnote,tsetnote,thistime,hfrange,hfgpranglo,hs,&hsindex,gpsize,dz))<0)
                        return(exit_status);
                } else if(texflag & IS_DECOR) {
                    if((exit_status = generate_decor_note
                        (thisnote,tsetnote,thispitch,thistime,hfrange,hfgpranghi,hfgpranglo,
                         hs,&hsindex,hsnotecnt,gprlow,gprange,gpsize,dz))<0)
                        return(exit_status);
                }
                if(exit_status==CONTINUE)
                    continue;
            }
            thisnote->next = nextnote;
            if(nextnote!=(noteptr)0)
                nextnote->last = thisnote;
            if(texflag & IS_ORN_OR_DEC)
                tsetnote = nextevent;
        } else if(texflag & IS_ORN_OR_DEC)
            tsetnote = getnextevent_to_decorate(tsetnote,&shadindex,dz);

        if(texflag & IS_MTF_OR_GRP) {
            tsetnote = nextnote;
            shadindex++;
        }
    }
    if(texflag & IS_MTF_OR_GRP)
        del_ghosts(shshsize,shadbits,shadow,shadowsize,tset);           /* 5 */

    if(texflag & IS_DECOR) {
        if(dz->vflag[DISCARD_ORIGLINE] && (exit_status = erase_shadow(shadowsize,shadow,tset))<0)
            return(exit_status);
    } else
        free(shadbits);
    if((texflag & MOTIF_IN_HF) || (texflag & IS_ORNATE))
        free(fithead);
    return arrange_notes_in_timeorder(tset);
}

/**************************** SET_HFMOTIFS *******************************
 *
 * initialise all parameters of input motifs to be used for MOTIFS or
 * ORNAMENTS, including their HS indexing!!!
 *
 * (1)For each input motif (called a 'phrase').
 *    phrnotecnt[n] = 0;
 * (2)  Initialise count of number of notes to 0, maximum amplitude
 *      of phrase to 0.0, and lowest pitch-index to hsnotecnt.
 * (3)  Go through each note of the motif.
 * (4)  Add up the number of notes in it.
 * (5)  Find it's lowest pitch.
 * (6)  Find it's loudest note (and set as phraseamp).
 * (7)  Establish dynamic range of phrase.
 * (9)  Ensure motif starts at zero time.
 * (10) Generate the corresponding HS indices, in their lowest configuration.
 * (11) Store hfindex of last note of phrase.
 */

int set_hfmotifs
(double *hs,motifptr *phrase,int *phrnotecnt,double *phraseamp,double *phrange,noteptr *phrlastnote,
 int *endhsindex,int **hfphrase,int hsnotecnt,fitptr fithead,double **notestor,dataptr dz)
{
    int exit_status;
    int n;
    double mintime, minamp, minpitch, maxpitch;
    noteptr thisnote, lastnote = (noteptr)0;
    for(n=0;n<dz->tex->phrasecnt;n++) {                                             /* 1 */
        if((exit_status = arrange_notes_in_timeorder(phrase[n]))<0)
            return(exit_status);
        phrnotecnt[n] = 0;
        phraseamp[n] = 0.0;                                                     /* 2 */
        minamp   = DBL_MAX;
        minpitch = DBL_MAX;
        maxpitch = -2.0f;
        mintime  = DBL_MAX;
        thisnote = phrase[n]->firstnote;                        /* 3 */
        while(thisnote!=(noteptr)0) {
            phrnotecnt[n]++;                                                        /* 4 */
            if((double)thisnote->pitch<minpitch)
                minpitch = thisnote->pitch;                     /* 5 */
            if((double)thisnote->pitch>maxpitch)
                maxpitch = thisnote->pitch;                     /* 5 */
            if(thisnote->ntime<mintime)
                mintime  = thisnote->ntime;                     /* 5 */
            if((double)thisnote->amp>phraseamp[n])
                phraseamp[n] = (double)thisnote->amp;/* 6 */
            if(thisnote->amp<minamp)
                minamp = thisnote->amp;                         /* 6 */
            thisnote = thisnote->next;
        }
        if(minamp   == DBL_MAX
           || minpitch == DBL_MAX
           || maxpitch == -2.0f
           || mintime  == DBL_MAX) {
            sprintf(errstr,"Error parsing notelist: set_hfmotifs()\n");
            return(PROGRAM_ERROR);
        }
        thisnote = phrase[n]->firstnote;
        phrange[n] = phraseamp[n] - (double)minamp;     /* 7 */
        while(thisnote!=(noteptr)0) {
            thisnote->ntime = (float)(thisnote->ntime - mintime);                           /* 9 */
            lastnote = thisnote;
            thisnote = thisnote->next;
        }
        if((phrlastnote[n] = lastnote)==(noteptr)0) {
            sprintf(errstr,"Encountered an empty motif: set_hfmotifs()\n");
            return(PROGRAM_ERROR);
        }
        if((exit_status = gethfphrase(hs,notestor,n,minpitch,maxpitch,hfphrase,hsnotecnt,phrnotecnt,phrase,fithead))<0) /* 10 */
            return(exit_status);
        endhsindex[n]  = *(hfphrase[n]+phrnotecnt[n]-1);                        /* 11 */
    }
    return(FINISHED);
}

/*********************** GETHFPHRASE *********************************
 *
 * Get the hfphrase best corresponding to the input motif.
 *
 * (0)  Create storage space to store HS-indices of phrase.
 * (1)  HIpitch of phrase higher than top of HS
 *      AND LOpitch of phrase lower than bottom of HS.
 *      ..Shrink the motif to size of HS, and establish hfphrase.
 * (2)  HIpitch of phrase higher than top of HS
 *      BUT LOpitch within HS range.
 *      .. transpose phraselimits down onto bottom pitch of HS.
 * (a)  If HIpitch of phrase STILL higher than top of HS
 *      ..Shrink the motif to size of HS, and establish hfphrase.
 * (3)  LOpitch of phrase lower than bottom of HS
 *      BUT HIpitch within HS range.
 *      .. transpose phraselimits up onto bottom pitch of HS.
 * (a)  If HIpitch of phrase is NOW higher than top of HS
 *      ..Shrink the motif to size of HS, and establish hfphrase.
 * (4)  Phrase lies with the limits of HS.
 *      .. transpose phraselimits down onto bottom pitch of HS.
 * (5)  Find appropriate hfphrase.
 */

int gethfphrase
(double *hs,double **notestor,int z,double lopitch,double hipitch,
 int **hfphrase,int hsnotecnt,int *phrnotecnt,motifptr *phrase,fitptr fithead)
{
    double origlo = lopitch, hstop = hs[hsnotecnt-1], hsbot = hs[0];
    double stepdown, stepup;
    if((hfphrase[z] = (int *)malloc(phrnotecnt[z] * sizeof(int)))==NULL) {  /* 0 */
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic field phrase array.\n");
        return(MEMORY_ERROR);
    }
    if(hipitch>=hstop) {
        if(lopitch<=hsbot)                              /* 1 */
            return shrink_to_hs(z,hsnotecnt,hipitch,lopitch,hstop,hsbot,hs,hfphrase[z],phrase,phrnotecnt);
        else  {
            stepdown = lopitch - hsbot;                     /* 2 */
            hipitch -= stepdown;
            lopitch -= stepdown;
            if(hipitch>hstop)                               /* a */
                return shrink_to_hs(z,hsnotecnt,hipitch,lopitch,hstop,hsbot,hs,hfphrase[z],phrase,phrnotecnt);
        }
    } else {
        if(lopitch<=hsbot) {                            /* 3 */
            stepup  = hsbot - lopitch;
            hipitch += stepup;
            lopitch += stepup;
            if(hipitch>hstop)                               /*a */
                return shrink_to_hs(z,hsnotecnt,hipitch,lopitch,hstop,hsbot,hs,hfphrase[z],phrase,phrnotecnt);
        } else {                                        /* 4 */
            stepdown = lopitch - hsbot;
            hipitch -= stepdown;
            lopitch -= stepdown;
        }
    }
    return findhfphrase(z,hs,hstop,hsnotecnt,lopitch,origlo,phrase,hfphrase,phrnotecnt,fithead,notestor);
}                                                                                       /* 5 */


/************************* SHRINK_TO_HS **************************
 *
 * Shrink motif so it fits with HS, then find best HS fit.
 *
 * (1)  Establish pitch-shrinking ratio.
 * (2)  For every note...
 * (3)  Shrink intervals and thus move pitch to appropriate position inside
 *      range of HS.
 * (4)  Find index of the next lowest HS notes.
 * (4a) If note falls outside field, set distance to note to maximum poss.
 * (4b) Else find pitch distances of this note from original pitch.
 * (5)  Find index of the next highest HS notes.
 * (5a) If note falls outside field, set distance to note to maximum poss.
 * (5b) Else find pitch distances of this note from original pitch.
 * (6)  Select HSindex of nearest pitch and store in hfphrase.
 * (7)  Save the lowest such index.
 * (8)  Transpose all indices down to lowest possible values.
 */

int shrink_to_hs(int z,int hsnotecnt,double hipitch,double lopitch,double hstop,
                 double hsbot,double *hs, int *hfphrase, motifptr *phrase,int *phrnotecnt)
{
    noteptr thisnote = phrase[z]->firstnote;
    double shrink ,thispitch, x, y;
    int minindex = MAXINT, xx, yy, q, n = 0;
    shrink = (hstop - hsbot)/(hipitch-lopitch);                     /* 1 */
    for(;;) {                                                       /* 2 */
        thispitch = thisnote->pitch;
        thispitch = ((thispitch - lopitch) * shrink) + hsbot;   /* 3 */
        if((xx = geths_lobnd(thispitch,hs,hsnotecnt))<0)                        /* 4 */
            x = DBL_MAX;                                            /* 4a */
        else
            x = fabs(hs[xx] - thispitch);                   /* 4b */
        if((yy = geths_hibnd(thispitch,hsnotecnt,hs))>=hsnotecnt)               /* 5 */
            y = DBL_MAX;                                            /* 5a */
        else
            y = fabs(hs[yy] - thispitch);                   /* 5b */
        if(x<y)
            q = xx;                                         /* 6 */
        else
            q = yy;
        *(hfphrase+n) = q;
        if(q<minindex)
            minindex = q;                                   /* 7 */
        if((thisnote = thisnote->next)==(noteptr)0)
            break;
        if(++n >= phrnotecnt[z]) {
            sprintf(errstr,"accounting problem in shrink_to_hs()\n");
            return(PROGRAM_ERROR);
        }
    }
    for(n=0;n<phrnotecnt[z];n++)                                    /* 8 */
        *(hfphrase+n) -= minindex;
    return(FINISHED);
}

/*********************** FINDHFPHRASE *********************************
 *
 * Extract the best fit onto the HS, and transpose it down to bottom
 * of indices.
 *
 * (1)  Set the FIT-pointer to head of fits-list.
 * (2)  Set up a store for the note values to be tested.
 * (3)  Get interval of transposition to take motif to bottom of HS range.
 * (4)  Store original note values of phrase transposed down to bottom of HS range.
 * (5)  Find the fit-value for motif in its start position.
 * (6)  Outer loop will transpose the phrase through a range...
 *              (7)     Set amount to transpose phrase (shift) to max.
                        Then for every note of the phrase.....
                        *              (8) Find the HSindex of the nearest note above the current note.
                        *              (If there isn't one break out completely).
                        *              (9)  If the difference in pitch between this and the current note
                        *              is smaller than the minimum shift, reset minimum shift.
                        *              (10)Then, for every note of the phrase...
                        *              (11)transpose it up by this minimum transposition.
                        *                      If this takes any note of the phrase outside the range of HS
                        *                      break out completely.
                        *      (12)Back in outer loop, find the fitting-value for this transposition
                        *                      of the motif, which is a measure of how far its notes are from
                        *                      the closest HS values.
                        * (13) Outside the outer loop, unlink the last unwanted member iof fitlist.
                        * (14) Find the best fitvalue, and return the amount by which orig motif
                        *              should be transposed to get onto this best fit.
                        * (15) Free the fit-list (retains fithead).
                        * (16) Get the appropriate HS indices.
                        */

int findhfphrase
(int z,double *hs,double hstop,int hsnotecnt,double lopitch,double origlo,
 motifptr *phrase,int **hfphrase,int *phrnotecnt,fitptr fithead,double **notestor)
{
    int exit_status;
    int hsindex, OK = 1, n, got_transpose = 0;
    noteptr thisnote = phrase[z]->firstnote;
    fitptr thisfit = fithead;                                                                       /* 1 */
    double shift, thisshift, transpose = 0.0, stepdown, *noteval;
    if(*notestor!=NULL)
        free(*notestor);                                                                                /* 2 */
    if((noteval = *notestor = (double *)malloc(phrnotecnt[z] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for note value array.\n");
        return(MEMORY_ERROR);
    }
    stepdown = origlo - lopitch;                                                            /* 3 */
    while(thisnote!=(noteptr)0) {
        *noteval = (double)thisnote->pitch - stepdown;                  /* 4 */
        noteval++;
        thisnote = thisnote->next;
    }
    if((exit_status = get_fit(*notestor,thisfit,lopitch,origlo,z,hsnotecnt,phrnotecnt,hs,&thisfit))<0)
        return(exit_status);                                                                    /* 5 */
    if(exit_status == CONTINUE) {
        transpose = thisfit->last->transpose;
        got_transpose = 1;
        OK  = 0;
    }
    while(OK) {                                                                                                     /* 6 */
        shift = DBL_MAX;                                                                                /* 7 */
        noteval = *notestor;
        for(n=0;n<phrnotecnt[z];n++) {
            if((hsindex = geths_above(*noteval,hs,hsnotecnt))>=hsnotecnt) { /* 8 */
                OK = 0;
                break;
            }
            if((thisshift  = hs[hsindex] - *noteval)<shift)
                shift = thisshift;                                                              /* 9 */
            noteval++;
        }
        if(shift==DBL_MAX) {
            sprintf(errstr,"Error parsing phrase: findhfphrase()\n");
            return(PROGRAM_ERROR);
        }
        if(!OK)
            break;
        noteval = *notestor;
        for(n=0;n<phrnotecnt[z];n++) {                                                  /* 10 */
            if((*noteval++  += (float)shift)>hstop) {
                OK = 0;                                                                                 /* 11 */
                break;
            }
        }
        lopitch += shift;
        if((exit_status = get_fit(*notestor,thisfit,lopitch,origlo,z,hsnotecnt,phrnotecnt,hs,&thisfit))<0)
            return(exit_status);                                                            /* 12 */
        if(exit_status == CONTINUE) {
            transpose = thisfit->last->transpose;
            got_transpose = 1;
            break;
        }
    }
    if((exit_status = fit_unlink(thisfit))<0)                                       /* 13 */
        return(exit_status);
    if (!got_transpose) {
        if((exit_status = best_fit(fithead,&transpose))<0)              /* 14 */
            return(exit_status);
    }
    free_fitlist(fithead);                                                                          /* 15 */
    return getmtfindeces(transpose,z,hsnotecnt,hs,phrase,phrnotecnt,hfphrase);
}                                                                                                                               /* 16 */

/*************************** GETMTFINDECES *****************************
 *
 * Get the HS indices of the phrase, transposed to lowest position in HS.
 *
 * (0)  Point to start of storage space for the HS-indices of the phrase.
 * (1)  Mark end of this storage space (for accounting purposes).
 * (2)  For every note of the phrase...
 *   (3)  Get the pitch..
 *   (4)  Transpose the pitch by the input transposition.
 *   (5)  Find the nearest HS set members above and below the note.
 *   (6)  Decide which is nearest.
 *   (7)  Assign index of nearest value to the HS-index store.
 *   (8)  Keep a note of the smallest index.
 *   (9)  Increment the index-store pointer.
 * (10) Decrement all the indeces to tranpose them all to lowest values
 *      in the hfphrase store.
 */

int getmtfindeces(double transpose,int z,int hsnotecnt,double *hs,motifptr *phrase,int *phrnotecnt,int **hfphrase)
{
    noteptr thisnote = phrase[z]->firstnote;
    int *thisindex, xx, yy, minindex = MAXINT, *indexend;
    double x, y, thispitch;
    thisindex = hfphrase[z];                                                /* 0 */
    indexend  = hfphrase[z] + phrnotecnt[z];                /* 1 */
    for(;;) {                                                                               /* 2 */
        thispitch  = thisnote->pitch;                           /* 3 */
        thispitch += transpose;                                         /* 4 */
        xx = geths_lobnd(thispitch,hs,hsnotecnt);       /* 5 */
        yy = geths_hibnd(thispitch,hsnotecnt,hs);
        x = fabs(hs[xx] - thispitch);                           /* 6 */
        y = fabs(hs[yy] - thispitch);
        if(x<y)
            *thisindex = xx;                                                /* 7 */
        else
            *thisindex = yy;
        if(*thisindex<minindex)
            minindex = *thisindex;                                  /* 8 */
        if((thisnote = thisnote->next)==(noteptr)0)
            break;
        if(++thisindex>=indexend) {                                     /* 9 */
            sprintf(errstr,"TEXTURE: Problem in getmtfindices()\n");
            return(PROGRAM_ERROR);
        }
    }
    for(thisindex = hfphrase[z];thisindex<indexend;thisindex++)
        *thisindex -= minindex;                                         /* 10 */
    return(FINISHED);
}

/****************************** GET_FIT ************************************
 *
 * Create list of fit_values for motif over HS.
 *
 * (1)  For every note of the phrase.
 * (3)  Find the index of the next lowest HS note. If there is not one
 *      something is wrong!
 * (4)  Find the index of the next highest HS note. If there is not one
 *      something is wrong!
 * (5)  Find the pitch distance between these notes on the original pitch.
 * (6)  Add the minimum of these two, to a running sum of such differences.
 * (7)  Store the fit-value and the associated ttansposition, creating a
 *      new storage space in the fit list, in the process.
 */

int get_fit(double *notestor,fitptr thisfit,double lopitch,double origlo,int z,
            int hsnotecnt,int *phrnotecnt,double *hs,fitptr *newfit)
{
    int exit_status;
    double x, y, sum = 0.0, *noteval = notestor;
    int xx, yy, n;
    for(n=0;n<phrnotecnt[z];n++) {                                                          /* 1 */
        if((xx = geths_lobnd(*noteval,hs,hsnotecnt))<0) {               /* 3 */
            sprintf(errstr,"Problem 1 in get_fit()\n");
            return(PROGRAM_ERROR);
        }
        if((yy=geths_hibnd(*noteval,hsnotecnt,hs))>=hsnotecnt) {/* 4 */
            sprintf(errstr,"Problem 2 in get_fit()\n");
            return(PROGRAM_ERROR);
        }
        x = fabs(hs[xx] - *noteval);                                                    /* 5 */
        y = fabs(hs[yy] - *noteval);
        sum += min(x,y);                                                                                /* 6 */
        noteval++;
    }
    if((exit_status = put_fit(thisfit,lopitch - origlo,sum,newfit))<0)
        return(exit_status);
    if(flteq(sum,0.0))
        return(CONTINUE);
    return(FINISHED);                                                                                       /* 7 */
}

/************************* PUT_FIT *****************************
 *
 * Put fit value in motif-fits list.
 */

int put_fit(fitptr thisfit,double transval,double fitval,fitptr *newfit)
{
    int exit_status;
    thisfit->transpose = (float)transval;
    thisfit->fit       = (float)fitval;
    if((exit_status = new_fit(thisfit,newfit))<0)
        return(exit_status);
    return(FINISHED);
}

/************************* BEST_FIT *****************************
 *
 * Find the appropriate transposition to move the current motif
 * to a pitch where it has the best fit with the HS.
 *
 * (1)  From all available transpositions, find best (lowest) fit value.
 * (2)  From all available transpositions, find all those which have
 *      this bestfit value, and select the one which involves the
 *      least transposition.
 * (3)  Once the transpositions begin to increase, we are moving away from
 *      the smallest transposition, so quit.
 * (4)  Return the best transposition.
 */

int best_fit(fitptr fithead,double *transpose)
{
    fitptr thisfit = fithead;
    double bestfit = DBL_MAX, besttranspose = DBL_MAX;
    double abstranspose = DBL_MAX, thistranspose;
    while(thisfit!=(fitptr)0) {                             /* 1 */
        if(thisfit->fit<(float)bestfit)
            bestfit = (double)thisfit->fit;
        thisfit = thisfit->next;
    }
    thisfit = fithead;
    while(thisfit!=(fitptr)0) {                             /* 2 */
        thistranspose = fabs((double)thisfit->transpose);
        if(flteq((double)thisfit->fit,bestfit)) {
            if(thistranspose<abstranspose) {
                besttranspose = (double)thisfit->transpose;
                abstranspose = fabs(besttranspose);
            }
        }
        if(thistranspose>abstranspose+FLTERR)
            break;                                  /* 3 */
        thisfit = thisfit->next;
    }
    if(besttranspose==DBL_MAX) {
        sprintf(errstr,"Problem in best_fit()\n");
        return(PROGRAM_ERROR);
    }
    *transpose = besttranspose;
    return(FINISHED);                               /* 4 */
}

/************************* INIT_FITS() *****************************
 *
 * Set up head item of a list of motif-fits.
 */

int init_fits(fitptr *thisfit)
{
    if((*thisfit = (fitptr)malloc(sizeof (struct ffit)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for fitting array.\n");
        return(MEMORY_ERROR);
    }
    (*thisfit)->next  = (fitptr)0;
    (*thisfit)->last  = (fitptr)0;
    return(FINISHED);
}

/************************* NEW_FIT() *****************************
 *
 * Set up next fit in a list of motif-fits.
 */

int new_fit(fitptr thisfit,fitptr *newfit)
{
    if((*newfit = (fitptr)malloc(sizeof (struct ffit)))==NULL) {
        sprintf(errstr,"new_fit()\n");
        return(PROGRAM_ERROR);
    }
    thisfit->next   = *newfit;
    (*newfit)->last = thisfit;
    (*newfit)->next = (fitptr)0;
    return(FINISHED);
}

/********************** FIT_UNLINK() ******************************
 *
 * Deletes empty address space at end of fitlist.
 */

int fit_unlink(fitptr thisfit)
{
    if(thisfit->last==(fitptr)0) {
        sprintf(errstr,"Problem in fit_unlink()\n");
        return(PROGRAM_ERROR);
    }
    thisfit = thisfit->last;
    free(thisfit->next);
    thisfit->next = (fitptr)0;
    return(FINISHED);
}

/********************** FREE_FITLIST() ******************************
 *
 * Deletes fitlist, retaining head.
 */

void free_fitlist(fitptr fithead)
{
    fitptr thisfit;
    if((thisfit = fithead->next)==(fitptr)0)
        return;
    while(thisfit->next!=(fitptr)0) {
        thisfit = thisfit->next;
        free(thisfit->last);
    }
    free(thisfit);
}

/**************************** HFADJUST *********************************
 *
 * Pitch outside HS range. Adjust it.
 *
 * (1)  If this is a HS, there's nothing we can do about it. Return -1.0
 *      which will cause note to be deleted in calling environment.
 * (2)  If it's an HF however....
 *      Set newpitch at appropriate interval from original pitch.
 * (3)  If it's now above the HS
 * (4)  transpose it down by octaves until it is in the HS.
 * (5)  If still not in HS, mark for deletion.
 * (6)  If it's BELOW the HS.
 * (7)  transpose it up by octaves until it is in the HS.
 * (8)  If still not in HS, mark for deletion.
 * (9)  Return new pitch.
 */

double hfadjust(double thispitch,double *hs,int x1,int x2,int texflag,int hsnotecnt)
{
    double newpitch;
    if(texflag & IS_HS)
        return(-1.0);                                                           /* 1 */
    else {
        newpitch = thispitch+hs[x1]-hs[x2];                     /* 2 */
        if(newpitch > hs[hsnotecnt-1]) {                        /* 3 */
            while(newpitch>hs[hsnotecnt-1])
                newpitch -= SEMITONES_PER_OCTAVE;       /* 4 */
            if(newpitch<hs[0])                                              /* 5 */
                return(-1.0);
        } else {
            if(newpitch<hs[0]) {                                    /* 6 */
                while(newpitch <hs[0])
                    newpitch += SEMITONES_PER_OCTAVE;/* 7 */
                if(newpitch>hs[hsnotecnt-1])
                    return(-1.0);                                   /* 8 */
            }
        }
    }
    return(newpitch);                                                               /* 9 */
}

/**************************** CONVERT_PITCH_TO_NEAREST_HSET_PITCH *********************************
 *
 * Convert pitch value to nearest HS value.
 *
 * ignores the problem of note repetitions in the tset.
 * Note repetitions were only important where the tset notes were being
 * forced onto an HS. Here the HS values are being extracted so the notes
 * can be ornamented. The pitch values of the tset are NOT themselves altered.
 */

int convert_pitch_to_nearest_hset_pitch(double thispitch,double *hs,int hsnotecnt)
{
    int n, m;
    if(thispitch<hs[0]) {
        if(hs[0] - thispitch > hs[1] - hs[0])
            return(-1);
        else
            return(0);
    }
    n = 1;
    m = n-1;
    while(n<hsnotecnt) {
        if(flteq(hs[n],thispitch))
            return(n);
        if(hs[n]>thispitch) {
            if(hs[n] - thispitch > thispitch - hs[m])
                return(m);
            else
                return(n);
        }
        n++;
        m++;
    }
    if(hsnotecnt>1 && (thispitch - hs[hsnotecnt-1] > hs[hsnotecnt-1] - hs[hsnotecnt-2]))
        return(-1);
    return(hsnotecnt-1);
}

/**************************** HFADJ2 *********************************
 *
 * HFindex outside HS range. Adjust it.
 * Applies only to HF (i.e. an HS containing octave transpositions)
 * not to HS.
 */

int hfadj2(int hsi,int hfnotecnt,int hsnotecnt)
{
    if(hsi<0) {
        while(hsi<0)
            hsi += hfnotecnt;
    } else {
        while(hsi>=hsnotecnt)
            hsi -= hfnotecnt;
    }
    if(hsi<0 || hsi>=hsnotecnt)
        return(-1);
    return(hsi);
}

/**************************** SETUP_HFIELD *********************************/

int setup_hfield(noteptr thisnote,double **hs,int *hsnotecnt,double **hf,int *hfnotecnt,int texflag)
{
    int exit_status;
    if(texflag & IS_HS) {
        if((exit_status = geths(thisnote,hs,hsnotecnt))<0)
            return(exit_status);
    } else {
        if((exit_status = gethf(thisnote,hf,hfnotecnt))<0)                                      /* 2 */
            return(exit_status);
        if((exit_status = gen_hs(hf,hs,hsnotecnt,*hfnotecnt))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/**************************** SETUP_FIRST_HFIELD *********************************/

int setup_first_hfield(noteptr thisnote,double **hs,int *hsnotecnt,double **hf,int *hfnotecnt,int texflag)
{
    int exit_status;
    if(texflag & IS_HS) {
        if((exit_status = get_first_hs(thisnote,hs,hsnotecnt))<0)
            return(exit_status);
    } else {
        if((exit_status = get_first_hf(thisnote,hf,hfnotecnt))<0)                                       /* 2 */
            return(exit_status);
        if((exit_status = gen_hs(hf,hs,hsnotecnt,*hfnotecnt))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/**************************** GET_NEXT_HFIELD *********************************/

int get_next_hfield(double **hf,double **hs,double inputtime,
                    int *hf_cnt,int *hs_cnt,int *hfnotecnt,noteptr *thishfnote,double *thishftime,double *nexthftime,int hfdatacnt,
                    int *hsnotecnt,noteptr *thishsnote,double *thishstime,double *nexthstime,double *hft,int texflag)
{
    if(texflag & IS_HS)
        return getnexths(hs,inputtime,hs_cnt,hsnotecnt,thishsnote,thishstime,nexthstime,hfdatacnt,hft);
    else
        return getnexthf(hf,hs,hsnotecnt,inputtime,hf_cnt,hfnotecnt,thishfnote,thishftime,nexthftime,hfdatacnt,hft);
}

/**************************** SETUP_MOTIF_NOTE *********************************
 *
 * (1)  If the program is given a pitchrange OUTSIDE the bounds of the
 *              specified HF or HS, do_ev_hfpch() returns a -ve value. In this
 *              case, the associated tset note is marked for DELETION by setting
 *              the bitflag associated with the shadow (address) of the tset note.
 *              It will be deleted at the very end, by del_ghosts().
 */

int setup_motif_note
(double thisamp,int thisinstr,double thisdur,noteptr tsetnote,noteptr phrasenote,
 double *thispitch,double thistime,int *hsindex,int hsnotecnt,double *hs,
 double *pptop,double *ppbot,int *shadindex,char *shadbits,double multiplier,dataptr dz)
{
    int exit_status;
    tsetnote->amp = (float)thisamp;
    tsetnote->instr = (unsigned char)thisinstr;
    if(dz->vflag[DONT_KEEP_MTFDUR])
        tsetnote->dur = (float)thisdur;
    else
        tsetnote->dur = (float)(phrasenote->dur * multiplier);
    if(flteq((double)tsetnote->dur,0.0)) {
        sprintf(errstr,"setup_motif_note(): Zero duration\n");
        return(PROGRAM_ERROR);
    }
    if((exit_status = do_ev_hfpch(thistime,hsindex,thispitch,hsnotecnt,hs,pptop,ppbot,dz))<0)
        return(exit_status);
    if(exit_status==CONTINUE) {
        set_shadbit(*shadindex,shadbits);    /* 1 */
        tsetnote = tsetnote->next;
        (*shadindex)++;
        return(CONTINUE);
    }
    tsetnote->pitch = (float)(*thispitch);
    return(FINISHED);
}

/**************************** SETUP_GROUP_NOTE *********************************/

int setup_group_note
(double thisamp,int thisinstr,double thisdur,noteptr tsetnote,
 double *thispitch,double thistime,int *hsindex,int hsnotecnt,double *hs,
 double *pptop,double *ppbot,int *shadindex,char *shadbits,dataptr dz)
{
    int exit_status;
    tsetnote->amp = (float)thisamp;
    tsetnote->instr = (unsigned char)thisinstr;
    tsetnote->dur = (float)thisdur;
    if((exit_status = do_ev_hfpch(thistime,hsindex,thispitch,hsnotecnt,hs,pptop,ppbot,dz))<0)   /* 1 */
        return(exit_status);
    if(exit_status==CONTINUE) {
        set_shadbit(*shadindex,shadbits);
        tsetnote = tsetnote->next;
        (*shadindex)++;
        return(CONTINUE);
    }
    tsetnote->pitch = (float)(*thispitch);
    return(FINISHED);
}

/**************************** SETUP_ORNAMENTATION *********************************/

int setup_ornamentation
(int *starthsi,int *endhsi,noteptr *nextevent,noteptr tsetnote,
 noteptr *phrasenote,int *shadindex,int *endhsindex,int *hfphrase,int phrno,dataptr dz)
{
    *starthsi  =  *hfphrase;
    *endhsi    =  endhsindex[phrno];
    *nextevent = getnextevent_to_decorate(tsetnote,shadindex,dz);
    if(!dz->vflag[IS_PRE])
        *phrasenote = (*phrasenote)->next;
    return(FINISHED);
}

/**************************** GENERATE_MOTIF_NOTE_PITCH *********************************/

int generate_motifin_note_pitch(int n,int hsindex,int starthsindex,int hsnotecnt,double thispitch,
                                int *hfphrase,noteptr *thisnote,double *hs,int texflag)
{
    int notehsindex, thishsindex;
    double newpitch;
    notehsindex = *(hfphrase+n+1);
    thishsindex = hsindex + notehsindex - starthsindex;
    if(thishsindex<0 || thishsindex>=hsnotecnt) {
        if((newpitch = hfadjust(thispitch,hs,notehsindex,starthsindex,texflag,hsnotecnt))<0.0) {
            *thisnote = (*thisnote)->last;
            free((*thisnote)->next);
            return(CONTINUE);
        } else
            (*thisnote)->pitch = (float)newpitch;
    } else
        (*thisnote)->pitch=(float)hs[thishsindex];
    return(FINISHED);
}

/**************************** GENERATE_MOTIF_NOTE_PITCH *********************************/

int generate_motif_note_pitch(noteptr tsetnote,noteptr phrasenote,double phrfirstnote,noteptr thisnote)
{
    thisnote->pitch=(float)(tsetnote->pitch + phrasenote->pitch - phrfirstnote);
    thisnote->pitch=(float)octadjust((double)thisnote->pitch);
    return(FINISHED);
}

/**************************** GENERATE_ORNAMENT_NOTE_PITCH *********************************
 *
 * hsi = harmonic-set index.
 */

int generate_ornament_note_pitch
(int n,double *hs,int hsi, int endhsi,int starthsi,noteptr *thisnote,
 noteptr *phrasenote,int *hfphrase,int hfnotecnt,int hsnotecnt,dataptr dz)
{
    unsigned int texflag = dz->tex->txflag;
    int thishsi, refhsi, notehsi;

    if(dz->vflag[IS_PRE]) {
        refhsi = endhsi;
        notehsi = *(hfphrase+n);
    } else {
        refhsi = starthsi;
        notehsi = *(hfphrase+n+1);
    }
    thishsi = hsi + notehsi - refhsi;
    *phrasenote = (*phrasenote)->next;
    if(thishsi<0 || thishsi>=hsnotecnt) {
        if((texflag & IS_HS) || (thishsi=hfadj2(thishsi,hfnotecnt,hsnotecnt))<0) {
            *thisnote = (*thisnote)->last;
            free((*thisnote)->next);
            return(CONTINUE);
        }
    }
    (*thisnote)->pitch=(float)hs[thishsi];
    return(FINISHED);
}

/********************** DO_EV_HFPCH *******************************
 *
 * Generate pitch of event (note group etc.) within HS.
 *
 * (2)  If getp_as_index() returns -1 , this means
 *      that the pitch range specified is outside the HS, so a value of
 *      -1.0 is returned to calling environment and tset note is DELETED!!
 */

int do_ev_hfpch(double thistime,int *hsindex,double *val,int hsnotecnt,double *hs,double *pptop,double *ppbot,dataptr dz)
{
    int exit_status;
    if((exit_status = getp_as_index(thistime,hsindex,hsnotecnt,hs,pptop,ppbot,dz))<0)
        return(exit_status);
    if(*hsindex==-1)                                /* a */
        return(CONTINUE);                       /* 2 */
    *val = hs[*hsindex];
    return(FINISHED);
}

/****************************** GETP_AS_INDEX *********************************
 *
 * Get integer value of a hf-pitch index, using pitch either from table,
 * or fixed value.
 *
 * (1)  Read values of pitch-range limits from tables (or fixed vals).
 * (2)  If either of pitch values is negative, if it's from a table,
 *      reject it....
 * (3)  But otherwise it can be regarded as a flag. Return -1 causing
 *      the INPUT pitch to be converted to a HS pitch and returned.
 * (4)  Check that range is not inverted.
 * (5)  Get the note in HS which is >  upper pitch boundary. If all HS notes
 *      are above this (function returns 0) return -1.
 *      If all HS notes are BELOW this pitch, function returns hsnotecnt,
 *      wehich forces upper limit of HSindex generated by doperm() to be
 *      hsnotecnt-1, the highest pitch in the HS.
 * (6)  Get the note in HS which is >= lower pitch boundary. If there
 *      is no such note (function returns hsnotecnt) return -1.
 * (7)  Return a weighted scattered value of hf-index, within the
 *      defined range.
 */

int getp_as_index(double thistime,int *val,int hsnotecnt,double *hs,double *pptop,double *ppbot,dataptr dz)
{
    int exit_status;
    int  a, b;
    double range;
    if(dz->brk[TEXTURE_MAXPICH]) {                                          /* 1 */
        if((exit_status = read_value_from_brktable(thistime,TEXTURE_MAXPICH,dz))<0)
            return(exit_status);
    }
    if(dz->brk[TEXTURE_MINPICH]) {
        if((exit_status = read_value_from_brktable(thistime,TEXTURE_MINPICH,dz))<0)
            return(exit_status);
    }
    *pptop = dz->param[TEXTURE_MAXPICH];
    *ppbot = dz->param[TEXTURE_MINPICH];
    if((range = dz->param[TEXTURE_MAXPICH] - dz->param[TEXTURE_MINPICH])<0.0)               /* 4 */
        swap (&dz->param[TEXTURE_MAXPICH],&dz->param[TEXTURE_MINPICH]);
    if((a = geths_above(dz->param[TEXTURE_MAXPICH],hs,hsnotecnt))==0) {                             /* 5 */
        *val = -1;
        return(FINISHED);
    }
    if((b = geths_hibnd(dz->param[TEXTURE_MINPICH],hsnotecnt,hs))>=hsnotecnt) {             /* 6 */
        *val = -1;
        return(FINISHED);
    }
    if((exit_status = hfscat(a-b,b,PM_PITCH,&a,dz))<0)      /* 7 */
        return(exit_status);
    if( a < 0 || a >= hsnotecnt) {                  /* 7 */
        sprintf(errstr,"TEXTURE: getp_as_index(): TW's logic of index generation wrong\n");
        return(DATA_ERROR);
    }
    *val = a;
    return(FINISHED);
}

/**************************** INIT_SHADBITS ****************************
 *
 * Set up, and initialise to zero, a bitflag with one bit for each shadow
 * (tset address).
 */

int init_shadbits(int shadowsize,int *shshsize,char **shadbits)
{
    int n = shadowsize;
    *shshsize = 1;
    while((n -= CHARBITSIZE)>0)
        (*shshsize)++;
    if((*shadbits = (char *)malloc((size_t)(*shshsize)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for shadwoing array.\n");
        return(MEMORY_ERROR);
    }
    for(n=0; n<(*shshsize); n++)
        (*shadbits)[n] = 0;
    return(FINISHED);
}

/****************************** GETHS ***********************************
 *
 * Extract HS from input data.
 */

int geths(noteptr thisnote,double **hs,int *hsnotecnt)
{
    int exit_status;
    int size = BIGARRAY;
    if((*hs = (double *)malloc(BIGARRAY * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic set array.\n");
        return(MEMORY_ERROR);
    }
    while(thisnote!=(noteptr)0) {
        if((exit_status = gethsnote(thisnote->pitch,*hs,hsnotecnt))<0)
            return(exit_status);
        if(*hsnotecnt >= BIGARRAY) {
            size += BIGARRAY;
            if((*hs=(double *)realloc((char *)(*hs),size * sizeof(double)))==NULL){
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic set array.\n");
                return(MEMORY_ERROR);
            }
        }
        thisnote = thisnote->next;
    }
    if((*hs=(double *)realloc((char *)(*hs),*hsnotecnt*sizeof(double)))==NULL){
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic set array.\n");
        return(MEMORY_ERROR);
    }
    upsort(*hs,*hsnotecnt);
    return(FINISHED);
}

/****************************** GETHF ***********************************
 *
 * Extract HF from input data.
 */

int gethf(noteptr thisnote,double **hf,int *hfnotecnt)
{
    int exit_status;
    int size = BIGARRAY;
    if((*hf = (double *)malloc(size * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic field array.\n");
        return(MEMORY_ERROR);
    }
    while(thisnote!=(noteptr)0) {
        if((exit_status = gethfnote(thisnote->pitch,*hf,hfnotecnt))<0)
            return(exit_status);
        if(*hfnotecnt >= size) {
            size += BIGARRAY;
            if((*hf = (double *)realloc((char *)(*hf),size * sizeof(double)))==NULL){
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic field array.\n");
                return(MEMORY_ERROR);
            }
        }
        thisnote = thisnote->next;
    }
    if((*hf=(double *)realloc((char *)(*hf),*hfnotecnt * sizeof(double)))==NULL){
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic field array.\n");
        return(MEMORY_ERROR);
    }
    upsort(*hf,*hfnotecnt);
    return(FINISHED);
}

/****************************** GEN_HS ***********************************
 *
 * Generate HS from a HF.
 *
 * NB This function assumes thas HF is ordered in ascending pitch order.
 *
 * (1)  Eliminate octave dupliates in hf.
 * (2)  Initialise the HS count to zero.
 * (3)  Start in the lowest octave.
 * (4)  Go round loop generating HS, until we're out of (MIDI) range.
 * (5)  Each time round loop, point to start of HF.
 * (6)  And for all HF members, put them in HS + a (loop) number of 8ves,
 *      counting the HS as we go.
 * (7)  Once the pitch of any note exceeds MIDI upper limit (MIDITOP), break
 *      from inner and (OK=0) outer loops.
 * (8)  Each time round outer loop, increment octave.
 * (9)  Reallocate the HS to the store 'hs'
 */

int gen_hs(double **hf,double **hs, int *hsnotecnt, int hfnotecnt)
{
    int n, m, k, thisoct, OK = 1;
    int size = BIGARRAY;
    double nextpitch;
    double *thishf;
    if((*hs)!=(double *)0)
        free(*hs);
    if((*hs = (double *)malloc(size * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic set array.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<hfnotecnt-1;n++) {            /* 1 */
        for(m=n+1;m<hfnotecnt;m++) {
            if(octeq((*hf)[n],(*hf)[m])) {
                k = m + 1;
                while(k < hfnotecnt) {
                    (*hf)[k-1] = (*hf)[k];
                    k++;
                }
                hfnotecnt--;
                m--;
            }
        }
    }
    *hsnotecnt = 0;                                                 /* 2 */
    thisoct = 0;                                            /* 3 */
    while(OK) {                                                     /* 4 */
        thishf = *hf;                                           /* 5 */
        for(n=0;n<hfnotecnt;n++) {                                      /* 6 */
            if((nextpitch = (*thishf++)+((double)thisoct*SEMITONES_PER_OCTAVE))<=MIDITOP) {
                (*hs)[(*hsnotecnt)++] = nextpitch;
                if(*hsnotecnt>=size) {
                    size += BIGARRAY;
                    if((*hs =(double *)realloc((char *)(*hs),size * sizeof(double)))==NULL) {
                        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic set array.\n");
                        return(MEMORY_ERROR);
                    }
                }
            } else {
                OK = 0;                                         /* 7 */
                break;
            }
        }
        thisoct++;                                              /* 8 */
    }
    if((*hs=(double *)realloc((char *)(*hs),*hsnotecnt * sizeof(double)))==NULL){
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic set array.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/**************************** CHEKREPEAT *********************************
 *
 * Flag if there is a repeated note in the source data.
 */

int chekrepeat(noteptr thisnote,double lastpitch)
{
    if(flteq((double)thisnote->pitch,lastpitch))
        return(TRUE);
    return(FALSE);
}

/****************************** GETNEXTHF ***********************************
 *
 * Get the harmonic field pertaining to this time, + 'hfnotecnt'.
 *
 * NB 2: THIS FUNCTION ASSUMES IN WILL BE ACCESSED IN TIME-INCREASING ORDER.
 *
 * (1)  Current time preset to zero.
 * (2)  Nexttime to time of next hf in hf_data_motif.
 * (3)  Static pointer points into current place in hf_data_motif.
 * (3a) If noteptr points to zero w'ere
        EITHER  we're at end of notelist: therefore no more HF data,
                hence retain existing data.
        OR      we have not initialised the noteptr, which should happen
                on first call to this function, BUT in this case the
                hfnotecnt will be zero as we have not yet created an HF.
                In this case, error, exit.
                * (4)  If time of function access still less than time of entry of next
                *      harmonic field, do nothing (i.e. stay with current hf returning
                *      current hfnotecnt).
                * (5)  If time of function access is after time of next harmonic field
                *      advance down hfs, until TOFA is before next hf (NB last time in
                *      the hft[] is AFTER end of texture duration).
                * (6)  Establish temporary storage space to read a new hf, and set the
                *      hf pointer 'thishf' to point to it.
                * (7)  Walk through the input notes until we get to notes corresponding
                *      to time of the hf we want.
                * (8)  Store the harmonic field in the temporary array.
                * (9)  Free any existing hf.
                * (10) Reallocate the new hf to the array hf[], and set the hf pointer
                *      'thishf' to point at it.
                * (11) Generate the associated HS !!!
                */

int getnexthf
(double **hf,double **hs, int *hsnotecnt,double inputtime,int *hf_cnt,int *hfnotecnt,
 noteptr *thishfnote,double *thishftime,double *nexthftime,int hfdatacnt,double *hft)
{
    int exit_status;
    int size = BIGARRAY;
    if(*thishfnote==(noteptr)0) {                                   /* 3a */
        if(*hfnotecnt==0) {
            sprintf(errstr,"INSUFFICIENT MEMORY for hfield notecount array.\n");
            return(PROGRAM_ERROR);
        }
        return(TRUE);
    }
    if(inputtime<*nexthftime)                                       /* 4 */
        return(FALSE);
    while(*nexthftime<=inputtime) {                         /* 5 */
        if(*hf_cnt>=hfdatacnt) {
            sprintf(errstr,"INSUFFICIENT MEMORY for hfield count array.\n");
            return(PROGRAM_ERROR);
        }
        *thishftime = *nexthftime;
        *nexthftime = hft[*hf_cnt];
        (*hf_cnt)++;
    }
    if(*hf!=(double *)0)                                            /* 9 */
        free(*hf);
    if((*hf = (double *)malloc(size * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for hfield array.\n");
        return(MEMORY_ERROR);
    }
    while(!flteq((double)(*thishfnote)->ntime,*thishftime)) {               /* 7 */
        if((*thishfnote = (*thishfnote)->next)==(noteptr)0) {
            sprintf(errstr,"TEXTURE: Search problem in getnexthf()\n");
            return(PROGRAM_ERROR);
        }
    }
    *hfnotecnt = 0;
    while(flteq((double)(*thishfnote)->ntime,*thishftime)) {                /* 8 */
        if((exit_status = gethfnote((*thishfnote)->pitch,*hf,hfnotecnt))<0)
            return(exit_status);
        if(*hfnotecnt >= size) {
            size += BIGARRAY;
            if((*hf = (double *)realloc((char *)(*hf),size * sizeof(double)))==NULL) {
                sprintf(errstr,"TEXTURE: realloc failed in getnexthf()\n");
                return(PROGRAM_ERROR);
            }
        }
        if((*thishfnote = (*thishfnote)->next)==(noteptr)0)
            break;
    }
    if((*hf=(double *)realloc((char *)(*hf),*hfnotecnt * sizeof(double)))==NULL){
        sprintf(errstr,"TEXTURE: getnexthf(): realloc() failed\n");     /* 10 */
        return(PROGRAM_ERROR);
    }
    upsort(*hf,*hfnotecnt);
    if((exit_status = gen_hs(hf,hs,hsnotecnt,*hfnotecnt))<0)  /* 11 */
        return(exit_status);
    return(TRUE);
}

/****************************** GETNEXTHS ***********************************
 *
 * Get the harmonic set pertaining to this time.
 *
 * NB 2: THIS FUNCTION ASSUMES IT WILL BE ACCESSED IN TIME-INCREASING ORDER.
 *
 * (3)  Static pointer points into current place in hf_data_motif.
 * (3a) If noteptr points to zero w'ere
        EITHER  we're at end of notelist: therefore no more HF data,
                hence retain existing data.
        OR      we have not initialised the noteptr, which should happen
                on first call to this function, BUT in this case the
                hsnotecnt will be zero as we have not yet created an HS.
                In this case, error, exit.
                * (4)  If time of function access still less than time of entry of next
                *      harmonic set, do nothing (i.e. stay with current hs returning
                *      current harmonic set cnt hsnotecnt).
                * (5)  If time of function access is after time of next harmonic set
                *      advance down HSs, until TOFA is before next hs (NB last time in
                *      the hft[] is AFTER end of texture duration).
                * (6)  Establish temporary storage space to read a new hs, and set the
                *      hs pointer 'thishs' to point to it.
                * (7)  Walk through the input notes until we get to notes corresponding
                *      to time of the hs we want.
                * (8)  Store the harmonic set in the temporary array.
                * (9)  Free any existing hs.
                * (10) Reallocate the new hs to the array hs[], and set the hs pointer
                *      'thishs' to point at it.
                */

int getnexths
(double **hs,double inputtime,int *hs_cnt,int *hsnotecnt,noteptr *thishsnote,
 double *thishstime,double *nexthstime,int hfdatacnt,double *hft)
{
    int exit_status;
    int size = BIGARRAY;
    if(*thishsnote==(noteptr)0) {                           /* 3a */
        if(*hsnotecnt==0) {
            sprintf(errstr,"getnexths() not initialised\n");
            return(PROGRAM_ERROR);
        }
        return(TRUE);
    }
    if(inputtime<*nexthstime)                                       /* 4 */
        return(FALSE);
    while(*nexthstime<=inputtime) {                         /* 5 */
        if(*hs_cnt>=hfdatacnt) {
            sprintf(errstr,"Timing problem in getnexths()\n");
            return(PROGRAM_ERROR);
        }
        *thishstime = *nexthstime;
        *nexthstime = hft[*hs_cnt];
        (*hs_cnt)++;
    }
    if(*hs!=(double *)0)                                            /* 9 */
        free(*hs);
    if((*hs = (double *)malloc(size * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic set array.\n");
        return(MEMORY_ERROR);
    }

    while(!flteq((double)(*thishsnote)->ntime,*thishstime)) {                       /* 7 */
        if((*thishsnote = (*thishsnote)->next)==(noteptr)0) {
            sprintf(errstr,"TEXTURE: Search problem in getnexths()\n");
            return(PROGRAM_ERROR);
        }
    }
    *hsnotecnt = 0;
    while(flteq((double)(*thishsnote)->ntime,*thishstime)) {                        /* 8 */
        if((exit_status = gethsnote((*thishsnote)->pitch,*hs,hsnotecnt))<0)
            return(exit_status);
        if(*hsnotecnt >= size) {
            size += BIGARRAY;
            if((*hs =(double *)realloc((char *)(*hs),size * sizeof(double)))==NULL) {
                sprintf(errstr,"TEXTURE: realloc failed in getnexths()\n");
                return(PROGRAM_ERROR);
            }
        }
        if((*thishsnote = (*thishsnote)->next)==(noteptr)0)
            break;
    }
    if((*hs=(double *)realloc((char *)(*hs),*hsnotecnt * sizeof(double)))==NULL){
        sprintf(errstr,"TEXTURE: getnexths(): realloc() failed\n");/* 10 */
        return(PROGRAM_ERROR);
    }
    upsort(*hs,*hsnotecnt);
    return(TRUE);
}

/***************************** GETHSNOTE ************************************
 *
 * Read a note and store in harmonic SET,  IF not already there.
 */

int gethsnote(double thispitch,double *thishs,int *hsnotecnt)
{
    int n, OK = 0;
    for(n=0;n<*hsnotecnt;n++) {
        if(flteq(thispitch,thishs[n])) {
            OK = 1;
            break;
        }
    }
    if(!OK)
        thishs[*hsnotecnt] = (double)thispitch;
    (*hsnotecnt)++;
    return(FINISHED);
}

/***************************** READHFTIMES ************************************
 *
 * Read times at which successive harmonic fields(sets) enter, and store as list.
 */

int readhftimes(noteptr firstnote, int *hfdatacnt,double **hft)
{
    double lasttime, firsttime;
    int arraysize = BIGARRAY;
    noteptr thisnote = firstnote;
    *hfdatacnt = 0;
    if((*hft = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic field times array.\n");
        return(MEMORY_ERROR);
    }
    lasttime = thisnote->ntime;
    (*hft)[0] = lasttime;
    while(thisnote!=(noteptr)0) {
        if(!flteq((double)thisnote->ntime,lasttime)) {
            if(++(*hfdatacnt)>=arraysize-1) {
                arraysize += BIGARRAY;
                if((*hft = (double *)realloc((char *)(*hft),arraysize * sizeof(double)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic field times array.\n");
                    return(PROGRAM_ERROR);
                }
            }
            (*hft)[*hfdatacnt] = thisnote->ntime;
            lasttime = thisnote->ntime;
        }
        thisnote = thisnote->next;
    }
    (*hft)[++(*hfdatacnt)] = DBL_MAX;                                       /* 1 */
    (*hfdatacnt)++;
    if((*hft = (double *)realloc((char *)(*hft),(*hfdatacnt) * sizeof(double)))==NULL){
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic field times array.\n");
        return(PROGRAM_ERROR);
    }
    if(!flteq(**hft,0.0)) {
        fprintf(stdout,"WARNING: Harmonic Field Data: No field at ZERO time.\n"
                "ADJUSTING FIRST FIELD TO ZERO TIME\n");
        fflush(stdout);
        thisnote  = firstnote;
        firsttime = thisnote->ntime;
        while(flteq(firsttime,**hft)) {
            thisnote->ntime = 0.0f;
            if((thisnote = thisnote->next)==(noteptr)0)
                break;
        }
        **hft = 0.0;
    }
    return(FINISHED);
}

/***************************** DEL_GHOSTS ******************************
 *
 * Delete all tset notes MARKED for deletion in shadbits bitflag.
 *
 * (1)  For each byte in the bitflag...
 * (2)  Set the bitmask to the first bit.
 *   (3)  For every bit in this byte....
 *   (4)  If this bit is set, delete the associated shadow (tset) note.
 *   (5)  Advance the bitflag internal to the byte leftwards.
 *   (6)  Advance the count of shadows, and if it reaches the total number
 *        of shadows, exit, because remaining bits in this byte (if any)
 *        have no meaning.
 */

void del_ghosts(int shshsize,char *shadbits,noteptr *shadow,int shadowsize,motifptr tset)
{
    int n,m,mask,k = 0;
    for(n=0;n<shshsize;n++) {                               /* 1 */
        mask = 1;                                                       /* 2 */
        for(m=0;m<CHARBITSIZE;m++) {            /* 3 */
            if(shadbits[n] & mask)                  /* 4 */
                del_note(shadow[k],tset);
            mask <<= 1;                                             /* 5 */
            if(++k >= shadowsize)                   /* 6 */
                return;
        }
    }
}

/************************** GET_HFGPPARAMS **************************
 *
 * Get the parameters for a group of notes.
 *
 * (0)  If pitch to decorate lies outside the HF range, return with gpsize
 *      set to zero.
 * (1)  If group range is expressed in HS-note units..
 *   (a) Read the gprange as an integer number of (HS) notes.
 *   (b) If HS-range exceeds HS limits, squeeze it.
 *   (c) If HS-range is to be oriented about note, orient it appropriately.
 * (2)  Otherwise...
 *   (a) Read group range in normal way.
 *   (b) If range exceeds HS limits, squeeze it.
 *   (c) If range is to be oriented about note, orient it appropriately.
 *   (d) If the range lies outside range of the HS,
 *       set the groupsize to ZERO and return.
 *   (e) Otherwise set the HS-range.
 * (3)  Read group size.
 * (4)  Read group density.
 * (5)  number of midicliks in a gpdense time-interval.
 * (6)  number of quantisation units, rounded.
 * (7)  readjust gpdense to be number of units * length of units,
 *      and reconvert to seconds.
 */

int get_hfgpparams(double thistime,double thispitch,double *hs,int hsnotecnt,
                   int *gpsize,int *hfrange,int *hfgpranglo,int *hfgpranghi,double *gprange,double *gpdense,int mingrpsize,
                   double *gprlow, int dectypecnt,unsigned char dectypestor,dataptr dz)
{
    int exit_status;
    if(thispitch<hs[0] || thispitch>hs[hsnotecnt-1]) {
        *gpsize = 0;                                            /* 0 */
        return(FINISHED);
    }
    if((exit_status = igetvalue(TEX_GPRANGHI,TEX_GPRANGLO,thistime,PM_GPRANG,hfrange,dz))<0)
        return(exit_status);

    if(!dectypecnt) {
        hfsqueezrange(thispitch,hfrange,hfgpranghi,hfgpranglo,hsnotecnt,hs);                                    /* b */
        dz->vflag[DECCENTRE] = FALSE;
    } else {
        if((exit_status = hforientrange
            (thispitch,hfrange,hfgpranghi,hfgpranglo,hsnotecnt,hs,dectypecnt,dectypestor,dz))<0) /* c */
            return(exit_status);
    }
    /* OCT 17 : hfgpranghi,lo are max & min INDECES within the harmonic field */

    *hfrange = *hfgpranghi - *hfgpranglo;                   /* e */

    /* OCT 17 : FROM hfgpranghi,lo the actual PITCH values can be deduced */
    *gprlow  = hs[*hfgpranglo];
    *gprange = hs[*hfgpranghi] - hs[*hfgpranglo];

    if((exit_status  = igetvalue(TEX_GPSIZEHI,TEX_GPSIZELO,thistime,PM_GPSIZE,gpsize,dz))<0)
        return(exit_status);

    if(*gpsize  < mingrpsize) {
        sprintf(errstr,"TEXTURE: Impossible GROUP SIZE value [%d]: get_hfgpparams()\n",*gpsize);
        return(PROGRAM_ERROR);
    }

    if((exit_status = getvalue(TEX_GPPACKHI,TEX_GPPACKLO,thistime,PM_GPDENS,gpdense,dz))<0)
        return(exit_status);
    /* NEW MAR 2000 */
    *gpdense *= MS_TO_SECS;

    if(dz->param[TEX_PHGRID]>0.0)
        *gpdense = quantise(*gpdense,dz->param[TEX_PHGRID]);
    return(FINISHED);
}

/***************************  HFSQUEEZRANGE ******************************
 *
 * Adjust range to lie within HF limits.
 *
 * (2)  Top of range is at current pitch's HF index (approx). If this is
 *      above current top-of-HF range (hsnotecnt) move top down.
 * (3)  Bottom of range is igprange below top. If this is below
 *      bottom-of-HF (0), move igprlow up.
 * (4)  Recalculate the true igprange within true limits.
 */

void hfsqueezrange(double thispitch,int *hfrange,int *hfgpranghi,int *hfgpranglo,int hsnotecnt,double *hs)
{
    int halfrange = *hfrange/2;
    if(halfrange * 2 != *hfrange)
        halfrange++;
    *hfgpranghi = min((geths_hibnd(thispitch,hsnotecnt,hs)+halfrange),hsnotecnt);   /* 2 */
    *hfgpranglo = max((*hfgpranghi - *hfrange),0);                                                  /* 3 */
    *hfrange = *hfgpranghi - *hfgpranglo;                                                                   /* 4 */
}

/****************************** GETHS_HIBND **********************************
 *
 *      hibnd >= pitch
 *
 * Find the index of the harmonic-set note that is immediately above or
 * equal to the input pitch.
 *
 * NB CALL WITH geths_hibnd((double)thisnote->pitch);
 *
 * If the pitch is beyond the HS, n returns the value 'hsnotecnt'.
 */

int geths_hibnd(double thispitch,int hsnotecnt,double *hs)
{
    int n;
    for(n=0;n<hsnotecnt;n++) {
        if(hs[n]>thispitch || flteq(hs[n],thispitch))
            break;
    }
    return(n);                              /* 1 */
}

/***************************  HFORIENTRANGE ******************************
 *
 * As orientrange() but applied to HF/HS case.
 */

int hforientrange
(double thispitch,int *hfrange,
 int *hfgpranghi,int *hfgpranglo,int hsnotecnt,double *hs,int dectypecnt,unsigned char dectypestor,dataptr dz)
{
    int exit_status;
    unsigned char dectype;
    int k;
    if((exit_status=doperm((int)dectypecnt,PM_ORIENT,&k,dz))<0)
        return(exit_status);
    if((exit_status=gettritype(k,dectypestor,&dectype))<0)
        return(exit_status);
    switch(dectype) {
    case(0):                        /* centred range */
        hfsqueezrange(thispitch,hfrange,hfgpranghi,hfgpranglo,hsnotecnt,hs);
        dz->vflag[DECCENTRE] = TRUE;
        break;
    case(1):                        /* range above note */
        *hfgpranghi = min((geths_hibnd(thispitch,hsnotecnt,hs) + *hfrange),hsnotecnt);
        *hfgpranglo = max((*hfgpranghi - *hfrange),0);
        *hfrange    = *hfgpranghi - *hfgpranglo;
        dz->vflag[DECCENTRE] = FALSE;
        break;
    case(2):                        /* range below note */
        *hfgpranghi = geths_hibnd(thispitch,hsnotecnt,hs);
        *hfgpranglo = max((*hfgpranghi - *hfrange),0);
        *hfrange    = *hfgpranghi - *hfgpranglo;
        dz->vflag[DECCENTRE] = FALSE;
        break;
    default:
        sprintf(errstr,"TEXTURE: Problem in hforientrange()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** GETHS_ABOVE **********************************
 *
 *      above > pitch
 *
 * Find the index of the harmonic-set note that is immediately above
 * the input pitch.
 *
 * NB CALL WITH geths_above((double)thisnote->pitch);
 *
 * If the pitch is beyond the HS, n gets the value 'hsnotecnt', which becomes
 * the upper limit of the search range for doperm() ensuring that highest
 * possible HSindex val is hsnotecnt-1, the highest pitch in the HS.
 */

int geths_above(double thispitch,double *hs,int hsnotecnt)
{
    int n;
    for(n=0;n<hsnotecnt;n++) {
        if(hs[n]>thispitch)
            break;
    }
    return(n);                                      /* 1 */
}

/****************************** GENERATE_GROUP_NOTE **********************************/

int generate_group_note
(noteptr thisnote,noteptr tsetnote,double thistime,int hfrange,int hfgpranglo,
 double *hs,int *hsindex,int gpsize,dataptr dz)
{
    int exit_status;
    unsigned char thisinstr;
    if((exit_status = do_grp_ins(tsetnote->instr,&thisinstr,dz))<0)
        return(exit_status);
    thisnote->instr = thisinstr;
    thisnote->motioncentre = tsetnote->motioncentre;
    if((exit_status = hfscat(hfrange,hfgpranglo,PM_GPPICH,hsindex,dz))<0)
        return(exit_status);
    thisnote->pitch  = (float)hs[*hsindex];         /* 7 */
    thisnote->ntime  = (float)thistime;
    return setspace(tsetnote,thisnote,gpsize,dz);
}

/******************************* HFSCAT ***********************************
 *
 * A weighted version of hfscatx().
 */

int hfscat(int prange,int pbottom,int permindex,int *val,dataptr dz)
{
    int exit_status;
    double bandbottom,  bandtop,  bandwidth;
    int   ibandbottom, ibandtop, ibandwidth, k;
    if(prange<=LAYERCNT) {
        if((exit_status = doperm((int)prange,permindex,&k,dz))<0)
            return(exit_status);
        *val = k + pbottom;
        return(FINISHED);

    }
    if((exit_status = doperm((int)BANDCNT,permindex,&k,dz))<0)
        return(exit_status);
    bandwidth = (double)prange/(double)LAYERCNT;
    switch(k) {
    case(0):
        bandbottom = 0;
        break;
    case(BANDCNT-1):
        bandbottom = (double)(LAYERCNT-1) * bandwidth;
        break;
    default:
        bandbottom = bandwidth + ((double)((k-1) * 2) * bandwidth);
        bandwidth *= 2.0;
        break;
    }
    bandtop     = bandbottom + bandwidth;
    ibandtop    = round(bandtop);
    ibandbottom = round(bandbottom);
    ibandwidth  = ibandtop - ibandbottom;
    k  = (int)(drand48() * (double)ibandwidth);
    k += ibandbottom;
    k += pbottom;
    *val = k;
    return(FINISHED);
}

/****************************** GENERATE_DECOR_NOTE **********************************/

int generate_decor_note(noteptr thisnote,noteptr tsetnote,
                        double thispitch,double thistime,int hfrange,int hfgpranghi,int hfgpranglo,double *hs,int *hsindex,
                        int hsnotecnt,double gprlow,double gprange,int gpsize,dataptr dz)
{
    int exit_status;
    double val;
    if((exit_status = do_grp_ins(tsetnote->instr,&thisnote->instr,dz))<0)
        return(exit_status);
    thisnote->motioncentre = tsetnote->motioncentre;
    if(dz->vflag[DECCENTRE]) {
        if((exit_status = dec_hfcentre
            (thispitch,hs,hsnotecnt,hfrange,hfgpranghi,hfgpranglo,gprlow,gprange,gpsize,&val,dz))<0)
            return(exit_status);
        thisnote->pitch = (float)val;
    } else {
        if((exit_status = hfscat(hfrange,hfgpranglo,PM_GPPICH,hsindex,dz))<0)
            return(exit_status);
        thisnote->pitch  = (float)hs[*hsindex];
    }
    thisnote->ntime  = (float)(thistime);
    return setspace(tsetnote,thisnote,gpsize,dz);
}

/************************* DEC_HFCENTRE *******************************/

int dec_hfcentre(double tsetpitch,double *hs,int hsnotecnt,int hfrange,int hfgpranghi,int hfgpranglo,
                 double gprlow,double gprange,int gpsize,double *val,dataptr dz)
{
    int exit_status;
    double a, gprhi = gprlow + gprange;
    int s, save, k, hfcentre, n_above, n_below, hsindex;
    if(tsetpitch-gprlow<=FLTERR || flteq(tsetpitch,gprhi)) {
        if((exit_status = hfscatx(hfrange,hfgpranglo,PM_GPPICH,&hsindex,dz))<0)
            return(exit_status);
        *val = hs[hsindex];
        return(FINISHED);                                       /* 1 */
    }
    n_below = hfrange/2;                                    /* 2 */
    if(((n_above=n_below)+n_below)!=hfrange)
        n_above++;
    if(((hfcentre =geths_hibnd(tsetpitch,hsnotecnt,hs)) + n_above) > hsnotecnt)
        n_above = hsnotecnt - hfcentre;                         /* 3 */
    if(hfcentre - n_below < 0)
        n_below = hfcentre;                                     /* 4 */
    if(n_above<=0 || n_below<=0) {
        if((exit_status = hfscatx(hfrange,hfgpranglo,PM_GPPICH,&hsindex,dz))<0)
            return(exit_status);
        else {
            *val = hs[hsindex];
            return(FINISHED);                                       /* 5 */
        }
    }
    save = dz->iparray[TXREPETCNT][PM_DECABV];                                              /* 6 */
    k = (gpsize+1)/2;
    if(dz->iparray[TXREPETCNT][PM_DECABV]>k)
        dz->iparray[TXREPETCNT][PM_DECABV] = k;
    if((exit_status = doperm((int)2,PM_DECABV,&s,dz))<0)
        return(exit_status);
    if(s) {
        if((exit_status = hfscatx(n_above,hfgpranghi - n_above,PM_GPPICH,&hsindex,dz))<0)
            return(exit_status);
    } else {
        if((exit_status = hfscatx(n_below,hfgpranglo,PM_GPPICH2,&hsindex,dz))<0)
            return(exit_status);
    }
    dz->iparray[TXREPETCNT][PM_DECABV] = save;
    a = hs[hsindex];
    if(a<MIDIBOT || a>MIDITOP) {
        sprintf(errstr,"TEXTURE: Problem in dec_hfcentre()\n");
        return(PROGRAM_ERROR);
    }
    *val = a;
    return(FINISHED);
}

/******************************* HFSCATX ***********************************
 *
 * As pscat, but working with ranges of integers, especially the indeces
 * of an HS.
 */

int hfscatx(int prange,int pbottom,int permindex,int *val,dataptr dz)
{
    int exit_status;
    double bandbottom,  bandtop,  bandwidth;
    int   ibandbottom, ibandtop, ibandwidth, k;
    if(prange<=BANDCNT) {
        if((exit_status = doperm((int)prange,permindex,val,dz))<0)
            return(exit_status);
        *val += pbottom;
        return(FINISHED);
    }
    if((exit_status = doperm((int)BANDCNT,permindex,val,dz))<0)
        return(exit_status);
    k = *val;
    bandwidth   = (double)prange/(double)BANDCNT;
    bandbottom  = (double)k     * bandwidth;
    bandtop     = (double)(k+1) * bandwidth;
    ibandbottom = round(bandbottom);
    ibandtop    = round(bandtop);
    ibandwidth  = ibandtop - ibandbottom;
    k  = (int)(drand48() * (double)ibandwidth);
    k += ibandbottom;
    *val = k + pbottom;
    return(FINISHED);
}

/******************************** SET_SHADBIT ****************************
 *
 * Set a bit in the shadow's bitfag.
 */

void set_shadbit(int k,char *shadbits)
{
    int bitunit  = k/CHARBITSIZE;
    int bitshift = k % 8;
    int mask = 1;
    mask <<= bitshift;
    shadbits[bitunit] |= mask;
}

/****************************** GETHS_LOBND **********************************
 *
 *      lobnd =< pitch
 *
 * Find the index of the harmonic-set note that is immediately below, or
 * equal to the input pitch.
 *
 * NB CALL WITH geths_lobnd((double)thisnote->pitch);
 *
 * NB: If the entered pitch is lower than the entire HS, function returns -1.
 */

int geths_lobnd(double thispitch,double *hs,int hsnotecnt)
{
    int n;
    for(n=0;n<hsnotecnt;n++) {
        if(hs[n]>thispitch)
            break;
    }
    return(--n);                                    /* 1 */
}

/***************************** GETHFNOTE ************************************
 *
 * Read a note and store in harmonic field IF not already there.
 */

int gethfnote(double thispitch,double *hf,int *hfnotecnt)
{
    int n, OK  = 0;
    float notetransp = (float)fmod(thispitch,SEMITONES_PER_OCTAVE);
    if(flteq((double)notetransp,0.0))
        notetransp = (float)(notetransp + SEMITONES_PER_OCTAVE);
    for(n=0;n<*hfnotecnt;n++) {
        if(flteq((double)notetransp,hf[n])) {
            OK = 1;
            break;
        }
    }
    if(!OK)
        hf[*hfnotecnt] = (double)notetransp;
    (*hfnotecnt)++;
    return(FINISHED);
}

/**************************** SETMTFPARAMS *******************************
 *
 * Establish easy parameters of motif or ornament.
 */

int setmtfparams
(noteptr thisnote,double thisamp,noteptr phrasenote,noteptr tsetnote,double ampdif,double notetime,
 int gpsize,double multiplier,dataptr dz)
{
    int exit_status;
    if((exit_status = do_mtf_params(thisnote,thisamp,phrasenote,tsetnote,ampdif,notetime,multiplier,dz))<0)
        return(exit_status);
    return setspace(tsetnote,thisnote,gpsize,dz);
}

/**************************** INITIALISE_HFPHRASES *******************************/

int initialise_hfphrases
(int ***hfphrase,int **endhsindex,int **phrnotecnt,double **phraseamp,double **phrange,
 noteptr **phrlastnote,dataptr dz)
{
    if((*hfphrase = (int **)malloc(dz->tex->phrasecnt * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic field phrases.\n");
        return(MEMORY_ERROR);
    }
    if((*endhsindex = (int *)malloc(dz->tex->phrasecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic set indices.\n");
        return(MEMORY_ERROR);
    }
    if((*phrnotecnt = (int *)malloc(dz->tex->phrasecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic field phrase notecnt.\n");
        return(MEMORY_ERROR);
    }
    if((*phraseamp = (double *)malloc(dz->tex->phrasecnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for phrase amplitude array.\n");
        return(MEMORY_ERROR);
    }
    if((*phrange  = (double *)malloc(dz->tex->phrasecnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for phrase range array.\n");
        return(MEMORY_ERROR);
    }
    if((*phrlastnote = (noteptr *)malloc(dz->tex->phrasecnt * sizeof(noteptr)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for phrase lastnote array.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/****************************** GET_FIRST_HS ***********************************
 *
 * Extract HS from input data.
 */

int get_first_hs(noteptr thisnote,double **hs,int *hsnotecnt)
{
    int exit_status;
    int size = BIGARRAY;
    noteptr startnote = thisnote;
    double thistime = 0.0;
    if((*hs = (double *)malloc(BIGARRAY * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic set array.\n");
        return(MEMORY_ERROR);
    }
    while(thisnote!=(noteptr)0) {
        if((exit_status = gethsnote(thisnote->pitch,*hs,hsnotecnt))<0)
            return(exit_status);
        if(thisnote==startnote)
            thistime = thisnote->ntime;
        else {
            if(!flteq((double)thisnote->ntime,thistime)) {
                (*hsnotecnt)--;
                break;
            }
        }
        if(*hsnotecnt >= BIGARRAY) {
            size += BIGARRAY;
            if((*hs=(double *)realloc((char *)(*hs),size * sizeof(double)))==NULL){
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic set array.\n");
                return(MEMORY_ERROR);
            }
        }
        thisnote = thisnote->next;
    }
    if((*hs=(double *)realloc((char *)(*hs),*hsnotecnt*sizeof(double)))==NULL){
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic set array.\n");
        return(MEMORY_ERROR);
    }
    upsort(*hs,*hsnotecnt);
    return(FINISHED);
}

/****************************** GET_FIRST_HF ***********************************
 *
 * Extract first HF from input data.
 */

int get_first_hf(noteptr thisnote,double **hf,int *hfnotecnt)
{
    int exit_status;
    int size = BIGARRAY;
    noteptr startnote = thisnote;
    double thistime = 0.0;

    if((*hf = (double *)malloc(size * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for harmonic field array.\n");
        return(MEMORY_ERROR);
    }
    while(thisnote!=(noteptr)0) {
        if((exit_status = gethfnote(thisnote->pitch,*hf,hfnotecnt))<0)
            return(exit_status);
        if(thisnote==startnote)
            thistime = thisnote->ntime;
        else {
            if(!flteq((double)thisnote->ntime,thistime)) {
                (*hfnotecnt)--;
                break;
            }
        }
        if(*hfnotecnt >= size) {
            size += BIGARRAY;
            if((*hf = (double *)realloc((char *)(*hf),size * sizeof(double)))==NULL){
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic field array.\n");
                return(MEMORY_ERROR);
            }
        }
        thisnote = thisnote->next;
    }
    if((*hf=(double *)realloc((char *)(*hf),*hfnotecnt * sizeof(double)))==NULL){
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate harmonic field array.\n");
        return(MEMORY_ERROR);
    }
    upsort(*hf,*hfnotecnt);
    return(FINISHED);
}

/******************************** OCTEQ *************************************/

int octeq(double a,double b)
{
    a = fmod(a,12.0);
    b = fmod(b,12.0);
    if(flteq(a,b)) {
        return(1);
    }
    return(0);
}
