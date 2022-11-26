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



/*******************************************************************************************
 *      TO COMPILE TKLIB4 : COMMENT OUT (ONLY) the 2nd PAIR OF LINES BELOW
 *      TO COMPILE CDPARSE: COMMENT OUT (ONLY) the 1st PAIR OF LINES BELOW
 *******************************************************************************************/
/*RWD we do ALL THIS at COMPILER LEVEL NOW! */
/**
#define IS_CDPMAIN              (1)
#undef  IS_CDPARSE
**/

/**
#define IS_CDPARSE              (1)
#undef  IS_CDPMAIN
**/
/* RWD BUT: we can do this for convenience... */
#ifndef IS_CDPARSE
#define IS_CDPMAIN
#endif

/*******************************************************************************************/

#include <math.h>
#include <headread.h>
#include <cdpstate.h>

#define FAILED          (-1)    /* check with Ambrose ???? */
#define SUCCEEDED       (0)             /* check with Ambrose ???? */


/*******************************************************************************************
 * COMPUTER SYSTEM
 *******************************************************************************************/
/*RWD: I want this confined to the library somehow... */
#ifdef _WIN32
#define IS_PC   (1)
#endif

#ifdef unix
#define IS_UNIX (1)
#define _MAX_PATH (255)
#endif

#if !defined IS_PC && !defined IS_UNIX
#error CDP2K (tkglobals.h) Error: no platform defined
#endif
/*******************************************************************************************
 * TYPE OF CDP OUTPUT: FOR CMDLINE OR SOUNDLOOM GRAPHIC INTERFACE
 *******************************************************************************************/

#ifdef IS_PC
#ifdef _MSC_VER
#pragma warning (disable : 4032)
#pragma warning (disable: 4100)
#pragma warning (disable: 4702)
#endif
#include <conio.h>                                      /* respond to keyboard */
#endif


#define F_SECSIZE       (256)

#ifdef IS_CDPMAIN

/*******************************************************************************************
 * FUNCTIONS GLOBAL TO CDPARSE, CDPARAMS and CDPMAIN
 *******************************************************************************************/

#define NOT_SET (0)      /* Cannot be changed from 0, see modify-brasssage constants list */

int  get_maxmode(int process);
int  allocate_and_initialise_validity_flags(int **valid,int *validcnt);
int  establish_application_validities(int filetype,int channels,int *valid);
int  valid_application(int process,int *valid);
int  assign_input_process_type(int process,int mode,int *inprocess_type);
int  setup_input_param_range_stores(int total_params,aplptr ap);
int  set_param_ranges(int process,int mode,double nyquist,float frametime,float arate,int srate,
                                int wlength,int insams,int channels,int wanted,
                                int filetype,int linecnt,double duration,aplptr ap);
int      set_legal_param_structure(int process,int mode,aplptr ap);
int      set_legal_option_and_variant_structure(int process,int mode,aplptr ap);
void set_formant_flags(int process,int mode,aplptr ap);
int  initialise_param_values(int process,int mode,int channels,double nyquist,float frametime,
                int insams,int srate,int wanted,int linecnt,double duration,double *default_val,int filetype,aplptr ap);
int  cdparse(char *filename,infileptr inputfile);
int  setup_special_data_ranges(int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap);
int readhead(infileptr inputfile,int ifd,char *filename,double *maxamp,double *maxloc, int *maxrep,int getmax,int getmaxinfo);

int  is_a_valid_input_filetype(int filetype);
int  is_a_valid_file_for_process(int filetype,int input_data_type);
int  is_a_text_input_filetype(int filetype);
int  is_a_textfile_type(int filetype);  /* after type conversion: not at input */
int  could_be_transpos_and_or_pitch_infiletype(int filetype);
int  is_brkfile_infiletype(int filetype);
int  is_numlist_infiletype(int filetype);
int  is_transpos_infiletype(int filetype);
int  is_pitch_infiletype(int filetype);
int  is_sndlist_only_infiletype(int filetype);
void rectify_window(float *flbuf,dataptr dz);
int      tkusage(int process,int mode);
void print_outmessage(char *str);
void print_outmessage_flush(char *str);
void print_outwarning_flush(char *str);
//TW NEW
int  value_is_numeric(char *str);

//TW NEW
#define VOWEL_STRING            (-6)
#define INTERVAL_STRING         (-5)
#define NOTE_STRING                     (-4)
#define REORDER_STRING          (-3)
#define SHUFFLE_STRING          (-2)
#define SNDFILENAME_STRING      (-1)

/* data stored in TK */

#define CDP_PROPS_CNT                           (34)

#define INPUT_FILETYPE                          (0)
#define INPUT_FILESIZE                          (1)
#define INPUT_INSAMS                            (2)
#define INPUT_SRATE                                     (3)
#define INPUT_CHANNELS                          (4)
#define INPUT_WANTED                            (5)
#define INPUT_WLENGTH                           (6)
#define INPUT_LINECNT                           (7)
#define INPUT_ARATE                                     (8)
#define INPUT_FRAMETIME                         (9)
#define INPUT_NYQUIST                           (10)
#define INPUT_DURATION                          (11)

#define INPUT_STYPE                                     (12)
#define INPUT_ORIGSTYPE                         (13)
#define INPUT_ORIGRATE                          (14)
#define INPUT_MLEN                                      (15)
#define INPUT_DFAC                                      (16)
#define INPUT_ORIGCHANS                         (17)
#define INPUT_SPECENVCNT                        (18)
#define INPUT_OUT_CHANS                         (19)
#define INPUT_DESCRIPTOR_BYTES          (20)
#define INPUT_IS_TRANSPOS                       (21)
#define INPUT_COULD_BE_TRANSPOS         (22)
#define INPUT_COULD_BE_PITCH            (23)
#define INPUT_DIFFERENT_SRATES          (24)
#define INPUT_DUPLICATE_SNDS            (25)
#define INPUT_BRKSIZE                           (26)
#define INPUT_NUMSIZE                           (27)
#define INPUT_ALL_WORDS                         (28)
#define INPUT_WINDOW_SIZE                       (29)
#define INPUT_MINBRK                            (30)
#define INPUT_MAXBRK                            (31)
#define INPUT_MINNUM                            (32)
#define INPUT_MAXNUM                            (33)

//TW NEW
#define BUF_MULTIPLIER  (20)    /* arbitrary size for moment */


/* RWD ARRRRGH! CANNOT declare a var in a header file! I declare in dzsetup.c, so it goes in the library */
/*unsigned int superzargo;*/    /* for timing involving several file-writes */
extern unsigned int superzargo;

#define NUMERICVAL_MARKER       '@'     /* MUST TALLY WITH TK CODE !! */
void splice_multiline_string(char *str,char *prefix);
#define PBAR_LENGTH                                     (256)

void splice_multiline_string(char *str,char *prefix);

#ifdef IS_UNIX
#define FILE_JOINER             '/'
#endif

#ifdef IS_PC
#define FILE_JOINER             '\\'
#endif

/*******  RWD we don't support old MacOS!
#ifdef IS_MAC
#define FILE_JOINER             ':'
#endif
*******/
int redefine_textfile_types(dataptr dz);
int strgetfloat(char **str,double *val);
//TW NEW
int strgetfloat_within_string(char **str,double *val);
void get_other_filename(char *filename,char c);
int close_and_delete_tempfile(char *filename,dataptr dz);
int smpflteq(double f1,double f2);
#define SMP_FLTERR 0.0000005            /* zero (within error) for samprate of 200000 or below */
void insert_new_number_at_filename_end(char *filename,int num,int overwrite_last_char);
void insert_new_chars_at_filename_end(char *filename,char * str);
void replace_filename_extension(char *filename,char *ext);
void delete_filename_lastchar(char *filename);
void reset_file_params_for_sndout(int *orig_proctype,int *orig_srate,
        int *orig_chans,int *orig_stype,dataptr dz);
int reset_peak_finder(dataptr dz);
int establish_peak_status(dataptr dz);
#endif
//TW names reflecting new status of these data items
#define CDP_SOUND               SAMP_SHORT
#define CDP_NONSOUND    SAMP_FLOAT

void copy_to_fileptr(infileptr i, fileptr f);
