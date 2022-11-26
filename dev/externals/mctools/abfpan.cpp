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

//abfpan.cpp  : write mono wave file into ambisonic B-format
// Dec 2005 support .amb extension, and 3ch output
// Jan 2010: corrected usage message about -b and -x
// Nov 2013: updated usage message formatting, infile must be mono
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "portsf.h"

#ifdef unix
/* in portsf.lib */
extern "C" {
    int stricmp(const char *a, const char *b);
}
#endif

#ifndef TWOPI
#define TWOPI	(6.283185307)
#endif
typedef struct abf_samp {
	float W;
	float X;
	float Y;
	float Z;	
} ABFSAMPLE;


enum {ARG_PROGNAME, ARG_INFILE, ARG_OUTFILE,ARG_STARTPOS, ARG_ENDPOS,ARG_NARGS};
void usage()
{
	fprintf(stderr,"CDP MCTOOLS V1.5.3 (c) RWD,CDP 2009,2010,2013\n"
				   "ABFPAN:  apply fixed or orbiting 1st-order B-Format pan to infile\n");
	fprintf(stderr,"usage : abfpan [-b][-x][-oN] infile outfile startpos endpos\n");
	fprintf(stderr,"    infile:   input soundfile, must be mono\n");
	fprintf(stderr,"    startpos: 0.0 <= startpos <= 1.0 (0.0 and 1.0 = Centre Front)\n");
	fprintf(stderr,"    endpos:   endpos < 0.0 gives anticlockwise rotation,\n"
				   "                endpos > 0.0 gives clockwise rotation.\n"
				   "                Units give number of revolutions, fraction gives final position\n"
				   "                Set endpos = startpos for fixed pan\n");
	fprintf(stderr,"    -b:       write output as horizontal Ambisonic B-format (.amb),\n"
                   "                (i.e. bypass B-Format decoding).\n"
			       "    -x:       write (WAVE_EX) format file (.wav). \n"			   
				   "                Default: write standard m/c soundfile (WAVE,AIFF).\n"
                   "                NB: -b overrides -x - no meaning in using both.\n"
				   "    -oN:      number of B-Format output channels: N = 3 or 4 only.\n"
				   "                Default: write 4-ch file.\n"
                   "    See also ABFPAN2 for 2nd order version with fixed height.\n"
	);
    fprintf(stderr,"\n");
}



#define SYN_BUFLEN	(32768)
#define TABLEN (1024)
int main(int argc,char *argv[])
{
	int ifd, ofd;
	int srate = 44100;
	int outchans = 4;
    MYLONG peaktime;
	long outsize;	
	float this_samp;
	char *sfname;
	double angle_incr;
	double start_angle = 0.0;
	double startpos,endpos;
	
	ABFSAMPLE abfsample;
	PSF_PROPS props;
	PSF_CHPEAK peaks[4];

	bool write_bformat = false;
	bool write_wavex = false;
	int iarg = outchans;

    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("1.5.2.\n");
        return 0;
    }
    
	if(argc < 5){
		usage();
		return 1;
	}

	if(psf_init()){
		fprintf(stderr,"abfpan2: Startup failure.\n");
		return 1;
	}
	
	while(argv[1][0] =='-'){
		
		switch(argv[1][1]){
		case('b'):
			write_bformat = true;
			break;

		case('x'):
			write_wavex = true;
			break;
		case('o'):
			if(argv[1][2]=='\0'){
				fprintf(stderr,"abfpan: error: -o flag requires channel value\n");
				return 1;
			}
			iarg = atoi(&(argv[1][2]));
			if(!(iarg == 3 || iarg == 4)){
				fprintf(stderr,"abfpan: error: invalid channel value for -o flag\n");
				return 1;
			}

			break;
		default:
			fprintf(stderr,"abfpan: error: illegal flag option %s.\n",argv[1]);
			return 1;
		}

		argc--; argv++;
	}

	if(argc != 5){
		usage();
		return 1;
	}

	if(write_bformat)
		outchans = iarg;
    
	sfname = argv[ARG_INFILE];
	startpos = atof(argv[ARG_STARTPOS]);
	if(startpos < 0.0 || startpos > 1.0){
		fprintf(stderr,"abfpan: startpos %.4lf out of range: must be between 0.0 and 1.0.\n",startpos);
		return 1;
	}

	endpos = atof(argv[ARG_ENDPOS]);



	ifd = psf_sndOpen(sfname,&props,0);
	if(ifd < 0){
		fprintf(stderr,"abfpan: unable to open infile %s.\n",argv[1]);
		return 1;
	}
	if(props.chans != 1){
		fprintf(stderr,"abfpan: infile must be mono.\n");
		psf_sndClose(ifd);
		return 1;
	}

	outsize = psf_sndSize(ifd);
	if(outsize <= 0){
		fprintf(stderr,"abfpan: infile is empty!\n");
		psf_sndClose(ifd);
		return 1;
	}
    srate = props.srate;
	props.chans = outchans;
    if(!is_legalsize(outsize,&props)){
        fprintf(stderr,"error: outfile size exceeds capacity of format.\n");
        return 1;
    }
                
	start_angle = - (TWOPI * startpos);	   //we think of positive as clockwise at cmdline!
	angle_incr = TWOPI / outsize;
	angle_incr *= (endpos - startpos);
	
	if(write_wavex){
		props.format = PSF_WAVE_EX;
		props.chformat = MC_QUAD;
	}
	if(write_bformat)
		props.chformat = MC_BFMT;
	ofd = psf_sndCreate(argv[ARG_OUTFILE],&props,0,0,PSF_CREATE_RDWR);
	if(ofd < 0){
		fprintf(stderr,"abfpan: can't create outfile %s.\n",argv[ARG_OUTFILE]);	
		return 1;
	}

	int i,got;
	int half_sec = srate / 4;
	long total_frames = 0;
	double d_srate = (double)srate;

	abfsample.Z = 0.0f;
	for(i=0; i < outchans; i++){
		peaks[i].val = 0.0f;
		peaks[i].pos = 0;
	}
	//should make one 360deg rotate over duration
	//TODO: make lookup_sin and lookup_cos ugens, taking angle arg
	if(write_bformat)
		printf("\nWriting B-Format file:\n");
	else
		printf("\n");
	while((got = psf_sndReadFloatFrames(ifd,&this_samp,1))==1){		
		abfsample.W = (float) (this_samp * 0.707);
        abfsample.X = (float) (this_samp * cos(start_angle));
		abfsample.Y	= (float) (this_samp * sin(start_angle));

		//set this for periphonic; otherwise = 0.0 for horizontal_only
		//abfsample.Z = (float) (this_samp*0.25);
		if(!write_bformat){
			//decode into quad file
			float aw = abfsample.W;
			float ax = abfsample.X * 0.707f;
			float ay = abfsample.Y * 0.707f;
            /* ignore wxyz names  - this is now handy container for quad speaker feeds*/
			abfsample.W = 0.33333f * (aw + ax + ay); /* front L */
			abfsample.X = 0.33333f * (aw + ax - ay); /* front R */
			abfsample.Y = 0.33333f * (aw - ax + ay); /* rear  L */
			abfsample.Z = 0.33333f * (aw - ax - ay); /* rear  R */
		}
		if(0 > psf_sndWriteFloatFrames(ofd,(float*)  &abfsample,1)){
			fprintf(stderr,"error writing abf sample frame  %ld\n",total_frames);
			return 1;
		}
		start_angle -= angle_incr;
		total_frames++;		
		if(total_frames % half_sec ==0) {
			printf("%.2lf secs\r",(double)total_frames / d_srate);
            fflush(stdout);
        }
	}

	if(got != 0){
		fprintf(stderr,"abfpan: warning: not all data was read.\n");
	}

	printf("%.4lf secs\nwritten %ld %d-ch sample frames to %s\n",(double)outsize / d_srate,outsize,outchans, argv[ARG_OUTFILE]);
	if(psf_sndReadPeaks( ofd,peaks,&peaktime)){
		printf("PEAK values:\n");
		for(i=0; i < outchans; i++){
            double val, dbval;
            val = (double) peaks[i].val;
            if(val > 0.0){
                dbval = 20.0 * log10(val);
                printf("CH %d: %.6f (%.2lfdB) at frame %u:\t%.4f secs\n",i,
                       val,dbval,(unsigned int) peaks[i].pos,(double)peaks[i].pos / (double) props.srate);
            }
            else{
                printf("CH %d: %.6f (-infdB) at frame %u:\t%.4f secs\n",i,
                        val, (unsigned int) peaks[i].pos,(double)peaks[i].pos / (double) props.srate); 
            }
        }
        
	}
	printf("\n");
	psf_sndClose(ifd);
	psf_sndClose(ofd);
	psf_finish();
	return 0;
}
