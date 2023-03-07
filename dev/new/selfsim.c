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
int     sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

/* CDP LIBRARY FUNCTIONS TRANSFERRED HERE */

static int      set_param_data(aplptr ap, int special_data,int maxparamcnt,int paramcnt,char *paramlist);
static int  set_vflgs(aplptr ap,char *optflags,int optcnt,char *optlist,
                      char *varflags,int vflagcnt, int vparamcnt,char *varlist);
static int      setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int      initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int      mark_parameter_types(dataptr dz,aplptr ap);
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

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int allocate_selfsim_buffer(dataptr dz);
static int do_selfsim(dataptr dz);
static int create_order_and_scaling_arrays(dataptr dz);
static int create_dissimilarity_array(dataptr dz);

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
    if((exit_status = establish_datastructure(&dz))<0) {                                    // CDP LIB
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
        if((exit_status = make_initial_cmdline_check(&argc,&argv))<0) {         // CDP LIB
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
        if((exit_status = setup_the_application(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {           // CDP LIB
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
    // open_first_infile            CDP LIB
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

    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {              // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //check_param_validity_and_consistency .....
    is_launched = TRUE;

    dz->extra_bufcnt =  0;
    dz->bptrcnt = 4;

    if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = allocate_selfsim_buffer(dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = create_order_and_scaling_arrays(dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = create_dissimilarity_array(dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = do_selfsim(dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = complete_output(dz))<0) {                                                                             // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    exit_status = print_messages_and_close_sndfiles(FINISHED,is_launched,dz);               // CDP LIB
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
    ap->option_cnt   = (char) optcnt;                       /*RWD added cast */
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
    dz->array_cnt=3;
    if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
        return(MEMORY_ERROR);
    }
    dz->parray[0] = NULL;
    dz->parray[1] = NULL;
    dz->parray[2] = NULL;
    dz->larray_cnt=3;
    if((dz->lparray  = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for internal long arrays.\n");
        return(MEMORY_ERROR);
    }
    dz->lparray[0] = NULL;
    dz->lparray[1] = NULL;
    dz->lparray[2] = NULL;
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
    int n, m;                                                       /* PARAMS */
    for(n=0;n<ap->max_param_cnt;n++) {
        switch(ap->param_list[n]) {
        case('0'):      break; /* dz->is_active[n] = 0 is default */
        case('i'):      dz->is_active[n] = (char)1; dz->is_int[n] = (char)1;dz->no_brk[n] = (char)1; break;
        case('I'):      dz->is_active[n] = (char)1;     dz->is_int[n] = (char)1;                                                 break;
        case('d'):      dz->is_active[n] = (char)1;                                                     dz->no_brk[n] = (char)1; break;
        case('D'):      dz->is_active[n] = (char)1;     /* normal case: double val or brkpnt file */     break;
        default:
            sprintf(errstr,"Programming error: invalid parameter type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }                                                               /* OPTIONS */
    for(n=0,m=ap->max_param_cnt;n<ap->option_cnt;n++,m++) {
        switch(ap->option_list[n]) {
        case('i'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1; dz->no_brk[m] = (char)1; break;
        case('I'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;                                                  break;
        case('d'): dz->is_active[m] = (char)1;                                                  dz->no_brk[m] = (char)1; break;
        case('D'): dz->is_active[m] = (char)1;  /* normal case: double val or brkpnt file */     break;
        default:
            sprintf(errstr,"Programming error: invalid option type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }                                                               /* VARIANTS */
    for(n=0,m=ap->max_param_cnt + ap->option_cnt;n < ap->variant_param_cnt; n++, m++) {
        switch(ap->variant_list[n]) {
        case('0'): break;
        case('i'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1; dz->no_brk[m] = (char)1; break;
        case('I'): dz->is_active[m] = (char)1; dz->is_int[m] = (char)1;                                                  break;
        case('d'): dz->is_active[m] = (char)1;                                                  dz->no_brk[m] = (char)1; break;
        case('D'): dz->is_active[m] = (char)1; /* normal case: double val or brkpnt file */              break;
        default:
            sprintf(errstr,"Programming error: invalid variant type in mark_parameter_types()\n");
            return(PROGRAM_ERROR);
        }
    }                                                               /* INTERNAL */
    for(n=0,
            m=ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt; n<ap->internal_param_cnt; n++,m++) {
        switch(ap->internal_param_list[n]) {
        case('0'):  break;       /* dummy variables: variables not used: but important for internal paream numbering!! */
        case('i'):      dz->is_int[m] = (char)1;        dz->no_brk[m] = (char)1;        break;
        case('d'):                                                              dz->no_brk[m] = (char)1;        break;
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
    if((exit_status = establish_application(dz))<0)         // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,0   ,1,1,"i"))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
        return(FAILED);
    dz->has_otherfile = FALSE;
    dz->input_data_type = ANALFILE_ONLY;
    dz->process_type        = EQUAL_ANALFILE;
    dz->outfiletype         = ANALFILE_OUT;
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
    dz->clength             = dz->wanted / 2;
    dz->chwidth     = dz->nyquist/(double)(dz->clength-1);
    dz->halfchwidth = dz->chwidth/2.0;
    dz->wlength             = dz->insams[0]/dz->wanted;
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
    ap->lo[0]       = 1;
    ap->hi[0]       = dz->wlength - 1;
    ap->default_val[0]      = 1;
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
                cnt = PRE_CMDLINE_DATACNT;      /* force exit from loop after assign_file_data_storage */
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
                    dz->brksize[dz->extrabrkno]     = inbrksize;
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

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
    return(FINISHED);
}

int read_special_data(char *str,dataptr dz)
{
    return(FINISHED);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if      (!strcmp(prog_identifier_from_cmdline,"selfsim"))       dz->process = SELFSIM;
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        return(USAGE_ONLY);
    }
    return(FINISHED);
}

/**************************** ALLOCATE_SPECLEAN_BUFFER ******************************/

int allocate_selfsim_buffer(dataptr dz)
{
    unsigned int buffersize;
    buffersize = dz->wanted * 2;
    dz->buflen = dz->wanted;
    if((dz->bigfbuf = (float*) malloc(buffersize * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize = dz->buflen;
    dz->flbufptr[0]  = dz->bigfbuf;
    dz->flbufptr[1]  = dz->bigfbuf + dz->big_fsize;
    dz->flbufptr[2]  = dz->flbufptr[1] + dz->big_fsize;
    return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
    fprintf(stderr,
            "\nINCREASE SPECTRAL SELF-SIMILARITY\n\n"
            "USAGE: selfsim selfsim inanalfile outanalfile param\n"
            "\n"
            "Type 'selfsim selfsim' for more info.\n");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"selfsim")) {
        fprintf(stdout,
                "USAGE:\n"
                "selfsim selfsim inanalfile outanalfile self-similarity-index\n"
                "\n"
                "Replaces spectral windows with most similar, louder window(s).\n"
                "\n"
                "SELF-SIMILARITY-INDEX    Number of similar windows to replace.\n"
                "      Value 1 uses loudest window to replace the most similar window.\n"
                "      then the next loudest window to replace window most similar to it\n"
                "      and so on, with appropriate overall-loudness scaling.\n"
                "      With value 2, loudest windows replaces the TWO most simil windows.\n"
                "      and so on.\n"
                "      If window A replaces B, and C is most simil to B,\n"
                "      then A also replaces C, etc\n"
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

/*
 *      IDEA: make spectrum more selfsimilar.
 *
 *      (0)     List all window numbers 0 - wlength-1 in int array W, and total-window-loudnesses in array L
 *      (1)     Scale all values in array L so 1 is maximum ....
 *      (2)     Create a scaling value array S, and set all these initially to 1.0
 *      (3)     List windows in loudness order in int-array Z
 *      (4) Scaling each window to max loudness, compare it with every other window, and assign a similarity value
 *              in a 1/2 matrix of values. (Lack of) Similarity = sum of all absolute differences between levels in all channels
 *              (Deal with 0 level windows!!)
 *      (5)     For each z in Z, find the K most similar windows k1,k2 etc, and replace those window numbers in W by a "z",
 together with a scaling value in S to take amps of z into orig amp of the k1,k2 etc items
 NB replace
 *      (6)     Remove all other (z,n) (n,z) from the matrix, by setting similarity val to -1
 *      (7)     for every k replaced, remove all (n,k) and (k,n) from the matrix, by setting similarity val to -1
 *      (7)     Continue until Z exhausted or all similarity values set to -1 (keep a count of similarity vals)
 *
 *      (4) For step 4, if z replaces g, compare similarity value of next most similar to z, and next most similar to g
 *              replacing whichever is the closer ..... or is this too complicated (once we have a chain of N comparisons!!!)
 *
 *
 *      Once we have a new order of windows
 *      (1)     Get the window
 *      (2)     Apply the scaling value
 *      (3)     Output the window
 */

//
//      ibuf1, ibuf2, dissarray size (wlength  * wlength)/2


/*************************************** CREATE_ORDER_AND_SCALING_ARRAYS ***************************************/

int create_order_and_scaling_arrays(dataptr dz) {

    int n, m, itemp, samps_read;
    int cc, vc;
    float *ibuf = dz->flbufptr[0];
    double *loudness;       //      (relative) loudness array, in original window order: size wlength
    double *scaling;        //      Loudness scaling applied to windows in final output: size wlength
    int *winorder;          //      Initial (and, later, Final) order of windows: size wlength
    int *loudorder; //      List of windows when arranged in decreasing loudness order:  size wlength
    int *winused;           //      List of windows already self-sim set
    double loud, maxloud = 0.0;

    if((dz->parray[0]  = (double *)malloc(dz->wlength * dz->wlength * sizeof(double)))==NULL) {
        sprintf(errstr,"Setting up array for similairty measure.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[1]  = (double *)malloc(dz->wlength * sizeof(double)))==NULL) {
        sprintf(errstr,"Setting up array for loudness values.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[2]  = (double *)malloc(dz->wlength * sizeof(double)))==NULL) {
        sprintf(errstr,"Setting up array for loudness scaling.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[0]  = (int *)malloc(dz->wlength * sizeof(int)))==NULL) {
        sprintf(errstr,"Setting up array for final window order.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[1]  = (int *)malloc(dz->wlength * sizeof(int)))==NULL) {
        sprintf(errstr,"Setting up array for window decreasing loudness order.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[2]  = (int *)malloc(dz->wlength * sizeof(int)))==NULL) {
        sprintf(errstr,"Setting up array for marking windows used.\n");
        return(MEMORY_ERROR);
    }
    loudness  = dz->parray[1];
    scaling   = dz->parray[2];
    winorder  = dz->lparray[0];
    loudorder = dz->lparray[1];
    winused   = dz->lparray[2];
    for(n = 0;n < dz->wlength; n++) {
        winorder[n] = n;                                                        //      Store window numbers in their initial order
        loudorder[n] = n;                                                       //      Store window numbers in their order of decreasing LOUDNESS
        winused[n] = 0;                                                         //      Flag windows which have been processed (initially none)

        if((samps_read  = fgetfbufEx(ibuf,dz->wanted,dz->ifd[0],0))<0) {
            sprintf(errstr,"Sound read error 1: %s\n",sferrstr());
            return(SYSTEM_ERROR);
        }
        loud = 0.0;                                                                     //      Calculate and store total amplitude of window
        for(cc=0,vc = 0;cc<dz->clength;cc++,vc+=2)
            loud += ibuf[AMPP];
        loudness[n] = loud;
        maxloud = max(loud,maxloud);                            //      Keep track of maximum window-amp
    }
    if(maxloud <= 0.0) {
        sprintf(errstr,"No significant level found in spectrum.\n");
        return DATA_ERROR;
    }
    for(n = 0;n < dz->wlength; n++) {
        loudness[n] /= (double)maxloud;                         //      Normalise window-loudness measures
        scaling[n] = 1.0;                                                       //      Preset output-scaling values to 1.0
    }

    //      Create array of window-numbers in loudness order

    for(n = 0;n < dz->wlength-1; n++) {
        for(m = n+1;m < dz->wlength; m++) {
            if(loudness[loudorder[m]] > loudness[loudorder[n]]) {
                itemp = loudorder[m];
                loudorder[m] = loudorder[n];
                loudorder[n] = itemp;
            }
        }
    }
    return FINISHED;
}

/*************************************** CREATE_DISSIMILARITY_ARRAY ***************************************/

int create_dissimilarity_array(dataptr dz) {

    int n, m, k1, k2, nbase, mbase, samps_read, zz = 0, measure = 256 * dz->wanted;
    int cc, vc;
    double *dissarray = dz->parray[0];              //      Dissimilarity-measure array: size (wlength * wlength);
    double *loudness  = dz->parray[1];
    float *ibuf1 = dz->flbufptr[0], *ibuf2 = dz->flbufptr[1];
    double dissim;


    fprintf(stdout,"INFO: Assessing self-similarity of %d windows.\n",dz->wlength);
    fflush(stdout);
    for(n = 0;n < dz->wlength-1; n++) {
        nbase = n * dz->wlength;
        sndseekEx(dz->ifd[0],n * dz->wanted,0); //      Seek to next window, and read it
        if((samps_read  = fgetfbufEx(ibuf1,dz->wanted,dz->ifd[0],0))<0) {
            sprintf(errstr,"Sound read error 1: %s\n",sferrstr());
            return(SYSTEM_ERROR);
        }
        for(m = n+1;m < dz->wlength; m++) {
            mbase = m * dz->wlength;
            if((samps_read  = fgetfbufEx(ibuf2,dz->wanted,dz->ifd[0],0))<0) {
                sprintf(errstr,"Sound read error 2: %s\n",sferrstr());
                return(SYSTEM_ERROR);
            }
            dissim = 0.0;
            for(cc=0,vc = 0;cc<dz->clength;cc++,vc+=2)
                //      (1)     Scale the amplitudes of the windows to match maxamp
                //      (2)     Frq component only significant in lowest channels
                dissim += fabs((ibuf1[AMPP]/loudness[n]) - (ibuf2[AMPP]/loudness[m])) + fabs(ibuf1[FREQ] - ibuf2[FREQ]);
            k1 = nbase + m;
            dissarray[k1] = dissim/(double)dz->clength;
            k2 = mbase + n;
            dissarray[k2] = dissarray[k1];          //      Make a symmetrical array: easier later!!!
            if(++zz % measure == 0) {
                fprintf(stdout,"INFO: Assessed %d windows.\n",n+1);
                fflush(stdout);
            }
        }

    }
    fprintf(stdout,"INFO: Assessed %d windows.\n",dz->wlength);
    fflush(stdout);
    for(n = 0;n < dz->wlength; n++) {
        k1 = (n * dz->wlength) + n;
        dissarray[k1] = 0.0;                                    //      Neutralise the diagonal ... don't compare window with itself
    }
    return FINISHED;
}

/*************************************** DO_SELFSIM ***************************************/

int do_selfsim(dataptr dz)
{
    int exit_status, selfsim_cnt = dz->iparam[0], cc, vc;
    float *ibuf = dz->flbufptr[0];
    double minval;
    int n, m, k,jj, windowsset = 0, thiswindow, thisclose, closest, arrayclosest, lastwindow, samps_read;
    double *dissarray = dz->parray[0];              //      Dissimilarity-measure array: size (wlength * wlength)/2;
    double *loudness  = dz->parray[1];              //      (relative) loudness array, in window order: size wlength
    double *scaling   = dz->parray[2];              //      Loudness scaling applied to windows in final output order
    int *winorder    = dz->lparray[0];              //      Final order of windows: size wlength
    int *loudorder   = dz->lparray[1];              //      List of windows when arranged in decreasing loudnewss order:  size wlength
    int *winused      = dz->lparray[2];             //      List of windows already self-sim set

    fprintf(stdout,"INFO: Reconstructing self-similarity data.\n");
    fflush(stdout);
    for(n = 0; n < dz->wlength;n++) {
        thiswindow = loudorder[n];                              //      Get next loudest window
        if(winused[thiswindow])                                 //      If this window has already been self-similared,
            jj = winorder[thiswindow];                      //      use the self-similar window again,
        else                                                                    //      else use this window
            jj = thiswindow;
        for(k = 0; k < selfsim_cnt;k++) {
            thisclose = thiswindow * dz->wlength;   //      Find matrix row entry gives first dissimilarity value in array
            minval = HUGE;                                          //      Set this as LEAST dissimilar = most similar
            closest = 0;                                            //      SAFETY
            arrayclosest = thisclose;                       //      SAFETY
            for(m = 0;m < dz->wlength;m++) {        //      Find the most similar
                if(m==n)
                    continue;                                       //      No self-comparison
                if(dissarray[thisclose] < minval) {
                    closest = m;
                    minval = dissarray[thisclose];
                    arrayclosest = thisclose;
                }
                thisclose++;
            }
            winorder[closest] = jj;                         // In the output window order, replace window "closest" by window "jj"
            scaling[closest]  = loudness[closest]/loudness[jj];     //      Scale to an appropriate amplitude


            winused[closest] = 1;                           //      Mark window as already having being assigned
            dissarray[arrayclosest] = HUGE;         //      Avoid it being used again when looking for next most selfsim window
            windowsset++;                                           //      Count number of windows set
            if(windowsset >= dz->wlength)
                break;
        }
        winused[n] = 1;
        windowsset++;
        if(windowsset >= dz->wlength)
            break;
    }
    lastwindow = -1;
    fprintf(stdout,"INFO: Creating self-similar output.\n");
    fflush(stdout);
    for(n = 0; n < dz->wlength;n++) {
        thiswindow = winorder[n];
        if(thiswindow != lastwindow) {
            sndseekEx(dz->ifd[0],thiswindow * dz->wanted,0);        //      Seek to next window, and read it
            if((samps_read  = fgetfbufEx(ibuf,dz->wanted,dz->ifd[0],0))<0) {
                sprintf(errstr,"Sound read error 3: %s\n",sferrstr());
                return(SYSTEM_ERROR);
            }
        }
        for(cc=0,vc = 0;cc < dz->clength;cc++,vc+=2)
            ibuf[AMPP] = (float)(ibuf[AMPP] * scaling[n]);
        if((exit_status = write_samps(ibuf,dz->wanted,dz))<0)
            return(exit_status);
        lastwindow = thiswindow;
    }
    return FINISHED;
}
