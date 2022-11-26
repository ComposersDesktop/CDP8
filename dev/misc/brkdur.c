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



#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#define CALCLIM 		(0.00002)
#define FLTERR	 		(0.00002)
#define ONE_OVER_LN2	(1.442695)
#define BIGARRAY		200
#define ENDOFSTR		('\0')
#define NEWLINE			('\n')

int 	read_brkdata(double **bbrk,FILE *fp,int mode);
void 	get_dur(double *bbrk,int bbrksize,double in_dur,int mode);
double 	cntevents(double dur,double s0,double s1);
int 	sizeq(double f1,double f2);
int  	get_float_from_within_string(char **str,double *val);
double 	round(double a);
int 	flteq(double a,double b);
const char* cdp_version = "7.1.0";

int main(int argc,char * argv [])
{
	double *bbrk, in_dur;
	FILE *fp;
	int bbrksize, mode;

	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(argc!=4) {
		fprintf(stdout,"ERROR: Bad function call.\n");
		fflush(stdout);
		return(0);
	}
	if(sscanf(argv[2],"%d",&mode)!=1) {
		fprintf(stdout,"ERROR: Failed to read mode.\n");
		fflush(stdout);
		return(0);
	}
	if(sscanf(argv[3],"%lf",&in_dur)!=1) {
		fprintf(stdout,"ERROR: Failed to read input file duration.\n");
		fflush(stdout);
		return(0);
	}
	if((fp = fopen(argv[1],"r"))==NULL) {
		fprintf(stdout,"ERROR: Failed to open file %s\n",argv[1]);
		fflush(stdout);
		return(0);
	}
	if((bbrksize = read_brkdata(&bbrk,fp,mode))<=0)
		return(0);
	get_dur(bbrk,bbrksize,in_dur,mode);
	return(1);
}

void get_dur(double *bbrk,int bbrksize,double in_dur,int mode)
{
	int n, m;
	double lasttime, lastval, thistime, thisval, newtime = 0;
	int done;
	double tratio;
	/*RWD need init */
	thistime = 0;
	lasttime = bbrk[0];
	lastval  = bbrk[1];
	if(mode == 1) {
		for(n = 1; n <bbrksize * 2; n+=2)
			bbrk[n] = pow(2.0,bbrk[n]/12.0);
	}
	done = 0;
	for(m=2,n= 1;n<bbrksize;m+=2,n++) {
		thistime = bbrk[m]; 
		thisval  = bbrk[m+1]; 
		if((in_dur > 0.0) && (thistime > in_dur)) {
			done = 1;
			tratio   = (in_dur - lasttime)/(thistime - lasttime);
			if(!flteq(tratio,0.0)) {
				thistime = in_dur;
				thisval  = ((thisval - lastval) * tratio) + lastval;
				newtime += cntevents(thistime - lasttime,lastval,thisval);
			}
		} else {
			newtime += cntevents(thistime - lasttime,lastval,thisval);
		}
		if(done)
			break;
		lasttime = thistime;
		lastval  = thisval;
	}
	if(!done && (thistime < in_dur)) {
		thistime = in_dur;
		thisval  = lastval;
		newtime += cntevents(thistime - lasttime,lastval,thisval);
	}
	fprintf(stdout,"INFO: DURATION is %lf\n",newtime);
	fflush(stdout);
}

/*************************** CNTEVENTS *****************************/

double cntevents(double dur,double s0,double s1)
{   
	double f1 = dur,f2;
	if(sizeq(s1,s0))
		return((f1*2.0)/(s1+s0));
	f2  = s1-s0;
	f1 /= f2;
	f2  = s1/s0;
	f2  = log(f2);
	f1 *= f2;
	return(fabs(f1));
}

/**************************** SIZEQ *******************************/

int sizeq(double f1,double f2)
{   
	double upperbnd, lowerbnd;
	upperbnd = f2 + CALCLIM;
	lowerbnd = f2 - CALCLIM;
	if((f1>upperbnd) || (f1<lowerbnd))
		return(0);
	return(1);
}


/**************************** READ_BRKDATA *******************************/

int read_brkdata(double **bbrk, FILE *fp,int mode)
{
	double *p, *w, zmax, zmin, lasttime = 0;	/*RWD added init */
	char *q,  temp[400];
	int is_time = 1, is_insert = 0,cnt = 0;
	int arraysize = BIGARRAY;
	if(mode == 1) {
		zmax = 120.0;
		zmin = -120.0;
	} else {
		zmax = 1000.0;
		zmin = 0.001;
	}
	if((*bbrk = (double *)malloc(arraysize * sizeof(double)))==NULL) {
		fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store input brktable data.\n");
		fflush(stdout);
		return(0);
	}
	p = *bbrk;
	cnt = 0;
	while(fgets(temp,200,fp)==temp) {	 /* READ AND TEST BRKPNT VALS */
		q = temp;
		while(get_float_from_within_string(&q,p)) {
			if(is_time) {
				if(cnt==0) {
					if(*p < 0.0) {
						fprintf(stdout,"ERROR: First timeval (%lf) in brkpntfile is less than zero.\n",*p);
						fflush(stdout);
						return(0);
					} else if (*p > 0.0) {
						is_insert = 1;
					}
				} else {
					if(is_insert) {	 		/*	Force val at time zero */
						p   += 2;
						cnt += 2;
						w = p;
						*p = *(p-2);
						p--;
						*p = *(p-2);
						p--;
						*p = *(p-2);
						(*bbrk)[0] = 0.0;
						p = w;
						is_insert = 0;
					}
					if(*p <= lasttime) {
						fprintf(stdout,"ERROR: Times (%lf & %lf) in brkpntfile are not in increasing order.\n",*p,lasttime);
						fflush(stdout);
						return(0);
					}
				}
				lasttime = *p;
			} else {
				if(*p < zmin || *p > zmax) {
					fprintf(stdout,"ERROR: Time stretch values are out of range (%lf to %lf)\n",zmin, zmax);
					fflush(stdout);
					return(0);
				}
			}
			is_time = !is_time;
			p++;
			if(++cnt >= arraysize) {
				arraysize += BIGARRAY;
				if((*bbrk = (double *)realloc((char *)(*bbrk),arraysize * sizeof(double)))==NULL) {
					fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to reallocate input brktable data.\n");
					fflush(stdout);
					return(0);
				}
				p = (*bbrk) + cnt;		
			}
		}
	}	    
	if(cnt < 4) {
		fprintf(stdout,"ERROR: Not enough data in brkpnt file.\n");
		fflush(stdout);
		return(0);
	}
	if(cnt&1) {
		fprintf(stdout,"ERROR: Data not paired correctly in brkpntfile.\n");
		fflush(stdout);
		return(0);
	}
	return cnt/2;
}

/************************** GET_FLOAT_FROM_WITHIN_STRING **************************/

int  get_float_from_within_string(char **str,double *val)
{
	char   *p, *valstart;
	int    decimal_point_cnt = 0, has_digits = 0;
	p = *str;
	while(isspace(*p))
		p++;
	valstart = p;	
	switch(*p) {
	case('-'):						break;
	case('.'): decimal_point_cnt=1;	break;
	default:
		if(!isdigit(*p))
			return(0);
		has_digits = 1;
		break;
	}
	p++;		
	while(!isspace(*p) && *p!=NEWLINE && *p!=ENDOFSTR) {
		if(isdigit(*p))
			has_digits = 1;
		else if(*p == '.') {
			if(++decimal_point_cnt>1)
				return(0);
		} else
			return(0);
		p++;
	}
	if(!has_digits || sscanf(valstart,"%lf",val)!=1)
		return(0);
	*str = p;
	return(1);
}

double round(double a)
{
	int b;
	if(a >= 0.0)
		b = (int)floor(a + 0.5);
	else {
		a = -a;
		b = (int)floor(a + 0.5);
		b = -b;
	}
	return b;
}

int flteq(double a,double b)
{
	double hibnd = a + FLTERR;	
	double lobnd = a - FLTERR;
	if(b >= lobnd && b <= hibnd)
		return 1;
	return 0;
}
