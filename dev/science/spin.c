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

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

#define ROOT2       (1.4142136)
#define is_wide is_mapping
#define is_bare_centre is_rectified 
#define pshift_factor is_sharp

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int setup_spin_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_spin_param_ranges_and_defaults(dataptr dz);
static int handle_the_extra_infile(char ***cmdline,int *cmdlinecnt,dataptr dz);
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
static int check_spin_param_validity_and_consistency(dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static void time_display(int samps_sent,dataptr dz);
static int create_spin_sndbufs(dataptr dz);
static void calcgains(double *ch1pos,double *pos,double *lastspin,int *flipped,int *movingforward,double *leftgain,double *rightgain,double srate,dataptr dz);
static void calcgains2(double *ch1pos,double *pos,double *lastspin,int *flipped,int *movingforward,double *leftgain,double *centregain,double *rightgain,double srate,dataptr dz);
static void calcgains3(double *ch1pos,double *pos,double *lastspin,int *flipped,int *movingforward,double *leftgain,double *centregain,double *rightgain,
                double *ooleftgain, double *oileftgain, double *ocentregain, double *oirightgain, double *oorightgain, double srate,dataptr dz);
static int spindopl(dataptr dz);
static int spinwdopl(dataptr dz);
static int spinwdopl2(dataptr dz);
static int spinqdopl(dataptr dz);
static int spinqdopl2(dataptr dz);

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
        if(dz->process == SPIN)
            dz->maxmode = 3;
        else
            dz->maxmode = 2;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        dz->is_wide = 0;
        if(dz->process == SPINQ || dz->mode > 0)
            dz->is_wide = 1;
        dz->is_bare_centre = 0;
        if((dz->process == SPIN && dz->mode == 2) || (dz->process == SPINQ && dz->mode == 1))
            dz->is_bare_centre = 1;

        // setup_particular_application =
        if((exit_status = setup_spin_application(dz))<0) {
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
    if((exit_status = setup_spin_param_ranges_and_defaults(dz))<0) {
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
//  handle_extra_infiles 
    if(dz->process == SPINQ) { 
        if((exit_status = handle_the_extra_infile(&cmdline,&cmdlinecnt,dz))<0) {
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
//  handle_special_data()       redundant
 
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency()
    if((exit_status = check_spin_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = open_the_outfile(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    if(dz->process == SPINQ)
        dz->bufcnt = 5;
    else
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

    if((exit_status = create_spin_sndbufs(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess()                        redundant
    //spec_process_file =
    if(dz->process == SPIN) {
        switch(dz->mode) {
        case(0): exit_status = spindopl(dz);    break;
        case(1): exit_status = spinwdopl(dz);   break;
        case(2): exit_status = spinwdopl2(dz);  break;
        }
    } else {
        switch(dz->mode) {
        case(0): exit_status = spinqdopl(dz);   break;
        case(1): exit_status = spinqdopl2(dz);  break;
        }
    }
    if(exit_status < 0) {
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
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************ OPEN_THE_OUTFILE *********************/

int open_the_outfile(dataptr dz)
{
    int exit_status, orig_chans = dz->infile->channels;
    if(dz->is_wide)
        dz->infile->channels = dz->iparam[SPNOCHNS];
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
        return(exit_status);
    dz->infile->channels = orig_chans;
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

/************************* SETUP_SPIN_APPLICATION *******************/

int setup_spin_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if(dz->is_wide)
        exit_status = set_param_data(ap,0   ,5,5,"Diidi");
    else
        exit_status = set_param_data(ap,0   ,5,2,"D00di");
    if(exit_status<0)
        return(FAILED);
    if(dz->is_wide) {
        if(dz->is_bare_centre)
            exit_status = set_vflgs(ap,"",0,"","bak",3,3,"ddd");
        else
            exit_status = set_vflgs(ap,"",0,"","bakc",4,4,"dddd");
    } else
        exit_status = set_vflgs(ap,"",0,"","ba",2,2,"dd");
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
        } else if(infile_info->channels != 2)  {
            sprintf(errstr,"File %s is not of correct type (must be stereo)\n",cmdline[0]);
            return(DATA_ERROR);
        } else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
            sprintf(errstr,"Failed to copy file parsing information\n");
            return(PROGRAM_ERROR);
        }
        free(infile_info);
    }
    return(FINISHED);
}

/************************ HANDLE_THE_EXTRA_INFILE *********************/

int handle_the_extra_infile(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
                    /* OPEN ONE EXTRA ANALFILE, CHECK COMPATIBILITY */
    int  exit_status;
    char *filename;
    fileptr fp2;
    double maxamp, maxloc;
    int maxrep;
    int getmax = 0, getmaxinfo = 0;
    infileptr ifp;
    filename = (*cmdline)[0];
    if((dz->ifd[1] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
        sprintf(errstr,"cannot open input file %s to read data.\n",filename);
        return(DATA_ERROR);
    }   
    if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store data on later infile. (1)\n");
        return(MEMORY_ERROR);
    }
    if((fp2 = (fileptr)malloc(sizeof(struct fileprops)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store data on later infile. (2)\n");
        return(MEMORY_ERROR);
    }
    if((exit_status = readhead(ifp,dz->ifd[1],filename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
        return(exit_status);
    copy_to_fileptr(ifp,fp2);
    if(fp2->filetype != SNDFILE) {
        sprintf(errstr,"%s is not a soundfile.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->channels != 2) {
        sprintf(errstr,"File %s is not of correct type (must be stereo)\n",filename);
        return(DATA_ERROR);
    }
    if((dz->insams[1] = sndsizeEx(dz->ifd[1]))<0) {     /* FIND SIZE OF FILE */
        sprintf(errstr, "Can't read size of input file %s.\n", filename);   //RWD added filename
        return(PROGRAM_ERROR);
    }
    if(dz->insams[1]==0) {
        sprintf(errstr, "File %s contains no data.\n",filename);
        return(DATA_ERROR);
    }
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************* SETUP_SPIN_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_spin_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[SPNRATE] = -100;
    ap->hi[SPNRATE] = 100.0;
    ap->default_val[SPNRATE] = 1;
    ap->lo[SPNBOOST]    = 0;
    ap->hi[SPNBOOST]    = 16;
    ap->default_val[SPNBOOST] = 2;
    ap->lo[SPNATTEN]    = 0;
    ap->hi[SPNATTEN]    = 1;
    ap->default_val[SPNATTEN] = 0;
    if(dz->is_wide) {
        if(dz->process == SPINQ)
            ap->lo[SPNOCHNS] = 5;
        else
            ap->lo[SPNOCHNS] = 4;
        ap->hi[SPNOCHNS] = 16;
        ap->default_val[SPNOCHNS] = 8;
        ap->lo[SPNOCNTR] = 1;
        ap->hi[SPNOCNTR] = 16;
        ap->default_val[SPNOCNTR] = 1;
        ap->lo[SPNCMIN] = 0;
        ap->hi[SPNCMIN] = 1;
        ap->default_val[SPNCMIN] = 0.0;
        if(!dz->is_bare_centre) {
            ap->lo[SPNCMAX] = 0;
            ap->hi[SPNCMAX] = 1;
            ap->default_val[SPNCMAX] = 0.5;
        }
    }
    ap->lo[SPNDOPL] = 0;
    ap->hi[SPNDOPL] = 12;
    ap->default_val[SPNDOPL] = 0.0;
    ap->lo[SPNXBUF] = 1;
    ap->hi[SPNXBUF] = 64;
    ap->default_val[SPNXBUF] = 1.0;

    if(dz->process == SPINQ)
        dz->maxmode = 2;
    else
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
            dz->is_wide = 0;
            if(dz->process == SPINQ || dz->mode > 0)
                dz->is_wide = 1;
            dz->is_bare_centre = 0;
            if((dz->process == SPIN && dz->mode == 2) || (dz->process == SPINQ && dz->mode == 1))
                dz->is_bare_centre = 1;
            if((exit_status = setup_spin_application(dz))<0)
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
    "USAGE: spin NAME mode infile outfile (parameters)\n"
    "\n"
    "where NAME can be any one of\n"
    "\n"
    "stereo  quad\n"
    "\n"
    "Type 'spin stereo'  for more info on spin stereo option... ETC.\n");
    return(USAGE_ONLY);
}

/******************************** DBTOLEVEL ***********************/

double dbtolevel(double val)
{
    int isneg = 0;
    if(flteq(val,0.0))
        return(1.0);
    if(val < 0.0) {
        val = -val;
        isneg = 1;
    }
    val /= 20.0;
    val = pow(10.0,val);
    if(isneg)
        val = 1.0/val;
    return(val);
}   

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"stereo"))              dz->process = SPIN;
    else if(!strcmp(prog_identifier_from_cmdline,"quad"))           dz->process = SPINQ;
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
    if(!strcmp(str,"stereo")) {
        fprintf(stderr,
        "USAGE:  spin stereo\n"
        "1 inf outf rate dopl xbuf [-bboost] [-aatten] [-eexpbuf]\n"
        "2 inf outf rate chns cntr dopl xbuf [-bboost] [-aatt] [-kcmn] [-ccmx]\n"
        "3 inf outf rate chns cntr dopl xbuf [-bboost] [-aatt] [-kcmn]\n"
        "\n"
        "Spin a wide stereo-image across the stereo space.\n"
        "(with possible doppler-shift on the moving edges).\n"
        "\n"
        "MODES 2 & 3 create 3-chan-wide output, centred on channel \"centre\",\n"
        "       in an \"ochans\"-channel outfile.\n"
        "\n"
        "When the spinning image crosses the centre ....\n"
        "MODE 2 uses outer channels to project stereo-at-centre image.\n"
        "MODE 3 uses ONLY central channel to project stereo-at-centre image.\n"
        "\n"
        "RATE  spin speed in cycles per second (can vary over time).\n"
        "      Positive values spin clockwise (as viewed from above).\n"
        "      Negative values spin anticlockwise (as viewed from above).\n"
        "BOOST Multiplicative level changes, as edges pass through centre.\n"
        "      gradually increase (*boost) as edge passes \"FRONT\" centre\n"
        "      and decreases (*1/boost) as edge passes \"REAR\" centre.\n"
        "      The two edges pass through centre simultaneously\n"
        "      so one edge gets louder and the other quieter.\n"
        "ATT   overall level attenuation (*att) as BOTH edges pass thro centre.\n"      
        "DOPL  Max doppler pitchshift, in semitones (Range 0-12).\n"
        "XBUF  Expand buffers by this factor (may be ness for large doppler shift)\n"
        "\n"
        "Mode 2 & 3 only...\n"
        "CHNS  Number of channels in output file.\n"        
        "CNTR  Output channel which carries the central channel of the output.\n"       
        "CMN   Min level on centre lspkr (0-1).\n"
        "Mode 2 only...\n"
        "CMX   Max level on centre lspkr (0-1).\n"
        "\n"
        "\n");
    } else if(!strcmp(str,"quad")) {
        fprintf(stderr,
        "USAGE: spin quad 1 inf1 inf2 outf\n"
        "rate ochns cntr dopl xbuf [-bboost] [-aatt] [-kcmn] [-ccmx]\n"
        "OR:    spin quad 2 inf1 inf2 outf\n"
        "rate ochns cntr dopl xbuf [-bboost] [-aatt] [-kcmn]\n"
        "\n"
        "Spin two wide stereo-image across a 5-channel-wide sound image\n"
        "(with possible doppler pitch-shift of the moving edges).\n"
        "\n"
        "When the spinning image crosses the centre ....\n"
        "MODE 1 uses outer channels to project stereo-at-centre image.\n"
        "MODE 2 uses ONLY central channel to project stereo-at-centre image.\n"
        "\n"
        "RATE  spin speed in cycles per second (can vary over time).\n"
        "      Positive values spin clockwise (as viewed from above).\n"
        "      Negative values spin anticlockwise (as viewed from above).\n"
        "OCHNS Number of channels in output file.\n"        
        "CNTR  Output channel which carries the central channel of the 5 outputs.\n"        
        "DOPL  Max doppler pitchshift, in semitones (Range 0-12).\n"
        "XBUF  Expand buffers used by process (may be nesss for large doppler shift)\n"
        "BOOST Multiplicative level changes, as edges pass through centre.\n"
        "      gradually increase (*boost) as edge passes \"FRONT\" centre\n"
        "      and decreases (*1/boost) as edge passes \"REAR\" centre.\n"
        "      The two edges pass through centre simultaneously\n"
        "      so one edge gets louder and the other quieter.\n"
        "ATT   overall level decrease (*atten) as BOTH edges pass thro centre.\n"       
        "CMN   Min level on centre lspkr (0-1).\n"
        "Mode 2 only...\n"
        "CMX   Max level on centre lspkr (0-1).\n"
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

/******************************** CHECK_SPIN_PARAM_VALIDITY_AND_CONSISTENCY *****************/

int check_spin_param_validity_and_consistency(dataptr dz)
{
    int exit_status;
    if(dz->is_wide) {
        if(dz->iparam[SPNOCNTR] > dz->iparam[SPNOCHNS]) {
            sprintf(errstr,"Centre channel (%d) is not within the range of output channels available (%d).\n",dz->iparam[SPNOCNTR],dz->iparam[SPNOCHNS]);
            return DATA_ERROR;
        }
        if(!dz->is_bare_centre) {
            if(dz->param[SPNCMIN] > dz->param[SPNCMAX]) {
                sprintf(errstr,"Minimum level at centre (%lf) cannot be greater than maximum (%lf).\n",dz->param[SPNCMIN],dz->param[SPNCMAX]);
                return DATA_ERROR;
            }
        }
    }
    if(dz->brksize[SPNRATE]) {
        if((exit_status = get_maxvalue_in_brktable(&(dz->param[SPNRATE]),SPNRATE,dz))<0)
            return exit_status;
    }                                                           //  Convert semitones to octaves
    dz->param[SPNDOPL] /= SEMITONES_PER_OCTAVE;                 //  NB at max speed, doplshift = speed*pshift_factor = dopl * maxspeed/maxspeed 
    dz->pshift_factor = dz->param[SPNDOPL]/dz->param[SPNRATE];  //  at halfspeed,  doplshift = halfspeed*pshift_factor = dopl * halfspeed/maxspeed = halfshift
    return FINISHED;                                            //  at zerospeed,  doplshift = 0*pshift_factor = 0
}

/******************************** CREATE_SPIN_SNDBUFS *****************/

int create_spin_sndbufs(dataptr dz)
{
    int bigbufsize;
    int frameunit, framesize, outbufsize;
    int outchans;
    if(dz->is_wide)
        outchans = dz->iparam[SPNOCHNS];
    else
        outchans = STEREO;
    frameunit = outchans + STEREO + STEREO;         //  frame must be a multiple of (changroupsize of input*2 (2 input bufs) + changroupsize of output)
    if(dz->process == SPINQ)
        frameunit += STEREO + STEREO;               //  Two extra stereo inputs
    framesize = F_SECSIZE * frameunit;              //  frame must also be a multiple of sectorsize
    if(dz->sbufptr == 0 || dz->sampbuf==0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs()\n");
        return(PROGRAM_ERROR);
    }
    bigbufsize = (int)(size_t)Malloc(-1);
    dz->buflen = bigbufsize / sizeof(float);        //  Ensure buffer contains an integer number of frames
    dz->buflen = (dz->buflen / framesize)  * framesize;
    bigbufsize = dz->buflen * sizeof(float);
    if(dz->iparam[SPNXBUF] > 1) {
        bigbufsize *= dz->iparam[SPNXBUF];
        dz->buflen *= dz->iparam[SPNXBUF];
    }
    if(bigbufsize <= 0) {
        sprintf(errstr,"Not enough memory available for expanded buffers\n");
        return MEMORY_ERROR;
    }
    outbufsize = (dz->buflen/frameunit) * outchans; //  Get output buffer size
    dz->buflen = (dz->buflen/frameunit) * STEREO;   //  Get TRUE (input) buffer size
    bigbufsize += STEREO * 2 * sizeof(float);       //  create extra space for wraparound points of 2-input-bufs for interp of vals at buffer end
    if(dz->process == SPINQ)                        // ..and in this case, also accomodate wraparound points of 2-bufs of 2nd-infile.
        bigbufsize += STEREO * 2 * sizeof(float);

    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->buflen += STEREO;                           // accomodate wraparound points in calculating buffer boundaries
    if(dz->process == SPINQ) {
        dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
        dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
        dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
        dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + dz->buflen;  //  Each input file has 2 buffers, 1 to read left chan, other to read right
        dz->sbufptr[4] = dz->sampbuf[4] = dz->sampbuf[3] + dz->buflen;  //  Reads may be out of sync, 
        dz->sampbuf[5] = dz->sampbuf[4] + outbufsize;                   //  and left-read may exhaust buffer before right-read (or v.v.)
    } else {
        dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
        dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;
        dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;
        dz->sampbuf[3] = dz->sampbuf[2] + outbufsize;
    }
    dz->buflen -= STEREO;
    return(FINISHED);
}

/******************************** CALCGAINS *****************/

void calcgains(double *ch1pos,double *pos,double *lastspin,int *flipped,int *movingforward,double *leftgain,double *rightgain,double srate,dataptr dz)
{
    double cycleincr, lgain, rgain;

    cycleincr = dz->param[SPNRATE]/srate;       //  How far into the rotation cycle, per sample-group       
    *ch1pos += cycleincr;
                                                //  Moving up, from 0 towards 1
    if(cycleincr > 0.0) {
                                                //  If rotate changes direction, 
        if(*lastspin < 0.0)                     //  if leaving centre after passing it (flipped), we're now approaching it (!flipped)
            *flipped = !(*flipped);             //  whereas if approaching centre before passing it (!flipped), we're now leaving it (flipped)

        if(!(*flipped)) {                       //  If we've not previously reached 1/2 way through cycle (travelling up cycle)
            if(*ch1pos > 0.5) {                 //  If we've now reached 1/2 cycle end  (Left-channel moved fully from L(-1) to R(1), 1/2 way up (-cos)-table)
                *movingforward = -(*movingforward);//   This edge starts to move backwards(if previously moving forwards) (or vice versa), 
                *flipped = 1;                   //  and FLAG the fact we've passed the flip-point
            }
        }
        if(*ch1pos  > 1.0) {                    //  If we've now reached full cycle end (the end of the (-cos) table, so we're back to start from -1 0 1 0 to -1)
            *flipped = 0;                       //  reset the flip-flag
            *movingforward = -(*movingforward); //  This edge starts to move forwards again (if previously backwards) (or vice versa)
            *ch1pos -= 1.0;                     //  reset ch1pos within 0-1 range (0-2PI range of (-cos) table)
        }
        *lastspin = dz->param[SPNRATE];         //  Only set "lastspin" when spin is NON-zero, so system remembers last (non-zero) motion direction

    } else if(cycleincr < 0.0) {                //  Opposite logic, moving down from 1 to 0

        if(*lastspin > 0.0)
            *flipped = !(*flipped);

        if(!(*flipped)) {                       //  If we've not previously reached 1/2 way through cycle (travelling down cycle)
            if(*ch1pos < 0.5) {                 //  If we've now reached 1/2 cycle end  (Left-channel moved fully from L(-1) to R(1), 1/2 way down (-cos)-table)
                *movingforward = -(*movingforward); //  This edge starts to move backwards(if previously moving forwards) (or vice versa), 
                *flipped = 1;                   //  and FLAG the fact we've passed the flip-point
            }
        }
        if(*ch1pos  < 0.0) {                    //  If we've now reached full cycle end (the start of the (-cos) table, so we're back to start from -1 0 1 0 to -1)
            *flipped = 0;                       //  reset the flip-flag
            *movingforward = -(*movingforward); //  This edge starts to move forwards again (if previously backwards) (or vice versa)
            *ch1pos += 1.0;                     //  reset ch1pos within 0-1 range (0-2PI range of (-cos) table)
        }
        *lastspin = dz->param[SPNRATE];
    }
    *pos = -cos(*ch1pos * TWOPI);               //  ch1pos ranges from 0 to 1 and recycles, change range to 0 to 2PI
                                                //  -cos goes then ranges (-1 0 1 0 -1 = Left Right Left)
    pancalc(*pos,&lgain,&rgain);
    *leftgain  = lgain;
    *rightgain = rgain;
}

/******************************** SPINDOPL ********************************
 *
 *  Everything is controlled by the parameter ch1pos.
 *  which cycles 0 -> 1, then flips back to 0, at a speed controlled by ROTATION RATE.
 *
 *  Using this cycling function, "calcgains"  calculates
 *
 *  (1) "pos" ... the current position of the (original) left edge of the image, in the stereo-rotation space.
 *  (2) "leftgain" and "rightgain", the required weightings on left and right channel of output to place this at position "pos" in output.
 *  (3) "rleftgain" and "rrightgain", the weightings of the (orig) right channel-src (BY ANTI-SYMMETRY) in output image.
 *
 * The doppler logic is as follows
 *
 * If the rotation is +ve, if pos (spatial position) -ve (to left) , motion is away from listener, pitch falls, sampleread-incr +ve
 *                         if pos +ve (to right), motion is towards listener, pitch rises, sampleread-incr -ve
 *
 * Speed varies sinusoidally from max at position -1(left) and +1(right) to min at 0(centre)
 * so pitch-incr depends on position (which is varying sinusoidally) but inversely (pos to left (-ve) gives +ve sampread-incr).
 * the step thro the pitch-table for +ve rotation is thus multipled by a factor X*(-pos)
 *
 * If the rotation is -ve, if pos -ve (to left) , motion is towards listener, pitch rises, sampread-incr -ve
 *                         if pos +ve (to right), motion is away from listener, pitch falls, sampread-incr +ve
 * the step thro the pitch-table for -ve rotation is thus multipled by a factor X*(pos)
 *
 * in general pitch-incr = (X*-rotsign*pos) ... pos varying between -1 and + 1
 *
 * We want pitch-incr to increase, as rotation-speed increases
 *
 * so pitch-incr = (Y*-rotrate*pos)
 *
 * User enters the MAXIMUM pitchshift required ... using the maximum rotation-rate, we calculate the "pshift_factor"
 * The "pshift_factor" is the pshift per cycles-per-sec
 *
 * so pitch-incr = (pshift_factor*-rotrate*pos)
 *
 * !!!!!!!!!!!!!
 *
 * pos   = spatial position of(originally) left channel of rotating source
 *
 *  Data is read from two parallel buffers, which are topped up once either pointer reaches its buffer end
 *  either by a file-read, or by copying from the other (already read-into) buffer.
 *  Only when both read-processes reach their data-ends does process terminate.
 *
 * iposl = pointer to possibly-fractional read-position in input-buffer for read of (orig) left chan, counted in stereo-samples
 * iposr = simil for (orig) right channel: Due to inverse doppler shifting, these read points are generally out of step, hence the 2 read buffers.
 */

int spindopl(dataptr dz)
{
    int exit_status, passno = 0, flipped = 0, movingforward, buf_advanced_l, buf_advanced_r;
    float *ibufl = dz->sampbuf[0], *ibufr = dz->sampbuf[1], *obuf = dz->sampbuf[2];
    double srate = (double)dz->infile->srate, iposl, iposr;
    double ch1pos, normaliser = 1.0, maxsamp = 0.0, time = 0.0, firstspin, lastspin;
    double pos, leftgain, lleftgain, rleftgain, rightgain, lrightgain, rrightgain, boost, atten, frac, diff, lval, rval, incrl, incrr;
    int c1, c2, n, stereo_pairs_read, stereo_pairs_read_l, stereo_pairs_read_r, lo, hi;
    int sampsread_l, sampsread_r, stereo_pairs_buflen = dz->buflen/STEREO;
    if(dz->brksize[SPNRATE]) {
        if((exit_status= read_value_from_brktable(time,SPNRATE,dz))< 0)
            return exit_status;
    }
    firstspin = dz->param[SPNRATE];
    for(passno = 0;passno < 2; passno++) {
        dz->total_samps_written = 0;
        iposl = 0;
        iposr = 0;
        display_virtual_time(dz->total_samps_written,dz);
        ch1pos = 0;                 //  ch1pos starts at beginning of motion-cycle range (0 of 0to1)
        if(firstspin >= 0.0)
            movingforward = 1;      //  Left image moves backwards (right image moves forwards) - clockwise, viewed from above
        else
            movingforward = -1;     //  Left image moves forward (right image moves backwards) - anticlockwise, viewed from above
        lastspin = firstspin;
        memset((char *)obuf,0,dz->buflen * sizeof(float));
        dz->total_samps_read = 0;
        dz->samps_left = dz->insams[0];
        if((sndseekEx(dz->ifd[0],0,0) < 0)){
            sprintf(errstr,"sndseek failed\n");
            return SYSTEM_ERROR;
        }
        c1 = 0; //  current left sample of output buffer
        c2 = 1; //  current right sample of output buffer

        memset((char *)obuf,0,dz->buflen * sizeof(float));
        memset((char *)ibufl,0,(dz->buflen + STEREO) * sizeof(float));
        memset((char *)ibufr,0,(dz->buflen + STEREO) * sizeof(float));

        dz->buflen += STEREO;                           //  accomodate wrap-around points
        if((exit_status = read_samps(ibufl,dz))<0)
            return(exit_status);
        memcpy((char *)ibufr,(char *)ibufl,dz->ssampsread * sizeof(float));
        dz->buflen -= STEREO;
        if(dz->ssampsread > dz->buflen) {               //  IF wraparound points read
            dz->ssampsread -= STEREO;                   //  Reset buffer params
            dz->total_samps_read -= STEREO;
            dz->samps_left += STEREO;
            sndseekEx(dz->ifd[0],dz->total_samps_read,0);   //  and Reset position in file
        }
        sampsread_l = dz->ssampsread;
        sampsread_r = dz->ssampsread;
        stereo_pairs_read_l = dz->ssampsread/STEREO;
        stereo_pairs_read_r = stereo_pairs_read_l;
        stereo_pairs_read = stereo_pairs_read_l;        //  Initially, same samples on both input buffers
        buf_advanced_l = 0;
        buf_advanced_r = 0;

        //  NB only one channel of stereo needs to be calcd - other follows BY SYMMETRY
    
        while(stereo_pairs_read > 0) {                  //  Process continues until BOTH input reads are exhausted

            time = (double)((dz->total_samps_written + c1)/STEREO)/srate;
            if((exit_status = read_values_from_all_existing_brktables(time,dz))< 0)
                return exit_status;

            calcgains(&ch1pos,&pos,&lastspin,&flipped,&movingforward,&leftgain,&rightgain,srate,dz);

            //  If either of the input pointers runs out of samples, attempt to read more ....

            //  If all left samps read before all right, stereo_pairs_read_l can now be ZERO.
            //  In this case, continue to read from (ZEROED) buffer, until right-chan read also reads zero samples .. signalling both reads exhausted

            if((stereo_pairs_read_l == 0 && iposl >= stereo_pairs_buflen) || (stereo_pairs_read_l > 0 && iposl >= stereo_pairs_read_l)) {
                if(buf_advanced_l && (stereo_pairs_read_l != 0)) {      //  IF left read ALREADY ahead of right-read, and left NOT exhaused, problem
                    sprintf(errstr,"Reading samples for 1st image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced_r) {                                    //  If already read into other (right) buffer,
                    memcpy((char *)ibufl,(char *)ibufr,(sampsread_r + STEREO) * sizeof(float));
                    sampsread_l = sampsread_r;                          //  simply copy input data from one buff to other
                    stereo_pairs_read_l = stereo_pairs_read_r;          //  Reset counters
                    buf_advanced_r = 0;                                 //  and indicate buffer-R is not read-ahead of buffer-L

                } else {                                                //  IF NOT, //  Read into l-buffer etc.
                    dz->buflen += STEREO;                               //  accomodating possible wrap-around points
                    memset((char *)ibufl,0,dz->buflen * sizeof(float)); 
                    if((exit_status = read_samps(ibufl,dz))<0)          
                        return(exit_status);
                    dz->buflen -= STEREO;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        dz->total_samps_read -= STEREO;
                        dz->samps_left += STEREO;
                        sndseekEx(dz->ifd[0],dz->total_samps_read,0);
                    }
                    sampsread_l = dz->ssampsread;
                    stereo_pairs_read_l = sampsread_l/STEREO;
                    buf_advanced_l = 1;                                 //  and indicate that L-buffer is read-ahead of right buffer
                }
                                                                        //  stereo_pairs_read set to MAX of reads from left & from right chans (as 1 buffer may be exhausted)
                stereo_pairs_read = max(stereo_pairs_read_l,stereo_pairs_read_r);
                iposl -= stereo_pairs_buflen;                           //  If previous buffer was full, backtrack by buflen lands inside buffer.
                iposl = max(0.0,iposl);                                 //  BUT, if previous read reached endoffile, (hence buffer here zeroed & nothing written to it).
                                                                        //  the baktrak jumps past zerobuf to a -ve val, so just reset to buf start (to read zeros) 
            }
                                    //  SIMIL FOR READING RIGHT-CHAN INFO

            if((stereo_pairs_read_r == 0 && iposr >= stereo_pairs_buflen) || (stereo_pairs_read_r > 0 && iposr >= stereo_pairs_read_r)) {
                if(buf_advanced_r && (stereo_pairs_read_r != 0)) {
                    sprintf(errstr,"Reading samples for 2nd image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced_l) {
                    memcpy((char *)ibufr,(char *)ibufl,(sampsread_l + STEREO) * sizeof(float));
                    sampsread_r = sampsread_l;
                    stereo_pairs_read_r = stereo_pairs_read_l;
                    buf_advanced_l = 0;
                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibufr,0,dz->buflen * sizeof(float));
                    if((exit_status = read_samps(ibufr,dz))<0)
                        return(exit_status);
                    dz->buflen -= STEREO;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        dz->total_samps_read -= STEREO;
                        dz->samps_left += STEREO;
                        sndseekEx(dz->ifd[0],dz->total_samps_read,0);
                    }
                    sampsread_r = dz->ssampsread;
                    stereo_pairs_read_r = sampsread_r/STEREO;
                    buf_advanced_r = 1;
                }
                stereo_pairs_read = max(stereo_pairs_read_l,stereo_pairs_read_r);
                iposr -= stereo_pairs_buflen;
                iposr = max(0.0,iposr);
            }
            if(stereo_pairs_read == 0)          //  Once BOTH reads get zero samples, we've reached end of both read processes, so quit
                break;

            lo = (int)floor(iposl);         //  Using doppler pointer on left-chan
            frac = iposl - (double)lo;          
            lo *= STEREO;                       //  Find left-chan samples adjacent to pointer
            hi = lo + 2;                        //  and Interp value
            diff = ibufl[hi] - ibufl[lo];
            lval = ibufl[lo] + (diff * frac);
            
            lo = (int)floor(iposr);         //  Using doppler pointer on right-chan
            frac = iposr - (double)lo;          
            lo *= STEREO;                       
            lo++;                               //  Find right-chan sample
            hi = lo + 2;                        //  simil
            diff = ibufr[hi] - ibufr[lo];
            rval = ibufr[lo] + (diff * frac);

            if(dz->param[SPNATTEN] > 0.0) {         //  Atten goes linearly 0->ATTEN->0 as output moves L->C->R
                atten = (1.0 - fabs(pos)) * dz->param[SPNATTEN];
                atten = 1.0 - atten;                //  So level is multiplied by (1-atten), going from 1->(1-atten)->1 from L->C->R
                leftgain  *= atten;
                rightgain *= atten;
            }
            lleftgain  = leftgain;                  //  To position left channel in stereo of output , calculate appropriate left and right gain
            lrightgain = rightgain;
            rleftgain  = rightgain;                 //  By symmetry, right channel inverts the level of left and right
            rrightgain = leftgain;
                                                    //  Differential boost between front and rear
            if(dz->param[SPNBOOST] > 0.0) {         //  Booster goes linearly 0->BOOST->0 as output moves L->C->R
                boost = (1.0 - fabs(pos)) * dz->param[SPNBOOST];
                boost += 1.0;                       //  Booster becomes a multiplier(divider)
                if(movingforward < 0) {             //  moving forwards (other channel moving backwards)
                    lleftgain  *= boost;            //  Original chan1(left) is moving across front, positioning levels are increased
                    lrightgain *= boost;
                    rleftgain  /= boost;            //  Original chan2(right) is moving across rear, positioning levels are decreased
                    rrightgain /= boost;
                } else {
                    lleftgain  /= boost;            //  Original chan1(left) is moving across rear, positioning levels are decrease
                    lrightgain /= boost;
                    rleftgain  *= boost;            //  Original chan2(right) is moving across front, positioning levels are increased
                    rrightgain *= boost;
                }
            }
            obuf[c1] = (float)(obuf[c1] + (lval * lleftgain));      //  Orig ch1 signal positioned at new pos, to left and right
            obuf[c2] = (float)(obuf[c2] + (lval * lrightgain));
            obuf[c1] = (float)(obuf[c1] + (rval * rleftgain));      //  Orig ch2 signal positioned at new pos, to left and right
            obuf[c2] = (float)(obuf[c2] + (rval * rrightgain));

            c1 += 2;
            c2 += 2;                                //  Advance in output buffer
                                                    //  and if it fills up, write to output
            if(c1 >= dz->buflen) {
                if(passno == 0) {
                    for(n=0;n<dz->buflen;n++)
                        maxsamp = max(maxsamp,fabs(obuf[n]));
                    dz->total_samps_written += dz->buflen;  //  Update to ensure "sampletime" is calculated correctly for sloom display
                    dz->process = DISTORT_PULSED;           //  Forces correct progress-bar display on Loom
                    display_virtual_time(dz->total_samps_written,dz);
                    dz->process = SPIN;
                } else {
                    if(normaliser < 1.0) {
                        for(n=0;n<dz->buflen;n++)
                            obuf[n] = (float)(obuf[n] * normaliser);
                    }
                    dz->process = DISTORT_PULSED;   //  Forces correct progress-bar display on Loom
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    dz->process = SPIN;
                }
                memset((char *)obuf,0,dz->buflen * sizeof(float));
                c1 = 0;
                c2 = 1;
            }                                       //  Calculate read-insound increment to accomodate doppler pshift
            incrl = dz->pshift_factor * (-dz->param[SPNRATE]) * pos;
            incrl = pow(2.0,incrl);                 //  Convert octaves to frq ratio
            incrr = 2.0 - incrl;                    //  For incrs and decrs to cancel each other in long term, we must have incr2 = 2 - incrl;

            iposl += incrl;                         //  Advance in input sound according to doppler pitchshift on (originally) left edge
            iposr += incrr;                         //  Advance in input sound according to doppler pitchshift on (originally) right edge
        }
        if(c1 > 0) {                                //  Write any residual output
            if(passno == 0) {
                for(n=0;n<c1;n++)
                    maxsamp = max(maxsamp,fabs(obuf[n]));
                dz->total_samps_written += c1;
                display_virtual_time(dz->total_samps_written,dz);
            } else {
                if(normaliser < 1.0) {
                    for(n=0;n<c1;n++)
                        obuf[n] = (float)(obuf[n] * normaliser);
                }
                dz->process = DISTORT_PULSED;   //  Forces correct progress-bar display on Loom
                if((exit_status = write_samps(obuf,c1,dz))<0)
                    return(exit_status);
                dz->process = SPIN;
            }
        }
        if(passno == 0) {
            if(maxsamp > 0.95)
                normaliser = 0.95/maxsamp;
            else if(maxsamp <= FLTERR) {
                sprintf(errstr,"No significant signal found in source file.\n");
                return DATA_ERROR;
            }
        }
    }
    return FINISHED;
}

/******************************** SPINWDOPL ********************************
 *
 * same logic as spindopl, but now output goes to multichan file, and signal goes to a central channel between two orig chans.
 */

int spinwdopl(dataptr dz)
{
    int exit_status, passno = 0, flipped = 0, movingforward, buf_advanced_l, buf_advanced_r, ochans = dz->iparam[SPNOCHNS];
    float *ibufl = dz->sampbuf[0], *ibufr = dz->sampbuf[1], *obuf = dz->sampbuf[2];
    double srate = (double)dz->infile->srate, iposl, iposr;
    double ch1pos, normaliser = 1.0, maxsamp = 0.0, time = 0.0, firstspin, lastspin;
    double pos = 0.0, leftgain, lleftgain, rleftgain, rightgain, lrightgain, rrightgain, boost, atten, frac, diff, lval, rval, incrl, incrr;
    int c1, c2, cc, opos, n, obuflen, stereo_pairs_read, stereo_pairs_read_l, stereo_pairs_read_r, lo, hi;
    int sampsread_l, sampsread_r, stereo_pairs_buflen = dz->buflen/STEREO;
    int lchan, rchan, cchan = dz->iparam[SPNOCNTR];
    double boostrange = dz->param[SPNCMAX] - dz->param[SPNCMIN];

    lchan = cchan - 1;
    if(lchan < 1)
        lchan += ochans;
    rchan = cchan + 1;
    if(rchan > ochans)
        rchan -= ochans;
    lchan--;        //  Convert from 1-N frame to 0to-1 frame for countingt channels
    rchan--;
    cchan--;
    obuflen = (dz->buflen/STEREO) * ochans;     //  Calc size of output buffer

    if(dz->brksize[SPNRATE]) {
        if((exit_status= read_value_from_brktable(time,SPNRATE,dz))< 0)
            return exit_status;
    }
    firstspin = dz->param[SPNRATE];
    dz->tempsize = (dz->insams[0]/STEREO) * ochans; //  For Loom progress-bar: total size of output
    for(passno = 0;passno < 2; passno++) {
        if(passno == 0) {
            fprintf(stdout,"INFO: Assessing output level\n");
            fflush(stdout);
        } else {
            fprintf(stdout,"INFO: Creating output sound\n");
            fflush(stdout);
        }
        dz->total_samps_written = 0;
        display_virtual_time(dz->total_samps_written,dz);
        ch1pos = 0;                 //  Channel 1 starts at beginning of motion-cycle range (0 of 0to1)
        if(firstspin >= 0.0)
            movingforward = 1;      //  Left image moves backwards (right image moves forwards) - clockwise, viewed from above
        else
            movingforward = -1;     //  Left image moves forward (right image moves backwards) - anticlockwise, viewed from above
        lastspin = firstspin;
        flipped = 0;
        dz->total_samps_read = 0;
        if((sndseekEx(dz->ifd[0],0,0) < 0)){
            sprintf(errstr,"sndseek failed\n");
            return SYSTEM_ERROR;
        }
        iposl = 0;
        iposr = 0;
        opos = 0;
        c1 = lchan;
        c2 = rchan;
        cc = cchan;

        memset((char *)obuf,0,obuflen * sizeof(float));
        memset((char *)ibufl,0,(dz->buflen + STEREO) * sizeof(float));
        memset((char *)ibufr,0,(dz->buflen + STEREO) * sizeof(float));

        dz->buflen += STEREO;                           //  accomodate wrap-around points
        if((exit_status = read_samps(ibufl,dz))<0)
            return(exit_status);
        memcpy((char *)ibufr,(char *)ibufl,dz->ssampsread * sizeof(float));
        dz->buflen -= STEREO;
        if(dz->ssampsread > dz->buflen) {               //  IF wraparound points read
            dz->ssampsread -= STEREO;                   //  Reset buffer params
            dz->total_samps_read -= STEREO;
            sndseekEx(dz->ifd[0],dz->total_samps_read,0);   //  and Reset position in file
        }

        sampsread_l = dz->ssampsread;
        sampsread_r = dz->ssampsread;
        stereo_pairs_read_l = dz->ssampsread/STEREO;
        stereo_pairs_read_r = stereo_pairs_read_l;
        stereo_pairs_read = stereo_pairs_read_l;        //  Initially, same samples on both input buffers
        buf_advanced_l = 0;
        buf_advanced_r = 0;

        //  NB only one channel of stereo needs to be calcd - other follows BY SYMMETRY

        while(stereo_pairs_read > 0) {
    
            time = (double)((dz->total_samps_written + c1)/STEREO)/srate;
            if((exit_status = read_values_from_all_existing_brktables(time,dz))< 0)
                return exit_status;

            calcgains(&ch1pos,&pos,&lastspin,&flipped,&movingforward,&leftgain,&rightgain,srate,dz);

            if((stereo_pairs_read_l == 0 && iposl >= stereo_pairs_buflen) || (stereo_pairs_read_l > 0 && iposl >= stereo_pairs_read_l)) {
                if(buf_advanced_l && (stereo_pairs_read_l != 0)) {
                    sprintf(errstr,"Reading samples for 1st image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced_r) {
                    memcpy((char *)ibufl,(char *)ibufr,(sampsread_r + STEREO) * sizeof(float));
                    sampsread_l = sampsread_r;
                    stereo_pairs_read_l = stereo_pairs_read_r;
                    buf_advanced_r = 0;

                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibufl,0,dz->buflen * sizeof(float)); 
                    if((exit_status = read_samps(ibufl,dz))<0)          
                        return(exit_status);
                    dz->buflen -= STEREO;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        dz->total_samps_read -= STEREO;
                        sndseekEx(dz->ifd[0],dz->total_samps_read,0);
                    }
                    sampsread_l = dz->ssampsread;
                    stereo_pairs_read_l = sampsread_l/STEREO;
                    buf_advanced_l = 1;
                }
                stereo_pairs_read = max(stereo_pairs_read_l,stereo_pairs_read_r);
                iposl -= stereo_pairs_buflen;
                iposl = max(0.0,iposl);
            }
                                    //  SIMIL FOR READING RIGHT-CHAN INFO

            if((stereo_pairs_read_r == 0 && iposr >= stereo_pairs_buflen) || (stereo_pairs_read_r > 0 && iposr >= stereo_pairs_read_r)) {
                if(buf_advanced_r && (stereo_pairs_read_r != 0)) {
                    sprintf(errstr,"Reading samples for 2nd image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced_l) {
                    memcpy((char *)ibufr,(char *)ibufl,(sampsread_l + STEREO) * sizeof(float));
                    sampsread_r = sampsread_l;
                    stereo_pairs_read_r = stereo_pairs_read_l;
                    buf_advanced_l = 0;
                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibufr,0,dz->buflen * sizeof(float));
                    if((exit_status = read_samps(ibufr,dz))<0)
                        return(exit_status);
                    dz->buflen -= STEREO;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        dz->total_samps_read -= STEREO;
//                      dz->samps_left += STEREO;
                        sndseekEx(dz->ifd[0],dz->total_samps_read,0);
                    }
                    sampsread_r = dz->ssampsread;
                    stereo_pairs_read_r = sampsread_r/STEREO;
                    buf_advanced_r = 1;
                }
                stereo_pairs_read = max(stereo_pairs_read_l,stereo_pairs_read_r);
                iposr -= stereo_pairs_buflen;
                iposr = max(0.0,iposr);
            }
            if(stereo_pairs_read == 0)
                break;

            lo = (int)floor(iposl);         //  Using doppler pointer on left-chan
            frac = iposl - (double)lo;          
            lo *= STEREO;                       //  Find left-chan samples adjacent to pointer
            hi = lo + 2;                        //  and Interp value
            diff = ibufl[hi] - ibufl[lo];
            lval = ibufl[lo] + (diff * frac);
            
            lo = (int)floor(iposr);         //  Using doppler pointer on right-chan
            frac = iposr - (double)lo;          
            lo *= STEREO;                       
            lo++;                               //  Find right-chan sample
            hi = lo + 2;                        //  simil
            diff = ibufr[hi] - ibufr[lo];
            rval = ibufr[lo] + (diff * frac);

            if(dz->param[SPNATTEN] > 0.0) {         //  Atten goes linearly 0->ATTEN->0 as output moves L->C->R
                atten = (1.0 - fabs(pos)) * dz->param[SPNATTEN];
                atten = 1.0 - atten;                //  So level is multiplied by (1-atten), going from 1->(1-atten)->1 from L->C->R
                leftgain  *= atten;
                rightgain *= atten;
            }
            lleftgain  = leftgain;                  //  To position left channel in stereo of output , calculate appropriate left and right gain
            lrightgain = rightgain;
            rleftgain  = rightgain;                 //  By symmetry, right channel inverts the level of left and right
            rrightgain = leftgain;
                                                    //  Differential boost between front and rear
            if(dz->param[SPNBOOST] > 0.0) {         //  Booster goes linearly 0->BOOST->0 as output moves L->C->R
                boost = (1.0 - fabs(pos)) * dz->param[SPNBOOST];
                boost += 1.0;                       //  Booster becomes a multiplier(divider)
                if(movingforward < 0) {             //  moving forwards (other channel moving backwards)
                    lleftgain  *= boost;            //  Original chan1(left) is moving across front, positioning levels are increased
                    lrightgain *= boost;
                    rleftgain  /= boost;            //  Original chan2(right) is moving across rear, positioning levels are decreased
                    rrightgain /= boost;
                } else {
                    lleftgain  /= boost;            //  Original chan1(left) is moving across rear, positioning levels are decrease
                    lrightgain /= boost;
                    rleftgain  *= boost;            //  Original chan2(right) is moving across front, positioning levels are increased
                    rrightgain *= boost;
                }
            }
            obuf[c1] = (float)(obuf[c1] + (lval * lleftgain));  //  Orig ch1 signal positioned at new pos, to left and right
            obuf[c2] = (float)(obuf[c2] + (lval * lrightgain));
            obuf[c1] = (float)(obuf[c1] + (rval * rleftgain));  //  Orig ch2 signal positioned at new pos, to left and right
            obuf[c2] = (float)(obuf[c2] + (rval * rrightgain));

            boost = (1.0 - fabs(pos)) * boostrange;             //  Central channel
            boost += dz->param[SPNCMIN];
            obuf[cc] = (float)((lval + rval) * boost);          //  Gets scaled mono mix of stereo input

            opos += ochans;
            c1 += ochans;
            c2 += ochans;
            cc += ochans;
            if(opos >= obuflen) {
                if(passno == 0) {
                    for(n=0;n<obuflen;n++)
                        maxsamp = max(maxsamp,fabs(obuf[n]));
                    dz->total_samps_written += obuflen;         //  Update to ensure "time" is calculated correctly
                    dz->process = DISTORT_PULSED;               //  Forces correct progress-bar display on Loom
                    time_display(dz->total_samps_written,dz);
                    dz->process = SPIN;
                } else {
                    if(normaliser < 1.0) {
                        for(n=0;n<obuflen;n++)
                            obuf[n] = (float)(obuf[n] * normaliser);
                    }
                    dz->process = DISTORT_PULSED;
                    if((exit_status = write_samps(obuf,obuflen,dz))<0)
                        return(exit_status);
                    dz->process = SPIN;
                }
                memset((char *)obuf,0,obuflen * sizeof(float));
                opos = 0;
                c1 = lchan;
                c2 = rchan;
                cc = cchan;
            }                                       //  Calculate read-insound increment to accomodate doppler pshift
            incrl = dz->pshift_factor * (-dz->param[SPNRATE]) * pos;
            incrl = pow(2.0,incrl);                 //  Convert octaves to frq ratio
            incrr = 2.0 - incrl;                    //  For incrs and decrs to cancel each other in long term, we must have incr2 = 2 - incrl;

            iposl += incrl;                         //  Advance in input sound according to doppler pitchshift on (originally) left edge
            iposr += incrr;                         //  Advance in input sound according to doppler pitchshift on (originally) right edge
        }
        if(opos > 0) {
            if(passno == 0) {
                for(n=0;n<opos;n++)
                    maxsamp = max(maxsamp,fabs(obuf[n]));
                dz->process = DISTORT_PULSED;
                display_virtual_time(dz->tempsize,dz);
                dz->process = SPIN;
            } else {
                if(normaliser < 1.0) {
                    for(n=0;n<opos;n++)
                        obuf[n] = (float)(obuf[n] * normaliser);
                }
                dz->process = DISTORT_PULSED;
                if((exit_status = write_samps(obuf,opos,dz))<0)
                    return(exit_status);
                dz->process = SPIN;
            }
        }
        if(passno == 0) {
            if(maxsamp > 0.95)
                normaliser = 0.95/maxsamp;
            else if(maxsamp <= FLTERR) {
                sprintf(errstr,"No significant signal found in source file.\n");
                return DATA_ERROR;
            }
        }
    }
    return FINISHED;
}

/******************************** SPINQDOPL ********************************
 *
 * The same logic, with 1 stereo input on chans -1 and +1,and other on -2 and +2, around a cnetre channel.
 */

int spinqdopl(dataptr dz)
{
    int exit_status, budge, passno = 0, flipped = 0, movingforward, buf_advanced1_l, buf_advanced1_r, buf_advanced2_l, buf_advanced2_r, ochans = dz->iparam[SPNOCHNS];
    float *ibuf1l = dz->sampbuf[0], *ibuf1r = dz->sampbuf[1], *ibuf2l = dz->sampbuf[2], *ibuf2r = dz->sampbuf[3], *obuf = dz->sampbuf[4];
    double srate = (double)dz->infile->srate, ipos1l, ipos1r, ipos2l, ipos2r;
    double ch1pos, normaliser = 1.0, maxsamp = 0.0, time = 0.0, firstspin, lastspin;
    double pos = 0.0, leftgain, lleftgain, rleftgain, rightgain, lrightgain, rrightgain, boost, atten, frac, diff, lval1, rval1, lval2, rval2, incrl, incrr;
    int c1, c2, cc, c3, c4, opos, n, obuflen;
    int stereo_pairs_read, stereo_pairs_read1, stereo_pairs_read2, stereo_pairs_read1_l, stereo_pairs_read1_r, stereo_pairs_read2_l, stereo_pairs_read2_r;
    int lo, hi, total_samps_read1, total_samps_read2, sampsread1_l, sampsread1_r, sampsread2_l, sampsread2_r, stereo_pairs_buflen = dz->buflen/STEREO;
    int lchan1, rchan1, lchan2, rchan2, cchan = dz->iparam[SPNOCNTR];
    double boostrange = dz->param[SPNCMAX] - dz->param[SPNCMIN];
    lchan1 = cchan - 1;
    if(lchan1 < 1)
        lchan1 += ochans;
    rchan1 = cchan + 1;
    if(rchan1 > ochans)
        rchan1 -= ochans;
    lchan2 = cchan - 2;
    if(lchan2 < 1)
        lchan2 += ochans;
    rchan2 = cchan + 2;
    if(rchan2 > ochans)
        rchan2 -= ochans;
    lchan1--;       //  Convert from 1-N frame to 0to-1 frame for countingt channels
    rchan1--;
    lchan2--;
    rchan2--;
    cchan--;
    obuflen = (dz->buflen/STEREO) * ochans;     //  Calc size of output buffer

    if(dz->brksize[SPNRATE]) {
        if((exit_status= read_value_from_brktable(time,SPNRATE,dz))< 0)
            return exit_status;
    }
    firstspin = dz->param[SPNRATE];
    dz->total_samps_written = 0;
    dz->tempsize = (dz->insams[0]/STEREO) * ochans; //  For Loom progress-bar: total size of output
    for(passno = 0;passno < 2; passno++) {
        if(passno == 0) {
            fprintf(stdout,"INFO: Assessing output level\n");
            fflush(stdout);
        } else {
            fprintf(stdout,"INFO: Creating output sound\n");
            fflush(stdout);
        }
        dz->total_samps_written = 0;
        display_virtual_time(dz->total_samps_written,dz);
        ch1pos = 0;                 //  Channel 1 starts at beginning of motion-cycle range (0 of 0to1)
        if(firstspin >= 0.0)
            movingforward = 1;      //  Left image moves backwards (right image moves forwards) - clockwise, viewed from above
        else
            movingforward = -1;     //  Left image moves forward (right image moves backwards) - anticlockwise, viewed from above
        lastspin = firstspin;
        flipped = 0;
        dz->total_samps_read = 0;
        if((sndseekEx(dz->ifd[0],0,0) < 0)){
            sprintf(errstr,"sndseek failed in input file 1\n");
            return SYSTEM_ERROR;
        }
        if((sndseekEx(dz->ifd[1],0,0) < 0)){
            sprintf(errstr,"sndseek failed in input file 2\n");
            return SYSTEM_ERROR;
        }
        ipos1l = 0;                 //  initialise all buffer pointers
        ipos1r = 0;
        ipos2l = 0;
        ipos2r = 0;
        opos = 0;
        c1 = lchan1;
        c2 = rchan1;
        c3 = lchan2;
        c4 = rchan2;
        cc = cchan;                 //  zero all buffers, including wraparound points

        memset((char *)obuf,0,dz->buflen * sizeof(float));
        memset((char *)ibuf1l,0,(dz->buflen + STEREO) * sizeof(float));
        memset((char *)ibuf1r,0,(dz->buflen + STEREO) * sizeof(float));
        memset((char *)ibuf2l,0,(dz->buflen + STEREO) * sizeof(float));
        memset((char *)ibuf2r,0,(dz->buflen + STEREO) * sizeof(float));

        dz->buflen += STEREO;                           //  accomodate wrap-around points
        memset((char *)ibuf1l,0,dz->buflen * sizeof(float));
        if((dz->ssampsread = fgetfbufEx(ibuf1l, dz->buflen,dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Can't read samples from input soundfile 1.\n");
            return(SYSTEM_ERROR);
        }
        memcpy((char *)ibuf1r,(char *)ibuf1l,dz->ssampsread * sizeof(float));
        dz->buflen -= STEREO;
        if(dz->ssampsread > dz->buflen) {               //  IF wraparound points read
            dz->ssampsread -= STEREO;                   //  Reset buffer params
            sndseekEx(dz->ifd[0],dz->ssampsread,0);     //  and Reset position in file
        }

        total_samps_read1 = dz->ssampsread;
        sampsread1_l = dz->ssampsread;
        sampsread1_r = dz->ssampsread;
        stereo_pairs_read1_l = dz->ssampsread/STEREO;
        stereo_pairs_read1_r = stereo_pairs_read1_l;
        stereo_pairs_read1 = stereo_pairs_read1_l;      //  Initially, same samples on both input buffers
        buf_advanced1_l = 0;
        buf_advanced1_r = 0;

        dz->buflen += STEREO;                           //  accomodate wrap-around points
        memset((char *)ibuf2l,0,dz->buflen * sizeof(float));
        if((dz->ssampsread = fgetfbufEx(ibuf2l, dz->buflen,dz->ifd[1],0)) < 0) {
            sprintf(errstr,"Can't read samples from input soundfile 2.\n");
            return(SYSTEM_ERROR);
        }
        memcpy((char *)ibuf2r,(char *)ibuf2l,dz->ssampsread * sizeof(float));
        dz->buflen -= STEREO;
        if(dz->ssampsread > dz->buflen) {               //  IF wraparound points read
            dz->ssampsread -= STEREO;                   //  Reset buffer params
            sndseekEx(dz->ifd[1],dz->ssampsread,0);     //  and Reset position in file
        }

        total_samps_read2 = dz->ssampsread;
        sampsread2_l = dz->ssampsread;
        sampsread2_r = dz->ssampsread;
        stereo_pairs_read2_l = dz->ssampsread/STEREO;
        stereo_pairs_read2_r = stereo_pairs_read2_l;
        stereo_pairs_read2 = stereo_pairs_read2_l;      //  Initially, same samples on both input buffers
        buf_advanced2_l = 0;
        buf_advanced2_r = 0;

        stereo_pairs_read = max(stereo_pairs_read1,stereo_pairs_read2);

        //  NB only one channel of stereo needs to be calcd - other follows BY SYMMETRY
    
        while(stereo_pairs_read > 0) {

            time = (double)((dz->total_samps_written + opos)/ochans)/srate; //  Time calculated from count of output
            if((exit_status= read_values_from_all_existing_brktables(time,dz))< 0)
                return exit_status;

            calcgains(&ch1pos,&pos,&lastspin,&flipped,&movingforward,&leftgain,&rightgain,srate,dz);

            if((stereo_pairs_read1_l == 0 && ipos1l >= stereo_pairs_buflen) || (stereo_pairs_read1_l > 0 && ipos1l >= stereo_pairs_read1_l)) {
                if(buf_advanced1_l && (stereo_pairs_read1_l != 0)) {
                    sprintf(errstr,"Reading samples for 1st sound, 1st image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced1_r) {
                    memcpy((char *)ibuf1l,(char *)ibuf1r,(sampsread1_r + STEREO) * sizeof(float));
                    sampsread1_l = sampsread1_r;
                    stereo_pairs_read1_l = stereo_pairs_read1_r;
                    buf_advanced1_r = 0;

                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibuf1l,0,dz->buflen * sizeof(float));    
                    if((dz->ssampsread = fgetfbufEx(ibuf1l, dz->buflen,dz->ifd[0],0)) < 0) {
                        sprintf(errstr,"Can't read samples from input soundfile 1.\n");
                        return(SYSTEM_ERROR);
                    }
                    dz->buflen -= STEREO;
                    budge = 0;
                    if(dz->ssampsread > dz->buflen) {
                        budge = 1;
                        dz->ssampsread -= STEREO;
                    }
                    total_samps_read1 += dz->ssampsread;
                    if(budge)
                        sndseekEx(dz->ifd[0],total_samps_read1,0);
                    sampsread1_l = dz->ssampsread;
                    stereo_pairs_read1_l = sampsread1_l/STEREO;
                    buf_advanced1_l = 1;
                }
                stereo_pairs_read1 = max(stereo_pairs_read1_l,stereo_pairs_read1_r);
                ipos1l -= stereo_pairs_buflen;
                ipos1l = max(0.0,ipos1l);
            }
                                    //  SIMIL FOR READING RIGHT-CHAN INFO

            if((stereo_pairs_read1_r == 0 && ipos1r >= stereo_pairs_buflen) || (stereo_pairs_read1_r > 0 && ipos1r >= stereo_pairs_read1_r)) {
                if(buf_advanced1_r && (stereo_pairs_read1_r != 0)) {
                    sprintf(errstr,"Reading samples for 1st file, 2nd image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced1_l) {
                    memcpy((char *)ibuf1r,(char *)ibuf1l,(sampsread1_l + STEREO) * sizeof(float));
                    sampsread1_r = sampsread1_l;
                    stereo_pairs_read1_r = stereo_pairs_read1_l;
                    buf_advanced1_l = 0;
                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibuf1r,0,dz->buflen * sizeof(float));
                    if((dz->ssampsread = fgetfbufEx(ibuf1r, dz->buflen,dz->ifd[0],0)) < 0) {
                        sprintf(errstr,"Can't read samples from input soundfile 1.\n");
                        return(SYSTEM_ERROR);
                    }
                    dz->buflen -= STEREO;
                    budge = 0;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        budge = 1;
                    }
                    total_samps_read1 += dz->ssampsread;
                    if(budge)
                        sndseekEx(dz->ifd[0],total_samps_read1,0);
                    sampsread1_r = dz->ssampsread;
                    stereo_pairs_read1_r = sampsread1_r/STEREO;
                    buf_advanced1_r = 1;
                }
                stereo_pairs_read1 = max(stereo_pairs_read1_l,stereo_pairs_read1_r);
                ipos1r -= stereo_pairs_buflen;
                ipos1r = max(0.0,ipos1r);
            }

            //  SAME THING FOR 2ND INFILE

            if((stereo_pairs_read2_l == 0 && ipos2l >= stereo_pairs_buflen) || (stereo_pairs_read2_l > 0 && ipos2l >= stereo_pairs_read2_l)) {
                if(buf_advanced2_l && (stereo_pairs_read2_l != 0)) {
                    sprintf(errstr,"Reading samples for 2nd sound, 1st image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced2_r) {
                    memcpy((char *)ibuf2l,(char *)ibuf2r,(sampsread2_r + STEREO) * sizeof(float));
                    sampsread2_l = sampsread2_r;
                    stereo_pairs_read2_l = stereo_pairs_read2_r;
                    buf_advanced2_r = 0;

                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibuf2l,0,dz->buflen * sizeof(float));    
                    if((dz->ssampsread = fgetfbufEx(ibuf2l, dz->buflen,dz->ifd[1],0)) < 0) {
                        sprintf(errstr,"Can't read samples from input soundfile 2.\n");
                        return(SYSTEM_ERROR);
                    }
                    dz->buflen -= STEREO;
                    budge = 0;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        budge = 1;
                    }
                    total_samps_read2 += dz->ssampsread;
                    if(budge)
                        sndseekEx(dz->ifd[1],total_samps_read2,0);
                    sampsread2_l = dz->ssampsread;
                    stereo_pairs_read2_l = sampsread2_l/STEREO;
                    buf_advanced2_l = 1;
                }
                stereo_pairs_read2 = max(stereo_pairs_read2_l,stereo_pairs_read2_r);
                ipos2l -= stereo_pairs_buflen;
                ipos2l = max(0.0,ipos2l);
            }
                                    //  SIMIL FOR READING RIGHT-CHAN INFO

            if((stereo_pairs_read2_r == 0 && ipos2r >= stereo_pairs_buflen) || (stereo_pairs_read2_r > 0 && ipos2r >= stereo_pairs_read2_r)) {
                if(buf_advanced2_r && (stereo_pairs_read2_r != 0)) {
                    sprintf(errstr,"Reading samples for 2nd file, 2nd image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced2_l) {
                    memcpy((char *)ibuf2r,(char *)ibuf2l,(sampsread2_l + STEREO) * sizeof(float));
                    sampsread2_r = sampsread2_l;
                    stereo_pairs_read2_r = stereo_pairs_read2_l;
                    buf_advanced2_l = 0;
                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibuf2r,0,dz->buflen * sizeof(float));
                    if((dz->ssampsread = fgetfbufEx(ibuf2r, dz->buflen,dz->ifd[1],0)) < 0) {
                        sprintf(errstr,"Can't read samples from input soundfile 2.\n");
                        return(SYSTEM_ERROR);
                    }
                    dz->buflen -= STEREO;
                    budge = 0;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        budge = 1;
                    }
                    if(budge)
                        total_samps_read2 += dz->ssampsread;
                    sndseekEx(dz->ifd[1],total_samps_read2,0);
                    total_samps_read2 += dz->ssampsread;
                    sampsread2_r = dz->ssampsread;
                    stereo_pairs_read2_r = sampsread2_r/STEREO;
                    buf_advanced2_r = 1;
                }
                stereo_pairs_read2 = max(stereo_pairs_read2_l,stereo_pairs_read2_r);
                ipos2r -= stereo_pairs_buflen;
                ipos2r = max(0.0,ipos2r);
            }

            stereo_pairs_read = max(stereo_pairs_read1,stereo_pairs_read2);
            if(stereo_pairs_read == 0)
                break;

            lo = (int)floor(ipos1l);            //  Using doppler pointer on left-chan
            frac = ipos1l - (double)lo;         
            lo *= STEREO;                       //  Find left-chan samples adjacent to pointer
            hi = lo + 2;                        //  and Interp value
            diff  = ibuf1l[hi] - ibuf1l[lo];
            lval1 = ibuf1l[lo] + (diff * frac);
            
            lo = (int)floor(ipos1r);            //  Using doppler pointer on right-chan
            frac = ipos1r - (double)lo;         
            lo *= STEREO;                       
            lo++;                               //  Find right-chan sample
            hi = lo + 2;                        //  simil
            diff  = ibuf1r[hi] - ibuf1r[lo];
            rval1 = ibuf1r[lo] + (diff * frac);

            //  and for file 2          

            lo = (int)floor(ipos2l);            //  Using doppler pointer on left-chan
            frac = ipos2l - (double)lo;         
            lo *= STEREO;                       //  Find left-chan samples adjacent to pointer
            hi = lo + 2;                        //  and Interp value
            diff  = ibuf2l[hi] - ibuf2l[lo];
            lval2 = ibuf2l[lo] + (diff * frac);
            
            lo = (int)floor(ipos2r);            //  Using doppler pointer on right-chan
            frac = ipos2r - (double)lo;         
            lo *= STEREO;                       
            lo++;                               //  Find right-chan sample
            hi = lo + 2;                        //  simil
            diff  = ibuf2r[hi] - ibuf2r[lo];
            rval2 = ibuf2r[lo] + (diff * frac);


            if(dz->param[SPNATTEN] > 0.0) {         //  Atten goes linearly 0->ATTEN->0 as output moves L->C->R
                atten = (1.0 - fabs(pos)) * dz->param[SPNATTEN];
                atten = 1.0 - atten;                //  So level is multiplied by (1-atten), going from 1->(1-atten)->1 from L->C->R
                leftgain  *= atten;
                rightgain *= atten;
            }
            lleftgain  = leftgain;                  //  To position left channel in stereo of output , calculate appropriate left and right gain
            lrightgain = rightgain;
            rleftgain  = rightgain;                 //  By symmetry, right channel inverts the level of left and right
            rrightgain = leftgain;
                                                    //  Differential boost between front and rear
            if(dz->param[SPNBOOST] > 0.0) {         //  Booster goes linearly 0->BOOST->0 as output moves L->C->R
                boost = (1.0 - fabs(pos)) * dz->param[SPNBOOST];
                boost += 1.0;                       //  Booster becomes a multiplier(divider)
                if(movingforward < 0) {             //  moving forwards (other channel moving backwards)
                    lleftgain  *= boost;            //  Original chan1(left) is moving across front, positioning levels are increased
                    lrightgain *= boost;
                    rleftgain  /= boost;            //  Original chan2(right) is moving across rear, positioning levels are decreased
                    rrightgain /= boost;
                } else {
                    lleftgain  /= boost;            //  Original chan1(left) is moving across rear, positioning levels are decrease
                    lrightgain /= boost;
                    rleftgain  *= boost;            //  Original chan2(right) is moving across front, positioning levels are increased
                    rrightgain *= boost;
                }
            }
            obuf[c1] = (float)(obuf[c1] + (lval1 * lleftgain)); //  Orig file1 left signal positioned at new pos, to left and right
            obuf[c2] = (float)(obuf[c2] + (lval1 * lrightgain));
            obuf[c3] = (float)(obuf[c3] + (lval2 * lleftgain)); //  Orig file2 left signal positioned at new pos, to left and right
            obuf[c4] = (float)(obuf[c4] + (lval2 * lrightgain));

            obuf[c1] = (float)(obuf[c1] + (rval1 * rleftgain)); //  Orig file1 right signal positioned at new pos, to left and right
            obuf[c2] = (float)(obuf[c2] + (rval1 * rrightgain));
            obuf[c3] = (float)(obuf[c3] + (rval2 * rleftgain)); //  Orig file2 right signal positioned at new pos, to left and right
            obuf[c4] = (float)(obuf[c4] + (rval2 * rrightgain));

            boost = (1.0 - fabs(pos)) * boostrange;             //  Central channel
            boost += dz->param[SPNCMIN];
            obuf[cc] = (float)((lval1 + rval1 + lval2 + rval2) * boost);            //  Gets scaled mono mix of stereo input

            opos += ochans;
            c1 += ochans;
            c2 += ochans;
            c3 += ochans;
            c4 += ochans;
            cc += ochans;
            if(opos >= obuflen) {
                if(passno == 0) {
                    for(n=0;n<obuflen;n++)
                        maxsamp = max(maxsamp,fabs(obuf[n]));
                    dz->total_samps_written += obuflen;         //  Update to ensure "time" is calculated correctly
                    dz->process = DISTORT_PULSED;               //  Forces correct progress-bar display on Loom
                    time_display(dz->total_samps_written,dz);
                    dz->process = SPIN;
                } else {
                    if(normaliser < 1.0) {
                        for(n=0;n<obuflen;n++)
                            obuf[n] = (float)(obuf[n] * normaliser);
                    }
                    dz->process = DISTORT_PULSED;
                    if((exit_status = write_samps(obuf,obuflen,dz))<0)
                        return(exit_status);
                    dz->process = SPIN;
                }
                memset((char *)obuf,0,obuflen * sizeof(float));
                opos = 0;
                c1 = lchan1;
                c2 = rchan1;
                c3 = lchan2;
                c4 = rchan2;
                cc = cchan;
            }                                       //  Calculate read-insound increment to accomodate doppler pshift

            incrl = dz->pshift_factor * (-dz->param[SPNRATE]) * pos;
            incrl = pow(2.0,incrl);                 //  Convert octaves to frq ratio
            incrr = 2.0 - incrl;                    //  For incrs and decrs to cancel each other in long term, we must have incr2 = 2 - incrl;

            ipos1l += incrl;                        //  Advance in input sound according to doppler pitchshift on (originally) left edge
            ipos1r += incrr;                        //  Advance in input sound according to doppler pitchshift on (originally) right edge
            ipos2l += incrl;                        //  and same for other file
            ipos2r += incrr;
        }
        if(opos > 0) {
            if(passno == 0) {
                for(n=0;n<opos;n++)
                    maxsamp = max(maxsamp,fabs(obuf[n]));
                dz->process = DISTORT_PULSED;
                display_virtual_time(dz->tempsize,dz);
                dz->process = SPIN;
            } else {
                if(normaliser < 1.0) {
                    for(n=0;n<opos;n++)
                        obuf[n] = (float)(obuf[n] * normaliser);
                }
                dz->process = DISTORT_PULSED;
                if((exit_status = write_samps(obuf,opos,dz))<0)
                    return(exit_status);
                dz->process = SPIN;
            }
        }
        if(passno == 0) {
            if(maxsamp > 0.95)
                normaliser = 0.95/maxsamp;
            else if(maxsamp <= FLTERR) {
                sprintf(errstr,"No significant signal found in source file.\n");
                return DATA_ERROR;
            }
        }
    }
    return FINISHED;
}

/******************************** SPINWDOPL2 ********************************
 *
 *  SPINWDOPL retains pans L and R edges of sound image  between channels C-1 and C+1 (where C is centre channel)
 *      complementing it with (variable) extra signal in C.
 * SPINWDOPL2 pans L-edge from chan C-1 to C, then from C to C+1, (and R-edge C+1 ->C then C->C-1)
 *      so that signal in centre channel is generated by the Ledge->C or Redge->C pans, directly.
 */

int spinwdopl2(dataptr dz)
{
    int exit_status, passno = 0, flipped = 0, movingforward, buf_advanced_l, buf_advanced_r, ochans = dz->iparam[SPNOCHNS];
    float *ibufl = dz->sampbuf[0], *ibufr = dz->sampbuf[1], *obuf = dz->sampbuf[2];
    double srate = (double)dz->infile->srate, iposl, iposr;
    double ch1pos, normaliser = 1.0, maxsamp = 0.0, time = 0.0, firstspin, lastspin;
    double pos = 0.0, leftgain, lleftgain, rleftgain, rightgain, lrightgain, rrightgain, centregain, lcentregain, rcentregain, atten, boost, frac, diff, lval, rval, incrl, incrr;
    int c1, c2, cc, opos, n, obuflen, stereo_pairs_read, stereo_pairs_read_l, stereo_pairs_read_r, lo, hi;
    int sampsread_l, sampsread_r, stereo_pairs_buflen = dz->buflen/STEREO;
    int lchan, rchan, cchan = dz->iparam[SPNOCNTR];

    lchan = cchan - 1;
    if(lchan < 1)
        lchan += ochans;
    rchan = cchan + 1;
    if(rchan > ochans)
        rchan -= ochans;
    lchan--;        //  Convert from 1-N frame to 0to-1 frame for countingt channels
    rchan--;
    cchan--;
    obuflen = (dz->buflen/STEREO) * ochans;     //  Calc size of output buffer

    atten = 1.0 - dz->param[SPNATTEN];
    if(dz->brksize[SPNRATE]) {
        if((exit_status= read_value_from_brktable(time,SPNRATE,dz))< 0)
            return exit_status;
    }
    firstspin = dz->param[SPNRATE];
    dz->tempsize = (dz->insams[0]/STEREO) * ochans; //  For Loom progress-bar: total size of output
    for(passno = 0;passno < 2; passno++) {
        if(passno == 0) {
            fprintf(stdout,"INFO: Assessing output level\n");
            fflush(stdout);
        } else {
            fprintf(stdout,"INFO: Creating output sound\n");
            fflush(stdout);
        }
        dz->total_samps_written = 0;
        display_virtual_time(dz->total_samps_written,dz);
        ch1pos = 0;                 //  Channel 1 starts at beginning of motion-cycle range (0 of 0to1)
        if(firstspin >= 0.0)
            movingforward = 1;      //  Left image moves backwards (right image moves forwards) - clockwise, viewed from above
        else
            movingforward = -1;     //  Left image moves forward (right image moves backwards) - anticlockwise, viewed from above
        lastspin = firstspin;
        flipped = 0;
        dz->total_samps_read = 0;
//      dz->samps_left = dz->insams[0];
        if((sndseekEx(dz->ifd[0],0,0) < 0)){
            sprintf(errstr,"sndseek failed\n");
            return SYSTEM_ERROR;
        }
        iposl = 0;
        iposr = 0;
        opos = 0;
        c1 = lchan;
        c2 = rchan;
        cc = cchan;

        memset((char *)obuf,0,obuflen * sizeof(float));
        memset((char *)ibufl,0,(dz->buflen + STEREO) * sizeof(float));
        memset((char *)ibufr,0,(dz->buflen + STEREO) * sizeof(float));

        dz->buflen += STEREO;                           //  accomodate wrap-around points
        if((exit_status = read_samps(ibufl,dz))<0)
            return(exit_status);
        memcpy((char *)ibufr,(char *)ibufl,dz->ssampsread * sizeof(float));
        dz->buflen -= STEREO;
        if(dz->ssampsread > dz->buflen) {               //  IF wraparound points read
            dz->ssampsread -= STEREO;                   //  Reset buffer params
            dz->total_samps_read -= STEREO;
//          dz->samps_left += STEREO;
            sndseekEx(dz->ifd[0],dz->total_samps_read,0);   //  and Reset position in file
        }

        sampsread_l = dz->ssampsread;
        sampsread_r = dz->ssampsread;
        stereo_pairs_read_l = dz->ssampsread/STEREO;
        stereo_pairs_read_r = stereo_pairs_read_l;
        stereo_pairs_read = stereo_pairs_read_l;        //  Initially, same samples on both input buffers
        buf_advanced_l = 0;
        buf_advanced_r = 0;

        //  NB only one channel of stereo needs to be calcd - other follows BY SYMMETRY

        while(stereo_pairs_read > 0) {
    
            time = (double)((dz->total_samps_written + c1)/STEREO)/srate;
            if((exit_status = read_values_from_all_existing_brktables(time,dz))< 0)
                return exit_status;

            calcgains2(&ch1pos,&pos,&lastspin,&flipped,&movingforward,&leftgain,&centregain,&rightgain,srate,dz);

            if((stereo_pairs_read_l == 0 && iposl >= stereo_pairs_buflen) || (stereo_pairs_read_l > 0 && iposl >= stereo_pairs_read_l)) {
                if(buf_advanced_l && (stereo_pairs_read_l != 0)) {
                    sprintf(errstr,"Reading samples for 1st image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced_r) {
                    memcpy((char *)ibufl,(char *)ibufr,(sampsread_r + STEREO) * sizeof(float));
                    sampsread_l = sampsread_r;
                    stereo_pairs_read_l = stereo_pairs_read_r;
                    buf_advanced_r = 0;

                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibufl,0,dz->buflen * sizeof(float)); 
                    if((exit_status = read_samps(ibufl,dz))<0)          
                        return(exit_status);
                    dz->buflen -= STEREO;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        dz->total_samps_read -= STEREO;
                        sndseekEx(dz->ifd[0],dz->total_samps_read,0);
                    }
                    sampsread_l = dz->ssampsread;
                    stereo_pairs_read_l = sampsread_l/STEREO;
                    buf_advanced_l = 1;
                }
                stereo_pairs_read = max(stereo_pairs_read_l,stereo_pairs_read_r);
                iposl -= stereo_pairs_buflen;
                iposl = max(0.0,iposl);
            }
                                    //  SIMIL FOR READING RIGHT-CHAN INFO

            if((stereo_pairs_read_r == 0 && iposr >= stereo_pairs_buflen) || (stereo_pairs_read_r > 0 && iposr >= stereo_pairs_read_r)) {
                if(buf_advanced_r && (stereo_pairs_read_r != 0)) {
                    sprintf(errstr,"Reading samples for 2nd image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced_l) {
                    memcpy((char *)ibufr,(char *)ibufl,(sampsread_l + STEREO) * sizeof(float));
                    sampsread_r = sampsread_l;
                    stereo_pairs_read_r = stereo_pairs_read_l;
                    buf_advanced_l = 0;
                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibufr,0,dz->buflen * sizeof(float));
                    if((exit_status = read_samps(ibufr,dz))<0)
                        return(exit_status);
                    dz->buflen -= STEREO;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        dz->total_samps_read -= STEREO;
                        sndseekEx(dz->ifd[0],dz->total_samps_read,0);
                    }
                    sampsread_r = dz->ssampsread;
                    stereo_pairs_read_r = sampsread_r/STEREO;
                    buf_advanced_r = 1;
                }
                stereo_pairs_read = max(stereo_pairs_read_l,stereo_pairs_read_r);
                iposr -= stereo_pairs_buflen;
                iposr = max(0.0,iposr);
            }
            if(stereo_pairs_read == 0)
                break;

            lo = (int)floor(iposl);         //  Using doppler pointer on left-chan
            frac = iposl - (double)lo;          
            lo *= STEREO;                       //  Find left-chan samples adjacent to pointer
            hi = lo + 2;                        //  and Interp value
            diff = ibufl[hi] - ibufl[lo];
            lval = ibufl[lo] + (diff * frac);
            
            lo = (int)floor(iposr);         //  Using doppler pointer on right-chan
            frac = iposr - (double)lo;          
            lo *= STEREO;                       
            lo++;                               //  Find right-chan sample
            hi = lo + 2;                        //  simil
            diff = ibufr[hi] - ibufr[lo];
            rval = ibufr[lo] + (diff * frac);

            lleftgain   = leftgain;             //  Position originally-L-edge by weighting on 3 lspkrs
            lcentregain = centregain;           
            lrightgain  = rightgain;            

            rleftgain   = rightgain;            //  By antisymmetry, do opposite for other edge
            rcentregain = centregain;
            rrightgain  = leftgain;
                                                    //  Differential boost between front and rear
            if(dz->param[SPNBOOST] > 0.0) {         //  Booster goes linearly 0->BOOST->0 as output moves L->C->R
                boost = (1.0 - fabs(pos)) * dz->param[SPNBOOST];
                boost += 1.0;                       //  Booster becomes a multiplier(divider)
                if(movingforward < 0) {             //  moving forwards (other channel moving backwards)
                    lcentregain *= boost;           //  Original chan1(left) is moving across front, positioning levels are increased
                    rcentregain /= boost;           //  Original chan2(right) is moving across rear, positioning levels are decreased
                } else {
                    lcentregain /= boost;           //  Original chan1(left) is moving across rear, positioning levels are decrease
                    rcentregain *= boost;           //  Original chan2(right) is moving across front, positioning levels are increased
                }
            }
            obuf[c1] = (float)(obuf[c1] + (lval * lleftgain));  //  Orig ch1 signal contribution , on left and right chans
            obuf[c2] = (float)(obuf[c2] + (lval * lrightgain));
            obuf[c1] = (float)(obuf[c1] + (rval * rleftgain));  //  Orig ch2 signal contribution , on left and right chans
            obuf[c2] = (float)(obuf[c2] + (rval * rrightgain));

            obuf[cc] = (float)(((lval * lcentregain) + (rval * rcentregain)) * atten);  //  Contribution of orig ch1 and ch2 to centre chan level
            obuf[cc] = (float)(obuf[cc] + ((lval + rval) * dz->param[SPNCMIN]));        //  Add monoed src at min-centre-level
            opos += ochans;
            c1 += ochans;
            c2 += ochans;
            cc += ochans;
            if(opos >= obuflen) {
                if(passno == 0) {
                    for(n=0;n<obuflen;n++)
                        maxsamp = max(maxsamp,fabs(obuf[n]));
                    dz->total_samps_written += obuflen;         //  Update to ensure "time" is calculated correctly
                    dz->process = DISTORT_PULSED;               //  Forces correct progress-bar display on Loom
                    time_display(dz->total_samps_written,dz);
                    dz->process = SPIN;
                } else {
                    if(normaliser < 1.0) {
                        for(n=0;n<obuflen;n++)
                            obuf[n] = (float)(obuf[n] * normaliser);
                    }
                    dz->process = DISTORT_PULSED;
                    if((exit_status = write_samps(obuf,obuflen,dz))<0)
                        return(exit_status);
                    dz->process = SPIN;
                }
                memset((char *)obuf,0,obuflen * sizeof(float));
                opos = 0;
                c1 = lchan;
                c2 = rchan;
                cc = cchan;
            }                                       //  Calculate read-insound increment to accomodate doppler pshift
            incrl = dz->pshift_factor * (-dz->param[SPNRATE]) * pos;
            incrl = pow(2.0,incrl);                 //  Convert octaves to frq ratio
            incrr = 2.0 - incrl;                    //  For incrs and decrs to cancel each other in long term, we must have incr2 = 2 - incrl;

            iposl += incrl;                         //  Advance in input sound according to doppler pitchshift on (originally) left edge
            iposr += incrr;                         //  Advance in input sound according to doppler pitchshift on (originally) right edge
        }
        if(opos > 0) {
            if(passno == 0) {
                for(n=0;n<opos;n++)
                    maxsamp = max(maxsamp,fabs(obuf[n]));
                dz->process = DISTORT_PULSED;
                display_virtual_time(dz->tempsize,dz);
                dz->process = SPIN;
            } else {
                if(normaliser < 1.0) {
                    for(n=0;n<opos;n++)
                        obuf[n] = (float)(obuf[n] * normaliser);
                }
                dz->process = DISTORT_PULSED;
                if((exit_status = write_samps(obuf,opos,dz))<0)
                    return(exit_status);
                dz->process = SPIN;
            }
        }
        if(passno == 0) {
            if(maxsamp > 0.95)
                normaliser = 0.95/maxsamp;
            else if(maxsamp <= FLTERR) {
                sprintf(errstr,"No significant signal found in source file.\n");
                return DATA_ERROR;
            }
        }
    }
    return FINISHED;
}

/******************************** CALCGAINS2 *****************
 *
 * This function outputs the weightings between L and C channel, and C & RIGHT channels for the item plaaced at "pos" in (L->R) range  -1 to 1
 */

void calcgains2(double *ch1pos,double *pos,double *lastspin,int *flipped,int *movingforward,double *leftgain,double *centregain,double *rightgain,double srate,dataptr dz)
{
    double cycleincr, lgain, rgain, lrcpos;

    cycleincr = dz->param[SPNRATE]/srate;       //  How far into the rotation cycle, per sample-group       
    *ch1pos += cycleincr;
                                                //  Moving up, from 0 towards 1
    if(cycleincr > 0.0) {
                                                //  If rotate changes direction, 
        if(*lastspin < 0.0)                     //  if leaving centre after passing it (flipped), we're now approaching it (!flipped)
            *flipped = !(*flipped);             //  whereas if approaching centre before passing it (!flipped), we're now leaving it (flipped)

        if(!(*flipped)) {                       //  If we've not previously reached 1/2 way through cycle (travelling up cycle)
            if(*ch1pos > 0.5) {                 //  If we've now reached 1/2 cycle end  (Left-channel moved fully from L(-1) to R(1), 1/2 way up (-cos)-table)
                *movingforward = -(*movingforward);//   This edge starts to move backwards(if previously moving forwards) (or vice versa), 
                *flipped = 1;                   //  and FLAG the fact we've passed the flip-point
            }
        }
        if(*ch1pos  > 1.0) {                    //  If we've now reached full cycle end (the end of the (-cos) table, so we're back to start from -1 0 1 0 to -1)
            *flipped = 0;                       //  reset the flip-flag
            *movingforward = -(*movingforward); //  This edge starts to move forwards again (if previously backwards) (or vice versa)
            *ch1pos -= 1.0;                     //  reset ch1pos within 0-1 range (0-2PI range of (-cos) table)
        }
        *lastspin = dz->param[SPNRATE];         //  Only set "lastspin" when spin is NON-zero, so system remembers last (non-zero) motion direction

    } else if(cycleincr < 0.0) {                //  Opposite logic, moving down from 1 to 0

        if(*lastspin > 0.0)
            *flipped = !(*flipped);

        if(!(*flipped)) {                       //  If we've not previously reached 1/2 way through cycle (travelling down cycle)
            if(*ch1pos < 0.5) {                 //  If we've now reached 1/2 cycle end  (Left-channel moved fully from L(-1) to R(1), 1/2 way down (-cos)-table)
                *movingforward = -(*movingforward); //  This edge starts to move backwards(if previously moving forwards) (or vice versa), 
                *flipped = 1;                   //  and FLAG the fact we've passed the flip-point
            }
        }
        if(*ch1pos  < 0.0) {                    //  If we've now reached full cycle end (the start of the (-cos) table, so we're back to start from -1 0 1 0 to -1)
            *flipped = 0;                       //  reset the flip-flag
            *movingforward = -(*movingforward); //  This edge starts to move forwards again (if previously backwards) (or vice versa)
            *ch1pos += 1.0;                     //  reset ch1pos within 0-1 range (0-2PI range of (-cos) table)
        }
        *lastspin = dz->param[SPNRATE];
    }
    *pos = -cos(*ch1pos * TWOPI);               //  ch1pos ranges from 0 to 1 and recycles, change range to 0 to 2PI
                                                //  -cos goes then ranges (-1 0 1 0 -1 = Left Right Left)

                                                //  pos range   = -1 to +1
    lrcpos = *pos * 2.0;                        //  lrcpos Range = -2 to 2
    if(*pos <= 0.0) {                           //  i.e. pos Range = -1 to 0, lrcpos range = -2 to 0
        lrcpos += 1.0;                          //  lrcpos range -1 to + 1
        pancalc(lrcpos,&lgain,&rgain);
        *leftgain   = lgain;
        *centregain = rgain;
        *rightgain  = 0.0;
    } else {                                    //  i.e. pos Range = 0 to 1, lcpos range = 0 to 2
        lrcpos -= 1.0;                          //  lrcpos range = -1 to +1
        pancalc(lrcpos,&lgain,&rgain);
        *leftgain   = 0.0;
        *centregain = lgain;
        *rightgain  = rgain;
    }
}

/******************************** SPINQDOPL2 ********************************
 *
 * The same logic, with 1 stereo input on chans -1 and +1,and other on -2 and +2, around a centre channel.
 */

int spinqdopl2(dataptr dz)
{
    int exit_status, budge, passno = 0, flipped = 0, movingforward, buf_advanced1_l, buf_advanced1_r, buf_advanced2_l, buf_advanced2_r, ochans = dz->iparam[SPNOCHNS];
    float *ibuf1l = dz->sampbuf[0], *ibuf1r = dz->sampbuf[1], *ibuf2l = dz->sampbuf[2], *ibuf2r = dz->sampbuf[3], *obuf = dz->sampbuf[4];
    double srate = (double)dz->infile->srate, ipos1l, ipos1r, ipos2l, ipos2r;
    double ch1pos, normaliser = 1.0, maxsamp = 0.0, time = 0.0, firstspin, lastspin;
    double pos = 0.0, leftgain, lleftgain, rleftgain, rightgain, lrightgain, rrightgain, boost, halfboost, atten, frac, diff, lval1, rval1, lval2, rval2, incrl, incrr;
    double centregain,ooleftgain,oileftgain,ocentregain,oirightgain,oorightgain, oval1, oval2;
    double loleftgain, lileftgain, lcentregain, lirightgain, lorightgain, roleftgain, rileftgain, rcentregain, rirightgain, rorightgain;        
    int c1, c2, cc, c3, c4, opos, n, obuflen;
    int stereo_pairs_read, stereo_pairs_read1, stereo_pairs_read2, stereo_pairs_read1_l, stereo_pairs_read1_r, stereo_pairs_read2_l, stereo_pairs_read2_r;
    int lo, hi, total_samps_read1, total_samps_read2, sampsread1_l, sampsread1_r, sampsread2_l, sampsread2_r, stereo_pairs_buflen = dz->buflen/STEREO;
    int lchan1, rchan1, lchan2, rchan2, cchan = dz->iparam[SPNOCNTR];
    lchan1 = cchan - 1;
    if(lchan1 < 1)
        lchan1 += ochans;
    rchan1 = cchan + 1;
    if(rchan1 > ochans)
        rchan1 -= ochans;
    lchan2 = cchan - 2;
    if(lchan2 < 1)
        lchan2 += ochans;
    rchan2 = cchan + 2;
    if(rchan2 > ochans)
        rchan2 -= ochans;
    lchan1--;       //  Convert from 1-N frame to 0to-1 frame for countingt channels
    rchan1--;
    lchan2--;
    rchan2--;
    cchan--;
    obuflen = (dz->buflen/STEREO) * ochans;     //  Calc size of output buffer
    atten = 1.0 - dz->param[SPNATTEN];

    if(dz->brksize[SPNRATE]) {
        if((exit_status= read_value_from_brktable(time,SPNRATE,dz))< 0)
            return exit_status;
    }
    firstspin = dz->param[SPNRATE];
    dz->total_samps_written = 0;
    dz->tempsize = (dz->insams[0]/STEREO) * ochans; //  For Loom progress-bar: total size of output
    for(passno = 0;passno < 2; passno++) {
        if(passno == 0) {
            fprintf(stdout,"INFO: Assessing output level\n");
            fflush(stdout);
        } else {
            fprintf(stdout,"INFO: Creating output sound\n");
            fflush(stdout);
        }
        dz->total_samps_written = 0;
        display_virtual_time(dz->total_samps_written,dz);
        ch1pos = 0;                 //  Channel 1 starts at beginning of motion-cycle range (0 of 0to1)
        if(firstspin >= 0.0)
            movingforward = 1;      //  Left image moves backwards (right image moves forwards) - clockwise, viewed from above
        else
            movingforward = -1;     //  Left image moves forward (right image moves backwards) - anticlockwise, viewed from above
        lastspin = firstspin;
        flipped = 0;
        dz->total_samps_read = 0;
        if((sndseekEx(dz->ifd[0],0,0) < 0)){
            sprintf(errstr,"sndseek failed in input file 1\n");
            return SYSTEM_ERROR;
        }
        if((sndseekEx(dz->ifd[1],0,0) < 0)){
            sprintf(errstr,"sndseek failed in input file 2\n");
            return SYSTEM_ERROR;
        }
        ipos1l = 0;                 //  initialise all buffer pointers
        ipos1r = 0;
        ipos2l = 0;
        ipos2r = 0;
        opos = 0;
        c1 = lchan1;
        c2 = rchan1;
        c3 = lchan2;
        c4 = rchan2;
        cc = cchan;                 //  zero all buffers, including wraparound points

        memset((char *)obuf,0,dz->buflen * sizeof(float));
        memset((char *)ibuf1l,0,(dz->buflen + STEREO) * sizeof(float));
        memset((char *)ibuf1r,0,(dz->buflen + STEREO) * sizeof(float));
        memset((char *)ibuf2l,0,(dz->buflen + STEREO) * sizeof(float));
        memset((char *)ibuf2r,0,(dz->buflen + STEREO) * sizeof(float));

        dz->buflen += STEREO;                           //  accomodate wrap-around points
        if((dz->ssampsread = fgetfbufEx(ibuf1l, dz->buflen,dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Can't read samples from input soundfile 1.\n");
            return(SYSTEM_ERROR);
        }
        memcpy((char *)ibuf1r,(char *)ibuf1l,dz->ssampsread * sizeof(float));
        dz->buflen -= STEREO;
        if(dz->ssampsread > dz->buflen) {               //  IF wraparound points read
            dz->ssampsread -= STEREO;                   //  Reset buffer params
            sndseekEx(dz->ifd[0],dz->ssampsread,0);     //  and Reset position in file
        }

        total_samps_read1 = dz->ssampsread;
        sampsread1_l = dz->ssampsread;
        sampsread1_r = dz->ssampsread;
        stereo_pairs_read1_l = dz->ssampsread/STEREO;
        stereo_pairs_read1_r = stereo_pairs_read1_l;
        stereo_pairs_read1 = stereo_pairs_read1_l;      //  Initially, same samples on both input buffers
        buf_advanced1_l = 0;
        buf_advanced1_r = 0;

        dz->buflen += STEREO;                           //  accomodate wrap-around points
        if((dz->ssampsread = fgetfbufEx(ibuf2l, dz->buflen,dz->ifd[1],0)) < 0) {
            sprintf(errstr,"Can't read samples from input soundfile 2.\n");
            return(SYSTEM_ERROR);
        }
        memcpy((char *)ibuf2r,(char *)ibuf2l,dz->ssampsread * sizeof(float));
        dz->buflen -= STEREO;
        if(dz->ssampsread > dz->buflen) {               //  IF wraparound points read
            dz->ssampsread -= STEREO;                   //  Reset buffer params
            sndseekEx(dz->ifd[1],dz->ssampsread,0);     //  and Reset position in file
        }

        total_samps_read2 = dz->ssampsread;
        sampsread2_l = dz->ssampsread;
        sampsread2_r = dz->ssampsread;
        stereo_pairs_read2_l = dz->ssampsread/STEREO;
        stereo_pairs_read2_r = stereo_pairs_read2_l;
        stereo_pairs_read2 = stereo_pairs_read2_l;      //  Initially, same samples on both input buffers
        buf_advanced2_l = 0;
        buf_advanced2_r = 0;

        stereo_pairs_read = max(stereo_pairs_read1,stereo_pairs_read2);

        //  NB only one channel of stereo needs to be calcd - other follows BY SYMMETRY
    
        while(stereo_pairs_read > 0) {

            time = (double)((dz->total_samps_written + opos)/ochans)/srate; //  Time calculated from count of output
            if((exit_status= read_values_from_all_existing_brktables(time,dz))< 0)
                return exit_status;

            calcgains3(&ch1pos,&pos,&lastspin,&flipped,&movingforward,&leftgain,&centregain,&rightgain,&ooleftgain,&oileftgain,&ocentregain,&oirightgain,&oorightgain,srate,dz);

            if((stereo_pairs_read1_l == 0 && ipos1l >= stereo_pairs_buflen) || (stereo_pairs_read1_l > 0 && ipos1l >= stereo_pairs_read1_l)) {
                if(buf_advanced1_l && (stereo_pairs_read1_l != 0)) {
                    sprintf(errstr,"Reading samples for 1st sound, 1st image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced1_r) {
                    memcpy((char *)ibuf1l,(char *)ibuf1r,(sampsread1_r + STEREO) * sizeof(float));
                    sampsread1_l = sampsread1_r;
                    stereo_pairs_read1_l = stereo_pairs_read1_r;
                    buf_advanced1_r = 0;

                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibuf1l,0,dz->buflen * sizeof(float));    
                    if((dz->ssampsread = fgetfbufEx(ibuf1l, dz->buflen,dz->ifd[0],0)) < 0) {
                        sprintf(errstr,"Can't read samples from input soundfile 1.\n");
                        return(SYSTEM_ERROR);
                    }
                    dz->buflen -= STEREO;
                    budge = 0;
                    if(dz->ssampsread > dz->buflen) {
                        budge = 1;
                        dz->ssampsread -= STEREO;
                    }
                    total_samps_read1 += dz->ssampsread;
                    if(budge)
                        sndseekEx(dz->ifd[0],total_samps_read1,0);
                    sampsread1_l = dz->ssampsread;
                    stereo_pairs_read1_l = sampsread1_l/STEREO;
                    buf_advanced1_l = 1;
                }
                stereo_pairs_read1 = max(stereo_pairs_read1_l,stereo_pairs_read1_r);
                ipos1l -= stereo_pairs_buflen;
                ipos1l = max(0.0,ipos1l);
            }
                                    //  SIMIL FOR READING RIGHT-CHAN INFO

            if((stereo_pairs_read1_r == 0 && ipos1r >= stereo_pairs_buflen) || (stereo_pairs_read1_r > 0 && ipos1r >= stereo_pairs_read1_r)) {
                if(buf_advanced1_r && (stereo_pairs_read1_r != 0)) {
                    sprintf(errstr,"Reading samples for 1st file, 2nd image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced1_l) {
                    memcpy((char *)ibuf1r,(char *)ibuf1l,(sampsread1_l + STEREO) * sizeof(float));
                    sampsread1_r = sampsread1_l;
                    stereo_pairs_read1_r = stereo_pairs_read1_l;
                    buf_advanced1_l = 0;
                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibuf1r,0,dz->buflen * sizeof(float));
                    if((dz->ssampsread = fgetfbufEx(ibuf1r, dz->buflen,dz->ifd[0],0)) < 0) {
                        sprintf(errstr,"Can't read samples from input soundfile 1.\n");
                        return(SYSTEM_ERROR);
                    }
                    dz->buflen -= STEREO;
                    budge = 0;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        budge = 1;
                    }
                    total_samps_read1 += dz->ssampsread;
                    if(budge)
                        sndseekEx(dz->ifd[0],total_samps_read1,0);
                    sampsread1_r = dz->ssampsread;
                    stereo_pairs_read1_r = sampsread1_r/STEREO;
                    buf_advanced1_r = 1;
                }
                stereo_pairs_read1 = max(stereo_pairs_read1_l,stereo_pairs_read1_r);
                ipos1r -= stereo_pairs_buflen;
                ipos1r = max(0.0,ipos1r);
            }

            //  SAME THING FOR 2ND INFILE

            if((stereo_pairs_read2_l == 0 && ipos2l >= stereo_pairs_buflen) || (stereo_pairs_read2_l > 0 && ipos2l >= stereo_pairs_read2_l)) {
                if(buf_advanced2_l && (stereo_pairs_read2_l != 0)) {
                    sprintf(errstr,"Reading samples for 2nd sound, 1st image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced2_r) {
                    memcpy((char *)ibuf2l,(char *)ibuf2r,(sampsread2_r + STEREO) * sizeof(float));
                    sampsread2_l = sampsread2_r;
                    stereo_pairs_read2_l = stereo_pairs_read2_r;
                    buf_advanced2_r = 0;

                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibuf2l,0,dz->buflen * sizeof(float));    
                    if((dz->ssampsread = fgetfbufEx(ibuf2l, dz->buflen,dz->ifd[1],0)) < 0) {
                        sprintf(errstr,"Can't read samples from input soundfile 2.\n");
                        return(SYSTEM_ERROR);
                    }
                    dz->buflen -= STEREO;
                    budge = 0;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        budge = 1;
                    }
                    total_samps_read2 += dz->ssampsread;
                    if(budge)
                        sndseekEx(dz->ifd[1],total_samps_read2,0);
                    sampsread2_l = dz->ssampsread;
                    stereo_pairs_read2_l = sampsread2_l/STEREO;
                    buf_advanced2_l = 1;
                }
                stereo_pairs_read2 = max(stereo_pairs_read2_l,stereo_pairs_read2_r);
                ipos2l -= stereo_pairs_buflen;
                ipos2l = max(0.0,ipos2l);
            }
                                    //  SIMIL FOR READING RIGHT-CHAN INFO

            if((stereo_pairs_read2_r == 0 && ipos2r >= stereo_pairs_buflen) || (stereo_pairs_read2_r > 0 && ipos2r >= stereo_pairs_read2_r)) {
                if(buf_advanced2_r && (stereo_pairs_read2_r != 0)) {
                    sprintf(errstr,"Reading samples for 2nd file, 2nd image-edge exceeds other by > buffer length: CANNOT PROCEED\n");
                    return MEMORY_ERROR;
                }
                if(buf_advanced2_l) {
                    memcpy((char *)ibuf2r,(char *)ibuf2l,(sampsread2_l + STEREO) * sizeof(float));
                    sampsread2_r = sampsread2_l;
                    stereo_pairs_read2_r = stereo_pairs_read2_l;
                    buf_advanced2_l = 0;
                } else {
                    dz->buflen += STEREO;
                    memset((char *)ibuf2r,0,dz->buflen * sizeof(float));
                    if((dz->ssampsread = fgetfbufEx(ibuf2r, dz->buflen,dz->ifd[1],0)) < 0) {
                        sprintf(errstr,"Can't read samples from input soundfile 2.\n");
                        return(SYSTEM_ERROR);
                    }
                    dz->buflen -= STEREO;
                    budge = 0;
                    if(dz->ssampsread > dz->buflen) {
                        dz->ssampsread -= STEREO;
                        budge = 1;
                    }
                    total_samps_read2 += dz->ssampsread;
                    if(budge)
                        sndseekEx(dz->ifd[1],total_samps_read2,0);
                    total_samps_read2 += dz->ssampsread;
                    sampsread2_r = dz->ssampsread;
                    stereo_pairs_read2_r = sampsread2_r/STEREO;
                    buf_advanced2_r = 1;
                }
                stereo_pairs_read2 = max(stereo_pairs_read2_l,stereo_pairs_read2_r);
                ipos2r -= stereo_pairs_buflen;
                ipos2r = max(0.0,ipos2r);
            }

            stereo_pairs_read = max(stereo_pairs_read1,stereo_pairs_read2);
            if(stereo_pairs_read == 0)
                break;

            lo = (int)floor(ipos1l);            //  Using doppler pointer on left-chan
            frac = ipos1l - (double)lo;         
            lo *= STEREO;                       //  Find left-chan samples adjacent to pointer
            hi = lo + 2;                        //  and Interp value
            diff  = ibuf1l[hi] - ibuf1l[lo];
            lval1 = ibuf1l[lo] + (diff * frac);
            
            lo = (int)floor(ipos1r);            //  Using doppler pointer on right-chan
            frac = ipos1r - (double)lo;         
            lo *= STEREO;                       
            lo++;                               //  Find right-chan sample
            hi = lo + 2;                        //  simil
            diff  = ibuf1r[hi] - ibuf1r[lo];
            rval1 = ibuf1r[lo] + (diff * frac);

            //  and for file 2          

            lo = (int)floor(ipos2l);            //  Using doppler pointer on left-chan
            frac = ipos2l - (double)lo;         
            lo *= STEREO;                       //  Find left-chan samples adjacent to pointer
            hi = lo + 2;                        //  and Interp value
            diff  = ibuf2l[hi] - ibuf2l[lo];
            lval2 = ibuf2l[lo] + (diff * frac);
            
            lo = (int)floor(ipos2r);            //  Using doppler pointer on right-chan
            frac = ipos2r - (double)lo;         
            lo *= STEREO;                       
            lo++;                               //  Find right-chan sample
            hi = lo + 2;                        //  simil
            diff  = ibuf2r[hi] - ibuf2r[lo];
            rval2 = ibuf2r[lo] + (diff * frac);

            //  FOR THE INNER SOUND

            lleftgain   = leftgain;             //  Position originally-L-edge by weighting on 3 lspkrs
            lcentregain = centregain;           
            lrightgain  = rightgain;            

            rleftgain   = rightgain;            //  By antisymmetry, do opposite for other edge
            rcentregain = centregain;
            rrightgain  = leftgain;

            //  Differential boost between front and rear
            if(dz->param[SPNBOOST] > 0.0) {         //  Booster goes linearly 0->BOOST->0 as output moves L->C->R
                boost = (1.0 - fabs(pos)) * dz->param[SPNBOOST];
                boost += 1.0;                       //  Booster becomes a multiplier(divider)
                if(movingforward < 0) {             //  moving forwards (other channel moving backwards)
                    lcentregain *= boost;           //  Original chan1(left) is moving across front, positioning levels are increased
                    rcentregain /= boost;           //  Original chan2(right) is moving across rear, positioning levels are decreased
                } else {
                    lcentregain /= boost;           //  Original chan1(left) is moving across rear, positioning levels are decrease
                    rcentregain *= boost;           //  Original chan2(right) is moving across front, positioning levels are increased
                }
            }
            obuf[c1] = (float)(obuf[c1] + (lval1 * lleftgain)); //  Orig ch1 signal contribution , on left and right chans
            obuf[c2] = (float)(obuf[c2] + (lval1 * lrightgain));
            obuf[c1] = (float)(obuf[c1] + (rval1 * rleftgain)); //  Orig ch2 signal contribution , on left and right chans
            obuf[c2] = (float)(obuf[c2] + (rval1 * rrightgain));

            oval1  = (float)(((lval1 * lcentregain) + (rval1 * rcentregain)) * atten);  //  Contribution of orig ch1 and ch2 to centre chan level
            oval1 += (lval1 + rval1) * dz->param[SPNCMIN];                              //  Add monoed src at min-centre-level
            obuf[cc] = (float)(obuf[cc] + oval1);

            //  FOR THE OUTER SOUND

            loleftgain  = ooleftgain;           //  Weightings on the 5 loudspkrs, of the orig-LEFT-edge of image
            lileftgain  = oileftgain;
            lcentregain = ocentregain;          
            lirightgain = oirightgain;          
            lorightgain = oorightgain;          

            roleftgain  = oorightgain;          //  by anti-symmetry, Weightings for the orig-RIGHT-edge of image
            rileftgain  = oirightgain;
            rcentregain = ocentregain;          
            rirightgain = oileftgain;           
            rorightgain = ooleftgain;           



        //  Differential boost between front and rear
            if(dz->param[SPNBOOST] > 0.0) {         //  Booster goes linearly 0->BOOST->0 as output moves L->C->R
                boost = (1.0 - fabs(pos)) * dz->param[SPNBOOST];
                boost += 1.0;                       //  Booster becomes a multiplier(divider)
                halfboost = boost/2.0;
                if(movingforward < 0) {             //  moving forwards (other channel moving backwards)
                    lcentregain *= boost;           //  Original chan1(left) is moving across front, positioning levels are increased
                    lileftgain  *= halfboost;
                    lirightgain *= halfboost;
                    rcentregain /= boost;           //  Original chan2(right) is moving across rear, positioning levels are decreased
                    rileftgain  /= halfboost; 
                    rirightgain /= halfboost; 
                } else {
                    lcentregain /= boost;           //  Original chan1(left) is moving across rear, positioning levels are decrease
                    lileftgain  /= halfboost;
                    lirightgain /= halfboost;
                    rcentregain *= boost;           //  Original chan2(right) is moving across front, positioning levels are increased
                    rileftgain  *= halfboost; 
                    rirightgain *= halfboost; 
                }
            }                                                       //  Orig L-edge signal contribution , on all 5 chans
            obuf[c3] = (float)(obuf[c3] + (lval2 * loleftgain));    //  Left outer chan
            obuf[c1] = (float)(obuf[c1] + (lval2 * lileftgain));    //  Left inner chan
            obuf[c2] = (float)(obuf[c2] + (lval2 * lirightgain));   //  Right inner chan
            obuf[c4] = (float)(obuf[c4] + (lval2 * lorightgain));   //  Right outer chan

            obuf[c3] = (float)(obuf[c3] + (rval2 * roleftgain));    //  Orig R-edge signal contribution , on all 5 chans
            obuf[c1] = (float)(obuf[c1] + (rval2 * rileftgain));
            obuf[c2] = (float)(obuf[c2] + (rval2 * rirightgain));
            obuf[c4] = (float)(obuf[c4] + (rval2 * rorightgain));

            oval2 = ((lval2 * lcentregain) + (rval2 * rcentregain)) * atten;    //  Contribution of orig ch1 and ch2 (outer) snd to centre chan level
            oval2 += (lval2 + rval2) * dz->param[SPNCMIN];                      //  Add monoed src at min-centre-level
            obuf[cc] = (float)(obuf[cc] + oval2);
            opos += ochans;
            c1 += ochans;
            c2 += ochans;
            c3 += ochans;
            c4 += ochans;
            cc += ochans;
            if(opos >= obuflen) {
                if(passno == 0) {
                    for(n=0;n<obuflen;n++)
                        maxsamp = max(maxsamp,fabs(obuf[n]));
                    dz->total_samps_written += obuflen;         //  Update to ensure "time" is calculated correctly
                    dz->process = DISTORT_PULSED;               //  Forces correct progress-bar display on Loom
                    time_display(dz->total_samps_written,dz);
                    dz->process = SPIN;
                } else {
                    if(normaliser < 1.0) {
                        for(n=0;n<obuflen;n++)
                            obuf[n] = (float)(obuf[n] * normaliser);
                    }
                    dz->process = DISTORT_PULSED;
                    if((exit_status = write_samps(obuf,obuflen,dz))<0)
                        return(exit_status);
                    dz->process = SPIN;
                }
                memset((char *)obuf,0,obuflen * sizeof(float));
                opos = 0;
                c1 = lchan1;
                c2 = rchan1;
                c3 = lchan2;
                c4 = rchan2;
                cc = cchan;
            }                                       //  Calculate read-insound increment to accomodate doppler pshift

            incrl = dz->pshift_factor * (-dz->param[SPNRATE]) * pos;
            incrl = pow(2.0,incrl);                 //  Convert octaves to frq ratio
            incrr = 2.0 - incrl;                    //  For incrs and decrs to cancel each other in long term, we must have incr2 = 2 - incrl;

            ipos1l += incrl;                        //  Advance in input sound according to doppler pitchshift on (originally) left edge
            ipos1r += incrr;                        //  Advance in input sound according to doppler pitchshift on (originally) right edge
            ipos2l += incrl;                        //  and same for other file
            ipos2r += incrr;
        }
        if(opos > 0) {
            if(passno == 0) {
                for(n=0;n<opos;n++)
                    maxsamp = max(maxsamp,fabs(obuf[n]));
                dz->process = DISTORT_PULSED;
                display_virtual_time(dz->tempsize,dz);
                dz->process = SPIN;
            } else {
                if(normaliser < 1.0) {
                    for(n=0;n<opos;n++)
                        obuf[n] = (float)(obuf[n] * normaliser);
                }
                dz->process = DISTORT_PULSED;
                if((exit_status = write_samps(obuf,opos,dz))<0)
                    return(exit_status);
                dz->process = SPIN;
            }
        }
        if(passno == 0) {
            if(maxsamp > 0.95)
                normaliser = 0.95/maxsamp;
            else if(maxsamp <= FLTERR) {
                sprintf(errstr,"No significant signal found in source file.\n");
                return DATA_ERROR;
            }
        }
    }
    return FINISHED;
}

/******************************** CALCGAINS3 *******************************
 *
 * This function outputs the weightings between L and C channel, and C & RIGHT channels for the item plaaced at "pos" in (L->R) range  -1 to 1
 */

void calcgains3(double *ch1pos,double *pos,double *lastspin,int *flipped,int *movingforward,double *leftgain,double *centregain,double *rightgain,
                double *ooleftgain, double *oileftgain, double *ocentregain, double *oirightgain, double *oorightgain, double srate,dataptr dz)
{
    double cycleincr, lgain, rgain, lrcpos, olrcpos;

    cycleincr = dz->param[SPNRATE]/srate;       //  How far into the rotation cycle, per sample-group       
    *ch1pos += cycleincr;
                                                //  Moving up, from 0 towards 1
    if(cycleincr > 0.0) {
                                                //  If rotate changes direction, 
        if(*lastspin < 0.0)                     //  if leaving centre after passing it (flipped), we're now approaching it (!flipped)
            *flipped = !(*flipped);             //  whereas if approaching centre before passing it (!flipped), we're now leaving it (flipped)

        if(!(*flipped)) {                       //  If we've not previously reached 1/2 way through cycle (travelling up cycle)
            if(*ch1pos > 0.5) {                 //  If we've now reached 1/2 cycle end  (Left-channel moved fully from L(-1) to R(1), 1/2 way up (-cos)-table)
                *movingforward = -(*movingforward);//   This edge starts to move backwards(if previously moving forwards) (or vice versa), 
                *flipped = 1;                   //  and FLAG the fact we've passed the flip-point
            }
        }
        if(*ch1pos  > 1.0) {                    //  If we've now reached full cycle end (the end of the (-cos) table, so we're back to start from -1 0 1 0 to -1)
            *flipped = 0;                       //  reset the flip-flag
            *movingforward = -(*movingforward); //  This edge starts to move forwards again (if previously backwards) (or vice versa)
            *ch1pos -= 1.0;                     //  reset ch1pos within 0-1 range (0-2PI range of (-cos) table)
        }
        *lastspin = dz->param[SPNRATE];         //  Only set "lastspin" when spin is NON-zero, so system remembers last (non-zero) motion direction

    } else if(cycleincr < 0.0) {                //  Opposite logic, moving down from 1 to 0

        if(*lastspin > 0.0)
            *flipped = !(*flipped);

        if(!(*flipped)) {                       //  If we've not previously reached 1/2 way through cycle (travelling down cycle)
            if(*ch1pos < 0.5) {                 //  If we've now reached 1/2 cycle end  (Left-channel moved fully from L(-1) to R(1), 1/2 way down (-cos)-table)
                *movingforward = -(*movingforward); //  This edge starts to move backwards(if previously moving forwards) (or vice versa), 
                *flipped = 1;                   //  and FLAG the fact we've passed the flip-point
            }
        }
        if(*ch1pos  < 0.0) {                    //  If we've now reached full cycle end (the start of the (-cos) table, so we're back to start from -1 0 1 0 to -1)
            *flipped = 0;                       //  reset the flip-flag
            *movingforward = -(*movingforward); //  This edge starts to move forwards again (if previously backwards) (or vice versa)
            *ch1pos += 1.0;                     //  reset ch1pos within 0-1 range (0-2PI range of (-cos) table)
        }
        *lastspin = dz->param[SPNRATE];
    }
    *pos = -cos(*ch1pos * TWOPI);               //  ch1pos ranges from 0 to 1 and recycles, change range to 0 to 2PI
                                                //  -cos goes then ranges (-1 0 1 0 -1 = Left Right Left)

                                                //  pos range   = -1 to +1
    lrcpos = *pos * 2.0;                        //  lrcpos Range = -2 to 2
    if(*pos <= 0.0) {                           //  i.e. pos Range = -1 to 0, lrcpos range = -2 to 0
        lrcpos += 1.0;                          //  lrcpos range -1 to + 1
        pancalc(lrcpos,&lgain,&rgain);
        *leftgain   = lgain;
        *centregain = rgain;
        *rightgain  = 0.0;
    } else {                                    //  i.e. pos Range = 0 to 1, lcpos range = 0 to 2
        lrcpos -= 1.0;                          //  lrcpos range = -1 to +1
        pancalc(lrcpos,&lgain,&rgain);
        *leftgain   = 0.0;
        *centregain = lgain;
        *rightgain  = rgain;
    }

    olrcpos = *pos * 4.0;                       //  olrcpos Range = -4 to 4
    if(*pos <= -0.5) {                          //  i.e. pos Range = -1 to -0.5, olrcpos range = -4 to -2
        olrcpos += 3.0;                         //  olrcpos range -1 to + 1
        pancalc(olrcpos,&lgain,&rgain);
        *ooleftgain   = lgain;
        *oileftgain   = rgain;
        *ocentregain  = 0.0;
        *oirightgain  = 0.0;
        *oorightgain  = 0.0;
    } else if(*pos <= 0.0) {                    //  i.e. pos Range = -0.5 to 0, olcpos range = -2 to 0
        olrcpos += 1.0;                         //  olrcpos range = -1 to +1
        pancalc(olrcpos,&lgain,&rgain);
        *ooleftgain   = 0.0;
        *oileftgain   = lgain;
        *ocentregain  = rgain;
        *oirightgain  = 0.0;
        *oorightgain  = 0.0;
    } else if(*pos <= 0.5) {                    //  i.e. pos Range = 0 to 0.5, olcpos range = 0 to 2
        olrcpos -= 1.0;                         //  olrcpos range = -1 to +1
        pancalc(olrcpos,&lgain,&rgain);
        *ooleftgain   = 0.0;
        *oileftgain   = 0.0;
        *ocentregain  = lgain;
        *oirightgain  = rgain;
        *oorightgain  = 0.0;
    } else {                                    //  i.e. pos Range = 0.5 to 1, olcpos range = 2 to 4
        olrcpos -= 3.0;                         //  olrcpos range = -1 to +1
        pancalc(olrcpos,&lgain,&rgain);
        *ooleftgain   = 0.0;
        *oileftgain   = 0.0;
        *ocentregain  = 0.0;
        *oirightgain  = lgain;
        *oorightgain  = rgain;
    }
}

