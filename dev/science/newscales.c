/*
 *	Generating melodic (etc) output using tones of defined spectrum, in non-tempered tunings.
 *	
 *	Get list of notes, times, amplitudes, durs from textlisting or MIDI device: LOOM INTERFACE
 *	
 *	Map to a different temperament if required : LOOM INTERFACE
 *	
 *	Generate list of frequencies, amplitudes, durations required
 *	Find the MAXIMUM duration at each frequency
 *	
 *	Define the spectrum of the synthesized tones, 
 *		from a listing of harmonics and their relative amplitudes:	LOOM INTERFACE
 *
 *	Synthesis the max-duration(+safety note) required at each frequency
 *
 *	Cut shorter notes to size by editing the maxlength notes
 *		Here must associate each output file with appropriate output time/amp info (for mix)
 *
 *	Employ any envelope-shaping, vib, trem, jitter required
 *	Create a mixfile to mix the resulting events
 *	Check output level, reset mixamplitude
 *	Run mix
 */


// USAGE: newscales outfile datafile spectrumfile srate



#define SC_SYN_SRATE (0)
#define MAXLEVEL (0.9) 

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <cdpmain.h>
#include <synth.h>
#include <processno.h>
#include <modeno.h>
#include <pnames.h>
#include <flags.h>
#include <arrays.h>
#include <math.h>
#include <logic.h>
#include <speccon.h>
#include <sfsys.h>
#include <osbind.h>
#include <string.h>
//TW UPDATE
#include <limits.h>
#include <filetype.h>

#include <filetype.h>
#include <ctype.h>
#include <srates.h>


#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int	sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

/**************************** SYNTHESIS STAGE ****************************/

#define SC_SYN_TABSIZE 4096

static int generate_all_tones(double *frq,int frqcnt,int *duration,double **harm,double **amp, dataptr dz);
static int  gentable(double *tab,int tabsize,dataptr dz);
static int gen_sound(double frq, double *tab,int sampdur,double sr,double *harm,double *amp, int harmcnt,dataptr dz);
static double getval(double *tab,int i,double fracstep,double amp);
static void advance_in_table(double frq,int *i,double convertor,double *step,double *fracstep,double dtabsize);
static read_srate(char *str,dataptr dz);

static int setup_newscales_application(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int read_spectrum_data(char **cmdline,double ***harm,double ***amp,double maxfrq,dataptr dz);
static int read_newscales_data(char **cmdline,double **frq,int **duration,double *maxfrq,dataptr dz);
static int establish_application(dataptr dz);

/*
 *	Params must be a special data file containing
 *	(1) A list of frequencies to synthesize (each required frq synthd only ONCE
 *	(2) A list of their durations (these are > maximum duration used by any frequency in the original input melody)
 *	(3) A list of harmonics
 *	(4)	A list of harmonic amplitudes
 */


/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
	char **cmdline;
	int  cmdlinecnt;
	int n;
	int is_launched = FALSE;

	double *frq, **harm, **amp, maxfrq;
	int *duration;
	
	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
						/* CHECK FOR SOUNDLOOM */
//	if((sloom = sound_loom_in_use(&argc,&argv)) > 1) {
//		sloom = 0;
//		sloombatch = 1;
//	}
	if(sflinit("cdp")){
		sfperror("cdp: initialisation\n");
		return(FAILED);
	}
						  /* SET UP THE PRINCIPLE DATASTRUCTURE */
	if((exit_status = establish_datastructure(&dz))<0) {					// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if(argc < 4 || argc > 5) {
		usage2("dummy");	
		return(FAILED);
	}
	if(argc == 5){
		if((exit_status = read_srate(argv[4],dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else
		dz->infile->srate = 44100;
	dz->nyquist = dz->infile->srate/2.0;
	cmdline    = argv;
	cmdlinecnt = argc;
	if((exit_status = setup_newscales_application(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	cmdline++;
	cmdlinecnt--;
	if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = read_newscales_data(cmdline,&frq,&duration,&maxfrq,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	cmdline++;
	cmdlinecnt--;
	if((exit_status = read_spectrum_data(cmdline,&harm,&amp,maxfrq,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	dz->infile->channels = 1;
	is_launched = TRUE;
	dz->bufcnt = 1;
	if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
		return(MEMORY_ERROR);
	}
	if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0;n <dz->bufcnt; n++)
		dz->sampbuf[n] = dz->sbufptr[n] = (float *)0;
	dz->sampbuf[n] = (float *)0;

	if((exit_status = create_sndbufs(dz))<0) {							// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = generate_all_tones(frq,dz->itemcnt,duration,harm,amp,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = complete_output(dz))<0) {										// CDP LIB
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	exit_status = print_messages_and_close_sndfiles(FINISHED,is_launched,dz);		// CDP LIB
	free(dz);
	return(SUCCEEDED);
}


int generate_all_tones(double *frq,int frqcnt,int *duration,double **harm,double **amp,dataptr dz)
{
	int exit_status;
	double *tab;
	int n, sampdur;
	double sr = (double)dz->infile->srate;
	char outfilename[200], temp[4];
	if((dz->insams = (int *)malloc(1 * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory for insams arrays.\n");
		return(MEMORY_ERROR);
	}
	if((tab = (double *)malloc((SC_SYN_TABSIZE+1) * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory for wave table.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = gentable(tab,SC_SYN_TABSIZE,dz))<0)
		return(exit_status);
	n = 0;
	while(n < frqcnt) {
		strcpy(outfilename,dz->outfilename);
		sprintf(temp,"%d",n);
		strcat(outfilename,temp);
		strcat(outfilename,".wav");
		sampdur = duration[n];
		dz->insams[0] = duration[n]; 	
		dz->process_type = EQUAL_SNDFILE;	/* allow sndfile to be created */
		if((exit_status = create_sized_outfile(outfilename,dz))<0) {
			sprintf(errstr, "Failed to open file %s\n",outfilename);
			/*free(outfilename);*/
			dz->ofd = -1;
			dz->process_type = OTHER_PROCESS;
			return(GOAL_FAILED);
		}
		reset_filedata_counters(dz);
//HEREH, DEPENDING ON WHICH NOTE IT IS, WE GET THE HARMONIC COUNT FROM wordcnt[n],
		
		if((exit_status = gen_sound(frq[n],tab,sampdur,sr,harm[n],amp[n],dz->wordcnt[n],dz))<0) {
			fprintf(stdout,"Synthesis %d failed.\n",n+1);
			fflush(stdout);
			return(GOAL_FAILED);
		}
		if((exit_status = headwrite(dz->ofd,dz))<0) {
			fprintf(stdout,"Failed to write header to synthed output %d.\n",n+1);
			fflush(stdout);
			return(exit_status);
		}
		n++;
		if((exit_status = reset_peak_finder(dz))<0) {
			if(n < frqcnt) {
				fprintf(stdout,"Failed to reset peakfinder for synthesis %d.\n",n+1);
				fflush(stdout);
				return(exit_status);
			}
		}
		if(sndcloseEx(dz->ofd)<0) {
			sprintf(errstr,"Failed to close output soundfile for synthesis %d.\n",n);
			return(SYSTEM_ERROR);
		}
		dz->ofd = -1;
	}
	return(FINISHED);
}	

/******************************* GENTABLE ******************************/

int gentable(double *tab,int tabsize,dataptr dz)
{   
	int n;
	double step;
	step = (2.0 * PI)/(double)tabsize;
	for(n=0;n<tabsize;n++)
		tab[n] = sin(step * (double)n) * F_MAXSAMP;
	tab[tabsize] = 0.0;
	return(FINISHED);
}

/****************************** GEN_SOUND *************************/

int gen_sound(double frq, double *tab,int sampdur,double sr,double *harm,double *amp, int harmcnt,dataptr dz)
{
	int exit_status;
	double inverse_sr = 1.0/sr;
	double *step;
	double dtabsize = (double)SC_SYN_TABSIZE;
	double convertor = dtabsize * inverse_sr, maxsamp, normaliser;
	int n, m, *i;
	int chans = dz->infile->channels;
	double harmfrq, harmamp;
	double *fracstep, val;
	int do_start = 1;
	int synth_splicelen = SYNTH_SPLICELEN * chans;
	int total_samps = 0;
	int startj = 0, endj = SYNTH_SPLICELEN;
	int total_samps_left = sampdur;
	int endsplicestart = sampdur - synth_splicelen;
	int todo;
	if((step = (double *)malloc(harmcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory all harmonic wave table step counters.\n");
		return(MEMORY_ERROR);
	}
	if((fracstep = (double *)malloc(harmcnt * sizeof(double)))==NULL) {
		sprintf(errstr,"Insufficient memory all harmonic wave table step counters.\n");
		return(MEMORY_ERROR);
	}
	if((i = (int *)malloc(harmcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"Insufficient memory all harmonic wave table position indicators.\n");
		return(MEMORY_ERROR);
	}
	for(n = 0;n < harmcnt;n++) {
		step[n] = 0.0;
		fracstep[n] = 0.0;
		i[n] = 0;
	}

	if(sampdur < (synth_splicelen * 2) + chans) {
		fprintf(stdout,"ERROR: Specified output duration is less then available splicing length.\n");
		return(DATA_ERROR);
	}
	maxsamp = 0.0;

	/* FIRST PASS: ADD UP ALL HARMONICS, AND GET MAX OUTPUT LEVEL */
	
	while(total_samps_left > 0) {
		memset((char *)dz->bigbuf,0,dz->buflen * sizeof(float));
		if(total_samps_left/dz->buflen <= 0)
			todo = total_samps_left;
		else
			todo = dz->buflen;
		for(n = 0;n < harmcnt;n++) {
			harmfrq = frq * harm[n];
			harmamp = amp[n];
			m = 0;
			while(m < todo) {
				val = getval(tab,i[n],fracstep[n],harmamp);
				dz->bigbuf[m] = (float)(dz->bigbuf[m] + val);
				m++;
				advance_in_table(harmfrq,&(i[n]),convertor,&(step[n]),&(fracstep[n]),dtabsize);
			}
		}
		m = 0;
		while(m < todo) {
			if(fabs(dz->bigbuf[m]) > maxsamp)
				maxsamp = fabs(dz->bigbuf[m]);
			m++;
		}	
		total_samps_left -= dz->buflen;
	}
	normaliser = 1.0;
	if(maxsamp > MAXLEVEL)
		normaliser = MAXLEVEL/maxsamp;
	total_samps_left = sampdur;
	while(total_samps_left > 0) {
		memset((char *)dz->bigbuf,0,dz->buflen * sizeof(float));
		if(total_samps_left/dz->buflen <= 0)
			todo = total_samps_left;
		else
			todo = dz->buflen;
		for(n = 0;n < harmcnt;n++) {
			harmfrq = frq * harm[n];
			harmamp = amp[n];
			m = 0;
			while(m < todo) {
				val = getval(tab,i[n],fracstep[n],harmamp);
				dz->bigbuf[m] = dz->bigbuf[m] + (float)val;
				m++;
				advance_in_table(harmfrq,&(i[n]),convertor,&(step[n]),&(fracstep[n]),dtabsize);
			}
		}
		m = 0;
		while(m < todo) {
			dz->bigbuf[m] = (float)(dz->bigbuf[m] * normaliser);
			if(do_start) {
				dz->bigbuf[m] = (float)(dz->bigbuf[m] * (startj++/(double)SYNTH_SPLICELEN));
				if(startj >= SYNTH_SPLICELEN)
					do_start = 0;
			}
			if(total_samps >= endsplicestart)
				dz->bigbuf[m] = (float)(dz->bigbuf[m] * (endj--/(double)SYNTH_SPLICELEN));
			total_samps++;
			m++;
		}
		if(todo) {
			if((exit_status = write_samps(dz->bigbuf,todo,dz))<0)
				return(exit_status);
		}
		total_samps_left -= dz->buflen;
	}
	return(FINISHED);
}
  
/****************************** GETVAL *************************/

double getval(double *tab,int i,double fracstep,double amp)
{
	double diff, val  = tab[i];
	
	diff = tab[i+1] - val;
	val += diff * fracstep;
	val *= amp;
	return(val);
}

/****************************** ADVANCE_IN_TABLE *************************/

static void advance_in_table(double frq,int *i,double convertor,double *step,double *fracstep,double dtabsize)
{
	*step += frq * convertor;
	while(*step >= dtabsize)
		*step -= dtabsize;
	*i = (int)(*step); /* TRUNCATE */
	*fracstep = *step - (double)(*i);
}

/************************ HANDLE_THE_OUTFILE *********************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
	int len;
	char *filename = (*cmdline)[0], *p, *q, *full_filename;
	if(filename[0]=='-' && filename[1]=='f') {
		dz->floatsam_output = 1;
		dz->true_outfile_stype = SAMP_FLOAT;
		filename+= 2;
	}
	full_filename = filename;
	len = strlen(filename);
	q = filename;
	p = filename + len;
	p--;
	while(p >= q) {		/* Sesparate off path */
		if(*p == '/' || *p == '\\') {
			filename = p+1;
			break;
		}
		p--;
	}
	if((len = strlen(filename)) <= 0) {
		sprintf(errstr,"Invalid outfilename (path only).\n");
		return(MEMORY_ERROR);
	}
	q = filename;
	len = strlen(filename);
	p = filename + len;
	p--;
	while(p >= q) {
		if(*p == '.') {		/* Snip off file extension */
			*p = ENDOFSTR;
			break;
		}
		p--;
	}
	if((len = strlen(filename)) <= 0) {
		sprintf(errstr,"Invalid outfilename.\n");
		return(MEMORY_ERROR);
	}
	p = filename + len;
	p--;
	if(isdigit(*p)) {
		sprintf(errstr,"Invalid outfilename (ends in number).\n");
		return(MEMORY_ERROR);
	}
	filename = full_filename;
	len = strlen(filename);
	len++;
	if((dz->outfilename = (char *)malloc(len * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store output filename.\n");
		return(MEMORY_ERROR);
	}
	strcpy(dz->outfilename,filename);	   
	(*cmdline)++;
	(*cmdlinecnt)--;
	return(FINISHED);
}

/************************* SETUP_NEWSCALES_APPLICATION *******************/

int setup_newscales_application(dataptr dz)
{
	int exit_status;
	aplptr ap;
	if((exit_status = establish_application(dz))<0)		// GLOBAL
		return(FAILED);
	ap = dz->application;
	// set_legal_infile_structure -->
	dz->has_otherfile = FALSE;
	// assign_process_logic -->
	dz->input_data_type = NO_FILE_AT_ALL;
	dz->process_type	= UNEQUAL_SNDFILE;	
	dz->outfiletype  	= SNDFILE_OUT;
	return(FINISHED);
}

/************************* READ_SRATE *******************/

int read_srate(char *str,dataptr dz)
{
	int dummy;
	if(sscanf(str,"%d",&dummy) != 1) {
		sprintf(errstr,"Failed to read output sample rate.\n");
		return(DATA_ERROR);
	}
	if(BAD_SR(dummy)) {
		sprintf(errstr,"Invalid sample rate.\n");
		return(DATA_ERROR);
	}
	dz->infile->srate = dummy;
	return(FINISHED);
}

/************************* READ_NEWSCALES_DATA *******************/

int read_newscales_data(char **cmdline,double **frq,int **duration,double *maxfrq,dataptr dz)
{
	double *f, dummy;
	int n = 0, dur, *d;
	char temp[200], *q;
	int isfrq;
	FILE *fp;
	if((fp = fopen(cmdline[0],"r"))==NULL) {
		sprintf(errstr,	"Can't open datafile %s to read data.\n",cmdline[0]);
		return(DATA_ERROR);
	}
	n = 0;
	while(fgets(temp,200,fp)==temp) {	 /* READ AND TEST VALS */
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&q,&dummy)) {
			n++;
		}
	}	    
	if(n <= 0) {
		sprintf(errstr,"NO DATA Found in datafile.\n");
		return(DATA_ERROR);
	}
	if(ODD(n)) {
		sprintf(errstr,"DATA INCORRECTLY PAIRED in datafile %s.\n",cmdline[0]);
		return(DATA_ERROR);
	}
	n = n/2;
	dz->itemcnt = n;
	if((*frq = (double *)malloc(n * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store note frequencies.\n");
		return(MEMORY_ERROR);
	}
	if((*duration = (int *)malloc(n * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store note max-durations.\n");
		return(MEMORY_ERROR);
	}
	fseek(fp,0,0);
	f = *frq;
	d = *duration;
	n = 0;
	isfrq = 1;
	*maxfrq = 0.0;
	while(fgets(temp,200,fp)==temp) {	 /* READ AND STORE VALS */
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		while(get_float_from_within_string(&q,&dummy)) {
			if(isfrq) {
				if(dummy <= 5) {
					sprintf(errstr,"Frq <= 5: too low for this application.\n");
					return(DATA_ERROR);
				}
				if(dummy > *maxfrq)
					*maxfrq = dummy;
				*f = dummy;
				f++;
			} else {
				if(dummy > 3600) {
					sprintf(errstr,"Duration greater than an hour: too long for this application.\n");
					return(DATA_ERROR);
				} else if (dummy <= 0.03) {
					sprintf(errstr,"Duration less than30 mS: too short for this application.\n");
					return(DATA_ERROR);
				}
				dur = (int)(round(dummy * dz->infile->srate));
				*d = dur;
				d++;
			}
			isfrq = !isfrq;
		}
	}
	return(FINISHED);
}

/************************* READ_SPECTRUM_DATA *******************/

int read_spectrum_data(char **cmdline,double ***harm,double ***amp,double maxfrq,dataptr dz)
{
	double *h, *a, dummy;
	int harmcnt, linecnt, k;
	char temp[200], *q;
	int isharm;
	FILE *fp;
	if((fp = fopen(cmdline[0],"r"))==NULL) {
		sprintf(errstr,	"Can't open datafile %s to read data.\n",cmdline[0]);
		return(DATA_ERROR);
	}
	harmcnt = 0;
	linecnt = 0;
	while(fgets(temp,200,fp)==temp) {	 /* READ AND TEST VALS */
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		k = 0;
		while(get_float_from_within_string(&q,&dummy)) {
			k++;
		}
		if(k == 0) {
			continue;
		}
		linecnt++;
	}
	if(linecnt != 1 && linecnt != dz->itemcnt) {
		sprintf(errstr,"No. of lines of spectral data (%d) does NOT = no. of notes entered (%d).\n",linecnt,dz->itemcnt);
		return(DATA_ERROR);
	}
	if((*harm = (double **)malloc(dz->itemcnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store harmonics info.\n");
		return(MEMORY_ERROR);
	}
	if((*amp = (double **)malloc(dz->itemcnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store amplitudes of harmonics.\n");
		return(MEMORY_ERROR);
	}
	if((dz->wordcnt = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store count of harmonics for each note.\n");
		return(MEMORY_ERROR);
	}
	linecnt = 0;
	fseek(fp,0,0);
	while(fgets(temp,200,fp)==temp) {	 /* READ AND TEST VALS */
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		k = 0;
		while(get_float_from_within_string(&q,&dummy)) {
			k++;
		}
		if(k == 0) {
			continue;
		}
		if(k == 0 || ODD(k)) {
			sprintf(errstr,"SPECTRAL DATA INCORRECTLY PAIRED in datafile %s line %d\n",cmdline[0],linecnt+1);
			return(DATA_ERROR);
		}
		k /= 2;
		if(((*harm)[linecnt] = (double *)malloc(k * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store harmonics info for note %d.\n",linecnt+1);
			return(MEMORY_ERROR);
		}
		if(((*amp)[linecnt] = (double *)malloc(k * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to store amplitudes of harmonics for note %d.\n",linecnt+1);
			return(MEMORY_ERROR);
		}
		dz->wordcnt[linecnt] = k;
		linecnt++;
	}						/* IF ONLY ONE SPECTRAL DATA LINE, ASSIGN SAME SPECTRUM TO ALL INPUT NOTES */
	fseek(fp,0,0);
	isharm = 1;
	linecnt = 0;
	while(fgets(temp,200,fp)==temp) {	 /* READ AND STORE VALS */
		h = (*harm)[linecnt];
		a = (*amp)[linecnt];
		q = temp;
		if(*q == ';')	//	Allow comments in file
			continue;
		k = 0;
		while(get_float_from_within_string(&q,&dummy)) {
			if(isharm) {
				if(dummy < 1) {
					sprintf(errstr,"Invalid harmonic value (< 1).\n");
					return(DATA_ERROR);
				} else if(dummy * maxfrq > dz->nyquist) {
					sprintf(errstr,"Harmonic value (%lf) too high for max frequency (%lf) being used\n",dummy,maxfrq);
					return(DATA_ERROR);
				}
				*h = dummy;
				h++;
			} else {
				if(dummy > 1 || dummy <= 0) {
					sprintf(errstr,"Harmonic amplitude out of range (>0 - 1).\n");
					return(DATA_ERROR);
				}
				*a = dummy;
				a++;
			}
			isharm = !isharm;
		}
		linecnt++;
	}	    
	if(linecnt == 1) {
		while(linecnt < dz->itemcnt) {
			(*harm)[linecnt] = (*harm)[0];
			(*amp)[linecnt]  = (*amp)[0];
			dz->wordcnt[linecnt] = dz->wordcnt[0];
			linecnt++;
		}
	}	
	return(FINISHED);
}

int usage2(char *dummy)
{
	fprintf(stderr,"USAGE: newscales outfile datafile spectrumfile [srate]\n");
	fprintf(stderr,"outfile      Generic name for output soundfiles (must not end with a number)\n");
	fprintf(stderr,"datafile     Frq-amp pairs defining note-events in output\n");
	fprintf(stderr,"spectrumfile Harmonicnumber-amplitude pairs, define spectrum of note.\n");
	fprintf(stderr,"             One line for all notes\n");
	fprintf(stderr,"             OR\n");
	fprintf(stderr,"             One line for each note in the frq-list.\n");
	fprintf(stderr,"            (First line corresponds to lowest note, etc.)\n");
	return(FAILED);
}

/****************************** ESTABLISH_APPLICATION *******************************/

int establish_application(dataptr dz)
{
	aplptr ap;
	if((dz->application = (aplptr)malloc(sizeof (struct applic)))==NULL) {
		sprintf(errstr,"establish_application()\n");
		return(MEMORY_ERROR);
	}
	ap = dz->application;
	memset((char *)ap,0,sizeof(struct applic));
	return(FINISHED);
}

int assign_process_logic(dataptr dz)
{
	return(FINISHED);
}
void set_legal_infile_structure(dataptr dz)
{
}
int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
	return(FINISHED);
}
int setup_internal_arrays_and_array_pointers(dataptr dz)
{
	return(FINISHED);
}
int establish_bufptrs_and_extra_buffers(dataptr dz)
{
	return(FINISHED);
}
int inner_loop(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
	return(FINISHED);
}
int get_process_no(char *str,dataptr dz)
{
	return(FINISHED);
}
int usage3(char *str1,char *str2)
{
	return(FINISHED);
}
int usage1()
{
	return(FINISHED);
}
int read_special_data(char *str,dataptr dz)
{
	return(FINISHED);
}

//***************************** APPLICATION_INIT **************************/
//
//int application_init(dataptr dz)
//{
//	int exit_status;
//	int storage_cnt;
//	int tipc, brkcnt;
//	aplptr ap = dz->application;
//	if(ap->vflag_cnt>0)
//		initialise_vflags(dz);	  
//	tipc  = ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt;
//	ap->total_input_param_cnt = (char)tipc;
//	if(tipc>0) {
//		if((exit_status = setup_input_param_range_stores(tipc,ap))<0)			  
//			return(exit_status);
//		if((exit_status = setup_input_param_defaultval_stores(tipc,ap))<0)		  
//			return(exit_status);
//		if((exit_status = setup_and_init_input_param_activity(dz,tipc))<0)	  
//			return(exit_status);
//	}
//	brkcnt = tipc;
//
//	if(dz->input_data_type==BRKFILES_ONLY 					   
//	|| dz->input_data_type==UNRANGED_BRKFILE_ONLY 
//	|| dz->input_data_type==DB_BRKFILES_ONLY 
//	|| dz->input_data_type==SNDFILE_AND_BRKFILE 
//	|| dz->input_data_type==SNDFILE_AND_UNRANGED_BRKFILE 
//	|| dz->input_data_type==SNDFILE_AND_DB_BRKFILE 
//	|| dz->input_data_type==ALL_FILES
//	|| dz->input_data_type==ANY_NUMBER_OF_ANY_FILES
//	|| dz->outfiletype==BRKFILE_OUT) {
//		dz->extrabrkno = brkcnt;			  
//		brkcnt++;		/* extra brktable for input or output brkpntfile data */
//	}
//	if(brkcnt>0) {
//		if((exit_status = setup_and_init_input_brktable_constants(dz,brkcnt))<0)			  
//			return(exit_status);
//	}
//
//	if((storage_cnt = tipc + ap->internal_param_cnt)>0) {		  
//		if((exit_status = setup_parameter_storage_and_constants(storage_cnt,dz))<0)	  
//			return(exit_status);
//		if((exit_status = initialise_is_int_and_no_brk_constants(storage_cnt,dz))<0)	  
//			return(exit_status);
//	}													   
// 	if((exit_status = mark_parameter_types(dz,ap))<0)	  
//			return(exit_status);
//
//	if((exit_status = establish_infile_constants(dz))<0)
//		return(exit_status);
//	if((exit_status = establish_bufptrs_and_extra_buffers(dz))<0)	
//		return(exit_status);
//	if((exit_status = setup_internal_arrays_and_array_pointers(dz))<0)	 
//		return(exit_status);
//	return(FINISHED);
//}


