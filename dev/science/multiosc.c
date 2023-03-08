
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

#define ROOT2       (1.4142136)

#define DOUBLE_OSC      0
#define TRIPLE_OSC      1
#define QUADRUPLE_OSC   2


#define MOSCAMP         0.9         //  Output level for sound output

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int setup_multiosc_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int setup_multiosc_param_ranges_and_defaults(dataptr dz);
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

static int multiosc_param_preprocess(dataptr dz);
static int multiosc(dataptr dz);
static double multioscillator(double onehzincr,double *tabpos,double *tabpos2,double *tabpos3,double *tabpos4,int oscilcnt,dataptr dz);
static int create_multiosc_sndbufs(dataptr dz);
static int setup_readtable(dataptr dz) ;
static double read_wavetable(double tabpos,dataptr dz);

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
        dz->maxmode = 3;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_multiosc_application(dz))<0) {
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
    dz->infile->channels = 1;
    // parse_infile_and_hone_type() = 
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_multiosc_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // open_first_infile() : redundant
    // handle_extra_infiles() : redundant
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency() redundant
    if((exit_status = open_the_outfile(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    if((exit_status = create_multiosc_sndbufs(dz))<0) {                                     // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = multiosc_param_preprocess(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //spec_process_file =
    if((exit_status = multiosc(dz))<0) {
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


/***************************** OPEN_THE_OUTFILE **************************/

int open_the_outfile(dataptr dz)
{
    int exit_status;
    dz->infile->channels = 1;
    dz->infile->srate = dz->iparam[MOSC_SRATE];
    dz->outfile->channels = 1;
    dz->outfile->srate = dz->iparam[MOSC_SRATE];
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
        return(exit_status);
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

/************************* SETUP_MULTIOSC_APPLICATION *******************/

int setup_multiosc_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    switch(dz->mode) {
    case(0):    exit_status = set_param_data(ap,0,10,6, "dDDD0000id");  break;
    case(1):    exit_status = set_param_data(ap,0,10,8, "dDDDDD00id");  break;
    case(2):    exit_status = set_param_data(ap,0,10,10,"dDDDDDDDid");  break;
    }
    if(exit_status <0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
        return(exit_status);
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = NO_FILE_AT_ALL;
    dz->process_type = UNEQUAL_SNDFILE; 
    dz->outfiletype  = SNDFILE_OUT;
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

/************************* SETUP_MULTIOSC_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_multiosc_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()

    ap->lo[MOSC_DUR]    = 0.0;
    ap->hi[MOSC_DUR]    = 32767.0;
    ap->default_val[MOSC_DUR]   = 1.0;
    ap->lo[MOSC_FRQ1]   = .001;
    ap->hi[MOSC_FRQ1]   = 10000;
    ap->default_val[MOSC_FRQ1] = 440;
    ap->lo[MOSC_FRQ2]   = .001;
    ap->hi[MOSC_FRQ2]   = 10000;
    ap->default_val[MOSC_FRQ2] = 440;
    ap->lo[MOSC_AMP2]   = 0;
    ap->hi[MOSC_AMP2]   = 1.0;
    ap->default_val[MOSC_AMP2] = 0;
    if(dz->mode >= TRIPLE_OSC) {
        ap->lo[MOSC_FRQ3]   = .001;
        ap->hi[MOSC_FRQ3]   = 10000;
        ap->default_val[MOSC_FRQ3] = 440;
        ap->lo[MOSC_AMP3]   = 0;
        ap->hi[MOSC_AMP3]   = 1.0;
        ap->default_val[MOSC_AMP3] = 0;
    }
    if(dz->mode == QUADRUPLE_OSC) {
        ap->lo[MOSC_FRQ4]   = .001;
        ap->hi[MOSC_FRQ4]   = 10000;
        ap->default_val[MOSC_FRQ4] = 440;
        ap->lo[MOSC_AMP4]   = 0;
        ap->hi[MOSC_AMP4]   = 1.0;
        ap->default_val[MOSC_AMP4] = 0;
    }
    ap->lo[MOSC_SRATE]  = 16000;
    ap->hi[MOSC_SRATE]  = 96000;
    ap->default_val[MOSC_SRATE] = 44100;
    ap->lo[MOSC_SPLEN]  = 1;
    ap->hi[MOSC_SPLEN]  = 50;
    ap->default_val[MOSC_SPLEN] = 50;
    dz->maxmode = 3;
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
            if((exit_status = setup_multiosc_application(dz))<0)
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
    usage2("multiosc");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"multiosc"))                dz->process = MULTIOSC;
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

/**************************** USAGE3 *************************/

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/**************************** MULTIOSC_PARAM_PREPROCESS *************************/

int multiosc_param_preprocess (dataptr dz)
{
    int exit_status;
    double srate;
    if(BAD_SR(dz->iparam[MOSC_SRATE])) {
        sprintf(errstr,"Invalid sample rate (%d) entered.\n",dz->iparam[MOSC_SRATE]);
        return(MEMORY_ERROR);
    }
    srate = (double)dz->iparam[MOSC_SRATE];
    dz->iparam[MOSC_DUR] = (int)round(dz->param[MOSC_DUR] * srate);
    dz->iparam[MOSC_SPLEN] = (int)round(dz->param[MOSC_SPLEN] * MS_TO_SECS * srate);
    if(dz->iparam[MOSC_SPLEN] * 2 >= dz->iparam[MOSC_DUR]) {
        sprintf(errstr,"Duration too short for dovetailing splices.\n");
        return(MEMORY_ERROR);
    }
    if((exit_status = setup_readtable(dz))<0)
        return(exit_status);
    return FINISHED;
}

/**************************** CREATE_MULTIOSC_SNDBUFS ****************************/

int create_multiosc_sndbufs(dataptr dz)
{
    int n;
    int bigbufsize;
    int framesize;
    framesize = F_SECSIZE;
    dz->bufcnt = 1;
    if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    bigbufsize = (int)(size_t) Malloc(-1);
    bigbufsize /= dz->bufcnt;
    if(bigbufsize <=0)
        bigbufsize  = framesize * sizeof(float);

    dz->buflen = bigbufsize / sizeof(float);    
    dz->buflen = (dz->buflen / framesize)  * framesize;
    bigbufsize = dz->buflen * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize  * dz->bufcnt)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    for(n=0;n<dz->bufcnt;n++)
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    return(FINISHED);
}

/****************************** GET_THE_MODE_FROM_CMDLINE *********************************/

int get_the_mode_from_cmdline(char *str,dataptr dz)
{
    char temp[200], *p;
    if(sscanf(str,"%s",temp)!=1) {
        fprintf(stderr,"Cannot read mode of program.\n");
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

/******************************** READ_WAVETABLE ********************************
 *
 *  Wavefrom is in a table of size SYNTH_TABSIZE.
 *  This could be a sintable, or any regular (symmetrical waveform)
 *  Or, I guess, a short sample with some sort of symmetry so that when it phase-wraps
 *  it is continuous.
 */

double read_wavetable(double tabpos,dataptr dz)
{
    int loindex, hiindex;
    double loval, hival, valdiff, timefrac, val, *sintab = dz->parray[0];
    loindex = (int)floor(tabpos);
    hiindex = loindex + 1;
    loval   = sintab[loindex];
    hival   = sintab[hiindex];
    valdiff = hival - loval;
    timefrac = tabpos - (double)loindex;
    val = loval + (valdiff * timefrac);
    return val;
}

/******************************** SETUP_READTABLE **************************/

int setup_readtable(dataptr dz) 
{
    double *sintab;
    int n;
    if((dz->parray = (double **)malloc(1 * sizeof(double **)))==NULL) { // sin table
        sprintf(errstr,"INSUFFICIENT MEMORY for table arrays.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc((SYNTH_TABSIZE + 1) * sizeof(double)))==NULL) {    // sin table
        sprintf(errstr,"INSUFFICIENT MEMORY for wave table.\n");
        return(MEMORY_ERROR);
    }
    sintab = dz->parray[0];
    for(n=0;n<SYNTH_TABSIZE;n++)
        sintab[n] = sin(PI * 2.0 * ((double)n/(double)SYNTH_TABSIZE));                              
    sintab[n] = sintab[0];                          /* wrap around point */

    return FINISHED;
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"multiosc")) {
        fprintf(stdout,
        "USAGE:\n"
        "multiosc multiosc 1 outfile dur frq1 frq2 amp2 srate dovesplice\n"
        "multiosc multiosc 2 outfile dur frq1 frq2 amp2 frq3 amp3 srate dovesplice\n"
        "multiosc multiosc 3 outfile dur frq1 frq2 amp2 frq3 amp3 frq4 amp4 srate dovesplice\n"
        "\n"
        "\n"
        "MODE 1 Oscillation of oscillation.\n"
        "MODE 2 Oscillation of oscillation of oscillation.\n"
        "MODE 3 Oscillation of oscillation of oscillation of oscillation.\n"
        "\n"
        "DUR        Duration of synthesized sound, or textfile output.\n"
        "FRQ1       (Possibly time-varying) driving frequency of output.\n"
        "FRQ2 etc   Nested (possibly time-varying) oscillations of table read.\n"
        "AMP2 etc   Amplitude of suboscillations (Range 0 - 1).\n"
        "SRATE      Sample rate of synthesized sound.\n"
        "DOVESPLICE Length of splice of start and end of output sound (mS).\n"
        "\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

/*********************************************** MULTIOSC *****************************************/

int multiosc(dataptr dz) 
{
    int exit_status, oscilcnt = 0;
    double time, val, srate, onehzincr /* , spliceincr = 0.0 , upspliceval = 0.0, dnspliceval = 1.0 */;
//    int infade = 0, outfade = 0;
    float *obuf = dz->sampbuf[0];
    int obufpos = 0, outsamps = 0;
    double tabpos = 0.0, tabpos2 = 0.0, tabpos3 = 0.0, tabpos4 = 0.0;
    switch(dz->mode) {
    case(QUADRUPLE_OSC): oscilcnt = 4;  break;
    case(TRIPLE_OSC):    oscilcnt = 3;  break;
    case(DOUBLE_OSC):    oscilcnt = 2;  break;
    }
    srate = (double)dz->iparam[MOSC_SRATE];
//    infade = (int)floor(dz->param[MOSC_SPLEN] * MS_TO_SECS * srate);
//    outfade = dz->iparam[MOSC_DUR] - infade;
//    spliceincr = 1.0/infade;
//    upspliceval = 0.0;
//    dnspliceval = 1.0 - spliceincr;
    onehzincr = (double)SYNTH_TABSIZE/srate;
    time = 0.0;
    while(outsamps < dz->iparam[MOSC_DUR]) {
        if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
            return exit_status;
        switch(dz->mode) {
        case(QUADRUPLE_OSC):
            if(dz->brksize[MOSC_AMP4])
                dz->param[MOSC_AMP4] *= (double)SYNTH_TABSIZE;
            //   fall thro
        case(TRIPLE_OSC):
            if(dz->brksize[MOSC_AMP3])
                dz->param[MOSC_AMP3] *= (double)SYNTH_TABSIZE;
            //   fall thro
        case(DOUBLE_OSC):
            if(dz->brksize[MOSC_AMP2])
                dz->param[MOSC_AMP2] *= (double)SYNTH_TABSIZE;
            break;
        }
        val = multioscillator(onehzincr,&tabpos,&tabpos2,&tabpos3,&tabpos4,oscilcnt,dz);
        obuf[obufpos++] = (float)(MOSCAMP * val);
        if(obufpos >= dz->buflen) {                 //  If sound-buffer full, write sound output
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;
        }
        outsamps++;
        time = (double)outsamps/srate;
    }
    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/*********************************************** MULTIOSCILLATOR *****************************************/

double multioscillator(double onehzincr,double *tabpos,double *tabpos2,double *tabpos3,double *tabpos4,int oscilcnt,dataptr dz) 
{
    double tabincr, val, outval;
    outval = read_wavetable(*tabpos,dz);
    tabincr  = onehzincr * dz->param[MOSC_FRQ1];
    *tabpos  += tabincr;
    tabincr  = onehzincr * dz->param[MOSC_FRQ2];
    *tabpos2 += tabincr;
    if(oscilcnt > 2) {
        tabincr   = onehzincr * dz->param[MOSC_FRQ3];
        *tabpos3 += tabincr;
        if(oscilcnt > 3) {
            tabincr = onehzincr * dz->param[MOSC_FRQ4];
            *tabpos4 += tabincr;
            while(*tabpos4 >= SYNTH_TABSIZE)
                *tabpos4 -= SYNTH_TABSIZE;
            val = read_wavetable(*tabpos4,dz);
            *tabpos3 += val * dz->param[MOSC_AMP4] * SYNTH_TABSIZE; //  Val 0-1 scaled up to some proportion of the SYNTH_TABSIZE
        }
        while(*tabpos3 >= SYNTH_TABSIZE)
            *tabpos3 -= SYNTH_TABSIZE;
        while(*tabpos3 < 0)
            *tabpos3 += SYNTH_TABSIZE;
        val = read_wavetable(*tabpos3,dz);
        *tabpos2 += val * dz->param[MOSC_AMP3] * SYNTH_TABSIZE;
    }
    while(*tabpos2 >= SYNTH_TABSIZE)
        *tabpos2 -= SYNTH_TABSIZE;
    while(*tabpos2 < 0)
        *tabpos2 += SYNTH_TABSIZE;
    val = read_wavetable(*tabpos2,dz);
    *tabpos += val * dz->param[MOSC_AMP2] * SYNTH_TABSIZE; //   Val 0-1 scaled up to some proportion of the SYNTH_TABSIZE
    while(*tabpos >= SYNTH_TABSIZE)
        *tabpos -= SYNTH_TABSIZE;
    while(*tabpos < 0)
        *tabpos += SYNTH_TABSIZE;
    return outval;
}
