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
 
/*	allpass.c
 *	allpass filter
 *	A. Bentley, Composers' Desktop Project, Version Nov 1987
 */


/*
 *	$Log: allpass.c%v $
 * Revision 3.3  1994/10/31  22:08:15  martin
 * first rcs version
 *
 */

#include <stdio.h>
#include <stdlib.h>	
#include <math.h>
#include <sfsys.h>
#include <cdplib.h>
static char *sfsys_h_rcsid = SFSYS_H_RCSID;
static char *cdplib_h_rcsid = CDPLIB_H_RCSID;

void usage();
void allpass_long(long,long,long,long,const short*,short*,long*,short *);

#define BUFLEN	(32768)
#define FP_ONE  (65536.0)
#define LOG_PS  (16)

PROGRAM_NUMBER(0xf93c2700);

void
main(argc,argv)
int argc;
char *argv[];
{
	int ifd, ofd;
	int sampsize;
	int quickflag = 1;	/* default is fast fixed point calc */
	register int rv;
	float	ip, op;
#ifdef ORIGINAL_VERSION
	long	lip, lop;
	long	i;
#endif
	float 	*delbuf1;
	float   *delbuf2;
	long	rdsamps;
	long	*ldelbuf1;
	short	*sdelbuf2;
	short	*sinbuf;
	short	*soutbuf;
	double  delay; 
	float	gain, prescale = 1.0f;
	long	lgain, lprescale;
	long 	delsamps;
	long	dbp1 = 0, dbp2 = 0;
	long	chans;
	long	isr;
	double	sr;

	if(sflinit("allpass")){
		sfperror("Allpass: initialisation");
		exit(1);
	}
    	while(argv[1] != 0 && argv[1][0] == '-') {
	      switch(argv[1][1]) {
	      case 'f':	
		 quickflag--;
		 argv++;
		 argc--;
		 break;
	      default:
	         usage();
	         break;
	      }
	}
	if(argc < 5 || argc > 6) usage();

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
	/* get command line args */
	delay = abexpr(argv[3],sr);
	if(delay < 0.0){
		fprintf(stderr,"\nAllpass: invalid delay parameter");
		exit(1);
	}
	gain = (float)abexpr(argv[4],sr);
	if((gain < -1.0) || (gain > 1.0)){
		fprintf(stderr,"\nAllpass: gain out of range");
		exit(1);
	}

	if((ofd = sndcreat(argv[2],-1,sampsize)) < 0) {
		  fprintf(stderr,"Cannot open output file %s\n",argv[2]);
		  exit(1);
	}

	lgain = (long) (gain * FP_ONE);
	if(argc > 5) {
		prescale = (float)abexpr(argv[5],sr);
		if((prescale < -1.0) || (prescale > 1.0))
		   fprintf(stderr,
			"Allpass: warning - prescale exceeds +/-1\n");
	}
	lprescale = (long) (prescale * FP_ONE);		
	/* allocate buffer for delay line */
	delsamps = (long) (delay * sr);
	if(quickflag) {
	   if(( sinbuf = (short *) malloc(BUFLEN * sizeof(short))) == NULL) {
		fprintf(stderr,"\nNot enough memory for input buffer");
		exit(1);
	   }
	   if(( soutbuf = (short *) malloc(BUFLEN * sizeof(short))) == NULL) {
		fprintf(stderr,"\nNot enough memory for output buffer");
		exit(1);
	   }
	   if(( ldelbuf1 = (long *) calloc(delsamps, sizeof(long))) == NULL) {
		fprintf(stderr,"\nNot enough memory for delay buffer 1");
		exit(1);
	   }
	   if(( sdelbuf2 = (short *) calloc(delsamps, sizeof(short))) == NULL) {
		fprintf(stderr,"\nNot enough memory for delay buffer 2");
		exit(1);
	   }
	}else{
	   if(( delbuf1 = (float *) calloc(delsamps, sizeof(float))) == NULL) {
		fprintf(stderr,"\nNot enough memory for delay buffer 1");
		exit(1);
	   }
	   if(( delbuf2 = (float *) calloc(delsamps, sizeof(float))) == NULL) {
		fprintf(stderr,"\nNot enough memory for delay buffer 2");
		exit(1);
	   }
	}
	stopwatch(1);
    
	for(;;){
		if((rv = fgetfloat(&ip,ifd)) < 0){
			fprintf(stderr,"\nError in reading file"); 
			exit(1);
		}
		if(!rv) break;
			ip *= prescale;
		op = (-gain) * ip;
		op += delbuf1[dbp1];
		op += gain * delbuf2[dbp2];
			delbuf1[dbp1++] = ip;
		delbuf2[dbp2++] = op;

		if(dbp1 >= delsamps) dbp1 = 0;
		if(dbp2 >= delsamps) dbp2 = 0;

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
}

void
usage()
{
	fprintf(stderr,"\nAllpass - A Groucho Program: $Revision: 3.3 $");
	fprintf(stderr,"\n%s\n",
	"usage: allpass [-f] infile outfile delay gain [prescale]");
	exit(0);
}

/*RWD: my allpass functions */
//TODO: optimize using ptr arithmetic
void allpass_long(long prescale, 
				   long buflen, 
				   long l_gain,
				   long l_delsamps,
				   const short *s_inbuf,
				   short *s_outbuf,
				   long *l_delbuf1,
				   short *s_delbuf2)
{
	int i; 
	static long	db_p1 = 0;
	static long db_p2 = 0;
	//do allpass on block of samples
	for(i = 0; i < buflen; i++){
		   long lip,lop;
		   lip = s_inbuf[i] * prescale;
		   lop = (-l_gain) * (lip >> LOG_PS);
		   lop += l_delbuf1[db_p1];
		   lop += l_gain * s_delbuf2[db_p2];

		   l_delbuf1[db_p1++] = lip;
		   s_outbuf[i] = s_delbuf2[db_p2++] = (short) (lop >> LOG_PS);

		   if(db_p1 >= l_delsamps) db_p1 = 0;
		   if(db_p2 >= l_delsamps) db_p2 = 0;
    }
	   //block done
	//return &soutbuf[0];			//if we return anything?
}
