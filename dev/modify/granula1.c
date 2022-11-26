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



/*
 *	ABOUT THIS PROCESS
 *
 *	There are (up to) 5 buffers invloved here.....
 *	GRS_SBUF into which the input data is read  from the file.
 *	GRS_BUF	 into which the input data is mixed down to mono, where necessary. Otherwise, initial reads go directly to this buffer.
 *		This buffer has an overflow sector of GRS_BUF_SMPXS samples.
 *		The 1st read fills the buffer and the overflow.
 *		After the first read these overflow samples are copied to the bufstart, and a new input buffer start is marked, overflow-samples into the GRS_BUF..
 *	GRS_IBUF is then the point at which further samples are read into the input buffer.
 *	GRS_LBUF is buffer into which grains are copied for processing to the output, (they are timestretched, pshifted or and spliced).
 *	GRS_OBUF is the buffer into which the finished grains are copied ready for output.
 *
 *	GRS_INCHANS is the number of channels in the input grains i.e. after they have (or have not) been mixed down to mono.
 *	GRS_OUTCHANS is the number of channels in the output, which can be
 *		Mono, where no spatial processing takes place, and the input is mono
 *		Multichannel, where spatial processing takes place and the input, is
 *			(a) Mono
 *			(b) A selected (mono) channel from a multichannel input.
 *			(c)	A mix down of a multichannel input into a mono source.
 *		Multichannel, where the input is multichannel, and no spatialisation is requested.
 */

/* floatsam version */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <arrays.h>
#include <filetype.h>
#include <modify.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <osbind.h>

#ifdef unix
#define round(x) lround((x))
#endif

#define SAUS_SBUF		(0)
#define SAUS_OBUF		(1)
#define	SAUS_BUF(x)		(((x)*2)+2)
#define	SAUS_IBUF(x)	(((x)*2)+3)

static int    read_samps_granula(int firsttime,dataptr dz);
static int    granulate(int *thissnd,long *aipc,long *aopc,float **iiiiptr,float **LLLLptr,
				double inv_sr,long *samptotal,float **maxwrite,int *nctr,dataptr dz);
static int    read_a_large_buf(dataptr dz);
static int    read_a_normal_buf_with_wraparound(dataptr dz);
static int    baktrak_granula(long samptotal,int absiicnt_per_chan,float **iiptr,dataptr dz);
static int    reset_granula(int resetskip,dataptr dz);
static int    make_grain(float *b,float **iiptr,float aamp,int gsize_per_chan,double *transpos,dataptr dz);
static int    make_stereo_grain(float *b,float **iiptr,float aamp,int gsize_per_chan,double *transpos,dataptr dz);
static int    write_grain(float **maxwrite,float **Fptr, float **FFptr,int gsize_per_chan,int *nctr,dataptr dz);
static int    write_stereo_grain(double rpos,float **maxwrite,float **Fptr,float **FFptr,
				int gsize_per_chan,int *nctr,dataptr dz);
static double dehole(double pos);
static int    write_samps_granula(long k,int *nctr,dataptr dz);
static void   do_splices(int gsize_per_chan,int bspl,int espl,dataptr dz);
static void   do_btab_splice(dataptr dz);
static void   do_bsplice(int gsize_per_chan,dataptr dz,int bspl);
static void   do_etab_splice(int gsize_per_chan,dataptr dz);
static void   do_esplice(int gsize_per_chan,dataptr dz,int espl);
static void   do_stereo_splices(int gsize_per_chan,int bspl,int espl,dataptr dz);
static void   do_stereo_btab_splice(dataptr dz);
static void   do_stereo_bsplice(int gsize_per_chan,dataptr dz,int bspl);
static void   do_stereo_etab_splice(int gsize_per_chan,dataptr dz);
static void   do_stereo_esplice(int gsize_per_chan,dataptr dz,int espl);
static float  interp_gval_with_amp(float *s,double flcnt_frac,int chans,float ampp);
static float  interp_gval(float *s,double flcnt_frac,int chans);
static int    set_instep(int ostep_per_chan,dataptr dz);
static int    set_outstep(int gsize_per_chan,dataptr dz);
static int    set_range(int absiicnt,dataptr dz);
static long   do_scatter(int ostep_per_chan,dataptr dz);
static int    set_ivalue(int flag,int paramno,int hparamno,int rangeno,dataptr dz);
static double set_dvalue(int flag,int paramno,int hparamno,int rangeno,dataptr dz);
static int    renormalise(int nctr,dataptr dz);

static int  read_samps_sausage(int firsttime,dataptr dz);
static int  read_a_specific_large_buf(int k,dataptr dz);
static int  read_a_specific_normal_buf_with_wraparound(int k,dataptr dz);
static int  baktrak_sausage(int thissnd,long samptotal,int absiicnt_per_chan,float **iiptr,dataptr dz);
static int  reset_sausage(int thissnd,int resetskip,dataptr dz);
static int  get_next_insnd(dataptr dz);
static void perm_sausage(int cnt,dataptr dz);
static void insert(int n,int t,int cnt_less_one,dataptr dz);
static void prefix(int n,int cnt_less_one,dataptr dz);
static void shuflup(int k,int cnt_less_one,dataptr dz);

static int grab_an_appropriate_block_of_sausage_memory(long *this_bloksize,long standard_block,int bufdivisor,dataptr dz);

#ifdef MULTICHAN

static int make_multichan_grain(float *b,float **iiptr,float aamp,int gsize_per_chan,double *transpos,dataptr dz);
static void	do_multichan_splices(int gsize_per_chan,int bspl,int espl,dataptr dz);
static void do_multichan_btab_splice(dataptr dz);
static void do_multichan_bsplice(int gsize_per_chan,dataptr dz,int bspl);
static void do_multichan_etab_splice(int gsize_per_chan,dataptr dz);
static void do_multichan_esplice(int gsize_per_chan,dataptr dz,int espl);
static int write_multichan_grain(double rpos,int chan, int chanb,float **maxwrite,float **Fptr,float **FFptr,int gsize_per_chan,int *nctr,dataptr dz);

#endif

/********************************* GRANULA_PROCESS *************************************/

int granula_process(dataptr dz)
{
	int    exit_status;
	int    firsttime = TRUE, thissnd = 0;
	int    nctr = 0;	 /* normalisation vals counter */
	long   absicnt_per_chan = 0, absocnt_per_chan = 0;
	float  *iptr;
	float   *Fptr = dz->fptr[GRS_LBUF];
 	long   vals_to_write, total_ssampsread;
	double sr = (double)dz->infile->srate;
	double inverse_sr = 1.0/sr;
	float   *maxwrite = dz->fptr[GRS_LBUF];	/* pointer to last sample created */
//TW AGREED DELETIONS

	if(sloom)
		dz->total_samps_read = 0;
	dz->itemcnt = 0;
	switch(dz->process) {
	case(SAUSAGE):
		if((exit_status = read_samps_sausage(firsttime,dz))<0)
			return(exit_status);
		iptr = dz->sampbuf[SAUS_BUF(thissnd)];
		break;
	case(BRASSAGE):
		if((exit_status = read_samps_granula(firsttime,dz))<0)
			return(exit_status);
		iptr = dz->sampbuf[GRS_BUF];
		break;
	}
	total_ssampsread = dz->ssampsread;	
	display_virtual_time(0L,dz);
	do {
		if((exit_status = granulate(&thissnd,&absicnt_per_chan,&absocnt_per_chan,&iptr,&Fptr,
									inverse_sr,&total_ssampsread,&maxwrite,&nctr,dz))<0)
			return(exit_status);
	} while(exit_status==CONTINUE);

	vals_to_write = maxwrite - dz->fptr[GRS_LBUF];
	while(vals_to_write > dz->iparam[GRS_LONGS_BUFLEN]) {					   
// TW REVISED July 2004
//		if((exit_status = write_samps_granula(dz->buflen,&nctr,dz))<0)
		if((exit_status = write_samps_granula(dz->iparam[GRS_LONGS_BUFLEN],&nctr,dz))<0)
			return(exit_status);
		memmove((char *)dz->fptr[GRS_LBUF],(char *)dz->fptr[GRS_LBUFEND],
			dz->iparam[GRS_LBUF_SMPXS] * sizeof(float));
		memset((char *)dz->fptr[GRS_LBUFMID],0,dz->iparam[GRS_LONGS_BUFLEN] * sizeof(float));
		vals_to_write -= dz->iparam[GRS_LONGS_BUFLEN];
	}
	if(vals_to_write > 0) {
		if((exit_status = write_samps_granula(vals_to_write,&nctr,dz))<0)
			return(exit_status);
	}
	if(dz->total_samps_written <= 0) {
		sprintf(errstr,"SOURCE POSSIBLY TOO SHORT FOR THIS OPTION: Try 'Full Monty'\n");
		return(GOAL_FAILED);
	}
	display_virtual_time(0,dz);
#ifdef MULTICHAN
	dz->infile->channels = dz->iparam[GRS_OUTCHANS];	// setup ONLY NOW for headwrite
#endif
	return renormalise(nctr,dz);
}

/******************************* READ_SAMPS_GRANULA *******************************/

int read_samps_granula(int firsttime,dataptr dz)
{
	int exit_status;
	if(firsttime) {
		if((exit_status = read_a_large_buf(dz))<0)
			return(exit_status);
	} else {
		if((exit_status = read_a_normal_buf_with_wraparound(dz))<0)
			return(exit_status);
	}
	if(firsttime)
		dz->iparam[SAMPS_IN_INBUF] = dz->ssampsread;
	else
		dz->iparam[SAMPS_IN_INBUF] = dz->ssampsread + dz->iparam[GRS_BUF_SMPXS];

	return(FINISHED);
}

/*  INPUT BUFFER :-		
 *
 *	|-----------BUFLEN-----------|
 *
 *	buf      ibuf		   bufend
 *	|_________|__________________|buf_smpxs|
 *					 			/
 *	|buf_smpxs|			   <<-COPY_________/
 *
 *		  	  |-----------BUFLEN-----------|
 */

/***************************** RENORMALISE ****************************
 *
 * (1)	Find smallest normalising factor S (creating gretest level reduction).
 * (2)	For each normalising factor N, find value which will bring it
 *	down to S. That is S/N. Reinsert these values in the normalising
 *	factor array.
 *	REnormalising with these factors, will ensure whole file is normalised
 *	using same factor (S), which is also the smallest factor required.
 * (3)	Seek to start of outfile.
 * (4)	Set infile pointer to same as outfile pointer, so that read_samps()
 *	now reads from OUTFILE.
 * (5)	Reset samps_read counter.
 * (6)	Size of buffers we read depends on whether we output a stereo
 *	file or a mono file.
 * (7) 	Reset normalisation-factor array pointer (m).
 *	While we still have a complete buffer to read..
 * (8)  Read samps into output buffer (as, if output is stereo, this
 *	may be larger than ibuf, and will accomodate the data).
 * (9)	Renormalise all values.
 * (10)	Increment pointer to normalisation factors.
 * (11)	Seek to start of current buffer, in file.
 * (12)	Overwrite data in file.
 * (13)	Re-seek to end of current buffer, in file.
 * (14)	Calcualte how many samps left over at end, and if any...
 * (16)	Read last (short) buffer.
 * (17)	Set buffer size to actual number of samps left.
 * (18)	Renormalise.
 * (19)	Seek to start of ACTUAL buffer in file.
 * (20)	Write the real number of samps left in buffer.
 * (21) Check the arithmetic.
 * (22) Restore pointer to original infile, so finish() works correctly.
 */
  
int renormalise(int nctr,dataptr dz)
{
	int exit_status;
	long n, m = 0;
	long total_samps_read = 0;
	long samp_total = dz->total_samps_written, samps_remaining, last_total_samps_read;
	double min_norm  = dz->parray[GRS_NORMFACT][0];	/* 1 */
	float  *s = NULL;
	double nf;
	long   cnt;

    /* RWD Nov 2 2010: hack to avoid crash! */
    SFPROPS inprops;
    int inchans = 0;
    if(snd_headread(dz->ifd[0],&inprops)){
        inchans = inprops.chans;
    }
    else {
        sprintf(errstr, "WARNING: Can't read infile properties for normalization.\n");
		return SYSTEM_ERROR;    
    }
    
//TW NEW MECHANISM writes renormalised vals from original file created (tempfile) into true outfile		
	if(sndcloseEx(dz->ifd[0]) < 0) {
		sprintf(errstr, "WARNING: Can't close input soundfile\n");
		return(SYSTEM_ERROR);
	}
	if(sndcloseEx(dz->ofd) < 0) {
		sprintf(errstr, "WARNING: Can't close output soundfile\n");
		return(SYSTEM_ERROR);
	}
	if((dz->ifd[0] = sndopenEx(dz->outfilename,0,CDP_OPEN_RDWR)) < 0) {	   /*RWD Nov 2003 need RDWR to enable sndunlink to work */
		sprintf(errstr,"Failure to reopen file %s for renormalisation.\n",dz->outfilename);
		return(SYSTEM_ERROR);
	}
	sndseekEx(dz->ifd[0],0,0);
//	if(!sloom)
//		dz->wordstor[0][strlen(dz->wordstor[0]) -9] = ENDOFSTR;
	if((exit_status = create_sized_outfile(dz->wordstor[0],dz))<0) {
		sprintf(errstr,"Failure to create file %s for renormalisation.\n",dz->wordstor[0]);
		return(exit_status);
	}
	if((exit_status = reset_peak_finder(dz))<0)
		return(exit_status);
	switch(dz->process) {
	case(BRASSAGE):	s = dz->sampbuf[GRS_OBUF];	break;
	case(SAUSAGE):	s = dz->sampbuf[SAUS_OBUF];	break;
	}
	dz->total_samps_written = 0;
	for(m=1;m<nctr;m++) {
		if((dz->parray[GRS_NORMFACT])[m] < min_norm)
			min_norm  = (dz->parray[GRS_NORMFACT])[m];
	}

	if(min_norm < 1.0) {
		sprintf(errstr,"Renormalising by %lf\n",min_norm);
		print_outmessage_flush(errstr);
		for(m=0;m<nctr;m++)								/* 2 */
			(dz->parray[GRS_NORMFACT])[m] = min_norm/(dz->parray[GRS_NORMFACT])[m];
	}
//TW new mechanism: lines not needed
	dz->total_samps_read = 0;						/* 5 */
	display_virtual_time(0L,dz);					/* works on dz->total_samps_read here, so param irrelevant */
/* RWD nov 2010 hack, part 2: do this multipLy only if infile is mono */
    if(inchans ==1)
        dz->buflen *= dz->iparam[GRS_OUTCHANS];			/* 6 */												
	m = 0;											/* 7 */
	cnt = dz->buflen;
	while(dz->total_samps_read + dz->buflen < samp_total) {
		if((exit_status = read_samps(s,dz))<0) {
			close_and_delete_tempfile(dz->outfilename,dz);
			return(exit_status);
		}											/* 8 */
		total_samps_read += dz->ssampsread;
		if(min_norm < 1.0) {
			nf  = (dz->parray[GRS_NORMFACT])[m];
			for(n=0;n<cnt;n++)							/* 9 */
				s[n] = /*round*/ (float)((double)s[n] * nf);
			m++;										/* 10 */
		}
		if((exit_status = write_samps(s,dz->buflen,dz))<0) {
			close_and_delete_tempfile(dz->outfilename,dz);
			return(exit_status);					/* 12 */
		}
	}												/* 14 */
	if((samps_remaining = samp_total - dz->total_samps_read)>0) {
		last_total_samps_read =  dz->total_samps_read;
		if((exit_status = read_samps(s,dz))<0) {
			close_and_delete_tempfile(dz->outfilename,dz);
			return(exit_status);					/* 16 */
		}
		dz->buflen = samps_remaining;	/* 17 */
		if(min_norm < 1.0) {
			nf  = (dz->parray[GRS_NORMFACT])[m];
			for(n=0;n<dz->buflen;n++)					/* 18 */
				s[n] = /*round*/(float) ((double)s[n] * nf);
		}
		if(samps_remaining > 0) {
			if((exit_status = write_samps(s,samps_remaining,dz))<0) {
				close_and_delete_tempfile(dz->outfilename,dz);
				return(exit_status);					/* 20 */
			}
		}
	}
	if(dz->total_samps_written != samp_total) {		/* 21 */
		sprintf(errstr,"ccounting problem: Renormalise()\n");
		close_and_delete_tempfile(dz->outfilename,dz);
		return(PROGRAM_ERROR);
	}
	close_and_delete_tempfile(dz->outfilename,dz);
	return(FINISHED);
}

/*************************** GRANULATE *********************************
 * iptr     = advancing base-pointer in input buffer.
 * iiptr    = true read-pointer in input buffer.
 * absicnt_per_chan  = advancing-base-counter in input stream, counting TOTAL samps.
 * absiicnt_per_chan = true counter in input stream, counting TOTAL samps to write pos.
 * Fptr     = advancing base-pointer in output buffer.
 * FFptr    = true write pointer in output buffer.
 * absocnt_per_chan  = advancing-base-counter in output stream, measuring OUTsize.
 */

/* rwd: some major changes here to clear bugs */
/* tw:  some more major changes here to clear implementation errors */

//TW removed redundant 'sr' parameter
int granulate(int *thissnd,long *aipc,long *aopc,float **iiiiptr,float **LLLLptr,
		double inv_sr,long *samptotal,float **maxwrite,int *nctr,dataptr dz)
{
	int exit_status;
	long  absicnt_per_chan = *aipc, absocnt_per_chan = *aopc;
	float *iiptr, *iptr = *iiiiptr, *endbufptr = NULL, *startbufptr = NULL, *thisbuf = NULL;
	float  *FFptr, *Fptr = *LLLLptr;
	long  isauspos, iisauspos;
	float   aamp = -1;
	int   bspl = 0, espl = 0, lastsnd;
	int   firsttime = FALSE;
	int   gsize_per_chan, ostep_per_chan, istep_per_chan, absiicnt_per_chan, rang_per_chan;
	long  smpscat_per_chan;
	int   resetskip = 0;
	double  time = (double)absicnt_per_chan * inv_sr;
	double	transpos = 1.0, position;
	long saved_total_samps_read = 0;
#ifdef MULTICHAN
	int chana, chanb;
#endif
	if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
		return(exit_status);

	gsize_per_chan = set_ivalue(dz->iparray[GRS_FLAGS][G_GRAINSIZE_FLAG],GRS_GRAINSIZE,GRS_HGRAINSIZE,GRS_GRANGE,dz);

	if(absicnt_per_chan + gsize_per_chan >= dz->iparam[ORIG_SMPSIZE])
		return(FINISHED);

	ostep_per_chan = set_outstep(gsize_per_chan,dz);

	if(dz->iparam[GRS_OUTLEN]>0 && (absocnt_per_chan>=dz->iparam[GRS_OUTLEN]))
		return(FINISHED); 	/* IF outfile LENGTH SPECIFIED HAS BEEN MADE: EXIT */

	FFptr = Fptr;
	if(dz->iparray[GRS_FLAGS][G_SCATTER_FLAG])	{ /* If grains scattered, scatter FFptr */
		smpscat_per_chan = do_scatter(ostep_per_chan,dz);	
		FFptr += (smpscat_per_chan * dz->iparam[GRS_OUTCHANS]);	
	}
	if(FFptr < dz->fptr[GRS_LBUF]) {
		sprintf(errstr,"Array overrun 1: granula()\n");
		return(PROGRAM_ERROR); /* FFptr can be outside the Lbuffer because Fptr is outside it */
	}
	if(FFptr >= dz->fptr[GRS_LTAILEND]) {
		if((exit_status = write_samps_granula(dz->iparam[GRS_LONGS_BUFLEN],nctr,dz))<0)
			return(exit_status);
/* IF CURRENT POINTER AT END OF LBUF WRITE SAMPS, WRAP AROUND POINTERS */
		memmove((char *)dz->fptr[GRS_LBUF],(char *)dz->fptr[GRS_LBUFEND],
			dz->iparam[GRS_LBUF_SMPXS] * sizeof(float));
		memset((char *)dz->fptr[GRS_LBUFMID],0,dz->iparam[GRS_LONGS_BUFLEN] * sizeof(float));
		FFptr    -= dz->iparam[GRS_LONGS_BUFLEN];	
		if((Fptr -= dz->iparam[GRS_LONGS_BUFLEN])  < dz->fptr[GRS_LBUF]) {
			sprintf(errstr,"Array overrun 2: granula()\n");
			return(PROGRAM_ERROR);
		}
		*maxwrite -= dz->iparam[GRS_LONGS_BUFLEN];
	}

	istep_per_chan    = set_instep(ostep_per_chan,dz);	
	if(istep_per_chan==0 && dz->iparam[GRS_OUTLEN]==0) {
		sprintf(errstr,"velocity or instep has become so small that file will be infinitely long!!\n"
					   "Try slightly increasing your very small values.\n");
		return(GOAL_FAILED);
	}
	iiptr    = iptr;

	if(dz->process==SAUSAGE) {
		lastsnd   = *thissnd;
		isauspos  = iptr  - dz->sampbuf[SAUS_BUF(lastsnd)];
		iisauspos = iiptr - dz->sampbuf[SAUS_BUF(lastsnd)];
		*thissnd  = get_next_insnd(dz);
		iiptr     = dz->sampbuf[SAUS_BUF(*thissnd)] + iisauspos;
		iptr      = dz->sampbuf[SAUS_BUF(*thissnd)] + isauspos;
	}

	absiicnt_per_chan = absicnt_per_chan;
	if(dz->iparray[GRS_FLAGS][G_RANGE_FLAG]) {
		rang_per_chan      = set_range(absiicnt_per_chan,dz);		/* RESET iiptr etc WITHIN SEARCHRANGE */	
		absiicnt_per_chan -= rang_per_chan;
		iiptr             -= rang_per_chan * dz->iparam[GRS_INCHANS];
	}
	switch(dz->process) {
	case(BRASSAGE):	
		endbufptr   = dz->sbufptr[GRS_BUF];		
		startbufptr = dz->sampbuf[GRS_BUF];		
		break;		
	case(SAUSAGE):	
		endbufptr   = dz->sbufptr[SAUS_BUF(*thissnd)];	
		startbufptr = dz->sampbuf[SAUS_BUF(*thissnd)];		
		break;		
	}
	if(iiptr >= endbufptr) {
		while(iiptr >= endbufptr) {
			switch(dz->process) {
			case(BRASSAGE):
				if((read_samps_granula(firsttime,dz))<0)					/* IF iiptr OFF END OF IBUF */
					return(exit_status);
				break;
			case(SAUSAGE):
				if((read_samps_sausage(firsttime,dz))<0)					/* IF iiptr OFF END OF IBUF */
					return(exit_status);
				break;
			}
			*samptotal += dz->ssampsread;
			if(dz->ssampsread<=0)
				return(FINISHED);
					/* READ SAMPS, WRAP BACK POINTER */
			iiptr -= dz->buflen; 	
			iptr  -= dz->buflen;
		}
	} else if(iiptr < startbufptr) {								/* IF RANGE TAKES US BAK OUT OF THIS BUF, */

		if(sloom)
			saved_total_samps_read = dz->total_samps_read;	/* saved so display_virtual_time() works during baktrak!! */
		if((resetskip = *samptotal - dz->iparam[SAMPS_IN_INBUF])<0) {	/* SET RESETSKIP TO START OF CURRENT BUFFER */
			sprintf(errstr,"Error in baktraking: granula()\n");
			return(PROGRAM_ERROR);
		}
		switch(dz->process) {
		case(BRASSAGE):
			if((exit_status = baktrak_granula(*samptotal,absiicnt_per_chan,&iiptr,dz))<0)
				return(exit_status);									/* SEEK BACKWDS, & RESET iiptr In NEW BUF */
			break;
		case(SAUSAGE):
			if((exit_status = baktrak_sausage(*thissnd,*samptotal,absiicnt_per_chan,&iiptr,dz))<0)
				return(exit_status);									/* SEEK BACKWDS, & RESET iiptr In NEW BUF */
			break;
		}
		if(sloom)
			dz->total_samps_read = saved_total_samps_read;		/* restore so its not changed by the baktraking!! */
	}
	if(dz->iparray[GRS_FLAGS][G_AMP_FLAG])
		/* RWD was set_ivalue*/
		aamp = (float) set_dvalue(dz->iparray[GRS_FLAGS][G_AMP_FLAG],GRS_AMP,GRS_HAMP,GRS_ARANGE,dz);
									/* GET SPLICE VALUES */
	bspl = set_ivalue(dz->iparray[GRS_FLAGS][G_BSPLICE_FLAG],GRS_BSPLICE,GRS_HBSPLICE,GRS_BRANGE,dz);
	espl = set_ivalue(dz->iparray[GRS_FLAGS][G_ESPLICE_FLAG],GRS_ESPLICE,GRS_HESPLICE,GRS_ERANGE,dz);
	switch(dz->process) {
	case(BRASSAGE):	thisbuf = dz->sampbuf[GRS_BUF];					break;
	case(SAUSAGE):	thisbuf = dz->sampbuf[SAUS_BUF(*thissnd)];		break;
	}

#ifndef MULTICHAN

	switch(dz->iparam[GRS_INCHANS]) {
	case(1):
		if(!make_grain(thisbuf,&iiptr,aamp,gsize_per_chan,&transpos,dz))	
			return(FINISHED);										/* COPYGRAIN TO GRAINBUF,INCLUDING ANY PSHIFT */
		do_splices(gsize_per_chan,bspl,espl,dz);					/* DO SPLICES IN GRAINBUF */
		break;
	case(2):
		if(!make_stereo_grain(thisbuf,&iiptr,aamp,gsize_per_chan,&transpos,dz))	
			return(FINISHED);										/* COPYGRAIN TO GRAINBUF,INCLUDING ANY PSHIFT */
		do_stereo_splices(gsize_per_chan,bspl,espl,dz);				/* DO SPLICES IN GRAINBUF */
		break;
	}
#else

	if(!make_multichan_grain(thisbuf,&iiptr,aamp,gsize_per_chan,&transpos,dz))	
		return(FINISHED);											/* COPYGRAIN TO GRAINBUF,INCLUDING ANY PSHIFT */
		do_multichan_splices(gsize_per_chan,bspl,espl,dz);			/* DO SPLICES IN GRAINBUF */

#endif

	if(dz->iparray[GRS_FLAGS][G_SPACE_FLAG]) {		/* IF SPATIAL INFO, GET IT,WRITE STEREO GRAIN */
		position = set_dvalue(dz->iparray[GRS_FLAGS][G_SPACE_FLAG],GRS_SPACE,GRS_HSPACE,GRS_SPRANGE,dz);
#ifndef MULTICHAN
		if((exit_status = write_stereo_grain(position,maxwrite,&Fptr,&FFptr,gsize_per_chan,nctr,dz))<0)
			return(exit_status);
#else
		if(dz->out_chans > 2) {
			chana = (int)floor(position);
			position  -= (double)chana;	//	position is relative position between 2 adjacent out-chans
			chanb = chana + 1;			//	chanb is adjacent to chana
			if(chana > dz->out_chans)	//	chana beyond last lspkr wraps around to 1st lspkr
				chana = 1;		
			else if(chana < 1)			//	chana below 1st loudspeaker wraps around to last-lspkr
				chana = dz->out_chans;
			if((exit_status = write_multichan_grain(position,chana,chanb,maxwrite,&Fptr,&FFptr,gsize_per_chan,nctr,dz))<0)
				return(exit_status);
		} else {
			if((exit_status = write_stereo_grain(position,maxwrite,&Fptr,&FFptr,gsize_per_chan,nctr,dz))<0)
				return(exit_status);
		}
#endif
	} else {			/* WRITE GRAIN */
		if((exit_status = write_grain(maxwrite,&Fptr,&FFptr,gsize_per_chan,nctr,dz))<0)
			return(exit_status);
	}
	if(sloom)
		saved_total_samps_read = dz->total_samps_read;	/* saved so not disturbed by restoring of baktrakd-from buffer */

	switch(dz->process) {
	case(BRASSAGE): 											/* IF BAKTRAKD : RESTORE ORIGINAL BUFFER */
		if(resetskip && (exit_status = reset_granula(resetskip,dz))<0)	
			return(exit_status);
		break;
	case(SAUSAGE):
		if(resetskip && (exit_status = reset_sausage(*thissnd,resetskip,dz))<0)	
			return(exit_status);
		break;
	}
	if(sloom)
		dz->total_samps_read = saved_total_samps_read;
#ifndef MULTICHAN
	iptr    += istep_per_chan;		/* MOVE FORWARD IN INFILE */
	if(dz->iparam[GRS_INCHANS]==STEREO)
		iptr    += istep_per_chan;			/* move further if using STEREO INPUT DATA */
#else
	iptr += (istep_per_chan * dz->iparam[GRS_INCHANS]);		/* MOVE FORWARD IN postmixdown input-buffer, which is either MONO or multichannel */
#endif
	absicnt_per_chan += istep_per_chan;		/* rwd: moved here from top of input section */

#ifndef MULTICHAN
	Fptr    += ostep_per_chan;				/* Fptr may still be outside (bottom of) Lbuf */
	if(dz->iparray[GRS_FLAGS][G_SPACE_FLAG] || dz->iparam[GRS_INCHANS]==STEREO)
		Fptr += ostep_per_chan;	/* Advance by stereo step, for STEREO OUTPUT */
#else
	Fptr += ostep_per_chan * dz->iparam[GRS_OUTCHANS];	/* Move forward in output buffer, which can be mono, spatialised-stereo or multichannel */
#endif
		
			/*      so we don't lose first grain */
	absocnt_per_chan += ostep_per_chan;		

	*aipc = absicnt_per_chan;	/* RETURN VALUES OF RETAINED VARIABLES */
	*aopc = absocnt_per_chan;
	*iiiiptr = iptr;
	*LLLLptr = Fptr;
	return(CONTINUE);
}

/*************************** BAKTRAK_GRANULA ******************************
 *
 *     <------------ baktrak(b) ---------------->
 *
 *     <--(b-x)-->
 *		  		  <------------ x -------------->
 *		 		 |-------- current bufer --------|
 *
 *     |-------- new buffer --------|
 *      <------------x------------->
 *				
 */

#ifndef MULTICHAN

int baktrak_granula(long samptotal,int absiicnt_per_chan,float **iiptr,dataptr dz)
{
	int  exit_status;
	long bktrk, new_position;
	unsigned long reset_size = dz->buflen + dz->iparam[GRS_BUF_SMPXS];

	bktrk = samptotal - (absiicnt_per_chan * dz->iparam[GRS_INCHANS]);
//TW I agree, this is FINE!!, and, with sndseekEx, algo is more efficient
//	whenever the search-range is relatively small (and equally efficient otherwise)
	*iiptr      = dz->sampbuf[GRS_BUF];

	if((new_position = samptotal - bktrk)<0) {
		fprintf(stdout,"WARNING: Non-fatal program error:\nRange arithmetic problem - 2, in baktraking.\n");
		fflush(stdout);
		new_position = 0;
		*iiptr = dz->sampbuf[GRS_BUF];
	}
 	if(dz->iparam[GRS_CHAN_TO_XTRACT])   /* IF we've converted from STEREO file */
		new_position *= 2;
	if(sndseekEx(dz->ifd[0],new_position,0)<0) {
		sprintf(errstr,"sndseekEx error: baktrak_granula()\n");
		return(SYSTEM_ERROR);
	}
	memset((char *)dz->sampbuf[GRS_BUF],0,reset_size * sizeof(float));
    if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
 		reset_size *= 2;
    	memset((char *)dz->sampbuf[GRS_SBUF],0,reset_size * sizeof(float));
	}
	if((exit_status = read_a_large_buf(dz))<0)
		return(exit_status);
	dz->iparam[SAMPS_IN_INBUF] = dz->ssampsread;
	return(FINISHED);
}

#else

int baktrak_granula(long samptotal,int absiicnt_per_chan,float **iiptr,dataptr dz)
{
	int  exit_status,  chans = dz->infile->channels;
	long bktrk, new_position;
	unsigned long reset_size = dz->buflen + dz->iparam[GRS_BUF_SMPXS];

	bktrk = samptotal - (absiicnt_per_chan * dz->iparam[GRS_INCHANS]);
//TW I agree, this is FINE!!, and, with sndseekEx, algo is more efficient
//	whenever the search-range is relatively small (and equally efficient otherwise)
	*iiptr      = dz->sampbuf[GRS_BUF];

	if((new_position = samptotal - bktrk)<0) {
		fprintf(stdout,"WARNING: Non-fatal program error:\nRange arithmetic problem - 2, in baktraking.\n");
		fflush(stdout);
		new_position = 0;
		*iiptr = dz->sampbuf[GRS_BUF];
	}
 	if(dz->iparam[GRS_CHAN_TO_XTRACT])   /* IF we've converted from STEREO file */
		new_position *= chans;
	if(sndseekEx(dz->ifd[0],new_position,0)<0) {
		sprintf(errstr,"sndseekEx error: baktrak_granula()\n");
		return(SYSTEM_ERROR);
	}
	memset((char *)dz->sampbuf[GRS_BUF],0,reset_size * sizeof(float));
    if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
  		reset_size *= chans;
    	memset((char *)dz->sampbuf[GRS_SBUF],0,reset_size * sizeof(float));
	}
	if((exit_status = read_a_large_buf(dz))<0)
		return(exit_status);
	dz->iparam[SAMPS_IN_INBUF] = dz->ssampsread;
	return(FINISHED);
}

#endif

/*************************** READ_A_LARGE_BUF ******************************/

#ifndef MULTICHAN

int read_a_large_buf(dataptr dz)
{
	int exit_status;
	long n, m, k;
	dz->buflen += dz->iparam[GRS_BUF_SMPXS];
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		dz->buflen *= 2;
		if((exit_status = read_samps(dz->sampbuf[GRS_SBUF],dz))<0)
			return(exit_status);
		dz->buflen /= 2;
		dz->ssampsread /= 2;
		switch(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		case(1):
			for(n=0,m=0;n<dz->ssampsread;n++,m+=2)
			(dz->sampbuf[GRS_BUF])[n] = (dz->sampbuf[GRS_SBUF])[m];
			break;
		case(2):
			for(n=0,m=1;n<dz->ssampsread;n++,m+=2)
				(dz->sampbuf[GRS_BUF])[n] = (dz->sampbuf[GRS_SBUF])[m];
			break;
		case(BOTH_CHANNELS):
			for(n=0,m=0,k=1;n<dz->ssampsread;n++,m+=2,k+=2)
				(dz->sampbuf[GRS_BUF])[n] = (float)(((dz->sampbuf[GRS_SBUF])[m] + (dz->sampbuf[GRS_SBUF])[k]) * 0.5f);
			break;
		}
	} else {
		if((exit_status = read_samps(dz->sampbuf[GRS_BUF],dz))<0)
			return(exit_status);
	}
	dz->buflen -= dz->iparam[GRS_BUF_SMPXS];
	return(FINISHED);
}

#else

int read_a_large_buf(dataptr dz)
{
	int exit_status;
	long n, m, ibufcnt;
	int chans = dz->infile->channels;
	double sum;
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		dz->buflen += dz->iparam[GRS_BUF_SMPXS];
		dz->buflen *= chans;
		if((exit_status = read_samps(dz->sampbuf[GRS_SBUF],dz))<0)
			return(exit_status);
		dz->buflen /= chans;
		dz->buflen -= dz->iparam[GRS_BUF_SMPXS];
		dz->ssampsread /= chans;
		switch(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		case(ALL_CHANNELS):
			ibufcnt = 0;
			for(n=0;n<dz->ssampsread;n++) {
				sum = 0.0;
				for(m=0;m<chans;m++)
					sum += dz->sampbuf[GRS_SBUF][ibufcnt++];
				dz->sampbuf[GRS_BUF][n] = (float)(sum/(double)chans);
			}
			break;
		default:
			for(n=0,m=dz->iparam[GRS_CHAN_TO_XTRACT];n<dz->ssampsread;n++,m+=chans)
				dz->sampbuf[GRS_BUF][n] = dz->sampbuf[GRS_SBUF][m];
			break;
		}
	} else {
		dz->buflen += dz->iparam[GRS_BUF_SMPXS];
		if((exit_status = read_samps(dz->sampbuf[GRS_BUF],dz))<0)
			return(exit_status);
		dz->buflen -= dz->iparam[GRS_BUF_SMPXS];
	}
	return(FINISHED);
}

#endif

/*************************** READ_A_NORMAL_BUF_WITH_WRAPAROUND ******************************/

#ifndef MULTICHAN

int read_a_normal_buf_with_wraparound(dataptr dz)
{
	int exit_status;
	long n,m,k;
	memmove((char *)dz->sampbuf[GRS_BUF],(char *)(dz->sbufptr[GRS_BUF]),
		dz->iparam[GRS_BUF_SMPXS] * sizeof(float));
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		dz->buflen *= 2;
		if((exit_status = read_samps(dz->sampbuf[GRS_SBUF],dz))<0)
			return(exit_status);
		dz->buflen /= 2;
		dz->ssampsread  /= 2;
		switch(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		case(1):
			for(n=0,m=0;n<dz->ssampsread;n++,m+=2)
				(dz->sampbuf[GRS_IBUF])[n] = (dz->sampbuf[GRS_SBUF])[m];
			break;
		case(2):
			for(n=0,m=1;n<dz->ssampsread;n++,m+=2)
				(dz->sampbuf[GRS_IBUF])[n] = (dz->sampbuf[GRS_SBUF])[m];
			break;
		case(BOTH_CHANNELS):
			for(n=0,m=0,k=1;n<dz->ssampsread;n++,m+=2,k+=2)
				(dz->sampbuf[GRS_IBUF])[n] = (float)(((dz->sampbuf[GRS_SBUF])[m] + (dz->sampbuf[GRS_SBUF])[k]) * 0.5f);
			break;
		}
	} else {
		if((exit_status = read_samps(dz->sampbuf[GRS_IBUF],dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

#else

int read_a_normal_buf_with_wraparound(dataptr dz)
{
	int exit_status;
	long n,m,ibufcnt;
	double sum;
	int chans = dz->infile->channels;
	memmove((char *)dz->sampbuf[GRS_BUF],(char *)(dz->sbufptr[GRS_BUF]),
		dz->iparam[GRS_BUF_SMPXS] * sizeof(float));
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		dz->buflen *= chans;
		if((exit_status = read_samps(dz->sampbuf[GRS_SBUF],dz))<0)
			return(exit_status);
		dz->buflen /= chans;
		dz->ssampsread  /= chans;
		switch(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		case(ALL_CHANNELS):
			ibufcnt = 0;
			for(n=0;n<dz->ssampsread;n++) {
				sum = 0.0;
				for(m=0;m<chans;m++)
					sum += dz->sampbuf[GRS_SBUF][ibufcnt++];
				dz->sampbuf[GRS_IBUF][n] = (float)(sum/(double)chans);
			}
			break;
		default:
			for(n=0,m=dz->iparam[GRS_CHAN_TO_XTRACT];n<dz->ssampsread;n++,m+=chans)
				dz->sampbuf[GRS_IBUF][n] = dz->sampbuf[GRS_SBUF][m];
			break;
		}
	} else {
		if((exit_status = read_samps(dz->sampbuf[GRS_IBUF],dz))<0)
			return(exit_status);
	}
	return(FINISHED);
}

#endif

/****************************** RESET_GRANULA *******************************/

#ifndef MULTICHAN

int reset_granula(int resetskip,dataptr dz)
{
	unsigned long reset_size = dz->buflen + dz->iparam[GRS_BUF_SMPXS];

	memset((char *)dz->sampbuf[GRS_BUF],0,reset_size * sizeof(float));
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		reset_size *= 2;
    	memset((char *)dz->sampbuf[GRS_SBUF],0,reset_size * sizeof(float));
	}
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) /* If we've converted from a multichannel file */
		resetskip *= 2;
	if(sndseekEx(dz->ifd[0],resetskip,0)<0) {
		sprintf(errstr,"sndseek error: reset_granula()\n");
		return(SYSTEM_ERROR);
	}
	return read_a_large_buf(dz);
}

#else

int reset_granula(int resetskip,dataptr dz)
{
	unsigned long reset_size = dz->buflen + dz->iparam[GRS_BUF_SMPXS];

	memset((char *)dz->sampbuf[GRS_BUF],0,reset_size * sizeof(float));
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		reset_size *= dz->infile->channels;
    	memset((char *)dz->sampbuf[GRS_SBUF],0,reset_size * sizeof(float));
	}
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) /* If we've converted from a multichannel file */
		resetskip *= dz->infile->channels;
	if(sndseekEx(dz->ifd[0],resetskip,0)<0) {
		sprintf(errstr,"sndseek error: reset_granula()\n");
		return(SYSTEM_ERROR);
	}
	return read_a_large_buf(dz);
}

#endif

/************************** DO_SPLICES *****************************/

void do_splices(int gsize_per_chan,int bspl,int espl,dataptr dz)
{
	if(dz->iparam[GRS_IS_BTAB] && (dz->iparam[GRS_BSPLICE] < gsize_per_chan/2))
		do_btab_splice(dz);
	else
		do_bsplice(gsize_per_chan,dz,bspl);
	if(dz->iparam[GRS_IS_ETAB] && (dz->iparam[GRS_ESPLICE] < gsize_per_chan/2))
		do_etab_splice(gsize_per_chan,dz);
	else
		do_esplice(gsize_per_chan,dz,espl);
}

/************************** DO_BTAB_SPLICE ****************************/

void do_btab_splice(dataptr dz)
{
	long n, k = dz->iparam[GRS_BSPLICE];
	double *d;
	float *gbufptr = dz->extrabuf[GRS_GBUF];   
	if(k==0)
		return;
	d = dz->parray[GRS_BSPLICETAB];
	for(n=0;n<k;n++) {
		*gbufptr = (float)/*round*/(*gbufptr * *d); 
		gbufptr++;
		d++;
	}
}

/************************** DO_BSPLICE ****************************
 *
 * rwd: changed to avoid f/p division, added exponential option
 */
void do_bsplice(int gsize_per_chan,dataptr dz,int bspl)
{
	double dif,val,lastval,length,newsum,lastsum,twodif;
	long n, k = min(bspl,gsize_per_chan);
	float *gbufptr = dz->extrabuf[GRS_GBUF];
	if(k==0)
		return;
	val = 0.0;
	length = (double)k;
	if(!dz->vflag[GRS_EXPON]){
		dif = 1.0/length;
		lastval = dif;
		*gbufptr++ = (float)val;
		*gbufptr = (float)/*round*/(*gbufptr * lastval);
		gbufptr++;   
		for(n=2;n<k;n++) {
			val = lastval + dif;
			lastval = val;
			*gbufptr = (float) /*round*/(*gbufptr * val);
			gbufptr++;
		}
	} else {			/* fast quasi-exponential */
		dif = 1.0/(length*length);
		twodif = dif * 2.0;
		lastsum = 0.0;
		lastval = dif;
		*gbufptr++ = (float)val;
		*gbufptr = (float) /*round*/(*gbufptr * lastval);	
		gbufptr++;
		for(n=2;n<k;n++) {
 			newsum = lastsum + twodif;
 			val = lastval + newsum + dif;
			*gbufptr = (float) /*round*/(*gbufptr * val);
			gbufptr++;
			lastval = val;
			lastsum = newsum;
		}
	}
}

/************************** DO_ETAB_SPLICE ****************************/

void do_etab_splice(int gsize_per_chan,dataptr dz)
{
	long n, k = (long)dz->iparam[GRS_ESPLICE];
	double *d;
	float *gbufptr = dz->extrabuf[GRS_GBUF] + gsize_per_chan - k;   
	if(k==0)
		return;
	d = dz->parray[GRS_ESPLICETAB];
	for(n=0;n<k;n++) {
		*gbufptr = (float) /*round*/(*gbufptr * *d); 
		gbufptr++;
		d++;
	}
}

/************************** DO_ESPLICE ****************************/
/* rwd: changed to avoid f/p division, added exponential option */

void do_esplice(int gsize_per_chan,dataptr dz,int espl)
{
	double dif,val,lastval,length,newsum,lastsum,twodif;
	long n, k = min(espl,gsize_per_chan);
	float *gbufptr = dz->extrabuf[GRS_GBUF] + gsize_per_chan;
	if(k==0)
		return;
	val = 0.0;
	length = (double) k;	
	if(!dz->vflag[GRS_EXPON]) {
		dif = 1.0/length;
		lastsum = dif;
		*--gbufptr = (float)val;
		gbufptr--;
		*gbufptr = (float) /*round*/(*gbufptr * lastsum);   
		for(n=k-3;n>=0;n--) {
			val = lastsum + dif;
			lastsum = val;
			gbufptr--;
			*gbufptr = (float) /*round*/(*gbufptr * val);
		}
	} else {			/* fast quasi-exponential */ 
		dif = 1.0/(length * length);
		twodif = dif * 2.0;
		lastsum = 0.0;
		lastval = dif;
		*--gbufptr = (float)val;
		gbufptr--;
		*gbufptr = (float) /*round*/(*gbufptr * lastval); 
		for(n=k-3;n>=0;n--) {
			newsum =lastsum + twodif;
			val = lastval + newsum + dif;
			gbufptr--;
			*gbufptr = (float) /*round*/ (*gbufptr * val);
			lastval = val;
			lastsum = newsum;
		}
	}
}

/***************************** MAKE_GRAIN ***************************
 * iicnt = RELATIVE read-pointer in input buffer.
 */

int make_grain(float *b,float **iiptr,float aamp,int gsize_per_chan,double *transpos,dataptr dz)
{  						/* rwd: added interpolation option */
	long n,real_gsize = gsize_per_chan;
	double flcnt;
	long iicnt;
	double flcnt_frac;
	float *s = *iiptr, *gbufptr = dz->extrabuf[GRS_GBUF];

	if(aamp>=0) {
		if(!dz->iparray[GRS_FLAGS][G_PITCH_FLAG]) {
			if(real_gsize >= (long)dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			for(n=0;n<real_gsize;n++)  /* COPY GRAIN TO GRAINBUF & RE-LEVEL ETC */
				*gbufptr++ = (float)(*s++ * aamp);
		} else {
			iicnt = s - b;
			*transpos = set_dvalue(dz->iparray[GRS_FLAGS][G_PITCH_FLAG],GRS_PITCH,GRS_HPITCH,GRS_PRANGE,dz);
			*transpos = pow(2.0,*transpos);
			if((int)(real_gsize * (*transpos))+1+iicnt >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			flcnt = (double)iicnt;
			if(!dz->vflag[GRS_NO_INTERP]){
				flcnt_frac = flcnt - (double)iicnt;
				for(n=0;n<real_gsize;n++) {
					*gbufptr++ = interp_gval_with_amp(s,flcnt_frac,dz->iparam[GRS_INCHANS],aamp);
					flcnt += *transpos;  
					iicnt = (long)flcnt;	    	    
					s = b + iicnt;
					flcnt_frac = flcnt - (double) iicnt;	    	    
				}
			} else {  			/* do truncate as originally */
				for(n=0;n<real_gsize;n++){
					*gbufptr++ = (float)(*s * aamp);
					flcnt += *transpos;
					iicnt = round(flcnt);
					s     = b + iicnt;
				}
			}	
		}
	} else {		/* NO CHANGE IN AMPLITUDE */
		if(!dz->iparray[GRS_FLAGS][G_PITCH_FLAG]) {
			if(real_gsize >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			for(n=0;n<real_gsize;n++)
				*gbufptr++ = *s++;
		} else {
			iicnt = s - b;
			*transpos = set_dvalue(dz->iparray[GRS_FLAGS][G_PITCH_FLAG],GRS_PITCH,GRS_HPITCH,GRS_PRANGE,dz);
			*transpos = pow(2.0,*transpos);
			if((int)(real_gsize * (*transpos))+1+iicnt >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			flcnt = (double)iicnt;
			if(dz->vflag[GRS_NO_INTERP]){	  /* do truncate as originally */
				for(n=0;n<gsize_per_chan;n++){
					*gbufptr++ = *s;
					flcnt += *transpos;
					iicnt = round(flcnt);
					s = b + iicnt;
				}
			} else {  			
				flcnt_frac = flcnt - (double)iicnt;
				for(n=0;n<real_gsize;n++) {
					*gbufptr++ = interp_gval(s,flcnt_frac,dz->iparam[GRS_INCHANS]);
					flcnt += *transpos;  
					iicnt = (long)flcnt;	    	    
					s = b + iicnt;
					flcnt_frac = flcnt - (double)iicnt;	    	    
				}
			}	
		}
	}
	*iiptr = s;
	return(1);
}

/***************************** MAKE_STEREO_GRAIN ***************************
 * iicnt = RELATIVE read-pointer in input buffer (starts at 0).
 */

int make_stereo_grain(float *b,float **iiptr,float aamp,int gsize_per_chan,double *transpos,dataptr dz)
{
	long n;
	double flcnt;
	long iicnt, real_gsize = gsize_per_chan * 2;
	double flcnt_frac;
	float *s = *iiptr, *gbufptr = dz->extrabuf[GRS_GBUF];
	if(aamp>=0) {
		if(!dz->iparray[GRS_FLAGS][G_PITCH_FLAG]) {
			if(real_gsize >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			for(n=0;n<real_gsize;n++)   /* COPY GRAIN TO GRAINBUF & RE-LEVEL ETC */
				*gbufptr++ = (*s++ * aamp);
		} else {			
			iicnt = (s - b)/2;
			*transpos = set_dvalue(dz->iparray[GRS_FLAGS][G_PITCH_FLAG],GRS_PITCH,GRS_HPITCH,GRS_PRANGE,dz);
			*transpos = pow(2.0,*transpos);
			if((((int)(gsize_per_chan * *transpos)+1) + iicnt) * 2 >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			flcnt = (double)iicnt;
			if(dz->vflag[GRS_NO_INTERP]){  /* do truncate as originally */
				for(n=0;n<gsize_per_chan;n++){
					*gbufptr++ = (*s * aamp);
					s++;
					*gbufptr++ = (*s * aamp);
					flcnt += *transpos;
					iicnt = round(flcnt);
					s = b + (iicnt * 2);
				}
			} else {  			
				flcnt_frac = flcnt - (double)iicnt;
				for(n=0;n<gsize_per_chan;n++) {
					*gbufptr++ = interp_gval_with_amp(s,flcnt_frac,dz->iparam[GRS_INCHANS],aamp);
					s++;
					*gbufptr++ = interp_gval_with_amp(s,flcnt_frac,dz->iparam[GRS_INCHANS],aamp);
					flcnt += *transpos;  
					iicnt = (long)flcnt;
					s = b + (iicnt * 2);
					flcnt_frac = flcnt - (double) iicnt;	    	    
				}
			}					   
		}
	} else {		/* NO CHANGE IN AMPLITUDE */
		if(!dz->iparray[GRS_FLAGS][G_PITCH_FLAG]) {
			if(real_gsize >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			for(n=0;n<real_gsize;n++)
				*gbufptr++ = *s++;
		} else {
			iicnt = (s - b)/2;
			*transpos = set_dvalue(dz->iparray[GRS_FLAGS][G_PITCH_FLAG],GRS_PITCH,GRS_HPITCH,GRS_PRANGE,dz);
			*transpos = pow(2.0,*transpos);
			if((((int)(gsize_per_chan * *transpos)+1) + iicnt) * 2 >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			flcnt = (double)iicnt;
			if(!dz->vflag[GRS_NO_INTERP]){
 				flcnt_frac = flcnt - (double)iicnt;
				for(n=0;n<gsize_per_chan;n++) {
					*gbufptr++ = interp_gval(s,flcnt_frac,dz->iparam[GRS_INCHANS]);
					s++;
					*gbufptr++ = interp_gval(s,flcnt_frac,dz->iparam[GRS_INCHANS]);
					flcnt += *transpos;  
					iicnt = (long)flcnt;	    	    
					s = b + (iicnt * 2);
					flcnt_frac = flcnt - (double)iicnt;	    	    
				}
			} else {  			/* do truncate as originally */
				for(n=0;n<gsize_per_chan;n++){
					*gbufptr++ = *s++;
					*gbufptr++ = *s;
					flcnt += *transpos;
					iicnt = round(flcnt);
					s = b + (iicnt * 2);
				}	
			}
		}
	}
	*iiptr = s;
	return(1);
}

/***************************** MAKE_MULTICHAN_GRAIN ***************************
 * iicnt = RELATIVE read-pointer in input buffer (starts at 0).
 */

int make_multichan_grain(float *b,float **iiptr,float aamp,int gsize_per_chan,double *transpos,dataptr dz)
{
	long n, k;
	int chans = dz->iparam[GRS_INCHANS];		// GRS_INCHANS = no of chans in input grain, i.e. mono if its been mixed down to mono
	double flcnt;								//	but = dz->infile->channels, if not
	long iicnt, real_gsize = gsize_per_chan * chans;
	double flcnt_frac;
	float *s = *iiptr, *gbufptr = dz->extrabuf[GRS_GBUF];
	if(aamp>=0) {
		if(!dz->iparray[GRS_FLAGS][G_PITCH_FLAG]) {
			if(real_gsize >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			for(n=0;n<real_gsize;n++)   /* COPY GRAIN TO GRAINBUF & RE-LEVEL ETC */
				*gbufptr++ = (*s++ * aamp);
		} else {			
			iicnt = (s - b)/chans;
			*transpos = set_dvalue(dz->iparray[GRS_FLAGS][G_PITCH_FLAG],GRS_PITCH,GRS_HPITCH,GRS_PRANGE,dz);
			*transpos = pow(2.0,*transpos);
			if((((int)(gsize_per_chan * *transpos)+1) + iicnt) * chans >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			flcnt = (double)iicnt;
			if(dz->vflag[GRS_NO_INTERP]){  /* do truncate as originally */
				for(n=0;n<gsize_per_chan;n++){
					for(k = 0; k < chans;k++) {
						*gbufptr++ = (*s * aamp);
						s++;
					}
					flcnt += *transpos;
					iicnt = round(flcnt);
					s = b + (iicnt * chans);
				}
			} else {  			
				flcnt_frac = flcnt - (double)iicnt;
				for(n=0;n<gsize_per_chan;n++) {
					for(k = 0; k < chans;k++) {
						*gbufptr++ = interp_gval_with_amp(s,flcnt_frac,dz->iparam[GRS_INCHANS],aamp);
						s++;
					}
					flcnt += *transpos;  
					iicnt = (long)flcnt;
					s = b + (iicnt * chans);
					flcnt_frac = flcnt - (double) iicnt;	    	    
				}
			}					   
		}
	} else {		/* NO CHANGE IN AMPLITUDE */
		if(!dz->iparray[GRS_FLAGS][G_PITCH_FLAG]) {
			if(real_gsize >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			for(n=0;n<real_gsize;n++)
				*gbufptr++ = *s++;
		} else {
			iicnt = (s - b)/chans;
			*transpos = set_dvalue(dz->iparray[GRS_FLAGS][G_PITCH_FLAG],GRS_PITCH,GRS_HPITCH,GRS_PRANGE,dz);
			*transpos = pow(2.0,*transpos);
			if((((int)(gsize_per_chan * *transpos)+1) + iicnt) * chans >= dz->iparam[SAMPS_IN_INBUF])	  /* JUNE 1996 */
				return(0);
			flcnt = (double)iicnt;
			if(!dz->vflag[GRS_NO_INTERP]){
 				flcnt_frac = flcnt - (double)iicnt;
				for(n=0;n<gsize_per_chan;n++) {
					for(k=0;k < chans;k++) {
						*gbufptr++ = interp_gval(s,flcnt_frac,dz->iparam[GRS_INCHANS]);
						s++;
					}
					flcnt += *transpos;  
					iicnt = (long)flcnt;	    	    
					s = b + (iicnt * chans);
					flcnt_frac = flcnt - (double)iicnt;	    	    
				}
			} else {  			/* do truncate as originally */
				for(n=0;n<gsize_per_chan;n++){
					for(k=0;k < chans;k++)
						*gbufptr++ = *s++;
					flcnt += *transpos;
					iicnt = round(flcnt);
					s = b + (iicnt * chans);
				}	
			}
		}
	}
	*iiptr = s;
	return(1);
}

/************************* WRITE_GRAIN ****************************/

int write_grain(float **maxwrite,float **Fptr, float **FFptr,int gsize_per_chan,int *nctr,dataptr dz)
{
	int exit_status;
	long n, to_doo, exess;
	float *writend;
	long real_gsize = gsize_per_chan * dz->iparam[GRS_INCHANS];
	float *fptr = *Fptr, *ffptr = *FFptr;
	float *gbufptr = dz->extrabuf[GRS_GBUF];
	if((writend = (ffptr + real_gsize)) > *maxwrite)
		*maxwrite = writend;
	if(ffptr + real_gsize > dz->fptr[GRS_LTAILEND]) {
		to_doo = dz->fptr[GRS_LTAILEND] - ffptr;
		if((exess  = real_gsize - to_doo) >= dz->buflen) {
 			sprintf(errstr,"Array overflow:write_grain().\n");
			return(PROGRAM_ERROR);
		}
		for(n = 0; n < to_doo; n++)
			*ffptr++ += *gbufptr++;
		if((exit_status = write_samps_granula(dz->buflen,nctr,dz))<0)
			return(exit_status);
		memmove((char *)dz->fptr[GRS_LBUF],(char *)dz->fptr[GRS_LBUFEND],
			(size_t)dz->iparam[GRS_LBUF_SMPXS] * sizeof(float));
		memset((char *)dz->fptr[GRS_LBUFMID],0,dz->iparam[GRS_LONGS_BUFLEN] * sizeof(float));
		ffptr -= dz->iparam[GRS_LONGS_BUFLEN];	
		fptr  -= dz->iparam[GRS_LONGS_BUFLEN];
		*maxwrite -= dz->iparam[GRS_LONGS_BUFLEN];	/* APRIL 1996 */
		for(n = 0; n < exess; n++)
			*ffptr++ += *gbufptr++;
		*Fptr  = fptr;
		*FFptr = ffptr;
		return(FINISHED);
	}
	for(n=0;n<real_gsize;n++)
		*ffptr++ += *gbufptr++;
	*Fptr  = fptr;
	*FFptr = ffptr;
	return(FINISHED);
}

/************************* WRITE_STEREO_GRAIN ****************************/

int write_stereo_grain(double rpos,float **maxwrite,float **Fptr,float **FFptr,int gsize_per_chan,int *nctr,dataptr dz)
{
	int exit_status;
	long   n, to_doo, exess;
	long   /**writend,*/ real_gsize = gsize_per_chan * STEREO;
	double lpos;
	double adjust = dehole(rpos);
	float  *writend, *fptr = *Fptr, *ffptr = *FFptr;
	float  *gbufptr = dz->extrabuf[GRS_GBUF];
	lpos  = (1.0 - rpos) * adjust;
	rpos *= adjust;
	if((writend = (ffptr + real_gsize)) > *maxwrite)
		*maxwrite = writend;
	if(ffptr + real_gsize >= dz->fptr[GRS_LTAILEND]) {
		to_doo = dz->fptr[GRS_LTAILEND] - ffptr;
		if((exess  = real_gsize - to_doo) >= dz->iparam[GRS_LONGS_BUFLEN]) {
 			sprintf(errstr,"Array overflow: write_stereo_grain()\n");
			return(PROGRAM_ERROR);
		}
		to_doo /= 2;
		for(n = 0; n < to_doo; n++) {
			*ffptr++ += (float) (*gbufptr * lpos);
			*ffptr++ += (float) (*gbufptr++ * rpos);
		}
		if((exit_status = write_samps_granula(dz->iparam[GRS_LONGS_BUFLEN],nctr,dz))<0)
			return(exit_status);
		memmove((char *)dz->fptr[GRS_LBUF],(char *)dz->fptr[GRS_LBUFEND],
			(size_t)dz->iparam[GRS_LBUF_SMPXS] * sizeof(float));
		memset((char *)dz->fptr[GRS_LBUFMID],0,dz->iparam[GRS_LONGS_BUFLEN] * sizeof(float));
		ffptr -= dz->iparam[GRS_LONGS_BUFLEN];
		fptr  -= dz->iparam[GRS_LONGS_BUFLEN];
		*maxwrite -= dz->buflen; /* APR 1996 */
		exess /= 2;
		for(n = 0; n < exess; n++) {
			*ffptr++ += /*round*/(float)(*gbufptr * lpos);
			*ffptr++ += /*round*/(float)(*gbufptr++ * rpos);
		}
		*Fptr  = fptr;
		*FFptr = ffptr;
		return(FINISHED);
	}
	for(n=0;n<gsize_per_chan;n++) {
		*ffptr++ += /*round*/(float)(*gbufptr   * lpos); /*rwd: was *gbuf */
		*ffptr++ += /*round*/(float)(*gbufptr++ * rpos);
	}
	*Fptr  = fptr;
	*FFptr = ffptr;
	return(FINISHED);
}

/****************************** DEHOLE ************************/

#define NONLIN  0.5
#define DEVIATE 0.25

double dehole(double pos)
{
	double comp = 1.0 - (fabs((pos * 2.0) - 1.0));	/* 1 */
	comp = pow(comp,NONLIN);						/* 2 */
	comp *= DEVIATE;								/* 3 */
	comp += (1.0 - DEVIATE);						/* 4 */
	return(comp);
}


/**************************** DO_SCATTER *****************************
 *  scatter forward by fraction of randpart of 1 event separation.
 *
 *		  Possible scatter  |      	|-------------->|     
 *		    Range selected  |		|------>      	|	
 *	Random choice within range  |		|--->      	|	
 */

long do_scatter(int ostep_per_chan,dataptr dz)
{
	double scat  = dz->param[GRS_SCATTER] * drand48();
	long   smpscat_per_chan = round((double)ostep_per_chan * scat);
	return(smpscat_per_chan);
}				

/******************************** WRITE_SAMPS_GRANULA ****************************
 *
 * Normalise each buffer before writing to obuf, then to file.
 * Save the normalisation factor in array normfact[].
 */

int write_samps_granula(long k,int *nctr,dataptr dz)
{
	int exit_status;
	/*long*/double val = 0.0 /*0L, max_long = 0L*/;
	long n;
	double thisnorm = 1.0, max_double = 0.0;
	float *s = NULL;
	float  *l = dz->fptr[GRS_LBUF];
	switch(dz->process) {
	case(BRASSAGE):	s = dz->sampbuf[GRS_OBUF];	break;
	case(SAUSAGE):	s = dz->sampbuf[SAUS_OBUF];	break;
	}
	for(n=0;n<k;n++) {
		if((val = fabs(l[n])) > max_double)
			max_double = val;
	}
	if(/*max_long*/max_double > F_MAXSAMP) {
		thisnorm = (double)F_MAXSAMP/max_double;
		for(n=0;n<k;n++)
			s[n] =  /*round*/(float) ((double)(l[n]) * thisnorm);
	} else {
		for(n=0;n<k;n++)
			s[n] =  l[n];
	}
	if((exit_status = write_samps(s,k,dz))<0)
		return(exit_status);
	(dz->parray[GRS_NORMFACT])[(*nctr)++] = thisnorm;
	if(*nctr >= dz->iparam[GRS_ARRAYSIZE]) {
		dz->iparam[GRS_ARRAYSIZE] += BIGARRAY;
		if((dz->parray[GRS_NORMFACT] = 
		(double *)realloc((char *)(dz->parray[GRS_NORMFACT]),dz->iparam[GRS_ARRAYSIZE]*sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate normalisation array.\n");
			return(MEMORY_ERROR);
		}
	}
	return(FINISHED);
}

#ifndef MULTICHAN

/************************** DO_STEREO_SPLICES *****************************/

void do_stereo_splices(int gsize_per_chan,int bspl,int espl,dataptr dz)
{
	if(dz->iparam[GRS_IS_BTAB] && (dz->iparam[GRS_BSPLICE] < gsize_per_chan/2))
		do_stereo_btab_splice(dz);
	else
		do_stereo_bsplice(gsize_per_chan,dz,bspl);
	if(dz->iparam[GRS_IS_ETAB] && (dz->iparam[GRS_ESPLICE] < gsize_per_chan/2))
		do_stereo_etab_splice(gsize_per_chan,dz);
	else
		do_stereo_esplice(gsize_per_chan,dz,espl);
}

/************************** DO_STEREO_BTAB_SPLICE ****************************/

void do_stereo_btab_splice(dataptr dz)
{
	long n, k = dz->iparam[GRS_BSPLICE];
	double *d;
	float *gbufptr = dz->extrabuf[GRS_GBUF];   
	if(k==0)
		return;
	d = dz->parray[GRS_BSPLICETAB];
	for(n=0;n<k;n++) {
		*gbufptr = (float)/*round*/(*gbufptr * *d); 
		gbufptr++;
		*gbufptr = (float)/*round*/(*gbufptr * *d); 
		gbufptr++;
		d++;
	}
}

/************************** DO_STEREO_BSPLICE ****************************
 *
 * rwd: changed to avoid f/p division, added exponential option
 */
void do_stereo_bsplice(int gsize_per_chan,dataptr dz,int bspl)
{
	double dif,val,lastval,length,newsum,lastsum,twodif;
	long n, k = min(bspl,gsize_per_chan);
	float *gbufptr = dz->extrabuf[GRS_GBUF];
	if(k==0)
		return;
	val = 0.0;
	length = (double)k;
	if(!dz->vflag[GRS_EXPON]){
		dif = 1.0/length;
		lastval = dif;
		*gbufptr++ = (float)val;
		*gbufptr++ = (float)val;
		*gbufptr = (float)/*round*/(*gbufptr * lastval);
		gbufptr++;   
		*gbufptr = (float) /*round*/(*gbufptr * lastval);
		gbufptr++;   
		for(n=2;n<k;n++) {
			val = lastval + dif;
			lastval = val;
			*gbufptr = (float) /*round*/(*gbufptr * val);
			gbufptr++;
			*gbufptr = (float) /*round*/(*gbufptr * val);
			gbufptr++;
		}
	}  else {			/* fast quasi-exponential */
		dif = 1.0/(length*length);
		twodif = dif * 2.0;
		lastsum = 0.0;
		lastval = dif;
		*gbufptr++ = (float)val;/* mca - round or truncate? */
		*gbufptr++ = (float)val;/* mca - round or truncate? */
		*gbufptr = (float) /*round*/(*gbufptr * lastval);	 /*** fixed MAY 1998 ***/
		gbufptr++;
		*gbufptr = (float) /*round*/(*gbufptr * lastval);	 /*** fixed MAY 1998 ***/
		gbufptr++;
		for(n=2;n<k;n++) {
 			newsum = lastsum + twodif;
 			val = lastval + newsum + dif;
			*gbufptr = (float) /*round*/(*gbufptr * val);
			gbufptr++;
			*gbufptr = (float) /*round*/(*gbufptr * val);
			gbufptr++;
			lastval = val;
			lastsum = newsum;
		}
	}
}

/************************** DO_STEREO_ETAB_SPLICE ****************************/

void do_stereo_etab_splice(int gsize_per_chan,dataptr dz)
{
	long n, k = dz->iparam[GRS_ESPLICE];
	double *d;
	float *gbufptr = dz->extrabuf[GRS_GBUF] + ((gsize_per_chan - k) * 2);   
	if(k==0)
		return;
	d = dz->parray[GRS_ESPLICETAB];
	for(n=0;n<k;n++) {
		*gbufptr = (float) /*round*/(*gbufptr * *d); 
		gbufptr++;
		*gbufptr = (float) /*round*/(*gbufptr * *d); 
		gbufptr++;
		d++;
	}
}

/************************** DO_STEREO_ESPLICE ****************************/
/* rwd: changed to avoid f/p division, added exponential option */

void do_stereo_esplice(int gsize_per_chan,dataptr dz,int espl)
{
	double dif,val,lastval,length,newsum,lastsum,twodif;
	long n, k = min(espl,gsize_per_chan);
	long real_gsize = gsize_per_chan * 2;
	float *gbufptr = dz->extrabuf[GRS_GBUF] + real_gsize;
	if(k==0)
		return;
	val = 0.0;
	length = (double) k;
	if(!dz->vflag[GRS_EXPON]) {
		dif = 1.0/length;
		lastsum = dif;
		gbufptr--;
		*gbufptr = (float)val;
		gbufptr--;
		*gbufptr = (float)val;
		gbufptr--;
		*gbufptr = (float) /*round*/(*gbufptr * lastsum);   
		gbufptr--;
		*gbufptr = (float) /*round*/(*gbufptr * lastsum);   
		for(n=k-3;n>=0;n--) {
			val = lastsum + dif;
			lastsum = val;
			gbufptr--;
			*gbufptr = (float) /*round*/(*gbufptr * val);
			gbufptr--;
			*gbufptr = (float) /*round*/(*gbufptr * val);
		}
	} else {			/* fast quasi-exponential */ 
		dif = 1.0/(length * length);
		twodif = dif * 2.0;
		lastsum = 0.0;
		lastval = dif;
		gbufptr--;
		*gbufptr = (float)val;
		gbufptr--;
		*gbufptr = (float)val;
		gbufptr--;
		*gbufptr = (float) /*round*/(*gbufptr * lastval);
		gbufptr--;
		*gbufptr = (float)/*round*/(*gbufptr * lastval);
		for(n=k-3;n>=0;n--) {
			newsum =lastsum + twodif;
			val = lastval + newsum + dif;
			gbufptr--;
			*gbufptr = (float)/*round*/(*gbufptr * val);
			gbufptr--;
			*gbufptr = (float)/*round*/(*gbufptr * val);
			lastval = val;
			lastsum = newsum;
		}
	}
}

#else

/************************** DO_MULTICHAN_SPLICES *****************************/

void do_multichan_splices(int gsize_per_chan,int bspl,int espl,dataptr dz)
{
	if(dz->iparam[GRS_IS_BTAB] && (dz->iparam[GRS_BSPLICE] < gsize_per_chan/2))
		do_multichan_btab_splice(dz);
	else
		do_multichan_bsplice(gsize_per_chan,dz,bspl);
	if(dz->iparam[GRS_IS_ETAB] && (dz->iparam[GRS_ESPLICE] < gsize_per_chan/2))
		do_multichan_etab_splice(gsize_per_chan,dz);
	else
		do_multichan_esplice(gsize_per_chan,dz,espl);
}

/************************** DO_MULTICHAN_BTAB_SPLICE ****************************/

void do_multichan_btab_splice(dataptr dz)
{
	long n, j, k = dz->iparam[GRS_BSPLICE];
	double *d;
	float *gbufptr = dz->extrabuf[GRS_GBUF];   
	
	if(k==0)
		return;
	d = dz->parray[GRS_BSPLICETAB];
	for(n=0;n<k;n++) {
		for(j=0;j<dz->iparam[GRS_INCHANS];j++) {
			*gbufptr = (float)/*round*/(*gbufptr * *d); 
			gbufptr++;
		}
		d++;
	}
}

/************************** DO_MULTICHAN_BSPLICE ****************************
 *
 * rwd: changed to avoid f/p division, added exponential option
 */
void do_multichan_bsplice(int gsize_per_chan,dataptr dz,int bspl)
{
	double dif,val,lastval,length,newsum,lastsum,twodif;
	long n, j, k = min(bspl,gsize_per_chan);
	float *gbufptr = dz->extrabuf[GRS_GBUF];
	int chans = dz->iparam[GRS_INCHANS];
	if(k==0)
		return;
	val = 0.0;
	length = (double)k;
	if(!dz->vflag[GRS_EXPON]){
		dif = 1.0/length;
		lastval = dif;
		for(j= 0; j<chans;j++)
			*gbufptr++ = (float)val;
		for(j= 0; j<chans;j++) {
			*gbufptr = (float)/*round*/(*gbufptr * lastval);
			gbufptr++;   
		}
		for(n=2;n<k;n++) {
			val = lastval + dif;
			lastval = val;
			for(j= 0; j<chans;j++) {
				*gbufptr = (float) /*round*/(*gbufptr * val);
				gbufptr++;
			}
		}
	}  else {			/* fast quasi-exponential */
		dif = 1.0/(length*length);
		twodif = dif * 2.0;
		lastsum = 0.0;
		lastval = dif;
		for(j=0;j<chans;j++)
			*gbufptr++ = (float)val;/* mca - round or truncate? */
		for(j=0;j<chans;j++) {
			*gbufptr = (float) /*round*/(*gbufptr * lastval);	 /*** fixed MAY 1998 ***/
			gbufptr++;
		}
		for(n=2;n<k;n++) {
 			newsum = lastsum + twodif;
 			val = lastval + newsum + dif;
			for(j=0;j<chans;j++) {
				*gbufptr = (float) /*round*/(*gbufptr * val);
				gbufptr++;
			}
			lastval = val;
			lastsum = newsum;
		}
	}
}

/************************** DO_MULTICHAN_ETAB_SPLICE ****************************/

void do_multichan_etab_splice(int gsize_per_chan,dataptr dz)
{
	long n, j, k = dz->iparam[GRS_ESPLICE];
	double *d;
	int chans = dz->iparam[GRS_INCHANS];
	float *gbufptr = dz->extrabuf[GRS_GBUF] + ((gsize_per_chan - k) * chans);   
	if(k==0)
		return;
	d = dz->parray[GRS_ESPLICETAB];
	for(n=0;n<k;n++) {
		for(j=0;j<chans;j++) {
			*gbufptr = (float) /*round*/(*gbufptr * *d); 
			gbufptr++;
		}
		d++;
	}
}

/************************** DO_MULTICHAN_ESPLICE ****************************/
/* rwd: changed to avoid f/p division, added exponential option */

void do_multichan_esplice(int gsize_per_chan,dataptr dz,int espl)
{
	double dif,val,lastval,length,newsum,lastsum,twodif;
	long n, j, k = min(espl,gsize_per_chan);
	int chans = dz->iparam[GRS_INCHANS];
	long real_gsize = gsize_per_chan * chans;
	float *gbufptr = dz->extrabuf[GRS_GBUF] + real_gsize;
	if(k==0)
		return;
	val = 0.0;
	length = (double) k;
	if(!dz->vflag[GRS_EXPON]) {
		dif = 1.0/length;
		lastsum = dif;
		for(j=0;j<chans;j++) {
			gbufptr--;
			*gbufptr = (float)val;
		}
		for(j=0;j<chans;j++) {
			gbufptr--;
			*gbufptr = (float) /*round*/(*gbufptr * lastsum);   
		}
		for(n=k-3;n>=0;n--) {
			val = lastsum + dif;
			lastsum = val;
			for(j=0;j<chans;j++) {
				gbufptr--;
				*gbufptr = (float) /*round*/(*gbufptr * val);
			}
		}
	} else {			/* fast quasi-exponential */ 
		dif = 1.0/(length * length);
		twodif = dif * 2.0;
		lastsum = 0.0;
		lastval = dif;
		for(j=0;j<chans;j++) {
			gbufptr--;
			*gbufptr = (float)val;
		}
		for(j=0;j<chans;j++) {
			gbufptr--;
			*gbufptr = (float) /*round*/(*gbufptr * lastval);
		}
		for(n=k-3;n>=0;n--) {
			newsum =lastsum + twodif;
			val = lastval + newsum + dif;
			for(j=0;j<chans;j++) {
				gbufptr--;
				*gbufptr = (float)/*round*/(*gbufptr * val);
			}
			lastval = val;
			lastsum = newsum;
		}
	}
}

#endif

/************************** INTERP_GVAL_WITH_AMP ****************************/

float interp_gval_with_amp(float *s,double flcnt_frac,int chans,float ampp)
{
	/*long*/float tthis = /*(long)*/ *s * ampp;
	/*long*/float next = /*(long)*/ *(s+chans) * ampp;
	//long val  = this + round((double)(next-this) * flcnt_frac);
	//return(short)((val+TWO_POW_14) >> 15);
	return (float) (tthis + ((next-tthis) * flcnt_frac));
}

/************************** INTERP_GVAL ****************************/

float interp_gval(float *s,double flcnt_frac,int chans)
{
	float tthis = *s;
	float next = *(s+chans);
	float val  = (float)(tthis + /*round*/((double)(next-tthis) * flcnt_frac));
	return(val);
}
	
/*************************** SET_RANGE ******************************/

int set_range(int absiicnt,dataptr dz)
{
	long val;
	val = min(dz->iparam[GRS_SRCHRANGE],absiicnt);
	val = round((double)val * drand48());
	return(val);
}

/*************************** SET_OUTSTEP ******************************/

int set_outstep(int gsize_per_chan,dataptr dz)		
{							
	double dens;
	int val = 0;
	/* TW 4 :2002 */
	switch(dz->process){
	case(BRASSAGE):
		switch(dz->mode) {
		case(GRS_BRASSAGE):
		case(GRS_FULL_MONTY):
		case(GRS_REVERB):
		case(GRS_GRANULATE):
			dens = set_dvalue(dz->iparray[GRS_FLAGS][G_DENSITY_FLAG],GRS_DENSITY,GRS_HDENSITY,GRS_DRANGE,dz);
			val  = round((double)gsize_per_chan/(double)dens);
			break;
		default:
			val  = round((double)gsize_per_chan/(double)GRS_DEFAULT_DENSITY);
		}
		break;

	case(SAUSAGE):
		dens = set_dvalue(dz->iparray[GRS_FLAGS][G_DENSITY_FLAG],GRS_DENSITY,GRS_HDENSITY,GRS_DRANGE,dz);
		val  = round((double)gsize_per_chan/(double)dens);
		break;
	}
	return(val);
}

/*************************** SET_INSTEP ******************************/

int set_instep(int ostep_per_chan,dataptr dz)	
{												/* rwd: added range error traps */
	double velocity;
	int istep_per_chan = 0;
	switch(dz->process) {		 /* TW 4:2002 */
    case(BRASSAGE):
		switch(dz->mode) {
		case(GRS_BRASSAGE):
		case(GRS_FULL_MONTY):
		case(GRS_TIMESTRETCH):
			velocity = set_dvalue(dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG],GRS_VELOCITY,GRS_HVELOCITY,GRS_VRANGE,dz);
			istep_per_chan =  round(velocity * (double)ostep_per_chan);
			break;
		default:
			istep_per_chan =  ostep_per_chan;	/* default velocity is 1.0 */
			break;
		}
		break;
	case(SAUSAGE):
		velocity = set_dvalue(dz->iparray[GRS_FLAGS][G_VELOCITY_FLAG],GRS_VELOCITY,GRS_HVELOCITY,GRS_VRANGE,dz);
		istep_per_chan =  round(velocity * (double)ostep_per_chan);
		break;
	}
	return(istep_per_chan);
}

/*************************** SET_IVALUE ******************************/

int set_ivalue(int flag,int paramno,int hparamno,int rangeno,dataptr dz)
{
	int val = 0;
	switch(flag) {
	case(NOT_SET):
	case(FIXED):
	case(VARIABLE):
		val = dz->iparam[paramno];
		break;
	case(RANGE_VLO):
	case(RANGE_VHI):
	case(RANGE_VHILO):
		dz->iparam[rangeno] = dz->iparam[hparamno] - dz->iparam[paramno];
		/* fall thro */
	case(RANGED):
		val = round((drand48() * dz->iparam[rangeno]) + (double)dz->iparam[paramno]);
		break;
	}    
	return(val);
}

/*************************** SET_DVALUE ******************************/

double set_dvalue(int flag,int paramno,int hparamno,int rangeno,dataptr dz)
{
	double val = 0;
	switch(flag) {
	case(NOT_SET):
	case(FIXED):
	case(VARIABLE):
		val = dz->param[paramno];
		break;
	case(RANGE_VLO):
	case(RANGE_VHI):
	case(RANGE_VHILO):
		dz->param[rangeno] = dz->param[hparamno] - dz->param[paramno];
		/* fall thro */
	case(RANGED):
		val = (drand48() * dz->param[rangeno]) + dz->param[paramno];
		break;
	}    
	return(val);
}

/******************************* READ_SAMPS_SAUSAGE *******************************/

int read_samps_sausage(int firsttime,dataptr dz)
{
	int exit_status, n;
	int samps_read = INT_MAX;	/* i.e. larger than possible */
	for(n=0;n<dz->infilecnt;n++) {
		if(firsttime) {
			if((exit_status = read_a_specific_large_buf(n,dz))<0)
				return(exit_status);
			samps_read = min(samps_read,dz->ssampsread);
		} else {
			if((exit_status = read_a_specific_normal_buf_with_wraparound(n,dz))<0)
				return(exit_status);
			samps_read = min(samps_read,dz->ssampsread);
		}
	}
	if(samps_read==INT_MAX) {           // RWD 06-2019 was LONG_MAX
		sprintf(errstr,"Problem reading sound buffers.\n");
		return(PROGRAM_ERROR);
	}
	if(firsttime)
		dz->iparam[SAMPS_IN_INBUF] = dz->ssampsread;
	else
		dz->iparam[SAMPS_IN_INBUF] = dz->ssampsread + dz->iparam[GRS_BUF_SMPXS];
	return(FINISHED);
}

/*************************** READ_A_SPECIFIC_LARGE_BUF ******************************/

#ifndef MULTICHAN

int read_a_specific_large_buf(int j,dataptr dz)
{
	long bigbufsize = dz->buflen;	   /* RWD odd one, this... */
	long n, m, k, samps_read;
	int bufno = SAUS_BUF(j);
	bigbufsize += dz->iparam[GRS_BUF_SMPXS];
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		bigbufsize *= 2;
		if((samps_read = fgetfbufEx(dz->sampbuf[SAUS_SBUF],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read samps from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
		bigbufsize /= 2;			   /* RWD still in samps...buflen + smpxs */
		dz->ssampsread /= 2;
		switch(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		case(1):
			for(n=0,m=0;n<dz->ssampsread;n++,m+=2)
				(dz->sampbuf[bufno])[n] = (dz->sampbuf[SAUS_SBUF])[m];
			break;
		case(2):
			for(n=0,m=1;n<dz->ssampsread;n++,m+=2)
				(dz->sampbuf[bufno])[n] = (dz->sampbuf[SAUS_SBUF])[m];
			break;
		case(BOTH_CHANNELS):
			for(n=0,m=0,k=1;n<dz->ssampsread;n++,m+=2,k+=2)
				(dz->sampbuf[bufno])[n] = (float)(((dz->sampbuf[SAUS_SBUF])[m] + (dz->sampbuf[SAUS_SBUF])[k])* 0.5f);
			break;
		}
	} else {
		if((samps_read = fgetfbufEx(dz->sampbuf[bufno],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read samps from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
	}
	/* dz->bigbufsize -= dz->iparam[GRS_BUF_XS]; */ /* leaves dz->buflen unchanged, which is fine */
	return(FINISHED);
}

#else

int read_a_specific_large_buf(int j,dataptr dz)
{
	long bigbufsize = dz->buflen;	   /* RWD odd one, this... */
	long n, m, samps_read, ibufcnt;
	int bufno = SAUS_BUF(j), chans = dz->infile->channels;
	double sum;
	bigbufsize += dz->iparam[GRS_BUF_SMPXS];
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		bigbufsize *= chans;
		if((samps_read = fgetfbufEx(dz->sampbuf[SAUS_SBUF],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read samps from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
		bigbufsize /= chans;			   /* RWD still in samps...buflen + smpxs */
		dz->ssampsread /= chans;
		switch(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		case(ALL_CHANNELS):
			ibufcnt = 0;
			for(n=0;n<dz->ssampsread;n++) {
				sum = 0.0;
				for(m=0;m<chans;m++)
					sum += dz->sampbuf[GRS_SBUF][ibufcnt++];
				dz->sampbuf[bufno][n] = (float)(sum/(double)chans);
			}
			break;
		default:
			for(n=0,m=dz->iparam[GRS_CHAN_TO_XTRACT];n<dz->ssampsread;n++,m+=chans)
				dz->sampbuf[bufno][n] = dz->sampbuf[GRS_SBUF][m];
			break;
		}
	} else {
		if((samps_read = fgetfbufEx(dz->sampbuf[bufno],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read samps from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
	}
	return(FINISHED);
}

#endif

/*************************** READ_A_SPECIFIC_NORMAL_BUF_WITH_WRAPAROUND ******************************/

#ifndef MULTICHAN

int read_a_specific_normal_buf_with_wraparound(int j,dataptr dz)
{
	long bigbufsize = dz->buflen;
	long n,m,k,samps_read;
	int bufno  = SAUS_BUF(j);
	int ibufno = SAUS_IBUF(j);
	memmove((char *)dz->sampbuf[bufno],(char *)(dz->sbufptr[bufno]),
		dz->iparam[GRS_BUF_SMPXS] * sizeof(float));
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		bigbufsize *= 2;
		if((samps_read = fgetfbufEx(dz->sampbuf[SAUS_SBUF],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
		bigbufsize /= 2;
		dz->ssampsread /= 2;
		switch(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		case(1):
			for(n=0,m=0;n<dz->ssampsread;n++,m+=2)
				(dz->sampbuf[ibufno])[n] = (dz->sampbuf[GRS_SBUF])[m];
			break;
		case(2):
			for(n=0,m=1;n<dz->ssampsread;n++,m+=2)
				(dz->sampbuf[ibufno])[n] = (dz->sampbuf[GRS_SBUF])[m];
			break;
		case(BOTH_CHANNELS):
			for(n=0,m=0,k=1;n<dz->ssampsread;n++,m+=2,k+=2)
				(dz->sampbuf[ibufno])[n] = (float)(((dz->sampbuf[GRS_SBUF])[m] + (dz->sampbuf[GRS_SBUF])[k])* 0.5f);
			break;
		}
	} else {
		if((samps_read = fgetfbufEx(dz->sampbuf[ibufno],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
	}
	return(FINISHED);
}

#else

int read_a_specific_normal_buf_with_wraparound(int j,dataptr dz)
{
	long bigbufsize = dz->buflen;
	long n,m,samps_read, ibufcnt;
	double sum;
	int bufno  = SAUS_BUF(j);
	int ibufno = SAUS_IBUF(j), chans = dz->infile->channels;
	memmove((char *)dz->sampbuf[bufno],(char *)(dz->sbufptr[bufno]),dz->iparam[GRS_BUF_SMPXS] * sizeof(float));
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		bigbufsize *= chans;
		if((samps_read = fgetfbufEx(dz->sampbuf[SAUS_SBUF],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
		bigbufsize /= chans;
		dz->ssampsread /= chans;
		switch(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		case(ALL_CHANNELS):
			ibufcnt = 0;
			for(n=0;n<dz->ssampsread;n++) {
				sum = 0.0;
				for(m=0;m<chans;m++)
					sum += dz->sampbuf[GRS_SBUF][ibufcnt++];
				dz->sampbuf[ibufno][n] = (float)(sum/(double)chans);
			}
			break;
		default:
			for(n=0,m=dz->iparam[GRS_CHAN_TO_XTRACT];n<dz->ssampsread;n++,m+=chans)
				dz->sampbuf[ibufno][n] = dz->sampbuf[GRS_SBUF][m];
			break;
		}
	} else {
		if((samps_read = fgetfbufEx(dz->sampbuf[ibufno],bigbufsize,dz->ifd[j],0))<0) {
			sprintf(errstr,"Failed to read from file %d.\n",j+1);
			return(SYSTEM_ERROR);
		}
		dz->ssampsread = samps_read;
	}
	return(FINISHED);
}

#endif

/*************************** BAKTRAK_SAUSAGE ******************************
 *
 *     <------------ baktrak(b) ---------------->
 *
 *     <--(b-x)-->
 *		  		  <------------ x -------------->
 *		 		 |-------- current bufer --------|
 *
 *     |-------- new buffer --------|
 *      <------------x------------->
 *				
 */

#ifndef MULTICHAN

int baktrak_sausage(int thissnd,long samptotal,int absiicnt_per_chan,float **iiptr,dataptr dz)
{
	int  exit_status;
	long bktrk, new_position;
	unsigned long reset_size = dz->buflen + dz->iparam[GRS_BUF_SMPXS];
//TW I agree, this is FINE!!, and, with sndseekEx, algo is more efficient
//	whenever the search-range is relatively small (and equally efficient otherwise)
	bktrk = samptotal - (absiicnt_per_chan * dz->iparam[GRS_INCHANS]);
	*iiptr      = dz->sampbuf[SAUS_BUF(thissnd)];

	if((new_position = samptotal - bktrk)<0) {
		fprintf(stdout,"WARNING: Non-fatal program error:\nRange arithmetic problem - 2, in baktraking.\n");
		fflush(stdout);
		new_position = 0;
		*iiptr = dz->sampbuf[SAUS_BUF(thissnd)];
	}
    if(dz->iparam[GRS_CHAN_TO_XTRACT])   /* IF we've converted from STEREO file */
		new_position *= 2;
	if(sndseekEx(dz->ifd[thissnd],new_position,0)<0) {
		sprintf(errstr,"sndseek error: baktrak_sausage()\n");
		return(SYSTEM_ERROR);
	}
	memset((char *)dz->sampbuf[SAUS_BUF(thissnd)],0,reset_size * sizeof(float));
    if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		reset_size *= 2;
    	memset((char *)dz->sampbuf[SAUS_SBUF],0,reset_size * sizeof(float));
	}
	if((exit_status = read_a_specific_large_buf(thissnd,dz))<0)
		return(exit_status);
	dz->iparam[SAMPS_IN_INBUF] = dz->ssampsread;
	return(FINISHED);
}

#else

int baktrak_sausage(int thissnd,long samptotal,int absiicnt_per_chan,float **iiptr,dataptr dz)
{
	int  exit_status,  chans = dz->infile->channels;
	long bktrk, new_position;
	unsigned long reset_size = dz->buflen + dz->iparam[GRS_BUF_SMPXS];
	bktrk = samptotal - (absiicnt_per_chan * dz->iparam[GRS_INCHANS]);
	*iiptr      = dz->sampbuf[SAUS_BUF(thissnd)];

	if((new_position = samptotal - bktrk)<0) {
		fprintf(stdout,"WARNING: Non-fatal program error:\nRange arithmetic problem - 2, in baktraking.\n");
		fflush(stdout);
		new_position = 0;
		*iiptr = dz->sampbuf[SAUS_BUF(thissnd)];
	}
    if(dz->iparam[GRS_CHAN_TO_XTRACT])   /* IF we've converted from MULTICHAN file */
		new_position *= chans;
	if(sndseekEx(dz->ifd[thissnd],new_position,0)<0) {
		sprintf(errstr,"sndseek error: baktrak_sausage()\n");
		return(SYSTEM_ERROR);
	}
	memset((char *)dz->sampbuf[SAUS_BUF(thissnd)],0,reset_size * sizeof(float));
    if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		reset_size *= chans;
    	memset((char *)dz->sampbuf[SAUS_SBUF],0,reset_size * sizeof(float));
	}
	if((exit_status = read_a_specific_large_buf(thissnd,dz))<0)
		return(exit_status);
	dz->iparam[SAMPS_IN_INBUF] = dz->ssampsread;
	return(FINISHED);
}

#endif

/****************************** RESET_SAUSAGE *******************************/

#ifndef MULTICHAN

int reset_sausage(int thissnd,int resetskip,dataptr dz)
{
	int exit_status;
	unsigned long reset_size = dz->buflen + dz->iparam[GRS_BUF_SMPXS];

	memset((char *)dz->sampbuf[SAUS_BUF(thissnd)],0,reset_size * sizeof(float));
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		reset_size *= 2;
    	memset((char *)dz->sampbuf[SAUS_SBUF],0,reset_size * sizeof(float));
	}
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) /* If we've converted from a stereo file */
		resetskip *= 2;
	if(sndseekEx(dz->ifd[thissnd],resetskip,0)<0) {
		sprintf(errstr,"sndseek error: reset_sausage()\n");
		return(SYSTEM_ERROR);
	}
	if((exit_status = read_a_specific_large_buf(thissnd,dz))<0)
		return(exit_status);
	dz->iparam[SAMPS_IN_INBUF] = dz->ssampsread;  
	return(FINISHED);
}

#else

int reset_sausage(int thissnd,int resetskip,dataptr dz)
{
	int exit_status, chans = dz->infile->channels;
	unsigned long reset_size = dz->buflen + dz->iparam[GRS_BUF_SMPXS];

	memset((char *)dz->sampbuf[SAUS_BUF(thissnd)],0,reset_size * sizeof(float));
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) {
		reset_size *= chans;
    	memset((char *)dz->sampbuf[SAUS_SBUF],0,reset_size * sizeof(float));
	}
	if(dz->iparam[GRS_CHAN_TO_XTRACT]) /* If we've converted from a stereo file */
		resetskip *= chans;
	if(sndseekEx(dz->ifd[thissnd],resetskip,0)<0) {
		sprintf(errstr,"sndseek error: reset_sausage()\n");
		return(SYSTEM_ERROR);
	}
	if((exit_status = read_a_specific_large_buf(thissnd,dz))<0)
		return(exit_status);
	dz->iparam[SAMPS_IN_INBUF] = dz->ssampsread;  
	return(FINISHED);
}

#endif

/****************************** GET_NEXT_INSND *******************************/

int get_next_insnd(dataptr dz)
{
	if(dz->itemcnt <= 0) {
		perm_sausage(dz->infilecnt,dz);
		dz->itemcnt = dz->infilecnt;
	}
	dz->itemcnt--;
	return(dz->iparray[SAUS_PERM][dz->itemcnt]);
}

/****************************** PERM_SAUSAGE *******************************/

void perm_sausage(int cnt,dataptr dz)
{
	int n, t;
	for(n=0;n<cnt;n++) {
		t = (int)(drand48() * (double)(n+1));	 /* TRUNCATE */
		if(t==n)
			prefix(n,cnt-1,dz);
		else
			insert(n,t,cnt-1,dz);
	}
}

/****************************** INSERT ****************************/

void insert(int n,int t,int cnt_less_one,dataptr dz)
{   
	shuflup(t+1,cnt_less_one,dz);
	dz->iparray[SAUS_PERM][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefix(int n,int cnt_less_one,dataptr dz)
{   
	shuflup(0,cnt_less_one,dz);
	dz->iparray[SAUS_PERM][0] = n;
}

/****************************** SHUFLUP ****************************/

void shuflup(int k,int cnt_less_one,dataptr dz)
{   
	int n;
	for(n = cnt_less_one; n > k; n--)
		dz->iparray[SAUS_PERM][n] = dz->iparray[SAUS_PERM][n-1];
}

/****************************** SAUSAGE_PREPROCESS ****************************/

int sausage_preprocess(dataptr dz)
{
	if((dz->iparray[SAUS_PERM] = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for permutation array for sausage.\n");
		return(MEMORY_ERROR);
	}
	dz->iparray[SAUS_PERM][0] = dz->infilecnt + 1;	/* impossible value to initialise perm */

	return create_sized_outfile(dz->outfilename,dz);



	/*return(FINISHED);*/
}

/*************************** CREATE_SAUSAGE_BUFFERS **************************/

#ifndef MULTICHAN

int create_sausage_buffers(dataptr dz)
{	
	long standard_block = (long)Malloc(-1);
	long this_bloksize;
	int   exit_status, n;
	int   convert_to_stereo = FALSE, overall_size, bufdivisor = 0;
	float *tailend;
	long  stereo_buflen = 0, stereo_bufxs = 0, outbuflen;
//TW All buffers are in floats, so this not needed
//	int   lfactor = sizeof(long)/sizeof(float), n;
	if(dz->iparray[GRS_FLAGS][G_SPACE_FLAG])
		convert_to_stereo = TRUE;							
	if((dz->extrabuf[GRS_GBUF] = (float *)malloc(dz->iparam[GRS_GLBUF_SMPXS] * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create grain buffer.\n");	 /* GRAIN BUFFER */
		return(MEMORY_ERROR);
	}
				/* CALCULATE NUMBER OF BUFFER CHUNKS REQUIRED : bufdivisor */

	if(dz->iparam[GRS_CHANNELS]>0)
		bufdivisor += 2;							/* 2 for stereo-infile, before reducing to mono */
//TW All buffers are in floats
//	bufdivisor += dz->infilecnt + 1 + lfactor;		/* infilecnt IN mono, 1 OUT,   1 long OUT  */
	bufdivisor += dz->infilecnt + 2;				/* infilecnt IN mono, 1 OUT,   1 LBUF OUT  */
	if(convert_to_stereo)
//TW All buffers are in floats
//		bufdivisor += 1 + lfactor;					/*                    2nd OUT, 2nd long OUT */
		bufdivisor += 2;							/*                    2nd OUT, 2nd LBUF OUT */


	
	this_bloksize = standard_block;
	for(;;) {
		if((exit_status = grab_an_appropriate_block_of_sausage_memory(&this_bloksize,standard_block,bufdivisor,dz))<0)
			return(exit_status);

					/* CALCULATE AND ALLOCATE TOTAL MEMORY REQUIRED : overall_size */
	
		overall_size = (dz->buflen * bufdivisor) + (dz->iparam[GRS_BUF_SMPXS] * dz->infilecnt) + dz->iparam[GRS_LBUF_SMPXS];
		if(dz->iparam[GRS_CHANNELS])				
			overall_size += 2 * dz->iparam[GRS_BUF_SMPXS];					/* IF stereo, also allow for bufxs in stereo inbuf */

//TW	if(overall_size<0) 
		if(overall_size * sizeof(float)<0) {
			sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");	/* arithmetic overflow */
			return(MEMORY_ERROR);
		}
		if((dz->bigbuf=(float *)malloc(overall_size * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
			return(MEMORY_ERROR);
		}
					/* SET SIZE OF inbuf, outbuf, AND Lbuf (FOR CALCS) */
		outbuflen   = dz->buflen;
		if(convert_to_stereo)								/* For stereo out, outbuflen is double length of inbuf */
			outbuflen *= 2;
		if(dz->iparam[GRS_CHANNELS]) {
			stereo_buflen = dz->buflen * 2;
			stereo_bufxs  = dz->iparam[GRS_BUF_SMPXS] * 2;
		}										 
		dz->iparam[GRS_LONGS_BUFLEN] = outbuflen;			/* Longs buffer is same size as obuf */
		if(dz->iparam[GRS_LBUF_SMPXS] > dz->iparam[GRS_LONGS_BUFLEN])
			continue;
		break;
	}
				/* DIVIDE UP ALLOCATED MEMORY IN SPECIALISED BUFFERS */	
																	
	if(dz->iparam[GRS_CHANNELS]) {				 					/* sbuf : extra stereo input buffer, if required */						 
		dz->sampbuf[SAUS_SBUF]   = dz->bigbuf;									 
		dz->sampbuf[SAUS_BUF(0)] = dz->sampbuf[SAUS_SBUF] + stereo_buflen + stereo_bufxs;			 
	} else												 
		dz->sampbuf[SAUS_BUF(0)] = dz->bigbuf;						
	dz->sbufptr[SAUS_BUF(0)]  = dz->sampbuf[SAUS_BUF(0)] + dz->buflen;	
	dz->sampbuf[SAUS_IBUF(0)] = dz->sampbuf[SAUS_BUF(0)] + dz->iparam[GRS_BUF_SMPXS]; 
	tailend     		      = dz->sbufptr[SAUS_BUF(0)] + dz->iparam[GRS_BUF_SMPXS];  
	for(n=1;n<dz->infilecnt;n++) {
		dz->sampbuf[SAUS_BUF(n)]  = tailend;								   /* Lbuf: buffer for calculations */
		dz->sbufptr[SAUS_BUF(n)]  = dz->sampbuf[SAUS_BUF(n)] + dz->buflen;	
		dz->sampbuf[SAUS_IBUF(n)] = dz->sampbuf[SAUS_BUF(n)] + dz->iparam[GRS_BUF_SMPXS]; 
		tailend     		      = dz->sbufptr[SAUS_BUF(n)] + dz->iparam[GRS_BUF_SMPXS]; 
	}
	dz->fptr[GRS_LBUF]     = tailend;								   
	dz->fptr[GRS_LBUFEND]  = dz->fptr[GRS_LBUF] + dz->iparam[GRS_LONGS_BUFLEN];
	dz->fptr[GRS_LTAILEND] = dz->fptr[GRS_LBUFEND] + dz->iparam[GRS_LBUF_SMPXS];
	dz->fptr[GRS_LBUFMID]  = dz->fptr[GRS_LBUF]    + dz->iparam[GRS_LBUF_SMPXS];
	dz->sampbuf[SAUS_OBUF]  = /*(short *)*/(dz->fptr[GRS_LTAILEND]);		 		    

								/* INITIALISE BUFFERS */

	memset((char *)dz->bigbuf,0,overall_size * sizeof(float));
	return(FINISHED);
}

#else

int create_sausage_buffers(dataptr dz)
{
	long standard_block = (long)Malloc(-1);
	long this_bloksize;
	int   exit_status, n, chans = dz->infile->channels;
	int   overall_size, bufdivisor = 0;
	float *tailend;
	long  multichan_buflen = 0, multichan_bufxs = 0, outbuflen;
	if((dz->extrabuf[GRS_GBUF] = (float *)malloc(dz->iparam[GRS_GLBUF_SMPXS] * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to create grain buffer.\n");	 /* GRAIN BUFFER */
		return(MEMORY_ERROR);
	}
				/* CALCULATE NUMBER OF BUFFER CHUNKS REQUIRED : bufdivisor */

	if(dz->iparam[GRS_CHANNELS]>0)
		bufdivisor += chans;							/* chans for multichan-infile, before reducing to mono */
	bufdivisor += dz->infilecnt;						/* infilecnt IN mono, 1 OUT,   1 LBUF OUT  */
	for(n=0;n<dz->outfile->channels;n++)
		bufdivisor += 1 + sizeof(long)/sizeof(float);	/* 1 float and 1 long buf for each input channel */
	this_bloksize = standard_block;
	for(;;) {
		if((exit_status = grab_an_appropriate_block_of_sausage_memory(&this_bloksize,standard_block,bufdivisor,dz))<0)
			return(exit_status);

					/* CALCULATE AND ALLOCATE TOTAL MEMORY REQUIRED : overall_size */
	
		overall_size = (dz->buflen * bufdivisor) + (dz->iparam[GRS_BUF_SMPXS] * dz->infilecnt) + dz->iparam[GRS_LBUF_SMPXS];
		if(dz->iparam[GRS_CHANNELS])				
			overall_size += chans * dz->iparam[GRS_BUF_SMPXS];					/* IF multichan, also allow for bufxs in multichan inbuf */

		if(overall_size * sizeof(float)<0) {
			sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");	/* arithmetic overflow */
			return(MEMORY_ERROR);
		}
		if((dz->bigbuf=(float *)malloc(overall_size * sizeof(float)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
			return(MEMORY_ERROR);
		}
					/* SET SIZE OF inbuf, outbuf, AND Lbuf (FOR CALCS) */
		outbuflen = dz->buflen;

		outbuflen *= dz->out_chans;
		if(dz->iparam[GRS_CHANNELS]) {
			multichan_buflen = dz->buflen * chans;
			multichan_bufxs  = dz->iparam[GRS_BUF_SMPXS] * chans;
		}										 
		dz->iparam[GRS_LONGS_BUFLEN] = outbuflen;			/* Longs buffer is same size as obuf */
		if(dz->iparam[GRS_LBUF_SMPXS] > dz->iparam[GRS_LONGS_BUFLEN])
			continue;
		break;
	}
				/* DIVIDE UP ALLOCATED MEMORY IN SPECIALISED BUFFERS */	
																	
	if(dz->iparam[GRS_CHANNELS]) {				 					/* sbuf : extra stereo input buffer, if required */						 
		dz->sampbuf[SAUS_SBUF]   = dz->bigbuf;									 
		dz->sampbuf[SAUS_BUF(0)] = dz->sampbuf[SAUS_SBUF] + multichan_buflen + multichan_bufxs;			 
	} else												 
		dz->sampbuf[SAUS_BUF(0)] = dz->bigbuf;						
	dz->sbufptr[SAUS_BUF(0)]  = dz->sampbuf[SAUS_BUF(0)] + dz->buflen;	
	dz->sampbuf[SAUS_IBUF(0)] = dz->sampbuf[SAUS_BUF(0)] + dz->iparam[GRS_BUF_SMPXS]; 
	tailend     		      = dz->sbufptr[SAUS_BUF(0)] + dz->iparam[GRS_BUF_SMPXS];  
	for(n=1;n<dz->infilecnt;n++) {
		dz->sampbuf[SAUS_BUF(n)]  = tailend;								   /* Lbuf: buffer for calculations */
		dz->sbufptr[SAUS_BUF(n)]  = dz->sampbuf[SAUS_BUF(n)] + dz->buflen;	
		dz->sampbuf[SAUS_IBUF(n)] = dz->sampbuf[SAUS_BUF(n)] + dz->iparam[GRS_BUF_SMPXS]; 
		tailend     		      = dz->sbufptr[SAUS_BUF(n)] + dz->iparam[GRS_BUF_SMPXS]; 
	}
	dz->fptr[GRS_LBUF]     = tailend;								   
	dz->fptr[GRS_LBUFEND]  = dz->fptr[GRS_LBUF] + dz->iparam[GRS_LONGS_BUFLEN];
	dz->fptr[GRS_LTAILEND] = dz->fptr[GRS_LBUFEND] + dz->iparam[GRS_LBUF_SMPXS];
	dz->fptr[GRS_LBUFMID]  = dz->fptr[GRS_LBUF]    + dz->iparam[GRS_LBUF_SMPXS];
	dz->sampbuf[SAUS_OBUF]  = /*(short *)*/(dz->fptr[GRS_LTAILEND]);		 		    

								/* INITIALISE BUFFERS */

	memset((char *)dz->bigbuf,0,overall_size * sizeof(float));
	return(FINISHED);
}

#endif

/*  INPUT BUFFERS :-		
 *
 *	|-----------BUFLEN-----------|
 *
 *	buf      ibuf		   bufend    tailend
 *	|_________|__________________|buf_smpxs|  ..... (obuf->)
 *					             /
 *	|buf_smpxs|	          <<-COPY_________/
 *
 *		      |-----------BUFLEN-----------|
 *
 *
 *
 *  OUTPUT LONGS BUFFER:-
 *
 *	Lbuf	   Lbufmid	  Lbufend
 *	|____________|_______________|_Lbuf_smpxs_|
 *				                 /
 *	|_Lbuf_smpxs_|         <<-COPY___________/	
 *
 */

/*************************** GRAB_AN_APPROPRIATE_BLOCK_OF_SAUSAGE_MEMORY **************************/

#ifndef MULTICHAN

int grab_an_appropriate_block_of_sausage_memory(long *this_bloksize,long standard_block,int bufdivisor,dataptr dz)
{
	/*int  sector_blok;*/
	do {
//TW	if((dz->buflen = *this_bloksize)< 0) {	/* arithmetic overflow */
		if((dz->buflen = *this_bloksize) * sizeof(float) < 0) {	/* arithmetic overflow */
			sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
			return(MEMORY_ERROR);
		}
		*this_bloksize += standard_block;
				/* CALCULATE SIZE OF BUFFER REQUIRED : dz->bigbufsize */

		dz->buflen -= (dz->infilecnt * dz->iparam[GRS_BUF_SMPXS]) + dz->iparam[GRS_LBUF_SMPXS];	
															/* Allow for overflow areas */
		if(dz->iparam[GRS_CHANNELS])				
			dz->buflen -= 2 * dz->iparam[GRS_BUF_SMPXS];	/* Allow for overflow space in additional stereo inbuf */
		dz->buflen /= bufdivisor;						/* get unit buffersize */
	/*	sector_blok = SECSIZE;	*/							/* Read and write buf sizes must be multiples of SECSIZE */
		if(dz->iparam[GRS_CHANNELS])						/* If reading stereo: 2* SECSIZE reduces to single mono SECSIZE */
	/*		sector_blok *= 2;		*/						/* So dz->bigbufsize must be a multiple of (2 * SECSIZE) */
			dz->buflen = (dz->buflen / STEREO) * STEREO;	   /*RWD: */
	
	/*	dz->buflen = (dz->bigbufsize/sector_blok) * sector_blok;*/
	} while(dz->buflen <= 0);
	return(FINISHED);
}

#else

int grab_an_appropriate_block_of_sausage_memory(long *this_bloksize,long standard_block,int bufdivisor,dataptr dz)
{
	do {
		if((dz->buflen = *this_bloksize) * sizeof(float) < 0) {	/* arithmetic overflow */
			sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
			return(MEMORY_ERROR);
		}
		*this_bloksize += standard_block;
		dz->buflen -= (dz->infilecnt * dz->iparam[GRS_BUF_SMPXS]) + dz->iparam[GRS_LBUF_SMPXS];	
															/* Allow for overflow areas */
		if(dz->iparam[GRS_CHANNELS])				
			dz->buflen -= dz->infile->channels * dz->iparam[GRS_BUF_SMPXS];	/* Allow for overflow space in additional multichan inbuf */
		dz->buflen /= bufdivisor;							/* get unit buffersize */
		if(dz->iparam[GRS_CHANNELS])						/* If reading multichano: chans * SECSIZE reduces to single mono SECSIZE */
			dz->buflen = (dz->buflen / dz->infile->channels) * dz->infile->channels;
	} while(dz->buflen <= 0);
	return(FINISHED);
}

#endif


#ifdef MULTICHAN

/************************* WRITE_MULTICHAN_GRAIN ****************************/

int write_multichan_grain(double rpos,int chana,int chanb,float **maxwrite,float **Fptr,float **FFptr,int gsize_per_chan,int *nctr,dataptr dz)
{
	int exit_status;
	long   n, to_doo, exess;
	long   real_gsize = gsize_per_chan * dz->out_chans;
	double lpos;
	double adjust = dehole(rpos);
	float  *writend, *fptr = *Fptr, *ffptr = *FFptr;
	float  *gbufptr = dz->extrabuf[GRS_GBUF];
	lpos  = (1.0 - rpos) * adjust;
	rpos *= adjust;
	chana--;
	chanb--;
	if((writend = (ffptr + real_gsize)) > *maxwrite)
		*maxwrite = writend;
	if(ffptr + real_gsize >= dz->fptr[GRS_LTAILEND]) {
		to_doo = dz->fptr[GRS_LTAILEND] - ffptr;
		if((exess  = real_gsize - to_doo) >= dz->iparam[GRS_LONGS_BUFLEN]) {
 			sprintf(errstr,"Array overflow: write_stereo_grain()\n");
			return(PROGRAM_ERROR);
		}
		to_doo /= dz->out_chans;
		for(n = 0; n < to_doo; n++) {
			*(ffptr + chana) += (float) (*gbufptr * lpos);
			*(ffptr + chanb) += (float) (*gbufptr * rpos);
			gbufptr++;
			ffptr += dz->out_chans;
		}
		if((exit_status = write_samps_granula(dz->iparam[GRS_LONGS_BUFLEN],nctr,dz))<0)
			return(exit_status);
		memmove((char *)dz->fptr[GRS_LBUF],(char *)dz->fptr[GRS_LBUFEND],
			(size_t)dz->iparam[GRS_LBUF_SMPXS] * sizeof(float));
		memset((char *)dz->fptr[GRS_LBUFMID],0,dz->iparam[GRS_LONGS_BUFLEN] * sizeof(float));
		ffptr -= dz->iparam[GRS_LONGS_BUFLEN];
		fptr  -= dz->iparam[GRS_LONGS_BUFLEN];
		*maxwrite -= dz->buflen; /* APR 1996 */
		exess /= dz->out_chans;
		for(n = 0; n < exess; n++) {
			*(ffptr + chana) += (float) (*gbufptr * lpos);
			*(ffptr + chanb) += (float) (*gbufptr * rpos);
			gbufptr++;
			ffptr += dz->out_chans;
		}
		*Fptr  = fptr;
		*FFptr = ffptr;
		return(FINISHED);
	}
	for(n=0;n<gsize_per_chan;n++) {
		*(ffptr + chana) += (float) (*gbufptr * lpos);
		*(ffptr + chanb) += (float) (*gbufptr * rpos);
		gbufptr++;
		ffptr += dz->out_chans;
	}
	*Fptr  = fptr;
	*FFptr = ffptr;
	return(FINISHED);
}

#endif
