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
 
/*  cdparams_other() AND tkusage_other() NEED TO BE UPDATED
 *  standalone.h NEEDS TO BE UPDATED
 *  gobo AND gobosee NEED TO BE UPDATED
 */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <standalone.h>
#include <tkglobals.h>
#include <blur.h>
#include <filetype.h>
#include <modeno.h>
//#include <formants.h>
#include <cdpmain.h>
//#include <special.h>
#include <logic.h>
#include <globcon.h>
#include <cdpmain.h>
#include <sfsys.h>
#include <ctype.h>
#include <string.h>
#include <science.h>
#include <sfsys.h>

#ifdef unix
#define round lround
#endif

char errstr[2400];
const char* cdp_version = "6.0.2";

/* extern */ int sloom = 0;
/* extern */ int    sloombatch = 0;
/* extern */ int anal_infiles = 1;
/* extern */ int is_converted_to_stereo = -1;

/* CDP LIBRARY FUNCTIONS TRANSFERRED HERE */

static int  set_param_data(aplptr ap, int special_data,int maxparamcnt,int paramcnt,char *paramlist);
static int  set_vflgs(aplptr ap,char *optflags,int optcnt,char *optlist,
                char *varflags,int vflagcnt, int vparamcnt,char *varlist);
static int  setup_parameter_storage_and_constants(int storage_cnt,dataptr dz);
static int  initialise_is_int_and_no_brk_constants(int storage_cnt,dataptr dz);
static int  mark_parameter_types(dataptr dz,aplptr ap);
static int  establish_application(dataptr dz);
static int  application_init(dataptr dz);
static int  initialise_vflags(dataptr dz);
static int  setup_input_param_defaultval_stores(int tipc,aplptr ap);
static int  setup_and_init_input_param_activity(dataptr dz,int tipc);
static int  get_tk_cmdline_word(int *cmdlinecnt,char ***cmdline,char *q);
static int  assign_file_data_storage(int infilecnt,dataptr dz);
static int  get_the_mode_from_cmdline(char *str,dataptr dz);
static int  check_specav_param_validity_and_consistency(dataptr dz);
static int  allocate_specav_buffers(dataptr dz);
//static int  store_wordlist(char *filename,dataptr dz);

/* CDP LIB FUNCTION MODIFIED TO AVOID CALLING setup_particular_application() */

static int  parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);

/* SIMPLIFICATION OF LIB FUNC TO APPLY TO JUST THIS FUNCTION */

static int  parse_infile_and_check_type(char **cmdline,dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
static int  setup_specav_application(dataptr dz);
static int  setup_specav_param_ranges_and_defaults(dataptr dz);
static int  open_the_first_infile(char *filename,dataptr dz);
static int  handle_the_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int do_specav(dataptr dz) ;

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
/*  FILE *fp   = NULL; */
    dataptr dz = NULL;
//  char *special_data_string = NULL;
    char **cmdline = NULL;
    int  cmdlinecnt;
//    aplptr ap;
    int is_launched = FALSE;

                        /* CHECK FOR SOUNDLOOM */
//TW UPDATE
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if((sloom = sound_loom_in_use(&argc,&argv)) > 1) {
        sloom = 0;
        sloombatch = 1;
    }
    if(sflinit("cdp")){
        sfperror("cdp: initialisation\n");
        return(FAILED);
    }

                          /* SET UP THE PRINCIPLE DATASTRUCTURE */
    if((exit_status = establish_datastructure(&dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    dz->maxmode = 3;
    if(!sloom) {
                          /* INITIAL CHECK OF CMDLINE DATA */
        if((exit_status = make_initial_cmdline_check(&argc,&argv))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdline    = argv;  /* GET PRE_DATA, ALLOCATE THE APPLICATION, CHECK FOR EXTRA INFILES */
        cmdlinecnt = argc;
// get_process_and_mode_from_cmdline -->
        if(!strcmp(argv[0],"specav"))
            dz->process = SPECAV;
        else
            usage1();
        cmdline++;
        cmdlinecnt--;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
// <--
        // setup_particular_application =
        if((exit_status = setup_specav_application(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    } else {
        //parse_TK_data() =
        if((exit_status = parse_sloom_data(argc,argv,&cmdline,&cmdlinecnt,dz))<0) {     /* includes setup_particular_application()      */
            exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);/* and cmdlinelength check = sees extra-infiles */
            return(exit_status);         
        }
    }
//    ap = dz->application;

    // parse_infile_and_hone_type() = 
    if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

    if((exit_status = setup_specav_param_ranges_and_defaults(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

                    /* OPEN FIRST INFILE AND STORE DATA, AND INFORMATION, APPROPRIATELY */

    if((exit_status = open_the_first_infile(cmdline[0],dz))<0) {    
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);  
        return(FAILED);
    }
    cmdlinecnt--;
    cmdline++;
    if(dz->mode == 1) {
        if((exit_status = handle_the_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);      
            return(FAILED);
        }
    } else if(dz->infilecnt > 1) {
        sprintf(errstr,"This process requires a single input file\n");
        exit_status = DATA_ERROR;
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);      
        return(FAILED);
    }
    // handle_outfile
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency....
    if((exit_status = check_specav_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    if((exit_status = allocate_specav_buffers(dz))<0){
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // param_preprocess
    // spec_process_file
    if((exit_status = do_specav(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
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
    int tipc;
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
    //THERE ARE NO brktables USED IN THIS PROCESS
    if((storage_cnt = tipc + ap->internal_param_cnt)>0) {         
        if((exit_status = setup_parameter_storage_and_constants(storage_cnt,dz))<0)   
            return(exit_status);
        if((exit_status = initialise_is_int_and_no_brk_constants(storage_cnt,dz))<0)      
            return(exit_status);
    }                                                      
    if((exit_status = mark_parameter_types(dz,ap))<0)     
            return(exit_status);
    
    // establish_infile_constants() replaced by
        dz->infilecnt = -2; /* flags 2 or more */
    // establish_bufptrs_and_extra_buffers
    dz->extra_bufcnt =  0; 
    dz->bptrcnt = 4;    
    // setup_internal_arrays_and_array_pointers(): not required
    return establish_spec_bufptrs_and_extra_buffers(dz);
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

/************************* SETUP_SPECAV_APPLICATION *******************/

int setup_specav_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if(dz->mode == 2) {
        if((exit_status = set_param_data(ap,0   ,0,0,""      ))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,  "g",1,"i"  ,"n"    ,1,0,"0"   ))<0)
            return(FAILED);
    } else {
        if((exit_status = set_param_data(ap,0   ,2,2,"dd"      ))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,  "",0,""  ,"n"     ,1,0,"0"    ))<0)
            return(FAILED);
    }
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    if(dz->mode == 1)
        dz->input_data_type = MANY_ANALFILES;
    else
        dz->input_data_type = ANALFILE_ONLY;
    dz->process_type    = TO_TEXTFILE;  
    dz->outfiletype     = TEXTFILE_OUT;
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
            sprintf(errstr,"Failed tp parse input file %s\n",cmdline[0]);
            return(PROGRAM_ERROR);
        } else if(infile_info->filetype != ANALFILE)  {
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

/************************* SETUP_SPECAV_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_specav_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    if(dz->mode == 2) {
        ap->lo[0]           = 1;
        ap->hi[0]           = dz->wlength - 1;
        ap->default_val[0]  = 1;
    } else {
        ap->lo[0]           = dz->frametime;
        ap->hi[0]           = dz->duration - dz->frametime;
        ap->default_val[0]  = dz->frametime;
        ap->lo[1]           = dz->frametime * 2;
        ap->hi[1]           = dz->duration;
        ap->default_val[1]  = dz->duration;
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
            if((exit_status = setup_specav_application(dz))<0)
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

/************************** OPEN_THE_FIRST_INFILE *****************************/

int open_the_first_infile(char *filename,dataptr dz)
{
    if((dz->ifd[0] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
        sprintf(errstr,"Failure to open file %s for input.\n",filename);
        return(SYSTEM_ERROR);
    }
    if(dz->infilecnt<=0 || dz->infile->filetype!=ANALFILE) {
        sprintf(errstr,"%s is wrong type of file for this process.\n",filename);
        return(DATA_ERROR);
    }
    dz->samps_left = dz->insams[0];
    return(FINISHED);
}

/************************ HANDLE_THE_EXTRA_INFILES *********************/

int handle_the_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
                    /* OPEN ANY FURTHER INFILES, CHECK COMPATIBILITY, STORE DATA AND INFO */
    int  exit_status, n;
    char *filename;

    if(dz->infilecnt > 1) {
        for(n=1;n<dz->infilecnt;n++) {
            filename = (*cmdline)[0];
            if((exit_status = handle_other_infile(n,filename,dz))<0)
                return(exit_status);
            (*cmdline)++;
            (*cmdlinecnt)--;
        }
    } else {
        sprintf(errstr,"Insufficient input files for this process\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/************************ HANDLE_THE_OUTFILE *********************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz)
{
    char *filename = NULL;

    if(sloom) {
        filename = (*cmdline)[0];
    } else {
        if(*cmdlinecnt<=0) {
            sprintf(errstr,"Insufficient cmdline parameters.\n");
            return(USAGE_ONLY);
        }
        filename = (*cmdline)[0];
        if(filename[0]=='-' && filename[1]=='f') {
            sprintf(errstr,"-f flag used incorrectly on command line (output is not a sound file).\n");
            return(USAGE_ONLY);
        }
        if(file_has_invalid_startchar(filename) || value_is_numeric(filename)) {
            sprintf(errstr,"Outfile name %s has invalid start character(s) or looks too much like a number.\n",filename);
            return(DATA_ERROR);
        }
    }
    if(dz->mode == 2)
        strcpy(dz->outfilename,filename);
    else if ((dz->fp = fopen(filename,"w")) == NULL) {
        sprintf(errstr,"Cannot open output file %s\n", filename);
        return(DATA_ERROR);
    }
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/******************************** USAGE ********************************/

int usage1(void)
{
    return(usage2("specav"));
}

int usage2(char *str)
{
    if(!strcmp(str,"specav")) {
        fprintf(stdout,
        "USAGE: specav specav 1 inafil outfil starttime endtime [-n]\n"
        "OR:    \n"
        "specav specav 2 inafil1 inafil2 [inafil3 ....] outfil starttime endtime [-n]\n"
        "OR:    \n"
        "specav specav 3 inafil generic_outfilname [-ggroupcnt] [-n]\n"
        "\n"
        "FIND AVERAGE SPECTRUM OF A SOUND OR OF SEVERAL SOUNDS\n"
        "OR\n"
        "EXTRACT CHANGING SPECTRUM OF SINGLE ANALYSIS FILE AS LIST OF TEXTFILES\n"
        "\n"
        "INAFIL    Inputs are analysis files (of the same type).\n"
        "OUTFIL    Output is a textfile, or a set of textfiles.\n"
        "STARTTIME Start time of section to be averaged.\n"
        "ENDTIME   End time of section to be averaged.\n"
        "GROUPCNT  Number of windows averaged in each output textfile (default = 1).\n"
        "-n        Normalise the output data, to peak at 1.0\n"
        "(Useful to display formant contour of source).\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    sprintf(errstr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/************************* DO_SPECAV *******************/

int do_specav(dataptr dz) 
{
    int exit_status, n, k, j, outframecnt = 0, wincnt, gpcnt;
    double *avstore, maxamp, normaliser = 1.0;
    char *p, thisbasnam[200], filename[200], thisext[200], thisnum[200];
    dz->flbufptr[1]  = dz->bigfbuf;
    if((avstore = (double *)malloc(dz->buflen * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to accumulate spectra.\n");
        return(MEMORY_ERROR);
    }
    if(dz->mode == 2) {
        p = dz->outfilename;            //  Get elements of output file names
        while(*p != ENDOFSTR) {
            if(*p == '.') {
                strcpy(thisext,p);
                *p = ENDOFSTR;
                strcpy(thisbasnam,dz->outfilename);
                break;
            }
            p++;
        }
        if(*p == ENDOFSTR) {            //  If no file extension found, force ".txt"
            strcpy(thisext,".txt");
            strcpy(thisbasnam,dz->outfilename);
        }
        memset((char *)avstore,0,dz->buflen * sizeof(double));
        dz->samps_left = dz->insams[0];
        wincnt = 0;             //  Count of input windows
        gpcnt = 0;              //  Count of windows within an output-group of windows
        outframecnt = 0;        //  Count of output-groups
        if(dz->vflag[0]) {      // Find normalisation value
            maxamp = 0.0;
            memset((char *)avstore,0,dz->buflen * sizeof(double));
            while(dz->samps_left > 0) {
                memset((char *)dz->bigfbuf,0,dz->buflen * sizeof(float));
                if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
                    return exit_status;
                if(wincnt > 0) {                    //  Ignore 1st window of pvoc output (which is all 0.0)
                    for(k = 0; k < dz->ssampsread; k++)
                        avstore[k] += dz->bigfbuf[k];
                    gpcnt++;
                    if(gpcnt == dz->iparam[0]) {    //  Once a window-group is complete, find max amp value
                        for(k = 0; k < dz->buflen; k+=2) {
                            if(avstore[k] > maxamp)
                                maxamp = avstore[k];
                        }
                        gpcnt = 0;                  //  Reset avstore for next group
                        memset((char *)avstore,0,dz->buflen * sizeof(double));
                    }
                }
                wincnt++;
            }
            if(maxamp <= FLTERR) {
                sprintf(errstr,"No significant level found in source file.\n");
                return(DATA_ERROR);
            }
            normaliser = 1.0/maxamp;
            sndseekEx(dz->ifd[0],0,0);
            reset_filedata_counters(dz);
            memset((char *)avstore,0,dz->buflen * sizeof(double));
            wincnt = 0;
            gpcnt = 0;
        }
        while(dz->samps_left > 0) {
            memset((char *)dz->bigfbuf,0,dz->buflen * sizeof(float));
            if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
                return exit_status;
            if(wincnt > 0) {                        //  Ignore first (zero) window
                for(k = 0; k < dz->ssampsread; k++)
                    avstore[k] += dz->bigfbuf[k];   //  Add spectral data into averaging window
                gpcnt++;
                if(gpcnt == dz->iparam[0]) {        //  Once a full group of windows have been added in
                    if(dz->vflag[0]) {              //  Normalise
                        for(k = 0; k < dz->buflen; k+=2)
                            dz->bigfbuf[k] = (float)(avstore[k] * normaliser);
                    } else {                        //  or divide values by groupsize
                        for(k = 0; k < dz->buflen; k+=2)
                            dz->bigfbuf[k] = (float)(avstore[k] / (double)gpcnt);
                    }                               
                    gpcnt = 0;                      //  and reset group counter and averaging buffer
                    memset((char *)avstore,0,dz->buflen * sizeof(double));

                    if(outframecnt > 1)             // Once first outputfile used
                        fclose(dz->fp);             // Close it before creating next (last file closed on exit from main)
                    strcpy(filename,thisbasnam);
                    sprintf(thisnum,"%d",outframecnt+1);
                    strcat(filename,"_");           //  Create numbered filename
                    strcat(filename,thisnum);
                    strcat(filename,thisext);
                    if((dz->fp = fopen(filename,"w")) == NULL) {
                        sprintf(errstr,"Cannot open output file %s\n", filename);
                        return(DATA_ERROR);
                    }                               //  Write data to file
                    for(k = 0, j = 1; k < dz->buflen; k+=2, j+=2) {
                        if(fprintf(dz->fp,"%lf\t%lf\n",dz->bigfbuf[j],dz->bigfbuf[k])<2) {
                            sprintf(errstr,"fput_brk(): Problem writing spectral data to file %s\n",filename);
                            return(PROGRAM_ERROR);
                        }   //  i.e. store the average frq followed by the average amplitude
                    }
                    outframecnt++;
                }
            }
            wincnt++;
        }
        return FINISHED;
    }
    memset((char *)avstore,0,dz->buflen * sizeof(float));
    for(n=0;n<dz->infilecnt;n++) {
        dz->samps_left = dz->insams[n];
        dz->ifd[0] = dz->ifd[n];
        wincnt = 0;
        while(dz->samps_left > 0) {
            if((exit_status = read_samps(dz->bigfbuf,dz)) < 0)
                return exit_status;
            if(wincnt >= dz->iparam[0]) {
                for(k = 0; k < dz->ssampsread; k++)
                    avstore[k] += dz->bigfbuf[k];
                outframecnt++;
            }
            if(wincnt >= dz->iparam[1])
                break;
            wincnt++;
        }
    }
    for(k = 0, j = 1; k < dz->buflen; k+=2,j+=2) {
        avstore[k] = avstore[k]/(double)outframecnt;
        avstore[j] = avstore[j]/(double)outframecnt;
        if(avstore[j] < 0.0)
            avstore[j] = 0.0;
        else if(avstore[j] > dz->nyquist)
            avstore[j] = dz->nyquist;
    }
    if(dz->vflag[0]) {  // normalise
        maxamp = 0.0;
        for(k = 0; k < dz->buflen; k+=2)
            maxamp = max(avstore[k],maxamp);
        if((maxamp > FLTERR) && (maxamp < 1.0)) {
            for(k = 0; k < dz->buflen; k+=2)
                avstore[k] /= maxamp;
        }
    }
    for(k = 0, j = 1; k < dz->buflen; k+=2, j+=2) {
        if(fprintf(dz->fp,"%lf\t%lf\n",avstore[j],avstore[k])<2) {
            sprintf(errstr,"fput_brk(): Problem writing spectral data to file.\n");
            return(PROGRAM_ERROR);
        }   //  i.e. store the average frq followed by the average amplitude
    }   
            
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

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
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

/****************************** GET_MODE *********************************/

int get_the_mode_from_cmdline(char *str,dataptr dz)
{
    if(sscanf(str,"%d",&dz->mode)!=1) {
        sprintf(errstr,"Cannot read mode of program.\n");
        return(USAGE_ONLY);
    }
    if(dz->mode <= 0 || dz->mode > dz->maxmode) {
        sprintf(errstr,"Program mode value [%d] is out of range [1 - %d].\n",dz->mode,dz->maxmode);
        return(USAGE_ONLY);
    }
    dz->mode--;     /* CHANGE TO INTERNAL REPRESENTATION OF MODE NO */
    return(FINISHED);
}

/**************************** CHECK_SPECAV_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_specav_param_validity_and_consistency(dataptr dz)
{
    int temp, n;
    double duration;
    if(dz->mode == 2)
        return FINISHED;
    for(n = 0; n <dz->infilecnt; n++) {
        duration  = (dz->insams[n]/dz->wanted) * dz->frametime;
        if(dz->param[0] > duration)
            dz->param[0] = duration;
        if(dz->param[1] > duration)
            dz->param[1] = duration;
    }
    dz->iparam[0] = (int)round(dz->param[0]/dz->frametime);
    dz->iparam[1] = (int)round(dz->param[1]/dz->frametime);
    if(dz->iparam[1] < dz->iparam[0]) {
        temp = dz->iparam[0];
        dz->iparam[0] = dz->iparam[1];
        dz->iparam[1] = temp;
    }
    return FINISHED;
}

/**************************** ALLOCATE_SPECAV_BUFFERS ******************************/

int allocate_specav_buffers(dataptr dz)
{
//  int exit_status;
    unsigned int buffersize;
    if(dz->bptrcnt < 4) {
        sprintf(errstr,"Insufficient bufptrs established in allocate_double_buffer()\n");
        return(PROGRAM_ERROR);
    }
//TW REVISED: buffers don't need to be multiples of secsize
    buffersize = dz->wanted;
    dz->buflen = buffersize;
    if((dz->bigfbuf = (float*)malloc(dz->buflen * 2 * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for output buffer.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize = dz->buflen;
    dz->flbufptr[2]  = dz->bigfbuf + dz->buflen;      
    dz->flbufptr[3]  = dz->flbufptr[2] + dz->buflen;
    return(FINISHED);
}

