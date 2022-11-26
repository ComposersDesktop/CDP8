/* Copyright (c) 2009, 2014 Richard Dobson

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

/* POST_BOOK!*/
/* RWD Oct 2009: added MC_CUBE to listy of supported speaker layouts */
/* corrected 7.1 speaker value (hex) */
/* Aug 2012 corrected SPKRS_MONO value */
/* Nov 2013: added SPKRS_6_1 and MC_SURR_6_1 to list */

#ifndef __RIFFWAV_H_INCLUDED
#define __RIFFWAV_H_INCLUDED

/* Revision 16th September 2003: added TPDF dither support */
/* revision Dec 2005: support new B-Format extensions */
/* RWD Nov 1 2006: extended with double sread/write for research purposes! */
/* July 2009: attempt portability with 64bit platforms */
/* unable to test Win64 yet - but the symbol to check is simply _WIN64  */
#ifdef __GNUC__ 
# ifdef __LP64__
#   define CPLONG64
# endif
#endif
/* failing that, manually define CPLONG64 in makefile or project setting */
#ifdef CPLONG64
// all this effort is for readPeaks() and ctime only!
# define MYLONG int
#else
# define MYLONG long
#endif

#ifdef __cplusplus
extern "C" {	
#endif	   
/* compatible with <windows.h> */
#ifndef DWORD
typedef unsigned MYLONG DWORD;
typedef unsigned short WORD;
#endif
/* NB: AIFF spec always illustrates chunksize as (signed) long; 
   even though nFrames is always unsigned long!
   So we make everything DWORD here.
 */

/* the file sample formats we could support */

typedef enum {
	PSF_SAMP_UNKNOWN	=	0,
	PSF_SAMP_8,				   /* not yet supported! */
	PSF_SAMP_16,
	PSF_SAMP_24,
	PSF_SAMP_32,
	PSF_SAMP_IEEE_FLOAT
} psf_stype;

/* the file format */
/* currently based only on file extension. 
   To be friendly, we should parse the header to get the format.
*/
typedef enum {
	PSF_FMT_UNKNOWN = 0,		/* e.g if no extension given. This could also signify 'raw' */
	PSF_STDWAVE,
	PSF_WAVE_EX,
	PSF_AIFF,
	PSF_AIFC
} psf_format;

/* provisional stab at error codes */
enum {
	PSF_E_NOERROR		= 0,
	PSF_E_CANT_OPEN		= -1,
	PSF_E_CANT_CLOSE	= -2,
	PSF_E_CANT_WRITE	= -3,
	PSF_E_CANT_READ		= -4,
	PSF_E_NOT_WAVE		= -5,
	PSF_E_BAD_TYPE		= -6,
	PSF_E_BAD_FORMAT	= -7,
	PSF_E_UNSUPPORTED	= -8,
	PSF_E_NOMEM			= -9,
	PSF_E_BADARG		= -10,
	PSF_E_CANT_SEEK		= -11,
	PSF_E_TOOMANYFILES  = -12,
	PSF_E_FILE_READONLY = -13,
	PSF_E_SEEK_BEYOND_EOF = -14
};

#define NUM_SPEAKER_POSITIONS (18)

#define SPEAKER_FRONT_LEFT				0x1
#define SPEAKER_FRONT_RIGHT				0x2
#define SPEAKER_FRONT_CENTER			0x4
#define SPEAKER_LOW_FREQUENCY			0x8
#define SPEAKER_BACK_LEFT				0x10
#define SPEAKER_BACK_RIGHT				0x20
#define SPEAKER_FRONT_LEFT_OF_CENTER	0x40
#define SPEAKER_FRONT_RIGHT_OF_CENTER	0x80
#define SPEAKER_BACK_CENTER				0x100
#define SPEAKER_SIDE_LEFT				0x200
#define SPEAKER_SIDE_RIGHT				0x400
#define SPEAKER_TOP_CENTER				0x800
#define SPEAKER_TOP_FRONT_LEFT			0x1000
#define SPEAKER_TOP_FRONT_CENTER		0x2000
#define SPEAKER_TOP_FRONT_RIGHT			0x4000
#define SPEAKER_TOP_BACK_LEFT			0x8000
#define SPEAKER_TOP_BACK_CENTER			0x10000
#define SPEAKER_TOP_BACK_RIGHT			0x20000
#define SPEAKER_RESERVED      			0x80000000

/* my extras*/
#define SPKRS_UNASSIGNED	(0)
#define SPKRS_MONO			(0x00000004)
#define SPKRS_STEREO		(0x00000003)
#define SPKRS_GENERIC_QUAD	(0x00000033)
#define SPKRS_SURROUND_LCRS	(0x00000107)
#define SPKRS_SURR_5_0      (0x00000037)
#define SPKRS_DOLBY5_1		(0x0000003f)
#define SPKRS_6_1			(0x0000013f)
#define SPKRS_7_1           (0x000000ff)
#define SPKRS_CUBE          (SPKRS_GENERIC_QUAD | SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT | SPEAKER_TOP_BACK_LEFT | SPEAKER_TOP_BACK_RIGHT)
#define SPKRS_ACCEPT_ALL	(0xffffffff)	 /*???? no use for a file*/


/* support for the PEAK chunk */
typedef struct psf_chpeak {	
	float val;
	DWORD pos;   /* OK for all WAVE and AIFF <= 4GB */
} PSF_CHPEAK;

/* second two are speculative at present! */
typedef enum  {PSF_CREATE_RDWR,PSF_CREATE_TEMPORARY,PSF_CREATE_WRONLY} psf_create_mode;
/* the speakerfeed format */
/* MC_WAVE_EX is a possibly temporary one to cover abstruse infile formats! */
typedef enum { STDWAVE,MC_STD,MC_MONO,MC_STEREO,MC_QUAD,MC_LCRS,MC_BFMT,MC_DOLBY_5_1,
				MC_SURR_5_0,MC_SURR_6_1,MC_SURR_7_1,MC_CUBE,MC_WAVE_EX } psf_channelformat;

/* read access support */
/* for psf_sndSeek(); ~should~ map directly to fseek mode flags*/
enum { PSF_SEEK_SET=0,PSF_SEEK_CUR,PSF_SEEK_END};

enum {PSF_DITHER_OFF,PSF_DITHER_TPDF};

/* main structure to define a soundile. Extended props must be asked for separately */
typedef struct psf_props 
{
	int		srate;
	int		chans;	
	psf_stype	samptype;		
	psf_format	format;			
	psf_channelformat chformat;	
	/* probably add more stuff...esp for full WAVE-EX support */
} PSF_PROPS;


/*************** PUBLIC FUNCS */

/* init sfs system. return 0 for success */
int psf_init(void);
/* close sfs-system. Does auto cleanup of open files, etc. return 0 for success */
int psf_finish(void);
/* Create soundfile from props.
   Supports clipping or non-clipping of floats to 0dbFS, 
   set minimum header (or use PEAK)
   returns Sf descriptor >= 0, or some PSF_E_*  on error.
*/
/* using WIN32, it is possible to share for reading, but not under ANSI */
/* maybe we just abandon all that */ 
/* TODO (?): enforce non-destructive creation */
int psf_sndCreate(const char *path,const PSF_PROPS *props, int clip_floats,int minheader,int mode);


/* open existing soundfile. Receive format info in props. Supports auto rescale from PEAK
   data, with floats files. Only RDONLY access supported.
   Return sf descriptor >= 0, or some PSF_E_+ on error.
  */
int psf_sndOpen(const char *path,PSF_PROPS *props, int rescale);
/*snd close. Updates PEAK data if used. return 0 for success, or some PSF_E_+ on error.*/
int psf_sndClose(int sfd);

/* all data read/write is counted in multi-channel sample 'frames', NOT in raw samples.*/

/* get size of file, in m/c frames. Return size, or PSF_E_BADARG on bad sfd  */
int psf_sndSize(int sfd);

/* write m/c frames of floats. this updates internal PEAK data automatically.
   return num frames written, or  some PSF_E_* on error. 
  */
int psf_sndWriteFloatFrames(int sfd, const float *buf, DWORD nFrames);
int psf_sndWriteDoubleFrames(int sfd, const double *buf, DWORD nFrames);
/* as above, with 16bit data */
int psf_sndWriteShortFrames(int sfd, const short *buf, DWORD nFrames);

/* 	get current m/c frame position in file, or PSF_E_BADARG with bad sfd*/    
int psf_sndTell(int sfd);

/* m/c frame wrapper for stdio fseek. return 0 for success. Offset counted in m/c frames.
 seekmode must be one of PSF_SEEK_* options, to ensure correct mapping to SEEK_CUR, etc */
int psf_sndSeek(int sfd,int offset,int seekmode);

/* read m/c sample frames into floats buffer. return nFrames, or some PSF_E_*.
    if file opened with rescale = 1, over-range floats data, as indicated by PEAK chunk,
	is automatically scaled to 0dbFS.
	NB we could add a facility to define 'headroom', as some level below 0dBFS.
 */
int psf_sndReadFloatFrames(int sfd, float *buf, DWORD nFrames);
int psf_sndReadDoubleFrames(int sfd, double *buf, DWORD nFrames);
/* can add ReadShortFrames if we really need it! */

int psf_sndReadPeaks(int sfd,PSF_CHPEAK peakdata[],MYLONG *peaktime);

/* find the soundfile format from the filename extension */
/* currently supported: .wav, .aif, .aiff,.aifc,.afc, .amb */
psf_format psf_getFormatExt(const char *path);


/* set/unset dither. 
   Returns 0 on success, -1 if error (unrecognised type, or read-only)
   no-op for input files
*/

int psf_sndSetDither(int sfd,unsigned int dtype);
/* get current dither setting */
int psf_sndGetDither(int sfd);

/* RWD NEW Nov 2009 */
int psf_speakermask(int sfd);

/* Feb 2010 */
int psf_getWarning(int sfd,const char** warnstring);

/* Sept 2010  return 1 for true, 0 for false */
    /* NB tests up to 32bit size for now! */
int is_legalsize(unsigned long nFrames, const PSF_PROPS *props);
    
#ifdef __cplusplus
}
#endif

#endif
