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
//  _cdprogs\scramble scramble 5 alan_bellydancefc.wav test.wav cuts.txt

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

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int setup_scramble_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_scramble_param_ranges_and_defaults(dataptr dz);
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
static int scramble(dataptr dz);
static void rndpermm(int permlen,int *permm);
static void insert(int m,int t,int permlen,int *permm);
static void prefix(int m,int permlen,int *permm);
static void shuflup(int k,int permlen, int *permm);
static int write_waveset(int *obufpos,int *lastbuf,int ibufstart,int len,double amp,double incr,dataptr dz);
static int handle_the_special_data(char *str,dataptr dz);
static int get_levels(int *wset_store,int wcnt,double *level,dataptr dz);

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
        dz->maxmode = 14;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_scramble_application(dz))<0) {
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
    if((exit_status = setup_scramble_param_ranges_and_defaults(dz))<0) {
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
    switch(dz->mode) {
    case(4):    case(5):    case(6):    case(7):
    case(10):   case(11):   case(12):   case(13):
        if((exit_status = handle_the_special_data(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdlinecnt--;
        cmdline++;
        break;
    }

    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency() redundant
    is_launched = TRUE;
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

    if((exit_status = create_sndbufs(dz))<0) {                                  // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess()                        redundant
    //spec_process_file =
    if((exit_status = scramble(dz))<0) {
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

/************************* SETUP_SCRAMBLE_APPLICATION *******************/

int setup_scramble_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    switch(dz->mode) {
    case(0):    //  fall  thro
    case(1): exit_status = set_param_data(ap,0   ,2,2,"di");        break;
    case(2):    //  fall  thro
    case(3):    //  fall  thro
    case(8):    //  fall  thro
    case(9): exit_status = set_param_data(ap,0   ,2,1,"0i");        break;
    default: exit_status = set_param_data(ap,VERGEDATA,2,1,"0i");   break;
    }
    if((exit_status)<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"cta",3,"iDD","",0,0,""))<0)
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

/************************* SETUP_SCRAMBLE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_scramble_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    if(dz->mode <= 1) {
        ap->lo[SCR_DUR]  = 1.0;
        ap->hi[SCR_DUR]  = 7200.0;
        ap->default_val[SCR_DUR]  = 20.0;
    }
    ap->lo[SCR_SEED] = 0;
    ap->hi[SCR_SEED] = 256;
    ap->default_val[SCR_SEED] = 1;
    ap->lo[SCR_CNT]  = 1.0;
    ap->hi[SCR_CNT]  = 256.0;
    ap->default_val[SCR_CNT]  = 1.0;
    ap->lo[SCR_TRNS]  = 0.0;
    ap->hi[SCR_TRNS]  = 12.0;
    ap->default_val[SCR_TRNS]  = 0.0;
    ap->lo[SCR_ATTEN] = .0;
    ap->hi[SCR_ATTEN] = 1.0;
    ap->default_val[SCR_ATTEN] = 0.0;
    dz->maxmode = 14;
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
            if((exit_status = setup_scramble_application(dz))<0)
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
    usage2("scramble");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"scramble"))                dz->process = SCRUNCH;
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
    if(!strcmp(str,"scramble")) {
        fprintf(stderr,
        "USAGE:\n"
        "scramble scramble 1-2   infile outfile dur  seed [-ccnt] [-ttrns] [-aatten]\n"
        "scramble scramble 3-4   infile outfile      seed [-ccnt] [-ttrns] [-aatten]\n"
        "scramble scramble 5-8   infile outfile cuts seed [-ccnt] [-ttrns] [-aatten]\n"
        "scramble scramble 9-10  infile outfile      seed [-ccnt] [-ttrns] [-aatten]\n"
        "scramble scramble 11-14 infile outfile cuts seed [-ccnt] [-ttrns] [-aatten]\n"
        "\n"
        "Scramble order of waveset in src.\n"
        "\n"
        "DUR   Duration of output file.\n"
        "SEED  Random seed (0-256). Same seed with same rand-params gives same output.\n"
        "CNT   Number of wavesets in waveset-groups to be scrambled. (Range 1 to 256)\n"
        "TRNS  Range of any random transposition of wavesets (semitones) (Range 0 to 12).\n"
        "ATTEN Range of any random attenuation of wavesets (Range 0 to 1).\n"
        "CUTS  Textfile of (increasing) times in src: process in each separate segment.\n"
        "\n"
        "\"TRNS\" and \"ATTEN\" can vary through (output) time.\n"
        "\n"
        "Reassembly is ....\n"
        "Mode 1:  in random order.\n"
        "Mode 2:  in permuted random order (all wavesets used before any reused).\n"
        "Mode 3:  in order of increasing size (falling pitch).\n"
        "Mode 4:  in order of decreasing size (rising pitch).\n"
        "Mode 5:  in order of increasing size, in each segment.\n"
        "Mode 6:  in order of decreasing size, in each segment.\n"
        "Mode 7:  in order of increasing then decreasing size.\n"
        "Mode 8:  in order of decreasing then increasing size.\n"
        "Mode 9:  in order of increasing level.\n"
        "Mode 10: in order of decreasing level.\n"
        "Mode 11: in order of increasing level, in each segment.\n"
        "Mode 12: in order of decreasing level, in each segment.\n"
        "Mode 13: in order of increasing then decreasing level.\n"
        "Mode 14: in order of decreasing then increasing level.\n"
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

/******************************** SCRAMBLE ********************************/

int scramble(dataptr dz)
{
    int exit_status, phase, phasecnt, do_swap, gpcnt, increase = 0, *permm;
    int n, segcnt, k, kend = 0, kstart = 0, beyond, j, outsamps, wcnt, s, e, ss, ee, lastbuf_start, coasting, coasting_start = 0, coasting_end;
    int len, lena, lenb, cnt, ibufstart, obufpos, thisbuf, lastbuf, smpsout, temps, tempe, smp_segend, *wset_store;
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[2];
    double amp, incr, time, leva, levb, srate = (double)dz->infile->srate;
    double *segtime = NULL, *level = NULL;

    srand(dz->iparam[SCR_SEED]);
    switch(dz->mode) {
    case(4):    case(5):    case(6):    case(7):
    case(10):   case(11):   case(12):   case(13):
        segtime = dz->parray[0];
        break;
    }
    outsamps = (int)round(dz->param[0] * srate);

    fprintf(stdout,"INFO: Counting wavesets.\n");
    fflush(stdout);
    dz->total_samps_read = 0;
    n = 0;
    wcnt = 0;
    phase = 0;
    phasecnt = 0;
    gpcnt = dz->iparam[SCR_CNT] * 2;
    dz->total_samps_read = 0;
    do {
        lastbuf_start = dz->total_samps_read;
        if((exit_status = read_samps(ibuf,dz))<0)
            return exit_status;
        n = 0;
        while(n < dz->ssampsread) {
            if(ibuf[n] == 0.0)
                ;
            else {
                switch(phase) {
                case(0):
                    if(ibuf[n] > 0.0)
                        phase = 1;
                    else
                        phase = -1;
                    wcnt++;
                    break;
                case(1):
                    if(ibuf[n] < 0.0) {
                        if(++phasecnt >= gpcnt) {
                            wcnt++;
                            phasecnt = 0;
                        }
                        phase = -1;
                    }
                    break;
                case(-1):
                    if(ibuf[n] > 0.0) {
                        if(++phasecnt >= gpcnt) {
                            wcnt++;
                            phasecnt = 0;
                        }
                        phase = 1;
                    }
                    break;
                }
            }
            n++;
        }
    } while(dz->ssampsread > 0);

    if((dz->lparray = (int **)malloc(sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create data storage.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[0] = (int *)malloc((wcnt * 2) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store waveset coordinates.\n");
        return(MEMORY_ERROR);
    }
    wset_store = dz->lparray[0];
    if((permm = (int *)malloc(wcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory to create segment-order permutation store.\n");
        return(MEMORY_ERROR);
    }                                                               
    if(dz->mode >= 8) {
        if((level = (double *)malloc(wcnt * sizeof(double)))==NULL) {
            sprintf(errstr,"Insufficient memory to create segment-levels store.\n");
            return(MEMORY_ERROR);
        }                                                               
    }
    fprintf(stdout,"INFO: Storing waveset coordinates.\n");
    fflush(stdout);
        
    dz->total_samps_read = 0;
    n = 0;
    phase = 0;
    phasecnt = 0;
    k = 0;
    sndseekEx(dz->ifd[0],0,0);
    dz->total_samps_read = 0;
    do {
        lastbuf_start = dz->total_samps_read;
        if((exit_status = read_samps(ibuf,dz))<0)
            return exit_status;
        n = 0;
        while(n < dz->ssampsread) {
            if(ibuf[n] == 0.0)
                ;
            else {
                switch(phase) {
                case(0):
                    if(ibuf[n] > 0.0)
                        phase = 1;
                    else
                        phase = -1;
                    wset_store[k++] = n + lastbuf_start;
                    wset_store[k++] = n + lastbuf_start;
                    break;
                case(1):
                    if(ibuf[n] < 0.0) {
                        if(++phasecnt >= gpcnt) {
                            wset_store[k++] = n + lastbuf_start;
                            wset_store[k++] = n + lastbuf_start;
                            phasecnt = 0;
                        }
                        phase = -1;
                    }
                    break;
                case(-1):
                    if(ibuf[n] > 0.0) {
                        if(++phasecnt >= gpcnt) {
                            wset_store[k++] = n + lastbuf_start;
                            wset_store[k++] = n + lastbuf_start;
                            phasecnt = 0;
                        }
                        phase = 1;
                    }
                    break;
                }
            }
            n++;
        }
    } while(dz->ssampsread > 0);

    for(k = 1; k < wcnt * 2; k++)           //  Lose duplication of first waveset start
        wset_store[k-1] = wset_store[k];    //  Duplication of end wavset end will be ignored
    wcnt--;

    fprintf(stdout,"INFO: Trimming waveset coordinates.\n");
    fflush(stdout);
    dz->total_samps_read = 0;
    n = 0;
    coasting = 0;
    k = 0;
    sndseekEx(dz->ifd[0],0,0);
    dz->total_samps_read = 0;
    do {
        lastbuf_start = dz->total_samps_read;
        if((exit_status = read_samps(ibuf,dz))<0)
            return exit_status;
        n = 0;
        while(n < dz->ssampsread) {
            if(ibuf[n] == 0.0) {
                if(!coasting)                           //  Count any blocks of zero-signal
                    coasting_start = n + lastbuf_start;
                coasting++;
                n++;
            } else if(coasting) {                       //  If there is zero signal at end of waveset
                coasting_end = n + lastbuf_start;       //  Snip it off the waveset
                for(k=2;k<wcnt;k+=2) {
                    if(wset_store[k] == coasting_end)
                        wset_store[k-1] = coasting_start;
                    else if(wset_store[k] > coasting_end)
                        break;
                }
                coasting = 0;
            }
            n++;
        }
    } while(dz->ssampsread > 0);

    if(dz->mode >= 8) {
        if((exit_status = get_levels(wset_store,wcnt,level,dz))<0)
            return exit_status;
    }
    dz->total_samps_read = 0;
    cnt = 0;
    obufpos = 0;
    lastbuf = -1;
    sndseekEx(dz->ifd[0],0,0);

    switch(dz->mode) {
    case(1):
        rndpermm(wcnt,permm);
        //  fall thro
    case(0):
        fprintf(stdout,"INFO: Generating output sound.\n");
        fflush(stdout);
        while((smpsout = dz->total_samps_written + obufpos) < outsamps) {
            time = (int)round(smpsout * srate);
            if(dz->mode == 1) {
                k = permm[cnt];
                if(++cnt >= wcnt) {
                    rndpermm(wcnt,permm);
                    cnt = 0;
                }
            } else 
                k = (int)floor(drand48() * wcnt);
            s = k*2;
            e = s+1;
            len = wset_store[e] - wset_store[s];
            thisbuf = wset_store[s]/dz->buflen;
            if(thisbuf != lastbuf) {
                sndseekEx(dz->ifd[0],thisbuf * dz->buflen,0);
                n = 0;
                dz->buflen *= 2;
                memset((char *)ibuf,0,dz->buflen * sizeof(float));
                if((exit_status = read_samps(ibuf,dz))<0)           //  Read a double buff
                    return exit_status;
                dz->buflen /= 2;
                lastbuf = thisbuf;
            }
            ibufstart = wset_store[s] - (thisbuf * dz->buflen);
            if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                return exit_status;
            if(dz->param[SCR_ATTEN] > 0.0)
                amp = 1.0 - (drand48() * dz->param[SCR_ATTEN]);
            else
                amp = 1.0;
            if(dz->param[SCR_TRNS] > 0.0) {
                incr = (drand48() * 2.0) - 1.0;
                incr *= dz->param[SCR_TRNS];
                incr = pow(2.0,incr/SEMITONES_PER_OCTAVE);
            } else
                incr = 1.0;
            if((exit_status = write_waveset(&obufpos,&thisbuf,ibufstart,len,amp,incr,dz))<0)
                return exit_status;
        }
        break;
    case(2):
    case(3):
    case(8):
    case(9):
        fprintf(stdout,"INFO: Sorting wavesets.\n");
        fflush(stdout);
        switch(dz->mode)  {
        case(2):
        case(3):
            for(k=0,s=0,e=1; k < wcnt-1; k++,s+=2,e+=2) {                   //  Sort wavesets for size
                lena = wset_store[e] - wset_store[s];
                for(j=k+1,ss = s+2,ee=e+2; j < wcnt; j++,ss+=2,ee+=2) {
                    lenb = wset_store[ee] - wset_store[ss];
                    do_swap = 0;
                    if(dz->mode == 2) {
                        if(lenb < lena)
                            do_swap = 1;
                    } else  {
                        if(lenb > lena)
                            do_swap = 1;
                    }
                    if(do_swap) {
                        temps = wset_store[s];
                        tempe = wset_store[e];
                        wset_store[s] = wset_store[ss];
                        wset_store[e] = wset_store[ee];
                        wset_store[ss] = temps;
                        wset_store[ee] = tempe;
                        lena  = lenb;
                    }
                }
            }
            break;
        default:
            for(k=0,s=0,e=1; k < wcnt-1; k++,s+=2,e+=2) {                   //  Sort wavesets for level
                leva = level[k];
                for(j=k+1,ss = s+2,ee=e+2; j < wcnt; j++,ss+=2,ee+=2) {
                    levb = level[j];
                    do_swap = 0;
                    if(dz->mode == 8) {
                        if(levb < leva)
                            do_swap = 1;
                    } else  {
                        if(levb > leva)
                            do_swap = 1;
                    }
                    if(do_swap) {
                        temps = wset_store[s];
                        tempe = wset_store[e];
                        wset_store[s] = wset_store[ss];
                        wset_store[e] = wset_store[ee];
                        wset_store[ss] = temps;
                        wset_store[ee] = tempe;
                        level[k] = levb;
                        level[j] = leva;
                        leva = levb;
                    }
                }
            }
            break;
        }
        fprintf(stdout,"INFO: Generating output sound.\n");
        fflush(stdout);
        obufpos = 0;
        for(k=0,s=0,e=1;k < wcnt;k++,s+=2,e+=2) {
            smpsout = dz->total_samps_written + obufpos;
            time = (int)round(smpsout * srate);
            len = wset_store[e] - wset_store[s];
            thisbuf = wset_store[s]/dz->buflen;
            if(thisbuf != lastbuf) {
                sndseekEx(dz->ifd[0],thisbuf * dz->buflen,0);
                dz->buflen *= 2;
                memset((char *)ibuf,0,dz->buflen * sizeof(float));
                if((exit_status = read_samps(ibuf,dz))<0)           //  Read a double buff
                    return exit_status;
                dz->buflen /= 2;
                lastbuf = thisbuf;
            }
            ibufstart = wset_store[s] - (thisbuf * dz->buflen);
            if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                return exit_status;
            if(dz->param[SCR_ATTEN] > 0.0)
                amp = 1.0 - (drand48() * dz->param[SCR_ATTEN]);
            else
                amp = 1.0;
            if(dz->param[SCR_TRNS] > 0.0) {
                incr = (drand48() * 2.0) - 1.0;
                incr *= dz->param[SCR_TRNS];
                incr = pow(2.0,incr/SEMITONES_PER_OCTAVE);
            } else
                incr = 1.0;
            if((exit_status = write_waveset(&obufpos,&lastbuf,ibufstart,len,amp,incr,dz))<0)
                return exit_status;
        }
        break;
    default:
        switch(dz->mode) {
        case(4):    //  fall thro       //  Sort for increase in waveset size
        case(6):    //  fall thro       //  Initially sort for increase in waveset size
        case(10):   //  fall thro       //  Sort for increase in waveset level
        case(12): increase = 1; break;  //  Initially sort for increase in waveset level
        case(5):    //  fall thro       //  Sort for decrease in waveset size
        case(7):    //  fall thro       //  Initially sort for decrease in waveset size
        case(11):   //  fall thro       //  Sort for decrease in waveset level
        case(13): increase = -1; break; //  Initially sort for decrease in waveset level
        }
        obufpos = 0;
        kstart = 0;
        for(segcnt = 0;segcnt < dz->itemcnt; segcnt++) {            //  For each specified segment
            smp_segend = (int)round(segtime[segcnt] * srate);
            fprintf(stdout,"INFO: Choosing wavesets for segment %d.\n",segcnt+1);
            fflush(stdout);                                         //  Select wavesets to use

            kend = -1;
            for(k=kstart,s=kstart*2; k < wcnt; k++,s+=2) {
                if((beyond = wset_store[s] - smp_segend) >= 0) {    //  Find waveset starting at or beyond segtime
                    kend = k;                                           
                    if(k > 0 &&  (smp_segend - wset_store[s-2]) < beyond) {
                        if(kend-1 > kstart)                         //  If previous seg-start is nearer to desired seg-cut time, use that
                            kend--;
                    }
                    break;
                }
            }
            if(kend < 0)
                kend = wcnt-1;

            fprintf(stdout,"INFO: Sorting wavesets in segment %d.\n",segcnt+1);
            fflush(stdout);                                         
            switch(dz->mode) {
            case(4):    case(5):    case(6):    case(7):            //  Sort wavesets for size
                for(k=kstart,s=kstart*2,e=(kstart*2)+1; k < kend-1; k++,s+=2,e+=2) {
                    lena = wset_store[e] - wset_store[s];
                    for(j=k+1,ss = s+2,ee=e+2; j < kend; j++,ss+=2,ee+=2) {
                        lenb = wset_store[ee] - wset_store[ss];
                        do_swap = 0;
                        if(increase > 0) {
                            if(lenb < lena)
                                do_swap = 1;
                        } else  {
                            if(lenb > lena)
                                do_swap = 1;
                        }
                        if(do_swap) {
                            temps = wset_store[s];
                            tempe = wset_store[e];
                            wset_store[s] = wset_store[ss];
                            wset_store[e] = wset_store[ee];
                            wset_store[ss] = temps;
                            wset_store[ee] = tempe;
                            lena  = lenb;
                        }
                    }
                }
                break;
            default:                                                //  Sort wavesets for level
                for(k=kstart,s=kstart*2,e=(kstart*2)+1; k < kend-1; k++,s+=2,e+=2) {
                    leva = level[k];
                    for(j=k+1,ss = s+2,ee=e+2; j < kend; j++,ss+=2,ee+=2) {
                        levb = level[j];
                        do_swap = 0;
                        if(increase > 0) {
                            if(levb < leva)
                                do_swap = 1;
                        } else  {
                            if(levb > leva)
                                do_swap = 1;
                        }
                        if(do_swap) {
                            temps = wset_store[s];
                            tempe = wset_store[e];
                            wset_store[s] = wset_store[ss];
                            wset_store[e] = wset_store[ee];
                            wset_store[ss] = temps;
                            wset_store[ee] = tempe;
                            level[k] = levb;
                            level[j] = leva;
                            leva  = levb;
                        }
                    }
                }
                break;
            }
            fprintf(stdout,"INFO: Generating output sound for segment %d.\n",segcnt+1);
            fflush(stdout);
            for(k=kstart,s=kstart*2,e=(kstart*2)+1;k < kend;k++,s+=2,e+=2) {
                smpsout = dz->total_samps_written + obufpos;
                time = (int)round(smpsout * srate);
                len = wset_store[e] - wset_store[s];
                thisbuf = wset_store[s]/dz->buflen;
                if(thisbuf != lastbuf) {
                    sndseekEx(dz->ifd[0],thisbuf * dz->buflen,0);
                    dz->buflen *= 2;                    
                    memset((char *)ibuf,0,dz->buflen * sizeof(float));
                    if((exit_status = read_samps(ibuf,dz))<0)           //  Read a double buff
                        return exit_status;
                    dz->buflen /= 2;
                    lastbuf = thisbuf;
                }
                ibufstart = wset_store[s] - (thisbuf * dz->buflen);
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return exit_status;
                if(dz->param[SCR_ATTEN] > 0.0)
                    amp = 1.0 - (drand48() * dz->param[SCR_ATTEN]);
                else
                    amp = 1.0;
                if(dz->param[SCR_TRNS] > 0.0) {
                    incr = (drand48() * 2.0) - 1.0;
                    incr *= dz->param[SCR_TRNS];
                    incr = pow(2.0,incr/SEMITONES_PER_OCTAVE);
                } else
                    incr = 1.0;
                if((exit_status = write_waveset(&obufpos,&lastbuf,ibufstart,len,amp,incr,dz))<0)
                    return exit_status;
            }
            kstart = kend;
            if(kstart == wcnt - 1)                                      //  No more segments
                break;
            switch(dz->mode) {
            case(6):    //  fall thro
            case(7):    //  fall thro
            case(12):   //  fall thro
            case(13): increase = -increase; break;
            }
        }
        break;
    }
    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }
    return FINISHED;
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

/****************************** WRITE_WAVESET ***********************************/

int write_waveset(int *obufpos,int *lastbuf,int ibufstart,int len,double amp,double incr,dataptr dz)
{
    int exit_status;
    float *ibuf = dz->sampbuf[0], *ovflwbuf = dz->sampbuf[1], *obuf = dz->sampbuf[2];
    double d, dibufpos, frac, diff, val;
    int ibufpos;
    d = 0.0;
    dibufpos = (double)ibufstart;
    while(d < len) {
        ibufpos = (int)floor(dibufpos);
        if(ibufpos >= dz->buflen) {
            memcpy((char *)ibuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
            memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
            if((dz->ssampsread = fgetfbufEx(ovflwbuf,dz->buflen,dz->ifd[0],0)) < 0) {
                sprintf(errstr,"Can't read samples from input soundfile.\n");
                return(SYSTEM_ERROR);
            }
            ibufpos  -= dz->buflen;
            dibufpos -= (double)dz->buflen;
            (*lastbuf)++;
        }
        frac = dibufpos - (double)ibufpos;
        val  = ibuf[ibufpos];
        diff = ibuf[ibufpos+1] - val;
        val += frac * diff;
        obuf[(*obufpos)++] = (float)(val * amp);
        if(*obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            *obufpos = 0;
        }
        d += incr;
        dibufpos += incr;
    }
    return FINISHED;
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
            if(cnt == 0) {
                if(dummy <= 0.0) {
                    sprintf(errstr,"Invalid time (%lf) on line %d in file %s. Must be greater than zero.\n",dummy,linecnt+1,str);
                    return(DATA_ERROR);
                }
            } else if(dummy <= lasttime) {
                sprintf(errstr,"Times (%lf & %lf) do not advance in at line %d in file %s.\n",lasttime, dummy,linecnt,str);
                return(DATA_ERROR);
            } else if(dummy >= dz->duration) {
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
    if(!flteq(lasttime,dz->duration))
        cnt++;
    dz->itemcnt = cnt;
    if((dz->parray = (double **)malloc(sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create slice-time-data storage. (1)\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
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
            dz->parray[0][cnt] = dummy;
            if(++cnt >= dz->itemcnt) {
                done = 1;
                break;
            }
        }
        if(done)
            break;
    }
    if(!flteq(lasttime,dz->duration))
        dz->parray[0][cnt] = dz->duration;
    fclose(fp);
    return FINISHED;
}

/******************************************** GET_LEVELS *****************************/

int get_levels(int *wset_store,int wcnt,double *level,dataptr dz)
{
    int exit_status;
    float *ibuf = dz->sampbuf[0];
    double maxsamp, val;
    int n, s, e, k, last_samps_read, startsamp, endsamp;
    last_samps_read = 0;
    dz->total_samps_read = 0;
    sndseekEx(dz->ifd[0],0,0);
    if((exit_status = read_samps(ibuf,dz))<0)
        return exit_status;
    for(n=0,s=0,e=1;n < wcnt;n++,s+=2,e+=2) {
        while((startsamp = wset_store[s] - last_samps_read) >= dz->buflen) {
            last_samps_read = dz->total_samps_read;
            if((exit_status = read_samps(ibuf,dz))<0)
                return exit_status;
        }
        endsamp = wset_store[e] - last_samps_read;
        maxsamp = 0.0;
        for(k=startsamp;k < endsamp;k++) {
            if(k >= dz->buflen) {
                last_samps_read = dz->total_samps_read;
                if((exit_status = read_samps(ibuf,dz))<0)
                    return exit_status;
                endsamp -= dz->buflen;
                k = 0;
            }
            val = fabs(ibuf[k]);
            if(val > maxsamp)
                maxsamp = val;
        }
        level[n] = maxsamp;
    }
    return FINISHED;
}
