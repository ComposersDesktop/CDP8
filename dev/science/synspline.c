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

//  TEST IS
//  _cdprogs\synspline synspline test.wav 44100 2 440 7 8 2

// see HEREH for issues 
//  + still crashed beyond obufpos = 1560

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

#define MAXOUTAMP   0.95
#define CYCLENORM   0
#define is_drift is_transpos
#define maxdrift scalefact
#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int setup_synspline_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int setup_synspline_param_ranges_and_defaults(dataptr dz);
static int handle_the_outfile(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int open_the_outfile(dataptr dz);
//static int handle_the_special_data(char *str,dataptr dz);
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
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);

static int synspline(dataptr dz);
//static void incr_sinptr(int n,double time,double onehzincr,dataptr dz);
//static double read_level(int n,double time,dataptr dz);
static int create_synspline_sndbufs(dataptr dz);
//static int generate_packet_envelope (dataptr dz);
//static double read_packet_envelope(int kk,double incr,dataptr dz);
//static int modify_packet_envelope(dataptr dz);
//static void rndintperm(int *perm,int cnt);
//static void get_current_partial_vals(double time,double *pvals,int totalpartials,dataptr dz);
//static void pancalc(double position,double *leftgain,double *rightgain);
//static void sort_partials_into_ascending_frq_order(int total_partialcnt,double *pvals,double *sinptr,
//                double **llev,double **rlev,int **onoff,int **lmost,int **origspl,int *splordr,dataptr dz);
//static void resort_partials_into_original_frq_order(int total_partialcnt,double *pvals,double *sinptr,
//                double **llev,double **rlev,int **onoff,int **lmost,int **origspl,int *splordr,dataptr dz);
//static void xclusive(int *perm,int *permon,int *permoff,int max_partials_cnt,int partials_in_play, int **onoff,int stepcnt);
//static double emergepos(int emergchan,int chans,double time,double timespan);
//static double convergepos(int converchan,int chans,double time,double convergetime,double dur);
//static void spacebox_apply(double pos, double lev,int chans,int *lmost, int *rmost,double *rlev,double *llev,int spacetyp);
//static void spacebox(double *pos, int *switchpos, double posstep, int chans, int spacetyp, int configno, int configcnt,int *superperm);

//static double sinread(double *tabpos,double frq,dataptr dz);
static int check_synspline_param_validity_and_consistency(dataptr dz);
static int do_a_spline(int *wavelen,double srate,dataptr dz);
static double cosplint(int *hi,double time,int splincnt,dataptr dz);
static int cospline(int half_wavelen,int splincnt,dataptr dz) ;

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
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
        dz->maxmode = 0;
        // setup_particular_application =
        if(cmdlinecnt < 7 || cmdlinecnt > 10) {
            usage2("synspline");
            return(FAILED);
        }
        if((exit_status = setup_synspline_application(dz))<0) {
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
    ap = dz->application;
    dz->infile->channels = 1;
    // parse_infile_and_hone_type() = 
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_synspline_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // open_first_infile() : redundant
    // handle_extra_infiles() : redundant
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency()
    if((exit_status = check_synspline_param_validity_and_consistency(dz)) < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
    if((exit_status = create_synspline_sndbufs(dz))<0) {                                        // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = open_the_outfile(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = synspline(dz))<0) {
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
    p = filename;                   //  Drop file extension
    while(*p != ENDOFSTR) {
        if(*p == '.') {
            *p = ENDOFSTR;
            break;
        }
        p++;
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
    dz->infile->srate = dz->iparam[SPLIN_SRATE];
    dz->infile->channels = 1;
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

/************************* SETUP_SYNSPLINE_APPLICATION *******************/

int setup_synspline_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,0,6,6,"idDIIi"))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"sidv",4,"IIDD","n",1,0,"0"))<0)
        return(exit_status);
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    dz->input_data_type = NO_FILE_AT_ALL;
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

/************************* SETUP_SYNSPLINE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_synspline_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    ap->lo[SPLIN_SRATE] = 16000;
    ap->hi[SPLIN_SRATE] = 96000;
    ap->default_val[SPLIN_SRATE] = 44100.0;
    ap->lo[SPLIN_DUR]   = 0.0;
    ap->hi[SPLIN_DUR]   = 32767.0;
    ap->default_val[SPLIN_DUR] = 4.0;
    ap->lo[SPLIN_FRQ]   = .001;
    ap->hi[SPLIN_FRQ]   = 10000;
    ap->default_val[SPLIN_FRQ] = 440;
    ap->lo[SPLIN_CNT] = 0;
    ap->hi[SPLIN_CNT] = 64;
    ap->default_val[SPLIN_CNT] = 2;
    ap->lo[SPLIN_INTP]  = 0;
    ap->hi[SPLIN_INTP]  = 4096;
    ap->default_val[SPLIN_INTP] = 24;
    ap->lo[SPLIN_SEED]  = 0;
    ap->hi[SPLIN_SEED]  = 64;
    ap->default_val[SPLIN_SEED] = 1;
    ap->lo[SPLIN_MCNT] = 0;
    ap->hi[SPLIN_MCNT] = 64;
    ap->default_val[SPLIN_MCNT] = 0;
    ap->lo[SPLIN_MINTP] = 0;
    ap->hi[SPLIN_MINTP] = 4096;
    ap->default_val[SPLIN_MINTP] = 0;

    ap->lo[SPLIN_DRIFT] = 0;
    ap->hi[SPLIN_DRIFT] = 12;
    ap->default_val[SPLIN_DRIFT] = 0;
    ap->lo[SPLIN_DRVEL] = 0;
    ap->hi[SPLIN_DRVEL] = 1000;
    ap->default_val[SPLIN_DRVEL] = 0;
    dz->maxmode = 0;
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
    aplptr ap;

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
            if((exit_status = setup_synspline_application(dz))<0)
                return(exit_status);
            ap = dz->application;
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
    usage2("synspline");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"synspline"))               dz->process = SYNSPLINE;
    else {
        fprintf(stderr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
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
    if(!strcmp(str,"synspline")) {
        fprintf(stderr,
        "USAGE: synspline synspline outfile srate dur frq splinecnt interpval seed\n"
        "       [-smaxspline] [-imaxinterp] [-dpdrift -vdriftrate] [-n]\n"
        "\n"
        "Synthesis from waveforms made by smoothly joining randomly generated points.\n"
        "\n"
        "SRATE     Sample rate of synthesized sound.\n"
        "DUR       Duration of synthesized sound.\n"
        "FRQ       Fundamental frq of output (Range 0.001 to 10000 Hz) (PTR).\n"
        "SPLINECNT No. of rand vals (to smooth between) per half-wavecycle (0-64) (PTR).\n"
        "INTERPVAL No. of wavecycles over which 1 waveshape morphs to next (0 - 4096) (PTR).\n"
        "SEED      Same seed value produces identical output with same parameters.\n"
        "MAXSPLINE Max splinecnt: (PTR) Causes random vals of \"splinecnt\" to be generated,\n"
        "          in range \"SPLINECNT\" to \"MAXSPLINE\" (Ignored if set to zero).\n"
        "MAXINTERP Max interpval: (PTR) Causes random vals of \"interpval\" to be generated,\n"
        "          in range \"INTERPVAL\" to \"MAXINTERP\" (Ignored if set to zero).\n"
        "PDRIFT    Semitone (half)range of random drift in frequency. (0 - 12).\n"
        "DRIFTRATE Average time (mS) to next drift offset (True time 1/2 to 3/2 of this) (1-1000).\n"
        "-n        Normalise every cycle (default: cycles retain random valued amplitudes).\n"
        "\n"
        "(PTR = possibly time-varying).\n");
    } else
        fprintf(stdout,"Unknown option '%s'\n",str);
    return(USAGE_ONLY);
}

int usage3(char *str1,char *str2)
{
    fprintf(stderr,"Insufficient parameters on command line.\n");
    return(USAGE_ONLY);
}

/******************************** SYNSPLINE *********************************/

int synspline(dataptr dz)
{
    int exit_status, firsttime = 1, splinintp, rval;
    int wavelen1, wavelen2 = 0, this_wavelen, wavelendiff, j, k, n, m, obufpos = 0, driftpos;
    double thistime = 0.0, ampinterp1, ampinterp2, val1, val2, val, frac, diff, dpos1, dpos2, dincr1, dincr2, rrange, srate = dz->iparam[SPLIN_SRATE];
    float *splinebuf1 = dz->sampbuf[0], *splinebuf2 = dz->sampbuf[1], *obuf = dz->sampbuf[2], *ovflwbuf = dz->sampbuf[3];
    float *driftbuf = dz->sampbuf[4];
    double lastdrifttime = 0.0, nextdrifttime = 0.0, driftincr1 = 1.0, driftincr2 = 1.0, driftincrincr = 0.0, driftend, dbufpos, abstime;
    int lastdriftsamps = 0, nextdriftsamps = 0, abssamps = 0, driftbufpos = 0;
    memset((char *)splinebuf1,0,dz->buflen * sizeof(float));
    memset((char *)splinebuf2,0,dz->buflen * sizeof(float));
    if(dz->is_drift) {
        lastdrifttime = 0;
        lastdriftsamps = 0;
        if(dz->brksize[SPLIN_DRIFT]) {
            if((read_value_from_brktable(lastdrifttime,SPLIN_DRIFT,dz))<0)
                return DATA_ERROR;
        }
        driftincr1 = (drand48() * 2.0) - 1.0;                                   //  Range 0 to 1 -> 0 to 2 -> -1 to 1
        driftincr1 *= dz->param[SPLIN_DRIFT];
        driftincr1 = pow(2.0,driftincr1/SEMITONES_PER_OCTAVE);                  //  Read increment to produce initial transposition value
        if(dz->brksize[SPLIN_DRVEL]) {
            if((read_value_from_brktable(0,SPLIN_DRVEL,dz))<0)
                return DATA_ERROR;
        }
        nextdrifttime = (drand48()) + 0.5;                                      //  Range 0 to 1 -> 0 to 1/2 to 3/2
        nextdrifttime *= dz->param[SPLIN_DRVEL] * MS_TO_SECS;
        nextdriftsamps = (int)floor(nextdrifttime * srate);
        if(dz->brksize[SPLIN_DRIFT]) {
            if((read_value_from_brktable(nextdrifttime,SPLIN_DRIFT,dz))<0)
                return DATA_ERROR;
        }
        driftincr2 = ((drand48() * 2.0) - 1.0) - 0.5;
        driftincr2 *= dz->param[SPLIN_DRIFT];
        driftincr2 = pow(2.0,driftincr2/SEMITONES_PER_OCTAVE);                  //  Read increment to produce end transposition value
        driftincrincr = (double)(driftincr2 - driftincr1)/(nextdrifttime - lastdrifttime);
    }                                                                           //  Increment of increment
    while(thistime < dz->param[SPLIN_DUR]) {
        if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
            return exit_status;
        if(firsttime) {
            if((exit_status = do_a_spline(&wavelen2,srate,dz))<0)               //  Create a spline in splinebuf2
                return exit_status;
            firsttime = 0;
        } else {
            memset((char *)splinebuf1,0,dz->buflen * sizeof(float));            //  Copy spline2 to spline1
            memcpy((char *)splinebuf1,(char *)splinebuf2,dz->buflen * sizeof(float));
            memset((char *)splinebuf2,0,dz->buflen * sizeof(float));
            wavelen1 = wavelen2;
            if((exit_status = do_a_spline(&wavelen2,srate,dz))<0)               //  Create a 2nd spline in splinebuf2
                return exit_status;
            if(dz->is_drift) {
                    //  Output is written to driftbuf (rather than directly to obuf)
                memset((char *)driftbuf,0,dz->buflen2 * sizeof(float));
                driftpos = 0;
                for(n=0;n<wavelen1;n++)                                             //  Write 1st spline to outbuf
                    driftbuf[driftpos++] = splinebuf1[n];
                thistime += (double)wavelen1/srate;                                 //  Update time in output
                if(thistime >= dz->param[SPLIN_DUR])
                    break;
                splinintp = dz->iparam[SPLIN_INTP];                                 //  Get numjber of interpolations
                if(dz->iparam[SPLIN_MINTP] > 0) {                                   //  (and if there are min and max values,
                    rrange = dz->iparam[SPLIN_MINTP] - dz->iparam[SPLIN_INTP];      //  generate a rand val in the range)
                    rval = (int)floor(drand48() * (double)rrange);
                    splinintp += rval;
                }                                                                   //  Note any change in wavelength
                wavelendiff = wavelen2 - wavelen1;                      
                for(j = 0; j < splinintp;j++) {                                     //  Create the interpolated splines.
                    ampinterp1 = (double)(splinintp - j)/(double)splinintp;         //  Relative level of 1st spline at this count
                    ampinterp2 = (double)j/(double)splinintp;                       //  Relative level of 2nd spline at this count 
                    if(wavelendiff == 0) {
                        for(n = 0; n < wavelen1 ;n++) {                             //  If wavelens are same length
                            val1 = splinebuf1[n] * ampinterp1;                      //  do straightforward intepolation, saple by sample
                            val2 = splinebuf2[n] * ampinterp2;
                            driftbuf[driftpos++] = (float)(val1 + val2);
                        }
                        thistime += (double)wavelen1/srate;                         //  Update time in output
                        if(thistime >= dz->param[SPLIN_DUR])
                            break;
                    } else {                                                        //  If wavelens at start and endare different, wavelen at each interpolated wavecycle will be different,
                                                                                    //  so both start and end waveforms will need to be read by interpolation                                                                               
    
                        dpos1 = 0.0;                                                //  Fractional sample-read in spline1, initially zero
                        dpos2 = 0.0;                                                //  Fractional sample-read in spline2, initially zero
                                                                                    //  Then interpolate for current interpolated-spline's length
                        this_wavelen = wavelen1 + (int)round((double)wavelendiff * ((double)(j+1)/(double)(splinintp+1)));
                        dincr1 = (double)wavelen1/(double)this_wavelen;             //  Fractional sample-advance in 1st spline
                        dincr2 = (double)wavelen2/(double)this_wavelen;             //  Fractional sample-advance in 2nd spline
                        for(n = 0, m = this_wavelen; n < this_wavelen ;n++,m--) {
                                        //  spline 1 : Interpolate timewise for value
                            k = (int)floor(dpos1);
                            frac = dpos1 - (double)k;
                            diff = splinebuf1[k+1] - splinebuf1[k];
                            val1 = splinebuf1[k] + (diff * frac);
                                        //  spline 2 : Interpolate timewise for value
                            k = (int)floor(dpos2);
                            frac = dpos2 - (double)k;
                            diff = splinebuf2[k+1] - splinebuf2[k];
                            val2 = splinebuf2[k] + (diff * frac);
                                        //  Interpolate values for relative amplitude
                            val1 *= ampinterp1;
                            val2 *= ampinterp2;
                            driftbuf[driftpos++] = (float)(val1 + val2);
                                        //  Advance in original-splines
                            dpos1 += dincr1;
                            dpos2 += dincr2;
                        }
                        thistime += (double)this_wavelen/srate;                     //  Update time in output
                        if(thistime >= dz->param[SPLIN_DUR])
                            break;
                    }
                }
                //  Once all interpolating cycles have been written to driftbuf
                //  data is transpose-copied to outbuf

                driftend = (double)driftpos;                        //  Note end of sampoles in driftbuf (later samples are zeroed..deals with wraparound)
                dbufpos = 0.0;                                      //  Set to zero (possibly) fractional pointer into driftbuf.

                while(dbufpos < driftend) {                         //  Advance by interpolated increments in driftbuf until end of buf reached.
                    abssamps = dz->total_samps_written + obufpos;               
                    abstime = (double)abssamps/srate;               //  Absolute time (for reading any brktables) measured from absolute outbuf position
                    if(abssamps == nextdriftsamps) {
                        driftincr1     = driftincr2;                //  If we've reached the next designated time for generating a new transposition(incr) value,
                        lastdrifttime  = nextdrifttime;             //  Set the "last" values as what were the "next" values
                        lastdriftsamps = nextdriftsamps;
                        if(dz->brksize[SPLIN_DRVEL]) {
                            if((read_value_from_brktable(nextdrifttime,SPLIN_DRVEL,dz))<0)
                                return DATA_ERROR;                  //  Get the timestep to the next transposition val
                        }                                           //  Get next-transposition-value average-timing by adding step to current time
                        val = (drand48()) + 0.5;                    //  Generate rand variation of this in range 1/2 to 3/2 of value    
                        val *= dz->param[SPLIN_DRVEL] * MS_TO_SECS; 
                        nextdrifttime += val;
                        nextdriftsamps  = (int)floor(nextdrifttime * srate);
                        if(dz->brksize[SPLIN_DRIFT]) {              //  Get next-transposition-value
                            if((read_value_from_brktable(nextdrifttime,SPLIN_DRIFT,dz))<0)
                                return DATA_ERROR;
                        }
                        driftincr2 = ((drand48() * 2.0) - 1.0) - 0.5;
                        driftincr2 *= dz->param[SPLIN_DRIFT];       //  Calc next table-read increment
                        driftincr2 = pow(2.0,driftincr2/SEMITONES_PER_OCTAVE);
                                                                    //  and set the step change from current incr (driftincr1) to the next (driftincr2)
                        driftincrincr = (double)(driftincr2 - driftincr1)/(double)(nextdriftsamps - lastdriftsamps);
                    }
                    driftbufpos = (int)floor(dbufpos);
                    frac = dbufpos - (double)driftbufpos;           //  Read values from driftbuf by interpolation,
                    diff = driftbuf[driftbufpos+1] - driftbuf[driftbufpos];
                    diff *= frac;                                   //  and put interpolated value in obuf.
                    obuf[obufpos++] = (float)(driftbuf[driftbufpos] + diff); 
                    dbufpos += driftincr1;                          //  Advance in driftbuf 
                    driftincr1 += driftincrincr;                    //  AND alter advance-incr appropriately
                    if(driftincrincr >= 0)                          //  If driftincr(1) increasing
                        driftincr1 = min(driftincr1,driftincr2);    //  don't let driftincr rise above value of incr2
                    else                                            //  Else, driftincr(1) falling, so
                        driftincr1 = max(driftincr1,driftincr2);    //  don't let driftincr fall below value of incr2

                    if(obufpos >= dz->buflen2) {                    //  If obuf fills up, write to file
                        if((exit_status = write_samps(obuf,dz->buflen2,dz))<0)
                            return(exit_status);
                        memset((char *)obuf,0,dz->buflen2 * sizeof(float));
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen2 * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen2 * sizeof(float));
                        obufpos -= dz->buflen2;
                    }
                }
            } else {
                for(n=0;n<wavelen1;n++)                                             //  Write 1st spline to outbuf
                    obuf[obufpos++] = splinebuf1[n];
                thistime += (double)wavelen1/srate;                                 //  Update time in output
                if(thistime >= dz->param[SPLIN_DUR])
                    break;
                if(obufpos >= dz->buflen2) {
                    if((exit_status = write_samps(obuf,dz->buflen2,dz))<0)
                        return(exit_status);
                    memset((char *)obuf,0,dz->buflen2 * sizeof(float));
                    memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen2 * sizeof(float));
                    memset((char *)ovflwbuf,0,dz->buflen2 * sizeof(float));
                    obufpos -= dz->buflen2;
                }
                splinintp = dz->iparam[SPLIN_INTP];
                if(dz->iparam[SPLIN_MINTP] > 0) {
                    rrange = dz->iparam[SPLIN_MINTP] - dz->iparam[SPLIN_INTP];
                    rval = (int)floor(drand48() * (double)rrange);
                    splinintp += rval;
                }
                wavelendiff = wavelen2 - wavelen1;
                for(j = 0; j < splinintp;j++) {                                     //  Create the interpolated splines.
                    ampinterp1 = (double)(splinintp - j)/(double)splinintp;         //  Relative level of 1st spline at this count
                    ampinterp2 = (double)j/(double)splinintp;                       //  Relative level of 2nd spline at this count 
                    if(wavelendiff == 0) {                                          //  If splines are of same length
                        for(n = 0; n < wavelen1 ;n++) {
                            val1 = splinebuf1[n] * ampinterp1;
                            val2 = splinebuf2[n] * ampinterp2;
                            obuf[obufpos++] = (float)(val1 + val2);
                        }
                        thistime += (double)wavelen1/srate;                             //  Update time in output
                        if(thistime >= dz->param[SPLIN_DUR])
                            break;
                    } else {
                        dpos1 = 0.0;                                                //  Fractional sample-read in spline1, initially zero
                        dpos2 = 0.0;                                                //  Fractional sample-read in spline2, initially zero
                                                                                    //  Interpolate for current interpoalted-spline's length
                        this_wavelen = wavelen1 + (int)round((double)wavelendiff * ((double)(j+1)/(double)(splinintp+1)));
                        dincr1 = (double)wavelen1/(double)this_wavelen;             //  Fractional sample-advance in 1st spline
                        dincr2 = (double)wavelen2/(double)this_wavelen;             //  Fractional sample-advance in 2nd spline
                        for(n = 0, m = this_wavelen; n < this_wavelen ;n++,m--) {
                                        //  spline 1 : Interpolate timewise for value
                            k = (int)floor(dpos1);
                            frac = dpos1 - (double)k;
                            diff = splinebuf1[k+1] - splinebuf1[k];
                            val1 = splinebuf1[k] + (diff * frac);
                                        //  spline 2 : Interpolate timewise for value
                            k = (int)floor(dpos2);
                            frac = dpos2 - (double)k;
                            diff = splinebuf2[k+1] - splinebuf2[k];
                            val2 = splinebuf2[k] + (diff * frac);
                                        //  Interpolate values for relative amplitude
                            val1 *= ampinterp1;
                            val2 *= ampinterp2;
                            obuf[obufpos++] = (float)(val1 + val2);
                                        //  Advance in original-splines
                            dpos1 += dincr1;
                            dpos2 += dincr2;
                        }
                        thistime += (double)this_wavelen/srate;                     //  Update time in output
                        if(thistime >= dz->param[SPLIN_DUR])
                            break;
                    }
                    if(obufpos >= dz->buflen2) {
                        if((exit_status = write_samps(obuf,dz->buflen2,dz))<0)
                            return(exit_status);
                        memset((char *)obuf,0,dz->buflen2 * sizeof(float));
                        memcpy((char *)obuf,(char *)ovflwbuf,dz->buflen2 * sizeof(float));
                        memset((char *)ovflwbuf,0,dz->buflen2 * sizeof(float));
                        obufpos -= dz->buflen2;
                    }
                }
            }
        }
    }
    if(obufpos > 0) {
        if((exit_status = write_samps(obuf,obufpos,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/**************************** CREATE_SYNSPLINE_SNDBUFS ****************************/

int create_synspline_sndbufs(dataptr dz)
{
    int exit_status;
    int bigbufsize;
    int framesize, maxwavelen, segsize, max_intp;
    double frqmin, maxintp;
    framesize = F_SECSIZE;
    if(dz->brksize[SPLIN_FRQ]) {
        if((exit_status = get_minvalue_in_brktable(&frqmin,SPLIN_FRQ,dz))<0)
            return exit_status;
    } else
        frqmin = dz->param[SPLIN_FRQ];

    if(dz->is_drift)
        frqmin *= pow(2.0,-dz->maxdrift/SEMITONES_PER_OCTAVE);

    maxwavelen = (int)ceil((double)dz->iparam[SPLIN_SRATE]/frqmin);
    segsize = maxwavelen/framesize;
    if(segsize + framesize != maxwavelen)
        segsize++;
    maxwavelen = segsize * framesize;
    dz->buflen = maxwavelen + 4;    //  SAFETY

    if(dz->brksize[SPLIN_INTP]) {
        if((exit_status = get_maxvalue_in_brktable(&maxintp,SPLIN_INTP,dz))<0)
            return exit_status;
        max_intp = (int)round(maxintp);
    } else
        max_intp = dz->iparam[SPLIN_INTP];
    dz->buflen2 = (dz->buflen * (max_intp + 1)) + 4;

    bigbufsize = ((dz->buflen + dz->buflen2) * 2) * sizeof(float);
    if(dz->is_drift)
        bigbufsize += dz->buflen2 * sizeof(float);
    if((dz->bigbuf = (float *)malloc(bigbufsize)) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->bufcnt = 5;
    if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;                       //  splinebuf1
    dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + dz->buflen;      //  splinebuf2
    dz->sampbuf[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen;      //  obuf
    dz->sampbuf[3] = dz->sampbuf[3] = dz->sampbuf[2] + dz->buflen2;     //  ovflwbuf
    if(dz->is_drift) {
        dz->sampbuf[4] = dz->sampbuf[4] = dz->sampbuf[3] + dz->buflen2; //  pre-drift interpolation buffer
        dz->sampbuf[5] = dz->sampbuf[5] + dz->buflen2;                  //  end of pre-drift buffer
    } else
        dz->sampbuf[4] = dz->sampbuf[4] + dz->buflen2;                  //  end of ovflwbuf
    return FINISHED;
}

/**************************** CHECK_SYNSPLINE_PARAM_VALIDITY_AND_CONSISTENCY *************************/

int check_synspline_param_validity_and_consistency(dataptr dz)
{
    int exit_status;
    double maxfrq, firstfrq, dmaxcnt, dmmaxcnt, maxdriftvel, mindriftvel;
    int minwavelen, firstwavelen, maxcnt, mmaxcnt, sampdur;
    dz->maxdrift = 0.0;

    if(dz->brksize[SPLIN_FRQ]) {
        if((exit_status = get_maxvalue_in_brktable(&maxfrq,SPLIN_FRQ,dz))<0)
            return exit_status;
    } else
        maxfrq = dz->param[SPLIN_FRQ];

    if(dz->brksize[SPLIN_DRIFT]) {
        if((exit_status = get_maxvalue_in_brktable(&(dz->maxdrift),SPLIN_FRQ,dz))<0)
            return exit_status;
    } else
        dz->maxdrift = dz->param[SPLIN_DRIFT];
    if(dz->maxdrift == 0.0) {
        if(dz->brksize[SPLIN_DRVEL] || dz->param[SPLIN_DRVEL] > 0.0) {
            fprintf(stdout,"WARNING: Drift transposition zero or not set. Ignoring drift step value(s)\n");
            fflush(stdout);
            dz->param[SPLIN_DRVEL] = 0.0;
            dz->brksize[SPLIN_DRVEL] = 0;
        }
    } else {
        if(dz->brksize[SPLIN_DRVEL]) {
            if((exit_status = get_maxvalue_in_brktable(&maxdriftvel,SPLIN_DRVEL,dz))<0)
                return exit_status;
            if((exit_status = get_minvalue_in_brktable(&mindriftvel,SPLIN_DRVEL,dz))<0)
                return exit_status;
        } else {
            mindriftvel = dz->param[SPLIN_DRVEL];
            maxdriftvel = dz->param[SPLIN_DRVEL];
        }
        if(mindriftvel < 1.0) {
            sprintf(errstr,"If drift transposition set (and not 0), vals for drift step must be set (and not less than 1mS).\n");
            return DATA_ERROR;
        }
    }
    if(dz->maxdrift > 0.0) {
        maxfrq *= pow(2.0,dz->maxdrift/SEMITONES_PER_OCTAVE);
        dz->is_drift = 1;
    }
    minwavelen = (int)floor((double)dz->iparam[SPLIN_SRATE]/maxfrq);
    if(dz->brksize[SPLIN_CNT]) {
        if((exit_status = get_maxvalue_in_brktable(&dmaxcnt,SPLIN_CNT,dz))<0)
            return exit_status;
        maxcnt = (int)round(dmaxcnt);
    } else
        maxcnt = dz->iparam[SPLIN_CNT];

    if(dz->brksize[SPLIN_MCNT]) {
        if((exit_status = get_maxvalue_in_brktable(&dmmaxcnt,SPLIN_MCNT,dz))<0)
            return exit_status;
        mmaxcnt = (int)round(dmmaxcnt);
    } else
        mmaxcnt = dz->iparam[SPLIN_MCNT];
    maxcnt = max(maxcnt,mmaxcnt);

    dz->itemcnt = maxcnt + 4; // SAFETY

    if((dz->parray = (double **)malloc(5 * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store splint randvals. (1)\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[0] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store splint randvals. (2)\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[1] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing spline times.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[2] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing u-coordinates for spline.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[3] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing 2nd derivatives for spline.\n");
        return(MEMORY_ERROR);
    }
    if((dz->parray[4] = (double *)malloc(dz->itemcnt * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for storing rand values for splining.\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray = (int **)malloc(sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store splint sampvals. (1)\n");
        return(MEMORY_ERROR);
    }
    if((dz->lparray[0] = (int *)malloc(dz->itemcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store splint sampvals. (2)\n");
        return(MEMORY_ERROR);
    }
    if(maxcnt > 0) {
        if(minwavelen/2/maxcnt < 2) {
            sprintf(errstr,"Maximum frq and maximum spline count are incompatible. Reduce one or both of these.\n");
            return DATA_ERROR;
        }
    }
    sampdur = (int)floor(dz->param[SPLIN_DUR] * (double)dz->iparam[SPLIN_SRATE]);
    if(dz->brksize[SPLIN_FRQ])
        firstfrq = dz->brk[SPLIN_FRQ][1];
    else
        firstfrq = dz->param[SPLIN_FRQ];
    firstwavelen = (int)ceil((double)dz->iparam[SPLIN_SRATE]/firstfrq);
    if(firstwavelen > sampdur) {
        sprintf(errstr,"Duration too short to generate a complete wavecycle.\n");
        return DATA_ERROR;
    }
    if(dz->iparam[SPLIN_SEED] > 0)
        srand(dz->iparam[SPLIN_SEED]);
    else
        initrand48();
    return FINISHED;
}

/*************************************** COSPLINE *******************************
 *
 * Establish times for cosplint
 */

int cospline(int half_wavelen, int splincnt, dataptr dz)
{
    int *splintsamps = dz->lparray[0];
    double *splintime = dz->parray[1];
    int n;
    double srate = (double)dz->param[SPLIN_SRATE];
    splintime[0] = 0.0;
    for(n=0;n < splincnt;n++)                   //  There are   SPLIN-CNT+1 points
        splintime[n] = (double)splintsamps[n]/srate;
    splintime[n] = (double)half_wavelen/srate;
    return FINISHED;
}

/*************************************** SPLINT *******************************
 *
 * Do cosinusoidal spline.
 */

double cosplint(int *hi,double time,int splincnt,dataptr dz)
{
    double *splintime = dz->parray[1], *splintamp = dz->parray[4];
    double timestep, timelo, timefrac, valstep, vallo, val;
    if(time <= 0.0)
        return 0.0;
    while(time > splintime[*hi]) {
        (*hi)++;
        if(*hi > splincnt + 1)
            return 0.0; //  end of in table
    }
    vallo  = splintamp[(*hi)-1];
    timelo = splintime[(*hi)-1];
    timestep = splintime[*hi] - timelo;
    valstep  = splintamp[*hi] - vallo;

    timefrac = (time - timelo)/timestep;

    val = cos(timefrac * PI);   //  cos 0-PI:   range 1  to -1
    val = -val;                 //  -cos:       range -1 to  1
    val += 1;                   //  -cos+1:     range 0  to  2
    val /= 2;                   //  (-cos+1)/2  range 0  to  1
    val *=  valstep; 
    val +=  vallo;
    return val;
}

/******************************************* DO_A_SPLINE *****************************************/

int do_a_spline(int *wavelen,double srate,dataptr dz)
{
    int gotit = 0, n, m, hi = 0, orig_splincnt, rval, splincnt = 0;
    int half_wavelen;
    double maxrand, temp, maxsamp, rrange;
    int   *splintsamps = dz->lparray[0];
    double *splintrand  = dz->parray[4], *splintamp = dz->parray[4];
    float  *splinebuf2  = dz->sampbuf[1];  

    orig_splincnt = dz->iparam[SPLIN_CNT];
    if(dz->iparam[SPLIN_MCNT] > 0) {
        rrange = dz->iparam[SPLIN_MCNT] - dz->iparam[SPLIN_CNT];
        rval = (int)floor(drand48() * (double)rrange);
        orig_splincnt += rval;
    }
    *wavelen = (int)ceil((double)dz->iparam[SPLIN_SRATE]/dz->param[SPLIN_FRQ]);
    half_wavelen = (*wavelen)/2;
    *wavelen = half_wavelen * 2;
    if(orig_splincnt == 0) {
        for(n = 0; n < *wavelen; n++)
            splinebuf2[n] = (float)(MAXOUTAMP * sin(((double)n/(double)*wavelen) * TWOPI));
        return FINISHED;
    }
    while(!gotit) {
        splincnt = orig_splincnt;
        for(n = 0; n <= splincnt;n++)
            splintrand[n] = drand48();                  //  Generate splincnt+1 rand vals for splint positions
        maxrand = splintrand[0];
        for(n = 1; n <= splincnt;n++)
            maxrand = max(maxrand,splintrand[n]);       //  Find maxrand value
        for(n = 0; n <= splincnt;n++)
            splintrand[n] /= maxrand;                   //  Divide all by maxrand, so values in range 0 to 1        
        for(n = 0; n < splincnt;n++) {
            for(m = n; m <= splincnt;m++) {//   Sort vals into ascending order
                if(splintrand[m] < splintrand[n]) {
                    temp = splintrand[n];
                    splintrand[n] = splintrand[m];
                    splintrand[m] = temp;
                }
            }
        }
        for(n = 0; n < splincnt;n++)        //  Convert sorted rands to sample counts within half-wavelen
            splintsamps[n] = (int)round(splintrand[n] * (double)half_wavelen);
        splintsamps[n] = half_wavelen;                  //  Force last val to be at half-wavelen end.
        for(n = 0; n < splincnt;n++) {      //  Remove any identical sample values, and reduce splint_cnt appropriately
            if(splintsamps[n+1] == splintsamps[n]) {
                for(m = n;m < splincnt;m++)
                    splintsamps[n] = splintsamps[n+1];
                splincnt--;
            }
        }
        if(splincnt <= 0)
            continue;
        if(splintsamps[0] < 2) {                        //  Remove too sudden onsets
            for(n = 0;n < splincnt;n++)
                splintsamps[n] = splintsamps[n+1];
            splincnt--;
        }           
        if(splincnt <= 0)
            continue;
        
        if(half_wavelen - splintsamps[splincnt - 1] < 2) {
            for(n = 0;n < splincnt;n++)
                splintsamps[n] = splintsamps[n+1];
            splincnt--;
        }           
        if(splincnt <= 0)
            continue;
        gotit = 1;
    }

    memset((char *)splintrand,0, dz->itemcnt * sizeof(float));
    for(n = 0; n < splincnt;n++)
        splintamp[n] = drand48();                       //  Generate splincnt rand vals for amplitude
    if(dz->vflag[CYCLENORM]) {
        maxrand = splintamp[0];
        for(n = 1; n < splincnt;n++)
            maxrand = max(maxrand,splintamp[n]);        //  Find maxrand value
        for(n = 0; n < splincnt;n++)                    //  And normalise (amp) values within cycle
            splintamp[n] /= maxrand;
    }
    for(n = 0; n < splincnt;n++)                        //  Keep output within distortion limits
        splintamp[n] = (float)(splintamp[n] * MAXOUTAMP);

    splintamp[n] = 0.0;                                 //  Force final point to amplitude zero
    cospline(half_wavelen,splincnt,dz);                 //  Calculate times for splining
    for(n=0; n < half_wavelen;n++)                      //  Do COSIN-interps
        splinebuf2[n] = (float)cosplint(&hi,(double)n/srate,splincnt,dz);
    maxsamp = 0.0;
    for(n=0,m = half_wavelen; n < half_wavelen;n++,m++)
        splinebuf2[m] = (float)(-splinebuf2[n]);
    return FINISHED;
}
