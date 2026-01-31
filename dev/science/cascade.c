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
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

#define CASC_SPLICELEN (5)      //  mS default splicelen
#define MINECHO     (0.01)      //  -40dB, quietest echo heard
#define CASPANCURVE (0.4)       //  Causes pan to be faster at start
#define ROOT2       (1.4142136)
#define CAS_MAXLEVEL (0.95)

#define alternating  is_rectified
#define spreading    is_mapping
#define echomax      rampbrksize
#define max_shredcnt total_windows

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int setup_cascade_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_cascade_param_ranges_and_defaults(dataptr dz);
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
static int handle_the_special_data(char *str,double *clipmin,dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int create_cascade_sndbufs(int clipmax,dataptr dz);
static int cascade_params_preprocess(int *clipmax,double *clipmin,int *is_shred,int *max_shredno,dataptr dz);
static void initialise_cascade_random_sequence(int seed);
static void do_shredding(int *shredcnt,int *csscnt,int passno,int cliplen,int spliclen,int max_shredno,dataptr dz);
static void permute_chunks(dataptr dz);
static void insert(int n,int t,dataptr dz);
static void prefix(int n,dataptr dz);
static void shuflup(int k,dataptr dz);
static int cascade(double clipmin,int clipmax,int is_shred,int max_shredno,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt, is_shred = 0, max_shredno = 0;
    int n, clipmax;
    double clipmin = 0.0;
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
    dz->itemcnt = 0;
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
        if((exit_status = get_the_process_no(argv[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdline++;
        cmdlinecnt--;
        dz->maxmode = 10;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_cascade_application(dz))<0) {
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
    if((exit_status = setup_cascade_param_ranges_and_defaults(dz))<0) {
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
    if((dz->lparray = (int **)malloc(5 * sizeof(int *)))==NULL) {           //  Arrays for input cut times, and shred-cut times
        sprintf(errstr,"INSUFFICIENT MEMORY to create \"int\" arrays.\n");  //  and for remembering randomised vals, for use in 2nd pass
        return(MEMORY_ERROR);                                               //  (to ensure output level does not change).
    }
    if(dz->mode >= 5) {
        if((exit_status = handle_the_special_data(cmdline[0],&clipmin,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdlinecnt--;
        cmdline++;
    }
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency()      redundant
    is_launched = TRUE;
    dz->bufcnt = 6;
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

    //param_preprocess()
    if((exit_status = cascade_params_preprocess(&clipmax,&clipmin,&is_shred,&max_shredno,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = create_cascade_sndbufs(clipmax,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //spec_process_file =
    if((exit_status = cascade(clipmin,clipmax,is_shred,max_shredno,dz))<0) {
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
    switch(dz->mode) {
    case(0):
    case(5):
        dz->outfile->channels = dz->infile->channels;
        break;
    case(1):
    case(2):
    case(6):
    case(7):
        dz->infile->channels  = 2;  //  Mono in, stereo out
        dz->outfile->channels = 2;
        break;
    case(3):
    case(4):
    case(8):
    case(9):
        dz->infile->channels  = 8;  //  Mono in, 8-chan out
        dz->outfile->channels = 8;
        break;
    }
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
        return(exit_status);
    switch(dz->mode) {
    case(0):
    case(5):
        break;  // input channels = output channels
    default:
        dz->infile->channels = 1;   //  Mono in
        break;
    }
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

/************************* SETUP_CASCADE_APPLICATION *******************/

int setup_cascade_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if(dz->mode < 5)  {
        if((exit_status = set_param_data(ap,0   ,3,3,"DID"))<0)
            return(FAILED);
    } else {
        if((exit_status = set_param_data(ap,CASCLIPS,3,1,"0I0"))<0)
            return(FAILED);
    }
    if((exit_status = set_vflgs(ap,"ersNC",5,"IDiII","aln",3,0,"000"))<0)
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
        } else if(!(dz->mode == 0 || dz->mode == 5) && infile_info->channels != 1) {
            sprintf(errstr,"File %s is not of correct type for Mode %d\n",cmdline[0],dz->mode+1);
            return(DATA_ERROR);
        }
        if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
            sprintf(errstr,"Failed to copy file parsing information\n");
            return(PROGRAM_ERROR);
        }
        free(infile_info);
    }
    return(FINISHED);
}

/************************* SETUP_CASCADE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_cascade_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    if(dz->mode < 5) {
        ap->lo[CAS_CLIP]    = 0.005;
        ap->hi[CAS_CLIP]    = 60.0;
        ap->default_val[CAS_CLIP]   = .5;
        ap->lo[CAS_MAXCLIP] = 0;
        ap->hi[CAS_MAXCLIP] = 60.0;
    }
    ap->lo[CAS_ECHO]    = 1;
    ap->hi[CAS_ECHO]    = 64;
    ap->default_val[CAS_ECHO] = 8;
    ap->default_val[CAS_MAXCLIP] = 0;
    ap->lo[CAS_MAXECHO] = 0;
    ap->hi[CAS_MAXECHO] = 64;
    ap->default_val[CAS_MAXECHO] = 0;
    ap->lo[CAS_RAND] = 0;
    ap->hi[CAS_RAND] = 1;
    ap->default_val[CAS_RAND] = 0;
    ap->lo[CAS_SEED] = 0;
    ap->hi[CAS_SEED] = 64;
    ap->default_val[CAS_SEED] = 1;
    ap->lo[CAS_SHREDNO] = 0;
    ap->hi[CAS_SHREDNO] = 64;
    ap->default_val[CAS_SHREDNO] = 0;
    ap->lo[CAS_SHREDCNT] = 0;
    ap->hi[CAS_SHREDCNT] = 64;
    ap->default_val[CAS_SHREDCNT] = 0;
    dz->maxmode = 10;
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
            if((exit_status = setup_cascade_application(dz))<0)
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
    usage2("cascade");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"cascade"))             dz->process = CASCADE;
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
    if(!strcmp(str,"cascade")) {
        fprintf(stdout,
        "USAGE: cascade cascade 1-5  inf outf clipsize  echos clipmax [-eechosmax] \n"
        "OR:    cascade cascade 6-10 inf outf cuts echos [-eechosmax]\n"
        "\n"
        "AND:   [-rrand] [-sseed] [-Nshredno -Cshredcnt -a] [-l] [-n]\n"
        "\n"
        "Successive segments of src are repeat-echoed, and echosets superimposed on src.\n"
        "\n"
        "Modes\n"
        "1,6:  N-channels in -> N channels out: Every echo in same N-channels.\n"
        "2,7:  Mono (left in output)  ->stereo: Echosets pan to right.\n"
        "3,8:  Mono (centre in output)->stereo: Echosets pan alternately to L and R.\n"
        "4,9:  Mono (ch1 in output) -> 8chan: Every echo steps R, to next channel.\n"
        "5,10: Mono (ch1 in outout) -> 8chan: Echos of 1st echoset step R,next set step L,etc.\n"
        "\n"
        "CLIPSIZE (Modes 1-5)  Duration of segments to echo. Time-variable.\n"
        "or       (Range .005 to 60 secs)\n"
        "CUTS     (Modes 6-10) Textfile of (successive) src cut-times, creating segs to echo.\n"
        "\n"
        "ECHOS    Number of echos. Time-variable. (Range 1 to 64)\n"
        "\n"
        "CLIPMAX  Max duration of clips (time-variable). \"CLIPSIZE\" now read as minimum.\n"
        "         Actual clipsize is a random val between \"clipsize\" and \"clipmax\".\n"
        "         If \"clipmax\" is set to zero, it is ignored.\n"
        "\n"
        "ECHOSMAX Max number of echos (time-variable). \"ECHOS\" now read as minimum.\n"
        "         Actual number of echos is a random val between \"echos\" and \"echosmax\".\n"
        "         If \"echosmax\" is set to zero, it is ignored.\n"
        "\n"
        "RAND     Randomise timesteps between echos in echo-set: Range 0-1, time-variable.\n"
        "SEED     With same non-zero value, randomised vals are exactly same on new pass.\n"
        "\n"
        "SHREDNO  In each echo-stream, cut previous echo into \"shredno\" parts,\n"
        "         and random-shuffle parts to make next echo. Range 2-16,time-variable.\n"
        "SHREDCNT No of shreds to do, to create next echo element. Range 1-16,time-variable.\n"
        "         BOTH \"shredno\" and \"shredcnt\" must be set for shredding to take place.\n" 
        "-a   Also shred original clip. Only valid if \"shredno\" & \"shredcnt\" set.\n"
        "\n"
        "-l   Echos decay linearly in level (default, log decay - initial decays faster).\n"
        "-n   If output low, normalise it. (high output is normalised by default).\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/******************************** CASCADE ********************************/

int cascade(double clipmin,int clipmax,int is_shred,int max_shredno,dataptr dz)
{
    int exit_status, clipcnt, inchans, outchans, passno, panright, thischan;
    double thistime, atten, level, splicatten = -1.0, leftgain, rightgain, maxsamp, diff, pan, panfactor, normaliser = 0.0, srate, offset;
    int readsamps = 0, totalsamps = 0, totalabsamps, lastsamptime, ebufpos, obufpos, startobufpos, shredcnt, rcnt, csscnt;
    int outgrps, maxwrite, cliplen, rval, echocnt, samps_written, effective_cliplen, randrange, true_gp_ebufpos, true_gp_obufpos, offsetcnt, maxclips, e, n, m;
    int *cliparray = NULL, *clipstore = dz->lparray[3];
    double *offsets;
    float *ibuf, *echobuf, *obuf, *ovflwbuf;
    int spliclen, splicendstt, minclip;
    maxclips = (int)ceil(dz->duration/clipmin) + 4;             //  Maximum possible number of clips cut from file (+4 for SAFETY)
    if((offsets = (double *)malloc((maxclips * dz->echomax) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create offsets array. (2)\n");
        return(MEMORY_ERROR);                                       //  Stores any random offset values, during passno 0, for use in pass 1
    }
    if(is_shred && dz->iparam[CAS_SEED] == 0)                       //  Shredding uses a rand process, which needs to be identical on the 2 passes
        dz->iparam[CAS_SEED] = 2;                                   //  SO if SEED has not been set (non-zero val) set a value now (2 : arbitrary)  
    dz->tempsize = (dz->insams[0]/dz->infile->channels) * dz->outfile->channels;
    ibuf     = dz->sampbuf[0];
    echobuf  = dz->sampbuf[3];
    obuf     = dz->sampbuf[4];
    ovflwbuf = dz->sampbuf[5];
    inchans  = dz->infile->channels;
    outchans = dz->outfile->channels;
    srate = (double)dz->infile->srate;
    spliclen = (int)round(CASC_SPLICELEN * MS_TO_SECS * srate);
    minclip = spliclen * 2 * inchans;
    if(dz->mode >= 5)
        cliparray = dz->lparray[0];

    for(passno = 0; passno < 2; passno++) {
        display_virtual_time(0,dz);
        initialise_cascade_random_sequence(dz->iparam[CAS_SEED]);
        switch(passno) {
        case(0): fprintf(stdout,"INFO: Checking level.\n");     break;
        case(1): fprintf(stdout,"INFO: Generating output.\n");  break;
        }
        fflush(stdout);
        memset((char *)obuf,0,dz->buflen2 * 2 * sizeof(float));     //  Empty obuf and ovflwbuf
        sndseekEx(dz->ifd[0],0,0);
        panright = 1;       //  default to pan to right
        clipcnt = 0;
        thistime = 0.0;
        lastsamptime = 0;
        totalabsamps = 0;
        obufpos = 0;
        maxsamp = 0.0;
        maxwrite = 0;
        offsetcnt = 0;
        true_gp_obufpos = 0;                                    //  unrandomised position in obuf, in grp-samples
        dz->total_samps_written = 0;
        shredcnt = 1;
        rcnt = 0;                       //  Count randomised values of echocnt and cliplen              
        csscnt = 0;                     //  Count randomised values of chunk scatter                
        for(;;) {
            if(dz->mode >= 5) {
                totalsamps = cliparray[clipcnt];        //  counted in groups
                cliplen = totalsamps - lastsamptime;
                lastsamptime = totalsamps;
            } else {
                if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
                    return exit_status;
                dz->iparam[CAS_CLIP] = (int)round(dz->param[CAS_CLIP] * dz->infile->srate);
                if(dz->param[CAS_MAXCLIP] == 0.0) {
                    cliplen = dz->iparam[CAS_CLIP];
                } else {
                    if(passno == 0) {
                        dz->iparam[CAS_MAXCLIP] = (int)round(dz->param[CAS_MAXCLIP] * dz->infile->srate);
                        diff = dz->iparam[CAS_MAXCLIP] - dz->iparam[CAS_CLIP];
                        rval = (int)floor(drand48() * diff);
                        cliplen = dz->iparam[CAS_CLIP] + rval;
                        clipstore[rcnt++] = cliplen;
                    } else 
                        cliplen = clipstore[rcnt++];
                }
            }
            if(clipcnt > 0) {                                           //  After 1st clip, we baktrak by splicelen, to overlap with previous clip. 
                totalabsamps -= spliclen * dz->infile->channels;        //  So totalabssamps (position in infile) baktraks by splicelen
                cliplen += spliclen;                                    //  and clip extended by spliclen
            }
            readsamps = cliplen * dz->infile->channels;
            clipcnt++;
            totalabsamps += readsamps;
            if(dz->iparam[CAS_MAXECHO] == 0)
                echocnt = dz->iparam[CAS_ECHO];
            else {
                if(passno == 0) {
                    diff = dz->iparam[CAS_MAXECHO] - dz->iparam[CAS_ECHO];
                    rval = (int)floor(drand48() * diff);
                    echocnt = dz->iparam[CAS_ECHO] + rval;
                    clipstore[rcnt++] = (int)echocnt;
                } else
                    echocnt = (int)clipstore[rcnt++];
            }
            memset((char *)ibuf,0,dz->buflen * sizeof(float));
            if((exit_status = read_samps(ibuf,dz))<0)               //  Read samples at appropriate place
                return(exit_status);
            if(dz->ssampsread <= minclip)
                break;
            if(dz->ssampsread < readsamps) {                        //  If insufficient samples to make the specified clip, shorten the clip
                totalabsamps -= readsamps;
                readsamps = dz->ssampsread; 
                cliplen = readsamps/dz->infile->channels;
                totalabsamps += readsamps;
            }
            effective_cliplen = cliplen - spliclen; 
            randrange = effective_cliplen - 1;                      //  Range for possible random change of echo entry-time, in grp-samples.
            if(dz->vflag[CAS_LINEAR])
                atten = (1.0 - MINECHO)/(double)echocnt;
            else
                atten = pow(MINECHO,1.0/(double)echocnt);           //  Caluclate attenuation-at-each-echo to produce final echo level of MINECHO
            level = 1.0;
            ebufpos = 0;                                            //  Sample position in echobuf
            true_gp_ebufpos = 0;                                    //  unrandomised position in echobuf, in grp-samples
            startobufpos = obufpos;
            memset((char *)echobuf,0,dz->buflen2 * sizeof(float));  //  Clear echobuf
            splicendstt = cliplen - spliclen;                       //  Start of endsplice, counted in group-samples.

            if(is_shred && dz->vflag[CAS_SHREDSRC])                 //  If source is to be shredded, do it
                do_shredding(&shredcnt,&csscnt,passno,cliplen,spliclen,max_shredno,dz);

            switch(dz->mode) {  
            case(0):                                                //  In simple echos (no motion, but possibly multichan : mode 0)
            case(1):                                                //  or mono into stereo
            case(2):
            case(5):
            case(6):
            case(7):
                for(e=0;e <= echocnt; e++) {                        //  For original source and all echos
                    for(n=0;n < cliplen; n++)  {                    //  Copy the input clip to output
                        if(n < spliclen)                            //  Splicing start and end when appropriate
                            splicatten = (double)n/(double)spliclen;
                        else if(n >= splicendstt)
                            splicatten = (double)(cliplen - n - 1)/(double)spliclen;
                        else
                            splicatten = 1.0;                               
                        for(m = 0; m < inchans; m++) {
                            echobuf[ebufpos] = (float)(echobuf[ebufpos] + (ibuf[(n * inchans) + m] * level * splicatten));
                            ebufpos++;
                        }
                    }
                    if(is_shred)
                        do_shredding(&shredcnt,&csscnt,passno,cliplen,spliclen,max_shredno,dz);
                    
                    if((dz->param[CAS_RAND] > 0) && (e < echocnt)) {//  Last echo must be NOT random-shifted
                        true_gp_ebufpos += cliplen;                 //  to ensure complete set of echos fits within alotted buffer space
                        if(e < echocnt - 1) {
                            if(passno == 0) {
//HEREH STORE e
                                offset  = (drand48() * 2.0) - 1.0;  //  Range -1 to 1

                                offsets[offsetcnt++] = offset;
                            } else
                                offset = offsets[offsetcnt++];
                            offset *= dz->param[CAS_RAND];          //  Range +- randparam val
                            offset *= randrange;                    //  Range +- within size of permissible shift
                            ebufpos = true_gp_ebufpos + (int)floor(offset);
                        } else
                            ebufpos = true_gp_ebufpos;              //  Last echo always in unrandomised place place
                        ebufpos *= inchans;                         //  Change to total (ungrouped) sample count
                    }
                    ebufpos -= spliclen * inchans;                  //  Baktrak (splices echos together if not time-randomised)
                    if(dz->vflag[CAS_LINEAR])
                        level -= atten;
                    else
                        level *= atten;                             //  And attenuate from echo to echo
                }
                ebufpos += spliclen * inchans;                      //  Advance to end of echo-output, ready for outputting data
                break;
            default:                                    //  FOR MONO TO MULTICHANNEL
                for(e=0;e <= echocnt; e++) {                        //  For original source and all echos
                    for(n=0;n < cliplen; n++)  {                    //  Copy the input clip to output
                        if(ebufpos < spliclen)                      //  Splicing start and end when appropriate
                            splicatten = (double)n/(double)spliclen;
                        else if(ebufpos >= splicendstt)
                            splicatten = (double)(cliplen - n - 1)/(double)spliclen;
                        else
                            splicatten = 1.0;                       //  Here source is alwats mono          
                        echobuf[ebufpos++] = (float)(ibuf[n] * level * splicatten);
                    }
                    for(n=0; n < ebufpos; n++) {                    //  Copy each echo of the clip into output buffer AT THIS STAGE
                        obuf[obufpos] = (float)(obuf[obufpos] + echobuf[n]);
                        obufpos += outchans;                        //  advance to next corresponding channel's sample
                    }
                    maxwrite = max(maxwrite,obufpos);               //  Check maxwrite at each pass, as obufpos may backtrack (see next lines)
                    if(is_shred)
                        do_shredding(&shredcnt,&csscnt,passno,cliplen,spliclen,max_shredno,dz);
                    if(dz->param[CAS_RAND] > 0) {
                        thischan = obufpos % 8;
                        true_gp_obufpos += cliplen - spliclen;
                        if(e < echocnt - 1) {
                            if(passno == 0) {
//HEREH STORE e
                                offset  = (drand48() * 2.0) - 1.0;  //  Range -1 to 1
                                offsets[offsetcnt++] = offset;
                            } else
                                offset = offsets[offsetcnt++];
                            offset *= dz->param[CAS_RAND];          //  Range +- randparam val
                            offset *= randrange;                    //  Range +- within size of permissible shift
                            obufpos = true_gp_obufpos + (int)floor(offset);
                        } else                                      //  penultimate echo fixes final echo to be in unrandomised position
                            obufpos = true_gp_obufpos;              //  Last echo always in unrandomised place place
                        obufpos *= outchans;                        //  Change to total (ungrouped) sample count
                        obufpos += thischan;                        //  Offset to existing channel
                    }
                    if(e < echocnt) {
                        if(panright > 0)
                            obufpos++;                              //  Move to next output chan for next echo
                        else
                            obufpos--;                              //  Move to previous output chan for next echo
                    }
                    memset((char *)echobuf,0,dz->buflen2 * sizeof(float));
                    if(dz->vflag[CAS_LINEAR])
                        level -= atten;
                    else
                        level *= atten;                             //  And attenuate from echo to echo
                    ebufpos = 0;                                    //  Reset echobuffer for next echo
                }
                if(dz->alternating)                                 //  In alternating modes, swap between clockwise and anticlockwise motion
                    panright = -panright;
                outgrps = maxwrite/outchans;                        //  Round up maxwrite to a whole (multichan) set of samples
                if(outgrps * outchans < maxwrite)
                    outgrps++;
                maxwrite = outgrps * outchans;
                break;
            }

            //  For Mode 0 and stereo-output modes, now write to output buffer

            switch(dz->mode) {  
            case(0):
            case(5):
                for(n=0; n < ebufpos; n++) {                    //  Copy echoed clip into output buffer
                    obuf[obufpos] = (float)(obuf[obufpos] + echobuf[n]);
                    obufpos++;
                }
                maxwrite = max(maxwrite,obufpos);
                outgrps = maxwrite/outchans;                    //  Round up maxwrite to a whole (multichan) set of samples
                if(outgrps * outchans < maxwrite)
                    outgrps++;
                maxwrite = outgrps * outchans;
                break;
            case(1):                                            //  Mono source --> Stereo
            case(2):
            case(6):
            case(7):
                panfactor = (double)(ebufpos - 1);
                for(n=0; n < ebufpos; n++) {                    //  Copy echoed clip into output buffer, panning left to right
                    pan = (double)n/panfactor;                  //  Pan position in range 0 to 1
                    pan = pow(pan,CASPANCURVE);                 //  Bias the pan to be faster at start
                    if(dz->spreading)
                        pan = (pan * 2.0) - 1.0;                //  Pan position in range -1 to 1 left to right)
                    else if(panright < 0)
                        pan *= -1.0;                            //  Pan position in range 0 to -1 centre to left)
                    else
                        ;                                       //  Pan position in range 0 to 1 centre to right)
                    pancalc(pan,&leftgain,&rightgain);
                    obuf[obufpos] = (float)(obuf[obufpos] + (echobuf[n] * leftgain));
                    obufpos++;
                    obuf[obufpos] = (float)(obuf[obufpos] + (echobuf[n] * rightgain));
                    obufpos++;
                }
                maxwrite = max(maxwrite,obufpos);
                outgrps = maxwrite/outchans;                    //  Round up maxwrite to a whole (multichan) set of samples
                if(outgrps * outchans < maxwrite)
                    outgrps++;
                maxwrite = outgrps * outchans;

                if(dz->alternating)
                    panright = -panright;                       //  Alternating modes : alternate pans to left and right) 
                break;          
            }
            
            // MOVE TO WRITE POSITION FOR NEXT SET-OF-ECHOS
                                                                //  Move write-position for next set-of-echos, baktraking by a splicelen
            obufpos = startobufpos + ((cliplen - spliclen) * outchans);
            true_gp_obufpos = obufpos/outchans;
            if(obufpos >= dz->buflen2) {                        //  and if beyond end of obuf, do write
                if(passno == 0) {
                    for(n=0;n < dz->buflen2;n++)
                        maxsamp = max(maxsamp,fabs(obuf[n]));
                } else {
                    for(n=0;n < dz->buflen2;n++)
                        obuf[n] = (float)(obuf[n] * normaliser);
                    if((samps_written = fputfbufEx(obuf,dz->buflen2,dz->ofd))<=0) {
                        sprintf(errstr,"Can't write to output soundfile: %s\n",sferrstr());
                        return(SYSTEM_ERROR);
                    }
                    dz->total_samps_written += samps_written;   
                }                                               //  then copy back any overflow
                dz->process = GREV;                             //  Force correct display of progress_bar in Loom
                display_virtual_time(totalabsamps,dz);
                dz->process = CASCADE;
                memset((char *)obuf,0,dz->buflen2 * sizeof(float));
                memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen2 * sizeof(float));
                memset((char *)ovflwbuf,0,dz->buflen2 * sizeof(float));
                obufpos  -= dz->buflen2;
                true_gp_obufpos -= dz->buflen2/outchans;
                maxwrite -= dz->buflen2;
            }
            if(totalabsamps >= dz->insams[0])
                break;
            sndseekEx(dz->ifd[0],totalabsamps,0);
        }
        if(maxwrite > 0) {
            if(passno == 0) {
                for(n=0;n < maxwrite;n++)
                    maxsamp = max(maxsamp,fabs(obuf[n]));
            } else {
                for(n=0;n < maxwrite;n++)
                    obuf[n] = (float)(obuf[n] * normaliser);
                if((samps_written = fputfbufEx(obuf,maxwrite,dz->ofd))<=0) {        //  Write remaining samps
                    sprintf(errstr,"Can't write to output soundfile: %s\n",sferrstr());
                    return(SYSTEM_ERROR);
                }
                dz->total_samps_written += samps_written;
            }
            dz->process = GREV;
            display_virtual_time(totalabsamps,dz);
            dz->process = CASCADE;
        }
        if(passno == 0) {
            if(flteq(maxsamp,0.0)) {
                sprintf(errstr,"No significant signal in output.\n");
                return(PROGRAM_ERROR);
            }
            if(dz->vflag[CAS_UPNORMAL])
                normaliser = CAS_MAXLEVEL/maxsamp;
            else {
                normaliser = 1.0;
                if(maxsamp > CAS_MAXLEVEL)
                    normaliser = CAS_MAXLEVEL/maxsamp;
            }
        }
    }
    return FINISHED;
}

/******************************** CASCADE_PARAMS_PREPROCESS ********************************/

int cascade_params_preprocess(int *clipmax,double *clipmin,int *is_shred,int *max_shredno,dataptr dz)
{
    int exit_status;
    double thisclipmax, thisclipmin = 0.0, thisclipmaxmin, maxecho, this_echomin, this_echomax, this_shredno;
    double effective_clipmin, mincliplen, srate = (double)dz->infile->srate;
    int this_clipmax, n, thiscut, lastclip, spliclen, min_shredcnt, min_shredno, arraysize;
    int *cliparray = NULL;
    if(dz->param[CAS_MAXECHO] == 0)
        dz->brksize[CAS_MAXECHO] = 0;
    if(dz->mode >= 5) {
        dz->param[CAS_MAXCLIP] = 0.0;
        dz->brksize[CAS_MAXCLIP] = 0;
        dz->param[CAS_CLIP] = 0.0;
        dz->brksize[CAS_CLIP] = 0;
        cliparray = dz->lparray[0];
        *clipmax = 0;
        lastclip = 0;
        for(n=0;n < dz->itemcnt;n++) {
            thiscut = cliparray[n] - lastclip;
            *clipmax = max(*clipmax,thiscut);
            lastclip = cliparray[n];
        }
    } else {
        if(dz->param[CAS_MAXCLIP] == 0)
            dz->brksize[CAS_MAXCLIP] = 0;
        if(dz->brksize[CAS_CLIP]) {
            if((exit_status = get_maxvalue_in_brktable(&thisclipmax,CAS_CLIP,dz))<0)
                return exit_status;
            *clipmax = (int)ceil(thisclipmax * srate);
            if((exit_status = get_minvalue_in_brktable(&thisclipmin,CAS_CLIP,dz))<0)
                return exit_status;
        } else {
            thisclipmin = dz->param[CAS_CLIP];
            dz->iparam[CAS_CLIP] = (int)round(dz->param[CAS_CLIP] * srate);
            *clipmax = dz->iparam[CAS_CLIP];
        }
        if(dz->brksize[CAS_MAXCLIP]) {
            if((exit_status = get_minvalue_in_brktable(&thisclipmaxmin,CAS_MAXCLIP,dz))<0)
                return exit_status;
            if(thisclipmaxmin < dz->application->lo[CAS_CLIP]) {
                sprintf(errstr,"Brktable for clipmax contains invalid value(s) (below %lf secs)\n",dz->application->lo[CAS_CLIP]);
                return DATA_ERROR;
            }
            thisclipmin = min(thisclipmin,thisclipmaxmin);
            if((exit_status = get_maxvalue_in_brktable(&thisclipmax,CAS_MAXCLIP,dz))<0)
                return exit_status;
            this_clipmax = (int)ceil(thisclipmax * srate);
            *clipmax = max(*clipmax,this_clipmax);
        } else if (dz->param[CAS_MAXCLIP] > 0) {
            if (dz->param[CAS_MAXCLIP] < dz->application->lo[CAS_CLIP]) {
                sprintf(errstr,"Clipmax value (%lf) invalid (below %lf secs)\n",dz->param[CAS_MAXCLIP],dz->application->lo[CAS_CLIP]);
                return DATA_ERROR;
            }
            thisclipmin = min(thisclipmin,dz->param[CAS_MAXCLIP]);
            dz->iparam[CAS_MAXCLIP] = (int)ceil(dz->param[CAS_MAXCLIP] * srate);//  Max Clip duration in grouped-samples
            *clipmax = max(*clipmax,dz->iparam[CAS_MAXCLIP]);
        }
        *clipmin = thisclipmin;
        effective_clipmin = *clipmin - (CASC_SPLICELEN * MS_TO_SECS);
        dz->itemcnt = (int)ceil(dz->duration/effective_clipmin) + 1;            //  Max possible number of clips
    }
    if(dz->brksize[CAS_ECHO]) {
        if((exit_status = get_maxvalue_in_brktable(&maxecho,CAS_ECHO,dz))<0)
            return exit_status;
        dz->echomax = (int)round(maxecho);
    } else
        dz->echomax = dz->iparam[CAS_ECHO];                                                     // Max no of echos
    
    if(dz->brksize[CAS_MAXECHO]) {
        if((exit_status = get_minvalue_in_brktable(&this_echomin,CAS_MAXECHO,dz))<0)
            return exit_status;
        if((int)round(this_echomin) < (int)round(dz->application->lo[CAS_ECHO])) {
            sprintf(errstr,"Brktable for echomax contains invalid value (%d) (below %d)\n",(int)round(this_echomin),(int)round(dz->application->lo[CAS_ECHO]));
            return DATA_ERROR;
        }
        if((exit_status = get_maxvalue_in_brktable(&this_echomax,CAS_MAXECHO,dz))<0)
            return exit_status;
        dz->echomax = max(dz->echomax,(int)round(this_echomax));
    } else if (dz->iparam[CAS_MAXECHO] > 0) {
        if ((int)round(dz->iparam[CAS_MAXECHO]) < (int)round(dz->application->lo[CAS_ECHO])) {
            sprintf(errstr,"Echomax value (%d) invalid (below %d)\n",(int)round(dz->iparam[CAS_MAXECHO]),(int)round(dz->application->lo[CAS_ECHO]));
            return DATA_ERROR;
        }
        dz->echomax = max(dz->echomax,dz->iparam[CAS_MAXECHO]);                                     // Max no of echos
    }
    spliclen = (int)round(CASC_SPLICELEN * MS_TO_SECS * srate);
    *clipmax += spliclen;
    if(dz->brksize[CAS_SHREDCNT]) { 
        if((exit_status = get_minvalue_in_brktable(&this_shredno,CAS_SHREDNO,dz))<0)
            return exit_status;
        min_shredcnt = (int)round(this_shredno);
        if(min_shredcnt < 1) {
            sprintf(errstr,"Invalid shred count (%d) in shred-count file. (Minimum 1).\n",min_shredcnt);
            return DATA_ERROR;
        }
        if((exit_status = get_maxvalue_in_brktable(&this_shredno,CAS_SHREDNO,dz))<0)
            return exit_status;
        dz->max_shredcnt = (int)round(this_shredno);
    } else {
        min_shredcnt = dz->iparam[CAS_SHREDCNT];
        dz->max_shredcnt = dz->iparam[CAS_SHREDCNT];
    }
    if(dz->brksize[CAS_SHREDNO]) {  
        if((exit_status = get_minvalue_in_brktable(&this_shredno,CAS_SHREDNO,dz))<0)
            return exit_status;
        min_shredno = (int)round(this_shredno);
        if(min_shredno < 2) {
            sprintf(errstr,"Invalid shred number (%d) in shred-number file. (Minimum 2).\n",min_shredno);
            return DATA_ERROR;
        }
        if((exit_status = get_maxvalue_in_brktable(&this_shredno,CAS_SHREDNO,dz))<0)
            return exit_status;
        *max_shredno = (int)round(this_shredno);
    } else {
        *max_shredno = dz->iparam[CAS_SHREDNO];
        min_shredno = dz->iparam[CAS_SHREDNO];
    }
    if(((min_shredno == 0) && (min_shredcnt != 0)) || ((min_shredno != 0) && (min_shredcnt == 0))) {
        sprintf(errstr,"SHRED NUMBER and SHRED COUNT must both be set, or must both be zero.\n");
        return DATA_ERROR;
    }
    if((*clipmin/(double)*max_shredno)/3.0 <= dz->application->lo[CAS_CLIP]) {  //  Shreds into shredno pieces, which can be up to 1/3 as short
        mincliplen = dz->application->lo[CAS_CLIP] * (double)*max_shredno * 3.0;
        sprintf(errstr,"With shred number of (up to) %d, shortest segment must be > %.3lf secs.\n",*max_shredno,mincliplen);
        return DATA_ERROR;
    }
    if(dz->vflag[CAS_SHREDSRC] && (min_shredno == 0 || min_shredcnt == 0)) {
        sprintf(errstr,"To shred the source, both \"shred number\" or \"shred count\" must be given.\n");
        return DATA_ERROR;
    }
    if (min_shredno > 0) {
        *is_shred = 1;
        if((dz->lparray[1] = (int *)malloc((*max_shredno+1) * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create shred pointers array.\n");
            return(MEMORY_ERROR);
        }
        if((dz->lparray[2] = (int *)malloc((*max_shredno) * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create shred lengths array.\n");
            return(MEMORY_ERROR);
        }
        arraysize = dz->itemcnt * (dz->echomax + 1 + dz->vflag[CAS_SHREDSRC]) * dz->max_shredcnt + 4;   //  +4 = SAFETY
        if((dz->iparray = (int **)malloc(arraysize * sizeof(int *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create shred permutation array. (1)\n");
            return(MEMORY_ERROR);
        }
        for(n = 0; n < arraysize;n++) {
            if((dz->iparray[n] = (int *)malloc((*max_shredno + 1) * sizeof(int)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to create shred permutation array. (2)\n");
                return(MEMORY_ERROR);
            }
        }
    }
    if((dz->brksize[CAS_MAXCLIP] || dz->param[CAS_MAXCLIP] > 0) || (dz->brksize[CAS_MAXECHO] || dz->param[CAS_MAXECHO] > 0)) {
        if((dz->lparray[3] = (int *)malloc((dz->itemcnt+1) * 2 * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create randomised echocnt && cliplen arrays.\n");
            return(MEMORY_ERROR);
        }
    }
    if(*is_shred) {
                    // clips            echos                                   // shreds-per-echo  // shred elements           
        arraysize = dz->itemcnt * (dz->echomax + 1 + dz->vflag[CAS_SHREDSRC]) * dz->max_shredcnt * (*max_shredno) + 4;
        if((dz->lparray[4] = (int *)malloc(arraysize * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create array to store randomised shred values.\n");
            return(MEMORY_ERROR);
        }
    }
    switch(dz->mode) {
    case(2):
    case(7):
    case(4):
    case(9):
        dz->alternating = 1;        //  stero : pan to right, to left, to right etc     8chan: rotate clock, anticlok, clok etc
        break;
    default:
        dz->alternating = 0;
        break;
    }
    switch(dz->mode) {
    case(1):
    case(6):
        dz->spreading = 1;
        break;
    default:
        dz->spreading = 0;
        break;
    }
    return FINISHED;
}

/******************************** CREATE_CASCADE_SNDBUFS ********************************/

int create_cascade_sndbufs(int clipmax,dataptr dz)
{
    int bigbufsize, secsize;
    int framesize1 = F_SECSIZE * dz->infile->channels;
    int framesize2 = F_SECSIZE * dz->outfile->channels;

    if(dz->sbufptr == 0 || dz->sampbuf == 0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    dz->buflen  = clipmax * dz->infile->channels;
    if(dz->buflen < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create input sound buffers. (1)\n");
        return(PROGRAM_ERROR);
    }
    secsize = dz->buflen/framesize1;
    if(secsize * framesize1 != dz->buflen)
        secsize++;
    dz->buflen = secsize * framesize1;
    if(dz->buflen < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create input sound buffers. (2)\n");
        return(PROGRAM_ERROR);
    }
    dz->buflen2 = clipmax * (dz->echomax+1) * dz->outfile->channels; 
    if(dz->buflen2 < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create output sound buffers. (1)\n");
        return(PROGRAM_ERROR);
    }
    secsize = dz->buflen2/framesize2;
    if(secsize * framesize2 != dz->buflen2)
        secsize++;
    dz->buflen2 = secsize * framesize2;
    if(dz->buflen2 < 0) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create output sound buffers. (2)\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = ((3 * dz->buflen) + (3 * dz->buflen2)) * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create total sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;                   //  Inbuf
    dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;  //  Shredbuf
    dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;  //  Shredsplicebuf
    dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + dz->buflen;  //  Calculation buf for echos
    dz->sbufptr[4] = dz->sampbuf[4] = dz->sampbuf[3] + dz->buflen2; //  Outbuf
    dz->sbufptr[5] = dz->sampbuf[5] = dz->sampbuf[4] + dz->buflen2; //  Overflowbuf
    dz->sampbuf[6] = dz->sampbuf[6] + dz->buflen2;
    return(FINISHED);
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

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,double *clipmin,dataptr dz)
{
    int n, cnt, firsttime = 1;
    int inlen = dz->insams[0]/dz->infile->channels;                 //  Length of src file, in grouped samples
    FILE *fp;
    char temp[200], *p;
    double dummy = 0, cutlen = 0.0, lasttime, endcut, srate = (double)dz->infile->srate;
    double mincut = 2 * CASC_SPLICELEN * MS_TO_SECS;                    //  Min length of segment to echo > 2 * splicelen
    int mincutgpsamps = (int)ceil(mincut * (double)dz->infile->srate);  //  and in grouped_samples
    int lastclipcut;
    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read clip lengths.\n",str);
        return(DATA_ERROR);
    }
    cnt = 0;
    lasttime = 0;
    firsttime = 1;
    *clipmin = HUGE;
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy)) {
            cutlen = dummy - lasttime;
            if(cutlen <= mincut) {
                if(firsttime)
                    sprintf(errstr,"Invalid clip length between zero and first value (%lf) in file %s (must be greater than %lf seconds)\n",dummy,str,mincut);
                else
                    sprintf(errstr,"Invalid clip length (%lf) between times (%lf & %lf) in file %s (must be greater than %lf seconds)\n",cutlen,lasttime,dummy,str,mincut);
                return(DATA_ERROR);
            }
            if(dummy <= dz->duration)       //  Look for minimum cutsize WITHIN the file to be cut
                *clipmin = min(*clipmin,cutlen);
            lasttime = dummy;
            firsttime = 0;
            cnt++;
        }
    }
    if(cnt < 2) {
        sprintf(errstr,"Must be more than 1 clip time value in file %s.\n",str);
        return(DATA_ERROR);
    }
    dz->itemcnt = cnt;

    endcut = dz->duration - dummy;          //  If data stops before file end, find remaining file segment (endcut is +ve if last cut before file end)
    if(endcut > mincut)                     //  If endcut is negative, ignore. If <= mincut, gets joined into previous cut, making that larger than any previous minimum
        *clipmin = min(*clipmin,endcut);    //  If > mincut, treated as a separate cut, which could be smaller than current minimum

    if((dz->lparray[0] = (int *)malloc((dz->itemcnt + 1) * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create clip lengths array (2).\n");
        return(MEMORY_ERROR);
    }
    cnt = 0;
    fseek(fp,0,0);
    while(fgets(temp,200,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        while(get_float_from_within_string(&p,&dummy))
            dz->lparray[0][cnt++] = (int)round(dummy * srate);
    }
    for(n=0;n < dz->itemcnt;n++) {          //  Loose any data which is beyond end of infile
        if(dz->lparray[0][n] > inlen) {
            dz->itemcnt = n;
            break;
        }
    }
    if(dz->itemcnt <= 0) {
        sprintf(errstr,"No valid clip times in file %s.\n",str);
        return(MEMORY_ERROR);
    }
    dz->lparray[0][dz->itemcnt] = inlen;    //  Add end-of-file as last cutpoint
    lastclipcut = dz->lparray[0][dz->itemcnt] - dz->lparray[0][dz->itemcnt - 1];
    if(lastclipcut < mincutgpsamps)         //  If final clip is too small, eliminate it
        dz->lparray[0][dz->itemcnt-1] = inlen;
    else
        dz->itemcnt++;
    if(dz->itemcnt <= 1) {
        sprintf(errstr,"Less than 2 valid clip times in file %s.\n",str);
        return(MEMORY_ERROR);
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
        dirflag = -1;       /* signal to left  */
    else
        dirflag = 1;        /* signal to right */

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
        if(dirflag < 0) {   /* SIGNAL_TO_LEFT */
            *leftgain = invsquare;
            *rightgain = 0.0;
        } else {            /* SIGNAL_TO_RIGHT */
            *rightgain = invsquare;
            *leftgain = 0;
        }
    }
}

/***************************** INITIALISE_CASCADE_RANDOM_SEQUENCE ***************************/

void initialise_cascade_random_sequence(int seed)
{
    if(seed > 0)
        srand((int)seed);
    else
        initrand48();
}

/************************** DO_SHREDDING *******************************/

void do_shredding(int *shredcnt,int *csscnt,int passno,int cliplen,int spliclen,int max_shredno,dataptr dz)
{   
    double this_scatter, val, newval;
    int n, m, i, k, j, inchans = dz->infile->channels;
    int chunkscat;
    int *chunkptr = dz->lparray[1], *chunklen = dz->lparray[2], *chunkscatstore = dz->lparray[4], shbufpos;
    int total_len, chnk_len;
    int rawlen = (int)round((double)cliplen/(double)dz->iparam[CAS_SHREDNO]);
    float *ibuf = dz->sampbuf[0], *shredbuf = dz->sampbuf[1], *shredsplicebuf = dz->sampbuf[2], *old_addr;
    int *permm = dz->iparray[0];
    for(i = 0; i < dz->iparam[CAS_SHREDCNT]; i++) {                 //  For the number of shreds required
        total_len = 0;
        chunkptr[0] = 0;
        for(n=1;n<dz->iparam[CAS_SHREDNO];n++) {
            if(passno == 0) {
                this_scatter  = 1.0 + (((drand48() * 2.0) - 1.0)/3.0);  //  Range 2/3 to 4/3
                chunkscat = (int)(this_scatter * (double)rawlen);
                chunkscatstore[(*csscnt)++] = chunkscat;
            } else 
                 chunkscat = chunkscatstore[(*csscnt)++];
            chunkptr[n] = total_len + chunkscat;                    //  Randomised position of each chunk-boundary 
            total_len += rawlen;                                    //  Unrandomised position of each chunk-boundary
        }
        chunkptr[n] = cliplen;
        for(n=0;n<dz->iparam[CAS_SHREDNO];n++)
            chunklen[n] = chunkptr[n+1] - chunkptr[n];              //  Find lengths of all chunks
        (*shredcnt)++;
        if(passno == 0) {
            permute_chunks(dz);                                     //  Permute order of chunks
            for(n=0;n < max_shredno;n++)
                dz->iparray[*shredcnt][n] = permm[n];
        } else {
            for(n=0;n < max_shredno;n++)
                permm[n] = dz->iparray[*shredcnt][n];
        }
        memset((char *)shredbuf,0,dz->buflen * sizeof(float));
        memset((char *)shredsplicebuf,0,dz->buflen * sizeof(float));
        shbufpos = 0;
        for(j=0;j<dz->iparam[CAS_SHREDNO];j++) {                    //  For each permuted shred-element
            old_addr = &(ibuf[chunkptr[permm[j]] * inchans]);       //  Address of (permuted) chunk
            chnk_len = chunklen[permm[j]];                          //  Length of  (permuted) chunk (in grouped-samples)
                                                                    //  Copy into splicebuf
            memcpy((char *)shredsplicebuf,(char *)old_addr,(chnk_len * inchans) * sizeof(float));
            for(n=0, m = chnk_len - 1;n < spliclen;n++,m--) {       //  Splice both ends
                val = (double)n/(double)spliclen;
                for(k=0;k < inchans;k++) {
                    newval = shredsplicebuf[(n * inchans) + k];
                    newval *= val;
                    shredsplicebuf[(n * inchans) + k] = (float)newval;
                    newval = shredsplicebuf[(m * inchans) + k];
                    newval *= val;
                    shredsplicebuf[(m * inchans) + k] = (float)newval;
                }
            }
            for(n=0;n < chnk_len * inchans;n++)                     //  Copy each chunk into shredbuf
                shredbuf[shbufpos++] = shredsplicebuf[n];
            memset((char *)shredsplicebuf,0,dz->buflen * sizeof(float));

        }                                                           //  Once all chunks assembled
        memset((char *)ibuf,0,dz->buflen * sizeof(float));          //  Copy shredded source into source buf
        memcpy((char *)ibuf,(char *)shredbuf,(cliplen * inchans) * sizeof(float));
    }
}

/*************************** PERMUTE_CHUNKS ***************************/

void permute_chunks(dataptr dz)
{   
    int n, t;
    for(n=0;n<dz->iparam[CAS_SHREDNO];n++) {
        t = (int)(drand48() * (double)(n+1));    /* TRUNCATE */
        if(t==n)
            prefix(n,dz);
        else
            insert(n,t,dz);
    }
}

/****************************** INSERT ****************************/

void insert(int n,int t,dataptr dz)
{   
    shuflup(t+1,dz);
    dz->iparray[0][t+1] = n;
}

/****************************** PREFIX ****************************/

void prefix(int n,dataptr dz)
{   
    shuflup(0,dz);
    dz->iparray[0][0] = n;
}

/****************************** SHUFLUP ****************************/

void shuflup(int k,dataptr dz)
{   
    int n;
    for(n = dz->iparam[CAS_SHREDNO] - 1; n > k; n--)
        dz->iparray[0][n] = dz->iparray[0][n-1];
}

