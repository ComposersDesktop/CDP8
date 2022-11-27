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
#include <memory.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <texture.h>
#include <ctype.h>
#include <sfsys.h>
#include <osbind.h>
#include <cdpmain.h>
#include <standalone.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

#define	SQUASH	  (2.0)	/* non-linearity factor */
#define MAXPOSCNT (512)	/* max no. of spatial positions between lspkrs */

static int  initialise_texture_structure(dataptr dz);
static int  set_up_and_fill_insample_buffers(insamptr **insound,dataptr dz);
static int  setup_texflag(texptr tex,dataptr dz);
static int  adjust_some_input_params(dataptr dz);
static int  preset_some_internal_texture_params(dataptr dz);
static int  install_unused_texture_flags(int *total_flags,int unused_flags,dataptr dz);
static int  install_the_internal_flags(int total_flags,int internal_flags,dataptr dz);
static int  get_the_notedata(texptr tex,dataptr dz);
static int  extend_timeset(dataptr dz);
static int  generate_timeset(dataptr dz);
static int  do_prespace(motifptr tset,dataptr dz);
static int  get_sample_pitches(FILE *fp,dataptr dz);
static int  get_motifs(FILE *fp,int *motifcnt,dataptr dz);
static int  motifchek(motifptr thismotif);
static void convert_cmdline_instrnos_to_internal_representation(dataptr dz);
static int  check_max_transpos_compatible_with_splicelen(dataptr dz);
static int  set_amptype_params(dataptr dz);
static int  set_decor_pitchposition_params(dataptr dz);
static void copy_note(noteptr thisnote,noteptr orignote);
static int  add_motif_to_end_of_motiflist(motifptr *new,dataptr dz);
static int  scatter_and_quantise_tset_times(double *lasttime,noteptr *thisnote,dataptr dz);
static int  pre_space(noteptr thisnote,dataptr dz);
static int  getpos(double thistime,double *position,dataptr dz);
static int  spread_and_set_cpos(double *position,double given_position,double spread,dataptr dz);
static int  chekrang(double *val,dataptr dz);
static int  read_a_note_from_notedata_file
				(noteptr thisnote,int noteno,int motifno,double *lasttime,dataptr dz);
static int  generate_tset_times(double *thistime,noteptr *thisnote,dataptr dz);
static int  un_link_note(noteptr thisnote);
static int  new_motif(motifptr *thismotif);
static int  unlink_last_motif(motifptr thismotif);
static void subtract_one_from_brkvals(int paramno,dataptr dz);
static int  bigscatter(noteptr *thisnote,double thistime,double timestep,double scatter,double *lasttime,dataptr dz);
static int  get_data_item(char *q,char **p,double *val);
static int  init_note(noteptr *thisnote);
static void put_znote(noteptr thisnote);
static int  assign_timeset_hfset_motifsets(dataptr dz);
static int  massage_params(dataptr dz);
static int  init_motifs(dataptr dz);
static int generate_outpositions(dataptr dz);

/************************* TEXTURE_PREPROCESS ****************************
 *
 * (1)	Establish the bitflag which characterises the texture process.
 * (2)	For consistency across all texture processes, all unused flags are mallocd.
 *		This ensures that the numbering of the INTERNAL flags is consistent across all applics.
 * (3)	Convert some input parameters to form used internally, and check some against insnd lengths.
 */

int texture_preprocess(dataptr dz)
{
	int exit_status;
	int total_flags, unused_flags, n;
	unsigned int texflag;

	initialise_random_sequence(IS_TEX_RSEED,TEXTURE_SEED,dz);
	if((exit_status = initialise_texture_structure(dz))<0)			
		return(exit_status);

 	if((exit_status = set_up_and_fill_insample_buffers(&(dz->tex->insnd),dz))<0)
		return(exit_status);
	
    if((exit_status = initperm(&(dz->tex->perm),dz))<0)				
		return(exit_status);

	if((exit_status = setup_texflag(dz->tex,dz))<0)						/* 1 */
		return(exit_status);

	texflag = dz->tex->txflag;

	if((exit_status = adjust_some_input_params(dz))<0)		 
		return(exit_status);
																		
 	if((exit_status = preset_some_internal_texture_params(dz))<0)		
		return(exit_status);											
																		/* 2 */
	unused_flags = TOTAL_POSSIBLE_USER_FLAGS - dz->application->vflag_cnt;

	if((exit_status = install_unused_texture_flags(&total_flags,unused_flags,dz))<0)
		return(exit_status);

	if((exit_status = install_the_internal_flags(total_flags,INTERNAL_FLAGS_CNT,dz))<0)
		return(exit_status);

	if((exit_status = get_the_notedata(dz->tex,dz))<0)				
		return(exit_status);

    if((exit_status = assign_timeset_hfset_motifsets(dz))<0)			
		return(exit_status);

	if((exit_status = massage_params(dz))<0)							/* 3 */
		return(exit_status);
    if(texflag & ORN_DEC_OR_TIMED) {									
		if((exit_status = extend_timeset(dz))<0)						/* 7 */
			return(exit_status);
	} else {
		if((exit_status = generate_timeset(dz))<0)						/* 8 */
			return(exit_status);
	}
	if((exit_status = do_prespace(dz->tex->timeset,dz))<0)
		return(exit_status);

	if(dz->process == SIMPLE_TEX && dz->vflag[CYCLIC_TEXFLAG]) {
		if(dz->infilecnt < 2)
			dz->vflag[CYCLIC_TEXFLAG] = 0;
		else if (dz->vflag[PERM_TEXFLAG]) {
			if((dz->peakno = (int *)malloc(dz->infilecnt * sizeof(int))) == NULL) {
				sprintf(errstr,"Insufficient memory for clyclic permutations of input files.\n");
				return(MEMORY_ERROR);
			}
			if((dz->lastpeakno = (int *)malloc(dz->infilecnt * sizeof(int))) == NULL) {
				sprintf(errstr,"Insufficient memory for cyclic permutations of input files.\n");
				return(MEMORY_ERROR);
			}
			for(n=0;n<dz->infilecnt;n++) {
				dz->peakno[n] = n;
				dz->lastpeakno[n] = n;
			}
		}
	}
	if(dz->vflag[4]) {
		if((exit_status = generate_outpositions(dz))<0)					/* 8 */
			return(exit_status);
	}	
	return(FINISHED);
}

/*********************** INITIALISE_TEXTURE_STRUCTURE ******************************/

int initialise_texture_structure(dataptr dz)
{
	int n;
	if((dz->tex = (texptr)malloc(sizeof(struct textural)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for texture structure.\n");
		return(MEMORY_ERROR);
	}

	dz->tex->txflag     = 0;
	dz->tex->motifhead  = (motifptr)0;
	dz->tex->insnd	    = (insamptr *)0;
	dz->tex->timeset    = (motifptr)0;
	dz->tex->hfldmotif  = (motifptr)0;
	dz->tex->phrase     = (motifptr *)0;
	dz->tex->perm 	    = (int **)0;
	dz->tex->dectypstor = 0;
	dz->tex->dectypcnt	= 0;
 	dz->tex->amptypstor = 0;
	dz->tex->amptypcnt  = 0;
	dz->tex->phrasecnt  = 0;
	dz->tex->ampdirectd = FALSE;

	if((dz->tex->insnd	= (insamptr *)malloc(dz->infilecnt * sizeof(insamptr)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for texture insound structure.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->infilecnt;n++) {
		dz->tex->insnd[n] = (insamptr)0;
		if((dz->tex->insnd[n]= (insamptr)malloc(sizeof(struct insample)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for texture insound[%d] structure.\n",n+1);
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}
 
/************************** SET_UP_AND_FILL_INSAMPLE_BUFFERS ****************************
 *
 * 1) Set buffer pointer to zero, until it has been malloced.
 * 	  This make freeing of tex structure possible.
 */

int set_up_and_fill_insample_buffers(insamptr **insound,dataptr dz)
{
	int n;
	int samps_read;
	int thisbufsize /*, seccnt*/;
	int wrap_around_samps = 1;
	for(n=0;n<dz->infilecnt;n++) {
		((*insound)[n])->buffer = /*(float *)0*/NULL;					/* 1 */
		thisbufsize = dz->insams[n];
		thisbufsize += wrap_around_samps; 
		if((((*insound)[n])->buffer  = (float *)malloc(thisbufsize * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for texture insound buffer pointer %d.\n",n+1);
			return(MEMORY_ERROR);
		}
		memset((char *)((*insound)[n])->buffer,0,thisbufsize * sizeof(float));
		if((samps_read = fgetfbufEx(((*insound)[n])->buffer,thisbufsize,dz->ifd[n],0)) < 0) {
			sprintf(errstr,"Can't read sndfile %d to buffer: set_up_and_fill_insample_buffers()\n",n+1);
			return(SYSTEM_ERROR);
		}
		if(samps_read != dz->insams[n]) {
			sprintf(errstr,"Error reading sndfile %d to buf: set_up_and_fill_insample_buffers()\n",n+1);
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/***************************** SETUP_TEXFLAG ****************************/

int setup_texflag(texptr tex,dataptr dz)
{
	tex->txflag = 0;
	switch(dz->process) {
	case(SIMPLE_TEX): break;
	case(GROUPS):	  tex->txflag |= IS_GROUPS;													    break;
	case(DECORATED):  tex->txflag |= IS_DECOR;														break;
	case(PREDECOR):	  tex->txflag |= IS_DECOR;	tex->txflag |= ISPRE_DECORORN;						break;
	case(POSTDECOR):  tex->txflag |= IS_DECOR;	tex->txflag |= ISPOST_DECORORN;					    break;
	case(ORNATE):	  tex->txflag |= IS_ORNATE;													    break;
	case(PREORNATE):  tex->txflag |= IS_ORNATE; tex->txflag |= ISPRE_DECORORN;						break;
	case(POSTORNATE): tex->txflag |= IS_ORNATE; tex->txflag |= ISPOST_DECORORN;					    break;
	case(MOTIFS):	  tex->txflag |= IS_MOTIFS;													    break;
	case(MOTIFSIN):	  tex->txflag |= IS_MOTIFS; tex->txflag |= MOTIF_IN_HF;						    break;
	case(TIMED):	  														tex->txflag |= ISTIMED; break;
	case(TGROUPS):	  tex->txflag |= IS_GROUPS;	  						  	tex->txflag |= ISTIMED; break;
	case(TMOTIFS):	  tex->txflag |= IS_MOTIFS;	  						  	tex->txflag |= ISTIMED; break;
	case(TMOTIFSIN):  tex->txflag |= IS_MOTIFS; tex->txflag |= MOTIF_IN_HF; tex->txflag |= ISTIMED; break;
	default:
		sprintf(errstr,"Unknown process in setup_texflag()\n");
		return(PROGRAM_ERROR);
	}
	switch(dz->mode) {
	case(TEX_NEUTRAL):	break;
	case(TEX_HFIELD):	tex->txflag |= ISHARM;															break;
	case(TEX_HFIELDS):	tex->txflag |= ISHARM; 							tex->txflag |= ISMANY_HFLDS;	break;
	case(TEX_HSET):		tex->txflag |= ISHARM;	tex->txflag |= IS_HS;									break;
	case(TEX_HSETS):	tex->txflag |= ISHARM;	tex->txflag |= IS_HS;	tex->txflag |= ISMANY_HFLDS;	break;
	default:
		sprintf(errstr,"Unknown mode in setup_texflag()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************** ADJUST_SOME_INPUT_PARAMS ****************************
 *
 * (1)	dz->vflag[WHICH_CHORDNOTE] internally carries all 3 vals (first,highest,every).
 *
 * (2)	Gprange, in cases with Hfields, is an integer count of HF-field notes to use.
 */

int adjust_some_input_params(dataptr dz)
{
	int texflag = dz->tex->txflag;
	if((texflag & IS_ORN_OR_DEC) && dz->vflag[FORCE_EVERY]==TRUE)
		dz->vflag[WHICH_CHORDNOTE] = DECOR_EVERY;						/* 1 */

	if((texflag & IS_DEC_OR_GRP) && (texflag & ISHARM)) {				/* 2 */
		dz->is_int[TEX_GPRANGLO] = TRUE;
		dz->is_int[TEX_GPRANGHI] = TRUE;
		dz->iparam[TEX_GPRANGLO] = round(dz->param[TEX_GPRANGLO]);
		dz->iparam[TEX_GPRANGHI] = round(dz->param[TEX_GPRANGHI]);
	}
	return(FINISHED);
}

/***************************** PRESET_SOME_INTERNAL_TEXTURE_PARAMS ****************************/

int preset_some_internal_texture_params(dataptr dz)
{	
	int cnt = 0;
	dz->iparam[SPINIT]    = 0;	 cnt++;	/* ALL CLUMPS */
	dz->iparam[SPCNT]     = 0;	 cnt++;	
	dz->iparam[DIRECTION] = 0;	 cnt++;	
	dz->param[CPOS]       = 0.5; cnt++; /* for safety only */	/* SPATIALISATION */
	dz->param[TPOSITION]  = 0.5; cnt++; /* for safety only */
	dz->param[THISSPRANGE]= 1.0; cnt++; /* full range */
	/*dz->iparam[TEX_MAXOUT]= 0.0;   cnt++; *//* min value */
	dz->param[TEX_MAXOUT] = 0.0;   cnt++;		  /*RWD*/
	if(cnt != dz->application->internal_param_cnt) {
		sprintf(errstr,"preset_some_internal_texture_params() has false count\n");
		return(PROGRAM_ERROR);
	}
	dz->itemcnt = 0;
	return(FINISHED);
}

/***************************** INSTALL_UNUSED_TEXTURE_FLAGS ****************************/

int install_unused_texture_flags(int *total_flags,int unused_flags,dataptr dz)
{
	int n;
	*total_flags = dz->application->vflag_cnt + unused_flags;
	if(unused_flags > 0) {
		if((dz->vflag  = 
			(char *)realloc(dz->vflag,(*total_flags) * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal flags.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<unused_flags;n++) {
			switch(n) {
			case(4):	dz->vflag[2] = FALSE;	break;
			case(3):	dz->vflag[3] = FALSE; 	break;
			case(2):	dz->vflag[4] = FALSE; 	break;
			case(1):	dz->vflag[5] = FALSE; 	break;
			case(0):	dz->vflag[6] = FALSE;	break;
			}
		}
	}
	return(FINISHED);
}

/***************************** INSTALL_THE_INTERNAL_FLAGS ****************************/

int install_the_internal_flags(int total_flags,int internal_flags,dataptr dz)
{
		/* THESE FLAGS ARE NOT AVAILABLE TO USER : but they are used in the code */
	total_flags += internal_flags;
	if((dz->vflag  = 
		(char *)realloc(dz->vflag,total_flags * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to reallocate internal flags.\n");
		return(MEMORY_ERROR);
	}
	dz->iparam[DECCENTRE] = FALSE;
	return(FINISHED);
}

/***************************** GET_THE_NOTEDATA ****************************/

int get_the_notedata(texptr tex,dataptr dz)
{
	int exit_status;
	unsigned int texflag = tex->txflag;
	int motifcnt, expected_motifcnt = 0;

	if(dz->fp==NULL) {
		sprintf(errstr,"notedata fileptr not initialised: get_the_notedata()\n");
		return(PROGRAM_ERROR);
	}
	if((exit_status = get_sample_pitches(dz->fp,dz))<0) {
		sprintf(errstr,"Insufficient pitch values in notedata file.\n");
		return(DATA_ERROR);
	}
	if((exit_status = get_motifs(dz->fp,&motifcnt,dz))<0)
		return(exit_status);
	if(texflag & ORN_DEC_OR_TIMED)	expected_motifcnt++;	/* Line to work on */
	if(texflag & ISHARM)			expected_motifcnt++;	/* HF data		   */
	if(texflag & IS_ORN_OR_MTF)		expected_motifcnt++;	/* Ornmnts or mtfs */

	if(texflag & IS_ORN_OR_MTF)	{
		if(motifcnt < expected_motifcnt) {
			sprintf(errstr,"Insufficient motifs in notedata file.\n");
			return(DATA_ERROR);
		}
	} else {
		if(motifcnt!=expected_motifcnt) {
			sprintf(errstr,"Incorrect number [%d] of motifs in notedata file (expected %d).\n",
			motifcnt,expected_motifcnt);
			return(DATA_ERROR);
		}
	}
	return(FINISHED);
}

/**************************** EXTEND_TIMESET *******************************/

int extend_timeset(dataptr dz)
{
	int exit_status;
	int origcnt = 0, n;
	noteptr startnote = dz->tex->motifhead->firstnote;
	noteptr orignote, thisnote  = startnote;
	double *timediff;
	
	if(startnote==(noteptr)0) {
		sprintf(errstr,"Problem in note timings: extend_timeset()\n");
		return(PROGRAM_ERROR);
	}
	while(thisnote!=(noteptr)0) {
		if(thisnote->ntime > dz->param[TEXTURE_DUR]) {
			delete_notes_here_and_beyond(thisnote);
			return(FINISHED);
		}
		thisnote = thisnote->next;
		origcnt++;
	}

	if((timediff = (double *)malloc(origcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for timegaps array.\n");
		return(MEMORY_ERROR);
	}
	n = 1;
	thisnote = startnote;
	while(thisnote->next !=(noteptr)0) {
		thisnote = thisnote->next;
		timediff[n++] = thisnote->ntime - thisnote->last->ntime;
	}
	if(dz->brksize[TEXTURE_SKIP]) {
		if((exit_status = read_value_from_brktable(thisnote->ntime,TEXTURE_SKIP,dz))<0) {
			free(timediff);
			return(exit_status);
		}
	}
	timediff[0] = dz->param[TEXTURE_SKIP];
	n = 0;
	orignote = startnote;

	for(;;) {
		if(thisnote->ntime > dz->param[TEXTURE_DUR] && timediff[n] > 0.0)
			break;
		if((exit_status = make_new_note(&thisnote))<0) {
			free(timediff);
			return(exit_status);
		}
		copy_note(thisnote,orignote);
		thisnote->ntime = (float)(thisnote->last->ntime + timediff[n]);
		if(++n>=origcnt) {
			if(dz->brksize[TEXTURE_SKIP]) {
				if((exit_status = read_value_from_brktable(thisnote->ntime,TEXTURE_SKIP,dz))<0) {
					free(timediff);
					return(exit_status);
				}
				timediff[0] = dz->param[TEXTURE_SKIP];
			}
			n = 0;
			orignote = startnote;
		} else
			orignote = orignote->next;
	}
	dz->brksize[TEXTURE_SKIP] = 0;	/* prevent future reads */
	free(timediff);
	return(FINISHED);
}


/*********************************************************************
 *
 * (1)	Generating the tset (set of timed values on or around which the
 *		texture is to be generated).
*/
/************************** GENERATE_TIMESET ******************************
 *
 * (A)	Zero all other parameters besides time.
 */		

int generate_timeset(dataptr dz)
{
	int exit_status;
	double lasttime, thistime = 0.0;
	noteptr thisnote;
	motifptr tset;
	if((exit_status = add_motif_to_end_of_motiflist(&(dz->tex->timeset),dz))<0)
		return(exit_status);
	tset = 	dz->tex->timeset;
	if((thisnote = tset->firstnote)==(noteptr)0) {
		sprintf(errstr,"Failure to find 1st note in motif: generate_timeset()\n");
		return(PROGRAM_ERROR);
	}
	put_znote(thisnote);			/* A */
	thisnote->ntime = (float)thistime;
	if(dz->brksize[TEXTURE_PACK]) {
		if((exit_status = read_value_from_brktable(thistime,TEXTURE_PACK,dz))<0)
			return(exit_status);
	}		
	thistime += dz->param[TEXTURE_PACK];
	while(thistime<dz->param[TEXTURE_DUR]) {
		if((exit_status = generate_tset_times(&thistime,&thisnote,dz))<0)
			return(exit_status);
		put_znote(thisnote);				/* A */
	}
	thisnote = tset->firstnote;
	lasttime = thisnote->ntime;
	thisnote = thisnote->next;
	while(thisnote!=(noteptr)0) {
		if((exit_status = scatter_and_quantise_tset_times(&lasttime,&thisnote,dz))<0)
			return(exit_status);
	}
	dz->brksize[TEXTURE_PACK]  = 0;	/* set these brktables to appear empty, so no read attempts later */
	dz->brksize[TEXTURE_SCAT]  = 0;
	dz->brksize[TEXTURE_TGRID] = 0;
	return(FINISHED);
}

/***************************** DO_PRESPACE ******************************
 *
 * Spatialise the time set, prior to output phase&/or ornamentation.
 */

int do_prespace(motifptr tset,dataptr dz)
{
	int exit_status;
	noteptr thisnote = tset->firstnote;
	while(thisnote!=(noteptr)0) {
		if((exit_status = pre_space(thisnote,dz))<0)
			return(exit_status);
		thisnote = thisnote->next;
	}
	return(FINISHED);
}

/**************************** GET_SAMPLE_PITCHES *****************************/

int get_sample_pitches(FILE *fp,dataptr dz)
{
	int exit_status;
/* TW MARCH 2010: 2000--> 20000 */
	char temp[20000], *q;
	int pitchcnt = 0;
	double *p;
	int got_all_pitches = FALSE;
/* TW MARCH 2010: 2000 --> 20000 */
	while(!got_all_pitches && (fgets(temp,20000,fp)!=NULL)) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		p = &(((dz->tex->insnd)[pitchcnt])->pitch);
		while((exit_status = get_float_from_within_string(&q,p))==TRUE) {
			if(++pitchcnt >= dz->infilecnt) {
				got_all_pitches = TRUE;
				break;
			}
			p = &(((dz->tex->insnd)[pitchcnt])->pitch);
		}
		if(exit_status==FALSE)
			return(DATA_ERROR);
	}
	if(!got_all_pitches)
		return(DATA_ERROR);
	return(FINISHED);
}

/**************************** GET_MOTIFS *****************************
 * 
 * Read data from an ascii file to notelist.
 */

int get_motifs(FILE *fp,int *motifcnt,dataptr dz)
{
	int exit_status;
	motifptr thismotif;
	noteptr thisnote;
	char *p, temp[200];
	double lasttime=0.0;
	int datalen, noteno, motifno = 0;
    if((exit_status = init_motifs(dz))<0)		/* 8 */
		return(exit_status);
	thismotif = dz->tex->motifhead;
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p==ENDOFSTR)
			continue;
		if(*p!=TEXTURE_SEPARATOR) {
//TW UPDATE
			sprintf(errstr,"'%c' missing before datacount in notedata file: motif %d (or more notes listed than indicated by %cN)\n"
			"check datalen is correct\n",TEXTURE_SEPARATOR,motifno+1,TEXTURE_SEPARATOR);
			return(DATA_ERROR);
		}
		p++;
		if(!isdigit(*p) || sscanf(p,"%d",&datalen)!=1) {
			sprintf(errstr,"No datalength given: motif %d\n",motifno+1);
			return(DATA_ERROR);
		}
		if(datalen <= 0) {
			sprintf(errstr,"Invalid data length %d in notedata: motif %d\n",datalen,motifno+1);
			return(DATA_ERROR);
		}
		motifno++;
		thisnote = thismotif->firstnote;
		for(noteno=1;noteno<=datalen;noteno++) {
			if((exit_status = read_a_note_from_notedata_file(thisnote,noteno,motifno,&lasttime,dz))<0)
				return(exit_status);
			if(exit_status==CONTINUE) {
				noteno--;
				continue;
			}				
			if((exit_status = make_new_note(&thisnote))<0)
				return(exit_status);
		}
		if((exit_status = un_link_note(thisnote))<0)
			return(exit_status);
		if((exit_status = new_motif(&thismotif))<0)
			return(exit_status);
	}
	*motifcnt = motifno;
	if(motifno > 0)
		return unlink_last_motif(thismotif);
	return(FINISHED);
}

/************************** MOTIFCHEK ********************************
 *
 * Check we've not run out of motifs.
 */

int motifchek(motifptr thismotif)
{
	if(thismotif == (motifptr)0) {
		sprintf(errstr,"motifchek(): Insufficient motifs: even though correctly counted.\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/***************************** CONVERT_CMDLINE_INSTRNOS_TO_INTERNAL_REPRESENTATION ****************************/

void convert_cmdline_instrnos_to_internal_representation(dataptr dz)
{
	if(dz->brksize[TEXTURE_INSLO])
		subtract_one_from_brkvals(TEXTURE_INSLO,dz);
	else
		dz->iparam[TEXTURE_INSLO]--;

	if(dz->brksize[TEXTURE_INSHI])
		subtract_one_from_brkvals(TEXTURE_INSHI,dz);
	else
		dz->iparam[TEXTURE_INSHI]--;
}

/***************************** CHECK_MAX_TRANSPOS_COMPATIBLE_WITH_SPLICELEN ****************************/

int check_max_transpos_compatible_with_splicelen(dataptr dz)
{
	int n, exit_status;
	double max_maxpitch, max_minpitch, max_transpospitch, maxupshift, upratio, minlen;
	double min_sndlength = (TEXTURE_SPLICELEN + TEXTURE_SAFETY) * MS_TO_SECS * (double)dz->infile->srate;
	dz->frametime = TEXTURE_SPLICELEN;

	if(dz->brksize[TEXTURE_MAXPICH]) {
		if((exit_status = get_maxvalue_in_brktable(&max_maxpitch,TEXTURE_MAXPICH,dz))<0)
			return(exit_status);
	} else
		max_maxpitch = dz->param[TEXTURE_MAXPICH];

	if(dz->brksize[TEXTURE_MINPICH]) {
		if((exit_status = get_maxvalue_in_brktable(&max_minpitch,TEXTURE_MINPICH,dz))<0)
			return(exit_status);
	} else
		max_minpitch = dz->param[TEXTURE_MINPICH];

	max_transpospitch = max(max_minpitch,max_maxpitch);

	dz->zeroset = 0;	/* use as flag re splicelen change */
	for(n=0;n<dz->infilecnt;n++) {
		maxupshift = max_transpospitch - ((dz->tex->insnd)[n])->pitch;
		upratio	   = pow(2.0,(maxupshift/SEMITONES_PER_OCTAVE));
		minlen     = (double)dz->insams[n]/upratio;
		if(minlen < min_sndlength) {
			dz->frametime = 2.0;	
			min_sndlength = (dz->frametime + TEXTURE_SAFETY) * MS_TO_SECS * (double)dz->infile->srate;
			if(minlen < min_sndlength) {
				dz->frametime = 1.0;	
				min_sndlength = (dz->frametime + TEXTURE_SAFETY) * MS_TO_SECS * (double)dz->infile->srate;
				if(minlen < min_sndlength) {
					sprintf(errstr,"sndfile %d [%.3lf secs] too short for max upward transposition [ratio %.2lf].\n",
					n+1,(double)dz->insams[n]/(double)dz->infile->srate,upratio);
					return(DATA_ERROR);
				}
			}
		}
	}
	return(FINISHED);
}

/************************* SET_AMPTYPE_PARAMS *******************************/

int set_amptype_params(dataptr dz)
{   int mask = 1, n;
    dz->tex->amptypcnt  = 0;
    dz->tex->amptypstor = 0;
	switch(dz->iparam[TEX_AMPCONT]) {
	case(IS_MIXED):				 dz->tex->amptypstor |= 1;	dz->tex->amptypstor |= 2;							break;
	case(IS_CRESC):											dz->tex->amptypstor |= 2;							break;
	case(IS_FLAT):				 dz->tex->amptypstor |= 1;														break;
	case(IS_DECRESC):														   		  dz->tex->amptypstor |= 4;	break;
	case(IS_FLAT_AND_CRESC):	 dz->tex->amptypstor |= 1;	dz->tex->amptypstor |= 2;							break;
	case(IS_CRESC_AND_DECRESC):								dz->tex->amptypstor |= 2; dz->tex->amptypstor |= 4;	break;
	case(IS_FLAT_AND_DECRESC):	 dz->tex->amptypstor |= 1;					   	  	  dz->tex->amptypstor |= 4;	break;

	case(IS_DIRECTIONAL):		 dz->tex->ampdirectd = TRUE;	dz->tex->amptypcnt = 1;							break;
	case(IS_DIREC_OR_FLAT):		 dz->tex->ampdirectd = TRUE;	dz->tex->amptypcnt = 2;							break;
	default:
		sprintf(errstr,"Unknown case in set_amptype_params()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->tex->amptypstor > 0) {
    	for(n=0;n<3;n++) {
			if(mask & dz->tex->amptypstor)
		    	dz->tex->amptypcnt++;
			mask <<= 1;
    	}
	}
	return(FINISHED);
}

/************************* SET_DECOR_PITCHPOSITION_PARAMS ******************************/

int set_decor_pitchposition_params(dataptr dz)
{
	int n, mask = 1;
	dz->tex->dectypstor = 0;
	dz->tex->dectypcnt  = 0;
	switch(dz->iparam[TEX_DECPCENTRE]) {
	case(DEC_CENTRED): dz->tex->dectypstor |= 1; 														break;
	case(DEC_ABOVE):							 dz->tex->dectypstor |= 2; 								break;
	case(DEC_BELOW):														dz->tex->dectypstor |= 4;	break;
	case(DEC_C_A):	   dz->tex->dectypstor |= 1; dz->tex->dectypstor |= 2; 								break;
	case(DEC_C_B):	   dz->tex->dectypstor |= 1;							dz->tex->dectypstor |= 4;	break;
	case(DEC_A_B):								 dz->tex->dectypstor |= 2;	dz->tex->dectypstor |= 4;	break;
	case(DEC_C_A_B):   dz->tex->dectypstor |= 1; dz->tex->dectypstor |= 2;	dz->tex->dectypstor |= 4;	break;
	default:
		sprintf(errstr,"Unknown case in set_decor_pitchposition_params()\n");
		return(PROGRAM_ERROR);
	}
	if(dz->tex->dectypstor > 0) {
    	for(n=0;n<3;n++) {
			if(mask & dz->tex->dectypstor)
		    	dz->tex->dectypcnt++;
			mask <<= 1;
    	}
	}
	return(FINISHED);
}

/********************** DELETE_NOTES_HERE_AND_BEYOND *******************************

void delete_notes_here_and_beyond(noteptr startnote)
{
	noteptr here = startnote;
	if(here==(noteptr)0)
		return;
	while(here->next!=(noteptr)0)
		here=here->next;
	while(here!=startnote) {
		here=here->last;
		free(here->next);
	}
	if(startnote->last!=(noteptr)0)
		startnote->last->next = (noteptr)0;
	free(startnote);
}
****/

/********************** MAKE_NEW_NOTE ****************************
 *
 * Create new link in note list.
 */

int make_new_note(noteptr *thisnote)
{
    if(((*thisnote)->next = (noteptr)malloc(sizeof(struct nnote)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for a further note store.\n");
		return(MEMORY_ERROR);
	}
    (*thisnote)->next->last = *thisnote;
    *thisnote = (*thisnote)->next;
    (*thisnote)->next = (noteptr)0;
    return(FINISHED);
}

/**************************** COPY_NOTE *******************************/

void copy_note(noteptr thisnote,noteptr orignote)
{
	thisnote->ntime = orignote->ntime;
	thisnote->amp   = orignote->amp;
	thisnote->pitch = orignote->pitch;
	thisnote->dur   = orignote->dur;
	thisnote->instr = orignote->instr;
	thisnote->spacepos = orignote->spacepos;
	thisnote->motioncentre = orignote->motioncentre;
}

/************************** ADD_MOTIF_TO_END_OF_MOTIFLIST ******************************
 *
 * Create new motif at end of list.
 */

int add_motif_to_end_of_motiflist(motifptr *new,dataptr dz)
{
	int exit_status;
	motifptr here = dz->tex->motifhead;
	while(here->next!=(motifptr)0)
		here = here->next;
	if((exit_status =  new_motif(&here))<0)
		return(exit_status);
	*new = here;
	return(FINISHED);
}
    
/************************* SCATTER_AND_QUANTISE_TSET_TIMES ******************************/

int scatter_and_quantise_tset_times(double *lasttime,noteptr *thisnote,dataptr dz)
{
	int exit_status;
	double thistime = (*thisnote)->ntime;			/* 1 */
	double timestep = thistime - *lasttime;			/* 2 */
	*lasttime = thistime;							/* 2a */
	if(dz->brksize[TEXTURE_SCAT]) {
		if((exit_status = read_value_from_brktable(thistime,TEXTURE_SCAT,dz))<0)
			return(exit_status);
	}		
	if(dz->param[TEXTURE_SCAT]>1.0)	{				/* 3a */
//TW UPDATE (original code error)
		if((exit_status = bigscatter(thisnote,thistime,timestep,dz->param[TEXTURE_SCAT],lasttime,dz))<0)
			return(exit_status);
	} else {
		if(!flteq(dz->param[TEXTURE_SCAT],0.0)) 			/* 4 */
			thistime -= (float)(drand48() * dz->param[TEXTURE_SCAT] * timestep);
	
		if(dz->param[TEXTURE_TGRID]>0.0)
			thistime  = quantise(thistime,dz->param[TEXTURE_TGRID]);	/* 6 */
		(*thisnote)->ntime = (float)thistime;
	}
	*thisnote = (*thisnote)->next;
	return(FINISHED);				/* 8a */
}

/************************** PRE_SPACE ********************************
 *
 * Store spatial data in the tsetnote spacepos and motioncentre.
 */

int pre_space(noteptr thisnote,dataptr dz)
{
	int exit_status;
	double position;
	if((exit_status = getpos((double)thisnote->ntime,&position,dz))<0)
		return(exit_status);
	thisnote->spacepos     = (float)position;
	thisnote->motioncentre = (float)dz->param[CPOS];
	return(FINISHED);
}

/************************** GETPOS ****************************
 *
 * Calculate spatial position of event from it's time.
 */

int getpos(double thistime,double *position,dataptr dz)
{
	int exit_status;
	if(dz->brksize[TEXTURE_POS] && (exit_status = read_value_from_brktable(thistime,TEXTURE_POS,dz))<0)
		return(exit_status);
	if(dz->brksize[TEXTURE_SPRD] && (exit_status = read_value_from_brktable(thistime,TEXTURE_SPRD,dz))<0)
		return(exit_status);
	if(dz->param[TEXTURE_SPRD] > 0.0) {
		if((exit_status = spread_and_set_cpos(position,dz->param[TEXTURE_POS],dz->param[TEXTURE_SPRD],dz))<0)
			return(exit_status);
	} else {
/* NOV 1997--> */
		dz->param[CPOS] = dz->param[TEXTURE_POS];
/* <--NOV 1997 */
		*position       = dz->param[TEXTURE_POS];
	}
	return(FINISHED);
}

/***************************** SPREAD_AND_SET_CPOS *****************************/

int spread_and_set_cpos(double *position,double given_position,double spread,dataptr dz)
{
	int exit_status;
	//static int firsttime = 1;
	int	   posno, spswitch;
	int    poscnt,hposcnt;
	double hspread,spacing;
	double lchan;					/* leftmost channel of position value */
	double stereo_given_position;	/* position within stereo gap between 2 lspkrs (0-1) */

	/* total stereospace has MAXPOSCNT positions: calc no. & spacing of positions available at given spread */
	given_position -= 1.0;		/* input parma run from 1 to N, calc params run from 0 to (N-1) */
	if(given_position < 0.0)
		given_position += (double)dz->iparam[TEXTURE_OUTCHANS];
	dz->param[CPOS] = given_position;
	poscnt  = round(spread * (double)MAXPOSCNT);
	hposcnt = poscnt/2;
	hspread = spread/(double)2.0;
	if(hposcnt != 0)
		spacing = hspread/(double)hposcnt;
	else {
	/* bandwidth effectively zero: return given position */ 
		dz->param[CPOS] = given_position;
		*position       = given_position;
		return(FINISHED);
	}
	/* randomly select leftwards/rightwards of the given position */
	if((exit_status = doperm(2,PM_SPACE,&spswitch,dz))<0)
		return(exit_status);					
	if(!spswitch)
		spswitch = -1;								
	/* randomly chose a pos (to left or right) around centre pos */
	posno  = round(drand48() * hposcnt);				
	posno *= spswitch;								
	/* If (position+spread) leaks to right of stereo-space : squeeze position */
	lchan = (double)(int)round(given_position);
	stereo_given_position = given_position - lchan;
	
	dz->param[CPOS] = stereo_given_position;

	*position = dz->param[CPOS] + ((double)posno * spacing);
	*position += lchan;
	if(*position < 0.0)
		*position += (double)dz->iparam[TEXTURE_OUTCHANS];
	if(*position >= dz->iparam[TEXTURE_OUTCHANS])
		*position -= (double)dz->iparam[TEXTURE_OUTCHANS];
	return chekrang(position,dz);
}

/*************************** CHEKRANG **************************
 * 
 * Check variable lies within range 0.0 to 1.0.
 */

int chekrang(double *val,dataptr dz)
{
	if(flteq(*val,1.0))
		*val = 1.0;
	if(flteq(*val,0.0))
		*val = 0.0;
	if(*val<0.0 || *val>(double)dz->iparam[TEXTURE_OUTCHANS]) {						 
		sprintf(errstr,"value [%f] outside range 0-%d: chekrang()\n",*val,dz->iparam[TEXTURE_OUTCHANS]);
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/************************** READ_A_NOTE_FROM_NOTEDATA_FILE ********************************/

int read_a_note_from_notedata_file(noteptr thisnote,int noteno,int motifno,double *lasttime,dataptr dz)
{
	int exit_status;
	unsigned int texflag = dz->tex->txflag;
	char temp[200], *p, *q;
	double val;
	int instr_no, start;
	if(dz->fp==NULL) {
		sprintf(errstr,"Note datafile descriptor not initialised: read_a_note_from_notedata_file()\n");
		return(PROGRAM_ERROR);
	}
 	if(fgets(temp,200,dz->fp)==NULL){
		sprintf(errstr,"Note data line for note %d, motif %d missing in notedatafile\n",noteno,motifno);
		return(DATA_ERROR);
	}
	p =temp;
	while(isspace(*p))
		p++;
	q = p;
	if(*q==ENDOFSTR)
		return(CONTINUE);
	if((exit_status = get_data_item(q,&p,&val))<0) {
		sprintf(errstr,"No time data for note %d, motif %d in notedatafile\n",noteno,motifno); 
		return(DATA_ERROR);
	}
	if(exit_status==FINISHED) {
		sprintf(errstr,"No data after time for note %d, motif %d in notedatafile\n",noteno,motifno); 
		return(DATA_ERROR);
	}
	thisnote->ntime = (float)val;
	if((noteno > 1) && (*lasttime > thisnote->ntime)) {
		sprintf(errstr,"Notes in reverse time order: notedata file : motif %d: notes %d & %d\n",
		motifno,noteno,noteno-1);
		return(DATA_ERROR);
	}
	*lasttime = thisnote->ntime;
	p++;
	q = p;
	if((exit_status = get_data_item(q,&p,&val))<0) {
		sprintf(errstr,"No instr_no for note %d, motif %d in notedatafile\n",noteno,motifno); 
		return(DATA_ERROR);
	}
	if(exit_status==FINISHED) {
		sprintf(errstr,"No data after instr_no for note %d, motif %d in notedatafile\n",noteno,motifno); 
		return(DATA_ERROR);
	}
	instr_no = round(val);

	p++;
	q = p;
 	if((exit_status = get_data_item(q,&p,&val))<0) {
		sprintf(errstr,"No pitch data for note %d, motif %d in notedatafile\n",noteno,motifno); 
		return(DATA_ERROR);
	}
	if(exit_status==FINISHED) {
		sprintf(errstr,"No data after pitch for note %d, motif %d in notedatafile\n",noteno,motifno); 
		return(DATA_ERROR);
	}
	thisnote->pitch = (float)val;

/* NEEDS TESTING FOR MIDIRANGE: EXCEPT IN TIMING SET CASE */
	if(!(texflag & ISTIMED) || motifno!=1) {
		if(val < MIDIMIN || val > MIDIMAX) {
			sprintf(errstr,"pitch value [%lf] out of range (%d to %d): motif %d: note %d\n",
			val,MIDIMIN,MIDIMAX,motifno,noteno);
			return(DATA_ERROR);
		}
	}
	p++;
	q = p;
 	if((exit_status = get_data_item(q,&p,&val))<0) {
		sprintf(errstr,"No amplitude data for note %d, motif %d in notedatafile\n",noteno,motifno); 
		return(DATA_ERROR);
	}
	if(exit_status==FINISHED) {
		sprintf(errstr,"No data after amp for note %d, motif %d in notedatafile\n",noteno,motifno); 
		return(DATA_ERROR);
	}
	thisnote->amp = (float)val;
/* 1999 THIS NEEDS TESTING FOR MIDIRANGE EXCEPT IN TIMING SET CASE */
	if(!(texflag & ISTIMED) || motifno!=1) {
		if(val < MIDIMIN || val > MIDIMAX) {
			sprintf(errstr,"amplitude value [%lf] out of range (%d to %d): motif %d: note %d\n",
			val,MIDIMIN,MIDIMAX,motifno,noteno);
			return(DATA_ERROR);
		}
	}
	p++;
	q = p;
 	if((exit_status = get_data_item(q,&p,&val))<0) {
		sprintf(errstr,"No duration data for note %d, motif %d in notedatafile\n",noteno,motifno); 
		return(DATA_ERROR);
	}
	thisnote->dur = (float)val;
	if(texflag & IS_ORN_OR_MTF) {
		if(texflag & IS_ORNATE) {
			start = 2;
		} else {
			start = 1;
			if(texflag & ISTIMED)
				start++;
		}
		if(texflag & ISHARM)
			start++;
		if(motifno >= start) {
			if(val <= FLTERR) {
				sprintf(errstr,"Duration value [%lf] is too small: motif %d: note %d\n",val,motifno,noteno);
				return(DATA_ERROR);
			}
		}
	} else {
		thisnote->dur = 1.0f;	/* default ; redundant */
	}
	return(FINISHED);
}

/***************************** GENERATE_TSET_TIMES **********************************
 *
 * Store time initially as SECONDS.
 *
 * (1)	Create location for a new note.
 * (2)	Go to that new note.
 * (3)	Store the current time at this note.
 * (4)	Find note-density at this time, and generate time of next event.
 * (5)	Return nextnote.
 */

int generate_tset_times(double *thistime,noteptr *thisnote,dataptr dz)
{
	int exit_status;
	if((exit_status = make_new_note(thisnote))<0)
		return(exit_status);						/* 1 */
	(*thisnote)->ntime = (float)(*thistime);			/* 3 */
	if(dz->brksize[TEXTURE_PACK]) {
		if((exit_status = read_value_from_brktable(*thistime,TEXTURE_PACK,dz))<0)
			return(exit_status);
	}		
	*thistime  += dz->param[TEXTURE_PACK];			/* 4 */
	return(FINISHED);								/* 5 */
}

/********************** UN_LINK_NOTE ******************************
 *
 * Deletes empty address space at end of notelist.
 */

int un_link_note(noteptr thisnote)
{
	if(thisnote->last==(noteptr)0) {
		sprintf(errstr,"Problem in un_link_note()\n");
		return(PROGRAM_ERROR);
	}
	thisnote = thisnote->last;
	free(thisnote->next);
	thisnote->next = (noteptr)0;
	return(FINISHED);
}

/************************ NEW_MOTIF *****************************
 *
 * Set up next motif in a list of musical-motifs.
 * Set up location of first note of this motif.
 */

int new_motif(motifptr *thismotif)
{
	int exit_status;
	motifptr newmotif;
	noteptr  thisnote;
	//static int count = 1;
	if((newmotif = (motifptr)malloc(sizeof (struct motif)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store next motif.\n");
		return(MEMORY_ERROR);
	}
	(*thismotif)->next = newmotif;
	newmotif->last = *thismotif;
	newmotif->next = (motifptr)0;
	if((exit_status = init_note(&thisnote))<0)
		return(exit_status);
	thisnote->next = (noteptr)0;   
	thisnote->last = (noteptr)0;   
	newmotif->firstnote = thisnote;
	*thismotif = newmotif;
	return(FINISHED);
}

/********************** UNLINK_LAST_MOTIF ******************************
 *
 * Deletes empty address space at end of motiflist.
 */

int unlink_last_motif(motifptr thismotif)
{
	if(thismotif->last==(motifptr)0) {
		sprintf(errstr,"Problem in unlink_last_motif()\n");
		return(PROGRAM_ERROR);
	}
	thismotif = thismotif->last;
	free(thismotif->next);
	thismotif->next = (motifptr)0;
	return(FINISHED);
}

/**************************** SUBTRACT_ONE_FROM_BRKVALS ****************************/

void subtract_one_from_brkvals(int paramno,dataptr dz)
{
	double *p = dz->brk[paramno] + 1;
	double *pend = dz->brk[paramno] + (dz->brksize[paramno] * 2);
	while(p < pend) {
		*p -= 1.0;
		p += 2;
	}
}

/************************ BIGSCATTER *************************************
 *
 * (1)	The number of time-points to be randomly scattered as a group is
 *	scatcnt.
 * (1a)	Base time is the time from which these new scattered times are offset.
 * (2)	Save the current position in note list as 'here'.
 * (3)	Go forward in note list until there are scatcnt notes inclusive
 *	between here and there, or until list runs out.
 * (3a)	If the list runs out, modify scatcnt accordingly.
 * (4)	Note the (original) time of the last of these events. This will
 *	be returned as the new 'lasttime' for the next cycle of the
 *	program.
 * (5)	Make the time-step include the total duration of all these
 *	events.
 * (5a)	Generate a set of times AT RANDOM within this large timestep,
	sort them and store them.
 * (6)	Replace existing times by scattered times, relative to basetime.
 * (6a)	Convert to MIDitime.
 * (6b)	Quantise if necessary.
 * (7)	Replace the last time.
 * (9)	Return the last note we worked on.
 */

int bigscatter(noteptr *thisnote,double thistime,double timestep,double scatter,double *lasttime,dataptr dz)
{
	int n, scatcnt = round(scatter);		/* 1 */
	double *sct, *scti;
	double basetime = thistime - timestep;	/* 1a */
	noteptr here  = *thisnote;				/* 2 */
	noteptr there = *thisnote;				/* 2 */
	if((scti = (double *)malloc(scatcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for scattering structure.\n");
		return(MEMORY_ERROR);
	}
	sct = scti;
	for(n=0;n < scatcnt-1; n++) {			/* 3 */
		if(there->next==(noteptr)0) {
			scatcnt = n+1;					/* 3a */
			break;
		}
		there = there->next;
	}
	*lasttime = there->ntime;				/* 4 */
	timestep += *lasttime - thistime;		/* 5 */
	for(n= 0;n<scatcnt;n++)					/* 5a */
		*sct++ = drand48() * timestep;
	upsort(scti,scatcnt);					
	sct = scti;
	while(here!=there) {					/* 6 */
		thistime = basetime + (*sct++);
		if(dz->param[TEXTURE_TGRID]>0.0)
			thistime  = quantise(thistime,dz->param[TEXTURE_TGRID]);	/* 6b */
		here->ntime = (float)thistime;
		here = here->next;
	}
	thistime = basetime + *sct;				/* 7 */
	if(dz->param[TEXTURE_TGRID]>0.0)
		thistime  = quantise(thistime,dz->param[TEXTURE_TGRID]);
	there->ntime = (float)thistime;
	free(scti);
	*thisnote = there;
	return(FINISHED);					/* 9 */
}

/************************** QUANTISE *************************************
 *
 * Quantise time point onto a grid.
 */

double quantise(double thistime,double timegrid)
{
	int gridpoint;
	timegrid *= MS_TO_SECS;
	gridpoint = round(thistime/timegrid);
	return((double)gridpoint * timegrid);
}

/************************* GET_DATA_ITEM ****************************/

int get_data_item(char *q,char **p,double *val)
{
	int exit_status = CONTINUE;
	if(*q==ENDOFSTR)
		return(DATA_ERROR);
	while(isspace(**p)) {
		(*p)++;
		if(**p == ENDOFSTR)
			return(DATA_ERROR);
	}
	if(**p==TEXTURE_SEPARATOR)
		return(DATA_ERROR);
	while(!isspace(**p) && **p!=ENDOFSTR) {
		(*p)++;
	}
	if(**p==ENDOFSTR)
		exit_status = FINISHED;
	**p = ENDOFSTR;
	if(sscanf(q,"%lf",val)!=1)
		return(DATA_ERROR);
	return(exit_status);
}

/************************* INIT_NOTE ****************************
 *
 * Initialise notelist.
 */

int init_note(noteptr *thisnote)
{
	if((*thisnote = (noteptr)malloc(sizeof(struct nnote)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for another note.\n");
		return(MEMORY_ERROR);
	}
	(*thisnote)->next = (noteptr)0;
	(*thisnote)->last = (noteptr)0;
	return(FINISHED);
}

/********************** UPSORT *******************************
 * 
 * Sort set of doubles into ascending order.
 */

void upsort(double *scti,int scatcnt)
{
	double sct;
	int n, m;
	for(n=0;n<(scatcnt-1);n++) {
		for(m=n+1;m<scatcnt;m++) {
			if(*(scti+m)<*(scti+n)) {
				sct 	  = *(scti+m);
				*(scti+m) = *(scti+n);
				*(scti+n) = sct;
			}
		}
	}
}

/*********************** PUT_ZNOTE ********************************
 *
 * Put zero values in all parameters except time.
 */

void put_znote(noteptr thisnote)
{
	thisnote->amp   = 0.0f;
	thisnote->pitch  = 0.0f;
	thisnote->dur   = 0.0f;
	thisnote->instr = 0;
	thisnote->spacepos = 0.0f;
	thisnote->motioncentre = 0.0f;
}

/************************* ASSIGN_TIMESET_HFSET_MOTIFSETS ******************************
 *
 * Assign appropriate pointers to input motifs.
 *
 * (1)	If it's a timed texture, indicate first motif in list as that
 *		which defines the set of times (tset).
 * 		Similarly, if texture is ornamented or decorated, point to first
 *		motif as that to BE ornamented or decorated (tset).
 * (2)	If the texture restricted to HF(s) or HS(s), indicate next motif
 *		as the HF-set (hfldmotif). It may be read in different ways (as HF, as
 *		HS, or as time-varying of either type) at a later time.
 * (3)	If texture consists of motifs, or is to use specific ornaments...
 * (4)	count the number of motifs remaining.
 * (5)	Set up the array of phrases to point to the appropriate motifs.
 */

int assign_timeset_hfset_motifsets(dataptr dz)
{
	int exit_status;
	unsigned int texflag = dz->tex->txflag;
	motifptr here, thismotif = dz->tex->motifhead;
	int n;
	dz->tex->phrasecnt = 0;    
	if(texflag & ORN_DEC_OR_TIMED) {
		if((exit_status = motifchek(thismotif))<0)
			return(exit_status);
		dz->tex->timeset = thismotif;					/* 1 */
		thismotif = thismotif->next;
	}
	if(texflag & ISHARM) {								/* 2 */
		if((exit_status = motifchek(thismotif))<0)
			return(exit_status);
		dz->tex->hfldmotif = thismotif;
		thismotif = thismotif->next;
	}
	if((texflag & IS_ORNATE) || (texflag & IS_MOTIFS)) {	/* 3 */
		if((exit_status = motifchek(thismotif))<0)
			return(exit_status);
		here = thismotif;
		while(thismotif!=(motifptr)0) {					/* 4 */
			dz->tex->phrasecnt++;
			thismotif = thismotif->next;
		}												/* 5 */
		if((dz->tex->phrase = (motifptr *)malloc(dz->tex->phrasecnt * sizeof(motifptr)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for phrase array.\n");
			return(MEMORY_ERROR);
		}
		thismotif = here;
		n = -1;
		while(thismotif!=(motifptr)0) {
			dz->tex->phrase[++n] = thismotif;
			thismotif = thismotif->next;
		}
	}
	return(FINISHED);
}

/***************************** MASSAGE_PARAMS ****************************/

int massage_params(dataptr dz)
{
	int exit_status;
	unsigned int texflag = dz->tex->txflag;
	convert_cmdline_instrnos_to_internal_representation(dz);
	if((exit_status = check_max_transpos_compatible_with_splicelen(dz))<0)
		return(exit_status);
	if(texflag & IS_CLUMPED) {
		if((exit_status = set_amptype_params(dz))<0)
			return(exit_status);
	}
	if(texflag & IS_DECOR) {
		if((exit_status = set_decor_pitchposition_params(dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************* INIT_MOTIFS *****************************
 *
 * Set up head item of a list of musical-motifs.
 * Set up location of first note of first motif.
 */

int init_motifs(dataptr dz)
{
	int exit_status;
	noteptr thisnote;
	if((dz->tex->motifhead = (motifptr)malloc(sizeof (struct motif)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store any motifs.\n");
		return(MEMORY_ERROR);
	}
	dz->tex->motifhead->next  = (motifptr)0;
	dz->tex->motifhead->last  = (motifptr)0;
	if((exit_status = init_note(&thisnote))<0)
		return(exit_status);
	dz->tex->motifhead->firstnote = thisnote;
	thisnote->next = (noteptr)0;   
	thisnote->last = (noteptr)0;   
	return(FINISHED);
}

/******************************** INITPERM *********************************
 *
 * Establish storage space for permutation parameters.
 *
 * (1)	create pointers to the items within the permuted sets.		Initialise to zero.
 * (2)	Create storage for current length of each perm.				Initialise to zero.
 * (3)	Create storage for previous length of each perm.			Initialise to -1 (impossible).
 *		If new perm different length to old, space for permuted elements will need to be re-malloced.
 * (4)	Create pointers to storage areas for each permuted set.		Initialise to point nowhere. 
 *		These will be malloced when the size of each set-to-permute is known.
 * (5)	Create storage for previous outputed val in each perm.		Initialise to -1 (impossible). 
 *		This is useful at permutation boundaries, when a particular element may be repeated more
 *		than permitted no. of times if there's no check. These constants allow no. of repets to be counted.
 * (6)	Create counter of current no. of consecutive repets of an element in each of the (output) perms.
 *		Initialise to 0, though this is arbitrary, as repetcnt should always be set on any call of do_perm.
 * (7)	Create storage for max no. of allowed consecutive repets of an element in each perm-set.
 *		Initialise to 1, which is the default if no info is given to the contrary.
 */

int initperm(int ***permm,dataptr dz)
{
	int n;
	if((dz->iparray[TXPERMINDEX] = (int *)malloc(PERMCNT * sizeof(int)))==NULL) {   /* 1 */
		sprintf(errstr,"INSUFFICIENT MEMORY for permutation indeces.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<PERMCNT;n++)
		dz->iparray[TXPERMINDEX][n] = 0;

	if((dz->iparray[TXPERMLEN] = (int *)malloc(PERMCNT * sizeof(int)))==NULL) {   /* 2 */
		sprintf(errstr,"INSUFFICIENT MEMORY for permutation lengths.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<PERMCNT;n++)
		dz->iparray[TXPERMLEN][n] = 0;

	if((dz->iparray[TXLASTPERMLEN] = (int *)malloc(PERMCNT * sizeof(int)))==NULL) {   /* 3 */
		sprintf(errstr,"INSUFFICIENT MEMORY for last permutation lengths.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<PERMCNT;n++)
		dz->iparray[TXLASTPERMLEN][n] = -1;

	if((*permm = (int **)malloc(PERMCNT * sizeof(int *)))==NULL) {   /* 4 */
		sprintf(errstr,"INSUFFICIENT MEMORY for permutations.\n");
		return(MEMORY_ERROR);
	}

	for(n=0;n<PERMCNT;n++)
		(*permm)[n] = (int *)0;

	if((dz->iparray[TXLASTPERMVAL] = (int *)malloc(PERMCNT * sizeof(int)))==NULL) {   /* 5 */
		sprintf(errstr,"INSUFFICIENT MEMORY for last permutation values.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<PERMCNT;n++)
		dz->iparray[TXLASTPERMVAL][n] = -1;

	if((dz->iparray[TXREPETCNT] = (int *)malloc(PERMCNT * sizeof(int)))==NULL) {   /* 6 */
		sprintf(errstr,"INSUFFICIENT MEMORY for permutation repetition counts.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<PERMCNT;n++)
		dz->iparray[TXREPETCNT][n] = 1;

	if((dz->iparray[TXRPT] = (int *)malloc(PERMCNT * sizeof(int)))==NULL) {   /* 7 */
		sprintf(errstr,"INSUFFICIENT MEMORY for permutation repetitions.\n");
		return(MEMORY_ERROR);
	}
	/* THESE ARE THE DEFAULT NUMBER OF REPETITIONS-OF-AN-ENTITY ALLOWED IN A PERM */
	dz->iparray[TXRPT][PM_SPACE]  = SPACE_REPETS;
	dz->iparray[TXRPT][PM_PITCH]  = PITCH_REPETS;
	dz->iparray[TXRPT][PM_AMP]    = AMP_REPETS;
	dz->iparray[TXRPT][PM_DUR]    = DUR_REPETS;
	dz->iparray[TXRPT][PM_INSNO]  = INSNO_REPETS;
	dz->iparray[TXRPT][PM_GPRANG] = GPRANG_REPETS;
	dz->iparray[TXRPT][PM_GPSIZE] = GPSIZE_REPETS;
	dz->iparray[TXRPT][PM_GPDENS] = GPDENS_REPETS;
	dz->iparray[TXRPT][PM_GPCNTR] = GPCNTR_REPETS;
	dz->iparray[TXRPT][PM_GPPICH] = GPPICH_REPETS;
	dz->iparray[TXRPT][PM_ORNPOS] = ORNPOS_REPETS;
	dz->iparray[TXRPT][PM_GPSPAC] = GPSPAC_REPETS;
	dz->iparray[TXRPT][PM_ORIENT] = ORIENT_REPETS;
	dz->iparray[TXRPT][PM_DECABV] = DECABV_REPETS;
	dz->iparray[TXRPT][PM_MULT]   = MULT_REPETS;
	dz->iparray[TXRPT][PM_WHICH]  = WHICH_REPETS;
	dz->iparray[TXRPT][PM_GPPICH2]= GPPICH2_REPETS;
	return(FINISHED);
}

/************************************ GENERATE_OUTPOSITIONS ************************************
 *
 * In mode 1, output positions are preset, and permuted.
 */

int generate_outpositions(dataptr dz)
{
	int *lmost  = dz->iparray[dz->iarray_cnt-1], kk, jj, n, m;
	double *pos = dz->parray[dz->array_cnt-1], lowerpos = 0.0, dndiff, updiff, randwander, rando;
	kk = dz->infilecnt/dz->iparam[TEXTURE_OUTCHANS];
	if((kk * dz->iparam[TEXTURE_OUTCHANS]) != dz->infilecnt)
		kk++;		// number of positions per lspkr pair
	jj = 0;
	for(n = 0; n < dz->iparam[TEXTURE_OUTCHANS];n++) {
		for(m=0;m < kk;m++) {
			lmost[jj] = (n + (int)round(dz->param[TEXTURE_POS])) % dz->iparam[TEXTURE_OUTCHANS];
			if(kk==1)
				pos[jj] = 0.5;
			else {
				if(m == 0)
					pos[jj] = 1/(double)(2*kk);						//	e.g kk = 3 gives 1/6 : kk = 4 gives 1/8 
				else
					pos[jj] = (1/(double)(2*kk)) + (m/(double)kk);	//	e.g kk = 3 gives 1/6 +1/3: kk = 4 gives 1/8+1/4, 1/8+1/2 
			}
			jj++;
		}
		if(dz->param[TEXTURE_SPRD] > 0.0) {							//	Do rand scatter
			rando = max(dz->param[TEXTURE_SPRD],1.0);
			jj -= kk;
			lowerpos = 0;
			for(m=0;m < kk;m++) {
				if(m == 0)
					dndiff = pos[jj];
				else
					dndiff = (pos[jj] - lowerpos)/2.0;
				lowerpos = pos[jj];
				if(m == kk - 1)
					updiff = 1 - pos[jj];
				else
					updiff = (pos[jj+1] - pos[jj])/2.0;
				randwander = drand48() * rando;
				randwander *= 2.0;
				randwander -= 1.0;
				if(randwander < 0.0)
					pos[jj] += dndiff * randwander;
				else
					pos[jj] += updiff * randwander;
				jj++;
			}
		}
	}
	return FINISHED;
}
