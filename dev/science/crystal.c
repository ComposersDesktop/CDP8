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

/*  IDea here is to place a crystal in 3d space,
 *  Then to rotate crystal in 3d space, reading the position of the vertices as they appear projected onto the x-y plane.
 *
 *  If the original coordinates of a point (on a sphere) are x.y.z
 * then a rotation about the z-axis of X radians is given by matrix
 *
 *  cos(X)  sin(x)  0
 *  -sin(X) cos(X)  0
 *  0       0       1
 *
 * This creates new points (x',y',z') thus
 *
 *  x' = cos(X)*x  + sin(X)*y + 0*z
 *  y' = -sin(X)*x + cos(X)*y + 0*z
 *  z' = 0*x       + 0*y      + 1*z
 *
 *
 *  For a rotation around the y axis of Y radians, matrix is
 *
 *  cos(Y)    0     sin(Y)
 *  0         1     0
 *  -sin(Y)   0     cos(Y)
 *
 * This creates new points (x',y',z') thus
 *
 *  x' = cos(Y)*x  + 0*y  + sin(Y)*z
 *  y' = 0*x       + 1*y  + 0*z
 *  z' = -sin(Y)*x + 0*y  + cos(Y)*z
 *
 *  Calculate X and Y from the angular rotation speeds, and timestep, and apply them successively.
 *
 *
 *              params                  pos(xyz)    0         1         2           3           4           5       6           7       8           9
 *              params              of each vertex  ROTRATE-A ROTRATE-B TIMESTEP    DURATION    LOWPITCH    HIPITCH TIMEWIDTH   CHANS   FILT-PASS   FILT_STOP
 *  crystal rotate inf [inf2 ...] outf  special     rotA      rotB      tstep       dur         pl          ph      tw          ch      fp          fs
 *
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
#include <science.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>

// PARAMS

// ARRAYS

#define ORIG_VTX_DATA   (0)
#define VERTEX_DATA     (1)
#define ENV_DATA        (2)
#define CRY_DEN1        (3) 
#define CRY_DEN2        (4)
#define CRY_CN          (5)
#define CRY_S1          (6)
#define CRY_E1          (7)
#define CRY_S2          (8)
#define CRY_E2          (9)

//SND BUFFERS
//RWD OBUF and IBUF already defined in standalpone.h. So we need local names here
#define THISOBUF        (0) //  Output (multichan) buf
#define OVFLWBUF    (1) //  Overflow
#define TRNSBUF     (2) //  Trnasposed (by delay-process) sound buf
#define ENVBUF      (3) //  Enveloped-sound buf
#define FSBUF       (4) //  Filter or stack buffer
#define EBUF        (5) //  Raw-envelope buf
#define THISIBUF        (6) //  Input buf (or start of several input buffers)

// INTERNAL CONSTANTS

#define CRY_MINFBWIDTH  (50.0)  //  Minimum gap between pass and stop bands of filter
#define CRY_LBF         (200)   //  Param used in filter definitions
#define MAX_PROPORTION_8UP_IN_STAK  (0.66)  //  Max amount of 8va transposed version mixed into staks
#define MAXCRYSLEVEL    (0.95)
//  At present, transposed sounds normalised to 0.95.
//  If this results in stacks becoming less loud than non-stacks, change this to e.g. 0.5 ???
#define ALL_CHANS       (8) //  Channel count for 8-chan output
#define SIGNAL_TO_LEFT  (0)
#define SIGNAL_TO_RIGHT (1)
#define SAFETY          (16)
#define CRY_DOVE        (0.002)     // 2mS dovetail of start and end of srcs
#define CRY_STKFAC      (2.0)       //  Relation between the slope for introducing the 8va up transpos in the stack
                                    //  and the slope for introducing the two-oct transposition.
                                    //  e.g. if 8va slope is 0.5, 2_8va slope is 1
#define ROOT2           (1.4142136)

#define DOTEST 1

#define stackpeak       total_windows
#define no_of_vertices  itemcnt
#define envdatalen      ringsize
#define outcnt          could_be_transpos

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int check_crystal_param_validity_and_consistency(dataptr dz);
static int setup_crystal_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_crystal_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
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
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int handle_the_special_data(char *str,dataptr dz);
static int create_crystal_sndbufs(dataptr dz);
static int crystal_param_preprocess(dataptr dz);
//static int get_event_level(double time,double thispitch,double tabincr,int tabsize,int *obufpos,double *normaliser,double line_angle,double pos,dataptr dz);
static int read_value_from_brkarray(double *env,int *nextind,double *val,double thistime,dataptr dz);

static int stack_enveld_snd(double closeness, dataptr dz);
static int envelope_sound(int do_normalise,dataptr dz);
static int get_vectorlen(double *vectorlen,double x,double y,double z);
static void do_lphp_filter(dataptr dz);
static void initialise_filter_coeffs_lphp(dataptr dz);
static void calculate_filter_poles_lphp(double  signd,int filter_order,dataptr dz);
static int allocate_internal_params_lphp(dataptr dz);
static int establish_order_of_filter(dataptr dz);
static int setup_lphp_filter(dataptr dz);
static int filter_sound(double z, dataptr dz);
static int delay_transpose_input_sound(double midipitch, int n, dataptr dz);
static int calculate_pitch_and_time_params(double *midipitch,int *monosamptime,double x,double y,double eventtime,dataptr dz);
static int crystal_rotate(dataptr dz);
static int set_the_legal_internalparam_structure(aplptr ap);
static int handle_the_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz);
static void doperm(int *perm,int permlen);
static void hinsert(int m,int t,int *perm,int permlen);
static void hprefix(int m,int *perm,int permlen);
static void hshuflup(int k,int *perm,int permlen);
static int calculate_time_params(int *monosamptime,double x,double eventtime,dataptr dz) ;
static int check_position_of_event_group_in_output_buf(int passno,double *maxlevel,int *maxwrite,int minsamptime,double normaliser,dataptr dz);
static int write_sound_into_output_buf(int monosamptime,int minsamptime,double x,int eightrot,int *maxwrite,dataptr dz);
static void rotate_vertex(double *x,double *y,double *z,double rotation_in_xy_plane,double rotation_in_xz_plane,int vertexno,double eventtime,int *warning);
static void pancalc(double position,double *leftgain,double *rightgain);
static void dovetail(int dovelen, dataptr dz);
static int write_rotated_crystal_sound(float *obuf,int maxwrite, dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
    int n;
//    aplptr ap;
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
        dz->maxmode = 10;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_crystal_application(dz))<0) {
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
    if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_crystal_param_ranges_and_defaults(dz))<0) {
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

    if((exit_status = handle_the_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);      
        return(FAILED);
    }
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
//  handle_special_data .....
    if((exit_status = handle_the_special_data(cmdline[0],dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    cmdlinecnt--;
    cmdline++;
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency....
    if((exit_status = check_crystal_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = open_the_outfile(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    dz->bufcnt = 6;
    dz->bufcnt += dz->infilecnt;
    if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    for(n = 0;n <dz->bufcnt; n++)
        dz->sampbuf[n] = dz->sbufptr[n] = (float *)0;
    dz->sampbuf[n] = (float *)0;

    if((exit_status = create_crystal_sndbufs(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess ....
    if((exit_status = crystal_param_preprocess(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //spec_process_file =
    if((exit_status = crystal_rotate(dz))<0) {
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
    dz->infilecnt = -1;     //      Flags 1 or more infiles
        return(exit_status);
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
    int has_extension = 0, k;
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
    p = filename + strlen(filename);
    p--;
    while(p != filename) {
        if(*p == '.') {
            has_extension = 1;
            break;
        }
        p--;
    }
    strcpy(dz->outfilename,filename);
    if(!has_extension)
        strcat(dz->outfilename,".wav");
    if(dz->mode == 9) {
        k = strlen(dz->outfilename);
        if(sloom)
            k -= 5; // No need to store "0.wav"
        else
            k -= 4; // No need to store ".wav"
        if((dz->wordstor = (char **)malloc(1 * sizeof(char *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store generic name of outputfile (A).\n");
            return(MEMORY_ERROR);
        }
        if((dz->wordstor[0] = (char *)malloc((k+1) * sizeof(char)))==NULL) {    //  need extra space for ENDOFSTRING 
            sprintf(errstr,"INSUFFICIENT MEMORY to store generic name of outputfile (B).\n");
            return(MEMORY_ERROR);
        }
        strncpy(dz->wordstor[0],dz->outfilename,k);
        strcpy(dz->outfilename,dz->wordstor[0]);
        strcat(dz->outfilename,"0");
        strcat(dz->outfilename,".wav");
        dz->outcnt = 0;
    }
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
    int exit_status;
    if(dz->mode < 2)
        dz->infile->channels = dz->mode + 1;
    else if(dz->mode == 9)
        dz->infile->channels = 2;
    else
        dz->infile->channels = ALL_CHANS;
    dz->outchans = dz->infile->channels;
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
        return(exit_status);
    dz->infile->channels = 1;
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

/************************* SETUP_CRYSTAL_APPLICATION *******************/

int setup_crystal_application(dataptr dz)
{
    int exit_status;
    aplptr ap;

    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    exit_status = set_param_data(ap,CRYSTALDAT,7,7,"DDDDdDD");
    if(exit_status<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"psaPFS",6,"dddddd","",0,0,""))<0)
        return(FAILED);
    if((exit_status = set_the_legal_internalparam_structure(ap))<0)
        return(exit_status);                                        /* LIBRARY */
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = ONE_OR_MANY_SNDFILES;
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

/************************* SETUP_CRYSTAL_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_crystal_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()

    ap->lo[CRY_ROTA] = CRY_ROT_MIN;
    ap->hi[CRY_ROTA] = CRY_ROT_MAX;
    ap->default_val[CRY_ROTA] = 0.1;
    ap->lo[CRY_ROTB] = CRY_ROT_MIN;
    ap->hi[CRY_ROTB] = CRY_ROT_MAX;
    ap->default_val[CRY_ROTB] = 0.1;
    ap->lo[CRY_TWIDTH]  = CRY_TW_MIN;
    ap->hi[CRY_TWIDTH]  = CRY_TW_MAX;
    ap->default_val[CRY_TWIDTH] = 1;
    ap->lo[CRY_TSTEP] = CRY_TSTEP_MIN;
    ap->hi[CRY_TSTEP] = CRY_TSTEP_MAX;
    ap->default_val[CRY_TSTEP] = 1;
    ap->lo[CRY_DUR] = 0.1;
    ap->hi[CRY_DUR] = CRY_DUR_MAX;
    ap->default_val[CRY_DUR] = 20;
    ap->lo[CRY_PLO] = 0;
    ap->hi[CRY_PLO] = 127;
    ap->default_val[CRY_PLO] = 36;
    ap->lo[CRY_PHI] = 0;
    ap->hi[CRY_PHI] = 127;
    ap->default_val[CRY_PHI] = 72;
    ap->lo[CRY_FPASS]   = 16;
    ap->hi[CRY_FPASS]   = 4000;
    ap->default_val[CRY_FPASS] = CRY_PASSBAND;
    ap->lo[CRY_FSTOP]   = 50;
    ap->hi[CRY_FSTOP]   = 8000;
    ap->default_val[CRY_FSTOP] = CRY_STOPBAND;
    ap->lo[CRY_FATT]    = -96;
    ap->hi[CRY_FATT]    = 0;
    ap->default_val[CRY_FATT] = CRY_FATT_DFLT;
    ap->lo[CRY_FPRESC]  = 0;
    ap->hi[CRY_FPRESC]  = 1;
    ap->default_val[CRY_FPRESC] = CRY_FPRESC_DFLT;
    ap->lo[CRY_FSLOPE]  = 0.1;
    ap->hi[CRY_FSLOPE]  = 10;
    ap->default_val[CRY_FSLOPE] = CRYS_DEPTH_ATTEN;
    ap->lo[CRY_SSLOPE]  = 0.1;
    ap->hi[CRY_SSLOPE]  = 10;
    ap->default_val[CRY_SSLOPE] = CRYS_PROX_ATTEN;
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
            if((exit_status = setup_crystal_application(dz))<0)
                return(exit_status);
//            ap = dz->application;
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

/****************************** SET_THE_LEGAL_INTERNALPARAM_STRUCTURE *********************************/

int set_the_legal_internalparam_structure(aplptr ap)
{
    int exit_status;
    if((exit_status = set_internalparam_data("id",ap))<0)
        return exit_status;
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
    usage2("rotate");
    return(USAGE_ONLY);
}

/**************************** CHECK_CRYSTAL_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_crystal_param_validity_and_consistency(dataptr dz)
{
    double temp;
    if(!dz->brksize[CRY_PLO] && !dz->brksize[CRY_PHI]) {
        if(flteq(dz->param[CRY_PLO],dz->param[CRY_PHI])) {
            sprintf(errstr,"Zero pitchrange (%lf to %lf) specified.\n",dz->param[CRY_PLO],dz->param[CRY_PHI]);
            return(DATA_ERROR);
        } else if(dz->param[CRY_PLO] > dz->param[CRY_PHI]) {
            fprintf(stdout,"WARNING: Inverted pitchrange (%lf to %lf) specified.\n",dz->param[CRY_PLO],dz->param[CRY_PHI]);
            fflush(stdout);
        }
    }
    if(dz->brksize[CRY_TSTEP])
        dz->param[CRY_TSTEP] = dz->brk[CRY_TSTEP][1];
    if(dz->param[CRY_DUR] < dz->param[CRY_TSTEP]) {
        sprintf(errstr,"Output duration (%lf) less than timestep (%lf) from 1st event to next.\n",dz->param[CRY_DUR],dz->param[CRY_TSTEP]);
        return(DATA_ERROR);
    }
    if(dz->brksize[CRY_TWIDTH])
        dz->param[CRY_TWIDTH] = dz->brk[CRY_TWIDTH][1];
    if(dz->param[CRY_DUR] < dz->param[CRY_TWIDTH]) {
        sprintf(errstr,"Output duration (%lf) less than timewidth of first event (%lf).\n",dz->param[CRY_DUR],dz->param[CRY_TWIDTH]);
        return(DATA_ERROR);
    }
    if(dz->param[CRY_FPASS] > dz->param[CRY_FSTOP]) {
        temp = dz->param[CRY_FPASS];
        dz->param[CRY_FPASS] = dz->param[CRY_FSTOP];
        dz->param[CRY_FSTOP] = temp;
    }
    if((temp = dz->param[CRY_FSTOP] - dz->param[CRY_FPASS]) < CRY_MINFBWIDTH) { 
        sprintf(errstr,"Frequency difference between filter pass and stop bands (%lf) too small (min %lf Hz).\n",temp,CRY_MINFBWIDTH);
        return(DATA_ERROR);
    }
    return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"rotate"))          dz->process = CRYSTAL;
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
    if(!strcmp(str,"rotate")) {
        fprintf(stderr,
        "crystal rotate 1-10 fi [fi2 fi3..] fo vdat rota rotb twidth tstep dur plo phi\n"
        "        [-ppass -sstop] [-afatt] [-Pfpresc] [-Ffslope] [-Ssslope]\n"
        "\n"
        "Generate N snd-events based on position of N vertices of a crystal,\n"
        "Then rotate crystal in 3-d space, and generate another group of N events, etc.\n"
        "X coord -> time &, if stereo, space-position; Y -> pitch; Z -> brightness..i.e.\n"
        "Z-far snds lopass-filtrd mixed to orig; Z-close snds, 8va up, stacked on orig.\n"
        "\n"
        "FI        One Mono infile, multiply-read (with delay), generating out-events.\n"
        "          OR N mono infiles, generating different events for N vertices.\n"
        "FO        Output file.\n"
        "VDAT      Data file contains\n"
        "          (1) Triples, being (initial) X,Y,Z, coords of CRYSTAL VERTICES.\n"
        "                 Range > -1 to <1. Xsquared + Ysqrd + Zsqrd < 1 for all vertices.\n"
        "          (2) Time-val pairs defining envelope imposed on sound events.\n"
        "                 Times start at 0 & increase. Final time = duration of events.\n"
        "                 Value range 0 to 1. First and last values must be zero.\n"
        "ROTA,ROTB Rotate speed in xy_plane, & xz_plane, revs per sec (Range %.2lf to %.2lf)\n"
        "TWIDTH    Max time between onsets of 1st and last event in any N-events group.\n"
        "TSTEP     Time-step between each sampling of all N vertices of rotating-crystal.\n"
        "DUR       Total duration of output (must be greater than TSTEP and TWIDTH).\n"
        "PLO,PHI   Minimum and Maximum (MIDI) pitch of any event.\n"
        "PASS,STOP Pass+stop bands (Hz) for lopass filter.(stopfrq - passfrq >= %.0lf Hz).\n"
        "FATT      Max attenuation produced by filter-stop (dB) Range 0 to -96.\n"
        "FPRESC    Gain applied to attenuate source before applying filter (0-1).\n"
        "FSLOPE    Slope curve mixing filtered to unfilt snd (depth). (Range %.2lf to %.2lf).\n"
        "SSLOPE    Slope curve mixing transposed snd to orig (close). (Range %.2lf to %.2lf).\n"
        "          In both cases Linear slope = 1.0\n"
        "FOG       Generic name for output files.\n"
        "OUTCNT    Number of rotated-sets to output.\n"
        "MODES\n"
        "1  Mono output\n"
        "2  Stereo output\n"
        "3  Two chans of 8-chan output, spaced by single channel (here, chans 1 & 3).\n"
        "4,5,6  Ditto, chan-pair steps clockwise,anticlock or randomly btwn groups-of-events.\n"
        "7,8,9  Ditto, but pair of chans adjacent (e.g. 1,2 or 5,6).\n"
        "10     Stereo output: each set-of-vertices output as a separate soundfile.\n",
        CRY_ROT_MIN,CRY_ROT_MAX,CRY_MINFBWIDTH,MIN_FSLOPE,MAX_FSLOPE,MIN_SSLOPE,MAX_SSLOPE);
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
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

/**************************** HANDLE_THE_SPECIAL_DATA ****************************
 *
 * Series of lines containing x,y,z coords of crystal vertices.
 * ... followed by envelope data for creating sound from infile.
 */

int handle_the_special_data(char *str,dataptr dz)
{
    FILE *fp;
    double dummy = -1.0, /*sum,*/ vectorlen, lasttime, x, y, z;
    char temp[200], *p;
    int cnt = 0, linecnt = 0, vertexcnt = 0, datacnt = 0, jj, envelcnt = 0, k;
    int  istime, inenvel = 0;

    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read envelope data.\n",str);
        return(DATA_ERROR);
    }
    while(fgets(temp,200,fp)!=NULL) {
        cnt = 0;
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy))
            cnt++;
        if(inenvel)
            envelcnt += cnt;
        else {
            if(cnt != 3) {
                if(vertexcnt == 0) {    //  Must read all vertex-triples before reading envelope
                    sprintf(errstr,"Data in line %d not valid triples (x:y:z coords) in file %s\n",linecnt+1,str);
                    return(DATA_ERROR);
                } else {                //  Once all triples read, before counting envelope data, note size of triples-data
                    inenvel = 1;
                    envelcnt += cnt;
                }
            } else {
                vertexcnt++;
            }
        }
        linecnt++;
    }
    if(linecnt == 0) {
        sprintf(errstr,"No data found in file %s\n",str);
        return(DATA_ERROR);
    }
    if(vertexcnt == 0) {
        sprintf(errstr,"No crystal vertex data found in file %s\n",str);
        return(DATA_ERROR);
    }
    datacnt  = vertexcnt * 3;
    if(envelcnt == 0) {
        sprintf(errstr,"No envelope data found in file %s\n",str);
        return(DATA_ERROR);
    }
    if(ODD(envelcnt)) {
        sprintf(errstr,"envelope data not paired corectly in file %s\n",str);
        return(DATA_ERROR);
    }
    if(dz->infilecnt > 1 && vertexcnt != dz->infilecnt) {
        sprintf(errstr,"Number of input files (%d) does not correspond with number of vertices (%d)\n",dz->infilecnt,vertexcnt);
        return DATA_ERROR;
    }
    istime = 1;
    fseek(fp,0,0);
    linecnt = 0;
    cnt = 0;
    lasttime = -1.0;
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy)) {
            if(cnt < datacnt) {
                k = cnt % 3;
                if(dummy > 1.0 || dummy < -1.0) {
                    switch(k) {
                    case(0):
                        sprintf(errstr,"Crystal X-coord (%lf) out of range (-1 to 1) in line %d file %s\n",dummy,linecnt+1,str);
                        break;
                    case(1):
                        sprintf(errstr,"Crystal Y-coord (%lf) out of range (-1 to 1) in line %d file %s\n",dummy,linecnt+1,str);
                        break;
                    case(2):
                        sprintf(errstr,"Crystal Z-coord (%lf) out of range (-1 to 1) in line %d file %s\n",dummy,linecnt+1,str);
                        break;
                    }
                    return(DATA_ERROR);
                }
            } else {
                if(istime) {
                    if(lasttime < 0.0) {
                        if(dummy != 0.0) {
                            sprintf(errstr,"First time in envelope data (%lf) not at zero in line %d file %s\n",dummy,linecnt+1,str);
                            return(DATA_ERROR);
                        }
                    } else if(dummy <= lasttime) {
                        sprintf(errstr,"Times do not advance in envelope data at time (%lf) in line %d file %s\n",dummy,linecnt+1,str);
                        return(DATA_ERROR);
                    }
                    lasttime = dummy;
                } else {
                    if(lasttime == 0.0) {   //  Envelope values must start at zero
                        if(dummy != 0.0) {
                            sprintf(errstr,"First envelope value (%lf) is not zero in line %d file %s\n",dummy,linecnt+1,str);
                            return(DATA_ERROR);
                        }
                    } else if(dummy < 0.0 || dummy > 1.0) {
                        sprintf(errstr,"Envelope value (%lf) out of range (0 to 1) in line %d file %s\n",dummy,linecnt+1,str);
                        return(DATA_ERROR);
                    }
                }
                istime = !istime;
            }
            cnt++;
        }
        linecnt++;
    }
    if(dummy != 0.0) {
        sprintf(errstr,"Last envelope value (%lf) is not zero in line %d file %s\n",dummy,linecnt,str);
        return(DATA_ERROR);
    }
    if((dz->parray = (double **)malloc(10 * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store crystal special data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[ORIG_VTX_DATA] = (double *)malloc(datacnt * sizeof(double)))==NULL) {    //  Stores initial coords of cristal vertices
        sprintf(errstr,"INSUFFICIENT MEMORY to store crystal vertex coords data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[VERTEX_DATA] = (double *)malloc(datacnt * sizeof(double)))==NULL) {      //  Stores coords of cristal vertices as they rotate
        sprintf(errstr,"INSUFFICIENT MEMORY to store crystal vector data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[ENV_DATA] = (double *)malloc(envelcnt * sizeof(double)))==NULL) {        //  Stores sound-events-envelope
        sprintf(errstr,"INSUFFICIENT MEMORY to store envelope data.\n");
        return(MEMORY_ERROR);
    }
    dz->no_of_vertices = vertexcnt;
    dz->envdatalen = envelcnt;
    cnt = 0;
    fseek(fp,0,0);
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy)) {
            if(cnt < datacnt)
                dz->parray[ORIG_VTX_DATA][cnt] = dummy;                     //  Store initial coords of crystal vertices
            else
                dz->parray[ENV_DATA][cnt - datacnt] = dummy;                    //  Store sound-envelope data
            cnt++;
        }
    }
    fclose(fp);
    for(vertexcnt = 0; vertexcnt < dz->no_of_vertices; vertexcnt++) {
        jj = vertexcnt * 3;
//        sum = 0.0;
        x = dz->parray[ORIG_VTX_DATA][jj];
        y = dz->parray[ORIG_VTX_DATA][jj+1];
        z = dz->parray[ORIG_VTX_DATA][jj+2];
        if((get_vectorlen(&vectorlen,x,y,z))<0) {
            sprintf(errstr,"vertex %d lies outside the unit sphere.\n",vertexcnt+1);
            return DATA_ERROR;
        }
    }
    dz->rampbrksize = (int)ceil(lasttime * dz->infile->srate);                  //  Remember duration of envelope, in samples
    return FINISHED;
}

/**************************** CRYSTAL_ROTATE ****************************/

int crystal_rotate(dataptr dz)
{
    int passno, warning = 0, *perm, permno, exit_status, eightrot, vertexno;
    int vertexbas, vertindex, monosamptime, i;
    int maxwrite = 0, grpcnt, minsamptime;
    double *vertexcoord = dz->parray[VERTEX_DATA], rotation_in_xy_plane, rotation_in_xz_plane, midipitch, normaliser = 1.0;
    double *vertexorig = dz->parray[ORIG_VTX_DATA];
    double eventtime, outdur = dz->param[CRY_DUR], x, y, z, depth, closeness, maxlevel = 0.0;
    float *obuf = dz->sampbuf[THISOBUF];

    dz->tempsize = (int)ceil(outdur * (double)dz->infile->srate) * dz->outchans;

    if((perm = (int *)malloc(ALL_CHANS * sizeof(int))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create perm buffer.\n");
        return(PROGRAM_ERROR);
    }
    doperm(perm,ALL_CHANS);
    permno = 0;
    if(dz->mode == 5 || dz->mode == 8)  //  Random orientations in 8-channel space
        eightrot = perm[permno];
    else
        eightrot  = 0;  //  modes 3,4,6,7: rotating orientations in 8-channel space
    if((exit_status = setup_lphp_filter(dz))<0)
        return exit_status;
    for(passno = 0; passno < 2; passno++) {
        if(passno == 0)
            fprintf(stdout,"INFO: 1st pass : checking levels.\n");
        else
            fprintf(stdout,"INFO: 2nd pass : generating output.\n");
        fflush(stdout);
        rotation_in_xy_plane = 0.0;                                 //  INITIALISE ROTATIONS
        rotation_in_xz_plane = 0.0;
        for(vertexno=0;vertexno<dz->no_of_vertices;vertexno++) {    //  INITIALISE VERTEX-COORDS
            vertexbas = vertexno * 3;
            vertindex = vertexbas;
            vertexcoord[vertindex] = vertexorig[vertindex];
            vertindex++;
            vertexcoord[vertindex] = vertexorig[vertindex];
            vertindex++;
            vertexcoord[vertindex] = vertexorig[vertindex];
        }
        if(passno > 0)
            sndseekEx(dz->ifd[0],0,0);
        maxwrite = 0;
        dz->total_samps_written = 0;
        memset((char *)obuf,0,dz->buflen * 2 * sizeof(float));  //  Zero outbuffer and overflow buffer
        memcpy((char *)dz->parray[VERTEX_DATA],(char *)dz->parray[ORIG_VTX_DATA],(dz->no_of_vertices * 3 * sizeof(double)));
        eventtime = 0.0;
        while(eventtime < outdur) {
            if((exit_status = read_values_from_all_existing_brktables(eventtime,dz))<0)
                return DATA_ERROR;
            rotation_in_xy_plane += dz->param[CRY_ROTA] * dz->param[CRY_TSTEP] * TWOPI;     // rotation is TOTAL rotation from original position
            while(rotation_in_xy_plane >= TWOPI)
                rotation_in_xy_plane -= TWOPI;
            while(rotation_in_xy_plane < -TWOPI)
                rotation_in_xy_plane += TWOPI;
            if(flteq(0.0,rotation_in_xy_plane))         //  try to avoid rounding errors
                rotation_in_xy_plane = 0.0;
            if(flteq(TWOPI,rotation_in_xy_plane))       //  try to avoid rounding errors
                rotation_in_xy_plane = TWOPI;
            rotation_in_xz_plane += dz->param[CRY_ROTB] * dz->param[CRY_TSTEP] * TWOPI;     // rotation is TOTAL rotation from original position
            while(rotation_in_xz_plane >= TWOPI)
                rotation_in_xz_plane -= TWOPI;
            while(rotation_in_xz_plane < -TWOPI)
                rotation_in_xz_plane += TWOPI;
            if(flteq(0.0,rotation_in_xz_plane))         //  try to avoid rounding errors
                rotation_in_xz_plane = 0.0;
            if(flteq(TWOPI,rotation_in_xz_plane))       //  try to avoid rounding errors
                rotation_in_xz_plane = TWOPI;
            minsamptime = (int)(MAXINT - 1);
            for(vertexno=0;vertexno<dz->no_of_vertices;vertexno++) {
                vertexbas = vertexno * 3;
                x = vertexcoord[vertexbas];
                if((exit_status = calculate_time_params(&monosamptime,x,eventtime,dz))<0)   //  Calculate time of earliest event in group
                    return exit_status;
                minsamptime = min(minsamptime,monosamptime);
            }
            if(dz->mode == 9)
                maxwrite = 0;                       //  In mode 10, check how long vertex-set is, in each separate case, in order to write to its unique outfile
            else {
                if((exit_status = check_position_of_event_group_in_output_buf(passno,&maxlevel,&maxwrite,minsamptime,normaliser,dz))<0)
                    return exit_status;             //  In all other cases, check if outbuf is full, and if so, write to the (single) outfile
            }
            for(vertexno=0;vertexno<dz->no_of_vertices;vertexno++) {
                vertexbas = vertexno * 3;
                vertindex = vertexbas;
                x = vertexcoord[vertindex++];       //  NB vertexcoords have been initialised (above loop) to ORIGINAL COORDS
                y = vertexcoord[vertindex++];
                z = vertexcoord[vertindex];
                if((exit_status = calculate_pitch_and_time_params(&midipitch,&monosamptime,x,y,eventtime,dz))<0)
                    return exit_status;
                if((exit_status = delay_transpose_input_sound(midipitch,vertexno,dz))<0)    //  Transpose snd -> TRNSBUF : NORM
                    return exit_status;
                depth = z;
                if(depth < 0) {                                         //  Distant sounds are filtered
                    if((exit_status = filter_sound(depth,dz))<0)        //  Filter sound --> FSBUF--> mix with orig --> TRNSBUF
                        return exit_status;
                    if((exit_status = envelope_sound(1,dz))<0)          //  TRNSBUF --> ENVBUF : NORM
                        return exit_status;
                } else {
                    if((exit_status = envelope_sound(0,dz))<0)          //  TRNSBUF --> ENVBUF
                        return exit_status;
                    if(depth > 0) {                                     //  Close sounds are stacked
                        closeness = depth;
                        if((exit_status = stack_enveld_snd(closeness,dz)) < 0)  //  Transpose sound by 8va, --> FSBUF (suitably time-offset)
                            return PROGRAM_ERROR;                       //  and by 2 8vas --> TRANSBUF (suitably time-offset)
                    }                                                   //  Then mix the orig and 2 transpositions --> ENVBUF : NORM
                }
                if((exit_status = write_sound_into_output_buf(monosamptime,minsamptime,x,eightrot,&maxwrite,dz))<0)
                    return exit_status;

                //  AFTER using existing coords of vertices ... rotate the crystal

                vertindex = vertexbas;
                x = vertexorig[vertindex++];        //  Get original vertex coords
                y = vertexorig[vertindex++];
                z = vertexorig[vertindex];          //  Apply TOTAL rotation to this data
                rotate_vertex(&x,&y,&z,rotation_in_xy_plane,rotation_in_xz_plane,vertexno,eventtime,&warning);  // Do vector <= 1 test
                vertindex = vertexbas;
                vertexcoord[vertindex++] = x;       //  Reset vertex coords for next pass
                vertexcoord[vertindex++] = y;
                vertexcoord[vertindex]   = z;
            }
            switch(dz->mode) {
            case(3):    //  STEPPING CLOCKWISE
            case(6):
                if(++eightrot >= ALL_CHANS)                 //  Rotate (next) sound clockwise
                    eightrot -= ALL_CHANS;
                break;
            case(4):    //  STEPPING antiCLOCKWISE
            case(7):
                if(--eightrot < 0)                          //  Rotate (next) sound anticlockwise
                    eightrot += ALL_CHANS;
                break;
            case(5):    //  STEPPING RANDOMLY                   //  Random orientation
            case(8):
                if(++permno >= ALL_CHANS) {
                    doperm(perm,ALL_CHANS);
                    permno = 0;
                }
                eightrot = perm[permno];
                break;
            case(9):
                if(passno == 0) {
                    dz->total_samps_written += maxwrite;
                    dz->process = GREV;
                    display_virtual_time(0,dz);
                    dz->process = CRYSTAL;
                    for(i=0;i < maxwrite;i++)
                        maxlevel = max(maxlevel,fabs(obuf[i]));
                } else {
                    for(i=0;i < maxwrite;i++)
                        obuf[i] = (float)(obuf[i] * normaliser);
                    if((exit_status = write_rotated_crystal_sound(obuf,maxwrite,dz))<0)
                        return exit_status;
                }
                maxwrite = 0;
                break;
            }
            eventtime += dz->param[CRY_TSTEP];
        }
        if(dz->mode == 9) {
            if(passno == 0) {
                if(maxlevel > MAXCRYSLEVEL)
                    normaliser = MAXCRYSLEVEL/maxlevel;
            }
        } else {
            if(passno == 0) {
                if(maxwrite > 0) {
                    dz->total_samps_written += maxwrite;
                    dz->process = GREV;
                    display_virtual_time(0,dz);
                    dz->process = CRYSTAL;
                    for(i=0;i < maxwrite;i++)
                        maxlevel = max(maxlevel,fabs(obuf[i]));
                }
                if(maxlevel > MAXCRYSLEVEL)                 //  Set normaliser, even if no samps still to write
                    normaliser = MAXCRYSLEVEL/maxlevel;
            } else if(maxwrite > 0) {
                if(normaliser != 1.0) {
                    for(i=0;i < maxwrite;i++)               //  normalise output data
                        obuf[i] = (float)(obuf[i] * normaliser);
                }
                if(dz->outchans > 1) {
                    if(maxwrite % dz->outchans != 0) {      //  Ensure a whole final channel group is written
                        grpcnt = maxwrite/dz->outchans;
                        grpcnt++;
                        maxwrite = grpcnt * dz->outchans;
                    }
                }                                           //  write data to file, updating total_samps_written
                dz->process = GREV;
                if((exit_status = write_samps(obuf,maxwrite,dz))<0)
                    return(exit_status);
                dz->process = CRYSTAL;
            }   
        }
    }
    return FINISHED;
}

/*************************** CALCULATE_TIME_PARAMS **************************/

int calculate_time_params(int *monosamptime,double x,double eventtime,dataptr dz)
{
    double half_timewidth, time;
    
    half_timewidth = dz->param[CRY_TWIDTH]/2.0;
    time = eventtime + half_timewidth + (x * half_timewidth);
    *monosamptime = (int)round(time * dz->infile->srate);
    return FINISHED;
}

/*************************** CALCULATE_PITCH_AND_TIME_PARAMS **************************/

int calculate_pitch_and_time_params(double *midipitch,int *monosamptime,double x,double y,double eventtime,dataptr dz)
{
    double half_timewidth, time, half_prange;
    
    half_timewidth = dz->param[CRY_TWIDTH]/2.0;
    time = eventtime + half_timewidth + (x * half_timewidth);
    *monosamptime = (int)round(time * dz->infile->srate);
    half_prange = (dz->param[CRY_PHI] - dz->param[CRY_PLO])/2.0;
    *midipitch = dz->param[CRY_PLO] + half_prange + (y * half_prange);
    return FINISHED;
}

/*************************** DELAY_TRANSPOSE_INPUT_SOUND **************************/

int delay_transpose_input_sound(double midipitch, int vertexno, dataptr dz)
{
//  double frq, del, maxout, normaliser, absout, tabincr, tabpos, frac, diff;
//  int sampdel, srclen, opos, outpos, ipos, tabsize, thispos, nextpos, n;
    double frq, maxout, normaliser, absout, tabincr, tabpos, frac, diff;
    int srclen, opos, thispos, nextpos, n;
    float *ibuf, *trnsbuf = dz->sampbuf[TRNSBUF];

    memset((char *)trnsbuf,0,dz->rampbrksize * sizeof(float));
    if(dz->infilecnt > 1) {
        ibuf = dz->sampbuf[THISIBUF + vertexno];
        srclen = dz->insams[vertexno];
    } else {
        ibuf = dz->sampbuf[THISIBUF];
        srclen = dz->insams[0];
    }
    if(midipitch < 0 || midipitch > 127) {
        sprintf(errstr,"MIDI value out of range 0 - 127\n");
        return(GOAL_FAILED);
    }
    tabincr = (double)srclen/(double)dz->infile->srate; //  tabincr to read table once per second, i.e. at 1Hz

    frq = miditohz(midipitch);
    tabincr *= frq;                                 //  Frq-related table-read increment
    tabpos = 0;
    for(n = 0; n< dz->rampbrksize;n++) {
        thispos = (int)floor(tabpos);               //  Read input sample by interpolation
        nextpos = thispos+1;                        //  with incr determined by pitch/frq
        frac = tabpos - thispos;
        diff =  ibuf[nextpos] - ibuf[thispos];
        diff *= frac;
        trnsbuf[n] = (float)(ibuf[thispos] + diff);
        tabpos += tabincr;
        if(tabpos >= srclen)
            tabpos -= srclen;
    }
    maxout = -1;
    for(opos = 0; opos < dz->rampbrksize; opos++) {                 //  Find max sample
        absout = fabs(trnsbuf[opos]);
        maxout = max(absout,maxout);
    }
    if(maxout > MAXCRYSLEVEL) {
        normaliser = MAXCRYSLEVEL/maxout;
        for(opos = 0; opos < dz->rampbrksize; opos++)               //  Normalise
            trnsbuf[opos] = (float)(trnsbuf[opos] * normaliser);
    }
    return(FINISHED);
}

/*************************** FILTER_SOUND **************************/

int filter_sound(double z, dataptr dz)
{
    int i;
    float *trnsbuf = dz->sampbuf[TRNSBUF], *filtbuf = dz->sampbuf[FSBUF];
    double val, filtsig_level, unfilt_siglevel, maxval, normaliser;
    memset((char *)filtbuf,0,dz->rampbrksize * sizeof(float));
    do_lphp_filter(dz); //  Filters the transposed-snd in TRNSBUF, to FSBUF
    z = -z;             //  Change range from -1 to <0 to >0 to 1
    filtsig_level = pow(z,dz->param[CRY_FSLOPE]);
    unfilt_siglevel = 1.0 - filtsig_level;
    maxval = -1;
    for (i = 0 ; i < dz->rampbrksize; i++) {
        val = (trnsbuf[i] * unfilt_siglevel) + (filtbuf[i] * filtsig_level);        //  Mix filtered and unfiltered sound
        trnsbuf[i] = (float)val;
        maxval = max(fabs(val),maxval);
    }
    if(maxval > MAXCRYSLEVEL) {
        normaliser = MAXCRYSLEVEL/maxval;
        for (i = 0 ; i < dz->rampbrksize; i++) {
            trnsbuf[i] = (float)(trnsbuf[i] * normaliser);  
        }
    }
    return FINISHED;
}

/*************************** CREATE_CRYSTAL_SNDBUFS **************************/

int create_crystal_sndbufs(dataptr dz)
{
    int exit_status;
    unsigned int bigbufsize, inbufssize;
    int max_tw, n, m, eventlen, evbufsize;
    double maxtw;
    if(dz->sbufptr == 0 || dz->sampbuf==0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->brksize[CRY_TWIDTH]) {
        if((exit_status = get_maxvalue_in_brktable(&maxtw,CRY_TWIDTH,dz))<0)
            return exit_status;
    } else 
        maxtw = dz->param[CRY_TWIDTH];
    max_tw = (int)ceil(maxtw * (double)dz->infile->srate);          //  maximum time between first and last event onset within a time-set
    eventlen = dz->rampbrksize;                                     //  duration of event(s) in timeset
    dz->buflen = (max_tw + /* final */ eventlen) * dz->outchans;    //  Scale up from mono to number of output chans
    dz->buflen += SAFETY;
    inbufssize = 0;
    for(n=0;n<dz->infilecnt;n++)
        inbufssize += dz->insams[n] + 1;    //  Add wrap-around point

    evbufsize = dz->rampbrksize;    //  Store size of envelope, in samples

    bigbufsize = (dz->buflen * 2) + (evbufsize * 4) + inbufssize;   //  Need space for outbuf & overflowbuf
    if((dz->bigbuf = (float *)malloc(bigbufsize * sizeof(float))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }

    // MONO
    //    obuf         ovflwbuf     transposed event    enveloped event    filt/stack  raw-envelope     input sound
    //    obuf         ovflwbuf         trnsbuf             envbuf          fsbuf           ebuf            ibufs...
    //      0             1                 2                 3               4              5               6          (7 etc)
    //  |-----------|--------------|------------------|------------------|------------|-------------|---------------|---------------|
    //              
    //   buflen        buflen           evbufsize          evbufsize        evbufsize    evbufsize      insams[0]+1   (insams[1]+1 etc)
    //
    //  Read from inbuf, transpose into transposedeventbuf, but only as far as end of buf
    //  Envelope result into envelopedeventbuf (using sample-scale raw-envelope in "envelope")
    //  Add event to obuf (in multichan format if ness)
    //  If next group-of-writes start in overflwbuf, write obuf, and copy ovflwbuf->obuf, and zero ovflwbuf
    //  BUT NB ...
    //                                                                                          i  t  f f+t e
    //  If filtering used .... filter BEFORE enveloping (and curtail output to buffer size)     6->2->4->2->3  write to obuf from 3
    //                                                                                          i  t  e  s e+s(NORMD)
    //  If stack used .......... stack AFTER enveloping (so stack is no longer than evbufsize)  6->2->3->4->3  write to obuf from 3


    n = 0;
    dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf;                       // obuf [0]             //  0 = Output buffer   
    n++;                                                                                        //      size buflen
    dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + dz->buflen;    // ovflwbuf [1]         //  1 = overflow buffer
    n++;                                                                                        //      size buflen
    dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + dz->buflen;    // trnsbuf  [2]         //  2 = created event (transposition of ibuf)
    n++;                                                                                        //      size evbufsize
    dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + evbufsize;     // envbuf   [3]         //  3 = enveloped event
    n++;                                                                                        //      size evbufsize
    dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + evbufsize;     // fsbuf    [4]         //  4 = filtered or stacked
    n++;                                                                                        //      size evbufsize
    dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + evbufsize;     // ebuf     [5]         //  5 = raw envelope
    n++;                                                                                        //      size evbufsize
    dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + evbufsize;     // ibuf     [6]         //  6 = 1st insndbuf
    if(dz->infilecnt > 1) {                                                                     //      size insams[0]
        for(m=1;m<dz->infilecnt;m++) {                                  // +ibufs   [7.....]
            n++;                                                                                //  7etc = more insndbufs
            dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + (dz->insams[m-1] + 1);         //      size insams[m]
        }
    }
    return(FINISHED);
}

/************************************* CRYSTAL_PARAM_PREPROCESS ***********************************
 *
 * (1)  Read input file(s) to buffer(s), with wraparound point, for reading as a waveform table.
 * (2)  Convert input envelope to a sample scale array in another buffer.
 */

int crystal_param_preprocess(dataptr dz) 
{
    int exit_status;
    double *env = dz->parray[ENV_DATA], maxval = -1;
    int n, sampsread, dovelen;
    double srate = (double)dz->infile->srate, val, thistime;
    int origbuflen = dz->buflen, nextind;
    float *ibuf = dz->sampbuf[THISIBUF];
    float *ebuf = dz->sampbuf[EBUF];

    dovelen = (int)(CRY_DOVE * (double)dz->infile->srate);

    //  For multiple input files

    for(n = 0; n< dz->infilecnt;n++) {
        if(dz->insams[n] <= dovelen * 2.0) {
            sprintf(errstr,"Input file %d too short for start-and-end dovetails (min size %lf secs)\n",n+1,CRY_DOVE * 2);
            return DATA_ERROR;
        }
        dz->buflen = dz->insams[n];             //  Read input sound(s) to ibuf(s)
        ibuf = dz->sampbuf[THISIBUF+n];
        memset((char *)ibuf,0,dz->buflen * sizeof(float));
        if((sampsread = fgetfbufEx(ibuf, dz->buflen,dz->ifd[n],0)) < 0) {
            sprintf(errstr,"Can't read samples from input soundfile %d.\n",n+1);
            return(SYSTEM_ERROR);
        }
        ibuf[dz->buflen] = 0;                   //  Wrap-around zero-point
    }
    dz->buflen = origbuflen;
    dovetail(dovelen,dz);                       //  Dovetail input sounds

    dz->stackpeak = 0;
    nextind = 2;                                //  Read input envelope array into a sample-scale array in a buffer
    for(n = 0; n < dz->rampbrksize; n++) {
        thistime = (double)n/srate;
        if((exit_status = read_value_from_brkarray(env,&nextind,&val,thistime,dz))<0)
            return exit_status;
        ebuf[n] = (float)val;
        if(fabs(val) > maxval) {                //  Find loudest point in envelope (for stacking)
            maxval = fabs(val);
            dz->stackpeak = n;
        }
    }
    return FINISHED;
}

/**************************** READ_VALUE_FROM_BRKARRAY *****************************/

int read_value_from_brkarray(double *env,int *nextind,double *val,double time,dataptr dz)
{
    double thistim, nexttim, thisval, nextval, valdiff, timdiff, timfrac;
    nexttim = env[*nextind];
    while(time > nexttim) {
        if((*nextind += 2) >= dz->envdatalen) {
            sprintf(errstr, "Overshot end of envelope brktable while converting to sample-buffer.\n");
            return PROGRAM_ERROR;
        }
        nexttim = env[*nextind];
    }
    thistim = env[*nextind - 2];
    thisval = env[*nextind - 1];
    nextval = env[*nextind + 1];
    valdiff = nextval - thisval;
    timdiff = nexttim - thistim;
    timfrac = (time - thistim)/timdiff;
    valdiff *= timfrac;
    *val = thisval + valdiff;
    return FINISHED;
}

/********************************* SETUP_LPHP_FILTER *****************************/

int setup_lphp_filter(dataptr dz)
{
    int exit_status;
    int filter_order;
    double signd = -1.0;    /* low pass */
    filter_order = establish_order_of_filter(dz);
    if((exit_status = allocate_internal_params_lphp(dz))<0)
        return(exit_status);
    calculate_filter_poles_lphp(signd,filter_order,dz);
    initialise_filter_coeffs_lphp(dz);
    fflush(stdout);
    return(FINISHED);
}

/********************************* ESTABLISH_ORDER_OF_FILTER *****************************/

int establish_order_of_filter(dataptr dz)
{
    int filter_order;
    double tc, tp, tt, pii, xx, yy;
    double sr = (double)dz->infile->srate;
    if (dz->param[CRY_FPASS] < dz->param[CRY_FSTOP])        /* low pass */
        dz->param[CRY_MUL] = 2.0;
    else {
        dz->param[CRY_MUL] = -2.0;
        dz->param[CRY_FPASS] = dz->nyquist - dz->param[CRY_FPASS];
        dz->param[CRY_FSTOP] = dz->nyquist - dz->param[CRY_FSTOP];
    }
    pii = 4.0 * atan(1.0);
    dz->param[CRY_FPASS] = pii * dz->param[CRY_FPASS]/sr;
    tp = tan(dz->param[CRY_FPASS]);
    dz->param[CRY_FSTOP] = pii * dz->param[CRY_FSTOP]/sr;
    tc = tan(dz->param[CRY_FSTOP]);
    tt = tc / tp ;
    tt = (tt * tt);
    dz->param[CRY_FATT] = fabs(dz->param[CRY_FATT]);
    dz->param[CRY_FATT] = dz->param[CRY_FATT] * log(10.0)/10.0 ;
    dz->param[CRY_FATT] = exp(dz->param[CRY_FATT]) - 1.0 ;
    xx = log(dz->param[CRY_FATT])/log(tt) ;
    yy = floor(xx);
    if ((xx - yy) == 0.0 )
        yy = yy - 1.0 ;
    filter_order = ((int)yy) + 1;
    if (filter_order <= 1) 
        filter_order = 2;
    dz->iparam[CRY_CNT] = filter_order/2 ;
    filter_order = 2 * dz->iparam[CRY_CNT] ;
    fprintf(stdout,"INFO: Order of filter is %d\n", filter_order);
    fflush(stdout);
    dz->iparam[CRY_CNT] = min(dz->iparam[CRY_CNT],CRY_LBF);
    filter_order = 2 * dz->iparam[CRY_CNT];
    return(filter_order);
}

/********************************* ALLOCATE_INTERNAL_PARAMS_LPHP *****************************/

int allocate_internal_params_lphp(dataptr dz)
{
    if((dz->parray[CRY_DEN1] = (double *)malloc(dz->iparam[CRY_CNT] * sizeof(double)))==NULL
    || (dz->parray[CRY_DEN2] = (double *)malloc(dz->iparam[CRY_CNT] * sizeof(double)))==NULL
    || (dz->parray[CRY_CN]   = (double *)malloc(dz->iparam[CRY_CNT] * sizeof(double)))==NULL
    || (dz->parray[CRY_S1]   = (double *)malloc(dz->iparam[CRY_CNT] * sizeof(double)))==NULL
    || (dz->parray[CRY_E1]   = (double *)malloc(dz->iparam[CRY_CNT] * sizeof(double)))==NULL
    || (dz->parray[CRY_S2]   = (double *)malloc(dz->iparam[CRY_CNT] * sizeof(double)))==NULL
    || (dz->parray[CRY_E2]   = (double *)malloc(dz->iparam[CRY_CNT] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for arrays of filter parameters.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/********************************* CALCULATE_FILTER_POLES_LPHP *****************************/

void calculate_filter_poles_lphp(double  signd,int filter_order,dataptr dz)
{
    double ss, xx, aa, tppwr, x1, x2, cc;
    double pii = 4.0 * atan(1.0);
    double tp = tan(dz->param[CRY_FPASS]);
    int    k;
    ss = pii / (double)(2 * filter_order);
    for (k = 0; k < dz->iparam[CRY_CNT]; k++ ) {
        xx = (double) ((2.0 * (k+1)) - 1.0);
        aa = -sin(xx * ss);
        tppwr = pow(tp,2.0);
        cc = 1.0 - (2.0 * aa * tp) + tppwr;
        x1 = 2.0 * (tppwr - 1.0)/cc ;
        x2 = (1.0 + (2.0 * aa * tp) + tppwr)/cc ;
        dz->parray[CRY_DEN1][k] = signd * x1;
        dz->parray[CRY_DEN2][k] = -x2 ;
        dz->parray[CRY_CN][k]   = pow(tp,2.0)/cc ;
    }
}

/********************************* INITIALISE_FILTER_COEFFS_LPHP *****************************/

void initialise_filter_coeffs_lphp(dataptr dz)
{
    int k;
    for (k = 0 ; k < dz->iparam[CRY_CNT]; k++) {
        dz->parray[CRY_S1][k] = 0.0;
        dz->parray[CRY_S2][k] = 0.0;
        dz->parray[CRY_E1][k] = 0.0;
        dz->parray[CRY_E2][k] = 0.0;
    }
}

/***************************** DO_LPHP_FILTER *************************************/

void do_lphp_filter(dataptr dz)
{
    double *e1   = dz->parray[CRY_E1];
    double *e2   = dz->parray[CRY_E2];
    double *s1   = dz->parray[CRY_S1];
    double *s2   = dz->parray[CRY_S2];
    double *den1 = dz->parray[CRY_DEN1];
    double *den2 = dz->parray[CRY_DEN2];
    double *cn   = dz->parray[CRY_CN];
    int i, hasreported = 0;
    int  k;
    float *trnsbuf = dz->sampbuf[TRNSBUF], *filtbuf = dz->sampbuf[FSBUF];
    double ip, op = 0.0, b1;
    for (i = 0 ; i < dz->rampbrksize; i++) {
        ip = (double) trnsbuf[i];
        for (k = 0 ; k < dz->iparam[CRY_CNT]; k++) {
            b1    = dz->param[CRY_MUL] * cn[k];
            op    = (cn[k] * ip) + (den1[k] * s1[k]) + (den2[k] * s2[k]) + (b1 * e1[k]) + (cn[k] * e2[k]);
            s2[k] = s1[k];
            s1[k] = op;
            e2[k] = e1[k];
            e1[k] = ip;
        }
        op *= dz->param[CRY_FPRESC];
        if (fabs(op) > 1.0) {
#ifdef DOTEST
            if(!hasreported) {
                fprintf(stdout,"INFO: Overflow in Lowpass filter.\n");
                fflush(stdout);
                hasreported = 1;
            }
#endif
            dz->param[CRY_FPRESC] *= .9999;
            if (op  > 0.0)
                op = 1.0;
            else 
                op = -1.0;
        }
        filtbuf[i] = (float)op;
    }
}

/******************************** VALID_VECTORLEN *********************************/

int get_vectorlen(double *vectorelen,double x,double y,double z)
{
    double sum;
    sum = (x*x) + (y*y) + (z*z);
    *vectorelen = sqrt(sum);
    if(*vectorelen > 1.0)           //  i.e. sqrt(sum) > 1.0
        return -1;
    return 1;
}

/******************************** ENVELOPE_SOUND *********************************/

int envelope_sound(int do_normalise,dataptr dz)
{
    int i;
    double maxval = -1.0, normaliser, val;
    float *trnsbuf = dz->sampbuf[TRNSBUF], *envbuf = dz->sampbuf[ENVBUF], *ebuf = dz->sampbuf[EBUF];
    memset((char *)envbuf,0,dz->rampbrksize * sizeof(float));
    if(do_normalise) {
        for(i=0; i <dz->rampbrksize; i++) {
            val = trnsbuf[i] * ebuf[i];
            maxval = max(maxval,fabs(val));
            envbuf[i] = (float)val;             //  Enveloped sound into ENVBUF
        }
        if(maxval > MAXCRYSLEVEL) {
            normaliser = MAXCRYSLEVEL/maxval;
            for(i=0; i <dz->rampbrksize; i++)
                envbuf[i] = (float)(envbuf[i] * normaliser);
        }
    } else {
        for(i=0; i <dz->rampbrksize; i++)
            envbuf[i] = (float)(trnsbuf[i] * ebuf[i]);
    }
    return FINISHED;
}

/******************************** STACK_ENVELD_SND *********************************/

int stack_enveld_snd(double closeness,dataptr dz)
{
    float *envbuf = dz->sampbuf[ENVBUF], *fsbuf = dz->sampbuf[FSBUF], *transbuf = dz->sampbuf[TRNSBUF];
    int offset = dz->stackpeak/2, i;
    double maxval, octup_level, twooctup_level, src_level, val, normaliser;
    memset((char *)fsbuf,0,dz->rampbrksize * sizeof(float));
    memset((char *)transbuf,0,dz->rampbrksize * sizeof(float));
    for(i=0; i <dz->rampbrksize; i+=2) {            //  Transpose enveld snd by 8va up, offset so peak of envelopes, 
        if(offset >= dz->rampbrksize) {             //  in transposed and untransposed sounds, coincide.
            sprintf(errstr,"Stacking produced overflow of fsbuf.\n");
            return PROGRAM_ERROR;
        }
        fsbuf[offset++] = envbuf[i];
    }
    offset = dz->stackpeak * 3;
    offset = offset/4;
    for(i=0; i <dz->rampbrksize; i+=4) {            //  Transpose enveld snd by TWO 8va up, offset so peak of envelopes, 
        if(offset >= dz->rampbrksize) {             //  in transposed and untransposed sounds, coincide.
            sprintf(errstr,"Stacking produced overflow of fsbuf.\n");
            return PROGRAM_ERROR;
        }
        transbuf[offset++] = envbuf[i];
    }
    maxval = -1;
    octup_level = pow(closeness,dz->param[CRY_SSLOPE]) * MAX_PROPORTION_8UP_IN_STAK;
    twooctup_level= pow(closeness,(dz->param[CRY_SSLOPE] * CRY_STKFAC)) * MAX_PROPORTION_8UP_IN_STAK;
    src_level = 1.0 - octup_level;
    for(i=0; i <dz->rampbrksize; i++) { //  Mix orig and 8va transposed sounds 
        val = (envbuf[i] * src_level) + (fsbuf[i] * octup_level) + (transbuf[i] * twooctup_level);
        envbuf[i] = (float)val;
        maxval = max(maxval,fabs(val));
    }
    if(maxval > MAXCRYSLEVEL) {
        normaliser = MAXCRYSLEVEL/maxval;
        for(i=0; i <dz->rampbrksize; i++)
            envbuf[i] = (float)(envbuf[i] * normaliser);
    }
    return FINISHED;
}

/******************************** CHECK_POSITION_OF_SOUND_IN_OUTPUT_BUF ********************************
 *
 *  Check if the earliest of the event in the event-group is beyond the current buffer end
 *  and if so, write (or get max of) buffer, and advance buffers.
 */

int check_position_of_event_group_in_output_buf(int passno,double *maxlevel,int *maxwrite,int minsamptime,double normaliser,dataptr dz) // (check overflows - write to outfile)
{
    int exit_status;
    int absopos, i; // Absolute position of write-position in output file
    float *obuf = dz->sampbuf[THISOBUF], *ovflwbuf = dz->sampbuf[OVFLWBUF];
    switch(dz->mode) {
    case(0):    absopos = minsamptime;              break;  // MONO
    case(1):    absopos = minsamptime * 2;          break;  // STEREO
    default:    absopos = minsamptime * ALL_CHANS;  break;  // 8-CHAN
    }

    while(absopos > dz->total_samps_written + dz->buflen) { //  If current write-start is beyond end of current buffer
        if(passno == 0) {                                       
            for(i=0;i < dz->buflen;i++)
                *maxlevel = max(*maxlevel,fabs(obuf[i]));   //  write data to file, updating total_samps_written
            dz->total_samps_written += dz->buflen;
            dz->process = GREV;
            display_virtual_time(0,dz);
            dz->process = CRYSTAL;
        } else {                                            //  This could involve writing silent buffers
            if(normaliser != 1.0) {
                for(i=0;i < dz->buflen;i++)                 
                    obuf[i] = (float)(obuf[i] * normaliser);
            }
            dz->process = GREV;
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            dz->process = CRYSTAL;
        }
        for(i=0;i < dz->buflen;i++)                         //  Copy overflow back into obuf
            obuf[i] = ovflwbuf[i];                          //  and zero overflow   
        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
        *maxwrite -= dz->buflen;
    }
    return FINISHED;
}

/******************************** WRITE_SOUND_INTO_OUTPUT_BUF ********************************/

int write_sound_into_output_buf(int monosamptime,int minsamptime,double x,int eightrot,int *maxwrite, dataptr dz)
{
    int absopos;    // Absolute position of write-position in output file
    int opos, i, leftopos, rightopos;
    double leftgain = 0.0, rightgain = 0.0, val;
    float *obuf = dz->sampbuf[THISOBUF], *envbuf = dz->sampbuf[ENVBUF];
    switch(dz->mode) {
    case(0):    absopos = monosamptime;             break;  // MONO
    case(1):    absopos = monosamptime * 2;         break;  // STEREO
    default:    absopos = monosamptime * ALL_CHANS; break;  // 8-CHAN
    }
    if(dz->mode == 9)
        opos = (monosamptime - minsamptime) * 2;                //  New buffer for each vertex-set, and stereo
    else
        opos = absopos - dz->total_samps_written;

    switch(dz->mode) {
    case(0):    // MONO
        for(i=0;i < dz->rampbrksize;i++) {                      //  The buffer + overflow is always bigger than dz->rampbrksize = size of single event
            obuf[opos] = (float)(obuf[opos] + envbuf[i]);       //  + the total width of the event-group
            opos++;
        }
        *maxwrite = max(*maxwrite,opos);
        break;
    case(1):
    case(9):    // STEREO
        pancalc(x,&leftgain,&rightgain);
        for(i=0;i < dz->rampbrksize;i++) {
            val = obuf[opos] + (envbuf[i] * leftgain);
            obuf[opos++] = (float)val;
            val = obuf[opos] + (envbuf[i] * rightgain);
            obuf[opos++] = (float)val;
        }
        *maxwrite = max(*maxwrite,opos);
        break;
    default:
        leftopos = opos + eightrot;
        switch(dz->mode) {
        case(2):
        case(3):
        case(4):
        case(5): // STEREO-BETWEEN NON-ADJACENT CHANS IN 8-CHAN SPACE
            if((rightopos = leftopos + 2) >= ALL_CHANS)
                rightopos -= ALL_CHANS;
            break;
        default: // STEREO-BETWEEN ADJACENT CHANS IN 8-CHAN SPACE
            if((rightopos = leftopos + 1) >= ALL_CHANS)
                rightopos -= ALL_CHANS;
            break;
        }
        pancalc(x,&leftgain,&rightgain);
        for(i=0;i < dz->rampbrksize;i++) {
            val = obuf[leftopos] + (envbuf[i] * leftgain);
            obuf[leftopos] = (float)val;
            val = obuf[rightopos] + (envbuf[i] * rightgain);
            obuf[rightopos] = (float)val;
            leftopos += ALL_CHANS;
            rightopos += ALL_CHANS;
        }
        *maxwrite = max(*maxwrite,opos+ALL_CHANS);
    }
    return FINISHED;
}

/************************ HANDLE_THE_EXTRA_INFILES *********************/

int handle_the_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
                    /* OPEN ANY FURTHER INFILES, CHECK COMPATIBILITY, STORE DATA AND INFO */
    int  exit_status, n;
    char *filename;

    if(dz->infilecnt > 1) {
        for(n=1;n<dz->infilecnt;n++) {
            filename = (*cmdline)[0];
            if((exit_status = handle_other_infile(n,filename,dz))<0)
                return(exit_status);
            (*cmdline)++;
            (*cmdlinecnt)--;
        }
    }
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

/********************************** DOPERM etc *******************************/

void doperm(int *perm,int permlen) 
{
    int n, t;
    for(n=0;n<permlen;n++) {
        t = (int)floor(drand48() * (n+1));
        if(t==n) {
            hprefix(n,perm,permlen);
        } else {
            hinsert(n,t,perm,permlen);
        }
    }
}

void hinsert(int m,int t,int *perm,int permlen)
{
    hshuflup(t+1,perm,permlen);
    perm[t+1] = m;
}

void hprefix(int m,int *perm,int permlen)
{
    hshuflup(0,perm,permlen);
    perm[0] = m;
}

void hshuflup(int k,int *perm,int permlen)
{
    int n, *i;
    int z = permlen - 1;
    i = perm+z;
    for(n = z;n > k;n--) {
        *i = *(i-1);
        i--;
    }
}

/********************************** ROTATE_VERTEX *******************************
 *
 *  If the original coordinates of a point (on a sphere) are x.y.z
 * then a rotation about the z-axis of X radians is given by matrix
 *
 *  cos(X)  sin(x)  0
 *  -sin(X) cos(X)  0
 *  0       0       1
 *
 * This creates new points (xx,yy,zz) thus
 *
 *  xx = cos(X)*x  + sin(X)*y + 0*z
 *  yy = -sin(X)*x + cos(X)*y + 0*z
 *  zz = 0*x       + 0*y      + 1*z
 *
 *
 *  For a rotation around the y axis of Y radians, matrix is
 *
 *  cos(Y)    0     sin(Y)
 *  0         1     0
 *  -sin(Y)   0     cos(Y)
 *
 * This creates new points (x',y',z') thus
 *
 *  x' = cos(Y)*xx  + 0*yy + sin(Y)*zz
 *  y' = 0*xx       + 1*yy + 0*zz
 *  z' = -sin(Y)*xx + 0*yy + cos(Y)*zz
 *
 *  X and Y pre-calculated from the angular rotation speeds, and timestep, and applied successively.
 */

void rotate_vertex(double *x,double *y,double *z,double rotation_in_xy_plane,double rotation_in_xz_plane,int vertexno,double eventtime,int *warning)
{
    double xx, yy, zz, adjust, vectorlen;

    //  Rotate around z axis - in xy plane

    xx =   cos(rotation_in_xy_plane) * (*x);
    xx +=  sin(rotation_in_xy_plane) * (*y);

    xx = min(xx,1.0);       //  Avoid rounding errors
    xx = max(xx,-1.0);

    yy =  -sin(rotation_in_xy_plane) * (*x);
    yy +=  cos(rotation_in_xy_plane) * (*y);

    yy = min(yy,1.0);
    yy = max(yy,-1.0);
    zz =  *z;

    //  Rotate around y axis - in xz plane

    *x =   cos(rotation_in_xz_plane) * xx;
    *x +=  sin(rotation_in_xz_plane) * zz;

    *x = min(*x,1.0);
    *x = max(*x,-1.0);

    *y = yy;

    *z =  -sin(rotation_in_xz_plane) * xx;
    *z +=  cos(rotation_in_xz_plane) * zz;

    *z = min(*z,1.0);
    *z = max(*z,-1.0);

    if((get_vectorlen(&vectorlen,*x,*y,*z))<0) {
        if(*warning == 0) {
            fprintf(stdout,"WARNING: rotated vector %d lies outside unit sphere at time %lf : coords %lf %lf %lf len %.16lf\n",
            vertexno+1,eventtime,*x,*y,*z,vectorlen);
            fflush(stdout);
            *warning = 1;
        }
        adjust = ((*x)*(*x)) + ((*y)*(*y)) + ((*z)*(*z));
        adjust = sqrt(adjust);
        adjust = 1.0/adjust;
        *x *= adjust;
        *y *= adjust;
        *z *= adjust;
    }
}

/*************************************** DOVETAIL **********************************/

void dovetail(int dovelen, dataptr dz)
{
    float *buf;
    double splice;
    int /* buflen,*/ i, j, n;
    for(n= 0; n < dz->infilecnt; n++) {
        buf = dz->sampbuf[THISIBUF+n];
 //       buflen = dz->insams[n] + 1;
        for(i = 0, j = dz->insams[n]; i < dovelen; i++,j--) {
            splice = (double)i/(double)dovelen;
            buf[i] = (float)(buf[i] * splice);
            buf[j] = (float)(buf[j] * splice);
        }
    }
}

/*************************************** WRITE_ROTATED_CRYSTAL_SOUND **********************************/

int write_rotated_crystal_sound(float *obuf,int maxwrite, dataptr dz)
{
    int exit_status;
    char temp[8];
    if(dz->outcnt > 0) {                                //  If not first file being written
        if((exit_status = headwrite(dz->ofd,dz))<0)     //  Conclude and close last file
            return(exit_status);
        if((exit_status = reset_peak_finder(dz))<0)
            return(exit_status);
        if(sndcloseEx(dz->ofd) < 0) {
            fprintf(stdout,"WARNING: Can't close output soundfile %s\n",dz->outfilename);
            fflush(stdout);
        }
        dz->ofd = -1;
        strcpy(dz->outfilename,dz->wordstor[0]);        //  Create name of this new file
        sprintf(temp,"%d",dz->outcnt);
        strcat(dz->outfilename,temp);
        strcat(dz->outfilename,".wav");                 //  Create new outfile      
        dz->infile->channels = 2;
        if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
            return exit_status;
        dz->infile->channels = 1;
    }
    dz->process = GREV;
    if((exit_status = write_samps(obuf,maxwrite,dz))<0) //  Write to outfile (whether a new file or not)
        return(exit_status);
    dz->process = CRYSTAL;
    memset((char *)obuf,0,dz->buflen * sizeof(float));  //  Reset the outputbuffer to zero, ready for next vertex-set
    dz->outcnt++;
    return FINISHED;
}
