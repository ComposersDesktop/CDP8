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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sfsys.h>
#include <osbind.h>
#include <ctype.h>
#include <structures.h>
#include <pnames.h>
#include <processno.h>
#include <flags.h>
#include <modeno.h>
#include <cdpmain.h>
#include <globcon.h>
#include <tkglobals.h>
#include <logic.h>
#include <math.h>
#include <hfperm.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

#define BY_STACKING (0)
#define BY_DENSITY  (1)

#define SMALLEST_FIRST (0)
#define LARGEST_FIRST (1)

#define ROOT (0)
#define TOPNOTE (1)

#define KEEP_OCT_EQUIVS (0)
#define ELIMINATE_OCT_EQUIVS (1)

#define ROLLOFF (0.66)

static int 	generate_perms(double ***combo,int **combolen,int *combocnt,dataptr dz);
static int	set_element(int element_no,int total_elements,int field_cnt,int *element,
				double ***combo,int **combolen,int *combocnt);
static int 	store_combo(double ***combo,int *combocnt,int **combolen,int *element,int total_elements);
static void substitute_midivals_in_perms(double **combo,int *combolen,int combocnt,dataptr dz);
static int 	generate_hfsets(int combocnt,double **combo,int *combolen,
				double ***hfset,int **hfsetlen,int *hfsetcnt,dataptr dz);
static int 	transpose_element(int element_no,double *thiscombo,int thiscombolen,double *origval,
				double ***hfset,int **hfsetlen,int *hfsetcnt,dataptr dz);
static int 	store_transposition(double ***hfset,int **hfsetlen,int *hfsetcnt,double *thiscombo,int thiscombolen);
static int sort_hfsets(double **hfset,int *hfsetlen,int *hfsetcnt,int *grping,dataptr dz);
static int gen_output(int hfsetcnt,double **hfset,int *hfsetlen,int *grping,dataptr dz);
static int gen_outputs(int hfsetcnt,double **hfset,int *hfsetlen,int *grping,dataptr dz);
static int gen_text_output(int hfsetcnt,double **hfset,int *hfsetlen,int *grping,dataptr dz);
static int gen_midi_output(int hfsetcnt,double **hfset,int *hfsetlen,int *grping,dataptr dz);
static void print_grouping_structure(int n,int *grping,int *grping_cnt,int *in_grp,int hfsetcnt);
static void printout_grouping_structure(int n,int *grping,int *grping_cnt,int *in_grp,int hfsetcnt,dataptr dz);
static void set_pitch_to_print(int midi);
static void set_midi_to_print(int midi);
static void print_pitchset(void);
static void printout_pitchset(dataptr dz);
static int sort_by_chord(double **hfset,int *hfsetlen,double *intv,int *hfsetcnt,int *grping,dataptr dz);
static int sort_by_pitchclass_equivalence
	(double **hfset,int *hfsetlen,double *pich,int hfsetcnt,int *grping,int *intcnt,
	double *intv,int *intvcnt,double *intv2,int *intvcnt2,dataptr dz);
static int sort_by_reference_note_equivalence
	(double **hfset,int *hfsetlen,int hfsetcnt,int *grping,dataptr dz);
static void sort_by_maxint(double **hfset,int n,int m,int setlen,dataptr dz);
static void sort_by_density(double **hfset,int n,int m,int setlen,int lowest);
static void sort_by_density_regardless_of_size
	(double **hfset,double *intv,int *intcnt,double *intv2,int *intcnt2,int n,int m,int setlen,int densest_first);
static void sort_by_maxintsize_and_density(double **hfset,int n,int m,int setlen,int lowest);
static void sort_by_octave_equivalence(double **hfset,int n,int m,int setlen);
static void sort_by_interval_stacking(double **hfset,int n,int m,int setlen,int highest_first);
static void eliminate_oct_duplicated_sets(double **hfset,int *hfsetcnt,int *hfsetlen,int *grping,int *grping_cnt,int type);
static int  setup_naming(char **thisfilename,dataptr dz);
static int  create_outfile_name(int ccnt,char *thisfilename,dataptr dz);
static void gen_note(int maxjitter,double *tabstart,double *tabend,double **tabmin, double **tabmax,double **hfset,
	double tabscale,double sintabsize,int splicelen, double *sintab,int n, int j);
int next_file(int *samps_written,char *thisfilename,int *ccnt,unsigned int *space_needed,
	int *samp_space_available, dataptr dz);
static void gen_note2(double *tabstart,double *tabend,double midival,
	double tabscale,double sintabsize,int splicelen, double *sintab);
//static void gen_note3(double *tabstart,double *tabend,double midival,int tabsize,int splicelen, short *intab);
static void gen_note3(double *tabstart,double *tabend,double midival,int tabsize,int splicelen, float *intab);
int gen_dp_output(dataptr dz);

#define LEAVESPACE	(10*1024)

int sorting_type = 0;

int do_hfperm(dataptr dz)
{
	int exit_status;
	char temp[48];
	double **hfset = 0, **combo, secs;
	int *combolen, combocnt = 0;
	int hfsetcnt = 0, hrs = 0, mins = 0;
	int *hfsetlen = 0;
	int *grping = NULL;
	int grping_cnt;
	if((exit_status = generate_perms(&combo,&combolen,&combocnt,dz))<0)
		return(exit_status);
	substitute_midivals_in_perms(combo,combolen,combocnt,dz);	
	switch(dz->process) {
	case(HF_PERM1) :
		if((exit_status = generate_hfsets(combocnt,combo,combolen,&hfset,&hfsetlen,&hfsetcnt,dz))<0)
			return(exit_status);
		break;
	case(HF_PERM2):
		hfset = combo;
		hfsetcnt = combocnt;
		hfsetlen = combolen;
	}
	if((grping = (int *)malloc(hfsetcnt * sizeof(int)))==NULL)
		 print_outwarning_flush("Insufficient memory to store grouping information.\n");
	grping_cnt = sort_hfsets(hfset,hfsetlen,&hfsetcnt,grping,dz);
	grping_cnt /= 2;
	dz->tempsize  = hfsetcnt * (dz->iparam[HP1_ELEMENT_SIZE] + dz->iparam[HP1_GAP_SIZE]);
	dz->tempsize += grping_cnt * dz->iparam[HP1_GGAP_SIZE];
	if(dz->true_outfile_stype == SAMP_FLOAT)
		dz->tempsize *= sizeof(float);
	else
		dz->tempsize *= sizeof(short);
	if(dz->mode == HFP_SNDOUT || dz->mode == HFP_SNDSOUT) {
		if(dz->mode == HFP_SNDOUT && ((unsigned int)dz->tempsize > (getdrivefreespace(dz->outfilename) - LEAVESPACE))) {
			sprintf(errstr,"dz->tempsize = %d dz->outfilesize = %d Insufficient space on the hard disk to store the output file.\n",
				 dz->tempsize,dz->outfilesize);
			 return(MEMORY_ERROR);
		} else {
			sprintf(errstr,"Generating %d chords\n",hfsetcnt);
			print_outmessage(errstr);
			if((secs = dz->tempsize/(double)dz->infile->srate) > 60.0) {
				mins = (int)floor(secs/60.0);
				secs -= (mins * 60.0);
				if(mins  > 60) {
					hrs = mins/60;
					mins -= (hrs * 60);
				}
			}
			sprintf(errstr,"Duration of output file: ");
			if(hrs) {
				sprintf(temp,"%d hrs ",hrs);
				strcat(errstr,temp);
			}
			if(mins) {
				sprintf(temp,"%d mins ",mins);
				strcat(errstr,temp);
			}
			sprintf(temp,"%.2lf secs\n",secs);
			strcat(errstr,temp);
			print_outmessage_flush(errstr);
		}		
	}
	switch(dz->mode) {
	case(HFP_SNDOUT):
		if((exit_status = gen_output(hfsetcnt,hfset,hfsetlen,grping,dz))<0)
			return(exit_status);
		break;
	case(HFP_SNDSOUT):
		if((exit_status = gen_outputs(hfsetcnt,hfset,hfsetlen,grping,dz))<0)
			return(exit_status);
		break;
	case(HFP_TEXTOUT):
		if((exit_status = gen_text_output(hfsetcnt,hfset,hfsetlen,grping,dz))<0)
			return(exit_status);
		break;
	case(HFP_MIDIOUT):
		if((exit_status = gen_midi_output(hfsetcnt,hfset,hfsetlen,grping,dz))<0)
			return(exit_status);
		break;
	}
	return(FINISHED);
}

/************************** GENERATE_PERMS **************************/

int generate_perms(double ***combo,int **combolen,int *combocnt,dataptr dz)
{
	int exit_status;
	int n, j, maxsize;	
	int *element;
	if(dz->iparam[HP1_HFCNT] == dz->iparam[HP1_MINSET]) {
		if((*combo = (double **)malloc(sizeof(double *)))==NULL) {
			sprintf(errstr,"Out of memory for harmonic combinations.\n");
			return(MEMORY_ERROR);
		}				
		if((*combolen = (int *)malloc(sizeof(int)))==NULL) {
			sprintf(errstr,"Out of memory for harmonic combination counts.\n");
			return(MEMORY_ERROR);
		}				
		if(((*combo)[0] = (double *)malloc(dz->iparam[HP1_HFCNT] * sizeof(double)))==NULL) {
			sprintf(errstr,"Out of memory for harmonic combinations.\n");
			return(MEMORY_ERROR);
		}				
		for(n=0;n<dz->iparam[HP1_HFCNT];n++)
			(*combo)[0][n] = n;
		(*combolen)[0] = dz->iparam[HP1_HFCNT];
		*combocnt = 1;
		return(FINISHED);
	} else {
		if((element = (int *)malloc(dz->iparam[HP1_HFCNT] * sizeof(int)))==NULL) {
			sprintf(errstr,"Out of memory for permutation elements.\n");
			return(MEMORY_ERROR);
		}				
		if(dz->vflag[HP1_MINONLY]) 
			maxsize = dz->iparam[HP1_MINSET];
		else
			maxsize = dz->iparam[HP1_HFCNT];

		for(n=dz->iparam[HP1_MINSET];n<=maxsize;n++) {
			for(j=0;j<n;j++)	 	/* initialise perm elements */
				element[j] = j-1;
			if((exit_status = set_element(0,n,dz->iparam[HP1_HFCNT],element,combo,combolen,combocnt))<0) {
				free(element);
				return(exit_status);
			}
		}
	}
	free(element);
	return(FINISHED);
}

/************************** SET_ELEMENT **************************
 *
 * Recursive function to set successive elements of a permutation.
 */

int set_element(int element_no,int total_elements,int field_cnt,int *element,double ***combo,int **combolen,int *combocnt)
{
	int exit_status, k;
	int limit = field_cnt - total_elements + element_no;
	for(;;) {
		element[element_no]++;					/* incr the element number */
		if(element[element_no] > limit) {		/* if it's now beyond max for this element */
			if(element_no > 0) {				/* reset this, & all elements above it */
				k = element_no;					/* to be in ascending sequence from element below */
				while(k < total_elements) {		/* ready for incrementation of next lowest element */
					element[k] = element[k-1] + 1;
					k++;
				}
			}
			return(CONTINUE);					/* then return to next lowest element */
		}
		if(element_no >= total_elements-1) {	/* if all elements have been set, store the combination */
			if((exit_status = store_combo(combo,combocnt,combolen,element,total_elements))<0)
				return(exit_status);
		} else  { 								/* but if not, go on to set the next element */
			if((exit_status = set_element(element_no+1,total_elements,field_cnt,element,combo,combolen,combocnt))<0)
				return(exit_status);
		}
	}
	return(CONTINUE);	/* NOTREACHED */
}

/************************** STORE_COMBO **************************/

int store_combo(double ***combo,int *combocnt,int **combolen,int *element,int total_elements)
{
	int n, j;
	j = *combocnt;
	(*combocnt)++;								/* incr count of combinations */
	if(*combocnt == 1) {						/* create a new combo_store location */
		if((*combo = (double **)malloc(sizeof(double *)))==NULL) {
			sprintf(errstr,"Out of memory for harmonic combinations.\n");
			return(MEMORY_ERROR);
		}										/* create a new combo_store_cnt location */
		if((*combolen = (int *)malloc(sizeof(int)))==NULL) {
			sprintf(errstr,"Out of memory for harmonic combinations.\n");
			return(MEMORY_ERROR);
		}				
	} else {									
		if((*combo = (double **)realloc((char *)(*combo),(*combocnt) * sizeof(double *)))==NULL) {
			sprintf(errstr,"Out of memory for harmonic combinations.\n");
			return(MEMORY_ERROR);
		}				
		if((*combolen = (int *)realloc((char *)(*combolen),(*combocnt) * sizeof(int)))==NULL) {
			sprintf(errstr,"Out of memory for harmonic combinations.\n");
			return(MEMORY_ERROR);
		}				
	}											/* create the new combo_store */
	if(((*combo)[j] = (double *)malloc(total_elements * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of memory for harmonic combinations.\n");
		return(MEMORY_ERROR);
	}				
	for(n=0; n < total_elements; n++)			/* store the new combo */
		(*combo)[j][n] = (double)element[n];
	(*combolen)[j] = total_elements;			/* store the count of the new combo */
	return(FINISHED);
}

/************************** ELIMINATE_OCTAVE_DUPLICATIONS **************************/

int eliminate_octave_duplications(dataptr dz)
{
	double *hf = dz->parray[0];
	int n, m, k, octaves_eliminated = 0;
	for(n=0;n< dz->iparam[HP1_HFCNT]-1;n++) {	
		for(m=n+1;m< dz->iparam[HP1_HFCNT];m++) {
			if(flteq(fmod(hf[n],12.0),fmod(hf[m],12.0))) {
				octaves_eliminated = 1;
				k = m;
				dz->iparam[HP1_HFCNT]--;
				while(k < dz->iparam[HP1_HFCNT]) {
					hf[k] = hf[k+1];
					k++;
				}
				m--;
			}
		}
	}
	return octaves_eliminated;
}

/**************************** SUBSTITUTE_MIDIVALS_IN_PERMS *******************************/

void substitute_midivals_in_perms(double **combo,int *combolen,int combocnt,dataptr dz)
{
	double *hf = dz->parray[0];
	int n, m, k;
	for(n=0;n<combocnt;n++) {
		for(m=0;m < combolen[n];m++) {
			k = (int)combo[n][m];
			combo[n][m] = hf[k];
		}
	}
}

/**************************** BUBLSORT_HF *******************************/

void bublsort_hf(double *hf,dataptr dz)
{
	int n, m;
	double temp;
	for(n=0;n<dz->iparam[HP1_HFCNT]-1;n++) {
		for(m = n+1; m<dz->iparam[HP1_HFCNT]; m++) {
			if(hf[m] < hf[n]) {
				temp = hf[n];
				hf[n] = hf[m];
				hf[m] = temp;
			}
		}
	}
}

/**************************** GENERATE_OCTS *******************************/

int transpose_element(int element_no,double *thiscombo,int thiscombolen,double *origval,
	double ***hfset,int **hfsetlen,int *hfsetcnt,dataptr dz)
{
	int exit_status;
	for(;;) {
		thiscombo[element_no] += 12.0;
		if(thiscombo[element_no] > (double)dz->iparam[HP1_TOPNOTE]) {
			thiscombo[element_no] = origval[element_no];
			return CONTINUE;					
		}
		if(element_no >= thiscombolen-1) {	/* if all elements have been set, store the hfset */
			if((exit_status = store_transposition(hfset,hfsetlen,hfsetcnt,thiscombo,thiscombolen))<0)
				return(exit_status);
		} else  { 								/* but if not, go on to set the next element */
			if((exit_status = 
			transpose_element(element_no+1,thiscombo,thiscombolen,origval,hfset,hfsetlen,hfsetcnt,dz))<0)
				return(exit_status);
		}
	}
	return(CONTINUE);	/* NOTREACHED */
}

/**************************** STORE_TRANSPOSITION *******************************/

int store_transposition(double ***hfset,int **hfsetlen,int *hfsetcnt,double *thiscombo,int thiscombolen)
{
	int n, j;
	j = *hfsetcnt;
	(*hfsetcnt)++;								/* incr count of hfset */
	if(*hfsetcnt == 1) {						/* create a new hfset_store location */
		if((*hfset = (double **)malloc(sizeof(double *)))==NULL) {
			sprintf(errstr,"Out of memory for final sets.\n");
			return(MEMORY_ERROR);
		}										/* create a new hfset_store_cnt location */
		if((*hfsetlen = (int *)malloc(sizeof(int)))==NULL) {
			sprintf(errstr,"Out of memory for final sets.\n");
			return(MEMORY_ERROR);
		}				
	} else {									
		if((*hfset = (double **)realloc((char *)(*hfset),(*hfsetcnt) * sizeof(double *)))==NULL) {
			sprintf(errstr,"Out of memory for final sets.\n");
			return(MEMORY_ERROR);
		}				
		if((*hfsetlen = (int *)realloc((char *)(*hfsetlen),(*hfsetcnt) * sizeof(int)))==NULL) {
			sprintf(errstr,"Out of memory for final sets.\n");
			return(MEMORY_ERROR);
		}				
	}											/* create the new hfset store */
	if(((*hfset)[j] = (double *)malloc(thiscombolen * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of memory for final sets.\n");
		return(MEMORY_ERROR);
	}				
	for(n=0; n < thiscombolen; n++)			/* store the new hfset */
		(*hfset)[j][n] = thiscombo[n];
	(*hfsetlen)[j] = thiscombolen;			/* store the count of the new hfset */
	return(FINISHED);
}

/************************** GENERATE_HFSETS **************************/

int generate_hfsets(int combocnt,double **combo,int *combolen,double ***hfset,int **hfsetlen,int *hfsetcnt,dataptr dz)
{
	int exit_status;
	int n, j;
	double *origval;
	if((origval = (double *)malloc(dz->iparam[HP1_HFCNT] * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of memory for storing original comobo vals.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<combocnt;n++) {
		for(j=0;j<combolen[n];j++) {
			combo[n][j] -= 12.0;
			origval[j] = combo[n][j];
		}
		if((exit_status = transpose_element(0,combo[n],combolen[n],origval,hfset,hfsetlen,hfsetcnt,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************** SORT_HFSETS **************************/

int sort_hfsets(double **hfset,int *hfsetlen,int *hfsetcnt,int *grping,dataptr dz)
{
	int n, m, j;
	double temp;
	int itemp;
	double *intv, *intv2, *pich;
	int *intvcnt, *intvcnt2;
//	int grping_cnt = 0, in_grp = 0;
	int grping_cnt = 0;
	int *minint = NULL, *intcnt = NULL;
	int hfsetcount = *hfsetcnt;

	grping[0] = -1;					/* flags output not to do grouping */

	for(j = 0;j<hfsetcount;j++) {		 /*	Sort the individual sets themselves, so pitches are in ascending order */
		for(n=0;n<hfsetlen[j]-1;n++) {
			for(m = n+1; m<hfsetlen[j]; m++) {
				if(hfset[j][m] < hfset[j][n]) {
					temp = hfset[j][n];
					hfset[j][n] = hfset[j][m];
					hfset[j][m] = temp;
				}
			}
		}
	}
	print_outmessage_flush("Sorting chords by note-count.\n");
	for(n=0;n<hfsetcount-1;n++) {		 /*	Sort the sets by size */
		for(m = n+1; m<hfsetcount; m++) {
			if(hfsetlen[m] < hfsetlen[n]) {
				itemp = hfsetlen[n];
				hfsetlen[n] = hfsetlen[m];
				hfsetlen[m] = itemp;
			}
		}
	}
	switch(dz->iparam[HP1_SORT1]) {
	case(ROOT):
	case(TOP):
	case(PCLASS):
		if ((intv = (double *)malloc(dz->iparam[HP1_HFCNT] * sizeof(double)))==NULL) {
			print_outwarning_flush("Insufficient memory to sort chords in terms of pitch equivalence.\n");
			return 0;
		}														
		if ((intv2 = (double *)malloc(dz->iparam[HP1_HFCNT] * sizeof(double)))==NULL) {
			print_outwarning_flush("Insufficient memory to sort chords in terms of pitch equivalence.\n");
			return 0;
		}														
		if ((intvcnt = (int *)malloc(dz->iparam[HP1_HFCNT] * sizeof(int)))==NULL) {
			print_outwarning_flush("Insufficient memory to sort chords in terms of pitch equivalence.\n");
			return 0;
		}														
		if ((intvcnt2 = (int *)malloc(dz->iparam[HP1_HFCNT] * sizeof(int)))==NULL) {
			print_outwarning_flush("Insufficient memory to sort chords in terms of pitch equivalence.\n");
			return 0;
		}														
		if ((pich = (double *)malloc(dz->iparam[HP1_HFCNT] * sizeof(double)))==NULL) {
			print_outwarning_flush("Insufficient memory to sort chords in terms of pitch equivalence.\n");
			return 0;
		}														
		if(dz->vflag[HP1_DENS] == OTHER_SORT) {
			if ((minint = (int *)malloc(hfsetcount * sizeof(int)))==NULL)
				print_outwarning_flush("Insufficient memory to sort chord-groups.\n");
			else if ((intcnt = (int *)malloc(hfsetcount * sizeof(int)))==NULL)
				print_outwarning_flush("Insufficient memory to sort chord-groups.\n");
		}										
		switch(dz->iparam[HP1_SORT1]) {
		case(PCLASS):
			print_outmessage_flush("Sorting chords by pitchclass-equivalence.\n");
			grping_cnt = sort_by_pitchclass_equivalence
				(hfset,hfsetlen,pich,hfsetcount,grping,intcnt,intv,intvcnt,intv2,intvcnt2,dz);
			break;
		default:
			print_outmessage_flush("Sorting chords by reference-note-equivalence.\n");
			grping_cnt = sort_by_reference_note_equivalence(hfset,hfsetlen,hfsetcount,grping,dz);
			break;
		}
		break;
	case(CHORDTYPE):
	case(CHORD_1):
		if ((intv = (double *)malloc(dz->iparam[HP1_HFCNT] * sizeof(double)))==NULL) {
			print_outwarning_flush("Insufficient memory to sort chords in terms of interval equivalence.\n");
			return 0;
		}														
		print_outmessage_flush("Sorting chords by interval-equivalence.\n");
		grping_cnt = sort_by_chord(hfset,hfsetlen,intv,hfsetcnt,grping,dz);
		break;
	default:
		sprintf(errstr,"Unknown chord sort type.\n");
		return(PROGRAM_ERROR);
	}
	if(dz->vflag[HP1_ELIMOCTS] && (dz->iparam[HP1_SORT1] != CHORD_1))
		eliminate_oct_duplicated_sets(hfset,hfsetcnt,hfsetlen,grping,&grping_cnt,sorting_type);	
	return grping_cnt;
}

/************************** GEN_OUTPUT **************************/

int gen_output(int hfsetcnt,double **hfset,int *hfsetlen,int *grping,dataptr dz)
{
	int exit_status;
//	short *obuf = dz->sampbuf[0];
	float *obuf = dz->sampbuf[0];
	double *tab = (double *)dz->sampbuf[2];
	double *tabend = tab + dz->iparam[HP1_ELEMENT_SIZE];
	double tabscale = (double)HFP_TABSIZE/dz->infile->srate;
	double *sintab = (double *)dz->sampbuf[1];
	double sintabsize = (double)HFP_TABSIZE;
	double element_level;
	int samps_written = 0, samp_space_available, samps_to_write, gap_size;
	int n, i, j, grping_cnt = 0;
	int maxjitter = (int)round(drand48() * HFP_MAXJITTER * MS_TO_SECS * (double)dz->infile->srate);
	unsigned int element_byte_len = dz->iparam[HP1_ELEMENT_SIZE] * sizeof(double);
	unsigned int element_full_len = element_byte_len + (maxjitter * sizeof(double));
	double *tabmax = 0, *tabmin = tab + dz->iparam[HP1_ELEMENT_SIZE] + maxjitter;
	int splicelen = (int)round(HP1_SPLICELEN * MS_TO_SECS * (double)dz->infile->srate);
	int in_group = 0, in_grp = 0;

	print_outmessage_flush("Synthesis beginning.\n");
	memset((char *)dz->sampbuf[0],0,(size_t)dz->buflen * sizeof(float));
	for(n=0;n<hfsetcnt;n++) {
		memset((char *)dz->sampbuf[2],0,element_full_len);

		if(grping!=NULL && grping[0] != -1)
			print_grouping_structure(n,grping,&grping_cnt,&in_grp,hfsetcnt);
		errstr[0] = ENDOFSTR;

		for(j=0;j<hfsetlen[n];j++) {
			set_pitch_to_print((int)hfset[n][j]);
			gen_note(maxjitter,tab,tabend,&tabmin,&tabmax,hfset,tabscale,sintabsize,splicelen,sintab,n,j);
		}
		print_pitchset();
		element_level = HFP_OUTLEVEL/hfsetlen[n];
		tab = tabmin;
		while(tab < tabmax)
			*tab++ *= element_level;
		/* DO OUTPUT */
		samp_space_available = dz->buflen - samps_written;
		gap_size = dz->iparam[HP1_GAP_SIZE];
		if(grping!=NULL && grping[0] != -1) {
			if(grping[grping_cnt]==n) {
				in_group = 1;
				gap_size += dz->iparam[HP1_GGAP_SIZE];	 /* make gap at start */
				grping_cnt++;
			} else if(grping[grping_cnt]==-n) {
				in_group = 0;
				gap_size += dz->iparam[HP1_GGAP_SIZE];		/* make gap at end */
				grping_cnt++;				/* if grp end coincides with gp start */
				if(grping[grping_cnt]==n) { /* skip a grping value */
					in_group = 1;
					grping_cnt++;
				}
			} else if (!in_group)
				gap_size += dz->iparam[HP1_GGAP_SIZE]; 		/* make gap between non-grouped entries */
		}
		if(n!=0) {							/* no gap at start */
			while(gap_size >= samp_space_available) {
				if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
					return(exit_status);
				memset((char *)dz->sampbuf[0],0,(size_t)dz->buflen * sizeof(float));
				gap_size -= samp_space_available;
				samp_space_available = dz->buflen;
				samps_written = 0;
			}
			samps_written += gap_size;
			samp_space_available = dz->buflen - samps_written;
		}
		i = 0;
		tab = tabmin;
		samp_space_available = dz->buflen - samps_written;
		samps_to_write = tabmax - tabmin;
		while(i < samps_to_write) {
			if(samp_space_available > samps_to_write) {
				while(i < samps_to_write)
//					obuf[samps_written++] = (short)round(tab[i++] * MAXSAMP);
					obuf[samps_written++] = (float)tab[i++];
			} else {
				while(samps_written < dz->buflen)
//					obuf[samps_written++] = (short)round(tab[i++] * MAXSAMP);
					obuf[samps_written++] = (float)tab[i++];
				if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
					return(exit_status);
				memset((char *)dz->sampbuf[0],0,(size_t)dz->buflen * sizeof(float));
				samps_written = 0;
				samp_space_available = dz->buflen;
			}
		}
	}
	if(grping!=NULL && grping[0] != -1)
		print_grouping_structure(n,grping,&grping_cnt,&in_grp,hfsetcnt);
	samps_written += dz->iparam[HP1_GAP_SIZE];
	while(samps_written >= dz->buflen) {
		if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
			return(exit_status);
		samps_written -= dz->buflen;
	}	
	if(samps_written > 0) {
		if((exit_status = write_samps(dz->sampbuf[0],samps_written,dz))<0)
			return(exit_status);
	}	
	return(FINISHED);
}

/************************** GEN_OUTPUTS **************************/

int gen_outputs(int hfsetcnt,double **hfset,int *hfsetlen,int *grping,dataptr dz)
{
	int exit_status;
	float *obuf = dz->sampbuf[0];
	double *tab = (double *)dz->sampbuf[2];
	double *tabend = tab + dz->iparam[HP1_ELEMENT_SIZE];
	double tabscale = (double)HFP_TABSIZE/dz->infile->srate;
	double *sintab = (double *)dz->sampbuf[1];
	double sintabsize = (double)HFP_TABSIZE;
	double element_level;
	int samps_written = 0, samp_space_available, samps_to_write, gap_size;
	int n, i, j, grping_cnt = 0;
	int maxjitter = (int)round(drand48() * HFP_MAXJITTER * MS_TO_SECS * (double)dz->infile->srate);
	unsigned int element_byte_len = dz->iparam[HP1_ELEMENT_SIZE] * sizeof(double);
	unsigned int element_full_len = element_byte_len + (maxjitter * sizeof(double));
	double *tabmax = 0, *tabmin = tab + dz->iparam[HP1_ELEMENT_SIZE] + maxjitter;
	int splicelen = (int)round(HP1_SPLICELEN * MS_TO_SECS * (double)dz->infile->srate);
	int in_group = 0, in_grp = 0;
	char *thisfilename;
	int ccnt = 0;
	unsigned int space_needed = dz->tempsize;

	superzargo = 0;
	if((exit_status = setup_naming(&thisfilename,dz))<0)
		return(exit_status);
	if((exit_status = create_outfile_name(0,thisfilename,dz))<0)
		return(exit_status);
	dz->process_type = UNEQUAL_SNDFILE;	/* allow sndfile to be created */
	if((exit_status = create_sized_outfile(thisfilename,dz))<0) {
		fprintf(stdout, "WARNING: Soundfile %s already exists.\n", thisfilename);
		fflush(stdout);
		dz->process_type = OTHER_PROCESS;
		dz->ofd = -1;
		return(PROGRAM_ERROR);
	}							
	if((unsigned int)dz->outfilesize < space_needed) {
		 sprintf(errstr,"Insufficient space on the hard disk to store the output files.\n");
		 return(MEMORY_ERROR);
	}
	dz->total_samps_written = 0;
	print_outmessage_flush("Synthesis beginning.\n");
	memset((char *)dz->sampbuf[0],0,(size_t)dz->buflen * sizeof(float));
	for(n=0;n<hfsetcnt;n++) {
		memset((char *)dz->sampbuf[2],0,element_full_len);

		if(grping!=NULL && grping[0] != -1)
			print_grouping_structure(n,grping,&grping_cnt,&in_grp,hfsetcnt);
		errstr[0] = ENDOFSTR;

		for(j=0;j<hfsetlen[n];j++) {
			set_pitch_to_print((int)hfset[n][j]);
			gen_note(maxjitter,tab,tabend,&tabmin,&tabmax,hfset,tabscale,sintabsize,splicelen,sintab,n,j);
		}
		print_pitchset();
		element_level = HFP_OUTLEVEL/hfsetlen[n];
		tab = tabmin;
		while(tab < tabmax)
			*tab++ *= element_level;
		/* DO OUTPUT */
		samp_space_available = dz->buflen - samps_written;
		gap_size = dz->iparam[HP1_GAP_SIZE];
		if(grping!=NULL && grping[0] != -1) {
			if(grping[grping_cnt]==n) {
				if(in_group == 0 && n!=0) {
					if((exit_status = next_file(&samps_written,thisfilename,&ccnt,&space_needed,&samp_space_available,dz))<0)
						return(exit_status);
				}
				in_group = 1;
				grping_cnt++;
			} else if(grping[grping_cnt]==-n) {	/* if at end of group */
				in_group = 0;
				if((exit_status = next_file(&samps_written,thisfilename,&ccnt,&space_needed,&samp_space_available,dz))<0)
					return(exit_status);
				grping_cnt++;				/* if grp end coincides with gp start */
				if(grping[grping_cnt]==n) { /* skip a grping value */
					in_group = 1;
					grping_cnt++;
				}
			} else if(in_group == 0 && n!=0) {	/* if not a grouped chord */
				if((exit_status = next_file(&samps_written,thisfilename,&ccnt,&space_needed,&samp_space_available,dz))<0)
					return(exit_status);
			}
		}
		if(n!=0) {							/* no gap at start */
			while(gap_size >= samp_space_available) {
				if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
					return(exit_status);
				memset((char *)dz->sampbuf[0],0,(size_t)dz->buflen * sizeof(float));
				gap_size -= samp_space_available;
				samp_space_available = dz->buflen;
				samps_written = 0;
			}
			samps_written += gap_size;
			samp_space_available = dz->buflen - samps_written;
		}
		i = 0;
		tab = tabmin;
		samp_space_available = dz->buflen - samps_written;
		samps_to_write = tabmax - tabmin;
		while(i < samps_to_write) {
			if(samp_space_available > samps_to_write) {
				while(i < samps_to_write)
					obuf[samps_written++] = (float)tab[i++];
			} else {
				while(samps_written < dz->buflen)
					obuf[samps_written++] = (float)tab[i++];
				if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
					return(exit_status);
				memset((char *)dz->sampbuf[0],0,(size_t)dz->buflen * sizeof(float));
				samps_written = 0;
				samp_space_available = dz->buflen;
			}
		}
	}
	if(grping!=NULL && grping[0] != -1)
		print_grouping_structure(n,grping,&grping_cnt,&in_grp,hfsetcnt);
	samps_written += dz->iparam[HP1_GAP_SIZE];
	while(samps_written >= dz->buflen) {
		if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
			return(exit_status);
		samps_written -= dz->buflen;
	}	
	if(samps_written > 0) {
		if((exit_status = write_samps(dz->sampbuf[0],samps_written,dz))<0)
			return(exit_status);
	}	
	/* CLOSE FILE */
	dz->outfiletype  = SNDFILE_OUT;			/* allows header to be written  */
	if((exit_status = headwrite(dz->ofd,dz))<0)
		return(exit_status);
	dz->process_type = OTHER_PROCESS;		
	dz->outfiletype  = NO_OUTPUTFILE;		
	if((exit_status = reset_peak_finder(dz))<0)
		return(exit_status);
	if(sndcloseEx(dz->ofd) < 0) {
		fprintf(stdout,"WARNING: Can't close output soundfile %s\n",thisfilename);
		fflush(stdout);
	}
	dz->ofd = -1;							
	return(FINISHED);
}

/************************** GEN_TEXT_OUTPUT **************************/

int gen_text_output(int hfsetcnt,double **hfset,int *hfsetlen,int *grping,dataptr dz)
{
	int n, j, grping_cnt = 0;
	int in_group = 0, in_grp = 0;

	for(n=0;n<hfsetcnt;n++) {
		if(grping!=NULL && grping[0] != -1) {
			printout_grouping_structure(n,grping,&grping_cnt,&in_grp,hfsetcnt,dz);
		}
		errstr[0] = ENDOFSTR;
		for(j=0;j<hfsetlen[n];j++) {
			set_pitch_to_print((int)hfset[n][j]);
		}
		printout_pitchset(dz);
		if(grping!=NULL && grping[0] != -1) {
			if(grping[grping_cnt]==n) {
				in_group = 1;
				grping_cnt++;
			} else if(grping[grping_cnt]==-n) {	/* if at end of group */
				in_group = 0;
				grping_cnt++;				/* if grp end coincides with gp start */
				if(grping[grping_cnt]==n) { /* skip a grping value */
					in_group = 1;
					grping_cnt++;
				}
			}
		}
	}
	if(grping!=NULL && grping[0] != -1) {
		printout_grouping_structure(n,grping,&grping_cnt,&in_grp,hfsetcnt,dz);
	}
	display_virtual_time(dz->tempsize,dz);
	return(FINISHED);
}

/************************** GEN_MIDI_OUTPUT **************************/

int gen_midi_output(int hfsetcnt,double **hfset,int *hfsetlen,int *grping,dataptr dz)
{
	int n, j, grping_cnt = 0;
//	int in_group = 0, in_grp = 0;
	int in_group = 0;

	for(n=0;n<hfsetcnt;n++) {
		errstr[0] = ENDOFSTR;
		for(j=0;j<hfsetlen[n];j++)
			set_midi_to_print((int)hfset[n][j]);
		printout_pitchset(dz);
		if(grping!=NULL && grping[0] != -1) {
			if(grping[grping_cnt]==n) {
				in_group = 1;
				grping_cnt++;
			} else if(grping[grping_cnt]==-n) {	/* if at end of group, or not a grouped chord */
				in_group = 0;
				grping_cnt++;				/* if grp end coincides with gp start */
				if(grping[grping_cnt]==n) { /* skip a grping value */
					in_group = 1;
					grping_cnt++;
				}
			}
		}
	}
	display_virtual_time(dz->tempsize,dz);
	return(FINISHED);
}

/************************** PRINT_GROUPING_STRUCTURE **************************/

void print_grouping_structure(int n,int *grping,int *grping_cnt,int *in_grp,int hfsetcnt)
{
	if(grping[*grping_cnt]==-n && (n!=0)) {	  	/* if end of group marked */
		*in_grp = 0;
		print_outmessage_flush("))- - - - - - -\n");
		if(grping[(*grping_cnt)+1]==n) {		/* if start of group ALSO marked */
			*in_grp = 1;
			print_outmessage_flush("-------------((\n");
		}
	} else if(grping[*grping_cnt]==n) {			/* if start of group marked */
		*in_grp = 1;
		print_outmessage_flush("-------------((\n");
	} else if(!(*in_grp)) {						/* not inside a group */
		print_outmessage_flush("-----\n");
	} else if(n >= hfsetcnt) {					/* inside a group at end of whole pass */
		print_outmessage_flush("))- - - - - - -\n");
	}
}

/************************** PRINTOUT_GROUPING_STRUCTURE **************************/

void printout_grouping_structure(int n,int *grping,int *grping_cnt,int *in_grp,int hfsetcnt,dataptr dz)
{
	if(grping[*grping_cnt]==-n && (n!=0)) {	  	/* if end of group marked */
		*in_grp = 0;
		print_outmessage_flush("))- - - - - - -\n");
		fprintf(dz->fp,"))- - - - - - -\n");
		if(grping[(*grping_cnt)+1]==n) {		/* if start of group ALSO marked */
			*in_grp = 1;
			print_outmessage_flush("-------------((\n");
			fprintf(dz->fp,"-------------((\n");
		}
	} else if(grping[*grping_cnt]==n) {			/* if start of group marked */
		*in_grp = 1;
		print_outmessage_flush("-------------((\n");
		fprintf(dz->fp,"-------------((\n");
	} else if(!(*in_grp)) {						/* not inside a group */
		print_outmessage_flush("-----\n");
		fprintf(dz->fp,"-----\n");
	} else if(n >= hfsetcnt) {					/* inside a group at end of whole pass */
		print_outmessage_flush("))- - - - - - -\n");
		fprintf(dz->fp,"))- - - - - - -\n");
	}
}

/************************** SET_PITCH_TO_PRINT **************************/

void set_pitch_to_print(int midi)
{
	int oct =  (midi/12)-5;
	char temp[12];
	midi = midi%12;
	switch(midi) {
	case(0):	sprintf(temp,"C%d  ",oct);	break;
	case(1):	sprintf(temp,"C#%d ",oct);	break;
	case(2):	sprintf(temp,"D%d  ",oct);	break;
	case(3):	sprintf(temp,"Eb%d ",oct);	break;
	case(4):	sprintf(temp,"E%d  ",oct);	break;
	case(5):	sprintf(temp,"F%d  ",oct);	break;
	case(6):	sprintf(temp,"F#%d ",oct);	break;
	case(7):	sprintf(temp,"G%d  ",oct);	break;
	case(8):	sprintf(temp,"Ab%d ",oct);	break;
	case(9):	sprintf(temp,"A%d  ",oct);	break;
	case(10):	sprintf(temp,"Bb%d ",oct);	break;
	case(11):	sprintf(temp,"B%d  ",oct);	break;
	}
	if(oct>=0)
		strcat(temp," ");
	strcat(errstr,temp);
}

/************************** SET_MIDI_TO_PRINT **************************/

void set_midi_to_print(int midi)
{
	char temp[12];
	sprintf(temp,"%d  ",midi);
	strcat(errstr,temp);
}

/************************** PRINT_PITCHSET **************************/

void print_pitchset(void)
{
	char temp[12];
	sprintf(temp,"\n");
	strcat(errstr,temp);
	print_outmessage_flush(errstr);
}

/************************** PRINTOUT_PITCHSET **************************/

void printout_pitchset(dataptr dz)
{
	char temp[12];
	sprintf(temp,"\n");
	strcat(errstr,temp);
	print_outmessage_flush(errstr);
	fprintf(dz->fp,"%s",errstr);
}

/************************** SORT_BY_CHORD ***************************
 *
 *	Sort size-ordered then pitch-ordered sets, by equivalence of chords they make up.
 */

int sort_by_chord(double **hfset,int *hfsetlen,double *intv,int *hfsetcnt,int *grping,dataptr dz)
{
	int a, b, m, n, j, lastmatch, was_a_match;
	double *tempoint;
	int grping_cnt = 0, matching_intervals = 0;

	for(n=0;n<(*hfsetcnt)-1;n++) {		 			
		for(j = 0; j<hfsetlen[n]-1; j++)
			intv[j] = hfset[n][j+1] - hfset[n][j];			/* set up intervals of first set as a comparison set */
		lastmatch = n;
		was_a_match = 0;
		for(m = n+1; m<=*hfsetcnt; m++) {
			if(m==*hfsetcnt || hfsetlen[m] != hfsetlen[n]){	/* if got to end of group-of-sets of same size */

				if(was_a_match) {							/* if a matching-group has just been put together */
					for(a=n;a<lastmatch;a++) {
						for(b = a+1; b<=lastmatch; b++) {	/* sort matching chords into pitch-ascending order */
							if(hfset[b][0] < hfset[a][0]) {
								tempoint = hfset[a];
								hfset[a] = hfset[b];
								hfset[b] = tempoint;
							}
						}
					}	
					if(grping!=NULL && n!=0) {
						grping[grping_cnt++] = n;				/* ....mark group start */
						grping[grping_cnt++] = -(lastmatch+1);		/* mark group end */
					}
					if(lastmatch != m-1) {					/* if match-group didn't end at end of last samesize-group */	
						n = lastmatch+1;					/* baktrak n to position AFTER END of matching set */
						for(j = 0; j<hfsetlen[n]-1; j++)	/* set this set's intervals as comparison intervals */
							intv[j] = hfset[n][j+1] - hfset[n][j];	
						m = n;								/* reset m to run over sets after n */
						lastmatch = n;
						was_a_match = 0;					/* search for further matching sets */
						continue;
															/* ELSE last match ended at end of group-of-sets */
					} else if(m < (*hfsetcnt)-1) {			/* SO, if we're not at last set */
						for(j = 0; j<hfsetlen[m]-1; j++)	/* establish interval comparison set for new setsize */	
							intv[j] = hfset[m][j+1] - hfset[m][j];	
						lastmatch = m;						/* reset the base pointer of matching sets */
						was_a_match = 0;
						n = m;								/* skip n-index to base of next group samesize sets */
					}
				} else {									/* matching set not been made in this pass */
					m = *hfsetcnt;							/* force n to advance */
				}
			} else {
				matching_intervals = 1;						/* compare samesize (sorted) sets for matching intervals */
				for(j = 0; j<hfsetlen[m]-1; j++) {			
					if (!flteq(intv[j],hfset[m][j+1] - hfset[m][j])) {
						matching_intervals = 0;
						break;
					}
				}
				if(matching_intervals) {					/* if all intervals match */
					was_a_match = 1;						/* note tha a match has been found in this pass */
					lastmatch++;							/* incr pointer to last match position */
					if(m != lastmatch) {					/* if the matching set is not AT this position */
						tempoint = hfset[lastmatch];		/* move it to that position */
						hfset[lastmatch] = hfset[m];
						hfset[m] = tempoint;
					}
				}
			}
		}
	}
	if(dz->iparam[HP1_SORT1] == CHORD_1) {
		for(b = 0; b < grping_cnt; b+=2) {
			m = grping[b] + 1;
			n = -(grping[b+1]);
			j = n - m;
			while(n < *hfsetcnt)
				hfset[m++] = hfset[n++];
			*hfsetcnt -= j;
		}
		grping_cnt = 0;
		grping[0] = -1;
	}
	return grping_cnt;
}

/************************** SORT_BY_PITCHCLASS_EQUIVALENCE ***************************
 *
 *	Sort size-ordered THEN pitch-ordered sets, by pitch-class equivalence of root note in each size-group.
 */


int sort_by_pitchclass_equivalence
	(double **hfset,int *hfsetlen,double *pich,int hfsetcnt,int *grping,int *intcnt,
	double *intv,int *intvcnt,double *intv2,int *intvcnt2,dataptr dz)
{
	int j, k, m, n;
	int in_grp = 0, grping_cnt = 0, matching_pitches, setlen;

	n = 0;
	setlen = hfsetlen[n];
	for(j = 0; j<setlen; j++)
		pich[j] = fmod(hfset[n][j],12.0);			/* set up pitches of first set, as a comparison set */
	for(m=1;m<hfsetcnt;m++) {
		if(hfsetlen[m] != setlen) {
			if(in_grp) {
				grping[grping_cnt++] = -m;			/* mark end of group */
				in_grp = 0;
			}
			if(intcnt != NULL) {
				if(dz->vflag[HP1_DENS] == DENSE_SORT)
					sort_by_density_regardless_of_size(hfset,intv,intvcnt,intv2,intvcnt2,n,m,setlen,1);
				else
					sort_by_maxintsize_and_density(hfset,n,m,setlen,0);
			}
			n = m;
			setlen = hfsetlen[n];
			for(j = 0; j<setlen; j++)
				pich[j] = fmod(hfset[n][j],12.0);	/* set up pitches of new comparison set */
			continue;
		}
		matching_pitches = 1;			   			/* no pitch-class duplication in sets */
		for(j=0;j<hfsetlen[m];j++) {
			matching_pitches = 0;
			for(k=0;k<hfsetlen[m];k++) {
				if(flteq(fmod(hfset[m][j],12.0),pich[k])) {
					matching_pitches = 1;
					break;
				}
			}
			if(!matching_pitches)
				break;
		}
		if(matching_pitches) {
			if(!in_grp) {
				grping[grping_cnt++] = (m-1);		/* mark start of group */
				in_grp = 1;
			}
		} else {
			if(in_grp) {
				grping[grping_cnt++] = -m;			/* mark end of group */
				in_grp = 0;
			}
			if(intcnt != NULL) { 
				if(dz->vflag[HP1_DENS] == DENSE_SORT)
					sort_by_density_regardless_of_size(hfset,intv,intvcnt,intv2,intvcnt2,n,m,setlen,1);
				else
					sort_by_maxintsize_and_density(hfset,n,m,setlen,0);
			}
			n = m;
			setlen = hfsetlen[n];
			for(j = 0; j<setlen; j++)
				pich[j] = fmod(hfset[n][j],12.0);	/* set up pitches of new comparison set */
			continue;
		}
	}
	if(intcnt != NULL) { 
		if(dz->vflag[HP1_DENS] == DENSE_SORT)
			sort_by_density_regardless_of_size(hfset,intv,intvcnt,intv2,intvcnt2,n,m,setlen,1);
		else
			sort_by_maxintsize_and_density(hfset,n,m,setlen,0);
	}
	return(grping_cnt);
}

/************************** SORT_BY_REFERENCE_NOTE_EQUIVALENCE ***************************
 *
 *	Sort pitch-ordered by pitch-class equivalence of root note.
 */

int sort_by_reference_note_equivalence(double **hfset,int *hfsetlen,int hfsetcnt,int *grping,dataptr dz)
{
	int j, k, m, n;
	int in_grp = 0, grping_cnt = 0, setlen, end, note = 0;
	double *tempoint, thispitch, thatpitch;

	n = 0;
	m = n+1;
	while(m < hfsetcnt) {
		setlen = hfsetlen[n];
		end = setlen - 1;
		switch(dz->iparam[HP1_SORT1]) {
		case(ROOT):		note = 0; 	break;
		case(TOP):		note = end;	break;
		}
		while(hfsetlen[m] == setlen) {			/* find sets of same size */
			if(++m >= hfsetcnt)
				break;
		}
		for(j=n;j<m-1;j++) {					/* sort all these samesize sets into root pitchclass order */
			if(dz->process == HF_PERM1)
				thispitch = fmod(hfset[j][note],12.0);
			else
				thispitch = hfset[j][note];
			for(k=j+1;k<m;k++) {
				if(dz->process == HF_PERM1)
					thatpitch = fmod(hfset[k][note],12.0);
				else
					thatpitch = hfset[k][note];
				if(thatpitch < thispitch) {
					tempoint = hfset[k];
					hfset[k] = hfset[j];
					hfset[j] = tempoint;
				}
			}
		}
		k = n;										/* sort all these into same-root sets */
		j = k+1;
		if(j >= hfsetcnt)
			break;
		in_grp = 0;
		while(j < m) {								
			if(dz->process == HF_PERM1) {
				thispitch = fmod(hfset[k][note],12.0);
				thatpitch = fmod(hfset[j][note],12.0);
			} else {
				thispitch = hfset[k][note];
				thatpitch = hfset[j][note];
			}
			while(flteq(thatpitch,thispitch)) {
				if(!in_grp) {
					grping[grping_cnt++] = k;		/* mark start of group */
					in_grp = 1;
				}
				if(++j >= m)
					break;
				if(dz->process == HF_PERM1) {
					thispitch = fmod(hfset[k][note],12.0);
					thatpitch = fmod(hfset[j][note],12.0);
				} else {
					thispitch = hfset[k][note];
					thatpitch = hfset[j][note];
				}
			}
			if(in_grp) {							/* sort all these into density:tessitura order */
				grping[grping_cnt++] = -j;			/* mark end of group */
				sort_by_maxint(hfset,k,j,setlen,dz);	
				in_grp = 0;							/* sort same-root sets into interval size and stack order */
			}
			k = j;
			j = k+1;
		}		
		n = m;
		m = n+1;
	}
	return(grping_cnt);
}

/********************************* SORT_BY_MAXINT *********************************
 *
 *	Sort pitch-ordered sets of FIXED SIZE by size of largest interval, then by interval density
 */

void sort_by_maxint(double **hfset,int n,int m,int setlen,dataptr dz)
{
	int j, k;
	double j_maxint, k_maxint, *tempoint;
	int end = setlen - 1;
	int in_grp;
	
	for(j=n;j<m-1;j++) {					/* sort all by size of largest interval */
		j_maxint = hfset[j][end] - hfset[j][0];
		for(k=j+1;k<m;k++) {
			k_maxint = hfset[k][end] - hfset[k][0];
			switch(dz->vflag[HP1_SPAN]) {
			case(MAXSPAN_FIRST):
				if(k_maxint < j_maxint) {
					tempoint = hfset[j];
					hfset[j] = hfset[k];
					hfset[k] = tempoint;
					j_maxint = k_maxint;
				}
				break;
			case(MAXSPAN_LAST):
				if(k_maxint > j_maxint) {
					tempoint = hfset[j];
					hfset[j] = hfset[k];
					hfset[k] = tempoint;
					j_maxint = k_maxint;
				}
				break;
			}
		}
	}
	k = n;
	j = k+1;
	in_grp = 0;
	while(j < m) {							/* sort largest-interval sets  by internal interval arrangement */
		in_grp = 0;
		k_maxint = hfset[k][end] - hfset[k][0];
		j_maxint = hfset[j][end] - hfset[j][0];
		while(flteq(j_maxint,k_maxint)) {
			in_grp = 1;
			if(++j >= m)
				break;
			j_maxint = hfset[j][end] - hfset[j][0];
		}
		if(in_grp) {
			switch(dz->vflag[HP1_DENS]) {
			case(DENSE_SORT): 	sort_by_density(hfset,k,j,setlen,0);			break;
			case(OTHER_SORT):	sort_by_interval_stacking(hfset,k,j,setlen,0);	break;
			}
			sort_by_octave_equivalence(hfset,n,m,setlen);
		}
		k = j;
		j = k+1;
	}
}

/********************************* SORT_BY_DENSITY *********************************
 *
 *	Sort pitch-ordered sets of FIXED SIZE , WITH FIXED MAX INTERVAL by density...
 *  measure the deviation from the mean inteval size between adjacent notes.
 *  Minimum deviation = minimum density.
 */

void sort_by_density(double **hfset,int n,int m,int setlen,int lowest)
{
	int j, k, x;
	double j_deviation, k_deviation, y, average_intsize;
	double *tempoint;

	average_intsize = (hfset[n][setlen-1] - hfset[n][0])/(setlen-1);

	for(j=n;j<m-1;j++) {
		j_deviation = 0;
		for(x = 1; x < setlen; x++) {
			y = hfset[j][x] - hfset[j][x-1];
			y = fabs(y - average_intsize);
			j_deviation += y;		/* no advantage in taking rms */
		}
		for(k=j+1;k<m;k++) {
			k_deviation = 0;
			for(x = 1; x < setlen; x++) {
				y = hfset[k][x] - hfset[k][x-1];
				y = fabs(y - average_intsize);
				k_deviation += y;	/* no advantage in taking rms */
			}
			if(lowest) {
				if(k_deviation < j_deviation) {
					tempoint = hfset[k];
					hfset[k] = hfset[j];
					hfset[j] = tempoint;
					j_deviation = k_deviation;
				}
			} else {
				if(k_deviation > j_deviation) {
					tempoint = hfset[k];
					hfset[k] = hfset[j];
					hfset[j] = tempoint;
					j_deviation = k_deviation;
				}
			}
		}
	}
}

/********************************* SORT_BY_MAXINTSIZE_AND_DENSITY *********************************
 *
 *	Sort pitch-ordered sets of FIXED SIZE , NOT of FIXED MAX INTERVAL by density...
 *  measure the deviation from the mean inteval size between adjacent notes.
 *  Minimum deviation = minimum density.
 */

void sort_by_maxintsize_and_density(double **hfset,int n,int m,int setlen,int lowest)
{
	int j, k, end = setlen - 1, in_grp;
	double j_intsize, k_intsize;
	double *tempoint;

	for(j=n;j<m-1;j++) {				  		/* sort by max-intsize */
		j_intsize = hfset[j][end] - hfset[j][0];
		for(k=j+1;k<m;k++) {
			k_intsize = hfset[k][end] - hfset[k][0];
			if(k_intsize < j_intsize) {
				tempoint = hfset[k];
				hfset[k] = hfset[j];
				hfset[j] = tempoint;
				j_intsize = k_intsize;
			}
		}
	}
	j = n;
	k = j+1;
	while(k < m) {
		j_intsize = hfset[j][end] - hfset[j][0];
		in_grp = 0;
		while(flteq(hfset[k][end] - hfset[k][0],j_intsize)) {
			in_grp = 1;
			if(++k >= m)
				break;
		}
		if(in_grp)
			sort_by_density(hfset,j,k,setlen,lowest);
		j = k;
		k = j+1;
	}
}

/********************************* SORT_BY_OCTAVE_EQUIVALENCE *********************************
 *
 *	Sort pitch-ordered sets of FIXED SIZE by equivalence at octave transposition, putting lower tessitura item first.
 */

void sort_by_octave_equivalence
		(double **hfset,int n,int m,int setlen)
{
	int i, j, j_base, j_adj, k, oct_equivalence;
	double *tempoint, int_diff;

	for(j=n;j<m-1;j++) {
		j_base = j;
		j_adj = j+1;								/* if sets are separate by multiple of an octave */
		for(k=j+1;k<m;k++) {
			if(flteq(fmod((int_diff = hfset[k][0] - hfset[j][0]),12.0),0.0)) {
				oct_equivalence = 1;
				for(n=1;n<setlen;n++) {
					if(!flteq(hfset[k][n] - hfset[j][n],int_diff)) {
						oct_equivalence = 0;
						break;
					}
				}
				if(oct_equivalence) {				/* if oct-equivalent */
					if(k != j_adj) {				/* if NOT adjacent */
						tempoint = hfset[j_adj];
						hfset[j_adj] = hfset[k];	/* move oct-equiv set to be adjacent to j */
						hfset[k] = tempoint;
					}
					i = j;
					while(hfset[j_adj][0] < hfset[i][0]) {
						if(--i < j_base)
							break;
					}
					i++;
					if(i<=j) {
						tempoint = hfset[i];
						hfset[i] = hfset[j_adj];	/* move lower register item to correct place in group */
						hfset[j_adj] = tempoint;
					}
					j++;							/* advance to new oct-equiv position */
					j_adj++;
				}
			}
		}
	}
}

/********************************* SORT_BY_INTERVAL_STACKING *********************************
 *
 *	Sort pitch-ordered sets of FIXED SIZE , WITH FIXED MAX INTERVAL by sequnce of pitches within...
 *  NORMALLY - start with chord having the lowest pitches (on average)
 *  ... if 'highest-first' flag set, start with chord with highest pitches (on average)
 */

void sort_by_interval_stacking(double **hfset,int n,int m,int setlen,int highest_first)
{
	int j, k, x;
	double j_height, k_height;
	double *tempoint;

	for(j=n;j<m-1;j++) {
		j_height = 0;
		for(x = 1; x < setlen; x++)
			j_height += hfset[j][x] - hfset[j][0];
		for(k=j+1;k<m;k++) {
			k_height = 0;
			for(x = 1; x < setlen; x++)
				k_height += hfset[k][x] - hfset[k][0];
			if(highest_first) {
				if(k_height > j_height) {
					tempoint = hfset[k];
					hfset[k] = hfset[j];
					hfset[j] = tempoint;
					j_height = k_height;
				}
			} else {
				if(k_height < j_height) {
					tempoint = hfset[k];
					hfset[k] = hfset[j];
					hfset[j] = tempoint;
					j_height = k_height;
				}
			}
		}
	}
}

/********************************* ELIMINATE_OCT_DUPLICATED_SETS *********************************
 *
 *	Sort ordered-by-size, then pitch-ordered sets, eliniating stes dupliated at 8vas.
 */

void eliminate_oct_duplicated_sets(double **hfset,int *hfsetcnt,int *hfsetlen,int *grping,int *grping_cnt,int type)
{
	int a, b, n, m, j, k, oct_equivalence, setlen;
	double *tempoint, int_diff;

	n=0;
	m = n+1;
	setlen = hfsetlen[n];
	while(n < *hfsetcnt) {
		while(hfsetlen[m] == setlen) {							/* find equal-size sets */
			if(++m >= *hfsetcnt)
				break;
		}
		if(m == n+1) {
			n++;
			if((m = n+1) >= *hfsetcnt)
				break;
			setlen = hfsetlen[n];
			continue;
		}
		for(j=n;j<m-1;j++) {
			for(k = j+1;k<m;k++) {
				if(flteq(fmod((int_diff = hfset[k][0] - hfset[j][0]),12.0),0.0)) {
					oct_equivalence = 1;
					for(a=1;a<setlen;a++) {
						if(!flteq(hfset[k][a] - hfset[j][a],int_diff)) {
							oct_equivalence = 0;
							break;
						}
					}
					if(oct_equivalence) {					/* if oct-equivalent */
						if(type == TOPNOTE) {
							if(hfset[k][0] > hfset[j][0]) {
								tempoint = hfset[j];
								hfset[j] = hfset[k];		/* keep the higher-register item */
								hfset[k] = tempoint;
							}
						} else {
							if(hfset[k][0] < hfset[j][0]) {
								tempoint = hfset[j];
								hfset[j] = hfset[k];		/* keep the lower-register item */
								hfset[k] = tempoint;
							}
						}
						for(a=k,b=k+1;b<*hfsetcnt;a++,b++)	/* delete unwanted set */
							hfset[a] = hfset[b];
						if(grping != NULL) {				/* reposition grouping marks */
							for(a=0;a<*grping_cnt;a++) {		
															/* if start or end of short group */ 	
								if(((grping[a] == k)   && (a+1 < *grping_cnt) && (grping[a+1] >= -(k+2)))
								|| ((grping[a] == k-1) && (a+1 < *grping_cnt) && (grping[a+1] >= -(k+1)))) {
									if(a+2 < *grping_cnt) {	/* eliminate both */
										for(b = a+2; b < *grping_cnt;b++)
											grping[b-2] = grping[b];
									}
									*grping_cnt -= 2;
									a -= 1;					/* step back to resume parse */
								}
								else if(grping[a] > k)		/* decrement all markers beyond */
									grping[a]--;
								else if(grping[a] < -k)
									grping[a]++;
							}
						}
						(*hfsetcnt)--;						/* decrement all counters */
						m--;
						k--;
					}
				}
			}
		}
		n = m;
		if((m = n+1) >= *hfsetcnt)
			break;
		setlen = hfsetlen[n];
	}
}

/********************************* SORT_BY_DENSITY_REGARDLESS_OF_SIZE *********************************
 *
 *	Sort pitch-ordered sets of FIXED LENGTH . by density, regardless of MAX INTERVAL...
 *	.... sort on basis of maximum number of minsize intervals.
 */

void sort_by_density_regardless_of_size
	(double **hfset,double *interv,int *intervcnt,double *interv2,int *intervcnt2,int n,int m,int setlen,int densest_first)
{
	int a, j, k, x, q, do_swap, equivalent;
	double tempdb, *tempoint;
	int intsetlen1 = setlen - 1;
	int intsetlen2 = setlen - 1;
	int tempoi, end = setlen - 1;
	double *intv = interv, *intv2 = interv2;
	int *intvcnt = intervcnt, *intvcnt2 = intervcnt2, *tempii;

	for(j=n;j<m-1;j++) {
		intsetlen1 = setlen - 1;
		for(x=1;x<setlen;x++) {
			intv[x-1] = hfset[j][x] - hfset[j][x-1];   	/* find each interval and count it */
			intvcnt[x-1] = 1;
		}
		for(x=0;x<intsetlen1-1;x++) {
			for(q=x+1;q<intsetlen1;q++) {
				if(flteq(intv[q],intv[x])) {			/* compare intervals */
					intvcnt[x]++;						/* if same, incr count of first */
					a = q;
					while(a< intsetlen1-1)	{			/* and eliminate other from set */
						intv[a] = intv[a+1];
						a++;
					}
					intsetlen1--;
					q--;
				}
			}
		}
		for(x=0;x<intsetlen1-1;x++) {					/* sort intervals by size */
			for(q=x+1;q<intsetlen1;q++) {
				if(intv[q] < intv[x]) {
					tempdb = intv[q];
					intv[q] = intv[x];
					intv[x] = tempdb;
					tempoi = intvcnt[q];
					intvcnt[q] = intvcnt[x];
					intvcnt[x] = tempoi;
				}
			}
		}

		for(k=j+1;k<m;k++) {
			intsetlen2 = setlen - 1;
			for(x=1;x<setlen;x++) {
				intv2[x-1] = hfset[k][x] - hfset[k][x-1];
				intvcnt2[x-1] = 1;
			}
			for(x=0;x<intsetlen2-1;x++) {
				for(q=x+1;q<intsetlen2;q++) {
					if(flteq(intv2[q],intv2[x])) {
						intvcnt2[x]++;
						a = q;
						while(a < intsetlen2-1) {
							intv2[a] = intv2[a+1];
							a++;
						}
						intsetlen2--;
						q--;
					}
				}
			}
			for(x=0;x<intsetlen2-1;x++) {
				for(q=x+1;q<intsetlen2;q++) {
					if(intv2[q] < intv2[x]) {
						tempdb = intv2[q];
						intv2[q] = intv2[x];
						intv2[x] = tempdb;
						tempoi = intvcnt2[q];
						intvcnt2[q] = intvcnt2[x];
						intvcnt2[x] = tempoi;
					}
				}
			}
			do_swap = 0;
			equivalent = 1;
			if(densest_first) {
				for(a=0;a<min(intsetlen1,intsetlen2);a++) {
					if(intv2[a] > intv[a]) {
						equivalent = 0;
						break;
					} else if(intv2[a] < intv[a]) {
						equivalent = 0;
						do_swap = 1;
						break;
					} else {
						 if(intvcnt2[a] > intvcnt[a]) {
							equivalent = 0;
							do_swap = 1;
							break;
						} else if(intvcnt2[a] < intvcnt[a]) {
							equivalent = 0;
							break;
						}
					}
				}
			} else {
				for(a=0;a<min(intsetlen1,intsetlen2);a++) {
					if(intv2[a] < intv[a]) {
						equivalent = 0;
						break;
					} else if(intv2[a] > intv[a]) {
						equivalent = 0;
						do_swap = 1;
						break;
					} else {
						if(intvcnt2[a] < intvcnt[a]) {
							equivalent = 0;
							do_swap = 1;
							break;
						} else if(intvcnt2[a] > intvcnt[a]) {
							equivalent = 0;
							break;
						}
					}
				}
			}
			if((!do_swap) && equivalent) {	/* swap if total range is greater */
				if(hfset[j][end] - hfset[j][0] > hfset[k][end] - hfset[k][0])
					do_swap = 1;
				else if(hfset[j][0] > hfset[k][0])	/* swap if root is lower */
					do_swap = 1;
			}
			if(do_swap) {
				tempoint = hfset[k];
				hfset[k] = hfset[j];
				hfset[j] = tempoint;
				tempoint = intv;
				intv = intv2;
				intv2 = tempoint;
				tempii = intvcnt ;
				intvcnt = intvcnt2;
				intvcnt2 = tempii;
			}
		}
	}
}

/******************************* SETUP_NAMING ******************************/

int setup_naming(char **thisfilename,dataptr dz)
{
	int innamelen = strlen(dz->wordstor[0]);
	int numlen = 5;
// FEB 2010 TW
	if((*thisfilename = (char *)malloc((innamelen + numlen + 10) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/********************************** CREATE_OUTFILE_NAME ********************************/

int create_outfile_name(int ccnt,char *thisfilename,dataptr dz)
{
	strcpy(thisfilename,dz->wordstor[0]);
// FEB 2010 TW
	if(!sloom)
		insert_separator_on_sndfile_name(thisfilename,1);
	insert_new_number_at_filename_end(thisfilename,ccnt,1);
	return(FINISHED);
}

/********************************** GEN_NOTE ********************************/

void gen_note(int maxjitter,double *tabstart,double *tabend,double **tabmin, double **tabmax,double **hfset,
	double tabscale,double sintabsize,int splicelen, double *sintab,int n, int j)
{
	int jitter, initial_phase, i;
	double *this_tabend, *startsplice_end, *endsplice_start, *tab; 
	double sintabpos, sintabincr;
	int intpos, nxtpos;
	double frac, vallo, valhi, valdif;
	
	jitter = (int)round(drand48() * maxjitter);
	tab = tabstart + jitter;
	this_tabend = tabend + jitter;
	*tabmin = min(tab,*tabmin);
	*tabmax = max(this_tabend,*tabmax);
	tab++;			/* 1st val = 0 */
	/* START POSITION IN TABLE TO READ TO GENERATE SOUND */
	initial_phase = (int)round(drand48() * HFP_TABSIZE);
	sintabincr = (miditohz(hfset[n][j]) * tabscale);
	sintabpos = sintabincr + initial_phase;
	sintabpos = fmod(sintabpos,sintabsize);
	startsplice_end = tab + splicelen;
	endsplice_start = this_tabend - splicelen;
	/* START OF EACH CONSTITUENT MUST BE SPLICED */
	i = 0;
	while(tab < startsplice_end) {		
		intpos = (int)floor(sintabpos);
		nxtpos = intpos+1;
		frac = sintabpos - (double)intpos;
		vallo = sintab[intpos];
		valhi = sintab[nxtpos];
		valdif= valhi - vallo;
		*tab += (vallo + (valdif * frac)) * (i/(double)splicelen);
		i++;
		sintabpos += sintabincr;
		sintabpos = fmod(sintabpos,sintabsize);
		tab++;
	}
	while(tab < endsplice_start) {		
		intpos = (int)floor(sintabpos);
		nxtpos = intpos+1;
		frac = sintabpos - (double)intpos;
		vallo = sintab[intpos];
		valhi = sintab[nxtpos];
		valdif= valhi - vallo;
		*tab += vallo + (valdif * frac);
		sintabpos += sintabincr;
		sintabpos = fmod(sintabpos,sintabsize);
		tab++;
	}
	/* END OF EACH CONSTITUENT MUST BE SPLICED */
	i = splicelen - 1;
	while(tab < this_tabend) {		
		intpos = (int)floor(sintabpos);
		nxtpos = intpos+1;
		frac = sintabpos - (double)intpos;
		vallo = sintab[intpos];
		valhi = sintab[nxtpos];
		valdif= valhi - vallo;
		*tab += (vallo + (valdif * frac)) * (i/(double)splicelen);
		i--;
		sintabpos += sintabincr;
		sintabpos = fmod(sintabpos,sintabsize);
		tab++;
	}
}

/********************************** NEXT_FILE ********************************/

int next_file(int *samps_written,char *thisfilename,int *ccnt,unsigned int *space_needed, 
int *samp_space_available,dataptr dz)
{
	int exit_status;
				/* FLUSH BUFS */
	*samps_written += dz->iparam[HP1_GAP_SIZE];
	while(*samps_written >= dz->buflen) {
		if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
			return(exit_status);
		*samps_written -= dz->buflen;
	}	
	if(*samps_written > 0) {
		if((exit_status = write_samps(dz->sampbuf[0],*samps_written,dz))<0)
			return(exit_status);
	}	
				/* CLOSE FILE */
	dz->outfiletype  = SNDFILE_OUT;			/* allows header to be written  */
	if((exit_status = headwrite(dz->ofd,dz))<0)
		return(exit_status);
	dz->process_type = OTHER_PROCESS;		/* restore true status */
	dz->outfiletype  = NO_OUTPUTFILE;		/* restore true status */
	superzargo += dz->total_samps_written;	/* accumulator of total samples written */
	if((exit_status = reset_peak_finder(dz))<0)
		return(exit_status);
	if(sndcloseEx(dz->ofd) < 0) {
		fprintf(stdout,"WARNING: Can't close output soundfile %s\n",thisfilename);
		fflush(stdout);
	}
	dz->ofd = -1;
	(*ccnt)++;
				/* OPEN NEW FILE */
	if((exit_status = create_outfile_name(*ccnt,thisfilename,dz))<0)
		return(exit_status);
	dz->process_type = UNEQUAL_SNDFILE;	/* allow sndfile to be created */
	if((exit_status = create_sized_outfile(thisfilename,dz))<0) {
		fprintf(stdout, "WARNING: Soundfile %s already exists.\n",thisfilename);
		fflush(stdout);
		dz->process_type = OTHER_PROCESS;
		dz->ofd = -1;
		return(PROGRAM_ERROR);
	}							
	*space_needed -= dz->total_samps_written;
	if((unsigned int)dz->outfilesize < *space_needed) {
		 sprintf(errstr,"Insufficient space on the hard disk to store the next output file.\n");
		 return(MEMORY_ERROR);
	}
				/* RESET BUFFERS */
	dz->total_samps_written = 0;
	memset((char *)dz->sampbuf[0],0,(size_t)dz->buflen * sizeof(float));
	*samp_space_available = dz->buflen;
	*samps_written = 0;
//NEW TW
	return(FINISHED);
}

/********************************** GEN_DP_OUTPUT ********************************/

int gen_dp_output(dataptr dz)
{
	int exit_status;
	float *obuf = dz->sampbuf[0];
	double *tabend, *tab = (double *)dz->sampbuf[3];
	double *accumtab = (double *)dz->sampbuf[2];
	double *perm = dz->parray[1];
	double tabscale = (double)HFP_TABSIZE/dz->infile->srate;
	double *sintab = (double *)dz->sampbuf[1];
	float  *intab = dz->sampbuf[1];
	double sintabsize, pad;
	int intabsize;
	int samps_written = 0, samp_space_available, samps_to_write;
	int note_being_permed, noteno, index_in_new_perm, index_in_last_perm, i, j, no_of_perms, permcounter, transpos_cnt;
	int maxjitter = (int)round(drand48() * HFP_MAXJITTER * MS_TO_SECS * (double)dz->infile->srate);
	unsigned int element_byte_len = dz->iparam[DP_DUR] * sizeof(double);
	unsigned int element_full_len = element_byte_len + (maxjitter * sizeof(double));
	double *tabmin = accumtab, *tabmax = accumtab + dz->iparam[DP_DUR] + maxjitter;
	int splicelen = (int)round(HP1_SPLICELEN * MS_TO_SECS * (double)dz->infile->srate);
	double ampmult = 1.0/(double)(dz->iparam[DP_CYCCNT] + 1);
	double *permset, thistranspos;
	int accumtabstart, accumtabend, a, b;
	int jitter, endindex_of_last_perm, endindex_of_last_perm_but_one, endindex_in_new_perm = 0, len, thislen;

/* JUNE 2004 */	
sprintf(errstr,"ERROR: This program is currently malfunctioning.\n");
return(PROGRAM_ERROR);
/* JUNE 2004 */	
	switch(dz->process) {
	case(DEL_PERM):		
		sintabsize = (double)HFP_TABSIZE; 
		ampmult *= MAXSAMP;
		break;
	case(DEL_PERM2):	
		intabsize  = dz->iparam[DP_DUR]; 
		break;
	}

	dz->tempsize = dz->iparam[DP_DUR] * (dz->iparam[DP_NOTECNT] + dz->iparam[DP_CYCCNT]);

	if((permset = 
	(double *)malloc((unsigned int)round(pow(dz->iparam[DP_PERMCNT],dz->iparam[DP_CYCCNT])) * sizeof(double))) == NULL) {
		sprintf(errstr,"No memory for permutations of notes\n");
		return(MEMORY_ERROR);
	}
	for(noteno = 0; noteno < dz->iparam[DP_NOTECNT]/* + dz->iparam[DP_CYCCNT]*/; noteno++) {
		sprintf(errstr,"Expanding note %d\n",noteno+1);
		print_outmessage_flush(errstr);
		memset(accumtab,0,element_full_len);
		for(note_being_permed = noteno, no_of_perms = 0; 
		note_being_permed >= max(0,noteno - dz->iparam[DP_CYCCNT]);
		note_being_permed--, no_of_perms++) {
			if(note_being_permed >= dz->iparam[DP_NOTECNT])
				continue;
			jitter = (int)(drand48() * maxjitter);
			accumtabstart = jitter;
			permset[0] = dz->parray[1][note_being_permed];
			endindex_of_last_perm = 0;
			endindex_of_last_perm_but_one = 0;
			for(permcounter = 0; permcounter < no_of_perms; permcounter++) {
				index_in_last_perm = endindex_of_last_perm;
				endindex_in_new_perm = ((index_in_last_perm+1) * dz->iparam[DP_PERMCNT]) - 1;				
				index_in_new_perm  = endindex_in_new_perm; 
				while(index_in_last_perm >= 0) {
					for(transpos_cnt = dz->iparam[DP_PERMCNT]-1; transpos_cnt >= 0; transpos_cnt--)
						permset[index_in_new_perm--] = permset[index_in_last_perm] + perm[transpos_cnt];
					index_in_last_perm--;
				}
				endindex_of_last_perm_but_one = endindex_of_last_perm;
				endindex_of_last_perm = endindex_in_new_perm;
			}
			tab = (double *)dz->sampbuf[3];
			if(no_of_perms > 0) {
				if((len = (int)round((double)dz->iparam[DP_DUR]/(double)(endindex_of_last_perm_but_one+1))) <= splicelen * 2) {
					sprintf(errstr,"Notes have become too short for splices to be made.\n");
					return(GOAL_FAILED);
				}
				for(i = 0, j = 0; i <= endindex_of_last_perm; i++, j++) {
					j %= dz->iparam[DP_PERMCNT];
					switch(dz->process) {
					case(DEL_PERM):  
						thislen = (int)round(len * dz->parray[2][j]);
						if(thislen <= splicelen * 2) {
							sprintf(errstr,"Notes have become too short for splices to be made.\n");
							return(GOAL_FAILED);
						}
						tabend = tab + thislen;
						memset((char *)tab,0,element_byte_len);
						
						gen_note2(tab,tabend,permset[i],tabscale,sintabsize,splicelen,sintab);	break;
					case(DEL_PERM2): 
						thistranspos = pow(2.0,(permset[i]/12.0));
						thislen = (int)round(len/thistranspos);
						thislen = min(thislen,len);
						if(thislen <= splicelen * 2) {
							sprintf(errstr,"Notes have become too short for splices to be made.\n");
							return(GOAL_FAILED);
						}
						tabend = tab + thislen;
						memset((char *)tab,0,element_byte_len);
						gen_note3(tab,tabend,thistranspos,intabsize,splicelen,intab);			break;
					}
					accumtabstart = accumtabstart + thislen;
					accumtabend = accumtabstart + thislen;
					pad = pow(ROLLOFF,no_of_perms);
					accumtabstart += thislen;
				}
			} else { 
				len = dz->iparam[DP_DUR];
				tabend = tab + len;
				memset(tab,0,element_byte_len);
				switch(dz->process) {
				case(DEL_PERM):  gen_note2(tab,tabend,permset[0],tabscale,sintabsize,splicelen,sintab);	break;
				case(DEL_PERM2): gen_note3(tab,tabend,permset[0],intabsize,splicelen,intab);			break;
				}
				accumtabend = accumtabstart + len;
				for(a=accumtabstart, b = 0; a < accumtabend;a++,b++)
					accumtab[a] += tab[b]; 
				accumtabstart += len;
			}
		}
		/* DO OUTPUT */
		samp_space_available = dz->buflen - samps_written;
		i = 0;
		samps_to_write = tabmax - tabmin;
		while(i < samps_to_write) {
			if(samp_space_available > samps_to_write) {
				while(i < samps_to_write)
					obuf[samps_written++] = (float)(accumtab[i++] * ampmult);
			} else {
				while(samps_written < dz->buflen)
					obuf[samps_written++] = (float)(accumtab[i++] * ampmult);
				if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
					return(exit_status);
				memset((char *)dz->sampbuf[0],0,(size_t)dz->buflen * sizeof(float));
				samps_written = 0;
				samp_space_available = dz->buflen;
			}
		}
	}
	while(samps_written >= dz->buflen) {
		if((exit_status = write_samps(dz->sampbuf[0],dz->buflen,dz))<0)
			return(exit_status);
		samps_written -= dz->buflen;
	}	
	if(samps_written > 0) {
		if((exit_status = write_samps(dz->sampbuf[0],samps_written,dz))<0)
			return(exit_status);
	}	
	return(FINISHED);
}

/********************************** GEN_NOTE2 ********************************/

void gen_note2(double *tabstart,double *tabend,double midival,
	double tabscale,double sintabsize,int splicelen, double *sintab)
{
	int initial_phase, i;
	double *startsplice_end, *endsplice_start, *tab; 
	double sintabpos, sintabincr;
	int intpos, nxtpos;
	double frac, vallo, valhi, valdif;
	
	tab = tabstart;
	*tab++ = 0;		/* 1st val = 0 */
	/* START POSITION IN TABLE TO READ TO GENERATE SOUND */
	initial_phase = (int)round(drand48() * HFP_TABSIZE);
	sintabincr = (miditohz(midival) * tabscale);
	sintabpos = sintabincr + initial_phase;
	sintabpos = fmod(sintabpos,sintabsize);
	startsplice_end = tab + splicelen;
	endsplice_start = tabend - splicelen;
	/* START OF EACH CONSTITUENT MUST BE SPLICED */
	i = 0;
	while(tab < startsplice_end) {		
		intpos = (int)floor(sintabpos);
		nxtpos = intpos+1;
		frac = sintabpos - (double)intpos;
		vallo = sintab[intpos];
		valhi = sintab[nxtpos];
		valdif= valhi - vallo;
		*tab += (vallo + (valdif * frac)) * (i/(double)splicelen);
		i++;
		sintabpos += sintabincr;
		sintabpos = fmod(sintabpos,sintabsize);
		tab++;
	}
	while(tab < endsplice_start) {		
		intpos = (int)floor(sintabpos);
		nxtpos = intpos+1;
		frac = sintabpos - (double)intpos;
		vallo = sintab[intpos];
		valhi = sintab[nxtpos];
		valdif= valhi - vallo;
		*tab += vallo + (valdif * frac);
		sintabpos += sintabincr;
		sintabpos = fmod(sintabpos,sintabsize);
		tab++;
	}
	/* END OF EACH CONSTITUENT MUST BE SPLICED */
	i = splicelen - 1;
	while(tab < tabend) {		
		intpos = (int)floor(sintabpos);
		nxtpos = intpos+1;
		frac = sintabpos - (double)intpos;
		vallo = sintab[intpos];
		valhi = sintab[nxtpos];
		valdif= valhi - vallo;
		*tab += (vallo + (valdif * frac)) * (i/(double)splicelen);
		i--;
		sintabpos += sintabincr;
		sintabpos = fmod(sintabpos,sintabsize);
		tab++;
	}
}

/********************************** GEN_NOTE3 ********************************/

//void gen_note3(double *tabstart,double *tabend,double midival,int intabsize,int splicelen, short *intab)
void gen_note3(double *tabstart,double *tabend,double midival,int intabsize,int splicelen, float *intab)
{
	int i;
	double *startsplice_end, *endsplice_start, *tab; 
	double tabpos, tabincr, effective_len;
	int intpos, nxtpos;
	double frac, vallo, valhi, valdif;
	
	tab = tabstart;
	*tab++ = 0;		/* 1st val = 0 */
	tabincr = (miditohz(midival)/miditohz(60.0));
	effective_len = (double)(tabend - tabstart);
	if(effective_len * tabincr > (double)intabsize) {
		effective_len /= tabincr;
		tabend = tabstart + (int)effective_len;
	}
	tabpos = tabincr;
	startsplice_end = tab + splicelen;
	endsplice_start = tabend - splicelen;
	/* START OF EACH CONSTITUENT MUST BE SPLICED */
	i = 0;
	while(tab < startsplice_end) {		
		intpos = (int)floor(tabpos);
		nxtpos = intpos+1;
		frac = tabpos - (double)intpos;
		vallo = intab[intpos];
		valhi = intab[nxtpos];
		valdif= valhi - vallo;
		*tab += (vallo + (valdif * frac)) * (i/(double)splicelen);
		i++;
		tabpos += tabincr;
		tab++;
	}
	while(tab < endsplice_start) {		
		intpos = (int)floor(tabpos);
		nxtpos = intpos+1;
		frac = tabpos - (double)intpos;
		vallo = intab[intpos];
		valhi = intab[nxtpos];
		valdif= valhi - vallo;
		*tab += vallo + (valdif * frac);
		tabpos += tabincr;
		tab++;
	}
	/* END OF EACH CONSTITUENT MUST BE SPLICED */
	i = splicelen - 1;
	while(tab < tabend) {		
		intpos = (int)floor(tabpos);
		nxtpos = intpos+1;
		frac = tabpos - (double)intpos;
		vallo = intab[intpos];
		valhi = intab[nxtpos];
		valdif= valhi - vallo;
		*tab += (vallo + (valdif * frac)) * (i/(double)splicelen);
		i--;
		tabpos += tabincr;
		tab++;
	}
}
