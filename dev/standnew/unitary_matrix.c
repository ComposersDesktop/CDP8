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
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <pnames.h>
#include <filetype.h>
#include <processno.h>
#include <modeno.h>
#include <pvoc.h>
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

/* MATRIX FILES : FORMAT
 *
 * 1st value is analysis-overlap value
 * after this there are N*N * 2 values
 * where N = analysis chancnt as COMPLEX values (dz->chancnt)
 * and there are N*N entries in the matrix, each being a complex number.
 * So there N*N*2 entries, with all real & imaginary components included.
 */

#ifdef unix
#define round lround
#endif

#define overlap ringsize
#define ENVWIN 8
#define wincnt rampbrksize

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
//static int    handle_the_extra_infile(char ***cmdline,int *cmdlinecnt,dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
static int  setup_the_application(dataptr dz);
static int  setup_the_param_ranges_and_defaults(dataptr dz);
static int  check_the_param_validity_and_consistency(dataptr dz);
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int  get_the_mode_no(char *str, dataptr dz);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int  outfloats(float *nextOut, float *maxsampl,float *minsample,float *local_maxsample,int *num_overflows,int todo, int finished, dataptr dz);
static int  pvoc_float_array(int nnn,float **ptr);
static int  sndwrite_header(dataptr dz);
static void hamming(float *win,int winLen,int even);
static int  matrix_time_display(int nI,int srate,int *samptime,dataptr dz);
static int write_samps_matrix(float *bbuf,int samps_to_write,dataptr dz);
static int handle_matrixdata(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int do_specmatrix(dataptr dz);
static int matrix_process(dataptr dz);
static int create_random_unitary_matrix(dataptr dz);
static int multiply_by_matrix(float *reals,float *imags,double *matrix, int N);
static void change_extension(char *filename);
static int is_pow_of_two(int val);
static int matrix_cycle(double *matrix,double *matrix2,dataptr dz);
//static float complex_product(float r1,float i1,float r2,float i2,float *iout);
//static double normalise_complex(float *anal,float *anal2,dataptr dz);
int reals_(float *a, float *b, int n, int isn);
int fftmx(float *a,float *b,int ntot,int n,int nspan,int isn,int m,int *kt,
            float *at,float *ck,float *bt,float *sk,int *np,int nfac[]);
int fft_(float *a, float *b, int nseg, int n, int nspn, int isn);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status, n;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
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
//    ap = dz->application;
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

//  handle_extra_infiles() redundant
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
    if(dz->mode == MATRIX_USE) {
        if((exit_status = handle_matrixdata(&cmdlinecnt,&cmdline,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //check_param_validity_and_consistency .....
    if((exit_status = check_the_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;

    dz->bufcnt = 2;
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

    if((exit_status = create_sndbufs(dz))<0) {                                      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = do_specmatrix(dz))<0) {
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
    dz->array_cnt=2;
    if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
        return(MEMORY_ERROR);
    }
    dz->parray[0] = NULL;
    dz->parray[1] = NULL;
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
    int exit_status, len;
    char *filename = NULL;
    len = strlen((*cmdline)[0]);
    if((filename = (char *)malloc(len + 8))==NULL) {
        sprintf(errstr,"handle_the_outfile()\n");
        return(MEMORY_ERROR);
    }
    strcpy(filename,(*cmdline)[0]);
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
    switch(dz->mode) {
    case (MATRIX_USE):  
        exit_status = set_param_data(ap,MATRIX_DATA,0,0,""  );
        break;
    default: 
        exit_status = set_param_data(ap,0          ,2,2,"ii");  
        break;
    }
    if(exit_status<0)
        return(FAILED);
    switch(dz->mode) {
    case(MATRIX_MAKE):
    case(MATRIX_USE):
        exit_status = set_vflgs(ap,"",0,"","c",1,0,"0");
        break;
    default: 
        exit_status = set_vflgs(ap,"",0,"","" ,0,0,"" );
        break;
    }
    if(exit_status <0)
        return(FAILED);


    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = SNDFILES_ONLY;
    dz->process_type    = UNEQUAL_SNDFILE;  
    dz->outfiletype     = SNDFILE_OUT;
    dz->maxmode = 4;
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
        } else if(infile_info->filetype != SNDFILE)  {
            sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
            return(DATA_ERROR);
        } else if(infile_info->channels != MONO)  {
            sprintf(errstr,"File %s is not MONO\n",cmdline[0]);
            return(DATA_ERROR);
        } else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
            sprintf(errstr,"Failed to copy file parsing information\n");
            return(PROGRAM_ERROR);
        }
        free(infile_info);
    }
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
    case(MATRIX_USE):
        break;
    default:
        ap->lo[0]           = (double)2;
        ap->hi[0]           = (double)MAX_PVOC_CHANS;
        ap->default_val[0]  = (double)DEFAULT_PVOC_CHANS;
        ap->lo[1]           = (double)1;
        ap->hi[1]           = (double)4;
        ap->default_val[1]  = (double)DEFAULT_WIN_OVERLAP;
        break;
    }
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
            if((exit_status = setup_the_application(dz))<0)
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

/*********************** CHECK_THE_PARAM_VALIDITY_AND_CONSISTENCY *********************/

int check_the_param_validity_and_consistency(dataptr dz)
{
    if(dz->mode != MATRIX_USE) {
        if(!is_pow_of_two(dz->iparam[MATRIX_CHANS])) {
            sprintf(errstr,"Number of analysis channels must be a power of 2\n");
            return(DATA_ERROR);
        }
        dz->wanted  = dz->iparam[MATRIX_CHANS];
        dz->clength = dz->wanted/2;
        dz->overlap = dz->iparam[MATRIX_OVLAP];
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
    if      (!strcmp(prog_identifier_from_cmdline,"matrix"))        dz->process = MATRIX;
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        return(USAGE_ONLY);
    }
    return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
    fprintf(stderr,
    "\nMATRIX MANIPULATION OF SPECTRUM OF SOUND\n\n"
    "USAGE: matrix matrix (matrix-data) infile outfile(s).\n"
    "\n"
    "Type 'matrix matrix' for more info\n");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"matrix")) {
        fprintf(stdout,
        "USAGE:\n"
        "matrix matrix 1 infile outfiles analchans winoverlap [-c]\n"
        "matrix matrix 2 infile outfile inmatrixfile [-c]\n"
        "matrix matrix 3-4 infile outfiles analchans winoverlap\n"
        "\n"
        "MODE 1: Generates matrix, and transformed sound.\n"
        "MODE 2: Uses input matrix data to transform sound.\n"      
        "MODE 3: Exchanges reals and imags in fft output.\n"
        "MODE 4: Invert phase in each channel +ve to -ve\n"
        "\n"
        "ANALCHANS   No of Analysis Channels (Pow-of-2: 4-16384)\n"
        "WINOVERLAP  Overlap of Analysis windows (Range 1-4)\n"
        "-c          Apply matrix-transform Cyclically.\n"
        "            window1 = spec * matrix\n"
        "            window2 = spec * matrix * matrix\n"
        "            window3 = spec * matrix * matrix * matrix\n"
        "            and so on\n"
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

/************************ HANDLE_MATRIXDATA *********************/

int handle_matrixdata(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    aplptr ap = dz->application;
    int n, rowcnt, colcnt;
    int cnt_of_vals, cnt_of_matrix_vals, cnt_of_complex_vals;
    char temp[200], *q, *filename = (*cmdline)[0];
    double *p, *matrix;
    double dummy, maxmatrix;
    if(!sloom) {
        if(*cmdlinecnt <= 0) {
            sprintf(errstr,"Insufficient parameters on command line.\n");
            return(USAGE_ONLY);
        }
    }
    ap->data_in_file_only   = TRUE;
    ap->special_range       = TRUE;
    ap->min_special         = -1.0;
    ap->max_special         = 4.0;
    maxmatrix               = 1.0;
    if((dz->fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Cannot open datafile %s\n",filename);
        return(DATA_ERROR);
    }
    n = 0;
    p = &dummy;
    while(fgets(temp,200,dz->fp)!=NULL) {
        q = temp;
        if(is_an_empty_line_or_a_comment(q))
            continue;
        while(get_float_from_within_string(&q,p)) {
            if(n==0) {
                if(*p < 1.0 || *p > 4.0) {
                    sprintf(errstr,"Invalid analysis overlap val %d (range 1 - 4) : file %s\n",(int)round(*p),filename);
                    return(DATA_ERROR);
                }
                dz->overlap = (int)round(dummy);
            } else {
                if(*p < ap->min_special || *p > maxmatrix) {
                    sprintf(errstr,"Matrix value out of range (%lf - %lf) : line %d: file %s\n",ap->min_special,ap->max_special,n+1,filename);
                    return(DATA_ERROR);
                }
            }
            n++;
        }
    }
    if(n==0) {
        sprintf(errstr,"No data in matrix file %s\n",filename);
        return(DATA_ERROR);
    }
    if(dz->overlap < 1 || dz->overlap > 4) {
        sprintf(errstr,"Analysis Overlap value (%d) out of range (1-4) if file %s\n",dz->overlap,filename);
        return(DATA_ERROR);
    }
    cnt_of_vals = n;
    cnt_of_matrix_vals = n - 1;
    cnt_of_complex_vals = cnt_of_matrix_vals/2; //  for 512 complex vals : matrix is 512 * 512 complex values
                //  = 512 rows of 1024 (real+imag) values, so matrix is 1024 X 512 : and k = n/2 = 512 X 512
                //  1024 is dz->wanted  512 is dz->clength  so sqrt(k) = dz->clength
    dz->clength = (int)round(sqrt(cnt_of_complex_vals));
    if(!is_pow_of_two(dz->clength)) {
        sprintf(errstr,"Matrix has invalid no of values (%d : corresponds to chancnt %d)\n",cnt_of_matrix_vals,dz->clength*2);
        return(DATA_ERROR);
    }
    dz->wanted = dz->clength * 2;
    if ((dz->parray[0] = (double *)malloc(cnt_of_matrix_vals * sizeof(double)))==NULL) {
        sprintf(errstr,"Insufficient memory to store matrix data\n");
        return(MEMORY_ERROR);
    }
    if ((dz->parray[1] = (double *)malloc(cnt_of_matrix_vals * sizeof(double)))==NULL) {
        sprintf(errstr,"Insufficient memory to store 2nd matrix data\n");
        return(MEMORY_ERROR);
    }
    if(fseek(dz->fp,0,0)<0) {
        sprintf(errstr,"fseek() failed in handle_matrixdata()\n");
        return(SYSTEM_ERROR);
    }
    p = dz->parray[0];
    n = 0;
    while(fgets(temp,200,dz->fp)!=NULL) {
        q = temp;
        if(is_an_empty_line_or_a_comment(temp))
            continue;
        while(get_float_from_within_string(&q,p)) {
            if(n == 0)
                p--;                    //  overwrite first (ovlap) value by first true value in matrix
            if(++n >= cnt_of_vals)
                break;
            p++;
        }
        if(n >= cnt_of_vals)
            break;
    }
    matrix = dz->parray[0];
    dz->itemcnt = cnt_of_matrix_vals;
    if(fclose(dz->fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
        fflush(stdout);
    }
    rowcnt = -1;
    for(n = 0; n < cnt_of_matrix_vals; n+=2) {  //  advancing in real-imag pairs
        colcnt = (n % dz->wanted)/2;            //  new col reacned when we have a multiple of 1024 entries
        if(colcnt == 0)                         //  but this is column modcnt/2 as there are 2 entries per column
            rowcnt++;
        if(rowcnt == colcnt) {                  //  Diagonal entry : bot hreal and imag vals should be zero
            if((matrix[n] != 0.0) || (matrix[n+1] != 0.0)) {
                sprintf(errstr,"WARNING: Matrix data does not have zeros on the diagonal. Not a unitary matrix.\n");
                return(DATA_ERROR);
            }
        }
    }
    (*cmdline)++;       
    (*cmdlinecnt)--;
    return(FINISHED);
}

/* How to form an N X N unitary matrix.
 *
 * (1) Set up an empty N X N matrix = A
 * (2) Put any complex numbers at all, above the main diagonal
 * (3) Make the hermitian transpose A* i.e. rotate A about the diagonal
 *  and then replace the values by their complex conjugates.
 * (4) Form matrix B  =  A  -  A*
 * (5) Form matrix U  =  exp(B)
 * 
 * My interpretation for 4X4
 *    (1)            (2)                 (3)
 * |0 0 0 0|        |0 a+ib c+id e+if|  | 0    0    0   0|
 * |0 0 0 0|        |0  0   g+ih j+ik|  |a-ib  0    0   0|
 * |0 0 0 0|        |0  0    0   m+in|  |c-id g-ih  0   0|
 * |0 0 0 0|        |0  0    0    0  |  |e-if j-ik m-in 0|
 * 
 *      (4)                     (5)
 * |  0    a+ib  c+id  e+if|    |   1       exp(a+ib)   exp(c+id) exp(e+if)|
 * |-a+ib   0    g+ih  j+ik|    |exp(-a+ib)     1       exp(g+ih) exp(j+ik)|
 * |-c+id -g+ih     0  m+in|    |exp(-c+id) exp(-g+ih)      1     exp(e+if)|
 * |-e+if -j+ik  -m+in   0 |    |exp(-e+if) exp(-j +ik) exp(-e+if)    1    |
 * 
 * where "1" could be stored as exp(0,0)
 *
 * Store as a single array, row by row. 
 * Store real+imag data as *fptr  malloc(*fptr, N * N * 2, sizeof(float))
 * 
 * for row,column (Y,X)
 * rowindex = Y * N     with val-pairs = Y * N * 2
 * column index = X     with val-pairs = X * 2
 * row-column-index_real (Y,X) = (Y*N*2) + (X*2) = ((Y*N) + X)*2
 * row-column-index_imag (Y,X) = ditto+1
 */

/****************************** MULTIPLY_BY_MATRIX ************************
 *
 * Presumably FFT data is exp(x(n)+iy(n))  stored as reals[n] and imags[n]
 * 
 * Let N be the size of the (spectrum as amp and phase) vector
 *
 * MULTIPLY EXPONENTS incorrect but may be interesting!!
 *    NB:   (a+ib)*(c+id) = (ac-bd) + i(ad+bc)
 * ADD EXPONENTS to multply exponentials
 *    NB:   (a+ib)+(c+id) = (ac) + i(bd)
 */

int multiply_by_matrix(float *reals,float *imags,double *matrix, int N)
{
    double * transformed_realval, *transformed_imagval;
    double realval, imagval, rmval, imval, nurealval, nuimagval;
    int x, y, m_index;
    if((transformed_realval = (double *)malloc(sizeof(double) * (N*2)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to establish output vector reals.\n");
        return(MEMORY_ERROR);
    }
    if((transformed_imagval = (double *)malloc(sizeof(double) * (N*2)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to establish output vector imags.\n");
        return(MEMORY_ERROR);
    }

    for (x = 0; x < N; x++) {
        transformed_realval[x] = 0.0;
        transformed_imagval[x] = 0.0;
        for (y = 0; y < N; y++) {
            realval = reals[y];         //  get each (yth) value of the original vector
            imagval = imags[y];
//      rmval = "row-column-index_real[row y,col x]";
            m_index = ((y*N) + x)*2;    //  mutiplied by the (yth) row entry in the xth column
            rmval = matrix[m_index++];
            imval = matrix[m_index];
// MULTIPLY EXPONENTS
            nurealval = (realval * rmval) - (imagval * imval);  // (ac-bd)
            nuimagval = (realval * imval) + (imagval * rmval);  // (ad+bc)
// ADD EXPONENTS
//          nurealval = (realval + rmval);  // (a+c)
//          nuimagval = (imagval + imval);  // (b+d)

            transformed_realval[x] += nurealval;
            transformed_imagval[x] += nuimagval;
        }
    }
    for (x = 0; x < N; x++) {
        reals[x] = (float)transformed_realval[x];
        imags[x] = (float)transformed_imagval[x];
    }
    return FINISHED;
}

/****************************** CREATE_RANDOM_UNITARY_MATRIX ************************
 *
 * An arbitrary N X N unitary matrix
 * Length of diagonal = no of rows = no of cols = N
 * e.g. for N = 5
 * Number of vals required(K) =         1  +  2 ...   + (N-1)
 *                              |  0    x     x     x     x |
 *                              |    \                      |
 *                              |  0    0     x     x     x |
 *                              |          \                |
 *                              |  0    0     0     x     x |
 *                              |                \          |
 *                              |  0    0     0     0     x |
 *                              |                      \    |
 *                              |  0    0     0     0     0 |
 * K = (1 + 2 + 3 .... +(N-1))
 *
 * For unitary matrix, diagonal is all zero, and matrix is symmetrical about diagonal
 * except that REAL values to lower left are negation of their mirror values to upper right.
 *
 *  The matrix rows and columns are indexed by row,column (n,m)
 *  and the matrix is stored as row 1, row 2, row 3 ....
 *      for row,column (Y,X)
 *      index of row-start  = Y * N
 *      index of column entry in row = X
 *      row-column-index_(complex) (Y,X) = ((Y*N) + X)
 *      row-column-index_real/imag  (a pair of vals : real & imag : for each complex number)
 *      row-column-index_real       [(Y*N) + X] * 2
 *      row-column-index_imag       ditto + 1
 */

int create_random_unitary_matrix(dataptr dz)
{
    int N, K = 0, M, cnt, m_index, mc_index;
    double *r, *i, *matrix;
    int n, m;
    N = dz->clength;                    //  N is number of COMPLEX-number entries in FFT
    for(n=1; n<N; n++)                  //  Sum of 1,2, .... (N-1)
        K += n;
    M = N * N;                          //  matrix size (in complex-numbers);
    if((r = (double *)malloc(sizeof(double) * K))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to establish real matrix components.\n");
        return(MEMORY_ERROR);
    }
    if((i = (double *)malloc(sizeof(double) * K))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to establish imag matrix components.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc(sizeof(double) * (M*2)))==NULL) {  //  double size, to store reals & imags
        sprintf(errstr,"INSUFFICIENT MEMORY to establish matrix array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[1] = (double *)malloc(sizeof(double) * (M*2)))==NULL) {  //  double size, to store reals & imags
        sprintf(errstr,"INSUFFICIENT MEMORY to establish 2nd matrix array.\n");
        return(MEMORY_ERROR);
    }
    matrix = dz->parray[0];
    memset((char *)matrix,0,M * 2 * sizeof(double));        //  Presetting matrix vals to zero
                                                            //  ensures diagonal will hold zeros.
    initrand48();   

    for(n=0;n<K; n++) {                                     //  Generate all required (K) real numbers
        r[n] = drand48();           //  reals
        if(flteq(r[n],0.0)) {       //  Ensure no zeros in reals
            n--;
            continue;
        }
    }
    for(n=0;n<K; n++) {
        i[n] = drand48();           //  imags
    }
                                                                //  | 00  01  02  03  ... to upper right of diagonal    
    cnt = 0;        //  set counter for reals & imags           //  |    x                m > n
                                                                //  | 10  11  12  13  ...   
    for(n = 0; n < N; n++) {        //  rows                        |        x
        for(m = 0; m < N; m++) {    //  cols                        | 20  21  22  23  ... to lower left of diagonal
            if(m > n) {     //  to upper right of diagonal          |            x        m < n
                m_index = ((n*N)+m) * 2;                        //  | 30  31  32  33  ...
                matrix[m_index++] = r[cnt];                     //  |                x
                matrix[m_index]   = i[cnt++];                   //  | ... ... ... ..  ...   
            } else if(m < n) {
                m_index  = ((n*N)+m) * 2;                   //  matrix index here
                mc_index = ((m*N)+n) * 2;                   //  matrix index diagonally opposite
                matrix[m_index++] = -matrix[mc_index++];    //  COMPLEMENTARY real values
                matrix[m_index]   =  matrix[mc_index];      //  imag value is the same
            }   
        }
    }

    dz->itemcnt = M*2;
//                                              // TEST ONLY : check diagonal values are all zero
//  for(m = 0; m < N; m++) {        //  cols
//      for(n = 0; n < N; n++) {    //  rows
//          if(m == n) {
//              m_index = ((n*N)+m) * 2;
//              if(matrix[m_index] != 0.0 || matrix[m_index+1] != 0.0) {
//                  sprintf(errstr,"Error in array creation\n");
//                  return(PROGRAM_ERROR);
//              }
//          }
//      }
//  }
    for(n = 0; n < N; n++)
        dz->parray[1][n] = dz->parray[0][n];        // File 2nd array for cycling of matrix
    return FINISHED;
}

/*  NB:                                                     00 01 02 ....
 *      Matrix filled across one row then across next, so   10 11 12 ...
 *                                                          20 21 22 ...
 *      So any location nm where m > n (e.g. 12), 
 *      is always assigned before we reach mn (e.g. 21)
 *      So when we reach mn we can always use
 *      the previously assigned values in nm
 */

/********************************** DO_SPECMATRIX **************************/

int do_specmatrix(dataptr dz)
{
    int exit_status;
    int n;
    char filename[2000];
    if(dz->mode == MATRIX_MAKE) {
        if((exit_status = create_random_unitary_matrix(dz)) < 0) {  
            return(exit_status);
        }
        strcpy(filename,dz->outfilename);
        change_extension(filename);
        if((dz->fp = fopen(filename,"w")) == NULL) {
            fprintf(stdout,"WARNING: Failed to write matrix data to output file.\n");
            fflush(stdout);
        } else {
//          WRITE AS BINARY
            fprintf(dz->fp,"%d\n",dz->overlap);
            for(n=0;n<dz->itemcnt;n++) {
                fprintf(dz->fp,"%f\n",dz->parray[0][n]);
            }
            fclose(dz->fp);
        }
    }                           
    if((exit_status = matrix_process(dz))<0)
        return exit_status;
    return FINISHED;
}

/********************************** CHANGE_EXTENSION **************************/

void change_extension(char *filename)
{
    int len;
    char *p;
    len = strlen(filename);
    p = filename + len - 1;
    while(p > filename) {
        if(*p == '.')
            break;
        p--;
    }
    if(p > filename) {
        if(sloom)
            p--;
        *p = ENDOFSTR;
    }
    if(sloom) {
        strcat(filename,"1");
    } else {
        strcat(filename,".txt");
    }
    return;
}

/****************************** MATRIX_PROCESS ******************************/

int matrix_process(dataptr dz)
{
    int exit_status;
    int finished = 0;
    int rr, ii;
    float temp;
    int num_overflows = 0;
    int samptime = SAMP_TIME_STEP;

    double  *matrix,        /* pointer to start of matrix buffer */
            *matrix2;       /* pointer to start of 2nd matrix buffer */
    float   *input,         /* pointer to start of input buffer */
            *output,        /* pointer to start of output buffer */
            *anal,          /* pointer to start of analysis buffer */
            *banal,         /* pointer to anal[1] (for FFT calls) */
            *nextIn,        /* pointer to next empty word in input */
            *nextOut,       /* pointer to next empty word in output */
            *analWindow,    /* pointer to center of analysis window */
            *synWindow,     /* pointer to center of synthesis window */
            maxsample = 0.0, minsample = 0.0, biggest;

    int     M = 0,          /* length of analWindow impulse response */
            D = 0,          /* decimatin factor */
            I = 0,          /* interpolation factor (default will be I=D)*/
            analWinLen,     /* half-length of analysis window */
            synWinLen;      /* half-length of synthesis window */

    int outCount,       /* number of samples written to output */
            ibuflen,        /* length of input buffer */
            obuflen,        /* length of output buffer */
            nI = 0,         /* current input (analysis) sample */
            nO,             /* current output (synthesis) sample */
            nMaxOut;        /* last output (synthesis) sample */
    int isr,            /* sampling rate */
            endsamp = VERY_BIG_INT;

    float   sum,            /* scale factor for renormalizing windows */
            R,              /* input sampling rate */

            local_maxsample = 0.0;

    int     i,j,k,      /* index variables */
            Dd,         /* number of new inputs to read (Dd <= D) */
            Ii,         /* number of new outputs to write (Ii <= I) */
            N2,         /* dz->wanted/2 = dz->clength */
            NO,         /* synthesis NO = dz->wanted / P */
            NO2,        /* NO/2 */
            IO,         /* synthesis IO = I / P */
            IOi,        /* synthesis IOi = Ii / P */
            Mf = 0,     /* flag for even M */
            flag = 0,   /* end-of-input flag */
            matrix_ovlap;
    /*RWD */
//    float F = 0.0f;

    matrix  = dz->parray[0];
    matrix2 = dz->parray[1];

    isr      = dz->infile->srate;
    R        = (float)isr;

    if(flteq(R,0.0)) {
        sprintf(errstr,"Problem: zero sampling rate\n");
        return(DATA_ERROR);
    }
    N2 = dz->wanted / 2;        //  N2 = clength

//    F = /*(int)*/(float)(R /(float)dz->wanted);   /*RWD*/

    matrix_ovlap = dz->overlap - 1;     //  See pvoc_preprocess
    switch(matrix_ovlap){
        case 0: M = 4*dz->wanted;   break;
        case 1: M = 2*dz->wanted;   break;
        case 2: M = dz->wanted;     break;
        case 3: M = N2;             break;
        default:
            sprintf(errstr,"ERROR: Invalid window Overlap factor.\n");
            return(PROGRAM_ERROR);
    }

    Mf = 1 - M%2;

    if (M < 7) {
        fprintf(stdout,"WARNING: analWindow impulse response is too small\n");
        fflush(stdout);
    }
    ibuflen = 4 * M;
    obuflen = 4 * M;

    if((D = (int)(M/PVOC_CONSTANT_A)) == 0){
        fprintf(stdout,"WARNING: Decimation too low: adjusted.\n");
        fflush(stdout);
        D = 1;
    }

    if(sloom)
        dz->tempsize = dz->insams[0];

    I   = D;
    NO  = dz->wanted;   /* synthesis transform will be NO points */
    NO2 = NO/2;
    IO  = I;

    //  TW : possibly redundant
    if((exit_status = sndwrite_header(dz))<0)
        return(exit_status);

    //  DO ANALYSIS

    if(!sloom && !sloombatch) {
        fprintf(stdout,"analysis beginning\n"); 
        fflush(stdout);
    }
    /* set up analysis window: The window is assumed to be symmetric
        with M total points.  After the initial memory allocation,
        analWindow always points to the midpoint of the window
        (or one half sample to the right, if M is even); analWinLen
        is half the true window length (rounded down). Any low pass
        window will work; a Hamming window is generally fine,
        but a Kaiser is also available.  If the window duration is
        longer than the transform (M > N), then the window is
        multiplied by a sin(x)/x function to meet the condition:
        analWindow[Ni] = 0 for i != 0.  In either case, the
        window is renormalized so that the phase vocoder amplitude
        estimates are properly scaled.  The maximum allowable
        window duration is ibuflen/2. */

    if((exit_status = pvoc_float_array(M+Mf,&analWindow))<0)
        return(exit_status);
    analWinLen = M/2;
    analWindow += analWinLen;

    hamming(analWindow,analWinLen,Mf);

    for (i = 1; i <= analWinLen; i++)
        *(analWindow - i) = *(analWindow + i - Mf);

    if (M > dz->wanted) {
        if (Mf)
        *analWindow *=(float)
        ((double)dz->wanted*sin((double)PI*.5/dz->wanted)/(double)(PI*.5));
        for (i = 1; i <= analWinLen; i++) 
            *(analWindow + i) *=(float)
            ((double)dz->wanted * sin((double) (PI*(i+.5*Mf)/dz->wanted)) / (PI*(i+.5*Mf))); /* D.Timis */
        for (i = 1; i <= analWinLen; i++)
            *(analWindow - i) = *(analWindow + i - Mf);
    }

    sum = 0.0f;
    for (i = -analWinLen; i <= analWinLen; i++)
        sum += *(analWindow + i);

    sum = (float)(2.0/sum);     /*factor of 2 comes in later in trig identity*/

    for (i = -analWinLen; i <= analWinLen; i++)
        *(analWindow + i) *= sum;

    /* set up synthesis window:  For the minimal mean-square-error
        formulation (valid for N >= M), the synthesis window
        is identical to the analysis window (except for a
        scale factor), and both are even in length.  If N < M,
        then an interpolating synthesis window is used. */

    if((exit_status = pvoc_float_array(M+Mf,&synWindow))<0)
        return(exit_status);
    synWinLen = M/2;
    synWindow += synWinLen;

    if (M <= dz->wanted){
        hamming(synWindow,synWinLen,Mf);
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Mf);

        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;

        sum = 0.0f;
        for (i = -synWinLen; i <= synWinLen; i+=I)
            sum += *(synWindow + i) * *(synWindow + i);

        sum = (float)(1.0/ sum);

        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;
    } else {    
        hamming(synWindow,synWinLen,Mf);
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Mf);

        if (Mf)
            *synWindow *= (float)((double)IO * sin((double) (PI*.5/IO)) / (double)(PI*.5));
        for (i = 1; i <= synWinLen; i++) 
            *(synWindow + i) *=(float)
            ((double)IO * sin((double) (PI*(i+.5*Mf)/IO)) /(double) (PI*(i+.5*Mf)));
        for (i = 1; i <= synWinLen; i++)
            *(synWindow - i) = *(synWindow + i - Mf);

        sum = (float)(1.0/sum);

        for (i = -synWinLen; i <= synWinLen; i++)
            *(synWindow + i) *= sum;
    }
      
    /* set up input buffer:  nextIn always points to the next empty
        word in the input buffer (i.e., the sample following
        sample number (n + analWinLen)).  If the buffer is full,
        then nextIn jumps back to the beginning, and the old
        values are written over. */

    if((exit_status = pvoc_float_array(ibuflen,&input))<0)
        return(exit_status);

    nextIn = input;

    /* set up output buffer:  nextOut always points to the next word
        to be shifted out.  The shift is simulated by writing the
        value to the standard output and then setting that word
        of the buffer to zero.  When nextOut reaches the end of
        the buffer, it jumps back to the beginning.  */

    if((exit_status = pvoc_float_array(obuflen,&output))<0)
        return(exit_status);

    nextOut = output;

    /* set up analysis buffer for (N/2 + 1) channels: The input is real,
        so the other channels are redundant. */

    if((exit_status = pvoc_float_array(dz->wanted+2,&anal))<0)
        return(exit_status);
    banal = anal + 1;

    /* initialization: input time starts negative so that the rightmost
        edge of the analysis filter just catches the first non-zero
        input samples; output time is always T times input time. */

    outCount = 0;
    nI = -(analWinLen / D) * D; /* input time (in samples) */
    nO = nI;                    /* output time (in samples) */
    Dd = analWinLen + nI + 1;   /* number of new inputs to read */
    Ii = 0;             /* number of new outputs to write */
    IOi = 0;
    flag = 1;

    /* main loop:  If endsamp is not specified it is assumed to be very large
        and then readjusted when fgetfloat detects the end of input. */

    display_virtual_time(0L,dz);    

    dz->wincnt = 0;
    while(nI < (endsamp + analWinLen)){
        {
            static float *sbuf = 0;
            static int sblen = 0;
            int got, tocp;
            float *sp;

            if(sblen < Dd) {
                if(sbuf != 0)
                    free(sbuf);

                if((sbuf = (float *)malloc(Dd*sizeof(float))) == 0) {
                    sprintf(errstr, "pvoc: can't allocate short buffer\n");
                    return(MEMORY_ERROR);
                }
                sblen = Dd;
            }
            if((got = fgetfbufEx(sbuf, Dd, dz->ifd[0],0)) < 0) {
                sfperror("pvoc: read error");
                return(SYSTEM_ERROR);
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
        }
        if (nI > 0)
            for (i = Dd; i < D; i++){   /* zero fill at EOF */
                *(nextIn++) = 0.0f;
                if (nextIn >= (input + ibuflen))
                    nextIn -= ibuflen;
            }
    
/* analysis: The analysis subroutine computes the complex output at
    time n of (dz->wanted/2 + 1) of the phase vocoder channels.  It operates
    on input samples (n - analWinLen) thru (n + analWinLen) and
    expects to find these in input[(n +- analWinLen) mod ibuflen].
    It expects analWindow to point to the center of a
    symmetric window of length (2 * analWinLen +1).  It is the
    responsibility of the main program to ensure that these values
    are correct!  The results are returned in anal as succesive
    pairs of real and imaginary values for the lowest (dz->wanted/2 + 1)
    channels.   The subroutines fft and reals together implement
    one efficient FFT call for a real input sequence.  */


        for (i = 0; i < dz->wanted+2; i++) 
            *(anal + i) = 0.0f; /*initialize*/

        j = (nI - analWinLen - 1 + ibuflen) % ibuflen;  /*input pntr*/

        k = nI - analWinLen - 1;            /*time shift*/
        while (k < 0)
            k += dz->wanted;
        k = k % dz->wanted;
        for (i = -analWinLen; i <= analWinLen; i++) {
            if (++j >= ibuflen)
                j -= ibuflen;
            if (++k >= dz->wanted)
                k -= dz->wanted;
            *(anal + k) += *(analWindow + i) * *(input + j);
        }
        if((exit_status = fft_(anal,banal,1,N2,1,-2))<0)
            return(exit_status);
        if((exit_status = reals_(anal,banal,N2,-2))<0)
            return(exit_status);

        switch(dz->mode) {
        case(2):
            for(rr=0,ii = 1;rr<dz->wanted+2;rr+=2,ii+=2) {              // Swap reals and imags
                temp     = anal[rr];
                anal[rr] = anal[ii];
                anal[ii] = temp;
            }
            break;
        case(3):
            for(ii = 1;ii<dz->wanted+2;ii+=2)                           // Invert phase in each channel
                anal[ii] = (float)-anal[ii];
            break;
        default:
            if((exit_status = multiply_by_matrix(anal,banal,matrix,N2))< 0)
                return(exit_status);
            if(dz->vflag[0]) {
                if((exit_status = matrix_cycle(matrix,matrix2,dz))< 0)
                    return(exit_status);
            }
            break;
        }

//  DO RESYNTH

    /* synthesis: The synthesis subroutine uses the Weighted Overlap-Add
        technique to reconstruct the time-domain signal.  The (dz->wanted/2 + 1)
        phase vocoder channel outputs at time n are inverse Fourier
        transformed, windowed, and added into the output array.  The
        subroutine thinks of output as a shift register in which 
        locations are referenced modulo obuflen.  Therefore, the main
        program must take care to zero each location which it "shifts"
        out (to standard output). The subroutines reals and fft
        together perform an efficient inverse FFT.  */

        if((exit_status = reals_(anal,banal,NO2,2))<0)
            return(exit_status);
        if((exit_status = fft_(anal,banal,1,NO2,1,2))<0)
            return(exit_status);

        j = nO - synWinLen - 1;
        while (j < 0)
            j += obuflen;
        j = j % obuflen;

        k = nO - synWinLen - 1;
        while (k < 0)
            k += NO;
        k = k % NO;

        for (i = -synWinLen; i <= synWinLen; i++) { /*overlap-add*/
            if (++j >= obuflen)
                j -= obuflen;
            if (++k >= NO)
                k -= NO;
            *(output + j) += *(anal + k) * *(synWindow + i);
        }

        for (i = 0; i < IOi;){  /* shift out next IOi values */
            int j;
            int todo = min(IOi-i, output+obuflen-nextOut);

// NEW 2014 -->
            if((exit_status = outfloats(nextOut,&maxsample,&minsample,&local_maxsample,&num_overflows,todo,finished,dz))<0)
                return(exit_status);
// <-- NEW 2014
            i += todo;
            outCount += todo;
            for(j = 0; j < todo; j++)
                *nextOut++ = 0.0f;
            if (nextOut >= (output + obuflen))
                nextOut -= obuflen;

        }
                    
        if(flag                             /* flag means do this operation only once */
        && (nI > 0) && (Dd < D)){           /* EOF detected */
            flag = 0;
            endsamp = nI + analWinLen - (D - Dd);
        }

        nI += D;                /* increment time */
        nO += IO;
                                /* Dd = D except when the end of the sample stream intervenes */
        Dd = min(D, max(0, D+endsamp-nI-analWinLen));

        if (nO > (synWinLen + I))
            Ii = I;
        else if (nO > synWinLen)
            Ii = nO - synWinLen;
        else {
            Ii = 0;
            for (i=nO+synWinLen; i<obuflen; i++) {
                if (i > 0)
                    *(output+i) = 0.0f;
            }
        }
        IOi = Ii;


        if(nI > samptime && (exit_status = matrix_time_display(nI,isr,&samptime,dz))<0)
            return(exit_status);
        dz->wincnt++;
    }   /* End of SYNTH subloop */
    nMaxOut = endsamp;
    while (outCount <= nMaxOut){
        int todo = min(nMaxOut-outCount, output+obuflen-nextOut);
        if(todo == 0)
            break;
        if((exit_status = outfloats(nextOut,&maxsample,&minsample,&local_maxsample,&num_overflows,todo,finished,dz))<0)
            return(exit_status);
        outCount += todo;
        nextOut += todo;
        if (nextOut >= (output + obuflen))
            nextOut -= obuflen;

    }
    if((exit_status = matrix_time_display((int)endsamp,isr,&samptime,dz))<0)
        return(exit_status);

#ifndef NOOVERCHK
    if(num_overflows > 0) {
        biggest =  maxsample;
        if(-minsample > maxsample)
            biggest = -minsample;
        fprintf(stdout, "WARNING: %d samples overflowed, and were clipped\n",num_overflows);
        fprintf(stdout, "WARNING: maximum sample was %f  :  minimum sample was %f\n",maxsample,minsample);
        fprintf(stdout, "WARNING: You should reduce source level to avoid clipping: use gain of <= %lf\n",1.0/biggest);
        fflush(stdout);
    }
#endif
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
        sprintf(errstr,"pvoc: insufficient memory\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/************************************ OUTFLOATS ************************************/

int outfloats(float *nextOut,float *maxsample,float *minsample,float *local_maxsample,int *num_overflows,int todo,int finished,dataptr dz)
{
    static float *sbuf = 0;
    static int sblen = 0;
    float *sp;
    int cnt;
    float val;
    float local_minsample = 0.0;
    *local_maxsample = 0.0;
    if(sblen < todo) {
        if(sbuf != 0)
            free(sbuf); 
        if((sbuf = (float *)malloc(todo*sizeof(float))) == 0) {
            sprintf(errstr, "pvoc: can't allocate output buffer\n");
            return(MEMORY_ERROR);
        }
        sblen = todo;
    }

    sp = sbuf;
#ifdef NOOVERCHK
    for(cnt = 0; cnt < todo; cnt++)
        *sp++ = *nextOut++; 
#else
    for(cnt = 0; cnt < todo; cnt++) {
        val = *nextOut++;
        if(val >= 1.0 || val <= -1.0) {
            (*num_overflows)++;
            if(val > 0.0f) {
                if(val > *maxsample)
                    *maxsample = val;
                val = 1.0f;
            }
            if(val < 0.0f) {
                if(val < *minsample)
                    *minsample = val;
                val = -1.0f;
            }
        }
        *sp = val;
        if(*sp > *local_maxsample)
            *local_maxsample = val;
        if(*sp < local_minsample)
            local_minsample = val;
        sp++;
    }
#endif
    *local_maxsample = max(-local_minsample,*local_maxsample);
    if(finished && (*local_maxsample == 0.0))   //  Input read finished && Output buffer empty
        todo = 0;
    if(todo > 0) {
        if(write_samps_matrix(sbuf, todo, dz) < 0) {
            sfperror("pvoc: write error");
            return(SYSTEM_ERROR);
        }
    }
    return(FINISHED);
}

/************************************ SNDWRITE_HEADER ************************************/

int sndwrite_header(dataptr dz)
{
    dz->outfile->srate = dz->infile->srate;
    dz->outfile->channels = dz->infile->channels;
    fflush(stdout);
    return(FINISHED);
}

/*********************************** MATRIX_TIME_DISPLAY ***********************************/

int matrix_time_display(int nI,int srate,int *samptime, dataptr dz)
{
    display_virtual_time(nI,dz);    
    *samptime += SAMP_TIME_STEP;
    return(FINISHED);
}

/******************************* WRITE_SAMPS_MATRIX ********************************/

int write_samps_matrix(float *bbuf,int samps_to_write,dataptr dz)
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

/************************** MXFFT ********************/

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

int fft_(float *a, float *b, int nseg, int n, int nspn, int isn)
        /*  a: pointer to array 'anal'  */
        /*  b: pointer to array 'banal' */
{
    int exit_status;
    int nfac[16];       /*  These are one bigger than needed   */
                        /*  because wish to use Fortran array  */
                        /* index which runs 1 to n, not 0 to n */

    int     m = 0, nf, k, kt, ntot, j, jj, maxf, maxp=0;

/* work space pointers */
    float   *at, *ck, *bt, *sk;
    int     *np;


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
    for (m=0; !(k%16); nfac[++m]=4,k/=16)
        ;
    for (j=3,jj=9; jj<=k; j+=2,jj=j*j)
        for (; !(k%jj); nfac[++m]=j,k/=jj);

    if (k<=4){
        kt = m;
        nfac[m+1] = k;
        if(k != 1)
            m++;
    }
    else{
        if(k%4==0){
            nfac[++m]=2;
            k/=4;
        }

        kt = m;
            maxp = max((kt+kt+2),(k-1));
        for(j=2; j<=k; j=1+((j+1)/2)*2)
            if(k%j==0){
                nfac[++m]=j;
                k/=j;
            }
    }
    if(m <= kt+1) 
        maxp = m + kt + 1;
    if(m+kt > 15) {
        sprintf(errstr,"FFT parameter n has more than 15 factors : %d", n);
        return(DATA_ERROR);
    }
    if(kt!=0){
        j = kt;
        while(j)
            nfac[++m]=nfac[j--];
    }
    maxf = nfac[m-kt];
    if(kt > 0) 
        maxf = max(nfac[kt],maxf);

/*  allocate workspace - assume no errors! */
    at = (float *) calloc(maxf,sizeof(float));
    ck = (float *) calloc(maxf,sizeof(float));
    bt = (float *) calloc(maxf,sizeof(float));
    sk = (float *) calloc(maxf,sizeof(float));
    np = (int *) calloc(maxp,sizeof(int));

/* decrement pointers to allow FORTRAN type usage in fftmx */
    at--;   bt--;   ck--;   sk--;   np--;

/* call fft driver */

    if((exit_status = fftmx(a,b,ntot,nf,nspn,isn,m,&kt,at,ck,bt,sk,np,nfac))<0)
        return(exit_status);

/* restore pointers before releasing */
    at++;   bt++;   ck++;   sk++;   np++;

/* release working storage before returning - assume no problems */
    (void) free(at);
    (void) free(sk);
    (void) free(bt);
    (void) free(ck);
    (void) free(np);
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

/************************** IS_POW_OF_TWO ****************************/

int is_pow_of_two(int val)
{
    int intbitlen, j, k;
    int sum = 0;
    intbitlen = sizeof(int) * CHARBITSIZE;
    j = 1;
    for(k=1;k<intbitlen;k++) {      //  Max positive number = 0100 0000 0000 0000
        if(j & val)                 //  so check all bits from 1 to 15
            sum++;                  //  "sum" counts number of bits set
        j = j << 1; 
    }
    if(sum == 1)
        return 1;
    return 0;
}

/************************** MATRIX_CYCLE ****************************/

int matrix_cycle(double *matrix,double *matrix2,dataptr dz)
{
    int n;
    for(n=0;n<dz->itemcnt;n++)
        dz->parray[0][n] += dz->parray[1][n];
    return FINISHED;
}

/************************** COMPLEX_PRODUCT ****************************
 *
 *  (a + bi) *  (c - di)  =     (ac + bd) + i(bc - ad)
 *   r1  i1      r2  i2      (r1r2 + i1i2)   (i1r2 - r1i2)
 *
 *
 */

float complex_product(float r1,float i1,float r2,float i2,float *iout)
{
    double rreal;
    double iimag;
    rreal = (r1 * r2) + (i1 * i2);
    iimag = (i1 * r2) - (r1 * i2);
    *iout = (float)iimag;
    return (float)rreal;
}
