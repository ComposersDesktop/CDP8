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
#include <science.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>


#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

#define maxinseg rampbrksize
#define maxoutseg temp_sampsize
#define splicelen ringsize
#define warned zeroset
#define hastail fzeroset
#define notailanywhere is_mapping

#define SAFETY 4

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.1";

//CDP LIB REPLACEMENTS
static int setup_crumble_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_crumble_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int open_the_outfile(dataptr dz);
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
static int check_crumble_param_validity_and_consistency(dataptr dz);
static int create_crumble_sndbufs(dataptr dz);
static void shuflup(int *perm,int permsize,int k);
static void prefix(int *perm,int permsize,int n);
static void insert(int *perm,int permsize,int n,int t);
static void doperm(int *perm,int permsize,int *lastperm,int *lastperm2);
static int crumble(dataptr dz);

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
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_crumble_application(dz))<0) {
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
    if((exit_status = setup_crumble_param_ranges_and_defaults(dz))<0) {
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
//  check_param_validity_and_consistency ....
    if((exit_status = check_crumble_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    dz->bufcnt = 5;
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

    // create_sndbufs .....
    if((exit_status = create_crumble_sndbufs(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess()                        redundant
    //spec_process_file =
    if((exit_status = open_the_outfile(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = crumble(dz))<0) {
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
/* RWD malloc changed to calloc; helps debug version run as release! */

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
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
    int exit_status;
    if(dz->mode == 1)
        dz->infile->channels = 16;
    else
        dz->infile->channels = 8;
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
        return(exit_status);
    dz->infile->channels = 1;
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

/************************* SETUP_CRUMBLE_APPLICATION *******************/

int setup_crumble_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if(dz->mode==0)
        exit_status = set_param_data(ap,0   ,12,11,"ddd0iDDDDDDi");
    else
        exit_status = set_param_data(ap,0   ,12,12,"ddddiDDDDDDi");
    if(exit_status<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"",0,"","std",3,3,"dDd"))<0)
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
        } else if(infile_info->channels != MONO)  {
            sprintf(errstr,"File %s is not a mono soundfile\n",cmdline[0]);
            return(DATA_ERROR);
        } else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
            sprintf(errstr,"Failed to copy file parsing information\n");
            return(PROGRAM_ERROR);
        }
        free(infile_info);
    }
    return(FINISHED);
}

/************************* SETUP_CRUMBLE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_crumble_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[CRSTART] = 0;
    ap->hi[CRSTART] = dz->duration;
    ap->default_val[CRSTART] = 0;
    ap->lo[CRSTEP1] = 0;
    ap->hi[CRSTEP1] = dz->duration;
    ap->default_val[CRSTEP1] = 0;
    ap->lo[CRSTEP2] = 0;
    ap->hi[CRSTEP2] = dz->duration;
    ap->default_val[CRSTEP2] = 0;
    if(dz->mode == 1) {
        ap->lo[CRSTEP3] = 0;
        ap->hi[CRSTEP3] = dz->duration;
        ap->default_val[CRSTEP3] = 0;
    }
    ap->lo[CRORIENT] = 1;
    if(dz->mode == 1)
        ap->hi[CRORIENT] = 16;
    else
        ap->hi[CRORIENT] = 8;
    ap->default_val[CRORIENT] = 1;

    ap->lo[CRSIZE]  = 10.00001 * MS_TO_SECS;
    ap->hi[CRSIZE]  = dz->duration;
    ap->default_val[CRSIZE] = min(.25,dz->duration/2.0);
    ap->lo[CRRAND]  = 0;
    ap->hi[CRRAND]  = 1;
    ap->default_val[CRRAND] = 0;
    ap->lo[CRISCAT] = 0;
    ap->hi[CRISCAT] = 1;
    ap->default_val[CRISCAT] = 0;
    ap->lo[CROSCAT] = 0;
    ap->hi[CROSCAT] = 1;
    ap->default_val[CROSCAT] = 0;
    ap->lo[CROSTR]  = 1;
    ap->hi[CROSTR]  = 64;
    ap->default_val[CROSTR] = 1;
    ap->lo[CRPSCAT] = 0;
    ap->hi[CRPSCAT] = 12;
    ap->default_val[CRPSCAT] = 0;
    ap->lo[CRSEED]  = 1;
    ap->hi[CRSEED]  = 256;
    ap->default_val[CRSEED] = 1;
    ap->lo[CRSPLICE] = 2;
    ap->hi[CRSPLICE] = 50;
    ap->default_val[CRSPLICE] = 5;
    ap->lo[CRTAIL] = 0;
    ap->hi[CRTAIL] = 1000;
    ap->default_val[CRTAIL] = 0;
    ap->lo[CRDUR] = 0;
    ap->hi[CRDUR] = 3600;
    ap->default_val[CRDUR] = 0;
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
            if((exit_status = setup_crumble_application(dz))<0)
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
    usage2("sound");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"sound"))               dz->process = CRUMBLE;
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
    if(!strcmp(str,"sound")) {
        fprintf(stdout,
        "USAGE: crumble sound 1 inf outf stt dur1 dur2      params\n"
        "OR:    crumble sound 2 inf outf stt dur1 dur2 dur3 params\n"
        "\n"
        "where PARAMS are\n"
        "orient size rand iscat oscat ostrch pscat seed [-ssplice] [-ttail]\n"
        "\n"
        "Project a MONO source on all channels of multichan output, then segment it,\n"
        "and distribute the segments over smaller and smaller groups of channels.\n"
        "\n"
        "Mode 1 gives 8-channel output  (ochans = 8)\n"
        "Mode 2 gives 16-channel output (ochans = 16)\n"
        "\n"
        "STT     Start time of crumbling.\n"
        "DUR1    Duration of section where signal split into 2 images.\n"
        "DUR2    Duration of section where signal split into 4 images.\n"
        "DUR3    (Mode 2) Duration of section where signal split into 8 images.\n"
        "ORIENT  Input image first splits onto 2 blocks each of ochans/2 adjacent chans.\n"
        "        (1) chan \"ori\" with adjacent clockwise channels,\n"
        "        (2) remaining channels.\n"
        "SIZE*   (Average) duration of cut segments.\n"
        "RAND*   Randomisation of segment size. (Range 0 - 1)\n"
        "        Max(1) modifies \"siz\" randomly between siz/2 and (3*siz)/2\n"
        "ISCAT*  Scattering (in source) of start-time of next segment cut. (Range 0 - 1)\n"
        "        Cut-time in source always advances.\n"
        "        With no scatter, step to next cut-time = length of previous segment cut.\n"
        "        With max \"isct\"(1) step  by random time between 0 and previous-seglen.\n"
        "OSCAT*  Scattering (in output) of time-placement of segment used.(Range 0 - 1)\n"
        "        Time in output always advances.\n"
        "        With no scatter, step to next output-time = length of last segment cut.\n"
        "        With max \"osct\"(1) step by random time between >0 and previous-seglen.\n"
        "OSTRCH* Stretching of time-placement in output. (Range 1 - 64)\n"
        "        Step to next sgement-placement in output, is multiplied by ostr.\n"
        "        ostr > 1 should generate silent gaps in the output.\n"
        "PSCAT*  Pitch variation of output segments (Semitones).\n"
        "        for e.g. psct=3, pitch varies at random between +3 and -3 semitones.\n"
        "SEED    Same seed value gives identical output on successive process runs.\n"
        "SPLICE  Length of splices which cut the segments (in millseconds).\n"
        "TAIL*   Length of any exponential tail on segments (in millseconds).\n"
        "\n"
        "Parameters marked with \"*\" can vary over time (i.e. time in the outfile).\n"
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

/******************************** CRUMBLE ********************************
 *
 * For an 8-channel output...
 *  The process starts copying the mono input to ALL channels of the multichan output.
 *  At time-boundary[0] = "monoreadend", it begins to send segments of the input to channels 1234 and 5678 alternately
 *  (could be 2345 6781 etc. depending on the aprameter "orient").
 *  At time-boundary[1], begins to send segments of the input to channels 12 34 56 and 78 randomly permuted
 *  (could be 23 45 67 81 etc. depending on the parameter "orient").
 *  At time-boundary[2], begins to send segments of the input to channels 1 2 3 4 5 6 7 and 8 randomly permuted
 *
 *  If the segments have no tails (CRTAIL = 0), they are cut using a "splicelen" splice at both ends.
 *  With no input scatter (CRISCAT), segments are cut from the source in time order, with no gaps or overlaps.
 *  With scatter, start of next segment may (randomly) begin within the previous segment.
 *
 *  With no output scatter (CROSCAT), segments are joined to the output in time order, with no gaps or overlaps.
 *  With scatter, start of next segment may begin (randomly) within the previous segment (<1)
 *  or (randomly) beyond the end of the previous segment (>1).
 *
 *  After being cut, the segments may be (randomly) transposed within a speicified range (CRPSCAT).
 *
 *  If the segments have tails (CRTAIL > 0), segments calculations are done AS IF the segments had no tails
 *  (cutting time within source and placement time in output are calculated exactly as above
 *  using the length of the segments BEFORE the tail is added, in all calculations).
 *
 *  When the segments are cut, however, extra samples are added to the end of the bare seglength
 *  and in the output, an exponential decay is applied over these tails.
 *
 *  If tails are specified, the initial (unsegmented) section is also given a tail.
 *  This means that, initially "monoreadend" must be set at the END of the tail, so the mono->multimono process
 *  continues to the end of the tail.
 *  The read_process must then baktrak to the start of the tail, but it will then be BEFORE the "monoreadend" marker.
 *  to ensure that that this does not trap the process in looping in the mono->multimono process,
 *  the "monoreadend" is reset to ZERO, so the process knows it must not continue with mono->multimono processing.
 */

int crumble(dataptr dz)
{
    int exit_status, ochancnt, passno, chan, ochan, startchan, thischan, reread, sector, lastsector, n, m, firstread, monoread_completed;
    int curtailed, permsize, permcnt = 0, gpsize = 0, thisperm, lastperm, lastperm2, **perm;
    int obuflen, segsamps, segend, oseglen, instep, outstep, lo, hi, crstartsamps, thistail;
    int write_end = 0, segstart_opos, opos, ipos, epos, eposend, samples_before_bufstart;
    float *ibuf = dz->sampbuf[0], *iovbuf = dz->sampbuf[1], *obuf = dz->sampbuf[2], *oovbuf = dz->sampbuf[3], *ebuf = dz->sampbuf[4];
    double time, pshift, incr, frac, diff, val, iposd, boundary[4], balance[5];
    double maxsamp, normaliser = 1.0, srate = (double)dz->infile->srate, rval, spliceval, crsize, crstart;
    int samptime, endsplicstart, monoreadend, orig_monoreadend, splicetime, endsplice, buffer_end;

    crstart = dz->param[CRSTART];                               //  Start of segmentation process (end of monoread section)
    crstartsamps = (int)ceil(crstart * srate);                  //  Start of segmentation process

    if(dz->mode == 1)
        ochancnt = 16;
    else
        ochancnt = 8;
    obuflen = dz->buflen * ochancnt;

    if((perm = (int **)malloc(17 * sizeof(int *)))==NULL) {     //  Establish the arrays to permute channel and channel-group order
        sprintf(errstr,"Insufficient memory for random permutation arrays.\n");
        return(MEMORY_ERROR);
    }
    permsize = 2;                                               //  sets up perm[2], perm[4], perm[8] (perm[16]) containing 2,4,8,(16) integer entries to perm
    while(permsize < 17) {                                      //  (perm[2] is redundant, but it simplifies later coding to create it here)
        if((perm[permsize] = (int *)malloc(permsize * sizeof(int)))==NULL) {
            sprintf(errstr,"Insufficient memory for random-permutation-array[%d].\n",permsize);
            return(MEMORY_ERROR);
        }
        permsize *= 2;
    }
    boundary[0] = crstart;                                      //  End of copying mono to all of outchannels
    boundary[1] = boundary[0] + dz->param[CRSTEP1];             //  End of routing to (ochancnt/2)-size channel-groupings
    boundary[2] = boundary[1] + dz->param[CRSTEP2];             //  End of routing to (ochancnt/4)-size channel-groupings
    balance[1]  = ROOT_2;
    balance[2]  = balance[1] * ROOT_2;                          //  As output divided amongst more channels,
    balance[3]  = balance[2] * ROOT_2;                          //   individual channel level increases by ROOT2
    if(dz->mode==0)
        lastsector = 3;                                         //  In last sector, output routed to individual channels
    else {
        boundary[3] = boundary[2] + dz->param[CRSTEP3];
        balance[4]  = balance[3] * ROOT_2;
        lastsector = 4;
    }
    //  CHECK FOR TAIL ON END OF "MONOREAD" SECTION
    

    for(passno = 0;passno < 2;passno++) {
        srand((int)dz->param[CRSEED]);                          //  Reset random seed, so levels in real output pass match levels in level-check pass.
        permsize = 2;                                           //  For the chan-group (or channel) permutations that will be acative (2,4,8,(16))
        while(permsize < 17) {                                  //  Establish initial permutation of order for each active grouping
            lastperm  = -1;                                     //  For more details, see 2nd call to "doperm" below, and the notes with "doperm" function
            lastperm2 = -1;
            for(n=0;n<permsize;n++)
                perm[permsize][n] = n;                          //  Do initial random permute of entries for 2-perm (others are done later)
            if(n==2)
                doperm(perm[permsize],permsize,&lastperm,&lastperm2);
            permsize *= 2;
        }
        if(passno == 0)
            fprintf(stdout,"INFO: Assessing output level.\n");
        else
            fprintf(stdout,"INFO: Generating output.\n");

        //  RESET ALL PASS VARIABLE

        if(passno == 1) {
            if((sndseekEx(dz->ifd[0],0,0) < 0)){
                sprintf(errstr,"sndseek failed in input file.\n");
                return SYSTEM_ERROR;
            }
        }
        if(dz->notailanywhere)                                  
            dz->hastail = 0;
        else {                                                      //  If the segments have tails
            if(dz->brksize[CRTAIL]) {                               //  Find the tail length at the end of the monoread section
                if((exit_status = read_value_from_brktable(dz->param[CRSTART],CRTAIL,dz))<0)
                    return exit_status;
            }
            if(flteq(dz->param[CRTAIL],0.0))
                dz->hastail = 0;
            else
                dz->hastail = 1;
        }
        if(dz->hastail) {                                           //  If segment has tail at end of monoread section
            dz->param[CRTAIL] *= MS_TO_SECS;                        //  Read the tail-length, and ensure it is at-least minimum length
            dz->param[CRTAIL] = max(dz->param[CRTAIL],dz->param[CRSPLICE] * 2);
            dz->iparam[CRTAIL] = (int)ceil(dz->param[CRTAIL] * srate);
            endsplicstart = (int)ceil(crstart * srate);         //  start-of-endsplice is AT END of monoread-segment
            orig_monoreadend = endsplicstart + dz->iparam[CRTAIL];  //  and we initially READ to end of the tail
        
        } else {                                                    //  If there is NO TAIL at end of monoread-segment
            orig_monoreadend = (int)round(crstart * srate);     //  We initially READ to end of the segment.
            endsplicstart = orig_monoreadend - dz->iparam[CRSPLICE];//  and start-of-endsplice is just BEFORE end of monoread-segment.
        }
        monoreadend = orig_monoreadend;
        dz->samps_left = dz->insams[0];
        dz->total_samps_read = 0;
        samples_before_bufstart = 0;
        dz->total_samps_written = 0;
        write_end = 0;
        sector = 0;
        ipos = 0;
        opos = 0;
        time = 0.0;
        splicetime = -endsplicstart;                                //  splice time gets to 0 at start of splice
        samptime = 0;
        maxsamp = 0.0;
        thisperm = perm[2][0];
        firstread = 1;
        lastperm = -1;
        lastperm2 = -1;
        monoread_completed = 0;
        curtailed = 0;
        memset((char *)ibuf,0,dz->buflen * 2 * sizeof(float));      //  Zero the input buf and input-overflow buf
        memset((char *)obuf,0,obuflen * 2 * sizeof(float));         //  Zero the output buf and output-overflow buf

        while(dz->samps_left > 0) {
            if(firstread) {
                ibuf = dz->sampbuf[0];                              //  On first read, fill the buffer AND the read-ahead buffer
                dz->buflen *= 2;
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                dz->buflen /= 2;                                    //  After initial read, restore true length of buffer
                firstread = 0;
            } else {
                ibuf = dz->sampbuf[1];                              //  On subsequent reads, read into the read_ahead
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                ibuf = dz->sampbuf[0];                              //  but write will be from input-buffer proper
            }

            reread = 0;

            //  BEFORE CRUMBLE STARTS, JUST COPY INPUT TO (MULTICHAN) OUTPUT, "MONOREAD"

                    //  No need to read brktables here as we already have length of tail,
                    //  and all other possible timevarying params concern segmentation.
            
            while(samptime < monoreadend) {
    
                if(splicetime > 0) {                                    //  If in end splice (or exponential tail)
                    if(dz->hastail) {
                        val = (double)(dz->iparam[CRTAIL] - splicetime)/(double)dz->iparam[CRTAIL];
                        val *= val;                                     //  do exponential decay of tail
                    } else
                        val = (double)(dz->iparam[CRSPLICE] - splicetime)/(double)dz->iparam[CRSPLICE];
                    val *= ibuf[ipos];                                  //  or do normal splice
                } else
                    val = ibuf[ipos];                                   //  otherwise do straight copy
                for(chan = 0; chan < ochancnt; chan++)
                    obuf[opos++] = (float)val;                          //  Mono to multimono output
                ipos++;

                if(opos >= obuflen) {                                   //  If output buffer full
                    if(passno == 0) {
                        for(n=0;n<obuflen;n++)
                            maxsamp = max(maxsamp,fabs(obuf[n]));       //  On initial pass, find maxsamp
                        dz->process = BRASSAGE;                         //  (Force correct progress-bar display on Loom, based on samples READ)
                        display_virtual_time(dz->total_samps_read,dz);
                        dz->total_samps_written += obuflen;
                        dz->process = CRUMBLE;
                    } else {                                            //  On write-data pass,
                        if(normaliser < 1.0) {                          //  Do any required normalisation
                            for(n=0;n<obuflen;n++)
                                obuf[n] = (float)(obuf[n] * normaliser);
                        }
                        dz->process = BRASSAGE;                         //  and write to output
                        if((exit_status = write_samps(obuf,obuflen,dz))<0)
                            return(exit_status);
                        dz->process = CRUMBLE;
                    }
                    memset((char *)obuf,0,obuflen * sizeof(float));     //  Zero the output buf (there is no overflow at this stage)
                    opos -= obuflen;                                    //  Reset output buffer pointer
                }
                samptime++;
                splicetime++;
                if(ipos >= dz->buflen) {
                    reread = 1;                                         //  Whilst merely reading the input to the output
                    ipos = 0;                                           //  reset inbuf pointer to 0, and set flag to go read more samples.
                    break;
                }
            }
            if(reread) {                                                //  Until "starttime" is reached, continue monoread
                memcpy((char *)ibuf,(char *)iovbuf,dz->buflen * sizeof(float)); //  Copy the read-ahead buffer back into ibuf
                memset((char *)iovbuf,0,dz->buflen * sizeof(float));            //  Zero the read-ahead buf
                samples_before_bufstart += dz->buflen;                  //  Update count of samples read before buffer-start (used to calculate "time")
                continue;
            } else
                monoread_completed++;

            //  AT END OF MONO->MULTIMONO READ

            //  IF THE MULTIMONO HAS A TAIL, RECONFIGURE THE READ INFORMATION

            if(monoread_completed == 1) {

                if(dz->hastail) {                           //  Seek (back) to start of tail        
                    if((sndseekEx(dz->ifd[0],crstartsamps,0) < 0)){
                        sprintf(errstr,"sndseek (back to start of segmentation) failed in input file.\n");
                        return SYSTEM_ERROR;
                    }
                    ibuf = dz->sampbuf[0];                  //  Read a fresh DOUBLE buffer
                    dz->buflen *= 2;                        //  (first zeroing the input & input-overflow bufs).
                    memset((char *)ibuf,0,dz->buflen * sizeof(float));
                    if((exit_status = read_samps(ibuf,dz))<0)
                        return(exit_status);
                    dz->buflen /= 2;                        //  Reset for subsequent reads to be single bufreads

                    //  NB Subsequent reads will be to the READ-AHEAD buffer sampbuf[1], but this is set/unset at top of read loop                                                      

                    samples_before_bufstart = crstartsamps; //  Reset the input read counters.
                    time = (double)samples_before_bufstart/srate;
                    if(dz->iparam[CRDUR] > 0 && crstartsamps >= dz->iparam[CRDUR]) {
                        curtailed = 1;
                        break;
                    }
                    dz->total_samps_read = samples_before_bufstart + dz->ssampsread;
                    dz->samps_left = dz->insams[0] - dz->total_samps_read;
                    ipos = 0;                               //  Reposition the input buffer pointer.
                    opos = crstartsamps * ochancnt;
                    opos -= dz->total_samps_written;

                    if(opos < 0) {                          //  Should be rendant if buffer-szie calculations are correct!!!
                        sprintf(errstr,"OBUF UNDERFLOW\n");
                        return PROGRAM_ERROR;
                    }

                    monoreadend = -1;                       //  Ensure we're not trapped forever in monoreadings.

                //  OTHERWISE, SET THE TIME TO END OF MONOREAD
                
                } else
                    time = dz->param[CRSTART];
            }
                                                        //  sampsread could be a less-than-full SINGLE buffer: exit loop once samples exhausted
            buffer_end = min(dz->ssampsread,dz->buflen);//  or a full DOUBLE buffer (read-ahead): exit loop once pointer falls beyond SINGLE buffer

            write_end = opos;                           //  Latest sample written to output buffer is at opos

            //  ONLY ON CROSSING SECTOR BOUNDARIES, AND ONLY BEFORE WE REACH THE LASTSECTOR

            //  Crossing a sector boundary halves the channel-grouping in the output.
            //  For an 8-chan output
            //  Initial output is to 01234567: crossing 1st boundary puts output on 0123 & 4567
            //  (these groupings may be offset, by "orient", but each still has 4 adjacent chans).
            //  Hence we permute 2 sets of 4chans
            //  Crossing 2nd boundary puts output on 01 & 23 & 45 & 67 (possibly offset etc)
            //  Hence we permute 4 sets of 2chans
            //  Crossing 3rd boundary puts output on 0 & 1 & 2 & 3 & 4 & 5 & 6 & 7
            //  Hence we permute 8 individual channels


            //  END OF SECTOR BOUNDARY CALCULATIONS

            //  READING AND WRITING CUT SEGMENTS

            while(ipos < buffer_end) {

                if(sector < lastsector) {                   //  Until completely fragmented to separate chans
                    if(time >= boundary[sector]) {          //  Check if we've crossed time-boundary of next sector
                        sector++;                           //  Set up params to perm the groups-of-chans OR (in last sector) individual channels.
                        permsize = (int)pow(2,sector);      //  Sector 1 has 2 groups of (4)chans, sector 2 has 4 groups of (2)chans etc.
                        gpsize = ochancnt/permsize;         //  no of channels in each group.
                                                            //  Modify how the perm works, as number of perm items increases ....
                        lastperm = thisperm * 2;            //  e.g. with gpsize 4, if lastperm=1 last-used chan-gp-of4 contained channels 4567 (possibly offset etc).
                        lastperm2 = lastperm + 1;           //  After halving gpsize to 2, lastperm new gets "2" (chans 45) and lastperm2 gets "3" (chans 67).
                                                            //  e.g. with gpsize 2, if lastperm = 1 last-used chan-gp-of-2 contained channel 23.
                                                            //  after halving gpsize to 1, lastperm gets "2" (chan 2) and lastperm2 gets "3" (chans 3).
                                                            //  This ensures, that the new (larger) perm avoids using-first the channels used-last in previous-perm.
                                                            //  See the function "doperm" for more detials of how this works.

                        if(permsize==2)                     //  Where there are only 2 groups,
                            permcnt = !permcnt;             //   alternate between groups (0 and 1)
                        else                                //  otherwise perm the order of the groups, or the individual chans.
                            doperm(perm[permsize],permsize,&lastperm,&lastperm2);
                        permcnt = 0;                        //  Initialise the count of perm-items used.
                    }
                }

                if((exit_status= read_values_from_all_existing_brktables(time,dz))< 0)
                    return exit_status;

                //  Find size of tail, if any

                if(!dz->notailanywhere) {
                    if(dz->brksize[CRTAIL]) {
                        if(dz->param[CRTAIL] == 0.0)
                            dz->hastail = 0;
                        else {
                            dz->hastail = 1;
                            dz->param[CRTAIL]  = max((dz->application->lo[CRSPLICE] * 2),dz->param[CRTAIL]);
                            dz->param[CRTAIL] *= MS_TO_SECS;
                            dz->iparam[CRTAIL] = (int)ceil(dz->param[CRTAIL] * srate);
                        }
                    }
                }

                //  Find size of segment to cut, and initial values of subsequent step in infile and outfile

                crsize = dz->param[CRSIZE];

                if(dz->param[CRRAND] > 0.0) {
                    rval = drand48() * 0.5;         //  Range -1/2 to +1/2
                    rval *= dz->param[CRRAND];      //  Maxrange -1/2 to +1/2
                    rval += 1.0;                    //  Maxrange 1/2 to 3/2
                    crsize *= rval;
                }
                segsamps = (int)round(crsize * srate);                  //  samps to read
                instep  = segsamps;                                     //  step to next sample to read (before any randomisation)
                outstep = segsamps;                                     //  step (in gp-samples) to next sample to write (before any randomisation)

                //  Find transposition (and corresponding read-incr) of segment

                pshift = ((drand48() * 2.0) - 1.0) * dz->param[CRPSCAT];//  Randomly scatter pitch-shift (+- pshift range)
                incr = pow(2.0,-pshift/SEMITONES_PER_OCTAVE);           //  If transpos is +ve, pow(2,-ve) < 1 : if transpos is -ve, pos(2,+ve) is > 1
  
                if(dz->hastail) {
                    thistail = dz->iparam[CRTAIL];
                    if(pshift != 0.0)                                   //  If transposing down (incr +ve) output is inter, so tail would be lengthened
                        thistail  = (int)round((double)thistail/incr);  //  To retain tail of same length, read LESS input samples, to make tail correct length
                    segsamps += thistail;                               //  Increase the number of input samples to read, to include tail samples
                }
                
                //  Write (transposed) input segment into enveloping buffer, and note its length

                iposd = ipos;
                epos = 0;
                segend = ipos + segsamps;
                while(iposd < segend) {
                    lo = (int)floor(iposd);
                    frac = iposd - (double)lo;          
                    hi = lo + 1;
                    diff  = ibuf[hi] - ibuf[lo];
                    val = ibuf[lo] + (diff * frac);
                    ebuf[epos++] = (float)val;
                    iposd += incr;
                }
                oseglen = epos;
                eposend = oseglen-1;

                if(dz->warned && dz->hastail) {                         //  IF notailanywhere, value of CRTAIL has been preset to CRSPLICE
                    if(oseglen <= dz->iparam[CRSPLICE] + dz->iparam[CRTAIL]) {
                        if(dz->notailanywhere)
                            sprintf(errstr,"SEGMENT TOO SHORT FOR SPLICES.\n");
                        else
                            sprintf(errstr,"SEGMENT (%lf) TOO SHORT FOR SPLICE AND TAIL (%lf)\n",
                            (double)oseglen/srate,(double)(dz->iparam[CRSPLICE] + dz->iparam[CRTAIL])/srate);
                        return DATA_ERROR;
                    }
                }

                //  splice both ends of segment

                if(dz->hastail) {
                    for(n=0;n<dz->iparam[CRSPLICE];n++) {
                        spliceval = (double)n/(double)dz->iparam[CRSPLICE];
                        ebuf[n] = (float)(ebuf[n] * spliceval);
                    }
                    for(n=0,m=eposend;n<dz->iparam[CRTAIL];n++,m--) {
                        spliceval = (double)n/(double)dz->iparam[CRTAIL];
                        spliceval *= spliceval;                                         //  Exponential fade
                        ebuf[m] = (float)(ebuf[m] * spliceval);
                    }
                } else {
                    for(n=0,m=eposend;n<dz->iparam[CRSPLICE];n++,m--) {
                        spliceval = (double)n/(double)dz->iparam[CRSPLICE];
                        ebuf[n] = (float)(ebuf[n] * spliceval);
                        ebuf[m] = (float)(ebuf[m] * spliceval);
                    }
                }

                segstart_opos = opos;

                //  ADD INTO THE OUTPUT BUFFER, USING PERMUTATION OF OUTPUTA

                //  the goal chans are set by reading from the permutation of chans or chan-groups in "perm", counted by "permcnt"
                
                if(sector < lastsector) {                                               

                //  Writing to GROUPS of channels
                    
                    thisperm = perm[permsize][permcnt];                                 //  (thisperm * gpsize) gives lowest channel of current group
                    startchan = (dz->iparam[CRORIENT] + (thisperm * gpsize))%ochancnt;  //  with CRORIENT adding offset to start of group.
                    thischan  = startchan;                                          
                    for(n=0;n<oseglen;n++) {
                        for(chan = 0;chan < gpsize;chan++) {                            //  Write to all channels in channel-group (all adjacent)
                            ochan = opos+thischan;                                      //  amplifying level by appropriate "balance" param
                            obuf[ochan] = (float)(obuf[ochan] + (ebuf[n] * balance[sector]));
                            thischan++;                                                 //  going upwards round adjacent channels in group
                            thischan = thischan % ochancnt;
                        }
                        thischan = startchan;                                           //  Once one sample written to all output chans
                        opos += ochancnt;                                               //  reset start channel of group
                    }                                                                   //  and advance over ALL output-chans in output buffer
                } else {

                //  Writing to INDIVIDUAL channels
                    
                    thischan = perm[permsize][permcnt];                                 //  Write to channel selected by current perm
                    for(n=0;n<oseglen;n++) {
                        ochan = opos+thischan;      
                        obuf[ochan] = (float)(obuf[ochan] + (ebuf[n] * balance[sector]));
                        opos += ochancnt;
                    }
                }                                                                       //  Due to contraction/expansion of segments by pitch-scattering
                write_end = max(write_end,opos);                                        //  end of current output may not beyond end of previous writes
    
                if(permsize == 2)
                    permcnt = !permcnt;
                else if(++permcnt >= permsize) {                                        //  Once current perm of channels exhausted, reperm
                    doperm(perm[permsize],permsize,&lastperm,&lastperm2);
                    permcnt = 0;
                }
                
                //  GET PARAMETERS FOR NEXT SEGMENT

                //  If output write time is scattered, do scattering

                if(dz->param[CROSCAT] > 0.0) {
                    rval = drand48() * dz->param[CROSCAT];  //  0 to R
                    rval = 1.0 - rval;                      //  Range 1 to 1-R
                    outstep = (int)round((double)outstep * rval);
                }

                //  If output write time is stretched, do time-stretch

                outstep = (int)round((double)outstep * dz->param[CROSTR]);

                //  outstep MUST advance 

                if(outstep == 0)
                    outstep++;

                //  Convert to count in output samples      
                
                outstep *= ochancnt;
    
                //  Advance to next write position

                opos = segstart_opos + outstep;

                //  If next write will start beyond end of obuf, write outbuf, (Recursively in case there's a silent gap)

                if((dz->iparam[CRDUR] > 0) && (opos + dz->total_samps_written >= dz->iparam[CRDUR])) {
                    curtailed = 1;
                    break;
                }
                while(opos >= obuflen) {                                            
                    dz->process = BRASSAGE; //  progress-bar set in terms of READ-samples, rather than written-samples.
                    if(passno == 0) {
                        for(n=0;n<obuflen;n++)
                            maxsamp = max(maxsamp,fabs(obuf[n]));
                        display_virtual_time(dz->total_samps_read,dz);
                        dz->total_samps_written += obuflen;
                    } else {
                        if(normaliser < 1.0) {
                            for(n=0;n<obuflen;n++)
                                obuf[n] = (float)(obuf[n] * normaliser);
                        }
                        if((exit_status = write_samps(obuf,obuflen,dz))<0)
                            return(exit_status);
                    }
                    dz->process = CRUMBLE;
                    memcpy((char *)obuf,(char *)oovbuf,obuflen * sizeof(float));    //  Copy any overflow back into obuf
                    memset((char *)oovbuf,0,obuflen * sizeof(float));               //  Zero the output-overflow buf
                    write_end -= obuflen;                                           //  Reset buffer pointers
                    opos -= obuflen;
                }

                //  If ness, modify time where next input segment is cut

                if(dz->param[CRISCAT] > 0.0) {              //  If time of input segment is scattered in time
                    rval = drand48() * dz->param[CRISCAT];  //  Range 0 to +R
                    rval = 1.0 - rval;                      //  Range 1 to 1-R
                    instep = (int)round((double)instep * rval);
                }

                //  Advance to next input-read position

                ipos += instep;

                time = (double)((dz->total_samps_written+opos)/ochancnt)/srate;
            }
            if(curtailed)
                break;
            //  Once next-read-position is beyond end of input buffer, readjust buffers and counters before next read

            memcpy((char *)ibuf,(char *)iovbuf,dz->buflen * sizeof(float)); //  Copy the read-ahead buffer back into ibuf
            memset((char *)iovbuf,0,dz->buflen * sizeof(float));            //  Zero the input-overflow buf
            samples_before_bufstart += buffer_end;
            ipos -= buffer_end;

        }

        //  Once input file is exhausted, write any samples remaining in output buffer

        if(write_end > 0) {
            dz->process = BRASSAGE;
            if(passno == 0) {
                for(n=0;n<write_end;n++)
                    maxsamp = max(maxsamp,fabs(obuf[n]));
                display_virtual_time(dz->insams[0],dz);
                dz->total_samps_written += obuflen;
            } else {
                if(normaliser < 1.0) {
                    for(n=0;n<write_end;n++)
                        obuf[n] = (float)(obuf[n] * normaliser);
                }
                if(curtailed) {
                    endsplice = write_end/ochancnt;
                    if(dz->hastail) {
                        endsplice = min(endsplice,dz->iparam[CRTAIL]);
                        for(n = 0,m = write_end-ochancnt;n < endsplice;n++,m-=ochancnt) {
                            val = (double)n/(double)endsplice;
                            val *= val;
                            for(chan = 0; chan < ochancnt; chan++)
                                obuf[m+chan] = (float)(obuf[m+chan] * val);
                        }
                    } else {
                        endsplice = min(endsplice,dz->iparam[CRSPLICE]);
                        for(n = 0,m = write_end-ochancnt;n < endsplice;n++,m-=ochancnt) {
                            val = (double)n/(double)endsplice;
                            for(chan = 0; chan < ochancnt; chan++)
                                obuf[m+chan] = (float)(obuf[m+chan] * val);
                        }
                    }
                }
                if((exit_status = write_samps(obuf,write_end,dz))<0)
                    return(exit_status);
            }
            dz->process = CRUMBLE;
        }
        if(passno == 0) {
            if(maxsamp > 0.95)
                normaliser = 0.95/maxsamp;
            else if(maxsamp <= FLTERR) {
                sprintf(errstr,"No significant signal found in source file.\n");
                return DATA_ERROR;
            }
        }
    }
    return FINISHED;
}

/********************************** CHECK_CRUMBLE_PARAM_VALIDITY_AND_CONSISTENCY ********************/
/* TW fixes Jan 2023 */
int check_crumble_param_validity_and_consistency(dataptr dz)
{
    #define maxrandlenincr  1.5        //    1.5 is max random length increase
    #define maxrandlenshrnk 0.5        //    0.5 is max random length decrease
    int exit_status, ochans;
    double processtime, srate = (double)dz->infile->srate, frqratio;
    int minseglen, crstartsamps;
    if(dz->mode == 1)
        ochans = 16;
    else
        ochans = 8;
    dz->param[CRSPLICE] *= MS_TO_SECS;
    dz->iparam[CRSPLICE] = (int)ceil(dz->param[CRSPLICE] * srate);
    crstartsamps =  (int)ceil(dz->param[CRSTART] * srate);
    if(dz->brksize[CRTAIL]) {
        if((exit_status = read_value_from_brktable(dz->param[CRSTART],CRTAIL,dz))<0)
            return exit_status;
    }
    if(dz->param[CRTAIL] > 0.0) {
        dz->param[CRTAIL] = max(dz->param[CRTAIL],dz->application->lo[CRSPLICE] * 2.0);
        if(dz->iparam[CRTAIL] + crstartsamps >= dz->insams[0]) {
            sprintf(errstr,"Exponetial tail too long with the start segment.\n");
            return DATA_ERROR;
        }
    }
    dz->hastail = 1;
    dz->notailanywhere = 0;
    if(dz->brksize[CRTAIL]) {
        if((exit_status = get_maxvalue_in_brktable(&(dz->param[CRTAIL]),CRTAIL,dz))<0)
            return exit_status;
    } 
    if(dz->param[CRTAIL] > 0.0) {                                                           //  Working with MAX specified tail-length ...
        dz->param[CRTAIL]  = max(dz->param[CRTAIL],dz->application->lo[CRSPLICE] * 2.0);    //  ensure tail is set at at-least double minsplicelen
        dz->iparam[CRTAIL] = (int)ceil(dz->param[CRTAIL] * MS_TO_SECS * srate);         //  to compensate for tail decay being exponential
    } else {
        dz->hastail = 0;
        dz->notailanywhere = 1;
        dz->iparam[CRTAIL] = dz->iparam[CRSPLICE];
    }
    processtime = dz->param[CRSTART] + dz->param[CRSTEP1] + dz->param[CRSTEP2];
    if(dz->mode==1)
        processtime += dz->param[CRSTEP3];
    if(processtime >= dz->duration) {
        sprintf(errstr,"Sum of durations of splitting processes extends beyond end of input file.\n");
        return DATA_ERROR;
    }
    if(dz->brksize[CRRAND]) {
        if((exit_status = get_maxvalue_in_brktable(&(dz->param[CRRAND]),CRRAND,dz))<0)
            return exit_status;
    }
    
    if(dz->brksize[CRSIZE]) {
        if((exit_status = get_maxvalue_in_brktable(&(dz->param[CRSIZE]),CRSIZE,dz))<0)
            return exit_status;
    }
    if(dz->brksize[CRPSCAT]) {
        if((exit_status = get_maxvalue_in_brktable(&(dz->param[CRPSCAT]),CRPSCAT,dz))<0)
            return exit_status;
    }
    frqratio = pow(2.0,dz->param[CRPSCAT]/SEMITONES_PER_OCTAVE);        //  transpose max down, increases seglen by +ve frq ratio

    dz->maxinseg  = (int)ceil(dz->param[CRSIZE] * frqratio * srate * maxrandlenincr);
    if(dz->hastail) {
        dz->maxinseg += (int)ceil(dz->iparam[CRTAIL] * frqratio);       //  CRTAIL holds maxvalue, CRPSCAT minvalue, at present
    }
    dz->maxinseg *= 2;                                                  //  LARGE SAFETY MARGIN!!
    dz->maxoutseg = dz->maxinseg * ochans;                              //  max-segsize in outputbuf
    
    if(dz->brksize[CRSIZE]) {
        if((exit_status = get_minvalue_in_brktable(&(dz->param[CRSIZE]),CRSIZE,dz))<0)
            return exit_status;
    }                                                                   //  min-segsize in inputbuf
    minseglen =  (int)ceil((dz->param[CRSIZE]/frqratio) * srate * maxrandlenshrnk);
    dz->warned = 0;
    if(minseglen <= dz->iparam[CRSPLICE] + dz->iparam[CRTAIL]) {        //  NB, CRSPLICE is fixed and CRTAIL has been set at its maxvalue
        dz->warned = 1;
        fprintf(stdout,"WARNING: Minimum segment length (after any transpositions) may be too short for splices (and any tail).\n");
        fflush(stdout);
    }
    if(dz->param[CRDUR] > 0.0 && dz->param[CRDUR] < dz->param[CRSTART]) {
        fprintf(stdout,"WARNING: Terminate time (%lf) set before before disintegration begins (%lf).\n",dz->param[CRDUR],dz->param[CRSTART]);
        fflush(stdout);
    } else {
        dz->iparam[CRDUR] = (int)round(dz->param[CRDUR] * srate);
        dz->iparam[CRDUR] *= ochans;
    }
    dz->iparam[CRORIENT]--;     //  Change from 1-8 to 0-7 numbering (1-16, to 0-15)
    return FINISHED;
}

/******************************** CREATE_CRUMBLE_SNDBUFS *****************/

int create_crumble_sndbufs(dataptr dz)
{
    int bigbufsize, framesize, orig_framesize, frameunit, output_framesize, orig_output_framesize, outbufsize, outchans;
    if(dz->sbufptr == 0 || dz->sampbuf==0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->mode == 1)
        outchans = 16;
    else
        outchans = 8;
    frameunit = outchans + MONO;                    //  frame must be multiple of changroupsize of output & input;
    orig_framesize = F_SECSIZE * frameunit;         //  frame must also be a multiple of sectorsize
    framesize = orig_framesize;
    orig_output_framesize = F_SECSIZE * outchans;   //  Outputbuffer (only) framesize
    output_framesize = orig_output_framesize;
    while(output_framesize <= dz->maxoutseg) {      //  outputbuffer must accomodate the maximum-size of segment to be written
        output_framesize += orig_output_framesize;
        framesize        += orig_framesize;
    }
    framesize *= 2;                                 //  frame must accomodate double buffer for input, and for output
    frameunit *= 2;
    bigbufsize = (int)(size_t) Malloc(-1);
    dz->buflen = bigbufsize / sizeof(float);        //  Ensure buffer contains an integer number of frames
    dz->buflen = (dz->buflen / framesize)  * framesize;
    bigbufsize = dz->buflen * sizeof(float);
    if(bigbufsize <= 0) {
        dz->buflen = framesize;
        bigbufsize = dz->buflen * sizeof(float);
    }
    dz->buflen = dz->buflen/frameunit;              //  Get TRUE (input) buffer size
    outbufsize = dz->buflen * outchans;             //  Get output buffer size
    bigbufsize = ((dz->buflen * 2) + (outbufsize * 3)) * sizeof(float);
    /* 2023 */
    bigbufsize += dz->maxinseg * sizeof(float);     //  add in memory for segment-enveloping buffer
    if(bigbufsize <= 0) {
        sprintf(errstr,"TOO MUCH MEMORY REQUIRED for sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;                   // input
    dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;  // input overflow
    dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;  // output
    dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + outbufsize;  // output overflow
    dz->sbufptr[4] = dz->sampbuf[4] = dz->sampbuf[3] + outbufsize;  // enveloping buffer
    dz->sampbuf[5] = dz->sampbuf[4] + outbufsize + dz->maxinseg;    //2023
    return(FINISHED);
}

/*************************** PERMUTE_CHUNKS ***************************
 *
 * "lastperm2" is only set to a non-negative value when we cross a sector boundary.
 * At that point there are 2 previous channels or channel-groups to avoid when selecting next chan or group.
 * Once we are in lastsector (i.e. sending sigs to individual channels)
 * lastperm stays stuck at -1 and does not affect the perm calculations.
 */

void doperm(int *perm,int permsize,int *lastperm,int *lastperm2)
{   
    int n, t;
    if(*lastperm2 >= 0) {                                       //  If we've just changed sector, need to check 1st TWO perm items
                                                                //  are not in any of the channels in the previous chan-group used.
        do {                                                    
            for(n=0;n<permsize;n++) {
                t = (int)(drand48() * (double)(n+1));    /* TRUNCATE */
                if(t==n)
                    prefix(perm,permsize,n);
                else
                    insert(perm,permsize,n,t);
            }                                                   //  Avoid 1st * 2nd item in perm equaling last item-pair in previous perm
        } while(perm[0] == *lastperm || perm[0] == *lastperm2 || perm[1] == *lastperm || perm[1] == *lastperm2);
        *lastperm = perm[permsize-1];                           //  But once first perm generated, we only need to avoid 1 chan(gp).
        *lastperm2 = -1;                                        //  So set lastperm2 to an impossible val, which causes 2nd do loop to be called, instead.

    } else {

        do {                                                    //  Comparing same-grouping-size perm. Only need to check ONE perm item.
            for(n=0;n<permsize;n++) {
                t = (int)(drand48() * (double)(n+1));    /* TRUNCATE */
                if(t==n)
                    prefix(perm,permsize,n);
                else
                    insert(perm,permsize,n,t);
            }
        } while(perm[0] == *lastperm);                          //  Avoid 1st item in next perm equaling last item in previous perm
    
    }
}

/****************************** INSERT ****************************/

void insert(int *perm,int permsize,int n,int t)
{   
    shuflup(perm,permsize,t+1);
    perm[t+1] = n;
}

/****************************** PREFIX ****************************/

void prefix(int *perm,int permsize,int n)
{   
    shuflup(perm,permsize,0);
    perm[0] = n;
}

/****************************** SHUFLUP ****************************/

void shuflup(int *perm,int permsize,int k)
{   
    int n;
    for(n = permsize-1; n > k; n--)
        perm[n] = perm[n-1];
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

