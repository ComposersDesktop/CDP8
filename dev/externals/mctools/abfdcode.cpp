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

//abfdcode.cpp  : write mono wave file into ambisonic B-format
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "portsf.h"

#ifdef unix
/* in portsf.lib */
extern  "C" {
    int stricmp(const char *a, const char *b);
}
#endif

typedef struct abf_samp {
	float W;
	float X;
	float Y;
	float Z;	
} ABFSAMPLE;

int main(int argc,char *argv[])
{
	int i,ifd, ofd;
	long outchans = 4;
	long outsize;
	char *sfname;
	bool write_wavex = false;
	ABFSAMPLE abfsample;
	float frame[4];
	PSF_PROPS props;
	PSF_CHPEAK peaks[4];
	long inchans;
    MYLONG peaktime;

    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("1.2.1\n");
        return 0;
    }
    
	if(argc < 3){
		printf("\nCDP MCTOOLS: ABFDCODE v 1.2.1: CDP 1999,2004,2005\n"
				"Horizontal B Format Decoder\n"
				"usage: abfdcode [-x] infile outfile\n"
				"       -x    :   write WAVE_EX (Quad) outfile format\n"
				"                (requires .wav extension)\n"
				"NB: infile must have 3 or 4 channels\n"
                "UPDATE 2009: This program is now replaced by FMDCODE, \n"
                "and may be omitted in future updates to the Toolkit.\n"
                "For this task use fmdcode with e.g. layout 4.\n"
				);
		return 1;
	}
	while(argv[1][0] =='-'){		
		switch(argv[1][1]){
		case('x'):
			write_wavex = true;
			break;
		default:
			fprintf(stderr,"abfdecode: error: illegal flag option %s\n",argv[1]);
			return 1;
		}
		argc--; argv++;
	}
	if(argc < 3){
		fprintf(stderr,"CDP MCTOOLS: ABFDCODE.EXE: CDP 1999,2005\nHorizontal B Format Decoder\nusage: abfdcode [-x] infile outfile\n");
		return 1;
	}
	if(psf_init()){
		fprintf(stderr,"abfdcode: startup failure.\n");
		return 1;
	}
	
	sfname = argv[2];
	
	ifd = psf_sndOpen(argv[1],&props,0);
	if(ifd < 0){
		fprintf(stderr,"unable toopen infile %s\n",argv[1]);
		return 1;
	}

	inchans = props.chans;
	if(!(inchans == 3 || inchans == 4)){
		fprintf(stderr,"Sorry: infile is not first-order\n");
		psf_sndClose(ifd);
		return 1;
	}

	outsize = psf_sndSize(ifd);
	if(outsize <= 0){
		fprintf(stderr,"infile is empty!\n");
		psf_sndClose(ifd);
		return 1;
	}
	
	props.chformat = STDWAVE;
	props.chans = 4;
    if(!is_legalsize(outsize,&props)){
        fprintf(stderr,"error: outfile size exceeds capacity of format.\n");
        return 1;
    }
    
    
	if(write_wavex){
		props.chformat = MC_QUAD;
		props.format = PSF_WAVE_EX;
	}
	//ofd = sndcreat_formatted(sfname,outsize * outchans,stype,outchans,srate,CDP_CREATE_NORMAL);
	ofd = psf_sndCreate(sfname,&props,0,0,PSF_CREATE_RDWR);
	if(ofd < 0){
		fprintf(stderr,"can't create outfile %s.\n",sfname);
		psf_sndClose(ifd);
		return 1;
	}
	

	int got, halfsec = props.srate / 2;
	unsigned int framepos = 0;
	printf("\ndoing b-format decoding:\n");
	while((got = psf_sndReadFloatFrames(ifd,(float *) &abfsample,1))==1){
		//this_samp	= 0.5 * p_iosc->tick(1000);
		float aw = abfsample.W;
		float ax = abfsample.X * 0.707f;
		float ay = abfsample.Y * 0.707f;
		frame[0] = 0.3333f * (aw + ax + ay);
		frame[1] = 0.3333f * (aw + ax - ay);
		frame[2] = 0.3333f * (aw - ax + ay);
		frame[3] = 0.3333f * (aw - ax - ay);

		if(0 > psf_sndWriteFloatFrames(ofd,frame,1)){
			fprintf(stderr,"error writing sample block  %ld\n",got * outchans);
			psf_sndClose(ifd);
			psf_sndClose(ofd);
			return 1;
		}
		if(framepos % halfsec==0) {
			printf("%.2lf secs\r",(double)framepos / (double) props.srate);
            fflush(stdout);
        }
		framepos++;
	}

	if(got != 0){
		fprintf(stderr,"warning: not all data was read\n");
	}
	printf("\n%.4lf secs\nWritten %d quad frames to %s\n",(double)framepos / (double) props.srate,framepos,sfname);
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
                    val,(unsigned int) peaks[i].pos,(double)peaks[i].pos / (double) props.srate); 
            }
        }
              
    }
    psf_sndClose(ifd);
	psf_sndClose(ofd);
	psf_finish();
	return 0;
}
