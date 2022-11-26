/*
 * Copyright (c) 1983-2013 Trevor Wishart and Composers Desktop Project Ltd
 * http://www.trevorwishart.co.uk
 * http://www.composersdesktop.com
 *
 This file is part of the CDP System.

    The CDP System is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    The CDP System is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with the CDP System; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
 *
 */



/*
 *	$Log: stretcha.c%v $
 * Revision 1.1  1995/12/20  17:28:54  trevor
 * Initial revision
 *
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "sfsys.h"
				   
const char* cdp_version = "7.1.0";

#define	PROG "STRETCHA"

#define STRETCH			(1)
#define DUR 			(1)
#define	BEATS			(2)
#define	TEMPO			(4)
#define BEATS_AND_TEMPO	(6)
#define	MINUTE			(60.0)
#define	ENDOFSTR		('\0')
#define FLTERR	    0.000002

int		inputt = 0, outputt = 0, result = 0;
double	dur, beats, tempo, odur, obeats, otempo, answer;

char	zag = (char)0, tthis;

void	usage(void), help(void), logo(void);
//void	read_properties(void);
//void	write_properties(void);
void	process_file(char *);
void 	do_fraction(double);
int 	flteq(double,double);



#define finish(x)  return((x))

int
main(int argc, char *argv[])
{
	int n,ifd = -1;
    int infilesize = 0;
    SFPROPS props;

	if(sflinit("stretcha")) {
		sfperror("Initialisation");
		finish(1);
	}
    if(argc==2 && strcmp(argv[1],"--version")==0){
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if(argc==2 && !strcmp(argv[1],"-h")) {
		help();
        return 0;
    }
    if(argc < 4 || argc > 6){
		usage();
        return 1;
    }
	if(*argv[1]++!='-') {
	 	fprintf(stderr,"Parameter 1 must be a flag.\n");
		finish(1);
	}
 	switch(*argv[1]) {
	case('s'):	result = STRETCH;	break;
	case('c'):	result = BEATS;		break;
	default:
	 	fprintf(stderr,"Unknown flag '-%c' as 1st parameter.\n",*argv[1]);
		finish(1);
	}
	n = 2;
	while(n<argc) {
		if(*argv[n]++!='-') {
		 	fprintf(stderr,"Unknown parameter '%s'.\n",--argv[n]);
			finish(1);
		}
		tthis = *argv[n];
		switch(*argv[n]++) {
		case('d'):
			if(inputt & DUR) {
				if(tthis==zag)
					fprintf(stderr,"Repetition of flag -%c\n",zag);
				else
					fprintf(stderr,"You cannot have 2 input durations (-d and -f).\n");
				finish(1);
			}
			zag = 'd';
			if((inputt & TEMPO) || (inputt & BEATS)) {
				fprintf(stderr,"You cannot have input duration (-%c) AND input tempo (-t) or beatcnt (-b).\n",zag);
				finish(1);
			}
			inputt |= DUR;
			if(sscanf(argv[n],"%lf",&dur)!=1) {
				fprintf(stderr,"Cannot read duration value with -d flag.\n");
				finish(1);
			}
			break;
		case('f'):
			if(inputt & DUR) {
				if(tthis==zag)
					fprintf(stderr,"Repetition of flag -%c\n",zag);
				else
					fprintf(stderr,"You cannot have 2 input durations (-d and -f).\n");
				finish(1);
			}
			zag = 'f';
			if((inputt & TEMPO) || (inputt & BEATS)) {
				fprintf(stderr,"You cannot have input duration (-%c) AND input tempo (-t) or beatcnt (-b).\n",zag);
				finish(1);
			}
			inputt |= DUR;
			//open_infile(argv[n]);
			//read_infile_size();
			//read_and_check_properties();
            ifd = sndopenEx(argv[n],0,CDP_OPEN_RDONLY);
            if(ifd < 0){
                fprintf(stderr,"unable to open sfile %s: %s\n",argv[n],sferrstr());
                return 1;
            }
            infilesize = sndsizeEx(ifd);
            if(infilesize < 0){
                fprintf(stderr,"error reading sfile %s: %s\n",argv[n],sferrstr());
                return 1;
            }
            if(snd_headread(ifd,&props) < 0){
                fprintf(stderr,"error reading sfile props: %s: %s\n",argv[n],sferrstr());
                return 1;
            }
			dur	 = (double)(infilesize/props.chans)/(double) props.srate;
			break;
		case('b'):
			if(inputt & BEATS)
				fprintf(stderr,"Repetition of flag -b\n");
			inputt |= BEATS;
			if(inputt & DUR) {
				fprintf(stderr,"You cannot have input duration (-%c) AND input beatcnt (-b).\n",zag);
				finish(1);
			}
			if(sscanf(argv[n],"%lf",&beats)!=1) {
				fprintf(stderr,"Cannot read number of beats with -b flag.\n");
				finish(1);
			}
			break;
		case('t'):
			if(inputt & TEMPO)
				fprintf(stderr,"Repetition of flag -t\n");
			inputt |= TEMPO;
			if(inputt & DUR) {
				fprintf(stderr,"You cannot have input duration (-%c) AND input tempo (-t).\n",zag);
				finish(1);
			}
			if(sscanf(argv[n],"%lf",&tempo)!=1) {
				fprintf(stderr,"Cannot read tempo value with -t flag.\n");
				finish(1);
			}
			break;
		case('D'):
			if(outputt & DUR)
				fprintf(stderr,"Repetition of flag -D\n");
			outputt |= DUR;
			if((outputt & TEMPO) || (outputt & BEATS)) {
				fprintf(stderr,"You can't have output duration (-D) AND output tempo (-T) or output beatcnt (-B).\n");
				finish(1);
			}
			if(sscanf(argv[n],"%lf",&odur)!=1) {
				fprintf(stderr,"Cannot read output duration value with -D flag.\n");
				finish(1);
			}
			break;
		case('B'):
			if(outputt & BEATS)
				fprintf(stderr,"Repetition of flag -B\n");
			outputt |= BEATS;
			if(outputt & DUR) {
				fprintf(stderr,"You can't have output duration (-D) AND output beatcnt (-B).\n");
				finish(1);
			}
			if(sscanf(argv[n],"%lf",&obeats)!=1) {
				fprintf(stderr,"Cannot read number of output beats with -B flag.\n");
				finish(1);
			}
			break;
		case('T'):
			if(outputt & TEMPO)
				fprintf(stderr,"Repetition of flag -T\n");
			outputt |= TEMPO;
			if(outputt & DUR) {
				fprintf(stderr,"You can't have output duration (-D) AND output tempo (-T).\n");
				finish(1);
			}
			if(sscanf(argv[n],"%lf",&otempo)!=1) {
				fprintf(stderr,"Cannot read output tempo value with -T flag.\n");
				finish(1);
			}
			break;
		default:
		 	fprintf(stderr,"Unknown flag '-%c'.\n",*--argv[n]);
			finish(1);
		}
		n++;
	}
	if(!inputt) {
		fprintf(stderr,"No input values given (-d,-f,-t,-b).\n");
		finish(1);
	}
	if(!outputt) {
		fprintf(stderr,"No output values given (-D,-T,-B).\n");
		finish(1);
	}
	if(result==BEATS && outputt!=TEMPO) {
		if(outputt & TEMPO)
			fprintf(stderr,"For a crotchet count (-c) you must have output tempo (-T) ONLY.\n");
		else
			fprintf(stderr,"For a crotchet count (-c) you must have output tempo (-T) (ONLY).\n");
		finish(1);
	}
	if((outputt & BEATS) && !(outputt & TEMPO)) {
		fprintf(stderr,"Output beatcount given (-B) WITHOUT output tempo (-T).\n");
		finish(1);
	}
	if((inputt & BEATS) && !(inputt & TEMPO)) {
		fprintf(stderr,"Input beatcount given (-b) WITHOUT input tempo (-t).\n");
		finish(1);
	}
	if(inputt==TEMPO && result!=STRETCH) {
		fprintf(stderr,"Tempo input (-t) ALONE, can only be used with stretching (-s).\n");
		fprintf(stderr,"Otherwise you need an input beatcount (-b).\n");
		finish(1);
	}
	if(inputt==TEMPO && outputt!=TEMPO) {
		fprintf(stderr,"Tempo input ONLY (-t), with stretching (-s), needs a tempo ONLY output (-T).\n");
		fprintf(stderr,"Otherwise you need an input beatcount (-b).\n");
		finish(1);
	}
	if(inputt!=TEMPO &&	result==BEATS && outputt!=TEMPO) {
		fprintf(stderr,"Output must be tempo ONLY (-T) with crotchet count (-b).\n");
		finish(1);
	}
	if(inputt!=TEMPO &&	result==STRETCH && outputt==TEMPO) {
		fprintf(stderr,"Output must be duration (-D: or -T WITH -B) if stretching (-s) a duration.\n");
		finish(1);
	}
	switch(result) {
	case(BEATS):
		if(inputt==BEATS_AND_TEMPO)
			dur  = beats * MINUTE/tempo;
		answer = dur * otempo/MINUTE;
		printf("Number of beats at new tempo = %lf ",answer);
		do_fraction(answer);
		if(!flteq(answer,1.0) && answer < 1.0)
			printf("of a ");
		printf("crotchet");
		if(!flteq(answer,1.0) && answer > 1.0)
			printf("s");
		printf("\n");
		break;						
	case(STRETCH):
		if(inputt==TEMPO)
			answer = tempo/otempo;
		else {
			if(inputt == BEATS_AND_TEMPO)
				dur  = beats * MINUTE/tempo;
			if(outputt== BEATS_AND_TEMPO)
				odur = obeats * MINUTE/otempo;
			answer = odur/dur;
		}
		printf("Stretchfactor = %lf\n",answer);
		break;
	}
	finish(0);
}

/*************************************** USAGE ***********************************/

void usage(void)
{
	logo();
	fprintf(stderr, "USAGES:\n");
	fprintf(stderr, "stretcha -c -ddur|-fsndfile|(-bbeats -ttempo) -Touttempo\n");
	fprintf(stderr, "stretcha -s -ddur|-fsndfile|(-bbeats -ttempo) -Doutdur|(-Boutbeats -Touttempo)\n");
	fprintf(stderr, "stretcha -s             -tintempo             -Touttempo\n\n");
	fprintf(stderr, "-----------for more information, type 'stretcha -h' -------------\n");
}
/*************************************** HELP ***********************************/
															  
void help(void)
{
	fprintf(stderr, "stretcha -s takes...\n");
	fprintf(stderr, "\t    An INPUT DURATION and an OUTPUT DURATION.\n");
	fprintf(stderr, "\tOR: An INPUT TEMPO    and an OUTPUT TEMPO.\n");
	fprintf(stderr, "It calculates stretch required to timewarp a sound from one frame to other.\n\n");
	fprintf(stderr, "stretcha -c takes...\n");
	fprintf(stderr, "\t    AN INPUT DURATION and AN OUTPUT TEMPO.\n");
	fprintf(stderr, "It calculates number of crotchet beats sound will occupy at new tempo.\n\n");
	fprintf(stderr, "INPUT DURATION is input in secs, or as a sndfile (the program finds its dur),\n");
	fprintf(stderr, "	 or as a crotchet count (possibly fractional) WITH a tempo.\n");
	fprintf(stderr, "OUTPUT DURATION can be secs, or can be a crotchet count WITH a tempo.\n\n");
}

/*********************************** DO_FRACTION ******************************/

void do_fraction(double val)
{
	int n, m, k = 0;
	double d, test;
	for(n=2;n<17;n++) {
		d = 1.0/(double)n;
		if((test = fmod(val,d))< FLTERR || d - test < FLTERR) {
			m = (int)(val/d);	/* truncate */
			if(!flteq((double)m * d,val))
				m++;  			
			if(!flteq((double)m * d,val)) {
				m-=2;  			
				if(!flteq((double)m * d,val)) {
				    fprintf(stderr,"Problem in fractions\n");
				    return;
				}
			}			
			if(m > n) {
				k  = m/n;		/* truncate */
				m -= k * n;
			}
			printf("(");
			if(k)
				printf("%d & ",k);
			printf("%d/%d) ",m,n);
			break;
		}
	}
}

/*********************************** FLTEQ ******************************/

int flteq(double a,double b)
{
	double hibound = a + FLTERR;
	double lobound = a - FLTERR;
	if(b > hibound || b < lobound)
		return(0);
	return(1);
}


 /******************************** LOGO() **********************************/

void logo()
{   printf("\t    ***************************************************\n");
    printf("\t    *           COMPOSERS DESKTOP PROJECT             *\n");
    printf("\t                    %s $Revision: 6.0.0 $\n",PROG);
    printf("\t    *                                                 *\n");
    printf("\t    *       Calculate required time-stretches.        *\n");
    printf("\t    *                                                 *\n");
    printf("\t    *                by TREVOR WISHART                *\n");
    printf("\t    ***************************************************\n\n");
}

