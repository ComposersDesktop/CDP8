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



#include <columns.h>

void do_shuffle(int, int);

/**************************** M_REPOS ********************************/

int m_repos(int j)
{
	int n, m, k=cnt, OK, is_set = 0;
	int zib = -ifactor, zab = ifactor;
	for(n=0;n<k;n++) {
		OK = 1;
		if(n>j && !is_set) {
			zib++;
			zab++;
			is_set = 1;
		}
		for(m=zib;m<zab;m++) {
			if(n+m<0)	  continue;
			if(n+m>=cnt)  break;
			if(flteq(fmod(number[j],12.0),fmod(number[n+m],12.0))) {
				OK = 0;
				break;
			}
		}
		if(OK) {
		   do_shuffle(n,j);
		   if(j>n)
			   return(1);	/* list shuffled forward */
		   return(0);		/* list not shuffled forward */
		}
	}
	return(-1);
}	   

/**************************** F_REPOS ********************************/

int f_repos(int j)
{
	int n, m, k=cnt-ifactor, OK, is_set = 0;
	double interval;
	int zib = -ifactor, zab = ifactor;
	for(n=0;n<k;n++) {
		OK = 1;
		if(n>j && !is_set) {
			zib++;
			zab++;
			is_set = 1;
		}
		for(m=zib;m<zab;m++) {
			if(n+m<0)	  continue;
			if(n+m>=cnt)  break;
			interval = log(number[j]/number[n+m])/LOG_2_TO_BASE_E;
			interval = fmod(fabs(interval),1.0);
			if(interval<ONEMAX || interval>ONEMIN) {
				OK = 0;
				break;
			}
		}
		if(OK) {
		   do_shuffle(n,j);
		   if(j>n)
			   return(1);	/* list shuffled forward */
		   return(0);		/* list not shuffled forward */
		}
	}
	return(-1);
}	   

/**************************** DO_SHUFFLE ******************************/

void do_shuffle(int n,int j)
{
	int m;
	double keep = number[j];
	if(j > n) {
		for(m=j;m>n;m--)
		   number[m] = number[m-1];
	} else {
		for(m=j;m<n;m++)
			number[m] = number[m+1];
	}
	number[n] = keep;
}

/************************* CHECK_FOR_CONDITIONAL ************************/

void check_for_conditional(int *argc,char *argv[])
{
	int n;
	char less_than, greater_than;
	int condit_set = 0;

	if(sloom) {
		less_than = '@';
		greater_than = '+';
	} else {
		less_than = '{';
		greater_than = '}';
	}
	for(n=1;n<*argc;n++) {
		if((condit = *argv[n])==less_than || condit==greater_than) {
			if(condit==less_than)
				condit = '<';
			else
				condit = '>';
			argv[n]++;
			if(sscanf(argv[n],"%lf",&thresh)!=1) {
				sprintf(errstr,"Cannot read conditional value.\n");
				do_error();
			}
			while(n<(*argc)-1) {
				*(argv+n) = *(argv+n+1);
				n++;
			}
			(*argc)--;
			condit_set = 1;
		}
	}
	if(!condit_set)
		condit = 0;
}
