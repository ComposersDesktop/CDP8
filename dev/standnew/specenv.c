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

#ifdef unix
#define round(x) lround((x))
#else
#define round(x) cdp_round((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

#define SE_IBUF1    0
#define SE_IBUF2    1
#define SE_OBUF     2
#define SE_BUFEND   3

#define CHAN_SRCHRANGE_F    (4)

#define spececentrfrq specenvpch

#define MINSPECAMP  

const char* cdp_version = "7.1.0";

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
static int  handle_the_extra_infile(char ***cmdline,int *cmdlinecnt,dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
static int  setup_specenv_application(dataptr dz);
static int  setup_specenv_param_ranges_and_defaults(dataptr dz);
static int  check_specenv_param_validity_and_consistency(dataptr dz);
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int do_specenv(dataptr dz);
static int allocate_specenv_buffer(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
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
        // setup_particular_application =
        if((exit_status = setup_specenv_application(dz))<0) {
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
    if((exit_status = setup_specenv_param_ranges_and_defaults(dz))<0) {
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

//  handle_extra_infiles() =
    if((exit_status = handle_the_extra_infile(&cmdline,&cmdlinecnt,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
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
    //check_param_validity_and_consistency .....
    if((exit_status = check_specenv_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;

    //allocate_large_buffers() ... replaced by
    dz->extra_bufcnt =  0; 
    dz->bptrcnt = 3;
    
    if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = allocate_specenv_buffer(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess()                        redundant
    //spec_process_file =
    if((exit_status = do_specenv(dz))<0) {
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
    if((dz->insams[fileno] = sndsizeEx(dz->ifd[fileno]))<0) {       /* FIND SIZE OF FILE */
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

/************************* SETUP_SPECENV_APPLICATION *******************/

int setup_specenv_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,0   ,1,1,"i"      ))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"b",1,"d","pik",3,0,"000"))<0)
        return(FAILED);
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = TWO_ANALFILES;
    dz->process_type    = MIN_ANALFILE; 
    dz->outfiletype     = ANALFILE_OUT;
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

/************************* SETUP_SPECENV_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_specenv_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!s
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[0]           = 1;
    ap->hi[0]           = dz->clength;
    ap->default_val[0]  = 60;
    ap->lo[1]           = -1;
    ap->hi[1]           = 1;
    ap->default_val[1]  = 0;
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
            if((exit_status = setup_specenv_application(dz))<0)
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

/*********************** CHECK_SPECENV_PARAM_VALIDITY_AND_CONSISTENCY *********************/

int check_specenv_param_validity_and_consistency(dataptr dz)
{
    int n, m;
    double nextfrq, frqstep, jj, jj1, jj2;
    if(dz->insams[0] > dz->insams[1]) {
        sprintf(errstr,"First file is longer than 2nd: cannot proceed.\n");
        return(GOAL_FAILED);
    }
    if(dz->vflag[0]) {      //  IF PITCHWISE
        if(dz->iparam[0] > 4) {
            sprintf(errstr,"For octave-wise spectral envelope, max octaves-per-window is 4.\n");
            return(DATA_ERROR);
        }
    }
    if(dz->param[1] == 1.0) {
        sprintf(errstr,"Input file1 is not affected, if balance is set at 1.\n");
        return(DATA_ERROR);
    }
    if(dz->param[1] == -1.0) {
        sprintf(errstr,"Input file1 is merely replace by file2, if balance is set at -1.\n");
        return(DATA_ERROR);
    }
    if((dz->specenvfrq = (float *)malloc(dz->wanted * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for spectral envelope lower frequency boundaries.\n");
        return(MEMORY_ERROR);
    }
    if((dz->specenvtop = (float *)malloc(dz->wanted * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for spectral envelope upper frequency boundaries.\n");
        return(MEMORY_ERROR);
    }
    if((dz->specenvamp = (float *)malloc(dz->wanted * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for 1st spectral envelope amplitudes.\n");
        return(MEMORY_ERROR);
    }
    if((dz->specenvamp2 = (float *)malloc(dz->wanted * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for 2nd spectral envelope amplitudes.\n");
        return(MEMORY_ERROR);
    }
    if((dz->spececentrfrq = (float *)malloc((dz->wanted + 1) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for 2nd spectral envelope amplitudes.\n");
        return(MEMORY_ERROR);
    }
    n = 0;
    dz->specenvfrq[n] = 0.0;
    nextfrq = dz->halfchwidth;
    dz->specenvtop[n] = (float)nextfrq;
    n++;
    if(dz->vflag[0])                            //  octave-wise envelopes
        frqstep = pow(2.0,dz->iparam[0]);       //  N-octave frq multiplier
    else                                        //  frequency-wise envelopes
        frqstep = dz->chwidth * dz->iparam[0];  //  equal frequency steps
    while(nextfrq < dz->nyquist) {
        if(n >= dz->clength) {
            sprintf(errstr,"INSUFFICIENT MEMORY for spectral envelope data.\n");
            return(PROGRAM_ERROR);
        }
        dz->specenvfrq[n] = (float)nextfrq;
        if(dz->vflag[0])
            nextfrq *= frqstep;
        else
            nextfrq += frqstep;
        dz->specenvtop[n] = (float)nextfrq;
        n++;
    }
    dz->specenvcnt = n;
    dz->spececentrfrq[0] = 0.0; 
    if(dz->vflag[0]) {
        for(n = 0, m= 1; n < dz->specenvcnt; n++,m++) {
            hztomidi(&jj1,dz->specenvtop[n]);
            hztomidi(&jj2,dz->specenvfrq[n]);
            jj = (jj1 + jj2)/2.0;
            dz->spececentrfrq[m] = (float)miditohz(jj);         //  NB THESE ARE STORED AS FREQUENCIES
        }
    } else {
        for(n = 0, m= 1; n < dz->specenvcnt; n++,m++)
            dz->spececentrfrq[m] = (float)((dz->specenvtop[n] + dz->specenvfrq[n])/2.0);
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
    if      (!strcmp(prog_identifier_from_cmdline,"specenv"))           dz->process = SPECENV;
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        return(USAGE_ONLY);
    }
    return(FINISHED);
}

/**************************** allocate_specenv_buffer ******************************/

int allocate_specenv_buffer(dataptr dz)
{
    unsigned int buffersize;
    buffersize = dz->wanted * (dz->bptrcnt + 1);
    dz->buflen = dz->wanted;
    if((dz->bigfbuf = (float*) malloc(buffersize * sizeof(float)))==NULL) {  
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->flbufptr[SE_IBUF1]  = dz->bigfbuf;                          //  inbuf1
    dz->flbufptr[SE_IBUF2]  = dz->flbufptr[SE_IBUF1] + dz->wanted;  //  inbuf2
    dz->flbufptr[SE_OBUF]   = dz->flbufptr[SE_IBUF2] + dz->wanted;  //  obuf
    dz->flbufptr[SE_BUFEND] = dz->flbufptr[SE_OBUF]  + dz->wanted;  //  end of bufs
    return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
    return(usage2("specenv"));
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"specenv")) {
        fprintf(stderr,
        "USAGE:\n"
        "specenv specenv inanalfil1 inanalfil2 outanalfil windowsize [-bbal] [-p] [-i] [-k]\n"
        "\n"
        "Get the spectral envelope of file 2 and apply it to file 1.\n"
        "(File 2 must be at least as long as file 1).\n"
        "\n"
        "BAL  proportion of original files in output.\n"
        "     bal > 1 : balance of shaped-file to infile1 = (1-bal) : bal\n"
        "               (Higher balance keeps more of the original file1)\n" 
        "     bal < 1 : balance of shaped-file to infile2 = (1+bal) : -bal\n"
        "               (Lower balance keeps more of the imposed file2)\n" 
        "     default 0.0 (none of the original files in the output).\n"
        "\n"
        "-p : windowsize counted in octave steps.\n"
        "     Default: counted in analysis-channel-widths (srate/anal-chans).\n"
        "\n"
        "-i : IMPOSE spectral envelope (default, REPLACE spectral envelope)\n"  
        "\n"
        "-k : KEEP the loudness contour of the imposed file.\n" 
        "     Default: keep the loudness contour of the original file.\n"
        "\n"
        "Process equivalent to format extraction + formant impose/replace\n"
        "but allows for larger spectral windows.\n"
        "\n");
    } else {
        fprintf(stderr,
        "Unknown option '%s'\n"
        "USAGE:\n"
        "specenv specenv inanalfil1 inanalfil2 outanalfil windowsize [-f] [-i]\n",str);
    }   
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

/**************************** DO_SPECENV ****************************/

int do_specenv(dataptr dz)
{
    int exit_status, cc, vc, cnt, botchan, topchan, bwidth_in_chans;
    int n, samps_read1, samps_read2, wcnt;
    float *ibuf1, *ibuf2, *obuf;
    float topfreq, botfreq, frq, specfrq, up_specfrq, dn_specfrq, loamp, hiamp, amp1 = 0.0, amp2;
    double diff, ratio, pch, up_specpch, dn_specpch, ampdiff, ampstep, maxiamp = 0.0, maxiamp2 = 0.0, maxoamp = 0.0;
    double balance = dz->param[1];

    ibuf1 = dz->flbufptr[SE_IBUF1];
    ibuf2 = dz->flbufptr[SE_IBUF2];
    obuf  = dz->flbufptr[SE_OBUF];

    //  READ A WINDOW FROM EACH FILE
    wcnt = 0;
    while((samps_read1 = fgetfbufEx(ibuf1, dz->buflen,dz->ifd[0],0)) > 0) {
        if(samps_read1 < 0) {
            sprintf(errstr,"Failed to read data from first input file.\n");
            return(SYSTEM_ERROR);
        }
        samps_read2 = fgetfbufEx(ibuf2, dz->buflen,dz->ifd[1],0);
        if(samps_read2 < 0) {
            sprintf(errstr,"Failed to read data from second input file.\n");
            return(SYSTEM_ERROR);
        }
        topfreq = 0.0f;
        n = 0;
        while(n < dz->specenvcnt) {     //  Extract spectral envelope of both files
            botfreq  = topfreq;
            botchan  = (int)floor(botfreq/dz->chwidth); /* TRUNCATE */
            botchan -= CHAN_SRCHRANGE_F;
            botchan  =  max(botchan,0);
            topfreq  = dz->specenvtop[n];
            topchan  = (int)floor(topfreq/dz->chwidth); /* TRUNCATE */
            topchan += CHAN_SRCHRANGE_F;
            topchan  =  min(topchan,dz->clength);
            for(cc = botchan,vc = botchan * 2; cc < topchan; cc++,vc += 2) {
                if(fabs(ibuf1[FREQ]) >= botfreq && fabs(ibuf1[FREQ]) < topfreq)
                    dz->specenvamp[n] = (float)(dz->specenvamp[n] + ibuf1[AMPP]);
                if(fabs(ibuf2[FREQ]) >= botfreq && fabs(ibuf2[FREQ]) < topfreq)
                    dz->specenvamp2[n] = (float)(dz->specenvamp2[n] + ibuf2[AMPP]);
            }
            bwidth_in_chans    = max(1,(int)round((topfreq - botfreq)/dz->chwidth));
            dz->specenvamp[n]  = (float)(dz->specenvamp[n]/bwidth_in_chans);
            dz->specenvamp2[n] = (float)(dz->specenvamp2[n]/bwidth_in_chans);
            n++;
        }
        if(!dz->vflag[2]) {
            maxiamp  = 0.0;
            maxiamp2 = 0.0;
            maxoamp  = 0.0;
        }
        for(cc = 0,vc = 0; cc < dz->clength; cc++,vc += 2) {
            //  FOR EACH FREQUNCY IN THE SPECTRUM ....
            
            frq = (float)fabs(ibuf1[FREQ]);

            //  FIND THE SPECTRAL ENVELOPE LEVELS (amp1 & amp2) corresponding TO THIS FRQ
            
            cnt = 1;                                            //  speccentrecnt counts 0 to n | |   |   |     centre frqs of bands + val at 0Hz
                                                                //  specenv boundaries 0 to n-1 |___|___|___    bottom frqs of bands
            specfrq =   dz->spececentrfrq[cnt];                 //  specamp vals 0 to n-1         |   |   |     average amplitudes of bands
            while(frq > specfrq) {
                cnt++;                                          //  Search envelope bands until envband-frq is > input frq
                specfrq = dz->spececentrfrq[cnt];
            }
            if(dz->vflag[0]) {  // PITCHWISE                    
                hztomidi(&pch,frq);
                hztomidi(&up_specpch,specfrq);
                cnt--;
                hztomidi(&dn_specpch,dz->spececentrfrq[cnt]);
                diff = up_specpch - dn_specpch; 
                ratio = (pch - dn_specpch)/diff;
            } else {            //  FRQWISE                     //  thisfrq = *                 0   1   2*  3   4   
                up_specfrq = specfrq;                           //  specenv boundaries          |___|___|*__|___|___    bottom frqs of bands
                cnt--;                                          //  speccentrefrq-hi            x x   x  *|   x   x     cnt = 3
                dn_specfrq = dz->spececentrfrq[cnt];            //  speccentrefrq-lo            x x   |  *x   x   x     cnt = 2
                diff = up_specfrq - dn_specfrq;                 //  cnt of centres              0 1  ~2~ ~3~  4   5     
                ratio = (frq - dn_specfrq)/diff;                //                                      /
                                                                //  count of HIampval (below)     0   1  ~2~  3   4
                                                                //  HIamp                         a   a   A   a   a     cnt = 2 (for hiamp)
            }

            //  INTERPOLATE FOR SPECTRAL ENVELOPE AMPLITUDE AT THIS PARTICULAR FREQUENCY, IN BOTH ENVELOPES (if ness)
        
            if(!dz->vflag[1]) {     //  i.e. IMPOSE rather than REPLACE
                hiamp = dz->specenvamp[cnt];                    //  count of ampvals              0   1  ~2~  3   4
                loamp = dz->specenvamp[cnt - 1];                //  hiamp                         a   a   A   a   a
                ampdiff = hiamp - loamp;                        //  loamp                         a   A   a   a   a
                ampstep = ampdiff * ratio;                      //  count of ampvals              0  ~1~  2   3   4
                amp1 = (float)(loamp + ampstep);
            }
            hiamp = dz->specenvamp2[cnt];
            loamp = dz->specenvamp2[cnt - 1];
            ampdiff = hiamp - loamp;
            ampstep = ampdiff * ratio;
            amp2 = (float)(loamp + ampstep);

            if(dz->vflag[1])    //  IMPOSE
                obuf[AMPP] = (float)(ibuf1[AMPP] * amp2);
            else {              //  REPLACE
                if(amp1 > MINAMP)
                    obuf[AMPP] = (float)(ibuf1[AMPP] * amp2/amp1);
                else
                    obuf[AMPP] = ibuf1[AMPP];
            }
            obuf[FREQ] = ibuf1[FREQ];
            if(!dz->vflag[2]) {
                maxiamp  = max(maxiamp,ibuf1[AMPP]);
                maxiamp2 = max(maxiamp2,ibuf2[AMPP]);
                maxoamp  = max(maxoamp,obuf[AMPP]);
            }
        }   
        if(!dz->vflag[2]) {
            if((maxiamp2 < MINAMP) && maxiamp > MINAMP) {               //  If the superimposed spectrum level is (almost) zero, and the orig spectrum isn't
                for(cc = 0,vc = 0; cc <dz->clength; cc++, vc +=2)       //  Keep the orig spectrum
                    obuf[AMPP] = ibuf1[AMPP];
            } else if(maxoamp > MINAMP) {                               //  Else (if the maxoutput spectrum is not almost zero)
                for(cc = 0,vc = 0; cc <dz->clength; cc++, vc +=2)       //  adjust output level to level of input
                    obuf[AMPP] = (float)(obuf[AMPP] * maxiamp/maxoamp);
            } else if(maxiamp > MINAMP) {                               //  else, if input is not (nearly) zero, replace output by input
                for(cc = 0,vc = 0; cc <dz->clength; cc++, vc +=2)
                    obuf[AMPP] = ibuf1[AMPP];
            }
        }
        if(balance > 0.0) {
            for(cc = 0,vc = 0; cc <dz->clength; cc++, vc +=2)
                obuf[AMPP] = (float)((obuf[AMPP] * (1.0 - balance)) + (ibuf1[AMPP] * balance));
        } else if(balance < 0.0) {
            for(cc = 0,vc = 0; cc <dz->clength; cc++, vc +=2)
                obuf[AMPP] = (float)((obuf[AMPP] * (1.0 + balance)) - (ibuf2[AMPP] * balance));
        }
        if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
            return(exit_status);
        wcnt++;
    }
    return FINISHED;
}
