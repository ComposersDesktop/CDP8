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
#include <envlcon.h>
#include <srates.h>

#ifdef unix
#define round(x) lround((x))
#endif

#define envcnt  wlength
#define trofcnt rampbrksize
#define ENV_FSECSIZE 256
#define ENVSPEAK_PKSRCHWIDTH 3
#define MINEVENTFRQ 50      //  generates the minimum possible event length for mode 7

#define ESPK_GATED  ESPK_REPET
#define ESPK_SEED   ESPK_OFFST
#define ESPK_NWISE  ESPK_OFFST
#define ESPK_RATIO  ESPK_WHICH
#define ESPK_RAND   ESPK_GAIN

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;
//not used in Soundloom 17.0.4x
const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int check_envspeak_param_validity_and_consistency(dataptr dz);
static int setup_envspeak_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_envspeak_param_ranges_and_defaults(dataptr dz);
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
static int precalculate_peaks_array_and_splice(dataptr dz);
static int envspeak(dataptr dz);
static int getmaxattencnt(dataptr dz);
static int windows_in_sndfile(dataptr dz);
static int getenv_of_buffer(int samps_to_process,float **env,dataptr dz);
static double getmaxsamp(int startsamp, int sampcnt,float *buffer);
static int istrof(float *env,float *envend,float *q,int width);
static int randvary_pklen(int peaklen,dataptr dz);
static double dbtolevel(double val);
static int getmaxwhich(dataptr dz);
static int open_next_file(char *outfilename,int n,dataptr dz);
static void randperm(int z,int setlen,dataptr dz);
static void hinsert(int z,int m,int t,int setlen,dataptr dz);
static void hprefix(int z,int m,int setlen,dataptr dz);
static void hshuflup(int z,int k,int setlen,dataptr dz);
static int getcutdata(int *cmdlinecnt,char ***cmdline,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
    int n;
//   aplptr ap;
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
    dz->trofcnt = 0;
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
        dz->maxmode = 25;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_envspeak_application(dz))<0) {
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
    if((exit_status = setup_envspeak_param_ranges_and_defaults(dz))<0) {
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
 
    if(dz->mode >= 12) {
        if((exit_status = getcutdata(&cmdlinecnt,&cmdline,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIBs
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    dz->bufcnt = 4;
    if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {   //RWD need the extra buf here too
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    for(n = 0;n <dz->bufcnt; n++)
        dz->sampbuf[n] = dz->sbufptr[n] = (float *)0;
    dz->sampbuf[n] = (float *)0;

    //  1 double array for splice
    if((dz->parray = (double **)malloc(sizeof(double *))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create splice buffer (1).\n");
        return(MEMORY_ERROR);
    }
    //  2 float arrays for trofstore
    if((dz->fptr=(float **)malloc(2 * sizeof(float *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store envelope (1).\n");
        return(MEMORY_ERROR);
    }
//  create_sndbufs
    if((exit_status = create_sndbufs_for_envel(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    
    if(dz->mode != 24) {
        if((exit_status = precalculate_peaks_array_and_splice(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        dz->mode %= 12;
        //    check_param_validity_and_consistency....
        if((exit_status = check_envspeak_param_validity_and_consistency(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    //param_preprocess()                        redundant
    //spec_process_file =
    if((exit_status = envspeak(dz))<0) {
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
/* RWD malloc changed to calloc; helps debug version run as release! */

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
    char *filename = (*cmdline)[0], *nufilename;
    char prefix_units[] = "_00";

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
    if(dz->mode == 24) {
        strcpy(dz->outfilename,filename);
        if((exit_status = create_sized_outfile(filename,dz))<0)
            return(exit_status);
        (*cmdline)++;
        (*cmdlinecnt)--;
        return(FINISHED);
    }
    if(dz->mode == 9 || dz->mode == 21) {
        if((dz->wordstor = (char **)malloc(sizeof (char *)))==NULL) {
            sprintf(errstr,"Cannot set up storage for infile name (1)\n");
            return(MEMORY_ERROR);
        }
        if((dz->wordstor[0] = (char *)malloc((strlen(filename) + 12) * sizeof (char)))==NULL) {
            sprintf(errstr,"Cannot set up storage for infile name (2)\n");
            return(MEMORY_ERROR);
        }
        strcpy(dz->wordstor[0],filename);
    }
    if((dz->mode == 9 || dz->mode == 21) && !sloom) {
        if((nufilename = (char *)malloc((strlen(filename) + 12) * sizeof (char)))==NULL) {
            sprintf(errstr,"Cannot set up storage for infile name (2)\n");
            return(MEMORY_ERROR);
        }
        strcpy(nufilename,filename);
        insert_new_chars_at_filename_end(nufilename,prefix_units);
        insert_new_number_at_filename_end(nufilename,0,0);
        strcpy(dz->outfilename,nufilename);
        if((exit_status = create_sized_outfile(nufilename,dz))<0)
            return(exit_status);
    } else {
        strcpy(dz->outfilename,filename);
        if((exit_status = create_sized_outfile(filename,dz))<0)
            return(exit_status);
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

/************************* SETUP_ENVSPEAK_APPLICATION *******************/

int setup_envspeak_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    switch(dz->mode) {
    case(4):    //  fall thro
    case(5):    //  fall thro
    case(0): exit_status = set_param_data(ap,0   ,6,5,"iiiID0");    break;  //  repet & repet-shrink
    case(1): exit_status = set_param_data(ap,0   ,6,3,"iii000");    break;  //  reverse-repet
    case(2):    //  fall thro
    case(3): exit_status = set_param_data(ap,0   ,6,5,"iiiID0");    break;  //  atten "alternate"
    case(6): exit_status = set_param_data(ap,0   ,6,6,"iiiIDI");    break;  //  repeat part-of
    case(7):    //  fall thro
    case(8): exit_status = set_param_data(ap,0   ,6,6,"iiiIDD");    break;  //  repeat but shrink
    case(9): exit_status = set_param_data(ap,0   ,6,2,"ii0000");    break;  //  extract all
    case(10): exit_status = set_param_data(ap,0  ,6,3,"iii000");    break;  //  permute randomly
    case(11): exit_status = set_param_data(ap,0  ,6,3,"iiI000");    break;  //  permute N-wise
    case(16):   //  fall thro
    case(17):   //  fall thro
    case(12): exit_status = set_param_data(ap,XSPK_CUTS  ,6,4,"0iiID0");    break;  //  repet & repet-shrink
    case(13): exit_status = set_param_data(ap,XSPK_CUTS  ,6,2,"0ii000");    break;  //  reverse-repet
    case(14):   //  fall thro
    case(15): exit_status = set_param_data(ap,XSPK_CUTS  ,6,4,"0iiID0");    break;  //  atten "alternate"
    case(18): exit_status = set_param_data(ap,XSPK_CUTS  ,6,5,"0iiIDI");    break;  //  repeat part-of
    case(19):   //  fall thro
    case(20): exit_status = set_param_data(ap,XSPK_CUTS  ,6,5,"0iiIDD");    break;  //  repeat but shrink
    case(21): exit_status = set_param_data(ap,XSPK_CUTS  ,6,1,"0i0000");    break;  //  extract all
    case(22): exit_status = set_param_data(ap,XSPK_CUTS  ,6,2,"0ii000");    break;  //  permute randomly
    case(23): exit_status = set_param_data(ap,XSPK_CUTS  ,6,2,"0iI000");    break;  //  permute N-wise
    case(24): exit_status = set_param_data(ap,XSPK_CUTS  ,0,0,"");            break;    //    remove silences
    }
    if(exit_status < 0)
        return(FAILED);
    switch(dz->mode) {
    case(6):    //  fall thro
    case(18):
        exit_status = set_vflgs(ap,"",0,"","z",1,0,"0");
        break;
    default:
        exit_status = set_vflgs(ap,"",0,"","",0,0,"");
        break;
    }
    if(exit_status < 0)
        return(FAILED);
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = SNDFILES_ONLY;
    if(dz->mode == 2 || dz->mode == 3 || dz->mode == 14 || dz->mode == 15)
        dz->process_type    = EQUAL_SNDFILE;    
    else
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
        } else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
            sprintf(errstr,"Failed to copy file parsing information\n");
            return(PROGRAM_ERROR);
        }
        free(infile_info);
    }
    return(FINISHED);
}

/************************* SETUP_ENVSPEAK_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_envspeak_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    int mode = dz->mode % 12;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    if(dz->mode == 24) {
        dz->maxmode = 25;
        return(FINISHED);
    }
    ap->lo[ESPK_WINSZ]  = 5;                //  param 0
    ap->hi[ESPK_WINSZ]  = 1000;
    ap->default_val[ESPK_WINSZ] = 50;
    ap->lo[ESPK_SPLEN]  = 2;                //  param 1
    ap->hi[ESPK_SPLEN]  = 100;
    ap->default_val[ESPK_SPLEN] = 5;
    if(mode < 9) {
        ap->lo[ESPK_OFFST]  = 0;                //  param 2
        ap->hi[ESPK_OFFST]  = 100;
        ap->default_val[ESPK_OFFST] = 0;
        switch(mode) {
        case(2):    //  fall thro           //  SEGMENT DELETIONS
        case(3):
            ap->lo[ESPK_GAIN]   = -96;      //  param 4     //  attenuation of attenuated items
            ap->hi[ESPK_GAIN]   = 0;
            ap->default_val[ESPK_GAIN]  = -96;
            //  fall thro
        case(0):    //  fall thro           //  REPETITION OF SEGMENTS, OR GROUPSIZE IN SEGMENT DELETIONS
        case(4):    //  fall thro
        case(5):    //  fall thro
        case(7):    //  fall thro
        case(8):    //  fall thro
        case(6):                            //  NO OF REPETS OF DIVISION OF SEGMENT = GROUPSIZE TO ATTENUATE
            ap->lo[ESPK_REPET]  = 1;        //  param 3     //  NB ESPK_REPET = ESPK_GATED
            ap->hi[ESPK_REPET]  = 100;
            ap->default_val[ESPK_REPET] = 2;
            break;
        }
        if(!(mode == 1 || mode == 2 || mode == 3)) {
            ap->lo[ESPK_RAND]   = 0;        //  param 4
            ap->hi[ESPK_RAND]   = 1;
            ap->default_val[ESPK_RAND]  = 0;
        }
        switch(mode) {
        case(6):
            ap->lo[ESPK_WHICH]  = 1;        //  param 5     Which segment of each divided syllable is to be processed
            ap->hi[ESPK_WHICH]  = 100;
            ap->default_val[ESPK_WHICH] = 1;
            break;
        case(7):    //  fall thro
        case(8):
            ap->lo[ESPK_RATIO]  = 0.1;      //  param 5     By what ratio does the length of repeated syllables decrease
            ap->hi[ESPK_RATIO]  = 1.0;
            ap->default_val[ESPK_RATIO] = .5;
            break;
        }
    }
    switch(mode) {
    case(10):
        ap->lo[ESPK_SEED]   = 0;            //  param 3     Random seed for perms
        ap->hi[ESPK_SEED]   = 64;
        ap->default_val[ESPK_SEED]  = 0;
        break;
    case(11):
        ap->lo[ESPK_NWISE]  = 1;            //  param 3     Group-size for reversal
        ap->hi[ESPK_NWISE]  = 100;
        ap->default_val[ESPK_NWISE] = 2;
        break;
    }
    dz->maxmode = 25;
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
            if((exit_status = setup_envspeak_application(dz))<0)
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
    usage2("envspeak");
    return(USAGE_ONLY);
}

/**************************** CHECK_ENVSPEAK_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_envspeak_param_validity_and_consistency(dataptr dz)
{
//  NEEDS TO BE DONE WHEN TROFCNT IS KNOWN AND SPLICELEN IN GPSAMPS KNOWN !!!!
    int lastsamp = 0, n, k, sampsize, maxwhich;
    int chans = dz->infile->channels;
    int offset = dz->iparam[ESPK_OFFST];
    int *trof = dz->lparray[0];
    int splicelen, minseg, shorten, mintrofs_needed;
    splicelen = dz->iparam[ESPK_SPLEN] * chans;
    minseg = (splicelen * 2) + chans;
    if(dz->trofcnt == 0) {
        sprintf(errstr,"Trof array not established before testing parameters.\n");
        return PROGRAM_ERROR;
    }
    if(offset >= dz->trofcnt - 2) {
        if (offset == 0)
            sprintf(errstr,"ERROR: too few peaks found (%d).\n",dz->trofcnt);
        else
            sprintf(errstr,"ERROR: Offset (%d) too large for number of peaks found (%d).\n",offset,dz->trofcnt);
        return DATA_ERROR;
    }
    shorten = 0;
    // 2023
    for(n = 1; n <= dz->trofcnt; n++) {
        sampsize = trof[n] - lastsamp;
        if(sampsize < minseg) {
            fprintf(stdout,"Splice Length too long for some of \"syllables\".\nsampsize = %d minseg = %d trof[%d] = %d\n",sampsize,minseg,n,trof[n]);
//2023
            fprintf(stdout,"Attempting to delete too short \"syllables\".\n");
            shorten = 1;
            fflush(stdout);
            break;
        }
        lastsamp = trof[n];
    }
    if(shorten) {
        lastsamp = 0;
//2023
        for(n = 1; n <= dz->trofcnt; n++) {
            sampsize = trof[n] - lastsamp;
            while(sampsize < minseg) {
                if(n < dz->trofcnt) {
                    k = n + 1;
                    while(n < dz->trofcnt)
                        trof[n] = trof[k];
                } else
                    trof[dz->trofcnt - 1] = trof[dz->trofcnt];
                dz->trofcnt--;      //  Delete times until intertrof size big enough : decreasing trofcnt accordingly
                if(dz->trofcnt <= 0) {
                    sprintf(errstr,"No long-enough segments found, using this splicelength.\n");
                    return DATA_ERROR;
                }
            }
            lastsamp = trof[n];
        }
    }
    if(dz->mode == 2 || dz->mode == 3)
        mintrofs_needed = 1 + offset + getmaxattencnt(dz);
    else
        mintrofs_needed = 1;
    if(dz->trofcnt < mintrofs_needed) {
        sprintf(errstr,"Too few trofs found (%d) for this process and offset (%d)\n",dz->trofcnt,mintrofs_needed);
        return DATA_ERROR;
    }
    if(!(dz->mode == 10 || dz->mode == 11)) {
        if(!dz->brksize[ESPK_REPET] && dz->iparam[ESPK_REPET] <= 1) {
            switch(dz->mode) {
            case(0):    //  fall thro
            case(4):    //  fall thro
            case(5):
                fprintf(stdout, "WARNING: A repeat value of 1 or less will have no effect : only useful in a brkpoint file.\n");
                break;
            case(2):
                fprintf(stdout, "WARNING: Attenuation count value of 1 or less  as no effect : only useful in a brkpoint file.\n");
                break;
            case(3):
                fprintf(stdout, "WARNING: Attenuation count value of 1 or less will produce a silent file: only useful in a brkpoint file.\n");
                break;
            }
            fflush(stdout);
        }
    }
    switch(dz->mode) {
    case(2):    //  fall thro
    case(3):
        dz->param[ESPK_GAIN] = dbtolevel(dz->param[ESPK_GAIN]);
        break;
    case(6):
        maxwhich = getmaxwhich(dz);
        if(maxwhich > dz->iparam[ESPK_REPET]) {
            if(dz->brksize[ESPK_WHICH]) 
                sprintf(errstr,"The (maximum) chosen segment (%d) cannot be beyond the number of divisions (%d)\n",maxwhich,dz->iparam[ESPK_REPET]);
            else
                sprintf(errstr,"The chosen segment (%d) cannot be greater than the number of divisions (%d)\n",maxwhich,dz->iparam[ESPK_REPET]);
            return DATA_ERROR;
        }
        break;
    }   
    return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"envspeak"))                dz->process = ENVSPEAK;
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
    if(!strcmp(str,"envspeak")) {
        fprintf(stderr,
        "USAGE:\n"
        "envspeak envspeak 1,5,6 infile outfile wsize splice offset repet rand\n"
        "envspeak envspeak 2     infile outfile wsize splice offset\n"
        "envspeak envspeak 3-4   infile outfile wsize splice offset attencnt dbatten\n"
        "envspeak envspeak 7     infile outfile wsize splice offset div rand which [-z]\n"
        "envspeak envspeak 8-9   infile outfile wsize splice offset repet rand ratio\n"
        "envspeak envspeak 10    infile outfile wsize splice\n"
        "envspeak envspeak 11    infile outfile wsize splice seed\n"
        "envspeak envspeak 12    infile outfile wsize splice Nwise\n"
        "envspeak envspeak 13-24 AS ABOVE BUT replace \"wsize\" by \"cutsfile\"\n"
        "envspeak envspeak 25    infile outfile timesfile\n"
        "        Uses data in \"timesfile\" to remove silent gaps at indicated times.\n"
        "\n"
        "Process speech \"syllables\".\n"
        "\n"
        "Mode 1:    Repeat each syllables, \"repet\" times.\n"
        "Mode 2:    Reverse-repeat each syllable.\n"
        "Modes 3/4: (3) Attenuate N in N+1 syllabs    (4) Attenuate all except.\n"
        "Modes 5/6: (5) Repeat each syllab N times, shrinking from end  (6)  from start.\n"
        "Mode 7:    Divide each syllable into N parts, and repeat one of these N times.\n"
        "Mode 8:    For each syllab, Repeat, shortening each repetition, lopping-off end.\n"
        "Mode 9:    ditto, lopping-off start.\n"
        "Mode 10:   Extract all syllables.\n"
        "Mode 11:   Randomly reorder syllables.\n"
        "Mode 12:   Reverse order syllabs N-wise (e.g.for N=3 abc|def|ghi -> cba|fed|ihj).\n"
        "\n"
        "WSIZE     Size of envelope-search window in mS (default 50).\n"
        "CUTSFILE  List of times (apart from 0 & end) where infile cut to create syllables.\n"
        "SPLICE    Splice length in mS (default 15) \n"
        "OFFSET    Number of initial peaks to output unchanged.\n"
        "REPET     Number of repetitions of each syllable (Range 2 upwards).\n"
        "ATTENCNT  Groupsize (N) of syllabs ...(Mode 3) to attenuate (4) to NOT attenuate.\n"
        "          N means N in N+1 : so \"1\" means 1 in 2, \"3\" means 3 in 4 etc \n"
        "DBATTEN   Reduce attenuated segments by ATTEN dB: Modes 3-4. (Range -96 to < 0)\n"
        "DIV       Keep 1/DIVth part of syllable, to repeat DIV times.\n"
        "WHICH     Which syllable-fraction to keep (Range 1 to DIV)\n"
        "-z        Repeated elements do NOT grow in size (machine-like quality).\n"
        "RATIO     length of repeated elements reduces by RATIO : Range (> 0.0 to 1)\n"
        "RAND      Randomisation of lengths of repeated units.\n"
        "NWISE     Reverse order in groups of N.\n"
        "SEED      Intialisation for random order permutation (Modes 11 only).\n"
        "          If Seed > 0, using same seed again, gives IDENTICAL random output.\n"
        "\n"
        "REPET, ATTENCNT, ATTEN and WHICH may vary over time.\n");
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

/****************************** ENVSPEAK *********************************/

int envspeak(dataptr dz) 
{
    int exit_status, chans = dz->infile->channels, srate = dz->infile->srate, isrand;
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[1], *ovflwbuf = dz->sampbuf[2], *rbuf = dz->sampbuf[3];
    float temp;
    int gpsplicelen;
    int splicelen = 0, upsplice, mindur = (int)round((double)srate/MINEVENTFRQ) * chans;
    int obufpos = 0, lasttrofat = 0, thistrofat, peaklen, nupeaklen, splicend, rbufpos, upspliclen, thissplicelen;
    int *trof = dz->lparray[0];
    int mintrof, skipback, skipforward, trofdiv, offset, truepeaklen, /* truelasttrofat,*/ origpeaklen, sttseg;
    int gated = 0, n, m, i, j, k, z, nwise;
    double *splicebuf = dz->parray[0], time = 0, roffset, ratio;    
    double samps_per_sec = (double)srate * chans;
    char *outfilename;
    int namelen, numlen, *permm = NULL;
    int startcut, endcut, newendcut, /*startbuf,*/ localtrof, samps_to_write;
    int done = 0;
    if(dz->mode < 24) {
        gpsplicelen = dz->iparam[ESPK_SPLEN];
        if(dz->param[ESPK_RAND] > 0.0)
            initrand48();
        splicelen = gpsplicelen * chans;
    }
    switch(dz->mode) {
    case(0):            //  Repeat  
        mintrof = dz->iparam[ESPK_OFFST];
        for(n = 0; n <= dz->trofcnt; n++)  {
            thistrofat  = trof[n];
            peaklen = thistrofat - lasttrofat;
            splicend = splicelen - 1;
            if(n < mintrof) {           //  We must be at start of file : therefore no obufpos baktrak & no upsplice
                for(j = peaklen - 1, m = 0; m < thistrofat; m++,j--) {
                    if (j < splicelen) 
                        obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                    else                                                                    
                        obuf[obufpos] = ibuf[m];                                            //  else copy input
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
            } else {
                time = (double)thistrofat/(double)samps_per_sec;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return PROGRAM_ERROR;
                skipback = 0;
                if(lasttrofat > 0) {                //  If we're NOT at file start  
                    obufpos -= splicelen;       //  baktrak to splice to end of last segment written
                    peaklen += splicelen;       //  and length of peak is therefore one splicelen longer
                    upsplice = splicelen;
                    skipback = splicelen;
                } else
                    upsplice = 0;                   //  Prevents initial splice on start of file-segment
                splicend = splicelen - 1;
                for(k = 0, j = peaklen - 1, m = lasttrofat - skipback; m < thistrofat; m++,k++,j--) {
                    if(k < upsplice) 
                        obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                    else if (j < splicelen) 
                        obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                    else
                        obuf[obufpos] = ibuf[m];                                            //  just copy
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
                obufpos -= splicelen;
                origpeaklen = peaklen;
                if(upsplice == 0)               //  If peaklen has not already been lengthened
                    origpeaklen += splicelen;       //  do it now
                for(z = 0; z < dz->iparam[ESPK_REPET] - 1; z++) {
                    peaklen = origpeaklen;
                    thissplicelen = splicelen;
                    if(dz->param[ESPK_RAND] > 0.0) {        //  On each repet, reduce length of repet, at random (lopping bits off end)
                        do {
                            nupeaklen = randvary_pklen(origpeaklen - splicelen,dz);
                        }while (nupeaklen < 2*chans);
                        peaklen = nupeaklen + splicelen;
                        if(peaklen < splicelen * 2)
                            thissplicelen = peaklen/2;
                        else
                            thissplicelen = splicelen;
                        splicend = thissplicelen - 1;
                    }
                    splicend = splicelen - 1;
                    for(k = 0, j = peaklen - 1, m = lasttrofat - skipback; k < peaklen; m++,k++,j--) {
                        if(k < thissplicelen) 
                            obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                        else if (j < thissplicelen) 
                            obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[splicend--])); //  do downslice
                        else
                            obuf[obufpos] = ibuf[m];                                            // just copy
                        if(++obufpos >= dz->buflen * 2) {
                            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                return(exit_status);
                            memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                            memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                            obufpos -= dz->buflen;
                        }
                    }
                }
            }
            lasttrofat  = thistrofat;
        }
        break;
    case(1):            //  Repeat-reversed
        mintrof = dz->iparam[ESPK_OFFST];
        for(n = 0; n <= dz->trofcnt; n++)  {
            thistrofat  = trof[n];
            peaklen = thistrofat - lasttrofat;
            splicend = splicelen - 1;
            if(n < mintrof) {           //  Copy infile if process has not yet started
                peaklen = thistrofat;
                for(j = peaklen - 1, m = 0; m < thistrofat; m++,j--) {
                    if (j < splicelen) 
                        obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                    else
                        obuf[obufpos] = ibuf[m];                                            // or just copy
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
            } else {
                rbufpos = 0;
                skipback = 0;
                if(lasttrofat > 0) {
                    obufpos -= splicelen;
                    peaklen += splicelen;
                    upspliclen = splicelen;
                    skipback = splicelen;
                } else
                    upspliclen = 0;                     //  No initial splice
        
                splicend = splicelen - 1;
                for(k = 0, j = peaklen - 1, m = lasttrofat - skipback; m < thistrofat; m++,k++,j--) {   //  Write to both outbuf and reversing-buf
                    rbuf[rbufpos++] = ibuf[m];                      
                    if(k < upspliclen) 
                        obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                    else if (j < splicelen) 
                        obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m]  * splicebuf[splicend--]));            //  do downslice
                    else
                        obuf[obufpos] = ibuf[m];                                            //  just copy
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
                if(chans > 1) {                                                             //  Reverse channel groups in reversed buffer
                    for(m = 0; m < rbufpos; m += chans) {                                   //  IF NESS
                        for(i = 0,k = m, j = m+chans-1; i < chans/2; i++, k++,j--) {
                            temp = rbuf[j];
                            rbuf[j] = rbuf[k];
                            rbuf[k] = temp;
                        }
                    }
                }
                for(i = 0,m = 0, k = rbufpos - 1; i < rbufpos/2; i++, m++,k--) {                    //  Reverse Entire reverse-buf
                    temp = rbuf[m];
                    rbuf[m] = rbuf[k];
                    rbuf[k] = temp;
                }
                obufpos -= splicelen;
                if(upspliclen == 0)             //  If peaklen has not already been lengthened
                    peaklen += splicelen;       //  do it now
                splicend = splicelen - 1;
                for(m = 0, j = peaklen - 1; m < peaklen; m++,j--) {                         //  Write reverse-buf to outbuf
                    if(m < splicelen) 
                        obuf[obufpos] = (float)(obuf[obufpos] + (rbuf[m] * splicebuf[m]));  //  do upslice
                    else if (j < splicelen) 
                        obuf[obufpos] = (float)(obuf[obufpos] + (rbuf[m] * splicebuf[splicend--]));     //  do downslice
                    else
                        obuf[obufpos] = rbuf[m];                                            // just copy
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }               
                }
            }
            lasttrofat  = thistrofat;
        }
        break;
    case(2):    //  fall thro           Drop out peaks, in a patterned way
    case(3):
    //  PREGROUPED THE GATED AND UNGATED MATERIAL
        gated = 0;
        lasttrofat = 0;
        mintrof = dz->iparam[ESPK_OFFST];
        for(n = 0; n <= dz->trofcnt; n++)  {
            thistrofat = trof[n];
            peaklen = thistrofat - lasttrofat;
            splicend = splicelen - 1;
            if(n < mintrof) {
                for(j = peaklen - 1, m = 0; m < thistrofat; m++,j--) {
                    if (j < splicelen) 
                        obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice to attenuation level
                    else
                        obuf[obufpos] = ibuf[m];                                            // or just copy
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
            } else if((dz->mode == 2 && !gated) || (dz->mode == 3 && gated)) {
                skipback = 0;
                if(lasttrofat > 0) {                //  If we're NOT at file start  
                    obufpos -= splicelen;       //  baktrak to splice to end of last segment written
                    peaklen += splicelen;       //  and length of peak is therefore one splicelen longer
                    upsplice = splicelen;
                    skipback = splicelen;
                } else
                    upsplice = 0;                   //  Prevents initial splice on start of file-segment
                splicend = splicelen - 1;
                for(k = 0, j = peaklen - 1, m = lasttrofat - skipback; m < thistrofat; m++,k++,j--) {
                    if(k < upsplice) 
                        obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                    else if (j < splicelen) 
                        obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[splicend--])); //  do downslice
                    else
                        obuf[obufpos] = ibuf[m];                                            //  just copy
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
                gated = !gated;
            } else {
                skipback = 0;
                if(lasttrofat > 0) {                //  If we're NOT at file start  
                    obufpos -= splicelen;       //  baktrak to splice to end of last segment written
                    peaklen += splicelen;       //  and length of peak is therefore one splicelen longer
                    upsplice = splicelen;
                    skipback = splicelen;
                } else
                    upsplice = 0;               //  Prevents initial splice on start of file-segment
                splicend = splicelen - 1;
                for(k = 0, j = peaklen - 1, m = lasttrofat - skipback; m < thistrofat; m++,k++,j--) {
                    if(k < upsplice) 
                        obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] *  dz->param[ESPK_GAIN] * splicebuf[k]));  //  do upslice with attenuate
                    else if (j < splicelen) 
                        obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] *  dz->param[ESPK_GAIN] * splicebuf[splicend--])); //  do downslice with attenuate
                    else
                        obuf[obufpos] = (float)(ibuf[m] * dz->param[ESPK_GAIN]);                                        // just attenuate
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
                gated = !gated;
            }
            lasttrofat  = thistrofat;
        }
        break;
    case(4):    //  fall thro           //  Repeat  : Shrink
    case(5):
        mintrof = dz->iparam[ESPK_OFFST];
        for(n = 0; n <= dz->trofcnt; n++)  {
            thistrofat  = trof[n];
            peaklen = thistrofat - lasttrofat;
            trofdiv = (int)round((double)(peaklen/chans)/(double)dz->iparam[ESPK_REPET]) * chans;
            splicend = splicelen - 1;
            if(n < mintrof) {           //  We must be at start of file : therefore no obufpos baktrak & no upsplice
                for(j = peaklen - 1, m = 0; m < thistrofat; m++,j--) {
                    if (j < splicelen) 
                        obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                    else                                                                    
                        obuf[obufpos] = ibuf[m];                                            //  else copy input
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
            } else {
                time = (double)thistrofat/(double)samps_per_sec;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return PROGRAM_ERROR;
                skipback = 0;
                if(lasttrofat > 0) {                //  If we're NOT at file start  
                    obufpos -= splicelen;       //  baktrak to splice to end of last segment written
                    peaklen += splicelen;       //  and length of peak is therefore one splicelen longer
                    upsplice = splicelen;
                    skipback = splicelen;
                } else
                    upsplice = 0;                   //  Prevents initial splice on start of file-segment
                splicend = splicelen - 1;
                for(k = 0, j = peaklen - 1, m = lasttrofat - skipback; m < thistrofat; m++,k++,j--) {
                    if(k < upsplice) 
                        obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                    else if (j < splicelen) 
                        obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                    else
                        obuf[obufpos] = ibuf[m];                                            //  just copy
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
                obufpos -= splicelen;
                truepeaklen = peaklen;
//                truelasttrofat = lasttrofat;
                switch(dz->mode) {
                case(4):
                    for(z = 0; z < dz->iparam[ESPK_REPET] - 1; z++) {
                        peaklen = truepeaklen;
                        peaklen -= trofdiv;         //  On each repet, reduce length of repeat by loppibf bits of END of seg
                        truepeaklen -= trofdiv;     //  Advance and Remember UNrandomised peaklen
                        if(dz->param[ESPK_RAND] > 0.0) {
                            roffset = drand48() - 0.5;      //      Rand range max +- 1/2 peaklen;
                            roffset *= dz->param[ESPK_RAND];
                            offset = (int)round(roffset * ((peaklen - splicelen)/chans)) * chans;   //  offset is divisible by chans;
                            if(offset != 0)
                                peaklen += offset;
                        }
                        if(peaklen < splicelen * 2)
                            thissplicelen = peaklen/2;
                        else
                            thissplicelen = splicelen;
                        splicend = thissplicelen - 1;
                        for(k = 0, j = peaklen - 1, m = lasttrofat - skipback; k < peaklen; m++,k++,j--) {
                            if(k < thissplicelen) 
                                obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                            else if (j < thissplicelen) 
                                obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[splicend--])); //  do downslice
                            else
                                obuf[obufpos] = ibuf[m];                                            // just copy
                            if(++obufpos >= dz->buflen * 2) {
                                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                    return(exit_status);
                                memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                                memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                                obufpos -= dz->buflen;
                            }
                        }
                    }
                    break;
                case(5):
                    for(z = 0; z < dz->iparam[ESPK_REPET] - 1; z++) {
                        peaklen = truepeaklen;
                        peaklen -= trofdiv;
                        truepeaklen -= trofdiv;
                        lasttrofat = thistrofat - peaklen;
                        lasttrofat += trofdiv;      //  On each repet, reduce length of repeat by lopping bits off start of seg
                        offset = 0;
                        if(dz->param[ESPK_RAND] > 0.0) {
                            while(peaklen < chans * 2) {                //  Avoid negative-length segments
                                roffset = drand48() - 0.5;              //  Rand range max +- 1/2 peaklen TIMES the entered rand param (0-1)
                                roffset *= dz->param[ESPK_RAND];        //  Offset is divisible by chans;
                                offset = (int)round(roffset * ((peaklen - splicelen)/2/chans)) * chans;
                                peaklen += offset;
                            }                                           //  If peaklen shorter than upsplive+ downsplice
                        }                                               //  Shorten "thisspilcelen"
                        if(peaklen < splicelen * 2)                     //  This causes splice table to be read up to, and then down from
                            thissplicelen = peaklen/2;                  //  ony a part of it, but still gives equivalent up and down splices
                        else
                            thissplicelen = splicelen;
                        lasttrofat = thistrofat - (peaklen - thissplicelen);
                        splicend = thissplicelen - 1;
                        for(k = 0, j = peaklen - 1, m = lasttrofat; k < peaklen; m++,k++,j--) {
                            if(k < thissplicelen) 
                                obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                            else if (j < thissplicelen)
                                obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[splicend--])); //  do downslice
                            else
                                obuf[obufpos] = ibuf[m];                                            // just copy
                            if(++obufpos >= dz->buflen * 2) {
                                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                    return(exit_status);
                                memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                                memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                                obufpos -= dz->buflen;
                            }               
                        }
                    }
                    break;
                }
            }
            lasttrofat  = thistrofat;
        }
        break;
    case(6):                            //  Repeat divided element of syllables
        mintrof = dz->iparam[ESPK_OFFST];
        isrand = 0;
        for(n = 0; n <= dz->trofcnt; n++)  {
            thistrofat  = trof[n];
            peaklen = thistrofat - lasttrofat;
            trofdiv = (int)round((double)(peaklen/chans)/(double)dz->iparam[ESPK_REPET]) * chans;
            trofdiv = max(trofdiv,mindur);
            splicend = splicelen - 1;
            if(n < mintrof) {           //  We must be at start of file : therefore no obufpos baktrak & no upsplice
                for(j = peaklen - 1, m = 0; m < thistrofat; m++,j--) {
                    if (j < splicelen) 
                        obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                    else                                                                    
                        obuf[obufpos] = ibuf[m];                                            //  else copy input
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
            } else {
                time = (double)thistrofat/(double)samps_per_sec;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return PROGRAM_ERROR;
                skipback = 0;
                peaklen  = trofdiv;                     //  1/Nth part of syllable only, to be repeated N times
                origpeaklen = peaklen;
                skipforward = (dz->iparam[ESPK_WHICH] - 1) * trofdiv;       //  Finds seg to repeat
                for(z = 0; z < dz->iparam[ESPK_REPET]; z++) {
                    if(isrand)
                        peaklen = origpeaklen;
                    skipback = splicelen;
                    if((sttseg = lasttrofat + skipforward - skipback) >= 0) {
                        obufpos -= splicelen;           //  baktrak to splice to end of last segment written
                        if(dz->vflag[0]) {
                        /* HEREH */             
                            if(z == 0)
                                peaklen += splicelen;
                        } else {
                            peaklen += splicelen;       //  length of peaks increase
                            peaklen = min(peaklen,dz->insams[0]);
                        }
                        upsplice = splicelen;
                        lasttrofat = thistrofat - peaklen;
                    } else {
                        sttseg = 0;
                        upsplice = 0;                   //  Prevents initial splice on start of 1st file-segment
                    }
                    origpeaklen = peaklen;
                    isrand = 0;
                    if(dz->param[ESPK_RAND] > 0.0) {
                        roffset = drand48() - 0.5;      //      Rand range max +- 1/2 peaklen;
                        roffset *= dz->param[ESPK_RAND];
                        offset = (int)round(roffset * ((peaklen - splicelen)/chans)) * chans;   //  offset is divisible by chans;
                        peaklen += offset;
                        isrand = 1;
                    }
                    if(peaklen < splicelen * 2) {       //  This causes splice table to be read up to, and then down from
                        thissplicelen = peaklen/2;      //  ony a part of it, but still gives equivalent up and down splices
                        if(upsplice > 0)
                            upsplice = thissplicelen;
                    } else {
                        thissplicelen = splicelen;
                        if(upsplice > 0)
                            upsplice = thissplicelen;
                    }
                    splicend = splicelen - 1;
                    for(k = 0, j = peaklen - 1, m = sttseg; k < peaklen; m++,k++,j--) {
                        if(k < upsplice) 
                            obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                        else if (j < thissplicelen) 
                            obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                        else
                            obuf[obufpos] = ibuf[m];                                            //  just copy
                        if(++obufpos >= dz->buflen * 2) {
                            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                return(exit_status);
                            memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                            memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                            obufpos -= dz->buflen;
                        }
                    }
                }
            }
            lasttrofat  = thistrofat;
        }
        break;
    case(7):                            //  Repeat syllables, diminishing in length, lopping off end
        mintrof = dz->iparam[ESPK_OFFST];
        isrand = 0;
        for(n = 0; n <= dz->trofcnt; n++)  {
            thistrofat  = trof[n];
            peaklen = thistrofat - lasttrofat;
            origpeaklen = peaklen;
            ratio = dz->param[ESPK_RATIO];
            splicend = splicelen - 1;
            if(n < mintrof) {           //  We must be at start of file : therefore no obufpos baktrak & no upsplice
                for(j = peaklen - 1, m = 0; m < thistrofat; m++,j--) {
                    if (j < splicelen) 
                        obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                    else                                                                    
                        obuf[obufpos] = ibuf[m];                                            //  else copy input
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
            } else {
                time = (double)thistrofat/(double)samps_per_sec;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return PROGRAM_ERROR;
                for(z = 0; z < dz->iparam[ESPK_REPET]; z++) {
                    peaklen = origpeaklen;
                    if(lasttrofat > 0) {
                        obufpos -= splicelen;           //  baktrak to splice to end of last segment written
                        peaklen += splicelen;           //  include pre-splice
                        skipback = splicelen;
                        upsplice = splicelen;
                    } else {
                        upsplice = 0;                   //  Prevents initial splice on start of 1st file-segment
                        skipback = 0;
                    }
                    isrand = 0;
                    if(dz->param[ESPK_RAND] > 0.0) {
                        roffset = drand48() - 0.5;      //      Rand range max +- 1/2 peaklen;
                        roffset *= dz->param[ESPK_RAND];
                        offset = (int)round(roffset * ((peaklen - splicelen)/chans)) * chans;   //  offset is divisible by chans;
                        peaklen += offset;
                        isrand = 1;
                    }
                    if(peaklen < splicelen * 2) {       //  This causes splice table to be read up to, and then down from
                        thissplicelen = peaklen/2;      //  ony a part of it, but still gives equivalent up and down splices
                        if(upsplice > 0)
                            upsplice = thissplicelen;
                    } else {
                        thissplicelen = splicelen;
                        if(upsplice > 0)
                            upsplice = thissplicelen;
                    }
                    splicend = splicelen - 1;
                    for(k = 0, j = peaklen - 1, m = lasttrofat - skipback; k < peaklen; m++,k++,j--) {
                        if(k < upsplice) 
                            obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                        else if (j < thissplicelen) 
                            obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                        else
                            obuf[obufpos] = ibuf[m];                                            //  just copy
                        if(++obufpos >= dz->buflen * 2) {
                            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                return(exit_status);
                            memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                            memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                            obufpos -= dz->buflen;
                        }
                    }
                    origpeaklen = (int)round((double)(origpeaklen/chans) * ratio) * chans;
                    origpeaklen = max(origpeaklen,mindur);
                }
            }
            lasttrofat  = thistrofat;
        }
        break;
    case(8):                            //  Repeat syllables, diminishing in length, lopping off start
        mintrof = dz->iparam[ESPK_OFFST];
        isrand = 0;
        for(n = 0; n <= dz->trofcnt; n++)  {
            thistrofat  = trof[n];
            peaklen = thistrofat - lasttrofat;
            origpeaklen = peaklen;
            ratio = dz->param[ESPK_RATIO];
            splicend = splicelen - 1;
            if(n < mintrof) {           //  We must be at start of file : therefore no obufpos baktrak & no upsplice
                for(j = peaklen - 1, m = 0; m < thistrofat; m++,j--) {
                    if (j < splicelen) 
                        obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                    else                                                                    
                        obuf[obufpos] = ibuf[m];                                            //  else copy input
                    if(++obufpos >= dz->buflen * 2) {
                        if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                            return(exit_status);
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                        obufpos -= dz->buflen;
                    }
                }
            } else {
                time = (double)thistrofat/(double)samps_per_sec;
                if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                    return PROGRAM_ERROR;
                for(z = 0; z < dz->iparam[ESPK_REPET]; z++) {
                    peaklen = origpeaklen;
                    if(lasttrofat > 0) {
                        obufpos -= splicelen;           //  baktrak to splice to end of last segment written
                        peaklen += splicelen;           //  include pre-splice
                        skipback = splicelen;
                        upsplice = splicelen;
                    } else {
                        upsplice = 0;                   //  Prevents initial splice on start of 1st file-segment
                        skipback = 0;
                    }
                    isrand = 0;
                    if(dz->param[ESPK_RAND] > 0.0) {
                        roffset = drand48() - 0.5;      //      Rand range max +- 1/2 peaklen;
                        roffset *= dz->param[ESPK_RAND];
                        offset = (int)round(roffset * ((peaklen - splicelen)/chans)) * chans;   //  offset is divisible by chans;
                        peaklen += offset;
                        isrand = 1;
                    }
                    if(peaklen < splicelen * 2) {       //  This causes splice table to be read up to, and then down from
                        thissplicelen = peaklen/2;      //  ony a part of it, but still gives equivalent up and down splices
                        if(upsplice > 0)
                            upsplice = thissplicelen;
                    } else {
                        thissplicelen = splicelen;
                        if(upsplice > 0)
                            upsplice = thissplicelen;
                    }
                    splicend = splicelen - 1;
                    for(k = 0, j = peaklen - 1, m = max(0,thistrofat - peaklen); k < peaklen; m++,k++,j--) {
                        if(k < upsplice) 
                            obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                        else if (j < thissplicelen) 
                            obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                        else
                            obuf[obufpos] = ibuf[m];                                            //  just copy
                        if(++obufpos >= dz->buflen * 2) {
                            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                                return(exit_status);
                            memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                            memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                            obufpos -= dz->buflen;
                        }
                    }
                    origpeaklen = (int)round((double)(origpeaklen/chans) * ratio) * chans;
                    origpeaklen = max(origpeaklen,mindur);
                }
            }
            lasttrofat  = thistrofat;
        }
        break;
    case(9):                //  Extract syllables
        namelen = strlen(dz->wordstor[0]);
        numlen = 4;
        if((outfilename = (char *)malloc((namelen + numlen + 1) * sizeof(char)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store outfilenames.\n");
            return(MEMORY_ERROR);
        }
        for(n = 0; n <= dz->trofcnt; n++)  {
            obufpos = 0;
            if(n > 0) {
                if((exit_status = open_next_file(outfilename,n,dz)) < 0)
                    return exit_status;
            }
            thistrofat  = trof[n];
            peaklen = thistrofat - lasttrofat;
            splicend = splicelen - 1;
            if(lasttrofat > 0) {
                peaklen += splicelen;           //  include pre-splice
                upsplice = splicelen;
            } else {
                upsplice = 0;                   //  Prevents initial splice on start of 1st file-segment
            }
            if(peaklen < splicelen * 2) {       //  This causes splice table to be read up to, and then down from
                thissplicelen = peaklen/2;      //  ony a part of it, but still gives equivalent up and down splices
                if(upsplice > 0)
                    upsplice = thissplicelen;
            } else {
                thissplicelen = splicelen;
                if(upsplice > 0)
                    upsplice = thissplicelen;
            }
            dz->total_samps_written = 0;
            dz->tempsize = peaklen;
            splicend = splicelen - 1;
            for(k = 0, j = peaklen - 1, m = thistrofat - peaklen; k < peaklen; m++,k++,j--) {
                if(k < upsplice)
                    obuf[obufpos] = (float)(ibuf[m] * splicebuf[k]);    //  do upslice
                else if (j < thissplicelen) 
                    obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                else
                    obuf[obufpos] = ibuf[m];                            //  just copy
                if(++obufpos >= dz->buflen * 2) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                    memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                    obufpos -= dz->buflen;
                }
            }
            if(obufpos) {
                if((exit_status = write_samps(obuf,obufpos,dz))<0)
                    return(exit_status);
            }
            lasttrofat = thistrofat;
        }
        obufpos = 0;
        break;
    case(10):   //  fall thro                   //  Randomise order
    case(11):                                   //  N-wise permute order
        switch(dz->mode) {
        case(10):
            if(dz->iparam[ESPK_SEED] > 0)
                srand((int)dz->iparam[ESPK_SEED]);
            else
                initrand48();
            randperm(0,dz->trofcnt+1,dz);
            permm = dz->iparray[0];
            break;
        case(11):
            permm = dz->iparray[0];
            if(dz->brksize[ESPK_NWISE]) {
                time = 0.0;
                if((exit_status = read_value_from_brktable(time,ESPK_NWISE,dz))<0)
                    return exit_status;
            }
            i = 0;
            for(k = 0; k <= dz->trofcnt; k += nwise) {
                nwise = dz->iparam[ESPK_NWISE];
                z = min(k + nwise,dz->trofcnt+1);
                do {
                    z--;
                    permm[i++] = z;         //  Store reverse-ordered groups
                } while(z > k);
                if(dz->brksize[ESPK_NWISE]) {
                    time = (double)trof[k]/(double)samps_per_sec;
                    if((exit_status = read_value_from_brktable(time,ESPK_NWISE,dz))<0)
                        return exit_status;
                }
            }
            break;
        }
        for(n = 0; n <= dz->trofcnt; n++)  {
            k = permm[n];
            thistrofat  = trof[k--];
            if(k < 0)
                lasttrofat = 0;
            else
                lasttrofat = trof[k];
            peaklen = thistrofat - lasttrofat;
            splicend = splicelen - 1;
            if(lasttrofat > 0) {
                obufpos -= splicelen;           //  baktrak to splice to end of last segment written
                peaklen += splicelen;           //  include pre-splice
                upsplice = splicelen;
            } else
                upsplice = 0;                   //  Prevents initial splice on start of 1st file-segment
            if(peaklen < splicelen * 2) {       //  This causes splice table to be read up to, and then down from
                thissplicelen = peaklen/2;      //  ony a part of it, but still gives equivalent up and down splices
                if(upsplice > 0)
                    upsplice = thissplicelen;
            } else {
                thissplicelen = splicelen;
                if(upsplice > 0)
                    upsplice = thissplicelen;
            }
            splicend = splicelen - 1;
            for(k = 0, j = peaklen - 1, m = max(0,thistrofat - peaklen); k < peaklen; m++,k++,j--) {
                if(k < upsplice) 
                    obuf[obufpos] = (float)(obuf[obufpos] + (ibuf[m] * splicebuf[k]));  //  do upslice
                else if (j < thissplicelen) 
                    obuf[obufpos] = (float)(ibuf[m] * splicebuf[splicend--]);           //  do downslice
                else
                    obuf[obufpos] = ibuf[m];                                            //  just copy
                if(++obufpos >= dz->buflen * 2) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen * sizeof(float));
                    memset((char *)ovflwbuf,0,dz->buflen * sizeof(float));
                    obufpos -= dz->buflen;
                }
            }
            lasttrofat  = thistrofat;
        }
        break;
    case(24):
        obufpos = 0;            // No extra write at end
        k = 0;
        endcut = 0;
//        startbuf = 0;
        if((dz->ssampsread = fgetfbufEx(ibuf, dz->buflen,dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Can't read samples from soundfile.\n");
            return(SYSTEM_ERROR);
        }
        while(k < dz->trofcnt)    {
            localtrof = trof[k];
            if(ibuf[localtrof] != 0.0) {
                sprintf(errstr,"Time %lf is not in a zero-level part of the sound.\n",(double)trof[k]/srate);
                    return(DATA_ERROR);
            }
            while(flteq(ibuf[localtrof],0.0)) {
                localtrof--;
                if(localtrof < 0) {
                    localtrof = 0;
                    break;
                }
            }
            startcut = localtrof;
            startcut = startcut/chans;        //    Allow for multichan files
            startcut *= chans;

            localtrof = trof[k];
            while(flteq(ibuf[localtrof],0.0)) {
                localtrof++;
                if(localtrof >= dz->insams[0]) {
                    localtrof = dz->insams[0];
                    break;
                }
            }
            newendcut = localtrof;
            newendcut /= chans;
            newendcut *= chans;
            samps_to_write = startcut - endcut;
            ibuf += endcut;
            if((exit_status = write_samps(ibuf,samps_to_write,dz))<0)
                return(exit_status);
            ibuf -= endcut;
            endcut = newendcut;
            if(done)
                break;
            k++;
        }
        if(endcut < dz->insams[0]) {
            samps_to_write = dz->insams[0] - endcut;
            ibuf += endcut;
            if((exit_status = write_samps(ibuf,samps_to_write,dz))<0)
                return(exit_status);
            ibuf -= endcut;
        }
        break;
    }
    if(obufpos) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/*************************************** EXTRACT_ENV_FROM_SNDFILE **************************/

int extract_env_from_sndfile(dataptr dz)
{
    int exit_status;
    float *envptr;
    dz->envcnt = windows_in_sndfile(dz);
    if((dz->fptr[0]=(float *)malloc((dz->envcnt+20) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store envelope (2).\n");
        return(MEMORY_ERROR);
    }
    envptr = dz->fptr[0];
    dz->ssampsread = 1;
    while(dz->ssampsread > 0)   {
        if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Can't read samples from soundfile: extract_env_from_sndfile()\n");
            return(SYSTEM_ERROR);
        }
        if(sloom)
            display_virtual_time(dz->total_samps_read,dz);      
        if((exit_status = getenv_of_buffer(dz->ssampsread,&envptr,dz))<0)
            return exit_status;
    }
    dz->fptr[1] = envptr;       //  envend
    if((exit_status = sndseekEx(dz->ifd[0],0,0))<0) {
        sprintf(errstr,"Failed to return to start of file.\n");
        return SYSTEM_ERROR;
    }
    dz->total_samps_read = 0;
    return(FINISHED);
}

/************************* GETENV_OF_BUFFER [READENV] *******************************/

int getenv_of_buffer(int samps_to_process,float **envptr,dataptr dz)
{
    int  start_samp = 0;
    int envwindow_sampsize = dz->iparam[ESPK_WINSZ];
    float *ibuf = dz->sampbuf[0];
    float *env = *envptr;
    double convertor = 1.0/F_ABSMAXSAMP;
    while(samps_to_process >= envwindow_sampsize) {
        *env++       = (float) (getmaxsamp(start_samp,envwindow_sampsize,ibuf) * convertor);
        start_samp  += envwindow_sampsize;
        samps_to_process -= envwindow_sampsize;
    }
    if(samps_to_process)    /* Handle any final short buffer */
        *env++ = (float)(getmaxsamp(start_samp,samps_to_process,ibuf) * convertor);
    *envptr = env;
    return FINISHED;
}

/****************************** WINDOWS_IN_SNDFILE [GET_ENVSIZE] ******************************/
 
int windows_in_sndfile(dataptr dz)
{
    int envcnt, winsize = dz->iparam[ESPK_WINSZ];
    if(((envcnt = dz->insams[0]/winsize) * winsize)!=dz->insams[0])
        envcnt++;
    return envcnt;
}

/*************************** GETMAXSAMP *****************************/

double getmaxsamp(int startsamp, int sampcnt,float *buffer)
{
    int  i, endsamp = startsamp + sampcnt;
    double thisval, thismaxsamp = 0.0;
    for(i = startsamp; i<endsamp; i++) {
        if((thisval =  fabs(buffer[i]))>thismaxsamp)           
            thismaxsamp = thisval;
    }
    return thismaxsamp;
}

/*************************** ENVTROFSGET *************************/
 
int envtrofsget(dataptr dz)
{
    int /*done,*/ n, k, m;
    int upwards, troffcnt = 0, thistrofat, lasttrofat, envcnt, peaklen, ovflw;
    int *trof;
    float *p, *q;
    int envwindow_sampsize = dz->iparam[ESPK_WINSZ];
    int minseglen = (dz->iparam[ESPK_SPLEN] * 2) * dz->infile->channels;        //  Minimum segment to cut is larger than 2 splices.
    float *env, *envend;

    env = dz->fptr[0];
    envend = dz->fptr[1];
                    //   2 gpsplices     * chans;
    p = env+1;
    q = env;
    if (*p > *q)
        upwards = TRUE;
    else
        upwards = FALSE;
    troffcnt  = 0;
    lasttrofat = 0;
    envcnt = 0;
    while(p < envend) {
        if(upwards) {
            if(*p < *q) {
                upwards = FALSE;
            }
        } else {
            if(*p > *q) {                                               //  Peak-segments (separated by trofs)
                if(istrof(env,envend,q,ENVSPEAK_PKSRCHWIDTH)) {
                    thistrofat = envcnt * envwindow_sampsize;
                    peaklen = thistrofat - lasttrofat;
                    if(peaklen > minseglen) {                           //  Peak-segments must be longer than 2 splices
                        troffcnt++;                                     //  This also skips getting a trof AT 0 time
                        lasttrofat = thistrofat;
                    }
                }
                upwards = TRUE;
            }
        }
        p++;
        q++;
        envcnt++;
    }
    if(dz->mode == 10 || dz->mode == 11) {
        if((dz->iparray = (int **)malloc(sizeof(int *))) == NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store peak-locations.\n");
            return(MEMORY_ERROR);
        }
        if((dz->iparray[0] = (int *)malloc((troffcnt + 1) * sizeof(int))) == NULL) {//  Storage for permed or swapped syllable order
            sprintf(errstr,"INSUFFICIENT MEMORY to store peak-locations.\n");
            return(MEMORY_ERROR);
        }
    }
    trof = dz->lparray[0];
    dz->trofcnt = troffcnt;
    envcnt = 0;
    p = env+1;
    q = env;    
    if (*p > *q)
        upwards = TRUE;
    else
        upwards = FALSE;
    troffcnt = 0;
    lasttrofat = 0;
    envcnt = 0;
    while(envcnt < dz->envcnt) {
        if(upwards) {
            if(*p < *q) {
                upwards = FALSE;
            }
        } else {
            if(*p > *q) {                                               //  Peak-segments (separated by trofs)
                if(istrof(env,envend,q,ENVSPEAK_PKSRCHWIDTH)) {
                    thistrofat = envcnt * envwindow_sampsize;
                    peaklen = thistrofat - lasttrofat;
                    if(peaklen > minseglen) {
                        trof[troffcnt++] = envcnt * dz->iparam[ESPK_WINSZ]; //  Peak-segments must be longer than 2 splices
                        lasttrofat = thistrofat;                            //  This also skips getting a trof AT 0 time
                    }
                }
                upwards = TRUE;
            }
        }
        p++;
        q++;
        envcnt++;
    }
    trof[troffcnt] = dz->insams[0];     //  Add trof at EOF

    if(dz->mode == 2 || dz->mode == 3) {            //  PREGROUP (i.e. shuffle-down overwrite) THE SETS OF ATTENUATED SEGMENTS
//        done = 0;                                   //                                                  Say we hape attenSTEP = 3
        for(n = 0; n < dz->trofcnt; n+=2) {         //  At every trof value                             To merge (for attenuating) N segments, Shuffle down N-1 times
            if(n < dz->iparam[ESPK_OFFST])          //                                                  (last time tx is EOF and is AT trofcnt) 
                continue;                           //  To delete N trofs, shuffle down N-1 timings     t0  t1  t2  t3  t4  t5  t6  t7  (tx)  
            k = n + 1;                              //                                                  t0  MERGED  t3  t4  t5  t6  t7  (tx)  
            m = n + dz->iparam[ESPK_GATED];         //                                                  t0  t3  t4  t5  t6  t7 (tx)  REDUCE trofcnt by N-1 
            if((ovflw = m - dz->trofcnt) >= 0) {    //                                                                                          
                trof[k] = trof[dz->trofcnt];        //                                                  If at array end 
                dz->trofcnt -= (dz->iparam[ESPK_GATED] - ovflw);    //                                  t1  t2  t3  t4 (tx) OR  t1  t2  t3  (tx)        OR  t1  t2 (tx)
//                done = 1;                           //                                                      n   k       m           n   k         m             n   k    -  m
                break;                              //                                                  Copy(tx) into k :
            } else {                                //                                                  OVFLW = m - End = 0     OVFLW = m - End = 1     OVFLW = m - End = 2
                for(/* k, m */; m <= dz->trofcnt; k++,m++) {                //                                  Reduce trofcnt by 0     Reduce trofcnt by 1     Reduce trofcnt by 2
                    trof[k] = trof[m];        //RWD can hide unused vars above
                }
                dz->trofcnt -= dz->iparam[ESPK_GATED] - 1;      //  DEFAULT
            }
        }
    }
    if(dz->trofcnt <= 0) {
        sprintf(errstr,"FAILED TO FIND ANY ENVELOPE TROUGHS IN THE FILE.\n");
        return DATA_ERROR;
    }
    return(FINISHED);
}

/*************************** CREATE_SNDBUFS_FOR_ENVEL **************************
 *
 *  Only AFTER params have been read.
 */

int create_sndbufs_for_envel(dataptr dz)
{
    int bigbufsize;
    int n;
    /* All other cases */
    if(dz->bufcnt < 4) {
        sprintf(errstr,"bufcnt too low : must be at least 4.\n");
        return(PROGRAM_ERROR);
    }
    if(dz->sbufptr == 0 || dz->sampbuf == 0) {
        sprintf(errstr,"buffer pointers not allocated: create_sndbufs_for_envel()\n");
        return(PROGRAM_ERROR);
    }
    dz->buflen = dz->insams[0];
    bigbufsize = dz->buflen * (dz->bufcnt + 1) * sizeof(float);   //RWD ?? surely we need this further buf

    if((dz->bigbuf = (float *)malloc((bigbufsize))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(MEMORY_ERROR);
    }
    
    {  // RWD 13/23: debugging, looking for use of uninitialized memory
        unsigned int fbufsize = bigbufsize / sizeof(float);
        int i;
        for(i=0; i < fbufsize;i++)
            dz->bigbuf[i] = 0.0f;  // set to 1.0 to show anomalies
    }
/* RWD 13/23 */
    for(n=0;n<=dz->bufcnt;n++)               //  Leave making reversing buf till max size assessed
        dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);
    //dz->sbufptr[n] = dz->sampbuf[n] = dz->bigbuf + (dz->buflen * n);

    return(FINISHED);
}

/*************************** ISTROF **********************************/
 
int istrof(float *env,float *envend,float *q,int width)
{
    int up, down;
    float *upq, *downq, *r;
    if(width<2)
        return(TRUE);
    down = up = width/2;
    if(EVEN(width))
        down = up - 1;      /* set search params above and below q */
    downq = q - down;
    upq   = q + up;
    upq   = min(envend-1,upq);  /* allow for ends of envelope table */
    downq = max(env,downq);
    for(r = downq; r<=upq; r++) {
        if(*q > *r)
            return(FALSE);
    }
    return(TRUE);           /* if r is minimum of all in peak, return 1 */
 }
 
/*************************** PRECALCULATE_PEAKS_ARRAY_AND_SPLICE **********************************/
 
int precalculate_peaks_array_and_splice(dataptr dz)
{
    int exit_status, chans = dz->infile->channels;
    int gpsplicelen, splicelen, n, m, k;
    double splicincr, *splicebuf;
    int srate = dz->infile->srate;

    dz->iparam[ESPK_SPLEN] = (int)round(dz->param[ESPK_SPLEN] * MS_TO_SECS * (double)srate);
    gpsplicelen = dz->iparam[ESPK_SPLEN];
    splicelen = gpsplicelen * chans;
    splicincr = 1.0/(double)gpsplicelen;
    if((dz->parray[0] = (double *)malloc((splicelen * sizeof(double)))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create splice buffer (2).\n");
        return(MEMORY_ERROR);
    }
    splicebuf = dz->parray[0];
    for(n= 0, m = 0;n < gpsplicelen; n++, m += chans) {
        for(k = 0; k < chans; k++)
            splicebuf[m+k] = (double)n * splicincr;
    }
    if(dz->mode < 12) {
        dz->iparam[ESPK_WINSZ] = (int)round(dz->param[ESPK_WINSZ] * MS_TO_SECS * (double)srate) * chans;
        if((exit_status = extract_env_from_sndfile(dz))< 0)
            return exit_status;
        if((dz->lparray=(int **)malloc(sizeof(int *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store peaks.\n");
            return(MEMORY_ERROR);
        }
        if((dz->lparray[0]=(int *)malloc(dz->envcnt * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store peaks (2).\n");
            return(MEMORY_ERROR);
        }
        if((exit_status = envtrofsget(dz))< 0)
            return exit_status;
    } else {
        dz->ssampsread = 1;
        while(dz->ssampsread > 0)   {
            if((dz->ssampsread = fgetfbufEx(dz->sampbuf[0], dz->buflen,dz->ifd[0],0)) < 0) {
                sprintf(errstr,"Can't read samples from soundfile: getcutdata()\n");
                return(SYSTEM_ERROR);
            }
        }
    }
    return FINISHED;
}

/*************************** GETMAXATTENCNT **********************************/
 
int getmaxattencnt(dataptr dz)
{
    int n;
    int maxatten = -1;
    if(dz->brksize[ESPK_GATED] == 0)
        return dz->iparam[ESPK_GATED];
    else {
        for(n = 0; n < dz->brksize[ESPK_GATED]; n++)
            maxatten = max(maxatten,(int)round(dz->brk[ESPK_GATED][n]));
    }
    return maxatten;
}

/****************************** RANDVARY_PKLEN ******************************/

int randvary_pklen(int peaklen,dataptr dz)

{
    int varyrange, variance;
    int chans = dz->infile->channels;
    varyrange = (int)round((double)(peaklen/chans) * dz->param[ESPK_RAND]); //
    variance = (int)floor(drand48() * varyrange);       //
    if(variance == 0)                                   //  |----------peaklen-------------|
        return 0;                                       //                      |--vrange--|
    peaklen -= variance * chans;                        //                          |-vnce-|
    return peaklen;                                     //  |-------nupeaklen-------|
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

/*************************** GETMAXWHICH **********************************/
 
int getmaxwhich(dataptr dz)
{
    int n;
    int maxwhich = -1;
    if(dz->brksize[ESPK_WHICH] == 0)
        return dz->iparam[ESPK_WHICH];
    else {
        for(n = 0; n < dz->brksize[ESPK_WHICH]; n++)
            maxwhich = max(maxwhich,(int)round(dz->brk[ESPK_WHICH][n]));
    }
    return maxwhich;
}

/*************************** OPEN_NEXT_FILE **********************************/
 
int open_next_file(char *outfilename,int n,dataptr dz)
{
    int exit_status;
    char prefix_units[]     = "_00";
    char prefix_tens[]      = "_0";
    char prefix_hundreds[]  = "_";
    if(n > 0) {
        if((exit_status = headwrite(dz->ofd,dz))<0) {
            free(outfilename);
            return(exit_status);
        }
    }
    if(sndcloseEx(dz->ofd) < 0) {
        fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
        fflush(stdout);
    }
    dz->ofd = -1;
    strcpy(outfilename,dz->wordstor[0]);
    if(!sloom) {
        if(n<10)
            insert_new_chars_at_filename_end(outfilename,prefix_units);
        else if(n<100)
            insert_new_chars_at_filename_end(outfilename,prefix_tens);
        else if(n<1000)
            insert_new_chars_at_filename_end(outfilename,prefix_hundreds);
        else {
            sprintf(errstr,"Too many duplicates.\n");
            return(PROGRAM_ERROR);
        }
        insert_new_number_at_filename_end(outfilename,n,0);
    } else {
        insert_new_number_at_filename_end(outfilename,n,1);
    }
    if((exit_status = create_sized_outfile(outfilename,dz))<0) {
        sprintf(errstr, "Cannot open outfile Soundfile %s\n",outfilename);
        return(SYSTEM_ERROR);
    }
    return FINISHED;
}

/********************************* RANDPERM *************************************/

void randperm(int z,int setlen,dataptr dz)
{
    int n, t;
    for(n=0;n<setlen;n++) {
        t = (int)floor(drand48() * (n+1));
        if(t>=n)
            hprefix(z,n,setlen,dz);
        else
            hinsert(z,n,t,setlen,dz);
    }
}

/***************************** HINSERT **********************************
 *
 * Insert the value m AFTER the T-th element in iparray[].
 */

void hinsert(int z,int m,int t,int setlen,dataptr dz)
{   
    hshuflup(z,t+1,setlen,dz);
    dz->iparray[z][t+1] = m;
}

/***************************** HPREFIX ************************************
 *
 * Insert the value m at start of the permutation iparray[].
 */

void hprefix(int z,int m,int setlen,dataptr dz)
{
    hshuflup(z,0,setlen,dz);
    dz->iparray[z][0] = m;
}

/****************************** HSHUFLUP ***********************************
 *
 * move set members in iparray[] upwards, starting from element k.
 */

void hshuflup(int z,int k,int setlen,dataptr dz)
{   
    int n, *i;
    int y = setlen - 1;
    i = (dz->iparray[z]+y);
    for(n = y;n > k;n--) {
        *i = *(i-1);
        i--;
    }
}

/******************************** GETCUTDATA ***********************/

int getcutdata(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    aplptr ap = dz->application;
    int troffcnt, linecnt, chans = dz->infile->channels, srate = dz->infile->srate;
    char temp[2000], *q, *filename = (*cmdline)[0];
    int *trof;
    double *p, dummy;
    ap->data_in_file_only = TRUE;
    ap->special_range     = TRUE;
    ap->min_special       = 0;
    ap->max_special       = dz->duration;

    if((dz->fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Cannot open pattern datafile %s\n",filename);
        return(DATA_ERROR);
    }
    p = &dummy;
    linecnt = 0;
    troffcnt = 0;
    while(fgets(temp,200,dz->fp)!=NULL) {
        q = temp;
        if(is_an_empty_line_or_a_comment(temp)) {
            linecnt++;
            continue;
        }
        while(get_float_from_within_string(&q,p)) {
            if(*p < ap->min_special || *p > ap->max_special) {
                sprintf(errstr,"Cut position (%lf) out of range (%lf to %lf): file %s : line %d\n",
                    *p,ap->min_special,ap->max_special,filename,linecnt+1);
                return(DATA_ERROR);
            }
            troffcnt++;
        }
        linecnt++;
    }
    if(troffcnt == 0) {
        if(fclose(dz->fp)<0) {
            fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
            fflush(stdout);
        }
        sprintf(errstr,"No data found in file %s\n",filename);
        return(DATA_ERROR);
    }
    if((dz->lparray=(int **)malloc(sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store trofs (1).\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[0] = (int *)malloc((troffcnt + 1) * sizeof(int))) == NULL) {        //  Storage for syllable edit-point data
        sprintf(errstr,"INSUFFICIENT MEMORY to store trofs (2).\n");
        return(MEMORY_ERROR);
    }
    if(fseek(dz->fp,0,0)< 0) {
        sprintf(errstr,"Failed to return to start of file %s.\n",filename);
        return SYSTEM_ERROR;
    }
    troffcnt = 0;
    trof = dz->lparray[0];
    while(fgets(temp,200,dz->fp)!=NULL) {
        q = temp;
        if(is_an_empty_line_or_a_comment(temp))
            continue;
        while(get_float_from_within_string(&q,p))
            trof[troffcnt++] = (int)(*p * (double)srate) * chans;
    }
    if(fclose(dz->fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
        fflush(stdout);
    }
    if(trof[troffcnt - 1] >= dz->insams[0]-chans)   //  Ignore any splice at End-of-file
        troffcnt--;                                 //  by moving it to the troffcnt spot
    else
        trof[troffcnt] = dz->insams[0];
    dz->trofcnt = troffcnt;
    (*cmdline)++;       
    (*cmdlinecnt)--;
    return FINISHED;
}
