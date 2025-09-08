/*
 * Copyright (c) 1983-2023 Trevor Wishart and Composers Desktop Project Ltd
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



/**************************************************************************

                MAXSAMP2.C

    usage   maxsamp2 infile

    Finds the maximum sample of a file and writes it to the header.

    Based on maxsamp in CDPARSE

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osbind.h>
#include <math.h>
#include <float.h>
#include <float.h>
#include <sfsys.h>
#include <cdplib.h>

/*static void   usage(void);*/
static void report(void);
static int  tidy_up(int);
static void min_sec(int,int,int*,double*);
static void find_fmax(float*,int,int);
static int  try_header(int);
static int  open_in(char*,int);
static int  get_big_buf(void);
static int  get_max_samp(int,int);
static int  smpflteq(double f1,double f2);
static void force_new_header(void);

float *bigfbuf; /* buffer used to read samples from soundfile   */

size_t buflen;  /* buffer length in samps (eventually) */
int ifd;    /* input soundfile descriptor           */
int srate = 44100;  /* sampling rate of input       */
int channels = 2;   /* number of channels of input      */

double maxpdamp = DBL_MIN;  /* value of maximum negative sample */
double maxndamp = DBL_MAX;  /* value of maximum positive sample */
float maxpfamp;     /* float value of maximum positive sample   */
float maxnfamp;     /* float value of maximum negative sample   */
double maxdamp  = 0.0;  /* value of maximum sample          */

int maxloc = 0; /* location of maximum sample       */
int repeats = 1;    /* counts how many times the maximum repeats */
int pos_repeats = 0;
int neg_repeats = 0;
int in_header = 0;

unsigned int maxnloc, maxploc;
unsigned int *maxcnloc, *maxcploc;
double *maxcpdamp, *maxcndamp;
int *posreps, *negreps;
const char* cdp_version = "7.1.0";

int open_in(char *name,int force_read)      /* opens input soundfile and gets header */
{
    int is_sound = 0;
    SFPROPS props = {0};
    int open_type;
    if(force_read)
        open_type = CDP_OPEN_RDWR;
    else 
        open_type = CDP_OPEN_RDONLY;
    if( (ifd = sndopenEx(name,0,open_type)) < 0 )   {
        fprintf(stdout,"INFO: Cannot open file: %s\n\t",name);
        fflush(stdout);
        return(-1);
    }
    if(!snd_headread(ifd,&props)) {
        fprintf(stdout,"Failure to read sample size\n");
        fflush(stdout);
        return(-1);
    }
    if(props.type != 0) {
        fprintf(stdout,"Not a soundfile\n");
        fflush(stdout);
        return(-1);
    }
    srate = props.srate;
    channels = props.chans;

    return(is_sound);
}

int get_big_buf(void)   /* allocates memory for the biggest possible buffer */
{
    size_t i;

    buflen = (size_t) Malloc(-1) - sizeof(float);

    /* if less than one sector available */
    if( buflen < SECSIZE || ((bigfbuf=(float*)Malloc(buflen+sizeof(float))) == NULL)) {
        fprintf(stdout,"ERROR: Failed to allocate float buffer.\n");
        fflush(stdout);
        return 0;
    }
    i = ((size_t)bigfbuf+sizeof(float)-1)/sizeof(float)*sizeof(float);  /* align bigbuf to word boundary */
    bigfbuf = (float*)i;

    buflen /= sizeof(float);
    buflen = (buflen/channels) * channels;      /* align buflen to channel boundaries */
    if(buflen <= 0) {
        fprintf(stdout,"ERROR: Failed to allocate float buffer.\n");
        fflush(stdout);
        return 0;
    }
    return 1;
}

int get_max_samp(int ifd,int force_read)
{
    int got;
    int totalsamps = 0;
    int mins;
    double sec;
//    int total_got = 0;
    int j;

    switch(force_read) {
    case(0):
        if(try_header(ifd) >= 0)    //  No forcing: "try_header" succeeds. Maxsamp can be >0 or 0
            return(0);
        break;
    case(2):
        if(try_header(ifd) > 0)     //  "try_header" succeeds. Maxsamp can be >0 but NOT 0
            return(0);              //  if maxsamp is 0, searching for maxsamp is forced
        force_read = 1;
        break;
        /* else if info is not in header */
    }
    /* read and find maximum */
    while( (got = fgetfbufEx(bigfbuf,(int) buflen,ifd,0)) > 0 ) {
//        total_got += got;
        find_fmax(bigfbuf,got,totalsamps);
        totalsamps += got;
    }
    if( got < 0 ) {
        min_sec((int)(totalsamps/channels),srate,&mins,&sec);
        fprintf(stdout,"ERROR: An error has occured. The current Location is:\t %d min %6.3lf sec\n",mins,sec);
        fprintf(stdout,"ERROR: The maximum sample found so far is: %lf\n",maxdamp);
        fflush(stdout);
        return(-1);
    }
    maxpdamp = maxcpdamp[0];
    maxndamp = maxcndamp[0];
    maxploc  = maxcploc[0];
    maxnloc  = maxcnloc[0];
    pos_repeats = posreps[0];
    neg_repeats = negreps[0];
    for(j=1;j < channels; j++) {
        if(maxcpdamp[j] > maxpdamp) {
            maxpdamp = maxcpdamp[j];
            maxploc  = maxcploc[j];
            pos_repeats = posreps[j];
        } else if (maxcpdamp[j] < maxpdamp)
            ;
        else if(maxcploc[j] < maxploc)  // equal +ve vals
            maxploc = maxcploc[j];
        if(maxcndamp[j] < maxndamp) {
            maxndamp = maxcndamp[j];
            maxnloc  = maxcnloc[j];
            neg_repeats = negreps[j];
        } else if (maxcndamp[j] > maxndamp)
            ;
        else if(maxcnloc[j] < maxnloc)  // equal -ve vals
            maxnloc = maxcnloc[j];
    }
    maxpfamp = (float)maxpdamp;
    maxnfamp = (float)maxndamp;
    if(maxpdamp > -maxndamp)
        maxdamp = maxpdamp;
    else
        maxdamp = fabs(maxndamp);
    return(0);
}

void
report(void)
{
    int mins, maxchan = 0;
    double sec;
    if (!in_header && !pos_repeats && !neg_repeats) {
        fprintf(stdout,"INFO: All samples are zero.\n");
        fflush(stdout);
        return;
    }
    if(in_header) {
        fprintf(stdout,"KEEP: %lf %d %d\n",maxdamp,maxloc,-1);
    } else {
        if(maxpdamp > -maxndamp) {
            maxloc = maxploc;
            repeats = pos_repeats;
        } else if(maxpdamp < -maxndamp) {
            maxloc = maxnloc;
            repeats = neg_repeats;
        } else {
            maxloc  = min(maxploc,maxnloc);
            repeats = neg_repeats + pos_repeats;
        }
        maxchan = (maxloc % channels)+1;
        maxloc /= channels;
        fprintf(stdout,"KEEP: %lf %d %d\n",maxdamp,maxloc,repeats);
        if(pos_repeats)
            fprintf(stdout,"INFO: Maximum positive sample:      %f\n",maxpfamp);
        if(neg_repeats)
            fprintf(stdout,"INFO: Maximum negative sample:      %f\n",maxnfamp);
    }
    fprintf(stdout,"INFO: Maximum ABSOLUTE sample:      %lf\n",maxdamp);
    min_sec(maxloc,srate,&mins,&sec);
    if(maxchan > 0)
        fprintf(stdout,"INFO: Location of maximum sample:   %d min %7.4lf sec: chan %d\n",mins,sec,maxchan);
    else
        fprintf(stdout,"INFO: Location of maximum sample:   %d min %7.4lf sec\n",mins,sec);
    if(!in_header) {
        fprintf(stdout,"INFO: Number of times found:        %-d\n",repeats);
    }
    fprintf(stdout,"INFO: Maximum possible dB gain:     %-7.3lf\n",20.0*log10(1.0/maxdamp));
    fprintf(stdout,"INFO: Maximum possible gain factor: %-9.3lf\n",1.0/maxdamp);
    fflush(stdout);
}
    
int tidy_up(int where)
{
    switch(where)
    {
    case 0:
        Mfree(bigfbuf);
    case 1:
        sndcloseEx(ifd);
    case 2:
//       sffinish();
    default:
        break;
    }
    return(1);
}
    
int main(int argc,char *argv[])
{
    int force_read = 0, j;
    /*TICK */  // unsigned int time;
    int is_sound;
    /* get current time */
//    time = hz200();

    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if(argc == 3) {
        if(sscanf(argv[2],"%d",&force_read) != 1) {
            fprintf(stdout,"ERROR: bad 2nd parameter.\n");
            fflush(stdout);
            return tidy_up(3);
        }
        if(force_read < 1 || force_read > 2) {
            fprintf(stdout,"ERROR: bad 2nd parameter.\n");
            fflush(stdout);
            return tidy_up(3);
        }
        argc--;
    }
    if(argc !=2) {
        fprintf(stdout,"ERROR: wrong number of arguments.\n");
        fflush(stdout);
        return tidy_up(3);
    }

    /* initialise SFSYS */
    if( sflinit("maxsamp2") < 0 )
    {
        fprintf(stdout,"ERROR: Cannot initialise soundfile system.\n");
        fflush(stdout);
        return tidy_up(3);
    }

    /* open input file */
    if((is_sound = open_in(argv[1],force_read)) < 0)
        return tidy_up(2);
    /* get biggest buffer */
    if(get_big_buf() == 0)
        return tidy_up(1);

    if((maxcploc = (unsigned int *)malloc(channels * sizeof(unsigned int))) == NULL)
        return tidy_up(0);
    if((maxcnloc = (unsigned int *)malloc(channels * sizeof(unsigned int))) == NULL)
        return tidy_up(0);
    if((maxcpdamp = (double *)malloc(channels * sizeof(double))) == NULL)
        return tidy_up(0);
    if((maxcndamp = (double *)malloc(channels * sizeof(double))) == NULL)
        return tidy_up(0);
    if((posreps = (int *)malloc(channels * sizeof(int))) == NULL)
        return tidy_up(0);
    if((negreps = (int *)malloc(channels * sizeof(int))) == NULL)
        return tidy_up(0);
    for(j=0;j<channels;j++) {
        maxcploc[j] = 0;
        maxcnloc[j] = 0;
        maxcpdamp[j] = DBL_MIN;
        maxcndamp[j] = DBL_MAX;
        posreps[j] = 0;
        negreps[j] = 0;

    }

    /* max soundfiles */
    get_max_samp(ifd,force_read);

    /* send report */
    if(force_read)
        force_new_header();
    report();

    /* tidy up */
    return tidy_up(0);
}

/* converts samples into minutes and seconds */

void min_sec(int value,int srate,int *mins,double *secs)
{
    *secs = (double)value/srate;
    
    *mins = (int)floor(*secs/60);
    *secs -= *mins*60;
}

/**************************** SMPFLTEQ *******************************/
#define SMP_FLTERR 0.0000005

int smpflteq(double f1,double f2)
{
    double upperbnd, lowerbnd;
    upperbnd = f2 + SMP_FLTERR;
    lowerbnd = f2 - SMP_FLTERR;
    if((f1>upperbnd) || (f1<lowerbnd))
        return(0);
    return(1);
}

/**************************** TRY_HEADER ****************************
 *
 * checks if maxsamp information is in header
 */

int try_header(int ifd)
{
    int j;
    CHPEAK *peakdata;
    int peaktime;
    maxloc = sndsizeEx(ifd);

    if((peakdata = (CHPEAK *)malloc(channels * sizeof(CHPEAK)))==NULL) {
        return -1;
    }
    if(sndreadpeaks(ifd,channels,peakdata,&peaktime) < 0)
        return -1;
    for(j=0;j<channels;j++) {
        if(peakdata[j].value > maxdamp) {
            maxdamp = peakdata[j].value;
            maxloc  = peakdata[j].position;
        } else if(!(peakdata[j].value < maxdamp)) { /* i.e. equal values */
            maxloc  = min(peakdata[j].position,(unsigned int)maxloc);
        }
    }
    if(maxdamp == 0.0) {
        maxloc = 0;
        return 0;
    }
    in_header = 1;
    return 1;
}       

void find_fmax(float *buffer,int samps,int totalsamps)
{
    int i, j;

    for(j=0;j < channels; j++) {
        maxpdamp = 0.0;
        maxndamp = 0.0;
        for( i=j ; i<samps ; i+=channels ) {
            if(maxcpdamp[j] > 0.0 && smpflteq(buffer[i],maxcpdamp[j]))
                posreps[j]++;
            else if(maxcndamp[j] < 0.0 && smpflteq(buffer[i],maxcndamp[j]))
                negreps[j]++;
            else if( buffer[i] > maxcpdamp[j]) {
                maxcpdamp[j] = buffer[i];
                posreps[j] = 1;
                maxcploc[j] = totalsamps + i;
            } else if ( buffer[i] < maxcndamp[j] ) {
                maxcndamp[j] = buffer[i];
                negreps[j] = 1;
                maxcnloc[j] = totalsamps + i;
            }
        }
    }
}

void force_new_header(void) {
    int j;
    unsigned int maxp;
    double maxv;
    CHPEAK *peakdata;
    if((peakdata = (CHPEAK *)malloc(channels * sizeof(CHPEAK)))==NULL) {
        return;
    }
    for(j=0;j<channels;j++) {
        if(-maxcpdamp[j] < maxcndamp[j]) {
            maxv = maxcpdamp[j];
            maxp = maxcploc[j];
        } else if(-maxcpdamp[j] > maxcndamp[j]) {
            maxv = maxcndamp[j];
            maxp = maxcnloc[j];
        } else {    // equal
            if(maxcnloc[j] < maxcploc[j]) {
                maxv = maxcndamp[j];
                maxp = maxcnloc[j];
            } else {
                maxv = maxcpdamp[j];
                maxp = maxcploc[j];
            }
        }
        peakdata[j].value    = (float)maxv;
        peakdata[j].position = maxp;
    }
    if(sndputpeaks(ifd,channels,peakdata)<0) {
        fprintf(stderr,"NO HEADER WRITE: %s\n",sferrstr());
    }
}
