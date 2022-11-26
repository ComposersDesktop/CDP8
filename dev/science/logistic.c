#include <stdio.h>
#include <stdlib.h>
#include <sfsys.h>
#include <math.h>
#include <ctype.h>
#include <logic.h>


#include <Osbind.h>

#ifdef unix
#define round(x) lround((x))
#else
#define round(x) cdp_round((x))
#endif

//logistic_cycle_min(1) 0
//logistic_cycle_max(1) 2.893
//logistic_cycle_min(2) 2.894
//logistic_cycle_max(2) 3.398
//logistic_cycle_min(4) 3.399
//logistic_cycle_max(4) 3.527
//logistic_cycle_min(8) 3.528
//logistic_cycle_max(8) 3.560
//logistic_cycle_min(16) 3.561
//logistic_cycle_max(16) 3.567
//logistic_cycle_min(32) 3.568
//logistic_cycle_max(32) 3.569
//logistic_cycle_max(64) 3.570
//logistic_cycle_max(64) 3.571
//logistic_cycle_max(ch) 3.572
//logistic_cycle_max(ch) 4.000

#define CONTINUE      (1)
#define FINISHED      (0)
#define	DATA_ERROR	  (-3)	/* Data is unsuitable, incorrect or missing */


#define FALSE 0
#define TRUE 1
#define	PERROR 0.0002
#define	FLTERR 0.000002
#define XSTEP 0.001
#define ENDOFSTR ('\0')
#define NEWLINE ('\n')

//	For a 3 8va range, we have 36 semitones and 3600 cents 
//	We will assume that tunings less than 1 cent apart or in fact equal ... this gives an error bound of 0.0002
//	FLTERR is considerably more demanding than that

static int cyclic(double *iterates,double *setareti,int *cnt,int tailend);
static void usage(void);
static int flteq(double f1,double f2);
static int get_r_range(char *str,double *rlim);
static int get_tstep_range(char *str,double *tstep);
static int  get_float_from_within_string(char **str,double *val);

int main(int argc,char *argv[])
{
	int exit_status, cflag = 0;
	int cnt, tailend, limitend;
	char temp[200], filename[2000], thisfilename[2000];
	int n, m, cycles, use_limitdur = 0;
	int do_output, iterating;
	double r, rr, x, y, rlim[2], tstep[2],rrange, rstep, tsteprange =0.0;
	double iterates[32768], setareti[32768];
	double timestep, timerand, taillen, limitdur, time, thistime, trand, pmin, pmax, prang, pval;
	int datacount, seed;
	FILE *fp;
	if(argc < 11 || argc > 12)
		usage();

	if(sscanf(argv[1],"%s",filename)!=1) {
		fprintf(stdout,"INFO: Invalid generic output filename.\n");
		fflush(stdout);
		exit(1);
	}
	if((exit_status = get_r_range(argv[2],rlim)) == CONTINUE) {
		if(sscanf(argv[2],"%lf",&r)!=1) {
			fprintf(stdout,"INFO: Invalid logistic constant.\n");
			fflush(stdout);
			exit(1);
		} 
		if(r <= 0.0 || r > 4.0) {
			fprintf(stdout,"INFO: Logistic constant out of range (>0 - 4).\n");
			fflush(stdout);
			exit(1);
		} 
		if(r > 3.571)
			use_limitdur = 1;
		rlim[0] = r;
		rlim[1] = r;
	} else if(exit_status < 0)
		exit(1);
	if(rlim[0] > rlim[1]) {
		rr = rlim[0];
		rlim[0] = rlim[1];
		rlim[1] = rr;
	}

	if((exit_status = get_tstep_range(argv[3],tstep)) == CONTINUE) {
		if(sscanf(argv[3],"%lf",&timestep)!=1) {
			fprintf(stdout,"INFO: Invalid timestep.\n");
			fflush(stdout);
			exit(1);
		} 
		if(timestep <= 0.02 || timestep > 1.0) {
			fprintf(stdout,"INFO: Timestep out of range (0.02 to 1)\n");
			fflush(stdout);
			exit(1);
		} 
		tstep[0] = timestep;
		tstep[1] = timestep;
	} else if(exit_status < 0)
		exit(1);
	tsteprange = tstep[1] - tstep[0];
	if(sscanf(argv[4],"%lf",&timerand)!=1) {
		fprintf(stdout,"INFO: Invalid timerand\n");
		fflush(stdout);
		exit(1);
	} 
	if(timerand < 0.0 || timerand >= 1) {
		fprintf(stdout,"INFO: Timerand out of range (0 to <1)\n");
		fflush(stdout);
		exit(1);
	} 
	if(sscanf(argv[5],"%lf",&taillen)!=1) {
		fprintf(stdout,"INFO: Invalid tail length\n");
		fflush(stdout);
		exit(1);
	} 
	if(taillen < 0.0 || taillen > 60.0) {
		fprintf(stdout,"INFO: taillength out of range (0-60)\n");
		fflush(stdout);
		exit(1);
	} 
	if(taillen/tstep[0] > 32767.0) {
		fprintf(stdout,"INFO: This taillength with this timestep generates too many output values to store.\n");
		fflush(stdout);
		exit(1);
	} 
	if(sscanf(argv[6],"%lf",&limitdur)!=1) {
		fprintf(stdout,"INFO: Invalid tail length\n");
		fflush(stdout);
		exit(1);
	} 
	if(limitdur <= 0.0) {
		fprintf(stdout,"INFO: Invalid limit (must be > 0)\n");
		fflush(stdout);
		exit(1);
	} 
	if(limitdur/tstep[0] > 32767.0) {
		fprintf(stdout,"INFO: This limit with this timestep generates too many output values to store.\n");
		fflush(stdout);
		exit(1);
	} 
	if(sscanf(argv[7],"%d",&datacount)!=1) {
		fprintf(stdout,"INFO: Invalid datacount\n");
		fflush(stdout);
		exit(1);
	} 
	if(datacount <= 0) {
		fprintf(stdout,"INFO: Invalid output datacount (must be > 0)\n");
		fflush(stdout);
		exit(1);
	} 
	if(sscanf(argv[8],"%d",&seed)!=1) {
		fprintf(stdout,"INFO: Invalid seed value\n");
		fflush(stdout);
		exit(1);
	} 
	if(seed <= 0) {
		fprintf(stdout,"INFO: Invalid sedd value (must be > 0)\n");
		fflush(stdout);
		exit(1);
	} 
	if(sscanf(argv[9],"%lf",&pmin)!=1) {
		fprintf(stdout,"INFO: Invalid minimum MIDI pitch\n");
		fflush(stdout);
		exit(1);
	} 
	if(pmin < 0 || pmin > 127) {
		fprintf(stdout,"INFO: Invalid minimum MIDI pitch\n");
		fflush(stdout);
		exit(1);
	} 
	if(sscanf(argv[10],"%lf",&pmax)!=1) {
		fprintf(stdout,"INFO: Invalid maximum MIDI pitch\n");
		fflush(stdout);
		exit(1);
	} 
	if(pmax < 0 || pmax > 127) {
		fprintf(stdout,"INFO: Invalid maximum MIDI pitch\n");
		fflush(stdout);
		exit(1);
	}
	if(pmin > pmax) {
		trand = pmin;
		pmin = pmax;
		pmax = trand;
	}
	prang = pmax - pmin;
	if(prang < 1) {
		fprintf(stdout,"INFO: MIDI pitch range less than a semitone.\n");
		fflush(stdout);
		exit(1);
	}
	if(argc == 12) {
		if(strcmp(argv[11],"-c")) {
			fprintf(stdout,"INFO: Invalid last parameter\n");
			fflush(stdout);
			exit(1);
		} else 
			cflag = 1;
	}

	tailend  = (int)round(taillen/tstep[0]);
	limitend = (int)round(limitdur/tstep[0]);


//	"USAGE: logistic outfilename R timestep timerand taillen limitdur datacount seed\n"
	
	srand(seed);
	rrange = rlim[1] - rlim[0];
	rstep = rrange/(double)datacount;
	rr = rlim[0];
	for(n = 0; n < datacount;n++) {
		sprintf(temp,"%d.txt",n+1);
		strcpy(thisfilename,filename);
		strcat(thisfilename,temp);
		x = drand48();
		cycles = 0;
		cnt = 0;
		iterating = 1;
		do_output = 0;
		timestep = (drand48() * tsteprange) + tstep[0];
		while(iterating) {
			iterates[cnt] = x;
			y = 1 - x;
			x = rr * x * y;
			cnt++;
			if(cnt >= limitend) {
				if(use_limitdur)
					do_output = 1;
				else {
					if(cflag < 2) {
						fprintf(stdout,"WARNING: Encountered non-cyclic data unexpectedly for r = %lf: Is limit too short??\n",rr);
						fflush(stdout);
					}
					if(cflag == 0)
						exit(1);
					else {
						cflag = 2;
						do_output = 1;
					}
				}
			} else if(cnt > 2) {
				if((cycles = cyclic(iterates,setareti,&cnt,tailend)) > 0)
					do_output = 1;
			}
			if(do_output) {
				fp = fopen(thisfilename,"w");
				time = 0.0;
				thistime = time;
				for(m = 0; m < cnt;m++) {
					pval = (iterates[m] * prang) + pmin;
					sprintf(temp,"%lf\t%lf\n",thistime,pval);
					fputs(temp,fp);
					time += timestep;
					thistime = time;
					if(timerand > 0.0) {
						trand = ((drand48() * 2.0) - 1.0);	//	Range +- 1
						trand *= timerand/2.0;				//	Range +- 1/2 * timerand (= max rand deviation)
						trand *= timestep;					//	fraction of actual step between times
						thistime += trand;
					}
				}
				fclose(fp);
				iterating = 0;
			}
		}
		rr += rstep;
		rr = min(rr,rlim[1]);
		if(rr > 3.571)
			use_limitdur = 1;
	}
}

/**************************** CYCLIC *******************************/

int cyclic(double *iterates,double *setareti,int *cnt,int tailend)
{
	int k, m, j = 0, OK = 0, precyc;
	double nowval;
	for(k = *cnt - 1,m= 0; k >= 0;k--,m++)
		setareti[m] = iterates[k];	//	Copy the sequence in reverse order (the last value becomes first etc)
	for(m = 1; m < 64; m*=2) {		//	For every possible step between outputs (step by 1, by 2, by 4 etc)
		if(m > (*cnt)/2)			//	Cycle can involve max of half total number of entries
			return 0;
		OK = 1;						//	It's period doubling were looking for, so each group of2,4,8 etc items should cycle
		for(j = 0;j < m;j++) {		//	For every item 0,1,2 ... M-1
			nowval = setareti[j];	//	get the (start)value
			for(k = j+m; k < m*2; k += m) {	//	For every item that is step-m away, compare it with startvalue
				if(!flteq(nowval,setareti[k])) {
					OK = 0;			//	If they're not equal, we don't have a cycle of length m, so set OK to 0
					break;
				}
			}
			if(!OK)					//	If we don't have a cycle of length m, go to next value of m (2 times bigger)
				break;
		}
		if(OK)						//	If all the j cycles checked out, we DO have a cycle of length m, so quit
			break;
	}
	if(m >= 64)						//	If we never found any cycle, m will have become >= 128: so return 0
		return 0;	
	if(!OK)							//	Should be redundant
		return 0;

	//	Find start of cycling behaviour Using the found cycle value (m)
	precyc = 0;						//	move through the array, finding where the cycling really began
	while(precyc <= *cnt - (m*2)) {
		for(j = precyc;j < m+precyc;j++) {				//	For every item precyc + 0,1,2 ... M-1
			nowval = setareti[j];						//	get the (start)value
			for(k = j+m; k < precyc+(m*2); k += m) {	//	For every item that is step-m away, compare it with startvalue
				if(!flteq(nowval,setareti[k])) {
					OK = 0;			//	If they're not equal, we don't have a cycle of length m, so set OK to 0
					break;
				}
			}
			if(!OK)
				break;
		}
		if(!OK)
			break;
		precyc += m;				// precyc gets set at point in (reverse) array where cycling stops
	}								// So (cnt - precyc) is where cycliung begins

	if(precyc > tailend)			//	If cycling is longer than required tail ... shorten it
		*cnt -= (precyc - tailend);
	else if(precyc < tailend) {		//	Else if tailend needs to be longer ...
		k = tailend - precyc;		//	Additional length
		if((k += *cnt) >= 32768)	//	New end-length
			k = 32768;				//	Set true end of output array, with tail added
		j = *cnt - m;				//	Copy the endcycle to beyond current data end
		while(*cnt < k)
			iterates[(*cnt)++] = iterates[j++];
	}
	return m;
}

/**************************** USAGE *******************************/

void usage()
{
	fprintf(stdout,
	"USAGE: \n"
	"logistic outname R timestep timerand taillen limit datacount seed pmin pmax [-c]\n"
	"\n"
	"Calculate the iterations of the logistic equation,\n"
	"for a specific value (or range of vals) of the logistic equation constant 'R'.\n"
	"Associate the successive iterated vals with (untempered) MIDI-pitches,\n"
	"and with times, which start at zero, calculating further time values\n"
	"using \"timestep\" and \"timerand\".\n"
	"\n"
	"The output (of one pass) is a textfile with a list of time:MIDIval pairs.\n"
	"The process runs \"DATACOUNT\" times, outputting \"datacount\" textfiles,\n"
	"with different (random) starting values for each iteration.\n"
	"\n"
	"Once cyclical behaviour sets in,\n"
	"the process continue outputting values for a specified duration (\"TAILLEN\").\n"
	"If \"R\" is in the chaotic range (>=3.571), cyclical behaviour is unlikely,\n"
	"so an output of duration \"LIMIT\" is produced.\n"
	"\n"
	"\"R\" can be a number or a textfile with 2 numbers - bottom and top of range.\n"
	"If input as range, successive iterations increase \"R\" from bottom to top of range.\n"
	"\n"
	"\"timestep\" can be numeric or textfile with 2 numbers - bottom & top of range.\n"
	"If input as range, successive iterations choose timestep at random within range.\n"
	"\n"
	"The output values in the iteration process lie in the range 0 to 1.\n"
	"These will be mapped into the MIDI pitch range defined by PMIN and PMAX.\n"
	"\n"
	"OUTNAME     generic name for output data files.\n"
	"            These will take names \"myname1\",\"myname2\" etc.\n"
	"R           The constant in the logistic equation (Range 0-4).\n"
	"TIMESTEP    (average) timestep between output values (Range 0.02 to 1).\n"
	"TIMERAND    Randomisation of times: Range 0 - 1.\n"   
	"TAILLEN     Length of output once cycling begins.\n"
	"LIMIT       Maximum duration of output (for chaotic behaviour).\n"
	"            NB If set too short, some of \"DATACOUNT\" outputs may not be generated.\n"
	"DATACOUNT   Number of output sets to produce.\n"
	"SEED        Set seed value to initialise the rand selection.\n"
	"PMIN        Minimum MIDI pitch of output.\n"
	"PMAX        Maximum MIDI pitch of output.\n"
	"-c          If noncyclic data encountered unexpectedly, warn but continue.\n"
	"            (Default, unexpected non-cyclic data causes process to stop).\n");
	fflush(stdout);
	exit(1);
}

/**************************** FLTEQ *******************************/

int flteq(double f1,double f2)
{
	double upperbnd, lowerbnd;
	upperbnd = f2 + PERROR;		
	lowerbnd = f2 - PERROR;		
	if((f1>upperbnd) || (f1<lowerbnd))
		return(FALSE);
	return(TRUE);
}

/**************************** GET_R_RANGE ****************************/

int get_r_range(char *str,double *rlim)
{
	FILE *fp;
	double dummy;
	char temp[200], *p;
	int cnt = 0;

	if((fp = fopen(str,"r"))==NULL)
		return(CONTINUE);
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			if(cnt >= 2) {
				fprintf(stdout,"INFO: Data in \"r\" file should be just 2 values between 0 and 4\n");
				fflush(stdout);
				return(DATA_ERROR);
			}
			if(dummy < 0 || dummy > 4) {
				fprintf(stdout,"INFO: Data in \"r\" file should be just 2 values between 0 and 4\n");
				fflush(stdout);
				return(DATA_ERROR);
			}
			rlim[cnt] = dummy;
			cnt++;
		}
	}
	fclose(fp);
	if(cnt != 2) {
		fprintf(stdout,"INFO: Data in \"r\" file should be 2 values between 0 and 4\n");
		fflush(stdout);
		return(DATA_ERROR);
	}
	return FINISHED;
}

/**************************** GET_TSTEP_RANGE ****************************/

int get_tstep_range(char *str,double *tstep)
{
	FILE *fp;
	double dummy;
	char temp[200], *p;
	int cnt = 0;

	if((fp = fopen(str,"r"))==NULL)
		return(CONTINUE);
	while(fgets(temp,200,fp)!=NULL) {
		p = temp;
		while(isspace(*p))
			p++;
		if(*p == ';' || *p == ENDOFSTR)	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&p,&dummy)) {
			if(cnt >= 2) {
				fprintf(stdout,"INFO: Data in \"tiemstep\" file should be just 2 values between 0.02 and 1\n");
				fflush(stdout);
				return(DATA_ERROR);
			}
			if(dummy < 0.02 || dummy > 1) {
				fprintf(stdout,"INFO: Data in \"tiemstep\" file should be just 2 values between 0.02 and 1\n");
				fflush(stdout);
				return(DATA_ERROR);
			}
			tstep[cnt] = dummy;
			cnt++;
		}
	}
	fclose(fp);
	if(cnt != 2) {
		fprintf(stdout,"INFO: Data in \"timestep\" file should be 2 values between 0.02 and 1\n");
		fflush(stdout);
		return(DATA_ERROR);
	}
	if(tstep[0] > tstep[1]) {
		dummy = tstep[0];
		tstep[0] = tstep[1];
		tstep[1] = dummy;
	}
	return FINISHED;
}

/************************** GET_FLOAT_FROM_WITHIN_STRING **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

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
			return(FALSE);
		has_digits = TRUE;
		break;
	}
	p++;		
	while(!isspace(*p) && *p!=NEWLINE && *p!=ENDOFSTR) {
		if(isdigit(*p))
			has_digits = TRUE;
		else if(*p == '.') {
			if(++decimal_point_cnt>1)
				return(FALSE);
		} else
			return(FALSE);
		p++;
	}
	if(!has_digits || sscanf(valstart,"%lf",val)!=1)
		return(FALSE);
	*str = p;
	return(TRUE);
}
