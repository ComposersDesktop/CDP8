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
/*
 * Need to separate lparrays into INTEGER and UNSIGNED LONG
 * THEN try compiling
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

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int check_tesselate_param_validity_and_consistency(dataptr dz);
static int setup_tesselate_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int handle_the_special_data(char *str,dataptr dz);
static int setup_tesselate_param_ranges_and_defaults(dataptr dz);
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
//static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int tesselate(dataptr dz);
static int create_tesselate_sndbufs(dataptr dz);
static int copy_from_src(int srcno, int to, int *maxwrite, int offset, unsigned int samps_processed, unsigned int endsamp, dataptr dz);

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
        dz->maxmode = 0;
        // setup_particular_application =
        if((exit_status = setup_tesselate_application(dz))<0) {
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
    if((exit_status = setup_tesselate_param_ranges_and_defaults(dz))<0) {
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
    if(dz->infilecnt > 1) {
        if((exit_status = handle_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);      
            return(FAILED);
        }
    }
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
    if((exit_status = check_tesselate_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    if((exit_status = create_tesselate_sndbufs(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //spec_process_file =
    if((exit_status = tesselate(dz))<0) {
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

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
    int exit_status;
    dz->infile->channels = dz->iparam[TESS_CHANS];
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

/************************* SETUP_TESSELATE_APPLICATION *******************/

int setup_tesselate_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,TESSELATION   ,4,4,"iddi"))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
        return(FAILED);
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = ONE_OR_MANY_SNDFILES;
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

/************************* SETUP_MADRID_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_tesselate_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[TESS_CHANS]  = 2;
    ap->hi[TESS_CHANS]  = 16;
    ap->default_val[TESS_CHANS] = 8;
    ap->lo[TESS_PHRAS]  = FLTERR;
    ap->hi[TESS_PHRAS]  = 60;
    ap->default_val[TESS_PHRAS] = 1.0;
    ap->lo[TESS_DUR]    = 1;
    ap->hi[TESS_DUR]    = 7200;
    ap->default_val[TESS_DUR] = 60;
    ap->lo[TESS_TYP]    = 0;
    ap->hi[TESS_TYP]    = 15;
    ap->default_val[TESS_TYP] = 0;
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
            if((exit_status = setup_tesselate_application(dz))<0)
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
    usage2("tesselate");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"tesselate"))               dz->process = TESSELATE;
    else {
        fprintf(stdout,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        fflush(stdout);
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
    if(!strcmp(str,"tesselate")) {
        fprintf(stdout,
        "USAGE:\n"
        "tesselate tesselate inf [inf2 ...] outf datafile chans cycledur outdur type [-ffrom]\n"
        "\n"
        "Create repeating-pattern-with-shift in space and time.\n"
        "Each (mono) source repeated on pair(s) of channels,\n"
        "But with slightly different delay in the paired channels.\n"
        "Shortest repeat-time (without the extra delay) = \"cycledur\".\n"
        "\n"
        "CHANS     Number of output channels (>= 2) and an even number.\n"
        "CYCLEDUR  The delay between repetitions (same for every source)\n"
        "          (Sources can have staggered entry, so they form a rhythmic phrase)\n"
        "          see \"datafile\", line 2.\n"
        "          Increase in delay on paired-channel(s) determined by \"datafile\" - line 2.\n" 
        "OUTDUR    Duration of output sound.\n"
        "TYPE      0: Delay drift between odd and even channels (e.g. 1357-v-2468).\n"
        "          1: Delay drift between adjacent channels  (1-v-2, 2-v-3, 3-v-4 etc).\n"
        "          2: Delay drift between alternate channels (1-v-3, 2-v-4, 3-v-5 etc).\n"
        "          3 :Delay drift between every 3rd channel  (1-v-4, 2-v-5, 3-v-6 etc).\n"
        "          and so on.\n"
        "FROM      Pattern normally starts with all chans in sync at time zero.\n"
        "          To start later in pattern, specify an integer number of cycles\n"
        "          from which to start outputting sound.\n"
        "\n"
        "DATAFILE  textfile containing two lines, with the same number of entries per line,\n"
        "          and the number of entries corresponds to the number of input files..\n"
        "          Line 1 - Lists number of repeats before time-delayed repeat-cycle\n"
        "                   resychronises with the \"cycledur\"-delayed src.\n"
        "                   e.g.  with value 5 \n"
        "                   \"cycledur\"-delayed src x    x    x    x    x    x    x\n"
        "                   timedelayed src        x     x     x     x     x     x\n"
        "          Line 2 - Time delay of initial entry of each src.\n"
        "                   Allows sources to be arranged in some initial rhythmic order.\n"
        "                   e.g.  0.0  0.1  0.2   0.3   0.4   0.45  0.55  0.7\n"
        "                   Maximum time-delay must be less than \"cycledur\".\n"
        "                   All vals must be different.\n"
        "                   (With same value, 2 sources would collapse into one double-src).\n"
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

/******************************** TESSELATE ********************************/

int tesselate(dataptr dz)
{
    int exit_status, srcno, chan, layercnt, layerno, passno;
    int safety = 4, outchancnt = dz->iparam[TESS_CHANS], tesstyp = dz->iparam[TESS_TYP];
    unsigned int samps_processed = 0, endsamp;
    int maxwrite = 0, offset = 0, phrasdur_samps_mono, orig_buflen, n;
    double maxsamp = 0.0, normaliser = 1.0, expand;
    int *synccyle      = dz->iparray[0];
    int *staggersamps = dz->lparray[0];
    float *obuf  = dz->sampbuf[dz->infilecnt], *ovflw = dz->sampbuf[dz->infilecnt + 1], *ibuf, *origobuf;
    int *local_sampcnt, *current_timestep;

    phrasdur_samps_mono  = (int)ceil(dz->param[TESS_PHRAS] * dz->infile->srate);    //  length of shortest repeat-cycle, in samples, in input (mono) frame
    endsamp  = (unsigned int)ceil(dz->param[TESS_DUR] * dz->infile->srate);
    endsamp *= outchancnt;                                                          //  final sample to generate, in output (multichannel) frame
    layercnt = dz->infilecnt * outchancnt;                                          //  number of repeating layers  
//  if(dz->iparam[TESS_FROM] > 0) {                                         
//      offset = dz->iparam[TESS_FROM] * phrasdur_samps_mono * outchancnt;          //  If output starts after (syncd) start of the tesselation process
//      endsamp += offset;                                                          //  Offset the "endsamp" appropriately, to retain same output duration.
//  }
    orig_buflen = dz->buflen;                                                       
    for(n=0;n<dz->infilecnt;n++) {                                                  //  Read all input files into respective buffers.
        dz->buflen = dz->insams[n] + safety;
        ibuf = dz->sampbuf[n];
        if((dz->ssampsread = fgetfbufEx(ibuf, dz->buflen,dz->ifd[n],0)) < 0) {
            sprintf(errstr,"Can't read samples from input soundfile %d.\n",n+1);
            return(SYSTEM_ERROR);
        }
    }
    dz->buflen = orig_buflen;
    origobuf = obuf;

    //  There are "layercnt" repeating streams in the tesseleation, and all need accounting procedures
    
    if((current_timestep = (int *)malloc(layercnt * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory for stream timesteps.\n");
        return(MEMORY_ERROR);
    }
    if((local_sampcnt = (int *)malloc(layercnt * sizeof(int)))==NULL) {
        sprintf(errstr,"Insufficient memory for stream sample-counters.\n");
        return(MEMORY_ERROR);
    }
    //  INTIALISATION

    for(passno = 0;passno < 2; passno++) {

        samps_processed = 0;                                    

        for(srcno = 0; srcno < dz->infilecnt; srcno++) {            //  For every source (srcno)
            for(chan = 0; chan < outchancnt; chan++) {              //  For every channel (chan)
                layerno = (srcno * outchancnt) + chan;
                local_sampcnt[layerno] = staggersamps[srcno];       //  Initial sample-offset of repeating layer (srcno-chan).
                local_sampcnt[layerno] += chan;                     //  offset by channel it occupies in output
                expand = 1.0 + (1.0/(double)synccyle[srcno]);       //  Delay of associated cycle
                switch(tesstyp) {
                case(0):                                            //  1234 v 5678
                    if(EVEN(chan)) 
                        current_timestep[layerno] = phrasdur_samps_mono;
                    else
                        current_timestep[layerno] = (int)round(phrasdur_samps_mono * expand);
                    break;
                default:
                    if(srcno == chan)                               //  standard delay channel
                        current_timestep[layerno] = phrasdur_samps_mono;
                    else if((srcno + tesstyp) % outchancnt == chan) //  associated longer-delayed channel.
                        current_timestep[layerno] = (int)round(phrasdur_samps_mono * expand);
                    else                
                        current_timestep[layerno] = 0;              //  In all other cases, the source is not copied
                    break;
                }
                current_timestep[layerno] *= outchancnt;            // convert samp-step to output frame
            }
        }
        memset((char *)obuf,0,dz->buflen * sizeof(float));      //  (re)initialise output buffers.
        memset((char *)ovflw,0,dz->buflen * sizeof(float));
        maxsamp = 0;                                            //  Initialise maxsample-finder

        while(samps_processed < endsamp) {          //  Until we reach the last required sample

            //  PROCESS EVERY SOURCE FOR EVERY CHANNEL UNTIL BUFFER END IS REACHED

            maxwrite = 0;                                           // Initialise position of last sample written into buffer
            
            for(srcno = 0; srcno < dz->infilecnt; srcno++) {        //  For every source
                for(chan = 0; chan < outchancnt; chan++) {          //  For every channel
                    layerno = (srcno * outchancnt) + chan;          //  Set index to all associated arrays
                    if(current_timestep[layerno] == 0)              //  Don't copy source into channels with no timestep
                        continue;
                    
                    while(local_sampcnt[layerno] < dz->buflen) {        //  Write to output, until we reach buffer end
                        if((exit_status = copy_from_src(srcno,local_sampcnt[layerno],&maxwrite,offset,samps_processed,endsamp,dz))<0)
                            return exit_status;
                        local_sampcnt[layerno] += current_timestep[layerno];
                    }
                }
            }

            //  WRITE OUT COMPLETED BUFFER, THEN COPY OVERFLOW BACK TO BUFFER, AND CONTINUE

            samps_processed += dz->buflen;

            if(offset) {

                //  IF SKIPPING EARLY CYCLES, curtail buffer

                if(samps_processed > (unsigned int)offset) {    //  Check if we've reached the offset
                    offset %= dz->buflen;                       //  find offset position in  buffer
                    obuf += offset;                             //  Write from offset position only 
                    dz->buflen -= offset;
                    offset = 0;                                 //  switch off "offset", now we've passed it
                }

            } else {

                //  IF WE'VE REACHED END extend buffer

                if(samps_processed >= endsamp) {
                    maxwrite = maxwrite/dz->iparam[TESS_CHANS]; //  deal with everything in the outbuf (including any overflow).
                    maxwrite++;                                 //  Adjusting to ensure that a complete group of N-samples is considered
                    maxwrite  *= dz->iparam[TESS_CHANS];
                    dz->buflen = maxwrite;
                }

                //  OTHERWISE (buflen does not change) WE WRITE A COMPLETE BUFFER
            }   
            switch(passno) {
            case(0):                                        //  On first pass, find maximum sample in buffer
                for(n=0;n<dz->buflen;n++)
                    maxsamp = max(maxsamp,fabs(obuf[n]));
                break;
            case(1):                                        //  On second pass, normalise output, and write to file
                for(n=0;n<dz->buflen;n++)
                    obuf[n] = (float)(obuf[n] * normaliser);
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)       
                    return(exit_status);
                break;
            }
            dz->buflen = orig_buflen;                           //  Reset buffer start and buflen for next pass
            obuf = origobuf;

            memcpy((char *)obuf,(char *)ovflw,dz->buflen * sizeof(float));
            memset((char *)ovflw,0,dz->buflen * sizeof(float)); //  Copy overflow back into outbuffer, and zero overflow.
            for(layerno=0;layerno<layercnt;layerno++)
                local_sampcnt[layerno] -= dz->buflen;           //  Adjust position of start of next write, in buffer, for every layer
            maxwrite -= dz->buflen;                             //  readjust location-in-buffer of maximum-sample-written
        }
        if(passno == 0)                                         //  Once all processing has concluded...        
            normaliser = 0.95/maxsamp;                          //  On 1st pass, use "maxsamp" to calculate normaliser.
    }
    return FINISHED;
}

/**************************** CREATE_TESSELATE_SNDBUFS ****************************/

int create_tesselate_sndbufs(dataptr dz)
{
    int n, safety = 4;
    unsigned int lastbigbufsize = 0, bigbufsize = 0, maxinbufsize = 0, offset;
    unsigned int obufsize, ovflwbufsize;
    int *staggersamps = dz->lparray[0];

    dz->bufcnt = dz->infilecnt+2;
    if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    for(n=0;n<dz->infilecnt;n++) {
        offset = staggersamps[n];
        maxinbufsize = max(maxinbufsize,(unsigned int)((dz->insams[n] + offset) + safety));
        bigbufsize += (dz->insams[n] + safety) * sizeof(float);
        if(bigbufsize < lastbigbufsize) {
            sprintf(errstr,"Insufficient memory to store the input soundfiles in buffers.\n");
            return(MEMORY_ERROR);
        }
        lastbigbufsize = bigbufsize;
    }
    dz->buflen = maxinbufsize * dz->iparam[TESS_CHANS];     //  Output buffer is at least as big as longest input src + its delay
    if(dz->buflen < 0) {
        sprintf(errstr,"Insufficient memory to store output sound (a).\n");
        return(MEMORY_ERROR);
    }
    obufsize = dz->buflen * sizeof(float);
    if(obufsize < 0) {
        sprintf(errstr,"Insufficient memory to store output sound (b).\n");
        return(MEMORY_ERROR);
    }
    bigbufsize += obufsize; 
    if(bigbufsize < lastbigbufsize) {
        sprintf(errstr,"Insufficient memory to store the input soundfiles and create output buffer.\n");
        return(MEMORY_ERROR);
    }
    lastbigbufsize = bigbufsize;

    ovflwbufsize = obufsize;
    bigbufsize  += ovflwbufsize; 
    if(bigbufsize < lastbigbufsize) {
        sprintf(errstr,"Insufficient memory to store the input soundfiles and create overflow buffer.\n");
        return(MEMORY_ERROR);
    }
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[n] = dz->sampbuf[0] = dz->bigbuf;                                       //  inbuf 0
    for(n=1;n<dz->infilecnt;n++)                                                        //  inbufs 1 - N
        dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + dz->insams[n-1] + safety;
    dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + dz->insams[n-1] + safety;      //  obuf
    n++;
    dz->sbufptr[n] = dz->sampbuf[n] = dz->sampbuf[n-1] + dz->buflen;                    //  ovflwbuf
    n++;
    dz->sampbuf[n] = dz->sampbuf[n-1] + dz->buflen;                                     //  bufs_end
    return(FINISHED);
}

/**************************** COPY_FROM_SRC *****************************/

int copy_from_src(int srcno, int to, int *maxwrite, int offset, unsigned int samps_processed, unsigned int endsamp, dataptr dz)
{
    float *obuf = dz->sampbuf[dz->infilecnt];
    int n, k, thismaxwrite;
    unsigned int total_output_sampcnt_at_writestart, index_of_last_input_sample_read, index_of_last_output_sample_writ;
    total_output_sampcnt_at_writestart = samps_processed + to;
    index_of_last_input_sample_read  = dz->insams[srcno] - 1;                                       //  In INfile
    index_of_last_output_sample_writ = index_of_last_input_sample_read * dz->iparam[TESS_CHANS];    //  In OUTfile

    //  Only write to output if we're beyond any initial offset AND the write will NOT reach beyond the "endsamp" required

    if((total_output_sampcnt_at_writestart > (unsigned int)offset)
    && (total_output_sampcnt_at_writestart + index_of_last_output_sample_writ < endsamp)) {
        for(n = 0, k = to; n < dz->insams[srcno]; n++, k += dz->iparam[TESS_CHANS]) {
            obuf[to] = (float)(obuf[to] + dz->sampbuf[srcno][n]);
            to += dz->iparam[TESS_CHANS];
        }
        thismaxwrite = to + index_of_last_output_sample_writ + 1;   //  maxwrite (IN BUFFER) is 1 beyond last sample written
        *maxwrite = max(*maxwrite,thismaxwrite); 
    }
    return FINISHED;
}

/**************************** CHECK_TESSELATE_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_tesselate_param_validity_and_consistency(dataptr dz)
{
    int n, m;
    double *stagger    = dz->parray[0];
    int *staggersamps = dz->lparray[0];

    if(dz->param[TESS_PHRAS] > dz->param[TESS_DUR]) {
        sprintf(errstr,"Output duration (%lf) too short for phrase duration (%lf) specified.\n",dz->param[TESS_DUR],dz->param[TESS_PHRAS]);
        return(DATA_ERROR);
    }
    if(ODD(dz->iparam[TESS_CHANS])) {
        sprintf(errstr,"Number of output channels (%d) must be even.\n",dz->iparam[TESS_CHANS]);
        return(DATA_ERROR);
    }
    if(dz->iparam[TESS_TYP] >= dz->iparam[TESS_CHANS]) {
        sprintf(errstr,"Tesselation type (%d) must be less than the number of output channels (%d).\n",dz->iparam[TESS_TYP],dz->iparam[TESS_CHANS]);
        return(DATA_ERROR);
    }
    n = 0;
    while(n < dz->itemcnt) {
        if(stagger[n] >= dz->param[TESS_PHRAS]) {
            sprintf(errstr,"Stagger time (%lf) in line 3 of datafile, out of range (must be less than phase duration %lf).\n",stagger[n],dz->param[TESS_PHRAS]);
            return(DATA_ERROR);
        }                               //  Convert initial source delays to samples cnts in (multichan) outbuf.
        staggersamps[n] = (int)round(stagger[n] * dz->infile->srate) * dz->iparam[TESS_CHANS];
        n++;
    }
    n = 0;
    while(n < dz->itemcnt - 1) {
        m = n + 1;
        while(m < dz->itemcnt) {
            if(flteq(stagger[n],stagger[m])) {
                sprintf(errstr,"Not all staggered-entry-times in line 3 of datafile are distinct (e.g. %lf).\n",stagger[n]);
                return(DATA_ERROR);
            }
            m++;
        }
        n++;
    }
    return FINISHED;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
    double dummy = 0.0;
    FILE *fp;
    int cnt, linecnt, idummy;
    char temp[8000], *p;
    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read data.\n",str);
        return(DATA_ERROR);
    }
    linecnt = 0;
    while(fgets(temp,8000,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        cnt = 0;
        while(get_float_from_within_string(&p,&dummy)) {
            switch(linecnt) {
            case(0):
                idummy = (int)round(dummy);
                if(idummy < 2 || idummy > 1024) {
                    sprintf(errstr,"Resync count in line 2 of data, out of range (2 to 1024), in file %s\n",str);
                    return(DATA_ERROR);
                }
                break;
            case(1):
                if(dummy < 0.0) {
                    sprintf(errstr,"Invalid src-onset delay in line 3 of file %s\n",str);
                    return(DATA_ERROR);
                }
                break;
            }
            cnt++;
        }
        if(linecnt == 0) {
            dz->itemcnt = cnt;
            if(dz->itemcnt != dz->infilecnt) {
                sprintf(errstr,"No of data items (%d) in 1st line of file %s doesn't correspond to no of input files (%d)\n",dz->itemcnt,str,dz->infilecnt);
                return(DATA_ERROR);
            }
        } else {
            if(cnt != dz->itemcnt) {
                sprintf(errstr,"Different number of data items (%d %d) in different lines in file %s\n",dz->itemcnt,cnt,str);
                return(DATA_ERROR);
            }
        }
        linecnt++;
    }
    if(linecnt == 0) {
        sprintf(errstr,"No data found in file %s.\n",str);
        return(DATA_ERROR);
    }
    else if(linecnt != 2) {
        sprintf(errstr,"Should be 2 lines of data in file %s : found %d lines.\n",str,linecnt);
        return(DATA_ERROR);
    }
    fseek(fp,0,0);
    if((dz->parray = (double **)malloc(sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store stagger values.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store stagger values.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray = (int **)malloc(sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store stagger values.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[0] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store stagger values.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray = (int **)malloc(sizeof(int *)))==NULL) {   //RWD 2025 was just sizeof(int)
        sprintf(errstr,"INSUFFICIENT MEMORY to store integer data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[0] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {  
        sprintf(errstr,"INSUFFICIENT MEMORY to store initial-channel data.\n");
        return(MEMORY_ERROR);
    }
    fseek(fp,0,0);
    linecnt = 0;
    while(fgets(temp,8000,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        cnt = 0;
        while(get_float_from_within_string(&p,&dummy)) {
            switch(linecnt) {
            case(0):
                dz->iparray[0][cnt] = (int)round(dummy);
                break;
            case(1):
                dz->parray[0][cnt] = dummy;
                break;
            }
            cnt++;
        }
        linecnt++;
    }
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
        fflush(stdout);
    }
    return(FINISHED);
}
