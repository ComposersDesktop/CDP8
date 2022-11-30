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

/* GO TO HEREH
 *
 * The write procedure not quite correct when there is offset.
 * Need to write to obuf UP to the "maxoffset"
 * BUT advance the obufotr only up to end of "towrite" WITHOUT the offset.
 * and ONLY WRITE TO FILE when obfptr is AT or BEYOND buflen AT ~START~ OF THE WRITE
 * (not simply when the write itself will overflow the buflen)
 *
 * Probably also need to extend obuf by another buflen.
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

#ifdef unix
#define round(x) lround((x))
#endif

char errstr[2400];

#define ibuflen itemcnt

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "6.1.0";

#define MAXOUTLEVEL (0.95)
#define WRAP (16)                   //  wraparound extension of input buffer to allow for interpolated reading of last sample
#define SAFETY (64)                 //  Avoid overwriting end of arrays

#define SIGNAL_TO_LEFT  (0)
#define SIGNAL_TO_RIGHT (1)
#define ROOT2       (1.4142136)

#define maxoffset   total_windows   //  max time-offset of streams
#define outchinfo   is_mapping      //  lowest array number holding output-channel-info for each stream

//CDP LIB REPLACEMENTS
static int check_phasor_param_validity_and_consistency(double *maxoffset,dataptr dz);
static int setup_phasor_application(dataptr dz);
static int parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
static int parse_infile_and_check_type(char **cmdline,dataptr dz);
static int setup_phasor_param_ranges_and_defaults(dataptr dz);
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
//static int get_the_mode_from_cmdline(char *str,dataptr dz);
static int setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int create_phasor_sndbufs(int maxshiftslen,double maxoffset,int *offsetwrap,dataptr dz);
static int create_phaseshift_and_outchan_data_arrays(int *maxshiftslen, dataptr dz);
static int calculate_phase_shifts(double time, int *phaseshiftcnt, dataptr dz);
static int calculate_streams(double time,int phaseshiftcnt, int *obufpos, int *ibufpos, int passno, double normaliser, double *maxsamp,int offsetwrap,int * absolute_endofwrite,dataptr dz);
static void pancalc(double position,double *leftgain,double *rightgain);
static int phasor(int offsetwrap,dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
    int n, maxshiftslen = 0, offsetwrap = 0;
    double maxoffset = 0.0;
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
        if((exit_status = setup_phasor_application(dz))<0) {
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

    // parse_infile_and_hone_type() = 
    if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_phasor_param_ranges_and_defaults(dz))<0) {
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
    dz->duration = (double)dz->insams[0]/(double)dz->infile->srate;

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
    if((exit_status = check_phasor_param_validity_and_consistency(&maxoffset,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = open_the_outfile(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    if((exit_status = create_phaseshift_and_outchan_data_arrays(&maxshiftslen,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;
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

    if((exit_status = create_phasor_sndbufs(maxshiftslen,maxoffset,&offsetwrap,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //param_preprocess()                        redundant
    //spec_process_file =
    if((exit_status = phasor(offsetwrap,dz))<0) {
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
    int exit_status, origchans;
    origchans = dz->infile->channels;
    dz->infile->channels = dz->iparam[PHASOR_OCHANS];
    if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
        return(exit_status);
    dz->infile->channels = origchans;
    dz->outfile->channels = dz->iparam[PHASOR_OCHANS];
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

/************************* SETUP_PHASOR_APPLICATION *******************/

int setup_phasor_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    if((exit_status = set_param_data(ap,0   ,4,4,"iDDi"))<0)
        return(FAILED);
    if((exit_status = set_vflgs(ap,"o",1,"d","se",2,0,"00"))<0)
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

/************************* SETUP_GATE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_phasor_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    
    ap->lo[PHASOR_STREAMS]  = 2;
    ap->hi[PHASOR_STREAMS]  = 8;
    ap->default_val[PHASOR_STREAMS] = 2;
    
    ap->lo[PHASOR_FRQ]  = .01;
    ap->hi[PHASOR_FRQ]  = 100;
    ap->default_val[PHASOR_FRQ] = 1;
    
    ap->lo[PHASOR_SHIFT]    = 0;
    ap->hi[PHASOR_SHIFT]    = 12;
    ap->default_val[PHASOR_SHIFT] = .5;
    
    ap->lo[PHASOR_OCHANS]   = 1;
    ap->hi[PHASOR_OCHANS]   = 8;
    ap->default_val[PHASOR_OCHANS] = 1;
    
    ap->lo[PHASOR_OFFSET]   = 0;
    ap->hi[PHASOR_OFFSET]   = 500;
    ap->default_val[PHASOR_OFFSET] = 0;

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
            if((exit_status = setup_phasor_application(dz))<0)
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
    usage2("phasor");
    return(USAGE_ONLY);
}

/**************************** CHECK_GATE_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_phasor_param_validity_and_consistency(double *maxoffset,dataptr dz)
{
    int exit_status;
    if(dz->iparam[PHASOR_OCHANS] > dz->iparam[PHASOR_STREAMS]) {
        sprintf(errstr,"Number of output channels exceeds number of streams.\n");
        return(DATA_ERROR);
    }
    if(dz->brksize[PHASOR_OFFSET]) {
        if((exit_status = get_maxvalue_in_brktable(maxoffset,PHASOR_OFFSET,dz))<0)
            return exit_status;
    } else
        *maxoffset = dz->param[PHASOR_OFFSET];
    if(dz->vflag[0] && (dz->iparam[PHASOR_OCHANS] <= 2)) {
        sprintf(errstr,"NO SOUND-SURROUND POSSIBLE WITH %d OUTPUT CHANNELS\n",dz->iparam[PHASOR_OCHANS]); 
        return DATA_ERROR;
    }
    return FINISHED;
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if(!strcmp(prog_identifier_from_cmdline,"phasor"))              dz->process = PHASOR;
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
    if(!strcmp(str,"phasor")) {
        fprintf(stderr,
        "USAGE:\n"
        "phasor phasor infile outfile streams phasfrq shift ochans [-ooffset] [-s] [-e]\n"
        "\n"
        "Introduce phasing into signal : Takes a MONO input file.\n"
        "\n"
        "STREAMS  Number of output streams that phase-interact (Range 2 - 8)\n"
        "PHASFRQ  Frequency of packets (phase shifts forward then back in a single packet).\n"
        "SHIFT    Maximum phaseshift with packet: (Range 0 - 12 semitones).\n"
        "OCHANS   Number of output channels (not greater than number of streams).\n"
        "OFFSET   The streams may be time-offset from one another..\n"
        "        \"Offset\" is time-offset of the most-offset stream (Range 0 - 500 mS).\n"
        "         The other streams are offset by intermediate amounts.\n"
        "\n"
        "-s       Output chans (lspkrs) encircle audience : only with more than 2 ochans.\n"       
        "-e       Prints warnings re rounding-errors in calculating time-shifted streams.\n"       
        "\n"
        "\"shift\" and \"phasfrq\" can vary through time,\n"
        "but extreme phasfrq changes or reversals will produce anomalous output.\n"
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

/******************************** CREATE_PHASESHIFT_AND_OUTCHAN_DATA_ARRAYS ********************************/

int create_phaseshift_and_outchan_data_arrays(int *maxshiftslen, dataptr dz)
{
    int exit_status, n, m, chan;
    double time = 0, srate = (double)dz->infile->srate, leftgain = 0.0, rightgain = 0.0, chanpos, pos;
    *maxshiftslen = 0;
    while(time < dz->duration) {
        if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
            return exit_status;
        *maxshiftslen = max(*maxshiftslen,(int)ceil(srate/dz->param[PHASOR_FRQ]));
        time += 1.0/dz->param[PHASOR_FRQ];
    }
    while(((*maxshiftslen)/4) * 4 != *maxshiftslen)
        (*maxshiftslen)++;                          //  phaseshift arrays must have a multiple-of-4 entries
    *maxshiftslen += WRAP;                          //  Allow for wraparound point and accumulation (of samp-pointer position) errors
    dz->outchinfo = dz->iparam[PHASOR_STREAMS] - 1;
    if((dz->parray = (double **)malloc((dz->outchinfo + dz->iparam[PHASOR_STREAMS]) * sizeof(double *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to store phaseshift data arrays.\n");
        return(MEMORY_ERROR);
    }
    for(n = 0; n < dz->outchinfo; n++) {
        if((dz->parray[n] = (double *)malloc(((*maxshiftslen) + SAFETY) * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store phaseshift data, stream %d.\n",n+1);
            return(MEMORY_ERROR);
        }
    }
    for(n = dz->outchinfo; n < dz->outchinfo + dz->iparam[PHASOR_STREAMS]; n++) {
        if((dz->parray[n] = (double *)malloc(4 * sizeof(double)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY to store phaseshift data, stream %d.\n",n+1);
            return(MEMORY_ERROR);
        }
    }
    if(dz->iparam[PHASOR_STREAMS] == dz->iparam[PHASOR_OCHANS]) {
        for(n=0,m = dz->outchinfo;n < dz->iparam[PHASOR_OCHANS]; n++,m++) {
            dz->parray[m][0] = (double)n;
            dz->parray[m][1] = 1.0;
            dz->parray[m][2] = (double)((n+1)%dz->iparam[PHASOR_OCHANS]);
            dz->parray[m][3] = 0.0;
        }
    } else {
        if(dz->vflag[0]) {      /* Sound surround, 5 streams on 4 chans are positioned at 0  4/5  1+3/5  2+2/5  3+1/5 */
            for(n=0,m = dz->outchinfo;n < dz->iparam[PHASOR_STREAMS]; n++,m++) {
                chanpos = ((double)n/(double)dz->iparam[PHASOR_STREAMS]) * (double)dz->iparam[PHASOR_OCHANS];   //  e.g. 3.7
                chan = (int)floor(chanpos);                                                                     //  3
                pos = chanpos - (double)chan;                                                                   //  0.7 in range 0 to 1
                pos = (2.0 * pos) - 1.0;                                                                        //  0.4 in range -1 to 0
                pancalc(pos,&leftgain,&rightgain);
                dz->parray[m][0] = (double)chan;
                dz->parray[m][1] = leftgain;
                dz->parray[m][2] = (double)((chan+1)%dz->iparam[PHASOR_OCHANS]);
                dz->parray[m][3] = rightgain;
            }               
        } else {                /* Linear array, 5 streams on 4 chans are positioned at   0  3/4  1+1/2  2+3/4   3   */
            for(n=0,m = dz->outchinfo;n < dz->iparam[PHASOR_STREAMS]; n++,m++) {
                chanpos = (double)n/(double)(dz->iparam[PHASOR_STREAMS] - 1) * (double)(dz->iparam[PHASOR_OCHANS] - 1);
                chan = (int)floor(chanpos);
                pos = chanpos - (double)chan;
                pos = (2.0 * pos) - 1.0;
                pancalc(pos,&leftgain,&rightgain);
                dz->parray[m][0] = (double)chan;
                dz->parray[m][1] = leftgain;
                dz->parray[m][2] = (double)((chan+1)%dz->iparam[PHASOR_OCHANS]);
                dz->parray[m][3] = rightgain;
            }               
        }
    }
    return FINISHED;
}

/******************************** CREATE_PHASOR_SNDBUFS ********************************/

int create_phasor_sndbufs(int maxshiftslen,double maxoffset,int *offsetwrap,dataptr dz)
{
    int bigbufsize, seccnt;
    int framesize = F_SECSIZE;
    double srate = (double)dz->infile->srate;
    if(maxoffset > 0.0) {                                       //  Additional buffer space, if streams are offset from one another
        dz->maxoffset = (int)ceil(maxoffset * MS_TO_SECS * srate);
        *offsetwrap = dz->maxoffset * dz->iparam[PHASOR_OCHANS];
        *offsetwrap += WRAP;
        seccnt = (*offsetwrap)/framesize;
        if(seccnt * framesize < *offsetwrap)
            seccnt++;
        *offsetwrap = seccnt * framesize;
    }
    seccnt = maxshiftslen/framesize;
    if(seccnt * framesize < maxshiftslen)
        seccnt++;
    bigbufsize = seccnt * framesize;
    dz->buflen = bigbufsize;
    dz->ibuflen = dz->buflen;   //  Input buffer size
    bigbufsize *= (1 + (3 * dz->iparam[PHASOR_OCHANS]));        //  1 mono and 1 multichan (pbuf), + 1 dopuble-size outbuf
    if(maxoffset > 0.0)
        bigbufsize += 2 * *offsetwrap;                          //  once for pbuf, and once for overflowbuf
    if((dz->bigbuf = (float *)malloc(bigbufsize * sizeof(float))) == NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create sound buffers.\n");
        return(PROGRAM_ERROR);
    }
    dz->sbufptr[0] = dz->sampbuf[0] = dz->bigbuf;
    dz->sbufptr[1] = dz->sampbuf[1] = dz->bigbuf + dz->buflen;                              //  Input buffer (mono)
    dz->buflen *= dz->iparam[PHASOR_OCHANS];                                                //  Buflen used in calculation is size of multichan buffers
    dz->sbufptr[2] = dz->sampbuf[2] = dz->sampbuf[1] + dz->buflen + *offsetwrap;            //  Packet buffer (multichan) + its possible overflow + offset-extension
    dz->sampbuf[3]                  = dz->sampbuf[2] + dz->buflen + dz->buflen + *offsetwrap;// Output buffer (multichan) + overflow buffer + offset-extension to overflow
    return FINISHED;
}

/******************************** PHASOR ********************************/

int phasor(int offsetwrap,dataptr dz)
{
    int exit_status, passno;
    double time, normaliser = 1.0, maxsamp = 0.0;
    float *ibuf = dz->sampbuf[0], *obuf = dz->sampbuf[2];
    int phaseshiftcnt, ibufpos, obufpos, n, origbuflen = 0, absolute_endofwrite = 0;
    for(passno = 0; passno < 2; passno++) {
        dz->total_samps_read = 0;
        dz->samps_left = dz->insams[0];
        time = 0.0;
        ibufpos = 0;
        obufpos = 0;
        memset((char *)ibuf,0,dz->ibuflen * sizeof(float));
        memset((char *)obuf,0,((dz->buflen * 2) + offsetwrap) * sizeof(float));
        sndseekEx(dz->ifd[0],0,0);
        origbuflen = dz->buflen;
        dz->buflen = dz->ibuflen;
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        dz->buflen = origbuflen;
        while(time < dz->duration) {
            if((exit_status = calculate_phase_shifts(time,&phaseshiftcnt,dz))<0)
                return exit_status;
            if((exit_status = calculate_streams(time,phaseshiftcnt,&obufpos,&ibufpos,passno,normaliser,&maxsamp,offsetwrap,&absolute_endofwrite,dz))<0)
                return exit_status;
            if(exit_status == FINISHED)
                break;
            time += 1.0/dz->param[PHASOR_FRQ];
        }
        if(passno == 0) {
            if(absolute_endofwrite > 0) {
                for(n=0;n<absolute_endofwrite;n++)
                    maxsamp = max(maxsamp,fabs(obuf[n]));
            }
            if(maxsamp > MAXOUTLEVEL)
                normaliser = MAXOUTLEVEL/maxsamp;
        } else {
            if(absolute_endofwrite > 0) {
                for(n=0;n<absolute_endofwrite;n++)
                    obuf[n] = (float)(obuf[n] * normaliser);
                if((exit_status = write_samps(obuf,absolute_endofwrite,dz))<0)
                    return(exit_status);
            }
        }
    }
    return FINISHED;
}

/******************************** CALCULATE_PHASE_SHIFTS ********************************/

int calculate_phase_shifts(double time, int *phaseshiftcnt, dataptr dz)
{
    int exit_status;
    double srate = (double)dz->infile->srate, maxphaseshift, minphaseshift, thismax, posoffset, sum, error;
    double *sampincr;
    int midway, quartway, threequartway, n, k, j;
    if((exit_status = read_values_from_all_existing_brktables(time,dz))<0)
        return exit_status;
    maxphaseshift = dz->param[PHASOR_SHIFT];
    minphaseshift = maxphaseshift/(dz->iparam[PHASOR_STREAMS] - 1);
    *phaseshiftcnt = (int)floor(srate/dz->param[PHASOR_FRQ]);
    while((*phaseshiftcnt/4) * 4 != *phaseshiftcnt)
        (*phaseshiftcnt)++;                             //  phaseshift arrays must have a multiple-of-4 entries
    midway = (*phaseshiftcnt)/2;                        //  midpoint of arrays
    quartway = midway/2;
    threequartway = midway + quartway;
    for(n = 0; n < dz->iparam[PHASOR_STREAMS]-1;n++) {  //  Set max-shift for this stream
        sum = 0.0;
        thismax = min(maxphaseshift,(n+1) * minphaseshift);
        thismax = thismax/SEMITONES_PER_OCTAVE;         //  Convert to octaves
        posoffset = thismax/(double)quartway;           //  semitone-displacement per sample.
                                                            /*  If 8 samps to midway, midway = 8, quartway = 4 : midway-2 = 6   */
        sampincr = dz->parray[n];                           /*                                                                  */
        for(k = 0; k < quartway; k++)                       /*  Rising         A/\B             e.g. k=0  1   2   3             */
            sampincr[k] = min((k+1) * posoffset,thismax);   /*  portion A     /    \                 +1  +2  +3  +4             */
                                                            /*                       \C  D/          j=6  5   4                 */
        for(k = 0,j = midway - 2; k < quartway-1; k++,j--)  /*  Falling                \/            j=7 gets 0                 */
            sampincr[j] = sampincr[k];                      /*  portion B                                                       */

        sampincr[midway-1] = 0.0;

        for(k = 0,j = midway; k < midway; k++,j++)          /*  Falling & Rising portions C & D         */
            sampincr[j] = -sampincr[k];                     /*  Every +ve sampincr in A+B               */
                                                            /*  compensated for by equal -ve sampincr   */ 
                                                            /*  in C+D, so signals should resync!       */
        for(k=0;k < *phaseshiftcnt;k++)
            sampincr[k] = pow(2.0,sampincr[k]);             /*  semitones-->frqratio = sampincr         */
        sum = 0.0;
        for(k=0;k < *phaseshiftcnt;k++)                     /*  check error in summing all increments   */
            sum += sampincr[k];
        error = (sum - (double)*phaseshiftcnt)/(double)(*phaseshiftcnt);
        for(k=0;k < *phaseshiftcnt;k++)                     /*  adjust increments to eliminate error    */
            sampincr[k] -= error;
    }
    return FINISHED;
}

/******************************** CALCULATE_STREAMS ********************************/

int calculate_streams(double time,int phaseshiftcnt, int *obufpos, int *ibufpos, int passno, double normaliser, double *maxsamp, int offsetwrap, int *absolute_endofwrite, dataptr dz)
{
    int exit_status;
    float *ibuf = dz->sampbuf[0];
    float *pbuf = dz->sampbuf[1];
    float *obuf = dz->sampbuf[2];
    double ibufposd, srate = (double)dz->infile->srate, ch1lev = 0.0, ch2lev = 0.0;
    double loval, valdiff, timefrac, val, val1, val2;
    double *outchan_info, *sampincr;
    int n, origbuflen, offset[8], bufadjust, thisoffset, k, ibufpos_for_each_stream = 0, lopos, hipos, ovflow;
    int pbufpos = 0, advance_to_next_write = 0, pbufpos1 = 0, pbufpos2 = 0, endibufpos = 0, xmaxoffset, position_of_next_write, samps_to_write, xobufpos;
    int warning = 0, streamcnt = dz->iparam[PHASOR_STREAMS], ochan1, ochan2;
    memset((char *)pbuf,0, (dz->buflen + offsetwrap) * sizeof(float));
    if(*ibufpos + phaseshiftcnt + 1 >= dz->ibuflen) {       //  Ensure all of read will go into the input buffer
        bufadjust = dz->ssampsread - *ibufpos;
        sndseekEx(dz->ifd[0],dz->total_samps_read - bufadjust,0);
        dz->total_samps_read -= bufadjust;
        memset((char *)ibuf,0,dz->ibuflen * sizeof(float));
        origbuflen = dz->buflen;
        dz->buflen = dz->ibuflen;                           //  input buffer size
        if((exit_status = read_samps(ibuf,dz))<0)
            return(exit_status);
        dz->buflen = origbuflen;                            //  Restore dz->buflen for calcs in output buffer       
        if(dz->ssampsread <= 0)
            return FINISHED;
        *ibufpos = 0;
    }

    /* CALCULATE ANY STREAM TIME-OFFSETS */

    thisoffset = (int)round(dz->param[PHASOR_OFFSET] * MS_TO_SECS * srate);             //  With 4 streams, offsets go ...
    offset[0] = 0;                                                                          //
    xmaxoffset = 0;                                                                         //      |----------------------------
    for(k = 1; k < streamcnt-1;k++) {                                                       //      |   |----------------------------
        offset[k] = (int)round(((double)k/(double)(streamcnt - 1)) * (double)thisoffset);   //      |   |   |----------------------------
        xmaxoffset = max(xmaxoffset,offset[k]);                                             //      |   |   |   |----------------------------
    }                                                                                       //      0  1/3 2/3  thisoffset
    offset[k] = thisoffset;
    xmaxoffset = max(xmaxoffset,offset[k]);
    for(k = 0; k < streamcnt; k++) {
        if(k==0)
            ibufpos_for_each_stream = *ibufpos;             //  Current read-position in input
        else if(k==1) {
            endibufpos = ibufpos_for_each_stream;           //  ibufpos_for_each_stream has advanced from initial ibufpos , while writing stream 0 : 
                                                            //  and now tells us where we've reached in the input buffer.
            advance_to_next_write = pbufpos;                //  pbufpos has advanced from zero, while writing stream 0 : 
                                                            //  It now tells us how many samples to advance, in output-buf, for next set-of-streams to be written to output
        }
        pbufpos = offset[k] * dz->iparam[PHASOR_OCHANS];    //  Initial time-offset of this stream (as a whole number of channel-groups), default is zero 
        ibufposd = (double)(*ibufpos);                      //  Ditto as a floatval which can be fractionally incremented

        outchan_info = dz->parray[dz->outchinfo + k];       //  Output channel information for this stream

        ochan1 = (int)round(outchan_info[0]);                   
        ch1lev = outchan_info[1];
        ochan2 = (int)round(outchan_info[2]);
        ch2lev = outchan_info[3];
        pbufpos1 = pbufpos + ochan1;                        //  Actual start sample for this particular stream feeding particular output channel
        pbufpos2 = pbufpos + ochan2;                        //  Actual start sample for this particular stream feeding adjacent output channel

        for(n = 0; n < phaseshiftcnt; n++) {
            if(k==0) {
                pbuf[pbufpos] = (float)(pbuf[pbufpos] + ibuf[ibufpos_for_each_stream]); //  Stream 0 always goes entirely to channel 0
                pbufpos += dz->iparam[PHASOR_OCHANS];                                   //  Step to next output-channel-group
                ibufpos_for_each_stream++;                                              //  Advance at normal rate in input
            } else {
                sampincr = dz->parray[k-1];
                ibufposd += sampincr[n];                                                //  Advance in input by timewarp value stored in array
                lopos = (int)round(ibufposd);
                if(lopos >= phaseshiftcnt) {
                    warning++;
                    lopos = phaseshiftcnt;
                }
                hipos = lopos + 1;
                loval = ibuf[lopos];
                valdiff = ibuf[hipos] - loval;
                timefrac = ibufposd - (double)lopos;
                val = loval + (valdiff * timefrac);             //  Value obtained by interpolating in input sound
                val1 = val * ch1lev;                            //  Distribute this between the 2 appropriate output channels
                pbuf[pbufpos1] = (float)(pbuf[pbufpos1] + val1);
                val2 = val * ch2lev;
                pbuf[pbufpos2] = (float)(pbuf[pbufpos2] + val2);
                pbufpos1 += dz->iparam[PHASOR_OCHANS];          //  Step to appropriate-chan-positions in next output-channel-group
                pbufpos2 += dz->iparam[PHASOR_OCHANS];
            }
        }
    }
    *ibufpos = endibufpos;                                      //  Reset final ibufpos - at end of loop through input samps.

    position_of_next_write = *obufpos + advance_to_next_write;  //  Calculate position of NEXT write, from outbuf
                                                                //  Calculate absolute end of current write, in outbuf
    *absolute_endofwrite = position_of_next_write + (dz->iparam[PHASOR_OCHANS] * xmaxoffset);
    ovflow = *absolute_endofwrite - dz->buflen;
    if(ovflow >= dz->buflen + offsetwrap) {                     //  check that any overflow of principal output buffer does not exceed length of overflow-buffer
        sprintf(errstr,"Buffer size underestimated,\n");
        return PROGRAM_ERROR;
    }
                                                    //  Write samples into the output buffer (even if they  overflow), as far as absolute end of write
    samps_to_write = *absolute_endofwrite - *obufpos;
    xobufpos = *obufpos;
    for(n = 0; n < samps_to_write; n++) {
        obuf[xobufpos] = (float)(obuf[xobufpos] + pbuf[n]);
        xobufpos++;
    }
    *obufpos = position_of_next_write;              //  Reset output buffer write position, ready for next pass

    if(position_of_next_write >= dz->buflen) {      //  If no further samples will be added to the current buffer (because next write is beyond its end)
        if(passno == 0) {                           //  We can process-or-write the complete current buffer
            for(n=0;n<dz->buflen;n++)
                *maxsamp = max(*maxsamp,fabs(obuf[n]));
        } else {
            for(n=0;n<dz->buflen;n++)
                obuf[n] = (float)(obuf[n] * normaliser);
            if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                return(exit_status);
        }
        memset((char *)obuf,0,dz->buflen * sizeof(float));
        memcpy((char *)obuf,(char *)(obuf + dz->buflen), dz->buflen * sizeof(float));   //  Wrap-around any overflow in output buffer.
        memset((char *)(obuf + dz->buflen),0,dz->buflen * sizeof(float));               //  re-zero the overflow buffer.
        if(offsetwrap > 0) {                                                            //  Wrap around any data in the offsetwrap extension of the overflow buffer     
            memcpy((char *)(obuf + dz->buflen),(char *)(obuf + (dz->buflen * 2)), offsetwrap * sizeof(float));
            memset((char *)(obuf + (dz->buflen * 2)),0,offsetwrap * sizeof(float));     //  re-zero the offsetwrap buffer.
        }
        *obufpos -= dz->buflen;                                                         //  Reset outbuf pointer.
        *absolute_endofwrite -= dz->buflen; 
    }
    if(dz->vflag[1] && warning) {
        fprintf(stdout,"WARNING: %d rounding errors at time %lf\n",warning,time);
        fflush(stdout);
    }
    return CONTINUE;
}   

/************************************ PANCALC *******************************/

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
        if(dirflag == SIGNAL_TO_LEFT){
            *leftgain = invsquare;
            *rightgain = 0.0;
        } else {   /* SIGNAL_TO_RIGHT */
            *rightgain = invsquare;
            *leftgain = 0;
        }
    }
}

