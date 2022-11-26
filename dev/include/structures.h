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



/* floatsam version */
   /*********************************************/
   /* STORAGE OF DUPLICATE HEADERS IN SFRECOVER */
   /*********************************************/
#ifndef  _STRUCTURES_H_INCLUDED
#define  _STRUCTURES_H_INCLUDED

#include <sfsys.h>  /* for CHPEAK */

struct hhead {
        char  *addr;
        short cnt;
};

typedef struct hhead *headptr;

   /*******************************************/
   /* ELEMENTS IN PLAYLIST FOR MAKING TEXTURE */
   /*******************************************/

struct soundout {
float *inbuf;
unsigned int ibufcnt;
unsigned int st_sstttime;
unsigned int st_sendtime;
unsigned int obufpos;
int st_splicpos;
double ibufpos;
double step;
double lgain;
double rgain;
struct soundout *next;
struct soundout *last;
};

typedef struct soundout *sndoutptr;

/*************************************************/
/*  STORE CHAN DATA IN RINGBUF (TRACE,SUPR etc)  */
/*************************************************/

struct chanval {
        float   val;
        int             loc;
        struct  chanval *next;
        struct  chanval *last;
};

typedef struct chanval *chvptr;

/*************************************************/
/*              STORE LOUDEST-CHANNEL DATA (BLTR)               */
/*************************************************/

struct chanstore {
    float amp;
    int   chan;
};

typedef struct chanstore *chsptr;

/*************************************************/
/*                STORES SPEC SPLIT BANDING DATA                 */
/*************************************************/

struct bandd {
        int  bdoflag;           /*RWD these two were short */
        int  badditive;
        double bfrqlo;
        double bfrqhi;
        double bfrqdif;
        double bamp;
        double bampdif;
        double btrans;
};

typedef struct bandd *bandptr;

/*************************************************/
/*      WHICH CHANS ARE PEAKS, & HOW OFTEN (FOCU)        */
/*************************************************/

struct design {
        int     chan;
        int score;
        double amp;
};

typedef struct design *desptr;

/*************************************************/
/* FOCU: DATA FOR CALCULATING STABILITY OF PEAKS */
/*************************************************/

struct stability {
        int     offset;
        float *sbufstore;
        float *specstore;
        float **sbuf;                   /* pointers to windows within it        */
        float **spec;
        int *fpkstore;                  /* baktraking spectral-peaks buffer */
        int *total_pkcnt;               /* count of total peaks found per baktrak buffer */
        int *design_score;              /* array for designing filter           */
        int **fpk;                      /* pointers to peakstores within it */
        desptr *des;
};

typedef struct stability *stabptr;

   /*******************************************/
   /* ELEMENTS IN NOTELIST, STORING NOTE DATA */
   /*******************************************/

struct nnote {
        float pitch;
        float amp;
        float dur;
        float ntime;
        float spacepos;
        float motioncentre;
        unsigned char instr;
        struct nnote *next;
        struct nnote *last;
};

typedef struct nnote *noteptr;

   /**********************************************/
   /* ELEMENTS IN MOTIFLIST, STORING NOTE-MOTIFS */
   /**********************************************/

struct motif {
        noteptr firstnote;
        struct motif *next;
        struct motif *last;
};

typedef struct motif *motifptr;

   /*************************************************/
   /* PROPERTIES OF INPUT FILES, FOR TEXTURE APPLICS*/
   /*************************************************/

struct insample {
float *buffer;
double pitch;
};

typedef struct insample *insamptr;

   /***********************************************/
   /* ADDITIONAL DATA STORAGE FOR TEXTURE APPLICS */
   /***********************************************/

struct textural {
unsigned int txflag;
motifptr      motifhead;
motifptr      timeset;
motifptr      hfldmotif;
motifptr      *phrase;
insamptr      *insnd;
int           **perm;
unsigned char dectypstor;
unsigned char dectypcnt;
unsigned char amptypstor;
unsigned char amptypcnt;
int           phrasecnt;
int           ampdirectd;
};

typedef struct textural *texptr;

   /***********************************************/
   /*           DATA STORAGE FOR MIXING ACTIONS           */
   /***********************************************/

struct actval {
int  ifd;               /* file from which to read samples */
int  bufno;             /* these must be assigned before we start using this struct */
int samplen;            /* number of samples produced from whole file */
double llscale;         /* this is left  weighting of left-inchan  * 32768 */
double lrscale;         /* this is right weighting of left-inchan  * 32768 */
double rlscale;         /* this is left  weighting of right-inchan * 32768 */
double rrscale;         /* this is right weighting of right-inchan * 32768 */
int  stereo;            /* mono, stereo, mono-to-stereo, stereo_panned */
} ;

typedef struct actval  *actvalptr;

   /***********************************************/
   /*                           MIXING ACTIONS                    */
   /***********************************************/

struct action {
int       position;     /* time of action, in samples                 */
actvalptr val;          /* all associated values, stored in an actval */
int       role;         /* ON , REFILL, or OFF                        */
} ;

typedef struct action  *actptr;


   /***********************************************/
   /*                           MIXING BUFFERS                 */
   /***********************************************/


struct bufitem {
short status;            /* Buffer is in use or not */
float *buf;              /* Location of buffer      */
float *here;             /* Location in buffer      */
} ;

typedef struct bufitem *mixbufptr;

   /***********************************************/
   /*   APPLICATION (Possibly only for CMDLINE??  */
   /***********************************************/

struct applic {
char    max_param_cnt;          /* Max number of parameter taken by prog */
char    internal_param_cnt;     /* Number of internal parameters */
                                /****** ENTRY OF DATA TO PROGRAM ******/
char    formant_flag;           /* Indicates whether formant data is entered */
char    formant_qksrch;         /* Indicates alternative formant calculation */
char    **param_name;
char    **flagname;
char    *special_data_name;
char    *special_data_name2;
char    special_data;           /* Indicates if (and which) special data (pitch,filter params etc) is used by prog */
int     special_range;          /* Flag when range of special_data can be displayed in dialog-box */
int     other_special_range;    /* Flag when range of 2nd item of special_data CAN be displayed */
int     data_in_file_only;
int     input_process_type;
int     accepts_conflicting_srates;
double  min_special;            /* range for special_data */
double  max_special;
double  default_special;        /* default value of special_data */
double  min_special2;           /* range for 2nd item of special_data */
double  max_special2;
int     no_pitchwise_formants;
int     min_fbands;
int     max_freqwise_fbands;
int     max_pichwise_fbands;
int     no_pichwise_formants;
char    param_cnt;               /* Number of parameter taken by prog */
char    option_cnt;              /* Number of options on options menu, or cmdline */
char    vflag_cnt;               /* Total number of variants in menu, or possible on cmdline */
char    variant_param_cnt;       /* Number of variants with parameters */
char    total_input_param_cnt;

char*   display_type;           /* normal, log etc */
char*   has_subrange;
                                /* total number of params that can be input, including options & variants */
                                /* PARAMETER TYPE (int, double, brkpnt. brkpnt-int) for ... */
double *lo;                     /* parameter range min */
double *hi;                     /* parameter range max */
double *default_val;            /* parameter range max */
double *lolo;                   /* displayed parameter range min */
double *hihi;                   /* displayed parameter range max */
char   *param_list;             /* each param in turn */
char   *option_list;            /* each option in turn */
char   *variant_list;           /* each variant in turn */
char   *internal_param_list;    /* internal param types */
                                /* CMDLINE FLAGS */
char   *option_flags;           /* Cmdline flags used to differentiate options */
char   *variant_flags;          /* Cmdline flags used to differentiate variants */
};

typedef struct applic *aplptr;


   /***********************************************/
   /*            INFILE DATA STORAGE AT PARSING           */
   /***********************************************/
/* RWD FEB 2010: need unsigned values for file sizes */
struct filedata {
    int     filetype;
                        /* FILE DESCRIPTOR AND SIZES */
/*unsigned*/ int infilesize;
/*unsigned*/ int insams;    /*RWD FEB 2010 */
                        /* FILE PROPERTIES */
    int     srate;                          /* cdp file properties */
    int     channels;
    int     stype;
    int     origstype;
    int     origrate;
    int     Mlen;
    int     Dfac;
    int     origchans;
    int     specenvcnt;
    int     wanted;                         /* floats per window */
    int     wlength;                        /* length in windows */
    int     out_chans;                      /* outchans of mixfile */
    int     descriptor_samps;
                                /* FLAGS */
    int     is_transpos;            /* flags transpos, rather than pitch, in floatarray */
    int     could_be_transpos;      /* flags textfile data COULD be transpos data */
    int     could_be_pitch;         /* flags textfile data COULD be pitch data  */
    int     different_srates;       /* flags program can accept insnds of different srates */
    int     duplicate_snds;         /* flags there are duplicated sndfiles in mixfile */
                                /* COUNTERS */
    int     brksize;                        /* (paired) size of brkpnt data  */
    int     numsize;                        /* unpaired size of numeric data */

                                /* TEXTFILE DATA */
    int     linecnt;                    /* number of lines */
    int     all_words;                  /* number of words */

    float   arate;
    float   frametime;                  /* duration of window */
    float   window_size;            /* duration of (envelope) window */
    double  nyquist;
                                    /* BRKPNT DATA */
    double  duration;                       /* duration of brkpnt data */
    double  minbrk;
    double  maxbrk;
    double  minnum;
    double  maxnum;
};

typedef struct filedata *infileptr;

   /***********************************************/
   /*               INFILE PROPERTIES STORAGE             */
   /***********************************************/

struct fileprops {
    int     filetype;
    int     srate;
    int     stype;
    int     origstype;
    int     origrate;
    int     channels;
    int     origchans;      /* pitch, formant,transpos only */
    int     specenvcnt;     /* formants only */
    int     Mlen;
    int     Dfac;
    float   arate;
    float   window_size;    /* duration of (envelope) window */
};

typedef struct fileprops *fileptr;

   /***********************************************/
   /*             PROGRAM PRINCIPAL STRUCTURE             */
   /***********************************************/


struct datalist {
int                     process;                /* which application is being run */
int                     maxmode;                /* number of modes of operation of process */
int                     mode;                   /* actual mode of operation of process */
/* PROCESS TYPES */
int                     input_data_type;
int                     outfiletype;
int                     process_type;
char            *outfilename;  /* need this for synth */
/*RWD.10.98 force floatsam outfile*/
int                     floatsam_output;
double          peak_fval;
int             true_outfile_stype;
int                     clip_floatsams;

/* all this is to do with PEAK handling! */

int                     outchans;
int                     otheroutchans;
int                     needpeaks;
int                     needotherpeaks;
CHPEAK          *outpeaks;               /* for running PEAK counting */
CHPEAK          *otherpeaks;                    /* for 'otherfile' output;*/
unsigned int            *outpeakpos;     /* tracking position of peak, for each channel */
unsigned int            *otherpeakpos;

/* APPLICATION */
aplptr          application;    /* structure to store details of particular process */
/* BUFFERS */
float           *bigbuf;                /* internal large buffer */
float           **sampbuf;              /* buffer starts and ends */
float           **sbufptr;              /* buffer pointers */
float           *bigfbuf;               /* internal large buffer for floats */
float           **flbufptr;             /* buffer pointers for float bufs */
float           **windowbuf;    /* additional window_size buffers for floats */
float           **extrabuf;             /* extra non-standard-length snd-buffer pointers */
float           **extrabufptr;  /* extra_buffer pointers */
float           *amp;
float           *freq;
/* BRKPNT TABLES */
double          **brk;
double          **brkptr;
int             *brksize;
double          *lastind;
double          *lastval;
int                     *brkinit;
double          *firstval;
/* PARAMETERS */
double          *param;                 /* parameter storage */
int                     *iparam;                /* integer parameter storage */
char            *vflag;                 /* internal program flags set by variants in cmdline/menu */
/* PARAMETER MARKERS */
char            *is_int;                /* mark integer parameters, INCLUDING internal parameters */
char            *no_brk;                /* mark parameters which cannot be entered as brkpntfiles */
char            *is_active;             /* mark parameters which COULD actually be in use */
/* ARRAYS */
double          **parray;               /* parameter array storage */
int                     **iparray;              /* integer parameter array storage */
/* RWD used for submix syncatt  instead of lparray      */
float           **lfarray;
int             **lparray;              /* int parameter array storage */
float           *sndbuf;                /* short parameter array storage, for (pseudo) snd output */
/* INTERNAL POINTERS */
/*TW NAME CHANGE, TO avoid possible future error (lptrs were paired with lparrays)*/
float           **fptr;                 /* misc pointers to floats   */
double          **ptr;                  /* misc pointers to doubles */
/* FILE PROPERTY STORES */
fileptr         infile;                 /* first infile */
fileptr         outfile;                /* main outfile */
fileptr         otherfile;              /* 2nd infile which CAN have different sr, chans */
/* INFILE POINTERS AND SIZES */
int             *ifd;
/*int                   *infilesize;*/
int             *insams;
/* FORMANT ARRAYS */
float           *specenvfrq;
float           *specenvpch;
float           *specenvamp;
float           *specenvtop;
float           *specenvamp2;
/* FOR PITCH CONTOUR, TRANSPOSITION OR CHORDS */
float           *pitches;
float           *transpos;
float           *pitches2;
float           *transpos2;
float           *frq_template;
/* FOR FILTERS */
float           *fsampbuf;
double          *filtpeak;
double          *fbandtop;
double          *fbandbot;
int                     *peakno;
int                     *lastpeakno;
/* SPECIAL PURPOSE */
bandptr         *band;           /* for SPLIT */
stabptr         stable;          /* for FOCUS */
chsptr          loudest;         /*     FOR BLTR */
chvptr          ringhead;        /*     FOR SUPR and TRACE & PITCH */
chvptr          ringhead2;       /*     FOR MEAN */
double          *temp;
/* ENVELOPES */
float           *origenv;
float           *origend;
float           *env;
float           *envend;
double          *rampbrk;
/* MIXFILE */
actvalptr       *valstor;
actptr          *act;
mixbufptr       *buflist;
int                     *activebuf;
float           **activebuf_ptr;
/* TEXTTURE */
texptr          tex;
/* TEXTFILES */
int                     *wordcnt;
char            **wordstor;
FILE            *fp;
/* OTHER FILE PONTERS */
int             ofd;
int                     other_file;
/* DATA OR PROCESS CONSTANTS */
int             outfilesize;
/*int                   bigbufsize;*/
/*int                   bigbufsize2;    */
int             buflen2;
int             big_fsize;
int             buflen;
int             linecnt;
int             all_words;
int             extra_word;
int     numsize;
int             wanted;
/*int                   byteswanted;    */
int             sampswanted;
int             wlength;
int             clength;
int                     infilecnt;
int                     bptrcnt;
int                     bufcnt;                 /* buffers used by SND programs */
int                     extra_bufcnt;   /* extra_buffers used by SND programs */
int                     array_cnt;
int                     iarray_cnt;
int                     larray_cnt;
/*TW NAME CHANGE*/
/*int           lptr_cnt;*/
int                     fptr_cnt;
int                     ptr_cnt;
int                     formant_bands;
int                     specenvcnt;
/*int                   descriptor_bytes;*/
int                     descriptor_samps;
int             out_chans;
int                     extrabrkno;
double          nyquist;
double          chwidth;
double          halfchwidth;
double          minbrk;
double          maxbrk;
double          minnum;
double          maxnum;
double          duration;
/* COUNTERS */
int             total_windows;
int             total_samps_written;
/*int           total_bytes_written;*/
/*int           bytes_left;*/
int             samps_left;
int             ssampsread;
int             total_samps_read;
/*int           bytes_read;*/
/*int           samps_read;     */              /*RWD  just use ssampsread */
/*int           total_bytes_read;*/
int                     itemcnt;
int                     ringsize;
int             tempsize;                       /*RWD need both? */
int             temp_sampsize;          /*RWD my addition for housekeep channels*/
int             rampbrksize;
/* TIMERS */
float           time;
float           timemark;
float           frametime;
/* OTHER */
double          scalefact;
double          is_sharp;
double          is_flat;
/* FLAGS */
int                     specenv_type;
int                     deal_with_chan_data;
int             different_srates;
int             duplicate_snds;
int                     unspecified_filecnt;
int                     has_otherfile;
int                     is_transpos;
int                     could_be_transpos;
int                     could_be_pitch;
int                     finished;
int                     zeroset;
int                     fzeroset;
int                     is_mapping;
int                     is_rectified;

/*RWD */
/*int                   snd_ofd;*/
/*int                   snd_ifd;*/ /* just the one */
};

typedef struct datalist *dataptr;


#endif
