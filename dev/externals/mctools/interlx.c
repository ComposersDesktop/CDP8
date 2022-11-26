/*
 * Copyright (c) 1983-2013 Richard Dobson and Composers Desktop Project Ltd
 * http://people.bath.ac.uk/masrwd
 * http://www.composersdesktop.com
 * This file is part of the CDP System.
 * The CDP System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *
 * The CDP System is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with the CDP System; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
 
/*TODO: fix bug writing header when using -x with .amb extension (omits cbSize setting) */

/*interlx.c */
/* v1.3. nov 2005; support placeholder arg for silent channel */
/* v 1.7 beta ; added surr 5.0; updated sfsys with bit-correct 24bit copy */
/* v 1.8 March 2009 updated sfsys  for AIFC int24 suport */

/* OCT 2009 portsf(64) version. int24 aifc supported for reading. */
/* v2.0.1 Jan 2010: Corrected usage message to refer to outfile.  */
/* Nov 2013 added MC_SURR_6_1 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include "portsf.h"

/*
The number of channels defines the order of the soundfield:
 3 channel = h   = 1st order horizontal
 4 channel = f   = 1st order 3-D
 5 channel = hh  = 2nd order horizontal
 6 channel = fh  = 2nd order horizontal + 1st order height (formerly called 2.5 order)
 7 channel = hhh = 3rd order horizontal
 8 channel = fhh = 3rd order horizontal + 1st order height
 9 channel = ff  = 2nd order 3-D
11 channel = ffh = 3rd order horizontal + 2nd order height
16 channel = fff = 3rd order 3-D
*/

#define N_BFORMATS (10)
static const int bformats[N_BFORMATS] = {2,3,4,5,6,7,8,9,11,16};
#define MAX_INFILES (16)	//should keep most people happy! Won't get much more on a cmdline anyway

enum {OPT_STD_WAVE,OPT_WAVEX_GENERIC,OPT_WAVEX,OPT_WAVEX_LCRS,OPT_WAVEX_SURROUND,OPT_WAVEX_BFORMAT,
   OPT_SURR_5_0,OPT_WAVEX_7_1,OPT_WAVEX_CUBE,OPT_WAVEX_6_1, OPT_MAXOPTS};

#ifdef unix
/* in portsf.lib */
extern int stricmp(const char *a, const char *b);
#endif

#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif

void
usage()
{
	fprintf(stderr,"\nCDP MCTOOLS: INTERLX v2.1.0 (c) RWD,CDP 2009,2013\n"
			"Interleave mono or stereo files into a multi-channel file\n"
			"Usage: interlx [-tN] outfile infile1 infile2 [infile3...]\n"
			
			"       Up to %d files may be interleaved.\n"						
			"       Output format is taken from infile1.\n"
			"       Files must match sample rate and number of channels,\n"
			"          but can have different sample types.\n"
			"       To create a silent channel, for infile2 onwards,\n"
			"          use 0 (zero) as filename. Infile1 must be a soundfile.\n"
			"       NB: Speaker-positions in WAVE_EX infiles are ignored\n"
			"       Note that the same infile can be listed multiple times,\n"
			"          for example, to write a mono file as stereo, quad, etc.\n"
			"       The .amb B-Format extension is supported: \n"
			"          the program warns if channel count is anomalous.\n"
			"       recognised Bformat channel counts: 3,4,5,6,7,8,9,11,16.\n"
            "   -tN  : write outfile format as type N\n"
	        "    Available formats:\n"
			"    0   : (default) standard soundfile (.wav, .aif, .afc, .aifc)\n"
			"    1   : generic WAVE_EX (no speaker assignments)\n"
			"    2   : WAVE_EX mono/stereo/quad(LF,RF,LR,RR)   - total chans must match.\n"
			"    3   : WAVE_EX quad surround (L,C,R,S)         - total chans must be 4.\n"
			"    4   : WAVE_EX 5.1 format surround             - total chans must be 6.\n"
			"    5   : WAVE_EX Ambisonic B-format (W,X,Y,Z...) - extension .amb recommended.\n"
            "    6   : WAVE_EX 5.0 surround                    - total chans must be 5.\n"
            "    7   : WAVE_EX 7.1 Surround                    - total chans must be 8.\n"
            "    8   : WAVE_EX Cube Surround                   - total chans must be 8.\n"
            "    9   : WAVE_EX 6.1 surround (new in v 2.1.0)   - total chans must be 7.\n"
			"          in all cases: outfile has sample format of infile1\n"
			"          NB: types 1 to %d are for WAV format only\n"			
			,MAX_INFILES,OPT_MAXOPTS-1);
	
}
	

void cleanup(int *sflist)
{

	int i;
	for(i=0;i < MAX_INFILES; i++)
		if(sflist[i] >= 0)
			psf_sndClose(sflist[i]);
	psf_finish();
}

int main(int argc, char *argv[])
{

	long outframesize,thissize;
	int i,ofd;
	int ifdlist[MAX_INFILES];
	//int force_stype = -1,out_stype;
    int force_wave = 0;
	int infilearg,inchans;
	int num_infiles = 0;
	int halfsec;
    MYLONG peaktime;
	float *outframe	= NULL,*inframe = NULL;
	PSF_PROPS firstinprops,inprops;
	PSF_CHPEAK *fpeaks = NULL;
    int wave_type = -1;
    char *create_msg = NULL;
    psf_format informat = PSF_STDWAVE;
	char* p_dot = NULL;		/* to find extension of outfile */
	
	for(i=0;i < MAX_INFILES;i++)
		ifdlist[i] = -1;

    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("2.0.1.\n");
        return 0;
    }
    
	if(argc < 4) {
		fprintf(stderr,"interlx: insufficient arguments\n");
		usage();
		exit(1);
	}

	while(argv[1][0] =='-'){
		
		switch(argv[1][1]){
        case('t'):
			if(argv[1][2]=='\0'){
				fprintf(stderr,"-t flag requires parameter\n");
				usage();
				exit(1);
			}
			wave_type = atoi(&(argv[1][2]));
			if((wave_type < 0) || wave_type >= OPT_MAXOPTS){
				fprintf(stderr,"wave type out of range\n");
				usage();
				exit(1);
			}
			force_wave = 1;
			break;
		default:
			fprintf(stderr,"\nabfpan: error: illegal flag option %s",argv[1]);
			exit(1);
		}

		argc--; argv++;
	}

	if(argc < 4){
		fprintf(stderr,"interlx error: at least two infiles required!\n");
		usage();
		exit(1);
	}

	if(psf_init()){
		printf("Startup failure.\n");
		return  1;
	}
	
	//open first infile and get properties
	ifdlist[0] = psf_sndOpen(argv[2],&firstinprops,0);
	if(ifdlist[0] < 0){
		fprintf(stderr,"unable to open infile %s\n",argv[2]);
		cleanup(ifdlist);
		return 1;
	}

/* we don't know how to deal with speaker positions yet, so disregard these
	if(firstinprops.chformat > MC_STD){
		printf(stderr,"Warning,interlx: ignoring source file speaker positions\n");
	}
*/
	outframesize = psf_sndSize(ifdlist[0]);
	if(outframesize < 0){
		fprintf(stderr,"unable to read size of infile %s\n",argv[2]);
		cleanup(ifdlist);
		return 1;
	}
	inchans = firstinprops.chans;
	/*we can always allow more channels if someone really needs it! */
	if(!(inchans==1 || inchans==2)){
		fprintf(stderr,"interlx: error: infile %s has %d channels\n"
				"\t(only mono and stereo files can be used)\n",argv[2],inchans);
		cleanup(ifdlist);
		return 1;
	}
	
	num_infiles = 1;
	printf("interleaving %d-channel files,sample rate = %d\n",inchans,firstinprops.srate);

	infilearg = 3;
	while(argv[infilearg] != NULL){
		if(strcmp(argv[infilearg],"0")==0){
			ifdlist[num_infiles] = -1;           // mark silent channel
		}
		else{

			if((ifdlist[num_infiles] = psf_sndOpen(argv[infilearg],&inprops,0)) < 0){
				fprintf(stderr,"cannot open infile %s\n",argv[infilearg]);
				cleanup(ifdlist);
				return 1;
			}
			if(inprops.chans != firstinprops.chans){
				fprintf(stderr,"interlx: error: channel mismatch from infile %s\n",argv[infilearg]);
				cleanup(ifdlist);
				return 1;
			}

			if(inprops.srate != firstinprops.srate){
			   fprintf(stderr,"interlx: error: sample rate mismatch from infile %s\n",argv[infilearg]);
				cleanup(ifdlist);
				return 1;
			}

			thissize = psf_sndSize(ifdlist[num_infiles]);
			if(thissize < 0){
				fprintf(stderr,"unable to read size of infile %s\n",argv[infilearg]);
				cleanup(ifdlist);
				return 1;
			}			
			outframesize = max(outframesize,thissize);
		}
		
		infilearg++;
		num_infiles++;
		if(num_infiles > MAX_INFILES){
			fprintf(stderr,"Sorry! too many infiles. Maximum accepted is %d.\n",MAX_INFILES);
			cleanup(ifdlist);
			exit(1);
		}

	}

	
	inframe = malloc(inchans * sizeof(float));
	if(inframe==NULL){
		puts("interlx: error: no memory for input buffer!\n");
		cleanup(ifdlist);
		return 1;
	}

	firstinprops.chans *= num_infiles;
	outframe = (float *) malloc(firstinprops.chans * sizeof(float));
	if(outframe==NULL){
		puts("\ninterlx: error: no memory for output buffer!\n");
		cleanup(ifdlist);
		return 1;
	}

	fpeaks = (PSF_CHPEAK *) calloc(firstinprops.chans,sizeof(PSF_CHPEAK));
	if(fpeaks==NULL){
		puts("interlx: error: no memory for internal PEAK buffer\n");
		cleanup(ifdlist);
		return 1;
	}
    if(force_wave){
		int i,matched;
		
		switch(wave_type){
        case(OPT_WAVEX_GENERIC):
			inprops.chformat = MC_STD;
			informat = PSF_WAVE_EX;
			create_msg = "creating STD WAVE_EX file";
			break;
		case(OPT_WAVEX):
			switch(firstinprops.chans){
			case(1):
				firstinprops.chformat = MC_MONO;
				informat = PSF_WAVE_EX;
				create_msg = "creating MONO WAVE_EX file";
				break;
			case(2):
				firstinprops.chformat = MC_STEREO;
				informat = PSF_WAVE_EX;
				create_msg = "creating STEREO WAVE_EX file";
				break;
			case(4):
				firstinprops.chformat = MC_QUAD;
				informat = PSF_WAVE_EX;
				create_msg = "creating QUAD WAVE_EX file";
				break;
			default:
				fprintf(stderr,"infile nchans incompatible with requested WAVE-EX format\n");
				usage();
				cleanup(ifdlist);
				return 1;				
			}
			break;
		case(OPT_WAVEX_LCRS):
			if(firstinprops.chans != 4){
				fprintf(stderr,"result must have four channels\n");
				usage();
				cleanup(ifdlist);
				return 1;
			}
			firstinprops.chformat = MC_LCRS;
			informat = PSF_WAVE_EX;
			create_msg = "creating LCRS-surround WAVE_EX file";
			break;
		case(OPT_WAVEX_SURROUND):
			if(firstinprops.chans != 6){
				fprintf(stderr,"result must have six channels\n");
				usage();
				cleanup(ifdlist);
				exit(1);
			}
			firstinprops.chformat = MC_DOLBY_5_1;
			informat = PSF_WAVE_EX;
			create_msg = "creating 5.1 surround WAVE_EX file";
			break;
            
        case(OPT_SURR_5_0):
			if(firstinprops.chans != 5){
				fprintf(stderr,"result must have five channels.\n");
				usage();
				cleanup(ifdlist);
				return 1;
			}
			firstinprops.chformat = MC_SURR_5_0;
			informat = PSF_WAVE_EX;
			create_msg = "creating 5.0 surround WAVE_EX file";
			break;

		case(OPT_WAVEX_BFORMAT):
			matched = 0;
			for(i=0;i < N_BFORMATS;i++)	{
				if(firstinprops.chans == bformats[i]){
					matched = 1;
					break;
				}
			}
			if(!matched){
				printf("WARNING: No Bformat definition for %d-channel file.\n",inprops.chans);
			}
			firstinprops.chformat = MC_BFMT;
			informat = PSF_WAVE_EX;
			create_msg = "creating AMBISONIC B-FORMAT WAVE_EX file";
			break;
        case OPT_WAVEX_7_1:
            if(firstinprops.chans != 8){
				fprintf(stderr,"result must have  channels\n");
				usage();
				cleanup(ifdlist);
				return 1;
			}
			firstinprops.chformat = MC_SURR_7_1;
			informat = PSF_WAVE_EX;
			create_msg = "creating 7.1 surround WAVE_EX file";
			break;
        case OPT_WAVEX_CUBE:
            if(firstinprops.chans != 8){
				fprintf(stderr,"result must have  channels\n");
				usage();
				cleanup(ifdlist);
				return 1;
			}
			firstinprops.chformat = MC_CUBE;
			informat = PSF_WAVE_EX;
			create_msg = "creating cube surround WAVE_EX file";
			break;
		case OPT_WAVEX_6_1:
            if(firstinprops.chans != 7){
				fprintf(stderr,"result must have  channels\n");
				usage();
				cleanup(ifdlist);
				return 1;
			}
			firstinprops.chformat = MC_SURR_6_1;
			informat = PSF_WAVE_EX;
			create_msg = "creating 6.1 surround WAVE_EX file";
			break;
		default:
            inprops.chformat = STDWAVE;
			informat = PSF_STDWAVE;
			create_msg = "creating plain sound file";
			break;
		}		
	}
	
	/*  want to avoid WAVE_EX if just plain WAVE possible */
	firstinprops.format = informat;
		/*firstinprops.chformat = MC_STD;*/		/* RWD April 2006 do this here? */
	
	p_dot = strrchr(argv[1],'.');
	if(stricmp(++p_dot,"amb")==0) {
		int i;
		int matched = 0;
		firstinprops.chformat = MC_BFMT;
		for(i=0;i < N_BFORMATS;i++)	{
			if(firstinprops.chans == bformats[i]){
				matched = 1;
				break;
			}
		}
		if(!matched)
			printf("\nWARNING: channel count %d unknown for BFormat.\n",firstinprops.chans);

	}
    if(!is_legalsize(outframesize,&firstinprops)){
        fprintf(stderr,"error: outfile size %ld exceeds capacity of format.\n",outframesize);
        return 1;
    }
    printf("\n%s: %d channels, %ld frames.\n",create_msg,firstinprops.chans,outframesize);
	ofd = psf_sndCreate(argv[1],&firstinprops,0,0,PSF_CREATE_RDWR);
	
	
	if(ofd < 0){
		fprintf(stderr,"interlx: error: unable to create outfile %s.\n",argv[1]);
		cleanup(ifdlist);
		return 1;
	}

	halfsec = firstinprops.srate / 2;
	for(i=0;i < outframesize; i++){		// frame loop
		float *p_framesamp,*p_filesamp;
		int j;
		
		p_framesamp = outframe;
		memset((char *)outframe,0,firstinprops.chans * sizeof(float));

		for(j=0;j < num_infiles;j++) {	//file loop
			int k,got;
            
			memset((char *)inframe,0,inchans * sizeof(float));
			if(ifdlist[j] < 0){
				got = inchans; // placeholder - write silent channel
			}
			else{
				if((got = psf_sndReadFloatFrames(ifdlist[j],inframe,1)) < 0){
					fprintf(stderr,"interlx: error reading from infile %s\n",argv[2+j]);
					psf_sndClose(ofd);					
					cleanup(ifdlist);
					return 1;
				}
			}
			if(got==1){
				p_filesamp = inframe;
				for(k=0;k < inchans;k++)	//channel loop
					*p_framesamp++	= *p_filesamp++;
			}
			else
				p_framesamp += inchans;
		}
		
		if(psf_sndWriteFloatFrames(ofd,outframe,1) < 0){
			fprintf(stderr,"interlx: error writing to outfile\n");
			psf_sndClose(ofd);            
            cleanup(ifdlist);
            return 1;
		}
        if(i % halfsec==0) {
			printf("%.2lf secs\r",(double)i / (double) firstinprops.srate);
            fflush(stdout);
        }
	}
	printf("%.4lf secs\nWritten %ld sample frames to %s\n",(double)outframesize / (double)firstinprops.srate,outframesize,argv[1]);
	if(psf_sndReadPeaks( ofd,fpeaks,&peaktime)){
        printf("PEAK data:\n");
        for(i = 0; i < firstinprops.chans; i++) {
            double val, dbval;
            val = (double) fpeaks[i].val;
            dbval = 20.0 * log10(val);
            printf("CH %d: %.6f (%.2lfdB) at frame %u:\t%.4f secs\n",i,
                   val,dbval,(unsigned int) fpeaks[i].pos,(double)fpeaks[i].pos / (double) firstinprops.srate);
        }
    }
	
	printf("\n");
	psf_sndClose(ofd);
	free(inframe);
	free(outframe);
    if(fpeaks)
        free(fpeaks);
	cleanup(ifdlist);
    
	return 0;
}
