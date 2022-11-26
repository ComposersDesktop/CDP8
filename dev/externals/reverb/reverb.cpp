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
 
/* MODULE FOR REVERB.EXE */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef _DEBUG
#include <assert.h>
#endif
#include <algorithm>
#include <vector>
using namespace std;
/***** FORMULA FOR LOOP GAIN *****

  g = pow(10, - ( ((60*ilooptime)/reverbtime) / 20))

  derived from Moore:

	reverbtime = 60/(-20.log10(g)) * ilooptime

  then we set igain = g(1 - filtgain), officially...

  */
extern "C" {
#include <sfsys.h>
#include <osbind.h>
#include <cdplib.h>	   //NB requires stdio.h etc - time to change this?
}
#include "reverberator.h"
#include <string.h>
void usage(void);
long readtaps(FILE *fp, deltap **taps,double sr);

//my library of early reflections...
//by doing it this way I can use the sizeof() operator on the array names
#include "reflect.cpp"

enum cmdargs {
	PROGNAME,
	INFILE,
	OUTFILE,
	RGAIN,
	MIX,
	REVERBTIME,	
	DAMPING,
	LPFREQ,
	TRTIME,
	CONFIG
};

const char* cdp_version = "5.0.0";

///////////// Moorer reverb
int
main(int argc,char *argv[])

{
	int		ifd=-1, ofd=-1,i;	
	//double  delaytime,gain;
	double	rvbtime, damping, trailertime = 0.0; 	
	long	chans,outchans = 2;
	double	sr;
	//all this stuff from Brandt  Csound implementation of posh Moorer
	MOORERDATA mrdata = {0.050,0.056,0.061,0.068,0.072,0.078,0.02,0.014,0.009,0.006};	
	double predelay = 0.0;
	moorer *moorerverb = 0;
	tapdelay *tdelay = 0;
	deltap *ptaps = 0;	
	allpassfilter **chan_ap = 0;		//ptr to array of allpassfilter ptrs
	tonecontrol *tc_l_lowcut = 0, *tc_r_lowcut = 0;
	// could be replaced with a tonecontrol for deeper slope
	onepole *l_highcut = 0, *r_highcut = 0, *lp = 0;
	FILE *fp_earlies = 0; 
	long ntaps;
	bool want_mono = false;
	bool want_floats = false;
	bool usertaps = false;
	double lowcut_freq = 0.0,highcut_freq = 0.0,lp_freq = 0.0;
	float dry_gain,wet_gain,diff_gain;
	CHPEAK *peaks;
	SFPROPS props;
	// test vcomb with variable delay taps 
	//vmtcomb4 vcomb;
	//fastlfo lfo1,lfo2,lfo3,lfo4;
	if((argc==2) && strcmp(argv[1],"--version")==0) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}

	if(sflinit("reverb")){
		sfperror("reverb: initialisation");
		exit(1);
	}
	if(argc < CONFIG) usage();

	while((argc > 1) && argv[1][0]=='-'){
		char *arg = argv[1];
		switch(*(++arg)){
		case('p'):
			if(*(++arg) =='\0'){
				fprintf(stderr,"\npredelay requires a parameter");
				usage();
			}			
			predelay = atof(arg);
			if(predelay <0.0){
				fprintf(stderr,"\npredelay must be >= 0.0");
				usage();
			}
			predelay *= 0.001;			//arg is in msecs
			break;
		case('c'):			
			if(*(++arg) == '\0') {
				fprintf(stderr,"\n:reverb: -c flag requires a value");
				usage();				
			}
			outchans = atoi(arg);
			if(outchans < 1 || outchans > 16){
				fprintf(stderr,"\nreverb: impossible channel value requested");
				usage();
			}
			if(outchans==1)
				want_mono = true;
			break;
		case('e'):
			if(*(++arg)=='\0'){
				fprintf(stderr,"\nreverb: -e flag needs a filename");
				usage();
			}
			if((fp_earlies = fopen(arg,"r")) ==0){
				fprintf(stderr,"\nreverb: unable to open breakpoint file %s",arg);
				exit(1);
			}
			usertaps = true;
			break;
		case('f'):
			want_floats = true;
			if(*(++arg) != '\0')
				fprintf(stderr,"\nreverb: WARNING: -f flag does not take a parameter");
			break;
		case('L'):
			if(*(++arg) == '\0'){
				fprintf(stderr,"\nreverb: Lowcut flag needs frequency argument");
				usage();
			}			
			lowcut_freq = atof(arg);
			if(lowcut_freq <= 0.0){
				fprintf(stderr,"\nreverb: Lowcut freq must be greater than zero");
				usage();
			}
			break;
		case('H'):
			if(*(++arg) == '\0'){
				fprintf(stderr,"\nreverb: Highcut flag needs frequency argument");
				usage();
			}			
			highcut_freq = atof(arg);
			if(highcut_freq <= 0.0){
				fprintf(stderr,"\nreverb: Highcut freq must be greater than zero");
				usage();
			}
			break;
		default:
			fprintf(stderr,"\nreverb: illegal flag option %s",argv[1]);
			usage();
			break;
		}
		argc--;
		argv++;
	}
	peaks = (CHPEAK *) malloc(sizeof(CHPEAK) * outchans);
	if(peaks==NULL){
		fputs("\nNo memory for PEAK data\n",stderr);
		exit(1);
	}
	chan_ap = new allpassfilter*[outchans];
	if(chan_ap==0){
		fputs("\nreverb: no memory for multi-channel diffusion",stderr);
		exit(1);
	}


	if(argc < CONFIG)
		usage();

	if(argv[INFILE] != 0) {
	      	if((ifd = sndopenEx(argv[INFILE],0,CDP_OPEN_RDONLY)) < 0) {
		   fprintf(stderr,"\nreverb: cannot open input file %s\n", argv[INFILE]);
		   exit(1);
		}
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
	
	sr = (double) props.srate;
	chans = props.chans;

	if(chans > 2){
		fprintf(stderr,"\nreverb works only on mono or stereo files");
		exit(1);
	}
	
	
	diff_gain = atof(argv[RGAIN]);
	if(diff_gain < 0.0 || diff_gain > 1.0 ){
		printf("\nreverb: rgain must be >= 0.0 and  <= 1.0");
		usage();
	}
	dry_gain = (float) atof(argv[MIX]);			  //global output gain from diffuser
	if(dry_gain < 0.0 || dry_gain >= 1.0 ){
		printf("\nreverb: mix must be  between 0.0 and 1.0");
		usage();
	}		
	wet_gain = 1.0f - dry_gain;
	//probably not very scientific, but it works intuitively...
	//wet_gain *= 2;		  
	

	rvbtime = atof(argv[REVERBTIME]);	  
	if(rvbtime <= 0.0){
	   fprintf(stderr,
		"\nreverb: rvbtime must be > 0\n");
		exit(1);
	}

	damping = atof(argv[DAMPING]);
	if(damping < 0.0 || damping > 1.0){
		fprintf(stderr,"\nreverb: absorb must be in the range 0.0 - 1.0\n");
		exit(1);
	}
	double filt_limit = sr / 3.0;
	lp_freq = atof(argv[LPFREQ]);
	if(lp_freq < 0.0 || lp_freq > filt_limit){
		printf("\napass: lp_freq must be within 0.0 to %.4lfHz",filt_limit);
		usage();
	}

	trailertime = atof(argv[TRTIME]);
	if(trailertime < 0.0)
		trailertime = 0.0;
	
	if(argc==CONFIG+1){
		int got = 0;
		double dtime1,dtime2,dtime3,dtime4,dtime5,dtime6,
			adtime1,adtime2,adtime3,adtime4;
		FILE *fp = fopen(argv[CONFIG],"r");
		if(!fp)
			printf("\nreverb: can't open datafile %s: using presets",argv[CONFIG]);
		else{
			got = fscanf(fp,"%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",
				&dtime1,
				&dtime2,
				&dtime3,
				&dtime4,
				&dtime5,
				&dtime6,
				&adtime1,			 //allpasses
				&adtime2,
				&adtime3,
				&adtime4				
				);
			fclose(fp);
			if(got != 10)
				printf("\nreverb: error reading looptime values");
			else {
				//create prime delay times from the msec vals
				//get this in a loop later on...				
				//RWD TODO: check times are not too huge!
				mrdata.ctime1 = dtime1 * 0.001;
				mrdata.ctime2 = dtime2 * 0.001;
				mrdata.ctime3 = dtime3 * 0.001;
				mrdata.ctime4 = dtime4 * 0.001;
				mrdata.ctime5 = dtime5 * 0.001;
				mrdata.ctime6 = dtime6 * 0.001;
				mrdata.atime1 = adtime1 * 0.001;
				mrdata.atime2 = adtime2 * 0.001;
				mrdata.atime3 = adtime3 * 0.001;
				mrdata.atime4 = adtime4 * 0.001;
			}
		}
	}

	//create input low/high filters, if wanted
	if(lowcut_freq > 0.0){
		if(highcut_freq > 0.0 && highcut_freq <= lowcut_freq){
			fprintf(stderr,"\nreverb: Highcut frequency	 must be higher than Lowcut frequency");
			usage();
		}
		//lowcut is based on low shelving eq filter; here fixed at 12dB
		tc_l_lowcut = new tonecontrol(lowcut_freq,-12.0,LOW_SHELVE,sr);
		if(!tc_l_lowcut->create()){
			fprintf(stderr,"\nreverb: unable to create Lowcut filter");
			exit(1);
		}
		if(chans==2){
			tc_r_lowcut = new tonecontrol(lowcut_freq,-12.0,LOW_SHELVE,sr);
			if(!tc_r_lowcut->create()){
				fprintf(stderr,"\nreverb: unable to create Lowcut filter");
				exit(1);
			}
		}
	}

	if(highcut_freq > 0.0){		
		l_highcut = new onepole(highcut_freq,sr,LOW_PASS);
		if(chans==2)
			r_highcut = new onepole(highcut_freq,sr,LOW_PASS);
	}

	if(lp_freq > 0.0)
		lp		= new onepole(lp_freq,sr,LOW_PASS);


	moorerverb =  new moorer(&mrdata,rvbtime,damping,sr);
	//further external allpasses will be used to create multi-channel diffusion
	
	for(i = 0; i < outchans; i++){
		chan_ap[i] = new allpassfilter(sr,to_prime(0.005 + 0.00025 * (double)i,sr),0.4 ,0);
		if(chan_ap[i] == 0) {
			fprintf(stderr,"\nreverb: no memory for output diffusers");
			exit(1);
		}
		if(!chan_ap[i]->create()){
			fprintf(stderr,"\nreverb: no memory for output diffusers");
			exit(1);
		}
	}
	if(usertaps){
#ifdef _DEBUG
		assert(fp_earlies);
#endif
		ntaps = readtaps(fp_earlies,&ptaps,sr);
		if(ntaps==0){
			fprintf(stderr,"\nreverb: error reading tapfile");
			exit(1);
		}
		printf("\nloaded %ld early reflections from tapfile",ntaps);
		fclose(fp_earlies);
		fp_earlies = 0;
	}
	else {
		if(rvbtime <= 0.6){
			ntaps = sizeof(smalltaps) / sizeof(deltap);
			ptaps = smalltaps;
			printf("\nsmall-room: using %ld early reflections",ntaps);
		}
		else if(rvbtime > 0.6 && rvbtime <= 1.3){
			ntaps = sizeof(mediumtaps) / sizeof(deltap);
			ptaps = mediumtaps;
			printf("\nmedium-room: using %ld early reflections",ntaps);
		}
		else{
			ntaps = sizeof(largetaps) / sizeof(deltap);
			ptaps = largetaps;
			printf("\nlarge-room: using %ld early reflections",ntaps);
		}
	}
	tdelay = new tapdelay(ptaps,ntaps,sr);

	//if user prescribes a predelay, adjust all taptimes
	if(predelay > 0.0){
		double current_delay = ptaps[0].pos;
		double adjust = predelay - current_delay;
		for(int i = 0; i < ntaps; i++)
			ptaps[i].pos += adjust;
	}
	
	if(!moorerverb->create()){
		fprintf(stderr,"\nreverb: unable to create reverberator");
		exit(1);
	}
	if(!tdelay->create()){
		fprintf(stderr,"\nreverb: unable to create early reflections");
		exit(1);
	}
	
	//double maxgain = tdelay->get_maxgain();
	//if(maxgain > 1.0)
	//	tdelay->set_gain(maxgain * early_gain);
	printf("\npredelay = %.4lf secs",ptaps[0].pos);
	//wet_gain *= floor(maxgain);
	if(want_floats)
		props.samptype =FLOAT32;
	props.chans = outchans;

	if((ofd = sndcreat_ex(argv[OUTFILE],-1,&props,SFILE_CDP,CDP_CREATE_NORMAL)) < 0) {
		  fprintf(stderr,"\nreverb: cannot open output file %s\n",argv[OUTFILE]);
		  exit(1);
	}


	printf("\n");
	long outsamps = 0;
	long step = (long) (0.25 * sr);
#ifdef NOTDEF
	// vcomb test
	long seed = lfo1.init((double)inprops.srate,0.0,seed);
	lfo1.set_WaveType(LFO_RANDOM);
	lfo1.set_freq(0.5);
	lfo1.set_mod(1.0); /* no change - we control modf range externally */
	lfo2.init((double)inprops.srate,0.0,seed);
	lfo2.set_WaveType(LFO_RANDOM);
	lfo2.set_freq(0.55);
	lfo2.set_mod(1.0);
	lfo3.init((double)inprops.srate,0.0,seed);
	lfo3.set_WaveType(LFO_RANDOM);
	lfo3.set_freq(0.61);
	lfo3.set_mod(1.0);
	lfo4.init((double)inprops.srate,0.0,seed);
	lfo4.set_WaveType(LFO_RANDOM);
	lfo4.set_freq(0.66);
	lfo4.set_mod(1.0);
	vcomb.init(double)srate,0.8);
	double delay1,delay2,delay3,delay4;
	double delaymod = 0.05;
	delay1 = 0.05;
	delay2 = 0.056;
	delay3 = 0,061;
	delay4 = 0.068;
	vcomb.init(double)srate,delay4 + (delay4 * delaymod));
	vcomb.setfgains(
#endif
	//stopwatch(1);
	//run main single-sample loop
	float l_ip,r_ip,l_out,r_out,l_direct,r_direct,mono_direct;
	for(;;){
		int rv;
		float out,earlies;
		//read mon/left channnel
		if((rv = fgetfloatEx(&l_ip,ifd,0)) < 0){
			fprintf(stderr,"\nreverb: error reading infile"); 
			exit(1);
		}
		if(!rv) break;	   // end of inputfile	- handle any trailertime
		//apply any conditioning to direct signal
		if(tc_l_lowcut)
			l_ip = (float) tc_l_lowcut->tick((double)l_ip);
		if(l_highcut)
			l_ip = (float) l_highcut->tick((double)l_ip);
		l_direct = l_ip;
		mono_direct = l_direct;
		r_direct = l_direct;
		//handle R channnel if active
		if(chans==2){
			if((rv = fgetfloatEx(&r_ip,ifd,0)) < 0){
				fprintf(stderr,"\nError in reading file"); 
				exit(1);			
			}		
			if(!rv) break;					
			//apply any conditioning to direct signal
			if(tc_r_lowcut)
				r_ip = (float) tc_r_lowcut->tick((double)r_ip);
			if(r_highcut)
				r_ip = (float) r_highcut->tick((double)r_ip);
			r_direct = r_ip;
			if(want_mono)
				mono_direct = (l_direct + r_direct) * 0.5f;			
		}
		//mono_direct = (mixed) input to reverberator
		//possibly also filter it...
		if(lp)
			mono_direct = (float) lp->tick(mono_direct);					//input lowpass
		earlies = tdelay->tick(mono_direct);				// ---> early reflections 	
		//send (filtered) mono output from reverberator
		out = earlies + moorerverb->tick(earlies + mono_direct) * diff_gain;
		//and send thru multi_channel diffusers
		//NB must handle first 2 chans as special cases, then loop thru the rest
		//l_out = want_mono ? mono_direct : l_direct;
		//l_out *= dry_gain;
		l_out = chan_ap[0]->tick(out) * wet_gain;
		if(want_mono)			
			l_out += (mono_direct * dry_gain);
		else 
			l_out += l_direct * dry_gain;
		
		
		if((float)fabs((double)l_out) > peaks[0].value) {
			peaks[0].value = (float)fabs((double)l_out);
			peaks[0].position = outsamps;
		}
		if(fputfloatEx(&l_out,ofd) < 1){
			fprintf(stderr,"\nreverb: error writing output file"); 
			exit(1);
		}
 
		if(outchans>=2){
			r_out = r_direct * dry_gain;
			r_out += chan_ap[1]->tick(out) * wet_gain;			
			if((float)fabs((double)r_out) > peaks[1].value) {
				peaks[1].value = (float)fabs((double)r_out);
				peaks[1].position = outsamps;
			}
			if(fputfloatEx(&r_out,ofd) < 1){
				fprintf(stderr,"\nreverb: error writing output file"); 
				exit(1);
			}
		}

		//now any further channels
		for(i=2;i < outchans; i++){						
			l_out = chan_ap[i]->tick(out) * wet_gain;		   			
			if((float)fabs((double)l_out) > peaks[i].value) {
				peaks[i].value = (float)fabs((double)l_out);
				peaks[i].position = outsamps;
			}
			if(fputfloatEx(&out,ofd) < 1){
				fprintf(stderr,"\nreverb: error writing output file"); 
				exit(1);
			}
		}
		outsamps++;
		if((outsamps % step) == 0)
			//inform(step,sr);
            fprintf(stdout,"%.2f\r",(double)outsamps/(double)sr);

	}
	//end of infile; play out reverb tail as much as we're allowed
	int trtime  = (int)( trailertime * sr);	
	for(i = 0; i < 	trtime; i++){
		float tr_l_ip,tr_r_ip = 0.0f, tr_out,tr_earlies,tr_l_direct,tr_mono_direct,tr_r_direct,
			tr_l_out,tr_r_out;
		tr_l_ip = 0.0;
		if(tc_l_lowcut)
			tr_l_ip = (float) tc_l_lowcut->tick(tr_l_ip);
		if(l_highcut)
			tr_l_ip = (float) l_highcut->tick(tr_l_ip);
		tr_l_direct = tr_l_ip;
		tr_mono_direct = tr_l_direct;
		tr_r_direct = tr_l_direct;
		//handle R channnel if active
		if(chans==2){
			tr_r_ip = 0.0;	   // inout is zero, except if using input filters
			// get any samples from input conditioning filters
			if(tc_r_lowcut)
				tr_r_ip = (float) tc_r_lowcut->tick(tr_r_ip);						   
			if(r_highcut)
				tr_r_ip = (float) r_highcut->tick(tr_r_ip);
			tr_r_direct = tr_r_ip;
			if(want_mono)
				tr_mono_direct = (tr_l_direct + tr_r_direct) *0.5f;			
		}
		if(lp)
			tr_mono_direct = (float) lp->tick(tr_mono_direct);
		tr_earlies = tdelay->tick(tr_mono_direct);
		//send (filtered) mono output from reverberator
		tr_out = tr_earlies +  moorerverb->tick(tr_earlies * tr_mono_direct) * diff_gain;
		tr_l_out = chan_ap[0]->tick(tr_out) * wet_gain;
		if(want_mono)			
			tr_l_out += (tr_mono_direct * dry_gain);
		else 
			tr_l_out += tr_l_direct * dry_gain;
		if((float)fabs((double)tr_l_out) > peaks[0].value) {
			peaks[0].value = (float)fabs((double)tr_l_out);
			peaks[0].position = outsamps;
		}
		if(fputfloatEx(&tr_l_out,ofd) < 1){
			fprintf(stderr,"\nreverb: error writing to output file"); 
			exit(1);
		}
		if(outchans>=2){
			tr_r_out = tr_r_direct * dry_gain;
			tr_r_out += chan_ap[1]->tick(tr_out) * wet_gain;	
			if((float)fabs((double)tr_r_out) > peaks[1].value) {
				peaks[1].value = (float)fabs((double)tr_r_out);
				peaks[1].position = outsamps;
			}
			if(fputfloatEx(&tr_r_out,ofd) < 1){
				fprintf(stderr,"\nreverb: error writing to output file"); 
				exit(1);
			}
		}

		for(int j = 2;j < outchans;j++){
			float ch_out;
			ch_out = chan_ap[j]->tick(tr_out)  * wet_gain;
			if((float)fabs((double)ch_out) > peaks[j].value) {
				peaks[j].value = (float)fabs((double)ch_out);
				peaks[j].position = outsamps;
			}
			if(fputfloatEx(&ch_out,ofd) < 1){
				fprintf(stderr,"\nreverb: error writing to output file"); 
				exit(1);
			}
		}
		outsamps++;
		if((outsamps % step) ==0)
			//inform(step,sr);
            fprintf(stdout,"%.2f\r",(double)outsamps/(double)sr);
	}
	//stopwatch(0);
	printf("\nPEAK data:\n");
	for(i=0;i < outchans;i++)
		printf("Channel %d: %.4f at frame %d: %.4lf secs\n",i+1,
			peaks[i].value,peaks[i].position, (double) peaks[i].position / (double)props.srate);
	sndputpeaks(ofd,outchans,peaks);
	sndcloseEx(ifd);
	
	if(sndcloseEx(ofd) < 0)
		fprintf(stderr,"\nwarning: error closing outfile");
	delete [] peaks;
	if(moorerverb)
		delete moorerverb;
	for(i=0;i < outchans;i++)
		delete chan_ap[i];
	delete [] chan_ap;
	delete tdelay;
	if(usertaps)
		delete [] ptaps;
	return 0;
}



void
usage(void)
{
	fprintf(stderr,"\nreverb: Multi-channel reverberator\n\tVersion 1.1 Release 4 CDP 1998,1999\n");
	fprintf(stderr,
	"usage: reverb [flags] infile outfile rgain mix rvbtime absorb\n"
	"                                 lpfreq trailertime [times.txt]\n"
	"flags    : any or all of the following:\n"
	"   -cN   : create outfile with N channels (1 <= N <= 16 : default =  2)\n"
	"   -eFILE: read early reflections from breakpoint textfile FILE\n"
	"   -f    : force floating-point output (default: infile format)\n"
	"   -HN   : apply Highcut filter with cutoff freq N Hz to infile (6db/oct)\n"
	"   -LN   : apply Lowcut filter with cutoff freq N Hz to infile	(12dB/oct)\n"
	"   -pN   : force reverb predelay to N msecs (shifts early reflections)\n"
	"rgain    : set level of dense reverb (0.0 <= rgain <= 1.0)\n"    
	"mix      : dry/wet balance (source and reverb): 1.0<-- mix -->= 0.0\n"
	"rvbtime  : reverb decay time (to -60dB) in seconds\n"
	"absorb   : degree of hf damping to suggest air absorption: 0.0 <= absorb <= 1.0\n"
	"lpfreq   : lowpass filter cutoff freq (Hz) applied at input to reverb\n" 
	"           to disable either filter (absorb, lpfreq), use the value 0\n" 
	"trtime   : trailer time added to outfile for reverb tail (secs)\n"
	"times.txt: list of delay times (msecs) for 6 comb and 4 allpass filters\n"
	);
	exit(0);
}

long readtaps(FILE *fp, deltap **taps,double sr)
{
	vector<deltap> vtaps;
	deltap thistap;
	char line[256];
	int ntaps = 0;
	int linecount = 1;
	bool error = false;
	double time= 0.0, current_time = 0.0, val = 0.0;

	double sample_duration = 1.0 / sr;
	if(fp==0 || sr <= 0.0){
		*taps = 0;
		return 0;
	}
	while(fgets(line,256,fp))	{
		int got;
		if(line[0] == '\n' || line[0]== ';'){	//allow comment lines; this does not check for leading white-space...
			linecount++;
			continue;
		}

		if((got = sscanf(line,"%lf%lf",&time,&val))<2){			
			fprintf(stderr,"\nerror in reflections file: line %d",linecount);
			error =true;
			break;
		}
		if(time < 0.0){
			fprintf(stderr,"\nerror in tapfile: line %d: bad time value",linecount);
			error = true;
			break;
		}
		//if (first) val = 0.0, or VERY close
		if(time < sample_duration)	{ //non-zero time must be at least one sample on!			
			fprintf(stderr,"\nWARNING: very small taptime %.4lf treated as zero",time);
			time = 0.0;			
		}

		if(current_time != 0.0 && time==current_time){
			fprintf(stderr,"\nWARNING: duplicate time in line %d: ignored",linecount);
			linecount++;
			continue;
		}
		if(time < current_time){
			fprintf(stderr,"\nerror in tapfile: time out of sequence: line %d",linecount);
			error = true;
			break;
		}
		current_time = time;
		thistap.pos = time;
		if(val <=0.0 || val > 1.0){
			fprintf(stderr,"\nerror: bad amplitude in tapfile: line %d",linecount);
			error = true;
			break;
		}
		thistap.val = val;
		vtaps.push_back(thistap);
		linecount++;
	}
	
	ntaps = vtaps.size();
	if(ntaps==0){
		fprintf(stderr,"\ntapfile contains no data!");
		error = true;
	}
	if(error){
		*taps = 0;
		return 0;
	}
	*taps = new deltap[ntaps];
	if(taps==0)
		return 0;
	vector<deltap>::iterator I =  vtaps.begin(); 
	for(int i = 0; I != vtaps.end();i++, I++){
		(*taps)[i].pos = I->pos;
		(*taps)[i].val = I->val;
	}

	return ntaps;
}