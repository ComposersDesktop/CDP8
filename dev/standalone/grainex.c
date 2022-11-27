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



/*
 *	INPUT MUST BE MONO
 */

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <grain.h>
#include <filetype.h>
#include <modeno.h>
#include <formants.h>
#include <cdpmain.h>
#include <special.h>
#include <logic.h>
#include <globcon.h>
#include <cdpmain.h>
#include <sfsys.h>
#include <ctype.h>
#include <string.h>
#include <standalone.h>
#include <osbind.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

/*extern*/ int	sloom = 0;
/* TW May 2001 */
/*extern*/ int sloombatch = 0;	/*TW may 2001 */
/*extern*/ int anal_infiles = 0;
/*extern*/ int is_converted_to_stereo = -1;
const char* cdp_version = "7.1.0";

static int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int setup_grextend_application(dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_grextend_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int check_grextend_param_validity_and_consistency(dataptr dz);
static int grevex(dataptr dz);
static void get_rrrenv_of_buffer(int samps_to_process,int envwindow_sampsize,float **envptr,float *buffer);
static float getmaxsampr(int startsamp, int sampcnt,float *buffer);
static int grevex(dataptr dz);
static void do_repet_restricted_perm(int *arr, int *perm, int arrsiz, int *endval);
static int do_envgrain_addwrite(int startsearch,int endsearch,int *last_total_samps_read,int *obufpos,dataptr dz);
static int establish_application(dataptr dz);
static int set_param_data(aplptr ap, int special_data,int maxparamcnt,int paramcnt,char *paramlist);
static int set_vflgs(aplptr ap,char *optflags,int optcnt,char *optlist,char *varflags,int vflagcnt, int vparamcnt,char *varlist);
static int application_init(dataptr dz);
static int initialise_vflags(dataptr dz);
static int setup_input_param_defaultval_stores(int tipc,aplptr ap);
static int setup_and_init_input_param_activity(dataptr dz,int tipc);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int mark_parameter_types(dataptr dz,aplptr ap);
static int assign_file_data_storage(int infilecnt,dataptr dz);
static int get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static void hhinsert(int m,int t,int setlen,int *perm);
static void hhprefix(int m,int setlen,int *perm);
static void hhshuflup(int k,int setlen,int *perm);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int create_grextend_sndbufs(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
//	FILE *fp   = NULL;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt, n;
	aplptr ap;
	int is_launched = FALSE;
	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
						/* CHECK FOR SOUNDLOOM */
	if((sloom = sound_loom_in_use(&argc,&argv)) > 1) {
		sloom = 0;
		sloombatch = 1;
	}
	if(sflinit("cdp")){
		sfperror("cdp: initialisation\n");
		return(FAILED);
	}
						  /* SET UP THE PRINCIPLE DATASTRUCTURE */
	if((exit_status = establish_datastructure(&dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(!sloom) {
		if(argc == 1) {
			usage1();	
			return(FAILED);
		} else if(argc == 2) {
			usage2(argv[1]);	
			return(FAILED);
		}
							  /* INITIAL CHECK OF CMDLINE DATA */
		if((exit_status = make_initial_cmdline_check(&argc,&argv))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline    = argv;	/* GET PRE_DATA, ALLOCATE THE APPLICATION, CHECK FOR EXTRA INFILES */
		cmdlinecnt = argc;
		if((get_the_process_no(argv[0],dz))<0)
			return(FAILED);
		cmdline++;
		cmdlinecnt--;
		dz->maxmode = 0;
		if((exit_status = setup_grextend_application(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else {
		if((exit_status = parse_sloom_data(argc,argv,&cmdline,&cmdlinecnt,dz))<0) {  	/* includes setup_particular_application()      */
			exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);/* and cmdlinelength check = sees extra-infiles */
			return(exit_status);		 
		}
	}

	ap = dz->application;

	// parse_infile_and_hone_type() = 
	if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// setup_param_ranges_and_defaults() =
	if((exit_status = setup_grextend_param_ranges_and_defaults(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// open_first_infile		CDP LIB
	if((exit_status = open_first_infile(cmdline[0],dz))<0) {	
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
		return(FAILED);
	}
//TW UPDATE
	cmdlinecnt--;
	cmdline++;
	// handle_outfile() = 
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {		// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
//	check_param_validity_and_consistency....
	if((exit_status = check_grextend_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
 	is_launched = TRUE;
	switch(dz->process) {
	case(GREV_EXTEND): dz->bufcnt = 3; break;
	}
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0;n <dz->bufcnt; n++)
		dz->sampbuf[n] = dz->sbufptr[n] = (float *)0;
	dz->sampbuf[n] = (float *)0;
	if((exit_status = create_grextend_sndbufs(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	// param_preprocess() .....

	if((exit_status = grevex(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = complete_output(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	exit_status = print_messages_and_close_sndfiles(FINISHED,is_launched,dz);
	free(dz);
	return(SUCCEEDED);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	if      (!strcmp(prog_identifier_from_cmdline,"extend"))			dz->process = GREV_EXTEND;
	else {
		sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
		return(USAGE_ONLY);
	}
	return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
	return usage2("extend");
}


/******************************** USAGE2 ********************************/

int usage2(char *str)
{
	if(!strcmp(str,"extend")) {		
	    fprintf(stdout,
		"FIND GRAINS IN A SOUND, AND EXTEND AREA THAT CONTAINS THEM\n\n"
		"USAGE:\n"
		"grainex extend inf outf wsiz trof plus stt end\n\n"
		"WSIZ    sizeof window in ms, determines size of grains to find.\n"
		"TROF    acceptable trough height, relative to adjacent peaks (range >0 - <1)\n"
		"PLUS    how much duration to add to source.\n"
		"STT     Time of start of grain material within source.\n"
		"END     Time of end of grain material within source.\n");
	} else
		fprintf(stdout,"Unknown option '%s'\n",str);
	return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
	fprintf(stderr,"Insufficient parameters on command line.\n");
	return(USAGE_ONLY);
}

/************************* EXTRACT_RRR_ENV_FROM_SNDFILE *******************************/

int extract_rrr_env_from_sndfile(int paramno,dataptr dz)
{
	int n;
	float *envptr;
	int bufcnt;
	if(((bufcnt = dz->insams[0]/dz->buflen)*dz->buflen)!=dz->insams[0])
		bufcnt++;
	envptr = dz->env;
	for(n = 0; n < bufcnt; n++)	{
		if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[0],0)) < 0) {
			sprintf(errstr,"Can't read samples from soundfile: extract_rrr_env_from_sndfile()\n");
			return(SYSTEM_ERROR);
		}
		if(sloom)
			display_virtual_time(dz->total_samps_read,dz);
		get_rrrenv_of_buffer(dz->ssampsread,dz->iparam[paramno],&envptr,dz->sampbuf[0]);
	}
	dz->envend = envptr;
	return(FINISHED);
}

/************************* GET_RRRENV_OF_BUFFER *******************************/

void get_rrrenv_of_buffer(int samps_to_process,int envwindow_sampsize,float **envptr,float *buffer)
{
	int  start_samp = 0;
	float *env = *envptr;
	while(samps_to_process >= envwindow_sampsize) {
		*env++       = getmaxsampr(start_samp,envwindow_sampsize,buffer);
		start_samp  += envwindow_sampsize;
		samps_to_process -= envwindow_sampsize;
	}
	if(samps_to_process)	/* Handle any final short buffer */
		*env++ = getmaxsampr(start_samp,samps_to_process,buffer);
	*envptr = env;
}

/*************************** GETMAXSAMPR ******************************/

float getmaxsampr(int startsamp, int sampcnt,float *buffer)
{
	int  i, endsamp = startsamp + sampcnt;
	float thisval, thismaxsamp = 0.0f;
	for(i = startsamp; i<endsamp; i++) {
		if((thisval = (float)fabs(buffer[i]))>thismaxsamp)
			thismaxsamp = thisval;
	}
	return(thismaxsamp);
}

/************************** GREVEX **********************/

int grevex(dataptr dz)
{
	int exit_status, finished, start_negative, *arr, *perm, *arr2, endval, gotstart;
	int n, m, j=0, k, indur, element_cnt, minpeakloc, envcnt, last_total_samps_read, startsearch, endsearch, obufpos;
	int origstartbuf, origsamspread, ibufpos;
	int expansion;
	double maxsamp0, maxsamp1, peakav, minpeakav;
	int firsttrof, up, gotmaxsamp0, crossed_zero_to_positive, crossed_zero_to_negative;
	float *e, *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
	int *pa;

	if(((envcnt = dz->insams[0]/dz->iparam[GREV_SAMP_WSIZE]) * dz->iparam[GREV_SAMP_WSIZE])!=dz->insams[0])
		envcnt++;
	if((dz->env=(float *)malloc((envcnt + 12) * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for envelope array.\n");
		return(MEMORY_ERROR);
	}
	e = dz->env;
	if((pa =(int *)malloc((envcnt + 12) * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for peak positions array.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = extract_rrr_env_from_sndfile(GREV_SAMP_WSIZE,dz))<0)	/* Get envel of whole sound */
		return(exit_status);
	dz->total_samps_read = 0;
	display_virtual_time(0,dz);
	envcnt = dz->envend - dz->env;
	n = 0;
	k = 0;
	pa[k++] = 0;
	while(flteq(e[n],e[0])) {
		n++;
		if(n >= envcnt) {
			sprintf(errstr,"NO PEAKS IN THE FILE\n");
			return(GOAL_FAILED);
		}
	}
	if(e[n] < e[0]) {
		firsttrof = 1;
		up = -1;
	} else {
		firsttrof = 0;
		up = 1;
	}
	
	/* KEEP ONLY THE PEAKS AND TROUGHS OF THE ENVELOPE, AND THEIR LOCATIONS */

	while (n <envcnt) {	/* store peaks and troughs only */
		switch(up) {
		case(1):
			if(e[n] < e[n-1]) {
				dz->env[k] = dz->env[n-1]; 
				pa[k]  = (n-1) * dz->iparam[GREV_SAMP_WSIZE];
				k++;
				up = -1;
			}
			break;
		case(-1):
			if(e[n] > e[n-1]) {
				dz->env[k] = dz->env[n-1]; 
				pa[k]  = (n-1) * dz->iparam[GREV_SAMP_WSIZE];
				k++;
				up = 1;
			}
			break;
		}
		n++;
	}
	if((envcnt = k) <= 3) {
		sprintf(errstr,"INSUFFICIENT PEAKS IN THE FILE.\n");
		return(GOAL_FAILED);
	}

	/* KEEP ONLY THE (DEEP ENOUGH) TROUGHS OF THE ENVELOPE */
	
	switch(firsttrof) {
	case(0):		/* if trof at start */
		k = 1;		/* set item at 0 NOT to be overwritten (as it is first trof) (set k=1) */
		j = 1;		/* search for good trofs between peaks, from (j=)1 */
		break;
	case(1):		/* if trof not at start */
		k = 0;		/* set item at 0 to be overwritten by 1st trof found (k=0) */
		j = 0;		/* search for good trofs between peaks, from (j=)0 */
		break;
	}
	for(n=j;n<envcnt-2;n++) {
		peakav = dz->env[n] + dz->env[n+2];
		if(peakav * dz->param[GREV_TROFRAC] >= dz->env[n+1]) {	/* NB TROF_FAC alreday PRE-MULTIPLIED by 2.0 */
			pa[k]  = pa[n+1];
			k++;
		}
	}
	if((envcnt = k) <= 3) {
		sprintf(errstr,"INSUFFICIENT VALID TROUGHS IN THE FILE.\n");
		return(GOAL_FAILED);
	}

	/* SEARCH WAVEFORM FOR ZERO_CROSSING AT MORE ACCURATE TROUGH */

	if((sndseekEx(dz->ifd[0],0,0))<0) {
		sprintf(errstr,"seek error 1\n");
		return(SYSTEM_ERROR);
	}
	last_total_samps_read = 0;
	k = (int)round((double)dz->iparam[GREV_SAMP_WSIZE] * 0.5);	/* search around size of envel window */
	startsearch = max(pa[0] - k, 0);
	endsearch   = min(pa[0] + k,dz->insams[0]);
	dz->total_samps_read = 0;
	while(startsearch > dz->buflen) {
		dz->total_samps_read += dz->buflen;
		startsearch -= dz->buflen;
	}
	if(dz->total_samps_read > 0) {
		if((sndseekEx(dz->ifd[0],dz->total_samps_read,0))<0) {
			sprintf(errstr,"seek error 2\n");
			return(SYSTEM_ERROR);
		}
		last_total_samps_read = dz->total_samps_read;
		endsearch   -= last_total_samps_read;
	}
	if((exit_status = read_samps(ibuf,dz))<0)
		return(exit_status);
	n = 0;
	finished = 0;
	while(n<envcnt) {
		maxsamp0 = 0.0;
		maxsamp1 = 0.0;
		gotmaxsamp0 = 0;
		minpeakav = HUGE;
		minpeakloc = -1;
		j = startsearch;
		crossed_zero_to_positive = 0;
		crossed_zero_to_negative = 0;
		if(ibuf[j] <= 0)
			start_negative = 1;
		else
			start_negative = 0;
		do {
			if(j >= dz->ssampsread) {
				last_total_samps_read = dz->total_samps_read;
				endsearch -= dz->buflen;
				j		  -= dz->buflen;
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				if(dz->ssampsread == 0) {
					finished = 1;
					break;
				}
			}
			if(!crossed_zero_to_negative) {		/* before signal crosses to negative */
				if(start_negative) {
					if(ibuf[j] <= 0.0) {
						j++;
						continue;
					}
					start_negative = 0;
				}
				if(!gotmaxsamp0) {				/* First time only, look for first maxsamp */
					if(ibuf[j] > maxsamp0)		/* (after first time, it gets val passed back from 2nd maxsamp */
						maxsamp0 = ibuf[j];
				} 
				if (ibuf[j] < 0.0) {			/* if not crossed zero to -ve, look for, and mark, zero-cross to -ve */
					crossed_zero_to_negative = j + last_total_samps_read;
					gotmaxsamp0 = 1;
				}
			} else if (ibuf[j] >= 0) {		/* if crossed zero to neg and we're now crossing back to +ve */
				crossed_zero_to_positive = 1;
				if(ibuf[j] > maxsamp1)		/* look for 2nd maxsamp */
					maxsamp1 = ibuf[j];
			} else if (crossed_zero_to_positive) {	/* having crossed from -ve to +ve, we're now -ve again, in a new cycle */
				if((peakav = maxsamp0 + maxsamp1) < minpeakav) {
					minpeakav = peakav;
					minpeakloc = crossed_zero_to_negative;
				}
				maxsamp0 = maxsamp1;
				crossed_zero_to_positive = 0;
				crossed_zero_to_negative = 0;
			}
			j++;
		} while(j < endsearch || minpeakloc < 0);

		if(minpeakloc < 0) {	
			if (finished) {		/* deal with endcases where waveform fails to cross zero (twice) */
				if(crossed_zero_to_negative > 0)
					pa[n++] = crossed_zero_to_negative;
				envcnt = n;
				break;
			} else {	
				sprintf(errstr,"FAILED TO FIND ONE OF THE LOCAL MINIMA.\n");
				return(PROGRAM_ERROR);
			}
		}
		pa[n] = minpeakloc;		
		n++;
		startsearch = max(pa[n] - k, 0);
		endsearch   = min(pa[n] + k,dz->insams[0]);
		if(startsearch >= dz->total_samps_read) {
			while(startsearch >= dz->total_samps_read) {
				last_total_samps_read = dz->total_samps_read;
				if((exit_status = read_samps(ibuf,dz))<0)
					return(exit_status);
				if(last_total_samps_read >= dz->total_samps_read) {
					envcnt = n;
					break;
				}
			}
		}
		startsearch -= last_total_samps_read;
		endsearch   -= last_total_samps_read;
		while(startsearch < 0) {	/* very tiny windows may cause backtracking in file */
			last_total_samps_read -= dz->buflen;
			if((sndseekEx(dz->ifd[0],last_total_samps_read,0))<0) {
				sprintf(errstr,"seek error 3\n");
				return(SYSTEM_ERROR);
			}
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			dz->total_samps_read = last_total_samps_read + dz->ssampsread;
			startsearch += dz->buflen;
			endsearch += dz->buflen;
		}
	}
	if((sndseekEx(dz->ifd[0],0,0))<0) {
		sprintf(errstr,"seek error 4\n");
		return(SYSTEM_ERROR);
	}
	dz->total_samps_read = 0;
	last_total_samps_read = 1;	/* Value 1 forces first seek and read */
	obufpos = 0;
	switch(dz->process) {
	case(GREV_EXTEND):
		gotstart = 0;
		j = 0;		/* DEFAULT START OF AREA TO PROCESS (Safety only) */
		k = envcnt;	/* DEFAULT END OF AREA TO PROCESS IF NONE FOUND */
		for(n=0;n<envcnt;n++) {
			if (!gotstart) {
				if(pa[n] >= dz->iparam[3]) {
					j = n;
					gotstart = 1;
				}
			}
			if(pa[n] > dz->iparam[4]) {
				k = n;
				break;
			}
		}
		if(j == k) {
			sprintf(errstr,"INSUFFICIENT PEAKS IN THE FILE AREA SPECIFIED.\n");
			return(GOAL_FAILED);
		}
		for(n=0,m=j; m < k; n++,m++)						/* REDUCE ENVELOPE TO FILE SECTION REQUIRED */
			pa[n] = pa[m];
		envcnt = n;

		indur = pa[envcnt-1] - pa[0];
		dz->iparam[2] += indur;								/*	TOTAL DURATION OF GRIT SECTION */

		element_cnt = envcnt-1;								/* ARRAYS FOR GRAIN PERMUTATION WITH NO REPETS */

		fprintf(stdout,"INFO: Number of grains found = %d\n",element_cnt);
		fflush(stdout);
		if(element_cnt <= 0) {
			sprintf(errstr,"Insufficient grains to proceed.\n");
			return(DATA_ERROR);
		}
		if((arr = (int *)malloc(element_cnt * sizeof(int)))==NULL) {
			sprintf(errstr,"Insufficient memory for permutation array 1.\n");
			return(MEMORY_ERROR);
		}
		if((perm = (int *)malloc(element_cnt * sizeof(int)))==NULL) {
			sprintf(errstr,"Insufficient memory for permutation array 2.\n");
			return(MEMORY_ERROR);
		}
		if((arr2 = (int *)malloc(element_cnt * sizeof(int)))==NULL) {
			sprintf(errstr,"Insufficient memory for permutation array 3.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<element_cnt;n++)
			arr[n] = n;
		memset((char *)obuf,0,dz->buflen * sizeof(float));
		obufpos = 0;										/*	READ FILE BEFORE GRIT SECTION */
		ibufpos = 0;
		origstartbuf = 0;
		origsamspread = min(dz->buflen,dz->insams[0]);
		while(pa[0] >= dz->total_samps_read) {
			last_total_samps_read = dz->total_samps_read;
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			origstartbuf = last_total_samps_read;
			origsamspread = dz->ssampsread;
			if(pa[0] >= dz->total_samps_read) {
				if((exit_status = write_samps(ibuf,dz->buflen,dz))<0)
					return(exit_status);
			} else {
				obufpos = pa[0] - last_total_samps_read;
				memcpy((char *)obuf,(char *)ibuf,obufpos * sizeof(float));
				ibufpos = obufpos;
			}
		}
		expansion = 0;
		j = element_cnt;
		endval = -1;
		while(expansion < dz->iparam[2]) {						/*	EXTEND GRIT SECTION, SELECTING GRAINS AT RANDOM (no repets) */
			if(j == element_cnt) {
				do_repet_restricted_perm(arr,perm,element_cnt,&endval);
				for(n=0;n<element_cnt;n++)
					arr2[n] = arr[perm[n]];
				j = 0;
			}
			k = arr2[j++];				;
			startsearch = pa[k];
			endsearch   = pa[k+1];
			if((exit_status = do_envgrain_addwrite(startsearch,endsearch,&last_total_samps_read,&obufpos,dz))<0)
				return(exit_status);
			expansion += endsearch - startsearch;		
		}
		if((sndseekEx(dz->ifd[0],origstartbuf,0))<0) {		/*	READ FILE AFTER GRIT SECTION */
			sprintf(errstr,"seek error 5\n");
			return(SYSTEM_ERROR);
		}
		dz->total_samps_read = origstartbuf;
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		n = ibufpos;
		while(n < origsamspread) {
			obuf[obufpos++] = ibuf[n++];
			if(obufpos >= dz->buflen) {
				if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
					return(exit_status);
				memset((char *)obuf,0,dz->buflen * sizeof(float));
				obufpos = 0;
			}
		}
		while(dz->total_samps_read < dz->insams[0]) {
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			if(dz->ssampsread == 0)
				break;
			for(n = 0;n < dz->ssampsread;n++) {
				obuf[obufpos++] = ibuf[n];
				if(obufpos >= dz->buflen) {
					if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
						return(exit_status);
					memset((char *)obuf,0,dz->buflen * sizeof(float));
					obufpos = 0;
				}
			}
		}
		break;
	}
	if(obufpos > 0) {
		if((exit_status = write_samps(obuf,obufpos,dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

/************************** DO_ENVGRAIN_ADDWRITE **********************/

int do_envgrain_addwrite(int startsearch,int endsearch,int *last_total_samps_read,int *obufpos,dataptr dz)
{
	int exit_status;
	int step, n, m;
	float *ibuf = dz->sampbuf[0], *obuf  = dz->sampbuf[1];
	if(*obufpos < 0) {
		sprintf(errstr,"GRAIN TOO LARGE TO BACKTRACK IN BUFFER.\n");
		return(GOAL_FAILED);
	}
	if(startsearch > dz->total_samps_read || startsearch < *last_total_samps_read) {
		step = (startsearch / dz->buflen) * dz->buflen; 
		if((sndseekEx(dz->ifd[0],step,0))<0) {
			sprintf(errstr,"seek error 6\n");
			return(SYSTEM_ERROR);
		}
		*last_total_samps_read = step;
		if((exit_status = read_samps(ibuf,dz))<0)
			return(exit_status);
		dz->total_samps_read = *last_total_samps_read + dz->ssampsread; 
	}
	startsearch -= *last_total_samps_read;
	endsearch   -= *last_total_samps_read;
	m = *obufpos;
	for(n=startsearch;n <endsearch;n++) {
		if(n >= dz->buflen) {
			*last_total_samps_read = dz->total_samps_read;
			if((exit_status = read_samps(ibuf,dz))<0)
				return(exit_status);
			n = 0;
			endsearch -= dz->buflen;
		}
		obuf[m++] = ibuf[n];
		if(m >= dz->buflen) {
			if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
				return(exit_status);
			memset((char *)obuf,0,dz->buflen * sizeof(float));
			m -= dz->buflen;
		}
	}
	*obufpos = m;
	return(FINISHED);
}

/************************* SETUP_GREXTEND_APPLICATION *******************/

int setup_grextend_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// SEE parstruct FOR EXPLANATION of next 2 functions
	switch(dz->process) {
	case(GREV_EXTEND):
		if((exit_status = set_param_data(ap,0   ,5,5,"ddddd"      ))<0)
			return(FAILED);
		if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
			return(FAILED);
		break;
	}
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	switch(dz->process) {
	case(GREV_EXTEND):
		dz->input_data_type = SNDFILES_ONLY;
		break;
	}
	dz->process_type	= UNEQUAL_SNDFILE;	
	dz->outfiletype  	= SNDFILE_OUT;
	return application_init(dz);	//GLOBAL
}

/************************* PARSE_INFILE_AND_CHECK_TYPE *******************/

int parse_infile_and_check_type(char **cmdline,dataptr dz)
{
	int exit_status;
	infileptr infile_info;
	if(!sloom) {
		if((infile_info = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for infile structure to test file data.");
			return(MEMORY_ERROR);
		} else if((exit_status = cdparse(cmdline[0],infile_info))<0) {
			sprintf(errstr,"Failed to parse input file %s\n",cmdline[0]);
			return(PROGRAM_ERROR);
		} else if(infile_info->filetype != SNDFILE)  {
			sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
			return(DATA_ERROR);
		} else if(infile_info->channels != 1)  {
			sprintf(errstr,"File %s is not of correct type (must be mono)\n",cmdline[0]);
			return(DATA_ERROR);
		} else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
			sprintf(errstr,"Failed to copy file parsing information\n");
			return(PROGRAM_ERROR);
		}
		free(infile_info);
	}
	return(FINISHED);
}

/************************* SETUP_GREXTEND_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_grextend_param_ranges_and_defaults(dataptr dz)
{
	int exit_status;
	aplptr ap = dz->application;
	// set_param_ranges()
	ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
	// NB total_input_param_cnt is > 0 !!!s
	if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
		return(FAILED);
	// get_param_ranges()
	switch(dz->process) {
	case(GREV_EXTEND):
		ap->lo[GREV_WSIZE]			  = (8.0/dz->infile->srate) * SECS_TO_MS;
		ap->hi[GREV_WSIZE]			  = (dz->duration/3.0) * SECS_TO_MS;
		ap->default_val[GREV_WSIZE]	  = 5;
		ap->lo[GREV_TROFRAC]		  = FLTERR;		
		ap->hi[GREV_TROFRAC]		  = 1.0 - FLTERR;
		ap->default_val[GREV_TROFRAC] = .2;		
		ap->lo[2]			= FLTERR;		
		ap->hi[2]			= 3600;
		ap->default_val[2]  = 1.0;		
		ap->lo[3]			= 0.0;		
		ap->hi[3]			= dz->duration;
		ap->default_val[3]  = 0.0;		
		ap->lo[4]			= 0.0;		
		ap->hi[4]			= dz->duration;
		ap->default_val[4]  = dz->duration;
		break;
	}
	if(!sloom)
		put_default_vals_in_all_params(dz);
	return(FINISHED);
}

/************************ HANDLE_THE_OUTFILE *********************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int exit_status;
	char *filename = (*cmdline)[0];
	if(filename[0]=='-' && filename[1]=='f') {
		dz->floatsam_output = 1;
		dz->true_outfile_stype = SAMP_FLOAT;
		filename+= 2;
	}
	if(!sloom) {
		if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
			sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
			return(DATA_ERROR);
		}
	}
	strcpy(dz->outfilename,filename);	   
	if((exit_status = create_sized_outfile(filename,dz))<0)
		return(exit_status);
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/**************************** CHECK_GREXTEND_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_grextend_param_validity_and_consistency(dataptr dz)
{
	double temp;
	switch(dz->process) {
	case(GREV_EXTEND):
		if(flteq(dz->param[4],dz->param[3])) {
			sprintf(errstr,"Incompatible start and end times for grain segment.\n");
			return(DATA_ERROR);
		}
		if(dz->param[4] < dz->param[3]) {
			temp = dz->param[4];
			dz->param[4] = dz->param[3];
			dz->param[3] = temp;
		}
		dz->iparam[2] = (int)round(dz->param[2] * dz->infile->srate);
		dz->iparam[3] = (int)round(dz->param[3] * dz->infile->srate);
		dz->iparam[4] = (int)round(dz->param[4] * dz->infile->srate);
		dz->iparam[GREV_SAMP_WSIZE] =  (int)round(dz->param[GREV_WSIZE] * MS_TO_SECS * (double)dz->infile->srate);
		dz->param[GREV_TROFRAC] /= 2.0;	/* include averaging factor here */
		dz->tempsize = dz->insams[0] + dz->iparam[2];
		break;
	}
	return(FINISHED);
}

/*************************** CREATE_GREXTEND_SNDBUFS **************************/

int create_grextend_sndbufs(dataptr dz)
{
	int n;
	size_t bigbufsize;
	if(dz->sbufptr == 0 || dz->sampbuf==0) {
		sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
		return(PROGRAM_ERROR);
	}
	bigbufsize = (size_t)Malloc(-1);
	bigbufsize /= dz->bufcnt;
	if(bigbufsize <=0)
		bigbufsize  =  F_SECSIZE * sizeof(float);	  	  /* RWD keep ths for now */
	dz->buflen = (int)(bigbufsize / sizeof(float));	
	/*RWD also cover n-channels usage */
	bigbufsize = dz->buflen * sizeof(float);
	if((dz->bigbuf = (float *)malloc(bigbufsize  * dz->bufcnt)) == NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	for(n=0;n<dz->bufcnt;n++)
		dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
	return(FINISHED);
}

/****************************** DO_REPET_RESTRICTED_PERM ****************************/

void do_repet_restricted_perm(int *arr, int *perm, int arrsiz, int *endval)
{
	int n, t;
	int done = 0;
	while(!done) {
		for(n=0;n<arrsiz;n++) {
			t = (int)(drand48() * (double)(n+1)); /* TRUNCATE */
			if(t==n)
				hhprefix(n,arrsiz,perm);
			else
				hhinsert(n,t,arrsiz,perm);
		}
		if(arr[perm[0]] != *endval) {	/* if this is val (repeated) at end of last perm */
			done = 1;					/* if this is not val at end of last perm */
		}
	}
	*endval = arr[perm[arrsiz-1]];
}

/***************************** HHINSERT **********************************
 *
 * Insert the value m AFTER the T-th element in perm[].
 */

void hhinsert(int m,int t,int setlen,int *perm)
{
	hhshuflup(t+1,setlen,perm);
	perm[t+1] = m;
}

/***************************** HHPREFIX ************************************
 *
 * Insert the value m at start of the permutation perm[].
 */

void hhprefix(int m,int setlen,int *perm)
{			  
	hhshuflup(0,setlen,perm);
	perm[0] = m;
}

/****************************** HHSHUFLUP ***********************************
 *
 * move set members in perm[] upwards, starting from element k.
 */
void hhshuflup(int k,int setlen,int *perm)
{
	int n, *i;
	int z = setlen - 1;
	i = (perm+z);
	for(n = z;n > k;n--) {
		*i = *(i-1);
		i--;
	}
}

/**************************************************/
/* GENERAL FUNCTIONS, REPLACING CDP LIB FUNCTIONS */
/**************************************************/

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

/****************************** SET_VFLGS *********************************/

int set_vflgs
(aplptr ap,char *optflags,int optcnt,char *optlist,char *varflags,int vflagcnt, int vparamcnt,char *varlist)
{
	ap->option_cnt 	 = (char) optcnt;			/*RWD added cast */
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

/***************************** APPLICATION_INIT **************************/

int application_init(dataptr dz)
{
	int exit_status;
	int storage_cnt;
	int tipc, brkcnt;
	aplptr ap = dz->application;
	if(ap->vflag_cnt>0)
		initialise_vflags(dz);	  
	tipc  = ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt;
	ap->total_input_param_cnt = (char)tipc;
	if(tipc>0) {
		if((exit_status = setup_input_param_range_stores(tipc,ap))<0)			  
			return(exit_status);
		if((exit_status = setup_input_param_defaultval_stores(tipc,ap))<0)		  
			return(exit_status);
		if((exit_status = setup_and_init_input_param_activity(dz,tipc))<0)	  
			return(exit_status);
	}
	brkcnt = tipc;
	//THERE ARE NO INPUTFILE brktables USED IN THIS PROCESS
	if(brkcnt>0) {
		if((exit_status = setup_and_init_input_brktable_constants(dz,brkcnt))<0)			  
			return(exit_status);
	}
	if((storage_cnt = tipc + ap->internal_param_cnt)>0) {		  
		if((exit_status = setup_parameter_storage_and_constants(storage_cnt,dz))<0)	  
			return(exit_status);
		if((exit_status = initialise_is_int_and_no_brk_constants(storage_cnt,dz))<0)	  
			return(exit_status);
	}													   
 	if((exit_status = mark_parameter_types(dz,ap))<0)	  
		return(exit_status);
	
	// establish_infile_constants() replaced by
	switch(dz->process) {
	case(PSOW_INTERLEAVE):
	case(PSOW_REPLACE):
		dz->infilecnt = 2;
		break;
	default:
		dz->infilecnt = 1;
		break;
	}
	if((exit_status = setup_internal_arrays_and_array_pointers(dz))<0)	 
		return(exit_status);
	//establish_bufptrs_and_extra_buffers():
	return(FINISHED);
}

/********************** SETUP_PARAMETER_STORAGE_AND_CONSTANTS ********************/
/* RWD mallo changed to calloc; helps debug verison run as release! */

int setup_parameter_storage_and_constants(int storage_cnt,dataptr dz)
{
	if((dz->param       = (double *)calloc(storage_cnt, sizeof(double)))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 1\n");
		return(MEMORY_ERROR);
	}
	if((dz->iparam      = (int    *)calloc(storage_cnt, sizeof(int)   ))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 2\n");
		return(MEMORY_ERROR);
	}
	if((dz->is_int      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 3\n");
		return(MEMORY_ERROR);
	}
	if((dz->no_brk      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
		sprintf(errstr,"setup_parameter_storage_and_constants(): 5\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/************** INITIALISE_IS_INT_AND_NO_BRK_CONSTANTS *****************/

int initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz)
{
	int n;
	for(n=0;n<storage_cnt;n++) {
		dz->is_int[n] = (char)0;
		dz->no_brk[n] = (char)0;
	}
	return(FINISHED);
}

/***************************** MARK_PARAMETER_TYPES **************************/

int mark_parameter_types(dataptr dz,aplptr ap)
{
	int n, m;							/* PARAMS */
	for(n=0;n<ap->max_param_cnt;n++) {
		switch(ap->param_list[n]) {
		case('0'):	break; /* dz->is_active[n] = 0 is default */
		case('i'):	dz->is_active[n] = (char)1; dz->is_int[n] = (char)1;dz->no_brk[n] = (char)1; break;
		case('I'):	dz->is_active[n] = (char)1;	dz->is_int[n] = (char)1; 						 break;
		case('d'):	dz->is_active[n] = (char)1;							dz->no_brk[n] = (char)1; break;
		case('D'):	dz->is_active[n] = (char)1;	/* normal case: double val or brkpnt file */	 break;
		default:
			sprintf(errstr,"Programming error: invalid parameter type in mark_parameter_types()\n");
			return(PROGRAM_ERROR);
		}
	}						 		/* OPTIONS */
	for(n=0,m=ap->max_param_cnt;n<ap->option_cnt;n++,m++) {
		switch(ap->option_list[n]) {
		case('i'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;	dz->no_brk[m] = (char)1; break;
		case('I'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1; 						 break;
		case('d'): dz->is_active[m] = (char)1; 							dz->no_brk[m] = (char)1; break;
		case('D'): dz->is_active[m] = (char)1;	/* normal case: double val or brkpnt file */	 break;
		default:
			sprintf(errstr,"Programming error: invalid option type in mark_parameter_types()\n");
			return(PROGRAM_ERROR);
		}
	}								/* VARIANTS */
	for(n=0,m=ap->max_param_cnt + ap->option_cnt;n < ap->variant_param_cnt; n++, m++) {
		switch(ap->variant_list[n]) {
		case('0'): break;
		case('i'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;	dz->no_brk[m] = (char)1; break;
		case('I'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;	 						 break;
		case('d'): dz->is_active[m] = (char)1; 							dz->no_brk[m] = (char)1; break;
		case('D'): dz->is_active[m] = (char)1; /* normal case: double val or brkpnt file */		 break;
		default:
			sprintf(errstr,"Programming error: invalid variant type in mark_parameter_types()\n");
			return(PROGRAM_ERROR);
		}
	}								/* INTERNAL */
	for(n=0,
	m=ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt; n<ap->internal_param_cnt; n++,m++) {
		switch(ap->internal_param_list[n]) {
		case('0'):  break;	 /* dummy variables: variables not used: but important for internal paream numbering!! */
		case('i'):	dz->is_int[m] = (char)1;	dz->no_brk[m] = (char)1;	break;
		case('d'):								dz->no_brk[m] = (char)1;	break;
		default:
			sprintf(errstr,"Programming error: invalid internal param type in mark_parameter_types()\n");
			return(PROGRAM_ERROR);
		}
	}
	return(FINISHED);
}

/***************************** ESTABLISH_APPLICATION **************************/

int establish_application(dataptr dz)
{
	aplptr ap;
	if((dz->application = (aplptr)malloc(sizeof (struct applic)))==NULL) {
		sprintf(errstr,"establish_application()\n");
		return(MEMORY_ERROR);
	}
	ap = dz->application;
	memset((char *)ap,0,sizeof(struct applic));
	return(FINISHED);
}

/************************* INITIALISE_VFLAGS *************************/

int initialise_vflags(dataptr dz)
{
	int n;
	if((dz->vflag  = (char *)malloc(dz->application->vflag_cnt * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY: vflag store,\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->application->vflag_cnt;n++)
		dz->vflag[n]  = FALSE;
	return FINISHED;
}

/************************* SETUP_INPUT_PARAM_DEFAULTVALS *************************/

int setup_input_param_defaultval_stores(int tipc,aplptr ap)
{
	int n;
	if((ap->default_val = (double *)malloc(tipc * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for application default values store\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<tipc;n++)
		ap->default_val[n] = 0.0;
	return(FINISHED);
}

/***************************** SETUP_AND_INIT_INPUT_PARAM_ACTIVITY **************************/

int setup_and_init_input_param_activity(dataptr dz,int tipc)
{
	int n;
	if((dz->is_active = (char   *)malloc((size_t)tipc))==NULL) {
		sprintf(errstr,"setup_and_init_input_param_activity()\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<tipc;n++)
		dz->is_active[n] = (char)0;
	return(FINISHED);
}

/******************************** SETUP_AND_INIT_INPUT_BRKTABLE_CONSTANTS ********************************/

int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt)
{	
	int n;
	if((dz->brk      = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 1\n");
		return(MEMORY_ERROR);
	}
	if((dz->brkptr   = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 6\n");
		return(MEMORY_ERROR);
	}
	if((dz->brksize  = (int    *)malloc(brkcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 2\n");
		return(MEMORY_ERROR);
	}
	if((dz->firstval = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 3\n");
		return(MEMORY_ERROR);												  
	}
	if((dz->lastind  = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 4\n");
		return(MEMORY_ERROR);
	}
	if((dz->lastval  = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 5\n");
		return(MEMORY_ERROR);
	}
	if((dz->brkinit  = (int     *)malloc(brkcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"setup_and_init_input_brktable_constants(): 7\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<brkcnt;n++) {
		dz->brk[n]     = NULL;
		dz->brkptr[n]  = NULL;
		dz->brkinit[n] = 0;
		dz->brksize[n] = 0;
	}
	return(FINISHED);
}

/********************************* PARSE_SLOOM_DATA *********************************/

int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz)
{
	int exit_status;
	int cnt = 1, infilecnt;
	int filesize, insams, inbrksize;
	double dummy;
	int true_cnt = 0;
	aplptr ap;

	while(cnt<=PRE_CMDLINE_DATACNT) {
		if(cnt > argc) {
			sprintf(errstr,"Insufficient data sent from TK\n");
			return(DATA_ERROR);
		}
		switch(cnt) {
		case(1):	
			if(sscanf(argv[cnt],"%d",&dz->process)!=1) {
				sprintf(errstr,"Cannot read process no. sent from TK\n");
				return(DATA_ERROR);
			}
			break;

		case(2):	
			if(sscanf(argv[cnt],"%d",&dz->mode)!=1) {
				sprintf(errstr,"Cannot read mode no. sent from TK\n");
				return(DATA_ERROR);
			}
			if(dz->mode > 0)
				dz->mode--;
			//setup_particular_application() =
			if((exit_status = setup_grextend_application(dz))<0)
				return(exit_status);
			ap = dz->application;
			break;

		case(3):	
			if(sscanf(argv[cnt],"%d",&infilecnt)!=1) {
				sprintf(errstr,"Cannot read infilecnt sent from TK\n");
				return(DATA_ERROR);
			}
			if(infilecnt < 1) {
				true_cnt = cnt + 1;
				cnt = PRE_CMDLINE_DATACNT;	/* force exit from loop after assign_file_data_storage */
			}
			if((exit_status = assign_file_data_storage(infilecnt,dz))<0)
				return(exit_status);
			break;
		case(INPUT_FILETYPE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->filetype)!=1) {
				sprintf(errstr,"Cannot read filetype sent from TK (%s)\n",argv[cnt]);
				return(DATA_ERROR);
			}
			break;
		case(INPUT_FILESIZE+4):	
			if(sscanf(argv[cnt],"%d",&filesize)!=1) {
				sprintf(errstr,"Cannot read infilesize sent from TK\n");
				return(DATA_ERROR);
			}
			dz->insams[0] = filesize;	
			break;
		case(INPUT_INSAMS+4):	
			if(sscanf(argv[cnt],"%d",&insams)!=1) {
				sprintf(errstr,"Cannot read insams sent from TK\n");
				return(DATA_ERROR);
			}
			dz->insams[0] = insams;	
			break;
		case(INPUT_SRATE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->srate)!=1) {
				sprintf(errstr,"Cannot read srate sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_CHANNELS+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->channels)!=1) {
				sprintf(errstr,"Cannot read channels sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_STYPE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->stype)!=1) {
				sprintf(errstr,"Cannot read stype sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ORIGSTYPE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->origstype)!=1) {
				sprintf(errstr,"Cannot read origstype sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ORIGRATE+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->origrate)!=1) {
				sprintf(errstr,"Cannot read origrate sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MLEN+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->Mlen)!=1) {
				sprintf(errstr,"Cannot read Mlen sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DFAC+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->Dfac)!=1) {
				sprintf(errstr,"Cannot read Dfac sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ORIGCHANS+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->origchans)!=1) {
				sprintf(errstr,"Cannot read origchans sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_SPECENVCNT+4):	
			if(sscanf(argv[cnt],"%d",&dz->infile->specenvcnt)!=1) {
				sprintf(errstr,"Cannot read specenvcnt sent from TK\n");
				return(DATA_ERROR);
			}
			dz->specenvcnt = dz->infile->specenvcnt;
			break;
		case(INPUT_WANTED+4):	
			if(sscanf(argv[cnt],"%d",&dz->wanted)!=1) {
				sprintf(errstr,"Cannot read wanted sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_WLENGTH+4):	
			if(sscanf(argv[cnt],"%d",&dz->wlength)!=1) {
				sprintf(errstr,"Cannot read wlength sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_OUT_CHANS+4):	
			if(sscanf(argv[cnt],"%d",&dz->out_chans)!=1) {
				sprintf(errstr,"Cannot read out_chans sent from TK\n");
				return(DATA_ERROR);
			}
			break;
			/* RWD these chanegs to samps - tk will have to deal with that! */
		case(INPUT_DESCRIPTOR_BYTES+4):	
			if(sscanf(argv[cnt],"%d",&dz->descriptor_samps)!=1) {
				sprintf(errstr,"Cannot read descriptor_samps sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_IS_TRANSPOS+4):	
			if(sscanf(argv[cnt],"%d",&dz->is_transpos)!=1) {
				sprintf(errstr,"Cannot read is_transpos sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_COULD_BE_TRANSPOS+4):	
			if(sscanf(argv[cnt],"%d",&dz->could_be_transpos)!=1) {
				sprintf(errstr,"Cannot read could_be_transpos sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_COULD_BE_PITCH+4):	
			if(sscanf(argv[cnt],"%d",&dz->could_be_pitch)!=1) {
				sprintf(errstr,"Cannot read could_be_pitch sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DIFFERENT_SRATES+4):	
			if(sscanf(argv[cnt],"%d",&dz->different_srates)!=1) {
				sprintf(errstr,"Cannot read different_srates sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DUPLICATE_SNDS+4):	
			if(sscanf(argv[cnt],"%d",&dz->duplicate_snds)!=1) {
				sprintf(errstr,"Cannot read duplicate_snds sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_BRKSIZE+4):	
			if(sscanf(argv[cnt],"%d",&inbrksize)!=1) {
				sprintf(errstr,"Cannot read brksize sent from TK\n");
				return(DATA_ERROR);
			}
			if(inbrksize > 0) {
				switch(dz->input_data_type) {
				case(WORDLIST_ONLY):
					break;
				case(PITCH_AND_PITCH):
				case(PITCH_AND_TRANSPOS):
				case(TRANSPOS_AND_TRANSPOS):
					dz->tempsize = inbrksize;
					break;
				case(BRKFILES_ONLY):
				case(UNRANGED_BRKFILE_ONLY):
				case(DB_BRKFILES_ONLY):
				case(ALL_FILES):
				case(ANY_NUMBER_OF_ANY_FILES):
					if(dz->extrabrkno < 0) {
						sprintf(errstr,"Storage location number for brktable not established by CDP.\n");
						return(DATA_ERROR);
					}
					if(dz->brksize == NULL) {
						sprintf(errstr,"CDP has not established storage space for input brktable.\n");
						return(PROGRAM_ERROR);
					}
					dz->brksize[dz->extrabrkno]	= inbrksize;
					break;
				default:
					sprintf(errstr,"TK sent brktablesize > 0 for input_data_type [%d] not using brktables.\n",
					dz->input_data_type);
					return(PROGRAM_ERROR);
				}
				break;
			}
			break;
		case(INPUT_NUMSIZE+4):	
			if(sscanf(argv[cnt],"%d",&dz->numsize)!=1) {
				sprintf(errstr,"Cannot read numsize sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_LINECNT+4):	
			if(sscanf(argv[cnt],"%d",&dz->linecnt)!=1) {
				sprintf(errstr,"Cannot read linecnt sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ALL_WORDS+4):	
			if(sscanf(argv[cnt],"%d",&dz->all_words)!=1) {
				sprintf(errstr,"Cannot read all_words sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_ARATE+4):	
			if(sscanf(argv[cnt],"%f",&dz->infile->arate)!=1) {
				sprintf(errstr,"Cannot read arate sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_FRAMETIME+4):	
			if(sscanf(argv[cnt],"%lf",&dummy)!=1) {
				sprintf(errstr,"Cannot read frametime sent from TK\n");
				return(DATA_ERROR);
			}
			dz->frametime = (float)dummy;
			break;
		case(INPUT_WINDOW_SIZE+4):	
			if(sscanf(argv[cnt],"%f",&dz->infile->window_size)!=1) {
				sprintf(errstr,"Cannot read window_size sent from TK\n");
					return(DATA_ERROR);
			}
			break;
		case(INPUT_NYQUIST+4):	
			if(sscanf(argv[cnt],"%lf",&dz->nyquist)!=1) {
				sprintf(errstr,"Cannot read nyquist sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_DURATION+4):	
			if(sscanf(argv[cnt],"%lf",&dz->duration)!=1) {
				sprintf(errstr,"Cannot read duration sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MINBRK+4):	
			if(sscanf(argv[cnt],"%lf",&dz->minbrk)!=1) {
				sprintf(errstr,"Cannot read minbrk sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MAXBRK+4):	
			if(sscanf(argv[cnt],"%lf",&dz->maxbrk)!=1) {
				sprintf(errstr,"Cannot read maxbrk sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MINNUM+4):	
			if(sscanf(argv[cnt],"%lf",&dz->minnum)!=1) {
				sprintf(errstr,"Cannot read minnum sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		case(INPUT_MAXNUM+4):	
			if(sscanf(argv[cnt],"%lf",&dz->maxnum)!=1) {
				sprintf(errstr,"Cannot read maxnum sent from TK\n");
				return(DATA_ERROR);
			}
			break;
		default:
			sprintf(errstr,"case switch item missing: parse_sloom_data()\n");
			return(PROGRAM_ERROR);
		}
		cnt++;
	}
	if(cnt!=PRE_CMDLINE_DATACNT+1) {
		sprintf(errstr,"Insufficient pre-cmdline params sent from TK\n");
		return(DATA_ERROR);
	}

	if(true_cnt)
		cnt = true_cnt;
	*cmdlinecnt = 0;		

	while(cnt < argc) {
		if((exit_status = get_tk_cmdline_word(cmdlinecnt,cmdline,argv[cnt]))<0)
			return(exit_status);
		cnt++;
	}
	return(FINISHED);
}

/********************************* GET_TK_CMDLINE_WORD *********************************/

int get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q)
{
	if(*cmdlinecnt==0) {
		if((*cmdline = (char **)malloc(sizeof(char *)))==NULL)	{
			sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline array.\n");
			return(MEMORY_ERROR);
		}
	} else {
		if((*cmdline = (char **)realloc(*cmdline,((*cmdlinecnt)+1) * sizeof(char *)))==NULL)	{
			sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline array.\n");
			return(MEMORY_ERROR);
		}
	}
	if(((*cmdline)[*cmdlinecnt] = (char *)malloc((strlen(q) + 1) * sizeof(char)))==NULL)	{
		sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline item %d.\n",(*cmdlinecnt)+1);
		return(MEMORY_ERROR);
	}
	strcpy((*cmdline)[*cmdlinecnt],q);
	(*cmdlinecnt)++;
	return(FINISHED);
}


/****************************** ASSIGN_FILE_DATA_STORAGE *********************************/

int assign_file_data_storage(int infilecnt,dataptr dz)
{
	int exit_status;
	int no_sndfile_system_files = FALSE;
	dz->infilecnt = infilecnt;
	if((exit_status = allocate_filespace(dz))<0)
		return(exit_status);
	if(no_sndfile_system_files)
		dz->infilecnt = 0;
	return(FINISHED);
}

/****************************** SET_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	return(FINISHED);		
}

/************************* SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS *******************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	return(FINISHED);
}

/************************* redundant functions: to ensure libs compile OK *******************/

int assign_process_logic(dataptr dz)
{
	return(FINISHED);
}

void set_legal_infile_structure(dataptr dz)
{}

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	return(FINISHED);
}

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
	return(FINISHED);
}

int read_special_data(char *str,dataptr dz)	
{
	return(FINISHED);
}
