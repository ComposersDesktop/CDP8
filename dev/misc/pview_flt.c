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



/**************************************************************************
 *
 * Convert snd data to text format suitable for Pview in Sound Loom
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osbind.h>
#include <math.h>
#include <float.h>
#include <sfsys.h>

/*RWD: changed all fprintf tests to < 0 */
/*Aug10-2005: fixed buflen bug from TW report */


#define SHORTMONOSCALE		256.0		/* Scales shorts in range 0-32768 to display in range 0 128 */
#define SHORTSTEREOSCALE	512.0		/* Scales shorts in range 0-32768 to display in stereorange 0 64 */
#define FLOATMONOSCALE		128.0		/* Scales shorts in range 0-32768 to display in range 0 128 */
#define FLOATSTEREOSCALE	64.0		/* Scales shorts in range 0-32768 to display in stereorange 0 64 */
#define VIEW_WIDTH			900.0		/* width of sloom display */

#define F_SECSIZE (512.0)
/* 	Functions	*/

static	int		tidy_up(int,unsigned int);
static  int		open_in(char*);
static	int		get_big_buf(void);
static	int		make_textfile(char *filename);


/*short *bigbuf;	*/	/* buffer used to read samples from soundfile 	*/
float *bigbuf;

size_t buflen;				/* buffer length in bytes			*/
int  ifd;					/* input soundfile descriptor		*/
int srate;					/* sampling rate of input			*/
int  channels;				/* number of channels of input		*/
int startsamp, endsamp;
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
	unsigned int start = hz1000();
	char filename[200];

	if((argc==2) && strcmp(argv[1],"--version")==0) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(argc!=4)  {
		fprintf(stdout,"ERROR: Wrong number of arguments.\n");
		fflush(stdout);
		return tidy_up(2,start);
	}
	if(sscanf(argv[2],"%d",&startsamp)!=1) {
		fprintf(stdout,"ERROR: cannot read start sample number.\n");
		fflush(stdout);
		return tidy_up(2,start);
	}
	if(sscanf(argv[3],"%d",&endsamp)!=1) {
		fprintf(stdout,"ERROR: cannot read end sample number.\n");
		fflush(stdout);
		return tidy_up(2,start);
	}
	/* open input file */
	if(open_in(argv[1]) < 0)
		return tidy_up(2,start);

	strcpy(filename,argv[1]);

	/* get biggest buffer */
	if(get_big_buf() == 0)
		return tidy_up(1,start);
		
	/* max soundfiles */
	if(make_textfile(filename)<0)
		return tidy_up(1,start);

	/* tidy up */
	return tidy_up(0,start);
}

/**************************** OPEN_IN ****************************
 *
 * opens input soundfile and gets header 
 */

int open_in(char *name)	/* opens input soundfile and gets header */
{
	

	if( (ifd = sndopenEx(name,0,CDP_OPEN_RDONLY)) < 0 )	{
		fprintf(stdout,"INFO: Cannot open file: %s\n\t",name);
		fflush(stdout);
		return(-1);
	}

	/*	get sampling rate	*/

	if(sndgetprop(ifd,"sample rate", (char *)&srate,sizeof(int)) < 0) {
		srate = 44100;
		fprintf(stdout,"WARNING: Cannot get sampling rate of %s : Default of 44100 assumed\n",name);
		fflush(stdout);
	}

	/*	get channels 		*/

	if(sndgetprop(ifd, "channels", (char*)&channels, sizeof(int)) < 0) {
		channels = 2;
		fprintf(stdout,"WARNING: Cannot get channels of %s Default of STEREO assumed\n",name);
		fflush(stdout);
	}
	return 0;
}

/**************************** GET_BIG_BUF ****************************
 *
 * allocates memory for the biggest possible buffer 
 */

int	get_big_buf()
{
	size_t i;

	buflen = (size_t) Malloc(-1) - sizeof(float);	/* allow for alignment */

	/* if less than one sector available */
	if( buflen < (F_SECSIZE * sizeof(float)) || (bigbuf=(float*)Malloc(buflen+sizeof(float))) == NULL) {
		fprintf(stdout,"ERROR: Failed to allocate float buffer.\n");
		fflush(stdout);
		return 0;
	}
	i = ((size_t)bigbuf+sizeof(float)-1)/sizeof(float)*sizeof(float);	/* align bigbuf to word boundary */
	bigbuf = (float*)i;
	buflen = (size_t)((buflen/F_SECSIZE)*F_SECSIZE);
	buflen /= sizeof(float);
	/*Aug10-2005 */
	buflen /= channels;
	buflen *= channels;
	return 1;
}

/**************************** MAKE TEXTFILE *****************************/

int make_textfile(char *filename)
{
	int sampsgot, n, m, zoomfact;
	double gpsample0, gpsample1;
	int total_samps_got;
	int gpcnt, sampout, sampouttrue;

	/*RWD*/
	int linecount = 0;

	/* check header first */
	FILE *fp;

	/* read and find maximum */

	if(channels > 2) {
		fprintf(stdout,"ERROR: Process only works with mono or stereo files\n");
		fflush(stdout);
		return -1;
	}
	if((fp = fopen("cdptest00.txt","w"))==NULL) {
		fprintf(stdout, "ERROR: Failed to open the Sound Loom display file 'cdptest00.txt'\n");
		fflush(stdout);
		return -1;
	}
	zoomfact = max(1,(int)ceil((double)(endsamp - startsamp)/VIEW_WIDTH));
	if(fprintf(fp,"%d\n",zoomfact) < 1) {
		fclose(fp);
		fprintf(stdout, "ERROR: Failed to complete data write to Sound Loom display file 'cdptest00.txt'\n");
		fflush(stdout);
		return -1;
	}
	gpcnt = 0;
	gpsample0 = 0.0;
	gpsample1 = 0.0;
	total_samps_got = 0;
	while((sampsgot = fgetfbufEx(bigbuf,(int) buflen,ifd,1)) > 0 ) {
		switch(channels) {
		case(1):
			for(n=0;n<sampsgot;n++) {
				total_samps_got++;
				if(total_samps_got <= startsamp)
					continue;
				else if(total_samps_got > endsamp)
					break;
				if(zoomfact == 1)
					gpsample0 = bigbuf[n];
				else
					gpsample0 += fabs(bigbuf[n]);
				gpcnt++;
				if(gpcnt >= zoomfact) {
					gpsample0 /= (double)zoomfact;
					sampout = (short)round(gpsample0 * FLOATMONOSCALE); 
					if(fprintf(fp,"%lf %d\n",gpsample0,sampout) < 0) {
						fclose(fp);
						fprintf(stdout, "ERROR: Failed to complete data write to Sound Loom display file 'cdptest00.txt'\n");
						fflush(stdout);
						return -1;
					}
					gpsample0 = 0.0;
					gpcnt = 0;
				}
			}
			if(gpcnt > 0) {
				gpsample0 /= (double)gpcnt;
				sampout = (short)round(gpsample0 * FLOATMONOSCALE); 
					if(fprintf(fp,"%lf %d\n",gpsample0,sampout) < 0) {
					fclose(fp);
					fprintf(stdout, "ERROR: Failed to complete data write to Sound Loom display file 'cdptest00.txt'\n");
					fflush(stdout);
					return -1;
				}
			}
			break;
		case(2):
			for(n=0,m=1;m<sampsgot;n+=2,m+=2) {		 /*RWD test was using n */
				total_samps_got++;
				if(total_samps_got <= startsamp)
					continue;
				else if(total_samps_got > endsamp)
					break;
				if(zoomfact == 1) {
					gpsample0 = bigbuf[n];
					gpsample1 = bigbuf[m];
				} else {
					gpsample0 += fabs(bigbuf[n]);
					gpsample1 += fabs(bigbuf[m]);
				}
				gpcnt++;
				if(gpcnt >= zoomfact) {
					gpsample0 /= (double)zoomfact;
					gpsample1 /= (double)zoomfact;
					sampout = (short)round(gpsample0 * FLOATSTEREOSCALE); 
					
					linecount++;
					if(fprintf(fp,"%lf %d\n",gpsample0,sampout) < 0) {
						fclose(fp);
						fprintf(stdout, "ERROR: Failed to complete data write to Sound Loom display file 'cdptest00.txt'\n");
						fflush(stdout);
						return -1;
					}
					sampout = (short)round(gpsample1 * FLOATSTEREOSCALE);
					
					linecount++;
					if(fprintf(fp,"%lf %d\n",gpsample1,sampout) < 0) {
						fclose(fp);
						fprintf(stdout, "ERROR: Failed to complete data write to Sound Loom display file 'cdptest00.txt'\n");
						fflush(stdout);
						return -1;
					}
					gpsample0 = 0.0;
					gpsample1 = 0.0;
					gpcnt = 0;
				}
			}
			if(gpcnt > 0) {
				gpsample0 /= (double)gpcnt;
				gpsample1 /= (double)gpcnt;
				sampout = (short)round(gpsample0 * FLOATSTEREOSCALE);
				
				linecount++;
				if(fprintf(fp,"%lf %d\n",gpsample0,sampout) < 0) {
					fclose(fp);
					fprintf(stdout, "ERROR: Failed to complete data write to Sound Loom display file 'cdptest00.txt'\n");
					fflush(stdout);
					return -1;
				}
				sampouttrue = (short)round(gpsample1);
				sampout = (short)round(gpsample1 * FLOATSTEREOSCALE);
				
				linecount++;
				if(fprintf(fp,"%lf %d\n",gpsample1,sampout) < 0) {
					fclose(fp);
					fprintf(stdout, "ERROR: Failed to complete data write to Sound Loom display file 'cdptest00.txt'\n");
					fflush(stdout);
					return -1;
				}
			}
			break;
		default:	  /*RWD*/
			break;
		}
/*		break; */
	}
	fclose(fp);
	return(0);
}

/**************************** TIDY_UP ****************************
 *
 * Exit, freeing buffers and closing files where necessary.
 */

int tidy_up(int where,unsigned int start)
{
	switch(where) {
	case 0:
		Mfree(bigbuf);
	case 1:
		sndcloseEx(ifd);
	case 2:
		sffinish();
	default:
		break;
	}
	while(!(hz1000() - start))
		;
	return(1);
}
	
