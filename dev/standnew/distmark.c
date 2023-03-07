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

#define minstep scalefact
#define stepstorecnt rampbrksize
#define maxstep is_sharp

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

#define DMARK_CNT 0
#define DMARK_STR 1
#define DMARK_RND 2
#define DMARK_GAIN 3

#define DMARK_FLIP 0
#define DMARK_TAIL 1

#define SAFETY 16

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int check_distmark_param_validity_and_consistency(dataptr dz);
static int setup_distmark_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_distmark_param_ranges_and_defaults(dataptr dz);
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
//static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int create_distmark_sndbufs(dataptr dz);
static int create_distmark_sndbufs2(dataptr dz);
static int preprocess_distmark(dataptr dz);
static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int read_mark_data(char *filename,dataptr dz);
static int get_timeinterpcnt(int lastgplen,int gplen,int markstep,double time,dataptr dz);
static int distmark(dataptr dz);
static int distmark2(dataptr dz);
static int  get_the_mode_no(char *str, dataptr dz);

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
        dz->maxmode = 2;
        if(cmdlinecnt <= 0) {
            sprintf(errstr,"Too few commandline parameters.\n");
            return(FAILED);
        }
        if((get_the_mode_no(cmdline[0],dz))<0)
            return(FAILED);
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_distmark_application(dz))<0) {
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
    if((exit_status = setup_distmark_param_ranges_and_defaults(dz))<0) {
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

//  handle_extra_infiles() : redundant
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
//  handle_special_data ............
    if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency .......
    if((exit_status = check_distmark_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    dz->bufcnt = 7;
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

    if((exit_status = create_distmark_sndbufs(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess ......
    if((exit_status = preprocess_distmark(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = create_distmark_sndbufs2(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    switch(dz->mode) {
    case(0):
        if((exit_status = distmark(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    case(1):
        if((exit_status = distmark2(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    }
    if((exit_status = complete_output(dz))<0) {                                 // CDP LIB
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
    int exit_status;
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

/************************* SETUP_DISTMARK_APPLICATION *******************/

int setup_distmark_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,MARKLIST   ,1,1,"D"))<0)
        return(FAILED);
    switch(dz->mode) {
    case(0):    exit_status = set_vflgs(ap,"sr",2,"DD","ft",2,0,"00");  break;
    case(1):    exit_status = set_vflgs(ap,"srg",3,"DDD","fa" ,2,0,"00");   break;
    }
    // set_legal_infile_structure -->
    if(exit_status <0)
        return(FAILED);
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = SNDFILES_ONLY;
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

/************************* SETUP_DISTMARK_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_distmark_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[DMARK_CNT]   = .5;
    ap->hi[DMARK_CNT]   = 1000;
    ap->default_val[DMARK_CNT]  = 5;
    ap->lo[DMARK_STR]   = 1;
    ap->hi[DMARK_STR]   = 256;
    ap->default_val[DMARK_STR]  = 1;
    ap->lo[DMARK_RND]   = 0;
    ap->hi[DMARK_RND]   = 1;
    ap->default_val[DMARK_RND]  = 0;
    if(dz->mode == 1) {
        ap->lo[DMARK_GAIN]  = 0.0;
        ap->hi[DMARK_GAIN]  = 1.0;
        ap->default_val[DMARK_GAIN] = 1.0;
    }
    dz->maxmode = 2;
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
            if((exit_status = setup_distmark_application(dz))<0)
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
    usage2("distmark");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"distmark"))                dz->process = DISTMARK;
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
    if(!strcmp(str,"distmark")) {
        fprintf(stderr,
        "USAGE:\n"
        "distmark distmark 1 infile outfile marklist unitlen [-ststretch] [-rrand] [-f] [-t]\n"
        "distmark distmark 2 infile outfile marklist unitlen [-ststretch] [-rrand] [-f]\n"
        "\n"
        "Interpolate between waveset-groups at marked points in MONO soundfile.\n"
        "In Mode 2 interpolate within ALTERNATE marked blocks.\n"
        "\n"
        "MARKLIST List of times within source at which to find waveset-groups.\n"
        "UNITLEN  (approx) size of waveset group to find (mS). Can vary over time.\n"
        "         Min UNITLEN < 1/2 of min step between Times in MARKLIST.\n"  
        "TSTRETCH Timestretch distances between marks, in making output.\n"
        "RAND     Randomise duration of interpolated wavesets (Range 0 - 1).\n" 
        "         Randomisation decreases waveset lengths (heard \"pitch\" higher).\n"
        "-f   Flip phase of alternate wavesets.\n"
        "-t   Add original (remaining) tail of source sound to output.\n"  
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

/*************************** PREPROCESS_DISTMARK **************************/

int preprocess_distmark(dataptr dz) 
{
    int exit_status, seek_forward;
    float *ibuf = dz->sampbuf[0];
    double *times = dz->parray[0], mindur, time;
    int *wavesetbloks, n, m, sampcnt, seccnt, nusampcnt, offset, bufpos, stt, wgstt, wavsetgplen;
    double srate = (double)dz->infile->srate;
    if((dz->lparray = (int **)malloc(2 * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for Waveset samplecnt stores.\n");
        return(MEMORY_ERROR);
    }

    //  Create array big enough to store sizes of each intermediate time-interpd waveset-gp as we step from one mark to next

    if(dz->brksize[DMARK_CNT]) {                        //  Get smallest waveset-gp size
        if((exit_status = get_minvalue_in_brktable(&mindur,DMARK_CNT,dz))<0)
            return exit_status;
    } else
        mindur = dz->param[DMARK_CNT];
    mindur *= MS_TO_SECS;
    dz->stepstorecnt = (int)ceil(dz->maxstep/mindur);
    dz->stepstorecnt *= 8;  //  SAFETY
    if((dz->lparray[1] = (int *)malloc(dz->stepstorecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for Waveset startsample store (2).\n");
        return(MEMORY_ERROR);
    }

    //  Set up store for true startsample and samplelen of waveset-gp at each time-mark

    if((dz->lparray[0] = (int *)malloc(dz->itemcnt * 2 * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for Waveset startsample store (2).\n");
        return(MEMORY_ERROR);
    }
    wavesetbloks = dz->lparray[0];

    //  Find exact locations and sizes of wavset-gps to be used

    for(n=0,m=0;n < dz->itemcnt;n++,m+=2) {
        time = times[n];                        //  Go to next timemark in sndfile
        sampcnt = (int)round(time * srate);
        seccnt = sampcnt/F_SECSIZE;
        nusampcnt = seccnt * F_SECSIZE;         //  Curtal to nearest sector boundary (may be unness)
        offset = sampcnt - nusampcnt;
        if((sndseekEx(dz->ifd[0],nusampcnt,0))<0) {
            sprintf(errstr,"sndseek 1 failed.\n");
            return GOAL_FAILED;
        }
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        bufpos = offset;
        while(ibuf[bufpos] < 0.0) {             //  Search for 1st downward-going waveset start, after "time"
            if(++bufpos > dz->ssampsread) {
                sprintf(errstr,"No waveset-group start found (1) at time %lf (Buffer too short?)\n",time);
                return(GOAL_FAILED);
            }
        }
        while(ibuf[bufpos] >= 0.0) {
            if(++bufpos > dz->ssampsread) {
                sprintf(errstr,"No waveset-group start found (2) at time %lf (Buffer too short?)\n",time);
                return(GOAL_FAILED);
            }
        }
        stt = bufpos;
        wgstt = nusampcnt + bufpos;             //  Note absolute sample-cnt at start of waveset-gp

        if(dz->brksize[DMARK_CNT]) {                    //  Get size of required waveset group at this "time"
            if((exit_status = read_value_from_brktable(time,DMARK_CNT,dz))<0)
                return(exit_status);            //  (either fixed param dz->param[0] mS or read brktable val)
        }
        wavsetgplen = (int)round(dz->param[DMARK_CNT] * MS_TO_SECS * srate);
        bufpos += wavsetgplen;
        if(bufpos >= dz->ssampsread) {
            sprintf(errstr,"Anomaly in buffer sizing looking for end of waveset-group at time %lf (1)\n",time);
            return(PROGRAM_ERROR);
        }
        seek_forward = 1;
        while(seek_forward) {
            while(ibuf[bufpos] < 0.0) {             //  Skip over samples below zero
                if(++bufpos > dz->ssampsread) {
                    seek_forward = 0;               //  IF run off buf end, try backward seek instead
                    break;
                }
            }
            if(!seek_forward)
                break;
            while(ibuf[bufpos] >= 0.0) {            //  Search for 1st downward-going waveset end, after "time"
                if(++bufpos > dz->ssampsread) {
                    seek_forward = 0;               //  IF run off buf end, try backward seek instead
                    break;
                }
            }
            break;                                  //  If reached 1st subzero sample after zero-crossing (still seeking forward) break
        }
        if(!seek_forward) {                         //  Else, seek backwards
            bufpos = stt + wavsetgplen;
            while(ibuf[bufpos] >= 0.0) {            //  Searching bkwds, if samps above zero, skip over samples above zero
                if(--bufpos < stt) {
                    sprintf(errstr,"No waveset-group end found (1) after time %lf\n",time);
                    return(GOAL_FAILED);
                }
            }
            while(ibuf[bufpos] < 0.0) {             //  Search bkwds, skip over samples below zero
                if(--bufpos < stt) {
                    sprintf(errstr,"No waveset-group end found (2) after time %lf\n",time);
                    return(GOAL_FAILED);
                }
            }
            bufpos++;                               //  Go back to last below-zero sample found 
        }
        wavesetbloks[m]   = wgstt;                  //  Absolute sample position of start of waveset-group
        wavesetbloks[m+1] = bufpos - stt;           //  Size of waveset group
    }
    return FINISHED;
}

/*************************** CREATE_DISTMARK_SNDBUFS **************************/

int create_distmark_sndbufs(dataptr dz)
{
    int framesize = F_SECSIZE;
    int bigbufsize, doubleframe, seccnt, n;
    doubleframe = framesize * 2;
    if(dz->sbufptr == 0 || dz->sampbuf==0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    seccnt = dz->buflen/framesize;
    if(seccnt * framesize < dz->buflen)
        seccnt++;
    dz->buflen = seccnt * framesize;            //  Input/output bufs are a multiple of framesize
    if(dz->buflen2 < doubleframe)               //  Waveset-group processing buffers must be at least 2 framesizes int
        dz->buflen2 = doubleframe;              //  because of the way seek approximates to framesize boundaries    
    bigbufsize = dz->buflen * 3;
    if((dz->bigbuf = (float *)malloc(bigbufsize * sizeof(float))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    for(n=0; n<3; n++)
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (n * dz->buflen);
    return(FINISHED);
}

int create_distmark_sndbufs2(dataptr dz)
{
    int framesize = F_SECSIZE;
    int bigbufsize, doubleframe, n, m;
    int *wavesetlens = dz->lparray[0];
    doubleframe = framesize * 2;
    dz->buflen2 = 0;
    for(n = 0,m = 1; n < dz->itemcnt; n++,m+=2)
        dz->buflen2 = max(dz->buflen2,wavesetlens[m]);
    dz->buflen2 += SAFETY;
    if(dz->buflen2 < doubleframe)               //  Waveset-group processing buffers must be at least 2 framesizes int
        dz->buflen2 = doubleframe;              //  because of the way seek approximates to framesize boundaries    
    bigbufsize = dz->buflen2 * 4;
    if((dz->bigfbuf = (float *)malloc(bigbufsize * sizeof(float))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    for(m = 0,n=3; n<7; m++,n++)                //  This creates buffers 4 to 7 out of dz->bigFFFbuf
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigfbuf + (m * dz->buflen2);
    return(FINISHED);
}

/*************************** DISTMARK **************************
 *
 *  1st waveset-group is read into BUFA
 *  2nd waveset-group is read into BUFB
 *  The smaller waveset-group is stretched to length of larger, and written into STRETCHBUF
 *  Sample-by-sample amplitude interpolation is initially between the larger waveset-group and the stretched waveset-group ...
 *      These are labelled BUF1 and BUF2 and the output goes to INTERPBUF
 *  The output in interpbuf is now Time-interpolated into the OBUF
 *
 */

int distmark(dataptr dz)
{
    int exit_status, n, m, j, k, invertphase;
    int gpstt, gplen, lastgpstt = 0, lastgplen = 0, seekto, bufpos, thispos, nextpos, biglen, timinterpcnt, samplen, obufpos, rbufpos = 0, presamps;
    float *ibuf = dz->sampbuf[0],*obuf = dz->sampbuf[1];
    float *bufa = dz->sampbuf[2],*bufb = dz->sampbuf[3],*stretchbuf = dz->sampbuf[4],*interpbuf = dz->sampbuf[5], *revbuf = dz->sampbuf[6];
    float *buf1,*buf2;
    int *wavgpdata = dz->lparray[0], *intplens = dz->lparray[1];
    double time, fracpos, thisval, valdiff, val, val1, val2, fpos, advnc = 0.0, rndval;
    double *times = dz->parray[0];

    obufpos = 0;
    for(n=0,m=0;n < dz->itemcnt;n++,m+=2) {
        invertphase = 0; 
        gpstt = wavgpdata[m];
        gplen = wavgpdata[m+1];
        time = times[n];
        memset((char *)bufa,0,dz->buflen2 * sizeof(float));
        if(n==0) {                                              //  If 1st waveset-grp
            if(gpstt != 0) {
                if((sndseekEx(dz->ifd[0],0,0))<0) {             //  If 1st mark not at file start
                    sprintf(errstr,"sndseek 2 failed.\n");      //  Copy sound before 1st mark direct to output
                    return GOAL_FAILED;
                }
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                presamps = gpstt;
                while(presamps > dz->buflen) {                  //  Copy any complete buflens of this material, directly to output file
                    if((exit_status = write_samps(ibuf,dz->ofd,dz))<0)
                        return(exit_status);
                    presamps -= dz->buflen;         
                    if((exit_status = read_samps(ibuf,dz))<0)
                        return(exit_status);
                }
                if(presamps > 0) {                              //  and if there's any incomplete buffer, copy this into the output buffer
                    memcpy((char *)obuf,(char *)ibuf,presamps * sizeof(float));
                    obufpos = presamps;
                }
            }
            seekto = (gpstt/F_SECSIZE) * F_SECSIZE;             //  Seek to nearest sector boundary below desired location
            if((sndseekEx(dz->ifd[0],seekto,0))<0) {
                sprintf(errstr,"sndseek 3 failed.\n");
                return GOAL_FAILED;
            }
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            bufpos = gpstt - seekto;                            //  Copy wavesetgroup to buffer B           
            memcpy((char *)bufb,(char *)(ibuf + bufpos),gplen * sizeof(float));
        } else {                                                //  If NOT 1st waveset-grp, copy contents of buf B to buf A
            memcpy((char *)bufa,(char *)bufb,lastgplen * sizeof(float));

        // After 1st waveset group, look for next waveset-group and put in buffer B

            seekto = (gpstt/F_SECSIZE) * F_SECSIZE;             //  Seek to nearest sector boundary below desired location
            if((sndseekEx(dz->ifd[0],seekto,0))<0) {
                sprintf(errstr,"sndseek 4 failed.\n");
                return GOAL_FAILED;
            }
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            bufpos = gpstt - seekto;                            //  Copy wavesetgroup to buffer B           
            memcpy((char *)bufb,(char *)(ibuf + bufpos),gplen * sizeof(float));

            if(gplen > lastgplen)  {                            //  If new waveset-gp (in bufB) longer than previous waveset-gp (in bufA)

                bufa[lastgplen] = 0.0f;                         //  Add wrap-around point to buffer A
                advnc = (double)lastgplen/(double)gplen;        //  Set read-incr. Expanding, so incr < 1.0
                fpos = 0.0;                                     //  Stretch contents of bufA into bufC
                k = 0;
                while(fpos < lastgplen) {
                    thispos = (int)floor(fpos);
                    nextpos = thispos+1;
                    fracpos = fpos - (double)thispos;
                    thisval = bufa[thispos];
                    valdiff = bufa[nextpos] - thisval;
                    val = thisval + (valdiff * fracpos);
                    stretchbuf[k++] = (float)val;
                    fpos += advnc;
                }
                buf1 = stretchbuf;                              //  Interp is from tstretched bufa (in stretchbuf)
                buf2 = bufb;                                    //  to bufb
                biglen = gplen;                                 //  Length of samples whos values to be interpd (biggest waveset-gp)

            } else if(gplen < lastgplen)  {                     //  If new waveset-gp (bufB) shorter than previous waveset-gp (bugA)

                bufb[gplen] = 0.0f;                             //  Add wrap-around point to buffer B
                advnc = (double)gplen/(double)lastgplen;        //  Set read-incr. Expanding ... so incr < 1.0
                fpos = 0.0;                                     //  Stretch contents of bufB into bufC
                k = 0;
                while(fpos < gplen) {
                    thispos = (int)floor(fpos);
                    nextpos = thispos+1;
                    fracpos = fpos - (double)thispos;
                    thisval = bufb[thispos];
                    valdiff = bufb[nextpos] - thisval;
                    val = thisval + (valdiff * fracpos);
                    stretchbuf[k++] = (float)val;
                    fpos += advnc;
                }
                buf1 = bufa;                                    //  Interp is from bufa 
                buf2 = stretchbuf;                              //  to tstretched version of bufb (in stretchbuf)
                biglen = lastgplen;

            } else {                                            // ELSE buffers are same length

                buf1 = bufa;                                    //  Interp is from bufa to bufb 
                buf2 = bufb;
                biglen = gplen;
            }
    
        //  Calculate number of waveset-gp copies needed for time-interpolation, and store the lengths of the copies

            if((timinterpcnt = get_timeinterpcnt(lastgplen,gplen,gpstt-lastgpstt,time,dz))<0) {
                exit_status = timinterpcnt;
                return exit_status;
            }
            for(k=0;k<lastgplen;k++) {                          //  Copy unchanged previous waveset to output
                obuf[obufpos++] = bufa[k];
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
            for(j=1;j < timinterpcnt;j++) {                     //  For each intervening waveset-gp

                for(k=0;k<biglen;k++) {                         //  Interp between sample vals in buf1 and buf2 to get new waveset shape
                    val1 = buf1[k] * (double)(timinterpcnt - j)/(double)timinterpcnt;
                    val2 = buf2[k] * (double)j/(double)timinterpcnt;
                    interpbuf[k] = (float)(val1 + val2);
                }                                               //  and add wrap-around zero at end
                interpbuf[k] = 0.0;
                samplen = intplens[j];                          //  Timestretch new shape appropriately, and put in obuf
                if(dz->brksize[DMARK_RND]) {
                    if((exit_status = read_value_from_brktable(time,DMARK_RND,dz))<0)
                        return exit_status;
                }
                if(dz->param[DMARK_RND] > 0.0) {
                    rndval = drand48() * 0.5 * dz->param[DMARK_RND];    //  Randval in range 0 to 1/2, if dmark_rnd param = (max) 1.0
                    rndval = 1.0 - rndval;                              //  Randval in range 1 to 1/2, if dmark_rnd param = (max) 1.0 : otherwise in range 1 to > 1/2
                    samplen = (int)round((double)samplen * rndval);
                }
                advnc = (double)biglen/(double)samplen;
                if(dz->vflag[DMARK_FLIP])
                    invertphase = !invertphase;
                fpos = 0.0;
                if(invertphase) {
                    rbufpos = 0;
                    while(fpos < biglen) {
                        thispos = (int)floor(fpos);
                        nextpos = thispos+1;
                        fracpos = fpos - (double)thispos;
                        thisval = interpbuf[thispos];
                        valdiff = interpbuf[nextpos] - thisval;
                        val = thisval + (valdiff * fracpos);
                        revbuf[rbufpos++] = (float)val;
                        fpos += advnc;
                    }
                    rbufpos--;
                    while(rbufpos >= 0) {
                        obuf[obufpos++] = -(revbuf[rbufpos]);       //  Time-reverse and invert phase
                        if(obufpos >= dz->buflen) {
                            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                return(exit_status);
                            obufpos = 0;
                        }
                        rbufpos--;
                    }
                } else {
                    while(fpos < biglen) {
                        thispos = (int)floor(fpos);
                        nextpos = thispos+1;
                        fracpos = fpos - (double)thispos;
                        thisval = interpbuf[thispos];
                        valdiff = interpbuf[nextpos] - thisval;
                        val = thisval + (valdiff * fracpos);
                        obuf[obufpos++] = (float)val;
                        if(obufpos >= dz->buflen) {
                            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                return(exit_status);
                            obufpos = 0;
                        }
                        fpos += advnc;
                    }
                }
            }
        }
        lastgpstt = gpstt;
        lastgplen = gplen;
    }

    //  Write tail of file

    seekto = (lastgpstt/F_SECSIZE) * F_SECSIZE;
    if((sndseekEx(dz->ifd[0],seekto,0))<0) {
        sprintf(errstr,"sndseek 5 failed.\n");
        return GOAL_FAILED;
    }
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);
    bufpos = lastgpstt - seekto;                            //  Copy wavesetgroup to buffer B           
    if(dz->vflag[DMARK_TAIL]) {
        while(dz->ssampsread > 0) {
            while(bufpos < dz->ssampsread) {
                obuf[obufpos++] = ibuf[bufpos++];
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            bufpos = 0;
        }
    } else {
        for(n=0;n < lastgplen;n++) {
            obuf[obufpos++] = ibuf[bufpos++];
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }
    }
    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/************************ HANDLE_THE_SPECIAL_DATA *********************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int exit_status;
    if(!sloom) {
        if(*cmdlinecnt <= 0) {
            sprintf(errstr,"Insufficient parameters on command line.\n");
            return(USAGE_ONLY);
        }
    }
    if((exit_status = read_mark_data((*cmdline)[0],dz))<0)
        return(exit_status);
    (*cmdline)++;       
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************************ READ_MARK_DATA ************************************/

int read_mark_data(char *filename,dataptr dz)
{
    double *time, lasttime, dummy, timestep;
    int cnt, warned = 0;
    char temp[200], *q;
//    aplptr ap;
    FILE *fp;

//    ap = dz->application;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr, "Can't open file %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    cnt = 0;
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,&dummy)) {
            cnt++;
        }
    }       
    if(cnt == 0) {
        sprintf(errstr,"No data in file %s\n",filename);
        return(DATA_ERROR);
    }
    if((dz->parray = (double **)malloc(sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s. (1)\n",filename);
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc(cnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s. (2)\n",filename);
        return(MEMORY_ERROR);
    }
    time = dz->parray[0];
    rewind(fp);
    lasttime = -1.0;
    cnt = 0;
    dz->minstep = HUGE;
    dz->maxstep = 0.0;
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,&dummy)) {
            if(dummy < 0.0 || dummy <= lasttime) {
                sprintf(errstr,"Times do not advance correctly in file %s.\n",filename);
                return(DATA_ERROR);
            }
            if(dummy >= dz->duration) {
                if (!warned) {
                    fprintf(stdout,"WARNING: Times beyond end of sndfile (%lf) in file %s. Ignoring them.\n",dz->duration,filename);
                    fflush(stdout);
                    warned = 1;
                }
                break;
            }
            *time = dummy;
            if(cnt > 0) {
                timestep = *time - lasttime;
                if(timestep < dz->minstep)
                    dz->minstep = timestep;
                if(timestep > dz->maxstep)
                    dz->maxstep = timestep;
            }
            lasttime = *time;
            time++;
            cnt++;
        }
    }
    timestep = dz->duration - lasttime;
    if(timestep < dz->minstep)
        dz->minstep = timestep;
    if(timestep > dz->maxstep)
        dz->maxstep = timestep;
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
        fflush(stdout);
    }
    dz->itemcnt = cnt;
    return(FINISHED);
}

/**************************** CHECK_DISTMARK_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_distmark_param_validity_and_consistency(dataptr dz)
{
    int exit_status;
    double maxdur, maxstr, srate = (double)dz->infile->srate;
    if(dz->brksize[DMARK_CNT]) {                    //  Find maximum duration of waveset-gps (param in mS)
        if((exit_status = get_maxvalue_in_brktable(&maxdur,DMARK_CNT,dz))<0)
            return exit_status;
    } else
        maxdur = dz->param[DMARK_CNT];
    maxdur *= MS_TO_SECS;
    if(maxdur >= dz->minstep/2.0) {
        sprintf(errstr,"Max dur of waveset-units (%lf secs) greater than 1/2 of min step between marks (%lf secs).\n",maxdur,dz->minstep);
        return(DATA_ERROR);
    }
    if(dz->brksize[DMARK_STR]) {                    //  Find maximum tmestretch value
        if((exit_status = get_maxvalue_in_brktable(&maxstr,DMARK_STR,dz))<0)
            return exit_status;
    } else
        maxstr = dz->param[DMARK_STR];
    dz->buflen  = (int)ceil(dz->maxstep * maxstr * srate) * 2;          //  Larger than largest step in infile * largest tstretch val
    dz->buflen2 = (int)ceil(maxdur * (double)dz->infile->srate) * 2;    //  Larger than largest waveset-gp unit
    return FINISHED;
}

/************************************** GET_TIMEINTERPCNT **********************************************
 *
 * Calculate number, and sizes, of intermediate time-interpd waveset-gps
 */

int get_timeinterpcnt(int lastgplen,int gplen,int markstep,double time,dataptr dz)
{
    int exit_status;
    int *unitlens = dz->lparray[1];
    double avlen;   
    int timeinterpcnt, totallen, lendiff, n, thislen, offset, lastoffset = 0, outmarkstep;
    avlen = (double)(lastgplen + gplen)/2.0;                //  Average unit length
    if(dz->brksize[DMARK_STR]) {
        if((exit_status = read_value_from_brktable(time,DMARK_STR,dz))<0)
            return exit_status;
    }
    outmarkstep = (int)round((double)markstep * dz->param[DMARK_STR]);
    timeinterpcnt = (int)round((double)outmarkstep/avlen);  //  Number of average units in total length
    totallen = 0;
    lendiff = gplen - lastgplen;
    for(n=0;n < timeinterpcnt;n++) {                        //  Using this 1st estimated value, sum sample-duration of all the interpd waveset-gps between 1 mark and next
        thislen = lastgplen + (int)round(lendiff * (double)n/(double)timeinterpcnt);
        totallen += thislen;
    }
    offset = outmarkstep - totallen;                        //  Compare this with the desired samplelen
    if(offset > 0) {
        while(offset > 0) {                                 //  IF sum falls below desired value
            lastoffset = offset;                            //  try incrementing "timeinterpcnt"
            totallen = 0;                                   //  until sum falls ABOVE desired value.
            timeinterpcnt++;
            for(n=0;n < timeinterpcnt;n++) {
                thislen = lastgplen + (int)round(lendiff * (double)n/(double)timeinterpcnt);
                totallen += thislen;
            }
            offset = outmarkstep - totallen;
        }
        if(abs(offset) > abs(lastoffset))                   //  choose the timeinterpcnt which gives closest approx to outmarkstep
            timeinterpcnt--;
    } else if(offset < 0) {                                 //  ELSE of sum falls above desired value
        while(offset < 0) {                                 //  try decrementing "timeinterpcnt"
            lastoffset = offset;                            //  until sum falls BELOW desired value.    
            totallen = 0;
            timeinterpcnt--;
            if(timeinterpcnt == 0)
                break;
            for(n=0;n < timeinterpcnt;n++) {
                thislen = lastgplen + (int)round(lendiff * (double)n/(double)timeinterpcnt);
                totallen += thislen;
            }
            offset = outmarkstep - totallen;
        }
        if(abs(offset) > abs(lastoffset))                   //  choose the timeinterpcnt which gives closest approx to outmarkstep
            timeinterpcnt++;
    }
    if(timeinterpcnt < 2) {                                 //  Force at least 2 interp steps (1 = orig waveset-gp, 2 = interp towards next waveset-gp)
        fprintf(stdout,"WARNING: Less than 2 interpolation steps before time %lf\n",time);
        fflush(stdout);
        timeinterpcnt = 2;
    } else if(dz->vflag[DMARK_FLIP] && !EVEN(timeinterpcnt))
        timeinterpcnt++;
    if(timeinterpcnt >= dz->stepstorecnt) {
        sprintf(errstr,"Memory store for sizes of intermediate waveset-groups (%d), not large enough (%d needed).\n",dz->stepstorecnt,timeinterpcnt);
        return PROGRAM_ERROR;
    }
    for(n=0;n < timeinterpcnt;n++) {
        thislen = lastgplen + (int)round(lendiff * (double)n/(double)timeinterpcnt);
        unitlens[n] = thislen;
    }
    return timeinterpcnt;
}


int distmark2(dataptr dz)
{
    int exit_status, n, m, j, k, invertphase, done = 0;
    int gpstt, gplen, lastgpstt = 0, lastgplen = 0, seekto, bufpos, thispos, nextpos, biglen, timinterpcnt, samplen, obufpos, rbufpos = 0, presamps;
    float *ibuf = dz->sampbuf[0],*obuf = dz->sampbuf[1];
    float *bufa = dz->sampbuf[2],*bufb = dz->sampbuf[3],*stretchbuf = dz->sampbuf[4],*interpbuf = dz->sampbuf[5], *revbuf = dz->sampbuf[6];
    float *buf1,*buf2;
    int *wavgpdata = dz->lparray[0], *intplens = dz->lparray[1];
    double time, fracpos, thisval, valdiff, val, val1, val2, fpos, advnc = 0.0, rndval, thistime;
    double *times = dz->parray[0];
    int lastbufpos = 0, samps_to_copy;
    obufpos = 0;
    for(n=0,m=0;n < dz->itemcnt;n++,m+=2) {
        invertphase = 0; 
        gpstt = wavgpdata[m];
        gplen = wavgpdata[m+1];
        time = times[n];
        if((dz->vflag[1] == 0 && EVEN(n)) || (dz->vflag[1] == 1 && ODD(n))) {
            memset((char *)bufa,0,dz->buflen2 * sizeof(float));
            if (n > 0) {                                        //  If not the first marked-waveset-group (0 is EVEN)
                lastbufpos = lastbufpos + lastgplen;            //  copy from end of last interp-block to current interp-block start
                j = lastbufpos;
                samps_to_copy = wavgpdata[m] - wavgpdata[m-2];
                for(k = 0; k < samps_to_copy;k++) {
                    obuf[obufpos++] = ibuf[j++];
                    if(obufpos >= dz->buflen) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        obufpos = 0;
                    }
                    if(j >= dz->buflen) {
                        if((exit_status = read_samps(ibuf,dz))<0)
                            return(exit_status);
                        if(dz->ssampsread == 0) {
                            done = 1;
                            break;
                        }
                        j = 0;
                    }
                }
                if(done)
                    break;
            }
            if(n == 0 && gpstt != 0) {
                if((sndseekEx(dz->ifd[0],0,0))<0) {             //  If 1st mark not at file start
                    sprintf(errstr,"sndseek 2 failed.\n");      //  Copy sound before 1st mark direct to output
                    return GOAL_FAILED;
                }
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                presamps = gpstt;
                while(presamps > dz->buflen) {                  //  Copy any complete buflens of this material, directly to output file
                    if((exit_status = write_samps(ibuf,dz->ofd,dz))<0)
                        return(exit_status);
                    presamps -= dz->buflen;         
                    if((exit_status = read_samps(ibuf,dz))<0)
                        return(exit_status);
                }
                if(presamps > 0) {                              //  and if there's any incomplete buffer, copy this into the output buffer
                    memcpy((char *)obuf,(char *)ibuf,presamps * sizeof(float));
                    obufpos = presamps;
                }
            }
            seekto = (gpstt/F_SECSIZE) * F_SECSIZE;             //  Seek to nearest sector boundary below desired location
            if((sndseekEx(dz->ifd[0],seekto,0))<0) {
                sprintf(errstr,"sndseek 3 failed.\n");
                return GOAL_FAILED;
            }
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            bufpos = gpstt - seekto;                            //  Copy wavesetgroup to buffer B           
            memcpy((char *)bufb,(char *)(ibuf + bufpos),gplen * sizeof(float));
        } else {                                                //  If NOT 1st waveset-grp, copy contents of buf B to buf A
            memcpy((char *)bufa,(char *)bufb,lastgplen * sizeof(float));

        // After 1st waveset group, look for next waveset-group and put in buffer B

            seekto = (gpstt/F_SECSIZE) * F_SECSIZE;             //  Seek to nearest sector boundary below desired location
            if((sndseekEx(dz->ifd[0],seekto,0))<0) {
                sprintf(errstr,"sndseek 4 failed.\n");
                return GOAL_FAILED;
            }
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            bufpos = gpstt - seekto;                            //  Copy wavesetgroup to buffer B           
            lastbufpos = bufpos;
            lastgplen = gplen;
            memcpy((char *)bufb,(char *)(ibuf + bufpos),gplen * sizeof(float));

            if(gplen > lastgplen)  {                            //  If new waveset-gp (in bufB) longer than previous waveset-gp (in bufA)

                bufa[lastgplen] = 0.0f;                         //  Add wrap-around point to buffer A
                advnc = (double)lastgplen/(double)gplen;        //  Set read-incr. Expanding, so incr < 1.0
                fpos = 0.0;                                     //  Stretch contents of bufA into bufC
                k = 0;
                while(fpos < lastgplen) {
                    thispos = (int)floor(fpos);
                    nextpos = thispos+1;
                    fracpos = fpos - (double)thispos;
                    thisval = bufa[thispos];
                    valdiff = bufa[nextpos] - thisval;
                    val = thisval + (valdiff * fracpos);
                    stretchbuf[k++] = (float)val;
                    fpos += advnc;
                }
                buf1 = stretchbuf;                              //  Interp is from tstretched bufa (in stretchbuf)
                buf2 = bufb;                                    //  to bufb
                biglen = gplen;                                 //  Length of samples whos values to be interpd (biggest waveset-gp)

            } else if(gplen < lastgplen)  {                     //  If new waveset-gp (bufB) shorter than previous waveset-gp (bugA)

                bufb[gplen] = 0.0f;                             //  Add wrap-around point to buffer B
                advnc = (double)gplen/(double)lastgplen;        //  Set read-incr. Expanding ... so incr < 1.0
                fpos = 0.0;                                     //  Stretch contents of bufB into bufC
                k = 0;
                while(fpos < gplen) {
                    thispos = (int)floor(fpos);
                    nextpos = thispos+1;
                    fracpos = fpos - (double)thispos;
                    thisval = bufb[thispos];
                    valdiff = bufb[nextpos] - thisval;
                    val = thisval + (valdiff * fracpos);
                    stretchbuf[k++] = (float)val;
                    fpos += advnc;
                }
                buf1 = bufa;                                    //  Interp is from bufa 
                buf2 = stretchbuf;                              //  to tstretched version of bufb (in stretchbuf)
                biglen = lastgplen;

            } else {                                            // ELSE buffers are same length

                buf1 = bufa;                                    //  Interp is from bufa to bufb 
                buf2 = bufb;
                biglen = gplen;
            }

            //  Calculate number of waveset-gp copies needed for time-interpolation, and store the lengths of the copies

            if((timinterpcnt = get_timeinterpcnt(lastgplen,gplen,gpstt-lastgpstt,time,dz))<0) {
                exit_status = timinterpcnt;
                return exit_status;
            }
            if(dz->brksize[DMARK_GAIN]) {
                thistime = (dz->total_samps_written + obufpos)/(double)dz->infile->srate;
                if((exit_status = read_value_from_brktable(thistime,DMARK_GAIN,dz)) < 0)
                    return exit_status;
            }
            for(k=0;k<lastgplen;k++) {                          //  Copy unchanged previous waveset to output
                obuf[obufpos++] = (float)(bufa[k] * dz->param[DMARK_GAIN]);
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
            for(j=1;j < timinterpcnt;j++) {                     //  For each intervening waveset-gp

                for(k=0;k<biglen;k++) {                         //  Interp between sample vals in buf1 and buf2 to get new waveset shape
                    val1 = buf1[k] * (double)(timinterpcnt - j)/(double)timinterpcnt;
                    val2 = buf2[k] * (double)j/(double)timinterpcnt;
                    interpbuf[k] = (float)(val1 + val2);
                }                                               //  and add wrap-around zero at end
                interpbuf[k] = 0.0;
                samplen = intplens[j];                          //  Timestretch new shape appropriately, and put in obuf
                if(dz->brksize[DMARK_RND]) {
                    if((exit_status = read_value_from_brktable(time,DMARK_RND,dz))<0)
                        return exit_status;
                }
                if(dz->param[DMARK_RND] > 0.0) {
                    rndval = drand48() * 0.5 * dz->param[DMARK_RND];    //  Randval in range 0 to 1/2, if dmark_rnd param = (max) 1.0
                    rndval = 1.0 - rndval;                              //  Randval in range 1 to 1/2, if dmark_rnd param = (max) 1.0 : otherwise in range 1 to > 1/2
                    samplen = (int)round((double)samplen * rndval);
                }
                advnc = (double)biglen/(double)samplen;
                if(dz->vflag[DMARK_FLIP])
                    invertphase = !invertphase;
                fpos = 0.0;
                if(invertphase) {
                    rbufpos = 0;
                    while(fpos < biglen) {
                        thispos = (int)floor(fpos);
                        nextpos = thispos+1;
                        fracpos = fpos - (double)thispos;
                        thisval = interpbuf[thispos];
                        valdiff = interpbuf[nextpos] - thisval;
                        val = thisval + (valdiff * fracpos);
                        revbuf[rbufpos++] = (float)val;
                        fpos += advnc;
                    }
                    rbufpos--;
                    while(rbufpos >= 0) {
                        obuf[obufpos++] = (float)(-revbuf[rbufpos] * dz->param[DMARK_GAIN]);        //  Time-reverse and invert phase
                        if(obufpos >= dz->buflen) {
                            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                return(exit_status);
                            obufpos = 0;
                        }
                        rbufpos--;
                    }
                } else {
                    while(fpos < biglen) {
                        thispos = (int)floor(fpos);
                        nextpos = thispos+1;
                        fracpos = fpos - (double)thispos;
                        thisval = interpbuf[thispos];
                        valdiff = interpbuf[nextpos] - thisval;
                        val = thisval + (valdiff * fracpos);
                        obuf[obufpos++] = (float)(val * dz->param[DMARK_GAIN]);
                        if(obufpos >= dz->buflen) {
                            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                return(exit_status);
                            obufpos = 0;
                        }
                        fpos += advnc;
                    }
                }
            }
        }
        lastgpstt = gpstt;
        lastgplen = gplen;
    }
    if(!done)  {
        seekto = (lastgpstt/F_SECSIZE) * F_SECSIZE;
        if((sndseekEx(dz->ifd[0],seekto,0))<0) {
            sprintf(errstr,"sndseek 5 failed.\n");
            return GOAL_FAILED;
        }
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        bufpos = lastgpstt - seekto;                            //  Copy wavesetgroup to buffer B           
        while(dz->ssampsread > 0) {
            while(bufpos < dz->ssampsread) {
                obuf[obufpos++] = ibuf[bufpos++];
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            bufpos = 0;
        }
    }
    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }
    return FINISHED;
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

