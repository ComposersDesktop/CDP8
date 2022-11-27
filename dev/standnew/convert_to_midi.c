/*
 * CREATE STANDARD MIDI FILE FROM CDP FRQ BRKPNT FILE FOR STABLE PITCHES (WHICH HAS NOTE STARTS AND ENDS)
 *
 *	Assumes a single voice, using a sigle MIDI channel (0)
 *	and note-off occurs before following note-on
 *
 *	If "staccato" is set to 0, program reads note on and note off times.
 *	If "staccato" > 0, note-offs are generated from note ons, and note-ends in input file are ignored.
 *
 *	CALL WITH
 *	convert_to_midi(seqfile,envfile,infiledatalen,staccato,outfilename,system);
 *
 *  where infiledatalen is no of midi notes in input = 1/4 of entries in frq-brkpnt file. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <sfsys.h>   // RWD for macro defs

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#else
# ifndef round
//static int round(double d);
static int twround(double d);   //RWD
#define round(x) twround((x))   //RWD
# endif
#endif


#define MIDI_NOTE_ON	(unsigned char)144	/*	0x90 = 1001 0000 = 1001 is Note-on : 0000 is channel number */
#define MIDI_NOTE_OFF	(unsigned char)128	/*	0x80 = 1000 0000 = 1001 is Note-on : 0000 is channel number */
#define LOW_A			6.875				/*	Frequency of A below MIDI 0 */
#define CONVERT_LOG10_TO_LOG2	3.321928	/* MiditoHz conversions */
#define TRUE	1
#define FALSE	0
#define NEWLINE		('\n')
#define ENDOFSTR	('\0')

static void Write_ThisIsAMIDI_MThdChunk_with_mS_timings (char format_type,char track_cnt,FILE *fp);
static void WriteTrackID (unsigned int midi_track_data_bytes,FILE *fp);
static void WriteVarLen (unsigned int value,FILE *fp);
static void WriteNormalLong (unsigned int value,FILE *fp);
static void Write_TrackEnd (FILE *fp);
static void create_note(double timestep,char midi,double thislevel,int on,FILE *fp);
static char HzToMidi (double frq);
static int  get_float_from_within_string(char **str,double *val);
static int midi_data_bytes_in_midi_track (int infiledatalen,double *seqdata,double staccato);
static int  CalcVarLen (unsigned int value);
static void Write_ThisIsAMIDI_PC_header(unsigned int total_bytes_in_file,FILE *fp);

#ifndef round
static int round(double d);
#endif

int main(int argc,char *argv[])
{
	FILE *fp;
	double *seqdata, *envdata, *p, *end;
	char *q;
	double lasttime, thistime, nexttime, timestep, thislevel, stacdur;
	int on, padbyte;
	int n,m,k;
	unsigned int midi_track_data_bytes, total_bytes_in_file;
	char temp[200];
	int system;
	char seqfile[64];
	char envfile[64];
	int infiledatalen;
	double staccato;
	char outfilename[64];
	char thismidi;

	if(argc == 1) {
		fprintf(stdout,"Converts frqequency-brkpnt and peakdata textfiles to a standard midi data file.\n");
		fprintf(stdout,"\n");
		fprintf(stdout,"USAGE:\n");
		fprintf(stdout,"convert_to_midi  frqbrkpnt  peakvals  datalen  staccato  outfilename  system\n");
		fprintf(stdout,"\n");
		fprintf(stdout,"FRQBRKPNT   File of time-frq pairs for note starts and ends.\n");
		fprintf(stdout,"PEAKVALS    Levels of each note (Range 0-1).\n");
		fprintf(stdout,"DATALEN     Number of peaks (no. of vals in 'frqbrkpnt' should be datalen * 4)\n");
		fprintf(stdout,"STACCATO    Force staccato output.\n");
		fprintf(stdout,"OUTFILENAME (File extension will be forced to '.mid' or '.rmi')\n");
		fprintf(stdout,"SYSTEM      0 gives Standard Midi File, with extension '.mid'\n");
		fprintf(stdout,"            1 gives PC Midi File, with extension '.rmi'\n");
		fflush(stdout);
		return 1;
	}
	if(argc != 7) {
		fprintf(stdout,"ERROR: WRONG NUMBER OF ARGUMENTS\n");
		fflush(stdout);
		return 0;
	}
	strcpy(seqfile,argv[1]);
	strcpy(envfile,argv[2]);
	if(sscanf(argv[3],"%d",&infiledatalen)<1) {
		fprintf(stdout,"ERROR: CANNOT READ INFILE DATA LENGTH [%s].\n",argv[3]);
		fflush(stdout);
		return 0;
	}	
	if(sscanf(argv[4],"%lf",&staccato)<1) {
		fprintf(stdout,"ERROR: CANNOT READ STACCATO DURATION [%s].\n",argv[4]);
		fflush(stdout);
		return 0;
	}	
	strcpy(outfilename,argv[5]);
	q = outfilename + strlen(outfilename);
	q--;
	while(*q != '.') {
		q--;
		if(q <= outfilename)
			break;
	}
	if(q > outfilename)
		*q = ENDOFSTR;
	if(sscanf(argv[6],"%d",&system)<1) {
		fprintf(stdout,"ERROR: CANNOT READ HEADER TYPE [%s].\n",argv[4]);
		fflush(stdout);
		return 0;
	}	
	if(system == 1)
		strcat(outfilename,".rmi");
	else
		strcat(outfilename,".mid");

	if((seqdata = (double *)malloc((infiledatalen * 4) * sizeof(double)))==NULL) {
		fprintf(stdout,"ERROR: INSUFFICIENT MEMORY TO CREATE INPUT DATA ARRAY.\n");
		fflush(stdout);
		return 0;
	}
	if((envdata = (double *)malloc(infiledatalen * sizeof(double)))==NULL) {
		fprintf(stdout,"ERROR: INSUFFICIENT MEMORY TO CREATE LOUDNESS DATA ARRAY.\n");
		fflush(stdout);
		return 0;
	}
	p = seqdata;
	if((fp = fopen(seqfile,"r"))==NULL) {			
		fprintf(stdout,"ERROR: FAILED TO OPEN FILE %s TO READ SEQUENCE DATA\n",seqfile);
		fflush(stdout);
		return 0;
	}
	end = p + (infiledatalen * 4);
	while(fgets(temp,200,fp)!=NULL) {	 /* READ AND TEST MIDI DATA INFO, ASSUMING IT TO BE IN CORRECT FORMAT!! */
		q = temp;
		while(get_float_from_within_string(&q,p)) {
			p++;			
			if (p >= end)
				break;
		}	    
	}
	fclose(fp);
	if(p - seqdata != infiledatalen * 4) {
		fprintf(stdout,"ERROR: COUNT OF MIDI DATA ITEMS DOES NOT CORRESPOND TO PARAM FOR INPUT DATA LENGTH\n");
		fflush(stdout);
		return 0;
	}	
	p = envdata;
	end = p + infiledatalen;
	if((fp = fopen(envfile,"r"))==NULL) {			
		fprintf(stdout,"ERROR: FAILED TO OPEN FILE %s TO READ LOUDNESS DATA\n",envfile);
		fflush(stdout);
		return 0;
	}
	while(fgets(temp,200,fp)!=NULL) {	 /* READ AND TEST MIDI DATA INFO, ASSUMING IT TO BE IN CORRECT FORMAT!! */
		q = temp;
		while(get_float_from_within_string(&q,p)) {
			p++;			
			if (p >= end)
				break;
		}	    
	}
	fclose(fp);
	if(p - envdata != infiledatalen) {
		fprintf(stdout,"ERROR: COUNT OF LOUDNESS ITEMS DOES NOT CORRESPOND TO PARAM FOR INPUT DATA LENGTH\n");
		fflush(stdout);
		return 0;
	}	

	if((fp = fopen(outfilename,"w"))==NULL) {			
		fprintf(stdout,"ERROR: FAILED TO OPEN MIDI DATAFILE %s TO WRITE DATA\n",outfilename);
		fflush(stdout);
		return 0;
	}
	total_bytes_in_file = 14 /* MThd Chunk */ + 4 /*MTrk track identifier */ ;
	midi_track_data_bytes = midi_data_bytes_in_midi_track(infiledatalen,seqdata,staccato);
	total_bytes_in_file += CalcVarLen(midi_track_data_bytes) + midi_track_data_bytes;
	
	if((total_bytes_in_file/2)*2 != total_bytes_in_file) {	/* Does (single) chunk have EVEN number of bytes? */
		padbyte = 1;
	} else {
		padbyte = 0;
	}
	if (system == 1)
		total_bytes_in_file += padbyte;	/* For PC, force an EVEN chunk-size */
	switch(system) {
	case(0):
		break;
	case(1): /* PC */
		Write_ThisIsAMIDI_PC_header(total_bytes_in_file,fp);
		break;
	}
	Write_ThisIsAMIDI_MThdChunk_with_mS_timings ((char)0,(char)1,fp);
		/* Assuming format type 0, 1 track only */
	WriteTrackID (midi_track_data_bytes,fp);
		/* Assuming there is 1 track, numbered 0 */
	lasttime = 0.0;
	on = 1;
	k = 0;
	for(n=0,m=1;n < infiledatalen * 4;n+=2,m+=2) {
		thistime  = seqdata[n];
		thismidi  = (char)round(HzToMidi(seqdata[m]));
		timestep  = thistime - lasttime;
		if (on) {
			thislevel = envdata[k++];
			create_note(timestep,thismidi,thislevel,1,fp);
			lasttime = thistime;
			if (staccato > 0.0) {		/* If staccato, note off determined by staccato dur */
				if(m+4 < infiledatalen) {
					nexttime = seqdata[m+4];
					stacdur = (nexttime - thistime)/2.0;
					stacdur = min(stacdur,staccato);
				} else {
					stacdur = staccato;
				}
				create_note(stacdur,thismidi,thislevel,0,fp);
				lasttime = thistime + stacdur;
			}
			on = 0;
		} else {
			if (staccato <= 0.0) {		/* IF not staccato, note-off taken from input data */
				create_note(timestep,thismidi,thislevel,0,fp);
				lasttime = thistime;	/*	But, if staccato, ignore the note off input */
			}
			on = 1;
		}
	}
	Write_TrackEnd(fp);
	if(system == 1)	{ /* PC riff format */
		if(padbyte)
			putc(0,fp);
	}
	fclose(fp);
	return 1;
}

/*********************** WRITE A 32-BIT REPRESENTATION OF "THIS is MIDI data = ascii MThd" ***********/

void Write_ThisIsAMIDI_MThdChunk_with_mS_timings (char format_type,char track_cnt,FILE *fp)
{
	register char buffer;
	buffer = 77;	/* M */
	putc(buffer,fp);
	buffer = 84;	/* T */
	putc(buffer,fp);
	buffer = 104;	/* h */
	putc(buffer,fp);
	buffer = 100;	/* d */
	putc(buffer,fp);
	buffer = 0;		/* sizeof MThd chunk is 6 = 0006 */
	putc(buffer,fp);
	putc(buffer,fp);
	putc(buffer,fp);
	buffer = 6;		/* sizeof MThd chunk is 6 = 0006 */
	putc(buffer,fp);
	buffer = 0;		/* format_type 00 0x */
	putc(buffer,fp);
	buffer = format_type;
	putc(buffer,fp);
	buffer = 0;		/* track_cnt   00 0x */
	putc(buffer,fp);
	buffer = track_cnt;
	putc(buffer,fp);
	buffer = (char)-25;	/* -25 means 25 frames per sec */
	putc(buffer,fp);
	buffer = 40;	/* 40 units per frame -> 1 unit per ms */
	putc(buffer,fp);
}

/*********************** WRITE TRACK ID  ***********/

void WriteTrackID (unsigned int midi_track_data_bytes,FILE *fp)
{
	register char buffer;
	buffer = 77;	/* M */
	putc(buffer,fp);
	buffer = 84;	/* T */
	putc(buffer,fp);
	buffer = 114;	/* r */
	putc(buffer,fp);
	buffer = 107;	/* k */
	putc(buffer,fp);
	WriteVarLen(midi_track_data_bytes,fp);
}

/*********************** WRITE A 32-BIT int, SPLIT INTO 7-bit WORDS IN REVERSE BYTE ORDER = MIDI TIME REPRESENTATION ***********/

void WriteVarLen (unsigned int value,FILE *fp)
{
	register unsigned int buffer;
	buffer = value & 0x7F;
	while (value >>= 7) {
		buffer <<= 8;
		buffer |= ((value & 0x7F) | 0x80);
	}
	for (;;) {
		putc(buffer,fp);
		if(buffer & 0x80)
			buffer >>= 8;
		else
			break;
	}
}

/*********************** WRITE A 32-BIT int IN NORMAL ORDER ***********/

void WriteNormalLong (unsigned int value,FILE *fp)
{
	unsigned int buffer;
	int n = 0;
	while (n < 4) {
		buffer = value & 0xFF;
		putc(buffer,fp);
		value >>= 8;
		n++;
	}
}

/*********************** ASCERTAIN BYTE LENGTH OF MIDI TIME REPRESENTATION ***********/

int CalcVarLen (unsigned int value)
{
	register unsigned int buffer;
	int cnt = 0;
	buffer = value & 0x7F;
	while (value >>= 7) {
		buffer <<= 8;
		buffer |= ((value & 0x7F) | 0x80);
	}
	for (;;) {
		cnt++;
		if(buffer & 0x80)
			buffer >>= 8;
		else
			break;
	}
	return cnt;
}

/*********************** WRITE "END OF TRACK" ***********/

void Write_TrackEnd (FILE *fp)
{
	register char buffer;
	buffer = (char)-1;				/* FF = |1111|1111| = -128 */
	putc(buffer,fp);
	buffer = (char)0x2F;
	putc(buffer,fp);
	buffer = 0;
	putc(buffer,fp);
}

/*********************** CREATE MIDI NOTE ON OR NOTE OFF DATA ***********/

void create_note(double timestep,char midi,double thislevel,int on,FILE *fp)
{
	register unsigned char buffer;
	unsigned int timedata;
	char velocity;
	timedata = (unsigned int)round(timestep * 1000.0);	/* TIME IN mS */
	velocity = (char)round(thislevel * 127.0);
	WriteVarLen(timedata,fp);
	if (on)
		buffer = MIDI_NOTE_ON;
	else
		buffer = MIDI_NOTE_OFF;
	putc(buffer,fp);
	buffer = midi;
	putc(buffer,fp);
	buffer = velocity;
	putc(buffer,fp);
}

/*********************** CONVERT FRQ TO MIDI ***********/

char HzToMidi (double frq) {
	double dmidi;
	char midi;
	if (frq < LOW_A) {
		frq = LOW_A;
	}
   	dmidi = frq / LOW_A;
	dmidi = (log10(dmidi) * CONVERT_LOG10_TO_LOG2 * 12.0) - 3.0;
	midi  = (char)round(dmidi);
	if (midi > 127) {
		midi = 127;
	} else if(midi < 0) {
		midi = 0;
	}
	return midi;
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

/*********************** CALCULATE NUMBER OF DATA BYTES TO FOLLOW ***********/

int midi_data_bytes_in_midi_track(int infiledatalen,double *seqdata,double staccato)
{
	double thistime, nexttime, timestep, stacdur; 
	double lasttime = 0.0;
	unsigned int timedata;
	unsigned int cnt = 0;
	int on = 1;
	int n,m;
	for(n=0,m=1;n < infiledatalen * 4;n+=2,m+=2) {
		thistime  = seqdata[n];
		timestep  = thistime - lasttime;
		lasttime = thistime;
		if (on) {
			timedata = (unsigned int)round(timestep * 1000.0);	/* TIME IN mS */
			cnt += CalcVarLen(timedata);
			cnt += 3;					/* MIDI status + key + velocity */
			if (staccato > 0.0) {
				if(m+4 < infiledatalen) {
					nexttime = seqdata[m+4];
					stacdur = (nexttime - thistime)/2.0;
					stacdur = min(stacdur,staccato);
				} else {
					stacdur = staccato;
				}
				timedata = (unsigned int)round(stacdur * 1000.0);
				cnt += CalcVarLen(timedata);
				cnt += 3;
				lasttime = thistime + stacdur;
			}
			on = 0;
		} else {
			if (staccato <= 0.0) {
				timedata = (unsigned int)round(timestep * 1000.0);
				cnt += CalcVarLen(timedata);
				cnt += 3;
				lasttime = thistime;
			}
			on = 1;
		}
	}
	cnt += 3;		/* Add Track end bytes */
	return cnt;
}

void Write_ThisIsAMIDI_PC_header(unsigned int total_bytes_in_file,FILE *fp)
{
	register char buffer;
	buffer = 82;	/* R */
	putc(buffer,fp);
	buffer = 73;	/* I */
	putc(buffer,fp);
	buffer = 70;	/* F */
	putc(buffer,fp);
	buffer = 70;	/* F */
	putc(buffer,fp);
	WriteNormalLong(total_bytes_in_file,fp);
	buffer = 82;	/* R */
	putc(buffer,fp);
	buffer = 77;	/* M */
	putc(buffer,fp);
	buffer = 73;	/* I */
	putc(buffer,fp);
	buffer = 68;	/* D */
	putc(buffer,fp);
}

//#ifndef round

static int twround(double d)
{
	d += 0.5;
	return (int)floor(d);
}

//endif
