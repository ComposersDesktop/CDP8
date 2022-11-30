/*
 * Copyright (c) 1983-2013 Trevor Wishart and Composers Desktop Project Ltd
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

// modes
#define DO_UNKNOT   0
#define DO_KNOT     1
#define KNOTCNT     2

#define KNOT_REGULAR .1     //  Difference in duration between events for the pulse to be deemed regular

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

#define celltime    scalefact

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int unknot_param_preprocess(int *patterncnt,dataptr dz);
static int setup_unknot_application(dataptr dz);
static int setup_unknot_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int open_the_outfile(dataptr dz);
static int setup_and_init_input_param_activity(dataptr dz,int tipc);
static int setup_input_param_defaultval_stores(int tipc,aplptr ap);
static int establish_application(dataptr dz);
static int initialise_vflags(dataptr dz);
static int setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int mark_parameter_types(dataptr dz,aplptr ap);
//static int get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int open_all_infiles(char **cmdline, int *patterncnt, dataptr dz);
static int open_special_infile(char *str, dataptr dz);
static int unknot(int *patterncnt,int *knotdatacnt,double **spatpos,double **spatstep,double **comboarray,int *combopatterncnt,double **outdata,dataptr dz);
static int pulse_regular(double a,double b,double minstep);
static int pulse_more(double thisstep,double maxstep,double minerror);
static int unknot_output(int *patterncnt,int knotdatacnt,double *spatpos,double *spatstep,double *comboarray,int combopatterncnt,double *outdata,dataptr dz);
static void sortcomoboarray(int combopatterncnt,double *comboarray);
static int setup_respatialisation_params(double *spatpos,double *spatstep,double *comboarray, int combopatterncnt,int respacecnt,dataptr dz);
static int GenerateKnotCombo (double *comboarray,int combono,int *patterncnt,dataptr dz);
static int IsAnIntegerInRange(char *str,int imin, int imax);
static int IsAFilename(char *str);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status, paramcnt = 0, test, OK, OK2, OK3;
    dataptr dz = NULL;
    char **cmdline, *p;
    int *patterncnt;
    int  cmdlinecnt;
    int knotdatacnt;
    double *spatpos, *spatstep, *outdata, *comboarray, dummy;
    int combopatterncnt, n;
    aplptr ap;
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
    if(argc == 1) {
        usage1();   
        return(FAILED);
    } else if(argc == 2) {
        usage2(argv[1]);    
        return(FAILED);
    }
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
    if((exit_status = setup_unknot_application(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {       // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

    ap = dz->application;

    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_unknot_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    switch(dz->mode) {
    case(KNOTCNT):  paramcnt = 0;   break;
    default:        paramcnt = 10;  break;
    }
    n = argc - 1;
    while(argv[n][0] == '-') {  //  Count backwards over any flags
        paramcnt++;             //  And add these items to number of params on cmdline
        n--;
    }
    if((dz->infilecnt = cmdlinecnt - paramcnt) <= 0) {
        fprintf(stderr,"Too few parameters on cmdline.\n");
        return(FAILED);
    }
    test = 0;
    OK = 1;
    OK2 = 1;
    OK3 = 1;
    while(test < 8) {
        if(dz->mode == KNOTCNT) {
            if(!IsAFilename(argv[n]))
                OK3 = 0;
        } else {
            switch(test) {
            case(0):
            case(1):
                p = argv[n];
                if(!get_float_from_within_string(&p,&dummy))
                    OK = 0;
                if(dummy < 0.0 || dummy > 8.0)
                    OK = 0;
                break;
            case(2):
                if(!IsAnIntegerInRange(argv[n],0,5))
                    OK2 = 0;
                break;
            case(4):
                if(!IsAnIntegerInRange(argv[n],1,256))
                    OK2 = 0;
                break;
            case(3):
            case(5):
            case(6):
            case(7):
                if(!IsAnIntegerInRange(argv[n],0,256))
                    OK2 = 0;
                break;
            }
        }
        if(!OK) {
            fprintf(stderr,"Invalid or out of range channel-range position (%s).\n\n",argv[n]);
            usage2("unknot");
            return(FAILED);
        }
        if(!OK2) {
            fprintf(stderr,"Invalid or out of range integer param (%s) (or too few integer params).\n\n",argv[n]);
            usage2("unknot");
            return(FAILED);
        }
        if(!OK3) {
            fprintf(stderr,"Invalid param (%s) (expecting file): possibly too many or too few params.\n\n",argv[n]);
            usage2("unknot");
            return(FAILED);
        }
        test++;
        n--;
        if(n < 3)
            break;
    }
    dz->ifd = NULL;             //  Prevents spurious attempts to close output soundfiles

    if((patterncnt  = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
        fprintf(stderr,"setting up input file data counters.\n");
        return(FAILED);
    }
    if((exit_status = open_all_infiles(cmdline,patterncnt,dz))<0) { 
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);  
        return(FAILED);
    }
    cmdline += dz->infilecnt;
    cmdlinecnt -= dz->infilecnt;

    // handle_outfile() = 
    if(dz->mode != KNOTCNT) {
        if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }

//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
//  handle_special_data()       redundant
 
    if(dz->mode != KNOTCNT) {
        if((exit_status = open_special_infile(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdline++;
        cmdlinecnt--;
    }

    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency....
    if((exit_status = unknot_param_preprocess(patterncnt,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    //spec_process_file =
    if(dz->mode != KNOTCNT) {
        if((exit_status = open_the_outfile(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    if((exit_status = unknot(patterncnt,&knotdatacnt,&spatpos,&spatstep,&comboarray,&combopatterncnt,&outdata,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(dz->mode != KNOTCNT) {
        if((exit_status = unknot_output(patterncnt,knotdatacnt,spatpos,spatstep,comboarray,combopatterncnt,outdata,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
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
    dz->extrabrkno = brkcnt;              
    brkcnt++;       /* Required by CDP library logic for textfile input */
    // BUT THERE ARE NO INPUTFILE brktables USED IN THIS PROCESS
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
    dz->infilecnt = -1; //  Flags 1 or more infiles
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
    if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
        sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
        return(DATA_ERROR);
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

/************************* SETUP_UNKNOT_APPLICATION *******************/

int setup_unknot_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    switch(dz->mode) {
    case(DO_UNKNOT):
    case(DO_KNOT):   exit_status = set_param_data(ap,0,8,8,"iiiiiidd"); break;
    case(KNOTCNT):   exit_status = set_param_data(ap,0,8,0,"00000000"); break;
    }
    if(exit_status < 0)
        return(FAILED);
    if(dz->mode == KNOTCNT)
        exit_status = set_vflgs(ap,"",0,"","mct",3,2,"di0");
    else    
        exit_status = set_vflgs(ap,"",0,"","mcte",4,2,"di00");
    if(exit_status < 0)
        return(FAILED);
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = ALL_FILES;
    dz->process_type    = TO_TEXTFILE;  
    dz->outfiletype     = TEXTFILE_OUT;
    return application_init(dz);    //GLOBAL
}

/************************* SETUP_UNKNOT_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_unknot_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    if(dz->mode != KNOTCNT) {
        ap->lo[KNOT_PATREP]     = 0;
        ap->hi[KNOT_PATREP]     = 256;
        ap->default_val[KNOT_PATREP] = 2;
        ap->lo[KNOT_COMBOREP]   = 0;
        ap->hi[KNOT_COMBOREP]   = 256;
        ap->default_val[KNOT_COMBOREP] = 2;
        ap->lo[KNOT_ALLREP]     = 0;
        ap->hi[KNOT_ALLREP]     = 256;
        ap->default_val[KNOT_ALLREP] = 2;
        ap->lo[KNOT_UNKNOTREP]  = 1;
        ap->hi[KNOT_UNKNOTREP]  = 256;
        ap->default_val[KNOT_UNKNOTREP] = 2;
        ap->lo[KNOT_GOALREP]    = 0;
        ap->hi[KNOT_GOALREP]    = 256;
            ap->default_val[KNOT_GOALREP] = 2;
        ap->lo[KNOT_SPACETYP]   = 0;
        ap->hi[KNOT_SPACETYP]   = 5;
        ap->default_val[KNOT_SPACETYP] = 0;
        ap->lo[KNOT_CHANA]      = 0;
        ap->hi[KNOT_CHANA]      = 8;
        ap->default_val[KNOT_CHANA] = 1;
        ap->lo[KNOT_CHANB]      = 0;
        ap->hi[KNOT_CHANB]      = 8;
        ap->default_val[KNOT_CHANB] = 1;
    }
    ap->lo[KNOT_MIN]        = FLTERR;
    ap->hi[KNOT_MIN]        = 0.1;
    ap->default_val[KNOT_MIN] = KNOT_REGULAR;
    ap->lo[KNOT_CLIP]       = 1;
    ap->hi[KNOT_CLIP]       = 16;
    ap->default_val[KNOT_CLIP] = 1;

    dz->maxmode = 3;
    put_default_vals_in_all_params(dz);
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


/**************************** UNKNOT_PARAM_PREPROCESS *****************************/

int unknot_param_preprocess(int *patterncnt,dataptr dz)
{
    int n;
    int endtimeloc, warned = 0;
    double endtime = 0.0;
    for(n=0;n<dz->infilecnt;n++) {
        endtimeloc = (patterncnt[n] - 1) * 4;
        if(n==0)
            endtime = dz->parray[n][endtimeloc];
        else if(!flteq(endtime,dz->parray[n][endtimeloc])) {
            if(!warned) {
                fprintf(stdout,"WARNING: PATTERNS DO NOT ALL END IN SYNC.\n");
                fflush(stdout);
                warned = 1;
            }
            endtime = max(endtime,dz->parray[n][endtimeloc]);
        }
    }
    dz->celltime = endtime;

    if(dz->mode == KNOTCNT) {
        dz->iparam[KNOT_PATREP]     = 0;
        dz->iparam[KNOT_COMBOREP]   = 0;
        dz->iparam[KNOT_ALLREP]     = 0;
        dz->iparam[KNOT_UNKNOTREP]  = 0;
        dz->iparam[KNOT_GOALREP]    = 0;
        dz->iparam[KNOT_SPACETYP]   = 0;
        dz->iparam[KNOT_CHANA]      = 0;
        dz->iparam[KNOT_CHANB]      = 0;
    } else {
        if(dz->param[KNOT_CHANA] < 1.0)
            dz->param[KNOT_CHANA] += 8.0;   //  Change range from 0 to 8, to 1 to < 9.0
        dz->param[KNOT_CHANA] -= 1.0;       //  Change range to 0 to < 8.0  (allows modulo-8 arithmetic)
        if(dz->param[KNOT_CHANB] < 1.0)
            dz->param[KNOT_CHANB] += 8.0;
        dz->param[KNOT_CHANB] -= 1.0;
    }
    return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"unknot"))              dz->process = UNKNOT;
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

/******************************** USAGE1 ********************************/

int usage1(void)
{
    usage2("unknot");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"unknot")) {
        fprintf(stdout,
        "USAGE: unknot unknot 1-2 inf1 inf2 [inf3 ...] "
        "   outfile combos r_pats r_combos r_all r_unknots r_goal spacetyp cha chb ....\n"
        "OR:    unknot unknot 3 inf1 inf2 [inf3 ...] ....\n "
        "[-mmin] [-cclip] [-e] [-t]\n"
        "\n"
        "Takes set of textfiles, representing rhythmic patterns, creating combined pattern,\n"
        "repeating this, gradually unknotting it to regular timepulse & spatial distribution.\n"
        "MODES:\n"
        "1) Unknots (Events gradually moved, in turn, to be time & space equidistant).\n"
        "2) Knots   (Runs process in reverse).\n"
        "3) Counts unknotting steps (No output).\n"
        "There are 5 (possible) stages to the unknotting process....\n"
        "(1)  Repeat (each) original pattern (optional).\n"
        "(2)  Repeat (each) specified combination of patterns (optional).\n"
        "(3)  Repeat the combination of all patterns (optional).\n"
        "(4)  Unknot the combined pattern.\n"
        "(5)  Repeat the unknotted pattern (optional).\n"
        "Input patterns have lines each with 4 entries \"time  MIDI  level  pos\"\n"
        "time :     must start at zero and not decrease.\n"
        "MIDI :     vals between 0 & 127, where convention is 0 indicates a silent file.\n"
        "level:     lies between >0 and 1.\n"
        "pos  :     position in an 8-channel space, range 0 to 8 (fractional values OK).\n"
        "PARAMETERS:\n"
        "COMBOS    Textfile List of pattern combos to use. (or zero, if no combos used)\n"
        "          Each line lists 2 or more patterns to combine.\n"
        "Number of repetitions of......\n"
        "R_PATS    each original pattern    (Unknot, placed at start : Knot, placed at end).\n"
        "R_COMBOS  each pattern combination.(Unknot, placed next : Knot, placed penultimate).\n"
        "R_ALL     combined initial pattern.\n"
        "R_UNKNOTS each unknotting/knotting step.\n"
        "R_GOAL    goal pattern.\n"
        "SPACETYP  Type of spatial distribution for unknotted pattern.\n"
        "  0  No spatial redistribution.        3  Rotate anticlock between channels.\n"
        "  1  Alternating between channels.     4  Rotate clock-anticlock between channels.\n"
        "  2  Rotate clockwise between chans.   5  Rotate anticlock-clock between channels.\n"
        "(Redistributions to mono, tutti etc, can be made by a mix to mono, & crossfading).\n"
        "CHA       First channel for spacetyp redistribution.\n"
        "CHB       Second channel for spacetyp redistribution.\n"
        "MIN       Event-Duration-differences read as \"zero\" (pulse \"regular\" = unknotted).\n"
        "CLIP      Number of events to reposition at each (un)knotting step (default 1).\n"
        "-e        Spatial redistribution starts at (un)knotting end.\n"
        "          (default: ....starts at (un)knotting start.)\n"
        "-t        Output data with titles and spacing to identify patterns being output.\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

/******************************** USAGE3 ********************************/

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/******************************** OPEN_ALL_INFILES ********************************/

int open_all_infiles(char **cmdline, int *patterncnt, dataptr dz)
{
    int n, linecnt, cnt, idummy;
    double lasttime, dummy;
    FILE *fp;
    char temp[400], *p;
    if((dz->parray = (double **)malloc(dz->infilecnt * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY pattern storage integer arrays.\n");
        return(MEMORY_ERROR);
    }
    for(n = 0; n < dz->infilecnt;n++) {
        if((fp = fopen(cmdline[n],"r"))==NULL) {
            sprintf(errstr,"Cannot open file %s to read pattern data. (Check usage)\n",cmdline[n]);
            return(DATA_ERROR);
        }
        patterncnt[n] = 0;
        linecnt = 1;
        lasttime = -1.0;
        while(fgets(temp,400,fp)!=NULL) {
            p = temp;
            while(isspace(*p))
                p++;
            if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
                continue;
            cnt = 0;
            while(get_float_from_within_string(&p,&dummy)) {
                switch(cnt) {
                case(0):
                    if(linecnt == 1 && dummy != 0.0) {
                        sprintf(errstr,"FILE %s does not begin at time zero\n",cmdline[n]);
                        return(DATA_ERROR);
                    }
                    if(dummy < lasttime) {
                        sprintf(errstr,"FILE %s does not advance between times %lf and %lf\n",cmdline[n],lasttime,dummy);
                        return(DATA_ERROR);
                    }
                    lasttime = dummy;
                    break;
                case(1):
                    idummy = (int)round(dummy);
                    if(idummy != dummy) {
                        sprintf(errstr,"FILE %s has non-integer MIDI value (%lf) at time %lf\n",cmdline[n],dummy,lasttime);
                        return(DATA_ERROR);
                    }
                    if(idummy < 0 || idummy > 127) {
                        sprintf(errstr,"FILE %s has out of range MIDI value (%d) at time %lf\n",cmdline[n],idummy,lasttime);
                        return(DATA_ERROR);
                    }
                    break;
                case(2):
                    if(dummy <= 0 || dummy > 1.0) {
                        sprintf(errstr,"FILE %s has out of range LEVEL value (%lf) at time %lf\n",cmdline[n],dummy,lasttime);
                        return(DATA_ERROR);
                    }
                    break;
                case(3):
                    if(dummy < 0 || dummy > 8.0) {
                        sprintf(errstr,"FILE %s has out of range POSITION value (%lf) at time %lf\n",cmdline[n],dummy,lasttime);
                        return(DATA_ERROR);
                    }
                    break;
                default: 
                    sprintf(errstr,"FILE %s has too many values on line %d\n",cmdline[n],linecnt);
                    return(DATA_ERROR);
                    break;
                }
                cnt++;
                patterncnt[n]++;
            }
            if(cnt != 4) {
                sprintf(errstr,"FILE %s has too few values on line %d (should be 4)\n",cmdline[n],linecnt);
                return(DATA_ERROR);
                break;
            }
            linecnt++;
        }
        fseek(fp,0,0);
        if((dz->parray[n] = (double *)malloc(patterncnt[n] * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for pattern %d data.\n",n+1);
            return(MEMORY_ERROR);
        }
        cnt = 0;
        while(fgets(temp,400,fp)!=NULL) {
            p = temp;
            while(isspace(*p))
                p++;
            if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
                continue;
            while(get_float_from_within_string(&p,&dummy))
                dz->parray[n][cnt++] = dummy;
        }
        patterncnt[n] /= 4; //  4 entries per line in pattern
        fclose(fp);
    }
    return FINISHED;
}

/******************************** OPEN_SPECIAL_INFILE ********************************/

int open_special_infile(char *str, dataptr dz)
{
    int linecnt, cnt, kcnt, idummy;
    double dummy;
    FILE *fp;
    char temp[400], *p;
    int *gpcnts, *gpcheck;
    if (!strcmp(str,"0")) {
        dz->itemcnt = 0;
        return FINISHED;
    }
    if((gpcheck = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for pattern checking.\n");
        return(MEMORY_ERROR);
    }
    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read pattern data.\n",str);
        return(DATA_ERROR);
    }
    linecnt = 0;
    while(fgets(temp,400,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        cnt = 0;
        while(get_float_from_within_string(&p,&dummy)) {
            if (cnt >= dz->infilecnt) {
                sprintf(errstr,"TOO MANY ITEMS (%d) IN GROUPING ON LINE %d IN FILE %s\n",cnt+1,linecnt+1,str);
                return(DATA_ERROR);
            }
            idummy = (int)round(dummy);
            if(idummy < 1 || idummy > dz->infilecnt) {
                sprintf(errstr,"PATTERN MEMBER \"%d\" OUT OF RANGE (1 - %d) IN FILE %s\n",idummy,dz->infilecnt,str);
                return(DATA_ERROR);
            }
            gpcheck[cnt] = idummy;
            kcnt = cnt-1;
            while(kcnt >= 0) {
                if(gpcheck[cnt] == gpcheck[kcnt]) { 
                    sprintf(errstr,"ITEM \"%d\" DUPLICATED IN LINE %d IN FILE %s\n",gpcheck[cnt],linecnt+1,str);
                    return(DATA_ERROR);
                }
                kcnt--;
            }
            cnt++;
        }
        if(cnt < 2) {
            sprintf(errstr,"LINE %d IN FILE %s HAS ONLT ONE ENTRY (MUST BE AT LEAST 2)\n",linecnt+1,str);
            return(DATA_ERROR);
        }
        linecnt++;
    }
    if(linecnt <= 0) {
        sprintf(errstr,"NO DATA FOUND IN FILE %s.\n",str);
        return(DATA_ERROR);
    }
    dz->itemcnt = linecnt;
    if((dz->iparray = (int **)malloc((dz->itemcnt+1) * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for pattern groupings.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[dz->itemcnt] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for pattern grouping %d\n",linecnt+1);
        return(MEMORY_ERROR);
    }
    gpcnts = dz->iparray[dz->itemcnt];
    fseek(fp,0,0);
    linecnt = 0;
    while(fgets(temp,400,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        cnt = 0;
        while(get_float_from_within_string(&p,&dummy))
            cnt++;
        gpcnts[linecnt] = cnt;
        if((dz->iparray[linecnt] = (int *)malloc(cnt * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for pattern grouping %d\n",linecnt+1);
            return(MEMORY_ERROR);
        }
        linecnt++;
    }
    fseek(fp,0,0);
    linecnt = 0;
    while(fgets(temp,400,fp)!=NULL) {
        p = temp;
        while(isspace(*p))
            p++;
        if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
            continue;
        cnt = 0;
        while(get_float_from_within_string(&p,&dummy)) {
            dz->iparray[linecnt][cnt] = (int)round(dummy);
            cnt++;
        }
        linecnt++;
    }
    fclose(fp);
    return FINISHED;
}

/******************************** UNKNOT **********************************
 *
 *  dz->parray[0] to dz->parray[dz->infilecnt-1] contain all input patterns (in sets of 4 values)
 *  patterncnt[n] contains no of LINES in pattern n (4 items per"line") so arraysizes are patterncnt[n]*4
 *  Data in arrays interleaved as as "time MIDI level position"
 *  itemcnt = number of combos (for initial play of combos)
 *  dz->iparray[dz->itemcnt] contains the sizes of each combo
 *  dz->iparray[0] to dz->iparray[itemcnt-1] contains the actual combo
 *
 *  celltime is duration of cell to be unknotted.
 */

int unknot(int *patterncnt,int *knotdatacnt,double **spat_pos,double **spat_step,double **combo_array,int *combo_patterncnt,double **out_data,dataptr dz)
{
    double *timediff, *comboarray, *outdata = NULL, *spatpos, *spatstep, *outpos;
    int comboarraysize, outdiffcnt, outtestcnt, unknotting_eventcnt, outpatterncnt, outdatacnt = 0;
    int n, m, r, k, tn, tm, tnow,  tnext, tlast, passno, smooth, combopatterncnt, lastnonzerodiff;
    int unknotcnt = 0, maxcnt, adjacent, zerocnt = 0, prezerocnt, postzerocnt, adjacentzeros, maxloc, respacecnt = 0;
    double tim_n, mid_n, lev_n, pos_n, tim_m, mid_m, lev_m, pos_m;
    double bastime, maxdiff, span, halfspan, sum;
    int spacetyp = dz->iparam[KNOT_SPACETYP];
    int writesteps = 0;

    combopatterncnt = 0;
    for(n=0;n < dz->infilecnt;n++)
        combopatterncnt += patterncnt[n] - 1;                           //  Count total number of events in combined pattern
    *combo_patterncnt = combopatterncnt;
    comboarraysize = combopatterncnt * 4;
    if((*combo_array = (double *)malloc(comboarraysize * sizeof(double)))==NULL) {      
        sprintf(errstr,"Insufficient memory for output pattern.\n");    //  Array to store combined pattern
        return(MEMORY_ERROR);
    }
    comboarray = *combo_array;
    if((timediff = (double *)malloc(combopatterncnt * sizeof(double)))==NULL) { //  Array to store timesteps in combined pattern
        sprintf(errstr,"Insufficient memory for output pattern.\n");
        return(MEMORY_ERROR);
    }
    if((*spat_step = (double *)malloc(combopatterncnt * sizeof(double)))==NULL) {   //  Array to store spatial steps, for moving pattern element
        sprintf(errstr,"Insufficient memory for output pattern.\n");
        return(MEMORY_ERROR);
    }
    spatstep = *spat_step;
    if((*spat_pos = (double *)malloc(combopatterncnt * sizeof(double)))==NULL) {    //  Array to spatial positions, as pattern elements are moved
        sprintf(errstr,"Insufficient memory for output pattern.\n");
        return(MEMORY_ERROR);
    }
    spatpos = *spat_pos;
    if((outpos = (double *)malloc(combopatterncnt * sizeof(double)))==NULL) {   //  Array to spatial positions, as pattern elements are moved
        sprintf(errstr,"Insufficient memory for output pattern.\n");
        return(MEMORY_ERROR);
    }
    k = 0;

    for(n=0;n < dz->infilecnt;n++) {                                    //  Copy all patterns into combination array
        for(m=0;m < (patterncnt[n] - 1) * 4; m++)                       //  But dropping endmarkers
            comboarray[k++] = dz->parray[n][m]; 
    }
    sortcomoboarray(combopatterncnt,comboarray);
    outdiffcnt = combopatterncnt - 1;
    tnow = 0;
    for(n= 0, tnext=4;n < outdiffcnt;n++,tnext+=4) {                    //  Caculate and store timesteps in combined pattern
        timediff[n] = comboarray[tnext] - comboarray[tnow];
        tnow = tnext;
    }
    lastnonzerodiff = outdiffcnt-1; 
    for(n = outdiffcnt-1;n >= 0;n--) {
        if(flteq(timediff[n],0.0)) {                                    //  Count zero steps (simultaneous events) in comobo-pattern
            zerocnt++;
            if(n == lastnonzerodiff)
                lastnonzerodiff--;
        }
    }
    if(outdiffcnt == zerocnt) {
        sprintf(errstr,"ALL EVENTS ARE SYNCHRONOUS: CANNOT PROCEED\n");
        return DATA_ERROR;
    }
    if(outdiffcnt - 1 == zerocnt) {
        sprintf(errstr,"ALL EVENTS BUT ONE ARE SYNCHRONOUS: CANNOT PROCEED\n");
        return DATA_ERROR;
    }
    outtestcnt = outdiffcnt - zerocnt;                  //  How many time-steps to test, when checking result of unknotting step

    for(passno = 0;passno<2;passno++) {                 //  First pass assesses number of unknot steps and hence size of array
        writesteps = 0;
        if(passno > 0) {
            tnow = 0;
            for(n= 0, tnext=4;n < outdiffcnt;n++,tnext+=4) {    //  Reset original timediff vals
                timediff[n] = comboarray[tnext] - comboarray[tnow];
                tnow = tnext;
            }
        }
        smooth     = 0;                                 //  Second pass does the unknotting
        outdatacnt = 0;                                 //  Count the size of the output array
        bastime    = 0.0;                               //  Start-time of each pattern, as unknotting takes place
        unknotcnt  = 0;                                 //  Count the number of unknotting steps
        while(!smooth) {
            maxdiff     = -1;   //  Preset maximum step between events
            maxcnt      = 0;    //  Count number of steps having the maximum value
            adjacent    = 0;    //  Count the number of steps equal-and-adjacent to the current maximum step
            zerocnt     = 0;    //  Count any run of zero-steps (event-sumultaneities)
            prezerocnt  = 0;    //  Remember number of zero-steps immediately prior to maximum step
            postzerocnt = 0;    //  Remember number of zero steps immediately after a maximum step
            adjacentzeros = 0;  //  Remember number of zero steps immediately prior to any maximum step equal-and-adjacent to the current maximum step
            maxloc      = 0;    //  Initialise location of found maximum step
            for(n = 0;n < outdiffcnt;n++) {
                if(flteq(timediff[n],0.0)) {            //  Skip, but count,  zero steps
                    zerocnt++;                          //  Count zeros
                    if(maxloc + postzerocnt == n-1)     //  If previous value was the maximum, or a zero-following-a-maximum
                        postzerocnt++;                  //  count zeros AFTER a maximum
                } else if(pulse_more(timediff[n],maxdiff,dz->param[KNOT_MIN])) {        //  If this is a maximum
                    maxdiff = timediff[n];              //  remember size
                    maxloc = n;                         //  and location
                    maxcnt = 1;                         //  And start count of number of maxima with this value
                    adjacent = 0;                       //  Number of adjacent maxima with this value (yet)
                    adjacentzeros = 0;                  //  There are not yet adjacent maxima, so no zeros pre-adjacent maxima
                    prezerocnt = zerocnt;               //  Remember Number of preceding zerosteps
                    zerocnt = 0;                        //  Reset zerocounter
                    postzerocnt = 0;                    //  No zeros AFTER the current maximum (yet)

                } else if(pulse_regular(timediff[n],maxdiff,dz->param[KNOT_MIN])) { //  If this is "equal" to the current maximum
                    if(n == maxloc+postzerocnt+1) {     //  If minimum adjacent to current min or separated from it solely by zeros
                        adjacent += postzerocnt+1;      //  count the adjacent maxima and zeros
                        adjacentzeros = prezerocnt;     //  and if zeros adjacent to first of adjacent-maxima, remember this
                        postzerocnt = 0;                //  Reset zeros AFTER current min to 0

                    } else {                            //  If new maximum is not next to previous one
                        adjacent = 0;                   //  no adjacent maxima here (yet), 
                        adjacentzeros = 0;              //  therefore no zeros adjacent to adjacent-maxima
                        prezerocnt = zerocnt;           //  Remember Number of preceding zerosteps
                    }
                    maxloc = n;                         //  Remember the time-latest of the adjacent maxima
                    maxcnt++;                           //  Count the maxima with this value
                    zerocnt = 0;                        //  Reset zerocounter
                    postzerocnt = 0;                    //  No zeros AFTER the current maximum (yet)
                } else {
                    zerocnt = 0;                        //  postzerocnt is NOT reset, as this is remembering
                }
                                                        //  any set of zeros after last maximum found.
                                                        //  prezerocnt is NOT reset, as this is remembering
                                                        //  any set of zeros prior to last maximum found.
            }
            if(maxcnt == outtestcnt) {                  //  If all steps are equivalent
                if(unknotcnt == 0) {
                    sprintf(errstr,"INITIAL PATTERN IS ALREADY UNKNOTTED\n");
                    return DATA_ERROR;
                }
                smooth = 1;                             //  process is complete.
            }
            if(!smooth) {
                if(maxloc == prezerocnt) {                          //  isolated maximum at start, (or after ALL-zero steps)
                    if(maxloc+postzerocnt+1 >= outdiffcnt) {
                        sprintf(errstr,"ERROR IN SEGMENT ACCOUNTING 1\n");
                        return(PROGRAM_ERROR);
                    }                                               //  Span time-interval and one ABOVE (ignoring intervening zero-steps)
                    span = timediff[maxloc] + timediff[maxloc+postzerocnt+1];
                    halfspan = span/2.0;                            //  Find average
                    timediff[maxloc]                = halfspan;     //  Equalise the time-differences
                    timediff[maxloc+postzerocnt+1]  = halfspan;

                } else if(maxloc == lastnonzerodiff) {              //  maximum at top edge

                    if(adjacent) {                                  //  if adjacent to others
                        maxloc -= adjacent;                         //  Find earliest maxdiff segment
                        prezerocnt = adjacentzeros;                 //  Find any zeros immediately prior to this
                    }
                                                                    //  Span time-intervals and  one BELOW (ignoring any intervening zero-steps)
                    if(maxloc-prezerocnt-1 < 0) {
                        sprintf(errstr,"ERROR IN SEGMENT ACCOUNTING 2\n");
                        return(PROGRAM_ERROR);
                    }
                    span = timediff[maxloc] + timediff[maxloc-prezerocnt-1];
                    halfspan = span/2.0;                            //  Find average
                    timediff[maxloc-prezerocnt-1]   = halfspan;     //  Equalise the time-differences
                    timediff[maxloc]                = halfspan;
                                                                    //  maximum not at edge
                } else {
                    if(maxloc+postzerocnt+1 < outdiffcnt) {         //  Span max seg and one ABOVE
                        span = timediff[maxloc] + timediff[maxloc+postzerocnt+1];
                        halfspan = span/2.0;
                        timediff[maxloc]               = halfspan;
                        timediff[maxloc+postzerocnt+1] = halfspan;
                    } else {                                        //  Else, maximum entirely followed by zeros, span max seg and one BELOW
                        if(adjacent) {                              //  if adjacent to others
                            maxloc -= adjacent;                     //  Find earliest maxdiff segment
                            prezerocnt = adjacentzeros;             //  Find any zeros immediately prior to this
                        }                                           
                        if(maxloc-prezerocnt-1 < 0) {
                            sprintf(errstr,"ERROR IN SEGMENT ACCOUNTING 3\n");
                            return(PROGRAM_ERROR);
                        }
                        span = timediff[maxloc] + timediff[maxloc-prezerocnt-1];
                        halfspan = span/2.0;
                        timediff[maxloc-prezerocnt-1] = halfspan;
                        timediff[maxloc]              = halfspan;

                    }
                }
            }
            if(unknotcnt % dz->iparam[KNOT_CLIP] == 0) {            //  Write out every Nth copy of unknotting, depending on value of KNOT_CLIP 
                writesteps++;
                switch(passno) {
                case(0):
                    outdatacnt += combopatterncnt;
                    break;
                case(1):
                    if(respacecnt) {
                        for(n = 0; n < combopatterncnt;n++) {
                            outpos[n] = spatpos[n] + 1.0;               //  Reconvert current spatial-position of each event to (0)1-8 range for output channel
                            if(outpos[n] > 8.0)
                                outpos[n] -= 8.0;
                        }
                        for(r=0;r<dz->iparam[KNOT_UNKNOTREP];r++) {     // Copy to output UNKNOTREP copies of part-unknotted stream 
                            sum = 0.0;
                            outdata[outdatacnt++] = bastime;
                            outdata[outdatacnt++] = comboarray[1];
                            outdata[outdatacnt++] = comboarray[2];
                            outdata[outdatacnt++] = outpos[0];          //  Put in new position data
                            m = 4;
                            for(n = 0; n < outdiffcnt; n++) {
                                sum += timediff[n];                     //  Increment time through the new pattern
                                outdata[outdatacnt++] = sum + bastime;  //  Offet time by start-time of this pattern
                                m++;                                    //  Copy the MIDI-level data to output
                                outdata[outdatacnt++] = comboarray[m++];
                                outdata[outdatacnt++] = comboarray[m++];
                                m++;
                                outdata[outdatacnt++] = outpos[n+1];    //  Put in new position data
                            }
                            bastime += dz->celltime;                    //  Increment to next pattern start time
                        }
                        for(n = 0; n < combopatterncnt;n++) {
                            spatpos[n] += spatstep[n];      //  Advance position towards goal position for each pattern element
                            if(spatpos[n] >= 8.0)           //  spatpos uses 0 - <8 range for modulo8 arithmetic
                                spatpos[n] -= 8.0;
                            else if(spatpos[n] < 0.0)
                                spatpos[n] += 8.0;
                        }
                    } else {                
                        for(r=0;r<dz->iparam[KNOT_UNKNOTREP];r++) {     // Copy to output UNKNOTREP copies of part-unknotted stream 
                            sum = 0.0;
                            outdata[outdatacnt++] = bastime;
                            outdata[outdatacnt++] = comboarray[1];
                            outdata[outdatacnt++] = comboarray[2];
                            outdata[outdatacnt++] = comboarray[3];
                            m = 4;
                            for(n = 0; n < outdiffcnt; n++) {
                                sum += timediff[n];                     //  Increment time through the new pattern
                                outdata[outdatacnt++] = sum + bastime;  //  Offet time by start-time of this pattern
                                m++;                                    //  Copy the MIDI-level-position data to output
                                outdata[outdatacnt++] = comboarray[m++];
                                outdata[outdatacnt++] = comboarray[m++];
                                outdata[outdatacnt++] = comboarray[m++];
                            }
                            bastime += dz->celltime;                    //  Increment to next pattern start time
                        }
                    }
                    break;
                }
            }
            unknotcnt++;
        }
        if(passno == 0) {
            if(writesteps <= 1)
                fprintf(stdout,"INFO: %d STEP ONLY: NO UNKNOTTING\n",writesteps);
            else
                fprintf(stdout,"INFO: %d STEPS IN UNKNOTTING\n",writesteps);
            fflush(stdout);
            if(dz->mode == KNOTCNT)
                return FINISHED;
            else if(writesteps <= 1) {
                fprintf(stdout,"WARNING: NO UNKNOTTING - MIN or CLIP TOO LARGE?? - HENCE NO RESPATIALISING.\n");
                fflush(stdout);
            }
            outdatacnt *= 4;                                //  Every counted patterns needs 4 slots
            outdatacnt *= dz->iparam[KNOT_UNKNOTREP];       //  Each pattern is output KNOT_UNKNOTREP times
            if((*out_data = (double *)malloc(outdatacnt * sizeof(double)))==NULL) {
                sprintf(errstr,"Insufficient memory to store output data.\n");
                return(MEMORY_ERROR);
            }
            outdata = *out_data;
            if(spacetyp > 0 && !dz->vflag[3] && writesteps > 1) {           //  Respatialise DURING unknotting
                respacecnt = writesteps;
                setup_respatialisation_params(spatpos,spatstep,comboarray,combopatterncnt,respacecnt,dz);
            }
        }
    }
    if(dz->mode == DO_KNOT) {
        unknotting_eventcnt = outdatacnt/4;
        m = unknotting_eventcnt - 1;
        n = 0;
        outpatterncnt = writesteps * dz->iparam[KNOT_UNKNOTREP];        //  Number of patterns in unknotting output
        m = outpatterncnt - 1;
        while(m > n) {                              //  INVERT ARRAY in groups of patterns
            tm = m * combopatterncnt * 4;           //  Indeces of time at start of each pattern
            tn = n * combopatterncnt * 4;
            for(k = 0; k < combopatterncnt;k++) {   //  Progressing through one entire pattern
                tim_m = outdata[tm];                
                mid_m = outdata[tm+1];
                lev_m = outdata[tm+2];
                pos_m = outdata[tm+3];

                tim_n = outdata[tn];
                mid_n = outdata[tn+1];
                lev_n = outdata[tn+2];
                pos_n = outdata[tn+3];

                outdata[tm]   = tim_n;              //  Exchange data of the two patterns
                outdata[tm+1] = mid_n;
                outdata[tm+2] = lev_n;
                outdata[tm+3] = pos_n;

                outdata[tn]   = tim_m;
                outdata[tn+1] = mid_m;
                outdata[tn+2] = lev_m;
                outdata[tn+3] = pos_m;
                tm += 4;                    
                tn += 4;
            }
            m--;
            n++;
        }
        bastime = 0.0;
        tlast = 0;
        outpatterncnt = writesteps * dz->iparam[KNOT_UNKNOTREP];        //  Number of patterns in unknotting output
        for(n = 0,r = 0; n < outpatterncnt;n++,r+=combopatterncnt) {    //  For every pattern, (each pattern starts at r)
            sum  = 0.0;                                                 //  Single pattern timesum
            tnow = r * 4;                                               //  Index in outpattern of start of this pattern
            for(m=0,k=r;m<combopatterncnt-1;m++,k++) {                  //  Move time-pattern to correct basetime
                tnext = tnow+4;
                span = outdata[tnext] - outdata[tnow];
                outdata[tnow] = sum + bastime;
                sum += span;
                tnow = tnext;
            }
            outdata[tnow] = sum + bastime;
            bastime += dz->celltime;
        }
    }
    *knotdatacnt = outdatacnt;
    return(FINISHED);
}

/********************************************* UNKNOT_OUTPUT *********************************/

int unknot_output(int *patterncnt,int knotdatacnt,double *spatpos,double *spatstep,double *comboarray,int combopatterncnt,double *outdata,dataptr dz)
{
    int respace = 0, n, pcnt, r, ccnt, spacetyp = dz->iparam[KNOT_SPACETYP];
    double *thispattern, *combo, bastime, endtime, thispos, starttime;
    int combosize = 0, tim, mid, lev, pos, outpatterncnt, unknotting_eventcnt, sttim, stmid, stlev, stpos;
    char temp[400];

    for(n=0;n<dz->infilecnt;n++) {
        for(pcnt=0;pcnt < patterncnt[n]; pcnt++)
            combosize++;
    }
    combosize *= 4;
    if((combo = (double *)malloc(combosize * sizeof(double)))==NULL) {
        sprintf(errstr,"Insufficient memory to store intermediate combined-patterns.\n");
        return(MEMORY_ERROR);
    }
        
    bastime = 0.0;
    
    if(dz->mode == DO_UNKNOT) {
        if(dz->iparam[KNOT_PATREP]) {
            if(dz->vflag[2]) {
                sprintf(temp,";ORIGINAL PATTERN REPEATS\n");
                fputs(temp,dz->fp);
            }
            for(n=0;n<dz->infilecnt;n++) {
                for(r = 0; r < dz->iparam[KNOT_PATREP];r++) {
                    thispattern = dz->parray[n];
                    tim = 0;
                    mid = 1;
                    lev = 2;
                    pos = 3;
                    for(pcnt=0;pcnt < patterncnt[n] - 1; pcnt++) {      //  Don't output the endmarker events
                        sprintf(temp,"%lf\t%d\t%lf\t%lf\n",thispattern[tim] + bastime,(int)round(thispattern[mid]),thispattern[lev],thispattern[pos]);
                        if(fputs(temp,dz->fp) < 0) {
                            sprintf(errstr,"FAILED TO WRITE TO INDIVIDUAL PATTERNS OUTPUT FILE \n");
                            return SYSTEM_ERROR;
                        }
                        tim += 4;
                        mid += 4;
                        lev += 4;
                        pos += 4;
                    }
                    if(dz->vflag[2]) {
                        sprintf(temp,"\n");
                        fputs(temp,dz->fp);
                    }
                    bastime += dz->celltime;
                }
            }
        }
        if(dz->iparam[KNOT_COMBOREP]) {
            if(dz->vflag[2]) {
                sprintf(temp,";REPEATS OF COMBINATIONS OF ORIGINAL PATTERNS\n");
                fputs(temp,dz->fp);
            }
            for(ccnt = 0; ccnt < dz->itemcnt; ccnt++) {     //  For each pattern combination
                combopatterncnt = GenerateKnotCombo(combo,ccnt,patterncnt,dz);
                for(r=0;r<dz->iparam[KNOT_COMBOREP];r++) {
                    tim = 0;
                    mid = 1;
                    lev = 2;
                    pos = 3;
                    for(pcnt=0;pcnt < combopatterncnt; pcnt++) {
                        sprintf(temp,"%lf\t%d\t%lf\t%lf\n",combo[tim] + bastime,(int)round(combo[mid]),combo[lev],combo[pos]);
                        if(fputs(temp,dz->fp) < 0) {
                            sprintf(errstr,"FAILED TO WRITE PATTERN COMBINATIONS TO OUTPUT FILE \n");
                            return SYSTEM_ERROR;
                        }
                        tim += 4;
                        mid += 4;
                        lev += 4;
                        pos += 4;
                    }
                    if(dz->vflag[2]) {
                        sprintf(temp,"\n");
                        fputs(temp,dz->fp);
                    }
                    bastime += dz->celltime;
                }
            }
        }
    }
    if(dz->iparam[KNOT_ALLREP]) {                   //  For combination of all patterns
        if(dz->vflag[2]) {
            sprintf(temp,";REPEATS OF COMBINED-PATTERN\n");
            fputs(temp,dz->fp);
        }
        combopatterncnt = GenerateKnotCombo(combo,-1,patterncnt,dz);
        for(r=0;r<dz->iparam[KNOT_ALLREP];r++) {
            tim = 0;
            mid = 1;
            lev = 2;
            pos = 3;
            for(pcnt=0;pcnt < combopatterncnt; pcnt++) {
                if(dz->mode == DO_KNOT)         //  Get unknotted spatialisation
                    thispos = outdata[pos];
                else
                    thispos = combo[pos];
                sprintf(temp,"%lf\t%d\t%lf\t%lf\n",combo[tim] + bastime,(int)round(combo[mid]),combo[lev],thispos);
                if(fputs(temp,dz->fp) < 0) {
                    sprintf(errstr,"FAILED TO WRITE TOTAL-PATTERN REPETITIONS TO OUTPUT FILE \n");
                    return SYSTEM_ERROR;
                }
                tim += 4;
                mid += 4;
                lev += 4;
                pos += 4;
            }
            if(dz->vflag[2]) {
                sprintf(temp,"\n");
                fputs(temp,dz->fp);
            }
            bastime += dz->celltime;
        }
        if(dz->mode == DO_KNOT) {       //  Get unknotted spatialisation
            for(n= 0;n < combopatterncnt; n++) {
                pos = (n * 4) + 3;
                thispos = outdata[pos];


            }
        }
    }

    //  OUTPUT THE UNKNOTTING

    unknotting_eventcnt = knotdatacnt/4;
    outpatterncnt = unknotting_eventcnt/combopatterncnt;
    tim = 0;
    mid = 1;
    lev = 2;
    pos = 3;
    endtime = bastime;
    if(dz->vflag[2]) {
        sprintf(temp,";UNKNOTTING\n");
        fputs(temp,dz->fp);
    }
    for(n = 0; n < unknotting_eventcnt;n++) {
        sprintf(temp,"%lf\t%d\t%lf\t%lf\n",outdata[tim] + bastime,(int)round(outdata[mid]),outdata[lev],outdata[pos]);
        if(fputs(temp,dz->fp) < 0) {
            sprintf(errstr,"FAILED TO WRITE UNKNOTTING DATA TO OUTPUT FILE \n");
            return SYSTEM_ERROR;
        }
        if(dz->vflag[2]) {
            if(n % combopatterncnt  == combopatterncnt-1) {
                sprintf(temp,"\n");
                fputs(temp,dz->fp);
            }
        }
        tim += 4;
        mid += 4;                   //  Note that tim advances to end of unknotting array "outdata"
        lev += 4;
        pos += 4;
    }
    bastime += (dz->celltime * outpatterncnt);  //  Next event is outpatterncnt cells later

    //  IF repatialise AFTER unknotting CALCULATE RESPATIALISATION PARAMS

    if(spacetyp > 0 && dz->vflag[3] && dz->iparam[KNOT_GOALREP] > 1) {
        respace = 1;
        setup_respatialisation_params(spatpos,spatstep,comboarray,combopatterncnt,dz->iparam[KNOT_GOALREP],dz);
    }
    //  OUTPUT THE GOAL PATTERN

    if(dz->iparam[KNOT_GOALREP]) {
        if(dz->vflag[2]) {
            sprintf(temp,";GOAL PATTERN REPEATS\n");
            fputs(temp,dz->fp);
        }
        combopatterncnt = 0;
        for(n=0;n < dz->infilecnt;n++)
            combopatterncnt += patterncnt[n] - 1;   //  Count total number of events in combined pattern
        sttim = tim - (combopatterncnt * 4);        //  Set counters to start of goal pattern (in unknotting array)
        stmid = mid - (combopatterncnt * 4);
        stlev = lev - (combopatterncnt * 4);
        stpos = pos - (combopatterncnt * 4);
        starttime = outdata[sttim];
        tim = sttim;
        mid = stmid; 
        lev = stlev; 
        pos = stpos;
        for(r=0;r<dz->iparam[KNOT_GOALREP];r++) {
            for(n = 0; n < combopatterncnt;n++) {
                if(respace) {
                    thispos = spatpos[n] + 1.0;     //  Reconvert to (0)1-8 range for output channel
                    if(thispos > 8.0)
                        thispos -= 8.0;
                    sprintf(temp,"%lf\t%d\t%lf\t%lf\n",(outdata[tim] - starttime) + bastime,(int)round(outdata[mid]),outdata[lev],thispos);
                    if(fputs(temp,dz->fp) < 0) {
                        sprintf(errstr,"FAILED TO WRITE GOAL EVENT REPETITIONS TO OUTPUT FILE\n");
                        return SYSTEM_ERROR;
                    }
                    spatpos[n] += spatstep[n];      //  Advance position towards goal position for each pattern element
                    if(spatpos[n] >= 8.0)           //  spatpos uses 0 - <8 range for modulo8 arithmetic
                        spatpos[n] -= 8.0;
                    else if(spatpos[n] < 0.0)
                        spatpos[n] += 8.0;
                } else {
                    sprintf(temp,"%lf\t%d\t%lf\t%lf\n",(outdata[tim] - starttime) + bastime,(int)round(outdata[mid]),outdata[lev],outdata[pos]);
                    if(fputs(temp,dz->fp) < 0) {
                        sprintf(errstr,"FAILED TO WRITE GOAL EVENT REPETITIONS TO OUTPUT FILE\n");
                        return SYSTEM_ERROR;
                    }
                }
                tim += 4;
                mid += 4;
                lev += 4;
                pos += 4;
            }
            tim = sttim;                            //  back to start of last pattern of unknotting's "outdata"
            mid = stmid; 
            lev = stlev; 
            pos = stpos;
            if(dz->vflag[2]) {
                sprintf(temp,"\n");
                fputs(temp,dz->fp);
            }
            bastime += dz->celltime;
        }
    }
    if(dz->mode == DO_KNOT) {
        if(dz->iparam[KNOT_COMBOREP]) {
            if(dz->vflag[2]) {
                sprintf(temp,";REPEATS OF COMBINATIONS OF ORIGINAL PATTERNS\n");
                fputs(temp,dz->fp);
            }
            for(ccnt = 0; ccnt < dz->itemcnt; ccnt++) {     //  For each pattern combination
                combopatterncnt = GenerateKnotCombo(combo,ccnt,patterncnt,dz);
                for(r=0;r<dz->iparam[KNOT_COMBOREP];r++) {
                    tim = 0;
                    mid = 1;
                    lev = 2;
                    pos = 3;
                    for(pcnt=0;pcnt < combopatterncnt; pcnt++) {
                        sprintf(temp,"%lf\t%d\t%lf\t%lf\n",combo[tim] + bastime,(int)round(combo[mid]),combo[lev],combo[pos]);
                        if(fputs(temp,dz->fp) < 0) {
                            sprintf(errstr,"FAILED TO WRITE PATTERN COMBINATIONS TO OUTPUT FILE \n");
                            return SYSTEM_ERROR;
                        }
                        tim += 4;
                        mid += 4;
                        lev += 4;
                        pos += 4;
                    }
                    if(dz->vflag[2]) {
                        sprintf(temp,"bastime = %lf\n",bastime);
                        fputs(temp,dz->fp);
                    }
                    bastime += dz->celltime;
                }
            }
        }
        if(dz->iparam[KNOT_PATREP]) {
            if(dz->vflag[2]) {
                sprintf(temp,";ORIGINAL PATTERN REPEATS\n");
                fputs(temp,dz->fp);
            }
            for(n=0;n<dz->infilecnt;n++) {
                for(r = 0; r < dz->iparam[KNOT_PATREP];r++) {
                    thispattern = dz->parray[n];
                    tim = 0;
                    mid = 1;
                    lev = 2;
                    pos = 3;
                    for(pcnt=0;pcnt < patterncnt[n] - 1; pcnt++) {      //  Don't output the endmarker events
                        sprintf(temp,"%lf\t%d\t%lf\t%lf\n",thispattern[tim] + bastime,(int)round(thispattern[mid]),thispattern[lev],thispattern[pos]);
                        if(fputs(temp,dz->fp) < 0) {
                            sprintf(errstr,"FAILED TO WRITE TO INDIVIDUAL PATTERNS OUTPUT FILE \n");
                            return SYSTEM_ERROR;
                        }
                        tim += 4;
                        mid += 4;
                        lev += 4;
                        pos += 4;
                    }
                    if(dz->vflag[2]) {
                        sprintf(temp,"\n");
                        fputs(temp,dz->fp);
                    }
                    bastime += dz->celltime;
                }
            }
        }
    }
    return FINISHED;
}

/**************************************** PULSE_REGULAR ****************************************/

int pulse_regular(double a,double b,double minstep)
{
    if(fabs(a-b) < minstep)
        return 1;
    return 0;
}

/**************************************** PULSE_MORE ****************************************/

int pulse_more(double thisstep,double maxstep,double minerror)
{
    if(thisstep > maxstep + minerror)
        return 1;
    return 0;
}

/**************************************** GENERATEKNOTCOMBO ****************************************/
 // dz->iparray[dz->itemcnt] contains the sizes of each combo
 // dz->iparray[0] to dz->iparray[itemcnt-1] contains the actual combo

int GenerateKnotCombo (double *comboarray,int combono,int *patterncnt,dataptr dz)
{
    int arraysize = 0;
    int *thiscombo, *combocnts, thiscombocnt, k = 0, n, m, thispattern;
    if(combono >= 0) {
        thiscombo = dz->iparray[combono];       //  Array with list of patterns in this combination
        combocnts = dz->iparray[dz->itemcnt];   //  Array counting number of members in EACH combination
        thiscombocnt = combocnts[combono];      //  Count of members in THIS combination
        for(n = 0;n < thiscombocnt;n++) {       //  For every pattern in this combination
            thispattern = thiscombo[n];         //  Get this pattern
            thispattern--;                      //  Change to 0 to n-1 counting
            for(m=0;m < (patterncnt[thispattern] - 1) * 4; m++) {
                comboarray[k++] = dz->parray[thispattern][m];   
                arraysize++;
            }
        }
    } else {
        for(n=0;n < dz->infilecnt;n++) {        //  Copy all patterns into combination array
            for(m=0;m < (patterncnt[n] - 1) * 4; m++) {
                comboarray[k++] = dz->parray[n][m]; 
                arraysize++;
            }
        }
    }
    thiscombocnt = arraysize/4;
    sortcomoboarray(thiscombocnt,comboarray);
    return thiscombocnt;
}

/**************************************** SORTCOMOBOARRAY ****************************************/

void sortcomoboarray(int combopatterncnt,double *comboarray)
{
    int n, m, tn, tm;
    double  tim_m, mid_m,lev_m, pos_m, tim_n, mid_n, lev_n, pos_n;
    for(n=0;n < combopatterncnt - 1;n++) {                              //  Sort combined pattern into time-order
        tn = n * 4;
        tim_n = comboarray[tn];
        for(m=n+1;m < combopatterncnt;m++) {
            tm = m * 4;
            tim_m = comboarray[tm];
            if(tim_m < tim_n) {
                mid_n = comboarray[tn+1];
                lev_n = comboarray[tn+2];
                pos_n = comboarray[tn+3];

                mid_m = comboarray[tm+1];
                lev_m = comboarray[tm+2];
                pos_m = comboarray[tm+3];

                comboarray[tn]   = tim_m;
                comboarray[tn+1] = mid_m;
                comboarray[tn+2] = lev_m;
                comboarray[tn+3] = pos_m;

                comboarray[tm]   = tim_n;
                comboarray[tm+1] = mid_n;
                comboarray[tm+2] = lev_n;
                comboarray[tm+3] = pos_n;
                tim_n = tim_m;
            }
        }
    }
}

/**************************************** SETUP_RESPATIALISATION_PARAMS ****************************************/

int setup_respatialisation_params(double *spatpos,double *spatstep,double *comboarray, int combopatterncnt,int respacecnt,dataptr dz)
{
    int n, m, spactyp;
    int steps, pos;
    double final_spatial_spread = 0, final_space_step, startpos, distance, goal;

    spactyp = dz->iparam[KNOT_SPACETYP];
    if(spactyp == 1) {
        for(n=0,m=1;n<combopatterncnt;n+=2,m+=2) {  //  Assign actual goal-spatial-positions to each event
            spatpos[n] = dz->param[KNOT_CHANA];     //  Alternating
            spatpos[m] = dz->param[KNOT_CHANB];
        }
    } else {
        switch(spactyp) {                           //  Assign final spatial spread for each event
        case(2):
        case(4):
            if(dz->param[KNOT_CHANB] > dz->param[KNOT_CHANA])
                final_spatial_spread = dz->param[KNOT_CHANB] - dz->param[KNOT_CHANA];
            else if (dz->param[KNOT_CHANB] == dz->param[KNOT_CHANA])
                final_spatial_spread = 8;
            else    //  dz->param[KNOT_CHANB] < dz->param[KNOT_CHANA])
                final_spatial_spread = dz->param[KNOT_CHANB] - dz->param[KNOT_CHANA] + 8;
            break;
        case(3):
        case(5):
            if(dz->param[KNOT_CHANA] > dz->param[KNOT_CHANB])
                final_spatial_spread = dz->param[KNOT_CHANA] - dz->param[KNOT_CHANB];
            else if (dz->param[KNOT_CHANA] == dz->param[KNOT_CHANB])
                final_spatial_spread = 8;
            else    //  dz->param[KNOT_CHANA] < dz->param[KNOT_CHANB])
                final_spatial_spread = dz->param[KNOT_CHANA] - dz->param[KNOT_CHANB] + 8;
            break;
        }
        switch(spactyp) {                               //  Assign actual goal-spatial-positions for each event
        case(2):
            final_space_step = (double)final_spatial_spread/(double)(combopatterncnt-1);
            spatpos[0] = dz->param[KNOT_CHANA];
            for(n=1;n<combopatterncnt-1;n++) {
                spatpos[n] = dz->param[KNOT_CHANA] + (n * final_space_step);
                if(spatpos[n] >= 8.0)
                    spatpos[n] -= 8.0;
            }
            spatpos[combopatterncnt-1] = dz->param[KNOT_CHANB];
            break;
        case(3):
            final_space_step = (double)final_spatial_spread/(double)(combopatterncnt-1);
            spatpos[0] = dz->param[KNOT_CHANA];
            for(n=1;n<combopatterncnt-1;n++) {
                spatpos[n] = dz->param[KNOT_CHANA] - (n * final_space_step);
                if(spatpos[n] < 0.0)
                    spatpos[n] += 8.0;
            }
            spatpos[combopatterncnt-1] = dz->param[KNOT_CHANB];
            break;
        case(4):
            steps = combopatterncnt/2;
            if(steps * 2 != combopatterncnt)
                steps++;
            final_space_step = (double)final_spatial_spread/(double)steps;
            spatpos[0] = dz->param[KNOT_CHANA];
            for(n=1;n<steps;n++) {
                spatpos[n] = dz->param[KNOT_CHANA] + (n * final_space_step);
                if(spatpos[n] >= 8.0)
                    spatpos[n] -= 8.0;
            }
            spatpos[n++] = dz->param[KNOT_CHANB];
            for(m = 1;n<combopatterncnt;n++,m++) {
                spatpos[n] = dz->param[KNOT_CHANB] - (m * final_space_step);
                if(spatpos[n] < 0.0)
                    spatpos[n] += 8.0;
            }
            break;
        case(5):
            steps = combopatterncnt/2;
            if(steps * 2 != combopatterncnt)
                steps++;
            final_space_step = (double)final_spatial_spread/(double)steps;
            spatpos[0] = dz->param[KNOT_CHANA];
            for(n=1;n<steps;n++) {
                spatpos[n] = dz->param[KNOT_CHANA] - (n * final_space_step);
                if(spatpos[n] < 0.0)
                    spatpos[n] += 8.0;
            }
            spatpos[n++] = dz->param[KNOT_CHANB];
            for(m = 1;n<combopatterncnt;n++,m++) {
                spatpos[n] = dz->param[KNOT_CHANB] + (m * final_space_step);
                if(spatpos[n] >= 8.0)
                    spatpos[n] -= 8.0;
            }
            break;
        }
    }                                   //  Calculate stepsize necessary to move events to final positions by shortest route
    pos = 3;
    for(n=0;n<combopatterncnt;n++) {                        //  Each event takes shortest route to final position
        startpos = comboarray[pos] - 1;                     //  e.g. 7:0    7:4     0:7     4:7
        distance = fabs(spatpos[n] - startpos);             //  e.g. 7      3       7       3
        distance = min(distance,8.0 - distance);            //  e.g. 1      3       1       3
        if((goal = startpos + distance) >= 8.0)             //  e.g. 8      10      1       7
            goal -= 8.0;                                    //  e.g. 0      2       1       7
        if(flteq(goal,spatpos[n]))                          //      YES     NO      NO      YES
            spatstep[n] = distance/(respacecnt-1);          //      clock                   clock
        else                                                //          anticlok anticlok
            spatstep[n] = -distance/(respacecnt-1); 
        spatpos[n] = comboarray[pos] - 1;   //  Assign the initial spatial positions of each event (in 0 to chas-1 frame)
        pos += 4;
    }
    return FINISHED;
}

int IsAnIntegerInRange(char *str,int imin, int imax)    
{
    char *q, *p = str;
    int i;
    q = p + strlen(str);
    q--;
    while(q != p) {
        if(!isdigit(*q))
            return 0;
        q--;
    }
    i = atoi(str);
    if(i < imin || i > imax)
        return 0;
    return 1;
}

int IsAFilename(char *str)
{
    char *q, *p = str;
    int ispoint = 0, alpha = 0, wasbackslash = 0;
    if(isdigit(*p) || (*p == '.') || (*p == '-'))
        return 0;
    q = p + strlen(str);
    q--;
    while(q != p) {
        if(!isalnum(*q) && (*q != '-') && (*q != '_') && (*q != '.') && (*q != '\\'))
            return 0;       //  Must be aphanumeric with valid spearators
        if(isalpha(*q))
            alpha++;
        if(*q == '\\') {            // RWD removed extra brackets
            if(wasbackslash)
                return 0;   //  No double backslashes
            wasbackslash = 1;
        } else
            wasbackslash = 0;
        if(*q == '.') {     
            if(ispoint)
                return 0;   //  No more than one file-extension-marker "."
            ispoint++;
        }
        q--;
    }
    if(!alpha)              //  Must have alphabetic chars
        return 0;
    return 1;
}
