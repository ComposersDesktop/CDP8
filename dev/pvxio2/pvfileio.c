          
/* pvfileio.c */
/* pvocex format test routines*/
/* Initial version RWD May 2000.
 * All rights reserved: work in progress!
 * Manifestly not a complete API yet!
 * In particular, error returns are kept very simplistic at the moment.
 * (and are not even very consistent yet...)
 * In time, a full set of error values and messages will be developed.
 *
 * NB: the RIFF<WAVE> functions only look for, and accept, a PVOCEX format file.
 * NB also: if windows.h is included anywhere (should be no need in this file,
 *          or in pvfileio.h),
 *          the WAVE_FORMAT~ symbols may need to be #ifndef-ed.
 */

/*   very simple CUSTOM window chunk:
 *
 *  <PVXW><size><data>
 *
 *  where size as usual gives the size of the data in bytes.
 *  the size in samples much match dwWinlen (which may not be the same as N (fft length)
 *  the sample type must be the same as that of the pvoc data itself
 *  (only floatsams supported so far)
 *  values must be normalized to peak of 1.0
 */
//#ifdef WINDOWS
//#include "stdafx.h"
//#endif

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#endif
#ifdef unix
#include <unistd.h>
#define O_BINARY (0)
#define _S_IWRITE S_IWRITE
#define _S_IREAD  S_IREAD
#endif

#ifdef _DEBUG
#include <assert.h>
#endif
#include "pvdefs.h"
#include "pvfileio.h"

/* NB probably no good on 64bit platforms */
#define REVDWBYTES(t)   ( (((t)&0xff) << 24) | (((t)&0xff00) << 8) | (((t)&0xff0000) >> 8) | (((t)>>24) & 0xff) )
#define REVWBYTES(t)    ( (((t)&0xff) << 8) | (((t)>>8) &0xff) )
#define TAG(a,b,c,d)    ( ((a)<<24) | ((b)<<16) | ((c)<<8) | (d) )
#ifndef WAVE_FORMAT_EXTENSIBLE
# define WAVE_FORMAT_EXTENSIBLE (0xFFFE)
#endif
#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM (0x0001)
#endif
#ifndef WAVE_FORMAT_IEEE_FLOAT
#define WAVE_FORMAT_IEEE_FLOAT (0x0003)
#endif

const GUID KSDATAFORMAT_SUBTYPE_PVOC = {
                    0x8312b9c2,
                    0x2e6e,
                    0x11d4,
                    { 0xa8, 0x24, 0xde, 0x5b, 0x96, 0xc3, 0xab, 0x21 }
};


static  char *pv_errstr = "";

#define MAXFILES (16)
/* or any desired larger number: will be dynamically allocated one day */
typedef union {
    int32_t lsamp;
    float fsamp;
    unsigned char bytes[4];
} SND_SAMP;

typedef struct pvoc_file {
    WAVEFORMATEX fmtdata;
    PVOCDATA pvdata;
    /* RWD 2022 to help CDP pvocex extension, requires two-stage file creation/update */
    int32_t propsoffset;
    int32_t needsupdate;    //for two-stage creation/update CDP-style
    int32_t fmtchunkoffset;
    int32_t datachunkoffset;
    int32_t nFrames;   /* no of frames in file */
    int32_t FramePos;  /* where we are in file */
    int32_t curpos;
    int32_t fd;
    int32_t to_delete;
    int32_t readonly;
    int32_t do_byte_reverse;
    char *name;
    float *customWindow;
    
} PVOCFILE;



static PVOCFILE *files[MAXFILES];

static int32_t pvoc_writeheader(int32_t ofd);
static int32_t pvoc_readheader(int32_t ifd,WAVEFORMATPVOCEX *pWfpx);

int32_t pvoc_getdatasize_bytes(int32_t fd)
{
    int32_t datachunksize, framesize;
    
    if(files[fd]==NULL)
        return -1;
    if(files[fd]->nFrames == 0)
        return 0;
    framesize = files[fd]->pvdata.nAnalysisBins * 2 * sizeof(float);
    datachunksize = files[fd]->nFrames * framesize * files[fd]->fmtdata.nChannels;
    return datachunksize;
}

static int32_t write_guid(int32_t fd,int32_t byterev,const GUID *pGuid)
{
    int32_t written;
#ifdef _DEBUG
    assert(fd >= 0);
    assert(pGuid);
#endif

    if(byterev){
        GUID guid;
        guid.Data1 = REVDWBYTES(pGuid->Data1);
        guid.Data2 = REVWBYTES(pGuid->Data2);
        guid.Data3 = REVWBYTES(pGuid->Data3);
        memcpy((char *) (guid.Data4),(char *) (pGuid->Data4),8);
        written = write(fd,(char *) &guid,sizeof(GUID));
    }
    else
        written = write(fd,(char *) pGuid,sizeof(GUID));

    return written;

}

static int32_t compare_guids(const GUID *gleft, const GUID *gright)
{
    const char *left = (const char *) gleft, *right = (const char *) gright;
    return !memcmp(left,right,sizeof(GUID));
}


static int32_t write_pvocdata(int32_t fd,int32_t byterev,const PVOCDATA *pData)
{
    int32_t written;
 //   int32_t dwval;
#ifdef _DEBUG
    assert(fd >= 0);
    assert(pData);
#endif

    if(byterev){
//        int32_t revdwval;
        PVOCDATA data;
        SND_SAMP ssamp;
        data.wWordFormat = REVWBYTES(pData->wWordFormat);
        data.wAnalFormat = REVWBYTES(pData->wAnalFormat);
        data.wSourceFormat = REVWBYTES(pData->wSourceFormat);
        data.wWindowType = REVWBYTES(pData->wWindowType);
        data.nAnalysisBins = REVDWBYTES(pData->nAnalysisBins);
        data.dwWinlen   = REVDWBYTES(pData->dwWinlen);
        data.dwOverlap   = REVDWBYTES(pData->dwOverlap);
        data.dwFrameAlign = REVDWBYTES(pData->dwFrameAlign);
        
        ssamp.fsamp = pData->fAnalysisRate;
        //dwval = * (int32_t *) &(pData->fAnalysisRate);
        ssamp.lsamp = REVDWBYTES(/*dwval*/ ssamp.lsamp);
        //data.fAnalysisRate = * (float *) &dwval;
        data.fAnalysisRate = ssamp.fsamp;
        
        ssamp.fsamp = pData->fWindowParam;
        //dwval = * ( int32_t *) &(pData->fWindowParam);
        ssamp.lsamp = REVDWBYTES(ssamp.lsamp);
        //dwval = REVDWBYTES(dwval);
        //data.fWindowParam = * (float *) &dwval;
        data.fWindowParam = ssamp.fsamp;
        
        written = write(fd,(char *) &data,sizeof(PVOCDATA));
    }
    else
        written = write(fd,(char *) pData,sizeof(PVOCDATA));

    return written;

}

static int32_t write_fmt(int fd, int byterev,const WAVEFORMATEX *pfmt)
{   
    /*NB have to write out each element, as not guaranteed alignment othewise.
     *  Consider it documentation. */

#ifdef _DEBUG
    assert(fd >=0);
    assert(pfmt);
    assert(pfmt->cbSize == 62);
#endif

    if(byterev){
        WAVEFORMATEX fmt;
        fmt.wFormatTag      = REVWBYTES(pfmt->wFormatTag);
        fmt.nChannels       = REVWBYTES(pfmt->nChannels);
        fmt.nSamplesPerSec  = REVDWBYTES(pfmt->nSamplesPerSec);
        fmt.nAvgBytesPerSec = REVDWBYTES(pfmt->nAvgBytesPerSec);
        fmt.nBlockAlign     = REVWBYTES(pfmt->nBlockAlign);
        fmt.wBitsPerSample  = REVWBYTES(pfmt->wBitsPerSample);
        fmt.cbSize          = REVWBYTES(pfmt->cbSize);

        if(write(fd,(char *) &(fmt.wFormatTag),sizeof(WORD)) != sizeof(WORD)
            || write(fd,(char *) &(fmt.nChannels),sizeof(WORD)) != sizeof(WORD)
            || write(fd,(char *) &(fmt.nSamplesPerSec),sizeof(DWORD)) != sizeof(DWORD)
            || write(fd,(char *) &(fmt.nAvgBytesPerSec),sizeof(DWORD)) != sizeof(DWORD)
            || write(fd,(char *) &(fmt.nBlockAlign),sizeof(WORD)) != sizeof(WORD)
            || write(fd,(char *) &(fmt.wBitsPerSample),sizeof(WORD)) != sizeof(WORD)
            || write(fd,(char *) &(fmt.cbSize),sizeof(WORD)) != sizeof(WORD))

        return 0;

    }
    else {
       if(write(fd,(char *) &(pfmt->wFormatTag),sizeof(WORD)) != sizeof(WORD)
            || write(fd,(char *) &(pfmt->nChannels),sizeof(WORD)) != sizeof(WORD)
            || write(fd,(char *) &(pfmt->nSamplesPerSec),sizeof(DWORD)) != sizeof(DWORD)
            || write(fd,(char *) &(pfmt->nAvgBytesPerSec),sizeof(DWORD)) != sizeof(DWORD)
            || write(fd,(char *) &(pfmt->nBlockAlign),sizeof(WORD)) != sizeof(WORD)
            || write(fd,(char *) &(pfmt->wBitsPerSample),sizeof(WORD)) != sizeof(WORD)
            || write(fd,(char *) &(pfmt->cbSize),sizeof(WORD)) != sizeof(WORD))

        return 0;
    }

    return SIZEOF_WFMTEX;
}

static int32_t pvoc_writeWindow(int32_t fd,int32_t byterev,float *window,DWORD length)
{
    if(byterev){
        /* don't corrupt source array! */
        DWORD i;
        int32_t lval, *lp = (int32_t *) window;

        for(i=0;i < length; i++){
            lval = *lp++;
            lval = REVDWBYTES(lval);
            if(write(fd,(char *)&lval,sizeof(int32_t)) != sizeof(int32_t))
                return 0;
        }
    }
    else{
        if(write(fd,(char *) window,length * sizeof(float)) != (int32_t)(length*sizeof(float)))
            return 0;
    }


    return (int32_t)(length * sizeof(float));
}

static int32_t pvoc_readWindow(int fd, int byterev, float *window,DWORD length)
{
    if(byterev){
        DWORD i;
        int32_t got,oval,lval, *lp = (int32_t *) window;
#ifdef SINGLE_FLOAT 
        for(i=0;i < length;i++){
            if(read(fd,(char *)&lval,sizeof(int32_t)) != sizeof(int32_t))
                return 0;
            oval = REVDWBYTES(lval);
            *lp++ = oval;
        }
#else
        /* read whole block then swap...should be faster */
        got = read(fd,(char *) window,length * sizeof(float));
        if(got != (int)(length * sizeof(float)))
            return 0;
        /* then byterev */
        for(i=0;i < length;i++){
            lval = *lp;
            oval = REVDWBYTES(lval);
            *lp++ = oval;
        }

#endif
    }
    else{
        if(read(fd,(char *) window,length * sizeof(float)) != (int)(length * sizeof(float)))
            return 0;
    }

    return length * sizeof(float);

}



const char *pvoc_errorstr(void)
{
    return (const char *) pv_errstr;
}



/* thanks to the SNDAN programmers for this! */
/* return 0 for big-endian machine, 1 for little-endian machine*/
/* probably no good for 16bit swapping though */
static int32_t byte_order(void)
{
  int32_t   one = 1;
  char* endptr = (char *) &one;
  return (*endptr);
}

/***** loosely modelled on CDP sfsys ******
 *  This is a static array, but coul be made dynamic in an OOP sort of way.
 *  The idea is that all low-level operations and data 
 *  are completely hidden from the user, so that internal format changes can be made
 * with little or no disruption to the public functions.
 * But avoiding the full monty of a C++ implementation.

 *******************************************/

int32_t init_pvsys(void)
{
    int32_t i;

    if(files[0] != NULL) {
        pv_errstr = "\npvsys: already initialized";
        return 0;
    }
    for(i = 0;i < MAXFILES;i++)
        files[i] = NULL;

    return 1;
}

static void prepare_pvfmt(WAVEFORMATEX *pfmt,DWORD chans, DWORD srate,
                          pv_stype stype)
{

#ifdef _DEBUG
    assert(pfmt);
#endif


    pfmt->wFormatTag        = WAVE_FORMAT_EXTENSIBLE;
    pfmt->nChannels         = (WORD) chans;
    pfmt->nSamplesPerSec    = srate;
    pfmt->nBlockAlign       = 0;  /* decl to please gcc */
    switch(stype){
    case(STYPE_16):
        pfmt->wBitsPerSample = (WORD)16;
        pfmt->nBlockAlign    = (WORD)(chans * 2 *  sizeof(char));
        
        break;
    case(STYPE_24):
        pfmt->wBitsPerSample = (WORD) 24;
        pfmt->nBlockAlign    = (WORD)(chans *  3 * sizeof(char));
        break;
    case(STYPE_32):
    case(STYPE_IEEE_FLOAT):
        pfmt->wBitsPerSample = (WORD) 32;
        pfmt->nBlockAlign    = (WORD)(chans *  4 * sizeof(char));
        break;
    default:
#ifdef _DEBUG
        assert(0);
#endif
        break;
    }

    pfmt->nAvgBytesPerSec   = pfmt->nBlockAlign * srate;
    /* if we have extended WindowParam fields, or something ,will need to adjust this */
    pfmt->cbSize            = 62;

}


/* lots of different ways of doing this! */
/* we will need  one in the form:
 * in pvoc_fmtcreate(const char *fname,PVOCDATA *p_pvfmt, WAVEFORMATEX *p_wvfmt);
 */

/* a simple minimalist function to begin with!*/
/*set D to 0, and/or dwWinlen to 0, to use internal default */
/* fWindow points to userdef window, or is NULL */
/* NB currently this does not enforce a soundfile extension; probably it should... */
int32_t  pvoc_createfile(const char *filename,
                     DWORD fftlen,DWORD overlap,DWORD chans,
                     DWORD format,int32_t srate,
                     pv_stype stype,pv_wtype wtype,
                     float wparam,float *fWindow,DWORD dwWinlen)
{
    
    int32_t i;
    long N,D;
    char *pname;
    PVOCFILE *pfile = NULL;
    float winparam = 0.0f;

    N = fftlen;                   /* keep the CARL varnames for now */
    D = overlap;

    if(N == 0 || chans <=0 || filename==NULL || D > N) {
        pv_errstr = "\npvsys: bad arguments";
        return -1;
    }
    if(/*format < PVOC_AMP_FREQ ||*/ format > PVOC_COMPLEX) {    /* RWD unsigned, so nothing < 0 possible */
        pv_errstr = "\npvsys: bad format parameter";
        return -1;
    }

    if(!(wtype >= PVOC_DEFAULT && wtype <= PVOC_CUSTOM)){
        pv_errstr = "\npvsys: bad window type";
        return -1;
    }

    /* load it, but ca't write until we have a PVXW chunk definition...*/
    if(wtype==PVOC_CUSTOM){

    }

    if(wtype==PVOC_DEFAULT)
        wtype = PVOC_HAMMING;

    if(wtype==PVOC_KAISER)
        if(wparam != 0.0f)
            winparam = wparam;
    /*will need an internal default for window parameters...*/

    for(i=0;i < MAXFILES;i++)
        if(files[i]==NULL)
            break;
    if(i==MAXFILES) {
        pv_errstr = "\npvsys: too many files open";
        return -1;
    }
    pfile = (PVOCFILE *) malloc(sizeof(PVOCFILE));

    if(pfile==NULL){
        pv_errstr = "\npvsys: no memory";
        return -1;
    }
    pname = (char *) malloc(strlen(filename)+1);
    if(pname == NULL){
        free(pfile);
        pv_errstr = "\npvsys: no memory";
        return -1;
    }
    pfile->customWindow = NULL;
    /* setup rendering inforamtion */
    prepare_pvfmt(&pfile->fmtdata,chans,srate,stype);

    strcpy(pname,filename);

    pfile->pvdata.wWordFormat = PVOC_IEEE_FLOAT;
    pfile->pvdata.wAnalFormat = (WORD) format;
    pfile->pvdata.wSourceFormat =  (stype == STYPE_IEEE_FLOAT ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM);
    pfile->pvdata.wWindowType = wtype;
    pfile->pvdata.nAnalysisBins = (N>>1) + 1;
    if(dwWinlen==0)
        pfile->pvdata.dwWinlen      = N;
    else
        pfile->pvdata.dwWinlen  = dwWinlen;
    if(D==0)
        pfile->pvdata.dwOverlap = N/8;
    else
        pfile->pvdata.dwOverlap = D;
    pfile->pvdata.dwFrameAlign = pfile->pvdata.nAnalysisBins * 2 * sizeof(float);
    pfile->pvdata.fAnalysisRate = (float)srate / (float) pfile->pvdata.dwOverlap;
    pfile->pvdata.fWindowParam = winparam;
    pfile->to_delete = 0;
    pfile->readonly = 0;
    pfile->needsupdate = 0;   //could be overruled later, but only once!
    
    if(fWindow!= NULL){
        pfile->customWindow = malloc(dwWinlen * sizeof(float));
        if(pfile->customWindow==NULL){
            pv_errstr = "\npvsys: no memory for custom window";
            return -1;
        }
        memcpy((char *)(pfile->customWindow),(char *)fWindow,dwWinlen * sizeof(float));
    }


    pfile->fd = open(filename,O_BINARY | O_CREAT | O_RDWR | O_TRUNC,_S_IWRITE | _S_IREAD);
    if(pfile->fd < 0){
        free(pname);        
        if(pfile->customWindow)
            free(pfile->customWindow);
        free(pfile);

        pv_errstr = "\npvsys: unable to create file";
        return -1;
    }
    pfile->propsoffset = 0;
    pfile->datachunkoffset = 0;
    pfile->nFrames = 0;
    pfile->FramePos = 0;
    pfile->curpos = 0;
    pfile->name = pname;
    pfile->do_byte_reverse = !byte_order(); 
    files[i] = pfile;

    if(!pvoc_writeheader(i)) {
        close(pfile->fd);
        remove(pfile->name);
        free(pfile->name);
        if(pfile->customWindow)
            free(pfile->customWindow);
        free(pfile);
        files[i] = NULL;
        return -1;
    }

    return i;
}

int32_t pvoc_getpvxprops(int32_t ifd, PVOCDATA *data)
{
    if(files[ifd]==NULL)
        return 0;
#ifdef _DEBUG
    assert(data != NULL);
#endif
    memcpy((char*) data,(char*) &files[ifd]->pvdata,sizeof(PVOCDATA));
    return 1;
}
/* tell pvsys to update pvocdata on close; return 0 for success */
/* only other way to do this is to test initial params != 0 */
/* I want to control this explicitly, for now */
int32_t pvoc_set_needsupdate(int32_t ifd)
{
    if(files[ifd]==NULL)
        return -1;
    if(files[ifd]->readonly)
        return -1;
    files[ifd]->needsupdate = 1;
    return 0;
}

//returns 0 for success
int32_t pvoc_canupdate(int32_t ifd)
{
    if(files[ifd]==NULL || files[ifd]->readonly)
        return -1;
    if(files[ifd]->needsupdate)
        return 0;
    else
        return -1;
}

//ONLY when creating new file using 2-stages.called last thing before file close.
#if 0
int32_t pvoc_updateprops(int32_t fd, const PVOCDATA *data)
{
    DWORD pos;
    
    if(files[fd]==NULL)
        return -1;
    if(files[fd]->readonly)
        return -1;
    if(files[fd]->needsupdate==0)
        return -1;
#ifdef _DEBUG
    // the only field that MUST be set, needed for read/write frames
    assert(files[fd]->pvdata.nAnalysisBins == data->nAnalysisBins);
#endif
    // need to seek to propsoffset, then we can call write_pvocdata
    pos = lseek(files[fd]->fd,files[fd]->propsoffset,SEEK_SET);
    if(pos != files[fd]->propsoffset){
        pv_errstr = "\npvsys: error updating pvoc props";
        return 0;
    }
    return 1;
}
#endif

int32_t pvoc_openfile(const char *filename,PVOCDATA *data,WAVEFORMATEX *fmt)
{
    int32_t i;
    WAVEFORMATPVOCEX wfpx;
    char *pname;
    PVOCFILE *pfile = NULL;
//  long size = sizeof(WAVEFORMATPVOCEX);
    
    if(data==NULL || fmt==NULL){
        pv_errstr = "\npvsys: Internal error: NULL data arrays";
        return -1;
    }

    for(i=0;i < MAXFILES;i++)
        if(files[i]==NULL)
            break;
    if(i==MAXFILES){
        pv_errstr = "\npvsys: too many files open";
        return -1;
    }

    pfile = (PVOCFILE *) malloc(sizeof(PVOCFILE));
    if(pfile==NULL){
        pv_errstr = "\npvsys: no memory for file data";
        return -1;
    }
    pfile->customWindow = NULL;
    pname = (char *) malloc(strlen(filename)+1);
    if(pname == NULL){
        free(pfile);
        pv_errstr = "\npvsys: no memory";
        return -1;
    }
    pfile->fd = open(filename,O_BINARY | O_RDONLY,_S_IREAD);
    if(pfile->fd < 0){
        free(pname);
        free(pfile);
        pv_errstr = "\npvsys: unable to create file";
        return -1;
    }
    strcpy(pname,filename);
    pfile->datachunkoffset = 0;
    pfile->nFrames = 0;
    pfile->curpos = 0;
    pfile->FramePos = 0;
    pfile->name = pname;
    pfile->do_byte_reverse = !byte_order(); 
    pfile->readonly = 1;
    pfile->needsupdate = 0;   // just to state it explicitly!
    pfile->to_delete = 0;
    files[i] = pfile;

    if(!pvoc_readheader(i,&wfpx)){
        close(pfile->fd);
        free(pfile->name);
        if(pfile->customWindow)
            free(pfile->customWindow);
        free(pfile);
        files[i] = NULL;
        return -1;
    }

    memcpy((char *)data, (char *)&(wfpx.data),sizeof(PVOCDATA));
    memcpy((char *)fmt,(char *)&(wfpx.wxFormat.Format),SIZEOF_WFMTEX);

    files[i] = pfile;

    return i;

}

static int32_t pvoc_readfmt(int32_t fd,int32_t byterev,WAVEFORMATPVOCEX *pWfpx)
{
    DWORD dword;
    WORD word;

#ifdef _DEBUG
    assert(fd >= 0);
    assert(pWfpx);
#endif

    if(read(fd,(char *) &(pWfpx->wxFormat.Format.wFormatTag),sizeof(WORD)) != sizeof(WORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.nChannels),sizeof(WORD)) != sizeof(WORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.nSamplesPerSec),sizeof(DWORD)) != sizeof(DWORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.nAvgBytesPerSec),sizeof(DWORD)) != sizeof(DWORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.nBlockAlign),sizeof(WORD)) != sizeof(WORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.wBitsPerSample),sizeof(WORD)) != sizeof(WORD)
        || read(fd,(char *) &(pWfpx->wxFormat.Format.cbSize),sizeof(WORD)) != sizeof(WORD)){
        pv_errstr = "\npvsys: error reading Source format data";
        return 0;
    }

    if(byterev){
        word = pWfpx->wxFormat.Format.wFormatTag;
        pWfpx->wxFormat.Format.wFormatTag= REVWBYTES(word);
        word = pWfpx->wxFormat.Format.nChannels;
        pWfpx->wxFormat.Format.nChannels = REVWBYTES(word);
        dword = pWfpx->wxFormat.Format.nSamplesPerSec;
        pWfpx->wxFormat.Format.nSamplesPerSec = REVDWBYTES(dword);
        dword = pWfpx->wxFormat.Format.nAvgBytesPerSec;
        pWfpx->wxFormat.Format.nAvgBytesPerSec = REVDWBYTES(dword);
        word = pWfpx->wxFormat.Format.nBlockAlign;
        pWfpx->wxFormat.Format.nBlockAlign = REVWBYTES(word);
        word = pWfpx->wxFormat.Format.wBitsPerSample;
        pWfpx->wxFormat.Format.wBitsPerSample = REVWBYTES(word);
        word = pWfpx->wxFormat.Format.cbSize;
        pWfpx->wxFormat.Format.cbSize = REVWBYTES(word);

    }

    /* the first clues this is pvx format...*/
    if(pWfpx->wxFormat.Format.wFormatTag != WAVE_FORMAT_EXTENSIBLE){
        pv_errstr = "\npvsys: not a WAVE_EX file";
        return 0;
    }

    if(pWfpx->wxFormat.Format.cbSize != 62){
        pv_errstr = "\npvsys: bad size for fmt chunk";
        return 0;
    }

    if(read(fd,(char *) &(pWfpx->wxFormat.Samples.wValidBitsPerSample),sizeof(WORD)) != sizeof(WORD)
        || read(fd,(char *) &(pWfpx->wxFormat.dwChannelMask),sizeof(DWORD)) != sizeof(DWORD)
        || read(fd,(char *) &(pWfpx->wxFormat.SubFormat),sizeof(GUID)) != sizeof(GUID)){
        pv_errstr = "\npvsys: error reading Extended format data";
        return 0;
    }

    if(byterev){
        word = pWfpx->wxFormat.Samples.wValidBitsPerSample;
        pWfpx->wxFormat.Samples.wValidBitsPerSample = REVWBYTES(word);
        dword = pWfpx->wxFormat.dwChannelMask;
        pWfpx->wxFormat.dwChannelMask = REVDWBYTES(dword);

        dword = pWfpx->wxFormat.SubFormat.Data1;
        pWfpx->wxFormat.SubFormat.Data1 = REVDWBYTES(dword);
        word = pWfpx->wxFormat.SubFormat.Data2;
        pWfpx->wxFormat.SubFormat.Data2 = REVWBYTES(word);
        word = pWfpx->wxFormat.SubFormat.Data3;
        pWfpx->wxFormat.SubFormat.Data3 = REVWBYTES(word);
        /* don't need to reverse the char array */
    }

    /* ... but this is the clincher */
    if(!compare_guids(&(pWfpx->wxFormat.SubFormat),&KSDATAFORMAT_SUBTYPE_PVOC)){
        pv_errstr = "\npvsys: not a PVOCEX file";
        return 0;
    }

    if(read(fd,(char *) &(pWfpx->dwVersion),sizeof(DWORD)) != sizeof(DWORD)
        || read(fd,(char *) &(pWfpx->dwDataSize),sizeof(DWORD)) != sizeof(DWORD)
        || read(fd,(char *) &(pWfpx->data),sizeof(PVOCDATA)) != sizeof(PVOCDATA)){
        pv_errstr = "\npvsys: error reading Extended pvoc format data";
        return 0;
    }

    if(byterev){
        SND_SAMP ssamp;
        dword = pWfpx->dwVersion;
        pWfpx->dwVersion = REVDWBYTES(dword);

        /* check it now! */
        if(pWfpx->dwVersion != PVX_VERSION){
            pv_errstr = "\npvsys: unknown pvocex Version";
            return 0;
        }

        dword = pWfpx->dwDataSize;
        pWfpx->dwDataSize = REVDWBYTES(dword);

        word = pWfpx->data.wWordFormat;
        pWfpx->data.wWordFormat= REVWBYTES(word);
        word = pWfpx->data.wAnalFormat;
        pWfpx->data.wAnalFormat= REVWBYTES(word);
        word = pWfpx->data.wSourceFormat;
        pWfpx->data.wSourceFormat= REVWBYTES(word);
        word = pWfpx->data.wWindowType;
        pWfpx->data.wWindowType= REVWBYTES(word);

        dword = pWfpx->data.nAnalysisBins;
        pWfpx->data.nAnalysisBins = REVDWBYTES(dword);
        dword = pWfpx->data.dwWinlen;
        pWfpx->data.dwWinlen = REVDWBYTES(dword);
        dword = pWfpx->data.dwOverlap;
        pWfpx->data.dwOverlap = REVDWBYTES(dword);
        dword = pWfpx->data.dwFrameAlign;
        pWfpx->data.dwFrameAlign = REVDWBYTES(dword);

        ssamp.fsamp = pWfpx->data.fAnalysisRate;
        //dword = * (DWORD *)&(pWfpx->data.fAnalysisRate);  /* RWD TODO: use union */
        //dword = REVDWBYTES(dword);
        ssamp.lsamp = REVDWBYTES(ssamp.lsamp);
        //pWfpx->data.fAnalysisRate = *(float *)&dword;
        pWfpx->data.fAnalysisRate = ssamp.fsamp;
        
        ssamp.fsamp = pWfpx->data.fWindowParam;
        //dword = * (DWORD *)&(pWfpx->data.fWindowParam);
        //dword = REVDWBYTES(dword);
        ssamp.lsamp = REVDWBYTES(ssamp.lsamp);
        //pWfpx->data.fWindowParam = *(float *)&dword;
        pWfpx->data.fWindowParam = ssamp.fsamp;

    }
    if(pWfpx->dwVersion != PVX_VERSION){
        pv_errstr = "\npvsys: unknown pvocex Version";
        return 0;
    }

    return 1;


}


static int32_t pvoc_readheader(int32_t ifd,WAVEFORMATPVOCEX *pWfpx)
{
    DWORD tag, size,riffsize;
    int32_t fmtseen = 0, /* dataseen = 0,*/ windowseen = 0;
//  DWORD windowlength = 0;

#ifdef _DEBUG
    assert(pWfpx);
    assert(files[ifd]);
    assert(files[ifd]->fd >= 0);
    size = sizeof(WAVEFORMATEXTENSIBLE);
    size += 2 * sizeof(DWORD);
    size += sizeof(PVOCDATA);
#endif

    if(read(files[ifd]->fd,(char *) &tag,sizeof(DWORD)) != sizeof(DWORD)
        || read(files[ifd]->fd,(char *) &size,sizeof(DWORD)) != sizeof(DWORD)){
        pv_errstr = "\npvsys: error reading header";
        return 0;
    }
    if(files[ifd]->do_byte_reverse)
        size = REVDWBYTES(size);
    else
        tag = REVDWBYTES(tag);

    if(tag != TAG('R','I','F','F')){
        pv_errstr = "\npvsys: not a RIFF file";
        return 0;
    }
    if(size < 24 * sizeof(DWORD) + SIZEOF_FMTPVOCEX){
        pv_errstr = "\npvsys: file too small";
        return 0;
    }
    riffsize = size;
    if(read(files[ifd]->fd,(char *) &tag,sizeof(DWORD)) != sizeof(DWORD)){
        pv_errstr = "\npvsys: error reading header";
        return 0;
    }

    if(!files[ifd]->do_byte_reverse)
        tag = REVDWBYTES(tag);

    if(tag != TAG('W','A','V','E')){
        pv_errstr = "\npvsys: not a WAVE file";
        return 0;
    }
    riffsize -= sizeof(DWORD);
    /*loop for chunks */
    while(riffsize > 0){
        if(read(files[ifd]->fd,(char *) &tag,sizeof(DWORD)) != sizeof(DWORD)
            || read(files[ifd]->fd,(char *) &size,sizeof(DWORD)) != sizeof(DWORD)){
            pv_errstr = "\npvsys: error reading header";
            return 0;
        }
        if(files[ifd]->do_byte_reverse)
            size = REVDWBYTES(size);
        else
            tag = REVDWBYTES(tag);
        riffsize -= 2 * sizeof(DWORD);
        switch(tag){
        case TAG('f','m','t',' '):
            /* bail out if not a pvoc file: not trying to read all WAVE formats!*/
            if(size < SIZEOF_FMTPVOCEX){
                pv_errstr = "\npvsys:   not a PVOC-EX file";
                return 0;
            }
            if(!pvoc_readfmt(files[ifd]->fd,files[ifd]->do_byte_reverse,pWfpx)){
                pv_errstr = "\npvsys: error reading format chunk";
                return 0;
            }
            riffsize -=  SIZEOF_FMTPVOCEX;
            fmtseen = 1;
            memcpy((char *)&(files[ifd]->fmtdata),(char *)&(pWfpx->wxFormat),SIZEOF_WFMTEX);
            memcpy((char *)&(files[ifd]->pvdata),(char *)&(pWfpx->data),sizeof(PVOCDATA));
            break;
        case TAG('P','V','X','W'):
            if(!fmtseen){
                pv_errstr = "\npvsys: PVXW chunk found before fmt chunk.";
                return 0;
            }
            if(files[ifd]->pvdata.wWindowType!=PVOC_CUSTOM){
                /*whaddayado? can you warn the user and continue?*/
                pv_errstr = "\npvsys: PVXW chunk found but custom window not specified";
                return 0;
            }
            files[ifd]->customWindow = malloc(files[ifd]->pvdata.dwWinlen * sizeof(float));
            if(files[ifd]->customWindow == NULL){
                pv_errstr = "\npvsys: no memory for custom window data.";
                return 0;
            }
            if(pvoc_readWindow(files[ifd]->fd,files[ifd]->do_byte_reverse,
                files[ifd]->customWindow,files[ifd]->pvdata.dwWinlen)
                != (int)(files[ifd]->pvdata.dwWinlen * sizeof(float))){
                pv_errstr = "\npvsys: error reading window data.";
                return 0;
            }
            windowseen = 1;
            break;
        case TAG('d','a','t','a'):
            if(riffsize - size  != 0){
                pv_errstr = "\npvsys: bad RIFF file";
                return 0;
            }
            
            if(!fmtseen){
                pv_errstr = "\npvsys: bad format, data chunk before fmt chunk";
                return 0;
            }

            if(files[ifd]->pvdata.wWindowType==PVOC_CUSTOM)
                if(!windowseen){
                    pv_errstr = "\npvsys: custom window chunk PVXW not found";
                    return 0;
                }

            files[ifd]->datachunkoffset = lseek(files[ifd]->fd,0,SEEK_CUR);
            files[ifd]->curpos = files[ifd]->datachunkoffset;
            /* not m/c frames, for now */
            files[ifd]->nFrames = size / files[ifd]->pvdata.dwFrameAlign;

            return 1;
            break;
        default:
            /* skip any onknown chunks */
            riffsize -= 2 * sizeof(DWORD);
            if(lseek(files[ifd]->fd,size,SEEK_CUR) < 0){
                pv_errstr = "\npvsys: error skipping unknown WAVE chunk";
                return 0;
            }
            riffsize -= size;
            break;
        }

    }
    /* if here, something very wrong!*/
    pv_errstr = "\npvsys: bad format in RIFF file";
    return 0;

}

static int32_t pvoc_writeheader(int ofd)
{
    int32_t tag,size,version;
    WORD validbits;
    
    const GUID *pGuid =  &KSDATAFORMAT_SUBTYPE_PVOC;

#ifdef _DEBUG
    assert(files[ofd] != NULL);
    assert(files[ofd]->fd >=0);
#endif


    tag = TAG('R','I','F','F');
    size = 0;
    if(files[ofd]->do_byte_reverse)
        size = REVDWBYTES(size);
    if(!files[ofd]->do_byte_reverse)
        tag = REVDWBYTES(tag);

    if(write(files[ofd]->fd,&tag,sizeof(int32_t)) != sizeof(int32_t)
        || write(files[ofd]->fd,&size,sizeof(int32_t)) != sizeof(int32_t)) {
        pv_errstr = "\npvsys: error writing header";
        return 0;
    }

    tag = TAG('W','A','V','E');
    if(!files[ofd]->do_byte_reverse)
        tag = REVDWBYTES(tag);
    if(write(files[ofd]->fd,&tag,sizeof(int32_t)) != sizeof(int32_t)){
        pv_errstr = "\npvsys: error writing header";
        return 0;
    }

    tag = TAG('f','m','t',' ');
    size =  SIZEOF_WFMTEX + sizeof(WORD) + 
            sizeof(DWORD) 
            + sizeof(GUID) 
            + 2*sizeof(DWORD)
            + sizeof(PVOCDATA);
    if(files[ofd]->do_byte_reverse)
        size = REVDWBYTES(size);
    if(!files[ofd]->do_byte_reverse)
        tag = REVDWBYTES(tag);
    if(write(files[ofd]->fd,(char *)&tag,sizeof(int32_t)) != sizeof(int32_t)
        || write(files[ofd]->fd,(char *)&size,sizeof(int32_t)) != sizeof(int32_t)) {
        pv_errstr = "\npvsys: error writing header";
        return 0;
    }
    /* we need to record where we are, as we may have to update fmt data before file close */
    files[ofd]->fmtchunkoffset = lseek(files[ofd]->fd,0,SEEK_CUR);
    
    if(write_fmt(files[ofd]->fd,files[ofd]->do_byte_reverse,&(files[ofd]->fmtdata)) != SIZEOF_WFMTEX){
        pv_errstr = "\npvsys: error writing fmt chunk";
        return 0;
    }

    validbits = files[ofd]->fmtdata.wBitsPerSample;  /*nothing fancy here */
    if(files[ofd]->do_byte_reverse)
        validbits = REVWBYTES(validbits);

    if(write(files[ofd]->fd,(char *) &validbits,sizeof(WORD)) != sizeof(WORD)){
        pv_errstr = "\npvsys: error writing fmt chunk";
        return 0;
    }
    /* we will take this from a WAVE_EX file, in due course */
    size = 0;   /*dwChannelMask*/
    if(write(files[ofd]->fd,(char *)&size,sizeof(DWORD)) != sizeof(DWORD)){
        pv_errstr = "\npvsys: error writing fmt chunk";
        return 0;
    }

    if(write_guid(files[ofd]->fd,files[ofd]->do_byte_reverse,pGuid) != sizeof(GUID)){
        pv_errstr = "\npvsys: error writing fmt chunk";
        return 0;
    }
    version  = 1;
    size = sizeof(PVOCDATA);
    if(files[ofd]->do_byte_reverse){
        version = REVDWBYTES(version);
        size = REVDWBYTES(size);
    }

    if(write(files[ofd]->fd,&version,sizeof(int32_t)) != sizeof(int32_t)
        || write(files[ofd]->fd,&size,sizeof(int32_t)) != sizeof(int32_t)){
        pv_errstr = "\npvsys: error writing fmt chunk";
        return 0;
    }
/* RWD new 2022 so we can update all pvocex props on closing new file */
    files[ofd]->propsoffset = lseek(files[ofd]->fd,0,SEEK_CUR);
    
    if(write_pvocdata(files[ofd]->fd,files[ofd]->do_byte_reverse,&(files[ofd]->pvdata)) != sizeof(PVOCDATA)){
        pv_errstr = "\npvsys: error writing fmt chunk";
        return 0;

    }

    /* VERY experimental; may not even be a good idea...*/

    if(files[ofd]->customWindow){
        tag = TAG('P','V','X','W');
        size = files[ofd]->pvdata.dwWinlen * sizeof(float);
        if(files[ofd]->do_byte_reverse)
            size = REVDWBYTES(size);
        else
            tag = REVDWBYTES(tag);
        if(write(files[ofd]->fd,(char *)&tag,sizeof(int32_t)) != sizeof(int32_t)
            || write(files[ofd]->fd,(char *)&size,sizeof(int32_t)) != sizeof(int32_t)) {
            pv_errstr = "\npvsys: error writing header";
            return 0;
        }
        if(pvoc_writeWindow(files[ofd]->fd,
                            files[ofd]->do_byte_reverse,
                            files[ofd]->customWindow,
                            files[ofd]->pvdata.dwWinlen)!= (int)(files[ofd]->pvdata.dwWinlen * sizeof(float))){
            pv_errstr = "\npvsys: error writing window data.";
            return 0;
        }
    }

    /* no other chunks to write yet! */
    tag = TAG('d','a','t','a');
    if(!files[ofd]->do_byte_reverse)
        tag = REVDWBYTES(tag);
    if(write(files[ofd]->fd,&tag,sizeof(int32_t)) != sizeof(int32_t)){
        pv_errstr = "\npvsys: write error writing header";
        return 0;
    }

    /* we need to update size later on...*/

    size = 0;
    if(write(files[ofd]->fd,&size,sizeof(int32_t)) != sizeof(int32_t)){
        pv_errstr = "\npvsys: write error writing header";
        return 0;
    }
    files[ofd]->datachunkoffset = lseek(files[ofd]->fd,0,SEEK_CUR);

    files[ofd]->curpos = files[ofd]->datachunkoffset;
    return 1;
}

/* called by pvoc_closefile() */
/* which checks "readonly", if not set, file is newly created */
static int32_t pvoc_updateheader(int ofd)
{
    int32_t riffsize,datasize;
    DWORD pos;
    //RWD Oct 2025 avoid singed/unsigned errors
    long longpos;

#ifdef _DEBUG   
    assert(files[ofd]);
    assert(files[ofd]->fd >= 0);
    //assert(files[ofd]->curpos == lseek(files[ofd]->fd,0,SEEK_CUR));
    assert(!files[ofd]->readonly);
#endif
    datasize = files[ofd]->curpos - files[ofd]->datachunkoffset;
    pos = lseek(files[ofd]->fd,files[ofd]->datachunkoffset-sizeof(DWORD),SEEK_SET);
    if(pos != files[ofd]->datachunkoffset-sizeof(DWORD)){
        pv_errstr = "\npvsys: seek error updating data chunk";
        return 0;
    }

    if(files[ofd]->do_byte_reverse)
        datasize = REVDWBYTES(datasize);
    if(write(files[ofd]->fd,(char *) &datasize,sizeof(DWORD)) != sizeof(DWORD)){
        pv_errstr = "\npvsys: write error updating data chunk";
        return 0;
    }


    riffsize = files[ofd]->curpos - 2* sizeof(DWORD);
    if(files[ofd]->do_byte_reverse)
        riffsize = REVDWBYTES(riffsize);
    pos = lseek(files[ofd]->fd,sizeof(DWORD),SEEK_SET);
    if(pos != sizeof(DWORD)){
        pv_errstr = "\npvsys: seek error updating riff chunk";
        return 0;
    }
    if(write(files[ofd]->fd,(char *) &riffsize,sizeof(DWORD)) != sizeof(DWORD)){
        pv_errstr = "\npvsys: write error updating riff chunk";
        return 0;
    }
 
    // update PVOCDATA and WAVEFORMATEX fields if required
    if(files[ofd]->needsupdate){
#ifdef _DEBUG
        fprintf(stderr,"updating in pvoc_update_header()\n");
#endif
        WORD validbits;
        
        pos = lseek(files[ofd]->fd,files[ofd]->fmtchunkoffset,SEEK_SET);
        if(pos != (DWORD) files[ofd]->fmtchunkoffset){
            pv_errstr = "\npvsys: seek error updating fmt data";
            return 0;
        }
        
        pos = write_fmt(files[ofd]->fd,files[ofd]->do_byte_reverse,&(files[ofd]->fmtdata));
        if(pos != SIZEOF_WFMTEX){
            pv_errstr = "\npvsys: write error updating fmt data";
            return 0;
        }
        // need to update validbits in case we need it, at least make it sensible
        // this should have been updated earlier
        validbits = files[ofd]->fmtdata.wBitsPerSample;
        if(files[ofd]->do_byte_reverse)
            validbits = REVWBYTES(validbits);
        
        if(write(files[ofd]->fd,(char *) &validbits,sizeof(WORD)) != sizeof(WORD)){
            pv_errstr = "\npvsys: error writing extended fmt chunk";
            return 0;
        }
        pos = lseek(files[ofd]->fd,files[ofd]->propsoffset,SEEK_SET);
        if(pos != (DWORD) files[ofd]->propsoffset){
            pv_errstr = "\npvsys: seek error updating pvx data";
            return 0;
        }
        //int32_t write_pvocdata(int32_t fd,int32_t byterev,const PVOCDATA *pData)
        pos = write_pvocdata(files[ofd]->fd,files[ofd]->do_byte_reverse,&(files[ofd]->pvdata));
        if(pos != sizeof(PVOCDATA)){
            pv_errstr = "\npvsys: write error updating pvx data";
            return 0;
        }
    }
    /*pos*/ longpos = lseek(files[ofd]->fd, 0, SEEK_END);
    if(/*pos*/ longpos  < 0) {
        pv_errstr = "\npvsys: seek error seeking to end of file";
        return 0;
    }
    
    return 1;
}

int32_t pvoc_update_closefile(int ofd, const PVOCDATA *data, const WAVEFORMATEXTENSIBLE *wfx)
{
    if(files[ofd]==NULL){
        pv_errstr = "\npvsys: file does not exist";
        return 0;
    }
    
    if(files[ofd]->fd < 0){
        pv_errstr = "\npvsys: file not open";
        return 0;
    }
    if(files[ofd]->readonly) {
        pv_errstr = "\ncannot update readonly file"; // probably an input file
    }
    else {
        memcpy((char*) &files[ofd]->fmtdata, (char*) &wfx->Format, sizeof(WAVEFORMATEX));
        memcpy((char*) &files[ofd]->pvdata, (char*) data, sizeof(PVOCDATA));
    }
    return pvoc_closefile(ofd);
}


int32_t pvoc_closefile(int ofd)
{
    if(files[ofd]==NULL){
        pv_errstr = "\npvsys: file does not exist";
        return 0;
    }

    if(files[ofd]->fd < 0){
        pv_errstr = "\npvsys: file not open";
        return 0;
    }
    if(!files[ofd]->readonly) {
        if(!pvoc_updateheader(ofd))
            return 0;
    }
    close(files[ofd]->fd);
    if(files[ofd]->to_delete && !(files[ofd]->readonly))
        remove(files[ofd]->name);

    free(files[ofd]->name);
    free(files[ofd]);
    files[ofd] = 0;

    return 1;
}

/* does not directly address m/c streams, or alternative numeric formats, yet
 * so for m/c files, write each frame in turn, for each channel.
 * The format requires multi-channel frames to be interleaved in the usual way:
 * if nChannels= 4, the file will contain:
 * frame[0][0],frame[0][1],frame[0][2],frame[0][3],frme[1][0],frame[1][1].....
 *
 * The idea is to offer e.g. a floats version and a longs version ONLY, but
 * independently of the underlying representation, so that the user can write a floats
 * block, even though the underlying format might be longs or doubles. Most importantly,
 * the user does not have to deal with byte-reversal, which would otherwise always be the case
 * it the user had direct access to the file.
 * 
 * So these functions are the most likely to change over time!.
 *
 * return 0 for error, 1 for success. This could change....


 */
int32_t pvoc_putframes(int ofd,const float *frame,int32_t numframes)
{
    DWORD i;
    DWORD towrite;  /* count in 'words' */
    int32_t temp,*lfp;
    

    if(files[ofd]==NULL){
        pv_errstr = "\npvsys: bad file descriptor";
        return 0;
    }
    if(files[ofd]->fd < 0){
        pv_errstr = "\npvsys: file not open";
        return 0;
    }
    /* NB doubles not supported yet */
    
    towrite = files[ofd]->pvdata.nAnalysisBins * 2 * numframes;
    
    if(files[ofd]->do_byte_reverse){
        /* do this without overwriting source data! */
        lfp = (int32_t *) frame;
        for(i=0;i < towrite; i++){
            temp = *lfp++;
            temp = REVDWBYTES(temp);
            if(write(files[ofd]->fd,(char *) &temp,sizeof(int32_t)) != sizeof(int32_t)){
                pv_errstr = "\npvsys: error writing data";
                return 0;
            }

        }
    }
    else {
        if(write(files[ofd]->fd,(char *) frame,towrite * sizeof(float)) != (int32_t)(towrite * sizeof(float))){
            pv_errstr = "\npvsys: error writing data";
            return 0;
        }
    }

    files[ofd]->FramePos += numframes;
    files[ofd]->nFrames += numframes;
    files[ofd]->curpos += towrite * sizeof(float);
    return 1;
}

/* Simplistic read function
 * best practice here is to read nChannels frames *
 * return -1 for error, 0 for EOF, else numframes read
 */
int32_t pvoc_getframes(int32_t ifd,float *frames,DWORD nframes)
{
    int32_t i;
    int32_t toread;
    int32_t oval,temp,*lfp;
    int32_t got;
    int32_t rc = -1;
    if(files[ifd]==NULL){
        pv_errstr = "\npvsys: bad file descriptor";
        return rc;
    }
    if(files[ifd]->fd < 0){
        pv_errstr = "\npvsys: file not open";
        return rc;
    }

    toread = files[ifd]->pvdata.nAnalysisBins * 2 * nframes;

    if(files[ifd]->do_byte_reverse){
        lfp = (int32_t *) frames;
#ifdef SINGLE_FLOAT
        for(i=0;i < toread;i++){
            if((got=read(files[ifd]->fd,(char *) &temp,sizeof(int32_t))) <0){
                pv_errstr = "\npvsys: error reading data";
                return rc;
            }
            if(got < sizeof(int32_t)){
                /* file size incorrect? */
                return 0;           /* assume EOF */
            }
            temp = REVDWBYTES(temp);
            *lfp++ = temp;
        }
#else
        /* much faster on G4!!! */
        got = read(files[ifd]->fd,(char *)frames,toread * sizeof(float));
        if(got < 0){
            pv_errstr = "\npvsys: error reading data";
            return rc;
        }
        for(i=0;i < got / (int32_t) sizeof(float);i++){
            temp = *lfp;
            oval = REVDWBYTES(temp);
            *lfp++ = oval;
        }
        if(got < (int)(toread * sizeof(float))){
            /* some (user?) error in file size: return integral number of frames read*/
            toread  = got / (files[ifd]->pvdata.nAnalysisBins * 2 * sizeof(float));
            /* RWD 4:2002 need to revise this too */
            nframes = toread;
        }
            
#endif
        rc = nframes;   /*RWD 4:2002 */
    }
    else{
        if((got = read(files[ifd]->fd,(char *)frames,toread * sizeof(float))) < (int32_t)(toread * sizeof(float))){
            if(got < 0){
                pv_errstr = "\npvsys: error reading data";
                return rc;
            }
            else if(got < (int)(toread * sizeof(float))){
                /* some (user?) error in file size: return integral number of frames read*/
                toread  = got / (files[ifd]->pvdata.nAnalysisBins * 2 * sizeof(float));
                rc = toread;
                /* RWD 4:2002 need to revise this too */
                nframes = toread;
            }
        }
        else
            rc = nframes;
    }
    /*files[ifd]->curpos += (toread * sizeof(float));*/
    files[ifd]->curpos += got;  /* RWD 4:2002 */
    files[ifd]->FramePos += nframes;

    return rc;
}
// return 0 for success, -1 for error
int32_t pvoc_rewind(int32_t ifd,int32_t skip_first_frame)
{
    int32_t rc = -1;
    int32_t fd;
    int32_t pos;
    int32_t skipsize = 0;
    int32_t skipframes = 0;

    if(files[ifd]==NULL){
        pv_errstr = "\npvsys: bad file descriptor";
        return rc;
    }
    if(files[ifd]->fd < 0){
        pv_errstr = "\npvsys: file not open";
        return rc;
    }
    skipframes = files[ifd]->fmtdata.nChannels;
    //skipsize =  files[ifd]->pvdata.dwFrameAlign * skipframes;
    
    
    fd = files[ifd]->fd;
    pos = files[ifd]->datachunkoffset;
    if(skip_first_frame){
        skipsize =  files[ifd]->pvdata.dwFrameAlign * skipframes;
        pos += skipsize;
    }
    if(lseek(fd,pos,SEEK_SET) != pos ) {
        pv_errstr = "\npvsys: error rewinding file";
        return rc;
    }
    files[ifd]->curpos = /* files[ifd]->datachunkoffset + skipsize */ pos;
    files[ifd]->FramePos = skipframes;

    return 0;

}

/* may be more to do in here later on */
int32_t pvsys_release(void)
{
    int32_t i;

    for(i=0;i < MAXFILES;i++){
        if(files[i]) {
#ifdef _DEBUG
            fprintf(stderr,"\nDEBUG WARNING: file %d still open!\n",i);
#endif
            if(!pvoc_closefile(i)){             
                pv_errstr = "\npvsys: unable to close file on termination";
                return 0;
            }
        }
    }
    return 1;
}

/*return raw framecount:  channel-agnostic for now */
int32_t pvoc_framecount(int32_t ifd)
{
    if(files[ifd]==NULL)
        return -1;

    return files[ifd]->nFrames;
}
/* RWD Jan 2014   */
// return -1 for error, else framepos
int32_t pvoc_framepos(int32_t ifd)
{
    if(files[ifd]==NULL)
        return -1;
        
    return files[ifd]->FramePos;
}
// return -1 for error, 0 for success
//NB framecount always disregards chans. But offset represents m/c block: offset*2 to get frames
int32_t pvoc_seek_mcframe(int32_t ifd, int32_t offset, int32_t mode)
{
 //   DWORD mcframealign;
    int32_t rawoffset;  /* RWD need to be signed to work for to and fro seeks*/
    int32_t rc = -1;
    int32_t mcframealign;
    int32_t pvxcur = 0;
    if(files[ifd]==NULL)
        return -1;
    mcframealign =  files[ifd]->pvdata.dwFrameAlign * files[ifd]->fmtdata.nChannels;
    rawoffset = offset * mcframealign;
    switch (mode) {
    case SEEK_SET:
        // offset is m/c quantity, e.g. 2 * frame for stereo
        if (offset >= (files[ifd]->nFrames / files[ifd]->fmtdata.nChannels)) {
            pv_errstr = "\npvsys: seek target beyond end of file";
            break;
        }
        rawoffset += files[ifd]->datachunkoffset;
        if (lseek(files[ifd]->fd, rawoffset, SEEK_SET) != (long)rawoffset) {
            pv_errstr = "\npvsys: seek error, SEEK_SET";
            return -1;
        }
        files[ifd]->curpos = rawoffset;
        files[ifd]->FramePos = offset * files[ifd]->fmtdata.nChannels;
        rc = 0;
        break;
    case SEEK_END:
        // go to end of file + offset, offset <= 0
        if (offset > 0) {
            pv_errstr = "\npvsys: seek target before start of file, offset must be <= 0";
            break;
        }
#ifdef _DEBUG
        fprintf(stderr, "pvoc_seek_mcframe: fd = %d\n", ifd);
#endif
        //NB not relative to datachunkoffset in this case
        if (lseek(files[ifd]->fd, rawoffset, SEEK_END) < 0) {
            pv_errstr = "\npvsys: seek error, SEEK_END";
            return -1;
        }
#ifdef _DEBUG
        fprintf(stderr, "pvoc_seek_mcframe: files[%d]->nFrames = %d\n", ifd, files[ifd]->nFrames);
#endif
        files[ifd]->FramePos = files[ifd]->nFrames - (offset * files[ifd]->fmtdata.nChannels);
        files[ifd]->curpos = files[ifd]->datachunkoffset + files[ifd]->FramePos * files[ifd]->pvdata.dwFrameAlign;
#ifdef _DEBUG
        fprintf(stderr, "pvoc_seek_mcframe: got curpos = %d\n", files[ifd]->curpos);
#endif
        rc = 0;  //success!
        break;
    case SEEK_CUR:
        pvxcur = pvoc_framepos(ifd);
        if (pvxcur + offset >= files[ifd]->nFrames) {
            pv_errstr = "\npvsys: seek target beyond end of file";
            return rc;
        }
        if (pvxcur + offset < 0) {
            pv_errstr = "\npvsys: seek target beyond start of file";
            return rc;
        }
        rawoffset = offset * mcframealign;
        if (lseek(files[ifd]->fd, rawoffset, SEEK_CUR) < 0) {
            pv_errstr = "\npvsys: seek error, SEEK_CUR";
            return -1;
        }
        files[ifd]->FramePos = pvxcur + (offset * files[ifd]->fmtdata.nChannels);
        files[ifd]->curpos = files[ifd]->datachunkoffset + files[ifd]->FramePos * files[ifd]->pvdata.dwFrameAlign;
        rc = 0;
        break;
    }
    return rc;
}
