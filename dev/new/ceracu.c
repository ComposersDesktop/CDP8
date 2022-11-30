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

#define SIGNAL_TO_LEFT  (0)
#define SIGNAL_TO_RIGHT (1)
#define ROOT2       (1.4142136)
#define dupl descriptor_samps

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.0";

//CDP LIB REPLACEMENTS
static int setup_ceracu_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_ceracu_param_ranges_and_defaults(dataptr dz);
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
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int handle_the_special_data(char *str,dataptr dz);
static int create_ceracu_sndbufs(dataptr dz);
static int ceracu(dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static int get_lcm(int arraycnt,int *lcm,dataptr dz);
static int ceracu_param_preprocess(dataptr dz);

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
//      if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
//          print_messages_and_close_sndfiles(exit_status,is_launched,dz);
//          return(exit_status);
//      }
//      cmdline++;
//      cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_ceracu_application(dz))<0) {
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
    //ap = dz->application;

    // parse_infile_and_hone_type() = 
    if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_ceracu_param_ranges_and_defaults(dz))<0) {
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
    if((exit_status = open_the_outfile(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency()  redundant
    is_launched = TRUE;

    if((exit_status = create_ceracu_sndbufs(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = ceracu_param_preprocess(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //spec_process_file =
    if((exit_status = ceracu(dz))<0) {
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
    char *filename = (*cmdline)[0], *p;
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
    p = filename;                   //  Drop file extension
    while(*p != ENDOFSTR) {
        if(*p == '.') {
            *p = ENDOFSTR;
            break;
        }
        p++;
    }
    strcpy(dz->outfilename,filename);
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

/************************* SETUP_CERACU_APPLICATION *******************/

int setup_ceracu_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,CYCLECNTS,5,5,"diddi"))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"",0,"","ol",2,0,"00"))<0)
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

/************************* SETUP_CERACU_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_ceracu_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[CER_MINDUR]  = 0.0;
    ap->hi[CER_MINDUR]  = dz->duration * 32;
    ap->default_val[CER_MINDUR] = dz->duration;
    ap->lo[CER_OCHANS]  = 1;
    ap->hi[CER_OCHANS]  = 16;
    ap->default_val[CER_OCHANS] = 8;
    ap->lo[CER_CUTOFF]  = 0.0;
    ap->hi[CER_CUTOFF]  = 3600;
    ap->default_val[CER_CUTOFF] = 60;
    ap->lo[CER_DELAY]   = 0.0;
    ap->hi[CER_DELAY]   = dz->duration * 16;
    ap->default_val[CER_DELAY] = 0;
    ap->lo[CER_DSTEP]   = -16;
    ap->hi[CER_DSTEP]   = 16;
    ap->default_val[CER_DSTEP] = 1;
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
            if((exit_status = setup_ceracu_application(dz))<0)
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
    usage2("ceracu");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"ceracu"))              dz->process = CERACU;
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
    if(!strcmp(str,"ceracu")) {
        fprintf(stdout,
        "USAGE:\n"
        "ceracu ceracu inf outf cyclcnts mincycdur chans outdur echo echshift [-o] [-l]\n"
        "\n"
        "Repeat src in several cyclestreams that resynchronise after specified counts.\n"
        "One complete pass is a \"resync-cycle\". e.g. for 10 12 15\n"
        "Source repeat 10,12 and 15 times before cyclestreams resynchronise ....\n"
        "IF source sound is \"A\", cyclestreams would be ....\n"
        "A     A     A     A     A     A     A     A     A     A     A (10 times)\n"
        "A    A    A    A    A    A    A    A    A    A    A    A    A (12 times)\n"
        "A   A   A   A   A   A   A   A   A   A   A   A   A   A   A   A (15 times)\n"
        "|--------------------------Resync cycle --------------------|\n"
        "|---| = Mincycdur (see below)\n"
        "\n"
        "CYCLECNTS A list of integers, being\n"
        "          the number of repeats in each cyclestream before the streams resync.\n"
        "MINCYCDUR Time before the first repeat in the fastest cyclestream.\n"
        "          If set to ZERO, assumed to be the duration of the input sound.\n"
        "CHANS:    Number of channels in output. (Not ness same as number of cyclestreams)\n"
        "OUTDUR    Duration of output. (If set to ZERO, outputs a single resync-cycle).\n"
        "          Process always outputs a WHOLE NUMBER of complete resync-cycles,\n"
        "          equal to or greater than the specified output duration.\n"
        "          If true duration > 1 hour, sound curtailed, unless \"-o\" flag set.\n"
        "ECHO      Single-echo-delay of entire output, in secs (Set to zero for no echo).\n"
        "ECHSHIFT  Spatial offset of echo-delay (an integer value). (Ignored if no echo)\n"
        "          1 = 1 chan to right; 2 = 2 chans to right; -1 = 1 channel to left; etc\n"
        "-o        Override duration restriction, to produce all resync-cycles (CARE!).\n"
        "-l        Output channels arranged linearly (default: arranged in a circle).\n"
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

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
    int n, k, cnt, *cyclen;
    FILE *fp;
    char temp[200], *p;
    double dummy;
    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read cyclecounts.\n",str);
        return(DATA_ERROR);
    }
    cnt = 0;
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy)) {
            if(dummy < 1.0) {
                sprintf(errstr,"Invalid cyclecnt (%.1lf) (must be >=1)\n",dummy);
                return(DATA_ERROR);
            }
            cnt++;
        }
    }
    if(cnt < 2) {
        sprintf(errstr,"Must be more than 1 cycle value\n");
        return(DATA_ERROR);
    }
    dz->itemcnt = cnt;

/*********************************************
 *
 *  dz->iparray[0] = cyclecnts (eventually as samplecnts)
 *  dz->iparray[1] = Array to determine lcm of samplecnts, and as temp store of cyclecnts
 *  dz->iparray[2] = leftmost channel for output for each cycle
 *  dz->iparray[3] = rightmost channel for output for each cycle
 *  dz->iparray[4] = pointers associated with cycles
 */
        
    if((dz->iparray = (int **)malloc(5 * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create integer arrays.\n");
        return(MEMORY_ERROR);
    }
    for(n = 0; n < 4;n++) {     //  5th array established later
        if((dz->iparray[n] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create integer array %d.\n",n+1);
            return(MEMORY_ERROR);
        }
    }
    cyclen = dz->iparray[0];
    cnt = 0;
    fseek(fp,0,0);
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy)) {
            cyclen[cnt] = (int)round(dummy);
            cnt++;
        }
    }
    for(cnt = 0;cnt < dz->itemcnt-1;cnt++) {
        for(n=cnt+1;n<dz->itemcnt;n++) {
            if(cyclen[cnt] == cyclen[n]) {
                sprintf(errstr,"Not all cycle counts (which must be whole numbers) are distinct (e.g. %d)\n",cyclen[n]);
                return(DATA_ERROR);
            } else if(cyclen[cnt] > cyclen[n]) {
                k = cyclen[cnt];
                cyclen[cnt] = cyclen[n];
                cyclen[n] = k;
            } 
        }
    }
//  Array of position of left and right levels for each cycle
    if((dz->parray = (double **)malloc(2 * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create double arrays.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create array for output positions.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[1] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create array for output positions.\n");
        return(MEMORY_ERROR);
    }
    return FINISHED;
}

/**************************** CERACU_PARAM_PREPROCESS ****************************/

int ceracu_param_preprocess(dataptr dz)
{
    int exit_status, chans = dz->iparam[CER_OCHANS];
    double srate = (double)dz->infile->srate, *pos = dz->parray[0], *llev = dz->parray[0], *rlev = dz->parray[1];
    double leftgain, rightgain;
    int *cyclen = dz->iparray[0], *lmost = dz->iparray[2], *rmost = dz->iparray[3], done_warning = 0, n;
    int totallen, lcm_of_cycles;
    int maxcyclecnt, ptrcnt;

    while(dz->iparam[CER_DSTEP] < 0)
        dz->iparam[CER_DSTEP] = dz->iparam[CER_OCHANS] - dz->iparam[CER_DSTEP];
    dz->iparam[CER_DSTEP] %= dz->iparam[CER_OCHANS];
    dz->iparam[CER_DELAY]  = (int)round(dz->param[CER_DELAY] * srate);
    dz->iparam[CER_CUTOFF] = (int)round(dz->param[CER_CUTOFF] * srate);
    dz->iparam[CER_MINDUR] = (int)round(dz->param[CER_MINDUR] * srate); //  Number of samples in the minimum repeat-duration
    if(dz->iparam[CER_MINDUR] == 0)                                     //  If set to zero, assumed to be length of input sound
        dz->iparam[CER_MINDUR] = dz->insams[0];

    //  Calculate cycle-lengths for each cycle

    maxcyclecnt = cyclen[dz->itemcnt - 1];                              //  Item repeated most has least repeat-duration, to fit into sync-cycle
    totallen = maxcyclecnt * dz->iparam[CER_MINDUR];                    //  So, initially, totallen = maxcyclecnt * min cyclelen in samples
    if((exit_status = get_lcm(dz->itemcnt,&lcm_of_cycles,dz))<0)        //  Find lcm of all cycles EXCEPT max-cycle
        return exit_status;
    while((totallen/lcm_of_cycles) * lcm_of_cycles != totallen) {       //  Force totallen to be divisible by all cycles.
        totallen++;
        if(totallen < 0) {
            sprintf(errstr,"Output length has overflowed: Cannot proceed.\n");
            return(DATA_ERROR);
        }
        if(!done_warning) {
            if(dz->vflag[CER_OVERRIDE] && (totallen > 3600 * dz->infile->srate)) {
                fprintf(stdout,"WARNING: Output is longer than 1 hour\n");
                fflush(stdout);
                done_warning = 1;
            }
        }
    }
    for(n=0;n<dz->itemcnt;n++) {                                        //  Put adjusted length of cycles, in samples, in array
        if((cyclen[n] = totallen/cyclen[n]) < 0) {
            sprintf(errstr,"Output length for cycle %d has overflowed: Cannot proceed.\n",n+1);
            return(DATA_ERROR);
        }
    }
    if(dz->param[CER_DELAY] >= cyclen[dz->itemcnt-1]) {
        sprintf(errstr,"Delay time (%lf) must be less than Fastest Repeat Time (%lf).\n",dz->param[CER_DELAY],(double)cyclen[dz->itemcnt-1]/srate);
        return(DATA_ERROR);
    }
    dz->ringsize = totallen;
    if(dz->iparam[CER_CUTOFF] == 0)                                     //  If output duration set to zero
        dz->iparam[CER_CUTOFF] = dz->ringsize;                          //  Assumed to be length of ONE complete cycle
    ptrcnt = dz->itemcnt;
    dz->dupl = (int)ceil((double)dz->insams[0]/(double)cyclen[dz->itemcnt - 1]);
    if(dz->dupl > 1) 
        ptrcnt *= dz->dupl;
    if(dz->iparam[CER_DELAY] > 0)
        ptrcnt *= 2;
    if((dz->iparray[4] = (int *)malloc(ptrcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create cycle-pointers.\n");
        return(MEMORY_ERROR);
    }

    //  Assign output positions and levels for sounds

    if(dz->itemcnt == chans) {
        for(n = 0;n<dz->itemcnt;n++) {
            lmost[n] = n;
            rmost[n] = (lmost[n] + 1) % chans;
            llev[n] = 1.0;
            rlev[n] = 0.0;
        }
    } else {
        if (chans == 1) {
            for(n = 0;n<dz->itemcnt;n++) {
                lmost[n] = 0;
                rmost[n] = 0;
                llev[n] = 1.0;      //  Daft, but keeps same algo for all cases
                rlev[n] = 0.0;
            }
        } else {
            if(dz->vflag[CER_LINEAR] || chans == 2) {
                for(n = 0;n<dz->itemcnt;n++) {
                    pos[n] = ((chans - 1) * n)/(double)(dz->itemcnt - 1);
                    lmost[n] = (int)floor(pos[n]);
                    pos[n]  -= lmost[n]; 
                }
            } else {
                for(n = 0;n<dz->itemcnt;n++) {
                    pos[n] = (chans * n)/(double)dz->itemcnt;
                    lmost[n] = (int)floor(pos[n]);
                    pos[n]  -= lmost[n]; 
                }
            }
        }
        for(n = 0;n<dz->itemcnt;n++) {
            rmost[n] = (lmost[n] + 1) % chans;
            if(flteq(pos[n],0.0)) {
                rlev[n] = 0.0;
                llev[n] = 1.0;  //  pos values overwritten by associated level values (ETC below)
            } else if(flteq(pos[n],1.0)) {
                rlev[n] = 1.0;
                llev[n] = 0.0;
            } else {
                pos[n] *= 2.0;
                pos[n] -= 1.0;  //  Change position to -1 to +1 range
                pancalc(pos[n],&leftgain,&rightgain);
                rlev[n] = rightgain;
                llev[n] = leftgain;
            }
        }
    }
    return FINISHED;
}

/*************************** LCM **********************************/

int get_lcm(int arraycnt,int *lcm,dataptr dz)
{
    int *cycarray = dz->iparray[1];
    int origlcm;
    int n, m, k, div, OK;
    for(n = 0,m = arraycnt - 1; n < arraycnt; n++,m--)
        cycarray[m] = dz->iparray[0][n];            //  Copy array to descending order
    for(n = 0; n < arraycnt; n++) {                 //  Eliminate "1" values
        if(cycarray[n] == 1) {
            for(k = n+1; k < arraycnt;k++)          //  Eliminate value "1" from lcm search
                cycarray[k-1] = cycarray[k];
            arraycnt--;
            break;
        }
    }
    if(arraycnt == 1) {                             //  If only one value remains,
        *lcm = cycarray[0];                         //  This is the LCM
        return FINISHED;
    }
    for(n = 0; n < arraycnt - 1; n++) {             //  Eliminate exact multiples
        for(m = n+1; m < arraycnt; m++) {
            div = cycarray[n]/cycarray[m];
            if(div * cycarray[m] == cycarray[n]) {  //  Exact divisor
                for(k = m+1; k < arraycnt;k++)      //  Eliminate divisor
                    cycarray[k-1] = cycarray[k];
                arraycnt--;
                m--;
            }
        }
//       n++;  /* RWD ???? probably don't want this. */
    }
    if(arraycnt == 1) {                     //  If only one value remains,
        *lcm = cycarray[0];                 //  This is the LCM, as all other values (eliminated) were its divisors
        return FINISHED;
    }
    *lcm = cycarray[0] * cycarray[1];   //  If arraycnt = 2, this product is LCM, as these 2 values are not mutual divisors
    origlcm = *lcm;                     //  If arraycnt > 2, test this initial "lcm" against other remaining numbers in array
    if (arraycnt > 2) {
        OK = 0;
        while(!OK) {
            OK = 1;
            for(n=0;n < arraycnt;n++) {
                div = (*lcm)/cycarray[n];
                if(div * cycarray[n] == *lcm)   {   //  cycarray[n] is a divisor of "lcm"
                    for(k = n+1; k < arraycnt;k++)  //  Eliminate divisor cycarray[n] 
                        cycarray[k-1] = cycarray[k];//  As this will be divisor of this "lcm" and any multiple of it
                    origlcm = *lcm;                 //  This becomes the lcm so far
                    arraycnt--;
                    n--;
                } else {
                    OK = 0;             //  cycarray[n] is NOT a divisor of this "lcm"
                    break;
                }
            }
            if(!OK) {                   //  if there are still array components that do not divide the "lcm"
                *lcm += origlcm;            //  ascend through multiples of original "lcm" until a true lcm is found
                if(*lcm < 0) {
                    sprintf(errstr,"Numeric overflow in calculationg Lowest Common Multiple of cycle lengths.\n");
                    return(DATA_ERROR);
                }
            }
        }
    }
    return FINISHED;
}

/************************************ PANCALC *******************************/

void pancalc(double position,double *leftgain,double *rightgain)
{
    int dirflag;
    double temp;
    double relpos;
    double reldist, invsquare;

    if(position < 0.0)
        dirflag = SIGNAL_TO_LEFT;       /* signal on left */
    else
        dirflag = SIGNAL_TO_RIGHT;

    if(position < 0)
        relpos = -position;
    else 
        relpos = position;
    if(relpos <= 1.0){      /* between the speakers */
        temp = 1.0 + (relpos * relpos);
        reldist = ROOT2 / sqrt(temp);
        temp = (position + 1.0) / 2.0;
        *rightgain = temp * reldist;
        *leftgain = (1.0 - temp ) * reldist;
    } else {                /* outside the speakers */
        temp = (relpos * relpos) + 1.0;
        reldist  = sqrt(temp) / ROOT2;   /* relative distance to source */
        invsquare = 1.0 / (reldist * reldist);
        if(dirflag == SIGNAL_TO_LEFT){
            *leftgain = invsquare;
            *rightgain = 0.0;
        } else {   /* SIGNAL_TO_RIGHT */
            *rightgain = invsquare;
            *leftgain = 0;
        }
    }
}

/************************************ CERACU *******************************/

int ceracu(dataptr dz)
{
    int exit_status, chans = dz->iparam[CER_OCHANS], k, samps_left, insampcnt = dz->insams[0], mins = 0, hrs = 0;
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
    double maxsamp = 0.0, normaliser, srate = (double)dz->infile->srate, secs = 0.0;
    int n, nn, m, lmst, rmst, ptrcnt, samps_read, bas, opos;
    unsigned int outcnt, mintotalout;
    int *iptr = dz->iparray[4], *cyclen = dz->iparray[0], *orig_cyclen = dz->iparray[1], *lmost = dz->iparray[2], *rmost = dz->iparray[3];
    double *llev = dz->parray[0], *rlev = dz->parray[1];
    if((samps_read  = fgetfbufEx(dz->sampbuf[0], insampcnt,dz->ifd[0],0))<0) {
        sprintf(errstr,"Sound read error with input soundfile: %s\n",sferrstr());
        return(SYSTEM_ERROR);
    }
    memset((char *)obuf,0,dz->buflen * sizeof(float));
    mintotalout = (int)ceil((double)dz->iparam[CER_CUTOFF]/(double)dz->ringsize);   //  How many complete-resyncing-cycles begin within specified outdur
    mintotalout *= dz->ringsize;                                                    //  Outdur is a complete number of these c-r-cyles
    if(mintotalout < 0) {
        sprintf(errstr,"Projected output length produces numeric overflow : Cannot proceed.\n");
        return(DATA_ERROR);
    }
    if(!dz->vflag[CER_OVERRIDE] && (mintotalout > (unsigned int)(3600 * dz->infile->srate))) {
        sprintf(errstr,"Projected output is longer than 1 hour: terminating (use the \"Override\" flag to force longer output).\n");
        return(DATA_ERROR);
    }
    secs = (double)mintotalout/srate;
    mins = (int)floor(secs/60.0);
    secs -= (double)(mins * 60);
    hrs = mins/60;
    mins = mins - (hrs * 60);
    if(hrs)
        fprintf(stdout,"INFO: Output duration will be at least %d hrs %d mins and %.2lf secs\n",hrs,mins,secs);
    else if(mins)
        fprintf(stdout,"INFO: Output duration will be at least %d mins and %.2lf secs\n",mins,secs);
    else
        fprintf(stdout,"INFO: Output duration will be at least %.2lf secs\n",secs);
    secs = (double)cyclen[dz->itemcnt - 1]/srate;
    mins = (int)floor(secs/60.0);
    secs -= (double)(mins * 60);
    hrs = mins/60;
    mins = mins - (hrs * 60);
    if(hrs)
        fprintf(stdout,"INFO: Minimum cycle step will be %d hrs %d mins and %.2lf secs\n",hrs,mins,secs);
    else if(mins)
        fprintf(stdout,"INFO: Minimum cycle step will be %d mins and %.2lf secs\n",mins,secs);
    else
        fprintf(stdout,"INFO: Minimum cycle step will be %lf secs\n",secs);
    fprintf(stdout,"INFO: First pass: assessing level.\n");
    fflush(stdout);
    for(n = 0;n<dz->itemcnt;n++)
        iptr[n] = 0;
    if(dz->dupl > 1) {                          //  If sound overlaps itself in output
        for(k = 1;k < dz->dupl;k++) {           //  Every duplicated pointer is staggered a whole cycle before previous copy
            bas = dz->itemcnt * k;
            for(n = 0;n<dz->itemcnt;n++)
                iptr[n + bas] = -(cyclen[n] * k);
        }
        for(n = 0;n<dz->itemcnt;n++) {
            orig_cyclen[n] = cyclen[n];
            cyclen[n] *= dz->dupl;              //  And then each duplicated pointer only switches on once every dupl cycles
        }
    }
    ptrcnt = dz->itemcnt * dz->dupl;
    if(dz->iparam[CER_DELAY] > 0) {
        bas = ptrcnt;
        for(n = 0;n < ptrcnt;n++)
            iptr[n + bas] = iptr[n] - dz->iparam[CER_DELAY];
    }
    opos = 0;
    outcnt = 0;
    while(outcnt < mintotalout) {
        for(n = 0; n < ptrcnt; n++) {
            nn = n % dz->itemcnt;
            if(iptr[n] >= 0 && iptr[n] < insampcnt) {
                obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[iptr[n]] * llev[nn]));
                obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[iptr[n]] * rlev[nn]));
            } 
            iptr[n]++;
            if(iptr[n] >= cyclen[nn])
                iptr[n] = 0;
        }
        if(dz->iparam[CER_DELAY] > 0) {
            for(n = 0,m = ptrcnt; n < ptrcnt; n++,m++) {
                nn = n % dz->itemcnt;
                lmst = (lmost[nn] + dz->iparam[CER_DSTEP]) % chans;
                rmst = (lmst + 1) % chans;
                if(iptr[m] >= 0 && iptr[m] < insampcnt) {
                    obuf[opos + lmst] = (float)(obuf[opos + lmst] + (ibuf[iptr[m]] * llev[nn]));    //  Use levels in orig chans llev\rlev[nn],
                    obuf[opos + rmst] = (float)(obuf[opos + rmst] + (ibuf[iptr[m]] * rlev[nn]));    //  applying them to delayed copy in 
                }                                                                                   //  (possible different) chans lmst/rmst.
                iptr[m]++;
                if(iptr[m] >= cyclen[nn])
                    iptr[m] = 0;
            }
        }
        outcnt++;
        opos += chans;
        if(opos >= dz->buflen) {
            fprintf(stdout,"INFO: Level Check at %lf secs\n",(double)outcnt/srate);
            fflush(stdout);
            for(n=0;n < dz->buflen;n++)
                maxsamp = max(maxsamp,fabs(obuf[n]));
            memset((char *)obuf,0,dz->buflen * sizeof(float));
            opos = 0;
        }
    }
    samps_left = 1;
    while(samps_left) {     //  Flush remaining buffers
        samps_left = 0;
        for(n = 0; n < ptrcnt; n++) {
            nn = n % dz->itemcnt;
            if(iptr[n] > 0 && iptr[n] < insampcnt) {
                samps_left++;
                break;
            }
        }
        if(dz->iparam[CER_DELAY] > 0) {
            for(n = 0, m= ptrcnt; n < ptrcnt; n++,m++) {
                nn = n % dz->itemcnt;
                if(iptr[m] > 0 && iptr[m] < insampcnt) {
                    samps_left++;
                    break;
                }
            }
        }
        if(!samps_left)
            break;
        for(n = 0; n < ptrcnt; n++) {
            nn = n % dz->itemcnt;
            if(iptr[n] > 0 && iptr[n] < insampcnt) {
                obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[iptr[n]] * llev[nn]));
                obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[iptr[n]] * rlev[nn]));
            }
            if(iptr[n] != 0)
                iptr[n]++;
            if(iptr[n] >= cyclen[nn])
                iptr[n] = 0;
        }
        if(dz->iparam[CER_DELAY] > 0) {
            for(n = 0, m= ptrcnt; n < ptrcnt; n++,m++) {
                nn = n % dz->itemcnt;
                lmst = (lmost[nn] + dz->iparam[CER_DSTEP]) % chans;
                rmst = (lmst + 1) % chans;
                if(iptr[m] >= 0 && iptr[m] < insampcnt) {
                    obuf[opos + lmst] = (float)(obuf[opos + lmst] + (ibuf[iptr[m]] * llev[nn]));
                    obuf[opos + rmst] = (float)(obuf[opos + rmst] + (ibuf[iptr[m]] * rlev[nn]));
                } 
                if(iptr[m] != 0)
                    iptr[m]++;
                if(iptr[m] >= cyclen[nn])
                    iptr[n] = 0;
            }
        }
        outcnt++;
        opos += chans;
        if(opos >= dz->buflen) {
            fprintf(stdout,"INFO: Level Check at %lf secs\n",(double)outcnt/srate);
            fflush(stdout);
            for(n=0;n < dz->buflen;n++)
                maxsamp = max(maxsamp,fabs(obuf[n]));
            memset((char *)obuf,0,dz->buflen * sizeof(float));
            opos = 0;
        }
    }
    if(opos >= 0) {
        fprintf(stdout,"INFO: Level Check at %lf secs\n",(double)outcnt/srate);
        fflush(stdout);
        for(n=0;n < opos;n++)
            maxsamp = max(maxsamp,fabs(obuf[n]));
    }
    normaliser = 1.0;
    if(maxsamp > 0.95) {
        normaliser = 0.95/maxsamp;
        fprintf(stdout,"INFO: Output will be normalised by %.2lf secs\n",normaliser);
        fflush(stdout);
    }
    if(sloom)
        dz->insams[0] = outcnt * chans;     //  This forces sloom progress bar to proceed correctly, without mod to libraries
    reset_filedata_counters(dz);
    memset((char *)obuf,0,dz->buflen * sizeof(float));
    for(n = 0;n<dz->itemcnt;n++)
        iptr[n] = 0;
    if(dz->dupl > 1) {
        for(n = 0;n<dz->itemcnt;n++)
            cyclen[n] = orig_cyclen[n];
        for(k = 1;k < dz->dupl;k++) {
            bas = dz->itemcnt * k;
            for(n = 0;n<dz->itemcnt;n++)
                iptr[n + bas] = -(cyclen[n] * k);
        }
        for(n = 0;n<dz->itemcnt;n++)
            cyclen[n] *= dz->dupl;
    }
    if(dz->iparam[CER_DELAY] > 0) {
        bas = ptrcnt;
        for(n = 0;n < ptrcnt;n++)
            iptr[n + bas] = iptr[n] - dz->iparam[CER_DELAY];
    }
    opos = 0;
    outcnt = 0;
    fprintf(stdout,"INFO: Second pass: creating sound.\n");
    fflush(stdout);
    while(outcnt < mintotalout) {
        for(n = 0; n < ptrcnt; n++) {
            nn = n % dz->itemcnt;
            if(iptr[n] >= 0 && iptr[n] < insampcnt) {
                obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[iptr[n]] * llev[nn]));
                obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[iptr[n]] * rlev[nn]));
            } 
            iptr[n]++;
            if(iptr[n] >= cyclen[nn])
                iptr[n] = 0;
        }
        if(dz->iparam[CER_DELAY] > 0) {
            for(n = 0,m = ptrcnt; n < ptrcnt; n++,m++) {
                nn = n % dz->itemcnt;
                lmst = (lmost[nn] + dz->iparam[CER_DSTEP]) % chans;
                rmst = (lmst + 1) % chans;
                if(iptr[m] >= 0 && iptr[m] < insampcnt) {
                    obuf[opos + lmst] = (float)(obuf[opos + lmst] + (ibuf[iptr[m]] * llev[nn]));
                    obuf[opos + rmst] = (float)(obuf[opos + rmst] + (ibuf[iptr[m]] * rlev[nn]));
                } 
                iptr[m]++;
                if(iptr[m] >= cyclen[nn])
                    iptr[m] = 0;
            }
        }
        outcnt++;
        opos += chans;
        if(opos >= dz->buflen) {
            if(normaliser < 1.0) {
                for(n=0;n < dz->buflen;n++)
                    obuf[n] = (float)(obuf[n] * normaliser);
            }
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            memset((char *)obuf,0,dz->buflen * sizeof(float));
            opos = 0;
        }
    }   
    samps_left = 1;
    while(samps_left) {     //  Flush remaining buffers
        samps_left = 0;
        for(n = 0; n < ptrcnt; n++) {
            nn = n % dz->itemcnt;
            if(iptr[n] > 0 && iptr[n] < insampcnt) {
                samps_left = 1;
                break;
            }
        }
        if(dz->iparam[CER_DELAY] > 0) {
            for(n = 0, m= ptrcnt; n < ptrcnt; n++,m++) {
                nn = n % dz->itemcnt;
                if(iptr[m] > 0 && iptr[m] < insampcnt) {
                    samps_left++;
                    break;
                }
            }
        }
        if(!samps_left)
            break;
        for(n = 0; n < ptrcnt; n++) {
            nn = n % dz->itemcnt;
            if(iptr[n] > 0 && iptr[n] < insampcnt) {
                obuf[opos + lmost[nn]] = (float)(obuf[opos + lmost[nn]] + (ibuf[iptr[n]] * llev[nn]));
                obuf[opos + rmost[nn]] = (float)(obuf[opos + rmost[nn]] + (ibuf[iptr[n]] * rlev[nn]));
            } 
            if(iptr[n] != 0)
                iptr[n]++;
            if(iptr[n] >= cyclen[nn])
                iptr[n] = 0;
        }
        if(dz->iparam[CER_DELAY] > 0) {
            for(n = 0, m= ptrcnt; n < ptrcnt; n++,m++) {
                nn = n % dz->itemcnt;
                lmst = (lmost[nn] + dz->iparam[CER_DSTEP]) % chans;
                rmst = (lmst + 1) % chans;
                if(iptr[m] >= 0 && iptr[m] < insampcnt) {
                    obuf[opos + lmst] = (float)(obuf[opos + lmst] + (ibuf[iptr[m]] * llev[nn]));
                    obuf[opos + rmst] = (float)(obuf[opos + rmst] + (ibuf[iptr[m]] * rlev[nn]));
                } 
                if(iptr[m] != 0)
                    iptr[m]++;
                if(iptr[m] >= cyclen[nn])
                    iptr[n] = 0;
            }
        }
        opos += chans;
        if(opos >= dz->buflen) {
            if(normaliser < 1.0) {
                for(n=0;n < dz->buflen;n++)
                    obuf[n] = (float)(obuf[n] * normaliser);
            }
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            memset((char *)obuf,0,dz->buflen * sizeof(float));
            opos = 0;
        }
    }   
    if(opos >= 0) {
        if(normaliser < 1.0) {
            for(n=0;n < opos;n++)
                obuf[n] = (float)(obuf[n] * normaliser);
        }
        if((exit_status = write_samps(obuf,opos,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/**************************** CREATE_CERACU_SNDBUFS ****************************/

int create_ceracu_sndbufs(dataptr dz)
{
    int n, safety = 4;
    unsigned int bigbufsize = 0;
    float *bottom;
    dz->bufcnt = 2;
    if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    bigbufsize = (dz->insams[0] + safety) * sizeof(float);
    dz->buflen = (512 * 512) * dz->iparam[CER_OCHANS];  
    bigbufsize += (dz->buflen + (safety * dz->iparam[NTEX_CHANS])) * sizeof(float);
    if(bigbufsize < 0) {
        sprintf(errstr,"Input sound too large.\n");
        return(MEMORY_ERROR);
    }
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    bottom = dz->bigbuf;
    for(n = 0;n<dz->infilecnt;n++) {
        dz->sbufptr[n] = dz->sampbuf[n] = bottom;
        bottom += dz->insams[n] + safety;
    }
    dz->sbufptr[n] = dz->sampbuf[n] = bottom;
    return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
    int exit_status;
    dz->infile->channels = dz->iparam[CER_OCHANS];
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
        return(exit_status);
    dz->infile->channels = 1;
    return(FINISHED);
}

