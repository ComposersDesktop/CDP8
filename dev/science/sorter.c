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

//  _cdprogs\sorter sorter 1 alan_bellydancefbn.wav test.wav alan_bellydancefbn_pich.brk -p128 -o128 -f
//  WRITE BUFFER OVER AND OVER, WHY???
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

#define CRESC   0
#define DECRESC 1
#define ACCEL   2
#define RIT     3
#define RAND    4

#define MAXLEV  0.95

// parrays
#define PEAKS       0
#define MINIMA      0
#define LEVELS      1
#define INCRS       2
#define ORIGINCRS   3
// lparrays
#define POS     0
#define LEN     1
#define ORIGPOS 2

#define SORTER_MIN_DUR (0.001)
#define SORTER_MAX_DUR (10.0)
#define TWO_THIRDS 0.6666

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int check_sorter_param_validity_and_consistency(dataptr dz);
static int setup_sorter_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_sorter_param_ranges_and_defaults(dataptr dz);
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
//static double dbtolevel(double val);
static int do_sorter(dataptr dz);
static int find_all_positive_peaks(int *peakcnt,dataptr dz);
static int find_all_local_minima(int peakcnt,int *local_minima_cnt,dataptr dz);
static int group_local_minima(int local_minima_cnt,int *group_cnt,int *ostep,dataptr dz);
static int locate_zero_crossings(int group_cnt,dataptr dz);
static int find_levels(int group_cnt,dataptr dz);
static int do_sorted_output(int group_cnt,int ostep,dataptr dz);
static void rndpermm(int permlen,int *permm);
static void insert(int m,int t,int permlen,int *permm);
static void prefix(int m,int permlen,int *permm);
static void shuflup(int k,int permlen, int *permm);
static int get_median_pitch(double midi,double *medianfrq,dataptr dz);
static int larger_grouping(int *group_cnt,dataptr dz);

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
        dz->maxmode = 5;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_sorter_application(dz))<0) {
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
    if((exit_status = setup_sorter_param_ranges_and_defaults(dz))<0) {
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
    if((exit_status = check_sorter_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
//  dz->bufcnt = 3;
    dz->bufcnt = 4;
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

    dz->bufcnt = 2; // Initially, just make the input buffers
    if((exit_status = create_sndbufs(dz))<0) {                          // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    dz->bufcnt = 4;
    //param_preprocess()                        redundant
    //spec_process_file =
    if((exit_status = do_sorter(dz))<0) {
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

/************************* SETUP_SORTER_APPLICATION *******************/

int setup_sorter_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if(dz->mode == RAND)
        exit_status = set_param_data(ap,0   ,2,2,"Di");
    else
        exit_status = set_param_data(ap,0   ,2,1,"D0");
    if(exit_status < 0)
        return exit_status;
    if((exit_status = set_vflgs(ap,"sopm",4,"dDdd","f",1,0,"0")) < 0)
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

/************************* SETUP_SORTER_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_sorter_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[SORTER_SIZE] = 0.0;
    ap->hi[SORTER_SIZE] = 2000;
    ap->default_val[SORTER_SIZE] = 0;
    if(dz->mode == RAND) {
        ap->lo[SORTER_SEED] = 0;
        ap->hi[SORTER_SEED] = 256;
        ap->default_val[SORTER_SEED] = 1;
    }
    ap->lo[SORTER_SMOOTH]   = 0;
    ap->hi[SORTER_SMOOTH]   = 50;
    ap->default_val[SORTER_SMOOTH] = 5;
    ap->lo[SORTER_OMIDI]    = 0;
    ap->hi[SORTER_OMIDI]    = 128;
    ap->default_val[SORTER_OMIDI] = 0;
    ap->lo[SORTER_IMIDI]    = 0;
    ap->hi[SORTER_IMIDI]    = 128;
    ap->default_val[SORTER_IMIDI] = 0;
    ap->lo[SORTER_META] = 0.0;
    ap->hi[SORTER_META] = dz->duration/3.0;
    ap->default_val[SORTER_META] = 0.0;
    dz->maxmode = 5;
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
            if((exit_status = setup_sorter_application(dz))<0)
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
    usage2("sorter");
    return(USAGE_ONLY);
}

/**************************** CHECK_SORTER_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_sorter_param_validity_and_consistency(dataptr dz)
{
    int exit_status;
    int n;
    double maxval, minval;
    double val, srate = (double)dz->infile->srate;

    if(dz->param[SORTER_IMIDI] > 0) {
        if(!(dz->vflag[0] && dz->brksize[SORTER_SIZE])) {
            sprintf(errstr,"Cannot transpose elements to a given pitch if sizedata is not time-varying frequency.\n");
            return DATA_ERROR;
        }
    }
    if(dz->param[SORTER_OMIDI] == 128) {
        if(!(dz->vflag[0] && dz->brksize[SORTER_SIZE])) {
            sprintf(errstr,"Cannot set output event-rate TO median pitch if sizedata is not time-varying frequency.\n");
            return DATA_ERROR;
        }
    }
    if(dz->brksize[SORTER_OMIDI]) {
        if((exit_status = get_maxvalue_in_brktable(&maxval,SORTER_OMIDI,dz))<0)
            return exit_status;
        if(maxval > 127) {
            sprintf(errstr,"MIDI values in a brkpnt file for the OUTPUT frequency cannot exceed 127\n");
            return DATA_ERROR;
        }
        for(n=1; n<dz->brksize[SORTER_OMIDI] * 2;n+=2) {//  Convert, output MIDI values, to frq, to wavelen in samples 
            val = miditohz(dz->brk[SORTER_OMIDI][n]);   //  We do not need the original pitch vales
            dz->brk[SORTER_OMIDI][n] = srate/val;
        }
    } else {
        if(dz->param[SORTER_OMIDI] > 127 && dz->param[SORTER_OMIDI] < 128) {
            sprintf(errstr,"MIDI values above 127 cannot be used (except 128.0)\n");
            return DATA_ERROR;
        }                                               //  But in this case,  do NOT do the preconversion to wavelen
    }                                                   //  as we need midi val when precalculating "ostep" in group_local_minima 
    if(dz->brksize[SORTER_SIZE]) {
        if((exit_status = get_maxvalue_in_brktable(&maxval,SORTER_SIZE,dz))<0)
            return exit_status;
        if((exit_status = get_minvalue_in_brktable(&minval,SORTER_SIZE,dz))<0)
            return exit_status;
        if((dz->vflag[0] == 0 && maxval > dz->duration/2) || (dz->vflag[0] == 1 && minval < 2.0/dz->duration))  {
            sprintf(errstr,"Elementsize (%.1lf) too big for infile. (If data's frq, set flag).\n",maxval);
            return DATA_ERROR;
        }
        if(dz->vflag[0]) {
            if(minval < 2.0/dz->duration)  {
                sprintf(errstr,"Min frequency (%lf) too low, given the duration of input file.\n",minval);
                return DATA_ERROR;
            }
            for(n=1;n<dz->brksize[SORTER_SIZE] * 2;n+=2)
                dz->brk[SORTER_SIZE][n] = 1.0/dz->brk[SORTER_SIZE][n];
        } else {
            if(maxval > dz->duration/2)  {
                sprintf(errstr,"Max elementsize (%lf) too large, given the duration of input file.\n",maxval);
                return DATA_ERROR;
            } 
            if(minval < 0.001 && minval > 0.0) {
                sprintf(errstr,"Min permitted duration for time-varying elementsize = %lf\n",SORTER_MIN_DUR);
                return DATA_ERROR;
            }
        }
    } else {
        if(dz->vflag[0]) {
            if(dz->param[SORTER_SIZE] < 2.0/dz->duration) {
                sprintf(errstr,"Freq too low for infile. (If data's duration, unset frq flag).\n");
                return DATA_ERROR;
            }
            dz->param[SORTER_SIZE] = 1.0/dz->param[SORTER_SIZE];
        } else {
            if(dz->param[SORTER_SIZE] < 0.001 && dz->param[SORTER_SIZE] > 0.0) {
                sprintf(errstr,"Minimum (non-zero) duration for elementsize = %lf\n",SORTER_MIN_DUR);
                return DATA_ERROR;
            } else if(dz->param[SORTER_SIZE] > dz->duration/2) {
                sprintf(errstr,"Elementsize too big for infile. (If meant to be frq, set flag).\n");
                return DATA_ERROR;
            }
        }
    }
    if(dz->param[SORTER_META] > 0.0) {
        if(!dz->vflag[0]) {
            sprintf(errstr,"Cannot set meta-group size if Frequency Flag is not set.\n");
            return DATA_ERROR;
        }
        if(dz->param[SORTER_IMIDI] == 0) {
            sprintf(errstr,"No point in setting meta-group count if pitch is not to be altered.\n");
            return DATA_ERROR;
        }
        for(n=1;n<dz->brksize[SORTER_SIZE] * 2;n+=2) {
            if(dz->brk[SORTER_SIZE][n] >= dz->param[SORTER_META]) {
                sprintf(errstr,"Larger gp (%.3lf) too small to accomodate element (%.3lf) from frq %.1lf\n",
                    dz->param[SORTER_META],dz->brk[SORTER_SIZE][n],1.0/dz->brk[SORTER_SIZE][n]);
                return DATA_ERROR;
            }
        }
    }       
    return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"sorter"))              dz->process = SORTER;
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
    if(!strcmp(str,"sorter")) {
        fprintf(stderr,
        "USAGE:\n"
        "sorter sorter 1-4 inf outf esiz [-ssmooth] [-oopch] [-ppch] [-mmeta] [-f]\n"
        "sorter sorter 5   inf outf esiz seed [-ssmooth] [-oopch] [-ppch] [-mmeta] [-f]\n"
        "\n"
        "Chops mono source into elements then reorganises by loudness, or duration.\n"
        "\n"
        "ESIZ    Approximate size of elements to sort, in seconds.\n"
        "        If set to zero, individual wavesets are chosen as elements.\n"
        "SMOOTH  Fade in (and out) each segment, with a \"SMOOTH\" mS splice.\n"
        "        (If elementsize is zero, this is ignored).\n"
        "SEED    Mode 5 (\"Order at Random\"): Rerun with same non-zero seed value\n"
        "        outputs the SAME random ordering of elements.\n"
        "        (Zero seed-value gives different random ordering on each run).\n"
        "OPCH    Output elements with separation equivalent to MIDI pitch,\"OPCH\".\n"
        "        If \"-f\" flag used, value can be set to 128 to specify\n"
        "        the median pitch of the source.\n"
        "        If value is set to zero, parameter is ignored.\n"
        "\n"
        "-f      Elementsize read as frq value (= 1.0/duration)\n"
        "        and could be a frequency-trace of the pitch of the source.\n"
        "\n"
        "The following parameters can only be used if \"-f\" flag is set.\n"
        "\n"
        "PCH     Transpose input elements to MIDI pitch specified.\n"
        "        If value set to 128, the median pitch of the source is used.\n"
        "        If value is set to zero, parameter is ignored.\n"
        "\n"
        "The following parameter is only useful if \"PCH\" is set.\n"
        "\n"
        "METAGRP Size of meta-grouping, in seconds.\n"
        "        Allows larger units to be (approx) pitch-correlated.\n"
        "        Src 1st cut to pitch-wavelen-scale elements & transpositions calcd.\n"
        "        These elements are then further grouped to (approx) \"METAGRP\" size.\n"
        "        Must be larger than largest element (1/frq) from frq trace.\n"
        "        If \"METAGRP\" set to zero, it is ignored.\n"
        "\n"
        "If \"PCH\" is NOT set, larger groupings obtained by larger value of \"ESIZ\".\n"
        "\n"
        "Mode 1: Sort to Crescendo.\n"
        "Mode 2: Sort to Decrescendo.\n"
        "Mode 3: Sort to Accelerando. (With very small elements, may rise in pitch).\n"
        "Mode 4: Sort to Ritardando.  (With very small elements, may fall in pitch).\n"
        "Mode 5: Order at Random.\n"
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

/****************************** DO_SORTER *********************************/

int do_sorter(dataptr dz)
{
    int exit_status;
    int peakcnt = 0, local_minima_cnt = 0, group_cnt = 0, ostep = 0;
    int bigbufsize, bigbufsize2, maxlen, n, *len;
    double *incr;

    /* FIND ALL POSITIVE-GOING-HALF-CYCLE PEAKS */
    
    if(dz->mode == RAND && dz->iparam[SORTER_SEED] > 0)
        srand(dz->iparam[SORTER_SEED]);                     //  Initialise randomisation
        
    if((exit_status = find_all_positive_peaks(&peakcnt,dz)) < 0)
        return(exit_status);

    /* FIND MINIMA AMONGST POSITIVE-PEAKS : overwriting the arrays peak-vals & peak-position with minima-vals & minima-positions */

    if(dz->brksize[SORTER_SIZE] || (dz->param[SORTER_SIZE] > 0.0)) {
    
        local_minima_cnt = 0;
        if((exit_status = find_all_local_minima(peakcnt,&local_minima_cnt,dz)) < 0)
            return(exit_status);

        /* GROUP THE MINIMA ACCORDING TO SPECIFIED WINDOW LENGTH */

        if((exit_status = group_local_minima(local_minima_cnt,&group_cnt,&ostep,dz))<0)
            return exit_status;
    } else
        group_cnt = peakcnt;

    /* SEARCH FOR ZERO CROSSINGS AFTER MINIMA */

    if((exit_status = locate_zero_crossings(group_cnt,dz)) < 0)
        return (exit_status);

    if(dz->param[SORTER_META] > 0.0) {

        /* SAVE THE ORIGINAL SEGMENT POSITIONS AND INCREMENTS */

        if((dz->parray[ORIGINCRS] = (double *)malloc((group_cnt+1) * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to bakup original input-read incrs.\n");
            return(MEMORY_ERROR);
        }
        dz->parray[ORIGINCRS][group_cnt] = 0.0;
        if((dz->lparray[ORIGPOS] = (int *)malloc((group_cnt+1) * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to bakup original segment positions.\n");
            return(MEMORY_ERROR);
        }
        dz->lparray[ORIGPOS][group_cnt] = 0;
        memcpy((char *)dz->parray[ORIGINCRS],(char *)dz->parray[INCRS],group_cnt * sizeof(double));
        memcpy((char *)dz->lparray[ORIGPOS],(char *)dz->lparray[POS],group_cnt * sizeof(int));

        /* FIND THE LARGER ELEMENT-GROUPINGS, AVERAGE THE INCR-VALS APPLYING TO THE ORIGINAL (SMALLER) ELEMENTS THEY CONTAIN */

        if((exit_status = larger_grouping(&group_cnt,dz))<0)
            return exit_status;
    }
    dz->buflen2 = dz->buflen;               //  If output segments are to be overlapped (in which case we need an overflow buffer to accomnodate largest poss seg)
                                            //  Check maximum size of output segments and IF NESS, make sound buffers bigger
    if(dz->brksize[SORTER_OMIDI] || (dz->param[SORTER_OMIDI] > 0)) {        
        maxlen = 0;
        len = dz->lparray[LEN];
        if(dz->param[SORTER_IMIDI] > 0) {
            incr = dz->parray[INCRS];
            for(n=0;n<group_cnt;n++)
                maxlen = max(maxlen,(int)ceil((double)len[n]/incr[n]));
        } else {
            for(n=0;n<group_cnt;n++)
                maxlen = max(maxlen,len[n]);
        }
        maxlen += 64;   //  SAFETY
        maxlen = ((maxlen/F_SECSIZE) + 1) * F_SECSIZE;
        if(dz->buflen < maxlen)
            dz->buflen2 = maxlen;
    }

    //  MAKE THE OUTPUT SOUND BUFFERS

    bigbufsize  = dz->buflen * sizeof(float);
    bigbufsize2 = dz->buflen2 * sizeof(float);
    free(dz->bigbuf);                   //  inbufs   outbufs
    if((dz->bigbuf = (float *)malloc((bigbufsize + bigbufsize2) * 2)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create larger sound buffers for output writes.\n");
        return(PROGRAM_ERROR);
    }
    for(n=0;n<3;n++)
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + dz->buflen2; 

    if(dz->mode == CRESC || dz->mode == DECRESC) {
        if((exit_status = find_levels(group_cnt,dz))<0)
            return exit_status;
    }
    group_cnt--;    /* There are N slicing points for groupsegs, and therefore N-1 groupsegs */

    if((exit_status = do_sorted_output(group_cnt,ostep,dz))<0)
        return exit_status;
    return FINISHED;
}

/****************************** FIND_ALL_POSITIVE_PEAKS ***********************************/

int find_all_positive_peaks(int *peakcnt,dataptr dz)
{
    int exit_status;
    double *peak, thispeak;
    int *pos, poscnt = 0;
    float *ibuf = dz->sampbuf[0];
    int thissamp = 0, thispos = 0, bufpos = 0;

    fprintf(stdout,"INFO: Finding positive peaks.\n");
    fflush(stdout);
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);

    while(ibuf[bufpos] <= 0) {      /* skip values below zero */
        if(++thissamp >= dz->insams[0]) {
            sprintf(errstr,"Cannot locate any (+ve) peaks in the signal.\n");
            return(DATA_ERROR);
        }
        if(++bufpos >= dz->buflen) {
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            bufpos = 0;
        }
    }
    while(thissamp < dz->insams[0]) {
        while(ibuf[bufpos] >= 0.0) {
            if(++thissamp >= dz->insams[0])
                break;
            if(++bufpos >= dz->buflen) {
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                bufpos = 0;
            }
        }
        poscnt++;                       // poscnt counts the number of positive 1/2 cycles in the signal
        while(ibuf[bufpos] < 0) {       // then skip over -ve part of signal
            if(++thissamp >= dz->insams[0])
                break;
            if(++bufpos >= dz->buflen) {
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                bufpos = 0;
            }
        }
    }
    poscnt += 16;   //  SAFETY
    if((dz->parray = (double **)malloc(4 * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create double data storage.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[PEAKS] = (double *)malloc((poscnt+1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create peak-data storage.\n");
        return(MEMORY_ERROR);
    }
    dz->parray[PEAKS][poscnt] = 0.0;
    if((dz->lparray = (int **)malloc(3 * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create int storage.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[POS] = (int *)malloc((poscnt+1) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create minima-time-data storage. (2)\n");
        return(MEMORY_ERROR);
    }
    dz->lparray[POS][poscnt] = 0;

    peak = dz->parray[PEAKS];
    pos  = dz->lparray[POS];
    *peak++ = 0;                            /* Store a zero value at start of peaks */
    *pos++ = 0;                             /* this becmoes the first local minimum */
    sndseekEx(dz->ifd[0],0,0);
    thissamp = 0;
    bufpos = 0;
    thispeak = -1.0;
    while(ibuf[bufpos] <= 0) {              /* skip values below zero */
        thissamp++;
        if(++bufpos >= dz->buflen) {
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            bufpos = 0;
        }
    }
    while(thissamp < dz->insams[0]) {
        if(ibuf[bufpos] >= 0.0) {
            if(ibuf[bufpos] > thispeak) {   /* search for (positive) peak val */
                thispeak = ibuf[bufpos];    
                thispos  = thissamp;
            }
        } else {
            peak[*peakcnt] = thispeak;      /* once signal becomes -ve, store last found peak */
            pos[*peakcnt] = thispos;
            (*peakcnt)++;
            while(ibuf[bufpos] < 0) {       /* then skip over -ve part of signal */
                if(++thissamp >= dz->insams[0])
                    break;
                if(++bufpos >= dz->buflen) {
                    if((exit_status = read_samps(ibuf,dz))<0)
                        return(exit_status);
                    bufpos = 0;
                }
            }
            thispeak = ibuf[bufpos];    /* once signal is +ve again, set up an initial value for peak */
            thispos  = thissamp;
        }
        thissamp++;
        if(++bufpos >= dz->buflen) {
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            bufpos = 0;
        }
    }
    if(*peakcnt > 0) {                      /* check for peak found near end, before signal goes -ve once more */
        if((thispos != pos[(*peakcnt)-1]) && (thispeak > 0.0)) {
            peak[*peakcnt] = thispeak;
            pos[*peakcnt] = thispos;
            (*peakcnt)++;
        }
    }
    if(*peakcnt < 4) {
        sprintf(errstr,"Insufficient signal peaks found to search for loudness elements.\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/****************************** FIND_ALL_LOCAL_MINIMA ***********************************/

int find_all_local_minima(int peakcnt,int *local_minima_cnt,dataptr dz)
{
    int thispeak;
    double *peak = dz->parray[PEAKS];
    int *pos = dz->lparray[POS];
    int finished = 0;

    fprintf(stdout,"INFO: Finding local minima.\n");
    fflush(stdout);

    *local_minima_cnt = 1;
    thispeak = 2;
    while(thispeak < peakcnt) {
        while(peak[thispeak] <= peak[thispeak-1]) { /* while peaks are falling, look for local peak minimum */
            if(++thispeak >= peakcnt) {
                finished = 1;
                break;
            }
        }
        if(finished)
            break;
        peak[*local_minima_cnt] = peak[thispeak-1]; /* store value and position of local mimimum */
        pos[*local_minima_cnt]  = pos[thispeak-1];
        (*local_minima_cnt)++;
        while(peak[thispeak] >= peak[thispeak-1]) { /* skip over rising sequence of peaks */
            if(++thispeak >= peakcnt) {
                break;
            }
        }
    }
    if(*local_minima_cnt < 3) {
        sprintf(errstr,"Insufficient local minima found.\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/****************************** GROUP_LOCAL_MINIMA ***********************************
 *
 *  element_dur is average length of element searched for, in samples
 */

int group_local_minima(int local_minima_cnt,int *group_cnt,int *ostep,dataptr dz)
{
    int exit_status;
    int thisminimum, element_dur, lo_element_dur, hi_element_dur, startpos, sampdur, minpos = 0;
    double *minimum = dz->parray[MINIMA], *incr = NULL, median_frq = 0, minmin, thistime = 0.0, srate = (double)dz->infile->srate;
    double o_median_frq = 0;
    int *pos = dz->lparray[POS];

    if(dz->param[SORTER_IMIDI] > 0) {           //  If transposing the elements, need to find median pitch & store their original pitches
        if((exit_status = get_median_pitch(dz->param[SORTER_IMIDI],&median_frq,dz))<0)
            return exit_status;
        if((dz->parray[INCRS] = (double *)malloc((local_minima_cnt+1) * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create peak-data storage.\n");
            return(MEMORY_ERROR);
        }
        dz->parray[INCRS][local_minima_cnt] = 0.0;
        incr = dz->parray[INCRS];
    }
    if(!dz->brksize[SORTER_OMIDI] && (dz->param[SORTER_OMIDI] > 0)) {   //  If setting repaet rate of output events to a single frq
        if((dz->param[SORTER_OMIDI] == 128) && (dz->param[SORTER_OMIDI] == 128))
            *ostep = (int)round(srate/median_frq);                      // If setting to input-median, and median frq already known, use it
        else {                                                          //  else, calculate median, or any other SINGLE pitch being used for output.
            if((exit_status = get_median_pitch(dz->param[SORTER_OMIDI],&o_median_frq,dz))<0)
                return exit_status;
            *ostep = (int)round(srate/o_median_frq);
        }
    }
    fprintf(stdout,"INFO: Grouping minima.\n");
    fflush(stdout);

    if(dz->brksize[SORTER_SIZE]) {
        if((exit_status = read_value_from_brktable(0,SORTER_SIZE,dz))<0)
            return exit_status;
        if(dz->param[SORTER_IMIDI] > 0)
            incr[0] = median_frq * dz->param[SORTER_SIZE];  //  Transpostion = mew_frq/orig_frq = new_frq/(1.0/wavelen) = new_frq * wavelen
    }
    element_dur = (int)round(dz->param[SORTER_SIZE] * srate);
    lo_element_dur = (int)round((double)element_dur * TWO_THIRDS);
    hi_element_dur = element_dur + (element_dur/2);
    *group_cnt = 1;
    thisminimum = 1;
    startpos = pos[0];
    while(thisminimum < local_minima_cnt) {     //  Go through all the minima looking for a set of minima falling between the specified time-
        minmin = 2.0;                           //  Set an impossible value for the minum of these minima(!)
        sampdur = pos[thisminimum] - startpos;  //  How far is the this minimum from start of group of minima?
        if( sampdur <= lo_element_dur)          //  If below required range, ignore and go to next minimum
            ;
        else if (sampdur < hi_element_dur) {    //  If within range, is this lower than any other minimum found so far within range
            if(minimum[thisminimum] < minmin) {
                minmin = minimum[thisminimum];
                minpos = pos[thisminimum];      //  IF so remember its value and position
            }
        } else {                                //  Otherwise we've exceeded the searchrange.       
            if(minmin == 2.0) {                 //  If no minimum found within range: use minimum we're at now
                minmin = minimum[thisminimum];
                minpos = pos[thisminimum];
            }                                   //  Write end of grouped-minima back into minima array (overwriting original values)
            minimum[*group_cnt] = minmin;
            pos[*group_cnt] = minpos;           //  and advance in count of gp-minima
            startpos = minpos;
            if(dz->brksize[SORTER_SIZE]) {
                thistime = (double)startpos/srate;
                if((exit_status = read_value_from_brktable(thistime,SORTER_SIZE,dz))<0)
                    return exit_status;
                if(dz->param[SORTER_IMIDI] > 0)
                    incr[*group_cnt] = median_frq * dz->param[SORTER_SIZE];
                element_dur = (int)round(dz->param[SORTER_SIZE] * srate);
                lo_element_dur = (int)round((double)element_dur * TWO_THIRDS);
                hi_element_dur = element_dur + (element_dur/2);
            }
            (*group_cnt)++;
        }
        thisminimum++;
    }
    if(*group_cnt < 1) {
        sprintf(errstr,"Insufficient sortable elements found.\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/****************************** LOCATE_ZERO_CROSSINGS ***********************************/

int locate_zero_crossings(int group_cnt,dataptr dz)
{
    int exit_status, finished = 0, bufno, thisbuf, bufpos;
    int n, *len;
    float  *ibuf = dz->sampbuf[0];
    double *trof = dz->parray[MINIMA];
    int   *pos  = dz->lparray[POS];

    fprintf(stdout,"INFO: Finding zero-crossings.\n");
    fflush(stdout);

    sndseekEx(dz->ifd[0],0,0);
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);
    bufno = 0;
    if((dz->lparray[LEN] = (int *)malloc((group_cnt+1) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create element level storage.\n");
        return(MEMORY_ERROR);
    }
    dz->lparray[LEN][group_cnt] = 0;
    len = dz->lparray[LEN];
    for(n=1;n<group_cnt;n++) {
        bufpos = pos[n];
        thisbuf = bufpos/dz->buflen;
        while(thisbuf > bufno) {
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);
            bufno++;
        }
        bufpos %= dz->buflen;
        while (trof[n] >= 0.0) {                /* advance position from minimum +ve peak until value crosses zero */
            pos[n]++;
            if(pos[n] >= dz->insams[0]) {
                finished = 1;
                if(trof[n] > 0.0) {             /* if end of file does not go to zero, Warn */
                    fprintf(stdout,"WARNING: End_of_sound segment doesn't fall to zero level, & may cause clicks in output. (Dovetail end of sound?)\n");
                    fflush(stdout);
                }
                break;
            }
            bufpos++;
            if(bufpos >= dz->buflen) {
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                bufno++;
                bufpos -= dz->buflen;
            }
            trof[n] = ibuf[bufpos];
        }
        len[n-1] = pos[n] - pos[n-1];           /*  Store lengths of cut segments */
        if(finished)
            break;
    }
    return(FINISHED);
}

/****************************** FIND_LEVELS ***********************************/

int find_levels(int group_cnt,dataptr dz)
{
    int exit_status;
    int n, samppos, bufpos;
    float  *ibuf = dz->sampbuf[0];
    double *level, maxsamp;
    int   *pos = dz->lparray[POS];

    fprintf(stdout,"INFO: Finding loudness of elements.\n");
    fflush(stdout);

    if((dz->parray[LEVELS] = (double *)malloc((group_cnt+1) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create element level storage.\n");
        return(MEMORY_ERROR);
    }
    dz->parray[LEVELS][group_cnt] = 0.0;
    level = dz->parray[LEVELS];
    sndseekEx(dz->ifd[0],0,0);
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);
    samppos = pos[0];
    bufpos = samppos;
    while(bufpos >= dz->buflen) {
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        bufpos -= dz->buflen;
    }
    for(n=1;n<group_cnt;n++) {
        maxsamp = -2.0;
        while(samppos < pos[n]) {
            if(fabs(ibuf[bufpos]) > maxsamp)
                maxsamp = fabs(ibuf[bufpos]);
            if(++samppos >= dz->insams[0]) {
                sprintf(errstr,"ERROR IN READING BUFFERS.\n");
                return PROGRAM_ERROR;
            }
            if(++bufpos >= dz->buflen) {
                if((exit_status = read_samps(ibuf,dz))<0)
                    return(exit_status);
                bufpos = 0;
            }
        }
        *level = maxsamp;
        level++;
    }
    return(FINISHED);
}

/****************************** DO_SORTED_OUTPUT ***********************************/

int do_sorted_output(int group_cnt,int ostep,dataptr dz)
{
    int exit_status, *permm = NULL;
    int outcontrol = 0, passno;
    float  *ibuf = dz->sampbuf[0], *i_readbuf = dz->sampbuf[0], *i_ovflwbuf = dz->sampbuf[1], *obuf = dz->sampbuf[2], *o_ovflwbuf = dz->sampbuf[3];
    double *level = dz->parray[LEVELS];
    int   *pos  = dz->lparray[POS];
    int   *len  = dz->lparray[LEN];
    double *incr = dz->parray[INCRS];
    int   n, m, k, kk, stemp, outcnt, outend, obufpos = 0, ibufpos = 0, smooth = 0, maxsplice, bufswritten;
    int maxwrite = 0;
    int obufstart;
    double temp, minval, val, srate = (double)dz->infile->srate;
    double dibufpos, dibufend, splval = 0.0, diff, frac;
    double thistime;
    double normaliser, maxsamp;

    if(dz->brksize[SORTER_OMIDI])                   //  If brkpoint file controls outmidi pitch, flag that we must READ the "ostep" between output writes
        outcontrol = 1;                             //  (if ostep controlled byt a dingle param, it has already been set)
                                                                            
    if((!dz->brksize[SORTER_SIZE] && (dz->param[SORTER_SIZE] == 0.0)) || flteq(dz->param[SORTER_SMOOTH],0.0))
        smooth = 0;                                 //  No smoothing if using single waveset, or if no smoothing set as apram
    else {
        smooth = (int)round(dz->param[SORTER_SMOOTH] * MS_TO_SECS * srate);
        if(dz->brksize[SORTER_SIZE]) {
            if((exit_status = get_minvalue_in_brktable(&minval,SORTER_SIZE,dz))<0)
                return exit_status;
        } else
            minval = dz->param[SORTER_SIZE];        //  If using splice-smoothing on elements ...
        minval *= TWO_THIRDS;                       //  smoothing splice cannot be longer than
        minval /= 2.0;                              //  1/2 length of smallest segment searched for
        maxsplice = (int)floor(minval * srate);
        smooth = min(smooth,maxsplice);
    }

    //  SORT THE SEGMENTS

    fprintf(stdout,"INFO: Processing sound.\n");
    fflush(stdout);

    switch(dz->mode) {
    case(CRESC):
        for(n = 0; n < group_cnt-1; n++) {
            for(m = n+1; m < group_cnt; m++) {
                if(level[n] > level[m]) {
                    temp = level[n];
                    level[n] = level[m];
                    level[m] = temp;
                    stemp   = pos[n];
                    pos[n]  = pos[m];
                    pos[m]  = stemp;
                    stemp   = len[n];
                    len[n]  = len[m];
                    len[m]  = stemp;
                    if(dz->param[SORTER_IMIDI] > 0) {
                        temp = incr[n];
                        incr[n] = incr[m];
                        incr[m] = temp;
                    }
                }
            }
        }
        break;
    case(DECRESC):
        for(n = 0; n < group_cnt-1; n++) {
            for(m = n+1; m < group_cnt; m++) {
                if(level[n] < level[m]) {
                    temp = level[n];
                    level[n] = level[m];
                    level[m] = temp;
                    stemp   = pos[n];
                    pos[n]  = pos[m];
                    pos[m]  = stemp;
                    stemp   = len[n];
                    len[n]  = len[m];
                    len[m]  = stemp;
                    if(dz->param[SORTER_IMIDI] > 0) {
                        temp = incr[n];
                        incr[n] = incr[m];
                        incr[m] = temp;
                    }
                }
            }
        }
        break;
    case(ACCEL):
        for(n = 0; n < group_cnt-1; n++) {
            for(m = n+1; m < group_cnt; m++) {
                if(len[n] < len[m]) {
                    stemp   = pos[n];
                    pos[n]  = pos[m];
                    pos[m]  = stemp;
                    stemp   = len[n];
                    len[n]  = len[m];
                    len[m]  = stemp;
                    if(dz->param[SORTER_IMIDI] > 0) {
                        temp = incr[n];
                        incr[n] = incr[m];
                        incr[m] = temp;
                    }
                }
            }
        }
        break;
    case(RIT):
        for(n = 0; n < group_cnt-1; n++) {
            for(m = n+1; m < group_cnt; m++) {
                if(len[n] > len[m]) {
                    stemp   = pos[n];
                    pos[n]  = pos[m];
                    pos[m]  = stemp;
                    stemp   = len[n];
                    len[n]  = len[m];
                    len[m]  = stemp;
                    if(dz->param[SORTER_IMIDI] > 0) {
                        temp = incr[n];
                        incr[n] = incr[m];
                        incr[m] = temp;
                    }
                }
            }
        }
        break;
    case(RAND):
        if((permm = (int *)malloc(group_cnt * sizeof(int)))==NULL) {
            sprintf(errstr,"Insufficient memory to create segment-order permutation store.\n");
            return(MEMORY_ERROR);
        }                                                               
        rndpermm(group_cnt,permm);                              //  Permute order of segments
        break;
    }

    //  OUTPUT THE SOUND

    maxsamp = 0.0;                                              //  maxsamp and normaliser preset. Used when output segments overlap one-another
    normaliser = 1.0;
    if(outcontrol || (ostep > 0))                               //  If output segs overlapped, need two passes, so output can be normalised
        passno = 0;                                             //  Need to normalise
    else 
        passno = 1;
    while(passno < 2) {
        bufswritten = 0;
        memset((char *)obuf,0,dz->buflen2 * sizeof(float));
        memset((char *)o_ovflwbuf,0,dz->buflen2 * sizeof(float));
        maxwrite = 0;                                           //  If segments overlapped, maxwrite may be higher than end of last seg written 
        obufpos = 0;
        if(passno == 0) {
            fprintf(stdout,"INFO: Testing level.\n");
            fflush(stdout);
        } else {
            fprintf(stdout,"INFO: Writing sound.\n");
            fflush(stdout);
        }
        if(outcontrol) {                                            //  If output segment placement controlled by brktable, find current ostep
            if((exit_status = read_value_from_brktable(0.0,SORTER_OMIDI,dz))<0)
                return exit_status;                                 //  If controlled by simple fixed param this has already been set
            ostep = (int)round(dz->param[SORTER_OMIDI]);            //  NB ostep > 0 also FLAGS that output is (possibly) OVERLAPPED segments
        }                                                           //  (with no output-placement control we simply write-out from where we last got to)

        //  FOR EVERY SEGMENT

        for(n = 0; n < group_cnt; n++) {
                                                                    //  Where outsegs may overlap, and a step between written segs is given,
            obufstart = obufpos;                                    //  obufstart marks where current write starts, and is used as mark to step FROM, for next write position.
            if(dz->mode == RAND)
                k = permm[n];                                       //  Get the reordered segment to use
            else                                                    //  and seek to it in infile.
                k = n;
            sndseekEx(dz->ifd[0],pos[k],0);
            if(dz->param[SORTER_IMIDI] > 0)                         //  Where transposition to be done
                dz->buflen *= 2;                                    //  Read to a double buffer, the 2nd buffer being an overflow buffer    
            ibuf = dz->sampbuf[0];                                  //  Otherwize read to a single buffer
            memset((char *)ibuf,0,dz->buflen * sizeof(float));      
            if((exit_status = read_samps(ibuf,dz))<0)
                return(exit_status);

            //  IN CASE WHERE SEGMENTS ARE (POSSIBLY) TRANSPOSED (we have an input overflow buffer, for wraparound point).

            if(dz->param[SORTER_IMIDI] > 0) {                       //  Future reads (in this pass) will be to the (single) overflow buf (sampbuf1)
                dz->buflen /= 2;                                    //  With previous ovflwbuf being copied back into sampbuf[0], now called i_readbuf

                dibufpos = 0.0;                                     //  Set up the (fractionally incremented) pointer into input buffer.
                dibufend = len[k] - 1;                              //  and the reverse-counter from end of samps to write (this is for possible end-smothing splice).

                while(dibufpos < len[k]) {                          //  Read (by fractional increments) until we get to end of this segment.
                    if(ostep == 0) {
                        if(obufpos >= dz->buflen2) {                //  When using simple output buffer (no outseg overlaps), ALWAYS check for overflow
                            dz->process = GREV;                     //  and write to output if (single) buffer overflows
                            if((exit_status = write_samps(obuf,dz->buflen2,dz))<0)
                                return(exit_status);
                            dz->process = SORTER;
                            memset((char *)obuf,0,dz->buflen2 * sizeof(float));
                            obufpos  -= dz->buflen2;
                            maxwrite -= dz->buflen2;
                            bufswritten++;
                        }
                    } else if(dibufpos == 0) {                      //  When using o_ovflwbuf, only check overflow at START of write
                        if(obufpos >= dz->buflen2) {                //  If outptr has stepped beyond end of obuf, do a write
                                                                    //  Further sample reads of this segment MAY overflow into the ovflbuf
                                                                    //  so buffer must be long enough to accomodate longest seg when maximally tstretched.
                            if(passno == 0) {                       
                                for(kk = 0; kk < dz->buflen2; kk++) //  On 1st first pass, for overlapping segs, check the max output level
                                    maxsamp = max(maxsamp,fabs(obuf[kk]));
                            } else {                                //  On 2nd pass, normalise output level before writing to file.
                                for(kk = 0; kk < dz->buflen2; kk++)
                                    obuf[kk] = (float)(obuf[kk] * normaliser);
                                dz->process = GREV;
                                if((exit_status = write_samps(obuf,dz->buflen2,dz))<0)
                                    return(exit_status);
                                dz->process = SORTER;
                            }                                       //  Do copy-back of output-o_ovflwbuf into true obuf
                            memset((char *)obuf,0,dz->buflen2 * sizeof(float));
                            memcpy((char *)obuf,(char *)o_ovflwbuf,dz->buflen2 * sizeof(float));
                            memset((char *)o_ovflwbuf,0,dz->buflen2 * sizeof(float));
                            obufpos  -= dz->buflen2;
                            obufstart -= dz->buflen2;               //  Readjust the marker for the start-place in buffer from which to measure step to next-write.
                            maxwrite -= dz->buflen2;                //  Readjust marker of maximum sample written in obuf
                            bufswritten++;
                        }
                    }                                               //  Due to interpolation we need a wrap-around sample at buf end (hence we have an i_ovflwbuf)
                    if(dibufpos >= dz->buflen) {                    //  If input pointer runs beyond i_readbuf end
                                                                    //  Copy i_ovflwbuf into i_readbuf, and get samps into i_ovflwbuf
                        memcpy((char *)i_readbuf,(char *)i_ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)i_ovflwbuf,0,dz->buflen * sizeof(float));
                        if((exit_status = read_samps(i_ovflwbuf,dz))<0) 
                            return(exit_status);
                        dibufpos -= dz->buflen;;
                    }
                    if(smooth) {
                        if(dibufpos < smooth)
                            splval = dibufpos/(double)smooth;
                        else if(dibufend< smooth)
                            splval = dibufend/(double)smooth;
                        else
                            splval = 1.0;
                    }
                    ibufpos = (int)floor(dibufpos);
                    val  = i_readbuf[ibufpos];
                    diff = i_readbuf[ibufpos+1] - i_readbuf[ibufpos];
                    frac = dibufpos - (double)ibufpos;
                    val += diff * frac;
                    obuf[obufpos] = (float)(obuf[obufpos] + (val * splval));
                    obufpos++;
                    dibufpos += incr[k];
                    dibufend -= incr[k];
                }
            } else {
                ibufpos = 0;
                outcnt = 0;
                outend = len[k] - 1;
                while(outcnt < len[k]) {
                    if(ostep == 0) {
                        if(obufpos >= dz->buflen2) {
                            dz->process = GREV;
                            if((exit_status = write_samps(obuf,dz->buflen2,dz))<0)
                                return(exit_status);
                            dz->process = SORTER;
                            memset((char *)obuf,0,dz->buflen2 * sizeof(float));
                            obufpos  -= dz->buflen2;
                            maxwrite -= dz->buflen2;
                            bufswritten++;
                        }
                    } else if(outcnt == 0) {
                        if(obufpos >= dz->buflen2) {
                            if(passno == 0) {
                                for(kk = 0; kk < dz->buflen2; kk++)
                                    maxsamp = max(maxsamp,fabs(obuf[kk]));
                            } else {
                                for(kk = 0; kk < dz->buflen2; kk++)
                                    obuf[kk] = (float)(obuf[kk] * normaliser);
                                dz->process = GREV;
                                if((exit_status = write_samps(obuf,dz->buflen2,dz))<0)
                                    return(exit_status);
                                dz->process = SORTER;
                            }
                            memset((char *)obuf,0,dz->buflen2 * sizeof(float));
                            memcpy((char *)obuf,(char *)o_ovflwbuf,dz->buflen2 * sizeof(float));
                            memset((char *)o_ovflwbuf,0,dz->buflen2 * sizeof(float));
                            obufpos   -= dz->buflen2;
                            obufstart -= dz->buflen2;
                            maxwrite  -= dz->buflen2;
                            bufswritten++;
                        }
                    }
                    if(ibufpos >= dz->buflen) {
                        if((exit_status = read_samps(ibuf,dz))<0)
                            return(exit_status);
                        ibufpos = 0;
                    }
                    if(smooth) {
                        if(outcnt < smooth)
                            splval = (double)outcnt/(double)smooth;
                        else if(outend < smooth)
                            splval = (double)outend/(double)smooth;
                        ibuf[ibufpos] = (float)(ibuf[ibufpos] * splval);
                    }
                    obuf[obufpos] = (float)(obuf[obufpos] + ibuf[ibufpos]);
                    obufpos++;
                    ibufpos++;
                    outcnt++;
                    outend--;
                }
            }
            maxwrite = max(obufpos,maxwrite);
            if(outcontrol) {                        //  If output segment placement controlled by brktable, find current ostep
                thistime = (double)((bufswritten * dz->buflen) + obufstart + ostep)/srate;
                if((exit_status = read_value_from_brktable(thistime,SORTER_OMIDI,dz))<0)
                    return exit_status;
                ostep = (int)round(dz->param[SORTER_OMIDI]);
            }
            if(ostep > 0)                           //  if ostep set by a simple parameter, it's already known
                obufpos = obufstart + ostep;
                                                    //  If ostep is 0, we just carry on writing in obuf from where we've got to
        }
        if(maxwrite > 0) {
            if(passno == 0) {
                for(kk = 0; kk < maxwrite; kk++)
                    maxsamp = max(maxsamp,fabs(obuf[kk]));
                if(maxsamp > MAXLEV)
                    normaliser = MAXLEV/maxsamp;
            } else {
                if(ostep > 0) {                     
                    for(kk = 0; kk < maxwrite; kk++)
                        obuf[kk] = (float)(obuf[kk] * normaliser);
                }
                dz->process = GREV;
                if((exit_status = write_samps(obuf,maxwrite,dz))<0)
                    return(exit_status);
                dz->process = SORTER;
            }
        }
        passno++;
    } 
    return FINISHED;
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

/****************************** GET_MEDIAN_PITCH ***********************************/

int get_median_pitch(double midi,double *medianfrq,dataptr dz)
{
    int n, pitch, midival[128], maxpcnt = 0, median = -1;
    if(midi < 128)
        *medianfrq = miditohz(midi);
    else {
        for(n=0;n<128;n++)
            midival[n] = 0;
        for(n=1;n<dz->brksize[SORTER_SIZE] * 2;n+=2) {
            pitch = (int)round(unchecked_hztomidi(1.0/dz->brk[SORTER_SIZE][n]));
            midival[pitch]++;
        }
        for(n=0;n<128;n++) {
            if(midival[n] > maxpcnt) {
                maxpcnt = midival[n];
                median = n;
            }
        }
        if(median < 0) {
            sprintf(errstr,"Failed to find mediam pitch of source.\n");
            return DATA_ERROR;
        }
        *medianfrq = miditohz(median);
    }
    return FINISHED;
}

/****************************** LARGER_GROUPING ***********************************
 *
 *  element_dur is average length of element searched for, in samples
 */

int larger_grouping(int *group_cnt,dataptr dz)
{
    int incrcnt;
    int thisminimum, element_dur, lo_element_dur, startpos, sampdur, origdatindx, startorigpos;
    double *incr = dz->parray[INCRS], *origincr = dz->parray[ORIGINCRS], newincr, srate = (double)dz->infile->srate;
    int *pos = dz->lparray[POS], *origpos = dz->lparray[ORIGPOS], *len  = dz->lparray[LEN], new_group_cnt;

    fprintf(stdout,"INFO: Re-grouping elements.\n");
    fflush(stdout);

    element_dur = (int)round(dz->param[SORTER_META] * srate);
    lo_element_dur = (int)round((double)element_dur * TWO_THIRDS);
    new_group_cnt = 1;                                  //  The firast position in the larger-group set is same as 1st in smaller group set
    thisminimum = 1;                                    //  So we start our searching at index1 (not 0)
    startpos = pos[0];                                  //  setting the place we're measuring from as the start-position index zero
    origdatindx = 1;                                    //  (and similarly in the original position and incr data)
    startorigpos = origpos[origdatindx];
    while(thisminimum < *group_cnt) {                   //  Go through all the original (grouped)minima looking for a set of minima falling within new larger-grouplen
        sampdur = pos[thisminimum] - startpos;          //  How far is the this minimum from start of larger-group of minima?
                                                        //  If below required range, ignore and go to next orig minimum
        if(sampdur > lo_element_dur) {                  //  Once enough orig minima to span the larger-group..
            newincr = 0.0;                              //  find the average incr-value amongst the original small elements
            incrcnt = 0;
            while(startorigpos <= pos[thisminimum]) {
                newincr += origincr[origdatindx-1];
                incrcnt++;
                origdatindx++;
                startorigpos = origpos[origdatindx];
            }
            newincr /= (double)incrcnt;                 //  Take the average.

            pos[new_group_cnt]    = pos[thisminimum];   //  Set new larger-group position (overwriting originals)
            incr[new_group_cnt-1] = newincr;            //  and new larger-group incr     (overwriting originals)
            len[new_group_cnt-1]  = pos[new_group_cnt] - pos[new_group_cnt-1];  //  and new larger group lengths (overwriting originals)
            startpos = pos[thisminimum];                //  Reset position for next search for larger groups
            new_group_cnt++;                            //  and advance in count of larger-groups
        }
        thisminimum++;                                  //  Advance in original group-positions
    }
    if(new_group_cnt < 1) {
        sprintf(errstr,"Insufficient sortable grouped-elements found.\n");
        return(DATA_ERROR);
    }
    *group_cnt = new_group_cnt;
    return(FINISHED);
}
