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

// defined in science.h
// #define 

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


#ifdef unix
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int anal_infiles = 0;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";



//CDP LIB REPLACEMENTS
static int check_timeseries_param_validity_and_consistency(dataptr dz);
static int setup_timeseries_application(dataptr dz);
static int setup_timeseries_param_ranges_and_defaults(dataptr dz);
static void init_specsynth(dataptr dz);
static int allocate_timeseries_buffer(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
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
static int count_and_test_for_infiles(int argc,char *argv[],dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int redefine_textfile_types2(infileptr infile_info,dataptr dz);
static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int open_the_outfile(dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
//static void insert_harmonics(int **peaked,int *peakcnt,double harmamp,float fundamental,double pkwidth,dataptr dz);
//static void randomiseamps(int *perm,int *peaked,dataptr dz);
//static void rndintperm(int *perm,int cnt);
//static void spread_peaks(int *peaked,double spreaddnratio,double spreadupratio,dataptr dz);
static int  get_float_with_e_from_within_string(char **str,double *val);
static int spline(double *nums,double *secondderiv,double *x,double *y,dataptr dz);
static int splint(double pointer,double *thisval,int *hi,int arraycnt,double *secondderiv, double *x, double *y);
static int ts_oscil(dataptr dz);
static void find_mean(double *mean,double *top,double *bottom,dataptr dz);
static void scale_data_in_one_to_minusone_range(double mean,double top,double bottom,dataptr dz);
static int ts_trace(dataptr dz);
static double calcsample (double thisval,int sintabsize,double incrementor,dataptr dz);
static int  create_sintab (dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
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
        dz->maxmode = 0;
        if((get_the_process_no(argv[0],dz))<0)
            return(FAILED);
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_timeseries_application(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        if((exit_status = count_and_test_for_infiles(cmdlinecnt,cmdline,dz))<0) {
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
    init_specsynth(dz);
    if((exit_status = setup_timeseries_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // handle_infile() = 
    // open_first_infile        CDP LIB
    if((exit_status = open_first_infile(cmdline[0],dz))<0) {    
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);  
        return(FAILED);
    }
    cmdlinecnt--;
    cmdline++;
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(dz->process == TS_TRACE) {
        if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency....
    if((exit_status = check_timeseries_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    if((exit_status = open_the_outfile(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = allocate_timeseries_buffer(dz))<0) {                          // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    switch(dz->process) {
    case(TS_OSCIL):
        if((exit_status = ts_oscil(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    case(TS_TRACE):
        if((exit_status = ts_trace(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
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
//          if(!sloom)
//              fprintf(stderr,errstr);
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
//          if(!sloom)
//              fprintf(stderr,errstr);
            return(MEMORY_ERROR);
        }
        strcpy(ap->option_list,optlist);
        if((ap->option_flags = (char *)malloc((size_t)(optcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for option_flags\n");
//          if(!sloom)
//              fprintf(stderr,errstr);
            return(MEMORY_ERROR);
        }
        strcpy(ap->option_flags,optflags); 
    }
    ap->vflag_cnt = (char) vflagcnt;           
    ap->variant_param_cnt = (char) vparamcnt;
    if(vflagcnt) {
        if((ap->variant_list  = (char *)malloc((size_t)(vflagcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for variant_list\n");
//          if(!sloom)
//              fprintf(stderr,errstr);
            return(MEMORY_ERROR);
        }
        strcpy(ap->variant_list,varlist);       
        if((ap->variant_flags = (char *)malloc((size_t)(vflagcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY: for variant_flags\n");
//          if(!sloom)
//              fprintf(stderr,errstr);
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

    if(dz->input_data_type==UNRANGED_BRKFILE_ONLY) {
        dz->extrabrkno = brkcnt;              
        brkcnt++;       /* create brktable poniter for param0, and use point to and read (parray) input data during process */
    } else
        brkcnt++;
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
    if((exit_status = establish_bufptrs_and_extra_buffers(dz))<0)   
        return(exit_status);
    if((exit_status = setup_internal_arrays_and_array_pointers(dz))<0)   
        return(exit_status);
    return(FINISHED);
}

/******************************** SETUP_AND_INIT_INPUT_BRKTABLE_CONSTANTS ********************************/

int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt)
{   
    int n;
    if((dz->brk      = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 1\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(MEMORY_ERROR);
    }
    if((dz->brkptr   = (double **)malloc(brkcnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 6\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(MEMORY_ERROR);
    }
    if((dz->brksize  = (int    *)malloc(brkcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 2\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(MEMORY_ERROR);
    }
    if((dz->firstval = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 3\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(MEMORY_ERROR);                                                 
    }
    if((dz->lastind  = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 4\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(MEMORY_ERROR);
    }
    if((dz->lastval  = (double  *)malloc(brkcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 5\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(MEMORY_ERROR);
    }
    if((dz->brkinit  = (int     *)malloc(brkcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 7\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
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
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(MEMORY_ERROR);
    }
    if((dz->iparam      = (int    *)calloc(storage_cnt, sizeof(int)   ))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 2\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(MEMORY_ERROR);
    }
    if((dz->is_int      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 3\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(MEMORY_ERROR);
    }
    if((dz->no_brk      = (char   *)calloc(storage_cnt, sizeof(char)))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 5\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
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
//          if(!sloom)
//              fprintf(stderr,errstr);
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
//          if(!sloom)
//              fprintf(stderr,errstr);
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
//          if(!sloom)
//              fprintf(stderr,errstr);
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
//          if(!sloom)
//              fprintf(stderr,errstr);
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
//          if(!sloom)
//              fprintf(stderr,errstr);
            return(DATA_ERROR);
        }
    }
    strcpy(dz->outfilename,filename);      
    (*cmdline)++;
    (*cmdlinecnt)--;
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
//      if(!sloom)
//          fprintf(stderr,errstr);
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
//      if(!sloom)
//          fprintf(stderr,errstr);
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
//      if(!sloom)
//          fprintf(stderr,errstr);
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
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(MEMORY_ERROR);
    }
    for(n=0;n<tipc;n++)
        dz->is_active[n] = (char)0;
    return(FINISHED);
}

/************************* SETUP_TIMESERIES_APPLICATION *******************/

int setup_timeseries_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    dz->bufcnt = 1;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    switch(dz->process) {
    case(TS_OSCIL):
        if((exit_status = set_param_data(ap,0   ,1,1,"D"))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"d",1,"d","cf",2,0,"00"))<0)
            return(FAILED);
        dz->input_data_type = NUMLIST_ONLY;
        dz->process_type    = UNEQUAL_SNDFILE;  
        dz->outfiletype     = SNDFILE_OUT;
        break;
    case(TS_TRACE):
        if((exit_status = set_param_data(ap,TS_HARM   ,3,3,"Ddd"))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"d",1,"d","cf",2,0,"00"))<0)
            return(FAILED);
        dz->input_data_type = NUMLIST_ONLY;
        dz->process_type    = UNEQUAL_SNDFILE;  
        dz->outfiletype     = SNDFILE_OUT;
        break;
    }
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    return application_init(dz);    //GLOBAL
}

/************************* SETUP_TIMESERIES_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_timeseries_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    switch(dz->process) {
    case(TS_OSCIL):
        ap->lo[TS_TSTRETCH] = 0;
        ap->hi[TS_TSTRETCH] = TS_MAXOCT;
        ap->default_val[TS_TSTRETCH] = 0;
        ap->lo[TS_OMAXDUR] = 1;
        ap->hi[TS_OMAXDUR] = 60;
        ap->default_val[TS_OMAXDUR] = 10;
        break;
    case(TS_TRACE):
        ap->lo[TS_TSTRETCH] = 1;
        ap->hi[TS_TSTRETCH] = TS_MAXTSTRETCH;
        ap->default_val[TS_TSTRETCH] = 0;
        ap->lo[TS_FRQ] = TS_MINFRQ;
        ap->hi[TS_FRQ] = dz->nyquist/2.0;
        ap->default_val[TS_FRQ] = 440.0;
        ap->lo[TS_HALFRANGE] = 0;
        ap->hi[TS_HALFRANGE] = TS_MAXRANGE;
        ap->default_val[TS_HALFRANGE] = TS_DFLTRANGE;
        ap->lo[TS_TMAXDUR] = 0;
        ap->hi[TS_TMAXDUR] = 600;
        ap->default_val[TS_TMAXDUR] = 0;
        break;
    }
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
    aplptr ap;

    while(cnt<=PRE_CMDLINE_DATACNT) {
        if(cnt > argc) {
            sprintf(errstr,"Insufficient data sent from TK\n");
//          if(!sloom)
//              fprintf(stderr,errstr);
            return(DATA_ERROR);
        }
        switch(cnt) {
        case(1):    
            if(sscanf(argv[cnt],"%d",&dz->process)!=1) {
                sprintf(errstr,"Cannot read process no. sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;

        case(2):    
            if(sscanf(argv[cnt],"%d",&dz->mode)!=1) {
                sprintf(errstr,"Cannot read mode no. sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            if(dz->mode > 0)
                dz->mode--;
            //setup_particular_application() =
            if((exit_status = setup_timeseries_application(dz))<0)
                return(exit_status);
            ap = dz->application;
            break;

        case(3):    
            if(sscanf(argv[cnt],"%d",&infilecnt)!=1) {
                sprintf(errstr,"Cannot read infilecnt sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
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
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_FILESIZE+4): 
            if(sscanf(argv[cnt],"%d",&filesize)!=1) {
                sprintf(errstr,"Cannot read infilesize sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            dz->insams[0] = filesize;   
            break;
        case(INPUT_INSAMS+4):   
            if(sscanf(argv[cnt],"%d",&insams)!=1) {
                sprintf(errstr,"Cannot read insams sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            dz->insams[0] = insams; 
            break;
        case(INPUT_SRATE+4):    
            if(sscanf(argv[cnt],"%d",&dz->infile->srate)!=1) {
                sprintf(errstr,"Cannot read srate sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_CHANNELS+4): 
            if(sscanf(argv[cnt],"%d",&dz->infile->channels)!=1) {
                sprintf(errstr,"Cannot read channels sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_STYPE+4):    
            if(sscanf(argv[cnt],"%d",&dz->infile->stype)!=1) {
                sprintf(errstr,"Cannot read stype sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_ORIGSTYPE+4):    
            if(sscanf(argv[cnt],"%d",&dz->infile->origstype)!=1) {
                sprintf(errstr,"Cannot read origstype sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_ORIGRATE+4): 
            if(sscanf(argv[cnt],"%d",&dz->infile->origrate)!=1) {
                sprintf(errstr,"Cannot read origrate sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_MLEN+4): 
            if(sscanf(argv[cnt],"%d",&dz->infile->Mlen)!=1) {
                sprintf(errstr,"Cannot read Mlen sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_DFAC+4): 
            if(sscanf(argv[cnt],"%d",&dz->infile->Dfac)!=1) {
                sprintf(errstr,"Cannot read Dfac sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_ORIGCHANS+4):    
            if(sscanf(argv[cnt],"%d",&dz->infile->origchans)!=1) {
                sprintf(errstr,"Cannot read origchans sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_SPECENVCNT+4):   
            if(sscanf(argv[cnt],"%d",&dz->infile->specenvcnt)!=1) {
                sprintf(errstr,"Cannot read specenvcnt sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            dz->specenvcnt = dz->infile->specenvcnt;
            break;
        case(INPUT_WANTED+4):   
            if(sscanf(argv[cnt],"%d",&dz->wanted)!=1) {
                sprintf(errstr,"Cannot read wanted sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_WLENGTH+4):  
            if(sscanf(argv[cnt],"%d",&dz->wlength)!=1) {
                sprintf(errstr,"Cannot read wlength sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_OUT_CHANS+4):    
            if(sscanf(argv[cnt],"%d",&dz->out_chans)!=1) {
                sprintf(errstr,"Cannot read out_chans sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
            /* RWD these chanegs to samps - tk will have to deal with that! */
        case(INPUT_DESCRIPTOR_BYTES+4): 
            if(sscanf(argv[cnt],"%d",&dz->descriptor_samps)!=1) {
                sprintf(errstr,"Cannot read descriptor_samps sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_IS_TRANSPOS+4):  
            if(sscanf(argv[cnt],"%d",&dz->is_transpos)!=1) {
                sprintf(errstr,"Cannot read is_transpos sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_COULD_BE_TRANSPOS+4):    
            if(sscanf(argv[cnt],"%d",&dz->could_be_transpos)!=1) {
                sprintf(errstr,"Cannot read could_be_transpos sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_COULD_BE_PITCH+4):   
            if(sscanf(argv[cnt],"%d",&dz->could_be_pitch)!=1) {
                sprintf(errstr,"Cannot read could_be_pitch sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_DIFFERENT_SRATES+4): 
            if(sscanf(argv[cnt],"%d",&dz->different_srates)!=1) {
                sprintf(errstr,"Cannot read different_srates sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_DUPLICATE_SNDS+4):   
            if(sscanf(argv[cnt],"%d",&dz->duplicate_snds)!=1) {
                sprintf(errstr,"Cannot read duplicate_snds sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_BRKSIZE+4):  
            if(sscanf(argv[cnt],"%d",&inbrksize)!=1) {
                sprintf(errstr,"Cannot read brksize sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            if(inbrksize > 0) {
                switch(dz->input_data_type) {
                case(WORDLIST_ONLY):
                case(NUMLIST_ONLY):
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
//                      if(!sloom)
//                          fprintf(stderr,errstr);
                        return(DATA_ERROR);
                    }
                    if(dz->brksize == NULL) {
                        sprintf(errstr,"CDP has not established storage space for input brktable.\n");
//                      if(!sloom)
//                          fprintf(stderr,errstr);
                        return(PROGRAM_ERROR);
                    }
                    dz->brksize[dz->extrabrkno] = inbrksize;
                    break;
                default:
                    sprintf(errstr,"TK sent brktablesize > 0 for input_data_type [%d] not using brktables.\n",dz->input_data_type);
//                  if(!sloom)
//                      fprintf(stderr,errstr);
                    return(PROGRAM_ERROR);
                }
                break;
            }
            break;
        case(INPUT_NUMSIZE+4):  
            if(sscanf(argv[cnt],"%d",&dz->numsize)!=1) {
                sprintf(errstr,"Cannot read numsize sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_LINECNT+4):  
            if(sscanf(argv[cnt],"%d",&dz->linecnt)!=1) {
                sprintf(errstr,"Cannot read linecnt sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_ALL_WORDS+4):    
            if(sscanf(argv[cnt],"%d",&dz->all_words)!=1) {
                sprintf(errstr,"Cannot read all_words sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_ARATE+4):    
            if(sscanf(argv[cnt],"%f",&dz->infile->arate)!=1) {
                sprintf(errstr,"Cannot read arate sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_FRAMETIME+4):    
            if(sscanf(argv[cnt],"%lf",&dummy)!=1) {
                sprintf(errstr,"Cannot read frametime sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            dz->frametime = (float)dummy;
            break;
        case(INPUT_WINDOW_SIZE+4):  
            if(sscanf(argv[cnt],"%f",&dz->infile->window_size)!=1) {
                sprintf(errstr,"Cannot read window_size sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                    return(DATA_ERROR);
            }
            break;
        case(INPUT_NYQUIST+4):  
            if(sscanf(argv[cnt],"%lf",&dz->nyquist)!=1) {
                sprintf(errstr,"Cannot read nyquist sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_DURATION+4): 
            if(sscanf(argv[cnt],"%lf",&dz->duration)!=1) {
                sprintf(errstr,"Cannot read duration sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_MINBRK+4):   
            if(sscanf(argv[cnt],"%lf",&dz->minbrk)!=1) {
                sprintf(errstr,"Cannot read minbrk sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_MAXBRK+4):   
            if(sscanf(argv[cnt],"%lf",&dz->maxbrk)!=1) {
                sprintf(errstr,"Cannot read maxbrk sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_MINNUM+4):   
            if(sscanf(argv[cnt],"%lf",&dz->minnum)!=1) {
                sprintf(errstr,"Cannot read minnum sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        case(INPUT_MAXNUM+4):   
            if(sscanf(argv[cnt],"%lf",&dz->maxnum)!=1) {
                sprintf(errstr,"Cannot read maxnum sent from TK\n");
//              if(!sloom)
//                  fprintf(stderr,errstr);
                return(DATA_ERROR);
            }
            break;
        default:
            sprintf(errstr,"case switch item missing: parse_sloom_data()\n");
//          if(!sloom)
//              fprintf(stderr,errstr);
            return(PROGRAM_ERROR);
        }
        cnt++;
    }
    if(cnt!=PRE_CMDLINE_DATACNT+1) {
        sprintf(errstr,"Insufficient pre-cmdline params sent from TK\n");
//      if(!sloom)
//          fprintf(stderr,errstr);
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
//          if(!sloom)
//              fprintf(stderr,errstr);
            return(MEMORY_ERROR);
        }
    } else {
        if((*cmdline = (char **)realloc(*cmdline,((*cmdlinecnt)+1) * sizeof(char *)))==NULL)    {
            sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline array.\n");
//          if(!sloom)
//              fprintf(stderr,errstr);
            return(MEMORY_ERROR);
        }
    }
    if(((*cmdline)[*cmdlinecnt] = (char *)malloc((strlen(q) + 1) * sizeof(char)))==NULL)    {
        sprintf(errstr,"INSUFFICIENT MEMORY for TK cmdline item %d.\n",(*cmdlinecnt)+1);
//      if(!sloom)
//          fprintf(stderr,errstr);
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

/************************* SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS *******************/

int setup_internal_arrays_and_array_pointers(dataptr dz)
{
    int n;
    dz->array_cnt = 3;
    if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->array_cnt;n++)
        dz->parray[n] = NULL;
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

int establish_bufptrs_and_extra_buffers(dataptr dz)
{
    dz->extra_bufcnt = 0;
    dz->bptrcnt = 1;
    dz->bufcnt  = 1;
    return establish_groucho_bufptrs_and_extra_buffers(dz);
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
    fprintf(stderr,
    "\nCREATE SOUND FROM TIME-SERIES TEXT DATA\n\n"
    "USAGE: ts NAME parameters\n"
    "\n"
    "where NAME can be any one of\n"
    "\n"
    "oscil trace\n\n"
    "Type 'ts oscil' for more info on ts oscil..ETC.\n");
    return(USAGE_ONLY);
}

/**************************** CHECK_TIMESERIES_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_timeseries_param_validity_and_consistency(dataptr dz)
{
    int exit_status;
    double centrefrq, maxtranspos, maxfrq, maxharm, brkmax;
    int maxpos;
    if(dz->process == TS_TRACE) { 
        if(dz->brksize[TS_FRQ] > 0) {
            if ((exit_status = get_maxvalue_in_brktable(&brkmax,TS_FRQ,dz)) < 0)
                return(exit_status);
            centrefrq = brkmax;
        } else 
            centrefrq = dz->param[TS_FRQ];
        if(dz->brksize[TS_HALFRANGE] > 0) {
            if ((exit_status = get_maxvalue_in_brktable(&brkmax,TS_HALFRANGE,dz)) < 0)
                return(exit_status);
            maxtranspos = pow(2.0,(brkmax/12.0));
        } else 
            maxtranspos = pow(2.0,(dz->param[TS_HALFRANGE]/12.0));
        maxfrq = centrefrq * maxtranspos;
        maxharm = dz->scalefact;
        maxfrq *= maxharm;
        if(maxfrq >= dz->nyquist) {
            maxpos = (int)floor(dz->nyquist/(centrefrq * maxtranspos));
            if(maxpos < 1)
                sprintf(errstr,"All output too high for (max) frq and range.");
            else
                sprintf(errstr,"Max harmonic (%d) too high for (max) frq and range.\nMax possible harmonic = %d\n",(int) round(maxharm),maxpos);   //RWD added cast
            return(DATA_ERROR);
        }
        if(dz->param[TS_TMAXDUR] <= 0.0) {
            if(dz->vflag[1]) {
                sprintf(errstr,"Loop-forcing flag (-f) set, but no max duration given.");
                return(DATA_ERROR);
            }
            dz->wanted = 0;
        }
        else if(dz->param[TS_TMAXDUR] < 1.0) {
            sprintf(errstr,"Maximum duration must be >= 1 second\n");
            return(DATA_ERROR);
        } else
            dz->wanted = (int)round(dz->param[TS_TMAXDUR] * SPEKSR);
    } else {
        if(!dz->brksize[TS_TSTRETCH])
            dz->param[TS_TSTRETCH] = pow(2.0,dz->param[TS_TSTRETCH]);   /* convert octaves to frq(time) ratio, for "oscil" */
        if(dz->param[TS_OMAXDUR] <= 0.0)
            dz->wanted = 0;
        else if(dz->param[TS_OMAXDUR] < 1.0) {
            sprintf(errstr,"Maximum duration must be >= 1 second\n");
            return(DATA_ERROR);
        } else
            dz->wanted = (int)round(dz->param[TS_OMAXDUR] * SPEKSR);
    }
    return(FINISHED);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"oscil"))                   dz->process = TS_OSCIL;
    else if(!strcmp(prog_identifier_from_cmdline,"trace"))              dz->process = TS_TRACE;
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
//      if(!sloom)
//          fprintf(stderr,errstr);
        return(USAGE_ONLY);
    }
    return(FINISHED);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"oscil")) {
        fprintf(stderr,
        "USAGE: ts oscil indata outsnd downsample [-dmaxdur] [-c] [-f]\n"
        "\n"
        "Treat data in time-series format (list of numbers) as a soundwave plot.\n"
        "\n"
        "INDATA     text data as a list of numerical values.\n"
        "OUTSOUND   sound output file.\n"
        "DOWNSAMPLE downward transposition of data in octaves (0 - %d)\n"
        "           (= timestretch as a power of two)\n" 
        "           Can vary over time (time values are times in output sound).\n" 
        "MAXDUR     max duration of output (range 1 - 600).\n"
        "-c         Interpolate downsampled data using cubic spline (default linear).\n" 
        "-f         Force duration to 'maxdur' (if ness) by looping input data.\n" 
        "           (invalid flag if no 'maxdur' specified).\n"
        "\n",TS_MAXOCT);
    } else if(!strcmp(str,"trace")) {
        fprintf(stderr,
        "USAGE: ts trace indata outsnd harmdata tstr frq hrange [-dmaxdur] [-c] [-f]\n"
        "\n"
        "Treat data in time-series format (list of numbers) as the pitch-trace\n"
        "of some defined waveform.\n"
        "\n"
        "INDATA     text data as a list of numerical values.\n"
        "OUTSOUND   sound output file.\n"
        "HARMDATA   textfile: list of number pairs = 'harmonic-number amplitude'\n"
        "           for each harmonic in waveform. \n"
        "           Fractional values (> 1) will generate inharmonic spectra.\n"
        "           '0' (zero), instead of a filename, will output a pure sinetone.\n"
        "TSTR       time stretch of data (range 1 - %d).\n"
        "           Can vary over time (time values are times in output sound).\n" 
        "FRQ        frequency of the mean pitch of the output (range %.0lf - %d).\n"
        "HRANGE     pitch range upwards (or down) from mean, in semitones (range 0 - %d).\n"
        "MAXDUR     max duration of output (range 1 - 600).\n"
        "-c         Interpolate time-stretched data using cubic spline (default linear).\n" 
        "-f         Force duration to 'maxdur' (if ness) by looping input data.\n" 
        "           (invalid flag if no 'maxdur' specified).\n"
        "\n",TS_MAXTSTRETCH,TS_MINFRQ,SPEKSR/4,TS_MAXRANGE);
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/******************************** INIT_SPECSYNTH (redundant)  ********************************/

void init_specsynth(dataptr dz)
{
    dz->outfile->stype      = dz->infile->stype     = SAMP_SHORT;
    dz->outfile->channels   = dz->infile->channels  = 1;
    dz->outfile->srate = dz->infile->srate = SPEKSR;
    dz->nyquist = (double)(SPEKSR / 2.0);       
    dz->needpeaks = 0;
}

/**************************** ALLOCATE_TIMESERIES_BUFFER ******************************/

int allocate_timeseries_buffer(dataptr dz)
{
    int exit_status;
    dz->bufcnt = 1;
    exit_status = create_sndbufs(dz);
    return(exit_status);
}

/************************** HANDLE_THE_SPECIAL_DATA **********************************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int cnt, is_harmno, isfile = 1, m, n;
    char *filename = (*cmdline)[0];
    FILE *fp = NULL;
    double *p, dummy, maxharmno = -1.0;
    char temp[200], *q;
    if(!strcmp(filename,"0")) {
        cnt = 2;
        isfile = 0;
    } else {
        if((fp = fopen(filename,"r"))==NULL) {
            sprintf(errstr, "Can't open data file %s to read data.\n",filename);
            return(DATA_ERROR);
        }
        cnt = 0;
        p = &dummy;
        while(fgets(temp,200,fp)==temp) {
            q = temp;
            if(*q == ';')   //  Allow comments in file
                continue;
            while(get_float_with_e_from_within_string(&q,p))
                cnt++;
        }
        if(cnt == 0) {
            sprintf(errstr,"No data in data file %s\n",filename);
            return(DATA_ERROR);
        }
        if((cnt % 2) != 0) {
            sprintf(errstr,"data in file %s not paired correctly.\n",filename);
            return(DATA_ERROR);
        }
    }
    dz->itemcnt = 0;
    if((dz->parray[TS_HARMONICS] = (double *)malloc(cnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for input data in file %s.\n",filename);
        return(MEMORY_ERROR);
    }
    if(!isfile) {
        dz->parray[TS_HARMONICS][0] = 1.0;
        dz->parray[TS_HARMONICS][1] = 1.0;
        dz->itemcnt = 1;
        dz->scalefact = 1.0;
    } else {
        fseek(fp,0,0);
        p = dz->parray[TS_HARMONICS];
        is_harmno = 1;
        while(fgets(temp,200,fp)==temp) {
            q = temp;
            if(*q == ';')   //  Allow comments in file
                continue;
            while(get_float_with_e_from_within_string(&q,p)) {
                dummy = *p;
                if(is_harmno) {
                    if(dummy < 1) {
                        sprintf(errstr,"Invalid harmonic number (%lf) in file %s (min = 1)\n",dummy,filename);
                        return(DATA_ERROR);
                    }
                    if(dummy > maxharmno)
                        maxharmno = dummy;
                } else {
                    if((dummy <= 0.0) || (dummy > 1.0)) {
                        sprintf(errstr,"harmonic amplitude (%lf) out of range (>0-1) in file %s.\n",dummy,filename);
                        return(DATA_ERROR);
                    }
                    dz->itemcnt++;
                }
                is_harmno = !is_harmno;
                p++;
            }
        }
        if(fclose(fp)<0) {
            fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
            fflush(stdout);
        }
        dz->scalefact = maxharmno;
        for(n = 0; n < dz->itemcnt-1; n++) {
            m = n+1;
            while(m < dz->itemcnt) {
                if(flteq(dz->parray[TS_HARMONICS][n*2],dz->parray[TS_HARMONICS][m*2])) {
                    sprintf(errstr,"Duplicated harmonic number (%lf) in file %s.\n",dz->parray[TS_HARMONICS][n*2], filename);  //RWD added filename arg - correct?
                    return(DATA_ERROR);
                }
                m++;
            }
        }
    }
    if((dz->fbandbot = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {         //  Stores pointers into sintable
        sprintf(errstr,"INSUFFICIENT MEMORY to create harmonics-reading pointers.\n");
        return(MEMORY_ERROR);
    }
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************** GET_FLOAT_WITH_E_FROM_WITHIN_STRING **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

int  get_float_with_e_from_within_string(char **str,double *val)
{
    char   *p, *valstart;
    int    decimal_point_cnt = 0, has_digits = 0, has_e = 0, lastchar = 0;
    p = *str;
    while(isspace(*p))
        p++;
    valstart = p;   
    switch(*p) {
    case('-'):                      break;
    case('.'): decimal_point_cnt=1; break;
    default:
        if(!isdigit(*p))
            return(FALSE);
        has_digits = TRUE;
        break;
    }
    p++;
    while(!isspace(*p) && *p!=NEWLINE && *p!=ENDOFSTR) {
        if(isdigit(*p))
            has_digits = TRUE;
        else if(*p == 'e') {
            if(has_e || !has_digits)
                return(FALSE);
            has_e = 1;
        } else if(*p == '-') {
            if(!has_e || (lastchar != 'e'))
                return(FALSE);
        } else if(*p == '.') {
            if(has_e || (++decimal_point_cnt>1))
                return(FALSE);
        } else
            return(FALSE);
        lastchar = *p;
        p++;
    }
    if(!has_digits || sscanf(valstart,"%lf",val)!=1)
        return(FALSE);
    *str = p;
    return(TRUE);
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
        } else if((exit_status = redefine_textfile_types2(infile_info,dz))<0) {
            sprintf(errstr,"Unknown data type in file %s\n",cmdline[0]);
            return(DATA_ERROR);
        } else if(infile_info->filetype != NUMLIST)  {
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

/************************************* REDEFINE_TEXTFILE_TYPES2 ******************************/

int redefine_textfile_types2(infileptr infile_info,dataptr dz)
{
    switch(infile_info->filetype) {
    case(TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(TRANSPOS_OR_NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(TRANSPOS_OR_PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(TRANSPOS_OR_UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(NORMD_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(NORMD_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(DB_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(DB_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(PITCH_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(PITCH_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(PITCH_POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(UNRANGED_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(UNRANGED_BRKFILE_OR_NUMLIST_OR_WORDLIST):
    case(NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(NUMLIST_OR_WORDLIST):
    case(POSITIVE_BRKFILE):
    case(POSITIVE_BRKFILE_OR_NUMLIST_OR_LINELIST_OR_WORDLIST):
    case(POSITIVE_BRKFILE_OR_NUMLIST_OR_WORDLIST):
        switch(dz->process) {
        case(TS_OSCIL):
        case(TS_TRACE):
            infile_info->filetype = NUMLIST;
            break;
        }
        break;
    default:
        sprintf(errstr,"Unknown input textfile type\n");
        return(PROGRAM_ERROR);
    }
    return FINISHED;
}

/************************************* TS_OSCIL ******************************/

int ts_oscil(dataptr dz)
{
    int exit_status;
    double mean, top, bottom, numend, pointer, step = 0.0, thisval, jj, val, valstep, atten, maxsamp, time, firststep = 1.0;
    double *secondderiv = NULL, *x = NULL, *y = NULL, *nums;
    int n, sampcnt, totsamps, hi, ii, samps_to_write;
    float *obuf;
    obuf = dz->sampbuf[0];
    nums = dz->parray[TS_DATA];
    numend = (double)(dz->numsize - 1);
    find_mean(&mean,&top,&bottom,dz);
    scale_data_in_one_to_minusone_range(mean,top,bottom,dz);

    if(dz->brksize[TS_TSTRETCH] || dz->param[TS_TSTRETCH] > 1) {
        sampcnt = 0;
        if(dz->brksize[TS_TSTRETCH]) {
            if((exit_status = read_value_from_brktable(0.0,TS_TSTRETCH,dz))<0)
                return(exit_status);
            dz->param[TS_TSTRETCH] = pow(2.0,dz->param[TS_TSTRETCH]);   
            firststep = 1.0/dz->param[TS_TSTRETCH];
        } else
            step = 1.0/dz->param[TS_TSTRETCH];
        if (dz->vflag[0]) {

            // Cubic spline arrays 

            if((secondderiv = (double *)malloc(dz->numsize * sizeof(double)))==NULL) {
                fprintf(stdout,"INSUFFICIENT MEMORY for storing slope of input data.\n");
                fflush(stdout);
                return(MEMORY_ERROR);
            }
            if((x = (double *)malloc(dz->numsize * sizeof(double)))==NULL) {
                fprintf(stdout,"INSUFFICIENT MEMORY for storing x-coords of input data.\n");
                fflush(stdout);
                return(MEMORY_ERROR);
            }
            if((y = (double *)malloc(dz->numsize * sizeof(double)))==NULL) {
                fprintf(stdout,"INSUFFICIENT MEMORY for storing y-coords of input data.\n");
                fflush(stdout);
                return(MEMORY_ERROR);
            }
            if((exit_status = spline(nums,secondderiv,x,y,dz))<0)
                return(exit_status);

            // Test level, in case splining causes oscillator to curve out of range
            if(dz->brksize[TS_TSTRETCH])
                step = firststep;
            pointer = step;
            hi = 0;
            atten = 1.0;
            maxsamp = 0.0;
            while(pointer < numend) {
                if((exit_status = splint(pointer,&thisval,&hi,dz->numsize,secondderiv,x,y))<0)
                    return(FAILED);
                if(fabs(thisval) > maxsamp)
                    maxsamp = fabs(thisval);
                pointer += step;
            }
            if(maxsamp > TS_MAXLEVEL)
                atten = TS_MAXLEVEL/maxsamp;
        
            for(;;) {
                totsamps = 0;
                if(dz->brksize[TS_TSTRETCH])
                    step = firststep;
                obuf[sampcnt++] = 0.0f;
                totsamps++;
                hi = 0;
                while(pointer < numend) {
                    if((exit_status = splint(pointer,&thisval,&hi,dz->numsize,secondderiv,x,y))<0)
                        return(FAILED);
                    obuf[sampcnt++] = (float)(thisval * atten);
                    if(sampcnt >= dz->buflen) {
                        if((dz->wanted > 0) && dz->wanted <= dz->total_samps_written + dz->buflen) {
                            samps_to_write = dz->wanted - dz->total_samps_written;
                            if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
                                return(exit_status);
                            return FINISHED;
                        }
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        sampcnt = 0;
                    }
                    if(dz->brksize[TS_TSTRETCH] && (totsamps % 256 == 0)) {
                        time = (double)totsamps/(double)dz->infile->srate;
                        if((exit_status = read_value_from_brktable(time,TS_TSTRETCH,dz))<0)
                            return(exit_status);
                        dz->param[TS_TSTRETCH] = pow(2.0,dz->param[TS_TSTRETCH]);   
                        step = 1.0/dz->param[TS_TSTRETCH];
                    }
                    totsamps++;
                    pointer += step;
                }
                if(!dz->vflag[1] || (dz->total_samps_written + sampcnt >= dz->wanted))
                    break;
            }       
        } else {    // LINEAR INTERP
            for(;;) {
                totsamps = 0;   
                if(dz->brksize[TS_TSTRETCH])
                    step = firststep;
                pointer = 0.0;
                while (pointer < numend) {
                    ii = (int)floor(pointer);
                    jj = pointer - (double)ii;
                    val = nums[ii];
                    valstep = nums[ii+1] - nums[ii];
                    valstep *= jj;
                    obuf[sampcnt++] = (float)(val + valstep);
                    if(sampcnt >= dz->buflen) {
                        if((dz->wanted > 0) && dz->wanted <= dz->total_samps_written + dz->buflen) {
                            samps_to_write = dz->wanted - dz->total_samps_written;
                            if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
                                return(exit_status);
                            return FINISHED;
                        }
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        sampcnt = 0;
                    }
                    if(dz->brksize[TS_TSTRETCH] && (totsamps % 256 == 0)) {
                        time = (double)totsamps/(double)dz->infile->srate;
                        if((exit_status = read_value_from_brktable(time,TS_TSTRETCH,dz))<0)
                            return(exit_status);
                        dz->param[TS_TSTRETCH] = pow(2.0,dz->param[TS_TSTRETCH]);   
                        step = 1.0/dz->param[TS_TSTRETCH];
                    }
                    totsamps++;
                    pointer += step;
                }
                if(!dz->vflag[1] || (dz->total_samps_written + sampcnt >= dz->wanted))
                    break;
            }       
        }
    } else {    //  NO TIMESTRETCH
        sampcnt = 0;
        for(;;) {
            for(n=0;n < dz->numsize;n++) {
                if(sampcnt >= dz->buflen) {
                    if((dz->wanted > 0) && dz->wanted <= dz->total_samps_written + dz->buflen) {
                        samps_to_write = dz->wanted - dz->total_samps_written;
                        if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
                            return(exit_status);
                        return FINISHED;
                    }
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    sampcnt = 0;
                }
                obuf[sampcnt++] = (float)nums[n];
            }
            if(!dz->vflag[1] || (dz->total_samps_written + sampcnt >= dz->wanted))
                break;
        }       
    }
    if(sampcnt > 0) {
        if((dz->wanted > 0) && dz->wanted < dz->total_samps_written + sampcnt)
            sampcnt = dz->wanted - dz->total_samps_written;
        if((exit_status = write_samps(obuf,sampcnt,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/************************************* TS_TRACE ******************************/

int ts_trace(dataptr dz)
{
    int exit_status, h, dotest = 0;
    double mean, top, bottom, numend, pointer, step = 0.0, thisval, jj, val, valstep, time, firststep = 1.0;
    double outval, maxoutval, atten, midpitch;
    double *secondderiv = NULL, *x = NULL, *y = NULL, *nums;
    int n, sampcnt, hi, ii, samps_to_write, totsamps;
    float *obuf;
    double incrementor = (double)TS_SINTABSIZE/(double)dz->infile->srate;   // step forward in sintable, at 1 Hz
    
    if((exit_status = create_sintab(dz)) < 0)
        return(exit_status);
    
    obuf = dz->sampbuf[0];
    nums = dz->parray[TS_DATA];
    numend = (double)(dz->numsize - 1);
    find_mean(&mean,&top,&bottom,dz);
                                            //  Scale data range to pitch-range specified 
    scale_data_in_one_to_minusone_range(mean,top,bottom,dz);
    for(n = 0;n < dz->numsize; n++)
        nums[n] *= dz->param[TS_HALFRANGE];
    midpitch = unchecked_hztomidi(dz->param[TS_FRQ]);
    for(n = 0;n < dz->numsize; n++)
        nums[n] += midpitch;
    
    if(dz->brksize[TS_TSTRETCH] || (dz->param[TS_TSTRETCH] > 1)) {
        if(dz->brksize[TS_TSTRETCH]) {
            if((exit_status = read_value_from_brktable(0.0,TS_TSTRETCH,dz))<0)
                return(exit_status);
            dz->param[TS_TSTRETCH] = pow(2.0,dz->param[TS_TSTRETCH]);   
            firststep = 1.0/dz->param[TS_TSTRETCH];
        } else
            step = 1.0/dz->param[TS_TSTRETCH];

        if (dz->vflag[0]) {

            // Set up arrays and 2nd derivatives, for cubic spline

            if((secondderiv = (double *)malloc(dz->numsize * sizeof(double)))==NULL) {
                fprintf(stdout,"INSUFFICIENT MEMORY for storing slope of input data.\n");
                fflush(stdout);
                return(MEMORY_ERROR);
            }
            if((x = (double *)malloc(dz->numsize * sizeof(double)))==NULL) {
                fprintf(stdout,"INSUFFICIENT MEMORY for storing x-coords of input data.\n");
                fflush(stdout);
                return(MEMORY_ERROR);
            }
            if((y = (double *)malloc(dz->numsize * sizeof(double)))==NULL) {
                fprintf(stdout,"INSUFFICIENT MEMORY for storing y-coords of input data.\n");
                fflush(stdout);
                return(MEMORY_ERROR);
            }
            if((exit_status = spline(nums,secondderiv,x,y,dz))<0)
                return(exit_status);

            find_mean(&mean,&top,&bottom,dz);   // Find range of expanded data

            totsamps = 0;
            if(dz->brksize[TS_TSTRETCH])
                step = firststep;
            pointer = step; //  run cubic spline to test new top frq is within range
            hi = 0;
            while(pointer < numend) {
                if((exit_status = splint(pointer,&thisval,&hi,dz->numsize,secondderiv,x,y))<0)
                    return(FAILED);
                if(thisval > top) {
                    top = thisval;
                    dotest = 1;
                }
                pointer += step;
            }
            if(dotest) {
                top = miditohz(top);
                if(top * dz->scalefact >= dz->nyquist) {    // scalefact = max harmonic number
                    fprintf(stdout,"CUBIC SPLINING adjustment sends highest harmonics beyond nyquist.\n");
                    fflush(stdout);
                    return(DATA_ERROR);
                }
            }
        }
    }

    for(h = 0; h < dz->itemcnt;h++)     //  Preset-to-zero harmonic-pointers within sintab
        dz->fbandbot[h] = 0.0;

    maxoutval = 0.0;                    //  Preset maxlevel, for maxlevel test
    atten = 1.0;                        //  Preset overall attenuation
    sampcnt = 0;
    totsamps = 0;
    if(dz->brksize[TS_TSTRETCH])
        step = firststep;

    if(dz->brksize[TS_TSTRETCH] || (dz->param[TS_TSTRETCH] > 1)) {
        if(dz->vflag[0]) {  // CUBIC SPLINE INTERP

            //  Do first pass for level test

            pointer = step;
            hi = 0;
            while(pointer < numend) {
                if((exit_status = splint(pointer,&thisval,&hi,dz->numsize,secondderiv,x,y))<0)
                    return(FAILED);
                outval = calcsample(thisval,TS_SINTABSIZE,incrementor,dz);
                if(fabs(outval) >= maxoutval)
                    maxoutval = outval;
                pointer += step;
            }
            if(maxoutval > TS_MAXLEVEL)
                atten = TS_MAXLEVEL/maxoutval;

            // Do real pass

            for(h = 0; h < dz->itemcnt;h++) //  Reset harmonic-pointers within sintab
                dz->fbandbot[h] = 0.0;
            for(;;) {
                totsamps = 0;
                if(dz->brksize[TS_TSTRETCH])
                    step = firststep;
                obuf[sampcnt++] = 0.0f;         //  Splining assumes signal starts at zero
                totsamps++;
                pointer = step;
                hi = 0;
                while(pointer < numend) {
                    if((exit_status = splint(pointer,&thisval,&hi,dz->numsize,secondderiv,x,y))<0)
                        return(FAILED);
                    outval = calcsample(thisval,TS_SINTABSIZE,incrementor,dz);
                    obuf[sampcnt++] = (float)(outval * atten);      //  Scale by any attenuation required and advance to next sample
                    if(sampcnt >= dz->buflen) {
                        if((dz->wanted > 0) && dz->wanted <= dz->total_samps_written + dz->buflen) {
                            samps_to_write = dz->wanted - dz->total_samps_written;
                            if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
                                return(exit_status);
                            return FINISHED;
                        }           
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        sampcnt = 0;
                    }
                    totsamps++;
                    if(dz->brksize[TS_TSTRETCH] && (totsamps % 256 == 0)) {
                        time = (double)totsamps/(double)dz->infile->srate;
                        if((exit_status = read_value_from_brktable(time,TS_TSTRETCH,dz))<0)
                            return(exit_status);
                        dz->param[TS_TSTRETCH] = pow(2.0,dz->param[TS_TSTRETCH]);   
                        step = 1.0/dz->param[TS_TSTRETCH];
                    }
                    pointer += step;
                }
                if(!dz->vflag[1] || (dz->total_samps_written + sampcnt >= dz->wanted))
                    break;
            }

        } else {    // LINEAR INTERP
            pointer = 0.0;

            //  Do first pass for level test

            while (pointer < numend) {
                ii = (int)floor(pointer);
                jj = pointer - (double)ii;
                val = nums[ii];
                valstep = nums[ii+1] - nums[ii];
                valstep *= jj;
                val += valstep;
                outval = calcsample(val,TS_SINTABSIZE,incrementor,dz);
                if(fabs(outval) >= maxoutval)
                    maxoutval = outval;
                pointer += step;
            }
            if(maxoutval > TS_MAXLEVEL)
                atten = TS_MAXLEVEL/maxoutval;

            // Do real pass

            for(h = 0; h < dz->itemcnt;h++)     //  Preset harmonic-pointers within sintab
                dz->fbandbot[h] = 0.0;
            for(;;) {
                if(dz->brksize[TS_TSTRETCH])
                    step = firststep;
                totsamps = 0;
                pointer = 0.0;
                while (pointer < numend) {
                    ii = (int)floor(pointer);
                    jj = pointer - (double)ii;
                    val = nums[ii];
                    valstep = nums[ii+1] - nums[ii];
                    valstep *= jj;
                    val += valstep;
                    outval = calcsample(val,TS_SINTABSIZE,incrementor,dz);
                    obuf[sampcnt++] = (float)(outval * atten);      //  Scale by any attenuation required and advance to next sample
                    if(sampcnt >= dz->buflen) {
                        if((dz->wanted > 0) && dz->wanted <= dz->total_samps_written + dz->buflen) {
                            samps_to_write = dz->wanted - dz->total_samps_written;
                            if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
                                return(exit_status);
                            return FINISHED;
                        }
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        sampcnt = 0;
                    }
                    if(dz->brksize[TS_TSTRETCH] && (totsamps % 256 == 0)) {
                        time = (double)totsamps/(double)dz->infile->srate;
                        if((exit_status = read_value_from_brktable(time,TS_TSTRETCH,dz))<0)
                            return(exit_status);
                        dz->param[TS_TSTRETCH] = pow(2.0,dz->param[TS_TSTRETCH]);   
                        step = 1.0/dz->param[TS_TSTRETCH];
                    }
                    totsamps++;
                    pointer += step;
                }
                if(!dz->vflag[1] || (dz->total_samps_written + sampcnt >= dz->wanted))
                    break;
            }
        }

    } else {    //  NO TIMESTRETCH

        //  Do first pass for level test

        for(n=0;n < dz->numsize;n++) {
            outval = calcsample(nums[n],TS_SINTABSIZE,incrementor,dz);
            if(fabs(outval) > maxoutval)
                maxoutval = outval;
        }
        if(maxoutval > TS_MAXLEVEL)
            atten = TS_MAXLEVEL/maxoutval;

        // Do real pass
        
        for(;;) {
            for(n=0;n < dz->numsize;n++) {
                outval = calcsample(nums[n],TS_SINTABSIZE,incrementor,dz);
                obuf[sampcnt++] = (float)(outval * atten);      //  Scale by any attenuation required and advance to next sample
                if(sampcnt >= dz->buflen) {
                    if((dz->wanted > 0) && dz->wanted <= dz->total_samps_written + dz->buflen) {
                        samps_to_write = dz->wanted - dz->total_samps_written;
                        if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
                            return(exit_status);
                        return FINISHED;
                    }
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    sampcnt = 0;
                }
            }
            if(!dz->vflag[1] || (dz->total_samps_written + sampcnt >= dz->wanted))
                break;
        }
    }
    if(sampcnt > 0) {
        if((dz->wanted > 0) && dz->wanted < dz->total_samps_written + sampcnt)
            sampcnt = dz->wanted - dz->total_samps_written;
        if((exit_status = write_samps(obuf,sampcnt,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/************************************* FIND_MEAN ******************************/

void find_mean(double *mean,double *top,double *bottom,dataptr dz)
{
    double sum = 0.0, numax = -HUGE, numin = HUGE;
    double *nums = dz->parray[TS_DATA];
    int n;
    sum = nums[0];
    numax = nums[0];
    numin = nums[0];
    for(n = 1;n < dz->numsize; n++) {
        sum += nums[n];
        if(nums[n] > numax)
            numax = nums[n];
        if(nums[n] < numin)
            numin = nums[n];
    }
    *mean = sum /(double)dz->numsize;
    *top = numax;
    *bottom = numin;
}

/************************************* SCALE_DATA_IN_ONE_TO_MINUSONE_RANGE ******************************/

void scale_data_in_one_to_minusone_range(double mean,double top,double bottom,dataptr dz)
{
    int n;
    double *nums = dz->parray[TS_DATA];
    double uprange = top - mean;
    double dnrange = mean - bottom;
    double maxrange = max(uprange,dnrange);
    for(n = 0;n < dz->numsize; n++)
        nums[n] -= mean;
    if(maxrange > 1.0) {
        for(n = 0;n < dz->numsize; n++)
            nums[n] /= maxrange;
    }
}

/************************************ SPLINE ************************************
 *
 * This function returns the second derivative at all points of the curve.
 *
 * We assume that the curve is flat (first derivative = 0) at start and end points
 */

int spline(double *nums,double *secondderiv,double *x,double *y,dataptr dz)
{
    double firstderivstt = 0.0, firstderivend = 0.0; 
    double *u, sig, ppp, qend, uend;
    int i, k, arraylen = dz->numsize;
    if((u = (double *)malloc(arraylen * sizeof(double)))==NULL) {
        fprintf(stdout,"INSUFFICIENT MEMORY for slope calculations 1.\n");
        fflush(stdout);
        return(MEMORY_ERROR);
    }
    for(i=0;i<dz->numsize;i++) {
        x[i] = (double)i;   // TIME is the independent, monotonically increasing variable
        y[i] = nums[i];
    }
    secondderiv[0] = -0.5;
    u[0] = (3.0/(x[1] - x[0])) * (((y[1] - y[0])/(x[1] - x[0])) - firstderivstt);

    for(i = 1;i < arraylen-1; i++) {
        sig = (x[i] - x[i-1])/(x[i+1] - x[i-1]);
        ppp = (sig * secondderiv[i-1]) + 2.0;
        secondderiv[i] = (sig - 1.0)/ppp;
        u[i] = ((y[i+1] - y[i])/(x[i+1] - x[i])) - ((y[i] - y[i-1])/(x[i] - x[i-1]));
        u[i] = (((6.0 * u[i])/(x[i+1] - x[i-1])) - (sig * u[i-1]))/ppp;
    }
    // i = arraylen-1; = last entry in array
    qend = 0.5;
    uend = (3.0/(x[i] - x[i-1])) * (firstderivend - ((y[i] - y[i-1])/(x[i] - x[i-1])));
    secondderiv[i] = (uend - (qend * u[i-1]))/((qend * secondderiv[i-1]) + 1.0);
    for(k = i-1;k >= 0;k--) {
        secondderiv[k] = (secondderiv[k] * secondderiv[k+1]) + u[k];
    }
    free(u);
    return(FINISHED);
}

/************************************ SPLINT ************************************
 *
 * This function returns the value at a specified frq on curve, using cspline interp (2nd-deriv) data..
 */

int splint(double pointer,double *thisval,int *hi,int arraycnt,double *secondderiv, double *x, double *y)
{
    int klo, khi = *hi;
    double fullstep, a, b, amp, jjj, kkk;

    /* Establish which input values bracket 'pointer' */

    while(x[khi] <= pointer) {
        khi++;
        if(khi >= arraycnt) {
            *thisval = y[arraycnt - 1];
            return(FINISHED);
        }
    }
    klo = khi - 1;
    if(klo < 0) {
        *thisval = y[0];
        return(FINISHED);
    }
    fullstep = x[khi] - x[klo];
    a = (x[khi] - pointer)/fullstep;
    b = (pointer - x[klo])/fullstep;
    amp  = a * y[klo];
    amp += b * y[khi];

    jjj  = a * a * a;
    jjj -= a;
    jjj *= secondderiv[klo];

    kkk  = b * b * b;
    kkk -= b;
    kkk *= secondderiv[khi];

    jjj += kkk;
    jjj *= fullstep * fullstep;
    jjj /= 6.0;
    
    amp += jjj;
    *thisval = amp;
    *hi = khi;
    return(FINISHED);
}

/************************************ CALCSAMPLE ************************************/

double calcsample (double thisval,int sintabsize,double incrementor,dataptr dz)
{
    int h, nn, aa;
    int tabindex;
    double hno, hamp, tabfrac, tabstep, tabval, outval, thisfrq, basefrq;
    double *sintab = dz->parray[TS_SINETAB];
    outval = 0.0;
    basefrq =  miditohz(thisval);                   //  Convert midi-pitch to frq
    for(h = 0,nn = 0, aa = 1; h < dz->itemcnt;h++,nn+=2,aa+=2) {    //  h count harmonics, 
        hno  = dz->parray[TS_HARMONICS][nn];        //  Get the harmonic number from parray[1]
        thisfrq = basefrq * hno;                                                            //  'nn' points to harmonic number, 'aa' points to harmonic amplitude
        hamp = dz->parray[TS_HARMONICS][aa];        //  Get the harmonic amplitude from parray[1]
        tabindex = (int)floor(dz->fbandbot[h]);     //  Read table with this harmonic's pointer
        tabfrac  = dz->fbandbot[h] - (double)tabindex;
        tabstep = sintab[tabindex+1] - sintab[tabindex];
        tabstep *= tabfrac;
        tabval = sintab[tabindex] + tabstep;        
        tabval *= hamp;                             //  Scale by amplitude of this harmonic
        outval += tabval;                           //  Add value from this harmonic into value at sample
        dz->fbandbot[h] += incrementor * thisfrq;       //  Advance this harmonic pointer appropriately
        if(dz->fbandbot[h] >= (double)TS_SINTABSIZE)    //  wrapping around the sinetable
            dz->fbandbot[h] -= (double)TS_SINTABSIZE;
    }
    return outval;
}

/************************************ CREATE_SINTAB ************************************/

int create_sintab(dataptr dz)
{   
    int n;
    double step, *tab;
    if((dz->parray[TS_SINETAB] = (double *)malloc((TS_SINTABSIZE + 1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
        return(MEMORY_ERROR);
    }
    tab = dz->parray[TS_SINETAB];
    step = (2.0 * PI)/(double)TS_SINTABSIZE;
    for(n=0;n<TS_SINTABSIZE;n++)
        tab[n] = sin(step * (double)n) * F_MAXSAMP;
    tab[TS_SINTABSIZE] = 0.0;   // wraparound point
    return(FINISHED);
}

/************************************ COUNT_AND_TEST_FOR_INFILES ************************************/

int count_and_test_for_infiles(int argc,char *argv[],dataptr dz)
{
    switch(dz->input_data_type) {
    case(NUMLIST_ONLY):
        dz->infilecnt = 1;
        break;
    default:
        sprintf(errstr,"Unknown input_data_type: count_and_allocate_for_infiles()\n");
        return(PROGRAM_ERROR);
    }
    if((dz->insams = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for dummy infile-sampsize array.\n");
        return(MEMORY_ERROR);
    }                   
    return FINISHED;
}
