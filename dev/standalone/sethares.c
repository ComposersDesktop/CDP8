/*
 * Copyright (c) 1983-2020 Trevor Wishart and Composers Desktop Project Ltd
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
 *  PROCESS TO EXTRACT PEAKS IN SPECTRUM
 *
 *  Based on Bill Sethares idea of using the median as a reference level
 *  described verbally at CCMIX session in Paris.
 *
 *  M_WINSIZE    = parameter for size of peak-search subwindow, in semitones
 *  M_PEAKING    = How much louder than median must a peak be (Range 1 ->)
 *  M_AMPFLOOR   = (relative) level below which peaks are ignored
 *  M_INTUNE     = parameter for in-tune-ness of harmonics, in semitones
 *  M_LOFRQ      = min frq to keep
 *  M_HIFRQ      = max frq to keep
 *  MODES 0 pitch(amp) data: 1 data in streams: 2 data streamed after statistical check
 *  dz->vflag[0] = lose the amplitude information
 *  dz->vflag[1] = mark_zeros, where no peaks found
 *
 *  Output goes to a textfile of
 *  (1) Time, Pitch and amplitude data OR 
 *  (2) Time and Pitch data only
    where ...
 *  (1) Areas free of peaks may be ignored, or marked with zeros
 *  (2) Data may be streamed. The number of streams = maximum number peaks found.
 *      Where there are insufficient peaks in a window to fill all streams,
 *      (zero amplitude) best-fit virtual frqs are added to the streams for those windows.
 *      Streaming may be slow, as assiging peaks to streams uses a combinatorial optimisation scheme.
 *  (3) A statistical survey of the data may be attempted, to extract more streams
 *      This assumes that the source has a relatively steady spectrum
 *  Currently NO data reduction.
 */

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
#include <speccon.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

static int check_the_param_validity_and_consistency(dataptr dz);
static int setup_the_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_the_param_ranges_and_defaults(dataptr dz);
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
static int get_the_mode_no(char *str, dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int allocate_buffer(dataptr dz);
static void displaytime(double time,dataptr dz);

static int is_a_harmonic(double frq1,double frq2,double intune_ratio);
static void eliminate_harmonics(float *peakfrq,float *peakamp,int *len,double intune_ratio);
static void eliminate_duplicated_frqs(float *peakfrq,float *peakamp,int *len);
static void sortfrqs(float *peakfrq,float *peakamp,int len);
static void sortamps(float *medianarray,int * chanarray,int len);
static int locate_peaks_in_subwindow(double lofrq,double hifrq,float *medianarray,int *chanarray,char *peakchan,float * peakfrq,float * peakamp,int peakcnt,dataptr dz);
static int locate_peaks(float *medianarray,int *chanarray,char *peakchan,float *peakfrq,float *peakamp,double frqratio,dataptr dz);
static void combinations(int *combo,int k,int i, int j,int peakcnt,int maxpeakcnt,double *minratiosum,int *instream,
                float *peakfrq,float *maxpeakfrq,float *streamfrq,int more);
static void find_best_fit(int *combo,int peakcnt,double *minratiosum,int *instream,float *peakfrq,float *streamfrq);
static int stream_peaks(float *peakfrq,float *peakamp,float *maxpeakfrq,float *lastmaxpeakfrq,float *streamfrq,
                 int wcnt,int lastmax,int firstmax,int maxpeakcnt,int peakcnt,int *instream,
                 float *streampeakfrq,float *streampeakamp,dataptr dz);
static int get_next_window_with_max_number_of_peaks(int *peakcnt,int maxpeakcnt,float *lastmaxpeakfrq,float *maxpeakfrq,
        int *firstmax,int *lastmax,int wcnt,double frqratio,
        int *peaknos,float *medianarray,int *chanarray,char *peakchan,float *peakfrq,float *peakamp,dataptr dz);
static int get_initial_data(float *maxwinamp,int *peakcnt,int *maxpeakcnt,int *firstmax,float *medianarray,int *chanarray,char *peakchan,
        float *peakfrq,float *peakamp,int *peaknos,float *maxpeakfrq,float *lastmaxpeakfrq,double frqratio,
            int bincnt,float *binlofrq,int *pitchbin,float *bintrof,float *streamfrq,dataptr dz);
static int extract_peaks(dataptr dz);

#define lose_amps    dz->vflag[0]
#define as_midi      dz->vflag[1]
#define quantised    dz->vflag[2]
#define mark_zeros   dz->vflag[3]
#define filt_format  dz->vflag[4]

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
    //aplptr ap;
    int is_launched = FALSE;
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
        dz->maxmode = 4;
        if((get_the_mode_no(cmdline[0],dz))<0)
            return(FAILED);
        cmdline++;
        cmdlinecnt--;
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
    //ap = dz->application;

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

//  handle_extra_infiles(): redundant
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
//  handle_special_data()       redundant except
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //check_param_validity_and_consistency ....
    if((exit_status = check_the_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;

    //allocate_large_buffers() ... replaced by  CDP LIB
    dz->extra_bufcnt =  0;
    dz->bptrcnt = 1;
    if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = allocate_buffer(dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    
    if((exit_status = extract_peaks(dz))< 0) {
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

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int exit_status;
    char *filename = NULL;
    filename = (*cmdline)[0];
    strcpy(dz->outfilename,filename);      
    if((exit_status = create_sized_outfile(filename,dz))<0)
        return(exit_status);
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
    exit_status = set_param_data(ap,0   ,5,5,"ddddd");
    if(exit_status <0)
        return(FAILED);
    switch(dz->mode) {
    case(0):
        exit_status = set_vflgs(ap,"h",1,"d","amqz",4,0,"0000");
        break;
    case(1):
    case(2):
    case(3):
        exit_status = set_vflgs(ap,"h",1,"d","amqzf",5,0,"00000");
        break;
    }
    if(exit_status<0)
        return(FAILED);
    dz->has_otherfile = FALSE;
    dz->input_data_type = ANALFILE_ONLY;
    dz->process_type    = TO_TEXTFILE;  
    dz->outfiletype     = TEXTFILE_OUT;
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
            sprintf(errstr,"Failed tp parse input file %s\n",cmdline[0]);
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
    ap->lo[0]           = 1;        //  winsize: semitones
    ap->hi[0]           = 96;
    ap->default_val[0]  = 12;
    ap->lo[1]           = 1;        //  peaking: ratio to median
    ap->hi[1]           = 1000;
    ap->default_val[1]  = 1.5;
    ap->lo[2]           = .0001;    //  floor: ratio to max window in entire file
    ap->hi[2]           = 1;
    ap->default_val[2]  = .001;
    ap->lo[3]           = dz->chwidth;  //  min frq to find
    ap->hi[3]           = dz->nyquist;
    ap->default_val[3]  = dz->chwidth;
    ap->lo[4]           = dz->chwidth;  //  max frq to find
    ap->hi[4]           = dz->nyquist;
    ap->default_val[4]  = dz->nyquist;
    ap->lo[5]           = 0.0;      //  intunenuess of harmonics, semitones
    ap->hi[5]           = 6.0;
    ap->default_val[5]  = 1.0;
    dz->maxmode = 4;
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
    //aplptr ap;

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
            //ap = dz->application;
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

/************************************ CHECK_THE_PARAM_VALIDITY_AND_CONSISTENCY **************************************/

 int check_the_param_validity_and_consistency(dataptr dz)
{
    if(dz->param[M_LOFRQ] >= dz->param[M_HIFRQ]) {
        sprintf(errstr,"Low frequency limit (%lf) equal to, or greater than high frequency limit (%lf).\n",dz->param[M_LOFRQ],dz->param[M_HIFRQ]);
        return(USER_ERROR);
    }
    if(dz->mode > 1) {
        if(filt_format) {
            if(mark_zeros) {
                mark_zeros = 0;
                fprintf(stdout,"WARNING: filter format output overrides marking no-peak areas.\n");
                fflush(stdout);
            }
        }
    }
    return FINISHED;
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
    return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if      (!strcmp(prog_identifier_from_cmdline,"extract"))       dz->process = SETHARES;
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        return(USAGE_ONLY);
    }
    return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
    usage2("extract");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"extract")) {
        fprintf(stderr,
        "USAGE: peak extract\n"
        "1   analfile outf winsiz peak floor lo hi [-htune] [-a] [-m] [-q] [-z]\n"
        "2-4 analfile outf winsiz peak floor lo hi [-htune] [-a] [-m] [-q] [-z] [-f]\n"
        "\n"
        "MODE 1 Output time and frq:amp for peaks in each window.\n"
        "          Number of peaks may vary from window to window.\n"
        "MODE 2 Assign peaks to max number of pitch-streams.\n"
        "          Number of streams = max number of peaks found in any of windows.\n"
        "          Where windows have too few peaks to fit all these streams,\n"
        "          frq vals (at zero amplitude) at the (current) best-fit frq \n"
        "          are set in the stream. All peaks will lie in some stream.\n"
        "MODE 3 Assign peaks to most prominent pitch-streams.\n"
        "          This mode assumes spectrum is relatively constant.\n"
        "          It parses entire file, counting peaks in every semitone interval.\n"
        "          Then uses most visited semitones as stream centres.\n"
        "          Time-varying pitch(amp) data then assigned to those streams.\n"
        "          Peaks may be discarded where insufficient streams available.\n"
        "MODE 4 Find streams as mode 3, then get average pitch + relative amp\n"
        "          of each stream. Output does not vary through time.\n"
        "\n"
        "winsiz semitone width of window used to search for peaks in spectrum.\n"
        "       (Range 1 - 96)\n"
        "peak   To retain as peak, channel amp must be 'peak' times louder than\n"
        "       median amp of all channels in window (Range 1 - 1000)\n"
        "floor  Peaks must exceed a floor amplitude to be retained (Range 0.0001 - 1).\n"
        "       Entered val is multiplied by max channel amp found anywhere in file.\n"
        "lo     Minimum frq to accept as a peak (Range analchanwidth - nyquist)\n"
        "hi     Maximum frq to accept as a peak (Range analchanwidth - nyquist)\n"
        "tune   If non-zero, any peak which is a harmonic of another peak\n"
        "       (to within 'tune' semitones accuracy) is discarded (Range 0 - 6).\n"
        "       If set to zero, all peaks are retained.\n"
        "-a  Lose the amplitude information:\n"
        "       If set, amps information discarded.\n"
        "-m  Output frq information as midi:\n"
        "-q  Output frq quantised to tempered scale (in quarter-tones):\n"
        "-z  Mark peak-free segments:\n"
        "       start/end of peak-free areas marked by (timed) zeros in output.\n"
        "-f  Output in filter varibank format: This flag cancels the -z flag.\n"
        "       If -a flag also set, all outputs amps are set to 1.0.\n"
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

/**************************** ALLOCATE_BUFFER ******************************/

int allocate_buffer(dataptr dz)
{
    dz->buflen = dz->wanted;
    if((dz->bigfbuf = (float*) malloc(dz->buflen * sizeof(float)))==NULL) {  
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->flbufptr[0] = dz->bigfbuf;
    return(FINISHED);
}

/**************************** EXTRACT_PEAKS ******************************/

int extract_peaks(dataptr dz)
{
    int exit_status;
    char temp2[20000];
    char temp[20000];
    char *peakchan;
    float *peakfrq, *peakamp, *medianarray;
    int *chanarray, *peaknos, *instream;
    float *maxpeakfrq, *lastmaxpeakfrq, *streamfrq, *streampeakfrq, *streampeakamp, *binlofrq, *bintrof;
    double frqratio, lasttime = 0.0, lastholetime = 0.0, maxamp, ampratio, normaliser;
    int firstmax = 0, lastmax = 0, wcnt, samps_read, *pitchbin, val;
    int maxpeakcnt = -1, peakcnt=0, inhole, k;
    float maxwinamp = 0.0;
    double time;
    int bincnt;
    double frq;
    double *avamp, *avfrq;

    if(dz->param[M_INTUNE] > 0.0)
        dz->param[M_INTUNE] = pow(SEMITONE_INTERVAL,fabs(dz->param[M_INTUNE])); 
    dz->total_samps_read = 0;
    frqratio = pow(SEMITONE_INTERVAL,fabs(dz->param[M_WINSIZE]));
    if((peakchan = (char *)malloc(dz->clength * sizeof(char))) == NULL) {
        sprintf(errstr,"Insufficient memory for peakchan arrays.\n");
        return(MEMORY_ERROR);
    }
    if((peakfrq = (float *)malloc(dz->clength * sizeof(float))) == NULL) {
        sprintf(errstr,"Insufficient memory for peak frq arrays.\n");
        return(MEMORY_ERROR);
    }
    if((peakamp = (float *)malloc(dz->clength * sizeof(float))) == NULL) {
        sprintf(errstr,"Insufficient memory for peak amp arrays.\n");
        return(MEMORY_ERROR);
    }
    if((medianarray = (float *)malloc(dz->clength * sizeof(float))) == NULL) {
        sprintf(errstr,"Insufficient memory for meanval arrays.\n");
        return(MEMORY_ERROR);
    }
    if((chanarray = (int *)malloc(dz->clength * sizeof(int))) == NULL) {
        sprintf(errstr,"Insufficient memory for meanchannel arrays.\n");
        return(MEMORY_ERROR);
    }
    if((peaknos = (int *)malloc(dz->wlength * sizeof(int))) == NULL) {
        sprintf(errstr,"Insufficient memory for peakchan arrays.\n");
        return(MEMORY_ERROR);
    }
    if((maxpeakfrq = (float *)malloc(dz->clength * sizeof(float))) == NULL) {
        sprintf(errstr,"Insufficient memory for peak frq arrays.\n");
        return(MEMORY_ERROR);
    }
    if((lastmaxpeakfrq = (float *)malloc(dz->clength * sizeof(float))) == NULL) {
        sprintf(errstr,"Insufficient memory for peak frq arrays.\n");
        return(MEMORY_ERROR);
    }
    if((streamfrq = (float *)malloc(dz->clength * sizeof(float))) == NULL) {
        sprintf(errstr,"Insufficient memory for peak amp arrays.\n");
        return(MEMORY_ERROR);
    }
    if((instream = (int *)malloc(dz->clength * sizeof(int))) == NULL) {
        sprintf(errstr,"Insufficient memory for peak amp arrays.\n");
        return(MEMORY_ERROR);
    }
    if((streampeakfrq = (float *)malloc(dz->clength * sizeof(float))) == NULL) {
        sprintf(errstr,"Insufficient memory for orig peak frq arrays.\n");
        return(MEMORY_ERROR);
    }
    if((streampeakamp = (float *)malloc(dz->clength * sizeof(float))) == NULL) {
        sprintf(errstr,"Insufficient memory for orig peak amp arrays.\n");
        return(MEMORY_ERROR);
    }
    if((avamp = (double *)malloc(dz->clength * sizeof(double))) == NULL) {
        sprintf(errstr,"Insufficient memory for orig peak amp arrays.\n");
        return(MEMORY_ERROR);
    }
    if((avfrq = (double *)malloc(dz->clength * sizeof(double))) == NULL) {
        sprintf(errstr,"Insufficient memory for orig peak amp arrays.\n");
        return(MEMORY_ERROR);
    }
    //range = dz->nyquist/dz->halfchwidth;
    frq = dz->halfchwidth;
    bincnt = 0;
    while(frq <= dz->nyquist) {
        bincnt++;
        frq *= SEMITONE_INTERVAL;
    }
    bincnt++;
    if((binlofrq = (float *)malloc(bincnt * sizeof(float))) == NULL) {
        sprintf(errstr,"Insufficient memory for peakchan arrays.\n");
        return(MEMORY_ERROR);
    }
    if((pitchbin = (int *)malloc(bincnt * sizeof(int))) == NULL) {
        sprintf(errstr,"Insufficient memory for peakchan arrays.\n");
        return(MEMORY_ERROR);
    }
    if((bintrof = (float *)malloc(dz->clength * sizeof(float))) == NULL) {
        sprintf(errstr,"Insufficient memory for peakchan arrays.\n");
        return(MEMORY_ERROR);
    }
    memset((char *)pitchbin,0,bincnt * sizeof(int));
    k = 0;
    binlofrq[k++] = (float)dz->halfchwidth;
    while(k < bincnt) {
        binlofrq[k] = (float)(binlofrq[k-1] * SEMITONE_INTERVAL);
        k++;
    }
    if((exit_status = get_initial_data(&maxwinamp,&peakcnt,&maxpeakcnt,&firstmax,medianarray,chanarray,
        peakchan,peakfrq,peakamp,peaknos,maxpeakfrq,lastmaxpeakfrq,frqratio,bincnt,binlofrq,pitchbin,bintrof,streamfrq,dz))<0)
        return exit_status;
    dz->param[M_AMPFLOOR] *= maxwinamp;     //  Set ampfloor relative to loudest window found
    normaliser = 1.0/maxwinamp;
    time = 0.0;
    inhole = 0;
    wcnt = 0;
    dz->total_samps_read = 0;
    while((samps_read = fgetfbufEx(dz->bigfbuf,dz->buflen,dz->ifd[0],0)) > 0) {
        peakcnt = locate_peaks(medianarray,chanarray,peakchan,peakfrq,peakamp,frqratio,dz);
        if(dz->mode > 0) {
            if(dz->mode == 1) {
                if(wcnt == firstmax) {
                    if((exit_status = get_next_window_with_max_number_of_peaks(&peakcnt,maxpeakcnt,lastmaxpeakfrq,maxpeakfrq,&firstmax,&lastmax,wcnt,frqratio,
                    peaknos,medianarray,chanarray,peakchan,peakfrq,peakamp,dz))<0) 
                        return exit_status;
                }
            }
            if(peakcnt != maxpeakcnt) {
                stream_peaks(peakfrq,peakamp,maxpeakfrq,lastmaxpeakfrq,streamfrq,wcnt,lastmax,firstmax,maxpeakcnt,peakcnt,
                    instream,streampeakfrq,streampeakamp,dz);
                peakcnt = maxpeakcnt;
            }
        }
        if((dz->mode == 2) && filt_format) {
            for(k=0;k<peakcnt;k++)
                peakamp[k] = (float)(peakamp[k] * normaliser);
        }
        if(dz->mode == 3) {
            if(peakcnt > 0) {
                for(k=0;k<peakcnt;k++) {
                    avfrq[k] += peakfrq[k];
                    avamp[k] += peakamp[k];
                }
            }
        } else {
            if(peakcnt > 0) {
                if(mark_zeros && inhole) {                  //  If not interpolating and at end of peakless time-period
                    if(!flteq(lasttime,lastholetime)) 
                        sprintf(temp,"%lf\t0 0",lasttime);  //  Mark end of peakless time, if necessary
                    inhole = 0;
                }
                sprintf(temp,"%lf",time);                   //  Write peaks (frq+amp) found at current time
                for(k=0;k<peakcnt;k++) {
                    if(quantised || as_midi) {
                        peakfrq[k] = (float)unchecked_hztomidi(peakfrq[k]);
                        if(quantised) {
                            val = (int)round(peakfrq[k] * 2.0);
                            peakfrq[k] = (float)((double)val/2.0);
                        }
                        if(!as_midi)
                            peakfrq[k] = (float)miditohz(peakfrq[k]);
                    }
                    strcat(temp,"\t");
                    if(quantised && as_midi)
                        sprintf(temp2," %.1lf",peakfrq[k]);
                    else
                        sprintf(temp2," %lf",peakfrq[k]);
                    strcat(temp,temp2);
                    if(!lose_amps) {                        //  Print amplitudes, if amps not suppressed
                        sprintf(temp2," %lf",peakamp[k]);
                        strcat(temp,temp2);                 //  If filter-format and amps suppressed, set amps to 1.0, and print
                    } else if((dz->mode > 0) && filt_format) {
                        sprintf(temp2," 1.0");
                        strcat(temp,temp2);
                    }
                }
                if((exit_status = fprintf(dz->fp,"%s\n",temp))<0)
                    return exit_status;
            } else if(mark_zeros && !inhole) {              //  If no peaks found + not interpolating + not already in peakless time-period
                sprintf(temp,"%lf\t0 0",time);              //  Mark start of peakless-time
                if((exit_status = fprintf(dz->fp,"%s\n",temp))<0)
                    return exit_status;
                lastholetime = time;
                inhole = 1;
            }
        }
        lasttime = time;
        time += dz->frametime;
        dz->total_samps_read += samps_read;
        displaytime(time,dz);
    }
    if(samps_read < 0) {
        sprintf(errstr,"Sample Read Failed After %d Samples\n",dz->total_samps_read);
        return(SYSTEM_ERROR);
    }
    if(dz->mode == 3) {
        maxamp = 0.0;
        for(k=0;k<peakcnt;k++) {
            avfrq[k] /= (double)dz->wlength;
            maxamp = max(avamp[k],maxamp);
        }
        if(maxamp <= 0.0) {
            sprintf(errstr,"No significant peak level found.\n");
            return(DATA_ERROR);
        }
        ampratio = 1.0/maxamp;
        if(filt_format) {
            temp2[0] = ENDOFSTR;
            for(k=0;k<peakcnt;k++) {
                if(as_midi || quantised) {
                    avfrq[k] = unchecked_hztomidi(avfrq[k]);
                    if(quantised) {
                        val = (int)round(avfrq[k] * 2.0);
                        avfrq[k] = (double)val/2.0;
                    }
                    if(!as_midi)
                        avfrq[k] = miditohz(avfrq[k]);
                }
                if(lose_amps) {
                    if(quantised && as_midi)
                        sprintf(temp,"\t%.1lf\t1.0",avfrq[k]);
                    else
                        sprintf(temp,"\t%lf\t1.0",avfrq[k]);
                } else {
                    if(quantised && as_midi)
                        sprintf(temp,"\t%.1lf\t%lf",avfrq[k],avamp[k] * ampratio);
                    else
                        sprintf(temp,"\t%lf\t%lf",avfrq[k],avamp[k] * ampratio);
                }
                strcat(temp2,temp);
            }
            sprintf(temp,"0");
            strcat(temp,temp2);
            if((exit_status = fprintf(dz->fp,"%s\n",temp))<0)
                return exit_status;
            sprintf(temp,"1000");
            strcat(temp,temp2);
            if((exit_status = fprintf(dz->fp,"%s\n",temp))<0)
                return exit_status;
        } else {
            for(k=0;k<peakcnt;k++) {
                if(as_midi || quantised) {
                    avfrq[k] = unchecked_hztomidi(avfrq[k]);
                    if(quantised) {
                        val = (int)round(avfrq[k] * 2.0);
                        avfrq[k] = (double)val/2.0;
                    }
                    if(!as_midi)
                        avfrq[k] = miditohz(avfrq[k]);
                }
                if(lose_amps) {
                    if(quantised && as_midi)
                        sprintf(temp,"%.1lf",avfrq[k]);
                    else
                        sprintf(temp,"%lf",avfrq[k]);
                } else {
                    if(quantised && as_midi)
                        sprintf(temp,"%.1lf\t%lf",avfrq[k],avamp[k] * ampratio);
                    else
                        sprintf(temp,"%lf\t%lf",avfrq[k],avamp[k] * ampratio);
                }
                if((exit_status = fprintf(dz->fp,"%s\n",temp))<0)
                    return exit_status;
            }
        }
    }
    return(FINISHED);
}
    
/****************************** LOCATE_PEAKS ***********************************
 *
 * Locate peaks in pvoc window
 */

 int locate_peaks(float *medianarray,int *chanarray,char *peakchan,float *peakfrq,float *peakamp,double frqratio,dataptr dz)
{   
    int peakcnt = 0;
    double lofrq = 0.0;
    double hifrq = dz->chwidth;
    double overlap_lofrq, overlap_hifrq;
    memset((char *)peakchan,0,dz->clength * sizeof(char));          //  Initialise peakchan array to NO peaks
    while(hifrq < dz->param[M_HIFRQ]) {                                 //  Find peaks in (next disjuct) subwindow
        if(lofrq < dz->param[M_LOFRQ]) {
            lofrq = hifrq;                                          //  Advance frqs to next disjunct window
            hifrq = lofrq * frqratio;
            continue;
        }
        peakcnt = locate_peaks_in_subwindow (lofrq,hifrq,medianarray,chanarray,peakchan,peakfrq,peakamp,peakcnt,dz);
        overlap_lofrq = ((lofrq + hifrq)/2.0);                      //  Set lofrq for overlapping window
        lofrq = hifrq;                                              //  Advance frqs to next disjunct window
        hifrq = lofrq * frqratio;
        overlap_hifrq = ((lofrq + hifrq)/2.0);                      //  Set hifrq for overlapping window
        if(overlap_hifrq < dz->nyquist)                             //  Do peak search in overlapping window
            peakcnt = locate_peaks_in_subwindow (overlap_lofrq,overlap_hifrq,medianarray,chanarray,peakchan,peakfrq,peakamp,peakcnt,dz);
    }
    if(peakcnt > 0) {
        eliminate_duplicated_frqs(peakfrq,peakamp,&peakcnt);
        sortfrqs(peakfrq,peakamp,peakcnt);
        if(dz->param[M_INTUNE] > 0.0)
            eliminate_harmonics(peakfrq,peakamp,&peakcnt,dz->param[M_INTUNE]);
    }
    return peakcnt;
}

/**************************** LOCATE_PEAKS_IN_SUBWINDOW *************************
 *
 *  Find peak within a frequency-rahe (subwindow) within the analysis-window.
 *  As search uses overlapping subwindows, ensure peak is not already stored before storing it.
 */

int locate_peaks_in_subwindow(double lofrq,double hifrq,float *medianarray,int *chanarray,char *peakchan,float * peakfrq,float * peakamp,int peakcnt,dataptr dz)
{
    int vc, cc, j, k;
    int medianarraycnt = 0;
    double medianval, peakfloor, minfrq = max(lofrq,dz->halfchwidth);   //  Eliminate frequencies too close to zero
    for( vc = 0,cc= 0; vc < dz->wanted; vc += 2,cc++) {
        if(dz->flbufptr[0][FREQ] >= minfrq) {
            if (dz->flbufptr[0][FREQ] < hifrq) {                //  Collect amplitude in all channels within window-range
                medianarray[medianarraycnt] = dz->flbufptr[0][AMPP];
                chanarray[medianarraycnt]   = cc;
                medianarraycnt++;
            } else
                break;
        }
    }
    sortamps(medianarray,chanarray,medianarraycnt);         //  Sort collected amps into ascending order
    j = medianarraycnt/2;
    if(ODD(medianarraycnt))                                 //  Find the median
        medianval = (medianarray[j] + medianarray[j+1])/2.0;
    else
        medianval = medianarray[medianarraycnt/2];
    peakfloor = medianval * dz->param[M_PEAKING];           //  Establish min level for a peak "peaking" is >= 1
    if(peakfloor > dz->param[M_AMPFLOOR]) {
        for(k=j; k < medianarraycnt;k++) {                  //  Locate peaks in this window
            if((medianarray[k] >= peakfloor) && (medianarray[k] > dz->param[M_AMPFLOOR])) { 
                cc = chanarray[k];
                vc = cc * 2;
                if(!peakchan[cc]) {                         //  If peak is not already marked as being in peak-list
                    peakchan[cc] = 1;                       //  Add it to the list
                    peakamp[peakcnt] = dz->flbufptr[0][AMPP];
                    peakfrq[peakcnt] = dz->flbufptr[0][FREQ];
                    peakcnt++;
                }
            }
            k++;
        }
    }
    return peakcnt;
}

/****************************** SORTAMPS ***********************************
 *
 * Sort channel amplitudes into ascending order, in order to find median
 */

void sortamps(float *medianarray,int * chanarray,int len)
{
    int m, n, chan;
    float temp;
    if(len<=1)
        return;
    for(n = 0;n < len-1;n++) {
        m = n+1;
        while(m < len) {
            if(medianarray[n] > medianarray[m]) {
                temp = medianarray[n];
                medianarray[n] = medianarray[m];
                medianarray[m] = temp;
                chan = chanarray[n];
                chanarray[n] = chanarray[m];
                chanarray[m] = chan;
            }
            m++;
        }
    }
}

/****************************** SORTFRQS ***********************************
 *
 * Sort peak frequencies into ascending order
 */

void sortfrqs(float *peakfrq,float *peakamp,int len)
{
    int m, n;
    float temp;
    if(len<=1)
        return;
    for(n = 0;n < len-1;n++) {
        m = n+1;
        while(m < len) {
            if(peakfrq[n] > peakfrq[m]) {
                temp = peakfrq[n];
                peakfrq[n] = peakfrq[m];
                peakfrq[m] = temp;
                temp = peakamp[n];
                peakamp[n] = peakamp[m];
                peakamp[m] = temp;
            }
            m++;
        }
    }
}

/****************************** ELIMINATE_DUPLICATED_FRQS ***********************************/

void eliminate_duplicated_frqs(float *peakfrq,float *peakamp,int *len)
{
    int m, n, j, len_less_one;
    if(*len<=1)
        return;
    len_less_one = *len - 1;
    for(n = 0;n < len_less_one;n++) {
        m = n+1;
        while(m < *len) {
            if(flteq(peakfrq[n],peakfrq[m])) {
                peakamp[n] = max(peakamp[n],peakamp[m]);    //  Keep loudest peak channel
                j = m+1;
                while(j < *len) {                           //  Eliminate duplicated peak, by shuflback
                    peakfrq[j-1] = peakfrq[j];
                    peakamp[j-1] = peakamp[j];
                    j++;
                }
                (*len)--;
                len_less_one--;
            } else
                m++;
        }
        n++;
    }
}           

/****************************** ELIMINATE_HARMONICS ***********************************/

void eliminate_harmonics(float *peakfrq,float *peakamp,int *len,double intune_ratio)
{
    int m, n, j, len_less_one;
    if(*len<=1)
        return;
    len_less_one = *len - 1;
    for(n = 0;n < len_less_one;n++) {
        m = n+1;
        while(m < *len) {
            if(is_a_harmonic(peakfrq[m],peakfrq[n],intune_ratio)) {
                j = m+1;
                while(j < *len) {                       //  Eliminate harmonic, by shuflback
                    peakfrq[j-1] = peakfrq[j];
                    peakamp[j-1] = peakamp[j];
                    j++;
                }
                (*len)--;
                len_less_one--;
            } else
                m++;
        }
    }
}

/**************************** IS_A_HARMONIC *************************/

int is_a_harmonic(double frq1,double frq2,double intune_ratio)
{
    double ratio;
    int   iratio;
    double intvl;

    ratio = frq1/frq2;
    iratio = round(ratio);

    if(ratio > iratio)
        intvl = ratio/(double)iratio;
    else
        intvl = (double)iratio/ratio;
    if(intvl > intune_ratio)
        return(FALSE);
    return(TRUE);
}

/**************************** STREAM_PEAKS *************************
 *
 *  If there are LESS peaks Than there are peak-streams.
 *
 *  if instream[k] = n
 *  peak k has been assigned to stream n
 *
 *  If there are MORE peaks Than there are peak-streams.
 *
 *  if instream[k] = n
 *  peak n has been assigned to stream k
 *
 *  Attempt to place found peaks within the pitch-streams whose (putative frqs) are defined by interpolation between
 *  last window with max number of peaks and
 *  next window with max number of peaks
 */

int stream_peaks(float *peakfrq,float *peakamp,float *maxpeakfrq,float *lastmaxpeakfrq,float *streamfrq,
                 int wcnt,int lastmax,int firstmax,int maxpeakcnt,int peakcnt,int *instream,
                 float *streampeakfrq,float *streampeakamp,dataptr dz)
{
    int *combo;
    int k, n, combo_cnt, morepeaksthanstreams;          //  combo_cnt is number of items (peaks) for which we must find
    double minratiosum = HUGE, thisfrq;                 //  all possible combinations..... e.g.
    int step, loc;                                      //  Either: Number of ways to assign 5 peaks to 8 streams
    double ratio;                                       //  Or: Number of ways to select 5 peaks from 8 peaks, to assign to 5 streams
    if(dz->mode ==1) {
        step = firstmax - lastmax;                      //  Window-count gap between previous and next windows having full (max) set of peaks.
        loc  = wcnt - lastmax;                          //  Window-count distance from previous full-peak-set to current window.
        ratio = (double)loc/(double)step;               //  Find proportion of the gap this represents.

        for(k=0;k < maxpeakcnt;k++) {
            thisfrq = maxpeakfrq[k] - lastmaxpeakfrq[k];
            thisfrq *= ratio;                       //  Establish where each stream frq "should" lie, by interpolation                      
            thisfrq += lastmaxpeakfrq[k];           //  between preceding and following windows containing MAX number of peaks
            streamfrq[k] = (float)thisfrq;
        }
    }                                               //  NB  for MODE2 streamfrqs are already set

    if (peakcnt < maxpeakcnt) {
        combo_cnt = peakcnt;                            //  e.g. 5(peakcnt) peaks are to be assigned to 8(maxpeakcnt) streams
        morepeaksthanstreams = 0;
    } else {                                            //  e.g. 5(maxpeakcnt) of 8 peaks are to be assigned to 5(peakcnt) streams
        combo_cnt = maxpeakcnt;
        morepeaksthanstreams = 1;                       //  In either case we need to find all ways to distrib 5 objects among 8 locations  
    }
    if((combo = (int *)malloc(combo_cnt * sizeof(int)))==NULL) {
        sprintf(errstr,"No memory for combinations array\n");
        return(MEMORY_ERROR);
    }
                                                        //  Assign peaks to streams in all possible combinations
    combinations(combo,0,0,maxpeakcnt - peakcnt,peakcnt,maxpeakcnt,&minratiosum,instream,peakfrq,maxpeakfrq,streamfrq,morepeaksthanstreams);
                                                        //  And calculate best fit - 
                                                        //  i.e. least deviation of frqs-of-peaks from frqs-of-streams to which they are assigned
    if(peakcnt < maxpeakcnt) {
        k = 0;                                          //  from the predicted frq of the peak-stream
        for(n=0;n<maxpeakcnt;n++)  {
            if(k >= peakcnt) {                          //  Once all active-peaks have been assigned to streams,
                streampeakfrq[n] = streamfrq[n];        //  force remaining streams to zero amp 
                streampeakamp[n] = 0.0f;
            } else if(instream[k] == n) {               //  If this active-peak(k) has been assigned to current stream(n)
                streampeakfrq[n] = peakfrq[k];          //  assign active-peak frq and amp to that stream                   
                streampeakamp[n] = peakamp[k];                  
                k++;                                    //  and get next active peak;
            } else {                                        
                streampeakfrq[n] = streamfrq[n];//  Otherwise, force stream to zero amp 
                streampeakamp[n] = 0.0f;
            }
        }
        for(n=0;n<maxpeakcnt;n++)  {
            peakfrq[n] = streampeakfrq[n];              //  Rewrite the complete frq and amp arrays for all peak streams
            peakamp[n] = streampeakamp[n];                  
        }
    } else {
        k = 0;                                          //  from the existing peak frqs
        for(n=0;n<peakcnt;n++)  {
            if(instream[k] == n) {                      //  If this peak(n) has been assigned to stream k
                streampeakfrq[k] = peakfrq[n];          //  assign peak frq and amp to that stream                  
                streampeakamp[k] = peakamp[n];                  
                if(++k >= maxpeakcnt)                   //  and get next stream
                    break;
            }
        }
        for(n=0;n<maxpeakcnt;n++)  {
            peakfrq[n] = streampeakfrq[n];              //  Rewrite the complete frq and amp arrays for all peak streams
            peakamp[n] = streampeakamp[n];                  
        }
    }
    return FINISHED;
}

/************************************ COMBINATIONS **************************************
 *
 * Find all possible ways of assigning peakcnt PEAKS to STREAMS
 *
 */

void combinations(int *combo,int k,int i, int j,int peakcnt,int maxpeakcnt,double *minratiosum,int *instream,
        float *peakfrq,float *maxpeakfrq,float *streamfrq,int morepeaksthanstreams)
{
    int n;
    if(k >= peakcnt) {
        if(morepeaksthanstreams)    //  If more peaks than streams
            find_best_fit(combo,maxpeakcnt,minratiosum,instream,maxpeakfrq,peakfrq);
        else                        //  If less peaks than streams
            find_best_fit(combo,peakcnt,minratiosum,instream,peakfrq,streamfrq);
        return;
    }
    for(n = i;n <= j;n++) {
        combo[k] = n;
        combinations(combo,k+1,n+1,j+1,peakcnt,maxpeakcnt,minratiosum,instream,peakfrq,maxpeakfrq,streamfrq,morepeaksthanstreams);
    }
}

/************************************ FIND_BEST_FIT_LESS **************************************
 *
 *  Here we try a smaller number of peaks in a larger number of streams
 *  
 *  Find deviation of actual peak frqs from predicted frqs of streams assigned.
 *  If it is lower than previous deviation, remember the assigned-streams.
 */

void find_best_fit(int *combo,int smallsetcnt,double *minratiosum,int *instream,float *smallset,float *largeset)
{
    int n = 0;
    double ratiosum = 0.0, ratio;
    while(n < smallsetcnt) {
        if(smallset[n] > largeset[combo[n]])            //  Compare peak frq with predited frequency of stream
            ratio = smallset[n]/largeset[combo[n]];     //  to which it is assigned in THIS combination
        else
            ratio = largeset[combo[n]]/smallset[n];
        ratiosum += ratio;                                      //  Sum frq ratios for all peaks
        n++;
    }
    if(ratiosum < *minratiosum) {                               //  Compare with existing minimum ratio sum
        *minratiosum = ratiosum;                                //  and if less (better fit)
        for(n=0;n<smallsetcnt;n++)                                  //  Remember how peaks are assigned to streams
            instream[n] = combo[n];
    }
}

/************************************ GET_INITIAL_DATA **************************************
 *
 *  Find loudesnt window in entire file: used to set AMPFLOOR.
 *  If output data is to be assigned to peak streams ..
 *     find maximum number of peaks in any of the windows, remembering the frq values in the first such window.
 */

int get_initial_data(float *maxwinamp,int *peakcnt,int *maxpeakcnt,int *firstmax,float *medianarray,int *chanarray,char *peakchan,
    float *peakfrq,float *peakamp,int *peaknos,float *maxpeakfrq,float *lastmaxpeakfrq,double frqratio,
    int bincnt,float *binlofrq,int *pitchbin,float *bintrof,float *streamfrq,dataptr dz)
{
    int wcnt = 0, samps_read, vc, k, j;
    double time = 0.0;
    int lastval;
    int gotmin = 0;
    int done;
    if(dz->mode > 0)
        fprintf(stdout,"INFO: Searching for maximum number of peaks, and maximum window amplitude.\n");
    else
        fprintf(stdout,"INFO: Searching for maximum window amplitude.\n");
    fflush(stdout);
    while((samps_read = fgetfbufEx(dz->bigfbuf,dz->buflen,dz->ifd[0],0)) > 0) {
        for( vc = 0; vc < dz->wanted; vc += 2) {
            if(dz->flbufptr[0][AMPP] > *maxwinamp)
                *maxwinamp = dz->flbufptr[0][AMPP];
        }
        *peakcnt = locate_peaks(medianarray,chanarray,peakchan,peakfrq,peakamp,frqratio,dz);
        switch(dz->mode) {
        case(1):        //  SET UP NORMAL STREAMING 
            peaknos[wcnt] = *peakcnt;           //  Store number of peaks in each window
            if(*peakcnt > *maxpeakcnt) {        
                *maxpeakcnt = *peakcnt;
                *firstmax = wcnt;
                for(k=0;k<*peakcnt;k++) {       //  Stores frqs of peaks in (1st) window with max no. of peaks
                    maxpeakfrq[k] = peakfrq[k];
                    lastmaxpeakfrq[k] = peakfrq[k];
                }
            }
            break;
        case(2):                //  DO STATISTICS ON PEAKS 
        case(3):
            for(k=0;k<*peakcnt;k++) {
                done = 0;
                for(j = 1; j < bincnt; j++) {
                    if(peakfrq[k] < binlofrq[j]) {
                        (pitchbin[j-1])++;
                        done = 1;
                        break;
                    }
                }
                if(!done)
                    (pitchbin[j-1])++;
            }
        }
        wcnt++;
        dz->total_samps_read += samps_read;
        time += dz->frametime;
        displaytime(time,dz);
    }
    if(samps_read < 0) {
        sprintf(errstr,"Failed to read samples after sample %d\n",dz->total_samps_read);
        return(SYSTEM_ERROR);
    }
    if(sndseekEx(dz->ifd[0],0,0)< 0) {
        sprintf(errstr,"Failed to seek to start of file, after initial read\n");
        return(SYSTEM_ERROR);
    }
    if(dz->mode == 2 || dz->mode == 3) {        //  Find peaks among semitonal pitch-bins
        j = 0;
        lastval = -1;
        gotmin = 0;
        for(k=0;k<bincnt;k++) {
            if(gotmin) {
                if(pitchbin[k] < lastval) {     //  PASSED OVER A PEAK
                    streamfrq[j] = (binlofrq[k-1] + binlofrq[k])/2;
                    j++;
                    gotmin = 0;
                }
            } else if(pitchbin[k] > lastval)
                gotmin = 1;

            lastval = pitchbin[k];
        }
        *maxpeakcnt = j;
    }
    dz->total_samps_read = 0;
    return FINISHED;
}

/************************************ GET_NEXT_WINDOW_WITH_MAX_NUMBER_OF_PEAKS **************************************
 *
 *  Seek to next window which has the max number of peaks, and remember these peaks.
 *  These peaks, and peaks in previous window with max number of peaks,
 *  are used to calculate the effective frq of the pitch-streams in any intervening windows
 *  which do not have the same number of peaks (MODE2 = internal mode 1).
 */

int get_next_window_with_max_number_of_peaks(int *peakcnt,int maxpeakcnt,float *lastmaxpeakfrq,float *maxpeakfrq,
        int *firstmax,int *lastmax,int wcnt,double frqratio,
        int *peaknos,float *medianarray,int *chanarray,char *peakchan,float *peakfrq,float *peakamp,dataptr dz)
{
    int k,j,samps_read;
    for(k=0;k<*peakcnt;k++)
        lastmaxpeakfrq[k] = maxpeakfrq[k];
    *lastmax = *firstmax;
    j = wcnt + 1;
    while(j < dz->wlength) {
        if(peaknos[j] == maxpeakcnt) {      //  Find next window that has all the maximum number of peaks
            if((sndseekEx(dz->ifd[0],j * dz->buflen,0)) < 0) {
                sprintf(errstr,"File seek to find next maxpeak window, failed.\n");
                return(SYSTEM_ERROR);
            }                               //  Get the peaks in that window
            if((samps_read = fgetfbufEx(dz->bigfbuf,dz->buflen,dz->ifd[0],0)) < 0) {
                sprintf(errstr,"Sample Read Failed After Seeking for next maximum peakcnt window\n");
                return(SYSTEM_ERROR);
            }                               //  Get the peaks in that window
            *peakcnt = locate_peaks(medianarray,chanarray,peakchan,peakfrq,peakamp,frqratio,dz);
            for(k=0;k<*peakcnt;k++)         //  Update the maxpeak array
                maxpeakfrq[k] = peakfrq[k];
            if(sndseekEx(dz->ifd[0],wcnt * dz->buflen,0)<0) {
                sprintf(errstr,"File seek AFTER finding next maxpeak window, failed.\n");
                return(SYSTEM_ERROR);
            }
            break;                          //  Seek back to original window position
        }
        j++;
    }                                       //  If no further window is found with max no. of peaks, maxpeakfrq does not change
    *firstmax = j;                          //  and processing continues to end of file
    return FINISHED;
}

/************************************ DISPLAYTIME **************************************/

void displaytime(double secs,dataptr dz)
{
    double float_time;
    int display_time,mins;
    if(sloom) {
        float_time = min(1.0,(double)dz->total_samps_read/(double)dz->insams[0]);
        display_time = round(float_time * PBAR_LENGTH);
        fprintf(stdout,"TIME: %d\n",display_time);
        fflush(stdout);
    } else {
        mins = (int)(secs/60.0);    /* TRUNCATE */
        secs -= (double)(mins * 60);
        fprintf(stdout,"\r%d min %5.2lf sec", mins, secs);
    }
}
