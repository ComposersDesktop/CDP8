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



/*************************** BOUNCE_OFF_FILE_END_IF_NECESSARY 
**************************/

int bounce_off_file_end_if_necessary(int *here,int thisdur,int chans,int 
splicelen,dataptr dz)
{
	int diff, initial_diff, needed;
	initial_diff = abs(dz->insams[0] - *here);	/* Ensure enough room to read 
final segment and splice */
	if(EVEN(chans) && ODD(initial_diff)) {
		sprintf(errstr,"Stereo anomaly at file end: 
bounce_off_file_end_if_necessary()\n");
		return(PROGRAM_ERROR);
	}
	if((needed = thisdur + splicelen) > dz->sampbuf[0]) {
		sprintf(errstr,"Segement duration exceeds size of sound\n");
		return(DATA_ERROR);
	}
	while((diff = dz->insams[0] - *here) < needed) {
		*here -= initial_diff; 		   			/* Bounce off end of buffer if ness */
		if(*here < 0) {					   		/* But, if it bounces off BOTH ends !! */
			*here = (dz->insams[0] - needed)/dz->infile->channels;
			*here = (int)round(*here * drand48()) * dz->infile->channels;
/*
			sprintf(errstr,"Anomaly in bounce_off_file_end_if_necessary()\n");
			return(PROGRAM_ERROR);
*/
		}
	}
	return(FINISHED);
}

