/*
 * Take a set of amp-frq data in the correct range,
 * find the notches in the spectrum.
 * Create a new spectrum by inverting these notches to become peaks
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

//CDP LIB REPLACEMENTS
static int get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static int handle_the_special_data(FILE *fpi, double **parray, int *itemcnt);
static int test_the_special_data(double *parray, int itemcnt, double nyquist);
static int peakiso(double *parray,int itemcnt,double *outarray,double minnotch);
static int get_float_with_e_from_within_string(char **str,double *val);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	FILE *fpi, *fpo;
	int k;
	int exit_status;
	double *parray, *outarray;
	int itemcnt, srate;
	double minnotch = 0.0, nyquist;
	char temp[200], temp2[200];
	if(argc < 4 || argc > 5) {
		usage1();	
		return(FAILED);
	}
	if(argc == 5) {
		if(sscanf(argv[4],"%lf",&minnotch) != 1) {
			fprintf(stderr,"CANNOT READ POINTCNT (%s)\n",argv[3]);
			return(FAILED);
		}
		if(minnotch > 1.0 || minnotch < 0.0) {
			fprintf(stderr,"INVALID MINIMUM NOTCH %s (range 0 - 1)\n",argv[3]);
			return(FAILED);
		}
	} 
	if(sscanf(argv[3],"%d",&srate) != 1) {
		fprintf(stderr,"CANNOT READ SAMPLERATE (%s)\n",argv[3]);
		return(FAILED);
	}
 	if(srate < 44100 || BAD_SR(srate)) {
		fprintf(stderr,"INVALID SAMPLE RATE ENTERED (44100,48000,88200,96000 only).\n");
		return(FAILED);
	}
	nyquist = (double)(srate/2);
	if((fpi = fopen(argv[1],"r")) == NULL) {
		fprintf(stderr,"CANNOT OPEN DATAFILE %s\n",argv[1]);
		return(FAILED);
	}
	if((exit_status = handle_the_special_data(fpi,&parray,&itemcnt))<0)
		return(FAILED);
	if((exit_status = test_the_special_data(parray,itemcnt,nyquist))<0)
		return(FAILED);
	if((outarray = (double *)malloc(itemcnt * sizeof(double)))==NULL) {
		fprintf(stderr,"INSUFFICIENT MEMORY for input data.\n");
		return(MEMORY_ERROR);
	}
	if((fpo = fopen(argv[2],"w")) == NULL) {
		fprintf(stderr,"CANNOT OPEN OUTPUT DATAFILE %s\n",argv[2]);
		return(FAILED);
	}
	if((exit_status = peakiso(parray,itemcnt,outarray,minnotch)) < 0)
		return(FAILED);
	k = 0;
	for(k = 0;k < itemcnt;k+=2) {
		sprintf(temp,"%lf",outarray[k]);
		sprintf(temp2,"%lf",outarray[k+1]);
		strcat(temp2,"\n");
		strcat(temp,"\t");
		strcat(temp,temp2);
		fputs(temp,fpo);
	}
	if(fclose(fpo)<0) {
		fprintf(stderr,"WARNING: Failed to close output data file %s.\n",argv[2]);
	}
	return(SUCCEEDED);
}


/******************************** USAGE1 ********************************/

int usage1(void)
{
	fprintf(stderr,
	"USAGE: peakiso datafile outdatafile srate [minnotch]\n"
	"\n"
	"Isolate peaks in a spectrum.\n"
	"\n"
	"DATAFILE     textfile of frq/amp pairs.\n"
	"OUTDATAFILE  isolated peaks as frq/amp pairs.\n"
	"MINNOTCH     min depth of notch to qualify as peak-separator (Range 0-1).\n"
	"\n");
	return(USAGE_ONLY);
}

/************************** HANDLE_THE_SPECIAL_DATA **********************************/

int handle_the_special_data(FILE *fpi, double **parray, int *itemcnt)
{
	int cnt;
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
		fprintf(stderr,"Data not paired correctly in input data file\n");
		return(DATA_ERROR);
	}
	if(cnt == 0) {
		fprintf(stderr,"No data in input data file\n");
		return(DATA_ERROR);
	}
	if((*parray = (double *)malloc(cnt * sizeof(double)))==NULL) {
		fprintf(stderr,"INSUFFICIENT MEMORY for input data.\n");
		return(MEMORY_ERROR);
	}
	fseek(fpi,0,0);
	p = *parray;
	while(fgets(temp,200,fpi)==temp) {
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_with_e_from_within_string(&q,p)) {
			p++;
		}
	}
	if(fclose(fpi)<0) {
		fprintf(stderr,"WARNING: Failed to close input data file.\n");
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
				fprintf(stderr,"Amp (%lf) out of range (0-1) at line %d in datafile.\n",*p,linecnt);
				return(DATA_ERROR);
			}
		} else {
			if(*p < 0.0 || *p > nyquist) {
				fprintf(stderr,"Frq (%lf) out of range (0 - %lf) at line %d in datafile.\n",*p,nyquist,linecnt);
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

/************************** PEAKISO **********************************/

int peakiso(double *parray,int itemcnt,double *outarray,double minnotch)
{
	int ampat, trofampat, pkampbelowat, pkampaboveat, nexttrofat, writeat, j;
	double amp, lastamp, ampstep, lastampstep, belowstep, abovestep, notchdepth;
	double *pkbefore, *pkafter, *notch;
	int trofcnt, badnotches, jj, kk, notchwidth, prepeak, postpeak, pksttej, pkendej;
	int ampsttej, ampendej;
	double sttval, endval;
	int *pkbeforeat, *notchat, *orig_notchat;

	memset((char *)outarray,0,itemcnt * sizeof(double));

		/* FIND PEAKS AND TROUGHS */

	lastamp = parray[1];
	lastampstep = 0;
	ampat = 3;
	trofcnt = 0;
	while(ampat < itemcnt) {
		amp = parray[ampat];
		ampstep = amp - lastamp;
		if(ampstep < 0) {		
			if(lastampstep > 0) {			// falling after rise = peak
				outarray[ampat-2] = 1.0;	//	peak
			}
			lastampstep = ampstep;
		} else if(ampstep > 0) {
			if(lastampstep < 0) {			// rising after fall = trough
				outarray[ampat-2] = -1.0;	//	trough
				trofcnt++;
			}
			lastampstep = ampstep;
		}									//	else, if flat : lastampstep not updated
		lastamp = amp;						//	saddle points are thus ignored, while flat peaks or troughs are spotted
		ampat += 2;
	}
	if(trofcnt == 0) {
		fprintf(stderr,"NO NOTCHES FOUND\n");
		for(j = 0; j < itemcnt; j+=2) {
			outarray[j] = parray[j];
			outarray[j+1] = 0.0;
		}
		return FINISHED;
	}
	if((pkbefore = (double *)malloc((trofcnt + 6) * sizeof(double)))==NULL) {
		fprintf(stderr,"INSUFFICIENT MEMORY for trough calculations (1).\n");
		return(MEMORY_ERROR);
	}
	if((pkafter = (double *)malloc((trofcnt + 6) * sizeof(double)))==NULL) {
		fprintf(stderr,"INSUFFICIENT MEMORY for trough calculations (2).\n");
		return(MEMORY_ERROR);
	}
	if((pkbeforeat = (int *)malloc((trofcnt + 6) * sizeof(int)))==NULL) {
		fprintf(stderr,"INSUFFICIENT MEMORY for trough calculations (1).\n");
		return(MEMORY_ERROR);
	}
	if((notch = (double *)malloc((trofcnt + 6) * sizeof(double)))==NULL) {
		fprintf(stderr,"INSUFFICIENT MEMORY for trough calculations (2).\n");
		return(MEMORY_ERROR);
	}
	if((notchat = (int *)malloc((trofcnt + 6) * sizeof(int)))==NULL) {
		fprintf(stderr,"INSUFFICIENT MEMORY for trough calculations (2).\n");
		return(MEMORY_ERROR);
	}
	if((orig_notchat = (int *)malloc((trofcnt + 6) * sizeof(int)))==NULL) {
		fprintf(stderr,"INSUFFICIENT MEMORY for trough calculations (2).\n");
		return(MEMORY_ERROR);
	}
	trofcnt = 0;
	ampat = 1;
	pkampbelowat = -1;						//	position of peak before trough
	pkampaboveat = -1;						//	position of peak after trough
	trofampat = -1;							//	position of trough

	/* NOTE LEVELS BEFORE AND AFTER NOTCHES AND MARK BAD NOTCHES */

	badnotches = 0;

	while(ampat < itemcnt) {
		amp = outarray[ampat];
		if(amp < 0.0) {
			if(pkampbelowat < 0) {
				pkampbelowat = 1;
				pkbefore[trofcnt] = parray[pkampbelowat];
				pkbeforeat[trofcnt] = pkampbelowat;
			}
			trofampat = ampat;
			notch[trofcnt] = parray[ampat];
			notchat[trofcnt] = ampat;
		} else if(amp > 0.0) {				//	found peak
			if(trofampat < 0) {				//	if not yet found trof, this is the peak below the trof
				pkampbelowat = ampat;
				pkbefore[trofcnt] = parray[pkampbelowat];
				pkbeforeat[trofcnt] = pkampbelowat;
			} else {						//	otherwise, its peak above trof : we now have complete notch
				pkampaboveat = ampat;
				pkafter[trofcnt] = parray[pkampaboveat];
				belowstep = pkbefore[trofcnt] - parray[trofampat];
				abovestep = pkafter[trofcnt]  - parray[trofampat];
				notchdepth = min(belowstep,abovestep);
				if(notchdepth < minnotch)
					badnotches = 1;
				trofcnt++;
				pkampbelowat = pkampaboveat; //	move to next notch
				pkbefore[trofcnt] = parray[pkampbelowat];
				pkbeforeat[trofcnt] = pkampbelowat;
				pkampaboveat = -1;
				trofampat = -1;
			}
		}
		ampat += 2;							//	if no peak found after last trough .. 
	}										//	and last trof is not at very end (shoulf be impossible)
	if(trofampat >= 0 && (trofampat != itemcnt-1) && (pkampaboveat < 0)) {
		pkampaboveat = itemcnt - 1;			//	set peak to top edge of spectrum
		pkafter[trofcnt]  = parray[pkampaboveat];
		belowstep = pkbefore[trofcnt] - parray[trofampat];
		abovestep = pkafter[trofcnt]  - parray[trofampat];
		notchdepth = min(belowstep,abovestep);
		if(notchdepth < minnotch)			//	Mark notches that are not deep enough
			badnotches = 1;
		trofcnt++;
		pkbefore[trofcnt] = parray[pkampaboveat];	//	Extra "pkbefore" after last notch
		pkbeforeat[trofcnt] = pkampaboveat;
	}
	for(kk=0;kk<trofcnt;kk++)
		orig_notchat[kk] = notchat[kk];

	/* ELIMINATE ADJACENT BAD NOTCHES */
		
	if(badnotches) {
		for(kk = 0;kk < trofcnt;kk++) {									//	x	      x
			if(pkbefore[kk] - notch[kk]	< minnotch) {					//	 x   o   x
				if((kk>0) && (pkbefore[kk] - notch[kk-1] < minnotch)) {	//    x x x x
					//ELIMINATE PEAK									//	   x   o
					outarray[pkbeforeat[kk]] = 0.0;
					// KEEP DEEPEST NOTCH
					if(notch[kk] <= notch[kk-1]) {					//		x	      x
						outarray[notchat[kk-1]] = 0.0;				//		 x   o   x
						notch[kk-1] = notch[kk];					//		  x x x x
						notchat[kk-1]= notchat[kk];					//		   +   o			
					} else {
						outarray[notchat[kk]] = 0.0;
					}
					for(jj = kk+1;jj < trofcnt; jj++) {
						pkbefore[jj-1] = pkbefore[jj];
						pkbeforeat[jj-1] = pkbeforeat[jj];
						notch[jj-1] = notch[jj];
						notchat[jj-1] = notchat[jj];
					}
					pkbefore[jj-1] = pkbefore[jj];
					pkbeforeat[jj-1] = pkbeforeat[jj];
				} else if(pkbefore[kk+1] - notch[kk] < minnotch) {	//		   o   +
					//ELIMINATE NOTCH								//		  x x x x
 					outarray[notchat[kk]] = 0.0;					//		 x   o	 x
					//ELIMINATE LOWEST PEAK							//		x		  x
					if(pkbefore[kk] >= pkbefore[kk+1]) {
						outarray[pkbeforeat[kk+1]] = 0.0;
						if(kk < trofcnt-1) {
							notch[kk] = notch[kk+1];
							notchat[kk] = notchat[kk+1];
						}
						for(jj = kk+2;jj < trofcnt; jj++) {
							pkbefore[jj-1] = pkbefore[jj];
							pkbeforeat[jj-1] = pkbeforeat[jj];
							notch[jj-1] = notch[jj];
							notchat[jj-1] = notchat[jj];
						}
						pkbefore[jj-1] = pkbefore[jj];
						pkbeforeat[jj-1] = pkbeforeat[jj];
					} else {
						outarray[pkbeforeat[kk]] = 0.0;
						for(jj = kk+1;jj < trofcnt; jj++) {
							pkbefore[jj-1] = pkbefore[jj];
							pkbeforeat[jj-1] = pkbeforeat[jj];
							notch[jj-1] = notch[jj];
							notchat[jj-1] = notchat[jj];
						}
						pkbefore[jj-1] = pkbefore[jj];
						pkbeforeat[jj-1] = pkbeforeat[jj];
					}
				} else {
					//ELIMINATE PEAK BEFORE	AND NOTCH				//			x
					outarray[pkbeforeat[kk]] = 0.0;					//	   o   x
					outarray[notchat[kk]] = 0.0;					//	  x x x
					for(jj = kk+1;jj < trofcnt; jj++) {				//	 x   o
						pkbefore[jj-1] = pkbefore[jj];				//  x
						pkbeforeat[jj-1] = pkbeforeat[jj];
						notch[jj-1] = notch[jj];
						notchat[jj-1] = notchat[jj];
					}
					pkbefore[jj-1] = pkbefore[jj];
					pkbeforeat[jj-1] = pkbeforeat[jj];
				}
				kk--;
				trofcnt--;
			} else if(pkbefore[kk+1] - notch[kk] < minnotch) {		//  x
				//ELIMINATE PEAK AFTER AND NOTCH					//	 x   o
				outarray[pkbeforeat[kk+1]] = 0.0;					//	  x x x
				outarray[notchat[kk]] = 0.0;						//	   o   x
				if(kk < trofcnt-1) {								//		    x
					notch[kk] = notch[kk+1];
					notchat[kk] = notchat[kk+1];
				}
				for(jj = kk+2;jj < trofcnt; jj++) {
					pkbefore[jj-1] = pkbefore[jj];
					pkbeforeat[jj-1] = pkbeforeat[jj];
					notch[jj-1] = notch[jj];
					notchat[jj-1] = notchat[jj];
				}
				pkbefore[jj-1] = pkbefore[jj];
				pkbeforeat[jj-1] = pkbeforeat[jj];
				kk--;
				trofcnt--;
			}
		}
	}
	writeat = 0;							//	position of write to output
	
	/*	TRIM THE PEAKS */
	if(trofcnt > 0) {
		for(kk = 0;kk < trofcnt;kk++) {
			nexttrofat = notchat[kk] - 1;	//	nexttrofat points to the PAIR of values: notchat points to its amplitude
			if(kk == 0)
				prepeak = pkbeforeat[kk];
			else
				prepeak = pkbeforeat[kk] - notchat[kk-1];
			postpeak = notchat[kk] - pkbeforeat[kk];
			notchwidth = min(prepeak,postpeak);
			pksttej = (pkbeforeat[kk] - notchwidth) - 1;
			ampsttej = pksttej + 1;
			sttval = parray[ampsttej];
			ampsttej += 2;
			while(parray[ampsttej] < sttval) {		// avoid any curling up again at peak edges
				sttval = parray[ampsttej] ;
				ampsttej += 2;
			}
			ampsttej -= 2;
			pksttej = ampsttej - 1;
			if(pksttej == pkendej)					//	Avoid peaks overlapping each other
				pksttej += 2;
			pkendej = (pkbeforeat[kk] + notchwidth) - 1;
			ampendej = pkendej + 1;
			endval = parray[ampendej];
			ampendej -= 2;
			while(parray[ampendej] < endval) {		// avoid any curling up again at peak edges
				endval = parray[ampendej] ;
				ampendej -= 2;
			}
			ampendej += 2;
			pkendej = ampendej - 1;
			while(writeat < pksttej) {
				outarray[writeat] = parray[writeat];
				writeat++;
				outarray[writeat] = 0.0;
				writeat++;
			}
			while(writeat < pkendej) {
				outarray[writeat] = parray[writeat];
				writeat++;
				outarray[writeat] = parray[writeat];
				writeat++;
			}
			while(writeat < nexttrofat) {
				outarray[writeat] = parray[writeat];
				writeat++;
				outarray[writeat] = 0.0;
				writeat++;
			}
		}
		nexttrofat = itemcnt;
		prepeak = pkbeforeat[kk] - notchat[kk-1];
		postpeak = (itemcnt + 1) - pkbeforeat[kk];
		notchwidth = min(prepeak,postpeak);
		pksttej = (pkbeforeat[kk] - notchwidth) - 1;
		if(pksttej == pkendej)
			pksttej += 2;
		pkendej = (pkbeforeat[kk] + notchwidth) - 1;
		while(writeat <pksttej) {
			outarray[writeat] = parray[writeat];
			writeat++;
			outarray[writeat] = 0.0;
			writeat++;
		}
		while(writeat <pkendej) {
			outarray[writeat] = parray[writeat];
			writeat++;
			outarray[writeat] = parray[writeat];
			writeat++;
		}
		while(writeat < nexttrofat) {
			outarray[writeat] = parray[writeat];
			writeat++;
			outarray[writeat] = 0.0;
			writeat++;
		}
	} else {
		while(writeat < itemcnt) {
			outarray[writeat] = parray[writeat];
			writeat++;
			outarray[writeat] = 0.0;
			writeat++;
		}
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

