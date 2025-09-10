/*
 * Copyright (c) 1983-2023 Trevor Wishart and Composers Desktop Project Ltd
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

//  COMMENT
//  Works now for instruments with spectra defined internally
//  With note-loudness-envelope
//  With vibrato
//  Modified to accept an instrument, time, note, level, duration as parameters.
//  Tested with 1 note and with 2 notes.
//  Modified to accept several instruments and tested with 3 instruments.
//  Modified to make output stereo, and distrib outputs in (fixed) positions in stereo space.
//  Jitter (from quantised times) added
//  All instrument data consolidated into a structure "synstrument"
//  Multichan output option implemented
//  Orient multichan to be in front of listener.

//  TO DO
//  (0) Get (at least) 6 distinguishable instruments working
//  (1) Figure out how to enter data, and how to display entered data, in music-staff format.
//  (2) Work on permutation rules.
//  (3) Generating several outputs and playing side-by-side with input, for comparison.
//
//  This is first stage of creating a multi-instrument synth engine, to test out chanber-music ideas.
//  Currently it does additive synth and packet synth, with input data files.
//  Idea is to choose a set of data values and use these as INTERNAL tables, to synthesize instrument notes.
//
//  The synth must be extended as follows
//  (1) Add vibrato option
//  (2) Control the individual events generated (by each internal-table controlled instrument) from 
//      new external params for pitch, level and duration (could be MIDI,MIDI,quantised-time)
//  (3) From an input score (several instruments) generate an output
//  (4) From an input score, generate several derived scores according to rules of transformation.
//  (5) Play the outputs, and hence select the best outcome .... ETC in an evloving sequence-of-steps. 
//  (6) Convert the output data into staff notation.


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
#include <math.h>
#include <mixxcon.h>
#include <osbind.h>
#include <standalone.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>

#ifdef unix
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

#define SAFETY  256

#define VIBSHIFT    (1.25)  //  Extension of pre-vibrato output, to allow for any possible length-variation with addition of vibrato

#define MS_MM       0
#define MS_MAXJIT   1
#define MS_OCHANS   2

#define MAXOUTDUR   60  //  1 minute
#define MSYNSRATE   44100
//#define   MSYNCHANS   1

#define INS_NO      (0)
#define NOTE_TIME   (0)
#define NOTE_PICH   (1)
#define NOTE_LEVL   (2)
#define NOTE_DUR    (3)

#define MSYNMAXQDUR (768)   //  Assumes time specified in quantisation units of semiquavers, maxdur = semibreve * 8

#define MAXENTRYCNT (129)   //  64 partials+levels and 1 time value
#define MAXLINECNT  (16)    //  16 possible sets of changing spectral data
#define MAXENVPNTS  (8)     //  Maximum number of points specifiying note-envelope for instrument
#define MAXSCORLINE (48)    //  Maximum number of lines (and therefore instruments) in score

#define MAXPARTIALS (64)    //  (MAXENTRYCNT - 1)/2;

#define ROOT2       (1.4142136)
#define SIGNAL_TO_LEFT  (0)
#define SIGNAL_TO_RIGHT (1)

#define UNITSPERSEMIQ 3     //  No of time-units per semiquaver

#define FLUTE_PASSFRQ   260
#define FLUTE_STOPFRQ   200
#define FILTATTEN   -96.0     /* RWD was integer, this seems more idiomatic */

#define TRUMPET     0
#define CLARINET    1
#define PIANO_RH    2
#define PIANO_LH    3
#define FLUTE       4
#define VIOLIN      5
#define CELLO       6

#define vibrato   is_flat
                                    //  Array-indices: indicate which array contains which data

#define sinarray        fzeroset            //  sinetable : for sound and vibrato creation
#define pntarray        zeroset             //  various pointers into sinetable, for all partials and for vibrato
#define pakt_env        is_transpos         //  Envelope of wave-packets used in synth
#define temp_tab        could_be_transpos   //  Temporary storage of modified wave-packet
#define orig_tab        could_be_pitch      //  Temporary storage of original wave-packet
#define ins_spectrum    is_rectified        //  Spectrum of current instrument being used
#define ins_envel       specenv_type        //  Loudness-envelope template of current instrument being used
#define ins_vibd        deal_with_chan_data //  Time-changing data for depth of vibrato for current-note
#define ins_vibf        unspecified_filecnt //  Time-changing data for frq of vibrato for current-note
#define ochan_pos       finished            //  Positions of instrument streams in multichan output
#define scoredata       is_mapping          //  Storage of input score-data
#define partialtabs_cnt ringsize            //  Number of partial-data tables (partialno at time t, partiallevel at time t, for all t)
#define filterbas       duplicate_snds      //  Base of counter of filter arrays    
                                    //  Size of sound and envelope arrays   

#define max_notedur     tempsize            //  samplelength of longest note to be generated
#define maxoutsamp      rampbrksize         //  Total samplelength of output sound

                                    //  Global constants

#define timeconvert     is_sharp            //  Conversion from semiquavers to time at input MM

// ALPHA : instrument specific data

struct synstrument {
char    name[64];
double  *spectrum;
int     partial_cnt;    //  No of partials specified in spectrum
int     line_cnt;       //  No of lines, each specifying spectrum at a different time in its evolution
int     line_entrycnt;  //  Total no of vals to specify line of spec: = line_cnt * (1 + (partialcnt * 2)) (time + all partialno-loudness pairs)
int     spec_len;       //  Total no of vals to specify spec: = line_entrycnt * line_cnt
double  *env;           //  Envelope in time-val pairs, 
int     env_len;        //  env_len counts all entries so with 3 pairs, env_len = 6 : 10 or 12 ONLY !!
int     has_vibrato;    //  flag (0 1)
double  maxvibdepth;    //  max depth of any vibrato, in semitones
int     packettype;     //  If packettype == 1, synthesis by packets, else, simple additive synth
double  squeeze;        //  Narrowing of packet envelope (0 - 1000). Below 1.0 broaden packet. Vals near zero or very high produce clicks or silence.
double  centre;         //  Centring of peak of packet envelope. 0 peak at centre: -1 peak at start: 1 peak at end.
int     rangebot;       //  Lowest note of instrument (MIDI)
int     rangetop;       //  Highest note of instrument (MIDI)
double  balance;        //  Level of instrument relative to other instruments 
int     overlap;        //  Flags if successive instrument-notes can overlap (e.g. piano resonance).
int     doublestop;     //  Max number of simultaneous notes instrument can play (also see rules!!).
};

typedef struct synstrument *synptr;


//  Preloaded (time-changing) spectra and loudness-envelope for synthetic instruments

static int orchestra_size = 7;

static double trumpet[99] =     {0.0,1,0.970464,2,0.991561,3,0.590717,4,0.759494,5,0.632911,6,1.000000,7,0.573840,8,0.358650,
                                    9,0.177215,10,0.033755,11,0.025316,12,0.016878,13,0.016878,14,0.012658,15,0.008439,16,0.012658,
                                0.1,1,0.970464,2,0.991561,3,0.590717,4,0.759494,5,0.632911,6,1.000000,7,0.573840,8,0.358650,
                                    9,0.177215,10,0.033755,11,0.025316,12,0.016878,13,0.016878,14,0.012658,15,0.008439,16,0.012658,
                                1.0,1,0.970464,2,0.991561,3,0.590717,4,0.759494,5,0.632911,6,1.000000,7,0.573840,8,0.358650,
                                    9,0.177215,10,0.033755,11,0.025316,12,0.016878,13,0.016878,14,0.012658,15,0.008439,16,0.012658};
static double trumpenv[12] =   {0.0, 0,
                                0.035,1,
                                0.085, 1,
                                0.125,.3,
                                0.9,.3,
                                1.0, 0};

static double clarinet[42] =   {0.0,1,1,3,0.9,5,0.8,7,0.7,9,0.6,11,0.5,13,0.4,15,0.3,17,0.2,19,0.1,
                                1.0,1,1,3,0.9,5,0.8,7,0.7,9,0.6,11,0.5,13,0.4,15,0.3,17,0.2,19,0.1};
static double clarenv[12]  =   {0.0, 0,
                                0.035,1,
                                0.085, 1,
                                0.125, 1,
                                0.9,   1,
                                1.0, 0};

static double flute[62]    =   {0.0, 1,1.000000,2,0.028184,3,0.031623,4,0.007943,5,0.008913,6,0.003548,7,0.003162,8,0.001778,
                                     9,0.001778,10,0.001585,11,0.001585,12,0.001413,13,0.001413,14,0.001259,15,0.001259,
                                1.0, 1,1.000000,2,0.028184,3,0.031623,4,0.007943,5,0.008913,6,0.003548,7,0.003162,8,0.001778,
                                     9,0.001778,10,0.001585,11,0.001585,12,0.001413,13,0.001413,14,0.001259,15,0.001259};
static double flutenv[12]  =   {0.0, 0,
                                0.065,0.5,
                                0.085,1,
                                0.125,1,
                                0.9,  1,
                                1.0, 0};

static double piano[100]   =   {0.0,1,1,2,1    ,4,1  ,7,1    ,8,1  , 12,1    ,17,1  ,18,1   ,19,1,   33.67,1,   21.7262,1,   30.4134,1,
                                0.3,1,1,2,0.9  ,4,0.8, 7,0.7  ,8,0.6, 12,0.5  ,17,0.4,18,0.3 ,19,0.3, 33.67,0.3 ,21.7262,0.15,30.4134,0.15,
                                0.6,1,.5,2,0.45 ,4,0.4,7,0.35 ,8,0.3, 12,0.25 ,17,0.2,18,0.15,19,0.15,33.67,0.1 ,21.7262,0.15,30.4134,0.15,
                                1.0,1,.5,2,0.225,4,0.2,7,0.175,8,0.15,12,0.125,17,0.1,18,0.07,19,0.03,33.67,0.03,21.7262,0.0 ,30.4134,0.0};
static double pianoenv[12] =   {0.0, 1,
                                0.035,0.5,
                                0.07,0.25,
                                0.1, 0.125,
                                1.0, 0.0625,
                                3.0, 0};

static double violin[100]    =  {0.0,1,1.0,2,0.707956,3,0.501184,4,0.141243,5,0.199535,6,0.177826,7,0.281860,8,0.281860,9,0.089114,10,0.223880,11,0.158485,12,0.141243,
                                0.07,1,1.0,2,0.707956,3,0.501184,4,0.141243,5,0.199535,6,0.177826,7,0.281860,8,0.281860,9,0.089114,10,0.223880,11,0.158485,12,0.141243,
                                0.09,1,1.000000,2,0.501202,3,0.251185,4,0.019950,5,0.039814,6,0.031622,7,0.079445,8,0.079445,9,0.007941,10,0.050122,11,0.025117,12,0.019950,
                                1.0, 1,1.000000,2,0.501202,3,0.251185,4,0.019950,5,0.039814,6,0.031622,7,0.079445,8,0.079445,9,0.007941,10,0.050122,11,0.025117,12,0.019950};

static double violenv[12]  =   {0.0, 0,
                                0.07,1,
                                0.085,1,
                                0.125,1,
                                0.9,  1,
                                1.0, 0};

static double cello[92]    =  {0.0,1,1,2,.5,3,.07,4,.33,5,0.09,6,0.09,7,0.11,8,0.1,9,0.1,10,0.045,11,0.02,
                                0.07,1,1,2,.5,3,.07,4,.33,5,0.09,6,0.09,7,0.11,8,0.1,9,0.1,10,0.045,11,0.02,
                                0.09,1,1,2,.5,3,.07,4,.33,5,0.09,6,0.09,7,0.11,8,0.1,9,0.1,10,0.045,11,0.02,
                                1.0, 1,1,2,.5,3,.07,4,.33,5,0.09,6,0.09,7,0.11,8,0.1,9,0.1,10,0.045,11,0.02};
static double cellenv[12]  =   {0.0, 0,
                                0.07,1,
                                0.085,1,
                                0.125,1,
                                0.8,  1,
                                1.0, 0};

char errstr[2400];

//static int testing = 0;

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int setup_synthesizer_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int setup_synthesis_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(char *cmdline,dataptr dz);
static int open_the_outfile(dataptr dz);
static int setup_and_init_input_param_activity(dataptr dz,int tipc);
static int setup_input_param_defaultval_stores(int tipc,aplptr ap);
static int establish_application(dataptr dz);
static int initialise_vflags(dataptr dz);
static int setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int mark_parameter_types(dataptr dz,aplptr ap);
static int assign_file_data_storage(int infilecnt,dataptr dz);
static int get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
//static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

static int synthesis_param_preprocess(dataptr dz);
static int score_synthesis(int inlinecnt,double onehzincr,synptr *orchestra,int flt_cnt,double flt_mul,dataptr dz);
static void incr_sinptr(int n,double time,double onehzincr,double frq,double *partialfrq,dataptr dz);
static double read_level(int n,double time,dataptr dz);
static int create_synthesizer_sndbufs(dataptr dz);
static int generate_packet_envelope (dataptr dz);
static double read_packet_envelope(int kk,double incr,dataptr dz);
static int modify_packet_envelope(double synth_ctr,double synth_sqz,dataptr dz);
//static double get_frq_with_vibrato(int *init,int n,double onehzincr,double srate,dataptr dz);

static int pretest_the_special_data(char *cmdline,int *inlinecnt,synptr *orchestra,dataptr dz);
static int setup_the_special_data_for_given_instr(int instrno,double *data,synptr instrument,dataptr dz);
static int setup_the_data_arrays(int linecnt,double synth_ctr,double synth_sqz,dataptr dz);
static int impose_note_envelope(float *obuf,double *notenv,int envlen,int sampcnt,int linecnt,int notecnt,synptr instrument,dataptr dz);
static void generate_vibrato_curves(double maxdepth, double notedur, dataptr dz);
static int add_packet_vibrato(int *init,double onehzincr,double srate,int *sampcnt,int packet_dur,dataptr dz);
static int read_instrument_name_from_start_of_line(char **p,char *insnam,int *insno,synptr *orchestra,int linecnt);
static int note_synthesis(int totaloutsamps,double frq,double levl,double onehzincr,int linecnt,int notecnt,int envlen,int *sampcnt,
                synptr instrument,int instrno,int flt_cnt,double flt_mul,dataptr dz);
static int retest_count_and_store_the_special_data(char *str,synptr *orchestra,int *flt_cnt,double *flt_mul,dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static int initialise_instruments(synptr **orchestra);
static int check_clustering(int insno,int chordcnt,int doublestop,char *insname,int time, int *midival);
static int valid_cluster(int insno,double *linestor,int datacnt,synptr *orchestra);
static int add_vibrato(double onehzincr,double srate,int outsampcnt,float *obuf,float *obuf2,dataptr dz);
static int msetup_lphp_filter(int *flt_cnt,double *flt_mul,double passfrq,double stopfrq,dataptr dz);
static int mdo_lphp_filter(int sampcnt,int flt_cnt,double flt_mul,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status, inlinecnt, flt_cnt = 0;
    dataptr dz = NULL;
    synptr *orchestra = NULL;
    char **cmdline;
    char indatafile[400];
    int  cmdlinecnt;
    double synth_ctr = 0.0, synth_sqz = 0.0, flt_mul = 0.0;
//    aplptr ap;
    int is_launched = FALSE;
    double onehzincr = (double)SYNTH_TABSIZE/(double)MSYNSRATE;

    srand(16);
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
    if((exit_status = initialise_instruments(&orchestra))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
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
        }
    }
    if(!sloom) {
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
        dz->maxmode = 0;
        // setup_particular_application =
        if((exit_status = setup_synthesizer_application(dz))<0) {
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
//    ap = dz->application;
    // parse_infile_and_hone_type() = 
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_synthesis_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // open_first_infile() : redundant
    // handle_extra_infiles() : redundant
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(cmdline[0],dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    cmdlinecnt--;
    cmdline++;
//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
//  handle_special_data()
    if((exit_status = pretest_the_special_data(cmdline[0],&inlinecnt,orchestra,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    strcpy(indatafile,cmdline[0]);
    cmdlinecnt--;
    cmdline++;
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = synthesis_param_preprocess(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency() redundant

    if((exit_status = setup_the_data_arrays(inlinecnt,synth_ctr,synth_sqz,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = retest_count_and_store_the_special_data(indatafile,orchestra,&flt_cnt,&flt_mul,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    if((exit_status = create_synthesizer_sndbufs(dz))<0) {                          // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
// Must come AFTER the special_data function

    if((exit_status = open_the_outfile(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //process_file =
    if((exit_status = score_synthesis(inlinecnt,onehzincr,orchestra,flt_cnt,flt_mul,dz))<0) {
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
    //RWD 2025 was ap->max_param_cnt+1
        if((ap->param_list = (char *)malloc((size_t)(ap->max_param_cnt+2)))==NULL) {    
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
    //THERE ARE NO INPUTFILE brktables USED IN THIS PROCESS
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
    dz->infilecnt = 1;
    //establish_bufptrs_and_extra_buffers():
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

/************************ HANDLE_THE_OUTFILE *********************/

int handle_the_outfile(char *str,dataptr dz)
{
    char filename[2000];
    char *p,*q;
    strcpy(filename,str);
    if(!sloom) {
        if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
            sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
            return(DATA_ERROR);
        }
    }
    q = filename;
    p = filename;                   //  Drop file extension
    while(*p != ENDOFSTR) {
        if(*p == '.') {
            *p = ENDOFSTR;
            break;
        }
        p++;
    }
    p--;
    while(p != filename) {          //  Drop file path
        if(*p == '/') {
            q = p+1;
            break;
        }
        p--;
    }
    strcpy(dz->outfilename,q);
    return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
    int exit_status;
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
        return(exit_status);
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

/************************* SETUP_SYNTHESIZER_APPLICATION *******************/

int setup_synthesizer_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,SCOREDATA,1,1,"dD"))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"jo",2,"di","b",1,0,"0"))<0)
        return(exit_status);
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = NO_FILE_AT_ALL;
    dz->process_type    = UNEQUAL_SNDFILE;  
    dz->outfiletype     = SNDFILE_OUT;
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
        } else if(infile_info->filetype != SNDFILE)  {
            sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
            return(DATA_ERROR);
        } else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
            sprintf(errstr,"Failed to copy file parsing information\n");
            return(PROGRAM_ERROR);
        }
        free(infile_info);
    }
    return(FINISHED);
}

/************************* SETUP_SYNTHESIS_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_synthesis_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[MS_MM]   = 30.0;
    ap->hi[MS_MM]   = 500.0;
    ap->default_val[MS_MM]  = 60.0;
    ap->lo[MS_MAXJIT]   = 0;
    ap->hi[MS_MAXJIT]   = 20;
    ap->default_val[MS_MAXJIT]  = 15;
    ap->lo[MS_OCHANS]   = 2;
    ap->hi[MS_OCHANS]   = 8;
    ap->default_val[MS_OCHANS]  = 2;
    dz->maxmode = 0;
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
//    aplptr ap;

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
            if((exit_status = setup_synthesizer_application(dz))<0)
                return(exit_status);
//           ap = dz->application;
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

int read_special_data(char *str,dataptr dz) 
{
    return(FINISHED);
}

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
    return(FINISHED);
}

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    return(FINISHED);
}


/******************************** USAGE1 ********************************/

int usage1(void)
{
    usage2("synth");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"synth"))               dz->process = MULTISYN;
    else {
        fprintf(stderr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        return(USAGE_ONLY);
    }
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

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"synth")) {
        fprintf(stderr,
        "USAGE:\n"
        "multisynth synth outfile score MM [-jjitter] [-oochans] [-b]\n"
        "\n"
        "Synthesize several sound-streams from a score.\n"
        "\n"
        "SCORE  Text file with each line in the form:.\n"
        "       \"Insname\" followed by any number of sets-of-4-values representing\n"
        "       \"Time\"  \"Pitch\"  \"Loudness\"  and  \"Duration\".\n"
        "       e.g.  (with two 4-sets) \"trumpet 0 62 .7 2 8 64 .9 12\"\n"
        "\n"
        "Insname    name of instrument: one of the following.\n"
        "           \"flute\"   \"clarinet\"  \"trumpet\"  \"violin\"  \"cello\"\n"
        "           \"pianoRH\" \"pianoLH\" (LH must immediately follow RH in score file).\n"
        "\n"
        "Time       Note onset time, measured in thirds-of-semiquavers from time 0 (integer).\n"
        "           \"Cello\" & \"violin\" & and \"piano\" (LH and RH)\n"
        "           may play up to 4 different notes simultaneously,\n"
        "           so long as the note-combinations are possible on the real instrument.\n"
        "\n"
        "Pitch      Pitch of the note, as a MIDI value (integer).\n"
        "Loudness   A positive number no bigger than 1.\n"
        "Duration   Duration of the note, measured in thirds-of-semiquavers (integer).\n"
        "           \"Piano\" notes may overlay one another (sustained resonance)\n"
        "           but other instruments may not, so their duration values must not\n"
        "           create a note sustained beyond the start of the following note.\n"
        "           Simultaneously sounding notes must have the same duration.\n"
        "\n"
        "MM     Metronome mark determining tempo of sound output.\n"
        "JITTER Max random divergence from regular note placement in mS (default 15).\n"
        "OCHANS Number of output channels (2-8): default stereo.\n"
        "       The sounds in successive lines will be spaced in the output-space\n"
        "       from line-1, at the far left, to the last-line, at far right.\n"
        "       (single-line score output will be placed at front centre).\n"
        "\n"
        "-b     If \"ochans\" is greater than stereo, lspkr array assumed to be surround.\n"
        "       To force a bounded array (with a left extreme and right extreme)\n"
        "       use the \"-b\" flag.\n"
        "       For surround, outputs numbered in order clockwise from front-centre.\n"
        "       For bounded, outputs numbered in order from far-left to far-right.\n"
        "\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/**************************** SYNTHESIS_PARAM_PREPROCESS *************************/

int synthesis_param_preprocess (dataptr dz)
{
    dz->infile->channels = dz->iparam[MS_OCHANS];
    dz->infile->srate = MSYNSRATE;
    dz->nyquist = (double)MSYNSRATE/2.0;
    // Duration of thirds of semiquavers
    dz->timeconvert = (60.0/dz->param[MS_MM])/12.0;
    dz->param[MS_MAXJIT] *= MS_TO_SECS;
    return(FINISHED);
}

/******************************** SCORE_SYNTHESIS *********************************/

#define LEFT 0
#define RITE 1

int score_synthesis(int inlinecnt,double onehzincr,synptr *orchestra,int flt_cnt,double flt_mul,dataptr dz)
{
    int exit_status, linecnt, m, typ, instrno, notecnt, pos, envlen, outchans = dz->iparam[MS_OCHANS], leftchan, ritechan, packet_type;
    double *data, *env = dz->parray[dz->ins_envel], *ochanpos = dz->parray[dz->ochan_pos];
    double srate = (double)MSYNSRATE, normaliser = 0.0, synth_ctr = 1.0, synth_sqz = 0.0, changain[2], jitter;
    double time, frq = 0.0, levl = 1.0, dur = 0.0, maxval;
    float *obuf = dz->sampbuf[0], *notebuf = dz->sampbuf[1];
    unsigned int startsamp = 0, totaloutsamps = 0, outsampcnt;
    int notesamps = 0, sampcnt, n;
    synptr instrument;
    memset((char *)obuf,0,dz->buflen * sizeof(float));
    if(!sloom)
        fprintf(stdout,"INFO: First Score-pass. ");
    for(linecnt = 0, pos = 0; linecnt < inlinecnt; linecnt++,pos +=3) {
        leftchan = (int)round(ochanpos[pos]);
        ritechan = (leftchan + 1) % outchans;
        changain[LEFT] = ochanpos[pos+1];
        changain[RITE] = ochanpos[pos+2];
        notecnt = 0;
        data = dz->parray[dz->scoredata + linecnt];
        instrno = (int)round(data[INS_NO]);
        instrument = orchestra[instrno];                //  Point to relevant instrument-structure
        data++; //  Move into note data
        if((exit_status = setup_the_special_data_for_given_instr(instrno,data,instrument,dz))<0)
            return(exit_status);
        packet_type = instrument->packettype;
        if(packet_type) {
            synth_sqz  = instrument->squeeze;               //  Centring of peak of packet envelope.
            synth_ctr  = instrument->centre;                //  Narrowing of packet envelope.
            if((exit_status = modify_packet_envelope(synth_ctr,synth_sqz,dz))<0)
                return(exit_status);
        }
        m = 0;
        while(data[m] >= 0) {
            typ = m % 4;
            switch(typ) {
            case(NOTE_TIME):
                time = data[m] * dz->timeconvert;
                if(time > 0.0 && dz->param[MS_MAXJIT] > 0.0) {
                    jitter = drand48() * 2.0;
                    jitter -= 1.0;
                    jitter *= dz->param[MS_MAXJIT];
                    time += jitter;
                    time = max(time,0.0);
                }
                startsamp = (int)round(time * srate) * outchans;    //  Position of note in total output
                break;
            case(NOTE_PICH):
                frq = miditohz(data[m]);
                break;
            case(NOTE_LEVL):
                levl = data[m];
                break;
            case(NOTE_DUR):
                dur = data[m] * dz->timeconvert;
                if(instrument->overlap)                 //  If "overlap" allow note to decay completely
                    dur = max(dur,instrument->env[instrument->env_len - 2]);
                notesamps = (int)round(dur * srate);    //  Length of note in (MONO) note-synth buf
                break;
            }
            m++;
            if(m%4 == 0) {                              //  Once a whole note-data-set (4 items) is gathered
                notecnt++;
                envlen  = instrument->env_len;          //  (envlen modifiable later, if note is very short)
                if(instrument->has_vibrato)
                    generate_vibrato_curves(instrument->maxvibdepth,dur,dz);
                for(n=0;n < envlen;n++)                 // copy complete instrument envelope template into note-envelope array
                    env[n] = instrument->env[n];        // Get note envelope (this will be modified when true duration of note is known)
                if((exit_status = note_synthesis(notesamps,frq,levl,onehzincr,linecnt,notecnt,envlen,&sampcnt,instrument,instrno,flt_cnt,flt_mul,dz))<0)
                    return(exit_status);
                for(n = 0,outsampcnt = startsamp;n < sampcnt;n++,outsampcnt+=outchans) {
                    obuf[outsampcnt+leftchan] = (float)(obuf[outsampcnt+leftchan] + (notebuf[n] * levl * changain[LEFT]));
                    obuf[outsampcnt+ritechan] = (float)(obuf[outsampcnt+ritechan] + (notebuf[n] * levl * changain[RITE]));
                }
                totaloutsamps = max(totaloutsamps,outsampcnt);
            }
        }
    }
    maxval = 0.0;
    for(outsampcnt=0;outsampcnt<totaloutsamps;outsampcnt++)
        maxval = max(maxval,fabs(obuf[outsampcnt]));
    normaliser = 0.95/maxval;
    if(!sloom) {
        fprintf(stdout,"\nNormalising output.\n");
        fflush(stdout);
    }
    for(outsampcnt=0;outsampcnt<totaloutsamps;outsampcnt++)
        obuf[outsampcnt] = (float)(obuf[outsampcnt]  * normaliser);
    if((exit_status = write_samps(obuf,totaloutsamps,dz))<0)
        return(exit_status);
    return FINISHED;
}

/******************************** NOTE_SYNTHESIS *********************************/

int note_synthesis(int totaloutsamps,double frq,double levl,double onehzincr,int linecnt,int notecnt,int envlen,int *sampcnt,synptr instrument,int instrno,
                   int flt_cnt,double flt_mul,dataptr dz)
{
    int exit_status, n;
    int loindex, hiindex, packet_dur = 0, kk, total_outsamps;
    double loval, hival, valdiff, timefrac, val, level, maxval = 0.0, packet_incr = 0, envv, passfrq, stopfrq;
    float *obuf = dz->sampbuf[1], *obuf2 = dz->sampbuf[2];
    double srate = (double)dz->infile->srate;
    double *sintab = dz->parray[dz->sinarray], *sinptr = dz->parray[dz->pntarray], *notenv = dz->parray[dz->ins_envel];
    double time = 0.0, normaliser, partialfrq = HUGE;
    int packet_phase = 1, partialcnt;
    int init, packet_type;
    packet_type = instrument->packettype;
    if(packet_type) {
        packet_dur  = (int)round((1.0/frq) * srate);
        packet_incr = (double)TREMOLO_TABSIZE/(double)(packet_dur - 1); //  Forces last read to be at end of packet envelope (zero)
    }
    if(!sloom)
        fprintf(stdout,"*");
    *sampcnt = 0;
    init = 1;
    memset((char *)obuf,0,dz->max_notedur * sizeof(float));
    memset((char *)obuf2,0,dz->max_notedur * sizeof(float));

    for(n=0;n<instrument->partial_cnt;n++) {
        partialfrq = frq * dz->parray[n][1];    //  Get initial partial-frq
        if(partialfrq >= dz->nyquist)           //  Ignore any partials beyond the nyquist
            break;
    }
    partialcnt = n;
    if(packet_type) {
        while(*sampcnt < totaloutsamps) {
            for(n=0;n<partialcnt;n++)
                sinptr[n] = 0.0;
            for(kk = 0; kk<packet_dur;kk++) {
                time = (double)(*sampcnt)/srate;
                for(n=0;n<partialcnt;n++) {
                    loindex = (int)floor(sinptr[n]);
                    hiindex = loindex + 1;
                    loval   = sintab[loindex];
                    hival   = sintab[hiindex];
                    valdiff = hival - loval;
                    timefrac = sinptr[n] - (double)loindex;
                    val = loval + (valdiff * timefrac);
                    level = read_level(n,time,dz);
                    val *= level;
                    obuf[*sampcnt] = (float)(obuf[*sampcnt] + val);
                    incr_sinptr(n,time,onehzincr,frq,&partialfrq,dz);
                }
                envv = read_packet_envelope(kk,packet_incr,dz);
                obuf[*sampcnt] = (float)(obuf[*sampcnt] * envv * packet_phase);
                (*sampcnt)++;
            }
            if(instrument->has_vibrato)
                add_packet_vibrato(&init,onehzincr,srate,sampcnt,packet_dur,dz);
            packet_phase = -packet_phase;
        }
        for(kk=0;kk<*sampcnt;kk++)
            maxval = max(maxval,fabs(obuf[kk]));
    } else {
        for(n=0;n<partialcnt;n++)
            sinptr[n] = 0.0;
        if(instrument->has_vibrato)
            total_outsamps = (int)round(totaloutsamps * VIBSHIFT);   // Generate more than required, to allow for vibrato
        else
            total_outsamps = totaloutsamps;
        for(kk = 0; kk<total_outsamps;kk++) {
            time = (double)kk/srate;
            for(n=0;n<partialcnt;n++) {
                loindex = (int)floor(sinptr[n]);
                hiindex = loindex + 1;
                loval   = sintab[loindex];
                hival   = sintab[hiindex];
                valdiff = hival - loval;
                timefrac = sinptr[n] - (double)loindex;
                val = loval + (valdiff * timefrac);
                level = read_level(n,time,dz);
                val *= level;
                obuf[kk] = (float)(obuf[kk] + val);
                incr_sinptr(n,time,onehzincr,frq,&partialfrq,dz);
            }
        }
        if(instrument->has_vibrato)
            add_vibrato(onehzincr,srate,totaloutsamps,obuf,obuf2,dz);
        if(instrno == FLUTE) {
            passfrq = FLUTE_PASSFRQ;
            stopfrq = FLUTE_STOPFRQ;
            if((exit_status = msetup_lphp_filter(&flt_cnt,&flt_mul,passfrq,stopfrq,dz))<0)
                return exit_status;
            if((exit_status = mdo_lphp_filter(totaloutsamps,flt_cnt,flt_mul,dz))<0)
                return exit_status;
            if((exit_status = mdo_lphp_filter(totaloutsamps,flt_cnt,flt_mul,dz))<0)
                return exit_status;
        }
        for(kk=0;kk<total_outsamps;kk++)
            maxval = max(maxval,fabs(obuf[kk]));
    }
    packet_phase = 1;
    normaliser = levl/maxval;

    time = 0.0;
    *sampcnt = 0;
    if(!sloom)
        fprintf(stdout,"*");
    memset((char *)obuf,0,dz->max_notedur * sizeof(float));
    memset((char *)obuf2,0,dz->max_notedur * sizeof(float));
    init = 1;
    if(packet_type) {
        while(*sampcnt < totaloutsamps) {
            for(n=0;n<partialcnt;n++)               //  Zero sine-table pointers for all partials, at start of packet
                sinptr[n] = 0.0;
            for(kk = 0; kk<packet_dur;kk++) {
                time = (double)(*sampcnt)/srate;
                for(n=0;n<partialcnt;n++) {         
                    loindex = (int)floor(sinptr[n]);            //  Read from sintable, using partial-increment, for each partial
                    hiindex = loindex + 1;
                    loval   = sintab[loindex];
                    hival   = sintab[hiindex];
                    valdiff = hival - loval;
                    timefrac = sinptr[n] - (double)loindex;
                    val = loval + (valdiff * timefrac);
                    level = read_level(n,time,dz);              //  Read corresponding level
                    val *= level;
                    obuf[*sampcnt] = (float)(obuf[*sampcnt] + val);
                    incr_sinptr(n,time,onehzincr,frq,&partialfrq,dz);   //  Track (modify if ness) the partial-incr value for this partial
                }
                                                            //  Once all partial-samples added, impose packet envelope
                envv = read_packet_envelope(kk,packet_incr,dz);
                obuf[*sampcnt] = (float)(obuf[*sampcnt] * envv * normaliser * packet_phase);
                (*sampcnt)++;
            }
            if(instrument->has_vibrato)
                add_packet_vibrato(&init,onehzincr,srate,sampcnt,packet_dur,dz);
            packet_phase = -packet_phase;
        }
    } else {
        for(n=0;n<partialcnt;n++)
            sinptr[n] = 0.0;
        if(instrument->has_vibrato)
            total_outsamps = (int)round(totaloutsamps * VIBSHIFT);   // Generate more than required, to allow for vibrato
        else
            total_outsamps = totaloutsamps;
        for(kk = 0; kk<total_outsamps;kk++) {
            time = (double)kk/srate;
            for(n=0;n<partialcnt;n++) {
                loindex = (int)floor(sinptr[n]);
                hiindex = loindex + 1;
                loval   = sintab[loindex];
                hival   = sintab[hiindex];
                valdiff = hival - loval;
                timefrac = sinptr[n] - (double)loindex;
                val = loval + (valdiff * timefrac);
                level = read_level(n,time,dz);
                val *= level;
                obuf[kk] = (float)(obuf[kk] + val);
                incr_sinptr(n,time,onehzincr,frq,&partialfrq,dz);
            }
            obuf[kk] = (float)(obuf[kk] * normaliser);
        }
        if(instrument->has_vibrato)
            add_vibrato(onehzincr,srate,totaloutsamps,obuf,obuf2,dz);
        if(instrno == FLUTE) {
            if((exit_status = mdo_lphp_filter(totaloutsamps,flt_cnt,flt_mul,dz))<0)
                return exit_status;
            if((exit_status = mdo_lphp_filter(totaloutsamps,flt_cnt,flt_mul,dz))<0)
                return exit_status;
        }
        *sampcnt = totaloutsamps;
    }
    if((exit_status = impose_note_envelope(obuf,notenv,envlen,totaloutsamps,linecnt,notecnt,instrument,dz))<0)
        return exit_status;
    return FINISHED;
}

/**************************** IMPOSE_NOTE_ENVELOPE ****************************/

int impose_note_envelope(float *obuf,double *notenv,int envlen,int sampcnt,int linecnt,int notecnt,synptr instrument,dataptr dz)
{
    double time, srate = (double)MSYNSRATE, envval, dur, fade,  endenvtime;
    int n;
    int envnext = 2;
    double thistime, thisval, nexttime, nextval, timediff, valdiff, tratio;
    endenvtime = instrument->env[instrument->env_len-2];
    thistime = notenv[0];
    thisval  = notenv[1]; 
    nexttime = notenv[2], 
    nextval  = notenv[3];
    timediff = nexttime - thistime;
    valdiff  = nextval - thisval;

    dur  = (double)sampcnt/srate;

    if(instrument->overlap)

        //  Ensure (post-vibrato) duration of note does not spill over end of envelope data

        dur = min(endenvtime,dur);

    else {

        //  Curtail or extend prototype instr envelope to (post-vibrato) duration of note

        if(envlen == 12) {                  //  rise, on, down, hold,off
            fade = notenv[10] - notenv[8];
            if(dur > notenv[6] + fade) {    //   x x
                notenv[10] = dur;           //        x    <-x->
                notenv[8]  = dur - fade;    //  x           <-x->
            } else if(dur > notenv[6]) {
                notenv[6] = dur;            //   x x
                notenv[7] = 0.0;            //          envlen NOW = 8
                                            //  x     x
            } else if(dur > notenv[4]) {
                notenv[4] = dur;            //   x
                notenv[5] = 0.0;            //          envlen NOW = 6
                                            //  x  x
            } else {
                sprintf(errstr,"Note too short in line %d : note %d\n",linecnt+1,notecnt);
                return DATA_ERROR;
            }
        } else {                        //  rise, on, down, decay
            if(dur > notenv[6]) {           //   x x
                notenv[8] = dur;            //       x
                                            //  x        <-x->
            } else if(dur > notenv[4]) {
                notenv[4] = dur;            //   x
                notenv[5] = 0.0;            //          envlen NOW = 6
                                            //  x  x
            } else {
                sprintf(errstr,"Note too short in line %d : note %d\n",linecnt+1,notecnt);
                return DATA_ERROR;
            }
        }
    }
    for(n=0;n<sampcnt;n++) {
        time = (double)n/srate;
        if(time == 0.0 || time >= dur)
            envval = 0.0;
        else {
            while (nexttime <= time) {
                thistime = nexttime;
                thisval  = nextval;
                envnext += 2;
                nexttime = notenv[envnext];
                nextval  = notenv[envnext+1];
                timediff = nexttime - thistime;
                valdiff  = nextval - thisval;
            }
            tratio = (time - thistime)/timediff;
            envval = (valdiff * tratio) + thisval;
        }
        obuf[n] = (float)(obuf[n] * envval * instrument->balance);
    }
    memset((char *)(obuf + sampcnt),0,(dz->max_notedur - sampcnt) * sizeof(float));
    return FINISHED;
}

/**************************** INCR_SINPTR ****************************/

void incr_sinptr(int n,double time,double onehzincr,double frq,double *partialfrq,dataptr dz)
{
    int m;
    double  hival, loval, hitime, lotime, timediff, timefrac, valdiff, partialval, thisincr;
    double *sinptr = dz->parray[dz->pntarray];
    double *thispartial = dz->parray[n];
    m = 0;
    while(thispartial[m] < time) {
        m += 2;
        if(m >= dz->partialtabs_cnt)
            break;
    }
    if(m==0)
        partialval = thispartial[1];
    else if(m < dz->partialtabs_cnt) {
        hival  = thispartial[m+1];
        loval  = thispartial[m-1];
        hitime = thispartial[m];
        lotime = thispartial[m-2];
        timediff = hitime - lotime;
        timefrac = (time - lotime)/timediff;
        valdiff  = hival - loval;
        partialval = loval + (valdiff * timefrac);
    } else
        partialval = thispartial[dz->partialtabs_cnt-1];

    //  Convert partial numbers to table-increments

    thisincr = partialval * onehzincr;
    thisincr *= frq;
    sinptr[n] += thisincr;
    if(sinptr[n] >= SYNTH_TABSIZE)
        sinptr[n] -= (double)SYNTH_TABSIZE;
}

/**************************** READ_LEVEL ****************************/

double read_level(int n,double time,dataptr dz)
{
    int m;
    double  hival, loval, hitime, lotime, timediff, timefrac, valdiff, level;
    double *thislevel = dz->parray[n + MAXPARTIALS];
    m = 0;
    while(thislevel[m] < time) {
        m += 2;
        if(m >= dz->partialtabs_cnt)
            break;
    }
    if(m==0) {
        level = thislevel[1];
    } else if(m < dz->partialtabs_cnt) {
        hival  = thislevel[m+1];
        loval  = thislevel[m-1];
        hitime = thislevel[m];
        lotime = thislevel[m-2];
        timediff = hitime - lotime;
        timefrac = (time - lotime)/timediff;
        valdiff  = hival - loval;
        level = loval + (valdiff * timefrac);
    } else {
        level = thislevel[dz->partialtabs_cnt-1];
    }
    return level;
}

/**************************** CREATE_SYNTHESIZER_SNDBUFS ****************************/

int create_synthesizer_sndbufs(dataptr dz)
{
    unsigned int bigbufsize;
    int framesize, segsize;
    int safety = (int)(dz->param[MS_MAXJIT] * dz->infile->srate) * dz->infile->channels;
    dz->bufcnt = 3;
    framesize = F_SECSIZE * dz->infile->channels;
    if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    dz->buflen = dz->maxoutsamp + dz->max_notedur + safety; //  Allow for jitter and vibrato
    segsize = dz->buflen / framesize;
    if(segsize * framesize < dz->buflen)
        segsize++;
    dz->buflen = segsize * framesize;
    bigbufsize = (dz->buflen + (2 * dz->max_notedur)) * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;                           //  Whole output sound
    dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;          //  Output note
    dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->max_notedur;     //  Any vibrato transform of output note
    dz->sampbuf[3]                  = dz->sampbuf[2] + dz->max_notedur;
    return(FINISHED);
}

/**************************** GENERATE_PACKET_ENVELOPE *************************/

int generate_packet_envelope (dataptr dz)
{
    int n;
    double *costab, *origtab;
    if((dz->parray[dz->pakt_env] = (double *)malloc((TREMOLO_TABSIZE + 1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }
    costab = dz->parray[dz->pakt_env];
    if((dz->parray[dz->temp_tab] = (double *)malloc((TREMOLO_TABSIZE + 1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[dz->orig_tab] = (double *)malloc((TREMOLO_TABSIZE + 1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }
    origtab = dz->parray[dz->orig_tab];
    for(n=0;n<TREMOLO_TABSIZE;n++) {
        costab[n] = cos(PI * 2.0 * ((double)n/(double)TREMOLO_TABSIZE));
        costab[n] += 1.0;
        costab[n] /= 2.0;
        costab[n] = 1.0 - costab[n];
        origtab[n] = costab[n];
    }
    costab[n]  = 0.0; /* wrap around point */
    origtab[n] = 0.0; /* wrap around point */
    return(FINISHED);
}

/**************************** MODIFY_PACKET_ENVELOPE *************************/

int modify_packet_envelope (double synth_ctr,double synth_sqz,dataptr dz)
{
    int n, halftabsize = TREMOLO_TABSIZE/2;
    int isneg = 0, tablopos, tabhipos;
    double *costab, *temptab, *origtab, diff, tabrem, tabincr, lotabincr, hitabincr, readpos, frac;

    costab  = dz->parray[dz->pakt_env];
    temptab = dz->parray[dz->temp_tab];
    origtab = dz->parray[dz->orig_tab];
    if(flteq(synth_sqz,1.0)) {
        for(n=0;n<=TREMOLO_TABSIZE;n++)
            temptab[n] = origtab[n];
    } else {
        for(n=0;n<=TREMOLO_TABSIZE;n++)
            temptab[n] = pow(origtab[n],synth_sqz);
    }
    if(flteq(synth_ctr,1.0)) {
        for(n=0;n<=TREMOLO_TABSIZE;n++)
            costab[n] = temptab[n];
    } else {
            if(synth_ctr < 0.0) {
                frac = 1.0 + synth_ctr;
                isneg = 1;
            } else 
                frac = 1.0 - synth_ctr;
            if(isneg) {
                lotabincr = 1.0/frac;
                hitabincr = 1.0/(2.0 - frac);
            } else {
                lotabincr = 1.0/(2.0 - frac);
                hitabincr = 1.0/frac;
            }
            readpos = 0;
            tabincr = lotabincr;
            for(n=0;n<TREMOLO_TABSIZE;n++) {
                if(readpos >= halftabsize) {
                    tabincr = hitabincr;
                }
                tablopos = (int)floor(readpos);
                tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
                tabrem    = readpos - (double)tablopos;
                diff      = temptab[tabhipos] - temptab[tablopos];
                costab[n] = temptab[tablopos] + (diff * tabrem);
                readpos += tabincr;
            }
    }
    return(FINISHED);
}

/**************************** READ_PACKET_ENVELOPE *************************/

double read_packet_envelope(int kk,double incr,dataptr dz)
{
    double *costab, tabpos, tabrem, diff, envv;
    int tablopos, tabhipos;
    costab = dz->parray[dz->pakt_env];
    tabpos = (double)kk * incr;
    tablopos = (int)floor(tabpos);
    tabhipos = min(tablopos + 1,TREMOLO_TABSIZE);
    tabrem   = tabpos - (double)tablopos;
    diff = costab[tabhipos] - costab[tablopos];
    envv = costab[tablopos] + (diff * tabrem);
    return envv;
}

/**************************** SETUP_THE_DATA_ARRAYS ****************************/

int setup_the_data_arrays(int inlinecnt,double synth_ctr,double synth_sqz,dataptr dz)
{
    int exit_status;
    double *sintab;
    int n;
    int zz;
    dz->array_cnt = (MAXPARTIALS * 2) + inlinecnt + 10 + 7; 
                                            //  An array for every partial-pno, every partial-level, 
                                            //  An array for every input line of data
                                            //  + (0) snd-sintable 
                                            //  + (1) sintab-incr-pointers 
                                            //  + (2-4) packet envelope + 2 packet-envelope-temp-arrays
                                            //  + (5) storage of instrument-note-spectral_data 
                                            //  + (6) storage if instr-note-envelope-data   
                                            //  + (7) storage of note-vibrato-depth-curve
                                            //  + (8) storage of note-vibrato-frq-curve
                                            //  + (9) multichan positions of output streams
                                            //  + 7 for filter calculations             
    if((dz->parray = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create partial data arrays.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n <dz->array_cnt;n++)
        dz->parray[n] = NULL;
    dz->filterbas = dz->array_cnt - 7;
    zz = MAXPARTIALS * 2;
    for(n=0;n <zz;n++) {                    //  2 entries (time and value) for every line in the data.
        if((dz->parray[n] = (double *)malloc((MAXLINECNT * 2) * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store partial data.\n");
            return(MEMORY_ERROR);
        }
    }
    dz->scoredata    = (MAXPARTIALS * 2);   
    dz->sinarray     = (MAXPARTIALS * 2) + inlinecnt;
    dz->pntarray     = dz->sinarray + 1;
    dz->pakt_env     = dz->sinarray + 2;
    dz->temp_tab     = dz->sinarray + 3;
    dz->orig_tab     = dz->sinarray + 4;
    dz->ins_spectrum = dz->sinarray + 5;
    dz->ins_envel    = dz->sinarray + 6;
    dz->ins_vibd     = dz->sinarray + 7;
    dz->ins_vibf     = dz->sinarray + 8;
    dz->ochan_pos    = dz->sinarray + 9;

    //  Establish sine-table

    if((dz->parray[dz->sinarray] = (double *)malloc((SYNTH_TABSIZE + 1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }
    sintab = dz->parray[dz->sinarray];
    for(n=0;n<SYNTH_TABSIZE;n++)
        sintab[n] = sin(PI * 2.0 * ((double)n/(double)SYNTH_TABSIZE));
    sintab[n] = sintab[0];                          /* wrap around point */

    //  Pointers into sintable for all partials, and one for vibrato read

    if((dz->parray[dz->pntarray] = (double *)malloc((MAXPARTIALS+1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }

    if((exit_status = generate_packet_envelope(dz))<0)
        return(exit_status);

    for(n=0;n<=MAXPARTIALS;n++)             //  Zero sine-table pointers for all partials and for vibrato
        dz->parray[dz->pntarray][n] = 0.0;

    //  Max size array for time-changing spectrum of instrument

    if((dz->parray[dz->ins_spectrum] = (double *)malloc(MAXENTRYCNT * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }

    //  Max size array for loudness envelope of instrument

    if((dz->parray[dz->ins_envel] = (double *)malloc((MAXENVPNTS * 2) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }

    //  Small arrays for vibrato params of notes

    if((dz->parray[dz->ins_vibd] = (double *)malloc(6 * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[dz->ins_vibf] = (double *)malloc(6 * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }

    //  Array for multichan output positions (channo of left-chan plus pair of L-R levels) of streams

    if((dz->parray[dz->ochan_pos] = (double *)malloc((MAXSCORLINE * 3) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }
    return FINISHED;
}
    
/**************************** GENERATE_VIBRATO_CURVES *************************/

void generate_vibrato_curves(double maxdepth, double notedur, dataptr dz)
{
    double *vibdepth = dz->parray[dz->ins_vibd];
    double *vibfrq   = dz->parray[dz->ins_vibf];
    double time, depth, frq;
    vibdepth[0] = 0.0;
    vibdepth[1] = 0.0;
    time = drand48() * notedur/2;
    time +=     notedur/4;
    vibdepth[2] = time;
    depth = drand48() * maxdepth/2; 
    depth += maxdepth/2;
    vibdepth[3] = depth;
    vibdepth[4] = notedur;
    vibdepth[5] = 0.0;
    frq = drand48() * 4.0;
    frq += 4.0;
    vibfrq[0] = 0.0;
    vibfrq[1] = frq;
    frq = drand48() * 4.0;
    frq += 4.0;
    vibfrq[2] = time;
    vibfrq[3] = frq;
    frq = drand48() * 4.0;
    frq += 4.0;
    vibfrq[4] = notedur;
    vibfrq[5] = frq;
}

/**************************** ADD_VIBRATO *************************/

int add_vibrato(double onehzincr,double srate,int outsampcnt,float *obuf,float *obuf2,dataptr dz)
{
    int vibnext = 2;
    double *vib_depth = dz->parray[dz->ins_vibd], obufptr = 0.0;
    double *vib_frq   = dz->parray[dz->ins_vibf];
    double *sintab = dz->parray[dz->sinarray];
    static double thistime, thisfrq, thisdepth, nexttime, nextfrq, nextdepth, timediff, depthdiff, frqdiff, endtime;
    double time, vibdepth, vibfrq, timefrac, tratio, val, loval, hival, valdiff, frac;
    int loindex, hiindex, sampcnt, obufi = 0;
    double vib_sinpntr = dz->parray[dz->pntarray][MAXPARTIALS];

    //  Initialise variables
    
    thistime = 0.0;
    thisdepth = vib_depth[1];
    nexttime  = vib_depth[2];
    nextdepth = vib_depth[3];
    endtime   = vib_depth[4];
    thisfrq   = vib_frq[1];
    nextfrq   = vib_frq[3];
    depthdiff = nextdepth - thisdepth;
    frqdiff   = nextfrq - thisfrq;
    timediff  = nexttime - thistime;
    vib_sinpntr = 0.0;

    sampcnt = 0;
    obuf2[sampcnt++] = obuf[0];

    while(sampcnt < outsampcnt) {

        //  Write vibratoed value to obuf2

        time = (double)sampcnt/srate;
        obufi = (int)floor(obufptr);
        loval = obuf[obufi];
        hival = obuf[obufi+1];
        valdiff = hival - loval;
        frac = obufptr - obufi;
        val = loval + (valdiff * frac);
        obuf2[sampcnt] = (float)val;

        //  Find current frequency and depth of vibrato
        
        if(time >= endtime) {
            vibdepth = vib_depth[5];
            vibfrq   = vib_frq[5];
        } else {
            while (nexttime <= time) {
                thistime  = nexttime;
                thisdepth = nextdepth;
                thisfrq   = nextfrq;
                vibnext += 2;
                nexttime  = vib_depth[vibnext];
                nextdepth = vib_depth[vibnext+1];
                nextfrq   = vib_frq[vibnext+1];
                depthdiff = nextdepth - thisdepth;
                frqdiff   = nextfrq - thisfrq;
                timediff  = nexttime - thistime;
            }
            tratio = (time - thistime)/timediff;
            vibfrq   = (frqdiff * tratio) + thisfrq;
            vibdepth = (depthdiff * tratio) + thisdepth;
        }

        //  Use current pointer-position for vibrato, in sintable, to read sinusoidal vibrato-divergence

        loindex = (int)floor(vib_sinpntr);  //  Read from sintable, using vibrato-sinptr
        hiindex = loindex + 1;
        loval   = sintab[loindex];
        hival   = sintab[hiindex];
        valdiff = hival - loval;
        timefrac = vib_sinpntr - (double)loindex;
        val = loval + (valdiff * timefrac);

        //  Advance vibrato sin-table-read-pointer
        
        vib_sinpntr += vibfrq * onehzincr;
        if(vib_sinpntr >= SYNTH_TABSIZE)
            vib_sinpntr -= (double)SYNTH_TABSIZE;

        //  Multiply sinusoidal val by current semitone-depth of vib
        
        val *= vibdepth;
            // convert semitone-offset to frq-ratio (>1 for up, <1 for down) and advance in pre-vibrato sound by frq-ratio
        obufptr += SEMITONES_AS_RATIO(val);
        sampcnt++;
    }       //  Copy vibratoed output to original obuf
    memset((char *)obuf,0,dz->max_notedur * sizeof(float));
    memcpy((char *)obuf,(char *)obuf2,sampcnt * sizeof(float));
    return FINISHED;
}

/**************************** ADD_PACKET_VIBRATO *************************/

int add_packet_vibrato(int *init,double onehzincr,double srate,int *sampcnt,int packet_dur,dataptr dz)
{
    int vibnext = 2;
    double *vib_depth = dz->parray[dz->ins_vibd];
    double *vib_frq   = dz->parray[dz->ins_vibf];
    double *sintab = dz->parray[dz->sinarray];
    static double thistime, thisfrq, thisdepth, nexttime, nextfrq, nextdepth, timediff, depthdiff, frqdiff, endtime;
    double time, vibdepth, vibfrq, timefrac, tratio, val, loval, hival, valdiff, incr;
    int loindex, hiindex, new_packet_dur, packet_shift;
    double *vib_sinpntr = &(dz->parray[dz->pntarray][MAXPARTIALS]);


    //  Initialise variables
    
    if(*init) {
        thistime = 0.0;
        thisdepth = vib_depth[1];
        nexttime  = vib_depth[2];
        nextdepth = vib_depth[3];
        endtime   = vib_depth[4];
        thisfrq   = vib_frq[1];
        nextfrq   = vib_frq[3];
        depthdiff = nextdepth - thisdepth;
        frqdiff   = nextfrq - thisfrq;
        timediff  = nexttime - thistime;
        *vib_sinpntr = 0.0;
        *init = 0;
    }

    //  Find current frequency and depth of vibrato
    
    time = (double)(*sampcnt)/srate;
    if(time == 0.0) {
        vibdepth = thisdepth;
        vibfrq   = thisfrq;
    } else if(time >= endtime) {
        vibdepth = vib_depth[5];
        vibfrq   = vib_frq[5];
    } else {
        while (nexttime <= time) {
            thistime  = nexttime;
            thisdepth = nextdepth;
            thisfrq   = nextfrq;
            vibnext += 2;
            nexttime  = vib_depth[vibnext];
            nextdepth = vib_depth[vibnext+1];
            nextfrq   = vib_frq[vibnext+1];
            depthdiff = nextdepth - thisdepth;
            frqdiff   = nextfrq - thisfrq;
            timediff  = nexttime - thistime;
        }
        tratio = (time - thistime)/timediff;
        vibfrq   = (frqdiff * tratio) + thisfrq;
        vibdepth = (depthdiff * tratio) + thisdepth;
    }

    //  Use current pointer-position for vibrato, in sintable, to read sinusoidal vibrato-divergence

    loindex = (int)floor(*vib_sinpntr); //  Read from sintable, using vibrato-sinptr
    hiindex = loindex + 1;
    loval   = sintab[loindex];
    hival   = sintab[hiindex];
    valdiff = hival - loval;
    timefrac = *vib_sinpntr - (double)loindex;
    val = loval + (valdiff * timefrac);

    //  Advance vibrato sin-table-read-pointer by duration of one packet
    
    *vib_sinpntr += vibfrq * onehzincr * packet_dur;
    if(*vib_sinpntr >= SYNTH_TABSIZE)
        *vib_sinpntr -= (double)SYNTH_TABSIZE;

    //  Multiply sinusoidal val by current semitone-depth of vib
    
    val *= vibdepth;

    // convert semitone offset to frq-ratio : THEN wavelen (packet) duration = 1.0/frq-ratio

    incr = 1.0/SEMITONES_AS_RATIO(val);
        
    new_packet_dur = (int)round((double)packet_dur * incr);
    packet_shift = new_packet_dur - packet_dur;
    *sampcnt += packet_shift;
    return FINISHED;
}

/**************************** SETUP_THE_SPECIAL_DATA_FOR_GIVEN_INSTR ****************************/

int setup_the_special_data_for_given_instr(int instrno,double *data,synptr instrument,dataptr dz)
{
    double val, timeval = 0.0;
    int timepos, valpos, pno_cnt, lev_cnt = 0;
    double *instr = dz->parray[dz->ins_spectrum], *env = dz->parray[dz->ins_envel];
    int lstart;
    int line, mm, cnt;

    //  Clearly arrays for spectrum and envelope

    memset((char *)instr,0, MAXENTRYCNT * sizeof(double));
    memset((char *)env,0, (MAXENVPNTS * 2) * sizeof(double));

    // ALPHA : Setup instrument specific spectrum

    instr   = instrument->spectrum; //  instrument spectrum
    timepos = 0;                    //  Pointer to time-values in all arrays
    valpos  = 1;                    //  Pointer to val-at-time in all arrays
    pno_cnt = 0;                    //  Pointer to partial-pno table
    lstart  = MAXPARTIALS;          //  Start of partial-level table
    for(line=0; line < instrument->line_cnt; line++) {
        mm = instrument->line_entrycnt * line;          //  Index of entry in the instrument array, at start of current line
        for(cnt = 0;cnt < instrument->line_entrycnt;cnt++) {
            val = instr[mm];
            switch(cnt) {
            case(0):
                pno_cnt = 0;                            //  Point to start of pnos, and levels
                lev_cnt = lstart;
                timeval = val;
                break;
            default:
                if(ODD(cnt)) {                          //  Put pno in appropriate pno-array
                    dz->parray[pno_cnt][timepos] = timeval;
                    dz->parray[pno_cnt][valpos]  = val;
                    pno_cnt++;
                } else {                                //  Put level in appropriate level-array
                    dz->parray[lev_cnt][timepos] = timeval;
                    dz->parray[lev_cnt][valpos]  = val;
                    lev_cnt++;
                }
                break;
            }
            mm++;
        }
        timepos += 2;                                   //  Advance pointers in pno and level tables        
        valpos  +=2;
    }
    dz->partialtabs_cnt = instrument->line_cnt * 2;     //  Store lengths of partial tables (1 time and 1 value entry from each dataline)
    return(FINISHED);
}

/**************************** PRETEST_THE_SPECIAL_DATA ****************************/

int pretest_the_special_data(char *str,int *inlinecnt,synptr *orchestra,dataptr dz)
{
    double dummy = 0.0, lasttime = 0.0, lastpitch[16];
    synptr instrument;
    double timestep;
    int insno, typ, coincident = 0, /* doublestop,*/ lastdur, pianoRHcnt = 0, pianoLHcnt = 0, pianoRHpos = -1, pianoLHpos = -1;
    FILE *fp;
    int cnt, linecnt, n, m;
    char temp[8000], insnam[200],*p;
    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read times.\n",str);
        return(DATA_ERROR);
    }
    linecnt = 1;
    while(fgets(temp,8000,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        if((read_instrument_name_from_start_of_line(&p,insnam,&insno,orchestra,linecnt)) < 0) {
            return DATA_ERROR;
        }
        if(!(strcmp(insnam,"pianoRH"))) {
            pianoRHcnt++;
            if(pianoRHcnt > 1) {
                sprintf(errstr,"Cannot handle more than 1 piano right-hand.\n");
                return DATA_ERROR;
            }
            pianoRHpos = linecnt;
        } else if(!(strcmp(insnam,"pianoLH"))) {
            pianoLHcnt++;
            if(pianoLHcnt > 1) {
                sprintf(errstr,"Cannot handle more than 1 piano left-hand.\n");
                return DATA_ERROR;
            }
            pianoLHpos = linecnt;
        }
        instrument = orchestra[insno]; // ALPHA : Sets up instrument specific pitch-ranges
//        doublestop = instrument->doublestop;
        cnt = 0;
        lastdur = 0;
        while(get_float_from_within_string(&p,&dummy)) {
            typ = cnt % 4;
            switch(typ) {
            case(0):            //  time
                if(cnt > 0) {
                    timestep = dummy - lasttime;
                    if(timestep < 0) {
                        sprintf(errstr,"Times do not advance (%lf %lf) in line %d\n",lasttime,dummy,linecnt);
                        return DATA_ERROR;
                    }
                    if(timestep == 0)
                        coincident++;
                    else if(timestep < 3) {
                        sprintf(errstr,"Times do not advance sufficiently (%d to %d = %d units) in line %d (min: 3 units)\n",(int)round(lasttime),(int)round(dummy),(int)round(dummy - lasttime),linecnt);
                        return DATA_ERROR;
                    } else
                        coincident = 1;
                    if(coincident > instrument->doublestop) {
                        sprintf(errstr,"Too many coincident notes at time %lf in line %d\n",lasttime,linecnt);
                        return DATA_ERROR;
                    }
                } else
                    coincident = 1;
                lasttime = dummy;
                break;
            case(1):            //  pitch
                if(dummy < instrument->rangebot || dummy > instrument->rangetop) {
                    sprintf(errstr,"Pitch value (%d) out of range (%d to %d) for instrument %s line %d\n",(int)round(dummy),instrument->rangebot,instrument->rangetop,insnam,linecnt);
                    return DATA_ERROR;
                }
                lastpitch[coincident - 1] = dummy;
                if(coincident > 1) {
                    for(n = 0;n < coincident-1;n++) {
                        for(m = n+1;m < coincident;m++) {
                            if(lastpitch[n] == lastpitch[m]) {
                                sprintf(errstr,"Identical Pitch values (%d) used at same time (%d) for instrument %s line %d\n",(int)round(lastpitch[n]),(int)round(lasttime),insnam,linecnt);
                                return DATA_ERROR;
                            }
                        }
                    }
                }
                break;
            case(2):            //  level
                if(dummy <= 0.0 || dummy > 1.0) {
                    sprintf(errstr,"Level value (%lf) out of range (>0 to 1) for instrument %s\n",dummy,insnam);
                    return DATA_ERROR;
                }
                break;
            case(3):            //  dur
                if(dummy < 1 || dummy > MSYNMAXQDUR) {
                    sprintf(errstr,"Duration in semiquavers (%d) out of range (1 to %d) for instrument %s\n",(int)round(dummy),MSYNMAXQDUR,insnam);
                    return DATA_ERROR;
                }
                if(coincident > 1 && dummy != lastdur) {
                    sprintf(errstr,"Notes in \"%s\" at same time %d, have different durations (%d and %d)\n",insnam,(int)round(lasttime),lastdur,(int)round(dummy));
                    return DATA_ERROR;
                }
                lastdur = (int)round(dummy);
                break;
            }
            cnt++;
        }
        if(cnt % 4 != 0) {
            sprintf(errstr,"Invalid number of numeric entries (%d) on line %d (must be in sets of 4: time,pitch,level,duration).\n",cnt,linecnt);
            return(DATA_ERROR);
        }
        linecnt++;
    }
    linecnt--;
    fclose(fp);
    if(linecnt == 0) {
        sprintf(errstr,"No significant data found in score file.\n");
        return(DATA_ERROR);
    }
    
    if(linecnt >= MAXSCORLINE) {
        sprintf(errstr,"Too many data lines (%d) found in score file (max %d).\n",linecnt,MAXSCORLINE);
        return(DATA_ERROR);
    }
    *inlinecnt = linecnt;
    if(pianoLHcnt != pianoRHcnt) {
        sprintf(errstr,"Piano LH and RH not both present.\n");
        return(DATA_ERROR);
    }
    if(pianoRHpos >= 0 && pianoRHpos+1 != pianoLHpos) {
        sprintf(errstr,"Piano LH does not immediately follow piano RH.\n");
        return(DATA_ERROR);
    }
    return FINISHED;
}

/**************************** RETEST_COUNT_AND_STORE_THE_SPECIAL_DATA ****************************
 *
 *  In Pass 1
 *  (a) makes an instrument dependent test, to check that successive notes don't overlap, if this not possible for specific instrument.
 *  (b) uses dz->timeconvert, derived from the input MM parameter to calculate sizes of buffers required.
 *  (c) creates the correct-sized arrays to store the scoredata lines.
 *  In Pass 2
 *  (a)stores the scoredata lines.
 */

int retest_count_and_store_the_special_data(char *str,synptr *orchestra,int *flt_cnt,double *flt_mul,dataptr dz)
{
    double dummy = 0.0, lastdur = 0.0, maxdur = 0.0, maxnotedur = 0.0, lasttime = 0.0, timestep, leftgain, rightgain;
    double *linestor, *ochanpos = dz->parray[dz->ochan_pos], sustain = 0.0, lrpos;
    int insno, cnt, typ, linecnt, n, m, itimestep, idummy, ilastdur = 0, ochans = dz->iparam[MS_OCHANS];
    int surround = 0, leftmostchan, ispiano = -1, instrcnt = 0, coincidence;
    synptr instrument;
    FILE *fp;
    char temp[8000], insnam[200],*p;
    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read times.\n",str);
        return(DATA_ERROR);
    }
    if(ochans > 2 && !dz->vflag[0])
        surround = 1;
    linecnt = 0;
    coincidence = 1;
    while(fgets(temp,8000,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        read_instrument_name_from_start_of_line(&p,insnam,&insno,orchestra,(int)(linecnt+1));
        if(!strcmp(insnam,"pianoRH"))
            ispiano = linecnt;
        instrument = orchestra[insno];          //  ALPHA Establish note-overlap type
        if(instrument->overlap)                 //  If sustaining instrument, check entire decay-length of note
            sustain = instrument->env[instrument->env_len - 2];
        else
            sustain = 0.0;
        cnt = 0;
        while(get_float_from_within_string(&p,&dummy)) {
            typ = cnt % 4;
            switch(typ) {
            case(0):            //  time
                if(cnt > 0) {
                    idummy = (int)floor(dummy);
                    if(idummy != dummy) {
                        sprintf(errstr,"Bad time value (%lf) (Times must be integers) on line %d (%s).\n",dummy,linecnt+1,insnam);
                        return(DATA_ERROR);
                    }
                    if((idummy % 3 != 0) && (idummy % 4 != 0))  {
                        sprintf(errstr,"Bad time value (%d) (Times must be multiples of 3 or 4) on line %d (%s).\n",idummy,linecnt+1,insnam);
                        return(DATA_ERROR);
                    }
                    timestep = dummy - lasttime;
                    itimestep = (int)round(timestep);
                    timestep *= dz->timeconvert;        //  Convert thirds-of-semiquavers to seconds
                    if(itimestep == 0) {
                        coincidence++;
                        if(coincidence > instrument->doublestop) {
                            sprintf(errstr,"Too many coincident notes at time %d in line %d (%s)\n",ilastdur,linecnt+1,insnam);
                            return DATA_ERROR;
                        }
                    } else if(itimestep < UNITSPERSEMIQ) {
                        sprintf(errstr,"Timestep between entries %d and %d (= %d) too small (MIN 3) on line %d (%s)\n",(int)round(lasttime),(int)round(dummy),itimestep,linecnt+1,insnam);
                        return DATA_ERROR;
                    } else
                        coincidence = 1;
                    if(instrument->doublestop < 2 && timestep < lastdur) {      // Test instrument for note-overlaps
                        sprintf(errstr,"Duration (%d) at time %d too long for timestep (%d) in line %d (%s)\n",ilastdur,(int)round(lasttime),itimestep,linecnt+1,insnam);
                        return DATA_ERROR;
                    }
                } else
                    coincidence = 1;
                lasttime = dummy;
                break;
            case(3):            //  dur
                idummy = (int)floor(dummy);
                if(idummy != dummy) {
                    sprintf(errstr,"Bad duration value (%lf) (Durations must be integers) on line %d (%s).\n",dummy,linecnt+1,insnam);
                    return(DATA_ERROR);
                }
                if((idummy % 3 != 0) && (idummy % 4 != 0))  {
                    sprintf(errstr,"Bad duration (%d) (must be multiples of 3 or 4) on line %d (%s).\n",idummy,linecnt+1,insnam);
                    return(DATA_ERROR);
                }
                if(coincidence > 1) {
                    if(idummy != ilastdur) {
                        sprintf(errstr,"Coincident notes must have same duration (time %d on line %d  : %s).\n",(int)round(lasttime),linecnt+1,insnam);
                        return(DATA_ERROR);
                    }
                }
                ilastdur = idummy;
                lastdur = dummy * dz->timeconvert;      //  Convert thirds-of-semiquavers to seconds
                lastdur = max(lastdur,sustain);
                maxdur = max(maxdur,lasttime+lastdur);
                maxnotedur = max(maxnotedur,lastdur);
                break;
            }
            cnt++;
        }
        if((dz->parray[dz->scoredata+linecnt] = (double *)malloc((cnt+1) * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for score line %d.\n",linecnt+1);
            return(MEMORY_ERROR);
        }
        linecnt++;
    }
    dz->maxoutsamp  = (int)ceil(maxdur * (double)MSYNSRATE) * dz->iparam[MS_OCHANS];
    dz->max_notedur = (int)ceil(maxnotedur * 2 * (double)MSYNSRATE);    //  Instrument streams are mono, but we have to allow for vibrato

    //  Position the instrument streams in the multichan-output space
    
        //  single instrument-stream

    instrcnt = linecnt;
    if(ispiano >= 0)
        instrcnt--;

    if(instrcnt == 1) {
        if(ochans == 2) {                   //  if stereo, create central image
            ochanpos[0] = 0;                //  Left channel is channel 1(0)
            ochanpos[1] = 0.5;              //  signal on 1+2
            ochanpos[2] = 0.5;
        } else if(surround) {               //  if multichan-surround, create image on front-centre channel 1(0)
            ochanpos[0] = 0;                //  Left channel is channel 1(0): front centre
            ochanpos[1] = 1.0;              //  Signal on 1
            ochanpos[2] = 0.0;
        } else {                            //  if multichan linear, create image on central channel(s)
            if(ODD(ochans)) {
                ochanpos[0] = ochans/2;     //  e.g. 7 --> 3
                ochanpos[1] = 1.0;          //  all signal on 3
                ochanpos[2] = 0.0;
            } else {
                ochanpos[0] = ochans/2 - 1; //  e.g. 8 --> 3
                ochanpos[1] = 0.5;          //  signal on 3+4
                ochanpos[2] = 0.5;
            }
        }
        if(ispiano) {                       //  Both LH and RH on same channel
            if(ochans == 2) {
                ochanpos[3] = 0;
                ochanpos[4] = 0.5;
                ochanpos[5] = 0.5;
            } else if(surround) {
                ochanpos[3] = 0;
                ochanpos[4] = 1.0;
                ochanpos[5] = 0.0;
            } else {
                if(ODD(ochans)) {
                    ochanpos[3] = ochans/2;
                    ochanpos[4] = 1.0;
                    ochanpos[5] = 0.0;
                } else {
                    ochanpos[3] = ochans/2 - 1;
                    ochanpos[4] = 0.5;
                    ochanpos[5] = 0.5;
                }
            }
        }
    } else if(instrcnt <= ochans) {

        //  1 instrument to a channel

        for(n = 0,m = 0; m < linecnt * 3; n++,m+=3) {
            ochanpos[m] = n;        //  Assign each instrument-stream to (successive) output channel
            ochanpos[m+1] = 1.0;    //  All level on this channel
            ochanpos[m+2] = 0.0;    //  None on adjacent channel
            if(n == ispiano) {      //  Force both piano outputs to same channel
                n--;
                ispiano = -1;       //  Prevent this happening again
            }
        }

        if(surround) {              //  If sound_surround, orient instruments to front of space.
            leftmostchan = ochans/2 + 1;
            for(n = 0,m = 0; n < linecnt; n++,m+=3)
                ochanpos[m] = (double)(((int)round(ochanpos[m]) + leftmostchan) % ochans);
        }

    } else if(ochans == 2 || dz->vflag[0]) {

        //  if linear array, distribute instruments equispaced over multichannel linear array

        for(n = 0,m = 0;m < (linecnt-1) * 3; n++,m+=3) {
            if(n >= instrcnt - 1) {                             //  If piano (double entry) is last item : say instrcnt = 5, but linecnt =6
                ochanpos[m]   = (double)(ochans - 1);           //  then n will reach instrcnt-1 before m reaches (linecnt-1)*3
                ochanpos[m+1] = 0.0;                            //  In this case, stick piano RH in rightmost channel of array, then go on
                ochanpos[m+2] = 1.0;                            //  (outside loop) to add pianoLH at same position in array.
            } else {
                ochanpos[m] = (double)n/(double)(instrcnt - 1); //  e.g for 5 instr
                                                                //  0 1/4 2/4 3/4 () (Range  0 to 1)
                ochanpos[m] *= ochans-1;                        //  e.g. stereo (*1) --> 0 1/4 2/4 3/4 ()     (Range  0 to 1)
                                                                //  e.g. 4-chan (*3) --> 0 3/4 1+1/2 2+1/4 () (Range  0 to 3)
                lrpos = ochanpos[m];
                while(lrpos > 1.0)                              //  stereo 0 1/4 2/4 3/4 ()  :  4-chan 0 3/4 1+1/2 2+1/4 ()
                    lrpos -= 1.0;                               //  stereo 0 1/4 2/4 3/4 ()  :  4-chan 0 3/4   1/2   1/4 ()
                pancalc(lrpos,&leftgain,&rightgain);
                ochanpos[m]   = floor(ochanpos[m]);             //  stereo 0   0   0   0 ()  :  4-chan 0  0   1    2     ()
                ochanpos[m+1] = leftgain;
                ochanpos[m+2] = rightgain;
                if(n == ispiano) {      //  Force both piano outputs to same channel
                    n--;
                    ispiano = -1;
                }
            }
        }
        ochanpos[m]   = (double)(ochans - 1);               //  stereo(0) (0) (0) (0) 1  :  4-chan(0)(0) (1)   (2)   3
        ochanpos[m+1] = 0.0;
        ochanpos[m+2] = 1.0;                                //  all signal to rightmost speaker

    } else {

        //  if sound-surround, distribute instruments equispaced around multichannel suround-array

        for(n = 0,m = 0;m < linecnt*3; n++,m+=3) {
            ochanpos[m] = (double)n/(double)instrcnt;       //  e.g for 5 instr
                                                            //  0 1/5 2/5 3/5 4/5 (Range  0 to 1)
            ochanpos[m] *= ochans;                          //  e.g. 4-chan (*4) --> 0  4/5  1+3/5 2+2/5 3+1/5 (Range  0 to 4)
                                                            //  e.g. 8-chan (*8) --> 0 1+3/5 3+1/5 4+4/5 6+2/5 (Range  0 to 8)
            lrpos = ochanpos[m];
            while(lrpos > 1.0)                              //  4-chan 0  4/5  1+3/5 2+2/5 3+1/5  :  8-chan 0 1+3/5 3+1/5 4+4/5 6+2/5   
                lrpos -= 1.0;                               //  4-chan 0  4/5    3/5   2/5   1/5  :  8-chan 0   3/5   1/5   4/5   2/5
            pancalc(lrpos,&leftgain,&rightgain);
            ochanpos[m]   = floor(ochanpos[m]);             //  4-chan 0   0   1     2     3      :  8-chan 0  1    3     4     6
            ochanpos[m+1] = leftgain;   
            ochanpos[m+2] = rightgain;
            if(n == ispiano) {      //  Force both piano outputs to same channel
                n--;
                ispiano = -1;       //  prevent this happening again
            }
        }
        leftmostchan = ochans/2 + 1;                        //  Orient data
        for(n = 0,m = 0; n < linecnt; n++,m+=3)
            ochanpos[m] = (double)(((int)round(ochanpos[m]) + leftmostchan) % ochans);
    }

    fseek(fp,0,0);
    linecnt = 0;
    while(fgets(temp,8000,fp)!=NULL) {
        cnt = 0;
        linestor = dz->parray[dz->scoredata + linecnt];
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        read_instrument_name_from_start_of_line(&p,insnam,&insno,orchestra,(int)(linecnt+1));
        linestor[cnt] = (double)insno;
        cnt++;
        while(get_float_from_within_string(&p,&dummy)) {
            linestor[cnt] = dummy;
            cnt++;
        }
        if(!valid_cluster(insno,linestor,cnt,orchestra))
            return DATA_ERROR;
        linestor[cnt] = -1; // End of scoredataline marker
        linecnt++;
    }
    fclose(fp);
    return(FINISHED);
}

/************************************ PANCALC *******************************/

void pancalc(double position,double *leftgain,double *rightgain)
{
    int dirflag;
    double temp;
    double relpos;
    double reldist, invsquare;

    if(position < 0.0)
        dirflag = SIGNAL_TO_LEFT;       /* signal on left */
    else
        dirflag = SIGNAL_TO_RIGHT;

    if(position < 0) 
        relpos = -position;
    else 
        relpos = position;
    if(relpos <= 1.0){      /* between the speakers */
        temp = 1.0 + (relpos * relpos);
        reldist = ROOT2 / sqrt(temp);
        temp = (position + 1.0) / 2.0;
        *rightgain = temp * reldist;
        *leftgain = (1.0 - temp ) * reldist;
    } else {                /* outside the speakers */
        temp = (relpos * relpos) + 1.0;
        reldist  = sqrt(temp) / ROOT2;   /* relative distance to source */
        invsquare = 1.0 / (reldist * reldist);
        if(dirflag == SIGNAL_TO_LEFT){
            *leftgain = invsquare;
            *rightgain = 0.0;
        } else {   /* SIGNAL_TO_RIGHT */
            *rightgain = invsquare;
            *leftgain = 0;
        }
    }
}

/**************************** READ_INSTRUMENT_NAME_FROM_START_OF_LINE ****************************/

int read_instrument_name_from_start_of_line(char **p,char *insnam,int *insno,synptr *orchestra,int linecnt)
{
    int n = 0;
    char *q;
    q = *p;
    while(!isspace(*q) && *q != ENDOFSTR) {
        insnam[n++] = *q;
        q++;
    }
    if(n == 0) {
        sprintf(errstr,"No instrument name in line %d\n",linecnt);
        return DATA_ERROR;
    }
    insnam[n] = ENDOFSTR;
    while(isspace(*q))
        q++;
    if(*q == ENDOFSTR) {
        sprintf(errstr,"No further data in line %d\n",linecnt);
        return DATA_ERROR;
    }
    for(n=0;n<orchestra_size;n++) {
        if(!strcmp(insnam,orchestra[n]->name)) {
            *insno = n;
            break;
        }
    }
    if(n == orchestra_size) {
        sprintf(errstr,"Unknown instrument name (%s) in line %d\n",insnam,linecnt);
        return DATA_ERROR;
    }
    *p = q;
    return FINISHED;
}

/**************************** VALID_CLUSTER ****************************/

int valid_cluster(int insno,double *linestor,int datacnt,synptr *orchestra)
{
    int midival[64]; 
    char insname[64];
    int n = 0, chordcnt = 0, time, midi, lasttime = 0, lastmidi = 0;
    synptr instrument = orchestra[insno];
    int doublestop = instrument->doublestop;
    switch(insno) {
    case(2):    strcpy(insname,"pianoRH");  break;
    case(3):    strcpy(insname,"pianoLH");  break;
    case(5):    strcpy(insname,"violin");   break;
    case(6):    strcpy(insname,"cello");    break;
    }
    n = 1;
    while(n < datacnt) {
        time = (int)round(linestor[n++]);
        midi = (int)round(linestor[n++]);
        n += 2;
        if(n > 5) {
            if(time == lasttime) {
                if(chordcnt == 0)
                    midival[chordcnt++] = lastmidi;
                midival[chordcnt++] = midi;
            } else {
                if(!check_clustering(insno,chordcnt,doublestop,insname,lasttime,midival))
                    return DATA_ERROR;
                chordcnt = 0;
            }
        }
        lasttime = time;
        lastmidi = midi;
    }
    return FINISHED;
}

/**************************** CHECK_CLUSTERING ****************************/

#define FIFTH   7   // semitones
#define OCTAV   12  // semitones

int check_clustering(int insno,int chordcnt,int doublestop,char *insname,int time, int *midival)
{
    int maxmidi = -1000, minmidi = 1000, k, n, range, openstring, position;
    int minposition = 1000, maxposition = -1;
    char openname[4];
    if(chordcnt < 1)
        return FINISHED;
    if(chordcnt > doublestop)  {
        sprintf(errstr,"CLUSTER SIZE TOO LARGE in %s at time %d\n",insname,time);
        return DATA_ERROR;
    }
    for(k = 0;k < chordcnt;k++) {
        maxmidi = max(midival[k],maxmidi);
        minmidi = min(midival[k],minmidi);
    }
    range = maxmidi - minmidi;
    switch(insno) {
    case(PIANO_RH):     // pianoRH
    case(PIANO_LH):     // pianoLH
        if (range > OCTAV) {
            sprintf(errstr,"CLUSTER RANGE (%d) TOO LARGE (MAX %d) in %s at time %d\n",range,OCTAV,insname,time);
            return DATA_ERROR;
        }
        break;
    case(VIOLIN):       //  violin
    case(CELLO):        //  cello
        if(insno == VIOLIN)
            openstring = 55;
        else
            openstring = 36;
        n = 0;
        for(k=0;k<chordcnt;k++) {
            if(insno == 5) {
                switch(n) {
                case(0):    strcpy(openname,"G");    break;
                case(1):    strcpy(openname,"D");    break;
                case(2):    strcpy(openname,"A");    break;
                case(3):    strcpy(openname,"E");    break;
                }
            } else {
                switch(n) {
                case(0):    strcpy(openname,"C");    break;
                case(1):    strcpy(openname,"G");    break;
                case(2):    strcpy(openname,"D");    break;
                case(3):    strcpy(openname,"A");    break;
                }
            }
            if(midival[k] < openstring) {
                sprintf(errstr,"CLUSTER NOTE %d NOT ACCESSIBLE ON %s STRING OF %s",midival[k],openname,insname);
                return DATA_ERROR;
            }
            position = midival[k] - (n * FIFTH);
            minposition = min(minposition,position);
            maxposition = max(maxposition,position);
            n++;
            openstring += FIFTH;
        }
        if (maxposition - minposition > FIFTH) {
            sprintf(errstr,"CLUSTER HAS IMPOSSIBLE STRETCH FROM POSITION %d TO POSITION %d ON %s\n",minposition,maxposition,insname);
            return DATA_ERROR;
        }
    }
    return FINISHED;            
}

/***************************** MDO_LPHP_FILTER *************************************/

int mdo_lphp_filter(int sampcnt,int flt_cnt,double flt_mul,dataptr dz)
{
    int flt_den1  = dz->filterbas;
    int flt_den2  = dz->filterbas+1;
    int flt_count = dz->filterbas+2;
    int flt_s1    = dz->filterbas+3;
    int flt_s2    = dz->filterbas+4;
    int flt_e1    = dz->filterbas+5;
    int flt_e2    = dz->filterbas+6;
    int i;
    int  k;
    float *buf = dz->sampbuf[1];
    double ip, op = 0.0, b1;
    double *e1   = dz->parray[flt_e1];
    double *e2   = dz->parray[flt_e2];
    double *s1   = dz->parray[flt_s1];
    double *s2   = dz->parray[flt_s2];
    double *den1 = dz->parray[flt_den1];
    double *den2 = dz->parray[flt_den2];
    double *cn   = dz->parray[flt_count];
    for (i = 0 ; i < sampcnt; i++) {
        ip = (double) buf[i];
        for (k = 0 ; k < flt_cnt; k++) {
            b1    = flt_mul * cn[k];
            op    = (cn[k] * ip) + (den1[k] * s1[k]) + (den2[k] * s2[k]) + (b1 * e1[k]) + (cn[k] * e2[k]);
            s2[k] = s1[k];
            s1[k] = op;
            e2[k] = e1[k];
            e1[k] = ip;
        }
        if (fabs(op) > 1.0) {
            dz->iparam[FLT_OVFLW]++;
            dz->param[FLT_PRESCALE] *= .9999;
            if (op  > 0.0)
                op = 1.0;
            else 
                op = -1.0;
        }
        buf[i] = (float)op;
    }
    return FINISHED;
}

/********************************* MSETUP_LPHP_FILTER *****************************/

int msetup_lphp_filter(int *flt_cnt,double *flt_mul,double passfrq,double stopfrq,dataptr dz)
{
    int flt_den1  = dz->filterbas;
    int flt_den2  = dz->filterbas+1;
    int flt_count = dz->filterbas+2;
    int flt_s1    = dz->filterbas+3;
    int flt_s2    = dz->filterbas+4;
    int flt_e1    = dz->filterbas+5;
    int flt_e2    = dz->filterbas+6;
    int filter_order, k;
    double tc, tp, tt, pii, xx, yy, flt_gain;
    double sr = MSYNSRATE, nyquist = sr/2.0;
    double ss, aa, tppwr, x1, x2, cc;
    *flt_mul = -2.0;
    passfrq = nyquist - passfrq;
    stopfrq = nyquist - stopfrq;
    pii = 4.0 * atan(1.0);
    passfrq = pii * passfrq/sr;
    tp = tan(passfrq);
    stopfrq = pii * stopfrq/sr;
    tc = tan(stopfrq);
    tt = tc / tp ;
    tt = (tt * tt);
    flt_gain = fabs(FILTATTEN);
    flt_gain = flt_gain * log(10.0)/10.0 ;
    flt_gain = exp(flt_gain) - 1.0 ;
    xx = log(flt_gain)/log(tt) ;
    yy = floor(xx);
    if ((xx - yy) == 0.0 )
        yy = yy - 1.0 ;
    filter_order = ((int)yy) + 1;
    if (filter_order <= 1) 
        filter_order = 2;
    *flt_cnt = filter_order/2 ;
    filter_order = 2 * *flt_cnt;
    filter_order = 2 * *flt_cnt;
    if(dz->parray[flt_den1] != NULL) {
        free(dz->parray[flt_den1]);
        free(dz->parray[flt_den2]);
        free(dz->parray[flt_count]);
        free(dz->parray[flt_s1]);
        free(dz->parray[flt_e1]);
        free(dz->parray[flt_s2]);
        free(dz->parray[flt_e2]);
    }
    if((dz->parray[flt_den1] = (double *)malloc(*flt_cnt * sizeof(double)))==NULL
    || (dz->parray[flt_den2] = (double *)malloc(*flt_cnt * sizeof(double)))==NULL
    || (dz->parray[flt_count]= (double *)malloc(*flt_cnt * sizeof(double)))==NULL
    || (dz->parray[flt_s1]   = (double *)malloc(*flt_cnt * sizeof(double)))==NULL
    || (dz->parray[flt_e1]   = (double *)malloc(*flt_cnt * sizeof(double)))==NULL
    || (dz->parray[flt_s2]   = (double *)malloc(*flt_cnt * sizeof(double)))==NULL
    || (dz->parray[flt_e2]   = (double *)malloc(*flt_cnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for arrays of filter parameters.\n");
        return(MEMORY_ERROR);
    }
    ss = pii / (double)(2 * filter_order);
    for (k = 0; k < *flt_cnt; k++ ) {
        xx = (double) ((2.0 * (k+1)) - 1.0);
        aa = -sin(xx * ss);
        tppwr = pow(tp,2.0);
        cc = 1.0 - (2.0 * aa * tp) + tppwr;
        x1 = 2.0 * (tppwr - 1.0)/cc ;
        x2 = (1.0 + (2.0 * aa * tp) + tppwr)/cc ;
        dz->parray[flt_den1][k] = x1;
        dz->parray[flt_den2][k] = -x2 ;
        dz->parray[flt_count][k]   = pow(tp,2.0)/cc ;
    }
    for (k = 0; k < *flt_cnt; k++) {
        dz->parray[flt_s1][k] = 0.0;
        dz->parray[flt_s2][k] = 0.0;
        dz->parray[flt_e1][k] = 0.0;
        dz->parray[flt_e2][k] = 0.0;
    }
    return(FINISHED);
}

/**************************** INITIALISE_INSTRUMENTS ****************************/

int initialise_instruments(synptr **orchestra)
{
    int n;
    synptr instrument;
    if((*orchestra = (synptr *)malloc(MAXSCORLINE * sizeof(synptr)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing pointers to internal instrument data.\n");
        return(MEMORY_ERROR);                       //  Allocate pointers to all instrument data
    }
    for(n = 0;n < MAXSCORLINE; n++) {
        if(((*orchestra)[n] = (synptr)malloc(sizeof(struct synstrument)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for storing internal instrument data %d.\n",n+1);
            return(MEMORY_ERROR);                   //  Allocate pointers to this instrument data
        }
        instrument = (*orchestra)[n];
        switch(n) {
        case(0):                        //  trumpet     
            strcpy(instrument->name,"trumpet");
            instrument->partial_cnt = 16;       //  Must tally with no. of partials in instrument spectrum definition
            instrument->line_cnt    = 3;        //  Must tally with no. of (timed) lines in instrument spectrum definition
            instrument->env_len     = 12;       //  Must tally with no. of entries (count time and val as separate entries) in instrument envelope definition
            instrument->has_vibrato = 1;
            instrument->maxvibdepth = 0.3;
            instrument->packettype  = 0;
            instrument->squeeze     = 1.0;
            instrument->centre      = -0.8;
            instrument->rangebot    = 54;
            instrument->rangetop    = 79;
            instrument->overlap     = 0;
            instrument->doublestop  = 1;
            instrument->balance     = 2;
            instrument->spectrum = trumpet;     //  Point to relevant static array
            instrument->env      = trumpenv;    //  Point to relevant static array
            break;
        case(1):                        //  clarinet
            strcpy(instrument->name,"clarinet");
            instrument->partial_cnt = 10;
            instrument->line_cnt    = 2;
            instrument->env_len     = 12;
            instrument->has_vibrato = 0;
            instrument->maxvibdepth = 0.0;
            instrument->packettype  = 1;
            instrument->squeeze     = 1.0;
            instrument->centre      = -0.8;
            instrument->rangebot    = 50;
            instrument->rangetop    = 91;
            instrument->overlap     = 0;
            instrument->doublestop  = 1;
            instrument->balance     = 1;
            instrument->spectrum = clarinet;    //  Point to relevant static array
            instrument->env      = clarenv;     //  Point to relevant static array
            break;
        case(2):                        //  piano
            strcpy(instrument->name,"pianoRH");
            instrument->partial_cnt = 12;
            instrument->line_cnt    = 4;
            instrument->env_len     = 12;
            instrument->has_vibrato = 0;
            instrument->maxvibdepth = 0.0;
            instrument->packettype  = 0;
            instrument->squeeze     = 1.0;
            instrument->centre      = -0.8;
            instrument->rangebot    = 48;
            instrument->rangetop    = 108;
            instrument->overlap     = 1;
            instrument->doublestop  = 4;
            instrument->balance     = 2.5;
            instrument->spectrum = piano;       //  Point to relevant static array
            instrument->env      = pianoenv;    //  Point to relevant static array
            break;
        case(3):                        //  piano
            strcpy(instrument->name,"pianoLH");
            instrument->partial_cnt = 12;
            instrument->line_cnt    = 4;
            instrument->env_len     = 12;
            instrument->has_vibrato = 0;
            instrument->maxvibdepth = 0.0;
            instrument->packettype  = 0;
            instrument->squeeze     = 1.0;
            instrument->centre      = -0.8;
            instrument->rangebot    = 21;
            instrument->rangetop    = 72;
            instrument->overlap     = 1;
            instrument->doublestop  = 4;
            instrument->balance     = 2.5;
            instrument->spectrum = piano;       //  Point to relevant static array
            instrument->env      = pianoenv;    //  Point to relevant static array
            break;
        case(4):                        //  flute
            strcpy(instrument->name,"flute");
            instrument->partial_cnt = 20;
            instrument->line_cnt    = 2;
            instrument->env_len     = 12;
            instrument->has_vibrato = 1;
            instrument->maxvibdepth = 0.5;
            instrument->packettype  = 0;
            instrument->squeeze     = 1.0;
            instrument->centre      = -0.8;
            instrument->rangebot    = 60;
            instrument->rangetop    = 92;
            instrument->overlap     = 0;
            instrument->doublestop  = 1;
            instrument->balance     = 0.35;
            instrument->spectrum = flute;       //  Point to relevant static array
            instrument->env      = flutenv;     //  Point to relevant static array
            break;
        case(5):                        //  violin
            strcpy(instrument->name,"violin");
            instrument->partial_cnt = 12;
            instrument->line_cnt    = 4;
            instrument->env_len     = 12;
            instrument->has_vibrato = 1;
            instrument->maxvibdepth = 1.5;
            instrument->packettype  = 0;
            instrument->squeeze     = 1.0;
            instrument->centre      = -0.8;
            instrument->rangebot    = 55;
            instrument->rangetop    = 96;
            instrument->overlap     = 0;
            instrument->doublestop  = 4;
            instrument->balance     = 0.5;
            instrument->spectrum = violin;      //  Point to relevant static array
            instrument->env      = violenv;     //  Point to relevant static array
            break;
        case(6):                        //  cello
            strcpy(instrument->name,"cello");
            instrument->partial_cnt = 11;
            instrument->line_cnt    = 4;
            instrument->env_len     = 12;
            instrument->has_vibrato = 1;
            instrument->maxvibdepth = 1.8;
            instrument->packettype  = 1;
            instrument->squeeze     = 1.0;
            instrument->centre      = -0.8;
            instrument->rangebot    = 36;
            instrument->rangetop    = 72;
            instrument->overlap     = 0;
            instrument->doublestop  = 4;
            instrument->balance     = 0.7;
            instrument->spectrum = cello;       //  Point to relevant static array
            instrument->env      = cellenv;     //  Point to relevant static array
            break;

    //  NB: BE SURE TO INCREASE orchestra_size WHEN ADDING A NEW INSTRUMENT!!!!
        
        }
        instrument->line_entrycnt = (instrument->partial_cnt * 2) + 1;                  //  No of items in line defining spectrum at specific time
        instrument->spec_len       = instrument->line_entrycnt * instrument->line_cnt;  //  Total number of entries for spectral data
    }
    return FINISHED;
}
