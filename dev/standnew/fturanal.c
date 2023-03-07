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
 *  SPLICES NOT WORKING IN REASSEMBLY.
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
#include <formants.h>
#include <speccon.h>

#define SAFETY 64
#define FTURRAND 0
#define maxstep is_sharp
#define minstep scalefact
#define maxframecnt rampbrksize

#define FMNT_BANDS          (4)
#define CHAN_SRCHRANGE_F    (4)

#ifdef unix
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.2.0";

//CDP LIB REPLACEMENTS
//static int check_specanal_param_validity_and_consistency(dataptr dz);
static int setup_fturanal_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_fturanal_param_ranges_and_defaults(dataptr dz);
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
//static int specanal(dataptr dz);

static int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int read_mark_data(char *filename,dataptr dz);
static int read_feature_data(char *filename,dataptr dz);
static int check_fturanal_param_validity_and_consistency(dataptr dz);
static int fturanal_param_preprocess(dataptr dz);
static int fturanal (dataptr dz);
static int ftursynth (dataptr dz);

static int ftur_initialise_specenv(dataptr dz);
static int ftur_set_specenv_frqs(dataptr dz);
static int ftur_setup_octaveband_steps(double **interval,dataptr dz);
static int ftur_setup_low_octave_bands(int arraycnt,dataptr dz);
static int ftur_extract_specenv(float *anal,dataptr dz);
static int get_median_brightness(double *brightness,double *framebrite,int framecnt,int startindex,dataptr dz);
static int get_median_formant_frqs(double *framefmnt1,double *framefmnt2,double *framefmnt3,double *fmnt1,double *fmnt2,double *fmnt3,int framecnt,int startindex, dataptr dz);
static int extract_brightness(float *buf,double *bright,dataptr dz);
static int extract_formant_frqs(double *f1,double *f2,double *f3,dataptr dz);
static int generate_tail_segments(dataptr dz);
static int create_fturanal_sndbufs(dataptr dz);

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
        switch(dz->process) {
        case(FTURANAL):     dz->maxmode = 3;    break;
        case(FTURSYNTH):    dz->maxmode = 10;   break;
        }
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_fturanal_application(dz))<0) {
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
    // open_first_infile        CDP LIB
    if((exit_status = open_first_infile(cmdline[0],dz))<0) {    
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);  
        return(FAILED);
    }
    cmdlinecnt--;
    cmdline++;
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_fturanal_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_extra_infiles() : redundant
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
//  handle_special_data ....
    switch(dz->process) {
    case(FTURANAL):             //  6 for: Seg starttimes, formant1 median freq, formant2 median freq, formant3 median freq, brightness, possibly expanded seg starttimes
        dz->array_cnt = 10;     //  4 for frame-by-frame vals of: formant1 freq, formant2 freq, formant3 freq, brightness
        dz->iarray_cnt = 2;     //  2 for: Marking which of segments is a Head : median Sorting bins
        break;
    case(FTURSYNTH):                
        dz->array_cnt = 5;      //  5 for: Seg starttimes, formant1 median freq, formant2 median freq, formant3 median freq, brightness
        dz->larray_cnt = 1;     //  1 for seg start-sampletime and end-sampletime pairs..
        dz->iarray_cnt = 1;     //  1 for: Sorting the data into order
        break;
    }
    if((exit_status = handle_the_special_data(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency .....
    switch(dz->process) {
    case(FTURANAL):
        if((exit_status = fturanal_param_preprocess(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    case(FTURSYNTH):
        if((exit_status = check_fturanal_param_validity_and_consistency(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    }
//  check_param_validity_and_consistency()  redundant
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = create_fturanal_sndbufs(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    switch(dz->process) {
    case(FTURANAL):
        if((exit_status = fturanal(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        break;
    case(FTURSYNTH):                
/* TEST */
fprintf(stderr,"START\n");
/* TEST */
        if((exit_status = ftursynth(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
/* TEST */
fprintf(stderr,"END\n");
/* TEST */
        break;
    }
    if((exit_status = complete_output(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    exit_status = print_messages_and_close_sndfiles(FINISHED,is_launched,dz);
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
    if(dz->process == FTURSYNTH) {
        if(filename[0]=='-' && filename[1]=='f') {
            dz->floatsam_output = 1;
            dz->true_outfile_stype = SAMP_FLOAT;
            filename+= 2;
        }
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

/************************* SETUP_FTURANAL_APPLICATION *******************/

int setup_fturanal_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    switch(dz->process) {
    case(FTURANAL):
        if((exit_status = set_param_data(ap,MARKLIST,0,0,""))<0)
            return(FAILED);
        if(dz->mode == 0) {
            if((exit_status = set_vflgs(ap,"r",1,"d","",0,0,""))<0)
                return(FAILED);
        } else {
            if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
                return(FAILED);
        }
        dz->input_data_type = ANALFILE_ONLY;
        dz->process_type    = TO_TEXTFILE;  
        dz->outfiletype     = TEXTFILE_OUT;
        dz->maxmode = 3;
        break;
    case(FTURSYNTH):
        if((exit_status = set_param_data(ap,FTURDATA ,0,0,""))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"s",1,"d","",0,0,""))<0)
            return(FAILED);
        dz->input_data_type = SNDFILES_ONLY;
        dz->process_type    = UNEQUAL_SNDFILE;  
        dz->outfiletype     = SNDFILE_OUT;
        dz->maxmode = 10;
        break;
    }
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
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
            return(DATA_ERROR);
        } else if((dz->process == FTURANAL  && infile_info->filetype != ANALFILE)
                ||(dz->process == FTURSYNTH && infile_info->filetype != SNDFILE))  {
            sprintf(errstr,"File %s is not of correct type\n",cmdline[0]);
            return(DATA_ERROR);
        } else if(dz->process == FTURSYNTH && infile_info->channels != 1)  {
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

/************************* SETUP_FTURANAL_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_fturanal_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    switch(dz->process) {
    case(FTURANAL):
        if(dz->mode == 0) {
            ap->lo[0] = (double)0;
            ap->hi[0] = (double)1;
            ap->default_val[0]  = 0.2;
        }
        dz->maxmode = 3;
        break;
    case(FTURSYNTH):
        dz->maxmode = 10;
        ap->lo[0] = (double)2;
        ap->hi[0] = (double)15;
        ap->default_val[0]  = 5;
        break;
    }
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
            if((exit_status = setup_fturanal_application(dz))<0)
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
    fprintf(stderr,
    "\nUSAGE: fturanal NAME (mode) infile outfile (parameters)\n"
    "\n"
    "AUTOMATIC FEATURE EXTRACTION\n"
    "\n"
    "where NAME can be any one of\n"
    "anal   synth"
    "\n"
    "Type 'fturanal anal' for more info on anal option.. ETC.\n");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(     !strcmp(prog_identifier_from_cmdline,"anal"))       dz->process = FTURANAL;
    else if(!strcmp(prog_identifier_from_cmdline,"synth"))      dz->process = FTURSYNTH;
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
    if(!strcmp(str,"anal")) {
        fprintf(stderr,
        "Extract spectral features from an analysis file: output to textfile.\n"
        "\n"
        "USAGE:\n"
        "fturanal anal 1   inanalfil outfeaturefil marklist [-rrand]\n"
        "fturanal anal 2-3 inanalfil outfeaturfile marklist\n"
        "\n"
        "MARKLIST A list of timemarks in source, marking (paired) Heads and Tails.\n"
        "         e.g. consonant onset, and vowel continuation of source.\n"
        "         (It is assumed that the first mark is at a Head segment.)\n"
        "\n"
        "Extract data ...\n"
        "MODE 1   from Heads, & from Tails cut to segments, size approx equal to Heads.\n"
        "MODE 2   from Heads and Tails, as defined by marklist.\n"
        "MODE 3   from Tails only.\n"
        "\n"
        "RAND     Randomise timing of Tail segments (Range 0 to 1).\n"
        "\n"
        "OUTFEATURFILE  Outfile is a list of ....\n"
        "         (a) Segment start-times (+ duration of source).\n"
        "         (b) (Median) Frequency of 1st formant.\n"
        "         (c) (Median) Frequency of 2nd formant.\n"
        "         (d) (Median) Frequency of 3rd formant.\n"
        "         (e) Spectral brightness (Range 0 to 1).\n");
    } else if(!strcmp(str,"synth")) {
        fprintf(stderr,
        "Use spectral features data to reassemble MONO source file.\n"
        "\n"
        "USAGE:\n"
        "fturanal synth 1-10 inwavfil outfile infeaturefile [-ssplicelen]\n"
        "\n"
        "INFEATUREFILE list of feature values obtained from the analysis file\n"
        "          derived from the INWAVFIL.\n"
        "        (a) Segment start-times (+ duration of source).\n"
        "        (b) (Median) Frequency of 1st formant.\n"
        "        (c) (Median) Frequency of 2nd formant.\n"
        "        (d) (Median) Frequency of 3rd formant.\n"
        "        (e) Spectral brightness (Range 0 to 1).\n"
        "\n"
        "SPLICELEN for cutting segments, in mS (Range 2 to 15: default 5).\n"
        "\n"
        "MODE 1  Reassemble in order of increasing 1st Formant.\n"
        "MODE 2  Reassemble in order of decreasing 1st Formant.\n"
        "MODE 3  Reassemble in order of increasing 2nd Formant.\n"
        "MODE 4  Reassemble in order of decreasing 2nd Formant.\n"
        "MODE 5  Reassemble in order of increasing 3rd Formant.\n"
        "MODE 6  Reassemble in order of decreasing 3rd Formant.\n"
        "MODE 7  Reassemble in order of increasing Brightness.\n"
        "MODE 9  Reassemble in order of increasing Formants, summed.\n"
        "MODE 10 Reassemble in order of decreasing Formants, summed.\n");
    } else {
        fprintf(stdout,"Unknown option '%s'\n",str);
        fflush(stdout);
    }
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

/************************ FTUR_INITIALISE_SPECENV ************************/

int ftur_initialise_specenv(dataptr dz)
{
    dz->clength = dz->wanted/2;
    dz->nyquist = dz->infile->origrate/2.0;
    dz->chwidth = dz->nyquist/(double)(dz->infile->channels / 2);
    if((dz->specenvfrq = (float *)malloc(dz->clength * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant frq array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->specenvpch = (float *)malloc(dz->clength * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant pitch array.\n");
        return(MEMORY_ERROR);
    }
    /*RWD  zero the data */
    memset(dz->specenvpch,0,dz->clength * sizeof(float));

    if((dz->specenvamp = (float *)malloc(dz->clength * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant aplitude array.\n");
        return(MEMORY_ERROR);
    }
    if((dz->specenvtop = (float *)malloc(dz->clength * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for formant frq limit array.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/************************ FTUR_SET_SPECENV_FRQS ************************
 *
 * PICHWISE BANDS  = number of points per octave
 */

int ftur_set_specenv_frqs(dataptr dz)
{
    int exit_status;
    int arraycnt;
    double bandbot;
    double *interval;
    int m, n, k = 0, cc;
    arraycnt   = dz->clength + 1;
    if((exit_status = ftur_setup_octaveband_steps(&interval,dz))<0)
        return(exit_status);
    if((exit_status = ftur_setup_low_octave_bands(arraycnt,dz))<0)
        return(exit_status);
    k  = TOP_OF_LOW_OCTAVE_BANDS;
    cc = CHAN_ABOVE_LOW_OCTAVES;
    while(cc <= dz->clength) {
        m = 0;
        if((bandbot = dz->chwidth * (double)cc) >= dz->nyquist)
            break;
        for(n=0;n<FMNT_BANDS;n++) {
            if(k >= arraycnt) {
                sprintf(errstr,"Formant array too small: ftur_set_specenv_frqs()\n");
                return(PROGRAM_ERROR);
            }
            dz->specenvfrq[k]   = (float)(bandbot * interval[m++]);
            dz->specenvpch[k]   = (float)log10(dz->specenvfrq[k]);
            dz->specenvtop[k++] = (float)(bandbot * interval[m++]);
        }   
        cc *= 2;        /* 8-16: 16-32: 32-64 etc as 8vas, then split into bands */
    }
    dz->specenvfrq[k] = (float)dz->nyquist;
    dz->specenvpch[k] = (float)log10(dz->nyquist);
    dz->specenvtop[k] = (float)dz->nyquist;
    dz->specenvamp[k] = (float)0.0;
    k++;
    dz->specenvcnt = k;
    return(FINISHED);
}

/************************* FTUR_SETUP_OCTAVEBAND_STEPS ************************/

int ftur_setup_octaveband_steps(double **interval,dataptr dz)
{
    double octave_step;
    int n = 1, m = 0, halfbands_per_octave = FMNT_BANDS * 2;
    if((*interval = (double *)malloc(halfbands_per_octave * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing interval array for formants.\n");
        return(MEMORY_ERROR);
    }
    while(n < halfbands_per_octave) {
        octave_step   = (double)n++/(double)halfbands_per_octave;
        (*interval)[m++] = pow(2.0,octave_step);
    }
    (*interval)[m] = 2.0;
    return(FINISHED);
}

/************************ ftur_setup_low_octave_bands ***********************
 *
 * Lowest PVOC channels span larger freq steps and therefore we must
 * group them in octave bands, rather than in anything smaller.
 */

int ftur_setup_low_octave_bands(int arraycnt,dataptr dz)
{
    int n;
    arraycnt   = dz->clength + 1;
    if(arraycnt < LOW_OCTAVE_BANDS) {
        sprintf(errstr,"Insufficient array space for low_octave_bands\n");
        return(PROGRAM_ERROR);
    }
    for(n=0;n<LOW_OCTAVE_BANDS;n++) {
        switch(n) {
        case(0):
            dz->specenvfrq[0] = (float)1.0;                 /* frq whose log is 0 */
            dz->specenvpch[0] = (float)0.0;
            dz->specenvtop[0] = (float)dz->chwidth;         /* 8VA wide bands */
            break;
        case(1):
            dz->specenvfrq[1] = (float)(dz->chwidth * 1.5); /* Centre Chs 1-2 */    
            dz->specenvpch[1] = (float)log10(dz->specenvfrq[1]);
            dz->specenvtop[1] = (float)(dz->chwidth * 2.0);
            break;
        case(2):
            dz->specenvfrq[2] = (float)(dz->chwidth * 3.0); /* Centre Chs 2-4 */
            dz->specenvpch[2] = (float)log10(dz->specenvfrq[2]);
            dz->specenvtop[2] = (float)(dz->chwidth * 4.0);
            break;
        case(3):
            dz->specenvfrq[3] = (float)(dz->chwidth * 6.0); /* Centre Chs 4-8 */    
            dz->specenvpch[3] = (float)log10(dz->specenvfrq[3]);
            dz->specenvtop[3] = (float)(dz->chwidth * 8.0);
            break;
        default:
            sprintf(errstr,"Insufficient low octave band setups in ftur_setup_low_octave_bands()\n");
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/********************** FTUR_EXTRACT_SPECENV *******************/

int ftur_extract_specenv(float *anal,dataptr dz)
{
    int    n, cc, vc, specenvcnt_less_one;
    int    botchan, topchan;
    double botfreq, topfreq;
    double bwidth_in_chans;
    float *ampstore;
    ampstore = dz->specenvamp;
    specenvcnt_less_one = dz->specenvcnt - 1;
    vc = 0;
    for(n=0;n<dz->specenvcnt;n++)
        ampstore[n] = (float)0.0;
    topfreq = 0.0f;
    n = 0;
    while(n < specenvcnt_less_one) {
        botfreq  = topfreq;
        botchan  = (int)((botfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
        botchan -= CHAN_SRCHRANGE_F;
        botchan  =  max(botchan,0);
        topfreq  = dz->specenvtop[n];
        topchan  = (int)((topfreq + dz->halfchwidth)/dz->chwidth); /* TRUNCATE */
        topchan += CHAN_SRCHRANGE_F;
        topchan  =  min(topchan,dz->clength);
        for(cc = botchan,vc = botchan * 2; cc < topchan; cc++,vc += 2) {
            if(anal[FREQ] < 0.0)            // rectify if ness
                anal[FREQ] = (float)-anal[FREQ];
            if(anal[FREQ] >= botfreq && anal[FREQ] < topfreq)
                ampstore[n] = (float)(ampstore[n] + anal[AMPP]);
        }
        bwidth_in_chans   = (double)(topfreq - botfreq)/dz->chwidth;
        ampstore[n] = (float)(ampstore[n]/bwidth_in_chans);
        n++;
    }
    return(FINISHED);
}

/************************ HANDLE_THE_SPECIAL_DATA *********************/

int handle_the_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int exit_status;
    if(!sloom) {
        if(*cmdlinecnt <= 0) {
            sprintf(errstr,"Insufficient parameters on command line.\n");
            return(USAGE_ONLY);
        }
    }
    switch(dz->process) {
    case(FTURANAL):
        if((exit_status = read_mark_data((*cmdline)[0],dz))<0)
            return(exit_status);
        break;
    case(FTURSYNTH): 
        if((exit_status = read_feature_data((*cmdline)[0],dz))<0)
            return(exit_status);
        break;
    }
    (*cmdline)++;       
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************************ READ_MARK_DATA ************************************/

int read_mark_data(char *filename,dataptr dz)
{
    double *time, lasttime, dummy, timestep;
    int cnt, warned = 0;
    int arraysize;
    char temp[200], *q;
//    aplptr ap;
    FILE *fp;

//    ap = dz->application;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr, "Can't open file %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    cnt = 0;
    lasttime = -1.0;
    dz->minstep = HUGE;
    dz->maxstep = 0.0;
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,&dummy)) {
            if(dummy < 0.0 || dummy <= lasttime) {
                sprintf(errstr,"Times do not advance correctly in file %s.\n",filename);
                return(DATA_ERROR);
            }
            if(dummy >= dz->duration) {
                if(!warned) {
                    fprintf(stdout,"WARNING: Times beyond end of sndfile (%lf) in file %s. Ignoring them.\n",dz->duration,filename);
                    fflush(stdout);
                    warned = 1;
                }
                break;
            }
            if(cnt > 0) {
                timestep = dummy - lasttime;
                if(timestep < dz->minstep)
                    dz->minstep = timestep;
                if(timestep > dz->maxstep)
                    dz->maxstep = timestep;
            }
            lasttime = dummy;
            cnt++;
        }
    }       
    if(cnt == 0) {
        sprintf(errstr,"No data in file %s\n",filename);
        return(DATA_ERROR);
    }
    if(cnt < 4) {
        sprintf(errstr,"Insufficient data (%d values) in file %s : Need at least 4\n",cnt,filename);
        return(DATA_ERROR);
    }
    timestep = dz->duration - lasttime;
    if(timestep < dz->minstep)
        dz->minstep = timestep;
    if(timestep > dz->maxstep)
        dz->maxstep = timestep;

    if((dz->parray = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s. (1)\n",filename);
        return(MEMORY_ERROR);
    }
    if((dz->iparray = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s. (3)\n",filename);
        return(MEMORY_ERROR);
    }
    if(dz->mode == 0) {
        arraysize = (int)ceil(dz->duration/dz->minstep) + SAFETY;                       //  In this mode, tail segs are further subdivided
        if((dz->iparray[0] = (int *)malloc(arraysize * sizeof(int)))==NULL) {           //  Array for Head/Tail flags for labelling segments
            sprintf(errstr,"INSUFFICIENT MEMORY for Head/Tail flags for labelling segments\n");
            return(MEMORY_ERROR);
        }
        if((dz->parray[0] = (double *)malloc(arraysize * sizeof(double)))==NULL) {      //  Array for segment times + endtime
            sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s. (2)\n",filename);
            return(MEMORY_ERROR);
        }
    } else {
        arraysize = (int)ceil(dz->itemcnt) + SAFETY;
        if((dz->iparray[0] = (int *)malloc(arraysize * sizeof(int)))==NULL) {               //  Array for Head/Tail flags for labelling segments
            sprintf(errstr,"INSUFFICIENT MEMORY for Head/Tail flags for labelling segments\n");
            return(MEMORY_ERROR);
        }
        if((dz->parray[0] = (double *)malloc((cnt + 1) * sizeof(double)))==NULL) {          //  Array for Head/Tail times + endtime
            sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s. (2)\n",filename);
            return(MEMORY_ERROR);
        }
    }   
    time = dz->parray[0];
    rewind(fp);
    lasttime = -1.0;
    cnt = 0;
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,&dummy)) {
            if(dummy < 0.0 || dummy <= lasttime) {
                sprintf(errstr,"Times do not advance correctly in file %s.\n",filename);
                return(DATA_ERROR);
            }
            if(dummy >= dz->duration) {
                if(!warned) {
                    fprintf(stdout,"WARNING: Times beyond end of sndfile (%lf) in file %s. Ignoring them.\n",dz->duration,filename);
                    fflush(stdout);
                }
                break;
            }
            *time = dummy;
            if(cnt > 0) {
                timestep = *time - lasttime;
                if(timestep < dz->minstep)
                    dz->minstep = timestep;
                if(timestep > dz->maxstep)
                    dz->maxstep = timestep;
            }
            lasttime = *time;
            time++;
            cnt++;
        }
    }
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
        fflush(stdout);
    }
    if((dz->parray[1] = (double *)malloc(arraysize * sizeof(double)))==NULL) {          //  Array for Formant1 data from all segs
        sprintf(errstr,"INSUFFICIENT MEMORY Array for Formant1 data from all segs.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[2] = (double *)malloc(arraysize * sizeof(double)))==NULL) {          //  Array for Formant2 data from all segs
        sprintf(errstr,"INSUFFICIENT MEMORY Array for Formant2 data from all segs.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[3] = (double *)malloc(arraysize * sizeof(double)))==NULL) {          //  Array for Formant3 data from all segs
        sprintf(errstr,"INSUFFICIENT MEMORY Array for Formant3 data from all segs.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[4] = (double *)malloc(arraysize * sizeof(double)))==NULL) {          //  Array for Brightness data from all segs
        sprintf(errstr,"INSUFFICIENT MEMORY Array for Brightness data from all segs.\n");
        return(MEMORY_ERROR);
    }
    if(dz->mode == 0) {
        if((dz->parray[5] = (double *)malloc(arraysize * sizeof(double)))==NULL) {      //  Temporary array for calculation of expanded times set (mode 0)
            sprintf(errstr,"INSUFFICIENT MEMORY for calculation of expanded times set.\n");
            return(MEMORY_ERROR);
        }
    }
    dz->maxframecnt = (int)ceil(dz->maxstep/dz->frametime) + SAFETY;                    //  Arrays to store frame-by-frame calculation of formant frqs and brightness

    if((dz->parray[6] = (double *)malloc(dz->maxframecnt * sizeof(double)))==NULL) {            //  Array for window-by-window Formant1 data
        sprintf(errstr,"INSUFFICIENT MEMORY Array for window-by-window Formant1 data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[7] = (double *)malloc(dz->maxframecnt * sizeof(double)))==NULL) {            //  Array for window-by-window Formant2 data
        sprintf(errstr,"INSUFFICIENT MEMORY Array for window-by-window Formant2 data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[8] = (double *)malloc(dz->maxframecnt * sizeof(double)))==NULL) {            //  Array for window-by-window Formant3 data
        sprintf(errstr,"INSUFFICIENT MEMORY Array for window-by-window Formant3 data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[9] = (double *)malloc(dz->maxframecnt * sizeof(double)))==NULL) {            //  Array for window-by-window Brightness data
        sprintf(errstr,"INSUFFICIENT MEMORY Array for window-by-window Brightness data.\n");
        return(MEMORY_ERROR);
    }


    dz->itemcnt = cnt;
    return(FINISHED);
}

/************************************ READ_FEATURE_DATA ************************************/

int read_feature_data(char *filename,dataptr dz)
{
    double *time, lasttime, *f1, *f2, *f3, *brightness, dummy, timestep, srate = (double)dz->infile->srate;
    int *sttend, splicelen = dz->iparam[0];
    int cnt, n, m, truecnt = 0, warned = 0;
    char temp[20000], *q;
//    aplptr ap;
    FILE *fp;

//    ap = dz->application;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr, "Can't open file %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    cnt = 0;
    while(fgets(temp,20000,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,&dummy)) {
            cnt++;
        }
    }       
    if(cnt == 0) {
        sprintf(errstr,"No data in file %s\n",filename);
        return(DATA_ERROR);
    }
    if((dz->parray = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s. (1)\n",filename);
        return(MEMORY_ERROR);
    }
    if((dz->lparray = (int **)malloc(dz->larray_cnt * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for data in file %s. (2)\n",filename);
        return(MEMORY_ERROR);
    }
    if((dz->iparray = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY  for data in file %s. (3)\n",filename);
        return(MEMORY_ERROR);
    }
    dz->itemcnt = cnt/5;
    if((dz->parray[0] = (double *)malloc((dz->itemcnt + 1) * sizeof(double)))==NULL) {      //  Starttimes + total-dur
        sprintf(errstr,"INSUFFICIENT MEMORY for time data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[1] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {            //  Formant1 frq
        sprintf(errstr,"INSUFFICIENT MEMORY for formant1 frq data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[2] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {            //  Formant2 frq
        sprintf(errstr,"INSUFFICIENT MEMORY for formant2 frq data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[3] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {            //  Formant3 frq
        sprintf(errstr,"INSUFFICIENT MEMORY for formant3 frq data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[4] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {            //  Brightness
        sprintf(errstr,"INSUFFICIENT MEMORY for brightness data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[0] = (int *)malloc((dz->itemcnt + 1) * 2 * sizeof(int)))==NULL) {       //  Start and end edit-times, in samples
        sprintf(errstr,"INSUFFICIENT MEMORY for start and end sampletimes of segments\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[0] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {                 //  Sorting array
        sprintf(errstr,"INSUFFICIENT MEMORY for Sorting array.\n");
        return(MEMORY_ERROR);
    }
    time = dz->parray[0];
    f1   = dz->parray[1];
    f2   = dz->parray[2];
    f3   = dz->parray[3];
    brightness = dz->parray[4];
    sttend = dz->lparray[0];
    rewind(fp);
    lasttime = -1.0;
    cnt = 0;
    dz->minstep = HUGE;
    dz->maxstep = 0.0;
    while(fgets(temp,20000,fp)==temp) {
        q = temp;
        if(*q == ';')   //  Allow comments in file
            continue;
        while(get_float_from_within_string(&q,&dummy)) {
            if(cnt < dz->itemcnt) {
                if(dummy < 0.0 || dummy <= lasttime) {
                    sprintf(errstr,"Times (%lf and %lf) do not advance correctly in file %s.\n",lasttime,dummy,filename);
                    return(DATA_ERROR);
                }
                if(dummy >= dz->duration) {
                    if(!warned) {
                        fprintf(stdout,"WARNING: Times at or beyond end of sndfile (%lf) in file %s. Ignoring them.\n",dz->duration,filename);
                        fflush(stdout);
                        truecnt = cnt;
                        warned = 1;
                    }
                } else {
                    *time = dummy;
                    if(cnt > 0) {
                        timestep = *time - lasttime;
                        if(timestep < dz->minstep)
                            dz->minstep = timestep;
                        if(timestep > dz->maxstep)
                            dz->maxstep = timestep;
                        lasttime = *time;
                    }
                    time++;
                }
                cnt++;
                if(cnt == dz->itemcnt) {                //  Store segment starttimes, +total file duration
                    *time = dz->duration;
                    timestep = *time - lasttime;
                    if(timestep < dz->minstep)
                        dz->minstep = timestep;
                    if(timestep > dz->maxstep)
                        dz->maxstep = timestep;
                    if(!truecnt)
                        truecnt = dz->itemcnt;
                }
            } else if(cnt < dz->itemcnt * 2) {
                if(dummy < 0.0) {
                    sprintf(errstr,"Invalid formant1 data (%lf) in file %s.\n",dummy,filename);
                    return(DATA_ERROR);
                }
                if(cnt < dz->itemcnt + truecnt) {
                    *f1 = dummy;
                    f1++;
                }
                cnt++;
            } else if(cnt < dz->itemcnt * 3) {
                if(dummy < 0.0) {
                    sprintf(errstr,"Invalid formant2 data (%lf) in file %s.\n",dummy,filename);
                    return(DATA_ERROR);
                }
                if(cnt < (dz->itemcnt * 2) + truecnt) {
                    *f2 = dummy;
                    f2++;
                }
                cnt++;
            } else if(cnt < dz->itemcnt * 4) {
                if(dummy < 0.0) {
                    sprintf(errstr,"Invalid formant3 data (%lf) in file %s.\n",dummy,filename);
                    return(DATA_ERROR);
                }
                if(cnt < (dz->itemcnt * 3) + truecnt) {
                    *f3 = dummy;
                    f1++;
                }
                cnt++;
            } else {
                if(dummy < 0.0 || dummy > 1.0) {
                    sprintf(errstr,"Invalid brightness data (%lf) in file %s.\n",dummy,filename);
                    return(DATA_ERROR);
                }
                if(cnt < (dz->itemcnt * 4) + truecnt) {
                    *brightness = dummy;
                    brightness++;
                }
                cnt++;
            }
        }
    }
    dz->itemcnt = truecnt;
    time  = dz->parray[0];
    sttend = dz->lparray[0];
    for(n=0,m=0;n < dz->itemcnt;n++,m+=2) {
        sttend[m] = (int)round(time[n] * srate);                //  starttime of segment
        if(flteq(time[n+1],dz->duration))
            sttend[m+1] = dz->insams[0];
        else {                                                  //  endtime of segment
            sttend[m+1] = (int)round(time[n+1] * srate);        //  including splice fade-down time
            sttend[m+1] += splicelen;
            sttend[m+1] = min(sttend[m+1],dz->insams[0]);
        }
    }
    return(FINISHED);
}

/************************************ FTURANAL_PARAM_PREPROCESS ************************************/

int fturanal_param_preprocess(dataptr dz)
{
    int exit_status;
    if((exit_status = ftur_initialise_specenv(dz)) < 0)
        return(exit_status);
    if((exit_status = ftur_set_specenv_frqs(dz)) < 0)
        return(exit_status);
    if((dz->iparray[1] = (int *)malloc(dz->clength * sizeof(int)))==NULL) {     //  Array for Head/Tail flags for labelling segments
        sprintf(errstr,"INSUFFICIENT MEMORY for Median calculations.\n");
        return(MEMORY_ERROR);
    }
    return FINISHED;
}

/************************************ CHECK_FTURANAL_PARAM_VALIDITY_AND_CONSISTENCY ************************************/

int check_fturanal_param_validity_and_consistency(dataptr dz)
{
    double splicelen;
    splicelen = dz->param[0] * MS_TO_SECS;
    if(splicelen * 2.0 >= dz->minstep) {
        sprintf(errstr,"Splices (%lf mS) too int for minimum marked-segment length (%lf).\n",dz->param[0],dz->minstep);
        return(DATA_ERROR);
    }
    dz->iparam[0] = (int)round(splicelen * (double)dz->infile->srate);
    return FINISHED;
}

/************************************ FTURANAL ************************************/

int fturanal(dataptr dz)
{
    int exit_status, n, m, done = 0;
    int framecnt, startindex, samps_read, windows_in_buf, wc;
    double time, firsttime, nexttime, f1, f2, f3, bright;
    double *times = dz->parray[0];
    double *fmnt1 = dz->parray[1];
    double *fmnt2 = dz->parray[2];
    double *fmnt3 = dz->parray[3];
    double *brightness = dz->parray[4];
    double *framefmnt1 = dz->parray[5];
    double *framefmnt2 = dz->parray[6];
    double *framefmnt3 = dz->parray[7];
    double *framebrite = dz->parray[8];
    int *ishead = dz->iparray[0], is_head;
    time = 0.0;
    framecnt = 0;
    startindex = 0;
    firsttime = times[startindex];
    nexttime = times[startindex+1];
    is_head = 1;
    for(n=0;n<dz->itemcnt;n++) {                            //  Initially mark Head/Tail sequence
        ishead[n] = is_head;
        is_head = !is_head;
    }
    times[dz->itemcnt] = dz->duration;                      //  Complete times array with the duration of the src
    if(dz->mode == 0) {
        if((exit_status = generate_tail_segments(dz)) < 0)  //  For mode 0, Head/Tail sequence re-marked here, as tail subdivisions added 
            return exit_status;                             //  & dz->itemcnt increased
    }

    is_head = 1;
    while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
        if(samps_read < 0) {
            sprintf(errstr,"Failed to read data from input file.\n");
            return(SYSTEM_ERROR);
        }
        dz->flbufptr[0] = dz->bigfbuf;
        windows_in_buf = samps_read/dz->wanted;    
        for(wc=0; wc<windows_in_buf; wc++, dz->total_windows++) {
            if(dz->total_windows == 0) {
                dz->flbufptr[0] += dz->wanted;      //  Skip first (zeros) window
                continue;
            }
            if(time >= firsttime) {                 //  Skip over any sound before the first timemark in marklist
                if(time >= nexttime) {

                //  If reached next timemarker, get median vals from all frame-values in last segment
                    
                    if((dz->mode == 2 && !is_head) || dz->mode != 2) {      //  In mode2, data is collected only from the tails
                        if((exit_status = get_median_brightness(brightness,framebrite,framecnt,startindex,dz)) < 0)
                            return exit_status;
                        if((exit_status = get_median_formant_frqs(framefmnt1,framefmnt2,framefmnt3,fmnt1,fmnt2,fmnt3,framecnt,startindex,dz)) < 0)
                            return exit_status;
                    }

                //  Reset frames-in-segment counter, and advance aint time-markers
        
                    framecnt = 0;
                    startindex++;
                    if(startindex >= dz->itemcnt) { //  NB there is an extra time (src duration) at end of times data
                        done = 1;
                        break;
                    }

                //  Find if next segment is Head or Tail, and set next time-marker (ahead of here)
        
                    is_head = ishead[startindex];
                    nexttime = times[startindex+1];
                }

                //  Collect data from each (required) frame (window), and advnace framecount-in-segment

                if((dz->mode == 2 && !is_head) || dz->mode != 2) {      //  In mode2, data is collected only from the tails
                    if(framecnt >= dz->maxframecnt) {
                        sprintf(errstr,"Error in segment-framecnt accounting.\n");
                        return PROGRAM_ERROR;
                    }
                    if((exit_status = extract_brightness(dz->flbufptr[0],&bright,dz)) < 0)
                        return exit_status;
                    framebrite[framecnt] = bright;
                    if((exit_status = ftur_extract_specenv(dz->flbufptr[0],dz)) < 0)
                        return exit_status;
                    if((exit_status = extract_formant_frqs(&f1,&f2,&f3,dz)) < 0)
                        return exit_status;
                    framefmnt1[framecnt] = f1;
                    framefmnt2[framecnt] = f2;
                    framefmnt3[framecnt] = f3;
                    framecnt++;
                }
            }

            //  Advance in read-buffer

            dz->flbufptr[0] += dz->wanted;
            time += dz->frametime;;
        }
        if(done)
            break;
    }
    if(!done) {                                 //  Reached src end without dropping out of loop above
        if((dz->mode == 2 && !is_head) || dz->mode != 2) {      //  In mode2, data is collected only from the tails
            if((exit_status = get_median_brightness(brightness,framebrite,framecnt,startindex,dz)) < 0)
                return exit_status;
            if((exit_status = get_median_formant_frqs(framefmnt1,framefmnt2,framefmnt3,fmnt1,fmnt2,fmnt3,framecnt,startindex,dz)) < 0)
                return exit_status;
        }
    }
    if(dz->mode == 2) {                         //  In mode 2, join together H+T segs as single items
        for(m=1,n=2;n<dz->itemcnt;m++,n+=2)     //                        itemcnt                             itemcnt   
            times[m] = times[n];                //  H T     H T     H T     END     ~OR~    H T     H T      H END
        dz->itemcnt = m;                        //  h       h       h                       h       h        h
    }                                           
    for(n = 0; n < dz->itemcnt;n++) {
        if(fprintf(dz->fp,"%f ",times[n])<1) {
            sprintf(errstr,"Problem writing Segment starttime %d to output file\n",n+1);
            return(PROGRAM_ERROR);
        }
    }
    fprintf(dz->fp,"\n");
    for(n = 0; n < dz->itemcnt;n++) {
        if(fprintf(dz->fp,"%f ",fmnt1[n])<1) {
            sprintf(errstr,"Problem writing Formant1 data %d to output file\n",n+1);
            return(PROGRAM_ERROR);
        }
    }
    fprintf(dz->fp,"\n");
    for(n = 0; n < dz->itemcnt;n++) {
        if(fprintf(dz->fp,"%f ",fmnt2[n])<1) {
            sprintf(errstr,"Problem writing Formant2 data %d to output file\n",n+1);
            return(PROGRAM_ERROR);
        }
    }
    fprintf(dz->fp,"\n");
    for(n = 0; n < dz->itemcnt;n++) {
        if(fprintf(dz->fp,"%f ",fmnt3[n])<1) {
            sprintf(errstr,"Problem writing Formant3 data %d to output file\n",n+1);
            return(PROGRAM_ERROR);
        }
    }
    fprintf(dz->fp,"\n");
    for(n = 0; n < dz->itemcnt;n++) {
        if(fprintf(dz->fp,"%f ",brightness[n])<1) {
            sprintf(errstr,"Problem writing Brightness data %d to output file\n",n+1);
            return(PROGRAM_ERROR);
        }
    }
    fprintf(dz->fp,"\n");
    return FINISHED;
}

/************************************ FTURSYNTH ************************************/

int ftursynth (dataptr dz)
{
    int increases = 0, temp, exit_status;
    int n, m, k, j, z, obufpos, stt, sttsamp, endsamp, sampcnt1, sampcnt2;
    double valn, valm;
    double *f1 = dz->parray[1];
    double *f2 = dz->parray[2];
    double *f3 = dz->parray[3];
    double *br = dz->parray[4];
    double *data = NULL;                    //  Points to data-set to sort on
    int *order = dz->iparray[0];            //  Holds reordering sequence
    int *sttend = dz->lparray[0];
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *bufa = dz->sampbuf[2], *bufb = dz->sampbuf[3], *buf1, *buf2, *tempbuf;
    int splicelen = dz->iparam[0];

    if(dz->mode == 8 || dz->mode == 9) {    //  Sum the formant frqs
        for(n=0;n<dz->itemcnt;n++)
            br[n] = f1[n] + f2[n] + f3[n];
    }
    switch(dz->mode) {
    case(0): /* fall thro */
    case(1): data = f1; break;  
    case(2): /* fall thro */
    case(3): data = f2; break;  
    case(4): /* fall thro */
    case(5): data = f3; break;  
    case(6): /* fall thro */
    case(7): /* fall thro */
    case(8): /* fall thro */
    case(9): /* fall thro */    
        data = br;  break;  
    }
    if(EVEN(dz->mode))
        increases = 1;
    for(n=0;n<dz->itemcnt;n++)
        order[n] = n;
    for(n=0;n < dz->itemcnt-1;n++) {        //  Do the data/order sort
        valn = data[n];
        for(m=n+1;m < dz->itemcnt;m++) {
            valm = data[m];                 //  If vals out of order
            if((increases && valn > valm) || (!increases && valn < valm)) {
                data[n] = valm;             //  Swap vals in array
                data[m] = valn; 
                valn = data[n];             //  Reset valn to its new value

                temp = order[n];            //  Swap order of items
                order[n] = order[m];
                order[m] = temp;
            }
        }
    }
    buf1 = bufa;
    buf2 = bufb;
    obufpos = 0;

    //  Copy initial segment into buffer 1

    stt = order[0] * 2;                 //  Start and endtimes of segs are in double-length int-array
    sttsamp = sttend[stt];
    endsamp = sttend[stt+1];
    sampcnt1 = endsamp - sttsamp;
    sndseekEx(dz->ifd[0],sttsamp,0);
    if((exit_status = read_samps(ibuf,dz))<0)
        return(exit_status);
    memcpy((char *)buf1,(char *)ibuf,sampcnt1 * sizeof(float));     //  Copy src chunk to buf1
    for(k = 0; k < splicelen; k++)
        buf1[k] = (float)(buf1[k] * ((double)k/(double)splicelen)); //  DO start and end splices
    for(k = 0, j = sampcnt1-1; k < splicelen; k++,j--)
        buf1[j] = (float)(buf1[j] * ((double)k/(double)splicelen));

    //  Write the upsplice portion of 1st segment to obuf
    
    for(m=0; m < splicelen; m++) {
        obuf[obufpos++] = buf1[m];
        //PROBABLY REDUNDANT
        if(obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;
        }
    }
    //  Copy next segment into buffer 2

    for(n=1;n < dz->itemcnt;n++) {
        stt = order[n] * 2;
        sttsamp = sttend[stt];
        endsamp = sttend[stt+1];
        sampcnt2 = endsamp - sttsamp;
        sndseekEx(dz->ifd[0],sttsamp,0);
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        memcpy((char *)buf2,(char *)ibuf,sampcnt2 * sizeof(float));
        for(k = 0; k < splicelen; k++)
            buf2[k] = (float)(buf2[k] * ((double)k/(double)splicelen));
        for(k = 0, j = sampcnt2-1; k < splicelen; k++,j--)
            buf2[j] = (float)(buf2[j] * ((double)k/(double)splicelen));

        //  Write buf1 apart from the splice-brackets, to obuf

        for(m=splicelen; m < sampcnt1 - splicelen;m++) {
            obuf[obufpos++] = buf1[m];
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }

        //  Write the cross-splice

        for(z = 0;z < splicelen; z++,m++) {
            obuf[obufpos++] = (float)(buf1[m] + buf2[z]);
            if(obufpos >= dz->buflen) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufpos = 0;
            }
        }

        //  Swap the buffers, and buffer-data

        tempbuf = buf1;
        buf1    = buf2;
        buf2    = tempbuf;
        sampcnt1 = sampcnt2;
    }

    //  Write all of the remaining segment

    for(m=splicelen; m < sampcnt1; m++) {
        obuf[obufpos++] = buf1[m];
        if(obufpos >= dz->buflen) {
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
            obufpos = 0;
        }
    }

    //  Flush the output buffer

    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }   
    return FINISHED;
}

/************************************ GET_MEDIAN_BRIGHTNESS ************************************/

int get_median_brightness(double *brightness,double *framebrite,int framecnt,int startindex,dataptr dz)
{
    int n, m, maxmedian, maxpos;
    double lolim, hilim;
    int *medianbin = dz->iparray[1];
    memset((char *)medianbin,0,dz->clength * sizeof(int));
    for(n=0;n<framecnt;n++) {
        if(framebrite[n] <= 0.0)                //  Ignore zero values
            continue;
        lolim = 0;
        for(m=0;m<dz->clength;m++) {
            hilim = (double)(m+1)/(double)dz->clength;
            if(framebrite[n] >= lolim && framebrite[n] < hilim) {
                medianbin[m]++;
                break;
            }
            lolim = hilim;
        }
    }
    maxmedian = medianbin[0];
    maxpos = 0;
    for(m=1;m<dz->clength;m++) {
        if(medianbin[m] >= maxmedian) {         //  IF there are competing values (same no of occurences, this algo takes the highest brightness)
            maxmedian = medianbin[m];
            maxpos = m;
        }
    }
    brightness[startindex] = (double)maxpos/(double)dz->clength;
    return FINISHED;
}

/************************************ GET_MEDIAN_FORMANT_FRQS ************************************/

int get_median_formant_frqs
(double *framefmnt1,double *framefmnt2,double *framefmnt3,double *fmnt1,double *fmnt2,double *fmnt3,int framecnt,int startindex, dataptr dz)
{
    int n = 0, m, maxmedian, maxpos;
    int *medianbin = dz->iparray[1];
    memset((char *)medianbin,0,dz->specenvcnt * sizeof(int));
    for(n=0;n<framecnt;n++) {
        if(framefmnt1[n] <= 0.0)                //  Ignore zero values
            continue;
        for(m=0;m<dz->specenvcnt;m++) {
            if(flteq(framefmnt1[n],dz->specenvfrq[m])) {
                medianbin[m]++;
                break;
            }
        }
    }
    maxmedian = medianbin[0];
    maxpos = 0;
    for(m=1;m<dz->specenvcnt;m++) {
        if(medianbin[m] > maxmedian) {          //  IF there are competing values (same no of occurences, this algo takes the lower frq)
            maxmedian = medianbin[m];
            maxpos = m;
        }
    }
    fmnt1[startindex] = dz->specenvfrq[maxpos];

    memset((char *)medianbin,0,dz->specenvcnt * sizeof(int));
    for(n=0;n<framecnt;n++) {
        if(framefmnt2[n] <= 0.0)                //  Ignore zero values
            continue;
        for(m=0;m<dz->specenvcnt;m++) {
            if(flteq(framefmnt2[n],dz->specenvfrq[m])) {
                medianbin[m]++;
                break;
            }
        }
    }
    maxmedian = medianbin[0];
    maxpos = 0;
    for(m=1;m<dz->specenvcnt;m++) {
        if(medianbin[m] > maxmedian) {          //  IF there are competing values (same no of occurences, this algo takes the lower frq)
            maxmedian = medianbin[m];
            maxpos = m;
        }
    }
    fmnt2[startindex] = dz->specenvfrq[maxpos];
    memset((char *)medianbin,0,dz->specenvcnt * sizeof(int));
    for(n=0;n<framecnt;n++) {
        if(framefmnt3[n] <= 0.0)                //  Ignore zero values
            continue;
        for(m=0;m<dz->specenvcnt;m++) {
            if(flteq(framefmnt3[n],dz->specenvfrq[m])) {
                medianbin[m]++;
                break;
            }
        }
    }
    maxmedian = medianbin[0];
    maxpos = 0;
    for(m=1;m<dz->specenvcnt;m++) {
        if(medianbin[m] > maxmedian) {          //  IF there are competing values (same no of occurences, this algo takes the lower frq)
            maxmedian = medianbin[m];
            maxpos = m;
        }
    }
    fmnt3[startindex] = dz->specenvfrq[maxpos];
    return FINISHED;
}

/************************************ EXTRACT_BRIGHTNESS ************************************/

int extract_brightness(float *buf,double *bright,dataptr dz)
{
    int gotit = 0, cc, vc, hichan, lochan;
    double totalamp = 0.0, halftotal, ampsum, lastampsum = 0.0, ampchange, ampoffset, frq = 0.0, frqoffset;
    for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2)
        totalamp += buf[AMPP];
    halftotal = totalamp/2.0;
    ampsum = 0.0;
    for(cc=0,vc=0;cc < dz->clength;cc++,vc+=2) {
        ampsum += buf[AMPP];
        if(ampsum > halftotal) {
            hichan = cc;
            if(hichan == 0)
                break;
            lochan = cc-1;
            frq = lochan * dz->chwidth;
            gotit = 1;
            break;
        }
        lastampsum = ampsum;
    }
    if(gotit) {
        ampchange = buf[AMPP];
        ampoffset = (halftotal - lastampsum)/ampchange;
        frqoffset = dz->chwidth * ampoffset;
        frq += frqoffset;
        *bright = frq/dz->nyquist;
    } else
        *bright = 0.0;
    return FINISHED;
}

/************************************ EXTRACT_FORMANT_FRQS ************************************/

int extract_formant_frqs(double *f1,double *f2,double *f3,dataptr dz)

{
    int formantcnt = 0, cc, gotpeak = 0;
    double fmnt[3], thisamp;
    thisamp = dz->specenvamp[1];                //  We ignore the lowest frq band
    for(cc = 0; cc < 3; cc++)                   //  Intialise fmnt frqs to zero
        fmnt[cc] = 0.0;
    cc = 2;
    while(cc < dz->specenvcnt) {
        if(dz->specenvamp[cc] < thisamp) {      //  Search for initial fall in envelope
            if(!gotpeak) {                      //  If envelope continues to fall, ignore
                fmnt[formantcnt] = dz->specenvfrq[cc-1];
                gotpeak = 1;
                formantcnt++;
                if(formantcnt == 3)
                    break;
            }
        } else if(dz->specenvamp[cc] > thisamp) //  If envelope starts to rise                                  
            gotpeak = 0;                        //  reset peaksearch flag       
        thisamp = dz->specenvamp[cc];
        cc++;
    }
    *f1 = fmnt[0];
    *f2 = fmnt[1];
    *f3 = fmnt[2];
    return FINISHED;
}

/************************************ GENERATE_TAIL_SEGMENTS ************************************
 *
 *  Slice tail sections into segments approx same size as bracketing head segments
 */

int generate_tail_segments(dataptr dz)
{
    int n, j, k, tailsegcnt;
    double *time   = dz->parray[0];
    double *nutime = dz->parray[9];
    int *ishead   = dz->iparray[0];
    double stthsize, endhsize, meanhsize, tailsize, randoffset;
    ishead[0] = 1;
    ishead[1] = 0;
    nutime[0] = time[0];                                    //  H T           H T           H T     
    nutime[1] = time[1];                                    // n0 n1         n2 n3         n4 n5
    j = 2;                                                  //  |-|-----------|-|-----------|-|---------
    stthsize = time[1] - time[0];                           //      tailsize
    for(n=2;n < dz->itemcnt; n+=2) { // Step to next HEAD   //stthsize      endhsize    
        tailsize = time[n] - time[n-1];                     //
        endhsize = time[n+1] - time[n];                     
        meanhsize = (stthsize + endhsize)/2.0;              // New times(j), at n = 2, if tailsegcnt = 4
        tailsegcnt = (int)round(tailsize/meanhsize);        // n0 n1         n2 n3         n4 n5
        meanhsize = tailsize/(double)tailsegcnt;            //  |-|-----------|-|-----------|-|---------
        for(k=1;k < tailsegcnt;k++) {                       // j0 j1 |  |  |        
            nutime[j] = time[n-1] + (meanhsize * k);        //       j2 j3 j4 (3 new times inserted)
            if(dz->param[FTURRAND] > 0.0) {                 //  H T  T  T  T         
                randoffset = ((drand48() * 2.0) - 1.0)/3.0; //      <-><-><->
                randoffset *= dz->param[FTURRAND];          //      j2   j3j4
                randoffset *= meanhsize;                    //  Rand in possible range max of (-1/3 to + 1/3 * meanhsize)
                nutime[j] += randoffset;                    //      
            }                                               //
            ishead[j] = 0;                                  //  
            j++;                                            //  Ending at j5 = n2
        }                                                   // n0 n1         n2 n3         n4 n5
        nutime[j] = time[n];                                //  |-|-----------|-|-----------|-|---------
        ishead[j++] = 1;                                    // j0 j1 j2 j3 j4 | |
        nutime[j] = time[n+1];                              //               j5 j6
        ishead[j++] = 0;                                    //  H T  T  T  T  H T
        stthsize = endhsize;
    }
    if(n == dz->itemcnt) {                                  //  IF end of file is end of a tail
        tailsize = time[n] - time[n-1];                     //  H T     H T         End
        meanhsize = stthsize;                               //  | |     | |          |
        tailsegcnt = (int)round(tailsize/meanhsize);        //           n-1         n  
        meanhsize = tailsize/(double)tailsegcnt;            //         endhsize      |
        for(k=1;k < tailsegcnt;k++) {                       //        j22 j23  |  |  | 
            nutime[j] = time[n-1] + (meanhsize * k);        //               j24 j25 |
            if(dz->param[FTURRAND] > 0.0) {                 //               <-> <-> |  
                randoffset = ((drand48() * 2.0) - 1.0)/3.0; //                       j26
                randoffset *= dz->param[FTURRAND];          //  ....... H  T  T  T   T
                randoffset *= meanhsize;                    
                nutime[j] += randoffset;
            }                                               
            ishead[j] = 0;
            j++;
        }
        nutime[j] = time[n];
    }
    dz->itemcnt = j;
    memcpy((char *)time,(char *)nutime,(dz->itemcnt+1) * sizeof(double));
    return FINISHED;
}

/**************************** CREATE_FTURANAL_SNDBUFS ****************************/

int create_fturanal_sndbufs(dataptr dz)
{
    int n;
    unsigned int buffersize = 0;
    switch(dz->process) {
    case(FTURANAL):
        dz->bufcnt = 1;
        if((dz->flbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
            return(MEMORY_ERROR);
        }
        buffersize = dz->wanted * BUF_MULTIPLIER;
        dz->buflen = buffersize;
        if((dz->bigfbuf = (float*) malloc(buffersize * sizeof(float)))==NULL) {  
            sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
            return(MEMORY_ERROR);
        }
        dz->big_fsize = buffersize;
        break;
    case(FTURSYNTH):
        dz->bufcnt = 4;
        if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
            return(MEMORY_ERROR);
        }
        if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
            return(MEMORY_ERROR);
        }
        buffersize = (unsigned int)round((dz->maxstep * 2) * (double)dz->infile->srate);
        buffersize = (buffersize/F_SECSIZE) * F_SECSIZE;
        dz->buflen = buffersize;
        if((dz->buflen  < 0) || (dz->buflen * 4 < 0)) {  
            sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
            return(MEMORY_ERROR);
        }
        if((dz->bigfbuf = (float*) malloc(dz->buflen * 4 * sizeof(float)))==NULL) {  
            sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
            return(MEMORY_ERROR);
        }
        dz->sbufptr[0] = dz->sampbuf[0] = dz->bigfbuf;
        for(n = 1;n < dz->bufcnt;n++)
            dz->sbufptr[n] = dz->sampbuf[n] = dz->bigfbuf + (n * dz->buflen);
        dz->sampbuf[n] = dz->bigfbuf + (n * dz->buflen);
        break;
    }
    return(FINISHED);
}

