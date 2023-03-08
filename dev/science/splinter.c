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
 
//  _cdprogs\splinter splinter 1 testfrac.wav testsplint2.wav 0.710884 240 16 16 10 10 -e0 -s1 -p1 -f6000 -r0 -v0
//CRASHES ..... seems to be the waveset count that crashes it

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

#define QCYCLEMIN   2           //  Minimum size of a shrunk quartercycle
#define SPLENVWIN   20          //  Windowsize, in mS, for extracting envelope, used to normalise output, if "-i" flag set.
#define SPLMAXSPL   10          //  Max duration of selected waveset-group = 10 seconds
#define SPLSPLICE   2           //  Splicelen in mS

#define quartercyclecnt is_mapping      //  number of values marking the zero-crossings, maxima and minima of wavesetgroup
#define envwindowsize   rampbrksize     //  sample-size of window used for normalising envelope
#define wavesetgrplen   total_windows   //  Length of original wavesetgroup (samples)
#define target          wlength         //  Position of wavesetgroup in infile  (samples)
#define splintoffset    wanted          //  Length of splinter-set prior to wavesetgroup
#define arraysize       itemcnt
#define wmaxmpos        extra_word
#define splicelen       ringsize
#define enddur          all_words
#define minlen          numsize

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int check_splinter_param_validity_and_consistency(dataptr dz);
static int setup_splinter_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_splinter_param_ranges_and_defaults(dataptr dz);
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
static int splinter_param_preprocess(int *initial_phase,dataptr dz);
static int create_splinter_sndbufs1(dataptr dz);
static int generate_envelope_array_storage(dataptr dz);
static int create_splinter_sndbufs2(dataptr dz);
static int find_wavesets(int *initial_phase,dataptr dz);
static int store_wavesets(int initial_phase,dataptr dz);
static int generate_shrinkage_set(dataptr dz);
static int generate_timing_set(dataptr dz);
static int shrink_waveset(double shrink,int *splinterlen,dataptr dz);
static int normalise_buffer(int windowing_end,dataptr dz);
static int create_temp_sndbuf(dataptr dz);
static int splinter(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status, initial_phase = 0;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
    int n;
//  aplptr ap;
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
        dz->maxmode = 4;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_splinter_application(dz))<0) {
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
    if((exit_status = setup_splinter_param_ranges_and_defaults(dz))<0) {
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
//  check_param_validity_and_consistency....
    if((exit_status = check_splinter_param_validity_and_consistency(dz))<0) {
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

    //param_preprocess() ...
    if((exit_status = splinter_param_preprocess(&initial_phase,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }   //spec_process_file =
    if((exit_status = create_splinter_sndbufs1(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = generate_envelope_array_storage(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = create_splinter_sndbufs2(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = store_wavesets(initial_phase,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = splinter(dz))<0) {
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

/************************* SETUP_SPLINTER_APPLICATION *******************/

int setup_splinter_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,0   ,6,6,"diiidd"))<0)
        return(FAILED);
    if(dz->mode <= 1)
        exit_status = set_vflgs(ap,"espfrv",6,"idddDD","iI",2,0,"00");
    else
        exit_status = set_vflgs(ap,"espdrv",6,"idddDD","iI",2,0,"00");
    if(exit_status<0)
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

/************************* SETUP_SPLINTER_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_splinter_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[SPL_TIME]    = 0.0;
    ap->hi[SPL_TIME]    = dz->duration;
    ap->default_val[SPL_TIME]   = 0.0;
    ap->lo[SPL_WCNT]    = 0;
    ap->hi[SPL_WCNT]    = 256;
    ap->default_val[SPL_WCNT]   = 1;
    ap->lo[SPL_SHRCNT]  = 2;
    ap->hi[SPL_SHRCNT]  = 256;
    ap->default_val[SPL_SHRCNT] = SHRCNT_DFLT;
    ap->lo[SPL_OCNT]    = 0;
    ap->hi[SPL_OCNT]    = 256;
    ap->default_val[SPL_OCNT]   = OCNT_DFLT;
    ap->lo[SPL_PULS1]   = 0;
    ap->hi[SPL_PULS1]   = 50;
    ap->default_val[SPL_PULS1]  = PULS_DFLT;
    ap->lo[SPL_PULS2]   = 0;
    ap->hi[SPL_PULS2]   = 50;
    ap->default_val[SPL_PULS2]  = PULS_DFLT;
    ap->lo[SPL_ECNT]    = 0;
    ap->hi[SPL_ECNT]    = 10000;
    ap->default_val[SPL_ECNT]   = 0;
    ap->lo[SPL_SCURVE]  = 0.1;
    ap->hi[SPL_SCURVE]  = 10.0;
    ap->default_val[SPL_SCURVE] = 1;
    ap->lo[SPL_PCURVE]  = 0.1;
    ap->hi[SPL_PCURVE]  = 10.0;
    ap->default_val[SPL_PCURVE] = 1;
    if(dz->mode <= 1) {
        ap->lo[SPL_FRQ] = 1000.0;
        ap->hi[SPL_FRQ] = dz->nyquist/2.0;
        ap->default_val[SPL_FRQ]   = FREQ_DFLT;
    } else {
        ap->lo[SPL_DUR] = 5.0;
        ap->hi[SPL_DUR] = 50.0;
        ap->default_val[SPL_DUR]   = 5.0;
    }
    ap->lo[SPL_RND] = 0.0;
    ap->hi[SPL_RND] = 1.0;
    ap->default_val[SPL_RND] = 0;
    ap->lo[SPL_SHRND] = 0.0;
    ap->hi[SPL_SHRND] = 1.0;
    ap->default_val[SPL_SHRND] = 0;
    dz->maxmode = 4;
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
 //   aplptr ap;

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
            if((exit_status = setup_splinter_application(dz))<0)
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
    usage2("splinter");
    return(USAGE_ONLY);
}

/**************************** CHECK_SPLINTER_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_splinter_param_validity_and_consistency(dataptr dz)
{   
    if(dz->param[SPL_PULS1] > 0.0 && dz->param[SPL_PULS1] < 0.1) {
        fprintf(stderr,"Pulse frq where splinters join the original sound, cannot be less than 0.1.\n");
        return DATA_ERROR;
    }
    if(dz->param[SPL_PULS2] > 0.0 && dz->param[SPL_PULS2] < 0.1) {
        if(dz->mode == 0 || dz->mode == 2)
            fprintf(stderr,"Pulse frq where splintering begins, cannot be less than 0.1.\n");
        else
            fprintf(stderr,"Pulse frq where splintering ends, cannot be less than 0.1.\n");
        return DATA_ERROR;
    }
    return FINISHED;
}

/**************************** SPLINTER_PARAM_PREPROCESS *****************************/

int splinter_param_preprocess(int *initial_phase,dataptr dz)
{
    int exit_status;
//    double *shrinkage;

    if((dz->iparray = (int **)malloc(sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to envelope peak-position array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray = (int **)malloc(sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create minina/maxima store (0).\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[0] = (int *)malloc(dz->iparam[SPL_WCNT] * 4 * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create minina/maxima store.\n");
        return(MEMORY_ERROR);       //  Arrays stores sample position max(or min) in each half-waveset, and end-samples of half-waveset
    }
    if((dz->parray = (double **)malloc(3 * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create splintering arrays.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc(dz->iparam[SPL_SHRCNT] * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create shrinkage data array.\n");
        return(MEMORY_ERROR);       //  Arrays stores shrink factor dor successive shrunk waveset-groups
    }
//    shrinkage = dz->parray[0];
    if((dz->parray[1] = (double *)malloc((dz->iparam[SPL_SHRCNT] + dz->iparam[SPL_OCNT] + 1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create splinter timing array.\n");
        return(MEMORY_ERROR);       //  Arrays stores timings of successive pulses
    }
    if((exit_status = create_temp_sndbuf(dz))<0)                    //  Find and store the the required waveset-group
        return exit_status;
    
    if((exit_status = find_wavesets(initial_phase,dz))<0)           //  Find and store the the required waveset-group
        return exit_status;
    free(dz->bigbuf);
    if((exit_status = generate_shrinkage_set(dz))<0)                //  Generate the sequence of wavesetgroup-shrinkages
        return exit_status;
    if((exit_status = generate_timing_set(dz))<0)                   //  Generate onset times for the shrinking wavesetgroups
        return exit_status;
    dz->splicelen = (int)round(SPLSPLICE * MS_TO_SECS * dz->infile->srate);
    return FINISHED;
}

/**************************** GENERATE_ENVELOPE_ARRAY_STORAGE *************************************
 *
 *  Buflen (so far) is a multiple of F_SECSIZE (256) .
 *  So this algorithm will, in the last resort, always find 256 or its divisors !!
 */

int generate_envelope_array_storage(dataptr dz)
{
    double srate = (double)dz->infile->srate;
    int ewcnt = 0, orig_envwindowsize = 0, hifound = 0, lofound = 0;

    dz->envwindowsize = (int)round(SPLENVWIN * MS_TO_SECS * srate); //  Envelope window-length, in samples
    ewcnt = dz->buflen/dz->envwindowsize;                               //  Number of windows that will fit inside buffer
    if(ewcnt * dz->envwindowsize != dz->buflen) {                       //  If window is not an exact divisor of buflen

        //  adjust envelope length to be an exact divisor of buflen

        orig_envwindowsize = dz->envwindowsize;
        hifound = 0;
        while(!hifound) {                                               //  Enlarge the window until windows fit exactly into buffer
            dz->envwindowsize++;
            ewcnt = dz->buflen/dz->envwindowsize;
            if(dz->envwindowsize * ewcnt == dz->buflen) {
                hifound = 1;
            }
        }
        hifound = dz->envwindowsize;                                    //  set "hifound" to size of this (larger) window

        dz->envwindowsize = orig_envwindowsize;
        lofound = 0;                                        
        while(!lofound) {                                               //  Shrink the window until windows fit exactly into buffer
            dz->envwindowsize--;
            ewcnt = dz->buflen/dz->envwindowsize;
            if(dz->envwindowsize * ewcnt == dz->buflen) {
                lofound = 1;
                break;
            }
        }
        lofound = dz->envwindowsize;                                    //  set "lofound" to size of this (smaller) window
            
                                            
        if(hifound - orig_envwindowsize > orig_envwindowsize - lofound)
            dz->envwindowsize = lofound;                                //  use windowsize nearest to original size 
        else
            dz->envwindowsize = hifound;
    }
    dz->arraysize = (int)ceil(dz->buflen/dz->envwindowsize) + 64;   // 64=SAFETY (but 2 of these are needed for the bracketing envelope segments
    if((dz->parray[2] = (double *)malloc(dz->arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create enveloping array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[0] = (int *)malloc(dz->arraysize * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create enveloping peak-position array.\n");
        return(MEMORY_ERROR);
    }
    return FINISHED;
}

/************************** GET_THE_PROCESS_NO **********************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"splinter"))                dz->process = SPLINTER;
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
    if(!strcmp(str,"splinter")) {
        fprintf(stderr,
        "USAGE:\n"
        "splinter splinter 1-4 infile outfile target wcnt shrcnt ocnt p1 p2\n"
        "        [-eecnt] [-sscv] [-ppcv] [-ffrq | -ddur] [-rrand] [-vshrand] [-i] [-I]\n"
        "\n"
        "Creates splinters by repeating & shrinking selected waveset-group in sound.\n"
        "Either splinters repeat before merging with orig snd at time-in-src specified.\n"
        "OR original sound plays up to selected time, then splinters,\n"
        "\n"
        "Mode 1+3: Splinters lead into original sound.    Mode 1 splinters change pitch.\n"
        "Mode 2+4: Splinters emerge from original sound.  Mode 2 splinters change pitch.\n"
        "\n"
        "TARGET Time in src immediately before desired waveset-group.\n"
        "       Waveset group selected should not be longer than 1 minute.\n"
        "WCNT   Number of wavesets to use to create splinter group.\n"
        "SHRCNT Number of waveset-group repets over which shrinkage takes place.\n"
        "OCNT   Number of max-shrunken splinters beyond shrink.\n"
        "P1     Pulse-speed of waveset repetitions (Hz) at originating waveset.\n"
        "P2     Pulse-speed of shrunken wavesets (Hz) SHRCNT+OCNT splinters away.\n"
        "ECNT   Number of additional regular pulses beyond SHRCNT+OCNT.\n"
        "\n"
        "SCV    Shrinkage curve: 1 = linear:\n"
        "       >1 contracts more rapidly near originating waveset; <1 less rapidly.\n"
        "PCV    Pulse-speed curve: 1 = linear:\n"
        "       >1 accels more rapidly near originating waveset; <1 less rapidly.\n"
        "\n"
        "FRQ    Modes1+2: approx Frq (1/wavelen) of max-shrunk splinters (dflt c6000Hz).\n"
        "       Resultant wavelen must be > average wavesetlen in wavesets chosen\n"
        "       and less than nyquist/2.\n"
        "DUR    Modes3+4: approx Duration (mS) of max-shrunk splinters.\n"
        "\n"
        "RAND   Randomisation of pulse timing. Time-variable parameter.\n"
        "SHRAND Randomisation of pulse shrinkage. Time-variable parameter.\n"
        "-i     Mix all source into output. (Default: use source only where no splinters)\n"
        "-I     Mix none of source into output.\n"
        "\n"
        "If P1 set to zero, Pulse-speed 1 determined by duration of selected waveset-group.\n"
        "If P2 set to zero, Pulse-speed 2 same as Pulse-speed 1.\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/******************************** SPLINTER ********************************/

int splinter(dataptr dz)
{
    int exit_status;
    int samps_to_read, obufpos = 0, startobufpos, lastobufpos, thisobufpos = 0, ibufpos = 0, sampstep = 0, envend, maxwrite, ovflw;
    int subtarget, splinterlen = 0, k, n, m, eventscnt, samps_to_write, rndstep,initialobufpos, advance;
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *ovflwbuf = dz->sampbuf[2], *shrbuf = dz->sampbuf[4];
    double srate = (double)dz->infile->srate, shrink = 1.0, last_shrink, initialshrink, thisshrink, finalshrink;
    double *shrinkage = dz->parray[0], *timeset = dz->parray[1], shrinkstep = 0, dur, time, thistime, rnd;

    sndseekEx(dz->ifd[0],0,0);
    dz->total_samps_read = 0;
    memset((char *)ibuf,0,dz->buflen * sizeof(float));
    memset((char *)obuf,0,dz->buflen * sizeof(float));

    if(dz->mode == 0 || dz->mode == 2) {        //  MODE 0 or 2: attack-type

        //  IN ALL CASE we read to dz->target + dz->wavesetgrplen in infile and write (whatever part required) into outfile 
        
        if(dz->vflag[0]) {                                          //  If we're mixing-in the original source
            if((exit_status = read_samps(ibuf,dz))<0)               //  Read the infile : NB bufsize calcd to accept all of infile up to target + wavesetgrplen
                return(exit_status);
            samps_to_read = dz->target + dz->wavesetgrplen;
            if(dz->target >= dz->splintoffset) {                    //  IF infile start is BEFORE OR AT start of splinters, then 
                                                                    //  the read will be up to (and beyond) the wavesetgroup (buffersize calcd thus)
                memcpy((char *)obuf,(char *)ibuf,samps_to_read * sizeof(float));
                obufpos = dz->target - dz->splintoffset;            //  Set obuf pointer to start of splinters

            } else {                                                //  Infile starts AFTER start of splinters
                obufpos = dz->splintoffset - dz->target;            //  Find appropriate place in obuf to copy this to, and copy it
                memcpy((char *)(obuf+obufpos),(char *)ibuf,samps_to_read * sizeof(float));
                obufpos = 0;
            }
            ibufpos = samps_to_read; 
        } else {                                                    //  Not mixing in INBUF
            sndseekEx(dz->ifd[0],dz->target,0);                     //  Seek to wavesetgrp start
            dz->total_samps_read = dz->target;
            if((exit_status = read_samps(ibuf,dz))<0)               //  Read the infile
                return(exit_status);
            obufpos = dz->splintoffset;                             //  Write to obuf at place where wavesetgrp will be in output
            samps_to_read = dz->wavesetgrplen;                      //  Copy the wavesetgrp into obuf
            memcpy((char *)(obuf+obufpos),(char *)ibuf,samps_to_read * sizeof(float));
            obufpos = 0;
            ibufpos = samps_to_read;
        }
        initialshrink = shrinkage[0];                                       //  Get maximal shrink
        if((exit_status = shrink_waveset(initialshrink,&splinterlen,dz))<0)//and shrink waveset
            return exit_status;
        if(dz->param[SPL_PULS2] <= 0.0) {                           //  Get goal timestep
            if(dz->param[SPL_PULS1] <= 0.0)                         //  ... in samples
                sampstep = dz->wavesetgrplen;
            else {
                dur = 1.0/dz->param[SPL_PULS1];
                sampstep = (int)ceil(dur * srate);
            }
        } else {
            dur = 1.0/dz->param[SPL_PULS2];
            sampstep = (int)round(dur * srate);
        }
        
        //  If shrinakge can be randomised ..

        if((dz->brksize[SPL_SHRND]) || (dz->param[SPL_SHRND] > 0.0))
            shrinkstep = shrinkage[1] - initialshrink;

        //  IF THERE'S A PREQUEL

        thisobufpos = obufpos;
        if(dz->iparam[SPL_ECNT] > 0) {
            for(n=0;n <dz->iparam[SPL_ECNT]; n++) {
                obufpos = thisobufpos;
                if(n > 0 && n < dz->iparam[SPL_ECNT] - 1) {
                    
                    if(dz->brksize[SPL_RND]) {
                        thistime = (double)(dz->total_samps_written + obufpos)/srate;
                        if((exit_status = read_value_from_brktable(thistime,SPL_RND,dz))<0)
                            return exit_status;
                    }
                    if(dz->param[SPL_RND] > 0.0) {
                        rnd = ((drand48() * 2.0) - 1.0)/2.0;        //  range +1/2, -1/2
                        rnd *= dz->param[SPL_RND];
                        rndstep = (int)round(rnd * sampstep);
                        obufpos += rndstep;
                    }                                               //  If no randomisation of shrink, shrunk-waveset remains in original state
                    if(dz->brksize[SPL_SHRND]) {                    //  But if shrink is randomised, find current randomisation
                        thistime = (double)(dz->total_samps_written + obufpos)/srate;
                        if((exit_status = read_value_from_brktable(thistime,SPL_SHRND,dz))<0)
                            return exit_status;
                    }
                    if(dz->param[SPL_SHRND] > 0.0) {                //  If randomised > 0
                        rnd = drand48() * dz->param[SPL_SHRND];     //  range 0 to SPL_SHRND
                        shrink = initialshrink + (shrinkstep * rnd);//  Possibly expand-or-contract shrinkage
                        if((exit_status = shrink_waveset(shrink,&splinterlen,dz))<0)//and shrink waveset
                            return exit_status;
                    } else if(dz->brksize[SPL_SHRND]) {             //  If varying randomisation, but currently zero
                        if((exit_status = shrink_waveset(initialshrink,&splinterlen,dz))<0)
                            return exit_status;                     //  still need to reshrink waveset (in case shrink-size changed previously)
                    }
                }
                for(m=0;m<splinterlen;m++) {
                    obuf[obufpos] = (float)(obuf[obufpos] + shrbuf[m]);
                    if(++obufpos >= dz->buflen) {
                        sprintf(errstr,"Buffer should be long enough to contain all prequel splinters. (1)\n");
                        return PROGRAM_ERROR;
                    }
                }
                lastobufpos = thisobufpos;
                thisobufpos += sampstep;
                if(thisobufpos >= dz->buflen) {
                    sprintf(errstr,"Buffer should be long enough to contain all prequel splinters. (2)\n");
                    return PROGRAM_ERROR;
                }
            }
        }
        //  TIMESET ITSELF
        
        eventscnt = dz->iparam[SPL_SHRCNT] + dz->iparam[SPL_OCNT];
        startobufpos = obufpos;
        lastobufpos  = startobufpos;
        for(n=0,k = -dz->iparam[SPL_OCNT];n<eventscnt;n++,k++) {    //  For all timed events
            time = timeset[n];
            sampstep = (int)round(time * srate);
            thisobufpos = startobufpos + sampstep;                  //  Find time beyond starttime, in samples, at which to write next splinter
            advance = thisobufpos - lastobufpos;
            obufpos = thisobufpos;
            if(n > 0 && n < eventscnt - 1) {
                if(dz->brksize[SPL_RND]) {
                    thistime = (double)(dz->total_samps_written + obufpos)/srate;
                    if((exit_status = read_value_from_brktable(thistime,SPL_RND,dz))<0)
                        return exit_status;
                }
                if(dz->param[SPL_RND] > 0.0) {
                    rnd = ((drand48() * 2.0) - 1.0)/2.0;            //  range +1/2, -1/2
                    rnd *= dz->param[SPL_RND];
                    rndstep = (int)round(rnd * advance);
                    obufpos += rndstep;
                }
            }
            if(obufpos >= dz->buflen) {
                sprintf(errstr,"Buffer should be long enough to contain all time-sequence splinters. (1)\n");
                return PROGRAM_ERROR;
            }
            if(k > 0) {                                             //  If splinter-shrink starting to change       
                shrink = shrinkage[k];                              //  Get next shrink value
                if(n < eventscnt - 1) {                             //  If not at last of shrunk events
                    if(dz->brksize[SPL_SHRND]) {                    //  possibly randomise shrinking
                        thistime = (double)(dz->total_samps_written + obufpos)/srate;
                        if((exit_status = read_value_from_brktable(thistime,SPL_RND,dz))<0)
                            return exit_status;
                    }
                    if(dz->param[SPL_SHRND] > 0.0) {                //  If shrink to be randomised
                        shrinkstep = shrinkage[k+1] - shrink;       //  do it (shrinkkage[k+1] exists if n < eventscnt - 1)
                        rnd = drand48() * dz->param[SPL_SHRND];     //  range 0 to SPL_SHRND
                        shrink += (shrinkstep * rnd);               //  Possibly expand-or-contract shrinkage
                    }
                }
                if((exit_status = shrink_waveset(shrink,&splinterlen,dz))<0)
                    return exit_status;                             //  and re-shrink waveset
            }
            for(m=0;m<splinterlen;m++) {                            //  Add this splinter into the obuf
                obuf[obufpos] = (float)(obuf[obufpos] + shrbuf[m]);
                if(++obufpos >= dz->buflen) {
                    sprintf(errstr,"Buffer should be long enough to contain all time-sequence splinters. (2)\n");
                    return PROGRAM_ERROR;
                }
            }
            lastobufpos = thisobufpos;
        }                                                           //  Set obufpos at end of target wavesetgrp in obuf
        envend = max(dz->splintoffset,dz->target) + dz->wavesetgrplen;
        envend = max(envend,dz->buflen);
        if((exit_status = normalise_buffer(envend,dz))<0)
            return exit_status;

        //  WRITE REMAINDER OF INFILE

        if(!dz->vflag[1]) {
            while(dz->ssampsread > 0) {
                while(ibufpos < dz->ssampsread) {
                    obuf[obufpos++] = ibuf[ibufpos++];
                    if(obufpos >= dz->buflen) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        obufpos = 0;
                    }
                }
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                ibufpos = 0;
            }
        }

    } else {    //  MODE 1 or 3: post-event splinter
        
        //  IN ALL CASES we read to dz->target - dz->envwindowsize in infile and write all this to output

        subtarget = dz->target - dz->envwindowsize;             //  subtarget is 1 envelope-windowlen before target (which is start of target wavesetgroup)

        if(!dz->vflag[1]) {
            while(dz->total_samps_read < subtarget) {               //  Read all of src, up to the subtarget
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                if(dz->total_samps_read <= subtarget) {
                    if((exit_status = write_samps(ibuf,dz->buflen,dz))<0)
                        return(exit_status);
                } else {
                    samps_to_write = subtarget % dz->buflen;        //  and write it directly to the output
                    if((exit_status = write_samps(ibuf,samps_to_write,dz))<0)
                        return(exit_status);
                }       
            }
        }
        memset((char *)ibuf,0,dz->buflen * sizeof(float));
        sndseekEx(dz->ifd[0],subtarget,0);                      //  Seek to the subtarget
        dz->total_samps_read = subtarget;
        if((exit_status = read_samps(ibuf,dz))<0)               //  should read whole of remainder of sound, if buffers set up correctly!!!
            return(exit_status);
        if(dz->total_samps_read != dz->insams[0]) {
            sprintf(errstr,"Error in buffersize to accomodate all of end of source. total samps read = %d filesize = %d\n",dz->total_samps_read,dz->insams[0]);
            return PROGRAM_ERROR;
        }
        if(dz->vflag[0])                                        //  If source remnant to be mixed into output, read all of it   
            samps_to_read = dz->ssampsread;
        else                                                    //  If not, read up to the target waveset and to the end of it.
            samps_to_read = dz->envwindowsize + dz->wavesetgrplen;

        memcpy((char *)obuf,(char *)ibuf,samps_to_read * sizeof(float));
        maxwrite = samps_to_read;

        obufpos = dz->envwindowsize;                            //  Set obufpos to START OF WAVESET GROUP (splinters are timed from this point)

        eventscnt = dz->iparam[SPL_SHRCNT]+dz->iparam[SPL_OCNT];//  Number of events in the rit of pulses

        last_shrink = 0;
        initialobufpos = obufpos;
        lastobufpos = obufpos;
        for(n=0;n < eventscnt; n++) {                           //  Generate splinters and every time in the timing set: (wavesetgroup time assumed to be zero)
            k = min(n,dz->iparam[SPL_SHRCNT]-1);                //  Step through sequence of shrinks as far as dz->iparam[SPL_SHRCNT], where shrink is (and remains at) max. 
            shrink = shrinkage[k];                              //  Get shrink for this splinter.
            sampstep = (int)round(timeset[n] * srate);          //  Time step to next splinter, in samples
            thisobufpos = initialobufpos + sampstep;            //  Advance in outbuf to next splinter start position
            obufpos = thisobufpos;
            advance = thisobufpos - lastobufpos;
            if(n > 0 && n < eventscnt - 1) {
                if(dz->brksize[SPL_RND]) {
                    thistime = (double)(dz->total_samps_written + obufpos)/srate;
                    if((exit_status = read_value_from_brktable(thistime,SPL_RND,dz))<0)
                        return exit_status;
                }
                if(dz->param[SPL_RND] > 0.0) {
                    rnd = ((drand48() * 2.0) - 1.0)/2.0;        //  range +1/2, -1/2
                    rnd *= dz->param[SPL_RND];
                    rndstep = (int)round(rnd * advance);
                    obufpos += rndstep;
                }
            }
            if(obufpos >= dz->buflen) {
                sprintf(errstr,"ERROR: Buffer should be long enough to accomodate all of post-target output.(1)\n");
                return PROGRAM_ERROR;
            }
            thisshrink = shrink;
            if(shrink != last_shrink) {                         //  If shrink has changed, shrink the waveset
                if(k < dz->iparam[SPL_SHRCNT]-1)                //  Before last shrunk-waveset                          
                    shrinkstep = shrinkage[k+1] - shrink;       //  Get change in shrink to next waveset
                                                                //  (last step is retained)
                if(dz->brksize[SPL_SHRND]) {                    //  Possibly randomise the shrink value
                    thistime = (double)(dz->total_samps_written + obufpos)/srate;
                    if((exit_status = read_value_from_brktable(thistime,SPL_SHRND,dz))<0)
                        return exit_status;
                }
                if(dz->param[SPL_SHRND] > 0.0) {
                    rnd = drand48() * dz->param[SPL_SHRND];     //  range 0 to SPL_SHRND
                    thisshrink += shrinkstep * rnd;             //  Possibly expand-or-contract shrinkage
                }
                if((exit_status = shrink_waveset(thisshrink,&splinterlen,dz))<0)
                    return exit_status;
                                                                //  If the  (unrandomised) shrnik is not changing
            } else {                                            //  use the shrunk splinter generated at last pass
                                                                //  UNLESS it's randomised
                if(n > 0 && n < eventscnt - 1) {
                    if(dz->brksize[SPL_SHRND]) {
                        thistime = (double)(dz->total_samps_written + obufpos)/srate;
                        if((exit_status = read_value_from_brktable(thistime,SPL_SHRND,dz))<0)
                            return exit_status;
                    }
                    if(dz->param[SPL_SHRND] > 0.0) {
                        rnd = drand48() * dz->param[SPL_SHRND];     //  range 0 to SPL_SHRND
                        thisshrink += shrinkstep * rnd;             //  Possibly expand-or-contract shrinkage
                        if((exit_status = shrink_waveset(thisshrink,&splinterlen,dz))<0)
                            return exit_status;
                    } else if(dz->brksize[SPL_SHRND]) {             //  IF SHRND varies, but currently 0, still remake splinter, at unrandomised len
                        if((exit_status = shrink_waveset(shrink,&splinterlen,dz))<0)
                            return exit_status;
                    }
                }
            }
            last_shrink = shrink;                               //  Remember the (unrandomised) shrink value just used.
            if(obufpos + splinterlen >= dz->buflen) {
                sprintf(errstr,"ERROR: Buffer should be long enough to accomodate all of post-target output. (2)\n");
                return PROGRAM_ERROR;
            }
            for(m = 0; m < splinterlen; m++) {                  //  Add the shrunk splinter to the output   
                obuf[obufpos] = (float)(obuf[obufpos] + shrbuf[m]);
                obufpos++;
            }
            lastobufpos = thisobufpos;
        }
        maxwrite = max(maxwrite,obufpos);                       //  maxwrite = end of insams, obufpos = end of shrink-sequence
        if((exit_status = normalise_buffer(maxwrite,dz))<0)
            return exit_status;

        sampstep = dz->enddur;
        finalshrink = shrink;
                                                                //  Once all timed set of splinters have been written...
        for(n = 0; n < dz->iparam[SPL_ECNT]; n++) {             //  If there's any extension (splinters repeating at regular pulse)
            thisobufpos = lastobufpos + sampstep;               //  Advance (regular sampstep set previously), and check for overflow       
            obufpos = thisobufpos;
            if(dz->brksize[SPL_RND]) {                          //  Possibly randomise timing
                thistime = (double)(dz->total_samps_written + obufpos)/srate;
                if((exit_status = read_value_from_brktable(thistime,SPL_RND,dz))<0)
                    return exit_status;
            }
            if(dz->param[SPL_RND] > 0.0) {
                rnd = ((drand48() * 2.0) - 1.0)/2.0;        //  range +1/2, -1/2
                rnd *= dz->param[SPL_RND];
                rndstep = (int)round(rnd * sampstep);
                obufpos += rndstep;
            }
            if((ovflw = obufpos - dz->buflen) > 0) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                memset((char *)obuf,0,dz->buflen * sizeof(float));
                memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen2 * sizeof(float));
                memset((char *)ovflwbuf,0,dz->buflen2 * sizeof(float));
                obufpos -= dz->buflen;
                thisobufpos -= dz->buflen;
            }
            shrink = finalshrink;
            if(dz->brksize[SPL_SHRND]) {                    //  Possibly randomise shrinkage        
                thistime = (double)(dz->total_samps_written + obufpos)/srate;
                if((exit_status = read_value_from_brktable(thistime,SPL_SHRND,dz))<0)
                    return exit_status;
            }
            if(dz->param[SPL_SHRND] > 0.0) {
                rnd = drand48() * dz->param[SPL_SHRND];     //  range 0 to SPL_SHRND
                shrink = finalshrink + (shrinkstep * rnd);  //  Possibly expand-or-contract shrinkage
                if((exit_status = shrink_waveset(shrink,&splinterlen,dz))<0)//and shrink waveset
                    return exit_status;
            } else if(dz->brksize[SPL_SHRND]) {             //  Varying randomisation of shrinkage, but currently 0
                if((exit_status = shrink_waveset(finalshrink,&splinterlen,dz))<0)//and shrink waveset
                    return exit_status;
            }
            for(m = 0; m < splinterlen; m++) {                  //  Add copy of final splinter to output
                obuf[obufpos] = (float)(obuf[obufpos] + shrbuf[m]);
                obufpos++;
            }
            lastobufpos = thisobufpos;
        }
    }
    if(obufpos > 0) {                                           //  Write any samples remaining in obuf.
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

/******************************** CREATE_TEMP_SNDBUF ********************************/

int create_temp_sndbuf(dataptr dz)
{
    int bigbufsize, secsize;
    int framesize = F_SECSIZE;

    if(dz->sbufptr == 0 || dz->sampbuf == 0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = (int)(size_t)Malloc(-1);                                   //  Ensure no very tiny buffs (e.g. if attack very near start of sound)
    dz->buflen = bigbufsize/sizeof(float);
    secsize = dz->buflen/framesize;
    if(secsize * framesize != dz->buflen)
        secsize++;
    dz->buflen = secsize * framesize;                               //  Ensure bufsize is multiple of F_SECSIZE (256) .. ness for dz->envwindowsize calcs
    if(dz->buflen < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create temporary sound buffer (1).\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = dz->buflen * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create temporary sound buffer (2).\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;                   //  Inbuf
    return(FINISHED);
}

/******************************** CREATE_SPLINTER_SNDBUFS1 ********************************/

int create_splinter_sndbufs1(dataptr dz)
{
    int bigbufsize, secsize;
    int framesize = F_SECSIZE;

    if(dz->sbufptr == 0 || dz->sampbuf == 0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    if(dz->mode == 0 || dz->mode == 2) {    //  Pre-splinter    Allow for src pre-splintering to be mixed with splintered material inside a single buffer

        dz->buflen = dz->target;                                    //  Input buffer must contain infile up to wavesetgroup
        dz->buflen = max(dz->buflen,dz->splintoffset);              //  But also, all the pre-splinters
        dz->buflen += dz->wavesetgrplen;                            //  And the target wavesetgroup

    } else {            //  Post-splinter   Allow for src post-splintering to be mixed with splintered material inside a single buffer

        dz->buflen = dz->insams[0] - dz->target;                    //  Input buffer large enough to accomodate all of post-wavesetgroup infile
        dz->buflen = max(dz->buflen,dz->splintoffset);              //  And also all post wavesetgroup splinters
    }
    bigbufsize = (int)(size_t)Malloc(-1);                                   //  Ensure no very tiny buffs (e.g. if attack very near start of sound)
    dz->buflen = max((unsigned int)dz->buflen,bigbufsize/sizeof(float));
    secsize = dz->buflen/framesize;
    if(secsize * framesize != dz->buflen)
        secsize++;
    dz->buflen = secsize * framesize;                               //  Ensure bufsize is multiple of F_SECSIZE (256) .. ness for dz->envwindowsize calcs
    if(dz->buflen < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create output sound buffer.\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/******************************** CREATE_SPLINTER_SNDBUFS2 ********************************/

int create_splinter_sndbufs2(dataptr dz)
{
    int bigbufsize, secsize;
    int framesize = F_SECSIZE;
    dz->buflen += dz->envwindowsize * 2;                            //  Allow for 2 bracketing envelope windows
    secsize = dz->buflen/framesize;
    if(secsize * framesize != dz->buflen)
        secsize++;
    dz->buflen = secsize * framesize;                               //  Ensure bufsize is multiple of F_SECSIZE (256)
    if(dz->buflen < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create output sound buffer with eveloping brackets.\n");
        return(PROGRAM_ERROR);
    }
    dz->buflen2 = dz->wavesetgrplen + 64;                           //  Size of pre-shrunk wavesetgroup + SAFETY
    secsize = dz->buflen2/framesize;
    if(secsize * framesize != dz->buflen2)
        secsize++;
    dz->buflen2 = secsize * framesize;
    if(dz->buflen2 < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create wavesetgroup-storage buffer.\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = ((dz->buflen * 2) + (dz->buflen2 * 3)) * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create total sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;                   //  Inbuf
    dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;  //  Outbuf
    dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;  //  Ovflwbuf
    dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + dz->buflen2; //  Waveset store
    dz->sbufptr[4] = dz->sampbuf[4] = dz->sampbuf[3] + dz->buflen2; //  Shrunk-waveset store
    dz->sampbuf[5] = dz->sampbuf[4] + dz->buflen2;
    return(FINISHED);
}

/******************************** FIND_AND_STORE_WAVESETS ********************************/

int find_wavesets(int *initial_phase,dataptr dz)
{
    int exit_status, changephase = 0, halfcyclecnt = 0, phase;
    int sampskip, secsize, ibufpos = 0, splsamps, endsample;
    double spltime = dz->param[SPL_TIME], srate = (double)dz->infile->srate;
    float *ibuf = dz->sampbuf[0];

    splsamps = (int)floor(spltime * srate);     //  Find how far to skip into infile to find required waveset(s)
    secsize = dz->target/F_SECSIZE;
    sampskip = secsize * F_SECSIZE;                 //  curtail to previous secsize boundary
    sndseekEx(dz->ifd[0],sampskip,0);               //  Skip into infile to (just before) time of wavesetgrp given by user
    dz->total_samps_read = sampskip;
    if((exit_status = read_samps(ibuf,dz))<0)       //  Read infile
        return(exit_status);
    ibufpos = splsamps - sampskip;                  //  Position ibuf pointer to samptime given by user, then search forward for wavesetgrp start
    if(ibuf[ibufpos] == 0.0) {                      //  Advance to first non-zero sample
        while(ibuf[ibufpos] == 0.0) {
            ibufpos++;
            if(ibufpos >= dz->ssampsread) {
                sampskip += dz->ssampsread;
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                if(dz->ssampsread == 0) {
                    sprintf(errstr,"No wavesets found after time %lf\n",dz->param[SPL_TIME]);
                    return DATA_ERROR;
                } else
                    ibufpos = 0;
            }
        }
        if(ibuf[ibufpos] > 0.0)                     //  then note the phase (+ve going or -ve going)
            phase = 1;
        else
            phase = -1;
    } else if(ibuf[0] > 0.0) {                      //  Or if the signal already +ve
        while(ibuf[ibufpos] >= 0.0) {               //  advance to where signal changes phase (to -ve)
            ibufpos++;                              //  and set the phase value.
            if(ibufpos >= dz->ssampsread) {
                sampskip += dz->ssampsread;
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                if(dz->ssampsread == 0) {
                    sprintf(errstr,"No wavesets found after time %lf\n",dz->param[SPL_TIME]);
                    return DATA_ERROR;
                } else
                    ibufpos = 0;
            }
        }
        phase = -1;
    } else {                                        //  Or if the signal already -ve
        while(ibuf[ibufpos] <= 0.0) {               //  advance to where signal changes phase (to +ve)
            ibufpos++;                              //  and set the phase value.
            if(ibufpos >= dz->ssampsread) {
                sampskip += dz->ssampsread;
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                if(dz->ssampsread == 0) {
                    sprintf(errstr,"No wavesets found after time %lf\n",dz->param[SPL_TIME]);
                    return DATA_ERROR;
                } else
                    ibufpos = 0;
            }
        }
        phase = 1;
    }
    dz->target = sampskip + dz->total_samps_read - dz->ssampsread + ibufpos;
//  dz->target = sampskip + ibufpos;                    //  Position of target wavesetgrp in infile
    *initial_phase = phase;
    while(ibufpos < dz->ssampsread) {
        if(++ibufpos >= dz->ssampsread) {               //  Advance in inbuf    
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            if(dz->ssampsread == 0) {
                sprintf(errstr,"Insufficient wavesets found after time %lf\n",dz->param[SPL_TIME]);
                return DATA_ERROR;
            } else
                ibufpos = 0;
        }
        switch(phase) {
        case(1):
            if(ibuf[ibufpos] < 0.0)                     //  If phase changes (signal crosses zero) flag a phase-change
                changephase = 1;
            break;
        case(-1):
            if(ibuf[ibufpos] > 0.0)
                changephase = 1;
            break;
        }
        if(changephase) {                               //  If phasechange is flagged
            halfcyclecnt++;                             //  Count the completed half-cycle  
            phase = -phase;                             //  Change the phase
            changephase = 0;
        }
        if(halfcyclecnt >= 2 * dz->iparam[SPL_WCNT])    //  When enough half-cycles are counted, break
            break;
    }
    dz->quartercyclecnt = halfcyclecnt * 2;             //  No of quaretercycles in wavesetgroup
    endsample = dz->total_samps_read - dz->ssampsread + ibufpos;
    dz->wavesetgrplen = endsample - dz->target;
    return  FINISHED;
}

/******************************** STORE_WAVESETS ********************************/

int store_wavesets(int initial_phase,dataptr dz)
{
    int exit_status, changephase = 0, halfcyclecnt = 0, phase;
    int ibufpos = 0, wbufpos = 0, maxminstorecnt = 0, maxpos, maxmaxpos = 0;
    double maxmin, maxmaxmin;
    float *ibuf = dz->sampbuf[0], *wbuf = dz->sampbuf[3];
    int *maxminstore = dz->lparray[0];
    maxmin = 0.0;                                       //  Initialise the max(min) value and position
    maxpos = 0;
    sndseekEx(dz->ifd[0],dz->target,0);
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);
    ibufpos = 0;
    wbufpos = 0;
    wbuf[wbufpos++] = 0.0;
    phase = initial_phase;
    maxmaxmin = 0.0;
    maxmaxpos = 0;
    while(ibufpos < dz->ssampsread) {
        wbuf[wbufpos] = ibuf[ibufpos];                  //  Store wavesets in waveset-store
        if(fabs(wbuf[wbufpos]) > maxmin) {              //  Check for max(min) sample in the cycle
            maxpos = wbufpos;                           //  noting its new position and value, if it changes
            maxmin = fabs(wbuf[maxpos]);
        }
        if(++ibufpos >= dz->ssampsread) {               //  Advance in inbuf    
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            ibufpos = 0;
        }
        if(++wbufpos >= dz->buflen2) {                  //  Advance in waveset-store buffer, checking for overflow  
            sprintf(errstr,"Waveset group exceeds %d seconds in length.\n",SPLMAXSPL);
            return DATA_ERROR;
        }
        switch(phase) {
        case(1):
            if(ibuf[ibufpos] < 0.0)                     //  If phase changes (signal crosses zero) flag a phase-change
                changephase = 1;
            break;
        case(-1):
            if(ibuf[ibufpos] > 0.0)
                changephase = 1;
            break;
        }
        if(changephase) {                               //  If phasechange is flagged
            if(maxmin > maxmaxmin) {
                maxmaxmin = maxmin;
                maxmaxpos = maxpos;
            }
            maxminstore[maxminstorecnt++] = maxpos;     //  Store position of min(max) in completed half-cycle
            maxmin = 0.0;                               //  and re-initialise maxmin for next cycle
            maxminstore[maxminstorecnt++] = wbufpos;    //  Store end of completed half-cycle
            halfcyclecnt++;                             //  Count the completed half-cycle  
            phase = -phase;                             //  Change the phase
            changephase = 0;
        }
        if(halfcyclecnt >= 2 * dz->iparam[SPL_WCNT])    //  When enough half-cycles are stored, break
            break;
    }
    dz->wmaxmpos = maxmaxpos;
    return  FINISHED;
}

/******************************** SHRINK_WAVESET ********************************/

int shrink_waveset(double shrink,int *splinterlen,dataptr dz)
{
//HEREH Shrinkage not working correctly ... output too short
    float *wbuf = dz->sampbuf[3], *shrbuf = dz->sampbuf[4];
    int *maxminstore = dz->lparray[0];
    int thispos, nextpos, samplen, n, m, thissamp, nextsamp;
    double dpos, incr, thisval, valdiff, frac, val, splval; 
    int maxmincnt = 0, shrbufpos = 0, incrsteps;
    int prelen, postlen, preshrunk, startsamp, postshrunk, endsamp;

    memset((char *)shrbuf,0,dz->buflen2 * sizeof(float));
    thispos = 0;
    switch(dz->mode) {
    case(0):
    case(1):
        while(maxmincnt < dz->quartercyclecnt) {
            nextpos = maxminstore[maxmincnt];                       //  Find end of next quartercycle
            samplen = nextpos - thispos;                            //  Find sample-length of quartercycled
            if(samplen <= QCYCLEMIN) {                              //  Don't try to shrink any quarter-cycle that is only 1 or 2 samples int
                incr = 1.0;
                incrsteps = samplen;
            } else {
                incrsteps = (int)round((double)samplen * shrink);   //  Approx how many sampling steps to cover this quarter-cycle
                incrsteps = max(QCYCLEMIN,incrsteps);               //  Don't shrink any quartercycle to less than QCYCLEMIN samples    
                incr = (double)samplen/(double)incrsteps;           //  Readjust sampling step so its equally spaced over the quarter-cycle
            }
            dpos = (double)thispos;
            shrbuf[shrbufpos++] = wbuf[thispos];                    //  Retain the start of the quarter-cycle, and the min(max)                     
            for(n = 1;n < incrsteps;n++) {
                dpos += incr;                                       //  Shrink quartercycle by interpolation
                thissamp = (int)floor(dpos);
                nextsamp = thissamp + 1;
                thisval = wbuf[thissamp];
                valdiff = wbuf[nextsamp] - thisval;
                frac = dpos - (double)thissamp;
                val = thisval + (valdiff * frac);
                shrbuf[shrbufpos] = (float)val;
                shrbufpos++;
            }
            thispos = nextpos;
            maxmincnt++;
        }
        break;
    case(2):
    case(3):
        prelen     = dz->wmaxmpos;
        if(prelen > dz->splicelen)
            preshrunk  = (int)round(prelen * shrink);
        else
            preshrunk  = prelen;
        startsamp  = prelen - preshrunk;
        postlen    = dz->wavesetgrplen - dz->wmaxmpos;
        if(postlen > dz->splicelen)
            postshrunk  = (int)round(postlen * shrink);
        else
            postshrunk = postlen;
        endsamp    = dz->wmaxmpos + postshrunk;
        for(n = startsamp; n < endsamp;n++)
            shrbuf[shrbufpos++] = wbuf[n];
        if(prelen > dz->splicelen) {
            for(n = 0; n < dz->splicelen;n++) {
                splval = ((double)n/(double)dz->splicelen);
                shrbuf[n] = (float)(shrbuf[n] * splval);
            }
        }
        if(postlen > dz->splicelen) {
            for(n = 0,m = shrbufpos-1; n < dz->splicelen;n++,m--) {
                splval = ((double)n/(double)dz->splicelen);
                shrbuf[m] = (float)(shrbuf[m] * splval);
            }
        }
        break;
    }
    *splinterlen = shrbufpos;   //  Store size of shrunk waveset
    return FINISHED;
}

/******************************** GENERATE_SHRINKAGE_SET ********************************/

int generate_shrinkage_set(dataptr dz)
{
    int n, m, kk;
    double *shrinkage = dz->parray[0], shrinkcnt = dz->iparam[SPL_SHRCNT];
    double temp, srate = (double)dz->infile->srate;
    int shrinkdiff;

    if(dz->mode <= 1) {
        dz->minlen = (int)ceil(srate/dz->param[SPL_FRQ]);           //  maximally-shrunk-waveset wavelen, in samples
        dz->minlen *= dz->iparam[SPL_WCNT];                         //  multiplied by number of wavesets in group
        if(dz->minlen >= dz->wavesetgrplen) {
            temp = ((double)dz->minlen/(double)dz->wavesetgrplen) * dz->param[SPL_FRQ];
            sprintf(errstr,"Targeted-wavesets frq (c. %.2lf) >= goal frq (%.2lf): Cannot shrink.\n",temp,dz->param[SPL_FRQ]);

            return DATA_ERROR;
        }
    } else
        dz->minlen = (int)round(dz->param[SPL_DUR]*MS_TO_SECS*srate);//maximally-shrunk-envent length, in samples
    shrinkdiff = dz->wavesetgrplen - dz->minlen;                    //  Total amount of samples lost in max shortening

    for(n=0,m=1; m < dz->iparam[SPL_SHRCNT];n++,m++) {
        shrinkage[n] = (double)m/(double)shrinkcnt;                 //  Values >0 to <1  e.g. for 4 shrinks we get  [1/4  1/2  3/4]
        if(!flteq(dz->param[SPL_SCURVE],1.0))                       //  Ditto but warped, e.g. with warp 2          [1/16 1/4  9/16]
            shrinkage[n] = pow(shrinkage[n],dz->param[SPL_SCURVE]);
    }
    shrinkage[n] = 1;

    for(n=0; n < dz->iparam[SPL_SHRCNT];n++) {
        shrinkage[n] *= (double)shrinkdiff;                         //  Amount of samples lost at each step     (small to large vals)
        shrinkage[n] = (double)dz->wavesetgrplen - shrinkage[n];    //  Number of samples remaining at each step(large to small vals)
        shrinkage[n] = shrinkage[n]/(double)dz->wavesetgrplen;      //  Shrinkage at each step                  (large to small vals = less shrinking to more shrinking)
    }
    if(dz->mode == 0 || dz->mode == 2) {
        kk = dz->iparam[SPL_SHRCNT]/2;
        for(n = 0, m = dz->iparam[SPL_SHRCNT]-1; n < kk; n++, m--) { // Reverse shrinkage sequence for mode 0
            temp = shrinkage[n];
            shrinkage[n] = shrinkage[m];
            shrinkage[m] = temp;
        }
    }
    return FINISHED;
}

/******************************** GENERATE_TIMING_SET ********************************/

int generate_timing_set(dataptr dz)
{
    double *timeset = dz->parray[1];
    double dur1, dur2, durdiff, sum, temp, startdur, maxstep, srate = (double)dz->infile->srate;
    int eventscnt, n, m, kk, isshrand = 0;
    int extrasamps = 0, lastdur, minlength;
    eventscnt = dz->iparam[SPL_SHRCNT] + dz->iparam[SPL_OCNT];  //  Number of events in the rit/accel of pulses
    if(dz->param[SPL_PULS1] == 0)
        dur1 = (double)dz->wavesetgrplen/srate;
    else
        dur1 = 1.0/dz->param[SPL_PULS1];                        //  Pulse-rate of wavesets where they join original source
    if(dz->param[SPL_PULS2] == 0)
        dur2 = dur1;                                            //  Goal duration       
    else
        dur2 = 1.0/dz->param[SPL_PULS2];                        //  Pulse-rate of wavesets where they start/end
    durdiff = dur2 - dur1;                                      //  Difference in duration      

    for(n=0,m=1; m < eventscnt;n++,m++) {
        timeset[n] = (double)m/(double)eventscnt;               //  Values >0 to 1 e.g. for 4 steps     [1/4  1/2  3/4 ]
        if(!flteq(dz->param[SPL_PCURVE],1.0))                   //  Ditto but warped e.g. by factor 2   [1/16 1/4  9/16]
            timeset[n] = pow(timeset[n],dz->param[SPL_PCURVE]);
    }
    timeset[n] = 1;

    for(n=0; n < eventscnt;n++) {
        timeset[n] *= (double)durdiff;                          //  Succesive increments in event separation
        timeset[n] += dur1;                                     //  Succesive event separations
    }
    if(dz->mode == 0 || dz->mode == 2) {                        //  If this is an attack (pulses before event)          
        kk = eventscnt/2;                                       //  reverse sequence of durations. NB for kk 6/2 = 3 but 5/2 = 2 so ... 
        for(n = 0, m = eventscnt-1; n < kk; n++, m--) {         //  If eventscnt even, all end events swapped with all start events
            temp = timeset[n];                                  //  e.g. 6 events a:b:c:d:e:f --- swap 6/2 = 3 pairs a/f b/e c/d --> f:e:d:c:b:a
            timeset[n] = timeset[m];                            //  If eventscnt odd, middle event not swapped, remains where it is
            timeset[m] = temp;                                  //  e.g. 5 events a:b:c:d:e ----- swap 5/2 = 2 pairs a/e b/d --> e:d:c:b:a
        }
    }
    if(dz->mode == 0 || dz->mode == 2) {        //Splinters BEFORE wavesetgroup
        startdur = timeset[0];                                  //  Remember duration of first step between splinters
        sum = 0.0;
        for(n=0; n < eventscnt;n++) {                           //  Progressively sum event-separations to get relative times
            temp = timeset[n];                                  //  time[0] gets zero
            timeset[n] = sum;                                   //  time[1] gets separation[0]
            sum += temp;                                        //  time[2] gets separation[0] + separation[1]
        }                                                       //  time[3] gets separation[0] + separation[1] + separation[2] etc.
                                                                //  sum ends up as sum of all event-separations, so
        dz->splintoffset = (int)round(sum * srate);         //  "splintoffset" gets time where wavesetgroup starts

        if(dz->iparam[SPL_ECNT] > 0) {                          //  If there's an extension (extra regular pulsed splinters before timeset begins)
            extrasamps = (int)round(startdur * srate);          //  Add their duration to "splintoffset"
            extrasamps *= dz->iparam[SPL_ECNT];
            dz->splintoffset += extrasamps;                     //  This is distance from splinter start to wavesetgrp, in output
        }
    } else {
        dz->enddur = (int)round(dur2 * srate);                  //  Find duration of last step between splinters
        sum = 0.0;
        maxstep = 0.0;
        for(n=0; n < eventscnt;n++) {                           //  Progressively sum event-separations to get relative times
            maxstep = max(maxstep,timeset[n]);
            sum += timeset[n];                                  //  so time[0] gets separation[0] ... i.e. it is separation[0] AFTER the src-wavesetgroup
            timeset[n] = sum;                                   //  time[1] gets separation[0] + separation[1]
        }                                                       //  time[2] gets separation[0] + separation[1] + separation[2] etc.

        dz->splintoffset = (int)ceil(sum * srate);              //  "splintoffset" gets time where last splinter of sequence starts
        dz->splintoffset += (int)ceil(maxstep * srate);     //  This is AT LEAST length of last splinter in sequence
        dz->splintoffset += (int)ceil(maxstep * srate);     //  This allows for posssible randomisation

        lastdur = dz->enddur;

        if(dz->brksize[SPL_RND] || dz->param[SPL_RND] > 0.0)    //  Allow for maximal (random) contraction of step between events
            lastdur = (int)floor(dz->enddur * 0.5);

        if(dz->brksize[SPL_SHRND] || dz->param[SPL_SHRND] > 0.0) {  //  Allow for maximal (random) expansion of pulse-size
            minlength = (int)ceil(dz->minlen * 1.5);
            isshrand = 1;
        }
        else
            minlength = dz->minlen;
        if(lastdur <= minlength) {
            if(dz->mode == 1) {
                if(isshrand)
                    sprintf(errstr,"Final pulse-rate may cause randomised-length of max-shrunk pulses to overlap. Reduce rate or increase pulse frq.\n");
                else
                    sprintf(errstr,"Final pulse-rate causes max-shrunk pulses to overlap. Reduce rate or increase pulse frq.\n");
            } else {
                if(isshrand)
                    sprintf(errstr,"Final pulse-rate may cause randomised-length of max-shrunk pulses to overlap. Reduce rate or reduce pulse duration.\n");
                else
                    sprintf(errstr,"Final pulse-rate causes max-shrunk pulses to overlap. Reduce rate or reduce pulse duration.\n");
            }
            return DATA_ERROR;
        }
    }
    return FINISHED;
}

/******************************** NORMALISE_BUFFER ********************************/

int normalise_buffer(int windowing_end,dataptr dz)
{
    double *env = dz->parray[2], maxsamp, thiseval, nexteval, diff, eval;
    float *obuf = dz->sampbuf[1];
    int *loc = dz->iparray[0];
    int e, n, m, k, envsize, maxloc, windowstart, thispos, goalpos, samppos, gap;
    int needs_enveloping = 0, ethis, enext, done;
    int halfwindow = dz->envwindowsize/2;
    do {                                                        //  For all the normalisable samples, advance by windowlen blocks           
        for(n = 0,e = 0; n < windowing_end; n+=dz->envwindowsize,e++) {
            maxsamp = 0.0;
            maxloc = 0;
            for(m=0,k=n;m < dz->envwindowsize;m++,k++) {        //  In each window, find the maxsamp
                if(fabs(obuf[k]) > maxsamp) { 
                    maxsamp = fabs(obuf[k]);
                    maxloc = m;
                }
            }
            if(e >= dz->arraysize) {
                sprintf(errstr,"envelope arraysize exceeded.\n");
                return PROGRAM_ERROR;
            }
            env[e] = maxsamp;                                   //  And store the envelope val
            loc[e] = maxloc;                                    //  And position of maximum
        }
        envsize = e;
        env[e] = 0.0;                                           //  wrap-around point at end
        loc[e] = halfwindow;
        needs_enveloping = 0;
        for(e = 0;e <= envsize;e++) {                           //  Check where signal exceeds max (0.95)
            if(env[e] > 0.95) {                                 //  and force (re-)envelope to reduce level here
                env[e] = 0.95/env[e];
                needs_enveloping = 1;                           //  AND note the re-envelopeing is necessary
            } else                                              //  otherwise leave envelope level at 1.0 (no change)   
                env[e] = 1.0;
        }
        if(needs_enveloping) {                                  //  If enveloping required
            ethis = -1;                                         //  Interpolate the re-envelope vals, in order to envelope the src, in situ
            enext = 0;
            done = 0;
            for(windowstart = 0; windowstart < windowing_end; windowstart+=dz->envwindowsize) {
                ethis++;
                enext++;
                thiseval = env[ethis];
                nexteval = env[enext];
                if(thiseval < 1.0 && nexteval == 1.0) {
                    thispos = windowstart + loc[ethis];         //  Interp from maximum in this-window to middle of non-normalised next-window
                    goalpos = windowstart + dz->envwindowsize + halfwindow;
                } else if(thiseval == 1.0 && nexteval < 1.0) {
                    thispos = windowstart + halfwindow;         //  Interp from middle of non-normalised this-window to maximum in next
                    goalpos = windowstart + dz->envwindowsize + loc[enext];
                } else if(thiseval < 1.0 && nexteval < 1.0) {
                    thispos = windowstart + loc[ethis];         //  Interp from max in this window to max in next
                    goalpos = windowstart + dz->envwindowsize + loc[enext];
                } else { // (thiseval == 1.0 && nexteval == 1.0) do nothing
                    continue;
                }
                samppos = thispos;
                gap = goalpos - thispos;
                diff = nexteval - thiseval;
                for(m=0;m < gap;m++,samppos++) {
                    if(samppos >= dz->buflen) {
                        done = 1;
                        break;
                    }
                    eval = (double)m/(double)gap;
                    eval *= diff;
                    eval += thiseval;
                    obuf[samppos] = (float)(obuf[samppos] * eval);
                }
                if(done)
                    break;
            }
        }
    } while(needs_enveloping);                                  //  Do this recursively until nothing is too loud   
    return FINISHED;
}
