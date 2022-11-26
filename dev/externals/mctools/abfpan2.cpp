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

//abfpan2.cpp  : write mono wave file into ambisonic B-format
// Dec 2005 support .amb extension, and 3ch output
// OCT 2009 portsf version, with 2nd-order encoding
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include "portsf.h"

#ifndef M_PI
#define M_PI (3.141592654)
#endif
#ifndef TWOPI
#define TWOPI	(6.283185307)
#endif

#ifdef unix
/* in portsf.lib */
extern "C" {
    int stricmp(const char *a, const char *b);
}
#endif

enum {W,X,Y,Z,R,S,T,U,V};


enum {ARG_PROGNAME, ARG_INFILE, ARG_OUTFILE,ARG_STARTPOS,ARG_ENDPOS,ARG_NARGS};

void usage()
{
	fprintf(stderr,"\nCDP MCTOOLS V1.0.1 beta (c) RWD,CDP 2010"
				   "\n\nABFPAN2:  apply fixed or orbiting 2nd-order B-Format pan to infile\n");
	fprintf(stderr,"usage : abfpan2 [-gGAIN][-w] [-p[DEG]] infile outfile startpos endpos\n"
                   "    infile : mono source.\n"
                   "    outfile: 2nd order B-format output.\n"        
	               "        0.0 <= startpos <= 1.0 (0.0 and 1.0 = Centre Front).\n");
	fprintf(stderr,"    endpos : endpos < 0.0 gives anticlockwise rotation,\n"
				   "            endpos > 0.0 gives clockwise rotation.\n"
				   "            Units give number of revolutions, fraction gives final position\n"
				   "            Set endpos = startpos for fixed pan\n"
                   "    -gGAIN : scale infile amplitude by GAIN (GAIN > 0).\n"
                   "    -w     : Write standard soundfile (wave, aiff)\n"
                   "            Default: WAVEX B-Format; use .amb extension.\n"
                   "    -p[DEG]: write full 9-channel (periphonic) B-Format file.\n"
                   "        Default: write 5-channel (2nd-order horizontal) file.\n"
                   "        DEG:  optional fixed height argument (degrees).\n"
                   "             Range = -180  to +180,\n"
                   "             where  -90 = nadir, +90 = zenith (directly above).\n"
                   "             Default: DEG=0;  height channels (Z,R,S,T)  will be empty.\n"
                   "    NB: this program does not create a decoded output.\n"
                   "    Use FMDCODE to decode to choice of speaker layouts.\n"
				   );
    fprintf(stderr,"\n");
}

int main(int argc,char *argv[])
{
	int i,got,ifd, ofd;
	long srate = 44100;
	long outchans = 5;
    long do_peri = 0;
    int write_wav = 1;
	MYLONG peaktime;
	long outsize;	
	float this_samp;
	int half_sec;
	long total_frames;
	double d_srate;
	char *sfname;
	double angle_incr;
	double start_angle = 0.0;
	double startpos,endpos;
    double gain = 1.0;
    double degree=0.0,elevation = 0.0;  
	
	float abfsample[9];
    float outframe[5];
    float *p_frame;
    
	PSF_PROPS props;
	PSF_CHPEAK *peaks = NULL;
	
    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("1.0.1b\n");
        return 0;
    }
    
	if(argc < 5){
		usage();
		return 1;
	}

	if(psf_init()){
		fprintf(stderr,"\nabfpan2: Startup failure");
		return 1;
	}
	
	while(argv[1][0] =='-'){
		switch(argv[1][1]){
        case 'g':
            if(argv[1][2] == '\0'){
                fprintf(stderr,"abfpan2 Error: -g flag requires a value.\n");
                return 1;
            }
            gain = atof(&argv[1][2]);
            if(gain <= 0.0){
                printf("abfpan2: gain value must be positive!\n");
                return 1;
            }
            break;
        case 'p':
            if(argv[1][2] != '\0'){
                degree = atof(&argv[1][2]);
                if(degree < -180.0 || degree > 180.0){
                    fprintf(stderr,"-p: degree value out of range.\n");
                    return 1;
                }
                elevation = degree * (M_PI  / 180.0);
            }
            outchans = 9;
            do_peri = 1;
            break;
        case 'w':
            write_wav = 0;
            break;
		default:
			fprintf(stderr,"\nabfpan: error: illegal flag option %s",argv[1]);
			return 1;
		}
		argc--; argv++;
	}

	if(argc < ARG_NARGS ){
		usage();
		return 1;
	}

	sfname = argv[ARG_INFILE];
	startpos = atof(argv[ARG_STARTPOS]);
	if(startpos < 0.0 || startpos > 1.0){
		fprintf(stderr,"abfpan2: startpos %.4lf out of range: must be between 0.0 and 1.0\n",startpos);
		return 1;
	}

	endpos = atof(argv[ARG_ENDPOS]);
	ifd = psf_sndOpen(sfname,&props,0);
	if(ifd < 0){
		fprintf(stderr,"abfpan2: unable toopen infile %s.\n",sfname);
		return 1;
	}

    if(props.chans != 1){
		fprintf(stderr,"abfpan2: infile must be mono.\n");
		psf_sndClose(ifd);
		return 1;
	}

	outsize = psf_sndSize(ifd);
	if(outsize <= 0){
		fprintf(stderr,"abfpan2: infile is empty!\n");
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
    
	
    if(write_wav == 0)
        props.format = PSF_STDWAVE;
    else {
        printf("Writing B-Format file.\n");
	    props.format = PSF_WAVE_EX;
        props.chformat = MC_BFMT;
    }
    
    peaks = (PSF_CHPEAK*)  malloc(sizeof(PSF_CHPEAK) * outchans);
    memset(peaks,0,sizeof(PSF_CHPEAK) * outchans);
    
    
	ofd = psf_sndCreate(argv[ARG_OUTFILE],&props,0,0,PSF_CREATE_RDWR);
	if(ofd < 0){
		fprintf(stderr,"abfpan2: can't create outfile %s.\n",sfname);	
		return 1;
	}

	half_sec = srate / 2;
	total_frames = 0;
	d_srate = (double)srate;
	
	//should make one 360deg rotate over duration
	//TODO: make lookup_sin and lookup_cos ugens, taking angle arg
	
    if(do_peri)
        p_frame = abfsample;
    else
        p_frame = outframe;
    
	while((got = psf_sndReadFloatFrames(ifd,&this_samp,1))==1){
		double x,y,z,xx,yy,zz;
        x = cos(start_angle);
        y = sin(start_angle);
        if(elevation ==0.0)
            z = 0.0;
        else
            z = sin(elevation);
        xx = x * x;
        yy = y * y;
        zz = z * z;
        this_samp *= gain;
        if(do_peri) {
            abfsample[W] = (float) (this_samp * 0.7071);
            abfsample[X] = (float) (this_samp * x);
            abfsample[Y] = (float) (this_samp * y);
            abfsample[Z] = (float) (this_samp * z);
            abfsample[R] = (float) (this_samp * (1.5 * zz - 0.5));  /// ????
            abfsample[S] = (float) (this_samp * (2.0 * z * x));
            abfsample[T] = (float) (this_samp * (2.0 * y * z));
            abfsample[U] = (float) (this_samp * (xx-yy));
            abfsample[V] = (float) (this_samp * (2.0 * x * y));
        }
        else{
            outframe[0]  = (float) (this_samp * 0.7071);
            outframe[1]  = (float) (this_samp * x);
            outframe[2]  = (float) (this_samp * y);
            outframe[3] = (float) (this_samp * (xx-yy));
            outframe[4] = (float) (this_samp * (2.0 * x * y));
        }
            
		if(0 > psf_sndWriteFloatFrames(ofd, p_frame,1)){
			fprintf(stderr,"abfpan2: error writing frame %ld\n",total_frames);
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
		fprintf(stderr,"abfpan2: warning: not all data was read.\n");
	}
	printf("%.4lf secs\nWritten %ld frames to %s.\n",(double)total_frames / d_srate,total_frames, argv[ARG_OUTFILE]);
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
                       val,(unsigned int)peaks[i].pos,(double)peaks[i].pos / (double) props.srate); 
            }
        }
        
	}

	psf_sndClose(ifd);
	psf_sndClose(ofd);
	psf_finish();
	return 0;
}
