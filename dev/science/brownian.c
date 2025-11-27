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
/*  SYNTHESIS FROM RANDOM WALK THROUGH PITCH AND REAL SPACE
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

#define evsamps     total_windows

#define BRPQ    0.125       //  pitch quantisation to 1/16-tones
#define BRTQ    0.010       //  time quantisation to 10mS = 1/100th sec
#define BRSQ    0.03125     //  space quantisation: number of spatial steps between lspkrs = 32
#define BRAQ    0.5         //  amp step quantisation = 1/2 dB
#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int check_brownian_param_validity_and_consistency(dataptr dz);
static int setup_brownian_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_brownian_param_ranges_and_defaults(dataptr dz);
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
static void pancalc(double position,double *leftgain,double *rightgain);
static int write_event_to_output(int passno,double current_time,double current_position,double *maxsamp,double normaliser,int *obufpos,dataptr dz);
static int write_event(double current_pitch,double current_gain,double tabincr,dataptr dz);
static int do_brownian(dataptr dz);
static int get_gain(double *current_gain,dataptr dz);
static int get_position(double *space_position,dataptr dz);
static double get_timestep(dataptr dz);
static int get_next_pitch(double *currentpitch,double thistime,dataptr dz);
static int create_brownian_buffers(dataptr dz);
static void time_display(int samps_sent,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
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
        dz->maxmode = 2;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_brownian_application(dz))<0) {
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
//  ap = dz->application;

    // parse_infile_and_hone_type() = 
    if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_brownian_param_ranges_and_defaults(dz))<0) {
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
    if((exit_status = check_brownian_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = open_the_outfile(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    if((exit_status = create_brownian_buffers(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess()    redundant
    //process_file =
    if((exit_status = do_brownian(dz))<0) {
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
    int has_extension = 0;
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
    p = filename + strlen(filename);
    p--;
    while(p != filename) {
        if(*p == '.') {
            has_extension = 1;
            break;
        }
        p--;
    }
    strcpy(dz->outfilename,filename);
    if(!has_extension)
        strcat(dz->outfilename,".wav");
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
    int exit_status;
    dz->infile->channels = dz->iparam[BRCHANS];
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

/************************* SETUP_BROWNIAN_APPLICATION *******************/

int setup_brownian_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if(dz->mode == 0)
        exit_status = set_param_data(ap,0,12,12,"idDDDDddDDDi");
    else
        exit_status = set_param_data(ap,0,12,10,"id00DDddDDDi");
    if(exit_status < 0)
        return(FAILED);
    if(dz->mode == 0)
        exit_status = set_vflgs(ap,"amsd",4,"DDDD","l",1,0,"0");
    else
        exit_status = set_vflgs(ap,"am",2,"DD","l",1,0,"0");
    if(exit_status < 0)
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

/************************* SETUP_BROWNIAN_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_brownian_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()

    ap->lo[BRCHANS] = 1;
    ap->hi[BRCHANS] = 16;
    ap->default_val[BRCHANS] = 8;
    ap->lo[BRDUR] = dz->duration;
    ap->hi[BRDUR] = 7200;
    ap->default_val[BRDUR] = 20;
    if(dz->mode == 0) {
        ap->lo[BRATT] = .002;
        ap->hi[BRATT] = 8;
        ap->default_val[BRATT] = .02;
        ap->lo[BRDEC]   = .002;
        ap->hi[BRDEC]   = 8;
        ap->default_val[BRDEC] = .5;
    }
    ap->lo[BRPLO]   = 0;
    ap->hi[BRPLO]   = 127;
    ap->default_val[BRPLO] = 48;
    ap->lo[BRPHI]   = 0;
    ap->hi[BRPHI]   = 127;
    ap->default_val[BRPHI] = 72;
    ap->lo[BRPSTT]  = 0;
    ap->hi[BRPSTT]  = 127;
    ap->default_val[BRPSTT] = 60;
    ap->lo[BRSSTT]  = 1;
    ap->hi[BRSSTT]  = 16;
    ap->default_val[BRSSTT] = 1;
    ap->lo[BRPSTEP] = 0.125;
    ap->hi[BRPSTEP] = 24;
    ap->default_val[BRPSTEP] = .5;
    ap->lo[BRSSTEP] = 0;
    ap->hi[BRSSTEP] = 1;
    ap->default_val[BRSSTEP] = .0625;
    ap->lo[BRTICK]  = 0.002;
    ap->hi[BRTICK]  = 4;
    ap->default_val[BRTICK] = 0.04;
    ap->lo[BRSEED]  = 0;
    ap->hi[BRSEED]  = 255;
    ap->default_val[BRSEED] = 1;
    ap->lo[BRASTEP] = 0;
    ap->hi[BRASTEP] = 96;
    ap->default_val[BRASTEP] = 0;
    ap->lo[BRAMIN]  = 0;
    ap->hi[BRAMIN]  = 96;
    ap->default_val[BRAMIN] = 0;
    if(dz->mode == 0) {
        ap->lo[BRASLP]  = 0.1;
        ap->hi[BRASLP]  = 10;
        ap->default_val[BRASLP] = 1;
        ap->lo[BRDSLP]  = 0.1;
        ap->hi[BRDSLP]  = 10;
        ap->default_val[BRDSLP] = 1;
    }
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
//  aplptr ap;

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
            if((exit_status = setup_brownian_application(dz))<0)
                return(exit_status);
//          ap = dz->application;
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
    usage2("motion");
    return(USAGE_ONLY);
}

/**************************** CHECK_BROWNIAN_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_brownian_param_validity_and_consistency(dataptr dz)
{
    int exit_status, check = 0, error = 0;
    int n, m;

    //  Check that initial pitch is within specified range

    if(dz->brksize[BRPHI])
        dz->param[BRPHI] = dz->brk[BRPHI][1];
    if(dz->brksize[BRPLO])
        dz->param[BRPLO] = dz->brk[BRPLO][1];
    if(dz->param[BRPSTT] > dz->param[BRPHI] || dz->param[BRPSTT] < dz->param[BRPLO]) {
        sprintf(errstr,"START PITCH LIES OUTSIDE PITCH RANGE SPECIFIED (AT PROCESS START)");
        return DATA_ERROR;
    }

    //  Check that (maximum) pitch lies (everwhere) within specified pitch-range

    check = 0;
    error = 0;
    if(dz->brksize[BRPHI]) {
        if(dz->brksize[BRPLO])
            check = 3;      //  Check with both phi and plo 
        else
            check = 1;      //  Check with phi variable 
    } else if(dz->brksize[BRPLO] && !dz->brksize[BRPHI])
        check = 2;          //  Check with plo variable 

    if(dz->brksize[BRPSTEP]) {
        if((exit_status = get_maxvalue_in_brktable(&(dz->param[BRPSTEP]),BRPSTEP,dz))<0)
            return exit_status;
    }
    switch(check) {
    case(0):
        if(dz->param[BRPHI] + dz->param[BRPLO] <= dz->param[BRPSTEP])
            error = 1;
        break;
    case(1):
        for(n=0,m=0;n < dz->brksize[BRPHI];n++,m+=2) {
            if(dz->brk[BRPHI][m+1] + dz->param[BRPLO] <= dz->param[BRPSTEP]) {
                error = 1;
                break;
            }
        }
        break;
    case(2):
        for(n=0,m=0;n < dz->brksize[BRPLO];n++,m+=2) {
            if(dz->param[BRPHI] + dz->brk[BRPLO][m+1] <= dz->param[BRPSTEP]) {
                error = 1;
                break;
            }
        }
        break;
    case(3):
        for(n=0,m=0;n < dz->brksize[BRPHI];n++,m+=2) {
            if((exit_status = read_value_from_brktable(dz->brk[BRPHI][m],BRPLO,dz))<0)
                return(exit_status);
            if(dz->brk[BRPHI][m+1] + dz->param[BRPLO] < dz->param[BRPSTEP]) {
                error = 1;
                break;
            }
        }
        if(!error) {
            for(n=0,m=0;n < dz->brksize[BRPLO];n++,m+=2) {
                if((exit_status = read_value_from_brktable(dz->brk[BRPLO][m],BRPHI,dz))<0)
                    return(exit_status);
                if(dz->param[BRPHI] + dz->brk[BRPLO][m+1] < dz->param[BRPSTEP]) {
                    error = 1;
                    break;
                }
            }
        }
        break;

    }
    if(error) {
        if(dz->brksize[BRPSTEP]) {
            fprintf(stdout,"WARNING: PITCH-STEP MAY BE TOO LARGE FOR MINIMUM PITCHRANGE ENCOUNTERED.\n");
            fflush(stdout);
        } else {
            sprintf(errstr,"PITCH-STEP TOO LARGE FOR MINIMUM PITCHRANGE SPECIFIED.\n");
            return DATA_ERROR;
        }
    }
    if(dz->iparam[BRCHANS] == 1) {
        if(!flteq(dz->param[BRSSTT],1.0)) {
            fprintf(stdout,"WARNING: Start position (%.2lf) ignored for mono output.\n",dz->param[BRSSTT]);
            fflush(stdout);
        }
        if(!flteq(dz->param[BRSSTEP],0.0)) {
            fprintf(stdout,"WARNING: Spatial step (%.2lf) ignored for mono output.\n",dz->param[BRSSTEP]);
            fflush(stdout);
        }
        dz->param[BRSSTT] = 1.0;
    }
    dz->param[BRSSTT] -= 1.0;       //  change initial output-position from range 1toN to range 0toN-1

    if(!dz->vflag[0] && (dz->iparam[BRCHANS] < 3)) {
        fprintf(stdout,"WARNING: Output array must be LINEAR if output-channel count IS LESS THAN 3.\n");
        fflush(stdout);
        dz->vflag[0] = 1;
    }
    if(dz->vflag[0]) {
        if(dz->param[BRSSTT] > dz->iparam[BRCHANS] - 1) {
            sprintf(errstr,"INITIAL POSITION NOT WITHIN THE RANGE OF OUTPUT CHANNELS SPECIFIED, FOR A LINEAR ARRAY.\n");
            return DATA_ERROR;
        }
    } else {
        if(dz->param[BRSSTT] > dz->iparam[BRCHANS]) {
            sprintf(errstr,"INITIAL POSITION NOT WITHIN THE RANGE OF OUTPUT CHANNELS SPECIFIED.\n");
            return DATA_ERROR;
        }
    }
    if(dz->param[BRASTEP] <= 0.0 && dz->param[BRAMIN] > 0.0) {
        dz->param[BRAMIN] = 0.0;
        fprintf(stdout,"WARNING: No amplitude step: amplitude minimum has no effect.\n");
        fflush(stdout);
    }
    return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"motion"))              dz->process = BROWNIAN;
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
    if(!strcmp(str,"motion")) {
        fprintf(stderr,
        "USAGE:\n"
        "brownian motion 1 fi fo chans dur att dec plo phi pstart sstart step sstep tick seed\n"
        "[-aarange] [-mminamp] [-saslope] [-ddslope] [-l]\n"
        "OR\n"
        "brownian motion 2 fi fo chans dur plo phi pstart sstart step sstep tick seed\n"
        "[-aarange] [-mminamp] [-l]\n"
        "\n"
        "Generate texture of sampled elements following brownian motion in pitch and space.\n"
        "\n"
        "FI      (Mono) Source to be read at different speeds to generate output events.\n"
        "        MODE 1: src must start & end at sampval 0.0 : sampled as a waveform.\n"
        "        MODE 2: src can be anything, whole src is transposed for output events..\n"
        "                (In mode 2, very int source may take very long time to finish).\n"
        "FO      Output file.\n"
        "CHANS   Number of channels in output file.\n" 
        "DUR     (Max) duration of output file.\n" 
        "ATT*    Rise time of events (Mode 1 only).\n"
        "DEC*    Decay time of events (Mode 1 only).\n"
        "PLO*    Bottom of pitch range (MIDI).\n"
        "PHI*    Top of pitch range (MIDI).\n"
        "PSTART  Initial pitch (MIDI).\n"
        "SSTART  Initial spatial position (numbering chans 1 - N) (ignored if mono output).\n"
        "STEP*   Maximum pitch step between events.\n"
        "SSTEP*  Max spatial step between events (fraction of distance between channels).\n"
        "TICK*   (Average) Time between events.\n"
        "SEED    Seed (initialises random vals. Gives reproducible random sequence).\n"
        "ARANGE* Max loudness step between events, in dB (default = min = 0: max = 96dB).\n"
        "MINAMP* Min loudness (Range  >0 to 96dB). default = 0 = NO minimum.\n"
        "        (Only comes into play if \"ARANGE\" is > 0).\n"
        "        (If min > 0: if amp falls to -min dB, levels 'bounce' off the min value).\n"
        "        (If min = 0: no min set, & if level falls to -96dB, sounds stream halts).\n"
        "(Mode 1 only)\n"
        "ASLOPE* attack slope: < 1 rise fast then slows  : > 1 rise slow then faster.\n"
        "DSLOPE* decay slope:  < 1 fall slow then faster : > 1 fall fast then slows.\n"
        "        Slope ranges are 0.1 to 10.\n"
        "\n"
        "-l      loudspeakers arrayed in a line. (Default: arrayed in a \"circle\").\n"
        "\n"
        "All items marked with \"*\" can vary though time.\n");
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

/******************************** CREATE_BROWNIAN_BUFFERS *******************************
 *
 *  input buf length = dz->insams[0] + 1 (wraparound point)
 *  event buflen = dz->evbufcnt
 *  obuflen = dz->evbufcnt
 *  ovflwbuf = dz->evbufcnt
 */

#define SAFETY  48

int create_brownian_buffers(dataptr dz) 
{
    int exit_status;
    double srate = (double)dz->infile->srate, maxatt, maxdec, maxtransposition;
    int maxevdur, bigbufsize, real_buflen;
    dz->bufcnt = 4;
    if(dz->mode == 0) {
        if(dz->brksize[BRATT]) {
            if((exit_status = get_maxvalue_in_brktable(&maxatt,BRATT,dz))<0)
                return exit_status;
        } else 
            maxatt = dz->param[BRATT];
        if(dz->brksize[BRDEC]) {
            if((exit_status = get_maxvalue_in_brktable(&maxdec,BRDEC,dz))<0)
                return exit_status;
        } else 
            maxdec = dz->param[BRDEC];
        maxevdur = (int)ceil((maxatt + maxdec) * srate) + SAFETY;
    } else {
        if(dz->brksize[BRPLO]) {
            if((exit_status = get_minvalue_in_brktable(&maxatt,BRPLO,dz))<0)
                return exit_status;
        }
        maxtransposition = dz->param[BRPSTT] - dz->param[BRPLO];                //  Max downward transpos inb semitones
        maxtransposition  = pow(2.0,(maxtransposition * OCTAVES_PER_SEMITONE)); //  Max downward transpos as ratio
        maxevdur = (int)ceil(dz->insams[0] * maxtransposition) + SAFETY;
    }
    dz->buflen = maxevdur * dz->iparam[BRCHANS];
    bigbufsize = (dz->insams[0] + 1) + (3 * dz->buflen);

    if((dz->bigbuf = (float *)malloc(bigbufsize * sizeof(float))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    if((dz->sampbuf = (float **)malloc(dz->bufcnt * sizeof(float *))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(dz->bufcnt * sizeof(float *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    dz->sampbuf[0] = dz->sbufptr[0] = dz->bigbuf;
    dz->sampbuf[1] = dz->sbufptr[1] = dz->sampbuf[0] + dz->insams[0] + 1;
    dz->sampbuf[2] = dz->sbufptr[2] = dz->sampbuf[1] + dz->buflen;
    dz->sampbuf[3] = dz->sbufptr[3] = dz->sampbuf[2] + dz->buflen;
    real_buflen = dz->buflen;
    dz->buflen = dz->insams[0];             //  Read input sound
    if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
        return(exit_status);
    dz->sampbuf[0][dz->buflen] = 0.0f;      //  wraparound point
    dz->buflen  = real_buflen;
    return(FINISHED);
}

/*************************** GET_NEXT_PITCH **************************/

int get_next_pitch(double *currentpitch,double thistime,dataptr dz)
{
    double range, rangepos, randrangebot, randrangetop, randrange, randval;
    int qstep;
    double negval, nextpitch, pstep;

    range = dz->param[BRPHI] - dz->param[BRPLO];        //  total pitch range
    if(range <= dz->param[BRPSTEP]) {
        sprintf(errstr,"RANGE (%.4lf TO %.4lf) TOO NARROW FOR PITCH-STEPS (%.4lf) AT TIME %lf\n",
        dz->param[BRPHI],dz->param[BRPLO],dz->param[BRPSTEP],thistime);
        return DATA_ERROR;
    }
    rangepos = (*currentpitch - dz->param[BRPLO])/range;//  Relative position of current pitch in current range (between 0 and 1)
    if(rangepos <= 0.5) {                               //  Selection range for random numbers is adjusted
        randrangebot = -(2.0 * rangepos);               //  so that probability of moving downwards if near range bottom, is reduced
        randrangetop = 1.0;                             //  and probability of moving upwards if near range top, is reduced.
        randrange = -randrangebot + 1.0;                //  Total adjusted range.
    } else {
        randrangebot = -1.0;
        randrangetop = 2.0 * (1.0 - rangepos);
        randrange = 1.0 + randrangetop;
    }
    randval = drand48();                                //  randval generated
    negval = randval * randrange;                       //  randval used to determine up or down pitch-motion in weighted fashion.
    negval += randrangebot;
    if(negval < 0.0)
        negval = -1.0;
    else
        negval = 1.0;
    pstep = randval * dz->param[BRPSTEP];               //  Generate a random pitchstep (+ve)
    qstep = (int)round(pstep/BRPQ);                     //  Quantise it
    pstep = qstep * BRPQ;
    pstep *= negval;                                    //  Assign (weighted) +ve/-ve assignment
    nextpitch = *currentpitch + pstep;
    if(nextpitch < dz->param[BRPLO] || nextpitch > dz->param[BRPHI]) {
        pstep = -pstep;                                 //  Step reflected off top or bottom of range, if they are crossed
        nextpitch = *currentpitch + pstep;
    }
    *currentpitch = nextpitch;
    return FINISHED;
}

/*************************** GET_TIMESTEP **************************/

double get_timestep(dataptr dz)
{
    int qstep;
    double  tstep  = drand48() * 2.0 * dz->param[BRTICK];   //  Timestep lies between 0 and twice clockrate
    qstep = (int)round(tstep/BRTQ);                         //  Quantise it
    qstep = max(1,qstep);                                   //  Timestep cannot be zero
    tstep = qstep * BRTQ;
    return tstep;
}

/*************************** GET_POSITION **************************/

int get_position(double *space_position,dataptr dz)
{
    int qstep;
    double  sstep  = (drand48() * 2.0) - 1.0;           //  Range -1 to +1
    sstep  *= dz->param[BRSSTEP];                       //  Range -BRSSTEP to +BRSSTEP
    qstep = (int)round(sstep/BRSQ);                     //  Quantise it
    sstep = qstep * BRSQ;
    *space_position += sstep;                           //  Move position
    if(dz->vflag[0]) {
        if(*space_position < 0.0)                       //  Reflect off edges of space, if linear lspkr array   
            *space_position += 2.0 * sstep;
        else if(*space_position >= (dz->iparam[BRCHANS] - 1))
            *space_position -= 2.0 * sstep;
    } else {                                            //  Otherwise wrap-around surround-sound
        if(*space_position < 0.0)
            *space_position += dz->param[BRCHANS];
        else if(*space_position >= dz->param[BRCHANS])
            *space_position -= dz->param[BRCHANS];
    }
    return FINISHED;
}

/*************************** GET_GAIN **************************
 *
 * Once gain goes to zero, stop process.
 */

int get_gain(double *current_gain,dataptr dz)
{
    int qstep;
    double current_dB, orig_dB, astep;
    astep  = (drand48() * 2.0) - 1.0;       //  Range -1 to +1
    astep  *= dz->param[BRASTEP];           //  Range -BRASTEP to +BRASTEP
    qstep = (int)round(astep/BRAQ);         //  Quantise it
    astep = qstep * BRAQ;                       

    current_dB  = 1.0/(*current_gain);      //  Convert  current gain to dB
    current_dB  = log10(current_dB);
    current_dB *= 20.0;
    current_dB  = -current_dB;
    orig_dB = current_dB;
    current_dB += astep;                    //  Incr dB

    if(current_dB >= 0.0)                   //  Avoid gain >= 1.0 (bounce gain downwards)
        current_dB = orig_dB - astep;

    if(dz->param[BRAMIN] <= 0.0) {          //  If no minimum amp set
        if(current_dB <= MIN_DB_ON_16_BIT) { // check if gain has reached minimum
            *current_gain =  0.0;           //  And if so, return gain of zero
            return(FINISHED);               //  (which will halt the process)
        }
    } else {                                //  If minimum amp has been set,
        if(current_dB <= -dz->param[BRAMIN]) {//    if minimum reached
            current_dB = orig_dB - astep;   //  bounce amplitude off minimum value.
            if(current_dB >= 0.0) {         //  If amp then hits maximum (narrow amp range relative to amp jumps)
                *current_gain =  1.0;       //  set amp to full and return      
                return(FINISHED);
            }
        }
    }
    current_dB  = -current_dB;              //  Convert dB to gain, and return
    current_dB /= 20.0;
    current_dB  = pow(10.0,current_dB);
    *current_gain  = 1.0/current_dB;
    return(FINISHED);
}

/************************************ WRITE_EVENT ***********************************/

int do_brownian(dataptr dz) 
{
    int exit_status, passno;
    double tabincr, normaliser = 1.0, maxsamp = 0.0, current_time, current_pitch, current_gain, srate = (double)dz->infile->srate;
    double current_position;
    float *obuf = dz->sampbuf[2];
    int obufpos, n;
    tabincr = (double)dz->insams[0]/srate; //   tabincr to read table once per second, i.e. at 1Hz
    for(passno=0;passno<2;passno++) {
        display_virtual_time(0,dz);
        current_position = dz->param[BRSSTT];
        if(passno == 0) {
            fprintf(stdout,"INFO: Assessing output level.\n");
            fflush(stdout);
        } else {
            fprintf(stdout,"\nINFO: Generating output sound.\n");
            fflush(stdout);
        }
        srand((int)dz->iparam[BRSEED]); //  (Re)initialise random-number generator.
        current_time = 0;
        memset((char *)obuf,0,dz->buflen * 2 * sizeof(float));  //  Initialise outbuf AND overflow buf to 0
        dz->total_samps_written = 0;
        current_pitch = dz->param[BRPSTT];
        current_gain = 1.0;
        obufpos = 0;
        while(current_time < dz->param[BRDUR]) {
            if((exit_status = read_values_from_all_existing_brktables(current_time,dz))<0)
                return exit_status;
            if((exit_status = write_event(current_pitch,current_gain,tabincr,dz))<0)
                return exit_status;
            if((exit_status = write_event_to_output(passno,current_time,current_position,&maxsamp,normaliser,&obufpos,dz))<0)
                return exit_status;
            if((exit_status = get_next_pitch(&current_pitch,current_time,dz))<0)
                return exit_status;
            if(dz->param[BRASTEP] > 0.0) {
                get_gain(&current_gain,dz);
                if(current_gain <= 0.0) {
                    if(passno == 0) {
                        fprintf(stdout,"INFO: Process fades to zero at %.2lf secs\n",current_time);
                        fflush(stdout);
                    }
                    break;
                }
            }
            if(dz->iparam[BRCHANS] > MONO) {
                if((exit_status = get_position(&current_position,dz))<0)
                    return exit_status;
            }
            current_time += get_timestep(dz);
        }
        if(passno == 0) {
            if(dz->total_samps_written == 0) {      //  If no output has been written (and therefore no maximum assessed)
                for(n = 0;n < obufpos;n++) {        //  calculate the maximum sample NOW
                    if(fabs(obuf[n]) > maxsamp)
                        maxsamp = fabs(obuf[n]);
                }
            }
            normaliser = 0.95/maxsamp;
        } else {
            if(obufpos > 0) {       //  Write any remaining samples in output buffer
                for(n=0;n < obufpos;n++)
                    obuf[n] = (float)(obuf[n] * normaliser);
                if((exit_status = write_samps(obuf,obufpos,dz))<0)
                    return(exit_status);
            }
        }
    }
    return FINISHED;
}

/************************************ WRITE_EVENT ***********************************
 *
 * Writes a specifically pitched event, at specified level, into the event buffer.
 */

int write_event(double current_pitch,double current_gain,double tabincr,dataptr dz)
{
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1];
    double tabpos = 0.0, frac, diff, thisval, env, frq, srate = (double)dz->infile->srate;
    int thispos, nextpos, n, m, tabsize = dz->insams[0];
    memset((char *)obuf,0,dz->buflen * sizeof(float));
    dz->tempsize = (int)round(dz->param[BRDUR] * srate) * dz->iparam[BRCHANS];
    if(dz->mode == 0) {
        dz->iparam[BRATT] = (int)round(dz->param[BRATT] * srate);
        dz->iparam[BRDEC] = (int)round(dz->param[BRDEC] * srate);
        dz->evsamps = dz->iparam[BRATT] + dz->iparam[BRDEC];
        frq = miditohz(current_pitch);
        tabincr *= frq;                                             //  Frq-related table-read increment
        for(n = 0,m = -dz->iparam[BRATT]; n< dz->evsamps;n++,m++) { //  m gets to zero at end of attack = start of decay
            thispos = (int)floor(tabpos);                           //  Read input sample by interpolation
            nextpos = thispos+1;                                    //  with incr determined by pitch/frq
            frac = tabpos - thispos;
            diff =  ibuf[nextpos] - ibuf[thispos];
            diff *= frac;
            thisval = ibuf[thispos] + diff;
            if(n < dz->iparam[BRATT]) {                             //  Do enveloping on the fly
                env = (double)n/(double)dz->iparam[BRATT];
                env = pow(env,dz->param[BRASLP]);
            } else {
                env = 1.0 - ((double)m/(double)dz->iparam[BRDEC]);
                env = max(env,0.0);
                env = pow(env,dz->param[BRDSLP]);
            }
            env *= current_gain;                                    //  Scale envelope by current loudness
            thisval *= env;
            obuf[n] = (float)thisval;
            tabpos += tabincr;                                      //  Advance pointer-read, wrapping around at table end
            if(tabpos >= tabsize)
                    tabpos -= tabsize;
        }
    } else {
        tabincr = current_pitch - dz->param[BRPSTT];                //  semitone transposition
        tabincr = pow(2.0,(tabincr * OCTAVES_PER_SEMITONE));        //  frqratio transposition
        dz->evsamps = 0;
        while(tabpos < dz->insams[0]) {
            thispos = (int)floor(tabpos);                           //  Read input sample by interpolation
            nextpos = thispos+1;                                    //  with incr determined by pitch/frq
            frac = tabpos - thispos;
            diff =  ibuf[nextpos] - ibuf[thispos];
            diff *= frac;
            thisval = ibuf[thispos] + diff;
            thisval *= current_gain;                                //  Scale envelope by current loudness
            obuf[dz->evsamps] = (float)thisval;
            tabpos += tabincr;                                      //  Advance pointer-read
            dz->evsamps++;
        }
    }
    return FINISHED;
}

/************************************ WRITE_EVENT_TO_OUTPUT ***********************************
 *
 * Adds a specifically pitched event from event-buffer into output file, at correct time.
 */
int write_event_to_output(int passno,double current_time,double current_position,double *maxsamp,double normaliser,int *obufpos,dataptr dz)
{
    int current_left/* , current_right*/;
    int bufpos, n, j, samps_written;
    float val;
    int ochans = dz->iparam[BRCHANS];
    double leftgain, rightgain, srate = (double)dz->infile->srate;
    float *ibuf = dz->sampbuf[1];                           //  Input buffer is event buffer
    float *obuf = dz->sampbuf[2], *ovflwbuf = dz->sampbuf[3];
    bufpos = (int)round(current_time * srate) * ochans; //  Start of current N-channel block of samples in output file
    bufpos -= dz->total_samps_written;                      //  Start of current N-channel block of samples in buffer   

    while(bufpos >= dz->buflen) {                           //  If we've reached end of input buffer                        
        if(passno==0) {                                     //  On first pass, check maximum sample (for later normalisation)   
            for(n = 0;n < dz->buflen;n++) {
                if(fabs(obuf[n]) > *maxsamp)
                    *maxsamp = fabs(obuf[n]);
            }
            dz->total_samps_written += dz->buflen;          //  Maintain count of "written" samples
            time_display(dz->total_samps_written,dz);
        } else {
            for(n = 0;n < dz->buflen;n++)                   //  On second pass, normalise output, and write to file
                obuf[n] = (float)(obuf[n] * normaliser);
            if(dz->needpeaks){
                for(n=0;n < dz->buflen; n += dz->iparam[BRCHANS]){
                    for(j = 0;j < dz->outchans;j++){
                        val = (float)fabs(obuf[n+j]);
                        /* this way, posiiton of first peak value is stored */
                        if(val > dz->outpeaks[j].value){
                            dz->outpeaks[j].value = val;
                            dz->outpeaks[j].position = dz->outpeakpos[j];
                        }
                    }
                    /* count framepos */
                    for(j=0;j < dz->outchans;j++)
                        dz->outpeakpos[j]++;
                }
            }
            if((samps_written = fputfbufEx(obuf,dz->buflen,dz->ofd))<=0) {
                sprintf(errstr,"Can't write to output soundfile: %s\n",sferrstr());
                return(SYSTEM_ERROR);
            }
            dz->total_samps_written += samps_written;   
            time_display(dz->total_samps_written,dz);
        }                                                   //  copy back any overflow, and reset overflow buf to zero
        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
        bufpos -= dz->buflen;
    }
    if(dz->iparam[BRCHANS] == MONO) {
        for(n=0;n<dz->evsamps;n++) {
            obuf[bufpos] = (float)(obuf[bufpos] + ibuf[n]);
            bufpos++;
        }
    } else {
        current_left = (int)floor(current_position);            //  Find appropriate "left" and "right" channels for current-position of output
//        current_right = current_left + 1;
        current_position -= current_left;                       //  position becomes 0to1-range-position between adjacent channels
        current_position = (current_position * 2.0) - 1.0;      //  position becomes -1to+1-range- position between adjacent channels
        current_position = max(current_position,-1.0);
        current_position = min(current_position,1.0);
        pancalc(current_position,&leftgain,&rightgain);         //  Get adjusted gain for "left" and "right" contribs to output
        bufpos += current_left;                                 //  Go to correct "left" channel

        //  dz->evsamps has maximum vallue less than buflen, and overflow has length buflen:
        //  so wherver in current buffer the write starts, it will end within the buffer or the overflow

        for(n=0;n<dz->evsamps;n++) {                            //  Add new event into output buffer, in correct (pair of) channels
            obuf[bufpos] = (float)(obuf[bufpos] + (ibuf[n] * leftgain));
            bufpos++;
            obuf[bufpos] = (float)(obuf[bufpos] + (ibuf[n] * rightgain));
            bufpos--;
            bufpos += ochans;
        }
    }
    *obufpos = bufpos;
    return FINISHED;
}

/************************************ PANCALC *******************************/

#define SIGNAL_TO_LEFT  (0)
#define SIGNAL_TO_RIGHT (1)

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
        if(dirflag == SIGNAL_TO_LEFT) {
            *leftgain = invsquare;
            *rightgain = 0.0;
        } else {   /* SIGNAL_TO_RIGHT */
            *rightgain = invsquare;
            *leftgain = 0;
        }
    }
}

/******************************* TIME_DISPLAY **************************/

void time_display(int samps_sent,dataptr dz)
{
    if(sloom)
        dz->process = MTOS;
    display_virtual_time(samps_sent,dz);
    if(sloom)
        dz->process = BROWNIAN;
}

