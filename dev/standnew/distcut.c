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
/* maybe just in MinGW*/
#ifdef fileno
#undef fileno
#endif

#define fileno ringsize
#define ebuflen rampbrksize
#define badfile is_rectified

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
//static int check_gate_param_validity_and_consistency(dataptr dz);
static int setup_distcut_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_distcut_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int open_next_outfile(dataptr dz);
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
static int create_distcut_sndbufs1(dataptr dz);
static int create_distcut_sndbufs2(dataptr dz);
static int preprocess_distcut(dataptr dz);
static int distcut(dataptr dz);

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
        if((exit_status = setup_distcut_application(dz))<0) {
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
    if((exit_status = setup_distcut_param_ranges_and_defaults(dz))<0) {
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
//  check_param_validity_and_consistency() : redundant

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

    if((exit_status = create_distcut_sndbufs1(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess ......
    if((exit_status = preprocess_distcut(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

    //create_sndbufs =
    if((exit_status = create_distcut_sndbufs2(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //spec_process_file =
    if((exit_status = open_next_outfile(dz))<0)         //  Create first outfile
        return exit_status;
    if((exit_status = distcut(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(!dz->badfile) {
        if((exit_status = complete_output(dz))<0) {                                 // CDP LIB
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
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
    strcpy(dz->outfilename,filename);      
    p = filename + strlen(filename);
    p--;
    while(p > filename) {
        if(*p == '.') {
            *p = ENDOFSTR;      //  Snip off any extension
            break;
        }
        p--;
    }
    if(sloom) {                 //  If sloom, snip off trailing zero
        p = filename + strlen(filename);
        p--;    
        *p = ENDOFSTR;
    }
    if(dz->wordstor!=NULL)
        free_wordstors(dz);
    dz->all_words = 0;
    if((exit_status = store_filename(filename,dz))<0)
        return(exit_status);
    dz->fileno = 0;
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************ OPEN_NEXT_OUTFILE *********************/

int open_next_outfile(dataptr dz)
{
    int exit_status;
    char filename[400], temp[16];
    if(dz->fileno > 0) {
        if((exit_status = headwrite(dz->ofd,dz))<0)
            return(exit_status);
        if((exit_status = reset_peak_finder(dz))<0)
            return(exit_status);
        if(sndcloseEx(dz->ofd) < 0) {
            fprintf(stdout,"WARNING: Can't close output soundfile %s\n",dz->outfilename);
            fflush(stdout);
        }
        dz->ofd = -1;
        fprintf(stdout,"INFO: Writing File %d\n",dz->fileno);
        fflush(stdout);
    }
    strcpy(filename,dz->wordstor[0]);
    sprintf(temp,"%d",dz->fileno);
    strcat(filename,temp);
    strcat(filename,".wav");
    strcpy(dz->outfilename,filename);
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
        return(exit_status);
    dz->fileno++;
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

/************************* SETUP_DISTCUT_APPLICATION *******************/

int setup_distcut_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if(dz->mode == 0)
        exit_status = set_param_data(ap,0   ,3,2,"I0D");
    else
        exit_status = set_param_data(ap,0   ,3,3,"IID");
    if(exit_status<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"c",1,"d","",0,0,""))<0)
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

/************************* SETUP_DISTCUT_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_distcut_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[DCUT_CNT]    = 1;
    ap->hi[DCUT_CNT]    = 1000;
    ap->default_val[DCUT_CNT]   = 64;
    if(dz->mode==1) {
        ap->lo[DCUT_STP]    = 1;
        ap->hi[DCUT_STP]    = 1000;
        ap->default_val[DCUT_STP]   = 64;
    }
    ap->lo[DCUT_EXP]    = 0.02;
    ap->hi[DCUT_EXP]    = 50;
    ap->default_val[DCUT_EXP] = 1.0;
    ap->lo[DCUT_LIM]    = 0;
    ap->hi[DCUT_LIM]    = 96;
    ap->default_val[DCUT_LIM] = 40;
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
            if((exit_status = setup_distcut_application(dz))<0)
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
    usage2("distcut");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"distcut"))             dz->process = DISTCUT;
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
    if(!strcmp(str,"distcut")) {
        fprintf(stderr,
        "USAGE:\n"
        "distcut distcut 1 infile generic_outfilename cyclecnt exp [-climit]\n"
        "OR:\n"
        "distcut distcut 2 infile generic_outfilename cyclecnt cyclestep exp [-climit]\n"
        "\n"
        "Cut sound into elements with falling envelope.\n"
        "\n"
        "CYCLECNT  number of wavesets in each outfile.\n"
        "CYCLESTEP number of wavesets steps from start of one group to start of next.\n"
        "          (In mode 1 : cyclestep = cyclecnt : waveset-groups abutted and disjunct).\n"
        "EXP       Envelope Decay shape. 1 linear : >1 more rapid decay : < 1 less rapid.\n"
        "LIMIT     Minimum level of output events to accept (value 70 = -70dB).\n"
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

/*************************** PREPROCESS_DISTCUT **************************/

int preprocess_distcut(dataptr dz) {

    int exit_status, wavesetcnt = 0, wvlencnt = 0, kk, k, j, jj, phase, initial_phase = 0, done;
    float *ibuf = dz->sampbuf[0];
    double time = 0.0 , val, srate = (double)dz->infile->srate;
    int *envlen = NULL, *waveset_len = NULL, *envstart = NULL;
    int envcnt = 0, maxenvset = 0, waveset_sampcnt, env_sampcnt = 0, bufpos, waveset_group_start, waveset_group_len;

    if(flteq(dz->param[DCUT_LIM],0.0))
        dz->zeroset = 0;
    else {
        dz->zeroset = 1;
        val = -dz->param[DCUT_LIM];
        if(val<=MIN_DB_ON_16_BIT)
            val = 0.0;
        else {
            val  = -(val);
            val /= 20.0;
            val  = pow(10.0,val);
            val  = 1.0/(val);
        }
        dz->param[DCUT_LIM] = val;
    }
    if(dz->brksize[DCUT_CNT]) {
        if((exit_status = read_value_from_brktable(time,DCUT_CNT,dz))<0)
            return(exit_status);
    }
    for(kk=0;kk<2;kk++) {
        if((sndseekEx(dz->ifd[0],0,0) < 0)){
            sprintf(errstr,"sndseek failed\n");
            return SYSTEM_ERROR;
        }
        dz->total_samps_read = 0;
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        bufpos = 0;
        waveset_sampcnt = 0;
        dz->samps_left = dz->insams[0];
        
        //  FIND INITIAL PHASE

        if(kk==0) {
            phase = 0;
            while(bufpos < dz->ssampsread) {
                if(ibuf[bufpos] > 0) {
                    initial_phase = 1;
                    phase = 1;
                } else if(ibuf[bufpos] < 0) {
                    initial_phase = -1;
                    phase = -1;
                }
                waveset_sampcnt++;
                if(++bufpos >= dz->buflen) {
                    if((exit_status = read_samps(ibuf,dz))<0)
                        return(exit_status);
                    bufpos = 0;
                }
                if(phase != 0)
                    break;
            }
            if(phase == 0) {
                sprintf(errstr,"No sound found in source soundfile.\n");
                return DATA_ERROR;
            }
        } else
            phase = initial_phase;
        
        //  READ AND COUNT WAVESETS

        while(bufpos < dz->ssampsread) {
            switch(initial_phase) {
            case(1):
                switch(phase) {
                case(1):
                    if(ibuf[bufpos] >= 0)
                        waveset_sampcnt++;
                    else {
                        phase = -1;
                        waveset_sampcnt++;
                    }
                    break;
                case(-1):
                    if(ibuf[bufpos] <= 0)
                        waveset_sampcnt++;
                    else {                                              //  Waveset completed   
                        wavesetcnt++;                                   //  Count the wavesets
                        if(kk == 0)                                     //  FIRST PASS
                            waveset_sampcnt = 0;                        //  Reset waveset-sample-count

                        else {                                          //  SECOND_PASS 
                            if(dz->mode==0) {                               //  Mode 0 stores lengths of waveset-groups to use
                                env_sampcnt += waveset_sampcnt;             //  Add wavesetcount to overall envelope-unit sample-count
                                if(wavesetcnt >= dz->iparam[DCUT_CNT]) {    //  If we have enough wavesets for the envelope
                                    envlen[envcnt++] = env_sampcnt;         //  Store the envelope-unit sample-count
                                    maxenvset = max(maxenvset,env_sampcnt); //  Find largest envelope-unit sample-count
                                    env_sampcnt = 0;                        //  Rest envelope-unit sample-count
                                    wavesetcnt = 0;                         //  Reset waveset count
                                    if(dz->brksize[DCUT_CNT]) {             //  If ness: find the new value of waveset-grouping
                                        time = (double)(dz->total_samps_read + bufpos)/srate;
                                        if((exit_status = read_value_from_brktable(time,DCUT_CNT,dz))<0)
                                            return(exit_status);
                                    }
                                }
                            } else                                          //  Mode 1 initially just stores size of wavesets
                                waveset_len[wvlencnt++] = waveset_sampcnt;
                        }
                        waveset_sampcnt = 0;                            //  Reset waveset-sample-count
                        phase = initial_phase;                          //  Reset phase
                    }
                    break;
                }
                break;
            case(-1):
                switch(phase) {
                case(-1):
                    if(ibuf[bufpos] <= 0)
                        waveset_sampcnt++;
                    else {
                        phase = 1;
                        waveset_sampcnt++;
                    }
                    break;
                case(1):
                    if(ibuf[bufpos] >= 0)
                        waveset_sampcnt++;
                    else {                                              //  Waveset completed   
                        wavesetcnt++;                                   //  Count the wavesets

                        if(kk == 0)                                     //  FIRST PASS
                            waveset_sampcnt = 0;                        //  Reset waveset-sample-count

                        else {                                          //  SECOND_PASS 
                            if(dz->mode==0) {                               //  Mode 0 stores lengths of waveset-groups to use
                                env_sampcnt += waveset_sampcnt;             //  Add wavesetcount to overall envelope-unit sample-count
                                if(wavesetcnt >= dz->iparam[DCUT_CNT]) {    //  If we have enough wavesets for the envelope
                                    envlen[envcnt++] = env_sampcnt;         //  Store the envelope-unit sample-count
                                    maxenvset = max(maxenvset,env_sampcnt); //  Find largest envelope-unit sample-count
                                    env_sampcnt = 0;                        //  Rest envelope-unit sample-count
                                    wavesetcnt = 0;                         //  Reset waveset count
                                    if(dz->brksize[DCUT_CNT]) {             //  If ness: find the new value of waveset-grouping
                                        time = (double)(dz->total_samps_read + bufpos)/srate;
                                        if((exit_status = read_value_from_brktable(time,DCUT_CNT,dz))<0)
                                            return(exit_status);
                                    }
                                }
                            } else                                          //  Mode 1 initially just stores size of wavesets
                                waveset_len[wvlencnt++] = waveset_sampcnt;
                        }
                        waveset_sampcnt = 0;                            //  Reset waveset-sample-count
                        phase = initial_phase;                          //  Reset phase
                    }
                    break;
                }
                break;
            }
            if(++bufpos >= dz->buflen) {
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                bufpos = 0;
            }
        }
        if(kk == 0) {
            if(wavesetcnt <= 0) {
                sprintf(errstr,"No complete wavesets found.\n");
                return(DATA_ERROR);
            }
            if(dz->mode == 1) {
                if((dz->lparray = (int **)malloc(3 * sizeof(int *)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
                    return(MEMORY_ERROR);
                }
            } else {
                if((dz->lparray = (int **)malloc(sizeof(int *)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
                    return(MEMORY_ERROR);
                }
            }
            if((dz->lparray[0] = (int *)malloc(wavesetcnt * sizeof(int)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
                return(MEMORY_ERROR);
            }
            envlen = dz->lparray[0];
            if(dz->mode == 1) {
                if((dz->lparray[1] = (int *)malloc((wavesetcnt+1) * sizeof(int)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
                    return(MEMORY_ERROR);
                }
                if((dz->lparray[2] = (int *)malloc(wavesetcnt * sizeof(int)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY for internal int arrays.\n");
                    return(MEMORY_ERROR);
                }
                waveset_len = dz->lparray[2];
                envstart = dz->lparray[1];
            }
        } else {
            if(dz->mode == 0) {
                if(envcnt <= 0) {
                    sprintf(errstr,"No complete set of wavesets found.\n");
                    return(DATA_ERROR);
                }
            } else {                //  In mode 1, use lengths of individual wavesets to calculate envelope sizes, and start-samples
                done = 0;
                time = 0.0;
                waveset_group_start = 0;                        //  Sample-time of start of current waveset-group
                while(waveset_group_start < dz->insams[0]) {
                    if(dz->brksize[DCUT_CNT]) {
                        if((exit_status = read_value_from_brktable(time,DCUT_CNT,dz))<0)
                            return(exit_status);
                    }
                    if(dz->brksize[DCUT_STP]) {
                        if((exit_status = read_value_from_brktable(time,DCUT_STP,dz))<0)
                            return(exit_status);
                    }
                    envstart[envcnt] = waveset_group_start;     //  Start position (in samples) = (0 or) end of previous step
                    for(k=0,j = waveset_group_start;k < dz->iparam[DCUT_STP];k++,j++) {
                        if(j >= wvlencnt) {                     //  Ran off end of wavesets
                            done = 1;
                            break;
                        }
                        waveset_group_start += waveset_len[j];  //  Advance to next start position, summing sample_cnt as we go
                    }
                    if(done)
                        break;
                    waveset_group_len = 0;                      //  Go back to start of waveset_group: advance by size of envelope-group
                    for(k=0,jj = waveset_group_start;k < dz->iparam[DCUT_CNT];k++,jj++) {
                        if(jj >= wvlencnt) {                    //  Ran off end of wavesets
                            done = 1;
                            break;
                        }
                        waveset_group_len += waveset_len[j];    //  Summing samples as we go
                    }
                    if(done)
                        break;
                    envlen[envcnt] = waveset_group_len;         //  This gives us the length of the envelope here
                    maxenvset = max(maxenvset,waveset_group_len);
                    waveset_group_start = j;                    //  Reset the number of the waveset to start of next group
                    time = (double)(waveset_group_start)/srate;
                    envcnt++;
                }
                envstart[envcnt] = envstart[envcnt-1];          //  Wrap-around point so no anomalous read at table end
            }
            dz->itemcnt = envcnt;       //  Store number of envelopes to extract
            dz->ebuflen = maxenvset;    //  Store samplesize of maximum envelope-group
        }
    }       
    return FINISHED;
}

/*************************** CREATE_DISTCUT_SNDBUFS **************************/

int create_distcut_sndbufs1(dataptr dz)
{
    int framesize = F_SECSIZE;
    int bigbufsize;
    if(dz->sbufptr == 0 || dz->sampbuf==0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = framesize; // SAFETY
    dz->buflen = bigbufsize;
    bigbufsize  *= sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize * sizeof(float))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
    dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
    return(FINISHED);
}

/*************************** CREATE_DISTCUT_SNDBUFS **************************/

int create_distcut_sndbufs2(dataptr dz)
{
    int framesize = F_SECSIZE;
    int bigbufsize;
    bigbufsize = dz->ebuflen/framesize; // SAFETY
    if(bigbufsize * framesize != dz->ebuflen)
        bigbufsize++;
    bigbufsize *= framesize;
    dz->buflen = bigbufsize;
    bigbufsize += 16;   //  SAFETY
    if((dz->sampbuf[1] = (float *)malloc(bigbufsize * sizeof(float))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[1] = dz->sampbuf[1];
    dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
    return(FINISHED);
}

/*************************** DISTCUT **************************/

int distcut(dataptr dz)
{
    int exit_status, n;
    int samptime;
    float *buf = dz->sampbuf[1];
    double ee, maxsamp, time = 0.0, srate = (double)dz->infile->srate;
    int *envsize = dz->lparray[0], bufpos, *envstart = NULL;
    if(dz->mode==1)
        envstart = dz->lparray[1];
    dz->badfile = 0;
    samptime = 0;                                           //  Start at start of sound;
    for(n = 0;n < dz->itemcnt; n++) {                       //  For every envelope-group needed
        memset((char *)buf,0,dz->ebuflen * sizeof(float));  //  preset obuf to 0
        if((sndseekEx(dz->ifd[0],samptime,0) < 0)) {        //  Seek to start of envelope group
            sprintf(errstr,"sndseek failed\n");
            return SYSTEM_ERROR;
        }
        dz->buflen = envsize[n];
        if((exit_status = read_samps(buf,dz))<0)
            return(exit_status);

        for(bufpos=0;bufpos < envsize[n];bufpos++) {        //  Envelope the samples
            ee = 1.0 - ((double)bufpos/(double)envsize[n]);
            ee = pow(ee,dz->param[DCUT_EXP]);
            buf[bufpos] = (float)(buf[bufpos] * ee);
        }                                                   //  Write envelope-group to file
        if(dz->zeroset) {
            maxsamp = 0.0;
            for(bufpos=0;bufpos < envsize[n];bufpos++) {
                if(fabs(buf[bufpos]) > maxsamp)
                    maxsamp = fabs(buf[bufpos]);
            }
            if(maxsamp < dz->param[DCUT_LIM]) {
                dz->badfile = envsize[n];
                continue;
            }
        }
        dz->badfile = 0;
        if((exit_status = write_samps(buf,envsize[n],dz))<0)
            return(exit_status);    
        if(n < dz->itemcnt - 1) {
            if((exit_status = open_next_outfile(dz))<0)         //  Get next file
                return exit_status;
        }
        if(dz->mode==0)
            samptime += envsize[n];                             //  Advance to next envelope-group of samples
        else
            samptime = envstart[n+1];                           
        if(dz->brksize[DCUT_EXP]) {
            time = (double)samptime/srate;                      //  If EXP varies, read brktable
            if((exit_status = read_value_from_brktable(time,DCUT_EXP,dz))<0)
                return(exit_status);
        }
    }
    if(dz->badfile) {                                           //  If the last file opened was not written to (too low level)
        if((exit_status = write_samps(buf,dz->badfile,dz))<0)   //  Write data to it: this allows sfsys to truncate the file in sndcloseEx
            return(exit_status);                                //  Then delete it
        if(sndunlink(dz->ofd) < 0) {
            fprintf(stdout,"WARNING: Can't set output soundfile %s for deletion.\n",dz->outfilename);
            fflush(stdout);
        }
        if(sndcloseEx(dz->ofd) < 0) {
            fprintf(stdout,"WARNING: Can't close output soundfile %s :%s\n",dz->outfilename,sferrstr());        //RWD sferrstr is function
            fflush(stdout);
        }
        dz->ofd = -1;
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

