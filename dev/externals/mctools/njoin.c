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
 
/* njoin.c: concantenate files with optional spacing */
/* 12 Dec 2006 v 0.7: fixed bug in read_filelist: trap leading spaces on line */
/* OCT 2009: v1.0 support tilde-prefixed path under unix */
/* Jan 2010 v1.0.1 fixed bug processing lots of files and running out of portsf slots */
/* Nov 2013 recognise MC_SURR_6_1 */
/* Mar 2021  fix small file format type mismatch error (line 543 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <string.h>
#include <assert.h>
#ifdef unix
#include <glob.h>
#endif
#include "portsf.h"


#ifdef unix
/* in portsf.lib */
extern int stricmp(const char *a, const char *b);
#endif

#define F_MAXLEN (1024) 
/* For CERN will need to increase this a lot! */
#define MAXFILES (512)


enum {ARG_PROGNAME, ARG_FLIST, ARG_NARGS};
enum {ARG_OUTFILE = ARG_NARGS};

int read_filelist(FILE* ifp, char* p_flist[]);
void cleanup(int ifd, int ofd,char* flist[], int nfiles);

#ifdef unix
char* CreatePathByExpandingTildePath(char* path)
{
    glob_t globbuf;
    char **v;
    char *expandedPath = NULL, *result = NULL;
    
    assert(path != NULL);
    
    if (glob(path, GLOB_TILDE, NULL, &globbuf) == 0) //success
    {
        v = globbuf.gl_pathv; //list of matched pathnames
        expandedPath = v[0]; //number of matched pathnames, gl_pathc == 1
        
        result = (char*)calloc( strlen(expandedPath) + 1,sizeof(char)); //the extra char is for the null-termination
        if(result)
            strncpy(result, expandedPath, strlen(expandedPath) + 1); //copy the null-termination as well
        
        globfree(&globbuf);
    }
    
    return result;
}
#endif

void
usage()
{
	fprintf(stderr,"\nCDP MCTOOLS: NJOIN v1.1.1 (c) RWD,CDP 2006,2010,2013,2021\n"
			"concatenate multiple files into a single file\n"
			"Usage: njoin [-sSECS | -SSECS][-cCUEFILE][-x] filelist.txt [outfile] \n"						
			"       filelist.txt: text file containing list of sfiles\n"
			"                     in order. One file per line. \n"
			"                     Channel spec (if present) must be the same,\n"
			"                     but files with no spec assumed compatible.\n"
			"       -cCUEFILE   : if outfile used, generate cue textfile as CUEFILE.\n"
			"       -sSECS      : separate files with silence of SECS seconds\n"
			"       -SSECS      : as above, but no silence before first file.\n"
			"                     Default: files are joined with no gap.\n"
            "      -x           : strict: allow only CD-compatible files:\n"
            "                     Must use sr=44100, minimum duration 4 secs.\n"
			"       NB: Files must match sample rate and number of channels,\n"
			"        but can have different sample types.\n"
			"       Output sample format taken from file with highest precision.\n"
			"       If no outfile given: program scans files and prints report.\n"
#ifdef unix
            "       Unix systems:  ~/ notation for home dir supported for file paths.\n"
#endif
			);	
}

void cleanup(int ifd, int ofd,char* flist[], int nfiles)
{
	int i;
	for(i=0;i < nfiles; i++) {		
		if(flist[i])
			free(flist[i]);
	}
	if(ifd >=0)
		psf_sndClose(ifd);
	if(ofd >=0)
		psf_sndClose(ofd);	
}
/*SHORT8,SHORT16,FLOAT32,INT_32,INT2424,INT2432,INT2024,INT_MASKED*/
static int wordsize[] = {1,2,4,4,3,4,3};

/*return 0 for props2 same or less, 1 for props2 higher*/
int compare_precision(const PSF_PROPS* props1,const PSF_PROPS* props2)
{
	int retval = 0;
	switch(props1->samptype){
	case(PSF_SAMP_8):
		if(props2->samptype != props1->samptype)
			retval = 1;
		break;
	case(PSF_SAMP_16):
		if(props2->samptype > props1->samptype)
			retval = 1;
		break;
	case(PSF_SAMP_IEEE_FLOAT):
		/* only higher prec is 32bit int */
		if(props2->samptype== PSF_SAMP_32)
			retval = 1;
		break;
	case(PSF_SAMP_32):
		/* nothing higher than this!*/
		break;
	case(PSF_SAMP_24):
	//case(INT2432): // NB illegal for almost all formats!
	//case(INT2024):
		if(props2->samptype == PSF_SAMP_IEEE_FLOAT || props2->samptype == PSF_SAMP_32)
			retval = 1;
		break;
	default:  
		break;
	}
	return retval;
}

const char* stype_as_string(const PSF_PROPS* props)
{
	const char* msg;
	switch(props->samptype){
	case(PSF_SAMP_8):
		msg = "8-bit";
		break;
	case(PSF_SAMP_16):
		msg = "16-bit";
		break;
	case(PSF_SAMP_IEEE_FLOAT):
		msg = "32-bit floats";
		break;
	case(PSF_SAMP_32):
		msg = "32-bit integer";
		break;
	case(PSF_SAMP_24):
		msg = "24-bit";
		break;
	default:
		msg = "unknown WAVE_EX sample size!";
		break;
	}
	return msg;
}
//STDWAVE,MC_STD,MC_MONO,MC_STEREO,MC_QUAD,MC_LCRS,MC_BFMT,MC_DOLBY_5_1,MC_WAVE_EX 
const char* chformat_as_string(const PSF_PROPS* props)
{
	const char* msg;
	switch(props->chformat){
	case MC_STD:
		msg = "Generic WAVE-EX";
		break;
	case MC_MONO:
		msg = "WAVE_EX Mono";
		break;
	case MC_STEREO:
		msg = "WAVE_EX Stereo";
		break;
	case MC_QUAD:
		msg = "WAVE_EX Quad";
		break;
	case MC_LCRS:
		msg = "WAVE_EX LCRS Surround";
		break;
	case MC_BFMT:
		msg = "WAVE_EX B-Format";
		break;
	case MC_DOLBY_5_1:
		msg = "WAVE_EX Dolby 5.1";
		break;
	case MC_SURR_6_1:
	    msg =  "6.1 surround";
	    break;
    case MC_SURR_5_0:
        msg = "5.0 surround";
        break;
    case MC_SURR_7_1:
        msg = "7.1 Surround";
        break;
    case MC_CUBE:
        msg = "Cube Surround";
        break;
	case MC_WAVE_EX:
		msg = "WAVE-EX Custom Multi-Channel";
		break;
    default:  // STDWAVE
        msg = "Standard soundfile";
        break;
	}

	return msg;
}

int read_filelist(FILE* ifp, char* p_flist[])
{
	char buf[F_MAXLEN];
	long len;
	int nfiles = 0;
#ifdef _DEBUG
	assert(ifp);
	assert(p_flist);
#endif
	if(ifp==NULL || p_flist == NULL)
		return -1;
	while (fgets(buf,F_MAXLEN-1,ifp)){
		char* pbuf = buf;
		while(*pbuf == ' ')
			pbuf++;
		len = strlen(pbuf);
		if(len > 1){	// line has at least a eol byte
			p_flist[nfiles] = malloc(len+1);
			strcpy(p_flist[nfiles],pbuf);
			if(p_flist[nfiles][len-1] == 0x0A)
				p_flist[nfiles][len-1] = '\0';
			nfiles++;
		}
		if(feof(ifp))
			break;
		if(ferror(ifp))
			return -1;
	}	
	return nfiles;
}
 
int strict_check(PSF_PROPS* props,unsigned long dur)
{
    int ret = 0;
    unsigned long mindur = 4 * 44100;
    
    if(props->srate==44100 && dur >= mindur && props->chans==2)
        ret = 1;
    return ret;
}

int main(int argc, char* argv[])
{
	int i = 0,j,ofd = -1;
	int ifd = -1;
	char* flist[MAXFILES];
	char* cuefilename = NULL;
	FILE* cuefp = NULL;
	int num_infiles = 0;
	float *inframe = NULL;
    float*space_frame = NULL;
	PSF_PROPS inprops, thisinprops,outprops;
	PSF_CHPEAK *fpeaks = NULL;
	double space_secs = 0.0;
	long space_frames = 0;
    long thisdur = 0;
	double totaldur = 0.0;
	FILE* fp = NULL;
	int formatsrc = 0;
	unsigned int max_datachunk = 0xFFFFFFFFU - 1024U;   /* check Ok for PEAK chunk */
	double maxdur;
	double blockdur = 0.25;  /* match buffersize to srate, so we get tidy upodate msgs */
	long buflen,block_frames;
	unsigned long written;
	int error = 0;
	int do_process = 1;
	int have_s = 0, have_S = 0;
    int strict  =0;
    int strict_failures = 0;
    char* fname;

    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("1.1.0\n");
        return 0;
    }
    
	if(argc < ARG_NARGS){
		fprintf(stderr,"njoin: insufficient arguemnts\n");
		usage();
		return 1;
	}
	while(argv[1][0] =='-'){	
		switch(argv[1][1]){
		case('c'):
			if(argv[1][2]== '\0'){
				fprintf(stderr,"-c flag needs filename.\n");
				return 1;
			}
			cuefilename = &(argv[1][2]);
			break;
		case('s'):
			if(have_S){
				fprintf(stderr,"njoin: cannot have both -s and -S.\n");
				return 1;
			}
			space_secs = atof(&argv[1][2]);
			if(space_secs < 0.0){
				fprintf(stderr,"njoin: -tSECS cannot be negative!\n");
				return 1;
			}
			have_s = 1;
			break;
		case('S'):
			if(have_s){
				fprintf(stderr,"njoin: cannot have both -s and -S.\n");
				return 1;
			}
			space_secs = atof(&argv[1][2]);
			if(space_secs < 0.0){
				fprintf(stderr,"njoin: -tSECS cannot be negative!\n");
				return 1;
			}
			have_S = 1;
			break;
        case 'x':
            strict = 1;
            break;
		default:
			fprintf(stderr,"\nnjoin: illegal flag option %s",argv[1]);
			exit(1);
		}
		argc--; argv++;
	}

	if(argc < ARG_NARGS){
		fprintf(stderr,"njoin: insufficient arguemnts\n");
		usage();
		return 1;
	}
	
	if(argc==ARG_NARGS)
		do_process = 0;
	/********** READ filelist *********/

	fp = fopen(argv[ARG_FLIST],"r");
	if(fp==NULL){
		fprintf(stderr,"njoin: Unable to open input file %s\n",argv[ARG_FLIST]);
		return 1;
	}
	
	memset(flist,0,MAXFILES * sizeof(char*));
	num_infiles = read_filelist(fp, flist);
	if(num_infiles < 0){
		fprintf(stderr,"njoin: file error reading filelist %s\n",argv[ARG_FLIST]);
		fclose(fp);
		return 1;
	}
	if(num_infiles ==0){
		fprintf(stderr,"njoin: filelist is empty!\n");
		fclose(fp);
		return 1;
	}
	if(num_infiles ==1){
		fprintf(stderr,"njoin: only one file listed - nothing to do!\n");
		fclose(fp);
		return 1;
	}
	fclose(fp); fp = NULL;

#ifdef _DEBUG
	fprintf(stderr, "file list contains %d files: \n",num_infiles);	
	for(i=0;i < num_infiles; i++)
		fprintf(stderr,"%s\n",flist[i]);
#endif

	/********* open and check all soundfiles ***********/
	fprintf(stderr,"checking files...\n");
	
	if(psf_init()){
		fprintf(stderr,"njoin: startup failure.\n");
		return 1;
	}
    i = 0;
#ifdef unix
    fname = CreatePathByExpandingTildePath(flist[i]);
    /* must free pointer later */
#else
    fname = flist[i];
#endif
	//open first infile and get properties
	ifd = psf_sndOpen(fname,&inprops,0);
	if(ifd < 0){
		fprintf(stderr,"unable to open infile %s.\n",fname);
		cleanup(ifd,ofd,flist,num_infiles);
		return 1;
	}
	thisdur = psf_sndSize(ifd);
    if(strict){
        if(!strict_check(&inprops,thisdur)){
            fprintf(stderr,"Strict: file %s is not CD-compatible.\n",fname);
            strict_failures++;
        }
    }
	if(thisdur==0){
		fprintf(stderr,"WARNING: file 1 empty: %s\n",fname);
	}
	else  {	
		totaldur += (double) thisdur / inprops.srate;
	}
	psf_sndClose(ifd);
#ifdef unix
    free(fname);
#endif
	ifd = -1;
	/* scan firther files, find max precision */
	/* drop out if channel formats different */
	for(i=1; i <	num_infiles; i++){
#ifdef unix
        fname = CreatePathByExpandingTildePath(flist[i]);
        /* must free pointer later */
#else
        fname = flist[i];
#endif
        
        
		ifd = psf_sndOpen(fname,&thisinprops,0);
		if(ifd < 0){
			fprintf(stderr,"unable to open infile %s.\n",fname);
			cleanup(ifd,ofd,flist,num_infiles);
			exit(1);
		}
        thisdur = psf_sndSize(ifd);
        if(strict){
            if(!strict_check(&thisinprops,thisdur)){
                fprintf(stderr,"Strict: file %s is not CD-compatible.\n",fname);
                strict_failures++;
            }
        }
		if(inprops.chans != thisinprops.chans){
			fprintf(stderr,"njoin: channel mismatch in file %s",fname);
			cleanup(ifd,ofd,flist,num_infiles);
#ifdef unix
            free(fname);
#endif
			return 1;
		}
		if(inprops.srate != thisinprops.srate){
			fprintf(stderr,"njoin: sample rate mismatch in file %s",fname);
			cleanup(ifd,ofd,flist,num_infiles);
#ifdef unix
            free(fname);
#endif
			return 1;
		}
		/* allow old multichannel files to be compatible with everything! */
		if(! (inprops.chformat==(psf_channelformat)PSF_STDWAVE || thisinprops.chformat==(psf_channelformat)PSF_STDWAVE)){
			if(inprops.chformat != thisinprops.chformat){
				fprintf(stderr,"njoin: channel format mismatch in file %s",fname);
				cleanup(ifd,ofd,flist,num_infiles);
#ifdef unix
                free(fname);
#endif
				return 1;
			}
		}
		else {
			/* one file is generic: promote format if possible*/
			if(thisinprops.chformat > inprops.chformat)
				inprops.chformat = thisinprops.chformat;
		}

		/* compare wordlength precision */
		if(compare_precision(&inprops,&thisinprops)) {
			inprops = thisinprops;
			formatsrc = i;
		}
		thisdur = psf_sndSize(ifd);
		if(thisdur==0){
			fprintf(stderr,"WARNING: file %d empty: %s\n",i+1,fname);
		}
		else  {	
			totaldur += (double) thisdur / thisinprops.srate;
		}
		psf_sndClose(ifd);
		ifd = -1;
#ifdef unix
        free(fname);
#endif
	}
    if(strict_failures){ 
        fprintf(stderr,"Strict: %d files are CD-incompatible. Exiting.\n",strict_failures);
        return 1;
    }
    
	fprintf(stderr, "output format taken from file %d:\n\t%s\n",formatsrc+1,flist[formatsrc]);
	fprintf(stderr, "sample type: %s\n",stype_as_string(&inprops));  
	fprintf(stderr,"channel format: %s\n",chformat_as_string(&inprops));

	maxdur = (double)(max_datachunk/ inprops.chans / wordsize[inprops.samptype]) / inprops.srate;
	/*TODO: make sure we allow for size of PEAK chunk */
	fprintf(stderr, "Max duration available for this format: %f secs.\n",maxdur);
	if(have_S)
		fprintf(stderr,"Total outfile length including spacing: %f secs\n", totaldur + (num_infiles-1) * space_secs);
	else
		fprintf(stderr,"Total outfile length including spacing: %f secs\n", totaldur + (num_infiles) * space_secs);

	if(do_process == 0){
		if(totaldur > maxdur)
			fprintf(stderr, "Error: total duration exceeds capacity of file format.\n");
		cleanup(ifd,ofd,flist,num_infiles);
		return 0;
	}

	if(totaldur > maxdur){
		fprintf(stderr, "Sorry! Total duration exceeds capacity of file format.\nProcess aborted.\n");
		cleanup(ifd,ofd,flist,num_infiles);
		return 1;
	}
    
	/* if here, OK! We can make the file */
    printf("processing files...\n");
	outprops = inprops;
	/* try to make a legal wave file! */
	if((outprops.chans > 2 || outprops.samptype > PSF_SAMP_IEEE_FLOAT)
		 && (outprops.format==PSF_STDWAVE))
        outprops.chformat = /* PSF_WAVE_EX */ MC_STD;       //RWD 10:03:21
    block_frames = (long)(blockdur * outprops.srate);
	buflen = block_frames * outprops.chans;
	inframe = malloc(buflen * sizeof(float));
	if(inframe==NULL){
		puts("No memory!\n");
		cleanup(ifd,ofd,flist,num_infiles);
		return 1;
	}
	//setup PEAK data
	fpeaks = (PSF_CHPEAK *) calloc(outprops.chans,sizeof(PSF_CHPEAK));
	if(fpeaks==NULL){
		puts("njoin: error: no memory for internal PEAK buffer\n");
		cleanup(ifd,ofd,flist,num_infiles);
		return 1;
	}

	if(cuefilename){
		cuefp = fopen(cuefilename, "w");
		if(cuefp == NULL){
			fprintf(stderr, "WARNING: unable to create cue file %s.\n",cuefilename);
			cuefilename = NULL;
		}
	}

    space_frame = calloc(sizeof(float),outprops.chans);
    
	ofd = psf_sndCreate(argv[ARG_OUTFILE],&outprops,0,0,PSF_CREATE_RDWR);
	if(ofd < 0){
		fprintf(stderr,"njoin: Cannot create outfile %s\n",argv[ARG_OUTFILE]);
		cleanup(ifd,ofd,flist,num_infiles);
		return 1;
	}
	fprintf(stderr, "generating outfile...\n");
	written = 0;
	if(cuefp) {
		//fprintf(cuefp,"FILE %s WAVE\n",snd_getfilename(ofd));
        fprintf(cuefp,"FILE %s WAVE\n",argv[ARG_OUTFILE]);
		fprintf(cuefp,"\tTRACK 01 AUDIO\n");
				fprintf(cuefp,"\t\tINDEX 01 00:00:00\n");
	}
	space_frames = (long) (space_secs * outprops.srate + 0.5);
	/* add leading space ? */
	if(have_s && space_frames > 0){
		for(j=0; j < space_frames; j++) {
			if(psf_sndWriteFloatFrames(ofd,space_frame,1) < 0){
				fprintf(stderr,"njoin: error writing outfile\n");
				cleanup(ifd,ofd,flist,num_infiles);
				free(inframe);
				free(fpeaks);
				return 1;
			}
		}
		written += space_frames;
	}
	for(i=0; i < num_infiles; i++){
		long got, put;
        PSF_PROPS fprops; // dummy - not needed 
#ifdef unix
        fname = CreatePathByExpandingTildePath(flist[i]);
        /* must free pointer later */
        /*RWD TODO: may fail with null return if file does not exist */
#else
        
        fname = flist[i];
#endif        
		ifd = psf_sndOpen(fname,&fprops,0);
		if(ifd < 0){
			fprintf(stderr,"unable to open infile %s.\n",fname);
			error++;
			break;
		} 
        
		do {			
			got = psf_sndReadFloatFrames(ifd,inframe,block_frames);
			if(got < 0){
				fprintf(stderr,"njoin: error reading file %s\n",fname);
				error++;
				break;
			}
			
			put = psf_sndWriteFloatFrames(ofd,inframe,got);
			if(put != got){
				fprintf(stderr,"njoin: error writing outfile\n");
				error++;
				break;
			}
			written += got;
			fprintf(stderr,"%.2f\r",(double) written / outprops.srate);
		} while (got > 0);
		if(error)
			break;
		
		/* add space */
		if(i < num_infiles - 1){
			/* update cue file */
			if(cuefp){
				double pos = (double)(psf_sndTell(ofd)) / outprops.srate;
				int mins = (int)(pos / 60.0);
				int secs = (int)(pos - mins);
				int frames = (int) ((pos - (60.0 * mins + secs)) * 75.0);

				fprintf(cuefp,"\tTRACK %.2d AUDIO\n",i+2);
				fprintf(cuefp,"\t\tINDEX 01 %.2d:%.2d:%.2d\n",mins,secs,frames); 
			}
			
			for(j=0; j < space_frames ; j++) {
				if(psf_sndWriteFloatFrames(ofd,space_frame,1) < 0){
					fprintf(stderr,"njoin: error writing outfile\n");
					error++;
					break;
				}
			}
		}
		if(error)
			break;
		written += space_frames;
#ifdef unix
        free(fname);
#endif
        /*RWD Jan 2010 MUST close the file or we may run out of portsf slots! */
        psf_sndClose(ifd);
        ifd = -1;
	}

	if(error){
		fprintf(stderr,"Error: Outfile incomplete.\n");
//		sndunlink(ofd);
	}
	else {
		fprintf(stderr, "Done.\nWritten %ld frames to outfile\n",written);
		if(psf_sndReadPeaks(ofd,fpeaks,NULL) > 0){
            long i;
            double peaktime;
            printf("PEAK information:\n");	
			for(i=0;i < inprops.chans;i++){
                double val = fpeaks[i].val;
				peaktime = (double) fpeaks[i].pos / (double) inprops.srate;
                if(val==0.0)
                    printf("CH %ld:\t%.4f           at %.4f secs\n", i+1, val,peaktime);
                else
                    printf("CH %ld:\t%.4f (%.2fdB) at %.4f secs\n", i+1, val, 20.0*log10(val),peaktime);
			}
        }
	}
	if(inframe)
		free(inframe);
	if(fpeaks)
		free(fpeaks);
	if(cuefp)
		fclose(cuefp);
	cleanup(ifd,ofd,flist,num_infiles);
	psf_finish();
	return 0;
}
