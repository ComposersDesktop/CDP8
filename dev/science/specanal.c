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
#include <science.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>
#include <formants.h>
#include <speccon.h>

#define SAFETY 4

#define SA_CHANS    (0)
#define SA_WINOVLP  (1)
#define SA_FORMANT_BANDS    (2)

#define SA_PICH_LOLM        (2) /* low limit of frq search */
#define SA_PICH_HILM        (3) /* high limit of frq search */
#define SA_PICH_RNGE        (4) /* semitone range for frqs to be in tune with expected frq (semitones) */
#define SA_PICH_SRATIO      (5) /* sig-to-noise ratio: below this, window assumed to be  (pitchless) noise */
#define SA_PICH_MATCH       (6) /* how many of (MAXIMI=) 8 loudest peaks must be partials */
#define SA_PICH_VALID       (7) /* how many successive windows need to be pitched to confirm true pitch */
#define SA_PKCNT            (2) /* Number of peaks to use in filter design */
#define SA_TSTEP            (3) /* Time-step between pitch-assessments: mS */
#define SA_BOTRANGE         (4) /* Lowest acceptable pitch (MIDI) */
#define SA_TOPRANGE         (5) /* Highest acceptable pitch (MIDI) */
#define SA_GATE             (6) /* Level, relative to maxlevel, at which signal assumed to be silence */

#define SA_MIN_SMOOTH_SET   (3) /* minimum number of adjacent windows to be pitched, when smoothing */
#define PEAK_LIMIT          (.05)   
#define MAX_GLISRATE        (16.0)  /* Assumptions: pitch can't move faster than 16 octaves per sec: MAX_GLISRATE */ 
                                    /* Possible movement from window-to-window = MAX_GLISRATE * dz->frametime */
#define LAST_SMOOTH_NOT_SET (-1)
#define EIGHT_OVER_SEVEN    (1.142857143)
#define MAXIMUM_PARTIAL     (64)
#define ALMOST_TWO          (1.75)
// Pitch-absent markers
// NOT_SOUND = -2
// NOT_PITCH = -1
#define SA_TRANSIT   0.05   //  Transition time when stepping from one pitch to another     


#define SA_PVOC_TEST  (0)
#define SA_FFT        (1)
#define SA_PVOC       (2)
#define SA_LOGFFT     (3)
#define SA_CEPSTRUM   (4)
#define SA_FORMANTS   (5)
#define SA_AVERAGE    (6)
#define SA_FILTER     (7)
#define SA_TEMPERED   (8)
#define SA_HFVARY     (9)

#define SA_IGNORE_AMP (0)
#define SA_NO_HARMONICS (1)
#define SA_MIDILIST (2)
#define SA_DECHROM (3)
#define SA_DDOWNOCT  (4)
#define SA_DDOWN2OCT  (5)

#define SA_DOWNOCT  (0)
#define SA_DOWN2OCT (1)


#define CHAN_SRCHRANGE_F    (4)

#define BINCNT (128)    /* Number of semitone bins */

#define envcnt      rampbrksize
#define framecnt    ringsize
#define pitchcnt    temp_sampsize
static double quartertone;

#ifdef unix
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.2.0";

//CDP LIB REPLACEMENTS
static int check_specanal_param_validity_and_consistency(dataptr dz);
static int setup_specanal_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_specanal_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
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
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int specanal(dataptr dz);

static void hamming(float *win,int winLen,int even);
static int  pvoc_float_array(int nnn,float **ptr);
static int  sndwrite_header(int N2,int *Nchans,float *arate,float R,int D,int origsize,int *isr,int M,dataptr dz);
static int  write_samps_pvoc(float *bbuf,int samps_to_write,dataptr dz);
static int fft_(float *a, float *b, int nseg, int n, int nspn, int isn,float *at,float *ck,float *bt,float *sk,int *np);
static int fftmx(float *a,float *b,int ntot,int n,int nspan,int isn,int m,int *kt,float *at,float *ck,float *bt,float *sk,int *np,int nfac[]);
static int reals_(float *a, float *b, int n, int isn);
static int sa_initialise_specenv(dataptr dz);
static int sa_set_specenv_frqs(dataptr dz);
static int sa_setup_octaveband_steps(double **interval,dataptr dz);
static int sa_setup_low_octave_bands(int arraycnt,dataptr dz);
static int sa_extract_specenv(float *anal,dataptr dz);

static int is_a_harmonic_x(double frq1,double frq2);
static int extract_env_from_sndfile(dataptr dz);
static int get_tempered_pitch(float *averages, float *averages2, float *averages3, dataptr dz);
static int smooth_and_output_tempered_pitch(dataptr dz);
static int check_specanal_param_validity_and_consistency2(dataptr dz);
static int get_tempered_hf(float *averages, float *averages2, float *averages3, dataptr dz);
static int smooth_and_output_varying_hf(dataptr dz);
static int rerange_outofrange_pitch(int strandindex,int eventindex,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
    aplptr ap;
    int is_launched = FALSE;
    quartertone = sqrt(SEMITONE_INTERVAL);
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
        dz->maxmode = 10;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_specanal_application(dz))<0) {
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
    // open_first_infile        CDP LIB
    if((exit_status = open_first_infile(cmdline[0],dz))<0) {    
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);  
        return(FAILED);
    }
    cmdlinecnt--;
    cmdline++;
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_specanal_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_extra_infiles() : redundant
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
//  handle_special_data()       redundant
 
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency....
    if((exit_status = check_specanal_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    if((exit_status = specanal(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    switch(dz->mode) {
    case(SA_PVOC_TEST):
        dz->process = PVOC_ANAL;
        if((exit_status = complete_output(dz))<0) {
            dz->process = SPECANAL;
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        dz->process = SPECANAL;
        break;
    default:
        if((exit_status = complete_output(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    }
    exit_status = print_messages_and_close_sndfiles(FINISHED,is_launched,dz);
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

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    char *filename = (*cmdline)[0];
    if(filename[0]=='-' && filename[1]=='f') {
        dz->floatsam_output = 1;
        dz->true_outfile_stype = SAMP_FLOAT;
        filename+= 2;
    }
    if(!sloom) {
        if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
            sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
            return(DATA_ERROR);
        }
    }
    strcpy(dz->outfilename,filename);
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

/************************* SETUP_SPECANAL_APPLICATION *******************/

int setup_specanal_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if(dz->mode == SA_FORMANTS) {
        if((exit_status = set_param_data(ap,0   ,3,3,"iii"))<0)
            return(FAILED);
    } else if(dz->mode == SA_FILTER) {
        if((exit_status = set_param_data(ap,0   ,4,3,"iii0"))<0)
            return(FAILED);
    } else if(dz->mode == SA_TEMPERED) {
        if((exit_status = set_param_data(ap,0   ,4,3,"ii0d"))<0)
            return(FAILED);
    } else if(dz->mode == SA_HFVARY) {
        if((exit_status = set_param_data(ap,0   ,4,4,"iiid"))<0)
            return(FAILED);
    } else  {
        if((exit_status = set_param_data(ap,0   ,2,2,"ii"))<0)
            return(FAILED);
    }
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = SNDFILES_ONLY;
    switch(dz->mode) {
    case(SA_FFT):
    case(SA_LOGFFT):
    case(SA_CEPSTRUM):
    case(SA_FORMANTS):
        if((exit_status = set_vflgs(ap,"",0,"","n",1,0,"0"))<0)
            return(FAILED);
        dz->process_type    = TO_TEXTFILE;  
        dz->outfiletype     = TEXTFILE_OUT;
        break;
    case(SA_PVOC):
        if((exit_status = set_vflgs(ap,"",0,"","nf",2,0,"00"))<0)
            return(FAILED);
        dz->process_type    = TO_TEXTFILE;  
        dz->outfiletype     = TEXTFILE_OUT;
        break;
    case(SA_AVERAGE):
        if((exit_status = set_vflgs(ap,"",0,"","0",0,0,""))<0)
            return(FAILED);
        dz->process_type    = TO_TEXTFILE;  
        dz->outfiletype     = TEXTFILE_OUT;
        break;
    case(SA_FILTER):
        if((exit_status = set_vflgs(ap,"",0,"","ahmcdD",6,0,"000000"))<0)
            return(FAILED);
        dz->process_type    = TO_TEXTFILE;  
        dz->outfiletype     = TEXTFILE_OUT;
        break;
    case(SA_TEMPERED):
        if((exit_status = set_vflgs(ap,"btg",3,"iid","",0,0,""))<0)
            return(FAILED);
        dz->process_type    = TO_TEXTFILE;  
        dz->outfiletype     = TEXTFILE_OUT;
        break;
    case(SA_HFVARY):
        if((exit_status = set_vflgs(ap,"btg",3,"iid","d",2,0,"00"))<0)
            return(FAILED);
        dz->process_type    = TO_TEXTFILE;  
        dz->outfiletype     = TEXTFILE_OUT;
        break;
    default:
        if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
            return(FAILED);
        dz->process_type    = BIG_ANALFILE; 
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
            return(DATA_ERROR);
        } else if(infile_info->filetype != SNDFILE)  {
            sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
            return(DATA_ERROR);
        } else if(infile_info->channels != 1)  {
            sprintf(errstr,"File %s is not of correct type (must be mono)\n",cmdline[0]);
            return(DATA_ERROR);
        } else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
            sprintf(errstr,"Failed to copy file parsing information\n");
            return(PROGRAM_ERROR);
        }
        free(infile_info);
    }
    return(FINISHED);
}

/************************* SETUP_SPECANAL_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_specanal_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[SA_CHANS]    = (double)2;
    ap->hi[SA_CHANS]    = (double)PA_MAX_PVOC_CHANS;
    ap->default_val[SA_CHANS]   = PA_DEFAULT_PVOC_CHANS;
    ap->lo[SA_WINOVLP]  = (double)1;
    ap->hi[SA_WINOVLP]  = (double)4;
    ap->default_val[SA_WINOVLP] = 0.0;
    if(dz->mode == SA_FORMANTS) {
        ap->lo[SA_FORMANT_BANDS] = (double)1;
        ap->hi[SA_FORMANT_BANDS] = (double)12;
        ap->default_val[SA_FORMANT_BANDS]   = 4.0;
    } else {
        if (dz->mode == SA_FILTER || dz->mode == SA_HFVARY) {
            ap->lo[SA_PKCNT] = 1;
            ap->hi[SA_PKCNT] = 128;
            ap->default_val[SA_PKCNT]   = 6;
        } 
        if (dz->mode == SA_TEMPERED || dz->mode == SA_HFVARY) {
            ap->lo[SA_TSTEP] = 5;
            ap->hi[SA_TSTEP] = (dz->duration/2.0) * SECS_TO_MS;
            ap->default_val[SA_TSTEP]   = 100;
            ap->lo[SA_BOTRANGE] = 0;
            ap->hi[SA_BOTRANGE] = 127;
            ap->default_val[SA_BOTRANGE] = 0;
            ap->lo[SA_TOPRANGE] = 0;
            ap->hi[SA_TOPRANGE] = 127;
            ap->default_val[SA_TOPRANGE] = 127;
            ap->lo[SA_GATE] = 0;
            ap->hi[SA_GATE] = 0.5;
            ap->default_val[SA_GATE] = 0.1;
        }
    }
    dz->maxmode = 10;
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
            if((exit_status = setup_specanal_application(dz))<0)
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
    usage2("specanal");
    return(USAGE_ONLY);
}

/**************************** CHECK_SPECANAL_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_specanal_param_validity_and_consistency(dataptr dz)
{
    int exit_status;
    dz->is_mapping = 0;
    (dz->iparam[SA_WINOVLP])--;
    dz->iparam[SA_CHANS] = dz->iparam[SA_CHANS] + (dz->iparam[SA_CHANS]%4); // Force number of chans to be divisible by 4 (FFT happens twice for cepstrum)
    dz->wanted = dz->iparam[SA_CHANS] + 2;
    if(dz->mode == SA_FORMANTS) {   
        if((exit_status = sa_initialise_specenv(dz)) < 0)
            return(exit_status);
        if((exit_status = sa_set_specenv_frqs(dz)) < 0)
            return(exit_status);
    } else if (dz->mode == SA_TEMPERED || dz->mode == SA_HFVARY) {
        if(dz->iparam[SA_BOTRANGE] >= dz->iparam[SA_TOPRANGE] - 12) {
            sprintf(errstr,"INVALID PITCH RANGE: MUST BE > 1 Octave\n");
            return DATA_ERROR;
        }
        if(dz->vflag[SA_DOWNOCT] && dz->vflag[SA_DOWN2OCT]) {
            sprintf(errstr,"CANNOT TRANSPOSE DOWN BY 8va ~AND~ BY TWO 8vas\n");
            return DATA_ERROR;
        }
    } else if (dz->mode == SA_FILTER) {
        if(dz->vflag[SA_DDOWNOCT] && dz->vflag[SA_DDOWN2OCT]) {
            sprintf(errstr,"CANNOT TRANSPOSE DOWN BY 8va ~AND~ BY TWO 8vas\n");
            return DATA_ERROR;
        }
    }
    return FINISHED;
}

/**************************** CHECK_SPECANAL_PARAM_VALIDITY_AND_CONSISTENCY_2 *****************************/

int check_specanal_param_validity_and_consistency2(dataptr dz)
{
    int exit_status, n;

    dz->param[SA_TSTEP] = dz->param[SA_TSTEP] * MS_TO_SECS;
    dz->framecnt = (int)round(dz->param[SA_TSTEP]/dz->frametime);
    dz->param[SA_TSTEP] = dz->framecnt * dz->frametime;
    dz->iparam[SA_TSTEP] = (int)round(dz->param[SA_TSTEP] * dz->infile->srate);
    dz->envcnt = dz->insams[0]/dz->iparam[SA_TSTEP];
    if(dz->envcnt * dz->iparam[SA_TSTEP] < dz->insams[0])
        dz->envcnt++;
    if((dz->env = (float *)calloc(dz->envcnt + SAFETY,sizeof(float))) == 0) {
        sprintf(errstr, "Can't allocate buffer for reading envelope.\n");
        return(MEMORY_ERROR);
    }
    memset((char *)dz->env,0,(dz->envcnt + SAFETY) * sizeof(float));
    //  Buffer for reading sndfile to get envelope

    if((dz->sampbuf = (float **)malloc(sizeof(float *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers for extracting envelope.(1)\n");
        return(MEMORY_ERROR);
    }
    if((dz->sampbuf[0] = (float *)malloc((dz->iparam[SA_TSTEP] * 2) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers for extracting envelope.(2)\n");
        return(MEMORY_ERROR);
    }
    if((exit_status = extract_env_from_sndfile(dz)) < 0)
        return(exit_status);
    if((dz->iparray = (int **)malloc(8 * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing peaks data storage 0\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[0] = (int *)malloc((dz->envcnt+SAFETY) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing peaks data storage 1\n");
        return(MEMORY_ERROR);
    }
    if(dz->mode == SA_HFVARY) {
        for(n=1; n < 8;n++) {
            if((dz->iparray[n] = (int *)malloc((dz->envcnt+SAFETY) * sizeof(int)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY establishing peaks data storage %d\n",n+1);
                return(MEMORY_ERROR);
            }
        }
    }
    if((dz->parray = (double **)malloc(8 * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing pitch data storage 0\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc((dz->envcnt+SAFETY) * 2 * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing pitch data storage 1\n");
        return(MEMORY_ERROR);
    }
    if(dz->mode == SA_HFVARY) {
        for(n=1; n < dz->iparam[SA_PKCNT];n++) {
            if((dz->parray[n] = (double *)malloc((dz->envcnt+SAFETY) * 2 * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY establishing pitch data storage %d\n",n+1);
                return(MEMORY_ERROR);
            }
        }
    }
    dz->pitchcnt = 0;
    return FINISHED;
}

/******************************** DBTOLEVEL ***********************/

double dbtolevel(double val)
{
    int isneg = 0;
    if(flteq(val,0.0))
        return(1.0);
    if(val < 0.0) {
        val = -val;
        isneg = 1;
    }
    val /= 20.0;
    val = pow(10.0,val);
    if(isneg)
        val = 1.0/val;
    return(val);
}   

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"specanal"))        dz->process = SPECANAL;
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
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
    if(!strcmp(str,"specanal")) {
        fprintf(stderr,
        "Generate various types of analysis data, or filter data from sound.\n"
        "\n"
        "USAGE:\n"
        "specanal specanal 1  inf analofil chs ovlp\n"
        "specanal specanal 2,4,5 inf txtofils chs ovlp [-n]\n"
        "specanal specanal 3  inf txtofils chs ovlp [-n] [-f]\n"
        "specanal specanal 6  inf txtofils chs ovlp fbands [-n]\n"
        "specanal specanal 7  inf txtofil  chs ovlp\n"
        "specanal specanal 8  inf txtofil  chs ovlp pkcnt [-a] [-h] [-c] [-m] [-d|-D]\n"
        "specanal specanal 9  inf txtofil  chs ovlp       tstep [-bbot] [-ttop] [-ggate]\n"
        "specanal specanal 10 inf txtofil  chs ovlp pkcnt tstep [-bbot] [-ttop] [-ggate] [-d|-D]\n"
        "\n"
        "INF         A Mono soundfile.\n"
        "TXTOFIL(S)  (Generic) name for text output file(s).\n"
        "ANALOFIL    Analysis output file.\n"
        "CHS         Number of analysis points : a multiple of 4 (4 - 16380).\n"
        "OVLP        analysis window overlap (1-4), for PVOC analysis.\n"
        "FBANDS      Number of formant peaks to find.\n"
        "-n          Normalise output display.\n"
        "-f          Force frequencies into ascending order.\n"
        "\n"
        "Mode 1:  PVOC anal output as analfile.\n"
        "\n"
        "Modes 2-6 generate sets of text outfiles, 1 for each analysis window from input\n"
        "          (except first - zeroed - window).\n"
        "          These can be used to run a \"movie\" of the data in Soundloom.\n"
        "\n"
        "Mode 2:  FFT data as magnitude and channel centre frequency.\n"
        "Mode 3:  PVOC data as amplitude and frequency.\n"
        "Mode 4:  LOG of FFT magnitudes (output always noremalised).\n"
        "Mode 5:  Cepstrum (FFT of log FFT) (output always normalised).\n"
        "Mode 6:  Generate Formant Envelope.\n"
        "\n"
        "Modes 7-10 generate 1 textfile, a MIDI list, or a MIDI varibank filterdata file.\n"
        "\n"
        "Mode 7:  Signal level, at each semitone bin, averaged over entire sound.\n"
        "Mode 8:  Derived Harmonic Field (HF) of entire sound\n"
        "         converted to varibank-filter data, or a list of MIDI values.\n"
        "Mode 9:  (Changing) Pitch, in tempered scale: output as MIDI-type varibank file.\n"
        "             NB Pitch of most prominent partial, not necessarily of fundamental.\n"
        "Mode 10: (Changing) HF in tempered scale: output as MIDI-type varibank file.\n"
        "\n"
        "PKCNT   Number of peaks to use as filter frequencies.\n"
        "        (Run Mode 7 first to view the spectral envelope as a brkfile).\n"
        "TSTEP   Time step, in mS, between pitch-assessments.\n"
        "BOT     Minimum acceptable pitch (MIDI).\n"
        "TOP     Maximum acceptable pitch (MIDI).\n"
        "GATE    Level (relative to max) at which signal ignored (no pitch assigned).\n"
        "-a      Ignore relative amp of peaks (dflt, use relative amps) in filter definition.\n"
        "-h      Eliminate any peaks which are harmonics of others.\n"
        "-c      Dechromaticise: remove quieter pitch of any pair at semitone interval.\n" 
        "-m      Outputs list of MIDI values (default: MIDI-type varibank filter file).\n"
        "-d      Transpose entire filter down by an octave.\n"
        "-D      Transpose entire filter down by two octaves.\n"
        "\n");
    } else {
        fprintf(stdout,"Unknown option '%s'\n",str);
        fflush(stdout);
    }
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/****************************** GET_MODE *********************************/

int get_the_mode_from_cmdline(char *str,dataptr dz)
{
    char temp[200], *p;
    if(sscanf(str,"%s",temp)!=1) {
        sprintf(errstr,"Cannot read mode of program.\n");
        return(USAGE_ONLY);
    }
    p = temp + strlen(temp) - 1;
    while(p >= temp) {
        if(!isdigit(*p)) {
            fprintf(stderr,"Invalid mode of program entered.\n");
            return(USAGE_ONLY);
        }
        p--;
    }
    if(sscanf(str,"%d",&dz->mode)!=1) {
        fprintf(stderr,"Cannot read mode of program.\n");
        return(USAGE_ONLY);
    }
    if(dz->mode <= 0 || dz->mode > dz->maxmode) {
        fprintf(stderr,"Program mode value [%d] is out of range [1 - %d].\n",dz->mode,dz->maxmode);
        return(USAGE_ONLY);
    }
    dz->mode--;     /* CHANGE TO INTERNAL REPRESENTATION OF MODE NO */
    return(FINISHED);
}

/****************************** HAMMING ******************************/

void hamming(float *win,int winLen,int even)
{
    float Pi,ftmp;
    int i;

/***********************************************************
                    Pi = (float)((double)4.*atan((double)1.));
***********************************************************/
    Pi = (float)PI;
    ftmp = Pi/winLen;

    if (even) {
        for (i=0; i<winLen; i++)
        *(win+i) = (float)((double).54 + (double).46*cos((double)(ftmp*((float)i+.5))));
        *(win+winLen) = 0.0f;}
    else{   *(win) = 1.0f;
        for (i=1; i<=winLen; i++)
        *(win+i) =(float)((double).54 + (double).46*cos((double)(ftmp*(float)i)));
    }
    return;
}

/****************************** FLOAT_ARRAY ******************************/

int pvoc_float_array(int nnn,float **ptr)
{   /* set up a floating point array length nnn. */
    *ptr = (float *) calloc(nnn,sizeof(float));
    if(*ptr==NULL){
        sprintf(errstr,"specanal: insufficient memory\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/****************************** INTEGER_ARRAY ******************************/

int pvoc_integer_array(int nnn,int **ptr)
{   /* set up a floating point array length nnn. */
    *ptr = (int *) calloc(nnn,sizeof(int));
    if(*ptr==NULL){
        sprintf(errstr,"specanal: insufficient memory\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/****************************** SPECANAL ******************************/

int specanal(dataptr dz)
{
    int exit_status, filtercnt, bincnt, n, q, qq, harmonic_amongst_loudest_peaks, peakmidi, chromcnt, downtranspos;
    double  rratio, level, centrefrq, frq, range,
            maxsample = 0.0, minsample = 0.0, offset = 0.0, normaliser = 1.0;   /* normalisation variables */
    char    tempstr[24000], tempstr2[128];

    float   *input,         /* pointer to start of input buffer */
            *anal,          /* pointer to start of analysis buffer */
            *banal,         /* pointer to anal[1] (for FFT calls) */
            *averages=NULL, /* pointer to array storing average level of signal in semitone-bins */
            *averages2=NULL,/* pointer to array storing average level of signal in semitone-bins */
            *averages3=NULL,/* pointer to array storing average level of signal in semitone-bins */
            *nextIn,        /* pointer to next empty word in input */
            *analWindow,    /* pointer to center of analysis window */
            *i0,            /* pointer to amplitude channels */
            *i1,            /* pointer to frequency channels */
            *oi,            /* pointer to old phase channels */
            *oldInPhase,    /* pointer to start of input phase buffer */

            *sbuf = NULL,   /* input buffer array and pointer */
            *sp;

    int     M = 0,          /* length of analWindow impulse response */
            D = 0,          /* decimatin factor */
            /* RWD */
            /*F = 0,*/      /* fundamental frequency (determines dz->iparam[SA_CHANS]) */
            analWinLen,     /* half-length of analysis window */

            i,j,k,kk,jj,    /* index variables */
            Dd,             /* number of new inputs to read (Dd <= D) */
            N2,             /* dz->iparam[SA_CHANS]/2 */
            Mf = 0,         /* flag for even M */
            flag = 0,       /* end-of-input flag */

            got,            /* variables for keeping track of input */
            tocp,
            outframecnt,    /* and counting output */
            curtailed = 0,
            
            normalised = 0, /* flags to keep track of file and normalisation types */
            ordered = 0,    /* if set, listed frequencies arranged in ascending order */
            is_text_out = 0, 
            is_single_text_out = 0, /* pitch analysis procedures */
            is_anal_out = 0,
            *peaks=NULL,
            doskip,
            zeros = 0;

    int sampsize,       /* sample size for output file */
            ibuflen,        /* length of input buffer */
            nI = 0,         /* current input (analysis) sample */
            origsize = 0,   /* sample type of file analysed */
            isr,            /* sampling rate */
            Nchans,         /* no of chans */
            endsamp = PA_VERY_BIG_INT;

    float   real,           /* real part of analysis data */
            imag,           /* imaginary part of analysis data */
            mag,            /* magnitude of analysis data */
            phase,          /* phase of analysis data */
            angleDif,       /* angle difference */
            RoverTwoPi,     /* R/D divided by 2*Pi */
            sum,            /* scale factor for renormalizing windows */
            rIn,            /* decimated sampling rate */
            R,              /* input sampling rate */
            kfrq,jfrq,temp, /* for data-ordering calculations */
            peakfrq,
            arate,          /* sample rate for header on stdout if analysis output */
            F = 0.0f;       /*RWD */


    char    *p, thisext[200], thisbasnam[200], filename[200], thisnum[64];  /* handling output multiple outputfile names */

    int     orig_chans;
    int orig_srate;

    // fft ARRAYS
    
    float   *at, *ck, *bt, *sk;
    int     *np;
    int     bigenough = 1024, maxpos;
    double maxval;

    if((at = (float *) calloc(bigenough,sizeof(float))) == NULL) {
        sprintf(errstr,"Problem Allocating Array 'at' for FFT\n");
        return(MEMORY_ERROR);
    }
    if((ck = (float *) calloc(bigenough,sizeof(float))) == NULL) {
        sprintf(errstr,"Problem Allocating Array 'ck' for FFT\n");
        return(MEMORY_ERROR);
    }
    if((bt = (float *) calloc(bigenough,sizeof(float))) == NULL) {
        sprintf(errstr,"Problem Allocating Array 'bt' for FFT\n");
        return(MEMORY_ERROR);
    }
    if((sk = (float *) calloc(bigenough,sizeof(float))) == NULL) {
        sprintf(errstr,"Problem Allocating Array 'sk' for FFT\n");
        return(MEMORY_ERROR);
    }
    if((np = (int *) calloc(bigenough,sizeof(int))) == NULL) {
        sprintf(errstr,"Problem Allocating Array 'np' for FFT\n");
        return(MEMORY_ERROR);
    }

    switch(dz->mode) {
    case(SA_FFT):
    case(SA_FORMANTS):
        is_text_out = 1;
        if(dz->vflag[0])
            normalised = 1;
        break;
    case(SA_PVOC):
        is_text_out = 1;
        if(dz->vflag[0])
            normalised = 1;
        if(dz->vflag[1])
            ordered = 1;
        break;
    case(SA_LOGFFT):
    case(SA_CEPSTRUM):
        is_text_out = 1;
        normalised = 1; //  Always normalised
        break;
    case(SA_AVERAGE):
    case(SA_TEMPERED):
    case(SA_HFVARY):
    case(SA_FILTER):
        is_single_text_out = 1;
        break;
    default:
        is_anal_out = 1;
        break;
    }
    if(is_text_out || is_single_text_out) {
        p = dz->outfilename;            //  Get elements of output file names
        while(*p != ENDOFSTR) {
            if(*p == '.') {
                strcpy(thisext,p);
                *p = ENDOFSTR;
                strcpy(thisbasnam,dz->outfilename);
                break;
            }
            p++;
        }
        if(*p == ENDOFSTR) {            //  If no file extension found, force ".txt"
            strcpy(thisext,".txt");
            strcpy(thisbasnam,dz->outfilename);
        }
    }
    if(sndgetprop(dz->ifd[0],"sample type", (char *) &sampsize, sizeof(int)) < 0){
        sprintf(errstr,"specanal: failure to get sample type\n");
        return(MEMORY_ERROR);
    }

    isr      = dz->infile->srate;
    dz->nyquist = isr/2.0;
    R        = (float)isr;
    Nchans   = dz->infile->channels;
    /*RWD OCT 05 need this to preserved hires infile formats */
    origsize = dz->infile->stype;   

    if(flteq(R,0.0)) {
        sprintf(errstr,"Problem: zero sampling rate\n");
        return(DATA_ERROR);
    }
    
    sampsize = SAMP_FLOAT;
    N2 = dz->iparam[SA_CHANS] / 2;
    dz->chwidth = dz->nyquist/(double)N2;
    dz->halfchwidth = dz->chwidth/2;
    F = /*(int)*/(float)(R /(float)dz->iparam[SA_CHANS]);     /*RWD*/

    switch(dz->iparam[SA_WINOVLP]){
        case 0: M = 4*dz->iparam[SA_CHANS]; break;
        case 1: M = 2*dz->iparam[SA_CHANS]; break;
        case 2: M = dz->iparam[SA_CHANS];       break;
        case 3: M = N2;                         break;
        default:
            sprintf(errstr,"specanal: Invalid window overlap factor.\n");
            return(PROGRAM_ERROR);
    }

    Mf = 1 - M%2;

    if (M < 7) {
        fprintf(stdout,"WARNING: analWindow impulse response is too small\n");
        fflush(stdout);
    }
    ibuflen = 4 * M;

    if((D = (int)(M/PA_PVOC_CONSTANT_A)) == 0){
        fprintf(stdout,"WARNING: Decimation too low: adjusted.\n");
        fflush(stdout);
        D = 1;
    }

    arate = (float)(R/D);   /* Needed to write to outheader */
    dz->wanted = dz->iparam[SA_CHANS] + 2;  

    dz->frametime = (float)(1.0/arate);
    dz->tempsize = dz->insams[0];

    if(dz->mode == SA_TEMPERED || dz->mode == SA_HFVARY) {
        if((exit_status = check_specanal_param_validity_and_consistency2(dz))<0)
            return exit_status;
        if(dz->param[SA_TSTEP] <= dz->frametime * 2) {
            sprintf(errstr,"TIMESTEP TOO SHORT (Minimum > %lf mS)\n",(dz->frametime * 2 * SECS_TO_MS));
            return DATA_ERROR;
        }
    }
    if(is_anal_out) {       //  Only if Analfile output, write header
        if((exit_status = sndwrite_header(N2,&Nchans,&arate,R,D,origsize,&isr,M,dz))<0)
            return(exit_status);
        dz->process = PVOC_ANAL;
        orig_chans = dz->infile->channels; 
        orig_srate = dz->infile->srate; 
        dz->infile->channels = dz->outfile->channels;
        dz->infile->srate = dz->outfile->srate;
        if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
            return(exit_status);
        dz->process = SPECANAL;
        dz->infile->channels = orig_chans;
        dz->infile->srate = orig_srate; 
    }

    if((exit_status = pvoc_float_array(M+Mf,&analWindow))<0)
        return(exit_status);
    analWindow += (analWinLen = M/2);

    hamming(analWindow,analWinLen,Mf);

    for (i = 1; i <= analWinLen; i++)
        *(analWindow - i) = *(analWindow + i - Mf);
    if (M > dz->iparam[SA_CHANS]) {
        if (Mf)
            *analWindow *=(float)((double)dz->iparam[SA_CHANS]*sin((double)PI*.5/dz->iparam[SA_CHANS])/(double)(PI*.5));
        for (i = 1; i <= analWinLen; i++)
            *(analWindow + i) *=(float)((double)dz->iparam[SA_CHANS] * sin((double) (PI*(i+.5*Mf)/dz->iparam[SA_CHANS])) / (PI*(i+.5*Mf))); /* D.Timis */
        for (i = 1; i <= analWinLen; i++)
            *(analWindow - i) = *(analWindow + i - Mf);
    }

    sum = 0.0f;
    for (i = -analWinLen; i <= analWinLen; i++)
        sum += *(analWindow + i);

    sum = (float)(2.0/sum);     /*factor of 2 comes in later in trig identity*/

    for (i = -analWinLen; i <= analWinLen; i++)
        *(analWindow + i) *= sum;

    /* set up input buffer:  nextIn always points to the next empty
        word in the input buffer (i.e., the sample following
        sample number (n + analWinLen)).  If the buffer is full,
        then nextIn jumps back to the beginning, and the old
        values are written over. */

    if((exit_status = pvoc_float_array(ibuflen,&input))<0)
        return(exit_status);

    nextIn = input;

    /* set up analysis buffer for (N/2 + 1) channels: The input is real,
        so the other channels are redundant. oldInPhase is used
        in the conversion to remember the previous phase when
        calculating phase difference between successive samples. */

    dz->buflen = dz->iparam[SA_CHANS]+2;
    if((exit_status = pvoc_float_array(dz->buflen,&anal))<0)
        return(exit_status);
    banal = anal + 1;
    if((exit_status = pvoc_float_array(N2+1,&oldInPhase))<0)
        return(exit_status);


    if(dz->mode == SA_AVERAGE || dz->mode == SA_FILTER || dz->mode == SA_TEMPERED || dz->mode == SA_HFVARY) {
        if((exit_status = pvoc_float_array((BINCNT+1) * 2,&averages))<0)
            return(exit_status);
        if((exit_status = pvoc_integer_array(BINCNT,&peaks))<0)
            return(exit_status);
        for(n=0;n<BINCNT;n++)
            peaks[n] = -1;
        averages[0] = (float)(miditohz(0)/SEMITONE_INTERVAL);
        for(jj = 1, kk= 2; jj < BINCNT; jj++, kk+=2)
            averages[kk] = (float)miditohz(jj-0.5);
    }
    if(dz->mode ==  SA_TEMPERED || dz->mode == SA_HFVARY) {
        if((exit_status = pvoc_float_array((BINCNT+1) * 2,&averages2))<0)
            return(exit_status);
        for(jj = 1, kk= 2; jj < BINCNT; jj++, kk+=2)
            averages2[kk] = (float)miditohz(jj-0.5);
        if((exit_status = pvoc_float_array((BINCNT+1) * 2,&averages3))<0)
            return(exit_status);
        for(jj = 1, kk= 2; jj < BINCNT; jj++, kk+=2)
            averages3[kk] = (float)miditohz(jj-0.5);
    }

    /* initialization: input time starts negative so that the rightmost
        edge of the analysis filter just catches the first non-zero
        input samples; output time is always T times input time. */

    rIn  = (float)(R/(float)D);
    RoverTwoPi = (float)(rIn/TWOPI);
    nI = -(analWinLen / D) * D; /* input time (in samples) */
    Dd = analWinLen + nI + 1;   /* number of new inputs to read */
    flag = 1;

    /* main loop:  If endsamp is not specified it is assumed to be very large
        and then readjusted when fgetfloat detects the end of input. */

    display_virtual_time(0L,dz);    
    if((sbuf = (float *)calloc(D,sizeof(float))) == 0) {
        sprintf(errstr, "specanal: can't allocate buffer for Dd\n");
        return(MEMORY_ERROR);
    }

    outframecnt = 0;
    if(is_text_out && normalised) { // is_single_text_out uses this to count no of output windows

        // FIND NORMALISER or COUNT OUTPUT WINDOWS
        if(normalised) {
            fprintf(stdout,"Calculating normalisation factor\n");
            fflush(stdout);
        }
        while(nI < (endsamp + analWinLen)){

                /* prepare for analysis: read next Dd input values */
            memset((char *)sbuf,0,Dd*sizeof(float));
            if((got = fgetfbufEx(sbuf, Dd, dz->ifd[0],0)) < 0) {
                sfperror("specanal: read error");
                return(SYSTEM_ERROR);
            }
            if(outframecnt > 0) {
                zeros = 0;
                for(kk=0;kk < Dd;kk++) {
                    if(sbuf[kk] == 0.0)
                        zeros++;
                }
            }
            if(got < Dd)
                Dd = got;
            if(!is_single_text_out) {
                sp = sbuf;
                tocp = min(got, input+ibuflen-nextIn);
                got -= tocp;
                while(tocp-- > 0)
                    *nextIn++ = *sp++;
                if(got > 0) {
                    nextIn -= ibuflen;
                    while(got-- > 0)
                        *nextIn++ = *sp++;
                }
                if (nextIn >= (input + ibuflen))
                    nextIn -= ibuflen;
                
                if (nI > 0) {
                    for (i = Dd; i < D; i++){   /* zero fill at EOF */
                        *(nextIn++) = 0.0f;
                        if (nextIn >= (input + ibuflen))
                            nextIn -= ibuflen;
                    }
                }
                for (i = 0; i < dz->buflen; i++)
                    *(anal + i) = 0.0f; /*initialize*/

                j = (nI - analWinLen - 1 + ibuflen) % ibuflen;  /*input pntr*/

                k = nI - analWinLen - 1;            /*time shift*/
                while (k < 0)
                    k += dz->iparam[SA_CHANS];
                k = k % dz->iparam[SA_CHANS];
                for (i = -analWinLen; i <= analWinLen; i++) {       // windowed
                    if (++j >= ibuflen)
                        j -= ibuflen;
                    if (++k >= dz->iparam[SA_CHANS])
                        k -= dz->iparam[SA_CHANS];
                    *(anal + k) += *(analWindow + i) * *(input + j);
                }
                if((exit_status = fft_(anal,banal,1,N2,1,-2,at,ck,bt,sk,np))<0)
                    return(exit_status);
                if((exit_status = reals_(anal,banal,N2,-2))<0)
                    return(exit_status);

                if(outframecnt > 0) {
                    switch(dz->mode) {
                    case(SA_FFT):
                        break;
                    case(SA_LOGFFT):
                    case(SA_CEPSTRUM):
    //  Convert to amplitude-phase representation
                        for(i=0,i0=anal,i1=anal+1; i <= N2; i++,i0+=2,i1+=2){
                            real = *i0;
                            imag = *i1;
                            mag = (float) sqrt((double)(real * real + imag * imag));
                                        /* phase unwrapping */
                            if (real == 0.0) {
                                if(imag==0.0)
                                    rratio = 0.0;
                                else if(imag<0.0)
                                    rratio = -(PI/2.0);
                                else
                                    rratio = PI/2.0;
                            } else {
                                rratio = atan((double)imag/(double)real);
                                if(real<0.0) {
                                    if(imag<0.0)
                                        rratio -= PI;
                                    else
                                        rratio += PI;
                                }
                            }
                            phase = (float)rratio;
                            *i0 = mag;
                            *i1 = phase;
                        }
    //  Then take log of amplitude ONLY
                        for(i=0; i <= N2 * 2; i+=2)
                            *(anal + i) = (float)(log(fabs(*(anal + i))));
                        if(dz->mode == SA_CEPSTRUM) {
    // Do reconversion to real-imag, similar to PVOC code but without Angle-diff
                            for(i=0,i0=anal,i1=anal+1; i <= N2; i++,i0+=2,i1+=2){
                                mag   = *i0;
                                phase = *i1;
                                *i0 = (float)((double)mag * cos((double)phase));
                                *i1 = (float)((double)mag * sin((double)phase));
                            }
                            if((exit_status = reals_(anal,banal,N2,2))<0)
                                return(exit_status);
                            if((exit_status = fft_(anal,banal,1,N2,1,2,at,ck,bt,sk,np))<0)
                                return(exit_status);
                            for(i=0,i0=anal,i1=anal+1; i <= N2; i++,i0+=2,i1+=2){
                                real = *i0;
                                imag = *i1;
                                mag = (float) sqrt((double)(real * real + imag * imag));
                                            /* phase unwrapping */
                                if (real == 0.0) {
                                    if(imag==0.0)
                                        rratio = 0.0;
                                    else if(imag<0.0)
                                        rratio = -(PI/2.0);
                                    else
                                        rratio = PI/2.0;
                                } else {
                                    rratio = atan((double)imag/(double)real);
                                    if(real<0.0) {
                                        if(imag<0.0)
                                            rratio -= PI;
                                        else
                                            rratio += PI;
                                    }
                                }
                                phase = (float)rratio;
                                *i0 = mag;
                                *i1 = phase;
                            }
                        }
                        break;
                    case(SA_PVOC):
                    case(SA_AVERAGE):
                    case(SA_TEMPERED):
                    case(SA_HFVARY):
                    case(SA_FILTER):
                    case(SA_FORMANTS):
                        for(i=0,i0=anal,i1=anal+1,oi=oldInPhase; i <= N2; i++,i0+=2,i1+=2,oi++){
                            real = *i0;
                            imag = *i1;
                            *i0 =(float) sqrt((double)(real * real + imag * imag));
                                        /* phase unwrapping */
                            if (*i0 == 0.)
                                angleDif = 0.0f;

                            else {
                                rratio = atan((double)imag/(double)real);
                                if(real<0.0) {
                                    if(imag<0.0)
                                        rratio -= PI;
                                    else
                                        rratio += PI;
                                }
                                angleDif  = (phase = (float)rratio) - *oi;
                                *oi = phase;
                            }

                            if (angleDif > PI)
                                angleDif = (float)(angleDif - TWOPI);
                            if (angleDif < -PI)
                                angleDif = (float)(angleDif + TWOPI);

                                    /* add in filter center freq.*/

                            *i1 = angleDif * RoverTwoPi + ((float) i * F);
                        }
                        if(dz->mode == SA_FORMANTS) {
                            if((exit_status = sa_extract_specenv(anal,dz)) < 0)
                                return exit_status;
                        }
                        break;
                    }
                    switch(dz->mode) {
                    case(SA_FFT):   //  Output FFT data  as channel-centre frq/amp pair 
                    case(SA_PVOC):  //  Output PVOC data as frq-amp pair */
                    case(SA_LOGFFT):
                    case(SA_CEPSTRUM):
                        for(kk=0; kk < dz->buflen; kk+= 2) {
                            maxsample = max(maxsample,anal[kk]);
                            minsample = min(minsample,anal[kk]);
                        }
                        break;
                    case(SA_FORMANTS):
                        for(kk=0; kk < dz->infile->specenvcnt; kk++) {
                            maxsample = max(maxsample,dz->specenvamp[kk]);
                            minsample = min(minsample,dz->specenvamp[kk]);
                        }
                        break;
                    }
                    if(curtailed) {
                        if(outframecnt == 1) {
                            sprintf(errstr,"ERROR: Data in a channel of window at file start is zero, cannot proceed\n");
                            return(DATA_ERROR);
                        }
                        break;
                    }
                }
            }

            if(flag                             /* flag means do this operation only once */
            && (nI > 0) && (Dd < D)){           /* EOF detected */
                flag = 0;
                endsamp = nI + analWinLen - (D - Dd);
            }

            nI += D;                /* increment time */
                                    /* Dd = D except when the end of the sample stream intervenes */
            Dd = min(D, max(0, D+endsamp-nI-analWinLen));
            outframecnt++;
        }   /* End of main while loop */
        if(minsample < 0.0)
            offset = -minsample;
        if(flteq(maxsample - minsample,0.0)) {
            sprintf(errstr,"No level found in output data.\n");
            return(DATA_ERROR);
        }
        normaliser = 1.0/(maxsample - minsample);
        outframecnt = 0;
        sndseekEx(dz->ifd[0],0,0);
        nextIn = input;
        nI = -(analWinLen / D) * D;
        Dd = analWinLen + nI + 1;
        flag = 1;
        curtailed = 0;
        endsamp = PA_VERY_BIG_INT;
    }
    while(nI < (endsamp + analWinLen)){
        memset((char *)sbuf,0,Dd*sizeof(float));
        if((got = fgetfbufEx(sbuf, Dd, dz->ifd[0],0)) < 0) {
            sfperror("specanal: read error");
            return(SYSTEM_ERROR);
        }
        if(outframecnt > 0) {
            zeros = 0;
            for(kk=0;kk < Dd;kk++) {
                if(sbuf[kk] == 0.0)
                    zeros++;
            }
            if(zeros == Dd) {
                if(dz->mode != SA_AVERAGE && dz->mode != SA_FILTER && dz->mode != SA_TEMPERED && dz->mode != SA_HFVARY) {
                    fprintf(stdout,"No Signal at window %d, curtailing analysis here\n",outframecnt);
                    fflush(stdout);
                    curtailed = 1;
                    break;
                }
            }
        }
        if(is_text_out) {
            if(outframecnt > 0) {
                if(outframecnt > 1)             // Once first outputfile used
                    fclose(dz->fp);             // Close it before creating next (last file closed on exit from main)
                strcpy(filename,thisbasnam);
                sprintf(thisnum,"%d",outframecnt);
                strcat(filename,"_");           //  Create numbered filename
                strcat(filename,thisnum);
                strcat(filename,thisext);
                if((dz->fp = fopen(filename,"w")) == NULL) {
                    sprintf(errstr,"Cannot open output file %s\n", filename);
                    return(DATA_ERROR);
                }                               //  Write data to file
            }
        } else if(is_single_text_out) {
            if(outframecnt == 0) {
                strcpy(filename,thisbasnam);
                strcat(filename,thisext);
                if((dz->fp = fopen(filename,"w")) == NULL) {
                    sprintf(errstr,"Cannot open output file %s\n", filename);
                    return(DATA_ERROR);
                }                               //  Write data to file
            }
        }
        if(got < Dd)
            Dd = got;
        sp = sbuf;

        tocp = min(got, input+ibuflen-nextIn);
        got -= tocp;
        while(tocp-- > 0)
            *nextIn++ = *sp++;
        if(got > 0) {
            nextIn -= ibuflen;
            while(got-- > 0)
                *nextIn++ = *sp++;
        }
        if (nextIn >= (input + ibuflen))
            nextIn -= ibuflen;

        if (nI > 0) {
            for (i = Dd; i < D; i++){   /* zero fill at EOF */
                *(nextIn++) = 0.0f;
                if (nextIn >= (input + ibuflen))
                    nextIn -= ibuflen;
            }
        }

    /* ANALYSIS: The analysis subroutine computes the complex output at
    time n of (dz->iparam[SA_CHANS]/2 + 1) of the phase vocoder channels.  It operates
    on input samples (n - analWinLen) thru (n + analWinLen) and
    expects to find these in input[(n +- analWinLen) mod ibuflen].
    It expects analWindow to point to the center of a
    symmetric window of length (2 * analWinLen +1).  It is the
    responsibility of the main program to ensure that these values
    are correct!  The results are returned in anal as succesive
    pairs of real and imaginary values for the lowest (dz->iparam[SA_CHANS]/2 + 1)
    channels.   The subroutines fft and reals together implement
    one efficient FFT call for a real input sequence.  */

        for (i = 0; i < dz->buflen; i++) 
            *(anal + i) = 0.0f; /*initialize*/

        j = (nI - analWinLen - 1 + ibuflen) % ibuflen;  /*input pntr*/

        k = nI - analWinLen - 1;            /*time shift*/
        while (k < 0)
            k += dz->iparam[SA_CHANS];
        k = k % dz->iparam[SA_CHANS];
        for (i = -analWinLen; i <= analWinLen; i++) {
            if (++j >= ibuflen)
                j -= ibuflen;
            if (++k >= dz->iparam[SA_CHANS])
                k -= dz->iparam[SA_CHANS];
            *(anal + k) += *(analWindow + i) * *(input + j);
        }
        if((exit_status = fft_(anal,banal,1,N2,1,-2,at,ck,bt,sk,np))<0)
            return(exit_status);
        if((exit_status = reals_(anal,banal,N2,-2))<0)
            return(exit_status);
        switch(dz->mode) {
        case(SA_FFT):
            break;
        case(SA_LOGFFT):
        case(SA_CEPSTRUM):
//  Convert to amplitude-phase representation
            for(i=0,i0=anal,i1=anal+1; i <= N2; i++,i0+=2,i1+=2){
                real = *i0;
                imag = *i1;
                mag =(float) sqrt((double)(real * real + imag * imag));
                            /* phase unwrapping */
                if (real == 0.0) {
                    if(imag == 0.0)
                        rratio = 0.0;
                    else if(imag<0.0)
                        rratio = -(PI/2.0);
                    else
                        rratio = PI/2.0;
                } else {
                    rratio = atan((double)imag/(double)real);
                    if(real<0.0) {
                        if(imag<0.0)
                            rratio -= PI;
                        else
                            rratio += PI;
                    }
                }
                phase = (float)rratio;
                *i0 = mag;
                *i1 = phase;
            }
//  Then take log of amplitude ONLY
            for (i = 0; i <= N2 * 2; i+=2)
                *(anal + i) = (float)(log(fabs(*(anal + i))));
            if(dz->mode == SA_CEPSTRUM) {
// DO INVERSE conversion to real-imag
                for(i=0,i0=anal,i1=anal+1; i <= N2; i++,i0+=2,i1+=2){
                    mag   = *i0;
                    phase = *i1;
                    *i0 = (float)((double)mag * cos((double)phase));
                    *i1 = (float)((double)mag * sin((double)phase));
                }
                if((exit_status = reals_(anal,banal,N2,2))<0)
                    return(exit_status);
                if((exit_status = fft_(anal,banal,1,N2,1,2,at,ck,bt,sk,np))<0)
                    return(exit_status);
                for(i=0,i0=anal,i1=anal+1; i <= N2; i++,i0+=2,i1+=2){
                    real = *i0;
                    imag = *i1;
                    mag = (float) sqrt((double)(real * real + imag * imag));
                                /* phase unwrapping */
                    if (real == 0.0) {
                        if(imag==0.0)
                            rratio = 0.0;
                        else if(imag<0.0)
                            rratio = -(PI/2.0);
                        else
                            rratio = PI/2.0;
                    } else {
                        rratio = atan((double)imag/(double)real);
                        if(real<0.0) {
                            if(imag<0.0)
                                rratio -= PI;
                            else
                                rratio += PI;
                        }
                    }
                    phase = (float)rratio;
                    *i0 = mag;
                    *i1 = phase;
                }
            }
            break;
        case(SA_PVOC):
        case(SA_AVERAGE):
        case(SA_TEMPERED):
        case(SA_HFVARY):
        case(SA_FILTER):
        case(SA_PVOC_TEST):
        case(SA_FORMANTS):

    /* CONVERSION: The real and imaginary values in anal are converted to
    magnitude and angle-difference-per-second (assuming an 
    intermediate sampling rate of rIn) and are returned in
    anal. */

            for(i=0,i0=anal,i1=anal+1,oi=oldInPhase; i <= N2; i++,i0+=2,i1+=2,oi++){
                real = *i0;
                imag = *i1;
                *i0 =(float) sqrt((double)(real * real + imag * imag));
                            /* phase unwrapping */
                if (*i0 == 0.)
                    angleDif = 0.0f;

                else {
                    rratio = atan((double)imag/(double)real);
                    if(real<0.0) {
                        if(imag<0.0)
                            rratio -= PI;
                        else
                            rratio += PI;
                    }
                    angleDif  = (phase = (float)rratio) - *oi;
                    *oi = phase;
                }

                if (angleDif > PI)
                    angleDif = (float)(angleDif - TWOPI);
                if (angleDif < -PI)
                    angleDif = (float)(angleDif + TWOPI);

                        /* add in filter center freq.*/

                *i1 = angleDif * RoverTwoPi + ((float) i * F);
            }
            if(dz->mode == SA_FORMANTS) {
                if((exit_status = sa_extract_specenv(anal,dz)) < 0)
                    return exit_status;
            }
            break;
        }
        if(outframecnt == 0) {
            switch(dz->mode) {
            case(SA_PVOC_TEST):
                if((exit_status = write_samps_pvoc(anal, dz->buflen, dz)) < 0)
                    return(exit_status);
                break;
            }
        } else {
            switch(dz->mode) {
            case(SA_FFT):   //  Output FFT data  as channel-centre frq/amp pair 
            case(SA_LOGFFT):
            case(SA_CEPSTRUM):
                centrefrq = 0.0;
                for(kk=0; kk < dz->buflen; kk+= 2) {
                    level = anal[kk];
                    if(normalised)
                        level = (level + offset) * normaliser;
                    if(fprintf(dz->fp,"%f\t%f\n",(float)centrefrq,(float)level)<2) {
                        sprintf(errstr,"Problem writing FFT data to file %s\n",filename);
                        return(PROGRAM_ERROR);
                    }
                    centrefrq += dz->chwidth;
// HEREH: IS THIS THE CORRECT CALCULATION FOR centrefrq
                }
                break;
            case(SA_PVOC):  //  Output PVOC data as frq-amp pair */
                if(ordered) {
                    for(kk=0; kk < dz->buflen-2; kk+= 2) {          //  Force frq values into ascending order
                        kfrq = anal[kk+1];                          //  So output will display as a brkpnt
                        for(jj=kk + 2;jj < dz->buflen;jj += 2) {
                            jfrq = anal[jj+1];
                            if(jfrq < kfrq) {
                                temp = anal[kk+1];
                                anal[kk+1] = anal[jj+1];
                                anal[jj+1] = temp;
                                temp = anal[kk];
                                anal[kk] = anal[jj];
                                anal[jj] = temp;
                                kfrq = jfrq;
                            }
                        }
                    }
                }
                for(kk=0; kk < dz->buflen; kk+= 2) {
                    level = anal[kk];
                    if(normalised)
                        level = (level + offset) * normaliser;
                    if(fprintf(dz->fp,"%f\t%f\n",anal[kk+1],(float)level)<2) {
                        sprintf(errstr,"Problem writing PVOC data to file %s\n",filename);
                        return(PROGRAM_ERROR);
                    }
                }
                break;
            case(SA_AVERAGE):   //  Sum PVOC amplitude in semitonal bins */
            case(SA_TEMPERED):
            case(SA_HFVARY):
            case(SA_FILTER):
                if(dz->mode == SA_TEMPERED) {
                    if(outframecnt % dz->framecnt == 0) {
                        if((exit_status = get_tempered_pitch(averages,averages2,averages3,dz)) < 0)
                            return exit_status;
                    }
                } else if(dz->mode == SA_HFVARY) {
                    if(outframecnt % dz->framecnt == 0) {
                        if((exit_status = get_tempered_hf(averages,averages2,averages3,dz)) < 0)
                            return exit_status;
                    }
                }
                for(kk=0; kk < dz->buflen; kk+= 2) {
                    frq   = anal[kk+1];
                    level = anal[kk];
                    for(jj=0;jj < 256; jj+=2) {
                        if(frq < averages[jj]) {
                            averages[jj+1] = (float)(averages[jj+1] + level);
                            break;
                        }
                    }
                    if(jj >= 256)
                        averages[jj+1] = (float)(averages[jj+1] + level);
                }
                break;
            case(SA_FORMANTS):  //  Output FORMANTS data */
                for(kk=0; kk < dz->infile->specenvcnt; kk++) {
                    level = dz->specenvamp[kk];
                    if(normalised)
                        level = (level + offset) * normaliser;
                    if(fprintf(dz->fp,"%f\t%f\n",dz->specenvfrq[kk],(float)level)<2) {
                        sprintf(errstr,"Problem writing PVOC data to file %s\n",filename);
                        return(PROGRAM_ERROR);
                    }
                }
                break;
            case(SA_PVOC_TEST):
                if((exit_status = write_samps_pvoc(anal, dz->buflen, dz)) < 0)
                    return(exit_status);
                break;
            }
        }
        if(curtailed) {
            if(is_text_out || (is_single_text_out && (outframecnt == 1))) {
                fclose(dz->fp);
                if(remove(filename) < 0) {
                    fprintf(stdout, "ERROR: Can't delete output textfile %s\n",filename);
                    fflush(stdout);
                }
                dz->process_type = OTHER_PROCESS;   // prevents CDP trying to close non-existent file
            }
            if(outframecnt == 1) {
                fprintf(stdout,"ERROR: Data in window at file start is zero, cannot proceed\n");
                fflush(stdout);
                return(DATA_ERROR);
            }
            break;
        }
        if(flag                             /* flag means do this operation only once */
        && (nI > 0) && (Dd < D)){           /* EOF detected */
            flag = 0;
            endsamp = nI + analWinLen - (D - Dd);
        }

        nI += D;                /* increment time */
                                /* Dd = D except when the end of the sample stream intervenes */
        Dd = min(D, max(0, D+endsamp-nI-analWinLen));
        outframecnt++;
    }   /* End of main while loop */
    switch(dz->mode) {
    case(SA_AVERAGE):   //  Output average semitone-amplitude data
    case(SA_FILTER):
        maxsample = 0.0;
        minsample = 0.0;
        for(kk=0; kk < 258; kk+=2) {
            minsample = min(averages[kk+1],minsample);
            maxsample = max(averages[kk+1],maxsample);
        }
        range = maxsample - minsample;
        if(flteq(range,0.0)) {
            sprintf(errstr,"No significant signal found in input sound.\n");
            return(DATA_ERROR);
        }
        for(jj = 0, kk=0; jj < BINCNT; kk+=2, jj++) {
            if(kk == 0)
                averages[kk] = (float)(miditohz(0)/2.0);    //  i.e. anything below midi zero
            else
                averages[kk] = (float)(miditohz(jj-1));     //  i.e. anything around midi jj-1
            averages[kk+1] = (float)((averages[kk+1] - minsample)/range);
        }
        averages[256] = (float)miditohz(127);               //  i.e. anything at or above midi 127
        averages[257] = (float)((averages[257] - minsample)/range);

        switch(dz->mode) {
        case(SA_AVERAGE):
            for(jj=1,kk=2; jj < BINCNT; jj++, kk+=2) {      //  Ignore pitches below MIDI 0
                if(fprintf(dz->fp,"%d\t%f\n",(int)round(unchecked_hztomidi(averages[kk])),averages[kk+1])<1) {
                    sprintf(errstr,"Problem writing average semitone amplitude data to file %s\n",filename);
                    return(PROGRAM_ERROR);
                }
            }
            break;
        case(SA_FILTER):
            for(n = 0; n < BINCNT; n++) {               //  For every peak-numbering
                maxval = -10000;
                maxpos = 0;
                                                        //  For every semitone bin
                for(jj = 0, kk= 0; jj < BINCNT; jj++, kk+=2) {
                    doskip = 0;
                    if(n>0) {
                        for(qq=0;qq<n;qq++) {           //  If this bin is already a peak
                            if(peaks[qq] == jj) {
                                doskip = 1;
                                break;
                            }
                        }
                    }
                    if(doskip)
                        continue;
                    if(averages[kk+1] > maxval) {       //  is it the loudest non-listed peak??
                        maxval = averages[kk+1];
                        maxpos = jj;
                    }
                }
                peaks[n] = maxpos;                      //  peaks[n] takes the number of loudest peak amongst those not yet listed
                                                        //  peaks array thus becomes listing of peaks in descending loudness order
            }
            for(n = 0; n < BINCNT; n++) {               //  Move peak at MIDI zero to bottom of peaks list
                if(peaks[n] == 0) {
                    for(qq=n+1;qq<BINCNT;qq++)
                        peaks[qq-1] = peaks[qq];
                    peaks[BINCNT-1] = 0;
                    break;
                }
            }
                                                            //  If flag set, eliminate peaks a semitone away from more prominent peaks
            if(dz->vflag[SA_DECHROM]) {
                for(q = 0; q < BINCNT; q++) {               //  For ALL peaks, starting at the loudest
                    jj = peaks[q];                          
                    kk = jj*2;                              //  Find the associated frq-amp data
                    if(averages[kk] < miditohz(0))          //  (if frequency is below midi 0, keep)
                        continue;
                    peakmidi = (int)round(unchecked_hztomidi(averages[kk]));    //  Get the midipitch of this peak
                    qq = 0;
                    chromcnt = 0;
                    while(qq < BINCNT) {                    //  Compare this peak with all other peaks
                        if(qq != q) {                       
                            jj = peaks[qq];
                            kk = jj*2;                          //  If other pitch is a semitone higher or lower
                            if((averages[kk] >= 0.0) && (abs((int)round(unchecked_hztomidi(averages[kk])) - peakmidi) == 1)) {
                                averages[kk] = -1.0;            //  Flag less-prominent peak as not-wanted, by setting frq to -1
                                for(n=qq+1;n < BINCNT;n++)
                                    peaks[n-1] = peaks[n];      //  Shuffle-down overwrite peak-data (peak eliminated from list)
                                peaks[BINCNT-1] = jj;           //  Eliminated peak goes to bottom of list of peaks
                                if(++chromcnt > 1)              //  No more than 2 semitone-adjacent peaks, so if got 2, drop from inner loop
                                    break;
                                qq--;
                            }
                        }
                        qq++;
                    }
                }
            }
            filtercnt = 0;
            if(dz->vflag[SA_NO_HARMONICS]) {                    //  If flag set, eliminate harmonics of peaks, starting at most prominent peak
                do {
                    harmonic_amongst_loudest_peaks = 0;
                    for(q = 0; q < dz->iparam[SA_PKCNT]; q++) { //  For SA_PKCNT peaks, starting at the loudest
                        jj = peaks[q];                          
                        kk = jj*2;                              //  Find the associated frq-amp data
                        if(flteq(averages[kk],-1.0)) {          //  If this peak (and hence all the remaining peaks) has already been rejected,
                            filtercnt = q;                      //   reset filtercnt and quit.
                            break;
                        }
                        if(averages[kk] < miditohz(0))          //  (if frequency is below midi 0, keep)
                            continue;
                        peakfrq = averages[kk];
                        qq = 0;
                        while(qq < BINCNT) {                    //  Compare this peak with all other peaks
                            if(qq != q) {
                                jj = peaks[qq];
                                kk = jj*2;                          //  If higher frequency is a harmonic of lower
                                if((averages[kk] >= 0.0) && is_a_harmonic_x(averages[kk],peakfrq)) {
                                    averages[kk] = -1.0;            //  Flag peak as not-wanted, by setting frq to -1
                                    for(n=qq+1;n < BINCNT;n++)
                                        peaks[n-1] = peaks[n];      //  Shuffle-down overwrite peak-data (peak eliminated from list)
                                    peaks[BINCNT-1] = jj;           //  Eliminated peak goes to bottom of list of peaks
                                    if(qq < dz->iparam[SA_PKCNT])   //  If we have thus eliminated one of the currently loudest SA_PKCNT peaks
                                        harmonic_amongst_loudest_peaks = 1;
                                                                    //  Flag that a new search for harmonics is needed, with the new set of SA_PKCNT peaks
                                    qq--;
                                }
                            }
                            qq++;
                        }
                    }
                } while(harmonic_amongst_loudest_peaks);
            }
            if(filtercnt == 0) {                        //  If filtercnt has not been reset previously, set it now
                filtercnt = dz->iparam[SA_PKCNT];
                for(n=filtercnt;n < BINCNT;n++) {
                    jj = peaks[n];
                    kk = jj*2; 
                    averages[kk] = -1.0;                //  and set all unwanted peaks to -1
                }
            }
            jj = 0;
            bincnt = BINCNT;
            while(jj < bincnt-1) {                      //  Peaks set to -1 are eliminated by downward-shuffle overwrite
                kk = jj*2; 
                if(averages[kk] < 0) {
                    for(n = kk;n < (bincnt-1)*2; n++)
                        averages[n]  = averages[n+2];
                    bincnt--;
                } else  
                    jj++;
            }
            kk = jj*2; 
            if(averages[kk] < 0)
                bincnt--;           
            if(bincnt != filtercnt) {
                sprintf(errstr,"Error in bin-elimination logic. bincnt = %d filtercnt = %d\n",bincnt,filtercnt);
                return PROGRAM_ERROR;
            }

            // Convert to MIDI
            for(jj = 0,kk=0;jj < filtercnt;jj++,kk+=2)
                averages[kk] = (float)((int)round(unchecked_hztomidi(averages[kk])));

            //  Do any transposition required
            if(dz->vflag[SA_DDOWNOCT]) {    
                downtranspos = 1;
                for(jj = 0,kk=0;jj < filtercnt;jj++,kk+=2) {
                    if(averages[kk] - 12.0 < 0) {
                        downtranspos = 0;
                        break;
                    }
                }
                if(!downtranspos) {
                    fprintf(stdout,"WARNING: OCTAVE DOWN TRANSPOSITION NOT POSSIBLE.\n");
                    fflush(stdout);
                } else {
                    for(jj = 0,kk=0;jj < filtercnt;jj++,kk+=2)
                        averages[kk] -= 12.0;
                }
            } else if(dz->vflag[SA_DDOWN2OCT]) {    
                downtranspos = 2;
                for(jj = 0,kk=0;jj < filtercnt;jj++,kk+=2) {
                    if(averages[kk] - 24.0 < 0) {
                        downtranspos = 0;
                        break;
                    }
                }
                if(!downtranspos) {
                    fprintf(stdout,"WARNING: TWO OCTAVE DOWN TRANSPOSITION NOT POSSIBLE.\n");
                    fflush(stdout);
                } else {
                    for(jj = 0,kk=0;jj < filtercnt;jj++,kk+=2)
                        averages[kk] -= 24.0;
                }
            }
            if(dz->vflag[SA_IGNORE_AMP]) {          //  Ignore amplitudes (set them all to 1.0)
                for(jj=0,kk=1;jj < filtercnt;jj++,kk+=2)
                    averages[kk] = 1.0;
            }
            if(dz->vflag[SA_MIDILIST]) {
                sprintf(tempstr,"");
                for(jj = 0,kk=0;jj < filtercnt;jj++,kk+=2) {
                    if((int)round(averages[kk]) >= 0) {
                        sprintf(tempstr2,"%d\t",(int)round(averages[kk]));
                        strcat(tempstr,tempstr2);
                    }
                }
            } else {
                sprintf(tempstr,"0.0\t");
                for(jj = 0,kk=0;jj < filtercnt;jj++,kk+=2) {
                    sprintf(tempstr2,"%d\t%f\t",(int)round(averages[kk]),averages[kk+1]);
                    strcat(tempstr,tempstr2);
                }
                if(fprintf(dz->fp,"%s\n",tempstr)<1) {
                    sprintf(errstr,"Problem writing filter data to file %s\n",filename);
                    return(PROGRAM_ERROR);
                }
                sprintf(tempstr,"10000.0\t");
                for(jj = 0,kk=0;jj < filtercnt;jj++,kk+=2) {
                    sprintf(tempstr2,"%d\t%f\t",(int)round(averages[kk]),averages[kk+1]);
                    strcat(tempstr,tempstr2);
                }
            }
            if(fprintf(dz->fp,"%s\n",tempstr)<1) {
                sprintf(errstr,"Problem writing filter data to file %s\n",filename);
                return(PROGRAM_ERROR);
            }
            break;
        }
        break;
    case(SA_TEMPERED):
        if(outframecnt % dz->framecnt > 0) {        //  Extract pitch from any part-window at end
            if((exit_status = get_tempered_pitch(averages,averages2,averages3,dz)) < 0)
                return exit_status;
        }
        if((exit_status = smooth_and_output_tempered_pitch(dz))<0)
            return exit_status;
        break;
    case(SA_HFVARY):
        if(outframecnt % dz->framecnt > 0) {        //  Extract HF from any part-window at end
            if((exit_status = get_tempered_hf(averages,averages2,averages3,dz)) < 0)
                return exit_status;
        }
        if((exit_status = smooth_and_output_varying_hf(dz))<0)
            return exit_status;
        break;
    }
    return(FINISHED);
}

/************************************ SNDWRITE_HEADER ************************************/

int sndwrite_header(int N2,int *Nchans,float *arate,float R,int D,int origsize,int *isr,int M,dataptr dz)
{
    *Nchans = dz->iparam[SA_CHANS] + 2;
    dz->outfile->channels = *Nchans;
    *arate = (float)(R/(float)D);
    dz->outfile->srate = (int)(*arate);
    dz->outfile->arate = *arate;
    dz->outfile->origstype = origsize;
    dz->outfile->origrate = *isr;
    dz->outfile->Mlen = M;
    dz->outfile->Dfac = D;
    fflush(stdout);
    return(FINISHED);
}

/******************************* WRITE_SAMPS_PVOC ********************************/

int write_samps_pvoc(float *bbuf,int samps_to_write,dataptr dz)
{
    
    int samps_written;
    int i,j;
    int granularity = 22100;
    int this_granularity = 0;
    float val;

    if(dz->needpeaks){
        for(i=0;i < samps_to_write; i += dz->outchans){
            for(j = 0;j < dz->outchans;j++){
                val = (float)fabs(bbuf[i+j]);
                /* this way, posiiton of first peak value is stored */
                if(val > dz->outpeaks[j].value){
                    dz->outpeaks[j].value = val;
                    dz->outpeaks[j].position = dz->outpeakpos[j];
                }
            }
            /* count framepos */
            for(j=0;j < dz->outchans;j++)
                dz->outpeakpos[j]++;
        }
    }
    if((samps_written = fputfbufEx(bbuf,samps_to_write,dz->ofd))<=0) {
        sprintf(errstr,"Can't write to output soundfile: %s\n",sferrstr());
        return(SYSTEM_ERROR);
    }
    dz->total_samps_written += samps_written;   
    this_granularity += samps_to_write;
    if(this_granularity > granularity){
        display_virtual_time(dz->total_samps_written,dz);
        this_granularity -= granularity;
    }
    return(FINISHED);
}

///////////// MXFFT

/*
 *-----------------------------------------------------------------------
 * subroutine:  fft
 * multivariate complex fourier transform, computed in place
 * using mixed-radix fast fourier transform algorithm.
 *-----------------------------------------------------------------------
 *
 *  this is the call from C:
 *      fft_(anal,banal,&one,&N2,&one,&mtwo);
 *  CHANGED TO:-
 *      fft_(anal,banal,one,N2,one,mtwo);
 */

int fft_(float *a, float *b, int nseg, int n, int nspn, int isn,float *at,float *ck,float *bt,float *sk,int *np)
        /*  a: pointer to array 'anal'  */
        /*  b: pointer to array 'banal' */
{
    int exit_status;
    int nfac[16];       /*  These are one bigger than needed   */
                        /*  because wish to use Fortran array  */
                        /* index which runs 1 to n, not 0 to n */

    int     m = 0, nf, k, kt, ntot, j, jj, maxf, maxp=0;

/* work space pointers */
//  float   *at, *ck, *bt, *sk;
//  int     *np;


/* reduce the pointers to input arrays - by doing this, FFT uses FORTRAN
   indexing but retains compatibility with C arrays */
    a--;    b--;

/*  
 * determine the factors of n
 */
    k=nf=abs(n);
    if(nf==1) 
        return(FINISHED);

    nspn=abs(nf*nspn);
    ntot=abs(nspn*nseg);

    if(isn*ntot == 0){
        sprintf(errstr,"zero in FFT parameters %d %d %d %d",nseg, n, nspn, isn);
        return(DATA_ERROR);
    }
    for (m=0; !(k%16); nfac[++m]=4,k/=16);
    for (j=3,jj=9; jj<=k; j+=2,jj=j*j)
        for (; !(k%jj); nfac[++m]=j,k/=jj);

    if (k<=4) {
        kt = m;
        nfac[m+1] = k;
        if(k != 1) 
        m++;
    } else {
        if(k%4==0){
            nfac[++m]=2;
            k/=4;
        }

        kt = m;
        maxp = max((kt+kt+2),(k-1));
        for(j=2; j<=k; j=1+((j+1)/2)*2) {
            if(k%j==0){
                nfac[++m]=j;
                k/=j;
            }
        }
    }
    if(m <= kt+1) 
        maxp = m + kt + 1;
    if(m+kt > 15) {
        sprintf(errstr,"FFT parameter n has more than 15 factors : %d", n);
        return(DATA_ERROR);
    }
    if(kt!=0) {
        j = kt;
        while(j)
            nfac[++m]=nfac[j--];
    }
    maxf = nfac[m-kt];
    if(kt > 0) 
        maxf = max(nfac[kt],maxf);

/*  allocate workspace - assume no errors! */
//  at = (float *) calloc(maxf,sizeof(float));
//  ck = (float *) calloc(maxf,sizeof(float));
//  bt = (float *) calloc(maxf,sizeof(float));
//  sk = (float *) calloc(maxf,sizeof(float));
//  np = (int *) calloc(maxp,sizeof(int));

/* decrement pointers to allow FORTRAN type usage in fftmx */
    at--;   bt--;   ck--;   sk--;   np--;

/* call fft driver */

    if((exit_status = fftmx(a,b,ntot,nf,nspn,isn,m,&kt,at,ck,bt,sk,np,nfac))<0)
        return(exit_status);

/* restore pointers before releasing */
    at++;   bt++;   ck++;   sk++;   np++;

/* release working storage before returning - assume no problems */
//  (void) free(at);
//  (void) free(sk);
//  (void) free(bt);
//  (void) free(ck);
//  (void) free(np);
    return(FINISHED);
}

/*
 *-----------------------------------------------------------------------
 * subroutine:  fftmx
 * called by subroutine 'fft' to compute mixed-radix fourier transform
 *-----------------------------------------------------------------------
 */
int fftmx(float *a,float *b,int ntot,int n,int nspan,int isn,int m,int *kt,
            float *at,float *ck,float *bt,float *sk,int *np,int nfac[])
{   
    int i,inc,
        j,jc,jf, jj,
        k, k1, k2, k3=0, k4,
        kk,klim,ks,kspan, kspnn,
        lim,
        maxf,mm,
        nn,nt;
    double  aa, aj, ajm, ajp, ak, akm, akp,
        bb, bj, bjm, bjp, bk, bkm, bkp,
        c1, c2=0.0, c3=0.0, c72, cd,
        dr,
        rad, 
        sd, s1, s2=0.0, s3=0.0, s72, s120;

    double  xx; /****** ADDED APRIL 1991 *********/
    inc=abs(isn);
    nt = inc*ntot;
        ks = inc*nspan;
/******************* REPLACED MARCH 29: ***********************
                    rad = atan((double)1.0);
**************************************************************/
    rad = 0.785398163397448278900;
/******************* REPLACED MARCH 29: ***********************
                        s72 = rad/0.625;
                        c72 = cos(s72);
                        s72 = sin(s72);
**************************************************************/
    c72 = 0.309016994374947451270;
    s72 = 0.951056516295153531190;
/******************* REPLACED MARCH 29: ***********************
                        s120 = sqrt((double)0.75);
**************************************************************/
        s120 = 0.866025403784438707600;

/* scale by 1/n for isn > 0 ( reverse transform ) */

        if (isn < 0){
            s72 = -s72;
            s120 = -s120;
            rad = -rad;}
    else{   ak = 1.0/(double)n;
        for(j=1; j<=nt;j += inc){
                a[j] = (float)(a[j] * ak);
                b[j] = (float)(b[j] * ak);
        }
    }
    kspan = ks;
        nn = nt - inc;
        jc = ks/n;

/* sin, cos values are re-initialized each lim steps  */

        lim = 32;
        klim = lim * jc;
        i = 0;
        jf = 0;
        maxf = m - (*kt);
        maxf = nfac[maxf];
        if((*kt) > 0) 
        maxf = max(nfac[*kt],maxf);

/*
 * compute fourier transform
 */

lbl40:
    dr = (8.0 * (double)jc)/((double)kspan);
/*************************** APRIL 1991 POW & POW2 not WORKING.. REPLACE *******
            cd = 2.0 * (pow2 ( sin((double)0.5 * dr * rad)) );
*******************************************************************************/
    xx =  sin((double)0.5 * dr * rad);
        cd = 2.0 * xx * xx;
        sd = sin(dr * rad);
        kk = 1;
        if(nfac[++i]!=2) goto lbl110;
/*
 * transform for factor of 2 (including rotation factor)
 */
        kspan /= 2;
        k1 = kspan + 2;
        do{ do{ k2 = kk + kspan;
                ak = a[k2];
                bk = b[k2];
                a[k2] = (float)((a[kk]) - ak);
                b[k2] = (float)((b[kk]) - bk);
                a[kk] = (float)((a[kk]) + ak);
                b[kk] = (float)((b[kk]) + bk);
                kk = k2 + kspan;
        } while(kk <= nn);
            kk -= nn;
    }while(kk <= jc);
        if(kk > kspan) goto lbl350;
lbl60:  c1 = 1.0 - cd;
        s1 = sd;
        mm = min((k1/2),klim);
        goto lbl80;
lbl70:  ak = c1 - ((cd*c1)+(sd*s1));
        s1 = ((sd*c1)-(cd*s1)) + s1;
        c1 = ak;
lbl80:  do{ do{ k2 = kk + kspan;
                ak = a[kk] - a[k2];
                bk = b[kk] - b[k2];
                a[kk] = a[kk] + a[k2];
                b[kk] = b[kk] + b[k2];
                a[k2] = (float)((c1 * ak) - (s1 * bk));
                b[k2] = (float)((s1 * ak) + (c1 * bk));
                kk = k2 + kspan;
        }while(kk < nt);
            k2 = kk - nt;
            c1 = -c1;
            kk = k1 - k2;
    }while(kk > k2);
        kk += jc;
        if(kk <= mm) goto lbl70;
        if(kk < k2)  goto lbl90;
        k1 += (inc + inc);
        kk = ((k1-kspan)/2) + jc;
        if(kk <= (jc+jc)) goto lbl60;
        goto lbl40;
lbl90:  s1 = ((double)((kk-1)/jc)) * dr * rad;
        c1 = cos(s1);
        s1 = sin(s1);
        mm = min( k1/2, mm+klim);
        goto lbl80;
/*
 * transform for factor of 3 (optional code)
 */


lbl100: k1 = kk + kspan;
    k2 = k1 + kspan;
    ak = a[kk];
    bk = b[kk];
        aj = a[k1] + a[k2];
        bj = b[k1] + b[k2];
        a[kk] = (float)(ak + aj);
        b[kk] = (float)(bk + bj);
        ak += (-0.5 * aj);
        bk += (-0.5 * bj);
        aj = (a[k1] - a[k2]) * s120;
        bj = (b[k1] - b[k2]) * s120;
        a[k1] = (float)(ak - bj);
        b[k1] = (float)(bk + aj);
        a[k2] = (float)(ak + bj);
        b[k2] = (float)(bk - aj);
        kk = k2 + kspan;
        if(kk < nn)     goto lbl100;
        kk -= nn;
        if(kk <= kspan) goto lbl100;
        goto lbl290;

/*
 * transform for factor of 4
 */

lbl110: if(nfac[i] != 4) goto lbl230;
        kspnn = kspan;
        kspan = kspan/4;
lbl120: c1 = 1.0;
        s1 = 0;
        mm = min( kspan, klim);
        goto lbl150;
lbl130: c2 = c1 - ((cd*c1)+(sd*s1));
        s1 = ((sd*c1)-(cd*s1)) + s1;
/*
 * the following three statements compensate for truncation
 * error.  if rounded arithmetic is used, substitute
 * c1=c2
 *
 * c1 = (0.5/(pow2(c2)+pow2(s1))) + 0.5;
 * s1 = c1*s1;
 * c1 = c1*c2;
 */
        c1 = c2;
lbl140: c2 = (c1 * c1) - (s1 * s1);
        s2 = c1 * s1 * 2.0;
        c3 = (c2 * c1) - (s2 * s1);
        s3 = (c2 * s1) + (s2 * c1);
lbl150: k1 = kk + kspan;
        k2 = k1 + kspan;
        k3 = k2 + kspan;
        akp = a[kk] + a[k2];
        akm = a[kk] - a[k2];
        ajp = a[k1] + a[k3];
        ajm = a[k1] - a[k3];
        a[kk] = (float)(akp + ajp);
        ajp = akp - ajp;
        bkp = b[kk] + b[k2];
        bkm = b[kk] - b[k2];
        bjp = b[k1] + b[k3];
        bjm = b[k1] - b[k3];
        b[kk] = (float)(bkp + bjp);
        bjp = (float)(bkp - bjp);
        if(isn < 0) goto lbl180;
        akp = akm - bjm;
        akm = akm + bjm;
        bkp = bkm + ajm;
        bkm = bkm - ajm;
        if(s1 == 0.0) goto lbl190;
lbl160: a[k1] = (float)((akp*c1) - (bkp*s1));
        b[k1] = (float)((akp*s1) + (bkp*c1));
        a[k2] = (float)((ajp*c2) - (bjp*s2));
        b[k2] = (float)((ajp*s2) + (bjp*c2));
        a[k3] = (float)((akm*c3) - (bkm*s3));
        b[k3] = (float)((akm*s3) + (bkm*c3));
        kk = k3 + kspan;
        if(kk <= nt)   goto lbl150;
lbl170: kk -= (nt - jc);
        if(kk <= mm)   goto lbl130;
        if(kk < kspan) goto lbl200;
        kk -= (kspan - inc);
        if(kk <= jc)   goto lbl120;
        if(kspan==jc)  goto lbl350;
        goto lbl40;
lbl180: akp = akm + bjm;
        akm = akm - bjm;
        bkp = bkm - ajm;
        bkm = bkm + ajm;
        if(s1 != 0.0)  goto lbl160;
lbl190: a[k1] = (float)akp;
        b[k1] = (float)bkp;
        a[k2] = (float)ajp;
        b[k2] = (float)bjp;
        a[k3] = (float)akm;
        b[k3] = (float)bkm;
        kk = k3 + kspan;
        if(kk <= nt) goto lbl150;
        goto lbl170;
lbl200: s1 = ((double)((kk-1)/jc)) * dr * rad;
        c1 = cos(s1);
        s1 = sin(s1);
        mm = min( kspan, mm+klim);
        goto lbl140;

/*
 * transform for factor of 5 (optional code)
 */

lbl210: c2 = (c72*c72) - (s72*s72);
        s2 = 2.0 * c72 * s72;
lbl220: k1 = kk + kspan;
        k2 = k1 + kspan;
        k3 = k2 + kspan;
        k4 = k3 + kspan;
        akp = a[k1] + a[k4];
        akm = a[k1] - a[k4];
        bkp = b[k1] + b[k4];
        bkm = b[k1] - b[k4];
        ajp = a[k2] + a[k3];
        ajm = a[k2] - a[k3];
        bjp = b[k2] + b[k3];
        bjm = b[k2] - b[k3];
        aa = a[kk];
        bb = b[kk];
        a[kk] = (float)(aa + akp + ajp);
        b[kk] = (float)(bb + bkp + bjp);
        ak = (akp*c72) + (ajp*c2) + aa;
        bk = (bkp*c72) + (bjp*c2) + bb;
        aj = (akm*s72) + (ajm*s2);
        bj = (bkm*s72) + (bjm*s2);
        a[k1] = (float)(ak - bj);
        a[k4] = (float)(ak + bj);
        b[k1] = (float)(bk + aj);
        b[k4] = (float)(bk - aj);
        ak = (akp*c2) + (ajp*c72) + aa;
        bk = (bkp*c2) + (bjp*c72) + bb;
        aj = (akm*s2) - (ajm*s72);
    bj = (bkm*s2) - (bjm*s72);
    a[k2] = (float)(ak - bj);
        a[k3] = (float)(ak + bj);
        b[k2] = (float)(bk + aj);
        b[k3] = (float)(bk - aj);
        kk = k4 + kspan;
        if(kk < nn)     goto lbl220;
        kk -= nn;
        if(kk <= kspan) goto lbl220;
        goto lbl290;

/*
 * transform for odd factors
 */

lbl230: k = nfac[i];
    kspnn = kspan;
    kspan /= k;
    if(k==3)   goto lbl100;
    if(k==5)   goto lbl210;
    if(k==jf)  goto lbl250;
        jf = k;
        s1 = rad/(((double)(k))/8.0);
        c1 = cos(s1);
        s1 = sin(s1);
        ck[jf] = 1.0f;
    sk[jf] = 0.0f;
    for(j=1; j<k ; j++){
        ck[j] = (float)((ck[k])*c1 + (sk[k])*s1);
            sk[j] = (float)((ck[k])*s1 - (sk[k])*c1);
            k--;
            ck[k] = ck[j];
            sk[k] = -(sk[j]);
    }
lbl250: k1 = kk;
        k2 = kk + kspnn;
    aa = a[kk];
    bb = b[kk];
        ak = aa;
        bk = bb;
        j = 1;
        k1 += kspan;
        do{ k2 -= kspan;
            j++;
            at[j] = a[k1] + a[k2];
            ak = at[j] + ak;    
            bt[j] = b[k1] + b[k2];
            bk = bt[j] + bk;    
            j++;
            at[j] = a[k1] - a[k2];
            bt[j] = b[k1] - b[k2];
            k1 += kspan;
    }while(k1 < k2);
        a[kk] = (float)ak;
        b[kk] = (float)bk;
        k1 = kk;
        k2 = kk + kspnn;
        j = 1;
lbl270: k1 += kspan;
        k2 -= kspan;
        jj = j;
        ak = aa;
        bk = bb;
        aj = 0.0;
        bj = 0.0;
        k = 1;
        do{ k++;
            ak = (at[k] * ck[jj]) + ak;
            bk = (bt[k] * ck[jj]) + bk; 
            k++;
            aj = (at[k] * sk[jj]) + aj;
            bj = (bt[k] * sk[jj]) + bj;
            jj += j;
            if (jj > jf) 
            jj -= jf;
    }while(k < jf);
        k = jf - j;
        a[k1] = (float)(ak - bj);
        b[k1] = (float)(bk + aj);
        a[k2] = (float)(ak + bj);
        b[k2] = (float)(bk - aj);
        j++;
        if(j < k)     goto lbl270;
        kk += kspnn;
        if(kk <= nn)  goto lbl250;
        kk -= nn;
        if(kk<=kspan) goto lbl250;

/*
 * multiply by rotation factor (except for factors of 2 and 4)
 */

lbl290: if(i==m) goto lbl350;
        kk = jc + 1;
lbl300: c2 = 1.0 - cd;
        s1 = sd;
        mm = min( kspan, klim);
        goto lbl320;
lbl310: c2 = c1 - ((cd*c1) + (sd*s1));
        s1 = s1 + ((sd*c1) - (cd*s1));
lbl320: c1 = c2;
        s2 = s1;
        kk += kspan;
lbl330: ak = a[kk];
        a[kk] = (float)((c2*ak) - (s2 * b[kk]));
        b[kk] = (float)((s2*ak) + (c2 * b[kk]));
        kk += kspnn;
        if(kk <= nt) goto lbl330;
        ak = s1*s2;
        s2 = (s1*c2) + (c1*s2);
        c2 = (c1*c2) - ak;
        kk -= (nt - kspan);
        if(kk <= kspnn) goto lbl330;
        kk -= (kspnn - jc);
        if(kk <= mm)   goto lbl310;
        if(kk < kspan) goto lbl340;
        kk -= (kspan - jc - inc);
        if(kk <= (jc+jc)) goto lbl300;
        goto lbl40;
lbl340: s1 = ((double)((kk-1)/jc)) * dr * rad;
        c2 = cos(s1);
        s1 = sin(s1);
        mm = min( kspan, mm+klim);
        goto lbl320;

/*
 * permute the results to normal order---done in two stages
 * permutation for square factors of n
 */

lbl350: np[1] = ks;
        if (!(*kt)) goto lbl440;
        k = *kt + *kt + 1;
        if(m < k) 
        k--;
    np[k+1] = jc;
        for(j=1; j < k; j++,k--){
        np[j+1] = np[j] / nfac[j];
            np[k] = np[k+1] * nfac[j];
    }
        k3 = np[k+1];
        kspan = np[2];
        kk = jc + 1;
        k2 = kspan + 1;
        j = 1;
        if(n != ntot) goto lbl400;
/*
 * permutation for single-variate transform (optional code)
 */
lbl370: do{ ak = a[kk];
            a[kk] = a[k2];
            a[k2] = (float)ak;
            bk = b[kk];
            b[kk] = b[k2];
            b[k2] = (float)bk;
            kk += inc;
            k2 += kspan;
    }while(k2 < ks);
lbl380: do{ k2 -= np[j++];
            k2 += np[j+1];
    }while(k2 > np[j]);
        j = 1;
lbl390: if(kk < k2){
        goto lbl370;
    }
        kk += inc;
        k2 += kspan;
        if(k2 < ks) goto lbl390;
        if(kk < ks) goto lbl380;
        jc = k3;
        goto lbl440;
/*
 * permutation for multivariate transform
 */
lbl400: do{ do{ k = kk + jc;
                do{ ak = a[kk];
                    a[kk] = a[k2];
                    a[k2] = (float)ak;
                    bk = b[kk];
                    b[kk] = b[k2];
                    b[k2] = (float)bk;
                    kk += inc;
                    k2 += inc;
            }while(kk < k);
                kk += (ks - jc);
                k2 += (ks - jc);
        }while(kk < nt);
            k2 -= (nt - kspan);
            kk -= (nt - jc);
    }while(k2 < ks);
lbl420: do{ k2 -= np[j++];
            k2 += np[j+1];
    }while(k2 > np[j]);
        j = 1;
lbl430: if(kk < k2)      goto lbl400;
        kk += jc;
        k2 += kspan;
        if(k2 < ks)      goto lbl430;
        if(kk < ks)      goto lbl420;
        jc = k3;
lbl440: if((2*(*kt))+1 >= m)
        return(FINISHED);

        kspnn = *(np + *(kt) + 1);
        j = m - *kt;        
        nfac[j+1] = 1;
lbl450: nfac[j] = nfac[j] * nfac[j+1];
        j--;
        if(j != *kt) goto lbl450;
        *kt = *(kt) + 1;
        nn = nfac[*kt] - 1;
        jj = 0;
        j = 0;
        goto lbl480;
lbl460: jj -= k2;
        k2 = kk;
        kk = nfac[++k];
lbl470: jj += kk;
        if(jj >= k2) goto lbl460;
        np[j] = jj;
lbl480: k2 = nfac[*kt];
        k = *kt + 1;    
        kk = nfac[k];
        j++;
        if(j <= nn) goto lbl470;
/* Determine permutation cycles of length greater than 1 */
        j = 0;
        goto lbl500;
lbl490: k = kk;
        kk = np[k]; 
        np[k] = -kk;    
        if(kk != j) goto lbl490;
        k3 = kk;
lbl500: kk = np[++j];   
        if(kk < 0)  goto lbl500;
        if(kk != j) goto lbl490;
        np[j] = -j;
        if(j != nn) goto lbl500;
        maxf *= inc;
/* Perform reordering following permutation cycles */
        goto lbl570;
lbl510: j--;
        if (np[j] < 0) goto lbl510;
        jj = jc;
lbl520: kspan = jj;
        if(jj > maxf) 
        kspan = maxf;
        jj -= kspan;
        k = np[j];  
        kk = (jc*k) + i + jj;
        k1 = kk + kspan;
        k2 = 0;
lbl530: k2++;
        at[k2] = a[k1];
        bt[k2] = b[k1];
        k1 -= inc;
        if(k1 != kk) goto lbl530;
lbl540: k1 = kk + kspan;
        k2 = k1 - (jc * (k + np[k]));
        k = -(np[k]);
lbl550: a[k1] = a[k2];
        b[k1] = b[k2];
        k1 -= inc;
        k2 -= inc;
        if(k1 != kk) goto lbl550;
        kk = k2;
        if(k != j)   goto lbl540;
        k1 = kk + kspan;
        k2 = 0;
lbl560: k2++;
        a[k1] = at[k2];
        b[k1] = bt[k2];
        k1 -= inc;
        if(k1 != kk) goto lbl560;
        if(jj)       goto lbl520;
        if(j  != 1)  goto lbl510;
lbl570: j = k3 + 1;
        nt -= kspnn;
        i = nt - inc + 1;
        if(nt >= 0)  goto lbl510;
        return(FINISHED);; 
}


/*
 *-----------------------------------------------------------------------
 * subroutine:  reals
 * used with 'fft' to compute fourier transform or inverse for real data
 *-----------------------------------------------------------------------
 *  this is the call from C:
 *      reals_(anal,banal,N2,mtwo);
 *  which has been changed from CARL call
 *      reals_(anal,banal,&N2,&mtwo);
 */

int reals_(float *a, float *b, int n, int isn)

            /* a refers to an array of floats 'anal'   */
            /* b refers to an array of floats 'banal'  */
/* See IEEE book for a long comment here on usage */

{   int inc,
        j,
        k,
        lim,
        mm,ml,
        nf,nk,nh;
 
    double  aa,ab,
        ba,bb,
        cd,cn,
        dr,
        em,
        rad,re,
        sd,sn;
    double  xx; /******* ADDED APRIL 1991 ******/
/* adjust  input array pointers (called from C) */
    a--;    b--;
    inc=abs(isn);
    nf=abs(n);
        if(nf*isn==0){
            sprintf(errstr,"zero in reals parameters in FFT : %d : %d ",n,isn);
            return(DATA_ERROR);;
        }
        nk = (nf*inc) + 2;
        nh = nk/2;
/*****************************
        rad  = atan((double)1.0);
******************************/
    rad = 0.785398163397448278900;
        dr = -4.0/(double)(nf);
/********************************** POW2 REMOVED APRIL 1991 *****************
                    cd = 2.0 * (pow2(sin((double)0.5 * dr * rad)));
*****************************************************************************/
    xx = sin((double)0.5 * dr * rad);
        cd = 2.0 * xx * xx;
        sd = sin(dr * rad);
/*
 * sin,cos values are re-initialized each lim steps
 */
        lim = 32;
        mm = lim;
        ml = 0;
        sn = 0.0;
    if(isn<0){
        cn = 1.0;
        a[nk-1] = a[1];
        b[nk-1] = b[1]; }
    else {
        cn = -1.0;
        sd = -sd;
    }
        for(j=1;j<=nh;j+=inc)   {
            k = nk - j;
            aa = a[j] + a[k];
            ab = a[j] - a[k];
            ba = b[j] + b[k];
            bb = b[j] - b[k];
            re = (cn*ba) + (sn*ab);
            em = (sn*ba) - (cn*ab);
            b[k] = (float)((em-bb)*0.5);
            b[j] = (float)((em+bb)*0.5);
            a[k] = (float)((aa-re)*0.5);
        a[j] = (float)((aa+re)*0.5);
            ml++;
        if(ml!=mm){
            aa = cn - ((cd*cn)+(sd*sn));
            sn = ((sd*cn) - (cd*sn)) + sn;
            cn = aa;}
        else {
            mm +=lim;
            sn = ((float)ml) * dr * rad;
            cn = cos(sn);
            if(isn>0)
                cn = -cn;
            sn = sin(sn);
        }
    }
    return(FINISHED);
}

/************************ SA_INITIALISE_SPECENV ************************/

int sa_initialise_specenv(dataptr dz)
{
    int arraycnt;
    dz->clength = dz->wanted/2;
    dz->nyquist = dz->infile->srate/2.0;
    dz->chwidth = dz->nyquist/(double)(dz->iparam[SA_CHANS] / 2);
    arraycnt   = dz->clength + 1;
    if((dz->specenvfrq = (float *)malloc(arraycnt * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant frq array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->specenvpch = (float *)malloc(arraycnt * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant pitch array.\n");
        return(MEMORY_ERROR);
    }
    /*RWD  zero the data */
    memset(dz->specenvpch,0,arraycnt * sizeof(float));

    if((dz->specenvamp = (float *)malloc(arraycnt * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant aplitude array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->specenvtop = (float *)malloc(arraycnt * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant frq limit array.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/************************ SA_SET_SPECENV_FRQS ************************
 *
 * FREQWISE BANDS = number of channels for each specenv point
 * PICHWISE BANDS  = number of points per octave
 */

int sa_set_specenv_frqs(dataptr dz)
{
    int exit_status;
    int arraycnt;
    double bandbot;
    double *interval;
    int m, n, k = 0, cc;
    arraycnt   = dz->clength + 1;
    if((exit_status = sa_setup_octaveband_steps(&interval,dz))<0)
        return(exit_status);
    if((exit_status = sa_setup_low_octave_bands(arraycnt,dz))<0)
        return(exit_status);
    k  = TOP_OF_LOW_OCTAVE_BANDS;
    cc = CHAN_ABOVE_LOW_OCTAVES;
    while(cc <= dz->clength) {
        m = 0;
        if((bandbot = dz->chwidth * (double)cc) >= dz->nyquist)
            break;
        for(n=0;n<dz->iparam[SA_FORMANT_BANDS];n++) {
            if(k >= arraycnt) {
                sprintf(errstr,"Formant array too small: sa_set_specenv_frqs()\n");
                return(PROGRAM_ERROR);
            }
            dz->specenvfrq[k]   = (float)(bandbot * interval[m++]);
            dz->specenvpch[k]   = (float)log10(dz->specenvfrq[k]);
            dz->specenvtop[k++] = (float)(bandbot * interval[m++]);
        }   
        cc *= 2;        /* 8-16: 16-32: 32-64 etc as 8vas, then split into bands */
    }
    dz->specenvfrq[k] = (float)dz->nyquist;
    dz->specenvpch[k] = (float)log10(dz->nyquist);
    dz->specenvtop[k] = (float)dz->nyquist;
    dz->specenvamp[k] = (float)0.0;
    k++;
    dz->infile->specenvcnt = k;
    return(FINISHED);
}

/************************* SA_SETUP_OCTAVEBAND_STEPS ************************/

int sa_setup_octaveband_steps(double **interval,dataptr dz)
{
    double octave_step;
    int n = 1, m = 0, halfbands_per_octave = dz->iparam[SA_FORMANT_BANDS] * 2;
    if((*interval = (double *)malloc(halfbands_per_octave * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing interval array for formants.\n");
        return(MEMORY_ERROR);
    }
    while(n < halfbands_per_octave) {
        octave_step   = (double)n++/(double)halfbands_per_octave;
        (*interval)[m++] = pow(2.0,octave_step);
    }
    (*interval)[m] = 2.0;
    return(FINISHED);
}

/************************ SA_SETUP_LOW_OCTAVE_BANDS ***********************
 *
 * Lowest PVOC channels span larger freq steps and therefore we must
 * group them in octave bands, rather than in anything smaller.
 */

int sa_setup_low_octave_bands(int arraycnt,dataptr dz)
{
    int n;
    arraycnt   = dz->clength + 1;
    if(arraycnt < LOW_OCTAVE_BANDS) {
        sprintf(errstr,"Insufficient array space for low_octave_bands\n");
        return(PROGRAM_ERROR);
    }
    for(n=0;n<LOW_OCTAVE_BANDS;n++) {
        switch(n) {
        case(0):
            dz->specenvfrq[0] = (float)1.0;                 /* frq whose log is 0 */
            dz->specenvpch[0] = (float)0.0;
            dz->specenvtop[0] = (float)dz->chwidth;         /* 8VA wide bands */
            break;
        case(1):
            dz->specenvfrq[1] = (float)(dz->chwidth * 1.5); /* Centre Chs 1-2 */    
            dz->specenvpch[1] = (float)log10(dz->specenvfrq[1]);
            dz->specenvtop[1] = (float)(dz->chwidth * 2.0);
            break;
        case(2):
            dz->specenvfrq[2] = (float)(dz->chwidth * 3.0); /* Centre Chs 2-4 */
            dz->specenvpch[2] = (float)log10(dz->specenvfrq[2]);
            dz->specenvtop[2] = (float)(dz->chwidth * 4.0);
            break;
        case(3):
            dz->specenvfrq[3] = (float)(dz->chwidth * 6.0); /* Centre Chs 4-8 */    
            dz->specenvpch[3] = (float)log10(dz->specenvfrq[3]);
            dz->specenvtop[3] = (float)(dz->chwidth * 8.0);
            break;
        default:
            sprintf(errstr,"Insufficient low octave band setups in sa_setup_low_octave_bands()\n");
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/********************** SA_EXTRACT_SPECENV *******************/

int sa_extract_specenv(float *anal,dataptr dz)
{
    int    n, cc, vc, specenvcnt_less_one;
    int    botchan, topchan;
    double botfreq, topfreq;
    double bwidth_in_chans;
    float *ampstore;
    ampstore = dz->specenvamp;
    specenvcnt_less_one = dz->infile->specenvcnt - 1;
    vc = 0;
    for(n=0;n<dz->infile->specenvcnt;n++)
        ampstore[n] = (float)0.0;
    topfreq = 0.0f;
    n = 0;
    while(n < specenvcnt_less_one) {
        botfreq  = topfreq;
        botchan  = (int)((botfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
        botchan -= CHAN_SRCHRANGE_F;
        botchan  =  max(botchan,0);
        topfreq  = dz->specenvtop[n];
        topchan  = (int)((topfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
        topchan += CHAN_SRCHRANGE_F;
        topchan  =  min(topchan,dz->clength);
        for(cc = botchan,vc = botchan * 2; cc < topchan; cc++,vc += 2) {
            if(anal[FREQ] < 0.0)            // rectify if ness
                anal[FREQ] = (float)-anal[FREQ];
            if(anal[FREQ] >= botfreq && anal[FREQ] < topfreq)
                ampstore[n] = (float)(ampstore[n] + anal[AMPP]);
        }
        bwidth_in_chans   = (double)(topfreq - botfreq)/dz->chwidth;
        ampstore[n] = (float)(ampstore[n]/bwidth_in_chans);
        n++;
    }
    return(FINISHED);
}

/**************************** IS_A_HARMONIC_X *************************/

int is_a_harmonic_x(double frq1,double fundamental)
{
    double ratio;
    int   iratio, hcnt;
    double intvl, harmonicfrq;
    harmonicfrq = fundamental;
    hcnt = 1;
    while(harmonicfrq < frq1 * SEMITONE_INTERVAL) {
        ratio = frq1/harmonicfrq;
        iratio = round(ratio);
        if(iratio != 1) {
            if(ratio > iratio)
                intvl = ratio/(double)iratio;
            else
                intvl = (double)iratio/ratio;
            if(intvl < quartertone)
                return(TRUE);
        }
        hcnt++;
        harmonicfrq = fundamental * hcnt;
    }
    return(FALSE);
}

/******************************** EXTRACT_ENV_FROM_SNDFILE *****************************/
 
int extract_env_from_sndfile(dataptr dz)
{
    int n, i;
    float *buffer = dz->sampbuf[0];
    double thismaxsamp, thisval;
    for(n = 0; n < dz->envcnt; n++) {
        if((dz->ssampsread = fgetfbufEx(buffer, dz->iparam[SA_TSTEP],dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Can't read samples from soundfile: to extract envelope.\n");
            return(SYSTEM_ERROR);
        }
        thismaxsamp = 0.0;
        for(i = 0; i<dz->ssampsread; i++) {
            if((thisval =  fabs(buffer[i]))>thismaxsamp)           
                thismaxsamp = thisval;
        }
        dz->env[n] = (float)thismaxsamp;
    }

    //  NORMALISE

    thismaxsamp = -HUGE;
    for(n = 0; n < dz->envcnt; n++)
        thismaxsamp = max(thismaxsamp,dz->env[n]);
    if(thismaxsamp <= 0.0) {
        sprintf(errstr,"No significant level found in source sound.\n");
        return DATA_ERROR;
    }
    for(n = 0; n < dz->envcnt; n++)
        dz->env[n] = (float)(dz->env[n]/thismaxsamp);
    memset((char *)buffer,0,(dz->iparam[SA_TSTEP] * 2) * sizeof(float));
    sndseekEx(dz->ifd[0],0,0);
    return(FINISHED);
}   

/******************************************* GET_TEMPERED_PITCH ********************************/

int get_tempered_pitch(float *averages, float *averages2, float *averages3, dataptr dz) 
{
    double maxval;
    int maxpos;
    int *pitches = dz->iparray[0];
    int jj, kk;
    if(dz->pitchcnt != 0) {
        for(jj = 0, kk= 1; jj < BINCNT; jj++, kk+=2)
            averages3[kk] = averages[kk] + averages2[kk];   //  sum average levels from last 2 groups
        maxval = -10000;
        maxpos = 0;
            //  For every semitone bin                      //  Find loudest frq
        for(jj = 0, kk= 1; jj < BINCNT; jj++, kk+=2) {
            if(averages3[kk] > maxval) {                    //  is it the loudest
                maxval = averages3[kk];
                maxpos = jj;
            }
        }
        pitches[dz->pitchcnt] = maxpos - 1;                     // assign pitch (MIDI bins start at midi = -1 : so bin2 is midi=1)
    }
    memcpy((char *)averages2,(char *)averages,(BINCNT *2) * sizeof(float));
    for(jj = 0, kk= 1; jj < BINCNT; jj++, kk+=2)
        averages[kk] = 0.0;
    dz->pitchcnt++;
    return FINISHED;
}

/******************************************* GET_TEMPERED_HF ********************************/

int get_tempered_hf(float *averages, float *averages2, float *averages3, dataptr dz) 
{
    double maxval;
    int maxpos;
    int jj, kk, mm;
    if(dz->pitchcnt != 0) {
        for(jj = 0, kk= 1; jj < BINCNT; jj++, kk+=2)
            averages3[kk] = averages[kk] + averages2[kk];       //  sum average levels from last 2 groups
        for(mm = 0; mm < 8;mm++) {                              //  Get the 8 loudest peaks, in loudness order
            maxval = -10000;
            maxpos = 0;
                //  For every semitone bin                      //  Find loudest frq
            for(jj = 0, kk= 1; jj < BINCNT; jj++, kk+=2) {
                if(averages3[kk] > maxval) {                    //  is it the loudest
                    maxval = averages3[kk];
                    maxpos = jj;
                }
            }
            dz->iparray[mm][dz->pitchcnt] = maxpos - 1;         // assign pitch (MIDI bins start at midi = -1 : so bin2 is midi=1)
            kk = (maxpos * 2) + 1;
            averages3[kk] = -10000;                             // set the current loudest bin to a -ve val, so it is ignored in next pass of mm loop               
        }
    }
    memcpy((char *)averages2,(char *)averages,(BINCNT *2) * sizeof(float));
    for(jj = 0, kk= 1; jj < BINCNT; jj++, kk+=2)
        averages[kk] = 0.0;
    dz->pitchcnt++;
    return FINISHED;
}

/******************************************* SMOOTH_AND_OUTPUT_TEMPERED_PITCH ********************************/

int smooth_and_output_tempered_pitch(dataptr dz) 
{
    int n, m, t, k;
    int *pitches = dz->iparray[0], lastpitch, thispitch, pitchrepet;
    double *pichout = dz->parray[0], transittime;
    /* Put times into array */
    for(n=1, t = 2, m = 3;n < dz->envcnt; n++,t+=2,m+=2) {
        pichout[t] = dz->param[SA_TSTEP] * n;           //  Time    
        pichout[m] = pitches[n];                        //  Pitch
    }
    for(n=1,m=2;n<dz->envcnt;n++,m+=2) {
        if(dz->env[n] < dz->param[SA_GATE])             //  Mark for deletion, pitches found at below gate-level
            pichout[m] = -1.0;
        if(pichout[m] >= 0.0) {
            if((int)round(pichout[m+1]) < dz->param[SA_BOTRANGE])
                pichout[m] = -1;                        //  Mark for deletion, pitches below acceptable range
            else if((int)round(pichout[m+1]) > dz->param[SA_TOPRANGE])
                pichout[m] = -1;                    //  Mark for deletion, pitches below acceptable range   
        }
    }
    n = 1;
    m = 2;
    while(n<dz->envcnt-1) {
        if(pichout[m] < 0.0) {
            for(k = m+2; k < dz->envcnt*2; k++) //  Shuffle-down overwrite
                pichout[k-2] = pichout[k];
            dz->envcnt--;
        } else {
            n++;
            m += 2;
        }
    }
    if(pichout[m] < 0.0)
        dz->envcnt--;

    for(n=1,m=2;n<dz->envcnt;n++,m+=2) {
        if(flteq(pichout[m+1],pichout[m-1]))        //  Mark for deletion, pitches that duplicate the previous pitch
            pichout[m] = -1;
    }   
    n = 1;
    m = 2;
    while(n<dz->envcnt-1) {
        if(pichout[m] < 0.0) {
            for(k = m+2; k < dz->envcnt*2; k++) //  Shuffle-down overwrite
                pichout[k-2] = pichout[k];
            dz->envcnt--;
        } else {
            n++;
            m += 2;
        }
    }
    
    if(pichout[m] < 0.0)
        dz->envcnt--;
    if(dz->envcnt == 0) {
        sprintf(errstr,"NO PITCHES FOUND\n");
        return DATA_ERROR;
    }
    pichout[0] = 0.0;                               //  Force a value at time zero
    pichout[1] = pichout[3];

    //  Reduce data (where pitch is repeated)

    lastpitch = (int)round(pichout[1]);
    pitchrepet = 0;
    for(n = 1;n< dz->envcnt;n++) {
        k = n * 2;                              //  time-index in output array
        m = k+1;                                //  pitch-index in output array     
        thispitch = (int)round(pichout[m]);
        if(thispitch == lastpitch)
            pitchrepet++;
        else {
            if(pitchrepet) {                    //  If a pitch repeats
                k = n;                          //  Copy pitches down over eliminated pitches
                m = n - pitchrepet;
                k = k * 2;
                m = m * 2;
                while(k < dz->envcnt * 2)
                    pichout[m++] = pichout[k++];
                dz->envcnt -= pitchrepet;
                n -= pitchrepet;
            }
            lastpitch = thispitch;
            pitchrepet = 0;
        }
    }
    for(n = 0,m=0;n< dz->envcnt;n++,m+=2) {
        fprintf(dz->fp,"%f\t%d\t1\n",pichout[m],(int)round(pichout[m+1]));
        if(n < dz->envcnt-1) {
            transittime = min(SA_TRANSIT,(pichout[m+2] - pichout[m])/2.0);
            fprintf(dz->fp,"%f\t%d\t1\n",pichout[m+2] - transittime,(int)round(pichout[m+1]));
        } else
            fprintf(dz->fp,"10000\t%d\t1\n",(int)round(pichout[m+1]));
    }
    return FINISHED;
}

/******************************************* SMOOTH_AND_OUTPUT_VARYING_HF ********************************/

int smooth_and_output_varying_hf(dataptr dz) 
{
    int exit_status, reranged[8], lastpitch[8], thispitch[8], does_repeat;
    int n, m, t, k, f, omitsum = 0, hfrepet;
    int itemp, peakcnt, omit[8];
    double transittime, temp;
    char tempstr[4000], tempstr2[4000];
    int downtranspos = 0;

    /* Eliminate harmonics of lower peaks */
    if(dz->iparam[SA_PKCNT] < 8) {
        for(n=1;n < dz->envcnt; n++) {          /* Initially, arrays 0-7 run from loudest to quietest pitch */
            for(m=0,k = 7;m<4;m++,k--) {        /* invert order of array pitches at time 'n', so they run from quietest to loudest */
                itemp = dz->iparray[m][n];
                dz->iparray[m][n] = dz->iparray[k][n];
                dz->iparray[k][n] = itemp;
            }
            peakcnt = 8;
            for(m=0;m<8;m++) {
                if(dz->iparray[m][n] < 0)       // skip arrays that have already been eliminated from search
                    continue;
                for(t=0;t<peakcnt;t++) {        //  Compare each peak pitch with all others
                    if(t==m)                    //  except itself
                        continue;
                    if(dz->iparray[t][n] < 0)   // skip arrays that have already been eliminated from search
                        continue;
                                                // If one partial is a harmonic of another, mark it for elimination
                    if(dz->iparray[m][n] == dz->iparray[t][n] + 12 || dz->iparray[m][n] == dz->iparray[t][n] + 19 || dz->iparray[m][n] == dz->iparray[t][n] + 24) {
                        dz->iparray[m][n] = -1;
                        peakcnt--;              //  count remaining peaks
                        break;                  //  and abandon comparison with other peaks
                    }
                }
                if(peakcnt <= dz->iparam[SA_PKCNT])// if only the required no of peaks remains, stop    
                    break;
            }
            for(m=0,k = 7;m<4;m++,k--) {        /* invert order of array pitches at time 'n', so loudest is below quietest */
                itemp = dz->iparray[m][n];
                dz->iparray[m][n] = dz->iparray[k][n];
                dz->iparray[k][n] = itemp;
            }
            k = 0;                              //  Move SA_PKCNT valid peaks to topmost arrays 0 to SA_PKCNT-1
            for(m=0;m<8;m++) {
                if(dz->iparray[m][n] >= 0) {
                    if(k!=m)                    //  Move item up arrays only if ness        
                        dz->iparray[k][n] = dz->iparray[m][n];
                    k++;
                    if(k >= dz->iparam[SA_PKCNT])
                        break;
                }
            }
        }
    }
    /* Put times into arrays */
    for(n=1, t = 2, m = 3;n < dz->envcnt; n++,t+=2,m+=2) {
        for(k=0;k<dz->iparam[SA_PKCNT];k++) {
            dz->parray[k][t] = dz->param[SA_TSTEP] * n;     //  Time    
            dz->parray[k][m] = (double)dz->iparray[k][n];   //  Pitch
        }
    }
    for(n=1,t=2;n<dz->envcnt;n++,t+=2) {
        if(dz->env[n] < dz->param[SA_GATE])                 //  Mark for deletion, pitches found where sound is below gate-level
            dz->parray[0][t] = -1.0;
    }
    n = 1;
    m = 2;
    while(n<dz->envcnt-1) {
        if(dz->parray[0][m] < 0.0) {
            for(k=0;k<dz->iparam[SA_PKCNT];k++) {
                for(t = m+2; t < dz->envcnt*2; t++)         //  Shuffle-down overwrite
                    dz->parray[k][t-2] = dz->parray[k][t];
            }
            dz->envcnt--;
        } else {
            n++;
            m += 2;
        }
    }
    if(dz->parray[0][m] < 0.0)
        dz->envcnt--;

    if(dz->envcnt == 0) {
        sprintf(errstr,"NO PITCHES FOUND\n");
        return DATA_ERROR;
    }
    for(k=0;k<dz->iparam[SA_PKCNT];k++) {
        dz->parray[k][0] = 0.0;                             //  Force a value at time zero
        dz->parray[k][1] = dz->parray[k][3];
    }

                                                            //  Force pitches, at each time 'n', into ascending order
    for(n=0,f=1;n<dz->envcnt;n++,f+=2) {
        for(k=0;k<dz->iparam[SA_PKCNT]-1;k++) {
            for(m=k+1;m<dz->iparam[SA_PKCNT];m++) {
                if(dz->parray[m][f] < dz->parray[k][f]) {
                    temp = dz->parray[k][f];
                    dz->parray[k][f] = dz->parray[m][f];
                    dz->parray[m][f] = temp;
                }
            }
        }
    }

    //  If any filter strand is everywhere out of range, mark it for elimination

    for(k=0;k<dz->iparam[SA_PKCNT];k++) {
        omit[k] = 1;
        for(n=0,f=1;n<dz->envcnt;n++,f+=2) {
            if(dz->parray[k][f] >= dz->param[SA_BOTRANGE] && dz->parray[k][f] <= dz->param[SA_TOPRANGE]) {
                omit[k] = 0;
                break;
            }
        }
    }

    //  Delete out-of-range threads by shuffle-down overwrite

    for(k=0;k<dz->iparam[SA_PKCNT];k++) {
        if(omit[k]) {
            omitsum++;
            if(k < dz->iparam[SA_PKCNT] - 1) {
                for(m=k+1;m<dz->iparam[SA_PKCNT];m++)
                    dz->parray[m-1] = dz->parray[m];
            }
            if(--(dz->iparam[SA_PKCNT]) <= 0) {
                sprintf(errstr,"ALL EXTRACTED HARMONIC FIELD COMPONENTS ARE OUT OF RANGE.\n");
                return DATA_ERROR;
            }
        }
    }
    if(omitsum) {
        if(omitsum == 1)
            fprintf(stdout,"WARNING: 1 PEAK-STRAND ENTIRELY OUT OF RANGE. REDUCED PEAK-COUNT TO %d\n",dz->iparam[SA_PKCNT]);
        else
            fprintf(stdout,"WARNING: %d PEAK-STRANDS ARE ENTIRELY OUT OF RANGE. REDUCED PEAK-COUNT TO %d\n",omitsum,dz->iparam[SA_PKCNT]);
        fflush(stdout);
    }
    
    //  Data-reduce (remove repeating filters)

    for(t = 0;t < dz->iparam[SA_PKCNT];t++)
        lastpitch[t] = (int)round(dz->parray[t][1]);
    hfrepet = 0;
    for(n = 1;n< dz->envcnt;n++) {
        k = n * 2;                              //  time-index in output arrays
        m = k+1;                                //  pitch-index in output arrays        
        does_repeat = 1;
        for(t = 0;t < dz->iparam[SA_PKCNT];t++) {
            thispitch[t] = (int)round(dz->parray[t][m]);
            if(thispitch[t] != lastpitch[t]) {  //  If this and lasttime pitches are NOT the same
                                                // if both are out of range, both are zeroed in the output, so in effect equivalent
                if((thispitch[t] < dz->param[SA_BOTRANGE] || thispitch[t] > dz->param[SA_TOPRANGE])
                && (lastpitch[t] < dz->param[SA_BOTRANGE] || lastpitch[t] > dz->param[SA_TOPRANGE]))
                    continue;
                does_repeat = 0;                //  Otherwise, pitches not the same, so HF not repeated.
                break;
            }
        }
        if(does_repeat)
            hfrepet++;
        else {
            if(hfrepet) {                       //  If an HF repeats
                k = n;                          //  Copy all HF pitches down over eliminated pitches
                m = k - hfrepet;
                k = k * 2;
                m = m * 2;
                while(k < dz->envcnt * 2) {
                    for(t = 0;t < dz->iparam[SA_PKCNT];t++)
                        dz->parray[t][m] = dz->parray[t][k];
                    k++;
                    m++;
                }
                dz->envcnt -= hfrepet;
                n -= hfrepet;
            }
            for(t = 0;t < dz->iparam[SA_PKCNT];t++)
                lastpitch[t] = thispitch[t];
            hfrepet = 0;
        }
    }

    //  Attempt downward octave transpos, if flagged


    if(dz->vflag[SA_DOWNOCT]) { 
        downtranspos = 1;
        for(n = 0,t=1;n < dz->envcnt;n++,t+=2) {
            for(k=0;k<dz->iparam[SA_PKCNT];k++) {
                if(dz->parray[0][m+1] - 12.0 < 0) {
                    downtranspos = 0;
                    break;
                }
            }
            if(k<dz->iparam[SA_PKCNT])
                break;
        }
        if(!downtranspos) {
            fprintf(stdout,"WARNING: OCTAVE DOWN TRANSPOSITION NOT POSSIBLE.\n");
            fflush(stdout);
        }
    } else if(dz->vflag[SA_DOWN2OCT]) { 
        downtranspos = 2;
        for(n = 0,t=1;n < dz->envcnt;n++,t+=2) {
            for(k=0;k<dz->iparam[SA_PKCNT];k++) {
                if(dz->parray[0][m+1] - 24.0 < 0) {
                    downtranspos = 0;
                    break;
                }
            }
            if(k<dz->iparam[SA_PKCNT])
                break;
        }
        if(!downtranspos) {
            fprintf(stdout,"WARNING: TWO OCTAVE DOWN TRANSPOSITION NOT POSSIBLE.\n");
            fflush(stdout);
        }
    }
                                                //  Print to output, zeroing any out-of-range pitches
    for(n = 0,m=0;n < dz->envcnt;n++,m+=2) {
        reranged[0] = 0;
        if(dz->parray[0][m+1] < dz->param[SA_BOTRANGE] || dz->parray[0][m+1] > dz->param[SA_TOPRANGE]) {
            // FORCE ELEMENT TO BE IN RANGE, BUT ZERO IT
            if((exit_status = rerange_outofrange_pitch(0,n,dz))<0)
                return exit_status;
            reranged[0] = 1;
            if(downtranspos)
                dz->parray[0][m+1] -= (12.0 * downtranspos);
            sprintf(tempstr,"%f\t%d\t0",dz->parray[0][m],(int)round(dz->parray[0][m+1]));
        } else {
            if(downtranspos)
                dz->parray[0][m+1] -= 12.0;
            sprintf(tempstr,"%f\t%d\t1",dz->parray[0][m],(int)round(dz->parray[0][m+1]));
        }
        for(k=1;k<dz->iparam[SA_PKCNT];k++) {
            reranged[k] = 0;
            if(dz->parray[k][m+1] < dz->param[SA_BOTRANGE] || dz->parray[k][m+1] > dz->param[SA_TOPRANGE]) {
                if((exit_status = rerange_outofrange_pitch(k,n,dz))<0)
                    return exit_status;
                reranged[k] = 1;
                if(downtranspos)
                    dz->parray[k][m+1] -= 12.0;
                sprintf(tempstr2,"\t%d\t0",(int)round(dz->parray[k][m+1]));
            } else {
                if(downtranspos)
                    dz->parray[k][m+1] -= 12.0;
                sprintf(tempstr2,"\t%d\t1",(int)round(dz->parray[k][m+1]));
            }
            strcat(tempstr,tempstr2);
        }
        fprintf(dz->fp,"%s\n",tempstr);
        if(n < dz->envcnt-1) {
            transittime = min(SA_TRANSIT,(dz->parray[0][m+2] - dz->parray[0][m])/2.0);
            if(reranged[0])
                sprintf(tempstr,"%f\t%d\t0",dz->parray[0][m+2] - transittime,(int)round(dz->parray[0][m+1]));
            else
                sprintf(tempstr,"%f\t%d\t1",dz->parray[0][m+2] - transittime,(int)round(dz->parray[0][m+1]));
            for(k=1;k<dz->iparam[SA_PKCNT];k++) {
                if(reranged[k])
                    sprintf(tempstr2,"\t%d\t0",(int)round(dz->parray[k][m+1]));
                else
                    sprintf(tempstr2,"\t%d\t1",(int)round(dz->parray[k][m+1]));
                strcat(tempstr,tempstr2);
            }
            fprintf(dz->fp,"%s\n",tempstr);
        }
    }   
    // Extend end value to beyond source end
    if(reranged[0])
        sprintf(tempstr,"10000\t%d\t0",(int)round(dz->parray[0][m-1]));
    else
        sprintf(tempstr,"10000\t%d\t1",(int)round(dz->parray[0][m-1]));
    for(k=1;k<dz->iparam[SA_PKCNT];k++) {
        if(reranged[k])
            sprintf(tempstr2,"\t%d\t0",(int)round(dz->parray[k][m-1]));
        else
            sprintf(tempstr2,"\t%d\t1",(int)round(dz->parray[k][m-1]));
        strcat(tempstr,tempstr2);
    }
    fprintf(dz->fp,"%s\n",tempstr);
    return FINISHED;
}

/********************************************** RERANGE_OUTOFRANGE_PITCH **************************************
 *
 * If outofrange pitches remian in the filter definition they
 * (1) May cause unwanted glissi at transition points (from one pitch to another) in a filter thread
 * (2) May produce a false reading for the highest possible harmonic in the varibank interface.
 */

int rerange_outofrange_pitch(int strandindex,int eventindex,dataptr dz)
{
    int gotpre = 0, gotpost = 0;
    int thisevent, pitchindex;
    double prepitch = 0, postpitch = 0, newpitch;
    if(eventindex > 0) {
        thisevent = eventindex;
        while(thisevent > 0) {              //  Find 1st earlier pitch which is in range
            thisevent--;
            pitchindex = (thisevent * 2) + 1;
            if(dz->parray[strandindex][pitchindex] < dz->param[SA_BOTRANGE] || dz->parray[strandindex][pitchindex] > dz->param[SA_TOPRANGE])
                continue;
            else {
                gotpre = 1;
                prepitch = dz->parray[strandindex][pitchindex];
                break;
            }
        }
    }
    if(eventindex < dz->envcnt-1) {
        thisevent = eventindex;             //  Find first later pitch which is in range
        while(thisevent < dz->envcnt) {
            thisevent++;
            pitchindex = (thisevent * 2) + 1;
            if(dz->parray[strandindex][pitchindex] < dz->param[SA_BOTRANGE] || dz->parray[strandindex][pitchindex] > dz->param[SA_TOPRANGE])
                continue;
            else {
                gotpost = 1;
                postpitch = dz->parray[strandindex][pitchindex];
                break;
            }
        }
    }
    if(!gotpre && !gotpost) {
        sprintf(errstr,"ERROR IN OUT-OF-RANGE PITCH PROCESSING.\n");
        return PROGRAM_ERROR;
    }
    if(gotpre && gotpost)
        newpitch = (double)((int)round((prepitch + postpitch)/2.0));
    else if(!gotpre)
        newpitch = postpitch;
    else
        newpitch = prepitch;

    pitchindex = (eventindex * 2) + 1;
    dz->parray[strandindex][pitchindex] = newpitch;
    return FINISHED;
}
