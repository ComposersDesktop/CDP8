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
/*
 *  SPLICES NOT WORKING IN REASSEMBLY.
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
#include <formants.h>
#include <speccon.h>

#define SAFETY 64
#define maxstep is_sharp
#define minstep scalefact
#define maxxstep rampbrksize

#define SSPLICELEN 15

#define DISTMORERAND (0.2)

#define ZREPETS 0
#define ZDUR    1
#define ZMINSIZ 2
#define ZFRAC   3

#ifdef unix
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.2.0";

//CDP LIB REPLACEMENTS
static int setup_distmore_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_distmore_param_ranges_and_defaults(dataptr dz);
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
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int read_mark_data(char *filename,dataptr dz);
//static int read_feature_data(char *filename,dataptr dz);
static int check_distmore_param_validity_and_consistency(dataptr dz);
static int distmore_param_preprocess(dataptr dz);
static int distbright (dataptr dz);
static int distdouble (dataptr dz);

static int generate_tail_segments(dataptr dz);
static int create_distmore_sndbufs(dataptr dz);
static int recreate_distmore_sndbufs(dataptr dz);
static void reversebuf2(float *buf1,float *buf2,int sampcnt);
static void reversebuf(float *buf,int sampcnt);
static int segsbkwd(dataptr dz);
static int segszig(dataptr dz);
static void cutend_and_splicend(float *buf,int cutlen,int sampcnt,int splicelen);
static double getstepratio(dataptr dz);

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
        if((get_the_process_no(argv[0],dz))<0)
            return(FAILED);
        cmdline++;
        cmdlinecnt--;
        switch(dz->process) {
        case(DISTBRIGHT):   dz->maxmode = 3;    break;
        case(DISTDOUBLE):   dz->maxmode = 0;    break;
        case(SEGSBKWD):     dz->maxmode = 9;    break;
        case(SEGSZIG):      dz->maxmode = 3;    break;
        }
        if(dz->maxmode > 0) {
            if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
                print_messages_and_close_sndfiles(exit_status,is_launched,dz);
                return(exit_status);
            }
            cmdline++;
            cmdlinecnt--;
        }
        // setup_particular_application =
        if((exit_status = setup_distmore_application(dz))<0) {
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

    // parse_infile_and_hone_type() = 
    if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
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
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_distmore_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_extra_infiles() : redundant
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
    dz->array_cnt  = 0;
    dz->iarray_cnt = 0;
    dz->larray_cnt = 0;
    if(dz->process == DISTBRIGHT) {
        dz->array_cnt  = 3;     //  3 for: Seg starttimes, average zerocross rates, possibly expanded seg starttimes
        dz->iarray_cnt = 2;     //  2 for: H/T labels, reordering list
        dz->larray_cnt = 1;     //  For times as sample-cnts
    }
    if(dz->process == SEGSBKWD || dz->process == SEGSZIG) {
        dz->array_cnt  = 1;     //  1 for: Seg starttimes
        dz->larray_cnt = 1;     //  For times as sample-cnts
    }
//  handle_special_data ....
    if(dz->process != DISTDOUBLE && !(dz->process == SEGSZIG && dz->mode != 0)) {
        if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency .....
    if((exit_status = check_distmore_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = create_distmore_sndbufs(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(dz->process == DISTDOUBLE) {
        if((exit_status = distmore_param_preprocess(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    is_launched = TRUE;
    switch(dz->process) {
    case(DISTBRIGHT):
        if((exit_status = distbright(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    case(DISTDOUBLE):               
        if((exit_status = distdouble(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    case(SEGSBKWD):             
        if((exit_status = segsbkwd(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    case(SEGSZIG):              
        if((exit_status = segszig(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    }
    if((exit_status = complete_output(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
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

/************************* SETUP_DISTMORE_APPLICATION *******************/

int setup_distmore_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    switch(dz->process) {
    case(DISTBRIGHT):
        if((exit_status = set_param_data(ap,MARKLIST,0,0,""))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"s",1,"d","d",1,0,"0"))<0)
            return(FAILED);
        dz->maxmode = 3;
        break;
    case(DISTDOUBLE):
        if((exit_status = set_param_data(ap,0,1,1,"i"))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
            return(FAILED);
        dz->maxmode = 0;
        break;
    case(SEGSBKWD):
        if((exit_status = set_param_data(ap,MARKLIST,0,0,""))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
            return(FAILED);
        dz->maxmode = 9;
        break;
    case(SEGSZIG):
        switch(dz->mode) {
        case(0):    exit_status = set_param_data(ap,MARKLIST,2,1,"I0"); break;
        case(1):    exit_status = set_param_data(ap,0       ,2,1,"i0"); break;
        case(2):    exit_status = set_param_data(ap,0       ,2,2,"id"); break;
        }
        if(exit_status < 0)
            return(FAILED);
        switch(dz->mode) {
        case(0): exit_status = set_vflgs(ap,"sp",2,"dD","l",1,0,"0"); break;
        case(1): exit_status = set_vflgs(ap,"sp",2,"dd","l",1,0,"0"); break;
        case(2): exit_status = set_vflgs(ap,"sp",2,"dd","l",1,0,"0"); break;
        }
        if(exit_status < 0)
            return(FAILED);
        dz->maxmode = 3;
        break;
    }
    dz->input_data_type = SNDFILES_ONLY;
    dz->process_type    = UNEQUAL_SNDFILE;  
    dz->outfiletype     = SNDFILE_OUT;
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
            return(DATA_ERROR);
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

/************************* SETUP_DISTMORE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_distmore_param_ranges_and_defaults(dataptr dz)
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
    case(DISTBRIGHT):
        ap->lo[0] = 2;
        ap->hi[0] = 15;
        ap->default_val[0]  = 15;
        dz->maxmode = 3;
        break;
    case(DISTDOUBLE):
        ap->lo[0] = 1;
        ap->hi[0] = 4;
        ap->default_val[0]  = 1;
        dz->maxmode = 0;
        break;
    case(SEGSBKWD):
        dz->maxmode = 0;
        break;
    case(SEGSZIG):
        ap->lo[0] = 1;
        ap->hi[0] = 64;
        ap->default_val[0]  = 1;
        if(dz->mode==2) {
            ap->lo[1] = dz->duration * 2;
            ap->hi[1] = dz->duration * 64;
            ap->default_val[1]  = dz->duration * 4;
        }
        ap->lo[2] = 0;
        ap->hi[2] = 1000;
        ap->default_val[2] = 0;
        ap->lo[3] = 0.2;
        ap->hi[3] = 1.0;
        ap->default_val[3] = 1.0;
        dz->maxmode = 3;
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
            if((exit_status = setup_distmore_application(dz))<0)
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
    fprintf(stderr,
    "\nUSAGE: distmore NAME (mode) infile outfile (parameters)\n"
    "\n"
    "MORE WAVESET DISTORTION PROGRAMS\n"
    "\n"
    "where NAME can be any one of\n"
    "\n"
    "bright   double   segsbkwd   segszig\n"
    "\n"
    "Type 'distmore bright' for more info on distmore bright.. ETC.\n");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(     !strcmp(prog_identifier_from_cmdline,"bright"))     dz->process = DISTBRIGHT;
    else if(!strcmp(prog_identifier_from_cmdline,"double"))     dz->process = DISTDOUBLE;
    else if(!strcmp(prog_identifier_from_cmdline,"segsbkwd"))   dz->process = SEGSBKWD;
    else if(!strcmp(prog_identifier_from_cmdline,"segszig"))    dz->process = SEGSZIG;
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
    if(!strcmp(str,"bright")) {
        fprintf(stderr,
        "USAGE:  distmore bright 1-3 infil outfil marklist [-ssplicelen -d]\n"
        "\n"
        "\nReorders sound segments in order of average zero-crossing rate\n."
        "\n"
        "MARKLIST A list of timemarks in source, marking (paired) Heads and Tails.\n"
        "         e.g. consonant onset, and vowel continuation of source.\n"
        "         (It is assumed that the first mark is at a Head segment.)\n"
        "\n"
        "Extract data ...\n"
        "MODE 1   from Heads, & from Tails cut to segments, size approx equal to Heads.\n"
        "MODE 2   from Heads and Tails, as defined by marklist.\n"
        "MODE 3   from Tails only.\n"
        "\n"
        "SPLICELEN Length of splice in mS\n"
        "-d        Output in decreasing order of brightness. (Default: increasing).\n");
    } else if(!strcmp(str,"double")) {
        fprintf(stderr,
        "USAGE:  distmore double infil outfil mult\n"
        "\n"
        "Doubles (quadruples etc.) frq of each waveset\n."
        "\n"
        "MULT   Octave step up (1-4).\n");
    } else if(!strcmp(str,"segsbkwd")) {
        fprintf(stderr,
        "USAGE:  distmore segsbkwd 1-9 infil outfil marklist\n"
        "\n"
        "Reverses certain (sets of) segments\n."
        "\n"
        "MODE 1: Reverse Tails\n"
        "MODE 2: Reverse Heads\n"
        "MODE 3: Reverse Head+Tail pairs\n"
        "MODE 4: Reverse Head & Tail+Head+Tail set\n"
        "MODE 5: Reverse Head+Tail+Head+Tail set\n"
        "MODE 6: Reverse Head & Tail+Head+Tail+Head+Tail set\n"
        "MODE 7: Reverse Head+Tail+Head+Tail+Head+Tail set\n"
        "MODE 8: Reverse Head & Tail+Head+Tail+Head+Tail+Head+Tail set\n"
        "MODE 9: Reverse Head+Tail+Head+Tail+Head+Tail+Head+Tail set\n"
        "\n"
        "MARKLIST A list of timemarks in source, marking (paired) Heads and Tails.\n"
        "         e.g. consonant onset, and vowel continuation of source.\n"
        "         (It is assumed that the first mark is at a Head segment.)\n");
    } else if(!strcmp(str,"segszig")) {
        fprintf(stderr,
        "USAGE:  distmore segszig 1 infil outfil marklist repets [-sshrinkto] [-pprop] [-l]\n"
        "OR:     distmore segszig 2 infil outfil          repets [-sshrinkto] [-pprop] [-l]\n"
        "OR:     distmore segszig 3 infil outfil          repets dur [-sshrinkto] [-pprop]\n"
        "\n"
        "MODE1     Zigzags across tail segments of a soundfile while playing it.\n"
        "MODES 2&3 Zigzag across entire soundfile.\n"
        "\n"
        "MARKLIST A list of timemarks in source, marking (paired) Heads and Tails.\n"
        "         e.g. consonant onsets, and vowel continuations in source.\n"
        "         (First mark is assumed to be a Head segment).\n"
        "         In Mode 2, the whole file is processed.\n"
        "\n"
        "REPETS   Number of zigzags (timevariable).\n"
        "\n"
        "SHRINKTO If set to zero, has no effect.\n"
        "         Otherwize Zigzags contract to minimum size 'minsiz' mS (Range %d upwards).\n"
        "\n"
        "DUR      Duration of zigged output.\n"
        "\n"
        "PROP     Proportion of Tail to use. Default, all of it. (timevariable).\n"
        "         CARE: If length of used-portion of any particular tail too short,\n"
        "         (less than \"SHRINKTO\" size) zigs for that tail will not shrink.\n"
        "-l       Shrink zigs logarithmically (default, linear shrink).\n",(SSPLICELEN * 2) + 1);
    } else {
        fprintf(stdout,"Unknown option '%s'\n",str);
        fflush(stdout);
    }
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
    int arraysize;
    char temp[200], *q;
    aplptr ap;
    FILE *fp;

    ap = dz->application;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr, "Can't open file %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    cnt = 0;
    lasttime = -1.0;
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
                if(!warned) {
                    fprintf(stdout,"WARNING: Times beyond end of sndfile (%lf) in file %s. Ignoring them.\n",dz->duration,filename);
                    fflush(stdout);
                    warned = 1;
                }
                break;
            }
            if(cnt > 0) {
                timestep = dummy - lasttime;
                if(timestep < dz->minstep)
                    dz->minstep = timestep;
                if(timestep > dz->maxstep)
                    dz->maxstep = timestep;
            }
            lasttime = dummy;
            cnt++;
        }
    }       
    if(cnt == 0) {
        sprintf(errstr,"No data in file %s\n",filename);
        return(DATA_ERROR);
    }
    if(cnt < 4) {
        sprintf(errstr,"Insufficient data (%d values) in file %s : Need at least 4\n",cnt,filename);
        return(DATA_ERROR);
    }
    dz->itemcnt = cnt;
    timestep = dz->duration - lasttime;
    if(timestep < dz->minstep)
        dz->minstep = timestep;
    if(timestep > dz->maxstep)
        dz->maxstep = timestep;

    if((dz->parray = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for zerocross-density array (1)\n");
        return(MEMORY_ERROR);
    }
    if(dz->iarray_cnt > 0) {
        if((dz->iparray = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for reordering array (1)\n");
            return(MEMORY_ERROR);
        }
    }
    if(dz->larray_cnt > 0) {
        if((dz->lparray = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for sample times (1)\n");
            return(MEMORY_ERROR);
        }
    }
    if(dz->process == DISTBRIGHT && dz->mode == 0)
        arraysize = (int)ceil(dz->duration/dz->minstep) + SAFETY;                   //  In this mode, tail segs are further subdivided
    else
        arraysize = dz->itemcnt + SAFETY;

    if(dz->process == DISTBRIGHT) {
        if((dz->iparray[0] = (int *)malloc(arraysize * sizeof(int)))==NULL) {       //  Array for Head/Tail flags for labelling segments
            sprintf(errstr,"INSUFFICIENT MEMORY for Head/Tail flags for labelling segments\n");
            return(MEMORY_ERROR);
        }
        if((dz->iparray[1] = (int *)malloc(arraysize * sizeof(int)))==NULL) {       //  Array for Reordering these segments
            sprintf(errstr,"INSUFFICIENT MEMORY for reordering array\n");
            return(MEMORY_ERROR);
        }
    }
    if((dz->parray[0] = (double *)malloc(arraysize * sizeof(double)))==NULL) {      //  Array for segment times
        sprintf(errstr,"INSUFFICIENT MEMORY for time data in file %s. (2)\n",filename);
        return(MEMORY_ERROR);
    }
    if(dz->process == DISTBRIGHT) {
        if((dz->parray[1] = (double *)malloc(arraysize * sizeof(double)))==NULL) {  //  Array for zcross-density values (and possibly also for expanded time-set)
            sprintf(errstr,"INSUFFICIENT MEMORY for time data in file %s. (2)\n",filename);
            return(MEMORY_ERROR);
        }
    }
    if((dz->lparray[0] = (int *)malloc(arraysize * 2 * sizeof(int)))==NULL) {       //  Array for segment sampletimes
        sprintf(errstr,"INSUFFICIENT MEMORY for sampletime data from file %s. (2)\n",filename);
        return(MEMORY_ERROR);
    }
    time = dz->parray[0];
    rewind(fp);
    lasttime = -1.0;
    cnt = 0;
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,&dummy)) {
            if(dummy >= dz->duration) {
                if(!warned) {
                    fprintf(stdout,"WARNING: Times beyond end of sndfile (%lf) in file %s. Ignoring them.\n",dz->duration,filename);
                    fflush(stdout);
                }
                break;
            }
            *time = dummy;
            lasttime = *time;
            time++;
            cnt++;
        }
    }
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
        fflush(stdout);
    }
    if((dz->process == SEGSBKWD || dz->process == SEGSZIG) && dz->minstep <= (2.0 * SSPLICELEN * MS_TO_SECS)) {
        sprintf(errstr,"Some marked segment(s) shorter (%lf secs) than 2 splices (%lf): Cannot proceed.\n",dz->minstep,2.0 * SSPLICELEN * MS_TO_SECS);
        return DATA_ERROR;
    }
    dz->itemcnt = cnt;
    return(FINISHED);
}

/************************************ DISTMORE_PARAM_PREPROCESS ************************************/

int distmore_param_preprocess(dataptr dz)
{
    int exit_status, phase = 0, startphase = 0, halfwavesetcnt = 0;
    int n, wavsetsize = 0, maxwavsetsize = 0;
    float *ibuf = dz->sampbuf[0];
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);
    while(dz->ssampsread > 0) {
        for(n = 0; n < dz->ssampsread;n++) {
            if(ibuf[n] > 0.0) {
                if(startphase == 0)                     //  If startphase not set (at outset), set startphase to equal phase found
                    startphase = 1;
                phase = 1;
            } else if(ibuf[n] < 0.0) {
                if(startphase == 0)
                    startphase = -1;
                phase = -1;
            }
            if(phase != startphase) {                               //  If phase changes
                halfwavesetcnt++;                                   //  Count half-wavesets
                if(halfwavesetcnt > 1) {                            //  Once we have 2 half-wavesets, we have a waveset
                    maxwavsetsize = max(maxwavsetsize,wavsetsize);  //  Save largest
                    wavsetsize = 0;                                 //  Reset counts of wavsetsize and halfwavesets 
                    halfwavesetcnt = 0;
                }
                startphase = phase;                                 //  Starting phase reset to current phase
            }
            wavsetsize++;                                           //  Count size of current waveset
        }
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
    }
    if(maxwavsetsize > dz->maxxstep) {
        dz->maxxstep = maxwavsetsize;
        if((exit_status = recreate_distmore_sndbufs(dz))<0)
            return(exit_status);
    }
    sndseekEx(dz->ifd[0],0,0);
    return FINISHED;
}

/************************************ CHECK_DISTMORE_PARAM_VALIDITY_AND_CONSISTENCY ************************************/

int check_distmore_param_validity_and_consistency(dataptr dz)
{
    double srate = (double)dz->infile->srate;
    switch(dz->process) {
    case(DISTDOUBLE):
        dz->iparam[0] = (int)round(pow(2.0,dz->iparam[0]));             //  Convert octs to frq multiplier
        break;
    case(DISTBRIGHT):
        dz->iparam[0] = (int)round(dz->param[0] * MS_TO_SECS * srate);  //  Splicelen in samples
        break;
    case(SEGSZIG):
        if(dz->param[ZMINSIZ] > 0 && dz->param[ZMINSIZ] < (SSPLICELEN * 2) + 1) {
            sprintf(errstr,"MINIMUM ZIG SIZE CANNOT BE LESS THAN %d IF IT IS NOT ZERO\n",(SSPLICELEN * 2) + 1);
            return DATA_ERROR;
        }
        dz->iparam[ZMINSIZ] = (int)round(dz->param[ZMINSIZ] * MS_TO_SECS * srate);  //  Min size of zigs in samples
        break;
    }
    return FINISHED;
}

/************************************ DISTBRIGHT ************************************/

int distbright(dataptr dz)
{
    int exit_status, n, m, done = 0;
    int lastphase = 0, phase = 0;
    int startindex, zcro = 0, sttindex = 0;
    double srate = (double)dz->infile->srate;
    double *time = dz->parray[0];
    double *zcros = dz->parray[1];
    int firsttime, nexttime, samptime;
    int *ishead = dz->iparray[0], is_head;
    int *order = dz->iparray[1];            //  Holds reordering sequence
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *buf1, *buf2, *bufa = dz->sampbuf[2], *bufb = dz->sampbuf[3], *tempbuf;
    int increases = 1, temp;
    int k, j, z, obufpos, stt, sttsamp, endsamp, sampcnt1, sampcnt2;
    int *sttend = dz->lparray[0];
    double valn, valm;
    int splicelen = dz->iparam[0];
    samptime = 0;
    startindex = 0;
    is_head = 1;
    for(n=0;n<dz->itemcnt;n++) {                            //  Initially mark Head/Tail sequence
        ishead[n] = is_head;
        is_head = !is_head;
    }
    time[dz->itemcnt] = dz->duration;                       //  Complete times array with the duration of the src
    if(dz->mode == 0) {
        if((exit_status = generate_tail_segments(dz)) < 0)  //  For mode 0, Head/Tail sequence re-marked here, as tail subdivisions added 
            return exit_status;                             //  & dz->itemcnt increased
    }
    for(n=0,m=0;n < dz->itemcnt;n++,m+=2) {
        sttend[m] = (int)round(time[n] * srate);                //  starttime of segment
        if(flteq(time[n+1],dz->duration))
            sttend[m+1] = dz->insams[0];
        else {                                                  //  endtime of segment
            sttend[m+1] = (int)round(time[n+1] * srate);        //  including splice fade-down time
            sttend[m+1] += splicelen;
            sttend[m+1] = min(sttend[m+1],dz->insams[0]);
        }
    }
    firsttime = sttend[sttindex];
    nexttime  = sttend[sttindex+2];
    is_head = 1;
    if((exit_status = read_samps(ibuf,dz))< 0) {
        sprintf(errstr,"Failed to read data from input file.\n");
        return(SYSTEM_ERROR);
    }
    while(dz->ssampsread > 0) {
        n = 0;
        while(n < dz->ssampsread) {
            if(samptime >= firsttime) {                 //  Skip over any sound before the first timemark in marklist
                if(samptime >= nexttime) {
                    zcros[startindex] = (double)zcro/(time[sttindex+2] - time[sttindex]);
                    startindex++;
                    sttindex += 2;
                    zcro = 0;
                    lastphase = 0;
                    if(startindex >= dz->itemcnt) { //  NB there is an extra time (src duration) at end of times data
                        done = 1;
                        break;
                    }
                    is_head  = ishead[startindex];
                    nexttime = sttend[sttindex+2];
                }
                if(ibuf[n] > 0.0) {
                    phase = 1;
                    if(lastphase == 0)
                        lastphase = phase;
                } else if(ibuf[n] < 0.0) {
                    phase = -1;
                    if(lastphase == 0)
                        lastphase = phase;
                }
                if(phase != lastphase) {
                    zcro++;
                    lastphase = phase;
                }
            }
            samptime++;
            n++;
        }
        if(done)
            break;
        if((exit_status = read_samps(ibuf,dz))< 0) {
            sprintf(errstr,"Failed to read data from input file.\n");
            return(SYSTEM_ERROR);
        }
    }
    if(dz->mode == 2) {                         //  In mode 2, join together H+T segs as single items
        for(m=1,n=2;n<dz->itemcnt;m++,n+=2)     //                        itemcnt                             itemcnt   
            time[m] = time[n];                  //  H T     H T     H T     END     ~OR~    H T     H T      H END
        dz->itemcnt = m;                        //  h       h       h                       h       h        h
    }                                           

    // DO SORT AND THEN RECONSTRUCT

    dz->itemcnt = startindex;                   //  Reset count to actual count of zcros data items
    if(dz->vflag[0])
        increases = 0;
    for(n=0;n<dz->itemcnt;n++)
        order[n] = n;
    for(n=0;n < dz->itemcnt-1;n++) {            //  Do the data/order sort
        valn = zcros[n];
        for(m=n+1;m < dz->itemcnt;m++) {
            valm = zcros[m];                    //  If vals out of order
            if((increases && valn > valm) || (!increases && valn < valm)) {
                zcros[n] = valm;                //  Swap vals in array
                zcros[m] = valn;    
                valn = zcros[n];                //  Reset valn to its new value

                temp = order[n];                //  Swap order of items
                order[n] = order[m];
                order[m] = temp;
            }
        }
    }
    buf1 = bufa;
    buf2 = bufb;
    obufpos = 0;

    //  Copy initial segment into buffer 1

    stt = order[0] * 2;                 //  Start and endtimes of segs are in double-length int-array
    sttsamp = sttend[stt];
    endsamp = sttend[stt+1];
    sampcnt1 = endsamp - sttsamp;
    sndseekEx(dz->ifd[0],sttsamp,0);
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);
    memcpy((char *)buf1,(char *)ibuf,sampcnt1 * sizeof(float));     //  Copy src chunk to buf1
    for(k = 0; k < splicelen; k++)
        buf1[k] = (float)(buf1[k] * ((double)k/(double)splicelen)); //  DO start and end splices
    for(k = 0, j = sampcnt1-1; k < splicelen; k++,j--)
        buf1[j] = (float)(buf1[j] * ((double)k/(double)splicelen));

    //  Write the upsplice portion of 1st segment to obuf
    
    for(m=0; m < splicelen; m++) {
        obuf[obufpos++] = buf1[m];
        //PROBABLY REDUNDANT
        if(obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;
        }
    }
    //  Copy next segment into buffer 2

    for(n=1;n < dz->itemcnt;n++) {
        stt = order[n] * 2;
        sttsamp = sttend[stt];
        endsamp = sttend[stt+1];
        sampcnt2 = endsamp - sttsamp;
        sndseekEx(dz->ifd[0],sttsamp,0);
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        memcpy((char *)buf2,(char *)ibuf,sampcnt2 * sizeof(float));
        for(k = 0; k < splicelen; k++)
            buf2[k] = (float)(buf2[k] * ((double)k/(double)splicelen));
        for(k = 0, j = sampcnt2-1; k < splicelen; k++,j--)
            buf2[j] = (float)(buf2[j] * ((double)k/(double)splicelen));

        //  Write buf1 apart from the splice-brackets, to obuf

        for(m=splicelen; m < sampcnt1 - splicelen;m++) {
            obuf[obufpos++] = buf1[m];
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }

        //  Write the cross-splice

        for(z = 0;z < splicelen; z++,m++) {
            obuf[obufpos++] = (float)(buf1[m] + buf2[z]);
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }

        //  Swap the buffers, and buffer-data

        tempbuf = buf1;
        buf1    = buf2;
        buf2    = tempbuf;
        sampcnt1 = sampcnt2;
    }

    //  Write all of the remaining segment

    for(m=splicelen; m < sampcnt1; m++) {
        obuf[obufpos++] = buf1[m];
        if(obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;
        }
    }

    //  Flush the output buffer

    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }   
    return FINISHED;
}

/************************************ GENERATE_TAIL_SEGMENTS ************************************
 *
 *  Slice tail sections into segments approx same size as bracketing head segments
 */

int generate_tail_segments(dataptr dz)
{
    int n, j, k, tailsegcnt;
    double *time   = dz->parray[0];
    double *nutime = dz->parray[1];
    int *ishead   = dz->iparray[0];
    double stthsize, endhsize, meanhsize, tailsize, randoffset = 0.0;
    ishead[0] = 1;
    ishead[1] = 0;
    nutime[0] = time[0];                                    //  H T           H T           H T     
    nutime[1] = time[1];                                    // n0 n1         n2 n3         n4 n5
    j = 2;                                                  //  |-|-----------|-|-----------|-|---------
    stthsize = time[1] - time[0];                           //      tailsize
    for(n=2;n < dz->itemcnt; n+=2) { // Step to next HEAD   //stthsize      endhsize    
        tailsize = time[n] - time[n-1];                     //
        endhsize = time[n+1] - time[n];                     
        meanhsize = (stthsize + endhsize)/2.0;              // New times(j), at n = 2, if tailsegcnt = 4
        tailsegcnt = (int)round(tailsize/meanhsize);        // n0 n1         n2 n3         n4 n5
        meanhsize = tailsize/(double)tailsegcnt;            //  |-|-----------|-|-----------|-|---------
        for(k=1;k < tailsegcnt;k++) {                       // j0 j1 |  |  |        
            nutime[j] = time[n-1] + (meanhsize * k);        //       j2 j3 j4 (3 new times inserted)
                                                            //  H T  T  T  T         
            randoffset = ((drand48() * 2.0) - 1.0)/3.0;     //      <-> <-> <->
            randoffset *= DISTMORERAND;                     //       j2  j3  j4
            randoffset *= meanhsize;                        //  Rand in range max of (-1/3 to + 1/3 * meanhsize)
            nutime[j] += randoffset;                        //      
                                                            //
            ishead[j] = 0;                                  //  
            j++;                                            //  Ending at j5 = n2
        }                                                   // n0 n1         n2 n3         n4 n5
        nutime[j] = time[n];                                //  |-|-----------|-|-----------|-|---------
        ishead[j++] = 1;                                    // j0 j1 j2 j3 j4 | |
        nutime[j] = time[n+1];                              //               j5 j6
        ishead[j++] = 0;                                    //  H T  T  T  T  H T
        stthsize = endhsize;
    }
    if(n == dz->itemcnt) {                                  //  IF end of file is end of a tail
        tailsize = time[n] - time[n-1];                     //  H T     H T         End
        meanhsize = stthsize;                               //  | |     | |          |
        tailsegcnt = (int)round(tailsize/meanhsize);        //           n-1         n  
        meanhsize = tailsize/(double)tailsegcnt;            //         endhsize      |
        for(k=1;k < tailsegcnt;k++) {                       //        j22 j23  |  |  | 
            nutime[j] = time[n-1] + (meanhsize * k);        //               j24 j25 |
                                                            //               <-> <-> |  
            randoffset = ((drand48() * 2.0) - 1.0)/3.0;     //                       j26
            randoffset *= DISTMORERAND;                     //  ....... H  T  T  T   T
            randoffset *= meanhsize;                    
            nutime[j] += randoffset;
                                                            
            ishead[j] = 0;
            j++;
        }
        nutime[j] = time[n];
    }
    dz->itemcnt = j;
    memcpy((char *)time,(char *)nutime,(dz->itemcnt+1) * sizeof(double));
    memset((char *)nutime,0,(dz->itemcnt+1) * sizeof(double));  //  Clear, as array is reused for zcros-density vals
    return FINISHED;
}

/**************************** CREATE_DISTMORE_SNDBUFS ****************************/

int create_distmore_sndbufs(dataptr dz)
{
    int n;
    unsigned int buffersize = 0;
    switch(dz->process) {
    case(DISTBRIGHT):
        dz->bufcnt = 4;
        buffersize = (unsigned int)round((dz->maxstep * 2) * (double)dz->infile->srate);
        break;
    case(DISTDOUBLE):
        dz->bufcnt = 3;
        dz->maxxstep = F_SECSIZE * 64;
        buffersize = dz->maxxstep * 2;
        break;
    case(SEGSBKWD):
        dz->bufcnt = 4;
        if(dz->mode < 2)
            buffersize = (unsigned int)round((dz->maxstep * 2) * (double)dz->infile->srate);                //  largest seg + overflow
        else 
            buffersize = (unsigned int)round((dz->maxstep * (dz->mode+1)) * (double)dz->infile->srate); //  largest N segs + overflow
        break;
    case(SEGSZIG):
        if(dz->mode == 0) {
            dz->bufcnt = 5;
            buffersize = (unsigned int)round((dz->maxstep * 2) * (double)dz->infile->srate);                //  largest seg + overflow
        } else {
            dz->bufcnt = 3;
            buffersize = (unsigned int)(dz->insams[0] + SAFETY);                                            //  infile size + overflow
        }
        break;
    }
    if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    buffersize = (buffersize/F_SECSIZE + 1) * F_SECSIZE;
    dz->buflen = buffersize;
    if((dz->buflen  < 0) || (dz->buflen * dz->bufcnt < 0)) {  
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->bigfbuf = (float*) malloc(dz->buflen * dz->bufcnt * sizeof(float)))==NULL) {  
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigfbuf;
    for(n = 1;n < dz->bufcnt;n++)
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigfbuf + (n * dz->buflen);
    dz->sampbuf[n] = dz->bigfbuf + (n * dz->buflen);

    return(FINISHED);
}

/**************************** RECREATE_DISTMORE_SNDBUFS ****************************/

int recreate_distmore_sndbufs(dataptr dz)
{
    int n;
    unsigned int buffersize = 0;
    free(dz->bigbuf);
    buffersize = dz->maxxstep * 2;
    buffersize = (buffersize/F_SECSIZE) * F_SECSIZE;
    dz->buflen = buffersize;
    if((dz->buflen  < 0) || (dz->buflen * dz->bufcnt < 0)) {  
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->bigfbuf = (float*) malloc(dz->buflen * dz->bufcnt * sizeof(float)))==NULL) {  
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigfbuf;
    for(n = 1;n < dz->bufcnt;n++)
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigfbuf + (n * dz->buflen);
    dz->sampbuf[n] = dz->bigfbuf + (n * dz->buflen);
    return(FINISHED);
}

/**************************** DISTDOUBLE ****************************/

int distdouble(dataptr dz)
{
    int exit_status;
    int obufpos = 0, wbufpos = 0, n, j, k, lastk = 0, fill, m;
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *wbuf = dz->sampbuf[2];
    int lastphase = 0, phase = 0, zerocross = 0, jj, group = dz->iparam[0];
    if((exit_status = read_samps(ibuf,dz))< 0) {
        sprintf(errstr,"Failed to read data from input file.\n");
        return(SYSTEM_ERROR);
    }
    while(dz->ssampsread > 0) {
        n = 0;
        while(n < dz->ssampsread) {
            if(ibuf[n] > 0.0) {
                phase = 1;
                if(lastphase == 0)
                    lastphase = phase;
            } else if(ibuf[n] < 0.0) {
                phase = -1;
                if(lastphase == 0)
                    lastphase = phase;
            }
            if((lastphase != 0) && (phase != lastphase)) {
                zerocross++;
                if(zerocross == 2) {
                    switch(group) {
                    case(2):
                        if(EVEN(wbufpos)) {
                            for(k=0,j=0;k < wbufpos; k+=2,j++)      //  Copy every other sample
                                wbuf[j] = wbuf[k];
                            jj = j;
                            for(k=0;k < jj; k++,j++)                //  Duplicate the copy
                                wbuf[j] = wbuf[k];
                        } else {
                            for(k=0,j=0;k < wbufpos; k+=2,j++)      //  Copy every other sample
                                wbuf[j] = wbuf[k];
                            jj = j;
                            for(k=1;k < jj; k++,j++)                //  Duplicate the copy
                                wbuf[j] = wbuf[k];
                        }
                        break;
                    default:
                        if(wbufpos > dz->iparam[0]) {
                            for(k = 0,j= 0; k < wbufpos; k+=group,j++) {
                                wbuf[j] = wbuf[k];
                                lastk = k;
                            }
                            if(lastk != wbufpos - 1)
                                wbuf[j++] = wbuf[wbufpos - 1];
                            jj = j;
                            fill = (int)round((double)wbufpos/(double)j);
                            for(m=1;m < fill;m++) {
                                for(k=0;k < jj; k++,j++)            //  Duplicate the copy
                                    wbuf[j] = wbuf[k];
                            }
                            wbufpos = j;                            //  Truncate output (if ness)
                        }
                        break;
                    }
                    for(k=0;k < wbufpos; k++) {                 //  Copy to output buffer
                        obuf[obufpos++] = wbuf[k];
                        if(obufpos >= dz->buflen) {
                            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                return(exit_status);
                            obufpos = 0;
                        }
                    }
                    memset((char *)wbuf,0,dz->buflen * sizeof(float));
                    zerocross = 0;
                    wbufpos = 0;
                }
                lastphase = phase;
            }
            wbuf[wbufpos++] = ibuf[n];
            n++;
        }
        if((exit_status = read_samps(ibuf,dz))< 0) {
            sprintf(errstr,"Failed to read data from input file.\n");
            return(SYSTEM_ERROR);
        }
    }
    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }   
    return FINISHED;
}

/************************************ SEGSBKWD ************************************/

int segsbkwd(dataptr dz)
{
    int exit_status, n, m, forwards, reversals;
    double srate = (double)dz->infile->srate;
    double *time = dz->parray[0];
    int samptime;
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *buf1, *buf2, *bufa = dz->sampbuf[2], *bufb = dz->sampbuf[3], *tempbuf;
    int modd;
    int k, j, z, obufpos, stt, sttsamp, endsamp, sampcnt1, sampcnt2;
    int *sttend = dz->lparray[0];
    int splicelen = (int)round(SSPLICELEN * MS_TO_SECS * srate);
    samptime = 0;
    time[0] = 0.0;                                          //  Stretch initial time to start of file   
    time[dz->itemcnt] = dz->duration;                       //  Complete times array with the duration of the src

    switch(dz->mode) {
    case(0):    // H->  <-T
    case(1):    // H<-  ->T
        break;  
    case(2):    //  <-- <--
                //  H T H T
        for(m=0,n=0;n<=dz->itemcnt;m++,n+=2)    //  H1 T2   H2 T2   H3 T3   end     ~OR~    H1 T1   H2 T2   H3  end
            time[m] = time[n];                  //  H1      H2      H3                      H1      H2      H3
        dz->itemcnt = m-1;
        break;
    default:
        modd = dz->mode;                        //  modulus for selecting  times to keep
        if(ODD(dz->mode))                       //  modulus is 4 for modes 3 and 4 (etc)
            modd++;
        m = 0;
        for(n=0;n<=dz->itemcnt;n++) {           //  case(3):->  <--------                   case(4):<------------   
            k = n % modd;                       //          H   T   H   T                           H   T   H   T
            if(k == 0)                          //  case(5):->  <----------------           case(6) <--------------------
                time[m++] = time[n];            //          H   T   H   T   H   T                   H   T   H   T   H   T
            if(ODD(dz->mode) && k == 1)         //  case(7):->  <------------------------   case(8):<----------------------------
                time[m++] = time[n];            //          H   T   H   T   H   T   H   T           H   T   H   T   H   T   H   T
        }
        if(time[m-1] != dz->duration)
            time[m++] = dz->duration;
        dz->itemcnt = m-1;
        break;
    }
    for(n=0,m=0;n < dz->itemcnt;n++,m+=2) {
        sttend[m] = (int)round(time[n] * srate);            //  sample-starttime of segment
        if(flteq(time[n+1],dz->duration))
            sttend[m+1] = dz->insams[0];
        else {                                              //  sample-endtime of segment
            sttend[m+1] = (int)round(time[n+1] * srate);    //  including splice fade-down time
            sttend[m+1] += splicelen;
            sttend[m+1] = min(sttend[m+1],dz->insams[0]);
        }
    }
    switch(dz->mode) {
    case(0):    // FR
        forwards = 1;
        reversals = 1;
        break;
    case(1):    // RF
        forwards = 0;
        reversals = 1;
        break;
    default:                    
        if(EVEN(dz->mode)) {    
            forwards = 0;       //  case(EVEN): RRRR.....
            reversals = 0;
        } else {
            forwards = 1;       //  case(ODD):  FRFR.....
            reversals = 1;
        }
        break;
    }

    // RECONSTRUCT

    buf1 = bufa;
    buf2 = bufb;
    obufpos = 0;

    //  Copy initial segment into buffer 1

    stt = 0;                        //  Start and endtimes of segs are in double-length int-array
    sttsamp = sttend[stt];
    endsamp = sttend[stt+1];
    sampcnt1 = endsamp - sttsamp;
    sndseekEx(dz->ifd[0],sttsamp,0);
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);
    memcpy((char *)buf1,(char *)ibuf,sampcnt1 * sizeof(float));     //  Copy src chunk to buf1
    if(!forwards)
        reversebuf(buf1,sampcnt1);
    if(reversals)
        forwards = !forwards;
    for(k = 0; k < splicelen; k++)
        buf1[k] = (float)(buf1[k] * ((double)k/(double)splicelen)); //  DO start and end splices
    for(k = 0, j = sampcnt1-1; k < splicelen; k++,j--)
        buf1[j] = (float)(buf1[j] * ((double)k/(double)splicelen));

    //  Write the upsplice portion of 1st segment to obuf
    
    for(m=0; m < splicelen; m++) {
        obuf[obufpos++] = buf1[m];
        //PROBABLY REDUNDANT
        if(obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;
        }
    }
    //  Copy next segment into buffer 2

    for(n=1;n < dz->itemcnt;n++) {
        stt += 2;
        sttsamp = sttend[stt];
        endsamp = sttend[stt+1];
        sampcnt2 = endsamp - sttsamp;
        sndseekEx(dz->ifd[0],sttsamp,0);
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        memcpy((char *)buf2,(char *)ibuf,sampcnt2 * sizeof(float));
        if(!forwards)
            reversebuf(buf2,sampcnt2);
        if(reversals)
            forwards = !forwards;
        for(k = 0; k < splicelen; k++)
            buf2[k] = (float)(buf2[k] * ((double)k/(double)splicelen));
        for(k = 0, j = sampcnt2-1; k < splicelen; k++,j--)
            buf2[j] = (float)(buf2[j] * ((double)k/(double)splicelen));

        //  Write buf1 apart from the splice-brackets, to obuf

        for(m=splicelen; m < sampcnt1 - splicelen;m++) {
            obuf[obufpos++] = buf1[m];
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }

        //  Write the cross-splice

        for(z = 0;z < splicelen; z++,m++) {
            obuf[obufpos++] = (float)(buf1[m] + buf2[z]);
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }

        //  Swap the buffers, and buffer-data

        tempbuf = buf1;
        buf1    = buf2;
        buf2    = tempbuf;
        sampcnt1 = sampcnt2;
    }

    //  Write all of the remaining segment

    for(m=splicelen; m < sampcnt1; m++) {
        obuf[obufpos++] = buf1[m];
        if(obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;
        }
    }

    //  Flush the output buffer

    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }   
    return FINISHED;
}

/************************************ REVERSEBUF ************************************/

void reversebuf(float *buf,int sampcnt)
{
    int n,m,k;
    float temp;
    k = sampcnt/2;                                  //  ODD LEN(eg5) k = 5/2 -> 2       EVEN LEN(eg6) k = 6/2 -> 3
    for(n = 0,m = sampcnt-1;n < k; n++, m--) {      //  len = 5 k = 2   0 1 2 3 4       len = 6 k = 3   0 1 2 3 4 5 
        temp    = buf[n];                           //                  |_______|                       |_________|
        buf[n] = buf[m];                            //                    |___|                           |_____|   
        buf[m] = temp;                              //              n = 0 1                                 |_|
    }                                               //                                              n = 0 1 2
}

/************************************ REVERSEBUF2 ************************************/

void reversebuf2(float *buf1,float *buf2,int sampcnt)
{
    int n,m;
    for(n = 0,m = sampcnt-1;n < sampcnt; n++, m--)
        buf2[m] = buf1[n];
}

/************************************ SEGSZIG ************************************/

int segszig(dataptr dz)
{
    int exit_status, n, m, repets, done;
    double srate = (double)dz->infile->srate, stepratio = 1.0;
    double *time=NULL, thistime = 0.0;
    int step, cutlen;
    float *ibuf = dz->sampbuf[0], *obuf=NULL, *buf1=NULL, *buf2=NULL, *bufr=NULL;
    int k, j, z, obufpos = 0, stt, sttsamp = 0, endsamp = 0, sampcnt1, sampcnt2;
    int *sttend = NULL;
    int splicelen = (int)round(SSPLICELEN * MS_TO_SECS * srate);
    if(dz->mode == 0) {
        bufr = dz->sampbuf[1];
        buf1 = dz->sampbuf[2];
        buf2 = dz->sampbuf[3];
        obuf = dz->sampbuf[4];
        time = dz->parray[0];
        time[0] = 0.0;                                          //  Stretch initial time to start of file   
        time[dz->itemcnt] = dz->duration;                       //  Complete times array with the duration of the src
        sttend = dz->lparray[0];
        for(n=0,m=0;n < dz->itemcnt;n++,m+=2) {
            sttend[m] = (int)round(time[n] * srate);            //  sample-starttime of segment
            if(sttend[m] > splicelen)
                sttend[m] -= splicelen;
            if(flteq(time[n+1],dz->duration))
                sttend[m+1] = dz->insams[0];
            else {                                              //  sample-endtime of segment
                sttend[m+1] = (int)round(time[n+1] * srate);    //  including splice fade-down time
                sttend[m+1] += splicelen;
                sttend[m+1] = min(sttend[m+1],dz->insams[0]);
            }
        }
    } else {
        bufr = dz->sampbuf[1];
        obuf = dz->sampbuf[2];
    }
    switch(dz->mode) {
    case(0):

        done = 0;
        n = 0;
        stt = 0;                        //  Start and endtimes of segs are in double-length int-array
        sttsamp = sttend[stt];
        endsamp = sttend[stt+1];
        sampcnt1 = endsamp - sttsamp;
        sndseekEx(dz->ifd[0],sttsamp,0);
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);


        //  Read 1st Head (or whole file) and splice ends
        
        memcpy((char *)buf1,(char *)ibuf,sampcnt1 * sizeof(float));     //  Copy src chunk to buf1
        for(k = 0; k < splicelen; k++)
            buf1[k] = (float)(buf1[k] * ((double)k/(double)splicelen)); //  DO start and end splices
        for(k = 0, j = sampcnt1-1; k < splicelen; k++,j--)
            buf1[j] = (float)(buf1[j] * ((double)k/(double)splicelen));

    //  Write the upsplice portion of 1st segment to obuf
    
        for(m=0; m < splicelen; m++) {
            obuf[obufpos++] = buf1[m];
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }
        n++;
        while(n < dz->itemcnt) {
            thistime = time[n];
            if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
                return exit_status;
            stt += 2;
            sttsamp = sttend[stt];
            endsamp = sttend[stt+1];
            sampcnt2 = endsamp - sttsamp;
            sndseekEx(dz->ifd[0],sttsamp,0);
            if(dz->param[ZFRAC] < 1.0)
                sampcnt2 = (int)ceil((double)sampcnt2 * dz->param[ZFRAC]);

            if(dz->vflag[0])
                stepratio = 1.0;
            step = 0;

            //  If the zigzag is to repeat (dz->iparam[ZREPETS] > 1) then it must shrink
            //  If shrink MINSIZ IS set as 0, this flags NO  shrinkage
            //  If shrink MINSIZ <= size of segment, shrinkage is impossible

            if (dz->iparam[ZREPETS] > 1 && (dz->iparam[ZMINSIZ] > 0) && (dz->iparam[ZMINSIZ] <= sampcnt2)) {   //RWD added braces, hope this is what is intended!
                if(dz->vflag[0]) {
                    stepratio = 1.0/(pow(((double)sampcnt2/(double)dz->iparam[ZMINSIZ]),(1.0/((double)dz->iparam[ZREPETS] * 2.0))));
                    step = 1;
                } else {
                    step = (int)floor((double)(sampcnt2 - dz->iparam[ZMINSIZ])/((double)dz->iparam[ZREPETS] * 2.0));
                }
            }
            //  Copy Tail segment into buffer 2 and splice ends
            
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            memcpy((char *)buf2,(char *)ibuf,sampcnt2 * sizeof(float));
            for(k = 0; k < splicelen; k++)
                buf2[k] = (float)(buf2[k] * ((double)k/(double)splicelen));
            for(k = 0, j = sampcnt2-1; k < splicelen; k++,j--)
                buf2[j] = (float)(buf2[j] * ((double)k/(double)splicelen));

            //  Copy Reversed Tailsegment into buffer r 
            
            reversebuf2(buf2,bufr,sampcnt2);

            //  Write buf1 (Head) apart from the splice-brackets, to obuf

            for(m=splicelen; m < sampcnt1 - splicelen;m++) {
                obuf[obufpos++] = buf1[m];
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }

            //  Write the cross-splice from Head to Tail

            for(z = 0;z < splicelen; z++,m++) {
                obuf[obufpos++] = (float)(buf1[m] + buf2[z]);
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
            repets = 0;
            while(repets < dz->iparam[ZREPETS]) {
            
                //  Write buf2 apart from the splice-brackets, to obuf

                for(m=splicelen; m < sampcnt2 - splicelen;m++) {
                    obuf[obufpos++] = buf2[m];
                    if(obufpos >= dz->buflen) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        obufpos = 0;
                    }
                }

                    //  Write the cross-splice to the reversed version

                if(step > 0) {                      
                    reversebuf2(buf2,bufr,sampcnt2);//  Get the reversed version from the possibly shortened normal version
                    if(dz->vflag[0])
                        cutlen = (int)ceil((double)sampcnt2 * stepratio);
                    else
                        cutlen = sampcnt2 - step;       //  Cut short the reversed version
                    cutend_and_splicend(bufr,cutlen,sampcnt2,splicelen);
                    sampcnt2 = cutlen;
                }
                for(z = 0;z < splicelen; z++,m++) {
                    obuf[obufpos++] = (float)(buf2[m] + bufr[z]);
                    if(obufpos >= dz->buflen) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        obufpos = 0;
                    }
                }
                
                //  Write bufr (reversed version) apart from the splice-brackets, to obuf

                for(m=splicelen; m < sampcnt2 - splicelen;m++) {
                    obuf[obufpos++] = bufr[m];
                    if(obufpos >= dz->buflen) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        obufpos = 0;
                    }
                }
                repets++;
                if(repets < dz->iparam[ZREPETS]) {
                //  Write the cross-splice FROM the reversed version to the forward version
                    if(step > 0) {
                        reversebuf2(bufr,buf2,sampcnt2);    //  Get the forwards version from the shortened reversed version
                        if(dz->vflag[0])
                            cutlen = (int)ceil((double)sampcnt2 * stepratio);
                        else
                            cutlen = sampcnt2 - step;       //  Cut it short
                        cutend_and_splicend(buf2,cutlen,sampcnt2,splicelen);
                        sampcnt2 = cutlen;
                    }
                    for(z = 0;z < splicelen; z++,m++) {
                        obuf[obufpos++] = (float)(bufr[m] + buf2[z]);
                        if(obufpos >= dz->buflen) {
                            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                return(exit_status);
                            obufpos = 0;
                        }
                    }
                }
            }
            n++;
            if(n >= dz->itemcnt) {
        
                //  If no more Heads: Write remainder of reversed version to output and quit

                while(m < sampcnt2) {
                    obuf[obufpos++] = bufr[m++];
                    if(obufpos >= dz->buflen) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        obufpos = 0;
                    }
                }
                done = 1;
                break;
            }
            stt += 2;
            sttsamp = sttend[stt];
            endsamp = sttend[stt+1];
            sampcnt1 = endsamp - sttsamp;
            sndseekEx(dz->ifd[0],sttsamp,0);

            //  Read Next Head, and splice ends

            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            memcpy((char *)buf1,(char *)ibuf,sampcnt1 * sizeof(float));
            for(k = 0; k < splicelen; k++)
                buf1[k] = (float)(buf1[k] * ((double)k/(double)splicelen));
            for(k = 0, j = sampcnt1-1; k < splicelen; k++,j--)
                buf1[j] = (float)(buf1[j] * ((double)k/(double)splicelen));

            //  Write the cross-splice FROM the reversed version to the Head

            for(z = 0;z < splicelen; z++,m++) {
                obuf[obufpos++] = (float)(bufr[m] + buf1[z]);
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
            n++;
        }
        if(!done) {             //  Can only be "done" if ended on a Tail
                
            //  If not done, Write remainder of (FINAL) head to output

            for(m=splicelen; m < sampcnt1;m++) {
                obuf[obufpos++] = buf1[m];
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
        }
        break;

    case(1):
    case(2):
        sampcnt1 = dz->insams[0];
        if(dz->param[ZFRAC] < 1.0)
            sampcnt1 = (int)round(sampcnt1 * dz->param[ZFRAC]);
        step = 0;
        if(dz->mode == 2 || dz->vflag[0])
            stepratio = 1.0;
        if (dz->iparam[ZREPETS] > 1 && (dz->iparam[ZMINSIZ] > 0) && (dz->iparam[ZMINSIZ] <= sampcnt1)) {
            if(dz->mode ==1) {
                if(dz->vflag[0]) {
                    stepratio = 1.0/(pow(((double)sampcnt1/(double)dz->iparam[ZMINSIZ]),(1.0/((double)dz->iparam[ZREPETS] * 2.0))));
                    step = 1;
                } else
                    step = (int)floor((double)(sampcnt1 - dz->iparam[ZMINSIZ])/((double)dz->iparam[ZREPETS] * 2.0));
            } else {
                stepratio = getstepratio(dz);
                step = 1;
            }
        }

        //  Write buf1 (forwards) apart from the endsplice-len, to obuf

        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);

        if(dz->param[ZFRAC] < 1.0) {
            for(k = 0,j = sampcnt1 - 1;k < splicelen; k++,j--)
                ibuf[j] = (float)(ibuf[j] * ((double)k/(double)splicelen));
        }
        for(m=0; m < sampcnt1 - splicelen;m++) {
            obuf[obufpos++] = ibuf[m];
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }
        //  Copy segment (reversing it) into buffer r 
        
        reversebuf2(ibuf,bufr,sampcnt1);

        if(step > 0) {
            if(dz->mode == 2 || dz->vflag[0]) {
                cutlen = (int)ceil((double)sampcnt1 * stepratio);
                cutlen += splicelen;
                if(cutlen > dz->insams[0])
                    cutlen = dz->insams[0];
            } else
                cutlen = sampcnt1 - step;                           //  Cut short the reversed version if ness
            cutend_and_splicend(bufr,cutlen,sampcnt1,splicelen);    //  and splice the end
            sampcnt1 = cutlen;
        }
        //  Write the cross-splice from Forwards to Reverse

        for(z = 0;z < splicelen; z++,m++) {
            obuf[obufpos++] = (float)(ibuf[m] + bufr[z]);
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }

        //  Write the body of the Reverse file

        for(m=splicelen; m < sampcnt1 - splicelen;m++) {
            obuf[obufpos++] = bufr[m];
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }
        repets = 1;
        while(repets < dz->iparam[ZREPETS]) {
        //  Copy reversed segment (possibly shortened) (righting it) into buffer 1 
        
            reversebuf2(bufr,ibuf,sampcnt1);

            if(step > 0) {                      
                if(dz->mode == 2 || dz->vflag[0]) {
                    sampcnt1 -= splicelen;
                    cutlen = (int)ceil((double)sampcnt1 * stepratio);
                    cutlen += splicelen;
                    if(cutlen > dz->insams[0])
                        cutlen = dz->insams[0];
                } else
                    cutlen = sampcnt1 - step;       //  Cut short the righted version
                cutend_and_splicend(ibuf,cutlen,sampcnt1,splicelen);
                sampcnt1 = cutlen;
            }
                //  Write the cross-splice to the righted version

            for(z = 0;z < splicelen; z++,m++) {
                obuf[obufpos++] = (float)(bufr[m] + ibuf[z]);
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
            //  Write the body of the Righted file

            for(m=splicelen; m < sampcnt1 - splicelen;m++) {
                obuf[obufpos++] = ibuf[m];
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
            //  Copy righted segment (possibly shortened) (reversing it) into buffer r 
        
            reversebuf2(ibuf,bufr,sampcnt1);

            if(step > 0) {                      
                if(dz->mode == 2 || dz->vflag[0]) {
                    sampcnt1 -= splicelen;
                    cutlen = (int)ceil((double)sampcnt1 * stepratio);
                    cutlen += splicelen;
                    if(cutlen > dz->insams[0])
                        cutlen = dz->insams[0];
                } else
                    cutlen = sampcnt1 - step;       //  Cut short the reversed version
                cutend_and_splicend(bufr,cutlen,sampcnt1,splicelen);
                sampcnt1 = cutlen;
            }
            
                //  Write the cross-splice to the reversed version

            for(z = 0;z < splicelen; z++,m++) {
                obuf[obufpos++] = (float)(ibuf[m] + bufr[z]);
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
            //  Write the body of the Reversed file

            for(m=splicelen; m < sampcnt1 - splicelen;m++) {
                obuf[obufpos++] = bufr[m];
                if(obufpos >= dz->buflen) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufpos = 0;
                }
            }
            repets++;
        }

        //  Write the remainder of the last reversed file

        while(m < sampcnt1) {
            obuf[obufpos++] = bufr[m++];
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }
        break;
    }

    //  Flush the output buffer

    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }   
    return FINISHED;
}

/************************************ CUTEND_AND_SPLICEND ************************************/

void cutend_and_splicend(float *buf,int cutlen,int sampcnt,int splicelen)
{
    int k, j;
    for(k = 0,j=cutlen-1;k < splicelen;k++,j--)
        buf[j] = (float)(buf[j] * ((double)k/(double)splicelen));
    for(j=cutlen;j < sampcnt;j++)
        buf[j] = 0.0;
}

/******************************** GETSTEPRATIO **********************
 *
 *      Start length  A: endlen B
 *      If k = 2 * repets (counts every zig and zag), and "r" = stepratio
 *      Outsamps = A + (A*r) + (A*r*r) + (A*r*r*r) ....+ B
 *                   0    1    2         (k-1)
 *               = Ar + Ar + Ar +  ....Ar       + B
 */

double getstepratio(dataptr dz)
{
    int n, k = dz->iparam[ZREPETS] * 2, increasing, decreasing;
    double splicedur = SSPLICELEN * MS_TO_SECS;
    double r, outlen, outlenbas = dz->param[ZMINSIZ] * MS_TO_SECS;
    double miss, lastmiss, flterr = FLTERR, duration;
    duration = dz->duration;
    duration *= dz->param[ZFRAC];
    while(duration + (outlenbas * (k-1)) >= dz->param[ZDUR]) {      //  If required output dur too SHORT for no of repets
        if(k == 2) {                                                    //  reduce number of repets 
            r = duration/(dz->param[ZDUR] + splicedur);
            dz->iparam[ZREPETS] = k/2;
            return r;
        }
        k -= 2;
    }
    outlen = -1;
    while(outlen <= dz->param[ZDUR]) {                  //  If required output dur too long for no of repets
        outlen = outlenbas;                             //  increase number of repets       
        for(n = 0; n < k-1; n++)
            outlen += duration;
        if(outlen < dz->param[ZDUR] * 2.0)              //  i.e. start with an overshoot estimate, to shrink from
            k += 2;
    }
    r = pow((outlenbas/duration),(1.0/(double)(k-1)));
    outlen = -1.0;
    lastmiss = HUGE;
    increasing = 0;
    decreasing = 0;
    //  Find the number of repetitions which gives a ratio approaching closest to min value
    for(;;) {
        r = pow((outlenbas/duration),(1.0/(double)(k-1)));
        outlen = outlenbas;
        for(n = 0; n < k-1; n++)                        //                        2       3           (k-1)
            outlen += duration * pow(r,n);          //  dur(1) + dur*r + dur*r + dur*r .... +dur*r
        miss = fabs(outlen - dz->param[ZDUR]);
        if(miss < lastmiss) {
            if(decreasing)
                break;
            k++;
            increasing = 1;
        } else {
            if(increasing)
                break;
            k--;
            decreasing = 1;
        }
        lastmiss = miss;
    }
    k--;
    r = pow((outlenbas/duration),(1.0/(double)(k-1)));
    if(outlen > dz->param[ZDUR])
        increasing = 0;
    else
        increasing = 1;
    //  Hone ratio to give exaxct output duration
    while(!flteq(outlen,dz->param[ZDUR])) {
        if(outlen > dz->param[ZDUR]) {
            if(increasing)                  //  i.e. outlen increasing but now too big
                flterr /= 2.0;              //  Start to step down, by smaller steps
            r -= flterr;
            increasing = 0;
        } else {
            if(!increasing)                 //  i.e. outlen decreasing but now too small
                flterr /= 2.0;              //  Start to step up, by smaller steps
            r += flterr;
            increasing = 1;
        }
        outlen = outlenbas;
        for(n = 0; n < k-1; n++)                        //                        2       3           (k-1)
            outlen += duration * pow(r,n);          //  dur(1) + dur*r + dur*r + dur*r .... +dur*r
    }
    dz->iparam[ZREPETS] = k/2;
    return r;
}
