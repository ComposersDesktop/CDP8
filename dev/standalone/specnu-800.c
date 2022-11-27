/*
 * Copyright (c) 1983-2020 Trevor Wishart and Composers Desktop Project Ltd
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
/* RWD 15/12/20 remove usage handling for "sync" */
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
#include <repitch.h>
#include <formants.h>
#include <flags.h>
#include <ctype.h>
#include <sfsys.h>
#include <string.h>
#include <srates.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

#define total_windows_read  rampbrksize
#define last_windows_read   temp_sampsize
#define total_windows_written   buflen2
#define maxfraccnt linecnt
#define harmcnt linecnt
#define copycnt temp_sampsize

#define SPEX_TIM    0
#define SPEX_DUR    1
#define SPEX_STR    2
#define SPEX_WIN    3
#define SPEX_STT    0
#define SPEX_END    1

#define INV_BOT     0
#define INV_TOP     1
#define INV_PKTOP   2
#define INV_GAIN    3

#define ENDPAD_WINDOWS 16

char errstr[2400];

int anal_infiles = 1;
int sloom = 0;
int sloombatch = 0;

const char* cdp_version = "8.0.0";

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

static int specex(dataptr dz);
static int create_specex_buffers(dataptr dz);
// static int handle_syncdata(int *cmdlinecnt,char ***cmdline,dataptr dz);
// static int create_sync_buffers(dataptr dz);
static int advance_along_input_windows(int wdw_to_advance,dataptr dz);
static int generate_zero_window(int *owcnt,int atend,dataptr dz);
static int copy_final_window(int *owcnt,dataptr dz);
static int do_timestretching(int *thatwindow,int *owcnt,double abswinptr,dataptr dz);
// static int specsync(dataptr dz);
// static void force_wav_extension(char *filename);
static int transpart(dataptr dz);
static int transpose_within_formant_envelope(int vc,dataptr dz);
static int re_position_partials_in_appropriate_channels(dataptr dz);
#ifdef NOTDEF
//RWD: NOT USED
static int spec_trnsp(dataptr dz);
static int spec_trnsf(dataptr dz);
#endif
static int setup_formants(dataptr dz);
static int set_spec_env_frqs(int arraycnt,dataptr dz);
static int setup_octave_band_steps(double **interval,dataptr dz);
static int setup_lo_octave_bands(int arraycnt,dataptr dz);
static int extract_spec_env(int storeno,dataptr dz);
static int move_data_into_appropriate_chan(int vc,int truevc,float thisamp,float thisfrq,dataptr dz)    ;
static int specnu_normalise(double pre_totalamp,double post_totalamp,int paramno,dataptr dz);
static int spec_keep_formants(int ccbot,int cctop,dataptr dz);
static int shift_within_formant_envelope(int vc,dataptr dz);
static int spec_move_formants(int ccbot,int cctop,dataptr dz);
static int nuinvert(dataptr dz);
static int invert_spectrum(int ccbot,int cctop,int ccpk,int wins_read,dataptr dz);
static int find_maxamp_channel(int cclo,int cchi,float *maxamp,dataptr dz);
static int find_minamp_channel(int cclo,int cchi,float *minamp,dataptr dz);
static int create_contours(float maxamplo,float maxamphi,int botmaxloc,int topmaxloc,int cctop,dataptr dz);
static int do_spectral_invert(int ccbot,int cctop,float amprange,float minamp,dataptr dz);
static int allocate_quad_buffer(dataptr dz);
static int convolve(dataptr dz);
static int self_convolve(float *buf,int *wins_read,double *ratio,double *ratiosum,double *maxin,double *maxout,int passno,dataptr dz);
static int do_convolve(float *buf,float *buflast,float *buf2,float *buf2last,int *wins_read,double *ratio,double *ratiosum,
                double *maxin,double *maxout,int passno,dataptr dz);
static double chanadjust(double frq,double fbot,double ftop,dataptr dz);
static int do_specsnd(int *obufcnt,int positive,double transbot,double transrange,dataptr dz);
static int specsnd(dataptr dz);

static int create_specfrac_buffers(dataptr dz);
static int check_fractal_param_validity_and_consistency(dataptr dz);
static int specfrac(dataptr dz);
static int fractal_copy_and_shrink(dataptr dz);
static int fractal_copy_reverse_shorten(dataptr dz);
static int frac_copy(int *bufpos,int startwin,int endwin,dataptr dz);
static int frac_reverse_copy(int *bufpos,int startwin,int endwin,dataptr dz);
static int frac_interp_shrink(dataptr dz);
static int create_new_outfile(int n,char *outfilename,dataptr dz);
static int frac_add_shrink(dataptr dz);
static int fractal_reverse_copy_and_shrink(dataptr dz);
static int frac_reverse_second_half(int *buf2pos,int startwin,int endwin,dataptr dz);
static int frac_interleave(int gpcnt,dataptr dz);
static int frac_interleave_reverse(int gpcnt,dataptr dz);
static int frac_interleave_cycle(int gpcnt,dataptr dz);
static int frac_swap(int *buf2pos,int startwin,int endwin,dataptr dz);
static int fractal_by_halves(dataptr dz);
static int fractal_by_interleaving(dataptr dz);

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
//      case(SPECSYNC):     dz->maxmode = 0; break;
        case(SPECEX):       dz->maxmode = 0; break;
        case(TRANSPART):    dz->maxmode = 8; break;
        case(SPECINVNU):    dz->maxmode = 0; break;
        case(SPECCONV):     dz->maxmode = 2; break;
        case(SPECSND):      dz->maxmode = 0; break;
        case(SPECFRAC):     dz->maxmode = 9; break;
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
    ap = dz->application;
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
    if(dz->process == SPECLEAN || dz->process == SPECTRACT || (dz->process == SPECCONV && dz->mode == 1)) {
        if((exit_status = handle_the_extra_infile(&cmdline,&cmdlinecnt,dz))<0) {
            if(sloom) {
                if(dz->process == SPECCONV && dz->mode == 1) {
                    fprintf(stdout,"ERROR: This mode of the program needs TWO input file.\n");
                    fflush(stdout);
                    print_messages_and_close_sndfiles(exit_status,is_launched,dz);
                    return(FAILED);
                }
            } else {
                print_messages_and_close_sndfiles(exit_status,is_launched,dz);
                return(FAILED);
            }
        }
    }
    // handle_outfile() = 
    if((exit_status = handle_the_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
        if(sloom && dz->process == SPECCONV && dz->mode == 0) {
            fprintf(stdout,"ERROR: This mode of the program only takes ONE input file\n");
            fflush(stdout);
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        } else {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
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
//  case(SPECSYNC):     dz->extra_bufcnt =  0;  dz->bptrcnt = 6;                break;
    case(SPECEX):       dz->extra_bufcnt =  0;  dz->bptrcnt = 2;                break;
    case(TRANSPART):    dz->extra_bufcnt =  0;  dz->bptrcnt = 2;                break;
    case(SPECINVNU):    dz->extra_bufcnt =  0;  dz->bptrcnt = 5;                break;
    case(SPECCONV):     dz->extra_bufcnt =  0;  dz->bptrcnt = 6;                break;
    case(SPECSND):      dz->extra_bufcnt =  0;  dz->bptrcnt = 5;                break;
    case(SPECFRAC):     dz->extra_bufcnt =  0;  dz->bptrcnt = 3;                break;
    }
    if(dz->process == SPECEX) {
        if((exit_status = create_specex_buffers(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    } else {
        if((exit_status = establish_spec_bufptrs_and_extra_buffers(dz))<0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
        switch(dz->process) {
        case(SPEC_REMOVE):  exit_status = allocate_single_buffer(dz);   break;
        case(SPECRAND):
        case(SPECSLICE):
        case(SPECTRACT):
        case(SPECLEAN):     exit_status = allocate_speclean_buffer(dz); break;
        case(SPECSQZ):      exit_status = allocate_specsqz_buffer(dz);  break;
        case(TRANSPART):    exit_status = allocate_single_buffer(dz);   break;
        case(SPECINVNU):    exit_status = allocate_double_buffer(dz);   break;
        case(SPECCONV):
            switch(dz->mode) {
            case(0):        exit_status = allocate_double_buffer(dz);   break;
            case(1):        exit_status = allocate_quad_buffer(dz);     break;
            }
            break;
        case(SPECSND):      exit_status = allocate_double_buffer(dz);   break;
        case(SPECFRAC):     exit_status = create_specfrac_buffers(dz);  break;
        }
        if(exit_status < 0) {
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);
            return(FAILED);
        }
    }   
    //param_preprocess()                        redundant
    //spec_process_file =
    switch(dz->process) {
    case(SPEC_REMOVE):  exit_status = outer_loop(dz);       break;
    case(SPECLEAN):     exit_status = do_speclean(0,dz);    break;
    case(SPECTRACT):    exit_status = do_speclean(1,dz);    break;
    case(SPECSLICE):    exit_status = do_specslice(dz);     break;
    case(SPECRAND):     exit_status = specrand(dz);         break;
    case(SPECSQZ):      exit_status = specsqz(dz);          break;
//  case(SPECSYNC):     exit_status = specsync(dz);         break;
    case(SPECEX):       exit_status = specex(dz);           break;
    case(TRANSPART):    exit_status = transpart(dz);        break;
    case(SPECINVNU):    exit_status = nuinvert(dz);         break;
    case(SPECCONV):     exit_status = convolve(dz);         break;
    case(SPECSND):      exit_status = specsnd(dz);          break;
    case(SPECFRAC):     exit_status = specfrac(dz);         break;
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
    } else if(dz->process == SPECEX) {
        dz->iarray_cnt=1;
        if((dz->iparray  = (int **)malloc(dz->iarray_cnt * sizeof(int *)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for internal integer arrays.\n");
            return(MEMORY_ERROR);
        }
    } else if(dz->process == TRANSPART || dz->process == SPECINVNU || dz->process == SPECCONV || dz->process == SPECSND || dz->process == SPECFRAC) {
        ;
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

/***************************** HANDLE_THE_OUTFILE **************************/

int handle_the_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz)
{
    int exit_status, len;
    int origsrate, origchans;
    char *filename = NULL;
    if(dz->process == SPECSLICE && dz->mode < 4) {
        if(dz->wordstor!=NULL)
            free_wordstors(dz);
        dz->all_words = 0;
        if((exit_status = store_filename((*cmdline)[0],dz))<0)
            return(exit_status);
        len = strlen((*cmdline)[0]);
        if((filename = (char *)malloc(len + 8))==NULL) {
            sprintf(errstr,"handle_the_outfile()\n");
            return(MEMORY_ERROR);
        }
        strcpy(filename,dz->wordstor[0]);
        dz->itemcnt = 0;
    } else {
        len = strlen((*cmdline)[0]);
        if((filename = (char *)malloc(len + 8))==NULL) {
            sprintf(errstr,"handle_the_outfile()\n");
            return(MEMORY_ERROR);
        }
        strcpy(filename,(*cmdline)[0]);
    }
//  if(dz->process == SPECSYNC)         //  Force ".wav" extension
//      force_wav_extension(filename);
    strcpy(dz->outfilename,filename);      

    if(dz->process == SPECSND) {
        origchans = dz->infile->channels;
        origsrate = dz->infile->srate;
        dz->infile->channels = 1;
        dz->infile->srate = dz->infile->origrate;
        if((exit_status = create_sized_outfile(filename,dz))<0)
            return(exit_status);
        dz->infile->channels = origchans;
        dz->infile->channels = origsrate;
    } else if(dz->process == SPECRAND || dz->process == SPECSQZ || dz->process == SPECEX || dz->process == TRANSPART) {
        if((exit_status = create_sized_outfile(filename,dz))<0)
            return(exit_status);
    } else if(dz->process == SPECFRAC) {
        if(dz->wordstor!=NULL)
            free_wordstors(dz);
        dz->all_words = 0;
        if((exit_status = store_filename((*cmdline)[0],dz))<0)
            return(exit_status);
    }
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
        sprintf(errstr,"create_next_outfile NO MEMORY\n");
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
    case(SPECEX):
        if((exit_status = set_param_data(ap,0   ,3,3,"ddd"))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"w",1,"i","se",2,0,"00"))<0)
            return(FAILED);
        // set_legal_infile_structure -->
        dz->has_otherfile = FALSE;
        // assign_process_logic -->
        dz->input_data_type = ANALFILE_ONLY;
        dz->process_type    = BIG_ANALFILE; 
        dz->outfiletype     = ANALFILE_OUT;
        break;
    case(TRANSPART):
        if((exit_status = set_param_data(ap,0   ,3,3,"DDd"))<0)
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
    case(SPECINVNU):
        if((exit_status = set_param_data(ap,0   ,4,4,"dddd"))<0)
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
    case(SPECCONV):
        if((exit_status = set_param_data(ap,0   ,1,1,"d"))<0)
            return(FAILED);
        if(dz->mode == 1) {
            if((exit_status = set_vflgs(ap,"d",1,"i","p",1,0,""))<0)
            return(FAILED);
        } else {
            if((exit_status = set_vflgs(ap,"d",1,"i","",0,0,""))<0)
                return(FAILED);
        }
        // set_legal_infile_structure -->
        dz->has_otherfile = FALSE;
        // assign_process_logic -->
        switch(dz->mode) {
        case(0):    
            dz->input_data_type = ANALFILE_ONLY;
            dz->process_type    = EQUAL_ANALFILE;   
            break;  
        case(1):    
            dz->input_data_type = TWO_ANALFILES;    
            dz->process_type    = BIG_ANALFILE; 
            break;
        }
        dz->outfiletype     = ANALFILE_OUT;
        break;
    case(SPECSND):
        if((exit_status = set_param_data(ap,0   ,2,2,"ii"))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"",0,"","",0,0,""))<0)
            return(FAILED);
        // set_legal_infile_structure -->
        dz->has_otherfile = FALSE;
        // assign_process_logic -->
        dz->input_data_type = ANALFILE_ONLY;
        dz->process_type    = UNEQUAL_SNDFILE;  
        dz->outfiletype     = SNDFILE_OUT;
        break;
    case(SPECFRAC):
        if((exit_status = set_param_data(ap,0   ,1,1,"i"))<0)
            return(FAILED);
        if((exit_status = set_vflgs(ap,"",0,"","i",1,0,""))<0)
            return(FAILED);
        // set_legal_infile_structure -->
        dz->has_otherfile = FALSE;
        // assign_process_logic -->
        dz->input_data_type = ANALFILE_ONLY;
        dz->process_type    = OTHER_PROCESS;    
        dz->outfiletype     = NO_OUTPUTFILE;
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
    case(SPECEX):
        ap->lo[SPEX_TIM]            = 0;
        ap->hi[SPEX_TIM]            = dz->duration;
        ap->default_val[SPEX_TIM]   = 0;
        ap->lo[SPEX_DUR]            = dz->frametime * 3;
        ap->hi[SPEX_DUR]            = dz->duration;
        ap->default_val[SPEX_DUR]   = dz->frametime * 3;
        ap->lo[SPEX_STR]            = 2.0;
        ap->hi[SPEX_STR]            = 32767.0;
        ap->default_val[SPEX_STR]   = 64.0;
        ap->lo[SPEX_WIN]            = 1;
        ap->hi[SPEX_WIN]            = 8;
        ap->default_val[SPEX_WIN]   = 1;
        dz->maxmode = 0;
        break;
    case(TRANSPART):
        switch(dz->mode) {
        case(0):    //  fall thro
        case(1):
        case(2):
        case(3):
            ap->lo[0]   = -48;
            ap->hi[0]   = 48;
            ap->default_val[0]  = 12;
            break;
        case(4):    //  fall thro
        case(5):
        case(6):
        case(7):
            ap->lo[0]   = -dz->nyquist/4;
            ap->hi[0]   = dz->nyquist/4;
            ap->default_val[0]  = 100;
            break;
        }
        ap->lo[1]   = 5.0;
        ap->hi[1]   = dz->nyquist/2.0;
        ap->default_val[1]  = 440;
        ap->lo[2]   = 0.1;
        ap->hi[2]   = 1.0;
        ap->default_val[2]  = 1.0;
        dz->maxmode = 8;
        break;
    case(SPECINVNU):
        ap->lo[0]   = 0.0;
        ap->hi[0]   = dz->nyquist/2;
        ap->default_val[0]  = 0.0;
        ap->lo[1]   = 100.0;
        ap->hi[1]   = dz->nyquist;
        ap->default_val[1]  = dz->nyquist;
        ap->lo[2]   = 0;
        ap->hi[2]   = dz->nyquist;
        ap->default_val[2]  = 4;
        ap->lo[3]   = 0.1;
        ap->hi[3]   = 1.0;
        ap->default_val[3]  = 1.0;
        dz->maxmode = 0;
        break;
    case(SPECCONV):
        ap->lo[0]   = 0.1;
        ap->hi[0]   = 10.0;
        ap->default_val[0]  = 1.0;
        ap->lo[1]   = 1;
        if(dz->mode == 0)
            ap->hi[1]   = 10;
        else
            ap->hi[1]   = 2;
        ap->default_val[1]  = 1;
        dz->maxmode = 2;
        break;
    case(SPECSND):
        ap->lo[0]   = 0;
        ap->hi[0]   = 8;
        ap->default_val[0]  = 0;
        ap->lo[1]   = 1;
        ap->hi[1]   = 8;
        ap->default_val[1]  = 1;
        dz->maxmode = 0;
        break;
    case(SPECFRAC):
        switch(dz->mode) {
        case(6):    //  fall thro
        case(8):
            ap->lo[0]   = 2;
            break;
        default:
            ap->lo[0]   = 1;
            break;
        }
        ap->hi[0]   = 100;
        ap->default_val[0]  = 8;
        dz->maxmode = 9;
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
            if((exit_status = setup_the_application(dz))<0)
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

/*********************** CHECK_THE_PARAM_VALIDITY_AND_CONSISTENCY *********************/

int check_the_param_validity_and_consistency(dataptr dz)
{
    int n, m, cnt, exit_status;
    double lo,hi,lostep,histep,semitonespan, maxval = 0.0;
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
            sprintf(errstr,"GROUPING TOO LARGE FOR (MINIMUM) TIMEFRAME SPECIFIED : Max approx = %d\n",dz->iparam[0]/2);
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
//  case(SPECSYNC):
        break;
    case(SPECEX):
        histep = dz->duration - dz->param[SPEX_TIM];
        if(dz->param[SPEX_DUR] >= histep) {
            sprintf(errstr,"Duration of portion to be stretched extends beyond end of file.\n");
            return(DATA_ERROR);
        }
        dz->iparam[SPEX_TIM] = (int)round(dz->param[SPEX_TIM]/dz->frametime);       //  Start window of portion to be stretched
        dz->iparam[SPEX_DUR] = (int)round(dz->param[SPEX_DUR]/dz->frametime);       //  Number of windows in portion to be stretched

        n = dz->iparam[SPEX_DUR]/dz->iparam[SPEX_WIN];                              //  Stretch-duration must be a multiple of window-grouping
        dz->iparam[SPEX_DUR] = n * dz->iparam[SPEX_WIN];
        if(dz->iparam[SPEX_DUR] <= dz->iparam[SPEX_WIN]) {
            sprintf(errstr,"Portion to be stretched is too short to use groupings of %d windows.\n",dz->iparam[SPEX_WIN]);
            return(DATA_ERROR);
        }
        break;
    case(TRANSPART):
        if(dz->mode < 4) {
            if(dz->brksize[0]) {
                for(n=0,m=1;n<dz->brksize[0];n++,m+=2)
                    dz->brk[0][m] = SEMITONES_AS_RATIO(dz->brk[0][m]);
            } else
                dz->param[0] = SEMITONES_AS_RATIO(dz->param[0]);
        }
        break;
    case(SPECINVNU):
        if(dz->param[1] <= dz->param[0]) {
            sprintf(errstr,"top of inversion-area must be ABOVE bottom of inversion-area.\n");
            return DATA_ERROR;
        }
        lostep = dz->param[1] - dz->param[0];
        n = (int)round(lostep/dz->chwidth);
        if(n < 2) {
            sprintf(errstr,"Frq range of inversion-area (%lf) too small for analfile's windowsize (%lf).\n",lostep,dz->chwidth);
            return DATA_ERROR;
        }
        if(dz->param[2] < dz->param[0]) {
            sprintf(errstr,"top of peak-search-area must be within inversion-area.\n");
            return DATA_ERROR;
        }
        if(dz->param[2] > dz->param[0] + lostep/2) {
            sprintf(errstr,"top of peak-search-area must not be more than halfway through inversion-area.\n");
            return DATA_ERROR;
        }
        break;
    case(SPECCONV):
        if(dz->mode == 1) {
            if(dz->insams[1] > dz->insams[0]) {
                sprintf(errstr,"Wrong number of input files : Cannot proceed.\n");
                return DATA_ERROR;
            }
        }
        break;
    case(SPECSND):
        if(dz->iparam[1] <= dz->iparam[0]) {
            sprintf(errstr,"Range of transpositions Zero or inverted.\n");
            return(DATA_ERROR);
        }
        break;
    case(SPECFRAC):
        if((exit_status = check_fractal_param_validity_and_consistency(dz))<0)
            return(exit_status);
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
//  else if (!strcmp(prog_identifier_from_cmdline,"sync"))          dz->process = SPECSYNC;
    else if (!strcmp(prog_identifier_from_cmdline,"extend"))        dz->process = SPECEX;
    else if (!strcmp(prog_identifier_from_cmdline,"transpart"))     dz->process = TRANSPART;
    else if (!strcmp(prog_identifier_from_cmdline,"invert"))        dz->process = SPECINVNU;
    else if (!strcmp(prog_identifier_from_cmdline,"convolve"))      dz->process = SPECCONV;
    else if (!strcmp(prog_identifier_from_cmdline,"tosound"))       dz->process = SPECSND;
    else if (!strcmp(prog_identifier_from_cmdline,"fractal"))       dz->process = SPECFRAC;
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
    "remove    clean    subtract    slice    rand    squeeze\n\n"
    "extend   transpart    convolve     tosound\n\n"
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
#ifdef NOTDEF
        /* RWD : process excluded by TW, so needs excluding here too */
    } else if(!strcmp(str,"sync")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu sync inanalfile outanalfile syncdata\n"
        "\n"
        "SYNCDATA  is a textfile containing two columns of times.\n"
        "          The times in column 1 are the \"sync-to\" times\n"
        "          and the times in column 2, the \"sync-from\" times.\n"
        "\n"
        "          The input file is to be timewarped so that the\n"
        "          sounds located at the \"sync-from\" times in the input file\n"
        "          are moved to the corresponding \"sync-to\" times in the output.\n"
        "\n"
        "          A file can be point-to-point synchronised with a 2nd file\n"
        "          by listing, in col-1, times in 2nd file which are to be syncd TO,\n"
        "          and, listing in col-2 the corresponding times in the infile\n"
        "          which must be moved to sync with the those in col-1.\n"
        "\n"
        "          Times in both columns must increase,\n"
        "          and times in the \"sync-from\" column (2) cannot reach beyond\n"
        "          the duration of the input file.\n"
        "\n");
#endif
    } else if(!strcmp(str,"extend")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu extend inanalfile outanalfile time dur stretch -wwingroup -s -e\n"
        "\n"
        "Stretch time by repeating, with random perms (groups of) windows.\n"
        "\n"
        "TIME      Start time of portion to extend.\n"
        "DUR       Duration of portion to extend.\n"
        "STRETCH   Time-stretching multiplier.\n"
        "WINGROUP  number of windows in groups to rand-permute. Default 1.\n"
        "-s        Retain spectrum prior to time-stretched portion.\n"
        "-e        Retain spectrum after time-stretched portion.\n"
        "\n");
    } else if(!strcmp(str,"transpart")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu transpart inanalfile outanalfile 1-4 transpose from gain\n"
        "specnu transpart inanalfile outanalfile 5-8 shift     from gain\n"
        "\n"
        "Transpose spectrum by semitones\n"
        "MODES 1   only ABOVE given frequency : retain formants in place.\n"
        "MODES 2   only BELOW given frequency : retain formants in place.\n"
        "MODES 3   only ABOVE given frequency : formants moved.\n"
        "MODES 4   only BELOW given frequency : formants moved.\n"
        "Frequency Shift spectrum in Hz\n"
        "MODES 5   only ABOVE given frequency : retain formants in place.\n"
        "MODES 6   only BELOW given frequency : retain formants in place.\n"
        "MODES 7   only ABOVE given frequency : formants moved.\n"
        "MODES 8   only BELOW given frequency : formants moved.\n"
        "\n"
        "TRANSPOSE Semitone transposition. Range 4 octaves, up or down.\n"
        "SHIFT     Frequency shift. Range nyquist/4 up or down.\n"
        "FROM      Frq above/below which spectrum shifted (Range: 5 to nyquist/2).\n"
        "GAIN      Overall gain (Range: .1 to 1).\n"
        "\n");
    } else if(!strcmp(str,"invert")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu invert inanalfile outanalfile top lopkratio hipkratio gain\n"
        "\n"
        "Inverts the spectral amplitudes over a specified range of the spectrum.\n"
        "Using spectral peak to create spectral-amplitude-envelope used in inversion.\n"
        "Referring to the inverted portion of the spectrum as \"invspec\" .....\n"
        "\n"
        "BOT       Lowest frequency to include in invspec.\n"
        "TOP       Highest frequency to include in invspec.\n"
        "PEAKTOP   Highest frequency of area to search for spectral peak.\n"
        "          This must be within first half of invspec.\n"
        "GAIN      Overall gain (Range: .1 to 1).\n"
        "\n");
    } else if(!strcmp(str,"convolve")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu convolve 1 inanalfile outanalfile gain [-ddepth]\n"
        "specnu convolve 2 inanal1 inanal2 outanalfile gain [-ddepth]\n"
        "\n"
        "MODE 1    Convolve sound with itself.\n"
        "MODE 2    Convolve two sounds  : 2nd shorter or equal to 1st.\n"
        "\n"
        "GAIN      Overall gain (Range: .1 to 1).\n"
        "DEPTH     Repeat process this number of times (Default once only).\n"
        "\n");
    } else if(!strcmp(str,"tosound")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu tosound inanalfile outsndfile mintrans maxtrans\n"
        "\n"
        "Spectral envelope is treated as a waveform.\n"
        "\n"
        "Pitchline derived from local maximum amplitudes of the spectral windows.\n"
        "These are placed in a range defined by the max and min limits given.\n"
        "\n"
        "MINTRANS   Lower limit of 8va transposition (Range: 0 to 8).\n"
        "MAXTRANS   Upper limit of 8va transposition (Range: 1 to 8).\n"
        "\n");
    } else if(!strcmp(str,"fractal")) {
        fprintf(stdout,
        "USAGE:\n"
        "specnu fractal 1-5 inanalfile outanalfile depth [-i]\n"
        "\n"
        "Create fractal version of spectrum.\n"
        "NB Ouput sounds depend on number of analysis channels used in source.\n"
        "\n"
        "Mode 1:  Abutt 2 copies, then shrink to orig size by time-contraction.\n"
        "Mode 2:  Abutt copy with reverse-copy, then shrink by time-contraction.\n"
        "Mode 3:  As mode 2, but cut into segments and process each segment in turn.\n"
        "Mode 4:  As mode 3, but shrink by averaging.\n"
        "Mode 5:  Swap two halves of segments.\n"
        "Mode 6:  reverse 2nd half of segments.\n"
        "Mode 7:  Interleave two halves of segments.\n"
        "Mode 8:  Reverse 2nd half of segments, then interleave.\n"
        "Mode 9: Interleave, retaining original speed.\n"
        "\n"
        "DEPTH  Degree of fractal segmentation.\n"
        "-i     Output all intermediate stages of fractalisation.\n"
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
    double *p, lasttime = 0.0, maxfrq = -100.0, thisharmfrq;
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
                maxfrq = max(maxfrq,*p);
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
    dz->harmcnt = 0;
    thisharmfrq = maxfrq; 
    while(thisharmfrq < dz->nyquist) {
        dz->harmcnt++;
        thisharmfrq += maxfrq;
    }
    if(dz->harmcnt == 0) {
        sprintf(errstr,"No valid count of harmonics found in file.\n");
        return DATA_ERROR;
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
// JULY 2020
    dz->itemcnt = -1;
    if(dz->mode == 3)
        dz->param[0] = dz->harmcnt;
// JULY 2020 ABOVE
    while(n < dz->param[0]) {
        dz->total_samps_written = 0;
        if(sndseekEx(dz->ifd[0],0,0)<0) {
            sprintf(errstr,"Seek failed in input analysis file.\n");
            return(SYSTEM_ERROR);
        }
        if((exit_status = create_next_outfile(dz)) < 0) {
            return(exit_status);
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
//                      if((bandbot = bandbot + bigstep) >= dz->nyquist)
//                          bandtop = dz->nyquist;
//                      if((bandtop = bandbot + dz->param[1]) >= dz->nyquist)
//                          bandtop = dz->nyquist;
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
//                      if((bandbot *= bigstep) >= dz->nyquist)
//                          bandbot = dz->nyquist;
//                      if((bandtop = bandbot * frqmult) >= dz->nyquist)
//                          bandtop = dz->nyquist;
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
                bandtop = (dz->param[1] * (double)(n+1)) + (dz->param[1]/2.0);
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
    if((dz->brksize  = (int    *)realloc(dz->brksize,2 * sizeof(int)))==NULL) {
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

/********************************** SPECSYNC **********************************/

int specsync(dataptr dz)
{
    int exit_status = CONTINUE, owcnt = 0;                          //  owcnt = window-position in output buffer
    int n, thatwindow = 0, pad, remain;
    double *syncto = dz->parray[0], *syncfrom = dz->parray[1];
    double abswinptr = 0.0, stretch, incr, startsyncto;             //  abswinptr points to current time in spectrum being stretched.
    int   winptr = 0;                                              //  winptr points to current window in spectrum being stretched.
    startsyncto = syncto[0];
    if((exit_status = generate_zero_window(&owcnt,0,dz))<0)         //  output always starts with a zero-window
        return exit_status;

    //  Where sync-to times starts at zero but syncfrom time is > zero: 
    //  move pointer in INPUT FILE to relevant (post-zero) window.
    
    if(syncto[0] == 0.0 && syncfrom[0] > 0.0) {
        winptr  = (int) round(syncfrom[0]);
        abswinptr = (double)winptr;

    //  Where sync-to times start AFTER zero, but sync-from times start at zero: 
    //  pad output with zero-windows 
    
    } else if(syncto[0] > 0.0 && syncfrom[0] == 0.0) {
        pad = (int) round(syncto[0]);
        while(pad > 1) {                                            //  If this is more than a window length after start of sync-to file, pad output with zero-windows.
            if((exit_status = generate_zero_window(&owcnt,0,dz))<0)
                return exit_status;
            pad--;
        }

    // Where Neither first sync-point is at zero, put in extra points at time zero
    //  If you want files NOT to start in sync, pre-pad with silence

    } else if(syncto[0] > 0.0 && syncfrom[0] > 0.0) {   
        for(n = dz->itemcnt;n > 0; n--) {
            syncto[n] = syncto[n-1];
            syncfrom[n] = syncfrom[n-1];
        }
        syncto[0] = syncfrom[0] = 1;    //  Start from window 1
        startsyncto = syncto[0];
        dz->itemcnt++;
    }

    for(n=1;n<dz->itemcnt;n++) {                                    //  Stretch outfile by required timestep divided by actual timestep.
        stretch = (syncto[n] - startsyncto)/(syncfrom[n] - syncfrom[n-1]);
        incr = 1.0/stretch;                                         //  To make it longer (stretch > 1), incr in outfile is <1 ... and vice versa.
        while(abswinptr < syncfrom[n]) {                            //  Stretch file until we reach required time
            if((exit_status = do_timestretching(&thatwindow,&owcnt,abswinptr,dz))<0)
                return exit_status;
            if(exit_status == FINISHED)
                break;
            abswinptr += incr;
            if(abswinptr >= dz->wlength-1) {                        //  If we reach final window of input, set FINISHED and quit
                exit_status = FINISHED;
                break;
            }
        }
        if(exit_status == FINISHED)
            break;                                                  //  SELF-CORRECTION!!!!
        startsyncto = (double)(dz->total_windows_written + owcnt);  //  Revise start-time of syncto seg to be same as time actually reached in output
    }                                                               //  So length ratio for stretching calcd from true position (not where we're theortically at)
    if(exit_status != FINISHED) {
        incr = 1.0;                                                 //  If any of infile remains, read it out
        while(abswinptr < dz->wlength-1) {
            if((exit_status = do_timestretching(&thatwindow,&owcnt,abswinptr,dz))<0)
                return exit_status;
            if(exit_status == FINISHED)
                break;
            abswinptr += incr;
        }
    }
    if((exit_status = copy_final_window(&owcnt,dz))<0)              //  Ensure final window is copied
        return exit_status;;                                                
    if((exit_status = generate_zero_window(&owcnt,1,dz))<0)         //  Add a zero window at end, to avoid clip (will this work??)
        return exit_status;

    if((remain = dz->flbufptr[OHERE] - dz->flbufptr[OBUF]) > 0) {   //  Write any remaining samples
        if((exit_status = write_samps(dz->flbufptr[OBUF],remain,dz))<0)
            return(exit_status);
    }       
    return FINISHED;
}

/*************************** DO_TIMESTRETCHING ********************************
 *
 * Read/interpolate source analysis windows.
 *
 * (1)  The initial window to intepolate FROM is found by truncating the non-integer window count
 *      e.g. position 3.2 translates to window 3.
 * (2)  Is this is a move on from the current window position? How far?
 * (3)  If we need to move on, call advance-windows and set up a new PAIR of
 *      input windows.
 * (4)  Get Interpolation time-fraction.
 * (5)  Get output values by interpolation between input values.
 * (6)  Advance window-count in output buffer.
 * (7)  If outbuf full, write a complete out-windowbuf to output file, and reset outputbuf window-count to zero.
 */

int do_timestretching(int *thatwindow,int *owcnt,double abswinptr,dataptr dz)
{
    int exit_status;
    double amp0, amp1, frq0, frq1;
    int  vc;
    int thiswindow, step;
    double frac;
    thiswindow = (int) abswinptr; /* TRUNCATE */    /* 1 */
    step = thiswindow - *thatwindow;                /* 2 */
    if(step) {                                      /* 3 */ 
        if((exit_status = advance_along_input_windows(step,dz))<0)
            return(exit_status);
        if(exit_status == FINISHED)                 //  No more input to read
            return(FINISHED);
    }
    frac = abswinptr - (double)thiswindow;          /* 4 */
    for(vc = 0; vc < dz->wanted; vc += 2) {
        amp0 = dz->flbufptr[IHERE][AMPP];           /* 5 */
        frq0 = dz->flbufptr[IHERE][FREQ];
        amp1 = dz->flbufptr[INEXT][AMPP];
        frq1 = dz->flbufptr[INEXT][FREQ];
        dz->flbufptr[OHERE][AMPP]  = (float)(amp0 + ((amp1 - amp0) * frac));
        dz->flbufptr[OHERE][FREQ]  = (float)(frq0 + ((frq1 - frq0) * frac));
    }
    (*owcnt)++;                                     /* 6 */
    if((dz->flbufptr[OHERE] += dz->wanted) >= dz->flbufptr[OEND]) {
        if((exit_status = write_exact_samps(dz->flbufptr[OBUF],dz->buflen,dz))<0)
            return(exit_status);                    /* 7 */
        dz->flbufptr[OHERE] = dz->flbufptr[OBUF];
        dz->total_windows_written += SYNCBUFSCNT;
        *owcnt = 0;
    }
    *thatwindow = thiswindow;
    return(CONTINUE);
}

/*************************** COPY_FINAL_WINDOW ****************************/

int copy_final_window(int *owcnt,dataptr dz)
{
    int exit_status;
    int  vc;
    for(vc = 0; vc < dz->wanted; vc += 2) {
        dz->flbufptr[OHERE][AMPP]  = dz->flbufptr[IHERE][AMPP];
        dz->flbufptr[OHERE][FREQ]  = dz->flbufptr[IHERE][FREQ];
    }
    (*owcnt)++;
    memcpy((char *)dz->flbufptr[OEND],(char *)dz->flbufptr[OHERE],dz->wanted * sizeof(float));  //  remember last window written, to set final zero-window vals
    if((dz->flbufptr[OHERE] += dz->wanted) >= dz->flbufptr[2]) {
        if((exit_status = write_exact_samps(dz->flbufptr[OBUF],dz->buflen,dz))<0)
            return(exit_status);                    /* 6 */
        dz->flbufptr[OHERE] = dz->flbufptr[OBUF];
        dz->total_windows_written += SYNCBUFSCNT;
        *owcnt = 0;
    }
    return(FINISHED);
}

/*************************** GENERATE_ZERO_WINDOW ****************************/

int generate_zero_window(int *owcnt,int atend,dataptr dz)
{
    int exit_status;
    int  vc;
    if(atend) {
        for(vc = 0; vc < dz->wanted; vc += 2) {
            dz->flbufptr[OHERE][AMPP]  = (float)0.0;
            dz->flbufptr[OHERE][FREQ]  = dz->flbufptr[OEND][FREQ];      //  Retain frqs of last window written
        }
    } else {
        for(vc = 0; vc < dz->wanted; vc += 2) {
            dz->flbufptr[OHERE][AMPP]  = (float)0.0;
            dz->flbufptr[OHERE][FREQ]  = (float)((vc/2) * dz->chwidth); //  Central freq of channel
        }
    }
    (*owcnt)++;
    if((dz->flbufptr[OHERE] += dz->wanted) >= dz->flbufptr[OEND]) {
        if((exit_status = write_exact_samps(dz->flbufptr[OBUF],dz->buflen,dz))<0)
            return(exit_status);                    /* 6 */
        dz->flbufptr[OHERE] = dz->flbufptr[OBUF];
        dz->total_windows_written += SYNCBUFSCNT;
        *owcnt = 0;
    }
    return(FINISHED);
}

/*************************** ADVANCE_ALONG_INPUT_WINDOWS ****************************
 *
 * Advance window frame in input.
 * (1)  Advance lastwindow to thiswindow.
 * (2)  Advance thiswindow to next.
 * (3)  If at end of buffer, copy first of buffer-pair window to start of whole buffer i.e. to dz->bigbuf
 * (4)  And read more data in AFTER that: i.e. at IBUF.
 * (5)  If there are no more windows in source, return FINISHED.
 */

int advance_along_input_windows(int wdw_to_advance,dataptr dz)           
{
    int n;
    int samps_read;
    for(n=0;n<wdw_to_advance;n++) {
        dz->flbufptr[IHERE] = dz->flbufptr[INEXT];                      /* 1 */
        if((dz->flbufptr[INEXT] += dz->wanted) >= dz->flbufptr[OBUF]) { /* 2 */
            memmove((char *)dz->bigfbuf,(char *)dz->flbufptr[IHERE],dz->wanted * sizeof(float));
            dz->flbufptr[IHERE] = dz->bigfbuf;                          /* 3 */
            dz->flbufptr[INEXT] = dz->flbufptr[IBUF];                   /* 4 */
            if((samps_read = fgetfbufEx(dz->flbufptr[INEXT],dz->buflen,dz->ifd[0],0)) < 0) {
                sprintf(errstr,"Can't read samples from input soundfile\n");
                return(SYSTEM_ERROR);
            }
            if(samps_read == 0)                                         /* 5 */
                return FINISHED;
            dz->last_windows_read = dz->total_windows_read;
            dz->total_samps_read += samps_read;
            dz->total_windows_read = dz->total_samps_read/dz->wanted;
        }
    }
    return(CONTINUE);
}

/*************************** CREATE_SYNC_BUFFERS ****************************/

int create_sync_buffers(dataptr dz)
{
    int n;
    int buffersize;
    if((dz->flbufptr = (float **)malloc(sizeof(float *) * dz->bptrcnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for float sample buffers.\n");
        return(MEMORY_ERROR);
    }
    for(n = 0;n <dz->bptrcnt; n++)
        dz->flbufptr[n] = NULL;

    buffersize = dz->wanted * ((SYNCBUFSCNT * 2) + 2);                      //  Extra windows for (pre)wrap-around of input.
                                                                            //  2nd extra window to remember frqs in last window written.
    dz->buflen = SYNCBUFSCNT * dz->wanted;                                  //  Normal read & write size is SYNCBUFCNT windows.
    if((dz->bigfbuf = (float*)malloc(buffersize * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    //  Fixed pointers
    dz->flbufptr[IBUF] = dz->bigfbuf + dz->wanted;                          //  Input buf is offset from bufstart by 1 window, for wrap-around
    dz->flbufptr[OBUF] = dz->flbufptr[IBUF] + (SYNCBUFSCNT * dz->wanted);   //  Output buf
    dz->flbufptr[OEND] = dz->flbufptr[OBUF] + (SYNCBUFSCNT * dz->wanted);   //  Last frqs written are stored after OEND
    //  Moving pointers
    dz->flbufptr[IHERE] = dz->bigfbuf;                  //  Initial read starts at the pre-wraparound window and reads 1 extra window. 
                                                        //  After 1st read, last window is wrapped around to dz->bigfbuf. So IHERE is reset to this pre-wrap window.
    dz->flbufptr[INEXT] = dz->flbufptr[IBUF];           //  Subsequent reads are to IBUF (1 window after dz->bigfbuf), and INEXT is (re)set to there.
    dz->flbufptr[OHERE] = dz->flbufptr[OBUF];           //  Output write pos preset to OBUF start
    return(FINISHED);
}

/************************ HANDLE_SYNCDATA *********************
 *
 *  2 columns, 1st if times to sync TO,
 *  and 2nd of times in input file which need to be moved (by timestretch)
 *  so they coincide with the sync-to times.
 */

int handle_syncdata(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int linecnt, inlinecnt, cnt;
    char temp[200], *q, *filename = (*cmdline)[0];
    double dummy, lasttotime = -1.0, lastattime = -1.0;
    double *timetosyncto, *timetosyncfrom;
    if(!sloom) {
        if(*cmdlinecnt <= 0) {
            sprintf(errstr,"Insufficient parameters on command line.\n");
            return(USAGE_ONLY);
        }
    }
    if((dz->fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr,"Cannot open datafile %s\n",filename);
        return(DATA_ERROR);
    }

    //  COUNT AND TEST INPUT DATA

    cnt = 0;
    linecnt = 0;
    while(fgets(temp,200,dz->fp)!=NULL) {
        q = temp;
        if(is_an_empty_line_or_a_comment(q))
            continue;
        inlinecnt = 0;
        while(get_float_from_within_string(&q,&dummy)) {
            if(dummy < 0.0) {
                sprintf(errstr,"Negative time value (%lf) in sync data.\n",dummy);   /* RWD 04/08/20 missing arg dummy */
                return DATA_ERROR;
            }
            if(EVEN(cnt)) {
                if(lasttotime < 0.0)
                    lasttotime = dummy;
                else {
                    if(dummy <= lasttotime) {
                        sprintf(errstr,"Sync-to times (%lf and %lf) do not advance at lines %d and %d\n",lasttotime,dummy,linecnt,linecnt+1);
                        return DATA_ERROR;
                    }
                    lasttotime = dummy;
                }
            } else {
                if(lastattime < 0.0)
                    lastattime = dummy;
                else {
                    if(dummy <= lastattime) {
                        sprintf(errstr,"Sync-from times (%lf and %lf) do not advance at lines %d and %d\n",lastattime,dummy,linecnt,linecnt+1);
                        return DATA_ERROR;
                    }
                    lastattime = dummy;
                }
            }
            inlinecnt++;
            cnt++;
        }
        if(inlinecnt != 2) {
            sprintf(errstr,"Times not paired correctly on line %d\n",linecnt+1);
            return DATA_ERROR;
        }
        linecnt++;
    }
    if(cnt==0) {
        sprintf(errstr,"No data in file %s\n",filename);
        return(DATA_ERROR);
    }
    if(lastattime > dz->duration) {
        sprintf(errstr,"Synchronise-from times reach beyond the end of the input file.\n");
        return(MEMORY_ERROR);
    }
    dz->itemcnt = cnt/2;
    if ((dz->parray = (double **)malloc(2 * sizeof(double *)))==NULL) {
        sprintf(errstr,"Insufficient memory to create syntime data storage.\n");
        return(MEMORY_ERROR);
    }
    if ((dz->parray[0] = (double *)malloc((dz->itemcnt + 2) * sizeof(double)))==NULL) {
        sprintf(errstr,"Insufficient memory to create sync_to times array.\n");
        return(MEMORY_ERROR);
    }
    if ((dz->parray[1] = (double *)malloc((dz->itemcnt + 2) * sizeof(double)))==NULL) {
        sprintf(errstr,"Insufficient memory to create sync_at times array.\n");
        return(MEMORY_ERROR);
    }
    timetosyncto   = dz->parray[0];
    timetosyncfrom = dz->parray[1];
    if(fseek(dz->fp,0,0)< 0) {
        sprintf(errstr,"fseek() failed in handle_syncdata()\n");
        return(SYSTEM_ERROR);
    }

    //  READ INPUT DATA TO TWO ARRAYS, CONVERTING TIMES TO (possibly fractional) WINDOW-COUNTS

    cnt = 0;
    while(fgets(temp,200,dz->fp)!=NULL) {
        q = temp;
        if(is_an_empty_line_or_a_comment(temp))
            continue;
        while(get_float_from_within_string(&q,&dummy)) {
            if(EVEN(cnt)) {
                *timetosyncto = dummy/dz->frametime;        //  Converts synctime to (possibly fractional) windowcnts
                timetosyncto++;
            } else {
                *timetosyncfrom = dummy/dz->frametime;
                timetosyncfrom++;
            }
            cnt++;
        }
    }
    if(fclose(dz->fp)<0) {
        fprintf(stdout,"WARNING: Failed to close input textfile %s.\n",filename);
        fflush(stdout);
    }
    (*cmdline)++;       
    (*cmdlinecnt)--;
    return(FINISHED);
}

/************************ FORCE_WAV_EXTENSION *********************/

void force_wav_extension(char *filename)
{
    char *p = filename;
    while(*p != '.') {
        p++;
        if(*p == ENDOFSTR)
            break;
    }
    if(*p == ENDOFSTR)
        strcat(filename,".wav");
    else {
        p++;
        if(*p != 'w') {
            p--;
            *p = ENDOFSTR;
            strcat(filename,".wav");
        }
    }
}

/************************************ SPECEX **************************************/

int specex(dataptr dz)
{
    int n, k, exit_status, wins_out = 0, wins_read = 0, windows_needed;
    float *obuf = dz->flbufptr[0], *permbuf = dz->flbufptr[1], *obufptr, *obufend, *permwin;
    int samps_read, permcnt, winloc;
    int *perm;
    windows_needed = (int) round(dz->iparam[SPEX_DUR] * dz->param[SPEX_STR]);       //  Number of Windows generated by stretch

    dz->tempsize = windows_needed;                                                  //  Windows generated by stretching segment
    if(dz->vflag[0])
        dz->tempsize += dz->iparam[SPEX_TIM];                                       //  If start to be included, include start
    if(dz->vflag[1])
        dz->tempsize += dz->wlength - (dz->iparam[SPEX_TIM] + dz->iparam[SPEX_DUR]);//  If end to be included, include end
    dz->tempsize *= dz->wanted;
    obufptr = obuf;
    obufend = permbuf;
    if(dz->vflag[SPEX_STT] && (dz->iparam[SPEX_TIM] > 0)) {
        while(wins_read < dz->iparam[SPEX_TIM]) {
            if((samps_read = fgetfbufEx(obufptr, dz->wanted, dz->ifd[0],0)) < 0) {
                sprintf(errstr,"Failed to read data from input file at time %lf.\n",wins_read * dz->frametime);
                return(SYSTEM_ERROR);
            }
            wins_read++;
            obufptr += dz->wanted;
            if(obufptr >= obufend) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufptr = obuf;
            }
        }
    }
    if((samps_read = fgetfbufEx(permbuf, dz->buflen, dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Failed to read data from input file at time %lf.\n",wins_read * dz->frametime);
        return(SYSTEM_ERROR);
    }
    wins_read += dz->buflen/dz->wanted;
    permcnt = dz->iparam[SPEX_DUR]/dz->iparam[SPEX_WIN];    //  Forced to be an intiger in parameter-checking
    if((dz->iparray[0]  = (int *)malloc(permcnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for permutations array.\n");
        return(MEMORY_ERROR);
    }
    perm = dz->iparray[0];

    while(wins_out < windows_needed) {
        rndintperm(perm,permcnt);
        for(n = 0; n < permcnt; n++) {
            winloc = perm[n] * dz->iparam[SPEX_WIN] * dz->wanted;   //  perm-value * no of windows in permed-groups * size-of-window
            permwin = permbuf + winloc;
            for(k = 0; k < dz->iparam[SPEX_WIN]; k++) {             //  Copy from this point, the total number of windows in the permed-group
                memcpy((char *)obufptr,(char *)permwin,dz->wanted * sizeof(float));
                permwin += dz->wanted;
                obufptr += dz->wanted;
                if(obufptr >= obufend) {
                    if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                        return(exit_status);
                    obufptr = obuf;
                }
                wins_out++;
                if(wins_out >= windows_needed)
                    break;
            }
            if(wins_out >= windows_needed)
                break;
        }
    }
    if(dz->vflag[SPEX_END]) {
        samps_read = 1;
        while(samps_read > 0) {
            if((samps_read = fgetfbufEx(obufptr, dz->wanted, dz->ifd[0],0)) < 0) {
                sprintf(errstr,"Failed to read data from input file at time %lf.\n",wins_read * dz->frametime);
                return(SYSTEM_ERROR);
            }
            wins_read++;
            obufptr += dz->wanted;
            if(obufptr >= obufend) {
                if((exit_status = write_samps(obuf,dz->buflen,dz))<0)
                    return(exit_status);
                obufptr = obuf;
            }
        }
    }

    if(obufptr > obuf) {
        if((exit_status = write_samps(obuf,obufptr - obuf,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/*************************** CREATE_SPECEX_BUFFERS ****************************/
          
int create_specex_buffers(dataptr dz)
{
    long buffersize;
    if((dz->flbufptr = (float **)malloc(sizeof(float *) * dz->bptrcnt))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for float sample buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->buflen = dz->wanted * dz->iparam[SPEX_DUR];                     //  Buffer must contain total duration of area to be stretched
    buffersize = dz->buflen * dz->bptrcnt;
    if((dz->bigfbuf = (float *)malloc(buffersize * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->flbufptr[0] = dz->bigfbuf;
    dz->flbufptr[1] = dz->bigfbuf + dz->buflen;
    return(FINISHED);
}

/*************************** TRANSPART ****************************/
          
int transpart(dataptr dz)
{
    int exit_status;
    int samps_read, wins_read, ccdiv, cctop = 0, ccbot = 0;
    double thistime;
    float *buf = dz->bigfbuf;
    wins_read = 0;
    if((dz->windowbuf = (float **)malloc(sizeof(float *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for spare window array 1.\n");
        return(MEMORY_ERROR);
    }
    if((dz->windowbuf[0] = (float *)malloc(dz->wanted * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for spare window array 2.\n");
        return(MEMORY_ERROR);
    }
    if((samps_read = fgetfbufEx(buf, dz->wanted, dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Failed to read data from input file at time %lf\n",wins_read * dz->frametime);
        return(SYSTEM_ERROR);
    }
    if(dz->mode == 0 || dz->mode == 1 || dz->mode == 4 || dz->mode == 5) {
        if((dz->specenvfrq = (float *)malloc((dz->clength + 1) * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for formant frq array.\n");
            return(MEMORY_ERROR);
        }
        if((dz->specenvpch = (float *)malloc((dz->clength + 1) * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for formant pitch array.\n");
            return(MEMORY_ERROR);
        }
        /*RWD  zero the data */
        memset((char *)dz->specenvpch,0,(dz->clength + 1) * sizeof(float));

        if((dz->specenvamp = (float *)malloc((dz->clength + 1) * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for formant aplitude array.\n");
            return(MEMORY_ERROR);
        }
        if((dz->specenvtop = (float *)malloc((dz->clength + 1) * sizeof(float)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for formant frq limit array.\n");
            return(MEMORY_ERROR);
        }
        if((exit_status = setup_formants(dz))<0) {
            sprintf(errstr,"Failed to establish formant structure\n");
            return(SYSTEM_ERROR);
        }
    }
    while(samps_read > 0) {
        thistime = wins_read * dz->frametime;
        if((exit_status = read_values_from_all_existing_brktables(thistime,dz))<0)
            return exit_status;
        ccdiv = (int) round(dz->param[1]/dz->chwidth);
        if(EVEN(dz->mode)) {    //  Above
            ccbot = ccdiv;
            cctop = dz->clength;
        } else {                //  Below
            ccbot = 0;
            cctop = ccdiv;
        }
        switch(dz->mode) {
        case(0):    //  fall thro           //  Keep Formants
        case(1):    //  fall thro
        case(4):    //  fall thro
        case(5):
            if((exit_status = spec_keep_formants(ccbot,cctop,dz))<0)
                return exit_status;
            break;
        case(2):    //  fall thro           //  Don't Keep Formants
        case(3):    //  fall thro
        case(6):    //  fall thro
        case(7):
            if((exit_status = spec_move_formants(ccbot,cctop,dz))<0)
                return exit_status;
            break;
        }
        if((exit_status = write_samps(buf,samps_read,dz))<0)
            return(exit_status);
        if((samps_read = fgetfbufEx(buf, dz->wanted, dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Failed to read data from input file at time %lf\n",wins_read * dz->frametime);
            return(SYSTEM_ERROR);
        }
        wins_read++;
    }
    return FINISHED;
}

/********************************** SPEC_KEEP_FORMANTS **********************************
 *
 * transpose spectrum, but retain original spectral envelope.
 */

int spec_keep_formants(int ccbot,int cctop,dataptr dz)
{
    int exit_status;
    double pre_totalamp, post_totalamp;
    int cc, vc;
    float *buf = dz->bigfbuf;
    rectify_window(buf,dz);
    if((exit_status = extract_spec_env(0,dz))<0)
        return(exit_status);
    if((exit_status = get_totalamp(&pre_totalamp,buf,dz->wanted))<0)
        return(exit_status);
    switch(dz->mode) {
    case(0):    //  fall thro       //  Transpose
    case(1):
        for(cc = ccbot, vc = cc*2; cc < cctop; cc++, vc += 2) {
            if((exit_status = transpose_within_formant_envelope(vc,dz))<0)
                return(exit_status);
        }
        break;
    case(4):    //  fall thro       //  Shift
    case(5):
        for(cc = ccbot, vc = cc*2; cc < cctop; cc++, vc += 2) {
            if((exit_status = shift_within_formant_envelope(vc,dz))<0)
                return(exit_status);
        }
        break;
    }
    if((exit_status = re_position_partials_in_appropriate_channels(dz))<0)
        return(exit_status);
    if((exit_status = get_totalamp(&post_totalamp,buf,dz->wanted))<0)
        return(exit_status);
    return specnu_normalise(pre_totalamp,post_totalamp,2,dz);
}

/********************************** SPEC_MOVE_FORMANTS **********************************
 *
 * transpose or shift spectrum, (spectral envelope also moves).
 */
int spec_move_formants(int ccbot,int cctop,dataptr dz)
{
    int exit_status;
    double pre_totalamp, post_totalamp;
    int cc, vc;
    float *buf = dz->bigfbuf;
    rectify_window(buf,dz);
    if((exit_status = get_totalamp(&pre_totalamp,buf,dz->wanted))<0)
        return(exit_status);
    switch(dz->mode) {
    case(2):    //  fall thro       //  Transpose
    case(3):
        for(cc = ccbot, vc = cc*2; cc < cctop; cc++, vc += 2)
            buf[FREQ] = (float)(fabs(buf[FREQ]) * dz->param[1]);
        break;
    case(6):    //  fall thro       //  Shift
    case(7):
        for(cc = ccbot, vc = cc*2; cc < cctop; cc++, vc += 2)
            buf[FREQ] = (float)(fabs(buf[FREQ]) + dz->param[1]);
        break;
    }
    if((exit_status = re_position_partials_in_appropriate_channels(dz))<0)
        return(exit_status);
    if((exit_status = get_totalamp(&post_totalamp,buf,dz->wanted))<0)
        return(exit_status);
    return specnu_normalise(pre_totalamp,post_totalamp,2,dz);
}

/************************** TRANSPOSE_WITHIN_FORMANT_ENVELOPE *****************************/

int transpose_within_formant_envelope(int vc,dataptr dz)
{
    int exit_status;
    double thisspecamp, newspecamp, thisamp, formantamp_ratio; 
    float *buf = dz->bigfbuf;
    if((exit_status = getspecenvamp(&thisspecamp,(double)buf[FREQ],0,dz))<0)
        return(exit_status);
    buf[FREQ] = (float)(fabs(buf[FREQ])*dz->param[0]);
    if(buf[FREQ] < dz->nyquist) {
        if(thisspecamp < VERY_TINY_VAL)
            buf[AMPP] = 0.0f;
        else {
            if((exit_status = getspecenvamp(&newspecamp,(double)buf[FREQ],0,dz))<0)
                return(exit_status);
            if(newspecamp < VERY_TINY_VAL)
                buf[AMPP] = 0.0f;
            else {
                formantamp_ratio = newspecamp/thisspecamp;
                if((thisamp = buf[AMPP] * formantamp_ratio) < VERY_TINY_VAL)
                    buf[AMPP] = 0.0f;
                else
                    buf[AMPP] = (float)thisamp;
            }
        }
    }
    return(FINISHED);
}

/************************** SHIFT_TRANSPOSE_WITHIN_FORMANT_ENVELOPE *****************************/

int shift_within_formant_envelope(int vc,dataptr dz)
{
    int exit_status;
    double thisspecamp, newspecamp, thisamp, formantamp_ratio; 
    float *buf = dz->bigfbuf;
    if((exit_status = getspecenvamp(&thisspecamp,(double)buf[FREQ],0,dz))<0)
        return(exit_status);
    buf[FREQ] = (float)(fabs(buf[FREQ])+dz->param[0]);
    if(buf[FREQ] < dz->nyquist) {
        if(thisspecamp < VERY_TINY_VAL)
            buf[AMPP] = 0.0f;
        else {
            if((exit_status = getspecenvamp(&newspecamp,(double)buf[FREQ],0,dz))<0)
                return(exit_status);
            if(newspecamp < VERY_TINY_VAL)
                buf[AMPP] = 0.0f;
            else {
                formantamp_ratio = newspecamp/thisspecamp;
                if((thisamp = buf[AMPP] * formantamp_ratio) < VERY_TINY_VAL)
                    buf[AMPP] = 0.0f;
                else
                    buf[AMPP] = (float)thisamp;
            }
        }
    }
    return(FINISHED);
}

/************************ RE_POSITION_PARTIALS_IN_APPROPRIATE_CHANNELS *************************
 *
 * (1)  At each pass, preset store-buffer channel amps to zero.
 * (2)  Move frq data into appropriate channels, carrying the
 *      amplitude information along with them.
 *      Work down spectrum  for upward transposition, and
 * (3)  up spectrum for downward transposition,
 *      so that we do not overwrite transposed data before we move it.
 * (4)  Put new frqs back into src buff.
 */

int re_position_partials_in_appropriate_channels(dataptr dz)
{
    int exit_status;
    int truecc,truevc;
    int cc, vc;
    float *buf = dz->bigfbuf;
    for(vc = 0; vc < dz->wanted; vc+=2)                     /* 1 */
        dz->windowbuf[0][vc] = 0.0f;
    if(dz->param[0] > 1.0f) {                               /* 2 */
        for(cc=dz->clength-1,vc = dz->wanted-2; cc>=0; cc--, vc-=2) {
            if(buf[FREQ] < dz->nyquist && buf[AMPP] > 0.0f) {
                if((exit_status = get_channel_corresponding_to_frq(&truecc,(double)buf[FREQ],dz))<0)
                    return(exit_status);
                truevc = truecc * 2;
                if((exit_status = move_data_into_appropriate_chan(vc,truevc,buf[AMPP],buf[FREQ],dz))<0)
                    return(exit_status);
            }
        }
        for(vc = 0; vc < dz->wanted; vc++)
            buf[vc] = dz->windowbuf[0][vc]; 
    } else if(dz->param[0] < 1.0f){     /* 3 */
        for(cc=0,vc = 0; cc < dz->clength; cc++, vc+=2) {
            if(buf[FREQ] < dz->nyquist && buf[FREQ]>0.0) {
                if((exit_status = get_channel_corresponding_to_frq(&truecc,(double)buf[FREQ],dz))<0)
                    return(exit_status);
                truevc = truecc * 2;
                if((exit_status = move_data_into_appropriate_chan(vc,truevc,buf[AMPP],buf[FREQ],dz))<0)
                    return(exit_status);
            }
        }
        for(vc = 0; vc < dz->wanted; vc++)
            buf[vc] = dz->windowbuf[0][vc];     /* 4 */
    }
    return(FINISHED);
}

/******************* SETUP_FORMANTS *****************/

int setup_formants(dataptr dz)
{
    int exit_status;
    int arraycnt;
    aplptr ap = dz->application;
    if((exit_status = establish_formant_band_ranges(dz->infile->channels,ap))<0)
        return(exit_status);

    dz->specenv_type = PICHWISE_FORMANTS;           
    dz->formant_bands = 4;
    if((exit_status = initialise_specenv(&arraycnt,dz))<0)
        return(exit_status);
    if((exit_status = set_spec_env_frqs(arraycnt,dz))<0)
        return(exit_status);
    dz->descriptor_samps = dz->infile->specenvcnt * DESCRIPTOR_DATA_BLOKS ;
    return(FINISHED);
}

/************************ SET_SPECENV_FRQS ************************
 *
 * FREQWISE BANDS = number of channels for each specenv point
 * PICHWISE BANDS  = number of points per octave
 */

int set_spec_env_frqs(int arraycnt,dataptr dz)
{
    int exit_status;
    double bandbot;
    double *interval;
    int m, n, k = 0, cc;
    if((exit_status = setup_octave_band_steps(&interval,dz))<0)
        return(exit_status);
    if((exit_status = setup_lo_octave_bands(arraycnt,dz))<0)
        return(exit_status);
    k  = TOP_OF_LOW_OCTAVE_BANDS;
    cc = CHAN_ABOVE_LOW_OCTAVES;
    while(cc <= dz->clength) {
        m = 0;
        if((bandbot = dz->chwidth * (double)cc) >= dz->nyquist)
            break;
        for(n=0;n<dz->formant_bands;n++) {
            if(k >= arraycnt) {
                sprintf(errstr,"Formant array too small: set_spec_env_frqs()\n");
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
    dz->infile->specenvcnt = k;
    return(FINISHED);
}

/************************* SETUP_OCTAVEBAND_STEPS ************************/

int setup_octave_band_steps(double **interval,dataptr dz)
{
    double octave_step;
    int n = 1, m = 0, halfbands_per_octave = dz->formant_bands * 2;
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

/************************ SETUP_LOW_OCTAVE_BANDS ***********************
 *
 * Lowest PVOC channels span larger freq steps and therefore we must
 * group them in octave bands, rather than in anything smaller.
 */

int setup_lo_octave_bands(int arraycnt,dataptr dz)
{
    int n;
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
            sprintf(errstr,"Insufficient low octave band setups in setup_lo_octave_bands()\n");
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/********************** EXTRACT_SPEC_ENV *******************/

#define CHAN_SRCHRANGE_F    (4)

int extract_spec_env(int storeno,dataptr dz)
{
    int    n, cc, vc, specenvcnt_less_one;
    int    botchan, topchan;
    double botfreq, topfreq;
    double bwidth_in_chans;
    float *ampstore, *buf = dz->bigfbuf;
    switch(storeno) {
    case(0):    ampstore = dz->specenvamp;  break;
    case(1):    ampstore = dz->specenvamp2; break;
    default:
        sprintf(errstr,"Unknown storenumber in extract_spec_env()\n");
        return(PROGRAM_ERROR);
    }
    specenvcnt_less_one = dz->infile->specenvcnt - 1;
    vc = 0;
    for(n=0;n<dz->infile->specenvcnt;n++)
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
            if(buf[FREQ] >= botfreq && buf[FREQ] < topfreq)
                ampstore[n] = (float)(ampstore[n] + buf[AMPP]);
        }
        bwidth_in_chans   = (double)(topfreq - botfreq)/dz->chwidth;
        ampstore[n] = (float)(ampstore[n]/bwidth_in_chans);
        n++;
    }
    return(FINISHED);
}

/******************* MOVE_DATA_INTO_APPROPRIATE_CHANNEL *****************
 *
 *  If there turns out to be more than one partial to a channel,
 *  keep partial that was loudest in orig sound, retaining its
 *  loudness for further comparison.
 */

int move_data_into_appropriate_chan(int vc,int truevc,float thisamp,float thisfrq,dataptr dz)   
{
    float *buf = dz->bigfbuf;
    if(buf[AMPP] > dz->windowbuf[0][truevc]) {
        dz->windowbuf[0][truevc++] = thisamp;
        dz->windowbuf[0][truevc]   = thisfrq;
    }
    return(FINISHED);
}

/**************************** SPECNU_NORMALISE ***************************/

int specnu_normalise(double pre_totalamp,double post_totalamp,int paramno,dataptr dz)
{
    double normaliser;
    int vc;
    float *buf = dz->bigfbuf;

    if(post_totalamp < VERY_TINY_VAL) {
        if(!dz->zeroset) {
            fprintf(stdout,"WARNING: Zero-amp spectral window(s) encountered: orig window(s) substituted.\n");    
            fflush(stdout);
            dz->zeroset = TRUE;
        }
    } else {
        normaliser = pre_totalamp/post_totalamp;    /* REMOVED a multiplier of 0.5 : APRIL 1998 */
        for(vc = 0; vc < dz->wanted; vc += 2)
            buf[AMPP] = (float)(buf[AMPP] * normaliser * dz->param[paramno]);
    }
    return(FINISHED);
}

/*************************** NUINVERT ****************************/
          
int nuinvert(dataptr dz)
{
    int exit_status;
    int samps_read, wins_read, cctop, ccpk,ccbot;
    double thistime;
    float *buf = dz->bigfbuf;
    ccbot = (int)round(dz->param[INV_BOT]/dz->chwidth);     //  Lowest channel in inversion
    cctop = (int)round(dz->param[INV_TOP]/dz->chwidth);     //  Highest channel in inversion
    ccpk  = (int)round(dz->param[INV_PKTOP]/dz->chwidth);   //  Range of entire spectrum in which to search for peak
    wins_read  = 0;
    if((samps_read = fgetfbufEx(buf, dz->wanted, dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Failed to read data from input file at time %lf\n",wins_read * dz->frametime);
        return(SYSTEM_ERROR);
    }
    while(samps_read > 0) {
        thistime = wins_read * dz->frametime;
        if((exit_status = invert_spectrum(ccbot,cctop,ccpk,wins_read,dz))<0)
            return(exit_status);
        if((exit_status = write_samps(buf,samps_read,dz))<0)
            return(exit_status);
        if((samps_read = fgetfbufEx(buf, dz->wanted, dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Failed to read data from input file at time %lf\n",wins_read * dz->frametime);
            return(SYSTEM_ERROR);
        }
        wins_read++;
    }
    return FINISHED;
}

/********************************** INVERT_SPECTRUM **********************************
 *
 * Invert spectrum, but retain original spectral envelope.
 */

int invert_spectrum(int ccbot,int cctop,int ccpk,int wins_read,dataptr dz)
{
    int exit_status, botmaxloc, topmaxloc;
    float maxamplo = 0.0, maxamphi = 0.0, maxamp = 0.0, minamp = 0.0, amprange;
    float *buf = dz->bigfbuf;
    rectify_window(buf,dz);
    if((exit_status = find_maxamp_channel(ccbot,cctop,&maxamp,dz))<0)   //  maximum level in inverted section
        return FINISHED;
    if(maxamp < FLTERR)     //  No spectral data to invert
        return FINISHED;
    if((exit_status = find_minamp_channel(ccbot,cctop,&minamp,dz))<0)   //  minimum level in inverted section
        return FINISHED;
    amprange = maxamp - minamp;
    if((botmaxloc = find_maxamp_channel(0,ccpk,&maxamplo,dz))<0)        //  maximum level in bottom part of entire spectrum
        return FINISHED;    //  Data too low level to invert
    topmaxloc = cctop - 1;
    maxamphi  = buf[topmaxloc * 2];
    if((exit_status = create_contours(maxamplo,maxamphi,botmaxloc,topmaxloc,cctop,dz))<0)
        return(exit_status);
    if((exit_status = do_spectral_invert(ccbot,cctop,amprange,minamp,dz))<0)
        return(exit_status);
    return FINISHED;
}

/********************************** FIND_MINAMP_CHANNEL ***********************************/

int find_minamp_channel(int cclo,int cchi,float *minamp,dataptr dz)
{
    int cc, vc, maxloc;
    float *buf = dz->bigfbuf;
    *minamp = MAXFLOAT;
    maxloc = -1;
    for(cc = cclo, vc = cc*2; cc < cchi; cc++, vc += 2) {
        if(buf[AMPP] < *minamp) {
            *minamp = buf[AMPP];
            maxloc = cc;
        }
    }
    return maxloc;
}

/********************************** CREATE_CONTOURS **********************************
 *
 *        pk    top of   top of 
 *        lo  inversion spectrum
 *    -x--o     |           |
 *     | A  \ B |-----------|
 *     |      \ |           |
 * bottom of    o           |
 * inversion    pk          |
 *     |        hi          |
 *              |           |
 *      CONTOUR |Unaffected |
 */

int create_contours(float maxamplo,float maxamphi,int botmaxloc,int topmaxloc,int cctop,dataptr dz)
{
    int cc, vc, n, locdiff;
    float *buf2 = dz->flbufptr[2];
    double ratio, ampdiff, ampstep;

    ampdiff = maxamphi - maxamplo;
    locdiff = topmaxloc - botmaxloc;
    memset((char *)buf2,0,dz->wanted * sizeof(float));
    for(cc = 0, vc = 0; cc < botmaxloc; cc++, vc +=2)                           //  A
        buf2[AMPP] = maxamplo;

    for(cc = botmaxloc, vc = cc*2,n = 0; cc <= topmaxloc; cc++,vc +=2,n++) {    //  B
        ratio   = (double)n/(double)locdiff;
        ampstep = ampdiff * ratio;
        buf2[AMPP] = (float)(maxamplo + ampstep);
    }
    return FINISHED;
}

/**************************************** DO_SPECTRAL_INVERT *********************
 *
 *  (1) Find level in channels RELATIVE TO the contour
 *      chanamp/contouramp
 *  (2) Invert the order of the RELATIVE-AMPLITUDES
 *  (3) Establish now TRUE amplitude
 *      by multiplying relative-amplitude by inverted contour.
 */

int do_spectral_invert(int ccbot,int cctop,float amprange,float minamp,dataptr dz)
{
    int exit_status, cc, vc, vc2;
    int vctop = (cctop-1)*2;                                //  Last active amplitude channel in area being inverted
    int halfway = (ccbot + cctop)/2;
    float temp, numaxamp = 0.0, numinamp = 0.0;
    double ampratio;
    float *buf = dz->bigfbuf, *buf2 = dz->flbufptr[2];

    for(cc = ccbot, vc = cc*2; cc < cctop; cc++, vc +=2)    //  Find chanamp relative to contour
        buf[AMPP]  = (float)(buf[AMPP]/buf2[AMPP]);         //  chanamp/contouramp (contouramp vals cannot be zero)

    for(cc = ccbot, vc = cc*2,vc2 = vctop; cc < halfway; cc++, vc +=2, vc2 -=2) {
        temp     = buf[vc];                                 //  Invert order of amplitude-ratios
        buf[vc]  = buf[vc2];
        buf[vc2] = temp;
    }
    for(cc = ccbot, vc = cc*2; cc < cctop; cc++, vc +=2)    //  Find true amp from relative amp, now relative to contour
        buf[AMPP]  = (float)(buf[AMPP]*buf2[AMPP]);         //  relative-contouramp * contour
    if((exit_status = find_maxamp_channel(ccbot,cctop,&numaxamp,dz))<0) {
        sprintf(errstr,"Failed to find maximum amplitude after inversion\n");
        return PROGRAM_ERROR;
    }
    if((exit_status = find_minamp_channel(ccbot,cctop,&numinamp,dz))<0) {
        sprintf(errstr,"Failed to find minimum amplitude after inversion\n");
        return PROGRAM_ERROR;
    }
    ampratio = amprange/(numaxamp - numinamp);
    for(cc = ccbot, vc = cc*2; cc < cctop; cc++, vc +=2)    //  Scale output within range of input
        buf[AMPP] = (float)((((buf[AMPP] - numinamp) * ampratio) + minamp) * dz->param[INV_GAIN]);
    return FINISHED;
}

/********************************** FIND_MAXAMP_CHANNEL ***********************************/

int find_maxamp_channel(int cclo,int cchi,float *maxamp,dataptr dz)
{
    int cc, vc, maxloc;
    float *buf = dz->bigfbuf;
    *maxamp = 0.0;
    maxloc = -1;
    for(cc = cclo, vc = cc*2; cc < cchi; cc++, vc += 2) {
        if(buf[AMPP] > *maxamp) {
            *maxamp = buf[AMPP];
            maxloc = cc;
        }
    }
    return maxloc;
}

/**************************** ALLOCATE_QUAD_BUFFER ******************************/

int allocate_quad_buffer(dataptr dz)
{
    unsigned int buffersize;
    if(dz->bptrcnt < 6) {
        sprintf(errstr,"Insufficient bufptrs established in allocate_triple_buffer()\n");
        return(PROGRAM_ERROR);
    }
//TW REVISED: buffers don't need to be multiples of secsize
    buffersize = dz->wanted;
    dz->buflen = buffersize;
    if((dz->bigfbuf = (float*)malloc(dz->buflen*4 * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize = dz->buflen;
    dz->flbufptr[2]  = dz->bigfbuf + dz->big_fsize;   
    dz->flbufptr[3]  = dz->flbufptr[2] + dz->big_fsize;
    dz->flbufptr[4]  = dz->flbufptr[3] + dz->big_fsize;
    dz->flbufptr[5]  = dz->flbufptr[4] + dz->big_fsize;
    return(FINISHED);
}

/*************************** CONVOLVE ****************************/
          
int convolve(dataptr dz)
{
    int exit_status, passno = 1;
    int samps_read, samps_read2;
    double maxin = 0.0, maxout = 0.0, ubermaxin = -HUGE, ubermaxout = -HUGE;
    float *buf = dz->bigfbuf, *buflast = NULL, *buf2 = NULL, *buf2last = NULL;
    int wins_read;
    double ratio, ratiosum;
    while(passno <= 2) {
        wins_read  = 0;
        ratiosum = 0.0;
        dz->total_samps_read = 0;
        if(passno == 1)
            fprintf(stdout,"First pass : Assessing output level.\n");
        else {
            if((sndseekEx(dz->ifd[0],0,0)<0)){        
                sprintf(errstr,"sndseek() failed to Seek to start of file-1 for 2nd pass\n");
                return SYSTEM_ERROR;
            }                                                                   //  2nd pass : Seek to start of files
            if(dz->mode == 1) {
                if((sndseekEx(dz->ifd[1],0,0)<0)){        
                    sprintf(errstr,"sndseek() failed to Seek to start of file-2 for 2nd pass\n");
                    return SYSTEM_ERROR;
                }
            }
            maxin  = ubermaxin;                                         //  2nd pass : set maxin,out sent to OVERALL max vals   
            maxout = ubermaxout;                                        //  And send to proceeses, to do normalisation
            fprintf(stdout,"Second pass : Scaling data and creating output.\n");
        }
        fflush(stdout);
        if((samps_read = fgetfbufEx(buf, dz->wanted, dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Failed to read data from input file at time %lf\n",wins_read * dz->frametime);
            return(SYSTEM_ERROR);
        }
        if(dz->mode == 1) {
            if(passno == 1) {   
                buflast  = dz->flbufptr[2];
                buf2     = dz->flbufptr[3];
                buf2last = dz->flbufptr[4];
                memset((char *)buflast, 0,dz->wanted * sizeof(float));  //  Initialise window memory buffers to zero
                memset((char *)buf2last,0,dz->wanted * sizeof(float));
                ratio = (double)dz->insams[0]/(double)dz->insams[1];
            }
            if((samps_read2 = fgetfbufEx(buf, dz->wanted, dz->ifd[0],0)) < 0) {
                sprintf(errstr,"Failed to read data from input file at time %lf\n",wins_read * dz->frametime);
                return(SYSTEM_ERROR);
            }
        }
        while(samps_read > 0) {
            switch(dz->mode) {
            case(0):
                if((exit_status = self_convolve(buf,&wins_read,&ratio,&ratiosum,&maxin,&maxout,passno,dz))<0)
                    return(exit_status);
                break;
            case(1):
                if((exit_status = do_convolve(buf,buflast,buf2,buf2last,&wins_read,&ratio,&ratiosum,&maxin,&maxout,passno,dz))<0)
                    return(exit_status);
                break;
            }
            if(passno == 1) {                                           //  Calculate max window levels, on input & output, over entire sound
                ubermaxin  = max(maxin, ubermaxin);
                ubermaxout = max(maxout,ubermaxout);
            } else {                                                    //  Write samples only on 2nd pass
                if((exit_status = write_samps(buf,samps_read,dz))<0)
                    return(exit_status);
            }
            if((samps_read = fgetfbufEx(buf, dz->wanted, dz->ifd[0],0)) < 0) {
                sprintf(errstr,"Failed to read data from input file at time %lf\n",wins_read * dz->frametime);
                return(SYSTEM_ERROR);
            }
            wins_read++;
        }
        passno++;
    } 
    return FINISHED;
}

/**************************** SELF_CONVOLVE ******************************/

int self_convolve(float *buf,int *wins_read,double *ratio,double *ratiosum,double *maxin,double *maxout,int passno,dataptr dz)
{
    int exit_status, n, cc, vc;
    double val;
    rectify_window(buf,dz);
    if(passno == 1) {
        *maxin  = -HUGE;                            //  In PASS 1, record max window amplitude, on input and output,
        *maxout = -HUGE;                            //  to return to calling routine.
        for(cc=0, vc = 0; cc < dz->clength; cc++,vc+=2) {
            val = buf[AMPP];
            *maxin = max(*maxin,buf[AMPP]);
            for(n=0;n<dz->iparam[1];n++)            //  Multiply the amplitude by itself (param[1] times)
                val *= buf[AMPP];
            buf[AMPP] = (float)val;
            *maxout = max(*maxout,buf[AMPP]);
        }
    } else {
        for(cc=0, vc = 0; cc < dz->clength; cc++,vc+=2) {
            val = buf[AMPP];
            for(n=0;n<dz->iparam[1];n++)
                val *= buf[AMPP];
            buf[AMPP] = (float)val;
        }
        if((exit_status = specnu_normalise(*maxin,*maxout,0,dz))<0)
            return exit_status;
    }
    return FINISHED;
}

/**************************** DO_CONVOLVE ******************************/

int do_convolve(float *buf,float *buflast,float *buf2,float *buf2last,int *wins_read,double *ratio,double *ratiosum,
                double *maxin,double *maxout,int passno,dataptr dz)
{
    int exit_status, n, samps_read, cc ,vc;
    double aval, fval, fbot, ftop;
    double fbufdif1, fbufdif2, fdifsum;
    rectify_window(buf,dz);
    if(dz->vflag[0]) {
        if((samps_read = fgetfbufEx(buf2, dz->wanted, dz->ifd[1],0)) < 0) {
            sprintf(errstr,"Failed to read data from 2nd input file at time %lf\n",*wins_read * dz->frametime);
            return(SYSTEM_ERROR);
        }
        rectify_window(buf2,dz);
        if(samps_read == 0) {
            memset((char *)buf2,0,dz->wanted * sizeof(float));
        }
    } else {
        if(*ratiosum >= *wins_read) {       //  Only read 2nd file when 1st file has advanced to requisite no of windows
            if((samps_read = fgetfbufEx(buf2, dz->wanted, dz->ifd[1],0)) < 0) {
                sprintf(errstr,"Failed to read data from 2nd input file at time %lf\n",*wins_read * dz->frametime);
                return(SYSTEM_ERROR);
            }
            rectify_window(buf2,dz);
            *ratiosum += *ratio;            //  Find next 1st-file-input-window-number at which we'll next read 2nd file
        }
    }
    fbot = -dz->halfchwidth;
    ftop = dz->halfchwidth;
    if(passno == 1) {
        *maxin  = -HUGE;                                //  In PASS 1, record max window amplitude, on input and output, 
        *maxout = -HUGE;                                //  to return to calling routine.
        for(cc=0, vc = 0; cc < dz->clength; cc++,vc+=2) {
            aval = buf[AMPP];
            *maxin = max(*maxin,buf[AMPP]);
            fval  = buflast[FREQ];                      //  We will advance frq in PREVIOUS input window, but differently
            fbufdif1 = buf[FREQ]  - buflast[FREQ];      //  frq advance of 1st file to reach this channel
            fbufdif2 = buf2[FREQ] - buf2last[FREQ];     //  frq advance of 2nd file to reach this channel
            fdifsum  = fbufdif1 + fbufdif2;             //  Sum these (akin to summing phase)
            for(n=0;n<dz->iparam[1];n++) {
                aval *= buf2[AMPP];                     //  multiply the amps
                fval += fdifsum;                        //  advance previous channels freq by the new ammount ("sum phase")
                fdifsum += fbufdif2;                    //  In case process is to be repeated,
            }                                           //  advance by 2nd file's increment, again,

            fval = chanadjust(fval,fbot,ftop,dz);       //  Force output frqs into their original channels
            fbot += dz->chwidth;                        //  advance frq-limits to those of next channel
            ftop += dz->chwidth;
            buf[AMPP] = (float)aval;                    //  output new values
            buf[FREQ] = (float)fval;
            buflast[FREQ]  = buf[FREQ];                 //  Store cuurent values for next window calculations
            buf2last[FREQ] = buf2[FREQ];
            *maxout = max(*maxout,buf[AMPP]);
        }
    } else {
        for(cc=0, vc = 0; cc < dz->clength; cc++,vc+=2) {
            aval = buf[AMPP];                           //  In PASS 2, use window maximum IN ENTIRE FILE, on input & output, 
            fval  = buflast[FREQ];                      //  Passed back into this process by calling routine        ,
            fbufdif1 = buf[FREQ]  - buflast[FREQ];      //  To scale the output.
            fbufdif2 = buf2[FREQ] - buf2last[FREQ];
            fdifsum  = fbufdif1 + fbufdif2;
            for(n=0;n<dz->iparam[1];n++) {
                aval *= buf2[AMPP];
                fval += fdifsum;
                fdifsum += fbufdif2;
            }
            fval = chanadjust(fval,fbot,ftop,dz);
            fbot += dz->chwidth;
            ftop += dz->chwidth;
            buf[AMPP] = (float)aval;
            buf[FREQ] = (float)fval;
            buflast[FREQ]  = buf[FREQ];
            buf2last[FREQ] = buf2[FREQ];
        }
        if((exit_status = specnu_normalise(*maxin,*maxout,0,dz))<0)
            return exit_status;
    }
    return FINISHED;
}

/************************************ CHANADJUST *********************************
 *
 *  If frq outside its allotted channel, refelct off top and bottom till it's within channel,
 *
 *                                  IN NEXT CHAN ABOVE          EVEN NO OF CHANS ABOVE
 *                                     True     0                 True      0       1       2
 *  Frq ABOVE the channel limits    |       |->x    |           |       |-------|-------|->x    |
 *  How many channels above?            
 *  If 0 (next channel) just invert |    x<-|       |           |<------|
 *  OR if EVEN number of chans                                  |------>|
 *  bouce off top                                               |    x<-|
 *                                  
 *                                                              ODD NO OF CHANS ABOVE
 *                                                                 True     0       1
 *  If ODD number of chans,                                     |       |-------|->x    |
 *  bounce off bottom.
 *                                                              |<------|   
 *                                                              |->x    |
 *
 *                                  IN NEXT CHAN BELOW          EVEN NO OF CHANS BELOW
 *                                      0      True                 2       1       0      True
 *  Frq BELOW the channel limits    |    x<-|       |           |    x<-|-------|-------|       |
 *  How many channels below?            
 *  If 0 (next channel) just invert |       |->x    |           |------>|
 *  OR if EVEN number of chans                                  |<------|
 *  bouce off top                                               |->x    |
 *                                  
 *                                                              ODD NO OF CHANS BELOW
 *                                                                  1       0      True
 *  If ODD number of chans,                                     |    x<-|-------|       |
 *  bounce off top.
 *                                                                              |------>|   
 *                                                                              |    <-x|
 */

double chanadjust(double frq,double fbot,double ftop,dataptr dz)
{
    double ovflow, val;
    int chanxs;
    if((ovflow = frq - ftop) > 0.0) {           //  Frq above the channel limits
        chanxs = (int)floor(ovflow/dz->chwidth);//  How many channels above?
        if(EVEN(chanxs))                        //  If EVEN just bounce off top
            val = ftop - ovflow;                //
        else                                    //  Else bounce of bottom
            val = fbot + ovflow;                //
    } else if((ovflow = frq - fbot) < 0.0) {    //  Frq below channel limits
        ovflow = -ovflow;
        chanxs = (int)floor(ovflow/dz->chwidth);//  How many channels below?
        if(EVEN(chanxs))
            val = fbot + ovflow;                //  If EVEN just bounce off bottom
        else
            val = ftop - ovflow;                //  Else bounce of top
    } else
        val = frq;
    if(val < 0.0)                               //  check still within total range
        val = -val;
    if(val > dz->nyquist)
        val = dz->nyquist - val;
    return val;
}

/*************************************** SPECSND **************************/

int specsnd(dataptr dz)
{
    int exit_status;
    float *buf = dz->bigfbuf, *obuf = dz->flbufptr[2];
    int obufcnt = 0, positive = 1;
    long samps_read, wins_read = 0;
    double transrange;
    dz->param[0] = pow(2.0,dz->iparam[0]);      //  Convert oct tranpos to transpos ratios
    dz->param[1] = pow(2.0,dz->iparam[1]);
    transrange = dz->param[1] - dz->param[0];
    if((samps_read = fgetfbufEx(buf, dz->wanted, dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Failed to read data from input file at time %lf\n",wins_read * dz->frametime);
        return(SYSTEM_ERROR);
    }
    while(samps_read > 0) {
        if((exit_status = do_specsnd(&obufcnt,positive,dz->param[0],transrange,dz))<0)
            return(exit_status);
        positive = -positive;
        if((exit_status = write_samps(obuf,obufcnt,dz))<0)
            return(exit_status);
        if((samps_read = fgetfbufEx(buf, dz->wanted, dz->ifd[0],0)) < 0) {
            sprintf(errstr,"Failed to read data from input file at time %lf\n",wins_read * dz->frametime);
            return(SYSTEM_ERROR);
        }
        wins_read++;
    }
    return FINISHED;
}

/***************************************** DO_SPECSND ******************************** 
 *
 *  Find MEan of spectrum           |  /\
 *                                  |_/__\/\________
 *                                  |/      \/\
 *                                  |__________\__
 *
 *                                  
 *                                  |
 *  Subtract meanspectrum           |  /\
 *                                  |_/__\/\________
 *                                  |/      \/\
 *                                  |          \                
 *
 *  Contract to duration of input sound, taking into account decimation
 *  Write as sound.
 */

int do_specsnd(int *obufcnt,int positive,double transbot,double transrange,dataptr dz)
{
    float *buf = dz->bigfbuf, *obuf = dz->flbufptr[2];
    int cc, vc, j, k = 0;
    double sum = 0.0, maxsamp = -HUGE, pratio, pratio_step;
    rectify_window(buf,dz);
    k = 0;
    for(cc=0, vc = 0; cc < dz->clength; cc++,vc+=2)
        maxsamp = max(maxsamp,buf[AMPP]);
    pratio = (transrange * maxsamp) + transbot;
    pratio_step = pratio;
    *obufcnt = 0;
    k = 0;
    j = 0;
    for(cc=0, vc = 0; cc < dz->clength; cc++,vc+=2) {
        sum += buf[AMPP];
        j++;
        if(++k >= pratio_step) {
            sum /= pratio;
            sum = min(sum,0.95);
            obuf[*obufcnt] = (float)(sum * positive);
            (*obufcnt)++;
            pratio_step += pratio;
            sum = 0.0;
            j = 0;
        }
    }
    if(j > 0) {
        sum /= pratio;
        obuf[*obufcnt] = (float)(sum * positive);
        (*obufcnt)++;
    }
    return FINISHED;
}

/**************************** CREATE_SPECFRAC_BUFFERS ******************************/

int create_specfrac_buffers(dataptr dz)
{
    unsigned int buffersize;
    if(dz->bptrcnt < 3) {
        sprintf(errstr,"Insufficient bufptrs established in allocate_triple_buffer()\n");
        return(PROGRAM_ERROR);
    }
    buffersize = dz->insams[0] * 3;
    dz->buflen = dz->insams[0];
    if((dz->bigfbuf = (float*)malloc(buffersize * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for sound buffers.\n");
        return(MEMORY_ERROR);
    }
    dz->big_fsize = dz->buflen;
    dz->flbufptr[0] = dz->bigfbuf;
    dz->flbufptr[1] = dz->bigfbuf + dz->buflen;   
    dz->flbufptr[2] = dz->flbufptr[2] + (dz->buflen * 2);       //  2nd buffer is double size of first
    return(FINISHED);
}

/**************************** CHECK_FRACTAL_PARAM_VALIDITY_AND_CONSISTENCY *****************************/

int check_fractal_param_validity_and_consistency(dataptr dz)
{
    int minfracsize, maxsegcnt, maxcutcnt; 
    minfracsize = 1;
    maxsegcnt = dz->wlength - 1;        //  Number of segments obtained by cutting file into single windows
    maxcutcnt = 1;
    dz->maxfraccnt = 2;                 //  Count how many successive cuts-in-half are needed
    while(dz->maxfraccnt < maxsegcnt) { //  To generate NO MORE THAN maxfraccnt segments.
        dz->maxfraccnt *= 2;
        maxcutcnt++;                    //  calculate the maximum number of divisions by 2
    }                                   //  which will fit INSIDE the maxsegcnt
    if(dz->mode < 6)
        dz->maxfraccnt /= 2;
    dz->copycnt = dz->maxfraccnt;       //  TOTAL number of windows to be copied in each processing call
    dz->itemcnt = dz->maxfraccnt + 1;   //  Total number of windows to eventually be written out (includes window 0)
                                        //  AND HIGHEST window NUMBER in write-buf to be written to (includes window 0)
    maxcutcnt--;
    switch(dz->mode) {
    case(6):    //fall thro
    case(7):    //fall thro
    case(8):
        maxcutcnt--;
        break;
    }
    if(dz->iparam[FRACDEPTH] > maxcutcnt) {
        sprintf(errstr,"INFO: Maximum number of fractal cuts for this file = %d\n",maxcutcnt);
        return DATA_ERROR;
    }
    return FINISHED;
}

/**************************** SPECFRAC *****************************/

int specfrac(dataptr dz)
{
    int exit_status;
    int samps_read;
    if(!dz->vflag[0]) {
        dz->process_type = BIG_ANALFILE;
        dz->outfiletype  = ANALFILE_OUT;
        if((exit_status = create_sized_outfile(dz->outfilename,dz))<0)
            return(exit_status);
    }
    if((samps_read = fgetfbufEx(dz->flbufptr[0], dz->insams[0], dz->ifd[0],0)) < 0) {
        sprintf(errstr,"Failed to read data from input file\n");
        return(SYSTEM_ERROR);
    }
    memset((char *)dz->flbufptr[1],0,dz->insams[0] * sizeof(float));
    switch(dz->mode) {
    case(0):                //  Fractal copy and shrink
        if((exit_status = fractal_copy_and_shrink(dz))<0)
            return exit_status;
        break;
    case(1):                    //  Fractal reverse-copy and shrink by interpolation
        if((exit_status = fractal_reverse_copy_and_shrink(dz))<0)
            return exit_status;
        break;
    case(2):    //fall thro     //  Fractal reverse-copy N segments, and shrink by interpolation
    case(3):                    //  Fractal reverse-copy N segments, and shrink by addition
        if((exit_status = fractal_copy_reverse_shorten(dz))<0)
            return exit_status;
        break;
    case(4):    //fall thro     //  Swap two halves of segment
    case(5):                    //  Reverse second half of segment
        if((exit_status = fractal_by_halves(dz))<0)
            return exit_status;
        break;
    case(6):    //fall thro     //  Interleave two halves of segment
    case(7):    //fall thro     //  Reverse second half of segment, then interleave
    case(8):                    //  Reverse second half of segment, retaining original speed
        if((exit_status = fractal_by_interleaving(dz))<0)
            return exit_status;
        break;
    }                           //  Ensure ALL sound converted to wav by PVOC synth
    return FINISHED;
}

/**************************** FRACTAL_COPY_AND_SHRINK *****************************
 *
 *  |--->--->--->--->|  to  |--->--->--->--->|--->--->--->--->| to  |->->->->->->->->|   
 *
 *  |->->->->->->->->|  to  |->->->->->->->->|->->->->->->->->| to  |>>>>>>>>>>>>>>>>|
 */

//  MODE 0
int fractal_copy_and_shrink(dataptr dz)
{
    int exit_status;
    int n = 0, buf2pos;
    char outfilename[2000];
    float *buf = dz->flbufptr[0];
    for(n = 0;n < dz->iparam[FRACDEPTH]; n++) {                 //  For every cut-stage : loop-n
        if(dz->vflag[0]) {
            dz->process_type = BIG_ANALFILE;
            dz->outfiletype  = ANALFILE_OUT;
            if((exit_status = create_new_outfile(n,outfilename,dz))<0)
                return exit_status;
        }
        buf2pos = 0;                                            //  Copy from AFTER window 0 to itemcnt
        if((exit_status = frac_copy(&buf2pos,1,dz->itemcnt,dz))<0)
            return exit_status;
        if((exit_status = frac_copy(&buf2pos,1,dz->itemcnt,dz))<0)
            return exit_status;
        if((exit_status = frac_interp_shrink(dz)) < 0)
            return exit_status;
        if(dz->vflag[0]) {
            if((exit_status = write_samps(buf,dz->itemcnt * dz->wanted,dz))<0)
                return(exit_status);
            if((exit_status = headwrite(dz->ofd,dz))<0) {
                free(outfilename);
                return(exit_status);
            }
            if(sndcloseEx(dz->ofd) < 0) {
                fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
                fflush(stdout);
            }
            dz->ofd = -1;
            dz->process_type = OTHER_PROCESS;       /* restore true status */
            dz->outfiletype  = NO_OUTPUTFILE;       /* restore true status */
        }
    }                                                   //  Include window zero in output-write
    if(!dz->vflag[0]) {
        if((exit_status = write_samps(buf,dz->itemcnt * dz->wanted,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/**************************** FRACTAL_REVERSE_COPY_AND_SHRINK *****************************
 *
 *  |--->--->--->--->|  to  |--->--->--->--->|<---<---<---<---| to  |->->->--<-<-<-<-|   
 *
 *  |->->->-><-<-<-<-|  to  |->->->-><-<-<-<-|<-<-<-<-->->->->| to  |<<<<<<<<>>>>>>>>|
 */

int fractal_reverse_copy_and_shrink(dataptr dz)
{
    int exit_status;
    int n = 0, buf2pos;
    char outfilename[2000];
    float *buf = dz->flbufptr[0];
    for(n = 0;n < dz->iparam[FRACDEPTH]; n++) {                 //  For every cut-stage : loop-n
        if(dz->vflag[0]) {
            dz->process_type = BIG_ANALFILE;
            dz->outfiletype  = ANALFILE_OUT;
            if((exit_status = create_new_outfile(n,outfilename,dz))<0)
                return exit_status;
        }
        buf2pos = 0;                                            //  Copy from AFTER window 0 to itemcnt
        if((exit_status = frac_copy(&buf2pos,1,dz->itemcnt,dz))<0)
            return exit_status;
        if((exit_status = frac_reverse_copy(&buf2pos,1,dz->itemcnt,dz))<0)
            return exit_status;
        if((exit_status = frac_interp_shrink(dz)) < 0)
            return exit_status;
        if(dz->vflag[0]) {
            if((exit_status = write_samps(buf,dz->itemcnt * dz->wanted,dz))<0)
                return(exit_status);
            if((exit_status = headwrite(dz->ofd,dz))<0) {
                free(outfilename);
                return(exit_status);
            }
            if(sndcloseEx(dz->ofd) < 0) {
                fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
                fflush(stdout);
            }
            dz->ofd = -1;
            dz->process_type = OTHER_PROCESS;       /* restore true status */
            dz->outfiletype  = NO_OUTPUTFILE;       /* restore true status */
        }
    }                                                   //  Include window zero in output-write
    if(!dz->vflag[0]) {
        if((exit_status = write_samps(buf,dz->itemcnt * dz->wanted,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/**************************** FRACTAL_COPY_REVERSE_SHORTEN *****************************
 *
 *  SHRINK
 *
 *  |-------------->|   to  |--------------->|<---------------| to  |------->|<-------|  
 *
 *   |---A----|----B---| to |---A--->|<---A---|---B--->|<---B---|   |-a->|<-a-|-b->|<-b-|
 *        
 *  |-a->|<-a-|-b->|<-b-| to |-a->|<-a-|<-a-|-a->||-b->|<-b-|<-b-|-b->| to  |a>|<a|<a|a>|b>|<b|<b|b>|   
 *
 *  ADD
 *                                                                  |<---------------|
 *  |--------------->|  to  |--------------->|<---------------| to  |--------------->|   
 *
 *                                                                  |---a1-->--a2--->|
 *  |<---a2--<--a1---|      |<---a2--<---a1--|---a1-->--a2--->|     |<--a2--<--a1----|
 *  |----a1-->--a2-->|  to  |---a1--->--a2-->|---a2-->--a1--->| to  |---a2-->--a1--->|   
 *                                                                  |---a1-->--a2--->|  
 */

int fractal_copy_reverse_shorten(dataptr dz)
{
    int exit_status;
    char outfilename[2000];
    int grpcnt, cutcnt, startwin, endwin, n, j, buf2pos;
    float *buf = dz->flbufptr[0];
    grpcnt = dz->copycnt;                                       //  Start off repatterning all existing segments
    cutcnt = 1;
    for(n = 0;n < dz->iparam[FRACDEPTH]; n++) {                 //  For every cut-stage : loop-n
        if(dz->vflag[0]) {
            dz->process_type = BIG_ANALFILE;
            dz->outfiletype  = ANALFILE_OUT;
            if((exit_status = create_new_outfile(n,outfilename,dz))<0)
                return exit_status;
        }
        startwin = 0;                                           //  Specify start and end windows of first-cut of cutcnt repatternings
        endwin  = grpcnt;
        buf2pos = 0;                                            //  Reset pointer in 2nd window to 0;       
        for(j = 0; j < cutcnt;j ++) {                           //  For the number of cuts to be made at this stage : loop-j
                                                                //  Copy windows to 2nd buffer  abcd
            if((exit_status = frac_copy(&buf2pos,startwin,endwin,dz))<0)
                return exit_status;
            if((exit_status = frac_reverse_copy(&buf2pos,startwin,endwin,dz))<0)
                return exit_status;
            startwin += grpcnt;                             
            endwin   += grpcnt;                                 //  Advance start and end of cut in j-loop  
        }
        switch (dz->mode) {
        case(2):
            if((exit_status = frac_interp_shrink(dz)) < 0)
                return exit_status;
            break;
        case(3):
            if((exit_status = frac_add_shrink(dz)) < 0)
                return exit_status;
            break;
        }
        if(dz->vflag[0]) {
            if((exit_status = write_samps(buf,dz->itemcnt * dz->wanted,dz))<0)
                return(exit_status);
            if((exit_status = headwrite(dz->ofd,dz))<0) {
                free(outfilename);
                return(exit_status);
            }
            if(sndcloseEx(dz->ofd) < 0) {
                fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
                fflush(stdout);
            }
            dz->ofd = -1;
            dz->process_type = OTHER_PROCESS;       /* restore true status */
            dz->outfiletype  = NO_OUTPUTFILE;       /* restore true status */
        }
        grpcnt /= 2;                                            //  Gpsize to Cut gets smaller (by 1/2)
        cutcnt *= 2;                                            //  So The number of cuts goes up by 2
    }
    if(!dz->vflag[0]) {
        if((exit_status = write_samps(buf,dz->itemcnt * dz->wanted,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/**************************** FRAC_COPY *****************************/

//  COPY WINDOWS
int frac_copy(int *buf2pos,int startwin,int endwin,dataptr dz)
{
    int n, sttsamp;
    float *buf = dz->flbufptr[0], *buf2 = dz->flbufptr[1];
    for(n = startwin; n < endwin; n++) {
        sttsamp = n * dz->wanted;
        memcpy((char *)&(buf2[*buf2pos]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
        *buf2pos += dz->wanted;
    }
    return FINISHED;
}

/**************************** FRAC_REVERSE_COPY *****************************/

//  COPY WINDOWS IN REVERSE ORDER
int frac_reverse_copy(int *buf2pos,int startwin,int endwin,dataptr dz)
{
    int n, sttsamp;
    float *buf = dz->flbufptr[0], *buf2 = dz->flbufptr[1];
    for(n = endwin-1; n > startwin; n--) {
        sttsamp = n * dz->wanted;
        memcpy((char *)&(buf2[*buf2pos]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
        *buf2pos += dz->wanted;
    }
    return FINISHED;
}

/**************************** FRAC_INTERP_SHRINK *****************************
 *
 *  TIME-SHRINK TWO WINDOWS INTO ONE BY INTERPOLATING
 *
 *          Buf             Buf2
 *      size itemcnt     size copycnt     size copycnt
 *  |0<-------------->|<-------------->|<-------------->|
 *      = copycnt+1           |           /
 *                            |       /
 *          Buf               |   / Interpolate
 *      size itemcnt          |/    
 *  |0<-------------->|<-------------->|
 *      = copycnt+1      size copycnt
 *
 *
 *          Buf             Buf2
 *      size itemcnt     size copycnt
 *  |0<-------------->|<-------------->|
 *           ^         |______________|
 *           |                |
 *           |________________|
 *                  Copy
 *
 */

int frac_interp_shrink(dataptr dz)
{
    int copyto = 0, copyfrom = 0, n, m;
    float *buf = dz->flbufptr[0], *buf2 = dz->flbufptr[1];
    for(n = 1,m = 0; n < dz->itemcnt; n++,m+=2) {       //  Ignore window 0 -- and window 1, which retains value
        copyto   = n * dz->wanted;
        copyfrom = m * dz->wanted;
        memcpy((char *)&(buf[copyto]),(char *)&(buf2[copyfrom]),dz->wanted * sizeof(float));
    }
    memset((char *)buf2,0,dz->copycnt * 2 * dz->wanted * sizeof(float));  //SAFETY
    return FINISHED;
}

/**************************** FRAC_ADD_SHRINK *****************************
 *
 *  TIME-SHRINK TWO WINDOWS INTO ONE BY AVERAGING THEM
 *
 *          Buf             Buf2
 *      size itemcnt     size copycnt     size copycnt
 *  |0<-------------->|<-------------->|<-------------->|
 *      = copycnt+1           |           /
 *                            |       /
 *          Buf               |   / Average
 *      size itemcnt          |/    
 *  |0<-------------->|<-------------->|
 *      = copycnt+1      size copycnt
 *
 *
 *          Buf             Buf2
 *      size itemcnt     size copycnt
 *  |0<-------------->|<-------------->|
 *           ^         |______________|
 *           |                |
 *           |________________|
 *                  Copy
 *
 */

int frac_add_shrink(dataptr dz)
{
    int winfrom2, winto2, winto1, k, n, m, vc;                     //  Ignore window 1 IN FIRST buffer
    float *buf = dz->flbufptr[0], *buf2 = dz->flbufptr[1];
    double val;                                                     //  Add the 2 halves of the 2nd (double-length) buffer
    for(k = 1,n = 0,m = dz->copycnt; n < dz->copycnt; k++,n++,m++) {
        winfrom2 = m * dz->wanted;                                  //  Into the first buffer, ignoring window 1
        winto2   = n * dz->wanted;
        winto1   = k * dz->wanted;                                  //  Ignore window 1 IN FIRST buffer
        for(vc = 0;vc < dz->wanted;vc++) {
            val = (buf2[winto2 + vc] + buf2[winfrom2 + vc])/2.0;    //  Add the 2 halves of the 2nd (double-length) buffer
            buf[winto1 + vc] = (float)val;                          //  And place result in first buffer
        }
    }
    memset((char *)buf2,0,dz->copycnt * 2 * dz->wanted * sizeof(float));  //SAFETY
    return FINISHED;
}

/**************************** CREATE_NEW_OUTFILE *****************************/

int create_new_outfile(int n,char *outfilename,dataptr dz)
{
    int exit_status;
    char prefix_units[]     = "_00";
    char prefix_tens[]      = "_0";
    char prefix_hundreds[]  = "_";
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
        if(!sloom) {
            sprintf(errstr, "Soundfile %s already exists: Made %d outputs only.\n",outfilename,n);
            dz->ofd = -1;
            return(GOAL_FAILED);
        } else {
            dz->ofd = -1;
            sprintf(errstr, "Failed to create outfile number %d\n",n+1);
            return(SYSTEM_ERROR);
        }
    }
    return FINISHED;
}

/**************************** FRACTAL_BY_HALVES *****************************/

int fractal_by_halves(dataptr dz)
{
    int exit_status;
    char outfilename[2000];
    int grpcnt, cutcnt, startwin, endwin, n, j, buf2pos;
    float *buf2 = dz->flbufptr[1];
    grpcnt = dz->copycnt;                                       //  Start off repatterning all existing segments
    cutcnt = 1;
    for(n = 0;n < dz->iparam[FRACDEPTH]; n++) {                 //  For every cut-stage : loop-n
        if(dz->vflag[0]) {
            dz->process_type = BIG_ANALFILE;
            dz->outfiletype  = ANALFILE_OUT;
            if((exit_status = create_new_outfile(n,outfilename,dz))<0)
                return exit_status;
        }
        startwin = 0;                                           //  Specify start and end windows of first-cut of cutcnt repatternings
        endwin  = grpcnt;
        buf2pos = 0;                                            //  Reset pointer in 2nd window to 0;       
        for(j = 0; j < cutcnt;j ++) {                           //  For the number of cuts to be made at this stage : loop-j
                                                                //  Copy windows to 2nd buffer  abcd

            switch(dz->mode) {
            case(4):
                if((exit_status = frac_swap(&buf2pos,startwin,endwin,dz))<0)
                    return exit_status;
                break;
            case(5):
                if((exit_status = frac_reverse_second_half(&buf2pos,startwin,endwin,dz))<0)
                    return exit_status;
                break;
            }
            startwin += grpcnt;                             
            endwin   += grpcnt;                                 //  Advance start and end of cut in j-loop  
        }
        if(dz->vflag[0]) {
            if((exit_status = write_samps(buf2,dz->itemcnt * dz->wanted,dz))<0)
                return(exit_status);
            if((exit_status = headwrite(dz->ofd,dz))<0) {
                free(outfilename);
                return(exit_status);
            }
            if(sndcloseEx(dz->ofd) < 0) {
                fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
                fflush(stdout);
            }
            dz->ofd = -1;
            dz->process_type = OTHER_PROCESS;       /* restore true status */
            dz->outfiletype  = NO_OUTPUTFILE;       /* restore true status */
        }
        grpcnt /= 2;                                            //  Gpsize to Cut gets smaller (by 1/2)
        cutcnt *= 2;                                            //  So The number of cuts goes up by 2
    }
    if(!dz->vflag[0]) {
        if((exit_status = write_samps(buf2,dz->itemcnt * dz->wanted,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

/**************************** FRAC_SWAP *****************************
 *  
 *  SWAP TWO HALVES OF SEGMENT
 *  
 *  |abcdefghIJKLMNOP|  ->  |IJKLMNOPabcdefgh|  swap 8s
 *  |abcdefghIJKLMNOP|  ->  |efghabcdMNOPIJKL|  swap 4s
 *  |abcdefghIJKLMNOP|  ->  |cdabghefKLIJOPMN|  swap 2s
 *  |abcdefghIJKLMNOP|  ->  |badcfehgJILKNMPO|  swap 1s
 */

int frac_swap(int *buf2pos,int startwin,int endwin,dataptr dz)
{
    int n, m, sttsamp, goalsamp, bufinc;
    float *buf = dz->flbufptr[0] + dz->wanted, *buf2 = dz->flbufptr[1];
    int halfswaplen = (endwin - startwin)/2;
    int halfbuspos = startwin + halfswaplen;
    bufinc = 0;
    for(n = startwin + halfswaplen, m = startwin; m < halfbuspos; n++,m++) {
        sttsamp  = n * dz->wanted;
        goalsamp = m * dz->wanted;
        memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
        bufinc += dz->wanted;
    }
    for(n = startwin, m = startwin + halfswaplen; n < halfbuspos; n++,m++) {
        sttsamp  = n * dz->wanted;
        goalsamp = m * dz->wanted;
        memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
        bufinc += dz->wanted;
    }
    *buf2pos += bufinc;
    return FINISHED;
}

/**************************** FRAC_REVERSE_SECOND_HALF *****************************
 *  
 *  REVERSE 2nd HALF OF SEGMENT
 *  
 *  |abcdefghIJKLMNOP|  ->  |abcdefghPONMLKJI|  reverse-2ndhalf 8s
 *  |abcdefghIJKLMNOP|  ->  |abcdhgfeIJKLPONM|  reverse-2ndhalf 4s
 *  |abcdefghIJKLMNOP|  ->  |abdcefhgIJLKMNPO|  reverse-2ndhalf 2s
 *  |abcdefghIJKLMNOP|  ->  |badcfehgJILKNMPO|  reverse-2ndhalf 1s
 *  
 */

int frac_reverse_second_half(int *buf2pos,int startwin,int endwin,dataptr dz)
{
    int n, m, sttsamp, goalsamp, bufinc;
    float *buf = dz->flbufptr[0] + dz->wanted, *buf2 = dz->flbufptr[1];
    int halfswaplen = (endwin - startwin)/2;
    int halfbuspos = startwin + halfswaplen;
    bufinc = 0;
    for(n = startwin, m = startwin; m < halfbuspos; m++,n++) {          //  COPY FIRST HALF
        sttsamp  = n * dz->wanted;
        goalsamp = m * dz->wanted;
        memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
        bufinc += dz->wanted;
    }
    startwin += halfswaplen;
    endwin   += halfswaplen;
    for(n = startwin, m = endwin - 1; m >= halfbuspos; m--,n++) {   //  REVERSE COPY SECOND HALF
        sttsamp  = n * dz->wanted;
        goalsamp = m * dz->wanted;
        memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
        bufinc += dz->wanted;
    }
    *buf2pos += bufinc;
    return FINISHED;
}

/**************************** FRAC_INTERLEAVE *****************************
 *  
 *  INTERLEAVE TWO HALVES OF SEGMENT
 *  
 *  |abcdefghIJKLMNOP|  ->  |abcdefghIJKLMNOP|  intlv 8s x
 *  |abcdefghIJKLMNOP|  ->  |abcdIJKLefghMNOP|  intlv 4s
 *  |abcdefghIJKLMNOP|  ->  |abIJcdKLefMNghOP|  intlv 2s
 *  |abcdefghIJKLMNOP|  ->  |aIbJcKdLeMfNgOhP|  intlv 1s
 */

int frac_interleave(int gpcnt,dataptr dz)
{
    int cnt_in_1sthalf, cnt_in_2ndhalf, k, z, sttsamp, goalsamp;
    float *buf = dz->flbufptr[0] + dz->wanted, *buf2 = dz->flbufptr[1]; //  Ignore zero-window in input
    int itlvlen = dz->copycnt;
    int halfitlvlen = itlvlen/2;
    int qatritlvlen = gpcnt/4;
    k = 0;
    cnt_in_1sthalf = 0;
    cnt_in_2ndhalf = halfitlvlen;
    for(z = 0; z < qatritlvlen; z++, cnt_in_1sthalf++) {
        sttsamp = cnt_in_1sthalf * dz->wanted;                              //  cnt_in_1sthalf starts at 0 gets to 1/4 way
        goalsamp = k * dz->wanted;
        memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
        k++;
    }
    for(z = 0; z < qatritlvlen; z++, cnt_in_2ndhalf++) {
        sttsamp = cnt_in_2ndhalf * dz->wanted;                              //  cnt_in_2ndhalf starts at 1/2 gets to 3/4 way
        goalsamp = k * dz->wanted;
        memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
        k++;
    }
    while(k < dz->copycnt) {
        for(z = 0; z < qatritlvlen; z++, cnt_in_1sthalf++) {                    //  cnt_in_1sthalf starts at 1/4 way gets to 1/2 way
            sttsamp = cnt_in_1sthalf * dz->wanted;
            goalsamp = k * dz->wanted;
            memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
            k++;
        }
        for(z = 0; z < qatritlvlen; z++, cnt_in_2ndhalf++) {                    //  cnt_in_2ndhalf starts at 3/4 gets to end
            sttsamp = cnt_in_2ndhalf * dz->wanted;
            goalsamp = k * dz->wanted;
            memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
            k++;
        }
    }
    return FINISHED;
}

/**************************** FRAC_INTERLEAVE_REVERSE *****************************
 *  
 *  INTERLEAVE/REVERSE TWO HALVES OF SEGMENT
 *  
 *  |abcdefghIJKLMNOP|  ->  |abcdefghPONMLKJI|  swap 8s
 *  |abcdefghIJKLMNOP|  ->  |abcdPONMefghLKJI|  swap 4s
 *  |abcdefghIJKLMNOP|  ->  |abPOcdNMefLKghJI|  swap 2s
 *  |abcdefghIJKLMNOP|  ->  |aPbOcNdMeLfKgJhI|  swap 1s
 */

int frac_interleave_reverse(int gpcnt,dataptr dz)
{
    int cnt_in_1sthalf, cnt_in_2ndhalf, k, z, sttsamp, goalsamp;
    float *buf = dz->flbufptr[0] + dz->wanted, *buf2 = dz->flbufptr[1]; //  Ignore zero-window in input
    int itlvlen = dz->copycnt;
    int halfitlvlen = itlvlen/2;
    int qatritlvlen = gpcnt/4;
    k = 0;
    cnt_in_1sthalf = 0;
    cnt_in_2ndhalf = halfitlvlen + qatritlvlen - 1;                         //  Go to END of first segment in 2nd half
    for(z = 0; z < qatritlvlen; z++, cnt_in_1sthalf++) {
        sttsamp = cnt_in_1sthalf * dz->wanted;                              //  cnt_in_1sthalf starts at 0 gets to 1/4 way
        goalsamp = k * dz->wanted;
        memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
        k++;
    }
    for(z = 0; z < qatritlvlen; z++, cnt_in_2ndhalf--) {
        sttsamp = cnt_in_2ndhalf * dz->wanted;                              //  cnt_in_2ndhalf starts at 1/2 gets to 3/4 way
        goalsamp = k * dz->wanted;
        memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
        k++;
    }
    while(k < dz->copycnt) {
        for(z = 0; z < qatritlvlen; z++, cnt_in_1sthalf++) {                    //  cnt_in_1sthalf starts at 1/4 way gets to 1/2 way
            sttsamp = cnt_in_1sthalf * dz->wanted;
            goalsamp = k * dz->wanted;
            memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
            k++;
        }
        cnt_in_2ndhalf += (qatritlvlen * 2) - 1;                                //  Go from start of current segment, to END  of next
        for(z = 0; z < qatritlvlen; z++, cnt_in_2ndhalf--) {
            sttsamp = cnt_in_2ndhalf * dz->wanted;                              //  cnt_in_2ndhalf starts at 1/2 gets to 3/4 way
            goalsamp = k * dz->wanted;
            memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
            k++;
        }
    }
    return FINISHED;
}

/**************************** FRAC_INTERLEAVE_CYCLE *****************************
 *  
 *  INTERLEAVE TWO HALVES OF SEGMENT, BUT PROGRESS AT ORIGINAL SPEED
 *  
 *  |abcdefghIJKLMNOP|  ->  |abcdefghIJKLMNOP|  intlv 8s x
 *  |abcdefghIJKLMNOP|  ->  |abcdMNOPIJKLefgh|  intlv 4s
 *  |abcdefghIJKLMNOP|  ->  |abKLefOPIJcdMNgh|  intlv 2s
 *  |abcdefghIJKLMNOP|  ->  |aJcLeNgPIbKdMfOh|  intlv 1s
 */

int frac_interleave_cycle(int gpcnt,dataptr dz)
{
    int cnt_in_1sthalf, cnt_in_2ndhalf, k, z, sttsamp, goalsamp;
    float *buf = dz->flbufptr[0] + dz->wanted, *buf2 = dz->flbufptr[1]; //  Ignore zero-window in input
    int itlvlen = dz->copycnt;
    int halfitlvlen = itlvlen/2;
    int qatritlvlen = gpcnt/4;
    k = 0;
    cnt_in_1sthalf = 0;
    cnt_in_2ndhalf = halfitlvlen + 1;
    if(qatritlvlen <= 0) {
        sprintf(errstr,"INCORRECT DIVISION OF THE SPECTRUM\n");
        return PROGRAM_ERROR;
    }
    while(k < dz->copycnt) {
        for(z = 0; z < qatritlvlen; z++, cnt_in_1sthalf++) {
            sttsamp = cnt_in_1sthalf * dz->wanted;                              //  cnt_in_1sthalf starts at 0
            goalsamp = k * dz->wanted;
            memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
            k++;
        }
        for(z = 0; z < qatritlvlen; z++, cnt_in_2ndhalf++) {
            sttsamp = cnt_in_2ndhalf * dz->wanted;                              //  cnt_in_2ndhalf starts at 1
            goalsamp = k * dz->wanted;
            memcpy((char *)&(buf2[goalsamp]),(char *)&(buf[sttsamp]),dz->wanted * sizeof(float));
            k++;
        }
        cnt_in_1sthalf += qatritlvlen;              //  Leap over next window, in both halves
        cnt_in_2ndhalf += qatritlvlen;
        if(cnt_in_2ndhalf >= dz->copycnt)
            cnt_in_2ndhalf -= dz->copycnt;
    }
    return FINISHED;

}

/**************************** FRACTAL_BY_INTERLEAVING *****************************/

int fractal_by_interleaving(dataptr dz)
{
    int exit_status;
    char outfilename[2000];
    int grpcnt, n;
    float *buf = dz->flbufptr[0], *buf2 = dz->flbufptr[1];
    grpcnt = dz->copycnt;                                       //  Start off repatterning all existing segments
    if(dz->vflag[0]) {
        for(n = 0;n < dz->iparam[FRACDEPTH]; n++) {                 //  For every cut-stage : loop-n
            dz->process_type = BIG_ANALFILE;
            dz->outfiletype  = ANALFILE_OUT;
            if((exit_status = create_new_outfile(n,outfilename,dz))<0)
                return exit_status;
            switch(dz->mode) {
            case(6):
                if((exit_status = frac_interleave(grpcnt,dz))<0)
                    return exit_status;
                break;
            case(7):
                if((exit_status = frac_interleave_reverse(grpcnt,dz))<0)
                    return exit_status;
                break;
            case(8):
                if((exit_status = frac_interleave_cycle(grpcnt,dz))<0)
                    return exit_status;
                break;
            }
            if((exit_status = write_samps(buf,dz->wanted,dz))<0)        //  Write initial zero-buffer 
                return(exit_status);
            if((exit_status = write_samps(buf2,dz->itemcnt * dz->wanted,dz))<0)
                return(exit_status);
            if((exit_status = headwrite(dz->ofd,dz))<0) {
                free(outfilename);
                return(exit_status);
            }
            if(sndcloseEx(dz->ofd) < 0) {
                fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
                fflush(stdout);
            }
            dz->ofd = -1;
            dz->process_type = OTHER_PROCESS;       /* restore true status */
            dz->outfiletype  = NO_OUTPUTFILE;       /* restore true status */
            grpcnt /= 2;                                            //  Gpsize to Cut gets smaller (by 1/2)
        }
    } else {
        for(n = 0;n < dz->iparam[FRACDEPTH]; n++)                   //  For every cut-stage : loop-n
            grpcnt /= 2;                                            //  Gpsize to Cut gets smaller (by 1/2)
        switch(dz->mode) {
        case(6):
            if((exit_status = frac_interleave(grpcnt,dz))<0)
                return exit_status;
            break;
        case(7):
            if((exit_status = frac_interleave_reverse(grpcnt,dz))<0)
                return exit_status;
            break;
        case(8):
            if((exit_status = frac_interleave_cycle(grpcnt,dz))<0)
                return exit_status;
            break;
        }
        if((exit_status = write_samps(buf,dz->wanted,dz))<0)        //  Write initial zero-buffer 
            return(exit_status);
        if((exit_status = write_samps(buf2,dz->itemcnt * dz->wanted,dz))<0)
            return(exit_status);
    }
    return FINISHED;
}

