/*
 * Copyright (c) 1983-2013 Richard Dobson and Composers Desktop Project Ltd
 * http://people.bath.ac.uk/masrwd
 * http://www.composersdesktop.com
 * This file is part of the CDP System.
 * The CDP System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *
 * The CDP System is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with the CDP System; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
 
/***********************************************************************
Motorola

Motorola does not assume any liability ...
Author:   Tom Zudock (tomz@dsp.sps.mot.com)

see:
 http://collaboration.cmc.ec.gc.ca/science/rpn/biblio/ddj/Website/articles/DDJ/1996/9612/9612d/9612d.htm
 
Presented in Dr Dobbs Journal Vol21:12, December 1996.
 
Filename: roomresp.cpp

This program calculates the room response for a simple room.  The output is not
particularly delightful to listen to (it could sound phasey and
have ringing due to the simple model used), but this program is the
foundation for more complex virtual audio models which will eliminate
these anomalies.

Example form of the InputParametersTextFile:

0.5               # Input gain for data file (float)
1                 # Open Path = 0 Closed Path = 1 (int)
6    5    4       # Room Dimensions (X,Y,Z) (float)
0.85              # Reflectivity of walls (float)
5                 # Number of rooms per dimension, 1 or greater
2    3    2       # Listener coord (X,Y,Z) (float)
2    2    2    1  # Source coord and time at coord (X,Y,Z,t) (float)
3    2    2    1  # Source coord and time at coord (X,Y,Z,t) (float)
4    3    2    1  # Source coord and time at coord (X,Y,Z,t) (float)
3    4    2    1  # Source coord and time at coord (X,Y,Z,t) (float)
2    4    2    1  # Source coord and time at coord (X,Y,Z,t) (float)

***********************************************************************/

// included header files
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#include <process.h>
#include <string.h> 
#ifdef _DEBUG
#include <assert.h>
#endif
#include <vector>
#include <algorithm>
using namespace std;

#ifndef _MAX
#define _MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

// macro defined constants
//#define Vs 334.0
static const double Vs = 334.0;
static const double srate = 20000;
#define PI 3.14159265359
#define RefDist 1.0
enum args {
		REFLECTIVITY,
		REFLECTIONS,
		R_X,
		R_Y,
		R_Z,
		S_X,
		S_Y,
		S_Z,
		L_X,
		L_Y,
		L_Z
};

// structure for a point in 3 dimensional space
struct point {
	double X;
	double Y;
	double Z;
	point(){
		X = Y = Z = 0.0;
	}
	virtual ~point() {}
};

// structure for a point paired with a time duration
struct position {
	point Coord;
	double Time;
	position *NextSource;
};

const char* cdp_version = "5.0.0";

bool input_cmds(char *args[],int argc,point &room,point &source,point &listener,double &reflcoef,int &NR);

void
CalcImpResp(
    long *,
    double *,
    point &,
    point &,
    point &,
    double,
    double,
    int);

position *
InputParameters(
    FILE *,
    point &,
 //   point &,
    point &,
    double &,
    double &,
   int &);

void
InputParamsError(
    int);

//RWD stuff
typedef struct deltap {
		long time;
		double val;
	} DELTAP;

bool operator<(const DELTAP &a,const DELTAP &b)
{
	return (a.time < b.time);
}

void usage()					//RWD.1..12.98 left out flag -nNORM for now...
{
	printf("\n*********  ROOMRESP.EXE: by Tom Zudock  CDP build 1998,1999 ******"
		   "\n  GENERATE ROOM-RESPONSE EARLY REFLECTIONS DATAFILE\n"
		   "\nusage: roomresp [-aMAXAMP][-rRES] txtout.dat liveness nrefs "
		   "\n                          roomL roomW roomH"
		   "\n                             sourceL sourceW sourceH"
		   "\n                                listenerL listenerW listenerH");
	printf("\ntxtout.dat: text outfile containing early reflections in brkpoint format"
			"\n             suitable for input to rmverb, reverb or tdelay"
			"\nMAXAMP   : peak amp for data (0.0 < MAXAMP <= 1.0; default 1.0)"
			"\nRES      : time resolution for reflections in msecs"
			"\n             (0.1 <= RES <= 2; default 0.1)"
			"\nliveness : degree of reflection from each surface (0 to 1; typ: 0.95)"
			"\nnrefs    : number of reflections from each surface(nrefs > 0; typ: 2 to 5)"
			"\n           (WARNING: high values will create extremely long data files!)"
			"\nROOM PARAMETERS:"
			"\nroomL roomW roomH            : room size: (Length, Width, Height)"
			"\nsourceL sourceW ourceH       : position of sound source (as above)"
			"\nlistenerL listenerW listenerH: position of listener (as above)"
			"\n                               all dimensions are in metres"
			"\nNB: first output time is non-zero."
			);
}


/***********************************************************************
main

The main program is responsible for the primary program flow.
It contains the primary processing loop which calls subroutines
to handle the specialized functionality.
***********************************************************************/
int main (int argc, char **argv) {
	
//	FILE        *PositionFile = 0;  // input text parameters file
	//RWD to print earlies as text
	FILE		*earliesFile = 0;
#ifdef TESTING
	FILE		*rawfile = 0;				///just for testing...raw output from impresp funtionm
#endif
	int			i,NR = 0;							// num of mirrored rooms per dimension
	long		*TapDelay = 0;			// pointer to array of tap delays	
	double		Fs = 0.0;							// the sampling frequency
	double		*TapGain = 0;				// pointer to array of tap gains
	double		ReflCoef = 0.0;				// the reflectivity of the walls
//	position	*CurrentSource = 0;	// current source in positions list 
	point		Room;						// coords for the room size
	point		Source;					// coords of current sound source
	point		Listener;				// coords of the listener
#ifdef _DEBUG
	bool		write_c_struct = false;
#endif
	bool		do_normalize = true;
	double		normfac = 1.0;
	double      min_separation = 0.0001;

	if((argc==2) && strcmp(argv[1],"--version")==0) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(argc==1){
		usage();
		exit(0);
	}
	if (argc < 13){
		fprintf(stderr,"\ninsufficient arguments\n");
  		usage();
		exit(-1);
	}
  
	while(argc > 0 && argv[1][0]=='-'){
		double val;
		switch(argv[1][1]){
#ifdef _DEBUG
//my private facility...
		case('c'):
			write_c_struct = true;
			printf("\nwriting C-format file");
			break;
#endif
		case('a'):
			if(argv[1][2]=='\0'){
				fprintf(stderr,"\n-a flag requires a value\n");
				usage();
				exit(1);
			}
			val = atof(&(argv[1][2]));
#ifdef _DEBUG
			if(val <0.0 || val > 1.0){
#else
			if(val <=0.0 || val > 1.0){
#endif
				fprintf(stderr,"\n normalization value %lf out of range\n",val);
				usage();
				exit(1);
			}
#ifdef _DEBUG
			if(val==0.0) {
				do_normalize = false;
				normfac = 1.0;
				printf("\nskipping normalization");
			}
			else
#endif
				normfac = val;
			break;
		case('r'):
			if(argv[1][2]=='\0'){
				fprintf(stderr,"\n-r flag requires a value\n");
				usage();
				exit(1);
			}
			val = atof(&(argv[1][2]));			
			if(val < 0.1 || val > 2.0) {
				fprintf(stderr,"\nvalue %.4lf for RES out of range\n",val);
				usage();
				exit(1);				
			}
			else
				min_separation = val * 0.001;
			break;



		default:		
			fprintf(stderr,"\nbad flag option\n");
			exit(1);
			break;
		}
		argc--;
		argv++;
	}

	//step along arglist:
	argc--; argv++;

	
	if(argc != 12){
		fprintf(stderr,"\nincorrect number of numeric arguments\n");
		usage();
		exit(1);
	}	
	if((earliesFile = fopen(argv[0],"w"))==NULL){
		printf ("\nUnable to open %s for output\n",argv[2]);
		exit(-1);
	}
#ifdef TESTING
	if((rawfile = fopen("rawdata.txt","w"))==NULL){
		printf ("Unable to open rawdata.txt for output\n");
		exit(-1);
	}
#endif
	//step along again...
	argc--; argv++;

	//NB this sets the efective time-resolution for each tap, ie 1.0 / fs
	//so 1000 is a minimum, for 1msec resolution

	Fs = srate;

	if(!input_cmds(argv,argc,Room,Source,Listener,ReflCoef,NR))
		exit(1);	//already got the errmsg
  
	if((TapDelay = new long[NR*NR*NR])==NULL) {
    	printf("Error: Unable to allocate memory\n");
    	exit(-1);
    }
    if((TapGain = new double[NR*NR*NR])==NULL) {
    	printf("Error: Unable to allocate memory\n");
    	exit(-1);
    }
	
  
	CalcImpResp(
	    TapDelay,
		TapGain,
		Room,
		/*CurrentSource->Coord,*/Source,
		Listener,
		ReflCoef,
		Fs,
		NR);

	//sort deltaps, eliminate duplicates,  write to file
	vector<DELTAP> vtaps;
	DELTAP thistap,prevtap;
	prevtap.time = 0;
	prevtap.val = 0.0;
	for(i = 0; i < NR*NR*NR;i++) {
		thistap.time = TapDelay[i];
		thistap.val  =TapGain[i];
#ifdef TESTING
		fprintf(rawfile,"%ld\t%.6lf\n",thistap.time,thistap.val);
#endif
		//exclude tapval of 1.0: = direct sig at listening point
		if(thistap.val > 0.0 && thistap.val < 1.0)			// get a problem in release build otherwise....
			vtaps.push_back(thistap);	
	}

	//now we can sort them

	sort(vtaps.begin(),vtaps.end());
	
	vector<DELTAP>::iterator it;
	it = vtaps.begin();
	prevtap.time = it->time;
	prevtap.val =  it->val;

	//print all taps as we got them, first!

	double maxval = 0.0;
	for(;it != vtaps.end();it++){
#ifdef NOTDEF		
		fprintf(earliesFile,"\n\t%.4lf\t%.6lf",(double)it->time / Fs,it->val);
#endif
		maxval = _MAX(it->val,maxval);
		
	}
#ifdef _DEBUG
	printf("\noverall maxamp = %.4lf",maxval);
#endif

	//print first line, formatted for code
	//normalize all vals to max amplitude!
	
	if(do_normalize){
		normfac  /= prevtap.val;
#ifdef _DEBUG
		printf("\nnormalizing by %.4lf",normfac);
#endif
	}
	it = vtaps.begin();
	
	
	//normfac= floor(normfac);
#ifdef _DEBUG
	printf("\ninitial time=  %ld, initial amp = %.4lf",it->time,it->val);

	//RWD private facility!
	if(write_c_struct){
		fprintf(earliesFile,"\n\n{\n\t{%.4lf,%.6lf}",(double)prevtap.time / Fs,prevtap.val * normfac);
		int count = 1;
	
		for(;it != vtaps.end();it++){
			//get maxamp of any taps sharing a time (to 5 sig figs) - or should I add them somehow?
			if(fabs(it->time - prevtap.time) < min_separation){
				it->val = _MAX(it->val,prevtap.val);
				continue;
			}
			fprintf(earliesFile,",\n\t{%.5lf,%.6lf}",(double)it->time / Fs,it->val * normfac);
			prevtap = *it;
			count++;
		}
		fprintf(earliesFile,"\n\t};\n\nNUMTAPS = %d\n",count);
	}
	else {
#endif
		int count = 1;
		fprintf(earliesFile,"%.5lf\t%.6lf\n",(double)prevtap.time / Fs, prevtap.val * normfac);
		for(;it != vtaps.end();it++){
			if(fabs(it->time - prevtap.time) < min_separation){
				it->val = _MAX(it->val,prevtap.val);
				continue;
			}
			fprintf(earliesFile,"%.5lf\t%.6lf\n",(double)it->time / Fs, it->val * normfac);
			prevtap = *it;
			count++;
		}
		printf("\n\nNUMTAPS = %d\n",count);
#ifdef _DEBUG
	}
#endif
	fclose(earliesFile);
#ifdef TESTING
	fclose(rawfile);
#endif
	delete TapDelay,
	delete TapGain,
	printf ("done\n");
	return(0);
}               

/***********************************************************************
CalcImpResp

This subroutine calculates the time delays (in samples) and
attenuations for the early reflections in the room.
***********************************************************************/
void
CalcImpResp(
  long * TapDelay,
  double * TapGain,
  point & Room,
  point & Source,
  point & Listener,
  double ReflCoef,
  double Fs,
  int NR)						
{
	double Dist = 0.0;                       // distance travelled by sound ray
	double ReflOrd = 0.0;                    // reflection order of sound ray
	//RWD.11.98 need volatile, or VC++ optimizer does more bad things!
	volatile point MirrSource;                  // mirrored source x,y,z coords
	//RWD
	int NRovr2 = NR / 2;
	int index = 0;
  for (int i=-NRovr2;i<=NRovr2;i++) {    // loop through all 3 dimensions
    for (int j=-NRovr2;j<=NRovr2;j++) {
      for (int k=-NRovr2;k<=NRovr2;k++) {
        // calc x,y,z sound source coords in mirrored room		  
        MirrSource.X = (i)*Room.X
					   +fabs(((i)%2)*Room.X)
                       +pow(-1,(i))*Source.X;                       
        MirrSource.Y = (j)*Room.Y
                       +fabs(((j)%2)*Room.Y)
                       +pow(-1,(j))*Source.Y;
        MirrSource.Z = (k)*Room.Z
                       +fabs(((k)%2)*Room.Z)
                       +pow(-1,(k))*Source.Z;
        // calculate distance to listener
        Dist = sqrt(pow(MirrSource.X-Listener.X,2)
                    +pow(MirrSource.Y-Listener.Y,2)
                    +pow(MirrSource.Z-Listener.Z,2));
        // calculate delayed arrival time of reflection
		//RWD problem with release build...
		
		index = (i+NRovr2)*NR*NR+(j+NRovr2)*NR+(k+NRovr2);
		//this also stopped the optimizer doing bad things....
		//if(index%NR==0)
		//	printf("\nindex = %d\tX= %.4lf\tY=%.4lf\tZ=%.4lf\tDist = %.4lf",
		//		index,MirrSource.X,MirrSource.Y,MirrSource.Z,Dist);
																					
        TapDelay[index]=Dist/Vs*Fs;		
        ReflOrd = abs(i)+abs(j)+abs(k);
        // calculate attenuation for the reflection
		//RWD div/zero avoidance:
		if(Dist==0.0)
			TapGain[index] = 1.0;
		else
			TapGain[index]= pow(RefDist/Dist,2.0)*pow(ReflCoef,ReflOrd);		
      }
    }
  }
}

/***********************************************************************
InputParameters

This subroutine inputs parameters from a text file.  An example of
the format of the parameters file is below.  It also checks the
range of some of the parameters and creates a linked list representing
how the sound source changes position in the modeled room.

0.5               # Input gain for data file (float)
1                 # Open Path = 0 Closed Path = 1 (int)
6    5    4       # Room Dimensions (X,Y,Z) (float)
0.85              # Reflectivity of walls (float)
5                 # Number of rooms per dimension, 1 or greater
2    3    2       # Listener coord (X,Y,Z) (float)
2    2    2    1  # Source coord and time at coord (X,Y,Z,t) (float)
3    2    2    1  # Source coord and time at coord (X,Y,Z,t) (float)
4    3    2    1  # Source coord and time at coord (X,Y,Z,t) (float)
3    4    2    1  # Source coord and time at coord (X,Y,Z,t) (float)
2    4    2    1  # Source coord and time at coord (X,Y,Z,t) (float)

***********************************************************************/
position*
InputParameters(
  FILE *PositionFile,
  point &Room,
//  point &Source,
  point &Listener,
  double &ReflCoef,
  double &InGain,
  int &NR)
{
	position *First = new position,*Previous=First,*Next=Previous;
	int ClosedPath,LineCount=1;
	char StrLine[256];

	#define ReadParms(arg1)                           \
	if ((fgets(StrLine,256,PositionFile)) != NULL) {  \
		if (arg1)                                       \
			LineCount++;                                  \
		else                                            \
			InputParamsError(LineCount);                  \
	}                                                 \
	else                                              \
			InputParamsError(0);
		
	
#ifdef _DEBUG
	assert(PositionFile);
#endif
	ReadParms(sscanf(StrLine,"%le",&InGain)==1)
	ReadParms(sscanf(StrLine,"%d",&ClosedPath)==1)
	ReadParms(sscanf(StrLine,"%le %le %le",&Room.X,&Room.Y,&Room.Z)==3)
	ReadParms(sscanf(StrLine,"%le",&ReflCoef)==1)
	ReadParms(sscanf(StrLine,"%d",&NR)==1)
	ReadParms(sscanf(StrLine,"%le %le %le",&Listener.X,&Listener.Y,&Listener.Z)==3)

	if (NR%2 != 1) {
		printf ("Error: The number of reflected rooms per dimension must be odd (for symmetry)\n");
		exit(-1);
	}
	//RWD!!! was bitwise OR:  '|'
	if ((Listener.Y > Room.Y)||(Listener.X > Room.X)) { 					// Check listener position
		printf ("Error: Listener position puts listener outside of room \n");
		exit(-1);
	}		
  // Choose a default position for listener
	//RWD check this! am I always getting these setings????
	First->Coord.X = Room.X/2;
	First->Coord.Y = Room.Y/2;
	First->Coord.Z = Room.Z/2;
	First->Time = 0;
	First->NextSource=NULL;
	while (feof(PositionFile)==0) {                                              
		if ((fgets(StrLine,256,PositionFile)) != NULL) {
			if(sscanf(StrLine,"%le %le %le %le",&(Next->Coord.X),&(Next->Coord.Y),&(Next->Coord.Z),&(Next->Time))==4) {
				Next->NextSource = new position;
				Previous = Next;
				Next = Next->NextSource;
				LineCount++;
			}
			else {
				InputParamsError(LineCount);
			}
		}
	}
	delete (Previous->NextSource);
	if (ClosedPath)
		Previous->NextSource=First;
	else
		Previous->NextSource=NULL;
	return(First);
}

//RWD get params from cmdline: refl,nrefs,rX,rY,rZ,sX,sY,sZ,lX,lY,lZ
//fix ingain at 1.0
bool input_cmds(char *args[],int argc,point &room,point &source,point &listener,double &reflcoef,int &NR)
{
	double dval;
	int ival;
#ifdef _DEBUG
	assert(argc==11);
#endif
	if(argc != 11)
		return false;

	dval = atof(args[REFLECTIVITY]);
	if(dval <= 0.0 || dval > 1.0){
		fprintf(stderr,"\nbad value for liveliness\n");
		return false;
	}
	reflcoef = dval;

	ival = atoi(args[REFLECTIONS]);
	if(ival < 1){
		fprintf(stderr,"\nbad value for nrefs\n");
		return false;
	}

	NR = 1 + (2* ival);

	room.X = atof(args[R_X]);
	if(room.X <= 0.0){
		fprintf(stderr,"\nbad size for roomX\n");
		return false;
	}
	room.Y = atof(args[R_Y]);
	if(room.Y <= 0.0){
		fprintf(stderr,"\nbad size for roomY\n");
		return false;
	}
	room.Z = atof(args[R_Z]);
	if(room.Z <= 0.0){
		fprintf(stderr,"\nbad size for roomZ\n");
		return false;
	}
	listener.X = atof(args[L_X]);
	if(listener.X < 0.0){
		fprintf(stderr,"\nbad size for listenerX\n");
		return false;
	}
	listener.Y = atof(args[L_Y]);
	if(listener.Y < 0.0){
		fprintf(stderr,"\nbad size for listenerY\n");
		return false;
	}
	listener.Z = atof(args[L_Z]);
	if(listener.Z < 0.0){
		fprintf(stderr,"\nbad size for listenerZ\n");
		return false;
	}

	if ((listener.Y > room.Y)||(listener.X > room.X)|| (listener.Z > room.Z)) { // Check listener position
		printf ("\nError: Listener position puts listener outside room\n");
		return false;
	}

	source.X = atof(args[S_X]);
	if(source.X <= 0.0){
		fprintf(stderr,"\nbad size for sourceX\n");
		return false;
	}

	source.Y = atof(args[S_Y]);
	if(source.X <= 0.0){
		fprintf(stderr,"\nbad size for sourceY\n");
		return false;
	}

	source.Z = atof(args[S_Z]);
	if(source.X <= 0.0){
		fprintf(stderr,"\nbad size for sourceX\n");
		return false;
	}

	if ((source.Y > room.Y)||(source.X > room.X)|| (source.Z > room.Z)) { // Check source position
		printf ("\nError: Source position puts source outside room\n");
		return false;
	}


	return true;
}



/***********************************************************************
InputParamsError

This subroutine outputs an text error message to standard output
when an error is encountered in the InputParams subroutine.  The 
error text that is output is selected by the line number of the
input parameters file where the error occured.
***********************************************************************/
void
InputParamsError(
  int ErrNum)
{
	char ErrorStr[][256] = {
	"Error: EOF reached before all parameters input\n",
	"Error: Line %d requires 1 parameter the gain for the input file\n",
	"Error: Line %d requires 1 parameter (0 or 1): 0=closed path, 1=open path\n",
	"Error: Line %d requires 3 parameters (X,Y,Z): the room dimensions\n",
	"Error: Line %d requires 1 parameter the wall refelctivity\n",
	"Error: Line %d requires 1 parameter the number of mirrored rooms per dimension\n",
	"Error: Line %d requires 3 parameters (X,Y,Z): the listeners position\n",
	"Error: Line %d requires 4 parameters (X,Y,Z,t): sound position position and time\n"};
	if (ErrNum < 7)
		printf(ErrorStr[ErrNum],ErrNum);
	else
		printf(ErrorStr[7],ErrNum);
	exit(-1);

}

 
