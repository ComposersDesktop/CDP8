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



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sfsys.h>
#include <string.h>

/* RWD March 2012 changed sndsize to sndsizeEx */
#define	FAILED		(-1)
#define SUCCEEDED	(0)

static int readfhead(int ifd);
const char* cdp_version = "7.1.0";

int main(int argc, char *argv[])
{
	int ifd;
	FILE *fp;
	int specenvcnt, k, len;
	char *filename, out[64], *p, *q;
	float *buf;
	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(argc!=2) {
		fprintf(stdout,"ERROR: Insufficient params:: vuform formantfile.\n");
		fflush(stdout);
		return(FAILED);
	}
	if((ifd = sndopenEx(argv[1],0,CDP_OPEN_RDONLY)) < 0) {
		fprintf(stdout,"ERROR: Failure to open file %s for input.\n",argv[1]);
		fflush(stdout);
		return(FAILED);
	}
	len = strlen(argv[1]);
	if((filename = (char *)malloc((len+1) * sizeof(char)))==NULL) {
		fprintf(stdout,"ERROR: Failure to allocate memory 1.\n");
		fflush(stdout);
		return(FAILED);
	}
	strcpy(filename,argv[1]);
	p = filename;
	q = filename + len;
	while(p < q) {
		if(*p == '.')
			break;
		p++;
	}
	p--;
	*p++ = '1';
	*p++ = '.';
	*p++ = 't';
	*p++ = 'x';
	*p++ = 't';
	k = sndsizeEx(ifd);
	if(k <= 0) {
		fprintf(stdout,"ERROR: No data in file\n");
		fflush(stdout);
		return(FAILED);
	}
	if((specenvcnt = readfhead(ifd)) < 0)
		return(FAILED);
	specenvcnt *= 3;
	if(k < specenvcnt) {
		fprintf(stdout,"ERROR: Too little data in file\n");
		fflush(stdout);
		return(FAILED);
	}
	if((buf = (float *)malloc(specenvcnt * sizeof(float)))==NULL) {
		fprintf(stdout,"ERROR: Failure to allocate memory 2.\n");
		fflush(stdout);
		return(FAILED);
	}
	if(fgetfbufEx(buf,specenvcnt,ifd,0) < 0) {
		fprintf(stdout,"ERROR: Can't read samples from file.\n");
		fflush(stdout);
		return(FAILED);
	}
	sndcloseEx(ifd);
	if((fp = fopen(filename,"w"))==NULL) {
		fprintf(stdout,"ERROR: Can't open outputfile\n");
		fflush(stdout);
		return(FAILED);
	}
	specenvcnt /= 3;
	sprintf(out,"%d\n",specenvcnt);
	fputs(out,fp);
	for(k = specenvcnt;k < specenvcnt * 3;k++) {
		if(k % specenvcnt == 0)
			fputs("\n",fp);
		sprintf(out,"%.12lf\n",buf[k]);
		fputs(out,fp);
	}
	fclose(fp);
	return(SUCCEEDED);
}


int readfhead(int ifd)
{
	int isf;
	SFPROPS props = {0};
	if(!snd_headread(ifd,&props)) {
		fprintf(stdout,"ERROR: Failure to read properties\n");
		fflush(stdout);
		return(-1);
	}
	if(sndgetprop(ifd,"is a formant file",(char *) &isf,sizeof(int)) < 0) {
		fprintf(stdout,"ERROR: Not a formant file.\n");
		fflush(stdout);
		return(-1);
	}
	return props.specenvcnt;
}

