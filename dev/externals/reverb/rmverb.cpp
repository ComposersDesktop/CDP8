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
 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifdef _DEBUG
#include <assert.h>
#endif
#include <algorithm>
#include <vector>

using namespace std;
// TODO: clean up "new" handling, move to VC7 and get to use exceptions!
enum cmdargs {
		PROGNAME,
		INFILE,
		OUTFILE,
		REVERBTYPE,
		RGAIN,
		MIX,
		FEEDBACK,		
		LPABSORB,
		LPFREQ,
		TRTIME,
		CONFIG
};

enum reverb_type {
		DIFF_SMALL,
		DIFF_MEDIUM,
		DIFF_LARGE
};

extern "C" {
#include "portsf.h"
	   //NB requires stdio.h etc - time to change this?
}

#include "reverberator.h"
//RWD.10.98 TODO: for final versions, MUST limit input delay times! can't have 108sec delays.....
//get all reflection data
#include "reflect.cpp"


long readtaps(FILE *fp, deltap **taps,double sr);		

const char* cdp_version = "6.0.0";

void usage(void);

int
main(int argc,char *argv[])

{
	int i,ifd, ofd,reverbtype = DIFF_SMALL;
	FILE *fp = 0;
	long	chans,outchans = 2;
	double	sr,trailertime;
	cdp_processor *cdpdiff = 0;
	NESTDATA data1,data2;
	PSF_CHPEAK *peaks;
	PSF_PROPS props;
	unsigned int time1a,time1b,time1c,time2a,time2b,time2c;
	double feedback,fb_lpfreq, lp_freq,predelay = -1.0;
	double nyquist,pseudo_nyquist;
	double dry_gain,wet_gain;
	//double early_gain = 1.0;
	double diff_gain = 1.0;
	// nested-allpass diffusers
	small_diffuser *s_diffuser = 0;
	medium_diffuser *m_diffuser = 0;
	large_diffuser *l_diffuser = 0;
	// traioling plain allpasses for front pair de-correlation
	allpassfilter *ap1_l = 0,*ap2_l = 0;		
	allpassfilter *ap1_r = 0,*ap2_r = 0;
	// decorrelation for other channels (surround)
	allpassfilter **chan_ap = 0;
	onepole *lp = 0;
	// tonecontrols max -12dB slope, variable; 
	tonecontrol *tc_l_lowcut = 0, *tc_r_lowcut = 0;	
	tonecontrol *l_highcut = 0, *r_highcut = 0;
	double lowcut_freq = 0.0,highcut_freq = 0.0;
	tapdelay *tdelay = 0;	
	long ntaps = 0;
	deltap *ptaps = 0;
	bool want_mono = false;
	bool want_floats = false;
	bool usertaps = false;
	bool double_damping = false;

	if((argc==2) && strcmp(argv[1],"--version")==0) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(psf_init()){
		fprintf(stderr,"rmverb: initialisation failed\n");
		return 1;
	}
    	
	if(argc < CONFIG){
		if(argc > 1)
			fprintf(stderr,"rmverb: insufficient arguments\n");
		usage();
	}
	
	while((argc > 1) && argv[1][0]=='-'){
		char *arg = argv[1];
		switch(*(++arg)){
		case('p'):
			if(*(++arg) =='\0'){
				fprintf(stderr,"rmverb: predelay requires a parameter\n");
				usage();
			}
			
			predelay = atof(arg);
			if(predelay <0.0){
				fprintf(stderr,"rmverb: predelay must be >= 0.0\n");
				usage();
			}
			predelay *= 0.001;			//get msecs from user
			break;
		case('c'):
			if(*(++arg) == '\0') {
				fprintf(stderr,"rmverb: -c flag requires a value\n");
				usage();
			}
			outchans = atoi(arg);
			if(outchans < 1 || outchans > 16){
				fprintf(stderr,"rmverb: impossible channel value requested\n");
				usage();
			}
			if(outchans==1)
				want_mono = true;			
			break;
		case('d'):
			double_damping = true;
			break;
		case('e'):
			if(*(++arg)=='\0'){
				fprintf(stderr,"rmverb: -e flag needs a filename\n");
				usage();
			}
			if((fp = fopen(arg,"r")) ==0){
				fprintf(stderr,"rmverb: unable to open breakpoint file %s\n",arg);
				exit(1);
			}
			usertaps = true;
			break;
			// -f is CDP-specific: read old 32bit int format as floats
		case('f'):
			want_floats = true;
			if(*(++arg) != '\0')
				fprintf(stderr,"rmverb WARNING: -f flag does not take a parameter\n");
			break;
		case('L'):
			if(*(++arg) == '\0'){
				fprintf(stderr,"rmverb: -L flag needs frequency argument\n");
				usage();
			}			
			lowcut_freq = atof(arg);
			if(lowcut_freq <= 0.0){
				fprintf(stderr,"rmverb: Lowcut freq must be greater than zero\n");
				usage();
			}
			break;
		case('H'):
			if(*(++arg) == '\0'){
				fprintf(stderr,"rmverb: -H flag needs frequency argument\n");
				usage();
			}			
			highcut_freq = atof(arg);
			if(highcut_freq <= 0.0){
				fprintf(stderr,"rmverb: Highcut freq must be greater than zero\n");
				usage();
			}
			break;
		default:
			fprintf(stderr,"rmverb: illegal flag option %s\n",argv[1]);
			usage();
			break;
		}
		argc--;
		argv++;
	}

	if(argc < CONFIG){
		printf("rmverb: insufficient arguments\n");
		usage();
	}
#ifdef _DEBUG
	if(predelay > 0.0)
		printf("got predelay = %.4lf\n",predelay);
#endif

	chan_ap = new allpassfilter*[outchans];
	if(chan_ap==0){
		fprintf(stderr,"rmverb: no memory for multi-channel diffusion\n");
		exit(1);
	}

	if((ifd = psf_sndOpen(argv[INFILE],&props,0)) < 0) {
				fprintf(stderr,"\nrmverb: cannot open input file %s", argv[INFILE]);
				exit(1);
	}
	
	if(props.format  <= PSF_FMT_UNKNOWN || props.format > PSF_AIFC){
		fprintf(stderr,"infile is not a recognised soundfile\n");
		psf_sndClose(ifd);
		return 1;
	}
	
	sr = (double) props.srate;
	nyquist = sr / 2.0;
	pseudo_nyquist = nyquist * 0.7; // filters not reliable close to nyquist

	chans = props.chans;
	if(chans > 2){
		fprintf(stderr,"rmverb can only accept mono or stereo files\n");
		exit(1);
	}
		
	
	reverbtype =  atoi(argv[REVERBTYPE]) - 1;
	if((reverbtype < DIFF_SMALL) || (reverbtype > DIFF_LARGE)){
		fprintf(stderr,"rmverb: rmsize must be 1,2 or 3\n");
		exit(1);
	}
	
	diff_gain = atof(argv[RGAIN]);
	if(diff_gain <= 0.0 || diff_gain > 1.0){
		fprintf(stderr,"rgain must be > 0 and <= 1.0\n");
		exit(1);
	}

	dry_gain =  atof(argv[MIX]);			  //global output gain from diffuser
	if(dry_gain < 0.0 || dry_gain > 1.0 ){
		printf("reverb: mix must be  between 0.0 and 1.0\n");
		usage();
	}
	wet_gain = 1.0f - dry_gain;
	// some odd people like to have 100% wet signal...
	wet_gain *= 2.0;					//not v scientific, but works...
	
	feedback = atof(argv[FEEDBACK]);			  //feedback in diffuser
	if(feedback < 0.0 || feedback >1.0){
		printf("rmverb: feedback must be within 0.0 - 1.0\n");
		usage();
	}
	
	fb_lpfreq = atof(argv[LPABSORB]);			 //feedback lp-filter in diffuser
	if(fb_lpfreq < 0.0 || fb_lpfreq > pseudo_nyquist){
		printf("rmverb: absorb must be within 0 to %f\n",pseudo_nyquist);
		usage();
	}

	lp_freq = atof(argv[LPFREQ]);
	if(lp_freq < 0.0 || lp_freq > pseudo_nyquist){
		printf("rmverb: lpfreq must be within 0.0 to %.4lfHz\n",pseudo_nyquist);
		usage();
	}
	trailertime = atof(argv[TRTIME]);
	if(trailertime < 0.0){
		fprintf(stderr,"rmverb: trtime must be >= 0.0\n");
		usage();
	}
	//from -m flag; will override a stereo input too
	if(want_mono)
		outchans =  1;
	// NB: now specifying two trailing allpasses for diffuser
	// handles front pair; currently using chan_ap[] only for remaining channels
	// values are experimental!
	double apdelay = 0.02;
	double apfac = 0.02 / outchans;
	for(i = 0; i < outchans; i++){						// or use a random modifier?
		chan_ap[i] =  new allpassfilter(sr,to_prime(apdelay - apfac * (double)i,sr),0.4,0);
		if(chan_ap[i] ==0){
			fprintf(stderr,"rmverb: no memory for output diffusers\n");
			exit(1);
		}
		if(!chan_ap[i]->create()){
			fprintf(stderr,"rmverb: no memory for output diffusers\n");
			exit(1);
		}
	}
    // custom early reflections from time/value text-file
	if(usertaps){
#ifdef _DEBUG
		assert(fp);
#endif
		ntaps = readtaps(fp,&ptaps,sr);
		if(ntaps==0){
			fprintf(stderr,"rmverb: error reading tapfile\n");
			exit(1);
		}
		printf("rmverb: loaded %ld early reflections from tapfile\n",ntaps);
		fclose(fp);
		fp = 0;
	}


	//create input low/high filters, if wanted
	if(lowcut_freq > 0.0){
		if(highcut_freq > 0.0 && highcut_freq <= lowcut_freq){
			fprintf(stderr,"rmverb: Highcut frequency	 must be higher than Lowcut frequency\n");
			usage();
		}
		//lowcut is based on low shelving eq filter; here fixed at 12dB
		// but can easily add this as user param
		tc_l_lowcut = new tonecontrol(lowcut_freq,-12.0,LOW_SHELVE,sr);
		if(!tc_l_lowcut->create()){
			fprintf(stderr,"rmverb: unable to create Lowcut filter\n");
			exit(1);
		}
		if(chans==2){
			tc_r_lowcut = new tonecontrol(lowcut_freq,-12.0,LOW_SHELVE,sr);
			if(!tc_r_lowcut->create()){
				fprintf(stderr,"rmverb: unable to create Lowcut filter\n");
				exit(1);
			}
		}
	}

	if(highcut_freq > 0.0){		
		l_highcut = new tonecontrol(highcut_freq,-12.0,HIGH_SHELVE,sr);
		if(!l_highcut->create()){
			fprintf(stderr,"rmverb: unable to create Highcut filter\n");
				exit(1);
		}
		if(chans==2){
			r_highcut = new tonecontrol(highcut_freq,-12.0,HIGH_SHELVE,sr);
			if(!r_highcut->create()){
				fprintf(stderr,"\nrmverb: unable to create Highcut filter");
				exit(1);
			}
		}
	}

	//now create diffuser

	switch(reverbtype){
		case(DIFF_SMALL):
			time1a = to_prime(0.0047, sr);
			time1b = to_prime(0.022, sr);
			time1c = to_prime(0.0083,sr);

			time2a = to_prime(0.036,sr);
			time2b = to_prime(0.03,sr);
			time2c = 0;//dummy
			data1.time1 = time1a;
			data1.time2 = time1b;
			data1.time3 = time1c;
			data1.gain1 = 0.3;
			data1.gain2 = 0.4;
			data1.gain3 = 0.6;
			data1.delay1 = 0;
			data1.delay2 = 0;
			data2.time1 = time2a;
			data2.time2 = time2b;
			data2.time3 = time2c;
			data2.gain1 = 0.1;
			data2.gain2 = 0.4;
			data2.gain3 = 0.0;			//not used by gardner
			data2.delay1 = 0;
			data2.delay2 = 0;


			break;
		case(DIFF_MEDIUM):
			time1a = to_prime(/*0.0047*/0.035, sr);
			time1b = to_prime(0.0083, sr);
			time1c = to_prime(0.022,sr);

			time2a = to_prime(/*0.0292*/0.039,sr);
			time2b = to_prime(0.0098,sr);
			time2c = 0;//dummy
			data1.time1 = time1a;
			data1.time2 = time1b;
			data1.time3 = time1c;
			data1.gain1 = 0.3;
			data1.gain2 = 0.7;
			data1.gain3 = 0.5;
			data1.delay1 = 0;
			data1.delay2 = 0;
			data2.time1 = time2a;
			data2.time2 = time2b;
			data2.time3 = time2c;
			data2.gain1 = 0.3;
			data2.gain2 = 0.6;
			data2.gain3 = 0.0;			//not used by gardner
			data2.delay1 = 0;
			data2.delay2 = 0;



			break;
		case(DIFF_LARGE):
			// VERY LARGE!
			// NB: orig outer delays from Gardner generate very strong discrete echoes,
			// and hence also some pulsing in the dense reverb
			// for really large spaces (caverns) this is OK, otherwise we ant to control this
			// via predelay, and earlies
			// NOTE: this will NOT WORK when sharing a single delay line!
			// in that case, outer delay MUST be > internal ones
			// TODO: devise user param to control a range of delay values
			// (maybe derived from feedback level?), to further scale room size
			time1a = to_prime(0.03 /*0.087*/, sr);
			time1b = to_prime(0.062, sr);
			time1c = to_prime(0.022,sr);	//dummy: not used

			time2a = to_prime(0.018/*0.12*/,sr);
			time2b = to_prime(0.076,sr);
			time2c = to_prime(0.03,sr);		
			data1.time1 = time1a;
			data1.time2 = time1b;
			data1.time3 = time1c;
			data1.gain1 = 0.5;
			data1.gain2 = 0.25;
			data1.gain3 = 0.0;			//not used
			data1.delay1 = 0;
			data1.delay2 = 0;
			data2.time1 = time2a;
			data2.time2 = time2b;
			data2.time3 = time2c;
			data2.gain1 = 0.5;
			data2.gain2 = 0.25;
			data2.gain3 = 0.25;		
			data2.delay1 = 0;
			data2.delay2 = 0;
			break;
		default:
#ifdef _DEBUG
			assert(false);
#endif
			break;
	}
#ifdef _DEBUG
	//RWD: development only!
	if(argc==CONFIG+1){
		int got = 0;
		double time1a,time1b,time1c,time2a,time2b,time2c,gain1a,gain1b,gain1c,gain2a,gain2b,gain2c;
		FILE *fp = fopen(argv[CONFIG],"r");
		if(!fp)
			printf("rmverb: can't open datafile %s, using presets\n",argv[CONFIG]);
		else {
		   // time1a,gain1a,time2a,gain2a,time3a,gain3a...
			printf("loading diffusion data...\n");
			got = fscanf(fp,"%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",
			  &time1a,&gain1a,
			  &time1b,&gain1b,
			  &time1c,&gain1c,
			  &time2a,&gain2a,
			  &time2b,&gain2b,
			  &time2c,&gain2c);
			fclose(fp);
			if(got != 12)
				printf("rmverb: error reading data values\n");
			else {
				data1.time1 = to_prime(time1a,sr);
				data1.time2 = to_prime(time1b,sr);
				data1.time3 = to_prime(time1c,sr);
				data1.gain1 = gain1a;
				data1.gain2 = gain1b;
				data1.gain3 = gain1c;

				data2.time1 = to_prime(time2a,sr);
				data2.time2 = to_prime(time2b,sr);
				data2.time3 = to_prime(time2c,sr);
				data2.gain1 = gain2a;
				data2.gain2 = gain2b;
				data2.gain3 = gain2c;
			}
		}
	}
	
	else{	
		printf("using preset parameters:\n");
		printf("DATA 1: time1 = %u\n\ttime2 = %u\n\ttime3 = %u\n",data1.time1,data1.time2,data1.time3);
		printf("DATA 2: time1 = %u\n\ttime2 = %u\n\ttime3 = %u\n",data2.time1,data2.time2,data2.time3);
		printf("feedback = %.4lf,fb_lpfreq = %.4lf\n",feedback,fb_lpfreq);
		printf("dry gain = %.4lf, wet gain = %.4lf\n",dry_gain,wet_gain);
	}
#endif	

	
	switch(reverbtype){
	case(DIFF_SMALL):	
		printf("rmverb: using small room model\n");
		s_diffuser = new small_diffuser((unsigned int)(0.024 * sr),&data1,&data2,feedback,fb_lpfreq,sr);
		s_diffuser->set_damping(double_damping);
		cdpdiff =  dynamic_cast<cdp_processor *>(s_diffuser);
		if(!usertaps){
			ptaps = smalltaps;
			ntaps = sizeof(smalltaps) / sizeof(deltap);
		}
		break;
	case(DIFF_MEDIUM):
		printf("rmverb: using medium room model\n");
		m_diffuser = new medium_diffuser(0.108,&data1,&data2,feedback,fb_lpfreq,sr);
		m_diffuser->set_damping(double_damping);
		cdpdiff =  dynamic_cast<cdp_processor *>(m_diffuser);
		if(!usertaps){
			ptaps = mediumtaps;
			ntaps = sizeof(mediumtaps) / sizeof(deltap);
		}
		//ptaps = longtaps;
		//ntaps = sizeof(longtaps) / sizeof(deltap);

		break;
	case(DIFF_LARGE):
		printf("using large room model\n");
		//fb_lpfreq = 2600.0; // Gardner fixed val
		l_diffuser = new large_diffuser(&data1,&data2,feedback,fb_lpfreq,sr);
		l_diffuser->set_damping(double_damping);
		cdpdiff =  dynamic_cast<cdp_processor *>(l_diffuser);
		if(!usertaps){
			ptaps = largetaps;
			ntaps = sizeof(largetaps) / sizeof(deltap);
		}
		break;
	default:
#ifdef _DEBUG
		assert(false);
#endif
		break;
	}

	if(!cdpdiff){
		puts("rmverb: no memory for reverberator\n");
		exit(1);
	}
	if(!cdpdiff->create()){
		puts("rmverb: no memory for reverberator buffers\n");
		exit(1);
	}


	/*********** GLOBAL COMPONENTS *************/

	if(lp_freq > 0.0)
		lp		= new onepole(lp_freq,sr,LOW_PASS);
	
	//if user prescribes a predelay, adjust all taptimes
	if(predelay >= 0.0){
		double current_delay = ptaps[0].pos;
		double adjust = predelay - current_delay;

		for(int i = 0; i < ntaps; i++)
			ptaps[i].pos += adjust;

	}
	else
		predelay = ptaps[0].pos;	//to report to user
	tdelay = new tapdelay(ptaps,ntaps,sr);

	if(!tdelay->create()) {
		fputs("rmverb: no memory for early reflections\n",stderr);
		return 1;
	}

	//max_gain = tdelay->get_maxgain();
	//if(max_gain > 1.0)
	//	tdelay->set_gain(max_gain * early_gain);


	// final allpass chain: 0.02,0.014,0.009,0.006
	ap1_l		= new allpassfilter(sr,to_prime(0.02,sr),-0.6,0);
	ap2_l		= new allpassfilter(sr,to_prime(0.014,sr),0.4,0);
//	ap3_l		= new allpassfilter(to_prime(0.009,sr),-0.6,0);
//	ap4_l		= new allpassfilter(to_prime(0.006,sr),0.4,0);
	if(!(ap1_l->create()
		&& ap2_l->create()
//		&& ap3_l->create()
//		&& ap4_l->create()
		)){
		fprintf(stderr,"rmverb: no memory for allpass chain\n");
		exit(1);
	}
	if(outchans >=2){
		// slightly different params to make stereo!
		// tune front pair precisely; further chans use formula
		ap1_r		= new allpassfilter(sr,to_prime(0.025,sr),-0.6,0);
		ap2_r		= new allpassfilter(sr,to_prime(0.0145,sr),0.4,0);
//		ap3_r		= new allpassfilter(to_prime(0.0095,sr),-0.6,0);
//		ap4_r		= new allpassfilter(to_prime(0.0065,sr),0.4,0);
		if(!(ap1_r->create()
			&& ap2_r->create()
//			&& ap3_r->create()
//			&& ap4_r->create()
			)){
			fprintf(stderr,"rmverb: no memory for allpass chain\n");
			exit(1);
		}
	}

	

	printf("using %ld early reflections: predelay = %.2lf msecs\n",ntaps,predelay * 1000.0);
    int clipfloats = 1;
    int minheader = 0;
	if(want_floats) {
		props.samptype =PSF_SAMP_IEEE_FLOAT;
        clipfloats = 0;        
    }
	props.chans = outchans;
    
	if((ofd = psf_sndCreate(argv[OUTFILE],&props,clipfloats,minheader,PSF_CREATE_RDWR)) < 0) {
        fprintf(stderr,"rmverb: cannot open output file %s\n",argv[OUTFILE]);
        return 1;
	}

/****
 																		|wet
 *****   in-> ---o-->pre_lp -->earlies-o--->diffuser-->*diffgain-->--+--*---+----out
				 |				       |							 |		|
				 |					   |>--------*-(earlies)->-------|		*--dry
				 |							     ^earlygain				    |
				 |__________________________________________________________|

 ****/
	printf("\ninfile chans = %ld, outfile chans = %ld",chans,outchans);
	printf("\n");
	long outframes = 0;
	long step = (long)(0.25 * sr);
    float insamps[2], outsamps[16];
	double l_out,r_out;
//    float  finsamp, foutsamp;
	for(;;){
		int rv;
		double l_ip,r_ip, out,l_direct,r_direct,mono_direct,earlies = 0.0f;
        
		//read mono/left
		if((rv = psf_sndReadFloatFrames(ifd,insamps,1)) < 0){
			fprintf(stderr,"rmverb: error reading file\n"); 
			exit(1);			
		}		
		if(rv==0) break;		// end of inputfile	- handle any trailertime
		l_ip = (double) insamps[0];
		//apply any conditioning to direct signal
		if(tc_l_lowcut)
			l_ip =  tc_l_lowcut->tick(l_ip);
		if(l_highcut)
			l_ip =  l_highcut->tick(l_ip);
		l_direct = l_ip;
		mono_direct = l_direct;
		r_direct = l_direct;
		//handle R channnel if active
		if(chans==2){
            r_ip = (double) insamps[1];
			//apply any conditioning to direct signal			
			if(tc_r_lowcut)
				r_ip =  tc_r_lowcut->tick(r_ip);
			if(r_highcut)
				r_ip =  r_highcut->tick(r_ip);
			r_direct = r_ip;
			if(want_mono)	//input merged sig to reverbreator
				mono_direct = (l_direct + r_direct) * 0.5;			 
		}
		// mono_direct = (mixed) mono input to  reverb		
		//possibly also filter it...
		if(lp)
			mono_direct =  lp->tick(mono_direct);					//input lowpass
		earlies = tdelay->tick(mono_direct);				//early reflections	
		//send (filtered) mono output from reverberator

		// TODO: find formula for diffgain...
		out = earlies + (cdpdiff->tick(earlies + mono_direct) * diff_gain * 0.707);		 //the dense reverberation		
		// output:: use 2 allpasses per chan to generate stereo reverb, wider diffusion
		l_out = wet_gain *  ap2_l->tick(ap1_l->tick(out));

		// old method - 1 allpass per channel
		//l_out = chan_ap[0]->tick(out) * wet_gain;

		if(outchans == 1)			
			l_out += (mono_direct * dry_gain);
		else 
			l_out += l_direct * dry_gain;
        outsamps[0] = (float) l_out;
		
		if(outchans>=2){
			r_out = wet_gain *  ap2_r->tick(ap1_r->tick(out));			
			// old version
			//r_out = chan_ap[1]->tick(out) * wet_gain;
			r_out += r_direct * dry_gain;
			outsamps[1] = (float) r_out;
		}

		//now any further channels; reduced level of direct sig
		for(i=2;i < outchans; i++){
            out = earlies + (cdpdiff->tick(earlies + (mono_direct * 0.3)) * diff_gain * 0.707);
			l_out = wet_gain * chan_ap[i]->tick(out);
            outsamps[i] = (float) l_out;
			
		}
        if(psf_sndWriteFloatFrames(ofd,outsamps,1) != 1){
            fprintf(stderr,"rmverb: error writing output file\n"); 
			return 1;
        }
		outframes++;
		if((outframes % step) == 0)
			fprintf(stdout,"%.2f\r",(double)outframes/(double)sr);
	}
	
	int trtime  = (int)( trailertime * sr);
	for(i = 0; i < 	trtime; i++){		
		double tr_l_ip,tr_r_ip = 0.0f,tr_out,tr_l_direct,tr_r_direct,
			tr_mono_direct,tr_earlies = 0.0f,tr_l_out,tr_r_out;
		//need last active samps from input filter(s)
		tr_l_ip = 0.0;
		if(tc_l_lowcut)
			tr_l_ip =  tc_l_lowcut->tick(tr_l_ip);
		if(l_highcut)
			tr_l_ip =  l_highcut->tick(tr_l_ip);
		tr_l_direct = tr_l_ip;
		tr_mono_direct = tr_l_direct;
		tr_r_direct = tr_l_direct;
		//handle R channnel if active
		if(chans==2){
			tr_r_ip = 0.0;	   // inout is zero, except if using input filters
			// get any samples from input conditioning filters
			if(tc_r_lowcut)
				tr_r_ip =  tc_r_lowcut->tick(0.0);						   
			if(r_highcut)
				tr_r_ip =  r_highcut->tick(tr_r_ip);
			tr_r_direct = tr_r_ip;
			if(want_mono)
				tr_mono_direct = (tr_l_direct + tr_r_direct) *0.5f;			
		}

		// mono_direct = (mixed) mono input to  reverb
		//possibly also filter it...
		if(lp)
			tr_mono_direct =  lp->tick(tr_mono_direct);		
		tr_earlies = tdelay->tick(tr_mono_direct);
		//send (filtered) mono output from reverberator
		tr_out = tr_earlies + (cdpdiff->tick(tr_earlies + tr_mono_direct) * diff_gain * 0.707);		
		// output:: use 2 allpasses to generate stereo reverb, further diffusion

		// new version
		tr_l_out = wet_gain *  ap2_l->tick(ap1_l->tick(tr_out));
		// old version		
		//tr_l_out = chan_ap[0]->tick(tr_out) * wet_gain;

		if(want_mono)
			tr_l_out += (tr_mono_direct * dry_gain);
		else			
			tr_l_out += tr_l_direct * dry_gain;

        outsamps[0] = (float) tr_l_out;
		
		if(outchans>=2){						
			tr_r_out = wet_gain * ap2_r->tick(ap1_r->tick(tr_out));
			// old version:
			//tr_r_out = chan_ap[1]->tick(tr_out) * wet_gain;

			tr_r_out += tr_r_direct * dry_gain;	   // still need this cos of input filters
            outsamps[1] = (float) tr_r_out;
		}
		for(int j = 2; j < outchans; j++){			
			tr_r_out = chan_ap[j]->tick(tr_out) * wet_gain;			
            outsamps[j] = (float) tr_r_out;
			
		}
        if(psf_sndWriteFloatFrames(ofd,outsamps,1) != 1){
            fprintf(stderr,"rmverb: error writing output file\n"); 
			return 1;
        }
		outframes++;
		if((outframes % step)==0)
			//inform(step,sr);
			fprintf(stdout,"%.2f\r",(double)outframes/(double)sr);
	}
	fprintf(stdout,"%.2f\n",(double)outframes/(double)sr);
	//stopwatch(0);
    
	peaks = (PSF_CHPEAK *) malloc(sizeof(PSF_CHPEAK) * outchans);
    if(peaks && psf_sndReadPeaks(ofd,peaks,NULL)) {
        printf("\nPEAK data:\n");
        for(i=0;i < outchans;i++)
            printf("Channel %d: %.4f at frame %u: %.4lf secs\n",i+1,
                   peaks[i].val,peaks[i].pos, (double) peaks[i].pos / (double)props.srate);
	}
	else {
        fputs("rmverb: warning: unable to read PEAK data\n",stderr);
    }
	
	psf_sndClose(ifd);
	
	if(psf_sndClose(ofd) < 0)
		fprintf(stderr,"rmverb: error closing outfile\n");
    printf("\n");
    
    psf_finish();
	free(peaks);
	
	delete tdelay;
	
	if(usertaps)
		delete [] ptaps;
	//can I call delete on base-class ptr?
	if(s_diffuser)
		delete s_diffuser;
	if(m_diffuser)
		delete m_diffuser;
	if(l_diffuser)
		delete l_diffuser;
	if(lp)
		delete lp;
	if(outchans > 1){
		for(i=0;i < outchans; i++)
			delete chan_ap[i];
		delete [] chan_ap;
	}	
	delete tc_l_lowcut;	
	delete tc_r_lowcut;	
	delete l_highcut;
	delete r_highcut;
	return 0;
}


void
usage(void)
{
	fprintf(stderr,"\nrmverb: Multi-channel reverb with room simulation\n\tVersion 1.3 Release 5 CDP 1998,1999,2011");
	fprintf(stderr,
	"\nusage:\nrmverb [flags] infile outfile rmsize rgain mix fback absorb lpfreq trtime"
	"\nflags    : any or all of the following"	
	"\n   -cN   : create outfile with N channels (1 <= N <= 16 : default =2)"
	"\n   -d    : apply double lowpass damping (internal to nested allpasses)"
	"\n            (see <absorb>. NB: reduces reverb time: "
	"\n                  increase feedback to compensate)"
	"\n   -eFILE: read early reflections data from breakpoint textfile FILE"
	"\n   -f    : force floating-point output (default: infile format)"	
	"\n   -HN   : apply 12dB Highcut filter with cutoff freq N Hz) to infile"
	"\n   -LN   : apply 12dB Lowcut filter with cutoff freq N Hz to infile"
	"\n           (NB: currently fixed at 12dB/oct - could be made variable)\n"
	"\n   -pN   : force reverb predelay to N msecs (shifts early reflections)"
	"\nrmsize   : 1,2 or 3: small, medium or large"
	"\nrgain    : set level of dense reverb (0.0 < egain <= 1.0)"
	"\nmix      : dry/wet balance (source and reverb). 1.0<-- mix -->=0.0"
	"\nfback    : reverb feedback level: controls decay time. 0.0 <= fback <= 1.0"
	"\nabsorb   : cutoff freq (Hz) of internal lowpass filters (models air absorption)\n"
	"                (typ: 2500 for large room --- 4200 for small room "
	"\nlpfreq   : lowpass filter cutoff freq(Hz) applied at input to reverb"	
	"\n           NB: to disable either filter (absorb,lpfreq), use the value 0"
	"\ntrtime   : trailer time added to outfile for reverb tail (secs)\n");	
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