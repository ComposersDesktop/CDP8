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

//  HEREH
//  Might want to add option to amplify the verge attacks

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

#define VRG_SRCHWINDOW 2.0  //  2mS default half search-window (looing in both directions for peak near given time)
#define VRG_BOOSTEDGE  16   //  number of samps over which boosted seg rises in level

#define bigverge total_windows

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int setup_verges_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_verges_param_ranges_and_defaults(dataptr dz);
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
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int create_verges_sndbufs(dataptr dz);
static int handle_the_special_data(char *str,dataptr dz);
static int verges(dataptr dz);
static int check_verge_param_validity_and_consistency(dataptr dz);
static int write_verge(int *ibufpos,int *obufpos,int vergsamps,dataptr dz);
//static double get_total(int k,double initial_incr,double exp);
//static int find_peak(int *ibufpos,dataptr dz);
static int find_contraction_sampcount(int vsamps,int *sampcnt,double initial_incr,double exp,dataptr dz);
static int boost_verge(int ibufpos,int vergsamps,dataptr dz);
static int boost_verge2(int ibufpos,int vergsamps,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
    int n;
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
        dz->maxmode = 0;
        // setup_particular_application =
        if((exit_status = setup_verges_application(dz))<0) {
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
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_verges_param_ranges_and_defaults(dz))<0) {
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
//  check_param_validity_and_consistency()  redundant
    if((exit_status = check_verge_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

    //param_preprocess() redundant
    dz->bufcnt = 3;
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


    is_launched = TRUE;
//  create_sndbufs() =
    if((exit_status = create_verges_sndbufs(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

    //spec_process_file =
    if((exit_status = verges(dz))<0) {
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

/************************* SETUP_VERGES_APPLICATION *******************/

int setup_verges_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,VERGEDATA,0,0,""))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"ted",3,"DDD","nbs",3,0,"000"))<0)
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
        } else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
            sprintf(errstr,"Failed to copy file parsing information\n");
            return(PROGRAM_ERROR);
        }
        free(infile_info);
    }
    return(FINISHED);
}

/************************* SETUP_VERGES_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_verges_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[VRG_TRNSP] = -24.0;
    ap->hi[VRG_TRNSP] =  24.0;
    ap->default_val[VRG_TRNSP] = VRG_DFLT_TRNSP;
    ap->lo[VRG_CURVE] = 1.0;
    ap->hi[VRG_CURVE] = 8.0;
    ap->default_val[VRG_CURVE] = VRG_DFLT_CURVE;
    ap->lo[VRG_DUR] = 20;
    ap->hi[VRG_DUR] = 1000;
    ap->default_val[VRG_DUR] = VRG_DFLT_DUR;
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
            if((exit_status = setup_verges_application(dz))<0)
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
    usage2("verges");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"verges"))          dz->process = VERGES;
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
    if(!strcmp(str,"verges")) {
        fprintf(stderr,
        "USAGE:\n"
        "verges verges inf outf times [-ttransp] [-eexp] [-ddur] [-n] [-b|-s]\n"
        "\n"
        "Play source, with specified brief moments glissing up or down.\n"
        "\n"
        "TIMES  Textfile listing times of verge-attacks.\n"
        "\n"
        "TRANS  Semitone transposition at start of verge. (Defuault 5).\n"
        "EXP    slope of glissi (1(dflt) to 8) : higher vals gliss faster.\n"
        "DUR    duration of glissi, in mS. (default 100)\n"
        "\n"
        "-n     Use times given (default looks for peak closest to time in src).\n"
        "-b     Boost the level of the verges.\n"
        "-s     Suppress all input except verges. (Boost verges)\n"
        "\n"
        "\"TRANSP\", \"EXP\" and \"DUR\" can vary through time.\n");
    } else
        fprintf(stderr,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/******************************** CHECK_VERGE_PARAM_VALIDITY_AND_CONSISTENCY ********************************/

int check_verge_param_validity_and_consistency(dataptr dz)
{
    int exit_status, chans = dz->infile->channels; 
    double *vtimes = dz->parray[0], srate = (double)dz->infile->srate, maxsamp, maxdur;
    int vsamps, vergsamps, *vsmptimes = dz->lparray[0];
    int sampcnt = 0, lasttime = 0, n, absstart = 0, startpos, endpos, m, maxpos, gap, maxdursamps, winsamps, halfwinsamps, winend;
    float *ibuf;

    if(dz->vflag[1] && dz->vflag[2]) {
        sprintf(errstr,"\"Boost\" and \"Suppress\" cannot be used together.\n");
        return(DATA_ERROR);
    }   
    if(dz->brksize[VRG_TRNSP]) {
        for(n=0,m= 1;n<dz->brksize[VRG_TRNSP];n++,m+=2)
            dz->brk[VRG_TRNSP][m] = pow(2.0,dz->brk[VRG_TRNSP][m]/SEMITONES_PER_OCTAVE);
    } else
        dz->param[VRG_TRNSP] = pow(2.0,dz->param[VRG_TRNSP]/SEMITONES_PER_OCTAVE);

    if(dz->vflag[0]) {
        for(n = 0; n < dz->itemcnt; n++)
            vsmptimes[n] = (int)round(vtimes[n] * srate) * chans;
    } else {

    //  Find length of searchwindow to find peaks at given times
    
        halfwinsamps = (int)round(VRG_SRCHWINDOW * MS_TO_SECS * srate) * chans;
        winsamps = halfwinsamps * 2;

        //  Create temporary sound buffer to use to search for exact peak-positions

        dz->buflen = winsamps;
        dz->buflen *= 2;    // SAFETY
        if((ibuf = (float *)malloc(dz->buflen * sizeof(float))) == NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create temporary sound buffers to check verge time values.\n");
            return(PROGRAM_ERROR);
        }
        
        //  Find true peak-points close to verge times, and store as sample-counts

        dz->total_samps_read = 0;
        for(n = 0; n < dz->itemcnt; n++) { 
            vsamps = (int)round(vtimes[n] * srate) * chans;
            if((absstart = vsamps - halfwinsamps) < 0) {            //  start of window is BEFORE start of file
                if((winend = absstart + winsamps) >= dz->total_samps_read) {
                    if((exit_status = read_samps(ibuf,dz))<0)       //  If file has not been read, read a buffer of samples
                        return exit_status;                         //  (otherwise we already have a buffer of samples)
                }
                startpos = 0;                                       //  Set limits as file-start and end-of-halfwindow after vsamps
                endpos   = vsamps + halfwinsamps;

            } else {                                                //  start of window is AFTER start of file

                if((winend = absstart + winsamps) < dz->buflen) {   //  If end of window is BEFORE end of buffer
                    startpos = absstart;                            //  go to window in current buffer
                    endpos = winend;
                } else {                                            //  Else end of window is AFTER end of buffer
                    sndseekEx(dz->ifd[0],absstart,0);               //  seek and get another bufferfull
                    dz->total_samps_read = absstart;
                    if((exit_status = read_samps(ibuf,dz))<0)
                        return exit_status;
                    startpos = 0;
                    endpos   = winsamps;
                }
            }
            maxsamp = 0.0;
            maxpos = 0;
            for(m=startpos; m < endpos; m++) {
                if(fabs(ibuf[m]) > maxsamp) {
                    maxsamp = fabs(ibuf[m]);
                    maxpos = m;
                }
            }
            maxpos += dz->total_samps_read - dz->ssampsread;        //  Find absolute maxpos
            maxpos = (maxpos/chans) * chans;                        //  and round to start of sample-group
            vsmptimes[n] = maxpos;                                  //  and store.
        }

        free(ibuf);                                                 //  Free temporary soundbuf
    }

    //  Check contractions/expansions in making verges fit within times between verges

    dz->bigverge = 0;
    lasttime = vsmptimes[0];
    for(n = 1; n < dz->itemcnt; n++) { 
        if((exit_status = read_values_from_all_existing_brktables(vtimes[n-1],dz))<0)
            return exit_status;
        vergsamps = (int)round(dz->param[VRG_DUR] * MS_TO_SECS * srate) * chans;
        if((exit_status = find_contraction_sampcount(vergsamps,&sampcnt,dz->param[VRG_TRNSP],dz->param[VRG_CURVE],dz))<0)
            return exit_status;                                 //  Find required insamps at last verge
        gap = vsmptimes[n] - lasttime;                          //  Find sampstep between last verge and this
        if(gap <= sampcnt) {
            sprintf(errstr,"Gap between verge times at lines %d to %d (%lf) insufficent . need %lf secs with current input params\n",
                n,n+1,gap/chans/(double)dz->infile->srate,sampcnt/chans/(double)dz->infile->srate);
            return DATA_ERROR;
        }
        dz->bigverge = max(dz->bigverge,sampcnt);               //  Find biggest gap between verge sample-times
        lasttime = vsmptimes[n];
    }
    if((exit_status = read_values_from_all_existing_brktables(vtimes[n-1],dz))<0)
        return exit_status;
    vergsamps = (int)round(dz->param[VRG_DUR] * MS_TO_SECS * srate) * chans;
    if((exit_status = find_contraction_sampcount(vergsamps,&sampcnt,dz->param[VRG_TRNSP],dz->param[VRG_CURVE],dz))<0)
        return exit_status;
    gap = dz->insams[0] - lasttime;

    if(gap <= sampcnt) {
        sprintf(errstr,"Gap between last verge time and end of src insufficent (gap %d samps reqd %d)\n",gap,sampcnt);
        return DATA_ERROR;
    }
    dz->bigverge = max(dz->bigverge,sampcnt);                   //  Max set of samples needed to make any tiem-expanded verge

    if(dz->brksize[VRG_DUR]) {
        if((exit_status = get_maxvalue_in_brktable(&maxdur,VRG_DUR,dz))<0)
            return exit_status;
        maxdursamps = (int)round(maxdur * srate) * chans;
    } else
        maxdursamps = (int)round(dz->param[VRG_DUR] * srate) * chans;
    dz->bigverge = max(dz->bigverge,maxdursamps);               //  Max set of samples needed to make any verge

    sndseekEx(dz->ifd[0],0,0);
    dz->total_samps_read = 0;
    dz->ssampsread = 0;
    return FINISHED;
}

/******************************** VERGES ********************************/

int verges(dataptr dz)
{
    int exit_status, chans = dz->infile->channels; 
    float *ibuf = dz->sampbuf[0], *iovflwbuf = dz->sampbuf[1], *obuf = dz->sampbuf[2];
    double *vtimes = dz->parray[0], srate = (double)dz->infile->srate, lastvtime;
    int last_total_samps_read, ibufpos, obufpos, n, vsamp, endsamps, vergsamps, maxextend, *vsmptimes = dz->lparray[0];

    //  (much more than) the maximum amount the input-data can be stretched when creating a verge

    maxextend = (int)ceil(dz->application->hi[VRG_TRNSP]);

    lastvtime = 0.0;
    last_total_samps_read = 0;
    ibufpos = 0;
    obufpos = 0;
    dz->buflen *= 2;
    if((exit_status = read_samps(ibuf,dz))<0)
        return exit_status;
    dz->buflen /= 2;

    for(n = 0; n < dz->itemcnt; n++) { 
        vsamp = vsmptimes[n];
        while(last_total_samps_read + ibufpos < vsamp) {                                //  Read and write infile, up to start of next verge
            if(dz->vflag[2])
                obuf[obufpos] = 0.0f;
            else
                obuf[obufpos] = ibuf[ibufpos];
            if(++obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return exit_status;
                memset((char *)obuf,0,dz->buflen * sizeof(float));
                obufpos = 0;
            }
            if(++ibufpos >= dz->buflen * 2) {
                memcpy((char *)ibuf,(char *)iovflwbuf,dz->buflen * sizeof(float));
                memset((char *)iovflwbuf,0,dz->buflen * sizeof(float));
                last_total_samps_read = dz->total_samps_read;
                if((exit_status = read_samps(iovflwbuf,dz))<0)
                    return exit_status;
                ibufpos -= dz->buflen;
            }
        }                                                                               //  Read brktables at verge-time
        if((exit_status = read_values_from_all_existing_brktables(vtimes[n],dz))<0)
            return exit_status;
        vergsamps = (int)round(dz->param[VRG_DUR] * MS_TO_SECS * srate) * chans;
        if(ibufpos + (vergsamps * maxextend) >= dz->buflen * 2) {                       //  Check if verge runs overvend of ovflwbuf
            memcpy((char *)ibuf,(char *)iovflwbuf,dz->buflen * sizeof(float));          //  and if so, bakcpy to ibuf and rad more samps to ovflw buf
            memset((char *)iovflwbuf,0,dz->buflen * sizeof(float));
            last_total_samps_read = dz->total_samps_read;
            if((exit_status = read_samps(iovflwbuf,dz))<0)
                return exit_status;
            ibufpos -= dz->buflen;
        }
        if(dz->vflag[1]) {
            if((exit_status = boost_verge(ibufpos,vergsamps,dz))<0)
                return exit_status;
        } else if(dz->vflag[2]) {
            if((exit_status = boost_verge2(ibufpos,vergsamps,dz))<0)
                return exit_status;
        }
        if((exit_status = write_verge(&ibufpos,&obufpos,vergsamps,dz))<0)
            return exit_status;

    }
    if(!dz->vflag[2]) {
        endsamps = dz->insams[0] - (last_total_samps_read + ibufpos);                   //  If any more samples left
        while(endsamps > 0) {                                                           //  Read them to the output
            obuf[obufpos] = ibuf[ibufpos];
            if(++obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return exit_status;
                memset((char *)obuf,0,dz->buflen * sizeof(float));
                obufpos = 0;
            }
            if(++ibufpos >= dz->buflen * 2) {
                memcpy((char *)ibuf,(char *)iovflwbuf,dz->buflen * sizeof(float));
                memset((char *)iovflwbuf,0,dz->buflen * sizeof(float));
                if((exit_status = read_samps(iovflwbuf,dz))<0)
                    return exit_status;
                ibufpos -= dz->buflen;
            }
            endsamps--;
        }
    }
    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return exit_status;
    }
    return FINISHED;
}

/******************************** CREATE_VERGES_SNDBUFS ********************************/

int create_temporary_verge_sndbufs(dataptr dz)
{
    int chans = dz->infile->channels;
    int bigbufsize, secsize, minbuflen, framesize = F_SECSIZE * chans;

    minbuflen = dz->bigverge;
    dz->buflen = (int)Malloc(-1)/sizeof(float);
    dz->buflen = max(dz->buflen,minbuflen); 
    secsize = dz->buflen/framesize;
    if(secsize * framesize != dz->buflen)
        secsize++;
    dz->buflen = secsize * framesize;
    bigbufsize = (dz->buflen * 3) * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create total sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;                   //  Inbuf
    dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;  //  Inbuf overflow
    dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;  //  Outbuf
    dz->sampbuf[3] = dz->sampbuf[2] + dz->buflen;
    return(FINISHED);
}

/******************************** CREATE_VERGES_SNDBUFS ********************************/

int create_verges_sndbufs(dataptr dz)
{
    int chans = dz->infile->channels;
    int bigbufsize, secsize, minbuflen, framesize = F_SECSIZE * chans;

    minbuflen = dz->bigverge;
    dz->buflen = (int)Malloc(-1)/sizeof(float);
    dz->buflen = max(dz->buflen,minbuflen); 
    secsize = dz->buflen/framesize;
    if(secsize * framesize != dz->buflen)
        secsize++;
    dz->buflen = secsize * framesize;
    bigbufsize = (dz->buflen * 3) * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create total sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;                   //  Inbuf
    dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;  //  Inbuf overflow
    dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;  //  Outbuf
    dz->sampbuf[3] = dz->sampbuf[2] + dz->buflen;
    return(FINISHED);
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
    int done = 0;
    double dummy = 0.0, lasttime;
    FILE *fp;
    int cnt = 0, linecnt;
    char temp[800], *p;

    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read times.\n",str);
        return(DATA_ERROR);
    }
    linecnt = 0;
    lasttime = -1.0;
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy)) {
            if(dummy < 0.0) {
                sprintf(errstr,"Invalid time (%lf) at line %d in file %s.\n",dummy,linecnt+1,str);
                return(DATA_ERROR);
            } else if(dummy <= lasttime) {
                sprintf(errstr,"Times (%lf & %lf) not in increasing order at line %d in file %s.\n",lasttime, dummy,linecnt,str);
                return(DATA_ERROR);
            } else if(dummy > dz->duration) {
                fprintf(stdout,"WARNING: Time (%lf) beyond end of source-file, at line %d in file %s.\n",dummy,linecnt+1,str);
                fprintf(stdout,"WARNING: Ignoring data at and after this time.\n");
                fflush(stdout);
                done = 1;
                break;
            }
            lasttime = dummy;
            cnt++;
        }
        if(done)
            break;
        linecnt++;
    }
    if(cnt == 0) {
        sprintf(errstr,"No valid data found in file %s.\n",str);
        return(DATA_ERROR);
    }
    dz->itemcnt = cnt;
    if((dz->parray = (double **)malloc(sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create time-data storage. (1)\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create time-data storage. (2)\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray = (int **)malloc(sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create time-data storage as samples. (1)\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[0] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create time-data storage as samples. (2)\n");
        return(MEMORY_ERROR);
    }
    fseek(fp,0,0);
    cnt = 0;
    done = 0;
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy)) {
            dz->parray[0][cnt] = dummy;
            if(++cnt >= dz->itemcnt) {
                done = 1;
                break;
            }
        }
        if(done)
            break;
    }
    fclose(fp);
    return FINISHED;
}

/******************************************** WRITE_VERGE ******************************************/

int write_verge(int *ibufpos,int *obufpos,int vergsamps,dataptr dz)
{
    int exit_status, chans = dz->infile->channels, ch;
    float *ibuf = dz->sampbuf[0], *iovflwbuf = dz->sampbuf[1], *obuf = dz->sampbuf[2];
    int startibufpos = *ibufpos, k, m, thispos;
    double dm = 0.0, frac, thisincr, val, nextval, valdiff, env;                //  e.g. start with accel at 1.7                        
    double incr_change = (double)dz->param[VRG_TRNSP] - 1.0;                    //  incr_change = 0.7
    int gp_outcnt = vergsamps/chans;
    int boostedge = VRG_BOOSTEDGE * 2;
    for(k=gp_outcnt,m=0;k>=0;k--,m++) {                                         //  for evey increment
        thisincr  = pow((double)k/(double)gp_outcnt,(double)dz->param[VRG_CURVE]);//incr is k/scnt (raised to any power) .. a value between 1 and 0
        thisincr *= incr_change;                                                //  values beween 0.7 and 0
        thisincr += 1.0;                                                        //  values between 1.7 and 1 i.e. from transposed to not transposed
        dm += thisincr;                                                         //  advance the total increment
        if(k == 0) {                                    
            thispos = (int)round(dm);                                           //  and get corresponding (lower) sample no
            frac = 0.0;                                                         //  The last position of the increment must be at an integer-sample             
        } else {
            thispos = (int)floor(dm);
            frac = dm - (double)thispos;
        }
        *ibufpos = startibufpos + (thispos * chans);                            //  The actual position in the input buffer
        if(*ibufpos >= dz->buflen * 2) {
            memcpy((char *)ibuf,(char *)iovflwbuf,dz->buflen * sizeof(float));  //  if beyond end of iovflwbuf, do copybak and reread
            memset((char *)iovflwbuf,0,dz->buflen * sizeof(float));
            if((exit_status = read_samps(iovflwbuf,dz))<0)
                return exit_status;
            *ibufpos -= dz->buflen;                                             //  resetting ibufpos and
            startibufpos -= dz->buflen;                                         //  and the position from which we measure it!!!
        }
        env = 1.0;
        if(dz->vflag[2]) {                                                      //  If non-verge input is being suppressed
            if(m < boostedge)                                                   //  do splices on start and end of verge
                env = (double)m/(double)boostedge;
            else if(k < boostedge)
                env = (double)k/(double)boostedge;
        }
        for(ch=0;ch < chans;ch++,(*ibufpos)++,(*obufpos)++) {                   //  write all N chans, by interp
            val     = ibuf[*ibufpos];
            nextval = ibuf[*ibufpos + chans];
            valdiff = nextval - val;
            val += valdiff * frac;
            val *= env;
            obuf[*obufpos] = (float)val;
        }
        if(*obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return exit_status;
            memset((char *)obuf,0,dz->buflen * sizeof(float));
            *obufpos = 0;
        }
    }
    return FINISHED;
}

/******************************************** FIND_CONTRACTION_SAMPCOUNT **********************************
 *
 *  How many input samps needed for required output dur
 *
 */

int find_contraction_sampcount(int vsamps,int *sampcnt,double initial_incr,double exp,dataptr dz)
{
    int chans = dz->infile->channels;
    int n;
    double thistotal = 0.0, thisincr;
    double diff = initial_incr - 1.0;
    vsamps /= chans;
    thistotal = 0.0;
    for(n = vsamps; n >= 0;n--) {
        thisincr  = pow((double)n/(double)vsamps,exp);
        thisincr *= diff;
        thisincr += 1.0;
        thistotal += thisincr;
    }
    *sampcnt = (int)ceil(thistotal);
    *sampcnt *= chans;
    return FINISHED;
}

/******************************************** BOOST_VERGE ******************************************/

int boost_verge(int ibufpos,int vergsamps,dataptr dz)
{
    int ch, chans = dz->infile->channels;
    float *ibuf = dz->sampbuf[0];
    double maxsamp, boost, booster, thisbooster;
    int n, k, m, startibufpos = ibufpos, gp_vergsamps = vergsamps/chans;

    maxsamp = fabs(ibuf[ibufpos]);
    for(n=1;n < vergsamps;n++) {
        if(++ibufpos >= dz->buflen * 2) {
            sprintf(errstr,"Buffer overrun in boost_verge.\n");
            return PROGRAM_ERROR;
        }
        maxsamp = max(fabs(ibuf[ibufpos]),maxsamp);
    }
    boost = 0.95 - maxsamp;
    if(boost <= 0.0)
        return FINISHED;
    booster = 0.95/maxsamp;
    ibufpos = startibufpos;
    for(m=0,k=gp_vergsamps-1;m < gp_vergsamps;m++,k--) {
        if(m < VRG_BOOSTEDGE) {
            thisbooster = (double)m/(double)VRG_BOOSTEDGE;
            thisbooster *= booster - 1.0;
            thisbooster += 1.0;
            for(ch = 0;ch < chans;ch++) {
                ibuf[ibufpos] = (float)(ibuf[ibufpos] * thisbooster);
                ibufpos++;
            }
        } else if(k < VRG_BOOSTEDGE) {
            thisbooster = (double)k/(double)VRG_BOOSTEDGE;
            thisbooster *= booster - 1.0;
            thisbooster += 1.0;
            for(ch = 0;ch < chans;ch++) {
                ibuf[ibufpos] = (float)(ibuf[ibufpos] * thisbooster);
                ibufpos++;
            }
        } else {
            for(ch = 0;ch < chans;ch++) {
                ibuf[ibufpos] = (float)(ibuf[ibufpos] * booster);
                ibufpos++;
            }
        }
    }
    return FINISHED;
}

/******************************************** BOOST_VERGE2 ******************************************/

int boost_verge2(int ibufpos,int vergsamps,dataptr dz)
{
    int ch, chans = dz->infile->channels;
    float *ibuf = dz->sampbuf[0];
    double maxsamp, booster;
    int n, startibufpos = ibufpos, gp_vergsamps = vergsamps/chans;
    maxsamp = fabs(ibuf[ibufpos]);
    for(n=1;n < vergsamps;n++) {
        if(++ibufpos >= dz->buflen * 2) {
            sprintf(errstr,"Buffer overrun in boost_verge.\n");
            return PROGRAM_ERROR;
        }
        maxsamp = max(fabs(ibuf[ibufpos]),maxsamp);
    }
    booster = 0.95/maxsamp;
    booster = max(booster,1.0);
    ibufpos = startibufpos;
    for(n=0;n < gp_vergsamps;n++) {
        for(ch = 0;ch < chans;ch++) {                           // Don't need splices on input as we have splices on output
            ibuf[ibufpos] = (float)(ibuf[ibufpos] * booster);
            ibufpos++;
        }
    }
    return FINISHED;
}
