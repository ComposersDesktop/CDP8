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
//  HEREH
//      _cdprogs\stutter stutter alan_bellydancefc.wav test.wav datafile 20 1 0 0 0 1
//      _cdprogs\stutter stutter alan_bellydancefc.wav test.wav datafile dur segjoins silprop silmin silmax seed [-ttrans] [-aatten] [-bbias] [-mmindur] [-p]

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


#ifdef unix
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

#define minseglen is_flat
#define maxseglen is_sharp
#define segcnt    ringsize
#define silcnt    zeroset
#define silence   activebuf
#define smpsdur   total_windows

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int check_stutter_param_validity_and_consistency(dataptr dz);
static int setup_stutter_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_stutter_param_ranges_and_defaults(dataptr dz);
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
static int stutter(dataptr dz);
static int handle_the_special_data(char *str,dataptr dz);
static int stutter_param_preprocess(dataptr dz);
static int create_stutter_sndbufs(dataptr dz);
static void rndpermm(int permlen,int *permm);
static void insert(int m,int t,int permlen,int *permm);
static void prefix(int m,int permlen,int *permm);
static void shuflup(int k,int permlen, int *permm);
static void select_silence_inserts(int *silpermm,dataptr dz);
static int get_segno(int *segs_outcnt,int *permm,dataptr dz);
static int insert_silence(float *obuf,double *time,int *obufpos,dataptr dz);

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
        dz->maxmode = 0;
        // setup_particular_application =
        if((exit_status = setup_stutter_application(dz))<0) {
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
    if((exit_status = setup_stutter_param_ranges_and_defaults(dz))<0) {
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
//  handle_special_data ....
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
//  check_param_validity_and_consistency....
    if((exit_status = check_stutter_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess =
    if((exit_status = stutter_param_preprocess(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    dz->bufcnt = dz->segcnt + 2;
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

    if((exit_status = create_stutter_sndbufs(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //spec_process_file =
    if((exit_status = stutter(dz))<0) {
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

/************************* SETUP_STUTTER_APPLICATION *******************/

int setup_stutter_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,MOTORDATA,6,6,"didddi"))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"tabm",4,"DDDd","p",1,0,"0"))<0)
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

/************************* SETUP_STUTTER_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_stutter_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[STUT_DUR] = 1;
    ap->hi[STUT_DUR] = 7200;
    ap->default_val[STUT_DUR] = 20;
    ap->lo[STUT_JOIN] = 1;
    ap->hi[STUT_JOIN] = STUT_MAX_JOIN;
    ap->default_val[STUT_JOIN] = 1;
    ap->lo[STUT_SIL]  = 0;
    ap->hi[STUT_SIL]  = 1;
    ap->default_val[STUT_SIL]  = 0;
    ap->lo[STUT_SILMIN] = 0;
    ap->hi[STUT_SILMIN] = 10;
    ap->default_val[STUT_SILMIN] = 0.1;
    ap->lo[STUT_SILMAX] = 0;
    ap->hi[STUT_SILMAX] = 10;
    ap->default_val[STUT_SILMAX] = .5;
    ap->lo[STUT_SEED] = 0;
    ap->hi[STUT_SEED] = 256;
    ap->default_val[STUT_SEED] = 1;
    ap->lo[STUT_TRANS] = 0;
    ap->hi[STUT_TRANS] = 3;
    ap->default_val[STUT_TRANS] = 0;
    ap->lo[STUT_ATTEN] = 0;
    ap->hi[STUT_ATTEN] = 1;
    ap->default_val[STUT_ATTEN] = 0;
    ap->lo[STUT_BIAS] = -1.0;
    ap->hi[STUT_BIAS] = 1.0;
    ap->default_val[STUT_BIAS] = 0.0;
    ap->lo[STUT_MINDUR] = STUT_SPLICE + STUT_DOVE;
    ap->hi[STUT_MINDUR] = 250;
    ap->default_val[STUT_MINDUR] = STUT_MIN;
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
            if((exit_status = setup_stutter_application(dz))<0)
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
    usage2("stutter");
    return(USAGE_ONLY);
}

/**************************** CHECK_STUTTER_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_stutter_param_validity_and_consistency(dataptr dz)
{
    double temp;
    if(dz->param[STUT_SIL] == 0.0) {
        if (dz->param[STUT_SILMIN] > 0.0 || dz->param[STUT_SILMAX] > 0.0) {
            sprintf(errstr,"ERROR: You have selected NO silence inserts.\n");
            return DATA_ERROR;
        }
    } else if(dz->param[STUT_SILMIN] > dz->param[STUT_SILMAX]) {
        fprintf(stdout,"WARNING: Min silence duration (%lf) is greater than max silence duration (%lf).\n",dz->param[STUT_SILMIN],dz->param[STUT_SILMAX]);
        fprintf(stdout,"WARNING: Reversing these values.\n");
        fflush(stdout);
        temp = dz->param[STUT_SILMIN];
        dz->param[STUT_SILMIN] = dz->param[STUT_SILMAX];
        dz->param[STUT_SILMAX] = temp;
    }
    return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"stutter"))             dz->process = STUTTER;
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
    if(!strcmp(str,"stutter")) {
        fprintf(stderr,
        "USAGE:\n"
        "stutter stutter infile outfile datafile dur segjoins silprop\n"
        "silmin silmax seed [-ttrans] [-aatten] [-bbias] [-mmindur] [-p]\n"
        "\n"
        "Slice src (e.g. speech) into elements (e.g. words or syllables).\n"
        "Cut segments from elements, always cutting from element start.\n"
        "Play these in a random order (with possible intervening silence).\n"
        "\n"
        "DATAFILE List of times at which to slice src into elements.\n"
        "         Minimum timestep between times (MT) = 0.016 secs.\n"
        "         Times range: MT secs to (duration - MT).\n"
        "DUR      Duration of output.\n"
        "SEGJOINS Value 1: uses the specified elements as srcs to cut.\n"
        "         Value 2: also use pairs-of-segments as srcs.\n"
        "         Value N: also use 2,3,....N joined-segments as srcs.\n"
        "         Where joined segs used, cuts end in the last seg joined.\n"
        "         Range 1 to %d.\n"
        "SILPROP  Silence may be inserted at joins between cut-segments.\n"
        "         \"silprop\" is proportion of joins to have inserted silence.\n"
        "         (Range 0 - 1). (Apart from none, minimum is 1 in %d)\n"
        "SILMIN   Minimum duration of any silences at joins.\n"
        "SILMAX   Maximum duration of any silences at joins.\n"
        "SEED     Same seed value (with all other params same)\n"
        "         produces identical output from any random-varying processes.\n"
        "TRANS    Range (semitones) of any random transposition of segments.\n"
        "ATTEN    Range of any random attenuation of segment level (Range 0-1).\n"
        "BIAS     Bias size of segments cut (Range -1 to 1). 0 = no bias.\n"
        "         -ve vals bias towards smaller segs: +ve vals towards larger segs.\n"
        "MINDUR   Minimum duration of cut segments, in mS ( >8 mS ).\n"
        "-p       Permute elements (all elements used before any used again).\n"
        "         i.e.randomly order elements, cut segs from each and play.\n"
        "         Then permute order, cut new segs and play, etc.\n"
        "         (Default: segs cut from elements taken entirely at random).\n",STUT_MAX_JOIN,STUT_SILDIV);
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/******************************** STUTTER ********************************/

int stutter(dataptr dz)
{
    int exit_status, segs_outcnt, segno, silence_cnt = 0, chans = dz->infile->channels;
    float **ibuf, *sbuf, *obuf;
    int *gp_startcut = dz->lparray[0], *gp_sampsread = dz->lparray[1];
    double time, segdur, dsbufpos, splic, rnd, amp, trns, frac, diff, val, srate = (double)dz->infile->srate;
    double *inseg = dz->parray[0];
    int m, n, j, k, ch, gp_mindur, gp_splicelen, gp_cutlen, gp_cutpos, start_read, samps_to_read, gp_start, ibufpos, sbufpos, gp_sbufpos, obufpos;
 //   int seg_groupings_cnt , next_groupings_end;
    int *permm, *silpermm = NULL;

    gp_splicelen = (int)round(STUT_SPLICE * MS_TO_SECS * srate);
    gp_mindur = (int)round(dz->param[STUT_MINDUR] * MS_TO_SECS * srate);

    if((ibuf = (float **)malloc(dz->segcnt * sizeof(float *)))==NULL) {
        sprintf(errstr,"Insufficient memory to create input sound buffers.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->segcnt;n++)   {                           //  Establish all sound buffers
        ibuf[n] = dz->sampbuf[n];
        memset((char *)ibuf[n],0,dz->buflen * sizeof(float));
    }
    sbuf = dz->sampbuf[n++];
    memset((char *)sbuf,0,dz->buflen * sizeof(float));
    obuf = dz->sampbuf[n];
    memset((char *)obuf,0,dz->buflen * sizeof(float));

    seg_groupings_cnt = dz->itemcnt + 1;
//    next_groupings_end = seg_groupings_cnt; // NOT USED
    for(n=0,m=0;n < dz->segcnt;n++,m+=2) {              //  Cut all segments, noting their (grouped) length
        start_read = (int)round(inseg[m] * srate) * chans;
        if(n == dz->segcnt - 1 || (int)round(inseg[m+2] * srate) * chans == 0) {
            samps_to_read = dz->insams[0] - start_read;
            seg_groupings_cnt--;
//            next_groupings_end += seg_groupings_cnt;
        }
        else {
            segdur = inseg[m+1] - inseg[m];
            samps_to_read = (int)round(segdur * srate) * chans;
        }
        sndseekEx(dz->ifd[0],start_read,0);
        memset((char *)ibuf[n],0,dz->buflen);
        if((dz->ssampsread = fgetfbufEx(ibuf[n],samps_to_read,dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Can't read sample-set %d from input soundfile.\n",n+1);
            return(SYSTEM_ERROR);
        }
        if(dz->ssampsread != samps_to_read) {
            fprintf(stdout,"WARNING: Samps read (%d) not exactly as asked for (%d) for input seg %d\n",dz->ssampsread,samps_to_read,n+1);
            fflush(stdout);
        }

        gp_sampsread[n] = dz->ssampsread/chans;                 //  Remember gp_length of segment read
        gp_start = (int)round(inseg[m] * srate);                //  Find gp_sample start of this segment within input file
        gp_startcut[n] -= gp_start;                             //  Find start-cut-place WITHIN cut-segment

        if(n>0) {                                               //  Splice start and end of segments (except at start and end of source snd)
            ibufpos = 0;
            for(k=0;k<gp_splicelen;k++) {
                splic = (double)k/(double)gp_splicelen;
                for(ch=0;ch<chans;ch++) {
                    ibuf[n][ibufpos] = (float)(ibuf[n][ibufpos] * splic);
                    ibufpos++;
                }
            }
        }
        if(n<dz->segcnt-1) {
            for(k=0,j=gp_sampsread[n] - 1;k<gp_splicelen;k++,j--) {
                ibufpos = j * chans;
                splic = (double)k/(double)gp_splicelen;
                for(ch=0;ch<chans;ch++) {
                    ibuf[n][ibufpos] = (float)(ibuf[n][ibufpos] * splic);
                    ibufpos++;
                }
            }
            memset((char *)(ibuf[n] + dz->ssampsread),0,(dz->buflen - dz->ssampsread) * sizeof(float));
        }
    }                                                           //  Establish array for permuting order of segments, and possibly, occurence of silences
    if((permm = (int *)malloc(dz->segcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory to create segment-order permutation store.\n");
        return(MEMORY_ERROR);
    }                                                               
    if(dz->param[STUT_SIL] > 0.0) {                             //  If there are silence insertions, set up a permutation array for yes-no 
        if((silpermm = (int *)malloc(STUT_SILDIV * sizeof(int)))==NULL) {
            sprintf(errstr,"Insufficient memory to create segment-order permutation store.\n");
            return(MEMORY_ERROR);
        }                                                               
    }

    srand(dz->iparam[STUT_SEED]);                               //  Initialise randomisation
    if(dz->vflag[STUT_PERM])                                    //  If order of segments is to be permuted, set up first rand permutation       
        rndpermm(dz->segcnt,permm);                             //  Permute order of segments

    time = 0.0;
    segs_outcnt = 0;
    obufpos = 0;
    silence_cnt = 0;
    while(time < dz->param[STUT_DUR]) {
        if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
            return exit_status;
        sbufpos = 0;
        ibufpos = 0;
        segno = get_segno(&segs_outcnt,permm,dz);               //  Select a segment
        gp_cutlen = gp_sampsread[segno] - gp_startcut[segno];   //  Find the cutable length (from zero or, end of prior joined segment, to end of seg)
        rnd = drand48();
        if(dz->param[STUT_BIAS] != 1.0)                         // Generate (possibly biased) random value
            rnd = pow(rnd,dz->param[STUT_BIAS]);
                                                                //  Make a random cut of the CUTABLE LENGTH, which is at least gp_mindur in length
        gp_cutpos = (int)round((double)(gp_cutlen - gp_mindur) * rnd);
        gp_cutpos += gp_mindur;
        gp_cutpos += gp_startcut[segno];                        //  Include any prior joined-segs
        amp = 1.0;
        if(dz->param[STUT_ATTEN]) {
            rnd = drand48() * dz->param[STUT_ATTEN];
            amp -= rnd;
        }
        memset((char *)sbuf,0,dz->buflen * sizeof(float));
        for(n=0;n < gp_cutpos ;n++) {                           //  Copy segment to the segment buffer
            for(ch=0;ch < chans;ch++) {
                sbuf[sbufpos] = (float)(ibuf[segno][ibufpos] * amp);
                sbufpos++;
                ibufpos++;
            }
        }
        for(n = 0,m = gp_cutpos - 1;n < gp_splicelen;n++,m--) {//   Spice end of cut segment
            splic = (double)n/(double)gp_splicelen;
            sbufpos = m * chans;
            for(ch=0;ch < chans;ch++) {
                sbuf[sbufpos] = (float)(sbuf[sbufpos] * splic);
                sbufpos++;
            }
        }
        if(dz->param[STUT_TRANS]) {                             //  If segment is to be transposed
            trns = (drand48() * 2.0) - 1.0;                     //  Range -1 t0 1
            trns *= dz->param[STUT_TRANS];                      //  Range -TRANS to +TRANS semitones
            trns = pow(2.0,trns/SEMITONES_PER_OCTAVE);          //  and convert to frq ratio
        } else
            trns = 1.0;

        dsbufpos = 0.0; 
        while(dsbufpos<gp_cutpos) {                         //  copy segment to output, with possible transposition
            gp_sbufpos = (int)floor(dsbufpos);
            frac = dsbufpos - (double)gp_sbufpos;
            sbufpos = gp_sbufpos * chans;
            for(ch = 0;ch<chans;ch++) {
                val  = sbuf[sbufpos];
                diff = sbuf[sbufpos + chans] - val;
                val += frac * diff;
                obuf[obufpos++] = (float)val;
                sbufpos++;
            }
            if(obufpos >= dz->buflen) {                         //  If outbuffer fills  
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return exit_status;                         //  Write buffer-full of sound
                memset((char *)obuf,0,dz->buflen * sizeof(float));
                obufpos = 0;                                    //  and reset obuffer pointer
            }
            dsbufpos += trns;
        }
        time = (double)((dz->total_samps_written + obufpos)/chans)/srate;
        if(time >= dz->param[STUT_DUR])                         //  Check  time after segment written, and if reached required duration, break
            break;

        if(dz->param[STUT_SIL] > 0.0) {                         //  If there are silence insertions
            if(silence_cnt >= dz->silcnt) {                     //  If reached end of silence yes-no array 
                select_silence_inserts(silpermm,dz);            //  re-permute
                silence_cnt = 0;
            }
            if(dz->silence[silence_cnt]) {                      //  If this join is flagged for a silent-insert
                if((exit_status = insert_silence(obuf,&time,&obufpos,dz))<0)
                    return exit_status;                         //  do silent insert
            }
            silence_cnt++;                                      //  Advance yes-no flags array pointer
        }
    }
    if(obufpos > 0) {   
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
    int done = 0;
    double dummy = 0.0, lasttime;
    double splicedur = MOT_SPLICE * MS_TO_SECS;
    double dblsplicedur = splicedur * 2;
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
            if(cnt == 0) {
                if(dummy <= dblsplicedur) {
                    sprintf(errstr,"Invalid time (%lf) (closer to start than 2 splicedurs = %.3lf) at line %d in file %s.\n",dummy,dblsplicedur,linecnt+1,str);
                    return(DATA_ERROR);
                }
            } else if(dummy <= lasttime + dblsplicedur) {
                sprintf(errstr,"Times (%lf & %lf) not increasing by 2 splicedurs (%.3lf) line %d in file %s.\n",lasttime, dummy,dblsplicedur,linecnt,str);
                return(DATA_ERROR);
            } else if(dummy >= dz->duration - dblsplicedur) {
                fprintf(stdout,"WARNING: Time (%lf) too near or beyond end of source-file, at line %d in file %s.\n",dummy,linecnt+1,str);
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
    if((dz->parray = (double **)malloc(2 * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create slice-time-data storage. (1)\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[1] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create slice-time-data storage. (2)\n");
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
            dz->parray[1][cnt] = dummy;
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

/**************************** STUTTER_PARAM_PREPROCESS *****************************/

int stutter_param_preprocess(dataptr dz)
{
    double srate = (double)dz->infile->srate, segdur;
    int chans = dz->infile->channels;
    double *inslice = dz->parray[1], *outcuts;
    int n, m, k, kk, tot, outcnt, *gp_startcut;
    double overlap = (STUT_DOVE + STUT_SPLICE) * MS_TO_SECS;
    int datacnt = 0;
    dz->maxseglen = 0.0;
    dz->minseglen = HUGE;
    outcnt = (dz->itemcnt + 1) * 2;                 //  N slices (dz->itemcnt) converted to N+1 pairs of cut-times
    for(k = 0; k < dz->iparam[STUT_JOIN]; k++) {    //  Supplemented by others pairs, if segs are to be joined (datacnt)
        datacnt += outcnt;
        outcnt -= 2;
    }
    dz->segcnt = datacnt/2;
    if(dz->iparam[STUT_JOIN] > dz->segcnt) {
        sprintf(errstr,"Maximum joining of segments (%d) greater than actual number of segments (%d).\n",dz->iparam[STUT_JOIN],dz->segcnt);
        return DATA_ERROR;
    }
    if((dz->parray[0] = (double *)malloc(datacnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create segment-times storage.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray = (int **)malloc(2 * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create cut-data storage. (1)\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[0] = (int *)malloc(dz->segcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create cut-segs start and end storage.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[1] = (int *)malloc(dz->segcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create cut-segs length storage.\n");
        return(MEMORY_ERROR);
    }
    gp_startcut = dz->lparray[0];

    outcuts = dz->parray[0];                        //  e.g. itemcnt  = 4
    outcnt = (dz->itemcnt + 1) * 2;                 //  e.g. outcnt   = (4+1)*2 = 10
    tot = 0;
    for(k = 0; k < dz->iparam[STUT_JOIN]; k++) {
        outcuts[tot+outcnt-1] = dz->duration;   //  End cut is always at end of file
        m = dz->itemcnt - 1; 
        if(m-k >= 0) {
            for(n=tot+outcnt-2; m >= k; n-=2,m--) {
                outcuts[n]   = max(inslice[m-k] - overlap,0.0); //  Ensure startcut of each segment-group is NOT before file-start
                outcuts[n-1] = inslice[m] + overlap;
                segdur = outcuts[n+1] - outcuts[n];
                dz->maxseglen = max(dz->maxseglen,segdur);
                dz->minseglen = max(dz->minseglen,segdur);
            }
        }
        outcuts[tot]   = 0.0;                   //  Start cut is always at start of file
        segdur = outcuts[tot+1] - outcuts[tot];
        dz->maxseglen = max(dz->maxseglen,segdur);
        dz->minseglen = max(dz->minseglen,segdur);
        tot += outcnt;                          //  Advance to end of previous perm-set written
        outcnt -= 2;                            //  Reduce the number of items to write by 1-pair 
    }                                           //  (pairs go from A B C D (4) -> AB BC CD (3) -> ABC BCD (2) -> ABCD (1))
    //  e.g.                                    
    //  orig storage m= 0   1   2   3           dz->itemcnt = 4
    //  prog vals       A   B   C   D           k = 0 (single-segs)     initially
    //                                          outcnt = (4+1)*2 = 10   m-k = m-0 = 3, so 7&8 both get D
    //  final storage   0   1   2   3   4   5   6   7   8   9
    //  final vals      0   A+  -A  B+  B-  C+  C-  D+  D-  dur
    //                                          k = 1 (double-segs)     initially
    //                                          outcnt = (3+1)*2 = 8    m-k = m-1 = 2 so 6getsC and 5getsD
    //  final storage   0   1   2   3   4   5   6   7
    //  final vals      0   B+  A-  C+  B-  D+  C-  dur
    //                                          k = 2 (triple-segs)     initially
    //                                          outcnt = (2+1)*2 = 6    m-k = m-2 = 1 so 4getsB and 3getsD
    //  final storage   0   1   2   3   4   5   
    //  final vals      0   C+  A-  D+  B-  dur
    //                                          k = 3 (quad-segs)       initially
    //                                          outcnt = (1+1)*2 = 4    m-k = m-3 = 0 so 2getsA and 1getsD
    //  final storage   0   1   2   3
    //  final vals      0   D+  A-  dur
    //                                          k = 4 (5seg = allfile)  initially
    //                                          outcnt = (0+1)*2 = 2    m-k = m-4 = -1 so loop is not triggered
    //  final storage   0   1                   
    //  final vals      0   dur

    if(dz->minseglen < dz->param[STUT_MINDUR] * MS_TO_SECS) {
        sprintf(errstr,"Smalleset cut segment (%lf mS) is shorter than min duration set (%lf mS)\n",dz->minseglen * SECS_TO_MS,dz->param[STUT_MINDUR]);
        return DATA_ERROR;
    }
    kk = 0;
    for(k = 0; k < dz->iparam[STUT_JOIN]; k++) {            //  when k = 0, n runs frm -1 to 3 all cuts start at seg-starts + initial cut at 0 i.e. 0 A B C D
        for(n = k-1; n < dz->itemcnt; n++) {                //  when k = 1, n runs from 0 to 3 all cuts start 1 seg after double-seg-start     i.e. A B C D
            if(kk >= datacnt) {                             //  when k = 2, n runs from 1 to 3 all cuts start 2 segs after triple-seg-start    i.e. B C D
                sprintf(errstr,"Unexpected array overrun, storing segment-startcut data.\n");
                return PROGRAM_ERROR;                       //  when k = 3, n runs from 2 to 3 all cuts start 3 segs after  quad-seg-start     i.e. C D
            }                                               //  when k = 4, n runs from 3 to 3 only cut start 4 segs after  quin-seg-start     i.e. D
            if(n < 0)                                       //  i.e. with 4 slices there are 5 segments, and this is the entire input file!!
                gp_startcut[kk++] = 0;
            else                                            //  This is gp_sample at which cutting can begin in this segment, in absolute gp_samples
                gp_startcut[kk++] = (int)round((inslice[n] - overlap) * srate);

        }
    }

    if(dz->param[STUT_SIL] > 0) {                           //  If silences are to be inserted
        dz->param[STUT_SIL] *= (double)STUT_SILDIV;         //  establish proportion of silences
        dz->silcnt = (int)round(dz->param[STUT_SIL]);       //  and set up silence yes-no store
        if((dz->silence = (int *)malloc(STUT_SILDIV * sizeof(int)))==NULL) {
            sprintf(errstr,"Insufficient memory to create silence choice store.\n");
            return(MEMORY_ERROR);
        }
    } else
        dz->silcnt = 0;                                         
                                                                
    if(dz->brksize[STUT_BIAS]) {                                    //  Invert the bias vals entered.
        for(n=0,m=1;n<dz->brksize[STUT_BIAS];n++,m+=2) {            //  bias (range -1 to 1) becomes pow(10,0,bias) (range .1 to 10)
            dz->brk[STUT_BIAS][m] = -dz->brk[STUT_BIAS][m];         //  Biasing done by raising a linear function  between 0 and 1 to a power.  
            dz->brk[STUT_BIAS][m] = pow(10.0,dz->brk[STUT_BIAS][m]);//  So vals <0 (= < 1) produce weighting to LONGER cuts, as curve becomes fast-slow rise    
        }                                                           //  But entered values < 0 are intend to make bias towards SHORTER cuts.
    } else {                                                        //  Hence we invert the values.
        dz->param[STUT_BIAS] = -dz->param[STUT_BIAS];
        dz->param[STUT_BIAS] = pow(10.0,dz->param[STUT_BIAS]);
    }   
    dz->smpsdur = (int)round(dz->param[STUT_DUR] * srate) * chans;//    Get total required duration in samples
    return FINISHED;
}

/******************************** CREATE_STUTTER_SNDBUFS ********************************/

int create_stutter_sndbufs(dataptr dz)
{
    int n, chans = dz->infile->channels;
    int bigbufsize, secsize, framesize = F_SECSIZE * chans;
    double srate = (double)dz->infile->srate;

    dz->buflen = (int)ceil(dz->maxseglen * srate) * chans;
    secsize = dz->buflen/framesize;
    if(secsize * framesize != dz->buflen)
        secsize++;
    dz->buflen = secsize * framesize;
    dz->buflen += chans;    //  wrap-around point
    bigbufsize = (dz->buflen * dz->bufcnt) * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create total sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;                           //  1 Inbuf for each infile
    for(n=1;n < dz->bufcnt;n++)                                             //  1 untransposed segment buf
        dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + dz->buflen;    //  1 output buf
    dz->sampbuf[n] = dz->sampbuf[n-1] + dz->buflen;
    return(FINISHED);
}

/*************************** RNDPERMM ********************************/

void rndpermm(int permlen,int *permm)
{
    int n, t;
    for(n=0;n<permlen;n++) {        /* 1 */
        t = (int)(drand48() * (double)(n+1));       /* 2 */
        if(t==n)
            prefix(n,permlen,permm);
        else
            insert(n,t,permlen,permm);
    }
}

/***************************** INSERT **********************************
 *
 * Insert the value m AFTER the T-th element in permm[pindex].
 */

void insert(int m,int t,int permlen,int *permm)
{   
    shuflup(t+1,permlen,permm);
    permm[t+1] = m;
}

/***************************** PREFIX ************************************
 *
 * Insert the value m at start of the permutation permm[pindex].
 */

void prefix(int m,int permlen,int *permm)
{
    shuflup(0,permlen,permm);
    permm[0] = m;
}

/****************************** SHUFLUP ***********************************
 *
 * move set members in permm[pindex] upwards, starting from element k.
 */

void shuflup(int k,int permlen, int *permm)
{
    int n, *i;
    int z = permlen - 1;
    i = permm + z;
    for(n = z;n > k;n--) {
        *i = *(i-1);
        i--;
    }
}

/****************************** SELECT_SILENCE_INSERTS ***********************************/

void select_silence_inserts(int *silpermm,dataptr dz)
{
    int n;
    memset((char *)dz->silence,0,STUT_SILDIV);  //  Preset all joins to have no silence (silence-off = 0)
    rndpermm(STUT_SILDIV,silpermm);             //  Permute order of joins
    for(n=0;n < dz->silcnt;n++)                 //  Set 1st "silcnt" items of perm to silence-on (=1)
        dz->silence[silpermm[n]] = 1;
}

/****************************** GET_SEGNO ***********************************/

int get_segno(int *segs_outcnt,int *permm,dataptr dz)
{
    int segno;
    if(dz->vflag[STUT_PERM]) {
        if(*segs_outcnt >= dz->segcnt) {
            rndpermm(dz->segcnt,permm);         //  Permute order of segments
            *segs_outcnt = 0;
        }
        segno = permm[*segs_outcnt];
    } else                                      //  segno entirely random
        segno = (int)floor(drand48() * dz->segcnt);
    (*segs_outcnt)++;
    return segno;
}

/****************************** INSERT_SILENCE ***********************************/

int insert_silence(float *obuf,double *time,int *obufpos,dataptr dz)
{
    int exit_status, chans = dz->infile->channels;
    int silsmps, endsmp, overrun;
    double srate = (double)dz->infile->srate;
    double silrange = dz->param[STUT_SILMAX] - dz->param[STUT_SILMIN];

    silsmps = (int)round(drand48() * silrange * srate) * chans; //  Generate random duration silence within specified ranges
    *obufpos += silsmps;                                            
    endsmp = dz->total_samps_written + *obufpos;                    //  Calc where this will write to, in absolute samples
    if((overrun = endsmp - dz->smpsdur) > 0)                        //  Check if this overruns required duration
        *obufpos -= overrun;                                        //  and if it does, reduce the quantity of silence to write
    while(*obufpos >= dz->buflen) {
        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)       //  If obuf overflows
            return exit_status;                                     //  Write buffer-full of sound+silence or silence
        memset((char *)obuf,0,dz->buflen * sizeof(float));
        *obufpos -= dz->buflen;                                     //  and reset obuf pointer
    }                                                               //  Then recalc endtime     
    *time = (double)((dz->total_samps_written + *obufpos)/chans)/srate;
    return FINISHED;
}
