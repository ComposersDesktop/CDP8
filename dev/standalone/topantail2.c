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

 //#ifdef unix 
#define round(x) lround((x))
//#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int sloom = 0;
int sloombatch = 0;
int anal_infiles = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int setup_topntail_application(dataptr dz);
//static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_topntail_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int setup_and_init_input_param_activity(dataptr dz,int tipc);
static int setup_input_param_defaultval_stores(int tipc,aplptr ap);
static int establish_application(dataptr dz);
static int setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int mark_parameter_types(dataptr dz,aplptr ap);
//static int assign_file_data_storage(int infilecnt,dataptr dz);
static int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int check_topntail_param_validity_and_consistency(dataptr dz);
//static double dbtolevel(double val);
//static int topntail(dataptr dz);
static int create_topntail_buffer(dataptr dz);
static int top_and_tail(dataptr dz);
static int get_startsamp(float *ibuf,int chans,int nbuff,int *startsamp,double gate,double ngate,dataptr dz);
static int get_endsamp(float *ibuf,int chans,int nbuff,int *endsamp,double gate,double ngate,dataptr dz);
static int do_startsplice(int splicecnt,int startsamp,int *sampseek,int input_report,dataptr dz);
static int advance_to_endsamp(int endsamp,int *sampseek,int input_report,dataptr dz);
static int do_end_splice(int *k,int splicecnt,int input_report,dataptr dz);

#define TOPNTAIL -1

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
    int n;
    //aplptr ap;
    int is_launched = FALSE;
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
                        /* CHECK FOR SOUNDLOOM */
    if(sflinit("cdp")){
        sfperror("cdp: initialisation\n");
        return(FAILED);
    }
                          /* SET UP THE PRINCIPLE DATASTRUCTURE */
    if((exit_status = establish_datastructure(&dz))<0) {                    // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
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
    if((exit_status = setup_topntail_application(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {       // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //ap = dz->application;

    // parse_infile_and_hone_type() = 
    if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_topntail_param_ranges_and_defaults(dz))<0) {
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
//  handle_special_data()       redundant
 
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency() redundant
    if((exit_status = check_topntail_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    dz->bufcnt = 1;
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

    if((exit_status = create_topntail_buffer(dz))<0) {                          // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess()                        redundant
    //spec_process_file =
    if((exit_status = top_and_tail(dz))<0) {
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
    if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
        sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
        return(DATA_ERROR);
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

/************************* SETUP_TOPNTAIL_APPLICATION *******************/

int setup_topntail_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,0,2,2,"dd"))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"sb",2,"dd","",0,0,""))<0)
        return(FAILED);
    // set_legal_infile_structure -->
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
    return(FINISHED);
}

/************************* SETUP_TOPNTAIL_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_topntail_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()

    ap->lo[0]   = 0;
    ap->hi[0]   = 1;
    ap->default_val[0]  = 0;
    ap->lo[1]   = 0;
    ap->hi[1]   = 1;
    ap->default_val[1]  = 0;
    ap->lo[2]  = 2;
    ap->hi[2]  = 200;
    ap->default_val[2]  = 15;
    ap->lo[3]  = 0;
    ap->hi[3]  = 1000;
    ap->default_val[3]  = 0;

    dz->maxmode = 0;
    put_default_vals_in_all_params(dz);
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
    usage2("topantail");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"topantail"))
        dz->process = TOPNTAIL;
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
    if(!strcmp(str,"topantail")) {
        fprintf(stderr,
        "USAGE:\n"
        "topantail2 topantail infile outfile startgate endgate [-ssplicelen] [-bbacktrack]\n"
        "\n"
        "STARTGATE  level at start before which sound is to be removed (0-1).\n"
        "ENDGATE    level at end after which sound is to be removed (0-1).\n"
        "SPLICELEN  splicelen in mS.\n"
        "BACKTRACK  backtrack from initial gate point to an earlier splice point, in mS.\n"
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

/**************************** CHECK_TOPNTAIL_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_topntail_param_validity_and_consistency(dataptr dz)
{
    dz->iparam[3] = (int)round(dz->param[3] * MS_TO_SECS * dz->infile->srate) * dz->infile->channels;
    return FINISHED;
}

/*************************** CREATE_TOPNTAIL_BUFFER ************************/

int create_topntail_buffer(dataptr dz)
{
    size_t bigbufsize;
    bigbufsize = (long)Malloc(-1);
    dz->buflen = (int)(bigbufsize / sizeof(float));
    dz->buflen = (dz->buflen / dz->infile->channels) * dz->infile->channels;
    if((dz->bigbuf = (float*) Malloc(dz->buflen * sizeof(float)))== NULL){
        sprintf(errstr, "Can't allocate memory for sound.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/******************************* TOP_AND_TAIL *******************************/

int top_and_tail(dataptr dz)
{
    int  exit_status;
    float *ibuf = dz->bigbuf;
    float *obuf = ibuf;
    int  nbuff;
    int  chans = dz->infile->channels;
    int startsamp = 0;
    int endsamp = dz->insams[0];
    int splicecnt   = round(dz->param[2] * MS_TO_SECS * (double)dz->infile->srate);
    int splicesamps = splicecnt * chans;
    int sampseek = 0;
    int startsplice, endsplice = 0, samps_to_write;
    double gate1, gate2;

    dz->ssampsread = 0;

    nbuff = dz->insams[0]/dz->buflen;
    if(nbuff * dz->buflen < dz->insams[0])
        nbuff++;        /* number of buffers contaning entire file */
    gate1 = dz->param[0] * (double)F_MAXSAMP;
    gate2 = dz->param[1] * (double)F_MAXSAMP;
    if((exit_status = get_startsamp(ibuf,chans,nbuff,&startsamp,gate1,-gate1,dz))<0)
        return(exit_status);
    if((exit_status = get_endsamp(ibuf,chans,nbuff,&endsamp,gate2,-gate2,dz))<0)
        return(exit_status);
    if(endsamp == startsamp) {
        sprintf(errstr,"At this gate level, entire file will be removed.\n");
        return(GOAL_FAILED);
    }
    if((startsplice = startsamp - splicesamps) < 0)
        startsplice = 0;
    endsplice = endsamp;
    if((endsamp -= splicesamps) <= 0) {
        sprintf(errstr,"At this gate level, entire file will be removed.\n");
        return(GOAL_FAILED);
    }
    dz->ssampsread = 0;

    if(sndseekEx(dz->ifd[0],startsamp,0)<0) {
        sprintf(errstr,"sndseek() failed: 3\n");
        return(SYSTEM_ERROR);
    }
    if(startsplice != 0) {
        if((exit_status = do_startsplice(splicecnt,startsamp,&sampseek,0,dz))<0)
            return(exit_status);
    } else {
        if((exit_status = read_samps(dz->bigbuf,dz))<0) /* read buffer with additional sector */
            return(exit_status);
    }
//  endsamp = endsplice = dz->insams[0];

    if((exit_status = advance_to_endsamp(endsamp,&sampseek,0,dz))<0)
        return(exit_status);

    samps_to_write = endsamp - sampseek;

    if((exit_status = do_end_splice(&samps_to_write,splicecnt,0,dz))<0)
        return(exit_status);
    if(samps_to_write > 0) {
        if((exit_status = write_samps(obuf,samps_to_write,dz))<0)
            return(exit_status);
    }
    dz->total_samps_written = endsplice - startsplice;
    return(FINISHED);
}

/******************************* GET_STARTSAMP *******************************/

int get_startsamp(float *ibuf,int chans,int nbuff,int *startsamp,double gate,double ngate,dataptr dz)
{
    int exit_status;
    int gotstart = FALSE;
    int thisbuff = 0;
    int n;
    while(!gotstart && thisbuff < nbuff) {
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        for(n = 0;n < dz->ssampsread - 1;n++) {
            if(ibuf[n] > gate || ibuf[n] < ngate) {
                gotstart = TRUE;
                break;
            }
        }
        if(gotstart) {
            *startsamp  = n + (thisbuff * dz->buflen);
            *startsamp = (*startsamp/chans) * chans;    /* align to channel group boundary */
            break;
        }
        thisbuff++;
    }
    if(dz->iparam[3] > 0) {
        *startsamp -= dz->iparam[3];
        if(*startsamp <0)
            *startsamp = 0;
    }
    if(!gotstart) {
        sprintf(errstr,"Entire file is below gate level AAA\n");
        return(GOAL_FAILED);
    }
    return(FINISHED);
}

/******************************* GET_ENDSAMP *******************************/

int get_endsamp(float *ibuf,int chans,int nbuff,int *endsamp,double gate,double ngate,dataptr dz)
{
    int exit_status;
    int n;
    int gotend = FALSE;
    int thisbuff = nbuff - 1;   /* buffer that contains last sample */
    while(!gotend && thisbuff >= 0) {
        if(sndseekEx(dz->ifd[0],thisbuff * dz->buflen,0)<0) {
            sprintf(errstr,"sndseek() failed: 1\n");
            return(SYSTEM_ERROR);
        }
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        for(n = dz->ssampsread - 1;n>=0;n--) {
            if(ibuf[n] > gate || ibuf[n] < ngate) {
                gotend = TRUE;
                break;
            }
        }
        if(gotend) {
            *endsamp = n + (thisbuff * dz->buflen);
            *endsamp = (*endsamp/chans) * chans;    /* align to channel group boundary */
            break;
        }
        thisbuff--;
    }
    if(!gotend) {
        sprintf(errstr,"Entire file is below gate level.\n");
        return(GOAL_FAILED);
    }
    return(FINISHED);
}

/******************************* DO_STARTSPLICE *******************************/

int do_startsplice(int splicecnt,int startsamp,int *sampseek,int input_report,dataptr dz)
{
    int exit_status;
    int chans = dz->infile->channels;
    int k, samps_written;
    double aincr, a1;
    int n;
    int m;

// NEW TW June 2004
    *sampseek = startsamp;
    if((exit_status = read_samps(dz->bigbuf,dz))<0) /* read buffer with additional sector */
        return(exit_status);
    k = 0;
    aincr = 1.0/(double)splicecnt;
    a1 = 0.0;
    for(n = 0;n < splicecnt;n++) {
        for(m = 0; m < chans; m++) {
            dz->bigbuf[k] = (float) ((double)dz->bigbuf[k] * a1);
            k++;
        }
        if(k >= dz->ssampsread) {
            if(input_report) {
                if((exit_status = write_samps_no_report(dz->bigbuf,dz->buflen,&samps_written,dz))<0)
                    return(exit_status);
            } else {
                if((exit_status = write_samps(dz->bigbuf,dz->buflen,dz))<0)
                    return(exit_status);
            }
            *sampseek += dz->ssampsread;
            if((exit_status = read_samps(dz->bigbuf,dz))<0)     /*RWD added * */
                return(exit_status);
            if(dz->ssampsread <=0)
                return(FINISHED);
            k = 0;
        }
        a1 += aincr;
    }
    return(FINISHED);
}

/******************************* ADVANCE_TO_ENDSAMP *******************************/

int advance_to_endsamp(int endsamp,int *sampseek,int input_report,dataptr dz)
{
    int exit_status;
    int samps_written;
    while(endsamp > *sampseek + dz->ssampsread) {
        if(dz->ssampsread > 0) {
            if(input_report) {
                if((exit_status = write_samps_no_report(dz->bigbuf,dz->ssampsread,&samps_written,dz))<0)
                    return(exit_status);
            } else {
                if((exit_status = write_samps(dz->bigbuf,dz->ssampsread,dz))<0)
                    return(exit_status);
            }
        }
        *sampseek += dz->ssampsread;
        if((exit_status = read_samps(dz->bigbuf,dz))<0)
            return(exit_status);
        if(dz->ssampsread ==0) {
            return FINISHED;
        }
    }
    return(FINISHED);
}

/******************************* DO_END_SPLICE *******************************/

int do_end_splice(int *k,int splicecnt,int input_report,dataptr dz)
{
    int exit_status;
    int chans = dz->infile->channels;
    int n, samps_written;
    int m;
    double aincr = -(1.0/splicecnt);
    double a1 = 1.0 + aincr;
    for(n = 0;n < splicecnt;n++) {
        for(m = 0; m < chans; m++) {
            dz->bigbuf[*k] = (float) ((double)dz->bigbuf[*k] * a1);
            (*k)++;
        }
        if(*k >= dz->ssampsread) {
            if(dz->ssampsread > 0) {
                if(input_report) {
                    if((exit_status = write_samps_no_report(dz->bigbuf,dz->ssampsread,&samps_written,dz))<0)
                        return(exit_status);
                } else {
                    if((exit_status = write_samps(dz->bigbuf,dz->ssampsread,dz))<0)
                        return(exit_status);
                }
            }
            if((exit_status = read_samps(dz->bigbuf,dz))<0)
                return(exit_status);
            if(dz->ssampsread <=0) {
                return(FINISHED);
            }
            *k = 0;
        }
        a1 += aincr;
    }
    return(FINISHED);
}

