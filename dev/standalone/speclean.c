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
static int      handle_the_extra_infile(char ***cmdline,int *cmdlinecnt,dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
static int  setup_speclean_application(dataptr dz);
static int  setup_speclean_param_ranges_and_defaults(dataptr dz);
static int      check_speclean_param_validity_and_consistency(dataptr dz);
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int do_speclean(dataptr dz);
static int allocate_speclean_buffer(dataptr dz);

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
        // setup_particular_application =
        if((exit_status = setup_speclean_application(dz))<0) {
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
    if((exit_status = setup_speclean_param_ranges_and_defaults(dz))<0) {
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

    //      handle_extra_infiles() =
    if((exit_status = handle_the_extra_infile(&cmdline,&cmdlinecnt,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // handle_outfile() =
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

    //      handle_formants()                       redundant
    //      handle_formant_quiksearch()     redundant
    //      handle_special_data()           redundant

    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {              // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //check_param_validity_and_consistency .....
    if((exit_status = check_speclean_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;

    //allocate_large_buffers() ... replaced by
    dz->extra_bufcnt =  0;
    dz->bptrcnt = dz->iparam[0];

    if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = allocate_speclean_buffer(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess()                                            redundant
    //spec_process_file =
    if((exit_status = do_speclean(dz))<0) {
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

/************************ HANDLE_THE_EXTRA_INFILE *********************/

int handle_the_extra_infile(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
    /* OPEN ONE EXTRA ANALFILE, CHECK COMPATIBILITY */
    int  exit_status;
    char *filename;
    fileptr fp2;
    int fileno = 1;
    double maxamp, maxloc;
    int maxrep;
    int getmax = 0, getmaxinfo = 0;
    infileptr ifp;
    fileptr fp1 = dz->infile;
    filename = (*cmdline)[0];
    if((dz->ifd[fileno] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
        sprintf(errstr,"cannot open input file %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store data on later infile. (1)\n");
        return(MEMORY_ERROR);
    }
    if((fp2 = (fileptr)malloc(sizeof(struct fileprops)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store data on later infile. (2)\n");
        return(MEMORY_ERROR);
    }
    if((exit_status = readhead(ifp,dz->ifd[1],filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
        return(exit_status);
    copy_to_fileptr(ifp,fp2);
    if(fp2->filetype != ANALFILE) {
        sprintf(errstr,"%s is not an analysis file.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->origstype != fp1->origstype) {
        sprintf(errstr,"Incompatible original-sample-type in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->origrate != fp1->origrate) {
        sprintf(errstr,"Incompatible original-sample-rate in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->arate != fp1->arate) {
        sprintf(errstr,"Incompatible analysis-sample-rate in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->Mlen != fp1->Mlen) {
        sprintf(errstr,"Incompatible analysis-window-length in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->Dfac != fp1->Dfac) {
        sprintf(errstr,"Incompatible decimation factor in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->channels != fp1->channels) {
        sprintf(errstr,"Incompatible channel-count in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if((dz->insams[fileno] = sndsizeEx(dz->ifd[fileno]))<0) {           /* FIND SIZE OF FILE */
        sprintf(errstr, "Can't read size of input file %s.\n"
                "open_checktype_getsize_and_compareheader()\n",filename);
        return(PROGRAM_ERROR);
    }
    if(dz->insams[fileno]==0) {
        sprintf(errstr, "File %s contains no data.\n",filename);
        return(DATA_ERROR);
    }
    (*cmdline)++;
    (*cmdlinecnt)--;
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

/************************* SETUP_SPECLEAN_APPLICATION *******************/

int setup_speclean_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)         // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,0   ,2,2,"dd"      ))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
        return(FAILED);
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = TWO_ANALFILES;
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
    return(FINISHED);
}

/************************* SETUP_SPECLEAN_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_speclean_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!s
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[0]                       = dz->frametime;
    ap->hi[0]                       = min(1.0,dz->duration);
    ap->default_val[0]      = dz->frametime * 4.0;
    ap->lo[1]               = 1.0;
    ap->hi[1]               = CL_MAX_GAIN;
    ap->default_val[1]      = DEFAULT_NOISEGAIN;
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
            if((exit_status = setup_speclean_application(dz))<0)
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

/*********************** CHECK_SPECLEAN_PARAM_VALIDITY_AND_CONSISTENCY *********************/

int check_speclean_param_validity_and_consistency(dataptr dz)
{
    int n;

    dz->iparam[0] = (int)cdp_round(dz->param[0]/dz->frametime);
    dz->iparam[0] = max(dz->iparam[0],1);
    n = dz->insams[0]/dz->wanted;
    if(n < dz->iparam[0] + 1) {
        sprintf(errstr,"File is too short for the 'persist' parameter used.\n");
        return(GOAL_FAILED);
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

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if      (!strcmp(prog_identifier_from_cmdline,"clean"))                 dz->process = SPECLEAN;
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        return(USAGE_ONLY);
    }
    return(FINISHED);
}

/**************************** ALLOCATE_SPECLEAN_BUFFER ******************************/

int allocate_speclean_buffer(dataptr dz)
{
    unsigned int buffersize;
    buffersize = dz->wanted * (dz->bptrcnt + 2);
    dz->buflen = dz->wanted;
    if((dz->bigfbuf = (float*) malloc(buffersize * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
    fprintf(stderr,
            "\nFURTHER OPERATIONS ON ANALYSIS FILES\n\n"
            "USAGE: speclean NAME infile(s) outfile parameters: \n"
            "\n"
            "where NAME can be any one of\n"
            "\n"
            "clean\n\n"
            "Type 'speclean clean' for more info on speclean clean..ETC.\n");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"clean")) {
        fprintf(stderr,
                "USAGE:\n"
                "speclean clean sigfile noisfile outanalfile persist noisgain\n"
                "\n"
                "Eliminate frqbands in 'sigfile' falling below maxlevels found in 'noisfile'\n"
                "\n"
                "PERSIST    min time channel signal must exceed noise-level, to be retained.\n"
                "NOISGAIN   multiplies noiselevels found in noisfile before they are used\n"
                "           for comparison with infile signal: (Default 2).\n"
                "\n"
                "Both input files are analysis files.\n"
                "'noisfil' is a sample of noise (only) from the 'sigfile'.\n"
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

/*********************** INNER_LOOP ***************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
    return FINISHED;
}

/**************************** DO_SPECLEAN ****************************/

int do_speclean(dataptr dz)
{
    int exit_status, cc, vc;
    int n, samps_read, total_samps = 0;
    float *fbuf, *obuf, *ibuf, *nbuf;
    int *persist;
    fbuf = dz->bigfbuf;
    obuf = dz->bigfbuf;
    nbuf = dz->bigfbuf + (dz->buflen * dz->iparam[0]);
    ibuf = nbuf - dz->buflen;
    if((persist = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
        sprintf(errstr,"No memory for persistence counters.\n");
        return(MEMORY_ERROR);
    }
    memset((char *)persist,0,dz->clength * sizeof(int));
    memset((char *)nbuf,0,dz->buflen * sizeof(float));
    /* ESTABLISH NOISE CHANNEL-MAXIMA */
    while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[1],0)) > 0) {
        if(samps_read < 0) {
            sprintf(errstr,"Failed to read data from noise file.\n");
            return(SYSTEM_ERROR);
        }
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
            if(fbuf[AMPP] > nbuf[AMPP])
                nbuf[AMPP] = fbuf[AMPP];
        }
        total_samps += samps_read;
    }
    if(total_samps <= 0) {
        sprintf(errstr,"No data found in noise file.\n");
        return(SYSTEM_ERROR);
    }
    /* MUTIPLY BY 'NOISEGAIN' FACTOR */
    for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2)
        nbuf[AMPP] = (float)(nbuf[AMPP] * dz->param[1]);
    /* SKIP FIRST WINDOWS */
    dz->samps_left = dz->insams[0];
    if((samps_read = fgetfbufEx(obuf, dz->buflen,dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Failed to read data from signal file.\n");
        return(SYSTEM_ERROR);
    }
    if(samps_read < dz->buflen) {
        sprintf(errstr,"Problem reading data from signal file.\n");
        return(PROGRAM_ERROR);
    }
    if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
        return(exit_status);
    dz->samps_left -= dz->buflen;
    /* READ IN FIRST GROUP OF SIGNAL WINDOWS */
    dz->samps_left = dz->insams[0] - dz->wanted;
    if((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen * dz->iparam[0],dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Failed to read data from signal file.\n");
        return(SYSTEM_ERROR);
    }
    if(samps_read < dz->buflen * dz->iparam[0]) {
        sprintf(errstr,"Problem reading data from signal file.\n");
        return(PROGRAM_ERROR);
    }
    dz->samps_left -= dz->buflen * dz->iparam[0];
    for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
        /* FOR EACH CHANNEL */
        obuf = dz->bigfbuf;
        fbuf = ibuf;
        /* MOVING BACKWARDS THROUGH FIRST GROUP OF WINDOWS */
        while(fbuf >= obuf) {
            /* COUNT HOW MANY CONTIGUOUS WINDOWS EXCEED THE NOISE THRESHOLD */
            if(fbuf[AMPP] > nbuf[AMPP])
                persist[cc]++;
            else
                /* BUT IF ANY WINDOW IS BELOW NOISE THRESHOLD, BREAK */
                break;
            fbuf -= dz->buflen;
        }
        /* IF NOT ALL WINDOWS ARE ABOVE THE NOISE THRESHOLD, ZERO OUTPUT OF CHANNEL */
        if(persist[cc] < dz->iparam[0])
            obuf[AMPP] = 0.0f;
    }
    /* WRITE FIRST OUT WINDOW */
    if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
        return(exit_status);
    obuf = dz->bigfbuf;
    fbuf = obuf + dz->buflen;
    /* MOVE ALL WINDOWS BACK ONE */
    for(n = 1; n < dz->iparam[0]; n++) {
        memcpy((char *)obuf,(char *)fbuf,dz->wanted * sizeof(float));
        fbuf += dz->wanted;
        obuf += dz->wanted;
    }
    obuf = dz->bigfbuf;
    fbuf = obuf + dz->buflen;
    while(dz->samps_left > 0) {
        /* READ A NEW WINDOW TO END OF EXISTING GROUP */
        if((samps_read = fgetfbufEx(ibuf,dz->buflen,dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Problem reading data from signal file.\n");
            return(PROGRAM_ERROR);
        }
        dz->samps_left -= dz->wanted;
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
            /* IF LEVEL IN ANY CHANNEL IN NEW WINDOW FALLS BELOW NOISE THRESHOLD */
            if(ibuf[AMPP] < nbuf[AMPP]) {
                /* IF ITS A SIGNAL-PERSISTING CHANNEL, KEEP IT, BUT DECREMENT ITS PERSISTENCE VALUE */
                if(persist[cc] > 0)
                    persist[cc]--;
                else
                    /* IF NO PERSISTENCE LEFT, SET OUTPUT TO ZERO */
                    obuf[AMPP] = 0.0f;
                continue;
            } else {
                /* IF LEVEL IN ANY CHANNEL IN NEW WINDOW IS ABOVE NOISE THRESHOLD COUNT ITS PERSISTANCE */
                persist[cc]++;
                if(persist[cc] >= dz->iparam[0])
                    persist[cc] = dz->iparam[0];
                else
                    /* IF NOT PERSISTED LONG ENOUGH YET, SET OUTPUT TO ZERO */
                    obuf[AMPP] = 0.0f;
            }
        }
        /* WRITE THE OUTPUT */
        if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
            return(exit_status);
        /* MOVE ALL WINDOWS BACK ONE */
        if(dz->samps_left) {
            for(n = 1; n < dz->iparam[0]; n++) {
                memcpy((char *)obuf,(char *)fbuf,dz->wanted * sizeof(float));
                fbuf += dz->wanted;
                obuf += dz->wanted;
            }
        }
        obuf = dz->bigfbuf;
        fbuf = obuf + dz->buflen;
    }
    /* DEAL WITH REMAINDER OF FINAL BLOCK OF WINDOWS */
    obuf += dz->buflen;
    while(obuf < nbuf) {
        /* ADVANCING THROUGH THE WINDOWS */
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
            fbuf = obuf;
            /* IF LEVEL IN ANY CHANNEL FALLS BELOW NOISE THRESHOLD OR CHANNEL WAS NOT PREVIOUSLY PERSISTING */
            if((fbuf[AMPP] < nbuf[AMPP]) || !persist[cc]) {
                /* ZERO IT */
                fbuf[AMPP] = 0.0f;
                persist[cc] = 0;
                /* AND ZERO SAME CHANNEL IN ALL SUBSEQUENT WINDOWS */
                fbuf += dz->buflen;
                while(fbuf < nbuf) {
                    fbuf[AMPP] = 0.0f;
                    fbuf += dz->buflen;
                }
            }
            /* WRITE OUTPUT */
            if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
                return(exit_status);
        }
        obuf += dz->buflen;
    }
    return FINISHED;
}
