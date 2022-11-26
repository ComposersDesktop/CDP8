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
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <pnames.h>
#include <synth.h>
#include <processno.h>
#include <modeno.h>
#include <globcon.h>
#include <logic.h>
#include <filetype.h>
#include <mixxcon.h>
#include <speccon.h>
#include <flags.h>
#include <arrays.h>
#include <special.h>
#include <formants.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
#include <hfperm.h>
#include <math.h>
#include <srates.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

static int eliminate_octave_duplications(dataptr dz);
static int check_for_invalid_data(int k,dataptr dz);
static void transpose_to_lowest_oct(dataptr dz);
static void bublsort_hf(dataptr dz);
static void elim_dupls(dataptr dz);
static int create_hfpermbufs(dataptr dz);
static void establish_waveform(dataptr dz);
static int read_delperm_data(char *filename,dataptr dz);
static int read_delperm2_data(char *filename,dataptr dz);

char warnstr[2400];

/***************************** ESTABLISH_BUFPTRS_AND_EXTRA_BUFFERS **************************/

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
//	int is_spec = FALSE;
	dz->extra_bufcnt = -1;	/* ENSURE EVERY CASE HAS A PAIR OF ENTRIES !! */
	dz->bptrcnt = 0;
	dz->bufcnt  = 0;
	switch(dz->process) {
	case(HF_PERM1):			case(HF_PERM2):
		dz->extra_bufcnt = 0;	
		dz->bufcnt = 3;
		break;
	case(DEL_PERM):		
	case(DEL_PERM2):		
		dz->extra_bufcnt = 0;	
		dz->bufcnt = 4;
		break;
	default:
		sprintf(errstr,"Unknown program type [%d] in establish_bufptrs_and_extra_buffers()\n",dz->process);
		return(PROGRAM_ERROR);
	}

	if(dz->extra_bufcnt < 0) {
		sprintf(errstr,"bufcnts have not been set: establish_bufptrs_and_extra_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return establish_groucho_bufptrs_and_extra_buffers(dz);
}

/***************************** SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS **************************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	int n;		 
	dz->ptr_cnt    = -1;		/* base constructor...process */
	dz->array_cnt  = -1;
	dz->iarray_cnt = -1;
	dz->larray_cnt = -1;
	switch(dz->process) {
	case(HF_PERM1):	
	case(HF_PERM2):
		dz->array_cnt=1; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
	case(DEL_PERM):	  
	case(DEL_PERM2):	  
		dz->array_cnt=3; dz->iarray_cnt=0; dz->larray_cnt=0; dz->ptr_cnt= 0; dz->fptr_cnt = 0; break;
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
		if((dz->ptr    	= (double  **)malloc(dz->ptr_cnt  * sizeof(double *)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for internal pointer arrays.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->ptr_cnt;n++)
			dz->ptr[n] = NULL;
	}
	if(dz->fptr_cnt > 0)   {  	  
		if((dz->fptr = (float  **)malloc(dz->fptr_cnt * sizeof(float *)))==NULL) {
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
	case(HF_PERM1):
	case(HF_PERM2):
		switch(dz->mode) {
		case(HFP_SNDOUT):	setup_process_logic(WORDLIST_ONLY,UNEQUAL_SNDFILE,SNDFILE_OUT,dz);	 break;
		case(HFP_SNDSOUT):	setup_process_logic(WORDLIST_ONLY,OTHER_PROCESS,  NO_OUTPUTFILE,dz); break;
		case(HFP_TEXTOUT):	setup_process_logic(WORDLIST_ONLY,TO_TEXTFILE,	  TEXTFILE_OUT,dz);	 break;
		case(HFP_MIDIOUT):	setup_process_logic(WORDLIST_ONLY,TO_TEXTFILE,	  TEXTFILE_OUT,dz);	 break;
		default:
			sprintf(errstr,"Unknown mode: assign_process_logic()\n");
			return(PROGRAM_ERROR);
		}
		break;
	case(DEL_PERM):			setup_process_logic(WORDLIST_ONLY,UNEQUAL_SNDFILE,SNDFILE_OUT,dz);	 break;
	case(DEL_PERM2):		setup_process_logic(SNDFILES_ONLY,UNEQUAL_SNDFILE,SNDFILE_OUT,dz);	 break;
	default:
		sprintf(errstr,"Unknown process: assign_process_logic()\n");
		return(PROGRAM_ERROR);
		break;
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

/****************************** FORMERLY IN internal.c *********************************/
/***************************************************************************************/

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	int exit_status = FINISHED;
	mode = 0;
	switch(process) {
	case(HF_PERM1):
	case(HF_PERM2):		
		exit_status = set_internalparam_data("i",ap); 		
		break;
	case(DEL_PERM):
	case(DEL_PERM2):		
		exit_status = set_internalparam_data("ii",ap); 		
		break;
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
//	int exit_status = FINISHED;
	aplptr ap = dz->application;

	switch(ap->special_data) {
	case(DELPERM):	return read_delperm_data(str,dz);
	case(DELPERM2):	return read_delperm2_data(str,dz);
	default:
		sprintf(errstr,"Unknown special_data type: read_special_data()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN preprocess.c ********************************/
/********************************************************************************************/

/****************************** PARAM_PREPROCESS *********************************/

int param_preprocess(dataptr dz)	
{
	int exit_status = FINISHED;
	int octaves_eliminated;
	double *hf = dz->parray[0];
	switch(dz->process) {
	case(HF_PERM1):		
		dz->iparam[HP1_HFCNT] =  dz->numsize;
		if((exit_status = check_for_invalid_data(HP1_HFCNT,dz))<0)
			return(exit_status);
		octaves_eliminated = eliminate_octave_duplications(dz);
		if(dz->iparam[HP1_HFCNT] < dz->iparam[HP1_MINSET]) {
			sprintf(errstr,"Minimum size of note set (%d)",dz->iparam[HP1_HFCNT]);
			if(octaves_eliminated) {
				sprintf(warnstr," (after octave duplication elimination)");
				strcat(errstr,warnstr);
			}
			sprintf(warnstr,"\nis less than minimum size specified for chords (%d): cannot proceed.\n",dz->iparam[HP1_MINSET]);
			strcat(errstr,warnstr);
			return(DATA_ERROR);
		}
		transpose_to_lowest_oct(dz);
		bublsort_hf(dz);
		if(hf[dz->iparam[HP1_HFCNT]-1] > (double)dz->iparam[HP1_TOPNOTE]) {
			sprintf(errstr,"Entered range does not span the given harmonic field\n");
			return(DATA_ERROR);
		}
		break;
	case(HF_PERM2):		
		dz->iparam[HP1_HFCNT] =  dz->numsize;
		if((exit_status = check_for_invalid_data(HP1_HFCNT,dz))<0)
			return(exit_status);
		elim_dupls(dz);
		if(dz->iparam[HP1_HFCNT] < dz->iparam[HP1_MINSET]) {
			sprintf(errstr,"Minimum size of note set (%d)",dz->iparam[HP1_HFCNT]);
			sprintf(warnstr,"\nis less than minimum size specified for chords (%d): cannot proceed.\n",dz->iparam[HP1_MINSET]);
			strcat(errstr,warnstr);
			return(DATA_ERROR);
		}
		bublsort_hf(dz);
		break;
	case(DEL_PERM2):		
		if((dz->ssampsread = fgetfbufEx(dz->sampbuf[1],dz->insams[0],dz->ifd[0],0)) < 0) {
			sprintf(errstr,"Can't read samps from input soundfile.\n");
			return(SYSTEM_ERROR);
		}
		if((exit_status = check_for_invalid_data(DP_NOTECNT,dz))<0)
			return(exit_status);
		break;
	case(DEL_PERM):		
		dz->iparam[DP_NOTECNT] =  dz->numsize;
		if((exit_status = check_for_invalid_data(DP_NOTECNT,dz))<0)
			return(exit_status);
		break;
	default:
		sprintf(errstr,"Unknown process in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN procgrou.c **********************************/
/********************************************************************************************/

/**************************** GROUCHO_PROCESS_FILE ****************************/

int groucho_process_file(dataptr dz)   /* FUNCTIONS FOUND IN PROCESS.C */
{	
//	int exit_status = FINISHED;

	switch(dz->process) {
	case(HF_PERM1):		
	case(HF_PERM2):		
		return do_hfperm(dz);
	case(DEL_PERM):		
	case(DEL_PERM2):		
		return gen_dp_output(dz);
	default:
		sprintf(errstr,"Unknown case in process_file()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);	/* NOTREACHED */
}

/********************************************************************************************/
/********************************** FORMERLY IN pconsistency.c ******************************/
/********************************************************************************************/

/****************************** CHECK_PARAM_VALIDITY_AND_CONSISTENCY *********************************/

int check_param_validity_and_consistency(dataptr dz)
{
	int exit_status = FINISHED;
	int temp;
	switch(dz->process) {
	case(HF_PERM1):
		dz->iparam[HP1_BOTNOTE] += MIDDLE_C_MIDI + (dz->iparam[HP1_BOTOCT] * 12);
		dz->iparam[HP1_TOPNOTE] += MIDDLE_C_MIDI + (dz->iparam[HP1_TOPOCT] * 12);
		if(dz->iparam[HP1_TOPNOTE] < dz->iparam[HP1_BOTNOTE]) {
			temp = dz->iparam[HP1_TOPNOTE];
			dz->iparam[HP1_TOPNOTE] = dz->iparam[HP1_BOTNOTE];
			dz->iparam[HP1_BOTNOTE] = temp;
		}
		/* fall thro */
	case(HF_PERM2):
		if(dz->mode == HFP_SNDOUT || dz->mode == HFP_SNDSOUT) {
			if(BAD_SR(dz->iparam[HP1_SRATE])) {
				sprintf(errstr,"Invalid sample rate.\n");
				return(DATA_ERROR);
			}
			dz->infile->channels = 1;
			dz->infile->srate    = dz->iparam[HP1_SRATE];
			dz->infile->stype    = SAMP_SHORT;

			dz->iparam[HP1_ELEMENT_SIZE] = (int)round(dz->param[HP1_ELEMENT_SIZE] * dz->infile->srate);
			dz->iparam[HP1_GAP_SIZE] 	 = (int)round(dz->param[HP1_GAP_SIZE] 	  * dz->infile->srate);
			dz->iparam[HP1_GGAP_SIZE] 	 = (int)round(dz->param[HP1_GGAP_SIZE] 	  * dz->infile->srate);
		}
		if(dz->mode == HFP_SNDOUT) {
			if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
				return(exit_status);
		}
		break;
	case(DEL_PERM):
		if(BAD_SR(dz->iparam[DP_SRATE])) {
			sprintf(errstr,"Invalid sample rate.\n");
			return(DATA_ERROR);
		}
		dz->infile->channels = 1;
		dz->infile->srate    = dz->iparam[DP_SRATE];
		dz->infile->stype    = SAMP_SHORT;
		dz->iparam[DP_DUR] = (int)round(dz->param[DP_DUR] * dz->infile->srate);
		if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
			return(exit_status);
		break;
	case(DEL_PERM2):
		dz->iparam[DP_DUR] = (int)(dz->insams[0]);
		break;
	}
	return(FINISHED);
}

/****************************** HFPERM_CONSISTENCY *********************************/

int hfperm_consistency(dataptr dz)
{
	return(FINISHED);
}


/********************************************************************************************/
/********************************** FORMERLY IN buffers.c ***********************************/
/********************************************************************************************/

/**************************** ALLOCATE_LARGE_BUFFERS ******************************/

int allocate_large_buffers(dataptr dz)
{
	switch(dz->process) {
	case(HF_PERM1):
	case(HF_PERM2):
		if(dz->mode == HFP_SNDOUT || dz->mode == HFP_SNDSOUT)
			return create_hfpermbufs(dz);
		break;
	case(DEL_PERM):
	case(DEL_PERM2):
		return create_hfpermbufs(dz);
	default:
		sprintf(errstr,"Unknown program no. in allocate_large_buffers()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN cmdline.c ***********************************/
/********************************************************************************************/

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if     (!strcmp(prog_identifier_from_cmdline,"hfchords"))	dz->process = HF_PERM1;
	else if(!strcmp(prog_identifier_from_cmdline,"hfchords2"))	dz->process = HF_PERM2;
	else if(!strcmp(prog_identifier_from_cmdline,"delperm"))	dz->process = DEL_PERM;
	else if(!strcmp(prog_identifier_from_cmdline,"delperm2"))	dz->process = DEL_PERM2;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/********************************************************************************************/
/********************************** FORMERLY IN usage.c *************************************/
/********************************************************************************************/

/******************************** USAGE1 ********************************/

int usage1(void)
{
	sprintf(errstr,
	"USAGE: hfperm NAME (mode) outfile parameters\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"hfchords hfchords2 delperm\n"
	"\n"
	"OR: hfperm NAME (mode) infile outfile parameters\n"
	"\n"
	"where NAME can be any one of\n"
	"\n"
	"delperm2\n"
	"\n"
	"Type 'hfperm hfchords'  for more info on hfperm hfchords option... ETC.\n");
	return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"hfchords")) {
		fprintf(stdout,
    	"GENERATE ALL CHORDS POSSIBLE FROM NOTES IN GIVEN SET, WITHIN GIVEN PITCH RANGE\n"
    	"USING EACH  NOTE (OR ITS OCTAVE TRANSPOSITION) ONCE ONLY IN ANY ONE CHORD.\n\n"
		"USAGE: hfperm hfchords 1   ifil ofil sr nd gd pd min bn bo tn to srt [-m -s -a -o]\n"
		"OR:    hfperm hfchords 2   ifil ofil sr nd gd    min bn bo tn to srt [-m -s -a -o]\n"
		"OR:    hfperm hfchords 3-4 ifil ofil sr      min bn bo tn to srt [-m -s -a -o]\n"
		"IFIL   text input file of midi note values\n"
		"MODES: 1 SOUND OUT...Outputs 1 soundfile of all chords.\n"
		"       2 SOUNDS OUT..Outputs several sndfiles, chords grouped by sort specified\n"
		"       3 TEXT OUT ...Outputs textfile listing chords described by note names.\n"
		"       4 MIDI OUT ...Outputs list of chords described by MIDI values.\n"
		"PARAMETERS: OFIL output filename              SR  sample rate sound output\n"
		"            ND   duration each chord (secs)   GD  gap btwn chords (secs)\n"
		"            PD   gap btwn chord-groups        MIN min no. notes to combine\n"
		"BN  bottom note pitchrange, for derived chords (0-11 for C-C#....Bb-B)\n"
		"BO  octave of bottom note (0 = octave upwards from middle C)\n"
		"            TN  top note of pitchrange        TO  octave of top note\n"
		"SRT 0 root: 1 topnote: 2 pitchclass: 3 chordtype: 4 chordtype(keep 1 of each)\n"
		"   -m  generate only chords of min number of notes specified\n"
		"   -s  in each group, put smallest span chord first (otherwise it goes last).\n"
		"-a  do final sort by a method other than by chord density\n"
		"    For ROOT or TOP NOTE sort, sort by way intervals stacked inside chord.\n"
		"    For PITCHCLASS SET, sort by size of containing interval, then by density.\n"
		"-o  eliminate chords duplicating-as-a-whole any chord 1 or more 8vas distant.\n");
	} else if(!strcmp(str,"hfchords2")) {
		fprintf(stdout,
     	"GENERATE ALL CHORDS POSSIBLE FROM NOTES IN GIVEN SET.\n\n"
		"USAGE: hfperm hfchords2 1   ifil ofil sr nd gd pd min srt [-m -s -a -o]\n"
		"OR:    hfperm hfchords2 2   ifil ofil sr nd gd    min srt [-m -s -a -o]\n"
		"OR:    hfperm hfchords2 3-4 ifil ofil sr          min srt [-m -s -a -o]\n"
		"IFIL   text input file of midi note values\n"
		"MODES: 1 SOUND OUT...Outputs 1 soundfile of the chords.\n"
		"       2 SOUNDS OUT..Outputs several sndfiles, chords grouped by sort specified\n"
		"       3 TEXT OUT ...Outputs textfile listing chords described by note names.\n"
		"       4 MIDI OUT ...Outputs list of chords described by MIDI values.\n"
		"PARAMETERS ARE\n"
		"SRATE       sample rate of the sound output\n"
		"NOTE-DUR    duration of each chord generated (secs)\n"
		"GAP-DUR     duration of pauses between chords (secs)\n"
		"PAUSE-DUR   duration of pauses between groups of chords (secs)\n"
		"MINSET      minimum number of input notes to combine\n"
		"SORT BY     0 root:         1 topnote:         2 pitchclass set:\n"
		"            3 chord type:   4 chord type (keep 1 of each).\n"
		"-m  generate only chords of min number of notes specified\n"
		"-s  in each group, put smallest span chord first (otherwise it goes last).\n"
		"-a  do final sort by a method other than by chord density\n"
		"    For ROOT or TOP NOTE sort, sort by way intervals stacked inside chord.\n"
		"    For PITCHCLASS SET, sort by size of containing interval, then by density\n"
		"   -o  eliminate chords duplicating-as-a-whole any chord 1 or more 8vas distant\n");
	} else if(!strcmp(str,"delperm")) {
		fprintf(stdout,
    	"DELAY AND TRANSFORM EACH INPUT NOTE, CUMULATIVELY.\n\n"
		"USAGE: hfperm delperm 0 \n"
		"   infile outfile permfile srate initial-notelen cycles-of-perm\n"
		"PARAMETERS ARE\n"
		"   INFILE          text input file of midi note values\n"
		"   PERMFILE		pairs of vals: 1st a semitone transpos, 2nd a time multiplier\n"
		"                   Sum of all time-multipliers must be 1.0\n"
		"   SRATE           sample rate of the sound output\n"
		"   INITIAL-NOTELEN length of each input note, before it is transformed\n"
		"   CYCLES-OF-PERM  Number of times the permutation is recursively applied.\n");
	} else if(!strcmp(str,"delperm2")) {
		fprintf(stdout,
    	"DELAY AND TRANSFORM EACH INPUT NOTE, CUMULATIVELY.\n\n"
		"USAGE: hfperm delperm2 0 \n"
		"   infile outfile permfile cycles-of-perm\n"
		"PARAMETERS ARE\n"
		"   PERMFILE		pairs of vals: 1st a semitone transpos, 2nd a time multiplier\n"
		"                   Sum of all time-multipliers must be 1.0\n"
		"   CYCLES-OF-PERM  Number of times the permutation is recursively applied.\n");
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

/************************** CHECK_FOR_INVALID_DATA **************************/
 
int check_for_invalid_data(int k,dataptr dz)
{
	double *hf = dz->parray[0];
	int *bad, badcnt = 0;
	int n;
	if((bad = (int *)malloc(dz->iparam[k] * sizeof(int)))==NULL)
		return(MEMORY_ERROR);
	for(n=0;n< dz->iparam[k];n++) {	
		if(hf[n] < MIDIMIN || hf[n] > MIDIMAX)
			bad[badcnt++] = n;						
	}
	if(badcnt) {
		print_outmessage_flush("Invalid midi data in input.....\n");
		for(n=0;n < badcnt;n++) {
			sprintf(errstr,"item %d : value %ld\n",bad[n], round(hf[bad[n]]));
			print_outmessage_flush(errstr);
		}
		sprintf(errstr,"Cannot proceed\n");	
		return(DATA_ERROR);
	}
	return FINISHED;
}

/**************************** TRANSPOSE_TO_LOWEST_OCT *******************************/

void transpose_to_lowest_oct(dataptr dz)
{
	int n;
	double *hf = dz->parray[0];
	for(n=0;n<dz->iparam[HP1_HFCNT];n++) {
		while(hf[n] < (double)dz->iparam[HP1_BOTNOTE])
			hf[n] += 12.0;
		while(hf[n] >= (double)dz->iparam[HP1_BOTNOTE])
			hf[n] -= 12.0;
		hf[n] += 12.0;
	}
}

/**************************** BUBLSORT_HF *******************************/

void bublsort_hf(dataptr dz)
{
	int n, m;
	double temp;
	double *hf = dz->parray[0];
	for(n=0;n<dz->iparam[HP1_HFCNT]-1;n++) {
		for(m = n; m<dz->iparam[HP1_HFCNT]; m++) {
			if(hf[m] < hf[n]) {
				temp = hf[n];
				hf[n] = hf[m];
				hf[m] = temp;
			}
		}
	}
}

/**************************** ELIM_DUPLS *******************************/

void elim_dupls(dataptr dz)
{
	int n, m, k;
	double *hf = dz->parray[0];
	for(n=0;n<dz->iparam[HP1_HFCNT]-1;n++) {
		for(m = n+1; m<dz->iparam[HP1_HFCNT]; m++) {
			if(hf[m] == hf[n]) {
				for(k = m+1; k < dz->iparam[HP1_HFCNT];k++)
					hf[k-1] = hf[k];
				dz->iparam[HP1_HFCNT]--;
				m--;					
			}
		}
	}
}

/******************************** INNER_LOOP (redundant)  ********************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}




/*************************** CREATE_HFPERMBUFS **************************/

int create_hfpermbufs(dataptr dz)
{
	unsigned int presize = 0;
//TW ADDED
	size_t bigbufsize;
    int fbytesector = F_SECSIZE * sizeof(float);
	int max_jitter = (int)round((double)HFP_MAXJITTER * (double)MS_TO_SECS * (double)dz->infile->srate);
	int k = 0;

	switch(dz->process) {
	case(HF_PERM1):
	case(HF_PERM2):	
		k = HP1_ELEMENT_SIZE;	
		break;
	case(DEL_PERM):	
	case(DEL_PERM2):	
		k = DP_DUR;				
		break;
	}

	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_hfpermbufs()\n");
		return(PROGRAM_ERROR);
	}
	switch(dz->process) {
	case(HF_PERM1):
	case(HF_PERM2):
		presize = (dz->iparam[k] + max_jitter + HFP_TABSIZE + 1) * sizeof(double);
		break;
	case(DEL_PERM):
		presize = ((dz->iparam[k] * 2) + max_jitter + HFP_TABSIZE + 1) * sizeof(double);
		break;
	case(DEL_PERM2):
		presize = ((dz->iparam[k] * 2) + max_jitter) * sizeof(double)
					+ dz->iparam[k] * sizeof(float);
		break;
	}	
//TW CHANGED FOR FLOATS
	bigbufsize = (size_t) Malloc(-1);
	if((bigbufsize  = (bigbufsize/fbytesector) * fbytesector)<=0)
		bigbufsize  = fbytesector;
	while((dz->bigbuf = (float *)malloc((size_t)(bigbufsize + presize))) == NULL) {
		if((bigbufsize -= fbytesector) <= 0) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
			return(PROGRAM_ERROR);
		}
	}
	dz->buflen = (int)(bigbufsize/sizeof(float));
	memset(dz->bigbuf,0,bigbufsize + presize);
	switch(dz->process) {
	case(HF_PERM1):
	case(HF_PERM2):
	case(DEL_PERM):
		dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
		dz->sbufptr[1] = dz->sampbuf[1] = dz->bigbuf + dz->buflen;
		dz->sampbuf[2] = (float *)((double *)dz->sampbuf[1] + HFP_TABSIZE + 1);
		dz->sampbuf[3] = (float *)((double *)dz->sampbuf[2] + dz->iparam[k] + max_jitter);
		if(dz->process == DEL_PERM)
			dz->sampbuf[4] = (float *)((double *)dz->sampbuf[3] + dz->iparam[k]);
		establish_waveform(dz);
		break;
	case(DEL_PERM2):
		dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
		dz->sbufptr[1] = dz->sampbuf[1] = dz->bigbuf + dz->buflen;
		dz->sampbuf[2] = dz->sampbuf[1] + dz->insams[0];
		dz->sampbuf[3] = (float *)((double *)dz->sampbuf[2] + dz->iparam[k] + max_jitter);
		dz->sampbuf[4] = (float *)((double *)dz->sampbuf[3] + dz->iparam[k]);
		break;
	}
	return(FINISHED);
}

/*************************** ESTABLISH_WAVEFORM **************************
 *
 * Odd harmonics, declining in amplitude 1, 1/2. 1.3, 1/4 etc...
 */

void establish_waveform(dataptr dz)
{
	int n, partialno, ampdivide;
	double j, maxval = 0.0, normaliser;
	double *sintab = (double *)dz->sampbuf[1];

	memset((char *)sintab,0,HFP_TABSIZE * sizeof(double));
	for(partialno=1,ampdivide=1;partialno<12;partialno+=2,ampdivide++) {
		for(n=0;n < HFP_TABSIZE; n++) {
			j = sin(TWOPI * partialno * (double)n/(double)HFP_TABSIZE)/(double)ampdivide;
			*sintab += j;
			maxval = max(maxval,fabs(*sintab));
			sintab++;
		}
		j = sin(TWOPI * partialno * (double)n/(double)HFP_TABSIZE)/(double)ampdivide;
		*sintab += j;
		maxval = max(maxval,fabs(*sintab));
		sintab = (double *)dz->sampbuf[1];
	}
	normaliser = 1.0/maxval;
	for(n=0;n < HFP_TABSIZE; n++)
		*sintab++ *= normaliser;
}


/************************** READ_DELPERM_DATA ********************************/

int read_delperm_data(char *filename,dataptr dz)
{
	int n = 0, k, valcnt = 0;
	char temp[200], *p;
	double *val;
//	int is_transpos = TRUE;
	double dursum = 0.0;
	int arraysize = BIGARRAY;

	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	if((val = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"No memory for permutation data.\n");
		return(MEMORY_ERROR);
	}
	
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		while(strgetfloat(&p,&(val[valcnt]))) {
			if(++valcnt > arraysize) {
				arraysize += BIGARRAY;
				if((val = (double *)realloc((char *)val,arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Out of memory loading permutation data.\n");
					return(MEMORY_ERROR);
				}
			}			
		}
	}
	if(valcnt < arraysize) {
		if((val = (double *)realloc((char *)val,valcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"Squeezing permutation data.\n");
			return(MEMORY_ERROR);
		}
	}
	if(ODD(valcnt)) {
		sprintf(errstr,"Permutation data incorrectly paired.\n");
		return(DATA_ERROR);
	}
	valcnt /= 2;
	if((dz->parray[1] = (double *)malloc(valcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"No memory for transposition data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[2] = (double *)malloc(valcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"No memory for transposition data.\n");
		return(MEMORY_ERROR);
	}
	for(n=0,k=0;n<valcnt;n++) {
		if((dz->parray[1][n] = val[k++]) < dz->application->min_special
		||	dz->parray[1][n] > dz->application->max_special) {
			sprintf(errstr,"Transposition value %lf out of range, in permutation data.\n",dz->parray[1][n]);
			return(DATA_ERROR);
		}		
		if((dz->parray[2][n] = val[k++]) < dz->application->min_special2
		||	dz->parray[2][n] > dz->application->max_special2) {
			sprintf(errstr,"Duration multiplier value %lf out of range, in permutation data.\n",dz->parray[2][n]);
			return(DATA_ERROR);
		}		
		dursum += dz->parray[2][n];
	}
	if(!flteq(dursum,1.0)) {
		sprintf(errstr,"Duration multipliers in permutation data do not sum to 1.0.\n");
		return(DATA_ERROR);
	}				
	free(val);
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	dz->iparam[DP_PERMCNT] = valcnt;
	return(FINISHED);
}

/************************** READ_DELPERM2_DATA ********************************/

int read_delperm2_data(char *filename,dataptr dz)
{
	int n = 0, k, valcnt = 0/*, linecnt = 0*/;
	char temp[200], *p;
	double *val;
//	int is_transpos = TRUE;
	double dursum = 0.0;
	int arraysize = BIGARRAY;

	if((dz->fp = fopen(filename,"r"))==NULL) {
		sprintf(errstr,"Cannot open datafile %s\n",filename);
		return(DATA_ERROR);
	}
	if((val = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		sprintf(errstr,"No memory for permutation data.\n");
		return(MEMORY_ERROR);
	}
	while(fgets(temp,200,dz->fp)!=NULL) {
		p = temp;
		while(strgetfloat(&p,&(val[valcnt]))) {
			if(++valcnt > arraysize) {
				arraysize += BIGARRAY;
				if((val = (double *)realloc((char *)val,arraysize * sizeof(double)))==NULL) {
					sprintf(errstr,"Out of memory loading permutation data.\n");
					return(MEMORY_ERROR);
				}
			}			
		}
/*		if(!linecnt) {
			if((dz->parray[0] = (double *)malloc(valcnt * sizeof(double)))==NULL) {
				sprintf(errstr,"No memory for midi transposition data data.\n");
				return(MEMORY_ERROR);
			}
			dz->iparam[DP_NOTECNT] = valcnt;
			memcpy((char *)dz->parray[0],(char *)val,valcnt * sizeof(double));
			linecnt++;
			valcnt = 0;
			continue;
		}
*/
	}
	if(valcnt < arraysize) {
		if((val = (double *)realloc((char *)val,valcnt * sizeof(double)))==NULL) {
			sprintf(errstr,"Squeezing permutation data.\n");
			return(MEMORY_ERROR);
		}
	}
	if(ODD(valcnt)) {
		sprintf(errstr,"Permutation data incorrectly paired.\n");
		return(DATA_ERROR);
	}
	valcnt /= 2;
	if((dz->parray[1] = (double *)malloc(valcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"No memory for transposition data.\n");
		return(MEMORY_ERROR);
	}
	if((dz->parray[2] = (double *)malloc(valcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"No memory for duration multiplier data.\n");
		return(MEMORY_ERROR);
	}
	for(n=0,k=0;n<valcnt;n++) {
		if((dz->parray[1][n] = val[k++]) < dz->application->min_special
		||	dz->parray[1][n] > dz->application->max_special) {
			sprintf(errstr,"Transposition value %lf out of range, in permutation data.\n",dz->parray[1][n]);
			return(DATA_ERROR);
		}		
		if((dz->parray[2][n] = val[k++]) < dz->application->min_special2
		||	dz->parray[2][n] > dz->application->max_special2) {
			sprintf(errstr,"Duration multiplier value %lf out of range, in permutation data.\n",dz->parray[2][n]);
			return(DATA_ERROR);
		}		
		dursum += dz->parray[2][n];
	}
	if(!flteq(dursum,1.0)) {
		sprintf(errstr,"Duration multipliers in permutation data do not sum to 1.0. (%lf)\n",dursum);
		return(DATA_ERROR);
	}				
	free(val);
	if(fclose(dz->fp)<0) {
		fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
		fflush(stdout);
	}
	dz->iparam[DP_PERMCNT] = valcnt;
	return(FINISHED);
}

