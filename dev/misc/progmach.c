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



/******** This program is called from TK/TCL to check file compatibility ******
	***** and hence determine which programs will run with current files ******

					see progmach.tk

INPUT PARAMETERS TO THIS PROGRAM ARE

(a)	#THESE ACCUMULATING FLAGS
	1)	Count of input files
	2)	Filetype of 1st infile
	3)	Filetype of current infile
	4)	shared- filetypes
	5)	Bitflag re file compatibility (16+8+4+2)
	#			16 = ALL INFILES OF SAME TYPE
	#			8  = ALL INFILES OF SAME SRATE
	#			4  = ALL INFILES OF SAME CHANNEL COUNT
	#			2  = ALL INFILES HAVE ALL OTHER PROPERTIES COMPATIBLE
	#			1  = AT LEAST 1 OF INFILES IS A BINARY FILE
	#PROPS OF 1ST FILE (ONCE ASCERTAINED)
	6)	srate
	7)	chans
	8)	arate
	9)	stype
	10)	origstype
	11)	origrate
	12)	Mlen
	13)	Dfac
	14)	origchans
	15)	specenvcnt
	16)	descriptor_samps

b)	#THE FOLLOWING PROPERTIES OF THE CURRENT INPUT FILE
	17)	filetype
	18)	srate
	19)	channels
	20)	arate
	21)	stype
	22)	origstype
	23)	origrate
	24)	Mlen
	25)	Dfac
	26)	origchans
	27)	specenvcnt
	28)	descriptor_samps
***********/

#include <stdlib.h>
//TW UPDATE
#include <osbind.h>
#include <stdio.h>
#include <ctype.h>
#include "filetype.h"
#include <string.h>

#define	SAME_FILETYPE_BIT	(16)
#define	SAME_SRATE_BIT		(8)
#define	SAME_CHANNELS_BIT	(4)
#define	SAME_PROPS_BIT		(2)
#define	ONE_BINFILE_BIT		(1)

/* convert filetype to filetype-bit N  -> 2^(N-1) */

#define	BRKFILE_BIT			 (1)		/* #define	BRKFILE			 (1) */
#define	DB_BRKFILE_BIT		 (2)		/* #define	DB_BRKFILE		 (2) */
#define UNRANGED_BRKFILE_BIT (4)		/* #define UNRANGED_BRKFILE  (3) */
#define NUMLIST_BIT			 (8)		/* #define NUMLIST			 (4) */
#define SNDLIST_BIT		 	 (16)		/* #define SNDLIST		 	 (5) */
#define SYNCLIST_BIT		 (32)		/* #define SYNCLIST		 	 (6) */
#define MIXFILE_BIT		     (64)		/* #define MIXFILE		     (7) */
#define LINELIST_BIT		 (128)		/* #define LINELIST		 	 (8) */
#define TEXTFILE_BIT		 (256)		/* #define TEXTFILE		 	 (9) */
#define	SNDFILE_BIT			 (512)		/* #define	SNDFILE			 (10) */
#define	ANALFILE_BIT		 (1024)		/* #define	ANALFILE		 (11) */
#define	PITCHFILE_BIT		 (2048)		/* #define	PITCHFILE		 (12) */
#define TRANSPOSFILE_BIT	 (4096)		/* #define TRANSPOSFILE	 	 (13) */
#define	FORMANTFILE_BIT		 (8192)		/* #define	FORMANTFILE		 (14) */
#define	ENVFILE_BIT			 (16384)	/* #define	ENVFILE			 (15) */

#define SPECTRAL_BITS		 (15460)	/* All spectral type files */
#define POSTANAL_BITS		 (14336)	/* pitch, transpos & formant files */
#define BINFILE_BITS		 (32256)	/* All sndsys files: i.e. snd + spectral + envel */
char	errstr[400];

int convert_filetype_to_bit(int filetype);
int flteq(double f1,double f2);
unsigned int hz1000();
const char* cdp_version = "7.1.0";

int main(int argc, char *argv[])
{
	int 	filecnt, filetype, bitflag, channels;
	int 	stype, origstype, Mlen, Dfac, origchans, specenvcnt, descriptor_samps;
	int		srate, origrate, S_filetype;
	double 	arate;
//TW UPDATE
	unsigned int start = hz1000();
	
	int		N_filetype, N_channels, N_stype, N_origstype, N_Mlen, N_Dfac;
	int		N_origchans, N_specenvcnt, N_descriptor_samps;
	int		N_srate, N_origrate;
	double 	N_arate;
	
	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(argc!=29) {
		fprintf(stdout,"ERROR: Bad number of arguments to progmach.\n");
//TW UPDATE, as are all the following similar timer-insertions
		while (!(hz1000() - start))
			;
		return -1;	   	/* is this correct way to return an error to TK ?? Ditto below!! */
	}
	if(sscanf(argv[1],"%d",&filecnt)!=1) {
		fprintf(stdout,"ERROR: Bad filecnt argument to progmach.\n");
		while (!(hz1000() - start))
			;
		return -1;
	}
	if(sscanf(argv[5],"%d",&bitflag)!=1) {
		fprintf(stdout,"ERROR: Bad bitflag argument to progmach.\n");
		while (!(hz1000() - start))
			;
		return -1;
	}

	if(filecnt < 0) {
		fprintf(stdout,"ERROR: Bad value (%d) for filecnt in progmach.\n",filecnt);
		while (!(hz1000() - start))
			;
		return -1;
	} else if(filecnt==0) {

	/* GET PARAMETERS FROM THE INFILE */

		if(sscanf(argv[17],"%d", &filetype)!=1) {
			fprintf(stdout,"ERROR: Bad filetype argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[18],"%d",&srate)!=1) {
			fprintf(stdout,"ERROR: Bad srate argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[19],"%d", &channels)!=1) {
			fprintf(stdout,"ERROR: Bad channels argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[20],"%lf",&arate)!=1) {
			fprintf(stdout,"ERROR: Bad arate argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[21],"%d", &stype)!=1) {
			fprintf(stdout,"ERROR: Bad stype argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[22],"%d", &origstype)!=1) {
			fprintf(stdout,"ERROR: Bad origstype argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[23],"%d",&origrate)!=1) {
			fprintf(stdout,"ERROR: Bad origrate argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[24],"%d", &Mlen)!=1) {
			fprintf(stdout,"ERROR: Bad Mlen argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[25],"%d", &Dfac)!=1) {
			fprintf(stdout,"ERROR: Bad Dfac argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[26],"%d", &origchans)!=1) {
			fprintf(stdout,"ERROR: Bad origchans argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
	 	if(sscanf(argv[27],"%d", &specenvcnt)!=1) {
			fprintf(stdout,"ERROR: Bad specenvcnt argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[28],"%d", &descriptor_samps)!=1) {
			fprintf(stdout,"ERROR: Bad descriptor_samps argument to progmach from file %d.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		N_filetype = filetype;	  	/* current filetype = first filetype */
 		S_filetype = filetype;		/* shared filetype  = first filetype */

	} else {	/* NOT THE FIRST FILE */

	/* GET PARAMETERS FROM THE CUMULATIVE FLAG */

		if(sscanf(argv[2],"%d",&filetype)!=1) {
			fprintf(stdout,"ERROR: Bad file-0-type argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[4],"%d",&S_filetype)!=1) {
			fprintf(stdout,"ERROR: Bad shared-filetypes argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[6], "%d",&srate)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 sample-rate argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[7], "%d",&channels)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 channels argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[8], "%lf",&arate)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 analysis-rate argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[9], "%d",&stype)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 sample-type argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[10], "%d",&origstype)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 original-sample-type argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[11],"%d",&origrate)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 original-sample-rate argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[12],"%d",&Mlen)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 Mlen argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[13],"%d",&Dfac)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 decimation-factor argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[14],"%d",&origchans)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 original-channels argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[15],"%d",&specenvcnt)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 specenvcnt argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[16],"%d",&descriptor_samps)!=1) {
			fprintf(stdout,"ERROR: Bad file_0 descriptor_samps argument to progmach.\n");
			while (!(hz1000() - start))
				;
			return -1;
		}

	/* THEN GET PARAMETERS FROM THE INFILE */

		if(sscanf(argv[17],"%d", &N_filetype)!=1) {
			fprintf(stdout,"ERROR: Bad file %d filetype argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;					
		}
/**
		N_filetype = convert_filetype_to_bit(N_filetype);
**/
		if(sscanf(argv[18],"%d",&N_srate)!=1) {
			fprintf(stdout,"ERROR: Bad file %d sample-rate argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[19],"%d", &N_channels)!=1) {
			fprintf(stdout,"ERROR: Bad file %d channels argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[20],"%lf",&N_arate)!=1) {
			fprintf(stdout,"ERROR: Bad file %d analysis-rate argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;		
		}
		if(sscanf(argv[21],"%d", &N_stype)!=1) {
			fprintf(stdout,"ERROR: Bad file %d sample-type argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[22],"%d", &N_origstype)!=1) {
			fprintf(stdout,"ERROR: Bad file %d original-sample-type argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[23],"%d",&N_origrate)!=1) {
			fprintf(stdout,"ERROR: Bad file %d original-sample-rate argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[24],"%d", &N_Mlen)!=1) {
			fprintf(stdout,"ERROR: Bad file %d Mlen argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[25],"%d", &N_Dfac)!=1) {
			fprintf(stdout,"ERROR: Bad file %d decimation-factor argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[26],"%d", &N_origchans)!=1) {
			fprintf(stdout,"ERROR: Bad file %d original-channels argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
	 	if(sscanf(argv[27],"%d", &N_specenvcnt)!=1) {
			fprintf(stdout,"ERROR: Bad file %d specenvcnt argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}
		if(sscanf(argv[28],"%d", &N_descriptor_samps)!=1) {
			fprintf(stdout,"ERROR: Bad file %d descriptor_samps argument to progmach.\n",filecnt+1);
			while (!(hz1000() - start))
				;
			return -1;
		}

	/* TEST THE NEW FILE AGAINST THE FIRST FILE */

		if(bitflag & SAME_FILETYPE_BIT) {				/* if not same filetype */
			if(N_filetype & IS_A_SNDSYSTEM_FILE) {
				if(N_filetype!=filetype) {
					bitflag &= (~SAME_FILETYPE_BIT);	/* unset SAME_FILETYPE_BIT */
				}				
			} else if((S_filetype &= N_filetype)==0) 	/* generate shared-filetypes */
				bitflag &= (~SAME_FILETYPE_BIT);		/* unset SAME_FILETYPE_BIT */
		}									

		/* if not same srate */
		if((bitflag & SAME_SRATE_BIT) && N_srate!=srate)
			bitflag &= (~SAME_SRATE_BIT);				/* unset SAME_SRATE_BIT */

		if(bitflag & SAME_CHANNELS_BIT) {
														/* if 1 file is analfile, and other is anal-derived */
													 	/* compare channels with origchans: if not equal */
			if((filetype == IS_AN_ANALFILE) && ((N_filetype & IS_A_PTF_BINFILE)>ANALFILE)) {
				if(channels != N_origchans)
					bitflag &= (~SAME_CHANNELS_BIT);	/* unset SAME_CHANNELS_BIT */
			} else if(((filetype & IS_A_PTF_BINFILE) > ANALFILE) && (N_filetype == IS_AN_ANALFILE)) {
		 		if(N_channels != origchans)				 
					bitflag &= (~SAME_CHANNELS_BIT);	/* simil */
			} else if(channels != N_channels)			/* else simply compare channel-count, & if not equal */
				bitflag &= (~SAME_CHANNELS_BIT);		/* unset SAME_CHANNELS_BIT */
		}

		if(bitflag & SAME_PROPS_BIT) {				    
									  					/* If NOT (an analfile + a derived-from-anal file) */
			if(!((filetype == IS_AN_ANALFILE) && ((N_filetype & IS_A_PTF_BINFILE)>ANALFILE))
		    && !(((filetype & IS_A_PTF_BINFILE)>ANALFILE) && (N_filetype == IS_AN_ANALFILE))
			&& origchans != N_origchans)				/* compare origchans, and if not equal.. */
				bitflag &= (~SAME_PROPS_BIT);			/* unset SAME_PROPS_BIT */

														/* If both are formants files */
														/* & formant properties don't tally...*/
			if((filetype == IS_A_FORMANTFILE) && (N_filetype == IS_A_FORMANTFILE)
			&&((specenvcnt != N_specenvcnt) || (descriptor_samps != N_descriptor_samps)))
				bitflag &= (~SAME_PROPS_BIT);			/* unset SAME_PROPS_BIT */
			if(filetype != IS_A_SNDFILE) {
				if((!flteq(arate,N_arate))					/* compare all other properties */
//TW JUNE 2004: stype compatibility is no longer relevant: all internal calcs are floating point!!
			/*	|| (origstype 		 != N_origstype) */
				|| (origrate  		 != N_origrate)
				|| (Mlen 	  		 != N_Mlen)
				|| (Dfac 	  		 != N_Dfac)
//TW JUNE 2004: stype compatibility is no longer relevant: all internal calcs are floating point!!
			/*	|| (stype 			 != N_stype) */)
					bitflag &= (~SAME_PROPS_BIT);			/* and if they don't tally. unset SAME_PROPS_BIT */
			}
		}
	}

	/* ARE THERE ANY BINARY P or T FILES, SO FAR */
	/* Strictly speaking I'm being overlax in allowing binary Formant files here */ 
	/*	but as there are no non-binary formant-files, no need to test for them!! */

	if(!(bitflag & ONE_BINFILE_BIT)) {		       
		if((N_filetype & IS_A_PTF_BINFILE)>ANALFILE)	/* IF current file is a binary filetype */
			bitflag |= ONE_BINFILE_BIT;					/* set (at-least-one-binary-file)-BIT */
	}

	/* INCREMENT FILECNT */
	filecnt++;

	/* PRINT OUT THE CUMULATIVE FLAG */

	fprintf(stdout,"%d %d %d %d %d %d %d %lf %d %d %d %d %d %d %d %d\n",
			filecnt,filetype,N_filetype,S_filetype,bitflag,srate,channels,arate,stype,
			origstype,origrate,Mlen,Dfac,origchans,specenvcnt,descriptor_samps);
	return 0;
}

/**************************** FLTEQ *******************************/

#define FLTERR (0.000002)

int flteq(double f1,double f2)
{   
	double upperbnd, lowerbnd;
	upperbnd = f2 + FLTERR;		
	lowerbnd = f2 - FLTERR;		
	if((f1>upperbnd) || (f1<lowerbnd))
		return(0);
	return(1);
}

