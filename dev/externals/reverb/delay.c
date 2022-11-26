/*
 * Copyright (c) 1983-2013  Composers Desktop Project Ltd
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
 
/* delay.c  
 * generalised delay line featuring
 * feedthrough, feedforward and feedback
 * A. Bentley, Composers' Desktop Project, Nov 1987.
 * Revised and extended to stereo and command-line mode 
 * by Richard Dobson Oct 1993.
 */ 

static char *rcsid = "$Id: delay.c%v 3.3 1994/10/31 22:30:12 martin Exp $";
/*
 *	$Log: delay.c%v $
 * Revision 3.3  1994/10/31  22:30:12  martin
 * first rcs version
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "sfsys.h"
#include "cdplib.h"
static char *sfsys_h_rcsid = SFSYS_H_RCSID;
static char *cdplib_h_rcsid = CDPLIB_H_RCSID;

#define remainder	my_remainder

#define BUFLEN	  (4096)
#define	DELBUFSIZ (176400)	/* allows 2sec delay in stereo */
#define ONE_FLT   (65536.0)

void domono();
void dostereo();
void usage();
void delinit(int cmndline,char *argv[]);
void initfbuf();
void initlbuf();

int 	Funcflag = 0;
int 	Quickflag = 1;
long 	Nchans;
long	Delaysamps;
long    bufcnt;
long	remainder;
float 	*Fdelbuf;
long	*Ldelbuf;
double 	Sr;
double	Delay;
float	Feedforgain;
float	Feedbckgain;
float	Delaygain;
float	Prescale = 1.0F;
double	Invert =1.0;
double  Rescale;
long	Lfeedforgain;
long	Lfeedbckgain;
long	Ldelaygain;
long	Lprescale;
double  Trailtime = 0.;
long	Trailsamps = 0;
int	ifd;	 	/* input file descriptor */
int	ofd;		/* output file descriptor */
long 	isr;
long	sampsize;
short  	*inbuf;
short	*outbuf;

static char version[] =
	"~CDPVER~GROUCHO: DELAY Portable version 3.0, REVISED BY R. DOBSON OCT 1993.";
 
PROGRAM_NUMBER(0xf93c2705);

void
main(argc,argv)
int argc;
char *argv[];
{
int cmndline = 0;
char *ptr;
	
	if(sflinit("delay")) {
		sfperror("Delay: initialisation");
		exit(1);
	}
    	while(argv[1] != 0 && argv[1][0] == '-') {
	      switch(argv[1][1]) {
	          case 'f':	
		 	Quickflag--;
		 	break;
		  case 'p':
			ptr = (char *) &(argv[1][2]);
			if(*ptr==0)
			{
				usage();
			}
			Prescale = (float)atof(ptr);
			if(Prescale==0.0)
			{
			    printf("\ndelay: -p error, prescale = 0.0!\n");
			    usage();
			}
			break;
		   case 'i':
			Invert = -1.0;
			break;
	          default:
	         	usage();
	         	break;
	      }
	argv++; argc--;
	}
	if(argc < 3) usage();
	if(argc>4)
	{
	    if(argc<7)
		{
		fprintf(stderr,"\ndelay: incorrect no of parameters");
		usage();
		}
	    else
		{
		cmndline=1;
		}
	}
	if((inbuf = 
		(short *) malloc((BUFLEN) * sizeof(short))) == NULL) {
		fprintf(stderr,"\ndelay: not enough memory for input buffer");
		exit(1);
	}
	if((outbuf = 
		(short *) malloc(BUFLEN * sizeof(short))) == NULL) {
		fprintf(stderr,"\ndelay: not enough memory for output buffer");
		exit(1);
	}
      	if((ifd = sndopen(argv[1])) < 0) {
	  fprintf(stderr,"\ndelay: unable to open input file\n");
 	  exit(1);
	} 
	if(sndgetprop(ifd,"sample rate",(char *) &isr, sizeof(long)) < 0) {
		fprintf(stderr,"\ndelay: failed to get sample rate");
		exit(1);
	}
	if(sndgetprop(ifd,"channels",(char *) &Nchans, sizeof(long)) < 0) {
		fprintf(stderr,"\ndelay: failed to get channel data");
		exit(1);
	}
	if(sndgetprop(ifd,"sample type",(char *) &sampsize, sizeof(long)) < 0){
		fprintf(stderr,"\ndelay: failed to get sample type");
		exit(1);
	}
	if(Nchans > 2) {
	  fprintf(stderr,"\ndelay: too many channels! Mono or stereo only.\n");
	  exit(1);
	}

	Sr = (double) isr;

	delinit(cmndline,argv);

	if((ofd = sndcreat(argv[2],-1,sampsize)) < 0) {
	  fprintf(stderr,"\ndelay: unable to open output file\n");
	  exit(1);
	}

	if(!Quickflag) 
		initfbuf();
	else
		initlbuf();
	stopwatch(1);
	if(Nchans==1) {domono();} else {dostereo();}
	stopwatch(0);
	sndclose(ifd);
   	if(sndputprop(ofd,"sample rate",(char *) &isr, sizeof(long)) < 0){
	fprintf(stderr,"\ndelay: failed to write sample rate");
   	}
   	if(sndputprop(ofd,"channels", (char *) &Nchans, sizeof(long)) < 0){
	fprintf(stderr,"\ndelay: failed to write channel data");
   	}
	free(inbuf);
	free(outbuf);
	if(!Quickflag) {free(Fdelbuf);} else {free(Ldelbuf);}
   	sndclose(ofd);
	sffinish();
}


void
domono()
{
	int 	i;	
	long	rdsamps;
	float	input,output;
	long	linput,loutput;
	long	ltemp1,ltemp2;
	long	ipptr=0,opptr=0;

	do		/* while rdsamps */
	{
	   	if((rdsamps = fgetsbuf(inbuf, BUFLEN, ifd)) < 0) {
        	fprintf(stderr,"\ndelay: failure to read input file\n");
          	exit(1);
	    }
	    
	    for(i=0;i<rdsamps;i++)
		{
			
				/* delay line */
			input = inbuf[i] * Prescale;
			output = (input * Feedforgain) +		  //dry signal
				(Fdelbuf[opptr] * Delaygain);
			Fdelbuf[ipptr] = Fdelbuf[opptr++] * Feedbckgain; 
			Fdelbuf[ipptr++] += input ;
			outbuf[i] = (short) output;				  //dry + wet

			if(ipptr >= Delaysamps) ipptr -= Delaysamps;
			if(opptr >= Delaysamps) opptr -= Delaysamps;
			 
			
			if(ipptr < 0 || opptr < 0) {			
				 printf(
			  "\ninternal error, ipptr=%d,opptr=%d\n",ipptr,opptr);
			}	
		}	
        if(fputsbuf(outbuf,rdsamps,ofd) < rdsamps) {
		     fprintf(stderr,
				"\ndelay: failure in writing to output file\n");
				exit(1);
		}
		inform(rdsamps,Sr);
	} while(rdsamps > 0);

	    	/* now do trailer	*/
    rdsamps=BUFLEN;
	if (Trailsamps>0) {
		do	{	/* while bufcnt */
		    
		  	if(!bufcnt) 
				rdsamps = remainder;
					    	
			for(i=0;i<rdsamps;i++) {
		       	output=(Fdelbuf[opptr] * Delaygain);
		       	Fdelbuf[ipptr++]  = 
				Fdelbuf[opptr++] *Feedbckgain;
		       	outbuf[i] = (short) output;
		       	if(ipptr >= Delaysamps) ipptr -= Delaysamps;
		       	if(opptr >= Delaysamps) opptr -= Delaysamps;
		    }
					  		
        	if(fputsbuf(outbuf,rdsamps,ofd) < rdsamps){
				fprintf(stderr,
					"\ndelay: failure in writing to output file\n");
				exit(1);
		  	}
    		inform(BUFLEN,Sr);
			bufcnt -=1;
    	} while(bufcnt>=0);
    }				/* end if Trailsamps */
}				/* end domono	*/

void
dostereo()			/* do stereo delay */
{
int 	i;	
long	rdsamps;
float	Linput,Rinput;
float 	Loutput,Routput;
long	Llinput,Rlinput;
long	Lloutput,Rloutput;
long	Ltemp1,Ltemp2;
long	Rtemp1,Rtemp2;
long	Lipptr = 0,Ripptr = 1;
long	Lopptr = 0,Ropptr = 1;

    do			/* while rdsamps */
    {
	if((rdsamps = fgetsbuf(inbuf,BUFLEN,ifd)) < 0)
	{
		fprintf(stderr,"\ndelay: failure to read input file\n");
		exit(1);
	}
	    for(i=0;i<(rdsamps-1);i+=2)
	    {
		if(!Quickflag)
		{
			Linput=inbuf[i]*Prescale;
			Rinput=inbuf[i+1]*Prescale;
			Loutput=(Linput*Feedforgain) + 
				(Fdelbuf[Lopptr]*Delaygain);
			Routput=(Rinput*Feedforgain) + 
				(Fdelbuf[Ropptr]*Delaygain);
			Fdelbuf[Lipptr] = Fdelbuf[Lopptr]*Feedbckgain;
			Lopptr+=2;
			Fdelbuf[Ripptr] = Fdelbuf[Ropptr]*Feedbckgain;
			Ropptr+=2;
			Fdelbuf[Lipptr]+=Linput;
			Fdelbuf[Ripptr]+=Rinput;
			Lipptr+=2;
			Ripptr+=2;
			outbuf[i] = (short) Loutput;
			outbuf[i+1] = (short) Routput;

			if(Lipptr >= Delaysamps) Lipptr -= Delaysamps;
			if(Ripptr >= Delaysamps) Ripptr -= Delaysamps;
			if(Lopptr >= Delaysamps) Lopptr -= Delaysamps;
			if(Ropptr >= Delaysamps) Ropptr -= Delaysamps;
			}
		else
		{
			Llinput = inbuf[i] * Lprescale;
			Rlinput = inbuf[i+1] * Lprescale;
			Ltemp1 = Llinput >> 16;
			Rtemp1 = Rlinput >> 16;
			Ltemp2 = Ldelbuf[Lopptr] >> 16;
			Rtemp2 = Ldelbuf[Ropptr] >> 16;
				Lopptr += 2;
				Ropptr += 2;
			Lloutput = (Ltemp1 * Lfeedforgain) + 
				(Ltemp2 * Ldelaygain);
			Rloutput = (Rtemp1 * Lfeedforgain) +
				(Rtemp2 * Ldelaygain);
			Ldelbuf[Lipptr] = Ltemp2 * Lfeedbckgain;
			Ldelbuf[Ripptr] = Rtemp2 * Lfeedbckgain;
			Ldelbuf[Lipptr] += Llinput;
			Ldelbuf[Ripptr] += Rlinput;
				Lipptr += 2;
				Ripptr += 2;
			outbuf[i] = (short)(Lloutput>>16);
			outbuf[i+1] = (short)(Rloutput>>16);

			if(Lipptr >= Delaysamps) Lipptr -= Delaysamps;    
			if(Ripptr >= Delaysamps) Ripptr -= Delaysamps;
			if(Lopptr >= Delaysamps) Lopptr -= Delaysamps;
			if(Ropptr >= Delaysamps) Ropptr -= Delaysamps;
		}
	}
	if(fputsbuf(outbuf,rdsamps,ofd) < rdsamps) {
		fprintf(stderr,"\ndelay: failure in writing to output file\n");
		exit(1);
	}
	inform(rdsamps/2,Sr);
   }while(rdsamps > 0);		/* end do */

/* now do trailer */

    rdsamps=BUFLEN;
    if(Trailsamps>0)
    {
	do
	{
		if(!bufcnt) rdsamps = remainder;
		if(!Quickflag)
		    {
		    for(i=0;i<(rdsamps-1);i+=2)
			{
			Loutput = Fdelbuf[Lopptr]*Delaygain;
			Routput = Fdelbuf[Ropptr]*Delaygain;
			Fdelbuf[Lipptr] = Fdelbuf[Lopptr]*Feedbckgain;
			Fdelbuf[Ripptr] = Fdelbuf[Ropptr]*Feedbckgain;
			Lopptr+=2;
			Ropptr+=2;
			Lipptr+=2;
			Ripptr+=2;
			outbuf[i] = (short) Loutput;
			outbuf[i+1] = (short) Routput;
			if(Lipptr >= Delaysamps) Lipptr -= Delaysamps;
			if(Ripptr >= Delaysamps) Ripptr -= Delaysamps;
			if(Lopptr >= Delaysamps) Lopptr -= Delaysamps;
			if(Ropptr >= Delaysamps) Ropptr -= Delaysamps;
			}
		    }
		else
		    {
		    for(i=0;i<(rdsamps-1);i+=2)
		    	{
			Ltemp2 = Ldelbuf[Lopptr] >> 16;
				Lopptr+=2;
			Rtemp2 = Ldelbuf[Ropptr] >> 16;
				Ropptr+=2;
			Lloutput = Ltemp2 * Ldelaygain;
			Rloutput = Rtemp2 * Ldelaygain;
			Ldelbuf[Lipptr] = Ltemp2*Lfeedbckgain;
			Ldelbuf[Ripptr] = Rtemp2*Lfeedbckgain;
			Lipptr +=2;
			Ripptr +=2;			
			outbuf[i] = (short)(Lloutput>>16);
			outbuf[i+1] = (short)(Rloutput>>16);
			if(Lipptr >= Delaysamps) Lipptr -= Delaysamps;
			if(Ripptr >= Delaysamps) Ripptr -= Delaysamps;
			if(Lopptr >= Delaysamps) Lopptr -= Delaysamps;
			if(Ropptr >= Delaysamps) Ropptr -= Delaysamps;
			}
		    }
		if(fputsbuf(outbuf,rdsamps,ofd) < rdsamps) {
		fprintf(stderr,"\ndelay: failure in writing to output file\n");
		exit(1);
		}
		inform(rdsamps/2,Sr);
		bufcnt -=1;
	    } while (bufcnt>=0);
	}				/* end if */
   }					/* end dostereo */		
 
void
usage()
{
	fprintf(stderr,"delay - A Groucho Program: $Revision: 3.3 $\n");
	fprintf(stderr,
"usage: delay [-f][-pN][-i] infile outfile [4 cmndline parameters]\n");
	fprintf(stderr,
"	where -f sets output in floatsams, -p sets prescale = N for infile,\n");
	fprintf(stderr,
"	and -i inverts dry signal (for allpass filters).\n");
	fprintf(stderr,"\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
"Cmndline option: after [flags] filenames, enter all parameters in this order:",
"	delaytime mix feedbackgain trailertime",
"	 0.0  	< delaytime (msecs) 	<= maxdelay",
"	 0.0 	<= mix		 	<= 1.0",
"	-1.0 	<= feedbackgain    	<= 1.0",
"	 0.0 	<= trailertime (secs)	<= 30.0",
"where maxdelay =  8000ms @22050 mono",
"               =  4000ms @22050 stereo, and 44100 mono",
"               =  2000ms @44100 stereo"); 
	exit(0);
}

void
delinit(int cmndlin, char *argv[])
{
	double temp,maxdelay,mix;
	char msg[80];
	/*	Some jiggery pokery is needed in setting the
	 *	fixed point variables to avoid overloading
	 *	the input to the delay line. The input is
	 *	divided by two in conjunction with prescaling
	 *	and the feedthrough and delay outputs are
	 *	raised by two to compensate.
	 */
	maxdelay = ((double) (DELBUFSIZ) / Sr) * 1000.0; /* milliseconds */
	if(Nchans==2) {maxdelay /= 2;}
	if(cmndlin) 
	{
		temp = atof(argv[3]);
		if (temp > maxdelay)
		{
			printf("delay: delay time too long\n");
			exit(1);
		}
		Delaysamps = (long) ((Sr *temp) / 1000.0);
		mix = atof(argv[4]);
		if((mix < 0.0) || (mix > 1.0))
		{
			printf("delay: mix value out of range\n");
			exit(1);
		}
		Feedbckgain = (float)atof(argv[5]);
		if((Feedbckgain < -1.0) || (Feedbckgain > 1.0))
		{
			printf("delay: feedbackgain out of range\n");
			exit(1);
		}
		Trailtime = atof(argv[6]);
		if((Trailtime < 0.0) || (Trailtime > 1000.0))
		{
			printf("delay: trailtime out of range\n");
			exit(1);
		}
				
	}
	else		/* ask for params from terminal */
	{
		printf("max delay in msec = %f\n",maxdelay);
		sprintf(msg,
"Give delay time in milliseconds			:");
		temp  = accexpr(msg,0.0,maxdelay,0.0,0,Sr);
		if(temp==0.0)
		{
	printf("\nzero delay time!!! No point in running program. Bye!\n");
	exit(1);
		}
		Delaysamps = (long) ((Sr * temp) / 1000.0);
		printf("\ndelay length in samples = %ld \n",Delaysamps);
		sprintf(msg,
"Give mix proportion (0.0 <= level <= 1.0)		:");
		mix = accexpr(msg,0.0,1.0,0.0,0,Sr);
		sprintf(msg,
"Give feedback gain (-1.0 <= level <= 1.0)		:");
		Feedbckgain = (float)accexpr(msg,-1.0,1.0,0.0,0,Sr);
		sprintf(msg,
"Give trailer time (default = 0 <= trailtime <= 30.0)	:");
		Trailtime = accexpr(msg, 0.0,30.0,0.0,1,Sr);
	}
/* now massage parameters 			*/
/* nb delaysamps is per channel here 	*/
 
	Feedforgain = (float)((1.0 - mix)*Invert);	
	Lfeedbckgain = (long) (Feedbckgain * ONE_FLT);
	Delaygain = (float)mix;
	Ldelaygain = (long) (Delaygain * ONE_FLT * 2.0);	
	Rescale = (1.0 / (Feedbckgain + 1.0)); /* i/p compensation */
	Prescale *= (float)Rescale;
	Lprescale= (long) (Prescale * ONE_FLT * .5);
	Feedforgain /= (float)Rescale;
	Lfeedforgain = (long) (Feedforgain * ONE_FLT * 2.0);	
	Trailsamps = (long) (Trailtime * Sr);
	if(Nchans==2) Trailsamps *= 2;
	bufcnt = Trailsamps/BUFLEN;
	remainder = Trailsamps % BUFLEN;
}

void
initfbuf()
{
	Delaysamps*=Nchans;
	if((Fdelbuf = (float *) calloc(Delaysamps+2, sizeof(float))) == NULL){
	   fprintf(stderr,"\ndelay: not enough memory for main delay buffer");
	   exit(1);
	}
}

void
initlbuf()
{
	Delaysamps*=Nchans;
	if((Ldelbuf = (long *) calloc(Delaysamps+4, sizeof(long))) == NULL){
	    fprintf(stderr,"\ndelay: not enough memory for main delay buffer");
	    exit(1);
	}
}
