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
 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//lpcomb.cpp
extern "C" {
#include <sfsys.h>
#include <cdplib.h>	   //NB requires stdio.h etc - time to change this?
}

#include "reverberator.h"

//straight from Csound: nreverb (vdelay.c)
static bool prime( int val )
{
    int i, last;
	
    last = (int)sqrt( (double)val );
    for ( i = 3; i <= last; i+=2 ) {
      if ( (val % i) == 0 ) return false;
    }
    return true;
}

static int to_prime(double dur,double sr)
{
		int time = (int) (dur * sr);
		if(time % 2== 0) time += 1;
		while(!prime(time)) 
			time +=2;
		return time;
}

void
usage()
{

	printf("\nusage: lpcomb  infile outfile looptime rvbtime filtgain");
	exit(0);
}

void
main(int argc,char *argv[])

{
	int		ifd, ofd;	
	//double  delaytime,gain;
	double	rvbtime,gain2,filtgain,dloop; 	
	long	isr,chans,sampsize,ilooptime;
	double	sr;
	lpcomb	*comb1 = 0;


	if(sflinit("lpcomb")){
		sfperror("lpcomb: initialisation");
		exit(1);
	}


	if(argc < 6)
		usage();
	dloop = atof(argv[3]);
	if(dloop <= 0.0){
		fprintf(stderr,"\nlpcomb: invalid looptime parameter");
		exit(1);
	}

	rvbtime = atof(argv[4]);
	if(rvbtime <= 0.0){
		fprintf(stderr,"\nlpcomb: invalid delay parameter");
		exit(1);
	}
	filtgain = atof(argv[5]);
	if((filtgain < 0.0) || (filtgain > 1.0)){
		fprintf(stderr,"\nlpcomb: filter gain %lf out of range",filtgain);
		exit(1);
	}

	if(argv[1] != 0) {
	    if((ifd = sndopen(argv[1])) < 0) {
		   fprintf(stderr,"Allpass: cannot open input file %s\n", argv[1]);
		   exit(1);
		}
	}
	if(sndgetprop(ifd,"sample rate",(char *) &isr, sizeof(long)) < 0){
		fprintf(stderr,
		  "\nAllpass: failed to read sample rate for file %s",argv[1]);
		exit(1);
	}
	sr = (double) (isr);
	if(sndgetprop(ifd,"channels",(char *) &chans, sizeof(long)) < 0){
		fprintf(stderr,
		  "\nAllpass: failed to read channel data for file %s", argv[1]);
		exit(1);
	}
	if(chans != 1){
		fprintf(stderr,"\nAllpass works only on mono files");
		exit(1);
	}
	if(sndgetprop(ifd,"sample type",(char *) &sampsize, sizeof(long)) < 0){
		fprintf(stderr,
		  "\nAllpass: failed to read sample type for file %s", argv[1]);
		exit(1);
	}


	if((ofd = sndcreat(argv[2],-1,sampsize)) < 0) {
		  fprintf(stderr,"Cannot open output file %s\n",argv[2]);
		  exit(1);
	}
	ilooptime  = to_prime(dloop, sr);	
	gain2 = exp( log(0.001)* (dloop/rvbtime)) * (1. - filtgain) ;
	printf("\ngain2 = %lf\tfiltgain = %lf",gain2,filtgain);
	comb1 = new lpcomb(ilooptime,gain2,filtgain,1.0);
	if(!comb1->create()){
		printf("\nFailed to create comb filter");
		exit(1);
	}

	stopwatch(1);

	for(;;){
		int rv;
		float ip,op;

		if((rv = fgetfloat(&ip,ifd)) < 0){
			fprintf(stderr,"\nError in reading file"); 
			exit(1);
		}
		if(!rv)
			break;
		op = comb1->tick(ip);
		if(fputfloat(&op,ofd) < 1){
			fprintf(stderr,"\nError writing to output file"); 
			exit(1);
		}
	}
	stopwatch(0);
	sndclose(ifd);
	if(sndputprop(ofd,"sample rate", (char *) &isr, sizeof(long)) < 0){
		fprintf(stderr,"\nAllpass: failed to write sample rate");
	}
	if(sndputprop(ofd,"channels",(char *) &chans, sizeof(long)) < 0){
		fprintf(stderr,"\nAllpass: failed to write channel data");
	}
	sndclose(ofd);
	delete comb1;
	sffinish();
}
