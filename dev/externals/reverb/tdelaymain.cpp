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
 
//tdelay.cpp
//mono/stereo tapped delay line
//RWD,CDP 1998
//VERSION: initial, 0.01
//usage: tdelay [-f] infile outfile feedback tapfile.txt [trailertime]
//		-f : floatsam output
//		feedback (double) - obvious...
// tapfile.txt	lines of at least two values: time  amplitude  [pan]
// times must be increasing, non-identical
// amplitude +- 1.0
// pan -1.0--1.0: -1 = hard Left, 1= hard Right, 0.0 = Centre
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
extern "C" {
#include <sfsys.h>
}
#include <props.h>
#include "reverberator.h"
#include <vector>
#include <algorithm>
using namespace std;

typedef struct stereo_tap {
	double time;
	double amp;
	double pan;
} STAP ;


void usage(void);
#define ROOT2	(1.4142136)
enum pan_direction {SIGNAL_TO_LEFT,SIGNAL_TO_RIGHT};

enum args {INFILE=1,OUTFILE,TAPGAIN,FEEDBACK,MIX,TAPFILE,TRTIME};

void pancalc(double position,double *leftgain,double *rightgain);

const char* cdp_version = "5.0.0";


int main(int argc, char **argv)
{
	int ifd,ofd,inchans;
	float feedback = 0.0f;
	float dryfac = 0.0f,wetfac = 1.0f;
	double trailertime = 0.0;
	double current_time = 0.0;
	deltap *l_taps = 0,*r_taps = 0;
	tapdelay *l_dline = 0, *r_dline = 0;
	unsigned int i,nchans = 1,ntaps = 0,trailersamps = 0;
	bool is_stereo = false;
	bool floatsams_wanted = false;
	bool mix_input = true;
	SFPROPS props;
	FILE *tapfile = 0;
	double sample_duration,tapgain;
	double max_lgain = 1.0,max_rgain= 1.0;
	float max_gain = 1.0f;
	double sr; 
	CHPEAK *peaks;
	
	if((argc==2) && strcmp(argv[1],"--version")==0) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}

	if(argc < TRTIME){
		usage();
		exit(1);
	}
	if(sflinit("tdelay")){
		sfperror("tdelay: initialisation");
		exit(1);
	}
	
	while(argv[1][0] == '-'){
		switch(argv[1][1]){
		//case('s'):
		//	is_stereo = true;
		//	break;
		case('f'):
			floatsams_wanted = true;
			break;
		default:
			fprintf(stderr,"\nillegal flag option %s",argv[1]);
			usage();
			exit(1);
			break;
		}
		argv++;
		argc--;
	}
	if(argc < TRTIME){
		usage();
		exit(1);
	}

	if((ifd = sndopenEx(argv[INFILE],0,CDP_OPEN_RDONLY)) < 0) {
		fprintf(stderr,"reverb: cannot open input file %s: %s\n", argv[1],rsferrstr);
		exit(1);
	}
	if(!snd_headread(ifd,&props)){
		fprintf(stderr,"\nerror reading infile header: %s\n",rsferrstr);
		sndcloseEx(ifd);
		exit(1);
	}
	if(props.type != wt_wave){
		fprintf(stderr,"\ninfile is not a waveform file");
		sndcloseEx(ifd);
		exit(1);
	}

	inchans = props.chans;
	if(inchans > 2){
		fprintf(stderr,"\nSorry - cannot process files with more than two channels!");
		sndcloseEx(ifd);
		exit(1);
	}
	sample_duration = 1.0 / props.srate;

	if(floatsams_wanted)
		props.samptype =  FLOAT32;
	

	tapgain = atof(argv[TAPGAIN]);
	if(tapgain <= 0.0){
		fprintf(stderr,"\nError: tapgain must be > 0.0\n");
		exit(1);
	}


	feedback = (float) atof(argv[FEEDBACK]);
	if(feedback < -1.0f || feedback > 1.0f){
		fprintf(stderr,"\nfeedback out of range	-1.0 to +1.0");
		usage();
		sndcloseEx(ifd);
		exit(1);
	}

	dryfac = (float) atof(argv[MIX]);
	if(dryfac < 0.0f || dryfac >= 1.0f){
		fprintf(stderr,"\nmix value %s out of range",argv[4]);
		usage();
		sndcloseEx(ifd);
		exit(1);
	}
	wetfac = 1.0f - dryfac;

	if(argc==TRTIME+1){
		trailertime = atof(argv[TRTIME]);
		if(trailertime < 0.0){
			fprintf(stderr,"\ntrailertime must be >= 0.0");
			exit(1);
		}
		trailersamps = (unsigned int) (trailertime * props.srate);
	}
	tapfile = fopen(argv[TAPFILE],"r");
	if(!tapfile){
		fprintf(stderr,"\nunable to open tapfile %s",argv[TAPFILE]);
		usage();
		sndcloseEx(ifd);
		exit(1);
	}

	//read all lines, into taps array
	vector<STAP> ltaps;
	STAP thistap;
	double this_time,this_amp,this_pan;
	char line[255];

	int linecount = 1;
	this_time = 0.0; this_amp = 0.0;
	bool read_error = false;
	vector<STAP>::iterator I;

	//RWD TODO: take a tap at time = 0.0 as spec of direct sound : amp and pan
	
	while(fgets(line,255,tapfile) != NULL){
		this_pan = 0.0;
		int got;
		if(line[0] == '\n' || line[0]== ';'){	//allow comment lines; this does not check for leading white-space...
			linecount++;
			continue;
		}
		if((got = sscanf(line,"%lf%lf%lf",&this_time,&this_amp,&this_pan)) < 2){
			fprintf(stderr,"\nerror in tapfile: line %d: insufficient parameters",linecount);
			read_error = true;
			break;
		}
		//time= 0 no good for a taptime!
		if(this_time < 0.0){
			fprintf(stderr,"\nerror in tapfile: line %d: bad time value",linecount);
			read_error = true;
			break;
		}
		if(this_time < sample_duration)	{ //non-zero time must be at least one sample on!			
			fprintf(stderr,"\nWARNING: very small taptime %.4lf treated as zero - ignoring mix value",this_time);
			this_time = 0.0;
			mix_input = false;
		}
		if(current_time != 0.0 && this_time==current_time){
			fprintf(stderr,"\nWARNING: duplicate time in line %d: ignored",linecount);
			linecount++;
			continue;
		}		
		if(this_time < current_time){
			fprintf(stderr,"\nerror in tapfile: time out of sequence: line %d",linecount);
			read_error = true;
			break;
		}
		current_time = this_time;
		thistap.time = this_time;
		if(this_amp < 0.0 || this_amp > 1.0){
			fprintf(stderr,"\nerror in tapfile: line %d: bad amplitude value",linecount);
			read_error = true;
			break;
		}
		
		thistap.amp = this_amp;
		if(got==3){
			thistap.pan = this_pan;
			is_stereo = true;
			nchans = 2;
		}
		else
			thistap.pan = 0.0;
		ltaps.push_back(thistap);
		linecount++;
	}

	if(read_error){
		sndcloseEx(ifd);
		exit(1);
	}
	ntaps = ltaps.size();
	if(ntaps==0){
		printf("\nfile contains no data!");
		sndcloseEx(ifd);
		exit(1);
	}
	
	if(!mix_input)
		printf("\nzero taptime used: taking input mix control from tapfile");
#ifdef _DEBUG
	printf("\nread %d taps",ntaps);
	
	for(I = ltaps.begin(); I != ltaps.end();I++)
		printf("\n%.4lf\t%.4lf\t%.4lf",I->time,I->amp,I->pan);	//or whatever.....

	printf("\nfeedback =  %.4f",feedback);
	printf("\ndryfac = %.4f, wetfac = %.4f",dryfac,wetfac);

#endif
	if(is_stereo)
		printf("\ncreating stereo outfile");
	else
		printf("\ncreating mono outfile");
	
	
	l_taps = new deltap[ntaps];
	
	if(is_stereo){
		r_taps = new deltap[ntaps];
		//and create stereo delay-lines
		int j;
		for(j = 0,I = ltaps.begin(); I != ltaps.end();j++,I++){
		//printf("\n%.4lf\t%.4lf\t%.4lf",I->time,I->amp,I->pan);	//or whatever.....
			double l_gain,r_gain;
			pancalc(I->pan,&l_gain,&r_gain);
			l_taps[j].pos = r_taps[j].pos = I->time;
			l_taps[j].val = I->amp * l_gain;
			r_taps[j].val = I->amp * r_gain;
		}
	}
	//else create a mono delayline
	else {
		int j;
		for(j = 0,I = ltaps.begin(); I != ltaps.end();j++,I++){				
			l_taps[j].pos = I->time;
			l_taps[j].val = I->amp;
		}
	}


	l_dline  = new tapdelay(l_taps,ntaps,props.srate);
	if(!l_dline->create()){
		fputs("\nunable to  create delay line - probably no memory",stderr);
		delete [] l_taps;
		sndcloseEx(ifd);
		exit(1);
	}
	max_lgain = l_dline->get_maxgain();
	if(is_stereo){
		r_dline = new tapdelay(r_taps,ntaps,props.srate);
		if(!r_dline->create()){
			fputs("\nunable to  create stereo delay line - probably no memory",stderr);
			delete [] l_taps;
			delete [] r_taps;
			delete l_dline;
			sndcloseEx(ifd);
			exit(1);
		}
		max_rgain = r_dline->get_maxgain();
	}
	if(is_stereo)
		max_gain = (float) min(max_lgain,max_rgain);
	else
		max_gain = (float) max_lgain;

	/* recale max_gain by user param, eg:*/
	max_gain *= (float) tapgain;


	l_dline->set_gain((double)max_gain);
	if(is_stereo)
		r_dline->set_gain((double)max_gain);


	props.chans = nchans;
	peaks = (CHPEAK *) malloc(sizeof(CHPEAK) * nchans);
	if(peaks==NULL){
		fputs("No memory for PEAK data",stderr);
		exit(1);
	}
	for(i=0; i < nchans; i++){
		peaks[i].value = 0.0f;
		peaks[i].position = 0;
	}
	//OK, now open outfile!

	ofd = sndcreat_ex(argv[OUTFILE], -1,&props,SFILE_CDP,CDP_CREATE_NORMAL); 
	//ofd = sndcreat_formatted(argv[OUTFILE],-1,outsamp_type,nchans,props.srate,CDP_CREATE_NORMAL);
	if(ofd < 0){
		printf("\nunable to open outfile %s",argv[OUTFILE]);
		delete [] l_taps;
		delete l_dline;
		if(r_taps)
			delete [] r_taps;
		sndcloseEx(ifd);
		exit(1);
	}
	printf("\n");

	float l_op = 0.0f;		  
	float r_op = 0.0f;
	float l_out = 0.0f;
	float r_out = 0.0f;
	long outsamps = 0;
	long step = (long) (0.25 * props.srate);
	//RWD.10.98 BIG TODO: optimize all this!
	sr = (double) props.srate;
	if(is_stereo){
		for(;;){
			int rv;
			float ip,l_ip,r_ip;										
			if((rv = fgetfloatEx(&l_ip,ifd,0)) < 0){ 
					fprintf(stderr,"\nerror reading infile data");
					delete [] l_taps;
					delete l_dline;
					if(r_taps)
						delete [] r_taps;
					sndcloseEx(ifd);
					exit(1);				
			}
			if(!rv) 
				break;

			ip = l_ip;
			if(inchans==2){
				//mix stereo ip to mono for delay purposers
				if((rv = fgetfloatEx(&r_ip,ifd,0)) < 0){ 
					fprintf(stderr,"\nerror reading infile data");
					delete [] l_taps;
					delete l_dline;
					if(r_taps)
						delete [] r_taps;
					sndcloseEx(ifd);
					exit(1);				
				}
				if(!rv) 
				break;
				ip = (l_ip + r_ip) * 0.5f;
			}
			
		
			
			if(feedback != 0.0f) {
				l_op = l_dline->tick(ip + (l_op * feedback));
				r_op = r_dline->tick(ip + (r_op * feedback));
			}
			else{
				l_op = l_dline->tick(ip);
				r_op = r_dline->tick(ip);
			}

			//mix mono or stereo input with panned output:
			if(mix_input){
				l_out = l_op * wetfac + l_ip * dryfac;
				r_out = r_op * wetfac;
				if(inchans==2)
					r_out  += r_ip * dryfac;
				else
					r_out += l_ip * dryfac;
			}
			else{
				l_out = l_op;
				r_out = r_op;
			}
			/* rescale */
			l_out *=  max_gain;
			r_out *=  max_gain;
			if((float)fabs((double)l_out) > peaks[0].value) {
				peaks[0].value = (float)fabs((double)l_out);
				peaks[0].position = outsamps;
			}
			if((float)fabs((double)r_out) > peaks[1].value) {
				peaks[1].value = (float)fabs((double)r_out);
				peaks[1].position = outsamps;
			}


			if(fputfloatEx(&l_out,ofd) < 1){
				fprintf(stderr,"\nerror writing to outfile: %s",sferrstr());
				delete [] l_taps;
				delete l_dline;
				if(r_taps)
					delete [] r_taps;
				sndcloseEx(ifd);
				exit(1);
			}
			if(fputfloatEx(&r_out,ofd) < 1){
				fprintf(stderr,"\nerror writing to outfile: %s",sferrstr());
				delete [] l_taps;
				delete l_dline;
				if(r_taps)
					delete [] r_taps;
				sndcloseEx(ifd);
				exit(1);
			}
			outsamps++;
			if((outsamps % step) == 0)
				//inform(step,props.srate);
				fprintf(stdout,"%.2f\r",(double)outsamps/sr);
		}
		for(i = 0; i < trailersamps;i++){
			if(feedback != 0.0f){
				l_op = l_dline->tick(feedback * l_op);
				r_op = r_dline->tick(feedback * r_op);
			}
			else {
				l_op = l_dline->tick(0.0f);
				r_op = r_dline->tick(0.0f);
			}
			if(mix_input){			
				l_out = l_op * wetfac;
				r_out = r_op * wetfac;
			}
			else {
				l_out = l_op;
				r_out = r_op;
			}
			/* rescale */
			l_out *=  max_gain;
			r_out *=  max_gain;
			if((float)fabs((double)l_out) > peaks[0].value) {
				peaks[0].value = (float)fabs((double)l_out);
				peaks[0].position = outsamps;
			}
			if((float)fabs((double)r_out) > peaks[1].value) {
				peaks[1].value = (float)fabs((double)r_out);
				peaks[1].position = outsamps;
			}
			if(fputfloatEx(&l_out,ofd) < 1){
				fprintf(stderr,"\nerror writing to outfile: %s",sferrstr());
				delete [] l_taps;
				delete l_dline;
				if(r_taps)
					delete [] r_taps;
				sndcloseEx(ifd);
				exit(1);
			}
			if(fputfloatEx(&r_out,ofd) < 1){
				fprintf(stderr,"\nerror writing to outfile: %s",sferrstr());
				delete [] l_taps;
				delete l_dline;
				if(r_taps)
					delete [] r_taps;
				sndcloseEx(ifd);
				exit(1);
			}
			outsamps++;
			if((outsamps % step) == 0)
				//inform(step,props.srate);
				fprintf(stdout,"%.2f\r",(double)outsamps/sr);
		}
	}

	else{
		for(;;){
			int rv;
			float ip,l_ip,r_ip;			
			if((rv = fgetfloatEx(&l_ip,ifd,0)) < 0){ 
				fprintf(stderr,"\nerror reading infile data");
				delete [] l_taps;
				delete l_dline;
				sndcloseEx(ifd);
				exit(1);

			}
			if(!rv) 
				break;
			if(inchans==2){
				//mix stereo ip to mono for delay purposers
				if((rv = fgetfloatEx(&r_ip,ifd,0)) < 0){ 
					fprintf(stderr,"\nerror reading infile data");
					delete [] l_taps;
					delete l_dline;
					sndcloseEx(ifd);
					exit(1);				
				}
				if(!rv) 
				break;
				ip = (l_ip + r_ip) * 0.5f;
			}
			else
				ip = l_ip;
			if(feedback != 0.0f)
				l_op = l_dline->tick(ip + l_op * feedback);
			else
				l_op = l_dline->tick(ip);

			if(mix_input)
				l_out = l_op * wetfac + l_ip * dryfac;
			else
				l_out = l_op;
			/* rescale */
			l_out *=  max_gain;
			if((float)fabs((double)l_out) > peaks[0].value) {
				peaks[0].value = (float)fabs((double)l_out);
				peaks[0].position = outsamps;
			}
			
			if(fputfloatEx(&l_out,ofd) < 1){
				fprintf(stderr,"\nerror writing to outfile");
				delete [] l_taps;
				delete l_dline;
				sndcloseEx(ifd);
				exit(1);
			}
			outsamps++;
			if((outsamps % step) == 0)
				//inform(step,props.srate);
				fprintf(stdout,"%.2f\r",(double)outsamps/sr);
		}
		for(i = 0; i < trailersamps;i++){						
			l_op = l_dline->tick(feedback * l_op);
			if(mix_input)
				l_out = l_op * wetfac;
			else
				l_out = l_op;
			if((float)fabs((double)l_out) > peaks[0].value) {
				peaks[0].value = (float)fabs((double)l_out);
				peaks[0].position = outsamps;
			}
			if(fputfloatEx(&l_out,ofd) < 1){
				fprintf(stderr,"\nerror writing to outfile");
				delete [] l_taps;
				delete l_dline;
				sndcloseEx(ifd);
				exit(1);
			}
			outsamps++;
			if((outsamps % step) == 0)
				//inform(step,props.srate);
				fprintf(stdout,"%.2f\r",(double)outsamps/sr);
		}
	}
	fprintf(stdout,"%.2f\n",(double)outsamps/sr);
	printf("\nPEAK data:\n");
	for(i=0;i < nchans;i++)
		printf("Channel %u: %.4f at frame %d: %.4lf secs\n",i+1,
			peaks[i].value,peaks[i].position, (double) peaks[i].position / (double)props.srate);
	sndputpeaks(ofd,nchans,peaks);
	if(sndcloseEx(ofd) < 0){
		fprintf(stderr,"\nwarning: error closing outfile");

	}
	delete [] l_taps;
	delete l_dline;
	if(is_stereo) {
		delete [] r_taps;
		delete r_dline;
	}
	
	sndcloseEx(ifd);
	return 0;
}

	
void usage(void)
{
	printf("\n*******  STEREO MULTI-TAPPED DELAY WITH PANNING v1.0 1998 : CDP Release 4 ******\n");
	printf("\nusage:\n\ttapdelay [-f] infile outfile tapgain feedback mix taps.txt [trailtime]\n");
	printf("\n\t-f\t\twrite floating-point outfile");
	printf("\n\t\t\t(default: outfile has same format as infile)");
	printf("\n\ttapgain\t\tgain factor applied to output from delayline");
	printf("\n\t\t\t(tapgain > 0.0, typ 0.25)");
	printf("\n\tfeedback\trange: -1 <= feeedback <= 1.0");
	printf("\n\tmix\t\tproportion of source mixed with delay output.\n\t\t\trange: 0.0 <= mix < 1.0");
	printf("\n\ttrailtime\textra time in seconds (beyond infile dur) ");
	printf("\n\t\t\t  for delays to play out.");
	printf("\n\tStereo inputs are mixed to mono at input to the delay line.\n");
	printf("\n\ttaps.txt consists of list of taps in the format:");
	printf("\n\ttime amp [pan]");
	printf("\n\n\tAll values are floating-point, one tap per line.");
	printf("\n\tTimes (seconds) must be increasing. Duplicate times are ignored.");
	printf("\n\tA zero time (no delay) overrides the mix parameter,");
	printf("\n\t  and determines the level and pan of the (mono) input.");	
	printf("\n\tAmp values must be in the range 0.0 to 1.0");
	printf("\n\tEmpty lines are permitted, as are lines starting with  a semni-colon,");	
	printf("\n\t  which may be used for comments.");
	printf("\n\tIf a pan value is used in any line, outfile will be stereo.");
	printf("\n\tPan values are nominally in the range -1 to +1: 0 = centre (mono)");
	printf("\n\t within which constant-power panning is used.");
	printf("\n\tValues beyond these limits result in attenuation according to the");
	printf("\n\t   inverse-square law, to suggest distance beyond the speakers.\n");
	
}

//TW's constant-power pan routine, even does inv-square reductions beyond the speakers
void pancalc(double position,double *leftgain,double *rightgain)
{
	int dirflag;
	double temp;
	double relpos;
	double reldist, invsquare;

	if(position < 0.0)
		dirflag = SIGNAL_TO_LEFT;		/* signal on left */
	else
		dirflag = SIGNAL_TO_RIGHT;

	if(position < 0) 
		relpos = -position;
	else 
		relpos = position;
	if(relpos <= 1.0){		/* between the speakers */
		temp = 1.0 + (relpos * relpos);
		reldist = ROOT2 / sqrt(temp);
		temp = (position + 1.0) / 2.0;
		*rightgain = temp * reldist;
		*leftgain = (1.0 - temp ) * reldist;
	} else {				/* outside the speakers */
		temp = (relpos * relpos) + 1.0;			 /*RWD: NB same in both cases! */
		reldist  = sqrt(temp) / ROOT2;   /* relative distance to source */
		invsquare = 1.0 / (reldist * reldist);
		if(dirflag == SIGNAL_TO_LEFT){
			*leftgain = invsquare;
			*rightgain = 0.0;
		} else {   /* SIGNAL_TO_RIGHT */
			*rightgain = invsquare;
			*leftgain = 0;
		}
	}
}
