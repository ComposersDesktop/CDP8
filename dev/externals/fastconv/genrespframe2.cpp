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
 
/* genrespframe2.cpp */
/* generate m/c pvoc frames containing impulse response */
extern "C"
{
#include <portsf.h>
}
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>



#ifdef _DEBUG
#include <assert.h>
#endif

/* deinterleave input buffer of insize samps to array of chans buffers */
void mc_split(double* inbuf,double** out,int insize,int chans);
extern "C"{
	void fft_(double *, double *,int,int,int,int);
	void fftmx(double *,double *,int,int,int,int,int,int *,double *,double *,double *,double *,int *,int[]);
	void reals_(double *,double *,int,int);
}

/*  read impulse data from soundfile */
/* return 0 for error, or size of impulse */
/*we assume pvsys initialized by caller */
int genimpframe2(int ifd, double*** outbufs, double* rms,int imchans, double scalefac)
{
	int i,j;	
	double *insbuf = 0;
    double **inbuf = *outbufs;
	int fftsize = 1;
    int inframes;
    double*  rmsfac = 0;
    double* ampsum = 0;
    double  maxrmsfac = 0.0;
#ifdef _DEBUG
    double *re,*im,*mag;
#endif
	inframes = psf_sndSize(ifd);  // m/c frames
	if(inframes <= 0)
        return 0;
	while(fftsize < inframes*2)			/* convolution style - double-length */
		fftsize <<= 1;
	printf("impulse length = %d frames, fftsize = %d\n",inframes,fftsize);
    insbuf = new double[inframes * imchans];
#ifdef _DEBUG
    re = new double[fftsize/2+1];
    im = new double[fftsize/2+1];
    mag = new double[fftsize/2+1];
#endif
    for(i=0;i < imchans;i++){	
	    inbuf[i] = new double[fftsize+2];
	    if(inbuf[i]==NULL){
		    puts("no memory for file buffer!\n");                    
		    return 0;
	    }
	    memset(inbuf[i],0,(fftsize+2)* sizeof(double));
    }


	if(inframes != psf_sndReadDoubleFrames(ifd,insbuf,inframes)){
		fprintf(stderr,"Error reading impulse data\n");
		
		delete [] insbuf;
        // and do other cleanup!
        
		return 0;
	}
    rmsfac = new double[imchans];
    ampsum = new double[imchans];
    for(i=0;i< imchans;i++) {
        rmsfac[i] = 0.0;
        ampsum[i] = 0.0;
    }
    /* do user scaling first */
    for(i = 0;i < inframes * imchans;i++)
        insbuf[i] *= scalefac;
    /* now try to adjust rms  */

    for(i = 0;i < inframes;i++) {
        for(j=0;j < imchans;j++){
            double val = insbuf[i*imchans + j];
            ampsum[j] += fabs(val);
            rmsfac[j] += val*val; 
        }
    }
    for(j=0;j < imchans;j++){
        //    rmsfac = sqrt(rmsfac);
        rmsfac[j] /= inframes;
        rmsfac[j] = sqrt(rmsfac[j]);
        ampsum[j] /= inframes;
        if(rmsfac[j] > maxrmsfac)
            maxrmsfac = rmsfac[j];
#ifdef _DEBUG
        if(ampsum[j] > maxampsum)
            maxampsum = ampsum[j];
#endif
    }
    /* do the rescaling! */
          
#ifdef _DEBUG        
        printf("ampsum = %.4f\n",maxampsum);
#endif
    
    // now deinterleave to each inbuf
    mc_split(insbuf,inbuf,inframes * imchans,imchans);
    for(i=0;i < imchans;i++){
		double *anal = inbuf[i];
	    //rfftwnd_one_real_to_complex(forward_plan[i],inbuf[i],NULL);
		fft_(anal,anal+1,1,fftsize/2,1,-2);
		reals_(anal,anal+1,fftsize/2,-2);
    }
#ifdef _DEBUG
    /* in order to look at it all */
    double  magmax = 0.0;
    double magsum = 0.0;
    for(i=0;i < fftsize/2+1;i++){
        double thisre, thisim;
        thisre =inbuf[0][i*2];        
        thisim = inbuf[0][i*2+1]; 
        re[i] = thisre;
        im[i] = thisim;
        mag[i] = sqrt(thisre*thisre + thisim*thisim); 
        magsum += (mag[i] * mag[i]);
        if(mag[i] > magmax)
            magmax = mag[i];
    }
    magsum = sqrt(magsum);
    printf("maxamp of FFT = %.4f\n",magmax);
    printf("mean level of FFT = %.4f\n",magsum / (fftsize/2+1));
#endif	
    
    delete [] rmsfac;
    delete [] ampsum;

#ifdef _DEBUG
    delete [] re;
    delete [] im;
    delete [] mag;
#endif
    *rms = maxrmsfac;
	return inframes;
}
/* convert from input double buffer (read from mono text impulse file) */
int genimpframe1(double *insbuf, double*** outbuf, int npoints, double scalefac)
{
	int i;	
    double **inbuf = *outbuf;
	int fftsize = 1;
    int insamps;
#ifdef _DEBUG
    double *re,*im,*mag;
#endif
	insamps = npoints;  // m/c frames
	if(insamps <= 0)
        return 0;
	while(fftsize < insamps*2)			/* convolution style - double-length */
		fftsize <<= 1;
	printf("infile size = %d,impulse framesize = %d\n",insamps,fftsize);
    
#ifdef _DEBUG
    re = new double[fftsize/2+1];
    im = new double[fftsize/2+1];
    mag = new double[fftsize/2+1];
#endif
    	
	inbuf[0] = new double[fftsize+2];
	if(inbuf[0]==NULL){
	    puts("no memory for file buffer!\n");                    
	    return 0;
	}
	memset(inbuf[0],0,(fftsize+2)* sizeof(double)); 
#ifdef _DEBUG
    double ampsum = 0.0;
#endif
    for(i = 0;i < insamps;i++) {
#ifdef _DEBUG
        ampsum += fabs(insbuf[i]);
#endif        
        insbuf[i] *= scalefac;
    }
#ifdef _DEBUG
    printf("amplitude sum of impulse file = %f\n",ampsum);
#endif
    memcpy(inbuf[0],insbuf,npoints* sizeof(double));
	//rfftwnd_one_real_to_complex(forward_plan,inbuf[0],NULL);
	double *anal = inbuf[0];
	fft_(anal,anal+1,1,fftsize/2,1,-2);
	reals_(anal,anal+1,fftsize/2,-2);
    
#ifdef _DEBUG
    /* in order to look at it all */
    for(i=0;i < fftsize/2+1;i++){
        double thisre, thisim;
        thisre =inbuf[0][i*2];        
        thisim = inbuf[0][i*2+1]; 
        re[i] = thisre;
        im[i] = thisim;
        mag[i] = sqrt(thisre*thisre + thisim*thisim);
    }
#endif	
    
    
#ifdef _DEBUG
    delete [] re;
    delete [] im;
    delete [] mag;
#endif
   

	return insamps;
}
