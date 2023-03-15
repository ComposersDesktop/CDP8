/*
 * Copyright (c) 1983-2023 Richard Dobson and Composers Desktop Project Ltd
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
 
/*
 *	copysfx: version of copysfx using portsf
 */
/* Oct 2009: added cube */
/* Dec 2009 1.9.1. fixed horrible bug failing to convert to WAVE_EX! */
/* March 2012 corrected wave type arg parsing  */
/* August 2012 updated portsf to correct SPKRS_MONO */
/* Nov 2013 added MC_SURR_6_1 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef _WIN32
#include <malloc.h>			//RWD.6.5.99
#endif
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <portsf.h>

#define VERSION "Revision: 2.1.1 2020 "

#ifdef unix
/* in portsf.lib */
extern int stricmp(const char *a, const char *b);
#endif

enum {OPT_STD_WAVE,OPT_WAVEX_GENERIC,OPT_WAVEX,OPT_WAVEX_LCRS,OPT_WAVEX_SURROUND,OPT_WAVEX_BFORMAT,OPT_WAVEX_5_0,OPT_WAVEX_7_1,OPT_WAVEX_CUBE,OPT_WAVEX_6_1,};
/*
The number of channels defines the order of the soundfield:
 3 channel = h   = 1st order horizontal
 4 channel = f   = 1st order 3-D
 5 channel = hh  = 2nd order horizontal
 6 channel = fh  = 2nd order horizontal + 1st order height (formerly
called 2.5 order)
 7 channel = hhh = 3rd order horizontal
 8 channel = fhh = 3rd order horizontal + 1st order height
 9 channel = ff  = 2nd order 3-D
11 channel = ffh = 3rd order horizontal + 2nd order height
16 channel = fff = 3rd order 3-D
*/
#define N_BFORMATS (10)
static const int bformats[N_BFORMATS] = {2,3,4,5,6,7,8,9,11,16};

int get_sampsize(psf_stype type)
{
    int size = 0;
    switch(type){
		case PSF_SAMP_8:
			size = 1;
			break;
		case PSF_SAMP_16:
			size = 2;
			break;
		case PSF_SAMP_24:
			size = 3;
			break;
		case PSF_SAMP_32:
		case PSF_SAMP_IEEE_FLOAT:
			size = 4;
			break;
		default:
			break;
    }
    return size;
}



void
usage()
{
	fprintf(stderr,"\n\nCDP MCTOOLS: COPYSFX  (c) RWD,CDP %s\n",VERSION);
	fprintf(stderr,"\tcopy/convert soundfiles\n");
	fprintf(stderr,"usage: copysfx [-d][-h][-sN] [-tN] infile outfile\n");
	fprintf(stderr,"-d   : apply TPDF dither to 16bit outfile.\n"
                   "-s   : force output sample type to type N.\n");
	fprintf(stderr,"      Available sample types:\n"
                   "      1  : 16bit integers (shorts)\n"
                   "      2  : 32bit integer  (longs)\n"
                   "      3  : 32bit floating-point\n"
                   "      4  : 24bit integer 'packed' \n"
                   "      Default: format of infile.\n");
				   
	fprintf(stderr,"-h   : write mimimum header (no extra data).\n"
				   "       Default: include PEAK data.\n"
            );
/*	fprintf(stderr,"-i   : interpret 32bit files as floats (use with care!)\n"); */
	fprintf(stderr,"-tN  : write outfile format as type N\n");
	fprintf(stderr,"Possible formats:\n"
					"0   : standard soundfile (.wav, .aif, .afc, .aifc)\n"
					"1   : generic WAVE_EX (no speaker assignments)\n"
					"2   : WAVE_EX mono/stereo/quad(LF,RF,LR,RR) - infile nchans must match\n"
					"3   : WAVE_EX quad surround (L,C,R,S)       - infile must be quad\n"
					"4   : WAVE_EX 5.1 format surround           - infile must be 6-channel\n"
					"5   : WAVE_EX Ambisonic B-format (W,X,Y,Z...) - file extension .amb supported \n"
                    "6   : WAVE_EX 5.0 Surround                  - infile must be 5-channel\n"
                    "7   : WAVE_EX 7.1 Surround                  - infile must be 8-channel\n"
                    "8   : WAVE_EX Cube Surround                 - infile must be 8-channel\n"
                    "9   : WAVE_EX 6.1 Surround  (new in v2.1.0) - infile must be 7-channel\n"
					"default in all cases: outfile has format of infile\n"
					"NB: types 1 to 9 are for WAV format only\n"
                    "See also CHXFORMAT: destructively change WAVE_EX GUID and/or speaker mask.\n"
					);	
}



int
main(int argc, char *argv[])
{
	unsigned long size,i;
	unsigned long channels,srate;
	//RWD.6.5.99
	PSF_PROPS  inprops,outprops; 
	int force_wave = 0,rc = 0;
    psf_stype force_stype = 0;
	int opt_wave_type = -1;
	int min_header = 0;
	int interpret_floats = 0;
	int res;	
	char *create_msg = NULL;
    int do_dither = 0;
    psf_format  outformat = PSF_FMT_UNKNOWN;
	unsigned long update_size;
	PSF_CHPEAK  *peaks = NULL;
	MYLONG peaktime;
	char* ext = NULL;
	int have_amb_ext = 0;
    int ifd;		/* the input file */
    int ofd;		/* the output file */
    float *sampleframe = NULL;
	double outsize_bytes;
	double maxsize = pow(2.0,32.0) - 200.0;  /* some safety margin! */
	
    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("2.0.3\n");
        return 0;
    }

	if(psf_init()) {		
		printf("failed to init psfsys\n");
		exit(1);
	}

	if(argc < 3){
		usage();
		exit(1);
	}

	printf("\nCDP MCTOOLS: COPYSFX v2.0.3 (c) RWD,CDP 2011\n");

	while(argv[1][0]=='-'){
		int sflag = 0;
		switch(argv[1][1]){
        case 'd':
            do_dither = 1;
            break;
		case('t'):
			if(argv[1][2]=='\0'){
				fprintf(stderr,"-t flag requires parameter\n");
				usage();
				exit(1);
			}
			opt_wave_type = atoi(&(argv[1][2]));
			if((opt_wave_type < 0) || opt_wave_type > OPT_WAVEX_CUBE){
				fprintf(stderr,"wave type out of range\n");
				usage();
				exit(1);
			}
			force_wave = 1;
			break;		
		case('s'):
			if(argv[1][2]=='\0'){
				fprintf(stderr,"-s flag requires parameter\n");
				usage();
				exit(1);
			}
			if(force_stype >0){
				fprintf(stderr,"copysfx: error: -s flag may only be used once!\n");
				usage();
				exit(1);
			}
			sflag = atoi(&(argv[1][2]));
			if(sflag < 1 || sflag > 6){
				fprintf(stderr,"-s parameter out of range\n");
				usage();
				exit(1);
			}
			switch(sflag){
			case(1):
				force_stype = PSF_SAMP_16;
				break;
			case(2):
				force_stype = PSF_SAMP_32;
				break;
			case(3):
				force_stype = PSF_SAMP_IEEE_FLOAT;
				break;
			case(4):
				force_stype = PSF_SAMP_24;
				break;			
			default:
				fprintf(stderr,"internal error: unmatched sampletype parameter!\n");
				break;
			}
			break;
		case('h'):
            min_header = 1;    
			break;
#ifdef NOTDEF
		case('i'):
			interpret_floats = 1;
			break;
#endif
		default:
			fprintf(stderr,"unknown flag option %s\n",argv[1]);
			usage();
			exit(1);
			
		}
		argc--; argv++;
	}

	if(argc < 3){
		usage();
		exit(1);
	}

	if((ifd = psf_sndOpen(argv[1],&inprops,0)) < 0) {
		fprintf(stderr, "copysfx: can't open input SFfile %s:\n\t",
				argv[1]);
		exit(1);
	}
		
	size = psf_sndSize(ifd);
	channels = inprops.chans;
	sampleframe = (float *) malloc(channels * sizeof(float));
	if(sampleframe==NULL){
		psf_sndClose(ifd);
		puts("no memory for sample buffer\n");
		exit(1);
	}
	
	srate = inprops.srate;
	update_size = (unsigned long)((double)srate * 0.5);
	outprops = inprops;
	/* if -i flag used, infile is floats, therefore we write outfile as floats,
	 * UNLESS we also have outtype coercion!
	 */
	if(interpret_floats)
		outprops.samptype = PSF_SAMP_IEEE_FLOAT;

	if(inprops.format== PSF_WAVE_EX){		
		printf("opened WAVE-EX file:\n");
		switch(inprops.chformat){
		
		case(MC_STD):
			printf("\tno speaker assignments defined\n");		
			break;
		case(MC_MONO):
			printf("\tMONO\n");
			break;
		case(MC_STEREO):
			printf("\tSTEREO\n");
			break;		
		case(MC_QUAD):
			printf("\tGeneric quad format: LF-RF-LR-RR\n");			
			break;
		case(MC_LCRS):
			printf("\tQuad Surround: L-C-R-S format\n");			
			break;
		case(MC_BFMT):
			printf("\tAmbisonic B-Format\n");
			break;
		case(MC_DOLBY_5_1):
			printf("\t5.1 surround\n");
			break;
        case(MC_SURR_5_0):
			printf("\t5.0 surround\n");
			break;
		case(MC_SURR_6_1):
            printf("\t6.1 Surround\n");
            break;
        case(MC_SURR_7_1):
            printf("\t7.1 Surround\n");
            break;
        case(MC_CUBE):
            printf("\tCube Surround\n");
            break;
		default:
			printf("\tunrecognized speaker positions\n");
			break;						
		}
		if(min_header==1)
			printf("WARNING: minimum header is not recommended for WAVE_EX files\n");
	}

	peaks = (PSF_CHPEAK *) calloc(channels,sizeof(PSF_CHPEAK));
	if(peaks==NULL){
		puts("no memory for fpeak data buffer\n");
		psf_sndClose(ifd);
		exit(1);
	}

	//read infile peak data, if it exists, and report to user	
	res = psf_sndReadPeaks(ifd,peaks,  &peaktime);
	if(res ==0)	{
		printf("no peak data in this infile\n");
	}
	else if(res < 0){
		fprintf(stderr,"error reading infile peak data\n");
		psf_sndClose(ifd);
		exit(1);
	}
	else {
		time_t t_peaktime = (time_t) peaktime;
		printf("Infile PEAK data:\n\tcreation time: %s\n", ctime(&t_peaktime));
		for(i=0;i < channels; i++)
#ifdef CPLONG64
			printf("CH %ld: %.4lf at frame %u:\t%.4lf secs\n",
#else
            printf("(32) CH %ld: %.4lf at frame %lu:\t%.4lf secs\n",
#endif
				i,peaks[i].val,peaks[i].pos,(double) (peaks[i].pos) / (double)srate);
	}
	
	if(force_stype > PSF_SAMP_UNKNOWN)		
		outprops.samptype = force_stype;

	outsize_bytes = (double) size * outprops.chans * get_sampsize(outprops.samptype);
    if(outsize_bytes > maxsize){
        printf("output file size %.0f MB exceeds 4GB: cannot proceed.\n",outsize_bytes / 1048576.0);
        exit(1);
    }
	
	if(force_wave){
		int i,matched;
		//check file extension...
		switch(opt_wave_type){
		case(OPT_STD_WAVE):
			outprops.chformat = STDWAVE;
			outprops.format = PSF_STDWAVE;
			create_msg = "creating standard WAVE file\n";
			break;
		case(OPT_WAVEX_GENERIC):
			inprops.chformat = MC_STD;
			outprops.format = PSF_WAVE_EX;
			create_msg = "creating STD WAVE_EX file\n";
			break;
		case(OPT_WAVEX):
			switch(inprops.chans){
			case(1):
				outprops.chformat = MC_MONO;				
				outprops.format = PSF_WAVE_EX;
				create_msg = "creating MONO WAVE_EX file\n";
				break;
			case(2):
				outprops.chformat = MC_STEREO;				
				outprops.format = PSF_WAVE_EX;
				create_msg = "creating STEREO WAVE_EX file\n";
				break;
			case(4):
				outprops.chformat = MC_QUAD;			
				outprops.format = PSF_WAVE_EX;
				create_msg = "creating QUAD WAVE_EX file\n";
				break;
			default:
				fprintf(stderr,"infile nchans incompatible with requested WAVE-EX format\n");
				usage();
				psf_sndClose(ifd);
				if(peaks)
					free(peaks);
				exit(1);				
			}
			break;
		case(OPT_WAVEX_LCRS):
			if(inprops.chans != 4){
				fprintf(stderr,"infile must have four channels\n");
				usage();
				psf_sndClose(ifd);
				if(peaks)
					free(peaks);
				exit(1);
			}
			outprops.chformat = MC_LCRS;
			outprops.format = PSF_WAVE_EX;
			create_msg = "creating LCRS-surround WAVE_EX file\n";
			break;
		case(OPT_WAVEX_SURROUND):
			if(inprops.chans != 6){
				fprintf(stderr,"infile must have six channels\n");
				usage();
				psf_sndClose(ifd);
				if(peaks)
					free(peaks);
				exit(1);
			}
			outprops.chformat = MC_DOLBY_5_1;			
			outprops.format = PSF_WAVE_EX;
			create_msg = "creating 5.1 surround WAVE_EX file\n";
			break;			
		case(OPT_WAVEX_BFORMAT):
			matched = 0;
			for(i=0;i < N_BFORMATS;i++)	{
				if(inprops.chans == bformats[i]){
					matched = 1;
					break;
				}
			}
			if(!matched){
				printf("WARNING: No Bformat definition for %d-channel file.\n",inprops.chans);
			}
			outprops.chformat = MC_BFMT;			
			outprops.format = inprops.format = PSF_WAVE_EX;
			create_msg = "creating AMBISONIC B-FORMAT WAVE_EX file\n";
			break;
        case(OPT_WAVEX_5_0):
            if(inprops.chans != 5){
				fprintf(stderr,"infile must have five channels\n");
				usage();
				psf_sndClose(ifd);
				if(peaks)
					free(peaks);
				exit(1);
			}
			outprops.chformat = MC_SURR_5_0;			
			outprops.format = PSF_WAVE_EX;
			create_msg = "creating 5.0 surround WAVE_EX file\n";
			break;
				
        case(OPT_WAVEX_7_1):
            if(inprops.chans != 8){
				fprintf(stderr,"infile must have eight channels\n");
				usage();
				psf_sndClose(ifd);
				if(peaks)
					free(peaks);
				exit(1);
			}
			outprops.chformat = MC_SURR_7_1;			
			outprops.format = PSF_WAVE_EX;
			create_msg = "creating 7.1 surround WAVE_EX file\n";
			break;
        case OPT_WAVEX_CUBE:
            if(inprops.chans != 8){
				fprintf(stderr,"infile must have eight channels\n");
				usage();
				psf_sndClose(ifd);
				if(peaks)
					free(peaks);
				exit(1);
			}
			outprops.chformat = MC_CUBE;			
			outprops.format = PSF_WAVE_EX;
			create_msg = "creating 5.0 surround WAVE_EX file\n";
			break;			
        case(OPT_WAVEX_6_1):
            if(inprops.chans != 7){
				fprintf(stderr,"infile must have seven channels\n");
				usage();
				psf_sndClose(ifd);
				if(peaks)
					free(peaks);
				exit(1);
			}
			outprops.chformat = MC_SURR_6_1;			
			outprops.format = PSF_WAVE_EX;
			create_msg = "creating 6.1 surround WAVE_EX file\n";
			break;	    
		default:
			 printf("copysfx: Program error: impossible wave_ex type\n");
			 psf_sndClose(ifd);
				if(peaks)
					free(peaks);				
			 exit(1);
			 
		}
	}
	//ignore all that if user wants aiff!
	//informat = inprops.format;
	ext = strrchr(argv[2],'.');
	if(ext && stricmp(ext,".amb")==0)
		have_amb_ext = 1;

	if(have_amb_ext){
		if(!(outprops.format == PSF_WAVE_EX && outprops.chformat == MC_BFMT)){
			fprintf(stderr,"Error: .amb extension only allowed for WAVE_EX B-Format file.\n");
			exit(1);			
		} 
	}
    outformat  = psf_getFormatExt(argv[2]);

	if((ofd = psf_sndCreate(argv[2],&outprops,0,min_header,PSF_CREATE_RDWR)) < 0){	
		fprintf(stderr, "copysfx: can't create output file %s:\n\t",argv[2]);		
		psf_sndClose(ifd);
		fprintf(stderr,"\n");
		if(peaks)
			free(peaks);				
		exit(1);
	}
    if(do_dither){
        psf_sndSetDither(ofd,PSF_DITHER_TPDF);
    }
	if(force_wave){
		if(outprops.format==PSF_WAVE_EX){
			if(outformat > PSF_WAVE_EX)
                printf("WARNING: extended formats require .wav file format:\n\t - creating standard file\n");		
			else
			    printf("%s\n",create_msg);	
		}
	}	
	printf("copying...\n");    
    for(i=0;i < size;i++){
        /* salve to CEP users: need interpret_floats somewhere...? */
        if((psf_sndReadFloatFrames(ifd, sampleframe, 1)) != 1){
            
            fprintf(stderr,"copysfx: error reading from infile\n");
            psf_sndClose(ifd);
            psf_sndClose(ofd);
            free(sampleframe);
            if(peaks)
                free(peaks);
            exit(1);
        }
        if(psf_sndWriteFloatFrames(ofd, sampleframe,1)!=1){
            fprintf(stderr,"copysfx: error writing to outfile\n");
            psf_sndClose(ifd);
            psf_sndClose(ofd);
            free(sampleframe);
            if(peaks)
                free(peaks);
            exit(1);			
        }
        if((i / channels) % update_size == 0) {	 
            printf("%.2lf secs\r",(double) (i / channels) / (double) srate);
            fflush(stdout);
        }
    }
    printf("%.4lf secs\r",(double) (i / channels) / (double) srate);
    	
	if(psf_sndReadPeaks( ofd,peaks,&peaktime)){
		printf("Outfile PEAK values:\n");
		for(i=0; i < (unsigned long) inprops.chans; i++){
            double val, dbval;
            val = (double) peaks[i].val;
            if(val > 0.0){
                dbval = 20.0 * log10(val);
#ifdef CPLONG64
                printf("CH %ld: %.6f (%.2lfdB) at frame %u:\t%.4f secs\n",i,
#else
                printf("CH %ld: %.6f (%.2lfdB) at frame %lu:\t%.4f secs\n",i,
#endif
                       val,dbval,peaks[i].pos,(double)peaks[i].pos / (double) inprops.srate);
            }
            else{
#ifdef CPLONG64
                printf("CH %ld: %.6f (-infdB) at frame %u:\t%.4f secs\n",i,
#else        
                printf("CH %ld: %.6f (-infdB) at frame %lu:\t%.4f secs\n",i,
#endif
                       val,peaks[i].pos,(double)peaks[i].pos / (double) inprops.srate); 
            }
        }
	}
	if(psf_sndClose(ifd) < 0) {		
		rc++;
	}

	if(psf_sndClose(ofd) < 0) {		
		rc++;
	}
    if(sampleframe)
		free(sampleframe);
	if(peaks)
		free(peaks);	
	psf_finish();
	return rc;
}



