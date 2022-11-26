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



/* floats verison */
#define SHORT_BUFFERS   1                /* RWD TODO: ??? */

#define CHARBITSIZE                             (8)     /* size of char in bits */


/************************ ERROR HANDLING/RETURN **************/

#define CONTINUE      (1)
#define FINISHED      (0)
#define GOAL_FAILED       (-1)  /* program succeeds, but users goal not achieved: e.g. find pitch */
#define USER_ERROR    (-2)      /* program fails because wrong data, no data etc given by user */
#define DATA_ERROR        (-3)  /* Data is unsuitable, incorrect or missing */
#define MEMORY_ERROR  (-4)      /* program fails because ran out of, or mismanaged, memory */
#define SYSTEM_ERROR  (-5)  /* failure to write to or truncate outfile: usually means H/D is full */
#define PROGRAM_ERROR (-6)      /* program fails because the programming is naff */
#define USAGE_ONLY        (-7)  /* program interrogated for usage_message only */
#define TK_USAGE          (-8)  /* program interrogated by TK for usage_message only */
#define BAD_INFILECNT (-9)      /* Bad infilecnt sent from TK at program testing stage */

extern char errstr[];
extern char warnstr[];
extern char informstr[];
extern char username[];

extern int sloom;
/* TW May 2001 */
extern int sloombatch;
extern int anal_infiles;
extern int is_converted_to_stereo;

/************************** GENERAL **************************/

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (1)
#endif

#define LAST    (TRUE)

#define MAX_PARAM_LEN   (64)

//#define       MAX_CHANNELS    (4)                     /* maximum number  of SNDFILE channels */
#define MAX_CHANNELS    (16)
#define MAX_MIX_CHANS   (STEREO)        /* maximum number  of SNDFILE channels for mixing */

/* PROBLEM : maxfloat applies only to PC and I'm not sure I got it right !! */
/* RWD this is unix version */
/* defined in math.h on unix platforms */
#if defined _MSC_VER || defined __GNUC__
# ifndef MAXFLOAT
# define        MAXFLOAT        ((float)3.40282346638528860e+38)
# endif
#endif
#define FLTERR                  (0.000002)

/******************* UNIVERSAL CONSTANTS AND MACROS ***********************************/

//#define       MAX_SNDFILE_OUTCHANS    (4)      /* quad output, max for sndfiles, 1997 */
//allow for double ADAT chancount!
#define MAX_SNDFILE_OUTCHANS    (16)     /* quad output, max for sndfiles, 1997 */

#define MIN_FRACTION_OF_LEVEL   (1.0/32767.0)
//TODO: need to add 96000 and 88200
#define LEGAL_SRATE(x)  ((x)==16000 || (x)==24000 || (x)==22050 || (x)==32000 || (x)==44100 || (x)==48000)

#define ODD(x)                          ((x)&1)
#define EVEN(x)                         (!ODD(x))

#define MAXSAMP                         (32767)
#define MINSAMP                         (-32768)
#define ABSMAXSAMP                      (32768.0)
//RWD floatsam versions
#define F_MAXSAMP                       (1.0)
#define F_MINSAMP                       (-1.0)
#define F_ABSMAXSAMP            F_MAXSAMP
//RWD.7.99 for BRASSAGE/GRANULA
//#define LFACTOR                               (sizeof(long) / sizeof(short))
//or
#define LFACTOR                 (1)              /* RWD 4:2001 missing from tw2K verssion */

#define MIN_DB_ON_16_BIT        (-96.0)
/* RWD define positive too! */
#define MAX_DB_ON_16_BIT        (96.0)
#define MAXIMUM_SHORT           (32767)
#define ABSMAXIMUM_SHORT        (32768.0)
#define MINIMUM_SHORT           (-32768)

#define BIG_TIME                        (32767.0)

#define BIGARRAY                         (200)
#define FLTERR                           (0.000002)
#define CHARSIZE                         (8)
#define TWO_POW_15               (32768.0)
#define TWO_POW_14                       (16384)
//RWD floatsam versions  (???)
#define F_TWO_POW_15            F_MAXSAMP
#define F_TWO_POW_14            (F_MAXSAMP * 0.5)
/* PROBLEM : maxfloat applies only to PC and I'm not sure I got it right !! */

/******************* UNIVERSAL SPEC MACROS ***********************************/

#define AMPP                            (vc)     /* accesses an ampitude value in a float window */
#define FREQ                            (vc+1)   /* accesses a frequency value in a float window */

/* PROBLEM : maxfloat applies only to PC and I'm not sure I got it right !! */

#define SPAN_FACTOR     (1.5)

/******************* UNIVERSAL CONSTANTS AND MACROS ***********************************/
/* RWD need this! */
#ifndef PI
#define PI                                       (3.141592654)
#define TWOPI                           (2.0 * PI)
#endif



#define ROOT_2                           (1.414213562)
#define ONE_OVER_LN2             (1.442695)
#define LOG2(x)                          (log(x) * ONE_OVER_LN2)
#define LOG10_OF_2                       (0.301029995)
#define ENDOFSTR                         ('\0')
#define NEWLINE                          ('\n')
#define SEMITONES_PER_OCTAVE (12.0)
#define OCTAVES_PER_SEMITONE (0.08333333333)
#define SEMITONE_INTERVAL        (1.05946309436)
#define SEMITONE_DOWN            (0.9438743127)
#define SEMITONES_AS_RATIO(x)   (pow(SEMITONE_INTERVAL,(x)))
#define VERY_TINY_VAL            (0.00000000000000000001)  /* 10^-20 */
#define MINAMP                           (0.000000003)
#define BIGAMP                           (10000.0)
#define SPECIAL                          (0.00000000314159f)            /* marker value for specbare chans */

#define TIME_INTERVAL            (0.05)
#define MS_TO_SECS               (0.001)
#define SECS_TO_MS               (1000.0)

#define MINPITCH                         (9.0)          /* Pitch (Hz) corresponding, approx, to MIDI 0 */
#define LOW_A                            (6.875)        /* Frequency of A below MIDI 0  */
#ifndef MIDIMAX
#define MIDIMAX                          (127)          /* RWD 12:2003  written as doubles, but otherwise duplicates defs in columns.h */
#define MIDIMIN                          (0)
#endif
#define MAXWAVELEN               (.5)           /* seconds: for distort type progs */

#define MONO                             (1)
#define STEREO                           (2)

#define MU_MIN_DELAY            (0.1)
#define MU_MAX_DELAY            (100.0)

#define MU_MINTEMPO_DELAY       (1.0)
#define MU_MAXTEMPO_DELAY       (2000.0)

#define MU_MIN_TEMPO            (30)
#define MU_MAX_TEMPO            (1000)

/***************** CONSTANT CONNECTED WITH PITCHDATA *****/

#define DEFAULT_NYQUIST          (10000)        /* Fudge to allow brkpnt pitchdata to be tested */

/***************** CONSTANT CONNECTED WITH FORMANTS *****/

#define MINFBANDVAL                     (1)     /* min number of formant bands */
#define LOW_OCTAVE_BANDS                (4)
#define DESCRIPTOR_DATA_BLOKS   (2)

/***************** CONSTANT CONNECTED WITH MIXFILES *****/

#define MINPAN  (-32767.0)
#define MAXPAN  (32767.0)

#define PAN_LEFT        (-1.0)
#define PAN_RIGHT       (1.0)
#define PAN_CENTRE      (0.0)

#define MIX_MAXLINE             (7)
#define MIX_MIDLINE             (5)
#define MIX_MINLINE             (4)
#define MIX_NAMEPOS             (0)
#define MIX_TIMEPOS             (1)
#define MIX_CHANPOS             (2)
#define MIX_LEVELPOS    (3)
#define MIX_PANPOS              (4)
#define MIX_RLEVELPOS   (5)
#define MIX_RPANPOS             (6)

#define MIDDLE_C_MIDI   (60)

int     get_word_from_string(char **p,char **q);
int     get_float_from_within_string(char **str,double *val);
int     flteq(double f1,double f2);
void    swap(double *d1, double *d2);
void    iiswap(int *d1, int *d2);
double  miditohz(double val);
int     hztomidi(double *midi,double hz);
double  unchecked_hztomidi(double hz);
int     get_leveldb(char *str,double *val);
int     get_level(char *thisword,double *level);
int     is_dB(char *str);
double  dbtogain(double val);
int     is_an_empty_line_or_a_comment(char *p);
int     is_a_comment(char *p);
int     is_an_empty_line(char *p);
int     float_array(float **q,int size);
int     first_param_greater_than_second(int paramno1,int paramno2,dataptr dz);
int     first_param_not_less_than_second(int paramno1,int paramno2,dataptr dz);
int     establish_additional_brktable(dataptr dz);
void    reset_filedata_counters(dataptr dz);

int  open_first_infile(char *filename,dataptr dz);
int  handle_other_infile(int fileno,char *filename,dataptr dz);
int  get_maxvalue_in_brktable(double *brkmax,int paramno,dataptr dz);
int  get_minvalue_in_brktable(double *brkmin,int paramno,dataptr dz);
int  establish_file_data_storage_for_mix(int filecnt,dataptr dz);
int  establish_bufptrs_and_extra_buffers(dataptr dz);

int  setup_internal_arrays_and_array_pointers(dataptr dz);
int  assign_process_logic(dataptr dz);
void set_legal_infile_structure(dataptr dz);


/* BUFFERS */
int  allocate_large_buffers(dataptr dz);
int  create_iterbufs(double maxpscat,dataptr dz);
int  create_drunk_buffers(dataptr dz);

int  get_maxvalue(int paramno,double *maxval,dataptr dz);
int  convert_time_and_vals_to_samplecnts(int paramno,dataptr dz);
int  convert_time_to_samplecnts(int paramno,dataptr dz);
void delete_notes_here_and_beyond(noteptr startnote);
int  make_new_note(noteptr *thisnote);

/* PCONSISTENCY in distinct files */
int  filter_pconsistency(dataptr dz);
int  granula_pconsistency(dataptr dz);

int  param_preprocess(dataptr dz);

/* PREPROCESSING in distinct files */
int  distort_preprocess(dataptr dz);
int  distortenv_preprocess(dataptr dz);
int  distortmlt_preprocess(dataptr dz);
int  distortdiv_preprocess(dataptr dz);
int  distortshuf_preprocess(dataptr dz);
int  distortdel_preprocess(dataptr dz);
int  distortflt_preprocess(dataptr dz);
int  distorter_preprocess(int param1,int param2,int param3,dataptr dz);
int  texture_preprocess(dataptr dz);
int  zigzag_preprocess(dataptr dz);
int  loop_preprocess(dataptr dz);
int  scramble_preprocess(dataptr dz);
int  iterate_preprocess(dataptr dz);
int  drunk_preprocess(dataptr dz);
int  grain_preprocess(int gate_paramno,dataptr dz);
int  envel_preprocess(dataptr dz);
int  mix_preprocess(dataptr dz);
int  mixtwo_preprocess(dataptr dz);
int  mixcross_preprocess(dataptr dz);
int  get_inbetween_ratios(dataptr dz);
int  check_new_filename(char *filename,dataptr dz);
int  filter_preprocess(dataptr dz);
int  check_new_filename(char *filename,dataptr dz);

void initialise_random_sequence(int seed_flagno,int seed_paramno,dataptr dz);

/* SOUND INPUT-OUTPUT, TABLE READING AND GLOBAL FUNCS FOR PROCESSING */
/*int  read_bytes(char *bbuf,dataptr dz);*/
int  read_samps(float *bbuf,dataptr dz);
int  read_values_from_all_existing_brktables(double thistime,dataptr dz);
int  read_value_from_brktable(double thistime,int paramno,dataptr dz);

        /* spec */
int  get_totalamp(double *totalamp,float *sbuf,int wanted);
int  inner_loop(int *peakscore,int *descnt,int *in_start_portion,int *least,
                int *pitchcnt,int windows_in_buf,dataptr dz);
int  specbare(int *pitchcnt,dataptr dz);
int  normalise(double pre_totalamp,double post_totalamp,dataptr dz);
int  initialise_ring_vals(int thissize,double initial_amp,dataptr dz);
int  if_one_of_loudest_chans_store_in_ring(int vc,dataptr dz);
int  if_one_of_quietest_chans_store_in_ring(int vc,dataptr dz);
int  rearrange_ring_to_allow_new_entry_and_return_entry_address(chvptr *here,dataptr dz);
int  choose_bflagno_and_reset_mask_if_ness(int *bflagno,int cc,int *mask,int longpow2,int divmask);
int      move_data_into_appropriate_channel(int vc,int truevc,float thisamp,float thisfrq,dataptr dz);
int  move_data_into_some_appropriate_channel(int truevc,float thisamp,float thisfrq,dataptr dz);
int  construct_filter_envelope(int pkcnt_here,float *fbuf,dataptr dz);
int  spec_blur_and_bltr(dataptr dz);
int  get_amp_and_frq(float *floatbuf,dataptr dz);
int  put_amp_and_frq(float *floatbuf,dataptr dz);
int  get_amp(float *floatbuf,dataptr dz);
int  put_amp(float *floatbuf,dataptr dz);
int  get_statechanges(int avcnt,int scantableno,int avpitcharrayno,int statechangearrayno,
                                        double min_up_interval,double min_dn_interval,int datatype,dataptr dz);
int  rectify_frqs(float *floatbuf,dataptr dz);
int  advance_one_2fileinput_window(dataptr dz);
int  skip_or_special_operation_on_window_zero(dataptr dz);
int  move_along_formant_buffer(dataptr dz);
int  outer_twofileinput_loop(dataptr dz);
int  extract_formant_peaks2(int sl1param,int *thispkcnt,double lofrq_limit,double hifrq_limit,dataptr dz);
int      score_peaks(int *peakscore,int sl1_var,int stabl_var,dataptr dz);
int  collect_scores(int *cnt,int *descnt,dataptr dz);
int  sort_design(int no_of_design_elements,dataptr dz);
int  sort_equivalent_scores(int this_pkcnt,dataptr dz);
int  unscore_peaks(int *peakscore,dataptr dz);
int  specpfix(dataptr dz);
int  specrepitch(dataptr dz);

        /* filters */
void get_coeffs1(int n,dataptr dz);
void get_coeffs2(int n,dataptr dz);

int  file_has_invalid_startchar(char *filename);
int  file_has_reserved_extension(char *filename);
int  file_has_invalid_extension(char *filename);
int  derived_filetype(int filetype);

int  pvoc_preprocess(dataptr dz);

int  set_internalparam_data(const char *this_paramlist,aplptr ap);

int  convert_msecs_to_secs(int brktableno,dataptr dz);
int  establish_bottom_frqs_of_channels(dataptr dz);
int  establish_testtone_amps(dataptr dz);
int  setup_ring(dataptr dz);
int  setup_internal_bitflags(int bflag_array_no,int longpow,int divmask, dataptr dz);
int  log2_of_the_number_which_is_a_power_of_2(int *n,int k);
int two_to_the_power_of(int k);
int  init_bitflags_to_zero(int bflag_array_no,dataptr dz);
int  setup_stability_arrays_and_constants(int stability_val,int sval_less_one,dataptr dz);

int  zero_sound_buffers(dataptr dz);

int  initialise_window_frqs(dataptr dz);
int  read_both_files(int *windows_in_buf,int *got,dataptr dz);
int  outer_loop(dataptr dz);
int  gen_amplitude_in_lo_half_filterband(double *thisamp,double thisfrq,double filt_centre_frq,dataptr dz);
int  gen_amplitude_in_hi_half_filterband(double *thisamp,double thisfrq,double filt_centre_frq,dataptr dz);
int  filter_band_test(dataptr dz);

void convert_shiftp_vals(dataptr dz);

int create_sndbufs(dataptr dz);
int allocate_single_buffer(dataptr dz);
int allocate_double_buffer(dataptr dz);
int allocate_triple_buffer(dataptr dz);
int allocate_single_buffer_plus_extra_pointer(dataptr dz);
int lcm_for_buffers(unsigned int *lcm,int a,int b);
int allocate_analdata_plus_formantdata_buffer(dataptr dz);
int calculate_analdata_plus_formantdata_buffer(unsigned int *buffersize,dataptr dz);

int  get_process_no(char *prog_identifier_from_cmdline,dataptr dz);
int  get_mode_from_cmdline(char *str,dataptr dz);
int  get_process_no(char *prog_identifier_from_cmdline,dataptr dz);

int  establish_spec_bufptrs_and_extra_buffers(dataptr dz);
void setup_process_logic(int input,int process,int output,dataptr dz);

int do_the_bltr(int *last_total_windows,float *ampdif,float *freqdif,int blurfactor,dataptr dz);

void handle_pitch_zeros(dataptr dz);
int      check_depth_vals(int param_no,dataptr dz);

void rectify_window(float *flbuf,dataptr dz);
int  get_longer_file(dataptr dz);
//TW UPDATE
//int  keep_excess_bytes_from_correct_file(int *bytes_to_write,int file_to_keep,int got,int wc,dataptr dz);
int  keep_excess_samps_from_correct_file(int *bytes_to_write,int file_to_keep,int got,int wc,dataptr dz);
int  read_either_file(int *bytes_read,int file_to_keep,dataptr dz);

int sound_loom_in_use(int *argc, char **argv[]);

/* TEST ONLY */
int x(int y);

#define MAX_CYCLECNT    (32767.0)               /* arbitrary */
#define MAX_GRS_HVELOCITY       (32767.0)       /* max when using shorts, so has to be max */
#define BIG_VALUE               (32767.0)               /* arbitrary */
#define NOT_PITCH (-1.0)
#define NOT_SOUND (-2.0)

//TW FLOAT REVISIONS
/* replace MAXSAMP by a maximum range value, where it is used only for this purpose */
#define MAXRANGE (32767)
#define MINRANGE (-32768)
