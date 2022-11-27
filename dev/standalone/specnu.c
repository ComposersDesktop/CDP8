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
#include <speccon.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>
/* RWD 1/04/20 this is correct file with processes rand and squeeze */

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif
#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "7.1.1";

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

/* CDP LIB FUNCTION MODIFIED TO AVOID CALLING setup_particular_application() */

static int  parse_sloom_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);

/* SIMPLIFICATION OF LIB FUNC TO APPLY TO JUST THIS FUNCTION */

static int  parse_infile_and_check_type(char **cmdline,dataptr dz);
static int  handle_the_extra_infile(char ***cmdline,int *cmdlinecnt,dataptr dz);
static int  handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
static int  setup_the_application(dataptr dz);
static int  setup_the_param_ranges_and_defaults(dataptr dz);
static int  check_the_param_validity_and_consistency(dataptr dz);
static int  get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz);
static int  setup_and_init_input_brktable_constants(dataptr dz,int brkcnt);
static int  get_the_mode_no(char *str, dataptr dz);

/* BYPASS LIBRARY GLOBAL FUNCTION TO GO DIRECTLY TO SPECIFIC APPLIC FUNCTIONS */

static int spec_remove(dataptr dz);
static int do_speclean(int subtract,dataptr dz);
static int allocate_speclean_buffer(dataptr dz);
static int handle_pitchdata(int *cmdlinecnt,char ***cmdline,dataptr dz);
static void insert_newnumber_at_filename_end(char *filename,int num,int overwrite_last_char);
static int create_next_outfile(dataptr dz);
static int do_specslice(dataptr dz);
static int setup_specslice_parameter_storage(dataptr dz);
static int specpivot(dataptr dz);
static int specrand(dataptr dz);
static int specsqz(dataptr dz);
static void rndintperm(int *perm,int cnt);
static int allocate_specsqz_buffer(dataptr dz);
#ifdef NOTDEF
/* SPECTOVF FUNCTIONS */

static int  spectovf(dataptr dz);
static int  specget(double time,double maxamp,dataptr dz);
static int  close_to_frq_already_in_ring(chvptr *there,double frq1,dataptr dz);
static int  substitute_in_ring(int vc,chvptr here,chvptr there,dataptr dz);
static int  insert_in_ring(int vc, chvptr here, dataptr dz);
static int  put_ring_frqs_in_ascending_order(chvptr **partials,float *minamp,dataptr dz);
static void find_pitch(chvptr *partials,double lo_loud_partial,double hi_loud_partial,float minamp,double time,float amp,dataptr dz);
static int  equivalent_pitches(double frq1, double frq2, dataptr dz);
static int  is_peak_at(double frq,long window_offset,float minamp,dataptr dz);
static int  enough_partials_are_harmonics(chvptr *partials,double pich_pich,dataptr dz);
static int  is_a_harmonic(double frq1,double frq2,dataptr dz);
static int  local_peak(int thiscc,double frq, float *thisbuf, dataptr dz);

/* SPECTOVF2 FUNCTIONS */

static int spectovf2(dataptr dz);
static int locate_peaks(int firstpass,double time,dataptr dz);
static int keep_peak(int firstpass,int vc,double time,dataptr dz);
static int remove_non_persisting_peaks(int firstpass,dataptr dz);
static int store_peaks(long *outcnt,double time,dataptr dz);
static void sort_peaks(long outcnt,dataptr dz);
static int write_peaks(long outcnt,dataptr dz);


#define peaktrail_cnt formant_bands
#endif
/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    int exit_status;
    dataptr dz = NULL;
    char **cmdline;
    int  cmdlinecnt;
    //aplptr ap;
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
        case(SPEC_REMOVE):  dz->maxmode = 2; break;
        case(SPECLEAN):     dz->maxmode = 0; break;
        case(SPECTRACT):    dz->maxmode = 0; break;
        case(SPECSLICE):    dz->maxmode = 5; break;
        case(SPECRAND):     dz->maxmode = 0; break;
        case(SPECSQZ):      dz->maxmode = 0; break;
        }
        if(dz->maxmode > 0) {
            if(cmdlinecnt <= 0) {
                sprintf(errstr,"Too few commandline parameters.\n");
                return(FAILED);
            }
            if((get_the_mode_no(cmdline[0],dz))<0) {
                if(!sloom)
                    fprintf(stderr,"%s",errstr);
                return(FAILED);
            }
            cmdline++;
            cmdlinecnt--;
        }
        if((exit_status = setup_the_application(dz))<0) {
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
    //ap = dz->application;
    // parse_infile_and_hone_type() = 
    if((exit_status = parse_infile_and_check_type(cmdline,dz))<0) {
        exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    // setup_param_ranges_and_defaults() =
    if((exit_status = setup_the_param_ranges_and_defaults(dz))<0) {
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

//  handle_extra_infiles() =
    if(dz->process == SPECLEAN || dz->process == SPECTRACT) {
        if((exit_status = handle_the_extra_infile(&cmdline,&cmdlinecnt,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }

//  handle_formants()           redundant
//  handle_formant_quiksearch() redundant
//  handle_special_data()       redundant except
    if(dz->process == SPECSLICE && dz->mode == 3) {
        if((exit_status = handle_pitchdata(&cmdlinecnt,&cmdline,dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    } 
    if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {      // CDP LIB
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    //check_param_validity_and_consistency .....
    if((exit_status = check_the_param_validity_and_consistency(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    is_launched = TRUE;

    //allocate_large_buffers() ... replaced by  CDP LIB
    switch(dz->process) {
    case(SPEC_REMOVE):  dz->extra_bufcnt =  0;  dz->bptrcnt = 1;                break;
    case(SPECTRACT):
    case(SPECLEAN):     dz->extra_bufcnt =  0;  dz->bptrcnt = dz->iparam[0];    break;
    case(SPECSLICE):    dz->extra_bufcnt =  0;  dz->bptrcnt = 0;                break;
    case(SPECRAND):     dz->extra_bufcnt =  0;  dz->bptrcnt = 0;                break;
    case(SPECSQZ):      dz->extra_bufcnt =  0;  dz->bptrcnt = 4;                break;
//  case(SPECTOVF2):
//  case(SPECTOVF):     dz->extra_bufcnt =  3;  dz->bptrcnt = 2;                break;

    }
    if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    switch(dz->process) {
//  case(SPECTOVF):
//  case(SPECTOVF2):
    case(SPEC_REMOVE):  exit_status = allocate_single_buffer(dz);   break;
    case(SPECRAND):
    case(SPECSLICE):
    case(SPECTRACT):
    case(SPECLEAN):     exit_status = allocate_speclean_buffer(dz); break;
    case(SPECSQZ):      exit_status = allocate_specsqz_buffer(dz);  break;
    }
    if(exit_status < 0) {
        print_messages_and_close_sndfiles(exit_status,is_launched,dz);
        return(FAILED);
    }
    
    //param_preprocess()                        redundant
    //spec_process_file =
    switch(dz->process) {
    case(SPEC_REMOVE):  exit_status = outer_loop(dz);       break;
    case(SPECLEAN):     exit_status = do_speclean(0,dz);    break;
    case(SPECTRACT):    exit_status = do_speclean(1,dz);    break;
    case(SPECSLICE):    exit_status = do_specslice(dz);     break;
//  case(SPECTOVF):     exit_status = spectovf(dz);         break;
//  case(SPECTOVF2):    exit_status = spectovf2(dz);        break;
    case(SPECRAND):     exit_status = specrand(dz);         break;
    case(SPECSQZ):      exit_status = specsqz(dz);          break;
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

/************************ HANDLE_THE_EXTRA_INFILE *********************/

int handle_the_extra_infile(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
                    /* OPEN ONE EXTRA ANALFILE, CHECK COMPATIBILITY */
    int  exit_status;
    char *filename;
    fileptr fp2;
    int fileno = 1;
    double maxamp, maxloc;
    int maxrep;
    int getmax = 0, getmaxinfo = 0;
    infileptr ifp;
    fileptr fp1 = dz->infile; 
    filename = (*cmdline)[0];
    if((dz->ifd[fileno] = sndopenEx(filename,0,CDP_OPEN_RDONLY)) < 0) {
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
    if(fp2->filetype != ANALFILE) {
        sprintf(errstr,"%s is not an analysis file.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->origstype != fp1->origstype) {
        sprintf(errstr,"Incompatible original-sample-type in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->origrate != fp1->origrate) {
        sprintf(errstr,"Incompatible original-sample-rate in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->arate != fp1->arate) {
        sprintf(errstr,"Incompatible analysis-sample-rate in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->Mlen != fp1->Mlen) {
        sprintf(errstr,"Incompatible analysis-window-length in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->Dfac != fp1->Dfac) {
        sprintf(errstr,"Incompatible decimation factor in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if(fp2->channels != fp1->channels) {
        sprintf(errstr,"Incompatible channel-count in input file %s.\n",filename);
        return(DATA_ERROR);
    }
    if((dz->insams[fileno] = sndsizeEx(dz->ifd[fileno]))<0) {       /* FIND SIZE OF FILE */
        sprintf(errstr, "Can't read size of input file %s.\n"
        "open_checktype_getsize_and_compareheader()\n",filename);
        return(PROGRAM_ERROR);
    }
    if(dz->insams[fileno]==0) {
        sprintf(errstr, "File %s contains no data.\n",filename);
        return(DATA_ERROR);
    }
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

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
    dz->infilecnt = ONE_NONSND_FILE;
    //establish_bufptrs_and_extra_buffers():
    if(dz->process == SPECSLICE && dz->mode == 3) {
        if((exit_status = setup_specslice_parameter_storage(dz))<0)   
            return(exit_status);
    } else if(dz->process == SPECRAND) {
        dz->iarray_cnt=2;
        if((dz->iparray  = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for internal integer arrays.\n");
            return(MEMORY_ERROR);
        }
    } else {
        dz->array_cnt=2;
        if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
            return(MEMORY_ERROR);
        }
        dz->parray[0] = NULL;
        dz->parray[1] = NULL;
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

/***************************** HANDLE_THE_OUTFILE **************************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz)
{
    int exit_status, len;
    char *filename = NULL;
    if(dz->process == SPECSLICE && dz->mode < 4) {
        if(dz->wordstor!=NULL)
            free_wordstors(dz);
        dz->all_words = 0;
        if((exit_status = store_filename((*cmdline)[0],dz))<0)
            return(exit_status);
        len = strlen((*cmdline)[0]);
        if((filename = (char *)malloc(len + 2))==NULL) {
            sprintf(errstr,"handle_the_outfile()\n");
            return(MEMORY_ERROR);
        }
        strcpy(filename,dz->wordstor[0]);
        dz->itemcnt = 0;
    } else {
        filename = (*cmdline)[0];
    }
    strcpy(dz->outfilename,filename);      
    if((exit_status = create_sized_outfile(filename,dz))<0)
        return(exit_status);
    (*cmdline)++;
    (*cmdlinecnt)--;
    return(FINISHED);
}

/***************************** CREATE_NEXT_OUTFILE **************************/

int create_next_outfile(dataptr dz)
{
    int exit_status, len;
    char *filename = NULL;
    dz->itemcnt++;
    len = strlen(dz->wordstor[0]) + 5;
    if((filename = (char *)malloc(len))==NULL) {
        sprintf(errstr,"handle_the_outfile()\n");
        return(MEMORY_ERROR);
    }
    strcpy(filename,dz->wordstor[0]);
    insert_newnumber_at_filename_end(filename,dz->itemcnt,1);
    strcpy(dz->outfilename,filename);      
    if((exit_status = create_sized_outfile(filename,dz))<0)
        return(exit_status);
    return(FINISHED);
}

/***************************** INSERT_NEWNUMBER_AT_FILENAME_END **************************/

void insert_newnumber_at_filename_end(char *filename,int num,int overwrite_last_char)
/* FUNCTIONS ASSUMES ENOUGH SPACE IS ALLOCATED !! */
{
    char *p;
    char ext[64];
    p = filename + strlen(filename) - 1;
    while(p > filename) {
        if(*p == '/' || *p == '\\' || *p == ':') {
            p = filename;
            break;
        }
        if(*p == '.') {
            strcpy(ext,p);
            if(overwrite_last_char)
                p--;
            sprintf(p,"%d",num);
            strcat(filename,ext);
            return;
        }
        p--;
    }
    if(p == filename) {
        p += strlen(filename);
        if(overwrite_last_char)
            p--;
        sprintf(p,"%d",num);
    }
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

/************************* SETUP_THE_APPLICATION *******************/

int setup_the_application(dataptr dz)
{
    int exit_status;
    aplptr ap;
    if((exit_status = establish_application(dz))<0)     // GLOBAL
        return(FAILED);
    ap = dz->application;
    // SEE parstruct FOR EXPLANATION of next 2 functions
    switch(dz->process) {
    case(SPEC_REMOVE):
        if((exit_status = set_param_data(ap,0   ,4,4,"dddD"      ))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
            return(FAILED);
        // set_legal_infile_structure -->
        dz->has_otherfile = FALSE;
        // assign_process_logic -->
        dz->input_data_type = ANALFILE_ONLY;
        dz->process_type    = EQUAL_ANALFILE;   
        dz->outfiletype     = ANALFILE_OUT;
        break;
    case(SPECTRACT):
    case(SPECLEAN):
        if((exit_status = set_param_data(ap,0   ,2,2,"dd"      ))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
            return(FAILED);
        // set_legal_infile_structure -->
        dz->has_otherfile = FALSE;
        // assign_process_logic -->
        dz->input_data_type = TWO_ANALFILES;
        dz->process_type    = EQUAL_ANALFILE;   
        dz->outfiletype     = ANALFILE_OUT;
        break;
    case(SPECSLICE):
        switch(dz->mode) {
        case(0):
            if((exit_status = set_param_data(ap,0   ,2,2,"iI"      ))<0)
                return(FAILED);
            break;
        case(1):
        case(2):
            if((exit_status = set_param_data(ap,0   ,2,2,"iD"      ))<0)
                return(FAILED);
            break;
        case(3):
            if((exit_status = set_param_data(ap,P_BRK_DATA   ,0,0,""        ))<0)
                return(FAILED);
            break;
        case(4):
            if((exit_status = set_param_data(ap,0   ,1,1,"D"      ))<0)
                return(FAILED);
            break;
        }
        if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
            return(FAILED);
        // set_legal_infile_structure -->
        dz->has_otherfile = FALSE;
        // assign_process_logic -->
        dz->input_data_type = ANALFILE_ONLY;
        dz->process_type    = EQUAL_ANALFILE;   
        dz->outfiletype     = ANALFILE_OUT;
        break;
    case(SPECRAND):
        if((exit_status = set_param_data(ap,0   ,0,0,""))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"tg",2,"Di","",0,0,""))<0)
            return(FAILED);
        // set_legal_infile_structure -->
        dz->has_otherfile = FALSE;
        // assign_process_logic -->
        dz->input_data_type = ANALFILE_ONLY;
        dz->process_type    = EQUAL_ANALFILE;   
        dz->outfiletype     = ANALFILE_OUT;
        break;
    case(SPECSQZ):
        if((exit_status = set_param_data(ap,0   ,2,2,"DD"))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
            return(FAILED);
        // set_legal_infile_structure -->
        dz->has_otherfile = FALSE;
        // assign_process_logic -->
        dz->input_data_type = ANALFILE_ONLY;
        dz->process_type    = EQUAL_ANALFILE;   
        dz->outfiletype     = ANALFILE_OUT;
        break;
    }
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
    dz->clength     = dz->wanted / 2;
    dz->chwidth     = dz->nyquist/(double)(dz->clength-1);
    dz->halfchwidth = dz->chwidth/2.0;
    return(FINISHED);
}

/************************* SETUP_THE_PARAM_RANGES_AND_DEFAULTS *******************/

int setup_the_param_ranges_and_defaults(dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    // set_param_ranges()
    ap->total_input_param_cnt = (char)(ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt);
    // NB total_input_param_cnt is > 0 !!!s
    if((exit_status = setup_input_param_range_stores(ap->total_input_param_cnt,ap))<0)
        return(FAILED);
    // get_param_ranges()
    switch(dz->process) {
    case(SPEC_REMOVE):
        ap->lo[0]           = MIDIMIN;
        ap->hi[0]           = MIDIMAX;
        ap->default_val[0]  = 60;
        ap->lo[1]           = MIDIMIN;
        ap->hi[1]           = MIDIMAX;
        ap->default_val[1]  = 60;
        ap->lo[2]           = 0;
        ap->hi[2]           = dz->nyquist;
        ap->default_val[2]  = 6000.0;
        ap->lo[3]           = 0.0;
        ap->hi[3]           = 1.0;
        ap->default_val[3]  = 1.0;
        dz->maxmode = 2;
        break;
    case(SPECLEAN):
    case(SPECTRACT):
        ap->lo[0]           = 0.0;
        ap->hi[0]           = 1000.0;
        ap->default_val[0]  = dz->frametime * 8.0 * SECS_TO_MS;
        ap->lo[1]           = 1.0;
        ap->hi[1]           = CL_MAX_GAIN;
        ap->default_val[1]  = DEFAULT_NOISEGAIN;
        dz->maxmode = 0;
        break;
    case(SPECSLICE):
        switch(dz->mode) {
        case(0):
            ap->lo[0]           = 2;
            ap->hi[0]           = dz->clength/2;
            ap->default_val[0]  = 2;
            ap->lo[1]           = 1;
            ap->hi[1]           = dz->clength/2;
            ap->default_val[1]  = 1;
            break;
        case(1):
            ap->lo[0]           = 2;
            ap->hi[0]           = dz->clength/2;
            ap->default_val[0]  = 2;
            ap->lo[1]           = dz->chwidth;
            ap->hi[1]           = dz->nyquist/2.0;
            ap->default_val[1]  = dz->chwidth;
            break;
        case(2):
            ap->lo[0]           = 2;
            ap->hi[0]           = dz->clength/2;
            ap->default_val[0]  = 2;
            ap->lo[1]           = 0.5;
            ap->hi[1]           = (LOG2(dz->nyquist/dz->chwidth)/2.0) * SEMITONES_PER_OCTAVE;
            ap->default_val[1]  = SEMITONES_PER_OCTAVE;
            break;
        case(4):
            ap->lo[0]           = dz->chwidth;
            ap->hi[0]           = dz->nyquist - dz->chwidth;
            ap->default_val[0]  = dz->nyquist/2;
            break;
        }
        dz->maxmode = 4;
        break;
    case(SPECRAND):
        ap->lo[0]           = dz->frametime * 2;
        ap->hi[0]           = dz->duration;
        ap->default_val[0]  = dz->duration;
        ap->lo[1]           = 1;
        ap->hi[1]           = dz->wlength/2;
        ap->default_val[1]  = 1;
        dz->maxmode = 0;
        break;
    case(SPECSQZ):
        ap->lo[0]           = 0;
        ap->hi[0]           = dz->nyquist;
        ap->default_val[0]  = 440.0;
        ap->lo[1]           = 1.0/(double)dz->clength;
        ap->hi[1]           = 1;
        ap->default_val[1]  = 0.5;
        dz->maxmode = 0;
        break;
//  case(SPECTOVF):
//      ap->lo[SVF_RANGE]            = 0.0;
//      ap->hi[SVF_RANGE]            = 6.0;
//      ap->default_val[SVF_RANGE]   = 1.0;
//      ap->lo[SVF_MATCH]        = 1.0;
//      ap->hi[SVF_MATCH]        = (double)MAXIMI;
//      ap->default_val[SVF_MATCH]  = (double)ACCEPTABLE_MATCH;
//      ap->lo[SVF_HILM]         = SPEC_MINFRQ;
//      ap->hi[SVF_HILM]         = dz->nyquist/MAXIMI;
//      ap->default_val[SVF_HILM]   = dz->nyquist/MAXIMI;
//      ap->lo[SVF_LOLM]         = SPEC_MINFRQ;
//      ap->hi[SVF_LOLM]         = dz->nyquist/MAXIMI;
//      ap->default_val[SVF_LOLM]   = SPEC_MINFRQ;
//      dz->maxmode = 0;
//      break;
//  case(SPECTOVF2):
//      ap->lo[SVF2_HILM]           = SPEC_MINFRQ;      // maxfrq to look for peaks
//      ap->hi[SVF2_HILM]           = dz->nyquist;
//      ap->default_val[SVF2_HILM]  = ap->hi[0];
//      ap->lo[SVF2_PKNG]           = 1.0;              // how much louder than average-or-median must peak be, to be 'true' peak
//      ap->hi[SVF2_PKNG]           = 100.0;
//      ap->default_val[SVF2_PKNG]  = 2.0;
//      ap->lo[SVF2_CTOF]           = 0.0;              // relative cutoff amplitude (relative to window maxpeak)
//      ap->hi[SVF2_CTOF]           = 1.0;
//      ap->default_val[SVF2_CTOF]  = .01;
//      ap->lo[SVF2_WNDR]           = 0;                // semitone range that peak can wander and remain SAME peak-trail
//      ap->hi[SVF2_WNDR]           = 12.0;
//      ap->default_val[SVF2_WNDR]  = 1.0;
//      ap->lo[SVF2_PSST]           = dz->frametime * SECS_TO_MS;   // Duration (mS) for which peak-trail must persist, to be 'true' peak-trail.    
//      ap->hi[SVF2_PSST]           = 1.0 * SECS_TO_MS;
//      ap->default_val[SVF2_PSST]  = 4 * SECS_TO_MS;
//      ap->lo[SVF2_TSTP]           = dz->frametime * SECS_TO_MS;
//      ap->hi[SVF2_TSTP]           = 1.0 * SECS_TO_MS; //  Minimum timestep (mS) between data outputs to filter file.
//      ap->default_val[SVF2_TSTP]  = 4 * SECS_TO_MS;
//      ap->lo[SVF2_SGNF]           = 0;                //  semitone shift any of output peak must take before new filter val output.
//      ap->hi[SVF2_SGNF]           = 12.0;
//      ap->default_val[SVF2_SGNF]  = 0.5;
//      ap->lo[SVF2_LOLM]           = SPEC_MINFRQ;      // minfrq to look for peaks
//      ap->hi[SVF2_LOLM]           = dz->nyquist;
//      ap->default_val[SVF2_LOLM]  = SPEC_MINFRQ;
//      ap->lo[SVF2_WSIZ]           = 1.0;      // windowsize (semitones) used to locate peaks
//      ap->hi[SVF2_WSIZ]           = round(ceil(dz->nyquist/SPEC_MINFRQ)) * SEMITONES_PER_OCTAVE;
//      ap->default_val[SVF2_WSIZ]  = 12.0;
//      dz->maxmode = 0;
//      break;
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
    //aplptr ap;

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
            if((exit_status = setup_the_application(dz))<0)
                return(exit_status);
            //ap = dz->application;
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

/*********************** CHECK_THE_PARAM_VALIDITY_AND_CONSISTENCY *********************/

int check_the_param_validity_and_consistency(dataptr dz)
{
    int n, cnt, exit_status;
    double lo,hi,lostep,histep,semitonespan,maxval;
    switch(dz->process) {
    case(SPEC_REMOVE):
        dz->param[0] = miditohz(dz->param[0]);
        dz->param[1] = miditohz(dz->param[1]);
        lo = min(dz->param[0],dz->param[1]);
        if((dz->brksize[3] == 0) && flteq(dz->param[3],0.0)) {
            sprintf(errstr,"Attenuation is zero: spectrum will not be changed.\n");
            return(DATA_ERROR);
        }
        cnt = 0;
        hi = lo;
        while(hi <= dz->param[2]) {
            cnt++;
            hi += lo;
        }
        if(cnt == 0) {
            sprintf(errstr,"Upper search limit (%lf) means there are no pitches to search for.\n",dz->param[2]);
            return(MEMORY_ERROR);
        }
        hi = max(dz->param[0],dz->param[1]);
        dz->zeroset = 0;
        if(lo * 2.0 <= hi) {
            fprintf(stdout,"WARNING: Pitchrange of octave or more.\n");
            fprintf(stdout,"WARNING: Will eliminate entire spectrum between lower pitch and %lf Hz\n",dz->param[2]);
            fflush(stdout);
            dz->zeroset = 1;
            dz->param[0] = lo;
        } else {
            if((dz->parray[0] = (double *)malloc(cnt * sizeof(double)))==NULL) {
                sprintf(errstr,"No memory to store 1st set of harmonics.\n");
                return(MEMORY_ERROR);
            }
            if((dz->parray[1] = (double *)malloc(cnt * sizeof(double)))==NULL) {
                sprintf(errstr,"No memory to store 2nd set of harmonics.\n");
                return(MEMORY_ERROR);
            }
            lostep = lo;
            histep = hi;
            for(n = 0;n<cnt;n++) {
                dz->parray[0][n] = lo;
                dz->parray[1][n] = hi;
                lo += lostep;
                hi += histep;
            }
            dz->itemcnt = cnt;
        }
        break;
    case(SPECLEAN):
    case(SPECTRACT):
        dz->iparam[0] = (int)cdp_round((dz->param[0] * MS_TO_SECS)/dz->frametime);
        dz->iparam[0] = max(dz->iparam[0],1);
        n = dz->insams[0]/dz->wanted;
        if(n < dz->iparam[0] + 1) {
            sprintf(errstr,"File is too short for the 'persist' parameter used.\n");
            return(GOAL_FAILED);
        }
        break;
    case(SPECSLICE):
        if(dz->mode < 4) {
            if((exit_status = get_maxvalue(1,&maxval,dz)) < 0)
                return(exit_status);
        }
        if(dz->mode == 3) {
            dz->param[0] = (int)(floor(dz->nyquist/maxval));
            break;
        }
        switch(dz->mode) {
        case(0):
            if(dz->param[0] * maxval >= dz->clength) {
                sprintf(errstr,"Insufficient channels to accomodate your parameters.\n");
                return(GOAL_FAILED);
            }
            break;
        case(1):
            if(dz->param[0] * maxval >= dz->nyquist) {
                sprintf(errstr,"Insufficient bandwidth to accomodate your parameters.\n");
                return(GOAL_FAILED);
            }
            break;
        case(2):
            semitonespan = LOG2(dz->nyquist/dz->chwidth) * SEMITONES_PER_OCTAVE;
            if(dz->param[0] * maxval >= semitonespan) {
                sprintf(errstr,"Insufficient bandwidth to accomodate your parameters.\n");
                return(GOAL_FAILED);
            }
            break;
        }
        break;

    case(SPECRAND):
        if(dz->brksize[0]) {
            if((exit_status = get_minvalue_in_brktable(&(dz->param[0]),0,dz))<0)
                return(exit_status);
        }
        dz->iparam[0] = (int)round(dz->param[0]/dz->frametime); //  Convert timeframe to window count
        if(dz->iparam[0] < 2)
            dz->iparam[0] = 2;
        if (dz->iparam[1] * 2 >= dz->iparam[0]) {
            sprintf(errstr,"GROUPING TOO LARGE FOR (MINIUMUM) TIMEFRAME SPECIFIED\n");
            return DATA_ERROR;
        }
        if((dz->iparray[0]  = (int *)malloc((dz->wlength + 1) * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for internal integer array 0.\n");
            return(MEMORY_ERROR);
        }
        if((dz->iparray[1]  = (int *)malloc(dz->wlength * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for internal integer array 1.\n");
            return(MEMORY_ERROR);
        }
        break;
    case(SPECSQZ):
        break;
    }
    return FINISHED;
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

int get_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    return(FINISHED);
}

int read_special_data(char *str,dataptr dz) 
{
    return(FINISHED);
}

/********************************************************************************************/

int get_the_process_no(char *prog_identifier_from_cmdline,dataptr dz)
{
    if      (!strcmp(prog_identifier_from_cmdline,"remove"))        dz->process = SPEC_REMOVE;
    else if (!strcmp(prog_identifier_from_cmdline,"clean"))         dz->process = SPECLEAN;
    else if (!strcmp(prog_identifier_from_cmdline,"subtract"))      dz->process = SPECTRACT;
    else if (!strcmp(prog_identifier_from_cmdline,"slice"))         dz->process = SPECSLICE;
    else if (!strcmp(prog_identifier_from_cmdline,"rand"))          dz->process = SPECRAND;
    else if (!strcmp(prog_identifier_from_cmdline,"squeeze"))       dz->process = SPECSQZ;
    else {
        sprintf(errstr,"Unknown program identification string '%s'\n",prog_identifier_from_cmdline);
        return(USAGE_ONLY);
    }
    return(FINISHED);
}

/**************************** ALLOCATE_SPECLEAN_BUFFER ******************************/

int allocate_speclean_buffer(dataptr dz)
{
    unsigned int buffersize;
    buffersize = dz->wanted * (dz->bptrcnt + 2);
    dz->buflen = dz->wanted;
    if((dz->bigfbuf = (float*) malloc(buffersize * sizeof(float)))==NULL) {  
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/******************************** USAGE1 ********************************/

int usage1(void)
{
    fprintf(stderr,
    "\nFURTHER OPERATIONS ON ANALYSIS FILES\n\n"
    "USAGE: specnu NAME (mode) infile(s) outfile parameters: \n"
    "\n"
    "where NAME can be any one of\n"
    "\n"
//  "remove    makevfilt     makevfilt2\n\n";       NOT SATISFACTORY YET: DEC 2009
    "remove    clean    subtract    slice    rand    squeeze\n\n"
    "Type 'specnu remove' for more info on specnu remove..ETC.\n");
    return(USAGE_ONLY);
}

/******************************** USAGE2 ********************************/

int usage2(char *str)
{
    if(!strcmp(str,"remove")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu remove 1-2 inanalfile outanalfile midimin midimax rangetop atten\n"
        "\n"
        "MODE 1: removes pitch and its harmonics up to a specified frequency limit\n"
        "MODE 2: removes everything else.\n"
        "\n"
        "MIDIMIN    minimum pitch to remove (MIDI).\n"
        "MIDIMAX    maximum pitch to remove (MIDI).\n"
        "RANGETOP   frequency at which search for harmonics stops (Hz).\n"
        "ATTEN      attenuation of suppressed components (1 = max, 0 = none).\n"
        "\n"
        "Midi range normally should be small.\n"
        "If range is octave or more, whole spectrum between lower pitch & rangetop\n"
        "will be removed.\n"
        "\n");
    } else if(!strcmp(str,"clean")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu clean sigfile noisfile outanalfile persist noisgain\n"
        "\n"
        "Eliminate frqbands in 'sigfile' falling below maxlevels found in 'noisfile'\n"
        "\n"
        "PERSIST    min-time chan-signal > noise-level, to be retained (Range 0-1000 ms).\n"
        "NOISGAIN   multiplies noiselevels in noisfile channels before they are used\n"
        "           for comparison with infile signal: (Range 1 - %.0lf).\n"
        "\n"
        "Both input files are analysis files.\n"
        "'noisfil' is a sample of noise (only) from the 'sigfile'.\n"
        "\n",CL_MAX_GAIN);
    } else if(!strcmp(str,"subtract")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu subtract sigfile noisfile outanalfile persist noisgain\n"
        "\n"
        "Eliminate frqbands in 'sigfile' falling below maxlevels found in 'noisfile'\n"
        "and subtract amplitude of noise in noisfile channels from signal that is passed.\n"
        "\n"
        "PERSIST    min-time chan-signal > noise-level, to be retained (Range 0-1000 ms).\n"
        "NOISGAIN   multiplies noiselevels in noisfile channels before they are used\n"
        "           for comparison with infile signal: (Range 1 - %.0lf).\n"
        "\n"
        "Both input files are analysis files.\n"
        "'noisfil' is a sample of noise (only) from the 'sigfile'.\n"
        "\n",CL_MAX_GAIN);
    } else if(!strcmp(str,"slice")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu slice 1-3 inanalfile outanalfiles bandcnt chanwidth\n"
        "specnu slice 4 inanalfile outanalfiles pitchdata\n"
        "specnu slice 5 inanalfile outanalfile invertaroundfrq\n"
        "\n"
        "Slices spectrum (frqwise) into mutually exclusive parts, or inverts\n"
        "MODE 1: (moiree slice) -- chanwidth in analysis channels\n"
        "puts alternate (groups of) channels into different slices: e.g.\n"
        "bandcnt 3 chanwidth 1 -> outfil1: 0 3 6... outfil2: 1 4 7... outfil3: 2 5 8 ... \n"
        "bandcnt 2 chanwidth 2 -> outfil1: 0-1 4-5 9-8... outfil2: 2-3 6-7 10-11...\n"
        "(bandcnt must be > 1)\n"
        "MODE 2: (frq band slice) -- chanwidth in Hz\n"
        "puts alternate (groups of) chans into different equal-width frq bands: e.g.\n"
        "(bandcnt can be 1)\n"
        "MODE 3: (pitch band slice) -- chanwidth in semitones\n"
        "puts alternate (groups of) chans into different equal-width pitch bands: e.g.\n"
        "(bandcnt can be 1)\n"
        "MODE 4: (slice by harmonics) -- requires a text datafile of time-frq values\n"
        "(generated by \"repitch getpitch\"). Outputs a file for each harmonic tracked.\n"
        "\n"
        "With outfile \"myname\", produces files \"myname\", \"mynam1\", \"myname2\" etc.\n"
        "\n"
        "MODE 5: (invert freqwise) -- requires frq value to invert around\n"
        "\n");

    } else if(!strcmp(str,"rand")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu rand inanalfile outanalfile [-ttimescale] [-ggrouping]\n"
        "\n"
        "Randomise order of spectral windows.\n"
        "\n"
        "TIMESCALE determines no of windows locally-randomised. (dflt all windows)\n"
        "GROUPING  consecutive windows grouped into sets of size \"grouping\".\n"
        "          These sets are randomised (default grouping = 1 window per set).\n"
        "\n"
        "\"Timescale\" may vary over time\n"
        "\n");
    } else if(!strcmp(str,"squeeze")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu squeeze inanalfile outanalfile centrefrq squeeze\n"
        "\n"
        "Squeeze spectrum in frequency range, around a specified centre frq.\n"
        "\n"
        "CENTREFRQ frequency around which frequency data is squeezed.\n"
        "SQUEEZE   Amount of squeezing of spectrum (< 1)\n"
        "\n"
        "All parameters may vray over time.\n"
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

/*********************** INNER_LOOP ***************************/

int inner_loop
(int *peakscore,int *descnt,int *in_start_portion,int *least,int *pitchcnt,int windows_in_buf,dataptr dz)
{
    int exit_status;
//  int local_zero_set = FALSE;
    int wc;
    for(wc=0; wc<windows_in_buf; wc++) {
        if(dz->total_windows==0) {
            dz->flbufptr[0] += dz->wanted;
            dz->total_windows++;
            dz->time = (float)(dz->time + dz->frametime);
            continue;
        }
        if((exit_status = read_values_from_all_existing_brktables((double)dz->time,dz))<0)
            return(exit_status);
        if((exit_status = spec_remove(dz))<0)
            return(exit_status);
        dz->flbufptr[0] += dz->wanted;
        dz->total_windows++;
        dz->time = (float)(dz->time + dz->frametime);
    }
    return FINISHED;
}

/*********************** SPEC_REMOVE ***************************/

int spec_remove(dataptr dz)
{
    int vc, done, cnt;
    double *lo, *hi, gain = 1.0 - dz->param[3], lofrq;
    if(dz->zeroset) {
        if(dz->mode == 0) {
            for( vc = 0; vc < dz->wanted; vc += 2) {
                if((dz->flbufptr[0][FREQ] >= dz->param[0]) &&  (dz->flbufptr[0][FREQ] <= dz->param[2]))
                    dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * gain);
            }
        } else {
            for( vc = 0; vc < dz->wanted; vc += 2) {
                if((dz->flbufptr[0][FREQ] <= dz->param[0]) ||  (dz->flbufptr[0][FREQ] >= dz->param[2]))
                    dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * gain);
            }
        }
    } else {
        cnt = 0;
        done = 0;
        lo = dz->parray[0];
        hi = dz->parray[1];
        if(dz->mode == 0) {
            for( vc = 0; vc < dz->wanted; vc += 2) {
                if((dz->flbufptr[0][FREQ] >= lo[cnt]) &&  (dz->flbufptr[0][FREQ] <= hi[cnt]))
                    dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * gain);
                else {
                    while(dz->flbufptr[0][FREQ] >= hi[cnt]) {
                        cnt++;
                        if(cnt >= dz->itemcnt) {
                            done = 1;
                            break;
                        }
                    }
                    if(done)
                        break;
                }
            }
        } else {
            for( vc = 0; vc < dz->wanted; vc += 2) {
                if(dz->flbufptr[0][FREQ] < lo[cnt])
                    dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * gain);
                else {
                    if(cnt+1 >= dz->itemcnt)
                        lofrq = dz->nyquist;
                    else
                        lofrq = dz->nyquist = lo[cnt+1];
                    if(dz->flbufptr[0][FREQ] < lofrq && dz->flbufptr[0][FREQ] > hi[cnt]) {
                        dz->flbufptr[0][AMPP] = (float)(dz->flbufptr[0][AMPP] * gain);
                    } else {
                        cnt++;
                        if(cnt >= dz->itemcnt) {
                            done = 1;
                            break;
                        }
                    }
                }
                if(done)
                    break;
            }
        }
    }
    return(FINISHED);
}

/**************************** DO_SPECLEAN ****************************/

int do_speclean(int subtract,dataptr dz)
{
    int exit_status, cc, vc, k;
    int n, samps_read, total_samps = 0;
    float *fbuf, *obuf, *ibuf, *nbuf;
    int *persist, *on, fadelim;
    fadelim = dz->iparam[0] - 1;
    fbuf = dz->bigfbuf;
    obuf = dz->bigfbuf;
    nbuf = dz->bigfbuf + (dz->buflen * dz->iparam[0]);
    ibuf = nbuf - dz->buflen;
    if((persist = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
        sprintf(errstr,"No memory for persistence counters.\n");
        return(MEMORY_ERROR);
    }
    if((on = (int *)malloc(dz->clength * sizeof(int)))==NULL) {
        sprintf(errstr,"No memory for persistence counters.\n");
        return(MEMORY_ERROR);
    }
    memset((char *)persist,0,dz->clength * sizeof(int));
    memset((char *)on,0,dz->clength * sizeof(int));
    memset((char *)nbuf,0,dz->buflen * sizeof(float));
            /* ESTABLISH NOISE CHANNEL-MAXIMA */
    while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[1],0)) > 0) {
        if(samps_read < 0) {
            sprintf(errstr,"Failed to read data from noise file.\n");
            return(SYSTEM_ERROR);
        }       
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
            if(fbuf[AMPP] > nbuf[AMPP])
                nbuf[AMPP] = fbuf[AMPP];
        }
        total_samps += samps_read;
    }
    if(total_samps <= 0) {
        sprintf(errstr,"No data found in noise file.\n");
        return(SYSTEM_ERROR);
    }       
            /* MUTIPLY BY 'NOISEGAIN' FACTOR */
    for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2)
        nbuf[AMPP] = (float)(nbuf[AMPP] * dz->param[1]);
            /* SKIP FIRST WINDOWS */
    dz->samps_left = dz->insams[0];
    if((samps_read = fgetfbufEx(obuf, dz->buflen,dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Failed to read data from signal file.\n");
        return(SYSTEM_ERROR);
    }
    if(samps_read < dz->buflen) {
        sprintf(errstr,"Problem reading data from signal file.\n");
        return(PROGRAM_ERROR);
    }
    if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
        return(exit_status);
    dz->samps_left -= dz->buflen;
            /* READ IN FIRST GROUP OF SIGNAL WINDOWS */
    dz->samps_left = dz->insams[0] - dz->wanted;
    if((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen * dz->iparam[0],dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Failed to read data from signal file.\n");
        return(SYSTEM_ERROR);
    }
    if(samps_read < dz->buflen * dz->iparam[0]) {
        sprintf(errstr,"Problem reading data from signal file.\n");
        return(PROGRAM_ERROR);
    }
    dz->samps_left -= dz->buflen * dz->iparam[0];
    for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
        /* FOR EACH CHANNEL */
        obuf = dz->bigfbuf;
        fbuf = ibuf;
        /* MOVING BACKWARDS THROUGH FIRST GROUP OF WINDOWS */
        while(fbuf >= obuf) {
        /* COUNT HOW MANY CONTIGUOUS WINDOWS EXCEED THE NOISE THRESHOLD */
            if(fbuf[AMPP] > nbuf[AMPP])
                persist[cc]++;
            else
        /* BUT IF ANY WINDOW IS BELOW NOISE THRESHOLD, BREAK */
                break;
            fbuf -= dz->buflen;
        }
        /* IF NOT ALL WINDOWS ARE ABOVE THE NOISE THRESHOLD, ZERO OUTPUT OF CHANNEL */
        if(persist[cc] < dz->iparam[0])
            obuf[AMPP] = 0.0f;
        else {
            on[cc] = 1;
            if(subtract)
                obuf[AMPP] = (float)max(0.0,obuf[AMPP] - nbuf[AMPP]);
        }
    }
        /* WRITE FIRST OUT WINDOW */
    if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
        return(exit_status);
    obuf = dz->bigfbuf;
    fbuf = obuf + dz->buflen;
            /* MOVE ALL WINDOWS BACK ONE */
    for(n = 1; n < dz->iparam[0]; n++) {
        memcpy((char *)obuf,(char *)fbuf,dz->wanted * sizeof(float));
        fbuf += dz->wanted;
        obuf += dz->wanted;
    }
    obuf = dz->bigfbuf;
    fbuf = obuf + dz->buflen;
    while(dz->samps_left > 0) {
            /* READ A NEW WINDOW TO END OF EXISTING GROUP */
        if((samps_read = fgetfbufEx(ibuf,dz->buflen,dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Problem reading data from signal file.\n");
            return(PROGRAM_ERROR);
        }
        dz->samps_left -= dz->wanted;
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
            /* IF LEVEL IN ANY CHANNEL IN NEW WINDOW FALLS BELOW NOISE THRESHOLD */
            if(subtract)
                obuf[AMPP] = (float)max(0.0,obuf[AMPP] - nbuf[AMPP]);
            if(ibuf[AMPP] < nbuf[AMPP]) {
            /* IF CHANNEL IS ALREADY ON, KEEP IT, BUT DECREMENT ITS PERSISTENCE VALUE, and FADE OUTPUT LEVEL */
                if(on[cc]) {
                            /* If it's persisted longer than min, reset persistence to min */
                    if(persist[cc] > dz->iparam[0]) 
                        persist[cc] = dz->iparam[0];    
                            /* as persistence falls from min to zero, fade the output level */
                    if(--persist[cc] <= 0) {
                        on[cc] = 0;
                        obuf[AMPP] = 0.0f;
                    } else
                        obuf[AMPP] = (float)(obuf[AMPP] * ((double)persist[cc]/(double)dz->iparam[0]));
                } else
            /* ELSE, SET OUTPUT TO ZERO */
                    obuf[AMPP] = 0.0f;
            } else {
            /* IF CHANNEL LEVEL IS ABOVE NOISE THRESHOLD, COUNT ITS PERSISTANCE */
                persist[cc]++;
            /* IF ON ALREADY, RETAIN SIGNAL */
                if(on[cc])
                    ;
            /* ELSE IF PERSISTENCE IS INSUFFICIENT, ZERO OUTPUT */
                else if((k = persist[cc] - dz->iparam[0]) < 0)
                    obuf[AMPP] = 0.0f;
                else {
            /* ELSE IF PERSISTANCE IS BELOW FADELIM, FADE IN THAT OUTPUT */
                    if(persist[cc] < fadelim)
                        obuf[AMPP] = (float)(obuf[AMPP] * ((double)k/(double)dz->iparam[0]));
            /* ELSE RETAIN FULL LEVEL AND MARK CHANNEL AS (FULLY) ON */
                    else
                        on[cc] = 1; 
                }
            }
        }
            /* WRITE THE OUTPUT */
        if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
            return(exit_status);
            /* MOVE ALL WINDOWS BACK ONE */
        if(dz->samps_left) {
            for(n = 1; n < dz->iparam[0]; n++) {
                memcpy((char *)obuf,(char *)fbuf,dz->wanted * sizeof(float));
                fbuf += dz->wanted;
                obuf += dz->wanted;
            }
        }
        obuf = dz->bigfbuf;
        fbuf = obuf + dz->buflen;
    }
            /* DEAL WITH REMAINDER OF FINAL BLOCK OF WINDOWS */
    obuf += dz->buflen;
            /* ADVANCING THROUGH THE WINDOWS */
    while(obuf < nbuf) {
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
            if(!on[cc])
                obuf[AMPP] = 0.0f;
            else if(obuf[AMPP] < nbuf[AMPP]) {
                obuf[AMPP] = 0.0f;
                on[cc] = 0;
            } else if(subtract)
                obuf[AMPP] = (float)max(0.0,obuf[AMPP] - nbuf[AMPP]);
        }
        if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
            return(exit_status);
        obuf += dz->buflen;
    }
    return FINISHED;
}

/************************ GET_THE_MODE_NO *********************/

int get_the_mode_no(char *str, dataptr dz)
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

/************************ HANDLE_PITCHDATA *********************/

int handle_pitchdata(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int istime, valcnt;
    aplptr ap = dz->application;
    int n;
    char temp[200], *q, *filename = (*cmdline)[0];
    double *p, lasttime = 0.0;
    if(!sloom) {
        if(*cmdlinecnt <= 0) {
            sprintf(errstr,"Insufficient parameters on command line.\n");
            return(USAGE_ONLY);
        }
    }
    ap->data_in_file_only   = TRUE;
    ap->special_range       = TRUE;
    ap->min_special         = SPEC_MINFRQ;
    ap->max_special         = (dz->nyquist * 2.0)/3.0;
    if((dz->fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Cannot open datafile %s\n",filename);
        return(DATA_ERROR);
    }
    n = 0;
    while(fgets(temp,200,dz->fp)!=NULL) {
        q = temp;
        if(is_an_empty_line_or_a_comment(q))
            continue;
        n++;
    }
    if(n==0) {
        sprintf(errstr,"No data in file %s\n",filename);
        return(DATA_ERROR);
    }
    dz->brksize[1] = n;
    if ((dz->brk[1] = (double *)malloc(dz->brksize[1] * 2 * sizeof(double)))==NULL) {
        sprintf(errstr,"Insufficient memory to store pitch data\n");
        return(MEMORY_ERROR);
    }
    p = dz->brk[1];
    if(fseek(dz->fp,0,0)<0) {
        sprintf(errstr,"fseek() failed in handle_pitchdata()\n");
        return(SYSTEM_ERROR);
    }
    n = 0;
    while(fgets(temp,200,dz->fp)!=NULL) {
        q = temp;
        if(is_an_empty_line_or_a_comment(temp))
            continue;
        if(n >= dz->brksize[1]) {
            sprintf(errstr,"Accounting problem reading pitchdata\n");
            return(PROGRAM_ERROR);
        }
        istime = 1;
        valcnt = 0;
        while(get_float_from_within_string(&q,p)) {
            if(valcnt>=2) {
                sprintf(errstr,"Too many values on line %d: file %s\n",n+1,filename);
                return(DATA_ERROR);
            }

            if(istime) {
                if(n == 0) {
                    lasttime = *p;
                } else {
                    if(*p <= lasttime) {
                        sprintf(errstr,"Times fail to advance: line %d: file %s\n",n+1,filename);
                        return(DATA_ERROR);
                    }
                }
            } else {
                if(*p <= ap->min_special || *p >= ap->max_special) {
                    sprintf(errstr,"Pitch out of range (%lf - %lf) : line %d: file %s\n",ap->min_special,ap->max_special,n+1,filename);
                    return(DATA_ERROR);
                }
            }
            istime = !istime;
            valcnt++;
            p++;
        }
        if(!istime) {
            sprintf(errstr,"Time-pitch brkpnt data not correctly paired: line %d: file %s\n",n+1,filename);
            return(DATA_ERROR);
        }
        n++;
    }
    dz->brksize[1] = n;
    if(fclose(dz->fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
        fflush(stdout);
    }
    (*cmdline)++;       
    (*cmdlinecnt)--;
    return(FINISHED);
}

/**************************** DO_SPECSLICE ****************************/

int do_specslice(dataptr dz)
{
    int n, exit_status;
    float *ibuf = dz->bigfbuf, *obuf = dz->bigfbuf;
    double bigstep, thistime, bandbot, bandtop, frqmult;
    int ibigstep, ibandbot, ibandtop, cc, vc;
    int samps_read;
    n = 0;
    if(dz->mode == 4) {
        exit_status = specpivot(dz);
        return(exit_status);
    }
    while(n < dz->param[0]) {
        dz->total_samps_written = 0;
        if(n>0) {
            if(sndseekEx(dz->ifd[0],0,0)<0) {
                sprintf(errstr,"Seek failed in input analysis file.\n");
                return(SYSTEM_ERROR);
            }
            if((exit_status = create_next_outfile(dz)) < 0) {
                return(exit_status);
            }
        }
        thistime = 0.0;
        while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
            if(samps_read < 0) {
                sprintf(errstr,"Failed to read data from input file.\n");
                return(SYSTEM_ERROR);
            }
            if(dz->mode < 3) {
                if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
                    return(exit_status);
            }
            switch(dz->mode) {
            case(0):
                ibigstep = dz->iparam[0] * dz->iparam[1];
                ibandbot = dz->iparam[1] * n;
                ibandtop = ibandbot + dz->iparam[1];
                if(ibandbot >= dz->clength) {
                    sprintf(errstr,"Failure in internal channel accounting.\n");
                    return(PROGRAM_ERROR);
                }
                cc = 0;
                while(cc < dz->clength) {
                    vc = cc * 2;
                    while(cc < ibandbot) {
                        ibuf[AMPP]  = 0.0f; /* SETS AMPLITUDE OUTSIDE SELECTED BANDS TO ZERO */
                        cc++;
                        vc += 2;
                    }
                    while(cc < ibandtop) {
                        cc++;
                        vc += 2;
                    }
                    if((ibandbot += ibigstep) >= dz->clength)
                        ibandbot = dz->clength;
                    if((ibandtop = ibandbot + dz->iparam[1]) >= dz->clength)
                        ibandtop = dz->clength;
                }
                break;
            case(1):
                bigstep = dz->param[1] * (double)dz->iparam[0];
                bandbot = dz->param[1] * (double)n;
                if(bandbot >= dz->nyquist) {
                    sprintf(errstr,"Failure in internal channel accounting.\n");
                    return(PROGRAM_ERROR);
                }
                if((bandtop = bandbot + dz->param[1]) >= dz->nyquist)
                    bandtop = dz->nyquist;
                for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
                    if(ibuf[FREQ] < bandbot)
                        ibuf[AMPP] = 0.0f;
                    else if(ibuf[FREQ] >= bandtop) {
                        ibuf[AMPP] = 0.0f;
                        if((bandbot = bandbot + bigstep) >= dz->nyquist)
                            bandtop = dz->nyquist;
                        if((bandtop = bandbot + dz->param[1]) >= dz->nyquist)
                            bandtop = dz->nyquist;
                    }
                }
                break;
            case(2):
                frqmult = 1.0 + pow(2.0,dz->param[1]/SEMITONES_PER_OCTAVE);
                bandbot = dz->chwidth * pow(frqmult,(double)n);
                bigstep = pow(frqmult,(double)dz->iparam[0]);
                if((bandtop = bandbot * frqmult) >= dz->nyquist)
                    bandtop = dz->nyquist;
                if(n == 0)
                    bandbot = 0.0;
                for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
                    if(ibuf[FREQ] < bandbot)
                        ibuf[AMPP] = 0.0f;
                    else if(ibuf[FREQ] >= bandtop) {
                        ibuf[AMPP] = 0.0f;
                        if((bandbot *= bigstep) >= dz->nyquist)
                            bandbot = dz->nyquist;
                        if((bandtop = bandbot * frqmult) >= dz->nyquist)
                            bandtop = dz->nyquist;
                    }
                }
                break;
            case(3):
                if((exit_status = read_value_from_brktable(thistime,1,dz))<0)
                    return(exit_status);
                bandbot = (dz->param[1] * (double)n) + (dz->param[1]/2.0);
                if(bandbot >= dz->nyquist) {
                    sprintf(errstr,"Failure in internal channel accounting.\n");
                    return(PROGRAM_ERROR);
                }
                bandtop = bandbot + dz->param[0];
                if(n == 0)
                    bandbot = 0.0;
                for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
                    if(ibuf[FREQ] < bandbot)
                        ibuf[AMPP] = 0.0f;
                    else if(ibuf[FREQ] >= bandtop)
                        ibuf[AMPP] = 0.0f;
                }
                break;
            }
            thistime += dz->frametime;
            if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
                return(exit_status);
        }
        if((exit_status = headwrite(dz->ofd,dz))<0)
            return(exit_status);
        if((exit_status = reset_peak_finder(dz))<0)
            return(exit_status);
        if (n < dz->param[0] - 1) {
            if(sndcloseEx(dz->ofd) < 0) {
                fprintf(stdout,"WARNING: Can't close output soundfile %s\n",dz->outfilename);
                fflush(stdout);
            }
            dz->ofd = -1;
        }
        n++;
    }
    return FINISHED;
}

/**************************** SETUP_SPECSLICE_PARAMETER_STORAGE ****************************/

int setup_specslice_parameter_storage(dataptr dz) {

    if((dz->brk      = (double **)realloc(dz->brk,2 * sizeof(double *)))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 1\n");
        return(MEMORY_ERROR);
    }
    if((dz->brkptr   = (double **)realloc(dz->brkptr,2 * sizeof(double *)))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 6\n");
        return(MEMORY_ERROR);
    }
    if((dz->brksize  = (int    *)realloc(dz->brksize,2 * sizeof(long)))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 2\n");
        return(MEMORY_ERROR);
    }
    if((dz->firstval = (double  *)realloc(dz->firstval,2 * sizeof(double)))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 3\n");
        return(MEMORY_ERROR);
    }
    if((dz->lastind  = (double  *)realloc(dz->lastind,2 * sizeof(double)))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 4\n");
        return(MEMORY_ERROR);
    }
    if((dz->lastval  = (double  *)realloc(dz->lastval,2 * sizeof(double)))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 5\n");
        return(MEMORY_ERROR);
    }
    if((dz->brkinit  = (int     *)realloc(dz->brkinit,2 * sizeof(int)))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 7\n");
        return(MEMORY_ERROR);
    }
    dz->brk[1]     = NULL;
    dz->brkptr[1]  = NULL;
    dz->brkinit[1] = 0;
    dz->brksize[1] = 0;
    if((dz->param       = (double *)realloc(dz->param,2 * sizeof(double)))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 1\n");
        return(MEMORY_ERROR);
    }
    if((dz->iparam      = (int    *)realloc(dz->iparam, 2 * sizeof(int)   ))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 2\n");
        return(MEMORY_ERROR);
    }
    if((dz->is_int      = (char   *)realloc(dz->is_int,2 * sizeof(char)))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 3\n");
        return(MEMORY_ERROR);
    }
    if((dz->no_brk      = (char   *)realloc(dz->no_brk,2 * sizeof(char)))==NULL) {
        sprintf(errstr,"setup_specslice_parameter_storage(): 5\n");
        return(MEMORY_ERROR);
    }
    if((dz->is_active   = (char   *)realloc(dz->is_active,2 * sizeof(char)))==NULL) {
        sprintf(errstr,"setup_parameter_storage_and_constants(): 3\n");
        return(MEMORY_ERROR);
    }
    dz->is_int[1] = 0;
    dz->is_active[1] = 1;
    dz->no_brk[1] = (char)0;
    return FINISHED;
}
/* RWD: SPECTOVF, SPECTOVF2 code deleted 03/09/19 */

/************************************ SPECPIVOT **************************************/

int specpivot(dataptr dz)
{
    int exit_status, nullspec = 0, pivot = 0, vc1 = 0, vc2 = 0;
    float temp;
    int samps_read;
    double thistime = 0.0;
    if(dz->brksize[0] == 0) {
        pivot = (int)round(dz->param[0]/dz->chwidth);
        pivot *= 2;
        vc1 = pivot - 2;
        vc2 = pivot + 2;
        if(vc1 < 0 || vc2 >= dz->wanted) {
            sprintf(errstr,"Pivot frequency will generate a null spectrum\n");
            return(DATA_ERROR);
        }
    }
    while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
        if(dz->brksize[0]) {
            if((exit_status = read_value_from_brktable(thistime,0,dz))<0)
                return(exit_status);
            pivot = (int)round(dz->param[0]/dz->chwidth); 
            if(vc1 < 0 || vc2 >= dz->wanted)
                nullspec = 1;
        }
        vc1 = pivot - 2;
        vc2 = pivot + 2;
        for(;;) {
            if(vc1 < 0) {
                while(vc2 < dz->wanted) {
                    dz->bigfbuf[vc2] = 0.0f;
                    vc2 += 2;
                }
                break;
            } else if(vc2 >= dz->wanted) {
                while(vc1 >= 0) {
                    dz->bigfbuf[vc1] = 0.0f;
                    vc1 -= 2;
                }
                break;
            } else {
                temp = dz->bigfbuf[vc1];
                dz->bigfbuf[vc1] = dz->bigfbuf[vc2];
                dz->bigfbuf[vc2] = temp;
                vc1 -= 2;
                vc2 += 2;
            }
        }
        if((exit_status = write_samps(dz->bigfbuf,samps_read,dz))<0)
            return(exit_status);
        thistime += dz->frametime;
    }
    if(nullspec) {
        fprintf(stdout,"WARNING: some spectral windows ZEROed as pivot frq was too high or too low\n");
        fflush(stdout);
    }
    if(samps_read < 0) {
        sprintf(errstr,"Failed to read data from input file.\n");
        return(SYSTEM_ERROR);
    }
    return FINISHED;
}

/************************************ SPECRAND **************************************/

int specrand(dataptr dz)
{
    int n, m, exit_status, done = 0, outwindowcnt = 0;
    float *obuf = dz->bigfbuf;
    int samps_read;
    unsigned int thissample;
    double thistime = 0.0;
    int windowframestart, windowrangesize, windowgroupsize, scramblesetsize, thisgrpstart, thiswindow;

    int *perm = dz->iparray[1], *rwindow = dz->iparray[0];
    fprintf(stdout,"INFO: Calculating output window sequence.\n");
    fflush(stdout);
    windowframestart = 0;
    while(windowframestart < dz->wlength) {
        if(dz->brksize[0]) {
            if((exit_status = read_value_from_brktable(thistime,0,dz))<0)
                return(exit_status);
            dz->iparam[0] = (int)round(dz->param[0]/dz->frametime);
            if(dz->iparam[0] < 2)
                dz->iparam[0] = 2;                              //  Get size of timeframe (in windows) to randomise
        }
        windowrangesize = dz->iparam[0];
        windowgroupsize = dz->iparam[1]; 
        scramblesetsize = windowrangesize/windowgroupsize;      //  Number of window-groups to permute
        rndintperm(perm,scramblesetsize);                       //  Randomise order
        for(n = 0; n < scramblesetsize; n++) {
            thisgrpstart = perm[n] * windowgroupsize;           //  Find start of next (permd) group-of-windows, from zero baseline
            thiswindow   = thisgrpstart + windowframestart;     //  Find true window this refers to
            for(m=0;m<windowgroupsize;m++) {                    //  For all windows in group
                if(thiswindow >= dz->wlength)
                    break;
                if(outwindowcnt >= dz->wlength) {
                    done = 1;
                    break;
                }
                rwindow[outwindowcnt++] = thiswindow++;         //  Assign window as next in output sequence of windows
            }
            if(done)
                break;
        }
        windowframestart += windowrangesize;
        thistime += windowrangesize * dz->frametime;
    }
    fprintf(stdout,"INFO: Writing output windows.\n");
    fflush(stdout);
    for(n = 0; n < dz->wlength;n++) {
        thiswindow = rwindow[n];
        thissample = thiswindow * dz->wanted;
        if((sndseekEx(dz->ifd[0],thissample,0)<0)){        
            sprintf(errstr,"sndseek() failed at window %d at time %lf : thissample = %d len = %d windows = %d\n",thiswindow,thiswindow * dz->frametime,thissample,dz->insams[0],dz->wlength);
            return SYSTEM_ERROR;
        }
        if((samps_read = fgetfbufEx(obuf, dz->buflen,dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Failed to read data from input file at time %lf.\n",thiswindow * dz->frametime);
            return(SYSTEM_ERROR);
        }
        if((exit_status = write_samps(obuf,dz->wanted,dz))<0)
            return(exit_status);
    }
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

/************************************ SPECSQZ **************************************/

int specsqz(dataptr dz)
{
    int exit_status, cc, vc;
    double centrefrq, squeeze, thisamp, thisfrq;
    int thischan;
    int samps_read;
    float *ibuf = dz->bigfbuf, *obuf = dz->flbufptr[2];
    double thistime = 0.0;
    while((samps_read = fgetfbufEx(dz->bigfbuf, dz->buflen,dz->ifd[0],0)) > 0) {
        if(samps_read < 0) {
            sprintf(errstr,"Failed to read data from input file.\n");
            return(SYSTEM_ERROR);
        }
        memset((char *)obuf,0,dz->wanted * sizeof(float));
        if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
            return exit_status;
        centrefrq = dz->param[0];
        squeeze =   dz->param[1];
        for(cc = 0, vc = 0; cc < dz->clength; cc++, vc +=2) {
            thisamp = ibuf[AMPP];
            thisfrq = ibuf[FREQ] - centrefrq;
            thisfrq *= squeeze;
            thisfrq += centrefrq;
            thischan = (int)round(thisfrq/dz->chwidth);
            if(thisamp > obuf[thischan * 2])
                obuf[thischan * 2] = (float)thisamp;
        }
        if((exit_status = write_samps(obuf,samps_read,dz))<0)
            return(exit_status);
        thistime += dz->frametime;
    }
    return FINISHED;
}

/**************************** ALLOCATE_SPECSQZ_BUFFER ******************************/

int allocate_specsqz_buffer(dataptr dz)
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
    if((dz->bigfbuf = (float*)malloc(dz->buflen*2 * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize = dz->buflen;
    dz->flbufptr[2]  = dz->bigfbuf + dz->big_fsize;   
    dz->flbufptr[3]  = dz->flbufptr[2] + dz->big_fsize;
    return(FINISHED);
}

