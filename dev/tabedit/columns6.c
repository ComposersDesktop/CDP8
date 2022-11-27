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
#include <cdplib.h>
#include <time.h>

static void sort_set(double *set,int setcnt);
static void do_search(double thisval,double error,int **adjusted);
static void adjust_all_vals(double thisval,double gap,int n,int m);

/************************************** COMPRESS_SEQUENCE ****************************************
 *
 * parameter is interval compression multiplier.
 */

void compress_sequence(int multi)
{
	int n;
	int m, hdcnt = ifactor;
	char temp[200];
	double interval, nunote;
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	if(multi) {
		fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[0],number[1],number[2],number[3],number[4]);
		nunote = number[2];
		for(n=5;n<cnt;n+=5) {
			interval = (number[n+2] - number[n-3]) * factor;
			nunote += interval;
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],number[n+1],nunote,number[n+3],number[n+4]);
		}
	} else {
		for(n=0;n<cnt;n+=3)
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[n],number[n+1] * factor,number[n+2]);
	}
	fflush(stdout);
}

/************************************** TRANSPOSE_SEQUENCE ****************************************
 *
 * parameter is transposition in semitones.
 */

void transpose_sequence(int multi)
{
	int n;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	if(multi) {
		for(n=0;n<cnt;n+=5)
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],number[n+1],number[n+2] + factor,number[n+3],number[n+4]);
	} else {
		for(n=0;n<cnt;n+=3)
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[n],number[n+1] + factor,number[n+2]);
	}
	fflush(stdout);
}

/************************************** P_INVERTSET_SEQUENCE *****************************************/

void p_invertset_sequence(int multi)
{
	double  *set;
	int setcnt, n, gotit, q;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	if((set = (double *)malloc(cnt * sizeof(double)))==NULL) {
		fprintf(stdout,"ERROR: Insufficient memory to store pitch set.\n");
		fflush(stdout);
		exit(1);
	}
	setcnt = 0;
	if(multi) {
		for(n=2;n<cnt;n+=5) {
			gotit = 0;
			for(q = 0; q < setcnt; q++) {
				if(flteq(number[n],set[q])) {
					gotit = 1;
					break;
				}
			}
			if(!gotit)
				set[setcnt++] = number[n];
		}
		sort_set(set,setcnt);

		for(n=2;n<cnt;n+=5) {
			for(q=0;q<setcnt;q++) {
				if(flteq(number[n],set[q])) {
					number[n] = set[setcnt - 1 - q];
					break;
				}
			}
		}
		for(n=0;n<cnt;n+=5) 
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],number[n+1],number[n+2],number[n+3],number[n+4]);
	} else {
		for(n=1;n<cnt;n+=3) {
			gotit = 0;
			for(q = 0; q < setcnt; q++) {
				if(flteq(number[n],set[q])) {
					gotit = 1;
					break;
				}
			}
			if(!gotit)
				set[setcnt++] = number[n];
		}
		sort_set(set,setcnt);

		for(n=1;n<cnt;n+=3) {
			for(q=0;q<setcnt;q++) {
				if(flteq(number[n],set[q])) {
					number[n] = set[setcnt - 1 - q];
					break;
				}
			}
		}
		for(n=0;n<cnt;n+=3) 
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[n],number[n+1],number[n+2]);
	}
	fflush(stdout);
}

/************************************** P_EXPANDSET_SEQUENCE *****************************************
 *
 * parameter is set expansion multiplier.
 */

void p_expandset_sequence(int multi)
{
	double  *set=NULL, transpos;
	int setcnt, n, gotit, q, qn, qq, qoct;
	int m, hdcnt = ifactor;
	int lastsetpos=0, lastnusetpos=0, thissetpos, setstep;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	if(factor <= 1.0) {
		fprintf(stdout,"ERROR: Set expansion must be > 1\n");
		fflush(stdout);
		exit(1);
	}
	ifactor = (int)round(factor + 0.5);
	if((set = (double *)malloc((cnt * ifactor) * sizeof(double)))==NULL) {
		fprintf(stdout,"ERROR: Insufficient memory to store pitch set.\n");
		fflush(stdout);
		exit(1);
	}
	setcnt = 0;
	if(multi) {
		for(n=2;n<cnt;n+=5) {
			gotit = 0;
			for(q = 0; q < setcnt; q++) {
				if(flteq(number[n],set[q])) {
					gotit = 1;
					break;
				}
			}
			if(!gotit)
				set[setcnt++] = number[n];
		}
		sort_set(set,setcnt);
		fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[0],number[1],number[2],number[3],number[4]);
		for(q=0;q<setcnt;q++) {
			if(flteq(number[2],set[q])) {				/* find which set-member this is */
				lastsetpos = q;
				lastnusetpos = q;
				break;
			}
		}
		for(n=7;n<cnt;n+=5) {
			for(q=0;q<setcnt;q++) {
				if(flteq(number[n],set[q])) {				/* find which set-member this is */
					thissetpos = q;
					setstep = thissetpos - lastsetpos;
					setstep = (int)round(setstep * factor);
					qn = lastnusetpos + setstep;
					lastsetpos = thissetpos;
					lastnusetpos = qn;
					if(qn >= setcnt) {						/* if beyond existing set */			
						qq = qn%setcnt;						/* cyclically find appropriate set member */
						qoct = qn/setcnt;
						transpos = 12.0 * qoct;
						number[n] = set[qq] + transpos;
					} else if(qn < 0) {						/* if below existing set */			
						qoct = 0;							/* simil */
						while(qn < 0) {
							qn += setcnt;
							qoct++;
						}
						transpos = -(12.0 * qoct);
						number[n] = set[qn] + transpos;
					} else
						number[n] = set[qn];
					break;
				}
			}
		}
		for(n=5;n<cnt;n+=5) 
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],number[n+1],number[n+2],number[n+3],number[n+4]);
	} else {
		for(n=1;n<cnt;n+=3) {
			gotit = 0;
			for(q = 0; q < setcnt; q++) {
				if(flteq(number[n],set[q])) {
					gotit = 1;
					break;
				}
			}
			if(!gotit)
				set[setcnt++] = number[n];
		}
		sort_set(set,setcnt);
		for(n=1;n<cnt;n+=3) {
			for(q=0;q<setcnt;q++) {
				if(flteq(number[n],set[q])) {				/* find which set-member this is */
					qn = (int)round((q+1) * factor) - 1;	/* expand set-position */
					if(qn >= setcnt) {						/* if beyond existing set */			
						qq = qn%setcnt;						/* cyclically find appropriate set member */
						qoct = qn/setcnt;
						transpos = 12.0 * qoct;
						number[n] = set[qq] + transpos;
					} else
						number[n] = set[qn];
					break;
				}
			}
		}
		for(n=0;n<cnt;n+=3) 
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[n],number[n+1],number[n+2]);
	}
	fflush(stdout);
}

/************************************** P_INVERT_SEQUENCE *****************************************/

void p_invert_sequence(int multi)
{
	int n;
	double adjust;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}

	if(multi) {
		adjust = 2 * number[2];
		for(n=0;n<cnt;n+=5)
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],number[n+1],adjust - number[n+2],number[n+3],number[n+4]);
	} else {
		adjust = 2 * number[1];
		for(n=0;n<cnt;n+=3) 
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[n],adjust - number[n+1],number[n+2]);
	}
	fflush(stdout);
}

/************************************** T_REVERSE_SEQUENCE ****************************************/

void t_reverse_sequence(int multi)		
{
	double totaldur;
	int tend, n, te;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	totaldur = 0.0;
	tend = cnt - 4;

	if(multi) {
		for(n=0,te = tend;n<cnt;n+=5, te-=5) {
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],totaldur,number[n+2],number[n+3],number[n+4]);
			totaldur += number[te] - number[te - 5];
		}
	} else {
		for(n=0,te = tend;n<cnt;n+=3, te-=3) {
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",totaldur,number[n+1],number[n+2]);
			totaldur += number[te] - number[te - 3];
		}
	}
	fflush(stdout);
}

/************************************** P_REVERSE_SEQUENCE ****************************************/

void p_reverse_sequence(int multi)		
{
	int n, pe;
	int pend;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	pend = cnt-3;
	if(multi) {
		for(n=0, pe = pend;n<cnt;n+=5, pe -= 5)
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],number[n+1],number[pe],number[n+3],number[n+4]);
	} else {
		for(n=0, pe = pend;n<cnt;n+=3, pe -= 3)
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[n],number[pe],number[n+2]);
	}
	fflush(stdout);
}

/************************************** A_REVERSE_SEQUENCE ****************************************/

void a_reverse_sequence(int multi)		
{
	int n, ae;
	int m, hdcnt = ifactor;
	int aend = cnt - 2;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	if(multi) {
		for(n=0, ae = aend;n<cnt;n+=5, ae -= 5)
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],number[n+1],number[n+2],number[ae],number[n+4]);
	} else {
		for(n=0, ae = cnt-1;n<cnt;n+=3, ae -= 3)
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[n],number[n+1],number[ae]);
	}
	fflush(stdout);
}

/************************************** PA_REVERSE_SEQUENCE ***************************************/

void pa_reverse_sequence(int multi)		
{
	int n, pe;
	int pend;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	pend = cnt-3;
	if(multi) {
		for(n=0, pe = pend;n<cnt;n+=5, pe -= 5)
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],number[n+1],number[pe],number[pe+1],number[n+4]);
	} else {
		for(n=0, pe = pend;n<cnt;n+=3, pe -= 3)
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[n],number[pe],number[pe+1]);
	}
	fflush(stdout);
}

/************************************** TP_REVERSE_SEQUENCE ***************************************/

void tp_reverse_sequence(int multi)		
{
	double totaldur;
	int tend, n, te;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	totaldur = 0.0;
	tend = cnt-4;
	if(multi) {
		for(n=0,te = tend;n<cnt;n+=5, te-=5) {
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],totaldur,number[te+1],number[n+3],number[n+4]);
			totaldur += number[te] - number[te - 5];
		}
	} else {
		for(n=0,te = tend;n<cnt;n+=3, te-=3) {
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",totaldur,number[te+1],number[n+2]);
			totaldur += number[te] - number[te - 3];
		}
	}
	fflush(stdout);
}

/************************************** TA_REVERSE_SEQUENCE ***************************************/

void ta_reverse_sequence(int multi)		
{
	double totaldur;
	int tend, n, te;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	totaldur = 0.0;
	tend = cnt-4;
	if(multi) {
		for(n=0,te = tend;n<cnt;n+=5, te-=5) {
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],totaldur,number[n+2],number[te+2],number[n+4]);
			totaldur += number[te] - number[te - 5];
		}
	} else {
		for(n=0,te = tend;n<cnt;n+=3, te-=3) {
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",totaldur,number[n+1],number[te+2]);
			totaldur += number[te] - number[te - 3];
		}
	}
	fflush(stdout);
}

/************************************** TPA_REVERSE_SEQUENCE **************************************/

void tpa_reverse_sequence(int multi)		
{
	double totaldur;
	int tend, te, n;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	totaldur = 0.0;
	tend = cnt-4;
	if(multi) {
		for(n = 0,te = tend;te >= 0;te-=5,n+=5) {
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],totaldur,number[te+1],number[te+2],number[n+4]);
			totaldur += number[te] - number[te - 5];
		}
	} else {
		for(te = tend;te >= 0;te-=3) {
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",totaldur,number[te+1],number[te+2]);
			totaldur += number[te] - number[te - 3];
		}
	}
	fflush(stdout);
}

/************************************** LOOP_SEQUENCE **************************************
 *
 * params are loopcnt and lastdur.
 */

void loop_sequence(int multi)		
{
	int n, m, loopcnt = (int)factor;
	double lastdur = thresh, totaldur, basetime;
	int hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
		totaldur = number[cnt-4] + lastdur;
	} else {
		totaldur = number[cnt-3] + lastdur;
	}
	if(lastdur <= 0.0) {
		fprintf(stdout,"ERROR: final event duration is <= zero\n");
		fflush(stdout);
		exit(1);
	}
	if(loopcnt < 2) {
		fprintf(stdout,"ERROR: Loopcnt is <= 1\n");
		fflush(stdout);
		exit(1);
	}
	basetime = 0.0;
	for(n = 0; n<loopcnt; n++) {
		if(multi) {
			for(m=0;m < cnt; m+=5)
				fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[m],number[m+1] + basetime,number[m+2],number[m+3],number[m+4]);
		} else {
			for(m=0;m < cnt; m+=3)
				fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[m] + basetime,number[m+1],number[m+2]);
		}
		basetime += totaldur;
	}
	fflush(stdout);
}

/************************************** ABUT_SEQUENCES **************************************
 *
 * Read two tables, as strings, like 'jj'
 */

void abut_sequences(int multi)
{
	int n, m, hdcnt;
	double lasttime, time;
	char temp[264];
	if(factor <= 0.0) {
		fprintf(stdout,"ERROR: final event duration is less than or equal to zero\n");
		fflush(stdout);
		exit(1);
	}
	if(multi) {
		hdcnt = ifactor;
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
 		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
		firstcnt -= hdcnt;

		for(n = 0; n < firstcnt; n+= 5)
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],number[n+1],number[n+2],number[n+3],number[n+4]);
		lasttime = number[n-4];
		lasttime += factor;
		for(n = firstcnt + hdcnt; n < cnt; n+= 5) {
			time = number[n+1] + lasttime;
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],time,number[n+2],number[n+3],number[n+4]);
		}
	} else {
		for(n = 0; n < cnt; n+= 3)
			fprintf(stdout,"INFO: %s  %s  %s\n",strings[n],strings[n+1],strings[n+2]);
		lasttime = atof(strings[n-3]);
		lasttime += factor;
		for(n = cnt; n < stringscnt; n+= 3) {
			time = atof(strings[n]) + lasttime;
			sprintf(temp,"%lf",time);
			fprintf(stdout,"INFO: %s  %s  %s\n",temp,strings[n+1],strings[n+2]);
		}
	}
	fflush(stdout);
}

/************************************** SORT_SET **************************************/

void sort_set(double *set,int setcnt)
{
	int n, m;
	double temp;
	for(n=0;n<setcnt-1;n++) {
		for(m = n; m<setcnt; m++) {
			if(set[m] < set[n]) {
				temp = set[n];
				set[n] = set[m];
				set[m] = temp;
			}
		}
	}
}

/************************************** uptempo_sequence ****************************************
 *
 * parameter is tempo multiplier.
 */

void uptempo_sequence(int multi)		
{
	int n;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
	}
	if(multi) {
		for(n=0;n<cnt;n+=5)
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],number[n+1] * factor,number[n+2],number[n+3],number[n+4]);
	} else {
		for(n=0;n<cnt;n+=3)
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[n] * factor,number[n+1],number[n+2]);
	}
	fflush(stdout);
}

/************************************** ACCEL_SEQUENCE ****************************************
 *
 * parameter is multiplier of final event-time, thresh is curve of accel.
 */

void accel_sequence(int multi)		
{
	int n;
	double accel_step = (1.0/factor) - 1.0, lasttime;
	double convertor, frac, accel, time, dur;
	int m, hdcnt = ifactor;
	char temp[200];
	if(multi) {
		sprintf(errstr,"%lf",number[0]);
		for(n=1;n< hdcnt;n++) {lasttime = number[cnt-3];
			sprintf(temp,"  %lf",number[n]);
			strcat(errstr,temp);
		}
		strcat(errstr,"\n");
		fprintf(stdout,"INFO: %s\n",errstr);
		for(n = 0,m = hdcnt;m < cnt;n++, m++)
			number[n] = number[m];
		cnt -= hdcnt;
		lasttime = number[cnt-4];
	} else {
		lasttime = number[cnt-3];
	}
	convertor = 1.0/lasttime;
	if(multi) {
		for(n=0;n<cnt;n+=5) {
			frac = pow(number[n+1] * convertor,thresh);
			accel = 1.0 + (accel_step * frac);
			time = number[n+1] * accel;
			dur = number[n+4] * accel;
			fprintf(stdout,"INFO: %d  %lf  %lf  %lf  %lf\n",(int)number[n],time,number[n+2],number[n+3],dur);
		}
	} else {
		for(n=0;n<cnt;n+=3) {
			frac = pow(number[n] * convertor,thresh);
			accel = 1.0 + (accel_step * frac);
			time = number[n] * accel;
			fprintf(stdout,"INFO: %lf  %lf  %lf\n",time,number[n+1],number[n+2]);
		}
	}
	fflush(stdout);
}

/************************************** MEAN_TEMPO *****************************************/

void mean_tempo()		
{
	factor = (double)(60 * (cnt-1))/(number[cnt-1] - number[0]);
	factor = (round(factor * 100.0))/100.0;
	fprintf(stdout,"WARNING: Mean Tempo is %.2lf (assuming times are approx evenly spaced)\n",factor);
	fflush(stdout);
}

/************************************** TIME_TO_CROTCHETS *****************************************/

void time_to_crotchets(int beatvals) {
	double crotchet = factor;
	double semibrev = 4 * crotchet;
	double minim = 2 * crotchet;
	double quaver = crotchet / 2;	
	double semiquav = crotchet / 4;
	double tripquav = crotchet / 3;
	double tripsemiquav = crotchet / 6;
	double demisemiquav = crotchet / 8;
	double trip	= minim / 3;
	int n, m;
	int **adjusted;
	if ((adjusted = (int **)malloc(cnt * sizeof(int *)))==NULL) {
		fprintf(stdout,"ERROR: Insufficient memory\n");
		fflush(stdout);
		exit(1);
	}
	for(n=0;n<cnt;n++) {
		if ((adjusted[n] = (int *)malloc(cnt * sizeof(int)))==NULL) {
			fprintf(stdout,"ERROR: Insufficient memory\n");
			fflush(stdout);
			exit(1);
		}
		for(m=0;m<cnt;m++)
			adjusted[n][m] = 0;
	}
	do_search(semibrev,tripsemiquav,adjusted);
	do_search(minim,tripsemiquav,adjusted);
	do_search(crotchet,tripsemiquav,adjusted);
	do_search(tripquav,tripsemiquav/2,adjusted);
	do_search(trip,tripsemiquav/2,adjusted);
	do_search(quaver,demisemiquav,adjusted);
	do_search(semiquav,tripsemiquav/2,adjusted);

	if(beatvals) {
		for(n=1;n<cnt; n++)
			number[n-1] = number[n] - number[n-1];
		cnt--;
	}
	for(n=0;n<cnt;n++) {
		number[n] /= crotchet;
		number[n] = (round(number[n] * 1000))/1000.0;
	}
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

void do_search(double thisval,double error,int **adjusted) {
	int n, m;
	double up = thisval + error;
	double dn = thisval - error;
	double gap;
	for(n=0; n<cnt-1; n++) {
		for(m=n+1; m<cnt; m++) {
			if(adjusted[n][m])
				continue;
			gap = number[m] - number[n];
			if(gap > dn) {
				if(gap < up) {
					adjust_all_vals(thisval,gap,n,m);
					adjusted[n][m] = 1;
				} else
					break;
			}
		}
	}
}

void adjust_all_vals(double thisval,double gap,int n,int m) {
	
	int k;
	double thisgap;
	double adjust_within = thisval/gap;
	double discrep_beyond = thisval - gap;
	number[m] = number[n] + thisval;
	for(k = m+1; k < cnt; k++)
		number[k] += discrep_beyond;
	for(k = n+1; k < m; k++) {
		thisgap = number[k] - number[n];
		number[k] = number[n] + (thisgap * adjust_within);
	}
}

/************************************** ROTATE_LIST *****************************************/

void rotate_list(int reversed)
{
	int n;
	if(reversed) {
		for(n=1;n < stringscnt; n++)
			fprintf(stdout,"INFO: %s\n",strings[n]);
		fprintf(stdout,"INFO: %s\n",strings[0]);
	} else {
		fprintf(stdout,"INFO: %s\n",strings[stringscnt - 1]);
		for(n=0;n < stringscnt-1; n++)
			fprintf(stdout,"INFO: %s\n",strings[n]);
	}
	fflush(stdout);
}	

/************************************** SPLICE_POS *****************************************/

#define SHSECSIZE 256

void splice_pos(void)
{
	double splicelen = (number[cnt]+ .5)/1000.0;
	int srate  = round(number[cnt+1]);
	int chans  = round(number[cnt+2]);
	int chcnt  = round(number[cnt+3]);
	int splen = round(splicelen * srate) * chans;
	int seccnt, k1, k2;
	int n;
	if(((seccnt = splen / SHSECSIZE) * SHSECSIZE) < splen)
		seccnt++;
	splen = seccnt * SHSECSIZE;
	splen /= chcnt;
	for(n=0;n<cnt;n++) {
		if((k1 = round(number[n]) - splen) < 0) {
			fprintf(stdout,"ERROR: Splice falls before zero.\n");
			exit(1);
		}
	}
	for(n=0;n<cnt;n++) {
		k1 = round(number[n]) - splen;
		k2 = round(number[n]) + splen;
		fprintf(stdout,"INFO: %d  %d\n",k1,k2);
	}
	fflush(stdout);
}

/************************************** WARP_TIMES *****************************************/

void time_warp()
{
	int m, n;
	double thistime, nexttime, lasttime = 0.0, thisval, nextval, step, frac, val, sum, gap;
	m = firstcnt;
	for(n=0;n<firstcnt;n++) {
		if(n==0) {
			if(!(flteq(number[n],0.0))) {
				fprintf(stdout,"ERROR: Invalid 1st value %lf in list of times (must be ZERO)\n",number[n]);
				exit(1);
			}
			number[n] = 0.0;
			lasttime = number[n];
		} else if(number[n] <= lasttime) {
			fprintf(stdout,"ERROR: Times, in list of times, must be in increasing order\n");
			exit(1);
		}
	}	
	for(m=firstcnt+1;m<cnt;m+=2) {
		if(number[m] < FLTERR) {
			fprintf(stdout,"ERROR: Invalid value %lf in warping file (must be > 0)\n",number[m]);
			exit(1);
		}
	}
	fprintf(stdout,"INFO: %lf\n",0.0);
	sum = 0.0;
	m = firstcnt;
	for(n=1;n<firstcnt;n++) {
		while(number[m] <= number[n]) {
			if((m += 2) >= cnt)
				break;
		}
		if(m < cnt) {
			nexttime = number[m];
			nextval  = number[m+1];
			thistime = number[m-2];
			thisval  = number[m-1];
			step = nexttime - thistime;
			frac = (number[n] - thistime)/step;
			val = ((nextval - thisval) * frac) + thisval;
		} else {
			val = number[cnt-1];
		}
		gap = number[n] - number[n-1];
		val *= gap;
		sum += val;
		fprintf(stdout,"INFO: %lf\n",sum);
	}
	fflush(stdout);
}

/************************************** LIST_WARP *****************************************/

void list_warp()
{
	int m, n;
	int sizz = firstcnt-1;
	double index, thistime, nexttime, lasttime, thisval, nextval, step, frac, val;
	m = firstcnt;

	if(!flteq(number[firstcnt],0.0)) {
		fprintf(stdout,"ERROR: First time in warping file must be ZERO\n");
		exit(1);
	}
	number[firstcnt] = 0.0;
	lasttime = number[cnt-2];
	for(m=firstcnt;m<cnt;m+=2)
		number[m] /= lasttime;
	m = firstcnt;
	for(n=0;n<firstcnt;n++) {
		index = (double)n/(double)sizz;
		while(number[m] <= index) {
			if((m += 2) >= cnt)
				break;
		}
		if(m < cnt) {
			nexttime = number[m];
			nextval  = number[m+1];
			thistime = number[m-2];
			thisval  = number[m-1];
			step = nexttime - thistime;
			frac = (index - thistime)/step;
			val = ((nextval - thisval) * frac) + thisval;
		} else
			val = number[cnt-1];

		fprintf(stdout,"INFO: %lf\n",number[n] * val);
	}
	fflush(stdout);
}

/************************************** BRKWARP_TIMES *****************************************/

void brktime_warp()
{
	int m, n;
	double thistime, nexttime, thisval, nextval, step, frac, val, gap, sum;

	for(m=firstcnt+1;m<cnt;m+=2) {
		if(number[m] < FLTERR) {
			fprintf(stdout,"ERROR: Invalid value %lf in warping file (must be > 0)\n",number[m]);
			exit(1);
		}
	}
	if(!flteq(number[0],0.0)) {
		fprintf(stdout,"ERROR: Breakpoint file to BE warped must begin at time ZERO\n");
		exit(1);
	}
	number[0] = 0.0;
	fprintf(stdout,"INFO: %lf  %lf\n",number[0], number[1]);
	sum = 0.0;
	m = firstcnt;
	for(n=2;n<firstcnt;n+=2) {
		while(number[m] <= number[n]) {
			if((m += 2) >= cnt)
				break;
		}
		if(m < cnt) {
			nexttime = number[m];
			nextval  = number[m+1];
			thistime = number[m-2];
			thisval  = number[m-1];
			step = nexttime - thistime;
			frac = (number[n] - thistime)/step;
			val = ((nextval - thisval) * frac) + thisval;
		} else {
			val = number[cnt-1];
		}
		gap = number[n] - number[n-2];
		val *= gap;
		sum += val;
		fprintf(stdout,"INFO: %lf  %lf\n",sum, number[n+1]);
	}
	fflush(stdout);
}

/************************************** SEQWARP_TIMES *****************************************/

void seqtime_warp()
{
	int m, n, OK = 1;
	double thistime, nexttime, thisval, nextval, step, frac, val, gap, sum, lasttime;
	if(((firstcnt/3) * 3) != firstcnt)
		OK = 0;
	else if(!flteq(number[0],0.0)) 
		OK = 0;
	else {
		number[0] = 0.0;
		lasttime = number[0];
		for(n=3;n<firstcnt;n+=3) {
			if(number[n] <= lasttime) {
				OK = 0;
				break;
			}
			lasttime = number[n];
		}
	}
	if(!OK) {
		fprintf(stdout,"ERROR: First file is not a valid sequnce file.\n");
		exit(1);
	}
	m = firstcnt;
	for(m=firstcnt+1;m<cnt;m+=2) {
		if(number[m] < FLTERR) {
			fprintf(stdout,"ERROR: Invalid value %lf in warping file (must be > 0)\n",number[m]);
			exit(1);
		}
	}
	if(!flteq(number[0],0.0)) {
		fprintf(stdout,"ERROR: Breakpoint file to BE warped must begin at time ZERO\n");
		exit(1);
	}
	number[0] = 0.0;
	fprintf(stdout,"INFO: %lf  %lf  %lf\n",number[0],number[1],number[2]);
	sum = 0.0;
	m = firstcnt;
	for(n=3;n<firstcnt;n+=3) {
		while(number[m] <= number[n]) {
			if((m += 2) >= cnt)
				break;
		}
		if(m < cnt) {
			nexttime = number[m];
			nextval  = number[m+1];
			thistime = number[m-2];
			thisval  = number[m-1];
			step = nexttime - thistime;
			frac = (number[n] - thistime)/step;
			val = ((nextval - thisval) * frac) + thisval;
		} else {
			val = number[cnt-1];
		}
		gap = number[n] - number[n-3];
		val *= gap;
		sum += val;
		fprintf(stdout,"INFO: %lf  %lf  %lf\n",sum, number[n+1], number[n+2]);
	}
	fflush(stdout);
}

/************************************** BRKVAL_WARP *****************************************/

void brkval_warp()
{
	int m, n;
	double thistime, nexttime, thisval, nextval, step, frac, val;
	if(!flteq(number[0],0.0)) {
		fprintf(stdout,"ERROR: Breakpoint file to BE warped must begin at time ZERO\n");
		exit(1);
	}
	if(!flteq(number[firstcnt],0.0)) {
		fprintf(stdout,"ERROR: Warping file must begin at time ZERO\n");
		exit(1);
	}
	m = firstcnt;
	for(n=2;n<firstcnt;n+=2) {
		while(number[m] <= number[n]) {
			if((m += 2) >= cnt)
				break;
		}
		if(m < cnt) {
			nexttime = number[m];
			nextval  = number[m+1];
			thistime = number[m-2];
			thisval  = number[m-1];
			step = nexttime - thistime;
			frac = (number[n] - thistime)/step;
			val = ((nextval - thisval) * frac) + thisval;
		} else
			val = number[cnt-1];

		fprintf(stdout,"INFO: %lf  %lf\n",number[n], number[n+1] * val);
	}
	fflush(stdout);
}

/************************************** DUPLICATE_LIST_AT_STEP *****************************************/

void duplicate_list_at_step()
{
	int n, m;
	double base = 0.0;
	double step = number[cnt+1];
	ifactor = round(number[cnt]);
	step = number[cnt-1] - number[0] + step;
	for(m=0;m<ifactor;m++) {
		for(n=0;n<cnt;n++)
			fprintf(stdout,"INFO: %lf\n",number[n] + base);
		base += step;
	}
	fflush(stdout);
}

/************************************ TW_PSEUDO_EXP ************************************/

void tw_pseudo_exp()
{
	double bottime = number[0];
	double toptime = number[1];
	double botval  = number[2];
	double topval  = number[3];
	double timefrac = 0.5, valfrac  = 0.333333333333, step;
	int valbase = ifactor+1;
	int div, n, index;
	int mask = 1, span;
	cnt = ifactor;
	if(topval < botval)
		valfrac *= 2.0;
	while(mask < (cnt>>1)) {
		if(cnt & mask) {
			fprintf(stdout,"ERROR: Number of steps must be a multiple of 2.\n");
			fflush(stdout);
			exit(1);
		}
		mask <<= 1;
	}	
	free(number);
	if((number = (double *)malloc((cnt+1) * 2 * sizeof(double)))==NULL) {
		fprintf(stdout,"ERROR: Insufficient memory to store interpolated values.\n");
		fflush(stdout);
		exit(1);
	}
	number[0]	= bottime;	
	number[cnt]	= toptime;	
	number[cnt+1]	  = botval;	
	number[(cnt*2)+1] = topval;	

	div = 2;
	span = cnt/2;
	while(div <= cnt) {
		n = 1;
		while(n < div) {
			index = (cnt * n)/div;						/* ODD denominators, factor of 2 numerators */
			step = number[index+span] - number[index-span];	/* i.e. 1/2 : 1/4,3/4 : 1/8,3/8,5/8,7/8 etc */
			step *= timefrac;
			number[index] = number[index-span] + step;
			step = number[valbase+index+span] - number[valbase+index-span];
			step *= valfrac;
			number[valbase+index] = number[valbase+index-span] + step;
			n += 2;
		}
		div *= 2;
		span /= 2;
	}
	for(n=0; n<= cnt;n++)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[valbase+n]); 
	fflush(stdout);
}

/****************************** REVERSE_TIME_INTERVALS2 ******************************/

void reverse_time_intervals2(void)
{
	int n;
	double k = number[0];
	fprintf(stdout,"INFO: %lf\n",k);
	for(n = cnt-1;n>=1;n--) {
		k += (number[n] - number[n-1]);
		fprintf(stdout,"INFO: %lf\n",k);
	}
	fflush(stdout);
}

/****************************** INTERP_N_VALS ******************************/

void interp_n_vals(void)
{
	int n, m, subcnt = cnt - 1;
	double k, difdiv, diff;
	if(ifactor < 1) {
		fprintf(stdout,"ERROR: Invalid number (%d) of interpolated values\n",ifactor);
		fflush(stdout);
		exit(1);
	}
	ifactor++;
	difdiv = (double)ifactor;
	for(n = 0;n< subcnt;n++) {
		k = number[n];
		diff = (number[n+1] - k)/difdiv;
		for(m=0;m<ifactor;m++) {
			fprintf(stdout,"INFO: %lf\n",k);
			k += diff;
		}
	}
	fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

/************************************** GENERAL *****************************************/

#ifdef NOTDEF
/*RWD May 2005 lets keepo to one definition in sfsys */
void
initrand48()
{
	srand(time((time_t *)0));
}

double
drand48()
{
	return (double)rand()/(double)RAND_MAX;
}
#endif

/**************************** WARPED_TIMES *****************************/

void warped_times(int isdiff)
{
	int j, k, m, n, lastn, numcnt = cnt - firstcnt, itemp;
	double endtime, endval, frac, val = 0.0, thistime, timediff=0.0, valdiff=0.0, sum, temp;
	double timestep = (double)128/48000.0;
	/* This is default window length for standard PVOC analysis under CDP, at srate 48000 (windows 1024 samps: decimation 8)*/
	/* However, the timestep, so long as sufficiently small, is not critical, except for very long time stretches */
	double *vals = (double *)exmalloc(numcnt*sizeof(double));
	int *perm = (int *)exmalloc(numcnt*sizeof(int));
	int *perm2 = (int *)exmalloc(numcnt*sizeof(int));
	for(n = 0;n < numcnt;n++)
		perm[n] = n;
	for(k = 0,n = firstcnt; n < cnt-1; n++,k++) {	/* SORT NUMBERS TO ASCending ORDER, AND NOTE PERMUTATION OF POSITIONS */
		m = n+1;
		j = k+1;
		while(m  < cnt) {
			if(number[m] < number[n]) {
				temp = number[m];
				number[m] = number[n];
				number[n] = temp;
				itemp = perm[j];
				perm[j] = perm[k];
				perm[k] = itemp;
			}
			m++;
			j++;
		}
	}
	for(n=0;n<cnt;n++) {			/* 'INVERT' THE POSITION PERM */
		for(m=0;m<cnt;m++) {
			if(perm[m] == n) {
				perm2[n] = m;
				break;
			}
		}
	}
	perm = perm2;
	endtime = number[firstcnt-2];
	endval  = number[firstcnt-1];
	thistime = 0.0;
	sum = 0.0;
	n = 0;
	k = 0;
	for(m=firstcnt;m<cnt;m++) {
		while(thistime < number[m]) {
			if(thistime >= endtime) {
				val = endval;
			} else if(thistime <= number[0]) {
				val = number[1];
			} else {
				lastn = n;
				while(thistime > number[n])
					n += 2;
				if(n != lastn) {
					timediff = (number[n] - number[n-2]);
					valdiff  = (number[n+1] - number[n-1]);
				}
				frac = (thistime - number[n-2])/timediff;
				val =  (valdiff * frac) + number[n-1];
			}
			sum += val;
			thistime += timestep;
		}
		val = sum * timestep;
		if(isdiff)
			vals[k++] = val - number[m];
		else
			vals[k++] = val;
	}
	for(n=0;n<numcnt;n++)
		fprintf(stdout,"INFO: %lf\n",vals[perm[n]]);

	fflush(stdout);
}

/**************************** CUMULADD *****************************/

void cumuladd(void)
{
	int n;
	double upstep = number[cnt];
	fprintf(stdout,"INFO: %lf\n",number[0]);
	for(n=1;n<cnt;n++) {
		fprintf(stdout,"INFO: %lf\n",number[n] + upstep);
		upstep += number[cnt];
	}
	fflush(stdout);
}

/**************************** TW_PSUEDOPAN *****************************/

void tw_psuedopan(void)
{
	double inner_width = number[0], outer_width = number[1];
	double startpos    = number[2], time_propor = number[3];
	double bakfrth_dur = number[4], duration    = number[5];
	int leftwards = round(number[6]);
	double time = 0;
	double *vals;
	double large_tstep, small_tstep;
	double  left_pos, far_left_pos, right_pos, far_right_pos;
	double  dist_from_left, prop_dist;
	double nexttimestep = -1.0;
	int n, dostop = 0, cycle_point, valcnt = (int)floor(duration/bakfrth_dur) + 3;
	valcnt *= 12;
	if((vals = (double *)malloc(valcnt * sizeof(double)))==NULL) {
		fprintf(stdout,"ERROR: Insufficient memory to store pan data.\n");
		fflush(stdout);
		exit(1);
	}
	small_tstep = bakfrth_dur * time_propor;
	large_tstep = bakfrth_dur - small_tstep;
	small_tstep /= 4.0;	
	large_tstep /= 2.0;
	left_pos = -inner_width;
	far_left_pos = -outer_width;
	right_pos = inner_width;
	far_right_pos = outer_width;
	if(flteq(startpos,far_left_pos)) {
		nexttimestep = small_tstep;
		cycle_point = 0;
	} else if(flteq(startpos,left_pos)) {
		if(leftwards == 0) {
			nexttimestep = large_tstep;
			cycle_point = 1;
		} else {
			nexttimestep = small_tstep;
			cycle_point = 5;
		}
	} else if(flteq(startpos,right_pos)) {
		if(leftwards == 0) {
			nexttimestep = small_tstep;
			cycle_point = 2;
		} else {
			nexttimestep = large_tstep;
			cycle_point = 4;
		}
	} else if(flteq(startpos,far_right_pos)) {
			nexttimestep = small_tstep;
			cycle_point = 3;
	} else if(startpos < left_pos) {
		dist_from_left = startpos - far_left_pos;
		prop_dist = dist_from_left/(left_pos - far_left_pos);
		if(leftwards == 0) {
			cycle_point = 0;
			nexttimestep = small_tstep * (1.0 - prop_dist);
		} else {
			cycle_point = 5;
			nexttimestep = small_tstep * prop_dist;
		}
	} else if(startpos < right_pos) {
		dist_from_left = startpos -  left_pos;
		prop_dist = dist_from_left/(right_pos - left_pos);
		if(leftwards == 0) {
			cycle_point = 1;
			nexttimestep = large_tstep * (1.0 - prop_dist);
		} else {
			cycle_point = 4;
			nexttimestep = large_tstep * prop_dist;
		}
	} else {
		dist_from_left = startpos -  right_pos;
		prop_dist = dist_from_left/(far_right_pos - right_pos);
		if(leftwards == 0) {
			cycle_point = 2;
			nexttimestep = small_tstep * (1.0 - prop_dist);
		} else {
			cycle_point = 3;
			nexttimestep = small_tstep * prop_dist;
		}
	}

	n = 0;
	if(n + 12 >= valcnt) {
		fprintf(stdout,"ERROR: Internal error calculating memory required.\n");
		fflush(stdout);
		exit(1);
	}
	switch(cycle_point) {
	case(0):
		vals[n++] = time;
		vals[n++] = startpos;
		time += nexttimestep;
		nexttimestep = -1.0;
		if(time >= duration)
			dostop = 1;
		/* fall thro */
	case(1):
		vals[n++] = time;
		if(dostop) {
			dostop++;
			break;
		}
		if(nexttimestep > 0.0) {
			vals[n++] = startpos;
			time += nexttimestep;
			nexttimestep = -1.0;
		} else {
			vals[n++] = left_pos;
			time += large_tstep;
		}
		if(time >= duration)
			dostop = 1;
		/* fall thro */
	case(2):
		vals[n++] = time;
		if(dostop) {
			dostop++;
			break;
		}
		if(nexttimestep > 0.0) {
			vals[n++] = startpos;
			time += nexttimestep;
			nexttimestep = -1.0;
		} else {
			vals[n++] = right_pos;
			time += small_tstep;
		}
		if(time >= duration)
			dostop = 1;
		/* fall thro */
	case(3):
		vals[n++] = time;
		if(dostop) {
			dostop++;
			break;
		}
		if(nexttimestep > 0.0) {
			vals[n++] = startpos;
			time += nexttimestep;
			nexttimestep = -1.0;
		} else {
			vals[n++] = far_right_pos;
			time += small_tstep;
		}
		if(time >= duration)
			dostop = 1;
		/* fall thro */
	case(4):
		vals[n++] = time;
		if(dostop) {
			dostop++;
			break;
		}
		if(nexttimestep > 0.0) {
			vals[n++] = startpos;
			time+= nexttimestep;
			nexttimestep = -1.0;
		} else {
			vals[n++] = right_pos;
			time += large_tstep;
		}
		if(time >= duration)
			dostop = 1;
		/* fall thro */
	case(5):
		vals[n++] = time;
		if(dostop) {
			dostop++;
			break;
		}
		if(nexttimestep > 0.0) {
			vals[n++] = startpos;
			time+= nexttimestep;
			nexttimestep = -1.0;
		} else {
			vals[n++] = left_pos;
			time += small_tstep;
		}
		if(time >= duration)
			dostop = 1;
		break;
	}
	if(dostop < 2) {
		for (;;) {
			if(n + 12 >= valcnt) {
				fprintf(stdout,"ERROR: Internal error calculating memory required.\n");
				fflush(stdout);
				exit(1);
			}
			vals[n++] = time;
			vals[n++] = far_left_pos;
			if(dostop)
				break;
			time += small_tstep;
			if(time >= duration)
				dostop = 1;

			vals[n++] = time;
			vals[n++] = left_pos;
			if(dostop)
				break;
			time += large_tstep;
			if(time >= duration)
				dostop = 1;

			vals[n++] = time;
			vals[n++] = right_pos;
			if(dostop)
				break;
			time += small_tstep;
			if(time >= duration)
				dostop = 1;

			vals[n++] = time;
			vals[n++] = far_right_pos;
			if(dostop)
				break;
			time += small_tstep;
			if(time >= duration)
				dostop = 1;

			vals[n++] = time;
			vals[n++] = right_pos;
			if(dostop)
				break;
			time += large_tstep;
			if(time >= duration)
				dostop = 1;

			vals[n++] = time;
			vals[n++] = left_pos;
			if(dostop)
				break;
			time += small_tstep;
			if(time >= duration)
				dostop = 1;
		}
	}
	valcnt = n;
	for(n = 0;n < valcnt; n +=2) {
		fprintf(stdout,"INFO: %lf   %lf\n",vals[n],vals[n+1]);
	}
	fflush(stdout);
}

/**************************** SINJOIN *****************************/

void sinjoin(char c)
{
	double startval = number[0], endval = number[1];
	double starttime = number[2], endtime = number[3];
	int n, pointcnt = (int)round(number[4]), inverse = 0;
	double val = 0.0, valdiff = endval - startval;
	double timediff = endtime - starttime;
	double timestep = timediff/(double)pointcnt;
	double startrad = 0.0;
	double nstep_cosin = (PI/(double)pointcnt);
	double nstep_sin = (PI/(2.0 * (double)pointcnt));
	if((c == 'x' && valdiff < 0.0) || (c=='v' && valdiff > 0.0)) {
		inverse = 1;
		nstep_sin = -nstep_sin;
		startrad = PI/2.0;
	} else if(c=='c')
		startrad = PI;

	fprintf(stdout,"INFO: %lf  %lf\n",starttime,startval);
	for(n = 1; n< pointcnt;n++) {
		switch(c) {
		case('c'):
			startrad += nstep_cosin;
			val = (cos(startrad) + 1.0) / 2.0;
			break;
		case('v'):
		case('x'):
			startrad += nstep_sin;
			val = sin(startrad);
			if(inverse)
				val = 1.0 - val;
			break;
		}
		val *= valdiff;
		val += startval;
		starttime += timestep;
		fprintf(stdout,"INFO: %lf  %lf\n",starttime,val);
	}
	fprintf(stdout,"INFO: %lf  %lf\n",endtime,endval);
	fflush(stdout);
}

/************************************** BRKTIME_OWARP *****************************************/

void brktime_owarp()
{
	int m, n;
	double thistime, nexttime, thisval, nextval, step, frac, val, gap, sum;
	for(m=firstcnt+1;m<cnt;m+=2) {
		if(number[m] < FLTERR) {
			fprintf(stdout,"ERROR: Invalid value %lf in warping file (must be > 0)\n",number[m]);
			exit(1);
		}
	}
	if(!flteq(number[0],0.0)) {
		fprintf(stdout,"ERROR: Breakpoint file to BE warped must begin at time ZERO\n");
		exit(1);
	}
	number[0] = 0.0;
	fprintf(stdout,"INFO: %lf  %lf\n",number[0], number[1]);
	sum = 0.0;
	n = 2;
	m = firstcnt;
	for(;;) {
		while(number[m] <= sum) {
			if((m += 2) >= cnt)
				break;
		}
		if(m >= cnt)
			val = number[cnt-1];
		else {
			nexttime = number[m];
			nextval  = number[m+1];
			thistime = number[m-2];
			thisval  = number[m-1];
			step = nexttime - thistime;
			frac = (sum - thistime)/step;
			val = ((nextval - thisval) * frac) + thisval;
		}
		gap = number[n] - number[n-2];
		val *= gap;
		sum += val;
		fprintf(stdout,"INFO: %lf  %lf\n",sum, number[n+1]);
		if((n += 2) >= firstcnt)
			break;
	}
	fflush(stdout);
}
