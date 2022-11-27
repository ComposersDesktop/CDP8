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



/* columns */

#include <columns.h>

#define CALCLIM   		0.001
#define M 			7
#define NSTACK 			50
#define	MIDDLE_C_MIDI_OCTAVE	5
#define CONVERT_LOG10_TO_LOG2	(double)3.321928
#define LOW_A   		6.875	   /* Frequency of A below MIDI 0  */



double 	cntevents(double,double,double);
int 	samesize(double,double,double,double,double);
void 	approxtimes(double,double,int);
void 	getetime(double,double,double,double,int);
double 	gethibnd(double,double,double,double,double);
double 	getlobnd(double,double,double,double,double);
double 	refinesize(double,double,double,double,double,double);
void	prnt_pitchclass(int,int);
int		strgetfloat_db(char **,double *);
static void bublsort(void);
static void test_warp_data(void);

int		*cntr;
char 	**strings = 0;
char	*stringstore;
int		stringscnt = 0;
int	stringstoresize = 0, stringstart = 0;

/************************ TIMEVENTS ******************************
 *
 * 	Generates event positions from startsize and end size
 *
 *	Let start-time be T0 and end time be T1
 *	Let start size be S0 and end S1
 *
 *	number of event is given by :-
 *
 *		N = (T1-T0) log S1
 *		    -------    e__
 *		    (S1-S0)     S0
 *
 *	In general this will be non-integral, and we should
 *	round N to an integer, and recalculate S1 by successive 
 *	approximation.
 *
 *	Then positions of events are given by
 *	
 *	    for X = 0 to N		 (S1-S0)
 *					  ----- X
 *	T = (S1T0 - S0T1)   S0(T1 - T0)  (T1-T0)
 *	 s   -----------  + ----------  e 
 *	      (S1 - S0)      (S1 - S0)
 *
 * 	If difference in eventsizes input to the main program is very small
 * 	then infinite values result. To avoid this we divert the calculation
 * 	to another routine (which assumes the input values are equal.
 * 	The value of CALCLIM  = 0.001. (LArger than usual i.e. 0.05)
 *
 *	If size values are so BIG that they exceed segment duration, 
 *	eventsize becomes segment duration and a Warning is printed.  
 *(1)	events approx same size.
 *(2)	events of different sizes, find the number of events which approx
 *	fit in the duration.
 *(3)	If there's only one event .. that's it.
 *(4)	Note the error (difference between approximated whole number of events
 *	and the actual,non-integer, number of events).
 *(5)	Calculate acceptable bounds on a new endeventsize, which will give 
 *	a better fit of the integer number of events within the given duration.
 *(6)	Find an acceptable final value for endeventsize by successive approx
 *	NB we don't know whether the function increases as d increases 
 *	so lobound may be > hibound. However, the maths of the method will 
 *	still work, just inverting the sense of the search!!
 *(7)	Calculate the inital times of the events.
 *
 * This function returns the number of events within the time-interval,
 * and returns the times of these events in the array-of-doubles pos.
 */

int timevents(double intime0,double intime1,double insize0,double insize1)
{   
	int    number;
	double fnum, fnumber, error;
	double lobound, hibound, duration;
	if(flteq(insize0,0.0) || flteq(insize1,0.0)) {
		sprintf(errstr,"Event size of zero encountered.\n");
		do_error();
	}
	duration = (intime1-intime0);
	if(duration<=0.0)   {
		sprintf(errstr,"Inconsistent input times (2nd before 1st).\n");
		do_error();
	}
	if(fabs(insize1-insize0)<CALCLIM)			/* 1 */
		return(number = samesize(intime0,intime1,insize0,insize1,duration));
	fnum     = cntevents(duration,insize0,insize1);	/* 2 */
	number   = round(fnum);
	if(number<=1)					/* 3 */
		return(1);
	pos = (double *)exmalloc((number+1) * sizeof(double));
	fnumber = (double)(number);				/* 4 */
	error   = fabs(fnum - fnumber);
	lobound = insize1;					/* 5 */
	hibound = insize1;
	if(fnum<fnumber)
		hibound = gethibnd(fnum,fnumber,insize1,insize0,duration);
	if(fnum>fnumber)
		lobound = getlobnd(fnum,fnumber,insize1,insize0,duration);
	if(error > FLTERR) {				/* 6 */
		if(lobound < hibound) /* LOBOUND is a HIGH SIZE for a LOW COUNT!! */
		swap(&hibound,&lobound);
		insize1 = refinesize(hibound,lobound,fnumber,error,duration,insize0);
	} else {
	  	insize1 = (hibound+lobound)/2;
	}						/* 7 */
	getetime(intime0,intime1,insize0,insize1,number);
	pos[number] = intime1;
	return(number+1);
}

/*************************** CNTEVENTS *****************************/

double cntevents(double dur,double s0,double s1)
{   double f1,f2;

	f1  = dur;
	f2  = s1-s0;
	f1 /= f2;
	f2  = s1/s0;
	f2  = log(f2);
	f1 *= f2;
	return(fabs(f1));
}

/******************************* SAMESIZE *******************************
 *
 * get event positions, if eventsize approx same throughout segment.
 *
 *(1)	Get average size, find number of events and round to nearest int.
 *(3)	Recalculate size, and thence event times.
 */

int samesize
(double intime0,double intime1,double insize0,double insize1,double duration)
{
	int	number;
	double fnum, size;					/* 1 */
	size   = (insize0+insize1)/2;
	fnum   = duration/size;
	number = round(fnum);
	size   = duration/(double)(number);
	pos	= (double *)exmalloc((number+1) * sizeof(double));
	approxtimes(intime0,size,number);
	pos[number] = intime1;
	return(number+1);
}

/************************ APPROXTIME ***************************
 *
 * Calculate time-positions of equally spaced events.
 */

void approxtimes(double intime0,double size,int number)
{   int k; 
	double *q = pos;
	*q++ = intime0;
	for(k=1;k<number;k++) {
		*q = *(q-1) + size;
		q++;
	}
}

/******************************* GETETIME ********************************
 *
 * Calculate time-positions of events that vary in size between s0 and s1.
 */

void getetime(double t0,double t1,double s0,double s1,int number)
{   int n;
	double sdiff = s1-s0, tdiff = t1-t0, d1, d2, d3, *q = pos;
	*q++ = t0;
	for(n=1;n<number;n++)   {
		d1	= sdiff/tdiff;
		d1   *= (double)n;
		d1    = exp(d1);
		d2    = s0*tdiff;
		d2   /= sdiff;
		d1   *= d2;
		d2    = s1*t0;
		d3    = s0*t1;
		d2   -= d3;
		d2   /= sdiff;
		d1   += d2;
		*q++  = d1;
	}
}

/****************************** GETHIBND *****************************
 *
 * Find SMALLER VALUE of SIZE, to GIVE a LARGER VALUE of NO-OF-SEGS
 * which will act as an UPPER BOUND to be used for searching
 * for an endsize that will give the integer number of events in the duration,
 *
 * (0)	Start by REDUCING size, to give BIGGER fnum.
 *(1)  	If we're going downwards, (try -ve) and we go below zero,restore value
 * 	and subtract less.
 *(2)	If the fnums are moving in the opposite direction to what we expect, 
 *	restore values and increment in the opposite direction,but only half 
 *	as much!
 */

double gethibnd(double fnum,double fnumber,double insize1,
		double insize0,double duration)
{   double lastfnum, try = -1.0;	/* 0 */
	double bound = insize1;
	while(fnum<fnumber)  {
		lastfnum = fnum;
		bound += try;
		while(bound<=0)  {				/* 1 */
			bound -= try;
			try /= 2.0;
			bound += try;
		}
		fnum  = cntevents(duration,insize0,bound);
		if(fnum<lastfnum)  {				/* 2 */
			fnum=lastfnum;
			bound -= try;		
			try	 = -(try/2.0);
   		}
	}
	return(bound);
}

/****************************** GETLOBND *****************************
 *
 *
 * Find LARGER VALUE of SIZE, to GIVE a SMALLER VALUE of NO-OF-SEGS
 * which will act as a LOWER BOUND to be used for searching
 * for an endsize that will give the integer number of events in the duration,
 *
 * LOBOUND is a LARGER value of SIZE to give SMALLER value of NUM-OF-SEGS.
 *
 * (0)	Start by INCREASING size, to give SMALLER fnum.
 * (1)	If we're going downwards, (try -ve) and we go below zero, restore 
 *	propr value of bound, value and subtract less.
 * (2)	If the fnums are moving in the opposite direction to what we expect, 
 *	(fnum > lastfnum, while we're trying to DECREASE fnum)
 *	restore values and increment in the opposite direction,but only half 
 *	as much!
 */

double getlobnd(double fnum,double fnumber,double insize1,
		double insize0,double duration)
{   double try = 1.0;
	double lastfnum;
	double bound = insize1;
	while(fnum>fnumber)  {
		lastfnum = fnum;
		bound += try;
		while(bound<=0)  {
			bound -= try;
			try /= 2;
			bound += try;
		}
		fnum  = cntevents(duration,insize0,bound);
		if(fnum>lastfnum)  {
			fnum =lastfnum;
			bound -= try;		
			try = -(try/2);
		}
	}
	return(bound);
}

/***************************** REFINESIZE ******************************
 *
 * refine size of final event to reduce error within bounds.
 */

double refinesize(double hibound,double lobound,double fnumber,
		  double error,double duration,double insize0)
{   double size = (hibound+lobound)/2, fnum;
	while(error>(FLTERR))   {
		size = (hibound+lobound)/2;
		fnum  = cntevents(duration,insize0,size);
		error = fabs(fnumber-fnum);
		if(error>FLTERR)  {
			if(fnum<fnumber)
			lobound = size;
		else
			hibound = size;
		}
	}
	return(size);
}

/**************************STRGETFLOAT **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

int  strgetfloat(char **str,double *val)
{   char *p, *q, *end;
	double numero;
	int	point, valid;
	for(;;) {
		point = 0;
		p = *str;
		while(isspace(*p))
			p++;
		q = p;	
		if(!isdigit(*p) && *p != '.' && *p!='-')
			return(0);
		if(*p == '.'|| *p == '-') {
			if(*p == '-') {
				p++;
			} else {
				point++;
					p++;
			}
		}
		for(;;) {
			if(*p == '.') {
				if(point)
					return(0);
				else {
					point++;
					p++;
					continue;
				}
			}
			if(isdigit(*p)) {
				p++;
				continue;
			} else {
				if(!isspace(*p) && *p!=ENDOFSTR)
					return(0);
				else {
					end = p;
				    p = q;
					valid = 0;
		    		while(p!=end) {
					   	if(isdigit(*p))
					  		valid++;
						p++;
					}
					if(valid) {
			 			if(sscanf(q,"%lf",&numero)!=1)
							return(0);
						*val = numero;
					   *str = end;
						return(1);
					}
					return(0);
				}
			}
 		}
	}
	return(0);				/* NOTREACHED */
}

/**************************STRGETFLOAT_DB **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).	  IT JUMPS OVER 'db'
 */

int  strgetfloat_db(char **str,double *val)
{
	char *p, *q, *end;
	double numero;
	int    point, valid;
	for(;;) {
		point = 0;
		p = *str;
		while(isspace(*p))
			p++;
		q = p;	
		if(!isdigit(*p) && *p != '.' && *p!='-')
			return(0);
		if(*p == '.'|| *p == '-') {
			if(*p == '-') {
				p++;
			} else {
				point++;
				p++;
			}
		}
		for(;;) {
			if(*p == '.') {
				if(point)
					return(0);
				else {
					point++;
					p++;
					continue;
				}
			}
			if(isdigit(*p)) {
				p++;
				continue;
			} else {
				if(!isspace(*p) && *p!=ENDOFSTR && !(*p=='d' || *p=='D') && !(*(p+1)!='b' || *(p+1)!='B'))
					return(0);
				else {
					end = p;
					p = q;
					valid = 0;
					while(p!=end) {
						if(isdigit(*p))
							valid++;
						p++;
					}
					if(*end=='d' || *end=='D')
						end += 2;
					if(valid) {
						if(sscanf(q,"%lf",&numero)!=1)
							return(0);
						*val = numero;
						*str = end;
						return(1);
					}
					return(0);
				}
			}
		}
	}
	return(0);	   		 /* NOTREACHED */
}

/**************************STRGETSTR ***************************/

int  strgetstr(char **str,char *str2)
{   
	char *p, *q, c;
	p = *str;
	while(isspace(*p))
		p++;
	if(*p == ENDOFSTR)
		return(0);
	q = p;
	while(!isspace(*p))
		p++;
	c = *p;
	*p = ENDOFSTR;
	strcpy(str2,q);
	*p = c;
	*str = p;
	return(1);
}	

/************************** HZTOMIDI ************************/

double miditohz(double midi)
{   double frq;
	frq  = midi;
	frq += 3.0;
	frq /= 12.0;
	frq  = pow((double)2,frq);
	frq *= LOW_A;
	return(frq);
}

/****************************** MIDITOHZ *************************/

double hztomidi(double hz)
{   double midi;
	midi  = hz;
	midi /= LOW_A;
	midi  = log10(midi) * CONVERT_LOG10_TO_LOG2;
	midi *= 12.0;
	midi -= 3.0;
	return(midi);
}

/*********************** RNDPERM ************************/

void rndperm(double *z)
{
	int n,t;
	int k = cnt * sizeof(double);
	permm = (double *)exmalloc(k*2);
	memset((char *)permm,0,k*2);
	permmm = permm + cnt;
	for(n=0;n<cnt;n++) {
		t = (int)(drand48() * (double)(n+1)); /* TRUNCATE */
		if(t==n)
			prefix(z,n);
		else
			insert(z,n,t);
	}
	memcpy((char *)z,(char *)permmm,k);
	free(permm);
}

/*********************** RNDPERM2 ************************
 *
 * Does not malloc and free perm internally.
 */

void rndperm2(double *z)
{   int n,t;
	int k = cnt * sizeof(double);
	memset((char *)permm,0,k*2);
	permmm = permm + cnt;
	for(n=0;n<cnt;n++) {
		t = (int)(drand48() * (double)(n+1)); /* TRUNCATE */
		if(t==n)
			prefix(z,n);
		else
			insert(z,n,t);
	}
	memcpy((char *)z,(char *)permmm,k);
}

/*********************** MULTRNDPERM ************************
 *	START
 *	  orig      orig     orig
 *	|--------|--------|--------|
 *	1ST PERM
 *        perm1     orig     orig
 *	|--------|--------|--------|
 *	NEXT PERM
 *        perm1     perm2    orig
 *	|--------|--------|--------|
 *	MEMCPY
 *     perm1=perm2  orig     orig
 *	|--------|--------|--------|
 *	go to NEXT PERM
 */

void multrndperm(double *blok1)
{   int n, m;
	double lastenditem, *blok2, *blok3;
	int oneblok	= cnt * sizeof(double);
	int twobloks   = oneblok*2;
    int threebloks = oneblok*3;
	blok1 = (double *)exrealloc((char *)blok1,threebloks);
	blok2 = blok1 + cnt;
	blok3 = blok2 + cnt;
    memcpy((char *)blok2,(char *)blok1,oneblok);
	memcpy((char *)blok3,(char *)blok1,oneblok);
	permm = (double *)exmalloc(cnt * 2 * sizeof(double));
	rndperm2(blok1);			/* perm 1st copy */
	for(n=0;n<cnt;n++)
		do_valout(blok1[n]);
	for(m=1;m<ifactor;m++) {	/* now do perms on 2nd copy */
 	   lastenditem = *(blok2-1);
		do {
 			rndperm2(blok2);
		} while(blok2[0]==lastenditem);
			 /* Avoid element repeat at block boundaries */
		for(n=0;n<cnt;n++)
			do_valout(blok2[n]);
		memcpy((char *)blok1,(char *)blok2,twobloks); 
			/* Move perm along & restore orig set */
	}
 	fflush(stdout);
	free(permm);
}

/************************** INSERT *************************/

void insert(double *z,int m,int t)
{
	shuffle(t+1,m);
	permmm[t+1] = z[m];
}

/************************** PREFIX ***************************/

void prefix(double *z,int m)
{   permmm--;
	permmm[0] = z[m];
}

/*************************** SHUFFLE ***********************/

void shuffle(int k,int m)
{
	int n;
	double *i;
	if(k/2 > m) {	/* shuffle up */
		i = &permmm[m];
		for(n=m;n>k;n--) {
			*i = *(i-1);
			i--;
		}
	} else {		/* shuffle down */
		i = &permmm[0];
		for(n=0;n<k;n++) {
			*(i-1) = *i;
			i++;
		}
		permmm--;	/* Move base of perm down by 1 */
	}
}

/******************************** EXMALLOC ****************************/

char *exmalloc(int n)
{
	char *p;
	if((p = (char *)malloc(n))==NULL) {
		sprintf(errstr,"ERROR: Memory allocation failed\n");
		do_error();
	}
	return(p);
}

/************************ READ_INPUT_FILE ****************************/

void read_input_file(FILE *fpp)
{   char *p;
	double *number2;
	while(fgets(temp,20000,fpp)!=NULL) {
		p = temp;
		while(strgetfloat(&p,&number[cnt])) {
			if(++cnt >= arraysize) {
				arraysize += BIGARRAY;
/* NOVEMBER 2001 NEW */
				number2 = (double *)malloc(arraysize*sizeof(double));
				memcpy((void *)number2,(void *)number,cnt * sizeof(double));
				number = number2;
/* NOVEMBER 2001 OLD			
				number = (double *)exrealloc((char *)number,arraysize*sizeof(double));
*/					
			}
		}
	}
}

/************************* DO_INFILE ***************************/

void do_infile(char *argv)
{
	char *p;
	double *number2;
	if((fp[0] = fopen(argv,"r"))==NULL) {
		sprintf(errstr,"Cannot open infile %s\n",argv);
		do_error();
	}
	while(fgets(temp,20000,fp[0])!=NULL) {
		p = temp;
		while(strgetfloat(&p,&number[cnt])) {
			if(++cnt >= arraysize) {
				arraysize += BIGARRAY;
/* NOVEMBER 2001 NEW */
				if((number2=(double *)malloc(arraysize*sizeof(double)))==NULL) {
					sprintf(errstr,"Out of memory for more numbers at %d numbers\n",cnt);
					do_error();
				}
				memcpy((void *)number2,(void *)number,cnt * sizeof(double));
				number = number2;

/* NOVEMBER 2001 OLD
				if((number=(double *)exrealloc((char *)number,arraysize*sizeof(double)))==NULL) {
					sprintf(errstr,"Out of memory for more numbers at %d numbers\n",cnt);
					do_error();
				}
*/
			}
		}
	}
	if(cnt <=0) {
		sprintf(errstr,"Invalid or missing data.\n");
		do_error();
	}
	fclose(fp[0]);
}

/************************* DO_DB_INFILE ***************************/

void do_DB_infile(char *argv)
{
	char *p;
	if((fp[0] = fopen(argv,"r"))==NULL) {
		sprintf(errstr,"Cannot open infile %s\n",argv);
		do_error();
	}
	while(fgets(temp,20000,fp[0])!=NULL) {
		p = temp;
		while(strgetfloat_db(&p,&number[cnt])) {
			if(++cnt >= arraysize) {
				arraysize += BIGARRAY;
				number=(double *)exrealloc((char *)number,arraysize*sizeof(double));
			}
		}
	}
	if(cnt <=0) {
		sprintf(errstr,"Invalid or missing data.\n");
		do_error();
	}
	fclose(fp[0]);
}

/************************* DO_PITCHTEXT_INFILE ***************************/

void do_pitchtext_infile(char *argv)
{
	char *p;
	char temp2[200];
	if((fp[0] = fopen(argv,"r"))==NULL) {
		sprintf(errstr,"Cannot open infile %s\n",argv);
		do_error();
	}
	while(fgets(temp,200,fp[0])!=NULL) {
		p = temp;
		while(strgetstr(&p,temp2)) {
			number[cnt] = texttomidi(temp2);
			if(++cnt >= arraysize) {
				arraysize += BIGARRAY;
				number=(double *)exrealloc((char *)number,arraysize*sizeof(double));
			}
		}
	}
	if(cnt <=0) {
		sprintf(errstr,"ERROR: Invalid or missing data.\n");
		do_error();
	}
	fclose(fp[0]);
}

/************************* DO_OTHER_INFILE **********************/

void do_other_infiles(char *argv[])
{   
	int n;
	if(infilecnt>2)
		fp = (FILE **)exrealloc((char *)fp,infilecnt*sizeof(FILE *));
	firstcnt = cnt;
	for(n=1;n<infilecnt;n++) {
		if((fp[n] = fopen(argv[n+1],"r"))==NULL) {
			sprintf(errstr,"\nCannot open file %s to read.\n",argv[n+1]);
			do_error();
		}
		read_input_file(fp[n]);
		if (flag == 'A' && ro =='e') {
			file_cnt[n] = cnt - firstcnt;
			firstcnt = cnt;
		} else if((flag=='J' || (flag=='C' && ro == 'c')) && cnt != firstcnt * (n+1)) {
			sprintf(errstr,"Input file %d has wrong number of values to match previous column lengths.\n",n+1);
			do_error();
		}
	}
	if (flag == 'w' && (ro == 't' || ro == 'o')) {
		test_warp_data();
	}
}

/********************** DO_OUTFILE *****************************/

void do_outfile(char *argv) {

	if((fp[1] = fopen(argv,"w"))==NULL) {
		sprintf(errstr,"Cannot open file %s to write.\n",argv);
		do_error();
		exit(1);
	}
}

/**************************** QIKSORT ******************************/

void qiksort(void) {
	bublsort();
}

/************************ BELLPERM1 ***************************/

void bellperm1(void)
{   int n;
	for(n=0;n<cnt;n+=2)
		swap(&number[n], &number[n+1]);
}

/************************** BELLPERM2 ******************************/

void bellperm2(void)
{   int n;
	for(n=2;n<cnt;n+=2)
		swap(&number[n], &number[n-1]);
}

/************************* BELLPERM3 ******************************/

void bellperm3(void)
{   int n;
	for(n=2;n<cnt;n+=2)
		swap(&number[n], &number[n+1]);
}

/***************************** SWAP *******************************/

void swap(double *d0,double *d1)
{
	double temp = *d0;
	*d0 = *d1;
	*d1 = temp;
}

/**************************** FLTEQ *******************************/

int flteq(double f1,double f2)
{
	double upperbnd, lowerbnd;
	upperbnd = f2 + FLTERR;		
	lowerbnd = f2 - FLTERR;		
	if((f1>upperbnd) || (f1<lowerbnd))
		return(0);
	return(1);
}

/***************************** ELIMINATE ********************************/

void eliminate(int m)
{
	int n;
	for(n=m+1;n<cnt;n++)
		number[n-1] = number[n];
	cnt--;
}

/***************************** EXREALLOC *****************************/

char *exrealloc(char *p,int k)
{
	char *q;
	q = exmalloc(k);
	memcpy(q,p,k);
	free(p);
	return(q);
}

/****************************** TEXTTOMIDI ******************************/

double texttomidi(char *str)
{   
	int pitch, octave, octshift = 0;
	char *p = str;
	if(!strncmp(str,"B#",2) || !strncmp(str,"b#",2))
		octshift--;
	if((str = get_pitchclass(str,&pitch,1))==(char *)0) {
		sprintf(errstr,"Unknown pitch %s\n",p);
		do_error();
	}
	if(sscanf(str,"%d",&octave)!=1) {
		sprintf(errstr,"unknown pitch-string %s or no octave value\n",p);
		do_error();
	}
	octave += (MIDDLE_C_MIDI_OCTAVE + octshift);
	pitch  += (octave * 12);
	return((double)pitch);
}

/****************************** GET_PITCHCLASS ******************************/

char *get_pitchclass(char *str,int *pitch,int with_octave)
{
	switch(*str++) {
	case('c'): case('C'): *pitch = 0; break;
	case('d'): case('D'): *pitch = 2; break;
	case('e'): case('E'): *pitch = 4; break;
	case('f'): case('F'): *pitch = 5; break;
	case('g'): case('G'): *pitch = 7; break;
	case('a'): case('A'): *pitch = 9; break;
	case('b'): case('B'): *pitch = 11; break;
	default: return((char *)0);
	}
	switch(*str++) {
	case('#'): 		(*pitch)++; break;
	case('b'): 		(*pitch)--; break;
	default: 		str--; 		break;
	}
 	if(with_octave) {
		if(*str==ENDOFSTR)
			return((char *)0);		
	}
	return(str);
}

/******************************** LOGO() **********************************/

void logo(void)
{   printf("\t    ***************************************************\n");
	printf("\t    *           COMPOSERS DESKTOP PROJECT             *\n");
	printf("\t                   %s $Revision: 1.8 $\n",PROG);
	printf("\t    *    Manipulate or Generate columns of numbers    *\n");
	printf("\t    *                by TREVOR WISHART                *\n");
	printf("\t    ***************************************************\n\n");
}

/****************************** PITCHTOTEXT ******************************/

void pitchtotext(int midi)
{   int oct, n;
	int basetone;
	double diff;
	for(n=0;n<cnt;n++) {
		if(midi) {
			if(number[n] < MIDIMIN || number[n] > MIDIMAX) {
				fprintf(stdout,"ERROR: MIDI value %d (%lf) is out of range for conversion to pitch.\n", n+1,number[n]);
				fflush(stdout);
				exit(1);
			}
		} else {
			if(number[n] < MIDIMINFRQ || number[n] > MIDIMAXFRQ) {
				fprintf(stdout,"ERROR: frq value %d (%lf) is out of range for conversion to pitch.\n", n+1,number[n]);
				fflush(stdout);
				exit(1);
			}
		}
	}
	for(n=0;n<cnt;n++) {
		if(!midi)
			number[n] = hztomidi(number[n]);
		oct = (int)(number[n]/12.0);	/* TRUNCATE */
		oct -= 5;
		basetone = (int)floor(number[n]);
		diff = number[n] - (double)basetone;
		if(diff > .5)
			basetone++;
		prnt_pitchclass(basetone%12,oct);
		if(flteq(diff,0.0))
			strcat(errstr,"\n");
		else if(flteq(diff,0.5))	/* handle quarter tones */
			strcat(errstr,"+\n");
		else if(diff > .5)
			strcat(errstr,"(--)\n");
		else
			strcat(errstr,"(++)\n");
		fprintf(stdout,"INFO: %s",errstr);
	}
	fflush(stdout);
}

/****************************** PRNT_PITCHCLASS ******************************/

void prnt_pitchclass(int z,int oct)
{
   	switch(z) {
	case(0):	sprintf(errstr,"C%d",oct);	break;
	case(1):	sprintf(errstr,"C#%d",oct);	break;
	case(2):	sprintf(errstr,"D%d",oct);	break;
	case(3):	sprintf(errstr,"Eb%d",oct);	break;
	case(4):	sprintf(errstr,"E%d",oct);	break;
	case(5):	sprintf(errstr,"F%d",oct);	break;
	case(6):	sprintf(errstr,"F#%d",oct);	break;
	case(7):	sprintf(errstr,"G%d",oct);	break;
	case(8):	sprintf(errstr,"Ab%d",oct);	break;
	case(9):	sprintf(errstr,"A%d",oct);	break;
	case(10):	sprintf(errstr,"Bb%d",oct);	break;
	case(11):	sprintf(errstr,"B%d",oct);	break;
	}					  
}


void bublsort(void) {
	int n, m;
	double temp;
	for(n=0;n<cnt-1;n++) {
		for(m = n; m<cnt; m++) {
			if(number[m] < number[n]) {
				temp = number[n];
				number[n] = number[m];
				number[m] = temp;
			}
		}
	}
}

/************************* DO_OTHER_STRINGLINE_INFILE ***************************/

int do_other_stringline_infile(char *argv)
{
	int cols;
/*
	cnt = stringscnt; DONE PREVIOUSLY
*/
	cols = do_stringline_infile(argv,cnt);
	stringscnt -= cnt;
	return cols;
}

/************************* DO_STRINGLINE_INFILE ***************************/

int do_stringline_infile(char *argv,int n)
{
	int  strspace, cccnt, ccnt = 0, lcnt = 0;

	int space_step = 200, nn;
	int old_stringstoresize;
	char *p, *zong;
	char temp2[200];
	int total_space = space_step;
	if((fp[0] = fopen(argv,"r"))==NULL) {
		fprintf(stdout,"ERROR: Cannot open infile %s\n",argv);
		fflush(stdout);
		exit(1);
	}
	if(stringstoresize == 0) {
		if((stringstore = (char *)exmalloc(total_space))==NULL) {
			sprintf(errstr,"Out of Memory\n");
			do_error();
		}
	}
	while(fgets(temp,200,fp[0])!=NULL) {
		p = temp;
		cccnt = 0;
		while(strgetstr(&p,temp2)) {
			strspace = strlen(temp2)+1;
			old_stringstoresize = stringstoresize;
			if((stringstoresize += strspace) >= total_space) {
				while(stringstoresize  >= total_space)
					total_space += space_step;
				if((zong = (char *)malloc(total_space))==NULL) {
					sprintf(errstr,"Out of Memory\n");
					do_error();
				} 
				memcpy(zong,stringstore,old_stringstoresize);
				free(stringstore);
				stringstore = zong;
			}
			strcpy(stringstore + stringstart,temp2);
			stringstart += strspace;
			stringscnt++;
			cccnt++;
		}
		if(lcnt == 0) {
			ccnt = cccnt;
			lcnt++;
		} else if(ccnt != cccnt) {
			if(cccnt != 0) {
				fprintf(stdout,"ERROR: File %s is not a true table file (line %d has %d cols instead of %d).\n",
				argv,lcnt+1,cccnt,ccnt);
				fflush(stdout);
				exit(1);
			} else {
				continue;
			}
		} else {			
			lcnt++;
		}
	}
	if(stringscnt <= n) {
		fprintf(stdout,"ERROR: Invalid or missing data.\n");
		fflush(stdout);
		exit(1);
	}
	if(strings == 0) {
		if((strings = (char **)malloc(stringscnt * sizeof(char *)))==NULL) {
			sprintf(errstr,"Out of Memory\n");
			do_error();
		}
	} else {
		if((strings = (char **)realloc((char *)strings,stringscnt * sizeof(char *)))==NULL) {
			sprintf(errstr,"Out of Memory\n");
			do_error();
		}
	}
	p = stringstore;
	nn = 0;
	while(nn < stringscnt) {
		strings[nn] = p;
		while(*p != ENDOFSTR)
			p++;
		p++;
		nn++;
	}
	fclose(fp[0]);
	return ccnt;
}

/************************* DO_OTHER_STRINGLINE_INFILE ***************************/

void do_other_stringline_infiles(char *argv[],char c)
{
	int n;
	int sum, thiscolcnt, last_stringscnt, this_stringscnt, rowcnt=0, thisrowcnt;

	if((cntr = (int *)malloc(infilecnt*sizeof(int)))==NULL) {
		fprintf(stdout,"ERROR: Out of memory.\n");
		fflush(stdout);
		exit(1);
	}
	cntr[0] = stringscnt;
	sum = cntr[0];

	if(c == 'J') {
		if((rowcnt = stringscnt/colcnt) * colcnt != stringscnt) {
			fprintf(stdout,
			"ERROR: Incomplete table 1 : all rows must have same number of columns for this option.\n");
			fflush(stdout);
			exit(1);
		}
	}
	for(n=1;n<infilecnt;n++) {
		last_stringscnt = stringscnt;
		thiscolcnt = do_stringline_infile(argv[n+1],stringscnt);
		switch(c) {
		case('j'):
			if(thiscolcnt != colcnt) {
				fprintf(stdout,"ERROR: Count of columns incompatible in the input files.\n");
				fflush(stdout);
				exit(1);
			}
 			break;
		case('W'):
			if(thiscolcnt != colcnt) {
				fprintf(stdout,"ERROR: Count of columns incompatible in the input files.\n");
				fflush(stdout);
				exit(1);
			}
 			break;
		case('J'):
			this_stringscnt = stringscnt - last_stringscnt;
			if((thisrowcnt = this_stringscnt/thiscolcnt) * thiscolcnt != this_stringscnt) {
				fprintf(stdout,
				"ERROR: Table %d incomplete: all rows must have same number of columns for this option.\n",n+1);
				fflush(stdout);
				exit(1);
			}
			if(thisrowcnt != rowcnt) {
				fprintf(stdout,"ERROR: File %d does not have same number of rows as 1st file.\n",n+1);
				fflush(stdout);
				exit(1);
			}
			break;
		}
		cntr[n] = stringscnt - sum;
		sum += cntr[n];
	}
	if(c == 'J')
		colcnt = rowcnt;	/* held temporarily in global */
}

/************************* DO_STRING_INFILE ***************************/

void do_string_infile(char *argv)
{
	int  strspace;
	int space_step = 200, startstringscnt = stringscnt, n;
	int old_stringstoresize;
	char *p;
	char temp2[200];
	int total_space = space_step;
	if((fp[0] = fopen(argv,"r"))==NULL) {
		sprintf(errstr,"Cannot open infile %s\n",argv);
		do_error();
	}
	if(stringstoresize == 0) {
		if((stringstore = (char *)malloc(total_space))==NULL) {
			sprintf(errstr,"Out of Memory\n");
			do_error();
		}
	}
	while(fgets(temp,200,fp[0])!=NULL) {
		p = temp;
		while(strgetstr(&p,temp2)) {
			strspace = strlen(temp2)+1;
			old_stringstoresize = stringstoresize;
			if((stringstoresize += strspace) >= total_space) {
				while(stringstoresize  >= total_space)
					total_space += space_step;
				if((stringstore = (char *)realloc((char *)stringstore,total_space))==NULL) {
					sprintf(errstr,"Out of Memory\n");
					do_error();
				}
			}
			strcpy(stringstore + stringstart,temp2);
			stringstart += strspace;
			stringscnt++;
		}
	}
	if(stringscnt <=0) {
		sprintf(errstr,"Invalid or missing data.\n");
		do_error();
	}
	if(strings == 0) {
		if((strings = (char **)malloc(stringscnt * sizeof(char *)))==NULL) {
			sprintf(errstr,"Out of Memory\n");
			do_error();
		}
	} else {
		if((strings = (char **)realloc((char *)strings,stringscnt * sizeof(char *)))==NULL) {
			sprintf(errstr,"Out of Memory\n");
			do_error();
		}
	}
	p = stringstore;
	n = 0;
	while(n < startstringscnt) {
		while(*p != ENDOFSTR)
			p++;
		p++;
		n++;
	}
	while(n < stringscnt) {
		strings[n] = p;
		while(*p != ENDOFSTR)
			p++;
		p++;
		n++;
	}
	fclose(fp[0]);
}

/************************* TEST_WARP_DATA ***************************/

void test_warp_data(void) 
{
	int n = 0;
	if(number[n] < 0) {
		sprintf(errstr,"Negative time in breakpoint file: Cannot proceed.\n");
		do_error();
		exit(1);
	}
	if(number[1] < 1.0/1024.0 || number[1] > 1024.0) {
		sprintf(errstr,"Dubious Timestretch value (%lf) in brkpnt file.\n",number[1]);
		do_error();
		exit(1);
	}
	for(n = 2; n <firstcnt; n+=2) {
		if(number[n] <= number[n-2]) {
			sprintf(errstr,"Times (%lf & %lf) not in increasing order in brkpnt file.\n",number[n-2],number[n]);
			do_error();
			exit(1);
		}
		if(number[n+1] < 1.0/1024.0 || number[n+1] > 1024.0) {
			sprintf(errstr,"Dubious Timestretch value (%lf) in brkpnt file.\n",number[n+1]);
			do_error();
			exit(1);
		}
	}
	for(n = firstcnt; n <cnt; n++) {
		if(number[n] < 0.0) {
			sprintf(errstr,"Negative time (%lf) given in 2nd file: Cannot proceed.\n",number[n]);
			do_error();
			exit(1);
		}
	}
}
