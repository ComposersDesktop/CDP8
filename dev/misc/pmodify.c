/*
 * Copyright (c) 1983-2013 Trevor Wishart and Composers Desktop Project Ltd
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



/* PMODIFY
 *
 * Open pitch text data file and write to pitch binary data file
 * Close file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osbind.h>
#include <math.h>
#include <float.h>
#include <float.h>
#include <sfsys.h>
#include <cdplib.h>

static int headwrite(int ofd,int origchans,int origstype,int origrate,float arate,int Mlen,int Dfac);
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
	int samps_written;
	size_t bufsize, buflen, n;
	float *bigfbuf;
	int ofd;
	int origchans, origstype, origrate;
	float arate;
	int Mlen, Dfac;
	int srate, chans;
	double val;

	FILE *fp;

	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(argc != 11) {
		fprintf(stdout,"ERROR: Incorrect call to program which writes the pitch data.\n");
		fflush(stdout);
		return 1;
	}
	/* initialise SFSYS	*/
	if( sflinit("pdisplay") < 0 ) {
		fprintf(stdout,"ERROR: Cannot initialise soundfile system.\n");
		fflush(stdout);
		return 1;
	}

	/* open input file */
	if((fp = fopen(argv[1],"r"))==NULL)	{
		fprintf(stdout,"INFO: Cannot open temporary data file: %s\n",argv[1]);
		fflush(stdout);
//		sffinish();
	}

//TW output is analysis file
//	if((ofd = sndcreat(argv[2], -1, SAMP_FLOAT)) < 0) {
//		fprintf(stdout,"ERROR: Can't create output file %s (It may already exist)\n", argv[2]);
//		fflush(stdout);
//		sffinish();
//		return 1;
 //	}
	if(sscanf(argv[3],"%d",&origchans)!=1) {
		fprintf(stdout,"ERROR: Cannot read original-channels data,\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	if(sscanf(argv[4],"%d",&origstype)!=1) {
		fprintf(stdout,"ERROR: Cannot read original-sample-type data,\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	if(sscanf(argv[5],"%d",&origrate)!=1) {
		fprintf(stdout,"ERROR: Cannot read original-sample-rate data,\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	if(sscanf(argv[6],"%f",&arate)!=1) {
		fprintf(stdout,"ERROR: Cannot read analysis-rate data,\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	if(sscanf(argv[7],"%d",&Mlen)!=1) {
		fprintf(stdout,"ERROR: Cannot read Mlen data,\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	if(sscanf(argv[8],"%d",&Dfac)!=1) {
		fprintf(stdout,"ERROR: Cannot read Decimation Factor data,\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	if(sscanf(argv[9],"%d",&srate)!=1) {
		fprintf(stdout,"ERROR: Cannot read sample-rate data,\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	if(sscanf(argv[10],"%d",&chans)!=1) {
		fprintf(stdout,"ERROR: Cannot read channel data,\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	if((ofd = sndcreat_formatted(argv[2], -1, SAMP_FLOAT,
		chans,srate,CDP_CREATE_NORMAL)) < 0) {
		fprintf(stdout,"ERROR: Cannot open output file\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	bufsize = (size_t) Malloc(-1);

	if((bigfbuf=(float*)Malloc(bufsize+sizeof(float))) == NULL) {
		fprintf(stdout,"ERROR: Failed to allocate float buffer.\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	n = ((size_t)bigfbuf+sizeof(float)-1)/sizeof(float) * sizeof(float);	/* align bigfbuf to word boundary */
	bigfbuf = (float*)n;
	buflen  = bufsize/sizeof(float);

	n = 0;
	while (fscanf(fp,"%lf",&val)==1) {
		bigfbuf[n++] = (float)val; 		
		if(n>=buflen) {
			if((samps_written = fputfbufEx(bigfbuf,(int) buflen,ofd))<0) {
				fprintf(stdout,"ERROR: Can't write to output soundfile: (is hard-disk full?).\n");
				fflush(stdout);
				Mfree(bigfbuf);
				sndcloseEx(ofd);
				return 1;
			}
			n = 0;
		}		
	}
	if(n > 0) {
		if((samps_written = fputfbufEx(bigfbuf,(int)n,ofd))<0) {
			fprintf(stdout,"ERROR: Can't write to output soundfile: (is hard-disk full?).\n");
			fflush(stdout);
			Mfree(bigfbuf);
			sndcloseEx(ofd);
			return 1;
		}
	}
	fclose(fp);
	if (!headwrite(ofd,origchans,origstype,origrate,arate,Mlen,Dfac)) {
		fprintf(stdout,"ERROR: Failed to write valid header to output file.\n");
		fflush(stdout);
		sndunlink(ofd);
	}
	Mfree(bigfbuf);
	sndcloseEx(ofd);
//	sffinish();
	return 1;
}

/***************************** HEADWRITE *******************************/

int headwrite(int ofd,int origchans,int origstype,int origrate,float arate,int Mlen,int Dfac)
{
	int property_marker = 1;

	if(sndputprop(ofd,"orig channels", (char *)&origchans, sizeof(int)) < 0){
		fprintf(stdout,"ERROR: Failure to write original channel data: headwrite()\n");
		fflush(stdout);
		return(0);
	}
	if(sndputprop(ofd,"original sampsize", (char *)&origstype, sizeof(int)) < 0){
		fprintf(stdout,"ERROR: Failure to write original sample size. headwrite()\n");
		fflush(stdout);
		return(0);
	}
	if(sndputprop(ofd,"original sample rate", (char *)&origrate, sizeof(int)) < 0){
		fprintf(stdout,"ERROR: Failure to write original sample rate. headwrite()\n");
		fflush(stdout);
		return(0);
	}
	if(sndputprop(ofd,"arate",(char *)&arate,sizeof(float)) < 0){
		fprintf(stdout,"ERROR: Failed to write analysis sample rate. headwrite()\n");
		fflush(stdout);
		return(0);
	}
	if(sndputprop(ofd,"analwinlen",(char *)&Mlen,sizeof(int)) < 0){
		fprintf(stdout,"ERROR: Failure to write analysis window length. headwrite()\n");
		fflush(stdout);
		return(0);
	}
	if(sndputprop(ofd,"decfactor",(char *)&Dfac,sizeof(int)) < 0){
		fprintf(stdout,"ERROR: Failure to write decimation factor. headwrite()\n");
		fflush(stdout);
		return(0);
	}
    if(sndputprop(ofd,"is a pitch file", (char *)&property_marker, sizeof(int)) < 0){
		fprintf(stdout,"ERROR: Failure to write pitch property: headwrite()\n");
		fflush(stdout);
		return(0);
    }
	return(1);
}
