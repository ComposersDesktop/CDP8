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

/* fastconv.cpp: */
/*  self-contained prog for fast convolution (reverb, FIR filtering, bformat etc) 
 *  
*/

/* um, there is nothing C++ish about this code apart from use of new, delete! */

/*TODO: control wet/dry mix with something... */
/* auto-rescale arbitrary impulse responses? */
/* Feb 2013: rebuilt with updated portsf */
/* August 2013 epanded usage message re text file. */
extern "C"
{

#include <portsf.h>

}
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>


#ifdef _DEBUG
#include <assert.h>
#endif

#ifdef unix
#include <ctype.h>
int stricmp(const char *a, const char *b);
int strnicmp(const char *a, const char *b, const int length);
#endif

#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT (0x0003)
#endif

/* convert from mono text impulse file */
int genimpframe1(double *insbuf, double*** outbuf, int npoints, double scalefac);
/* convert from multichan soundfile */
int genimpframe2(int ifd,double*** outframe, double* rms,int imchans,double scalefac);
void mc_split(double* inbuf,double** out,int insize,int chans);
void mc_interl(double** in,double* out,int insize,int chans);
void complexmult(double *frame,const double *impulse,int length);


extern "C"{
void fft_(double *, double *,int,int,int,int);
void fftmx(double *,double *,int,int,int,int,int,int *,double *,double *,double *,double *,int *,int[]);
void reals_(double *,double *,int,int);
}


#define DEFAULT_BUFLEN (32768)

enum {ARG_NAME,ARG_INFILE,ARG_IMPFILE,ARG_OUTFILE,ARG_NARGS};

void usage(const char *progname)
{

    printf("\n\n%s v1.2 RWD,CDP July 2010,2013",progname);
	printf("\nMulti-channel FFT-based convolver");

	printf("\nUsage: \n    %s [-aX][-f] infile impulsefile outfile [dry]\n"		               
           "   -aX        : scale output amplitude by X\n"
           "   -f         : write output as floats (no clipping)\n"
           "  infile      : input soundfile to be processed.\n"
           "  impulsefile : m/c soundfile or mono text file containing impulse response,\n"
           "                  e.g. reverb or FIR filter.\n"
           "                Text file name must have extension .txt.\n"
		   "                  File must contain 1 column of floating point values,\n"
		   "                  typically in the range -1.0 to 1.0.\n"
           "              Supported channel combinations:\n"
           "               (a) mono infile, N-channel impulsefile;\n"
           "               (b) channels are the same;\n"
           "               (c) mono impulsefile, N-channel infile.\n"
           "  [dry]       :  set dry/wet mix (e.g. for reverb)\n"
           "                 Range: 0.0 - 1.0,  default = 0.0\n"
           "                 (uses sin/cos law for constant power mix)\n"
           "Note: some recorded reverb impulses effectively include the direct signal.\n"
           "In such cases  <dry>  need not be used\n"
           "Where impulsefile is filter response (FIR), optimum length is power-of-two - 1.\n",progname
		   );		   
}


double
timer()
{
	struct timeb now;
	double secs, ticks;	
	ftime(&now);
	ticks = (double)now.millitm/(1000.0);
	secs = (double) now.time;

	return secs + ticks;
}


void			
stopwatch(int flag) 
{
	static double start;		    
	int mins=0,hours=0;
	double end, secs=0;

	if(flag)
		start = timer();	
	else 
	{
		end    = timer(); 
		secs   = end - start;
		mins   = (int)(secs/60.0);
		secs  -= mins*60.0; 
		hours  = mins/60;
		mins  -= hours*60;

		fprintf(stderr,"\nElapsed time: ");
		if(hours > 0)
			fprintf(stderr,"%d h ", hours);
		if(mins > 0)
			fprintf(stderr,"%d m ", mins);
		fprintf(stderr,"%2.3lf s\n\n",secs);
	}
}

/* how do we want this to work?

  (1)  multi-chan impulse:  EITHER: mono infile  OR infile with same chan count
  (2)  mono impulse:  expanded to infile chancount
     Therefore: need to open both infiles before deciding action
*/
#define PI2 (1.570796327)

int main(int argc,char **argv)
{

	int fftlen = 1,imlen = 0,chans = 0,srate;
	double scalefac = 1.0f;
    double Ninv = 1.0;
	int insndfile = -1,inimpfile = -1,outsndfile = -1; 
    int nameoffset = 0;
	PSF_PROPS inprops,outprops, impulseprops;
    psf_format format;
    PSF_CHPEAK  *fpeaks = NULL;
	MYLONG peaktime;
	int i,jj,minheader = 0,do_timer = 1;
    int writefloats = 0;
    int framesneeded;
	double oneovrsr;
    double *inmonobuf = 0;
	double *insbuf=0,*outsbuf = 0;
    double **inbuf = 0, **outbuf = 0;    
	double **imbuf = 0;
	double **overlapbuf = 0;
    double rms = 0.0;
    double dry = 0.0;
    double wet = 1.0;
#ifdef FFTTEST
    double *anal = 0;
#endif
    	
    if(argv[0][0]=='.' && argv[0][1]=='/'){
        nameoffset  = 2;
    }
	
    if(argc==2){
        if(strcmp(argv[1],"--version")==0){
            printf("1.2.0.\n");
            return 0;
        }
    }

	if(psf_init()){
		puts("unable to start portsf\n");
		return 1;
	}


	if(argc < ARG_NARGS){		 
		usage(argv[0]+nameoffset);
		return(1);
	}

    

    while(argc > 1 && argv[1][0]=='-'){				
		switch(argv[1][1]){
        case 'a':
            scalefac =  atof(&(argv[1][2]));
            if(scalefac <=0.0){
                printf("Error: scalefac must be positive!\n");
                return 1;
            }
            break;        
        case 'f':
            writefloats = 1;
            break;
        default:
            break;
        }
        argv++;
        argc--;
    }		
	/* 2 legal possibilities: infile and outfile, or -I used with infile only */
	if(argc< ARG_NARGS){
		fprintf(stderr,"\nInsufficient arguments.");
		usage(argv[0]+nameoffset);
		return(1);
	}
    if(argc==ARG_NARGS+1){
        double dryarg;
        dryarg = atof(argv[ARG_NARGS]);
        if(dryarg < 0.0 || dryarg > 1.0){
            printf("dry value out of range.\n");
            return 0;
        }
        if(dryarg==1.0){
            printf("Warning: dry=1 copies input!\n");
            wet = 0.0;
        }
        else{
            dry = sin(PI2 * dryarg);
            wet = cos(PI2 * dryarg);
        }
    }

		

/* TODO:  where -F[file] is combined with -A, it signifies create analysis file
           compatible with impulse file (e.g 50% overlap, etc) */

	
	/* open infile, check props */	
		
	if((insndfile = psf_sndOpen(argv[ARG_INFILE],&inprops, 0)) < 0){
		fprintf(stderr,"\nUnable to open input soundfile %s",argv[1]);
        delete []imbuf;
		return 1;
	}
    srate = inprops.srate;
	if(srate <=0){
		fprintf(stderr,"\nBad srate found: corrupted file?\n");
        delete []imbuf;
		return 1;
	}
	chans = inprops.chans;
    framesneeded = psf_sndSize(insndfile);
    if(framesneeded <= 0){
        printf("Error in input file - no data!\n");
        psf_sndClose(insndfile);
        return 1;
    }
    /* open impulse file */
    /* check for soundfile */
      
    format = psf_getFormatExt(argv[ARG_IMPFILE]);
    if(format==PSF_FMT_UNKNOWN){  /* must be a text file */
        FILE *fp = 0;
        char tmp[80];
        char* dot;
        int size = 0,got = 0;
        int read = 0;

        dot = strrchr(argv[ARG_IMPFILE],'.');
        if(dot==NULL || stricmp(dot,".txt") != 0){
            fprintf(stderr,"Error: impulse text file must have .txt extension.\n");
            return 1;
        }
        fp = fopen(argv[ARG_IMPFILE],"r");
        if(fp==0){
            printf("Cannot open impulse text file %s\n.",argv[ARG_IMPFILE]);
            return 1;
        }
        /* count lines! */
        while(fgets(tmp,80,fp) != NULL)
            size++;
        if(size==0){
            printf("Impulse textfile %s has no data!.\n",argv[ARG_IMPFILE]);
            return 1;
        }
        rewind(fp);
            
        inmonobuf = new double[size+1];
        for(i=0;i < size;i++)  {
            read = fscanf(fp,"%lf",&inmonobuf[i]);
            if(read==0){
                i--;
                continue;
            }
            if(read==EOF)
                break;
            got++;
        }
        imlen = got;
        impulseprops.chans = 1;
        fclose(fp);
    }
    else{
        if((inimpfile = psf_sndOpen(argv[ARG_IMPFILE],&impulseprops, 0))< 0){
            fprintf(stderr,"\nUnable to open impulse file %s",argv[ARG_IMPFILE]);       
            return 0;
        }
        //printf("impulse file sr = %d\n",impulseprops.srate);
        if(srate != impulseprops.srate){
            printf("Error: files must have same sample rate");
            delete []imbuf;
            return 1;
        }
        int  len = psf_sndSize(inimpfile);
        if(len <= 0){
            printf("Error in impulse soundfile - no data!\n");
            return 1;
        }
        if(impulseprops.chans > 1){
            if(! (chans == 1 || chans == impulseprops.chans)){
                fprintf(stderr,"\nChannel mismatch.\n    Infile must be mono or have same channels as irfile.\n");
                return(1);
            }
            chans = impulseprops.chans;
        }    
    }

    imbuf = new double*[chans];

    /* if impulse file is mono, we will need to copy data to the other buffers */
    // allocate  impulseprops.chans buffers ; may need more

    if(inimpfile >=0){
	    if((imlen = genimpframe2(inimpfile,&imbuf,&rms,impulseprops.chans, scalefac)) == 0){
	    	fprintf(stderr,"Error reading impulse file\n");
	    	return(1);
	    }
    }
    
    else if(imlen){        
        genimpframe1(inmonobuf,&imbuf,imlen,scalefac);       
    }
    printf("mean rms level = %.4lf (%.2lf dB)\n",rms, 20.0 * log10(rms));
    

    framesneeded +=  imlen;        /* can  close outfile, when this length reached */

    while(fftlen < imlen*2)			/* convolution style - double-length */
		fftlen <<= 1;
    double norm = sqrt(2.0);       /* relative: rms of 0dBFS sine is 0.707 = 1/root2 */
    // scale factor: most of this sheer guesswork!
    Ninv = fftlen;
    Ninv /= sqrt(imlen*2);
    Ninv *= norm;
    Ninv /= imlen;
    
    // take simple avg of  rms for adjustment factor.
    // may not adequately represent some responses, e.g. with strong attack/earlies, soft reverb tail */
    double rmsdif2 =  (-20.0 * log10(rms)) * 0.5;
    double rmsadjust = pow(10, rmsdif2/20.0);
#ifdef _DEBUG
    printf("rescaling factor = %.6e\n",Ninv);   
    printf("rmsadjust = %.4lf\n",rmsadjust);
#endif
   Ninv /= rmsadjust; 


    /* copy buffers if required */
    for(i = impulseprops.chans;i < chans;i++){
        imbuf[i] = new double[fftlen+2];
        memcpy(imbuf[i],imbuf[0],(fftlen+2)* sizeof(double));
    }         
    oneovrsr = 1.0 / (double) srate;
    
   
	/* main i/o buffers */
	
	insbuf     = new double [fftlen  * chans];
    outsbuf    = new double [fftlen  * chans];
	inbuf      = new double*[chans];
    outbuf     = new double*[chans];  /* overlap-add buffers */
    overlapbuf = new double*[chans];
    
    for(i=0;i < chans;i++){
        inbuf[i]  = new double[fftlen+2];   // main in-place buf 
        outbuf[i] = new double[fftlen+2];        
	    /* channel buffers */	
        overlapbuf[i] = new double[fftlen/2];
        memset(overlapbuf[i],0,sizeof(double)*(fftlen/2));        
    }
#ifdef FFTTEST
    anal = new double[fftlen+2];
#endif
    
    for(i=0;i < chans;i++){		    
	    memset(inbuf[i], 0, sizeof(double) * (fftlen+2));
        memset(outbuf[i],0,sizeof(double) * (fftlen+2));
    }

    
	/* use generic init function */
/*bool phasevocoder::init(int outsrate,int fftlen,int winlen,int decfac,float scalefac,
						pvoc_scaletype stype,pvoc_windowtype wtype,pvocmode mode)
*/
	
	/*create output sfile */
	psf_stype smptype;
	psf_format outformat;
	/* will it be aiff, wav, etc? */
	outformat = psf_getFormatExt(argv[ARG_OUTFILE]);
	if(outformat==PSF_FMT_UNKNOWN){
		fprintf(stderr,"\nOutfile name has unrecognized extension.");
        delete []imbuf;
		return(1);
	}
		
		
	smptype = inprops.samptype;
    if(writefloats)
        smptype= PSF_SAMP_IEEE_FLOAT;

	/*the one gotcha: if output is floats and format is aiff, change to aifc */
	if(smptype==PSF_SAMP_IEEE_FLOAT && outformat==PSF_AIFF){
		fprintf(stderr,"Warning: AIFF output written as AIFC for float samples\n");
		outformat = PSF_AIFC;
	}

	outprops          = inprops;
	outprops.chans    = chans;
	outprops.srate    = srate;
	outprops.format   = outformat;
	outprops.samptype = smptype;
	outprops.chformat = STDWAVE;
    /* if irfile is BFormat, need to set outfile fmt likewise */
    if(impulseprops.chformat==MC_BFMT)
        outprops.chformat = MC_BFMT;
    /* I suppose people will want automatic decoding too...grrr */

    fpeaks = new PSF_CHPEAK[chans];
	if(fpeaks==NULL){
		puts("no memory for fpeak data buffer\n");
		return 1;
	}

	/*last arg: no clipping of f/p samples: we use PEAK chunk */
	if((outsndfile = psf_sndCreate(argv[ARG_OUTFILE],&outprops,0,minheader,PSF_CREATE_RDWR)) <0 ){
		fprintf(stderr,"\nUnable to open outfile %s",argv[ARG_OUTFILE]);
        delete []imbuf;
		return(1);
	}
				

	
	printf("\n");
	
	
	stopwatch(1);

	int written,thisblock,framesread;
    int frameswritten = 0;
	double intime= 0.0;
				
	while((framesread = psf_sndReadDoubleFrames(insndfile,insbuf,imlen)) > 0){        
		written = thisblock =  0;

		for(i = framesread * inprops.chans; i< fftlen * inprops.chans; i++)
			insbuf[i] = 0.0f;
        framesread = imlen;
		memset(inbuf[0],0,(fftlen+2) * sizeof(double));
        if(chans == inprops.chans)  {
            /* must clean buffers! */
		    for(i=0;i < chans;i++)
                memset(inbuf[i],0,(fftlen+2) * sizeof(double));
            mc_split(insbuf,inbuf,imlen * chans, chans);

        }
        else{
            for(i=0;i < chans;i++) {
                memset(inbuf[i],0,(fftlen+2) * sizeof(double));
                memcpy(inbuf[i],insbuf,imlen * sizeof(double));
                memset(outbuf[i],0,sizeof(double) * (fftlen+2));

            }
        }
        if(impulseprops.chans==1){
			
            for(jj = 0; jj < chans;jj++){
#ifdef FFTTEST
                int zz;
                memcpy(anal,inbuf[jj],(fftlen+2) * sizeof(double));
                fft_(anal,anal+1,1,fftlen/2,1,-2);
	            reals_(anal,anal+1,fftlen/2,-2);
                for(zz=0;zz < fftlen+2;zz++)
                    anal[zz] *= 0.001;
#endif
                //rfftwnd_one_real_to_complex(forward_plan[jj],inbuf[jj],NULL);	
				double *danal = inbuf[jj];
				fft_(danal,danal+1,1,fftlen/2,1,-2);
				reals_(danal,danal+1,fftlen/2,-2);
                complexmult(inbuf[jj],imbuf[0],fftlen+2);                
			    //rfftwnd_one_complex_to_real(inverse_plan[jj],(fftw_complex * )inbuf[jj],NULL);
				reals_(danal,danal+1,fftlen/2,2);
				fft_(danal,danal+1,1,fftlen/2,1,2);
            }           
        }
        else{
            for(jj = 0; jj < chans;jj++){
				//rfftwnd_one_real_to_complex(forward_plan[jj],inbuf[jj],NULL);                   				
                double *danal = inbuf[jj];
				fft_(danal,danal+1,1,fftlen/2,1,-2);
				reals_(danal,danal+1,fftlen/2,-2);
				complexmult(inbuf[jj],imbuf[jj],fftlen+2);                
				//rfftwnd_one_complex_to_real(inverse_plan[jj],(fftw_complex * )inbuf[jj],NULL); 
				reals_(danal,danal+1,fftlen/2,2);
				fft_(danal,danal+1,1,fftlen/2,1,2);
            }            
        }
        
        /* overlap-add  for each channel */
        /* TODO: verify  use of imlen here - should it be fftlen/2 -1 ? */
        for(jj=0;jj < chans;jj++){
            for(i=0;i < imlen;i++) {
                outbuf[jj][i] = inbuf[jj][i] + overlapbuf[jj][i];
                overlapbuf[jj][i] = inbuf[jj][i+imlen];
            }
        }
		mc_interl(outbuf,outsbuf,imlen,chans);

        if(inprops.chans == chans){
            for(i=0;i < framesread; i++) {
                for(jj=0;jj < chans;jj++){
                    int  outindex = i*chans + jj;            
                    outsbuf[outindex] *= Ninv;
                    outsbuf[outindex] *= wet;
                    outsbuf[outindex] += dry * insbuf[outindex];
                }
            }
        }
        /* elso mono input */
        else {
            for(i=0;i < framesread; i++) {
                for(jj=0;jj < chans;jj++){
                    int  outindex = i*chans + jj;
                    double inval = dry *  insbuf[i];
                    outsbuf[outindex] *= Ninv;
                    outsbuf[outindex] *= wet;
                    outsbuf[outindex] += inval;
                }
            }
        }

        if((written = psf_sndWriteDoubleFrames(outsndfile,outsbuf,framesread)) != framesread){
		    fprintf(stderr,"\nerror writing to outfile");
		    return(1);		               
        }
        frameswritten += written;

		if(do_timer){
			intime += (double)framesread * oneovrsr;
			printf("Input time: %.2lf\r",intime);
		}    
    }
    /* complete tail */

    if(frameswritten < framesneeded){
        // TODO: imlen? see above
        mc_interl(overlapbuf,outsbuf,imlen,chans);
        int towrite = framesneeded - frameswritten; 
        for(i=0;i < towrite * chans; i++) {
            outsbuf[i] *= Ninv;
            outsbuf[i] *= wet;
        }
        if((written = psf_sndWriteDoubleFrames(outsndfile,outsbuf,towrite)) != towrite){
	        fprintf(stderr,"\nerror writing to outfile");
	        return(1);
	    }
    }

	stopwatch(0);
    if(psf_sndReadPeaks( outsndfile,fpeaks,&peaktime)){
        double peakmax = 0.0;
		printf("PEAK values:\n");
		for(i=0; i < chans; i++)	{
            peakmax = fpeaks[i].val > peakmax ? fpeaks[i].val : peakmax;
			if(fpeaks[i].val < 0.0001f)
				printf("CH %d:\t%e (%.2lf dB)\tat frame %10u :\t%.4lf secs\n",i+1,
				fpeaks[i].val,20.0*log10(fpeaks[i].val),fpeaks[i].pos,(double) (fpeaks[i].pos) / (double)srate);
			else
				printf("CH %d:\t%.4lf (%.2lf dB)\tat frame %10u :\t%.4lf secs\n",i+1,
				fpeaks[i].val,20.0 * log10(fpeaks[i].val),fpeaks[i].pos,(double) (fpeaks[i].pos) / (double)srate);		
		}
        if(peakmax > 1.0)
            printf("Overflows!  Rescale by %.10lf\n",1.0 / peakmax);
	}

	if(insndfile >=0)
		psf_sndClose(insndfile);
	if(outsndfile >=0)
		psf_sndClose(outsndfile);
    if(inimpfile >=0)
	    psf_sndClose(inimpfile);
	psf_finish();
    delete [] fpeaks;
	if(insbuf)
		delete [] insbuf;
	if(outsbuf)
		delete [] outsbuf;
    

    for(i=0;i < chans;i++){
        if(inbuf){       
		    delete [] inbuf[i];
        }           
        if(outbuf){
		    delete [] outbuf[i];
        }       	
        if(imbuf){        
	        delete [] imbuf[i];
        }
        if(overlapbuf)
            delete [] overlapbuf[i];
    }
    delete [] outbuf;
    delete [] inbuf;
    delete [] imbuf;
    delete [] overlapbuf;
	return 0;
}


// insize is raw samplecount,buflen is insize/chans
void mc_split(double* inbuf,double** out,int insize,int chans)
{
    int i,j,buflen = insize/chans;
    double* pinbuf;

    
    for(j=0;j < chans;j++){
        pinbuf = inbuf+j;
        for(i=0;i < buflen;i++){
            out[j][i] = *pinbuf;
            pinbuf += chans;
        }
    }
}


/* insize is m/c frame count */
void mc_interl(double** in,double* out,int insize,int chans)
{
    int i,j;
    double* poutbuf;

    for(j = 0;j < chans;j++){
        poutbuf = out+j;
        for(i=0;i < insize;i++){
            *poutbuf = in[j][i];
            poutbuf += chans;
        }
    }
}

/* OR:  apply scalefac to impulse responses */
void complexmult(double *frame,const double *impulse,int length)
{
	double re,im;
	
	int i,j;
	

	for(i=0,j = 1;i < length;i+=2,j+=2){
		re = frame[i] * impulse[i] - frame[j] * impulse[j];
		im = frame[i] * impulse[j] + frame[j]* impulse[i];
		frame[i] = re;
		frame[j] = im;
	}
}

#ifdef unix
int stricmp(const char *a, const char *b)
{
	while(*a != '\0' && *b != '\0') {
		int ca = islower(*a) ? toupper(*a) : *a;
		int cb = islower(*b) ? toupper(*b) : *b;
        
		if(ca < cb)
			return -1;
		if(ca > cb)
			return 1;
        
		a++;
		b++;
	}
	if(*a == '\0' && *b == '\0')
		return 0;
	if(*a != '\0')
		return 1;
	return -1;
}

int
strnicmp(const char *a, const char *b, const int length)
{
	int len = length;
    
	while(*a != '\0' && *b != '\0') {
		int ca = islower(*a) ? toupper(*a) : *a;
		int cb = islower(*b) ? toupper(*b) : *b;
        
		if(len-- < 1)
			return 0;
        
		if(ca < cb)
			return -1;
		if(ca > cb)
			return 1;
        
		a++;
		b++;
	}
	if(*a == '\0' && *b == '\0')
		return 0;
	if(*a != '\0')
		return 1;
	return -1;
}
#endif






