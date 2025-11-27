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
//#include <srates.h>
#include <pnames.h>
//#include <extdcon.h>
#include <limits.h>


//static int testflag;

//#ifdef unix 
#define round(x) lround((x))
//#endif

#define srcsamps rampbrksize
#define counted is_rectified

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

#define MINFADE 2       // minimum fade of element in MS

const char* cdp_version = "7.0.0";

//CDP LIB REPLACEMENTS
static int setup_pulser_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_pulser_param_ranges_and_defaults(dataptr dz);
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
static int pulser(int ochancnt,dataptr dz);
static int do_pulser_preprocess(int ochancnt,dataptr dz);

static int generate_packet(int *packetsize,double frq,double upfrq,int rise,int sustain,int decay,int srcsize,double srate,dataptr dz);
static int output_packet(int obufpos,int packetsize,int chan,int *lastwrite,int ochancnt,dataptr dz);
static void rndintperm(int *perm,int cnt);
static void showpulsertime(int thisprocess,int samps_processed,double timerscale,dataptr dz);
static int handle_the_special_data(char *str,dataptr dz);
static int synthesize_src(double frq,double srate,double time,dataptr dz);
static void incr_sinptr(int n,double onehzincr,double frq,dataptr dz);
static int open_the_outfile(int ochancnt,dataptr dz);
static double splint(int k,double time,dataptr dz);
static int spline(int k,dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static int store_chans_to_use_data(char *temp,int cnt,dataptr dz);
static int get_chan_to_use(double time,dataptr dz);
static int get_channel_string(char *str,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status, ochancnt = 1;
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
    dz->iparray = NULL;
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
        dz->maxmode = 3;
        if((exit_status = get_the_mode_from_cmdline(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(exit_status);
        }
        cmdline++;
        cmdlinecnt--;
        // setup_particular_application =
        if((exit_status = setup_pulser_application(dz))<0) {
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
    if(dz->process != PULSER3) {
        if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
            exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_pulser_param_ranges_and_defaults(dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // open_first_infile        CDP LIB
    if(dz->process != PULSER3) {
        if((exit_status = open_first_infile(cmdline[0],dz))<0) {    
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);  
            return(FAILED);
        }
        cmdlinecnt--;
        cmdline++;

        if(dz->process == PULSER2) {
            if((exit_status = handle_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
                print_messages_and_close_sndfiles(exit_status,is_launched,dz);      
                return(FAILED);
            }
        }
    }
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
//  handle_formant_quiksearch() redundant

    if(dz->process == PULSER3) {
        if(dz->mode > 0) {
            if((dz->parray = (double **)malloc(8 * sizeof(double *)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to create synth data storage.\n");
                return(MEMORY_ERROR);
            }
            if((dz->iparray = (int **)malloc(2 * sizeof(int *)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to create synth data storage2.\n");
                return(MEMORY_ERROR);
            }
        } else {
            if((dz->parray = (double **)malloc(5 * sizeof(double *)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to create synth data storage.\n");
                return(MEMORY_ERROR);
            }
        }
        if((exit_status = handle_the_special_data(cmdline[0],dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        cmdlinecnt--;
        cmdline++;
    } else if(dz->mode == 0) {
        if((dz->parray = (double **)malloc(sizeof(double *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to create spline data storage.\n");
            return(MEMORY_ERROR);
        }
    } else if(dz->process != PULSER3 && dz->mode == 2) {
        exit_status = get_channel_string(cmdline[0],dz);
        if(exit_status < 0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return DATA_ERROR;
        }
        else if(exit_status == 0)  {        //  Found a filename
            if((exit_status = handle_the_special_data(cmdline[0],dz))<0) {
                print_messages_and_close_sndfiles(exit_status,is_launched,dz);
                return(FAILED);
            }
        }
        cmdlinecnt--;
        cmdline++;
    }
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if(dz->process != PULSER3 && dz->mode == 2) {
        if(dz->iparray == NULL) {
            if(dz->brksize[PLS_WIDTH] || (dz->param[PLS_WIDTH] > 0.0))
                ochancnt = 2;
        } else
            ochancnt = 8;
    }
    if((exit_status = open_the_outfile(ochancnt,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
//  check_param_validity_and_consistency() redundant
    is_launched = TRUE;
    dz->bufcnt = 4;
    if((dz->sampbuf = (float **)malloc(sizeof(float *) * (dz->bufcnt+1)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffers.\n");
        return(MEMORY_ERROR);
    }
    if((dz->sbufptr = (float **)malloc(sizeof(float *) * dz->bufcnt+1))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing sample buffer pointers.\n");
        return(MEMORY_ERROR);
    }
    for(n = 0;n <dz->bufcnt; n++)
        dz->sampbuf[n] = dz->sbufptr[n] = (float *)0;
    dz->sampbuf[n] = (float *)0;

    if((exit_status = do_pulser_preprocess(ochancnt,dz))<0) {   //  Also creates buffers
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = pulser(ochancnt,dz))<0) {
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

int open_the_outfile(int ochancnt, dataptr dz)
{
    int exit_status;
    if(dz->process == PULSER3) {
        dz->infile->channels = 1;
        dz->infile->srate = dz->iparam[PLS_SRATE];
        dz->outfile->channels = 1;
        dz->outfile->srate = dz->iparam[PLS_SRATE];
        dz->infile->stype  = SAMP_SHORT;
        dz->outfile->stype = SAMP_SHORT;
    }
    if(ochancnt > 1) {
        dz->infile->channels  = ochancnt;
        dz->outfile->channels = ochancnt;
    }
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
        return(exit_status);
    if(ochancnt > 1)
        dz->infile->channels = 1;
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

/************************* SETUP_PULSER_APPLICATION *******************/

int setup_pulser_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    if(dz->process == PULSER3) {
        switch(dz->mode) {
        case(0): exit_status = set_param_data(ap,SYN_SPEK,10,10,"dDddddddDD");  break;
        case(1):
        case(2): exit_status = set_param_data(ap,SYN_PARTIALS,10,10,"dDddddddDD");  break;
        }
    } else {
        switch(dz->mode) {
        case(0):    exit_status = set_param_data(ap,0,10,10,"dDddddddDD");          break;
        case(1):    exit_status = set_param_data(ap,0,10,9,"d0ddddddDD");           break;
        case(2):    exit_status = set_param_data(ap,SPACEDATA,10,9,"d0ddDDddDD");   break;
        }
    }
    if(exit_status < 0)
        return FAILED;
    switch(dz->process) {
    case(PULSER):
        switch(dz->mode) {
        case(2): exit_status = set_vflgs(ap,"",0,"","eEpaobsw",8,8,"DDDDDDiD"); break;
        default: exit_status = set_vflgs(ap,"",0,"","eEpaobs",7,7,"DDDDDDi"); break;
        }
        break;
    case(PULSER2):
        switch(dz->mode) {
        case(2): exit_status = set_vflgs(ap,"",0,"","eEpaobswr",9,8,"DDDDDDiD0"); break;
        default: exit_status = set_vflgs(ap,"",0,"","eEpaobsr",8,7,"DDDDDDi0"); break;
        }
        break;
    case(PULSER3):
        exit_status = set_vflgs(ap,"",0,"","eEpaobsSc",9,9,"DDDDDDiiD");
        break;
    }
    if(exit_status < 0)
        return FAILED;
    // set_legal_infile_structure -->
    dz->has_otherfile = FALSE;
    // assign_process_logic -->
    switch(dz->process) {
    case(PULSER):
        dz->input_data_type = SNDFILES_ONLY;
        dz->process_type    = UNEQUAL_SNDFILE;  
        dz->outfiletype     = SNDFILE_OUT;
        break;
    case(PULSER2):
        dz->input_data_type = MANY_SNDFILES;
        dz->process_type    = UNEQUAL_SNDFILE;  
        dz->outfiletype     = SNDFILE_OUT;
        break;
    case(PULSER3):
        dz->input_data_type = NO_FILE_AT_ALL;
        dz->process_type    = UNEQUAL_SNDFILE;  
        dz->outfiletype     = SNDFILE_OUT;
        break;
    }
    dz->maxmode = 3;
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
        } else if(infile_info->channels != MONO)  {
            sprintf(errstr,"File %s is not of correct type: must be MONO\n",cmdline[0]);
            return(DATA_ERROR);
        } else if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0) {
            sprintf(errstr,"Failed to copy file parsing information\n");
            return(PROGRAM_ERROR);
        }
        free(infile_info);
    }
    return(FINISHED);
}

/************************* SETUP_ITERLINE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_pulser_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()

    ap->lo[PLS_DUR]      = .02;
    ap->hi[PLS_DUR]      = 32767;
    ap->default_val[PLS_DUR] = 4;   
    if(dz->process == PULSER3 || dz->mode == 0) {
        ap->lo[PLS_PITCH]    = 24;
        ap->hi[PLS_PITCH]    = 96;
        ap->default_val[PLS_PITCH] = 60;    
    }
    ap->lo[PLS_MINRISE]  = 0.002;
    ap->hi[PLS_MINRISE]  = 0.2;
    ap->default_val[PLS_MINRISE] = 0.02;
    ap->lo[PLS_MAXRISE]  = 0.002;
    ap->hi[PLS_MAXRISE]  = 0.2;
    ap->default_val[PLS_MAXRISE] = 0.02;
    ap->lo[PLS_MINSUS]   = 0.0;
    ap->lo[PLS_MAXSUS]   = 0.0;
    if(dz->process == PULSER  && dz->mode == 2) {
        ap->hi[PLS_MAXSUS]   = 1.0;
        ap->hi[PLS_MINSUS]   = 1.0;
    } else {
        ap->hi[PLS_MAXSUS]   = 0.2;
        ap->hi[PLS_MINSUS]   = 0.2;
    }
    ap->default_val[PLS_MINSUS] = 0.0;
    ap->default_val[PLS_MAXSUS] = 0.0;
    ap->lo[PLS_MINDECAY] = 0.02;
    ap->hi[PLS_MINDECAY] = 2;
    ap->default_val[PLS_MINDECAY] = 0.05;
    ap->lo[PLS_MAXDECAY] = 0.02;
    ap->hi[PLS_MAXDECAY] = 2;
    ap->default_val[PLS_MAXDECAY] = 0.05;
    ap->lo[PLS_SPEED]   = 0.05;
    ap->hi[PLS_SPEED]   = 1;
    ap->default_val[PLS_SPEED] = 0.1;
    ap->lo[PLS_SCAT]    = 0.0;
    ap->hi[PLS_SCAT]    = 1.0;
    ap->default_val[PLS_SCAT] = 0.0;
    ap->lo[PLS_EXP] = 0.25;
    ap->hi[PLS_EXP] = 4;
    ap->default_val[PLS_EXP] = 1.0;
    ap->lo[PLS_EXP2]    = 0.25;
    ap->hi[PLS_EXP2]    = 4;
    ap->default_val[PLS_EXP2] = 1.0;
    ap->lo[PLS_PSCAT]   = 0;
    ap->hi[PLS_PSCAT]   = 1;
    ap->default_val[PLS_PSCAT] = 0;
    ap->lo[PLS_ASCAT]   = 0;
    ap->hi[PLS_ASCAT]   = 1;
    ap->default_val[PLS_ASCAT] = 0;
    ap->lo[PLS_OCT]     = 0;
    ap->hi[PLS_OCT]     = 24;
    ap->default_val[PLS_OCT] = 0;
    ap->lo[PLS_BEND]    = 0;
    ap->hi[PLS_BEND]    = 24;
    ap->default_val[PLS_BEND] = 0;
    ap->lo[PLS_SEED]    = 0.0;
    ap->hi[PLS_SEED]    = MAXSHORT;
    ap->default_val[PLS_SEED] = 0.0;
    if(dz->process == PULSER3) {
        ap->lo[PLS_SRATE]   = 16000;
        ap->hi[PLS_SRATE]   = 96000;
        ap->default_val[PLS_SRATE] = 44100.00;
        ap->lo[PLS_CNT] = 0;
        ap->hi[PLS_CNT] = 64;
        ap->default_val[PLS_CNT] = 0;
    }
    if(dz->process == PULSER  && dz->mode == 2) {
        ap->lo[PLS_WIDTH]   = 0;
        ap->hi[PLS_WIDTH]   = 1;
        ap->default_val[PLS_WIDTH] = 0;
    }
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
            if((exit_status = setup_pulser_application(dz))<0)
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
    int exit_status;
    exit_status = set_internalparam_data("diiii", ap);
    return(exit_status);
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
    fprintf(stdout,
    "\nGENERATE STREAMS OF PULSES\n\n"
    "USAGE: pulser NAME (mode) [infile] outfile parameters: \n"
    "\n"
    "where NAME can be any one of\n"
    "\n"
    "pulser     multi     synth\n\n"
    "Type 'pulser pulser' for more info on pulser pulser..ETC.\n");
    return(USAGE_ONLY);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"pulser"))              dz->process = PULSER;
    else if(!strcmp(prog_identifier_from_cmdline,"multi"))          dz->process = PULSER2;
    else if(!strcmp(prog_identifier_from_cmdline,"synth"))          dz->process = PULSER3;
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
    if(!strcmp(str,"pulser")) {
        fprintf(stderr,
        "USAGE: pulser pulser 1 infile outfile dur pitch\n"
        "OR:    pulser pulser 2 infile outfile dur\n"
        "OR:    pulser pulser 3 infile outfile spacedata dur\n"
        "minrise maxrise minsus maxsus mindecay maxdecay speed scatter\n"
        "[-eexpr] [-Eexpd] [-ppscat] [-aascat] [-ooctav] [-bbend] [-sseed] (-wwidth])\n"
        "ITERATE AN INPUT AND ENVELOPE THE SOUND GENERATED\n"
        "TO PRODUCE PITCHED PACKETS WHICH ARE OUTPUT IN A STREAM.\n"
        "MODE 1 : packets take spectral brightness (only) from source,\n"
        "         and pitch from pitch parameter.\n"
        "MODE 2 : packets derived from (start of) source.\n"
        "MODE 3 : packets derived from random startpoints within source.\n"
        "DUR          Duration of output stream.\n"
        "SPACEDATA    Zero (stereo output) ~~OR~~ gives 8-chan output, and expects...\n"
        "             String listing chans to use (vals from 1-8: in order, no repeats).\n"
        "             OR datafile (8-channel output).\n"
        "             Datafile consists of set of lines, each having\n"
        "             Time & List_of_chans_to_use (vals in range 1-8: no repeated vals).\n"
        "PITCH        (Mode 1) Midipitch of packets, Range 24 to 96 (may vary over time).\n"
        "MIN,MAXRISE  Min and max risetime of packet envelope (Range 0.002 to 0.2 secs).\n"
        "             (Risetime set as random value between the 2 limits)\n"
        "MIN,MAXSUS   Min and max sustain of packet envelope (Range 0.0 to 0.2 secs).\n"
        "             (Sustain set as random value between the 2 limits)\n"
        "MIN,MAXDECAY Min and max decaytime of packet envelope (Range 0.02 to 2 secs).\n"
        "             (Decaytime set as random value between the 2 limits)\n"
        "SPEED        (Average) time between packets in output (Range 0.05 to 1sec).\n"
        "SCATTER      Randomisation of speed (0-1)\n"
        "EXPR         Rise slope (1 linear: >1 steeper: <1 shallower) (Range .25 to 4)\n"
        "EXPD         Decay slope (1 linear: >1 steeper: <1 shallower) (Range .25 to 4)\n"
        "PSCAT        Random jitter of pitch of packets: Range 0 - 1 semitones.\n"
        "ASCAT        Random jitter of amplitude of packets: Range 0 (no jitter) to 1.\n"
        "OCTAV        Amount of lower octave reinforcement : Range 0 - 1\n"
        "BEND         Amount of upward pitchbend of packets : Range 0 - 1 semitones.\n"
        "SEED         Same seed-number produces identical output on rerun (Integer >=1).\n"
        "WIDTH        Mode 3 only :Range 0-1\n"
        "             In Stereo (\"SPACEDATA\" = 0)\n"
        "             Width of scatter-of-positions of packets, across stereo panorama.\n"
        "                  Zero value produces Mono output.\n"
        "             8-chan (\"SPACEDATA\" is a file of 8-chan spatialisatio data)\n"
        "             Width of scatter-of-positions away from loudspeaker-centric.\n"
        "                  Zero value produces outputs centred in the loudspeakers.\n"
        "All params except DUR,SEED and MIN/MAX RISE,SUSTAIN,and DECAY can vary over time.\n"
        "\n");
    } else if(!strcmp(str,"multi")) {
        fprintf(stderr,
        "USAGE: pulser multi 1   infile1 [infile2 ......] outfile dur pitch\n"
        "OR:    pulser multi 2-3 infile1 [infile2 ......] infile outfile dur\n"
        "minrise maxrise minsus maxsus mindecay maxdecay speed scatter\n"
        "[-eexpr] [-Eexpd] [-ppscat] [-aascat] [-ooctav] [-bbend] [-sseed] [-r]\n"
        "\n"
        "ITERATE AN INPUT AND ENVELOPE THE SOUND GENERATED\n"
        "TO PRODUCE PITCHED PACKETS WHICH ARE OUTPUT IN A STREAM.\n"
        "INPUT SOURCE FILES ARE USED IN RANDOMLY PERMUTED SEQUENCES\n"
        "\n"
        "MODE 1 : packets take spectral brightness (only) from sources,\n"
        "         and pitch from pitch parameter.\n"
        "MODE 2 : packets derived from (start of) sources.\n"
        "MODE 3 : packets derived from random startpoints within sources.\n"
        "\n"
        "DUR          Duration of output stream.\n"
        "PITCH        (Mode 1) Midipitch of packets, Range 24 to 96 (may vary over time).\n"
        "MIN,MAXRISE  Min and max risetime of packet envelope (Range 0.002 to 0.2 secs).\n"
        "             (Risetime set as random value between the 2 limits)\n"
        "MIN,MAXSUS   Min and max sustain of packet envelope (Range 0.0 to 0.2 secs).\n"
        "             (Sustain set as random value between the 2 limits)\n"
        "MIN,MAXDECAY Min and max decaytime of packet envelope (Range 0.02 to 2 secs).\n"
        "             (Decaytime set as random value between the 2 limits)\n"
        "SPEED        (Average) time between packets in output (Range 0.05 to 1sec).\n"
        "SCATTER      Randomisation of speed (0-1)\n"
        "EXPR         Rise slope (1 linear: >1 steeper: <1 shallower) (Range .25 to 4)\n"
        "EXPD         Decay slope (1 linear: >1 steeper: <1 shallower) (Range .25 to 4)\n"
        "PSCAT        Random jitter of pitch of packets: Range 0 - 1 semitones.\n"
        "ASCAT        Random jitter of amplitude of packets: Range 0 (no jitter) to 1.\n"
        "OCTAV        Amount of lower octave reinforcement : Range 0 - 1\n"
        "BEND         Amount of upward pitchbend of packets : Range 0 - 1 semitones.\n"
        "SEED         Same seed-number produces identical output on rerun (Integer >=1).\n"
        "-r           Selection of source file used for next packet, entirely random."
        "             (Default, all files used once, in random order, then new randorder).\n"
        "\n"
        "All params except DUR, SEED and MIN/MAX RISE,SUSTAIN,DECAY can vary over time.\n"
        "\n");
    } else if(!strcmp(str,"synth")) {
        fprintf(stderr,
        "USAGE: pulser synth 1-3 outfile partials-data dur pitch\n"
        "minrise maxrise minsus maxsus mindecay maxdecay speed scatter\n"
        "[-eexpr] [-Eexpd] [-ppscat] [-aascat] [-ooctav] [-bbend] [-sseed] [-Ssrate]\n"
        "[-ccount]\n"
        "\n"
        "ITERATE SYNTHESIZED WAVE-PACKETS DEFINED BY PARTIALS-DATA.\n"
        "\n"
        "PARTIALS DATA (pno = partial-number)\n"
        "\n"
        "MODE 1 : packets all have the same spectrum.\n"
        "   Partials data is pairs of \"pno level\"\n"
        "MODE 2 : spectrum changes through time from one packet to next.\n"
        "MODE 3 : spectrum changes at random from one packet to next (times ignored).\n"
        "   Partials data is lines of format \"time pno1 level1 [ pno2 level2 ...]\" \n"
        "   pno range 1-64 : level range -1 to 1 (-ve vals invert the phase).\n"
        "   Times must begin at zero and increase.\n"
        "\n"
        "DUR          Duration of output stream.\n"
        "PITCH        Midipitch of packets, Range 24 to 96 (may vary over time).\n"
        "MIN,MAXRISE  Min and max risetime of packet envelope (Range 0.002 to 0.2 secs).\n"
        "             (Risetime set as random value between the 2 limits)\n"
        "MIN,MAXSUS   Min and max sustain of packet envelope (Range 0.0 to 0.2 secs).\n"
        "             (Sustain set as random value between the 2 limits)\n"
        "MIN,MAXDECAY Min and max decaytime of packet envelope (Range 0.02 to 2 secs).\n"
        "             (Decaytime set as random value between the 2 limits)\n"
        "SPEED        (Average) time between packets in output (Range 0.05 to 1sec).\n"
        "SCATTER      Randomisation of speed (0-1)\n"
        "EXPR         Rise slope (1 linear: >1 steeper: <1 shallower) (Range .25 to 4)\n"
        "EXPD         Decay slope (1 linear: >1 steeper: <1 shallower) (Range .25 to 4)\n"
        "PSCAT        Random jitter of pitch of packets: Range 0 - 1 semitones.\n"
        "ASCAT        Random jitter of amplitude of packets: Range 0 (no jitter) to 1.\n"
        "OCTAV        Amount of lower octave reinforcement : Range 0 - 1\n"
        "BEND         Amount of upward pitchbend of packets : Range 0 - 1 semitones.\n"
        "SEED         Same seed-number produces identical output on rerun (Integer >=1).\n"
        "SRATE        Sampling rate for synthesis (44100 to 96000).\n"
        "COUNT        Number of partials to use in synthesis.\n"
        "             Count must lie between 1 and 64 (maximum partials)\n"
        "             Fractional vals: e.g. 2.4 uses partials 1 & 2 at specified level,\n"
        "             and partial 3 at 0.4 of specified level\n"
        "             Zero (default) means (ignore this param and) use ALL partials.\n"
        "\n"
        "All params except DUR,SEED,SRATE and MIN/MAX RISE,SUS,DECAY can vary over time.\n"
        "\n");
    } else
        fprintf(stderr,"Unknown option '%s'\n",str);
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

/**************************** DO_PULSER_PREPROCESS *************************************************************
 *
 *  in buffer (insams[0]) PACKET BUFFER (maxpacket + insams[0])     OUTPUT BUFFER (maxpacket * 2)
 *  |-----------------|------------------------------------------|-----------------------------------------|
 *
 */

int do_pulser_preprocess(int ochancnt,dataptr dz)
{
    int exit_status;
    double srate = (double)dz->infile->srate, *sintab;
    int maxpacket, seccnt, temp, insams, n, m, maxpar;
    int mininsams, minoutsams, maxmax;
    double maxtranspos, maxscat, maxbend, maxmaxd, maxmind;
    unsigned int big_buffer_size, obuflen;
    if(dz->brksize[PLS_MAXSUS]) {
        if(get_maxvalue_in_brktable(&maxmaxd,PLS_MAXSUS,dz) < 0)
            return(DATA_ERROR);
    } else
        maxmaxd = dz->param[PLS_MAXSUS];

    if(dz->brksize[PLS_MINSUS]) {
        if(get_maxvalue_in_brktable(&maxmind,PLS_MINSUS,dz) < 0)
            return(DATA_ERROR);
    } else
        maxmind = dz->param[PLS_MINSUS];

    if(maxmaxd < maxmind)
        maxmaxd = maxmind;
    maxmax = (int)round(maxmaxd * srate);

    dz->iparam[PLS_MAXRISE] = (int)round(dz->param[PLS_MAXRISE] * srate);
    dz->iparam[PLS_MINRISE] = (int)round(dz->param[PLS_MINRISE] * srate);
    if(!dz->brksize[PLS_MINSUS])
        dz->iparam[PLS_MINSUS] = (int)round(dz->param[PLS_MINSUS] * srate);
    if(!dz->brksize[PLS_MAXSUS])
        dz->iparam[PLS_MAXSUS] = (int)round(dz->param[PLS_MAXSUS] * srate);
    if(!dz->brksize[PLS_MINSUS] && !dz->brksize[PLS_MAXSUS]) {
        if(dz->iparam[PLS_MAXSUS] < dz->iparam[PLS_MINSUS]) {
            temp = dz->iparam[PLS_MAXSUS];
            dz->iparam[PLS_MAXSUS] = dz->iparam[PLS_MINSUS];
            dz->iparam[PLS_MINSUS] = temp;
        }
    }
    dz->iparam[PLS_MAXDECAY] = (int)round(dz->param[PLS_MAXDECAY] * srate);
    dz->iparam[PLS_MINDECAY] = (int)round(dz->param[PLS_MINDECAY] * srate);
    if(dz->iparam[PLS_MAXRISE] < dz->iparam[PLS_MINRISE]) {
        temp = dz->iparam[PLS_MAXRISE];
        dz->iparam[PLS_MAXRISE] = dz->iparam[PLS_MINRISE];
        dz->iparam[PLS_MINRISE] = temp;
    }
    if(dz->iparam[PLS_MAXDECAY] < dz->iparam[PLS_MINDECAY]) {
        temp = dz->iparam[PLS_MAXDECAY];
        dz->iparam[PLS_MAXDECAY] = dz->iparam[PLS_MINDECAY];
        dz->iparam[PLS_MINDECAY] = temp;
    }
    maxpacket = dz->iparam[PLS_MAXRISE] + maxmax + dz->iparam[PLS_MAXDECAY];

    if(dz->process == PULSER3) {
        if((dz->parray[2] = (double *)malloc((SYNTH_TABSIZE + 1) * sizeof(double)))==NULL) {    // sin table
            sprintf(errstr,"INSUFFICIENT MEMORY for sine table.\n");
            return(MEMORY_ERROR);
        }
        sintab = dz->parray[2];
        for(n=0;n<SYNTH_TABSIZE;n++)
            sintab[n] = sin(PI * 2.0 * ((double)n/(double)SYNTH_TABSIZE));                              
        sintab[n] = sintab[0];                          /* wrap around point */

        if(dz->mode == 0)
            maxpar = dz->itemcnt;           //  Only 1 set of partials to count
        else {
            maxpar = dz->iparray[1][0];     //  Find largest set of partials
            for(n = 1;n<dz->itemcnt;n++) 
                maxpar += dz->iparray[1][n];
        }
        if((dz->parray[3] = (double *)malloc(maxpar * sizeof(double)))==NULL) { // enough pointers into sin-table for largest set of partial
            sprintf(errstr,"INSUFFICIENT MEMORY to store partial sintable pointers.\n");
            return(MEMORY_ERROR);
        }
        if((dz->parray[6] = (double *)malloc(maxpar * sizeof(double)))==NULL) { // enough spaces for partial vals of largest set of partial
            sprintf(errstr,"INSUFFICIENT MEMORY to store partial interpolated partial vals.\n");
            return(MEMORY_ERROR);
        }
        if((dz->parray[7] = (double *)malloc(maxpar * sizeof(double)))==NULL) { // enough spaces for levels of largest set of partial
            sprintf(errstr,"INSUFFICIENT MEMORY to store partial interpolated partial levels.\n");
            return(MEMORY_ERROR);
        }
        for(n=0;n < maxpar;n++)
            dz->parray[3][n] = 0.0;

        if(!LEGAL_SRATE(dz->param[PLS_SRATE])) {
            sprintf(errstr,"INVALID SAMPLING RATE ENTERED.\n");
            return DATA_ERROR;
        }
        dz->counted = 0;
        if(dz->brksize[PLS_CNT]) {
            for(n = 0,m = 1;n<dz->brksize[PLS_CNT];n++,m+=2) {
                if(dz->brk[PLS_CNT][m] < 1.0) {
                    sprintf(errstr,"Breakpoint values for partials-count cannot be less than 1.\n");
                    return(DATA_ERROR);
                }
            }
            dz->counted = 1;
        } else if(dz->param[PLS_CNT] > 0.0) {
            if(dz->param[PLS_CNT] < 1.0) {
                sprintf(errstr,"Non-zero Partials-count (%lf) cannot be less than 1.\n",dz->param[PLS_CNT]);
                return(DATA_ERROR);
            }
            dz->counted = 1;
        }
    } else if(dz->mode > 0) {
        mininsams = dz->insams[0];
        for(n= 1;n<dz->infilecnt;n++)
            mininsams = min(mininsams,dz->insams[n]);
        if(dz->brksize[PLS_PSCAT]) {
            if((exit_status = get_maxvalue_in_brktable(&maxscat,PLS_PSCAT,dz))<0)
                return exit_status;
        } else
            maxscat = dz->param[PLS_PSCAT];

        if(dz->brksize[PLS_BEND]) {
            if((exit_status = get_maxvalue_in_brktable(&maxbend,PLS_BEND,dz))<0)
                return exit_status;
        } else
            maxbend = dz->param[PLS_BEND];
        maxtranspos = maxscat + maxbend;
        maxtranspos = pow(2.0,maxtranspos/SEMITONES_PER_OCTAVE);
        minoutsams = (int)floor((double)mininsams/maxtranspos); //  min no of outsamples at max transposition
        if(minoutsams <= maxpacket) {
            sprintf(errstr,"COMBO OF TRANSPOSITION, GLISS AND SCATTER MAKE SRC POSSIBLY TOO SHORT FOR MAX PACKET SIZE.\n");
            return DATA_ERROR;
        }
    }
    maxpacket += 40;    // SAFETY
    seccnt = maxpacket/F_SECSIZE;
    if(seccnt * F_SECSIZE < maxpacket)
        seccnt++;
    maxpacket = seccnt * F_SECSIZE;
    if(dz->process == PULSER3) {
        insams = maxpacket;
        dz->srcsamps = insams;
    } else {
        insams = dz->insams[0];
        if(dz->process == PULSER2) {
            for(n=1;n<dz->infilecnt;n++)
                insams = max(insams,dz->insams[n]);
        }
    }
    insams++;   // wraparound point
    big_buffer_size = insams + (insams + maxpacket) + (maxpacket * 2);
    dz->buflen = maxpacket;                                         //  |   insams  | insams   +mxpk  mxpk mxpk
    obuflen = dz->buflen;                                           //  |-----------|-----------i----|----|----|
    if(ochancnt > 1) {                                              //  |   INBUF         PKTBUF      OBUF OVFLW
        big_buffer_size += (dz->buflen * (ochancnt - 1) * 2);       // FOR 4chan-out add to obuf     |    i----i----i----|  
        obuflen *= ochancnt;                                        //               and to ovflw              i----i----i----|     
    }                                                               // = (obufcnt - 1) for both OBUF and OVFLW = (ochancnt - 1) * 2
    if(big_buffer_size <= 0 || obuflen <= 0) {
        sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers for stereo work.\n");
        return(MEMORY_ERROR);
    }
    if((dz->bigbuf = (float *)Malloc(big_buffer_size * sizeof(float)))==NULL) {
        sprintf(errstr, "INSUFFICIENT MEMORY to create sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;                           //  INPUT BUFFER
    dz->sbufptr[1] = dz->sampbuf[1] = dz->sampbuf[0] + insams;              //  PACKET BUFFER
    dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + insams + maxpacket;  //  OUTPUT BUFFER
    dz->sbufptr[3] = dz->sampbuf[3] = dz->sampbuf[2] + obuflen;             //  OVERFLOW BUFFER
    memset((char *)dz->sampbuf[0],0,(size_t)(insams * sizeof(float)));
    memset((char *)dz->sampbuf[1],0,(size_t)((insams + maxpacket) * sizeof(float)));
    memset((char *)dz->sampbuf[2],0,(size_t)(obuflen * sizeof(float)));
    memset((char *)dz->sampbuf[3],0,(size_t)(obuflen * sizeof(float)));
    return(FINISHED);
}

/****************************** PULSER *************************/

//#define ACCEPTABLE_MAXLEVEL 0.9
#define ACCEPTABLE_MAXLEVEL 0.75

int pulser(int ochancnt,dataptr dz)
{   int exit_status, passno, permcnt = 0, k, thisprocess = dz->process, splineread = 0, chan = 0;
    double time, srate = (double)dz->infile->srate, maxsamp = 0.0, normaliser = 1.0, rr, frq = 0.0, upfrq, timerscale;
    int secondderivtab = 0;
    int packetsize, obufpos, bufstep, lastwrite, riserange, susrange, decayrange, rise, sustain, decay, n, srcsize = 0, temp;
    int obuflen = dz->buflen;
    float *buf = dz->sampbuf[0], *obuf = dz->sampbuf[2], *ovflow = dz->sampbuf[3];
    int *perm;
    if(ochancnt > 1)
        obuflen = dz->buflen * ochancnt;
    if((dz->process == PULSER3 || dz->mode == 0) && dz->brksize[PLS_PITCH] > 0) {   //  Set up cubic spline array for pitch interp, wherever needed
        splineread = 1;
        if(dz->process == PULSER3) {
            if(dz->mode > 0) 
                secondderivtab = 5;
            else             
                secondderivtab = 4;
        } // else its in parray 0
        if((dz->parray[secondderivtab] = (double *)malloc(dz->brksize[PLS_PITCH] * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store second derivs for cubic spline.\n");
            return(MEMORY_ERROR);
        }
        if((exit_status = spline(secondderivtab,dz)) < 0)
            return exit_status;
    }
    if(dz->process == PULSER3)  
        srate = 44100.0;
    else
        srate = (double)dz->infile->srate;
    if((perm = (int *)malloc(sizeof(int) * dz->infilecnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY establishing file permutations buffer.\n");
        return(MEMORY_ERROR);
    }
    dz->tempsize = (int)round(dz->param[PLS_DUR] * srate) * ochancnt;
    timerscale = (double)(PBAR_LENGTH - 1)/(double)dz->tempsize;
    riserange  = dz->iparam[PLS_MAXRISE]  - dz->iparam[PLS_MINRISE];
    if(dz->brksize[PLS_MAXSUS])
        dz->iparam[PLS_MAXSUS] = (int)round(dz->param[PLS_MAXSUS] * srate);
    if(dz->brksize[PLS_MINSUS])
        dz->iparam[PLS_MINSUS] = (int)round(dz->param[PLS_MINSUS] * srate);
    susrange   = dz->iparam[PLS_MAXSUS]   - dz->iparam[PLS_MINSUS];
    if(susrange < 0.0) {
        susrange = -susrange;
        temp = dz->iparam[PLS_MAXSUS];
        dz->iparam[PLS_MAXSUS] = dz->iparam[PLS_MINSUS];
        dz->iparam[PLS_MINSUS] = temp;
    }
    decayrange = dz->iparam[PLS_MAXDECAY] - dz->iparam[PLS_MINDECAY];
    if(dz->process == PULSER) {
        if((dz->ssampsread = fgetfbufEx(buf, dz->insams[0],dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Can't read samples from input file.\n");
            return(SYSTEM_ERROR);
        }
        srcsize = dz->insams[0];
        buf[srcsize] = buf[srcsize-1];  //  wraparound point
    }
    for(passno = 0;passno < 2;passno++) {
        dz->total_samps_written = 0;
        if(sloom) {
            fprintf(stdout,"TIME: 0\n");
            fflush(stdout);
        }
        if(passno == 0)
            print_outmessage_flush("Initial pass, Assessing level.\n");
        else
            print_outmessage_flush("Creating sound.\n");
        memset((char *)obuf,0,obuflen * sizeof(float));
        memset((char *)ovflow,0,obuflen * sizeof(float));
        time = 0.0;
        obufpos = 0;
        lastwrite = 0;
        srand((int)dz->iparam[PLS_SEED]);               //  Restart SAME random sequence
//      initrand48();
        while(time < dz->param[PLS_DUR]) {
            if(dz->process == PULSER2) {
                if(dz->vflag[0])
                    k = (int)floor(drand48() * dz->infilecnt);
                else {
                    if(permcnt % dz->infilecnt == 0) {
                        rndintperm(perm,dz->infilecnt);
                        permcnt = 0;
                    }
                    k = perm[permcnt];
                    permcnt++;
                }
                sndseekEx(dz->ifd[k],0,0);
                if((dz->ssampsread = fgetfbufEx(buf, dz->insams[k],dz->ifd[k],0)) < 0) {
                    sprintf(errstr,"Can't (re)read samples from file %d.\n",k+1);
                    return(SYSTEM_ERROR);
                }
                srcsize = dz->insams[k];
                buf[srcsize] = buf[srcsize-1];
            }
            if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
                return(exit_status);
            if(dz->iparray != NULL) {
                if((chan = get_chan_to_use(time,dz)) < 0) {
                    sprintf(errstr,"FAILED TO FIND CHANNEL TO USE\n");
                    return(PROGRAM_ERROR);
                }
            }
            if(splineread)
                dz->param[PLS_PITCH] = splint(secondderivtab,time,dz);
            rise = (int)round(drand48() * riserange);
            rise += dz->iparam[PLS_MINRISE];
            rise = min(rise,dz->iparam[PLS_MAXRISE]);
            sustain = (int)round(drand48() * susrange);
            sustain += dz->iparam[PLS_MINSUS];
            sustain = min(sustain,dz->iparam[PLS_MAXSUS]);
            decay = (int)round(drand48() * decayrange);
            decay += dz->iparam[PLS_MINDECAY];
            decay = min(decay,dz->iparam[PLS_MAXDECAY]);
            if(dz->process == PULSER3) {
                frq = miditohz(dz->param[PLS_PITCH]);       //  Use input frq to generate packet
                synthesize_src(frq,srate,time,dz);
                frq = 1.0;                                  //  Use scatter and bend values to transpose it
            } else {
                if(dz->mode == 0) 
                    frq = miditohz(dz->param[PLS_PITCH]);
                else
                    frq = 1.0;  //  Frq == transposition in this case
            }
            if(dz->param[PLS_PSCAT] > 0.0) {
                rr = (drand48() * 2.0) - 1.0;   //  +- 1 semitone
                rr *= dz->param[PLS_PSCAT];     //  +- PLS_PSCAT semitones
                rr = pow(2.0,rr/SEMITONES_PER_OCTAVE);
                frq *= rr;
            }
            if(dz->param[PLS_BEND] > 0.0) {
                rr = pow(2.0,dz->param[PLS_BEND]/SEMITONES_PER_OCTAVE);
                upfrq = frq * rr;
            } else
                upfrq = frq;
            if((exit_status = generate_packet(&packetsize,frq,upfrq,rise,sustain,decay,srcsize,srate,dz))<0)
                return(exit_status);
            if((exit_status = output_packet(obufpos,packetsize,chan,&lastwrite,ochancnt,dz))<0)
                return(exit_status);
            bufstep = (int)round(dz->param[PLS_SPEED] * srate);
            if(dz->param[PLS_SCAT] > 0.0) {
                rr = (drand48() * 2.0) - 1.0;   //  Range -1 to +1
                rr /= 2.0;                      //  Range -1/2 to +1/2
                rr *= dz->param[PLS_SCAT];      //  Range = rand fraction of -1/2 to +1/2
                bufstep = (int)round((double)bufstep * (1.0 + rr));
            }
            if(ochancnt > 1)
                obufpos += bufstep * ochancnt;  //  Advance to write-position for next packet.
            else
                obufpos += bufstep;             //  Advance to write-position for next packet.

            while(obufpos >= obuflen) {         //  If this is beyond output buffer end, we can write the output buffer
                                                //  And if there's a leap of silence to next output event,
                if(passno == 0) {               //  loop keeps outputting (possibly empty) buffers until new output time is reached
                    for(n=0;n<obuflen;n++)  
                        maxsamp = max(maxsamp,fabs(obuf[n]));
                    if(sloom)
                        showpulsertime(thisprocess,obuflen,timerscale,dz);
                } else {
                    for(n=0;n<obuflen;n++)
                        obuf[n] = (float)(obuf[n] * normaliser);
                    if(sloom)
                        dz->process = DISTORT_PULSED;
                    if((exit_status = write_samps(obuf,obuflen,dz))<0)
                        return(exit_status);
                    if(sloom)
                        dz->process = thisprocess;
                }
                memcpy((char *)obuf,(char *)ovflow,obuflen * sizeof(float)); // Copy back overflow buffer
                memset((char *)ovflow,0,obuflen * sizeof(float));           //  Clean overflow buffer
                obufpos -= obuflen;
                lastwrite -= obuflen;
                lastwrite = max(0,lastwrite);
            }
            time += (double)bufstep/srate;
        }
        if(passno == 0) {               //  loop keeps outputting (possibly empty) buffers until new output time is reached
            for(n=0;n<lastwrite;n++)    
                maxsamp = max(maxsamp,fabs(obuf[n]));
            if(maxsamp > ACCEPTABLE_MAXLEVEL)
                normaliser = ACCEPTABLE_MAXLEVEL/maxsamp;
            if(sloom)
                showpulsertime(thisprocess,lastwrite,timerscale,dz);
        } else {
            if(lastwrite > 0) {
                for(n=0;n<lastwrite;n++)
                    obuf[n] = (float)(obuf[n] * normaliser);
                if(sloom)
                    dz->process = DISTORT_PULSED;
                if((exit_status = write_samps(obuf,lastwrite,dz))<0)
                    return(exit_status);
                if(sloom)
                    dz->process = thisprocess;
            }
        }
    }
    return FINISHED;
}

/**************************** OUTPUT_PACKET *****************************/

int output_packet(int obufpos,int packetsize,int chan,int *lastwrite,int ochancnt,dataptr dz)
{
    int n, obufpos1, obufpos2;
    int lhs = 0;
    float *pbuf = dz->sampbuf[1], *obuf = dz->sampbuf[2];
    double leftej = 0, range = 0, pos = 0, leftgain = 0, rightgain = 0;
    if(ochancnt == 2) {
        leftej = -dz->param[PLS_WIDTH];
        range = dz->param[PLS_WIDTH] * 2.0;
        pos   = (drand48() * range) + leftej;       //   In range -1 to +1
        pancalc(pos,&leftgain,&rightgain);
        for(n=0;n<packetsize;n++) {
            obuf[obufpos] = (float)(obuf[obufpos] + (pbuf[n] * leftgain));
            obufpos++;
            obuf[obufpos] = (float)(obuf[obufpos] + (pbuf[n] * rightgain));
            obufpos++;
        }                                           //                                               <- - - 2 - - ->
    } else if(ochancnt == 8) {                      //                                      3      1/2      4      1/2      5
                                                    //  e.g for centring on lspkr 4         |      way      |      way      |
        if(dz->param[PLS_WIDTH] > 0.0) {            //                                      |               |<- - - 2 - - ->|
            leftej = -dz->param[PLS_WIDTH];         //  Total range (4~5) = 2 and           |       #       |-------#-------|
            range = dz->param[PLS_WIDTH] * 2.0;     //  Range reduced to 1-to-width*2 then  |       #       |-------#-      |       
            pos   = (drand48() * range) + leftej;   //  centred on central lspkr(4) (pos=0) |       #   ----|----   #       |
                                                    //  So range now -width to +width.      |       #xxx----|----xxx#       |
            pos  /= 2.0;                            //  Convert from range btwn 1/2ways to  |       e.g.-1/3| +1/3          |
                                                    //  range btwn central & adjacent lspkr |xxxxxxxxxxx----|-----xxxxxxxxxx|
            if(pos < 0.0) {                         //                                      |           -1/6| +1/6          |
                lhs = 1;                            //  vals at LHS convert to to range 3~4 |               |               |
                pos = 1.0 - pos;                    //                                      |            5/6|               |
            } else {                                //  vals at RHS convert to to range 4~5 |               | -1/6          |
                lhs = 0;
                pos = -pos;
            }
            pancalc(pos,&leftgain,&rightgain);
            obufpos += chan;
            if(lhs) {
                obufpos1 = obufpos - 1;
                if(obufpos1 < 0)
                    obufpos1 += 8;
                obufpos2 = obufpos;
            } else {
                obufpos2 = obufpos + 1;
                if(obufpos2 >= 8)
                    obufpos2 = 0;
                obufpos1 = obufpos;
            }
            for(n=0;n<packetsize;n++) {
                obuf[obufpos1] = (float)(obuf[obufpos1] + (pbuf[n] * leftgain));
                obuf[obufpos2] = (float)(obuf[obufpos2] + (pbuf[n] * rightgain));
                obufpos1 += 8;
                obufpos2 += 8;
                obufpos += 8;
            }
            obufpos -= chan;

        } else {
                                                                    //              +               +               +               +     
            obufpos += chan;                                        //      obp    sch    obp+8     sch  obp+16     sch   obp+8n   sch
            for(n=0;n<packetsize;n++) {                             //      |_______|_______|_______|_______|_______|_______|.......|
                obuf[obufpos] = (float)(obuf[obufpos] + pbuf[n]);   //      0 0 0 0 X 0 0 0 0 0 0 0 X 0 0 0 0 0 0 0 X 0 0 0 0      last
                obufpos += 8;                                       //      |      1st      |      2nd      |   e.g.last            ptr
            }                                                       //      |     write     |     write     |     write     ^     advance
            obufpos -= chan;                                        //      |               |               |               |     returns
        }                                                           //                                                      |_<<<_  to
    } else {
        for(n=0;n<packetsize;n++) {
            obuf[obufpos] = (float)(obuf[obufpos] + pbuf[n]);
            obufpos++;
        }
    }
    if(obufpos > *lastwrite)        //  Keep record of maximumn output sample position in buffer
        *lastwrite = obufpos;
    return FINISHED;
}

/********************** GENERATE_PACKET ********************************/

int generate_packet(int *packetsize,double frq,double upfrq,int rise,int sustain,int decay,int srcsize,double srate,dataptr dz)
{
    int delaytype = 0;
    int n, m, k = 0, delay, writemax = 0, cyclecnt, updelay, thisdelay;
    double maxsamp = 0.0, normaliser = 1.0, env, rr, amp, delstep, del;
    int maxrange, pakrange, rndrange, srclo, srchi;
    double srcpos, diff, istep, frac, frqstep;
    float *pbuf = dz->sampbuf[1], *ibuf = dz->sampbuf[0];

    memset((char *)pbuf,0,(dz->ssampsread + dz->buflen) * sizeof(float));
    *packetsize = rise + sustain + decay;
    if(dz->process == PULSER3)
        delaytype = 0;
    else {
        switch(dz->mode) {
        case(0): delaytype = 1; break;
        case(1): 
        case(2): delaytype = 0; break;
        }
    }
    if(delaytype) {
        delay = (int)round(srate/frq);
        updelay = (int)round(srate/upfrq);
        delstep = (double)(delay - updelay)/((double)*packetsize/(double)updelay);  //  Approx only
        n = 0;
        cyclecnt = 0;
        del = (double)delay;
        thisdelay = delay;
        while(n < *packetsize) {
            if(dz->param[PLS_ASCAT] > 0) {
                if(cyclecnt % 2 == 0) {
                    for(m = 0,k = n;m<dz->ssampsread;m++,k++)
                        pbuf[k] = (float)(pbuf[k] + ibuf[m]);
                } else {
                    amp = drand48() * dz->param[PLS_ASCAT];
                    amp = 1.0 - amp;
                    for(m = 0,k = n;m<dz->ssampsread;m++,k++)
                        pbuf[k] = (float)(pbuf[k] + (ibuf[m] * amp));
                }
            } else {
                for(m = 0,k = n;m<dz->ssampsread;m++,k++)
                    pbuf[k] = (float)(pbuf[k] + ibuf[m]);
            }
            n += thisdelay;
            del -= delstep;
            thisdelay = (int)round(del);
            cyclecnt++;
        }
        writemax = k;
    } else {
        frqstep = (upfrq - frq)/(double)(*packetsize);
        k = 0;
        if(dz->process != PULSER3 && dz->mode == 2) {
            maxrange = (int)floor((double)srcsize/upfrq);       //  No of possible steps in src, at given frq : approx, use maxfrq (worst case)
            pakrange = (int)ceil((double)(*packetsize)/upfrq);  //  No of steps in src to make packet, at given frq
            rndrange = maxrange - pakrange;                     //  Available range of possible start-positions in src
            srcpos = drand48() * (double)rndrange;
        } else
            srcpos = 0.0;
        while(k < *packetsize) {
            srclo = (int)floor(srcpos);
            srchi = (int)ceil(srcpos);
            diff  = srcpos - (double)srclo;
            istep = ibuf[srchi] - ibuf[srclo];
            frac  = istep * diff;
            pbuf[k] = (float)(ibuf[srclo] + frac);
            srcpos += frq;
            frq += frqstep;
            k++;
        }
    }
    // Create attack and decay envelopes of packet  
    if(dz->param[PLS_EXP] != 1.0) {
        for(n=0;n < rise;n++) {
            env = (double)n/(double)rise;
            env = pow(env,dz->param[PLS_EXP]);
            pbuf[n] = (float)(pbuf[n] * env);
        }
    } else {
        for(n=0;n < rise;n++) {
            env = (double)n/(double)rise;
            pbuf[n] = (float)(pbuf[n] * env);
        }
    }
    if(dz->param[PLS_EXP2] != 1.0) {
        for(n=(*packetsize)-1,m=0;m < decay;n--,m++) {
            env = (double)m/(double)decay;
            env = pow(env,dz->param[PLS_EXP2]);
            pbuf[n] = (float)(pbuf[n] * env);
        }
    } else {
        for(n=(*packetsize)-1,m=0;m < decay;n--,m++) {
            env = (double)m/(double)decay;
            pbuf[n] = (float)(pbuf[n] * env);
        }
    }
    if(dz->process == PULSER3 || dz->mode == 0) {
        for(n= *packetsize;n < writemax;n++)        //  SAFETY : Zero rest of packet, beyond env end
            pbuf[n] = 0.0f;
    }
    for(n=0;n < *packetsize;n++)
        maxsamp = max(maxsamp,fabs(pbuf[n]));       
    normaliser = ACCEPTABLE_MAXLEVEL/maxsamp;   //  Normalise packet loudness
    if(dz->param[PLS_ASCAT] > 0.0) {            //  If amplitude jittered
        rr = drand48() * dz->param[PLS_ASCAT];  //  Get random amplitude reduction
        rr = 1.0 - rr;                          //  NB rr 0 = no-reduction, rr 1 = max reduxtion
        normaliser *= rr;                       
    }
    for(n=0;n < *packetsize;n++)            
        pbuf[n] = (float)(pbuf[n] * normaliser);
    return FINISHED;
}

/*********************** RNDINTPERM ************************/

void rndintperm(int *perm,int cnt)
{
    int n,t,k;
    memset((char *)perm,0,cnt * sizeof(int));
    for(n=0;n<cnt;n++) {
        t = (int)(drand48() * (double)(n+1)); /* TRUNCATE */
        if(t==n) {
            for(k=n;k>0;k--)
                perm[k] = perm[k-1];
            perm[0] = n;
        } else {
            for(k=n;k>t;k--)
                perm[k] = perm[k-1];
            perm[t] = n;
        }
    }
}

/*********************** SHOWPULSERTIME ************************/

void showpulsertime(int thisprocess,int samps_processed,double timerscale,dataptr dz)
{
    dz->total_samps_written += samps_processed;
    dz->process = DISTORT_PULSED;
    fprintf(stdout,"TIME: %lf\n",min((double)(PBAR_LENGTH-1),(double)dz->total_samps_written * timerscale));
    fflush(stdout);
    dz->process = thisprocess;
}

/******************************** SYNTHESIZE_SRC ********************************/

int synthesize_src(double frq,double srate,double time,dataptr dz)
{
    int n,i,j, *dataindex, *datasize, isfrac = 0, partialscnt, thispartialscnt, nextpartialscnt, nextlopar;
    int loindex, hiindex;
    double loval, hival, valdiff, timefrac, val, level, onehzincr, parfrac = 0.0,/* nexttime,*/ partialdiff, leveldiff;
    float *ibuf = dz->sampbuf[0];
    double *partials = dz->parray[0], *levels = dz->parray[1];
    double *sintab = dz->parray[2], *sinptr = dz->parray[3], *datatime, *thispartials = dz->parray[6], *thislevels = dz->parray[7];
    int sampcnt = 0, lopar;
    if(dz->mode == 0) {
        lopar = 0;                  //  Fixed set of partials
        partialscnt = dz->itemcnt;
        for(n=0;n < partialscnt;n++) {
            thispartials[n] = partials[n];
            thislevels[n]   = levels[n];
        }
    } else {
        datatime = dz->parray[4];   //  Timed sets of partials
        dataindex = dz->iparray[0];
        datasize  = dz->iparray[1];
        if(dz->mode == 1) {         //  Get partials at given time
            n = 0;
            while(time >= datatime[n]) {
                n++;
                if(n == dz->itemcnt)
                    break;
            }
            n--;
            lopar = dataindex[n];
            partialscnt = datasize[n];
            if(n == dz->itemcnt - 1) {                                  //  Use final partials set
                for(n=0,i = lopar;n < partialscnt;n++,i++) {
                    thispartials[n] = partials[i];
                    thislevels[n]   = levels[i];
                }
            } else {                                                    //  Interpolate partials
                thispartialscnt = partialscnt;
//                nexttime  = datatime[n+1];
                timefrac  = (time - datatime[n])/(datatime[n+1] - datatime[n]);
                nextlopar = dataindex[n+1];
                nextpartialscnt = datasize[n+1];
                partialscnt = max(thispartialscnt,nextpartialscnt);
                for(n=0,i = lopar, j = nextlopar;n < partialscnt;n++,i++,j++) {
                    if(n < thispartialscnt && n < nextpartialscnt) {    //  Both spectra have this partial
                        partialdiff  = partials[j] - partials[i];       //  interp
                        partialdiff *= timefrac;
                        thispartials[n] = partials[i] + partialdiff;
                        leveldiff    = levels[j] - levels[i];
                        leveldiff   *= timefrac;
                        thislevels[n] = levels[i] + leveldiff;
                    } else if(n > thispartialscnt) {                    //  More partials in next spectrum
                        thispartials[n] = partials[j];                  //  New partial rises to towards full level
                        thislevels[n] = levels[j] * timefrac;
                    } else {                                            //  Less partials in next spectrum
                        thispartials[n] = partials[i];                  //  Current partial level sinks towards zero
                        thislevels[n] = levels[i] * (1.0 - timefrac);
                    }
                }
            }
        } else  {                   //  Get partials at random
            n = (int)floor(drand48() * (double)dz->itemcnt);
            lopar = dataindex[n];
            partialscnt = datasize[n];
            for(n=0,i = lopar;n < partialscnt;n++,i++) {
                thispartials[n] = partials[i];
                thislevels[n]   = levels[i];
            }
        }
    }
    onehzincr = (double)SYNTH_TABSIZE/srate;
    memset((char *)ibuf,0,dz->srcsamps * sizeof(float));
    for(n=0;n<partialscnt;n++)      //  Zero sine-table pointers for all partials
        sinptr[n] = 0.0;
    if(dz->counted) {                                                   //  If we are counting how many partials to use
        partialscnt = min(partialscnt,(int)floor(dz->param[PLS_CNT]));  //  Find number of full-level partials
        if((parfrac = dz->param[PLS_CNT] - partialscnt) > 0.0)          //  And get level of any attenuated partial
            isfrac = 1;
    }
    while(sampcnt < dz->srcsamps) {
        for(n=0;n < partialscnt;n++) {
            loindex = (int)floor(sinptr[n]);                //  Read from sintable, using partial-increment
            hiindex = loindex + 1;
            loval   = sintab[loindex];
            hival   = sintab[hiindex];
            valdiff = hival - loval;
            timefrac = sinptr[n] - (double)loindex;
            val = loval + (valdiff * timefrac);
            level = thislevels[n];                          //  Read corresponding level
            val *= level;
            ibuf[sampcnt] = (float)(ibuf[sampcnt] + val);
            incr_sinptr(n,onehzincr,frq,dz);    //  Track (modify if ness) the partial-incr value for this partial
        }
        if(isfrac) {
            loindex = (int)floor(sinptr[n]);
            hiindex = loindex + 1;
            loval   = sintab[loindex];
            hival   = sintab[hiindex];
            valdiff = hival - loval;
            timefrac = sinptr[n] - (double)loindex;
            val = loval + (valdiff * timefrac);
            level = thislevels[n];
            val *= level * parfrac;                         //  Scale by fractional partial level
            ibuf[sampcnt] = (float)(ibuf[sampcnt] + val);
            incr_sinptr(n,onehzincr,frq,dz);    //  Track (modify if ness) the partial-incr value for this partial
        }
        sampcnt++;
    }
    return FINISHED;
}

/**************************** INCR_SINPTR ****************************/

void incr_sinptr(int n,double onehzincr,double frq,dataptr dz)
{
    double *sinptr = dz->parray[3], *partials = dz->parray[6];
    double thisincr = partials[n] * onehzincr;
    thisincr *= frq;
    sinptr[n] += thisincr;
    if(sinptr[n] >= SYNTH_TABSIZE)
        sinptr[n] -= (double)SYNTH_TABSIZE;
}

/**************************** HANDLE_THE_SPECIAL_DATA ****************************/

int handle_the_special_data(char *str,dataptr dz)
{
    double dummy = 0.0, lasttime = 0.0, *datatime;
    FILE *fp;
    int cnt = 0, linecnt, datacnt, thisdatacnt, *dataindex, *datasize;
    char temp[8000], *p;
    if((fp = fopen(str,"r"))==NULL) {
        sprintf(errstr,"Cannot open file %s to read times.\n",str);
        return(DATA_ERROR);
    }
    switch(dz->process) {
    case(PULSER3):
        linecnt = 0;
        if(dz->mode == 0) {         //  Read partial-no level pairs
            while(fgets(temp,8000,fp)!=NULL) {
                p = temp;
                while(isspace(*p))
                    p++;
                if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
                    continue;
                cnt = 0;
                while(get_float_from_within_string(&p,&dummy)) {
                    if(cnt==0) {
                        if(dummy < 1.0 || dummy > 64.0) {
                            sprintf(errstr,"Partial numbers must be >= 1.0 and no more than 64.0\n");
                            return(DATA_ERROR);
                        }
                    } else {
                        if(dummy < -1.0 || dummy > 1.0) {
                            sprintf(errstr,"Partial levels must lie between -1.0 and 1.0\n");
                            return(DATA_ERROR);
                        }
                    }
                    cnt++;
                }
                if(!EVEN(cnt)) {
                    sprintf(errstr,"Invalid number of entries (%d) on line %d in file %s\n",cnt,linecnt+1,str);
                    return(DATA_ERROR);
                }
                linecnt++;
            }
            if(linecnt == 0) {
                sprintf(errstr,"No data found in partials data file.\n");
                return(DATA_ERROR);
            }
            if((dz->parray[0] = (double *)malloc(linecnt * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to store partial number data.\n");
                return(MEMORY_ERROR);
            }
            if((dz->parray[1] = (double *)malloc(linecnt * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to store partial level data.\n");
                return(MEMORY_ERROR);
            }
            fseek(fp,0,0);
            dz->itemcnt = 0;
            while(fgets(temp,8000,fp)!=NULL) {
                p = temp;
                while(isspace(*p))
                    p++;
                if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
                    continue;
                cnt = 0;
                while(get_float_from_within_string(&p,&dummy)) {
                    switch(cnt) {
                    case(0):
                        dz->parray[0][dz->itemcnt] = dummy;
                        break;
                    case(1):
                        dz->parray[1][dz->itemcnt] = dummy;
                        dz->itemcnt++;
                        break;
                    }
                    cnt++;
                }
            }
        } else {            //  Read time partial-no level [partial-no2 level2 ...] lines
            while(fgets(temp,8000,fp)!=NULL) {  //  Count (non-empty) data-lines
                p = temp;
                while(isspace(*p))
                    p++;
                if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
                    continue;
                linecnt++;
            }
            if(linecnt < 2) {
                sprintf(errstr,"File %s must have more than one line of data.\n",str);
                return(MEMORY_ERROR);
            }
            if((dz->parray[4] = (double *)malloc(linecnt * sizeof(double)))==NULL) {    //  Store times
                sprintf(errstr,"INSUFFICIENT MEMORY to store partial times.\n");
                return(MEMORY_ERROR);
            }
            if((dz->iparray[0] = (int *)malloc(linecnt * sizeof(int)))==NULL) {         //  Store array index to partial data
                sprintf(errstr,"INSUFFICIENT MEMORY to store index to partials.\n");
                return(MEMORY_ERROR);
            }
            if((dz->iparray[1] = (int *)malloc(linecnt * sizeof(int)))==NULL) {         //  Store array index to partial count
                sprintf(errstr,"INSUFFICIENT MEMORY to store sizes of spectra.\n");
                return(MEMORY_ERROR);
            }
            dz->itemcnt = linecnt;
            datatime  = dz->parray[4];
            dataindex = dz->iparray[0];
            datasize  = dz->iparray[1];
            fseek(fp,0,0);
            linecnt = 0;
            datacnt = 0;
            while(fgets(temp,8000,fp)!=NULL) {  //  Check data validity, store times and count data entries
                p = temp;
                while(isspace(*p))
                    p++;
                if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
                    continue;
                cnt = 0;
                while(get_float_from_within_string(&p,&dummy)) {
                    switch(cnt) {
                    case(0):
                        if(linecnt == 0) {
                            if(dummy != 0) {
                                sprintf(errstr,"First time in partials data (%lf) must be zero.\n",dummy);
                                return(DATA_ERROR);
                            } else
                                lasttime = dummy;
                        } else {
                            if(dummy <= lasttime) {
                                sprintf(errstr,"Times do not advance at line %d in partials data.\n",linecnt+1);
                                return(DATA_ERROR);
                            }
                        }
                        datatime[linecnt] = dummy;  
                        break;
                    default:
                        if(ODD(cnt)) {  // ODD entries, partial numbers
                            if(dummy < 1.0 || dummy > 64.0) {
                                sprintf(errstr,"Invalid partial (%lf) (Range 1 - 64) on line %d.\n",dummy,linecnt+1);
                                return(DATA_ERROR);
                            }
                        } else { // EVEN values are levels
                            if(dummy < -1.0 || dummy > 1.0) {
                                sprintf(errstr,"Invalid partial level (%lf) (Range -1 to +1) on line %d.\n",dummy,linecnt+1);
                                return(DATA_ERROR);
                            }
                            datacnt++;  //  Count the pairs-of-data (but NOT the time entries)
                        }
                        break;
                    }
                    cnt++;
                }
                if(cnt < 3 || EVEN(cnt)) {
                    sprintf(errstr,"Invalid number of entries (%d) on line %d\n",cnt,linecnt+1);
                    return(DATA_ERROR);
                }
                linecnt++;
            }
            if((dz->parray[0] = (double *)malloc(datacnt * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to store partial number data.\n");
                return(MEMORY_ERROR);
            }
            if((dz->parray[1] = (double *)malloc(datacnt * sizeof(double)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to store partial level data.\n");
                return(MEMORY_ERROR);
            }
            datacnt = 0;
            linecnt = 0;
            fseek(fp,0,0);
            while(fgets(temp,8000,fp)!=NULL) {  //  store data and store counts of each spectra's partials
                p = temp;
                while(isspace(*p))
                    p++;
                if(*p == ';' || *p == ENDOFSTR)
                    continue;
                cnt = 0;
                dataindex[linecnt] = datacnt;   //  Store index in data-storage where this line's data begins
                thisdatacnt = 0;
                while(get_float_from_within_string(&p,&dummy)) {
                    if(cnt > 0) {
                        if(ODD(cnt))    // ODD entries, partial numbers
                            dz->parray[0][datacnt] = dummy;
                        else {          // EVEN values are levels
                            dz->parray[1][datacnt] = dummy;
                            datacnt++;
                            thisdatacnt++;                          //  Count the pno-level pairs
                        }
                    }
                    cnt++;
                }
                datasize[linecnt] = thisdatacnt; // Store size of this spectrum data in data-storage-array
                linecnt++;
            }
        }
        break;
    case(PULSER):
    case(PULSER2):
        linecnt = 0;
        while(fgets(temp,8000,fp)!=NULL) {
            p = temp;
            while(isspace(*p))
                p++;
            if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
                continue;
            p = temp;
            cnt = 0;
            while(get_float_from_within_string(&p,&dummy)) {
                if(cnt == 0) {
                    if(linecnt == 0) {
                        if(dummy != 0) {
                            sprintf(errstr,"First time in spatialisation data (%lf) must be zero.\n",dummy);
                            return(DATA_ERROR);
                        }
                    } else {
                        if(dummy <= lasttime) {
                            sprintf(errstr,"Times do not advance at line %d in spatialisation data.\n",linecnt+1);
                            return(DATA_ERROR);
                        }
                    }
                    lasttime = dummy;
                } else {
                    if((int)round(dummy) != dummy) {
                        sprintf(errstr,"Non-integer channel spec (%lf) in spatialisation data line %d.\n",dummy,linecnt+1);
                        return(DATA_ERROR);
                    }
                    if(dummy < 1 || dummy > 8) {
                        sprintf(errstr,"Invalid channel spec (%ld) in spatialisation data line %d.\n",round(dummy),linecnt+1);
                        return(DATA_ERROR);
                    }
                }
                cnt++;
            }
            if(cnt < 2) {
                sprintf(errstr,"No  spatialisation data on line %d.\n",linecnt + 1);
                return(DATA_ERROR);
            } 
            if(cnt > 9) {
                sprintf(errstr,"Too many spatialisation values on line %d.\n",linecnt + 1);
                return(DATA_ERROR);
            } 
            linecnt++;
        }
        if(linecnt == 0) {
            sprintf(errstr,"No data found in Chans-to-use data file.\n");
            return(DATA_ERROR);
        }
        if((dz->fptr = (float **)malloc(sizeof(float *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Chans-to-use time data.\n");
            return(MEMORY_ERROR);
        }
        if((dz->fptr[0] = (float *)malloc(linecnt * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Chans-to-use time data %d.\n",cnt+1);
            return(MEMORY_ERROR);
        }
        if((dz->iparray = (int **)malloc(2 * sizeof(int *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Chans-to-use data.\n");
            return(MEMORY_ERROR);
        }
        if((dz->iparray[0] = (int *)malloc(linecnt * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Chans-to-use data %d.\n",cnt+1);
            return(MEMORY_ERROR);
        }
        if((dz->iparray[1] = (int *)malloc(linecnt * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store Chans-to-use data %d.\n",cnt+1);
            return(MEMORY_ERROR);
        }
        fseek(fp,0,0);
        dz->itemcnt = linecnt;
        linecnt = 0;
        while(fgets(temp,8000,fp)!=NULL) {
            p = temp;
            while(isspace(*p))
                p++;
            if(*p == ';' || *p == ENDOFSTR) //  Allow comments in file
                continue;
            store_chans_to_use_data(temp,linecnt,dz);
            linecnt++;
        }
        break;
    }
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",str);
        fflush(stdout);
    }
    return(FINISHED);
}

/*************************************** SPLINT *******************************
 *
 * Do cubic spline, using 2nd derivatives table.
 */

double splint(int k,double time,dataptr dz)
{
    double *frqtab = dz->brk[PLS_PITCH];
    double *secondderiv = dz->parray[k];
    int arraysize = dz->brksize[PLS_PITCH];
    int hitim, lotim, hival, loval; //  time and value indeces into brkpoint array
    int hideriv, loderiv;           //  upper and lower indices into 2nd derivatives array
    double timestep, hitimefrac, lotimefrac, val1, val2, val3;
    if(time <= frqtab[0])
        return frqtab[1];
    hideriv = 0;
    hitim = 0;
    while(time > frqtab[hitim]) {
        hitim+=2;
        hideriv++;
        if(hideriv >= arraysize)
            return frqtab[(arraysize * 2)-1];   //  last value in table
    }
    loderiv = hideriv - 1;
    lotim = hitim - 2;
    loval = lotim+1;
    hival = hitim+1;
    timestep = frqtab[hitim] - frqtab[lotim];
    hitimefrac = (frqtab[hitim] - time)/timestep;
    lotimefrac = (time - frqtab[lotim])/timestep;
    
    val1  = hitimefrac * frqtab[loval];
    val1 += lotimefrac * frqtab[hival];

    val2  = (hitimefrac * hitimefrac * hitimefrac) - hitimefrac;
    val2 *= secondderiv[loderiv];

    val3  = (lotimefrac * lotimefrac * lotimefrac) - lotimefrac;
    val3 *= secondderiv[hideriv];

    val2 += val3;
    val2 *= (timestep * timestep)/6.0;
    val1 += val2;
    return val1;
}

/*************************************** SPLINE *******************************
 *
 * Establish 2nd dervatives table for cubic spline calculations.
 */

int spline(int secondderivtab,dataptr dz) 
{
    double firstderivatzero = 0.0, firstderivatend = 0.0, qn, un, val1, val2;   // firstderiveatend is a guess
    double *frqtab = dz->brk[PLS_PITCH];
    int arraysize = dz->brksize[PLS_PITCH];
    double *secondderiv = dz->parray[secondderivtab], *u;
    int n, t, v, tt, vv, k;
    double thistimestep, thisfrqstep, nexttimestep,nextfrqstep,bigtimestep, lasttime, lastval,sig,p;
    if((u = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for calculating spline derivatives.\n");
        return(MEMORY_ERROR);
    }
    secondderiv[0] = -0.5;
    thistimestep = frqtab[2] - frqtab[0];
    thisfrqstep  = frqtab[3] - frqtab[1];
    val1 = 3.0/thistimestep;
    val2 = (thisfrqstep/thistimestep) - firstderivatzero;
    u[0] = val1 * val2;
    lasttime = frqtab[0];
    lastval  = frqtab[1];
    for(n = 1,t = 2, v = 3, tt = 4, vv = 5; n < arraysize - 1;n++,t+=2,v+=2,tt+=2,vv +=2) { //  t indexes current times in brktable, v indexes current values
        thisfrqstep  = frqtab[v]  - lastval;                                                //  tt indexes NEXT time, vv indexes NEXT value
        nextfrqstep  = frqtab[vv] - frqtab[v];
        thistimestep = frqtab[t]  - lasttime;
        nexttimestep = frqtab[tt] - frqtab[t];
        bigtimestep  = frqtab[tt] - lasttime;
        sig = thistimestep/bigtimestep;
        p = (sig * secondderiv[n-1]) + 2.0;
        secondderiv[n] = (sig - 1.0)/p;
        u[n] = (nextfrqstep/nexttimestep) - (thisfrqstep/thistimestep);
        u[n] = (6.0 * u[n])/bigtimestep;
        u[n] -= sig * u[n-1];
        u[n] /= p;
        lasttime = frqtab[t];
        lastval =  frqtab[v];
    }
    qn = 0.5;
    thisfrqstep  = frqtab[v]  - lastval;
    thistimestep = frqtab[t]  - lasttime;
    val1 = 3.0/thistimestep;
    val2 = firstderivatend - (thisfrqstep/thistimestep);
    un = val1 * val2;
    val1 = un - (qn * u[n-1]);
    val2 = (qn * secondderiv[n-1]) + 1.0;
    secondderiv[n] = val1/val2;
    for(k = n-1;k >= 0;k--)
        secondderiv[k] = (secondderiv[k] * secondderiv[k+1]) + u[k];
    return FINISHED;
}

/************************************ PANCALC *******************************/

void pancalc(double position,double *leftgain,double *rightgain)
{
    int signal_to_left;
    double temp;
    double relpos;
    double reldist, invsquare;

    if(position < 0.0)
        signal_to_left = 1;     /* signal on left */
    else
        signal_to_left = 0;

    if(position < 0) 
        relpos = -position;
    else 
        relpos = position;
    if(relpos <= 1.0){      /* between the speakers */
        temp = 1.0 + (relpos * relpos);
        reldist = ROOT_2 / sqrt(temp);
        temp = (position + 1.0) / 2.0;
        *rightgain = temp * reldist;
        *leftgain = (1.0 - temp ) * reldist;
    } else {                /* outside the speakers */
        temp = (relpos * relpos) + 1.0;
        reldist  = sqrt(temp) / ROOT_2;   /* relative distance to source */
        invsquare = 1.0 / (reldist * reldist);
        if(signal_to_left){
            *leftgain = invsquare;
            *rightgain = 0.0;
        } else {   /* SIGNAL_TO_RIGHT */
            *rightgain = invsquare;
            *leftgain = 0;
        }
    }
}

/************************************ STORE_CHANS_TO_USE_DATA *******************************/

int store_chans_to_use_data(char *temp,int linecnt,dataptr dz)
{
    char *p = temp;
    double dummy;
    int test = 0, cnt = 0, val;
    while(get_float_from_within_string(&p,&dummy)) {
        if(cnt == 0)                    //  Stores time
            dz->fptr[0][linecnt] = (float)dummy;    
        else {
            val = (int)round(dummy);
            switch(val) {
            case(1):    test = test | 1;    break;
            case(2):    test = test | 2;    break;
            case(3):    test = test | 4;    break;
            case(4):    test = test | 8;    break;
            case(5):    test = test | 16;   break;
            case(6):    test = test | 32;   break;
            case(7):    test = test | 64;   break;
            case(8):    test = test | 128;  break;
            }
        }
        cnt++;
    }
    dz->iparray[0][linecnt] = test;     //  Stores flag indicating lspkrs to use
    dz->iparray[1][linecnt] = cnt-1;    //  Stores number of lspkrs to use
    return 1;
}

/************************************ GET_CHAN_TO_USE *******************************/

int get_chan_to_use(double time,dataptr dz)
{
    int n = 0, k, j;
    int this_chan = 0, usable_chans;
    int how_many_usable_chans, this_usable_chan_to_use, which_of_usable_chans = 0;
    while(n < dz->itemcnt) {            //  Find array at specified time
        if(time < dz->fptr[0][n])
            break;
        n++;
    }
    n--;
    how_many_usable_chans    = dz->iparray[1][n];
    this_usable_chan_to_use = (int)ceil(drand48() * how_many_usable_chans);
    usable_chans  = dz->iparray[0][n];
    k = 1;                          //  k = 1   2   4   8   16  32  64  128  = numeric val of bits to mark
    j = 1;                          //  j = 1   2   3   4   5   6   7   8    = bits to mark
    while(k < 129) {
        if(usable_chans & k) {
            this_chan = j;
            which_of_usable_chans++;
            if(which_of_usable_chans == this_usable_chan_to_use)
                break;
        }
        k *= 2;
        j++;
    }                               //  Chan info stored as 1-8
    return (this_chan - 1);         //  chans are assigend as offsets relative to a base
}                                   //  So chan 1 = base+0 : chan2 = base+1 : etc.

/************************************* CHANNELSTRING ************************
 *
 * We begin with iparray = NULL.
 * If a zero is found, stores nothing but returns 1 (so iparray remains NULL : -> output mono-or-stereo).
 * If valid filetype found, returns 0, causing fileread, and iparray creation  -> output 8-chan
 * If valid channel-string found, stores in iparray and returns 1.             -> output 8-chan 
 * IF an invalid filetype or an invalid string found, returns a DATA_ERROR..
 */

int get_channel_string(char *str,dataptr dz)
{
    int len, n, m, k = 0, test = 0, cnt = 0;
    char *p, *true_string_start, q, temp[2000],j[8];
    strcpy(temp,str);
    p = temp;
    p += strlen(temp);
    p -= 4;
    if(p > temp) {                      //  Look for a valid file extension
        if(!strcmp(p,".txt") || !strcmp(p,".brk"))
            return 0;                   //  Found a valid filetype
        else if(!strcmp(p,".wav") || !strcmp(p,".ana") || !strcmp(p,".frq") || !strcmp(p,".trn") || !strcmp(p,".for") \
             || !strcmp(p,".evl") || !strcmp(p,".txt") || !strcmp(p,".spe") || !strcmp(p,".spk") || !strcmp(p,".skk") \
             || !strcmp(p,".rmi") || !strcmp(p,".mid")) {
            sprintf(errstr,"INVALID FILETYPE FOR SPACEDATA\n");
            return DATA_ERROR;          //  Found an invalid filetype
        }
    }       
    p = temp;
    p++;                                //  Find 1st char in input string
    q = *p;
    *p = ENDOFSTR;
    p--;
    if(!strcmp(p,"@")) {                //  if sloom, strip sloom's number-marker "@"
        str++;
        k = 1;                          //  mark that string is shortened
    }
    if(!strcmp(str,"0"))                //  str is "0" : INDICATES A MONO OUTPUT, return
        return 1;
    len = strlen(str);
    if(len > 8) {
        sprintf(errstr,"SPACEDATA : INVALID FILENAME : INVALID STRING - TOO int FOR CHANNEL-NUMBER STRING\n");
        return DATA_ERROR;
    }
    p++;
    *p = q;                             //  Replace the previously overwritten character in copy of input string    
    if(k==0)                            //  If no "@" character has been removed, go back to start
        p--;
    true_string_start = p;
    n = 0;
    k = 1 + INT_TO_ASCII;
    m = 8 + INT_TO_ASCII;
    while(*p != ENDOFSTR) {
        if(*p < k || *p > m) {          //  Check for only values 1-8: store valid vals found
            sprintf(errstr,"SPACEDATA : INVALID FILENAME : INVALID STRING (%s) - CONTAINS CHARACTER (%c) OTHER THAN 1-8\n",true_string_start,*p);
            return DATA_ERROR;
        }
        j[n] = *p;
        p++;
        n++;
    }
    if(n > 1) {                         //  Look for duplicates amongst values found        
        for(m = 0;m < n-1;m++) {
            for(k = m+1;k < n;k++) {
                if(j[m] == j[k]) {
                    sprintf(errstr,"SPACEDATA : INVALID STRING (%s) - REPEATED NUMBER (%c)\n",true_string_start,j[m]);
                    return DATA_ERROR;
                }
            }
        }
    }
    if((dz->fptr = (float **)malloc(sizeof(float *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store Chans-to-use time data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->fptr[0] = (float *)malloc(sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store Chans-to-use time data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray = (int **)malloc(2 * sizeof(int *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store Chans-to-use data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[0] = (int *)malloc(sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store Chans-to-use data.\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparray[1] = (int *)malloc(sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store Chans-to-use data.\n");
        return(MEMORY_ERROR);
    }
    p = true_string_start;
    while(cnt < len) {                  //  Store channels-used-info as bitflag
        k = *p - INT_TO_ASCII;
        switch(k) {
        case(1):    test = test | 1;    break;
        case(2):    test = test | 2;    break;
        case(3):    test = test | 4;    break;
        case(4):    test = test | 8;    break;
        case(5):    test = test | 16;   break;
        case(6):    test = test | 32;   break;
        case(7):    test = test | 64;   break;
        case(8):    test = test | 128;  break;
        }
        cnt++;
        p++;
    }
    dz->iparray[0][0] = test;
    dz->iparray[1][0] = len;
    dz->fptr[0][0] = 0.0;
    dz->itemcnt = 1;
    return 1;
}
