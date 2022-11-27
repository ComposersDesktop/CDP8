/*
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

/**************

                DSHIFT.C

                Version 2.1

    REVISED 29/12/05

    Calculates Doppler shift for a source moving with a velocity 
    Vs between two speakers that are 10m appart and a listener 
    that is at rest, 5m from the line between them.

    It uses data similar to pan, when -1 means left and
    1 means right.

    If the frequency of the source is f and the frequency 
    perceived by the listener is f'

        then f'/f = ( 1 + Vs*cosA/V )

    where

        V = 331.45  speed of sound in air

        Vs =    -(distance between speakers / 2) *
            (next pan position - current pan position) / 
            (next point in time - current point in time)

        cosA = current pan position/ (1 + (current pan pos)^2 ) ^ 1/2

            is the angle between the speed of the source 
            and the line joining the listener and the source.

    Finally, it converts f'/f into semitone shift by means of

        semitone shift = 12 * log( f'/f ) / log2

    The program can be called by name with no parameters, in which 
    case it will start asking questions.

    Alternatively it can be called using the command

        dshift [-d] infile outfile [distance_between_speakers]

    where -d is a flag setting the time it takes an object to change direction
        This is useful in order to avoid clicks. N is the time in milliseconds. 
        Default: 10ms

        default distance between speakers = 10 m


    Copyright (c) Rajmil Fischman. Keele 1994.
                            *******************/
#include <stdio.h>
#include <stdlib.h>       
#include <math.h>
#include <string.h>

#define VSOUND  341.45  /*  speed of sound in air   */
#define LOG2    log(2.0)

//RWD cannot allocate 2^32 bytes!
#define MAXMEMORY   /*INT_MAX*/ (1024*1024) /* maximum memory to allocate */
#define MINMEMORY   512     /* minimum memory needed */
                     
                     
void usage(void);
void calculate(void);
void output(long);
void* getbpbuf(void);
long input(char*,char*);

typedef struct brpts    {
    double time;
    double param;
} BREAKPOINTS;

BREAKPOINTS *bp;
FILE *ifp;          /*  input file pointer  */
FILE *ofp;          /*  output file pointer */
long total;         /*  total entries   */
long maxentr;       /*  maximum entries */
double spdist = 10.0;   /*  distance between speakers   */
double turn_time = 0.01;    /*  turning time delay  */
double vs;      /*  source velocity */
double ccosa;       /*  current cosA    */
double ncosa;       /*  next cosA   */
double cfrat;       /*  current f'/f    */
double nfrat;       /*  next f'/f   */
double cstshift;    /*  currentsemitone shift   */
double nstshift;    /*  next semitone shift */


const char* cdp_version = "7.1.1";


int
main(int argc,char* argv[])

{
    char inname[2048];
    char outname[2048];
    
    if((argc==2) && strcmp(argv[1],"--version")==0) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    /* if not enough parameters */
    if((argc < 3) || (argc == 3 && argv[1][2]=='-' && argv[1][1]=='d'))
    {
        usage();
        return 1;
    }
    
    /* if there is a flag */
    if(argv[1][0]=='-' && argv[1][1] == 'd')
    { 
        /* get turning time */
        turn_time = atof(&argv[1][2]);
        // REW make msecs to secs
        if(turn_time < 0.0){
            printf("Error: turn-time cannot be negative!\n");
            return 1;
        }
        turn_time *=  0.001;
        

        /* get soundfile names */
        strcpy(&inname[0],argv[2]);
        strcpy(&outname[0],argv[3]);
  
        /* get speaker distance if needed */
        if(argc > 4)
            spdist = atof(argv[4]);
    }       
    else
    {
        /* get soundfile names only */
        strcpy(&inname[0],argv[1]);
        strcpy(&outname[0],argv[2]);
        
        /* get speaker distance if needed */
        if(argc > 3)
            spdist = atof(argv[3]);
    }

    /* print info */
    fprintf(stderr,"turning time = %7.3lf sec.\t",turn_time);
    fprintf(stderr,"speakers distance = %7.2lf m\n",spdist);

    /* allocate memory, get input, calculate Doppler and write to output */
    if(getbpbuf()!=NULL && input(inname,outname)>0)
    {
        calculate();
        free(bp);
    }
    //RWD close outfile!
    if(ofp)
        fclose(ofp);
    return 0;
}


void
usage()
{
    fprintf(stderr,"CDP Release 5 2005\n");
    fprintf(stderr,"USAGE:\tdshift [-dN] infile outfile [distance between speakers]\n\n");
    fprintf(stderr,"Calculates Doppler shift for a source moving between two speakers\n" 
    "that are 10m apart and a listener that is at rest,\n"
    "5m from the line between them.\n");
    fprintf(stderr,"infile is the pan breakpoint file, as used with modify:pan\n"
        "outfile is breakpoint file of semitone shifts\n"
        "to give to modify:speed, Mode 2(semitones).\n");
        fprintf(stderr,"(NB: modify:pan range: -1 = full left, 1 = full right)\n");
    fprintf(stderr,"-d is a flag setting the time it takes a sound to change direction.\n");
    fprintf(stderr,"N is the time in milliseconds. Default: 10ms\n");
    fprintf(stderr,"This is useful in order to avoid clicks.\n");
}

void*
getbpbuf()
{
    size_t size = MAXMEMORY;
    void *p;
    
    do
    {
        if((p=malloc(size)) == NULL )
        {
            if(size<MINMEMORY)
                fprintf(stderr,"Cannot allocate memory for breakpoints");
        }
        else
            break;
            
        size >>=1;
    } while (size>MINMEMORY);

    bp = (BREAKPOINTS*)p;
    maxentr = size/sizeof(BREAKPOINTS);

    return((void*)bp);
}


void
calculate(void)
{
    long i;
    double deltaTime;

    /* calculate cosine of next angle */
    ncosa = bp[0].param/sqrt(1+bp[0].param*bp[0].param);
        output(-1);
    for( i=0 ; i<total-1 ; i++ )
    { 
        /* avoid division by zero */
        deltaTime = (bp[i+1].time-bp[i].time);
        if(deltaTime <= 0)
            deltaTime = 0.0000001;

        /* calculate speed */
        vs =  - (spdist/2) * (bp[i+1].param-bp[i].param) / deltaTime;

        /* Cap maximum speed to avoid values higher than the speed of sound */
        if( vs >= VSOUND )
            vs = 0.999 * VSOUND;
        else if( vs <= -VSOUND )
            vs = -0.999 * VSOUND;
            
        /* calculate cosine of current angle */
        ccosa = ncosa;
        
        /* calculate cosine of next angle */
        ncosa = bp[i+1].param/sqrt(1+bp[i+1].param*bp[i+1].param);
    
        /* calculate current and next frequency ratio */
        cfrat = 1 + vs*ccosa/VSOUND;
        nfrat = 1 + vs*ncosa/VSOUND;
        
        /* convert these into semitones */
        cstshift = 12 * log(cfrat)/LOG2;
        nstshift = 12 * log(nfrat)/LOG2;
        
        /* write to file and to standard output */
        output(i);
    }

    fprintf(stderr,"\nread %ld breakpoints\n",i+1);


}

/* 
    INPUT()
    Open and read input file. Open output file
*/
long
input(char *inname,char *outname)
{
    //static char first = 1;

    if((ifp=fopen(inname,"ra")) == NULL )
    {
        fprintf(stderr,"Cannot open input file %s\n",inname);
        return(0);
    }
    for( total=0 ; total< maxentr ; total++ )
    {
        fscanf(ifp,"%lf%lf",&(bp[total].time),&(bp[total].param));
        if(bp[total].time < 0 || feof(ifp) != 0 )
            break;
    }
    fclose(ifp);

    if((ofp=fopen(outname,"wa")) == NULL )
        fprintf(stderr,"Cannot open output file %s\n",outname);

    return(1);
}

/*
    OUTPUT()
    Write to output file and standard output
*/
void
output(long i)
{
    static int add = 0; /* variable used to avoid adding the turining time 
                        to the first pair of points */
    
    if( i<0 )
    {
//      fprintf(stderr,"\n\n TIME\t PAN\t VS\t COS(A)\t F'/F\tSEMITONES\n\n");
        return;
    }
/*  else if (i<total-1)
    {
        fprintf(stderr,"%- 7.3lf\t%- 7.3lf\t%- 7.3lf\t%- 7.3lf\t%- 7.6lf\t%- 7.6lf\t\n",
            bp[i].time+add*turn_time,bp[i].param,vs,ccosa,cfrat,cstshift);
        fprintf(stderr,"%- 7.3lf\t%- 7.3lf\t%- 7.3lf\t%- 7.3lf\t%- 7.6lf\t%- 7.6lf\t\n",
            bp[i+1].time,bp[i+1].param,vs,ncosa,nfrat,nstshift);    }
    else
        fprintf(stderr,"%- 7.3lf\t%- 7.3lf\t%- 7.3lf\t%- 7.3lf\t%- 7.6lf\t%- 7.6lf\t\n",
            bp[i].time+add*turn_time,bp[i].param,vs,ccosa,cfrat,cstshift);

*/
    if(ofp!=NULL)
    {
        fprintf(ofp,"%lf\t%lf\n",bp[i].time+add*turn_time,cstshift);
        fprintf(ofp,"%lf\t%lf\n",bp[i+1].time,nstshift);
    }
    add = 1;
}
