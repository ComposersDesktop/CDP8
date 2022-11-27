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
 
/* chorder :  reorder channels in m/c file */
/* Jan 2010: corrected behaviour with out of range order chars */
/* July 2010 removed spurious premature call to usage() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <portsf.h>

/* set size of multi-channel frame-buffer */
#define NFRAMES (1024)

/* TODO define program argument list, excluding flags */
enum {ARG_PROGNAME,ARG_INFILE,ARG_OUTFILE,ARG_ORDERSTRING,ARG_NARGS};
#define N_BFORMATS (10)
static const int bformats[N_BFORMATS] = {2,3,4,5,6,7,8,9,11,16};

#ifdef unix
/* in portsf.lib */
extern int stricmp(const char *a, const char *b);
#endif


void usage(void)
{
    fprintf(stderr,"\nCDP MCTOOLS: CHORDER V1.2 (c) RWD,CDP 2009,2010\n");	
    fprintf(stderr,
            "Reorder soundfile channels.\n"
            "Usage:  chorder infile outfile orderstring\n"
            "  orderstring = any combination of characters a-z inclusive.\n"
            "  Infile channels are mapped in order as a=1,b=2...z=26\n"
            "  (For example: channels in a 4-channel file are represented by the\n"
            "    characters abcd; any other character is an error).\n"
            "  Characters must be lower case, and may be used more than once.\n"
            "  Duplicate characters duplicate the corresponding input channel.\n"
            "  The zero character (0) may be used to set a silent channel.\n"
            "  A maximum of 26 channels is supported for both input and output.\n"
            "  NB: infile (WAVEX) speaker positions are discarded.\n"
            "  The .amb extension is supported for the outfile.\n\n"
            );
}


int main(int argc, char* argv[])
{
	PSF_PROPS inprops,outprops;									
	long framesread;	
	/* init all dynamic resources to default states */
	unsigned int i;
    int halfsec;
	unsigned int framepos;
    long outsize;
    int ifd = -1,ofd = -1;
	int error = 0;
	PSF_CHPEAK* peaks = NULL;	
	psf_format outformat =  PSF_FMT_UNKNOWN;
	unsigned long nframes = 1;
	float* inframe = NULL;
	float* outframe = NULL;
    float* orderptr[26];
    char* argstring = NULL;
    unsigned int rootchar = 'a';
    unsigned int maxchar = 'z';
    unsigned int  nchars,nzeros = 0;
    unsigned int max_inchar;
    MYLONG peaktime;
	
    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("1.2.\n");
        return 0;
    }

	/* process any optional flags: remove this block if none used! */
	if(argc > 1){
		char flag;
		while(argv[1][0] == '-') {
			flag = argv[1][1];
			switch(flag){
			/*TODO: handle any  flag arguments here */
			case('\0'):
				printf("Error: missing flag name\n");
				return 1;
			default:
				break;
			}
			argc--;
			argv++;
		}
	}

	if(argc < ARG_NARGS){
		printf("insufficient arguments.\n");
		usage();
		return 1;
	}
    
    /* initial check of charstring */
    argstring = argv[ARG_ORDERSTRING];
    nchars = strlen(argstring);
    if(nchars > 26) {
        printf("error: order string too long.\n");
        return 1;
    }
    
	/*  always startup portsf */
	if(psf_init()){
		printf("unable to start portsf\n");
		return 1;
	}
																							
	ifd = psf_sndOpen(argv[ARG_INFILE],&inprops,0);															  
	if(ifd < 0){
		printf("Error: unable to open infile %s\n",argv[ARG_INFILE]);
		error++;
		goto exit;
	}
    outsize = psf_sndSize(ifd);
	if(outsize <= 0){
		fprintf(stderr,"chorder: infile is empty!\n");
		psf_sndClose(ifd);
		return 1;
	}
    inframe = (float*) malloc(nframes * inprops.chans * sizeof(float));
	if(inframe==NULL){
		puts("No memory!\n");
		error++;
		goto exit;
	}
    /* final validate and parse of charstring */
    max_inchar = rootchar;
    for(i=0;i < nchars;i++){
        unsigned int thischar = argstring[i];
//        printf("reading char %c (%d)\n",thischar,thischar);
        unsigned int chindex;
        if(thischar != '0' && (thischar < rootchar || thischar > maxchar)){
            printf("illegal character in order string: %c\n",thischar);
            goto exit;
        }
        if(thischar =='0'){
//            printf("setting channel %d to zero.\n",i);
            orderptr[i] = NULL;
            nzeros++;
        }
        else{
            if(thischar > max_inchar)
                max_inchar = thischar;
            chindex = thischar - rootchar;
            orderptr[i] = &inframe[chindex];
        }
    }
    if(nzeros==nchars)
        printf("Warning: order string is all zeros - a silent file will be made!\n");
    else{    
        /* count inclusively! */
        if(inprops.chans < (max_inchar - rootchar + 1)){
            printf("File has %d channels; order string defines non-existent channels.\n",inprops.chans);
            printf("For this file, maximum character is %c\n",rootchar+inprops.chans-1);
            goto exit;
        }
    }
	/* check file extension of outfile name, so we use correct output file format*/
	outformat = psf_getFormatExt(argv[ARG_OUTFILE]);
	if(outformat == PSF_FMT_UNKNOWN){
		printf("outfile name %s has unsupported extension.\n"
			"Use any of .wav, .aiff, .aif, .afc, .aifc, .amb\n",argv[ARG_OUTFILE]);
		error++;
		goto exit;
	}
	inprops.format = outformat;
	outprops = inprops;
	outprops.chans = nchars;
    if(!is_legalsize(outsize,&outprops)){
        fprintf(stderr,"error: outfile size exceeds capacity of format.\n");
        return 1;
    }
    /* any speaker assignment etc invalidated! */
    if(outformat == PSF_STDWAVE) {
        outprops.chformat = MC_STD;
        outprops.format =  PSF_WAVE_EX;
    }
    
    if(outformat==PSF_WAVE_EX){
        int matched = 0;
        for(i=0;i < N_BFORMATS;i++)	{
            if(inprops.chans == bformats[i]){
                matched = 1;
                break;
            }
        }
        if(!matched){
            printf("WARNING: No B-format definition for %d-channel file.\n",outprops.chans);
        }
        outprops.chformat = MC_BFMT;
        outprops.format =  PSF_WAVE_EX;
    }
    
    outframe = malloc(sizeof(float) * nchars);
    
	peaks  =  (PSF_CHPEAK*) malloc(outprops.chans * sizeof(PSF_CHPEAK));
	if(peaks == NULL){
		puts("No memory!\n");
		error++;
		goto exit;
	}
	ofd = psf_sndCreate(argv[ARG_OUTFILE],&outprops,0,0,PSF_CREATE_RDWR);
	if(ofd < 0){
		printf("Error: unable to create outfile %s\n",argv[ARG_OUTFILE]);
		error++;
		goto exit;
	}
	
    halfsec = inprops.srate / 2;
	framepos = 0;
	printf("processing....\n");									
	
	while ((framesread = psf_sndReadFloatFrames(ifd,inframe,1)) > 0){
        float val;
        for(i=0;i < nchars;i++){
            if(orderptr[i] == NULL)
                val = 0.0f;
            else
                val = *orderptr[i];
            outframe[i] = val;
        }
		

		if(psf_sndWriteFloatFrames(ofd,outframe,1) != 1){
			printf("Error writing to outfile\n");
			error++;
			break;
		}
        if((framepos % halfsec) == 0){
			printf("%.2lf secs\r",(double) framepos / (double) outprops.srate);
            fflush(stdout);
        }
		framepos++;
	}

	if(framesread < 0)	{
		printf("Error reading infile. Outfile is incomplete.\n");
		error++;
	}
	printf("\n%.4lf secs\nWritten %d frames to %s\n",(double)framepos / (double) outprops.srate,framepos,argv[ARG_OUTFILE]);
		
	if(psf_sndReadPeaks( ofd,peaks,&peaktime)){
		printf("PEAK values:\n");
		for(i=0; i < outprops.chans; i++){
            double val, dbval;
            val = (double) peaks[i].val;
            if(val > 0.0){
                dbval = 20.0 * log10(val);
                printf("CH %d: %.6f (%.2lfdB) at frame %u:\t%.4f secs\n",i,
                       val,dbval,(unsigned int) peaks[i].pos,(double)peaks[i].pos / (double) outprops.srate);
            }
            else{
                printf("CH %d: %.6f (-infdB) at frame %u:\t%.4f secs\n",i,
                       val,(unsigned int) peaks[i].pos,(double)peaks[i].pos / (double) outprops.srate); 
            }
        }
	}
	printf("\n");
exit:	 	
	if(ifd >= 0)
		psf_sndClose(ifd);
	if(ofd >= 0)
		psf_sndClose(ofd);
	if(inframe)
		free(inframe);
    if(outframe)
        free(outframe);
	if(peaks)
		free(peaks);

	psf_finish();
	return error;
}
