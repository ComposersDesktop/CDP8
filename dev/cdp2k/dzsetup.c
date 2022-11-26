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
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <cdpmain.h>

#include <formants.h>
#include <speccon.h>
#include <modicon.h>
#include <txtucon.h>

#include <memory.h>         /*RWD*/
#include <string.h>
#include <sfsys.h>

static int  establish_infile_and_outfile_property_structs(dataptr dz);
static void pointer_initialisation(dataptr dz);
static void initialise_file_property_struct(fileptr fp);
static void free_tex_structure(dataptr dz);
static void delete_motifs_here_and_beyond(motifptr startmtf);
static void free_application_structure_and_related_brktables(dataptr dz);
static void nullify_duplicate_wordptrs(dataptr dz);





/**************************** ESTABLISH_DATASTRUCTURE ************************/

int establish_datastructure(dataptr *dz)
{
    int exit_status;
    if((*dz = (dataptr)malloc(sizeof(struct datalist)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for main data structure\n");
        return(MEMORY_ERROR);
    }
    pointer_initialisation(*dz);
    if((exit_status = establish_infile_and_outfile_property_structs(*dz))<0)
        return(exit_status);
    return(FINISHED);
}

/******************** ESTABLISH_INFILE_AND_OUTFILE_PROPERTY_STRUCTS *************************/

int establish_infile_and_outfile_property_structs(dataptr dz)
{
    if((dz->infile = (fileptr)malloc(sizeof(struct fileprops)))==NULL) {
        sprintf(errstr,"INSUFICIENT MEMORY for infile_property_struct\n");
        return(MEMORY_ERROR);
    }
    if((dz->otherfile = (fileptr)malloc(sizeof(struct fileprops)))==NULL) {
        sprintf(errstr,"INSUFICIENT MEMORY for otherfile_property_struct\n");
        return(MEMORY_ERROR);
    }
    if((dz->outfile = (fileptr)malloc(sizeof(struct fileprops)))==NULL) {
        sprintf(errstr,"INSUFICIENT MEMORY for outfile_property_struct\n");
        return(MEMORY_ERROR);
    }
    initialise_file_property_struct(dz->infile);
    initialise_file_property_struct(dz->otherfile);
    initialise_file_property_struct(dz->outfile);
    /*RWD.7.98 */
    if((dz->outfilename = (char *) malloc(_MAX_PATH)) ==NULL){
        sprintf(errstr,"INSUFFICIENT MEMORY for outfile name\n");
        return MEMORY_ERROR;
    }
    return(FINISHED);
}

/******************************** POINTER_INITIALISATION *******************************/

void pointer_initialisation(dataptr dz)
{
    //RWD.10.98
    char *clipstring = NULL;

    memset((char *)dz,0,sizeof(struct datalist));   /* THIS INITIALISES EVERYTHING ELSE TO ZERO!!! */
    dz->process             = -1;
    dz->maxmode             = -1;
    dz->mode                = -1;
/* PROCESS TYPES */
    dz->input_data_type     = -2;
    dz->outfiletype         = -2;
    dz->process_type        = -2;
/* OTHER FILE PONTERS */
    dz->ofd                 = -1;
    dz->other_file          = -1;
    dz->extra_word          = -1;
    dz->extrabrkno          = -1;
    dz->deal_with_chan_data = RECTIFY_CHANNEL_FRQ_ORDER;
    dz->is_sharp            = EIGHTH_TONE;      /* default */
    dz->is_flat             = 1.0/EIGHTH_TONE;  /* default */
    /*RWD.7.98 */
    dz->outfilename         = NULL;
    //RWD.10.98
    dz->floatsam_output     = 0;
    dz->peak_fval           = 1.0;      //only report if overrange
    dz->true_outfile_stype  = SAMP_SHORT;
    dz->clip_floatsams      = 1;
    clipstring = getenv("CDP_NOCLIP_FLOATS");
    if(clipstring != NULL)
        if((strcmp(clipstring,"") != 0) && (atoi(clipstring) != 0))
            dz->clip_floatsams  = 0;
    dz->outpeaks = dz->otherpeaks = NULL;
    dz->outpeakpos = dz->otherpeakpos = NULL;
    dz->outchans = dz->otheroutchans = 0;
    dz->needpeaks = dz->needotherpeaks = 0;
}

/******************************** INITIALISE_FILE_PROPERTY_STRUCT *************************/

void initialise_file_property_struct(fileptr fp)
{
    fp->filetype    = -1;
    fp->srate       = 0;
    fp->stype       = -1;
    fp->origstype   = -1;
    fp->origrate    = 0;
    fp->channels    = 0;
    fp->origchans   = 0;
    fp->specenvcnt  = 0;
    fp->Mlen        = 0;
    fp->Dfac        = 0;
    fp->arate       = 0.0f;
    fp->window_size = 0.0f;
}

/******************************* SUPERFREE ******************************/

int superfree(dataptr dz)
{
    int n;
    if(dz!=NULL) {
        if(dz->tex != NULL)
            free_tex_structure(dz);

        if(dz->infile!=NULL)        free(dz->infile);       if(dz->outfile!=NULL)       free(dz->outfile);  
        if(dz->fp!=NULL)            fclose(dz->fp);
        if(dz->is_int!=NULL)        free(dz->is_int);       if(dz->no_brk!=NULL)        free(dz->no_brk);  
        if(dz->is_active!=NULL)     free(dz->is_active);    
        if(dz->bigbuf!=NULL)        free(dz->bigbuf);       if(dz->bigfbuf!=NULL)       free(dz->bigfbuf);  
        if(dz->amp!=NULL)           free(dz->amp);          if(dz->freq!=NULL)          free(dz->freq);
        if(dz->sbufptr!=NULL)       free(dz->sbufptr);      if(dz->sampbuf!=NULL)       free(dz->sampbuf); 
        if(dz->flbufptr!=NULL)      free(dz->flbufptr);     if(dz->windowbuf!=NULL)     free(dz->windowbuf);
        if(dz->ifd!=NULL) {
            free(dz->ifd);
            dz->ifd = NULL;       /*RWD Nov 2011 jic */
        }
        if(dz->insams!=NULL)        free(dz->insams);

        if(dz->application!=NULL)
            free_application_structure_and_related_brktables(dz);

        if((dz->extrabrkno >= 0) && (dz->brk !=NULL) && (dz->brk[dz->extrabrkno] !=NULL)) 
            free(dz->brk[dz->extrabrkno]);

        if(dz->brk!=NULL)           free(dz->brk);          if(dz->brkptr!=NULL)        free(dz->brkptr);  
        if(dz->brksize!=NULL)       free(dz->brksize);  
        if(dz->lastind!=NULL)       free(dz->lastind);      if(dz->lastval!=NULL)       free(dz->lastval); 
        if(dz->brkinit!=NULL)       free(dz->brkinit);      if(dz->firstval!=NULL)      free(dz->firstval); 
        if(dz->param!=NULL)         free(dz->param);        if(dz->iparam!=NULL)        free(dz->iparam);
        if(dz->vflag!=NULL)         free(dz->vflag);

        if((dz->array_cnt > 0) && (dz->parray!=NULL)) {
            for(n = 0;n< dz->array_cnt; n++)  { 
                if(dz->parray[n]!=NULL)  
                    free(dz->parray[n]); 
            }
        }
        if((dz->iarray_cnt > 0) && (dz->iparray!=NULL)) {
            for(n = 0;n< dz->iarray_cnt; n++) { 
                if(dz->iparray[n]!=NULL) 
                    free(dz->iparray[n]);
            }
        }
        if((dz->larray_cnt > 0) && (dz->lparray!=NULL)) {
            for(n = 0;n< dz->larray_cnt; n++) { 
                if(dz->lparray[n]!=NULL) 
                    free(dz->lparray[n]);                           
            }
        }   
        /* RWD 4:2002 lfarray shadows lparray for sumbix syncatt and distort*/
        if((dz->larray_cnt > 0) && (dz->lfarray!=NULL)) {
            for(n = 0;n< dz->larray_cnt; n++) { 
                if(dz->lfarray[n]!=NULL) 
                    free(dz->lfarray[n]);                               
            }
        }

        if(dz->lfarray != NULL)
            free(dz->lfarray);


        if(dz->parray!=NULL)        free(dz->parray);       if(dz->iparray!=NULL) free(dz->iparray);
        if(dz->lparray!=NULL)       free(dz->lparray);      if(dz->sndbuf!=NULL) free(dz->sndbuf);
        if(dz->fptr!=NULL)          free(dz->fptr);         if(dz->ptr!=NULL)
            free(dz->ptr);
        if(dz->specenvfrq!=NULL)    free(dz->specenvfrq);   if(dz->specenvpch!=NULL) free(dz->specenvpch);
        if(dz->specenvamp!=NULL)    free(dz->specenvamp);   if(dz->specenvtop!=NULL) free(dz->specenvtop);
        if(dz->specenvamp2!=NULL)   free(dz->specenvamp2);  if(dz->pitches!=NULL) free(dz->pitches);
        if(dz->transpos!=NULL)      free(dz->transpos);     if(dz->pitches2!=NULL) free(dz->pitches2);
        if(dz->transpos2!=NULL)     free(dz->transpos2);    if(dz->frq_template!=NULL) free(dz->frq_template);
        if(dz->fsampbuf!=NULL)      free(dz->fsampbuf);     if(dz->filtpeak!=NULL) free(dz->filtpeak);
        if(dz->fbandtop!=NULL)      free(dz->fbandtop);     if(dz->fbandbot!=NULL) free(dz->fbandbot);
        if(dz->peakno!=NULL)        free(dz->peakno);       if(dz->lastpeakno!=NULL) free(dz->lastpeakno);
        if(dz->band!=NULL)          free(dz->band);         if(dz->temp!=NULL)
            free(dz->temp);
        if(dz->origenv!=NULL)       free(dz->origenv);      if(dz->env!=NULL)
            free(dz->env);
        if(dz->rampbrk!=NULL)       free(dz->rampbrk);

        if(dz->valstor!=NULL) {
            for(n=0;n<dz->linecnt;n++) {
                if(dz->valstor[n]!=NULL)
                    free(dz->valstor[n]);
            }
            free(dz->valstor);
        }
        if(dz->buflist!=NULL) {
            for(n=0;n<dz->bufcnt;n++) {
                if(dz->buflist[n]!=NULL)
                    free(dz->buflist[n]);
            }
            free(dz->buflist);
        }

        if(dz->wordstor!=NULL)
            free_wordstors(dz);

        if(dz->wordcnt!=NULL)       free(dz->wordcnt);      if(dz->act!=NULL)
            free(dz->act);
        if(dz->activebuf!=NULL)     free(dz->activebuf);    if(dz->activebuf_ptr!=NULL) free(dz->activebuf_ptr);


        if(dz->outpeaks)
            free(dz->outpeaks);
        if(dz->otherpeaks)
            free(dz->otherpeaks);
        if(dz->outpeakpos)
            free(dz->outpeakpos);
        if(dz->otherpeakpos)
            free(dz->otherpeakpos);

        free(dz);
    }
    return(FINISHED);
}

/**************************** FREE_TEX_STRUCTURE *******************************/

void free_tex_structure(dataptr dz)
{
    int n;
    if(dz->tex->motifhead !=NULL)
        delete_motifs_here_and_beyond(dz->tex->motifhead);
    if(dz->tex->insnd !=NULL) {
        for(n=0;n<dz->infilecnt;n++) {
            if(((dz->tex->insnd)[n])->buffer != NULL)
                free(((dz->tex->insnd)[n])->buffer);
            free((dz->tex->insnd)[n]);
        }
        free(dz->tex->insnd);
    }
    if(dz->tex->phrase !=NULL) 
        free(dz->tex->phrase);
    if(dz->tex->perm !=NULL) {
        for(n=0;n<PERMCNT;n++) {
            if(dz->tex->perm[n] != NULL)
                free(dz->tex->perm[n]);
        }
        free(dz->tex->perm);
    }
    free(dz->tex);
}

/********************** DELETE_MOTIFS_HERE_AND_BEYOND *******************************/

void delete_motifs_here_and_beyond(motifptr startmtf)
{
    motifptr thismotif = startmtf;
    if(thismotif==NULL)
        return;
    while(thismotif->next!=NULL)
        thismotif=thismotif->next;
    while(thismotif!=startmtf) {       
        delete_notes_here_and_beyond(thismotif->firstnote);
        thismotif=thismotif->last;
        free(thismotif->next);
    }
    if(startmtf->last!=NULL)
        startmtf->last->next = NULL;
    free(startmtf);
}

/********************** DELETE_NOTES_HERE_AND_BEYOND *******************************/

void delete_notes_here_and_beyond(noteptr startnote)
{
    noteptr thisnote = startnote;
    if(thisnote==NULL)
        return;
    while(thisnote->next!=NULL)
        thisnote=thisnote->next;
    while(thisnote!=startnote) {
        thisnote=thisnote->last;
        free(thisnote->next);
    }
    if(startnote->last!=NULL)
        startnote->last->next = NULL;
    free(startnote);
}

/********************** FREE_APPLICATION_STRUCTURE_AND_RELATED_BRKTABLES *******************************/

void free_application_structure_and_related_brktables(dataptr dz)
{
    int n;
    aplptr ap = dz->application;
    if((ap->total_input_param_cnt > 0) && (dz->brk!=NULL)) {
        for(n = 0;n< ap->total_input_param_cnt; n++) { 
            if(dz->brk[n]!=NULL) 
                free(dz->brk[n]); 
        }
    }
    if(ap->param_list!=NULL)            free(ap->param_list);     
    if(ap->option_list!=NULL)           free(ap->option_list);
    if(ap->variant_list!=NULL)          free(ap->variant_list);   
    if(ap->internal_param_list!=NULL)   free(ap->internal_param_list);
    if(ap->option_flags!=NULL)          free(ap->option_flags);   
    if(ap->variant_flags!=NULL)         free(ap->variant_flags);
    if(ap->lo!=NULL)                    free(ap->lo);             
    if(ap->hi!=NULL)                    free(ap->hi);
    free(dz->application);
}

/********************** FREE_WORDSTORS *******************************/

void free_wordstors(dataptr dz)
{
    int n;
    nullify_duplicate_wordptrs(dz);
    for(n=0;n<dz->all_words;n++) {
        if(dz->wordstor[n]!=NULL)
            free(dz->wordstor[n]);
    }
    free(dz->wordstor);
}

/***************************** NULLIFY_DUPLICATE_WORDPTRS **************************/

void nullify_duplicate_wordptrs(dataptr dz)
{
    int n, m;
    for(n=0;n<dz->all_words-1;n++) {
        if(dz->wordstor[n]!= NULL) {
            for(m=n+1;m<dz->all_words;m++) {
                if(dz->wordstor[m]!=NULL) {
                    if(dz->wordstor[m]==dz->wordstor[n])
                        dz->wordstor[m] = NULL;
                }
            }
        }
    }
}

