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



#include <stdlib.h>
#include <stdio.h>
#include <osbind.h>
#include <structures.h>
#include <cdpmain.h>
#include <tkglobals.h>
#include <sfsys.h>
#include <cdparams.h>
#include <globcon.h>
#include <localcon.h>
#include <processno.h>
#include <modeno.h>
#include <filetype.h>
#include <formants.h>
#include <pnames.h>
#include <sndinfo.h>
#include <headread.h>
#include <house.h>
#include <pvoc.h>
#include <special.h>
#include <ctype.h>
#include <string.h>
#include <logic.h>
#include <math.h>
#include <float.h>
#include <standalone.h>

#ifdef unix
#define round(x) lround((x))
#else
#define round(x) cdp_round((x))
#endif

static int do_extra_files(dataptr dz);
static void strip_ext(char *temp) ;
static int assign_wavetype(dataptr dz);
extern char *get_other_filename_x(char *filename,char c);

void dz2props(dataptr dz, SFPROPS* props);

/*RWD we only do this for SNDFILEs, not analfiles etc */
void dz2props(dataptr dz, SFPROPS* props)
{
    if(dz->outfiletype == SNDFILE_OUT){
        switch(dz->true_outfile_stype){
        case SAMP_SHORT:
            props->samptype = SHORT16;
            break;
        case SAMP_FLOAT:
            props->samptype = FLOAT32;
            break;
        case SAMP_BYTE:
            props->samptype = SHORT8;
            break;
        case SAMP_LONG:
            props->samptype = INT_32;
            break;
        case SAMP_2424:
            props->samptype = INT2424;
            break;
        case SAMP_2432:
            props->samptype = INT2432;
            break;
        case SAMP_2024:
            props->samptype = INT2024;
            break;
        default:
            props->samptype = INT_MASKED;
            break;
        }
        props->srate = dz->outfile->srate;
        props->chans = dz->outchans; //or outfile->channels?
        props->type = wt_wave;
        props->format = WAVE_EX;
        props->chformat = MC_STD;
        
    }
}

/************************ SETUP_PARAM_RANGES_AND_DEFAULTS *********************/

int setup_param_ranges_and_defaults(dataptr dz)
{                   
    int exit_status;
    aplptr ap = dz->application;
    if(ap->total_input_param_cnt>0) {
        if((exit_status = set_param_ranges(dz->process,dz->mode,dz->nyquist,dz->frametime,dz->infile->arate,
        dz->infile->srate,dz->wlength,dz->insams[0],dz->infile->channels,dz->wanted,
        dz->infile->filetype,dz->linecnt,dz->duration,ap))<0)
            return(exit_status);
        if((exit_status = INITIALISE_DEFAULT_VALUES(dz->process,dz->mode,dz->infile->channels,
        dz->nyquist,dz->frametime,dz->insams[0],dz->infile->srate,dz->wanted,
        dz->linecnt,dz->duration,ap->default_val,dz->infile->filetype,ap))<0)
            return(exit_status);
        if(!sloom)
            put_default_vals_in_all_params(dz);
        else if(dz->process == BRASSAGE || dz->process == SAUSAGE)
            put_default_vals_in_all_params(dz);
    }
    return(FINISHED);
}

/************************ HANDLE_FORMANTS *********************/

int handle_formants(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    if(ap->formant_flag) {

        if(!sloom) {
            if(*cmdlinecnt <= 0) {
                sprintf(errstr,"Insufficient parameters on command line\n");
                return(USAGE_ONLY);
            } 
        }
        if((exit_status = read_formantband_data_and_setup_formants(cmdline,cmdlinecnt,dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/************************ HANDLE_FORMANT_QUIKSEARCH *********************/

int handle_formant_quiksearch(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    if(ap->formant_qksrch) {
        if((exit_status = read_formant_qksrch_flag(cmdline,cmdlinecnt,dz))<0)
            return(exit_status);
    }

    return(FINISHED);
}
                        
/************************ HANDLE_SPECIAL_DATA *********************/

int handle_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;

    if(ap->special_data) {
        if(!sloom) {
            if(*cmdlinecnt <= 0) {
                sprintf(errstr,"Insufficient parameters on command line.\n");
                return(USAGE_ONLY);
            }
        }
        if((exit_status = setup_special_data_ranges
        (dz->mode,dz->infile->srate,dz->duration,dz->nyquist,dz->wlength,dz->infile->channels,ap))<0)
            return(exit_status);

        if((exit_status = read_special_data((*cmdline)[0],dz))<0)
            return(exit_status);
        (*cmdline)++;       
        (*cmdlinecnt)--;
    }
    return(FINISHED);
}
                              
/************************ READ_FORMANT_QKSRCH_FLAG *********************/

int read_formant_qksrch_flag(char ***cmdline,int *cmdlinecnt,dataptr dz)
{

    if(!sloom) {
        if(*cmdlinecnt <= 0)
            return(FINISHED);
        if(!strcmp((*cmdline)[0],"-i")) {
             dz->deal_with_chan_data = IGNORE_ACTUAL_CHAN_FRQS;
            (*cmdline)++;
            (*cmdlinecnt)--;
        }
    } else {
        ((*cmdline)[0])++;      /* Skip numericval marker */
        switch((*cmdline)[0][0]) {
        case('0'):  break;  /* default value remains set */
        case('1'):  dz->deal_with_chan_data = IGNORE_ACTUAL_CHAN_FRQS; break;
        default:
            sprintf(errstr,"Unknown data at formant_quicksearch position in data sent from TK\n");
            return(DATA_ERROR);
        }
        (*cmdline)++;
        (*cmdlinecnt)--;
    }
    return(FINISHED);
}

/************************ HOUSEKEEP_FILES *********************/

int do_housekeep_files(char *filename,dataptr dz)
{
    int exit_status;

    if(dz->process==HOUSE_BUNDLE) {
        if(dz->wordstor!=NULL)
            free_wordstors(dz);
        dz->all_words = 0;
        if((exit_status = store_filename(filename,dz))<0)
            return(exit_status);
    }
    if(!sloom) {                /* cmdline uses infilename as default outname */
        if((exit_status = setup_file_outname_where_ness(filename,dz))<0)
            return(exit_status);
    }
    if(dz->process==HOUSE_COPY) {
        if(dz->mode!=COPYSF) {
            if(dz->infile->filetype!=SNDFILE) {
                sprintf(errstr,"This process only works with soundfiles.\n");
                return(DATA_ERROR);
            }
            return(FINISHED);
        }
        switch(dz->infile->filetype) {
        case(WORDLIST):     dz->process_type = TO_TEXTFILE;     dz->outfiletype = TEXTFILE_OUT;                   break;
        case(SNDFILE):      dz->process_type = EQUAL_SNDFILE;   dz->outfiletype = SNDFILE_OUT;                    break;
        case(ANALFILE):     dz->process_type = EQUAL_ANALFILE;  dz->outfiletype = ANALFILE_OUT;                   break;
        case(PITCHFILE):    dz->process_type = PITCH_TO_PITCH;  dz->outfiletype = PITCH_OUT;                      break;
        case(TRANSPOSFILE): dz->process_type = PITCH_TO_PITCH;  dz->outfiletype=PITCH_OUT;  dz->is_transpos=TRUE; break;
        case(FORMANTFILE):  dz->process_type = EQUAL_FORMANTS;  dz->outfiletype = FORMANTS_OUT;                   break;
        case(ENVFILE):      dz->process_type = EQUAL_ENVFILE;   dz->outfiletype = ENVFILE_OUT;                    break;
        default:
            sprintf(errstr,"Impossible infile type: assign_copy_constants()\n");
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/************************* CHECK_REPITCH_TYPE **********************/

int check_repitch_type(dataptr dz)
{
    if(dz->infile->filetype==PITCHFILE && dz->mode==TTT) {
        sprintf(errstr,"Cannot do a tranpos/transpos operation on a pitchfile\n");
        return(DATA_ERROR);
    } else if(dz->infile->filetype==TRANSPOSFILE) {
        if(dz->mode==PPT) {
            sprintf(errstr,"Can't do a pitch/pitch operation on a transposition file.\n");
            return(DATA_ERROR);
        } else if(dz->mode==PTP) {
            sprintf(errstr,"Can't do a pitch/transpos starting with a transposition file.\n");
            return(DATA_ERROR);
        }
    }
    return(FINISHED);
}

/****************************** EXCEPTIONAL_REPITCH_VALIDITY_CHECK *********************************/

int exceptional_repitch_validity_check(int *is_valid,dataptr dz)
{
    switch(dz->mode) {
    case(PPT):
    case(PTP):
        if(dz->could_be_pitch==FALSE)
            *is_valid = FALSE;            /*RWD was == both times */
        break;
    case(TTT):
        if(dz->could_be_transpos==FALSE)
            *is_valid = FALSE;
        break;
    default:
        sprintf(errstr,"Unknown case in exceptional_repitch_validity_check()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** SETUP_BRKTABLESIZES *********************************/

int setup_brktablesizes(infileptr infile_info,dataptr dz)
{

    if(could_be_transpos_and_or_pitch_infiletype(dz->infile->filetype)) {

        if(dz->input_data_type==BRKFILES_ONLY
        || dz->input_data_type==ALL_FILES
        || dz->input_data_type==DB_BRKFILES_ONLY
        || dz->input_data_type==UNRANGED_BRKFILE_ONLY) {    /* is NOT pitch or transpos data */
            if(dz->extrabrkno < 0 || dz->brk==NULL) {
                sprintf(errstr,"Attempt to use brksize before allocated:1: setup_brktablesizes()\n");
                return(PROGRAM_ERROR);
            }
            dz->brksize[dz->extrabrkno] = infile_info->brksize;

        } else                                          /* must be a pitch or transpos process */
            dz->tempsize = infile_info->brksize;            

    } else if(is_brkfile_infiletype(dz->infile->filetype)) {
        if(dz->extrabrkno < 0 || dz->brk==NULL) {
            sprintf(errstr,"Attempt to use brksize before allocated:2: setup_brktablesizes()\n");
            return(PROGRAM_ERROR);
        }
        dz->brksize[dz->extrabrkno] = infile_info->brksize;
    }
    return(FINISHED);
}

/****************************** STORE_FILENAME *********************************/

int store_filename(char *filename,dataptr dz)
{
    if(dz->wordstor != NULL) {
        sprintf(errstr,"Cannot store filename: wordstor already allocated.\n");
        return(PROGRAM_ERROR);
    }
    if((dz->wordstor = (char **)malloc(sizeof(char *)))==NULL) {
        sprintf(errstr,"Cannot store filename.\n");
        return(MEMORY_ERROR);
    }
    if((dz->wordstor[0] = (char *)malloc((strlen(filename)+1) * sizeof(char)))==NULL) {
        sprintf(errstr,"Cannot store filename.\n");
        return(MEMORY_ERROR);
    }
    if(strcpy(dz->wordstor[0],filename)!= dz->wordstor[0]) { 
        sprintf(errstr,"Failed to copy filename to store.\n");
        return(PROGRAM_ERROR);
    }
    dz->all_words++;
    return(FINISHED);
}

/****************************** STORE_FURTHER_FILENAME *********************************/

int store_further_filename(int n,char *filename,dataptr dz)
{
    if(dz->wordstor == NULL) {
        if((dz->wordstor = (char **)malloc((n+1) *sizeof(char *)))==NULL) {
            sprintf(errstr,"Cannot store further filename.\n");
            return(MEMORY_ERROR);
        }
    } else {
        if((dz->wordstor = (char **)realloc((dz->wordstor),(n+1) *sizeof(char *)))==NULL) {
            sprintf(errstr,"Cannot store further filename.\n");
            return(MEMORY_ERROR);
        }
    }
    if((dz->wordstor[n] = (char *)malloc((strlen(filename)+1) * sizeof(char)))==NULL) {
        sprintf(errstr,"Cannot store further filename.\n");
        return(MEMORY_ERROR);
    }
    if(strcpy(dz->wordstor[n],filename)!=dz->wordstor[n]) { 
        sprintf(errstr,"Failed to copy filename to store.\n");
        return(PROGRAM_ERROR);
    }
    dz->all_words++;
    return(FINISHED);
}

/************************ COUNT_AND_ALLOCATE_FOR_INFILES *********************/

int count_and_allocate_for_infiles(int argc,char *argv[],dataptr dz)
{
    int exit_status;    
    switch(dz->input_data_type) {
    case(NO_FILE_AT_ALL):   /* processes actually do NOT USE the input file */
        dz->infilecnt = 0;  
        break;
    case(SNDFILES_ONLY):            case(ANALFILE_ONLY):
    case(MIXFILES_ONLY):            case(SNDLIST_ONLY):             case(WORDLIST_ONLY):
    case(ENVFILES_ONLY):            case(BRKFILES_ONLY):            case(DB_BRKFILES_ONLY):
    case(FORMANTFILE_ONLY):         case(PITCHFILE_ONLY):           case(PITCH_OR_TRANSPOS):
    case(SND_OR_MIXLIST_ONLY):      case(SND_SYNC_OR_MIXLIST_ONLY): case(UNRANGED_BRKFILE_ONLY):
        dz->infilecnt = 1;  
        break;
    case(ALL_FILES):        /* i.e. processes accept and use any type of file */
        switch(dz->process) {
        case(HOUSE_BUNDLE):
            if((exit_status = count_bundle_files(argc,argv,dz))<0)
                return(exit_status);
            break;
        case(INFO_DIFF):  dz->infilecnt = 2;    break;
        default:          dz->infilecnt = 1;    break;
        }
        break;
    case(TWO_SNDFILES):             case(SNDFILE_AND_ENVFILE):      case(SNDFILE_AND_BRKFILE):      
    case(SNDFILE_AND_DB_BRKFILE):   case(TWO_ANALFILES):            case(ANAL_AND_FORMANTS):
    case(PITCH_AND_FORMANTS):       case(PITCH_AND_PITCH):          case(PITCH_AND_TRANSPOS):
    case(TRANSPOS_AND_TRANSPOS):    case(ANAL_WITH_PITCHDATA):      case(ANAL_WITH_TRANSPOS):
    case(SNDFILE_AND_UNRANGED_BRKFILE):
        dz->infilecnt = 2;  
        break;
    case(THREE_ANALFILES):
//TW NEW
    case(PFE):
        dz->infilecnt = 3;  
        break;
    case(ONE_OR_MANY_SNDFILES):
//TW NEW
    case(ONE_OR_MORE_SNDSYS):
        if((exit_status = count_infiles(argc,argv,dz))<0)
            return(exit_status);
        break;
    case(MANY_SNDFILES):            case(MANY_ANALFILES):   case(ANY_NUMBER_OF_ANY_FILES):
        if((exit_status = count_infiles(argc,argv,dz))<0)
            return(exit_status);
        if(dz->infilecnt < 2) {
            sprintf(errstr,"Insufficient input files for this process\n");
            return(USAGE_ONLY);
        }
        break;
    default:
        sprintf(errstr,"Unknown input_data_type: count_and_allocate_for_infiles()\n");
        return(PROGRAM_ERROR);
    }
    return allocate_filespace(dz);
}

/************************ COUNT_INFILES *********************/

int count_infiles(int argc,char *argv[],dataptr dz)
{
    int files_to_skip = 1, diff;
    int other_unflagged_items;
//TW NEW CODE OK
    if(dz->outfiletype != NO_OUTPUTFILE)
        files_to_skip++;

    argc -= files_to_skip;
    argv += files_to_skip;
    if(argc<0) {
        sprintf(errstr,"Insufficient cmdline parameters.\n");
        return(USAGE_ONLY);
    }
    other_unflagged_items = 0;
    while(argc > 0) {         /* count all unflagged items after outfile */
        if(*(argv[0])!='-' || !isalpha(argv[0][1]))
            other_unflagged_items++;        /* options, variants, formants, and formant_qiksrch are flagged */
        argc--;
        argv++;
    }
    if(dz->application->special_data)   /* subtract any special data item : not flagged */  
        other_unflagged_items--;
    if((diff = other_unflagged_items - dz->application->param_cnt)<0) {
        sprintf(errstr,"Insufficient cmdline parameters.\n");
        return(USAGE_ONLY);
    }
    dz->infilecnt = diff + 1; /* i.e. plus first infile */
    return(FINISHED);
}

/************************ COUNT_BUNDLE_FILES *********************/

int count_bundle_files(int argc,char *argv[],dataptr dz)
{
    /*int files_to_skip = 1;*/
    int orig_argc = argc;
    if(argc<1) {
        sprintf(errstr,"Insufficient cmdline parameters.\n");
        return(USAGE_ONLY);
    }
    while(argc > 0) {
        if(*(argv[0])=='-') {
            sprintf(errstr,"Invalid flag or parameter on cmdline.\n");
            return(DATA_ERROR);
        }
        argc--;
        argv++;
    }
    dz->infilecnt = orig_argc - 1; /* i.e. minus outfile */
    return(FINISHED);
}

/******************************* PUT_DEFAULT_VALS_IN_ALL_PARAMS **************************/

void put_default_vals_in_all_params(dataptr dz)
{
    aplptr ap = dz->application;
    int n;
    for(n=0;n<ap->total_input_param_cnt;n++) {
        dz->param[n]  = ap->default_val[n];
        if(dz->is_int[n])
            dz->iparam[n] = round(dz->param[n]);
    }
}

/************************ MAKE_INITIAL_CMDLINE_CHECK *********************/

int make_initial_cmdline_check(int *argc,char **argv[])
{
    int exit_status;
    if(*argc<4) {       /* INITIAL CHECK OF CMDLINE OR PRE-CMDLINE DATA */
        /*RWD May 2005*/
        fprintf(stdout,"CDP Release 7.1 2016\n");
        if((exit_status = usage(*argc,*argv))<0)
            return(exit_status);
    }
    (*argv)++;
    (*argc)--;
    return(FINISHED);
}

/************************ TEST_APPLICATION_VALIDITY *********************/

int test_application_validity(infileptr infile_info,int process,int *valid,dataptr dz)
{
    int exit_status;
    int is_valid = valid_application(process,valid);

    if((dz->process==REPITCH || process==REPITCHB) && is_a_text_input_filetype(infile_info->filetype) && is_valid) {
        if((exit_status = exceptional_repitch_validity_check(&is_valid,dz))<0)
            return(exit_status);
    }
    if(!is_valid) {                         /* GUI: invalid applics WON'T BE DISPLAYED on menus */
        sprintf(errstr,"Application doesn't work with this type of infile.\n");
        return(USAGE_ONLY);                 /* validity can be checked from dz->valid bitflags  */
    }
    return(FINISHED);
}

/************************ PARSE_INFILE_AND_HONE_TYPE *********************/

int parse_infile_and_hone_type(char *filename,int *valid,dataptr dz)
{
    int exit_status;
    infileptr infile_info;

    if(!sloom) {
        if((infile_info = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for infile structure to test TK data.");
            return(MEMORY_ERROR);
        }
        if((exit_status = cdparse(filename,infile_info))<0)
            return(exit_status);
        if((exit_status = establish_application_validities(infile_info->filetype,infile_info->channels,valid))<0)
            return(exit_status);
        if((exit_status = test_application_validity(infile_info,dz->process,valid,dz))<0) 
            return(exit_status);
        if((exit_status = copy_parse_info_to_main_structure(infile_info,dz))<0)
            return(exit_status);
        free(infile_info);
    }
                    /* KNOWING POSSIBLE TYPE OF INFILE, AND TYPE NEEDED BY PROCESS, HONE TYPE */
    if(is_a_text_input_filetype(dz->infile->filetype)) {
        if((exit_status = redefine_textfile_types(dz))<0)
            return(exit_status);
    }
    switch(dz->process) {
    case(HOUSE_COPY):   case(HOUSE_CHANS):  case(HOUSE_EXTRACT):    case(HOUSE_SPEC):   case(HOUSE_BUNDLE):
    case(HOUSE_SORT):   case(HOUSE_DISK):
    case(HOUSE_GATE):
        if((exit_status = do_housekeep_files(filename,dz))<0)
            return(exit_status);
        break;
    case(INFO_TIMELIST):    case(MIXDUMMY):
    case(INFO_LOUDLIST):
    case(MIX_ON_GRID):
    case(MIX_AT_STEP):  case(TOPNTAIL_CLICKS):
    case(MAKE_VFILT):
        dz->all_words = 0;
        if((exit_status = store_filename(filename,dz))<0)
            return(exit_status);
        break;
    case(MOD_LOUDNESS):
        if(dz->mode==LOUDNESS_LOUDEST || dz->mode==LOUDNESS_EQUALISE) { 
            dz->all_words = 0;
            if((exit_status = store_filename(filename,dz))<0)
                return(exit_status);
        }
        break;
    case(RANDCUTS): case(RANDCHUNKS):
        if(!sloom) {
            dz->all_words = 0;
            if((exit_status = store_filename(filename,dz))<0)
                return(exit_status);
        }
        break;
    case(REPITCH):  case(REPITCHB):
        if(!is_a_text_input_filetype(dz->infile->filetype)) {
            if((exit_status = check_repitch_type(dz))<0)
                return(exit_status);
        }
        break;
    }
    if(anal_infiles) {
        if((exit_status = set_chunklens_and_establish_windowbufs(dz))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/************************* SET_CHUNKLENS_AND_ESTABLISH_WINDOWBUFS **********************/

int set_chunklens_and_establish_windowbufs(dataptr dz)
{
    int exit_status;
    int n;
    switch(dz->infile->filetype) {
    case(ANALFILE):
        dz->clength     = dz->wanted / 2;
        dz->chwidth     = dz->nyquist/(double)(dz->clength-1);
        dz->halfchwidth = dz->chwidth/2.0;

        if((exit_status = float_array(&dz->amp,dz->clength))<0)       
            return(exit_status);
        if((exit_status = float_array(&dz->freq,dz->clength))<0)
            return(exit_status);
        break;
    case(PITCHFILE):
    case(TRANSPOSFILE):
        dz->wanted      = dz->infile->origchans;
        dz->clength     = dz->infile->origchans / 2;
        dz->chwidth     = dz->nyquist/(double)(dz->clength-1);
        dz->halfchwidth = dz->chwidth/2.0;

        if((exit_status = float_array(&dz->amp,dz->clength))<0)       
            return(exit_status);
        if((exit_status = float_array(&dz->freq,dz->clength))<0)
            return(exit_status);
        break;
    case(FORMANTFILE):
        dz->wanted      = dz->infile->specenvcnt;
        break;
    }
    for(n=0;n<dz->extra_bufcnt;n++) {
        if((exit_status = float_array(&(dz->windowbuf[n]),dz->wanted))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/******************************** SETUP_FILE_OUTNAME_WHERE_NESS ************************
*
*   CMDLINE uses infilename as basis of outfilename(s)
*   TK uses the default outfilename instead
*/

int setup_file_outname_where_ness(char *filename,dataptr dz)
{
    int exit_status;
    switch(dz->process) {                   
    case(RANDCUTS):                 
    case(RANDCHUNKS):                   
    case(HOUSE_CHANS):                  
    case(EDIT_CUTMANY):
    case(MANY_ZCUTS):
    case(SYLLABS):
    case(SHUDDER):
    case(JOIN_SEQ):
    case(JOIN_SEQDYN):
    case(SYNTH_WAVE):   case(SYNTH_NOISE):  case(SYNTH_SIL):    case(CLICK):
    case(ENV_CREATE):   case(ENV_EXTRACT):  case(MULTI_SYN):    
    case(FORMANTS):     case(FMNTSEE):
    case(P_SEE):        case(P_HEAR):
    case(PVOC_ANAL):    case(PVOC_SYNTH):
    case(P_SYNTH):      case(P_VOWELS):     case(ANALENV):
    case(LEVEL):        case(ENVSYN):
        dz->all_words = 0;                                  
        if((exit_status = store_filename(filename,dz))<0)
            return(exit_status);
        break;
    case(RRRR_EXTEND):
        if(dz->mode == 2) {
            dz->all_words = 0;                                  
            if((exit_status = store_filename(filename,dz))<0)
                return(exit_status);
        }
        break;
    case(HOUSE_COPY):
        if(dz->mode == DUPL) {
            dz->all_words = 0;                                  
            if((exit_status = store_filename(filename,dz))<0)
                return(exit_status);
        }
        break;
    case(HOUSE_EXTRACT):
        if(dz->mode==HOUSE_CUTGATE) {
            dz->all_words = 0;
            if((exit_status = store_filename(filename,dz))<0)
                return(exit_status);
        }
        break;
    case(HOUSE_SORT):
        if((exit_status = store_further_filename(dz->all_words,filename,dz))<0)
            return(exit_status);
        break;
    case(HF_PERM1):
    case(HF_PERM2):
        if(dz->mode == HFP_SNDSOUT) {
            dz->all_words = 0;
            if((exit_status = store_filename(filename,dz))<0)
                return(exit_status);
        }
        break;
    case(SYNTH_SPEC):
    case(TWIXT):
    case(HOUSE_GATE):
    case(TIME_GRID):
    case(FORMSEE):
    case(MAKE):
        dz->all_words = 0;                                  
        if((exit_status = store_filename(filename,dz))<0)
            return(exit_status);
        break;
    case(MAKE_VFILT):                           
        if((exit_status = store_further_filename(dz->all_words,filename,dz))<0)
            return(exit_status);
        break;
    case(MOD_LOUDNESS):
        if(dz->mode==LOUDNESS_LOUDEST || dz->mode==LOUDNESS_EQUALISE) { 
            if((exit_status = store_further_filename(dz->all_words,filename,dz))<0)
                return(exit_status);
        }
        break;
    case(MOD_RADICAL):
//TW Store outputfile name, for use with tempfile mechanism to avoic sndseek problems
        if(dz->mode==MOD_LOBIT || dz->mode==MOD_LOBIT2) { 
            dz->all_words = 0;
            if((exit_status = store_filename(filename,dz))<0)
                return(exit_status);
        }
        break;
    case(BRASSAGE): case(SAUSAGE):
//TW Store outputfile name, for use with tempfile mechanism to avoic sndseek problems
        dz->all_words = 0;
        if((exit_status = store_filename(filename,dz))<0)
            return(exit_status);
        break;
    }
    return(FINISHED);
}

/****************************** SET_SPECIAL_PROCESS_SIZES *********************************/

int set_special_process_sizes(dataptr dz)
{
    int maxsize, minsize;                                     
    int n;
    switch(dz->process) {
    case(SUM):      /* progs where outsize is maximum of insizes */
    case(MAX):
        maxsize = dz->insams[0];
        for(n=0;n<dz->infilecnt;n++)
            maxsize = max(dz->insams[n],maxsize);
        dz->wlength = maxsize/dz->wanted;        
        break;
    case(CROSS):    /* progs where outsize is minimum of insizes */
    case(MEAN):     
    case(VOCODE):   
    case(LEAF):
        minsize = dz->insams[0];
        for(n=1;n<dz->infilecnt;n++)
            minsize = min(dz->insams[n],minsize);
        dz->wlength = minsize/dz->wanted;
        break;
    }
    return(FINISHED);
}

/************************ COPY_PARSE_INFO_TO_MAIN_STRUCTURE *********************/

int copy_parse_info_to_main_structure(infileptr infile_info,dataptr dz)
{

    if(dz->infilecnt < 1 ||  dz->insams==NULL) {
        sprintf(errstr,"Attempt to use file arrays before allocated: copy_parse_info_to_main_structure()\n");
        return(PROGRAM_ERROR);
    }
    dz->infile->filetype    = infile_info->filetype;
    /*dz->infilesize[0]     = infile_info->infilesize;*/
    dz->insams[0]           = infile_info->insams;
    dz->infile->srate       = infile_info->srate;
    dz->infile->channels    = infile_info->channels;
    dz->infile->stype       = infile_info->stype;
    dz->infile->origstype   = infile_info->origstype;
    dz->infile->origrate    = infile_info->origrate;
    dz->infile->Mlen        = infile_info->Mlen;
    dz->infile->Dfac        = infile_info->Dfac;
    dz->infile->origchans   = infile_info->origchans;       
    dz->infile->specenvcnt  = infile_info->specenvcnt;
    dz->specenvcnt          = infile_info->specenvcnt;
    dz->wanted              = infile_info->wanted;
    dz->wlength             = infile_info->wlength;
    dz->out_chans           = infile_info->out_chans;
    dz->descriptor_samps    = infile_info->descriptor_samps;   /*RWD ???? */
    dz->is_transpos         = infile_info->is_transpos;
    dz->could_be_transpos   = infile_info->could_be_transpos;
    dz->could_be_pitch      = infile_info->could_be_pitch;
    dz->different_srates    = infile_info->different_srates;
    dz->duplicate_snds      = infile_info->duplicate_snds;
    dz->numsize             = infile_info->numsize;
    dz->linecnt             = infile_info->linecnt;
    dz->all_words           = infile_info->all_words;
    dz->infile->arate       = infile_info->arate;
    dz->frametime           = infile_info->frametime;
    dz->infile->window_size = infile_info->window_size;
    dz->nyquist             = infile_info->nyquist;
    dz->duration            = infile_info->duration;
    dz->minbrk              = infile_info->minbrk;
    dz->maxbrk              = infile_info->maxbrk;
    dz->minnum              = infile_info->minnum;
    dz->maxnum              = infile_info->maxnum;
    if(dz->process==MIXDUMMY || dz->process==HOUSE_BUNDLE || dz->process==HOUSE_SORT 
    || dz->process==MIX_ON_GRID  || dz->process==ADDTOMIX     || dz->process==MIX_AT_STEP 
    || dz->process==BATCH_EXPAND || dz->process==MIX_MODEL)
        return FINISHED;

    return setup_brktablesizes(infile_info,dz);
}

/************************ ALLOCATE_FILESPACE *********************/

int allocate_filespace(dataptr dz)
{
    int n;
    int zerofiles = 0;          
/* BELOW CHANGED FROM < 0: JAN 2000 */
    if(dz->infilecnt <= 0) {
        zerofiles = 1;
        dz->infilecnt = 1;   /* always want to allocate pointers */
    }
    /*
    if((dz->infilesize = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for infilesize array.\n");
        return(MEMORY_ERROR);
    }
    */
    if((dz->insams = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for infile-sampsize array.\n");
        return(MEMORY_ERROR);
    }                   
    if((dz->ifd = (int *)malloc(dz->infilecnt * sizeof(int)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for infile poniters array.\n");
        return(MEMORY_ERROR);
    }                   
    for(n=0;n<dz->infilecnt;n++) {
        dz->ifd[n]        = -1;
        dz->insams[n]     = 0L;
        /*dz->infilesize[n] = 0L;*/
    }
    if(zerofiles)
        dz->infilecnt = 0;
    return(FINISHED);
}

/************************ HANDLE_EXTRA_INFILES *********************/

int handle_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
                    /* OPEN ANY FURTHER INFILES, CHECK COMPATIBILITY, STORE DATA AND INFO */
    int  exit_status;
    int  n, k = 0;
    char *filename;

    if(dz->process == ADDTOMIX || dz->process == BATCH_EXPAND || dz->process == MIX_MODEL)
        k = dz->all_words - 1;

    if(dz->infilecnt > 1) {
        for(n=1;n<dz->infilecnt;n++) {
            filename = (*cmdline)[0];
            switch(dz->process) {
            case(ADDTOMIX):
            case(BATCH_EXPAND):
            case(MIX_MODEL):
                if((exit_status = store_further_filename(n+k,filename,dz))<0)
                    return(exit_status);
                if((exit_status = handle_other_infile(n,filename,dz))<0)
                    return(exit_status);
                break;
            case(MIX_ON_GRID):
            case(MIX_AT_STEP):
                if((exit_status = store_further_filename(n,filename,dz))<0)
                    return(exit_status);
                break;

            case(HOUSE_BUNDLE):
            case(MIXDUMMY):
                if((exit_status = store_further_filename(n,filename,dz))<0)
                    return(exit_status);
                break;
            case(INFO_TIMELIST):
            case(INFO_LOUDLIST):
                if((exit_status = store_further_filename(n,filename,dz))<0)
                    return(exit_status);
                if((exit_status = handle_other_infile(n,filename,dz))<0)
                    return(exit_status);
                break;
            case(MOD_LOUDNESS):
                if(dz->mode==LOUDNESS_LOUDEST || dz->mode==LOUDNESS_EQUALISE) { 
                    if((exit_status = store_further_filename(n,filename,dz))<0)
                        return(exit_status);
                }
                if((exit_status = handle_other_infile(n,filename,dz))<0)
                    return(exit_status);
                break;
            default:
                if((exit_status = handle_other_infile(n,filename,dz))<0)
                    return(exit_status);
                break;
            }
            (*cmdline)++;
            (*cmdlinecnt)--;
        }
        if((exit_status = set_special_process_sizes(dz))<0)
            return(exit_status);
    }
    if(dz->process == BATCH_EXPAND || dz->process == MIX_MODEL)
        dz->itemcnt = dz->infilecnt - 1;
    return(FINISHED);
}

/************************ HANDLE_OUTFILE *********************/
/* RWD I have a theory: this could be split into two functions where indicated:
 *  handle_outfile_name()
 * and handle_outfile()
 * and then handle_outfile() can be relocated at the end of param_preprocess(), 
 * wher it belongs; and all these tests can be eliminated!
 */
/************************ HANDLE_OUTFILE *********************/

int handle_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz)
{
    int exit_status;
    char *filename = NULL;
    char *filename2;
    int n;

    int look_for_float_flag = 0;
    if(dz->outfiletype==NO_OUTPUTFILE) {

        /* Look at all those "NO_OUTPUTFILE" processes which actually give a sound output (!!! it's a int story!) */
        /* Some of these processes have NO output file name in the cmdline situation but DO have an outfilename in SLOOM */
        /* as all SLOOM processes have an outfilename */

        if(sloom) {
            if (dz->process == RANDCUTS
            ||  dz->process == RANDCHUNKS
            ||  dz->process == HOUSE_CHANS
            || (dz->process == HOUSE_EXTRACT && dz->mode == HOUSE_CUTGATE)) {
                look_for_float_flag = 1;
            }
        }
        /* Some of these processes DO have an output file name in the cmdline situation (and, as always, in SLOOM) */

        if (dz->process == MIXINBETWEEN
        ||  dz->process == CYCINBETWEEN
        ||  dz->process == SYNTH_SPEC
        ||  dz->process == HOUSE_GATE
        || (dz->process == HF_PERM1 && dz->mode == HFP_SNDSOUT)
        || (dz->process == HF_PERM2 && dz->mode == HFP_SNDSOUT)) {
            look_for_float_flag = 1;
        }
    } else if(dz->outfiletype == SNDFILE_OUT) {

        /* Look at all processes with a SOUNDFILE output: all such processes have an output file name in the cmdline */     
        
        look_for_float_flag = 1;
    }
    if(look_for_float_flag) {

        /* For all these cases, check if the output file is flagged as float, and strip any flag */
        
        if(!sloom && (*cmdlinecnt<=0)) {
            sprintf(errstr,"Insufficient cmdline parameters.\n");
            return(USAGE_ONLY);
        }
        filename = (*cmdline)[0];
        if(filename[0]=='-' && filename[1]=='f') {
            dz->floatsam_output = 1;
            dz->true_outfile_stype = SAMP_FLOAT;
            filename+= 2;
        }
    }
    if(dz->outfiletype==NO_OUTPUTFILE) {

        /* These processes generate an output which is NOT a soundfile, and hence have an output file name on the cmd line */

        if(dz->process == MAKE_VFILT
        || (dz->process == MOD_PITCH && (dz->mode == MOD_TRANSPOS_INFO || dz->mode == MOD_TRANSPOS_SEMIT_INFO))) {
            if(!sloom && (*cmdlinecnt<=0)) {
                sprintf(errstr,"Insufficient cmdline parameters.\n");
                return(USAGE_ONLY);
            }
            filename = (*cmdline)[0];
            if(filename[0]=='-' && filename[1]=='f') {
                sprintf(errstr,"-f flag used incorrectly on command line (output is not a sound file).\n");
                return(USAGE_ONLY);
            }
        }
    }
    if (!sloom) {
        if(dz->process == MAKE) {
            if((filename = (*cmdline)[0]) == NULL) {
                return(USAGE_ONLY);
            }
        }
        if(dz->process == MAKE || dz->process == FORMSEE) {
            if((exit_status = setup_file_outname_where_ness(filename,dz))<0) {    
                print_messages_and_close_sndfiles(exit_status,is_launched,dz);  
                return(exit_status);
            }
        }
        if (dz->process == MAKE) {
            if((filename = get_other_filename_x(filename,'1')) == NULL) {
                sprintf(errstr,"Insufficient memory to store modified outfilename.\n");
                return MEMORY_ERROR;
            }
        }
    }
    /* in all these cases we wish to remember the name of the output filename from the cmdline */

    if(dz->process == ANALENV
    || dz->process == BRASSAGE
    || dz->process == CLICK
    || dz->process == EDIT_CUTMANY 
    || dz->process == MANY_ZCUTS 
    || dz->process == ENV_CREATE 
    || dz->process == ENV_EXTRACT
    || dz->process == FMNTSEE      
    || dz->process == FORMANTS   
    || ((dz->process == FORMSEE) && sloom)
    || dz->process == HF_PERM1
    || dz->process == HF_PERM2
    || dz->process == JOIN_SEQ 
    || dz->process == JOIN_SEQDYN
    || dz->process == LEVEL
    || ((dz->process == MAKE) && sloom)
    || dz->process == P_HEAR
    || dz->process == P_SEE      
    || dz->process == P_SYNTH    
    || dz->process == P_VOWELS 
    || dz->process == PVOC_ANAL  
    || dz->process == PVOC_SYNTH
    || dz->process == SAUSAGE 
    || dz->process == SHUDDER 
    || dz->process == SYLLABS
    || dz->process == SYNTH_NOISE 
    || dz->process == SYNTH_WAVE 
    || dz->process == MULTI_SYN
    || dz->process == SYNTH_SIL 
    || dz->process == TIME_GRID 
    || dz->process == TWIXT
    || dz->process == ENVSYN
    || (dz->process == RRRR_EXTEND && dz->mode == 2) 
    || (dz->process == MOD_RADICAL && (dz->mode == MOD_LOBIT || dz->mode == MOD_LOBIT2))) {

        /* all processes with a SOUNDFILE output have already been checked, so any outfiles found here must be NON soundfiles */
        
        if(filename == NULL) {
            if(!sloom && (*cmdlinecnt<=0)) {
                sprintf(errstr,"Insufficient cmdline parameters.\n");
                return(USAGE_ONLY);
            }
            filename = (*cmdline)[0];

            if(filename[0]=='-' && filename[1]=='f') {
                sprintf(errstr,"-f flag used incorrectly on command line (output is not a sound file).\n");
                return(USAGE_ONLY);
            }
        }       
        if((exit_status = setup_file_outname_where_ness(filename,dz))<0) {    
            print_messages_and_close_sndfiles(exit_status,is_launched,dz);  
            return(exit_status);
        }
    }
    if (!sloom) {
        if (dz->process==SHUDDER || dz->process == BRASSAGE || dz->process == SAUSAGE || dz->process == FORMSEE) {
            if((filename = get_other_filename_x(filename,'1')) == NULL) {
                sprintf(errstr,"Insufficient memory to store modified outfilename.\n");
                return MEMORY_ERROR;
            }
            if((n = sndopenEx(dz->wordstor[0],0,CDP_OPEN_RDONLY)) >= 0) {
                sprintf(errstr,"Output file %s already exists",dz->wordstor[0]);
                if(sndcloseEx(n) < 0)
                    strcat(errstr,": Cannot close the file.");
                strcat(errstr,"\n");
                return(GOAL_FAILED);
            }
        }
    }
    if(sloom) {

        /* ALL sound loom processes have an output filename: so get it, if not already found */

        if(filename == NULL)
            filename = (*cmdline)[0];

        /* These cases, on the soundloom , have ANALFILE & PITCHFILE output, and need to save or use outfilename as basis for additional outfile names */

        if(dz->process==PITCH || dz->process==TRACK) {
            dz->all_words = 0;
            if((exit_status = store_filename(filename,dz))<0)
                return(exit_status);
            if((exit_status = do_extra_files(dz)) != FINISHED)
                return(exit_status); 
        }
        if(dz->process==HOUSE_COPY && dz->mode == DUPL) {
            if((exit_status = setup_file_outname_where_ness(filename,dz))<0) {    
                print_messages_and_close_sndfiles(exit_status,is_launched,dz);  
                return(exit_status);
            }
        }
    }

    if(dz->outfiletype==NO_OUTPUTFILE) {

        /* in these cases also, we wish to remember the name of the output file name on the cmdline. */
        /* In non-soundloom case, some programs use the input filename to generate the outfilename,  */
        /* whereas Sound Loom ALWAYS has an outfilename which can be used.                           */

        if(sloom) {
            if(dz->process == MIXINBETWEEN || dz->process == CYCINBETWEEN) {
                if((exit_status = read_new_filename(filename,dz))<0)
                    return(exit_status);            /* read outfile as if it were special data */
            } else if(dz->process != HF_PERM1 && dz->process != HF_PERM2) {
                if((exit_status = setup_file_outname_where_ness(filename,dz))<0)
                    return(exit_status);
            }
        } else {
            if(dz->process == MIXINBETWEEN || dz->process == CYCINBETWEEN) {
                if(file_has_invalid_startchar(filename)) {
                    sprintf(errstr,"Filename %s has invalid start character(s)\n",filename);
                    return(DATA_ERROR);
                }
                if((exit_status = read_new_filename(filename,dz))<0)
                    return(exit_status);            /* read outfile as if it were special data */
            }

            if(dz->process == SYNTH_SPEC || dz->process == MAKE_VFILT) {
                if(file_has_invalid_startchar(filename)) {
                    sprintf(errstr,"Filename %s has invalid start character(s)\n",filename);
                    return(DATA_ERROR);
                }
                if((exit_status = setup_file_outname_where_ness(filename,dz))<0)
                    return(exit_status);
            }
        }
    } else {

        /* all other processes have an outputfile but not a sound outputfile (these were checked previously) */
        /* so, if a filename does not yet exist, get it from the cmdline */
        /* (all sloom outfilenames have already been read) */
            
        if(filename == NULL) {
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
    }

    //TW mechanics to setup sloom (and cmdline) compatible temp file: + normal usage SHUDDER

    if(sloom) {
        if (dz->process==SHUDDER || dz->process == BRASSAGE || dz->process == SAUSAGE || dz->process == FORMSEE
        || dz->process == MAKE)
            get_other_filename(filename,'1');
    }

// DEC 2009 FIX
//  if(dz->process==HOUSE_BUNDLE || dz->process==INFO_DIFF /* || (sloom && dz->process==PITCH)  */
    if(dz->process==HOUSE_BUNDLE 
    || dz->process==HOUSE_DUMP
    || (dz->process == MOD_LOUDNESS && dz->mode == LOUDNESS_EQUALISE)) {
        if((exit_status = store_further_filename(dz->all_words,filename,dz))<0)
            return(exit_status);
    }

    if(!sloom) {
        if(dz->process == EDIT_CUTMANY || dz->process == SYLLABS || dz->process == MANY_ZCUTS) {
            n = strlen(filename);
            if ((filename2 = (char *)malloc(n+2))==NULL) {
                sprintf(errstr,"Insufficient memory to store output filename.\n");
                return(MEMORY_ERROR);
            }
            strcpy(filename2,filename);
            insert_new_number_at_filename_end(filename2,1,0);
            filename = filename2;
        }
    }

    /* EXCEPT IN THOSE CASES WHERE outfile creation has to be delayed, create the outfile NOW */

    if(!(
       dz->process == SYNTH_WAVE 
    || (!sloom && dz->process == HOUSE_COPY && dz->mode == DUPL)
    || dz->process == MULTI_SYN
    || dz->process == SYNTH_NOISE 
    || dz->process == SYNTH_SPEC 
    || dz->process == SYNTH_SIL
    || dz->process == HOUSE_SPEC
    || dz->process == HOUSE_CHANS
    || dz->process == MIXINTERL
    || (dz->process >= SIMPLE_TEX  && dz->process <= TMOTIFSIN)
    || (dz->process == MOD_SPACE   && dz->mode == MOD_PAN)
    || (dz->process == MOD_REVECHO && dz->mode == MOD_STADIUM)
    || (dz->process == BRASSAGE    && dz->mode == GRS_REVERB)
    || (dz->process == BRASSAGE    && dz->mode == GRS_BRASSAGE)
    || (dz->process == BRASSAGE    && dz->mode == GRS_FULL_MONTY)
    || dz->process == SAUSAGE
    || (dz->process == MIXMAX      && dz->mode == MIX_LEVEL_ONLY)
    || dz->process == AUTOMIX
    || dz->process == MIX
    || dz->process == MIXTWO
    || dz->process == ENV_EXTRACT
    || dz->process == ENV_CREATE    
    || dz->process == ENV_BRKTOENV
    || dz->process == ENV_DBBRKTOENV
    || dz->process == FORMANTS
    || dz->process == FMNTSEE
    || dz->process == FORMSEE
    || dz->process == P_SEE
    || dz->process == P_HEAR
    || dz->process == PVOC_ANAL
    || dz->process == PVOC_SYNTH
    || dz->process == P_SYNTH
    || dz->process == P_VOWELS
    || dz->process == ANALENV
    || dz->process == LEVEL
    || (dz->process == HF_PERM1 && (dz->mode == HFP_SNDOUT || dz->mode == HFP_SNDSOUT))
    || (dz->process == HF_PERM2 && (dz->mode == HFP_SNDOUT || dz->mode == HFP_SNDSOUT))
    || dz->process == DEL_PERM
    || dz->process == P_GEN
    || dz->process == CLICK
    || dz->process == MIXINBETWEEN
    || dz->process == CYCINBETWEEN
    || dz->process == SCALED_PAN
    || (dz->process == HOUSE_EXTRACT && dz->mode == HOUSE_CUTGATE)
    || dz->process == HOUSE_GATE
    || dz->process == HOUSE_SORT
//NEW DEC 2010
    || dz->process == TIME_GRID
    /* and those cases where there really is no outfile */
    ||  (( dz->process == ENV_WARPING 
        || dz->process == ENV_REPLOTTING  
        || dz->process == ENV_RESHAPING) && dz->mode == ENV_PEAKCNT)
    || (dz->process == MOD_LOUDNESS && dz->mode == LOUDNESS_LOUDEST)
    || (dz->process == MOD_PITCH && (dz->mode == MOD_TRANSPOS_SEMIT_INFO || dz->mode == MOD_TRANSPOS_INFO))
    || (dz->process == TSTRETCH && dz->mode == TSTR_LENGTH)
    || dz->process == GRAIN_ASSESS
    || dz->process == GRAIN_COUNT
    || dz->process == DISTORT_CYCLECNT
    || dz->process == FIND_PANPOS
    || dz->process == INFO_PROPS
    || dz->process == INFO_SFLEN
    || (dz->process == INFO_TIMELIST && !sloom)
    || dz->process == INFO_TIMESUM
    || dz->process == INFO_TIMEDIFF
    || dz->process == INFO_SAMPTOTIME
    || dz->process == INFO_TIMETOSAMP
    || dz->process == INFO_MAXSAMP
    || dz->process == INFO_MAXSAMP2
    || dz->process == INFO_LOUDCHAN
    || dz->process == INFO_FINDHOLE
    || dz->process == INFO_DIFF
    || dz->process == INFO_CDIFF
    || dz->process == INFO_MUSUNITS
    || dz->process == WINDOWCNT
    || dz->process == CHANNEL
    || dz->process == FREQUENCY
    || dz->process == P_INFO
    || dz->process == P_ZEROS
    || dz->process == ZCROSS_RATIO
    || dz->process == MAKE_VFILT
    || dz->process == RANDCHUNKS
    || dz->process == RANDCUTS
    || dz->process == ENVSYN
    )) {
        if((exit_status = create_sized_outfile(filename,dz))<0)
            return(exit_status);
    } 
    /* if an outfile name has been read, copy it to dz->outfilename and advance along the cmdline */

    if(filename != NULL) {
    /*RWD.7.98 - mainly for SYNTH */
    //TW also for MIX programs ETC so may as well do it universally
        strcpy(dz->outfilename,filename);      
        (*cmdline)++;
        (*cmdlinecnt)--;
    }
    return(FINISHED);
}

/****************************** PRINT_MESSAGES_AND_CLOSE_SNDFILES ******************************/

int print_messages_and_close_sndfiles(int exit_status,int is_launched,dataptr dz)
{
    int n;                          /* OUTPUT ERROR MESSAGES */
    switch(exit_status) {
    case(FINISHED):     break;
    case(PROGRAM_ERROR):    
        fprintf(stdout,"ERROR: INTERNAL ERROR: (Bug?)\n");
        splice_multiline_string(errstr,"ERROR:");
        break;
    case(SYSTEM_ERROR):     
        fprintf(stdout,"ERROR: SYSTEM ERROR\n");
        splice_multiline_string(errstr,"ERROR:");
        break;
    case(MEMORY_ERROR):     
        fprintf(stdout,"ERROR: MEMORY ERROR\n");
        splice_multiline_string(errstr,"ERROR:");
        break;
    case(USER_ERROR):       
        if(sloom) {
            fprintf(stdout,"ERROR: DATA OR RANGE ERROR\n");
            splice_multiline_string(errstr,"ERROR:");
        } else {
            fprintf(stdout,"ERROR: INCORRECT USE\n");
            splice_multiline_string(errstr,"ERROR:");
        }
        break;
    case(DATA_ERROR):       
        fprintf(stdout,"ERROR: INVALID DATA\n");
        splice_multiline_string(errstr,"ERROR:");
        break;
    case(GOAL_FAILED):      
        fprintf(stdout,"ERROR: CANNOT ACHIEVE TASK:\n");
        splice_multiline_string(errstr,"ERROR:");
        break;
    case(USAGE_ONLY):
        if(sloom) {
            fprintf(stdout,"ERROR: PROGRAM ERROR: usage messages should not be called.\n");
            fflush(stdout);
        } else
            fprintf(stdout,"%s",errstr);                                
        break;
    case(TK_USAGE):         
        if(!sloom)
            fprintf(stdout,"ERROR: PROGRAM ERROR: TK usage messages should not be called.\n");
        else
            exit_status = FINISHED;
        break;
    default:
        fprintf(stdout,"ERROR: INTERNAL ERROR: (Bug?)\n");
        fprintf(stdout,"ERROR: Unknown case in print_messages_and_close_sndfiles)\n");
        exit_status = PROGRAM_ERROR;
        break;
    }
    if(dz != NULL) {                /* CLOSE (& DELETE) SNDFILES */
        if(dz->ofd >= 0 && (exit_status!=FINISHED || !is_launched) && sndunlink(dz->ofd) < 0)
            fprintf(stdout, "ERROR: Can't set output soundfile for deletion.\n");
        if((dz->ofd >= 0) && dz->needpeaks){
            if(sndputpeaks(dz->ofd,dz->outchans,dz->outpeaks)) {
                fprintf(stdout,"WARNING: failed to write PEAK data\n");
                fflush(stdout);
            }
        }
//      if(dz->ofd >= 0  && sndcloseEx(dz->ofd) < 0) {
//          fprintf(stdout, "WARNING: Can't close output sf-soundfile : %s\n",sferrstr());
//          fflush(stdout);
//      }

        if(dz->ofd >= 0) {
            if(sndcloseEx(dz->ofd) < 0) {
                fprintf(stdout, "WARNING: Can't close output sf-soundfile : %s\n",sferrstr());
                fflush(stdout);
            }
        }
        if(dz->other_file >= 0 && exit_status!=FINISHED && sndunlink(dz->other_file) < 0) {
            fprintf(stdout, "ERROR: Can't set secondary soundfile for deletion.\n");
            fflush(stdout);
        }
        if((dz->other_file >= 0) && dz->needotherpeaks){
            if(sndputpeaks(dz->other_file,dz->otheroutchans,dz->otherpeaks)) {
                fprintf(stdout,"WARNING: failed to write PEAK data\n");
                fflush(stdout);
            }
        }
        if(dz->other_file >= 0  && sndcloseEx(dz->other_file) < 0)
            fprintf(stdout, "WARNING: Can't close secondary soundfile.\n");
        if(dz->ifd != NULL && dz->infilecnt >= 0) {
            for(n=0;n<dz->infilecnt;n++) {
              /* ALL OTHER CASES */
                if(dz->ifd[n] >= 0 && sndcloseEx(dz->ifd[n]) < 0)
                    fprintf(stdout, "WARNING: Can't close input sf-soundfile %d.\n",n+1);
            }
        }
    }
//  sffinish();
    if(sloom)
        fprintf(stdout,"END:");
    else
#ifdef unix
        fprintf(stdout,"\n\n");
#else
        fprintf(stdout,"\n");   
#endif
    fflush(stdout);
    if(exit_status != FINISHED) {
        return(FAILED);
    }
    return(SUCCEEDED);
}

/****************************** DO_EXTRA_FILES *********************************/

int do_extra_files(dataptr dz)
{
    char temp[200];
    int endword = dz->all_words - 1;
    strcpy(temp,dz->wordstor[endword]);
//TW REVISION Dec 2002
//  p = temp + strlen(temp) - 1;
//  sprintf(p,"1");
    insert_new_number_at_filename_end(temp,1,1);
    switch(dz->mode) {
    case(PICH_TO_BIN):
//TW CORRECTED Dec 2002
        if((dz->other_file = sndcreat_formatted(temp,dz->insams[0]/dz->wanted,SAMP_FLOAT,
        1,dz->infile->srate,CDP_CREATE_NORMAL)) < 0) {
            sprintf(errstr,"Cannot open output pitch file %s\n",temp);
            return(DATA_ERROR);
        }
        break;
    case(PICH_TO_BRK):        
        strip_ext(temp);
        if((dz->fp = fopen(temp,"w"))==NULL) {
            sprintf(errstr,"Cannot open file %s for output.\n",temp);
            return(DATA_ERROR);
        }
        break;
    default:
        sprintf(errstr,"Unknown mode.\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/******************************* CREATE_SIZED_OUTFILE **************************/

int create_sized_outfile(char *filename,dataptr dz)
{
    int maxsize, minsize;
    int exit_status, n, orig_chans = 1;
    int stype = SAMP_FLOAT;

    switch(dz->process) {
    case(PVOC_ANAL):
        dz->true_outfile_stype = stype;
        dz->outfilesize = -1;
        break;
    case(PVOC_SYNTH):   case(PVOC_EXTRACT):
        if(dz->floatsam_output != 1)
            stype = dz->outfile->stype;
//NOV 2005: proposed RWD fix, changes above line to PVOC
//          stype = dz->outfile->stype;
        dz->true_outfile_stype = stype;
        dz->outfilesize = -1;
        break;
    case(SYNTH_SPEC):   
        dz->outfilesize = -1;
        break;
    default:
        switch(dz->process_type) {
        case(EQUAL_SNDFILE):
            /*RWD May 2005 */
#ifdef NOTDEF
            if(dz->floatsam_output != 1)
                stype = SAMP_SHORT;
            else
                stype = dz->infile->stype;
            dz->true_outfile_stype = stype;     
#else
            if(dz->infile->stype < 0)       /* if no infile, default to shorts */
                dz->infile->stype = SAMP_SHORT;
            if(dz->floatsam_output != 1)
                stype = dz->infile->stype;     /* keep infile stype for outfile */
            else 
               stype = SAMP_FLOAT;             /* or force floats */
            dz->true_outfile_stype = stype;

#endif
            dz->outfilesize = dz->insams[0];           /* RWD watch this... */
            break;
        case(EQUAL_ENVFILE):
        case(EQUAL_ANALFILE):   case(PITCH_TO_PITCH):
            stype = dz->infile->stype;
            dz->true_outfile_stype = stype;     
            dz->outfilesize = dz->insams[0];           /* RWD watch this... */
            break;
        case(CREATE_ENVFILE):
            dz->infile->channels = 1;
            /* fallthro */
        case(EXTRACT_ENVFILE):  case(UNEQUAL_ENVFILE):
        case(BIG_ANALFILE):     case(PITCH_TO_BIGPITCH):
            dz->true_outfile_stype = stype;
            dz->outfilesize = -1;
            break;
        case(ANAL_TO_FORMANTS):
        case(EQUAL_FORMANTS):
            dz->true_outfile_stype = stype;
/*
DO FORMANT FILES HAVE TO HAVE ONE CHANNEL ????? YES
*/
            dz->outfilesize = -1;
// COMMENT dz->infile->channels (MUST BE/IS) set to correct outval, then reset, external to this call
            break;
        case(PITCH_TO_ANAL):
            dz->true_outfile_stype = stype;
            dz->outfilesize = -1;
// This reset of infile->channels is for the benefit of MAKE & MAKE2: reset is below
            orig_chans = dz->infile->channels;
            dz->infile->channels = dz->infile->origchans;
            break;
        case(UNEQUAL_SNDFILE):
/*RWD May 2005 this code back to front! */
#ifdef NOTDEF
            if(dz->floatsam_output!=1)
                stype = SAMP_SHORT;
            else
                stype = dz->infile->stype;
            dz->true_outfile_stype = stype;
            dz->outfilesize = -1;
#else
            if(dz->infile->stype < 0)       /* if no infile, default to shorts */
                dz->infile->stype = SAMP_SHORT;
            if(dz->floatsam_output!=1)
                stype = dz->infile->stype;   /* outfile has infile sample type */
            else
                stype = SAMP_FLOAT;
            dz->true_outfile_stype = stype;  /* or floats if asked for */
            dz->outfilesize = -1;


#endif
            break;
        case(PSEUDOSNDFILE):
            dz->true_outfile_stype = SAMP_SHORT;
            dz->outfilesize = -1;
// COMMENT dz->infile->channels (MUST BE/IS) set to correct outval, then reset, external to this call
            break;
        case(MAX_ANALFILE):
            maxsize = dz->insams[0];
            for(n=1;n<dz->infilecnt;n++)
                maxsize = max(maxsize,dz->insams[n]);
            dz->outfilesize = maxsize;
            break;
        case(MIN_ANALFILE):
            minsize = dz->insams[0];
            for(n=1;n<dz->infilecnt;n++)
                minsize = min(minsize,dz->insams[n]);
            dz->outfilesize = minsize;
            break;
        case(PITCH_TO_PSEUDOSND):
            dz->outfilesize = dz->wlength;         /* RWD watch this... */
// COMMENT dz->infile->channels (MUST BE/IS) set to correct outval, then reset, external to this call
            break;
        case(TO_TEXTFILE):
            if(file_has_reserved_extension(filename)) {
                sprintf(errstr,"Cannot open a textfile (%s) with a reserved extension.\n",filename);
                return(USER_ERROR);
            }
            if((dz->fp = fopen(filename,"w"))==NULL) {
                sprintf(errstr,"Cannot open output file %s\n",filename);
                return(USER_ERROR);
            }
            return FINISHED;
        default:
            sprintf(errstr,"Invalid process_type %d: create_sized_outfile()\n",dz->process_type);
            return(PROGRAM_ERROR);
        }       
        break;
    }

    /* RWD April 2005 write WAVE_EX if possible! */
    /*TODO: read channel format from input, somewhere into dz*/
    if(dz->outfiletype== SNDFILE_OUT && (dz->infile->channels > 2 || stype > SAMP_FLOAT)){
        SFPROPS props, inprops;
        dz2props(dz,&props);
        props.chans = dz->infile->channels;
        props.srate = dz->infile->srate;
        if(dz->ifd && dz->ifd[0] >=0) {
            if(snd_headread(dz->ifd[0], &inprops))    /* snd_getchanformat not working ...*/
                props.chformat = inprops.chformat;
        }
#ifdef _DEBUG
        printf("DEBUG: writing WAVE_EX outfile\n");
#endif

        dz->ofd = sndcreat_ex(filename,dz->outfilesize,&props,SFILE_CDP,CDP_CREATE_NORMAL); 
        if(dz->ofd < 0){
            sprintf(errstr,"Cannot open output file %s\n", filename);
            return(DATA_ERROR);
        }
    }
    else{
        if((dz->ofd = sndcreat_formatted(filename,dz->outfilesize,stype,
                dz->infile->channels,dz->infile->srate,CDP_CREATE_NORMAL)) < 0) {
            sprintf(errstr,"Cannot open output file %s\n", filename);
            return(DATA_ERROR);
        }
    }
    dz->outchans = dz->infile->channels;
    if((exit_status = establish_peak_status(dz))<0)
        return(exit_status);
    switch(dz->process_type) {
    case(PITCH_TO_ANAL):        /* RESET, See above */
        dz->infile->channels = orig_chans;
        break;
    }
    switch(dz->process) {
    case(PVOC_ANAL): case(PVOC_SYNTH):  case(PVOC_EXTRACT):
        dz->outfilesize = sndsizeEx(dz->ofd);
        dz->total_samps_written = 0L;
        break;
    default:
        switch(dz->process_type) {
        case(UNEQUAL_SNDFILE):  case(UNEQUAL_ENVFILE):      case(CREATE_ENVFILE):
        case(EXTRACT_ENVFILE):  case(BIG_ANALFILE):         case(ANAL_TO_FORMANTS):
        case(PITCH_TO_ANAL):    case(PITCH_TO_BIGPITCH):    case(PSEUDOSNDFILE):
        case(PITCH_TO_PSEUDOSND):
// TW THIS doesn't achieve anythuing, I think.
            dz->outfilesize = sndsizeEx(dz->ofd);
            break;
        }
        break;
    }
    if(dz->process_type != TO_TEXTFILE) {
        if((exit_status = assign_wavetype(dz)) < 0)
            return exit_status;
    }
    return(FINISHED);
}


/* RWD.7.98 sfsys98 version, for SYNTH */

//TW REDUNDANT ??
//int create_sized_outfile_formatted(const char *filename,int srate,int channels, int stype,dataptr dz)
//{
//  if((dz->ofd = sndcreat_formatted(filename,-1,stype,
//          channels,srate,CDP_CREATE_NORMAL)) < 0) {
//      sprintf(errstr,"Cannot open output file %s: %s\n", filename,rsferrstr);
//      return SYSTEM_ERROR;
//  }
//  //RWD.10.98
//  dz->true_outfile_stype = stype; 
//  dz->total_samps_written = 0L;
//  return FINISHED;
//}

//TW CREATE TEMPFILE NAME (for bad sndseek cases: and other spcial cases)
void get_other_filename(char *filename,char c)
{
    char *p, *end;
    p = filename + strlen(filename);
    end = p;
    while(p > filename) {
        p--;
        if(*p == '.')
            break;              /* return start of final name extension */
        else if(*p == '/' || *p == '\\') {
            p =  end;           /* directory path component found before finding filename extension */
            break;              /* go to end of name */
        }
    }
    if(p == filename)
        p = end;                /* no finalname extension found, go to end of name */
    p--;                        /* insert '1' at name end */
    *p = c;
    return;
}

void insert_new_number_at_filename_end(char *filename,int num,int overwrite_last_char)
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

void insert_new_chars_at_filename_end(char *filename,char *str)
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
            *p = ENDOFSTR;
            strcat(filename,str);
            strcat(filename,ext);
            return;
        }
        p--;
    }
    if(p == filename)
        strcat(filename,str);
}

void replace_filename_extension(char *filename,char *ext)
/* FUNCTIONS ASSUMES ENOUGH SPACE IS ALLOCATED !! */
{
    char *p;
    p = filename + strlen(filename) - 1;
    while(p > filename) {
        if(*p == '/' || *p == '\\' || *p == ':') {
            p = filename;
            break;
        }
        if(*p == '.') {
            *p = ENDOFSTR;
            strcat(filename,ext);
            return;
        }
        p--;
    }
    if(p == filename)
        strcat(filename,ext);
}

void delete_filename_lastchar(char *filename)
{
    char *p, *q = filename + strlen(filename);
    p = q - 1;
    while(p > filename) {
        if(*p == '/' || *p == '\\' || *p == ':') {
            p = filename;
            break;
        }
        if(*p == '.') {
            while(p <= q) {
                *(p - 1) = *p;
                p++;
            }
            return;
        }
        p--;
    }
    if(p == filename) {
        p = q - 1;
        *p = ENDOFSTR;
    }
}

int reset_peak_finder(dataptr dz)
{
    int j;
    
    if(dz->needpeaks && (dz->ofd >= 0)){
        if(sndputpeaks(dz->ofd,dz->outchans,dz->outpeaks)) {
            fprintf(stdout,"WARNING: failed to write PEAK data\n");
            fflush(stdout);
        }
    }
    if(dz->outpeaks==NULL) {
        dz->outpeaks = (CHPEAK *) malloc(sizeof(CHPEAK) * dz->outchans);
        if(dz->outpeaks==NULL)
            return MEMORY_ERROR;
        dz->outpeakpos = (unsigned int *) malloc(sizeof(unsigned int) * dz->outchans);
        if(dz->outpeakpos==NULL)
            return MEMORY_ERROR;
    }   
//JAN 2007 moved
    for(j = 0;j<dz->outchans;j++)
        dz->outpeakpos[j] = 0;
    for(j = 0;j<dz->outchans;j++) {
        dz->outpeaks[j].value = 0.0;
        dz->outpeaks[j].position = 0;
    }
    dz->needpeaks = 1;
    return FINISHED;
}

int establish_peak_status(dataptr dz)
{
    int i;

    if(dz->outpeaks!=NULL)
        return FINISHED;

    if(dz->process < FOOT_OF_GROUCHO_PROCESSES) {
        switch(dz->process) {
//sound or pseudosnd output
        case(FMNTSEE):  case(FORMSEE):  case(PVOC_SYNTH):
        case(LEVEL):    case(P_SEE):    
// 2010
        case(MTON):     case(FLUTTER):
        case(SETHARES): case(MCHSHRED):
        case(MCHZIG):   case(MCHSTEREO):
            break;
        default:
//spectral output
            dz->needpeaks = 0;
            return FINISHED;
        }
    }
//spectral output
    switch(dz->process) {   
    case(P_SYNTH):  case(P_INSERT): case(P_PTOSIL): case(P_NTOSIL): case(P_SINSERT):    
    case(P_GEN):    case(P_INTERP): case(MAKE2):    case(ANALENV):  case(FREEZE2):
    case(PVOC_ANAL):    case(MAKE): case(ENVSYN):
// 2010
    case(ANALJOIN):     case(ONEFORM_GET):  case(ONEFORM_PUT):  case(ONEFORM_COMBINE):
    case(SPEC_REMOVE):  case(SPECROSS):     case(SPECLEAN):     case(SPECTRACT):
    case(BRKTOPI):      case(SPECSLICE):
    case(TUNEVARY):                 /* RWD Nov 21, may need to add other progs here... */
        dz->needpeaks = 0;
        return FINISHED;

//processes which generate text data.
// 2010
    switch(dz->process) {
    case(PTOBRK):
    case(RMRESP):   
    case(MULTIMIX): 
        dz->needpeaks = 0;
        return FINISHED;
    }

//processes which write to intermediate temporary files: needpeaks set to 1 later.
    case(SYNTH_SPEC):   case(MIXTWO):   case(SHUDDER):      
        dz->needpeaks = 0;
        return FINISHED;
//processes which are normalised via an intermediate temporary files: needpeaks set to 1 later.
    case(MOD_RADICAL):
        if(dz->mode == MOD_LOBIT || dz->mode == MOD_LOBIT2) {
            dz->needpeaks = 0;
            return FINISHED;
        }
        break;
//processes with several outputs, where peak is reset to zero before each file is written-to
    case(HOUSE_GATE):   
    case(TIME_GRID):
        dz->needpeaks = 0;
        return FINISHED;
    case(TWIXT):
        if(dz->mode == TRUE_EDIT) {
            dz->needpeaks = 0;
            return FINISHED;
        }
        break;
//processes needing no maxsamp data
    case(HOUSE_DUMP):   
        dz->needpeaks = 0;
        return FINISHED;
    }
    dz->needpeaks = 1;
    dz->outpeaks = (CHPEAK *) malloc(sizeof(CHPEAK) * dz->outchans);
    if(dz->outpeaks==NULL)
        return MEMORY_ERROR;
    dz->outpeakpos = (unsigned int *) malloc(sizeof(unsigned int) * dz->outchans);
    if(dz->outpeakpos==NULL)
        return MEMORY_ERROR;
    for(i=0;i < dz->outchans;i++){
        dz->outpeaks[i].value = 0.0f;
        dz->outpeaks[i].position = 0;
        dz->outpeakpos[i] = 0;
    }
    return FINISHED;
}

void strip_ext(char *temp) 
{
    char *p = temp + strlen(temp) - 1;
    while(p > temp) {
        if(*p == '.') {
            *p = ENDOFSTR;
            return;
        }
        p--;
    }
}

int assign_wavetype(dataptr dz)
{
    //wavetype wtype;
    int isenv = 1;
    int dummy = 1;
    switch(dz->process) {
        case(PVOC_ANAL):
        //wtype = wt_analysis;
        break;
    case(PVOC_SYNTH):   case(PVOC_EXTRACT): case(SYNTH_SPEC):
        //wtype = wt_wave;
        break;
    case(REPITCH):
        //if(dz->mode == PTP)
        //    wtype = wt_transposition;
        //else
        //    wtype = wt_pitch;
        break;
    default:
        switch(dz->process_type) {
        case(EQUAL_SNDFILE):    
        case(UNEQUAL_SNDFILE):
        case(PSEUDOSNDFILE):
        case(PITCH_TO_PSEUDOSND):
//          wtype = wt_wave;
            break;
        case(EQUAL_ENVFILE):
        case(CREATE_ENVFILE):
        case(EXTRACT_ENVFILE):
        case(UNEQUAL_ENVFILE):
            if(sndputprop(dz->ofd,"is an envelope",(char *) &isenv,sizeof(int)) < 0){
                sprintf(errstr,"Failure to write envelope property. assign_wavetype()\n");
                return(PROGRAM_ERROR);
            }
//          wtype = wt_binenv;
            break;
        case(EQUAL_ANALFILE):
        case(BIG_ANALFILE):
        case(MAX_ANALFILE):
        case(MIN_ANALFILE):
        case(PITCH_TO_ANAL):
 //           wtype = wt_analysis;
            break;
        case(PITCH_TO_PITCH):
        case(PITCH_TO_BIGPITCH):
            if((dz->process == REPITCH && dz->mode != PTP) || dz->is_transpos) {
                if(sndputprop(dz->ofd,"is a transpos file", (char *)&dummy, sizeof(int)) < 0) {
                    sprintf(errstr,"Failure to write transposition property. assign_wavetype()\n");
                    return(PROGRAM_ERROR);
                }
//              wtype = wt_transposition;
            } else if((dz->process == P_APPROX || dz->process == P_INVERT || dz->process == P_QUANTISE || 
            dz->process == P_RANDOMISE || dz->process == P_SMOOTH || dz->process == P_VIBRATO) 
            && (dz->mode == 1)) {
                if(sndputprop(dz->ofd,"is a transpos file", (char *)&dummy, sizeof(int)) < 0) {
                    sprintf(errstr,"Failure to write transposition property. assign_wavetype()\n");
                    return(PROGRAM_ERROR);
                }
            } else if((dz->process == P_EXAG) && (dz->mode == 1 || dz->mode == 3 || dz->mode == 5)) {
                if(sndputprop(dz->ofd,"is a transpos file", (char *)&dummy, sizeof(int)) < 0) {
                    sprintf(errstr,"Failure to write transposition property. assign_wavetype()\n");
                    return(PROGRAM_ERROR);
                }
            } else {
                if(sndputprop(dz->ofd,"is a pitch file", (char *)&dummy, sizeof(int)) < 0) {
                    sprintf(errstr,"Failure to write pitch property. assign_wavetype()\n");
                    return(PROGRAM_ERROR);
                }
//              wtype = wt_pitch;
            }
            break;
        case(ANAL_TO_FORMANTS):
        case(EQUAL_FORMANTS):
            if(sndputprop(dz->ofd,"is a formant file", (char *)&dummy, sizeof(int)) < 0) {
                sprintf(errstr,"Failure to write formant property. assign_wavetype(): %s\n",sferrstr());
                return(PROGRAM_ERROR);
            }
//          wtype = wt_formant;
            break;
        default:
            sprintf(errstr,"Unknown process_type while assigning output wavetype.\n");
            return PROGRAM_ERROR;
        }
    }
//  if(sndputprop(dz->ofd,"type",(char *)&wtype,sizeof(wavetype)) < 0){
//      sprintf(errstr,"Failure to write wavetype factor. assign_wavetype()\n");
//      return(PROGRAM_ERROR);
//  }
    return FINISHED;
}

//TW MODIFY INPUT OUTFILENAME, to deal with intermediate temp files, in comdline case.
//char *get_other_filename_x(char *filename,char c)
//{
//  char *p, *nufilename;
//  int len = strlen(filename);
//  char temp[] = "_cdptemp";
//  len += 9;
//  if(((char *)nufilename = (char *)malloc(len)) == NULL)
//      return NULL;
//  strcpy(nufilename,filename);
//  strcat(nufilename,temp);
//  p = nufilename + len;
//  *p = ENDOFSTR;
//  p--;
//  *p = c;
//  return nufilename;
//}

char *get_other_filename_x(char *filename,char c)
{
    char *p, *nufilename, ext[24];
    int len = strlen(filename);
    char temp[] = "_cdptemp";
    len += 12;
    if((nufilename = (char *)malloc(len)) == NULL)
        return NULL;
    strcpy(nufilename,filename);
    ext[0] = c;
    ext[1] = ENDOFSTR;
    p = nufilename;
    while(*p != ENDOFSTR) {
        if(*p == '.') {
            strcat(ext,p);
            *p = ENDOFSTR;
            break;
        }
        p++;
    }
    strcat(nufilename,temp);
    strcat(nufilename,ext);
    return nufilename;
}

// FEB 2010 TW

void insert_separator_on_sndfile_name(char *filename,int cnt)
/* FUNCTIONS ASSUMES ENOUGH SPACE IS ALLOCATED !! */
{
    int n;
    char ext[8];
    char *p = filename, *z = filename;                  //  z will be place to insert separator
    p += strlen(filename);
    p -= 4;     //  p set to start of any 4char extension [p]..a1   OR a1[p].wav    OR a1.[p]aiff   OR [p]aaa1  OR aaa[p]aaa1   OR aaaaaa1[p].wav   OR aaaaaa1.[p]aiff
                //  if p <= start of name ... 
                //  insert separator at end               [p]..a1 --> a1_                                                   
    if(p > filename) {                          
                //  if p is at start of 4 letter extension ... 
                //  insert separator at [p]             a1[p].wav --> a1_.wav   aaaaaa1[p].wav --> aaaaaa1_.wav 

        if(!strcmp(p,".wav") || !strcmp(p,".aif")) {
            strcpy(ext,p);
            z = p;
        } else {
            p--;//  p set to start of any 5char extension aa[p]aaaa1  OR aaaaaa1[p].aiff
                //  if p less than start of name ... 
                //  insert separator at end             [p]..aaa1 --> aaa1_     
            if(p > filename) {
                //  if p is at start of 5 letter extension ... 
                //  insert separator at [p]             a1[p].aiff --> a1_.aiff aaaaaa1[p].aiff --> aaaaaa1_.aiff

                if(!strcmp(p,".aiff")) {
                    strcpy(ext,p);
                    z = p;
                //  else, no extension match
                //  insert separator at end of name     aa[p]aaaa1 --> aaaaaa1_
                }
            }
        }
    }
    if(z == filename) {
        for(n=0;n<cnt;n++)
            strcat(filename,"_");
    } else {
        for(n=0;n<cnt;n++)
            *z++ = '_';
        *z = ENDOFSTR;
    }
}
