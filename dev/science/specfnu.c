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
    SEE FILE temp.cpp for pitch-modification originals to import here!!!!

    _cdprogs\specfnu specfnu 23 alan_bellydancefbn.ana test.wav hague_trials\triadhf.txt 1 -n1 -o1 -p1 -q1 -S

    Spectral channels are dz->specenvamp && dz->specenvfrq
    dz->specenvamp[0] = (float)0.0;
    dz->specenvfrq[0] = (float)1.0;
    frqstep = dz->halfchwidth * (double)dz->formant_bands;
    Defaulting to 12 so we get a spectral channel widtth of c 260hz (and smaller window option give 86Hz bands)
*/

//#define   SEECHANGE 1

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <filetype.h>
#include <processno.h>
#include <modeno.h>
#include <logic.h>
#include <globcon.h>
#include <cdpmain.h>
#include <formants.h>
#include <math.h>
#include <mixxcon.h>
#include <osbind.h>
#include <standalone.h>
#include <science.h>
#include <speccon.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>

#ifdef unix
#define round lround
#endif

#define VIEWSCALE       (1.0e3)                 //  Multiplier enabling pvoc amplitudes to be easily read in any on-screen debug print

#define warned      is_rectified
#define pktrofcnt   rampbrksize
#define xclude_nonh temp_sampsize
#define badpks      fzeroset
#define suprflag    unspecified_filecnt //  Used by F_SUPPRESS and F_NARROW
#define timedhf     unspecified_filecnt //  Used by F_SINUS
#define phasefactor is_flat
#define bincnt      different_srates
#define quantcnt    finished
#define in_tune     is_sharp
#define fundamental numsize
#define avfrq       maxnum  
#define timeblokcnt sampswanted 
#define ischange    could_be_transpos   
#define shortwins   duplicate_snds
#define frq_step    scalefact
#define needpitch   could_be_pitch  
#define fsil_ratio  minnum
#define retain_unpitched_data_for_deletion is_transpos

//  PROCESSING PASSES

#define FPREANAL    0
#define FSPECENV    1
#define FPROCESS    2

//  ARRAY NAMES

//  integer array names
#define PEAKPOS     0   //  iparray[0]
#define PCNTBIN     1   //  iparray[1]
#define FCNT        2   //  iparray[2]
#define ISHARM      3   //  iparray[3]
#define IARRAYCNT   4   //  Number of iparrays
#define F_CHTRAIN1  4
#define F_CHTRAIN2  5
#define F_CHTRAIN3  6
#define F_CHTRAIN4  7
#define F_CHTRAIN5  8
#define F_CHTRAIN6  9
#define F_CHTRAIN7  10
#define F_CHTRAIN8  11
#define F_CHTRAIN9  12
#define IBIGARRAYCNT 13 //  Number of iparrays

//  double array names

#define PBINPCH     1   //  parray[1]   pitch extraction
#define PBINTOP     2   //  parray[2]
#define PBINAMP     3   //  parray[3]
#define PREPICH     4   //  parray[4]   sorting input pitch data
#define QUANTPITCH  5   //  parray[5]   pitch quantisation
#define FILTPICH    5   //  parray[5]   filter design
#define FILTAMP     6   //  parray[6]
#define LOCALPCH    7   //  parray[7]
#define LOCALAMP    8   //  parray[8]
#define FMULT       9   //  parray[9]   concatenation of moving formants
#define FPITCHES    9   //  parray[9]   All pitches actually used by varibank filter
#define FSPEC3      10  //  parray[10]

#define PRE_P_ARRAYCNT  11  //  Total number of arrays, excluding arrays needed for pitch extraction

#define P_PRETOTAMP 11  //  parray[11]  loudness envelope extraction (and pitch-smoothing)
#define RUNNINGAVRG 12  //  parray[12]  pitch smoothing
#define RUNAVGDURS  13  //  parray[13]

#define F_PINTMAP   14  //  parray[14]  stores interval-map for pitch-inversion over HFs

#define TOT_DBLARRAYCNT 15  //  Total number of double arrays

//  float array names

//  spec peaks-and-trofs data
#define PKTROF  0       //  fptr[0] peaks-and-trofs
#define PKDIFF  1       //  fptr[1] peak-trof differences
//  filter-line storage for varibank filter data in F_MAKEFILT
#define FILTLINE 2      //  fptr[2]
#define LASTFLINE 3     //  fptr[3]
//  peaks-and-trofs data for moved formants
#define FAMP1   2       //  fptr[2]
#define FAMP2   3       //  fptr[3]
#define FAMP3   4       //  fptr[4]
#define FAMP4   5       //  fptr[5]
#define FFRQ1   6       //  fptr[6]
#define FFRQ2   7       //  fptr[7]
#define FFRQ3   8       //  fptr[8]
#define FFRQ4   9       //  fptr[9]
#define FPCH1   10      //  fptr[10]
#define FPCH2   11      //  fptr[11]
#define FPCH3   12      //  fptr[12]
#define FPCH4   13      //  fptr[13]
//  arrays for storing formant pitch data
#define FOR_FRQ1 2      //  Alternative names 
#define FOR_FRQ2 3      //  for same arrays
#define FOR_FRQ3 4      //  when used by
#define FOR_FRQ4 5      //  F_SINUS
#define QOR_FRQ1 6  
#define QOR_FRQ2 7  
#define QOR_FRQ3 8  
#define QOR_FRQ4 9  
#define P_SINUS_ARRAYS 8//  Number of arrays used in storing and manipulating formant pitch-streams:
                        //  from F_AMP1 to QOR_FRQ4

#define PKSARRAYCNT 14  //  Number of float arrays for dealing with peak/trof detection & moving formants

//  harmonics detection
#define HMNICBOUNDS 14  //  fptr[14]
//  contour extraction
#define CONTOURFRQ  15  //  fptr[15]
#define CONTOURPCH  16  //  fptr[16]
#define CONTOURAMP  17  //  fptr[17]
#define CONTOURTOP  18  //  fptr[18]
#define CONTOURAMP2 19  //  fptr[19]

#define TOTAL_FLARRAYCNT    20  //  Total number of float array

//  int arrays

#define FTIMES      0   //  lparray[0]  Counting time-breaks in building filter : F_MAKEFILT
#define AVSTT       1   //  lparray[1]  Counting during continuity_smoothing of pitch data

#define TOTAL_LPARRAYCNT    2   //  Total number of int arrays

//  ESTABLISHING FORMANT BANDS AMD CONTOUR ENVELOPE SIZE
#define SPECFNU_SHORT_FBANDS 4              //  
#define SPECFNU_FBANDS      12              //  Choose value to give best peak separation (but not too many  peaks) This is no of HAlF channel-widths
#define SPECFNU_LONG_FBANDS 72
#define CLENGTH_DEFAULT     512             //  Default number of analysis points (AP): AP determines window-width,
//  NO OF PEAKS + TROFS TO STORE
#define PKBLOK              9               //  5 trofs and 4 peaks = 9 points
//  DETECTION OF FUNDAMENTAL
#define TWO_OVER_THREE      0.6666
//  DETECTION OF HARMONICS
#define WITHIN_RANGE        1               //  Semitone range for a partial to be considered a fundamental of the peak harmonic (or vice versa)
// PITCH PROCESSING
#define FMAXIMI          (8)                //  No of (max) peaks to use to check for harmonicity
#define FCHANSCAN        (8)                //  No of channels to scan looking for equivalent pitches at possible peak
#define FPEAK_LIMIT      (.05)
#define MAXIMUM_PARTIAL  (64)               //  Maximum partial to search for
#define ALMOST_TWO       (1.5)              //  Approximating 8ve leaps,  when looking for 8va leap anomalies
#define EIGHT_OVER_SEVEN (1.142857143)      //  Intervals within which pitch is "close" to existing frequency
#define SEVEN_OVER_EIGHT (0.875)
#define GOOD_MATCH       (5)                //  No of peaks that must be harmonics for window to register as pitched
#define FBLIPLEN         (2)                //  Number of adjacent windows that must be pitched for pitch not to be spurious is MORE THAN FBLIPLEN
#define FSILENCE_RATIO   (42)               //  Any window falling SILENCE_RATIO dBs below maximum window-level assumed to be "silence"
#define FPICH_HILM       (dz->nyquist/FMAXIMI)//High limit of pitch to search for (as we need FMAXIMI peaks to search for pitch)
#define FMIN_SMOOTH_SET  (3)                //  Minimum number of adjacent smooth pitches to imply true pitch present
#define FMAX_GLISRATE    (16.0)             // Assumptions: pitch can't move faster than 16 octaves per sec: FMAX_GLISRATE
                                            // Possible movement from window-to-window = FMAX_GLISRATE * dz->frametime
#define VOICEHI         (1500.0)            //  F# above C 2 8vas abive middle C
#define MVOICEHI        (500)               //  B above middle C
#define VOICELO         (48.0)              //  G below C 2 8vas below middle C
//  PITCH INTERPOLATION
#define FMID_PITCH        (0)
#define FFIRST_PITCH      (1)
#define FEND_PITCH        (2)
#define FNO_PITCH         (3)
#define TOO_SHORT_PICH    (0.1)             //  Segment of pitchedness within line must be longer than this, to register
#define TOO_SHORT_UNPICH  (0.05)            //  Segment of unpitchedness within line must be longer than this, to register  
//  SPECTRUM PITCH-TRANSPOSITION
#define FSPEC_MINFRQ      (9.0)

//  F_SEE
#define SPECFNU_MAXLEVEL  (0.95)            //  Max level for normalisation
#define SPECFNU_BLOKCNT   7                 //  Number of samples of equal value forming a block for display in output of F_SEE
//  F_NARROW
#define F4_ZEROED         8                 //  Last formant(4) being zeroed is flagged by 4th bit in suprflag (1000) = 8
//  F_MAKEFILE
#define MIDIOCTSPAN       11                //  0-127 = 10 octaves (120) + 7semitones. so 11 octaves spans the range
#define MINWINDOWS        60                //  Min no of windows over which a filter can be defined
#define FILTER_STAGGER    0.05              //  Time between filter settings, when filters change
#define FILTER_BAD_LEAP (SEMITONES_PER_OCTAVE * 1.5)
//  F_MOVE(2)
#define F_LOFRQ_LIMIT    (1.0)
#define F_HIFRQ_LIMIT    (dz->nyquist/2.0)
//  F_SYLABTROF
//  COLORING ARPEGGIATION
#define ARPEG_RANGE     7.0                 //  Total number of partials being arpeggiated is 8 (i.e 0 to 7)
#define ARPEG_WIN_WIDTH 4.0                 //  Number of partials under highlighting window
#define ARPEG_WIN_HALFWIDTH (ARPEG_WIN_WIDTH/2.0)   //  Halfwifth highlighting window
//  SLOPE OF TAILOFF OF EXTRA PARTIALS ADDED AT SPECTRUM TOP
#define TAILOFF_SLOPE   4.0

#define INTERVAL_MAPPING    (29)            /* FILE OR VAL */   /* special_data : Map interval-->interval for spec pinvert */

#define PSEUDOCTMIN     (4)     //  Size range, in semitones, of any pseudo_octave used in tuning, or quantiosation spec
#define PSEUDOCTMAX     (48)
#define MAXSCALECNT     (100)   //  Max number of divisions of octave, in tuning data

#define VERY_SMOOTH     (0.3)   //  Very smooth pitchline
#define STABLE_DUR      (0.2)   //  Min duration of (quantised) pitch, which is NOT an ornament
#define ORNAMENT_DUR    (0.1)   //  Min duration of (quantised) pitch, which could be an ornament
#define PCHANGE_DUR     (0.05)  //  Time to shift pitch from one pitch to another
#define PRAND_TRANGE    (2.0)   //  Time-slot where rand-val of pitch-variation stays fixed, ranges from a minimum TO (min + 2*min)
        
#define PRAND_TSTEP     (0.05)
#define PRAND_ORN_TSTEP (0.015) 

#define MINPITCHMIDI    (1.662783)  //  MIDI val corresponding to MINPITCH


// formant frequencies and amplitudes for 1st 3 formants : might be useful : NOT USED as at MAY 15: 2016
#if 0
static double         heed[6] = {300,2600,2800,1.0,0.85,0.85};
static double          hid[6] = {360,2100,2300,1.0,1.0, 1.0 };
static double educAtedScot[6] = {500,2250,2450,1.0,0.92,0.25};
static double         laid[6] = {465,2250,2450,1.0,0.92,0.25};
static double         head[6] = {570,1970,2450,1.0,1.0, 0.75};
static double          had[6] = {750,1550,2450,1.0,1.0, 0.85};
static double         hard[6] = {680,1100,2450,1.0,1.0, 0.92};
static double          hod[6] = {600,900, 2450,1.0,1.0, 0.5 };
static double        horde[6] = {450,860, 2300,1.0,1.0, 0.82};
static double cokeNorthern[6] = {410,810, 2300,1.0,1.0, 0.82};
static double  mudNorthern[6] = {380,850, 2100,1.0,1.0, 0.6 };
static double     youscots[6] = {380,850, 2800,1.0,0.85,0.85};
static double   couldscots[6] = {380,1000,2450,1.0,0.92,0.25};
static double         mood[6] = {300,850, 2100,1.0,1.0, 0.6 };
static double  hubSouthern[6] = {720,1240,2300,1.0,0.85,0.3 };
static double         herd[6] = {580,1380,2450,1.0,1.0, 0.5 };
static double          nnn[6] = {580,1380,2625,1.0,0.0, 0.1 };
static double         mmmm[6] = {580,850, 2100,1.0,0.0, 0.0 };
static double      rrrflat[6] = {880,2100,2450,1.0,0.2, 0.0 };
static double       ththth[6] = {400,1000,2450,1.0,0.3, 0.2 };
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "8.0.0";

/* CDP LIBRARY FUNCTIONS TRANSFERRED HERE */

static int  set_param_data(aplptr ap, int special_data,int maxparamcnt,int paramcnt,char *paramlist);
static int  set_vflgs(aplptr ap,char *optflags,int optcnt,char *optlist,
                char *varflags,int vflagcnt, int vparamcnt,char *varlist);
static int  setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int  initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int  mark_parameter_types(dataptr dz,aplptr ap);
static int  establish_application(dataptr dz);
static int  application_init(dataptr dz);
static int  initialise_vflags(dataptr dz);
static int  setup_input_param_defaultval_stores(int tipc,aplptr ap);
static int  setup_and_init_input_param_activity(dataptr dz,int tipc);
static int  get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static int  assign_file_data_storage(int infilecnt,dataptr dz);

/* CDP LIB FUNCTION MODIFIED TO AVOID CALLING setup_particular_application() */

static int  parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);

/* SIMPLIFICATION OF LIB FUNC TO APPLY TO JUST THIS FUNCTION */

static int  parse_infile_and_check_type(char **cmdline,dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
static int  setup_the_application(dataptr dz);
static int  setup_the_param_ranges_and_defaults(dataptr dz);
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int  get_the_mode_no(char *str, dataptr dz);
static int check_specfnu_param_validity_and_consistency(dataptr dz);
static int new_inner_loop(int windows_in_buf,double *fmax,int passno,int limit,int *inner_lpcnt,double *phase,int *times_index,
                          double *target,int *previouspk,int contourcnt,int arp_param,int *up,double minpitch,double maxpitch,dataptr dz);
static int set_the_specenv_frqs(int arraycnt,dataptr dz);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static void force_extension(int type,char *filename);
static int outer_specfnu_loop(int contourcnt,dataptr dz);
static int setup_formant_arrays(int *contourcnt,dataptr dz);
static int formants_narrow(int inner_lpcnt,dataptr dz);
static int formants_squeeze(int inner_lpcnt,dataptr dz);
static int formants_invert(int inner_lpcnt,double *phase,int contourcnt,dataptr dz);
static int formants_rotate(int inner_lpcnt,double *phase,dataptr dz);
static int formants_see(dataptr dz);
static int formants_seepks(dataptr dz);
static int formants_negate(int contourcnt,dataptr dz);
static int formants_suppress(int inner_lpcnt,dataptr dz);
static int formants_makefilt(int inner_lpcnt,int *times_index,dataptr dz);
static int formants_move(int inner_lpcnt,dataptr dz);

//static int get_the_formant_sample_points(dataptr dz);
static int specfnu_preprocess(dataptr dz);
static int initialise_peakstore(dataptr dz);
static int get_peaks_and_trofs(int inner_lpcnt,int limit,int *previouspk,dataptr dz);
static int usage4(int mode);
static int get_fmax(double *fmax,dataptr dz);
static int handle_the_makefilt_special_data(char *str,dataptr dz);
static int handle_the_pquantise_special_data(char *str,dataptr dz);
static int sort_pitches(dataptr dz);
static int build_filter(int times_index,dataptr dz);
static int find_fundamental(float thisfrq,double target,int peakcc,float *newfrq,dataptr dz);
static int equivalent_pitches(double frq1, double frq2, dataptr dz);
static int move_formant(int fno,double frqoffset,int lotrofchan,int peakchan,int hitrofchan,float lotrofamp,float peakamp,float hitorfamp,int *truecnt,dataptr dz);
static int concatenate_moved_formants(dataptr dz);
static int edgefade_formant(float *thisfrq,float *thisamp,float *thispch,int hi,
        int *nuchanlen, int lotrofchan,int peakchan,int hitrofchan,float lotrofamp,float peakamp, float hitrofamp,float frqlimit,dataptr dz);
static int getspecenv3amp(double frq,double *amp,float *thispch,float *thisamp,int speccnt,dataptr dz);
static int fwindow_size(dataptr dz);

//  PITCH RELATED PROCESSING

static int locate_channel_of_fundamental(int inner_lpcnt,float *fundfrq, int *newcc,dataptr dz);
static int continuity_smoothing(dataptr dz);

//  FIND PITCH

static int  initialise_pitchwork(int is_launched,dataptr dz);
static int  establish_arrays_for_pitchwork(dataptr dz);
static int  specpitch(int inner_lpcnt,double *target,dataptr dz);
static int  close_to_frq_already_in_ring(chvptr *there,double frq1,dataptr dz);
static int  substitute_in_ring(int vc,chvptr here,chvptr there,dataptr dz);
static int  insert_in_ring(int vc, chvptr here, dataptr dz);
static int  put_ring_frqs_in_ascending_order(chvptr **partials,float *minamp,dataptr dz);
static int  found_pitch(chvptr *partials,double lo_loud_partial,double hi_loud_partial,float minamp,double *thepitch,dataptr dz);
//static int  found_pitch_1(chvptr *partials,double lo_loud_partial,double hi_loud_partial,float minamp,dataptr dz);
//static int  found_pitch_2(chvptr *partials,dataptr dz);
static int  smooth_spurious_octave_leaps(int pitchno,float minamp,dataptr dz);
static int  equivalent_pitches(double frq1, double frq2, dataptr dz);
static int  is_peak_at(double frq,int window_offset,float minamp,dataptr dz);
static int  enough_partials_are_harmonics(chvptr *partials,double thepitch,dataptr dz);
static int  is_equivalent_pitch(double frq1,double frq2,dataptr dz);
static int  is_above_equivalent_pitch(double frq1,double frq2,dataptr dz);

//static int    do_smooth(int n, double *slopechange,dataptr dz);
static int  local_peak(int thiscc,double frq, float *thisbuf, dataptr dz);
static int  locate_channel_of_pitch(int inner_lpcnt,float thepitch, int *newcc,dataptr dz);

//  MASSAGE PITCH DATA

static int  tidy_up_pitch_data(dataptr dz);
static int  anti_noise_smoothing(int wlength,float *pitches,float frametime);
static int  is_smooth_from_both_sides(int n,double max_pglide,float *pitches);
static int  is_initialpitch_smooth(char *smooth,double max_pglide,float *pitches);
static int  is_finalpitch_smooth(char *smooth,double max_pglide,int wlength,float *pitches);
static int  is_smooth_from_before(int n,char *smooth,double max_pglide,float *pitches);
static int  is_smooth_from_after(int n,char *smooth,double max_pglide,float *pitches);
static int  test_glitch_sets(char *smooth,double max_pglide,int wlength,float *pitches);
static void remove_unsmooth_pitches(char *smooth,int wlength,float *pitches);
static int  test_glitch_forwards(int gltchstart,int gltchend,char *smooth,double max_pglide,float *pitches);
static int  test_glitch_backwards(int gltchstart,int gltchend,char *smooth,
                double max_pglide,int wlength,float *pitches);
static int  eliminate_blips_in_pitch_data(dataptr dz);
static int  mark_zeros_in_pitchdata(dataptr dz);
static int  pitch_found(dataptr dz);
static int  averages_smooth(int bracketed,int bracket,int runavcnt,dataptr dz);

//  INTERPOLATE PITCH DATA

static int    interpolate_pitch(dataptr dz);
static int    do_interpolating(int *pitchno,int skip_silence,dataptr dz);
static double hz_to_pitchheight(double frqq);
static double pitchheight_to_hz(double pitch_height);

//  OTHER PITCH-RELATED FUNCTIONS

static int exclude_non_harmonics(dataptr dz);
static int shufflup_harmonics_bounds(int *top_hno,float the_fundamental,int boundscnt,dataptr dz);
static int channel_holds_harmonic(float the_fundamental,float frq,int cc,int *top_hno,dataptr dz);
static int initialise_contourenv(int *contourcnt,int contourbands,dataptr dz);
static int extract_contour(int contourcnt,dataptr dz);
static int getcontouramp(double *thisamp,int contourcnt,double thisfrq,dataptr dz);
static int suppress_harmonics(dataptr dz);
static int getcontouramp2(double *thisamp,int contourcnt,double thisfrq,dataptr dz);
static int remember_contouramp(int contourcnt,dataptr dz);
static int trof_detect(dataptr dz);
static void zero_spectrum_top(int cc,dataptr dz);
static int is_coloring(dataptr dz);
static int formants_recolor(int inner_lpcnt,double *phase,int *up,int arp_param,double minpitch,double maxpitch,int contourcnt,dataptr dz);
static int arpeggiate(double *arpegamp,float frq,double bwidth,double lobandbot,double lobandtop,double hibandbot,double hibandtop,int is_split,dataptr dz);
static int find_and_change_partials(float the_fundamental,float newfrq,int *isharm,double frqshift,double frqtrans,double randomisation,
                             double bwidth,double lobandbot,double lobandtop,double hibandbot,double hibandtop,int do_arpegg,int is_split,dataptr dz);
static int do_top_of_spectrum_fill(int cc,float endamp,float the_fundamental,double frqtrans,double frqshift,int *tail,dataptr dz);
static int getmeanpitch(double *meanpitch,double *minpitch,double *maxpitch,dataptr dz);
static int interval_mapping(double *thisint,double thismidi,dataptr dz);
static int read_interval_mapping(char *str,dataptr dz);
static int get_and_count_data_from_textfile(char *filename,double **brktable,dataptr dz);
static int exag_pitchline(float *newfrq,float thisfrq,double minpich,double maxpich,dataptr dz);
static int pitch_invert(float *newfrq,float the_fundamental,dataptr dz);
static int pitch_quantise_all(dataptr dz);
static double pitch_quantise(double thismidi,double *pstt, double *pend,dataptr dz);
static int pitch_smooth(dataptr dz);
static int pitch_randomise_all(dataptr dz);
static int get_rand_interval(double *thisintv,dataptr dz);
static int randomise_pitchblok(int start_win,int min_wcnt,int *newwcnt,dataptr dz);
static int quantise_randomise_pitchblok(int n,int wcnt,double *pstt,double *pend,dataptr dz);
static int formants_smooth_and_quantise_all(dataptr dz);
static int formants_sinus(int inner_lpcnt,dataptr dz);
static int store_formant_frq_data(int inner_lpcnt,dataptr dz);
static int pitchline_smooth(float *pichline,int min_wcnt,int ismidi,dataptr dz);
static int smooth_note_to_note_transitions(float *pichline,dataptr dz);
static int sort_pitches_for_tvary_hf(int entrycnt,int linecnt,dataptr dz);
static int octaviate_and_store_timed_hf_data(dataptr dz);
static void strip_end_space(char *p);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status, n;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt, k;
    aplptr ap;
    int is_launched = FALSE;
    int contourcnt = 0;
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
                        /* CHECK FOR SOUNDLOOM */
    if((sloom = sound_loom_in_use(&argc,&argv)) > 1) {
        sloom = 0;
        sloombatch = 1;
    }
    if(sflinit("cdp")){
        sfperror("cdp: initialisation\n");
        return(FAILED);
    }
                          /* SET UP THE PRINCIPLE DATASTRUCTURE */
    if((exit_status = establish_datastructure(&dz))<0) {                    // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(!sloom) {
        if(argc == 1) {
            usage1();   
            return(FAILED);
        } else if(argc == 2) {
            usage2(argv[1]);    
            return(FAILED);
        } else if(argc == 3) {
            usage3(argv[1],argv[2]);    
            return(FAILED);
        }
        if((exit_status = make_initial_cmdline_check(&argc,&argv))<0) {     // CDP LIB
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdline    = argv;
        cmdlinecnt = argc;
        if((get_the_process_no(argv[0],dz))<0)
            return(FAILED);
        cmdline++;
        cmdlinecnt--;
        dz->maxmode = 23;
        if(cmdlinecnt <= 0) {
            sprintf(errstr,"Too few commandline parameters.\n");
            return(FAILED);
        }
        if((get_the_mode_no(cmdline[0],dz))<0) {
            if(!sloom)
                fprintf(stderr,"%s",errstr);
            return(FAILED);
        }
        cmdline++;
        cmdlinecnt--;
        exit_status = CONTINUE;
        switch(dz->mode) {
        case(F_NEGATE):     //  fall thro
        case(F_SEE):        //  fall thro
        case(F_SYLABTROF):  //  fall thro
        case(F_SEEPKS): if(cmdlinecnt < 2)  exit_status = FAILED;   break;
        case(F_NARROW):     //  fall thro
        case(F_ROTATE):     //  fall thro
        case(F_SUPPRESS):   //  fall thro
        case(F_ARPEG):      //  fall thro
        case(F_OCTSHIFT):   //  fall thro
        case(F_TRANS):      //  fall thro
        case(F_FRQSHIFT):   //  fall thro
        case(F_RESPACE):    //  fall thro
        case(F_PINVERT):    //  fall thro
        case(F_PQUANT):     //  fall thro 
        case(F_RAND):       //  fall thro
        case(F_INVERT): if(cmdlinecnt < 3)  exit_status = FAILED;   break;
        case(F_PEXAGG):     //  fall thro
        case(F_SINUS):      //  fall thro
        case(F_MAKEFILT):   //  fall thro
        case(F_SQUEEZE):if(cmdlinecnt < 4)  exit_status = FAILED;   break;
        case(F_PCHRAND):if(cmdlinecnt < 5)  exit_status = FAILED;   break;
        case(F_MOVE2):      //  fall thro
        case(F_MOVE):   if(cmdlinecnt < 6)  exit_status = FAILED;   break;
        default:        
            fprintf(stderr,"Unknown mode (%d) at 2nd usage check.\n",dz->mode+1);
            return PROGRAM_ERROR;
        }
        if(exit_status == FAILED) {
            usage4(dz->mode);
            return FAILED;
        }
        if((exit_status = setup_the_application(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {       // CDP LIB
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    } else {
        //parse_TK_data() =
        if((exit_status = parse_sloom_data(argc,argv,&cmdline,&cmdlinecnt,dz))<0) {
            exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);         
        }
    }
    ap = dz->application;
    // parse_infile_and_hone_type() = 
    if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_the_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // open_first_infile        CDP LIB
    if((exit_status = open_first_infile(cmdline[0],dz))<0) {    
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);  
        return(FAILED);
    }
    cmdlinecnt--;
    cmdline++;

    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

    if(!sloom) {
        exit_status = CONTINUE;
        switch(dz->mode) {
        case(F_SEE):        //  fall thro
        case(F_SYLABTROF):  //  fall thro
        case(F_SEEPKS):  break;
        case(F_NARROW):     //  fall thro
        case(F_ROTATE):     //  fall thro
        case(F_SUPPRESS):   //  fall thro
        case(F_ARPEG):      //  fall thro
        case(F_OCTSHIFT):   //  fall thro
        case(F_TRANS):      //  fall thro
        case(F_FRQSHIFT):   //  fall thro
        case(F_RESPACE):    //  fall thro
        case(F_PINVERT):    //  fall thro
        case(F_PQUANT):     //  fall thro 
        case(F_RAND):       //  fall thro
        case(F_INVERT):  if(cmdlinecnt < 1) exit_status = FAILED;   break;
        case(F_PEXAGG):     //  fall thro
        case(F_SINUS):      //  fall thro
        case(F_MAKEFILT):   //  fall thro
        case(F_SQUEEZE): if(cmdlinecnt < 2) exit_status = FAILED;   break;
        case(F_PCHRAND): if(cmdlinecnt < 3) exit_status = FAILED;   break;
        case(F_MOVE2):      //  fall thro
        case(F_MOVE):    if(cmdlinecnt < 4) exit_status = FAILED;   break;
        case(F_NEGATE):  break;
        default:        
            fprintf(stderr,"Unknown mode (%d) at 3rd usage check.\n",dz->mode+1);
            return PROGRAM_ERROR;
        }
        if(exit_status == FAILED) {
            usage4(dz->mode);
            return FAILED;
        }
    }
    dz->retain_unpitched_data_for_deletion = 0;
    if(dz->mode == F_SINUS) {
        if((dz->iparray = (int **)malloc(IBIGARRAYCNT * sizeof(int*)))==NULL) {
            fprintf(stdout,"ERROR: INSUFFICIENT MEMORY for stores of formant tracking data.\n");
            fflush(stdout);
            return(FAILED);
        }
        for(n=0;n<IBIGARRAYCNT;n++)
            dz->iparray[n] = NULL;
    } else {
        if((dz->iparray = (int **)malloc(IARRAYCNT * sizeof(int*)))==NULL) {
            fprintf(stdout,"ERROR: INSUFFICIENT MEMORY for store of locations  of peaks and troughs(1).\n");
            fflush(stdout);
            return(FAILED);
        }
    }
    if((dz->parray = (double **)malloc(TOT_DBLARRAYCNT * sizeof(double *)))==NULL) {
        fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store Filter Data.\n");
        fflush(stdout);
        return(FAILED);
    }
    for(n=0;n<TOT_DBLARRAYCNT;n++)
        dz->parray[n] = NULL;
    if((dz->fptr = (float **)malloc(TOTAL_FLARRAYCNT * sizeof(float *)))==NULL) {
        fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store Intermediate formant data.\n");
        fflush(stdout);
        return(FAILED);
    }
    for(n=0;n < TOTAL_FLARRAYCNT;n++)
        dz->fptr[n] = NULL;
    if((dz->lparray = (int **)malloc(TOTAL_LPARRAYCNT * sizeof(int *)))==NULL) {
        fprintf(stdout,"ERROR: INSUFFICIENT MEMORY to store Filter Times Data.(1)\n");
        fflush(stdout);
        return(FAILED);
    }

//  handle_formant_quiksearch() redundant
//  handle_special_data ....
    switch(dz->mode) {
    case(F_MAKEFILT):
        if((exit_status = handle_the_makefilt_special_data(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdlinecnt--;
        cmdline++;
        break;
    case(F_PINVERT):
        if((exit_status = read_interval_mapping(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdlinecnt--;
        cmdline++;
        break;
    case(F_PCHRAND): // fall thro
    case(F_PQUANT):
        if((exit_status = handle_the_pquantise_special_data(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdlinecnt--;
        cmdline++;
        break;
    case(F_SINUS):
        if((exit_status = handle_the_pquantise_special_data(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdlinecnt--;
        cmdline++;
        break;
    }

    if(!sloom && dz->mode == F_NARROW) {
        for(k = 0;k < cmdlinecnt;k++) {
            if(!strncmp(cmdline[k],"-s",2)) {
                if(strlen(cmdline[k]) > 2) {
                    fprintf(stderr,"'-s' flag is for \"Use short windows\". Use '-o' for \"Suppress listed formants\".\n");
                    return FAILED;
                }
            }
        }
    }
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //check_param_validity_and_consistency ....
    dz->fundamental = 0;                      
    dz->needpitch = 0;

    if((exit_status = check_specfnu_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = exclude_non_harmonics(dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = suppress_harmonics(dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(dz->xclude_nonh) {       //  To exclude non-harmonic components
        dz->fundamental = 1;    //  Need ot know the fundamental
        dz->needpitch = 1;      //  So need to do pitch-processing
    }
    if(dz->needpitch || dz->mode == F_SYLABTROF || dz->mode == F_SINUS) {
        if((exit_status = initialise_pitchwork(is_launched,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }   
    }
    //  handle_formants ..  Replace FORMANT BANDS param of previous formant-progs, with fixed val SPECFNU_FBANDS that works: val = length of specenv-window in 1/2chans.

    if((exit_status = fwindow_size(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(dz->shortwins)
        dz->formant_bands = SPECFNU_SHORT_FBANDS;
    else
        dz->formant_bands = SPECFNU_FBANDS;

    if(dz->mode == F_INVERT || dz->mode == F_NEGATE)
        contourcnt = 1;

    is_launched = TRUE;

    //allocate_large_buffers() ... 
    dz->extra_bufcnt =  1;
    dz->bptrcnt = 4;

    if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = setup_formant_arrays(&contourcnt,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((dz->windowbuf[0] = (float *)malloc(dz->wanted * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store harmonics markers.\n");
        return(MEMORY_ERROR);
    }
    if((exit_status = allocate_single_buffer(dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    dz->flbufptr[2] = dz->bigfbuf + dz->buflen; //  End of buffer marker

    //param_preprocess ....
    if((exit_status = specfnu_preprocess(dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //spec_process_file =
    if((exit_status = outer_specfnu_loop(contourcnt,dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = complete_output(dz))<0) {                                     // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    exit_status = print_messages_and_close_sndfiles(FINISHED,is_launched,dz);       // CDP LIB
    free(dz);
    return(SUCCEEDED);
}

/**********************************************
        REPLACED CDP LIB FUNCTIONS
**********************************************/

/****************************** SET_PARAM_DATA *********************************/

int set_param_data(aplptr ap, int special_data,int maxparamcnt,int paramcnt,char *paramlist)
{
    ap->special_data   = (char)special_data;       
    ap->param_cnt      = (char)paramcnt;
    ap->max_param_cnt  = (char)maxparamcnt;
    if(ap->max_param_cnt>0) {
        if((ap->param_list = (char *)malloc((size_t)(ap->max_param_cnt+1)))==NULL) {    
            sprintf(errstr,"INSUFFICIENT MEMORY: for param_list\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->param_list,paramlist); 
    }
    return(FINISHED);
}

/****************************** SET_VFLGS *********************************/

int set_vflgs
(aplptr ap,char *optflags,int optcnt,char *optlist,char *varflags,int vflagcnt, int vparamcnt,char *varlist)
{
    ap->option_cnt   = (char) optcnt;           /*RWD added cast */
    if(optcnt) {
        if((ap->option_list = (char *)malloc((size_t)(optcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for option_list\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->option_list,optlist);
        if((ap->option_flags = (char *)malloc((size_t)(optcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for option_flags\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->option_flags,optflags); 
    }
    ap->vflag_cnt = (char) vflagcnt;           
    ap->variant_param_cnt = (char) vparamcnt;
    if(vflagcnt) {
        if((ap->variant_list  = (char *)malloc((size_t)(vflagcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for variant_list\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->variant_list,varlist);       
        if((ap->variant_flags = (char *)malloc((size_t)(vflagcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for variant_flags\n");
            return(MEMORY_ERROR);
        }
        strcpy(ap->variant_flags,varflags);

    }
    return(FINISHED);
}

/***************************** APPLICATION_INIT **************************/

int application_init(dataptr dz)
{
    int exit_status;
    int storage_cnt;
    int tipc, brkcnt;
    aplptr ap = dz->application;
    if(ap->vflag_cnt>0)
        initialise_vflags(dz);    
    tipc  = ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt;
    ap->total_input_param_cnt = (char)tipc;
    if(tipc>0) {
        if((exit_status = setup_input_param_range_stores(tipc,ap))<0)             
            return(exit_status);
        if((exit_status = setup_input_param_defaultval_stores(tipc,ap))<0)        
            return(exit_status);
        if((exit_status = setup_and_init_input_param_activity(dz,tipc))<0)    
            return(exit_status);
    }
    brkcnt = tipc;
    if(brkcnt>0) {
        if((exit_status = setup_and_init_input_brktable_constants(dz,brkcnt))<0)              
            return(exit_status);
    }
    if((storage_cnt = tipc + ap->internal_param_cnt)>0) {         
        if((exit_status = setup_parameter_storage_and_constants(storage_cnt,dz))<0)   
            return(exit_status);
        if((exit_status = initialise_is_int_and_no_brk_constants(storage_cnt,dz))<0)      
            return(exit_status);
    }                                                      
    if((exit_status = mark_parameter_types(dz,ap))<0)     
            return(exit_status);
    
    // establish_infile_constants() replaced by
    dz->infilecnt = ONE_NONSND_FILE;
    //establish_bufptrs_and_extra_buffers():
    return(FINISHED);
}

/******************************** SETUP_AND_INIT_INPUT_BRKTABLE_CONSTANTS ********************************/

int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt)
{   
    int n;
    if((dz->brk      = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 1\n");
        return(MEMORY_ERROR);
    }
    if((dz->brkptr   = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 6\n");
        return(MEMORY_ERROR);
    }
    if((dz->brksize  = (int    *)malloc(brkcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 2\n");
        return(MEMORY_ERROR);
    }
    if((dz->firstval = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 3\n");
        return(MEMORY_ERROR);                                                 
    }
    if((dz->lastind  = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 4\n");
        return(MEMORY_ERROR);
    }
    if((dz->lastval  = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 5\n");
        return(MEMORY_ERROR);
    }
    if((dz->brkinit  = (int     *)malloc(brkcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 7\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<brkcnt;n++) {
        dz->brk[n]     = NULL;
        dz->brkptr[n]  = NULL;
        dz->brkinit[n] = 0;
        dz->brksize[n] = 0;
    }
    return(FINISHED);
}

/********************** SETUP_PARAMETER_STORAGE_AND_CONSTANTS ********************/
/* RWD mallo changed to calloc; helps debug verison run as release! */

int setup_parameter_storage_and_constants(int storage_cnt,dataptr dz)
{
    if((dz->param       = (double *)calloc(storage_cnt, sizeof(double)))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 1\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparam      = (int    *)calloc(storage_cnt, sizeof(int)   ))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 2\n");
        return(MEMORY_ERROR);
    }
    if((dz->is_int      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 3\n");
        return(MEMORY_ERROR);
    }
    if((dz->no_brk      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 5\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/************** INITIALISE_IS_INT_AND_NO_BRK_CONSTANTS *****************/

int initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz)
{
    int n;
    for(n=0;n<storage_cnt;n++) {
        dz->is_int[n] = (char)0;
        dz->no_brk[n] = (char)0;
    }
    return(FINISHED);
}

/***************************** MARK_PARAMETER_TYPES **************************/

int mark_parameter_types(dataptr dz,aplptr ap)
{
    int n, m;                           /* PARAMS */
    for(n=0;n<ap->max_param_cnt;n++) {
        switch(ap->param_list[n]) {
        case('0'):  break; /* dz->is_active[n] = 0 is default */
        case('i'):  dz->is_active[n] = (char)1; dz->is_int[n] = (char)1;dz->no_brk[n] = (char)1; break;
        case('I'):  dz->is_active[n] = (char)1; dz->is_int[n] = (char)1;                         break;
        case('d'):  dz->is_active[n] = (char)1;                         dz->no_brk[n] = (char)1; break;
        case('D'):  dz->is_active[n] = (char)1; /* normal case: double val or brkpnt file */     break;
        default:
            sprintf(errstr,"Programming error: invalid parameter type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }                               /* OPTIONS */
    for(n=0,m=ap->max_param_cnt;n<ap->option_cnt;n++,m++) {
        switch(ap->option_list[n]) {
        case('i'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1; dz->no_brk[m] = (char)1; break;
        case('I'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;                          break;
        case('d'): dz->is_active[m] = (char)1;                          dz->no_brk[m] = (char)1; break;
        case('D'): dz->is_active[m] = (char)1;  /* normal case: double val or brkpnt file */     break;
        default:
            sprintf(errstr,"Programming error: invalid option type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }                               /* VARIANTS */
    for(n=0,m=ap->max_param_cnt + ap->option_cnt;n < ap->variant_param_cnt; n++, m++) {
        switch(ap->variant_list[n]) {
        case('0'): break;
        case('i'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1; dz->no_brk[m] = (char)1; break;
        case('I'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;                          break;
        case('d'): dz->is_active[m] = (char)1;                          dz->no_brk[m] = (char)1; break;
        case('D'): dz->is_active[m] = (char)1; /* normal case: double val or brkpnt file */      break;
        default:
            sprintf(errstr,"Programming error: invalid variant type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }                               /* INTERNAL */
    for(n=0,
    m=ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt; n<ap->internal_param_cnt; n++,m++) {
        switch(ap->internal_param_list[n]) {
        case('0'):  break;   /* dummy variables: variables not used: but important for internal paream numbering!! */
        case('i'):  dz->is_int[m] = (char)1;    dz->no_brk[m] = (char)1;    break;
        case('d'):                              dz->no_brk[m] = (char)1;    break;
        default:
            sprintf(errstr,"Programming error: invalid internal param type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/***************************** HANDLE_THE_OUTFILE **************************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz)
{
    int exit_status, len, orig_chans = 1;
    char *filename = NULL;
    len = strlen((*cmdline)[0]);
    if((filename = (char *)malloc(len + 8))==NULL) {
        sprintf(errstr,"handle_the_outfile()\n");
        return(MEMORY_ERROR);
    }
    strcpy(filename,(*cmdline)[0]);

    if(dz->mode == F_SEEPKS || dz->mode == F_SYLABTROF || dz->mode == F_MAKEFILT)
        force_extension(1,filename);
    else
        force_extension(0,filename);
    strcpy(dz->outfilename,filename);
    if(dz->mode == F_SEE) {
        orig_chans = dz->infile->channels;
        dz->infile->channels = 1;
    }
    if((exit_status = create_sized_outfile(filename,dz))<0)
        return(exit_status);
    if(dz->mode == F_SEE)
        dz->infile->channels = orig_chans;
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/***************************** ESTABLISH_APPLICATION **************************/

int establish_application(dataptr dz)
{
    aplptr ap;
    if((dz->application = (aplptr)malloc(sizeof (struct applic)))==NULL) {
        sprintf(errstr,"establish_application()\n");
        return(MEMORY_ERROR);
    }
    ap = dz->application;
    memset((char *)ap,0,sizeof(struct applic));
    return(FINISHED);
}

/************************* INITIALISE_VFLAGS *************************/

int initialise_vflags(dataptr dz)
{
    int n;
    if((dz->vflag  = (char *)malloc(dz->application->vflag_cnt * sizeof(char)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: vflag store,\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->application->vflag_cnt;n++)
        dz->vflag[n]  = FALSE;
    return FINISHED;
}

/************************* SETUP_INPUT_PARAM_DEFAULTVALS *************************/

int setup_input_param_defaultval_stores(int tipc,aplptr ap)
{
    int n;
    if((ap->default_val = (double *)malloc(tipc * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for application default values store\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<tipc;n++)
        ap->default_val[n] = 0.0;
    return(FINISHED);
}

/***************************** SETUP_AND_INIT_INPUT_PARAM_ACTIVITY **************************/

int setup_and_init_input_param_activity(dataptr dz,int tipc)
{
    int n;
    if((dz->is_active = (char   *)malloc((size_t)tipc))==NULL) {
        sprintf(errstr,"setup_and_init_input_param_activity()\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<tipc;n++)
        dz->is_active[n] = (char)0;
    return(FINISHED);
}

/************************* SETUP_THE_APPLICATION *******************/

int setup_the_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    switch(dz->mode) {
        //  resynth modes
    case(F_NARROW):  exit_status = set_param_data(ap,0    ,2,1,"D0");   break;
    case(F_SQUEEZE): exit_status = set_param_data(ap,0    ,2,2,"Di");   break;
    case(F_INVERT):  exit_status = set_param_data(ap,0    ,2,1,"D0");   break;
    case(F_ROTATE):  exit_status = set_param_data(ap,0    ,2,1,"D0");   break;
    case(F_NEGATE):  exit_status = set_param_data(ap,0    ,2,0,"00");   break;
    case(F_SUPPRESS):exit_status = set_param_data(ap,0    ,2,1,"i0");   break;
        //  text-out mode
    case(F_MAKEFILT):exit_status = set_param_data(ap,FFILT,2,1,"i0");   break;
        //  resynth modes
    case(F_MOVE):    exit_status = set_param_data(ap,0    ,4,4,"DDDD"); break;
    case(F_MOVE2):   exit_status = set_param_data(ap,0    ,4,4,"DDDD"); break;
    case(F_ARPEG):   exit_status = set_param_data(ap,0    ,2,1,"D0");   break;
    case(F_OCTSHIFT):exit_status = set_param_data(ap,0    ,2,1,"I0");   break;
    case(F_TRANS):   exit_status = set_param_data(ap,0    ,2,1,"D0");   break;
    case(F_FRQSHIFT):exit_status = set_param_data(ap,0    ,2,1,"D0");   break;
    case(F_RESPACE): exit_status = set_param_data(ap,0    ,2,1,"D0");   break;
    case(F_PINVERT): exit_status = set_param_data(ap,INTERVAL_MAPPING,2,1,"D0");    break;
    case(F_PEXAGG):  exit_status = set_param_data(ap,0    ,2,2,"DD");   break;
    case(F_PQUANT):  exit_status = set_param_data(ap,HFIELD,2,0,"00");  break;
    case(F_PCHRAND): exit_status = set_param_data(ap,HFIELD_OR_ZERO,2,2,"DD");  break;
    case(F_RAND):    exit_status = set_param_data(ap,0    ,2,1,"D0");   break;
        //  psuedosnd-out mode
    case(F_SEE):        break;
        //  text-out modes
    case(F_SEEPKS):     break;
    case(F_SYLABTROF):  break;
        //  resynth modes
    case(F_SINUS):   exit_status = set_param_data(ap,HFIELD_OR_ZERO,2,1,"D0");  break;
    default:         
        fprintf(stderr,"Unknown mode at setup_the_application()\n");
        return PROGRAM_ERROR;
    }
    if(exit_status < 0)
        return(FAILED);
    switch(dz->mode) {
        //  resynth modes
    case(F_NARROW):   exit_status = set_vflgs(ap,"go",2,"di","tfsxkr",6,0,"000000");break;
    case(F_SQUEEZE):  exit_status = set_vflgs(ap,"g", 1,"d", "tfsxkr",6,0,"000000");break;
    case(F_INVERT):   exit_status = set_vflgs(ap,"g", 1,"d", "sxkr",  4,0,"0000");  break;
    case(F_ROTATE):   exit_status = set_vflgs(ap,"g", 1,"d", "sxkr",  4,0,"0000");  break;
    case(F_NEGATE):   exit_status = set_vflgs(ap,"g", 1,"d", "f",     1,0,"0");     break;
    case(F_SUPPRESS): exit_status = set_vflgs(ap,"g", 1,"d", "sx",    2,0,"00");    break;
        //  text-out modes
    case(F_MAKEFILT): exit_status = set_vflgs(ap,"b", 1,"d", "kifs",  4,0,"0000");  break;
        //  resynth modes
    case(F_MOVE):     exit_status = set_vflgs(ap,"g", 1,"d", "tsxkr", 5,0,"00000"); break;
    case(F_MOVE2):    exit_status = set_vflgs(ap,"g", 1,"d", "tsnxkr",6,0,"000000");break;
    case(F_ARPEG):    exit_status = set_vflgs(ap,"g", 1,"d", "sxrdc", 5,0,"00000"); break;
    case(F_OCTSHIFT): exit_status = set_vflgs(ap,"glhp",4,"dddD","sxrdcf",6,0,"000000"); break;
    case(F_TRANS):    exit_status = set_vflgs(ap,"glhp",4,"dddD","sxrdcf",6,0,"000000"); break;
    case(F_FRQSHIFT): exit_status = set_vflgs(ap,"glhp",4,"dddD","sxrdcf",6,0,"000000"); break;
    case(F_RESPACE):  exit_status = set_vflgs(ap,"glhp",4,"dddD","sxrdcf",6,0,"000000"); break;
    case(F_PINVERT):  exit_status = set_vflgs(ap,"glhpbt",6,"dddDdd","sxrdc", 5,0,"00000"); break;
    case(F_PEXAGG):   exit_status = set_vflgs(ap,"glhpbt",6,"dddDdd","sxrdcTFMAB",10,0,"000000000");    break;
    case(F_PQUANT):   exit_status = set_vflgs(ap,"glhpbt",6,"dddDdd","sxrdcon", 7,0,"0000000"); break;
    case(F_PCHRAND):  exit_status = set_vflgs(ap,"glhpbt",6,"dddDdd","sxrdconk", 8,0,"00000000");   break;
    case(F_RAND):     exit_status = set_vflgs(ap,"glhp",4,"dddD","sxrdc", 5,0,"00000"); break;
        //  psuedosnd-out mode
    case(F_SEE):      exit_status = set_vflgs(ap,"",  0,"" , "s",     1,0,"0");     break;
        //  text-out modes
    case(F_SEEPKS):   exit_status = set_vflgs(ap,"",  0,"" , "s",     1,0,"0");     break;
    case(F_SYLABTROF):exit_status = set_vflgs(ap,"sp",2,"dd","PB",    2,0,"00");    break;
        //  resynth modes
    case(F_SINUS):    exit_status = set_vflgs(ap,"abcdeqpon",9,"dDDDDDDDD","sfrS",4,0,"0000");break;
    }
    if(exit_status < 0)
        return(FAILED);

    dz->has_otherfile   = FALSE;
    dz->input_data_type = ANALFILE_ONLY;
    switch(dz->mode) {
    case(F_SEE):
        dz->process_type    = PSEUDOSNDFILE;
        dz->outfiletype     = SNDFILE_OUT;
        break;
    case(F_SEEPKS):     //  fall thro
    case(F_SYLABTROF):  //  fall thro
    case(F_MAKEFILT):   
        dz->process_type    = TO_TEXTFILE;
        dz->outfiletype     = TEXTFILE_OUT;
        break;
    default:
        dz->process_type    = BIG_ANALFILE;     //  Add zero windowS at end, to avoid click
        dz->outfiletype     = ANALFILE_OUT;
    }
    return application_init(dz);    //GLOBAL
}

/************************* PARSE_INFILE_AND_CHECK_TYPE *******************/

int parse_infile_and_check_type(char **cmdline,dataptr dz)
{
    int exit_status;
    infileptr infile_info;
    if(!sloom) {
        if((infile_info = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for infile structure to test file data.");
            return(MEMORY_ERROR);
        } else if((exit_status = cdparse(cmdline[0],infile_info))<0) {
            sprintf(errstr,"Failed to parse input file %s\n",cmdline[0]);
            return(PROGRAM_ERROR);
        } else if(infile_info->filetype != ANALFILE)  {
            sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
            return(DATA_ERROR);
        } else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
            sprintf(errstr,"Failed to copy file parsing information\n");
            return(PROGRAM_ERROR);
        }
        free(infile_info);
    }
    dz->clength     = dz->wanted / 2;
    dz->chwidth     = dz->nyquist/(double)(dz->clength-1);
    dz->halfchwidth = dz->chwidth/2.0;
    return(FINISHED);
}

/************************* SETUP_THE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_the_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!s
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    switch(dz->mode) {
    case(F_NARROW):
        ap->lo[NARROWING] = 1.0;
        ap->hi[NARROWING] = 1000.0;
        ap->default_val[NARROWING] = 2.0;
        ap->lo[NARSUPRES] = 0;
        ap->hi[NARSUPRES] = 234;
        ap->default_val[NARSUPRES] = 0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN] = 10.0;
        ap->default_val[FGAIN] = 1.0;
        break;
    case(F_SQUEEZE):
        ap->lo[SQZFACT] = 1.0;
        ap->hi[SQZFACT] = 10.0;
        ap->default_val[SQZFACT]    = 2.0;
        ap->lo[SQZAT]  = 1;
        ap->hi[SQZAT]  = 4;
        ap->default_val[SQZAT]  = 1;
        ap->lo[FGAIN]  = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN]  = 1.0;
        break;
    case(F_INVERT):
        ap->lo[FVIB] = 0.0;
        ap->hi[FVIB] = 300.0;
        ap->default_val[FVIB] = 0.0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN] = 10.0;
        ap->default_val[FGAIN] = 1.0;
        break;
    case(F_ROTATE):
        ap->lo[RSPEED] = -300.0;
        ap->hi[RSPEED] = 300.0;
        ap->default_val[RSPEED] = 1.0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        break;
    case(F_NEGATE):
        ap->lo[FGAIN]  = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN]  = 1.0;
        break;
    case(F_SUPPRESS):
        ap->lo[SUPRF] = 1;
        ap->hi[SUPRF] = 1234;
        ap->default_val[SUPRF]  = 1;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN] = 10.0;
        ap->default_val[FGAIN] = 1.0;
        break;
    case(F_MAKEFILT):
        ap->lo[FPKCNT] = 1;
        ap->hi[FPKCNT] = MAXFILTVALS;
        ap->default_val[FPKCNT] = 1;
        ap->lo[FBELOW] = 0;
        ap->hi[FBELOW] = 127;
        ap->default_val[FBELOW] = 0;
        break;
    case(F_MOVE):
        ap->lo[FMOVE1] = -8000;
        ap->hi[FMOVE1] = 8000.0;
        ap->default_val[FMOVE1] = 0.0;
        ap->lo[FMOVE2] = -8000;
        ap->hi[FMOVE2] = 8000.0;
        ap->default_val[FMOVE2] = 0.0;
        ap->lo[FMOVE3] = -8000;
        ap->hi[FMOVE3] = 8000.0;
        ap->default_val[FMOVE3] = 0.0;
        ap->lo[FMOVE4] = -8000;
        ap->hi[FMOVE4] = 8000.0;
        ap->default_val[FMOVE4] = 0.0;
        ap->lo[FMVGAIN] = 0.01;
        ap->hi[FMVGAIN] = 10.0;
        ap->default_val[FMVGAIN] = 1.0;
        break;
    case(F_MOVE2):
        ap->lo[FMOVE1] = 10;
        ap->hi[FMOVE1] = 8000.0;
        ap->default_val[FMOVE1] = 200;
        ap->lo[FMOVE2] = 10;
        ap->hi[FMOVE2] = 8000.0;
        ap->default_val[FMOVE2] = 1200;
        ap->lo[FMOVE3] = 10;
        ap->hi[FMOVE3] = 8000.0;
        ap->default_val[FMOVE3] = 2000;
        ap->lo[FMOVE4] = 10;
        ap->hi[FMOVE4] = 8000.0;
        ap->default_val[FMOVE4] = 3200;
        ap->lo[FMVGAIN] = 0.01;
        ap->hi[FMVGAIN] = 10.0;
        ap->default_val[FMVGAIN] = 1.0;
        break;
    case(F_SYLABTROF):
        ap->lo[FMINSYL] = .05;
        ap->hi[FMINSYL] = .5;
        ap->default_val[FMINSYL] = MIN_SYLLAB_DUR;      //  0.08
        ap->lo[FMINPKG] = 0.01;
        ap->hi[FMINPKG] = 0.2;
        ap->default_val[FMINPKG] = MIN_PEAKTROF_GAP;    //  0.08
        break;
    case(F_ARPEG):
        ap->lo[FARPRATE] = -50.0;
        ap->hi[FARPRATE] =  50.0;
        ap->default_val[FARPRATE] = 1.0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        break;
    case(F_OCTSHIFT):
        ap->lo[COLINT] = -4.0;
        ap->hi[COLINT] =  4.0;
        ap->default_val[COLINT] = 1.0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        ap->lo[COL_LO] =  0.0;
        ap->hi[COL_LO] =  10000.0;
        ap->default_val[COL_LO] = 0.0;
        ap->lo[COL_HI] =  50.0;
        ap->hi[COL_HI] =  10000.0;
        ap->default_val[COL_HI] = 0.0;
        ap->lo[COLRATE] = -50.0;
        ap->hi[COLRATE] =  50.0;
        ap->default_val[COLRATE] = 0.0;
        break;
    case(F_TRANS):
        ap->lo[COLFLT] = -48.0;
        ap->hi[COLFLT] =  48.0;
        ap->default_val[COLFLT] = 1.0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        ap->lo[COL_LO] =  0.0;
        ap->hi[COL_LO] =  10000.0;
        ap->default_val[COL_LO] = 0.0;
        ap->lo[COL_HI] =  50.0;
        ap->hi[COL_HI] =  10000.0;
        ap->default_val[COL_HI] = 0.0;
        ap->lo[COLRATE] = -50.0;
        ap->hi[COLRATE] =  50.0;
        ap->default_val[COLRATE] = 0.0;
        break;
    case(F_FRQSHIFT):
        ap->lo[COLFLT] = -1000.0;
        ap->hi[COLFLT] =  1000.0;
        ap->default_val[COLFLT] = 100.0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        ap->lo[COL_LO] =  0.0;
        ap->hi[COL_LO] =  10000.0;
        ap->default_val[COL_LO] = 0.0;
        ap->lo[COL_HI] =  50.0;
        ap->hi[COL_HI] =  10000.0;
        ap->default_val[COL_HI] = 0.0;
        ap->lo[COLRATE] = -50.0;
        ap->hi[COLRATE] =  50.0;
        ap->default_val[COLRATE] = 0.0;
        break;
    case(F_RESPACE):
        ap->lo[COLFLT] = 1.0;
        ap->hi[COLFLT] = 1000.0;
        ap->default_val[COLFLT] = 10.0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        ap->lo[COL_LO] =  0.0;
        ap->hi[COL_LO] =  10000.0;
        ap->default_val[COL_LO] = 0.0;
        ap->lo[COL_HI] =  50.0;
        ap->hi[COL_HI] =  10000.0;
        ap->default_val[COL_HI] = 0.0;
        ap->lo[COLRATE] = -50.0;
        ap->hi[COLRATE] =  50.0;
        ap->default_val[COLRATE] = .0;
        break;
    case(F_PINVERT):
        ap->lo[COLFLT] = 0.0;
        ap->hi[COLFLT] = 127;
        ap->default_val[COLFLT] = 60;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        ap->lo[COL_LO] =  0.0;
        ap->hi[COL_LO] =  10000.0;
        ap->default_val[COL_LO] = 0.0;
        ap->lo[COL_HI] =  50.0;
        ap->hi[COL_HI] =  10000.0;
        ap->default_val[COL_HI] = 0.0;
        ap->lo[COLRATE] = -50.0;
        ap->hi[COLRATE] =  50.0;
        ap->default_val[COLRATE] = 0.0;
        ap->lo[COLLOPCH] = SPEC_MIDIMIN;
        ap->hi[COLLOPCH] = MIDIMAX;
        ap->default_val[COLLOPCH] = SPEC_MIDIMIN;
        ap->lo[COLHIPCH] = SPEC_MIDIMIN;
        ap->hi[COLHIPCH] =  MIDIMAX;
        ap->default_val[COLHIPCH] = MIDIMAX;
        break;
    case(F_PEXAGG):
        ap->lo[COLFLT] = 0.0;
        ap->hi[COLFLT] = MIDIMAX;
        ap->default_val[COLFLT] = 60;
        ap->lo[EXAGRANG] = 0.0;
        ap->hi[EXAGRANG] = PEX_MAX_RANG;
        ap->default_val[EXAGRANG] = 1.0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        ap->lo[COL_LO] =  0.0;
        ap->hi[COL_LO] =  10000.0;
        ap->default_val[COL_LO] = 0.0;
        ap->lo[COL_HI] =  50.0;
        ap->hi[COL_HI] =  10000.0;
        ap->default_val[COL_HI] = 0.0;
        ap->lo[COLRATE] = -50.0;
        ap->hi[COLRATE] =  50.0;
        ap->default_val[COLRATE] = 0.0;
        ap->lo[COLLOPCH] = SPEC_MIDIMIN;
        ap->hi[COLLOPCH] = MIDIMAX;
        ap->default_val[COLLOPCH] = SPEC_MIDIMIN;
        ap->lo[COLHIPCH] = SPEC_MIDIMIN;
        ap->hi[COLHIPCH] =  MIDIMAX;
        ap->default_val[COLHIPCH] = MIDIMAX;
        break;
    case(F_PQUANT):
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        ap->lo[COL_LO] =  0.0;
        ap->hi[COL_LO] =  10000.0;
        ap->default_val[COL_LO] = 0.0;
        ap->lo[COL_HI] =  50.0;
        ap->hi[COL_HI] =  10000.0;
        ap->default_val[COL_HI] = 0.0;
        ap->lo[COLRATE] = -50.0;
        ap->hi[COLRATE] =  50.0;
        ap->default_val[COLRATE] = 0.0;
        ap->lo[COLLOPCH] = SPEC_MIDIMIN;
        ap->hi[COLLOPCH] = MIDIMAX;
        ap->default_val[COLLOPCH] = SPEC_MIDIMIN;
        ap->lo[COLHIPCH] = SPEC_MIDIMIN;
        ap->hi[COLHIPCH] =  MIDIMAX;
        ap->default_val[COLHIPCH] = MIDIMAX;
        break;
    case(F_PCHRAND):
        ap->lo[FPRMAXINT] = 0.0;
        ap->hi[FPRMAXINT] = RANDPITCHMAX;
        ap->default_val[FPRMAXINT] = 2.0;
        ap->lo[FSLEW] = 0.1;
        ap->hi[FSLEW] = 10.0;
        ap->default_val[FSLEW] = 1.0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        ap->lo[COL_LO] =  0.0;
        ap->hi[COL_LO] =  10000.0;
        ap->default_val[COL_LO] = 0.0;
        ap->lo[COL_HI] =  50.0;
        ap->hi[COL_HI] =  10000.0;
        ap->default_val[COL_HI] = 0.0;
        ap->lo[COLRATE] = -50.0;
        ap->hi[COLRATE] =  50.0;
        ap->default_val[COLRATE] = 0.0;
        ap->lo[COLLOPCH] = SPEC_MIDIMIN;
        ap->hi[COLLOPCH] = MIDIMAX;
        ap->default_val[COLLOPCH] = SPEC_MIDIMIN;
        ap->lo[COLHIPCH] = SPEC_MIDIMIN;
        ap->hi[COLHIPCH] =  MIDIMAX;
        ap->default_val[COLHIPCH] = MIDIMAX;
        break;
    case(F_RAND):
        ap->lo[COLFLT] = 0.0;
        ap->hi[COLFLT] = 1.0;
        ap->default_val[COLFLT] = 0.1;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN]  = 10.0;
        ap->default_val[FGAIN] = 1.0;
        ap->lo[COL_LO] =  0.0;
        ap->hi[COL_LO] =  10000.0;
        ap->default_val[COL_LO] = 0.0;
        ap->lo[COL_HI] =  50.0;
        ap->hi[COL_HI] =  10000.0;
        ap->default_val[COL_HI] = 0.0;
        ap->lo[COLRATE] = -50.0;
        ap->hi[COLRATE] =  50.0;
        ap->default_val[COLRATE] = 0.0;
        break;
    case(F_SINUS):
        ap->lo[F_SINING] = 0.0;
        ap->hi[F_SINING] = 1.0;
        ap->default_val[F_SINING] = 1.0;
        ap->lo[FGAIN] = 0.01;
        ap->hi[FGAIN] = 10.0;
        ap->default_val[FGAIN] = 1.0;
        ap->lo[F_AMP1] = 0.0;
        ap->hi[F_AMP1] = 10.0;
        ap->default_val[F_AMP1] = 1.0;
        ap->lo[F_AMP2] = 0.0;
        ap->hi[F_AMP2] = 10.0;
        ap->default_val[F_AMP2] = 1.0;
        ap->lo[F_AMP3] = 0.0;
        ap->hi[F_AMP3] = 10.0;
        ap->default_val[F_AMP3] = 1.0;
        ap->lo[F_AMP4] = 0.0;
        ap->hi[F_AMP4] = 10.0;
        ap->default_val[F_AMP4] = 1.0;
        ap->lo[F_QDEP1] = 0.0;
        ap->hi[F_QDEP1] = 1.0;
        ap->default_val[F_QDEP1] = 0.0;
        ap->lo[F_QDEP2] = 0.0;
        ap->hi[F_QDEP2] = 1.0;
        ap->default_val[F_QDEP2] = 0.0;
        ap->lo[F_QDEP3] = 0.0;
        ap->hi[F_QDEP3] = 1.0;
        ap->default_val[F_QDEP3] = 0.0;
        ap->lo[F_QDEP4] = 0.0;
        ap->hi[F_QDEP4] = 1.0;
        ap->default_val[F_QDEP4] = 0.0;
        break;
    }
    dz->maxmode = 23;
    if(!sloom)
        put_default_vals_in_all_params(dz);
    return(FINISHED);
}

/********************************* PARSE_SLOOM_DATA *********************************/

int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz)
{
    int exit_status;
    int cnt = 1, infilecnt;
    int filesize, insams, inbrksize;
    double dummy;
    int true_cnt = 0;
    aplptr ap;

    while(cnt<=PRE_CMDLINE_DATACNT) {
        if(cnt > argc) {
            sprintf(errstr,"Insufficient data sent from TK\n");
            return(DATA_ERROR);
        }
        switch(cnt) {
        case(1):    
            if(sscanf(argv[cnt],"%d",&dz->process)!=1) {
                sprintf(errstr,"Cannot read process no. sent from TK\n");
                return(DATA_ERROR);
            }
            break;

        case(2):    
            if(sscanf(argv[cnt],"%d",&dz->mode)!=1) {
                sprintf(errstr,"Cannot read mode no. sent from TK\n");
                return(DATA_ERROR);
            }
            if(dz->mode > 0)
                dz->mode--;
            //setup_particular_application() =
            if((exit_status = setup_the_application(dz))<0)
                return(exit_status);
            ap = dz->application;
            break;

        case(3):    
            if(sscanf(argv[cnt],"%d",&infilecnt)!=1) {
                sprintf(errstr,"Cannot read infilecnt sent from TK\n");
                return(DATA_ERROR);
            }
            if(infilecnt < 1) {
                true_cnt = cnt + 1;
                cnt = PRE_CMDLINE_DATACNT;  /* force exit from loop after assign_file_data_storage */
            }
            if((exit_status = assign_file_data_storage(infilecnt,dz))<0)
                return(exit_status);
            break;
        case(INPUT_FILETYPE+4): 
            if(sscanf(argv[cnt],"%d",&dz->infile->filetype)!=1) {
                sprintf(errstr,"Cannot read filetype sent from TK (%s)\n",argv[cnt]);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_FILESIZE+4): 
            if(sscanf(argv[cnt],"%d",&filesize)!=1) {
                sprintf(errstr,"Cannot read infilesize sent from TK\n");
                return(DATA_ERROR);
            }
            dz->insams[0] = filesize;   
            break;
        case(INPUT_INSAMS+4):   
            if(sscanf(argv[cnt],"%d",&insams)!=1) {
                sprintf(errstr,"Cannot read insams sent from TK\n");
                return(DATA_ERROR);
            }
            dz->insams[0] = insams; 
            break;
        case(INPUT_SRATE+4):    
            if(sscanf(argv[cnt],"%d",&dz->infile->srate)!=1) {
                sprintf(errstr,"Cannot read srate sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_CHANNELS+4): 
            if(sscanf(argv[cnt],"%d",&dz->infile->channels)!=1) {
                sprintf(errstr,"Cannot read channels sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_STYPE+4):    
            if(sscanf(argv[cnt],"%d",&dz->infile->stype)!=1) {
                sprintf(errstr,"Cannot read stype sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_ORIGSTYPE+4):    
            if(sscanf(argv[cnt],"%d",&dz->infile->origstype)!=1) {
                sprintf(errstr,"Cannot read origstype sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_ORIGRATE+4): 
            if(sscanf(argv[cnt],"%d",&dz->infile->origrate)!=1) {
                sprintf(errstr,"Cannot read origrate sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_MLEN+4): 
            if(sscanf(argv[cnt],"%d",&dz->infile->Mlen)!=1) {
                sprintf(errstr,"Cannot read Mlen sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_DFAC+4): 
            if(sscanf(argv[cnt],"%d",&dz->infile->Dfac)!=1) {
                sprintf(errstr,"Cannot read Dfac sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_ORIGCHANS+4):    
            if(sscanf(argv[cnt],"%d",&dz->infile->origchans)!=1) {
                sprintf(errstr,"Cannot read origchans sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_SPECENVCNT+4):   
            if(sscanf(argv[cnt],"%d",&dz->infile->specenvcnt)!=1) {
                sprintf(errstr,"Cannot read specenvcnt sent from TK\n");
                return(DATA_ERROR);
            }
            dz->specenvcnt = dz->infile->specenvcnt;
            break;
        case(INPUT_WANTED+4):   
            if(sscanf(argv[cnt],"%d",&dz->wanted)!=1) {
                sprintf(errstr,"Cannot read wanted sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_WLENGTH+4):  
            if(sscanf(argv[cnt],"%d",&dz->wlength)!=1) {
                sprintf(errstr,"Cannot read wlength sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_OUT_CHANS+4):    
            if(sscanf(argv[cnt],"%d",&dz->out_chans)!=1) {
                sprintf(errstr,"Cannot read out_chans sent from TK\n");
                return(DATA_ERROR);
            }
            break;
            /* RWD these chanegs to samps - tk will have to deal with that! */
        case(INPUT_DESCRIPTOR_BYTES+4): 
            if(sscanf(argv[cnt],"%d",&dz->descriptor_samps)!=1) {
                sprintf(errstr,"Cannot read descriptor_samps sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_IS_TRANSPOS+4):  
            if(sscanf(argv[cnt],"%d",&dz->is_transpos)!=1) {
                sprintf(errstr,"Cannot read is_transpos sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_COULD_BE_TRANSPOS+4):    
            if(sscanf(argv[cnt],"%d",&dz->could_be_transpos)!=1) {
                sprintf(errstr,"Cannot read could_be_transpos sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_COULD_BE_PITCH+4):   
            if(sscanf(argv[cnt],"%d",&dz->could_be_pitch)!=1) {
                sprintf(errstr,"Cannot read could_be_pitch sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_DIFFERENT_SRATES+4): 
            if(sscanf(argv[cnt],"%d",&dz->different_srates)!=1) {
                sprintf(errstr,"Cannot read different_srates sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_DUPLICATE_SNDS+4):   
            if(sscanf(argv[cnt],"%d",&dz->duplicate_snds)!=1) {
                sprintf(errstr,"Cannot read duplicate_snds sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_BRKSIZE+4):  
            if(sscanf(argv[cnt],"%d",&inbrksize)!=1) {
                sprintf(errstr,"Cannot read brksize sent from TK\n");
                return(DATA_ERROR);
            }
            if(inbrksize > 0) {
                switch(dz->input_data_type) {
                case(WORDLIST_ONLY):
                    break;
                case(PITCH_AND_PITCH):
                case(PITCH_AND_TRANSPOS):
                case(TRANSPOS_AND_TRANSPOS):
                    dz->tempsize = inbrksize;
                    break;
                case(BRKFILES_ONLY):
                case(UNRANGED_BRKFILE_ONLY):
                case(DB_BRKFILES_ONLY):
                case(ALL_FILES):
                case(ANY_NUMBER_OF_ANY_FILES):
                    if(dz->extrabrkno < 0) {
                        sprintf(errstr,"Storage location number for brktable not established by CDP.\n");
                        return(DATA_ERROR);
                    }
                    if(dz->brksize == NULL) {
                        sprintf(errstr,"CDP has not established storage space for input brktable.\n");
                        return(PROGRAM_ERROR);
                    }
                    dz->brksize[dz->extrabrkno] = inbrksize;
                    break;
                default:
                    sprintf(errstr,"TK sent brktablesize > 0 for input_data_type [%d] not using brktables.\n",
                    dz->input_data_type);
                    return(PROGRAM_ERROR);
                }
                break;
            }
            break;
        case(INPUT_NUMSIZE+4):  
            if(sscanf(argv[cnt],"%d",&dz->numsize)!=1) {
                sprintf(errstr,"Cannot read numsize sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_LINECNT+4):  
            if(sscanf(argv[cnt],"%d",&dz->linecnt)!=1) {
                sprintf(errstr,"Cannot read linecnt sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_ALL_WORDS+4):    
            if(sscanf(argv[cnt],"%d",&dz->all_words)!=1) {
                sprintf(errstr,"Cannot read all_words sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_ARATE+4):    
            if(sscanf(argv[cnt],"%f",&dz->infile->arate)!=1) {
                sprintf(errstr,"Cannot read arate sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_FRAMETIME+4):    
            if(sscanf(argv[cnt],"%lf",&dummy)!=1) {
                sprintf(errstr,"Cannot read frametime sent from TK\n");
                return(DATA_ERROR);
            }
            dz->frametime = (float)dummy;
            break;
        case(INPUT_WINDOW_SIZE+4):  
            if(sscanf(argv[cnt],"%f",&dz->infile->window_size)!=1) {
                sprintf(errstr,"Cannot read window_size sent from TK\n");
                    return(DATA_ERROR);
            }
            break;
        case(INPUT_NYQUIST+4):  
            if(sscanf(argv[cnt],"%lf",&dz->nyquist)!=1) {
                sprintf(errstr,"Cannot read nyquist sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_DURATION+4): 
            if(sscanf(argv[cnt],"%lf",&dz->duration)!=1) {
                sprintf(errstr,"Cannot read duration sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_MINBRK+4):   
            if(sscanf(argv[cnt],"%lf",&dz->minbrk)!=1) {
                sprintf(errstr,"Cannot read minbrk sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_MAXBRK+4):   
            if(sscanf(argv[cnt],"%lf",&dz->maxbrk)!=1) {
                sprintf(errstr,"Cannot read maxbrk sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_MINNUM+4):   
            if(sscanf(argv[cnt],"%lf",&dz->minnum)!=1) {
                sprintf(errstr,"Cannot read minnum sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        case(INPUT_MAXNUM+4):   
            if(sscanf(argv[cnt],"%lf",&dz->maxnum)!=1) {
                sprintf(errstr,"Cannot read maxnum sent from TK\n");
                return(DATA_ERROR);
            }
            break;
        default:
            sprintf(errstr,"case switch item missing: parse_sloom_data()\n");
            return(PROGRAM_ERROR);
        }
        cnt++;
    }
    if(cnt!=PRE_CMDLINE_DATACNT+1) {
        sprintf(errstr,"Insufficient pre-cmdline params sent from TK\n");
        return(DATA_ERROR);
    }

    if(true_cnt)
        cnt = true_cnt;
    *cmdlinecnt = 0;        

    while(cnt < argc) {
        if((exit_status = get_tk_cmdline_word(cmdlinecnt,cmdline,argv[cnt]))<0)
            return(exit_status);
        cnt++;
    }
    return(FINISHED);
}

/********************************* GET_TK_CMDLINE_WORD *********************************/

int get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q)
{
    if(*cmdlinecnt==0) {
        if((*cmdline = (char **)malloc(sizeof(char *)))==NULL)  {
            sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline array.\n");
            return(MEMORY_ERROR);
        }
    } else {
        if((*cmdline = (char **)realloc(*cmdline,((*cmdlinecnt)+1) * sizeof(char *)))==NULL)    {
            sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline array.\n");
            return(MEMORY_ERROR);
        }
    }
    if(((*cmdline)[*cmdlinecnt] = (char *)malloc((strlen(q) + 1) * sizeof(char)))==NULL)    {
        sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline item %d.\n",(*cmdlinecnt)+1);
        return(MEMORY_ERROR);
    }
    strcpy((*cmdline)[*cmdlinecnt],q);
    (*cmdlinecnt)++;
    return(FINISHED);
}


/****************************** ASSIGN_FILE_DATA_STORAGE *********************************/

int assign_file_data_storage(int infilecnt,dataptr dz)
{
    int exit_status;
    int no_sndfile_system_files = FALSE;
    dz->infilecnt = infilecnt;
    if((exit_status = allocate_filespace(dz))<0)
        return(exit_status);
    if(no_sndfile_system_files)
        dz->infilecnt = 0;
    return(FINISHED);
}

/************************* redundant functions: to ensure libs compile OK *******************/

int assign_process_logic(dataptr dz)
{
    return(FINISHED);
}

void set_legal_infile_structure(dataptr dz)
{}

int set_legal_internalparam_structure(int process,int mode,aplptr ap)
{
    return(FINISHED);
}

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
    return(FINISHED);
}

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
    return(FINISHED);
}

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    return(FINISHED);
}

int read_special_data(char *str,dataptr dz) 
{
    return(FINISHED);
}

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
    return(FINISHED);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if      (!strcmp(prog_identifier_from_cmdline,"specfnu"))       dz->process = SPECFNU;
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        return(USAGE_ONLY);
    }
    return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
    usage2("specfnu");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"specfnu")) {
        fprintf(stdout,"\n"
        "USAGE: specfnu specfnu 1-23 inanalfile outfile [params]\n"
        "\n"
        "Modify spectral shape in relation to formant peaks, or show formant data.\n"
        "\n"
        "MODE 1:  NARROW FORMANTS:     Steepen skirts of formant peaks by power factor.\n"
        "MODE 2:  SQUEEZE SPECTRUM:    Squeeze the spectrum around specified formant.\n"
        "MODE 3:  INVERT FORMANTS:     Formant peaks become troughs, and troughs peaks.\n"
        "MODE 4:  ROTATE FORMANTS:     Formant peaks & frqs move up (or down) spectrum\n"
        "                              reappearing at foot(top) on reaching fmnts edge.\n"
        "MODE 5:  SPECTRAL NEGATIVE:   Spectral values inverted for each channel.\n"
        "MODE 6:  SUPPRESS FORMANTS:   Suppresses the selected formant(s).\n"
        "MODE 7:  GENERATE FILTER:     Output Varibank filtdata based on formant peaks.\n"
        "MODE 8:  MOVE FORMANTS BY:    Displace individual formants by a Hz value.\n"
        "MODE 9:  MOVE FORMANTS TO:    Displace individual formants to specified frqs.\n"
        "MODE 10: ARPEGGIATE:          Arppegiate partials of sound, under formants.\n"
        "MODE 11: OCTAVE-SHIFT:        Octave-shift pitch of sound, under formants.\n"
        "MODE 12: TRANSPOSE:           Transpose pitch of sound, under formants.\n"
        "MODE 13: FREQ-SHIFT:          Frequency shift partials of src, under formants.\n"
        "MODE 14: RESPACE PARTIALS:    Respace partials of sound, under formants.\n"
        "MODE 15: PITCH-INVERT:        Invert pitch of sound, under formants.\n"
        "MODE 16: PITCH-EXAGG/SMOOTH:  Exaggerate/Smooth pitchline, under formants.\n"
        "MODE 17: PITCH-QUANTISE:      Force pitch onto pitch field, under formants.\n"
        "MODE 18: PITCH-RANDOMISE:     Randomise pitch of src, under formants.\n"
        "MODE 19: RANDOMISE PARTIALS:  Random shift partials of sound, under formants.\n"
        "MODE 20: SEE SPEC ENVELOPES:  Outputs viewable (not playable) sndfile.\n"
        "MODE 21: SEE SPEC PEAKS/TROFS:Print textfile of frqs of peaks+trofs per window.\n"
        "MODE 22: GET LOUDNESS TROFS:  Print textfile of times-of-trofs between syllabs.\n"
        "MODE 23: SINE SPEECH:         Single sine wave repesenting each formant.\n"
        "\n"
        "Type \"specfnu specfnu 1\" for more info on NARROW FORMANTS.... etc.\n"
        "\n"
        "Spectral window-size: Small window or Large window ???\n"
        "\n"
        "If process uses \"Force fundamental\" flag, Large window will find fundamental\n"
        "whereas Small window may not.\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
    if(!strcmp(str2,"1")) {
        fprintf(stdout,"\n"
        "MODE 1: NARROW FORMANTS: Steepens skirts of formant peaks by power factor.\n"
        "\n"
        "USAGE: specfnu specfnu 1 inanalfil outanalfil narrow\n"
        "       [-ggain] [-ooff] [-t] [-f] [-s] [-x|-k] [-r]\n"
        "\n"
        "NARROW  Narrowing of individual formant peaks. Range 1 to 1000. Timevariable.\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "OFF     Suppress the listed formants. \"OFF\" can be any combination of\n"
        "        \"1\",\"2\",\"3\" & \"4\" but not all of them, and with no repetitions.\n"
        "-t      Zero top of spectrum (above fourth formant).\n"
        "-f      Force lowest formant to use fundamental frq as peak.\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-k      Kill harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n");
    } else if(!strcmp(str2,"2")) {
        fprintf(stdout,"\n"
        "MODE 2: SQUEEZE SPECTRUM AROUND FORMANT: Squeeze around specified formant.\n"
        "\n"
        "USAGE: specfnu specfnu 2 inanal outanal squeeze centre\n"
        "       [-ggain] [-t] [-f] [-s] [-x|-k] [-r]\n"
        "\n"
        "SQUEEZE Squeeze factor. Range 1 to 10. Timevariable.\n"
        "CENTRE  Formant peak at centre of squeeze. (range 1 to 4)\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "-t      Squeeze around trough above specified peak.\n"
        "-f      Force lowest formant to use fundamental frq as peak.\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-k      Kill harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n");
    } else if(!strcmp(str2,"3")) {
        fprintf(stdout,"\n"
        "MODE 3: INVERT FORMANTS: Formant peaks become troughs, and troughs peaks.\n"
        "\n"
        "USAGE: specfnu specfnu 3 inanal outanal vibrate [-ggain] [-s] [-x|-k] [-r]\n"
        "\n"
        "VIBRATE If not zero, cycle btwn orig & inverted at this frq. Timevariable.\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-k      Kill harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n");
    } else if(!strcmp(str2,"4")) {
        fprintf(stdout,"\n"
        "MODE 4: ROTATE FORMANTS: Formant peaks & frqs move up (or down) spectrum.\n"
        "        reappearing at foot (top) of formant-area, when they reach its edge.\n"
        "\n"
        "USAGE: specfnu specfnu 4 inanal outanal rspeed [-ggain] [-s] [-x|-k] [-r]\n"
        "\n"
        "RSPEED  How quickly the spectrum rotates, e.g. 2 = twice every second.\n"
        "        Range -300 to 300 rotations per second. Timevariable.\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-k      Kill harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n");
    } else if(!strcmp(str2,"5")) {
        fprintf(stdout,"\n"
        "MODE 5: SPECTRAL NEGATIVE: Spectral values inverted for each channel.\n"
        "\n"
        "USAGE: specfnu specfnu 5 inanal outanal [-ggain] [-f]\n"
        "\n"
        "GAIN   Output gain or attenuation. (Range 0.01 to 10)\n"
        "-f      \"Flat\" : does not re-envelope the output spectrum.\n");
    } else if(!strcmp(str2,"6")) {
        fprintf(stdout,"\n"
        "MODE 6: SUPPRESS FORMANTS: Suppresses the selected formants.\n"
        "\n"
        "USAGE: specfnu specfnu 6 inanalfile outanalfile formantlist [-ggain] [-s] [-x]\n"
        "\n"
        "FORMANTLIST which of (4) formants to suppress. e.g. \"1\" means suppress 1st,\n"
        "            while \"134\" means suppress 1st, 3rd and 4th.\n"
        "GAIN    Amplitude gain attenuation. (Range 0.1 to 10).\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n");
    } else if(!strcmp(str2,"7")) {
        fprintf(stdout,"\n"
        "MODE 7: GENERATE FILTER(S) FROM FORMANT(S): Outputs varibank filter data textfile.\n"
        "\n"
        "USAGE: specfnu specfnu 7 inanalfile outfiltfile datafile filtcnt\n"
        "       [-bbelow] [-k|-i] [-f] [-s]\n"
        "\n"
        "DATAFILE Textfile containing.\n"
        "    (1) List of times at which src to be divided into blocks for analysis.\n"
        "        Resulting filter step-changes between vals created from each block.\n"
        "        Value ZERO means use the entire source to make 1 fixed filter.\n"
        "    (2) Data about grid on which pitches you are searching for must lie.\n"
        "        There must first be a marker indicating the grid type. These are..\n"
        "        #HS:    Followed by listed MIDI pitches to search for.\n"
        "        #HF:    Followed by listed MIDI pitches to search for IN ALL OCTAVES.\n"
        "        #SCALE: Followed by just TWO values.\n"
        "              (a) the number of equal pitch divisions in an octave.\n"
        "              (b) MIDI pitch of any pitch to tune the scales to.\n"
        "        #ELACS: Followed by just THREE values.\n"
        "              (a) Size of \"octave\" in (possibly fractional) semitones.\n"
        "              (b) the number of equal pitch divisions in \"octave\".\n"
        "              (c) MIDI pitch of any pitch to tune the scales to.\n"
        "FILTCNT (Default value 1). (Max) No. peaks from each formant to use.\n"
        "BELOW   Try to ensure 1 (or more) pitches below MIDI value \"below\".\n"
        "-k      Keep relative (summed) loudnesses of peaks as part of filter design.\n"
        "-i      Keep inverse of loudnesses as part of filter design.\n"
        "        Default. All filter amplitudes set to 1.0\n"
        "-f      Force fundamental as pitch in lowest formant.\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "\n"
        "With time-varying filter, transitions between time blocks can be extreme.\n"
        "Wise to cut input into segments and produce fixed filters for each,\n"
        "OR pre-envelope THE input file, to zero the level around transitions\n"
        "deriving appropriate envelope times from the filter file times.\n");
    } else if(!strcmp(str2,"8")) {
        fprintf(stdout,"\n"
        "MODE 8: MOVE FORMANTS BY: Displace individual formants.\n"
        "\n"
        "USAGE: specfnu specfnu 8 inanal outanal mov1 mov2 mov3 mov4\n"
        "       [-ggain] [-t] [-s] [-x|-k] [-r]\n"
        "\n"
        "MOV1     Frq displacement, up or down, of formant 1. Timevariable.\n"
        "MOV2,3,4 Similarly for other formants.\n"
        "(Formants moving below zero or above nyquist/2 will disappear).\n"
        "GAIN     Output gain or attenuation. (Range 0.01 to 10)\n"
        "-t       Zero top of spectrum.\n"
        "-s       Use short-window for extracting spectral envelope.\n"
        "-x       Exclude non-harmonic partials.\n"
        "-k       Kill harmonic partials.\n"
        "-r       Replace unpitched (or extremely quiet) windows by silence.\n");
    } else if(!strcmp(str2,"9")) {
        fprintf(stdout,"\n"
        "MODE 9: MOVE FORMANTS TO: Displace individual formants.\n"
        "\n"
        "USAGE: specfnu specfnu 9 inanal outanal frq1 frq2 frq2 frq4\n"
        "       [-ggain] [-t] [-s] [-n] [-x|-k] [-r]\n"
        "\n"
        "FRQ1     New frq of formant 1. Timevariable.\n"
        "FRQ2,3,4 Similarly for other formants.\n"
        "GAIN     Output gain or attenuation. (Range 0.01 to 10)\n"
        "-t       Zero top of spectrum.\n"
        "-n       Use narrow formant bands.\n"
        "         Using narrow bands can give counterintuitive results.\n"
        "         Doesn't capture so many harmonic peaks, so sounds duller.\n"
        "-s       Use short-window for extracting spectral envelope.\n"
        "-x       Exclude non-harmonic partials.\n"
        "-k       Kill harmonic partials.\n"
        "-r       Replace unpitched (or extremely quiet) windows by silence.\n");
    } else if(!strcmp(str2,"10")) {
        fprintf(stdout,"\n"
        "MODE 10: ARPEGGIATE SPECTRUM: Arpeggiate spectrum of source, under formants.\n"
        "\n"
        "USAGE: specfnu specfnu 10 inanal outanal arprate\n"
        "       [-ggain] [-s] [-x] [-r] [-d|-c]\n"
        "\n"
        "ARPRATE Rate of arpeggiation of spectrum (Range -50 to +50 Hz).\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n"
        "-d      Arpeggiate downwards (default upwards).\n"
        "-c      Arpeggiate up and down (default upwards).\n");
    } else if(!strcmp(str2,"11")) {
        fprintf(stdout,"\n"
        "MODE 11: OCTAVE-SHIFT UNDER FORMANTS: Octave-shift src spectrum, under formants.\n"
        "\n"
        "USAGE: specfnu specfnu 11 inanal outanal octshift \n" 
        "       [-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c] [-f]\n"
        "\n"
        "OCTSHIFT 8va shift (Range -4 to + 4). (Timevariable).\n"
        "\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "ARPRATE Rate of arpeggiation of spectrum (Range -50 to +50 Hz).\n"
        "LOCUT   Cut off frequencies below this. (range 0 to 10000)\n"
        "HICUT   Cut off frequencies above this. (range 50 to 10000)\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n"
        "-d      Arpeggiate downwards (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-c      Arpeggiate up and down (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-f      If transposing downwards, fill spectrum top with extra harmonics.\n");
    } else if(!strcmp(str2,"12")) {
        fprintf(stdout,"\n"
        "MODE 12: TRANSPOSE UNDER FORMANTS: tranpose spectrum of src, under formants.\n"
        "\n"
        "USAGE: specfnu specfnu 12 inanal outanal transpos \n"
        "       [-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c] [-f]\n"
        "\n"
        "TRANSPOS semitone shift (Range -48 to + 48). (Timevariable).\n"
        "\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "ARPRATE Rate of arpeggiation of spectrum (Range -50 to +50 Hz).\n"
        "LOCUT   Cut off frequencies below this. (range 0 to 10000)\n"
        "HICUT   Cut off frequencies above this. (range 50 to 10000)\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n"
        "-d      Arpeggiate downwards (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-c      Arpeggiate up and down (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-f      If transposing downwards, fill spectrum top with extra harmonics.\n");
    } else if(!strcmp(str2,"13")) {
        fprintf(stdout,"\n"
        "MODE 13: FRQSHIFT UNDER FORMANTS: Frqshift src spectrum of under formants.\n"
        "\n"
        "USAGE: specfnu specfnu 13 inanal outanal frqshift\n"
        "       [-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c] [-f]\n"
        "\n"
        "FRQSHIFT Frequency shift(Hz) (Range -1000 to +1000). (Timevariable).\n"
        "\n"
        "GAIN     Output gain or attenuation. (Range 0.01 to 10)\n"
        "ARPRATE  Rate of arpeggiation of spectrum (Range -50 to +50 Hz).\n"
        "LOCUT    Cut off frequencies below this. (range 0 to 10000)\n"
        "HICUT    Cut off frequencies above this. (range 50 to 10000)\n"
        "-s       Use short-window for extracting spectral envelope.\n"
        "-x       Exclude non-harmonic partials.\n"
        "-r       Replace unpitched (or extremely quiet) windows by silence.\n"
        "-d       Arpeggiate downwards (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-c       Arpeggiate up and down (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-f       If shifting down, fill spectrum top with extra shifted partials.\n");
    } else if(!strcmp(str2,"14")) {
        fprintf(stdout,"\n"
        "MODE 14: RESPACE PARTIALS UNDER FORMANTS: Respace partials in src spectrum,\n"
        "                                          retaining formants.\n"
        "\n"
        "USAGE: specfnu specfnu 14 inanal outanal respace\n"
        "       [-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c] [-f]\n"
        "\n"
        "RESPACE  New frq spacing of partials (Hz) (Range 1 to 1000).\n"
        "                                             (Timevariable).\n"
        "\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "ARPRATE Rate of arpeggiation of spectrum (Range -50 to +50 Hz).\n"
        "LOCUT   Cut off frequencies below this. (range 0 to 10000)\n"
        "HICUT   Cut off frequencies above this. (range 50 to 10000)\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n"
        "-d      Arpeggiate downwards (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-c      Arpeggiate up and down (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-f      If narrowed spacing, fill spectrum top with extra shifted partials.\n");
    } else if(!strcmp(str2,"15")) {
        fprintf(stdout,"\n"
        "MODE 15: PITCH-INVERT UNDER FORMANTS: Invert pitch of src, under formants.\n"
        "\n"
        "USAGE: specfnu specfnu 15 inanal outanal map about [-ggain] [-parprate]\n"
        "       [-llocut] [-hhicut] [-blopch] [-thipch] [-s] [-x] [-r] [-d|-c]\n"
        "\n"
        "MAP       Set map to ZERO if no mapping is required..OTHERWISE\n"
        "          map is a textfile of paired values showing how intervals\n"
        "          (in, possibly fractional, SEMITONES)\n"
        "          are to be mapped onto their inversions.\n"
        "          Range +-%0lf semitones (+- %.0lf octaves).\n"
        "\n"
        "ABOUT   Pitch about which to invert pitch of src (Range %d to %d).\n"
        "        Pitch as MIDI, with possibly fractional val.  (Timevariable).\n"
        "        If value zero entered, uses the mean pitch of the input.\n"
        "\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "ARPRATE Rate of arpeggiation of spectrum (Range -50 to +50 Hz).\n"
        "LOCUT   Cut off frequencies below this. (range 0 to 10000)\n"
        "HICUT   Cut off frequencies above this. (range 50 to 10000)\n"
        "LOPCH   Minimum acceptable pitch (MIDI, possibly fractional).\n"
        "HIPCH   Maximum acceptable pitch (MIDI, possibly fractional).\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n"
        "-d      Arpeggiate downwards (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-c      Arpeggiate up and down (dflt upwards). (Only when \"arprate\" > 0).\n",MAXINTRANGE,MAXINTRANGE/SEMITONES_PER_OCTAVE,SPEC_MIDIMIN,MIDIMAX);
    } else if(!strcmp(str2,"16")) {
        fprintf(stdout,"\n"
        "MODE 16: PITCH-EXAGGERATE/SMOOTH UNDER FORMANTS: Exaggerate/Smooth pitch-line,\n"
        "                                                 retaining the formants.\n"
        "\n"
        "USAGE: specfnu specfnu 16 inanal outanal about rang [-ggain] [-parprate]\n"
        "       [-llocut] [-hhicut] [-blopch] [-thipch] [-T] [-F] [-M] [-A] [-B]\n"
        "       [-s] [-x] [-r] [-d|-c]\n"
        "\n"
        "ABOUT   Pitch about which to exagg/smooth pitchline of src (Range %d to %d).\n"
        "        Pitch as MIDI, with possibly fractional val.  (Timevariable).\n"
        "        If value zero entered, uses the mean pitch of the input.\n"
        "RANG    Expand or Contract Range of pitchline. (Range 0 - 1).\n"
        "\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "ARPRATE Rate of arpeggiation of spectrum (Range -50 to +50 Hz).\n"
        "LOCUT   Cut off frequencies below this. (range 0 to 10000)\n"
        "HICUT   Cut off frequencies above this. (range 50 to 10000)\n"
        "LOPCH   Minimum acceptable pitch (MIDI, possibly fractional).\n"
        "HIPCH   Maximum acceptable pitch (MIDI, possibly fractional).\n"
        "-T      Tie to TOP of pitch range.\n"
        "-F      Tie to FOOT of pitch range.\n"
        "-M      (only with \"-T\" and \"M\" flags) also Tie to range middle.\n"
        "-A      Do range change ABOVE mean only.\n"
        "-B      Do range change BELOW mean only.\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n"
        "-d      Arpeggiate downwards (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-c      Arpeggiate up and down (dflt upwards). (Only when \"arprate\" > 0).\n",SPEC_MIDIMIN,MIDIMAX);
    } else if(!strcmp(str2,"17")) {
        fprintf(stdout,"\n"
        "MODE 17: QUANTISE PITCH: Force source onto a specified pitch field.\n"
        "\n"
        "USAGE: specfnu specfnu 17 inanal outanal datafile [-ggain] [-parprate]\n"
        "       [-llocut] [-hhicut] [-blopch] [-thipch] [-s] [-x] [-r] [-d|-c] [-o] [-n]\n"
        "\n"
        "DATAFILE Textfile containing data about pitches to quantise to.\n"
        "        There must first be a marker indicating the grid type. These are..\n"
        "        #HS:    Followed by listed MIDI pitches to quantise to.\n"
        "        #HF:    Followed by listed MIDI pitches to quantise to IN ALL OCTAVES.\n"
        "        #THF:   Followed by lines each with time + list of MIDI pitches.\n"
        "                1st time must be zero & times must increase.\n"
        "                Each MIDI list must be the same length.\n"
        "                To change no of pitches from line to line, duplicate values.\n"
        "        #SCALE: Followed by just TWO values.\n"
        "              (a) the number of equal pitch divisions in an octave.\n"
        "              (b) MIDI pitch of any pitch to tune the scales to.\n"
        "        #ELACS: Followed by just THREE values.\n"
        "              (a) Size of \"pseudo-octave\" in (possibly fractional) semitones.\n"
        "              (b) the number of equal pitch divisions in peudo-octave.\n"
        "              (c) MIDI pitch of any pitch to tune the scales to.\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "ARPRATE Rate of arpeggiation of spectrum (Range -50 to +50 Hz).\n"
        "LOCUT   Cut off frequencies below this. (range 0 to 10000)\n"
        "HICUT   Cut off frequencies above this. (range 50 to 10000)\n"
        "LOPCH   Minimum acceptable pitch (MIDI, possibly fractional).\n"
        "HIPCH   Maximum acceptable pitch (MIDI, possibly fractional).\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n"
        "-d      Arpeggiate downwards (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-c      Arpeggiate up and down (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-o      Allow ORNAMENTS in the quantised pitch-line.\n"
        "-n      No smoothing of transitions between pitches.\n");
    } else if(!strcmp(str2,"18")) {
        fprintf(stdout,"\n"
        "MODE 18: PITCH RANDOMISE: Randomise pitch of src.\n"
        "\n"
        "USAGE: specfnu specfnu 18 inanal outanal datafile range slew [-ggain] [-parprate]\n"
        "       [-llocut] [-hhicut] [-blopch] [-thipch] [-s] [-x] [-r] [-d|-c] [-o] [-n] [-k]]\n"
        "\n"
        "DATAFILE If this set to ZERO, no pre-quantisation of pitch takes place.\n"
        "         Otherwise, this is textfile of data about pitches to quantise to.\n"
        "         There must first be a marker indicating the grid type. These are..\n"
        "         #HS:    Followed by listed MIDI pitches to quantise to.\n"
        "         #HF:    Followed by listed MIDI pitches to quantise to IN ALL OCTAVES.\n"
        "         #THF:   Followed by lines each with time + list of MIDI pitches.\n"
        "                 1st time must be zero & times must increase.\n"
        "                 Each MIDI list must be the same length.\n"
        "                 To change no of pitches from line to line, duplicate values.\n"
        "         #SCALE: Followed by just TWO values.\n"
        "               (a) the number of equal pitch divisions in an octave.\n"
        "               (b) MIDI pitch of any pitch to tune the scales to.\n"
        "         #ELACS: Followed by just THREE values.\n"
        "               (a) Size of \"pseudo-octave\" in (possibly fractional) semitones.\n"
        "               (b) the number of equal pitch divisions in peudo-octave.\n"
        "               (c) MIDI pitch of any pitch to tune the scales to.\n"
        "RANGE   semitone range within which random offsets generated. (Range 0 to %d)\n"
        "SLEW    Relationship between (possible) upward & downward rand variation.\n"
        "        e.g. 2: uprange = 2 * downrange     0.5: uprange = 0.5 * downrange.\n"
        "        Ranges 0.1 to 10. (Value 1 has no effect).\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "ARPRATE Rate of arpeggiation of spectrum (Range -50 to +50 Hz).\n"
        "LOCUT   Cut off frequencies below this. (range 0 to 10000)\n"
        "HICUT   Cut off frequencies above this. (range 50 to 10000)\n"
        "LOPCH   Minimum acceptable pitch (MIDI, possibly fractional).\n"
        "HIPCH   Maximum acceptable pitch (MIDI, possibly fractional).\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n"
        "-d      Arpeggiate downwards (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-c      Arpeggiate up and down (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-o      Open up FAST MOVEMENT in the quantised pitch-line.\n"
        "-n      No smoothing of transitions between pitches.\n"
        "-k      Kill-off formant reshaping.\n",RANDPITCHMAX);
    } else if(!strcmp(str2,"19")) {
        fprintf(stdout,"\n"
        "MODE 19: RANDOMISE SPECTRUM UNDER FORMANTS: Randomise spectrum, under formants.\n"
        "\n"
        "USAGE: specfnu specfnu 19 inanal outanal rand\n"
        "       [-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c]\n"
        "\n"
        "RAND    Randomisation of partial frequencies (Range 0 to 1).\n"
        "                                             (Timevariable).\n"
        "\n"
        "GAIN    Output gain or attenuation. (Range 0.01 to 10)\n"
        "ARPRATE Rate of arpeggiation of spectrum (Range -50 to +50 Hz).\n"
        "LOCUT   Cut off frequencies below this. (range 0 to 10000)\n"
        "HICUT   Cut off frequencies above this. (range 50 to 10000)\n"
        "-s      Use short-window for extracting spectral envelope.\n"
        "-x      Exclude non-harmonic partials.\n"
        "-r      Replace unpitched (or extremely quiet) windows by silence.\n"
        "-d      Arpeggiate downwards (dflt upwards). (Only when \"arprate\" > 0).\n"
        "-c      Arpeggiate up and down (dflt upwards). (Only when \"arprate\" > 0).\n");
    } else if(!strcmp(str2,"20")) {
        fprintf(stdout,"\n"
        "MODE 20: SEE SPECTRAL ENVELOPES: Outputs viewable (not playable) sndfile\n"
        "                                 showing spectral envelope at each window\n"
        "                                 as a block of +ve samples.\n"
        "\n"
        "USAGE: specfnu specfnu 20 inanalfile outpseudosndfile [-s]\n"
        "\n"
        "-s   Use short-window for extracting spectral envelope.\n");
    } else if(!strcmp(str2,"21")) {
        fprintf(stdout,"\n"
        "MODE 21: SEE SPECTRAL PEAKS & TROUGHS: Frqs of troughs/peaks in each window\n"
        "                                       printed to textfile, in the format...\n"
        "\n"
        "   \"Trough  PEAK-1  Trough  PEAK-2  Trough  PEAK-3  Trough  PEAK-4  Trough\"\n"
        "\n"
        "USAGE: specfnu specfnu 21 inanalfile outtextfile [-s]\n"
        "\n"
        "-s   Use short-window for extracting spectral envelope.\n");
    } else if(!strcmp(str2,"22")) {
        fprintf(stdout,"\n"
        "MODE 22: LIST TIMES OF TROUGHS BETWEEN SYLLABLES: Times printed to textfile.\n"
        "\n"
        "Will usually need post-correction \"by hand\".\n"
        "\n"
        "USAGE: specfnu specfnu 22 inanal outtextfile [-ssyldur] [-ppktrof] [-P|-B]\n"
        "\n"
        "SYLDUR  Min acceptable duration for a syllable (default %.04lf)\n"
        "PKTROF  Min height of peak above bracketing trofs (default %.04lf).\n"
         "       (Maximum possible height is 1.0).\n"
         "-P      Get peaks.\n"
         "-B      Get both troughs and peaks,\n",MIN_SYLLAB_DUR,MIN_PEAKTROF_GAP);
    } else if(!strcmp(str2,"23")) {
        fprintf(stdout,"\n"
        "MODE 23: SINE SPEECH: Convert formant frqs to sinus tones.\n"
        "\n"
        "USAGE: specfnu specfnu 23 inanalfile outanalfile hffile sining [-again]\n"
        "[-bamp1] [-camp2] [-damp3] [-eamp4] [-nqdep1] [-oqdep2] [-pdep3] [-qqdep4]\n" 
        "[-s] [-f] [-r] [-S]\n"
        "\n"
        "HFFILE If this set to ZERO, no pitch quantisation takes place.\n"
        "       Otherwise, this is textfile of data about pitches to quantise to.\n"
        "       There must first be a marker indicating the grid type. These are..\n"
        "       #HS:    Followed by listed MIDI pitches to quantise to.\n"
        "       #HF:    Followed by listed MIDI pitches to quantise to IN ALL OCTAVES.\n"
        "       #THF:   Followed by lines each with time + list of MIDI pitches.\n"
        "               1st time must be zero & times must increase.\n"
        "               Each MIDI list must be the same length.\n"
        "               To change no of pitches from line to line, duplicate values.\n"
        "       #SCALE: Followed by just TWO values.\n"
        "             (a) the number of equal pitch divisions in an octave.\n"
        "             (b) MIDI pitch of any pitch to tune the scales to.\n"
        "       #ELACS: Followed by just THREE values.\n"
        "             (a) Size of \"pseudo-octave\" in (possibly fractional) semitones.\n"
        "             (b) the number of equal pitch divisions in peudo-octave.\n"
        "             (c) MIDI pitch of any pitch to tune the scales to.\n"
        "\n"
        "GAIN   Output gain or attenuation. (Range 0.01 to 10)\n"
        "SINING Degree of sinusoidisation. Range 0 to 1.\n"
        "AMP1   Relative gain or attenuation of formant1. (Range 0.01 to 10)\n"
        "       AMP2,AMP3,AMP4 similar for the other formants.\n"
        "DEP1   If HF applied, how strongly to force pitches formant1 to hf (Range 0-1).\n"
        "DEP2, DEP3, DEP4   Similarly for other formant bands.\n"
        "-s     Use short-window for extracting spectral envelope.\n"
        "-f     Force lowest formant to use fundamental frq as peak.\n"
        "-r     Replace unpitched (or extremely quiet) windows by silence.\n"
        "-S     Smoothing between pitches in formant traces.\n");
    } else
        fprintf(stdout,"Unknown mode '%s'\n",str2);
    return(USAGE_ONLY);
}

/******************************** USAGE4 ********************************/

int usage4(int mode)
{
    fprintf(stdout,"\nToo few parameters ");
    switch(mode) {
    case(0):  
        fprintf(stdout,"NARROW FORMANTS\n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 1 inanal outanal narrow\n");
        fprintf(stdout,"[-ggain] [-ooff] [-t] [-f] [-s] [-x|-k] [-r]\n");       
        break;
    case(1):  
        fprintf(stdout,"SQUEEZE SPECTRUM AROUND FORMANT\n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 2 inanal outanal squeeze centre\n");
        fprintf(stdout,"[-ggain] [-t] [-f] [-s] [-x|-k] [-r]\n");       
        break;
    case(2):  
        fprintf(stdout,"INVERT FORMANTS\n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 3 inanal outanal vibrate [-ggain] [-s] [-x|-k] [-r]\n");                     
        break;
    case(3):  
        fprintf(stdout,"ROTATE FORMANTS\n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 4 inanal outanal rspeed [-ggain] [-s] [-x|-k] [-r]\n");                      
        break;
    case(4):  
        fprintf(stdout,"SPECTRAL NEGATIVE\n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 5 inanal outanal [-ggain] [-f]\n");                                          
        break;
    case(5):  
        fprintf(stdout,"SUPPRESS FORMANTS\n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 6 inanal outanal formantlist [-ggain] [-s] [-x]\n");                         
        break;
    case(6):  
        fprintf(stdout,"GENERATE FILTER(S) FROM FORMANT(S)\n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 7 inanal outfiltfil datafile filtcnt\n");
        fprintf(stdout,"[-bbelow] [-k|-i] [-f] [-s]\n");            
        break;
    case(7):  
        fprintf(stdout,"MOVE FORMANTS BY\n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 8 inanal outanal mov1 mov2 mov2 mov4\n");
        fprintf(stdout,"[-ggain] [-t] [-s] [-x|-k] [-r]\n");        
        break;
    case(8):  
        fprintf(stdout,"MOVE FORMANTS TO\n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 9 inanal outanal frq1 frq2 frq2 frq4\n");
        fprintf(stdout,"[-ggain] [-t] [-s] [-n] [-x|-k] [-r]\n");
        break;
    case(9):  
        fprintf(stdout,"ARPEGGIATE SPECTRUM \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 10 inanal outanal arprate\n");
        fprintf(stdout,"[-ggain] [-s] [-x] [-r] [-d|-c]\n");                
        break;
    case(10): 
        fprintf(stdout,"OCTAVE-SHIFT UNDER FORMANTS\n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 11 inanal outanal octshift\n");
        fprintf(stdout,"[-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c] [-f]\n");   
        break;
    case(11): 
        fprintf(stdout,"TRANSPOSE UNDER FORMANTS \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 12 inanal outanal transpos\n");
        fprintf(stdout,"[-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c] [-f]\n");   
        break;
    case(12): 
        fprintf(stdout,"FRQSHIFT UNDER FORMANTS \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 13 inanal outanal frqshift\n");
        fprintf(stdout,"[-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c] [-f]\n");   
        break;
    case(13): 
        fprintf(stdout,"RESPACE PARTIALS UNDER FORMANTS \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 14 inanal outanal respace\n");
        fprintf(stdout,"[-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c] [-f]\n");   
        break;
    case(14): 
        fprintf(stdout,"PITCH INVERT UNDER FORMANTS \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 15 inanal outanal about\n");
        fprintf(stdout,"[-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c]\n");            
        break;
    case(15): 
        fprintf(stdout,"PITCH EXAGGERATE UNDER FORMANTS \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 16 inanal outanal about rang [-ggain] [-parprate]\n");
        fprintf(stdout,"[-llocut] [-hhicut] [-blopch] [-thipch] [-T] [-F] [-M] [-A] [-B]\n");
        fprintf(stdout,"[-s] [-x] [-r] [-d|-c]\n");
        break;
    case(16): 
        fprintf(stdout,"PITCH QUANTISE UNDER FORMANTS \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 17 inanal outanal datafile [-ggain] [-parprate]\n");
        fprintf(stdout,"[-llocut] [-hhicut] [-blopch] [-thipch] [-s] [-x] [-r] [-d|-c]\n");
        break;
    case(17): 
        fprintf(stdout,"RANDOMISE SPECTRUM UNDER FORMANTS \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 18 inanal outanal rand\n");
        fprintf(stdout,"[-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c]\n");            
        break;
    case(18): 
        fprintf(stdout,"RANDOMISE SPECTRUM UNDER FORMANTS \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 19 inanal outanal rand\n");
        fprintf(stdout,"[-ggain] [-parprate] [-llocut] [-hhicut] [-s] [-x] [-r] [-d|-c]\n");            
        break;
    case(19): 
        fprintf(stdout,"SEE SPECTRAL ENVELOPES \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 20 inanalfile outpseudosndfile [-s]\n");             
        break;
    case(20): 
        fprintf(stdout,"SEE SPECTRAL PEAKS & TROUGHS \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 21 inanalfile outtextfile [-s]\n");                  
        break;
    case(21): 
        fprintf(stdout,"LIST TIMES OF TROUGHS BETWEEN SYLLABLES \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 22 inanal outtextfile [-ssyldur] [-ppktrof] [-P|-B]\n"); 
        break;
    case(22): 
        fprintf(stdout,"SINE SPEECH \n\n");
        fprintf(stdout,"USAGE: specfnu specfnu 23 inanalfile outanalfile hffile sining [-again]\n");
        fprintf(stdout,"[-bamp1] [-camp2] [-damp3] [-eamp4] [-nqdep1] [-oqdep2] [-pdep3] [-qqdep4]\n");
        fprintf(stdout,"[-s] [-f] [-r] [-S]\n");
        break;
    }
    fflush(stdout);
    return(USAGE_ONLY);
}

/************************ GET_THE_MODE_NO *********************/

int get_the_mode_no(char *str, dataptr dz)
{
    if(sscanf(str,"%d",&dz->mode)!=1) {
        sprintf(errstr,"Cannot read mode of program.\n");
        return(USAGE_ONLY);
    }
    if(dz->mode <= 0 || dz->mode > dz->maxmode) {
        sprintf(errstr,"Program mode value [%d] is out of range [1 - %d].\n",dz->mode,dz->maxmode);
        return(USAGE_ONLY);
    }
    dz->mode--;     /* CHANGE TO INTERNAL REPRESENTATION OF MODE NO */
    return(FINISHED);
}

/************************ FORCE_EXTENSION *********************/

void force_extension(int type,char *filename)
{
    char *q, *p = filename;
    int doit = 0;
    while(*p != '.') {
        p++;
        if(*p == ENDOFSTR)
            break;
    }
    if(*p == ENDOFSTR) {
        switch(type) {
        case(0):    strcat(filename,".wav");    break;
        case(1):    strcat(filename,".txt");    break;
        }
    } else {
        p++;
        q = p;
        switch(type) {
        case(0):
            if(*q++ != 'w')
                doit = 1;   //RWD  all 4 cases, needed assignment
            else {
                if(*q++ != 'a')
                    doit = 1;
                else {
                    if(*q++ != 'v')
                        doit = 1;
                    else {
                        if(*q != ENDOFSTR)
                            doit = 1;
                    }
                }
            }
            break;
        case(1):
            if(*q++ != 't')
                doit = 1;
            else {
                if(*q++ != 'x')
                    doit = 1;
                else {
                    if(*q++ != 't')
                        doit = 1;
                    else {
                        if(*q != ENDOFSTR)
                            doit = 1;
                    }
                }
            }
            break;
        }
        if(doit) {
            *p = ENDOFSTR;
            switch(type) {
            case(0):    strcat(filename,"wav"); break;
            case(1):    strcat(filename,"txt"); break;
            }
        }
    }
}

/**************************** OUTER_SPECFNU_LOOP ****************************/

int outer_specfnu_loop(int contourcnt,dataptr dz)
{
    int exit_status, n, zerolen, cc, vc, passno = 0, times_index, arp_param = 0, previouspk = -1, up = 1;
    int windows_in_buf, inner_lpcnt;
    double thisfrq, fmax = -HUGE, normaliser = 0.0;
    int limit = dz->wlength * PKBLOK;
    double phase = 0.0, target = 0.0, maxpitch = 0.0, minpitch = 0.0;

    //  Where, in the 3 processing loops, does each mode begin

    switch(dz->mode) {
    case(F_ARPEG):
        arp_param = FARPRATE;
        passno    = FPREANAL;
        break;
    case(F_OCTSHIFT):// fall thro
    case(F_TRANS):   // fall thro
    case(F_FRQSHIFT):// fall thro
    case(F_RESPACE): // fall thro
    case(F_PINVERT): // fall thro
    case(F_RAND):    // fall thro
    case(F_PEXAGG):  // fall thro
    case(F_PQUANT):  // fall thro
    case(F_PCHRAND):
        arp_param = COLRATE;
        passno    = FPREANAL;
        break;
    case(F_SYLABTROF)://fall thro
    case(F_SINUS):
        passno    = FPREANAL;
        break;
    case(F_NARROW): //  fall thro
    case(F_SQUEEZE)://  fall thro
    case(F_MOVE):   //  fall thro
    case(F_MOVE2):  //  fall thro
    case(F_INVERT): //  fall thro
    case(F_ROTATE): //  fall thro
    case(F_SUPPRESS):// fall thro
    case(F_MAKEFILT):
        if(dz->needpitch)
            passno = FPREANAL;  //  Extract pitch data (FPREANAL) before extracting formant env FSPECENV and then processing sound FPROCESS
        else
            passno = FSPECENV;
        break;
    case(F_NEGATE):
        if(dz->needpitch)
            passno = FPREANAL;  //  Extract pitch data (FPREANAL) before processing sound FPROCESS
        else
            passno = FPROCESS;  //  Otheriwise SKIP extracting formantenv (not needed) and go dircetly to FPROCESS
        break;
    case(F_SEE):
        passno = FSPECENV;      //  Extract formant env FSPECENV before processing sound FPROCESS
        break;
    case(F_SEEPKS):
        passno = FPROCESS;      //  Go directly to processing sound FPROCESS
        break;
    }
    while(passno < 3) {
/* TEST *
fprintf(stderr,"passno = %d\n",passno);
* TEST */

        //  Which modes skip some of the processing loops?

        if(passno > 0 && dz->mode == F_SYLABTROF) {
            passno++;   
            continue;   //  F_SYLABTROF needs to get loudness envelope (during pitch-analysis routine) but needs nothing else;
        }
        if(passno == FSPECENV && dz->mode == F_NEGATE) {
            passno++;   //  F_NEGATE may use pass0 (to get pitch), but doesn't need to get spectral peaks
            continue;
        }
        times_index = 1;
        inner_lpcnt = 0;
        if(sloom)
            dz->total_samps_read = 0L;

        //  Messaging
        
        switch(passno) {
        case(FPREANAL): //  (pass 0) EXTRACTING PITCH, AND LOUDNESS-ENVELOPE
            switch(dz->mode) {
            case(F_MOVE):       // fall thro
            case(F_MOVE2):      // fall thro
            case(F_NEGATE):     // fall thro
            case(F_INVERT):     // fall thro
            case(F_NARROW):     // fall thro
            case(F_SQUEEZE):    // fall thro
            case(F_ROTATE):     // fall thro
            case(F_ARPEG):      // fall thro
            case(F_OCTSHIFT):   // fall thro
            case(F_TRANS):      // fall thro
            case(F_FRQSHIFT):   // fall thro
            case(F_RESPACE):    // fall thro
            case(F_PINVERT):    // fall thro
            case(F_PEXAGG):     // fall thro
            case(F_PQUANT):     // fall thro
            case(F_PCHRAND):    // fall thro
            case(F_SINUS):      // fall thro
            case(F_RAND):       // fall thro
            case(F_MAKEFILT):   fprintf(stdout,"INFO: Extracting pitch contour.\n");    break;
            case(F_SYLABTROF):  fprintf(stdout,"INFO: Extracting loudness contour.\n"); break;
            default:
                sprintf(errstr,"Unknown mode in pass %d.\n",passno);
                return PROGRAM_ERROR;
            }
            break;
        case(FSPECENV): //  (pass 1) GET SPECTRAL ENVELOPE
            switch(dz->mode) {
            case(F_SEE):    fprintf(stdout,"INFO: Testing level.\n");       break;
            case(F_SQUEEZE):    // fall thro
            case(F_SUPPRESS):   // fall thro
            case(F_INVERT):     // fall thro
            case(F_ROTATE):     // fall thro
            case(F_MAKEFILT):   // fall thro
            case(F_MOVE):       // fall thro
            case(F_MOVE2):      // fall thro
            case(F_SINUS):      // fall thro
            case(F_NARROW): fprintf(stdout,"INFO: Finding spectral peak data.\n");  break;
            case(F_ARPEG):      // fall thro
            case(F_OCTSHIFT):   // fall thro
            case(F_TRANS):      // fall thro
            case(F_FRQSHIFT):   // fall thro
            case(F_RESPACE):    // fall thro
            case(F_PINVERT):    // fall thro
            case(F_PEXAGG):     // fall thro
            case(F_PQUANT):     // fall thro
            case(F_PCHRAND):    // fall thro
            case(F_RAND):   fprintf(stdout,"INFO: Finding spectral envelope.\n");   break;
            case(F_NEGATE):     // fall thro
            case(F_SEEPKS):     // fall thro
            case(F_SYLABTROF):
                sprintf(errstr,"Should not be visited on this pass (%d).\n",passno);
                return PROGRAM_ERROR;
            default:
                sprintf(errstr,"Unknown mode in pass %d.\n",passno);
                return PROGRAM_ERROR;
            }
            break;
        case(FPROCESS): //  (pass 2) PROCESS SOUND
            switch(dz->mode) {
            case(F_SEE):        // fall thro    
            case(F_MAKEFILT):   // fall thro
            case(F_SEEPKS): fprintf(stdout,"INFO: Writing output.\n");                          break;
            case(F_NARROW):
            case(F_ROTATE):
            case(F_SQUEEZE):
            case(F_SUPPRESS):
            case(F_INVERT):
            case(F_NEGATE): 
            case(F_ARPEG):      // fall thro
            case(F_OCTSHIFT):   // fall thro
            case(F_TRANS):      // fall thro
            case(F_FRQSHIFT):   // fall thro
            case(F_RESPACE):    // fall thro
            case(F_PINVERT):    // fall thro
            case(F_PEXAGG):     // fall thro
            case(F_PQUANT):     // fall thro
            case(F_SINUS):      // fall thro
            case(F_PCHRAND):    // fall thro
            case(F_RAND):       // fall thro
            case(F_MOVE2):      // fall thro
            case(F_MOVE):   fprintf(stdout,"INFO: Transforming spectrum and writing output.\n");break;
            case(F_SYLABTROF):
                sprintf(errstr,"Should not be visited on this pass (%d).\n",passno);
                return PROGRAM_ERROR;
            default:    
                sprintf(errstr,"Unknown mode in pass %d.\n",passno);
                return PROGRAM_ERROR;
            }
            break;
        }
        fflush(stdout);
/* TEST *
fprintf(stderr,"START of OUTER LOOP passno = %d\n",passno);
* TEST */
        sndseekEx(dz->ifd[0],0,0);
        if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
           return(exit_status);
        dz->total_windows = 0;
        dz->time = 0.0;
        while(dz->ssampsread > 0) {
            dz->flbufptr[0] = dz->bigfbuf;
            windows_in_buf = dz->ssampsread/dz->wanted;
/* TEST *
fprintf(stderr,"GOING TO INNER LOOP passno = %d\n",passno);
* TEST */
            if((exit_status = new_inner_loop(windows_in_buf,&fmax,passno,limit,&inner_lpcnt,&phase,&times_index,&target,&previouspk,contourcnt,arp_param,&up,minpitch,maxpitch,dz))<0)
                return(exit_status);
            if(passno == FPROCESS) { // (pass 2)
                switch(dz->mode) {
                case(F_SEE):    //  Normalise pseudosndfile output
                    for(n = 0;n < dz->ssampsread;n++) 
                        dz->bigfbuf[n] = (float)(dz->bigfbuf[n] * normaliser);
                    break;
                case(F_SEEPKS): //  Output is to textfile
                case(F_SYLABTROF):
                case(F_MAKEFILT):
                    break;
                default:        //  Write analfile output
/* TEST *
fprintf(stderr,"WRITING OUTPUT\n");
* TEST */
                    if((exit_status = write_samps(dz->bigfbuf,dz->ssampsread,dz))<0)
                        return(exit_status);
                    break;
                }
            }
            if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
               return(exit_status);
        }
        switch(passno) {
        case(FPREANAL):
            if(dz->mode != F_SYLABTROF) {
                if((exit_status = tidy_up_pitch_data(dz))<0)
                    return(exit_status);
            }
            switch(dz->mode) {
            case(F_PINVERT)://  fall thro
            case(F_PEXAGG):
                if(dz->param[COLFLT] <= 0.0) {                  //  No centre pivot, specified, use the mean
                    if((exit_status = getmeanpitch(&dz->param[COLFLT],&minpitch,&maxpitch,dz))<0)
                        return exit_status;
                }
                break;
            case(F_PQUANT):
                if((exit_status = pitch_quantise_all(dz))<0)
                        return exit_status;
                break;
            case(F_PCHRAND):
                if(dz->quantcnt) {
                    if((exit_status = pitch_quantise_all(dz))<0)
                            return exit_status;
                }
                if((exit_status = pitch_randomise_all(dz))<0)
                        return exit_status;
                break;
            }
            break;
        case(FSPECENV): //  (pass 1)
            switch(dz->mode) {
            case(F_SEE):
                if(fmax < VERY_TINY_VAL) {
                    sprintf(errstr,"No significant data found in input file.\n");
                    return DATA_ERROR;
                }
                normaliser = SPECFNU_MAXLEVEL/fmax;
                break;
            case(F_SINUS):
                if((exit_status = formants_smooth_and_quantise_all(dz))<0)
                        return exit_status;
                break;
            }
            break;
        case(FPROCESS): //  (pass 2)
            if(dz->mode == F_MAKEFILT) {
                if((exit_status = build_filter(times_index,dz)) < 0)
                   return(exit_status);
            }
            break;
        }
        if(dz->ssampsread < 0) {
            sprintf(errstr,"Sound read error.\n");
            return(SYSTEM_ERROR);
        }
/* TEST *
fprintf(stderr,"END of outer_loop passno = %d\n",passno);
* TEST */
        if(passno == 0 && dz->mode == F_SYLABTROF) {
            if((exit_status = trof_detect(dz))<0)
                return(exit_status);
            break;
        }       
        passno++;
    }
    if(dz->mode == F_SEE || dz->mode == F_SEEPKS || dz->mode == F_MAKEFILT || dz->mode == F_SYLABTROF)
        return FINISHED;
    if(!dz->ischange) {
        sprintf(errstr,"NO CHANGE TO THE ORIGINAL SOUND\n");
        return DATA_ERROR;
    }
    //  Write final zero buffers to avoid any end click
    
    dz->flbufptr[0] = dz->bigfbuf;
    zerolen = dz->infile->Mlen/dz->infile->Dfac;
    for(n=0;n<zerolen;n++) {
        memset((char *)dz->flbufptr[0],0,(size_t)dz->wanted * sizeof(float));
        thisfrq = 0.0;
        for(cc=0,vc = 0;cc < dz->clength;cc++,vc+=2) {
            dz->flbufptr[0][FREQ] = (float)thisfrq;
            thisfrq += dz->chwidth;
        }
        dz->flbufptr[0] += dz->wanted;
    }
    if((exit_status = write_samps(dz->bigfbuf,zerolen * dz->wanted,dz))<0)
        return(exit_status);
    return FINISHED;
}

/************************** SETUP_FORMANT_ARRAYS *********************/

int setup_formant_arrays(int *contourcnt,dataptr dz)
{
    int exit_status, scalefact;
    int arraycnt, contour_halfbands_per_step;
    dz->specenv_type  = FREQWISE_FORMANTS;                  //  Frqwise formants seem to give more obvious formant peaks (pitvchwise is multipeaky)
    scalefact = (int)round((double)dz->clength/(double)CLENGTH_DEFAULT);
    dz->formant_bands *= scalefact;                         //  Width of spectral-envelope bands depends on number of anal points actually used
    if((exit_status = initialise_specenv(&arraycnt,dz))<0)  //  Formant arrays set up here
        return(exit_status);
    if((exit_status = set_the_specenv_frqs(arraycnt,dz))<0) //  dz->specenvcnt (& dz->infile->specenvcnt) established here
        return(exit_status);                                //  (both required in formant library functions)
    dz->descriptor_samps = dz->specenvcnt * DESCRIPTOR_DATA_BLOKS;
    if(is_coloring(dz)) {
        if((dz->specenvamp2 = (float *)malloc(dz->specenvcnt * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for second formant amplitude array.\n");
            return(MEMORY_ERROR);
        }
    }
    if(*contourcnt) {
        contour_halfbands_per_step = SPECFNU_LONG_FBANDS * scalefact;       //  Width of spectral-contour bands depends on number of anal points actually used
        if((exit_status = initialise_contourenv(contourcnt,contour_halfbands_per_step,dz))<0)   //  Contour arrays set up here
            return(exit_status);
    }
    return(FINISHED);
}

/************************ SET_SPECENV_FRQS ************************
 *
 * FREQWISE BANDS = number of channels for each specenv point
 */

int set_the_specenv_frqs(int arraycnt,dataptr dz)
{
    double frqstep, thisfrq, nextfrq;
    int k = 0;
    dz->specenvamp[0] = (float)0.0;
    dz->specenvfrq[0] = (float)1.0;
    dz->specenvpch[0] = (float)log10(dz->specenvfrq[0]);
    dz->specenvtop[0] = (float)dz->halfchwidth;
    frqstep = dz->halfchwidth * (double)dz->formant_bands;
    dz->frq_step = frqstep;
    thisfrq = dz->specenvtop[0];
    k = 1;
    while((nextfrq = thisfrq + frqstep) < dz->nyquist) {
        if(k >= arraycnt) {
            sprintf(errstr,"Formant array too small: set_the_specenv_frqs()\n");
            return(PROGRAM_ERROR);
        }
        dz->specenvfrq[k] = (float)nextfrq;
        dz->specenvpch[k] = (float)log10(dz->specenvfrq[k]);
        dz->specenvtop[k] = (float)min(dz->nyquist,nextfrq);
        thisfrq           = nextfrq;
        k++;
    }
    dz->specenvfrq[k] = (float)dz->nyquist;
    dz->specenvpch[k] = (float)log10(dz->nyquist);
    dz->specenvtop[k] = (float)dz->nyquist;
    dz->specenvamp[k] = (float)0.0;
    k++;
    dz->specenvcnt = k;
    dz->infile->specenvcnt = dz->specenvcnt;
    return(FINISHED);
}

/**************************** NEW_INNER_LOOP ***************************/

int new_inner_loop(int windows_in_buf,double *fmax,int passno,int limit,int *inner_lpcnt,double *phase,int *times_index,
                   double *target,int *previouspk,int contourcnt,int arp_param,int *up,double minpitch, double maxpitch,dataptr dz)
{
    int exit_status;
    int wc;
    double totalamp = 0.0;

    for(wc=0; wc<windows_in_buf; wc++) {
/* TEST *
if(passno == 2)
fprintf(stderr,"wc = %d\n",wc);
* TEST */
        if(dz->total_windows==0) {
            if(passno == FPREANAL && dz->mode != F_SYLABTROF)
                dz->pitches[0] = (float)NOT_PITCH;
            else
                memset((char *)dz->flbufptr[0],0,(size_t)dz->wanted * sizeof(float));
        } else {
            if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
                return(exit_status);
            rectify_window(dz->flbufptr[0],dz);
            switch(passno) {
            case(FPREANAL): //  (pass 0)
                if((exit_status = get_totalamp(&totalamp,dz->flbufptr[0],dz->wanted))<0)
                    return(exit_status);
                dz->parray[P_PRETOTAMP][dz->total_windows] = totalamp;
                if(dz->mode != F_SYLABTROF) {
                    if((exit_status = specpitch(*inner_lpcnt,target,dz))<0)
                        return(exit_status);
                }
                break;
            case(FSPECENV): //  (pass 1)
                if((exit_status = extract_specenv(0,0,dz))<0)
                    return(exit_status);
                switch(dz->mode) {
                case(F_NARROW):     //  fall thro    
                case(F_SQUEEZE):    //  fall thro    
                case(F_INVERT):     //  fall thro    
                case(F_ROTATE):     //  fall thro    
                case(F_SUPPRESS):   //  fall thro    
                case(F_MOVE):       //  fall thro    
                case(F_MOVE2):      //  fall thro    
                case(F_SINUS):      //  fall thro    
                case(F_MAKEFILT): exit_status = get_peaks_and_trofs(*inner_lpcnt,limit,previouspk,dz);  break;
                case(F_SEE):    exit_status = get_fmax(fmax,dz);                                    break;
                case(F_ARPEG):      //  fall thro
                case(F_OCTSHIFT):   //  fall thro
                case(F_TRANS):      //  fall thro
                case(F_FRQSHIFT):   //  fall thro
                case(F_RESPACE):    //  fall thro
                case(F_PINVERT):    //  fall thro
                case(F_PEXAGG):     //  fall thro
                case(F_PQUANT):     //  fall thro
                case(F_PCHRAND):    //  fall thro
                case(F_RAND):   /*  Spec colouring doesn't need peak and trof info */               break;      
                }
                break;
            case(FPROCESS): //  (pass 2)
                if((exit_status = extract_specenv(0,0,dz))<0)
                    return(exit_status);
                switch(dz->mode) {
                case(F_NARROW):  exit_status = formants_narrow(*inner_lpcnt,dz);                    break;      
                case(F_SQUEEZE): exit_status = formants_squeeze(*inner_lpcnt,dz);                   break;
                case(F_INVERT):  exit_status = formants_invert(*inner_lpcnt,phase,contourcnt,dz);   break;
                case(F_ROTATE):  exit_status = formants_rotate(*inner_lpcnt,phase,dz);              break;
                case(F_SUPPRESS):exit_status = formants_suppress(*inner_lpcnt,dz);                  break;
                case(F_MOVE2):      //  fall thro    
                case(F_MOVE):    exit_status = formants_move(*inner_lpcnt,dz);                      break;
                case(F_MAKEFILT):exit_status = formants_makefilt(*inner_lpcnt,times_index,dz);      break; 
                case(F_SEE):     exit_status = formants_see(dz);                                    break;
                case(F_NEGATE):  exit_status = formants_negate(contourcnt,dz);                      break;
                case(F_SINUS):   exit_status = formants_sinus(*inner_lpcnt,dz);                     break;
                case(F_ARPEG):      //  fall thro
                case(F_OCTSHIFT):   //  fall thro
                case(F_TRANS):      //  fall thro
                case(F_FRQSHIFT):   //  fall thro
                case(F_RESPACE):    //  fall thro
                case(F_PINVERT):    //  fall thro
                case(F_PEXAGG):     //  fall thro
                case(F_PQUANT):     //  fall thro
                case(F_PCHRAND):    //  fall thro
                case(F_RAND):    exit_status = formants_recolor(*inner_lpcnt,phase,up,arp_param,minpitch,maxpitch,contourcnt,dz);break;
                case(F_SEEPKS):  exit_status = formants_seepks(dz);                                 break;
                }
                break;
            }
            if(exit_status<0)
                return(exit_status);
/* TEST *
fprintf(stderr,"END OF new_inner_loop passno = %d inner_lpcnt = %d of %d\n",passno,*inner_lpcnt,dz->wlength);
* TEST */

        }
        dz->flbufptr[0] += dz->wanted;
        dz->total_windows++;
        dz->time = (float)(dz->time + dz->frametime);
        (*inner_lpcnt)++;
    }
    return(FINISHED);
}

/************************** CHECK_SPECFNU_PARAM_VALIDITY_AND_CONSISTENCY *********************/

int check_specfnu_param_validity_and_consistency(dataptr dz)
{
    int exit_status;
    double brkmax = 0.0, brkmin = 0.0;

    if(dz->mode == F_PCHRAND) {
        if(dz->brksize[FPRMAXINT]) {
            if((exit_status = get_maxvalue_in_brktable(&brkmax,FPRMAXINT,dz))<0)
                return exit_status;
        }
        if(dz->param[FPRMAXINT] == 0.0) {
            sprintf(errstr,"RANDOMISATION RANGE IS ZERO: THIS HAS NO EFFECT.\n");
            return DATA_ERROR;
        }
    }
    switch(dz->mode) {
    case(F_SINUS):
        if(dz->vflag[FSIN_FUND])
            dz->fundamental = 1;
        if(dz->vflag[FSIN_EXI])
            dz->retain_unpitched_data_for_deletion = 1;
        break;  
    case(F_ROTATE):
        if(dz->brksize[RSPEED]) {
            if((exit_status = get_maxvalue_in_brktable(&brkmax,RSPEED,dz))<0)
                return exit_status;
        }
        if(dz->param[RSPEED] > (double)((int)floor(1.0/dz->frametime))) {
            sprintf(errstr,"ROTATION RATE TOO HIGH FOR THIS ANALYSIS DATA.\n");
            return DATA_ERROR;
        }
        if(dz->vflag[ROTATE_XNH] && dz->vflag[ROTATE_KHM]) {
            sprintf(errstr,"SUPPRESS NON-HARMONICS WITH SUPPRESS-HARMONICS WILL PRODUCE ZERO SIGNAL LEVEL.\n");
            return DATA_ERROR;
        }
        dz->fundamental = 1;
        if(dz->vflag[ROTATE_EXI])
            dz->retain_unpitched_data_for_deletion = 1;
        break;
    case(F_INVERT):
        if(dz->brksize[FVIB]) {
            if((exit_status = get_maxvalue_in_brktable(&brkmax,FVIB,dz))<0)
                return exit_status;
        }
        if(dz->param[FVIB] > (double)((int)floor(1.0/dz->frametime))) {
            sprintf(errstr,"VIBRATO RATE TOO HIGH FOR THIS ANALYSIS DATA.\n");
            return DATA_ERROR;
        }
        if(dz->vflag[INVERT_XNH] && dz->vflag[INVERT_KHM]) {
            sprintf(errstr,"SUPPRESS NON-HARMONICS WITH SUPPRESS-HARMONICS WILL PRODUCE ZERO SIGNAL LEVEL.\n");
            return DATA_ERROR;
        }
        dz->fundamental = 1;
        if(dz->vflag[INVERT_EXI])
            dz->retain_unpitched_data_for_deletion = 1;
        break;
    case(F_PEXAGG): //  fall thro
    case(F_PINVERT):
        if(dz->brksize[COLFLT]) {
            if((exit_status = get_minvalue_in_brktable(&brkmin,COLFLT,dz))<0)
                return exit_status;
            if(dz->param[COLFLT] <= 0) {
                sprintf(errstr,"Bottom of range is %d : Enter 0 only as flag (indicates \"use mean pitch of input\")\n",SPEC_MIDIMIN);
                return DATA_ERROR;
            }
        }
        //  fall thro
    case(F_PCHRAND):    //  fall thro
    case(F_PQUANT):
        if(dz->vflag[RECOLOR_DWN] || dz->vflag[RECOLOR_CYC]) {
            if(!dz->brksize[COLRATE] && dz->param[COLRATE] == 0.0) {
                if(dz->vflag[RECOLOR_DWN])
                    fprintf(stdout,"WARNING: Downward orientation of arpeggiation ignored, as there is no arpeggiation set.\n");
                if(dz->vflag[RECOLOR_CYC])
                    fprintf(stdout,"WARNING: Cyclic orientation of arpeggiation ignored, as there is no arpeggiation set.\n");
                fflush(stdout);
            }
        }
        if(dz->vflag[RECOLOR_DWN] && dz->vflag[RECOLOR_CYC]) {
            sprintf(errstr,"CANNOT ARPEGGIATE SPECTRUM BOTH \"DOWN\" AND \"CYCLICALLY\".\n");
            return DATA_ERROR;
        }
        dz->fundamental = 1;
        if(dz->vflag[RECOLOR_EXI])
            dz->retain_unpitched_data_for_deletion = 1;
        break;
    case(F_ARPEG):      //  fall thro
    case(F_OCTSHIFT):   //  fall thro
    case(F_TRANS):      //  fall thro
    case(F_FRQSHIFT):   //  fall thro
    case(F_RESPACE):    //  fall thro
    case(F_RAND):
        if(dz->mode != F_ARPEG) {
            if(dz->vflag[RECOLOR_DWN] || dz->vflag[RECOLOR_CYC]) {
                if(!dz->brksize[COLRATE] && dz->param[COLRATE] == 0.0) {
                    if(dz->vflag[RECOLOR_DWN])
                        fprintf(stdout,"WARNING: Downward orientation of arpeggiation ignored, as there is no arpeggiation set.\n");
                    if(dz->vflag[RECOLOR_CYC])
                        fprintf(stdout,"WARNING: Cyclic orientation of arpeggiation ignored, as there is no arpeggiation set.\n");
                    fflush(stdout);
                }
            }
        }
        if(dz->vflag[RECOLOR_DWN] && dz->vflag[RECOLOR_CYC]) {
            sprintf(errstr,"CANNOT ARPEGGIATE SPECTRUM BOTH \"DOWN\" AND \"CYCLICALLY\".\n");
            return DATA_ERROR;
        }
        dz->fundamental = 1;
        if(dz->vflag[RECOLOR_EXI])
            dz->retain_unpitched_data_for_deletion = 1;
        break;
    case(F_SUPPRESS):
        dz->suprflag = 0;
        switch(dz->iparam[SUPRF]) {             //  Set flags for which formant to suppress
        case(1):    dz->suprflag |= 1;  break;
        case(2):    dz->suprflag |= 2;  break;
        case(3):    dz->suprflag |= 4;  break;
        case(4):    dz->suprflag |= 8;  break;
        case(12):   //  fall thro
        case(21):   dz->suprflag |= 3;  break;
        case(13):   //  fall thro
        case(31):   dz->suprflag |= 5;  break;
        case(14):   //  fall thro
        case(41):   dz->suprflag |= 9;  break;
        case(23):   //  fall thro
        case(32):   dz->suprflag |= 6;  break;
        case(24):   //  fall thro
        case(42):   dz->suprflag |= 10; break;
        case(34):   //  fall thro
        case(43):   dz->suprflag |= 12; break;
        case(123):  //  fall thro
        case(132):  //  fall thro
        case(213):  //  fall thro
        case(231):  //  fall thro
        case(312):  //  fall thro
        case(321):  dz->suprflag |= 7;  break;
        case(124):  //  fall thro
        case(142):  //  fall thro
        case(214):  //  fall thro
        case(241):  //  fall thro
        case(412):  //  fall thro
        case(421):  dz->suprflag |= 11; break;
        case(134):  //  fall thro
        case(143):  //  fall thro
        case(314):  //  fall thro
        case(341):  //  fall thro
        case(413):  //  fall thro
        case(431):  dz->suprflag |= 13; break;
        case(234):  //  fall thro
        case(243):  //  fall thro
        case(324):  //  fall thro
        case(342):  //  fall thro
        case(423):  //  fall thro
        case(432):  dz->suprflag |= 14; break;
        case(1234): //  fall thro
        case(1243): //  fall thro
        case(1324): //  fall thro
        case(1342): //  fall thro
        case(1423): //  fall thro
        case(1432): //  fall thro
        case(2134): //  fall thro
        case(2143): //  fall thro
        case(2314): //  fall thro
        case(2341): //  fall thro
        case(2413): //  fall thro
        case(2431): //  fall thro
        case(3124): //  fall thro
        case(3142): //  fall thro
        case(3214): //  fall thro
        case(3241): //  fall thro
        case(3412): //  fall thro
        case(3421): //  fall thro
        case(4123): //  fall thro
        case(4132): //  fall thro
        case(4213): //  fall thro
        case(4231): //  fall thro
        case(4312): //  fall thro
        case(4321): dz->suprflag |= 15; break;
    default:
            sprintf(errstr,"Suppress param must \"1\",\"2\",\"3\", or \"4\" or some combo of these, with no repeated items.\n");    
            return DATA_ERROR;
        }
        break;
    case(F_NARROW):
        dz->suprflag = 0;
        switch(dz->iparam[NARSUPRES]) {
        case(0):    break;
        case(1):    dz->suprflag |= 1;  break;
        case(2):    dz->suprflag |= 2;  break;
        case(3):    dz->suprflag |= 4;  break;
        case(4):    dz->suprflag |= 8;  break;
        case(12):   //  fall thro
        case(21):   dz->suprflag |= 3;  break;
        case(13):   //  fall thro
        case(31):   dz->suprflag |= 5;  break;
        case(14):   //  fall thro
        case(41):   dz->suprflag |= 9;  break;
        case(23):   //  fall thro
        case(32):   dz->suprflag |= 6;  break;
        case(24):   //  fall thro
        case(42):   dz->suprflag |= 10; break;
        case(34):   //  fall thro
        case(43):   dz->suprflag |= 12; break;
        case(123):  //  fall thro
        case(132):  //  fall thro
        case(213):  //  fall thro
        case(231):  //  fall thro
        case(312):  //  fall thro
        case(321):  dz->suprflag |= 7;  break;
        case(124):  //  fall thro
        case(142):  //  fall thro
        case(214):  //  fall thro
        case(241):  //  fall thro
        case(412):  //  fall thro
        case(421):  dz->suprflag |= 11; break;
        case(134):  //  fall thro
        case(143):  //  fall thro
        case(314):  //  fall thro
        case(341):  //  fall thro
        case(413):  //  fall thro
        case(431):  dz->suprflag |= 13; break;
        case(234):  //  fall thro
        case(243):  //  fall thro
        case(324):  //  fall thro
        case(342):  //  fall thro
        case(423):  //  fall thro
        case(432):  dz->suprflag |= 14; break;
        default:
            sprintf(errstr,"Suppress param must \"1\",\"2\",\"3\", or \"4\" or some combo of 3 of these, with no repeated items.\n");   
            return DATA_ERROR;
        }
        dz->fundamental = 0;
        if(dz->vflag[NRW_FUND]) {
            if(dz->suprflag & 1) {
                fprintf(stdout,"WARNING: Cannot \"Track Fundamental\" if supressing formants 1: Ignoring.\n");
                fflush(stdout);
            } else
                dz->fundamental = 1;
        }
        if(dz->vflag[NRW_XNH] && dz->vflag[NRW_KHM]) {
            sprintf(errstr,"SUPPRESS NON-HARMONICS WITH SUPPRESS-HARMONICS WILL PRODUCE ZERO SIGNAL LEVEL.\n");
            return DATA_ERROR;
        }
        dz->fundamental = 1;
        if(dz->vflag[NRW_EXI])
            dz->retain_unpitched_data_for_deletion = 1;
        break;
    case(F_MAKEFILT):
        if(dz->vflag[KEEPAMP] && dz->vflag[KEEPINV]) {
            sprintf(errstr,"You cannot use both the peak amplitudes and the peak-amplitude-inverses.\n");   
            return DATA_ERROR;
        }
        dz->fundamental = 0;
        if(dz->vflag[FLT_FUND])
            dz->fundamental = 1;
        break;
    case(F_SQUEEZE):
        dz->fundamental = 0;
        if(dz->vflag[SQZ_FUND]) {
            if(dz->vflag[AT_TROFS]) {
                fprintf(stdout,"WARNING: Cannot \"Track Fundamental\" if squeezing around troughs: Ignoring.\n");
                fflush(stdout);
            } else if(dz->iparam[SQZAT] > 1) {
                fprintf(stdout,"WARNING: Cannot \"Track Fundamental\" if not squeezing around 1st formant: Ignoring.\n");
                fflush(stdout);
            } else
                dz->fundamental = 1;
        }
        if(dz->vflag[SQZ_XNH] && dz->vflag[SQZ_KHM]) {
            sprintf(errstr,"SUPPRESS NON-HARMONICS WITH SUPPRESS-HARMONICS WILL PRODUCE ZERO SIGNAL LEVEL.\n");
            return DATA_ERROR;
        }
        dz->fundamental = 1;
        if(dz->vflag[SQZ_EXI])
            dz->retain_unpitched_data_for_deletion = 1;
        break;
    case(F_NEGATE):
        if(dz->vflag[NEG_XNH] && dz->vflag[NEG_KHM]) {
            sprintf(errstr,"SUPPRESS NON-HARMONICS WITH SUPPRESS-HARMONICS WILL PRODUCE ZERO SIGNAL LEVEL.\n");
            return DATA_ERROR;
        }
        if(dz->vflag[NEG_XNH] || dz->vflag[NEG_KHM])
            dz->fundamental = 1;
        if(dz->vflag[NEG_EXI])
            dz->retain_unpitched_data_for_deletion = 1;
        break;
    case(F_MOVE):
        if(dz->vflag[MOV_XNH] && dz->vflag[MOV_KHM]) {
            sprintf(errstr,"SUPPRESS NON-HARMONICS WITH SUPPRESS-HARMONICS WILL PRODUCE ZERO SIGNAL LEVEL.\n");
            return DATA_ERROR;
        }
        dz->fundamental = 1;
        if(dz->vflag[MOV_EXI])
            dz->retain_unpitched_data_for_deletion = 1;
        break;
    case(F_MOVE2):
        if(dz->vflag[MOV2_XNH] && dz->vflag[MOV2_KHM]) {
            sprintf(errstr,"SUPPRESS NON-HARMONICS WITH SUPPRESS-HARMONICS WILL PRODUCE ZERO SIGNAL LEVEL.\n");
            return DATA_ERROR;
        }
        dz->fundamental = 1;
        if(dz->vflag[MOV2_EXI])
            dz->retain_unpitched_data_for_deletion = 1;
        break;
    }
    if(dz->fundamental)
        dz->needpitch = 1;
    switch(dz->mode) {
    case(F_OCTSHIFT):// fall thro
    case(F_TRANS):   // fall thro
    case(F_FRQSHIFT):// fall thro
    case(F_RESPACE): // fall thro
    case(F_PINVERT): // fall thro
    case(F_PEXAGG):  // fall thro
    case(F_PQUANT):  // fall thro
    case(F_PCHRAND): // fall thro
    case(F_RAND):
        if(dz->param[COL_LO] >= dz->param[COL_HI]) {
            sprintf(errstr,"Low frq cutoff is >= high frq cutoff. No output will be generated.\n");
            return DATA_ERROR;
        }
        break;
    }
    switch(dz->mode) {
    case(F_PINVERT): // fall thro
    case(F_PEXAGG):  // fall thro
    case(F_PQUANT):  // fall thro
    case(F_PCHRAND):
        if(dz->param[COLLOPCH] >= dz->param[COLHIPCH]) {
            sprintf(errstr,"Minimum acceptable pitch >= Maximum acceptable pitch. No output will be generated.\n");
            return DATA_ERROR;
        }
        break;
    }
    return FINISHED;
}

/************************** FORMANTS_NARROW *********************/

int formants_narrow(int inner_lpcnt,dataptr dz)
{
    int exit_status, n, k, thistrof, thispeak, nexttrof, spstt, spend, ccstt, vcstt, ccend = 0, ccendmax, vcend, cc, vc, chdiff, nuchdiff, chomit;
    int zccstt, zccend, zvcstt, newcc, get_fundamental = 0, last_formant_zeroed, top_hno;
    float frqstt, frqend, frq;
    float newfrq, the_fundamental = 0.0;
    double minamp, maxamp, constrict, pre_amptotal = 0.0, post_amptotal = 0.0;
    int flbuf_pktrofs[9];
    int   *peakat = dz->iparray[PEAKPOS];
    peakat += inner_lpcnt * PKBLOK;

    if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    if(dz->fundamental)
        the_fundamental = dz->pitches[inner_lpcnt];

    //  FIND THE flbufptr LOCATIONS OF THE PEAKS AND TROFS INDICATED IN THE specevamp ENVELOPE CURVE    

    for(n = 0; n < PKBLOK; n+=2) {                  //  Going through the trof and peaks in pairs
        if(dz->fundamental && n == 0)
            get_fundamental = 1;
        thistrof = n;
        thispeak = n+1;
        nexttrof = n+2;
        spstt = peakat[n];                          //  Get specenvamp locations of a trof.
        if(spstt == 0)                              //  Find specenv frqs associated with top and bottom of this trof
            frqstt = 0.0f;
        else
            frqstt = dz->specenvtop[spstt-1];
        frqend = dz->specenvtop[spstt];
        ccstt  = (int)floor(frqstt/dz->chwidth);    //  Find the flbufpr channels associated with these frqs
        ccend  = (int)ceil(frqend/dz->chwidth);
        minamp = HUGE;                              //  Find the flbuptr location of minimum amplitude.
        vcstt  = ccstt * 2;
        flbuf_pktrofs[thistrof] = ccstt;
        for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
            if(dz->flbufptr[0][AMPP] < minamp) {
                minamp = dz->flbufptr[0][AMPP];
                flbuf_pktrofs[thistrof] = cc;
            }
        }
        zccstt = flbuf_pktrofs[thistrof];           //  Remember flbufptr location of trof, for use (if ness) in formant zeroing
        if(n == PKBLOK - 1)                         //  EXIT when final trof sussed.
            break;
        spend  = peakat[nexttrof];                  //  Find specenv location of next TROF
        frqstt = dz->specenvtop[spstt];             //  Find the top of the frequency band associated with the lower trof
        frqend = dz->specenvtop[spend - 1];         //  Find the bottom of the frequency band associated with the upper trof
        ccstt  = (int)floor(frqstt/dz->chwidth);    //  Find the flbufpr channels associated with these frqs
        ccend  = (int)ceil(frqend/dz->chwidth);
        maxamp = -HUGE;                             //  Find flbufptr location of PEAK amplitude between the trofs
        vcstt  = ccstt * 2;
        flbuf_pktrofs[thispeak] = ccstt;
        for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
            if(dz->flbufptr[0][AMPP] > maxamp) {
                maxamp = dz->flbufptr[0][AMPP];
                flbuf_pktrofs[thispeak] = cc;
            }
        }
        if(get_fundamental) {                       //  If at peak1 and we want to force getting the fundamental (e.g. if adjacent harmonic louder)
            if(the_fundamental > 0.0) {
                if((exit_status = locate_channel_of_fundamental(inner_lpcnt,&newfrq,&newcc,dz))<0)
                    return exit_status;
                flbuf_pktrofs[thispeak] = newcc;
            }
            get_fundamental = 0;                    //  Only (try to) get fundamental when in first formant
        }
        if(dz->suprflag) {
            k = n/2;
            k = (int)round(pow(2,k));                   //  n = 0,2,4,6   n/2 = 0,1,2,3   pow(2,n/2) = 1,2,4,8 = 0001,0010,0100,1000
            if(dz->suprflag & k) {                      //  If formant flagged for zeroing
                spend  = peakat[nexttrof];              //  Find flbufptr location of minamp at NEXT trof
                frqstt = dz->specenvtop[spend-1];
                frqend = dz->specenvtop[spend];
                ccstt  = (int)ceil(frqstt/dz->chwidth);
                ccend  = (int)ceil(frqend/dz->chwidth);
                minamp = HUGE;
                zccend = ccstt;
                for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
                    if(dz->flbufptr[0][AMPP] < minamp) {
                        minamp = dz->flbufptr[0][AMPP];
                        zccend = cc;
                    }
                }
                zvcstt = zccstt * 2;                    //  Zero flbufptr between the 2 trofs
                for(cc = zccstt,vc = zvcstt; cc < zccend; cc++,vc+=2)
                    dz->flbufptr[0][AMPP] = 0.0f;
            }
        }
    }
    if(flbuf_pktrofs[0] > 0) {                      //  zero spectrum below initial trof.
        for(cc = 0,vc = 0; cc < flbuf_pktrofs[0]; cc++,vc+=2)
            dz->flbufptr[0][AMPP] = 0.0f;
    }
    ccendmax = 0;
    last_formant_zeroed = 0;
    for(n = 0; n < PKBLOK-1; n++) {                 //  Going through the trof and peaks in pairs
        ccstt  = flbuf_pktrofs[n];                  //  Find flbhfptr trof/peak.
        ccend  = flbuf_pktrofs[n+1];                //  Find following flbufptr peak/trof.
        if(n == PKBLOK-2)
            ccendmax = ccend;
        if(last_formant_zeroed)
            break;
        chdiff = ccend - ccstt;                     //  Find channel step.
        if(dz->suprflag) {
            if(EVEN(n)) {
                k = n/2;
                k = (int)round(pow(2,k));
                if(dz->suprflag & k) {                  //  If formant zeroed
                    if(k == F4_ZEROED)                  //  If last formant zeroed
                        last_formant_zeroed = 1;        //  Don't skip to next trof here as we want to set ccendmax before exiting!!!
                    else                                //  but otherwise
                        n++;                            //  skip to next trof
                    continue;
                }
            }
        }
        nuchdiff = (int)round((double)chdiff * dz->param[NARROWING]);
        chomit = chdiff - nuchdiff;                 //  Reduce it by SQUEEZE and see if any channels are lost
        if(chomit > 0) {
            k = 0;                                  //  If they are, zero this number of channels from trof, widening the trof.
            if(EVEN(n)) {                           //  If we're at a trof BEFORE a peak
                while(k < chomit) {                 //  forwardwise
                    vc = ccstt * 2;
                    dz->flbufptr[0][AMPP] = 0.0f;
                    k++;
                    ccstt++;
                }  // (and forcing trof start to zero).
            } else {                                //  If we're at a trof AFTER a peak
                while(k < chomit) {                 //  backwardswise
                    vc = ccend * 2;
                    dz->flbufptr[0][AMPP] = 0.0f;
                    k++;
                    ccend--;
                }
            }
        }
        chdiff = ccend - ccstt;                     //  After possible expansion of trof, calculate new chdiff
        vcstt  = ccstt * 2;
        vcend  = ccend * 2;
        if(EVEN(n)) {                               //  Trof followed by peak, "splice" upwards:   trof (possibly at new location) forced to zero: peak unchanged.
            for(k = 0, cc = ccstt, vc = vcstt; cc < ccend; k++, cc++, vc+=2) {
                constrict = (double)k/(double)chdiff;
                dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * constrict);
            }
        } else {                                    //  Peak followed by trof, "splice" downwards: trof (possibly at new location) forced to zero: peak unchanged.
            for(k = 0, cc = ccend, vc = vcend; cc > ccstt; k++, cc--, vc-=2) {
                constrict = (double)k/(double)chdiff;
                dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * constrict);
            }
        }
    }
    if(dz->vflag[ZEROTOP])                          //  Zero the remaining upper part of the spectrum (if flagged).
        zero_spectrum_top(ccendmax,dz);

    top_hno = 0;
    for(cc = 0, vc = 0; cc < dz->clength; cc++, vc+=2) {
        frq = dz->flbufptr[0][FREQ];
        if(dz->xclude_nonh) {                                       //  If excluding non-harmonic data
            if(the_fundamental > 0.0 && !channel_holds_harmonic(the_fundamental,frq,cc,&top_hno,dz)) {
                dz->flbufptr[0][AMPP] = 0.0f;                       //  If the window has no pitch, or the frq is not a harmonic of fundamental
                continue;                                           //  zero it
            }
        } else if(dz->vflag[NRW_KHM]) {                             //  If suppressing harmonics    
            if(the_fundamental > 0.0) {                             //  IF the chanfrq is a harmonic, zero it                                                       
                if(channel_holds_harmonic(the_fundamental,frq,cc,&top_hno,dz)) {
                    dz->flbufptr[0][AMPP] = 0.0f;
                    continue;
                }
            }
        }                                                           //  If there is no pitch (or pitch value has NOT been interpolated thro unpitched area)
        if(the_fundamental < 0.0) {                                 //  this means that we have flagged up non-pitch and silence for deletion
            dz->flbufptr[0][AMPP] = 0.0f;                           //  Zero the channel
                continue;
        }
    }

    if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    if(post_amptotal > pre_amptotal) {
        if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
            return(exit_status);
    }
    if(dz->param[FGAIN] > 0.0 && !flteq(dz->param[FGAIN],1.0))  {
        for(vc=0;vc<dz->wanted;vc+=2)
            dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FGAIN]);
    }
    return FINISHED;
}

/************************** FORMANTS_SQUEEZE *********************/

int formants_squeeze(int inner_lpcnt,dataptr dz)
{
    int exit_status, thistrof, nexttrof, centre, spstt, spend, ccstt, vcstt, ccend, cc, vc, lastcc, locc, hicc, nucc, nuvc, newcc, top_hno, get_fundamental = 0;
    float frqstt, frqend, frq, centrefrq, newfrq, the_fundamental = 0.0;
    double lastamp, minamp, maxamp, frqoffset, nuoffset, nufrq, pre_amptotal = 0.0, post_amptotal = 0.0;
    int *peakat = dz->iparray[PEAKPOS];
    peakat += inner_lpcnt * PKBLOK;

    if(dz->fundamental) {
        get_fundamental = 1;
        the_fundamental = dz->pitches[inner_lpcnt];
    }
    if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);

    centre = (dz->iparam[SQZAT] * 2) - 1;           //  Formants 1,2,3,4 peaks at peaktrof-points 1,3,5,7
    if(dz->vflag[AT_TROFS])
        centre++;                                   //  Trofs above them at peaktrof-points 2,4,6,8

    if(dz->vflag[AT_TROFS]) {
        thistrof = centre;
        spstt = peakat[thistrof];                   //  Get specenvamp locations of a trof.
        if(spstt == 0)                              //  Find specenv frqs associated with top and bottom of this trof
            frqstt = 0.0f;
        else
            frqstt = dz->specenvtop[spstt-1];
        frqend = dz->specenvtop[spstt];
        ccstt  = (int)floor(frqstt/dz->chwidth);    //  Find the flbufpr channels associated with these frqs
        ccend  = (int)ceil(frqend/dz->chwidth);
        minamp = HUGE;                              //  Find the flbuptr location of minimum amplitude.
        vcstt  = ccstt * 2;
        centre = ccstt;
        for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
            if(dz->flbufptr[0][AMPP] < minamp) {
                minamp = dz->flbufptr[0][AMPP];
                centre = cc;
            }
        }
    } else {
        thistrof = centre - 1;
        nexttrof = centre + 1;
        spstt  = peakat[thistrof];                  //  Find specenv location of previous TROF
        spend  = peakat[nexttrof];                  //  Find specenv location of next TROF
        frqstt = dz->specenvtop[spstt];             //  Find the top of the frequency band associated with the lower trof
        frqend = dz->specenvtop[spend - 1];         //  Find the bottom of the frequency band associated with the upper trof
        ccstt  = (int)floor(frqstt/dz->chwidth);    //  Find the flbufpr channels associated with these frqs
        ccend  = (int)ceil(frqend/dz->chwidth);
        maxamp = -HUGE;                             //  Find flbufptr location of PEAK amplitude between the trofs
        vcstt  = ccstt * 2;
        centre = ccstt;
        for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
            if(dz->flbufptr[0][AMPP] > maxamp) {
                maxamp = dz->flbufptr[0][AMPP];
                centre = cc;
            }
        }
        if(get_fundamental) {                       //  If at peak1 and we want to force getting the fundamental (e.g. if adjacent harmonic louder)
            if(the_fundamental  > 0.0) {
                if((exit_status = locate_channel_of_fundamental(inner_lpcnt,&newfrq,&newcc,dz))<0)
                    return exit_status;
                centre = newcc;
            }
            get_fundamental = 0;    
        }
    }

    if(dz->xclude_nonh || dz->vflag[SQZ_KHM]) {
        top_hno = 0;
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc+=2) {
            frq = dz->flbufptr[0][FREQ];
            if(dz->xclude_nonh) {                                       //  If excluding non-harmonic data
                if(the_fundamental > 0.0 && !channel_holds_harmonic(the_fundamental,frq,cc,&top_hno,dz)) {
                    dz->flbufptr[0][AMPP] = 0.0f;                       //  If the window has no pitch, or the frq is not a harmonic of fundamental
                    continue;                                           //  zero it
                }
            } else if(dz->vflag[SQZ_KHM]) {                             //  If suppressing harmonics    
                if(the_fundamental > 0.0) {                             //  IF the chanfrq is a harmonic, zero it                                                       
                    if(channel_holds_harmonic(the_fundamental,frq,cc,&top_hno,dz)) {
                        dz->flbufptr[0][AMPP] = 0.0f;
                        continue;
                    }
                }
            }                                                           //  If there is no pitch (or pitch value has NOT been interpolated thro unpitched area)
            if(the_fundamental < 0.0) {                                 //  this means that we have flagged up non-pitch and silence for deletion
                dz->flbufptr[0][AMPP] = 0.0f;                           //  Zero the channel
                    continue;
            }
        }
    }
    vc = centre*2;
    centrefrq = dz->flbufptr[0][FREQ];              //  Note the freq at the centre of the shribkage
    lastcc  = -1;                                   //  Preset variables    
    lastamp = -HUGE;
    locc    = dz->clength + 1;
    for(cc=0,vc=0;cc<centre;cc++,vc+=2) {           //  Squeezespectrum below centrefrq
        frqoffset = centrefrq - dz->flbufptr[0][FREQ];
        nuoffset  = frqoffset * dz->param[SQZFACT]; //  Find the altered frq of this channel, once spectrum squeezed
        nufrq     = centrefrq - nuoffset;
        nucc      = (int)round(nufrq/dz->chwidth);  //  Find new channel-location for this frq
        if(nucc < locc)
            locc = nucc;                            //  Note the lowest channel used by squeezed spectrum   
        nuvc = nucc * 2;
        if(nucc == lastcc) {                        //  If some other data has already been moved into this channel, keep whichever is the loudest
            if(dz->flbufptr[0][AMPP] > lastamp) {
                dz->flbufptr[0][nuvc]   = dz->flbufptr[0][AMPP];
                dz->flbufptr[0][nuvc+1] = (float)nufrq;
            }
        } else {                                    //  Otherwise, simply move transformed data into this new channel (which may be original chan, just with a frq change)
            dz->flbufptr[0][nuvc]   = dz->flbufptr[0][AMPP];
            dz->flbufptr[0][nuvc+1] = (float)nufrq;
        }
        lastamp = dz->flbufptr[0][nuvc];
        lastcc = nucc;
    }
    for(cc=0,vc=0;cc<locc;cc++,vc+=2)               //  Zero spectrum below squeezed material.
        dz->flbufptr[0][AMPP] = 0.0f;

    cc = centre + 1;                                //  Similar procedure for spectrum above centre
    vc = cc * 2;
    lastcc  = -1;
    lastamp = -HUGE;
    hicc    = -1;
    for(;cc<dz->clength;cc++,vc+=2) {
        frqoffset = dz->flbufptr[0][FREQ] - centrefrq;
        nuoffset  = frqoffset * dz->param[SQZFACT];
        nufrq     = centrefrq + nuoffset;
        nucc      = (int)round(nufrq/dz->chwidth);
        if(nucc > hicc)
            hicc = nucc;                            //  But new locate the highest channel used by squeezed spectrum    
        nuvc = nucc * 2;
        if(nucc == lastcc) {
            if(dz->flbufptr[0][AMPP] > lastamp) {
                dz->flbufptr[0][nuvc]   = dz->flbufptr[0][AMPP];
                dz->flbufptr[0][nuvc+1] = (float)nufrq;
            }
        } else {
            dz->flbufptr[0][nuvc]   = dz->flbufptr[0][AMPP];
            dz->flbufptr[0][nuvc+1] = (float)nufrq;
        }
        lastamp = dz->flbufptr[0][nuvc];
        lastcc = nucc;
    }
    cc = hicc + 1;
    vc = cc * 2;
    for(;cc<dz->clength;cc++,vc+=2)                 //  Zero spectrum above squeezed material.
        dz->flbufptr[0][AMPP] = 0.0f;

    if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    if(post_amptotal > pre_amptotal) {
        if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
            return(exit_status);
    }
    if(dz->param[FGAIN] > 0.0 && !flteq(dz->param[FGAIN],1.0))  {
        for(vc=0;vc<dz->wanted;vc+=2)
            dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FGAIN]);
    }
    return FINISHED;
}

/************************** FORMANTS_INVERT *********************/

int formants_invert(int inner_lpcnt,double *phase,int contourcnt,dataptr dz)
{
    int exit_status, cc, vc, top_hno;
    float the_fundamental;
    double vibpos, frq, specamp, contouramp, diff, pre_amptotal = 0.0, post_amptotal = 0.0;

    if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);

    if((exit_status = extract_contour(contourcnt,dz))<0)
        return exit_status;
    top_hno = 0;
    if(dz->xclude_nonh && dz->pitches[inner_lpcnt] < FSPEC_MINFRQ) {
        for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2)
            dz->flbufptr[0][AMPP] = 0.0f;
    } else {
        for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
            frq = (double)dz->flbufptr[0][FREQ];
            the_fundamental = (float)dz->pitches[inner_lpcnt];
            if(dz->xclude_nonh) {                                       //  If excluding non-harmonic data
                if(the_fundamental > 0.0 && !channel_holds_harmonic(the_fundamental,(float)frq,cc,&top_hno,dz)) {
                    dz->flbufptr[0][AMPP] = 0.0f;                       //  If the window has no pitch, or the frq is not a harmonic of fundamental
                    continue;                                           //  zero it
                }
            } else if(dz->vflag[INVERT_KHM]) {                          //  If suppressing harmonics    
                if(the_fundamental > 0.0) {                             //  IF the chanfrq is a harmonic, zero it                                                       
                    if(channel_holds_harmonic(the_fundamental,(float)frq,cc,&top_hno,dz)) {
                        dz->flbufptr[0][AMPP] = 0.0f;
                        continue;
                    }
                }
            }                                                           //  If there is no pitch (or pitch value has NOT been interpolated thro unpitched area)
            if(the_fundamental < 0.0) {                                 //  this means that we have flagged up non-pitch and silence for deletion
                dz->flbufptr[0][AMPP] = 0.0f;                           //  Zero the channel
                    continue;
            }
            if((exit_status = getspecenvamp(&specamp,frq,0,dz))<0)      //  Otherwise do spectral inversion
                return(exit_status);
            if((exit_status = getcontouramp(&contouramp,contourcnt,frq,dz))<0)
                return(exit_status);
            diff = specamp - contouramp;
            if(dz->param[FVIB] != 0.0) {
                vibpos = (sin(*phase));             //  sin from -1to1
                    diff *= vibpos;
                *phase = fmod(*phase + (dz->param[FVIB] * dz->phasefactor),TWOPI);
            }                                           //  phase advance depends on current frq ; fmoded to stay within range 0 to 2PI
            dz->flbufptr[0][AMPP] = (float)(max(0.0,contouramp - diff));
        }
    }   
    if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    if(post_amptotal > pre_amptotal) {
        if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
            return(exit_status);
    }
    if(dz->param[FGAIN] > 0.0 && !flteq(dz->param[FGAIN],1.0))  {
        for(vc=0;vc<dz->wanted;vc+=2)
            dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FGAIN]);
    }
    return FINISHED;
}

/************************** FORMANTS_ROTATE *********************/

int formants_rotate(int inner_lpcnt,double *phase,dataptr dz)
{
    int exit_status, spstt, spend, ccstt, ccend, vcstt, locc, lovc, hicc, hivc, cc, vc, nucc, nuvc, lastcc, top_hno;
    float frqstt, frqend, lofrq, hifrq, frq, the_fundamental = 0.0;
    double minamp, lastamp, frqdiff, frqstep, nufrq, phasestep, pre_amptotal = 0.0, post_amptotal = 0.0;
    int   *peakat = dz->iparray[PEAKPOS];

    peakat += inner_lpcnt * PKBLOK;

    if(dz->fundamental)
        the_fundamental = dz->pitches[inner_lpcnt];

    if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);

    spstt = peakat[0];
    spend = peakat[PKBLOK-1];
    if(spstt == 0)                              //  Find specenv frqs associated with top and bottom of this trof
        frqstt = 0.0f;
    else
        frqstt = dz->specenvtop[spstt-1];
    frqend = dz->specenvtop[spstt];
    ccstt  = (int)floor(frqstt/dz->chwidth);    //  Find the flbufpr channels associated with these frqs
    ccend  = (int)ceil(frqend/dz->chwidth);
    minamp = HUGE;                              //  Find the flbuptr location of minimum amplitude.
    vcstt  = ccstt * 2;
    locc = ccstt;
    for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
        if(dz->flbufptr[0][AMPP] < minamp) {
            minamp = dz->flbufptr[0][AMPP];
            locc = cc;
        }
    }
    lovc = locc * 2;
    vc = lovc;
    lofrq = dz->flbufptr[0][FREQ];

    frqstt = dz->specenvtop[spend-1];
    frqend = dz->specenvtop[spend];
    ccstt  = (int)floor(frqstt/dz->chwidth);    //  Find the flbufpr channels associated with these frqs
    ccend  = (int)ceil(frqend/dz->chwidth);
    minamp = HUGE;                              //  Find the flbuptr location of minimum amplitude.
    vcstt  = ccstt * 2;
    hicc = ccstt;
    for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
        if(dz->flbufptr[0][AMPP] < minamp) {
            minamp = dz->flbufptr[0][AMPP];
            hicc = cc;
        }
    }
    hivc = hicc * 2;
    vc = hivc;
    hifrq = dz->flbufptr[0][FREQ];
    frqdiff = hifrq - lofrq;
    frqstep = (*phase) * frqdiff;
    lastcc = -1;
    lastamp = -HUGE;

    if(dz->xclude_nonh || dz->vflag[ROTATE_KHM]) {
        top_hno = 0;
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc+=2) {
            frq = dz->flbufptr[0][FREQ];
            if(dz->xclude_nonh) {                                       //  If excluding non-harmonic data
                if(the_fundamental > 0.0 && !channel_holds_harmonic(the_fundamental,frq,cc,&top_hno,dz)) {
                    dz->flbufptr[0][AMPP] = 0.0f;                       //  If the window has no pitch, or the frq is not a harmonic of fundamental
                    continue;                                           //  zero it
                }
            } else if(dz->vflag[ROTATE_KHM]) {                          //  If suppressing harmonics    
                if(the_fundamental > 0.0) {                             //  IF the chanfrq is a harmonic, zero it                                                       
                    if(channel_holds_harmonic(the_fundamental,frq,cc,&top_hno,dz)) {
                        dz->flbufptr[0][AMPP] = 0.0f;
                        continue;
                    }
                }
            }                                                           //  If there is no pitch (or pitch value has NOT been interpolated thro unpitched area)
            if(the_fundamental < 0.0) {                                 //  this means that we have flagged up non-pitch and silence for deletion
                dz->flbufptr[0][AMPP] = 0.0f;                           //  Zero the channel
                    continue;
            }
        }
    }

    for(cc = locc, vc = lovc; cc <= hivc; cc++, vc+=2) {
        nufrq = dz->flbufptr[0][FREQ] + frqstep;
        while(nufrq > hifrq)                        //  Keep nufrq within bounds
            nufrq -= frqdiff;
        while(nufrq < lofrq)
            nufrq += frqdiff;
        nucc = (int)round(nufrq/dz->chwidth);       //  Find new channel-location for this frq
        nuvc = nucc * 2;
        if(nucc == lastcc) {                        //  If some other data has already been moved into this channel, keep whichever is the loudest
            if(dz->flbufptr[0][AMPP] > lastamp) {
                dz->flbufptr[0][nuvc]   = dz->flbufptr[0][AMPP];
                dz->flbufptr[0][nuvc+1] = (float)nufrq;
            }
        } else {                                    //  Otherwise, simply move transformed data into this new channel (which may be original chan, just with a frq change)
            dz->flbufptr[0][nuvc]   = dz->flbufptr[0][AMPP];
            dz->flbufptr[0][nuvc+1] = (float)nufrq;
        }
        lastamp = dz->flbufptr[0][nuvc];
        lastcc = nucc;
    }
    phasestep = dz->phasefactor * dz->param[RSPEED];
    *phase += phasestep;                            //  Advance phase according to rotation-speed.
    while(*phase > 1.0)                             //  Keep rotation "phase" within 0-1 bounds.
        *phase -= 1.0;
    while(*phase < 0.0)
        *phase += 1.0;
    if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    if(post_amptotal > pre_amptotal) {
        if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
            return(exit_status);
    }
    if(dz->param[FGAIN] > 0.0 && !flteq(dz->param[FGAIN],1.0))  {
        for(vc=0;vc<dz->wanted;vc+=2)
            dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FGAIN]);
    }
    return FINISHED;
}

/************************** FORMANTS_SUPPRESS *********************/

int formants_suppress(int inner_lpcnt,dataptr dz)
{
    int exit_status, n, k, pk, thistrof, nexttrof, spstt, spend, ccstt, vcstt, ccend, pretrof, posttrof, cc ,vc, chandiff;
    float frqstt, frqend;
    double ampdiff, amphere, thisvalue, preminamp, postminamp, thisspecamp, pre_amptotal = 0.0, post_amptotal = 0.0;
    int   *peakat = dz->iparray[PEAKPOS];

    peakat += inner_lpcnt * PKBLOK;

    if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);

    for(n = 0,pk = 1; n < PKBLOK-1; n+=2,pk *=2) {      //  Going through the trofs in pairs
        if(dz->suprflag & pk) {                         //  If this formant is to be supressed              
            thistrof = n;
            nexttrof = n+2;
            spstt = peakat[thistrof];                   //  Get specenvamp locations of a trof.
            if(spstt == 0)                              //  Find specenv frqs associated with top and bottom of this trof
                frqstt = 0.0f;
            else
                frqstt = dz->specenvtop[spstt-1];
            frqend = dz->specenvtop[spstt];
            ccstt  = (int)floor(frqstt/dz->chwidth);    //  Find the flbufpr channels associated with these frqs
            ccend  = (int)ceil(frqend/dz->chwidth);
            preminamp = HUGE;                           //  Find the flbuptr location of minimum amplitude.
            vcstt   = ccstt * 2;
            pretrof = ccstt;
            for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
                if(dz->flbufptr[0][AMPP] < preminamp) {
                    preminamp = dz->flbufptr[0][AMPP];
                    pretrof = cc;
                }
            }

            spend = peakat[nexttrof];                   //  Get specenvamp locations of next trof.
            frqstt = dz->specenvtop[spend-1];
            frqend = dz->specenvtop[spend];
            ccstt  = (int)floor(frqstt/dz->chwidth);    //  Find the flbufpr channels associated with these frqs
            ccend  = (int)ceil(frqend/dz->chwidth);
            postminamp = HUGE;                          //  Find the flbuptr location of minimum amplitude.
            vcstt  = ccstt * 2;
            posttrof = ccstt;
            for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
                if(dz->flbufptr[0][AMPP] < postminamp) {
                    postminamp = dz->flbufptr[0][AMPP];
                    posttrof = cc;
                }
            }
            ampdiff  = postminamp - preminamp;
            chandiff = posttrof - pretrof;
            cc = pretrof + 1;                           //  Scale the intervening channel amps to a linear interp between the 2 trofs
            vc = cc * 2;
            k = 1;
            for(k = 1;k < chandiff;k++,cc++,vc+=2) {
                amphere = ampdiff * ((double)k/(double)chandiff);
                if((exit_status = getspecenvamp(&thisspecamp,(double)dz->flbufptr[0][FREQ],0,dz))<0)
                    return(exit_status);
                if(thisspecamp <= 0.0)
                    dz->flbufptr[0][AMPP] = 0.0f;
                else {
                    if((thisvalue = dz->flbufptr[0][AMPP] * amphere/thisspecamp)  < VERY_TINY_VAL)
                        dz->flbufptr[0][AMPP] = 0.0f;
                    else
                        dz->flbufptr[0][AMPP] = (float)thisvalue;
                }
            }
        }
    }
    if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    if(post_amptotal > pre_amptotal) {
        if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
            return(exit_status);
    }
    if(dz->param[FGAIN] > 0.0 && !flteq(dz->param[FGAIN],1.0))  {
        for(vc=0;vc<dz->wanted;vc+=2)
            dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FGAIN]);
    }
    return FINISHED;
}

/********************* FORMANTS_SEE *************************/

int formants_see(dataptr dz)
{
    int cc,vc, k, blokcnt = SPECFNU_BLOKCNT;
    double thisamp;
    if(dz->specenvcnt * blokcnt >= dz->wanted) {
        sprintf(errstr,"Writing output will overrun buffer.\n");
        return PROGRAM_ERROR;
    }
    blokcnt = 7;
    for(cc=0,vc=0;cc<dz->specenvcnt;cc++,vc+=blokcnt) {
        thisamp = dz->specenvamp[cc];
        for(k=0;k<blokcnt;k++)
            dz->flbufptr[0][vc+k] = (float)thisamp; //  Write amplitude vals sequentially, in blocks of 7, for easy viewability
    }
    while(vc<dz->wanted)                            // Fill up rest of output block with zeros
        dz->flbufptr[0][vc++] = 0.0f;
    return(FINISHED);
}

/********************* FORMANTS_SEE *************************/

int formants_seepks(dataptr dz)
{
    int n, k, pad, trofcnt = 0, peakcnt = 0, rising = 0, done = 0, vc = 0, localmaxat = 0, localminat = 0;
    double lastamp, thismaxamp = 0.0, thisminamp = 0.0, localmax,localmin;
    char temp[200], temp2[200];
    temp[0] = ENDOFSTR;
    lastamp = dz->specenvfrq[0];
    for(n=0;n<dz->specenvcnt;n++) {
        switch(n) {
        case(0):
            thismaxamp = dz->specenvamp[n];
            thisminamp = dz->specenvamp[n];
            break;
        case(1):
            if(dz->specenvamp[n] > lastamp) {
                sprintf(temp2,"%.1lf",dz->flbufptr[0][vc+1]);       //  Lowest frq in infile
                pad = 14 - strlen(temp2);
                for(k = 0;k < pad;k++)
                    strcat(temp2," ");
                strcat(temp,temp2);
                trofcnt++;
                thismaxamp = dz->specenvamp[n];
                rising = 1;
            } else {
                thisminamp = dz->specenvamp[n];
                rising = 0;
            }
        default:
            if(rising) {
                if(dz->specenvamp[n] > lastamp)
                    thismaxamp = dz->specenvamp[n];
                else {
                    localmax = -HUGE;
                    for(k = 0;k < dz->formant_bands; k++, vc+=2) {
                        if(dz->flbufptr[0][AMPP] > localmax) {
                            localmax = dz->flbufptr[0][AMPP];
                            localmaxat = vc;
                        }
                    }
                    sprintf(temp2,"%.1lf",dz->flbufptr[0][localmaxat+1]);
                    pad = 14 - strlen(temp2);
                    for(k = 0;k < pad;k++)
                        strcat(temp2," ");
                    strcat(temp,temp2);
                    peakcnt++;
                    thisminamp = dz->specenvamp[n];
                    rising = 0;
                }
            } else {    //  falling
                if(dz->specenvamp[n] < lastamp)
                    thisminamp = dz->specenvamp[n];
                else {
                    localmin = HUGE;
                    for(k = 0;k < dz->formant_bands; k++, vc+=2) {
                        if(dz->flbufptr[0][AMPP] < localmin) {
                            localmin = dz->flbufptr[0][AMPP];
                            localminat = vc;
                        }
                    }
                    if(trofcnt>0)
                        sprintf(temp2,"%.1lf",dz->flbufptr[0][localminat+1]);
                    else
                        sprintf(temp2,"%.1lf",dz->flbufptr[0][localminat+1]);
                    if(++trofcnt == 5) {
                        strcat(temp,temp2);
                        done = 1;
                    } else {
                        pad = 14 - strlen(temp2);
                        for(k = 0;k < pad;k++)
                            strcat(temp2," ");
                        strcat(temp,temp2);
                        thisminamp = dz->specenvamp[n];
                        rising = 1;
                    }
                }
            }
            break;
        }
        if(done)
            break;
        lastamp = dz->specenvamp[n];
    }
    if(trofcnt < 5) {
        if(!dz->warned) {
            fprintf(stdout,"WARNING: At least one window has insufficient peaks (less than 4).\n");
            fprintf(dz->fp,"INSUFFICIENT PEAKS\n");
            dz->warned = 1;
        }
    } else {
        sprintf(temp2,"\n");
        strcat(temp,temp2);
        fprintf(dz->fp,"%s",temp);
    }
    return(FINISHED);
}

/********************* FORMANTS_NEGATE *************************/

int formants_negate(int contourcnt,dataptr dz)
{
    int exit_status, cc, vc;
    double minamp, maxamp, ampsum, amp, amp2, pre_amptotal = 0.0, post_amptotal = 0.0;
    float frq;

    if((exit_status = extract_contour(contourcnt,dz))<0)
        return exit_status;
    if((exit_status = remember_contouramp(contourcnt,dz))<0)
        return exit_status;
    if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    maxamp = -HUGE;
    minamp = HUGE;
    for(cc=0,vc = 0;cc < dz->clength;cc++,vc+=2) {                      //  Find max and min amplitudes of this window
        minamp = min(minamp,dz->flbufptr[0][AMPP]);
        maxamp = max(maxamp,dz->flbufptr[0][AMPP]);
    }
    ampsum = maxamp + minamp;
    for(cc=0,vc = 0;cc < dz->clength;cc++,vc+=2)                        //  stepup-inside-ampdiff = X - min: newval uses this as stepdn-within-ampdiff = max - stepup
        dz->flbufptr[0][AMPP] = (float)ampsum - dz->flbufptr[0][AMPP];  //   = max - (X - min) = max + min - X = ampsum - X !!

    if(!dz->vflag[FLATT]) {
        if((exit_status = extract_contour(contourcnt,dz))<0)
            return exit_status;
        for(cc=0,vc = 0;cc < dz->clength;cc++,vc+=2) {
            frq = dz->flbufptr[0][FREQ];
            getcontouramp(&amp,contourcnt,frq,dz);      //  Contour now
            getcontouramp2(&amp2,contourcnt,frq,dz);    //  Contour before
            if(amp2 < VERY_TINY_VAL)
                dz->flbufptr[0][AMPP] = 0.0f;
            else 
                dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * amp/amp2);
        }
    }
    if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    if(post_amptotal > pre_amptotal) {
        if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
            return(exit_status);
    }
    if(dz->param[FGAIN] > 0.0 && !flteq(dz->param[FGAIN],1.0))  {
        for(vc=0;vc<dz->wanted;vc+=2)
            dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FGAIN]);
    }
    return FINISHED;
}

/*********************** SPECFNU_PREPROCESS **********************/

int specfnu_preprocess(dataptr dz)
{
    int exit_status, n, m;
    dz->warned = 0;
    dz->ischange = 1;
    dz->in_tune = pow(SEMITONE_INTERVAL,WITHIN_RANGE);
    switch(dz->mode) {
    case(F_SEE):        //  fall thro
    case(F_SEEPKS):     //  fall thro
    case(F_SYLABTROF):  //  fall thro
        return FINISHED;
    case(F_ARPEG):      //  fall thro
    case(F_OCTSHIFT):   //  fall thro
    case(F_TRANS):      //  fall thro
    case(F_FRQSHIFT):   //  fall thro
    case(F_RESPACE):    //  fall thro
    case(F_PINVERT):    //  fall thro
    case(F_PEXAGG):     //  fall thro
    case(F_PQUANT):     //  fall thro
    case(F_PCHRAND):    //  fall thro
    case(F_RAND):
        dz->phasefactor = dz->frametime;    //  Arpeggiation sweeps 0 & 1
        if((dz->iparray[ISHARM] = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store harmonics markers.\n");
            return(MEMORY_ERROR);
        }
        return FINISHED;
    case(F_NARROW):                     //  Narrowing by 10 shrinks formantsize to 1/10
        if(dz->brksize[NARROWING]) {
            for(n=0,m=1;n<dz->brksize[NARROWING];n++,m+=2)
                dz->brk[NARROWING][m] = 1.0/dz->brk[NARROWING][m];
        } else
            dz->param[NARROWING] = 1.0/dz->param[NARROWING];
        dz->warned = 0;
        break;
    case(F_SQUEEZE):                    //  Squeeze by 10 shrinks spectrum by 1/10
        if(dz->brksize[SQZFACT]) {
            for(n=0,m=1;n<dz->brksize[SQZFACT];n++,m+=2)
                dz->brk[SQZFACT][m] = 1.0/dz->brk[SQZFACT][m];
        } else
            dz->param[SQZFACT] = 1.0/dz->param[SQZFACT];
        dz->warned = 0;
        break;                          
    case(F_INVERT):                             //  Variation is sinusoidal
        dz->phasefactor = TWOPI * dz->frametime;//  At 1Hz, we advance 2PI in 1 sec, so we advance 2PI * frametime in frametime-secs
        break;                                  //  At 10Hz,we advance 2PI * 10 in 1 sec, so we advance 2PI * 10 * frametime in frametime-secs

    case(F_ROTATE):                             //  Variation sweeps through formants range from "0" to "1"
        dz->phasefactor = dz->frametime;        //  At 1Hz, we advance by 1 in 1 sec, so we advance 1 * frametime in frametime-secs
        break;                                  //  At 10Hz,we advance 10 in 1 sec, so we advance 10 * frametime in frametime-secs
        
    case(F_MAKEFILT):                   //  Creates array to store pitches to use in filter
        if((dz->parray[FILTPICH] = (double *)malloc(dz->iparam[FPKCNT] * 4 * dz->timeblokcnt * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Pitches data.\n");
            return(MEMORY_ERROR);
        }
        if((dz->parray[FILTAMP]  = (double *)malloc(dz->iparam[FPKCNT] * 4 * dz->timeblokcnt * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Amps data.\n");
            return(MEMORY_ERROR);
        }
        if((dz->parray[LOCALPCH] = (double *)malloc(dz->iparam[FPKCNT] * 4 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Pitches data.\n");
            return(MEMORY_ERROR);
        }
        if((dz->parray[LOCALAMP]  = (double *)malloc(dz->iparam[FPKCNT] * 4 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Amps data.\n");
            return(MEMORY_ERROR);
        }
        break;
    case(F_MOVE):
    case(F_MOVE2):
        for(n = FAMP1; n < PKSARRAYCNT; n++) {
            if((dz->fptr[n] = (float *)malloc(dz->specenvcnt * sizeof(float)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to store Intermediate formant data %d.\n",n+1);
                return(MEMORY_ERROR);
            }
        }
        for(n = FMULT;n < PRE_P_ARRAYCNT; n++) {
            if((dz->parray[n] = (double *)malloc(dz->clength * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to store Intermediate formant double-data, item %d.\n",n+1);
                return(MEMORY_ERROR);
            }
        }
        if((dz->iparray[FCNT] = (int *)malloc(5 * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Intermediate dformant array data counts.\n");
            return(MEMORY_ERROR);
        }
        dz->ischange = 0;
        break;
    case(F_SINUS):
        for(n=0,m = FOR_FRQ1;n < P_SINUS_ARRAYS; n++,m++) {
            if((dz->fptr[m] = (float *)malloc(dz->wlength * sizeof(float)))==NULL) {            //  4 arrays store pitch-tracks of 4 formants
                sprintf(errstr,"INSUFFICIENT MEMORY to store Formant %d pitch-tracks.\n",n+1);  //  4 more arrays store smoothed or quantised tracks
                return(MEMORY_ERROR);                                                           //  & (later) related transposition values
            }
        }
        for(n=0,m = F_CHTRAIN1;n < 9; n++,m++) {                                                //  4 arrays store the (original) chanpositions 
            if((dz->iparray[m] = (int *)malloc(dz->wlength * sizeof(int)))==NULL) {             //  of the formant peakfrqs
                sprintf(errstr,"INSUFFICIENT MEMORY to store Formant %d pitch-tracks.\n",n+1);
                return(MEMORY_ERROR);
            }
        }
        break;
    }
    if((exit_status = initialise_specenv2(dz)) < 0)
        return exit_status;
    if(dz->mode == F_NEGATE)
        return FINISHED;
    if((exit_status = initialise_peakstore(dz)) < 0)
        return exit_status;
    dz->pktrofcnt = PKBLOK;     //  We start counting aFTER 1 block of peaks, as window 1 has no PEAK info
    dz->badpks = 0;
    dz->zeroset = 0;
    return FINISHED;
}

/*********************** GET_PEAKS_AND_TROFS **********************/

int get_peaks_and_trofs(int inner_lpcnt,int limit,int *previouspk,dataptr dz)
{
    int exit_status, n, i, j, k, there, valid, rising = 0, done = 0;
    int abscnt = dz->pktrofcnt, localcnt = 0, blokstep, ilpcnt;
    float lastamp = 0.0;
    double interp;
    float *peaktrof = dz->fptr[PKTROF];
    int   *pos      = dz->iparray[PEAKPOS];
    float *pkdiff   = dz->fptr[PKDIFF];
    for(n=0;n<dz->specenvcnt;n++) {
        switch(n) {
        case(0):
            break;
        case(1):
            if(dz->specenvamp[n] > lastamp) {
                peaktrof[abscnt] = lastamp;
                pos[abscnt]      = n-1;
                abscnt++;
                localcnt++;
                rising = 1;
            } else
                rising = 0;
            break;
        default:
            if(rising) {
                if(dz->specenvamp[n] < lastamp) {
                    peaktrof[abscnt] = lastamp;
                    pos[abscnt]      = n-1;
                    abscnt++;
                    localcnt++;
                    rising = 0;
                }
            } else {    //  falling
                if(dz->specenvamp[n] > lastamp) {
                    peaktrof[abscnt] = lastamp;
                    pos[abscnt]      = n-1;
                    abscnt++;
                    localcnt++;
                    if(localcnt == PKBLOK)      //  5 trofs and 4 peaks
                        done = 1;
                    else
                        rising = 1;
                }
            }
            break;
        }
        if(done)
            break;
        lastamp = dz->specenvamp[n];
    }
    if(!done) {
        dz->badpks = 1;
        dz->pktrofcnt += PKBLOK;
        if(dz->pktrofcnt >= limit) {        //  If we're at LAST window, and there are bad blocks, forwardfill with last valid vals
            if(*previouspk < 0) {
                sprintf(errstr,"No window has (the required) 4 peaks.\n");
                return DATA_ERROR;
            }
            for(k = *previouspk + PKBLOK ;k < dz->pktrofcnt; k += PKBLOK) {
                for(j = 0,there = k, valid = *previouspk; j < PKBLOK; j++,there++,valid++)
                    peaktrof[there] = peaktrof[valid];
                if(dz->mode == F_SINUS) {
                    ilpcnt = k/PKBLOK;
                    if((exit_status = store_formant_frq_data(ilpcnt,dz))<0)
                        return (exit_status);
                }
            }
            return FINISHED;
        }
    }
    else {
        if(dz->badpks) {
            if(*previouspk < 0) {       //  Backfill  non-peak bloks at start with same vals as 1st true peakblok found

                for(k = 0;k < dz->pktrofcnt; k += PKBLOK) {
                    for(j = 0,there = k, valid = dz->pktrofcnt; j < PKBLOK; j++,there++,valid++)
                        peaktrof[there] = peaktrof[valid];
                    if(dz->mode == F_SINUS) {
                        ilpcnt = k/PKBLOK;
                        if((exit_status = store_formant_frq_data(ilpcnt,dz))<0)
                            return (exit_status);
                    }
                }

            } else {                        //  Or infill  non-peak bloks with interp of existing peakbloks

                for(j = 0;j < PKBLOK; j++)                                                      //  Get differences between peak and trogfs vals in the 2 valid pkbloks     
                    pkdiff[j] = (float)(peaktrof[dz->pktrofcnt + j] - peaktrof[*previouspk + j]);
                blokstep = (dz->pktrofcnt - *previouspk)/PKBLOK;                                //  Find number of bad blocks
                for(k = *previouspk + PKBLOK, i = 1;k < dz->pktrofcnt; k += PKBLOK, i++) {
                    interp = (double)i/(double)blokstep;                                        //  Set interp porportion
                    for(j = 0,there = k; j < PKBLOK; j++,there++)                               //  Write in interpd values         
                        peaktrof[there] = (float)(peaktrof[*previouspk + k] + (pkdiff[j] * interp));
                }
            }
            dz->badpks = 0;
        }
        *previouspk = dz->pktrofcnt;
        if(dz->mode == F_SINUS) {
            if((exit_status = store_formant_frq_data(inner_lpcnt,dz))<0)
                return (exit_status);
        }
        dz->pktrofcnt += PKBLOK;
        if(dz->pktrofcnt >= limit)
            return FINISHED;
    }
    return CONTINUE;
}

/****************************************** INITIALISE_PEAKSTORE ************************************/

int initialise_peakstore(dataptr dz) 
{
    if((dz->fptr[PKTROF] = (float *)malloc((dz->wlength + 2) * PKBLOK * sizeof(float)))==NULL) {        //  +2 = SAFETY
        sprintf(errstr,"INSUFFICIENT MEMORY for store of window peaks and troughs(2).\n");
        return(MEMORY_ERROR);
    }
    if((dz->fptr[PKDIFF] = (float *)malloc(PKBLOK * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for store of differences between window peaks and troughs.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[PEAKPOS] = (int *)malloc(dz->wlength * PKBLOK * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for store of locations  of peaks and troughs(2).\n");
        return(MEMORY_ERROR);
    }
    return FINISHED;
}

/************************** GET_FMAX ***********************/

int get_fmax(double *fmax,dataptr dz)
{
    int n;
    for(n = 0; n < dz->specenvcnt;n++)
        *fmax = max(dz->specenvamp[n],*fmax);
    return FINISHED;
}

/**************************** HANDLE_THE_MAKEFILT_SPECIAL_DATA ****************************/

int handle_the_makefilt_special_data(char *str,dataptr dz)
{
    int is_hs = -1, is_pitches = 0, is_scale = 0, is_elacs = 0, scale_division = 12, ignore_further_times = 0;
    int cnttimes = 0, cntpitches = 0, linecnt, pitchpos, oct1pitchpos, step, n, m,k, offset, done, mindiffat, nubincnt, timepos, trueoct_cnt;
    double dummy = 0.0, reference_pitch = 60.0, lasttime = 0.0, ratio, octstep, nextpitch, mindiff, thisdiff, nupitch;
    double pseudoct_size = 12.0, semitone_len, thispitch, octfoot = 0.0;

    FILE *fp;
    double *prepitch = NULL, *binpitch = NULL, *bintop = NULL;
    int arraysize, *ftime;
    char temp[800], *p;
    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read data.\n",str);
        return(DATA_ERROR);
    }
    linecnt = 0;
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        if(*p == '#') {                 //  Look for #HF, #HS, or #SCALES marker
            if(cnttimes == 0) {
                sprintf(errstr,"No time segmentation specified in file %s.\n",str);
                return(DATA_ERROR);
            }
            if(strncmp(p,"#HS",3) == 0) {
                is_hs = 1;
                is_pitches = 1;
            } else if(strncmp(p,"#HF",3) == 0) {
                is_hs = 0;
                is_pitches = 1;
            } else if(strncmp(p,"#SCALE",6) == 0) {
                is_hs = 0;
                is_scale = 1;
                is_pitches = 1;
            } else if(strncmp(p,"#ELACS",6) == 0) {
                is_hs = 0;
                is_elacs = 1;
                is_pitches = 1;
            }
            else {
//                sprintf(errstr,"Invalid Field, Set, or Scale marker (%d) on line %d in file %s\n",linecnt+1,p,str);
                //RWD I think it is meant to say this:
                sprintf(errstr,"Invalid Field, Set, or Scale marker (%s) on line %d in file %s\n",p,linecnt+1,str);
                return(DATA_ERROR);
            }
        } else {
            while(get_float_from_within_string(&p,&dummy)) {
                if(is_pitches) {
                    if(is_scale) {                  //  For scales, we need to find note-per-oct and ref-pitch
                        switch(cntpitches) {
                        case(0):
                            if(dummy < 1 || dummy > MAXSCALECNT) {
                                sprintf(errstr,"Scale division (%d) at line %d out of range (1 to %d)\n",(int)round(dummy),linecnt+1,MAXSCALECNT);
                                return(DATA_ERROR);
                            }
                            scale_division = (int)round(dummy);
                            break;
                        case(1):
                            if(dummy < 0 || dummy > MIDIMAX) {
                                sprintf(errstr,"Invalid reference pitch data (%lf) for scale, at line %d. (Range MIDI 0 - 127)\n",dummy,linecnt+1);
                                return(DATA_ERROR);
                            }
                            reference_pitch = dummy;
                            break;
                        default:
                            sprintf(errstr,"Too many vals specified for scale in file %s: need only notes-per-octave and (MIDI)ref-pitch.\n",str);
                            return(DATA_ERROR);
                        }
                    } else if(is_elacs) {           //  For elacs, we need to find "octave"-size,note-per-oct and ref-pitch
                        switch(cntpitches) {
                        case(0):
                            if(dummy < PSEUDOCTMIN || dummy > PSEUDOCTMAX) {
                                sprintf(errstr,"\"Octave\" size (%d) at line %d out of range (%d to %d)\n",(int)round(dummy),linecnt+1,PSEUDOCTMIN,PSEUDOCTMAX);
                                return(DATA_ERROR);
                            }
                            pseudoct_size = (int)round(dummy);
                            break;
                        case(1):
                            if(dummy < 1 || dummy > MAXSCALECNT) {
                                sprintf(errstr,"Scale division (%d) at line %d out of range (1 to %d)\n",(int)round(dummy),linecnt+1,MAXSCALECNT);
                                return(DATA_ERROR);
                            }
                            scale_division = (int)round(dummy);
                            if((double)pseudoct_size/(double)scale_division < .01) {
                                sprintf(errstr,"Scale steps per true octave (%.lf) too small (Minimum %.2lf)\n",(double)pseudoct_size/(double)scale_division,0.01);
                                return(DATA_ERROR);
                            }
                            break;
                        case(2):
                            if(dummy < 0 || dummy > MIDIMAX) {
                                sprintf(errstr,"Invalid reference pitch data (%lf) for scale, at line %d. (Range MIDI 0 - 127)\n",dummy,linecnt+1);
                                return(DATA_ERROR);
                            }
                            reference_pitch = dummy;
                            break;
                        default:
                            sprintf(errstr,"Too many vals for escal in file %s: need only \"oct\"-size, notes-per-\"oct\" and (MIDI)ref-pitch.\n",str);
                            return(DATA_ERROR);
                        }
                    } else {                        //  For HS or HF we need a set of MIDI values   
                        if(dummy < 0 || dummy > MIDIMAX) {
                            sprintf(errstr,"Invalid pitch data (%lf) at line %d. (Range MIDI 0 - 127)\n",dummy,linecnt+1);
                            return(DATA_ERROR);
                        }
                    }
                    cntpitches++;
                } else {
                    if(ignore_further_times)        //  If ignoring times, keep reading till we reach HF/HS data
                        continue;
                    if(cnttimes == 0.0) {
                        if(dummy < 0) {
                            sprintf(errstr,"Invalid time (%lf) at line %d in file %s.\n",dummy,linecnt+1,str);
                            return(DATA_ERROR);
                        }                           //  If 1st time val at or after file end, Ignore it and further time-values         
                        if(dummy >= dz->duration) {
                            fprintf(stdout,"WARNING: First time (%lf) at or beyond file end (%lf): will use unsegmented src.\n",dummy,dz->duration);
                            fflush(stdout);
                            ignore_further_times = 1;
                        }
                    } else {                        //  Ensure time-markers increase
                        if (dummy <= lasttime) {
                            sprintf(errstr,"Times do not advance (%lf to %lf) at from line %d to line %d\n",lasttime,dummy,linecnt,linecnt+1);
                            return(DATA_ERROR);
                        }                           //  Ignore any time markers at or after file end
                        if(dummy >= dz->duration) {
                            fprintf(stdout,"WARNING: Time (%lf) at line %d is at or beyond file end (%lf): ignoring it (and later times)\n",dummy,linecnt+1,dz->duration);
                            fflush(stdout);
                            cnttimes--;             //  Counters the increment in "cnttimes" below
                            ignore_further_times = 1;
                        }
                    }
                    lasttime = dummy;
                    cnttimes++;
                }
            }
        }
    }
    if(cnttimes == 0) {     //  Should be redundant
        sprintf(errstr,"No time-segmentation specified, in file %s.\n",str);
        return(DATA_ERROR);
    }
    if(is_hs == -1) {
        sprintf(errstr,"No data for Harmonic-Field, Harmonic-Set or Scale-type specified in file %s.\n",str);
        return(DATA_ERROR);
    }
    if(is_scale || is_elacs) {
        if(is_scale) {
            arraysize = (scale_division * MIDIOCTSPAN) + 1;
            if((dz->parray[PBINPCH] = (double *)malloc(arraysize * 4 * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Pitch-Bin data.\n");
                return(MEMORY_ERROR);
            }
            binpitch = dz->parray[PBINPCH];
            pitchpos = 0;               //  Dummy entry in position [0] - (allows pitchtop counter to tally with pitch centre conter)
            binpitch[pitchpos++] = 0.0;
            
            //  SPECIFY PITCHES IN 1ST 8VA
            
            switch(scale_division) {
            case(1)://  fall thro       //  For rational divisors of 12
            case(2)://  fall thro
            case(3)://  fall thro
            case(4)://  fall thro
            case(6)://  fall thro
            case(12):
                step = (int)round(SEMITONES_PER_OCTAVE)/scale_division;
                for(n = 0;n < scale_division;n+=step)
                    binpitch[pitchpos++] = (double)n;
                break;
            default:                    //  Otherwise
                for(n = 0;n < scale_division;n++) {
                    ratio = (double)n/(double)scale_division;
                    binpitch[pitchpos++] = SEMITONES_PER_OCTAVE * ratio;
                }
                break;
            }

            //  SPECIFY PITCHES IN ALL OTHER 8VAs
            
            done = 0;
            octstep = 0;
            for(m=1;m < MIDIOCTSPAN; m++) {     //  For every 8va in total MIDI range
                oct1pitchpos = 1;               //  Point to vals in lowest 8va
                octstep += SEMITONES_PER_OCTAVE;
                for(n=0;n<scale_division;n++) { //  Copy these, N octaves higher, into higher octaves
                    nextpitch = binpitch[oct1pitchpos++] + octstep;
                    if(nextpitch > MIDIMAX) {
                        done = 1;
                        break;
                    }
                    binpitch[pitchpos++] = nextpitch;
                }
                if(done)
                    break;
            }
            dz->bincnt = pitchpos;  //  Remember how many bins we have

        } else {    //  is_elacs

            arraysize = 0;
            trueoct_cnt = 0;
            semitone_len = 0;
            while(trueoct_cnt < MIDIOCTSPAN) {
                arraysize    += scale_division;
                semitone_len += pseudoct_size;
                trueoct_cnt = (int)floor(semitone_len/SEMITONES_PER_OCTAVE);
            }
            if((dz->parray[PBINPCH] = (double *)malloc(arraysize * 4 * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Pitch-Bin data.\n");
                return(MEMORY_ERROR);
            }
            binpitch = dz->parray[PBINPCH];
            pitchpos = 0;               //  Dummy entry in position [0] - (allows pitchtop counter to tally with pitch centre conter)
            binpitch[pitchpos++] = 0.0;
            
            //  SPECIFY PITCHES IN EACH pseudo8va Ffor all pseudo8vas until MIDIMAX is reached
            thispitch = 0.0;
            octfoot = 0.0;
            while(thispitch <= MIDIMAX) {
                for(n = 0;n < scale_division;n++) {
                    ratio = (double)n/(double)scale_division;
                    if((thispitch = (pseudoct_size * ratio) + octfoot) > MIDIMAX) {
                        break;
                    }
                    binpitch[pitchpos++] = thispitch;
                }
                octfoot += pseudoct_size;
            }
            dz->bincnt = pitchpos;  //  Remember how many bins we have
        }
        if(is_elacs || !flteq(fmod(reference_pitch,SEMITONES_PER_OCTAVE),0.0)) {    //  Either, is_elacs, so pitches not guaranteed to include reference_pitch
                                                                                    //  OR, Reference-pitch is not at multiple of 12
            //  Find pitch closest to reference pitch, and find interval distance to ref-pitch
            
            mindiff = HUGE;
            mindiffat = 0;
            for(n = 1; n < dz->bincnt; n++) {                       //  binpitch[0] is a dummy
                thisdiff = fabs(reference_pitch - binpitch[n]);
                if(thisdiff < mindiff) {
                    mindiff = thisdiff;
                    mindiffat = n;
                }
            }

            //  Now transpose set so it includes ref-pitch

            mindiff = reference_pitch - binpitch[mindiffat];
            if(mindiff > 0.0) {
                for(n = 1; n < dz->bincnt; n++) {
                    nupitch = binpitch[n] + mindiff;
                    if(nupitch > 127)
                        break;
                    binpitch[n] = nupitch;
                }
                dz->bincnt = n;                                 //  May be smaller than orig, if binpitch pushed eyond 127
            } else if(mindiff < 0.0) {
                nubincnt = 1;
                for(n = 1; n < dz->bincnt; n++) {
                    nupitch = binpitch[n] + mindiff;
                    if(nupitch >= 0.0)                          //  binpitch now below zero are ignored, so the new bincnt (m) does not increment
                        binpitch[nubincnt++] = nupitch;         //  Otherwise new binpitch are written over orig binpitch (possibly lower in array - but never higher!!)
                }
                dz->bincnt = nubincnt;
            }
        }

        if((dz->iparray[PCNTBIN] = (int *)malloc(dz->bincnt * 4 * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Pitch-Counting Data.\n");
            return(MEMORY_ERROR);
        }
        if((dz->parray[PBINTOP] = (double *)malloc(dz->bincnt * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Pitch-Top Data.\n");
            return(MEMORY_ERROR);
        }
        if((dz->parray[PBINAMP] = (double *)malloc(dz->bincnt * 4 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Pitch-Amp Data.\n");
            return(MEMORY_ERROR);
        }
    } else {

        //  If not scale: create array to store pitchdata initially read from file

        if((dz->parray[PREPICH] = (double *)malloc((cntpitches + 1) * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Initial Filter Pitch Data.\n");
            return(MEMORY_ERROR);
        }
        prepitch = dz->parray[PREPICH];
    }

    arraysize = cnttimes;   
    arraysize+=2;               //  Need (possibly) to insert time at 0.0, and at end;
    if((dz->lparray[FTIMES] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store Filter Times Data.(2)\n");
        return(MEMORY_ERROR);
    }
    ftime = dz->lparray[FTIMES];
    timepos = 0;
    ftime[timepos++] = 0;

    rewind(fp);
    is_pitches = 0;
    linecnt = 0;
    cntpitches = 0;
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        if(*p == '#') {
            if(is_scale)                                //  If HF/HS data is a scale, we already have the pitch data: so quit
                break;
            is_pitches = 1;
            continue;
        } else {
            while(get_float_from_within_string(&p,&dummy)) {
                if(is_pitches)
                    prepitch[cntpitches++] = dummy;
                else {
                    if(ignore_further_times)
                        continue;
                    if(dummy >= dz->duration) {
                        ignore_further_times = 1;       //  Defaults to the zero val already in array
                        continue;
                    }
                    if(timepos == 1 && flteq(dummy,0.0))
                        continue;                       //  Already got time zero in times array, but may be other times to store
                    ftime[timepos++] = (int)round(dummy/dz->frametime);
                }
            }
        }
    }
    if(ftime[timepos-1] > dz->wlength - MINWINDOWS)     //  If last time very close to endoffile
        ftime[timepos-1] = dz->wlength + 1;             //  move it to AFTER end of file
    else {                                              //  If not, add another point AFTER end of file
        ftime[timepos] = dz->wlength + 1;
        timepos++;
    }
    dz->timeblokcnt = timepos - 1;                      //  If we have 5 times: 0 X Y Z END we have 4 bloks: 0-X: X-Y: Y-Z: Z-END
    fclose(fp);
    if(!is_scale) {
        dz->itemcnt = cntpitches;
        dz->itemcnt = sort_pitches(dz);                             //  Some pitches may be duplicated: they also need to be in ascending order
        if(is_hs)
            dz->bincnt = dz->itemcnt + 1;                           //  Space for all entered pitches + dummy value at [0]
        else {
            for(n = 0;n < dz->itemcnt; n++) {                       //  For HF, duplicate pitches over all 8vas
                while(prepitch[n] > 0.0) 
                    prepitch[n] -= SEMITONES_PER_OCTAVE;            //  1st transpose all pitches into lowest octave
                prepitch[n] += SEMITONES_PER_OCTAVE;
            }
            dz->itemcnt = sort_pitches(dz);                         //  After transposition, some pitches may be duplicated
            dz->bincnt = (dz->itemcnt * MIDIOCTSPAN) + 1;           //  pitches are to be duplicated in all 8vas + we need the dummy val at [0]
        }
        if((dz->parray[PBINPCH] = (double *)malloc(dz->bincnt * 4 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Pitch-Bin-Top data.\n");
            return(MEMORY_ERROR);
        }
        binpitch = dz->parray[PBINPCH];

        if(is_hs) {                                                 //  For HS, merely copy pitches into final array, leaving dummy value at [0]
            memcpy((char *)(dz->parray[PBINPCH] + 1),(char *)dz->parray[PREPICH],(dz->bincnt - 1) * sizeof(double));
            dz->parray[PBINPCH][0] = 0.0;
        } else {                                                    //  For HF, copy pitches into all 8vas in MIDI range
            done = 0;
            pitchpos = 0;
            binpitch[pitchpos++] = 0.0;                             //  Leave dummy value at zero
            octstep = 0;
            for(m=0;m < MIDIOCTSPAN; m++) {                         //  For every 8va in total MIDI range
                for(n=0;n<dz->itemcnt;n++) {                        //  Copy pitches in lowest octave into higher octaves
                    nextpitch = prepitch[n] + octstep;
                    if(nextpitch > MIDIMAX) {
                        done = 1;
                        break;
                    }
                    binpitch[pitchpos++] = nextpitch;
                }
                octstep += SEMITONES_PER_OCTAVE;
                if(done)
                    break;
            }
            dz->bincnt = pitchpos;  //  Remember how many bins we have
        }
        if((dz->parray[PBINTOP] = (double *)malloc(dz->bincnt * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Pitch-Bin-Top data.\n");
            return(MEMORY_ERROR);
        }
        if((dz->parray[PBINAMP] = (double *)malloc(dz->bincnt * 4 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Pitch-Bin-Amp Data.\n");
            return(MEMORY_ERROR);
        }
        if((dz->iparray[PCNTBIN] = (int *)malloc(dz->bincnt * 4 * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Pitch-Counting Data.\n");
            return(MEMORY_ERROR);
        }
    }

    //  Establish the pitch boundaries of the pitch bins (Bins are NO MORE than a semitone in width, and may be less if steps between pitches less than a semitone)

    bintop = dz->parray[PBINTOP];
    for(m=1;m< 4;m++) {
        offset = dz->bincnt * m;                    //  4 arrays, 1 for each formant
        for(n=0,k=offset;n<dz->bincnt;n++,k++)
            binpitch[k] = binpitch[n];
    }
    for(n=1;n < dz->bincnt-1;n++) {                                 //  For HS gaps width of pitch-search bins assumed to be a semitone 
        bintop[n] = min((binpitch[n+1] - binpitch[n])/2.0,0.5);     //  (or less, of gap between bins is less than a semitone)  
        bintop[n] += binpitch[n];
    }
    bintop[n] = binpitch[n] + (binpitch[n] - bintop[n-1]);          //  Upper Bound of topmost pitchbin assumed to be same distance from centre as its lower bound
    bintop[n]  = min(127.0,bintop[n]);                              //  Unless that takes it above 127

    bintop[0] = binpitch[1] - (bintop[1] - binpitch[1]);            //  Lower bound (pitchtop[0]) of lowest bin (binpitch[1]) same distance from centre as upper bound
    bintop[0] = max(0.0,bintop[0]);                                 //  Unless that takes it below zero

    memset((char *)dz->iparray[PCNTBIN],0,dz->bincnt * 4 *sizeof(int)); //  Preset arrays where values are ADDED into them
    memset((double *)dz->parray[PBINAMP],0,dz->bincnt * 4 * sizeof(double));

    if((dz->parray[FPITCHES] = (double *)malloc(dz->bincnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store pitches actually used by filter.\n");
        return(MEMORY_ERROR);
    }
    return FINISHED;
}

/**************************** HANDLE_THE_PQUANTISE_SPECIAL_DATA ****************************/

int handle_the_pquantise_special_data(char *str,dataptr dz)
{
    int exit_status, is_hs = -1, is_thf = 0, is_pitches = 0, is_scale = 0, is_elacs = 0, scale_division = 12, entrycnt = 0, pitchsort = 0;
    int cntpitches = 0, linecnt, pitchpos, oct1pitchpos, step, n, m, done, mindiffat, nubincnt;
    double dummy = 0.0, reference_pitch = 60.0, ratio, octstep, nextpitch, mindiff, thisdiff, nupitch, pseudoct_size = 12.0;
    double trueoct_cnt, semitone_len, thispitch, octfoot, lasttime = 0.0;
    FILE *fp;
    double *pset = NULL, *prepitch;
    int arraysize;
    char temp[800], *p;

    dz->timedhf = 0;

    if((dz->mode == F_PCHRAND || dz->mode == F_SINUS) && (!strcmp(str,"0") || !strcmp(str,"0.0"))) {
        dz->quantcnt = 0;
        return FINISHED;
    }
    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read data.\n",str);
        return(DATA_ERROR);
    }
    linecnt = 0;
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        if(*p == '#') {                 //  Look for #HF, #HS, or #SCALES marker
            strip_end_space(p);
            if(strcmp(p,"#HS") == 0) {
                is_hs = 1;
                is_pitches = 1;
            } else if(strcmp(p,"#HF") == 0) {
                is_hs = 0;
                is_pitches = 1;
            } else if(strcmp(p,"#SCALE") == 0) {
                is_hs = 0;
                is_scale = 1;
                is_pitches = 1;
            } else if(strcmp(p,"#ELACS") == 0) {
                is_hs = 0;
                is_elacs = 1;
                is_pitches = 1;
            } else if(strcmp(p,"#THF") == 0) {
                is_hs = 0;
                is_thf = 1;
                dz->timedhf = 1;
                is_pitches = 1;
            }
            else {
                sprintf(errstr,"Invalid Field, Set, or Scale marker (%s) on line %d in file %s\n",p,linecnt+1,str);
                return(DATA_ERROR);
            }
        } else {
            while(get_float_from_within_string(&p,&dummy)) {
                if(is_scale) {                  //  For scales, we need to find note-per-oct and ref-pitch
                    switch(cntpitches) {
                    case(0):
                        if(dummy < 1 || dummy > MAXSCALECNT) {
                            sprintf(errstr,"Scale division (%d) at line %d out of range (1 to %d)\n",(int)round(dummy),linecnt+1,MAXSCALECNT);
                            return(DATA_ERROR);
                        }
                        scale_division = (int)round(dummy);
                        break;
                    case(1):
                        if(dummy < 0 || dummy > MIDIMAX) {
                            sprintf(errstr,"Invalid reference pitch data (%lf) for scale, at line %d. (Range MIDI 0 - 127)\n",dummy,linecnt+1);
                            return(DATA_ERROR);
                        }
                        reference_pitch = dummy;
                        break;
                    default:
                        sprintf(errstr,"Too many vals specified for scale in file %s: need only notes-per-octave and (MIDI)ref-pitch.\n",str);
                        return(DATA_ERROR);
                    }
                } else if(is_elacs) {           //  For elacs, we need to find "octave"-size,note-per-oct and ref-pitch
                    switch(cntpitches) {
                    case(0):
                        if(dummy < PSEUDOCTMIN || dummy > PSEUDOCTMAX) {
                            sprintf(errstr,"\"Octave\" size (%d) at line %d out of range (%d to %d)\n",(int)round(dummy),linecnt+1,PSEUDOCTMIN,PSEUDOCTMAX);
                            return(DATA_ERROR);
                        }
                        pseudoct_size = (int)round(dummy);
                        break;
                    case(1):
                        if(dummy < 1 || dummy > MAXSCALECNT) {
                            sprintf(errstr,"Scale division (%d) at line %d out of range (1 to %d)\n",(int)round(dummy),linecnt+1,MAXSCALECNT);
                            return(DATA_ERROR);
                        }
                        scale_division = (int)round(dummy);
                        if((double)pseudoct_size/(double)scale_division < .01) {
                            sprintf(errstr,"Scale steps per true octave (%.lf) too small (Minimum %.2lf)\n",(double)pseudoct_size/(double)scale_division,0.01);
                            return(DATA_ERROR);
                        }
                        break;
                    case(2):
                        if(dummy < 0 || dummy > MIDIMAX) {
                            sprintf(errstr,"Invalid reference pitch data (%lf) for scale, at line %d. (Range MIDI 0 - 127)\n",dummy,linecnt+1);
                            return(DATA_ERROR);
                        }
                        reference_pitch = dummy;
                        break;
                    default:
                        sprintf(errstr,"Too many vals for escal in file %s: need only \"oct\"-size, notes-per-\"oct\" and (MIDI)ref-pitch.\n",str);
                        return(DATA_ERROR);
                    }
                } else if(is_thf) {         //  For thf, we need to find several hfs and their times
                    if(entrycnt == 0)
                        pitchsort = cntpitches;
                    else
                        pitchsort = cntpitches % entrycnt;
                    switch(pitchsort) {
                    case(0):
                        if(linecnt == 1) {
                            if(dummy != 0.0) {
                                sprintf(errstr,"First time must be zero for a time-changing HF, in file %s\n",str);
                                return(DATA_ERROR);
                            }
                        } else if(dummy <= lasttime) {
                            sprintf(errstr,"Times do not advance (%lf : %lf) from line %d to line %d in file %s\n",lasttime,dummy,linecnt-1,linecnt,str);
                            return(DATA_ERROR);
                        }
                        lasttime = dummy;
                        break;
                    default:
                        if(dummy < 0.0 || dummy > 127) {
                            sprintf(errstr,"Pitch value (%lf) out of range (0-127) in %d in file %s\n",dummy,linecnt,str);
                            return(DATA_ERROR);
                        }
                        break;
                    }
                } else {                        //  For HS or HF we need a set of MIDI values   
                    if(dummy < 0 || dummy > MIDIMAX) {
                        sprintf(errstr,"Invalid pitch data (%lf) at line %d. (Range MIDI 0 - 127)\n",dummy,linecnt+1);
                        return(DATA_ERROR);
                    }
                }
                cntpitches++;
            }
        }
        if(linecnt > 0 && is_thf) {
            if(linecnt == 1)
                entrycnt = cntpitches;

            else if(cntpitches % entrycnt != 0) {
                sprintf(errstr,"Lines %d & %d are not of same length in file %s\n",linecnt,linecnt+1,str);
                return(DATA_ERROR);
            }
        }
        linecnt++;
    }

    if(is_hs == -1) {
        sprintf(errstr,"No data for Harmonic-Field, Harmonic-Set or Scale-type specified in file %s.\n",str);
        return(DATA_ERROR);
    }
    if(is_scale || is_elacs) {
        if(is_scale) {
            arraysize = (scale_division * MIDIOCTSPAN) + 1;
            if((dz->parray[QUANTPITCH] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Pitch-Bin data.\n");
                return(MEMORY_ERROR);
            }
            pset = dz->parray[QUANTPITCH];
            pitchpos = 0;
            
            //  SPECIFY PITCHES IN 1ST 8VA
            
            switch(scale_division) {
            case(1)://  fall thro       //  For rational divisors of 12
            case(2)://  fall thro
            case(3)://  fall thro
            case(4)://  fall thro
            case(6)://  fall thro
            case(12):
                step = (int)round(SEMITONES_PER_OCTAVE)/scale_division;
                for(n = 0;n < scale_division;n+=step)
                    pset[pitchpos++] = (double)n;
                break;
            default:                    //  Otherwise
                for(n = 0;n < scale_division;n++) {
                    ratio = (double)n/(double)scale_division;
                    pset[pitchpos++] = SEMITONES_PER_OCTAVE * ratio;
                }
                break;
            }

            //  SPECIFY PITCHES IN ALL OTHER 8VAs
            
            done = 0;
            octstep = 0;
            for(m=1;m < MIDIOCTSPAN; m++) {     //  For every 8va in total MIDI range
                oct1pitchpos = 1;               //  Point to vals in lowest 8va
                octstep += SEMITONES_PER_OCTAVE;
                for(n=0;n<scale_division;n++) { //  Copy these, N octaves higher, into higher octaves
                    nextpitch = pset[oct1pitchpos++] + octstep;
                    if(nextpitch > MIDIMAX) {
                        done = 1;
                        break;
                    }
                    pset[pitchpos++] = nextpitch;
                }
                if(done)
                    break;
            }
            dz->bincnt = pitchpos;  //  Remember how many bins we have

        } else {    //  is_elacs

            arraysize = 0;
            trueoct_cnt = 0;
            semitone_len = 0;
            while(trueoct_cnt < MIDIOCTSPAN) {
                arraysize    += scale_division;
                semitone_len += pseudoct_size;
                trueoct_cnt = (int)floor(semitone_len/SEMITONES_PER_OCTAVE);
            }
            if((dz->parray[QUANTPITCH] = (double *)malloc(arraysize * 4 * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to store Filter-Pitch-Bin data.\n");
                return(MEMORY_ERROR);
            }
            pset = dz->parray[QUANTPITCH];
            pitchpos = 0;               //  Dummy entry in position [0] - (allows pitchtop counter to tally with pitch centre conter)
            
            //  SPECIFY PITCHES IN EACH pseudo8va for all pseudo8vas until MIDIMAX is reached
            thispitch = 0.0;
            octfoot = 0.0;
            while(thispitch <= MIDIMAX) {
                for(n = 0;n < scale_division;n++) {
                    ratio = (double)n/(double)scale_division;
                    if((thispitch = (pseudoct_size * ratio) + octfoot) > MIDIMAX) {
                        break;
                    }
                    pset[pitchpos++] = thispitch;
                }
                octfoot += pseudoct_size;
            }
            dz->bincnt = pitchpos;  //  Remember how many bins we have
        }
        if(is_elacs || !flteq(fmod(reference_pitch,SEMITONES_PER_OCTAVE),0.0)) {    //  Either, is_elacs, so pitches not guaranteed to include reference_pitch
                                                                                    //  OR, Reference-pitch is not at multiple of 12
            //  Find pitch closest to reference pitch, and find interval distance to ref-pitch
            
            mindiff = HUGE;
            mindiffat = 0;
            for(n = 1; n < dz->bincnt; n++) {                       //  pset[0] is a dummy
                thisdiff = fabs(reference_pitch - pset[n]);
                if(thisdiff < mindiff) {
                    mindiff = thisdiff;
                    mindiffat = n;
                }
            }

            //  Now transpose set so it includes ref-pitch

            mindiff = reference_pitch - pset[mindiffat];
            if(mindiff > 0.0) {
                for(n = 1; n < dz->bincnt; n++) {
                    nupitch = pset[n] + mindiff;
                    if(nupitch > 127)
                        break;
                    pset[n] = nupitch;
                }
                dz->bincnt = n;                                 //  May be smaller than orig, if pset pushed eyond 127
            } else if(mindiff < 0.0) {
                nubincnt = 1;
                for(n = 1; n < dz->bincnt; n++) {
                    nupitch = pset[n] + mindiff;
                    if(nupitch >= 0.0)                          //  pset now below zero are ignored, so the new bincnt (m) does not increment
                        pset[nubincnt++] = nupitch;         //  Otherwise new pset are written over orig pset (possibly lower in array - but never higher!!)
                }
                dz->bincnt = nubincnt;
            }
        }

    } else {

        //  If not scale: create array to store pitchdata initially read from file

        if((dz->parray[PREPICH] = (double *)malloc((cntpitches + 1) * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Initial Filter Pitch Data.\n");
            return(MEMORY_ERROR);
        }
        prepitch = dz->parray[PREPICH];
        rewind(fp);
        linecnt = 0;
        cntpitches = 0;
        while(fgets(temp,200,fp)!=NULL) {
            p = temp;
            while(isspace(*p))
                p++;
            if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
                continue;
            if(*p == '#')
                continue;
            while(get_float_from_within_string(&p,&dummy))
                    prepitch[cntpitches++] = dummy;
            linecnt++;
        }
        fclose(fp);
        if(is_thf) {                                                    //  Sort each line of pitches & transpose into lowest octave

            dz->quantcnt = entrycnt;                                    //  quantcnt isthe number of quantising pitches per line
            dz->itemcnt  = linecnt;                                     //  itemcnt is the number of different quantisation sets    
            sort_pitches_for_tvary_hf(dz->quantcnt,dz->itemcnt,dz);
            dz->bincnt  = (((entrycnt-1) * MIDIOCTSPAN) * dz->itemcnt) + dz->itemcnt;
                    //       pitches        dupl_vals       for         extra entry
                    //       per            in all          every       for time
                    //       line           8vas            line        in each line
        } else {
            dz->itemcnt = cntpitches;
            dz->itemcnt = sort_pitches(dz);                             //  Some pitches may be duplicated: they also need to be in ascending order
            if(is_hs) {
                dz->quantcnt = dz->itemcnt;                             //  Number of pitches to quantise to is exactly those entered
                dz->bincnt = dz->itemcnt + 1;                           //  Space for all entered pitches + dummy value at [0]
            } else {
                for(n = 0;n < dz->itemcnt; n++) {                       //  For HF, duplicate pitches over all 8vas
                    while(prepitch[n] >= 0.0) 
                        prepitch[n] -= SEMITONES_PER_OCTAVE;            //  1st transpose all pitches into lowest octave
                    prepitch[n] += SEMITONES_PER_OCTAVE;
                }
                dz->itemcnt = sort_pitches(dz);                         //  After transposition, some pitches may be duplicated
                dz->bincnt = (dz->itemcnt * MIDIOCTSPAN) + 1;           //  pitches are to be duplicated in all 8vas + we need the dummy val at [0]
            }
        }
        if((dz->parray[QUANTPITCH] = (double *)malloc(dz->bincnt * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Quantisation pitch field.\n");
            return(MEMORY_ERROR);
        }
        pset = dz->parray[QUANTPITCH];

        if(is_hs)                                                       //  For HS, merely copy pitches into final array
            memcpy((char *)dz->parray[QUANTPITCH],(char *)dz->parray[PREPICH],dz->bincnt * sizeof(double));
        else if(is_thf) {                                               //  For THF, copy pitches in each line-set into all 8vas in MIDI range
            if((exit_status = octaviate_and_store_timed_hf_data(dz))<0)
                return exit_status;
        } else {                                                    //  For HF, copy pitches into all 8vas in MIDI range
            done = 0;
            pitchpos = 0;
            octstep = 0;
            for(m=0;m < MIDIOCTSPAN; m++) {                         //  For every 8va in total MIDI range
                for(n=0;n<dz->itemcnt;n++) {                        //  Copy pitches in lowest octave into higher octaves
                    nextpitch = prepitch[n] + octstep;
                    if(nextpitch > MIDIMAX) {
                        done = 1;
                        break;
                    }
                    pset[pitchpos++] = nextpitch;
                }
                octstep += SEMITONES_PER_OCTAVE;
                if(done)
                    break;
            }
            dz->quantcnt = pitchpos;    //  Remember how many bins we have
        }
    }
    return FINISHED;
}

/****************************************** SORT_PITCHES ***************************/

int sort_pitches(dataptr dz)
{
    int n, m;
    double temp, *prepitch = dz->parray[PREPICH];
    int len = dz->itemcnt;
    for(n = 0; n < len - 1; n++) {          //  Sort into ascending order
        for(m = n+1; m < len;m++) {
            if(prepitch[m] < prepitch[n]) {
                temp = prepitch[m];
                prepitch[m] = prepitch[n];
                prepitch[n] = temp;
            }
        }
    }
    for(n = 0; n < len-1; n++) {            //  Eliminate duplicates
        if(flteq(prepitch[n+1],prepitch[n])) {
            for(m = n+1;m < len;m++)
                prepitch[m-1] = prepitch[m];
            len--;
        }
    }
    return len;
}

/****************************************** SORT_PITCHES_FOR_TVARY_HF ***************************/

int sort_pitches_for_tvary_hf(int entrycnt,int linecnt,dataptr dz)
{

    int line, line_cnt, later_line_cnt, abs_cnt, later_abs_cnt;
    double temp, *prepitch = dz->parray[PREPICH];

    for(line = 0; line < linecnt; line++) {                                                                 //  For each line
        for(line_cnt = 1, abs_cnt = line * entrycnt + 1; line_cnt < entrycnt - 1; line_cnt++,abs_cnt++) {   //  Ignoring the first (time) entry
            for(later_line_cnt = line_cnt+1,later_abs_cnt = abs_cnt+1; later_line_cnt < entrycnt;later_line_cnt++,later_abs_cnt++) {
                if(prepitch[later_abs_cnt] < prepitch[abs_cnt]) {                                           //  Sort pitches into ascending order
                    temp = prepitch[later_abs_cnt];
                    prepitch[later_abs_cnt] = prepitch[abs_cnt];
                    prepitch[abs_cnt] = temp;
                }
            }
        }
        for(line_cnt = 1, abs_cnt = line * entrycnt + 1; line_cnt < entrycnt; line_cnt++,abs_cnt++) {       //  transpose all pitches into lowest 8va
            while(prepitch[abs_cnt] >= 0.0) 
                prepitch[abs_cnt] -= SEMITONES_PER_OCTAVE;
            prepitch[abs_cnt] += SEMITONES_PER_OCTAVE;
        }
    }
    return  FINISHED;
}

/****************************************** FORMANTS_MAKEFILT ***************************/

int formants_makefilt(int inner_lpcnt,int *times_index,dataptr dz)
{
    int exit_status, n, cc, vc, k, j, spstt, spend, ccstt, ccend, vcstt, peakloc, newcc, peakno, get_fundamental = 0;
    float frqstt, frqend, thisamp, newfrq = 0.0;
    double thispch, maxamp;
    double *bintop = dz->parray[PBINTOP], *binamp = dz->parray[PBINAMP];
    int *bincnt = dz->iparray[PCNTBIN], *peakat = dz->iparray[PEAKPOS];
    int *ftimes = dz->lparray[FTIMES];
    peakat += inner_lpcnt * PKBLOK;

    if(*times_index < dz->timeblokcnt) {            //  If their is a list of times, and we haven't reached end of it
        if(inner_lpcnt >= ftimes[*times_index]) {   //  If we've got to time-boundary for next filter
            if((exit_status = build_filter(*times_index,dz)) < 0)
               return(exit_status);                 //  build the filter so far.
            for(n = 0; n < dz->bincnt; n++) {       //  Then zero the counting and amplitude sum bins for the next block of windows
                bincnt[n] = 0;
                binamp[n] = 0.0;
            }
            (*times_index)++;
        }
    }
    for(n = 0; n < PKBLOK-1; n+=2) {                //  Going through the trofs in pairs
        peakno = n/2;
        if(dz->fundamental && n == 0)
            get_fundamental = 1;
        spstt  = peakat[n];                         //  Get specenvamp locations of 2 adjacent trofs.
        spend  = peakat[n+2];
        frqstt = dz->specenvtop[spstt];             //  Find freq of top frq-edge of lower trof
        ccstt  = (int)floor(frqstt/dz->chwidth);    //  Find the flbufpr channels associated with these frqs
        frqend = dz->specenvtop[spend-1];           //  Find freq of bottom frq-edge of upper trof
        ccend  = (int)ceil(frqend/dz->chwidth);
        maxamp = -HUGE;                             //  Find the flbuptr location of maximum amplitude.
        vcstt  = ccstt * 2;
        peakloc = ccstt; 
        for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
            if(dz->flbufptr[0][AMPP] > maxamp) {
                maxamp  = dz->flbufptr[0][AMPP];
                peakloc = cc;
            }
        }
        if(get_fundamental) {                       //  If at peak1 and we want to force getting the fundamental (e.g. if adjacent harmonic louder)
            if((exit_status = locate_channel_of_fundamental(inner_lpcnt,&newfrq,&newcc,dz))<0)
                return exit_status;
            if(newfrq > 0.0)                        //  i.e. if we really have pitch data here
                peakloc = newcc;

            get_fundamental = 0;                    //  Only (try to) get fundamental when in first formant
        }
        vc = peakloc;
        thisamp = dz->flbufptr[0][AMPP];
        thispch = unchecked_hztomidi((double)dz->flbufptr[0][FREQ]);
        if(thispch > 0 && thispch <= 127) {
            for(k = 1,j= (peakno*dz->bincnt)+1;k < dz->bincnt;k++,j++) {    //  Find which bin (if any) the pitch falls in
                if(thispch < bintop[k] && thispch > bintop[k-1]) {
                    bincnt[j]++;                        //  Count occurences of this pitch
                    binamp[j] += thisamp;               //  Sum amps of all occurences of this pitch
                    break;
                }
            }
        }
    }
    return FINISHED;
}

/****************************************** BUILD_FILTER ***************************/

int build_filter(int times_index,dataptr dz)
{
    int n, j, k, z, pch, amp, filtercnt, this_time, next_time, target, get_below = 0, pitchcnt, newpitch, filtlinelen;
    int maxcnt;
    double maxfiltamp = 0.0, maxamp = 0.0, starttime, endtime, lasttime = 0.0, tempval, minpitch;
    double *binpitch = dz->parray[PBINPCH], *binamp = dz->parray[PBINAMP];
    double *filtpch = dz->parray[LOCALPCH], *filtamp = dz->parray[LOCALAMP];
    double *pchstore = dz->parray[FILTPICH], *ampstore = dz->parray[FILTAMP];
    double *fpitches = dz->parray[FPITCHES];
    float *filtline, *lastfiltline;
    double lastpch;
    int *countbin = dz->iparray[PCNTBIN];
    int maxat = 0, maxats[MAXFILTVALS];
    int *ftimes = dz->lparray[FTIMES];
    int blokcnt = dz->iparam[FPKCNT] * 4;
    int storepos = (times_index - 1) * blokcnt, storecnt;
    int filtbas, fmntno, fcnt, do_intermediate_line;


    memset((char *)filtpch,0,dz->iparam[FPKCNT] * 4 * sizeof(int));
    memset((char *)filtamp,0,dz->iparam[FPKCNT] * 4 * sizeof(double));

    if(dz->param[FBELOW])
        get_below = 1;

    //  FIND MOST PROMINENT PITCHES IN THIS BLOK
    for(fmntno = 0;fmntno < 4;fmntno++) {
        minpitch = 2000;
        filtbas = (fmntno * dz->iparam[FPKCNT]);
        memset((char *)maxats,0,MAXFILTVALS * sizeof(int));
        for(fcnt = 0,filtercnt = filtbas; fcnt < dz->iparam[FPKCNT]; fcnt++,filtercnt++) {
            maxcnt = -1;
            maxat = 0;
            for(j = 1,k = (fmntno*dz->bincnt)+1;j< dz->bincnt;j++,k++) {
                if(countbin[k] > maxcnt) {
                    maxcnt = countbin[k];                       //  Find pitch that occurs most frequently
                    maxamp = binamp[k];
                    maxat = k;
                } else if(countbin[k] == maxcnt) {
                    if(binamp[k] > maxamp) {                    //  If more than 1 pitch is equally frequent
                        maxamp = binamp[k];                     //  Take the one with the greatest amplitude (summed over all occurences)
                        maxat = k;
                    }
                }
            }
            maxats[fcnt]  = maxat;
            filtpch[filtercnt] = binpitch[maxat];               //  Set pitch found as a filter pitch
            filtamp[filtercnt] = binamp[maxat];
            countbin[maxat] = 0;                                //  Eliminate this pitch from next search

            minpitch = min(filtpch[filtercnt],minpitch);

        //  Freqs Below a given pitch are required, search for

            if(get_below && (minpitch > dz->param[FBELOW])) {

                while(minpitch > dz->param[FBELOW]) {

                    target = min(fcnt,dz->iparam[FPKCNT] - 1);
                    target += filtbas;
                    maxcnt = -1;
                    maxat = 0;
                    for(j = 1, k = (fmntno * dz->bincnt)+1;j < dz->bincnt;j++,k++) {
                        if(countbin[k] > maxcnt) {
                            maxcnt = countbin[k];                   //  Find pitch that occurs most frequently
                            maxamp = binamp[k];
                            maxat = k;
                        } else if(countbin[k] == maxcnt) {
                            if(binamp[k] > maxamp) {                //  If more than 1 pitch is equally frequent
                                maxamp = binamp[k];                 //  Take the one with the greatest amplitude (summed over all occurences)
                                maxat = k;
                            }
                        }
                    }
                    if(maxcnt <= 0)                                 //  IF there are NO suitable low frqs, exit 
                        break;
                    if(binpitch[maxat] < minpitch) {
                        filtpch[target] = binpitch[maxat];          //  Set pitch found as last filter pitch
                        filtamp[target] = binamp[maxat];
                        minpitch = binpitch[maxat];                 //  will; break from loop
                    }
                    countbin[maxat] = 0;                            //  Eliminate this pitch from next search
                }
            }
            if(maxcnt > 0)                                      //  Found minimum, drop searching in other formants
                get_below = 0;
        }
    }

    //  SORT INTO ASCENDING PITCH ORDER AND SILENCE DUPLICATES

    for(n = 0; n < blokcnt - 1; n++) {
        for(k = n+1; k < blokcnt; k++) {
            if(filtpch[n] > filtpch[k]) {
                tempval    = filtpch[n];
                filtpch[n] = filtpch[k];
                filtpch[k] = tempval;
                tempval    = filtamp[n];
                filtamp[n] = filtamp[k];
                filtamp[k] = tempval;
            } else if(flteq(filtpch[n],filtpch[k])) {
                filtamp[k] = 0.0;
            }
        }
    }

    //  IF AMPS TO BE RETAINED, FIND MAX, IN ORDER TO NORMALISE         

    for(filtercnt = 0; filtercnt < blokcnt; filtercnt++) {
        if(dz->vflag[KEEPAMP])                  
            maxfiltamp = max(maxfiltamp,filtamp[filtercnt]);
        else if(dz->vflag[KEEPINV]) {                       //  If amps to be inverted, invert them
            if(!flteq(filtamp[filtercnt],0.0))
                filtamp[filtercnt] = 1.0/filtamp[filtercnt];
            maxfiltamp = max(maxfiltamp,filtamp[filtercnt]);
        } else                                              //  Otherwise, set all amps to 1
            filtamp[filtercnt]  = 1.0;
    }
    if(dz->vflag[KEEPAMP] || dz->vflag[KEEPINV]) {          //  If amps or invamps retained, normalise
        for(filtercnt = 0; filtercnt < blokcnt; filtercnt++)
            filtamp[filtercnt]  /= maxfiltamp;
    }

    //  STORE THE FILTER DATA

    for(filtercnt = 0, storecnt = storepos; filtercnt < blokcnt; filtercnt++,storecnt++) {
        pchstore[storecnt] = filtpch[filtercnt];
        ampstore[storecnt] = filtamp[filtercnt];
    }


    //  ONCE ALL FILTER DATA GENERATED:

    if(times_index >= dz->timeblokcnt) {

        //  FIND 1st DISTINCT PITCH USED
        pitchcnt = 0;
        for(n = 0; n < storecnt; n++) {
            if(ampstore[n] > 0.0) {
                fpitches[0] = pchstore[n];
                pitchcnt++;
                break;
            }
        }
        if(pitchcnt == 0) {
            sprintf(errstr,"NO VALID PITCHES FOUND IN FILTER DATA.\n");
            return DATA_ERROR;
        }

        //      FIND ALL DISTINCT PITCHES USED

        while(n < storecnt) {
            if(ampstore[n] > 0.0) {
                newpitch = 1;
                for(k = 0; k < pitchcnt;k++) {
                    if(flteq(pchstore[n],pchstore[k])) {
                        newpitch = 0;
                        break;
                    }
                }
                if(newpitch)
                    fpitches[pitchcnt++] = pchstore[n];
            }
            n++;
        }
        //  SORT DISTINCT PITCHES INTO ASCENDING ORDER && ELIMINATE DUPLICATES

        for(n = 0; n < pitchcnt - 1; n++) {
            for(k = n+1; k < pitchcnt; k++) {
                if(fpitches[n] > fpitches[k]) {
                    tempval     = fpitches[n];
                    fpitches[n] = fpitches[k];
                    fpitches[k] = tempval;
                } else if(flteq(fpitches[n],fpitches[k])) {
                    if(k < pitchcnt - 1) {
                        for(j = k+1;j < pitchcnt;j++)
                            fpitches[j-1] = fpitches[j];
                    }
                    pitchcnt--;
                    k--;
                }
            }
        }
/* TEST *
for(z = 0;z < pitchcnt;z++)
fprintf(stderr,"%.1lf  ",fpitches[z]);
fprintf(stderr,"\n");
* TEST */

        //  ELIMINATE ANY PITCH ZEROS FROM PITCHES TO BE USED IN FILTER

        for(n = 0; n < pitchcnt; n++) {
            if(fpitches[n] <= 0.0) {
                if(n < pitchcnt - 1) {
                    for(k = n+1;k < pitchcnt;k++)
                        fpitches[k-1] = fpitches[k];
                }
                pitchcnt--;
            }
        }
        filtlinelen = (pitchcnt * 2)+1;
        if((dz->fptr[FILTLINE] = (float *)malloc(filtlinelen * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store varibank filter line-entry  data.\n");
            return(MEMORY_ERROR);
        }
        if((dz->fptr[LASTFLINE] = (float *)malloc(filtlinelen * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store varibank filter line-entry  data.\n");
            return(MEMORY_ERROR);
        }
        filtline = dz->fptr[FILTLINE];
        lastfiltline = dz->fptr[LASTFLINE];

        //  OUTPUT THE FILTER
        
        for(this_time = 0,next_time = 1; this_time < dz->timeblokcnt; this_time++,next_time++) {    
            starttime = ftimes[this_time] * dz->frametime;
            endtime   = ftimes[next_time] * dz->frametime;          //  There are (say) 5 times and 4 stores
            storepos  = this_time * blokcnt;

            filtline[0] = (float)starttime;
            for(z = 0,pch = 1,amp = 2;z < pitchcnt;z++,pch+=2,amp+=2) {
                filtline[pch] = (float)fpitches[z];
                filtline[amp] = 0.0;                                //  Establish a zero-amp line for filter
            }
            for(n = 0,k = storepos; n < filtercnt; n++,k++) {
                if(pchstore[k] <= 0.0) {                            //  Interpolate through any pitch-zeros
                    j = storepos;
                    lastpch = pchstore[k];
                    while(lastpch <= 0.0) {
                        j -= blokcnt;
                        if(j < 0)
                            break;
                        lastpch = pchstore[j];
                    }
                    while(lastpch <= 0.0) {
                        j += blokcnt;
                        if(j > dz->iparam[FPKCNT] * 4 * dz->timeblokcnt)
                            break;
                        lastpch = pchstore[j];
                    }
                    if(lastpch <= 0.0)
                        pchstore[k] = -1.0;                         //  If truly a zero (i.e. at no time is this pitch NOT a zero) mark as such
                    else
                        pchstore[k] = lastpch;
                }
                for(pch = 1,amp=2;pch < filtlinelen;pch+=2,amp+=2) {    //  For each pitch used at this time
                    if(pchstore[k] == filtline[pch]) {              //  Activate this pitch (turn on amp) in the filter
                        filtline[amp] = (float)ampstore[k];
                        break;
                    }
                }
            }
            if(this_time > 0) {                                     
                do_intermediate_line = 0;
                for(pch = 1,amp = 2;pch < filtlinelen;pch+=2,amp+=2) {
                    if(filtline[amp] != lastfiltline[amp]) {        //  If the amplitude of any pitch in filter changes         
                        do_intermediate_line = 1;
                        break;
                    }
                }
                if(do_intermediate_line) {                          //  Output an intermediate line, half-way between this time and last-time
                    fprintf(dz->fp,"%lf ",(lasttime + starttime)/2.0);
                    for(z=1; z < filtlinelen;z++)                   //  With the values from the previous time,
                        fprintf(dz->fp,"%lf ",lastfiltline[z]);     //  So that an amplitude fade or rise will happen in 1/2 duration of previous block
                    fprintf(dz->fp,"\n");
                }
            }
            for(z=0; z < filtlinelen;z++)                           //  Output the filter line at this time
                fprintf(dz->fp,"%lf ",filtline[z]);
            fprintf(dz->fp,"\n");
                                                                    //  Remember this line, in case we need to construct intermediate line
            memcpy((char *)lastfiltline,(char *)filtline,filtlinelen * sizeof(float));
            lasttime = starttime;
        }
    }
    return FINISHED;
}

/**************************** FIND_FUNDAMENTAL *************************/

int find_fundamental(float thisfrq,double target,int peakcc,float *newfrq,dataptr dz)
{
    int cc, vc, fund_at = -1, divisor = 2;
    double goalfrq, maxamp = -HUGE;
    float foundfrq = thisfrq;
    if(target == 0.0)
        target = thisfrq/* /2 */;
    while(fund_at < 0) {
        goalfrq = thisfrq/(double)divisor;
        if(goalfrq < target * TWO_OVER_THREE)
            break;
        for(cc=0,vc=0;cc < peakcc;cc++,vc+=2) {
            if(equivalent_pitches(dz->flbufptr[0][FREQ],goalfrq,dz) && dz->flbufptr[0][AMPP] > maxamp) {
                maxamp = dz->flbufptr[0][AMPP];
                foundfrq = dz->flbufptr[0][FREQ];
                fund_at = cc;
            }
        }
        if(fund_at < 0)
            divisor++;
    }
    *newfrq = foundfrq;     //  If no new frq found, newfrq defaults to input frq
    if(fund_at >= 0)
        return fund_at;     
    return peakcc;          //  If no new frq found, new chan defaults to original chan
}

/**************************** EQUIVALENT_PITCHES *************************/

int equivalent_pitches(double frq1, double frq2, dataptr dz)
{
    double ratio;
    int   iratio;
    double intvl;

    ratio = frq1/frq2;
    iratio = round(ratio);

    if(iratio!=1)
        return(FALSE);

    if(ratio > iratio)
        intvl = ratio/(double)iratio;
    else
        intvl = (double)iratio/ratio;
    if(intvl > dz->in_tune) 
        return FALSE;
    return TRUE;
}

/**************************** FORMANTS_MOVE *************************/

int formants_move(int inner_lpcnt,dataptr dz)
{
    int exit_status, truecnt, fno, paramno = 0, lotrofchan = 0, peakchan = 0, hitrofchan = 0, cc, vc, top_hno;
    float lotrofamp = 0.0, peakamp = 0.0, hitrofamp = 0.0, the_fundamental = 0.0, frq;
    double frqoffset, pre_amptotal = 0.0, post_amptotal = 0.0;
    float *peaktrof = dz->fptr[PKTROF];
    int   *pos      = dz->iparray[PEAKPOS];
    pos      += inner_lpcnt * PKBLOK;
    peaktrof += inner_lpcnt * PKBLOK;

    if(dz->fundamental)
        the_fundamental = dz->pitches[inner_lpcnt];

    if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);

    if(dz->xclude_nonh || dz->vflag[SQZ_KHM]) {
        top_hno = 0;
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc+=2) {
            frq = dz->flbufptr[0][FREQ];
            if(dz->xclude_nonh) {                                       //  If excluding non-harmonic data
                if(the_fundamental > 0.0 && !channel_holds_harmonic(the_fundamental,frq,cc,&top_hno,dz)) {
                    dz->flbufptr[0][AMPP] = 0.0f;                       //  If the window has no pitch, or the frq is not a harmonic of fundamental
                    continue;                                           //  zero it
                }
            } else if(dz->vflag[SQZ_KHM]) {                             //  If suppressing harmonics    
                if(the_fundamental > 0.0) {                             //  IF the chanfrq is a harmonic, zero it                                                       
                    if(channel_holds_harmonic(the_fundamental,frq,cc,&top_hno,dz)) {
                        dz->flbufptr[0][AMPP] = 0.0f;
                        continue;
                    }
                }
            }                                                           //  If there is no pitch (or pitch value has NOT been interpolated thro unpitched area)
            if(the_fundamental < 0.0) {                                 //  this means that we have flagged up non-pitch and silence for deletion
                dz->flbufptr[0][AMPP] = 0.0f;                           //  Zero the channel
                    continue;
            }
        }
    }
    for(fno = 1;fno <= 4; fno++) {
        truecnt = 0;
        switch(fno) {
        case(1):    
            paramno = FMOVE1;
            lotrofchan = pos[0];
            peakchan   = pos[1];
            hitrofchan = pos[2]; 
            lotrofamp  = peaktrof[0];
            peakamp    = peaktrof[1];
            hitrofamp  = peaktrof[2]; 
            break;
        case(2):    
            paramno = FMOVE2;   
            lotrofchan = pos[2];
            peakchan   = pos[3];
            hitrofchan = pos[4]; 
            lotrofamp  = peaktrof[2];
            peakamp    = peaktrof[3];
            hitrofamp  = peaktrof[4]; 
            break;
        case(3):    
            paramno = FMOVE3;   
            lotrofchan = pos[4];
            peakchan   = pos[5];
            hitrofchan = pos[6]; 
            lotrofamp  = peaktrof[4];
            peakamp    = peaktrof[5];
            hitrofamp  = peaktrof[6]; 
            break;
        case(4):    
            paramno = FMOVE4;   
            lotrofchan = pos[6];
            peakchan   = pos[7];
            hitrofchan = pos[8]; 
            lotrofamp  = peaktrof[6];
            peakamp    = peaktrof[7];
            hitrofamp  = peaktrof[8]; 
            break;
        }
        frqoffset = dz->param[paramno]; //  Read appropriate parameter for frq offset of the formant
        if(frqoffset != 0.0) {
            if((exit_status = move_formant(fno,frqoffset,lotrofchan,peakchan,hitrofchan,lotrofamp,peakamp,hitrofamp,&truecnt,dz))<0)
                return exit_status;
            dz->iparray[FCNT][fno] = truecnt;
        }
    }
    if((exit_status = concatenate_moved_formants(dz))<0)
        return exit_status;

    if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    if(post_amptotal > pre_amptotal) {
        if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
            return(exit_status);
    }
    if(dz->param[FMVGAIN] > 0.0 && !flteq(dz->param[FMVGAIN],1.0))  {
        for(vc=0;vc<dz->wanted;vc+=2)
            dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FMVGAIN]);
    }
    return FINISHED;
}

/**************************** MOVE_FORMANT *************************/

int move_formant(int fno,double frqoffset,int lotrofchan,int peakchan,int hitrofchan,float lotrofamp,float peakamp,float hitrofamp,int *truecnt,dataptr dz)
{
    int exit_status, n, speccnt, chanwidth, is_narrowedabove = 0, is_narrowedbelow = 0;
    float *thisamp = NULL, *thisfrq = NULL, *thispch = NULL;
    double frqcentre, half_specbandwidth, frqstep = 0.0,frqstart = 0.0;
    switch(fno) {
    case(1):    thisamp = dz->fptr[FAMP1];  thisfrq = dz->fptr[FFRQ1], thispch = dz->fptr[FPCH1];   break;
    case(2):    thisamp = dz->fptr[FAMP2];  thisfrq = dz->fptr[FFRQ2], thispch = dz->fptr[FPCH2];   break;
    case(3):    thisamp = dz->fptr[FAMP3];  thisfrq = dz->fptr[FFRQ3], thispch = dz->fptr[FPCH3];   break;
    case(4):    thisamp = dz->fptr[FAMP4];  thisfrq = dz->fptr[FFRQ4], thispch = dz->fptr[FPCH4];   break;
    }
    for(n=0;n<dz->specenvcnt;n++)                                       //  Flag all new formant data as "unknown"
        thisamp[n] = -1.0;
    speccnt = 0;
    *truecnt = 0;                                                       //  When formant set to specific frq, Must create bandwidth related to bwidth of (moved) formant
    if(dz->mode == F_MOVE2) {                                           //  Use measure of half specenvelope bandwidth
        frqcentre = frqoffset;                                          //  CHWIDTH     1       2          3            4                 5
        half_specbandwidth = dz->frq_step/2.0;                          //  = bwidth    |      |  |     |  |  |     |  |  |  |      |  |  |  |  |
        chanwidth = (hitrofchan - lotrofchan) + 1;                      //  bwidth     ---    ------   ---------   ------------    ---------------
        if(!dz->vflag[MOV2_NRW]) {                                      //            -0.5bw   -1bw     -1.5bw         -2bw             -2.5bw
            frqstart = max(0.0,frqcentre - (half_specbandwidth * chanwidth));//  Band edge below centre frq    This = 1/2chwidth * bwidth = chwidth * 1/2bwidth       
            frqstep  = dz->frq_step;                                    
        } else {                                                        
            frqstart = max(0.0,frqcentre - half_specbandwidth);         //  Narrow bands use a single bandwidth and slice it
            frqstep  = dz->frq_step/(double)chanwidth;
        }
    }                                                                   
    for(n=lotrofchan;n <= hitrofchan;n++) {                                 
        switch(dz->mode) {
        case(F_MOVE):
            thisfrq[speccnt] = (float)(dz->specenvfrq[n] + frqoffset);  //  Move the frq positions of the specenv information
            break;
        case(F_MOVE2):
            thisfrq[speccnt] = (float)frqstart;                         //  Reset the frq position of the specenv information
            frqstart += frqstep;
            frqstart = min(frqstart,F_HIFRQ_LIMIT);
            break;
        }
        thisamp[speccnt] = dz->specenvamp[n];
        if(thisfrq[speccnt] < F_LOFRQ_LIMIT)                            //  If anything drops off bottom of range, keep it but DON'T count it (it then gets overwritten), but note this
            is_narrowedbelow = 1;                                       //  (And don't attemp to calc pitch if frq falls below 0.0)
        else {
            thispch[speccnt] = (float)log10(thisfrq[speccnt]);          //  Calc pitch at thus frq
            if(thisfrq[speccnt] > F_HIFRQ_LIMIT)                        //  If anything drops off top of range, note this, keep it and COUNT it
                is_narrowedabove = 1;
            else
                (*truecnt)++;                                           //  Count the number of chans which are within range    
        }
        speccnt++;                                                      //  Count the number of channels processed
    }
    if(*truecnt == 0)                                                   //  i.e. everything is now out of range, so formant contributes nothing to output
        return FINISHED;
    if(is_narrowedbelow) {
    if((exit_status = edgefade_formant(thisfrq,thisamp,thispch,  0,    truecnt,lotrofchan,peakchan,hitrofchan,lotrofamp,peakamp,hitrofamp,(float)F_LOFRQ_LIMIT,dz))>0)
            return exit_status;                                 //   below,chansused   original-trof-peak-trof         & their orig amps       frqlimit
    } else if(is_narrowedabove) {
        if((exit_status = edgefade_formant(thisfrq,thisamp,thispch,  1,    truecnt,lotrofchan,peakchan,hitrofchan,lotrofamp,peakamp,hitrofamp,(float)F_HIFRQ_LIMIT,dz))>0)
            return exit_status;                                 //   above,chansused    original-trof-peak-trof        & their orig amps       frqlimit
    }
    return FINISHED;
}

/*********************************************** CONCATENATE_MOVED_FORMANTS *****************************************/

int concatenate_moved_formants(dataptr dz)
{
    int exit_status, cc, vc, fmnt, startbad = 0, midbad = 0,  badstt = 0, k, kk, j ,jj; 
    float frq;
    double maxamp, amp, thisspecamp1, valgap, ratio, diff, startval;
    double *spec3_amp = dz->parray[FSPEC3];
    double *multbuf   = dz->parray[FMULT];
    float *spec3amps[5], *spec3pchs[5];
    int truecnt[5];

    spec3amps[1] = dz->fptr[FAMP1];
    spec3amps[2] = dz->fptr[FAMP2];
    spec3amps[3] = dz->fptr[FAMP3];
    spec3amps[4] = dz->fptr[FAMP4];
    spec3pchs[1] = dz->fptr[FPCH1];
    spec3pchs[2] = dz->fptr[FPCH2];
    spec3pchs[3] = dz->fptr[FPCH3];
    spec3pchs[4] = dz->fptr[FPCH4];
    truecnt[1] = dz->iparray[FCNT][1];
    truecnt[2] = dz->iparray[FCNT][2];
    truecnt[3] = dz->iparray[FCNT][3];
    truecnt[4] = dz->iparray[FCNT][4];

    if(!(truecnt[1] || truecnt[2] || truecnt[3] || truecnt[4]))
        return FINISHED;
    else
        dz->ischange = 1;

    for(cc = 0,vc = 0;cc < dz->clength;cc++,vc += 2) {
        frq = dz->flbufptr[0][FREQ];                                    //  For the frq at this flbufptr channel
        maxamp = -HUGE;
        for(fmnt = 1; fmnt <= 4; fmnt++) {                              //  Find the largest formant env value in the 4 formant arrays
            getspecenv3amp(frq,&amp,spec3pchs[fmnt],spec3amps[fmnt],truecnt[fmnt],dz);
            maxamp = max(amp,maxamp);
        }
        spec3_amp[cc] = (float)maxamp;
        if(maxamp < 0.0) {                                              //  No formant data found
            if(frq > F_HIFRQ_LIMIT)
                multbuf[cc] = 1.0;                                      //  For frq range of source above upper frq limit, no change to source
            else 
                multbuf[cc] = -1.0;                                     //  Otherwise flag that we have no new formant data at this frq (should already be -1 by default)
        } else {
            if((exit_status = getspecenvamp(&thisspecamp1,(double)frq,0,dz))<0)
                return(exit_status);                                    //  Find original formant envelope value
            if(thisspecamp1 <= 0.0)
                multbuf[cc] = 0.0;
            else
                multbuf[cc] = maxamp/thisspecamp1;                      //  Change in relation to new specenv value
        }
    }
    startbad = 0;                                                       //  Now smooth the new formant data curve
    midbad   = 0;

    for(cc = 0,vc = 0;cc < dz->clength;cc++,vc+=2) {
        if(multbuf[cc] < 0.0) {
            switch(cc) {
            case(0): 
                startbad++;
                break;
            default: 
                if(startbad)
                    startbad++;
                else {
                    if(midbad == 0) {
                        badstt = cc;     
                    }
                    midbad++;
                }
                break;
            }
        } else {
            if(startbad) {                                              //  If no formant info at start
                for(k = 0,kk = (k*2)+1;k < startbad;k++,kk+=2) {        //  force these envelope vals to take same level as 1st trof
                    if(multbuf[cc] == 1.0)                              //  No formant data found in window, maxamp = 0.0
                        spec3_amp[k] = 0.0;
                    else
                        spec3_amp[k] = spec3_amp[cc];
                    frq = dz->flbufptr[0][kk];
                    if((exit_status = getspecenvamp(&thisspecamp1,(double)frq,0,dz))<0)
                        return(exit_status);
                    if(thisspecamp1 <= 0.0)
                        multbuf[k] = 0.0;
                    else
                        multbuf[k] = spec3_amp[k]/thisspecamp1;         //  Change in relation to new (interpolated) specenv value
                }
                startbad = 0;
            } else if(midbad) {                                         //  IF no formant info within, interpolate between trofs on either side
                if(multbuf[badstt-1] == 1.0)                            //  No formant data found in window, maxamp = 0.0
                    startval = 0.0;
                else
                    startval = spec3_amp[badstt-1];
                if(multbuf[cc] == 1.0)                                  //  No formant data found in window, maxamp = 0.0: gap = 0.0 - startval
                    valgap = -startval;
                else
                    valgap = spec3_amp[cc] - startval;
                for(k = 0,j = badstt,jj = (badstt*2)+1; k < midbad;k++,j++,jj+=2) {
                    ratio = (double)(k+1)/(double)(midbad+1);
                    diff = valgap * ratio;
                    spec3_amp[j] = (float)(startval + diff);
                    frq = dz->flbufptr[0][jj];
                    if((exit_status = getspecenvamp(&thisspecamp1,(double)frq,0,dz))<0)
                        return(exit_status);
                    if(thisspecamp1 <= 0.0)
                        multbuf[j] = 0.0;
                    else
                        multbuf[j] = spec3_amp[j]/thisspecamp1;         //  Change in relation to new specenv value
                }
                midbad = 0;
            }
        }
    }
    if(midbad) {                                                        //  If no formant info at end, force endvals to have same val as trof at end of last formant
        startval = spec3_amp[badstt-1];
        for(cc = badstt, vc = cc*2; cc < dz->clength;cc++,vc+=2) {
            frq = dz->flbufptr[0][FREQ];
            if((exit_status = getspecenvamp(&thisspecamp1,(double)frq,0,dz))<0)
                return(exit_status);
            if(thisspecamp1 <= 0.0)
                multbuf[cc] = 0.0;
            else
                multbuf[cc] = startval/thisspecamp1;                    //  Change in relation to new specenv value
        }
    }

    if(dz->vflag[MOV_ZEROTOP]) {                                        //  If top of spectrum (above top formant) to be zeroed
        for(cc = dz->clength - 1;cc >= 0;cc--) {                        //  Search down multbuf whilst ever the multiplier value is 1.0
            if(multbuf[cc] == 1.0)                                      //  i.e. multbuf does nothing to the spectrum i.e. no formant data has been moved there.
                multbuf[cc] = 0.0;                                      //  and zero this not-reformanted area of the spectrum
            else
                break;
        }
    }
    //  Now do the formant transformation!!

    for(cc = 0,vc = 0;cc < dz->clength;cc++,vc += 2)
        dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * multbuf[cc]);
    return FINISHED;
}
            
/**************************** EDGEFADE_FORMANT *************************/

int edgefade_formant(float *thisfrq,float *thisamp,float *thispch,int hi,
        int *nuchanlen, int lotrofchan,int peakchan,int hitrofchan,float lotrofamp,float peakamp, float hitrofamp,float frqlimit,dataptr dz)
{
    int chanlen, preskirtlen, nupreskirtlen, n, origchanlo, origchanhi;
    double shrinkage, trofrise, fracpos, peakampfoot, peakfracpos, peakamprise, nupeakamprise;
    double nupeakamp, envfoot, origchan, diff, amp, amprise, newamprise, newamp, origchfrac;
    float origamplo, origamphi;

    if(*nuchanlen > 1) {
        chanlen = hitrofchan - lotrofchan;                          //  No of channels originally spanned by formant
        
        shrinkage = (double)(*nuchanlen)/(double)chanlen;           //  Shrinkage of formant-size

        preskirtlen = peakchan - lotrofchan;                        //  channel-length of rising skirt to original peak
        trofrise    = hitrofamp - lotrofamp;                        //  Change in level between the 2 trofs bracketing the peak
        peakfracpos = (double)preskirtlen/(double)chanlen;          //  Fractional position of peak between trofs
        
        peakampfoot = lotrofamp + (trofrise * peakfracpos);         //  "Foot" of peak lies on the line joining the 2 bracketing trofs
        peakamprise = peakamp - peakampfoot;                        //  "Rise" is height of peak above foot

        nupeakamprise = peakamprise * shrinkage;                    //  Peak height lowered porportionally to shrinkage of format width
        nupeakamp     = peakampfoot + nupeakamprise;

        nupreskirtlen = (int)round((double)preskirtlen * shrinkage);//  Rising skirt of peak shrunk in size

        for(n = 0; n < nupreskirtlen; n++) {                        //  For each channel in the new preskirt
            fracpos = (double)n/(double)(*nuchanlen);
            envfoot = (trofrise * fracpos) + lotrofamp;             //  Calculate the value of the trof-line (interp value beween the 2 bracketing trofs)
            origchan = (double)lotrofchan + ((double)chanlen * fracpos);//  Which channel-position in ORIG specenv corresponds to this fraction into the skirt
            origchanlo = (int)floor(origchan);                      //  Find the bracketing channels in ORIG specenv
            origchanhi = min(origchanlo+1,dz->specenvcnt-1);        
            origchfrac = origchan - (double)origchanlo;
            origamplo = dz->specenvamp[origchanlo];
            origamphi = dz->specenvamp[origchanhi];
            diff = origamphi - origamplo;
            amp  = origamplo + (origchfrac * diff);                 //  Use these to interpolate for a value from ORIG specenvamp
            amprise = amp - envfoot;                                //  How far is this level above the trof-line
            newamprise = amprise * shrinkage;                       //  Shrink this rise-above-trof
            newamp = envfoot + newamprise;                          //  and calculate the new env amplitude.
            thisamp[n] = (float)newamp;                             //  store it 
        }
        thisamp[n] = (float)nupeakamp;                                  //  Set peak to new (shrunk) level

        for(n = nupreskirtlen + 1; n < *nuchanlen; n++) {           //  Do simil calcs for the post-skirt
            fracpos = (double)n/(double)(*nuchanlen);
            envfoot = (trofrise * fracpos) + lotrofamp;
            origchan = (double)lotrofchan + ((double)chanlen * fracpos);
            origchanlo = (int)floor(origchan);
            origchanhi = min(origchanlo+1,dz->specenvcnt-1);
            origchfrac = origchan - (double)origchanlo;
            origamplo = dz->specenvamp[origchanlo];
            origamphi = dz->specenvamp[origchanhi];
            diff = origamphi - origamplo;
            amp  = origamplo + (origchfrac * diff);
            amprise = amp - envfoot;
            newamprise = amprise * shrinkage;
            newamp = envfoot + newamprise;
            thisamp[n] = (float)newamp;
        }
    } else                                                          //  ELSE if only 1 specenv3val, which will be (start or end) trof,
        n = 1;                                                      //  set another specenv3val to the other (end or start) trof

    if(hi) {                                                        //  If falling off top of range
        thisamp[n] = hitrofamp;                                     //  Keep the upper trof value at end of new, shrunk formant
        thisfrq[n] = frqlimit;                                      //  But set its centre, & its upper limit, to the upper frqlimit
        thispch[n] = (float)log10(frqlimit);
    } else {                                                        //  If falling off bottom of range
        thisamp[0] = lotrofamp;                                     //  Keep the lower trof value at start of new, shrunk formant
        thisfrq[0] = frqlimit;                                      //  Force specchan centre to be at lofrqlimit
        thispch[0] = (float)log10(frqlimit);
    }
    n++;
    *nuchanlen = n;
    return FINISHED;
}

/**************************** GETSPECENV3AMP *************************/

int getspecenv3amp(double frq,double *amp,float *thispch,float *thisamp,int speccnt,dataptr dz)
{
    double pp, ratio, ampdiff;
    int z = 0;
    if(speccnt == 0) {      /*  FORMANT NOT ACTIVE */
        *amp = -1.0;        /*  flag no amp specified */
        return(FINISHED);
    }
    if(frq<0.0) {           /* FREQ NOT AVAILABLE */
        *amp = -1.0;        /*  flag no amp specified */
        return(FINISHED);   
    }
    if(frq<=1.0)            /* if frq very low, specify pitch 0.0 */
        pp = 0.0;
    else
        pp = log10(frq); 
    if(pp < thispch[0] || pp > thispch[speccnt-1]) {
        *amp = -1.0;        /*  If pitch out of range of this specenv3 */
        return(FINISHED);   /*  flag no amp specified */
    }
    while(thispch[z] < pp){ /*  Search for channels spanning frequency */
        z++;
        if(z >= speccnt - 1)
            break;
    }                       /*  Interp in specenvamp-envelope to find amp at this frq */
    ratio    = (pp - thispch[z-1])/(thispch[z] - thispch[z-1]);
    ampdiff  = thisamp[z] - thisamp[z-1];
    *amp = thisamp[z-1] + (ampdiff * ratio);
    *amp = max(0.0,*amp);   /*  Avoid -ve amps */
    return(FINISHED);
}

/**************************** FWINDOW_SIZE *************************/

int fwindow_size(dataptr dz)
{
    dz->shortwins = 0;
    switch(dz->mode) {
    case(F_NARROW):     if(dz->vflag[NRW_SW])   dz->shortwins = 1;  break;
    case(F_SQUEEZE):    if(dz->vflag[SQZ_SW])   dz->shortwins = 1;  break;
    case(F_NEGATE):     break;  //  Does not extract spectral envelope, but sues array to store OLD contour
    case(F_MAKEFILT):   if(dz->vflag[FLT_SW])   dz->shortwins = 1;  break;
    case(F_MOVE):       //  fall thro
    case(F_MOVE2):      if(dz->vflag[MOV_SW])       dz->shortwins = 1;  break;
    case(F_INVERT):     if(dz->vflag[INVERT_SW])    dz->shortwins = 1;  break;
    case(F_ROTATE):     if(dz->vflag[ROTATE_SW])    dz->shortwins = 1;  break;
    case(F_SUPPRESS):   if(dz->vflag[SUPPRESS_SW])  dz->shortwins = 1;  break;
    case(F_SEE):        if(dz->vflag[SEE_SW])       dz->shortwins = 1;  break;
    case(F_SEEPKS):     if(dz->vflag[SEEPKS_SW])    dz->shortwins = 1;  break;
    case(F_SINUS):      if(dz->vflag[FSIN_SW])      dz->shortwins = 1;  break;
    case(F_ARPEG):      //  fall_thro
    case(F_OCTSHIFT):   //  fall_thro
    case(F_TRANS):      //  fall_thro
    case(F_FRQSHIFT):   //  fall_thro
    case(F_RESPACE):    //  fall_thro
    case(F_PINVERT):    //  fall_thro
    case(F_PEXAGG):     //  fall_thro
    case(F_PQUANT):     //  fall thro
    case(F_PCHRAND):    //  fall thro
    case(F_RAND):       if(dz->vflag[RECOLOR_SW])   dz->shortwins = 1;  break;
    case(F_SYLABTROF):  break;
    default:
        sprintf(errstr,"Unknown mode in small-window check\n");
        return PROGRAM_ERROR;
    }
    return FINISHED;
}

//////////////////////////////////
//      GETTING THE PITCH       //
//////////////////////////////////


/***************************** INITIALISE_PITCHWORK *************************/

int initialise_pitchwork(int is_launched,dataptr dz)
{
    int exit_status;
    if((exit_status = establish_arrays_for_pitchwork(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }   
    dz->fsil_ratio = FSILENCE_RATIO/20.0;
    dz->fsil_ratio = pow(10.0,dz->fsil_ratio);
    dz->fsil_ratio = 1.0/dz->fsil_ratio;
    initrand48();                               //  Possibly needed
    if((exit_status = setup_ring(dz))<0)        //  Establish ring used for pitch-detection
        return(exit_status);
    return FINISHED;
}

/***************************** ESTABLISH_ARRAYS_FOR_PITCHWORK *************************/

int establish_arrays_for_pitchwork(dataptr dz)
{
    int i;
    if((dz->parray[P_PRETOTAMP] = (double *)malloc(dz->wlength * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for amp totalling array.\n");
        return(MEMORY_ERROR);
    }
    if(dz->mode == F_SYLABTROF)
        return(FINISHED);
    if((dz->parray[RUNNINGAVRG] = (double *)malloc(dz->wlength * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for running averages array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[RUNAVGDURS] = (double *)malloc(dz->wlength * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for running average durations array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[AVSTT] = (int *)malloc(dz->wlength * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for running averages window-position array.\n");
        return(MEMORY_ERROR);
    }
//RWD NOV97  quick solution to unassigned value somewhere in spec pitch!
    for(i = 0; i < dz->wlength;i++)
        dz->parray[P_PRETOTAMP][i] = 0.0;

    if((dz->pitches = (float *)malloc(dz->wlength * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for pitches array.\n");
        return(MEMORY_ERROR);
    }
    if(dz->mode == F_PQUANT || dz->mode == F_PCHRAND) {
        if((dz->pitches2 = (float *)malloc(dz->wlength * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for pitches array.\n");
            return(MEMORY_ERROR);
        }
    }
    return(FINISHED);
}

/****************************** SPECPITCH *******************************
 *
 * (1)  Ignore partials below low limit of pitch.
 * (2)  If this channel data is louder than any existing piece of data in ring.
 *      (Ring data is ordered loudness-wise)...
 * (3)  If this freq is too close to an existing frequency..
 * (4)  and if it is louder than that existing frequency data..
 * (5)  Substitute in in the ring.
 * (6)  Otherwise, (its a new frq) insert it into the ring.
 */
 
int specpitch(int inner_lpcnt,double *target, dataptr dz)
{
    int exit_status;
    int vc, pitchcc;
    chvptr here, there, *partials;
    float minamp, newfrq;
    double loudest_partial_frq, nextloudest_partial_frq, lo_loud_partial, hi_loud_partial, thepitch;
    if((partials = (chvptr *)malloc(FMAXIMI * sizeof(chvptr)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for partials array.\n");
        return(MEMORY_ERROR);
    }
    if((exit_status = initialise_ring_vals(FMAXIMI,-1.0,dz))<0)
        return(exit_status);
    for(vc=0;vc<dz->wanted;vc+=2) {
        here = dz->ringhead;
        if(dz->flbufptr[0][FREQ] > VOICELO) {       /* 1 */
            do {                                
                if(dz->flbufptr[0][AMPP] > here->val) {             /* 2 */
                    if((exit_status = close_to_frq_already_in_ring(&there,(double)dz->flbufptr[0][FREQ],dz))<0)
                        return(exit_status);
                    if(exit_status==TRUE) {
                        if(dz->flbufptr[0][AMPP] > there->val) {        /* 4 */
                            if((exit_status = substitute_in_ring(vc,here,there,dz))<0) /* 5 */
                                return(exit_status);
                        }
                    } else  {                                       /* 6 */
                        if((exit_status = insert_in_ring(vc,here,dz))<0)
                            return(exit_status);
                    }               
                    break;
                }
            } while((here = here->next)!=dz->ringhead);
        }
    }
    loudest_partial_frq     = dz->flbufptr[0][dz->ringhead->loc + 1];
    nextloudest_partial_frq = dz->flbufptr[0][dz->ringhead->next->loc + 1];
    if(loudest_partial_frq < nextloudest_partial_frq) {
        lo_loud_partial = loudest_partial_frq;
        hi_loud_partial = nextloudest_partial_frq;
    } else {
        lo_loud_partial = nextloudest_partial_frq;
        hi_loud_partial = loudest_partial_frq;
    }
    if((exit_status = put_ring_frqs_in_ascending_order(&partials,&minamp,dz))<0)
        return(exit_status);
    if((exit_status = found_pitch(partials,lo_loud_partial,hi_loud_partial,minamp,&thepitch,dz))<0)
        return(exit_status);
    if(exit_status==TRUE) {
        if((exit_status = locate_channel_of_pitch(inner_lpcnt,(float)thepitch,&pitchcc,dz))<0)
            return(exit_status);
        if((exit_status = find_fundamental((float)thepitch,*target,pitchcc,&newfrq,dz))<0)
            return(exit_status);
        *target = (newfrq + *target)/2.0;           //  target pitch is a running average
        dz->pitches[dz->total_windows] = (float)thepitch;
    } else
        dz->pitches[dz->total_windows] = (float)NOT_PITCH;
    if((exit_status = smooth_spurious_octave_leaps(dz->total_windows,minamp,dz))<0)
        return(exit_status);
    return FINISHED;
}

/**************************** CLOSE_TO_FRQ_ALREADY_IN_RING *******************************/

int close_to_frq_already_in_ring(chvptr *there,double frq1,dataptr dz)
{
    double frq2, frqratio;
    *there = dz->ringhead;
    do {
        if((*there)->val > 0.0) {
            frq2 = dz->flbufptr[0][(*there)->loc + 1];
            if(frq1 > frq2)
                frqratio = frq1/frq2;
            else
                frqratio = frq2/frq1;
            if(frqratio < EIGHT_OVER_SEVEN)
                return(TRUE);
        }
    } while((*there = (*there)->next) != dz->ringhead);
    return(FALSE);
}

/******************************* SUBSITUTE_IN_RING **********************/

int substitute_in_ring(int vc,chvptr here,chvptr there,dataptr dz)
{
    chvptr spare, previous;
    if(here!=there) {
        if(there==dz->ringhead) {
            sprintf(errstr,"IMPOSSIBLE! in substitute_in_ring()\n");
            return(PROGRAM_ERROR);
        }
        spare = there;
        there->next->last = there->last; /* SPLICE REDUNDANT STRUCT FROM RING */
        there->last->next = there->next;
        previous = here->last;
        previous->next = spare;         /* SPLICE ITS ADDRESS-SPACE BACK INTO RING */
        spare->last = previous;         /* IMMEDIATELY BEFORE HERE */
        here->last = spare;
        spare->next = here;
        if(here==dz->ringhead)          /* IF HERE IS RINGHEAD, MOVE RINGHEAD */
            dz->ringhead = spare;
        here = spare;                   /* POINT TO INSERT LOCATION */
    }
    here->val = dz->flbufptr[0][AMPP];  /* IF here==there */
    here->loc = vc;                     /* THIS WRITES OVER VAL IN EXISTING RING LOCATION */
    return(FINISHED);
}

/*************************** INSERT_IN_RING ***************************/

int insert_in_ring(int vc, chvptr here, dataptr dz)
{
    chvptr previous, newend, spare;
    if(here==dz->ringhead) {
        dz->ringhead = dz->ringhead->last;
        spare = dz->ringhead;
    } else {
        if(here==dz->ringhead->last)
            spare = here;
        else {
            spare  = dz->ringhead->last;
            newend = dz->ringhead->last->last;  /* cut ENDADR (spare) out of ring */
            dz->ringhead->last = newend;
            newend->next = dz->ringhead;
            previous = here->last;
            here->last = spare;                 /* reuse spare address at new loc by */ 
            spare->next = here;                 /* inserting it back into ring before HERE */
            previous->next = spare;
            spare->last = previous;
        }
    }
    spare->val = dz->flbufptr[0][vc];           /* Store new val in spare ring location */
    spare->loc = vc;
    return(FINISHED);
}

/************************** PUT_RING_FRQS_IN_ASCENDING_ORDER **********************/

int put_ring_frqs_in_ascending_order(chvptr **partials,float *minamp,dataptr dz)
{
    int k;
    chvptr start, ggot, here = dz->ringhead;
    float minpitch;
    *minamp = (float)MAXFLOAT;
    for(k=0;k<FMAXIMI;k++) {
        if((*minamp = min(dz->flbufptr[0][here->loc],*minamp))>=(float)MAXFLOAT) {
            sprintf(errstr,"Problem with amplitude out of range: put_ring_frqs_in_ascending_order()\n");
            return(PROGRAM_ERROR);
        }
        (here->loc)++;      /* CHANGE RING TO POINT TO FRQS, not AMPS */
        here->val = dz->flbufptr[0][here->loc];
        here = here->next;
    }
    here = dz->ringhead;
    minpitch = dz->flbufptr[0][here->loc];
    for(k=1;k<FMAXIMI;k++) {
        start = ggot = here;
        while((here = here->next)!=start) {     /* Find lowest frq */
            if(dz->flbufptr[0][here->loc] < minpitch) { 
                minpitch = dz->flbufptr[0][here->loc];
                ggot = here;
            }
        }
        (*partials)[k-1] = ggot;                /* Save its address */
        here = ggot->next;                      /* Move to next ring site */
        minpitch = dz->flbufptr[0][here->loc];  /* Preset minfrq to val there */
        ggot->last->next = here;                /* Unlink ringsite ggot */
        here->last = ggot->last;
    }
    (*partials)[k-1] = here;                    /* Remaining ringsite is maximum */

    here = dz->ringhead = (*partials)[0];       /* Reconstruct ring */
    for(k=1;k<FMAXIMI;k++) {
        here->next = (*partials)[k];
        (*partials)[k]->last = here;
        here = here->next;
    }
    here->next = dz->ringhead;                  /* Close up ring */
    dz->ringhead->last = here;
    return(FINISHED);
}

/******************************  FOUND_PITCH **************************/

int found_pitch(chvptr *partials,double lo_loud_partial,double hi_loud_partial,float minamp,double *thepitch,dataptr dz)
{
    int n, m, k, maximi_less_one = MAXIMUM_PARTIAL - 1, endd = 0;
    double whole_number_ratio, comparison_frq;
    for(n=1;n<maximi_less_one;n++) {
        for(m=n+1;m<MAXIMUM_PARTIAL;m++) {  /* NOV 7 */
            whole_number_ratio = (double)m/(double)n;
            comparison_frq     = lo_loud_partial * whole_number_ratio;
            if(equivalent_pitches(comparison_frq,hi_loud_partial,dz))
                endd = (MAXIMUM_PARTIAL/m) * n;     /* explanation at foot of file */
            else if(comparison_frq > hi_loud_partial)
                break;


            for(k=n;k<=endd;k+=n) {
                *thepitch = lo_loud_partial/(double)k;
                if(*thepitch>FPICH_HILM)
                    continue;
                if(*thepitch<SPEC_MINFRQ)
                    break;
                if(is_peak_at(*thepitch,0,minamp,dz)){
                    if(enough_partials_are_harmonics(partials,*thepitch,dz))
                        return TRUE;
                }
            }
        }
    }
    return(FALSE);
}

/************************ SMOOTH_SPURIOUS_OCTAVE_LEAPS ***************************/

int smooth_spurious_octave_leaps(int pitchno,float minamp,dataptr dz)
{
    double thispitch = dz->pitches[pitchno];
    double startpitch, lastpitch;
    int k = 0;
    if(pitchno<=0)
        return(FINISHED);
    lastpitch = dz->pitches[pitchno-1];
    if(lastpitch > SPEC_MINFRQ && thispitch > SPEC_MINFRQ) {    /* OCTAVE ADJ HERE */
        if(thispitch > lastpitch) {     /* OCTAVE ADJ FORWARDS */
            startpitch = thispitch;
            while(thispitch/lastpitch > ALMOST_TWO)
                thispitch /= 2.0;
            if(thispitch!=startpitch) {
                if(thispitch < SPEC_MINFRQ)
                    return(FINISHED);
                if(is_peak_at(thispitch,0L,minamp,dz))              
                    dz->pitches[pitchno] = (float)thispitch;
                else 
                    dz->pitches[pitchno] = (float)startpitch;
            }
            return(FINISHED);
        } else {
            while(pitchno>=1) {         /* OCTAVE ADJ BCKWARDS */
                k++;
                if((thispitch = dz->pitches[pitchno--])<SPEC_MINFRQ)
                    return(FINISHED);
            
                if((lastpitch = dz->pitches[pitchno])<SPEC_MINFRQ)
                    return(FINISHED);
            
                startpitch = lastpitch;
                while(lastpitch/thispitch > ALMOST_TWO)
                    lastpitch /= 2.0;
            
                if(lastpitch!=startpitch) {
                    if(lastpitch < SPEC_MINFRQ)
                        return(FINISHED);
                    if(is_peak_at(lastpitch,k,minamp,dz))                   
                        dz->pitches[pitchno] = (float)lastpitch;
                    else 
                        dz->pitches[pitchno] = (float)startpitch;
                }
            }
        }
    }
    return(FINISHED);
}

/*************************** IS_PEAK_AT ***************************/

int is_peak_at(double frq,int window_offset,float minamp,dataptr dz)
{
    float *thisbuf;
    int cc, vc, searchtop, searchbot;
    if(window_offset) {                             /* BAKTRAK ALONG BIGBUF, IF NESS */
        thisbuf = dz->flbufptr[0] - (window_offset * dz->wanted);
        if((int)thisbuf < 0 || thisbuf < dz->bigfbuf || thisbuf >= dz->flbufptr[2])
            return(FALSE);
    } else
        thisbuf = dz->flbufptr[0];
    cc = (int)((frq + dz->halfchwidth)/dz->chwidth);         /* TRUNCATE */
    searchtop = min(dz->clength,cc + FCHANSCAN + 1);
    searchbot = max(0,cc - FCHANSCAN);
    for(cc = searchbot ,vc = searchbot*2; cc < searchtop; cc++, vc += 2) {
        if(!equivalent_pitches((double)thisbuf[vc+1],frq,dz)) {
            continue;
        }
        if(thisbuf[vc] < minamp * FPEAK_LIMIT)
            continue;
        if(local_peak(cc,frq,thisbuf,dz))       
            return TRUE;
    }
    return FALSE;
}

/***************************** LOCAL_PEAK **************************/

int local_peak(int thiscc,double frq, float *thisbuf, dataptr dz)
{
    int thisvc = thiscc * 2;
    int cc, vc, searchtop, searchbot;
    double frqtop = frq * SEMITONE_INTERVAL;
    double frqbot = frq / SEMITONE_INTERVAL;
    searchtop = (int)((frqtop + dz->halfchwidth)/dz->chwidth);      /* TRUNCATE */
    searchtop = min(dz->clength,searchtop + PEAKSCAN + 1);
    searchbot = (int)((frqbot + dz->halfchwidth)/dz->chwidth);      /* TRUNCATE */
    searchbot = max(0,searchbot - PEAKSCAN);
    for(cc = searchbot ,vc = searchbot*2; cc < searchtop; cc++, vc += 2) {
        if(thisbuf[thisvc] < thisbuf[vc])
            return(FALSE);
    }
    return(TRUE);
}

/**************************** ENOUGH_PARTIALS_ARE_HARMONICS *************************/

int enough_partials_are_harmonics(chvptr *partials,double thepitch,dataptr dz)
{
    int n, good_match = 0;
    double thisfrq;
    for(n=0;n<FMAXIMI;n++) {
        if((thisfrq = dz->flbufptr[0][partials[n]->loc]) < thepitch)
            continue;
        if(is_equivalent_pitch(thisfrq,thepitch,dz)){       
            if(++good_match >= GOOD_MATCH)
                return TRUE;
        }
    }
    return FALSE;
}

/**************************** IS_EQUIVALENT_PITCH *************************/

int is_equivalent_pitch(double frq1,double frq2,dataptr dz)
{
    double ratio = frq1/frq2;
    int   iratio = round(ratio);
    double intvl;

    ratio = frq1/frq2;
    iratio = round(ratio);

    if(ratio > iratio)
        intvl = ratio/(double)iratio;
    else
        intvl = (double)iratio/ratio;
    if(intvl > SEMITONE_INTERVAL)
        return(FALSE);
    return(TRUE);
}

/**************************** IS_ABOVE_EQUIVALENT_PITCH *************************/

int is_above_equivalent_pitch(double frq1,double frq2,dataptr dz)
{
    double ratio = frq1/frq2;
    int   iratio = round(ratio);
    double intvl;

    ratio = frq1/frq2;
    iratio = round(ratio);
    intvl = ratio/(double)iratio;

    if(intvl > SEMITONE_INTERVAL)
        return(FALSE);
    return(TRUE);
}

//  SMOOTHING AND VERIFYING PITCH

/***************************** TIDY_UP_PITCH_DATA ***********************/

int tidy_up_pitch_data(dataptr dz)
{
    int exit_status;
    if((exit_status = anti_noise_smoothing(dz->wlength,dz->pitches,dz->frametime))<0)
        return(exit_status);
    if((exit_status = continuity_smoothing(dz))<0)
        return(exit_status);
    if((exit_status = mark_zeros_in_pitchdata(dz))<0)
        return(exit_status);
    if((exit_status = eliminate_blips_in_pitch_data(dz))<0)
        return(exit_status);
    if((exit_status = interpolate_pitch(dz))<0)
        return exit_status;
    return pitch_found(dz);
}

/**************************** PITCH_FOUND ****************************/

int pitch_found(dataptr dz)
{
    int n;
    for(n=0;n<dz->wlength;n++) {
        if(dz->pitches[n] > NOT_PITCH)
            return(FINISHED);
    }
    sprintf(errstr,"No valid pitch found.\n");
    return(GOAL_FAILED);
}
 
/********************************** MARK_ZEROS_IN_PITCHDATA ************************
 *
 * Disregard data on windows which are FSILENCE_RATIO below maximum level.
 */

int mark_zeros_in_pitchdata(dataptr dz)
{
    int n;
    double maxlevel = 0.0, minlevel;
    for(n=0;n<dz->wlength;n++) {
        if(dz->parray[P_PRETOTAMP][n] > maxlevel)
            maxlevel = dz->parray[P_PRETOTAMP][n];
    }
    minlevel = maxlevel * dz->fsil_ratio;
    for(n=0;n<dz->wlength;n++) {
        if(dz->parray[P_PRETOTAMP][n] < minlevel)
            dz->pitches[n] = (float)NOT_SOUND;
    }
    return(FINISHED);
}

/************************** ELIMINATE_BLIPS_IN_PITCH_DATA ****************************
 *
 * (1)  Eliminate any group of FBLIPLEN pitched windows, bracketed by
 *  unpitched windows, as unreliable data.
 */

int eliminate_blips_in_pitch_data(dataptr dz)
{
    int n, m, k, wlength_less_bliplen;
    int OK = 1;
    wlength_less_bliplen = dz->wlength - FBLIPLEN;
    for(n=1;n<wlength_less_bliplen;n++) {
        if(dz->pitches[n] > 0.0) {
            if(dz->pitches[n-1] < 0.0) {
                for(k = 1; k <= FBLIPLEN; k++) {
                    if(dz->pitches[n+k] < 0.0) {
                        for(m=0;m<k;m++)
                            dz->pitches[n+m] = (float)NOT_PITCH;
                        n += k;
                        continue;
                    }
                }
            }
        }
    }
    n = wlength_less_bliplen;
    if((dz->pitches[n] > 0.0) && (dz->pitches[n-1] < 0.0)) {
        for(k = 1; k < FBLIPLEN; k++) {
            if(dz->pitches[n+k] < 0.0)
                OK = 0;
            break;
        }
    }
    if(!OK)  {
        for(n=wlength_less_bliplen;n<dz->wlength;n++)
            dz->pitches[n] = (float)NOT_PITCH;
    }
    return(FINISHED);
}

/********************** ANTI_NOISE_SMOOTHING *********************/

int anti_noise_smoothing(int wlength,float *pitches,float frametime)
{
    char *smooth;
    double max_pglide;
    int n;
    if(wlength < FMIN_SMOOTH_SET + 1)
        return(FINISHED);

    max_pglide = pow(2.0,FMAX_GLISRATE) * frametime;

    if((smooth = (char *)malloc((size_t)wlength))==NULL) {
        sprintf(errstr,"aINSUFFICIENT MEMORY for smoothing array.\n");
        return(MEMORY_ERROR);
    }
    for(n=1;n<wlength-1;n++)
        smooth[n] = (char)is_smooth_from_both_sides(n,max_pglide,pitches);
    smooth[0]         = (char)is_initialpitch_smooth(smooth,max_pglide,pitches);
    smooth[wlength-1] = (char)is_finalpitch_smooth(smooth,max_pglide,wlength,pitches);
    for(n=FMIN_SMOOTH_SET-1;n<wlength;n++) {
        if(!smooth[n])
            smooth[n] = (char)is_smooth_from_before(n,smooth,max_pglide,pitches);
    }
    for(n=0;n<=wlength-FMIN_SMOOTH_SET;n++) {
        if(!smooth[n])
            smooth[n] = (char)is_smooth_from_after(n,smooth,max_pglide,pitches);
    }
    test_glitch_sets(smooth,max_pglide,wlength,pitches);
    remove_unsmooth_pitches(smooth,wlength,pitches);
    free(smooth);
    return(FINISHED);
}       

/********************** IS_SMOOTH_FROM_BOTH_SIDES *********************
 *
 * verify a pitch if it has continuity with the pitches on either side.
 */

int is_smooth_from_both_sides(int n,double max_pglide,float *pitches)
{
    float thispitch, pitch_before, pitch_after;
    double pre_interval, post_interval;
    if((thispitch = pitches[n]) < FLTERR)
        return FALSE;
    if((pitch_before = pitches[n-1]) < FLTERR)
        return  FALSE;
    if((pitch_after = pitches[n+1]) < FLTERR)
        return  FALSE;
    pre_interval  = pitch_before/thispitch;
    if(pre_interval < 1.0)
        pre_interval = 1.0/pre_interval;
    post_interval = pitch_after/thispitch;
    if(post_interval < 1.0)
        post_interval = 1.0/post_interval;
    if(pre_interval  > max_pglide
    || post_interval > max_pglide)
        return  FALSE;
    return  TRUE;
}

/********************** IS_INITIALPITCH_SMOOTH *********************
 *
 * verify first pitch if it has continuity with an ensuing verified pitch.
 */

int is_initialpitch_smooth(char *smooth,double max_pglide,float *pitches)
{
    float thispitch;
    int n;
    double post_interval;
    if((thispitch = pitches[0]) < FLTERR)
        return FALSE;
    for(n=1;n < FMIN_SMOOTH_SET;n++) {
        if(smooth[n]) {
            post_interval = pitches[n]/pitches[0];
            if(post_interval < 1.0)
                post_interval = 1.0/post_interval;
            if(post_interval <= pow(max_pglide,(double)n))
                return TRUE;
        }
    }   
    return(FALSE);
}
            
/********************** IS_FINALPITCH_SMOOTH *********************
 *
 * verify final pitch if it has continuity with a preceding verified pitch.
 */

int is_finalpitch_smooth(char *smooth,double max_pglide,int wlength,float *pitches)
{
    float thispitch;
    double pre_interval;
    int n;
    int last = wlength - 1;
    if((thispitch = pitches[last]) < FLTERR)
        return FALSE;
    for(n=1;n < FMIN_SMOOTH_SET;n++) {
        if(smooth[last-n]) {
            pre_interval = pitches[last-n]/pitches[last];
            if(pre_interval < 1.0)
                pre_interval = 1.0/pre_interval;
            if(pre_interval <= pow(max_pglide,(double)n))
                return TRUE;
        }
    }
    return(FALSE);
}
            
/********************** IS_SMOOTH_FROM_BEFORE *********************
 *
 * verify a pitch which has continuity with a preceding set of verified pitches.
 */

int is_smooth_from_before(int n,char *smooth,double max_pglide,float *pitches)
{
    float thispitch, pitch_before;
    double pre_interval;
    int m;
    if((thispitch = pitches[n]) < FLTERR)
        return FALSE;
    for(m=1;m<FMIN_SMOOTH_SET;m++) {        /* If there are (FMIN_SMOOTH_SET-1) smooth pitches before */
        if(!smooth[n-m])
            return(FALSE);
    }
    pitch_before = pitches[n-1];        /* Test the interval with the previous pitch */
    pre_interval  = pitch_before/thispitch;
    if(pre_interval < 1.0)
        pre_interval = 1.0/pre_interval;
    if(pre_interval  > max_pglide)   
        return  FALSE;                  /* And if it's acceptably smooth */
    return  TRUE;                       /* mark this pitch as smooth also */
}

/********************** IS_SMOOTH_FROM_AFTER *********************
 *
 * verify a pitch which has continuity with a following set of verified pitches.
 */

int is_smooth_from_after(int n,char *smooth,double max_pglide,float *pitches)
{
    float thispitch, pitch_after;
    double post_interval;
    int m;
    if((thispitch = pitches[n]) < FLTERR)
        return FALSE;
    for(m=1;m<FMIN_SMOOTH_SET;m++) {    /* If there are (FMIN_SMOOTH_SET-1) smooth pitches after */
        if(!smooth[n+m])
        return(FALSE);
    }
    pitch_after = pitches[n+1];     /* Test the interval with the next pitch */
    post_interval  = pitch_after/thispitch;
    if(post_interval < 1.0)
        post_interval = 1.0/post_interval;
    if(post_interval  > max_pglide)   
        return  FALSE;              /* And if it's acceptably smooth */
    return  TRUE;                   /* mark this pitch as smooth also */
}

/********************** TEST_GLITCH_SETS *********************
 *
 * This function looks for any sets of values that appear to be glitches
 * amongst the real pitch data.
 * It is possible some items are REAL pitch data isolated BETWEEN short glitches.
 * This function checks for these cases.
 */

int test_glitch_sets(char *smooth,double max_pglide,int wlength,float *pitches)
{
    int exit_status;
    int gotglitch = FALSE;
    int n, gltchend, gltchstart = 0;
    for(n=0;n<wlength;n++) {
        if(gotglitch) {         /* if inside a glitch */
            if(smooth[n]) {     /* if reached its end, mark the end, then process the glitch */
                gltchend = n;
                if((exit_status = test_glitch_forwards(gltchstart,gltchend,smooth,max_pglide,pitches))<0)
                    return(exit_status);
                if((exit_status = test_glitch_backwards(gltchstart,gltchend,smooth,max_pglide,wlength,pitches))<0)
                    return(exit_status);
                gotglitch = 0;
            }
        } else {                /* look for a glitch and mark its start */
            if(!smooth[n]) {
                gotglitch = 1;
                gltchstart = n;
            }
        }
    }
    if(gotglitch) {             /* if inside a glitch at end of data, process glitch */
        gltchend = n;
        test_glitch_forwards(gltchstart,gltchend,smooth,max_pglide,pitches);
    }
    return(FINISHED);
}

/********************* REMOVE_UNSMOOTH_PITCHES ***********************
 *
 * delete all pitches which have no verified continuity with surrounding pitches.
 */

void remove_unsmooth_pitches(char *smooth,int wlength,float *pitches)
{
    int n;
    for(n=0;n<wlength;n++) {
        if(!smooth[n])      
            pitches[n] = (float)NOT_PITCH;
    }
}

/********************** TEST_GLITCH_FORWARDS *********************
 *
 * searching from start of glitch, look for isolated true pitches
 * amongst glitch data.
 */

#define LAST_SMOOTH_NOT_SET (-1)

int test_glitch_forwards(int gltchstart,int gltchend,char *smooth,double max_pglide,float *pitches)
{
    int n, glcnt;
    int last_smooth, previous;
    double pre_interval;
    if((previous = gltchstart - 1) < 0)
        return FINISHED;
    if(pitches[previous] < FLTERR) {
        sprintf(errstr,"Error in previous smoothing logic: test_glitch_forwards()\n");
        return(PROGRAM_ERROR);
    }
    last_smooth = previous;
    n = gltchstart+1;                               /* setup params for local search of glitch */
    glcnt = 1;
    while(n < gltchend) {                           /* look through the glitch */
        if(pitches[n] > FLTERR) {                   /* if glitch location holds a true pitch */
            pre_interval = pitches[n]/pitches[previous]; 
            if(pre_interval < 1.0)
                pre_interval = 1.0/pre_interval;    /* compare against previous verified pitch */
            if(pre_interval <= pow(max_pglide,(double)(n-previous))) {
                smooth[n] = TRUE;                   /* if comparable: mark this pitch as verified */
                last_smooth = n;
            }
        }                                           
        n++;                                        /* Once more than a max-glitch-set has been scanned */                                          
                                                    /* or the end of the entire glitch is reached */
        if(++glcnt >= FMIN_SMOOTH_SET || n >= gltchend) {
            if(last_smooth == previous)
                break;                              /* If no new verifiable pitch found, give up */
            previous = last_smooth;
            n = last_smooth + 1;                    /* Otherwise start a new local search from newly verified pitch */
            glcnt = 1;                                  
        }
    }
    return(FINISHED);
}

/********************** TEST_GLITCH_BACKWARDS *********************
 *
 * searching from end of glitch, look for isolated true pitches
 * amongst glitch data.
 */

int test_glitch_backwards(int gltchstart,int gltchend,char *smooth,double max_pglide,int wlength,float *pitches)
{
    int n, glcnt, next, next_smooth;
    double post_interval;
    if((next = gltchend) >= wlength)
        return FINISHED;
    if(pitches[next] < FLTERR) {
        sprintf(errstr,"Error in previous smoothing logic: test_glitch_backwards()\n");
        return(PROGRAM_ERROR);
    }
    next_smooth = next;
    n = gltchend-2;                                 /* setup params for local search of glitch */
    glcnt = 1;
    while(n >= gltchstart) {                        /* look through the glitch */
        if(pitches[n] > FLTERR) {                   /* if glitch location holds a true pitch */
            post_interval = pitches[n]/pitches[next]; 
            if(post_interval < 1.0)
                post_interval = 1.0/post_interval;  /* compare against previous verified pitch */
            if(post_interval <= pow(max_pglide,(double)(next - n))) {
                smooth[n] = TRUE;                   /* if comparable: mark this pitch as verified */
                next_smooth = n;
            }
        }                                           
        n--;                                        /* Once more than a max-glitch-set has been scanned */
                                                    /* or the start of the entire glitch is reached */                                          
        if(++glcnt >= FMIN_SMOOTH_SET || n < gltchstart) {                  
            if(next_smooth == next)
                break;                              /* If no new verifiable pitch found, give up */
            next = next_smooth;
            n = next_smooth - 1;
            glcnt = 1;                              /* Otherwise start a new local search */
        }
    }
    return(FINISHED);
}

//////////////////////////////////
//      INTERPOLATING PITCH     //
//////////////////////////////////

/**************************** INTERPOLATE_PITCH ***************************/

#define TRUE_UNPITCHED   -3
#define SILENCE_BRACKETED 1
#define PITCH_BRACKETED   2
#define PITCH_ONSET       3
#define PITCH_ENDING      4

int interpolate_pitch(dataptr dz)
{
    int exit_status;
    int pitchno, unpitchstart = 0, unpitchcnt = 0, k;
    int min_unpitch_windows = (int)round(TOO_SHORT_UNPICH/dz->frametime);
    int unpitched_eventtype = 0;

    //  Eliminate too short unpitched-bloks appropriately to their context //

    if(dz->retain_unpitched_data_for_deletion) {
        for(pitchno=0;pitchno<dz->wlength;pitchno++) {
            if(flteq((double)dz->pitches[pitchno],NOT_SOUND)) {     //  SILENCE
                if(unpitchcnt == 0)
                    continue;                                       //  If there's an unpitched blok, it's followed by silence
                if(unpitchcnt > min_unpitch_windows) {              //  If unpitched is long enough
                    for(k = unpitchstart;k < pitchno;k++)           //  mark as truly unpitched
                        dz->pitches[k] = TRUE_UNPITCHED;            //  Else
                } else {
                    for(k = unpitchstart;k < pitchno;k++)
                        dz->pitches[k] = NOT_SOUND;                 //  Treat as silence (will be zeroed)
                }
                unpitchcnt = 0;

            } else if(dz->pitches[pitchno] > FSPEC_MINFRQ) {        //  PITCHED
                if(unpitchcnt == 0)
                    continue;
                if(unpitchcnt > min_unpitch_windows) {              //  If unpitched is long enough
                    for(k = unpitchstart;k < pitchno;k++)           //  mark as truly unpitched
                        dz->pitches[k] = TRUE_UNPITCHED;            //  Else
                } else {
                                                                    //  Unpitched blok followed by pitch
                    if (unpitchstart > 0 && (dz->pitches[unpitchstart-1] > FSPEC_MINFRQ))
                        unpitched_eventtype = PITCH_BRACKETED;      //  Bracketed by pitch
                    else                                            //  OR
                        unpitched_eventtype = PITCH_ONSET;          //  introduces pitched material (material before unpitched must have been silence)
                    switch(unpitched_eventtype) {
                    case(PITCH_ONSET):
                        for(k = unpitchstart;k < pitchno;k++)
                            dz->pitches[k] = NOT_SOUND;             //  Treat as silence (will be zeroed)
                        break;
                    case(PITCH_BRACKETED):                          //  will be interpolated to give a pitch value
                        break;
                    }
                }
                unpitchcnt = 0;
            } else {                                                //  UNPITCHED
                if(unpitchcnt == 0)
                    unpitchstart = pitchno;
                unpitchcnt++;
            }
        }

        //  Deal with any unpitched blok at end of file

        if(unpitchcnt > 0) {
            if(unpitchcnt > min_unpitch_windows) {                  //  If unpitched is long enough
                for(k = unpitchstart;k < dz->wlength;k++)           //  mark as truly unpitched
                    dz->pitches[k] = TRUE_UNPITCHED;
            } else {                                                //  Else
                if (unpitchstart > 0 && (dz->pitches[unpitchstart-1] >= FSPEC_MINFRQ))
                    unpitched_eventtype = PITCH_ENDING;                 //  Preceded by pitch
                else                                                //  OR
                    unpitched_eventtype = SILENCE_BRACKETED;        //  material before unpitched must have been silence
                switch(unpitched_eventtype) {
                case(SILENCE_BRACKETED):
                    for(k = unpitchstart;k < pitchno;k++)
                        dz->pitches[k] = NOT_SOUND;                 //  Treat as silence (will be zeroed)
                    break;
                case(PITCH_ENDING):                                 //  will be interpolated to give a pitch value
                    break;
                }
            }
        }
    }

    //  We now have a file with (1) PITCH, (2) Interpolatable stretches of NON_PICH, (3) TRUE_UNPITCHED and (4) NOT_SOUND

    for(pitchno=0;pitchno<dz->wlength;pitchno++) {
        if(dz->pitches[pitchno] < FSPEC_MINFRQ) {
            if(dz->retain_unpitched_data_for_deletion && dz->pitches[pitchno] <= NOT_SOUND)
                continue;
            if((exit_status = do_interpolating(&pitchno,dz->retain_unpitched_data_for_deletion,dz))<0)
                return(exit_status);
        }
    }

    //  Restore the NOT_PITCH flags to the material which is TRUE_UNPITCHED     so we have a file with pitch, non-pitch (-1) and not_sound (-2)

    if(dz->retain_unpitched_data_for_deletion) {
        for(pitchno=0;pitchno<dz->wlength;pitchno++) {
            if(flteq((double)dz->pitches[pitchno],TRUE_UNPITCHED))
                dz->pitches[pitchno] = NOT_PITCH;
        }
    }
    return(FINISHED);
}

/****************************** DO_INTERPOLATING *************************/

int do_interpolating(int *pitchno,int retain_unpitched_data_for_deletion,dataptr dz)
{
    int act_type = FMID_PITCH;
    int start = *pitchno, m;
    double startpitch, endpitch, thispitch, lastpitch, pstep;
    if(*pitchno==0L)                            //  Window is at start of file
        act_type = FFIRST_PITCH;            
    while(dz->pitches[*pitchno] < FSPEC_MINFRQ) {   //  If window is unpitched, advance until we find a pitched window
        if(++(*pitchno)>=dz->wlength) {         //  IF we reach end of file without finding pitched window
            if(act_type == FFIRST_PITCH)        //  if we started search at file start,
                act_type = FNO_PITCH;           //  there is no pitch to find in the file.
            else
                act_type = FEND_PITCH;          //  otherwise this is an unpitched blok at file end
            break;
        }
    }
    if(act_type==FMID_PITCH) {                  //  As default, this is an unpitched blok in mid file
        m = start-1;                            //  search backwards for a valid pitch
        while(dz->pitches[m] < FSPEC_MINFRQ) {
            if(--m <= 0) {                      //  and if we reach file start without finding one
                act_type = FFIRST_PITCH;        //  this is an unpitched blok at file start
                break;
            }
        }
        start = m;
    }
    switch(act_type) {
    case(FMID_PITCH):
        startpitch = hz_to_pitchheight((double)dz->pitches[start-1]);
        endpitch   = hz_to_pitchheight((double)dz->pitches[*pitchno]);
        pstep = (endpitch - startpitch)/(double)((*pitchno) - (start - 1));
        lastpitch = startpitch;
        for(m=start;m<*pitchno;m++) {           // Interp pitch across unpitched seg
            thispitch  = lastpitch + pstep;
            if(!(retain_unpitched_data_for_deletion && dz->pitches[m]<=NOT_SOUND))      //  i.e. If we are retaining truly non-pitched material and silence
                dz->pitches[m] = (float)pitchheight_to_hz(thispitch);       //  and this is such material, don't interp!!
            lastpitch = thispitch;
        }
        break;
    case(FFIRST_PITCH):
        for(m=0;m<*pitchno;m++) {               // Extend first clear pitch back to start
            if(!(retain_unpitched_data_for_deletion && (dz->pitches[m]<=NOT_SOUND)))
                dz->pitches[m] = dz->pitches[*pitchno];
        }
        break;
    case(FEND_PITCH):                           // Extend last clear pitch on to end
        for(m=start;m<*pitchno;m++) {
            if(!(retain_unpitched_data_for_deletion && (dz->pitches[m]<=NOT_SOUND)))
                dz->pitches[m] = dz->pitches[start-1];
        }
        break;
    case(FNO_PITCH): 
        sprintf(errstr,"No valid pitch found.\n");
        return(GOAL_FAILED);
    }
    (*pitchno)--;                               //  set next interpolation to start at end of this interpolated block
    return(FINISHED);
}

/***************************** HZ_TO_PITCHHEIGHT *******************************
 *
 * Real pitch is 12 * log2(frq/basis_frq).
 * 
 * BUT (with a little help from the Feynman lectures!!)
 * (1)  The basis_frq is arbitrary, and cancels out, so let it be 1.0.
        i.e. pitch1 = 12 * log2(frq1/basis_frq) = 12 * (log2(frq1) - log2(basis_frq));
             pitch2 = 12 * log2(frq2/basis_frq) = 12 * (log2(frq2) - log2(basis_frq));
             pitch1 - pitch2 = 12 * (log2(frq1) - log2(frq2)) = 12 * log2(frq1/frq2);
 * (2)  Finding the difference of 2 log2() numbers, interpolating and
 *      reconverting to pow(2.0,...) is no different to doing same
 *      calculation to base e.
 * (3)  The (12 *) is also a cancellable factor in all this.
 *      So pitch_height serves the same function as pitch in these calculations!!
 */

double hz_to_pitchheight(double frqq)
{   
    return log(frqq);
}

/***************************** PITCHHEIGHT_TO_HZ *******************************/

double pitchheight_to_hz(double pitch_height)
{
    return exp(pitch_height);
}

/************************* LOCATE CHANNEL OF FUNDAMENTAL *********************/

int locate_channel_of_fundamental(int inner_lpcnt,float *fundfrq, int *newcc,dataptr dz)
{
    double srchbot, srchtop, fundamp, ferror, frq, subfrq = HUGE, top;
    int cc, vc, subchan = 0;
    *fundfrq = (float)dz->pitches[inner_lpcnt];
    if(*fundfrq < 0.0) 
        return FINISHED;
    srchbot = *fundfrq * SEVEN_OVER_EIGHT;
    srchtop = *fundfrq * EIGHT_OVER_SEVEN;
    ferror = HUGE;
    if(inner_lpcnt == 0) {                              //  Window zero has no meaningful data
        frq = 0.0;                                      //  search using centre and top frqs of frq bins
        top = dz->halfchwidth;
        for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
            if(top < srchbot) {
                subfrq = frq;                           //  Remember chan-frq and number of the channel before fundamental (should be) found
                subchan = cc;
            } else if(frq < srchtop) {                  //  For any channel in range, find channel-centre closest to fundamental
                if(fabs(*fundfrq - frq) < ferror) {
                    ferror = fabs(*fundfrq - frq);
                    *newcc = cc;
                }
            } else if(ferror == HUGE) {                 //  If no frequencies within range of fundamental (should be impossible!!), keep closest
                if(fabs(frq - *fundfrq) < fabs(*fundfrq - subfrq))
                    *newcc = cc;                        //  If chan-frq above searched-for range is closer than one below, use its channel
                else
                    *newcc = subchan;                   //  else use channel below
                break;
            }
            frq += dz->chwidth;
            top += dz->chwidth;
        }
    } else {                                            //  Normal case
        fundamp = -HUGE;
        for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
            frq = dz->flbufptr[0][FREQ];
            if(frq < srchbot) {
                subfrq = frq;                           //  Remember frq and channo in channel before fundamental (should be) found
                subchan = cc;
            } else if(frq < srchtop) {          
                if(dz->flbufptr[0][AMPP] > fundamp) {   //  If (frqs near) fundamental found, find loudest occurence
                    fundamp = dz->flbufptr[0][AMPP];
                    *newcc = cc;
                }
            } else if(fundamp == -HUGE) {               //  If no frequencies within range of fundamental, keep closest in frq
                if(subfrq == HUGE)
                    *newcc = cc;                        //  If there is no subfrq, use the frq in chan above range
                if(fabs(frq - *fundfrq) < fabs(*fundfrq - subfrq))
                    *newcc = cc;                        //  If frq above is closer than one below, use its channel
                else
                    *newcc = subchan;                   //  else use channel below
                break;
            }
        }
    }
    return FINISHED;
}

/************************* LOCATE_CHANNEL_OF_PITCH *********************/

int locate_channel_of_pitch(int inner_lpcnt,float thepitch, int *newcc,dataptr dz)
{
    double srchbot, srchtop, fundamp, ferror, frq, subfrq = HUGE, top;
    int cc, vc, subchan = 0;
    if(thepitch < 0.0) {
        sprintf(errstr,"This process needs interpolated pitch data: the extracted pitchdata still has pitch-zeros or silence-zeros.\n");
        return USER_ERROR;
    }
    srchbot = thepitch * SEVEN_OVER_EIGHT;
    srchtop = thepitch * EIGHT_OVER_SEVEN;
    ferror = HUGE;
    if(inner_lpcnt == 0) {                              //  Window zero has no meaningful data
        frq = 0.0;                                      //  search using centre and top frqs of frq bins
        top = dz->halfchwidth;
        for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {       //RWD was vc+2
            if(top < srchbot) {
                subfrq = frq;                           //  Remember chan-frq and number of the channel before fundamental (should be) found
                subchan = cc;
            } else if(frq < srchtop) {                  //  For any channel in range, find channel-centre closest to fundamental
                if(fabs(thepitch - frq) < ferror) {
                    ferror = fabs(thepitch - frq);
                    *newcc = cc;
                }
            } else if(ferror == HUGE) {                 //  If no frequencies within range of fundamental (should be impossible!!), keep closest
                if(fabs(frq - thepitch) < fabs(thepitch - subfrq))
                    *newcc = cc;                        //  If chan-frq above searched-for range is closer than one below, use its channel
                else
                    *newcc = subchan;                   //  else use channel below
                break;
            }
            frq += dz->chwidth;
            top += dz->chwidth;
        }
    } else {                                            //  Normal case
        fundamp = -HUGE;
        for(cc=0,vc=0;cc<dz->clength;cc++,vc+=2) {
            frq = dz->flbufptr[0][FREQ];
            if(frq < srchbot) {
                subfrq = frq;                           //  Remember frq and channo in channel before fundamental (should be) found
                subchan = cc;
            } else if(frq < srchtop) {          
                if(dz->flbufptr[0][AMPP] > fundamp) {   //  If (frqs near) fundamental found, find loudest occurence
                    fundamp = dz->flbufptr[0][AMPP];
                    *newcc = cc;
                }
            } else if(fundamp == -HUGE) {               //  If no frequencies within range of fundamental, keep closest in frq
                if(subfrq == HUGE)
                    *newcc = cc;                        //  If there is no subfrq, use the frq in chan above range
                if(fabs(frq - thepitch) < fabs(thepitch - subfrq))
                    *newcc = cc;                        //  If frq above is closer than one below, use its channel
                else
                    *newcc = subchan;                   //  else use channel below
                break;
            }
        }
    }
    return FINISHED;
}

/************************************ CONTINUITY_SMOOTHING ***************************/

int continuity_smoothing(dataptr dz)
{
    double *running_average = dz->parray[RUNNINGAVRG];
    double *runavdur        = dz->parray[RUNAVGDURS];
    int *avstart            = dz->lparray[AVSTT];
    int runavcnt = 0, lastrunavcnt, n, maxavg, ahead, behind, bracketed, thisbracketed;
    double ratio, maxtime, bracketdur = 0.0;
    int done;

    memset((char *)running_average,0,dz->wlength * sizeof(double));
    memset((char *)runavdur,0,dz->wlength * sizeof(double));
    memset((char *)avstart,0,dz->wlength * sizeof(int));

    //  FIND BLOCKS WHOSE RUNNING AVERAGES DIFFER

    for(n = 0; n < dz->wlength;n++) {
        if(dz->pitches[n] < 0.0)                                    //  Don't weigh time of non-pitch areas
            continue;
        if(running_average[runavcnt] == 0.0) {                      //  First running average will not have a start value
            avstart[runavcnt] = n;
            running_average[runavcnt] = dz->pitches[n];
        } else {
            if(dz->pitches[n] > running_average[runavcnt])
                ratio = dz->pitches[n]/running_average[runavcnt];
            else
                ratio = running_average[runavcnt]/dz->pitches[n];
            if(ratio < ALMOST_TWO)                                  //  If next pitch is close to running-average, continue to average, and sum duration
                running_average[runavcnt] = (running_average[runavcnt] + dz->pitches[n])/2;
            else {                                                  //  IF not, start a new average
                runavcnt++;
                avstart[runavcnt] = n;
                running_average[runavcnt] = dz->pitches[n];
            }
        }
        runavdur[runavcnt] += dz->frametime;
    }
    runavcnt++;
    if(runavcnt < 2)
        return FINISHED;
    lastrunavcnt = runavcnt;
    do {
        //  FIND THE LONGEST BLOCK (Aread with no apparent pitch are not counted)

        maxtime = 0.0;
        maxavg = 0;
        for(n = 0;n<runavcnt;n++) {
            if(runavdur[n] > maxtime) {
                maxtime = runavdur[n];
                maxavg  = n;
            }
        }

        //  WORKING OUTWARDS FROM THE LONGEST BLOCK, ELIMINATE BLOCKS WHICH ARE SUDDENLY DIFFERENT, FOR TOO SHORT A TIME

        if(maxavg < runavcnt-2) {
            
            for(n = maxavg; n < runavcnt;n++) {                                 //  working upwards from the most persistent average
                ahead = n+2;                                                    //  find if the bracketing bloks of the next blok have similar average
                bracketed = n+1;
                if(ahead >= runavcnt)
                    break;
                bracketdur = 0.0;
                done = 0;
                if(running_average[n] > running_average[ahead])
                    ratio = running_average[n]/running_average[ahead];
                else
                    ratio = running_average[ahead]/running_average[n];
                bracketdur = runavdur[bracketed];
                if(ratio < ALMOST_TWO) {                                        //  If 2 bracketing averages are similar
                    if (runavdur[bracketed] < TOO_SHORT_PICH) {                 //  & intervening block is short
                        averages_smooth(bracketed,n,runavcnt,dz);               //  Smooth the intervening block
                        n++;                                                    //  and skip over the intervening (smoothed) block, and continue
                    } else                                                      
                        continue;                                               //  But if not short, proceed normally along the array

                } else {                                                        //  If 2 bracketing averages are not similar,
                    while(runavdur[bracketed]<TOO_SHORT_PICH) {                 //  we have at least 3 contiguous but different averages
                        ahead++;                                                //  proceed to wider bracket
                        bracketed++;
                        if(ahead >= runavcnt) {                                 //  until we find a matching pair of bracketing averages        
                            done = 1;
                            break;
                        }
                        bracketdur += runavdur[bracketed];
                        if(bracketdur > TOO_SHORT_PICH)                         //  If total duration of bracketed items no longer short, go back to outer loop
                            break;
                        if(running_average[n] > running_average[ahead])
                            ratio = running_average[n]/running_average[ahead];
                        else
                            ratio = running_average[ahead]/running_average[n];
                        if(ratio < ALMOST_TWO) {                                //  If we finally find a valid outer bracket
                                                                                //  (Total duration of things within bracket is still very short)
                            for(thisbracketed = n+1;thisbracketed < ahead;thisbracketed++) 
                                averages_smooth(thisbracketed,n,runavcnt,dz);   //  Smooth ALL the intervening block                    
                            n += (ahead-n)-1;                                   //  We want to advance to "ahead": loop will force advance of 1.
                            break;
                        }                                                       //  so advance here by distance from n to ahead (= ahead-n) minus 1

                    }                                                           //  Otherwise we have found yet another non-matching block
                }                                                               //  continue to assemble bracketed items
                if(done)
                    break;
            }
        } else                                                                  //  Either maxavg is at the v. end (so no end to check)
            n = maxavg;                                                         //  Or it is at the penultimate position, so need to check last blok

        //  CHECK THE LAST BLOCK, IF NOT ALREADY CHECKED
        
        if(n == runavcnt-2) {
            bracketed = n+1; 
            if(running_average[bracketed] > running_average[n])
                ratio = running_average[bracketed]/running_average[n];
            else
                ratio = running_average[n]/running_average[bracketed];
            if(ratio < ALMOST_TWO && runavdur[bracketed] < TOO_SHORT_PICH)
                averages_smooth(bracketed,n,runavcnt,dz);
        }
        
        //  NOW PROCEED BACKWARDS FROM LONGEST BLOK

        if(maxavg > 1) {

            for(n = maxavg; n >= 0; n--) {                                      //  working backwards from the most persistent average      
                behind = n-2;
                bracketed = n-1;
                if(behind < 0)
                    break;
                bracketdur = 0.0;
                done = 0;
                if(running_average[n] > running_average[behind])            
                    ratio = running_average[n]/running_average[behind];     
                else                                                    
                    ratio = running_average[behind]/running_average[n]; 
                if(ratio < ALMOST_TWO) {                                        //  If 2 bracketing averages are similar
                    if(runavdur[bracketed] < TOO_SHORT_PICH) {                  //  & intervening block is short
                        averages_smooth(bracketed,n,runavcnt,dz);               //  Smooth the intervening block
                            n--;                                                //  and skip over the intervening (smoothed) block, and continue
                        } else
                            continue;                                           //  But if not short, proceed normally along the array

                } else {                                                        //  If 2 bracketing averages are not similar,
                    while(runavdur[bracketed]<TOO_SHORT_PICH) {                 //  we have at least 3 contiguous but different averages
                        behind--;                                               //  proceed to wider bracket
                        bracketed--;
                        if(behind <= 0) {                                       //  until we find a matching pair of bracketing averages        
                            done = 1;
                            break;
                        }
                        bracketdur += runavdur[bracketed];
                        if(bracketdur > TOO_SHORT_PICH)                         //  If total duration of bracketed items no loner short, go back to outer loop
                            break;
                        if(running_average[n] > running_average[behind])
                            ratio = running_average[n]/running_average[behind];
                        else
                            ratio = running_average[behind]/running_average[n];
                        if(ratio < ALMOST_TWO) {                                //  If we finally find a valid outer bracket
                                                                                //  (Total duration of things within bracket is still very short)
                            for(thisbracketed = n-1;thisbracketed > behind;thisbracketed--) 
                                averages_smooth(thisbracketed,n,runavcnt,dz);   //  Smooth ALL the intervening block                    
                            n -= (n-behind)-1;                                  //  We want to regress to "behind": loop will force regress of 1, so advance here by "ahead-1"
                            break;                                              //  so regress here by distance from n to behind (= n-behind) minus 1
                        }                                                       // ang go back to outer loop

                    }                                                           //  Otherwise we have found yet another non-matching block
                }                                                               //  continue to assemble bracketed items
                if(done)
                    break;
            }
        }

        //  CHECK THE FIRST BLOCK IF NESS

        if(n == 1) {
            bracketed = 0;
            if(running_average[bracketed] > running_average[n])
                ratio = running_average[bracketed]/running_average[n];
            else
                ratio = running_average[n]/running_average[bracketed];
            if(ratio < ALMOST_TWO && runavdur[bracketed] < TOO_SHORT_PICH)
                averages_smooth(bracketed,n,runavcnt,dz);
        }

        //  NOW RECURSIVELY SMOOTH ON NEW VALUES UNTIL NO MORE SMOOTHING POSSIBLE

        memset((char *)running_average,0,dz->wlength * sizeof(double));
        memset((char *)runavdur,0,dz->wlength * sizeof(double));
        memset((char *)avstart,0,dz->wlength * sizeof(int));

        lastrunavcnt = runavcnt;
        runavcnt = 0;
        for(n = 0; n < dz->wlength;n++) {
            if(dz->pitches[n] < 0.0)
                continue;
            if(running_average[runavcnt] == 0.0) {
                avstart[runavcnt] = n;
                running_average[runavcnt] = dz->pitches[n];
            } else {
                if(dz->pitches[n] > running_average[runavcnt])
                    ratio = dz->pitches[n]/running_average[runavcnt];
                else
                    ratio = running_average[runavcnt]/dz->pitches[n];
                if(ratio < ALMOST_TWO)
                    running_average[runavcnt] = (running_average[runavcnt] + dz->pitches[n])/2;
                else {
                    runavcnt++;
                    avstart[runavcnt] = n;
                    running_average[runavcnt] = dz->pitches[n];
                }
            }
            runavdur[runavcnt] += dz->frametime;
        }
        runavcnt++;
        if(runavcnt < 2)
            break;
    } while(runavcnt < lastrunavcnt);

    return FINISHED;
}

/************************************ AVERAGES_SMOOTH ***************************/

int averages_smooth(int bracketed,int bracket,int runavcnt,dataptr dz)
{
    double new_running_average, ratio, newratio;
    int k, avend;
    int *avstart = dz->lparray[AVSTT];
    double *running_average = dz->parray[RUNNINGAVRG];

    if(bracketed+1 == runavcnt)
        avend = dz->wlength;
    else
        avend = avstart[bracketed+1];
        
    if(running_average[bracketed] > running_average[bracket]) {     //  If bracketed material too high
        do {
            new_running_average = 0.0;
            for(k = avstart[bracketed]; k < avend;k++) {
                if(dz->pitches[k] > 0.0) {                          //  8va transpose-down the bracketed material
                    dz->pitches[k] /= 2.0;
                    if(new_running_average == 0.0)
                        new_running_average = dz->pitches[k];
                    else
                        new_running_average = (new_running_average + dz->pitches[k])/2.0;   
                }
            }                                           
            if(new_running_average > running_average[bracket]) {    //  If we're still higher than the goal
                ratio = new_running_average/running_average[bracket];
                if(ratio < ALMOST_TWO) {                            //  But we're now in acceptable range
                    new_running_average = 0.0;  
                    for(k = avstart[bracketed]; k < avend;k++) {    //  try the next 8va down
                        if(dz->pitches[k] > 0.0) {
                            dz->pitches[k] /= 2.0;
                            if(new_running_average == 0.0)
                                new_running_average = dz->pitches[k];
                            else
                                new_running_average = (new_running_average + dz->pitches[k])/2.0;   
                        }
                    }
                    if(new_running_average > running_average[bracket])
                        newratio = new_running_average/running_average[bracket];
                    else
                        newratio = running_average[bracket]/new_running_average;
                    if(newratio > ratio) {                          //  if the new material is further away from the goal than originally
                        for(k = avstart[bracketed]; k < avend;k++) {
                            if(dz->pitches[k] > 0.0)
                                dz->pitches[k] *= 2.0;              //  restore to original 8va
                        }
                    }
                    break;                                          //  Now quit with new pitch
                }
            } else                                                  //  Else we.re mow lower than the goal, so at nearest
                break;                                              //  
        } while(ratio > ALMOST_TWO);

    } else {                                                        //  Else bracketed material is too low

        do {
            new_running_average = 0.0;
            for(k = avstart[bracketed]; k < avend;k++) {
                if(dz->pitches[k] > 0.0) {
                    dz->pitches[k] *= 2.0;                          //  8va transpose-up the bracketed material
                    if(new_running_average == 0.0)
                        new_running_average = dz->pitches[k];
                    else
                        new_running_average = (new_running_average + dz->pitches[k])/2.0;
                }
            }
            if(new_running_average < running_average[bracket]) {    //  If we're still lower than the goal
                ratio = running_average[bracket]/new_running_average;
                if(ratio < ALMOST_TWO) {                            //  But we're now in acceptable range
                    new_running_average = 0.0;  
                    for(k = avstart[bracketed]; k < avend;k++) {
                        if(dz->pitches[k] > 0.0) {
                            dz->pitches[k] *= 2.0;                  //  try the next 8va up
                            if(new_running_average == 0.0)
                                new_running_average = dz->pitches[k];
                            else
                                new_running_average = (new_running_average + dz->pitches[k])/2.0;   
                        }
                    }
                    if(new_running_average > running_average[bracket])
                        newratio = new_running_average/running_average[bracket];
                    else
                        newratio = running_average[bracket]/new_running_average;
                    if(newratio > ratio) {                          //  if the new material is further away from the goal than originally
                        for(k = avstart[bracketed]; k < avend;k++) {
                            if(dz->pitches[k] > 0.0)
                                dz->pitches[k] /= 2.0;              //  restore to original 8va
                        }
                    }
                    break;
                }
            } else                                                  //  Else we.re now higher than the goal, so at nearest
                break;
        } while(ratio > ALMOST_TWO);
    }
    return FINISHED;
}

/************************************ EXCLUDE_NON_HARMONICS ***************************/

int exclude_non_harmonics(dataptr dz)
{
    dz->xclude_nonh = 0;
    switch(dz->mode) {
    case(F_NARROW):   if(dz->vflag[NRW_XNH])        dz->xclude_nonh = 1;    break;
    case(F_SQUEEZE):  if(dz->vflag[SQZ_XNH])        dz->xclude_nonh = 1;    break;
    case(F_NEGATE):   if(dz->vflag[NEG_XNH])        dz->xclude_nonh = 1;    break;
    case(F_MOVE):     if(dz->vflag[MOV_XNH])        dz->xclude_nonh = 1;    break;
    case(F_MOVE2):    if(dz->vflag[MOV2_XNH])       dz->xclude_nonh = 1;    break;
    case(F_INVERT):   if(dz->vflag[INVERT_XNH])     dz->xclude_nonh = 1;    break;
    case(F_ROTATE):   if(dz->vflag[ROTATE_XNH])     dz->xclude_nonh = 1;    break;
    case(F_SUPPRESS): if(dz->vflag[SUPPRESS_XNH])   dz->xclude_nonh = 1;    break;
    case(F_ARPEG):      //  fall thro
    case(F_OCTSHIFT):   //  fall thro
    case(F_TRANS):      //  fall thro
    case(F_FRQSHIFT):   //  fall thro
    case(F_RESPACE):    //  fall thro
    case(F_PINVERT):    //  fall thro
    case(F_PEXAGG):     //  fall thro
    case(F_PQUANT):     //  fall thro
    case(F_PCHRAND):    //  fall thro
    case(F_RAND):     if(dz->vflag[RECOLOR_XNH])    dz->xclude_nonh = 1;    break;

    }
    if(dz->xclude_nonh) {
        if((dz->fptr[HMNICBOUNDS] = (float *)malloc(10 * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store harmonic peak boundaries.\n");
            return(MEMORY_ERROR);
        }
    }
    return FINISHED;
}

/************************************ ZERO_NON_PARTIALS ***************************/

#define ABOVE_NYQUIST 0

int channel_holds_harmonic(float the_fundamental,float frq,int cc,int *top_hno,dataptr dz) 
{
    float *harmbounds = dz->fptr[HMNICBOUNDS], this_harmonic_frq, lobnd, upbnd;
    int mid = 3, bndpairscnt = 5, boundscnt = bndpairscnt * 2;
    int k, hno, lower, upper;
    if(cc == 0) {
        hno = 1;
        for(k=0,lower=0,upper=1;k < bndpairscnt;k++,lower+=2,upper+=2) {
            this_harmonic_frq = (float)(the_fundamental * hno);     //  lo1 hi1     lo2 hi2     lo3 hi3     lo4 hi4     lo5 hi5
            lobnd = (float)(this_harmonic_frq * SEMITONE_DOWN);     //  |   |       |   |       |   |       |   |       |   |
            upbnd = (float)(this_harmonic_frq * SEMITONE_INTERVAL); //  0   1       2   3       4   5       6   7       8   9
            harmbounds[lower] = lobnd;
            harmbounds[upper] = upbnd;
            hno++;
        }
        *top_hno = bndpairscnt;

    }
    if(frq > harmbounds[mid])
        if(shufflup_harmonics_bounds(top_hno,the_fundamental,boundscnt,dz)== ABOVE_NYQUIST)
            return 0;

    for(k=0,hno = 1,lower=0,upper=1;k < bndpairscnt;k++,hno++,lower+=2,upper+=2) {
        if(frq > harmbounds[lower]) {
            if(frq < harmbounds[upper])                         //  Within frq bounds of a harmonic
                return 1;
            if(lower < boundscnt - 2) {
                if(frq < harmbounds[lower+2]) {                 //  Outside frq bounds of a harmonic
                    return 0;
                }
            } else {                                            //  Outside top harmonic boUnds
                if(shufflup_harmonics_bounds(top_hno,the_fundamental,boundscnt,dz)==ABOVE_NYQUIST)
                    return 0;                                   //  get next harmonic, and its bounds
                k--;
                lower -= 2;
                upper -= 2;
            }
        }
    }
    return 0;       //  SHOULD BE "NOT REACHED" !!!
}

/************************************ SHUFFLUP_HARMONICS_BOUNDS ***************************/

int shufflup_harmonics_bounds(int *top_hno,float the_fundamental,int boundscnt,dataptr dz)
{
    float *harmbounds = dz->fptr[HMNICBOUNDS];
    float this_harmonic_frq, lobnd, upbnd;
    int k;
    (*top_hno)++;                                           //      lo2 hi2     lo3 hi3     lo4 hi4     lo5 hi5
    this_harmonic_frq = (float)the_fundamental * (*top_hno);//      |   |       |   |       |   |       |   |       |   |
    for(k=2;k < boundscnt;k++)                              //      0   1       2   3       4   5       6   7       8   9
        harmbounds[k-2] = harmbounds[k];                    
    if(harmbounds[0] > dz->nyquist)
        return ABOVE_NYQUIST;
    lobnd = (float)(this_harmonic_frq * SEMITONE_DOWN);     //  Insert boundaries of next harmonic
    upbnd = (float)(this_harmonic_frq * SEMITONE_INTERVAL); //      lo2 hi2     lo3 hi3     lo4 hi4     lo5 hi5     lo6 hi6
    harmbounds[boundscnt - 2] = lobnd;                      //      |   |       |   |       |   |       |   |       |   |
    harmbounds[boundscnt - 1] = upbnd;                      //      0   1       2   3       4   5       6   7       8   9
    return 1;
}


/***************************************** INITIALISE_CONTOURENV ***************************/

int initialise_contourenv(int *contourcnt,int contour_halfbands_per_step,dataptr dz)
{
    int arraycnt, contourbands;
    double frqstep, thisfrq, nextfrq;
    int k = 0;
    contourbands = (int)ceil(dz->clength / (contour_halfbands_per_step / 2));
    arraycnt = contourbands + 2;
    if((dz->fptr[CONTOURFRQ] = (float *)malloc((arraycnt) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for contour frq array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->fptr[CONTOURPCH] = (float *)malloc((arraycnt) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for contour pitch array.\n");
        return(MEMORY_ERROR);
    }
    /*RWD  zero the data */
    memset((char *)dz->fptr[CONTOURPCH],0,arraycnt * sizeof(float));

    if((dz->fptr[CONTOURAMP] = (float *)malloc((arraycnt) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for contour amplitude array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->fptr[CONTOURTOP] = (float *)malloc((arraycnt) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for contour frq limit array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->fptr[CONTOURAMP2] = (float *)malloc((arraycnt) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for contour amplitude array.\n");
        return(MEMORY_ERROR);
    }

    dz->fptr[CONTOURAMP][0] = (float)0.0;
    dz->fptr[CONTOURFRQ][0] = (float)1.0;
    dz->fptr[CONTOURPCH][0] = (float)log10(dz->fptr[CONTOURFRQ][0]);
    dz->fptr[CONTOURTOP][0] = (float)dz->halfchwidth;
    frqstep = dz->halfchwidth * (double)contour_halfbands_per_step;
    thisfrq = dz->fptr[CONTOURTOP][0];
    k = 1;
    while((nextfrq = thisfrq + frqstep) < dz->nyquist) {
        if(k >= arraycnt) {
            sprintf(errstr,"Formant array too small: when setting the contour frqs\n");
            return(PROGRAM_ERROR);
        }
        dz->fptr[CONTOURFRQ][k] = (float)nextfrq;
        dz->fptr[CONTOURPCH][k] = (float)log10(dz->fptr[CONTOURFRQ][k]);
        dz->fptr[CONTOURTOP][k] = (float)min(dz->nyquist,nextfrq);
        thisfrq           = nextfrq;
        k++;
    }
    dz->fptr[CONTOURFRQ][k] = (float)dz->nyquist;
    dz->fptr[CONTOURPCH][k] = (float)log10(dz->nyquist);
    dz->fptr[CONTOURTOP][k] = (float)dz->nyquist;
    dz->fptr[CONTOURAMP][k] = (float)0.0;
    k++;
    *contourcnt = k;
    return(FINISHED);
}

/**************************** GETCONTOURAMP *************************/

int getcontouramp(double *thisamp,int contourcnt,double thisfrq,dataptr dz)
{
    double pp, ratio, ampdiff;
    float *contourpch = dz->fptr[CONTOURPCH], *contouramp = dz->fptr[CONTOURAMP];
    int z = 1;
    if(thisfrq<0.0) {   /* NOT SURE THIS IS CORRECT */
        *thisamp = 0.0; /* SHOULD WE PHASE INVERT & RETURN A -ve AMP ?? */
        return(FINISHED);   
    }
    if(thisfrq<=1.0)
        pp = 0.0;
    else
        pp = log10(thisfrq); 
    while(contourpch[z] < pp){
        z++;
        /*RWD may need to trap on size of array? */
        if(z == contourcnt - 1)
            break;
    }
    ratio    = (pp - contourpch[z-1])/(contourpch[z] - contourpch[z-1]);
    ampdiff  = contouramp[z] - contouramp[z-1];
    *thisamp = contouramp[z-1] + (ampdiff * ratio);
    *thisamp = max(0.0,*thisamp);
    return(FINISHED);
}

/**************************** GETCONTOURAMP2 *************************/

int getcontouramp2(double *thisamp,int contourcnt,double thisfrq,dataptr dz)
{
    double pp, ratio, ampdiff;
    float *contourpch = dz->fptr[CONTOURPCH], *contouramp2 = dz->fptr[CONTOURAMP2];
    int z = 1;
    if(thisfrq<0.0) {   /* NOT SURE THIS IS CORRECT */
        *thisamp = 0.0; /* SHOULD WE PHASE INVERT & RETURN A -ve AMP ?? */
        return(FINISHED);   
    }
    if(thisfrq<=1.0)
        pp = 0.0;
    else
        pp = log10(thisfrq); 
    while(contourpch[z] < pp){
        z++;
        /*RWD may need to trap on size of array? */
        if(z == contourcnt - 1)
            break;
    }
    ratio    = (pp - contourpch[z-1])/(contourpch[z] - contourpch[z-1]);
    ampdiff  = contouramp2[z] - contouramp2[z-1];
    *thisamp = contouramp2[z-1] + (ampdiff * ratio);
    *thisamp = max(0.0,*thisamp);
    return(FINISHED);
}

/*********************** REMEMBER_CONTOURAMP ****************/

int remember_contouramp(int contourcnt,dataptr dz)
{
    int n;
    float *contouramp2 = dz->fptr[CONTOURAMP2], *contouramp = dz->fptr[CONTOURAMP];
    if(contouramp2 == NULL) {
        sprintf(errstr,"Contour amp 2nd array has not been mallocd.\n");
        return PROGRAM_ERROR;
    }
    for(n = 0;n < contourcnt;n++)
        contouramp2[n] = contouramp[n];
    return FINISHED;
}

/********************** EXTRACT_CONTOUR *******************/

#define CHAN_SRCHRANGE_F (4)

int extract_contour(int contourcnt,dataptr dz)
{
    int    n, cc, vc, contourcnt_less_one;
    int    botchan, topchan;
    double botfreq, topfreq;
    double bwidth_in_chans;
    float *contouramp = dz->fptr[CONTOURAMP];
    memset((char *)contouramp,0,contourcnt * sizeof(float));
    contourcnt_less_one = contourcnt - 1;
    vc = 0;
    for(n=0;n<contourcnt;n++)
        contouramp[n] = (float)0.0;
    topfreq = 0.0f;
    n = 0;
    while(n < contourcnt_less_one) {
        botfreq  = topfreq;
        botchan  = (int)((botfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
        botchan -= CHAN_SRCHRANGE_F;
        botchan  =  max(botchan,0);
        topfreq  = dz->specenvtop[n];
        topchan  = (int)((topfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
        topchan += CHAN_SRCHRANGE_F;
        topchan  =  min(topchan,dz->clength);
        for(cc = botchan,vc = botchan * 2; cc < topchan; cc++,vc += 2) {
            if(dz->flbufptr[0][FREQ] >= botfreq && dz->flbufptr[0][FREQ] < topfreq)
                contouramp[n] = (float)(contouramp[n] + dz->flbufptr[0][AMPP]);
        }
        bwidth_in_chans   = (double)(topfreq - botfreq)/dz->chwidth;
        contouramp[n] = (float)(contouramp[n]/bwidth_in_chans);
        n++;
    }
    return(FINISHED);
}

/************************************ SUPPRESS_HARMONICS ***************************/

int suppress_harmonics(dataptr dz)
{
    int suppress_h = 0;
    switch(dz->mode) {
    case(F_MOVE):   if(dz->vflag[MOV_KHM])      suppress_h = 1; break;
    case(F_MOVE2):  if(dz->vflag[MOV2_KHM])     suppress_h = 1; break;
    case(F_NEGATE): if(dz->vflag[NEG_KHM])      suppress_h = 1; break;
    case(F_INVERT): if(dz->vflag[INVERT_KHM])   suppress_h = 1; break;
    case(F_NARROW): if(dz->vflag[NRW_KHM])      suppress_h = 1; break;
    case(F_SQUEEZE):if(dz->vflag[SQZ_KHM])      suppress_h = 1; break;
    case(F_ROTATE): if(dz->vflag[ROTATE_KHM])   suppress_h = 1; break;
    }
    if(suppress_h && dz->fptr[HMNICBOUNDS] == NULL) {
        if((dz->fptr[HMNICBOUNDS] = (float *)malloc(10 * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store harmonic peak boundaries.\n");
            return(MEMORY_ERROR);
        }
    }
    return FINISHED;
}

/************************************ TROF_DETECT ***************************/

int trof_detect(dataptr dz)     //  DOESN'T WORK will have to do it BY HAND : Need a SLOOM AP TO HELP !!!
{
    double *level = dz->parray[P_PRETOTAMP];
    double lev, lastlev = 0.0, maxlevel, rise, endtroftime, endtrofgap, maxval;
    double lastpeak, lastlastpeak, pk_to_pre_trof, pk_to_post_trof;
    int n, tim, val, lasttim, lastval, up_cnt = 0, dn_cnt = 0, trofcnt = 0, lasttrofcnt, endtrofat, lasttrofat, thistrofat = 0, thispeak;
    int k;
    double *trof;

    if((trof = (double *)malloc(dz->wlength * 2 * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store trof times.\n");
        return(MEMORY_ERROR);
    }
    //  Establish maximum level
    
    maxlevel = 0.0;
    for(n=0;n<dz->wlength;n++)
        maxlevel = max(maxlevel,level[n]);

    //  Find troughs, ensuring intervening peaks are sufficiently prominent
    
    lastpeak = -1.0;
    lastlastpeak = -1.0;
    pk_to_pre_trof = -1.0;

    for(n=0;n<dz->wlength;n++) {
        lev = level[n]/maxlevel;
        rise = lev - lastlev;
        if(rise > 0.0) {
            if(dn_cnt > 0) {
                trof[trofcnt++] = dz->frametime * (n-1);
                trof[trofcnt++] = lastlev;
                if(pk_to_pre_trof > 0.0) {                          //  If previous peak-after-a-trof exists
                    pk_to_post_trof = lastpeak - trof[trofcnt-1];
                    if(lastpeak < dz->fsil_ratio ||                 //  If the peak is not absolutely loud enough
                    (pk_to_pre_trof < MIN_PEAKTROF_GAP              //  OR the peak is not high enough above either bracketing trof
                    && pk_to_post_trof < MIN_PEAKTROF_GAP)) {
                        if(trof[trofcnt-3] < trof[trofcnt-3])       //  If previous trof lower than this
                            trofcnt -= 2;                           //  overwrite this one
                        else {                                      //  else        
                            for(k = trofcnt-2;k< trofcnt;k++)       //  overwrite last trof     
                                trof[k-2] = trof[k];
                            trofcnt -= 2;
                        }
                        lastpeak = lastlastpeak;                    //  lastpeak ignored: penultimate peak is now "lastpeak"
                    }
                }
                dn_cnt = 0;
                up_cnt = 0;
            }
            up_cnt++;
        } else {
            if(up_cnt > 0) { 
                lastlastpeak = lastpeak;                            //  remember this peak and the peak before it
                lastpeak = lastlev;                                 //
                if(trofcnt)                                         //  If a prior trof exists, calc ratio of pk to prior trof
                    pk_to_pre_trof = lastpeak - trof[trofcnt-1];
                dn_cnt = 0;
                up_cnt = 0;
            }
            dn_cnt++;                   
        }
        lastlev = lev;
    }

    //  Eliminate too close trofs

    lasttim = 0;
    lastval = 1;
    lasttrofcnt = trofcnt;
    for(tim=2,val=3;tim<trofcnt;tim+=2,val+=2) {
        if(trof[tim] - trof[lasttim] < MIN_SYLLAB_DUR) {
            if(trof[val] <= trof[lastval]) {
                k = tim;                    //  overwrite previous trof
                while(k < trofcnt) {
                    trof[k-2] = trof[k];
                    k++;
                }
            } else {
                k = tim + 2;                //  overwrite current trof
                while(k < trofcnt) {
                    trof[k-2] = trof[k];
                    k++;
                }
            }
            val -= 2;
            tim -= 2;
            trofcnt -= 2;
        } else {
            lasttim += 2;
            lastval += 2;
        }
    }
    endtroftime = 0.0;
    for(n=dz->wlength-1;n>=0;n--) {
        if(level[n] > dz->fsil_ratio) {
            endtroftime = (n+1) * dz->frametime;
            if(endtroftime > 0.0) {
                endtrofgap = endtroftime - trof[trofcnt-2];
                if(endtrofgap > 0.0) {
                    endtrofat = n+1;
                    lasttrofat = (int)round(trof[trofcnt-2]/dz->frametime);
                    maxval = 0.0;
                    for(k=lasttrofat;k<endtrofat;k++)
                        maxval = max(maxval,level[k]/maxlevel);
                    if(maxval - trof[trofcnt-1] < dz->fsil_ratio)
                        trof[trofcnt-2] = endtroftime;
                }
            }
            break;
        }
    }
    if(dz->vflag[PKS_ONLY] || dz->vflag[PKS_TROFS]) {
        for(n = 2;n < trofcnt;n+=2) {
            if(n==2)
                lasttrofat = (int)round(trof[n-2]/dz->frametime);   //  Trof times, converted to window nums
            else
                lasttrofat = thistrofat;
            thistrofat = (int)round(trof[n]/dz->frametime);
            maxlevel = trof[n-1];
            thispeak = lasttrofat;
            for(k = lasttrofat;k < thistrofat;k++) {                //  Amplitude levels
                if(level[k] > maxlevel) {
                    thispeak = k;
                    maxlevel = level[k];
                }
            }
            trof[n-1] = thispeak * dz->frametime;                   //  Store peaktimes in place of trof level vals
        }
        if(dz->vflag[PKS_ONLY]) {                                   //  Peaks are in odd table locations
            for(n = 1;n < trofcnt-2;n+=2)
                fprintf(dz->fp,"%lf\n",trof[n]);
        } else {                                                    //  Trofs & peaks are in all locations
            if(trof[0] != 0.0)
                fprintf(dz->fp,"%lf\n",0.0);                        //  Force trof at time zero
            for(n = 0;n < trofcnt-1;n++)                        
                fprintf(dz->fp,"%lf\n",trof[n]);
        }
        return FINISHED;
    }
    if(trof[0] != 0.0)                                             //  Force trof at time zero
        fprintf(dz->fp,"%lf\n",0.0);
    for(n = 0;n < trofcnt;n+=2)                                     //  Trof times are in even locations
        fprintf(dz->fp,"%lf\n",trof[n]);
    return FINISHED;
}

/************************************ ZERO_SPECTRUM_TOP ***************************/

void zero_spectrum_top(int cc,dataptr dz)
{
    int vc  = cc * 2;
    for(; cc < dz->clength;cc++, vc+=2)
        dz->flbufptr[0][AMPP] = 0.0f;
}

/************************************ FORMANTS_RECOLOR ***************************/

int formants_recolor(int inner_lpcnt,double *phase,int *up,int arp_param,double minpitch,double maxpitch,int contourcnt,dataptr dz)
{
    int exit_status, cc, vc, newcc, do_arpegg = 0, nextup = 1, tail = 0, k; 
    float the_fundamental, frq, thisfrq, newfrq = 0.0, endamp = 0.0;
    double frqtrans = 1.0, frqshift = 0.0, randomisation = 0.0, rand, incr; 
    double nextphase = 0.0, thisamp, origspecamp, newspecamp, vocode_ratio, searchfrqlimit;
    int *isharm = dz->iparray[ISHARM];
    double partial_number_centre, partial_number_foot, partial_number_top, arpegamp = 0.0, pre_amptotal = 0.0, post_amptotal = 0.0;
    double lower_partial_number_top = 0.0, lower_partial_number_foot = 0.0, upper_partial_number_top = 0.0, upper_partial_number_foot = 0.0;
    double lobandbot = 0.0, lobandtop = 0.0, hibandbot = 0.0, hibandtop = 0.0,  bwidth = 0.0;
    int is_split = 0, dotopfill = 0;

    //  Get window pitch

    the_fundamental = dz->pitches[inner_lpcnt];

    if(the_fundamental < 0.0) {                                         //  If unpitched and "silent" windows have been retained & this is one, silence the window
        for(cc = 0,vc = 0;cc < dz->clength;cc++,vc+=2)
            dz->flbufptr[0][AMPP] = 0.0f;
        *up = nextup;
        *phase = nextphase;
        return FINISHED;
    }

    //  IF ARPEGGIATING : Calculate the range of the arpeggiation window

    if(dz->brksize[arp_param] || dz->param[arp_param] > 0.0) {

        //  IF ARPEGGIATING : precalc nextphase by advancing phase according to arpeggiation-speed

        nextphase = *phase + (dz->param[arp_param] * dz->phasefactor);
        if(nextphase > 1.0) {
            nextup = !(*up);
            nextphase -= 1.0;
        } else
            nextup = *up;

        //  & calculate the range of the arpeggiation window

        if(dz->vflag[RECOLOR_DWN])
            partial_number_centre = (1.0 - *phase) * ARPEG_RANGE;
        else if(dz->vflag[RECOLOR_CYC]) {
            if(up)
                partial_number_centre = *phase * ARPEG_RANGE;
            else
                partial_number_centre = (1.0 - *phase) * ARPEG_RANGE;
        } else
            partial_number_centre = *phase * ARPEG_RANGE;
        partial_number_foot = partial_number_centre - ARPEG_WIN_HALFWIDTH;
        partial_number_top  = partial_number_centre + ARPEG_WIN_HALFWIDTH;
        if(partial_number_top > ARPEG_RANGE) {
            lower_partial_number_foot = 0.0;                                    //  NB partial numbers calculated over range 0 to 7
            lower_partial_number_top  = partial_number_top - ARPEG_RANGE;       //  But need to be in range 1-8 to multiply the fundamental
            upper_partial_number_foot = partial_number_top;
            upper_partial_number_top  = ARPEG_RANGE;
            lobandbot = (lower_partial_number_foot + 1.0) * the_fundamental;
            lobandtop = (lower_partial_number_top  + 1.0) * the_fundamental;
            hibandbot = (upper_partial_number_foot + 1.0) * the_fundamental;
            hibandtop = (upper_partial_number_top  + 1.0) * the_fundamental;
            is_split = 1;
        } else if(partial_number_foot < 0.0) {
            upper_partial_number_foot = partial_number_foot + ARPEG_RANGE;
            upper_partial_number_top  = ARPEG_RANGE;
            lower_partial_number_foot = 0.0;
            lower_partial_number_top  = partial_number_top;
            lobandbot = (lower_partial_number_foot + 1.0) * the_fundamental;
            lobandtop = (lower_partial_number_top  + 1.0) * the_fundamental;
            hibandbot = (upper_partial_number_foot + 1.0) * the_fundamental;
            hibandtop = (upper_partial_number_top  + 1.0) * the_fundamental;
            is_split = 1;
        } else {
            lobandbot = (partial_number_foot + 1.0) * the_fundamental;
            lobandtop = (partial_number_top  + 1.0) * the_fundamental;
            is_split = 0;
        }
        bwidth = ARPEG_WIN_WIDTH * the_fundamental;
        do_arpegg = 1;                                                          //  and setup arpeggiation flag
    }                                                           

    dotopfill = 0;  
    if(dz->vflag[RECOLOR_FIL]) {                                                //  Attempt to fill top of spectrum, where src is shrunk in spectral range
        switch(dz->mode) {
        case(F_OCTSHIFT):   if(dz->iparam[COLINT] < 0)  dotopfill = 1;                  break;
        case(F_FRQSHIFT):   //  fall thro   
        case(F_TRANS):      if(dz->param[COLINT] <  0)  dotopfill = 1;                  break;
        case(F_RESPACE):    if(dz->param[COLINT] < the_fundamental)     dotopfill = 1;  break;
        }
    }

    if((exit_status = get_totalamp(&pre_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);

    //  initialise the extra "windowbuf"

    memset((char *)dz->windowbuf[0],0,dz->wanted * sizeof(float));

    //  Get frq-moving value

    switch(dz->mode) {
    case(F_OCTSHIFT):   frqtrans = pow(2.0,dz->iparam[COLINT]);                     break;
    case(F_TRANS):      frqtrans = pow(2.0,dz->param[COLFLT]/SEMITONES_PER_OCTAVE); break;
    case(F_FRQSHIFT):   frqshift = dz->param[COLFLT];                               break;
    case(F_RESPACE):    frqshift = dz->param[COLFLT];                               break;
    case(F_RAND):       
        randomisation = dz->param[COLFLT];
        rand  = (drand48() * 2.0) - 1.0;                    //  For rand option, generate number in range -1 TO +1 or less
        rand /= 2.0;                                        //  to range +- 1/2.
        rand *= randomisation;                              //  Some value with this range, or a smaller range around centre at 0.0
        frqshift = the_fundamental * rand;                  //  Step is  <= +- 1/2 distance between harmonics
        break;
    case(F_PINVERT):
        if((exit_status = pitch_invert(&newfrq,the_fundamental,dz))<0)
            return exit_status;
        frqtrans = newfrq/the_fundamental;
        break;
    case(F_PEXAGG):
        if((exit_status = exag_pitchline(&newfrq,dz->pitches[inner_lpcnt],minpitch,maxpitch,dz))<0)
            return exit_status;
        frqtrans = newfrq/the_fundamental;
        break;
    case(F_PCHRAND)://  fall thro
    case(F_PQUANT):
        newfrq = dz->pitches2[inner_lpcnt];
        frqtrans = newfrq/the_fundamental;
        break;
    }

    //  Do the frq change required, on the fundamental

    switch(dz->mode) {                                          
    case(F_RESPACE):    newfrq = the_fundamental;   break;
    case(F_OCTSHIFT):   //  fall thro
    case(F_TRANS):      newfrq = (float)(the_fundamental * frqtrans);   break;
    case(F_FRQSHIFT):   //  fall thro
    case(F_RAND):       newfrq = (float)min(FSPEC_MINFRQ,max((float)(the_fundamental + frqshift),dz->nyquist)); break;
    case(F_ARPEG):      newfrq = the_fundamental;   break;
    }


    //  If EXI flag set, (non-harmonics channels TO BE zeroed)

        //  Find and transpose the partials: placing them in  "windowbuf"

    if(dz->vflag[RECOLOR_EXI]) {
        if((exit_status = find_and_change_partials(the_fundamental,newfrq,isharm,frqshift,frqtrans,randomisation,bwidth,lobandbot,lobandtop,hibandbot,hibandtop,do_arpegg,is_split,dz))<0)
            return exit_status;

        //  Then delete the non-partial channel data

        for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {
            if(dz->windowbuf[0][FREQ] <= 0.0) {
                dz->windowbuf[0][AMPP] = 0.0;
                dz->windowbuf[0][FREQ] = (float)((double)cc * dz->chwidth);
            }
        }
        //  Finally, copy transformed spectrum back into flbufptr, ready for output
    
        memcpy((char *)dz->flbufptr[0],(char *)dz->windowbuf[0],dz->wanted * sizeof(float));    
    
    } else {

    //  Otherwise, modify the entire spectrum: 
        
        //  First appegiate the entire spectrum (i.e. run the arpeggiation window over the entire spectrum)

        if(dz->mode == F_ARPEG || do_arpegg) {
            for(cc=0,vc=2;cc <dz->clength;cc++,vc+=2) {
                thisfrq = dz->flbufptr[0][FREQ];
                if((exit_status = arpeggiate(&arpegamp,thisfrq,bwidth,lobandbot,lobandtop,hibandbot,hibandtop,is_split,dz))<0)
                    return exit_status;
                dz->flbufptr[0][AMPP] = (float)arpegamp;
            }
        }

        //  For transposed or shifted data, put the transformed data into "windowbuf" in the appropriate (new) channel

        if(dz->mode != F_ARPEG) {

            switch(dz->mode) {
            case(F_OCTSHIFT):   //  fall thro
            case(F_TRANS):      //  fall thro
            case(F_PINVERT):    //  fall thro
            case(F_PEXAGG):     //  fall thro
            case(F_PQUANT):     //  fall thro
            case(F_PCHRAND):    //  fall thro
            case(F_FRQSHIFT):   
                for(cc=0,vc=2;cc <dz->clength;cc++,vc+=2) {
                    if(dz->mode == F_FRQSHIFT)
                        newfrq = (float)(dz->flbufptr[0][FREQ] + frqshift);     //  Frqshift
                    else                                                        //  OR
                        newfrq = (float)(dz->flbufptr[0][FREQ] * frqtrans);     //  Transpose   
                    newcc  = (int)round(newfrq/dz->chwidth);                    //  Find appropriate new channel
                    if(newcc >= 0 && newcc <= dz->clength-1) {                  //  If channel is within range
                        if(dz->flbufptr[0][AMPP] > dz->windowbuf[0][newcc*2]) { //  IF new data is louder than any data already copied into thus location
                            dz->windowbuf[0][newcc*2 + 1] = newfrq;             //  Copy data into windowbuf    
                            dz->windowbuf[0][newcc*2] = dz->flbufptr[0][AMPP];
                        }
                    }   
                }
                break;
            case(F_RESPACE):
                for(cc=0,vc=2;cc <dz->clength;cc++,vc+=2) {
                    newfrq = dz->flbufptr[0][FREQ];
                    if(newfrq >= the_fundamental) {                             //  For all data above the fundamental  
                        newfrq -= the_fundamental;                              //  Shrink its distance above fundamental, by ratio new-harm spacing to orig
                        newfrq *= (float)(frqshift/the_fundamental);            //  and add back onto fundamental
                        newfrq += the_fundamental;
                    }
                    newcc  = (int)round(newfrq/dz->chwidth);
                    if(newcc >= 0 && newcc <= dz->clength-1) {
                        if(dz->flbufptr[0][AMPP] > dz->windowbuf[0][newcc*2]) {
                            dz->windowbuf[0][newcc*2 + 1] = (float)(newfrq);
                            dz->windowbuf[0][newcc*2] = dz->flbufptr[0][AMPP];
                        }
                    }
                }
                break;
            case(F_RAND):
                for(cc=0,vc=2;cc <dz->clength;cc++,vc+=2) {
                    frq = dz->flbufptr[0][FREQ];
                    rand  = (drand48() * 2.0) - 1.0;
                    rand /= 2.0;
                    rand *= randomisation;
                    frqshift = the_fundamental * rand;
                    newfrq = (float)(dz->flbufptr[0][FREQ] + frqshift);
                    newcc  = (int)round(newfrq/dz->chwidth);
                    if(newcc >= 0 && newcc <= dz->clength-1) {
                        if(dz->flbufptr[0][AMPP] > dz->windowbuf[0][newcc*2]) {
                            dz->windowbuf[0][newcc*2 + 1] = (float)(newfrq);
                            dz->windowbuf[0][newcc*2] = dz->flbufptr[0][AMPP];
                        }
                    }
                }
                break;
            }
            if(dotopfill) {
                for(cc = dz->clength-1,vc=cc*2;cc >= 0;cc--,vc-=2) {
                    if(dz->windowbuf[0][FREQ] > 0.0) {
                        endamp = dz->windowbuf[0][AMPP];
                        break;
                    }
                }
                dotopfill = cc+1;
                if((exit_status = do_top_of_spectrum_fill(dotopfill,endamp,the_fundamental,frqtrans,frqshift,&tail,dz))<0)
                    return exit_status;
            }


            //  Put a reasonable freq on channels which have not received new data (HEREH : should these remain amp zero ??)

            for(cc=0,vc=2;cc <dz->clength;cc++,vc+=2) {
                if(dz->windowbuf[0][FREQ] == 0.0)                           
                    dz->windowbuf[0][FREQ] = (float)(cc * dz->chwidth);
            }

            //  Extract envelope of new data, into specenvamp2

            if(!(dz->mode == F_PCHRAND && dz->vflag[NO_RESHAPE])) {
                dz->flbufptr[1] = dz->windowbuf[0];
                if((exit_status = extract_specenv(1,1,dz))<0)
                    return(exit_status);

                //  Impose the ORIGINAL spectral envelope on the new data

                for(cc=0,vc=2;cc <dz->clength;cc++,vc+=2) {
                    frq = dz->windowbuf[0][FREQ];                               //  At new-spectrum channel-frq
                    if((exit_status = getspecenvamp(&newspecamp,frq,1,dz))<0)   //  Get amplitude on spectral envelope of new-spectrum, at this freq
                        return(exit_status);
                    if((exit_status = getspecenvamp(&origspecamp,frq,0,dz))<0)  //  Get amplitude on spectral envelope of original-spectrum, at this freq
                        return(exit_status);
                    if(newspecamp <= 0.0)                                       //  Check for zeros, in either spectrum
                        dz->windowbuf[0][AMPP] = 0.0f;
                    else {
                        vocode_ratio = origspecamp/newspecamp;
                        thisamp = dz->windowbuf[0][AMPP] * vocode_ratio;
                        if(thisamp < VERY_TINY_VAL)
                            dz->windowbuf[0][AMPP] = 0.0f;
                        else
                            dz->windowbuf[0][AMPP] = (float)thisamp;
                    }
                }
                if(tail > 0) {                                                  //  Any top-of-spectrum insertions, tail away to zero
                    for(k = 1,cc = dz->clength-1,vc = cc*2;k <= tail;k++,cc--,vc-=2) {
                        incr = pow((double)k/(double)(tail+1),TAILOFF_SLOPE);   
                        dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * incr);
                    }
                }
            }
                //  Finally, copy transformed spectrum back into flbufptr, ready for output
            
            memcpy((char *)dz->flbufptr[0],(char *)dz->windowbuf[0],dz->wanted * sizeof(float));    

        }

    }

    //  Now advance phase of arpeggiator
    
    *up = nextup;
    *phase = nextphase;

    //  Do any spoecified lopass and hipass filtering
    
    if(dz->param[COL_LO] > 0.0) {
        searchfrqlimit = dz->param[COL_LO] + (dz->chwidth * 2); 
        for(cc=0,vc=0;cc <dz->clength;cc++,vc+=2) {
            if(dz->flbufptr[0][FREQ] < dz->param[COL_LO])
                dz->flbufptr[0][AMPP] = 0.0f;
            else if(dz->flbufptr[0][FREQ] > searchfrqlimit)
                break;
        }
    }
    if(dz->param[COL_HI] > 0.0) {
        searchfrqlimit = dz->param[COL_HI]  - (dz->chwidth * 2); 
        for(cc=0,vc=2;cc <dz->clength;cc++,vc+=2) {
            if(dz->flbufptr[0][FREQ] < searchfrqlimit)
                continue;
            else if(dz->flbufptr[0][FREQ] > dz->param[COL_HI])
                dz->flbufptr[0][AMPP] = 0.0f;
        }
    }

    if((exit_status = get_totalamp(&post_amptotal,dz->flbufptr[0],dz->wanted))<0)
        return(exit_status);
    if(post_amptotal > pre_amptotal) {
        if((exit_status = normalise(pre_amptotal,post_amptotal,dz))<0)
            return(exit_status);
    }

    //  Apply any user entered attenuation

    if(dz->param[FGAIN] > 0.0 && !flteq(dz->param[FGAIN],1.0))  {
        for(vc=0;vc<dz->wanted;vc+=2)
            dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FGAIN]);
    }

    return FINISHED;
}

/*************************  ARPEGGIATE *************************
 *
 *  Arpeggaite runs a small, inverted cosin, window over the first 16 harmonics,
 *  recycling baclk to harmonic 1, qwhen it overruns the top of this range.
 *
 *  So, if the input phase is in range 0 to 1, the position within this ARPEG_RANGE window is in range 0 to 7 (inclusive)
 *
 *  The window itself is ARPEG_WIN_WIDTH partials wide.                                 |               1           |
 *                                                                                      |               _           |
 *  So, any partial (i.e. data in dz->windowbuf[0]) falling under this window           |             /   \         |
 *  (OR any data at all, in the flbufptr, when non-harmonic data not Suppressed).       |            /     \    -->>|
 *  has its amplitude multiplied by the window.                                         |       0   /       \   0   |
 *                                                                                      |       _ /           \ _   |
 *                                                                                      |               _           |
 *                                                                       Partial Nos    0   1   2   3   4   5   6   7
 *                                                                  Root multipliers    1   2   3   4   5   6   7   8
 */

int arpeggiate(double *arpegamp,float frq,double bwidth,double lobandbot,double lobandtop,double hibandbot,double hibandtop,int is_split,dataptr dz)
{
    double frq_offset_under_window, shaping;

        if(frq > lobandbot && frq < lobandtop) {
            frq_offset_under_window = (frq - lobandbot)/bwidth;
            shaping = -cos(frq_offset_under_window * TWOPI);    //  Range -1/+1/-1
            shaping = max(0.0,shaping + 1.0);                   //  Range 0 /+2/ 0
            shaping = min(1.0,shaping/2.0);                     //  Range 0 /+1/ 0

        } else if(is_split && (frq > hibandbot && frq < hibandtop)) {
            frq_offset_under_window = (frq - hibandbot)/bwidth;
            shaping = -cos(frq_offset_under_window * TWOPI);    //  Range -1/+1/-1
            shaping = max(0.0,shaping + 1.0);                   //  Range 0 /+2/ 0
            shaping = min(1.0,shaping/2.0);                     //  Range 0 /+1/ 0
        } else
            shaping  = 0.0;                                     //  Zero partial amplitude
    *arpegamp = shaping;
    return FINISHED;
}

/************************************ FIND_AND_CHANGE_PARTIALS ***************************/

int find_and_change_partials(float the_fundamental,float newfrq,int *isharm,double frqshift,double frqtrans,double randomisation,
                             double bwidth,double lobandbot,double lobandtop,double hibandbot,double hibandtop,int do_arpegg,int is_split,dataptr dz)
{
    int exit_status, cc, vc, thiscc, kcc, kvc, newcc, harmonic_found_in_src, harmonic_no;
    int most_appropriate_chan, harmonicbandcnt, firstchan_with_harmonic, lastchan_with_harmonic, above_most_approp_offset, below_most_approp_offset; 
    float thisfrq, chancentre, thatchancentre, thatfrq, the_harmonicfrq;
    double this_chan_frq_offset, that_chan_frq_offset, specamp = 0.0, arpegamp;
    double rand; 

    the_harmonicfrq = the_fundamental;
    harmonic_no = 1;
    harmonic_found_in_src = 0;

    //  HARMONICS FOUND IN SRC IN MULTIPLE CHANNELS, Have multiple channels in output
    //  If a particular harmonic NOT FOUND in src, put it in a single channel in output

    for(cc = 0,vc = 0;cc < dz->clength;cc++,vc+=2) {
        thisfrq = dz->flbufptr[0][FREQ];
        if(is_above_equivalent_pitch(thisfrq,the_harmonicfrq,dz)) {     //  Once the channel data is higher than the current harmonic frq, move to next harmonic
            if(!harmonic_found_in_src) {                                
                if(newfrq < dz->nyquist && newfrq > FSPEC_MINFRQ) {         //  BUT if no channel with current harmonic HAS BEEN found, insert it in a single channel

                    if(do_arpegg) {
                        if((exit_status = arpeggiate(&arpegamp,the_harmonicfrq,bwidth,lobandbot,lobandtop,hibandbot,hibandtop,is_split,dz))<0)
                            return exit_status;
                    } else
                        arpegamp = 1.0;
                    
                    thiscc = (int)floor((fabs(newfrq) + dz->halfchwidth)/dz->chwidth);
                    vc = thiscc*2;
                    dz->windowbuf[0][FREQ] = newfrq;
                    if((exit_status = getspecenvamp(&specamp,newfrq,0,dz))<0)
                        return(exit_status);
                    dz->windowbuf[0][AMPP]  = (float)(specamp * arpegamp);
                }
            }

            the_harmonicfrq += the_fundamental;                         //  Move to next harmonic
            harmonic_no++;
            if(the_harmonicfrq >= dz->nyquist)
                break;
            harmonic_found_in_src = 0;                                  //  Set flag telling us whether THIS harmonic is founds in src
            if(dz->mode == F_RAND) {
                rand  = (drand48() * 2.0) - 1.0;                        //  For rand option, generate number in range -1 TO +1 or less
                rand /= 2.0;                                            //  to range +- 1/2.
                rand *= randomisation;                                  //  Some value with this range, or a smaller range around centre at 0.0
                frqshift = the_fundamental * rand;                      //  Step is  <= +- 1/2 distance between harmonics
            }
            switch(dz->mode) {                                          //  Do the frq change required
            case(F_RESPACE):    newfrq = (float)(the_fundamental + (frqshift * harmonic_no));   break;
            case(F_OCTSHIFT):   //  fall thro
            case(F_PINVERT):    //  fall thro   
            case(F_PEXAGG):     //  fall thro   
            case(F_PQUANT):     //  fall thro   
            case(F_PCHRAND):    //  fall thro   
            case(F_TRANS):      newfrq = (float)(the_harmonicfrq * frqtrans);   break;
            case(F_FRQSHIFT):   //  fall thro
            case(F_RAND):       newfrq = (float)min(FSPEC_MINFRQ,max((float)(the_harmonicfrq + frqshift),dz->nyquist)); break;
            case(F_ARPEG):      newfrq = the_harmonicfrq;   break;
            }
            if(newfrq >= dz->nyquist || newfrq < FSPEC_MINFRQ)              //  If shifted frq becomes out of range, quit
                break;
        }
        if(is_equivalent_pitch(thisfrq,the_harmonicfrq,dz)) {           //  IF freq in channel a harmonic
            if(do_arpegg) {
                if((exit_status = arpeggiate(&arpegamp,the_harmonicfrq,bwidth,lobandbot,lobandtop,hibandbot,hibandtop,is_split,dz))<0)
                    return exit_status;
            } else
                arpegamp = 1.0;
            isharm[cc] = 1;
            harmonic_found_in_src = 1;
            chancentre = (float)(cc * dz->chwidth);                     //  Look in channels immediately above this
            this_chan_frq_offset = fabs(thisfrq - chancentre);
            most_appropriate_chan = cc;
            harmonicbandcnt = 1;
            firstchan_with_harmonic = cc;
            lastchan_with_harmonic  = cc;
            kcc = cc + 1;                                               //  How many adjacent channels contain this harmonic        
            for(kvc = kcc*2;kcc < dz->clength;kcc++,kvc+=2) {
                thatchancentre = (float)(kcc * dz->chwidth);            //  And which channel-centre is closest to the harmonic-frq
                thatfrq = dz->flbufptr[0][kvc+1];
                if(is_equivalent_pitch(thatfrq,the_harmonicfrq,dz)) {
                    isharm[kcc] = 1;
                    harmonicbandcnt++;
                    that_chan_frq_offset = fabs(thatfrq - thatchancentre);
                    if(that_chan_frq_offset < this_chan_frq_offset) {
                        most_appropriate_chan = kcc;
                        this_chan_frq_offset = that_chan_frq_offset;
                    }
                }
                else        //  If no futher adjacent channels containing this harmonic are found
                    break;  //  break from search for further chans containing the harmonic.
            }
                                                                        //  Put the transposed harmonic into the appropriate new channel
            newcc = (int)floor((fabs(newfrq) + dz->halfchwidth)/dz->chwidth);
            vc = newcc*2;
            dz->windowbuf[0][FREQ] = newfrq;                            //  windowbuf stores the transposed-harmonics data
            if((exit_status = getspecenvamp(&specamp,newfrq,0,dz))<0)   //  but under the original formant envelope
                return(exit_status);
            if(harmonicbandcnt > 1)                                     //  If harmonic appeared in > 1 adjacent channels
                specamp /= (double)harmonicbandcnt;                     //  Divide amp between chans that share this frequency
            dz->windowbuf[0][AMPP]  = (float)(specamp * arpegamp);
                                                            
            if(harmonicbandcnt > 1) {                                   //  If harmonic appeared in > 1 adjacent channels
                lastchan_with_harmonic = kcc-1;
                above_most_approp_offset = lastchan_with_harmonic - most_appropriate_chan;
                below_most_approp_offset = most_appropriate_chan - firstchan_with_harmonic;
                if(below_most_approp_offset > 0) {                      //  Fill same number of adjacent channels at new location
                    thiscc = max(newcc - below_most_approp_offset,0);
                    for(vc = thiscc*2;thiscc < newcc;thiscc++,vc+=2) {
                        dz->windowbuf[0][FREQ] = newfrq;
                        dz->windowbuf[0][AMPP] = (float)(specamp * arpegamp);;
                    }
                }
                if(above_most_approp_offset > 0) {
                    thiscc = min(newcc + above_most_approp_offset,dz->clength - 1);
                    for(vc = thiscc*2;thiscc > newcc;thiscc--,vc-=2) {
                        dz->windowbuf[0][FREQ] = newfrq;
                        dz->windowbuf[0][AMPP] = (float)(specamp * arpegamp);
                    }
                }
            }
            cc = lastchan_with_harmonic;                                //  Move search to last appearance of this harmonic
            vc = cc * 2;
            the_harmonicfrq += the_fundamental;                         //  And go to next harmonic
            harmonic_no++;
            if(the_harmonicfrq >= dz->nyquist)
                break;
            if(dz->mode ==F_RAND) {                                     //  Generating the next transformed frq
                rand  = (drand48() * 2.0) - 1.0;
                rand /= 2.0;
                rand *= randomisation;
                frqshift = the_fundamental * rand;
            }
            switch(dz->mode) {
            case(F_RESPACE):    newfrq = (float)(the_fundamental + (frqshift * harmonic_no));   break;
            case(F_OCTSHIFT):   //  fall thro
            case(F_PINVERT):    //  fall thro
            case(F_PEXAGG):     //  fall thro
            case(F_PQUANT):     //  fall thro
            case(F_PCHRAND):    //  fall thro
            case(F_TRANS):      newfrq = (float)(the_harmonicfrq * frqtrans);   break;
            case(F_FRQSHIFT):   //  fall thro
            case(F_RAND):       newfrq = (float)(the_harmonicfrq + frqshift);   break;
            case(F_ARPEG):      newfrq = the_harmonicfrq;   break;
            }
            if(newfrq >= dz->nyquist || newfrq < FSPEC_MINFRQ)              //  If shifted frq becomes out of range, quit
                break;
        }                   
    }
    return FINISHED;
}

/************************************ IS_COLORING ***************************/

int is_coloring(dataptr dz)
{
    switch(dz->mode) {
    case(F_OCTSHIFT):   //  fall thro 
    case(F_TRANS):      //  fall thro 
    case(F_FRQSHIFT):   //  fall thro 
    case(F_RESPACE):    //  fall thro 
    case(F_PINVERT):    //  fall thro 
    case(F_PEXAGG):     //  fall thro 
    case(F_PQUANT):     //  fall thro 
    case(F_PCHRAND):    //  fall thro 
    case(F_RAND):       return TRUE; break;
    }
    return FALSE;
}

/************************************ DO_TOP_OF_SPECTRUM_FILL ***************************/

int do_top_of_spectrum_fill(int cc,float endamp,float the_fundamental,double frqtrans,double frqshift,int *tail,dataptr dz)
{
    int finished = 0, newcc, lastnewcc = -1;
    float newfrq;
    double frq;
    frq = dz->nyquist;
    while (!finished) {
        frq += dz->chwidth;
        switch(dz->mode) {
        case(F_OCTSHIFT):   //  fall thro
        case(F_TRANS):      //  fall thro
        case(F_FRQSHIFT):   
            if(dz->mode == F_FRQSHIFT)
                newfrq = (float)(frq + frqshift);
            else
                newfrq = (float)(frq * frqtrans);
            newcc  = (int)round(newfrq/dz->chwidth);
            if(newcc >= 0 && newcc <= dz->clength-1) {
                dz->windowbuf[0][newcc*2 + 1] = newfrq;
                dz->windowbuf[0][newcc*2] = endamp;
                if(newcc > lastnewcc)
                    (*tail)++;
                lastnewcc = newcc;
            } else
                finished = 1;
            break;
        case(F_RESPACE):
            newfrq = (float)frq;
            if(newfrq >= the_fundamental) {
                newfrq -= the_fundamental;
                newfrq *= (float)(frqshift/the_fundamental);
                newfrq += the_fundamental;
            }
            newcc  = (int)round(newfrq/dz->chwidth);
            if(newcc >= 0 && newcc <= dz->clength-1) {
                dz->windowbuf[0][newcc*2 + 1] = (float)(newfrq);
                dz->windowbuf[0][newcc*2] = endamp;
                if(newcc > lastnewcc)
                    (*tail)++;
                lastnewcc = newcc;
            } else
                finished = 1;
            break;
        }
    }
    return FINISHED;
}

/************************************ GETMEANPITCH ***************************/

int getmeanpitch(double *meanpitch,double *minpitch,double *maxpitch,dataptr dz)
{
    int n, sumcnt = 0;
    double maxpitchfrq = -HUGE, minpitchfrq = HUGE;
    *meanpitch = 0.0;
    for(n = 0; n < dz->wlength; n++) {
        if(dz->pitches[n] >= MINPITCH) {
            maxpitchfrq = max(maxpitchfrq,dz->pitches[n]);
            minpitchfrq = min(minpitchfrq,dz->pitches[n]);
            *meanpitch += unchecked_hztomidi(dz->pitches[n]);
            sumcnt++;
        }
    }
    *meanpitch /= (double)sumcnt;
    *minpitch = unchecked_hztomidi(minpitchfrq); 
    *maxpitch = unchecked_hztomidi(maxpitchfrq); 
    if(*maxpitch < 0.0) {
        sprintf(errstr,"Failed to find maximum pitch in pitch data: (Possibly no definite pitch in file?)\n");
        return GOAL_FAILED;
    }
    if(*minpitch > 200) {
        sprintf(errstr,"Failed to find minimum pitch in pitch data: (Possibly no definite pitch in file?)\n");
        return GOAL_FAILED;
    }
    return FINISHED;
}

/************************** INTERVAL_MAPPING ************************
 *
 * Approximate input interval to nearest value in LHS column of input map.
 * Find variance from that value.                                           
 * Return corresponding value in RHS column of map, with the variance correction added.
 */

int interval_mapping(double *thisint,double thismidi,dataptr dz)
{
    double *p    = dz->parray[F_PINTMAP], *q;
    double *pend = dz->parray[F_PINTMAP] + (dz->itemcnt * 2);
    double variance, v1, v2;
                                        //  Normally, points to the normal upeard interval
    q = p+1;                            //  and q points  to the correspoinding downward interval

    if(*p < 0.0) {                      // But interval is already downward (-ve)
        p = q;                          // we read the inverted intervals column
        q = p-1;                        // and map to the non-inverted
    }
    if(thismidi <= *p) {                 /* intvl is below all entries in mapping table */
        variance = thismidi - *p;
        *thisint = *q - variance;       /* return map of bottom val (-variance) */
        return(FINISHED);               
    }
    while(thismidi > *p) {
        p += 2;
        q += 2;
        if(p >= pend) {                  /* intvl is above all entries in mapping table */
            p -= 2;
            q -= 2;
            variance = thismidi - *p;
            *thisint = *q - variance;   /* return map of top val (-variance) */
            return(FINISHED);
        }
    }
    v1 = *p - thismidi;                  /* intvl is between 2 entries in mapping table */
    v2 = thismidi - *(p-2);
    if(v1 > v2) {                        /* Compare variances, to find which intvl closer */
        variance = v2;
        *thisint = *(q-2) - variance;
    } else {
        variance = v1;
        *thisint = *q + variance;       /* return appropriate mapped pitch (+/-variance) */
    }
    return(FINISHED);            
}

/********************** READ_INTERVAL_MAPPING ************************/

int read_interval_mapping(char *filename,dataptr dz)
{
    int exit_status;
    double *p;
    int n, m;
    double src_n, src_m, goal_n, goal_m;

    if(!strcmp(filename,"0"))
        dz->is_mapping = FALSE;
    else { 

        //  Get data from textfile

        if((exit_status = get_and_count_data_from_textfile(filename,&(dz->parray[F_PINTMAP]),dz))<0)
            return(exit_status);
        p = dz->parray[F_PINTMAP];

        //  Check range of data

        for(n=0;n<dz->itemcnt;n++) {
            if(*p < -MAXINTRANGE || *p > MAXINTRANGE) {
                sprintf(errstr,
                "Mapping val (%lf) out of range (%.0lf to -%.0lf semitones) (8 8vas up or down).\n",
                *p,MAXINTRANGE,MAXINTRANGE);
                return(DATA_ERROR);
            }
            p++;
        }
        if(ODD(dz->itemcnt)) {
            sprintf(errstr,"Data not paired correctly in mapping file\n");
            return(DATA_ERROR);
        }

        //  Sort mapping data

        for(n=0;n<dz->itemcnt-2;n+=2) {
            src_n  = dz->parray[F_PINTMAP][n];
            goal_n = dz->parray[F_PINTMAP][n+1];
            for(m = n+2;m<dz->itemcnt;m+=2) {
                src_m  = dz->parray[F_PINTMAP][m];
                goal_m = dz->parray[F_PINTMAP][m+1];
                if(src_n > src_m) {
                    dz->parray[F_PINTMAP][n]   = src_m;
                    dz->parray[F_PINTMAP][m]   = src_n;
                    dz->parray[F_PINTMAP][n+1] = goal_m;
                    dz->parray[F_PINTMAP][m+1] = goal_n;
                    src_n  = src_m;
                    goal_n = goal_m;
                }
            }
        }
        dz->itemcnt /= 2;
        dz->is_mapping = TRUE;
    } 
    return(FINISHED);
}

/*************************** GET_AND_COUNT_DATA_FROM_TEXTFILE ******************************
 *
 * (1)  Gets double table data from a text file.
 * (2)  counts ALL items (not pairing them).
 * (3)  Does NOT check for in-range.
 */

int get_and_count_data_from_textfile(char *filename,double **brktable,dataptr dz)
{
    FILE *fp;
    double *p, dummy;
    char temp[200], *q;
    int n = 0;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr, "Can't open textfile %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    n = 0;
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,&dummy))
            n++;
    }       
    if(n == 0) {
        sprintf(errstr,"No data in special data file %s\n",filename);
        return(DATA_ERROR);
    }

    if((*brktable = (double *)malloc(n * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for data.\n");
        return(MEMORY_ERROR);
    }

    p = *brktable;
    rewind(fp);
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,p))
            p++;
    }       
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
        fflush(stdout);
    }
    dz->itemcnt = n;
    return(FINISHED);
}

/************************************* EXAG_PITCHLINE ******************************/

int exag_pitchline(float *newfrq,float thisfrq,double minpich,double maxpich,dataptr dz)
{
    int exit_status;
    double meanpich, thisint, thispich;
    dz->time = 0.0f;
    meanpich  = dz->param[COLFLT];

    if((exit_status = hztomidi(&thispich,thisfrq))<0)
        return(exit_status);
    if(maxpich - meanpich > SEMITONES_PER_OCTAVE)
        maxpich = meanpich + SEMITONES_PER_OCTAVE;
    if(meanpich - minpich > SEMITONES_PER_OCTAVE)
        minpich = meanpich - SEMITONES_PER_OCTAVE;
    
    /* if contour variable specified */
    if(dz->vflag[EXAG_HITIE]) {         //  TIES TO TOP OF RANGE    
        if(thispich > meanpich)
            thispich = (float)maxpich;
        else if(dz->vflag[EXAG_MIDTIE] && !dz->vflag[EXAG_LOTIE])
            thispich = (float)meanpich;
    }
    if(dz->vflag[EXAG_LOTIE]) {
        if(thispich < meanpich)
            thispich = (float)minpich;
        else if(dz->vflag[EXAG_MIDTIE] && !dz->vflag[EXAG_HITIE])
            thispich = (float)meanpich;
    }                                                                                           

        /* If range variable specified */
    if(dz->param[EXAGRANG] > 0.0) {                                     //  -1 flags no range variation

        if(thispich>=meanpich) {    
            if(!dz->vflag[ONLY_BELOW]) {
                thisint  = thispich - meanpich;                         //  What is interval from mean to this pitch
                thispich = meanpich + (thisint * dz->param[EXAGRANG]);
            }
        } else {                                                        //  Exaggerate it
            if(!dz->vflag[ONLY_ABOVE]) {
                thisint  = meanpich - thispich;                         //  and add-to/subtract-from mean
                thispich = meanpich - (thisint * dz->param[EXAGRANG]);
            }
        }
    }                                                                   //  Force pitch to be within range
    thispich = min(dz->param[COLHIPCH],max(dz->param[COLLOPCH],thispich));
    *newfrq = (float)miditohz(thispich);
    return(FINISHED);
}

/************************************* PITCH_INVERT ******************************/

int pitch_invert(float *newfrq,float the_fundamental,dataptr dz)
{
    int exit_status;
    double pivotpitch, rootpitch, newpitch, thisintvl, newintvl;
    pivotpitch = dz->param[COLINT];
    rootpitch  = unchecked_hztomidi(the_fundamental);
    thisintvl  = pivotpitch - rootpitch;
    if(dz->is_mapping) {
        if((exit_status = interval_mapping(&newintvl,thisintvl,dz))<0)
            return(exit_status);
        thisintvl = newintvl;
    }
    newpitch = max(dz->param[COLLOPCH],min(dz->param[COLHIPCH],pivotpitch + thisintvl));
    *newfrq   = (float)miditohz(newpitch);
    return FINISHED;
}

/********************************** PITCH_QUANTISE_ALL **************************/

int pitch_quantise_all(dataptr dz)
{
    int exit_status, at_timehfend = 0;
    double *pstt, *pend, *abs_pend = NULL, *next_pstt = NULL, *next_pend = NULL;
    double thismidi, qmidi, qmidi1, qmidi2, thistime = 0.0, nexttime = 0.0, time, time_ratio = 1.0;
    int n;

    pstt = dz->parray[QUANTPITCH]; 
    if(dz->timedhf) {
        abs_pend = pstt + (dz->quantcnt * dz->itemcnt) - 1;
        thistime = 0.0;
        pend = pstt + dz->quantcnt - 1;
        if(pend >= abs_pend)
            at_timehfend = 1;
        else {
            next_pstt = pstt + dz->quantcnt;
            next_pend = next_pstt + dz->quantcnt - 1;
            nexttime = *next_pstt;
        }
    } else
        pend = pstt + dz->quantcnt - 1;

    for(n=0;n<dz->wlength;n++) {
        if(dz->pitches[n] < MINPITCH) {
            dz->pitches2[n] = dz->pitches[n];
            continue;
        }
        if(dz->timedhf && !at_timehfend){
            time = dz->frametime * n;
            if(time > nexttime) {
                if(next_pstt + dz->quantcnt >= abs_pend) {
                    at_timehfend = 1;
                    pstt++;                 //  Change to the normal convention, MIDI data starts AFTER the time value
                } else {
                    thistime = nexttime;
                    pstt = next_pstt;
                    pend = next_pend;
                    next_pstt += dz->quantcnt;
                    next_pend += dz->quantcnt;
                    nexttime = *next_pstt;
                }
            }
            if(!at_timehfend)
                time_ratio = (time - thistime)/(nexttime - thistime);
        }
        if((exit_status = hztomidi(&thismidi,dz->pitches[n]))<0)
            return(exit_status);

        if(dz->timedhf && !at_timehfend) {
            qmidi1 = pitch_quantise(thismidi,pstt+1,pend,dz);   //  pstt is a time val: Timevarying HF MIDI vals start at pstt+1
            qmidi2 = pitch_quantise(thismidi,next_pstt+1,next_pend,dz);
            qmidi = ((qmidi2 - qmidi1) * time_ratio) + qmidi1;
        } else
            qmidi = pitch_quantise(thismidi,pstt,pend,dz);
        dz->pitches2[n] = (float)miditohz(qmidi);
    }

    //  NOW SMOOTH THE DATA 

    return pitch_smooth(dz);
}

/********************************** PITCH_QUANTISE **************************/

double pitch_quantise(double thismidi,double *pstt, double *pend,dataptr dz)
{
    double *p, qmidi;
    if(thismidi <= *pstt)
        qmidi = *pstt;
    else if(thismidi >= *pend)
        qmidi = *pend;
    else {
        p = pstt;
        while(thismidi > *p)
            p++;
        if((*p - thismidi) > (thismidi - *(p-1)))
            qmidi = *(p-1);
        else
            qmidi = *p;
    }
    return qmidi;
}

/********************************** PITCH_SMOOTH **************************/

int pitch_smooth(dataptr dz) 
{
    int exit_status, min_wcnt;
    float previouspitch;
    
    previouspitch = -1;

    if(dz->vflag[Q_ORNAMENTS])
        min_wcnt = (int)round(ORNAMENT_DUR/dz->frametime);
    else
        min_wcnt = (int)round(STABLE_DUR/dz->frametime);

    //  Pass 1: remove too-brief pitches

    if((exit_status = pitchline_smooth(dz->pitches2,min_wcnt,0,dz))<0)
        return exit_status;

    if(dz->vflag[Q_NOSMOOTH])
        return FINISHED;

    //  Pass 2: put in note-transitions

    return smooth_note_to_note_transitions(dz->pitches2,dz);
}

/********************************** PITCH_RANDOMISE_ALL **************************
 *
 *  Randomisation takes place over time-slots, themselves generated at randon.
 *  Time slots are based on a minimum time-slot, and range up to a max of PRAND_TRANGE * the min.
 *  To ensure we do not get a bias towards the longer slots, we take the sqrt of any value we get gtom drand48()
 *
 *  tslot = (int)round(sqrt(drand48()) * PRAND_TRANGE) * min_wcnt);
 */

int pitch_randomise_all(dataptr dz) 
{
    int exit_status, min_wcnt, tslot, blokcnt;
    double *pstt, *pend, prand_range;
    float thispitch, nextpitch;
    int n, m, newwcnt;

    if(dz->vflag[Q_ORNAMENTS])
        min_wcnt = (int)floor(PRAND_ORN_TSTEP/dz->frametime);
    else
        min_wcnt = (int)floor(PRAND_TSTEP/dz->frametime);

    if(dz->quantcnt) {
        pstt = dz->parray[QUANTPITCH]; 
        pend = dz->parray[QUANTPITCH] + dz->quantcnt - 1;
        blokcnt = 0;
        for(n=0;n<dz->wlength;n++) {                                //  For all previously quantised pitches
            thispitch = dz->pitches2[n];                            //  Get the startpitch of a blok having a fixed-quantised pitch
            m = n;
            do {
                blokcnt++;
                if(++m >= dz->wlength)
                    break;
                nextpitch = dz->pitches2[m];                        //  count the bloksize
            } while(nextpitch == thispitch);
            if(blokcnt > min_wcnt) {                                //  If the block of pitch is longer than the minimum bloksize   
                newwcnt = 0;
                while(newwcnt < blokcnt) {                          //  Generate rand time-slots within the blok
                    prand_range = (double)(blokcnt - newwcnt)/(double)min_wcnt; //  Set range for random-generation between min and remaining bloksize
                    tslot = ((int)round(sqrt(drand48()) * prand_range) * min_wcnt) + min_wcnt;
                    if(newwcnt + tslot > blokcnt - min_wcnt)        //  If the time-slot does not leave enough space for a a futher slot, within the blok
                        tslot = blokcnt - newwcnt;                  //  Forece the time-slot to take up all the remainder of the blok   
                                                                    //  Random-generate a new pitch, and quantise it to the quantisation grid
                    if((exit_status = quantise_randomise_pitchblok(n,tslot,pstt,pend,dz))<0)
                        return exit_status;
                                                                    //  Set the range for random-generation between min and this bloksize
                    newwcnt += tslot;                               //  Keep a running sum of window used within the pitchblok
                }
            } else if((exit_status = quantise_randomise_pitchblok(n,blokcnt,pstt,pend,dz))<0)
                return exit_status;
            blokcnt = 0;
            n = m - 1;                                          //  Advance the outer loop to where inner loop has got to   
        }
    } else {
        newwcnt = 0;                                            //  For all pitches
        for(n=0;n<dz->wlength;n++) {                            
            if((exit_status = randomise_pitchblok(n,min_wcnt,&newwcnt,dz))<0)
                return exit_status;                             //  Rand-transposes a block of data in a random-sized timeslot
            n = newwcnt - 1;                                    //  Outer loop jumps over transformed windows
        }
    }
    return FINISHED;
}

/********************************** RANDOMISE_PITCHBLOK **************************/

int randomise_pitchblok(int start_win,int min_wcnt,int *newwcnt,dataptr dz)
{
    int exit_status, tslot;
    double thisintv;
    int k, m;
    tslot = ((int)round(sqrt(drand48()) * PRAND_TRANGE) * min_wcnt) + min_wcnt;
    if(*newwcnt + tslot > dz->wlength - min_wcnt)       //  If it reaches too close to the end of the pitch data
        tslot = dz->wlength - *newwcnt;                 //  extend slot to end of pitch data.
    if((exit_status = get_rand_interval(&thisintv,dz)) < 0)
        return exit_status;                             //  Add a random interval to all pitches in this slot
    for(k = 0,m = start_win; k < tslot;k++,m++)
        dz->pitches2[m] = (float)(dz->pitches[m] + thisintv);
    *newwcnt += tslot;                                  //  Advance the cnt of transformed windows
    return FINISHED;
}

/********************************** QUANTISE_RANDOMISE_PITCHBLOK **************************/

int quantise_randomise_pitchblok(int n,int wcnt,double *pstt,double *pend,dataptr dz)
{
    int exit_status;
    double thisintv, thispitch;
    int k;
    if((exit_status = get_rand_interval(&thisintv,dz)) < 0) //  Generate random interval
        return  exit_status;
    thispitch =  dz->pitches2[n];                           //  Add interval to current pitchblok pitch
    thispitch += thisintv;
    thispitch =  pitch_quantise(thispitch,pstt,pend,dz);    //  Quantise the resulting pitch
    for(k=0;k <wcnt;k++)                                    //  Refill the pitchblock with the new pitch
        dz->pitches2[n++] = (float)thispitch;
    return FINISHED;
}


/************************** GET_RAND_INTERVAL ************************/

int get_rand_interval(double *thisintv,dataptr dz)
{                                                           
    double wiggle2, wiggle = (((drand48()) * 2.0) - 1.0);   //  Range -1 to 1; Then fabsRange 0 - 1 warped, biasing values to top   
        wiggle2 = pow(fabs(wiggle),0.33);                   //            -                 -
    if(wiggle < 0.0)                                        //          -              -
        wiggle = -wiggle2;                                  //        -     ->>     -   
    else                                                    //      -              -
        wiggle = wiggle2;                                   //    -               -
                                                            //  -                 - 
    *thisintv = dz->param[FPRMAXINT] * wiggle;
    if(wiggle > 0.0) {                                      //  If varies upwards
        if(dz->param[FSLEW] < 1.0)                          //  If slew is (say) 0.5, upper range is 1/2 of lower range, so multiply by slew
            *thisintv *= dz->param[FSLEW];
    } else if(wiggle < 0.0) {                               //  If varies downwards
        if(dz->param[FSLEW] > 1.0)                          //  If slew is (say) 2, lower range is 1/2 of upper range, so divide by slew
            *thisintv /= dz->param[FSLEW];
    }
    return(FINISHED);
}

/************************** FORMANTS_SMOOTH_AND_QUANTISE_ALL ************************/

int formants_smooth_and_quantise_all(dataptr dz)
{
    int exit_status, fno, offset, qdep_param, min_wcnt, at_timehfend = 0;
    int n;
    double thismidi, qmidi, qmidi1, qmidi2, quantintvl, time, thistime = 0, nexttime = 0.0, time_ratio = 1.0;
    double *pstt = NULL, *pend = NULL, *next_pstt = NULL, *next_pend = NULL, *abs_pend = NULL;
    float *ffrq[5], *qfrq[5];

    min_wcnt = (int)floor(PRAND_TSTEP/dz->frametime);   //  Min number of windows to indicate a stable pitch

    ffrq[1] = dz->fptr[FOR_FRQ1];                       
    ffrq[2] = dz->fptr[FOR_FRQ2];                       //  Point to appropriate arrays storing formant frq-data    
    ffrq[3] = dz->fptr[FOR_FRQ3];
    ffrq[4] = dz->fptr[FOR_FRQ4];
    qfrq[1] = dz->fptr[QOR_FRQ1];                       
    qfrq[2] = dz->fptr[QOR_FRQ2];                       //  Point to appropriate arrays storing eventual transposition of formant
    qfrq[3] = dz->fptr[QOR_FRQ3];
    qfrq[4] = dz->fptr[QOR_FRQ4];

    //  Array numbering error-check

    n = 0;
    offset = 6;
    for(fno = 1;fno <= 4;fno++) {
        qdep_param = fno+offset;
        switch(fno) {
        case(1): if(qdep_param != F_QDEP1)  n = 1;  break;
        case(2): if(qdep_param != F_QDEP2)  n = 1;  break;
        case(3): if(qdep_param != F_QDEP3)  n = 1;  break;
        case(4): if(qdep_param != F_QDEP4)  n = 1;  break;
        }
        if(n) {
            fprintf(stdout,"ERROR: Array  numbering mismatch: formants_smooth_and_quantise_all() Change offset, or array numbers.\n");
            return PROGRAM_ERROR;
        }
    }

    if(dz->quantcnt) {
        pstt = dz->parray[QUANTPITCH];
        pend = pstt + dz->quantcnt - 1;                 //  Initialise (for changing or unchanging) Quantisation Data
        if(dz->timedhf)                                 //  Initialise end address of Time-changing Quantisation Data
            abs_pend = pstt + (dz->quantcnt * dz->itemcnt) - 1;
    }

    fprintf(stdout,"INFO: Modifying formant paths.\n");

    //  For every formant-frq trace
    
    for(fno = 1; fno <= 4; fno++) {

        if(dz->timedhf) {
            at_timehfend = 0;
            pstt = dz->parray[QUANTPITCH]; 
            thistime = 0.0;
            pend = pstt + dz->quantcnt - 1;
            if(pend >= abs_pend)
                at_timehfend = 1;
            else {
                next_pstt = pstt + dz->quantcnt;
                next_pend = next_pstt + dz->quantcnt - 1;
                nexttime = *next_pstt;
            }
        }

        //  If data is to be quantised or smoothed

        if(dz->quantcnt || dz->vflag[F_SMOOTH]) {

            //  First convert frq to MIDI
            
            for(n=0;n < dz->wlength; n++) {

                qdep_param = fno+offset;
                if(dz->brksize[qdep_param]) {
                    time = dz->frametime * n;
                    if((exit_status = read_value_from_brktable(time,qdep_param,dz))<0)
                        return exit_status;
                }
                if(dz->timedhf && !at_timehfend){
                    time = dz->frametime * n;
                    if(time > nexttime) {
                        if(next_pstt + dz->quantcnt >= abs_pend) {
                            at_timehfend = 1;                               //  At last data set of gtime-changing data.
                            pstt++;                                         //  Change to standard read convention, ignoring time-value
                        } else {
                            thistime = nexttime;
                            pstt = next_pstt;
                            pend = next_pend;
                            next_pstt += dz->quantcnt;
                            next_pend += dz->quantcnt;
                            nexttime = *next_pstt;
                        }
                    }
                    if(!at_timehfend)
                        time_ratio = (time - thistime)/(nexttime - thistime);
                }
                if(dz->pitches[n] < MINPITCH || ffrq[fno][n] < MINPITCH) {  //  pitch has been smoothed since extracting formants
                    qfrq[fno][n] = -1.0;                                    //  So no processing of formant-pitch, flagged at zero  
                    continue;                                               //  so pitch window may now have been assigned a pitched but have no formant data
                }
                if((exit_status = hztomidi(&thismidi,ffrq[fno][n]))<0)
                    return(exit_status);

                //  If data is to be quantised, quantise it

                if(dz->quantcnt && dz->param[qdep_param] > 0.0) {
                    if(dz->timedhf && !at_timehfend) {

                        qmidi1 = pitch_quantise(thismidi,pstt+1,pend,dz);   //  pstt is a time val: MIDI vals start at pstt+1
                        qmidi2 = pitch_quantise(thismidi,next_pstt+1,next_pend,dz);
                        qmidi = ((qmidi2 - qmidi1) * time_ratio) + qmidi1;
                    } else
                        qmidi = pitch_quantise(thismidi,pstt,pend,dz);
                
            //  and allow for quantisation depth parameter
                    
                    if(dz->param[qdep_param] < 1.0) {
                        quantintvl = qmidi - thismidi;
                        quantintvl *= dz->param[qdep_param];
                        qmidi = thismidi + quantintvl;
                    }
                    thismidi = qmidi;
                }
            //  and store (possibly transformed) MIDI vals in qfrq

                qfrq[fno][n] = (float)thismidi;
            }

            //  If data is to be smoothed, now smooth it

            if(dz->vflag[F_SMOOTH]) {

                if((exit_status = pitchline_smooth(qfrq[fno],min_wcnt,1,dz))<0)
                    return exit_status;
            }

            //  Finally convert back to frq, then replace by frq-transposition value

            for(n=0;n < dz->wlength; n++) {
                if(qfrq[fno][n] < 0.0)                              //  Zero pitch or zero-pitched formant
                    qfrq[fno][n] = 1.0f;                            //  No transposition
                else {
                    qfrq[fno][n] = (float)miditohz(qfrq[fno][n]);   //  Converted MIDI to frq
                    qfrq[fno][n] = qfrq[fno][n]/ffrq[fno][n];       //  Convert frq to frq-transposition value
                }
            }
        } else {
            for(n=0;n < dz->wlength; n++)
                qfrq[fno][n] = 1.0;                                 //  If no processing of formant-pitch, transposition value = 1.0        
        }
    }
    return FINISHED;
}

/************************** FORMANTS_SINUS ************************/

int formants_sinus(int inner_lpcnt,dataptr dz)
{
    int exit_status, n, cc, vc, newcc, pkcnt, thistrof, thispeak, nexttrof, peakcc;
    int preskirt_stt, postskirt_end = 0, nu_preskirt_stt, nu_postskirt_end, skirtlen, offset, famp_param = 0;
    int *ftrn[9];
    float *tranpos[5];
    double skirting, frq, time;
    double ampscale[5];

    skirting = 1.0 - dz->param[F_SINING];   //  F_SINING        Range 0 (full formant) to 1 (sinusoid, single channel)
                                            //  Width of skirt  Range 1 (all of skirt) to 0 (no skirt)
    tranpos[1] = dz->fptr[QOR_FRQ1];                        
    tranpos[2] = dz->fptr[QOR_FRQ2];        //  point to arrays for formant transposition
    tranpos[3] = dz->fptr[QOR_FRQ3];
    tranpos[4] = dz->fptr[QOR_FRQ4];
    ftrn[0] = dz->iparray[F_CHTRAIN1];      //  And flbufptr channels of peaks and trofs
    ftrn[1] = dz->iparray[F_CHTRAIN2];
    ftrn[2] = dz->iparray[F_CHTRAIN3];
    ftrn[3] = dz->iparray[F_CHTRAIN4];
    ftrn[4] = dz->iparray[F_CHTRAIN5];
    ftrn[5] = dz->iparray[F_CHTRAIN6];
    ftrn[6] = dz->iparray[F_CHTRAIN7];
    ftrn[7] = dz->iparray[F_CHTRAIN8];
    ftrn[8] = dz->iparray[F_CHTRAIN9];
    ampscale[1] = dz->param[F_AMP1];
    ampscale[2] = dz->param[F_AMP2];
    ampscale[3] = dz->param[F_AMP3];
    ampscale[4] = dz->param[F_AMP4];

    //  Array numbering error-check

    n = 0;
    offset = 2;
    for(pkcnt = 1;pkcnt <= 4;pkcnt++) {
        famp_param = pkcnt+offset;
        switch(pkcnt) {
        case(1): if(famp_param != F_AMP1)   n = 1;  break;
        case(2): if(famp_param != F_AMP2)   n = 1;  break;
        case(3): if(famp_param != F_AMP3)   n = 1;  break;
        case(4): if(famp_param != F_AMP4)   n = 1;  break;
        }
        if(n) {
            fprintf(stdout,"ERROR: Array  numbering mismatch: formants_sinus() Change offset, or array numbers.\n");
            return PROGRAM_ERROR;
        }
    }

    //  Preset windowbuf to zeroamplitude, and default frequencies

    for(cc = 0, vc = cc*2;cc < dz->clength;cc++,vc+=2) {    
        dz->windowbuf[0][AMPP] = 0.0f;
        dz->windowbuf[0][FREQ] = (float)(cc * dz->chwidth);
    }

    pkcnt = 1;

    //  Copy transposed peaks (& their skirts) into windowbuf
    
    for(n = 0;n < PKBLOK-1;n+=2) {                  //  Going up channo-of-trof-peak data in pairs, where peaks are the 2nd items
        thistrof = n;
        thispeak = n+1;
        nexttrof = n+2;
        peakcc = ftrn[thispeak][inner_lpcnt];       //  Go to the original cc channel the fpeak data was in
        if(skirting > 0) {
            if(n == 0)                              //  Find the size of the peak skirts which must be transformed      
                preskirt_stt = ftrn[thistrof][inner_lpcnt];
            else
                preskirt_stt = postskirt_end;
            postskirt_end = ftrn[nexttrof][inner_lpcnt];

            skirtlen = peakcc - preskirt_stt;
            skirtlen = (int)round(skirtlen * skirting);
            nu_preskirt_stt = peakcc - skirtlen;

            skirtlen = postskirt_end - peakcc;
            skirtlen = (int)round(skirtlen * skirting);
            nu_postskirt_end = peakcc + skirtlen;
        } else {
            nu_preskirt_stt  = peakcc;
            nu_postskirt_end = peakcc;
        }
        if(dz->brksize[famp_param]) {
            time = dz->frametime * n;
            if((exit_status = read_value_from_brktable(time,famp_param,dz))<0)
                return exit_status;
        }                                                                   //  For the peak & the remaining skirt channels 
        for(cc = nu_preskirt_stt, vc = cc*2;cc <= nu_postskirt_end;cc++,vc+=2) {
            frq = dz->flbufptr[0][FREQ] * tranpos[pkcnt][inner_lpcnt];      //  Transpose them, 
            newcc = (int)round(frq/(double)dz->chwidth);                    //  put them in approp channel of windowbuf, with their orig amplitude
                                                        
                                                                            //  Pre modify level of formants, if params set
            if(ampscale[pkcnt] != 1.0)
                dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * ampscale[pkcnt]);

            if(dz->flbufptr[0][AMPP] > dz->windowbuf[0][newcc*2]) {         //  IF there's not already (new) data in that channel with a higher amplitude
                dz->windowbuf[0][newcc*2]   = dz->flbufptr[0][AMPP];
                dz->windowbuf[0][newcc*2+1] = (float)frq;
            }
        }
        pkcnt++;
    }

    //  Replace window by transformed window

    memcpy((char *)dz->flbufptr[0],(char *)dz->windowbuf[0],dz->wanted * sizeof(float));

    if(dz->param[FGAIN] > 0.0 && !flteq(dz->param[FGAIN],1.0))  {
        for(vc=0;vc<dz->wanted;vc+=2)
            dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * dz->param[FGAIN]);
    }
    
    return FINISHED;
}

/************************** STORE_FORMANT_FRQ_DATA ************************/

int store_formant_frq_data(int inner_lpcnt,dataptr dz)
{
    int exit_status, n, pkno, get_fundamental = 0, thistrof, thispeak, nexttrof, pretrof, spstt, spend, ccstt, vcstt, ccend, cc, vc, newcc;
    float frqstt, frqend, the_fundamental = 0.0f, newfrq; 
    double maxamp, minamp;
    int    *peakat = dz->iparray[PEAKPOS], *ftrn[9];
    float *ffrq[5];

    if(dz->fundamental)
        the_fundamental = dz->pitches[inner_lpcnt];

    peakat += inner_lpcnt * PKBLOK;                 //  Go to current position in peak-data array

    ffrq[1] = dz->fptr[FOR_FRQ1];                       
    ffrq[2] = dz->fptr[FOR_FRQ2];                   //  Point to appropriate arrays to store formant frq-data   
    ffrq[3] = dz->fptr[FOR_FRQ3];
    ffrq[4] = dz->fptr[FOR_FRQ4];
    ftrn[0] = dz->iparray[F_CHTRAIN1];              //  And assciated formant-frq channel in flbufptr[0]
    ftrn[1] = dz->iparray[F_CHTRAIN2];
    ftrn[2] = dz->iparray[F_CHTRAIN3];
    ftrn[3] = dz->iparray[F_CHTRAIN4];
    ftrn[4] = dz->iparray[F_CHTRAIN5];              //  And assciated formant-frq channel in flbufptr[0]
    ftrn[5] = dz->iparray[F_CHTRAIN6];
    ftrn[6] = dz->iparray[F_CHTRAIN7];
    ftrn[7] = dz->iparray[F_CHTRAIN8];
    ftrn[8] = dz->iparray[F_CHTRAIN9];              //  And assciated formant-frq channel in flbufptr[0]

    for(n = 0,pkno = 1; n < PKBLOK-1; n+=2,pkno++) { // Going through the trof and peaks in pairs
        thistrof = n;
        thispeak = n+1;
        nexttrof = n+2;
        spstt = peakat[n];                          //  Get specenvamp locations of a trof.
        if(n == 0) {
            if(dz->fundamental && n == 0)
                get_fundamental = 1;

            if(spstt == 0)                          //  Find specenv frqs associated with top and bottom of this trof
                frqstt = 0.0f;
            else
                frqstt = dz->specenvtop[spstt-1];
            frqend = dz->specenvtop[spstt];
            ccstt  = (int)floor(frqstt/dz->chwidth);//  Find the flbufpr channels associated with these frqs
            ccend  = (int)ceil(frqend/dz->chwidth);
            minamp = HUGE;                          //  Find the flbuptr location of minimum amplitude.
            vcstt  = ccstt * 2;
            ftrn[thistrof][inner_lpcnt] = ccstt;
            for(cc = ccstt,vc = vcstt; cc < ccend; cc++,vc+=2) {
                if(dz->flbufptr[0][AMPP] < minamp) {
                    minamp = dz->flbufptr[0][AMPP];
                    ftrn[thistrof][inner_lpcnt] = cc;
                }
            }
        }
        pretrof = ftrn[thistrof][inner_lpcnt];      //   if n>0, value of nexttrof in last loop pass, becomes thistrof in this pass
        spend  = peakat[nexttrof];                  //  Find specenv location of next TROF
        frqstt = dz->specenvtop[spstt];             //  Find the top of the frequency band associated with the lower trof
        frqend = dz->specenvtop[spend - 1];         //  Find the bottom of the frequency band associated with the upper trof
        ccstt  = (int)floor(frqstt/dz->chwidth);    //  Find the flbufpr channels associated with these frqs
        ccend  = (int)ceil(frqend/dz->chwidth);
        minamp = HUGE;                              //  Find the flbuptr location of minimum amplitude.
        ftrn[nexttrof][inner_lpcnt] = ccstt;
        for(cc = ccstt,vc = ccstt*2; cc < ccend; cc++,vc+=2) {
            if(dz->flbufptr[0][AMPP] < minamp) {
                minamp = dz->flbufptr[0][AMPP];
                ftrn[nexttrof][inner_lpcnt] = cc;
            }
        }
        ccstt  = pretrof;                           //  Search between these trofs, for the peak
        ccend  = ftrn[nexttrof][inner_lpcnt];
        maxamp = -HUGE;                             //  Find flbufptr location of PEAK amplitude between these trofs
        ftrn[thispeak][inner_lpcnt] = ccstt;
        for(cc = ccstt,vc = ccstt*2; cc < ccend; cc++,vc+=2) {
            if(dz->flbufptr[0][AMPP] > maxamp) {
                maxamp = dz->flbufptr[0][AMPP];
                ffrq[pkno][inner_lpcnt] = dz->flbufptr[0][FREQ];
                ftrn[thispeak][inner_lpcnt] = cc;
            }
        }
        if(get_fundamental) {                       //  If at peak1 and we want to force getting the fundamental (e.g. if adjacent harmonic louder)
            if(the_fundamental > 0.0) {
                if((exit_status = locate_channel_of_fundamental(inner_lpcnt,&newfrq,&newcc,dz))<0)
                    return exit_status;
                ffrq[pkno][inner_lpcnt] = newfrq;
                ftrn[thispeak][inner_lpcnt] = newcc;
            }
            get_fundamental = 0;                    //  Only (try to) get fundamental when in first formant
        }
    }   
    return FINISHED;
}

/************************** PITCHLINE_SMOOTH ************************/

int pitchline_smooth(float *pichline,int min_wcnt,int ismidi,dataptr dz)
{
    float previouspitch, thispitch;
    double minpitch;
    int n, m, k, wcnt;
    if(ismidi)
        minpitch = MINPITCHMIDI;
    else
        minpitch = MINPITCH;

    previouspitch = -1;

    wcnt = 0;
    for(n = 0; n < dz->wlength-1; n++) {
            if(pichline[n] < minpitch)                          //  Find the next pitch
                continue;                                           //  and start a count of the number of windows it persists for          
        thispitch = pichline[n];
        wcnt = 1;
        for(m = n+1; m < dz->wlength; m++) {                    //  Look at block of pitches beyond "thispitch"...

            if(pichline[m] < minpitch) {                    //  If we hit an unpitched area 
                if(wcnt < min_wcnt) {                           //  IF there were not enough windows in the block of pitch
                    if(previouspitch > 0.0) {                   //  Set them equal to the preceeding pitch (if there is one)
                        for(k = n; k < m; k++)
                            pichline[k] = previouspitch;
                    }
                } else                                          //  Else they were a valid block
                    previouspitch = thispitch;                  //  Set this pitch up as the previouspitch for next search
                break;                                          //  Outer loop will now skip this no-pitch window and proceed
            }                                                   //  Break to get next pitch;
            
            if(pichline[m] == thispitch)                    //  If still on same pitch, count it
                wcnt++;                                         //  Else
            else {                          
                if(wcnt < min_wcnt) {                           //  If we don't have valid wcnt for this pitch                      
                    if(previouspitch > 0.0) {                   //  set it equal to the previous pitch
                        for(k = n; k < m; k++)
                            pichline[k] = previouspitch;
                    }
                }
                break;                                          //  break to the outer loop 
            }
        }
        if(m == dz->wlength) {                                  //  If we didn't break from inner loop
            if(wcnt < min_wcnt) {                               //  If last block of pitch doesn't have valid wcnt for this pitch                       
                if(previouspitch > 0.0) {                       //  set it equal to the previous pitch
                    for(k = n; k < m; k++)
                        pichline[k] = previouspitch;
                }
            }
        }
        n = m-1;                                                //  forces window we've reached to be caught in outer loop
    }
    return FINISHED;
}

/************************** SMOOTH_NOTE_TO_NOTE_TRANSITIONS ************************/

int smooth_note_to_note_transitions(float *pichline,dataptr dz)
{
    double pitchstep;
    float thispitch;
    int n, m, j, k;
    int glide_wins  = (int)round(PCHANGE_DUR/dz->frametime);

    for(n = 0; n < dz->wlength-1; n++) {
        if(pichline[n] < MINPITCH)                              //  Find the next pitch
            continue;                                           //  and start a count of the number of windows it persists for          
        thispitch = pichline[n];
        for(m = n+1; m < dz->wlength; m++) {                    //  Look at block of pitches beyond "thispitch"...
            if(pichline[m] != thispitch) {                      //  If we hit a new pitch or an unpitched area
                if(pichline[m] > MINPITCH && m > glide_wins) {  //  if in new pitch area, and enough windows available todo pitch-glide
                    pitchstep = pichline[m] - thispitch;        //  Get step in pitch
                    pitchstep /= (double)(glide_wins+1);        //  i.e. If 4 wins in transit, these step by 1/5 2/5 3/5 and 4/5 of total pitchstep
                    for(j= 1,k=m-glide_wins;k < m;j++,k++)
                        pichline[k] = (float)(pichline[k] + (pitchstep * j));
                }
                break;                                          //  break to the outer loop 
            }
        }
        n = m-1;
    }
    return FINISHED;
}

/************************** OCTAVIATE_TIMED_HF_DATA ************************/

int octaviate_and_store_timed_hf_data(dataptr dz)
{

    int n, m, j;
    double *pset = dz->parray[QUANTPITCH], *prepitch = dz->parray[PREPICH];
    double nextpitch;
    double octstep;
    int loc = 0, pitchpos = 0;

    for(n=0;n < dz->bincnt;n++)                                     //  Preset the array to max possible quantisation value     
        pset[n] = MIDIMAX;
    for(n = 0; n < dz->itemcnt; n++) {                          //  For every line of entries
        if(pitchpos >= dz->bincnt) {
            sprintf(errstr,"Array overflow 1 in octaviate_and_store_timed_hf_data()\n");
            return PROGRAM_ERROR;
        }
        pset[pitchpos++] = prepitch[loc];                       //  Copy time of pitch-set
        octstep = 0.0;
        for(m=0;m < MIDIOCTSPAN; m++) {                         //  For evey octave in the MIDI range
            for(j = 1;j < dz->quantcnt;j++) {                   //  For every PITCH entry in the line
                nextpitch = prepitch[loc+j] + octstep;          //  Get entry and add octstep
                nextpitch = min(nextpitch,MIDIMAX);
                if(pitchpos >= dz->bincnt) {
                    sprintf(errstr,"Array overflow 2 in octaviate_and_store_timed_hf_data()\n");
                    return PROGRAM_ERROR;
                }
                pset[pitchpos++] = nextpitch;                   //  copy new pitch into final array
            }
            octstep += SEMITONES_PER_OCTAVE;
        }
        loc += dz->quantcnt;                                    //  Go to next set of original entries
    }
    dz->quantcnt--;                                             //  No of quantising items in one timed HF before octaviation
    dz->quantcnt *= MIDIOCTSPAN;                                //  No of quantising items in one timed HF after octaviation
    dz->quantcnt++;                                             //  With the time value
    return FINISHED;
}

/************************** STRIP_END_SPACE ************************/

void strip_end_space(char *p)
{
    char *q = p;
    while(*p != ENDOFSTR)
        p++;
    p--;
    while(isspace(*p)) {
        *p = ENDOFSTR;
        p--;
        if(p == q)
            break;
    }
    return;
}
