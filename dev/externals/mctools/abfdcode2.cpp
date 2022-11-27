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

//abfdcode2.cpp  : decode to std wavex speaker positions
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern "C"{
#include <sfsys.h>

}

typedef struct abf_samp {
	float W;
	float X;
	float Y;
	float Z;	
} ABFSAMPLE;

int main(int argc,char *argv[])
{
	int ifd, ofd;
	long outchans = 4;
	long outsize;
	char *sfname;
	bool write_wavex = false;
	ABFSAMPLE abfsample;
	float frame[4];
	SFPROPS props;
	CHPEAK peaks[4];
	long inchans;

	if(argc < 3){
		printf("\nCDP MCTOOLS: ABFDCODE2 v 1.0: RWD,CDP 2009\n"
				"Horizontal B Format Decoder\n"
				"usage: abfdcode2 infile outfile layout\n"
				"       (wavex layouts require .wav extension)\n"
				
				);
		return 1;
	}
	while(argv[1][0] =='-'){		
		switch(argv[1][1]){
		case('x'):
			write_wavex = true;
			break;
		default:
			fprintf(stderr,"abfdecode: error: illegal flag option %s\n",argv[1]);
			return 1;
		}
		argc--; argv++;
	}
	if(argc < 3){
		fprintf(stderr,"CDP MCTOOLS: ABFDCODE.EXE: CDP 1999,2005\nHorizontal B Format Decoder\nusage: abfdcode [-x] infile outfile\n");
		return 1;
	}
	if(sflinit("abfdcode")){
		fprintf(stderr,"unable to initialize sfsys\n");
		return 1;
	}
	
	sfname = argv[2];
	
	ifd = sndopenEx(argv[1],0,CDP_OPEN_RDONLY);
	if(ifd < 0){
		fprintf(stderr,"unable toopen infile %s\n",argv[1]);
		return 1;
	}

	if(!snd_headread(ifd,&props)){
		fprintf(stderr,"unable to read infile header data\n");
		sndcloseEx(ifd);
		return 1;
	}
	if(!props.type==wt_wave){
		fprintf(stderr,"infile is not a soundfile\n");
		sndcloseEx(ifd);
		return 1;
	}
	inchans = props.chans;
	if(!(inchans == 3 || inchans == 4)){
		fprintf(stderr,"Sorry: infile is not first-order\n");
		sndcloseEx(ifd);
		return 1;
	}

	outsize = sndsizeEx(ifd) / inchans;
	if(outsize <= 0){
		fprintf(stderr,"infile is empty!\n");
		sndcloseEx(ifd);
		return 1;
	}
	

	//srate = props.srate;
	//stype = props.samptype == FLOAT32 ? SAMP_FLOAT : SAMP_SHORT;
	props.chformat = STDWAVE;
	props.chans = 4;
	if(write_wavex){
		props.chformat = MC_QUAD;
		props.format = WAVE_EX;
	}
	//ofd = sndcreat_formatted(sfname,outsize * outchans,stype,outchans,srate,CDP_CREATE_NORMAL);
	ofd = sndcreat_ex(sfname,outsize * outchans,&props,SFILE_CDP,CDP_CREATE_NORMAL);
	if(ofd < 0){
		fprintf(stderr,"can't create outfile %s : %s\n",sfname,sferrstr());
		//delete p_iosc;
		//delete p_sintable;
		sndcloseEx(ifd);
		return 1;
	}
	peaks[0].value = peaks[1].value = peaks[2].value = peaks[3].value = 0.0f;
	peaks[0].position = peaks[1].position = peaks[2].position = peaks[3].position = 0;

	int got,quartersec = props.srate / 4;
	unsigned int framepos = 0;
	printf("\ndoing b-format decoding:\n");
	while((got = fgetfbufEx((float *) &abfsample,inchans,ifd,0))==inchans){
		int i;
		//this_samp	= 0.5 * p_iosc->tick(1000);
		float aw = abfsample.W;
		float ax = abfsample.X * 0.707f;
		float ay = abfsample.Y * 0.707f;
		frame[0] = 0.3333f * (aw + ax + ay);
		frame[1] = 0.3333f * (aw + ax - ay);
		frame[2] = 0.3333f * (aw - ax + ay);
		frame[3] = 0.3333f * (aw - ax - ay);

		if(0 > fputfbufEx(frame,outchans,ofd)){
			fprintf(stderr,"error writing sample block  %ld\n",got * outchans);
			sndcloseEx(ifd);
			sndcloseEx(ofd);
			return 1;
		}
		for(i=0;i < 4;i++){
			float val;
			val= (float)fabs(frame[i]);
			if(val > peaks[i].value) {
				peaks[i].value = val;
				peaks[i].position = framepos;
			}
		}
		if(framepos % quartersec==0)
			printf("%.2lf secs\r",(double)framepos / (double) props.srate);
		framepos++;
	}

	if(got != 0){
		fprintf(stderr,"warning: not all data was read\n");
	}
	printf("\n%.4lf secs\nWritten %d quad frames to %s\n",(double)framepos / (double) props.srate,framepos,sfname);
	printf("PEAK values:\n");
	printf("CH 1: %.4f at frame %ld:\t%.4f secs\n",peaks[0].value,peaks[0].position,(double)peaks[0].position / (double) props.srate);
	printf("CH 2: %.4f at frame %ld:\t%.4f secs\n",peaks[1].value,peaks[1].position,(double)peaks[1].position / (double) props.srate);
	printf("CH 3: %.4f at frame %ld:\t%.4f secs\n",peaks[2].value,peaks[2].position,(double)peaks[2].position / (double) props.srate);
	printf("CH 4: %.4f at frame %ld:\t%.4f secs\n",peaks[3].value,peaks[3].position,(double)peaks[3].position / (double) props.srate);
	sndputpeaks(ofd,4,peaks);
	sndcloseEx(ifd);
	sndcloseEx(ofd);
	sffinish();
	return 0;
}
