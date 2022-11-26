/*
 * Take a set of amp-frq data in the correct range,
 * and use cubic spline to replot curve to cover
 * every 1/64 tone over whole range.
 * Where, frq ratio between adjacent chans becomes < 1/64 tone
 * use chan centre frqs.
 */

#define PSEUDOCENT	(1.0009)	// approx 1/64th of a tone

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <filetype.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <globcon.h>
#include <cdpmain.h>
#include <math.h>
#include <mixxcon.h>
#include <osbind.h>
#include <science.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>

int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

char errstr[2400];  //RWD added
//CDP LIB REPLACEMENTS
static int allocate_output_array(double **outarray, int *outcnt, double chwidth, double halfchwidth, double nyquist);
static int get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static int handle_the_special_data(FILE *fpi, double **parray, double **secondderiv, double **x, double **y, int *itemcnt);
static int test_the_special_data(double *parray, int itemcnt, double nyquist);
static int smooth(double *x,double *y,double *secondderiv,double *outarray,int *outcnt,double chwidth,double halfchwidth,double nyquist,int arraycnt);
//static int smooth(double *parray,double *outarray,int *outcnt, double chwidth, double halfchwidth,double nyquist);
static int spline(double *parray,int itemcnt,double *secondderiv,double *x,double *y);
static int splint(double thisfrq,double *thisamp,int *hi,int arraycnt,double *secondderiv, double *x, double *y);
static int dove(double *parray,int itemcnt,double *outarray,int outcnt);
static int  get_float_with_e_from_within_string(char **str,double *val);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	FILE *fpi, *fpo;
	int k, OK = 0, dovetail = 0;
	int exit_status, srate, pointcnt, clength;
	double chwidth, halfchwidth, nyquist, *parray, *secondderiv, *x, *y, *outarray;
	int itemcnt, outcnt, arraycnt;
	char temp[200], temp2[200];
	if(argc < 5 || argc > 6) {
		usage1();	
		return(FAILED);
	}
	if((fpi = fopen(argv[1],"r")) == NULL) {
		fprintf(stdout,"CANNOT OPEN DATAFILE %s\n",argv[1]);
		fflush(stdout);
		return(FAILED);
	}
	if((exit_status = handle_the_special_data(fpi,&parray,&secondderiv,&x,&y,&itemcnt))<0) {
		return(FAILED);
	}
	arraycnt = itemcnt/2;
	if(sscanf(argv[3],"%d",&pointcnt) != 1) {
		fprintf(stdout,"CANNOT READ POINTCNT (%s)\n",argv[3]);
		fflush(stdout);
		return(FAILED);
	}
	OK = 0;
	k = 1;
	while(k < pointcnt) {
		k *= 2;
		if(k == pointcnt){
			OK= 1;
			break;
		}
	}
	if(!OK) {
		fprintf(stdout,"ANALYSIS POINTS PARAMETER MUST BE A POWER OF TWO.\n");
		fflush(stdout);
		return(FAILED);
	}
	if(sscanf(argv[4],"%d",&srate) != 1) {
		fprintf(stdout,"CANNOT READ SAMPLE RATE (%s)\n",argv[3]);
		fflush(stdout);
		return(FAILED);
	}
 	if(srate < 44100 || BAD_SR(srate)) {
		fprintf(stdout,"INVALID SAMPLE RATE ENTERED (44100,48000,88200,96000 only).\n");
		fflush(stdout);
		return(DATA_ERROR);
	}
	if(argc == 6) {
		if(strcmp(argv[5],"-s")) {
			fprintf(stdout,"UNKNOWN FINAL PARAMETER (%s).\n",argv[5]);
			fflush(stdout);
			return(DATA_ERROR);
		}
		dovetail = 1;
	}
	clength	 = (pointcnt + 2)/2;
	nyquist	 = (double)(srate / 2);		
	chwidth	 = nyquist/(double)clength;
	halfchwidth = chwidth / 2;

	if((exit_status = test_the_special_data(parray,itemcnt,nyquist))<0)
		return(FAILED);
	if((exit_status = 	allocate_output_array(&outarray,&outcnt,chwidth,halfchwidth,nyquist))<0)
		return(FAILED);
	if((fpo = fopen(argv[2],"w")) == NULL) {
		fprintf(stdout,"CANNOT OPEN OUTPUT DATAFILE %s\n",argv[2]);
		fflush(stdout);
		return(FAILED);
	}
	if((exit_status = spline(parray,itemcnt,secondderiv,x,y))<0)
		return(FAILED);
	if((exit_status = smooth(x,y,secondderiv,outarray,&outcnt,chwidth,halfchwidth,nyquist,arraycnt)) < 0)
		return(FAILED);
	if(dovetail) {
		if((exit_status = dove(parray,itemcnt,outarray,outcnt)) < 0)
			return(FAILED);
	}
	k = 0;
	for(k = 0;k < outcnt;k+=2) {
		sprintf(temp,"%lf",outarray[k]);
		sprintf(temp2,"%lf",outarray[k+1]);
		strcat(temp2,"\n");
		strcat(temp,"\t");
		strcat(temp,temp2);
		fputs(temp,fpo);
	}
	if(fclose(fpo)<0) {
		fprintf(stdout,"WARNING: Failed to close output data file %s.\n",argv[2]);
		fflush(stdout);
	}
	return(SUCCEEDED);
}


/******************************** USAGE1 ********************************/

int usage1(void)
{
	fprintf(stderr,
	"USAGE: cubicspline datafile outdatafile pointcnt srate [-s]\n"
	"\n"
	"Generate smoothed curve from input data re spectrum.\n"
	"\n"
	"DATAFILE     textfile of frq/amp pairs.\n"
	"OUTDATAFILE  textfile of smoothed data.\n"
	"POINTCNT     no of analysis points required in handling data.\n"
	"SRATE        samplerate of eventual sound output.\n"
	"-s           if spectral amp falls to zero before the nyquist,\n"
	"             keep zeroed end of spectrum at zero.\n."
	"\n");
	return(USAGE_ONLY);
}

/************************** HANDLE_THE_SPECIAL_DATA **********************************/

int handle_the_special_data(FILE *fpi, double **parray, double **secondderiv, double **x, double **y,int *itemcnt)
{
	int cnt, linecnt;
	int arraycnt;
	double *p, dummy;
	char temp[200], *q;
	cnt = 0;
	p = &dummy;
	while(fgets(temp,200,fpi)==temp) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_with_e_from_within_string(&q,p))
			cnt++;
	}
	if(ODD(cnt)) {
		fprintf(stdout,"Data not paired correctly in input data file\n");
		fflush(stdout);
		return(DATA_ERROR);
	}
	if(cnt == 0) {
		fprintf(stdout,"No data in input data file\n");
		fflush(stdout);
		return(DATA_ERROR);
	}
	if((*parray = (double *)malloc(cnt * sizeof(double)))==NULL) {
		fprintf(stdout,"INSUFFICIENT MEMORY for input data.\n");
		fflush(stdout);
		return(MEMORY_ERROR);
	}
	arraycnt = cnt/2;
	if((*secondderiv = (double *)malloc(cnt/2 * sizeof(double)))==NULL) {
		fprintf(stdout,"INSUFFICIENT MEMORY for storing slope of input data.\n");
		fflush(stdout);
		return(MEMORY_ERROR);
	}
	if((*x = (double *)malloc(cnt/2 * sizeof(double)))==NULL) {
		fprintf(stdout,"INSUFFICIENT MEMORY for storing x-coords of input data.\n");
		fflush(stdout);
		return(MEMORY_ERROR);
	}
	if((*y = (double *)malloc(cnt/2 * sizeof(double)))==NULL) {
		fprintf(stdout,"INSUFFICIENT MEMORY for storing y-coords of input data.\n");
		fflush(stdout);
		return(MEMORY_ERROR);
	}
	fseek(fpi,0,0);
	linecnt = 1;
	p = *parray;
	while(fgets(temp,200,fpi)==temp) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_with_e_from_within_string(&q,p)) {
			p++;
		}
		linecnt++;
	}
	if(fclose(fpi)<0) {
		fprintf(stdout,"WARNING: Failed to close input data file.\n");
		fflush(stdout);
	}
	*itemcnt = cnt;
	return(FINISHED);
}

/************************** TEST_THE_SPECIAL_DATA **********************************/

int test_the_special_data(double *parray, int itemcnt, double nyquist)
{
	double *p;
	int isamp = 0, linecnt = 1;
	p = parray;
	while(linecnt <= itemcnt/2) {
		if(isamp) {
			if(*p < 0.0 || *p > 1.0) {
				fprintf(stdout,"Amp (%lf) out of range (0-1) at line %d in datafile.\n",*p,linecnt);
				fflush(stdout);
				return(DATA_ERROR);
			}
		} else {
			if(*p < 0.0 || *p > nyquist) {
				fprintf(stdout,"Frq (%lf) out of range (0 - %lf) at line %d in datafile.\n",*p,nyquist,linecnt);
				fflush(stdout);
				return(DATA_ERROR);
			}
		}
		if(!isamp)
			linecnt++;
		isamp = !isamp;
		p++;
	}
	return(FINISHED);
}

/************************** ALLOCATE_OUTPUT_ARRAY **********************************/

int allocate_output_array(double **outarray,int *outcnt,double chwidth,double halfchwidth,double nyquist)
{
	int outsize = 1;  // Count initial halfwidht channel
	double lastlofrq, lofrq = halfchwidth;
	while(lofrq <= nyquist) {
		outsize++;
		lastlofrq = lofrq;	
		lofrq *= PSEUDOCENT;
		if(lofrq - lastlofrq >= chwidth)	//	Once chan separation is > chwidth
			lofrq = lastlofrq + chwidth;	//	Just set 1 value per channel
	}
	outsize += 64;  //	SAFETY
	outsize *= 2;
	if((*outarray = (double *)malloc(outsize * sizeof(double)))==NULL) {
		fprintf(stdout,"INSUFFICIENT MEMORY for input data.\n");
		fflush(stdout);
		return(MEMORY_ERROR);
	}
	*outcnt = outsize;
	return(	FINISHED);
}

/************************** SMOOTH **********************************

static int smooth(double *parray,double *outarray,int *outcnt,double chwidth,double halfchwidth,double nyquist)
{
	int k = 0;
	double lastlofrq, lofrq = halfchwidth;
	outarray[k] = 1.0;
	outarray[k+1] = 0.0;
	k = 2;
	while(lofrq <= nyquist) {
		k += 2;
		if(k > *outcnt) {
			fprintf(stdout,"OUT OF MEMORY IN OUTPUT ARRAY.\n");
			fflush(stdout);
			return(MEMORY_ERROR);
		}
		outarray[k] = 0.0;
		outarray[k+1] = lofrq;
		lastlofrq = lofrq;	
		lofrq *= PSEUDOCENT;
		if(lofrq - lastlofrq >= chwidth) {	//	Once chan separation is < HEREH
			lofrq = lastlofrq + chwidth;	//	Just set 1 value per channel
			outarray[k] = 1.0;
		}
	}
	*outcnt = k;
	return(	FINISHED);
}

/************************************ SPLINE ************************************
 *
 * This function returns the second derivative at all points of the curve.
 *
 * We assume that the curve is flat (first derivative = 0) at start and end points
 */

int spline(double *parray,int itemcnt,double *secondderiv,double *x,double *y)
{
	double firstderivstt = 0.0, firstderivend = 0.0; 
	double *u, sig, ppp, qend, uend;
	int i, k, arraylen = itemcnt/2;
	if((u = (double *)malloc(arraylen * sizeof(double)))==NULL) {
		fprintf(stdout,"INSUFFICIENT MEMORY for slope calculations 1.\n");
		fflush(stdout);
		return(MEMORY_ERROR);
	}
	for(i=0,k=0;k<itemcnt;k+=2,i++) {
		x[i] = parray[k];	// FRQ is the independent, monotonically increasing variable
		y[i] = parray[k+1];
	}
	secondderiv[0] = -0.5;
	u[0] = (3.0/(x[1] - x[0])) * (((y[1] - y[0])/(x[1] - x[0])) - firstderivstt);

	for(i = 1;i < arraylen-1; i++) {
		sig = (x[i] - x[i-1])/(x[i+1] - x[i-1]);
		ppp = (sig * secondderiv[i-1]) + 2.0;
		secondderiv[i] = (sig - 1.0)/ppp;
		u[i] = ((y[i+1] - y[i])/(x[i+1] - x[i])) - ((y[i] - y[i-1])/(x[i] - x[i-1]));
		u[i] = (((6.0 * u[i])/(x[i+1] - x[i-1])) - (sig * u[i-1]))/ppp;
	}
	// i = arraylen-1; = last entry in array
	qend = 0.5;
	uend = (3.0/(x[i] - x[i-1])) * (firstderivend - ((y[i] - y[i-1])/(x[i] - x[i-1])));
	secondderiv[i] = (uend - (qend * u[i-1]))/((qend * secondderiv[i-1]) + 1.0);
	for(k = i-1;k >= 0;k--) {
		secondderiv[k] = (secondderiv[k] * secondderiv[k+1]) + u[k];
	}
	free(u);
	return(FINISHED);
}

/************************************ SPLINT ************************************
 *
 * This function returns the value at a specified frq on curve, using cspline interp (2nd-deriv) data..
 */

int splint(double thisfrq,double *thisamp,int *hi,int arraycnt,double *secondderiv, double *x, double *y)
{
	int klo, khi = *hi;
	double fullstep, a, b, amp, jjj, kkk;

	/* Establish which input values bracket 'thisfrq' */

	while(x[khi] <= thisfrq) {
		khi++;
		if(khi >= arraycnt) {
			*thisamp = y[arraycnt - 1];
			return(FINISHED);
		}
	}
	klo = khi - 1;
	if(klo < 0) {
		*thisamp = y[0];
		return(FINISHED);
	}
	fullstep = x[khi] - x[klo];
	a = (x[khi] - thisfrq)/fullstep;
	b = (thisfrq - x[klo])/fullstep;
	amp  = a * y[klo];
	amp += b * y[khi];

	jjj  = a * a * a;
	jjj -= a;
	jjj *= secondderiv[klo];

	kkk  = b * b * b;
	kkk -= b;
	kkk *= secondderiv[khi];

	jjj += kkk;
	jjj *= fullstep * fullstep;
	jjj /= 6.0;
	
	amp += jjj;
	if(amp < 0.0)
		amp = 0.0;
	if(amp > 1.0) {
		fprintf(stdout,"INFO: SMOOTHED DATA CLIPPED\n");
		fflush(stdout);
		amp = 1.0;
	}
	*thisamp = amp;
	*hi = khi;
	return(FINISHED);
}

/************************** SMOOTH **********************************/

int smooth(double *x,double *y,double *secondderiv,double *outarray,int *outcnt,double chwidth,double halfchwidth,double nyquist,int arraycnt)
{
	int exit_status;
	int k = 0, hi = 0;
	double lastlofrq, lofrq = halfchwidth, thisamp;
	outarray[k] = 0.0;
	outarray[k+1] = 0.0;
	k = 2;
	while(lofrq < nyquist) {
		k += 2;
		if(k > *outcnt) {
			fprintf(stdout,"OUT OF MEMORY IN OUTPUT ARRAY.\n");
			fflush(stdout);
			return(MEMORY_ERROR);
		}
		if((exit_status = splint(lofrq,&thisamp,&hi,arraycnt,secondderiv,x,y))<0)
			return(FAILED);
		outarray[k] = lofrq;
		outarray[k+1] = thisamp;
		lastlofrq = lofrq;	
		lofrq *= PSEUDOCENT;
		if(lofrq - lastlofrq >= chwidth)	//	Once chan separation is < PSEUDOCENT
			lofrq = lastlofrq + chwidth;	//	Just set 1 value per channel
	}
	k += 2;
	outarray[k++] = nyquist;
	outarray[k++] = 0.0;
	*outcnt = k;
	if(outarray[0] == 0.0 && outarray[2] == 0.0) {		// Eliminate any duplication of zero start frq
		k = 0;
		hi = 2;
		while(hi < *outcnt) {
			outarray[k] = outarray[hi];
			k++;
			hi++;
		}
		*outcnt = k;
	}	
	return(FINISHED);
}

/************************** DOVE **********************************/

int dove(double *parray,int itemcnt,double *outarray,int outcnt)
{
	double *amp, *frq, *inarrayend = parray + itemcnt, *outarrayend = outarray + outcnt, tailstartfrq;
	amp = inarrayend - 1;
	if(!flteq(*amp,0.0))		// Input spectrum never falls to zero
		return(FINISHED);

	while(amp > parray) {		//	Search back from end of (orig) spectrum, until spectral vals are >0
		if(!flteq(*amp,0.0))
			break;
		amp -= 2;
	}
	if(amp <= parray)			//	(original) spectrum is everywhere zero
		return(FINISHED);
	frq = amp - 1;
	tailstartfrq = *frq;		//	Note frq at which (original) spectrum finally falls to zero

	frq = outarray;				//	Parse frq vals in output array
	while(frq < outarrayend) {
		if(*frq >= tailstartfrq) {
			amp = frq - 1;		//	Note where outfrq exceeds tailstartfrq of input, and step to amp BEFORE it
			break;
		}
		frq += 2;
	}
	if(frq >= outarrayend) {	//	Output frq never exceeds input frq: should be impossible
		fprintf(stdout,"DOVETAILING FAILED.\n");
		fflush(stdout);
		return(PROGRAM_ERROR);
	}
	while(amp < outarrayend) {	//	Search for point where output first touches zero
		if(flteq(*amp,0.0))
			break;
		amp += 2;
	}
	if(amp >= outarrayend) {
		fprintf(stdout,"NO DOVETAILING OCCURED.\n");
		fflush(stdout);
	}
	while(amp < outarrayend) {	//	Set tail to zero
		*amp = 0.0;
		amp += 2;
	}
	return(FINISHED);
}

/************************** GET_FLOAT_WITH_E_FROM_WITHIN_STRING **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

int  get_float_with_e_from_within_string(char **str,double *val)
{
	char   *p, *valstart;
	int    decimal_point_cnt = 0, has_digits = 0, has_e = 0, lastchar = 0;
	p = *str;
	while(isspace(*p))
		p++;
	valstart = p;	
	switch(*p) {
	case('-'):						break;
	case('.'): decimal_point_cnt=1;	break;
	default:
		if(!isdigit(*p))
			return(FALSE);
		has_digits = TRUE;
		break;
	}
	p++;
	while(!isspace(*p) && *p!=NEWLINE && *p!=ENDOFSTR) {
		if(isdigit(*p))
			has_digits = TRUE;
		else if(*p == 'e') {
			if(has_e || !has_digits)
				return(FALSE);
			has_e = 1;
		} else if(*p == '-') {
			if(!has_e || (lastchar != 'e'))
				return(FALSE);
		} else if(*p == '.') {
			if(has_e || (++decimal_point_cnt>1))
				return(FALSE);
		} else
			return(FALSE);
		lastchar = *p;
		p++;
	}
	if(!has_digits || sscanf(valstart,"%lf",val)!=1)
		return(FALSE);
	*str = p;
	return(TRUE);
}


