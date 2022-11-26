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



/*floatsam version*/
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <cdpmain.h>
#include <house.h>
#include <modeno.h>
#include <logic.h>
#include <pnames.h>
#include <flags.h>
#include <sfsys.h>

#include <string.h>
/**
#include <filetype.h>
#include <limits.h>
#include <math.h>
**/

/* mainfuncs.c */
extern void dz2props(dataptr dz, SFPROPS* props);


/*********************************** DO_CHANNELS ****************************/
/* RWD NB changes to ensure outfile is created with correct chan count. */
/* need full set of oufile props in dz, so we NEVER use dz-infile to spec the outfile! */
/* TODO: support n_channels, e.g. for zero, channel etc */

int do_channels(dataptr dz)
{
	int exit_status;
	int chans = dz->infile->channels, start_chan = 0, end_chan = 0;
	double maxoutsum, maxinval, sum, normaliser = 0.0;
//TW REVISION Dec 2002
//	char *outfilename, *outfilenumber;
	char *outfilename;
	char prefix[] = "_c";
	int namelen;
	int prefixlen = strlen(prefix);
	int infilesize, outfilesize;
	int k, m, n, j;
	float *buf = dz->bigbuf;
	int not_zeroed = 0, zeroed = 0, totlen;

	switch(dz->mode) {
	case(HOUSE_CHANNEL):
		dz->iparam[CHAN_NO]--;	/* change to internal numbering */
		break;
	case(HOUSE_ZCHANNEL):
		dz->iparam[CHAN_NO]--;	/* change to internal numbering */
		zeroed     =  dz->iparam[CHAN_NO];
		not_zeroed = !dz->iparam[CHAN_NO];
		break;
	}
	switch(dz->mode) {
	case(HOUSE_CHANNEL):
	case(HOUSE_CHANNELS):
		dz->buflen *= chans;				/* read chan * (contiguous) buffers */
// FEB 2010 TW
//		if(!sloom) {
//			namelen = strlen(dz->wordstor[0]);
//			q = dz->wordstor[0];
//			r = dz->wordstor[0] + namelen;
//			p = r - 1;
//			while((*p != '\\') && (*p != '/') && (*p != ':')) {
//				p--	;
//				if(p < dz->wordstor[0])
//					break;
//			}
//			if(p > dz->wordstor[0]) {
//				p++;
//				while(p <= r)
//					*q++ = *p++;
//			}
//		}
		namelen   = strlen(dz->wordstor[0]);
		if(!sloom)
// FEB 2010 TW
			totlen = namelen + prefixlen + 20;
		else
// FEB 2010 TW
			totlen = namelen + 20;
		if((outfilename = (char *)malloc(totlen * sizeof(char)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for outfile name.\n");
			return(MEMORY_ERROR);
		}
		switch(dz->mode) {
		case(HOUSE_CHANNEL):
			dz->tempsize = dz->insams[0]/chans;	/* total outfile size */
			start_chan = dz->iparam[CHAN_NO];
			end_chan   = dz->iparam[CHAN_NO]+1;
			break;
		case(HOUSE_CHANNELS):
			dz->tempsize = dz->insams[0]/dz->infile->channels;
			start_chan = 0;
			end_chan   = dz->infile->channels;
			break;
		}	
		dz->process_type = EQUAL_SNDFILE;	/* allow sndfile(s) to be created */
		/*RWD: GRRR! */
		dz->infile->channels = MONO;		/* force outfile(s) to be mono    */
		infilesize  = dz->insams[0];
		outfilesize = dz->insams[0]/chans;
		for(k=start_chan,j=0;k<end_chan;k++,j++) {
			if(sndseekEx(dz->ifd[0],0,0)<0) {
				sprintf(errstr,"sndseek failed.\n");
				return(SYSTEM_ERROR);
			}
			dz->total_samps_written = 0;
			dz->samps_left = dz->insams[0];
			dz->total_samps_read = 0;	/* NB total_samps_read NOT reset */
			strcpy(outfilename,dz->wordstor[0]);
			if(!sloom) {
				insert_new_chars_at_filename_end(outfilename,prefix);
				insert_new_number_at_filename_end(outfilename,k+1,0);
			} else
				insert_new_number_at_filename_end(outfilename,j,1);
			dz->insams[0] = outfilesize;   /* creates smaller outfile */
			if((exit_status = create_sized_outfile(outfilename,dz))<0) {
				sprintf(errstr,"Cannot open output file %s\n", outfilename);
				dz->insams[0] = infilesize;
				free(outfilename);
				return(DATA_ERROR);
			}
			dz->insams[0] = infilesize;		/* restore true value */
			while(dz->samps_left) {
				if((exit_status = read_samps(buf,dz))<0) {
					free(outfilename);
					return(exit_status);
				}
				for(n=k,m=0;n<dz->ssampsread;n+=chans,m++)
					buf[m] = buf[n];
				if(dz->ssampsread > 0) {
					if((exit_status = write_exact_samps(buf,dz->ssampsread/chans,dz))<0) {
						free(outfilename);
						return(exit_status);
					}
				}
			}
			dz->outfiletype  = SNDFILE_OUT;			/* allows header to be written  */
			/* RWD: will eliminate this eventually */
			if((exit_status = headwrite(dz->ofd,dz))<0) {
				free(outfilename);
				return(exit_status);
			}
			if(k < end_chan - 1) {
				if((exit_status = reset_peak_finder(dz))<0)
					return(exit_status);
				if(sndcloseEx(dz->ofd) < 0) {
					fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
					fflush(stdout);
				}
				/*free(outfilename);*/ /*RWD 25:9:2001 used again! */
				dz->ofd = -1;
			}
		}
		break;
	case(HOUSE_ZCHANNEL):
	case(STOM):
	case(MTOS):
		switch(chans) {
		case(MONO): 
			switch(dz->mode) {
			case(HOUSE_ZCHANNEL):
				/*RWD*/
				dz->outfile->channels = 2;

				dz->tempsize = dz->insams[0] * 2;	break;	/* total output size */
			case(STOM):
				sprintf(errstr,"This file is already mono!!\n");	return(GOAL_FAILED);
			case(MTOS):
				/*RWD*/
				dz->outfile->channels = 2;

				dz->tempsize = dz->insams[0] * 2;	break;	/* total output size */
			
			}
			break;
		case(STEREO): 
			dz->buflen *= chans;	/* read chans * (contiguous) buffers */
			switch(dz->mode) {
			case(HOUSE_ZCHANNEL):				
				/*RWD*/
				dz->outfile->channels = 2;

				dz->tempsize = dz->insams[0];		break;	/* total output size */
			case(STOM):
				/*RWD*/
				dz->outfile->channels = 1;
				
				dz->tempsize = dz->insams[0]/2;		break;	/* total output size */
			case(MTOS): sprintf(errstr,"This file is already stereo!!\n");	return(GOAL_FAILED);
			}
			break;
		default:
			switch(dz->mode) {
			case(MTOS):
				sprintf(errstr,"This process does not work with multichannel files!!\n");	return(GOAL_FAILED);
				break;
			case(STOM):	// MULTICHAN APPLICATION FOR MAKING THUMBNAILS
				dz->buflen *= chans;	/* read chans * (contiguous) buffers */
				dz->outfile->channels = 1;
				break;
			case(HOUSE_ZCHANNEL):
				dz->buflen *= chans;	/* read chans * (contiguous) buffers */
				dz->outfile->channels = dz->infile->channels;
				break;
			}
			break;
		}
		/*RWD NOW, we can create the outfile! */
		/*RWD April 2005 need this for wave-ex */
		if(dz->outfile->channels > 2 || dz->infile->stype > SAMP_FLOAT){
			SFPROPS props, inprops;
			dz2props(dz,&props);			
			props.chans = dz->outfile->channels;
			props.srate = dz->infile->srate;
			/* RWD: always need to hack it one way or another!*/

			if(dz->ifd && dz->ifd[0] >=0) {
				if(snd_headread(dz->ifd[0], &inprops)){
					/* if we receive an old WAVE 24bit file, want to write new wave-ex one! */
					if(inprops.chformat == STDWAVE)
						props.chformat = MC_STD;
					else
						props.chformat = inprops.chformat;
				}
			}
#ifdef _DEBUG
			printf("DEBUG: writing WAVE_EX outfile\n");
#endif
			dz->ofd = sndcreat_ex(dz->outfilename,-1,&props,SFILE_CDP,CDP_CREATE_NORMAL); 
			if(dz->ofd < 0){
				sprintf(errstr,"Cannot open output file %s : %s\n", dz->outfilename,sferrstr());
				return(DATA_ERROR);
			}
		}
		else
		//create outfile here, now we have required format info	
			dz->ofd = sndcreat_formatted(dz->outfilename,-1,dz->infile->stype,dz->outfile->channels,
									dz->infile->srate,									
									CDP_CREATE_NORMAL);
//TW ADDED, for peak chunk
		dz->outchans = dz->outfile->channels;
		establish_peak_status(dz);
		if(dz->ofd < 0)
			return DATA_ERROR;


// MULTICHAN APPLICATION FOR MAKING THUMBNAILS -->
		if(dz->mode == STOM && chans > 2) {
			maxoutsum = 0.0;
			maxinval = 0.0;
			while(dz->samps_left) {
				if((exit_status = read_samps(buf,dz))<0) {
					return(exit_status);
				}
				for(n=0;n< dz->ssampsread;n+=chans) {
					sum = 0.0;
					for(m=0;m < chans;m++) {
						maxinval = max(maxinval,fabs(buf[n+m]));
						sum += buf[n+m];
					}
					maxoutsum = max(maxoutsum,fabs(sum));
				}
			}
			normaliser = maxinval/maxoutsum;
			sndseekEx(dz->ifd[0],0,0);
			reset_filedata_counters(dz);
		}
// <-- MULTICHAN APPLICATION FOR MAKING THUMBNAILS

		while(dz->samps_left) {
			if((exit_status = read_samps(buf,dz))<0) {
				return(exit_status);
			}
			if(dz->ssampsread > 0) {
				switch(chans) {
				case(MONO): 
					switch(dz->mode) {
					case(HOUSE_ZCHANNEL):
						for(n=dz->ssampsread-MONO,m=(dz->ssampsread*STEREO)-STEREO;n>=0;n--,m-=2) {
							buf[m+zeroed]     = (float)0;
							buf[m+not_zeroed] = buf[n];														   
						}
						if((exit_status = write_samps(buf,dz->ssampsread * STEREO,dz))<0)
							return(exit_status);
						break;
					case(MTOS):
						for(n=dz->ssampsread-MONO,m=(dz->ssampsread*STEREO)-STEREO,k=m+1;n>=0;n--,m-=2,k-=2) {
							buf[m] = buf[n];
							buf[k] = buf[n];
						}
						if((exit_status = write_samps(buf,dz->ssampsread * STEREO,dz))<0)
							return(exit_status);
						break;
					}		
					break;
				case(STEREO): 
					switch(dz->mode) {
					case(HOUSE_ZCHANNEL):
						for(n=dz->iparam[CHAN_NO];n< dz->ssampsread;n+=2)
							buf[n] = (float)0;
						if((exit_status = write_samps(buf,dz->ssampsread,dz))<0) {
							return(exit_status);
						}
						break;
					case(STOM):
						if(dz->vflag[CHAN_INVERT_PHASE]) {
							for(n=0,m=0;n< dz->ssampsread;n+=2,m++)
								buf[m] = (float)((buf[n] - buf[n+1])/2.0);
						} else {
							for(n=0,m=0;n< dz->ssampsread;n+=2,m++)
								buf[m] = (float)((buf[n] + buf[n+1])/2.0);
						}
						if((exit_status = write_samps(buf,dz->ssampsread/2,dz))<0) {
							return(exit_status);
						}
						break;
					}
					break;
				default:		// HOUSE_ZCHANNEL
					switch(dz->mode) {
					case(HOUSE_ZCHANNEL):
						for(n=dz->iparam[CHAN_NO];n< dz->ssampsread;n+=dz->infile->channels)
							buf[n] = (float)0;
						if((exit_status = write_samps(buf,dz->ssampsread,dz))<0) {
							return(exit_status);
						}
						break;
					case(STOM):	// MULTICHAN APPLICATION FOR MAKING THUMBNAILS
						for(n=0,m=0;n< dz->ssampsread;n+=chans,m++) {
							sum = 0.0;
							for(k=0;k < chans;k++)
								sum += buf[n+k];
							sum *= normaliser;
							buf[m] = (float)sum;
						}
						if((exit_status = write_samps(buf,dz->ssampsread/chans,dz))<0) {
							return(exit_status);
						}
						break;
					}
					break;

				}
			}
		}
		switch(dz->mode) {
		case(HOUSE_ZCHANNEL):	dz->infile->channels = dz->outfile->channels;	break;
		case(STOM):				dz->infile->channels = MONO;  	break;
		case(MTOS):			 	dz->infile->channels = STEREO;	break;
		}
		break;
	}
	return(FINISHED);
}
