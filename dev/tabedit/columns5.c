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

#define MIN_TEMPO (.01666667)		/* 1 beat per hour ! */
#define MAX_TEMPO (60000)			/* 1 beat per millisecond ! */

int alphabetical_order(char *,char *);
double leveltodb(double,int);
double dbtolevel(double);
void hinsert(int m,int t,int *perm,int permlen);
void ascending_sort_cells(int *perm,int ccnt);
void hshuflup(int k,int *perm,int permlen);
void hprefix(int m,int *perm,int permlen);
void randperm(int *perm, int permlen);
static void invertenv(double piv);
static void docross
		(double lastnval,double lastmval,double thisnval, double thismval,double time,int *j,double *out,int typ);
static void hhinsert(int m,int t,int setlen,int *perm);
static void hhprefix(int m,int setlen,int *perm);
static void hhshuflup(int k,int setlen,int *perm);
static void do_repet_restricted_perm(int *arr, int *perm, int arrsiz, int allowed, int endval);
static void get_metre(char [],int *barlen,int *beatsize);
static double get_tempo(char *str);
static double get_beat(int n,int barlen);
static void get_offset(char *str,double *offset);
static double randoffset(double scatter);
static double readbrk(float *warpvals,double time,int wcnt);

/***************************** PRODUCT ***************************/

void product(void)
{
	int n;
	double sum = number[0];
	for(n=1;n<cnt;n++)
	sum *= number[n];
	do_valout_as_message(sum);
	fflush(stdout);
}

/*************************** HZ_TO_MIDI ************************/

void hz_to_midi(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		number[n] = hztomidi(number[n]);
		do_valout(number[n]);
	}
	fflush(stdout);
}

/******************************** FIND_MEAN *************************/

void find_mean(void)
{
	double sum = 0.0;
	int n;
	for(n=0;n<cnt;n++)
		sum += number[n];
	sum /= (double)cnt;
	do_valout_as_message(sum);
	fflush(stdout);
}

/******************************** MIDI_TO_HZ **************************/

void midi_to_hz(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		number[n] = miditohz(number[n]);
		do_valout(number[n]);
	}
	fflush(stdout);
}

/*************************** MAJOR_TO_MINOR *************************/

void major_to_minor(void)
{
	int n;
	int m3 = (4 + ifactor)%12;	/* MIDI location of major 3rd */
	int m6 = (9 + ifactor)%12;	/* MIDI location of major 6th */
	for(n=0;n<cnt;n++) {
		factor = fmod(number[n],TWELVE);
		if(flteq(factor,(double)m3) || flteq(factor,(double)m6))
			number[n] -= 1.0;
		do_valout(number[n]);
	}
	fflush(stdout);
}

/****************** REMOVE_MIDI_PITCHCLASS_DUPLICATES *******************/

void remove_midi_pitchclass_duplicates(void)
{
	int n, m, move;
	int k = cnt-ifactor;
	int failed = 0;
	for(n=0;n<k;n++) {
		for(m=1;m<=ifactor;m++) {
			if(n+m >= cnt)
				break;
			if(flteq(fmod(number[n],12.0),fmod(number[n+m],12.0))) {
				if((move = m_repos(n+m))<0) {
					failed++;
				} else {
					n += move;	/* list shufld forwd, or not */
					m--;		/* m+1th item now at m */
				}
			}
		}
	}
	if(failed)
		fprintf(stdout,"WARNING: %d items failed to be separated.\n",failed);
	for(n=0;n<cnt;n++)
		do_valout(number[n]);
	fflush(stdout);
}

/************************** REVERSE_LIST *************************/

void reverse_list(void)
{
	int n;
	for(n=stringscnt-1;n>=0;n--)
		do_stringout(strings[n]);
	fflush(stdout);
}

/************************* ROTATE_MOTIF *************************/

void rotate_motif(void)
{
	int n;
	for(n=cnt-ifactor;n<cnt;n++)
		do_valout(number[n]);
	for(n=0;n<cnt-ifactor;n++)
		do_valout(number[n]);
	fflush(stdout);
}

/************************** RATIOS ****************************/

void ratios(void)
{
	int n;
	for(n=1;n<cnt;n++) {
		if(flteq(number[n-1],0.0))
			do_stringout("INF\n");
		else
			do_valout(number[n]/number[n-1]);
	}
	fflush(stdout);
}

/******************************** RECIPROCALS ***********************/

void reciprocals(int positive_vals_only)
{
	int n;
	if(positive_vals_only) {
		for(n=0;n<cnt;n++) {
			if(number[n] < FLTERR) {
				fprintf(stdout,"ERROR: Invalid value %d (%lf) for this process\n",n+1,number[n]);
				fflush(stdout);
				exit(1);
			}
		}
	}
	for(n=0;n<cnt;n++) {
		switch(condit) {
		case(0):
			if(flteq(number[n],0.0)) {
				if(!sloom && !sloombatch)
					fprintf(fp[1],"INFINITE\n");
				else if(sloombatch) {
					fprintf(stdout,"INFINITE\n");
					fflush(stdout);
				} else {
					fprintf(stdout,"ERROR: Division by zero encountered (item %d) : Impossible\n",n+1);
					fflush(stdout);
					exit(1);
				}
			} else
				do_valout(factor/number[n]);
			break;
		case('>'):
			if(number[n]>thresh) {
				if(flteq(number[n],0.0)) {
					if(!sloom && !sloombatch)
						fprintf(fp[1],"INFINITE\n");
					else if(sloombatch) {
						fprintf(stdout,"INFINITE\n");
						fflush(stdout);
					} else {
						fprintf(stdout,"ERROR: Division by zero encountered (item %d) : Impossible\n",n+1);
						fflush(stdout);
						exit(1);
					}
				} else
					do_valout(factor/number[n]);
			} else
				do_valout(number[n]);
			break;
		case('<'):
			if(number[n]<thresh) {
				if(flteq(number[n],0.0)) {
					if(!sloom && !sloombatch)
						fprintf(fp[1],"INFINITE\n");
					else if(sloombatch) {
						fprintf(stdout,"INFINITE\n");
						fflush(stdout);
					} else {
						fprintf(stdout,"ERROR: Division by zero encountered (item %d) : Impossible\n",n+1);
						fflush(stdout);
						exit(1);
					}
				} else
					do_valout(factor/number[n]);
			} else
				do_valout(number[n]);
			break;
		}
	}
	fflush(stdout);
}

/****************************** RANDOMISE_ORDER *********************/

void randomise_order(int *perm)
{
	int n;
	randperm(perm,stringscnt);
	for(n=0;n<stringscnt;n++)
		do_stringout(strings[perm[n]]);
	fflush(stdout);
}

/****************************** RANDOMISE_ORDER *********************/

void randomise_Ntimes(int *perm)
{
	int n = 0, m, lastperm;
	for(m = 0;m< ifactor;m++) {
		if(n == 0)
			randperm(perm,stringscnt);
		else {
			lastperm = perm[stringscnt - 1];
			randperm(perm,stringscnt);
			while(perm[0] == lastperm)
				randperm(perm,stringscnt);
		}
		for(n=0;n<stringscnt;n++)
			do_stringout(strings[perm[n]]);
	}
	fflush(stdout);
}

/************************** ADD_RANDVAL_PLUS_OR_MINUS ********************/

void add_randval_plus_or_minus(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		number[n] += ((drand48() * 2.0) - 1.0) * factor;
		do_valout(number[n]);
	}
	fflush(stdout);
}

/****************************** ADD_RANDVAL ***************************/

void add_randval(void)
{
	int n;
	for(n=0;n<cnt;n++) {
 		number[n] += drand48() * factor;
		do_valout(number[n]);
	}
	fflush(stdout);
}

/************************ MULTIPLY_BY_RANDVAL ************************/

void multiply_by_randval(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		number[n] *= drand48() * factor;
		do_valout(number[n]);
	}
	fflush(stdout);
}

/***************************** RANDCHUNKS ************************/

void randchunks(void)
{
	double sum = 0.0;
	double randrange = fabs(number[1] - number[0]);
	double minn = min(number[1],number[0]);
	while(sum<factor) {
		do_valout(sum);
		sum += (drand48() * randrange) + minn;
	}
	fflush(stdout);
}

/************************** GENERATE_RANDOM_VALUES ***********************/

void generate_random_values(void)
{
	double randrange = number[1] - number[0];
	int n;
	for(n=0;n<factor;n++)
		do_valout((drand48() * randrange) + number[0]);
	fflush(stdout);
}

/****************************** RANDOM_0S_AND_1S ********************/

void random_0s_and_1s(void)
{
	int n;
	double sum;
	int totcnt = (int)round(number[0]);
	for(n=0;n<totcnt;n++) {
		sum = drand48() * 2.0;
  		if(sum>=1.0)
			do_stringout("1\n");
		else
			do_stringout("0\n");
	}
	fflush(stdout);
}

/****************************** RANDOM_PAIRS ********************/

void random_pairs(void)
{
	int n;
	double sum;
	int totcnt = (int)round(number[2]);
	char tempa[200];
	char tempb[200];
	sprintf(tempa,"%d",(int)round(number[0]));
	sprintf(tempb,"%d",(int)round(number[1]));
	for(n=0;n<totcnt;n++) {
		sum = drand48() * 2.0;
  		if(sum>=1.0)
			do_stringout(tempb);
		else
			do_stringout(tempa);
	}
	fflush(stdout);
}

/****************************** RANDOM_0S_AND_1S_RESTRAINED ********************/

void random_0s_and_1s_restrained(void)
{
	int n, cnt0 = 0, cnt1 = 0;
	double sum;
	int totcnt = (int)round(number[0]);
	int limit  = (int)round(number[1]);
	for(n=0;n<totcnt;n++) {
		sum = drand48() * 2.0;
		if(sum>=1.0) {
			cnt1++;
			if(cnt1 <= limit) {
				do_stringout("1\n");
				cnt0 = 0;
			} else {
				do_stringout("0\n");
				cnt1 = 0;
				cnt0 = 1;
			}
		} else {
			cnt0++;
			if(cnt0 <= limit) {
				do_stringout("0\n");
				cnt1 = 0;
			} else {
				do_stringout("1\n");
				cnt0 = 0;
				cnt1 = 1;
			}
		}
	}
	fflush(stdout);
}

/****************************** RANDOM_PAIRS_RESTRAINED ********************/

void random_pairs_restrained(void)
{
	int n, cnt0 = 0, cnt1 = 0;
	double sum;
	int totcnt = (int)round(number[2]);
	int limit  = (int)round(number[3]);
	char tempa[200];
	char tempb[200];
	sprintf(tempa,"%d",(int)round(number[0]));
	sprintf(tempb,"%d",(int)round(number[1]));
	for(n=0;n<totcnt;n++) {
		sum = drand48() * 2.0;
		if(sum>=1.0) {
			cnt1++;
			if(cnt1 <= limit) {
				do_stringout(tempb);
				cnt0 = 0;
			} else {
				do_stringout(tempa);
				cnt1 = 0;
				cnt0 = 1;
			}
		} else {
			cnt0++;
			if(cnt0 <= limit) {
				do_stringout(tempa);
				cnt1 = 0;
			} else {
				do_stringout(tempb);
				cnt0 = 0;
				cnt1 = 1;
			}
		}
	}
	fflush(stdout);
}

/****************************** RANDOM_SCATTER **********************/

void random_scatter(void)
{
	int n;
	double scatter; 
	double *diffs = (double *)exmalloc((cnt-1)*sizeof(double));
	for(n=0;n<cnt-1;n++) {
	 	diffs[n] = number[n+1] - number[n];
		diffs[n] /= 2.0;
	}
	for(n=1;n<cnt-1;n++) {
		scatter = ((drand48() * 2.0) - 1.0) * factor;
		if(scatter > 0.0)
			number[n] += diffs[n] * scatter;
		else
			number[n] += diffs[n-1] * scatter;
	}
	print_numbers();
}

/************************** RANDOM_ELIMINATION ***********************/

void random_elimination(void)
{
	int n, m;
	for(n=0;n<ifactor;n++) {
		m = (int)(drand48() * cnt);	/* TRUNCATE */
		eliminate(m);
	}
	print_numbers();
}

/*************************** EQUAL_DIVISIONS ************************/

void equal_divisions(void)
{
	double interval = (number[1] - number[0])/factor;
	double sum = number[0];
	if(interval > 0.0) {
		while(sum<=number[1]) {
			do_valout(sum);
			 sum += interval;
		}
	} else {
		while(sum>=number[1]) {
			do_valout(sum);
			sum += interval;
		}
	}
	fflush(stdout);
}

/************************* LOG_EQUAL_DIVISIONS ***********************/

void log_equal_divisions(void)
{
	double sum		  = log(number[0]);
	double top	  = log(number[1]);
	double interval = (top - sum)/factor;
	if(sum < top) {
		while(sum <= top - FLTERR) {
			do_valout(exp(sum));
			sum += interval;
		}
 		
	} else {
		while(sum >= top + FLTERR) {
			do_valout(exp(sum));
			sum += interval;
		}
	}
	do_valout(number[1]);
	fflush(stdout);
}

/****************************** QUADRATIC_CURVE_STEPS ******************/

void quadratic_curve_steps(void)
{
	double sum, diff = number[1] - number[0];
	double step = fabs(1.0/(factor - 1.0));
	double thisstep = 0.0;
	int cnt = 0, ifactor = round(factor);
	if(diff>0.0)
		number[2] = 1.0/number[2];
	for(;;) {
		sum = pow(thisstep,number[2]);
		sum = (sum * diff) + number[0];
		do_valout(sum);
		if(++cnt >= ifactor)
			break;
		thisstep += step;
	}
	fflush(stdout);
}

/*************************** PLAIN_BOB ***************************/

void plain_bob(void)
{
	int n, m, k;
	for(n=0;n<cnt;n++) 
		do_valout(number[n]);
	for(m=0;m<cnt-1;m++) {
		for(k=0;k<cnt-1;k++) {
			bellperm1();
			for(n=0;n<cnt;n++)
				do_valout(number[n]);
			bellperm2();
			for(n=0;n<cnt;n++)
				do_valout(number[n]);
		}
		bellperm1();
		for(n=0;n<cnt;n++)
			do_valout(number[n]);
		bellperm3();
		for(n=0;n<cnt;n++)
			do_valout(number[n]);
	}	
	fflush(stdout);
}

/*********************** REPEAT_INTERVALS *************************/

void repeat_intervals(void)
{
	int n, m;
	double z, k;
	for(n=0;n<cnt-1;n++)
		do_valout(number[n]);
	z = k = number[n] - number[0];
	for(m=1;m<=ifactor;m++) {
		for(n=0;n<cnt-1;n++)	/*TW Feb 2005*/
			do_valout(number[n] + z);
		z += k;
	}
	fflush(stdout);
}

/************** MAKE_EQUAL_INTEVALS_BTWN_GIVEN_VALS ************/

void make_equal_intevals_btwn_given_vals(void)
{
	double top = number[1];
	double sum = number[0];

	if(factor > 0.0) {
		if(number[0] > number[1]) {
			top = number[0];
			sum = number[1];
		}
	} else {
			if(number[0] < number[1]) {
			top = number[0];
			sum = number[1];
		}
	}
	if(top >= sum ) {
		while(sum<=top) {
			do_valout(sum);
			sum += factor;
		}
	} else {
		while(sum>=top) {
			do_valout(sum);
			sum += factor;
		}
	}
	fflush(stdout);
}

/********************** CREATE_INTERVALS ***********************/

void create_intervals(void)
{
	int n;
	for(n=0;n<ifactor;n++) {
		do_valout(number[0] * n);
	}
	fflush(stdout);
}

/********************** CREATE_INTERVALS_FROM_BASE ***********************/

void create_intervals_from_base(void)
{
	int n;
	double sum = number[1];
	for(n=0;n<ifactor;n++) {
		do_valout(sum);
		sum += number[0];
	}
	fflush(stdout);
}

/********************** CREATE_RATIOS_FROM_BASE ***********************/

void create_ratios_from_base(void)
{
	int n;
	double sum = number[1];
	for(n=0;n<ifactor;n++) {
		do_valout(sum);
		if(sum > HUGE/2.0 || sum < -HUGE/2.0) {
			sprintf(errstr,"Calculation overflows.\n");
			do_error();
		}
		sum *= number[0];
	}
	fflush(stdout);
}

/********************** CREATE_EQUAL_STEPS ***********************/

void create_equal_steps(void)
{
	int n;
	double sum = number[0];
	double step = (number[1] - number[0])/((double)ifactor - 1.0);
	for(n=0;n<ifactor;n++) {
		do_valout(sum);
		sum += step;
	}
	fflush(stdout);
}

/********************** CREATE_EQUAL_VALS ***********************/

void create_equal_vals(void)
{
	int n;
	for(n=0;n<ifactor;n++) {
		do_valout(number[0]);
	}
	fflush(stdout);
}

/************************ CHANGE_VALUE_OF_INTERVALS ********************/

void change_value_of_intervals(void)
{
	double interval, sum = number[0];
	int n;
	for(n=1;n<cnt;n++) {
		do_valout(sum);
		switch(ro) {
			case('a'): interval=(number[n]-number[n-1])+factor; break;
			case('m'): interval=(number[n]-number[n-1])*factor; break;
			default:
				fprintf(stdout,"ERROR: Unkonwn case in change_value_of_intervals()\n");
				fflush(stdout);
				exit(1);
				break;
		}
		sum += interval;
	}
	do_valout(sum);
	fflush(stdout);
}

/******************** MOTIVICALLY_INVERT_MIDI **********************/

void motivically_invert_midi(void)
{
	double factor = 2.0 * number[0];
	int n;
	for(n=1;n<cnt;n++)
		number[n] = factor - number[n];
	print_numbers();
}

/********************** MOTIVICALLY_INVERT_HZ **********************/

void motivically_invert_hz(void)
{
	double factor;
	int n;
	if(flteq((factor = number[0] * number[0]),0.0)) {
		sprintf(errstr,"First frq is zero : can't proceed.\n");
		do_error();
	}
	for(n=1;n<cnt;n++)
		number[n] = factor/number[n];
	print_numbers();
}

/******************** GET_INTERMEDIATE_VALUES ************************/

void get_intermediate_values(void)
{
	int n;
	double d;
	for(n=1;n<cnt;n++) {
		d = (number[n] + number[n-1])/2.0;
		do_valout(d);
	}
	fflush(stdout);
}

/******************** INSERT_INTERMEDIATE_VALUES ************************/

void insert_intermediate_values(void)
{
	int n;
	double d;
	fprintf(stdout,"INFO: %lf\n",number[0]);
	for(n=1;n<cnt;n++) {
		d = (number[n] + number[n-1])/2.0;
		fprintf(stdout,"INFO: %lf\n",d);
		fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}

/******************** INSERT_INTERMEDIATE_VALP ************************/

void insert_intermediate_valp(void)
{
	int n;
	double d;
	for(n=1;n<cnt;n+=2) {
		fprintf(stdout,"INFO: %lf\n",number[n-1]);
		d = (number[n] + number[n-1])/2.0;
		fprintf(stdout,"INFO: %lf\n",d);
		fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}

/************************** GET_INTERVALS ***********************/

void get_intervals(void)
{
	int n;
	for(n=1;n<cnt;n++)
		do_valout(number[n]-number[n-1]);
	fflush(stdout);
}

/************************ GET_ONE_SKIP_N ************************/

void get_one_skip_n(void)
{
	int n = 0;
	ifactor = round(factor);
 	if(ifactor < 1) {
		sprintf(errstr,"Invalid parameter (%d)\n",ifactor);
		do_error();
	}
	while(n<stringscnt) {
		do_stringout(strings[n++]);
		n += ifactor;
	}
	fflush(stdout);
}

/************************ GET_N_SKIP_ONE *************************/

void get_n_skip_one(void)
{
	int m, n = 0;	
	int k, ifactor = round(factor);

	if(ifactor < 1) {
		sprintf(errstr,"Invalid parameter (%d)\n",ifactor);
		do_error();
	}
	while(n<stringscnt) {
		if((k = n + ifactor) >= stringscnt) {
			for(m=n;m<stringscnt;m++)
				do_stringout(strings[m]);
			break;
		}
		for(m=n;m<k;m++)
			do_stringout(strings[m]);
		n+= ifactor+1;
	}
	fflush(stdout);
}

/************************* SUM_NWISE **************************/

void sum_nwise(void)
{
	int n, m;
	ifactor = round(factor);
/* RWD */
	if(ifactor > cnt) {
		sprintf(errstr,"group size (N) too large for infile\n");
		do_error();
	}
	for(n=0;n<=cnt-ifactor;n++) {	/*RWD: test was just < */
		for(m=1;m<ifactor;m++)
			number[n] += number[n+m];
		do_valout(number[n]);
	}
	fflush(stdout);
}

/************************* SUM_MINUS_OVERLAPS *********************/

void sum_minus_overlaps(void)
{
	int n;
	double sum = 0.0;
	for(n=0;n<cnt;n++)
		sum += number[n];
	sum -= (double)(cnt-1) * factor;
	do_valout_as_message(sum);
	fflush(stdout);
}

/******************** SUM_ABSOLUTE_DIFFERENCES ********************/

void sum_absolute_differences(void)
{
	int n;
	double sum = 0.0;
	for(n=1;n<cnt;n++)
		sum += fabs(number[n] - number[n-1]);
	sum -= (double)(cnt-1) * factor;
	do_valout_as_message(sum);
	fflush(stdout);
}

/**************************** STACK ***************************/

void stack(int with_last)
{
	int n;
		double sum = 0.0;
	for(n=0;n<cnt;n++) {
		do_valout(sum);
		sum += number[n] - factor;
	}
	if(with_last) {
		sum += factor;
		do_valout(sum);
	}
	fflush(stdout);
}

/*********************** DUPLICATE_VALUES **************************/

void duplicate_values(void)
{
	int n, m;
	for(n=0;n<stringscnt;n++) {
		for(m=0;m<ifactor;m++)
			do_stringout(strings[n]);
	}
	fflush(stdout);
}

/*********************** DUPLICATE_VALUES_STEPPED **************************/

void duplicate_values_stepped(void)
{
	int n, m;
	double step = number[cnt+1];
	double offset = 0.0;
	ifactor = round(number[cnt]);
	for(m=0;m<ifactor;m++) {
		for(n=0;n<cnt;n++)
			fprintf(stdout,"INFO: %lf\n",number[n] + offset);
		offset += step;
	}
	fflush(stdout);
}
/**************************** DUPLICATE_LIST **********************/

void duplicate_list(void)
{
	int n, m;
	for(m=0;m<ifactor;m++) {
		for(n=0;n<stringscnt;n++)
			do_stringout(strings[n]);
	}
	fflush(stdout);
}

/****************************** FORMAT_VALS ****************************/

void format_vals(void)
{ /*RWD new Format option : recast by TW */
	int n, m, OK = 1;
	double d = (double)cnt/(double)ifactor;
	int rowcnt = cnt/ifactor;
	char temp[64];

	errstr[0] = ENDOFSTR;
	if(d > (double)rowcnt)
		rowcnt++;
	if((rowcnt > 82) && sloom) {
		sprintf(errstr,"Too many (%d) rows to handle",rowcnt);
		do_error();
	}
	for(n=0;n<cnt;n+=rowcnt) {
		for(m=0;m<rowcnt;m++) {
			if(!sloom && !sloombatch) {
				if(n!=0 && m==0)
					fprintf(fp[1],"\n");
				if(n+m < cnt)
					fprintf(fp[1],"%.5lf ",number[n+m]);
				else
					OK = 0;
			} else {
				if(n!=0 && m==0) {
					fprintf(stdout,"INFO: %s\n",errstr);
					errstr[0] = ENDOFSTR;
				}
				if(n+m < cnt) {
					sprintf(temp,"%.5lf ",number[n+m]);
					strcat(errstr,temp);
				} else
					OK = 0;
			}
		}
		if(!OK)
			break;
	}
	if(!sloom && !sloombatch)
		fprintf(fp[1],"\n");
	else
		fprintf(stdout,"INFO: %s\n",errstr);
	fflush(stdout);
}


/****************************** COLUMN_FORMAT_VALS ****************************/

void column_format_vals(void)
{	int n, m;

	double d = (double)cnt/(double)ifactor;
	int rowcnt = cnt/ifactor;
	char temp[64];
	errstr[0] = ENDOFSTR;
	if(d > (double)rowcnt)
		rowcnt++;
	for(n=0;n<rowcnt;n++) {
		for(m=n;m<cnt;m+=rowcnt) {
			if(!sloom && !sloombatch)
				fprintf(fp[1],"%.5lf ",number[m]);
			else {
				sprintf(temp,"%.5lf ",number[m]);
				strcat(errstr,temp);
			}
		}
		if(!sloom && !sloombatch)
			fprintf(fp[1],"\n");
		else {
			fprintf(stdout,"INFO: %s\n",errstr);
			errstr[0] = ENDOFSTR;
		}
	}
	fflush(stdout);
}


/************************ INTERVAL_LIMIT() ********************/

void interval_limit(void)
{
	double interval, sum = number[0];
	int n;
	do_valout(sum);
	for(n=1;n<cnt;n++) {
		interval=number[n]-number[n-1];
		switch(ro) {
		case('l'): interval = max(interval,factor);	break;
		case('L'): interval = min(interval,factor);	break;
		}
		sum += interval;
		do_valout(sum);
	}
	fflush(stdout);	
}

/********************************** TIME_DENSITY ******************************/

void time_density(void)
{
	int n, m = 0, k, ended = 0;
	double min_endtime, *endtime = (double *)exmalloc(ifactor * sizeof(double));
	for(n=0;n<ifactor;n++)
		endtime[n] = 0.0;
	min_endtime = 0.0;
	for(;;) {
		for(n=0;n<ifactor;n++) {
			if(flteq(endtime[n],min_endtime)) {
				do_valout(endtime[n]);
				endtime[n] += number[m];
				if(++m >= cnt) {
					ended = 1;
					break;
				}
				min_endtime = endtime[n];
				for(k=0;k<ifactor;k++) {
					if(endtime[k] < min_endtime)
						min_endtime = endtime[k];
				}
			}
		}
		if(ended)
			break;
	}
	fflush(stdout);
}

/********************************** DIVIDE_LIST ******************************/

void divide_list(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		switch(condit) {
		case(0):
			number[n] /= factor;
			break;
		case('>'):
			if(number[n]>thresh)
				number[n] /= factor;
			break;
		case('<'):
			if(number[n]<thresh)
				number[n] /= factor;
			break;
		}
		do_valout(number[n]);
	}
	fflush(stdout);
}

/********************************** GROUP ******************************/

void group(void)
{
	int n, m = 0;
	while(m < ifactor) {
		for(n=m;n<stringscnt;n+=ifactor) {
			do_stringout(strings[n]);
		}
		if(!sloom && !sloombatch)
			fprintf(fp[1],"\n");
		m++;
	}
	fflush(stdout);
}

/********************************** DUPLICATE_OCTAVES ******************************/

void duplicate_octaves(void)
{
	int n, m;
	double d;
	for(n=0;n<cnt;n++)
		do_valout(number[n]);
	for(m=1;m<=ifactor;m++) {
		d = 12.0 * m;
		for(n=0;n<cnt;n++)
			do_valout(number[n] + d);
	}
	fflush(stdout);
}

/********************************** DUPLICATE_OCTFRQ ******************************/

void duplicate_octfrq(void)
{
	int n, m;
	double d;
	for(n=0;n<cnt;n++)
		do_valout(number[n]);
	for(m=1;m<=ifactor;m++) {
		d = pow(2.0,(double)m);
		for(n=0;n<cnt;n++)
			do_valout(number[n] * d);
	}
	fflush(stdout);
}

/********************************** INTERVAL_TO_RATIO ******************************/

void interval_to_ratio(int semitones,int tstretch) 
{
	int n;
	double bum = 0.0;
	for(n=0;n<cnt;n++) {
		if(semitones) {
			bum = number[n];
			number[n] /= 12.0;
		}
	 	if(fabs(number[n]) > MAXOCTTRANS) {
			if(!semitones)
				bum = number[n];
			sprintf(errstr,"Item %d (%lf) is too large or small to convert.\n",n+1,bum);
			do_error();
		}
	}
	for(n=0;n<cnt;n++) {
		number[n] = pow(2.0,number[n]);
		if(tstretch)
			number[n] = 1.0/number[n];
		do_valout(number[n]);
	}
	fflush(stdout);
}

/********************************** RATIO_TO_INTERVAL ******************************/

void ratio_to_interval(int semitones,int tstretch) 
{
	int n;
	for(n=0;n<cnt;n++) {
		if(number[n] < FLTERR) {
			fprintf(stdout,"ERROR: ratio %d (%lf) is too small or an impossible (negative) value.\n",n+1,number[n]);
			fflush(stdout);
			exit(1);
		}
		if(tstretch)
			number[n] = 1.0/number[n];
	}
	for(n=0;n<cnt;n++) {
		number[n] = log(number[n]) * ONE_OVER_LN2;
		if(semitones)
			number[n] *= 12.0;
		fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}

/********************************** DO_SLOPE ******************************/

void do_slope(void) 
{
	int n;
	double ddiff;
	do_valout(number[0]);
	for(n=1;n<cnt;n++) {
		ddiff = number[n] - number[0];
		ddiff *= factor;
		number[n] = number[0] + ddiff;
		do_valout(number[n]);
	}
	fflush(stdout);
}


/********************************** ALPHABETIC_SORT ******************************/

void alphabetic_sort(void)
{
	int n,m;
	char tempp[200],*p;
	for(n=1;n<stringscnt;n++)  {
		p =  strings[n];
		strcpy(tempp,strings[n]);
		m = n-1;
		while(m >= 0 && alphabetical_order(tempp,strings[m])) {
			strings[m+1] = strings[m];
			m--; 
		}
		strings[m+1] = p;
	}
	for(n=0;n<stringscnt;n++)
		do_stringout(strings[n]);	 
	fflush(stdout);
}

/******************************* ALPHABETICAL_ORDER **************************/

#define UPPERCASE(x)  ((x) >= 'A' && (x) < 'Z')


int alphabetical_order(char *str1,char *str2)
{
	char p, q;
	int n,m;
	int j = strlen(str1);
	int k = strlen(str2);
	m = min(j,k);
	for(n=0;n<m;n++) {
		p = str1[n];
		q = str2[n];
		if(UPPERCASE(p))	  p += 32;
		if(UPPERCASE(q))	  q += 32;
		if(p > q)  	return(0);
		if(p < q)  	return(1);
	}
	if(k<j)
		return(0);
	return(1);
}

/********************************** LEVEL_TO_DB ******************************/

void level_to_db(int sampleval) 
{
	int n;	
	for(n=0;n<cnt;n++) {
		if(sampleval)
			number[n] /= (double)MAXSAMP;
		number[n] = leveltodb(number[n],n);
		sprintf(errstr,"%lfdB\n",number[n]);
		do_stringout(errstr);
	}
	fflush(stdout);
}

/********************************** DB_TO_LEVEL ******************************/

void db_to_level(int sampleval) 
{
	int n;
	for(n=0;n<cnt;n++) {
		number[n] = dbtolevel(number[n]);
		if(sampleval)
			number[n] *= (double)MAXSAMP;
		do_valout(number[n]);
	}
	fflush(stdout);
}

/******************************** LEVELTODB ***********************/

double leveltodb(double val,int n)
{
	
	if(val <= 0.0) {
		sprintf(errstr,"Gain value %d <= 0.0: Cannot proceed\n",n+1);
		do_error();
	}
	val = log10(val);
	val *= 20.0;
	return(val);
}	
	
/******************************** DBTOLEVEL ***********************/

double dbtolevel(double val)
{
	int isneg = 0;
	if(flteq(val,0.0))
		return(1.0);
	if(val < 0.0) {
		val = -val;
		isneg = 1;
	}
	val /= 20.0;
	val = pow(10.0,val);
	if(isneg)
		val = 1.0/val;
	return(val);
}	

/******************************** COLUMNATE ***********************/

void columnate(void)
{
	int n;
	for(n=0;n<cnt;n++)
		do_valout(number[n]);
	fflush(stdout);
}

/******************************** SAMP_TO_TIME ***********************/

void samp_to_time(void)
{
	int n;
	double inv_sr = 1.0/(double)ifactor;
	for(n=0;n<cnt;n++)
		do_valout(number[n] * inv_sr);
	fflush(stdout);
}

/******************************** TIME_TO_SAMP ***********************/

void time_to_samp(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		do_valout(number[n] * (double)ifactor);
	}
	fflush(stdout);
}

/******************************** DELETE_SMALL_INTERVALS ***********************/

void delete_small_intervals(void)
{
	int n, m;
	int start = 1, end = 1, shrink;
	double diff;
	for(n=1;n<cnt;n++) {
		if((diff = number[n] - number[n-1]) < 0) {
			fprintf(stdout,"WARNING: Numbers must be in ascending order for this option.\n");
			fflush(stdout);
			exit(1);
		}
		if(diff <= factor)
			end++;
		else {
			if((shrink = (end - start))) {
				for(m = n-1;m < cnt; m++)
					number[m-shrink] = number[m];
				end = start;
				cnt -= shrink;
				n -= shrink;
			}
			start++;
			end++;
		}
	}
	for(n=0;n<cnt;n++)
		do_valout(number[n]);
	fflush(stdout);
}

/******************************** MARK_EVENT_GROUPS ***********************/

void mark_event_groups(void)
{
	int n, m;
	int start = 1, end = 1, shrink;
	int orig_cnt = cnt;
	double diff;
	for(n=1;n<cnt;n++) {
		if((diff = number[n] - number[n-1]) < 0) {
			fprintf(stdout,"WARNING: Numbers must be in ascending order for this option.\n");
			fflush(stdout);
			exit(1);
		}
		if(diff <= factor) {
			end++;
		} else {
			shrink = end - start;
			switch(shrink) {
			case(1):		  	/* 2 in group, preserve */
				start++;
				break;
			case(0):			/* 1 isolated point, duplicate */
				cnt++;
				if(cnt > orig_cnt) {
					if((number = (double *)realloc((char *)number,cnt * sizeof(double)))==NULL) {
						fprintf(stdout,"WARNING: Out of memory for storing numbers.\n");
						fflush(stdout);
						exit(1);
					}
					orig_cnt = cnt;
				}
				for(m = cnt-1; m >= n; m--)
					number[m] = number[m-1];
				n++;
				start++;
				end++;
				break;
			default:			/* >2 in group, bracket */
				shrink--;
				for(m = n-1;m < cnt; m++)
					number[m-shrink] = number[m];
				end = start;
				cnt -= shrink;
				n -= shrink;
				break;
			}
			start++;
			end++;
		}
	}
	shrink = end - start;
	switch(shrink) {
	case(0):
		cnt++;
		if(cnt > orig_cnt) {
			if((number = (double *)realloc((char *)number,cnt * sizeof(double)))==NULL) {
				fprintf(stdout,"WARNING: Out of memory for storing numbers.\n");
				fflush(stdout);
				exit(1);
			}
		}
		for(m = cnt-1; m >= n; m--)
			number[m] = number[m-1];
		break;
	case(1):
		break;
	default: 
		shrink--;
		for(m = n-1;m < cnt; m++)
			number[m-shrink] = number[m];
		cnt -= shrink;
		break;
	}
	for(n=0;n<cnt;n++)
		do_valout(number[n]);
	fflush(stdout);
}

/******************************** SPANPAIR ***********************
 *
 * span pairs of values.
 */

void spanpair(void)
{
	int n, m, is_even = 1;
	int cnt2 = cnt * 2;
	if(cnt & 1) {
		fprintf(stdout,"WARNING: This process only works on an even number of values.\n");
		fflush(stdout);
		exit(1);
	}
	if((number = (double *)realloc((char *)number,cnt2 * sizeof(double)))==NULL) {
		fprintf(stdout,"WARNING: Out of memory for storing numbers.\n");
		fflush(stdout);
		exit(1);
	}
	m = cnt2-1; 
	for(n=cnt-1;n>=0;n--) {
		if(number[n] < 0.0) {
			fprintf(stdout,"WARNING: No negative numbers allowed in this option.\n");
			fflush(stdout);
			exit(1);
		}
		if(is_even) {
			number[m--] = number[n] + factor + thresh;
			number[m--] = number[n] + factor;
		} else {
			number[m--] = number[n];
			number[m--] = max(number[n] - thresh, 0.0);
		}
		is_even = !is_even;
	}
	for(n=0;n<cnt2;n++)
		do_valout(number[n]);
	fflush(stdout);
}

/******************************** SPAN ***********************
 *
 * span single values with a pair.
 */

void span(void)
{
	int n, m;
	int cnt2 = cnt * 4;
	if((number = (double *)realloc((char *)number,cnt2 * sizeof(double)))==NULL) {
		fprintf(stdout,"WARNING: Out of memory for storing numbers.\n");
		fflush(stdout);
		exit(1);
	}
	m = cnt2-1; 
	for(n=cnt-1;n>=0;n--) {
		if(number[n] < 0.0) {
			fprintf(stdout,"WARNING: No negative numbers allowed in this option.\n");
			fflush(stdout);
			exit(1);
		}
		number[m--] = number[n] + factor + thresh;
		number[m--] = number[n] + factor;
		number[m--] = number[n];
		number[m--] = max(number[n] - thresh, 0.0);
	}
	for(n=0;n<cnt2;n++)
		do_valout(number[n]);
	fflush(stdout);
}

/******************************** SPAN_XALL ***********************
 *
 * span single values.
 */

void span_xall(void)
{
	int n, m;
	int cnt2 = cnt * 3;
	if((number = (double *)realloc((char *)number,cnt2 * sizeof(double)))==NULL) {
		fprintf(stdout,"WARNING: Out of memory for storing numbers.\n");
		fflush(stdout);
		exit(1);
	}
	m = cnt2-1; 
	for(n=cnt-1;n>=0;n--) {
		if(number[n] < 0.0) {
			fprintf(stdout,"WARNING: No negative numbers allowed in this option.\n");
			fflush(stdout);
			exit(1);
		}
		number[m--] = number[n] + thresh;
		number[m--] = number[n];
		number[m--] = max(number[n] - thresh, 0.0);
	}
	for(n=0;n<cnt2;n++)
		do_valout(number[n]);
	fflush(stdout);
}

/******************************** ALTERNATION PATTERNS ***********************/

void alt0101(void) {
	int n;
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n & 1]);
	fflush(stdout);
}
		
void alt0011(void) {
	int n;
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n & 2]);
	fflush(stdout);
}

void alt01100(void) {
	int n;
	fprintf(stdout,"INFO: %lf\n",number[0]);
	cnt--;
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[!(n & 2)]);
	fflush(stdout);
}

void alt0r0r(void) {
	int n;
	double rrange = number[2] - number[1];
	for(n=0;n<cnt;n++) {
		if(n & 1) {
			fprintf(stdout,"INFO: %lf\n",(drand48() * rrange) + number[1]);
		} else {
			fprintf(stdout,"INFO: %lf\n",number[0]);
		}
	}
	fflush(stdout);
}

void altr0r0(void) {
	int n;
	double rrange = number[2] - number[1];
	for(n=0;n<cnt;n++) {
		if(n & 1) {
			fprintf(stdout,"INFO: %lf\n",number[0]);
		} else {
			fprintf(stdout,"INFO: %lf\n",(drand48() * rrange) + number[1]);
		}
	}
	fflush(stdout);
}

void altrr00(void) {
	int n;
	double rrange = number[2] - number[1];
	for(n=0;n<cnt;n++) {
		if(n & 2) {
			fprintf(stdout,"INFO: %lf\n",number[0]);
		} else {
			fprintf(stdout,"INFO: %lf\n",(drand48() * rrange) + number[1]);
		}
	}
	fflush(stdout);
}

void alt00rr(void) {
	int n;
	double rrange = number[2] - number[1];
	for(n=0;n<cnt;n++) {
		if(n & 2) {
			fprintf(stdout,"INFO: %lf\n",(drand48() * rrange) + number[1]);
		} else {
			fprintf(stdout,"INFO: %lf\n",number[0]);
		}
	}
	fflush(stdout);
}

void alt0rr00r(void) {
	int n;
	double rrange = number[2] - number[1];
	fprintf(stdout,"INFO: %lf\n",number[0]);
	cnt--;
	for(n=0;n<cnt;n++) {
		if(n & 2) {
			fprintf(stdout,"INFO: %lf\n",number[0]);
		} else {
			fprintf(stdout,"INFO: %lf\n",(drand48() * rrange) + number[1]);
		}
	}
	fflush(stdout);
}

void altr00rr0(void) {
	int n;
	double rrange = number[2] - number[1];
	fprintf(stdout,"INFO: %lf\n",(drand48() * rrange) + number[1]);
	cnt--;
	for(n=0;n<cnt;n++) {
		if(n & 2) {
			fprintf(stdout,"INFO: %lf\n",(drand48() * rrange) + number[1]);
		} else {
			fprintf(stdout,"INFO: %lf\n",number[0]);
		}
	}
	fflush(stdout);
}

void altRR00(void) {
	int n;
	double k = 0.0;
	double rrange = number[2] - number[1];
	for(n=0;n<cnt;n++) {
		if(n & 2) {
			fprintf(stdout,"INFO: %lf\n",number[0]);
		} else {
			if (!(n & 1)) {
				k = (drand48() * rrange) + number[1];
			}
			fprintf(stdout,"INFO: %lf\n",k);
		}
	}
	fflush(stdout);
}

void alt00RR(void) {
	int n;
	double k = 0.0;
	double rrange = number[2] - number[1];
	for(n=0;n<cnt;n++) {
		if(n & 2) {
			if (!(n & 1)) {
				k = (drand48() * rrange) + number[1];
			}
			fprintf(stdout,"INFO: %lf\n",k);
		} else {
			fprintf(stdout,"INFO: %lf\n",number[0]);
		}
	}
	fflush(stdout);
}

void alt0RR00R(void) {
	int n;
	double k = 0.0;
	double rrange = number[2] - number[1];
	fprintf(stdout,"INFO: %lf\n",number[0]);
	cnt--;
	for(n=0;n<cnt;n++) {
		if(n & 2) {
			fprintf(stdout,"INFO: %lf\n",number[0]);
		} else {
			if (!(n & 1)) {
				k = (drand48() * rrange) + number[1];
			}
			fprintf(stdout,"INFO: %lf\n",k);
		}
	}
	fflush(stdout);
}

void altR00RR0(void) {
	int n;
	double k = 0.0;
	double rrange = number[2] - number[1];
	fprintf(stdout,"INFO: %lf\n",(drand48() * rrange) + number[1]);
	cnt--;
	for(n=0;n<cnt;n++) {
		if(n & 2) {
			if (!(n & 1)) {
				k = (drand48() * rrange) + number[1];
			}
			fprintf(stdout,"INFO: %lf\n",k);
		} else {
			fprintf(stdout,"INFO: %lf\n",number[0]);
		}
	}
	fflush(stdout);
}

/******************************** CHECK FOR ASCENDING_ORDER ***********************/

void ascending_order(void) {
	int n, OK = 1;
	for(n=1;n<cnt;n++) {
		if(number[n] <= number[n-1]) {
			OK = 0;
			break;
		}
	}
	if(!sloom && !sloombatch) {
		if(OK)
			fprintf(stdout, "sequence is in ascending order.");
		else
			fprintf(stdout, "sequence is NOT in ascending order at item %d (%lf)",n+1,number[n]);
	} else {
		if(OK)
			fprintf(stdout, "WARNING: sequence is in ascending order.");
		else
			fprintf(stdout, "WARNING: sequence is NOT in ascending order at item %d (%lf)",n+1,number[n]);
	}
	fflush(stdout);
}

/******************************** EDIT INDIVIDUAL VALUES ***********************/

void delete_vals(void) {
	int n, m, k = (int)factor;
	if(k < ifactor) {
		n = ifactor;
		ifactor = k;
		k = n;
	}
	k = min(k,cnt);
	if(k < cnt) {
		for(n=ifactor-1,m=k;m<cnt;m++,n++)
			number[n] = number[m];
		cnt -= (k - ifactor + 1);
	} else {
		cnt = ifactor-1;
	}
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

void replace_val(void) {
	int n;
	number[ifactor-1] = factor;
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

void insert_val(void) {			/* NB we have already malloced the extra space required */
	int n;
	ifactor--;
	if(ifactor < cnt) {
		for(n = cnt;n >= ifactor; n--)
			number[n] = number[n-1];
	}
	number[ifactor] = factor;
	cnt++;
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

/*************************** QUANTISED_SCATTER *******************************
 *
 *	number[0] = quantisation;
 *	number[1] = duration;
 *	number[2] = scatter;
 */

void quantised_scatter(int *perm,int permlen)
{
	int n, m, here, k, q_per_step, q_scat, q_hscat, total_q, iscat;
	double quantisation = number[0], dur = number[1], scat = number[2];
	total_q = (int)floor(dur/quantisation) + 1;
	q_per_step = (int)ceil(total_q/(cnt-1));
	n = 0;
	number[n++] = 0.0;
	if(scat <= 1.0)  {
		q_scat = (int)round(q_per_step * scat);
		q_hscat = q_scat/2;
		here = q_per_step;
		while(n < cnt) {
			k = (int)floor(drand48() * (double)q_scat) - q_hscat;
			number[n++] = here + k;
			here += q_per_step;
		}
	} else {
		iscat = round(scat);
		q_per_step *= iscat;
		here = 0;
		while(n < cnt) {
			randperm(perm,permlen);				/* permute the next group of quantisation cells */
			ascending_sort_cells(perm,iscat);	/* sort the first 'iscat' into ascending order */

			for(m=0;m<iscat;m++)
				perm[m]++;						/*	change range to range 1 to N, rather than 0 to N-1 */
			for(m=0;m<iscat;m++) {				/* position items above base-cellcnt */
				number[n] = here + perm[m];
				if(++n >= cnt)
					break;	
			}
			here += q_per_step;
		}
		free(perm);
	}
	for(n=0;n<cnt;n++)
		number[n] *= quantisation;
	print_numbers();
}

/*************************** RANDPERM *******************************
 *
 * Produce a random permutation of k integers.
 */

void randperm(int *perm,int permlen)
{
	int n, t;
	for(n=0;n<permlen;n++) {
		t = (int)floor(drand48() * (n+1));
		if(t==n) {
			hprefix(n,perm,permlen);
		} else {
			hinsert(n,t,perm,permlen);
		}
	}
}

void hinsert(int m,int t,int *perm,int permlen)
{
	hshuflup(t+1,perm,permlen);
	perm[t+1] = m;
}

void hprefix(int m,int *perm,int permlen)
{
	hshuflup(0,perm,permlen);
	perm[0] = m;
}

void hshuflup(int k,int *perm,int permlen)
{
	int n, *i;
	int z = permlen - 1;
	i = perm+z;
	for(n = z;n > k;n--) {
		*i = *(i-1);
		i--;
	}
}

void ascending_sort_cells(int *perm,int ccnt)
{
	int n, m, sct;
	for(n=0;n<(ccnt-1);n++) {
		for(m=n+1;m<ccnt;m++) {
			if(*(perm+m) < *(perm+n)) {
				sct 	  = *(perm+m);
				*(perm+m) = *(perm+n);
				*(perm+n) = sct;
			}
		}
	}
}

/************* REPLACING VALUES EQUAL TO > OR < A GIVEN VAL ****************/

void replace_equal(void) {
	int n;
	double maxe = number[cnt] + FLTERR;
	double mine = number[cnt] - FLTERR;
	double replace = number[cnt+1];
	for(n=0;n<cnt;n++) {
		if((number[n] < maxe) && (number[n] > mine))
			number[n] = replace;
		fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}
		
void replace_less(void) {
	int n;
	double mine = number[cnt];
	double replace = number[cnt+1];
	for(n=0;n<cnt;n++) {
		if(number[n] < mine)
			number[n] = replace;
		fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}
		
void replace_greater(void) {
	int n;
	double maxe = number[cnt];
	double replace = number[cnt+1];
	for(n=0;n<cnt;n++) {
		if(number[n] > maxe)
			number[n] = replace;
		fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}

/*************************** GAPPED QUANTISED GRIDS *************************/

void grid(int is_outside)
{
	int n, k, tot;
	double quant = number[cnt], sum, dogrid = 0;
	if(quant <= .001) {
		fprintf(stdout,"ERROR: Quantisation step too small.\n");
		fflush(stdout);
		return;
	} else if((tot = (int)ceil(number[cnt-1]/quant)) > 10000) {
		fprintf(stdout,"ERROR: Quantisation too small, relative to total duration.\n");
		fflush(stdout);
		return;
	} else if(tot < 2) {
		fprintf(stdout,"ERROR: Quantisation too large, relative to total duration.\n");
		fflush(stdout);
		return;
	}
	if((permm = (double *)malloc(tot * sizeof(double))) == NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		return;
	}
	if(is_outside)
		dogrid = 1;	
	n = 0;
	k = 0;
	sum = 0.0;
	while(n < cnt) {
		if(dogrid) {
			while(sum < number[n]) {
				permm[k++] = sum;
				sum += quant;
			}
		} else {		
			while(sum < number[n])
				sum += quant;
		}
		dogrid = !dogrid;
		n++;
	}
 	for(n=0;n<k;n++)
		fprintf(stdout,"INFO: %lf\n",permm[n]);
	fflush(stdout);
}

/****** DELETE N VALS AT RANDOM : NOT MORE THAN M ADJACENT VALS DELETED : NOT MORE THAN K ADJACENT VALS UNDELETED ******/

void randdel_not_adjacent(void)
{
	int adj, gap, sum, i, j, k, n, z;
	int delitems, keptitems, keepset_cnt, delset_cnt;
	int *vacuum, *box, *boxcnt, *box_assocd_with_cntr_no, *cntr_assocd_with_box_no;
	int remainder, unfilled_boxes, inverted = 0;
	delitems = round(number[cnt]);	/* number of items to delete */
	adj	= round(number[cnt+1]);		/* max no of adjacent items to delete */
	gap	= round(number[cnt+2]);		/* max no of adjacent non-deleted items */

	if(delitems >= cnt) {
		fprintf(stdout,"ERROR: This will delete the whole column.");
		fflush(stdout);
		return;
	}
	if(delitems < 1) {
		fprintf(stdout,"ERROR: Must delete 1 or more items.");
		fflush(stdout);
		return;
	}


	if(delitems > cnt/2) {
		/* Invert the algorithm */
		inverted = 1;
		keptitems = delitems;
		delitems = cnt - keptitems;
		k = adj;
		adj = gap;
		gap = k;
	} else {
		keptitems = cnt - delitems;
	}	
	if((vacuum = (int *)malloc(delitems * sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		return;
	}
	delset_cnt = 0;
	sum = 0;
	while(sum < delitems-1) {				/* generating deletable groups of random sizes */
		k = (int)floor(drand48()*adj) + 1;	/* within range set by 'adj' */
		if((sum+k) > delitems-1)			/* once enough items deleted, break */			
			break;
		else {
			vacuum[delset_cnt++] = k;		/* otherwise store the size of deletable group */
			sum += k;
		}
	}
	if(sum < delitems)						/* fix the size of final deletable group, to make deleted total correct */
		vacuum[delset_cnt++] = delitems - sum;

	if((vacuum = (int *)realloc((char *)vacuum,delset_cnt * sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Memory reallocation error.\n");
		fflush(stdout);
		return;
	}

	if(delset_cnt > keptitems) {			/* must be as many kept items as deleted groups, to separate those groups */
		fprintf(stdout,"ERROR: Not enough undeleted items remaining to complete the task.\n");
		fflush(stdout);
		return;
	} else if(delset_cnt == keptitems) { 	/* exactly as many keptitems as deleted groups, they must alternate */
		j = 0;
		n = 0;
		i = (int)floor(drand48() * 2.0);	/* select at random whether to start with deletes or not */
		if(inverted) {
			if(i) {								/* and either ... */
				while(j < delset_cnt) {
					for(k=0;k<vacuum[j];k++) {	/* print x items */
						fprintf(stdout,"INFO: %lf\n",number[n]);
						if(++n > cnt) {
							fprintf(stdout,"ERROR: Program anomaly in counting data. 6\n");
							break;
						}
					}							/* then skip 1 */
					if(++n > cnt) {
						fprintf(stdout,"ERROR: Program anomaly in counting data. 7\n");
						break;
					}
					j++;
				}
				fflush(stdout);			
				return;
			} else {							/* or .... */
				while(j < delset_cnt) {			/* skip 1 */
					if(++n > cnt) {
						fprintf(stdout,"ERROR: Program anomaly in counting data. 8\n");
						break;
					}
					for(k=0;k<vacuum[j];k++) {	/* then print x items */
						fprintf(stdout,"INFO: %lf\n",number[n]);	
						if(++n > cnt) {
							fprintf(stdout,"ERROR: Program anomaly in counting data. 9\n");
							break;
						}
					}
					j++;
				}
				fflush(stdout);
				return;
			}
		} else {
			if(i) {								/* and either ... */
				while(j < delset_cnt) {
					for(k=0;k<vacuum[j];k++) {	/* skip x items */
						if(++n > cnt) {
							fprintf(stdout,"ERROR: Program anomaly in counting data. 10\n");
							break;
						}
					}							/* then print 1 */
					fprintf(stdout,"INFO: %lf\n",number[n]);
					if(++n > cnt) {
						fprintf(stdout,"ERROR: Program anomaly in counting data. 11\n");
						break;
					}
					j++;
				}
				fflush(stdout);			
				return;
			} else {							/* or .... */
				while(j < delset_cnt) {			/* print 1 */
					fprintf(stdout,"INFO: %lf\n",number[n]);	
					if(++n > cnt) {
						fprintf(stdout,"ERROR: Program anomaly in counting data. 12\n");
						break;
					}
					for(k=0;k<vacuum[j];k++) {	/* then skip x items */
						if(++n > cnt) {
							fprintf(stdout,"ERROR: Program anomaly in counting data. 13\n");
							break;
						}
					}
					j++;
				}
				fflush(stdout);
				return;
			}
		}
	}
	keepset_cnt = delset_cnt+1;				/* otherwise, let there be 1 more sets of kept items then of deleted items */
											/* This is an aesthetic, choice.. there could be an equal number */
	if(( k = (int)ceil((double)keptitems/(double)keepset_cnt)) > gap) {
		gap = k + 1;						/* if min-size of largest kept group is > gap, can't use gap value */
		fprintf(stdout,"ERROR: Intergap distance incompatible with other demands. Adjusting to %d.\n",gap);
		fflush(stdout);
	}		
	if(((box	 = (int *)malloc(keepset_cnt * sizeof(int)))==NULL)
	|| ((boxcnt	 = (int *)malloc(keepset_cnt * sizeof(int)))==NULL)
	|| ((box_assocd_with_cntr_no = (int *)malloc(keepset_cnt * sizeof(int)))==NULL)
	|| ((cntr_assocd_with_box_no = (int *)malloc(keepset_cnt * sizeof(int)))==NULL)) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		return;
	}											
	for(n=0;n<keepset_cnt;n++) {					/* regard each set of kept items as a box */
		box_assocd_with_cntr_no[n] = n;				/* link each ball-counter to each box */
		cntr_assocd_with_box_no[n] = n;				/* link each box to each ball-counter */
			box[n] = 1;								/* Put 1 ball in each box: every kept set must have at least 1 item */
		boxcnt[n] = 0;								/* preset the boxcnts to zero */
	}
	if((remainder = keptitems - keepset_cnt) > 0) {	/* if any leftover balls */
		unfilled_boxes = keepset_cnt;				/* set number of unfilled boxes */
		for(n=0;n<remainder;n++) {					/* for all remaining balls */
			z = (int)floor(drand48() * unfilled_boxes);		
			box[z]++;								/* distribute each ball to a random box */

			if(box[z] >= gap) {									/* if the box getting the ball is now full */
				boxcnt[cntr_assocd_with_box_no[z]] = box[z];	/* store the count of balls in the box */
				unfilled_boxes--;								/* reduce number of boxes to put random balls into */
				while(z < unfilled_boxes) {					
					box[z] = box[z+1];					/* eliminate full box, by shuffling boxes above downwards */
					k = cntr_assocd_with_box_no[z+1];	/* get the boxcnter associated with the next box */
					box_assocd_with_cntr_no[k]--;		/* force it to point to next lower box (as boxes have moved down 1) */
					j = box_assocd_with_cntr_no[k];		/* get the box it now points to */
					cntr_assocd_with_box_no[j] = k;		/* get that to point back to the box counter */ 			
					z++;
				}
			}
		}
	}
	for(n=0;n<keepset_cnt;n++) {						/* save counts of balls in remaining boxes */
		if(boxcnt[n] <= 0)
			boxcnt[n] = box[box_assocd_with_cntr_no[n]];
	}	
	j = 0;
	n = 0;
	if(inverted) {
		while(j < delset_cnt) {
			for(k=0;k<boxcnt[j];k++) {						/* skip x items */
				if(++n > cnt) {
					fprintf(stdout,"ERROR: Program anomaly in counting data 4.\n");
					return;
				}
			}
			for(k=0;k<vacuum[j];k++) {						/* print y items */
				fprintf(stdout,"INFO: %lf\n",number[n]);
				if(++n > cnt) {
					fprintf(stdout,"ERROR: Program anomaly in counting data 5.\n");
					return;
				}
			}	   /* Note, all inverted patterns start and end with OFF events - an aesthetic decision */
			j++;
		}
	} else {
		while(j < delset_cnt) {
			for(k=0;k<boxcnt[j];k++) {				   		/* print x items */
				fprintf(stdout,"INFO: %lf\n",number[n]);
				if(++n > cnt) {
					fprintf(stdout,"ERROR: Program anomaly in counting data 1.\n");
					return;
				}
			}
			for(k=0;k<vacuum[j];k++) {						/* skip y items */
				if(++n > cnt) {
					fprintf(stdout,"ERROR: Program anomaly in counting data 2.\n");
					return;
				}
			}
			j++;
		}
		for(k=0;k<boxcnt[j];k++) {				   			/* print last x items */
			fprintf(stdout,"INFO: %lf\n",number[n]);
			if(++n > cnt) {
				fprintf(stdout,"ERROR: Program anomaly in counting data 3.\n");
				break;										
			}			/* Note, all patterns start and end with ON events - an aesthetic decision */
		}	 		
	}
	fflush(stdout);					
}

/****************************** REPLACE_WITH_RAND ******************************/

void replace_with_rand(int type) {

	int n;
	double lim = number[cnt], randlo = number[cnt+1], randhi = number[cnt+2];
	double k, randrang, limhi, limlo;

	if(randhi < randlo) {
		k 	   = randhi;
		randhi = randlo;
		randlo = k;
	}
	randrang = randhi - randlo;
	switch(type) {
	case(0):			/* equal */
		limhi = lim + FLTERR;
		limlo = lim - FLTERR;
		for(n=0;n<cnt;n++) {
			if((number[n] < limhi) && (number[n] > limlo))
				number[n] = (drand48() * randrang) + randlo;
			fprintf(stdout,"INFO: %lf\n",number[n]);
		}
		break;
	case(-1):			/* less */
		for(n=0;n<cnt;n++) {
			if(number[n] < lim)
				number[n] = (drand48() * randrang) + randlo;
			fprintf(stdout,"INFO: %lf\n",number[n]);
		}
		break;
	case(1):			/* greater */
		for(n=0;n<cnt;n++) {
			if(number[n] > lim)
				number[n] = (drand48() * randrang) + randlo;
			fprintf(stdout,"INFO: %lf\n",number[n]);
		}
		break;
	}
	fflush(stdout);
}

/****************************** RANDQUANTA_IN_GAPS ******************************/

void randquanta_in_gaps(void)
{
	int totcnt = round(number[cnt]);			  						/* number of vals to make */
	double q = number[cnt+1];											/* quantisation of grid */
	int mincnt = round(number[cnt+2]), maxcnt = round(number[cnt+3]);	/* min & max no of events in each time-interval */
	int k,j,n,m, posibmax = 0, maxqpnts = 0, tot_boxes,minpos,maxpos,remainder,unfilled_boxes;
	int boxpos, orig_boxpos;
	int *perm,*box,*boxcnt,*qpnts,*box_assocd_with_cntr_no,*cntr_assocd_with_box_no;
	double mintim,maxtim;
	double *qbas;
		
	if(maxcnt < mincnt) {	/* orient cnt-range */
		n = maxcnt;
		maxcnt = mincnt;
		mincnt = n;
	}
	if(mincnt < 1) {
		fprintf(stdout,"ERROR: Invalid count of number of items (%d)\n",mincnt);
		fflush(stdout);
		exit(1);
	}
	if(cnt & 1)				/* Force even number of pairs */
		cnt--;

	if((tot_boxes = cnt/2) < 1) {
		fprintf(stdout,"Too few value pairs in input column.\n");
		fflush(stdout);
		exit(1);
	}
	if(totcnt < tot_boxes * mincnt) {
		fprintf(stdout,"Insufficient items to distribute amongst the %d pairs.\n",tot_boxes);
		fflush(stdout);
		exit(1);
	}
	if(((box 	= (int *)malloc(tot_boxes * sizeof(int)))==NULL) 		/* 'boxes' store random placed 'balls' */
	|| ((boxcnt	= (int *)malloc(tot_boxes * sizeof(int)))==NULL) 		/* counts of balls in 'boxes' */
	|| ((qpnts 	= (int *)malloc(tot_boxes * sizeof(int)))==NULL)		/* no. of q-points in each time-interval */
																		/* = maximum number of balls for each box */
	|| ((qbas 	= (double *)malloc(tot_boxes * sizeof(double)))==NULL)	/* first q-point time in each time-interval */
	|| ((box_assocd_with_cntr_no = (int *)malloc(tot_boxes * sizeof(int)))==NULL)
	|| ((cntr_assocd_with_box_no = (int *)malloc(tot_boxes * sizeof(int)))==NULL)) {
		fprintf(stdout,"Out of memory.\n");								/* because boxes are 'deleted' & shuffled down */
		fflush(stdout);
		exit(1);
	}
	for(n=0, m =0; n < cnt; n += 2,m++) {
		mintim = number[n];
		maxtim = number[n+1];

		minpos = (int)floor(mintim/q);
		if(((double)minpos * q) < mintim)
			minpos++;
		maxpos = (int)floor(maxtim/q);

		qpnts[m] = maxpos - minpos + 1;
		if(qpnts[m] < mincnt) {
			fprintf(stdout,"ERROR: timegap between %lf and %lf will not take  %d items\n",number[n],number[n+1],mincnt);
			fflush(stdout);
			exit(1);
		}
		if(m == 0)	maxqpnts = qpnts[0];
		else		maxqpnts = max(maxqpnts,qpnts[m]);
	
		qbas[m]  = minpos * q;
		posibmax += min(qpnts[m],maxcnt);
	}
	if(posibmax < totcnt) {
		fprintf(stdout, "ERROR: total count of items exceeds available spaces.\n");
		fflush(stdout);
		exit(1);
	}

	if((perm = (int *)malloc(maxqpnts * sizeof(int)))==NULL) {
		fprintf(stdout,"Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	remainder = totcnt;
				
	/* DISTRIBUTE THE REMAINING ITEMS AT RANDOM (with constraints) BETWEEN THE boxcnt BOXES */

	for(n=0;n<tot_boxes;n++) {							/* regard each set of kept items as a box */
		box_assocd_with_cntr_no[n] = n;					/* link each ball-counter to each box */
		cntr_assocd_with_box_no[n] = n;					/* link each box to each ball-counter */
		box[n] = mincnt;								/* Put min no of balls in each box */
		remainder -= mincnt;
		boxcnt[n] = 0;									/* preset the boxcnts to zero */
	}
	if(remainder > 0) {									/* if any leftover balls */

							/* ELIMINATE ANY BOXES WHICH ARE ALREADY FULL */
		unfilled_boxes = tot_boxes;						/* set number of unfilled boxes as total no of boxes */
		boxpos = 0;
		for(n=0,m=0;n<tot_boxes;n++) {   				/* for all boxes */
			if(box[boxpos] >= qpnts[n]) {				/* if the box is already full */
				boxcnt[n] = box[boxpos];				/* store the count of balls that are in the box */
				unfilled_boxes--;						/* reduce number of boxes to put random balls into */
				orig_boxpos = boxpos;					/* save the position of the full-box which is being eliminated */
				while(boxpos < unfilled_boxes) {				
					box[boxpos] = box[boxpos+1];		/* eliminate full box, by shuffling boxes above downwards */
					k = cntr_assocd_with_box_no[boxpos+1];	/* get the boxcnter associated with the next box */
					box_assocd_with_cntr_no[k]--;		/* force it to point to next lower box (as boxes have moved down 1) */
					j = box_assocd_with_cntr_no[k];		/* get the box it now points to */
					cntr_assocd_with_box_no[j] = k;		/* get that to point back to the box counter */ 			
					boxpos++;
				}
				boxpos = orig_boxpos;					/* reset box position to where it was: now points to new box */
			} else { 									
				boxpos++;								/* if box getting ball is NOT full, go on to next box */
			}
		}
									/* DISTRIBUTE REMAINING BALLS */
		for(n=0;n<remainder;n++) {   					/* for all remaining balls */
			boxpos = (int)floor(drand48() * unfilled_boxes);		
			box[boxpos]++;								/* distribute each ball to a random box */
			k = cntr_assocd_with_box_no[boxpos];
			if(box[boxpos] >= qpnts[k]) {				/* if the box getting the ball is now full */
				boxcnt[k] = box[boxpos];				/* store the count of balls that are in the box */
				unfilled_boxes--;						/* reduce number of boxes to put random balls into */
				while(boxpos < unfilled_boxes) {					
					box[boxpos] = box[boxpos+1];		/* eliminate full box, by shuffling boxes above downwards */
					k = cntr_assocd_with_box_no[boxpos+1];	/* get the boxcnter associated with the next box */
					box_assocd_with_cntr_no[k]--;		/* force it to point to next lower box (as boxes have moved down 1) */
					j = box_assocd_with_cntr_no[k];		/* get the box it now points to */
					cntr_assocd_with_box_no[j] = k;		/* get that to point back to the box counter */ 			
					boxpos++;
				}
			}
		}
	}
	for(n=0;n<tot_boxes;n++) {							/* save counts of balls in remaining boxes */
		if(boxcnt[n] <= 0)
			boxcnt[n] = box[box_assocd_with_cntr_no[n]];
	}	
	/* DISTRIBUTE THE VALS AT RANDOM WITHIN THE BOXES */

	for(n=0;n<tot_boxes;n++) {
		randperm(perm,qpnts[n]);						/* permute the current group of quantisation cells */
		ascending_sort_cells(perm,boxcnt[n]);			/* sort the first 'boxcnt' of the perm, into ascending order */

		for(m=0;m<boxcnt[n];m++)						/* for each random value, multiply it by quantisation val */
			fprintf(stdout,"INFO: %lf\n",qbas[n] + (perm[m] * q));
	}													/* & add to to 1st quantised position in current time-interval */
	fflush(stdout);
}

/****************************** INVERT_NORMD_ENV ******************************/

void invert_normd_env(void)
{	
	invertenv(1.0);
}

/****************************** INVERT_AROUND_PIVOT ******************************/

void invert_around_pivot(void)
{	
	invertenv(number[cnt] * 2.0);
}

/****************************** INVERT_OVER_RANGE ******************************/

void invert_over_range(void)
{	
	invertenv(number[cnt] + number[cnt+1]);
}

/****************************** ENV_SUPERIMPOS ******************************/

void env_superimpos(int inverse,int typ)	/* PRODUCT SHOULD INVOLVE LOGS !! */
{
	double *out, *no1, *no2, tdiff, tratio, vdiff, vhere;
	int n, m, j, cnt2, skipped;
	double thisnval, thismval, lastnval, lastmval, time = 0.0, lastval = 0.0;

	if((out = (double *)malloc((cnt * 2) * sizeof(double)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	if(factor != 0.0) {		  			/* if inserted after time zero, move it to start time */
		for(n=firstcnt;n<cnt;n+=2)
			number[n] += factor;
	}
	if(inverse) {					/* If working with inverse of env, invert it */
		for(n=firstcnt+1;n<cnt;n+=2)
			number[n] = 1.0 - number[n];
	}
	if(number[firstcnt] < number[0]) {
		no1  = &(number[firstcnt]);	/* If inserted env starts BEFORE orig env, 'invert' process */
		no2  = number;
		cnt  = cnt - firstcnt;			/* establish independent counters for each infile */
		cnt2 = firstcnt;
	} else {
		no1  = number;
		no2  = &(number[firstcnt]);
		cnt2 = cnt - firstcnt;			/* establish independent counters for each infile */
		cnt  = firstcnt;
	}
	n = 0;
	m = 0;
	j = 0;
	thisnval = 0.0;
	thismval = 0.0;
	while(no1[n] < no2[0]) {
		out[j++] = no1[n++];
		thisnval = no1[n];
		thismval = no2[1];
		switch(typ) {
		case(MULT):		out[j++] = no1[n++] * no2[1];	break;
		case(ADD):		out[j++] = no1[n++] + no2[1];	break;
		case(SUBTRACT):	out[j++] = no1[n++] - no2[1];	break;
		case(ENVMAX):	
			out[j++] = max(no1[n],no2[1]); 
			n++;
			break;
		}
		if(n >= cnt)
			break;
	}
	while(n < cnt) {				
		if(no1[n] == no2[m]) {		/* brkpnts coincide */
			out[j++] = no1[n++];
			m++;					/* new brkpnt val is product of origs */
			lastnval = thisnval;
			lastmval = thismval;
			thisnval = no1[n];
			thismval = no2[m];
			switch(typ) {
			case(MULT):		out[j++] = no1[n++] * no2[m++];	break;
			case(ADD):		out[j++] = no1[n++] + no2[m++];	break;
			case(SUBTRACT):	out[j++] = no1[n++] - no2[m++];	break;
			case(ENVMAX):	
				out[j++] = max(no1[n],no2[m]);
				n++;
				m++;
				break;
			}
			if(m >= cnt2)			/* If at end of inserted env, break from loop */
				break;
		} else if(no1[n] > no2[m]) { 	/* inserted brkpnt falls before next orig-brkpnt */
			while(no1[n] > no2[m]) {
				time = no2[m];		/* take time from inserted brkpnt */
				tdiff = no1[n] - no1[n-2];
				tratio = (no2[m++] - no1[n-2])/tdiff;
				n++;
				vdiff = no1[n] - no1[n-2];
				vhere = (vdiff * tratio) + no1[n-2];	/* value of orig brkpnt, at this time */
				lastnval = thisnval;
				lastmval = thismval;
				thisnval = vhere;
				thismval = no2[m];
				docross(lastnval,lastmval,thisnval,thismval,time,&j,out,typ);
				out[j++] = time;	
				switch(typ) {
				case(MULT):		out[j++] = vhere * no2[m++];	break;
				case(ADD):		out[j++] = vhere + no2[m++];	break;
				case(SUBTRACT):	out[j++] = vhere - no2[m++];	break;
				case(ENVMAX):	
					out[j++] = max(vhere,no2[m]);	
					m++;
					break;
				}
				n--;				/* remain at same point in orig-brkfile */
				if(m >= cnt2)
					break;			/* If at end of inserted env, break from loop */
			}
		} else {					/* orig-brkpnt falls before next inserted brkpnt */
			while(no2[m] > no1[n]) {
				time = no1[n];		/* take time from orig brkpnt */
				tdiff = no2[m] - no2[m-2];
				tratio = (no1[n++] - no2[m-2])/tdiff;
				m++;
				vdiff = no2[m] - no2[m-2];
				vhere = (vdiff * tratio) + no2[m-2];	/* value of inserted brkpnt, at this time */
				lastnval = thisnval;
				lastmval = thismval;
				thismval = vhere;
				thisnval = no1[n];
				docross(lastnval,lastmval,thisnval,thismval,time,&j,out,typ);
				out[j++] = time;	
				switch(typ) {
				case(MULT):		out[j++] = vhere * no1[n++];	break;
				case(ADD):		out[j++] = vhere + no1[n++];	break;
				case(SUBTRACT):	out[j++] = vhere - no1[n++];	break;
				case(ENVMAX):	
					out[j++] = max(vhere,no1[n]);	
					n++;
					break;
				}
				m--;				/* remain at same point in inserted-brkfile */
				if(n >= cnt) {		/* if it at end of orig file */
					while(m < cnt2) {	  /* calc remaining inserted file points */
						out[j++] = no2[m++];
						switch(typ) {
						case(MULT):		out[j++] = no2[m++] * no1[cnt-1];	break;
						case(ADD):		out[j++] = no2[m++] + no1[cnt-1];	break;
						case(SUBTRACT):	out[j++] = no2[m++] - no1[cnt-1];	break;
						case(ENVMAX):	
							out[j++] = max(no2[m],no1[cnt-1]);
							m++;
							break;
						}
					}
					break;			/* and break from loop */
				}
			}
		}
		if(m >= cnt2)
			break;					/* If at end of inserted env, break from outer loop */
	}
			/* on leaving loop either m > cnt2 or n > cnt */

	while (n < cnt) {				/* if orig brkfile extends beyond inserted file */
		out[j++] = no1[n++];		/* calculate remaining points */
		switch(typ) {
		case(MULT):		out[j++] = no1[n++] * no2[cnt2 - 1];	break;
		case(ADD):		out[j++] = no1[n++] + no2[cnt2 - 1];	break;
		case(SUBTRACT):	out[j++] = no1[n++] - no2[cnt2 - 1];	break;
		case(ENVMAX):	
			out[j++] = max(no1[n],no2[cnt2 - 1]);	
			n++;
			break;
		}
	}
	while (m < cnt2) {				/* if inserted brkfile extends beyond orig file */
		out[j++] = no2[m++];		/* calculate remaining points */
		switch(typ) {
		case(MULT):		out[j++] = no2[m++] * no1[cnt - 1];	break;
		case(ADD):		out[j++] = no2[m++] + no1[cnt - 1];	break;
		case(SUBTRACT):	out[j++] = no2[m++] - no1[cnt - 1];	break;
		case(ENVMAX):	
			out[j++] = max(no2[m],no1[cnt - 1]);	
			m++;
			break;
		}
	}
	fprintf(stdout,"INFO: %lf  %lf\n",out[0],out[1]);
	skipped = 0;
	for(n=2;n<j-2;n += 2) {
		if(!flteq(out[n-1],out[n+1])) {		/* elementary datareduce */
			if(skipped)
				fprintf(stdout,"INFO: %lf  %lf\n",time,lastval);
			fprintf(stdout,"INFO: %lf  %lf\n",out[n],out[n+1]);
			skipped = 0;
		} else {
			skipped = 1;
			time 	= out[n];
			lastval = out[n+1];
		}
	}
	fprintf(stdout,"INFO: %lf  %lf\n",out[n],out[n+1]);
	fflush(stdout);
}

/****************************** DOCROSS ******************************/

void docross(double lastnval,double lastmval,double thisnval, double thismval,double time,int *j,double *out,int typ)
{
	int prehi = 0, posthi = 0;
	double lasttime, timediff, timecross, valcross, gradn, gradm;
	if(lastnval > lastmval)
		prehi = 1;
	else if(lastnval < lastmval)
		prehi = -1;
	if(thisnval > thismval)
			posthi = 1;
	else if(thisnval < thismval)
		posthi = -1;
	if(prehi && posthi && (prehi != posthi)) {	/* curves intersect */
		lasttime = out[*(j)-2];
		timediff = time - lasttime;
		if(flteq(timediff,0.0)) {
			fprintf(stdout,"ERROR: Came across time step too small to calculate.\n");
			fflush(stdout);
			exit(1);
		}
		gradn = (thisnval - lastnval)/timediff;
		gradm = (thismval - lastmval)/timediff;
		if(flteq(gradn,gradm)) {
			fprintf(stdout,"ERROR: possible error in curve crossing algorithm.\n");
			fflush(stdout);
			exit(1);
		}
		timecross = (lastmval - lastnval)/(gradn - gradm);
		valcross = (gradn * timecross) + lastnval;
		out[(*j)++] = timecross + lasttime;
		switch(typ) {
		case(MULT):		out[(*j)++] = (valcross * valcross);	break;
		case(ADD):		out[(*j)++] = (valcross + valcross);	break;
		case(SUBTRACT):	out[(*j)++] = 0.0;						break;
		case(ENVMAX):	out[(*j)++] = valcross;					break;
		}
	}
}

/****************************** ENV_DEL_INV ******************************/

void env_del_inv(void)
{
	int n, m;
	cnt += 2;
	if((number = (double *)realloc((char *)number,cnt * sizeof(double)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	m = cnt - 1;
	n = cnt - 3;
	while(n > 0) {
		number[m--] = 1.0 - number[n--];		/* inverse */
		number[m--]	= number[n--] + factor; 	/* delay */
	}
	number[1] = number[3];						/* extend initial val.. */
	number[0] = 0.0; 							/* ...back to zero time */
	for(n=0;n<cnt;n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[n+1]);
	fflush(stdout);
}

/****************************** ENV_DEL_INV ******************************/

void env_plateau(void)
{
	double plateau;
	int n;
	n = ifactor;
	n--; 					/* get true line number */
	n *= 2;					/* get brkpnt pair */
	n++;					/* get val in brkpnt pair */
	plateau = number[n];	/* get plateau val */
	n -= 2;
	number[n] = plateau;	/* put plateau val in previous val*/
	n--;					
	number[n] = 0.0;		/* set time here to zero */
	while(n < cnt) {
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[n+1]);
		n += 2;
	}
	fflush(stdout);
}

/****************************** TIME_FROM_BEAT_POSITION ******************************/

void time_from_beat_position(int has_offset)
{
	int n;
	double k;
	if(factor <= FLTERR) {
		fprintf(stdout,"ERROR: Beat duration is less than or equal to zero.\n");
		fflush(stdout);
		exit(1);
	}
	if(has_offset && !condit) {
		fprintf(stdout,"ERROR: No time offset given.\n");
		fflush(stdout);
		exit(1);
	}
	for(n=0;n< cnt;n++) {
		if((k = number[n] - 1.0) < 0.0) {
			fprintf(stdout,"ERROR: Position of beat %d is less than 1. Impossible.\n",n+1);
			fflush(stdout);
			exit(1);
		}
		number[n] = k * factor;
		if(has_offset)
			number[n] += thresh;
	}
	for(n=0;n< cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

/****************************** INSERT_AFTER_VAL ******************************/

void insert_after_val(void)
{
	int n, m, has_started = 0;
	for(n=0;n<cnt;n++) {
		if (!has_started) {
			if(number[n] == number[cnt]) {
				for(m=0;m <= n; m++)
					fprintf(stdout,"INFO: %lf\n",number[m]);
				fprintf(stdout,"INFO: %lf\n",number[cnt+1]);
				has_started = 1;
			}
		} else {
			fprintf(stdout,"INFO: %lf\n",number[n]);
		}
	}
	if(!has_started) {
		fprintf(stdout,"ERROR: Value %lf not found in the table.\n",number[cnt]);
		fflush(stdout);
		exit(1);
	}
	fflush(stdout);
}

/****************************** MIN_INTERVAL ******************************/

void min_interval(int ismax) {
	int n, pos;
	double min_int, max_int, this_int;
	if(cnt < 2) {
		fprintf(stdout,"ERROR: Too few values to run this process.\n");
		fflush(stdout);
		exit(1);
	}
	pos = 1;
	if (ismax) {
		max_int = number[1] - number[0];
		for(n=2;n<cnt;n++) {
			if((this_int = number[n] - number[n-1]) > max_int) {
				max_int = this_int;
				pos = n;
			}
		}
		fprintf(stdout,"WARNING: Maximum interval is %lf between entries %d and %d.\n",max_int,pos,pos+1);
		fflush(stdout);
	} else {
		min_int = number[1] - number[0];
		for(n=2;n<cnt;n++) {
			if((this_int = number[n] - number[n-1]) < min_int) {
				min_int = this_int;
				pos = n;
			}
		}
		fprintf(stdout,"WARNING: Minimum interval is %lf between entries %d and %d.\n",min_int,pos,pos+1);
		fflush(stdout);
	}
}

/****************************** INSERT_IN_ORDER ******************************/

void insert_in_order(void) {
	int n, pos = -1;
	for(n=0;n<cnt-1;n++) {
		if(number[n+1] <= number[n]) {
			fprintf(stdout,"ERROR: column not in ascending order.\n");
			fflush(stdout);
			exit(1);
		}
		if ((pos < 0) && (number[cnt] <= number[n])) {	/* position not found & new number goes here */
			pos = n;
		}
	}
	if ((pos < 0) && (number[cnt] <= number[n])) {	/* position not found & new number goes here */
		pos = n;
	}
	if(pos < 0) 			/* new number larger than all in column, goes at end */
		pos = cnt;			
	if(pos > 0) {			/* if new number not at start of column */
		for(n=0;n<pos;n++)
			fprintf(stdout,"INFO: %lf\n",number[n]);
	}					   	/* insert new number */
	fprintf(stdout,"INFO: %lf\n",number[cnt]);
	if(pos < cnt) {			/* if new mumber not at end of column */
		for(n=pos;n<cnt;n++)
			fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}

/****************************** INSERT_AT_START ******************************/

void insert_at_start(void) {
	int n;
	fprintf(stdout,"INFO: %lf\n",number[cnt]);
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

/****************************** INSERT_AT_END ******************************/

void insert_at_end(void) {
	int n;
	for(n=0;n<=cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

/****************************** FORMAT_STRS ****************************/

void format_strs(void)
{
	int n, m, OK = 1;
	double d = (double)stringscnt/(double)ifactor;
	int rowcnt = stringscnt/ifactor;
	char temp[64];
	errstr[0] = ENDOFSTR;
	if(d > (double)rowcnt)
		rowcnt++;
	for(n=0;n<stringscnt;n+=rowcnt) {
		for(m=0;m<rowcnt;m++) {
			if(n!=0 && m==0) {
				fprintf(stdout,"INFO: %s\n",errstr);
				errstr[0] = ENDOFSTR;
			}
			if(n+m < stringscnt) {
				sprintf(temp,"%s ",strings[n+m]);
				strcat(errstr,temp);
			} else
				OK = 0;
		}
		if(!OK)
			break;
	}
	fprintf(stdout,"INFO: %s\n",errstr);
	fflush(stdout);
}


/****************************** COLUMN_FORMAT_STRS ****************************/

void column_format_strs(void)
{	int n, m;

	double d = (double)stringscnt/(double)ifactor;
	int rowcnt = stringscnt/ifactor;
	char temp[64];
	errstr[0] = ENDOFSTR;
	if(d > (double)rowcnt)
		rowcnt++;
	for(n=0;n<rowcnt;n++) {
		for(m=n;m<stringscnt;m+=rowcnt) {
			sprintf(temp,"%s ",strings[m]);
			strcat(errstr,temp);
		}
		fprintf(stdout,"INFO: %s\n",errstr);
		errstr[0] = ENDOFSTR;
	}
	fflush(stdout);
}

/****************************** RANDOM_INTEGERS ****************************/

void random_integers(void)
{
	int n, i, j = round(number[1]);
	int range = abs(j - 1) + 1;
	int rangbot = (int)min(j,1);
	ifactor = round(number[0]);

	for(n=0;n<ifactor;n++) {
		i = (int)floor(drand48() * range) + rangbot;
		fprintf(stdout,"INFO: %d\n",i);
	}
	fflush(stdout);
}

/****************************** RANDOM_INTEGERS_EVENLY_SPREAD ****************************/

void random_integers_evenly_spread(void)
{
	int z, n, m, i, k, j, repets;
	int range, rangbot, arrsiz, endcnt, endval, allowed, checkpart;
	int *arr, *arr2, *perm;

	repets = round(number[2]);
	j = round(number[1]);
	range = abs(j - 1) + 1;
	rangbot = (int)min(j,1);
	arrsiz = range * repets;
	ifactor = round(number[0]);

	if((arr = (int *)malloc(arrsiz * sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	if((perm = (int *)malloc(arrsiz * sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	if((arr2 = (int *)malloc(repets * sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	n = 0;
	for(j=0;j<repets;j++) {					/* fill array with REPET copies of values */
		z = rangbot;
		for(i=0;i<range;i++)
			arr[n++] = z++;
	}
	endcnt = 0;							  	/* number of items repeated at end of previous perm */
	endval = 1;								/* value (possibly repeated) at end of previous perm */
	allowed = -1;							/* number of permissible repetitions of this val at start of NEW perm */
	checkpart = arrsiz - repets;			/* items at end of array to test for repetitions */
	n = 0;
	while(n < ifactor) {
		do_repet_restricted_perm(arr,perm,arrsiz,allowed,endval);
		j = 0;
		for(m = 0;m <arrsiz;m++,n++) {
			if(n >= ifactor)
				break;
			k = arr[perm[m]];
			fprintf(stdout,"INFO: %d\n",k);
			if(m >= checkpart)				/* save last checkable stretch of perm */
				arr2[j++] = k;
		}
		fflush(stdout);
		if(n >= ifactor)
			break;
		j--;
		endval = arr2[j--];					/* note the val at end of perm */
		endcnt = 1;							/* and count it */
		for(k = j; k>= 0; k--) {			
			if(arr2[k] == endval)			/* check adjacent vals, for repetition of value: count */
				endcnt++;
			else							/* if no more repetitions, finish counting */
				break;
		}
		allowed = repets - endcnt;			/* get number of permissible repets at start of next perm */
	}
}

/****************************** DO_REPET_RESTRICTED_PERM ****************************/

void do_repet_restricted_perm(int *arr, int *perm, int arrsiz, int allowed, int endval)
{
	int n, t;
	int checklen = allowed + 1;
	int done = 0;
	while(!done) {
		for(n=0;n<arrsiz;n++) {
			t = (int)(drand48() * (double)(n+1)); /* TRUNCATE */
			if(t==n)
				hhprefix(n,arrsiz,perm);
			else
				hhinsert(n,t,arrsiz,perm);
		}
		if(checklen <= 0)
			break;
		for(n=0;n<checklen;n++) {
			if(arr[perm[n]] == endval) {	/* if this is val (repeated) at end of last perm */
				if(allowed == 0) 			/* if repetition not allowed, force a new perm val */
					break;
				else						/* else, repetitions still allowed */
					allowed--;				/* decrement number of permissible further repets */ 
			} else {						
				done = 1;					/* if this is not val at end of last perm */
				break;						/* perm is OK */
			}			
		}
	}
}

/***************************** HHINSERT **********************************
 *
 * Insert the value m AFTER the T-th element in perm[].
 */

void hhinsert(int m,int t,int setlen,int *perm)
{
	hhshuflup(t+1,setlen,perm);
	perm[t+1] = m;
}

/***************************** HHPREFIX ************************************
 *
 * Insert the value m at start of the permutation perm[].
 */

void hhprefix(int m,int setlen,int *perm)
{			  
	hhshuflup(0,setlen,perm);
	perm[0] = m;
}

/****************************** HHSHUFLUP ***********************************
 *
 * move set members in perm[] upwards, starting from element k.
 */

void hhshuflup(int k,int setlen,int *perm)
{
	int n, *i;
	int z = setlen - 1;
	i = (perm+z);
	for(n = z;n > k;n--) {
		*i = *(i-1);
		i--;
	}
}

/****************************** TIME_FROM_BAR_BEAT_METRE_TEMPO ***********************************/

void time_from_bar_beat_metre_tempo(int has_offset)
{
	int barlen, beatsize, n;
	double tempo,beat,beatdur,time, offset = 0.0;

	if((tempo = get_tempo(strings[cnt+1])) <= 0.0)
		exit(1);
	if(has_offset)
		get_offset(strings[cnt+2],&offset);
	get_metre(strings[cnt],&barlen,&beatsize);
	beatdur = (60.0/(double)tempo) * (4.0/(double)beatsize);
	for(n=0;n<cnt;n++) {
		beat = get_beat(n,barlen);
		time = beat * beatdur;
		time += offset;
		fprintf(stdout,"INFO: %lf\n",time);
	}
	fflush(stdout);
}

/****************************** GET_METRE ***********************************/

void get_metre(char str[],int *barlen,int *beatsize)
{
	int pointcnt = 0, isvalid = 0, mask;
	char *q, *start, *p = str;
	start = p;
	q = start;
	p += strlen(p);
	/* strip trailing zeros */
	if(p == start) {
		fprintf(stdout, "ERROR: Invalid metre value. (No data).\n");
		fflush(stdout);
		exit(1);
	}
	p--;
	while(*p == '0') {
		*p = ENDOFSTR;		
		if(p == start)
			break;
		p--;
	}
	p = start;
	while(*p != ENDOFSTR) {
		if(*p == '.') {
			pointcnt++;
			q = p;
		} else if (!isdigit(*p)) {
			fprintf(stdout, "ERROR: Invalid metre value. non_digit = '%c' in numerator.\n",*p);
			fflush(stdout);
			exit(1);
		}
		p++;
	}
	if(q == start) {
		fprintf(stdout, "ERROR: Invalid metre value. No numerator.\n");
		fflush(stdout);
		exit(1);
	}
	if (pointcnt!=1) {
		if(pointcnt > 1)
			fprintf(stdout, "ERROR: Invalid metre value. %d decimal points : must be one only.\n",pointcnt);
		else
			fprintf(stdout, "ERROR: Invalid metre value. No decimal point\n");
		fflush(stdout);
		exit(1);
	}
	*q = ENDOFSTR;
	if(sscanf(start,"%d",barlen)<1) {							/* SAFETY (redundant) */
		fprintf(stdout, "ERROR: Invalid metre value. Cannot read barlength.\n");
		fflush(stdout);
		exit(1);
	}
	p = q+1;
	if(*p==ENDOFSTR) {
		fprintf(stdout, "ERROR: Invalid metre value. (No denominator).\n");
		fflush(stdout);
		exit(1);
	}
	start = p;
	if(*p == '0') {
		fprintf(stdout, "ERROR: Invalid metre value. (leading zeros in denominator).\n");		/* leading zero(s) */
		fflush(stdout);
		exit(1);
	}
	while(*p != ENDOFSTR) {
		if(!isdigit(*p)) {									 	/* non-numeric characters */
			fprintf(stdout, "ERROR: Invalid metre value. (non-digit character '%c' in denominator )\n",*p);
			fflush(stdout);
			exit(1);
		}
		p++;
	}
	if(sscanf(start,"%d",beatsize)<1) {							/* SAFETY (redundant) */
		fprintf(stdout, "ERROR: Invalid metre value. Failed to read beatsize from %s\n",start);
		fflush(stdout);
		exit(1);
	}
	mask = 1;
	while(mask < 512) {		/* Powers of 2 only !! Need special dispensation for Ferneyhovian metres like 4:10 */
		if((*beatsize) == mask) {
			isvalid = 1;
			break;
		}
		mask <<= 1;
	}	
	if(!isvalid) {
		fprintf(stdout, "ERROR: Invalid metre value. beatsize (%d) is invalid in standard usage\n",*beatsize);
		fflush(stdout);
		exit(1);
	}
}

/****************************** GET_BEAT ***********************************/

double get_beat(int n,int barlen)
{
	int pointcnt = 0;
	char *q = NULL, *p, *start;
	int bar;
	double beat;
	if(strlen(strings[n]) == 0) {
		fprintf(stdout, "ERROR: Invalid bar:beat value at item %d. (No value found)\n",n+1);
		fflush(stdout);
		exit(1);
	}
	p = strings[n];
		/* must have COLON */
	while(*p != ENDOFSTR) {
		if(*p == ':') {
			pointcnt++;
			q = p;
		} else if (!isdigit(*p) && (*p != '.')) {
			pointcnt = -1;
			break;
		}
		p++;
	}
	if(pointcnt != 1) {
		switch(pointcnt) {
		case(-1):
			fprintf(stdout, "ERROR: Invalid character (%c) in bar:beat value '%s' in item %d\n",*p,strings[n],n+1);
			break;
		case(0):
			fprintf(stdout, "ERROR: Invalid bar:beat value '%s' at item %d. (No colon found)\n",strings[n],n+1);
			break;
		default:
			fprintf(stdout, "ERROR: Invalid bar:beat value '%s' at item %d : %d colons found (should be only 1)\n",
			strings[n],n+1,pointcnt);
			break;
		}
		fflush(stdout);
		exit(1);
	}
	*q = ENDOFSTR;
	if(sscanf(strings[n],"%d",&bar)<1) {	/* SAFETY (redundant) */
		fprintf(stdout, "ERROR: Invalid bar:beat value at item %d. Failed to read bar count.\n",n+1);
		fflush(stdout);
		exit(1);
	}
	if(bar < 1) {
		fprintf(stdout, "ERROR: Invalid bar:beat value at item %d. Bar count less than 1.\n",n+1);
		fflush(stdout);
		exit(1);
	}
	bar--;
	start = q+1;
	if(*start==ENDOFSTR) {
		fprintf(stdout, "ERROR: Invalid bar:beat value at item %d. No beatcount.\n",n+1);
		fflush(stdout);
		exit(1);
	}
	if(sscanf(start,"%lf",&beat)<1) {												/* SAFETY (redundant) */
		fprintf(stdout, "ERROR: Invalid bar:beat value at item %d. Failed to read beatcount.\n",n+1);
		fflush(stdout);
		exit(1);
	}
	if(beat < 1.0) {
		fprintf(stdout, "ERROR: Invalid bar:beat value at item %d. Beat count less than 1.\n",n+1);
		fflush(stdout);
		exit(1);
	} else if(beat >= (double)(barlen + 1)) {
		fprintf(stdout, "ERROR: Invalid bar:beat value at item %d. Beat count beyond bar end.\n",n+1);
		fflush(stdout);
		exit(1);
	} 
	beat -= 1.0;
	beat += (bar * (double)barlen);
	return beat;
}

/****************************** SCALE_FROM ***********************************/

void scale_from(void) {

	double interval, pivot = number[cnt];
	double scaler = number[cnt+1];
	int n;
	for(n = 0;n<cnt;n++) {
		interval = number[n] - pivot;
		interval *= scaler;
		fprintf(stdout,"INFO: %lf\n",pivot + interval);
	}
	fflush(stdout);
}

/****************************** SCALE_ABOVE ***********************************/

void scale_above(void) {

	double interval, pivot = number[cnt];
	double scaler = number[cnt+1];
	int n;
	for(n = 0;n<cnt;n++) {
		interval = number[n] - pivot;
		if(interval > 0.0) {
			interval *= scaler;
			fprintf(stdout,"INFO: %lf\n",pivot + interval);
		} else
			fprintf(stdout,"INFO: %lf\n",number[n]);
	}			
	fflush(stdout);
}

/****************************** SCALE_BELOW ***********************************/

void scale_below(void) {

	double interval, pivot = number[cnt];
	double scaler = number[cnt+1];
	int n;
	for(n = 0;n<cnt;n++) {
		interval = number[n] - pivot;
		if(interval < 0.0) {
			interval *= scaler;
			fprintf(stdout,"INFO: %lf\n",pivot + interval);
		} else
			fprintf(stdout,"INFO: %lf\n",number[n]);
	}			
	fflush(stdout);
}

/************************** SPAN_RISE ********************************/

void span_rise(void) {

	int n;

	if(number[0] < 0.0) {
		fprintf(stdout,"ERROR: This option only works with ascending values greater than or equal to zero.\n");
		fflush(stdout);
		exit(1);
	}
	for(n = 1;n<cnt;n++) {
		if(number[n] - number[n-1] <= thresh)  {
			fprintf(stdout,"ERROR: No space for span between number %d (%lf) and number %d (%lf)\n",
			n,number[n-1],n+1,number[n]);
			fflush(stdout);
			exit(1);
		}
	}
	if(number[0] < thresh) {
		fprintf(stdout,"WARNING: First value too close to zero: NO span inserted.\n");
		fflush(stdout);
	} else {
		fprintf(stdout,"INFO: %lf\n",number[0] - thresh);
	}
	fprintf(stdout,"INFO: %lf\n",number[0]);

	for(n = 1;n<cnt;n++) {
		fprintf(stdout,"INFO: %lf\n",number[n] - thresh);
		fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}

/************************** SPAN_FALL ********************************/

void span_fall(void) {

	int n;

	if(number[0] < 0.0) {
		fprintf(stdout,"ERROR: This option only works with ascending values greater than or equal to zero.\n");
		fflush(stdout);
		exit(1);
	}
	for(n = 1;n<cnt;n++) {
		if(number[n] - number[n-1] <= thresh)  {
			fprintf(stdout,"ERROR: No space for span between number %d (%lf) and number %d (%lf)\n",
			n,number[n-1],n+1,number[n]);
			fflush(stdout);
			exit(1);
		}
	}
	for(n = 0;n<cnt;n++) {
		fprintf(stdout,"INFO: %lf\n",number[n]);
		fprintf(stdout,"INFO: %lf\n",number[n] + thresh);
	}
	fflush(stdout);
}

/************************** CYCLIC_SELECT ********************************/

void cyclic_select(char c) {

	int n, m, start, step, k = (int)c, startpos = 0, steppos=0;
	double item = 0.0;
	
	switch(k) {
	case('s'):
		steppos  = cnt;
		startpos = cnt+1;
		break;
	case('a'):
	case('m'):
		steppos  = cnt+1;
		startpos = cnt+2;
		break;
	}
	if((start = (int)round(number[startpos])) > cnt) {
		fprintf(stdout,"ERROR: There are no numbers at or beyond position %d\n",start);
		fflush(stdout);
		exit(1);
	} else if(start < 1) {
		fprintf(stdout,"ERROR: There are no numbers at or before position %d\n",start);
		fflush(stdout);
		exit(1);
	}
	start--;
	if(abs(step = (int)round(number[steppos])) < 1) {
		fprintf(stdout,"ERROR: Step between values cannot be zero.\n");
		fflush(stdout);
		exit(1);
	}
	if(c != 's') {
		if(step < 0) {
			fprintf(stdout,"ERROR: Step between values cannot be negative.\n");
			fflush(stdout);
			exit(1);
		}
		item = number[cnt];
	}
	switch(k) {
	case('s'):
		if(step > 0) {
			for(n = start;n<cnt;n+=step)
				fprintf(stdout,"INFO: %lf\n",number[n]);
		} else {
			for(n = start;n>=0;n+=step)
				fprintf(stdout,"INFO: %lf\n",number[n]);
		}
		break;
	case('a'):
	case('m'):
		n = 0;
		while(n < start)
			fprintf(stdout,"INFO: %lf\n",number[n++]);
		for(m = 0;n<cnt;m++,n++) {
			m %= step;
			if(m == 0) {
				switch(k) {
				case('a'):	fprintf(stdout,"INFO: %lf\n",number[n] + item); break;
				case('m'):	fprintf(stdout,"INFO: %lf\n",number[n] * item); break;
				}
			} else {
				fprintf(stdout,"INFO: %lf\n",number[n]);
			}
		}
		break;
	}
	fflush(stdout);
}

/************************** SPAN_ALL ********************************/

void span_all(void) {

	int n, zero_exists = 0, top_exists = 0;

	if(number[0] < 0.0) {
		fprintf(stdout,"ERROR: Numbers begin before zero.\n");
		fflush(stdout);
		exit(1);
	}
	if(number[cnt-1] > factor) {
		fprintf(stdout,"ERROR: Numbers already run beyond %lf\n",factor);
		fflush(stdout);
		exit(1);
	}
	if(number[0] <= 0.0)
		zero_exists = 1;
	if(number[cnt-1] >= factor)
		top_exists = 1;
	if(zero_exists) {
		if(top_exists) {
			fprintf(stdout,"ERROR: Numbers already begin at zero and end at %lf\n",factor);
			fflush(stdout);
			exit(1);
		} else {
			fprintf(stdout,"WARNING: Numbers already start at zero\n");
			fflush(stdout);
		}
	}
	if(top_exists) {
		fprintf(stdout,"WARNING: Numbers already end at %lf\n",factor);
		fflush(stdout);
	}
	if(!zero_exists)
		fprintf(stdout,"INFO: %lf\n",0.0);
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	if(!top_exists)
		fprintf(stdout,"INFO: %lf\n",factor);
	fflush(stdout);
}

/************************** GET_TEMPO ********************************/

double get_tempo(char *str)
{
	char *p = str;
	double tempo;
	int pointcnt = 0;
	while(*p != ENDOFSTR) {
		if(*p == '.') {
			pointcnt++;
		} else if(!isdigit(*p)) {
			pointcnt = -1;
			break;
		}
		p++;
	}
	if(pointcnt < 0 || pointcnt > 1) {
		fprintf(stdout, "ERROR: Invalid tempo value.\n");
		fflush(stdout);
		return(-1.0);
	}
	if(sscanf(str,"%lf",&tempo)!=1) {
		fprintf(stdout, "ERROR: Invalid tempo value.\n");
		fflush(stdout);
		return(-1.0);
	}
	if(tempo <= 0.0) {
		fprintf(stdout, "ERROR: Zero or negative tempo: impossible.\n");
		fflush(stdout);
		return(-1.0);
	} else if(tempo > MAX_TEMPO) {
		fprintf(stdout, "ERROR: Invalid tempo value. Beats shorter than 1 millisecond (!!).\n");
		fflush(stdout);
		return(-1.0);
	} else if(tempo < MIN_TEMPO) {
		fprintf(stdout, "ERROR: Invalid tempo value. Beats longer than 1 hour (!!).\n");
		fflush(stdout);
		return(-1.0);
	}
	return tempo;
}

/************************** GENERATE_RANDOMISED_VALS ********************************/

void generate_randomised_vals(void)
{
	double scatter = number[2], span = number[0], sum = 0.0, mean, range, *temp = NULL, d;
	int cnt = (int)round(number[1]), n, m, j, subcnt;
	int bigscat = 0;
	if(scatter > 1.0)
		bigscat = (int)round(scatter);
	if(bigscat) {
		if((temp = (double *)malloc(bigscat * sizeof(double)))==NULL) {
			fprintf(stdout,"Out of memory.\n");
			fflush(stdout);
			exit(1);
		}
	}
	if((number = (double *)realloc((char *)number,(cnt+1) * sizeof(double)))==NULL) {
		fprintf(stdout,"Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	mean = span/(double)cnt;
	number[0] = 0.0;
	number[cnt] = span;

	if(bigscat) {
		for(n=1;n < cnt;n+= bigscat) {
			if((subcnt = n + bigscat) > cnt)			/* find position of last number in this pass */
				subcnt = cnt;	  						/* set end position of pass */
			subcnt--;									/* allow for item already written at 1 */
			if((subcnt %= bigscat) == 0)				/* set size of pass */
				subcnt = bigscat;
			range = mean * subcnt;						/* set range of pass */
			for(m = 0; m < subcnt; m++)
				temp[m] = sum + (drand48() * range);	/* generate values within this range */
			for(m=0;m < subcnt - 1; m++) {				
				for(j=1;j < subcnt; j++) {
					if(temp[m] > temp[j]) {				/* sort */
						d = temp[j];
						temp[j] = temp[m];
						temp[m] = d;
					}
				}
			}
			for(m=0;m<subcnt;m++)						/* concatenate to list of numbers */
				number[n+m] = temp[m];
			sum += range;								/* step over range */
		}
	} else {
		for(n=1;n < cnt;n++) {
			sum += mean;
			number[n] = sum + (mean * randoffset(scatter));
		}
	}
	for(n=0;n<=cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

/************************** RAND_OFFSET ********************************
 *
 * rand number in maximal range -half to +half
 */

double randoffset(double scatter)
{
	return (((drand48() * 2.0) - 1.0) * 0.5) * scatter;
}

/************************** GET_OFFSET ********************************/

void get_offset(char *str,double *offset)
{
	if(sscanf(str,"%lf",offset)!=1) {
		fprintf(stdout,"Cannot read time offset.\n");
		fflush(stdout);
		exit(1);
	}
}

/************************** PITCH_TO_DELAY ********************************/

void pitch_to_delay(int midi)
{
	int n;
	for(n = 0;n < cnt; n++) {
		if(midi) {
			if(number[n] < MIDIMIN || number[n] > MIDIMAX) {
				fprintf(stdout,"MIDI value %d (%lf) is out of range.\n", n+1,number[n]);
				fflush(stdout);
				exit(1);
			}
			number[n] = miditohz(number[n]);
		} else if(number[n] < FLTERR || number[n] > 24000.0) {
			fprintf(stdout,"Frequency value %d (%lf) is out or range.\n",n+1,number[n]);
			fflush(stdout);
			exit(1);
		}
		number[n] = 1000.0/number[n];
	}
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

/************************** DELAY_TO_PITCH ********************************/

void delay_to_pitch(int midi)
{
	int n;
	for(n = 0;n < cnt; n++) {
		number[n] /= 1000; 
		if(number[n] < FLTERR) {
			fprintf(stdout,"Delay value %d (%lf) is out or range for conversion to pitch value.\n",n+1,number[n]);
			fflush(stdout);
			exit(1);
		}
		number[n] = 1.0/number[n];
		if(midi) {
			if(number[n] < MIDIMINFRQ || number[n] > MIDIMAXFRQ) {
				fprintf(stdout,"delay value %d (%lf) is out of range for conversion to MIDI.\n", n+1,number[n]);
				fflush(stdout);
				exit(1);
			}
			number[n] = hztomidi(number[n]);
		} else if(number[n] < FLTERR || number[n] > 24000) {
			fprintf(stdout,"Delay value %d (%lf) is out or range for conversion to frq.\n",n+1,number[n]);
			fflush(stdout);
			exit(1);
		}
	}
	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}


/* NEW FUNCS HANDLING BRKTABLES DIRECTLY ***** JUNE 2000 ***/


/****************************** REVERSE_TIME_INTERVALS ******************************/

void reverse_time_intervals(void)
{
	int n, m;
	double k = number[0];
	fprintf(stdout,"INFO: %lf  %lf\n",k,number[1]);
	for(n=3,m = cnt-2;n<cnt;m-=2,n+=2) {
		k += (number[m] - number[m-2]);
		fprintf(stdout,"INFO: %lf  %lf\n",k,number[n]);
	}
	fflush(stdout);
}

/****************************** REVERSE_ORDER_OF_BRKVALS ******************************/

void reverse_order_of_brkvals(void)
{
	int n, m;
	for(n=0,m = cnt-1;n<cnt;m-=2,n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[m]);
	fflush(stdout);
}

/****************************** INVERTENV ******************************/

void invertenv(double piv)
{	
	int n;
	for(n=1;n<cnt;n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n-1],piv - number[n]);
	fflush(stdout);
}

/******************************** THRESH_CUT ****************************/

void thresh_cut(void)
{
	int n,m, k = 0;
	double ratio, *time;
	int isgreater = 0;
	if((cnt <= 0) || ((time = (double *)malloc(cnt * sizeof(double)))==NULL)) {
		fprintf(stdout,"ERROR: Insufficient memory.\n");
		return;
	}
	if(number[1] > factor)
		isgreater = 1;
	for(m=2,n=3;n<cnt;n+=2,m+=2) {
		switch(isgreater) {
		case(0):			
			if(number[n] > factor) {
				ratio = (factor - number[n-2])/(number[n] - number[n-2]);
				time[k++] = ((number[m] - number[m-2]) * ratio) + number[m-2];
				isgreater = 1;
			}
			break;
		case(1):			
			if(number[n] <= factor) {
				ratio = (factor - number[n-2])/(number[n] - number[n-2]);
				time[k++] = ((number[m] - number[m-2]) * ratio) + number[m-2];
				isgreater = 0;
			}
			break;
		}
	}
	if(k == 0) {
		fprintf(stdout,"ERROR: The values do not cross the threshold.\n");
		free(time);
		return;
	}
	for(n=0;n<k;n++)
		do_valout(time[n]);
	free(time);
}

/******************************** BAND_CUT ****************************/

void band_cut(void)
{
	int n,m, k = 0;
	double ratio, *time, z, hibnd, lobnd;
	int bandpos;
	if((cnt <= 0) || ((time = (double *)malloc(cnt * sizeof(double)))==NULL)) {
		fprintf(stdout,"ERROR: Insufficient memory.\n");
		return;
	}
	lobnd = number[cnt];
	hibnd = number[cnt+1];

	if(lobnd > hibnd) {
		z = lobnd;
		lobnd = hibnd;
		hibnd = z;
	}
	if(number[1] >= lobnd && number[1] <= hibnd)
		bandpos = 0;
	else if(number[1] < lobnd)
		bandpos = -1;
	else
		bandpos = 1;
	for(m=2,n=3;n<cnt;n+=2,m+=2) {
		switch(bandpos) {
		case(0):			
			if(number[n] < lobnd) {		   /* crosses out downwards */
				ratio = (lobnd - number[n-2])/(number[n] - number[n-2]);
				time[k++] = ((number[m] - number[m-2]) * ratio) + number[m-2];
				bandpos = -1;				   
			} else if(number[n] > hibnd) {	   /* crosses out upwards */
				ratio = (hibnd - number[n-2])/(number[n] - number[n-2]);
				time[k++] = ((number[m] - number[m-2]) * ratio) + number[m-2];
				bandpos = 1;
			}
			break;
		case(1):			
			if(number[n] <= hibnd) {	   		/* crosses in from above */
				ratio = (hibnd - number[n-2])/(number[n] - number[n-2]);
				time[k++] = ((number[m] - number[m-2]) * ratio) + number[m-2];
				bandpos = 0;
			}
			if(number[n] < lobnd) {			/* then possibly out below */
				ratio = (lobnd - number[n-2])/(number[n] - number[n-2]);
				time[k++] = ((number[m] - number[m-2]) * ratio) + number[m-2];
				bandpos = -1;
			}
			break;
		case(-1):			
			if(number[n] >= lobnd) {		   /* crosses in from below */
				ratio = (lobnd - number[n-2])/(number[n] - number[n-2]);
				time[k++] = ((number[m] - number[m-2]) * ratio) + number[m-2];
				bandpos = 0;
			}
			if(number[n] > hibnd) {			/* then possibly out above */
				ratio = (hibnd - number[n-2])/(number[n] - number[n-2]);
				time[k++] = ((number[m] - number[m-2]) * ratio) + number[m-2];
				bandpos = 1;
			}
			break;
		}
	}
	if(k == 0) {
		fprintf(stdout,"ERROR: The values in the 2nd column do not cross into or out of the specified band.\n");
		free(time);
		return;
	}
	for(n=0;n<k;n++)
		do_valout(time[n]);
	fflush(stdout);
	free(time);
}



/****************************** ENV_APPEND ******************************/

void env_append(void)
{
	int n, dojoin = 0;
	double first_endtime;

	first_endtime = number[firstcnt-2];
	if (factor < first_endtime) { 
		fprintf(stdout,"ERROR: Second envelope starts before first one ends.\n");
		fflush(stdout);
		exit(1);
	} else if(flteq(factor,first_endtime)) {
		if(!flteq(number[firstcnt-1],number[firstcnt+1])) {
			fprintf(stdout,"ERROR: Abutting envelopes are not at same level (%lf and %lf).\n",number[firstcnt-1],number[firstcnt+1]);
			fflush(stdout);
			exit(1);
		}
		dojoin = 1;
	}
	for(n = firstcnt; n < cnt; n+=2) {
		number[n] += factor;
	}
	if(dojoin) {
		for(n=0;n<firstcnt;n+=2)
			do_valpair_out(number[n],number[n+1]);
		for(n=firstcnt+2;n<cnt;n+=2)
			do_valpair_out(number[n],number[n+1]);
	} else {
		for(n=0;n<cnt;n+=2)
			do_valpair_out(number[n],number[n+1]);
	}
	fflush(stdout);
}

/****************************** ABUTT ******************************/

void abutt(void)
{
	int n,m = 0, i;
	double displace;
	int indx = 0;
	for(i=0;i<infilecnt-1;i++) { 			/* check abutting values match */
		indx += file_cnt[i];
		if(!flteq(number[indx-1],number[indx+1])) {
			fprintf(stdout,"ERROR: Abutting values between files %d and %d do not match\n",i+1,i+2);
			fflush(stdout);
			exit(1);
		}
	}
	for(n=0;n < file_cnt[0]; n+=2) { 		/* print all of file 1 */
		fprintf(stdout, "INFO: %lf  %lf\n",number[m],number[m+1]);
		m += 2;
	}
	displace = number[m-2];		 			/* displace by last time in 1st file */
	for(i = 1; i <infilecnt; i++) {			/* for all other files */
		m += 2;								/* skip first (abutting) value */
		for(n=2;n < file_cnt[i]; n+=2) { 	/* for all other values in file - displace time values */
			fprintf(stdout, "INFO: %lf  %lf\n",number[m] + displace,number[m+1]);
			m += 2;
		}
		displace += number[m-2];			/* increase displacement by last time in this file */
	}
	fflush(stdout);
}

/****************************** QUANTISE_TIME ******************************/

void quantise_time(void)
{
	int n;
	for(n=0;n<cnt;n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",round(number[n]/factor) * factor,number[n+1]);
	fflush(stdout);
}

/****************************** QUANTISE_VAL ******************************/

void quantise_val(void)
{
	int n;
	for(n=0;n<cnt;n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],round(number[n+1]/factor) * factor);
	fflush(stdout);
}

/****************************** EXPAND_TABLE_DUR_BY_FACTOR ******************************/

void expand_table_dur_by_factor(void)
{
	int n;
	for(n=0;n<cnt;n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n] * factor,number[n+1]);
	fflush(stdout);
}

/****************************** EXPAND_TABLE_VALS_BY_FACTOR ******************************/

void expand_table_vals_by_factor(void)
{
	int n;
	for(n=0;n<cnt;n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[n+1] * factor);
	fflush(stdout);
}

/****************************** EXPAND_TABLE_TO_DUR ******************************/

void expand_table_to_dur(void)
{
	int n;
	double ratio;

	if(number[cnt-2] <= 0.0) {
		fprintf(stdout,"ERROR: Final time in table is zero: cannot proceed.\n");
		return;
	}
	ratio = factor/number[cnt-2];

	for(n=0;n<cnt;n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n] * ratio,number[n+1]);
	fflush(stdout);
}

/****************************** CUT_TABLE_AT_TIME ******************************/

void cut_table_at_time(void)
{
	int n;
	double timediff,valdiff,timeratio,outval;

	for(n=0;n<cnt;n+=2) {
		if(flteq(number[n],factor)) {
			fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[n+1]);
			break;
		} else if(number[n] < factor) {
			fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[n+1]);
		} else if(n==0) {
			fprintf(stdout,"WARNING: No values occur before the cut-off time\n");
			break;
		} else {
			timediff  = number[n] - number[n-2];
			valdiff   = number[n+1] - number[n-1];
			timeratio = (factor - number[n-2])/timediff;
			valdiff  *= timeratio;
			outval	= number[n-1] + valdiff;
			fprintf(stdout,"INFO: %lf  %lf\n",factor,outval);
			break;
		}
	}
	fflush(stdout);
}

/****************************** EXTEND_TABLE_TO_DUR ******************************/

void extend_table_to_dur(void)
{
	int n;
 	for(n=0;n<cnt;n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[n+1]);
	fprintf(stdout,"INFO: %lf  %lf\n",factor,number[n-1]);
	fflush(stdout);
}

/****************************** LIMIT_TABLE_VAL_RANGE ******************************/

void limit_table_val_range(void)
{
	int n;
 	if(condit) {
		for(n=1;n<cnt;n+=2) {
			number[n] = max((number[n] = min(factor,number[n])),thresh);
			fprintf(stdout,"INFO: %lf  %lf\n",number[n-1],number[n]);
		}
	} else {
		for(n=1;n<cnt;n+=2) {
			number[n] = min(factor,number[n]);
			fprintf(stdout,"INFO: %lf  %lf\n",number[n-1],number[n]);
		}
	}
	fflush(stdout);
}

/****************************** SUBSTITUTE ******************************/

void substitute(void)
{
	int n;
 	for(n=0;n<cnt;n++) {
		if(flteq(number[n],thresh))
			number[n] = factor;
		fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}

/****************************** SUBSTITUTE_ALL ******************************/

void substitute_all(void)
{
	int n;
 	for(n=0;n<stringscnt;n++)
		fprintf(stdout,"INFO: %s\n",string);
	fflush(stdout);
}

/****************************** SUBSTITUTE ******************************/

void mean_of_reversed_pairs(void)
{
	int n;
	double z1;
 	for(n=1;n<cnt;n++) {
		if(number[n]<number[n-1]) {
			z1 = (number[n] + number[n-1])/2.0;
			number[n-1]	= z1 - FLTERR;
			if((n-2 > 0) && number[n-2] >= number[n-1]) {
				fprintf(stdout,"WARNING: numbers %d (%lf) and %d (%lf) failed to be separated.\n",
				n,number[n-1],n+1,number[n]);
				fflush(stdout);
				return;
			}
			number[n]   = z1 + FLTERR;
			if((n+1 < cnt) && number[n+1] <= number[n]) {
				fprintf(stdout,"WARNING: numbers %d (%lf) and %d (%lf) failed to be separated.\n",
				n+1,number[n],n+2,number[n+1]);
				fflush(stdout);
				return;
			}
		}
	}
 	for(n=0;n<cnt;n++)
		fprintf(stdout,"INFO: %lf\n",number[n]);
	fflush(stdout);
}

/****************************** CONVERT_SPACE_TEX_TO_PAN ******************************/

void convert_space_tex_to_pan(void)
{
	int n;
	for(n=1;n<cnt;n+=2) {
		number[n] *=2.0;
		number[n] -=1.0;
	}
 	for(n=0;n<cnt;n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[n+1]);
	fflush(stdout);
}

/****************************** CONVERT_SPACE_PAN_TO_TEX ******************************/

void convert_space_pan_to_tex(void)
{
	int n;
	for(n=1;n<cnt;n+=2) {
		number[n] *=0.5;
		number[n] +=0.5;
	}
 	for(n=0;n<cnt;n+=2)
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[n+1]);
	fflush(stdout);
}

/****************************** CONVERT_TO_EDITS ******************************/

void convert_to_edits(void)
{
	int n;
	double start, end;
	for(n=0;n<cnt-1;n++) {
		start = max(0.0,number[n] - factor);
		end = max(0.0,number[n+1] + factor);
		fprintf(stdout,"INFO: %lf  %lf\n",start,end);
	}
	fflush(stdout);
}

/****************************** COSIN_SPLINE ******************************/

void cosin_spline(void)
{
	int n, cnt_less_one;
	double val, valchange, skew, startval ,endval;
	skew = number[3];
	startval = number[1];
	endval   = number[2];
	valchange = endval - startval;
	cnt_less_one = cnt - 1;
	if(flteq(skew,1.0)) {
		for(n=0;n<cnt;n++) {
			val  = ((double)n/(double)cnt_less_one) * PI;
			val  = cos(val);
			val += 1.0;
			val /= 2.0;
			val  = 1.0 - val;
			val  = max(0.0,val);
			val  = min(val,1.0);
			val *= valchange;
			val += startval;
			fprintf(stdout,"INFO: %lf\n",val);
		}
	} else {
		for(n=0;n<cnt;n++) {
			val  = ((double)n/(double)cnt_less_one);	/* val in 0 -1 range */
			val  = pow(val,skew);						/* val skewed, still in 0-1 range */
			val  = val * PI;							/* (skewed) val in range 0 to PI */
			val  = cos(val);							/* cosin val running from 1 to -1 */
			val += 1.0;									/* cosin val running from 2 to 0  */	
			val /= 2.0;									/* cosin val running from 1 to 0  */	
			val  = 1.0 - val;							/* cosin val running from 0 to 1  */	
			val  = max(0.0,val);						/* ensure 0-1 range not exceeeded */
			val  = min(val,1.0);						
			val *= valchange;							/* apply cosin shape to val change */
			val += startval;							/* add cosin change to initial val */
			fprintf(stdout,"INFO: %lf\n",val);
		}
	}
	fflush(stdout);
}

/****************************** DISTANCE_FROM_GRID ******************************/

void distance_from_grid()
{
	int n, m, besterror;
	double *diff = (double *)exmalloc((cnt-1)*sizeof(double));
	double *error = (double *)exmalloc(21 * sizeof(double));
	double maxdiff = 0.0, mindiff = 0.0, lastmindiff, diffrange, diffstep, thisdiff, thisgrid, minerror;
	double lowlimit = FLTERR/10000;

	for(n=0;n<cnt-1;n++) {
		diff[n] = number[n+1] - number[n];

/*
		if(diff[n]  <= 0.0) {
			fprintf(stdout, "ERROR: Process only works with increasing sequences of numbers.\n");
			fflush(stdout);
			exit(1);
		}
*/
		if(n==0) {
			mindiff = diff[0];
			maxdiff = diff[0];
		} else {
			mindiff = min(diff[n],mindiff);
			maxdiff = max(diff[n],maxdiff);
		}
	}

	lastmindiff = mindiff;
	while((diffrange = maxdiff - mindiff) > lowlimit * 20.0) {
		diffstep = diffrange/20.0;
		thisdiff = mindiff;
		for(m=0;m<=20;m++) {
			error[m] = 0;
			thisgrid = number[0];
			for(n=1;n<cnt;n++) {
				thisgrid += thisdiff;
				error[m] += fabs(number[n] - thisgrid);
			}
			thisdiff += diffstep;
		}
		minerror = error[0];
		besterror = 0;
		for(m=1;m<=20;m++) {
			if(minerror > error[m]) {
				minerror = error[m];
				besterror = m;
			}
		}
		mindiff = mindiff + (besterror * diffstep);
		lastmindiff = mindiff;
		maxdiff = mindiff + diffstep;
		mindiff -= diffstep;
	}
	thisgrid = number[0];
	for(n=0;n<cnt;n++) {
		thisdiff = number[n] - thisgrid;
		if(thisdiff < 0.0 && thisdiff > -FLTERR)
			thisdiff = 0.0;
		fprintf(stdout,"INFO: %lf\n",thisdiff);
		thisgrid += lastmindiff;
	}
	fflush(stdout);
}

/****************************** SINUSOID ******************************/

void sinusoid(void) {
	double maxval  = number[0], minval  = number[1];
	double range   = maxval - minval;
	double phase   = (fmod(number[2],360.0)/360.0) * TWOPI;
	double periods = number[3];
	double valdens = number[4];
	int n, valcnt = (int)floor((periods * valdens) + 1.0);
	double val, phasestep = TWOPI/valdens;

	for(n = 0; n < valcnt; n++) {
		val = (sin(phase) + 1.0)/2.0;
		val *= range;
		val += minval;
		fprintf(stdout,"INFO: %lf\n",val);
		phase = phase + phasestep; /* should be fmod by TWOPI, but seems to work without this */
	}
	fflush(stdout);
}

/****************************** RAND_INTS_WITH_FIXED_ENDS ****************************/

void rand_ints_with_fixed_ends(void)
{
	int z, n, m, i, k, j, j1, j2, repets, fullperms, partperm, startval, finval;
	int range, rangbot, arrsiz, endcnt, endval, allowed, checkpart, done = 0;
	int *arr, *arr2, *perm;

	repets = round(number[5]);
	startval = round(number[3]);
	finval   = round(number[4]);
	j1 = round(number[1]);
	j2 = round(number[2]);
	range = abs(j2 - j1) + 1;
	rangbot = (int)min(j1,j2);
	arrsiz = range * repets;
	ifactor = round(number[0]) - 2;
	fullperms = ifactor / arrsiz;
	partperm = ifactor - (fullperms * arrsiz);
	if(partperm == 0) {
		fullperms--;		/* The last set of vals has to be calculated separately */
		partperm = arrsiz;	/* as, unlike others, it will have to be compared with the finval */
	}

	if((arr = (int *)malloc(arrsiz * sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	if((perm = (int *)malloc(arrsiz * sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	if((arr2 = (int *)malloc(repets * sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	n = 0;
	for(j=0;j<repets;j++) {					/* fill array with REPET copies of values. */
		z = rangbot;						/* this set can be permd AS A WHOLE, as repet adjacent copies of any val */
		for(i=0;i<range;i++)				/* which might arise in perming this set, are allowed */
			arr[n++] = z++;
	}
	endcnt = 0;							  	/* number of items repeated at end of previous perm */
	endval = startval;						/* value (possibly repeated) at end of previous perm */
											/* initially this is just the 'startval' fixed by the user */
	allowed = repets - 1;					/* number of permissible repetitions of this val at start of NEW perm */
	checkpart = arrsiz - repets;			/* items at end of array to test for repetitions */
	n = 0;
	fprintf(stdout,"INFO: %d\n",startval);	/* Output user-specified first value */
	while(n < fullperms) {
		do_repet_restricted_perm(arr,perm,arrsiz,allowed,endval);
		j = 0;
		for(m = 0;m <arrsiz;m++) {
			k = arr[perm[m]];
			fprintf(stdout,"INFO: %d\n",k);
			if(m >= checkpart)				/* save last checkable stretch of perm */
				arr2[j++] = k;
		}
		fflush(stdout);
		j--;
		endval = arr2[j--];					/* note the val at end of perm */
		endcnt = 1;							/* and count it */
		for(k = j; k>= 0; k--) {			
			if(arr2[k] == endval)			/* check adjacent vals, for repetition of value: count */
				endcnt++;
			else							/* if no more repetitions, finish counting */
				break;
		}
		allowed = repets - endcnt;			/* get number of permissible repets at start of next perm */
		n++;
	}
	k = partperm - 1;	/* index of last item of next perm which will actually be outputted */
	j = repets - 1;		/* How many items at end of partperm, other than item k, which need to be checked for value-repetition */
	while(!done) {
		do_repet_restricted_perm(arr,perm,arrsiz,allowed,endval);
		for(n=k;n>=k - j;n--) {
			if(arr[perm[n]] == finval) {	/* Check end vals of the-set-of-values-in-the-final-perm-which-will-actually-be-outputted */
				if(allowed == 0) 			/* against 'finval', to avoid too many value-repetitions at end of output */
					break;
				else
					allowed--;
			} else {						
				done = 1;
				break;
			}			
		}
	}
	for(m = 0;m <partperm;m++) {
		k = arr[perm[m]];
		fprintf(stdout,"INFO: %d\n",k);
	}
	fprintf(stdout,"INFO: %d\n",finval);
	fflush(stdout);
}

/****************************** RAND_ZIGS ****************************/

void rand_zigs(void)
{
	int n, k, j, j1, j2, finval, range, rangbot, arrsiz, outvals, lastval, t, done;
	int *arr, *perm;

	outvals = round(number[0]);
	j1 = round(number[1]);
	j2 = round(number[2]);
	finval = round(number[3]);
	range = abs(j2 - j1) + 1;
	rangbot = (int)min(j1,j2);
	if(finval < rangbot || finval > (int)max(j1,j2)) {
		fprintf(stdout,"ERROR: Final value specified does not lie within the range of values specified.\n");
		fflush(stdout);
		exit(1);
	}
	arrsiz = range;
	if((arr = (int *)malloc(arrsiz * sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	if((perm = (int *)malloc(arrsiz * sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	k = rangbot;						/* this set can be permd AS A WHOLE, as repet adjacent copies of any val */
	for(n=0;n<range;n++)				/* which might arise in perming this set, are allowed */
		arr[n] = k++;
	j = 0;
	lastval = rangbot - 1;
	done = 0;
	while(j < outvals) {
		for(n=0;n<arrsiz;n++) {
			t = (int)(drand48() * (double)(n+1)); /* Do Perm */
			if(t==n)
				hhprefix(n,arrsiz,perm);
			else
				hhinsert(n,t,arrsiz,perm);
		}
		for(n=0;n<arrsiz;n++) {
			if (lastval < arr[perm[n]])  {
				k = lastval + 1;
				while(k < arr[perm[n]]) {
					fprintf(stdout,"INFO: %d\n",k);
					if(++j >= outvals) {
						lastval = k;
						done = 1;
						break;
					}
					k++;
				}
			} else if(lastval > arr[perm[n]]) {
				k = lastval - 1;
				while(k > arr[perm[n]]) {
					fprintf(stdout,"INFO: %d\n",k);
					if(++j >= outvals) {
						lastval = k;
						done = 1;
						break;
					}
					k--;
				}
			} else {		/* next perm val can only be equal to previous at join of two different perms */
				break;		/* in this case, get a different perm */
			}
			if(done) {
				break;
			}
			lastval = arr[perm[n]];
			fprintf(stdout,"INFO: %d\n",lastval);
			j++;
		}
	}
	if(lastval < finval) {
		lastval++;
		while(lastval <= finval) {
			fprintf(stdout,"INFO: %d\n",lastval);
			lastval++;
		}
	} else if(lastval > finval) {
		lastval--;
		while(lastval >= finval) {
			fprintf(stdout,"INFO: %d\n",lastval);
			lastval--;
		}
	}
	fflush(stdout);
}

/************************** ELIMINATE_DUPLTEXT *************************/

void eliminate_dupltext(void)
{
	int m,n,k;
	for(n=0;n<stringscnt-1;n++) {
		for(m=n+1;m<stringscnt;m++) {
			if(!strcmp(strings[n],strings[m])) {
				for(k = m;k < stringscnt-1; k++)
					strings[k] = strings[k+1];
				m--;
				stringscnt--;
			}
		}
	}
	for(n=0; n < stringscnt;n++)
		do_stringout(strings[n]);
	fflush(stdout);
}

/************************** RANDOM_WARP *************************/

void random_warp(void)
{
	int n, wcnt;
	float *warpvals, *number2;
	double lastsum, diff, warp, dummy;
	FILE *fp;
	char *p;
	arraysize = 100;
	if((warpvals = (float *)malloc(arraysize * sizeof(float)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		return;
	}
	if((fp = fopen(string,"r"))==NULL) {
		sprintf(errstr,"Cannot open infile %s\n",string);
		do_error();
	}
	wcnt = 0;
	while(fgets(temp,20000,fp)!=NULL) {
		p = temp;
		while(strgetfloat(&p,&dummy)) {
			warpvals[wcnt] = (float)dummy;
			if(++wcnt >= arraysize) {
				arraysize += BIGARRAY;
				if((number2=(float *)malloc(arraysize*sizeof(float)))==NULL) {
					sprintf(errstr,"Out of memory for more warp values at %d numbers\n",cnt);
					do_error();
				}
				memcpy((void *)number2,(void *)warpvals,cnt * sizeof(float));
				warpvals = number2;
			}
		}
	}
	fclose(fp);
	if(wcnt ==0 || (wcnt & 1)) {
		sprintf(errstr,"Invalid or missing warp data.\n");
		do_error();
	}
	lastsum = number[0];
	do_valout(lastsum);
	for(n=1;n<cnt;n++) {
		diff = number[n] - number[n-1];
		warp = readbrk(warpvals,number[n],wcnt);
		warp = (((drand48() * 2.0) - 1.0) * warp) + 1.0;
		diff *= warp;
		lastsum += diff;
		do_valout(lastsum);
	}
}

double readbrk(float *warpvals,double time,int wcnt)
{
	int n, got = 0;
	double wlasttime = 0.0, wlastval=0.0, wthistime=0.0, wthisval=0.0, val;
	double timeratio, valdiff;
	for(n = 0; n< wcnt; n+= 2) {
		if(warpvals[n] < time) {
			wlasttime = warpvals[n]; 
			wlastval  = warpvals[n+1]; 
		} else {
			wthistime = warpvals[n];
			wthisval  = warpvals[n+1]; 
			got = 1;
			break;
		}
	}
	if(!got)
		return (double)warpvals[wcnt - 1];
	valdiff   = wthisval  - wlastval;
	timeratio = (time - wlasttime)/(wthistime - wlasttime);
	val = (valdiff * timeratio) + wlastval;
	return val;
}

