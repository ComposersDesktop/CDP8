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

// rmsinfo
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <portsf.h>
#include <math.h>
#include <signal.h>
#ifdef WIN32
# if _MSC_VER  && _MSC_VER <= 1200
# include <new.h>
# endif
#endif


#ifdef unix
/* in portsf.lib */
extern "C" {
    int stricmp(const char *a, const char *b);
}
#endif

enum {ARG_NAME, ARG_INFILE,ARG_NARGS};

enum {CH_RMS_POWER,CH_AMPSUM,CH_BISUM,CH_NORM_RMS,CH_NORM_AMPSUM};

typedef struct {
    double rmspower;
    double abssum;
    double bisum;
    double norm_rms;
    double norm_abssum;
} CH_RMSINFO;

#define BUFLEN (1024)

static int scanning(1);

void runhandler(int sig)
{
	if(sig == SIGINT){
		scanning = 0;
    }
}

void usage(){
    printf("\nCDP MCTOOLS: RMSINFO v1.0.1 (c) RWD, CDP 2009\n");
    printf("Scans infile and reports rms and average loudness (power) of infile,\n"
           " relative to digital peak (0dBFS)\n"
           );
    printf("Usage: rmsinfo [-n] infile [startpos [endpos]]\n"
           "Standard output shows:\n"
           "     RMS level\n"
           "     Average level\n"
           "     DC level (bipolar average)\n"
           "-n : Include equivalent 0dBFS-normalised RMS and AVG levels.\n"
           );
    printf("Optional arguments:\n"
           "startpos  :  start file scan from <startpos> seconds.\n"
           "endpos    :  finish file scan at <endpos> seconds.\n"
           );
    printf("To stop a scan early, use CTRL-C.\n"
           "Program will report levels up to that point.\n\n"
           );
}
#ifdef WIN32
#if _MSC_VER  && _MSC_VER <= 1200

 int newhandler(size_t size);
 class bad_alloc{};
 int newhandler(size_t size)
 {
     throw bad_alloc();
 }
#endif
#endif



int main(int argc, char**argv)  {
    long inframes,framesread=0,total_framesread;
    double maxsamp = 0.0,startpos = 0.0,endpos;
    long startframe = 0,endframe;
    long halfsecframes = 0;
    int i,j,ifd = -1;
    int chans;
    int do_norm = 0;
	PSF_PROPS inprops;
    CH_RMSINFO* rmsinfo = NULL;
    double* rmsfac  = 0;
    double* ampsum  = 0;
    double* ampsumb = 0;
    double* inbuf   = 0;
    double* nrmsfac = 0;
    double* nampsum = 0;
    
    
    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("1.0.1\n");
        return 0;
    }
#ifdef WIN32    
# if _MSC_VER  && _MSC_VER <= 1200
     _set_new_handler( newhandler );
# endif
#endif
    if(psf_init()){
		puts("unable to start portsf\n");
		return 1;
	}
    if(argc < 2){		 
		usage();
		return(1);
	}
    while(argv[1][0]=='-'){
		switch(argv[1][1]){
        case 'n':
            do_norm = 1;
            break;
        default:
            fprintf(stderr, "Unrecognised flag option %s\n",argv[1]);
            return 1;
        }
        argc--; argv++;
    }
    if(argc < 2){		 
		usage();
		return(1);
	}
    if((ifd = psf_sndOpen(argv[ARG_INFILE],&inprops, 0)) < 0){
		fprintf(stderr,"\nUnable to open input soundfile %s",argv[ARG_INFILE]);
		return(1);
	}
	inframes = psf_sndSize(ifd);  // m/c frames
    endframe = inframes;
    
	if(inframes <= 0)
        return 0;
	if(argc >= 3) {
        long lpos;
        startpos = atof(argv[ARG_INFILE+1]);
        if(startpos < 0.0){
            fprintf(stderr,"Error: startpos must be positive\n");
            return 1;
        }
        lpos = (long)( startpos * inprops.srate);
        if(lpos > inframes){
            fprintf(stderr,"Error: startpos value beyond end of file.\n");
            return 1;
        }
        startframe = lpos;
    }
    if(argc >= 4) {
        long lpos;
        endpos = atof(argv[ARG_INFILE+2]);
        
        lpos = (long)(endpos * inprops.srate);
        if(lpos > inframes){
            fprintf(stderr,"Warning: endpos value too large - reset to end of file.\n");
            return 1;
        }
        endframe = lpos;
        if(!(endframe > startframe)){
            fprintf(stderr,"Error: endpos must be beyond startpos.\n");
            return 1;
        }
    }
    if(startframe)
        printf("Starting at frame %ld, ending at frame %ld\n",startframe, endframe);
    
    chans   = inprops.chans;
    try {
        inbuf   = new double[BUFLEN * chans];
        rmsinfo = new CH_RMSINFO[chans];
        rmsfac  = new double[chans];
        ampsum  = new double[chans];
        ampsumb = new double[chans];
        nrmsfac = new double[chans];
        nampsum = new double[chans];
    }
    catch(...){
        fputs("no memory!\n",stderr);
        return 1;
    }
    for(i=0; i < chans; i++){
        rmsfac[i]  = 0.0;
        ampsum[i]  = 0.0;
        ampsumb[i] = 0.0;
        nrmsfac[i] = 0.0;
        nampsum[i] = 0.0;
    }
    halfsecframes = inprops.srate / 2; 
    signal(SIGINT,runhandler);
    long wanted = endframe - startframe;
    printf("Scanning %ld frames (%.3lf secs):\n",wanted, (double)wanted / inprops.srate);
    total_framesread = 0;
    if(startframe) {
        if(psf_sndSeek(ifd,startframe,PSF_SEEK_SET)){
            fprintf(stderr,"File Seek error.\n");
            return 1;
        }
    }
        
	while((framesread = psf_sndReadDoubleFrames(ifd,inbuf,BUFLEN)) > 0){
        double fval;
        for(i = 0;i < framesread;i++) {
            for(j = 0; j < chans; j++){
                double val = inbuf[i*chans + j];
                fval = fabs(val);
                maxsamp = fval >  maxsamp ? fval : maxsamp;
                ampsum[j]   += fval;
                rmsfac[j]   += val*val; 
                ampsumb[j]  += val;
            }
            total_framesread++;
            if(scanning==0)
                break;
            if(total_framesread == wanted) 
                break;
            if((total_framesread % halfsecframes) == 0){
                printf("%.2lf\r",total_framesread / (double) inprops.srate);
                fflush(stdout);
            }
        }
        if(total_framesread == wanted) {
            break;
        }
    }
    if(framesread < 0){
        fprintf(stderr,"Error reading file.\n");
        return 1;
    }
    for(i=0;i < chans;i++){
        rmsfac[i]  /= total_framesread;
        rmsfac[i]   = sqrt(rmsfac[i]);
        ampsum[i]  /= total_framesread;
        ampsumb[i] /= total_framesread;
    }

    double normfac = 1.0 / maxsamp;
    if(scanning==0)
        printf("\nScan stopped.\n");
    if(total_framesread < inframes){
        printf("Scanned %ld frames (%.2lf secs).\n",total_framesread,total_framesread / (double)inprops.srate);
    }
    
    printf("Maximum sample = %lf (%.2lfdB)\n",maxsamp,20.0 * log10(maxsamp));
    printf("Maximum normalisation factor = %.4f\n",normfac);
    
    for(i=0;i < chans;i++){
        rmsinfo[i].rmspower    = rmsfac[i];
        rmsinfo[i].abssum      = ampsum[i];
        rmsinfo[i].bisum       = ampsumb[i];
        rmsinfo[i].norm_rms    = normfac * rmsfac[i];
        rmsinfo[i].norm_abssum = normfac * ampsum[i];
    }
    if(do_norm){
        printf("\t   RMS LEVEL\t    AVG  \t   NET DC\t   NORM RMS\t  NORM AVG\n");
        printf("CH\t AMP\t DB\t AMP\t DB\t AMP\t DB\t AMP\t DB\t AMP\t DB     \n");
    }
    else{
        printf("\t   RMS LEVEL\t    AVG  \t   NET DC\n");
        printf("CH\t AMP\t DB\t AMP\t DB\t AMP\t DB\n"); 
    }
    for(i=0;i < chans;i++){
        double d1,d2,d3,d4,d5;
        
        d1 = 20*log10(rmsfac[i]);
        d2 = 20*log10(ampsum[i]);
        d3 = 20*log10(fabs(ampsumb[i]));
        d4 = 20*log10(normfac * rmsfac[i]);
        d5 = 20*log10(normfac * ampsum[i]);
        if(do_norm){
            printf("%d\t%.5lf\t%.2lf\t%.5lf\t%.2lf\t%+.4lf\t%.2lf\t%.5lf\t%.2lf\t%.5lf\t%.2lf\n",i+1,
               rmsinfo[i].rmspower,d1,
               rmsinfo[i].abssum,d2,
               rmsinfo[i].bisum,d3,
               rmsinfo[i].norm_rms,d4,
               rmsinfo[i].norm_abssum,d5
            );
        }
        else {
            printf("%d\t%.5lf\t%.2lf\t%.5lf\t%.2lf\t%+.4lf\t%.2lf\n",i+1,
                   rmsinfo[i].rmspower,d1,
                   rmsinfo[i].abssum,d2,
                   rmsinfo[i].bisum,d3
            );
        }
    }
        
    delete [] inbuf;
    delete [] rmsfac;
    delete [] ampsum;
    delete [] ampsumb;
    delete [] rmsinfo;
    psf_sndClose(ifd);
    psf_finish();

	return 0;
}
