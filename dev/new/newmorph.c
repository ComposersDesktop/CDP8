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
 
//
//  MPH_COS = 0

#define NMPH_COSTABSIZE (512)

#include <stdio.h>
#include <stdlib.h>
#include <osbind.h>
#include <structures.h>
#include <filetype.h>
#include <pnames.h>
#include <tkglobals.h>
#include <globcon.h>
#include <modeno.h>
#include <logic.h>
#include <morph.h>
#include <cdpmain.h>
#include <formants.h>
#include <speccon.h>
#include <sfsys.h>
#include <string.h>
#include <morph.h>
#include <standalone.h>
#include <speccon.h>
// #include <filetype.h>
// #include <modeno.h>
 #include <ctype.h>

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];
const char* cdp_version = "7.1.0";

/* extern */ int sloom = 0;
/* extern */ int    sloombatch = 0;
/* extern */ int anal_infiles = 1;
/* extern */ int is_converted_to_stereo = -1;

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
//static int  store_wordlist(char *filename,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

/* CDP LIB FUNCTION MODIFIED TO AVOID CALLING setup_particular_application() */

static int  parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);

/* SIMPLIFICATION OF LIB FUNC TO APPLY TO JUST THIS FUNCTION */

static int  parse_infile_and_check_type(char **cmdline,dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
static int  setup_specmorph_application(dataptr dz);
static int  setup_specmorph_param_ranges_and_defaults(dataptr dz);
static int  open_the_first_infile(char *filename,dataptr dz);
static int  handle_the_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz);

static int get_the_mode_from_cmdline(char *str,dataptr dz);

static int  read_specific_file_to_specific_buf(int fileno,int *windows_to_buf,float *floatbuf,dataptr dz);
static int  warped_cosin(double *interpratio,double exponent,dataptr dz);
static int  time_warp(double *interpratio,double exponent,dataptr dz);
static int  cosin_lookup(double *interpratio,dataptr dz);

static int do_newmorph(double alen, dataptr dz);
static void newmorph_core(double interpratio,dataptr dz);
static int find_the_average_spectral_peaks(dataptr dz);
static int make_newmorph_costable(dataptr dz);
static int create_peakcnt_stores(dataptr dz);
static int morph_preprocess(dataptr dz);
static int check_consistency_of_newmorph_params(dataptr dz);
static int allocate_new_triple_buffer(dataptr dz);
static void find_the_spectral_peaks(int *peakcnt,dataptr dz);
static void do_morphenv(dataptr dz);
static int find_the_average_spectral_peaks_and_correlates(dataptr dz);
static int handle_the_special_data(char *,dataptr dz);
static int read_the_special_data(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
/*  FILE *fp   = NULL; */
    dataptr dz = NULL;
//  char *special_data_string = NULL;
    char **cmdline;
    int  cmdlinecnt;
    //aplptr ap;
    int is_launched = FALSE;

                        /* CHECK FOR SOUNDLOOM */
//TW UPDATE
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if((sloom = sound_loom_in_use(&argc,&argv)) > 1) {
        sloom = 0;
        sloombatch = 1;
    }
    if(sflinit("cdp")){
        sfperror("cdp: initialisation\n");
        return(FAILED);
    }

                          /* SET UP THE PRINCIPLE DATASTRUCTURE */
    if((exit_status = establish_datastructure(&dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(!sloom) {
                          /* INITIAL CHECK OF CMDLINE DATA */
        if((exit_status = make_initial_cmdline_check(&argc,&argv))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdline    = argv;  /* GET PRE_DATA, ALLOCATE THE APPLICATION, CHECK FOR EXTRA INFILES */
        cmdlinecnt = argc;
// get_process_and_mode_from_cmdline -->
        if (!strcmp(argv[0],"newmorph")) {  
            dz->process = SPECMORPH;
            dz->maxmode = 7;
        } else if (!strcmp(argv[0],"newmorph2")) {
            dz->process = SPECMORPH2;
            dz->maxmode = 3;
        }
        else
            return usage1();
        cmdline++;
        cmdlinecnt--;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_specmorph_application(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    } else {
        //parse_TK_data() =
        if((exit_status = parse_sloom_data(argc,argv,&cmdline,&cmdlinecnt,dz))<0) {     /* includes setup_particular_application()      */
            exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);/* and cmdlinelength check = sees extra-infiles */
            return(exit_status);         
        }
    }
    //ap = dz->application;

    // parse_infile_and_hone_type() = 
    if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // setup_param_ranges_and_defaults = MOVED IN THIS CASE ONLY TO LATER

                    /* OPEN FIRST INFILE AND STORE DATA, AND INFORMATION, APPROPRIATELY */

    if((exit_status = open_the_first_infile(cmdline[0],dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    cmdlinecnt--;
    cmdline++;
    if(dz->process == SPECMORPH) {
        if((exit_status = handle_the_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    if((exit_status = setup_specmorph_param_ranges_and_defaults(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // FOR display_virtual_time 

    if(dz->process == SPECMORPH) {
        if(dz->mode == 6)
            dz->tempsize = min(dz->insams[0],dz->insams[1]);
        else
            dz->tempsize = dz->insams[1] + ((int)round(dz->param[NMPH_STAG]/dz->frametime) * dz->wanted);
    } else 
            dz->tempsize = dz->insams[0];
    // handle_outfile
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

    // handle_formants
    // handle_formant_quiksearch
    if(dz->process == SPECMORPH2 && dz->mode != 0) {
        if((exit_status = handle_the_special_data(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdlinecnt--;
        cmdline++;
    }
    // handle_special_data
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(dz->process == SPECMORPH || dz->mode != 0) {
        if((exit_status = check_consistency_of_newmorph_params(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    // check_param_validity_and_consistency = 
    is_launched = TRUE;

    if((exit_status = allocate_new_triple_buffer(dz))<0){
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // param_preprocess = 
    // spec_process_file
    if((exit_status = morph_preprocess(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = specmorph(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(!(dz->process == SPECMORPH && dz->mode == 6)) {
        if((exit_status = complete_output(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
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
    int storage_cnt, n;
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
        dz->infilecnt = -2; /* flags 2 or more */
    // establish_bufptrs_and_extra_buffers
    dz->extra_bufcnt =  0;
    dz->bptrcnt = 5;
    // setup_internal_arrays_and_array_pointers()
    dz->iarray_cnt = 4;
    dz->array_cnt = 5;
    if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->array_cnt;n++)
        dz->parray[n] = NULL;
    if((dz->iparray  = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for internal integer arrays.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->iarray_cnt;n++)
        dz->iparray[n] = NULL;
    return establish_spec_bufptrs_and_extra_buffers(dz);
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

/************************* SETUP_SPECMORPH_APPLICATION *******************/

int setup_specmorph_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if(dz->process == SPECMORPH2) {
        switch(dz->mode) {
        case(0):
            if((exit_status = set_param_data(ap,0   ,6,1,"0000i0"    ))<0)
                return(FAILED);
            if((exit_status = set_vflgs(ap,  "",0,""  ,"",0,0,""))<0)
                return(FAILED);
            break;
        default:
            if((exit_status = set_param_data(ap,MPEAKS,6,4,"0dddi0"    ))<0)
                return(FAILED);
            if((exit_status = set_vflgs(ap,  "r",1,"D"  ,"",0,0,""))<0)
                return(FAILED);
            break;
        }
    } else {
        switch(dz->mode) {
        case(4):
        case(5):
            if((exit_status = set_param_data(ap,0   ,6,5,"ddddi0"    ))<0)
                return(FAILED);
            if((exit_status = set_vflgs(ap,  "r",1,"D"  ,""      ,0,0,""))<0)
                return(FAILED);
            break;
        case(6):
            if((exit_status = set_param_data(ap,0   ,6,2,"0000ii"    ))<0)
                return(FAILED);
            if((exit_status = set_vflgs(ap,  "",0,""  ,"enf"   ,3,0,"000"))<0)
                return(FAILED);
            break;
        default:
            if((exit_status = set_param_data(ap,0   ,6,5,"ddddi0"    ))<0)
                return(FAILED);
            if((exit_status = set_vflgs(ap,  "",0,""  ,"enf"   ,3,0,"000"))<0)
                return(FAILED);
            break;
        }
    }   
    dz->has_otherfile = FALSE;
    if(dz->process == SPECMORPH2) {
        if (dz->mode == 0) {
            dz->input_data_type = ANALFILE;
            dz->process_type    = TO_TEXTFILE;  
            dz->outfiletype     = TEXTFILE_OUT;
        } else {
            dz->input_data_type = ANALFILE;
            dz->process_type    = EQUAL_ANALFILE;   
            dz->outfiletype     = ANALFILE_OUT;
        }
    } else {
        dz->input_data_type = TWO_ANALFILES;
        dz->process_type    = BIG_ANALFILE; 
        dz->outfiletype     = ANALFILE_OUT;
    }
    return application_init(dz);    //GLOBAL
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
        sprintf(errstr,"setup_and_init_input_brktable_constants(): 6\n");
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
    if((exit_status = set_chunklens_and_establish_windowbufs(dz))<0)
        return(exit_status);
    return(FINISHED);
}

/************************* SETUP_SPECMORPH_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_specmorph_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // setup_input_param_range_stores()
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    if(dz->process == SPECMORPH2) {
        if(dz->mode != 0) {
            ap->lo[NMPH_ASTT] = 0.0;
            ap->hi[NMPH_ASTT] = (dz->wlength - 1) * dz->frametime;
            ap->default_val[NMPH_ASTT] = 0.0;
            ap->lo[NMPH_AEND] = dz->frametime * 2;
            ap->hi[NMPH_AEND] = BIG_TIME;
            ap->default_val[NMPH_AEND] = dz->duration;
            ap->lo[NMPH_AEXP] = 0.02;
            ap->hi[NMPH_AEXP] = 50.0;
            ap->default_val[NMPH_AEXP] = 1.0;
            ap->lo[NMPH_APKS] = 1;
            ap->hi[NMPH_APKS] = 16;
            ap->default_val[NMPH_APKS] = 8;
            ap->lo[NMPH_RAND] = 0;
            ap->hi[NMPH_RAND] = 1;
            ap->default_val[NMPH_RAND] = 1;
        } else {
            ap->lo[NMPH_APKS] = 1;
            ap->hi[NMPH_APKS] = 16;
            ap->default_val[NMPH_APKS] = 8;
        }
        dz->maxmode = 3;
    } else {
        if(dz->mode == 6) {
            ap->lo[NMPH_APKS] = 1;
            ap->hi[NMPH_APKS] = 16;
            ap->default_val[NMPH_APKS] = 8;
            ap->lo[NMPH_OCNT] = 1;
            ap->hi[NMPH_OCNT] = 64;
            ap->default_val[NMPH_OCNT] = 6;
        } else {
            ap->lo[NMPH_STAG] = 0.0;
            ap->hi[NMPH_STAG] = (dz->wlength-1) * dz->frametime;
            ap->default_val[NMPH_STAG] = 0.0;
            ap->lo[NMPH_ASTT] = 0.0;
            ap->hi[NMPH_ASTT] = (dz->wlength - 1) * dz->frametime;
            ap->default_val[NMPH_ASTT] = 0.0;
            ap->lo[NMPH_AEND] = dz->frametime * 2;
            ap->hi[NMPH_AEND] = BIG_TIME;
            ap->default_val[NMPH_AEND] = dz->duration;
            ap->lo[NMPH_AEXP] = 0.02;
            ap->hi[NMPH_AEXP] = 50.0;
            ap->default_val[NMPH_AEXP] = 1.0;
            ap->lo[NMPH_APKS] = 1;
            ap->hi[NMPH_APKS] = 16;
            ap->default_val[NMPH_APKS] = 8;
            if(dz->mode == 4 || dz->mode == 5) {
                ap->lo[NMPH_RAND] = 0;
                ap->hi[NMPH_RAND] = 1;
                ap->default_val[NMPH_RAND] = 1;
            }
        }
        dz->maxmode = 7;
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
            //setup_particular_application()=
            if((exit_status = setup_specmorph_application(dz))<0)
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
            /* RWD these changes to samps - tk will have to deal with that! */
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

/************************** OPEN_THE_FIRST_INFILE *****************************/

int open_the_first_infile(char *filename,dataptr dz)
{
    if((dz->ifd[0] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
        sprintf(errstr,"Failure to open file %s for input.\n",filename);
        return(SYSTEM_ERROR);
    }
    if(dz->infilecnt<=0 || dz->infile->filetype!=ANALFILE) {
        sprintf(errstr,"%s is wrong type of file for this process.\n",filename);
        return(DATA_ERROR);
    }
    dz->samps_left = dz->insams[0];
    return(FINISHED);
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
    } else {
        sprintf(errstr,"Insufficient input files for this process\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/************************ HANDLE_THE_OUTFILE *********************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz)
{
    int exit_status;
    char *filename = NULL, *p;
    int n;
    int stype = SAMP_FLOAT;

    n = strlen((*cmdline)[0]);
    if((filename = (char *)malloc((n + 10) * sizeof(char)))==NULL)  {
        sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
        return(MEMORY_ERROR);
    }
    if(sloom) {
        strcpy(filename,((*cmdline)[0]));
    } else {
        if(*cmdlinecnt<=0) {
            sprintf(errstr,"Insufficient cmdline parameters.\n");
            return(USAGE_ONLY);
        }
        strcpy(filename,((*cmdline)[0]));
        if(filename[0]=='-' && filename[1]=='f') {
            sprintf(errstr,"-f flag used incorrectly on command line (output is not a sound file).\n");
            return(USAGE_ONLY);
        }
        if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
            sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
            return(DATA_ERROR);
        }
    }
    if(dz->process == SPECMORPH2 && dz->mode == 0) {
        p = filename + strlen(filename);
        p--;
        while(*p != '.') {
            p--;
            if(p == filename)
                break;
        }
        if(*p == '.') {
            *p = ENDOFSTR;
            strcat(filename,".txt");
        }
        strcpy(dz->outfilename,filename);
        if(file_has_reserved_extension(filename)) {
            sprintf(errstr,"Cannot open a textfile (%s) with a reserved extension.\n",filename);
            return(USER_ERROR);
        }
        if((dz->fp = fopen(filename,"w"))==NULL) {
            sprintf(errstr,"Cannot open output file %s\n",filename);
            return(USER_ERROR);
        }
    } else {
        if(dz->process == SPECMORPH && dz->mode == 6) {
            p = filename + strlen(filename);
            p--;
            while(*p != '.') {
                p--;
                if(p == filename)
                    break;
            }
            if(*p == '.') {
                if(sloom)
                    p--;
                *p = ENDOFSTR;
            } else {
                if(sloom) {
                    p = filename + strlen(filename);
                    p--;
                    *p = ENDOFSTR;
                }
            }
            dz->all_words = 0;
            if((exit_status = store_filename(filename,dz))<0)
                return(exit_status);

            strcpy(dz->outfilename,dz->wordstor[0]);
            strcat(dz->outfilename,"0.ana");
        } else
            strcpy(dz->outfilename,filename);
        dz->true_outfile_stype = stype;
        dz->outfilesize = -1;
        if((dz->ofd = sndcreat_formatted(dz->outfilename,dz->outfilesize,stype,
                dz->infile->channels,dz->infile->srate,CDP_CREATE_NORMAL)) < 0) {
            sprintf(errstr,"Cannot open output file %s\n", dz->outfilename);
            return(DATA_ERROR);
        }
        dz->outchans = dz->infile->channels;
        dz->needpeaks = 1;
        dz->outpeaks = (CHPEAK *) malloc(sizeof(CHPEAK) * dz->outchans);
        if(dz->outpeaks==NULL)
            return MEMORY_ERROR;
        dz->outpeakpos = (unsigned int *) malloc(sizeof(unsigned int) * dz->outchans);
        if(dz->outpeakpos==NULL)
            return MEMORY_ERROR;
        for(n=0;n < dz->outchans;n++){
            dz->outpeaks[n].value = 0.0f;
            dz->outpeaks[n].position = 0;
            dz->outpeakpos[n] = 0;
        }
        strcpy(dz->outfilename,filename);
    }
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/******************************** USAGE ********************************/

int usage1(void)
{
    fprintf(stderr,
    "\nNEW TYPES OF SPECTRAL MORPHING\n\n"
    "USAGE: newmorph NAME mode infile1 infile2 outfile parameters: \n"
    "\n"
    "where NAME can be any one of\n"
    "\n"
    "newmorph  newmorph2\n\n"
    "Type 'newmorph newmorph' for more info on newmorph newmorph..ETC.\n");
    return(USAGE_ONLY);
}

int usage2(char *str)
{
    if(!strcmp(str,"newmorph")) {
        fprintf(stderr,
        "USAGE: newmorph newmorph 1-6 analfile1 analfile2 outanalfile\n"
        "           stagger  startmorph  endmorph  exponent  peaks  [-e] [-n] [-f]\n"
        "OR:    newmorph newmorph 7 analfile1 analfile2 outanalfile\n"
        "           peaks  outcnt  [-e] [-n] [-f]\n"
        "\n"
        "MORPH BETWEEN DISSIMILAR SPECTRA\n"
        "\n"
        "STAGGER    Time before entry of file 2.\n"
        "STARTMORPH Time (in 1st file) at which morph starts.\n"
        "ENDMORPH   Time (in 1st file) at which morph ends.\n"
        "EXPONENT   Exponent of interpolation.\n"
        "PEAKS      Number of peaks to interpolate.\n"
        "OUTCNT     Mode 7 makes \"outcnt\" distinct output files.\n"
        "-e         Retain the loudness envelope of the first sound.\n"
        "           (in this case output cuts off when 1st sound reaches its end).\n"
        "-n         No interpolation of anything except the peaks.\n"
        "-f         Only frq is determined by peaks in sound 2.\n"
        "\n"
        "MODES\n"
        "1   interpolate linearly (exp 1) between the average peak channels\n"
        "    or over a curve of increasing (exp >1) or decreasing (exp <1) slope,\n"
        "    simultaneously moving spectral peaks, and interpolating all remaining channels.\n"
        "\n"
        "2   interpolate cosinusoidally (exp 1) between the average peak channels\n"
        "    or over a warped cosinusoidal spline (exp not equal to 1),\n"
        "    simultaneously moving spectral peaks, and interpolating all remaining channels.\n"
        "\n"
        "3   As mode 1, using channel-by-channel calculation of peaks.\n"
        "4   As mode 2, using channel-by-channel calculation of peaks.\n"
        "5   Sound 1 (gradually) tuned to (averaged) harmonic field sound 2. Linear.\n"
        "6   Sound 1 (gradually) tuned to (averaged) harmonic field sound 2. Cosinusoidal.\n"
        "7   Sound 1 morphed towards sound2 in \"outcnt\" steps, each step a new output file.\n"
    "\n");
    } else if(!strcmp(str,"newmorph2")) {
        fprintf(stderr,
        "USAGE: newmorph newmorph2 1 analfileA outtextfile peakcnt\n"
        "\n"
        "Find frequencies of most prominent spectral peaks (in order of prominence).\n"
        "Ouput these as a textfile list, a \"peaksfile\".\n"
        "\n"
        "OR:    newmorph newmorph2 2-3 analfileB outanalfile peaksfile \n"  /* RWD 11-20, corrected order */
        "           startmorph  endmorph  exponent  peakcnt  [-rrand]\n"
        "\n"
        "Morph between dissimilar spectra\n"
         "(Peaks of analfileB move towards peaks extracted from analfileA).\n"
        "\n"
        "PEAKCNT    Number of most-prominent source-peaks to find.\n"
        "PEAKSFILE  Textfile listing goal-peak frequencies: most prominent first.\n"
        "STARTMORPH Time at which morph starts.\n"
        "ENDMORPH   Time at which morph ends.\n"
        "EXPONENT   Exponent of interpolation.\n"
        "-r         Randomisation of the goal peak frequency. (Range 0 - 1)\n"
        "\n"
        "MODES\n"
        "\n"
        "2   Sound (gradually) tuned to harmonic field specified in textfile \"peaksfile\".\n"
        "3   Ditto, but interpolation is timewise-cosinusoidal.\n"
        "\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    sprintf(errstr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
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

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
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

/***************************** SPECMORPH ***********************/

int specmorph(dataptr dz)
{
#define FILE0 (0)
#define FILE1 (1)

    int     exit_status, cc, vc, peakcnt, n, m;
    int windows_to_buf0 = 0, windows_to_buf1 = 0, minsize;
    int start_of_morph = 0, end_of_morph = 0;
    double  alen = 0.0, interpratio;
    int window_position_in_current_buf_for_file1;
    int total_windows_processed_from_file0 = 0, 
            total_windows_processed_from_file1 = 0;
    int     finished_file0 = 0, 
            finished_file1 = 0;
    int total_windows_in_file0 = dz->insams[0]/dz->wanted;
    int total_windows_in_file1 = dz->insams[1]/dz->wanted;
    double *peakfrq1 = dz->parray[3], *peakamp2 = dz->parray[2];
    char    temp[64];
    int stype = SAMP_FLOAT;

    
    if(dz->process == SPECMORPH) {
        if (dz->mode == 6) {
            start_of_morph  = 0;
            minsize = dz->insams[0];
            for(n=1;n<dz->infilecnt;n++)
                minsize = min(dz->insams[n],minsize);
            dz->wlength = minsize/dz->wanted;
            end_of_morph = dz->wlength;
        } else {
            start_of_morph  = dz->iparam[NMPH_ASTT];
            end_of_morph    = dz->iparam[NMPH_AEND];
            alen            = (double)(dz->iparam[NMPH_AEND] - dz->iparam[NMPH_ASTT]);
        }
    } else if(dz->mode > 0) {   //  SPECMORPH2
        start_of_morph  = dz->iparam[NMPH_ASTT];
        end_of_morph    = dz->iparam[NMPH_AEND];
        alen            = (double)(dz->iparam[NMPH_AEND] - dz->iparam[NMPH_ASTT]);
    }
    if(dz->process == SPECMORPH && dz->mode != 6)
        window_position_in_current_buf_for_file1 = -dz->iparam[NMPH_STAG];
    else
        window_position_in_current_buf_for_file1 = 0;

    fflush(stdout);
    if(dz->process == SPECMORPH && dz->mode < 2) {
        fprintf(stdout,"INFO: Searching for spectral peaks\n");
        if((exit_status = find_the_average_spectral_peaks(dz)) < 0)
            return exit_status;
    } else if((dz->process == SPECMORPH && dz->mode >= 4 && dz->mode != 6) || dz->process == SPECMORPH2) {
        if(dz->process == SPECMORPH2 && dz->mode > 0)
            fprintf(stdout,"INFO: Assigning spectral peaks\n");
        else
            fprintf(stdout,"INFO: Searching for spectral peaks\n");
        if((exit_status = find_the_average_spectral_peaks_and_correlates(dz)) < 0)
            return exit_status;
    }
    if(dz->process == SPECMORPH2 && dz->mode == 0) {
        peakcnt = dz->iparam[NMPH_APKS];
        for(cc = 0;cc < peakcnt;cc++) {
            if((fprintf(dz->fp,"%lf\n",peakamp2[cc]))<1) {
                sprintf(errstr,"fput_brk(): Problem writing peak data to file.\n");
                return(PROGRAM_ERROR);
            }
        }
        return FINISHED;
    }
    if(dz->process == SPECMORPH && dz->mode == 6) {
        for(n = 0,m = 1; n < dz->iparam[NMPH_OCNT];n++,m++) {
            fprintf(stdout,"INFO: Generating file %d\n",m);
            fflush(stdout);
            interpratio = (double)m/(double)(dz->iparam[NMPH_OCNT] + 2);
            dz->total_windows = 0;
            while(dz->total_windows < end_of_morph) {
                if((exit_status = read_specific_file_to_specific_buf(FILE0,&windows_to_buf0,dz->flbufptr[0],dz))<0)
                    return(exit_status);
                if((exit_status = read_specific_file_to_specific_buf(FILE1,&windows_to_buf1,dz->flbufptr[1],dz))<0)
                    return(exit_status);
                newmorph_core(interpratio,dz);
                if((exit_status = write_exact_samps(dz->bigfbuf,dz->big_fsize,dz))<0)
                    return(exit_status);
                dz->total_windows++;
            }
            dz->total_samps_written = 0;
            if((exit_status = headwrite(dz->ofd,dz))<0)
                return(exit_status);
            if(sndcloseEx(dz->ofd) < 0) {
                fprintf(stdout,"WARNING: Can't close output soundfile %s\n",dz->outfilename);
                fflush(stdout);
            }
            dz->ofd = -1;
            if(m < dz->iparam[NMPH_OCNT]) {
                if((exit_status = reset_peak_finder(dz))<0)
                    return(exit_status);
                strcpy(dz->outfilename,dz->wordstor[0]);
                sprintf(temp,"%d.ana",m);
                strcat(dz->outfilename,temp);
                if((dz->ofd = sndcreat_formatted(dz->outfilename,dz->outfilesize,stype,
                        dz->infile->channels,dz->infile->srate,CDP_CREATE_NORMAL)) < 0) {
                    sprintf(errstr,"Cannot open output file %s : %s\n", dz->outfilename,sferrstr());
                    return(DATA_ERROR);
                }
                if(sndseekEx(dz->ifd[0],0,0)<0) {
                    sprintf(errstr,"Sound seek failed in file 1: after file %d\n",m);
                    return SYSTEM_ERROR;
                }
                if(sndseekEx(dz->ifd[1],0,0)<0) {
                    sprintf(errstr,"Sound seek failed in file 2: after file %d\n",m);
                    return SYSTEM_ERROR;
                }
            }
        }
        return(FINISHED);
    }
    dz->total_windows = 0;
    fprintf(stdout,"INFO: Doing the morph\n");
    fflush(stdout);
    do {
        if(dz->process == SPECMORPH && dz->mode < 4) {
            if(!finished_file0) {
                if(total_windows_processed_from_file0 >= end_of_morph) {
                    finished_file0 = 1;
                } else {
                    if((exit_status = read_specific_file_to_specific_buf(FILE0,&windows_to_buf0,dz->flbufptr[0],dz))<0)
                        return(exit_status);
                }
            }
            if(window_position_in_current_buf_for_file1>=0) {
                if((exit_status = read_specific_file_to_specific_buf(FILE1,&windows_to_buf1,dz->flbufptr[1],dz))<0)
                    return(exit_status);
                if(total_windows_processed_from_file0 >= start_of_morph) {
                    if(!finished_file0) {
                        if((exit_status = do_newmorph(alen,dz))<0)
                            return(exit_status);
                    } else if(dz->vflag[0]) {
                        if(total_windows_processed_from_file0 < total_windows_in_file0)
                            do_morphenv(dz);
                        else
                            break;
                    } else
                        memmove((char *)dz->flbufptr[0],(char *)dz->flbufptr[1],(size_t)(dz->wanted * sizeof(float)));
                }
                if(++total_windows_processed_from_file1 >= total_windows_in_file1)
                    finished_file1 = 1;
            }
            if((exit_status = write_exact_samps(dz->bigfbuf,dz->big_fsize,dz))<0)
                return(exit_status);
            total_windows_processed_from_file0++;
            window_position_in_current_buf_for_file1++;
        } else {
            if(total_windows_processed_from_file0 < total_windows_in_file0) {
                if((exit_status = read_specific_file_to_specific_buf(FILE0,&windows_to_buf0,dz->flbufptr[0],dz))<0)
                    return(exit_status);
            }
            if(dz->process == SPECMORPH) {
                if(window_position_in_current_buf_for_file1>=0) {
                    if((exit_status = read_specific_file_to_specific_buf(FILE1,&windows_to_buf1,dz->flbufptr[1],dz))<0)
                        return(exit_status);
                    if(total_windows_processed_from_file0 >= start_of_morph) {
                        if(total_windows_processed_from_file0 < end_of_morph ) {
                            if((exit_status = do_newmorph(alen,dz))<0)
                                return(exit_status);
                        } else if(total_windows_processed_from_file0 < total_windows_in_file0) {
                            for(cc=0,vc = 0;cc < dz->clength;cc++,vc+=2)
                                dz->flbufptr[0][FREQ] = (float)peakfrq1[cc];
                        } else
                            break;
                    }
                    if(++total_windows_processed_from_file1 >= total_windows_in_file1)
                        finished_file1 = 1;
                }
            } else {
                if(total_windows_processed_from_file0 >= start_of_morph) {
                    if(total_windows_processed_from_file0 < end_of_morph ) {
                        if((exit_status = do_newmorph(alen,dz))<0)
                            return(exit_status);
                    } else if(total_windows_processed_from_file0 < total_windows_in_file0) {
                        for(cc=0,vc = 0;cc < dz->clength;cc++,vc+=2)
                            dz->flbufptr[0][FREQ] = (float)peakfrq1[cc];
                    } else
                        break;
                }
            }
            if((exit_status = write_exact_samps(dz->bigfbuf,dz->big_fsize,dz))<0)
                return(exit_status);
            total_windows_processed_from_file0++;
            window_position_in_current_buf_for_file1++;
        }
    } while(!finished_file1);
    
    return(FINISHED);
}

/***************************** READ_SPECIFIC_FILE_TO_SPECIFIC_BUF ***********************/


int read_specific_file_to_specific_buf(int fileno,int *windows_to_buf,float *floatbuf,dataptr dz)
{
    int samps_read;
    if((samps_read = fgetfbufEx(floatbuf,dz->big_fsize,dz->ifd[fileno],0))<=0) {
        if(samps_read<0) {
            sprintf(errstr,"Sound read error.\n");
            return(SYSTEM_ERROR);
        } else {
            sprintf(errstr,"Error in buffer arithmetic in read_specific_file_to_specific_buf()\n");
            return(PROGRAM_ERROR);
        }
    }
    *windows_to_buf = samps_read/dz->wanted;
    return(FINISHED);
}

/*************************************** DO_NEWMORPH ******************************************/

int do_newmorph(double alen, dataptr dz)
{
    int exit_status;
    double interpratio, time;
    interpratio = (double)dz->total_windows/(double)alen;
    if(dz->process == SPECMORPH) {
        switch(dz->mode) {
        case(NMPH_LINE):
        case(NMPH_LINEX):
            interpratio = pow(interpratio,dz->param[NMPH_AEXP]);
            break;
        case(NMPH_LINET):  
            interpratio = pow(interpratio,dz->param[NMPH_AEXP]);
            time = (double)dz->total_windows * dz->frametime;
            if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                return(exit_status);
            break;
        case(NMPH_COSIN):
        case(NMPH_COSINX):
            if((exit_status = warped_cosin(&interpratio,dz->param[NMPH_AEXP],dz))<0)
                return(exit_status);
            break;
        case(NMPH_COSINT):
            if((exit_status = warped_cosin(&interpratio,dz->param[NMPH_AEXP],dz))<0)
                return(exit_status);
            time = (double)dz->total_windows * dz->frametime;
            if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                return(exit_status);
            break;
        }
    } else {
        switch(dz->mode) {
        case(1): 
            time = (double)dz->total_windows * dz->frametime;
            if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                return(exit_status);
            break;
        case(2):
            if((exit_status = warped_cosin(&interpratio,dz->param[NMPH_AEXP],dz))<0)
                return(exit_status);
            time = (double)dz->total_windows * dz->frametime;
            if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                return(exit_status);
            break;
        }
    }
    newmorph_core(interpratio,dz);
    dz->total_windows++;
    return(FINISHED);
}   

/****************************** WARPED_COSIN ******************************/

int warped_cosin(double *interpratio,double exponent,dataptr dz)
{
    int exit_status;
    if((exit_status = cosin_lookup(interpratio,dz))<0)
        return(exit_status);
    if(*interpratio < .5) {
        if((exit_status = time_warp(interpratio,exponent,dz))<0)
        return(exit_status);
    } else if(*interpratio > .5) {
        *interpratio = 1.0 - *interpratio;
        if((exit_status = time_warp(interpratio,exponent,dz))<0)
        return(exit_status);
        *interpratio = 1.0 - *interpratio;
    }
    return(FINISHED);
}

/****************************** TIME_WARP ******************************/

int time_warp(double *interpratio,double exponent,dataptr dz)
{
    *interpratio = min(*interpratio * 2.0,1.0);
    *interpratio = pow(*interpratio,exponent);
    *interpratio /= 2.0;
    return(FINISHED);
}

/****************************** COSIN_LOOKUP ******************************/

int cosin_lookup(double *interpratio,dataptr dz)
{
    double dtabindex = *interpratio * (double)NMPH_COSTABSIZE;
    int tabindex = (int)floor(dtabindex);   /* TRUNCATE */
    double frac  = dtabindex - (double)tabindex;
    double thisval = dz->parray[0][tabindex++]; /* table has wraparound pnt */
    double nextval = dz->parray[0][tabindex];
    double diff = nextval - thisval;
    double step = diff * frac;
    *interpratio = thisval + step;
    return(FINISHED);
}

/********************** CHECK_CONSISTENCY_OF_NEWMORPH_PARAMS **********************/

int check_consistency_of_newmorph_params(dataptr dz)
{
    double duration0, duration1 = 0.0;
    int morphing = 0;
    if(dz->process == SPECMORPH && dz->mode == 6)
        return FINISHED;
    duration0  = (dz->insams[0]/dz->wanted) * dz->frametime;
    if(dz->process == SPECMORPH) {
        duration1 = ((dz->insams[1]/dz->wanted) * dz->frametime) + dz->param[NMPH_STAG];
        if(dz->param[NMPH_ASTT] < dz->param[NMPH_STAG]) {
            sprintf(errstr,"start of amp interpolation is set before entry of 2nd soundfile.\n");
            return(DATA_ERROR);
        }
        dz->iparam[NMPH_STAG] = round(dz->param[NMPH_STAG]/dz->frametime);
    }
    if(dz->process == SPECMORPH || dz->mode > 0)    //  i.e. also if process is SPECMORPH2 and we're not in mode 0
        morphing = 1;
    if(morphing) {
        if(dz->process == SPECMORPH) {
            if(dz->param[NMPH_AEND] > duration0 || dz->param[NMPH_AEND] > duration1) {
                sprintf(errstr, "end of interpolation is beyond end of one of soundfiles.\n");
                return(DATA_ERROR);
            }
        } else {
            if(dz->param[NMPH_AEND] > duration0) {
                fprintf(stdout, "WARNING : end of interpolation beyond end of soundfile, truncated to file duration.\n");
                fflush(stdout);
                dz->param[NMPH_AEND] = duration0;
            }
        }
        if(dz->param[NMPH_AEND] <= dz->param[NMPH_ASTT]) {
            sprintf(errstr,"interpolation starttime is after (or equal to) its endtime.\n");
            return(USER_ERROR);
        }
        dz->iparam[NMPH_AEND] = round(dz->param[NMPH_AEND]/dz->frametime);      //  Convert to count in windows
        dz->iparam[NMPH_ASTT] = round(dz->param[NMPH_ASTT]/dz->frametime);
    }
    return(FINISHED);
}

/************************** MORPH_PREPROCESS ******************************/

int morph_preprocess(dataptr dz)
{
    int exit_status;
    if((dz->process == SPECMORPH && (dz->mode==NMPH_COSIN || dz->mode==NMPH_COSINX || dz->mode==NMPH_COSINT)) \
    || (dz->process == SPECMORPH2 && dz->mode==2))  {
        if((exit_status = make_newmorph_costable(dz))<0)
            return(exit_status);
    }
    if(dz->process == SPECMORPH2 && dz->mode > 0)   {
        if((exit_status = create_peakcnt_stores(dz))<0)
            return(exit_status);
        return read_the_special_data(dz);
    }
    return create_peakcnt_stores(dz);
}

/************************ MAKE_MORPH_COSTABLE ***********************/

int make_newmorph_costable(dataptr dz)
{
    double d, d2;
    int n;
    d = PI/(double)NMPH_COSTABSIZE;
    if((dz->parray[0] = (double *)malloc((NMPH_COSTABSIZE + 1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for morph cosin table.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<NMPH_COSTABSIZE;n++) {
        d2 = cos((double)n * d);
        d2 += 1.0;
        d2 /= 2.0;
        d2 = 1.0 - d2;
        d2 = min(d2,1.0);   /* trap calc errors */
        d2 = max(d2,0.0);
        dz->parray[0][n] = d2;
    }
    dz->parray[0][n++] = 1.0;   /* wrap-around point */
    return(FINISHED);
}

/************************ CREATE_PEAKCNT_STORES ***********************/

int create_peakcnt_stores(dataptr dz)
{
    if((dz->iparray[0] = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for 1st peak count store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[1] = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for 2nd peak count store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[2] = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for peak location store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[3] = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for peak location store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[1] = (double *)malloc(dz->clength * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for peak levels store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[2] = (double *)malloc(dz->clength * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for peak levels store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[3] = (double *)malloc(dz->clength * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for peak levels store.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[4] = (double *)malloc(dz->clength * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for peak levels store.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/******************** FIND_THE_AVERAGE_SPECTRAL_PEAKS ****************************/

int find_the_average_spectral_peaks(dataptr dz)
{
    int exit_status;
    double *peakamp1 = dz->parray[1], *peakamp2   = dz->parray[2];
    double *peakfrq1 = dz->parray[3], *peakfrq2 = dz->parray[4];
    int *peakno1  = dz->iparray[1], *peakno2  = dz->iparray[2];
    int peakcnt = dz->iparam[NMPH_APKS];
    int n, m, k;
    double tempamp, tempfrq, peaklo, peakhi;
    int windows_to_buf = 0, samps_left, wcnt;
    int totwcnt = 0, cc, vc, done, peaks_possible, peaks_possible1 = 0, peaks_possible2 = 0;

    for(n=0;n < dz->clength;n++) {
        peakamp1[n] = 0.0;
        peakno1[n]  = n * 2;
        peakamp2[n] = 0.0;
        peakno2[n]  = n * 2;
    }
    samps_left = dz->insams[0];
    while(samps_left) {
        if((exit_status = read_specific_file_to_specific_buf(FILE0,&windows_to_buf,dz->flbufptr[0],dz))<0)
            return(exit_status);
        samps_left -= windows_to_buf * dz->wanted;
        for(wcnt =0;wcnt < windows_to_buf;wcnt++) {
            if(totwcnt == 0) {
                totwcnt++;
                continue;
            }
            for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {
                peakamp1[cc] += fabs(dz->flbufptr[0][AMPP]);
                if(totwcnt == 1)
                    peakfrq1[cc] = fabs(dz->flbufptr[0][FREQ]);                         //  Get total amplitude for each channel
                else
                    peakfrq1[cc] = (peakfrq1[cc] + fabs(dz->flbufptr[0][FREQ]))/2.0;    //  Get average freq for each channel
                totwcnt++;
            }
        }           
    }
    for(n=0;n<dz->clength;n++) {
        if(peakamp1[n] > 0.0)
            peaks_possible1++;
    }

    for(n=0;n < dz->clength - 1;n++) {                                          //  Sort into loudness order
        for(m = n+1; m < dz->clength;m++) {
            if(peakamp1[n] < peakamp1[m]) {
                tempamp = peakamp1[n];
                tempfrq = peakfrq1[n];
                k = peakno1[n];
                peakamp1[n] = peakamp1[m];
                peakfrq1[n] = peakfrq1[m];
                peakno1[n]  = peakno1[m];
                peakamp1[m] = tempamp;
                peakfrq1[m] = tempfrq;
                peakno1[m] = k;
            }
        }
    }
    if(sndseekEx(dz->ifd[0],0,0)<0) {
        sprintf(errstr,"Sound seek failed in file 1\n");
        return SYSTEM_ERROR;
    }

    samps_left = dz->insams[1];                         //  Same with 2nd file
    totwcnt = 0;
    while(samps_left) {
        if((exit_status = read_specific_file_to_specific_buf(FILE1,&windows_to_buf,dz->bigfbuf,dz))<0)
            return(exit_status);
        samps_left -= windows_to_buf * dz->wanted;
        for(wcnt =0;wcnt < windows_to_buf;wcnt++) {
            if(totwcnt == 0) {
                totwcnt++;
                continue;
            }
            for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {
                peakamp2[cc] += fabs(dz->flbufptr[0][AMPP]);
                if(totwcnt == 1)
                    peakfrq2[cc] = fabs(dz->flbufptr[0][FREQ]);
                else
                    peakfrq2[cc] = (peakfrq2[cc] + fabs(dz->flbufptr[0][FREQ]))/2.0;
            }
            totwcnt++;
        }           
    }
    for(n=0;n < dz->clength - 1;n++) {
        for(m = n+1; m < dz->clength;m++) {
            if(peakamp2[m] > peakamp2[n]) {
                tempamp = peakamp2[n];
                tempfrq = peakfrq2[n];
                k = peakno2[n];
                peakamp2[n] = peakamp2[m];
                peakfrq2[n] = peakfrq2[m];
                peakno2[n]  = peakno2[m];
                peakamp2[m] = tempamp;
                peakfrq2[m] = tempfrq;
                peakno2[m] = k;
            }
        }
    }
    if(sndseekEx(dz->ifd[1],0,0)<0) {
        sprintf(errstr,"Sound seek failed in file 2\n");
        return SYSTEM_ERROR;
    }
                                //  Eliminate equivalents of peak in adjacent channels
    done = 0;
    for(n=0;n<dz->clength-1;n++) {
        peaklo = peakno1[n] * dz->chwidth;
        peakhi = peaklo + dz->halfchwidth;
        peaklo = peaklo - dz->halfchwidth;
        for(m=n+1;m<dz->clength;m++) {
            if(m >= peakcnt) {                                  //  once we have peakcnt valid peaks, quit
                done = 1;
                break;
            }
            if(peakfrq1[m] > peaklo && peakfrq1[m] < peakhi) {  //  equivalent peak (with lower amp, as amps sorted into descending order)
                for(k = m+1; k < dz->clength; k++) {
                    peakamp1[k-1] = peakamp1[k];                //  eliminate
                    peakfrq1[k-1] = peakfrq1[k];
                    peakno1[k-1] = peakno1[k];
                }
                m--;                                            //  stay at same place in loop
            }
        }
        if(done)
            break;
    }
    done = 0;
                            //  Same for 2nd file 
    for(n=0;n<dz->clength-1;n++) {
        peaklo = peakno2[n] * dz->chwidth;
        peakhi = peaklo + dz->halfchwidth;
        peaklo = peaklo - dz->halfchwidth;
        for(m=n+1;m<dz->clength;m++) {
            if(m >= peakcnt) {
                done = 1;
                break;
            }
            if(peakfrq2[m] > peaklo && peakfrq2[m] < peakhi) {
                for(k = m+1; k < dz->clength; k++) {
                    peakamp2[k-1] = peakamp2[k];
                    peakfrq2[k-1] = peakfrq2[k];
                    peakno2[k-1] = peakno2[k];
                }
                m--;
            }
        }
        if(done)
            break;
    }
    for(n=0;n<dz->clength;n++) {
        if(peakamp1[n] > 0.0)
            peaks_possible1++;
        if(peakamp2[n] > 0.0)
            peaks_possible2++;
    }
    peaks_possible = min(peaks_possible1,peaks_possible2);
    if(peaks_possible < peakcnt) {
        fprintf(stdout,"WARNING: Less peaks in spectrum (%d) than peakcnt specified (%d) : reducing peakcnt\n",peaks_possible,peakcnt);
        fflush(stdout);
        peakcnt = peaks_possible;
        dz->iparam[NMPH_APKS] = peakcnt;
    }

    // Finally : sort peaks into ascending order    

    for(n=0;n<peakcnt-1;n++) {
        for(m = n+1;m < peakcnt;m++) {
            if(peakno1[n] > peakno1[m]) {
                k = peakno1[n];
                peakno1[n] = peakno1[m];
                peakno1[m] = k;
            }
            if(peakno2[n] > peakno2[m]) {
                k = peakno2[n];
                peakno2[n] = peakno2[m];
                peakno2[m] = k;
            }
        }
    }
    return FINISHED;
}

/******************** NEWMORPH_CORE ****************************/

void newmorph_core(double interpratio,dataptr dz)
{
    int *peakno1  = dz->iparray[1], *peakno2  = dz->iparray[2], *newpeak = dz->iparray[3];
    double *peakfrq1 = dz->parray[3];
    int n, cc, vc, origvc, done, peakcnt = dz->iparam[NMPH_APKS];
    double amp, amp0 = 0.0, amp1 = 0.0, frq, frq0, frq1, hiamp, loamp, totamp_before, totamp_after, normaliser, rr, gapup, gapdn;
    memset((char *)dz->flbufptr[2],0,dz->wanted * sizeof(float));

    if((dz->process == SPECMORPH && dz->mode >= 4) || dz->process == SPECMORPH2) {
        for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {        //  Now do normal interpolation
            frq0 = dz->flbufptr[0][FREQ];
            frq1 = peakfrq1[cc];
            if(dz->param[NMPH_RAND] > 0.0) {
                if(cc < dz->clength - 1)
                    gapup = peakfrq1[cc+1] - peakfrq1[cc];
                else
                    gapup = min(dz->nyquist - peakfrq1[cc], peakfrq1[cc] - peakfrq1[cc-1]);
                if(cc > 0)
                    gapdn = peakfrq1[cc] - peakfrq1[cc-1];
                else
                    gapdn = min(peakfrq1[cc], peakfrq1[cc+1] - peakfrq1[cc]);
                rr = ((drand48() * 2.0) - 1.0) * 0.5;
                if(rr >= 0.0)
                    frq1 += (rr * dz->param[NMPH_RAND] * gapup);
                else
                    frq1 -= (rr * dz->param[NMPH_RAND] * gapdn);
            }
            frq = (frq1 - frq0) * interpratio;
            frq += frq0;
            dz->flbufptr[0][FREQ] = (float)frq;
        }
        return;
    }
    if(dz->mode > 1)
        find_the_spectral_peaks(&peakcnt,dz);
    
    for(n=0;n<peakcnt;n++) {                        //  Interpolate between peak values, storing in 3rd buffer
        vc = peakno1[n];
        amp0 = dz->flbufptr[0][AMPP];
        frq0 = dz->flbufptr[0][FREQ];
        vc = peakno2[n];
        amp1 = dz->flbufptr[1][AMPP];
        frq1 = dz->flbufptr[1][FREQ];
        amp = (amp1 - amp0) * interpratio;
        amp += amp0;
        frq = (frq1 - frq0) * interpratio;
        frq += frq0;
        cc = (int)round(frq/dz->chwidth);
        vc = cc * 2;
        if(dz->vflag[2])                            //  If only the frq is determined by sound 2, retain the sound 1 (peak) amp
            dz->flbufptr[2][AMPP] = dz->flbufptr[0][AMPP];
        else
            dz->flbufptr[2][AMPP] = (float)amp;
        dz->flbufptr[2][FREQ] = (float)frq;
        newpeak[n] = vc;
    }
    totamp_before = 0.0;
    for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {        //  Now do normal interpolation
        totamp_before += dz->flbufptr[0][AMPP];
        frq0 = dz->flbufptr[0][FREQ];
        frq1 = dz->flbufptr[1][FREQ];
        frq = (frq1 - frq0) * interpratio;
        frq += frq0;
        origvc = vc;
        done = 0;
        for(n=0;n < peakcnt; n++) {                     //  For channels with peak value, but not in same channel as peak in other file,
            if(vc == peakno1[n] && vc != peakno2[n] ) { //  give (originally a peak) an amp intermediate between adjacent channels (kludge!!)
                vc -= 2;                                //  IF peak moces out oc channel, then orig peak-channel will not be too loud (hopefully)   
                if(vc < 0)                              //  If peaks doesn't move between chans, this interpd-val will be overwritten below.
                    loamp = 0.0;
                else
                    loamp = dz->flbufptr[0][AMPP];
                vc += 4;
                if(vc >= dz->wanted)
                    hiamp = 0.0;
                else
                    hiamp = dz->flbufptr[0][AMPP];
                amp0 = (loamp + hiamp)/2.0;
                vc = origvc;
                amp1 = dz->flbufptr[1][AMPP];
                done = 1;
                break;
            } else if(vc == peakno2[n] && vc != peakno1[n] ) {  // ditto
                vc -= 2;
                if(vc < 0)
                    loamp = 0.0;
                else
                    loamp = dz->flbufptr[1][AMPP];
                vc += 4;
                if(vc >= dz->wanted)
                    hiamp = 0.0;
                else
                    hiamp = dz->flbufptr[1][AMPP];
                amp1 = (loamp + hiamp)/2.0;
                vc = origvc;
                amp0 = dz->flbufptr[0][AMPP];
                done = 1;
                break;
            }
        }
        vc = origvc;
        if(dz->vflag[1]) {                          // if -n flag set (no other interps), only interp the "done" (peak) chans
            if(done) {                              //  leaving all other channels with their file1 values
                amp = (amp1 - amp0) * interpratio;
                amp += amp0;
                dz->flbufptr[0][AMPP] = (float)amp;
                dz->flbufptr[0][FREQ] = (float)frq;
            }
        } else {
            if (!done) {                                //  In standard case (NOT -n): if no pre-interp already done 
                amp0 = dz->flbufptr[0][AMPP];           //  set up original chans for normal interp
                amp1 = dz->flbufptr[1][AMPP];
            }
            amp = (amp1 - amp0) * interpratio;          //  Now do the interp on pre-interpd (peak) chans AND on other chans
            amp += amp0;
            dz->flbufptr[0][AMPP] = (float)amp;
            dz->flbufptr[0][FREQ] = (float)frq;
        }
    }                                               //  THEN
    for(n=0;n < peakcnt; n++) {                     //  Overwrite normal interp with interpd-peaks  
        vc = newpeak[n];
        dz->flbufptr[0][AMPP] = dz->flbufptr[2][AMPP];
        dz->flbufptr[0][FREQ] = dz->flbufptr[2][FREQ]; 
    }
    if(dz->vflag[0]) {                              //  If -e, set window level to orig file1 window-level
        totamp_after  = 0.0;
        for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2)
            totamp_after += dz->flbufptr[0][AMPP];
        if(totamp_after > 0) {
            normaliser = totamp_before/totamp_after;
            for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2)
                dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * normaliser);
        }
    }
}

/**************************** ALLOCATE_NEW_TRIPLE_BUFFER ******************************/

int allocate_new_triple_buffer(dataptr dz)
{
    unsigned int buffersize;
    if(dz->bptrcnt < 4) {
        sprintf(errstr,"Insufficient bufptrs established in allocate_new_triple_buffer()\n");
        return(PROGRAM_ERROR);
    }
//TW REVISED: buffers don't need to be multiples of secsize
    buffersize = dz->wanted;
    dz->buflen = buffersize;
    if((dz->bigfbuf = (float*)malloc(dz->buflen*3 * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize = dz->buflen;
    dz->flbufptr[0]  = dz->bigfbuf;
    dz->flbufptr[1]  = dz->flbufptr[0] + dz->big_fsize;
    dz->flbufptr[2]  = dz->flbufptr[1] + dz->big_fsize;
    dz->flbufptr[3]  = dz->flbufptr[2] + dz->big_fsize;
    return(FINISHED);
}


/******************** FIND_THE_AVERAGE_PEAKS ****************************/

void find_the_spectral_peaks(int *peakcnt,dataptr dz)
{
    double *peakamp1 = dz->parray[1], *peakamp2   = dz->parray[2];
    double *peakfrq1 = dz->parray[3], *peakfrq2 = dz->parray[4];
    int *peakno1  = dz->iparray[1], *peakno2  = dz->iparray[2];
    int n, m, k;
    double tempamp, tempfrq, peaklo, peakhi;
    int cc, vc, done, peaks_possible, peaks_possible1 = 0, peaks_possible2 = 0;

    for(n=0;n < dz->clength;n++) {
        peakamp1[n] = 0.0;
        peakno1[n]  = n * 2;
        peakamp2[n] = 0.0;
        peakno2[n]  = n * 2;
    }
    for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {
        peakamp1[cc] = fabs(dz->flbufptr[0][AMPP]);
        peakfrq1[cc] = fabs(dz->flbufptr[0][FREQ]);                         //  Get total amplitude for each channel
    }
    for(n=0;n < dz->clength - 1;n++) {                                          //  Sort into loudness order
        for(m = n+1; m < dz->clength;m++) {
            if(peakamp1[n] < peakamp1[m]) {
                tempamp = peakamp1[n];
                tempfrq = peakfrq1[n];
                k = peakno1[n];
                peakamp1[n] = peakamp1[m];
                peakfrq1[n] = peakfrq1[m];
                peakno1[n]  = peakno1[m];
                peakamp1[m] = tempamp;
                peakfrq1[m] = tempfrq;
                peakno1[m] = k;
            }
        }
    }
    for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {
        peakamp2[cc] = fabs(dz->flbufptr[1][AMPP]);
        peakfrq2[cc] = fabs(dz->flbufptr[1][FREQ]);                         //  Get total amplitude for each channel
    }
    for(n=0;n < dz->clength - 1;n++) {                                          //  Sort into loudness order
        for(m = n+1; m < dz->clength;m++) {
            if(peakamp2[n] < peakamp2[m]) {
                tempamp = peakamp2[n];
                tempfrq = peakfrq2[n];
                k = peakno2[n];
                peakamp2[n] = peakamp2[m];
                peakfrq2[n] = peakfrq2[m];
                peakno2[n]  = peakno2[m];
                peakamp2[m] = tempamp;
                peakfrq2[m] = tempfrq;
                peakno2[m] = k;
            }
        }
    }

    done = 0;
    for(n=0;n<dz->clength-1;n++) {
        peaklo = peakno1[n] * dz->chwidth;
        peakhi = peaklo + dz->halfchwidth;
        peaklo = peaklo - dz->halfchwidth;
        for(m=n+1;m<dz->clength;m++) {
            if(m >= *peakcnt) {                                 //  once we have peakcnt valid peaks, quit
                done = 1;
                break;
            }
            if(peakfrq1[m] > peaklo && peakfrq1[m] < peakhi) {  //  equivalent peak (with lower amp, as amps sorted into descending order)
                for(k = m+1; k < dz->clength; k++) {
                    peakamp1[k-1] = peakamp1[k];                //  eliminate
                    peakfrq1[k-1] = peakfrq1[k];
                    peakno1[k-1] = peakno1[k];
                }
                m--;                                            //  stay at same place in loop
            }
        }
        if(done)
            break;
    }
    done = 0;
                            //  Same for 2nd file 
    for(n=0;n<dz->clength-1;n++) {
        peaklo = peakno2[n] * dz->chwidth;
        peakhi = peaklo + dz->halfchwidth;
        peaklo = peaklo - dz->halfchwidth;
        for(m=n+1;m<dz->clength;m++) {
            if(m >= *peakcnt) {
                done = 1;
                break;
            }
            if(peakfrq2[m] > peaklo && peakfrq2[m] < peakhi) {
                for(k = m+1; k < dz->clength; k++) {
                    peakamp2[k-1] = peakamp2[k];
                    peakfrq2[k-1] = peakfrq2[k];
                    peakno2[k-1] = peakno2[k];
                }
                m--;
            }
        }
        if(done)
            break;
    }
    for(n=0;n<dz->clength;n++) {
        if(peakamp1[n] > 0.0)
            peaks_possible1++;
        if(peakamp2[n] > 0.0)
            peaks_possible2++;
    }
    peaks_possible = min(peaks_possible1,peaks_possible2);
    if(peaks_possible < *peakcnt) {
        *peakcnt = peaks_possible;
    }

    // Finally : sort peaks into ascending order

    for(n=0;n<(*peakcnt)-1;n++) {
        for(m = n+1;m < *peakcnt;m++) {
            if(peakno1[n] > peakno1[m]) {
                k = peakno1[n];
                peakno1[n] = peakno1[m];
                peakno1[m] = k;
            }
            if(peakno2[n] > peakno2[m]) {
                k = peakno2[n];
                peakno2[n] = peakno2[m];
                peakno2[m] = k;
            }
        }
    }
}

/*************************************** DO_MORPHENV ******************************************/

void do_morphenv(dataptr dz)
{
    int cc, vc;
    double totamp_before = 0.0, totamp_after = 0.0, normaliser;
    for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {        //  Now do normal interpolation
        totamp_before += dz->flbufptr[0][AMPP];
        totamp_after  += dz->flbufptr[1][AMPP];
        if(totamp_after > 0) {
            normaliser = totamp_before/totamp_after;
            for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2)
                dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[1][AMPP] * normaliser);
        }
    }
    dz->total_windows++;
}   

/******************** FIND_THE_AVERAGE_SPECTRAL_PEAKS_AND_CORRELATES ****************************/

int find_the_average_spectral_peaks_and_correlates(dataptr dz)
{
    int exit_status, gotit;
    double *peakamp2   = dz->parray[2];
    double *peakfrq1 = dz->parray[3], *peakfrq2 = dz->parray[4];
    int *peakno2  = dz->iparray[2];
    int peakcnt = dz->iparam[NMPH_APKS];
    int n, m, k;
    double tempamp, tempfrq, peaklo, peakhi, frq;
    int windows_to_buf = 0, samps_left = 0, wcnt;
    int totwcnt = 0, cc, vc, done, peaks_possible = 0;

    if((dz->process != SPECMORPH2) || dz->mode == 0) {
    
        for(n=0;n < dz->clength;n++) {
            peakamp2[n] = 0.0;
            peakno2[n]  = n * 2;
        }
        totwcnt = 0;
        if(dz->process == SPECMORPH2)
            samps_left = dz->insams[0];
        else
            samps_left = dz->insams[1];
        while(samps_left) {
            if(dz->process == SPECMORPH2) {
                if((exit_status = read_specific_file_to_specific_buf(FILE0,&windows_to_buf,dz->bigfbuf,dz))<0)
                    return(exit_status);
            } else {
                if((exit_status = read_specific_file_to_specific_buf(FILE1,&windows_to_buf,dz->bigfbuf,dz))<0)
                    return(exit_status);
            }
            samps_left -= windows_to_buf * dz->wanted;
            for(wcnt =0;wcnt < windows_to_buf;wcnt++) {
                if(totwcnt == 0) {
                    totwcnt++;
                    continue;
                }
                for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {
                    peakamp2[cc] += fabs(dz->flbufptr[0][AMPP]);
                    if(totwcnt == 1)
                        peakfrq2[cc] = fabs(dz->flbufptr[0][FREQ]);
                    else
                        peakfrq2[cc] = (peakfrq2[cc] + fabs(dz->flbufptr[0][FREQ]))/2.0;
                }
                totwcnt++;
            }
        }
        for(n=0;n < dz->clength - 1;n++) {
            for(m = n+1; m < dz->clength;m++) {
                if(peakamp2[m] > peakamp2[n]) {
                    tempamp = peakamp2[n];
                    tempfrq = peakfrq2[n];
                    k = peakno2[n];
                    peakamp2[n] = peakamp2[m];
                    peakfrq2[n] = peakfrq2[m];
                    peakno2[n]  = peakno2[m];
                    peakamp2[m] = tempamp;
                    peakfrq2[m] = tempfrq;
                    peakno2[m] = k;
                }
            }
        }
        if(dz->process != SPECMORPH2) {
            if(sndseekEx(dz->ifd[1],0,0)<0) {
                sprintf(errstr,"Sound seek failed in file 2\n");
                return SYSTEM_ERROR;
            }
        }
                                    //  Eliminate equivalents of peak in adjacent channels
        done = 0;

        for(n=0;n<dz->clength-1;n++) {
            peaklo = peakno2[n] * dz->chwidth;
            peakhi = peaklo + dz->halfchwidth;
            peaklo = peaklo - dz->halfchwidth;
            for(m=n+1;m<dz->clength;m++) {
                if(m >= peakcnt) {
                    done = 1;
                    break;
                }
                if(peakfrq2[m] > peaklo && peakfrq2[m] < peakhi) {
                    for(k = m+1; k < dz->clength; k++) {
                        peakamp2[k-1] = peakamp2[k];
                        peakfrq2[k-1] = peakfrq2[k];
                        peakno2[k-1] = peakno2[k];
                    }
                    m--;
                }
            }
            if(done)
                break;
        }
        for(n=0;n<dz->clength;n++) {
            if(peakamp2[n] > 0.0)
                peaks_possible++;
        }
        if(peaks_possible < peakcnt) {
            fprintf(stdout,"WARNING: Less peaks in spectrum (%d) than peakcnt specified (%d) : reducing peakcnt\n",peaks_possible,peakcnt);
            fflush(stdout);
            peakcnt = peaks_possible;
            dz->iparam[NMPH_APKS] = peakcnt;
        }
        if(dz->process == SPECMORPH2 && dz->mode == 0)
            return FINISHED;
    }

    // Finally : assign freqs to all channels, to correspond (octavewise to peak data)

    if(dz->process == SPECMORPH2 && dz->mode > 0)
        peakcnt = min(peakcnt,dz->itemcnt);

    for(cc=0,vc = 0;cc < dz->clength;cc++,vc+=2) {
        peaklo = cc * dz->chwidth;
        peakhi = peaklo + dz->halfchwidth;
        peaklo = peaklo - dz->halfchwidth;
        gotit = 0;
        for(n=0;n<peakcnt;n++) {
            if(peakfrq2[n] >= peaklo && peakfrq2[n] <= peakhi) {
                peakfrq1[cc] = peakfrq2[n];
                gotit = 1;
            } else {
                frq = peakfrq2[n];
                gotit = 0;
                if(frq > peakhi) {
                    while(frq > peakhi)
                        frq /= 2;
                    if(frq > peaklo) {
                        peakfrq1[cc] = frq;
                        gotit = 1;
                    }
                } else if (frq < peaklo) {
                    while(frq < peaklo)
                        frq *= 2;
                    if(frq < peakhi) {
                        peakfrq1[cc] = frq;
                        gotit = 1;
                    }
                }
            }
            if(gotit)
                break;
        }
        if(!gotit)
            peakfrq1[cc] = -10000;
    }
    for(cc=0,vc = 0;cc < dz->clength;cc++,vc+=2) {
        if(peakfrq1[cc] == -10000) {
            if(cc - 1 >= 0 && peakfrq1[cc-1] != -10000)
                peakfrq1[cc] = peakfrq1[cc-1];
            else if(cc + 1 < dz->clength && peakfrq1[cc+1] != -10000)
                peakfrq1[cc] = peakfrq1[cc+1];
            else
                peakfrq1[cc] = cc * dz->chwidth;
        }
    }
    return FINISHED;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
    int exit_status;
    double dummy = 0.0;
    FILE *fp;
    int cnt = 0, linecnt;
    char temp[800], *p;

    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read times.\n",str);
        return(DATA_ERROR);
    }
    linecnt = 0;
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy)) {
            if(dummy < 0.0 || dummy > dz->nyquist) {
                sprintf(errstr,"Invalid peak data (%lf) at line %d.\n",dummy,linecnt+1);
                return(DATA_ERROR);
            }
            cnt++;
        }
    }
    if(cnt > dz->clength) {
        sprintf(errstr,"Too many peak frqs (%d) : max =  %d.\n",cnt,dz->clength);
        return(DATA_ERROR);
    }
    dz->itemcnt = cnt;
    dz->all_words = 0;
    if((exit_status = store_filename(str,dz))<0)
        return(exit_status);
    fclose(fp);
    return FINISHED;
}

/**************************** READ_THE_SPECIAL_DATA ****************************/

int read_the_special_data(dataptr dz)
{
    double dummy, *peakfrq2 = dz->parray[4];
    int cnt = 0;
    FILE *fp;
    char temp[800], *p;
        
    if((fp = fopen(dz->wordstor[0],"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read peak data.\n",dz->wordstor[0]);
        return(DATA_ERROR);
    }
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy))
            peakfrq2[cnt++] = dummy;
    }
    fclose(fp);
    return FINISHED;
}

