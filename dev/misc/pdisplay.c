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



/* PDISPLAY
 *
 * Open pitch data file (we already know the properties) 
 * and send pitch data to output stream
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
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
	size_t buflen,n;
	int samps_read;
	float *bigfbuf;
	int ifd;

	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(argc != 2) {
		fprintf(stdout,"ERROR: Wrong number of arguments: pdisplay sndfilename.\n");
		fflush(stdout);
		return 1;
	}
	/* initialise SFSYS	*/
	if( sflinit("pdisplay") < 0 ) {
		fprintf(stdout,"ERROR: Cannot initialise soundfile system.\n");
		fflush(stdout);
		return 1;
	}

	buflen = (size_t) Malloc(-1);

	if((bigfbuf=(float*)Malloc(buflen+sizeof(float))) == NULL) {
		fprintf(stdout,"ERROR: Failed to allocate float buffer.\n");
		fflush(stdout);
//		sffinish();
		return 1;
	}
	n = ((size_t)bigfbuf+sizeof(float)-1)/sizeof(float) * sizeof(float);	/* align bigbuf to word boundary */
	bigfbuf = (float*)n;
	buflen /= sizeof(float); 

	/* open input file */
	if( (ifd = sndopenEx(argv[1],0,CDP_OPEN_RDONLY)) < 0 )	{
		fprintf(stdout,"INFO: Cannot open file: %s\n\t",argv[1]);
		fflush(stdout);
		Mfree(bigfbuf);
//		sffinish();
		return 1;
	}

    while((samps_read = fgetfbufEx(bigfbuf,(int) buflen,ifd,0)) > 0) {
		for(n = 0; n < samps_read; n++) {
			fprintf(stdout,"INFO: %lf\n",bigfbuf[n]);
			fflush(stdout);
		}			
	}
	Mfree(bigfbuf);
	sndcloseEx(ifd);
//	sffinish();
	return 1;
}
