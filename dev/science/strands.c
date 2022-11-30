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



//  Attempted to integrate a "-b" flag that synthesizes the bands
//  One after another (rather trhan superimposed)
//  NEED TO TEST HIS WORKS (and test orig not broken)
//  This code saved on STICK
//  ORIG_CODE saved as strand_orig.c ***

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
#include <standalone.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>
#ifdef unix
#include <aaio.h>
#endif

#define STRAND_SRATE (44100)
#define STRAND_MAXWARP  (3) //  Max timewarp, takes 0-1 line and, at max(min) cubes (or cube-roots) values, 
                            //      to give concave or convex distortion of 0-1 value line.
#define WAVYFRAC    (0.4)   //  Boundary of band, during wavy-band-turbulence, can wander to max of 2/5 of bandwidth up or down.
#define TURBPOW     (0.5)   //  Random 0-1 LEVEL values, raised to this power, to weight them towards louder vals dominating
#define TURBFLATTEN (2.0)   //  As turbulence kicks in (in range 0 -> 1), any waviness of band boundaries flattens out.
                            //      Amplitude of waves is related to value of (1 - turbulence), (range 1 -> 0)
                            //      The straight line of this function is made concave by this power factor
                            //      So amplitude dips more quickly towards zero at start of any turbulence.

#define splicesize  rampbrksize
#define datasize    temp_sampsize
#define subbanded   is_mapping
#define derivs      is_transpos
#define rotmix      is_transpos
#define sintable    is_rectified
#define costable    specenv_type
#define sinptrs     zeroset
#define frqstores   fzeroset
#define tessitura   is_flat 
#define scaler      is_sharp
#define rotlevel    could_be_pitch
#define levderivs   could_be_transpos
#define flowdata    descriptor_samps
#define flowcnt     ringsize

#define twist dz->brk[STRAND_TWIST]

#define LAMINAR     0
#define TOTWIST     1
#define FROMTWIST   2
#define TWISTED     3

#define ROOT2       (1.4142136)
#define SIGNAL_TO_LEFT  (0)
#define SIGNAL_TO_RIGHT (1)

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int setup_strands_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int setup_strands_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int open_the_outfile(char *sfnam,dataptr dz);
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

static int strands_param_preprocess(double *pitchrange,double *band_separation,double *tessitura,int *isturb,int threadvalscnt,dataptr dz);
static int synthesis(dataptr dz);
//static void incr_sinptr(int n,double time,double onehzincr,dataptr dz);
//static double read_level(int n,double time,dataptr dz);
static int create_strands_sndbufs(dataptr dz);
static int spline(int streamno,int islevel,dataptr dz);
static double splint(int streamno,double time,int islevel,dataptr dz);
static int create_next_outfile(int no, char *sfnam, char *temp,dataptr dz);
static void scatter_streamdata_in_last_cycle(double time,int bandno,int threadscnt,int threadbas, int outcnt,double lastcycendtime,dataptr dz);
static double wavy(int bandno,double bandtop_mean,double tessitura,double amp,double *wavyfrq,double *tabpos,int *cycend,double scaling,dataptr dz);
static int establish_flow_types(dataptr dz);
static int get_flow_type(double time,double *transitstart,double *twistreffrq,double *transittime,dataptr dz) ;
static double turblevel(double turbulence,double thislevel,dataptr dz);
static int check_turbulence_values(int *isturb,dataptr dz);
static void assign_turbulence_pitches(int streamcnt,double pitchbot,double pitchrange,int *perm,double *turbpset,double *turbpitch);
static void pancalc(double position,double *leftgain,double *rightgain);

static double sinread(int iscos,double *tabpos,double frq,int *cycend,double scaling,dataptr dz);
static int strands(double pitchrange,double band_separation,double tessitura,int isturb,dataptr dz);
static int output_strands(char *sfnam,dataptr dz);
static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,int *threadvalscnt,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline, sfnam[400];
    int  cmdlinecnt, k, isturb = 0, threadvalscnt = 0;
    double pitchrange, band_separation, tessitura;
    aplptr ap;
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
        dz->maxmode = 3;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_strands_application(dz))<0) {
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
    dz->infile->channels = 2;
    dz->infile->srate = STRAND_SRATE;
    // parse_infile_and_hone_type() = 
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_strands_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // open_first_infile() : redundant
    // handle_extra_infiles() : redundant
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
    strcpy(sfnam,dz->outfilename);
    if(sloom) {
        k = strlen(sfnam);
        k--;
        *(sfnam + k) = ENDOFSTR;    //  (shorten generic filename by 1 cyaracter, discarding "0"
    }
    if(dz->mode == 2) {
        if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,&threadvalscnt,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(dz->iparam[STRAND_SEED] > 0) {
        srand((int)dz->iparam[STRAND_SEED]);
    }
//  check_param_validity_and_consistency()  redundant
    is_launched = TRUE;
    if(dz->mode == 1) {
        if((exit_status = create_strands_sndbufs(dz))<0) {                      // CDP LIB
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    if((exit_status = strands_param_preprocess(&pitchrange,&band_separation,&tessitura,&isturb,threadvalscnt,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(EVEN(dz->mode) && !sloom)
        strcat(dz->outfilename,"0.txt");
    if((exit_status = open_the_outfile(dz->outfilename,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // process_file =
    if((exit_status = strands(pitchrange,band_separation,tessitura,isturb,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(dz->mode == 1) {
        if((exit_status = synthesis(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    } else {
        if((exit_status = output_strands(sfnam,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
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
    char *filename = (*cmdline)[0], *p;
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
    p = filename;                   //  Drop file extension
    while(*p != ENDOFSTR) {
        if(*p == '.') {
            *p = ENDOFSTR;
            break;
        }
        p++;
    }
    strcpy(dz->outfilename,filename);
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(char *sfnam,dataptr dz)
{
    int exit_status;
    if(!sloom && dz->mode == 1)
        strcat(sfnam,"0");
    if((exit_status = create_sized_outfile(sfnam,dz))<0)
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

/************************* SETUP_STRANDS_APPLICATION *******************/

int setup_strands_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if(dz->mode == 2) {
        if((exit_status = set_param_data(ap,COUTHREADS,14,13,"di0dddDDDDDDDi"))<0)
            return(FAILED);
    } else {
        if((exit_status = set_param_data(ap,0,14,14,"diidddDDDDDDDi"))<0)
            return(FAILED);
    }
    if(dz->mode == 0 || dz->mode == 2) {
        if((exit_status = set_vflgs(ap,"gmf",3,"ddi","",0,0,""))<0)
            return(exit_status);
    } else {
        if((exit_status = set_vflgs(ap,"gmf",3,"ddi","s",1,0,"0"))<0)
            return(exit_status);
    }
    dz->input_data_type = NO_FILE_AT_ALL;
    switch(dz->mode) {
    case(0): 
    case(2): 
        dz->process_type    = TO_TEXTFILE;  
        dz->outfiletype     = TEXTFILE_OUT;
        break;
    case(1): 
        dz->process_type    = UNEQUAL_SNDFILE;  
        dz->outfiletype     = SNDFILE_OUT;
        break;
    }
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
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

/************************* SETUP_STRANDS_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_strands_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[STRAND_DUR]  = 0.0;
    ap->hi[STRAND_DUR]  = 32767.0;
    ap->default_val[STRAND_DUR] = 10.0;
    ap->lo[STRAND_BANDS] = 1;
    ap->hi[STRAND_BANDS] = 16;
    ap->default_val[STRAND_BANDS] = 4;
    if(dz->mode != 2) {
        ap->lo[STRAND_THRDS] = 2;   
        ap->hi[STRAND_THRDS] = 100;
        ap->default_val[STRAND_THRDS] = 3;
    }
    ap->lo[STRAND_TSTEP] = 1;
    ap->hi[STRAND_TSTEP] = 500;
    ap->default_val[STRAND_TSTEP] = 100;
    ap->lo[STRAND_BOT] = 0;
    ap->hi[STRAND_BOT] = 127;
    ap->default_val[STRAND_BOT] = 36;
    ap->lo[STRAND_TOP] = 0;
    ap->hi[STRAND_TOP] = 127;
    ap->default_val[STRAND_TOP] = 84;
    ap->lo[STRAND_TWIST] = 0;
    ap->hi[STRAND_TWIST] = 10;
    ap->default_val[STRAND_TWIST] = 3;
    ap->lo[STRAND_RAND] = 0;
    ap->hi[STRAND_RAND] = 1;
    ap->default_val[STRAND_RAND] = 0;
    ap->lo[STRAND_SCAT] = 0;
    ap->hi[STRAND_SCAT] = 1;
    ap->default_val[STRAND_SCAT] = 0;
    ap->lo[STRAND_VAMP] = 0;
    ap->hi[STRAND_VAMP] = 1;
    ap->default_val[STRAND_VAMP] = 0;
    ap->lo[STRAND_VMIN] = 0;
    ap->hi[STRAND_VMIN] = 4;
    ap->default_val[STRAND_VMIN] = 0;
    ap->lo[STRAND_VMAX] = 0;
    ap->hi[STRAND_VMAX] = 4;
    ap->default_val[STRAND_VMAX] = 0;
    ap->lo[STRAND_TURB] = 0;
    ap->hi[STRAND_TURB] = 2;
    ap->default_val[STRAND_TURB] = 0;
    ap->lo[STRAND_SEED] = 0;
    ap->hi[STRAND_SEED] = 256;
    ap->default_val[STRAND_SEED] = 0;
    ap->lo[STRAND_GAP] = 0;
    ap->hi[STRAND_GAP] = 12;
    ap->default_val[STRAND_GAP] = 0;
    ap->lo[STRAND_MINB] = 1;
    ap->hi[STRAND_MINB] = 24;
    ap->default_val[STRAND_MINB] = 12;
    ap->lo[STRAND_3D] = -1;
    ap->hi[STRAND_3D] = 1;
    ap->default_val[STRAND_3D] = 0;
    dz->maxmode = 3;
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
            if((exit_status = setup_strands_application(dz))<0)
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
    usage2("strands");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"strands"))             dz->process = STRANDS;
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
    if(!strcmp(str,"strands")) {
        fprintf(stderr,
        "USAGE: strands strands 1 generic_outdatafilename ...\n"
        "OR:    strands strands 2 outsndfilename ...\n"
        "dur bands threads tstep bot top twist rand scat vamp vmin vmax turb seed\n"
        "[-ggap] [-mminband] [-f3d] [-s]\n"
        "\n"
        "OR:    strands strands 3 generic_outdatafilename ...\n"
        "thrdsfile dur bands tstep bot top twist rand scat vamp vmin vmax turb seed\n"
        "[-ggap] [-mminband] [-f3d] [-s]\n"
        "\n"
        "Generate pitch-data for several streams (or the combined sound itself)\n"
        "where total pitch-tessitura divided into equal-sized pitch-BANDS\n"
        "and each band made of pitch-line THREADS, cycling around one-another\n"
        "pitchwise, within the band, IF twist strays from zero.\n"
        "Strands in adjacent bands spiral in opposite directions.\n"
        "\n"
        "DUR       Duration of output data or sound.\n"
        "BANDS     Number of bands in the output.\n"
        "THREADS   Number of sounds in a band.\n"
        "THRDSFILE Textfile listing number of sounds in EACH band.\n"
        "TSTEP     Timestep (in mS) between pitch-data vals in output.\n"
        "BOT       Lowest pitch of all output.\n"
        "TOP       Highest pitch of all output.\n"
        "TWIST     Rate of cycling of output pitches in bands (Hz).\n"
        "RAND      Random variation of twist frq from band to band.\n"
        "SCAT      Random-warp of cycles of threads in same band.\n"
        "VORTEX WAVINESS\n"
        "VAMP      ammount band-boundaries wander from fixed frqs.\n" 
        "VMIN      min rate of oscillation of band boundaries.\n"
        "VMAX      max rate of oscillation of band boundaries.\n"
        "TURBULENCE\n"
        "TURB      0  none: >0-to-1 turbulence in bands: >1-to-2 gobal turbulence.\n"
        "SEED      Seed value for randomisation settings.\n"
        "GAP       Minimum pitch-gap between bands.\n"
        "MINBAND   Minimum pitchwidth of any band.\n"
        "3D        Motion in 3D:  0 = none: 1 = bottom band up at front: -1 = down.\n"
        "\n"
        "-s        (Mode 2 only) - outputs each band in succesion.\n"
        "\n"
        "Press any key to see further information.\n");
        while(!kbhit())
            ;
        if(kbhit()) {
            fprintf(stderr,
            "\n"
            "TWIST &   With twist ZERO, streams do not cycle round one another\n"
            "LAMINAR   but stay at steady(ish) pitches .. (\"laminar flow\").\n"
            "FLOW      To force laminar flow, use 2 adjacent brkpnt vals of zero\n"
            "          (laminar flow is forced between the associated times).\n"
            "          To move to twisted flow, follow a zero val by a non-zero val.\n"
            "          To maintain twisted flow, follow non-zero val by non-zero val.\n"
            "          To untwist the flow, follow a non-zero val by a zero val.\n"
            "          \n"
            "          During laminar flow,\n"
            "          \"RAND\" controls pitch-jitter of laminae.\n"
            "          \"SCAT\" controls level-jitter of laminae.\n"
            "          \n"
            "3D        Outputs 2 extra param sets for each stream.\n"
            "          1st set can control loudness, or synth partial-cnt\n"
            "          in creation of a synthesized output.\n"
            "          2nd set can control a mix with reverb (etc) version,\n"
            "          (parameter is level of modified source in that mix)\n"
            "          both to simulate distance from the listener.\n"
            "          In modes 1 & 3, this extra data is output in separate files\n"
            "          AFTER all the pitchdata files have been output.\n"
            "          The output order is ...\n"
            "          all pitch data, all 1st controlfiles, all 2nd controlfiles.\n"
            "\n"
            "TURBULENT This can only happen (turb > 0.0) when twist > 0.0\n"
            "FLOW      (i.e. there can be no turbulence during laminar flow\n"
            "          or in the transitions to and from twisted flow).\n"
            "\n"
            "MODE 2 SOUND OUTPUT\n"
            "          Intended only as sound-test for the data output in mode 1.\n"
            "          Bands are output, equally spaced, across the stereo stage.\n"
            "\n");
        }
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/**************************** STRANDS_PARAM_PREPROCESS *************************
 *
 *  Arrays  0 to N-1  store the stream frq information at each timestep.
 *  Array   N stores the simetable.
 *  Array   N+1 store sintable-pointer values for each stream being synthesized.
 */

int strands_param_preprocess(double *pitchrange,double *band_separation,double *tessitura,int *isturb,int threadvalscnt,dataptr dz)
{
    int exit_status, synth_arrays = 0, rotation_arrays = 0, universal_arrays;
    int n;
    double srate, endsplice, gaps;
    double *sintab, *costab;

    //  Setup global params
    
    dz->param[STRAND_TSTEP] *= MS_TO_SECS;
    dz->datasize   = (int)ceil(dz->param[STRAND_DUR]/dz->param[STRAND_TSTEP]) + 1;  //  Size of data output depends on timestep used
    if(dz->mode == 2) {
        if(threadvalscnt != dz->iparam[STRAND_BANDS]) {
            sprintf(errstr,"NUMBER OF THREAD COUNTS in data file (%d) does not tally with band count (%d).\n",threadvalscnt,dz->iparam[STRAND_BANDS]);
            return(MEMORY_ERROR);
        }
        dz->itemcnt = 0;
        for(n=0;n<dz->iparam[STRAND_BANDS];n++)
            dz->itemcnt += dz->iparray[0][n];
    } else
        dz->itemcnt = dz->iparam[STRAND_THRDS] * dz->iparam[STRAND_BANDS];          //  No of threads in output
    dz->scalefact  = (double)SYNTH_TABSIZE/(double)STRAND_SRATE;                    //  Scales reading of sintab to sound-output samplerate
    dz->scaler     = (double)SYNTH_TABSIZE * dz->param[STRAND_TSTEP];               //  Scales reading of sintab to output-data "sample-rate"

    //  Setup effective bandwidths

    *pitchrange = dz->param[STRAND_TOP] - dz->param[STRAND_BOT];                    //  Total pitchrange used
    if(*pitchrange <= 0) {
        sprintf(errstr,"Invalid pitch range (<= 0.0).\n");
        return(DATA_ERROR);
    }
    *band_separation = *pitchrange/(double)dz->param[STRAND_BANDS]; //  Separation is distance between bands (prior to any gapping)
    gaps = dz->param[STRAND_GAP] * (dz->param[STRAND_BANDS] - 1);
    if(*pitchrange - gaps <= 0.0) {
        sprintf(errstr,"Pitch range  (%lf) incompatible with band gaps (totalling %lf).\n",*pitchrange,gaps);
        return(DATA_ERROR);
    }
    *tessitura  = *pitchrange - gaps;
    *tessitura /= dz->param[STRAND_BANDS];                      //  Tessitura is actual width of bands, after gapping
    if(*tessitura < dz->param[STRAND_MINB]) {                   //  (and before any wavy-banding distortion)
        if(gaps > 0.0) {
            sprintf(errstr,"Pitch range of a band (%lf) incompatible with min bandwidth and band gaps.\n",*tessitura);
            return(DATA_ERROR);
        } else {
            sprintf(errstr,"Pitch range of a band (%lf) incompatible with minimum bandwidth (%lf).\n",*tessitura,dz->param[STRAND_MINB]);
            return(DATA_ERROR);
        }
    }

    //  ESTABLISH ARRAYS AS FOLLOWS

    //  List of frqs, at specific times, in each strand             (dz->itemcnt)
    //  Sin table                                                   (1)
    //  Cos table                                                   (1)
    //  Pointers into sinetable for each strand                     (1)
    //  Table to store FLOW-TYPES                                   (1)

    universal_arrays = dz->itemcnt + 4;

    //  SYNTHESIS ALSO NEEDS....
    //  2nd-derivs of frqdata for each strand, for cubic splining   (dz->itemcnt)

    synth_arrays = dz->itemcnt;     

    //  If 3D, outputs level data
    //  List of levels, at specific times, in each strand           (dz->itemcnt)
    //  2nd-derivs of leveldata for each strand, for cubic splining (dz->itemcnt)

    rotation_arrays = dz->itemcnt * 2;
    
    if((dz->parray = (double **)malloc((universal_arrays+synth_arrays+rotation_arrays) * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for data storage.\n");
        return(MEMORY_ERROR);
    }

    //  Storage of stream pitch-data

    for(n=0;n<dz->itemcnt;n++) {
        if((dz->parray[n] = (double *)malloc((dz->datasize * 2) * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for stream-data %d storage and beyond.\n",n+1);
            return(MEMORY_ERROR);
        }
    }                                               //  pitches     AAAAAAA
    dz->sintable  = dz->itemcnt;                    //  +sintab     AAAAAAA B
    dz->costable  = dz->itemcnt + 1;                //  +sintab     AAAAAAA BC
    dz->sinptrs   = dz->itemcnt + 2;                //  +sinptrs    AAAAAAA BCD
    dz->flowdata  = dz->itemcnt + 3;                //  +flowtpye   AAAAAAA BCDE 
    dz->rotlevel  = dz->itemcnt + 4;                //  +rotlevel   AAAAAAA BCDE FFFFFFF
    dz->rotmix    = dz->rotlevel;   //  Used by different modes, (and for info only as variables defined as equivalent)
    dz->derivs    = dz->rotlevel + dz->itemcnt;     //  +levderivs  AAAAAAA BCDE FFFFFFF GGGGGGG
    dz->levderivs = dz->rotlevel + dz->itemcnt;     //  +levderivs  AAAAAAA BCDE FFFFFFF GGGGGGG HHHHHHH 

    //  Establish sinetable: fixed size
    if((dz->parray[dz->sintable] = (double *)malloc((SYNTH_TABSIZE +1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }
    sintab = dz->parray[dz->sintable];
    for(n=0;n<SYNTH_TABSIZE;n++)
        sintab[n] = sin(PI * 2.0 * ((double)n/(double)SYNTH_TABSIZE));
    sintab[n] = sintab[0];                          /* wrap around point */

    //  Establish inverted cosine table: fixed size
    if((dz->parray[dz->costable] = (double *)malloc((SYNTH_TABSIZE +1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for cosin table.\n");
        return(MEMORY_ERROR);
    }
    costab = dz->parray[dz->costable];
    for(n=0;n<SYNTH_TABSIZE;n++)
        costab[n] = -cos(PI * 2.0 * ((double)n/(double)SYNTH_TABSIZE));
    costab[n] = costab[0];                          /* wrap around point */

    //  Establish sine-table pointers: one for each stream
    if((dz->parray[dz->sinptrs] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for strand sintable pointers.\n");
        return(MEMORY_ERROR);
    }

    //  If streams rotate in radial direction, establish arrays to store relative-level-data, +-90 out of phase with pitch motion

    if(dz->iparam[STRAND_3D]) {
        for(n=0;n<dz->itemcnt;n++) {
            if((dz->parray[dz->rotlevel+n] = (double *)malloc((dz->datasize * 2) * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY for storing radial motion data %d and beyond.\n",n+1);
                return(MEMORY_ERROR);
            }
        }
    }

    //  If sound is to be synthesized

    if(dz->mode == 1) {
        //  establish 2nd derivative arrays, for cubic spline interpolation
        for(n=0;n<dz->itemcnt;n++) {
            if((dz->parray[dz->derivs+n] = (double *)malloc((dz->datasize * 2) * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY for storing stream-data cubic-spline slope %d and beyond.\n",n+1);
                return(MEMORY_ERROR);
            }
        }
        if(dz->iparam[STRAND_3D]) {
            for(n=0;n<dz->itemcnt;n++) {
                if((dz->parray[dz->levderivs+n] = (double *)malloc((dz->datasize * 2) * sizeof(double)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY for storing radial motion level cubic-spline slope %d and beyond.\n",n+1);
                    return(MEMORY_ERROR);
                }
            }
        }
        //  Establish output length in samples
        srate = (double)dz->infile->srate;
        dz->iparam[STRAND_DUR] = (int)round(dz->param[STRAND_DUR] * srate) * dz->infile->channels;

        //  Establish end splice length
        endsplice = 50.0 * MS_TO_SECS;                                      //  Go for big splice
        if(dz->param[STRAND_DUR] <= endsplice * 2)
            endsplice = min(dz->param[STRAND_DUR]/4.0,5.0 * MS_TO_SECS);    //  Else go for small splice
        dz->splicesize = (int)floor(endsplice * srate);                 //  Establish size of final splice
    } else {

        if(dz->iparam[STRAND_3D]) {
    //  2nd set of control data files, 180 or 0 out of phase with pitch motion

            for(n=0;n<dz->itemcnt;n++) {
                if((dz->parray[dz->rotmix+n] = (double *)malloc((dz->datasize * 2) * sizeof(double)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY for storing 2nd radial motion data %d and beyond.\n",n+1);
                    return(MEMORY_ERROR);
                }
            }
        }
    }
    //  Determine where flow is laminar, or banded, and where it transits between the two

    if((exit_status = establish_flow_types(dz))<0)
        return exit_status;

    //  Check turbulence parameter is consistent with physical model

    if((exit_status = check_turbulence_values(isturb,dz))<0)
        return exit_status;

    return(FINISHED);
}

/******************************** SYNTHESIS *********************************/

int synthesis(dataptr dz)
{
    int exit_status, n, m, cycend = 0, getlevel = 1, bandno, chans = dz->infile->channels;
    int inendsplice, instartsplice;
    int totaloutsamps, sampcnt = 0, startspliceend = dz->splicesize * chans, outsamps_generated, xxtempsize;
    int endsplicestart = dz->iparam[STRAND_DUR] - (dz->splicesize * chans), threadbas;
    double val, maxval = 1.0, srate = (double)dz->infile->srate, time = 0.0, spliceincr, spliceval, normaliser;
    double level, thispitch, thisfrq, pos = -1.0, posstep;
    float *obuf = dz->sampbuf[0];
    double *sinptr = dz->parray[dz->sinptrs], *llev, *rlev;

// NB This assumes data has been generated, in 1st dz->itemcnt parrays

    dz->tempsize = dz->iparam[STRAND_DUR];
    
    if((llev = (double *)malloc(dz->iparam[STRAND_BANDS] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing left levels of stereo output.\n");
        return(MEMORY_ERROR);
    }
    if((rlev = (double *)malloc(dz->iparam[STRAND_BANDS] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing right levels of stereo output.\n");
        return(MEMORY_ERROR);
    }
    posstep = 2.0/(double)(dz->iparam[STRAND_BANDS] - 1);
    bandno = 0;
    for(n= 0; n < dz->itemcnt;n++) {
        if((exit_status = spline(n,0,dz))<0)            //  Establish array of 2nd derivs for cubic spline interp of FRQ values
            return exit_status;
        if(n % dz->iparam[STRAND_THRDS] == 0) {         //  Establish stere-positioning levels for L+R chann outputs, for each stream
            pancalc(pos,&(llev[bandno]),&(rlev[bandno]));
            pos += posstep;
            bandno++;
        }
    }
    if(dz->iparam[STRAND_3D]) {
        for(n= 0; n< dz->itemcnt;n++) {
            if((exit_status = spline(n,getlevel,dz))<0) //  Establish array of 2nd derivs for cubic spline interp of LEVEL values
                return exit_status;
        }
    }
    spliceincr = 1.0/(double)dz->splicesize;
    spliceval = 0.0;
    instartsplice = 1;
    inendsplice = 0;
    totaloutsamps = dz->iparam[STRAND_DUR];
    fprintf(stdout,"INFO: First pass: assessing synthesis level.\n");
    fflush(stdout);
    memset((char *)obuf,0,dz->buflen * sizeof(float));
    for(n=0;n<dz->itemcnt;n++)                          //  Zero sine-table pointers for all streams
        sinptr[n] = 0.0;
    outsamps_generated = 0;
    if(dz->vflag[0]) {
        for(bandno = 0; bandno < dz->iparam[STRAND_BANDS];bandno++) {
            instartsplice = 1;
            inendsplice   = 0;
            fprintf(stdout,"INFO: Band %d\n",bandno+1);
            fflush(stdout);
            threadbas = bandno * dz->iparam[STRAND_THRDS];
            outsamps_generated = 0;
            while(outsamps_generated < totaloutsamps) {
                time = (double)outsamps_generated/chans/srate;
                for(m=0,n=threadbas;m<dz->iparam[STRAND_THRDS];n++,m++) {
                    thispitch = splint(n,time,0,dz);            //  Pitch read by cubic-splining between generated values in data output
                    thisfrq = miditohz(thispitch);
                    val = sinread(0,&(sinptr[n]),thisfrq,&cycend,dz->scalefact,dz); 
                                                                //  Use frq to read sinetable, using tabpointer for specific stream
                    if(dz->iparam[STRAND_3D]) {
                        level = splint(n,time,getlevel,dz);     //  Read corresponding level
                        val *= level;
                    }
                    obuf[sampcnt]   = (float)(obuf[sampcnt]   + (val * llev[bandno]));
                    obuf[sampcnt+1] = (float)(obuf[sampcnt+1] + (val * rlev[bandno]));
                }
                if(instartsplice) {
                    obuf[sampcnt]   = (float)(obuf[sampcnt]   * spliceval);
                    obuf[sampcnt+1] = (float)(obuf[sampcnt+1] * spliceval);
                    spliceval += spliceincr;
                    spliceval = min(spliceval,1.0);
                } else if(inendsplice) {
                    obuf[sampcnt]   = (float)(obuf[sampcnt]   * spliceval);
                    obuf[sampcnt+1] = (float)(obuf[sampcnt+1] * spliceval);
                    spliceval -= spliceincr;
                    spliceval = max(spliceval,0.0);
                }
                maxval = max(maxval,fabs(obuf[sampcnt]));
                maxval = max(maxval,fabs(obuf[sampcnt+1]));
                if((sampcnt+=2) >= dz->buflen) {
                    dz->process = GREV;
                    dz->total_samps_written = outsamps_generated;
                    xxtempsize = dz->tempsize;
                    dz->tempsize *= dz->iparam[STRAND_BANDS];
                    display_virtual_time(dz->total_samps_written,dz);
                    dz->tempsize = xxtempsize;
                    memset((char *)obuf,0,dz->buflen * sizeof(float));
                    sampcnt = 0;
                }
                outsamps_generated += 2;
                if(!inendsplice && (outsamps_generated >= endsplicestart)) {
                    inendsplice = 1;
                    spliceval = 1.0;
                } 
                if(instartsplice && (outsamps_generated >= startspliceend))
                    instartsplice = 0;
            }
            dz->process = GREV;
            dz->total_samps_written = outsamps_generated;
            display_virtual_time(dz->total_samps_written,dz);
            dz->process = STRANDS;
        }
    } else {
        while(outsamps_generated < totaloutsamps) {
            time = (double)outsamps_generated/chans/srate;
            for(n=0;n<dz->itemcnt;n++) {
                bandno = n/dz->iparam[STRAND_THRDS];
                thispitch = splint(n,time,0,dz);            //  Pitch read by cubic-splining between generated values in data output
                thisfrq = miditohz(thispitch);
                val = sinread(0,&(sinptr[n]),thisfrq,&cycend,dz->scalefact,dz); 
                                                            //  Use frq to read sinetable, using tabpointer for specific stream
                if(dz->iparam[STRAND_3D]) {
                    level = splint(n,time,getlevel,dz);     //  Read corresponding level
                    val *= level;
                }
                obuf[sampcnt]   = (float)(obuf[sampcnt]   + (val * llev[bandno]));
                obuf[sampcnt+1] = (float)(obuf[sampcnt+1] + (val * rlev[bandno]));
            }
            if(instartsplice) {
                obuf[sampcnt]   = (float)(obuf[sampcnt]   * spliceval);
                obuf[sampcnt+1] = (float)(obuf[sampcnt+1] * spliceval);
                spliceval += spliceincr;
                spliceval = min(spliceval,1.0);
            } else if(inendsplice) {
                obuf[sampcnt]   = (float)(obuf[sampcnt]   * spliceval);
                obuf[sampcnt+1] = (float)(obuf[sampcnt+1] * spliceval);
                spliceval -= spliceincr;
                spliceval = max(spliceval,0.0);
            }
            maxval = max(maxval,fabs(obuf[sampcnt]));
            maxval = max(maxval,fabs(obuf[sampcnt+1]));
            if((sampcnt+=2) >= dz->buflen) {
                dz->process = GREV;
                dz->total_samps_written = outsamps_generated;
                display_virtual_time(dz->total_samps_written,dz);
                dz->process = STRANDS;
                memset((char *)obuf,0,dz->buflen * sizeof(float));
                sampcnt = 0;
            }
            outsamps_generated += 2;
            if(!inendsplice && (outsamps_generated >= endsplicestart)) {
                inendsplice = 1;
                spliceval = 1.0;
            } 
            if(instartsplice && (outsamps_generated >= startspliceend))
                instartsplice = 0;
        }
        dz->process = GREV;
        dz->total_samps_written = outsamps_generated;
        display_virtual_time(dz->total_samps_written,dz);
        dz->process = STRANDS;
    }
    normaliser = 0.85/maxval;
    time = 0.0;
    spliceval = 0.0;
    instartsplice = 1;
    inendsplice = 0;
    dz->total_samps_written = 0;
    outsamps_generated = 0;
    sampcnt = 0;
    for(n=0;n<dz->itemcnt;n++)                          //  Zero sine-table pointers for all streams
        sinptr[n] = 0.0;
    fprintf(stdout,"INFO: Second pass: synthesis.\n");
    fflush(stdout);
    memset((char *)obuf,0,dz->buflen * sizeof(float));
    if(dz->vflag[0]) {
        for(bandno = 0; bandno < dz->iparam[STRAND_BANDS];bandno++) {
            instartsplice = 1;
            inendsplice   = 0;
            fprintf(stdout,"INFO: Band %d\n",bandno+1);
            fflush(stdout);
            threadbas = bandno * dz->iparam[STRAND_THRDS];
            outsamps_generated = 0;
            while(outsamps_generated < totaloutsamps) {
                time = (double)outsamps_generated/chans/srate;
                for(m=0,n=threadbas;m<dz->iparam[STRAND_THRDS];n++,m++) {
                    thispitch = splint(n,time,0,dz);                //  Frq read by cubic-splining between generated values in data output
                    thisfrq = miditohz(thispitch);
                    val = sinread(0,&(sinptr[n]),thisfrq,&cycend,dz->scalefact,dz); 
                                                                //  Use frq to read sinetable, using tabpointer for specific stream
                    if(dz->iparam[STRAND_3D]) {
                        level = splint(n,time,getlevel,dz);     //  Read corresponding level
                        val *= level;
                    }
                    obuf[sampcnt]   = (float)(obuf[sampcnt]   + (val * rlev[bandno]));
                    obuf[sampcnt+1] = (float)(obuf[sampcnt+1] + (val * llev[bandno]));
                    if(instartsplice) {
                        obuf[sampcnt]   = (float)(obuf[sampcnt]   * spliceval);
                        obuf[sampcnt+1] = (float)(obuf[sampcnt+1] * spliceval);
                        spliceval += spliceincr;
                        spliceval = min(spliceval,1.0);
                    } else if(inendsplice) {
                        obuf[sampcnt]   = (float)(obuf[sampcnt]   * spliceval);
                        obuf[sampcnt+1] = (float)(obuf[sampcnt+1] * spliceval);
                        spliceval -= spliceincr;
                        spliceval = max(spliceval,0.0);
                    }
                }
                outsamps_generated += 2;
                if(!inendsplice && (outsamps_generated >= endsplicestart)) {
                    inendsplice = 1;
                    spliceval = 1.0;
                } 
                if(instartsplice && (outsamps_generated >= startspliceend))
                    instartsplice = 0;
                if((sampcnt+=2) >= dz->buflen) {
                    dz->process = GREV;
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    dz->process = STRANDS;
                    sampcnt = 0;
                    memset((char *)obuf,0,dz->buflen * sizeof(float));
                }
            }
        }
    } else {
        while(outsamps_generated < totaloutsamps) {
            time = (double)outsamps_generated/chans/srate;
            for(n=0;n<dz->itemcnt;n++) {
                bandno = n/dz->iparam[STRAND_THRDS];
                thispitch = splint(n,time,0,dz);                //  Frq read by cubic-splining between generated values in data output
                thisfrq = miditohz(thispitch);
                val = sinread(0,&(sinptr[n]),thisfrq,&cycend,dz->scalefact,dz); 
                                                            //  Use frq to read sinetable, using tabpointer for specific stream
                if(dz->iparam[STRAND_3D]) {
                    level = splint(n,time,getlevel,dz);     //  Read corresponding level
                    val *= level;
                }
                obuf[sampcnt]   = (float)(obuf[sampcnt]   + (val * rlev[bandno]));
                obuf[sampcnt+1] = (float)(obuf[sampcnt+1] + (val * llev[bandno]));
            }
            obuf[sampcnt]   = (float)(obuf[sampcnt]   * normaliser);
            obuf[sampcnt+1] = (float)(obuf[sampcnt+1] * normaliser);
            if(instartsplice) {
                obuf[sampcnt]   = (float)(obuf[sampcnt]   * spliceval);
                obuf[sampcnt+1] = (float)(obuf[sampcnt+1] * spliceval);
                spliceval += spliceincr;
                spliceval = min(spliceval,1.0);
            } else if(inendsplice) {
                obuf[sampcnt]   = (float)(obuf[sampcnt]   * spliceval);
                obuf[sampcnt+1] = (float)(obuf[sampcnt+1] * spliceval);
                spliceval -= spliceincr;
                spliceval = max(spliceval,0.0);
            }
            outsamps_generated += 2;
            if(!inendsplice && (outsamps_generated >= endsplicestart)) {
                inendsplice = 1;
                spliceval = 1.0;
            } 
            if(instartsplice && (outsamps_generated >= startspliceend))
                instartsplice = 0;
            if((sampcnt+=2) >= dz->buflen) {
                dz->process = GREV;
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                dz->process = STRANDS;
                sampcnt = 0;
                memset((char *)obuf,0,dz->buflen * sizeof(float));
            }
        }
    }
    if(sampcnt) {
        dz->process = GREV;
        if((exit_status = write_samps(obuf,sampcnt,dz))<0)
            return(exit_status);
        dz->process = STRANDS;
    }
    return FINISHED;
}

/**************************** CREATE_STRANDS_SNDBUFS ****************************/

int create_strands_sndbufs(dataptr dz)
{
    int n;
    int bigbufsize;
    int framesize;
    framesize = F_SECSIZE;
    dz->bufcnt = 1;
    if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    bigbufsize = (int)Malloc(-1);
    bigbufsize /= dz->bufcnt;
    if(bigbufsize <=0)
        bigbufsize  = framesize * sizeof(float);

    dz->buflen = bigbufsize / sizeof(float);    
    dz->buflen = (dz->buflen / framesize)  * framesize;
    bigbufsize = dz->buflen * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize  * dz->bufcnt)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    for(n=0;n<dz->bufcnt;n++)
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);

    return(FINISHED);
}

/****************************** GET_THE_MODE_FROM_CMDLINE *********************************/

int get_the_mode_from_cmdline(char *str,dataptr dz)
{
    char temp[200], *p;
    if(sscanf(str,"%s",temp)!=1) {
        fprintf(stderr,"Cannot read mode of program.\n");
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

/**************************** SINREAD *************************/
    
double sinread(int iscos,double *tabpos,double frq,int *cycend,double scaling,dataptr dz)
{
    double tabincr, val, valdiff, timefrac;
    int lopos, hipos;
    double *sintab; 
    if(iscos)
        sintab = dz->parray[dz->costable]; 
    else
        sintab = dz->parray[dz->sintable]; 
    *cycend = 0;
    lopos = (int)floor(*tabpos);
    hipos = (int)ceil(*tabpos);
    timefrac = *tabpos - (double)lopos;
    val = sintab[lopos];
    valdiff = sintab[hipos] - val;
    val += valdiff * timefrac;
    tabincr = frq * scaling;
    *tabpos += tabincr;
    if(*tabpos >= SYNTH_TABSIZE) {
        *cycend = 1;
        *tabpos -= SYNTH_TABSIZE;
    }
    return val;
}

/*************************************** SPLINE *******************************
 *
 * Establish 2nd dervatives table for cubic spline calculations.
 */

int spline(int streamno,int islevel,dataptr dz) 
{
    double firstderivatzero = 0.0, firstderivatend = 0.0, qn, un, val1, val2;   // firstderiveatend is a guess
    double *datatab, *secondderiv, *u;
    int arraysize = dz->datasize;
    int n, t, v, tt, vv, k;
    double thistimestep, thisfrqstep, nexttimestep,nextfrqstep,bigtimestep, lasttime, lastval,sig,p;
    if((u = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for calculating spline derivatives.\n");
        return(MEMORY_ERROR);
    }
    if(islevel) {
        datatab = dz->parray[dz->rotlevel+streamno];    //  The stored level data already calcd for stream "streamno"
        secondderiv = dz->parray[dz->levderivs+streamno];
    } else {
        datatab = dz->parray[streamno];                 //  The stored frq data already calcd for stream "streamno"
        secondderiv = dz->parray[dz->derivs+streamno];
    }
    secondderiv[0] = -0.5;
    thistimestep = datatab[2] - datatab[0];
    thisfrqstep  = datatab[3] - datatab[1];
    val1 = 3.0/thistimestep;
    val2 = (thisfrqstep/thistimestep) - firstderivatzero;
    u[0] = val1 * val2;
    lasttime = datatab[0];
    lastval  = datatab[1];
    for(n = 1,t = 2, v = 3, tt = 4, vv = 5; n < arraysize - 1;n++,t+=2,v+=2,tt+=2,vv +=2) { //  t indexes current times in brktable, v indexes current values
        thisfrqstep  = datatab[v]  - lastval;                                               //  tt indexes NEXT time, vv indexes NEXT value
        nextfrqstep  = datatab[vv] - datatab[v];
        thistimestep = datatab[t]  - lasttime;
        nexttimestep = datatab[tt] - datatab[t];
        bigtimestep  = datatab[tt] - lasttime;
        sig = thistimestep/bigtimestep;
        p = (sig * secondderiv[n-1]) + 2.0;
        secondderiv[n] = (sig - 1.0)/p;
        u[n] = (nextfrqstep/nexttimestep) - (thisfrqstep/thistimestep);
        u[n] = (6.0 * u[n])/bigtimestep;
        u[n] -= sig * u[n-1];
        u[n] /= p;
        lasttime = datatab[t];
        lastval =  datatab[v];
    }
    qn = 0.5;
    thisfrqstep  = datatab[v]  - lastval;
    thistimestep = datatab[t]  - lasttime;
    val1 = 3.0/thistimestep;
    val2 = firstderivatend - (thisfrqstep/thistimestep);
    un = val1 * val2;
    val1 = un - (qn * u[n-1]);
    val2 = (qn * secondderiv[n-1]) + 1.0;
    secondderiv[n] = val1/val2;
    for(k = n-1;k >= 0;k--)
        secondderiv[k] = (secondderiv[k] * secondderiv[k+1]) + u[k];
    return FINISHED;
}

/*************************************** SPLINE_LEVEL *******************************
 *
 * Establish 2nd dervatives table for cubic spline calculations.
 */

int spline_level(int streamno,dataptr dz) 
{
    double firstderivatzero = 0.0, firstderivatend = 0.0, qn, un, val1, val2;   // firstderiveatend is a guess
    double *levtab = dz->parray[dz->rotlevel + streamno];   //  The stored frq data already calcd for stream "streamno"
    int arraysize = dz->datasize;
    double *secondderiv = dz->parray[dz->levderivs + streamno], *u;
    int n, t, v, tt, vv, k;
    double thistimestep, thisfrqstep, nexttimestep,nextfrqstep,bigtimestep, lasttime, lastval,sig,p;
    if((u = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for calculating spline derivatives.\n");
        return(MEMORY_ERROR);
    }
    secondderiv[0] = -0.5;
    thistimestep = levtab[2] - levtab[0];
    thisfrqstep  = levtab[3] - levtab[1];
    val1 = 3.0/thistimestep;
    val2 = (thisfrqstep/thistimestep) - firstderivatzero;
    u[0] = val1 * val2;
    lasttime = levtab[0];
    lastval  = levtab[1];
    for(n = 1,t = 2, v = 3, tt = 4, vv = 5; n < arraysize - 1;n++,t+=2,v+=2,tt+=2,vv +=2) { //  t indexes current times in brktable, v indexes current values
        thisfrqstep  = levtab[v]  - lastval;                                                //  tt indexes NEXT time, vv indexes NEXT value
        nextfrqstep  = levtab[vv] - levtab[v];
        thistimestep = levtab[t]  - lasttime;
        nexttimestep = levtab[tt] - levtab[t];
        bigtimestep  = levtab[tt] - lasttime;
        sig = thistimestep/bigtimestep;
        p = (sig * secondderiv[n-1]) + 2.0;
        secondderiv[n] = (sig - 1.0)/p;
        u[n] = (nextfrqstep/nexttimestep) - (thisfrqstep/thistimestep);
        u[n] = (6.0 * u[n])/bigtimestep;
        u[n] -= sig * u[n-1];
        u[n] /= p;
        lasttime = levtab[t];
        lastval =  levtab[v];
    }
    qn = 0.5;
    thisfrqstep  = levtab[v]  - lastval;
    thistimestep = levtab[t]  - lasttime;
    val1 = 3.0/thistimestep;
    val2 = firstderivatend - (thisfrqstep/thistimestep);
    un = val1 * val2;
    val1 = un - (qn * u[n-1]);
    val2 = (qn * secondderiv[n-1]) + 1.0;
    secondderiv[n] = val1/val2;
    for(k = n-1;k >= 0;k--)
        secondderiv[k] = (secondderiv[k] * secondderiv[k+1]) + u[k];
    return FINISHED;
}

/*************************************** SPLINT *******************************
 *
 * Do cubic spline, using 2nd dervatives table.
 */

double splint(int streamno,double time,int islevel,dataptr dz)
{
    double *datatab, *secondderiv;
    int arraysize = dz->datasize;
    int hitim, lotim, hival, loval; //  time and value indeces into brkpoint array
    int hideriv, loderiv;           //  upper and lower indices into 2nd derivatives array
    double timestep, hitimefrac, lotimefrac, val1, val2, val3;
    if(islevel) {
        datatab = dz->parray[dz->rotlevel+streamno];
        secondderiv = dz->parray[dz->levderivs+streamno];
    } else {
        datatab = dz->parray[streamno];
        secondderiv = dz->parray[dz->derivs+streamno];
    }
    if(time <= datatab[0])
        return datatab[1];
    hideriv = 0;
    hitim = 0;
    while(time > datatab[hitim]) {
        hitim+=2;
        hideriv++;
        if(hideriv >= arraysize)
            return datatab[(arraysize * 2)-1];  //  last value in table
    }
    loderiv = hideriv - 1;
    lotim = hitim - 2;
    loval = lotim+1;
    hival = hitim+1;
    timestep = datatab[hitim] - datatab[lotim];
    hitimefrac = (datatab[hitim] - time)/timestep;
    lotimefrac = (time - datatab[lotim])/timestep;
    
    val1  = hitimefrac * datatab[loval];
    val1 += lotimefrac * datatab[hival];

    val2  = (hitimefrac * hitimefrac * hitimefrac) - hitimefrac;
    val2 *= secondderiv[loderiv];

    val3  = (lotimefrac * lotimefrac * lotimefrac) - lotimefrac;
    val3 *= secondderiv[hideriv];

    val2 += val3;
    val2 *= (timestep * timestep)/6.0;
    val1 += val2;
    return val1;
}

/****************************** STRANDS ***********************************/

int strands(double pitchrange,double band_separation,double tessitura,int isturb,dataptr dz)
{
    int exit_status, n, t, flow_type, istwist = 0, *permlocal = NULL, *permglobal = NULL, rotup = dz->iparam[STRAND_3D], thth, threadscnt;
    int outcnt = 0, entrycnt;
    double time = 0.0;
    double *lastcycendtime, *bandtwist, *tabpos, *bandtopmean, *bandtop, *bandbot, *phasoffset, *flow;
    double *wtabpos, *wavyfrq, *lampitch, *turbpset = NULL, *localturbpitch = NULL, *globalturbpitch = NULL;
    double bandhalfgap, frqrange, frqmin, speedscat, squeeze, bandwidth, halfbandwidth, bandcentre, lasttabpos, thistabpos, thistabpos2, sinval;
    int *cycend, *wcycend, dummycycend, piover2phaseshift, piphaseshift, phasdist, th, bandno, threadbas, newcycle, iswavy, lastiswavy, *threadlens = NULL;
    double lamstep, halflamstep, pitchwander, level, twistreffrq = 0.0, gturb;
    double transittime = 0.0, transitfrac, laminarpitch, thisthreadcentre, thishalfbandwidth, thisturb;
    double upbandwidth, dnbandwidth, upbandlim, dnbandlim, xs, lamlevel, pitch, transitstart, vamp, thisbandbot;
    if(dz->mode == 2)
        threadlens = dz->iparray[0];                                            //  Time of end of previous twist-cycle
                                                                                //  Used when scattering stream-times within cycle
    if((lastcycendtime = (double *)malloc(dz->iparam[STRAND_BANDS] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing endtime of cycle in each band.\n");
        return(MEMORY_ERROR);
    }                                                                           //  Flag that band's twist cycle has been completed
    if((cycend = (int *)malloc(dz->iparam[STRAND_BANDS] * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing twist cycle completion flags.\n");
        return(MEMORY_ERROR);
    }
    for(bandno = 0;bandno < dz->iparam[STRAND_BANDS];bandno++) {                //  Initialise all cycle-end flags to 0 
        lastcycendtime[bandno] = 0.0;
        cycend[bandno] = 0;
    }                                                                           //  frq of each laminar thread
    if((lampitch = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing frqs of laminar threads.\n");
        return(MEMORY_ERROR);
    }                                                                           //  twisting rate of each band
    if((bandtwist = (double *)malloc(dz->iparam[STRAND_BANDS] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing twist rate of each band.\n");
        return(MEMORY_ERROR);
    }                                                                           //  Sintable read-position for each thread
    if((tabpos = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing sinetable read-position in each band.\n");
        return(MEMORY_ERROR);
    }                                                                           //  Sintable read-position for wavy band-boundaries
    if((wtabpos = (double *)malloc((dz->iparam[STRAND_BANDS] - 1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing Sintable read-position for wavy band-boundaries.\n");
        return(MEMORY_ERROR);
    }                                                                           //  wavy band-boundaries oscillation-frequency
    if((wcycend = (int*)malloc((dz->iparam[STRAND_BANDS] - 1) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing Sintable cycle end markers for wavy band-boundaries.\n");
        return(MEMORY_ERROR);
    }                                                                           //  wavy band-boundaries oscillation-frequency
    if((wavyfrq = (double *)malloc((dz->iparam[STRAND_BANDS] - 1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing wavy band-boundaries oscillation-frequency.\n");
        return(MEMORY_ERROR);
    }                                                                           // upper boundary pitches of each band (ignoring any gapping)
    if((bandtopmean = (double *)malloc(dz->iparam[STRAND_BANDS] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing upper boundary pitches of each band.\n");
        return(MEMORY_ERROR);
    }                                                                           // lower boundary pitches of each band (ignoring any gapping)
    if((bandtop = (double *)malloc(dz->iparam[STRAND_BANDS] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing lower boundary pitches of each band.\n");
        return(MEMORY_ERROR);
    }                                                                           //  Current bottom pitch of each band
    if((bandbot = (double *)malloc(dz->iparam[STRAND_BANDS] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing Current bottom pitch of each band.\n");
        return(MEMORY_ERROR);
    }                                                                           //  Phase offset of pointers defining streams within a band
    if((phasoffset = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing Phase offset of pointers defining streams within a band.\n");
        return(MEMORY_ERROR);
    }
    if(isturb) {
        if((turbpset = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for storing pre-assigned turbulence pitches.\n");
            return(MEMORY_ERROR);
        }
        if((localturbpitch = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for storing local turbulence pitches.\n");
            return(MEMORY_ERROR);
        }
        if((globalturbpitch = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for storing global turbulence pitches.\n");
            return(MEMORY_ERROR);
        }
        if((permlocal = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for storing local turbulence pitches.\n");
            return(MEMORY_ERROR);
        }
        if((permglobal = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for storing global turbulence pitches.\n");
            return(MEMORY_ERROR);
        }
        for(n=0;n < dz->itemcnt;n++) {
            permlocal[n] = n;
            permglobal[n] = n;
        }
    }

    flow = dz->parray[dz->flowdata];

    //  Establish phase-offset for 3-d motion : level mix is PI/2 out of phase with pitch-reading.

    piover2phaseshift = (int)round((double)SYNTH_TABSIZE/4.0);
    piphaseshift = (int)round((double)SYNTH_TABSIZE/2.0);

    thth = 0;
    if(dz->mode == 2) {
    //  Establish phase-offset of streams within bands
        for(n = 0;n < dz->iparam[STRAND_BANDS];n++) {
            threadscnt = threadlens[n];
            phasdist = (int)round((double)SYNTH_TABSIZE/(double)threadscnt);
            for(th = 0;th < threadscnt;th++) {
                phasoffset[thth] = th * phasdist;
                thth++;
            }
        }
    } else {
    //  Establish phase-offset of streams within bands
        phasdist = (int)round((double)SYNTH_TABSIZE/(double)dz->iparam[STRAND_THRDS]);
        for(n = 0;n < dz->iparam[STRAND_BANDS];n++) {
            for(th = 0;th < dz->iparam[STRAND_THRDS];th++) {
                phasoffset[thth] = th * phasdist;
                thth++;
            }
        }
    }
        //  Preset phase-of sinetable read for threads in each band
    for(th = 0;th < dz->itemcnt;th++)
        tabpos[th] = phasoffset[th];


    //  Get initial value of all time-varying variables
    time = 0;
    if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
        return exit_status;

    //  Intialise band boundaries (before any gapping)

    bandhalfgap = dz->param[STRAND_GAP]/2.0;
    bandtopmean[0] = dz->param[STRAND_BOT] + band_separation;   //  bandtopmean is generally in the gap between bands, 
    for(bandno=1; bandno < dz->iparam[STRAND_BANDS]-1;bandno++) //  or at the top and bottom limits of entire pitchrange
        bandtopmean[bandno] = bandtopmean[bandno-1] + band_separation;
    bandtopmean[bandno] = dz->param[STRAND_TOP];                //  bandtopmean is generally in the gap between bands, 

    //  Initialise actual band limits (after gapping)
    
    bandbot[0] = dz->param[STRAND_BOT];
    for(bandno=1; bandno < dz->iparam[STRAND_BANDS];bandno++)
        bandbot[bandno] = bandtopmean[bandno-1] + bandhalfgap;
    for(bandno=0; bandno < dz->iparam[STRAND_BANDS] - 1;bandno++)
        bandtop[bandno] = bandtopmean[bandno] - bandhalfgap;
    bandtop[bandno] = dz->param[STRAND_TOP];

    //  Preset all wavyfrequences, for waviness of band boundaries

    lastiswavy = 0;

    //  Initialise all frqs for any possible laminar flow

    lamstep = pitchrange/(double)(dz->itemcnt - 1);
    halflamstep = lamstep/2.0;
    lampitch[0] = dz->param[STRAND_BOT];
    for(th = 1;th < dz->itemcnt-1; th++)
        lampitch[th] = lampitch[th-1] + lamstep;
    lampitch[th] = dz->param[STRAND_TOP];

    //  Initialise all frqs for any possible twisted flow

    for(n=0,t=0;n < dz->flowcnt;n++,t+=4) {
        flow_type = (int)round(flow[t+1]);
        if(flow_type != LAMINAR) {      //  TOTWIST,and FROMTWIST (as well as TWISTED) both need bandtwist-frq values etc
            transitstart = flow[t];
            twistreffrq = flow[t+2];
            transittime = flow[t+3];
            istwist = 1;
            break;
        }
    }
    if(istwist) {
        for(bandno = 0;bandno < dz->iparam[STRAND_BANDS];bandno++) {
            bandtwist[bandno] = twistreffrq;            //  get twist-freq of band read from start of true twist
            if(dz->param[STRAND_RAND] > 0.0) {          //  If randomised (from one band to another): generate random scatter
                speedscat = drand48() * dz->param[STRAND_RAND];
                speedscat += 1.0;                       //  Allowing frq-scatter in range *1 to (max)*2
                if(drand48() < 0.5)                     //  and, at random, select N or 1/N (maximal range *1/2 to *2)  
                    speedscat = 1.0/speedscat;
                bandtwist[bandno] *= speedscat;         //  and set randomised value as true twist-frequency of band
            }
        }
    }
    flow_type = get_flow_type(0.0,&transitstart,&twistreffrq,&transittime,dz);

    while(outcnt < dz->datasize) {
        if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
            return exit_status;
        iswavy = 0;
        if(dz->param[STRAND_VAMP] > 0.0 && (dz->param[STRAND_VMIN] > 0.0 || dz->param[STRAND_VMAX] > 0.0))
            iswavy = 1;
        if(!lastiswavy && iswavy) {                     //  When waviness starts, set up cycle reading in sintable
            frqrange = fabs(dz->param[STRAND_VMAX] - dz->param[STRAND_VMIN]);
            frqmin = min(dz->param[STRAND_VMAX],dz->param[STRAND_VMIN]);
            for(bandno = 0; bandno < dz->iparam[STRAND_BANDS] - 1; bandno++) {
                wavyfrq[bandno] = drand48() * frqrange;
                wavyfrq[bandno] += frqmin;
                wcycend[bandno] = 0;
            }
        }
        lastiswavy = iswavy;

        switch(flow_type) {

        case(LAMINAR):

            for(th = 0;th < dz->itemcnt;th++) {
                pitch = lampitch[th];
                pitchwander = dz->param[STRAND_RAND] * halflamstep;
                if(th == 0)
                    pitchwander *= drand48();
                else if(th == dz->itemcnt - 1)
                    pitchwander *= -drand48();
                else
                    pitchwander *= (drand48() * 2.0) - 1.0;
                pitch += pitchwander;
                entrycnt = outcnt * 2;                      //  Save pitch vals generated
                dz->parray[th][entrycnt++] = time;
                dz->parray[th][entrycnt++] = pitch;             
                if(dz->iparam[STRAND_3D]) {
                    level = dz->param[STRAND_SCAT] * drand48();
                    level = 1.0 - level;
                    entrycnt = outcnt * 2;                  //  Save level vals generated
                    dz->parray[dz->rotlevel+th][entrycnt++] = time;
                    dz->parray[dz->rotlevel+th][entrycnt++] = level;                
                    entrycnt = outcnt * 2;                  //  Dummy values for reverb-etc mix 
                    dz->parray[dz->rotmix+th][entrycnt++] = time;
                    dz->parray[dz->rotmix+th][entrycnt++] = 0.0;                
                }
            }
            break;

        case(TWISTED):

            rotup = dz->iparam[STRAND_3D];
            
            //  IF TURBULENCE IS (APPROACHING) GLOBAL: calculate turbulence pitches over entire range, across all bands

            if(dz->param[STRAND_TURB] >= 1.0)
                assign_turbulence_pitches(dz->itemcnt,dz->param[STRAND_BOT],pitchrange,permglobal,turbpset,globalturbpitch);
                //  for each band in turn
            threadbas = 0;
            for(bandno = 0;bandno < dz->iparam[STRAND_BANDS]; bandno++) {
                if(dz->mode == 2)
                    threadscnt = threadlens[bandno];
                else {
                    threadscnt = dz->iparam[STRAND_THRDS];
                    threadbas = bandno * dz->iparam[STRAND_THRDS];  //  Get index of lowest thread in band
                }

                //  IF TURBULENCE IS IN-BAND OR GLOBAL: Calculate turbulence pitches WITHIN band

                if(dz->param[STRAND_TURB] > 0.0) {
                    if(bandno == 0)
                        thisbandbot = dz->param[STRAND_BOT];
                    else
                        thisbandbot = bandtopmean[bandno-1];
                    assign_turbulence_pitches(threadscnt,thisbandbot,tessitura,permlocal + threadbas,turbpset,localturbpitch);
                }
                
                //  If at end of a twist cylce for the band, scatter previous cycle times, and recalculate twist frequency

                newcycle = 0;
                if(cycend[bandno]) {                            //  If band twist-cycle reaches sintab end
                    if(dz->param[STRAND_SCAT] > 0.0)            //  Scatter data in this band, over last cycle
                        scatter_streamdata_in_last_cycle(time,bandno,threadscnt,threadbas,outcnt,lastcycendtime[bandno],dz);
                    lastcycendtime[bandno] = time;              //  Note start of next cycle
                    cycend[bandno] = 0;                         //  Unmark "end of cycle"
                    newcycle = 1;
                    bandtwist[bandno] = dz->param[STRAND_TWIST];    //  get twist-freq of band read from input data

                    if(dz->param[STRAND_RAND] > 0.0) {          //  If randomised (from one band to another): generate random scatter
                        speedscat = drand48() * dz->param[STRAND_RAND];
                        speedscat += 1.0;                       //  Allowing frq-scatter in range *1 to (max)*2
                        if(drand48() < 0.5)                     //  and, at random, select N or 1/N (maximal range *1/2 to *2)  
                            speedscat = 1.0/speedscat;
                        bandtwist[bandno] *= speedscat;         //  and set randomised value as true twist-frequency of band
                    }
                }

                if(bandno < dz->iparam[STRAND_BANDS] - 1) {
                //  Use warp factors to get any wavy-variation of top boundary for current band (except for top of top band which is fixed)

                    if(iswavy && dz->param[STRAND_TURB] < 1.0) {    //  If Turbulence >= 1, no longer ness to calc wavy-boundaries
                        vamp = dz->param[STRAND_VAMP];                                  //  Amplitude of waviness decreases as turbulence increases (0->1)

                        if(dz->param[STRAND_TURB] > 0.0)                                //  The factor (1 - turbulence), with values (1->0) is used.
                            vamp *= pow((1 - dz->param[STRAND_TURB]),TURBFLATTEN);      //  TURBFLATTEN makes curve dip towards zero more quickly at start.
                        bandtop[bandno] = wavy(bandno,bandtopmean[bandno],tessitura,vamp,&(wavyfrq[bandno]),&(wtabpos[bandno]),&(wcycend[bandno]),dz->scaler,dz);
                    //  Use halfgap to force band separation 
                        bandtop[bandno] -= bandhalfgap;
                    } else
                        bandtop[bandno] = bandtopmean[bandno] - bandhalfgap;
                }                                                       //  Use previously calcd value of top of next lowest band, 
                if(bandno > 0)                                          //  to find bottom boundary of current band.
                    bandbot[bandno] = bandtop[bandno - 1] + dz->param[STRAND_GAP]; //   using gap to force band separation 
                //  Use band minimum width to modify values, if ness

                bandwidth = bandtop[bandno] - bandbot[bandno];
                if((squeeze = dz->param[STRAND_MINB] - bandwidth) > 0) {
                    bandtop[bandno] += squeeze;
                    bandwidth = bandtop[bandno] - bandbot[bandno];
                }
                halfbandwidth = bandwidth/2.0;
                bandcentre = bandbot[bandno] + halfbandwidth;

                //  SAFETY: recalibrate the phase-offsets for the strans
                if(newcycle) {
                    for(th=1;th<threadscnt;th++) {
                        tabpos[threadbas + th] = tabpos[threadbas] + phasoffset[threadbas + th];
                        if(tabpos[threadbas + th] >= SYNTH_TABSIZE)
                            tabpos[threadbas + th] -= (double)SYNTH_TABSIZE;
                    }
                    newcycle = 0;
                }
                thisturb = dz->param[STRAND_TURB];

                for(th=0;th<threadscnt;th++) {
                    lasttabpos = tabpos[threadbas+th];          //  Remember current table location

                    if(thisturb == 2.0)                         //  TOTAL GLOBAL TURBULENCE 
                        pitch = globalturbpitch[threadbas+th];  //  use global turbulence pitch

                    else if(thisturb >= 1.0)                    //  TOTAL INBAND TURBULENCE or beyond
                        pitch = localturbpitch[threadbas+th];   //  use in-band turbulent pitch

                    else {  // turb < 1.0                       //  ELSE: (some degree of) TWISTING : Read sintable at correct location
                        if(th==0)                               //  ONLY lowest thread used to count cycles
                            sinval = sinread(1,&(tabpos[threadbas]),bandtwist[bandno],&(cycend[bandno]),dz->scaler,dz); 
                        else
                            sinval = sinread(1,&(tabpos[threadbas+th]),bandtwist[bandno],&dummycycend,dz->scaler,dz);
                        pitch = sinval * halfbandwidth;         //  Expand -1-to +1 range to fit into bandwidth
                        pitch += bandcentre;
                    }
                    if(thisturb > 0.0) {            //  If turbulent                            
                        if(thisturb < 1.0)          //  MOVING TOWARDS INBAND TURBULENCE :  
                                                    //  pitch is from sinval : interpolate towards in-band turbulent pitch
                            pitch = ((1.0 - thisturb) * pitch) + (thisturb * localturbpitch[threadbas+th]);

                        else if(thisturb < 2.0) {   // thisturb >= 1.0

                            gturb = thisturb - 1.0; //  MOVING TOWARDS GLOBAL TURBULENCE : 
                                                    //  pitch is from localturbpitch : interpolate towards global turbulent pitch
                            pitch = ((1.0 - gturb) * pitch) + (gturb * globalturbpitch[threadbas+th]);
                        }
                    }

                    entrycnt = outcnt * 2;                      //  Save pitch vals generated
                    dz->parray[threadbas + th][entrycnt++] = time;
                    dz->parray[threadbas + th][entrycnt++] = pitch;
                    if(rotup) {                             //  If band is to turn in radial direction
                    
                        switch(rotup) {         //  generate phase-shifted data for mixing filtered copy
                        case(1):                        
                            thistabpos =  lasttabpos + piover2phaseshift;   //  Rotating UP at the front
                            while(thistabpos >= SYNTH_TABSIZE)
                                thistabpos  -= (double)SYNTH_TABSIZE;
                            thistabpos2 =  lasttabpos + piphaseshift;
                            while(thistabpos2 >= SYNTH_TABSIZE)
                                thistabpos2 -= (double)SYNTH_TABSIZE;
                            break;
                        case(-1):                                           //  Rotating DOWN at the front
                            thistabpos  = lasttabpos - piover2phaseshift;
                            while(thistabpos < 0)
                                thistabpos  += (double)SYNTH_TABSIZE;
                            thistabpos2 = lasttabpos;
                            break;
                        }
                        sinval = sinread(1,&thistabpos,bandtwist[bandno],&dummycycend,dz->scaler,dz);
                        level = sinval + 1.0;                   //  Scale into 0 to 1 range
                        level /= 2.0;

                        if(dz->param[STRAND_TURB] > 0.0)
                            level = turblevel(dz->param[STRAND_TURB],level,dz);
                        entrycnt = outcnt * 2;                  //  Save mixdata values
                        dz->parray[dz->rotlevel + threadbas + th][entrycnt++] = time;
                        dz->parray[dz->rotlevel + threadbas + th][entrycnt++] = level;

                        sinval = sinread(1,&thistabpos2,bandtwist[bandno],&dummycycend,dz->scaler,dz);
                        level = sinval + 1.0;                   //  Scale into 0 to 1 range
                        level /= 2.0;

                        if(dz->param[STRAND_TURB] > 0.0)                //  THIS IS ILLOGICAL, BUT perhaps OK for turbulent flow!!
                            level = turblevel(dz->param[STRAND_TURB],level,dz);
                        entrycnt = outcnt * 2;                  //  Save mixdata values
                        dz->parray[dz->rotmix + threadbas + th][entrycnt++] = time;
                        dz->parray[dz->rotmix + threadbas + th][entrycnt++] = level;
                    }
                }
                rotup = -rotup;     //  If rotating, alternate bands rotate in opposite directions
                if(dz->mode == 2)
                    threadbas += threadscnt;
            }
            break;

        case(TOTWIST):
        case(FROMTWIST):

            rotup = dz->iparam[STRAND_3D];
            
            transitfrac = (time - transitstart)/transittime;    //  Fractional position in transit to full twisting

            //  THE FOLLOWING CODE IS DONE AS IF WE'RE ALREADY IN A FULL TWIST
            
            threadbas = 0;
            for(bandno = 0;bandno < dz->iparam[STRAND_BANDS]; bandno++) {

                
                if(dz->mode == 2)
                    threadscnt = threadlens[bandno];
                else {
                    threadscnt = dz->iparam[STRAND_THRDS];
                    threadbas = bandno * dz->iparam[STRAND_THRDS];
                }
                newcycle = 0;
                if(cycend[bandno]) {
                    if(dz->param[STRAND_SCAT] > 0.0)
                        scatter_streamdata_in_last_cycle(time,bandno,threadscnt,threadbas,outcnt,lastcycendtime[bandno],dz);
                    lastcycendtime[bandno] = time;
                    cycend[bandno] = 0;
                    newcycle = 1;
                    bandtwist[bandno] = twistreffrq;

                    if(dz->param[STRAND_RAND] > 0.0) {
                        speedscat = drand48() * dz->param[STRAND_RAND];
                        speedscat += 1.0;
                        if(drand48() < 0.5)
                            speedscat = 1.0/speedscat;
                        bandtwist[bandno] *= speedscat;
                    }
                }

                //  WAVY BAND BOUNDARIES NOT PERMITTED IN TRANSITION TO TWIST

                if(bandno < dz->iparam[STRAND_BANDS] - 1)
                    bandtop[bandno] = bandtopmean[bandno] - bandhalfgap;
                if(bandno > 0)
                    bandbot[bandno] = bandtop[bandno - 1] + bandhalfgap;

                bandwidth = bandtop[bandno] - bandbot[bandno];
                if((squeeze = dz->param[STRAND_MINB] - bandwidth) > 0) {
                    bandtop[bandno] += squeeze;
                    bandwidth = bandtop[bandno] - bandbot[bandno];
                }
                halfbandwidth = bandwidth/2.0;
                bandcentre = bandbot[bandno] + halfbandwidth;

                if(newcycle) {
                    for(th=1;th<threadscnt;th++) {
                        tabpos[threadbas + th] = tabpos[threadbas] + phasoffset[threadbas + th];
                        if(tabpos[threadbas + th] >= SYNTH_TABSIZE)
                            tabpos[threadbas + th] -= (double)SYNTH_TABSIZE;
                    }
                }

                //  WE NOW INTEGRATE THE LAMINAR AND THE TWIST INFORMATION
                
                if(flow_type == TOTWIST)
                    bandwidth *= transitfrac;           //  Twist bandwidth increases from zero to full as we do transit to twist
                else    
                    bandwidth *= (1.0 - transitfrac);   //  Twist bandwidth decreases from full to zero as we do transit from twist

                //  CALCUALTE POSITION IF IT WERE LAMINAR

                for(th=0;th<threadscnt;th++) {
                    laminarpitch = lampitch[th+threadbas];
                    pitchwander = dz->param[STRAND_RAND] * halflamstep;
                    if(th+threadbas == 0)
                        pitchwander *= drand48();
                    else if(th+threadbas == dz->itemcnt - 1)
                        pitchwander *= -drand48();
                    else
                        pitchwander *= (drand48() * 2.0) - 1.0;
                    laminarpitch += pitchwander;

                    //  INTERPOLATE  btween laminar position and band-centre of twisting-threads 

                    if(flow_type == TOTWIST) {                      //  as we approach true twist
                        thisthreadcentre = (bandcentre - laminarpitch) * transitfrac;
                        thisthreadcentre += laminarpitch;
                    } else {                                        //  as we approach laminar
                        thisthreadcentre = (laminarpitch - bandcentre) * transitfrac;
                        thisthreadcentre += bandcentre;
                    }

                    //  Find upper/lower limits of stream rotating around current centre (at it expands/contracts to/from full bandwidth)

                    thishalfbandwidth = bandwidth/2.0;
                    upbandwidth = thishalfbandwidth;
                    dnbandwidth = thishalfbandwidth;
                    upbandlim = thisthreadcentre + thishalfbandwidth;
                    dnbandlim = thisthreadcentre - thishalfbandwidth;

                    //  Check total limits of band range are not exceeded

                    if((xs = upbandlim - bandtop[bandno]) > 0) {
                        upbandwidth -= xs;
                        dnbandwidth += xs;
                    } else if((xs = bandbot[bandno] - dnbandlim) > 0) {
                        dnbandwidth -= xs;
                        upbandwidth += xs;
                    }

                    //  CALCULATE POSITION IN A TWIST
                    
                    lasttabpos = tabpos[threadbas+th];
                    if(th==0)
                        sinval = sinread(1,&(tabpos[threadbas]),bandtwist[bandno],&(cycend[bandno]),dz->scaler,dz); 
                    else
                        sinval = sinread(1,&(tabpos[threadbas+th]),bandtwist[bandno],&dummycycend,dz->scaler,dz);

                    //  SCALE TWIST POSITION INTO THE (narrowed) RANGE AROUND THE (moving) CENTRE
                    
                    if(sinval >= 0.0) {
                        pitch = sinval * upbandwidth;           //  Expand -1-to +1 range to fit into current halfbandwidth
                        pitch += thisthreadcentre;
                    } else {
                        pitch = sinval * dnbandwidth;           //  NB sinval is initially -ve, 
                        pitch += thisthreadcentre;              //  so adding it to bandcentre, takes it below bandcentre
                    }                   
                    entrycnt = outcnt * 2;
                    dz->parray[threadbas + th][entrycnt++] = time;
                    dz->parray[threadbas + th][entrycnt++] = pitch;

                    if(rotup) {

                        // calculate level for laminar flow

                        lamlevel = dz->param[STRAND_SCAT] * drand48();
                        lamlevel = 1.0 - lamlevel;

                        // calculate level for twisted flow
                        
                        switch(rotup) {
                        case(1):                        
                            thistabpos =  lasttabpos + piover2phaseshift;   //  Rotating UP at the front
                            while(thistabpos >= SYNTH_TABSIZE)
                                thistabpos  -= (double)SYNTH_TABSIZE;
                            thistabpos2 =  lasttabpos + piphaseshift;
                            while(thistabpos2 >= SYNTH_TABSIZE)
                                thistabpos2 -= (double)SYNTH_TABSIZE;
                            break;
                        case(-1):                                           //  Rotating DOWN at the front
                            thistabpos  = lasttabpos - piover2phaseshift;
                            while(thistabpos < 0)
                                thistabpos  += (double)SYNTH_TABSIZE;
                            thistabpos2 = lasttabpos;
                            break;
                        }
                        sinval = sinread(1,&thistabpos,bandtwist[bandno],&dummycycend,dz->scaler,dz);
                        level = sinval + 1.0;
                        level /= 2.0;

                        //  Interpolate the two levels, according to 

                        if(flow_type == TOTWIST)                //  how far we are into transit to full twist
                            level = (level * transitfrac) + (lamlevel * (1.0 - transitfrac));
                        else                                    //  how far we are into transit to full laminar
                            level = (level * (1.0 - transitfrac)) + (lamlevel * transitfrac);
                        entrycnt = outcnt * 2;                  //  Save mixdata values
                        dz->parray[dz->rotlevel + threadbas + th][entrycnt++] = time;
                        dz->parray[dz->rotlevel + threadbas + th][entrycnt++] = level;

                        sinval = sinread(1,&thistabpos2,bandtwist[bandno],&dummycycend,dz->scaler,dz);
                        level = sinval + 1.0;
                        level /= 2.0;

                        //  Interpolate the two mixlevels, according to 

                        if(flow_type == TOTWIST)                //  how far we are into transit to full twist
                            level = level * transitfrac;
                        else                                    //  how far we are into transit to full laminar
                            level = level * (1.0 - transitfrac);
                        entrycnt = outcnt * 2;                  //  Save mixdata values
                        dz->parray[dz->rotmix + threadbas + th][entrycnt++] = time;
                        dz->parray[dz->rotmix + threadbas + th][entrycnt++] = level;
                    }
                }
                rotup = -rotup;
                if(dz->mode == 2)
                    threadbas += threadscnt;
            }
            break;
        }
        outcnt++;
        time = outcnt * dz->param[STRAND_TSTEP];
        flow_type = get_flow_type(time,&transitstart,&twistreffrq,&transittime,dz);
    }
    return FINISHED;
}

/****************************** OUTPUT_STRANDS ***********************************/

int output_strands(char *sfnam,dataptr dz)
{
    int exit_status, n, m, j, k;
    char temp[400];
    double *stream;
    dz->tempsize = dz->itemcnt;
                        //  1st data file already open
    fprintf(stdout,"INFO: Outputting pitch-threads 1 to %d.\n",dz->itemcnt);
    fflush(stdout);
    for(n=0;n < dz->itemcnt;n++) {
        if(sloom) {     //  Send valid message to progress-bar
            dz->process_type = SCREEN_MESSAGE;      
            dz->process      = FIND_PANPOS;
            display_virtual_time(n+1,dz);
            dz->process_type = TO_TEXTFILE;     
            dz->process      = STRANDS;
        }
        if(n > 0) {     //  Current datafile closed as next is opened
            if((exit_status = create_next_outfile(n,sfnam,temp,dz))<0)
                return exit_status;
        }
        stream = dz->parray[n];
        for(m = 0, j= 0; m < dz->datasize; m++,j+=2) {
            sprintf(temp,"%.16lf\t%lf\n",stream[j],stream[j+1]);
            if(fputs(temp,dz->fp) < 0) {
                fclose(dz->fp);
                sprintf(errstr,"CANNOT WRITE PITCH DATA TO FILE %s\n",dz->outfilename);
                return SYSTEM_ERROR;
            }
        }
    }                   
    if(dz->iparam[STRAND_3D]) {
        fprintf(stdout,"INFO: Outputting 1st set of control threads %d to %d.\n",dz->itemcnt + 1,dz->itemcnt * 2);
        fflush(stdout);
        for(k = 0,n=dz->itemcnt;k < dz->itemcnt;n++,k++) {
            if(sloom) {     //  Send valid message to progress-bar
                dz->process_type = SCREEN_MESSAGE;      
                dz->process      = FIND_PANPOS;
                display_virtual_time(k+1,dz);
                dz->process_type = TO_TEXTFILE;     
                dz->process      = STRANDS;
            }
            if((exit_status = create_next_outfile(n,sfnam,temp,dz))<0)
                return exit_status;
            stream = dz->parray[dz->rotlevel + k];
            for(m = 0, j= 0; m < dz->datasize; m++,j+=2) {
                sprintf(temp,"%.16lf\t%lf\n",stream[j],stream[j+1]);
                if(fputs(temp,dz->fp) < 0) {
                    fclose(dz->fp);
                    sprintf(errstr,"CANNOT WRITE ROTATION-CONTROL1 DATA TO FILE %s\n",dz->outfilename);
                    return SYSTEM_ERROR;
                }
            }
        }                   
        fprintf(stdout,"INFO: Outputting 2nd set of control threads %d to %d.\n",(dz->itemcnt*2) + 1,dz->itemcnt * 3);
        fflush(stdout);
        for(k = 0,n=dz->itemcnt*2;k < dz->itemcnt;n++,k++) {
            if(sloom) {     //  Send valid message to progress-bar
                dz->process_type = SCREEN_MESSAGE;      
                dz->process      = FIND_PANPOS;
                display_virtual_time(k+1,dz);
                dz->process_type = TO_TEXTFILE;     
                dz->process      = STRANDS;
            }
            if((exit_status = create_next_outfile(n,sfnam,temp,dz))<0)
                return exit_status;
            stream = dz->parray[dz->rotmix + k];
            for(m = 0, j= 0; m < dz->datasize; m++,j+=2) {
                sprintf(temp,"%.16lf\t%lf\n",stream[j],stream[j+1]);
                if(fputs(temp,dz->fp) < 0) {
                    fclose(dz->fp);
                    sprintf(errstr,"CANNOT WRITE ROTATION-CONTROL2 DATA TO FILE %s\n",dz->outfilename);
                    return SYSTEM_ERROR;
                }
            }
        }                   
    }                       //  Last datafile is closed on "complete_output"
    return FINISHED;
}

/**************************** CREATE_NEXT_OUTFILE ****************************/

int create_next_outfile(int no, char *sfnam, char *temp,dataptr dz)
{
    int exit_status;
    if(fclose(dz->fp) < 0) {
        sprintf(errstr, "Failed to close file %s\n",dz->outfilename);
        dz->process_type = OTHER_PROCESS;   //  prevents complaint at "complete output"
        return(GOAL_FAILED);
    }
    strcpy(dz->outfilename,sfnam);
    sprintf(temp,"%d",no);
    strcat(dz->outfilename,temp);
    if(!sloom)
        strcat(dz->outfilename,".txt");
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0) {
        sprintf(errstr, "Failed to open file %s\n",dz->outfilename);
        /*free(outfilename);*/
        dz->process_type = OTHER_PROCESS;   //  prevents complaint at "complete output"
        return(GOAL_FAILED);
    }
    return(FINISHED);
}

/**************************** SCATTER_STREAMDATA_IN_LAST_CYCLE ****************************/

void scatter_streamdata_in_last_cycle(double time,int bandno,int threadscnt,int threadbas, int outcnt,double lastcycendtime,dataptr dz)
{
    double warp, cyclen, timefrac;
    int n, m, k, t;
    double *thisthread;
    cyclen = time - lastcycendtime;
    for(n=0,m= threadbas;n < threadscnt;n++,m++) {                      //  For each stream in the band
        thisthread = dz->parray[m];                                     //  Get timewarp-value
        warp = (dz->param[STRAND_SCAT] * (STRAND_MAXWARP - 1.0));       //  Range of warping, some fraction of 1-to-maxwarp range
        warp *=  drand48();                                             //  Get random value in range
        warp += 1.0;                                                    //  Add back the 1, so warp in range 1 to maxwarp   
        if(drand48() < 0.5)
            warp = 1.0/warp;                                            //  Warp then randomly gets value warp or 1/warp
        for(k=0,t=0;k < outcnt;k++,t+=2) {                              //  Search the existing stored values
            if(thisthread[t] > lastcycendtime) {                        //  For any stored value within last cycle
                timefrac = (thisthread[t] - lastcycendtime)/cyclen;     //  Find its time-position as a fraction of length of cycle (range 0-1)
                timefrac = pow(timefrac,warp);                          //  Warp its time-position (also range 0-1)
                thisthread[t] = (cyclen * timefrac) + lastcycendtime;   //  Overwrite original timing
            }
        }
    }
}

/**************************** WAVY ****************************
 *
 *  Motion of boundaries between bands.
 */

double wavy(int bandno,double bandtop_mean,double tessitura,double amp,double *wavyfrq,double *tabpos,int *cycend,double scaling,dataptr dz)
{
    double range, sinval, frqrange, frqmin, bandtop;
    sinval = sinread(0,tabpos,*wavyfrq,cycend,scaling,dz);  //  The tabpos is updated, the wavyfrq is fixed, the cycend is read
    range = (tessitura * WAVYFRAC) * amp;
    bandtop = bandtop_mean + (range * sinval);
    if(*cycend) {                                           //  The wavyfrq is updated only at a cycle end
        frqrange = fabs(dz->param[STRAND_VMAX] - dz->param[STRAND_VMIN]);
        frqmin = min(dz->param[STRAND_VMAX],dz->param[STRAND_VMIN]);
        *wavyfrq = drand48() * frqrange;
        *wavyfrq += frqmin;
    }
    return bandtop;
}

/************************************** CHECK_TWIST_VARIABLE ****************************
 *
 *  Twist can be zero at start of file, 
 *  or be zero over a stretch of the file, 
 *  i.e. there can be 2 adjacent zero values, but no more or less than 2, at any one location in the file.
 */

int establish_flow_types(dataptr dz) 
{
    int n, m, twisttime, twistval, t, v, k, f, d, lastflowtype, flowtype;
    int arraysize = dz->brksize[STRAND_TWIST];
    double *flow, lasttime;
    if(arraysize < 3)
        dz->flowcnt = 1;
    else {
        for(n=2,m= 5;n < arraysize;n++,m+=2) {
            if(twist[m] == 0.0 && twist[m-2] == 0.0 && twist[m-4] == 0.0) {     //  Compact any series of 3 zeros
                for(k = n*2; k < arraysize*2;k++)                               //  So zeros only exist, at most, in pairs
                    twist[k-2] = twist[k];
                arraysize--;
                n--;
                m-=2;
            }
        }
        dz->brksize[STRAND_TWIST] = arraysize;
        lastflowtype = -1;
        for(n=1,m= 3;n < arraysize;n++,m+=2) {
            if(twist[m] == 0.0 && twist[m-2] == 0.0)
                flowtype = LAMINAR;
            else if(twist[m-2] == 0.0 && twist[m] != 0.0)
                flowtype = TOTWIST;
            else if(twist[m-2] != 0.0 && twist[m] == 0.0)
                flowtype = FROMTWIST;
            else
                flowtype = TWISTED;
            if(flowtype != lastflowtype) {
                dz->flowcnt++;
                lastflowtype = flowtype;
            }
        }
    }
    if((dz->parray[dz->flowdata] = (double *)malloc((dz->flowcnt * 4) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing flow-types information.\n");
        return(MEMORY_ERROR);
    }
    flow = dz->parray[dz->flowdata];
    switch(arraysize) {
    case(0):
        flow[0] = 0.0;
        if(dz->param[STRAND_TWIST] == 0)    flow[1] = LAMINAR;
        else                                flow[1] = TWISTED;
        flow[2] = dz->param[STRAND_TWIST];
        flow[3] = dz->param[STRAND_DUR];
        break;
    case(1):
        flow[0] = 0.0;
        if(dz->brk[STRAND_TWIST][1] == 0)   flow[1] = LAMINAR;
        else                                flow[1] = TWISTED;
        flow[2] = dz->param[STRAND_TWIST];
        flow[3] = dz->param[STRAND_DUR];
        break;
    default:
        t = 0;
        v = 1;
        f = 2;
        d = 3;
        lastflowtype = -1;
        lasttime = 0.0;
        for(n=1,twisttime = 2,twistval = 3;n < arraysize;n++,twisttime+=2,twistval+=2) {
            if(twist[twistval-2] == 0.0 && twist[twistval] == 0.0)
                flowtype = LAMINAR;
            else if(twist[twistval-2] == 0.0 && twist[twistval] != 0.0)
                flowtype = TOTWIST;
            else if(twist[twistval-2] != 0.0 && twist[twistval] == 0.0)
                flowtype = FROMTWIST;
            else
                flowtype = TWISTED;
            if(flowtype != lastflowtype) {
                flow[t] = twist[twisttime-2];               //  Time of start of new flow-type
                flow[v] = flowtype;                         //  Flow type
                flow[d] = twist[twisttime] - lasttime;      //  Duration of totwist or fromtwist transition (if relevant)
                lasttime = twist[twisttime];
                switch(flowtype) {
                case(TOTWIST):
                    flow[f] = twist[twistval];              //  Twist value at true twist start (at end of transit)
                    break;
                case(FROMTWIST):
                case(TWISTED):
                    flow[f] = twist[twistval-2];            //  Twist value at start of transit (end of previous true-twist) 
                    break;                                  //  or start of true-twist (only used where flow starts with twist at time zero)
                case(LAMINAR):
                    flow[f] = 0;                            //  Value not used
                    break;
                }
                t+=4;
                v+=4;
                f+=4;
                d+=4;
                lastflowtype = flowtype;
            }
        }
        flow[0] = 0.0;      //  Force start at zero time
        break;
    }
    return FINISHED;
}

/******************************************* GET_FLOW_TYPE *********************************
 *
 *  Read off the type of flow being generated, from prestored table.
 */

int get_flow_type(double time,double *transitstart,double *twistreffrq,double *transittime,dataptr dz) 
{
    double *flow = dz->parray[dz->flowdata];
    int n = 0, t = 0, flow_type;
    while(time >= flow[t]) {
        if(++n > dz->flowcnt) {
            n--;
            break;
        }
        t += 4;
    }
    t -= 4;
    *transitstart = flow[t]; 
    flow_type = (int)round(flow[t+1]);
    *twistreffrq = flow[t+2];
    *transittime = flow[t+3];
    return flow_type;
}

/******************************************* TURBLEVEL *********************************/

double turblevel(double turbulence,double thislevel,dataptr dz)
{
    double level;
    level = drand48();
    level = pow(level,TURBPOW);
    if(turbulence < 1.0)
        level = (level * turbulence) + (thislevel * (1.0 - turbulence));
    return level;
}

/******************************************* CHECK_TURBULENCE_VALUES *********************************
 *
 *  Flow can be turbulent (turbulence > 0.0) where flow is TWISTED.
 */

int check_turbulence_values(int *isturb,dataptr dz)
{
    int exit_status, flow_type, n, ft, starttime, endtime, turbcnt, tt, vv;

    double *turb, *flow = dz->parray[dz->flowdata];
    double turbmax, turbtime, turbval, flowstart, flowend;

    //  IF turbulence is fixed, and not zero
        
    if(!dz->brksize[STRAND_TURB]) {
        if(dz->param[STRAND_TURB] > 0.0) {
            *isturb = 1;

            //  Turbulence everywhere: flow must be twisted everywhere
        
            for(n=0,ft = 1; n < dz->flowcnt;n++,ft+=4) {
                flow_type = (int)round(flow[ft]);
                if(flow_type != TWISTED) {
                    sprintf(errstr,"Turbulence cannot exceed zero if flow is not twisted.\n");
                    return DATA_ERROR;
                }
            }
        }
    } else {
        turb = dz->brk[STRAND_TURB];
        if((exit_status = get_maxvalue_in_brktable(&turbmax,STRAND_TURB,dz))<0)
            return exit_status;
        if(turbmax > 0.0)
            *isturb = 1;
        if(dz->flowcnt == 1) {

            //  If only a single flow-type, this must be twisted if flow is ANYWHERE turbulent

            flow_type = (int)round(flow[1]);
            if(flow_type != TWISTED && turbmax > 0.0) {
                sprintf(errstr,"Turbulence cannot exceed zero if flow is not twisted.\n");
                return DATA_ERROR;
            }
        } else {

            //  Turbulence at specific times: must coincide with twisted flow

            turbcnt = 0;
            tt = 0;
            vv = 1;
            turbtime = turb[tt];
            turbval  = turb[vv];
        
            for(n=0,starttime = 0,ft = 1,endtime = 3; n < dz->flowcnt-1;n++,starttime+=4,ft+=4,endtime +=4) {
                flow_type = (int)round(flow[ft]);
                
                //  Check turbulence value in all NON-twisted flows
                
                if(flow_type != TWISTED) {
                    flowstart = flow[starttime];
                    flowend   = flow[endtime];
                    while(turbtime < flowstart) {   //  Advance in turbulence values into next flowtype
                        if(++turbcnt >= dz->brksize[STRAND_TURB])
                            break;
                        tt += 2;    
                        vv += 2;
                        turbtime = turb[tt];
                        turbval  = turb[vv];
                    }
                    if(turbtime < flowstart) {      //  If reached end of turbulence values, 
                        if(turbval > 0.0)  {        //  test last turbulence value against all further flows
                            if(++n < dz->flowcnt) { //  (if there are any)
                                starttime += 4;
                                ft += 4;
                                for(;n <dz->flowcnt;n++,starttime+=4,ft+=4) {
                                    flow_type = (int)round(flow[ft]);
                                    if(flow_type != TWISTED) {
                                        sprintf(errstr,"Turbulence at %c in non-twisted flow.\n",starttime);
                                        return DATA_ERROR;
                                    }
                                }
                            }
                        }                           //  Then break out of outer test loop
                        break;
                    }
                    //  ELSE turbtime >= flowstart : 
                    //  Test turbulence values within the NON-twisted flow
                    if(turbval > 0.0) {
                        sprintf(errstr,"Turbulence at %lf in non-twisted flow.\n",turbtime);
                        return DATA_ERROR;
                    }
                    while(turbtime < flowend) {
                        if(++turbcnt >= dz->brksize[STRAND_TURB])
                            break;
                        tt += 2;    
                        vv += 2;
                        turbtime = turb[tt];
                        turbval  = turb[vv];
                        if(turbval > 0.0) {
                            sprintf(errstr,"Turbulence at %lf in non-twisted flow.\n",turbtime);
                            return DATA_ERROR;
                        }
                    }
                    if(turbtime < flowend) {        //  Reached end of turbulence values, 
                        if(turbval > 0.0)  {        //  so test last turbulence value against all further flows
                            if(++n < dz->flowcnt) { //  (if there are any)
                                starttime += 4;
                                ft += 4;
                                for(;n <dz->flowcnt;n++,starttime+=4,ft+=4) {
                                    flow_type = (int)round(flow[ft]);
                                    if(flow_type != TWISTED) {
                                        sprintf(errstr,"Turbulence at %d in non-twisted flow.\n",starttime);
                                        return DATA_ERROR;
                                    }
                                }
                            }
                        }                           //  Then break out of outer test loop
                        break;
                    }
                }
            }
        }
    }
    return FINISHED;
}

/********************************************** TURBULENT_BANDSWAP ********************************
 *
 *  Idea here is to invert the order of threads in some pattern to force bands to cross.
 *
 *  *perm points to the lowest band (&perm[threadbas]) in the current band.
 *  e.g. with 4 streans per band, pointing to band 1, it points to the group perm[4][5][6][7]
 *  which initially have the values 4,5,6,7
 *  The process then permutes the values of this group.
 *  However these are permd, this set of perm VALUES (4567) will always lie in this locations perm[4-7]
 *
 *  Threads are grouped into sets of "grpcnt".
 *  This grouping is selected AT RANDOM on each call.
 *  Then the order in each of these group is inverted.
 *
 *  There are 5 possibilities
 *
 *  (1) grpcnt = streamcnt
 *
 *      e.g. streamcnt = 8  A B C D E F G H
 *           grpcnt = 8     
 *          Reverse the entire set
 *                          H G F E D C B A
 *
 *  (2) grpcnt > 2 * streamcnt
 *
 *      e.g. streamcnt = 8  A B C D E F G H
 *           grpcnt = 5
 *          Reverse lowest group first, then next lowest etc
 *          (there will be some double reversings)
 *                          E D C B A f g h
 *                          e d c H G F A B
 *
 *
 *  (3) grpcnt <= 2 * streamcnt, but divides streamcnt exactly
 *
 *      e.g. streamcnt = 8  A B C D E F G H
 *           grpcnt = 2     
 *      Reverse each group in turn
 *                          B A c d e f g h
 *                          b a D C e f g h
 *                          b a d c F E g h
 *                          b a d c f e G H
 *
 *  (4) grpcnt <= 2 * streamcnt, but does NOT divide streamcnt exactly
 *      And remainder is 1
 *
 *      e.g. streamcnt = 7  A B C D E F G
 *           grpcnt = 3
 *      Reverse each complete group
 *                          C B A d e f g
 *                          c b a F E D g
 *      Then rotate the whole set
 *                          G c b a f e d
 *  OR 
 *  (5) grpcnt <= 2 * streamcnt, but does NOT divide streamcnt exactly
 *      And remainder is > 1
 *
 *      e.g. streamcnt = 8  a b c d e f g h
 *           grpcnt = 3
 *      Reverse each complete group, then the incomplete group
 *           grpcnt = 3     C B A d e f g h
 *                          c b a F E D g h
 *                          c b a f e d H G
 *
 */

void turbulent_bandswap(int *perm,int streamcnt)
{
    int temp, n, m, k, setcnt, remainder;
    int gpcnt = streamcnt - 1;                  //  e.g. with 6 streams: no of groupings = 5 (2,3,4,5,6)
    gpcnt = (int)floor(drand48() * gpcnt);      //  gpcnt range = 0 - 4
    gpcnt += 2;                                 //  gpcnt range = 2 - 6
    gpcnt = min(gpcnt,streamcnt);               //  Deal with vanishingly small probability that drand48() returns 1.0 exactly giving value 7!!

    if(gpcnt == streamcnt) {                    // CASE 1
        n = 0;
        m = streamcnt - 1;
        while(n < m) {      //  Invert entire group
            temp = perm[n];
            perm[n] = perm[m];
            perm[m] = temp;
            n++;
            m--;
        }
    } else if(gpcnt > streamcnt * 2) {      // CASE 2
        n = 0;
        m = gpcnt - 1;
        while(n < m) {      //  Invert bottom-abutted group
            temp = perm[n];
            perm[n] = perm[m];
            perm[m] = temp;
            n++;
            m--;
        }
        n = streamcnt - gpcnt;
        m = streamcnt - 1;
        while(n < m) {      //  Invert top-abutted group
            temp = perm[n];
            perm[n] = perm[m];
            perm[m] = temp;
            n++;
            m--;
        }
    }
    else {
        setcnt = streamcnt/gpcnt;
        for(k=0;k<setcnt;k++) {
            n = k * gpcnt;
            m = n + gpcnt - 1;
            while(n < m) {  //  Invert each group in turn
                temp = perm[n];
                perm[n] = perm[m];
                perm[m] = temp;
                n++;
                m--;
            }
        }
        remainder = streamcnt%gpcnt;
        switch(remainder) {
        case(0):                        //  CASE 3
            break;
        case(1):                        //  CASE 4
            temp = perm[streamcnt - 1];
            n = streamcnt - 1;
            while(n > 0) {              //  upshufl
                perm[n] = perm[n-1];
                n--;
            }                           // take top to bottom
            perm[0] = temp;
            break;
        default:                        //  CASE 5
            n = setcnt * gpcnt;
            m = streamcnt - 1;
            while(n < m) {  //  Invert the remnant set
                temp = perm[n];
                perm[n] = perm[m];
                perm[m] = temp;
                n++;
                m--;
            }
            break;
        }
    }
}

/********************************************* ASSIGN_TURBULENCE_PITCHES ********************************************
 *
 *  streamcnt  = no of streams in band      (or total number of streams)
 *  pitchbot   = lowpitch boundary of band  (or of entire range)
 *  pitchrange = pitchwidth of band         (or of entire flow)
 *  perm       = section of the perm array beginning at the lowest thread in this band 
 *                                          (or entire perm array for all threads)
 */

void assign_turbulence_pitches(int streamcnt,double pitchbot,double pitchrange,int *perm,double *turbpset,double *turbpitch)
{   
    int gotit = 0, n, m;
    double maxrand = 0.0, minrand = 0.0, randrange, scaleup, temp;

    //  GENERATE RANDOM VALUES WITHIN RANGE 0 to 1

    while(!gotit) {
        maxrand = 0.0;
        minrand = 1.0;
        for(n=0;n < streamcnt;n++) {
            turbpset[n] = drand48();
            minrand = min(minrand,turbpset[n]);
            maxrand = max(maxrand,turbpset[n]);
        }
        if(maxrand > minrand)       //  Ensure there's not a zero-spread of rand-values
            gotit = 1;
    }
    randrange = maxrand - minrand;
    scaleup = pitchrange/randrange;

    //  SORT INTO ASCENDING ORDER

    for(n=0;n < streamcnt-1;n++) {
        for(m=n+1;m < streamcnt;m++) {
            if(turbpset[m] < turbpset[n]) {
                temp        = turbpset[n];
                turbpset[n] = turbpset[m];
                turbpset[m] = temp;
            }
        }
    }
    //  SCALE TO PITCHRANGE

    for(n=0;n < streamcnt;n++) {
        turbpset[n] -= minrand;                 //  Move randomvalues into  0 - randrange
        turbpset[n] *= scaleup;                 //  Scale to full pitchrange
        turbpset[n] += pitchbot;                    //  Put in actual pitch values
    }
    turbulent_bandswap(perm,streamcnt);         //  Permute order of streams, so all bands cross
    for(n=0;n < streamcnt;n++)                  //  Assign the pitches to the bands
        turbpitch[perm[n]] = turbpset[n];               
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

/************************** HANDLE_THE_SPECIAL_DATA **********************************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,int *threadvalscnt,dataptr dz)
{
    int cnt, *i, idummy;
    char *filename = (*cmdline)[0];
    FILE *fp;
    double dummy;
    char temp[200], *q;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr, "Can't open data file %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    cnt = 0;
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,&dummy)) {
            idummy = (int)round(dummy);
            if(idummy < 2 || idummy > 100) {
                sprintf(errstr,"Invalid thread count (%d) in file %s (Valid Range 2 - 100)\n",idummy,filename);
                return(MEMORY_ERROR);
            }
            cnt++;
        }
    }
    if(cnt == 0) {
        sprintf(errstr,"No data in data file %s\n",filename);
        return(DATA_ERROR);
    }
    if((dz->iparray  = (int **)malloc(sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for input data in file %s.\n",filename);
        return(MEMORY_ERROR);
    }
    if((dz->iparray[0] = (int *)malloc(cnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for input data in file %s.\n",filename);
        return(MEMORY_ERROR);
    }
    fseek(fp,0,0);
    i = dz->iparray[0];
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,&dummy)) {
            *i = (int)round(dummy);
            i++;
        }
    }
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
        fflush(stdout);
    }
    *threadvalscnt = cnt;
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}
