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
 License aint with the CDP System; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <logic.h>
#include <pnames.h>
#include <formants.h>
#include <cdparams.h>
#include <globcon.h>
#include <speccon.h>
#include <filetype.h>
#include <formants.h>
#include <localcon.h>
#include <special.h>
#include <vowels2.h>
#include <modeno.h>
#include <string.h>    /* RWD */
#include <standalone.h>
#include <science.h>
#include <sfsys.h>
#include <txtucon.h>
#include <extdcon.h>
#include <modicon.h>
#include <speccon.h>
#include <science.h>
#include <pvoc.h>

#define RRRR_EXTEND (345)

#define SUBRANGE    (TRUE)

#define PROCESS_NOT_SET (-100)

static int  establish_application2(aplptr *ap);
static void initialise_application_vals2(aplptr ap);
static int  set_legal_application_structure2(aplptr ap, int process,int mode);
static int  print_dialog_box_info2(int process,int mode,int total_params,int bare_flags,aplptr ap);
static int  print_info_on_formants2(aplptr ap);
static int  names_check2(int bare_flags,aplptr ap);
static int  print_special_data_info2(aplptr ap);
static int  print_param_info2(int total_params,aplptr ap);
static int  print_flag_info2(int bare_flags,aplptr ap);
static int  init_param_default_array2(int total_params,aplptr ap);

static void superfree_application2(aplptr ap);

static int  can_specify_brkpntfile_len2(int process,int mode);
static int  do_the_parameter_display2(int display_type,char *pname,char *pname2,char *pname3,char ptype,
            double ranglo,double ranghi,double dflt,double r2lo,double r2hi,double dflt2);
static int  get_subrange2(double *lolo, double *hihi,int n,aplptr ap);

static int set_vflgs2
(aplptr ap,char *optflags,int optcnt,char *optlist,char *varflags,int vflagcnt, int vparamcnt,char *varlist);
int set_param_data(aplptr ap, int special_data,int maxparamcnt,int paramcnt,char *paramlist);
static int  establish_special_data_type2(int process,int mode, aplptr ap);
static int  setup_special_data2(int process,int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap);
static int  setup_special_data_names2(int process,int mode,aplptr ap);
static int  deal_with_formants2(int process,int mode,int channels,aplptr ap);
static int  get_param_names2(int process,int mode,int total_params,aplptr ap);
static int  get_param_ranges2
(int process,int mode,int total_params,double nyquist,float frametime,float arate,int srate,
int wlength,int insams,int channels,int wanted,
int filetype,int linecnt,double duration,aplptr ap);
static int set_param_ranges2
(int process,int mode,double nyquist,float frametime,float arate,int srate,
int wlength,int insams,int channels,int wanted,
int filetype,int linecnt,double duration,aplptr ap);
static void setup_display2(int paramno,int dtype,int subrang,double lo,double hi,aplptr ap);
static int  establish_display2(int process,int mode,int total_params,float frametime,double duration,aplptr ap);
static int  setup_flagnames2(int process,int mode,int total_flags,aplptr ap);

static int initialise_param_values2(int process,int mode,int channels,double nyquist,float frametime,
    int insams,int srate,int wanted,int linecnt,double duration,double *default_val,int filetype,aplptr ap);
static void set_formant_flags2(int process,int mode,aplptr ap)  ;                                     ;
static int establish_formant_band_ranges2(int channels,aplptr ap);
static int setup_input_param_range_stores2(int total_params,aplptr ap);
static int deal_with_special_data2(int process,int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap);
static int setup_special_data_ranges2(int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap);
static int set_legal_param_structure2(int process,int mode, aplptr ap);
static int set_legal_option_and_variant_structure2(int process,int mode,aplptr ap);

char errstr[1000];
char paramstr[6000];

//#ifdef unix
#define round(x) lround((x))
//#else
//#define round(x) cdp_round((x))
//#endif

int sloom = 1;
int sloombatch = 0;
/* was 6.2.0; RWD Dec 22, new TW fix for specfnu, default param setting */
const char* cdp_version = "6.2.1";

/******************************* MAIN/CDPARAMS *******************************/

int main(int argc, char *argv[])
{
    int     exit_status;
    int     process, mode, channels;
    double  duration, nyquist;
    float   frametime, arate;
    int    wlength, infilesize, wanted, srate, insams;
    int     filetype, linecnt, user_paramcnt, total_params, bare_flags, infilecnt;  

    aplptr ap;

    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if((exit_status = parse_indata(argc,argv,&process,&mode,&infilecnt,&filetype,&infilesize,&insams,
        &srate,&channels,&wanted,&wlength,&linecnt,&arate,&frametime,&nyquist,&duration))<0) {
        fprintf(stdout,"ERROR: %s",errstr);
        fflush(stdout);
        return(FAILED);
    }
    if(mode>0)
        mode--; /* !!!!! INTERNAL REPRESENTATION OF MODENO COUNTS FROM ZERO */

    if((exit_status = establish_application2(&ap))<0)
        return(exit_status);

    ap->accepts_conflicting_srates = does_process_accept_conflicting_srates(process);

    if((exit_status = set_legal_application_structure2(ap,process,mode))<0) {
        superfree_application2(ap);
        fprintf(stdout,"ERROR: %s\n",errstr);
        fflush(stdout);
        return(FAILED);
    }

    total_params  = ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt;
    user_paramcnt = ap->param_cnt + ap->option_cnt + ap->variant_param_cnt;
    bare_flags    = ap->vflag_cnt - ap->variant_param_cnt;

    if((exit_status = deal_with_special_data2(process,mode,srate,duration,nyquist,wlength,channels,ap))<0) {
        superfree_application2(ap);
        fprintf(stdout,"ERROR: %s\n",errstr);
        fflush(stdout);
        return(exit_status);
    }
    if((exit_status = deal_with_formants2(process,mode,channels,ap))<0) {
        superfree_application2(ap);
        fprintf(stdout,"ERROR: %s\n",errstr);
        fflush(stdout);
        return(exit_status);
    }
    if(total_params > 0) {
        if((exit_status = get_param_names2(process,mode,total_params,ap))<0) {
            superfree_application2(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
        if((exit_status = get_param_ranges2(process,mode,total_params,nyquist,frametime,arate,srate,wlength,
                                        insams,channels,wanted,filetype,linecnt,duration,ap))<0) {
            superfree_application2(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
        if((exit_status = init_param_default_array2(total_params,ap))<0) {
            superfree_application2(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
        if((exit_status = initialise_param_values2(process,mode,channels,nyquist,frametime,insams,srate,
                                                wanted,linecnt,duration,ap->default_val,filetype,ap))<0) {
            superfree_application2(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
        if((exit_status = establish_display2(process,mode,total_params,frametime,duration,ap))<0) {
            superfree_application2(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
    }
    if(bare_flags > 0) {
        if((exit_status = setup_flagnames2(process,mode,bare_flags,ap))<0) {
            superfree_application2(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
    }
    if((exit_status = print_dialog_box_info2(process,mode,user_paramcnt,bare_flags,ap))<0) {
        superfree_application2(ap);
        fprintf(stdout,"ERROR: %s",errstr);
        fflush(stdout);
        return(FAILED);
    }
    fflush(stdout);
    return(SUCCEEDED);
}

/******************************* ESTABLISH_APPLICATION2 *******************************/

int establish_application2(aplptr *ap)
{
    if((*ap = (aplptr)malloc(sizeof(struct applic)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to establish_application2\n");
        return(MEMORY_ERROR);
    }
    initialise_application_vals2(*ap);
    return(FINISHED);
}

/************************* INITIALISE_APPLICATION_VALS2 *************************/

void initialise_application_vals2(aplptr ap)
{
    ap->max_param_cnt         = (char)0;
    ap->special_data          = (char)0;
    ap->formant_flag          = (char)0;
    ap->formant_qksrch        = (char)0;
    ap->min_fbands            = MINFBANDVAL;
    ap->max_freqwise_fbands   = MINFBANDVAL;
    ap->max_pichwise_fbands   = MINFBANDVAL;
    ap->no_pichwise_formants  = FALSE;
    ap->special_range         = FALSE;
    ap->other_special_range   = FALSE;
    ap->data_in_file_only     = FALSE;
    ap->input_process_type    = PROCESS_NOT_SET;
    ap->accepts_conflicting_srates = FALSE;
    ap->param_name            = NULL;
    ap->flagname              = NULL;
    ap->special_data_name     = /* (char*)0 */ NULL; //RWD 06-16
    ap->special_data_name2    = /* (char*)0 */ NULL;
    ap->param_cnt             = (char)0;
    ap->option_cnt            = (char)0;
    ap->vflag_cnt             = (char)0;
    ap->variant_param_cnt     = (char)0;
    ap->total_input_param_cnt = (char)0;
    ap->lo                    = NULL;
    ap->hi                    = NULL;
    ap->default_val           = NULL;
    ap->lolo                  = NULL;
    ap->hihi                  = NULL;
    ap->display_type          = NULL;
    ap->has_subrange          = NULL;
    ap->param_list            = NULL;
    ap->option_list           = NULL;
    ap->variant_list          = NULL;
}   

/************************** SET_LEGAL_APPLICATION_STRUCTURE2 *****************************/

int set_legal_application_structure2(aplptr ap,int process,int mode)
{
    int exit_status;
    if((exit_status = set_legal_param_structure2(process,mode,ap))<0)
        return(exit_status);

    if((exit_status = set_legal_option_and_variant_structure2(process,mode,ap))<0)
        return(exit_status);
    return(FINISHED);
}   

/******************************* SUPERFREE_APPLICATION2 *******************************/

void superfree_application2(aplptr ap)
{
    if(ap->param_name   !=NULL)     free(ap->param_name);
    if(ap->flagname     !=NULL)     free(ap->flagname);
    if(ap->param_list   !=NULL)     free(ap->param_list);
    if(ap->option_list  !=NULL)     free(ap->option_list);
    if(ap->variant_list !=NULL)     free(ap->variant_list);
    if(ap->lo           !=NULL)     free(ap->lo);
    if(ap->hi           !=NULL)     free(ap->hi);
    if(ap->default_val  !=NULL)     free(ap->default_val);
    if(ap->lolo         !=NULL)     free(ap->lolo);
    if(ap->hihi         !=NULL)     free(ap->hihi);
    if(ap->display_type !=NULL)     free(ap->display_type);
    if(ap->has_subrange !=NULL)     free(ap->has_subrange);
    free(ap);
}

/******************************* PRINT_DIALOG_BOX_INFO2 *******************************/

int print_dialog_box_info2(int process,int mode,int user_paramcnt,int bare_flags,aplptr ap)
{
    int exit_status;

    if((exit_status = names_check2(bare_flags,ap))<0)
        return(exit_status);

    if((exit_status = can_specify_brkpntfile_len2(process,mode))<0)
        return(exit_status);
    fprintf(stdout,"%d\n",exit_status);  /* TELLS WHETHER BRKPNTFILE DATA CAN HAVE SPECIFIC LENGTH */

    if((exit_status = print_info_on_formants2(ap))<0)
        return(exit_status);

    if((exit_status = print_special_data_info2(ap))<0)
        return(exit_status);

    if((exit_status = print_param_info2(user_paramcnt,ap))<0)
        return(exit_status);

    if((exit_status = print_flag_info2(bare_flags,ap))<0)
        return(exit_status);
    fprintf(stdout,"ENDPARAMS\n");
    return(FINISHED);
}

/******************************* NAMES_CHECK2 *******************************/

int names_check2(int bare_flags,aplptr ap)
{
    int array_checked = FALSE;
    int n, m;
    int checkcnt      = 0;

    for(n=0;n<ap->max_param_cnt;n++) {
        if(ap->param_list[n]!='0') {
            if(!array_checked) {
                if(ap->param_name==NULL) {
                    sprintf(errstr,"Parameter names array not initialised: names_check2()\n");
                    return(PROGRAM_ERROR);
                } else
                    array_checked = TRUE;
            }
            if(ap->param_name[n]==NULL) {
                sprintf(errstr,"Parameter name %d not initialised: names_check2()\n",checkcnt+1);
                return(PROGRAM_ERROR);
            }
            checkcnt++;
        }
    }
    for(n=ap->max_param_cnt,m=0;m<ap->option_cnt;n++,m++) {
        if(!array_checked) {
            if(ap->param_name==NULL) {
                sprintf(errstr,"Option-Parameter names array not initialised: names_check2()\n");
                return(PROGRAM_ERROR);
            } else
                array_checked = TRUE;
        }
        if(ap->param_name[n]==NULL) {
            sprintf(errstr,"Option-Parameter names %d not initialised: names_check2()\n",ap->param_cnt+m+1);
            return(PROGRAM_ERROR);
        }
    }
    for(n=ap->max_param_cnt+ap->option_cnt,m=0;m<ap->variant_param_cnt;n++,m++) {
        if(!array_checked) {
            if(ap->param_name==NULL) {
                sprintf(errstr,"Variant-Parameter names array not initialised: names_check2()\n");
                return(PROGRAM_ERROR);
            } else
                array_checked = TRUE;
        }
        if(ap->param_name[n]==NULL) {
            sprintf(errstr,"Variant-Parameter names %d not initialised: names_check2()\n",ap->param_cnt+ap->option_cnt+m+1);
            return(PROGRAM_ERROR);
        }
    }
    array_checked = FALSE;
    for(n=ap->max_param_cnt+ap->option_cnt+ap->variant_param_cnt,m=0;m<bare_flags;n++,m++) {
        if(!array_checked) {
            if(ap->flagname==NULL) {
                sprintf(errstr,"Flag names array not initialised: names_check2()\n");
                return(PROGRAM_ERROR);
            } else
                array_checked = TRUE;
        }
        if(ap->flagname[m]==NULL) {
            sprintf(errstr,"Flag name %d not initialised: names_check2()\n",m+1);
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/**************************** PRINT_INFO_ON_FORMANTS2 *************************/

int print_info_on_formants2(aplptr ap)
{
    int exit_status;
    if(ap->formant_flag) {

        if(ap->no_pichwise_formants) {
            if((exit_status = do_the_parameter_display2
            (LINEAR,"FRQWISE_FMNT_BANDS","","",'i',ap->min_fbands,ap->max_freqwise_fbands,FBAND_DEFAULT,
                                     ap->min_fbands,ap->max_freqwise_fbands,0.0))<0)
                return(exit_status);
        } else {
            if((exit_status = do_the_parameter_display2
            (SWITCHED,"FORMANT_BANDS","PITCHWISE","FRQWISE",'i',ap->min_fbands,ap->max_pichwise_fbands,FBAND_DEFAULT,
                                     ap->min_fbands,ap->max_freqwise_fbands,FBAND_DEFAULT))<0)
                return(exit_status);
        }
    }

    if(ap->formant_qksrch) {
        if((exit_status = do_the_parameter_display2(CHECKBUTTON,"FORMANT_QUICKSEARCH","","",'i',0.0,0.0,0.0,0.0,0.0,0.0))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/******************************* PRINT_SPECIAL_DATA_INFO2 *******************************/

int print_special_data_info2(aplptr ap)
{ 
    int exit_status;
    int rangecnt = 0;
    if(ap->special_data) {
        rangecnt = 0;
        if(ap->special_range)
            rangecnt++;
        if(ap->other_special_range)
            rangecnt++;
        switch(ap->data_in_file_only) {
        case(TRUE):
            switch(rangecnt) {
            case(0):
                if((exit_status = do_the_parameter_display2(FILENAME,ap->special_data_name,"","",(char)0,
                0.0,0.0,0.0,0.0,0.0,0.0))<0)
                    return(exit_status);
                break;
            case(1):
                if((exit_status = do_the_parameter_display2(FILENAME,ap->special_data_name,"","",(char)1,
                ap->min_special,ap->max_special,0.0,0.0,0.0,0.0))<0)
                    return(exit_status);
                break;
            case(2):
                if((exit_status = do_the_parameter_display2(FILENAME,ap->special_data_name,"","",(char)2,
                ap->min_special,ap->max_special,0.0,ap->min_special2,ap->max_special2,0.0))<0)
                    return(exit_status);
                break;
            }
            break;
        case(FALSE):
            if((exit_status = do_the_parameter_display2(FILE_OR_VAL,ap->special_data_name,"","",'D',
            ap->min_special,ap->max_special,ap->default_special,0.0,0.0,0.0))<0)
                return(exit_status);
            break;
        case(FILE_OR_ZERO):
            switch(rangecnt) {
            case(0):
                if((exit_status = do_the_parameter_display2(OPTIONAL_FILE,ap->special_data_name,"","",(char)0,
                0.0,0.0,0.0,0.0,0.0,0.0))<0)
                    return(exit_status);
                break;
            case(1):
                if((exit_status = do_the_parameter_display2(OPTIONAL_FILE,ap->special_data_name,"","",(char)1,
                ap->min_special,ap->max_special,0.0,0.0,0.0,0.0))<0)
                    return(exit_status);
                break;
            case(2):
                if((exit_status = do_the_parameter_display2(OPTIONAL_FILE,ap->special_data_name,"","",(char)2,
                ap->min_special,ap->max_special,0.0,ap->min_special2,ap->max_special2,0.0))<0)
                    return(exit_status);
                break;
            }
            break;
        case(FNAM_STRING):
            if((exit_status = do_the_parameter_display2(STRING_E,ap->special_data_name,"","",(char)0,
            0.0,0.0,0.0,0.0,0.0,0.0))<0)
                return(exit_status);
            break;
        }
    }
    return(FINISHED);
}

/******************************* PRINT_PARAM_INFO2 *******************************/

int print_param_info2(int user_paramcnt,aplptr ap)
{
    int exit_status;
    int checkcnt = 0, n, m;
    double lolo, hihi;
    if(user_paramcnt<=0)
        return(FINISHED);
    if(ap->max_param_cnt > 0) {
        for(n=0;n<ap->max_param_cnt;n++) {
            if(ap->param_list[n]!='0') {
                get_subrange2(&lolo,&hihi,n,ap);
                if((exit_status = do_the_parameter_display2(ap->display_type[n],ap->param_name[n],"","",
                ap->param_list[n],ap->lo[n],ap->hi[n],ap->default_val[n],lolo,hihi,0.0))<0)
                    return(exit_status);
                checkcnt++;
            }
        }
        if(checkcnt!=ap->param_cnt) {
            sprintf(errstr,"parameter accounting problem: print_param_info2()\n");
            return(PROGRAM_ERROR);
        }
    }
    if(ap->option_cnt > 0) {
        for(n=ap->max_param_cnt,m=0;m<ap->option_cnt;n++,m++) {
            get_subrange2(&lolo,&hihi,n,ap);
            if((exit_status = do_the_parameter_display2(ap->display_type[n],ap->param_name[n],"","",
            ap->option_list[m],ap->lo[n],ap->hi[n],ap->default_val[n],lolo,hihi,0.0))<0)
                return(exit_status);
        }
    }
    if(ap->variant_param_cnt > 0) {
        for(n=ap->max_param_cnt+ap->option_cnt,m=0;m<ap->variant_param_cnt;n++,m++) {
            get_subrange2(&lolo,&hihi,n,ap);
            if((exit_status = do_the_parameter_display2(ap->display_type[n],ap->param_name[n],"","",
            ap->variant_list[m],ap->lo[n],ap->hi[n],ap->default_val[n],lolo,hihi,0.0))<0)
                return(exit_status);
        }
    }
    return(FINISHED);
}

/******************************* PRINT_FLAG_INFO2 *******************************/

int print_flag_info2(int bare_flags,aplptr ap)
{
    int m;

    if(bare_flags>0) {
        for(m=0;m<bare_flags;m++) {
            do_the_parameter_display2(CHECKBUTTON,ap->flagname[m],"","",0,0,0,0,0,0,0);
        }
    }
    return(FINISHED);
}

/******************************* PARSE_INDATA *******************************/

int parse_indata(int argc,char *argv[],int *process,int *mode,int *infilecnt,
int *filetype,int *infilesize,int *insams,int *srate,
int *channels,int *wanted,int *wlength,int *linecnt,float *arate,float *frametime,double *nyquist,double *duration)
{
    if(argc!=16) {
        sprintf(errstr,"Wrong number of params to cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[1],"%d",process)!=1) {
        sprintf(errstr,"Cannot read process number: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[2],"%d",mode)!=1) {
        sprintf(errstr,"Cannot read mode number: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[3],"%d",infilecnt)!=1) {
        sprintf(errstr,"Cannot read infilexnt: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[4],"%d",filetype)!=1) {
        sprintf(errstr,"Cannot read file type: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[5],"%d",infilesize)!=1) {
        sprintf(errstr,"Cannot read infilesize: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[6],"%d",insams)!=1) {
        sprintf(errstr,"Cannot read number of samples in infile: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[7],"%d",srate)!=1) {
        sprintf(errstr,"Cannot read srate: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[8],"%d",channels)!=1) {
        sprintf(errstr,"Cannot read channel count: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[9],"%d",wanted)!=1) {
        sprintf(errstr,"Cannot read wanted: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[10],"%d",wlength)!=1) {
        sprintf(errstr,"Cannot read wlength: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[11],"%d",linecnt)!=1) {
        sprintf(errstr,"Cannot read arate: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[12],"%f",arate)!=1) {
        sprintf(errstr,"Cannot read arate: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[13],"%f",frametime)!=1) {
        sprintf(errstr,"Cannot read nyquist: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[14],"%lf",nyquist)!=1) {
        sprintf(errstr,"Cannot read nyquist: cdparams()\n");
        return(DATA_ERROR);
    }
    if(sscanf(argv[15],"%lf",duration)!=1) {
        sprintf(errstr,"Cannot read duration: cdparams()\n");
        return(DATA_ERROR);
    }
    return FINISHED;
}

/******************************* INIT_PARAM_DEFAULT_ARRAY2 *******************************/

int init_param_default_array2(int total_params,aplptr ap)
{
    if((ap->default_val = (double *)malloc((total_params) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for param_default_array\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/****************************** DO_THE_PARAMETER_DISPLAY2 *********************************/

int do_the_parameter_display2
(int display_type,char *pname,char *pname2,char *pname3,char ptype,double ranglo,double ranghi,double dflt,double r2lo,double r2hi,double dflt2)
{
    char quoted_pname[200], quoted_pname2[200], quoted_pname3[200];

//  switch(display_type) {
//  case(LINEAR):   case(LOG):      case(PLOG): case(POWTWO):
//  case(SWITCHED): case(NUMERIC):  case(FILE_OR_VAL):
//      if(ranghi - ranglo <= FLTERR) {
//          fprintf(stdout,"ERROR: Effectively zero range encountered for parameter %s\n",pname);
//          fflush(stdout);
//          return(FAILED);
//      }
//      break;
//  }

    quoted_pname[0]  = '\0';
    quoted_pname2[0] = '\0';
    quoted_pname3[0] = '\0';
    if(strlen(pname)>0) {           /* If name exists, enclose it in double quotes */
        strcat(quoted_pname,"\"");
        strcat(quoted_pname,pname);
        strcat(quoted_pname,"\"");
    }
    if(strlen(pname2)>0) {          /* If 2nd name exists, enclose it in double quotes */
        strcat(quoted_pname2,"\"");
        strcat(quoted_pname2,pname2);
        strcat(quoted_pname2,"\"");
    }
    if(strlen(pname3)>0) {          /* If 3rd name exists, enclose it in double quotes */
        strcat(quoted_pname3,"\"");
        strcat(quoted_pname3,pname3);
        strcat(quoted_pname3,"\"");
    }

    switch(display_type) {
    case(LINEAR):   
        fprintf(stdout,"LINEAR %s %c %lf %lf %lf %lf %lf\n",
            quoted_pname,   /* param name           */
            ptype,          /* data type            */
            ranglo,         /* bottom of range      */
            ranghi,         /* top of range         */
            dflt,           /* default value        */
            r2lo,           /* bottom of subrange   */
            r2hi);          /* top of subrange      */
        break;
    case(LOG):
        fprintf(stdout,"LOG %s %c %lf %lf %lf %lf %lf\n",
            quoted_pname,   /* param name           */
            ptype,          /* data type            */
            ranglo,         /* bottom of range      */
            ranghi,         /* top of range         */
            dflt,           /* default value        */
            r2lo,           /* bottom of subrange   */
            r2hi);          /* top of subrange      */
        break;
    case(POWTWO):
        fprintf(stdout,"POWTWO %s %c %lf %lf %lf %lf %lf\n",
            quoted_pname,   /* param name           */
            ptype,          /* data type            */
            ranglo,         /* bottom of range      */
            ranghi,         /* top of range         */
            dflt,           /* default value        */
            r2lo,           /* bottom of subrange   */
            r2hi);          /* top of subrange      */
        break;
    case(PLOG):                     
        fprintf(stdout,"PLOG %s %c %lf %lf %lf %lf %lf\n",
            quoted_pname,   /* param name           */
            ptype,          /* data type            */
            ranglo,         /* bottom of range      */
            ranghi,         /* top of range         */
            dflt,           /* default value        */
            r2lo,           /* bottom of subrange   */
            r2hi);          /* top of subrange      */
        break;
    case(SWITCHED):                 
        fprintf(stdout,"SWITCHED %s %s %s %c %lf %lf %lf %lf %lf %lf\n",
            quoted_pname,   /* param name           */
            quoted_pname2,  /* switchname 1         */
            quoted_pname3,  /* switchname 2         */
            ptype,          /* data type            */
            ranglo,         /* bottom of range      */
            ranghi,         /* top of range         */
            dflt,           /* default value        */
            r2lo,           /* bottom of 2nd range  */
            r2hi,           /* top of 2nd range     */
            dflt2);         /* default in 2nd range */
        break;
    case(FILENAME): 
        if(ptype > (char)2)     /* Deals with ptype derived from ap->param_list (etc) e.g. MIXINBETWEEN */
            ptype = (char)1;    /* As ptype there refers to data type(int or float) */
                                /* While ptype here refers to number of range DISPLAYS */
        fprintf(stdout,"FILENAME %s %d %lf %lf %lf %lf\n",
            quoted_pname,   /* param name           */
            ptype,          /* ranges to display    */
            ranglo,         /* bottom of range      */
            ranghi,         /* top of range         */
            r2lo,           /* bottom of 2nd range  */
            r2hi);          /* top of 2nd range     */
        break;
    case(OPTIONAL_FILE):    
        fprintf(stdout,"OPTIONAL_FILE %s %d %lf %lf %lf %lf\n",
            quoted_pname,   /* param name           */
            ptype,          /* ranges to display    */
            ranglo,         /* bottom of range      */
            ranghi,         /* top of range         */
            r2lo,           /* bottom of 2nd range  */
            r2hi);          /* top of 2nd range     */
        break;
    case(FILE_OR_VAL):  
        fprintf(stdout,"FILE_OR_VAL %s %c %lf %lf %lf\n",
            quoted_pname,   /* param name           */
            ptype,          /* data type            */
            ranglo,         /* bottom of range      */
            ranghi,         /* top of range         */
            dflt);          /* default val          */
        break;
    case(CHECKBUTTON):      fprintf(stdout,"CHECKBUTTON %s\n",quoted_pname);    break;
    case(FILE_OR_VOWELS):   fprintf(stdout,"VOWELS %s\n",quoted_pname);
    case(GENERIC_FILENAME): fprintf(stdout,"GENERICNAME %s\n",quoted_pname);    break;
        break;
    case(NUMERIC):
        fprintf(stdout,"NUMERIC %s %c %lf %lf %lf\n",
            quoted_pname,   /* param name       */
            ptype,          /* data type        */
            ranglo,         /* bottom of range  */
            ranghi,         /* top of range     */
            dflt);          /* default value    */
        break;
    case(LOGNUMERIC):
        fprintf(stdout,"LOGNUMERIC %s %c %lf %lf %lf\n",
            quoted_pname,   /* param name       */
            ptype,          /* data type        */
            ranglo,         /* bottom of range  */
            ranghi,         /* top of range     */
            dflt);          /* default value    */
        break;
    case(SRATE):
        fprintf(stdout,"SRATE_GADGET %s %lf\n",
            quoted_pname,   /* param name       */
            dflt);          /* default value    */
        break;
    case(MIDI):
        fprintf(stdout,"MIDI_GADGET %s %lf\n",
            quoted_pname,   /* param name       */
            dflt);          /* default value    */
        break;
    case(OCTAVES):
        fprintf(stdout,"OCT_GADGET %s %lf\n",
            quoted_pname,   /* param name       */
            dflt);          /* default value    */
        break;
    case(TWOFAC):
        fprintf(stdout,"TWOFAC %s %lf\n",
            quoted_pname,   /* param name       */
            dflt);          /* default value    */
        break;
    case(STRING_E):         
        fprintf(stdout,"STRING_E %s\n",quoted_pname);
        break;
    default:
        sprintf(errstr,"Unknown parameter type %d: do_the_parameter_display2()\n",display_type);
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
} 

/******************************* GET_SUBRANGE2 *******************************/

int get_subrange2(double *lolo, double *hihi,int n,aplptr ap)
{
    if(ap->has_subrange[n]) {
        *lolo = ap->lolo[n];
        *hihi = ap->hihi[n];
    } else {
        *lolo = ap->lo[n];
        *hihi = ap->hi[n];
    }
    return(FINISHED);
}

/****************************** CAN_SPECIFY_BRKPNTFILE_LEN2 *********************************/

int can_specify_brkpntfile_len2(int process,int mode)
{                        
    switch(process) {
    case(TAPDELAY):
    case(RMRESP):
    case(RMVERB):
    case(MIXMULTI):
    case(ANALJOIN):
    case(PTOBRK):
    case(ONEFORM_GET):
    case(ONEFORM_PUT):
    case(ONEFORM_COMBINE):
    case(PSOW_INTERP):
    case(PSOW_SYNTH):
    case(PSOW_IMPOSE):
    case(PSOW_SPLIT):
    case(PSOW_REPLACE):
    case(PSOW_LOCATE):
    case(PSOW_CUT):
    case(NEWGATE):
    case(SPEC_REMOVE):
    case(PREFIXSIL):
    case(PSOW_REINF):
    case(PARTIALS_HARM):
    case(SPECROSS):
    case(LUCIER_GETF):
    case(LUCIER_GET):
    case(SPECLEAN):
    case(SPECTRACT):
    case(PHASE):
    case(BRKTOPI):
    case(SPECSLICE):
    case(GREV_EXTEND):
    case(PEAKFIND):
    case(CONSTRICT):
    case(EXPDECAY):
    case(TEX_MCHAN):
    case(HOVER):
    case(HOVER2):
    case(MULTIMIX):
    case(SEARCH):
    case(MCHANREV):
    case(MCHSTEREO):
    case(MTON):
    case(ABFPAN):
    case(ABFPAN2):
    case(ABFPAN2P):
    case(CHANNELX):
    case(CHORDER):
    case(FMDCODE):
    case(CHXFORMAT):
    case(CHXFORMATG):
    case(CHXFORMATM):
    case(INTERLX):
    case(COPYSFX):
    case(NJOIN):
    case(NJOINCH):
    case(NMIX):
    case(SFEXPROPS):
    case(SETHARES):
    case(MCHSHRED):
    case(MCHZIG):
    case(SPECGRIDS):
    case(ISOLATE):
    case(REJOIN):
    case(PANORAMA):
    case(ECHO):
    case(PACKET):
    case(TRANSIT):
    case(TRANSITF):
    case(TRANSITD):
    case(TRANSITFD):
    case(TRANSITS):
    case(TRANSITL):
    case(CANTOR):
    case(SHRINK):
    case(CERACU):
    case(SHIFTER):
    case(SUBTRACT):
    case(SPEKLINE):
    case(FILTRAGE):
    case(SELFSIM):
    case(ITERFOF):
    case(PULSER):
    case(PULSER2):  
    case(PULSER3):
    case(CHIRIKOV):
    case(MULTIOSC):
    case(SYNFILT):
    case(STRANDS):
    case(REFOCUS):
    case(CHANPHASE):
    case(SILEND):
    case(SPECULATE):
    case(SPECTUNE):
    case(REPAIR):
    case(DISTSHIFT):
    case(QUIRK):
    case(SPECFOLD):
    case(TESSELATE):
    case(WAVEFORM):
    case(DVDWIND):
    case(SPLINTER):
    case(MOTOR):
    case(STUTTER):
    case(SCRUNCH):
    case(IMPULSE):
    case(RRRR_EXTEND):  // version 8+
    case(FLATTEN):
    case(BOUNCE):
    case(TOSTEREO):
    case(SUPPRESS):
    case(CALTRAIN):
    case(SPECENV):
    case(CLIP):
    case(SPECEX):
    case(MATRIX):
    case(SPECINVNU):
    case(SPECCONV):
    case(FRACTAL):
    case(FRACSPEC):
    case(SPECSND):
    case(SPECFRAC):
    case(ENVSCULPT):
    case(TREMENV):
    case(DCFIX):
        return(FALSE);
    case(PSOW_STRETCH):
    case(PSOW_DUPL):
    case(PSOW_DEL):
    case(PSOW_STRFILL):
    case(PSOW_FREEZE):
    case(PSOW_CHOP):
    case(PSOW_FEATURES):
    case(PSOW_SPACE):
    case(PSOW_INTERLEAVE):
    case(PSOW_EXTEND):
    case(PSOW_EXTEND2):
    case(LUCIER_PUT):
    case(LUCIER_DEL):
    case(FLUTTER):
    case(FOFEX_EX):
    case(FOFEX_CO):
    case(PEAKCHOP):
    case(MANYSIL):
    case(WRAPPAGE):
    case(RMSINFO):
    case(MCHITER):
    case(SUPERACCU):
    case(PARTITION):
    case(GLISTEN):
    case(TUNEVARY):
    case(TREMOLO):
    case(SYNTHESIZER):
    case(NEWTEX):
    case(TAN_ONE):
    case(TAN_TWO):
    case(TAN_SEQ):
    case(TAN_LIST):
    case(SPECTWIN):
    case(MADRID):
    case(FRACTURE):
    case(SPECSPHINX):
    case(NEWDELAY):
    case(ITERLINE):
    case(ITERLINEF):
    case(SPECRAND):
    case(SPECSQZ):
    case(ROTOR):
    case(DISTCUT):
    case(ENVCUT):
    case(BROWNIAN):
    case(SPIN):
    case(SPINQ):
    case(CRUMBLE):
    case(PHASOR):
    case(CRYSTAL):
    case(CASCADE):
    case(SYNSPLINE):
    case(REPEATER):
    case(VERGES):
    case(TWEET):
    case(SORTER):
    case(DISTMARK):
    case(DISTREP):
    case(TRANSPART):
    case(ENVSPEAK):
    case(EXTSPEAK):
        return(TRUE);
    case(SPECMORPH):
        switch(mode) {
        case(4):
        case(5):
            return(TRUE);
        default:
            return(FALSE);
        }
        break;
    case(SPECMORPH2):
        switch(mode) {
        case(1):
        case(2):
            return(TRUE);
        default:
            return(FALSE);
        }
        break;
    case(FRAME):
        switch(mode) {
        case(0):
        case(1):
        case(6):
            return(TRUE);
        default:
            return(FALSE);
        }
        break;
    case(RETIME):
        switch(mode) {
        case(1):
        case(4):
            return(TRUE);
        default:
            return(FALSE);
        }
        break;
    case(MCHANPAN):
        switch(mode) {
        case(2):
        case(3):
        case(5):
        case(7):
        case(8):
        case(9):
            return(TRUE);
        default:
            return(FALSE);
        }
        break;
    case(STRANS):
        switch(mode) {
        case(0):
        case(1):
        case(3):
            return(TRUE);
        case(2):
            return(FALSE);
        }
        break;
    case(SPECFNU):
        switch(mode) {
        case(F_SUPPRESS): // fall thro
        case(F_MAKEFILT):
            return(FALSE);
        default:
            return(TRUE);
        }
        break;
    default:
        sprintf(errstr,"Unknown case in can_specify_brkpntfile_len2()\n");
        return(PROGRAM_ERROR);
    }
    return(FALSE);
}

/****************************** SET_LEGAL_PARAM_STRUCTURE2 *********************************/

int set_legal_param_structure2(int process,int mode, aplptr ap)
{
                                        /*  |                             |m| |        | */
                                        /*  |                             |a| |        | */
                                        /*  |                             |x| |        | */
                                        /*  |                             |p|p|        | */
                                        /*  |                             |a|a|        | */
                                        /*  |  special-data               |r|r| param  | */
                                        /*  |                             |a|a|  list  | */
                                        /*  |                             |m|m|        | */
                                        /*  |                             |c|c|        | */
                                        /*  |                             |n|n|        | */
                                        /*  |                             |t|t|        | */

    switch(process) {
    case(TAPDELAY): return set_param_data(ap,TAPDELAY_DATA                ,4,4,  "dDDd"       );
    case(RMRESP):   return set_param_data(ap,0                            ,11,11,"diddddddddd");
    case(RMVERB):   return set_param_data(ap,TAPDELAY_OPTION              ,7,7,  "idddddd"    );
    case(MIXMULTI): return set_param_data(ap,0                            ,0,0,  ""           );
    case(ANALJOIN): return set_param_data(ap,0                            ,0,0,  ""           );
    case(PTOBRK):   return set_param_data(ap,0                            ,1,1,  "d"          );
    case(PSOW_STRETCH): return set_param_data(ap,0                        ,3,3,  "DDi"        );
    case(PSOW_DUPL):    return set_param_data(ap,0                        ,3,3,  "DIi"        );
    case(PSOW_DEL):     return set_param_data(ap,0                        ,3,3,  "DIi"        );
    case(PSOW_STRFILL): return set_param_data(ap,0                        ,4,4,  "DDiD"       );
    case(PSOW_FREEZE):  return set_param_data(ap,0                        ,8,8,  "DddiDDDd"   );
    case(PSOW_CHOP):    return set_param_data(ap,0                        ,2,2,  "DD"         );
    case(PSOW_INTERP):  return set_param_data(ap,0                        ,7,7,  "dddDDDD"    );
    case(PSOW_FEATURES):return set_param_data(ap,0                        ,11,11,"DiDDDDDdIDI");
    case(PSOW_SYNTH): 
        switch(mode) {
        case(0):
        case(1):        return set_param_data(ap,SYNTHBANK                ,2,2,  "DD"         );
        case(2):
        case(3):        return set_param_data(ap,TIMEVARYING_SYNTHBANK    ,2,2,  "DD"         );
        case(4):        return set_param_data(ap,0                        ,2,2,  "DD"         );
        }
        break;
    case(PSOW_IMPOSE):  return set_param_data(ap,0                        ,4,4,  "DDdd"       );
    case(PSOW_SPLIT):   return set_param_data(ap,0                        ,4,4,  "Didd"       );
    case(PSOW_SPACE):   return set_param_data(ap,0                        ,5,5,  "DiDDD"      );
    case(PSOW_INTERLEAVE): return set_param_data(ap,0                     ,6,6,  "DDiDDD"     );
    case(PSOW_REPLACE): return set_param_data(ap,0                        ,3,3,  "DDi"        );
    case(PSOW_EXTEND):  return set_param_data(ap,0                        ,8,8,  "DddiDDDD"   );
    case(PSOW_EXTEND2): return set_param_data(ap,0                        ,6,6,  "dddDDI"     );
    case(PSOW_LOCATE):  return set_param_data(ap,0                        ,2,2,  "Dd"         );
    case(PSOW_CUT):     return set_param_data(ap,0                        ,2,2,  "Dd"         );
    case(ONEFORM_GET):  return set_param_data(ap,0                        ,1,1,  "d"          );
    case(ONEFORM_PUT):  return set_param_data(ap,0                        ,0,0,  ""           );
    case(ONEFORM_COMBINE):  return set_param_data(ap,0                    ,0,0,  ""           );
    case(NEWGATE):      return set_param_data(ap,0                        ,1,1,  "d"          );
    case(SPEC_REMOVE):  return set_param_data(ap,0                        ,4,4,  "dddD"       );
    case(PREFIXSIL):    return set_param_data(ap,0                        ,1,1,  "d"          );
    case(STRANS):
        switch(mode) {
            case(0):
            case(1):    return set_param_data(ap,0                        ,1,1,  "D"          );
            case(2):    return set_param_data(ap,0                        ,2,2, "dd"          );
            case(3):    return set_param_data(ap,0                        ,2,2, "DD"          );
        }
        break;
    case(SPECROSS):     return set_param_data(ap,0                        ,9,9,  "dididdddD"  );
    case(PSOW_REINF):   
        switch(mode) {
            case(0):    return set_param_data(ap,PSOW_REINFORCEMENT       ,1,1,  "D"          );
            case(1):    return set_param_data(ap,PSOW_INHARMONICS         ,1,1,  "D"          );
        }
        break;
    case(PARTIALS_HARM):
        switch(mode) {
            case(0):
            case(1):    return set_param_data(ap,0                        ,2,2,  "dd"         );
            case(2):
            case(3):    return set_param_data(ap,0                        ,3,3,  "ddd"        );
        }
        break;
    case(LUCIER_GETF):  return set_param_data(ap,0                         ,2,2,"dd"          );
    case(LUCIER_GET):   return set_param_data(ap,0                         ,2,2,"dd"          );
    case(LUCIER_PUT):   return set_param_data(ap,0                         ,2,2,"ID"          );
    case(LUCIER_DEL):   return set_param_data(ap,0                         ,1,1,"D"           );
    case(SPECTRACT):
    case(SPECLEAN):     return set_param_data(ap,0                         ,2,2,"dd"          );
    case(PHASE):        return set_param_data(ap,0                         ,0,0,""            );
    case(BRKTOPI):      return set_param_data(ap,0                         ,0,0,""            );
    case(SPECSLICE):
        switch(mode) {
            case(0):    return set_param_data(ap,0                        ,2,2,  "iI"         );
            case(1):
            case(2):    return set_param_data(ap,0                        ,2,2,  "iD"         );
            case(3):    return set_param_data(ap,P_BRK_DATA               ,0,0,  ""           );
            case(4):    return set_param_data(ap,0                        ,1,1,  "D"          );
        }
        break;
    case(FOFEX_EX):
        switch(mode) {
        case(0):        return set_param_data(ap,FOFEX_EXCLUDES           ,3,3,"Ddi"          );
        case(1):        return set_param_data(ap,0                        ,3,3,"Ddi"          );
        case(2):        return set_param_data(ap,FOFEX_EXCLUDES           ,3,3,"Ddi"          );

        }
        break;
    case(FOFEX_CO):
        switch(mode) {
        case(FOF_SINGLE):
            return set_param_data(ap,FOFBANK_INFO   ,10,4,"DDdi000000");
            break;
        case(FOF_SUM):
        case(FOF_LOSUM):
        case(FOF_MIDSUM):
        case(FOF_HISUM):
            return set_param_data(ap,FOFBANK_INFO   ,10,3,"DDd0000000");
        case(FOF_LOHI):
            return set_param_data(ap,FOFBANK_INFO   ,10,7,"DDdiidd000");
            break;
        case(FOF_TRIPLE):
            return set_param_data(ap,FOFBANK_INFO   ,10,10,"DDdiiidddd");
            break;
        case(FOF_MEASURE):
            return set_param_data(ap,FOFBANK_INFO ,10,0,"0000000000");
            break;
        }
        break;
    case(GREV_EXTEND): return set_param_data(ap,0,5,5,"ddddd");
    case(PEAKFIND):    return set_param_data(ap,0,1,1,"D");
    case(CONSTRICT):   return set_param_data(ap,0,1,1,"D");
    case(EXPDECAY):    return set_param_data(ap,0,2,2,"dd");
    case(PEAKCHOP):
        switch(mode) {
        case(0):       return set_param_data(ap,0,5,5,"dDDDD");
        case(1):       return set_param_data(ap,0,5,3,"dDD00");
        case(2):       return set_param_data(ap,RHYTHM,5,5,"dDDDD");
        }
        break;
    case(MCHANPAN):
        switch(mode) {
        case(0):       return set_param_data(ap,MCHANDATA,1,1,"i");
        case(1):       return set_param_data(ap,MCHANDATA2,1,1,"i");
        case(2):       return set_param_data(ap,0,6,6,"iDDDDd");
        case(3):       return set_param_data(ap,0,5,5,"iiDDD");
        case(4):       return set_param_data(ap,ANTIPHON,2,2,"id");
        case(5):       return set_param_data(ap,ANTIPHON,4,4,"iDDd");
        case(6):       return set_param_data(ap,CROSSPAN,1,1,"D");
        case(7):       return set_param_data(ap,0,2,2,"DD");
        case(8):       return set_param_data(ap,0,4,4,"iiDd");
        case(9):       return set_param_data(ap,0,1,1,"i");
        }
        break;
    case(TEX_MCHAN):   return set_param_data(ap,TEX_NOTEDATA,26,13,"dDDDIIDDDDDDi0000000000000");
    case(MANYSIL):     return set_param_data(ap,MANYSIL_DATA,1,1,"d");
    case(RETIME):
        switch(mode) {
        case(0):       return set_param_data(ap,RETIME_DATA,1,1,"d");
        case(1):       return set_param_data(ap,IDEAL_DATA,3,3,"dDd");
        case(2):       return set_param_data(ap,0,4,4,"dddd");
        case(3):       return set_param_data(ap,0,3,3,"ddd");
        case(4):       return set_param_data(ap,0,2,2,"Dd");
        case(5):       return set_param_data(ap,RETEMPO_DATA,4,4,"dddd");
        case(6):       return set_param_data(ap,RETEMPO_DATA,3,3,"ddd");
        case(7):       return set_param_data(ap,0,5,5,"ddiid");
        case(8):       return set_param_data(ap,RETIME_MASK,1,1,"d");
        case(9):       return set_param_data(ap,0,2,2,"dd");
        case(10):      return set_param_data(ap,0,1,1,"d");
        case(11):      return set_param_data(ap,RETIME_FNAM,0,0,"");
        case(12):      return set_param_data(ap,0,1,1,"d");
        case(13):      return set_param_data(ap,0,2,2,"dd");
        }
        break;
    case(HOVER):       return set_param_data(ap,0,6,6,"DDDDdd");
    case(HOVER2):      return set_param_data(ap,0,5,5,"DDDDd");
    case(MULTIMIX):
        switch(mode) {
        case(0):       return set_param_data(ap,0,0,0,"");
        case(1):       return set_param_data(ap,0,0,0,"");
        case(2):       return set_param_data(ap,0,1,1,"d");
        case(3):       return set_param_data(ap,0,1,1,"d");
        case(4):       return set_param_data(ap,0,4,4,"dddd");
        case(5):       return set_param_data(ap,0,0,0,"");
        case(6):       return set_param_data(ap,0,2,2,"ii");
        case(7):       return set_param_data(ap,0,1,1,"i");
        }
        break;
    case(FRAME):
        switch(mode) {
        case(0):       return set_param_data(ap,FRAMEDATA,1,1,"D");
        case(1):       return set_param_data(ap,FRAMEDATA,2,2,"DD");
        case(2):       return set_param_data(ap,FRAMEDATA,0,0,"");
        case(3):       return set_param_data(ap,0,1,1,"d");
        case(4):       return set_param_data(ap,0,0,0,"");
        case(5):       return set_param_data(ap,0,2,2,"ii");
        case(6):       return set_param_data(ap,FRAMEDATA,1,1,"D");
        case(7):       return set_param_data(ap,0,0,0,"");
        }
        break;
    case(SEARCH):      return set_param_data(ap,0,0,0,"");
    case(MCHANREV):    return set_param_data(ap,0,7,7,"dddiidd");
    case(WRAPPAGE):    return set_param_data(ap,WRAP_FOCUS,20,20,"iDDDDDDDDDDDDDDDDDDd");
    case(MCHSTEREO):   return set_param_data(ap,OCHANDATA,2,2,"id");
    case(MTON):        return set_param_data(ap,0,1,1,"i");
    case(FLUTTER):     return set_param_data(ap,FLUTTERDATA,3,3,"DDd");
    case(ABFPAN):      return set_param_data(ap,0,2,2,"dd");
    case(ABFPAN2):     return set_param_data(ap,0,2,2,"dd");
    case(ABFPAN2P):    return set_param_data(ap,0,2,2,"dd");
    case(CHANNELX):    return set_param_data(ap,CHANXDATA,0,0,"");
    case(CHORDER):     return set_param_data(ap,CHORDATA,0,0,"");
    case(FMDCODE):     return set_param_data(ap,0,1,1,"i");
    case(CHXFORMAT):   return set_param_data(ap,0,0,0,"");
    case(CHXFORMATG):  return set_param_data(ap,0,0,0,"");
    case(CHXFORMATM):  return set_param_data(ap,0,0,0,"");
    case(INTERLX):     return set_param_data(ap,0,1,1,"i");
    case(COPYSFX):     return set_param_data(ap,0,0,0,"");
    case(NJOIN):       return set_param_data(ap,0,0,0,"");
    case(NJOINCH):     return set_param_data(ap,0,0,0,"");
    case(NMIX):        return set_param_data(ap,0,0,0,"");
    case(RMSINFO):     return set_param_data(ap,0,2,2,"dd");
    case(SFEXPROPS):   return set_param_data(ap,0,0,0,"");
    case(SETHARES):    return set_param_data(ap,0,5,5,"ddddd");
    case(MCHSHRED):
        switch(mode) {
        case(0):       return set_param_data(ap,0,4,4,"iddi");
        case(1):       return set_param_data(ap,0,4,3,"idd0");
        }
        break;
    case(MCHZIG):
        switch(mode) {
        case(0):       return set_param_data(ap,0,5,5,"ddddi");
        case(1):       return set_param_data(ap,ZIGDATA,5,1,"0000i");
        }
        break;
    case(MCHITER):
        switch(mode) {
        case(0):       return set_param_data(ap,0,2,2,"id");
        case(1):       return set_param_data(ap,0,2,2,"ii");
        }
        break;
    case(SPECSPHINX):  return set_param_data(ap,0,0,0,"");
    case(NEWDELAY):
        switch(mode) {
        case(0):       return set_param_data(ap,0,3,3,"Ddd");
        case(1):       return set_param_data(ap,0,3,3,"ddd");
        }
        break;
    case(ITERLINE):    return set_param_data(ap,ITERTRANS,8,7,"dDDDD0di");  
    case(ITERLINEF):   return set_param_data(ap,ITERTRANSF,8,7,"dDDDD0di"); 
    case(SPECRAND):    return set_param_data(ap,0         ,0,0,"");
    case(SPECSQZ):     return set_param_data(ap,0         ,2,2,"DD");
    case(FILTRAGE):
        switch(mode) {
        case(0):       return set_param_data(ap,0   ,11,9, "diddddddd00");
        case(1):       return set_param_data(ap,0   ,11,11,"diddddddddd");
        }
        break;
    case(SELFSIM):     return set_param_data(ap,0,1,1,"i");
    case(ITERFOF):     return set_param_data(ap,0,2,2,"Dd");
    case(PULSER):
        switch(mode) {
        case(0):       return set_param_data(ap,0,10,10,"dDddddddDD");
        case(2):       return set_param_data(ap,SPACEDATA,10,9,"d0ddDDddDD");
        default:       return set_param_data(ap,0,10,9, "d0ddddddDD");
        }
        break;
    case(PULSER2):
        switch(mode) {
        case(0):       return set_param_data(ap,0,10,10,"dDddddddDD");
        case(2):       return set_param_data(ap,SPACEDATA,10,9,"d0ddDDddDD");
        default:       return set_param_data(ap,0,10,9, "d0ddDDddDD");
        }
        break;
    case(PULSER3):     return set_param_data(ap,SYN_SPEK,10,10,"dDddddddDD");
    case(CHIRIKOV):
        switch(mode) {
        case(0): 
        case(1):       return set_param_data(ap,0,5,5,"dDDid");
        case(2): 
        case(3):       return set_param_data(ap,0,7,7,"dDDDDDD");
        }
        break;
    case(MULTIOSC):
        switch(mode) {
        case(0):       return set_param_data(ap,0,10,6, "dDDD0000id");
        case(1):       return set_param_data(ap,0,10,8, "dDDDDD00id");
        case(2):       return set_param_data(ap,0,10,10,"dDDDDDDDid");
        }
        break;
    case(SYNFILT):
        switch(mode) {
        case(0):       return set_param_data(ap,SYN_FILTERBANK,        6,6,"iiDidi");
        case(1):       return set_param_data(ap,TIMEVARYING_FILTERBANK,6,6,"iiDidi");
        }
        break;
    case(SPECMORPH):
        switch(mode) {
        case(6):       return set_param_data(ap,0     ,6,2,"0000ii");
        default:       return set_param_data(ap,0     ,6,5,"ddddi0");
        }
        break;
    case(SPECMORPH2):
        switch(mode) {
        case(0):       return set_param_data(ap,0     ,6,1,"0000i0");
        default:       return set_param_data(ap,MPEAKS,6,4,"0dddi0");
        }
        break;
    case(SUPERACCU):
        if(mode > 1)
            return set_param_data(ap,TUNING,0,0,"");
        else
            return set_param_data(ap,0,0,0,"");
        break;
    case(PARTITION):
        switch(mode) {
        case(0):       return set_param_data(ap,0,2,2,"ii");
        case(1):       return set_param_data(ap,0,2,2,"id");
        }
        break;
    case(SPECGRIDS):   return set_param_data(ap,0,2,2,"ii");
    case(GLISTEN):     return set_param_data(ap,0,2,2,"II");
    case(TUNEVARY):    return set_param_data(ap,TUNELOW_DATA,0,0,"");
    case(ISOLATE):
        switch(mode) {
        case(ISO_SEGMNT):      return set_param_data(ap,ISOLATES, 2,0,"00");
        case(ISO_GROUPS):      return set_param_data(ap,ISOGROUPS,2,0,"00");
        case(ISO_THRESH):      return set_param_data(ap,0        ,2,2,"DD");
        case(ISO_SLICED):      return set_param_data(ap,ISOSLICES,2,0,"00");
        case(ISO_OVRLAP):      return set_param_data(ap,ISOSYLLS, 2,0,"00");
        }
        break;
    case(REJOIN):      return set_param_data(ap,0, 0,0,"");
    case(PANORAMA):
        switch(mode) {
        case(0):       return set_param_data(ap,0,         5,5,"idddi");
        case(1):       return set_param_data(ap,PANOLSPKRS,5,3,"00ddi");
        }
        break;
    case(TREMOLO):     return set_param_data(ap,0,4,4,"DDDi");
    case(ECHO):        return set_param_data(ap,0,3,3,"DDd" );
    case(PACKET):      return set_param_data(ap,PAK_TIMES,3,3,"ddd");
    case(SYNTHESIZER):
        switch(mode) {
            case(0):
            case(1):   return set_param_data(ap,SYN_PARTIALS,3,3,"idD"   );
            case(2):   return set_param_data(ap,SYN_PARTIALS,6,6,"idDiiD");
            case(3):   return set_param_data(ap,0,           9,9,"idDididDD");
        }
        break;
    case(NEWTEX):
        switch(mode) {
            case(0):   return set_param_data(ap,NTEX_TRANPOS,9,5,"diDDi0000");
            case(1):   return set_param_data(ap,0           ,9,6,"diDDid000");
            case(2):   return set_param_data(ap,0           ,9,8,"diDDi0DDD");
        }
        break;
    case(CERACU):      return set_param_data(ap,CYCLECNTS,5,5,"diddi");
    case(MADRID):       
        switch(mode) {
            case(0):       return set_param_data(ap,0,6,6,"diiDDD");
            case(1):       return set_param_data(ap,MAD_SEQUENCE,6,6,"diiDDD");
        }
        break;
    case(SHIFTER):     return set_param_data(ap,SHFCYCLES,7,7,"ddiiiid");
    case(SUBTRACT):    return set_param_data(ap,0,0,0,"");
    case(SPEKLINE):
        switch(mode) {
            case(0):   return set_param_data(ap,SPEKLDATA,12,12,"iididddddddd");
            case(1):   return set_param_data(ap,SPEKLDATA,12,8, "0id00dddd0dd");
        }
        break;
    case(FRACTURE):
        switch(mode) {
            case(0):   return set_param_data(ap,ENVSERIES,9,5,"iiDDD0000");
            case(1):   return set_param_data(ap,ENVSERIES,9,9,"iiDDDiDDD");
        }
        break;
    case(TAN_ONE):     return set_param_data(ap,0,5,4,"didd0");
    case(TAN_TWO):     return set_param_data(ap,0,5,5,"diddd");
    case(TAN_SEQ):     return set_param_data(ap,0,5,3,"d0dd0");
    case(TAN_LIST):    return set_param_data(ap,0,5,3,"d0dd0");
    case(SPECTWIN):    return set_param_data(ap,0,0,0,"");
    case(TRANSIT):     return set_param_data(ap,0,6,5,"ddidd0");
    case(TRANSITF):    return set_param_data(ap,0,6,6,"ddiddd");
    case(TRANSITD):    return set_param_data(ap,0,6,5,"ddidd0");
    case(TRANSITFD):   return set_param_data(ap,0,6,6,"ddiddd");
    case(TRANSITS):    return set_param_data(ap,0,6,4,"dd0dd0");
    case(TRANSITL):    return set_param_data(ap,0,6,4,"dd0dd0");
    case(CANTOR):       
        switch(mode) {
        case(0):
        case(1):      return set_param_data(ap,0,5,5,"ddddd");
        case(2):      return set_param_data(ap,0,5,5,"diidd");
        }
        break;
    case(SHRINK):
        switch(mode) {
        case(SHRM_LISTMX):
            return set_param_data(ap,SHRFOC,6,5,"0ddddd");
            break;
        case(SHRM_TIMED):
            return set_param_data(ap,0     ,6,6,"dddddd");
            break;
        default:
            return set_param_data(ap,0     ,6,5,"0ddddd");
            break;
        }
        break;
    case(STRANDS):
        switch(mode) {
        case(2):
            return set_param_data(ap,COUTHREADS,14,13,"di0dddDDDDDDDi");
            break;
        default: 
            return set_param_data(ap,0,14,14,"diidddDDDDDDDi");
            break;
        }
        break;
    case(REFOCUS):   return set_param_data(ap,0,5,5,"diDDD");
    case(CHANPHASE): return set_param_data(ap,0,1,1,"i");
    case(SILEND):    return set_param_data(ap,0,1,1,"d");
    case(SPECULATE): return set_param_data(ap,0,2,2,"dd");
    case(SPECTUNE):
        switch(mode) {
        case(3):    // fall thro
        case(0):    return set_param_data(ap,0         ,0,0,"");
        case(1):    return set_param_data(ap,TUNINGLIST,0,0,"");
        case(2):    return set_param_data(ap,TUNINGLIST,0,0,"");
        }
        break;
    case(REPAIR):   return set_param_data(ap,0,1,1,"i");
    case(DISTSHIFT):
        switch(mode) {
        case(0):     return set_param_data(ap,0,2,2,"ii");
        default:     return set_param_data(ap,0,2,1,"i0");
        }
        break;
    case(QUIRK):    return set_param_data(ap,0,1,1,"d");
    case(ROTOR):
        switch(mode) {
        case(0):     return set_param_data(ap,ROTORDAT,9,9,"iDDDIIddD");
        default:     return set_param_data(ap,ROTORDAT,9,8,"iDDDIIdd0");
        }
        break;
    case(DISTCUT):
        switch(mode) {
        case(0):     return set_param_data(ap,0,3,2,"I0D");
        default:     return set_param_data(ap,0,3,3,"IID");
        }
        break;
    case(ENVCUT):
        switch(mode) {
        case(0):     return set_param_data(ap,0,4,3,"D0DD");
        default:     return set_param_data(ap,0,4,4,"DDDD");
        }
        break;
    case(SPECFOLD):  
        switch(mode) {
        case(1):     return set_param_data(ap,0,3,2,"ii0");
        default:     return set_param_data(ap,0,3,3,"iii");
        }
        break;
    case(BROWNIAN):
        switch(mode) {
        case(0):     return set_param_data(ap,0,12,12,"idDDDDddDDDi");
        case(1):     return set_param_data(ap,0,12,10,"id00DDddDDDi");
        }
        break;
    case(SPIN):
        switch(mode) {
        case(0):     return set_param_data(ap,0,5,3,"D00di");
        case(1):     return set_param_data(ap,0,5,5,"Diidi");
        case(2):     return set_param_data(ap,0,5,5,"Diidi");
        }
        break;
    case(SPINQ):     return set_param_data(ap,0,5,5,"Diidi");
    case(CRUMBLE):
        switch(mode) {
        case(0):     return set_param_data(ap,0   ,12,11,"ddd0iDDDDDDi");
        case(1):     return set_param_data(ap,0   ,12,12,"ddddiDDDDDDi");
        }
        break;
    case(PHASOR):    return set_param_data(ap,0,4,4,"iDDi");
    case(TESSELATE): return set_param_data(ap,TESSELATION,4,4,"iddi");
    case(CRYSTAL):   return set_param_data(ap,CRYSTALDAT,7,7,"DDDDdDD");
    case(WAVEFORM):
        switch(mode) {
        case(0):     return set_param_data(ap,0   ,3,2,"di0");
        case(1):     return set_param_data(ap,0   ,3,2,"dd0");
        case(2):     return set_param_data(ap,0   ,3,3,"ddd");
        }
        break;
    case(DVDWIND):   return set_param_data(ap,0,2,2,"DD");
    case(CASCADE):
        if(mode < 5) return set_param_data(ap,0       ,3,3,"DID");
        else         return set_param_data(ap,CASCLIPS,3,1,"0I0");
        break;
    case(SYNSPLINE): return set_param_data(ap,0,6,6,"idDIIi");
    case(SPLINTER):  return set_param_data(ap,0   ,6,6,"diiidd");
    case(REPEATER): 
        if(mode >= 2)
            return set_param_data(ap,REPEATDATA,3,3,"DDD");
        else
            return set_param_data(ap,REPEATDATA,3,0,"000");
        break;
    case(VERGES):    return set_param_data(ap,VERGEDATA,0,0,"");
    case(MOTOR):
        switch(mode) {
        case(1):     //fall thro
        case(4):     //fall thro
        case(7):     return set_param_data(ap,MOTORDATA,6,6,"dDDDDD");
        default:     return set_param_data(ap,0        ,6,6,"dDDDDD");
        }
        break;
    case(STUTTER):   return set_param_data(ap,MOTORDATA,6,6,"didddi");
    case(IMPULSE):   return set_param_data(ap,0        ,6,6,"dDDDID");
    case(SCRUNCH):
        switch(mode) {
        case(0):     //fall thro
        case(1):     return set_param_data(ap,0   ,2,2,"di");
        case(2):     //fall thro
        case(3):     //fall thro
        case(8):     //fall thro
        case(9):     return set_param_data(ap,0   ,2,1,"0i");
        default:     return set_param_data(ap,VERGEDATA,2,1,"0i");
        }
        break;
    case(TWEET):
        switch(mode) {
        case(2):     return set_param_data(ap,FOFEX_EXCLUDES,4,2,"Dd00");
        default:     return set_param_data(ap,FOFEX_EXCLUDES,4,4,"DdID");
        }
        break;
    case(RRRR_EXTEND):  //  Version 8+
        switch(mode) {
        case(0):     return set_param_data(ap,0,11,10,"dddidiDD0DD");
        case(1):     return set_param_data(ap,0,11,11,"dddidiDDiDD");
        case(2):     return set_param_data(ap,0,11,4, "dd0id000000");
        }
        break;
    case(SORTER):
        switch(mode) {
        case(4):     return set_param_data(ap,0   ,2,2,"Di");
        default:     return set_param_data(ap,0   ,2,1,"D0");
        }
        break;
    case(SPECFNU):
        switch(mode) {
        case(F_NARROW):   return set_param_data(ap,0               ,2,1,"D0"  );
        case(F_SQUEEZE):  return set_param_data(ap,0               ,2,2,"Di"  );
        case(F_INVERT):   return set_param_data(ap,0               ,2,1,"D0"  );
        case(F_ROTATE):   return set_param_data(ap,0               ,2,1,"D0"  );
        case(F_NEGATE):   return set_param_data(ap,0               ,2,0,"00"  );
        case(F_SUPPRESS): return set_param_data(ap,0               ,2,1,"i0"  );
        case(F_MAKEFILT): return set_param_data(ap,FFILT           ,2,1,"i0"  );
        case(F_MOVE):     return set_param_data(ap,0               ,4,4,"DDDD");
        case(F_MOVE2):    return set_param_data(ap,0               ,4,4,"DDDD");
        case(F_ARPEG):    return set_param_data(ap,0               ,2,1,"D0"  );
        case(F_OCTSHIFT): return set_param_data(ap,0               ,2,1,"I0"  );
        case(F_TRANS):    return set_param_data(ap,0               ,2,1,"D0"  );
        case(F_FRQSHIFT): return set_param_data(ap,0               ,2,1,"D0"  );
        case(F_RESPACE):  return set_param_data(ap,0               ,2,1,"D0"  );
        case(F_PINVERT):  return set_param_data(ap,INTERVAL_MAPPING,2,1,"D0"  );
        case(F_PEXAGG):   return set_param_data(ap,0               ,2,2,"DD"  );
        case(F_PQUANT):   return set_param_data(ap,HFIELD          ,2,0,"00"  );
        case(F_PCHRAND):  return set_param_data(ap,HFIELD_OR_ZERO  ,2,2,"DD"  );
        case(F_RAND):     return set_param_data(ap,0               ,2,1,"D0"  );
        case(F_SEE):      return set_param_data(ap,0               ,0,0,""    );
        case(F_SEEPKS):   return set_param_data(ap,0               ,0,0,""    );
        case(F_SYLABTROF):return set_param_data(ap,0               ,0,0,""    );
        case(F_SINUS):    return set_param_data(ap,HFIELD_OR_ZERO  ,2,1,"D0"  );
        }
        break;
    case(FLATTEN):   return set_param_data(ap,0   ,2,2,"dd" );
    case(BOUNCE):    return set_param_data(ap,0   ,5,5,"idddd");
    case(DISTMARK):  return set_param_data(ap,MARKLIST             ,1,1,"D"   );
    case(DISTREP):   return set_param_data(ap,0   ,2,2,"II" );
    case(TOSTEREO):  return set_param_data(ap,0   ,2,2,"dd" );
    case(SUPPRESS):  return set_param_data(ap,MANYCUTS             ,3,3,"ddi" );
    case(CALTRAIN):  return set_param_data(ap,0   ,2,2,"dd" );
    case(SPECENV):   return set_param_data(ap,0   ,1,1,"i"  );
    case(CLIP):      return set_param_data(ap,0   ,1,1,"d"  );
    case(SPECEX):    return set_param_data(ap,0   ,3,3,"ddd");
    case(MATRIX):
        switch(mode) {
        case(MATRIX_USE):  return set_param_data(ap,MATRIX_DATA     ,0,0,""   );
        default:           return set_param_data(ap,0               ,2,2,"ii" );
        }
        break;
    case(TRANSPART): return set_param_data(ap,0   ,3,3,"DDd");
    case(SPECINVNU): return set_param_data(ap,0   ,4,4,"dddd");
    case(SPECCONV):  return set_param_data(ap,0   ,1,1,"d"  );
    case(SPECSND):   return set_param_data(ap,0   ,2,2,"ii" );
    case(SPECFRAC):  return set_param_data(ap,0   ,1,1,"i"  );
    case(FRACTAL):
        switch(mode) {
            case(0): return set_param_data(ap,FRACSHAPE         ,1,0,"0"  );
            case(1): return set_param_data(ap,FRACSHAPE         ,1,1,"d"  );
        }
        break;
    case(FRACSPEC):  return set_param_data(ap,FRACSHAPE         ,1,0,"0"  );
    case(ENVSPEAK):
        switch(mode) {
        case(4):    //  fall thro
        case(5):    //  fall thro
        case(0):     return set_param_data(ap,0  ,6,5,"iiiID0");    //  repet & repet-shrink
        case(1):     return set_param_data(ap,0  ,6,3,"iii000");    //  reverse-repet
        case(2):    //  fall thro
        case(3):     return set_param_data(ap,0  ,6,5,"iiiID0");    //  atten "alternate"
        case(6):     return set_param_data(ap,0  ,6,6,"iiiIDI");    //  repeat part-of
        case(7):    //  fall thro
        case(8):     return set_param_data(ap,0  ,6,6,"iiiIDD");    //  repeat but shrink
        case(9):     return set_param_data(ap,0  ,6,2,"ii0000");    //  extract all
        case(10):    return set_param_data(ap,0  ,6,3,"iii000");    //  permute randomly
        case(11):    return set_param_data(ap,0  ,6,3,"iiI000");    //  permute N-wise
        case(16):   //  fall thro
        case(17):   //  fall thro
        case(12):    return set_param_data(ap,XSPK_CUTS  ,6,4,"0iiID0");    //  repet & repet-shrink
        case(13):    return set_param_data(ap,XSPK_CUTS  ,6,2,"0ii000");    //  reverse-repet
        case(14):   //  fall thro
        case(15):    return set_param_data(ap,XSPK_CUTS  ,6,4,"0iiID0");    //  atten "alternate"
        case(18):    return set_param_data(ap,XSPK_CUTS  ,6,5,"0iiIDI");    //  repeat part-of
        case(19):   //  fall thro
        case(20):    return set_param_data(ap,XSPK_CUTS  ,6,5,"0iiIDD");    //  repeat but shrink
        case(21):    return set_param_data(ap,XSPK_CUTS  ,6,1,"0i0000");    //  extract all
        case(22):    return set_param_data(ap,XSPK_CUTS  ,6,2,"0ii000");    //  permute randomly
        case(23):    return set_param_data(ap,XSPK_CUTS  ,6,2,"0iI000");    //  permute N-wise
        }
        break;
    case(EXTSPEAK):
        switch(mode) {
        case(9):    //  fall thro
        case(10):   //  fall thro
        case(6):    //  fall thro
        case(7):     return set_param_data(ap,XSPK_CUTS    ,6,5,"0iiIDi");  break;
        case(11):   //  fall thro
        case(8):     return set_param_data(ap,XSPK_CUTPAT  ,6,4,"0iiID0");  break;
        case(3):    //  fall thro
        case(4):    //  fall thro
        case(0):    //  fall thro
        case(1):     return set_param_data(ap,0            ,6,6,"iiiIDi");  break;
        case(5):    //  fall thro
        case(2):     return set_param_data(ap,XSPK_PATTERN ,6,5,"iiiID0");  break;
        case(12):   //  fall thro
        case(13):   //  fall thro
        case(15):   //  fall thro
        case(16):    return set_param_data(ap,XSPK_CUTARG  ,6,3,"0i00Di");  break;
        case(14):   //  fall thro
        case(17):    return set_param_data(ap,XSPK_CUPATA  ,6,2,"0i00D0");  break;
        }
        break;
    case(ENVSCULPT):
        switch(mode) {
        case(0):     return set_param_data(ap,0   ,6,4,"dddd00");   break;
        case(1):     return set_param_data(ap,0   ,6,6,"dddddd");   break;
        case(2):     return set_param_data(ap,0   ,6,3,"0ddd00");   break;
        }
        break;
    case(TREMENV):   return set_param_data(ap,0,4,4,"ddid");
        break;
    case(DCFIX):     return set_param_data(ap,0,1,1,"d");
        break;
    default:
        sprintf(errstr,"Unknown process (%d) in set_legal_param_structure2()\n",process);
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}                                                           

/************************** SET_LEGAL_OPTION_AND_VARIANT_STRUCTURE2 ***********/

int set_legal_option_and_variant_structure2(int process,int mode,aplptr ap)                                       
{
                                           /*|      | |        |        |v| |       */
                                           /*|      |o|        |        |f|v|       */
                                           /*|option|p| option |variant |l|p|variant*/
                                           /*|flags |t|  list  | flags  |a|a|list   */
                                           /*|      |c|        |        |g|r|       */
                                           /*|      |n|        |        |c|a|      */
                                           /*|      |t|        |        |n|m|      */
                                           /*|      | |        |        |t|s|       */
    switch(process) {
    case(TAPDELAY):     return set_vflgs2(ap,""     ,0,""      ,"f"     ,1,0,"0"     );
    case(RMRESP):       return set_vflgs2(ap,""     ,0,""      ,"ar"    ,2,2,"dd"    );
    case(RMVERB):       return set_vflgs2(ap,""     ,0,""      ,"LHpcdf",6,4,"dddi00");
    case(MIXMULTI):     return set_vflgs2(ap,"seg"  ,3,"ddd"   ,""       ,0,0,""     );
    case(ANALJOIN):     return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PTOBRK):       return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_STRETCH): return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_DUPL):    return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_DEL):     return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_STRFILL): return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_FREEZE):  return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_CHOP):    return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_INTERP):  return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_FEATURES):return set_vflgs2(ap,""     ,0,""      ,"a"      ,1,0,"0"    );
    case(PSOW_SYNTH):   return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_IMPOSE):  return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_SPLIT):   return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_SPACE):   return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_INTERLEAVE): return set_vflgs2(ap,""  ,0,""      ,""       ,0,0,""     );
    case(PSOW_REPLACE): return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_EXTEND):  return set_vflgs2(ap,""     ,0,""      ,"s"      ,1,0,"0"    );
    case(PSOW_EXTEND2): return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_LOCATE):  return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PSOW_CUT):     return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(ONEFORM_GET):  return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(ONEFORM_PUT):  return set_vflgs2(ap,"lhg"  ,3,"ddd"   ,""       ,0,0,""     );
    case(ONEFORM_COMBINE):  return set_vflgs2(ap,"" ,0,""      ,""       ,0,0,""     );
    case(NEWGATE):      return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(SPEC_REMOVE):  return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PREFIXSIL):    return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(STRANS):
        switch(mode) {
        case(0):
        case(1):        return set_vflgs2(ap,"",     0,""      ,"o"      ,1,0,"0"    );
        case(2):        return set_vflgs2(ap,"s"    ,1,"d"     ,""       ,0,0,""     );
        case(3):        return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
        }
        break;
    case(SPECROSS):     return set_vflgs2(ap,"",     0,""      ,"ap"     ,2,0,"00"   );
    case(PSOW_REINF):
        switch(mode) {
        case(0):        return set_vflgs2(ap,"",     0,""      ,"ds"     ,2,1,"d0"   );
        case(1):        return set_vflgs2(ap,"",     0,""      ,"w"      ,1,1,"d"    );
        }
        break;
    case(PARTIALS_HARM):  return set_vflgs2(ap,""   ,0,""      ,"v"      ,1,0,"0"    );
    case(LUCIER_GETF):  return set_vflgs2(ap,""     ,0,""      ,"c"      ,1,1,"d"    );
    case(LUCIER_GET):   return set_vflgs2(ap,""     ,0,""      ,"l"      ,1,0,"0"    );
    case(LUCIER_PUT):   return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(LUCIER_DEL):   return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(SPECTRACT):
    case(SPECLEAN):     return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PHASE):
        switch(mode) {
            case(0):    return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
            case(1):    return set_vflgs2(ap,"t"    ,1,"d"     ,""       ,0,0,""     );
        }
        break;
    case(BRKTOPI):      return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(SPECSLICE):    return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(FOFEX_EX):     return set_vflgs2(ap,""     ,0,""      ,"w"      ,1,0,"0"    );
    case(FOFEX_CO):
        switch(mode) {
        case(FOF_SINGLE):
        case(FOF_MEASURE):
            return set_vflgs2(ap,"",0,"",""  ,0,0,""  );
        case(FOF_SUM):
        case(FOF_LOSUM):
        case(FOF_MIDSUM):
        case(FOF_HISUM):
        case(FOF_LOHI):
        case(FOF_TRIPLE):
            return set_vflgs2(ap,"",0,"","n" ,1,0,"0" );
        }
        break;
    case(GREV_EXTEND):  return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PEAKFIND):     return set_vflgs2(ap,""     ,0,""      ,"t"      ,1,1,"d"    );
    case(CONSTRICT):    return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(EXPDECAY):     return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(PEAKCHOP):
        switch(mode) {
        case(0):        return set_vflgs2(ap,""     ,0,""      ,"gqsnrm" ,6,6,"ddDDII");
        case(1):        return set_vflgs2(ap,""     ,0,""      ,"gq"     ,2,2,"dd");
        case(2):        return set_vflgs2(ap,""     ,0,""      ,"gqsnr"  ,5,5,"ddDDI");
        }
        break;
    case(MCHANPAN):
        switch(mode) {
        case(0):        return set_vflgs2(ap,""     ,0,""      ,"f"      ,1,1,"d"    );
        case(1):        return set_vflgs2(ap,""     ,0,""      ,"fm"     ,2,2,"dd"   );
        case(2):        return set_vflgs2(ap,""     ,0,""      ,"s"      ,1,0,"0"    );
        case(8):        return set_vflgs2(ap,""     ,0,""      ,"a"      ,1,0,"0"    );
        case(9):        return set_vflgs2(ap,"fmg"  ,3,"ddI"   ,"ar"     ,2,0,"00"   );
        default:        return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
        }
        break;
    case(TEX_MCHAN):    return set_vflgs2(ap,"aps"  ,3,"DDD"   ,"rwcp"   ,5,1,"i0000");
    case(MANYSIL):      return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(RETIME):
        switch(mode) {
        case(4):        return set_vflgs2(ap,"sea"  ,3,"ddd"   ,""       ,0,0,""     );
        case(9):        return set_vflgs2(ap,"mp"   ,2,"id"    ,""       ,0,0,""     );
        default:        return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
        }
        break;
    case(HOVER):        return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(HOVER2):       return set_vflgs2(ap,""     ,0,""      ,"sn"     ,2,0,"00"   );
    case(MULTIMIX):
        switch(mode) {
        case(6):        return set_vflgs2(ap,"st"   ,2,"id"    ,""       ,0,0,""     );
        default:        return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
        }
        break;
    case(FRAME):
        switch(mode) {
        case(0):        return set_vflgs2(ap,"s"    ,1,"d"     ,""       ,0,0,""     );
        case(1):        return set_vflgs2(ap,"s"    ,1,"d"     ,""       ,0,0,""     );
        case(4):        return set_vflgs2(ap,""     ,0,""      ,"b"      ,1,0,"0"    );
        case(7):        return set_vflgs2(ap,""     ,0,""      ,"b"      ,1,0,"0"    );
        default:        return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
        }
        break;
    case(SEARCH):       return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(MCHANREV):     return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(WRAPPAGE):     return set_vflgs2(ap,"b"    ,1,"i"     ,"eo"     ,2,0,"00"   );
    case(MCHSTEREO):    return set_vflgs2(ap,""     ,0,""      ,"s"      ,1,0,"0"    );
    case(MTON):         return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(FLUTTER):      return set_vflgs2(ap,""     ,0,""      ,"r"      ,1,0,"0"    );
    case(ABFPAN):       return set_vflgs2(ap,"o"    ,1,"i"     ,"bx"     ,2,0,"00"   );
    case(ABFPAN2):      return set_vflgs2(ap,"g"    ,1,"d"     ,"w"      ,1,0,"0"    );
    case(ABFPAN2P):     return set_vflgs2(ap,"g"    ,1,"d"     ,"pw"     ,2,1,"d0"   );
    case(CHANNELX):     return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(CHORDER):      return set_vflgs2(ap,""     ,0,""      ,"a"      ,1,0,""     );
    case(FMDCODE):      return set_vflgs2(ap,""     ,0,""      ,"xw"     ,2,0,"00"   );
    case(CHXFORMAT):    return set_vflgs2(ap,"s"    ,1,"i"     ,""       ,0,0,""     );
    case(CHXFORMATG):   return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(CHXFORMATM):   return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(INTERLX):      return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(COPYSFX):      return set_vflgs2(ap,"st"   ,2,"ii"    ,"dh"     ,2,0,"00"   );
    case(NJOIN):        return set_vflgs2(ap,"s"    ,1,"d"     ,"xc"     ,2,0,"00"   );
    case(NJOINCH):      return set_vflgs2(ap,""     ,0,""      ,"x"      ,1,0,"0"    );
    case(NMIX):         return set_vflgs2(ap,"o"    ,1,"d"     ,"df"     ,2,0,"00"   );
    case(RMSINFO):      return set_vflgs2(ap,""     ,0,""      ,"n"      ,1,0,"0"    );
    case(SFEXPROPS):    return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(SETHARES):
        switch(mode) {
        case(0):        return set_vflgs2(ap,"h"    ,1,"d"     ,"amqz"   ,4,0,"0000" );
        case(1):
        case(2):
        case(3):        return set_vflgs2(ap,"h"    ,1,"d"     ,"amqzf"  ,5,0,"00000");
        }
        break;
    case(MCHSHRED):     return set_vflgs2(ap,""     ,0,""      ,""       ,0,0,""     );
    case(MCHZIG):
        switch(mode) {
        case(0):       return set_vflgs2(ap,"smr"   ,3,"ddi"  ,"a"       ,1,0,"0"    );
        case(1):       return set_vflgs2(ap,"s"     ,1,"d"    ,"a"       ,1,0,"0"    );
        }
        break;
    case(MCHITER):     return set_vflgs2(ap,""      ,0,""      ,"drpafgs",7,7,"DDDDDdi");
    case(NEWDELAY):
        switch(mode) {
        case(0):
        case(2):       return set_vflgs2(ap,""      ,0,""      ,""       ,0,0,""     );
        default:       return set_vflgs2(ap,"rdm"   ,3,"did"   ,""       ,0,0,""     );
        }
        break;
    case(ITERLINE):
    case(ITERLINEF):   return set_vflgs2(ap,""      ,0,""      ,"n"      ,1,0,"0"    ); 
    case(FILTRAGE):    return set_vflgs2(ap,"s"     ,1,"i"     ,""       ,0,0,""     );
    case(SELFSIM):     return set_vflgs2(ap,""      ,0,""      ,""       ,0,0,""     );
    case(ITERFOF):
        switch(mode) {
        case(0):
        case(2):       return set_vflgs2(ap,"patTErvVdD",10,"DDDDdDDDDD","s",1,1,"i");
        default:       return set_vflgs2(ap,"patTErvVdDgGFfSPi",17,"DDDDdDDDDDDDDDDiD","s",1,1,"i");
        }
        break;
    case(PULSER):
        switch(mode) {
        case(2):       return set_vflgs2(ap,""      ,0,""      ,"eEpaobsw",8,8,"DDDDDDiD");
        default:       return set_vflgs2(ap,""      ,0,""      ,"eEpaobs", 7,7,"DDDDDDi");
        }
        break;
    case(PULSER2):
        switch(mode) {
        case(2):       return set_vflgs2(ap,""      ,0,""      ,"eEpaobswr",9,8,"DDDDDDiD0");
        default:       return set_vflgs2(ap,""      ,0,""      ,"eEpaobsr",8,7,"DDDDDDi0");
        }
        break;
    case(PULSER3):     return set_vflgs2(ap,""      ,0,""      ,"eEpaobsSc",9,9,"DDDDDDiiD");
    case(CHIRIKOV):    return set_vflgs2(ap,"",0    ,""        ,""       ,0,0,""     );
    case(MULTIOSC):    return set_vflgs2(ap,""      ,0,""      ,""       ,0,0,""     );
    case(SYNFILT):     return set_vflgs2(ap,"",0    ,""        ,"do"     ,2,0,"00"   );
    case(SPECRAND):    return set_vflgs2(ap,"tg"    ,2,"Di"    ,""       ,0,0,""     );
    case(SPECSQZ):     return set_vflgs2(ap,""      ,0,""      ,""       ,0,0,""     );
    case(SPECSPHINX):
        switch(mode) {
        case(0):       return set_vflgs2(ap,"af"    ,2,"DD"    ,""       ,0,0,""     );
        case(1):       return set_vflgs2(ap,"bg"    ,2,"DD"    ,""       ,0,0,""     );
        case(2):       return set_vflgs2(ap,"dgc"   ,3,"Ddd"   ,"e"      ,1,0,"0"    );
        }
        break;
    case(SPECMORPH):
        switch(mode) {
        case(4):
        case(5):       return set_vflgs2(ap,"r"     ,1,"D"     ,""       ,0,0,""     );
        default:       return set_vflgs2(ap,""      ,0,""      ,"enf"    ,3,0,"000"  );
        }
        break;
    case(SPECMORPH2):
        switch(mode) {
        case(0):       return set_vflgs2(ap,""       ,0,""      ,""      ,0,0,""     );
        default:       return set_vflgs2(ap,"r"      ,1,"D"     ,""      ,0,0,""     );
        }
        break;
    case(SUPERACCU):   return set_vflgs2(ap,"dg"    ,2,"DD"    ,"r"      ,1,0,"0"    );
    case(PARTITION):
        switch(mode) {
        case(0):       return set_vflgs2(ap,""      ,0,""      ,""       ,0,0,""     );
        case(1):       return set_vflgs2(ap,""      ,0,""      ,"rs"     ,2,2,"dd"   );
        }
        break;
    case(SPECGRIDS):   return set_vflgs2(ap,""      ,0,""      ,""       ,0,0,""     );
    case(GLISTEN):     return set_vflgs2(ap,"pdv"   ,3,"DDD"   ,""       ,0,0,""     );
    case(TUNEVARY):    return set_vflgs2(ap,"fc"    ,2,"DD"    ,"tb"     ,2,2,"ID"   );
    case(ISOLATE):
        switch(mode) {
        case(ISO_SEGMNT):
        case(ISO_GROUPS):
        case(ISO_SLICED): return set_vflgs2(ap,"s"  ,1,"d"     ,"xr"     ,2,0,"00"   );
        case(ISO_THRESH): return set_vflgs2(ap,"sml",3,"ddd"   ,"xr"     ,2,0,"00"   );
        case(ISO_OVRLAP): return set_vflgs2(ap,"sd" ,2,"dd"    ,"xr"     ,2,0,"00"   );
        }
        break;
    case(REJOIN):      return set_vflgs2(ap,"g"     ,1,"d"    ,"r"       ,1,0,"0"    );
    case(PANORAMA):    return set_vflgs2(ap,"r"     ,1,"d"    ,"pq"      ,2,0,"00"   );
    case(TREMOLO):     return set_vflgs2(ap,""      ,0,""     ,""        ,0,0,""     );
    case(ECHO):        return set_vflgs2(ap,"rc"    ,2,"Dd"   ,""        ,0,0,""     );
    case(PACKET):      return set_vflgs2(ap,""      ,0,""     ,"nfs"     ,3,0,"000"  );
    case(SYNTHESIZER):
        switch(mode) {
        case(0):       return set_vflgs2(ap,""      ,0,""     ,""        ,0,0,""     );
        case(1):       return set_vflgs2(ap,"nc"    ,2,"DD"   ,"f"       ,1,0,"0"    );
        case(2):       return set_vflgs2(ap,"udfsneEcCtr",11,"ddddiididid","azmxj",5,0,"00000");
        case(3):       return set_vflgs2(ap,"rf"    ,2,"Di"   ,"e"       ,1,0,"0"    );
        }
        break;
    case(NEWTEX):
        switch(mode) {
        case(0):       return set_vflgs2(ap,"sneEcCr",7       ,"dIididd","xj",2,0,"00");
        case(1):
        case(2):       return set_vflgs2(ap,"sneEcCr",7       ,"diididd","xj",2,0,"00");
        }
        break;
    case(CERACU):      return set_vflgs2(ap,""       ,0       ,""       ,"ol",2,0,"00");
    case(MADRID):
        switch(mode) {
        case(0):       return set_vflgs2(ap,"s"      ,1       ,"i"      ,"elrR",4,0,"0000");
        case(1):       return set_vflgs2(ap,"s"      ,1       ,"i"      ,"el"  ,2,0,"00");
        }
        break;
    case(SHIFTER):     return set_vflgs2(ap,""       ,0       ,""       ,"zrl" ,3,0,"000");
    case(SUBTRACT):    return set_vflgs2(ap,"c"      ,1       ,"i"      ,""    ,0,0,"");
    case(SPEKLINE):    return set_vflgs2(ap,""       ,0       ,""       ,""    ,0,0,"");
    case(FRACTURE):
        switch(mode) {
        case(0):       return set_vflgs2(ap,"rpdvesthmi",10,"DDDDDDDiDD","yl"   ,2,0,"00");
        case(1):       return set_vflgs2(ap,"rpdvesthmiazclfjkwg",19,"DDDDDDDiDDddddddddd","y",1,0,"0");
        }
        break;
    case(TAN_ONE):
    case(TAN_TWO):
    case(TAN_SEQ):
    case(TAN_LIST):
        switch(mode) {
        case(0):       return set_vflgs2(ap,"fjs"   ,3,"iDd"  ,"rl"      ,2,0,"00"   );
        case(1):       return set_vflgs2(ap,"fj"    ,2,"iD"   ,"rl"      ,2,0,"00"   );
        }
        break;
    case(SPECTWIN):    return set_vflgs2(ap,"efdsr" ,5,"DDidd",""        ,0,0,""     );
    case(TRANSIT):     return set_vflgs2(ap,"tdem"  ,4,"dddd" ,"l"       ,1,0,"0"    );
    case(TRANSITF):    return set_vflgs2(ap,"tdem"  ,4,"dddd" ,"l"       ,1,0,"0"    );
    case(TRANSITD):    return set_vflgs2(ap,"tdem"  ,4,"dddd" ,"l"       ,1,0,"0"    );
    case(TRANSITFD):   return set_vflgs2(ap,"tdem"  ,4,"dddd" ,"l"       ,1,0,"0"    );
    case(TRANSITS):    return set_vflgs2(ap,""      ,0,""     ,"l"       ,1,0,"0"    );
    case(TRANSITL):    return set_vflgs2(ap,""      ,0,""     ,"l"       ,1,0,"0"    );
    case(CANTOR):       
        switch(mode) {
        case(0):
        case(1):       return set_vflgs2(ap,""      ,0,""     ,"e"       ,1,0,"0"    );
        case(2):       return set_vflgs2(ap,""      ,0,""     ,""        ,0,0,""     );
        }
        break;
    case(SHRINK):
        switch(mode) {
        case(SHRM_FINDMX):
                       return set_vflgs2(ap,"smrglq",6,"dddddd","nieo"   ,4,0,"0000" );
        case(SHRM_LISTMX):
                       return set_vflgs2(ap,"smrgl" ,5,"ddddd" ,"nieo"   ,4,0,"0000" );
        default:       return set_vflgs2(ap,"smr"   ,3,"ddd"   ,"ni"     ,2,0,"00"   );
        }
        break;
    case(STRANDS):
        switch(mode) {
        case(0):
        case(2):      return set_vflgs2(ap,"gmf"    ,3,"ddi"    ,""      ,0,0,""    );
        case(1):      return set_vflgs2(ap,"gmf"    ,3,"ddi"    ,""      ,1,0,"s"   );
        }
        break;
    case(REFOCUS):    return set_vflgs2(ap,"oens"   ,4,"ddii"   ,""      ,0,0,""    );
    case(CHANPHASE):  return set_vflgs2(ap,""       ,0,""       ,""      ,0,0,""    );
    case(SILEND):     return set_vflgs2(ap,""       ,0,""       ,""      ,0,0,""    );
    case(SPECULATE):  return set_vflgs2(ap,""       ,0,""       ,"r"     ,1,0,"0"   );
    case(REPAIR):     return set_vflgs2(ap,""       ,0,""       ,""      ,0,0,"0"   );
    case(DISTSHIFT):  return set_vflgs2(ap,""       ,0,""       ,""      ,0,0,"0"   );
    case(QUIRK):      return set_vflgs2(ap,""       ,0,""       ,""      ,0,0,"0"   );
    case(ROTOR):      return set_vflgs2(ap,"d"      ,1,"d"      ,"s"     ,1,0,"0"   );
    case(SPECTUNE):
        switch(mode) {
        case(3):     return set_vflgs2(ap,"mlhseiwn",8,"idddddid","rb"   ,2,0,"00"  );
        default:     return set_vflgs2(ap,"mlhseiwn",8,"idddddid","rbf"  ,3,0,"000" );
        }
        break;
    case(DISTCUT):    return set_vflgs2(ap,"c"      ,1,"d"      ,""      ,0,0,""    );
    case(ENVCUT):     return set_vflgs2(ap,"c"      ,1,"d"      ,""      ,0,0,""    );
    case(SPECFOLD):   return set_vflgs2(ap,""       ,0,""       ,"a"     ,1,0,"0"   );
    case(BROWNIAN):
        switch(mode) {
        case(0):     return set_vflgs2(ap,"amsd"    ,4,"DDDD"   ,"l"     ,1,0,"0"   );
        case(1):     return set_vflgs2(ap,"am"      ,2,"DD"     ,"l"     ,1,0,"0"   );
        }
        break;
    case(SPIN):
        switch(mode) {
        case(0):     return set_vflgs2(ap,""        ,0,""       ,"ba"    ,2,2,"dd"  );
        case(1):     return set_vflgs2(ap,""        ,0,""       ,"bakc"  ,4,4,"dddd");
        case(2):     return set_vflgs2(ap,""        ,0,""       ,"bak"   ,3,3,"ddd" );
        }
        break;
    case(SPINQ):
        switch(mode) {
        case(0):     return set_vflgs2(ap,""        ,0,""       ,"bakc"  ,4,4,"dddd");
        case(1):     return set_vflgs2(ap,""        ,0,""       ,"bak"   ,3,3,"ddd" );
        }
    case(CRUMBLE):   return set_vflgs2(ap,""        ,0,""       ,"std"   ,3,3,"dDd" );
    case(PHASOR):    return set_vflgs2(ap,"o"       ,1,"d"      ,"se"    ,2,0,"00"  );
    case(TESSELATE): return set_vflgs2(ap,""        ,0,""       ,""      ,0,0,"0"   );
    case(CRYSTAL):   return set_vflgs2(ap,"psaPFS"  ,6,"dddddd" ,""      ,0,0,""    );
    case(WAVEFORM):  return set_vflgs2(ap,""        ,0,""       ,""      ,0,0,""    );
    case(DVDWIND):   return set_vflgs2(ap,""        ,0,""       ,""      ,0,0,""    );
    case(CASCADE):   return set_vflgs2(ap,"ersNC"   ,5,"IDiII"  ,"aln"   ,3,0,"000" );
    case(SYNSPLINE): return set_vflgs2(ap,"sidv"    ,4,"IIDD"   ,"n"     ,1,0,"0"   );
        break;
    case(SPLINTER):
        switch(mode) {
        case(0):    //  fall thro
        case(1):     return set_vflgs2(ap,"espfrv"  ,6,"idddDD" ,"iI"    ,2,0,"00"  );
        case(2):    //  fall thro
        case(3):     return set_vflgs2(ap,"espdrv"  ,6,"idddDD" ,"iI"    ,2,0,"00"  );
        }
        break;
    case(REPEATER):  return set_vflgs2(ap,"rps"     ,3,"DDi"    ,""      ,0,0,""    );
    case(VERGES):    return set_vflgs2(ap,"ted"     ,3,"DDD"    ,"nbs"   ,3,0,"000" );
    case(MOTOR):
        switch(mode) {
        case(0):
        case(3):
        case(6):    return set_vflgs2(ap,"fpjtyebvs",9,"DDDDDDDDi","a"   ,1,0,"0"   );
        default:    return set_vflgs2(ap,"fpjtyebvs",9,"DDDDDDDDi","ac"  ,2,0,"00"  );
        }
        break;
    case(STUTTER):  return set_vflgs2(ap,"tabm"    ,4,"DDDd"     ,"p"    ,1,0,"0"   );
    case(SCRUNCH):  return set_vflgs2(ap,"cta"     ,3,"iDD"      ,""     ,0,0,""    );
    case(IMPULSE):  return set_vflgs2(ap,"gsc"     ,3,"Dii"      ,""     ,0,0,""    );
    case(TWEET):    return set_vflgs2(ap,""        ,0,""         ,"w"    ,1,0,"0"   );
    case(RRRR_EXTEND):  // version 8+
        switch(mode) {
        case(0):    return set_vflgs2(ap,""        ,0,""         ,"se"   ,2,0,"00"  );
        case(1):    return set_vflgs2(ap,""        ,0,""         ,"se"   ,2,0,"00"  );
        case(2):    return set_vflgs2(ap,""        ,0,""         ,""     ,0,0,""    );
        }
        break;
    case(SORTER):   return set_vflgs2(ap,"sopm"    ,4,"dDdd"     ,"f"     ,1,0,"0"  );
    case(SPECFNU):
        switch(mode) {
        case(F_NARROW):   return set_vflgs2(ap,"go"       ,2,"di"       ,"tfsxkr"    ,6 ,0,"000000"   );
        case(F_SQUEEZE):  return set_vflgs2(ap,"g"        ,1,"d"        ,"tfsxkr"    ,6 ,0,"000000"   );
        case(F_INVERT):   return set_vflgs2(ap,"g"        ,1,"d"        ,"sxkr"      ,4 ,0,"0000"     );
        case(F_ROTATE):   return set_vflgs2(ap,"g"        ,1,"d"        ,"sxkr"      ,4 ,0,"0000"     );
        case(F_NEGATE):   return set_vflgs2(ap,"g"        ,1,"d"        ,"f"         ,1 ,0,"0"        );
        case(F_SUPPRESS): return set_vflgs2(ap,"g"        ,1,"d"        ,"sx"        ,2 ,0,"00"       );
        case(F_MAKEFILT): return set_vflgs2(ap,"b"        ,1,"d"        ,"kifs"      ,4 ,0,"0000"     );
        case(F_MOVE):     return set_vflgs2(ap,"g"        ,1,"d"        ,"tsxkr"     ,5 ,0,"00000"    );
        case(F_MOVE2):    return set_vflgs2(ap,"g"        ,1,"d"        ,"tsnxkr"    ,6 ,0,"000000"   );
        case(F_ARPEG):    return set_vflgs2(ap,"g"        ,1,"D"        ,"sxrdc"     ,5 ,0,"00000"    );
        case(F_OCTSHIFT): return set_vflgs2(ap,"glhp"     ,4,"dddD"     ,"sxrdcf"    ,6 ,0,"000000"   );
        case(F_TRANS):    return set_vflgs2(ap,"glhp"     ,4,"dddD"     ,"sxrdcf"    ,6 ,0,"000000"   );
        case(F_FRQSHIFT): return set_vflgs2(ap,"glhp"     ,4,"dddD"     ,"sxrdcf"    ,6 ,0,"000000"   );
        case(F_RESPACE):  return set_vflgs2(ap,"glhp"     ,4,"dddD"     ,"sxrdcf"    ,6 ,0,"000000"   );
        case(F_PINVERT):  return set_vflgs2(ap,"glhpbt"   ,6,"dddDdd"   ,"sxrdc"     ,5 ,0,"00000"    );
        case(F_PEXAGG):   return set_vflgs2(ap,"glhpbt"   ,6,"dddDdd"   ,"sxrdcTFMAB",10,0,"000000000");
        case(F_PQUANT):   return set_vflgs2(ap,"glhpbt"   ,6,"dddDdd"   ,"sxrdcon"   ,7 ,0,"0000000"  );
        case(F_PCHRAND):  return set_vflgs2(ap,"glhpbt"   ,6,"dddDdd"   ,"sxrdconk"  ,8 ,0,"00000000" );
        case(F_RAND):     return set_vflgs2(ap,"glhp"     ,4,"dddD"     ,"sxrdc"     ,5 ,0,"00000"    );
        case(F_SEE):      return set_vflgs2(ap,""         ,0,""         ,"s"         ,1 ,0,"0"        );
        case(F_SEEPKS):   return set_vflgs2(ap,""         ,0,""         ,"s"         ,1 ,0,"0"        );
        case(F_SYLABTROF):return set_vflgs2(ap,"sp"       ,2,"dd"       ,"PB"        ,2 ,0,"00"       );
        case(F_SINUS):    return set_vflgs2(ap,"abcdeqpon",9,"dDDDDDDDD","sfrS"      ,4 ,0,"0000"     );
        }
        break;
    case(FLATTEN):        return set_vflgs2(ap,"t"        ,1,"d"        ,""          ,0,0,""          );
    case(BOUNCE):         return set_vflgs2(ap,"s"        ,1,"d"        ,"ec"        ,2,0,"00"        );
    case(DISTMARK):
        switch(mode) {
            case(0):      return set_vflgs2(ap,"sr"       ,2,"DD"       ,"ft"        ,2,0,"00"        );
            case(1):      return set_vflgs2(ap,"srg"      ,3,"DDD"      ,"fa"        ,2,0,"00"        );
        }
        break;
    case(DISTREP):        return set_vflgs2(ap,"ks"       ,2,"id"       ,""          ,0,0,""          );
    case(TOSTEREO):       return set_vflgs2(ap,"olrm"     ,4,"iiid"     ,""          ,0,0,""          );
    case(SUPPRESS):       return set_vflgs2(ap,""         ,0,""         ,""          ,0,0,""          );
    case(CALTRAIN):       return set_vflgs2(ap,"l"        ,1,"d"        ,""          ,0,0,""          );
    case(SPECENV):        return set_vflgs2(ap,"b"        ,1,"d"        ,"pik"       ,3,0,"000"       );
    case(CLIP):           return set_vflgs2(ap,""         ,0,""         ,""          ,0,0,""          );
    case(SPECEX):         return set_vflgs2(ap,"w"        ,1,"i"        ,"se"        ,2,0,"00"        );
    case(MATRIX):         return set_vflgs2(ap,""         ,0,""         ,"c"         ,1,0,"0"         );
        switch(mode) {
        case(MATRIX_MAKE):  // fall thro
        case(MATRIX_USE): return set_vflgs2(ap,""         ,0,""         ,"c"         ,1,0,"0"         );
        default:          return set_vflgs2(ap,""         ,0,""         ,""          ,0,0,""          );
        }
        break;
    case(TRANSPART):      return set_vflgs2(ap,""         ,0,""         ,""          ,0,0,""          );
    case(SPECINVNU):      return set_vflgs2(ap,""         ,0,""         ,""          ,0,0,""          );
    case(SPECCONV):
        switch(mode) {
        case(0):          return set_vflgs2(ap,"d"        ,1,"i"        ,""          ,0,0,""          );
        case(1):          return set_vflgs2(ap,"d"        ,1,"i"        ,""          ,1,0,"p"         );
        }
        break;
    case(SPECSND):        return set_vflgs2(ap,""         ,0,""         ,""          ,0,0,""          );
    case(SPECFRAC):       return set_vflgs2(ap,""         ,0,""         ,"i"         ,1,0,"0"         );
    case(FRACTAL):
        switch(mode) {
        case(0):          return set_vflgs2(ap,"mti"      ,3,"IDD"      ,"so"        ,2,0,"00"        );
        case(1):          return set_vflgs2(ap,"mti"      ,3,"IDD"      ,"s"         ,1,0,"0"         );
        }
        break;
    case(FRACSPEC):       return set_vflgs2(ap,"mti"      ,3,"IDD"      ,"sn"        ,2,0,"00"        );
    case(ENVSPEAK):
        switch(mode) {
        case(18):   //  fall thro
        case(6):          return set_vflgs2(ap,""          ,0,""        ,"z"          ,1,0,"0"        );
        default:          return set_vflgs2(ap,""          ,0,""        ,""           ,0,0,""         );
        }
        break;
    case(EXTSPEAK):
        switch(mode) {
        case(6):    //  fall thro
        case(0):          return set_vflgs2(ap,""          ,0,""        ,"tekoir"     ,6,0,"000000"   );
        case(7):    //  fall thro
        case(8):    //  fall thro
        case(1):    //  fall thro
        case(2):          return set_vflgs2(ap,""          ,0,""        ,"tekoi"      ,5,0,"00000"    );
        case(9):    //  fall thro
        case(3):          return set_vflgs2(ap,""          ,0,""            ,"tekr"       ,4,0,"0000"     );
        case(10):   //  fall thro
        case(11):   //  fall thro
        case(4):    //  fall thro
        case(5):          return set_vflgs2(ap,""          ,0,""            ,"tek"        ,3,0,"000"      );
        case(12):         return set_vflgs2(ap,""          ,0,""            ,"teor"       ,4,0,"0000"     );
        case(15):         return set_vflgs2(ap,""          ,0,""            ,"ter"        ,3,0,"000"      );
        case(13):   //  fall thro
        case(14):         return set_vflgs2(ap,""          ,0,""            ,"teo"        ,3,0,"000"      );
        case(16):   //  fall thro 
        case(17):         return set_vflgs2(ap,""          ,0,""            ,"te"         ,2,0,"00"       );
        }
        break;
    case(ENVSCULPT):
        switch(mode) {
        case(0):          return set_vflgs2(ap,""           ,0,""           ,"0"          ,0,0,""         );
        case(1):          return set_vflgs2(ap,"r"          ,1,"d"          ,"0"          ,0,0,""         );
        case(2):          return set_vflgs2(ap,"r"          ,1,"d"          ,"o"          ,1,0,"0"        );
        }
        break;
    case(TREMENV):        return set_vflgs2(ap,""           ,0,""           ,""           ,0,0,""         );
    case(DCFIX):          return set_vflgs2(ap,""           ,0,""           ,""           ,0,0,""         );
    }
    sprintf(errstr,"Unknown process %d: set_vflgs2()\n",process);
    return(PROGRAM_ERROR);                                                  
}

/****************************** SET_VFLGS2 *********************************/

int set_vflgs2
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

/****************************** DEAL_WITH_SPECIAL_DATA2 *********************************/

int deal_with_special_data2(int process,int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap)
{
    int exit_status;
    if((exit_status = establish_special_data_type2(process,mode,ap))<0)
        return(exit_status);
    if(ap->special_data) {
        if((exit_status = setup_special_data2(process,mode,srate,duration,nyquist,wlength,channels,ap))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/****************************** ESTABLISH_SPECIAL_DATA_TYPE2 *********************************/

int establish_special_data_type2(int process,int mode,aplptr ap)
{
    ap->special_data = 0;
    switch(process) {
    case(TAPDELAY):     ap->special_data = TAPDELAY_DATA;   break;
    case(RMVERB):       ap->special_data = TAPDELAY_OPTION; break;
    case(PSOW_SYNTH):   
        switch(mode) {
        case(0):
        case(1):
            ap->special_data = SYNTHBANK;
            break;
        case(2):
        case(3):
            ap->special_data = TIMEVARYING_SYNTHBANK;
            break;
        default:
            break;
        }
        break;
    case(PSOW_REINF):
        switch(mode) {
        case(0):
            ap->special_data = PSOW_REINFORCEMENT;
            break;
        case(1):
            ap->special_data = PSOW_INHARMONICS;
            break;
        }
        break;
    case(SPECSLICE):
        if(mode == 3)
            ap->special_data = P_BRK_DATA;
        break;
    case(FOFEX_EX):
        if(mode != 1)
            ap->special_data = FOFEX_EXCLUDES;
        break;
    case(FOFEX_CO):
        ap->special_data = FOFBANK_INFO;
        break;
    case(MCHANPAN):
        switch(mode) {
        case(0):
            ap->special_data = MCHANDATA;
            break;
        case(1):
            ap->special_data = MCHANDATA2;
            break;
        case(4):
        case(5):
            ap->special_data = ANTIPHON;
            break;
        case(6):
            ap->special_data = CROSSPAN;
            break;
        }
        break;
        
        break;
    case(TEX_MCHAN):
        ap->special_data = TEX_NOTEDATA;    
        break;
    case(MANYSIL):
        ap->special_data = MANYSIL_DATA;    
        break;
    case(RETIME):
        switch(mode) {
        case(0):
            ap->special_data = RETIME_DATA;
            break;
        case(1):
            ap->special_data = IDEAL_DATA;
            break;
        case(5):
        case(6):
            ap->special_data = RETEMPO_DATA;
            break;
        case(8):
            ap->special_data = RETIME_MASK;
            break;
        case(11):
            ap->special_data = RETIME_FNAM;
            break;
        }
        break;
    case(FRAME):
        switch(mode) {
        case(0):
        case(1):
        case(2):
        case(6):
            ap->special_data = FRAMEDATA;
            break;
        }
        break;
    case(WRAPPAGE):
        ap->special_data = WRAP_FOCUS;
        break;
    case(MCHSTEREO):
        ap->special_data = OCHANDATA;
        break;
    case(FLUTTER):
        ap->special_data = FLUTTERDATA;
        break;
    case(CHANNELX):
        ap->special_data = CHANXDATA;
        break;
    case(CHORDER):
        ap->special_data = CHORDATA;
        break;
    case(MCHZIG):
        if(mode==1)
            ap->special_data = ZIGDATA;
        break;
    case(SUPERACCU):
        if(mode > 1)
            ap->special_data = TUNING;
        break;
    case(TUNEVARY):
        ap->special_data = TUNELOW_DATA;
        break;
    case(ISOLATE):
        switch(mode) {
        case(ISO_SEGMNT):   ap->special_data = ISOLATES;    break;
        case(ISO_GROUPS):   ap->special_data = ISOGROUPS;   break;
        case(ISO_SLICED):   ap->special_data = ISOSLICES;   break;
        case(ISO_OVRLAP):   ap->special_data = ISOSYLLS;    break;
        }
        break;
    case(PANORAMA):
        if(mode == 1)
            ap->special_data = PANOLSPKRS;
        break;
    case(PACKET):           ap->special_data = PAK_TIMES;    break;
    case(SYNTHESIZER):
        if(mode != 3)
            ap->special_data = SYN_PARTIALS;
        break;
    case(CERACU):           ap->special_data = CYCLECNTS;    break;
    case(NEWTEX):
        if(mode == 0)
            ap->special_data = NTEX_TRANPOS;
        break;
    case(MADRID):
        if(mode == 1)
            ap->special_data = MAD_SEQUENCE;
        break;
    case(SHIFTER):
        ap->special_data = SHFCYCLES;
        break;
    case(FRACTURE):
        ap->special_data = ENVSERIES;
        break;
    case(SPEKLINE):
        ap->special_data = SPEKLDATA;
        break;
    case(SHRINK):
        if(mode == SHRM_LISTMX)
            ap->special_data = SHRFOC;
        break;
    case(SPECMORPH2):
        if(mode > 0)
            ap->special_data = MPEAKS;
        break;
    case(ITERLINE):
        ap->special_data = ITERTRANS;
        break;
    case(ITERLINEF):
        ap->special_data = ITERTRANSF;
        break;
    case(PULSER3):
        if(mode == 0)
            ap->special_data = SYN_SPEK;
        else
            ap->special_data = SYN_PARTIALS;
        break;
    case(SYNFILT):
        if(mode == 0)
            ap->special_data = SYN_FILTERBANK;
        else
            ap->special_data = TIMEVARYING_FILTERBANK;
        break;
    case(STRANDS):
        if(mode == 2)
            ap->special_data = COUTHREADS;
        break;
    case(SPECTUNE):
        if((mode != 3) && mode > 0)
            ap->special_data = TUNINGLIST;
        break;
    case(ROTOR):
        ap->special_data = ROTORDAT;
        break;
    case(TESSELATE):
        ap->special_data = TESSELATION;
        break;
    case(CRYSTAL):
        ap->special_data = CRYSTALDAT;
        break;
    case(CASCADE):
        if(mode >= 5)
            ap->special_data = CASCLIPS;
        break;
    case(FRACTAL):
    case(FRACSPEC):
        ap->special_data = FRACSHAPE;
        break;
    case(REPEATER):
        ap->special_data = REPEATDATA;
        break;
    case(VERGES):
        ap->special_data = VERGEDATA;
        break;
    case(DISTMARK):
        ap->special_data = MARKLIST;
        break;
    case(MOTOR):
        if(mode % 3 == 1)
            ap->special_data = MOTORDATA;
        break;
    case(STUTTER):
        ap->special_data = MOTORDATA;
        break;
    case(SCRUNCH):
        if((mode > 3 && mode < 8) || mode > 9)
            ap->special_data = VERGEDATA;
        break;
    case(TWEET):
        ap->special_data = FOFEX_EXCLUDES;
        break;
    case(SPECFNU):
        switch(mode) {
        case(F_MAKEFILT):
            ap->special_data = FFILT;
            break;
        case(F_PQUANT):
            ap->special_data = HFIELD;
            break;
        case(F_PINVERT):
            ap->special_data = INTERVAL_MAPPING;
            break;
        case(F_PCHRAND): // fall thro
        case(F_SINUS):
            ap->special_data = HFIELD_OR_ZERO;
            break;
        }
        break;
    case(PULSER):
    case(PULSER2):
        ap->special_data = SPACEDATA;
        break;
    case(SUPPRESS):
        ap->special_data = MANYCUTS;
        break;
    case(MATRIX):
        if(mode == MATRIX_USE)
            ap->special_data = MATRIX_DATA;
        break;
    case(ENVSPEAK):
        if(mode >= 12)
            ap->special_data = XSPK_CUTS;
        break;
    case(EXTSPEAK):
        if(mode == 2 || mode == 5)
            ap->special_data = XSPK_PATTERN;
        else if(mode == 6 || mode == 7 || mode == 9 || mode == 10)
            ap->special_data = XSPK_CUTS;
        else if(mode == 8 || mode == 11)
            ap->special_data = XSPK_CUTPAT;
        else if(mode == 12 || mode == 13 || mode == 15 || mode == 16)
            ap->special_data = XSPK_CUTARG;
        else if(mode == 14 || mode == 17)
            ap->special_data = XSPK_CUPATA;
        break;
    case(PEAKCHOP):
        if(mode == 2)
            ap->special_data = RHYTHM;
        break;
    }
    return(FINISHED);
}

/************************ SETUP_SPECIAL_DATA2 *********************/

int setup_special_data2(int process,int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap)
{
    int exit_status;

    if((exit_status = setup_special_data_ranges2(mode,srate,duration,nyquist,wlength,channels,ap))<0)
        return(exit_status);
    if((exit_status = setup_special_data_names2(process,mode,ap))<0)
        return(exit_status);
    return(FINISHED);
}

/************************ SETUP_SPECIAL_DATA_RANGES2 *********************/

int setup_special_data_ranges2(int mode,int srate,double duration,double nyquist,int wlength,int channels,aplptr ap)
{
    switch(ap->special_data) {
    case(TAPDELAY_DATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 32767.0;
        ap->other_special_range = TRUE;
        ap->min_special2        = -16;
        ap->max_special2        = 16;
        break;
    case(TAPDELAY_OPTION):
        ap->data_in_file_only   = FILE_OR_ZERO;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 32767.0;
        ap->other_special_range = TRUE;
        ap->min_special2        = -16;
        ap->max_special2        = 16;
        break;
    case(SYNTHBANK):
    case(TIMEVARYING_SYNTHBANK):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        switch(mode) {
        case(0):
        case(2):
            ap->min_special     = 0.1;
            ap->max_special     = nyquist/2.0;
            break;
        case(1):
        case(3):
            ap->min_special     = unchecked_hztomidi(0.1);
            ap->max_special     = MIDIMAX;
            break;
        }
        ap->other_special_range = TRUE;
        ap->min_special2        = 0.0;
        ap->max_special2        = 1.0;
        break;
    case(PSOW_REINFORCEMENT):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 2;
        ap->max_special         = 256;
        ap->other_special_range = TRUE;
        ap->min_special2        = FLTERR;
        ap->max_special2        = 16.0;
        break;
    case(PSOW_INHARMONICS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 1;
        ap->max_special         = 256;
        ap->other_special_range = TRUE;
        ap->min_special2        = FLTERR;
        ap->max_special2        = 16.0;
        break;
    case(P_BRK_DATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = SPEC_MINFRQ;
        ap->max_special         = (nyquist * 2.0)/3.0;
        ap->other_special_range = FALSE;
        break;
    case(FOFEX_EXCLUDES):
        ap->data_in_file_only   = FALSE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = round(duration * srate);
        ap->default_special     = 0;
        break;
    case(FOFBANK_INFO):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 1;
        ap->max_special         = round(duration * 4000);       // 4000 guestimate of highest poss fof frq
        ap->min_special2        = srate/4000;
        ap->max_special2        = ceil(srate/MINPITCH);
        break;
    case(MCHANDATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 16;                           // assumes max output chans = 16
        ap->min_special2        = -1;
        ap->max_special2        = 1;
        break;
    case(MCHANDATA2):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 16;                           // assumes max output chans = 16
        break;
    case(ANTIPHON):
        ap->data_in_file_only   = FNAM_STRING;
        ap->special_range       = FALSE;
        break;
    case(CROSSPAN):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(TEX_NOTEDATA):
        ap->data_in_file_only   = TRUE;
        break;
    case(MANYSIL_DATA):
        ap->data_in_file_only   = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 3600;
        break;
    case(RETIME_DATA):
        ap->data_in_file_only   = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 32767;
        break;
    case(RETEMPO_DATA):
        ap->data_in_file_only   = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 3600;
        break;
    case(RETIME_MASK):
        ap->data_in_file_only   = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 1;
        break;
    case(IDEAL_DATA):
        ap->data_in_file_only   = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 32767;
        break;
    case(RETIME_FNAM):
        ap->data_in_file_only   = FNAM_STRING;
        ap->special_range       = FALSE;
        break;
    case(FRAMEDATA):
        ap->data_in_file_only   = FALSE;
        if(mode==6)
            ap->min_special     = 1;
        else
            ap->min_special     = 0;
        ap->max_special         = channels;
        break;
    case(WRAP_FOCUS):
        ap->special_range       = FALSE;
        ap->min_special         = 0;
        ap->max_special         = 16;
        ap->min_special2        = -1;
        ap->max_special2        = 1;
        break;
    case(OCHANDATA):
        ap->data_in_file_only   = FALSE;
        ap->min_special         = 1;
        ap->max_special         = 16;
        break;
    case(FLUTTERDATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(CHANXDATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(CHORDATA):
        ap->data_in_file_only   = FNAM_STRING;
        ap->special_range       = FALSE;
        break;
    case(ZIGDATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = ZIG_SPLICELEN * MS_TO_SECS * 3;
        ap->max_special         = duration;
        break;
    case(TUNING):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = SPEC_MINFRQ;
        ap->max_special         = (nyquist * 2.0)/3.0;
        break;
    case(TUNELOW_DATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 127;
        break;
    case(ISOLATES):
    case(ISOGROUPS):
    case(ISOSLICES):
    case(ISOSYLLS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(PANOLSPKRS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 360;
        break;
    case(PAK_TIMES):
        ap->data_in_file_only   = FALSE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = duration;
        break;
    case(SYN_PARTIALS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(NTEX_TRANPOS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(MAD_SEQUENCE):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(SHFCYCLES):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 2;
        ap->max_special         = 32767;
        break;
    case(SPEKLDATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(ENVSERIES):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 1;
        break;
    case(SHRFOC):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(MPEAKS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = FLTERR;
        ap->max_special         = nyquist;
        break;
    case(ITERTRANS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = -24.0;
        ap->max_special         = 24.0;
        break;
    case(ITERTRANSF):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = -12.0;
        ap->max_special         = 12.0;
        break;
    case(CYCLECNTS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 1;
        ap->max_special         = 100;
        break;
    case(SYN_SPEK):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(SYN_FILTERBANK):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = unchecked_hztomidi(FLT_MINFRQ);
        ap->max_special         = MIDIMAX;
        break;
    case(TIMEVARYING_FILTERBANK):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = unchecked_hztomidi(FLT_MINFRQ);
        ap->max_special         = MIDIMAX;
        ap->other_special_range = TRUE;
        ap->min_special2        = FLT_MINGAIN;
        ap->max_special2        = FLT_MAXGAIN;
        break;
    case(COUTHREADS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 2;
        ap->max_special         = 100;
        break;
    case(TUNINGLIST):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 4;
        ap->max_special         = 127;
        break;
    case(ROTORDAT):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 1;
        break;
    case(TESSELATION):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(CRYSTALDAT):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(CASCLIPS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(FRACSHAPE):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        if(mode == 0) {
            ap->min_special         = FRAC_MINTRNS;
            ap->max_special         = FRAC_MAXTRNS;
        } else {
            ap->min_special         = FRAC_MINMIDI;
            ap->max_special         = FRAC_MAXMIDI;
        }
        break;
    case(REPEATDATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(VERGEDATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = duration;
        break;
    case(MOTORDATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = MOT_SPLICE * MS_TO_SECS * 2;
        ap->max_special         = duration - (MOT_SPLICE * MS_TO_SECS * 2);
        break;
    case(FFILT):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(HFIELD):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(HFIELD_OR_ZERO):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = FALSE;
        break;
    case(INTERVAL_MAPPING):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = -MAXINTRANGE;
        ap->max_special         = MAXINTRANGE;
        break;
    case(MARKLIST):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = duration - 0.0005;
        break;
    case(MANYCUTS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = duration;
        break;
    case(SPACEDATA):
        ap->data_in_file_only   = FALSE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 12345678;
        break;
    case(MATRIX_DATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = 4;
        break;
    case(XSPK_PATTERN):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 1;
        ap->max_special         = 1000;
        break;
    case(XSPK_CUTS):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = duration;
        break;
    case(XSPK_CUTPAT):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = duration;
        ap->other_special_range = TRUE;
        ap->min_special2        = 1;
        ap->max_special2        = 1000;
        break;
    case(XSPK_CUTARG):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = duration;
        ap->other_special_range = TRUE;
        ap->min_special2        = 1;
        ap->max_special2        = 1000;
        break;
    case(XSPK_CUPATA):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 0;
        ap->max_special         = duration;
        ap->other_special_range = TRUE;
        ap->min_special2        = 1;
        ap->max_special2        = 1000;
        break;
    case(RHYTHM):
        ap->data_in_file_only   = TRUE;
        ap->special_range       = TRUE;
        ap->min_special         = 1;
        ap->max_special         = 8;
        break;
    default:
        sprintf(errstr,"Unknown special_data type: setup_special_data_ranges2()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/************************ SETUP_SPECIAL_DATA_NAMES2 *********************/

int setup_special_data_names2(int process,int mode,aplptr ap)
{
    switch(ap->special_data) {
    case(TAPDELAY_DATA):    ap->special_data_name   = "DELAY_TIMES_AMPS_(&_POSITIONS)"; break;
    case(TAPDELAY_OPTION):  ap->special_data_name   = "DELAY_TIMES_AMPS_(&_POSITIONS)"; break;
    case(SYNTHBANK):
        switch(mode) {
        case(0):
        case(2):
            ap->special_data_name   = "FRQ_AND_AMP_OF_OSCILLATORS"; 
            break;
        case(1):
        case(3):
            ap->special_data_name   = "PITCH(MIDI)_&_AMP_OF_OSCILLATORS";
            break;
        }
        break;
    case(TIMEVARYING_SYNTHBANK):
        switch(mode) {
        case(0):
        case(2):
            ap->special_data_name   = "TIMELIST_OF_FRQS&AMPS_OF_OSCILLATORS";
            break;
        case(1):
        case(3):
            ap->special_data_name   = "TIMELIST_OF_PITCHES&AMPS_OF_OSCILLATORS";
            break;
        }
        break;
    case(PSOW_REINFORCEMENT):
        ap->special_data_name       = "HARMONIC_NO_AND_AMPLITUDE_PAIR";
        break;
    case(PSOW_INHARMONICS):
        ap->special_data_name       = "INHARMONIC_NO_AND_AMPLITUDE_PAIR";
        break;
    case(P_BRK_DATA):
        ap->special_data_name       = "PITCH_BRKPOINT_TEXTFILE";
        break;
    case(FOFEX_EXCLUDES):
        if(process == TWEET)
            ap->special_data_name       = "TIMEBLOKS_IN_SRC_NOT_TO_SEARCH_FOR_FOFS";
        else
            ap->special_data_name       = "AREAS_(IN_SAMPLECNTS)_NOT_TO_SEARCH_FOR_FOFS";
        break;
    case(FOFBANK_INFO):
        ap->special_data_name       = "FOFBANK_INFORMATION";
        break;
    case(MCHANDATA):
        ap->special_data_name       = "PANNING_DATA";
        break;
    case(MCHANDATA2):
        ap->special_data_name       = "OUT-CHANNEL_SEQUENCE";
        break;
    case(ANTIPHON):
        ap->special_data_name       = "ANTIPHONAL_CHANNELS_SETS(e.g._abcd-efgh)";
        break;
    case(CROSSPAN):
        ap->special_data_name       = "CHANNEL_CONFIGURATIONS";
        break;
    case(TEX_NOTEDATA):
        ap->special_data_name       = "NOTE_DATA";
        break;
    case(MANYSIL_DATA):
        ap->special_data_name       = "SILENCE_LOCATIONS_&_DURATIONS";
        break;
    case(RETIME_DATA):
        ap->special_data_name       = "LOCATIONS_OF_BEATS";
        break;
    case(RETEMPO_DATA):
        if(mode == 5)
            ap->special_data_name       = "BEAT_LOCATIONS_OF_EVENTS";
        else
            ap->special_data_name       = "TIME_LOCATIONS_OF_EVENTS";
        break;
    case(RETIME_MASK):
        ap->special_data_name       = "MASKING_PATTERN";
        break;
    case(IDEAL_DATA):
        ap->special_data_name       = "RELOCATION_OF_BEATS";
        break;
    case(RETIME_FNAM):
        ap->special_data_name       = "OUTPUT_TEXTFILE";
        break;
    case(FRAMEDATA):
        switch(mode) {
        case(2):
            ap->special_data_name   = "REORIENTATION";
            break;
        case(6):
            ap->special_data_name   = "CHANNELS_TO_MODIFY";
            break;
        default:
            ap->special_data_name   = "SNAKING_SEQUENCE";
            break;
        }
        break;
    case(WRAP_FOCUS):
        ap->special_data_name       = "CENTRE_OF_SPREAD";
        break;
    case(OCHANDATA):
        ap->special_data_name       = "OUT_CHANNEL_ON_WHICH_STEREO_INPUT_CENTRED";
        break;
    case(FLUTTERDATA):
        ap->special_data_name       = "CHANNEL-SET_SEQUENCE";
        break;
    case(CHANXDATA):
        ap->special_data_name       = "INPUT_CHANNELS_TO_EXTRACT";
        break;
    case(CHORDATA):
        ap->special_data_name       = "REORDERING_STRING";
        break;
    case(ZIGDATA):
        ap->special_data_name       = "ZIGZAG_TIMES";
        break;
    case(TUNING):
        ap->special_data_name       = "TUNING_FREQUENCIES";
        break;
    case(TUNELOW_DATA):
        ap->special_data_name       = "TIMES_AND_(MIDI)_TUNINGS";
        break;
    case(ISOLATES):
    case(ISOGROUPS):
        ap->special_data_name       = "SEGMENT_CUT_TIMES";
        break;
    case(ISOSLICES):
    case(ISOSYLLS):
        ap->special_data_name       = "SLICE_TIMES";
        break;
    case(PANOLSPKRS):
        ap->special_data_name       = "ANGULAR_POSITIONS_OF_LSPKRS";
        break;
    case(PAK_TIMES):
        ap->special_data_name       = "PACKET_LOCATION(S)_IN_SOURCE";
        break;
    case(SYN_PARTIALS):
        ap->special_data_name       = "PARTIALS_DATA";
        break;
    case(NTEX_TRANPOS):
        ap->special_data_name       = "TRANSPOSITION_DATA";
        break;
    case(MAD_SEQUENCE):
        ap->special_data_name       = "SEQUENCE_DATA";
        break;
    case(SHFCYCLES):
        ap->special_data_name       = "CYCLE_LENGTHS";
        break;
    case(SPEKLDATA):
        ap->special_data_name       = "SPECTRAL_LINES_DATA";
        break;
    case(ENVSERIES):
        ap->special_data_name       = "ENVELOPE_SERIES";
        break;
    case(SHRFOC):
        ap->special_data_name       = "PEAK_TIMES";
        break;
    case(CYCLECNTS):
        ap->special_data_name       = "CYCLE_COUNTS";
        break;
    case(MPEAKS):
        ap->special_data_name       = "PEAK_FRQS";
        break;
    case(ITERTRANS):
    case(ITERTRANSF):
        ap->special_data_name       = "TRANSPOSITION_DATA";
        break;
    case(SYN_SPEK):
        ap->special_data_name       = "PARTIAL_NUMBERS_AND_LEVELS";
        break;
    case(SYN_FILTERBANK):
        ap->special_data_name   = "TIMELIST_OF_PITCHES_OF_FILTS";
        break;
    case(TIMEVARYING_FILTERBANK):
        ap->special_data_name   = "TIMELIST_OF_PICHS&AMPS_OF_FILTS";
        break;
    case(COUTHREADS):
        ap->special_data_name   = "THREADCOUNTS_IN_EACH_BAND";
        break;
    case(TUNINGLIST):
        ap->special_data_name   = "TUNING-PITCHES_FILE";
        break;
    case(ROTORDAT):
        ap->special_data_name   = "NOTE-EVENT_ENVELOPE";
        break;
    case(TESSELATION):
        ap->special_data_name   = "TESSELATION_DATA";
        break;
    case(CRYSTALDAT):
        ap->special_data_name   = "CRYSTAL_VERTEX_DATA";
        break;
    case(CASCLIPS):
        ap->special_data_name   = "CUT_TIMES_IN_SOURCE";
        break;
    case(FRACSHAPE):
        if(mode == 0)
            ap->special_data_name = "SEMITONE_TRANSPOSITION_PATTERN";
        else
            ap->special_data_name = "MIDI_PITCH_PATTERN";
        break;
    case(REPEATDATA):
        ap->special_data_name   = "REPEATED_SEGMENTS_DATA";
        break;
    case(VERGEDATA):
        if(process == SCRUNCH)
            ap->special_data_name   = "SLICE_TIMES";
        else
            ap->special_data_name   = "VERGE_TIMES";
        break;
    case(MOTORDATA):
        ap->special_data_name   = "SLICE_TIMES";
        break;
    case(FFILT):
        ap->special_data_name   = "TIMINGS_&_PITCH_GRID";
        break;
    case(HFIELD):
        ap->special_data_name   = "PITCH_GRID";
        break;
    case(HFIELD_OR_ZERO):
        ap->special_data_name   = "PITCH_GRID(OR_ZERO)";
        break;
    case(INTERVAL_MAPPING):
        ap->special_data_name   = "INTERVAL_MAP";
        break;
    case(MARKLIST):
        ap->special_data_name   = "TIME_MARKS_IN_SOURCE";
        break;
    case(MANYCUTS):
        ap->special_data_name   = "TIMESLOTS";
        break;
    case(SPACEDATA):
        ap->special_data_name   = "LSPKR_SELECTION_(8CHAN_OUT_ONLY)";
        break;
    case(MATRIX_DATA):
        ap->special_data_name   = "MATRIX_DATA_FILE";
        break;
    case(XSPK_PATTERN):
        ap->special_data_name   = "PATTERN_OF_INSERTS";
        break;
    case(XSPK_CUTS):
        ap->special_data_name   = "SYLLABLE_BOUNDARY_TIMES";
        break;
    case(XSPK_CUTPAT):
        ap->special_data_name   = "SYLLABLE_BOUNDARY_TIMES_&_PATTERN_OF_INSERTS";
        break;
    case(XSPK_CUTARG):
        ap->special_data_name   = "SYLLABLE_BOUNDARY_TIMES_&_SYLLABLES_TO_TARGET";
        break;
    case(XSPK_CUPATA):
        ap->special_data_name   = "SYLLAB_BOUNDARY_TIMES,_SYLLABS_TO_TARGET_&_INSERTS_PATTERN";
        break;
    case(RHYTHM):
        ap->special_data_name   = "RHYTHM_CELL";
        break;
    default:
        sprintf(errstr,"Unknown special_data type: setup_special_data_names2()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/**************************** DEAL_WITH_FORMANTS2 *************************/

int deal_with_formants2(int process,int mode,int channels,aplptr ap)
{
    int exit_status;
    set_formant_flags2(process,mode,ap);
    if(ap->formant_flag) {
        if((exit_status = establish_formant_band_ranges2(channels,ap))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/**************************** SET_FORMANT_FLAGS2 *************************/

void set_formant_flags2(int process,int mode,aplptr ap)
{
    switch(process) {
    case(TAPDELAY):
    case(RMRESP):
    case(RMVERB):   
    case(MIXMULTI):
    case(ANALJOIN):
    case(PTOBRK):
    case(PSOW_STRETCH):
    case(PSOW_DUPL):
    case(PSOW_DEL):
    case(PSOW_STRFILL):
    case(PSOW_FREEZE):
    case(PSOW_CHOP):
    case(PSOW_INTERP):
    case(PSOW_FEATURES):
    case(PSOW_SYNTH):
    case(PSOW_IMPOSE):
    case(PSOW_SPLIT):
    case(PSOW_SPACE):
    case(PSOW_INTERLEAVE):
    case(PSOW_REPLACE):
    case(PSOW_EXTEND):
    case(PSOW_EXTEND2):
    case(PSOW_LOCATE):
    case(PSOW_CUT):
    case(ONEFORM_GET):
    case(ONEFORM_COMBINE):
    case(NEWGATE):
    case(SPEC_REMOVE):
    case(PREFIXSIL):
    case(STRANS):
    case(PSOW_REINF):
    case(PARTIALS_HARM):  
    case(SPECROSS):
    case(LUCIER_GETF):
    case(LUCIER_GET):
    case(LUCIER_PUT):
    case(LUCIER_DEL):
    case(SPECTRACT):
    case(SPECLEAN):
    case(PHASE):
    case(SPECSLICE):
    case(FOFEX_EX):
    case(FOFEX_CO):
    case(GREV_EXTEND):
    case(PEAKFIND):
    case(CONSTRICT):
    case(EXPDECAY):
    case(PEAKCHOP):
    case(MCHANPAN):
    case(TEX_MCHAN):
    case(MANYSIL):
    case(RETIME):
    case(HOVER):
    case(HOVER2):
    case(MULTIMIX):
    case(FRAME):
    case(SEARCH):
    case(MCHANREV):
    case(WRAPPAGE):
    case(MCHSTEREO):
    case(MTON):
    case(FLUTTER):
    case(ABFPAN):
    case(ABFPAN2):
    case(ABFPAN2P):
    case(CHANNELX):
    case(CHORDER):
    case(FMDCODE):
    case(CHXFORMAT):
    case(CHXFORMATG):
    case(CHXFORMATM):
    case(INTERLX):
    case(COPYSFX):
    case(NJOIN):
    case(NJOINCH):
    case(NMIX):
    case(RMSINFO):
    case(SFEXPROPS):
    case(SETHARES):
    case(MCHSHRED):
    case(MCHZIG):
    case(MCHITER):
    case(SPECSPHINX):
    case(SPECMORPH):
    case(SPECMORPH2):
    case(SUPERACCU):
    case(PARTITION):
    case(SPECGRIDS):
    case(GLISTEN):
    case(TUNEVARY):
    case(ISOLATE):
    case(REJOIN):
    case(PANORAMA):
    case(TREMOLO):
    case(ECHO):
    case(PACKET):
    case(SYNTHESIZER):
    case(NEWTEX):
    case(CERACU):
    case(MADRID):
    case(SHIFTER):
    case(SUBTRACT):
    case(SPEKLINE):
    case(FRACTURE):
    case(TAN_ONE):
    case(TAN_TWO):
    case(TAN_SEQ):
    case(TAN_LIST):
    case(SPECTWIN):
    case(TRANSIT):
    case(TRANSITF):
    case(TRANSITD):
    case(TRANSITFD):
    case(TRANSITS):
    case(TRANSITL):
    case(CANTOR):
    case(SHRINK):
    case(NEWDELAY):
    case(FILTRAGE):
    case(SELFSIM):
    case(ITERFOF):
    case(ITERLINE):
    case(ITERLINEF):
    case(SPECRAND):
    case(SPECSQZ):
    case(PULSER):
    case(PULSER2):
    case(PULSER3):
    case(CHIRIKOV):
    case(MULTIOSC):
    case(SYNFILT):
    case(STRANDS):
    case(REFOCUS):
    case(CHANPHASE):
    case(SILEND):
    case(SPECULATE):
    case(SPECTUNE):
    case(REPAIR):
    case(DISTSHIFT):
    case(QUIRK):
    case(ROTOR):
    case(DISTCUT):
    case(ENVCUT):
    case(SPECFOLD):
    case(BROWNIAN):
    case(SPIN):
    case(SPINQ):
    case(CRUMBLE):
    case(PHASOR):
    case(TESSELATE):
    case(CRYSTAL):
    case(WAVEFORM):
    case(DVDWIND):
    case(CASCADE):
    case(SYNSPLINE):
    case(SPLINTER):
    case(REPEATER):
    case(VERGES):
    case(MOTOR):
    case(STUTTER):
    case(SCRUNCH):
    case(IMPULSE):
    case(TWEET):
    case(RRRR_EXTEND):
    case(SORTER):
    case(SPECFNU):
    case(FLATTEN):
    case(BOUNCE):
    case(DISTMARK):
    case(DISTREP):
    case(TOSTEREO):
    case(SUPPRESS):
    case(CALTRAIN):
    case(SPECENV):
    case(CLIP):
    case(SPECEX):
    case(MATRIX):
    case(TRANSPART):
    case(SPECINVNU):
    case(SPECCONV):
    case(SPECSND):
    case(SPECFRAC):
    case(FRACTAL):
    case(FRACSPEC):
    case(ENVSPEAK):
    case(EXTSPEAK):
    case(ENVSCULPT):
    case(TREMENV):
    case(DCFIX):
        break;
    case(ONEFORM_PUT):
        ap->formant_qksrch = TRUE;
        break;
    }
}

/**************************** ESTABLISH_FORMANT_BAND_RANGE2 *************************/

int establish_formant_band_ranges2(int channels,aplptr ap)
{
    int clength = channels/2;
    int clength_less_one = clength - 1;
    if(clength_less_one < 0) {
        sprintf(errstr,"Invalid call to process formants: establish_formant_band_ranges2()\n");
        return(PROGRAM_ERROR);
    }
    if(clength_less_one < LOW_OCTAVE_BANDS) {
        ap->no_pichwise_formants = TRUE;
    } else
        ap->max_pichwise_fbands = MAX_BANDS_PER_OCT;
    ap->max_freqwise_fbands = (clength_less_one)/2;
    return(FINISHED);
}

/******************************* GET_PARAM_NAMES2 *******************************/

int get_param_names2(int process,int mode,int total_params,aplptr ap)
{
    if((ap->param_name = (char **)malloc((total_params) * sizeof(char *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: to get_param_names2\n"); 
        return(MEMORY_ERROR);
    }

    switch(process) {

    case(TAPDELAY):
        ap->param_name[0]           = "OUTPUT_GAIN";
        ap->param_name[1]           = "FEEDBACK";
        ap->param_name[2]           = "SOURCE_SIGNAL_IN_MIX";
        ap->param_name[3]           = "DECAY_TAIL_DURATION";
        break;
    case(RMRESP):
        ap->param_name[0]  = "LIVENESS";
        ap->param_name[1]  = "NUMBER_OF_REFLECTIONS";
        ap->param_name[2]  = "ROOM_LENGTH";
        ap->param_name[3]  = "ROOM_WIDTH";
        ap->param_name[4]  = "ROOM_HEIGHT";
        ap->param_name[5]  = "POSITION_OF_SRC_LENGTHWAYS";
        ap->param_name[6]  = "POSITION_OF_SRC_WIDTHWAYS";
        ap->param_name[7]  = "HEIGHT_OF_SRC";
        ap->param_name[8]  = "POSITION_OF_LISTENER_LENGTHWAYS";
        ap->param_name[9]  = "POSITION_OF_LISTENER_WIDTHWAYS";
        ap->param_name[10] = "HEIGHT_OF_LISTENER";
        ap->param_name[11] = "PEAK_AMPLITUDE_OF_DATA";
        ap->param_name[12] = "REFLECTION_TIME_RESOLUTION_(MS)";
        break;
    case(RMVERB):
        ap->param_name[0]  = "ROOM_SIZE_(1_SMALL_:_3_LARGE)";
        ap->param_name[1]  = "DENSE_REVERB_GAIN";
        ap->param_name[2]  = "SOURCE_SIGNAL_IN_MIX";
        ap->param_name[3]  = "FEEDBACK";
        ap->param_name[4]  = "AIR-ABSORPTION_FILTER_CUTOFF";
        ap->param_name[5]  = "LOWPASS_REVERB-INPUT_CUTOFF";
        ap->param_name[6]  = "DECAY_TAIL_DURATION";
        ap->param_name[7]  = "LOWPASS_INPUT_CUTOFF_(zero_to_disable)";
        ap->param_name[8]  = "HIGHPASS_INPUT_CUTOFF_(zero_to_disable)";
        ap->param_name[9]  = "REVERB_PREDELAY_(MS)";
        ap->param_name[10] = "NUMBER_OF_OUTPUT_CHANNELS";
        break;
    case(MIXMULTI):
        ap->param_name[MIX_START]   = "MIXING_STARTTIME";
        ap->param_name[MIX_END]     = "MIXING_ENDTIME";
        ap->param_name[MIX_ATTEN]   = "ATTENUATION";
        break;
    case(ANALJOIN):
        break;
    case(PTOBRK):
        ap->param_name[0]   = "MIN DURATION (MS) OF VALID PITCHDATA";
        break;
    case(PSOW_STRETCH):
        ap->param_name[0]  = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1]  = "TIME_STRETCH";
        ap->param_name[2]  = "NO_OF_GRAINS_PER_BLOCK";
        break;
    case(PSOW_DUPL):
        ap->param_name[0]  = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1]  = "NUMBER_OF_DUPLICATIONS";
        ap->param_name[2]  = "NO_OF_GRAINS_PER_BLOCK";
        break;
    case(PSOW_DEL):
        ap->param_name[0]  = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1]  = "ONE_IN_HOW_MANY_GRAINS_TO_KEEP";
        ap->param_name[2]  = "NO_OF_GRAINS_PER_BLOCK";
        break;
    case(PSOW_STRFILL):
        ap->param_name[0]  = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1]  = "TIME_STRETCH";
        ap->param_name[2]  = "NO_OF_GRAINS_PER_BLOCK";
        ap->param_name[3]  = "TRANSPOSITION_IN_SEMITONES";
        break;
    case(PSOW_FREEZE):
        ap->param_name[0]  = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1]  = "GRAB_TIME";
        ap->param_name[2]  = "OUTPUT_DURATION";
        ap->param_name[3]  = "NUMBER_OF_GRAINS_TO_GRAB";
        ap->param_name[4]  = "DENSITY_(PITCH_TRANSPOSITION)";
        ap->param_name[5]  = "SPECTRAL_TRANSPOSITION";
        ap->param_name[6]  = "RANDOMISATION";
        ap->param_name[7]  = "GAIN";
        break;
    case(PSOW_CHOP):
        ap->param_name[0]  = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1]  = "FILE_OF_CUT_TIME_:_CHUNKLEN_PAIRS";
        break;
    case(PSOW_INTERP):
        ap->param_name[PS_SDUR]      = "SUSTAIN_DURATION_OF_FIRST_GRAIN";
        ap->param_name[PS_IDUR]      = "DURATION_OF_INTERPOLATION";
        ap->param_name[PS_EDUR]      = "SUSTAIN_DURATION_OF_FINAL_GRAIN";
        ap->param_name[PS_VIBFRQ]    = "VIBRATO_FREQUENCY";
        ap->param_name[PS_VIBDEPTH]  = "VIBRATO_DEPTH_(SEMITONES)";
        ap->param_name[PS_TREMFRQ]   = "TREMOLO_FREQUENCY";
        ap->param_name[PS_TREMDEPTH] = "TREMOLO_DEPTH";
        break;
    case(PSOW_FEATURES):
        ap->param_name[0] = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1] = "NO_OF_GRAINS_PER_BLOCK";
        switch(mode) {
        case(0): ap->param_name[2] = "TRANSPOSITION_(SEMITONES)_WITH_TIMEWARP";       break;
        case(1): ap->param_name[2] = "TRANSPOSITION_(SEMITONES)_WITH_PITCH_DIVISION"; break;
        }
        ap->param_name[3]  = "VIBRATO_FREQUENCY";
        ap->param_name[4]  = "VIBRATO_DEPTH_(SEMITONES)";
        ap->param_name[5]  = "SPECTRAL_TRANSPOSITION_(SEMITONES)";
        ap->param_name[6]  = "HOARSENESS";
        ap->param_name[7]  = "ATTENUATION";
        ap->param_name[8]  = "SUBHARMONIC_NUMBER";
        ap->param_name[9]  = "SUBHARMONIC_LEVEL";
        ap->param_name[10] = "FOF_STRETCHING";
        break;
    case(PSOW_SYNTH):
        ap->param_name[0] = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1] = "DEPTH_OF_FOF_CONTOURING";
        break;
    case(PSOW_IMPOSE):
        ap->param_name[0] = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1] = "DEPTH_OF_FOF_CONTOURING";
        ap->param_name[2] = "WINDOW_SIZE_(mS)";
        ap->param_name[3] = "GATE_LEVEL_(dB)";
        break;
    case(PSOW_SPLIT):
        ap->param_name[0] = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1] = "SUBHARMONIC_NO";
        ap->param_name[2] = "UPWARD_TRANSPOSITION_(SEMITONES)";
        ap->param_name[3] = "RELATIVE_LEVEL_OF_UPTRANSPOSED";
        break;
    case(PSOW_SPACE):
        ap->param_name[0] = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1] = "SUBHARMONIC_NO";
        ap->param_name[2] = "SPATIAL_SEPARATION";
        ap->param_name[3] = "LEFT-RIGHT_RELATIVE-LEVEL";
        ap->param_name[4] = "SUPPRESS_HIGH_COMPONENTS";
        break;
    case(PSOW_INTERLEAVE):
        ap->param_name[0] = "PITCH_BRPNT_FILE_(SND_1)";
        ap->param_name[1] = "PITCH_BRPNT_FILE_(SND_2)";
        ap->param_name[2] = "FOFS_PER_CHUNK";
        ap->param_name[3] = "PITCH_BIAS";
        ap->param_name[4] = "RELATIVE-LEVEL";
        ap->param_name[5] = "RELATIVE-WEIGHTING";
        break;
    case(PSOW_REPLACE):
        ap->param_name[0] = "PITCH_BRPNT_FILE_(SND_1)";
        ap->param_name[1] = "PITCH_BRPNT_FILE_(SND_2)";
        ap->param_name[2] = "FOFS_PER_CHUNK";
        break;
    case(PSOW_EXTEND):
        ap->param_name[0]        = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[PS_TIME]  = "FREEZE_TIME";
        ap->param_name[PS_DUR]   = "OUTPUT_DURATION_OF_WHOLE_SOUND";
        ap->param_name[PS_SEGS]  = "NUMBER_OF_GRAINS_TO_GRAB";
        ap->param_name[PSE_VFRQ] = "VIBRATO_FREQUENCY";
        ap->param_name[PSE_VDEP] = "VIBRATO_DEPTH_(SEMITONES)";
        ap->param_name[PSE_TRNS] = "GRAIN_PITCH_TRANSPOSITION_(SEMITONES)";
        ap->param_name[PSE_GAIN] = "LOUDNESS_CONTOUR";
        break;
    case(PSOW_EXTEND2):
        ap->param_name[0]        = "START_TIME_OF_GRAIN";
        ap->param_name[1]        = "END_TIME_OF_GRAIN";
        ap->param_name[PS_DUR]   = "OUTPUT_DURATION_OF_WHOLE_SOUND";
        ap->param_name[PS2_VFRQ] = "VIBRATO_FREQUENCY";
        ap->param_name[PS2_VDEP] = "VIBRATO_DEPTH_(SEMITONES)";
        ap->param_name[PS2_NUJ]  = "MOVE_GRAINTIME_BY_N_ZEROCROSSINGS";
        break;
    case(PSOW_LOCATE):
        ap->param_name[0]        = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[PS_TIME]  = "TIME";
        break;
    case(PSOW_CUT):
        ap->param_name[0]        = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[PS_TIME]  = "TIME";
        break;
    case(ONEFORM_GET):
        ap->param_name[0]  = "TIME_OF_FORMANT";
        break;
    case(ONEFORM_PUT):
        ap->param_name[FORM_FTOP] = "LOW_FRQ_LIMIT";
        ap->param_name[FORM_FBOT] = "HIGH_FRQ_LIMIT";
        ap->param_name[FORM_GAIN] = "GAIN";
        break;
    case(ONEFORM_COMBINE):
        break;
    case(NEWGATE):
        ap->param_name[0] = "GATE_LEVEL_(dB)";
        break;
    case(SPEC_REMOVE):
        switch(mode) {
        case(0):
            ap->param_name[0] = "MINIMUM_PITCH_TO_REMOVE_(MIDI)";
            ap->param_name[1] = "MAXIMUM_PITCH_TO_REMOVE_(MIDI)";
            ap->param_name[2] = "FRQ_OF_HIGHEST_HARMONIC_TO_REMOVE_(HZ)";
            ap->param_name[3] = "ATTENUATION_OF_PITCH_COMPONENT";
            break;
        case(1):
            ap->param_name[0] = "MINIMUM_PITCH_TO_RETAIN_(MIDI)";
            ap->param_name[1] = "MAXIMUM_PITCH_TO_RETAIN_(MIDI)";
            ap->param_name[2] = "FRQ_OF_HIGHEST_HARMONIC_TO_RETAIN_(HZ)";
            ap->param_name[3] = "ATTENUATION_OF_OTHER_COMPONENTS";
            break;
        }
        break;
    case(PREFIXSIL):
        ap->param_name[0]  = "DURATION";
        break;
    case(STRANS):
        switch(mode) {
        case(0):
            ap->param_name[0]  = "SPEED_MULTIPLIER";
            break;
        case(1):
            ap->param_name[0] = "TRANSPOSITION_IN_SEMITONES";
            break;
        case(2):
            ap->param_name[0] = "ACCELERATION";
            ap->param_name[1] = "GOAL_TIME";
            ap->param_name[2] = "START_TIME";
            break;
        case(3):
            ap->param_name[0] = "CYCLES_PER_SECOND";
            ap->param_name[1] = "SEMITONE_DEPTH";
            break;
        }
        break;
    case(PSOW_REINF):
        ap->param_name[0] = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        if(mode == 0)
            ap->param_name[1] = "DELAY_(MS)_OF_HARMONICS";
        else
            ap->param_name[1] = "WEIGHT";
        break;
    case(PARTIALS_HARM):
        ap->param_name[0] = "FUNDAMENTAL_FREQUENCY";
        ap->param_name[1] = "THRESHOLD_AMPLITUDE";
        if(mode > 1)
            ap->param_name[2] = "TIME_OF_WINDOW";
        break;
    case(SPECROSS):
        ap->param_name[PICH_RNGE]   = "IN-TUNE_RANGE_(SEMIT)";
        ap->param_name[PICH_VALID]  = "MIN_WINDOWS_TO_CONFIRM_PITCH";
        ap->param_name[PICH_SRATIO] = "SIGNAL_TO_NOISE_RATIO_(dB)";
        ap->param_name[PICH_MATCH]  = "VALID_HARMONICS_COUNT";
        ap->param_name[PICH_LOLM]   = "LOW_PITCH_LIMIT_(HZ)";
        ap->param_name[PICH_HILM]   = "HIGH_PITCH_LIMIT_(HZ)";
        ap->param_name[PICH_THRESH] = "PARTIAL_THRESHOLD_AMPLITUDE";
        ap->param_name[SPCMPLEV]    = "OUTPUT_LEVEL";
        ap->param_name[SPECHINT]    = "SPECTRAL_INTERPOLATION";
        break;
    case(LUCIER_GETF):
        ap->param_name[LUCIER_CUT]         = "LOW_FRQ_CUTOFF";
        /* fall thro */
    case(LUCIER_GET):
        ap->param_name[MIN_ROOM_DIMENSION] = "MIN_ROOM_DIMENSION_(METRES)";
        ap->param_name[ROLLOFF_INTERVAL]   = "ROLLOFF_INTERVAL";
        break;
    case(LUCIER_PUT):
        ap->param_name[RESON_CNT]          = "RESONANCE_COUNT";
        ap->param_name[RES_EXTEND_ATTEN]   = "OCTAVE_DUPLICATION_ROLLOFF";
        break;
    case(LUCIER_DEL):
        ap->param_name[SUPR_COEFF]         = "SUPPRESSION";
        break;
    case(SPECTRACT):
    case(SPECLEAN):
        ap->param_name[0]   = "PERSISTANCE_(mS)";
        ap->param_name[1]   = "NOISE_PREGAIN";
        break;
    case(PHASE):
        if(mode == 1)
            ap->param_name[0]   = "PHASE_TRANSFER";
        break;
    case(SPECSLICE):
        switch(mode) {
        case(0):
            ap->param_name[0]   = "NUMBER_OF_SLICES";
            ap->param_name[1]   = "ANALYSIS_CHANNEL_GROUPING";
            break;
        case(1):
            ap->param_name[0]   = "NUMBER_OF_SLICES";
            ap->param_name[1]   = "SLICE_BANDWIDTH";
            break;
        case(2):
            ap->param_name[0]   = "NUMBER_OF_SLICES";
            ap->param_name[1]   = "SLICE_WIDTH_IN_SEMITONES";
            break;
        case(4):
            ap->param_name[0]   = "FREQUENCY_PIVOT";
            break;
        }
        break;
    case(FOFEX_CO):
        ap->param_name[0] = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        ap->param_name[1] = "LOUDNESS_ENVELOPE";
        ap->param_name[2] = "OVERALL_GAIN";
        switch(mode) {
        case(FOF_MEASURE):
            break;
        case(FOF_SINGLE):
            ap->param_name[3] = "FOF_NUMBER";
            break;
        case(FOF_LOHI):
            ap->param_name[3] = "FOF_NUMBER_FOR_LO_PITCHES";
            ap->param_name[4] = "FOF_NUMBER_FOR_HI_PITCHES";
            ap->param_name[5] = "MIN_FRQ_FROM_WHICH_TO_VARY_QUALITY";
            ap->param_name[6] = "MAX_FRQ_FOR_WHICH_TO_VARY_QUALITY";
            break;
        case(FOF_TRIPLE):
            ap->param_name[3] = "FOF_NUMBER_FOR_LO_PITCHES";
            ap->param_name[4] = "FOF_NUMBER_FOR_HI_PITCHES";
            ap->param_name[5] = "FOF_NUMBER_FOR_LOUDER_EVENTS";
            ap->param_name[6] = "MIN_FRQ_FROM_WHICH_TO_VARY_QUALITY";
            ap->param_name[7] = "MAX_FRQ_FOR_WHICH_TO_VARY_QUALITY";
            ap->param_name[8] = "MIN_OUTLEVEL_FROM_WHICH_TO_VARY_QUALITY";
            ap->param_name[9] = "MAX_OUTLEVEL_FOR_WHICH_TO_VARY_QUALITY";
            break;
        }
        break;
    case(FOFEX_EX):
        ap->param_name[0] = "PITCH_BREAKPOINT_FILE_(WITH_POSSIBLE_ZEROS)";
        switch(mode) {
        case(0):
        case(2):
            ap->param_name[1] = "REJECT_FOFS_THIS_DBs_LOWER_THAN_MAX_FOF";
            break;
        case(1):
            ap->param_name[1] = "TIME_IN_FILE";
            break;
        }
        ap->param_name[2] = "EXTRACT_FOFS_IN_GROUPS_OF";
        break;
    case(GREV_EXTEND):
        ap->param_name[GREV_WSIZE]      = "ENVELOPE_WINDOW_SIZE_(MS)";
        ap->param_name[GREV_TROFRAC]    = "DEPTH_OF_TROUGHS_AS_PROPORTION_OF_PEAK_HEIGHT";
        ap->param_name[2]               = "EXTEND_BY_HOW_MUCH";
        ap->param_name[3]               = "TIME_WHERE_GRAINS_START";
        ap->param_name[4]               = "TIME_WHERE_GRAINS_END";
        break;
    case(PEAKFIND):
        ap->param_name[0] = "ENVELOPE_WINDOWSIZE_(MS)";
        ap->param_name[1] = "THRESHOLD_LEVEL_FOR_PEAKS";
        break;
    case(CONSTRICT):
        ap->param_name[0] = "PERCENTAGE_DECIMATION";
        break;
    case(EXPDECAY):
        ap->param_name[0] = "DECAY_START_TIME";
        ap->param_name[1] = "DECAY_END_TIME";
        break;
    case(PEAKCHOP):
        ap->param_name[PKCH_WSIZE]  = "WINDOW_SIZE_(MS)";
        ap->param_name[PKCH_WIDTH]  = "PEAK_WIDTH_(MS)";
        ap->param_name[PKCH_SPLICE] = "RISE_TIME_(MS)";
        ap->param_name[PKCH_GATE]   = "GATE";
        ap->param_name[PKCH_SKEW]   = "PEAK_CENTRING";
        if(mode == 0 || mode == 2) {
            ap->param_name[PKCH_TEMPO] = "TEMPO"; 
            ap->param_name[PKCH_GAIN]  = "GAIN"; 
            ap->param_name[PKCH_SCAT]  = "TEMPO_SCATTER";
            ap->param_name[PKCH_NORM]  = "LEVELLING";
            ap->param_name[PKCH_REPET] = "REPETITION";
        }
        if(mode == 0)
            ap->param_name[PKCH_MISS]  = "SKIP_BY";
        break;
    case(MCHANPAN):
        switch(mode) {
        case(9):
            ap->param_name[3] = "EVENT_GROUP_SIZE";
            /* fall thro */
        case(1):
            ap->param_name[2] = "MINIMUM_DURATION_OF_SILENCES_(mS)";
            /* fall thro */
        case(0):
            ap->param_name[0] = "CHANNELS_IN_OUTPUT_FILE";
            ap->param_name[1] = "FOCUS";
            break;
        case(2):
            ap->param_name[5] = "MINIMUM_DURATION_OF_SILENCES_(mS)";
            /* fall thro */
        case(3):
            ap->param_name[0] = "CHANNELS_IN_OUTPUT_FILE";
            ap->param_name[1] = "CENTRE_OF_SPREAD";
            ap->param_name[2] = "CHANNEL_SPREAD";
            ap->param_name[3] = "DEPTH_OF_SPREAD_FRONT";
            ap->param_name[4] = "LEVEL_ROLLOFF_WITH_ADDED_CHANNELS";
            break;
        case(4):
            ap->param_name[0] = "CHANNELS_IN_OUTPUT_FILE";
            ap->param_name[1] = "MINIMUM_DURATION_OF_SILENCES_(mS)";
            break;
        case(5):
            ap->param_name[0] = "CHANNELS_IN_OUTPUT_FILE";
            ap->param_name[1] = "ANTIPHONY_TIMESTEP";
            ap->param_name[2] = "SILENCE_BETWEEN_ANTIPHONAL_EVENTS";
            ap->param_name[3] = "SPLICELENGTH_(mS)";
            break;
        case(6):
            ap->param_name[0] = "LEVEL_ROLLOFF_WITH_ADDED_CHANNELS";
            break;
        case(7):
            ap->param_name[0] = "CENTRE_OF_SPREAD";
            ap->param_name[1] = "CHANNEL_SPREAD";
            break;
        case(8):
            ap->param_name[0] = "CHANNELS_IN_OUTPUT_FILE";
            ap->param_name[1] = "START_CHANNEL";
            ap->param_name[2] = "SPEED_(CYCLES_PER_SEC)";
            ap->param_name[3] = "FOCUS";
            break;
        }
        break;
    case(TEX_MCHAN):
        ap->param_name[TEXTURE_DUR]     = "OUTPUT_DURATION";
        ap->param_name[TEXTURE_PACK]    = "EVENT_PACKING";
        ap->param_name[TEXTURE_SCAT]    = "EVENT_SCATTER";
        ap->param_name[TEXTURE_TGRID]   = "TIME_GRID_UNIT_(MS)";
        ap->param_name[TEXTURE_INSLO]   = "FIRST_SND-IN-LIST_TO_USE";
        ap->param_name[TEXTURE_INSHI]   = "LAST_SND-IN-LIST_TO_USE";
        ap->param_name[TEXTURE_MINAMP]  = "MIN_EVENT_GAIN_(MIDI)";
        ap->param_name[TEXTURE_MAXAMP]  = "MAX_EVENT_GAIN_(MIDI)";
        ap->param_name[TEXTURE_MINDUR]  = "MIN_EVENT_SUSTAIN";
        ap->param_name[TEXTURE_MAXDUR]  = "MAX_EVENT_SUSTAIN";
        ap->param_name[TEXTURE_MINPICH] = "MIN_PITCH_(MIDI)";
        ap->param_name[TEXTURE_MAXPICH] = "MAX_PITCH_(MIDI)";
        ap->param_name[TEXTURE_OUTCHANS]= "NUMBER_OF_OUTPUT_CHANNELS";
        ap->param_name[TEXTURE_ATTEN]   = "OVERALL_ATTENUATION";
        ap->param_name[TEXTURE_POS]     = "SPATIAL_POSITION";
        ap->param_name[TEXTURE_SPRD]    = "SPATIAL_SPREAD";
        ap->param_name[TEXTURE_SEED]    = "SEED";
        break;
    case(MANYSIL):
        ap->param_name[0] = "SPLICE_LENGTH_(mS)";
        break;
    case(RETIME):
        switch(mode) {
        case(0):
            ap->param_name[0] = "METRONOME_MARK_OR_BEAT_DURATION";
            break;
        case(1):
            ap->param_name[0] = "METRONOME_MARK_OR_BEAT_DURATION";
            ap->param_name[1] = "PEAKWIDTH_(mS)";
            ap->param_name[2] = "SPLICE_LENGTH_(mS)";
            break;
        case(2):
            ap->param_name[0] = "MINIMUM_INTER-EVENT_SILENCE_(mS)";
            ap->param_name[1] = "OUTPUT_PEAKWIDTH_(mS)";
            ap->param_name[2] = "OUTPUT_SPLICE_LENGTH_(mS)";
            ap->param_name[3] = "ORIGINAL_SPLICE_LENGTH_(mS)";
            break;
        case(3):
            ap->param_name[0] = "METRONOME_MARK_OR_BEAT_DURATION";
            ap->param_name[1] = "MINIMUM_INTER-EVENT_SILENCE_(mS)";
            ap->param_name[2] = "PREGAIN";
            break;
        case(4):
            ap->param_name[0] = "TEMPO_CHANGE_FACTOR";
            ap->param_name[1] = "MINIMUM_INTER-EVENT_SILENCE_(mS)";
            ap->param_name[2] = "TEMPO_CHANGE_STARTS_AFTER";
            ap->param_name[3] = "TEMPO_CHANGE_ENDS_BEFORE";
            ap->param_name[4] = "SYNCHRONISATION_TIME";
            break;
        case(5):
            ap->param_name[0] = "METRONOME_MARK_OR_BEAT_DURATION";
            ap->param_name[1] = "TIME_OF_FIRST_SOUNDING_EVENT_IN_OUTPUT";
            ap->param_name[2] = "MINIMUM_INTER-EVENT_SILENCE_(mS)";
            ap->param_name[3] = "PREGAIN";
            break;
        case(6):
            ap->param_name[0] = "TIME_OF_FIRST_SOUNDING_EVENT_IN_OUTPUT";
            ap->param_name[1] = "MINIMUM_INTER-EVENT_SILENCE_(mS)";
            ap->param_name[2] = "PREGAIN";
            break;
        case(7):
            ap->param_name[0] = "METRONOME_MARK_OR_BEAT_DURATION";
            ap->param_name[1] = "EVENT_LOCATION";
            ap->param_name[2] = "BEATS_IN_EVENT";
            ap->param_name[3] = "EVENT_REPETITIONS";
            ap->param_name[4] = "MINIMUM_INTER-EVENT_SILENCE_(mS)";
            break;
        case(8):
            ap->param_name[0] = "MINIMUM_INTER-EVENT_SILENCE_(mS)";
            break;
        case(9):
            ap->param_name[0] = "MINIMUM_INTER-EVENT_SILENCE_(mS)";
            ap->param_name[1] = "DEGREE_OF_LEVEL_EQUALISATION";
            ap->param_name[2] = "ACCENT_GROUPING";
            ap->param_name[3] = "PREGAIN";
            break;
        case(10):
            ap->param_name[0] = "MINIMUM_INTER-EVENT_SILENCE_(mS)";
            break;
        case(12):
            ap->param_name[0] = "NEW_PEAK_TIME";
            break;
        case(13):
            ap->param_name[0] = "NEW_PEAK_TIME";
            ap->param_name[1] = "ORIGINAL_PEAK_TIME";
            break;
        }
        break;
    case(HOVER):
        ap->param_name[0] = "RATE_OF_HOVER_(Hz)";
        ap->param_name[1] = "LOCATION_OF_HOVERING";
        ap->param_name[2] = "RANDOMISATION_OF_RATE";
        ap->param_name[3] = "RANDOMISATION_OF_LOCATION";
        ap->param_name[4] = "SPLICE_LENGTH_(mS)";
        ap->param_name[5] = "OUTPUT_DURATION";
        break;
    case(HOVER2):
        ap->param_name[0] = "RATE_OF_HOVER_(Hz)";
        ap->param_name[1] = "LOCATION_OF_HOVERING";
        ap->param_name[2] = "RANDOMISATION_OF_RATE";
        ap->param_name[3] = "RANDOMISATION_OF_LOCATION";
        ap->param_name[4] = "OUTPUT_DURATION";
        break;
    case(MULTIMIX):
        switch(mode) {
        case(0):
        case(1):
        case(5):
            break;
        case(2):
            ap->param_name[0] = "TIME_STEP_BETWEEN_ENTRIES";
            break;
        case(3):
            ap->param_name[0] = "RELATIVE_LEVEL_OF_OUTER_PAIR";
            break;
        case(4):
            ap->param_name[0] = "FRONT_PAIR_LEVEL";
            ap->param_name[1] = "FRONT_WIDE_PAIR_LEVEL";
            ap->param_name[2] = "REAR_WIDE_PAIR_LEVEL";
            ap->param_name[3] = "REAR_PAIR_LEVEL";
            break;
        case(6):
            ap->param_name[0] = "OUTPUT_CHANNEL_COUNT";
            ap->param_name[1] = "START_CHANNEL";
            ap->param_name[2] = "CHANNEL_SKIP";
            ap->param_name[3] = "TIMESTEP";
            break;
        case(7):
            ap->param_name[0] = "OUTPUT_CHANNEL_COUNT";
            break;
        }
        break;
    case(FRAME):
        switch(mode) {
        case(0):
            ap->param_name[0] = "ROTATION_SPEED_(CYCLES_PER_SEC)";
            ap->param_name[1] = "SMEAR";
            break;
        case(1):
            ap->param_name[0] = "1ST_ROTATION_SPEED_(CYCLES_PER_SEC)";
            ap->param_name[1] = "2ND_ROTATION_SPEED_(CYCLES_PER_SEC)";
            ap->param_name[2] = "SMEAR";
            break;
        case(2):
        case(4):
        case(7):
            break;
        case(3):
            ap->param_name[0] = "MIRROR_PLANE";
            break;
        case(5):
            ap->param_name[0] = "CHANNEL_TO_SWAP";
            ap->param_name[1] = "CHANNEL_TO_SWAP_WITH";
            break;
        case(6):
            ap->param_name[0] = "GAIN_OR_ENVELOPE";
            break;
        }
        break;
    case(SEARCH):
        break;
    case(MCHANREV):
        ap->param_name[STAD_PREGAIN] = "INPUT_GAIN";
        ap->param_name[STAD_ROLLOFF] = "LEVEL_LOSS_WITH_DISTANCE";
        ap->param_name[STAD_SIZE]    = "STADIUM_SIZE_MULTIPLIER";
        ap->param_name[STAD_ECHOCNT] = "NUMBER_OF_ECHOS";
        ap->param_name[REV_OCHANS]   = "NUMBER_OF_OUTPUT_CHANNELS";
        ap->param_name[REV_CENTRE]   = "CENTRE_OF_SOUND_IMAGE";
        ap->param_name[REV_SPREAD]   = "SPREAD_OF_ECHOS/REVERB";
        break;
    case(WRAPPAGE):
        ap->param_name[WRAP_OUTCHANS]   = "OUTPUT_CHANNEL_COUNT";
        ap->param_name[WRAP_SPREAD]     = "WIDTH_OF_SPATIAL_SPREAD";
        ap->param_name[WRAP_DEPTH]      = "DEPTH_OF_SPREAD_FRONT";
        ap->param_name[WRAP_VELOCITY]   = "TIMESHRINK";
        ap->param_name[WRAP_HVELOCITY]  = "TIMESHRINK_LIMIT";
        ap->param_name[WRAP_DENSITY]    = "DENSITY";
        ap->param_name[WRAP_HDENSITY]   = "DENSITY_LIMIT";
        ap->param_name[WRAP_GRAINSIZE]  = "GRAINSIZE_(MS)";
        ap->param_name[WRAP_HGRAINSIZE] = "GRAINSIZE_LIMIT_(MS)";
        ap->param_name[WRAP_PITCH]      = "PITCHSHIFT(SEMITONES)";
        ap->param_name[WRAP_HPITCH]     = "PITCHSHIFT_LIMIT_(SEMIT)";
        ap->param_name[WRAP_AMP]        = "GRAIN_LOUDNESS_RANGE";
        ap->param_name[WRAP_HAMP]       = "LOUDNESS_RANGE_LIMIT";
        ap->param_name[WRAP_BSPLICE]    = "STARTSPLICE_(MS)";
        ap->param_name[WRAP_HBSPLICE]   = "STARTSPLICE_LIMIT_(MS)";
        ap->param_name[WRAP_ESPLICE]    = "ENDSPLICE_(MS)";
        ap->param_name[WRAP_HESPLICE]   = "ENDSPLICE_LIMIT_(MS)";
        ap->param_name[WRAP_SRCHRANGE]  = "SEARCHRANGE_(MS)";
        ap->param_name[WRAP_SCATTER]    = "SCATTER";
        ap->param_name[WRAP_OUTLEN]     = "OUTPUT_LENGTH_(SECS)";
        ap->param_name[WRAP_BUFXX]      = "BUFFER_SIZE_MULTIPLIER";
        break;
    case(MCHSTEREO):
        ap->param_name[0] = "OUTPUT_CHANNEL_COUNT";
        ap->param_name[1] = "PREGAIN";
        break;
    case(MTON):
        ap->param_name[0] = "OUTPUT_CHANNEL_COUNT";
        break;
    case(FLUTTER):
        ap->param_name[0] = "FLUTTER_FREQUENCY_(Hz)";
        ap->param_name[1] = "FLUTTER_DEPTH";
        ap->param_name[2] = "GAIN";
        break;
    case(ABFPAN):
        ap->param_name[0] = "START_POSITION";
        ap->param_name[1] = "END_POSITION";
        ap->param_name[2] = "NUMBER_OF_B_FORMAT_OUTPUT_CHANNELS";
        break;
    case(ABFPAN2):
        ap->param_name[0] = "START_POSITION";
        ap->param_name[1] = "END_POSITION";
        ap->param_name[2] = "PREGAIN";
        break;
    case(ABFPAN2P):
        ap->param_name[0] = "START_POSITION";
        ap->param_name[1] = "END_POSITION";
        ap->param_name[2] = "PREGAIN";
        ap->param_name[3] = "HEIGHT";
        break;
    case(CHANNELX):
        break;
    case(CHORDER):
        break;
    case(FMDCODE):
        ap->param_name[0] = "OUTPUT_LAYOUT";
        break;
    case(CHXFORMAT):
        ap->param_name[0] = "LSPKR_POSITION_MASK";
        break;
    case(CHXFORMATG):
    case(CHXFORMATM):
        break;
    case(INTERLX):
        ap->param_name[0] = "OUTPUT_CHANNEL_FORMAT";
        break;
    case(COPYSFX):
        ap->param_name[0] = "SAMPLE_TYPE";
        ap->param_name[1] = "OUTFILE_FORMAT";
        break;
    case(NJOIN):
        ap->param_name[0] = "SILENCE_BETWEEN_FILES_(SECS)";
        break;
    case(NJOINCH):
        break;
    case(NMIX):
        ap->param_name[0] = "START_TIME_OF_2nd_FILE";
        break;
    case(RMSINFO):
        ap->param_name[0] = "START_TIME_OF_SCAN";
        ap->param_name[1] = "END_TIME_OF_SCAN";
        break;
    case(SFEXPROPS):
        break;
    case(SETHARES):
        ap->param_name[0] = "SEARCH_WINDOW_SIZE_(SEMITONES)";
        ap->param_name[1] = "PEAKING_RATIO";
        ap->param_name[2] = "AMPLITUDE_FLOOR";
        ap->param_name[3] = "LOW_PITCH_LIMIT";
        ap->param_name[4] = "HIGH_PITCH_LIMIT";
        ap->param_name[5] = "IN-TUNE_RANGE_(SEMITONES)";
        break;
    case(MCHSHRED):
        ap->param_name[0] = "NUMBER_OF_SHREDS";
        ap->param_name[1] = "AVERAGE_CHUNKLENGTH";
        ap->param_name[2] = "CUT_SCATTER";
        if(mode == 0)
            ap->param_name[3] = "OUTPUT_CHANNEL_CNT";
        break;
    case(MCHZIG):
        ap->param_name[MZIG_START]  = "ZIGZAGGING_START_TIME";
        ap->param_name[MZIG_END]    = "ZIGZAGGING_END_TIME";
        ap->param_name[MZIG_DUR]    = "MIN_DURATION_OUTFILE";
        ap->param_name[MZIG_MIN]    = "MIN_ZIG_LENGTH";
        ap->param_name[MZIG_OCHANS] = "OUTPUT_CHANNEL_CNT";
        ap->param_name[MZIG_SPLEN]  = "SPLICE_LENGTH_(MS)";
        if(mode==0) {
            ap->param_name[MZIG_MAX]   = "MAX_ZIG_LENGTH";
            ap->param_name[MZIG_RSEED] = "RANDOM_SEED";
        }
        break;
    case(MCHITER):
        ap->param_name[MITER_OCHANS]    = "OUTPUT_CHANNEL_CNT";
        switch(mode) {
        case(0):    ap->param_name[MITER_DUR]     = "OUTPUT_DURATION";      break;
        case(1):    ap->param_name[MITER_REPEATS] = "NUMBER_OF_REPEATS";    break;
        }
        ap->param_name[MITER_DELAY]  = "DELAY";
        ap->param_name[MITER_RANDOM] = "RANDOMISATION_OF_DELAY";
        ap->param_name[MITER_PSCAT]  = "PITCH_SCATTER";
        ap->param_name[MITER_ASCAT]  = "AMPLITUDE_SCATTER";
        ap->param_name[MITER_FADE]   = "PROGRESSIVE_FADE";
        ap->param_name[MITER_GAIN]   = "OVERALL_GAIN";
        ap->param_name[MITER_RSEED]  = "SEED_RANDOM_GENERATOR";
        break;
    case(SPECSPHINX):
        switch(mode) {
        case(0):
            ap->param_name[0] = "AMPLITUDE_BALANCE";
            ap->param_name[1] = "FREQUENCY_BALANCE";
            break;
        case(1):
            ap->param_name[0] = "BIAS";
            ap->param_name[1] = "GAIN";
            break;
        case(2):
            ap->param_name[0] = "CARVE_DEPTH";
            ap->param_name[1] = "OUTPUT_GAIN";
            ap->param_name[2] = "CUTOFF_FREQUENCY";
            break;
        }
        break;
    case(SPECMORPH):
        if(mode == 6) {
            ap->param_name[NMPH_APKS] = "NUMBER_OF_PEAKS_TO_MAP";
            ap->param_name[NMPH_OCNT] = "NUMBER_OF_INTERMEDIATE_FILES";
        } else {
            ap->param_name[NMPH_STAG] = "2nd_FILE_ENTRY_TIME";
            ap->param_name[NMPH_ASTT] = "INTERPOLATION_START";
            ap->param_name[NMPH_AEND] = "INTERPOLATION_END";
            ap->param_name[NMPH_AEXP] = "INTERPOLATION_EXPONENT";
            ap->param_name[NMPH_APKS] = "NUMBER_OF_PEAKS_TO_MAP";
            if(mode >= 4)
                ap->param_name[NMPH_RAND] = "RANDOMISATION_OF_GOAL_PEAK_FRQ";
        }
        break;
    case(SPECMORPH2):
        ap->param_name[NMPH_APKS] = "NUMBER_OF_PEAKS_TO_FIND";
        if(mode > 0) {
            ap->param_name[NMPH_ASTT] = "INTERPOLATION_START";
            ap->param_name[NMPH_AEND] = "INTERPOLATION_END";
            ap->param_name[NMPH_AEXP] = "INTERPOLATION_EXPONENT";
            ap->param_name[NMPH_RAND] = "RANDOMISATION_OF_GOAL_PEAK_FRQ";
        }
        break;

    case(SUPERACCU):
        ap->param_name[0] = "DECAY_RATE_(GAIN_FACTOR_PER_SECOND)";
        ap->param_name[1] = "GLISS_RATE_(8vas_PER_SECOND)";
        break;
    case(PARTITION):
        switch(mode) {
        case(0):
            ap->param_name[0] = "GRID_COUNT_(NO._OF_OUTPUT_FILES)";
            ap->param_name[1] = "WAVESET_COUNT_PER_GRID_BLOCK";
            break;
        case(1):
            ap->param_name[0] = "GRID_COUNT_(NO._OF_OUTPUT_FILES)";
            ap->param_name[1] = "DURATION_OF_GRID_BLOCKS";
            ap->param_name[2] = "RANDOMISATION_OF_DURATION";
            ap->param_name[3] = "SPLICE_LENGTH_(mS)";
            break;
        }
        break;
    case(SPECGRIDS):
        ap->param_name[0] = "NUMBER_OF_OUTPUT_SPECTRA";
        ap->param_name[1] = "NUMBER_OF_ADJACENT_CHANNELS_IN_EACH_BLOCK_IN_GRIDS";
        break;
    case(GLISTEN):
        ap->param_name[0] = "GROUP_DIVISIONS";
        ap->param_name[1] = "WINDOW_LENGTH";
        ap->param_name[2] = "PITCHSHIFT_(SEMITONES)";
        ap->param_name[3] = "WINDOWLENGTH_RANDOMISE";
        ap->param_name[4] = "GROUPDIVIDE_RANDOMISE";
        break;
    case(TUNEVARY):
        ap->param_name[0] = "FOCUS";
        ap->param_name[1] = "CLARITY";
        ap->param_name[2] = "TRACE_INDEX";
        ap->param_name[3] = "LOW_FRQ_LIMIT";
        break;
    case(ISOLATE):
        switch(mode) {
        case(ISO_OVRLAP):
            ap->param_name[ISO_SPL] = "SPLICE_LENGTH_(mS)";
            ap->param_name[ISO_DOV] = "SEGMENT_OVERLAP_(mS)";
            break;
        case(ISO_THRESH):
            ap->param_name[ISO_THRON]  = "THRESHOLD_ON_LEVEL_(dB)";
            ap->param_name[ISO_THROFF] = "THRESHOLD_OFF_LEVEL_(dB)";
            ap->param_name[ISO_SPL]    = "SPLICE_LENGTH_(mS)";
            ap->param_name[ISO_MIN]    = "MINIMUM_SEGMENT_LENGTH_(mS)";
            ap->param_name[ISO_LEN]    = "SEGMENT_RETAIN_LENGTH_(mS)";
            break;
        default:
            ap->param_name[ISO_SPL] = "SPLICE_LENGTH_(mS)";
            break;
        }
        break;
    case(REJOIN):
        ap->param_name[0]   = "GAIN";
        break;
    case(PANORAMA):
        if(mode == 0) {
            ap->param_name[PANO_LCNT]  = "NO._OF_LOUDSPEAKERS";
            ap->param_name[PANO_LWID]  = "TOTAL_ANGULAR_WIDTH_OF_LSPKRS";
        }
        ap->param_name[PANO_SPRD]  = "ANGULAR_WIDTH_OF_SOUND_IMAGE";
        ap->param_name[PANO_OFST]  = "ANGULAR_OFFSET_OF_SOUND_IMAGE";
        ap->param_name[PANO_CNFG]  = "SOUND_CONFIGURATION";
        ap->param_name[PANO_RAND]  = "RANDOMISATION";
        break;
    case(TREMOLO):
        ap->param_name[TREMOLO_FRQ]  = "TREMOLO_FREQUENCY";
        ap->param_name[TREMOLO_DEP]  = "TREMOLO_DEPTH";
        ap->param_name[TREMOLO_AMP]  = "OVERALL_GAIN";
        ap->param_name[TREMOLO_SQZ]  = "PEAK_NARROWING";
        break;
    case(ECHO):
        ap->param_name[ECHO_DELAY] = "DELAY";
        ap->param_name[ECHO_ATTEN] = "ATTENUATION";
        ap->param_name[ECHO_DUR]   = "MAXIMUM_DURATION";
        ap->param_name[ECHO_RAND]  = "RANDOMISATION";
        ap->param_name[ECHO_CUT]   = "CUTOFF_LEVEL";
        break;
    case(PACKET):
        ap->param_name[PAK_DUR] = "DURATION_(mS)";
        ap->param_name[PAK_SQZ] = "NARROWING";
        ap->param_name[PAK_CTR] = "CENTRING";
        break;
    case(SYNTHESIZER):
        ap->param_name[SYNTHSRAT] = "SAMPLE_RATE";
        ap->param_name[SYNTH_DUR] = "DURATION";
        ap->param_name[SYNTH_FRQ] = "FUNDAMENTAL_FREQUENCY";
        if(mode == 1) {
            ap->param_name[SYNTH_SQZ] = "PACKET_NARROWING";
            ap->param_name[SYNTH_CTR] = "PACKET_CENTRING";
        }
        else if(mode == 2) {
            ap->param_name[SYNTH_CHANS] = "CHANNEL_COUNT";
            ap->param_name[SYNTH_MAX]   = "MAX_OCTAVE_TRANSPOSITION";
            ap->param_name[SYNTH_RATE]  = "TIMESTEP";
            ap->param_name[SYNTH_RISE]  = "RISETIME";
            ap->param_name[SYNTH_FALL]  = "FALLTIME";
            ap->param_name[SYNTH_STDY]  = "STEADY_STATE";
            ap->param_name[SYNTH_SPLEN] = "SPLICETIME_(mS)";
            ap->param_name[SYNTH_NUM]   = "PARTIALS_IN_PLAY";
            ap->param_name[SYNTH_EFROM] = "EMERGENCE_CHANNEL";
            ap->param_name[SYNTH_ETIME] = "EMERGENCE_TIME";
            ap->param_name[SYNTH_CTO]   = "CONVERGENCE-CHANNEL";
            ap->param_name[SYNTH_CTIME] = "CONVERGENCE_TIME";
            ap->param_name[SYNTH_STYPE] = "SPECIAL_SPACE_TYPE";
            ap->param_name[SYNTH_RSPEED] = "ROTATION_SPEED";
        }
        else if(mode == 3) {
            ap->param_name[SYNTH_ATK]    = "SAMPLES_IN_SPIKE_ATTACK";
            ap->param_name[SYNTH_EATK]   = "SLOPE_OF_SPIKE_ATTACK";
            ap->param_name[SYNTH_DEC]    = "SAMPLES_IN_SPIKE_DECAY";
            ap->param_name[SYNTH_EDEC]   = "SLOPE_OF_SPIKE_DECAY";
            ap->param_name[SYNTH_ATOH]   = "RATIO_ONSEG_TO_ONOFF_GROUPLENGTH";
            ap->param_name[SYNTH_GTOW]   = "RATIO_ONOFFONOFF_GROUPLENGTH_TO_WAVECYCLE_LENGTH";
            ap->param_name[SYNTH_RAND]   = "RANDOMISATION_OF_SPIKES";
            ap->param_name[SYNTH_FLEVEL] = "FRACTALISATION_LEVEL_FOR_SPIKES_TO_FLIP_POS_TO_NEG";
        }
        break;
    case(NEWTEX):
        ap->param_name[NTEX_DUR]    = "DURATION";
        ap->param_name[NTEX_CHANS]  = "CHANNEL_COUNT";
        switch(mode) {
        case(0):
            ap->param_name[NTEX_MAX] = "ADDITIONAL_OCTAVE_TRANSPOSITIONS";
            break;
        case(1):
        case(2):
            ap->param_name[NTEX_MAX] = "MAX_SOURCE_DUPLICATION";
            break;
        }
        ap->param_name[NTEX_RATE]   = "TIMESTEP";
        ap->param_name[NTEX_STYPE]  = "SPECIAL_SPACE_TYPE";
        ap->param_name[NTEX_SPLEN]  = "SPLICETIME_(mS)";
        ap->param_name[NTEX_NUM]    = "STREAMS_IN_PLAY";
        switch(mode) {
        case(1):
            ap->param_name[NTEX_DEL]= "STREAM_DELAY";
            break;
        case(2):
            ap->param_name[NTEX_LOC]= "LOCUS";
            ap->param_name[NTEX_AMB]= "AMBITUS";
            ap->param_name[NTEX_GST]= "DRUNK_STEP";
            break;
        }
        ap->param_name[NTEX_EFROM]  = "EMERGENCE_CHANNEL";
        ap->param_name[NTEX_ETIME]  = "EMERGENCE_TIME";
        ap->param_name[NTEX_CTO]    = "CONVERGENCE-CHANNEL";
        ap->param_name[NTEX_CTIME]  = "CONVERGENCE_TIME";
        ap->param_name[NTEX_RSPEED] = "ROTATION_SPEED";
        break;
    case(CERACU):
        ap->param_name[CER_MINDUR] = "FASTEST_REPEAT_TIME";
        ap->param_name[CER_OCHANS] = "OUTPUT_CHANNEL_COUNT";
        ap->param_name[CER_CUTOFF] = "MINIMUM_OUTPUT_DURATION";
        ap->param_name[CER_DELAY]  = "ECHO_DELAY";
        ap->param_name[CER_DSTEP]  = "ECHO_SPATIAL_OFFSET";
        break;
    case(MADRID):
        ap->param_name[MAD_DUR]   = "OUTPUT_DURATION";
        ap->param_name[MAD_CHANS] = "OUTPUT_CHANNEL_COUNT";
        ap->param_name[MAD_STRMS] = "NUMBER_OF_STREAMS";
        ap->param_name[MAD_DELF]  = "DELETION_FACTOR";
        ap->param_name[MAD_STEP]  = "EVENT_TIME_STEP";
        ap->param_name[MAD_RAND]  = "EVENT_TIME_RANDOMISATION";
        ap->param_name[MAD_SEED]  = "DELETION_SEED_VALUE";
        break;
    case(SHIFTER):
        ap->param_name[SHF_CYCDUR] = "CYCLE_DURATION";
        ap->param_name[SHF_OUTDUR] = "MIN_OUTPUT_DURATION";
        ap->param_name[SHF_OCHANS] = "OUTPUT_CHANNEL_COUNT";
        ap->param_name[SHF_SUBDIV] = "MINIMUM_BEAT_SUBDIVISION";
        ap->param_name[SHF_LINGER] = "LINGER_CYCLES";
        ap->param_name[SHF_TRNSIT] = "TRANSITION_CYCLES";
        ap->param_name[SHF_LBOOST] = "FOCUS_LEVEL_BOOST";
        break;
    case(SUBTRACT):
        ap->param_name[0]  = "CHANNEL_TO_SUBTRACT";
        break;
    case(SPEKLINE):
        if(mode == 0) {
            ap->param_name[0]  = "ANALYSIS_CHANNELS";
            ap->param_name[3]  = "NO_OF_ADDED_HARMONICS";
            ap->param_name[4]  = "HARMONICS_ROLLOFF(dB)";
            ap->param_name[9]  = "OVERALL_GAIN";
        }
        ap->param_name[1]  = "SOUND_SAMPLING_RATE";
        ap->param_name[2]  = "OUTPUT_DURATION";
        ap->param_name[5]  = "FOOT_OF_INPUT_DATA";
        ap->param_name[6]  = "CEILING_OF_INPUT_DATA";
        ap->param_name[7]  = "FOOT_OF_SPECTRUM_IN_OUTPUT";
        ap->param_name[8]  = "CEILING_OF_SPECTRUM_IN_OUTPUT";
        ap->param_name[10] = "SPECTRAL_WARP";
        ap->param_name[11] = "AMPLITUDE_RANGE_FLATTENING";
        break;
    case(FRACTURE):
        ap->param_name[FRAC_CHANS]  = "OUTPUT_CHANNELS";
        ap->param_name[FRAC_STRMS]  = "NUMBER_OF_SPATIAL_STREAMS";
        ap->param_name[FRAC_PULSE]  = "PULSE_DURATION";
        ap->param_name[FRAC_DEPTH]  = "DEPTH_AND_STACK";
        ap->param_name[FRAC_STACK]  = "STACKING_INTERVAL";
        ap->param_name[FRAC_INRND]  = "READ_RANDOMISATION";
        ap->param_name[FRAC_OUTRND] = "PULSE_RANDOMISATION";
        ap->param_name[FRAC_SCAT]   = "STREAM_DISPERSAL";
        ap->param_name[FRAC_LEVRND] = "LEVEL_RANDOMISATION";
        ap->param_name[FRAC_ENVRND] = "ENVELOPE_RANDOMISATION";
        ap->param_name[FRAC_STKRND] = "STACK_RANDOMISATION";
        ap->param_name[FRAC_PCHRND] = "PITCH_RANDOMISATION_IN_CENTS";
        ap->param_name[FRAC_SEED]   = "RANDOM_SEED";
        ap->param_name[FRAC_MIN]    = "MINIMUM_FRAGMENT_DURATION";
        ap->param_name[FRAC_MAX]    = "MAXIMUM_FRAGMENT_DURATION";
        if(mode > 0) {
            ap->param_name[FRAC_CENTRE]   = "CENTRE_OF_IMAGE";
            ap->param_name[FRAC_FRONT]    = "FRONT_POSITION";
            ap->param_name[FRAC_MDEPTH]   = "DEPTH_BEHIND_FRONT";
            ap->param_name[FRAC_ROLLOFF]  = "LEVEL_ROLLOFF_WITH_ADDED_CHANNELS";
            ap->param_name[FRAC_ATTEN]    = "ATTENUATION_FACTOR";
            ap->param_name[FRAC_ZPOINT]   = "SUBTEND_ZERO_POINT";
            ap->param_name[FRAC_CONTRACT] = "CONTRACTION_FACTOR";
            ap->param_name[FRAC_FPOINT]   = "MAX_FILTER_POINT"; 
            ap->param_name[FRAC_FFACTOR]  = "FILTER_MIX_FACTOR";
            ap->param_name[FRAC_FFREQ]    = "FILTER_LOPASS_FREQUENCY";
            ap->param_name[FRAC_UP]       = "FADE_IN";
            ap->param_name[FRAC_DN]       = "FADE_OUT";
            ap->param_name[FRAC_GAIN]     = "OVERALL_GAIN";
        }   
        break;
    case(TAN_ONE):
        ap->param_name[TAN_DUR]       = "DURATION";
        ap->param_name[TAN_STEPS]     = "EVENT_COUNT";
        if(mode==0) {
            ap->param_name[TAN_MANG]  = "MAXIMUM_ANGLE";
            ap->param_name[TAN_SLOW]  = "DRAG";
        } else
            ap->param_name[TAN_SKEW]  = "SKEW";
        ap->param_name[TAN_DEC]       = "DECIMATION";
        ap->param_name[TAN_FOCUS]     = "FOCUS_POSITION";
        ap->param_name[TAN_JITTER]    = "JITTER";
        break;
    case(TAN_TWO):
        ap->param_name[TAN_DUR]       = "DURATION";
        ap->param_name[TAN_STEPS]     = "EVENT_COUNT";
        if(mode == 0) {
            ap->param_name[TAN_MANG]  = "MAXIMUM_ANGLE";
            ap->param_name[TAN_SLOW]  = "DRAG";
        } else
            ap->param_name[TAN_SKEW]  = "SKEW";
        ap->param_name[TAN_DEC]       = "DECIMATION";
        ap->param_name[TAN_FBAL]      = "BALANCE_ACCUMULATOR";
        ap->param_name[TAN_FOCUS]     = "FOCUS_POSITION";
        ap->param_name[TAN_JITTER]    = "JITTER";
        break;
    case(TAN_SEQ):
    case(TAN_LIST):
        ap->param_name[TAN_DUR]       = "DURATION";
        if(mode==0) {
            ap->param_name[TAN_MANG]  = "MAXIMUM_ANGLE";
            ap->param_name[TAN_SLOW]  = "DRAG";
        } else
            ap->param_name[TAN_SKEW]  = "SKEW";
        ap->param_name[TAN_DEC]       = "DECIMATION";
        ap->param_name[TAN_FOCUS]     = "FOCUS_POSITION";
        ap->param_name[TAN_JITTER]    = "JITTER";
        break;
    case(SPECTWIN):
        ap->param_name[0] = "FREQUENCY_INTERPOLATION";
        ap->param_name[1] = "ENVELOPE_INTERPOLATION";
        ap->param_name[2] = "SPECTRAL_DUPLICATIONS";
        ap->param_name[3] = "DUPLICATION_INTERVAL";
        ap->param_name[4] = "DUPLICATION_ROLLOFF";
        break;
    case(TRANSIT):
    case(TRANSITF):
    case(TRANSITD):
    case(TRANSITFD):
    case(TRANSITS):
    case(TRANSITL):
        ap->param_name[TRAN_FOCUS]      = "FOCUS";
        ap->param_name[TRAN_DUR]        = "DURATION";
        if(process < TRANSITS)
            ap->param_name[TRAN_STEPS]  = "REPETITIONS";
        if(mode == CENTRAL)
            ap->param_name[TRAN_MAXA]   = "MAXIMUM_DISTANCE";
        else
            ap->param_name[TRAN_MAXA]   = "MAXIMUM_ANGLE";
        ap->param_name[TRAN_DEC]        = "DECIMATION";
        if(process == TRANSITF || process == TRANSITFD)
            ap->param_name[TRAN_FBAL]   = "BALANCE_DECIMATION";
        if(process < TRANSITS) {
            ap->param_name[TRAN_THRESH] = "THRESHOLD_FOR_EXTENSION";
            ap->param_name[TRAN_DECLIM] = "DECIMATION_MAXIMUM";
            ap->param_name[TRAN_MINLEV] = "FINAL_GAIN";
            ap->param_name[TRAN_MAXDUR] = "MAXIMUM_DURATION";
        }
        break;
    case(CANTOR):
        switch(mode) {
        case(0):
        case(1):
            ap->param_name[CA_HOLEN]    = "HOLE_SIZE";
            ap->param_name[CA_HOLEDIG]  = "DIG_DEPTH";
            ap->param_name[CA_TRIGLEV]  = "TRIGGER_DEPTH";
            ap->param_name[CA_SPLEN]    = "SPLICE_LENGTH";
            break;
        case(2):
            ap->param_name[CA_HOLEN]    = "MNIMUM_HOLE_LEVEL";
            ap->param_name[CA_HOLEDIG]  = "DIG_DEPTH";
            ap->param_name[CA_WOBBLES]  = "LAYER_COUNT";
            ap->param_name[CA_WOBDEC]   = "LAYER_DECIMATION";
            break;
        }
        ap->param_name[CA_MAXDUR]   = "MAXIMUM_DURATION";
        break;
    case(SHRINK):
        if(mode == SHRM_TIMED)
            ap->param_name[SHR_TIME]  = "CENTRE_OF_SHRINKAGE";
        ap->param_name[SHR_INK]       = "SOUND_SHRINKAGE";
        if(mode >= SHRM_FINDMX) {
            ap->param_name[SHR_WSIZE] = "WINDOW_SIZE_(mS)";
            ap->param_name[SHR_AFTER] = "SHRINKAGE_START";
        } else {
            ap->param_name[SHR_GAP]   = "EVENT_GAP";
            ap->param_name[SHR_DUR]   = "DURATION";
        }
        ap->param_name[SHR_CNTRCT]    = "TIME_CONTRACTION";
        ap->param_name[SHR_SPLEN]     = "SPLICE_LENGTH";
        ap->param_name[SHR_SMALL]     = "MINIMUM_EVENT_DURATION";
        ap->param_name[SHR_MIN]       = "MINIMUM_EVENT_SEPARATION";
        ap->param_name[SHR_RAND]      = "EVENT_TIME_RANDOMISATION";
        if(mode >= SHRM_FINDMX) {
            ap->param_name[SHR_GATE]  = "INPUT_GATE";
            ap->param_name[SHR_LEN]   = "MINIMUM_LENGTH_FOR_SQUEEZE_START";
        }
        if(mode == SHRM_FINDMX)
            ap->param_name[SHR_SKEW]  = "SKEW";
        break;
    case(NEWDELAY):
        if(mode == 0) {
            ap->param_name[DELAY_DELAY]         = "MIDI_PITCH";
            ap->param_name[DELAY_MIX]           = "DELAYED_SIGNAL_IN_MIX";
            ap->param_name[DELAY_FEEDBACK]      = "FEEDBACK";
        } else {
            ap->param_name[0]   = "MIDI_PITCH";
            ap->param_name[1]   = "HEAD_DURATION";
            ap->param_name[2]   = "FACTOR_BY_WHICH_HEAD_DURATION_EXTENDED";
            ap->param_name[3]   = "DELAY_RANDOMISATION";
            ap->param_name[4]   = "MAX_OF_LEVEL_DIP_IN_EXTENDED_HEAD_(A_FACTOR)";
            ap->param_name[5]   = "POSITION_OF_MAX_DIP_AS_FRACTION_OF_HEAD_LENGTH";
        }
        break;
    case(ITERLINE):
    case(ITERLINEF):
        ap->param_name[ITER_DUR]    = "OUTPUT_DURATION";
        ap->param_name[ITER_DELAY]  = "DELAY";
        ap->param_name[ITER_RANDOM] = "RANDOMISATION_OF_DELAY";
        ap->param_name[ITER_PSCAT]  = "PITCH_SCATTER";
        ap->param_name[ITER_ASCAT]  = "AMPLITUDE_SCATTER";
        ap->param_name[ITER_GAIN]   = "OVERALL_GAIN";
        ap->param_name[ITER_RSEED]  = "SEED_RANDOM_GENERATOR";
        break;
    case(FILTRAGE):
        ap->param_name[FILTR_DUR]   =   "DURATION";
        ap->param_name[FILTR_CNT]   =   "NUMBER_OF_FILTERS";
        ap->param_name[FILTR_MMIN]  =   "MIN_MIDIPITCH_OF_FILTERS";
        ap->param_name[FILTR_MMAX]  =   "MAX_MIDIPITCH_OF_FILTERS";
        ap->param_name[FILTR_DIS]   =   "PITCH_DISTRIBUTION";
        ap->param_name[FILTR_RND]   =   "PITCH_RANDOMISATION";
        ap->param_name[FILTR_AMIN]  =   "MINIMUM_FILTER_AMPLITUDE";
        ap->param_name[FILTR_ARND ] =   "AMPLITUDE_RANDOMISATION";
        ap->param_name[FILTR_ADIS]  =   "AMPLITUDE_DISTRIBUTION";
        if(mode == 1) {
            ap->param_name[FILTR_STEP]  =   "TIMESTEP_BETWEEN_FILTER_SETS";
            ap->param_name[FILTR_SRND]  =   "RANDOMISATION_OF_TIMESTEP";
        }
        ap->param_name[FILTR_SEED]  =   "RANDOM_SEED";
        break;
    case(SELFSIM):
        ap->param_name[0]   =   "SELF_SIMILARITY_INDEX";
        break;
    case(ITERFOF):
        if(mode < 2)
            ap->param_name[ITF_DEL] =   "SEMITONE_TRANSPOSITION_OF_LINE";
        else
            ap->param_name[ITF_DEL] =   "MIDI_PITCH_OF_LINE";
        ap->param_name[ITF_DUR]     =   "DURATION";
        ap->param_name[ITF_PRND]    =   "SEGMENT_PITCH_RANDOMISATION";
        ap->param_name[ITF_AMPC]    =   "MAX_OF_RANDOMISED_AMPLITUDE_REDUCTION";
        ap->param_name[ITF_TRIM]    =   "TRIMMED_DURATION_OF_ELEMENTS";
        ap->param_name[ITF_TRBY]    =   "FADE_DURATION_OF_ELEMENTS";
        ap->param_name[ITF_SLOP]    =   "FADE_SLOPE";
        ap->param_name[ITF_RAND]    =   "PITCH_ROUGHNESS";
        ap->param_name[ITF_VMIN]    =   "MIN_VIBRATO_FREQUENCY";
        ap->param_name[ITF_VMAX]    =   "MAX_VIBRATO_FREQUENCY";
        ap->param_name[ITF_DMIN]    =   "MIN_VIBRATO_DEPTH";
        ap->param_name[ITF_DMAX]    =   "MAX_VIBRATO_DEPTH";
        if(EVEN(mode))
            ap->param_name[ITF_SEED1]   =   "RANDOM_SEED";
        else {
            ap->param_name[ITF_GMIN]    =   "MIN_LEVEL_NOTES";
            ap->param_name[ITF_GMAX]    =   "MAX_LEVEL_NOTES";
            ap->param_name[ITF_UFAD]    =   "NOTE_INFADE_DURATION";
            ap->param_name[ITF_FADE]    =   "NOTE_OUTFADE_DURATION";
            ap->param_name[ITF_GAPP]    =   "GAP_BETWEEN_NOTES";
            ap->param_name[ITF_PORT]    =   "PORTAMENTO_TYPE";
            ap->param_name[ITF_PINT]    =   "PORTAMENTO_INTERVAL";
            ap->param_name[ITF_SEED2]   =   "RANDOM_SEED";
        }
        break;
    case(PULSER):
    case(PULSER2):
    case(PULSER3):
        ap->param_name[PLS_DUR]      = "DURATION";
        if(process == PULSER3 || mode == 0)
            ap->param_name[PLS_PITCH] = "MIDI_PITCH";
        ap->param_name[PLS_MINRISE]  = "RISE-TIME_MINIMUM";
        ap->param_name[PLS_MAXRISE]  = "RISE-TIME_MAXIMUM";
        ap->param_name[PLS_MINSUS]   = "SUSTAIN-TIME_MINIMUM";
        ap->param_name[PLS_MAXSUS]   = "SUSTAIN-TIME_MAXIMUM";
        ap->param_name[PLS_MINDECAY] = "DECAY-TIME_MINIMUM";
        ap->param_name[PLS_MAXDECAY] = "DECAY-TIME_MAXIMUM";
        ap->param_name[PLS_SPEED]    = "TIME_STEP_BETWEEN_PACKETS";
        ap->param_name[PLS_SCAT]     = "PACKET_TIME_RANDOMISATION";
        ap->param_name[PLS_EXP]      = "SLOPE_OF_ATTACK";
        ap->param_name[PLS_EXP2]     = "SLOPE_OF_DECAY";
        ap->param_name[PLS_PSCAT]    = "PITCH_SCATTER_(SEMITONES)";
        ap->param_name[PLS_ASCAT]    = "AMPLITUDE_SCATTER";
        ap->param_name[PLS_OCT]      = "OCTAVIATION";
        ap->param_name[PLS_BEND]     = "PACKET_PITCH_BEND";
        ap->param_name[PLS_SEED]     = "RANDOM_SEED";
        if(process == PULSER3) {
            ap->param_name[PLS_SRATE] = "SAMPLING_RATE";
            ap->param_name[PLS_CNT]   = "PARTIAL_COUNT";
        } else if(mode == 2)
            ap->param_name[PLS_WIDTH] = "SPATIAL_WIDTH";
        break;
    case(CHIRIKOV):
        ap->param_name[CHIR_DUR]       = "DURATION";
        ap->param_name[CHIR_FRQ]       = "FREQUENCY";
        ap->param_name[CHIR_DAMP]      = "DAMPING";
        if(mode < 2) {
            ap->param_name[CHIR_SRATE] = "SAMPLE_RATE";
            ap->param_name[CHIR_SPLEN] = "SPLICE_LENGTH_(mS)";
        } else {
            ap->param_name[CHIR_PMIN]  = "MINIMUM_MIDI-PITCH";
            ap->param_name[CHIR_PMAX]  = "MAXIMUM_MIDI-PITCH";
            ap->param_name[CHIR_STEP]  = "TIME_STEP";
            ap->param_name[CHIR_RAND]  = "TIME_RANDOMISATION";
        }
        break;
    case(MULTIOSC):
        ap->param_name[MOSC_DUR]  = "DURATION";
        ap->param_name[MOSC_FRQ1] = "FREQUENCY";
        ap->param_name[MOSC_FRQ2] = "FREQUENCY_TWO";
        ap->param_name[MOSC_AMP2] = "AMPLITUDE_TWO";
        if(mode >= 1) {
            ap->param_name[MOSC_FRQ3] = "FREQUENCY_THREE";
            ap->param_name[MOSC_AMP3] = "AMPLITUDE_THREE";
        }
        if(mode == 2) {
            ap->param_name[MOSC_FRQ4] = "FREQUENCY_FOUR";
            ap->param_name[MOSC_AMP4] = "AMPLITUDE_FOUR";
        }
        ap->param_name[MOSC_SRATE]  = "SAMPLE_RATE";
        ap->param_name[MOSC_SPLEN]  = "SPLICE_LENGTH_(mS)";
        break;
    case(SYNFILT):
        ap->param_name[SYNFLT_SRATE]   = "SAMPLE_RATE";
        ap->param_name[SYNFLT_CHANS]   = "CHANNEL_COUNT";
        ap->param_name[SYNFLT_Q]       = "FILTER_Q";
        ap->param_name[SYNFLT_HARMCNT] = "NUMBER_OF_HARMONICS";
        ap->param_name[SYNFLT_ROLLOFF] = "ROLL_OFF";
        ap->param_name[SYNFLT_SEED]    = "RANDOM_SEED";
        break;
    case(SPECRAND):
        ap->param_name[0]   =   "RANDOMISATION_TIMESCALE";
        ap->param_name[1]   =   "WINDOW_GROUPING";
        break;
    case(SPECSQZ):
        ap->param_name[0]   =   "CENTRE_FREQUENCY";
        ap->param_name[1]   =   "SQUEEZE_FACTOR";
        break;
    case(STRANDS):
        ap->param_name[STRAND_DUR]   = "DURATION";
        ap->param_name[STRAND_BANDS] = "NUMBER_OF_BANDS";
        if(mode != 2)
            ap->param_name[STRAND_THRDS] = "NUMBER_OF_THREADS_PER_BAND";
        ap->param_name[STRAND_TSTEP] = "TIMESTEP_BETWEEN_OUTPUT_VALUES_(mS)";
        ap->param_name[STRAND_BOT]   = "BOTTOM_OF_PITCH_RANGE";
        ap->param_name[STRAND_TOP]   = "TOP_OF_PITCH_RANGE";
        ap->param_name[STRAND_TWIST] = "FREQUENCY_OF_BAND_ROTATION";
        ap->param_name[STRAND_RAND]  = "RANDOM_DIVERSITY_OF_BAND_FREQUENCIES";
        ap->param_name[STRAND_SCAT]  = "RANDOM_WARPING_OF_THREAD_OSCILLATIONS";
        ap->param_name[STRAND_VAMP]  = "BAND_BOUNDARY_WAVINESS";
        ap->param_name[STRAND_VMIN]  = "WAVINESS_MIN_FREQUENCY";
        ap->param_name[STRAND_VMAX]  = "WAVINESS_MAX_FREQUENCY";
        ap->param_name[STRAND_TURB]  = "TURBULENCE";
        ap->param_name[STRAND_SEED]  = "RANDOM_SEED";
        ap->param_name[STRAND_GAP]   = "MINIMUM_PITCH_INTERVAL_BETWEEN_BANDS";
        ap->param_name[STRAND_MINB]  = "MINIMUM_PITCH_WIDTH_OF_BANDS";
        ap->param_name[STRAND_3D]    = "ROTATION_IN_3D";
        break;
    case(REFOCUS):
        ap->param_name[REFOC_DUR]    = "DURATION";
        ap->param_name[REFOC_BANDS]  = "NUMBER_OF_BANDS";
        ap->param_name[REFOC_RATIO]  = "FOCUSING_RATIO";
        ap->param_name[REFOC_TSTEP]  = "TIMESTEP_TO_NEXT_REFOCUS";
        ap->param_name[REFOC_RAND]   = "TIMESTEP_RANDOMISATION";
        ap->param_name[REFOC_OFFSET] = "OFFSET_BEFORE_REFOCUSING_BEGINS";
        ap->param_name[REFOC_END]    = "TIME_AT_WHICH_REFOCUSING_ENDS";
        ap->param_name[REFOC_XCPT]   = "NO_FOCUS_ON_EXTREMAL_BAND";
        ap->param_name[REFOC_SEED]   = "RANDOM_SEED";
        break;
    case(CHANPHASE):
        ap->param_name[0]    = "CHANNEL_TO_INVERT";
        break;
    case(SILEND):
        if(mode == 0)
            ap->param_name[0]    = "DURATION_OF_PADDING_SILENCE";
        else
            ap->param_name[0]    = "DURATION_OF_OUTPUT_SOUND";
        break;
    case(SPECULATE):
        ap->param_name[0]    = "MINIMUM_FREQUENCY";
        ap->param_name[1]    = "MAXIMUM_FREQUENCY";
        break;
    case(SPECTUNE):
        ap->param_name[0] = "VALID_HARMONICS_COUNT";
        ap->param_name[1] = "MINIMUM_MIDI_PITCH";
        ap->param_name[2] = "MAXIMUM_MIDI_PITCH";
        ap->param_name[3] = "START_TIME_FOR_PITCH_SEARCH";
        ap->param_name[4] = "END_TIME_FOR_PITCH_SEARCH";
        ap->param_name[5] = "IN-TUNE_RANGE_(SEMIT)";
        ap->param_name[6] = "MIN_WINDOWS_TO_CONFIRM_PITCH";
        ap->param_name[7] = "SIGNAL_TO_NOISE_RATIO_(dB)";
        break;
    case(REPAIR):
        ap->param_name[0]    = "OUTPUT_CHANNEL_COUNT";
        break;
    case(DISTSHIFT):
        ap->param_name[0]    = "WAVESET_GROUP_SIZE";
        if(mode==0)
            ap->param_name[1]    = "WAVESET_SHIFT";
        break;
    case(QUIRK):
        ap->param_name[0]    = "POWER_FACTOR";
        break;
    case(ROTOR):
        ap->param_name[ROT_CNT]     = "COUNT_OF_NOTES_PER_SET";
        ap->param_name[ROT_PMIN]    = "MIN_MIDI_PITCH"; 
        ap->param_name[ROT_PMAX]    = "MAX_MIDI_PITCH";
        ap->param_name[ROT_NSTEP]   = "MAX_DURATION_SLOWEST_BEAT";
        ap->param_name[ROT_PCYC]    = "NUMBER_OF_SETS_PER_PITCH_CYCLE";
        ap->param_name[ROT_TCYC]    = "NUMBER_OF_SETS_PER_SPEED_CYCLE"; 
        ap->param_name[ROT_PHAS]    = "INITIAL_PHASE_DIFFERENCE_BETWEEN_CYCLES";
        ap->param_name[ROT_DUR]     = "(MININIMUM)_OUTPUT_DURATION";
        if(mode==0)
            ap->param_name[ROT_GSTEP] = "TIME_STEP_BETWEEN_WHOLE_SETS";
        ap->param_name[ROT_DOVE]    = "DOVETAIL_DURATION_(mS)";
        break;
    case(DISTCUT):
        ap->param_name[DCUT_CNT] = "CYCLE_COUNT";
        if(mode==1)
            ap->param_name[DCUT_STP] = "CYCLE_STEP";
        ap->param_name[DCUT_EXP] = "DECAY_EXPONENT";
        ap->param_name[DCUT_LIM] = "CUTOFF_(dB)";
        break;
    case(ENVCUT):
        ap->param_name[ECUT_CNT] = "ENVELOPE_DURATION";
        if(mode==1)
            ap->param_name[ECUT_STP] = "TIMESTEP_TO_NEXT_SEGMENT";
        ap->param_name[ECUT_ATT] = "ATTACK_DURATION_(mS)";
        ap->param_name[ECUT_EXP] = "DECAY_EXPONENT";
        ap->param_name[ECUT_LIM] = "CUTOFF_(dB)";
        break;
    case(SPECFOLD):
        ap->param_name[0] = "CHANNEL_WHERE_PROCESSING_STARTS";
        ap->param_name[1] = "CHANNELS_TO_PROCESS";
        switch(mode) {
        case(0): 
            ap->param_name[2] = "NUMBER_OF_FOLDINGS";
            break;
        case(2): 
            ap->param_name[2] = "RANDOM_SEED";
            break;
        }
        break;
    case(BROWNIAN):
        ap->param_name[BRCHANS] = "OUTPUT_CHANNEL_COUNT";
        ap->param_name[BRDUR]   = "(MAX)_OUTPUT_DURATION";
        if(mode == 0) {
            ap->param_name[BRATT] = "EVENT_ATTACK_DURATION";
            ap->param_name[BRDEC] = "EVENT_DECAY_DURATION";
        }
        ap->param_name[BRPLO]   = "BOTTOM_OF_PITCHRANGE_(MIDI)";
        ap->param_name[BRPHI]   = "TOP_OF_PITCHRANGE_(MIDI)";
        ap->param_name[BRPSTT]  = "STARTING_PITCH_(MIDI)";
        ap->param_name[BRSSTT]  = "START_POSITION";
        ap->param_name[BRPSTEP] = "MAX_PITCH_STEP_(SEMITONES)";
        ap->param_name[BRSSTEP] = "MAX_SPACE_STEP";
        ap->param_name[BRTICK]  = "AVERAGE_TIMESTEP_BETWEEN_EVENTS";
        ap->param_name[BRSEED]  = "RANDOM_SEED";
        ap->param_name[BRASTEP] = "MAX_AMPLITUDE_STEP(dB)";
        ap->param_name[BRAMIN]  = "MINIMUM_AMPLITUDE(dB)";
        if(mode == 0) {
            ap->param_name[BRASLP]  = "ATTACK_SLOPE";
            ap->param_name[BRDSLP]  = "DECAY_SLOPE";
        }
        break;
    case(SPIN):
        ap->param_name[SPNRATE]  = "ROTATION_RATE(CYCLES_PER_SEC)";
        ap->param_name[SPNBOOST] = "DIFFERENTIAL_BETWEEN_FRONT_AND_REAR";
        ap->param_name[SPNATTEN] = "ATTENUATION_AT_CENTRE";
        ap->param_name[SPNDOPL]  = "MAX_DOPPLER_PITCHSHIFT(SEMITONES)";
        ap->param_name[SPNXBUF]  = "EXPAND_BUFFERS_BY";
        if(mode > 0) {
            ap->param_name[SPNOCHNS]  = "OUTFILE_CHANNEL_COUNT";
            ap->param_name[SPNOCNTR]  = "CENTRE_CHANNEL_OF_IMAGE";
            ap->param_name[SPNCMIN]   = "MIN_LEVEL_ON_CENTRE_CHAN";
            if(mode == 1)
                ap->param_name[SPNCMAX]   = "MAX_BOOST_ON_CENTRE_CHAN";
        }
        break;
    case(SPINQ):
        ap->param_name[SPNRATE]  = "ROTATION_RATE(CYCLES_PER_SEC)";
        ap->param_name[SPNBOOST] = "DIFFERENTIAL_BETWEEN_FRONT_AND_REAR";
        ap->param_name[SPNATTEN] = "ATTENUATION_AT_CENTRE";
        ap->param_name[SPNOCHNS] = "OUTFILE_CHANNEL_COUNT";
        ap->param_name[SPNOCNTR] = "CENTRE_CHANNEL_OF_IMAGE";
        ap->param_name[SPNDOPL]  = "MAX_DOPPLER_PITCHSHIFT(SEMITONES)";
        ap->param_name[SPNXBUF]  = "EXPAND_BUFFERS_BY";
        ap->param_name[SPNCMIN]  = "MIN_LEVEL_ON_CENTRE_CHAN";
        if(mode == 0)
            ap->param_name[SPNCMAX]  = "MAX_BOOST_ON_CENTRE_CHAN";
        break;
    case(CRUMBLE):
        ap->param_name[CRSTART]  = "START_TIME";
        ap->param_name[CRSTEP1]  = "DURATION_OF_HALF_SPLITS";
        ap->param_name[CRSTEP2]  = "DURATION_OF_QUARTER_SPLITS";
        if(mode==1)
            ap->param_name[CRSTEP3]  = "DURATION_OF_EIGHTH_SPLITS";
        ap->param_name[CRORIENT] = "ORIENTATION";
        ap->param_name[CRSIZE]   = "SEGMENT_SIZE";
        ap->param_name[CRRAND]   = "SIZE_RANDOMISATION";
        ap->param_name[CRISCAT]  = "INPUT_SCATTER";
        ap->param_name[CROSCAT]  = "OUTPUT_SCATTER";
        ap->param_name[CROSTR]   = "OUTPUT_TIMESTRETCH";
        ap->param_name[CRPSCAT]  = "PITCH_SCATTER";
        ap->param_name[CRSEED]   = "SEED";
        ap->param_name[CRSPLICE] = "SPLICELENGTH(mS)";
        ap->param_name[CRTAIL]   = "LENGTH_OF_EXPONENTIAL_TAIL(mS)";
        ap->param_name[CRDUR]    = "MAXIMUM_DURATION";
        break;
    case(PHASOR):
        ap->param_name[PHASOR_STREAMS] = "NUMBER_OF_PHASING_STREAMS";
        ap->param_name[PHASOR_FRQ]     = "FREQUENCY_OF_PHASING_WAVE";
        ap->param_name[PHASOR_SHIFT]   = "MAX_PHASING_SHIFT_(SEMITONES)";
        ap->param_name[PHASOR_OCHANS]  = "NUMBER_OF_OUTPUT_CHANNELS";
        ap->param_name[PHASOR_OFFSET]  = "TIME_OFFSET_OF_STREAMS_(mS)";
        break;
    case(TESSELATE):
        ap->param_name[TESS_CHANS] = "OUTPUT_CHANNEL_COUNT";
        ap->param_name[TESS_PHRAS] = "REPEAT_CYCLE_DURATION";
        ap->param_name[TESS_DUR]   = "OUTPUT_DURATION";
        ap->param_name[TESS_TYP]   = "TESSELATION_TYPE";
        break;
    case(CRYSTAL):
        ap->param_name[CRY_ROTA]    = "ROTATION_SPEED_AROUND_Z_AXIS_(CYCS_PER_SEC)";
        ap->param_name[CRY_ROTB]    = "ROTATION_SPEED_AROUND_Y_AXIS_(CYCS_PER_SEC)";
        ap->param_name[CRY_TWIDTH]  = "MAXIMUM_TIMEWIDTH_OF_ONE_ALL-VERTICES-PLAY";
        ap->param_name[CRY_TSTEP]   = "TIMESTEP_BETWEEN_ONE_ALL-VERTICES-PLAY_AND_NEXT";
        ap->param_name[CRY_DUR]     = "TOTAL_DURATION_OF_OUTPUT";
        ap->param_name[CRY_PLO]     = "MIN_MIDI_PITCH_OF_ANY_VERTEX_(WHEREVER_ROTATED)";
        ap->param_name[CRY_PHI]     = "MAX_MIDI_PITCH_OF_ANY_VERTEX_(WHEREVER_ROTATED)";
        ap->param_name[CRY_FPASS]   = "FRQ_OF_PASSBAND_OF_FILTER_FOR_DISTANCE_CUES";
        ap->param_name[CRY_FSTOP]   = "FRQ_OF_STOPBAND_OF_FILTER_FOR_DISTANCE_CUES";
        ap->param_name[CRY_FATT]    = "MAX_ATTENUATION(DB)_OF_FILTER";
        ap->param_name[CRY_FPRESC]  = "GAIN_APPLIED_TO_SRC_BEFORE_FILTERING";
        ap->param_name[CRY_FSLOPE]  = "CURVE_SLOPE_FOR_MIXING_FILT_TO_UNFILT_SND(DEPTH_CUE)";
        ap->param_name[CRY_SSLOPE]  = "CURVE_SLOPE_FOR_MIXING_IN_OCTAVE-UP-SND(PROXIMITY_CUE)";
        break;
    case(WAVEFORM):
        ap->param_name[WF_TIME] = "TIME_OF_SAMPLING";
        if(mode == 0)
            ap->param_name[WF_CNT]  = "COUNT_OF_HALF_WAVESETS";
        else
            ap->param_name[WF_DUR]  = "DURATION_TO_SAMPLE_(mS)";
        if(mode == 2)
            ap->param_name[WF_BAL]  = "BALANCE_WITH_SINUSOID";
        break;
    case(DVDWIND):
        ap->param_name[0]   = "TIME_CONTRACTION";
        ap->param_name[1]   = "SIZE_OF_CLIPS(mS)";
        break;
    case(CASCADE):
        if(mode < 5) {
            ap->param_name[CAS_CLIP]    = "SEGMENT_LENGTH";
            ap->param_name[CAS_MAXCLIP] = "MAX_SEGMENT_LENGTH";
        }
        ap->param_name[CAS_ECHO]    = "ECHO_COUNT";
        ap->param_name[CAS_MAXECHO] = "MAX_ECHO_COUNT";
        ap->param_name[CAS_RAND]    = "TIME_RANDOMISATION_OF_ECHOS";
        ap->param_name[CAS_SEED]    = "RANDOM_SEED";
        ap->param_name[CAS_SHREDNO] = "CHUNKS_IN_ANY_SEGMENT-SHRED";
        ap->param_name[CAS_SHREDCNT]= "REPEAT_SHREDDINGS";
        break;
    case(SYNSPLINE):
        ap->param_name[SPLIN_SRATE] = "SAMPLE_RATE";
        ap->param_name[SPLIN_DUR]   = "DURATION";
        ap->param_name[SPLIN_FRQ]   = "FREQUENCY";
        ap->param_name[SPLIN_CNT]   = "SPLINE_COUNT";
        ap->param_name[SPLIN_INTP]  = "INTERPOLATION_STEPS";
        ap->param_name[SPLIN_SEED]  = "RANDOM_SEED";
        ap->param_name[SPLIN_MCNT]  = "MAXIMUM_SPLINE_COUNT";
        ap->param_name[SPLIN_MINTP] = "MAXIMUM_INTERPOLATION_STEPS";
        ap->param_name[SPLIN_DRIFT] = "MAXIMUM_PITCH_DRIFT_(SEMITONES)";
        ap->param_name[SPLIN_DRVEL] = "AVERAGE_TIME(mS)_BETWEEN_NEW_DRIFT_VALS";
        break;
    case(SPLINTER):
        ap->param_name[SPL_TIME]   = "TIME_(BEFORE)_WAVESET_GROUP";
        ap->param_name[SPL_WCNT]   = "WAVESET_COUNT";
        ap->param_name[SPL_SHRCNT] = "SHRINKING_SPLINTERS_COUNT";
        ap->param_name[SPL_OCNT]   = "SHRUNK_SPLINTERS_COUNT";
        ap->param_name[SPL_PULS1]  = "SPLINTER_PULSE_RATE_AT_ORIG_WAVESET";
        ap->param_name[SPL_PULS2]  = "GOAL_PULSE_RATE";
        ap->param_name[SPL_ECNT]   = "EXTRA_SPLINTERS_COUNT";
        ap->param_name[SPL_SCURVE] = "SHRINK_CONTOUR";
        ap->param_name[SPL_PCURVE] = "TIMING_CONTOUR";
        if(mode <= 1)
            ap->param_name[SPL_FRQ]= "FRQ_OF_SHRUNK_WAVESETS";
        else
            ap->param_name[SPL_DUR]= "DURATION(mS)_OF_SHRUNK_WAVESETS";
        ap->param_name[SPL_RND]    = "SPLINTER_TIMING_RANDOMISATION";
        ap->param_name[SPL_SHRND]  = "SPLINTER_SHRINKING_RANDOMISATION";
        break;
    case(REPEATER):
        if(mode >= 2) {
            ap->param_name[REP_ACCEL] = "ACCELERATION_OF_REPEATS";
            ap->param_name[REP_WARP]  = "WARP_OF_ACCELERATION";
            ap->param_name[REP_FADE]  = "FADE_CONTOUR";
        }
        ap->param_name[REP_RAND]   = "RANGE_OF_ANY_RANDOM_EXPANSION_OF_DELAY_TIMES";
        ap->param_name[REP_TRNSP]  = "RANGE_OF_ANY_RANDOM_PITCH_VARIATION(SEMITONES)";
        ap->param_name[REP_SEED]   = "RANDOM_SEED";
        break;
    case(VERGES):
        ap->param_name[VRG_TRNSP] = "VERGE_TRANSPOSITION(SEMITONES)";
        ap->param_name[VRG_CURVE] = "VERGE_SLOPE";
        ap->param_name[VRG_DUR]   = "VERGE_DURATION";
        break;
    case(MOTOR):
        ap->param_name[MOT_DUR]    = "TOTAL_OUTPUT_DURATION";
        ap->param_name[MOT_FRQ]    = "INNER_PULSE_FRQ(Hz)";
        ap->param_name[MOT_PULSE]  = "OUTER_PULSE_RATE(Hz)";
        ap->param_name[MOT_FRATIO] = "INNER_PULSES_PROPORTION_ON-TO-OFF_TIME";
        ap->param_name[MOT_PRATIO] = "OUTER_PULSES_PROPORTION_ON-TO-OFF_TIME";
        ap->param_name[MOT_SYM]    = "SYMMETRY_OF_OUTER_PULSES";
        ap->param_name[MOT_FRND]   = "INNER_PULSE_FRQ_RANDOMISATION";
        ap->param_name[MOT_PRND]   = "OUTER_PULSE_FRQ_RANDOMISATION";
        ap->param_name[MOT_JIT]    = "PITCH_RANDOMISATION_RANGE";
        ap->param_name[MOT_TREM]   = "ATTENUATION_RANDOMISATION_RANGE";
        ap->param_name[MOT_SYMRND] = "SYMMETRY_RANDOMISATION";
        ap->param_name[MOT_EDGE]   = "INNER_PULSES_DECAY_TAIL";
        ap->param_name[MOT_BITE]   = "OUTER_PULSE_SHARPNESS";
        ap->param_name[MOT_VARY]   = "READ_ADVANCE_RANDOMISATION";
        ap->param_name[MOT_SEED]   = "RANDOM_SEED";
        break;
    case(STUTTER):
        ap->param_name[STUT_DUR]    = "OUTPUT_DURATION";
        ap->param_name[STUT_JOIN]   = "MAX_SEGMENT_JOINS";
        ap->param_name[STUT_SIL]    = "PROPORTION_OF_SILENCE";
        ap->param_name[STUT_SILMIN] = "MIN_SILENCE_DURATION";
        ap->param_name[STUT_SILMAX] = "MAX_SILENCE_DURATION";
        ap->param_name[STUT_SEED]   = "RANDOM_SEED";
        ap->param_name[STUT_TRANS]  = "MAX_RANGE_OF_RANDOM_TRANSPOSITION(SEMITONES)";
        ap->param_name[STUT_ATTEN]  = "MAX_RANGE_OF_RANDOM_ATTENUATION";
        ap->param_name[STUT_BIAS]   = "SEGMENT_LENGTH_BIAS";
        ap->param_name[STUT_MINDUR] = "MIN_SEGMENT_DURATION(mS)";
        break;
    case(SCRUNCH):
        ap->param_name[SCR_DUR]   = "OUTPUT_DURATION";
        ap->param_name[SCR_SEED]  = "RANDOM_SEED";
        ap->param_name[SCR_CNT]   = "WAVESET_GROUPING_COUNT";
        ap->param_name[SCR_TRNS]  = "MAX_RANGE_OF_RANDOM_TRANSPOSITION(SEMITONES)"; 
        ap->param_name[SCR_ATTEN] = "MAX_RANGE_OF_RANDOM_ATTENUATION";
        break;
    case(IMPULSE):
        ap->param_name[IMP_DUR]   = "OUTPUT_DURATION";
        ap->param_name[IMP_PICH]  = "PITCH_OF_IMPULSE_STREAM(MIDI)";
        ap->param_name[IMP_CHIRP] = "GLISSING_OF_IMPULSE";
        ap->param_name[IMP_SLOPE] = "SLOPE_OF_IMPULSE"; 
        ap->param_name[IMP_CYCS]  = "NUMBER_OF_PEAKS_PER_IMPULSE";
        ap->param_name[IMP_LEV]   = "IMPULSE_LEVEL";
        ap->param_name[IMP_GAP]   = "PROPORTIONAL_GAP_BETWEEN_IMPULSES";
        ap->param_name[IMP_SRATE] = "SAMPLE_RATE";
        ap->param_name[IMP_CHANS] = "OUTPUT_CHANNEL_CNT";
        break;
    case(TWEET):
        ap->param_name[TWT_PDAT]  = "FREQUENCY_OF_SRC(HZ)";
        ap->param_name[TWT_MIN]   = "LOWEST_LEVEL_AT_WHICH_FOFS_DETECTED(dB)";
        switch(mode) {
        case(0):
            ap->param_name[TWT_PKCNT] = "NUMBER_OF_PEAKS_IN_CHIRP";
            ap->param_name[TWT_CHIRP] = "GLISSING_OF_CHIRP";
            break;
        case(1):
            ap->param_name[TWT_PKCNT] = "FREQUENCY_OF_CHIRP";
            ap->param_name[TWT_CHIRP] = "GLISSING_OF_CHIRP";
            break;
        }
        break;
    case(RRRR_EXTEND):  //  version 8+
        if(mode == 1) {
            ap->param_name[RRR_GATE]      = "GATE_LEVEL_BELOW_WHICH_SIGNAL_ENVELOPE_IGNORED";
            ap->param_name[RRR_SKIP]      = "NUMBER_OF_UNITS_AT_ITERATIVE_START_TO_OMIT";
            ap->param_name[RRR_GET]       = "MINIMUM_NO_OF_SEGMENTS_TO_FIND_IN_SRC";
            ap->param_name[RRR_GRSIZ]     = "APPROX_SIZE_OF_GRANULE_(MS)";
        } else {
            ap->param_name[RRR_START]     = "START_OF_SECTION_TO_BE_EXTENDED";
            ap->param_name[RRR_END]       = "END_OF_SECTION_TO_BE_EXTENDED";
            ap->param_name[RRR_GET]       = "ANTICIPATED_NO_OF_SEGMENTS_TO_FIND_IN_SRC";
        }
        ap->param_name[RRR_SLOW]          = "GRAIN_SEPARATION_TIMESTRETCH";
        ap->param_name[RRR_REGU]          = "GRAIN_SEPARATION_REGULARISATION";
        ap->param_name[RRR_RANGE]         = "APPROX_RANGE_OF_ITERATIVE_SOUND_(OCTAVES)";
        if(mode != 2) {
            ap->param_name[RRR_STRETCH]   = "TIME_EXTENSION_OF_MATERIAL_ITSELF";
            ap->param_name[RRR_REPET]     = "MAX_ADJACENT_OCCURENCES_OF_ANY_SEG_IN_OUTPUT";
            ap->param_name[RRR_ASCAT]     = "AMPLITUDE_SCATTER_(MULTIPLIER)";
            ap->param_name[RRR_PSCAT]     = "PITCH_SCATTER_(SEMITONES)";
        }
        break;
    case(SORTER):
        ap->param_name[SORTER_SIZE]     = "ELEMENT_SIZE";
        if(mode == 4)
            ap->param_name[SORTER_SEED] = "SEED";
        ap->param_name[SORTER_SMOOTH]   = "SMOOTHING";
        ap->param_name[SORTER_OMIDI]    = "OUTPITCH";
        ap->param_name[SORTER_IMIDI]    = "ELEMENT_PITCH";
        ap->param_name[SORTER_META]     = "META_GROUPING";
        break;
    case(SPECFNU):
        switch(mode) {
        case(F_NARROW):
            ap->param_name[NARROWING] = "NARROWING";
            ap->param_name[NARSUPRES] = "SUPPRESS_SET";
            ap->param_name[FGAIN]     = "GAIN";
            break;
        case(F_SQUEEZE):
            ap->param_name[SQZFACT] = "SQUEEZE";
            ap->param_name[SQZAT]   = "CENTRE_FORMANT";
            ap->param_name[FGAIN]   = "GAIN";
            break;
        case(F_INVERT):
            ap->param_name[FVIB]    = "VIBRATE_RATE";
            ap->param_name[FGAIN]   = "GAIN";
            break;
        case(F_ROTATE):
            ap->param_name[RSPEED]  = "ROTATION_SPEED";
            ap->param_name[FGAIN]   = "GAIN";
            break;
        case(F_NEGATE):
            ap->param_name[FGAIN]   = "GAIN";
            break;
        case(F_SUPPRESS):
            ap->param_name[SUPRF]   = "SUPPRESS_SET";
            ap->param_name[FGAIN]   = "GAIN";
            break;
        case(F_MAKEFILT):
            ap->param_name[FPKCNT] = "PEAKS_PER_FORMANT";
            ap->param_name[FBELOW] = "FORCE_A_PITCH_BELOW";
            break;
        case(F_MOVE):
            ap->param_name[FMOVE1]  = "MOVE1";
            ap->param_name[FMOVE2]  = "MOVE2";
            ap->param_name[FMOVE3]  = "MOVE3";
            ap->param_name[FMOVE4]  = "MOVE4";
            ap->param_name[FMVGAIN] = "GAIN";
            break;
        case(F_MOVE2):
            ap->param_name[FMOVE1]  = "FRQ1";
            ap->param_name[FMOVE2]  = "FRQ2";
            ap->param_name[FMOVE3]  = "FRQ3";
            ap->param_name[FMOVE4]  = "FRQ4";
            ap->param_name[FMVGAIN] = "GAIN";
            break;
        case(F_ARPEG):
            ap->param_name[FARPRATE] = "ARPEGGIATION_RATE";
            ap->param_name[FGAIN]    = "GAIN";
            break;
        case(F_OCTSHIFT):
            ap->param_name[COLINT]  = "OCTAVE_SHIFT";
            ap->param_name[FGAIN]   = "GAIN";
            ap->param_name[COL_LO]  = "LOW_CUT";
            ap->param_name[COL_HI]  = "HIGH_CUT";
            ap->param_name[COLRATE] = "ARPEGGIATION_RATE";
            break;
        case(F_TRANS):
            ap->param_name[COLFLT]  = "TRANSPOSITION_(SEMITONES)";
            ap->param_name[FGAIN]   = "GAIN";
            ap->param_name[COL_LO]  = "LOW_CUT";
            ap->param_name[COL_HI]  = "HIGH_CUT";
            ap->param_name[COLRATE] = "ARPEGGIATION_RATE";
            break;
        case(F_FRQSHIFT):
            ap->param_name[COLFLT]  = "FREQUENCY_SHIFT";
            ap->param_name[FGAIN]   = "GAIN";
            ap->param_name[COL_LO]  = "LOW_CUT";
            ap->param_name[COL_HI]  = "HIGH_CUT";
            ap->param_name[COLRATE] = "ARPEGGIATION_RATE";
            break;
        case(F_RESPACE):
            ap->param_name[COLFLT]  = "FREQUENCY_SPACING";
            ap->param_name[FGAIN]   = "GAIN";
            ap->param_name[COL_LO]  = "LOW_CUT";
            ap->param_name[COL_HI]  = "HIGH_CUT";
            ap->param_name[COLRATE] = "ARPEGGIATION_RATE";
            break;
        case(F_PINVERT):
            ap->param_name[COLFLT]   = "PIVOT_PITCH";
            ap->param_name[FGAIN]    = "GAIN";
            ap->param_name[COL_LO]   = "LOW_CUT";
            ap->param_name[COL_HI]   = "HIGH_CUT";
            ap->param_name[COLRATE]  = "ARPEGGIATION_RATE";
            ap->param_name[COLLOPCH] = "LOW_PITCH_LIMIT";
            ap->param_name[COLHIPCH] = "HIGH_PITCH_LIMIT";
            break;
        case(F_PEXAGG):
            ap->param_name[COLFLT]   = "PIVOT_PITCH";
            ap->param_name[EXAGRANG] = "RANGE_MULTIPLIER";
            ap->param_name[FGAIN]    = "GAIN";
            ap->param_name[COL_LO]   = "LOW_CUT";
            ap->param_name[COL_HI]   = "HIGH_CUT";
            ap->param_name[COLRATE]  = "ARPEGGIATION_RATE";
            ap->param_name[COLLOPCH] = "LOW_PITCH_LIMIT";
            ap->param_name[COLHIPCH] = "HIGH_PITCH_LIMIT";
            break;
        case(F_PQUANT):
            ap->param_name[FGAIN]    = "GAIN";
            ap->param_name[COL_LO]   = "LOW_CUT";
            ap->param_name[COL_HI]   = "HIGH_CUT";
            ap->param_name[COLRATE]  = "ARPEGGIATION_RATE";
            ap->param_name[COLLOPCH] = "LOW_PITCH_LIMIT";
            ap->param_name[COLHIPCH] = "HIGH_PITCH_LIMIT";
            break;
        case(F_PCHRAND):
            ap->param_name[FPRMAXINT] = "RANDOMISATION_RANGE";
            ap->param_name[FSLEW]     = "RANDOMISATION_SLEW";
            ap->param_name[FGAIN]     = "GAIN";
            ap->param_name[COL_LO]    = "LOW_CUT";
            ap->param_name[COL_HI]    = "HIGH_CUT";
            ap->param_name[COLRATE]   = "ARPEGGIATION_RATE";
            ap->param_name[COLLOPCH]  = "LOW_PITCH_LIMIT";
            ap->param_name[COLHIPCH]  = "HIGH_PITCH_LIMIT";
            break;
        case(F_RAND):
            ap->param_name[COLFLT]  = "RANDOMISATION";
            ap->param_name[FGAIN]   = "GAIN";
            ap->param_name[COL_LO]  = "LOW_CUT";
            ap->param_name[COL_HI]  = "HIGH_CUT";
            ap->param_name[COLRATE] = "ARPEGGIATION_RATE";
            break;
        case(F_SYLABTROF):
            ap->param_name[FMINSYL] = "MINIMUM_SYLLABLE_DURATION";
            ap->param_name[FMINPKG] = "MINIMUM_PEAK_HEIGTH";
            break;
        case(F_SINUS):
            ap->param_name[F_SINING] = "DEPTH";
            ap->param_name[FGAIN]    = "GAIN";
            ap->param_name[F_AMP1]   = "LEVEL1";
            ap->param_name[F_AMP2]   = "LEVEL2";
            ap->param_name[F_AMP3]   = "LEVEL3";
            ap->param_name[F_AMP4]   = "LEVEL4";
            ap->param_name[F_QDEP1]  = "HFIELD_DEPTH1";
            ap->param_name[F_QDEP2]  = "HFIELD_DEPTH2";
            ap->param_name[F_QDEP3]  = "HFIELD_DEPTH3";
            ap->param_name[F_QDEP4]  = "HFIELD_DEPTH4";
            break;
        }
        break;
    case(FLATTEN):
        ap->param_name[0]   = "ELEMENTSIZE";
        ap->param_name[1]   = "SHOULDER";
        ap->param_name[2]   = "TAIL";
        break;
    case(BOUNCE):
        ap->param_name[0]   = "NUMBER_OF_BOUNCES";
        ap->param_name[1]   = "FIRST_BOUNCE_EXTENT";
        ap->param_name[2]   = "BOUNCE_ACCEL";
        ap->param_name[3]   = "FINAL_LEVEL";
        ap->param_name[4]   = "LEVEL_DECAY_SLOPE";
        ap->param_name[5]   = "SHRINK_GRADUALLY(MIN_DURATION)";
        break;
    case(DISTMARK):
        ap->param_name[0]   = "WAVESETGROUP_LENGTH(mS)";
        ap->param_name[1]   = "TIMESTRETCH";
        ap->param_name[2]   = "GROUPLENGTH_RANDOMISATION";
        if(mode == 1)
            ap->param_name[3]   = "ATTENUATION_OF_DISTORTED_REGIONS";
        break;
    case(DISTREP):
        ap->param_name[0]   = "REPETITION_COUNT";
        ap->param_name[1]   = "COUNT_OF_WAVESETS_IN_GROUP_TO_REPEAT";
        ap->param_name[2]   = "WAVESETS_TO_SKIP_AT_START";
        ap->param_name[3]   = "SPLICELENGTH_(mS)";
        break;
    case(TOSTEREO):
        ap->param_name[0]   = "DIVERGE_START_TIME";
        ap->param_name[1]   = "DIVERGE_END_TIME";
        ap->param_name[2]   = "OUTPUT_CHANNELS";
        ap->param_name[3]   = "LEFT_CHANNEL_TO_:_or_FORK_FROM";
        ap->param_name[4]   = "RIGHT_CHANNEL_TO";
        ap->param_name[5]   = "CHANNEL_LEVELS_IN_MIX_TO_MONO";
        break;
    case(SUPPRESS):
        ap->param_name[0]   = "BAND_LOW_FREQUENCY";
        ap->param_name[1]   = "BAND_HIGH_FREQUENCY";
        ap->param_name[2]   = "PARTIALS_TO_SUPPRESS";
        break;
    case(CALTRAIN):
        ap->param_name[0]   = "AVERAGE_OVER_TIME_(SECS)";
        ap->param_name[1]   = "BLUR_ABOVE_FRQ_(HZ)";
        ap->param_name[2]   = "CUT_BASS_BELOW_FRQ_(HZ)";
        break;
    case(SPECENV):
        ap->param_name[0]   = "WINDOWSIZE_(ANALCHANCNT_OR_OCTAVES)";
        ap->param_name[1]   = "BALANCE_WITH_SOURCES";
        break;
    case(CLIP):
        switch(mode) {
        case(0):
            ap->param_name[0]   = "CLIPPING_LEVEL_IN_INPUT_SIGNAL";
            break;
        case(1):
            ap->param_name[0]   = "FRACTION_OF_WAVESET_LEVEL_AT_WHICH_TO_CLIP";
            break;
        }
        break;
    case(SPECEX):
        ap->param_name[0]   = "STARTTIME_OF_REGION_TO_STRETCH";
        ap->param_name[1]   = "DURATION_OF_REGION_TO_STRETCH";
        ap->param_name[2]   = "TIME_STRETCHING_RATIO";
        ap->param_name[3]   = "NO_OF_WINDOWS_IN_GROUPS-FOR-PERMUTATION";
        break;
    case(MATRIX):
        switch(mode) {
        case(MATRIX_USE):
            break;
        default:
            ap->param_name[MATRIX_CHANS] = "ANALYSIS_POINTS";
            ap->param_name[MATRIX_OVLAP] = "ANALWINDOW_OVERLAP";
            break;
        }
        break;
    case(TRANSPART):
        if(mode < 4)
            ap->param_name[0] = "TRANSPOSITION_IN_SEMITONES";
        else
            ap->param_name[0] = "FREQUNCY_SHIFT_IN_HZ";
        if(EVEN(mode))
            ap->param_name[1] = "ABOVE_THIS_FREQUENCY_ONLY";
        else
            ap->param_name[1] = "BELOW_THIS_FREQUENCY_ONLY";
        ap->param_name[2] = "OVERALL_GAIN";
        break;
    case(SPECINVNU):
        ap->param_name[0]   = "START_FRQ_OF_REGION_TO_INVERT";
        ap->param_name[1]   = "END_FRQ_OF_REGION_TO_INVERT";
        ap->param_name[2]   = "TOP_FRQ_TO_END_SEARCH_FOR_SPECTRAL_PEAK";
        ap->param_name[3]   = "GAIN";
        break;
    case(SPECCONV):
        ap->param_name[0]   = "OVERALL_GAIN";
        ap->param_name[1]   = "APPLY_PROCESS_THIS_MANY_TIMES";
        break;
    case(SPECSND):
        ap->param_name[0]   = "MINIMUM_UPWARD_TRANSPOSITION_IN_OCTAVES";
        ap->param_name[1]   = "MAXIMUM_UPWARD_TRANSPOSITION_IN_OCTAVES";
        break;
    case(FRACTAL):
        if(mode == 1)
            ap->param_name[0]   = "DURATION_OF_FRACTAL_PATTERN";
        ap->param_name[1]   = "NUMBER_OF_FRACTAL_LAYERS";
        ap->param_name[2]   = "TIME_STRETCH_OF_FRACTAL_PATTERN";
        ap->param_name[3]   = "INTERVAL_WARPING_OF_FRACTAL_PATTERN";
        break;
    case(FRACSPEC):
        ap->param_name[1]   = "NUMBER_OF_FRACTAL_LAYERS";
        ap->param_name[2]   = "TIME_STRETCH_OF_FRACTAL_PATTERN";
        ap->param_name[3]   = "INTERVAL_WARPING_OF_FRACTAL_PATTERN";
        break;
    case(SPECFRAC):
        ap->param_name[0]   = "NUMBER_OF_FRACTAL_LAYERS";
        break;
    case(ENVSPEAK):
        if(mode < 12)
            ap->param_name[0]   = "ENVELOPE_WINDOW_SIZE_(mS)";
        mode %= 12;
        ap->param_name[1]   = "SPLICE_LENGTH_(mS)";
        if(mode < 9) {
            ap->param_name[2] = "INITIAL_WINDOWS_TO_SKIP";
            switch(mode) {
            case(2):
                ap->param_name[3] = "N_(ATTEN_N_in_every_N+1_SYLLABLES)";
                ap->param_name[4] = "GAIN_ON_ATTENUATED_SYLLABLES";
                break;
            case(3):
                ap->param_name[3] = "N_(DON'T_ATTEN_N_in_every_N+1_SYLLABS)";
                ap->param_name[4] = "GAIN_ON_ATTENUATED_SYLLABLES";
                break;
            case(0):    //  fall thro
            case(4):    //  fall thro
            case(5):    //  fall thro
            case(7):    //  fall thro
            case(8):
                ap->param_name[3] = "NUMBER_OF_REPETITIONS";
                break;
            }
            if(!(mode == 1 || mode == 2 || mode == 3)) {
                ap->param_name[4] = "DEGREE_OF_RANDOMISATION_LENGTHS_OF_REPET_UNITS";
            }
            switch(mode) {
            case(6):
                ap->param_name[3] = "NO_OF_PARTS_TO_DIVIDE_SYLLABLES_INTO";
                ap->param_name[5] = "WHICH_DIVIDED_ELEMENT_TO_USE";
                break;
            case(7):    //  fall thro
            case(8):
                ap->param_name[5] = "CONTRACTION_RATIO";
                break;
            }
        }
        switch(mode) {
        case(10):
            ap->param_name[2] = "SEED_FOR_RANDOM_PERMUTATIONS";
            break;
        case(11):
            ap->param_name[2] = "REVERSE_ORDER_IN_GROUPS_OF";
            break;
        }
        break;
    case(EXTSPEAK):
        switch(mode) {
        case(0):    //  fall thro
        case(1):    //  fall thro
        case(3):    //  fall thro
        case(4):
            ap->param_name[5]   = "SEED_FOR_RANDOM_GENERATOR";
            //  fall thro
        case(2):    //  fall thro
        case(5):
            ap->param_name[0]   = "ENVELOPE_WINDOW_SIZE_(mS)";
            ap->param_name[1]   = "SPLICE_LENGTH_(mS)";
            ap->param_name[2]   = "SYLLABLES_AT_START_TO_OUTPUT_UNCHANGED";
            ap->param_name[3]   = "N_=_RETAIN_1_ORIG_SYLLAB_FOR_EVERY_N_OVERWRITTEN";
            ap->param_name[4]   = "0VERALL_ATTENUATION_OF_INSERTS";
            break;
        case(6):    //  fall thro
        case(7):    //  fall thro
        case(9):    //  fall thro
        case(10):
            ap->param_name[5]   = "SEED_FOR_RANDOM_GENERATOR";
            //  fall thro
        case(8):    //  fall thro
        case(11):
            ap->param_name[1]   = "SPLICE_LENGTH_(mS)";
            ap->param_name[2]   = "SYLLABLES_AT_START_TO_OUTPUT_UNCHANGED";
            ap->param_name[3]   = "N_=_RETAIN_1_ORIG_SYLLAB_FOR_EVERY_N_OVERWRITTEN";
            ap->param_name[4]   = "0VERALL_ATTENUATION_OF_INSERTS";
            break;
        case(12):   //  fall thro
        case(13):   //  fall thro
        case(15):   //  fall thro
        case(16):
            ap->param_name[5]   = "SEED_FOR_RANDOM_GENERATOR";
            //  fall thro
        case(14):   //  fall thro
        case(17):
            ap->param_name[1]   = "SPLICE_LENGTH_(mS)";
            ap->param_name[4]   = "0VERALL_ATTENUATION_OF_INSERTS";
            break;
        }
        break;
    case(ENVSCULPT):
        if(mode != 2)
            ap->param_name[0]   = "ENVELOPE_WINDOW_SIZE_(mS)";
        ap->param_name[1]   = "RISE_TIME_(mS)";
        ap->param_name[2]   = "DECAY_DURATION";
        ap->param_name[3]   = "STEEPNESS";
        if(mode == 1) {
            ap->param_name[4]   = "CONSONANT_DECAY_START";
            ap->param_name[5]   = "CONSONANT_DECAY_END";
        }
        if(mode != 0)
            ap->param_name[6]   = "LOUDNESS_RATIO_OF_FIRST_TO_SECOND_ATTACKS";
        break;
    case(TREMENV):
        ap->param_name[TREMOLO_FRQ]  = "TREMOLO_FREQUENCY";
        ap->param_name[TREMOLO_DEP]  = "TREMOLO_DEPTH";
        ap->param_name[TREMOLO_AMP]  = "WINDOW_SIZE_(mS)";
        ap->param_name[TREMOLO_SQZ]  = "PEAK_NARROWING";
        break;
    case(DCFIX):
        ap->param_name[0]  = "MINIMUM_DURATION_OF_DC_(mS)";
        break;
    default:
        sprintf(errstr,"Unknown case: get_param_names2()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** GET_PARAM_RANGES2 *********************************/

int get_param_ranges2
(int process,int mode,int total_params,double nyquist,float frametime,float arate,int srate,
int wlength,int insams,int channels,int wanted,
int filetype,int linecnt,double duration,aplptr ap)
{
    int exit_status;
    if((exit_status = setup_input_param_range_stores2(total_params,ap))<0)
        return(exit_status);
    return set_param_ranges2(process,mode,nyquist,frametime,arate,srate,wlength,
                            insams,channels,wanted,filetype,linecnt,duration,ap);
}       
    
/****************************** SETUP_INPUT_PARAM_RANGE_STORES2 *********************************/

int setup_input_param_range_stores2(int total_params,aplptr ap)
{
    if((ap->lo = (double *)malloc((total_params) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: range lo limits\n"); 
        return(MEMORY_ERROR);
    }
    if((ap->hi = (double *)malloc((total_params) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: range hi limits\n"); 
        return(MEMORY_ERROR);
    }
    return FINISHED;  /*RWD 9:2001 */
}

/****************************** SET_PARAM_RANGES2 *********************************/

int set_param_ranges2
(int process,int mode,double nyquist,float frametime,float arate,int srate,
int wlength,int insams,int channels,int wanted,
int filetype,int linecnt,double duration,aplptr ap)
{
    double infilesize_in_samps = 0.0, chwidth;

    switch(filetype) {
    case(SNDFILE):  
        infilesize_in_samps = (double)(insams/channels);    break;
    case(ENVFILE):
        infilesize_in_samps = duration;                     break;
    }
    switch(process) {
    case(TAPDELAY):
        ap->lo[0] = 0.000001;
        ap->hi[0] = 1000.0;
        ap->lo[1] = -1;
        ap->hi[1] = 1;
        ap->lo[2] = 0;
        ap->hi[2] = .999999;
        ap->lo[3] = 0.0;
        ap->hi[3] = 1000;
        break;
    case(RMRESP):
        ap->lo[0]  = 0;     /* liveness */
        ap->hi[0]  = 1;
        ap->lo[1]  = 1;     /* nrefs */
        ap->hi[1]  = 1000;
        ap->lo[2]  = .01;   /* roomL */
        ap->hi[2]  = 1000;
        ap->lo[3]  = .01;   /* roomW */
        ap->hi[3]  = 1000;
        ap->lo[4]  = .01;   /* roomH */
        ap->hi[4]  = 1000;
        ap->lo[5]  = 0;     /* srcL */
        ap->hi[5]  = 1000;
        ap->lo[6]  = 0;     /* srcW */
        ap->hi[6]  = 1000;
        ap->lo[7]  = 0;     /* srcH */
        ap->hi[7]  = 1000;
        ap->lo[8]  = 0;     /* listenerL */
        ap->hi[8]  = 1000;
        ap->lo[9]  = 0;     /* listenerW */
        ap->hi[9]  = 1000;
        ap->lo[10] = 0;     /* listenerH */
        ap->hi[10] = 1000;
        ap->lo[11] = 0.000001;  /* maxamp */
        ap->hi[11] = 1.0;
        ap->lo[12] = 0.1;   /* res */
        ap->hi[12] = 2;
        break;
    case(RMVERB):
        ap->lo[0]  = 1;         /* roomsize */
        ap->hi[0]  = 3;
        ap->lo[1]  = 0.000001;  /* dense_reverb_gain */
        ap->hi[1]  = 1;
        ap->lo[2]  = 0;         /* source_in_mix */
        ap->hi[2]  = 0.99999;
        ap->lo[3]  = 0;         /* feedback */
        ap->hi[3]  = 1;
        ap->lo[4]  = 0;         /* air-absorption_cutoff */
        ap->hi[4]  = nyquist;
        ap->lo[5]  = 0;         /* lopass_reverb-input_cutoff */
        ap->hi[5]  = nyquist;
        ap->lo[6]  = 0;         /* decay_tail */
        ap->hi[6]  = 1000.0;
        ap->lo[7]  = 0;     /* lopass_input_cutoff */
        ap->hi[7]  = nyquist;
        ap->lo[8]  = 0;     /* hipass_input_cutoff */
        ap->hi[8]  = nyquist;
        ap->lo[9]  = 0;         /* predelay */
        ap->hi[9]  = (duration + 1000.0) * SECS_TO_MS;
        ap->lo[10] = 1;         /* output_chans */
        ap->hi[10] = 16;
        break;
    case(MIXMULTI):
        ap->lo[MIX_START]   = 0.0;
        ap->hi[MIX_START]   = duration;
        ap->lo[MIX_END]     = 0.0;
        ap->hi[MIX_END]     = duration;
        ap->lo[MIX_ATTEN]   = 0.0;
        ap->hi[MIX_ATTEN]   = 1.0;
        break;
    case(ANALJOIN):
        break;
    case(PTOBRK):
        ap->lo[0]   = min(duration * SECS_TO_MS,1.0);
        ap->hi[0]   = min(duration * SECS_TO_MS,1000.0);
        break;
    case(PSOW_STRETCH):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[1]  = .1;            /* time_stretch */
        ap->hi[1]  = 10.0;
        ap->lo[2]  = 1;             /* no of grains per block */
        ap->hi[2]  = 256;
        break;
    case(PSOW_DUPL):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[1]  = 2;         /* no of dupls */
        ap->hi[1]  = 256;
        ap->lo[2]  = 1;             /* no of grains per block */
        ap->hi[2]  = 256;
        break;
    case(PSOW_DEL):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[1]  = 2;             /* proportion to keep */
        ap->hi[1]  = 20;
        ap->lo[2]  = 1;             /* no of grains per block */
        ap->hi[2]  = 256;
        break;
    case(PSOW_STRFILL):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[1]  = 1.0;           /* time_stretch */
        ap->hi[1]  = 10.0;
        ap->lo[2]  = 1;             /* no of grains per block */
        ap->hi[2]  = 256;
        ap->lo[3]  = -24.0;         /* transposition in 8vas */
        ap->hi[3]  = 24.0;
        break;
    case(PSOW_FREEZE):
        ap->lo[PS_TIME] = 0.0;
        ap->hi[PS_TIME] = duration;
        ap->lo[PS_DUR]  = 0.0;
        ap->hi[PS_DUR]  = 32767.0;
        ap->lo[PS_SEGS] = 1;
        ap->hi[PS_SEGS] = 256;
        ap->lo[PS_DENS] = .001;
        ap->hi[PS_DENS] = 100;
        ap->lo[PS_TRNS] = .125;
        ap->hi[PS_TRNS] = 8;
        ap->lo[PS_RAND] = 0.0;
        ap->hi[PS_RAND] = 1.0;
        ap->lo[PS_GAIN] = 0.0;
        ap->hi[PS_GAIN] = 1.0;
        break;
    case(PSOW_CHOP):
        ap->lo[1]   = 1.0;
        ap->hi[1]   = insams;
        break;
    case(PSOW_INTERP):
        ap->lo[PS_SDUR]      = 0.0;
        ap->hi[PS_SDUR]      = 32767.0;
        ap->lo[PS_IDUR]      = 0.0;
        ap->hi[PS_IDUR]      = 32767.0;
        ap->lo[PS_EDUR]      = 0.0;
        ap->hi[PS_EDUR]      = 32767.0;
        ap->lo[PS_VIBFRQ]    = 0.0;
        ap->hi[PS_VIBFRQ]    = 20;
        ap->lo[PS_VIBDEPTH]  = 0.0;
        ap->hi[PS_VIBDEPTH]  = 3.0;
        ap->lo[PS_TREMFRQ]   = 0.0;
        ap->hi[PS_TREMFRQ]   = 30.0;
        ap->lo[PS_TREMDEPTH] = 0.0;
        ap->hi[PS_TREMDEPTH] = 10.0;
        break;
    case(PSOW_FEATURES):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[1]  = 1;             /* no of grains per block */
        ap->hi[1]  = 256;
        ap->lo[2]  = -48;
        ap->hi[2]  = 96;
        ap->lo[3]  = 0.0;
        ap->hi[3]  = 20;
        ap->lo[4]  = 0.0;
        ap->hi[4]  = 3.0;
        ap->lo[5]  = -24;
        ap->hi[5]  = 24;
        ap->lo[6]  = 0;
        ap->hi[6]  = 1;
        ap->lo[7]  = 0;
        ap->hi[7]  = 1;
        ap->lo[8]  = 0;
        ap->hi[8]  = 8;
        ap->lo[9]  = 0;
        ap->hi[9]  = 1;
        ap->lo[10] = 1;
        ap->hi[10] = 4096;
        break;
    case(PSOW_SYNTH):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[1]  = 0.0;           /* depth */
        ap->hi[1]  = 1.0;
        break;
    case(PSOW_IMPOSE):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[1]  = 0.0;           /* depth */
        ap->hi[1]  = 1.0;
        ap->lo[2]  = 1.0;           /* windowsize (ms)  */
        ap->hi[2]  = 200.0;
        ap->lo[3]  = -96.0;         /* gate (db) */
        ap->hi[3]  = 0.0;
        break;
    case(PSOW_SPLIT):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[1]  = 3.0;           /* subharmonic_no */
        ap->hi[1]  = 8.0;
        ap->lo[2]  = 0.0;           /* upward_transposition_(semitones) */
        ap->hi[2]  = 48.0;
        ap->lo[3]  = 0.0;           /* subharmonic_level */
        ap->hi[3]  = 8.0;
        break;
    case(PSOW_SPACE):
        ap->lo[0] = -2.0;           /* pitch */
        ap->hi[0] = nyquist;
        ap->lo[1] = 2.0;            /* subharmonic_no */
        ap->hi[1] = 5.0;
        ap->lo[2] = -1;             /* spatial separation */
        ap->hi[2] = 1;
        ap->lo[3] = .001;           /* LR relative level */
        ap->hi[3] = 1000;
        ap->lo[4] = 0.0;            /* LOHI relative level */
        ap->hi[4] = 1.0;
        break;
    case(PSOW_INTERLEAVE):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[1]  = -2.0;          /* pitch */
        ap->hi[1]  = nyquist;
        ap->lo[2]  = 1;             /* fofs_per_cunk */
        ap->hi[2]  = 16;
        ap->lo[3] = -1.0;           /* pitch biasing */
        ap->hi[3] = 1.0;
        ap->lo[4] = 0.0001;         /* relative level */
        ap->hi[4] = 10000.0;
        ap->lo[5] = .0625;          /* weighting */
        ap->hi[5] = 16;
        break;
    case(PSOW_REPLACE):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[1]  = -2.0;          /* pitch */
        ap->hi[1]  = nyquist;
        ap->lo[2]  = 1;             /* fofs_per_cunk */
        ap->hi[2]  = 16;
        break;
    case(PSOW_EXTEND):
        ap->lo[0]  = -2.0;              /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[PS_TIME]  = 0.0;         /* grabtime */
        ap->hi[PS_TIME]  = duration;    
        ap->lo[PS_DUR]   = duration;    /* TOTAL duration of output */
        ap->hi[PS_DUR]   = 32767.0;
        ap->lo[PS_SEGS]  = 1;
        ap->hi[PS_SEGS]  = 256;
        ap->lo[PSE_VFRQ] = 0.0;
        ap->hi[PSE_VFRQ] = 20;
        ap->lo[PSE_VDEP] = 0.0;
        ap->hi[PSE_VDEP] = 3.0;
        ap->lo[PSE_TRNS] = -48.0;
        ap->hi[PSE_TRNS] = 24.0;
        ap->lo[PSE_GAIN] = 0.0;
        ap->hi[PSE_GAIN] = 10.0;
        break;
    case(PSOW_EXTEND2):
        ap->lo[0]    = 0.0;         /* grabtime */
        ap->hi[0]    = duration;    
        ap->lo[1]    = 0.0;         /* grabtime */
        ap->hi[1]    = duration;    
        ap->lo[PS_DUR]   = duration;    /* TOTAL duration of output */
        ap->hi[PS_DUR]   = 32767.0;
        ap->lo[PS2_VFRQ] = 0.0;
        ap->hi[PS2_VFRQ] = 20;
        ap->lo[PS2_VDEP] = 0.0;
        ap->hi[PS2_VDEP] = 3.0;
        ap->lo[PS2_NUJ]  = -24.0;
        ap->hi[PS2_NUJ]  = 24.0;
        break;
    case(PSOW_LOCATE):
        ap->lo[0]  = -2.0;              /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[PS_TIME]  = 0.0;         /* time */
        ap->hi[PS_TIME]  = duration;    
        break;
    case(PSOW_CUT):
        ap->lo[0]  = -2.0;              /* pitch */
        ap->hi[0]  = nyquist;
        ap->lo[PS_TIME]  = 0.0;         /* time */
        ap->hi[PS_TIME]  = duration;    
        break;
    case(ONEFORM_GET):
        ap->lo[0]  = 0.0;
        ap->hi[0]  = duration;
        break;
    case(ONEFORM_PUT):
        ap->lo[FORM_FTOP]       = PITCHZERO;
        ap->hi[FORM_FTOP]       = nyquist;
        ap->lo[FORM_FBOT]       = PITCHZERO;
        ap->hi[FORM_FBOT]       = nyquist;
        ap->lo[FORM_GAIN]       = FLTERR;
        ap->hi[FORM_GAIN]       = FORM_MAX_GAIN;
        break;
    case(ONEFORM_COMBINE):
        break;
    case(NEWGATE):
        ap->lo[0] = -96.0;
        ap->hi[0] = 0.0;
        break;
    case(SPEC_REMOVE):
        ap->lo[0] = MIDIMIN;
        ap->hi[0] = MIDIMAX;
        ap->lo[1] = MIDIMIN;
        ap->hi[1] = MIDIMAX;
        ap->lo[2] = SPEC_MINFRQ;
        ap->hi[2] = nyquist;
        ap->lo[3] = 0.0;
        ap->hi[3] = 1.0;
        break;
    case(PREFIXSIL):
        ap->lo[0] = 0.0;
        ap->hi[0] = 32767.0;
        break;
    case(STRANS):
        switch(mode) {
        case(0):
            ap->lo[VTRANS_TRANS] = MIN_TRANSPOS;
            ap->hi[VTRANS_TRANS] = MAX_TRANSPOS;
            break;
        case(1):           
            ap->lo[VTRANS_TRANS] = EIGHT_8VA_DOWN;
            ap->hi[VTRANS_TRANS] = EIGHT_8VA_UP;
            break;
        case(2):           
            ap->lo[ACCEL_ACCEL]     = MIN_ACCEL;
            ap->hi[ACCEL_ACCEL]     = MAX_ACCEL;
            ap->lo[ACCEL_GOALTIME]  = MINTIME_ACCEL;
            ap->hi[ACCEL_GOALTIME]  = duration;
            ap->lo[ACCEL_STARTTIME] = 0.0;
            ap->hi[ACCEL_STARTTIME] = duration - MINTIME_ACCEL;
            break;
        case(3):           
            ap->lo[VIB_FRQ]     = 0.0;
            ap->hi[VIB_FRQ]     = MAX_VIB_FRQ;
            ap->lo[VIB_DEPTH]   = 0.0;
            ap->hi[VIB_DEPTH]   = EIGHT_8VA_UP;
            break;
        }
        break;
    case(PSOW_REINF):
        ap->lo[0]  = -2.0;          /* pitch */
        ap->hi[0]  = nyquist;
        if(mode == 0) {
            ap->lo[1]  = 0.0;
            ap->hi[1]  = 1000.0;
        } else {
            ap->lo[1]  = 1.0;
            ap->hi[1]  = 256.0;
        }
        break;
    case(PARTIALS_HARM):  
        ap->lo[0] = SPEC_MINFRQ;
        ap->hi[0] = nyquist;
        ap->lo[1] = FLTERR;
        ap->hi[1] = 1.0;
        if(mode > 1) {
            ap->lo[2] = 0.0;
            ap->hi[2] = duration;
        }
        break;
    case(SPECROSS):
        ap->lo[PICH_RNGE]           = 0.0;
        ap->hi[PICH_RNGE]           = 6.0;
        ap->lo[PICH_VALID]          = 0.0;
        ap->hi[PICH_VALID]          = (double)wlength;
        ap->lo[PICH_SRATIO]         = 0.0;
        ap->hi[PICH_SRATIO]         = SIGNOIS_MAX;
        ap->lo[PICH_MATCH]          = 1.0;
        ap->hi[PICH_MATCH]          = (double)MAXIMI;
        ap->lo[PICH_HILM]           = SPEC_MINFRQ;
        ap->hi[PICH_HILM]           = nyquist/MAXIMI;
        ap->lo[PICH_LOLM]           = SPEC_MINFRQ;
        ap->hi[PICH_LOLM]           = nyquist/MAXIMI;
        ap->lo[PICH_THRESH]         = 0.0;
        ap->hi[PICH_THRESH]         = 1.0;
        ap->lo[SPCMPLEV]            = 0.0;
        ap->hi[SPCMPLEV]            = 1.0;
        ap->lo[SPECHINT]            = 0.0;
        ap->hi[SPECHINT]            = 1.0;
        break;
    case(LUCIER_GETF):
        ap->lo[LUCIER_CUT]          = MINPITCH;
        ap->hi[LUCIER_CUT]          = nyquist/2.0;
        /* fall thro */
    case(LUCIER_GET):
        ap->lo[MIN_ROOM_DIMENSION]  = SPEED_OF_SOUND/(2.0 * nyquist);
        ap->hi[MIN_ROOM_DIMENSION]  = SPEED_OF_SOUND/(2.0 * MINPITCH);
        ap->lo[ROLLOFF_INTERVAL]    = 0.0;
        ap->hi[ROLLOFF_INTERVAL]    = 48.0; /* 4 8va */
        break;
    case(LUCIER_PUT):
        ap->lo[RESON_CNT]           = 1.0;
        ap->hi[RESON_CNT]           = 100.0;
        ap->lo[RES_EXTEND_ATTEN]    = 0.0;
        ap->hi[RES_EXTEND_ATTEN]    = 1.0;
        break;
    case(LUCIER_DEL):
        ap->lo[SUPR_COEFF]          = 0.0;
        ap->hi[SUPR_COEFF]          = 1.0;
        break;
    case(SPECTRACT):
    case(SPECLEAN):
        ap->lo[0]   = 1.0;
        ap->hi[0]   = 1000.0;
        ap->lo[1]   = 1.0;
        ap->hi[1]   = CL_MAX_GAIN;
        break;
    case(PHASE):
        if(mode == 1) {
            ap->lo[0]   = 0.0;
            ap->hi[0]   = 1.0;
        }
        break;
    case(SPECSLICE):
        channels = (channels-1)/2;
        chwidth = nyquist/(double)channels;
        switch(mode) {
        case(0):
            ap->lo[0]           = 2;
            ap->hi[0]           = channels;
            ap->lo[1]           = 1;
            ap->hi[1]           = channels;
            break;
        case(1):
            ap->lo[0]           = 2;
            ap->hi[0]           = channels;
            ap->lo[1]           = chwidth;
            ap->hi[1]           = nyquist/2.0;
            break;
        case(2):
            ap->lo[0]           = 2;
            ap->hi[0]           = channels;
            ap->lo[1]           = 0.5;
            ap->hi[1]           = (LOG2(nyquist/chwidth)/2.0) * SEMITONES_PER_OCTAVE;
            break;
        case(4):
            ap->lo[0]           = chwidth;
            ap->hi[0]           = nyquist - chwidth;
            break;
        }
        break;
    case(FOFEX_CO):
        if(mode == FOF_MEASURE)
            break;
        ap->lo[0]           = -2.0;
        ap->hi[0]           = nyquist;
        ap->lo[1]           = 0;
        ap->hi[1]           = 1;
        ap->lo[2]           = 0;
        ap->hi[2]           = 2;
        switch(mode) {
        case(FOF_SINGLE):
            ap->lo[3]           = 1;
            ap->hi[3]           = 1024;
            break;
        case(FOF_LOHI):
            ap->lo[3]           = 0;
            ap->hi[3]           = 32167;
            ap->lo[4]           = 0;
            ap->hi[4]           = 32167;
            ap->lo[5]           = 55;
            ap->hi[5]           = 880;
            ap->lo[6]           = 55;
            ap->hi[6]           = 880;
            break;
        case(FOF_TRIPLE):
            ap->lo[3]           = 0;
            ap->hi[3]           = 32167;
            ap->lo[4]           = 0;
            ap->hi[4]           = 32167;
            ap->lo[5]           = 0;
            ap->hi[5]           = 32167;
            ap->lo[6]           = 55;
            ap->hi[6]           = 880;
            ap->lo[7]           = 55;
            ap->hi[7]           = 880;
            ap->lo[8]           = 0;
            ap->hi[8]           = 1;
            ap->lo[9]           = 0;
            ap->hi[9]           = 1;
            break;
        }
        break;
    case(FOFEX_EX):
        ap->lo[0]           = -2.0;
        ap->hi[0]           = nyquist;
        switch(mode) {
        case(0):
        case(2):
            ap->lo[1]           = -60.0;
            ap->hi[1]           = 0.0;
            break;
        case(1):
            ap->lo[1]           = 0.0;
            ap->hi[1]           = duration;
            break;
        }
        ap->lo[2]           = 1;
        ap->hi[2]           = 16;
        break;
    case(GREV_EXTEND):
        ap->lo[GREV_WSIZE]   = (8.0/srate) * SECS_TO_MS;
        ap->hi[GREV_WSIZE]   = (duration/3.0) * SECS_TO_MS;
        ap->lo[GREV_TROFRAC] = FLTERR;
        ap->hi[GREV_TROFRAC] = 1.0 - FLTERR;
        ap->lo[2]            = FLTERR;
        ap->hi[2]            = 3600;
        ap->lo[3]            = 0.0;
        ap->hi[3]            = duration;
        ap->lo[4]            = 0.0;
        ap->hi[4]            = duration;
        break;
    case(PEAKFIND):
        ap->lo[0]   = 1;
        ap->hi[0]   = 500;
        ap->lo[1]   = 0.0;
        ap->hi[1]   = 1.0;
        break;
    case(CONSTRICT):
        ap->lo[0]   = 0;
        ap->hi[0]   = 200;
        break;
    case(EXPDECAY):
        ap->lo[0]   = 0;
        ap->hi[0]   = duration;
        ap->lo[1]   = 0;
        ap->hi[1]   = duration;
        break;
    case(PEAKCHOP):
        ap->lo[PKCH_WSIZE]  = 1.0;  //wsize for envelope extraction
        ap->hi[PKCH_WSIZE]  = min(duration,1.0) * SECS_TO_MS;
        ap->lo[PKCH_WIDTH]  = 0.0;  // peakwidth (mS)
        ap->hi[PKCH_WIDTH]  = 1000.0;
        ap->lo[PKCH_SPLICE] = 1.0;  // risetime (mS)
        ap->hi[PKCH_SPLICE] = 200;
        ap->lo[PKCH_GATE]   = 0.0;  // gate (0-1)
        ap->hi[PKCH_GATE]   = 1.0;
        ap->lo[PKCH_SKEW]   = 0.0;  // centring (0-1)
        ap->hi[PKCH_SKEW]   = 1.0;
        if(mode == 0 || mode == 2) {
            ap->lo[PKCH_TEMPO]  = 20;   // tempo (MM)
            ap->hi[PKCH_TEMPO]  = 3000;
            ap->lo[PKCH_GAIN]    = 0.0; // overall gain
            ap->hi[PKCH_GAIN]    = 1.0;
            ap->lo[PKCH_SCAT]   = 0.0;  // scatter (0-1)
            ap->hi[PKCH_SCAT]   = 1.0;
            ap->lo[PKCH_NORM]   = 0.0;  // normalise (0-1)
            ap->hi[PKCH_NORM]   = 1.0;
            ap->lo[PKCH_REPET]  = 0;    // repeat attacks
            ap->hi[PKCH_REPET]  = 256;
        }
        if(mode == 0) {
            ap->lo[PKCH_MISS]   = 0;    // skip attacks (i.e. take every n+1th attack only)
            ap->hi[PKCH_MISS]   = 64;
        }
        break;
    case(MCHANPAN):
        switch(mode) {
        case(9):
            ap->lo[3]   = 1;
            ap->hi[3]   = 100;
            /* fall thro */
        case(1):
            ap->lo[2]   = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[2]   = (32767.0/(double)srate) * SECS_TO_MS;
            /* fall thro */
        case(0):
            ap->lo[0]   = 3;
            ap->hi[0]   = 16;
            ap->lo[1]   = 0;
            ap->hi[1]   = 1;
            break;
        case(2):
            ap->lo[0]   = 3;
            ap->hi[0]   = 16;
            ap->lo[1]   = 1.0;          // centre
            ap->hi[1]   = 16.0;
            ap->lo[2]   = 1.0;          // spread
            ap->hi[2]   = 16.0;
            ap->lo[3]   = 1.0;          // depth
            ap->hi[3]   = 8;
            ap->lo[4]   = 0.0;          // rolloff
            ap->hi[4]   = 1.0;
            ap->lo[5]   = (2.0/(double)srate) * SECS_TO_MS;     //  inter-event silence
            ap->hi[5]   = (32767.0/(double)srate) * SECS_TO_MS;
        case(3):
            ap->lo[0]   = 3;
            ap->hi[0]   = 16;
            ap->lo[1]   = 0.0;          // centre
            ap->hi[1]   = 16.0;
            ap->lo[2]   = 0.0;          // spread
            ap->hi[2]   = 16.0;
            ap->lo[3]   = 0.0;          // depth
            ap->hi[3]   = 8;
            ap->lo[4]   = 0.0;          // rolloff
            ap->hi[4]   = 1.0;
            break;
        case(4):
            ap->lo[0]   = 3;
            ap->hi[0]   = 16;
            ap->lo[1]   = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[1]   = (32767.0/(double)srate) * SECS_TO_MS;
            break;
        case(5):
            ap->lo[0]   = 3;
            ap->hi[0]   = 16;
            ap->lo[1]   = 0.0;
            ap->hi[1]   = 60.0;
            ap->lo[2]   = 0.0;
            ap->hi[2]   = 60.0;
            ap->lo[3]   = 0.0;
            ap->hi[3]   = 1000.0;
            break;
        case(6):
            ap->lo[0]   = 0.0;          // rolloff
            ap->hi[0]   = 1.0;
            break;
        case(7):
            ap->lo[0]   = 0.0;
            ap->hi[0]   = 16.0;
            ap->lo[1]   = 0.0;
            ap->hi[1]   = 16.0;
            break;
        case(8):
            ap->lo[0]   = 3;
            ap->hi[0]   = 16;
            ap->lo[1]   = 1;
            ap->hi[1]   = 16;
            ap->lo[2]   = 0;
            ap->hi[2]   = 64;
            ap->lo[3]   = 0;
            ap->hi[3]   = 1;
            break;
        }
        break;
    case(TEX_MCHAN):
        ap->lo[TEXTURE_DUR]         = TEXTURE_MIN_DUR;
        ap->hi[TEXTURE_DUR]         = BIG_TIME;
        ap->lo[TEXTURE_PACK]        = 1.0/srate;
        ap->hi[TEXTURE_PACK]        = MAX_PACKTIME;
        ap->lo[TEXTURE_SCAT]        = 0.0;
        ap->hi[TEXTURE_SCAT]        = MAX_SCAT_TEXTURE;
        ap->lo[TEXTURE_TGRID]       = 0.0;
        ap->hi[TEXTURE_TGRID]       = TEXTURE_MAX_TGRID;
        ap->lo[TEXTURE_INSLO]       = 1.0;
        ap->hi[TEXTURE_INSLO]       = (double)SF_MAXFILES;
        ap->lo[TEXTURE_INSHI]       = 1.0;
        ap->hi[TEXTURE_INSHI]       = (double)SF_MAXFILES;
        ap->lo[TEXTURE_MAXAMP]      = MIDIBOT;
        ap->hi[TEXTURE_MAXAMP]      = MIDITOP; /* default 64 */
        ap->lo[TEXTURE_MINAMP]      = MIDIBOT;
        ap->hi[TEXTURE_MINAMP]      = MIDITOP; /* default 64 */
        ap->lo[TEXTURE_MAXDUR]      = (TEXTURE_SPLICELEN + TEXTURE_SAFETY) * MS_TO_SECS;
        ap->hi[TEXTURE_MAXDUR]      = BIG_TIME; 
        ap->lo[TEXTURE_MINDUR]      = (TEXTURE_SPLICELEN + TEXTURE_SAFETY) * MS_TO_SECS;
        ap->hi[TEXTURE_MINDUR]      = BIG_TIME; 
        ap->lo[TEXTURE_MAXPICH]     = MIDIBOT;
        ap->hi[TEXTURE_MAXPICH]     = MIDITOP;
        ap->lo[TEXTURE_MINPICH]     = MIDIBOT;
        ap->hi[TEXTURE_MINPICH]     = MIDITOP;
        ap->lo[TEXTURE_OUTCHANS]    = 2;
        ap->hi[TEXTURE_OUTCHANS]    = 16;
        ap->lo[TEXTURE_ATTEN]       = FLTERR;
        ap->hi[TEXTURE_ATTEN]       = 1.0;
        ap->lo[TEXTURE_POS]         = 0.0;
        ap->hi[TEXTURE_POS]         = 16.0;
        ap->lo[TEXTURE_SPRD]        = 0.0;
        ap->hi[TEXTURE_SPRD]        = 16.0;
        ap->lo[TEXTURE_SEED]        = 0.0;
        ap->hi[TEXTURE_SEED]        = MAXSHORT;
        break;
    case(MANYSIL):
        ap->lo[SIL_SPLICELEN]       = 0.0; /* default */
        ap->hi[SIL_SPLICELEN]       = 1000.0;
        break;
    case(RETIME):
        switch(mode) {
        case(0):
            ap->lo[MM]      = 0.0;
            ap->hi[MM]      = 400.0;
            break;
        case(1):
            ap->lo[MM]      = 0.0;
            ap->hi[MM]      = 400.0;
            ap->lo[RETIME_WIDTH]        = 1.0;
            ap->hi[RETIME_WIDTH]        = 1000.0;
            ap->lo[RETIME_SPLICE]       = 1.0;
            ap->hi[RETIME_SPLICE]       = 1000.0;
            break;
        case(2):       
            ap->lo[0]    = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[0]    = 10000.0;
            ap->lo[1]   = 1.0;
            ap->hi[1]   = 1000.0;
            ap->lo[2]   = 1.0;
            ap->hi[2]   = 1000.0;
            ap->lo[3]   = 1.0;
            ap->hi[3]   = 1000.0;
            break;
        case(3):
            ap->lo[0]   = 0.0;
            ap->hi[0]   = 6000.0;
            ap->lo[1]   = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[1]   = 10000.0;
            ap->lo[2]   = 0.0;
            ap->hi[2]   = 1.0;
            break;
        case(4):
            ap->lo[0]   = 0.01;
            ap->hi[0]   = 100.0;
            ap->lo[1]   = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[1]   = 10000.0;
            ap->lo[2]   = 0;
            ap->hi[2]   = duration;
            ap->lo[3]   = 0;
            ap->hi[3]   = duration;
            ap->lo[4]   = 0;
            ap->hi[4]   = duration;
            break;
        case(5):
            ap->lo[0]   = 0.01;         // MM
            ap->hi[0]   = 1000.0;
            ap->lo[1]   = 0.00;         // Offset
            ap->hi[1]   = 1000.0;
            ap->lo[2]   = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[2]   = 10000.0;      // Minsil
            ap->lo[3]   = 0.0;
            ap->hi[3]   = 1.0;          // Pregain
            break;
        case(6):
            ap->lo[0]   = 0.00;         // Offset
            ap->hi[0]   = 1000.0;
            ap->lo[1]   = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[1]   = 10000.0;      // Minsil
            ap->lo[2]   = 0.0;
            ap->hi[2]   = 1.0;          // Pregain
            break;
        case(7):
            ap->lo[MM]  = 0.01;
            ap->hi[MM]  = 1000.0;
            ap->lo[BEAT_AT] = 0.0;
            ap->hi[BEAT_AT] = duration;
            ap->lo[BEAT_CNT] = 1.0;
            ap->hi[BEAT_CNT] = 24.0;
            ap->lo[BEAT_REPEATS] = 1.0;
            ap->hi[BEAT_REPEATS] = 1000.0;
            ap->lo[BEAT_SILMIN]  = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[BEAT_SILMIN]  = 10000.0;
            break;
        case(8):
            ap->lo[0]   = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[0]   = 10000.0;      // Minsil
            break;
        case(9):
            ap->lo[0]   = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[0]   = 10000.0;
            ap->lo[1]   = 0.0;
            ap->hi[1]   = 1.0;
            ap->lo[2]   = 0;
            ap->hi[2]   = 32767;
            ap->lo[3]   = 0.0;
            ap->hi[3]   = 1.0;
            break;
        case(10):
            ap->lo[0]   = (2.0/(double)srate) * SECS_TO_MS;
            ap->hi[0]   = 10000.0;
            break;
        case(12):
            ap->lo[0]   = 0.0;
            ap->hi[0]   = 3600.0;
            break;
        case(13):
            ap->lo[0]   = 0.0;
            ap->hi[0]   = 3600.0;
            ap->lo[1]   = 0.0;
            ap->hi[1]   = duration;
            break;
        }
        break;
    case(HOVER):
        ap->lo[0]   = 1.0 / (duration * 2.0);
        ap->hi[0]   = nyquist/2.0;
        ap->lo[1]   = 0.0;
        ap->hi[1]   = duration;
        ap->lo[2]   = 0.0;
        ap->hi[2]   = 1.0;
        ap->lo[3]   = 0.0;
        ap->hi[3]   = 1.0;
        ap->lo[4]   = 0.0;
        ap->hi[4]   = 100.0;
        ap->lo[5]   = 0.0;
        ap->hi[5]   = 32767.0;
        break;
    case(HOVER2):
        ap->lo[0]   = 1.0 / (duration * 2.0);
        ap->hi[0]   = nyquist/2.0;
        ap->lo[1]   = 0.0;
        ap->hi[1]   = duration;
        ap->lo[2]   = 0.0;
        ap->hi[2]   = 1.0;
        ap->lo[3]   = 0.0;
        ap->hi[3]   = 1.0;
        ap->lo[4]   = 0.0;
        ap->hi[4]   = 32767.0;
        break;
    case(MULTIMIX):
        switch(mode) {
        case(2):
            ap->lo[0]   = 0.0;
            ap->hi[0]   = 10000.0;
            break;
        case(3):
            ap->lo[0]   = 0.0;
            ap->hi[0]   = 256.0;
            break;
        case(4):
            ap->lo[0]   = 0.0;
            ap->hi[0]   = 1.0;
            ap->lo[1]   = 0.0;
            ap->hi[1]   = 1.0;
            ap->lo[2]   = 0.0;
            ap->hi[2]   = 1.0;
            ap->lo[3]   = 0.0;
            ap->hi[3]   = 1.0;
            break;
        case(6):
            ap->lo[0]   = 2;
            ap->hi[0]   = 16.0;
            ap->lo[1]   = 1;
            ap->hi[1]   = 16.0;
            ap->lo[2]   = -16;
            ap->hi[2]   = 16;
            ap->lo[3]   = 0;
            ap->hi[3]   = 3600;
            break;
        case(7):
            ap->lo[0]   = 2;
            ap->hi[0]   = 16.0;
            break;
        }
        break;
    case(FRAME):
        switch(mode) {
        case(0):
            ap->lo[0]   = -500.0;
            ap->hi[0]   = 500.0;
            ap->lo[1]   = 0.0;
            ap->hi[1]   = 0.5;
            break;
        case(1):
            ap->lo[0]   = -500.0;
            ap->hi[0]   = 500.0;
            ap->lo[1]   = -500.0;
            ap->hi[1]   = 500.0;
            ap->lo[2]   = 0.0;
            ap->hi[2]   = 0.5;
            break;
        case(2):
        case(4):
        case(7):
            break;
        case(3):
            ap->lo[0]   = 1.0;
            ap->hi[0]   = 16.5;
            break;
        case(5):
            ap->lo[0]   = 1.0;
            ap->hi[0]   = 16.0;
            ap->lo[1]   = 1.0;
            ap->hi[1]   = 16.0;
            break;
        case(6):
            ap->lo[0]   = 0.0;
            ap->hi[0]   = 1.0;
            break;
        }
        break;
    case(SEARCH):
        break;
    case(MCHANREV):
        ap->lo[STAD_PREGAIN] = 1.0/(double)MAXSHORT;
        ap->hi[STAD_PREGAIN] = 1.0;
        ap->lo[STAD_ROLLOFF] = 1.0/(double)MAXSHORT;
        ap->hi[STAD_ROLLOFF] = 1.0;
        ap->lo[STAD_SIZE]    = (DFLT_STAD_DELTIME * 2.0)/srate;
        ap->hi[STAD_SIZE]    = MAX_STAD_DELAY;  /* arbitrary */
        ap->lo[STAD_ECHOCNT] = (double)2;
        ap->hi[STAD_ECHOCNT] = (double)MAX_ECHOCNT;
        ap->lo[REV_OCHANS]   = (double)2;
        ap->hi[REV_OCHANS]   = (double)16;
        ap->lo[REV_CENTRE]   = (double)0;
        ap->hi[REV_CENTRE]   = (double)16;
        ap->lo[REV_SPREAD]   = (double)2;
        ap->hi[REV_SPREAD]   = (double)16;
        break;
    case(WRAPPAGE):
        ap->lo[WRAP_OUTCHANS]   = -16.0;
        ap->hi[WRAP_OUTCHANS]   = 16.0;
        ap->lo[WRAP_SPREAD]     = 0.0;
        ap->hi[WRAP_SPREAD]     = 16.0;
        ap->lo[WRAP_DEPTH]      = 0.0;
        ap->hi[WRAP_DEPTH]      = 16.0;
        ap->lo[WRAP_VELOCITY]   = 0.0;
        ap->hi[WRAP_VELOCITY]   = WRAP_MAX_VELOCITY;
        ap->lo[WRAP_HVELOCITY]  = 0.0;
        ap->hi[WRAP_HVELOCITY]  = WRAP_MAX_VELOCITY;
        ap->lo[WRAP_DENSITY]    = FLTERR;
        ap->hi[WRAP_DENSITY]    = WRAP_MAX_DENSITY;
        ap->lo[WRAP_HDENSITY]   = (1.0/srate);
        ap->hi[WRAP_HDENSITY]   = (double)MAXSHORT/2.0;
        ap->lo[WRAP_GRAINSIZE]  = WRAP_MIN_SPLICELEN * 2.0;
        ap->hi[WRAP_GRAINSIZE]  = (infilesize_in_samps/srate) * SECS_TO_MS;
        ap->lo[WRAP_HGRAINSIZE] = WRAP_MIN_SPLICELEN * 2.0;
        ap->hi[WRAP_HGRAINSIZE] = (infilesize_in_samps/srate) * SECS_TO_MS;
        ap->hi[WRAP_PITCH]      = LOG2(nyquist/WRAP_MIN_LIKELY_PITCH) * SEMITONES_PER_OCTAVE;
        ap->lo[WRAP_PITCH]      = -(ap->hi[WRAP_PITCH]);
        ap->hi[WRAP_HPITCH]     = LOG2(nyquist/WRAP_MIN_LIKELY_PITCH) * SEMITONES_PER_OCTAVE;
        ap->lo[WRAP_HPITCH]     = -(ap->hi[WRAP_HPITCH]);
        ap->lo[WRAP_AMP]        = 0.0;
        ap->hi[WRAP_AMP]        = 1.0;
        ap->lo[WRAP_HAMP]       = 0.0;
        ap->hi[WRAP_HAMP]       = 1.0;
        ap->lo[WRAP_BSPLICE]    = WRAP_MIN_SPLICELEN;
        ap->hi[WRAP_BSPLICE]    = ap->hi[WRAP_GRAINSIZE]/2.0;
        ap->lo[WRAP_HBSPLICE]   = WRAP_MIN_SPLICELEN;
        ap->hi[WRAP_HBSPLICE]   = ap->hi[WRAP_GRAINSIZE]/2.0;
        ap->lo[WRAP_ESPLICE]    = WRAP_MIN_SPLICELEN;
        ap->hi[WRAP_ESPLICE]    = ap->hi[WRAP_GRAINSIZE]/2.0;
        ap->lo[WRAP_HESPLICE]   = WRAP_MIN_SPLICELEN;
        ap->hi[WRAP_HESPLICE]   = ap->hi[WRAP_GRAINSIZE]/2.0;
        ap->lo[WRAP_SRCHRANGE]  = 0.0;
        ap->hi[WRAP_SRCHRANGE]  = (duration * 2.0) * SECS_TO_MS;
        ap->lo[WRAP_SCATTER]    = 0.0;
        ap->hi[WRAP_SCATTER]    = 1.0;
        ap->lo[WRAP_OUTLEN]     = 0.0;
        ap->hi[WRAP_OUTLEN]     = BIG_TIME;
        ap->lo[WRAP_BUFXX]      = 0.0;
        ap->hi[WRAP_BUFXX]      = 64.0;
        break;
    case(MCHSTEREO):
        ap->lo[0] = 2;
        ap->hi[0] = 16;
        ap->lo[1] = 0.0;
        ap->hi[1] = 1.0;
        break;
    case(MTON):
        ap->lo[0] = 2;
        ap->hi[0] = 16;
        break;
    case(FLUTTER):
        ap->lo[0] = 0.0;
        ap->hi[0] = MAX_VIB_FRQ;
        ap->lo[1] = 0.0;
        ap->hi[1] = 16.0;
        ap->lo[2] = 0.0;
        ap->hi[2] = 10.0;
        break;
    case(ABFPAN):
        ap->lo[0] = 0.0;
        ap->hi[0] = 1.0;
        ap->lo[1] = -10000.0;
        ap->hi[1] = 10000.0;
        ap->lo[2] = 3;
        ap->hi[2] = 4;
        break;
    case(ABFPAN2):
        ap->lo[0] = 0.0;
        ap->hi[0] = 1.0;
        ap->lo[1] = -10000.0;
        ap->hi[1] = 10000.0;
        ap->lo[2] = 0;
        ap->hi[2] = 100;
        break;
    case(ABFPAN2P):
        ap->lo[0] = 0.0;
        ap->hi[0] = 1.0;
        ap->lo[1] = -10000.0;
        ap->hi[1] = 10000.0;
        ap->lo[2] = 0;
        ap->hi[2] = 100;
        ap->lo[3] = -180;
        ap->hi[3] = 180;
        break;
    case(CHANNELX):
        break;
    case(CHORDER):
        break;
    case(FMDCODE):
        ap->lo[0] = 1.0;
        ap->hi[0] = 12.0;
        break;
    case(CHXFORMAT):
        ap->lo[0] = 0.0;
        ap->hi[0] = 262144.0;
        break;
    case(CHXFORMATG):
    case(CHXFORMATM):
        break;
    case(INTERLX):
        ap->lo[0] = 0;
        ap->hi[0] = 8;
        break;
    case(COPYSFX):
        ap->lo[0] = 0;
        ap->hi[0] = 4;
        ap->lo[1] = -1;
        ap->hi[1] = 8;
        break;
    case(NJOIN):
        ap->lo[0] = -60;
        ap->hi[0] = 60;
        break;
    case(NJOINCH):
        break;
    case(NMIX):
        ap->lo[0] = 0.0;
        ap->hi[0] = 3600.0;
        break;
    case(RMSINFO):
        ap->lo[0] = 0;
        ap->hi[0] = duration;
        ap->lo[1] = 0;
        ap->hi[1] = duration;
        break;
    case(SFEXPROPS):
        break;
    case(SETHARES):
        chwidth = nyquist/(double)((wanted/2)-1);
        ap->lo[0] = 1;          //  winsize: semitones
        ap->hi[0] = 96;
        ap->lo[1] = 1;          //  peaking: ratio to median
        ap->hi[1] = 1000;
        ap->lo[2] = .0001;      //  ampfloor: ratio to max window in entire file
        ap->hi[2] = 1;
        ap->lo[3] = chwidth;    //  min frq to find
        ap->hi[3] = nyquist;
        ap->lo[4] = chwidth;    //  max frq to find
        ap->hi[4] = nyquist;
        ap->lo[5] = 0.0;        //  intunenuess of harmonics, semitones
        ap->hi[5] = 6.0;
        break;
    case(MCHSHRED):
        ap->lo[0] = 1.0;
        ap->hi[0] = (double)MSHR_MAX;
        ap->lo[1] = (double)((MSHR_SPLICELEN * 3)/(double)srate);
        ap->hi[1] = (duration/2.0)-FLTERR;
        ap->lo[2] = 0.0;
        ap->hi[2] = (double)MSHR_MAX_SCATTER;
        if(mode == 0) {
            ap->lo[3]   = 2;
            ap->hi[3]   = 16;
        }
        break;
    case(MCHZIG):
        ap->lo[MZIG_START]  = 0.0;
        ap->hi[MZIG_START]  = duration - (MZIG_SPLICELEN * MS_TO_SECS);
        ap->lo[MZIG_END]    = ((MZIG_SPLICELEN * 2) + MZIG_MIN_UNSPLICED) * MS_TO_SECS;
        ap->hi[MZIG_END]    = duration;
        ap->lo[MZIG_DUR]    = duration + FLTERR;
        ap->hi[MZIG_DUR]    = BIG_TIME;
        ap->lo[MZIG_MIN]    = ((MZIG_SPLICELEN * 2) + MZIG_MIN_UNSPLICED) * MS_TO_SECS;
        ap->hi[MZIG_MIN]    = duration - (2 * MZIG_SPLICELEN * MS_TO_SECS);
        ap->lo[MZIG_OCHANS] = 2;
        ap->hi[MZIG_OCHANS] = 16;
        ap->lo[MZIG_SPLEN]  = MMIN_ZIGSPLICE;
        ap->hi[MZIG_SPLEN]  = MMAX_ZIGSPLICE;
        if(mode==0) {
            ap->lo[MZIG_MAX] = ((MZIG_SPLICELEN * 2) + MZIG_MIN_UNSPLICED) * MS_TO_SECS;
            ap->hi[MZIG_MAX] = duration - (2 * MZIG_SPLICELEN * MS_TO_SECS);
            ap->lo[MZIG_RSEED] = 0.0;
            ap->hi[MZIG_RSEED] = MAXSHORT;
        }
        break;
    case(MCHITER):
        ap->lo[MITER_OCHANS]  = 2;
        ap->hi[MITER_OCHANS]  = 16;
        switch(mode) {
        case(0):
            ap->lo[MITER_DUR] = duration;
            ap->hi[MITER_DUR] = BIG_TIME;
            break;
        case(1):
            ap->lo[MITER_REPEATS]   = 1.0;
            ap->hi[MITER_REPEATS]   = BIG_VALUE;
            break;
        }
        ap->lo[MITER_DELAY] = FLTERR;
        ap->hi[MITER_DELAY] = ITER_MAX_DELAY;
        ap->lo[MITER_RANDOM]= 0.0;
        ap->hi[MITER_RANDOM]= 1.0;
        ap->lo[MITER_PSCAT] = 0.0;
        ap->hi[MITER_PSCAT] = ITER_MAXPSHIFT;   
        ap->lo[MITER_ASCAT] = 0.0;
        ap->hi[MITER_ASCAT] = 1.0;
        ap->lo[MITER_FADE]  = 0.0;
        ap->hi[MITER_FADE]  = 1.0;
        ap->lo[MITER_GAIN]  = 0.0;
        ap->hi[MITER_GAIN]  = 1.0;
        ap->lo[MITER_RSEED] = 0.0;
        ap->hi[MITER_RSEED] = MAXSHORT;
        break;
    case(SPECSPHINX):
        switch(mode) {
        case(0):
            ap->lo[0] = 0.0;
            ap->hi[0] = 1.0;
            ap->lo[1] = 0.0;
            ap->hi[1] = 1.0;
            break;
        case(1):
            ap->lo[0] = -1.0;
            ap->hi[0] = 1.0;
            ap->lo[1] = 0.01;
            ap->hi[1] = 100.0;
            break;
        case(2):
            ap->lo[0] = 0.0;
            ap->hi[0] = 1.0;
            ap->lo[1] = 0.01;
            ap->hi[1] = 100.0;
            ap->lo[2] = 0;
            ap->hi[2] = nyquist;
            break;
        }
        break;
    case(SPECMORPH):
        if(mode == 6) {
            ap->lo[NMPH_APKS] = 1;
            ap->hi[NMPH_APKS] = 16;
            ap->lo[NMPH_OCNT] = 1;
            ap->hi[NMPH_OCNT] = 64;
        } else {
            ap->lo[NMPH_STAG] = 0.0;
            ap->hi[NMPH_STAG] = (wlength-1) * frametime;
            ap->lo[NMPH_ASTT] = 0.0;
            ap->hi[NMPH_ASTT] = (wlength - 1) * frametime;
            ap->lo[NMPH_AEND] = frametime * 2;
            ap->hi[NMPH_AEND] = BIG_TIME;
            ap->lo[NMPH_AEXP] = 0.02;
            ap->hi[NMPH_AEXP] = 50.0;
            ap->lo[NMPH_APKS] = 1;
            ap->hi[NMPH_APKS] = 16;
            if(mode >= 4) {
                ap->lo[NMPH_RAND] = 0;
                ap->hi[NMPH_RAND] = 1;
            }
        }
        break;
    case(SPECMORPH2):
        ap->lo[NMPH_APKS] = 1;
        ap->hi[NMPH_APKS] = 16;
        if(mode > 0) {
            ap->lo[NMPH_ASTT] = 0.0;
            ap->hi[NMPH_ASTT] = (wlength - 1) * frametime;
            ap->lo[NMPH_AEND] = frametime * 2;
            ap->hi[NMPH_AEND] = BIG_TIME;
            ap->lo[NMPH_AEXP] = 0.02;
            ap->hi[NMPH_AEXP] = 50.0;
            ap->lo[NMPH_RAND] = 0;
            ap->hi[NMPH_RAND] = 1;
        }
        break;
    case(SUPERACCU):
        ap->lo[0] = 0.00001;
        ap->hi[0] = 0.9;
        ap->lo[1] = -MAXGLISRATE/frametime;
        ap->hi[1] = MAXGLISRATE/frametime;
        break;
    case(PARTITION):
        ap->lo[0] = 2;
        ap->hi[0] = 256;
        switch(mode) {
        case(0):
            ap->lo[1] = 1;
            ap->hi[1] = 1024;
            break;
        case(1):
            ap->lo[1] = 0.01;
            ap->hi[1] = duration;
            ap->lo[2] = 0.0;
            ap->hi[2] = 1.0;
            ap->lo[3] = 0.0;
            ap->hi[3] = 50.0;
            break;
        }
        break;
    case(SPECGRIDS):
        ap->lo[0] = 2.0;
        ap->hi[0] = (wanted/4) * 2;
        ap->lo[1] = 1.0;
        ap->hi[1] = (wanted)/4;
        break;
    case(GLISTEN):
        ap->lo[0]   = 2;
        ap->hi[0]   = (wanted/2) - 1;
        ap->lo[1]   = 1;
        ap->hi[1]   = 1024;
        ap->lo[2]   = 0;
        ap->hi[2]   = 12;
        ap->lo[3]   = 0;
        ap->hi[3]   = 1;
        ap->lo[4]   = 0;
        ap->hi[4]   = 1;
        break;
    case(TUNEVARY):
        ap->lo[0]   = 0.0;
        ap->hi[0]   = 1.0;
        ap->lo[1]   = 0.0;
        ap->hi[1]   = 1.0;
        ap->lo[2]   = 1.0;
        ap->hi[2]   = wanted/2;
        ap->lo[3]   = MINPITCH;
        ap->hi[3]   = nyquist;
        break;
    case(ISOLATE):
        switch(mode) {
        case(ISO_OVRLAP):
            ap->lo[ISO_SPL] = 0;
            ap->hi[ISO_SPL] = 500;
            ap->lo[ISO_DOV] = 0;
            ap->hi[ISO_DOV] = 20;
            break;
        case(ISO_THRESH):
            ap->lo[ISO_THRON]   = -60;
            ap->hi[ISO_THRON]   = 0;
            ap->lo[ISO_THROFF]  = -96;
            ap->hi[ISO_THROFF]  = 0;
            ap->lo[ISO_SPL]     = 0;
            ap->hi[ISO_SPL]     = 500;
            ap->lo[ISO_MIN]     = 20;
            ap->hi[ISO_MIN]     = 500;
            ap->lo[ISO_LEN]     = 0;
            ap->hi[ISO_LEN]     = 500;
            break;
        default:
            ap->lo[ISO_SPL] = 0;
            ap->hi[ISO_SPL] = 500;
            break;
        }
        break;
    case(REJOIN):
        ap->lo[0]   = 0;
        ap->hi[0]   = 1;
        break;
    case(PANORAMA):
        if(mode == 0) {
            ap->lo[PANO_LCNT] = 3.0;
            ap->hi[PANO_LCNT] = 16.0;
            ap->lo[PANO_LWID] = 190.0;
            ap->hi[PANO_LWID] = 360.0;
        }
        ap->lo[PANO_SPRD]     = 0.0;
        ap->hi[PANO_SPRD]     = 360.0;
        ap->lo[PANO_OFST]     = -180;
        ap->hi[PANO_OFST]     = 180;
        ap->lo[PANO_CNFG]     = 1;
        ap->hi[PANO_CNFG]     = 128;
        ap->lo[PANO_RAND]     = 0;
        ap->hi[PANO_RAND]     = 1;
        break;
    case(TREMOLO):
        ap->lo[TREMOLO_FRQ] = 0.0;
        ap->hi[TREMOLO_FRQ] = 500;
        ap->lo[TREMOLO_DEP] = 0.0;
        ap->hi[TREMOLO_DEP] = 1.0;
        ap->lo[TREMOLO_AMP] = 0.0;
        ap->hi[TREMOLO_AMP] = 1.0;
        ap->lo[TREMOLO_SQZ] = 1.0;
        ap->hi[TREMOLO_SQZ] = 100.0;
        break;
    case(ECHO):
        ap->lo[ECHO_DELAY] = duration;
        ap->hi[ECHO_DELAY] = 3600.0;
        ap->lo[ECHO_ATTEN] = 0;
        ap->hi[ECHO_ATTEN] = 1;
        ap->lo[ECHO_DUR]   = duration * 2;
        ap->hi[ECHO_DUR]   = 3600.0 + duration;
        ap->lo[ECHO_RAND]  = 0;
        ap->hi[ECHO_RAND]  = 1;
        ap->lo[ECHO_CUT]   = -96;
        ap->hi[ECHO_CUT]   = -6;
        break;
    case(PACKET):
        ap->lo[PAK_DUR] = 2.0;
        ap->hi[PAK_DUR] = (duration/2.0) * SECS_TO_MS;
        ap->lo[PAK_SQZ] = 0.0;
        ap->hi[PAK_SQZ] = 1000.0;
        ap->lo[PAK_CTR] = -1.0;
        ap->hi[PAK_CTR] = 1.0;
        break;
    case(SYNTHESIZER):
        ap->lo[SYNTHSRAT]   = 16000;
        ap->hi[SYNTHSRAT]   = 96000;
        ap->lo[SYNTH_DUR]   = 0.0;
        ap->hi[SYNTH_DUR]   = 32767.0;
        ap->lo[SYNTH_FRQ]   = .001;
        ap->hi[SYNTH_FRQ]   = 10000;
        if(mode == 1) {
            ap->lo[SYNTH_SQZ]   = 0.0;
            ap->hi[SYNTH_SQZ]   = 1000.0;
            ap->lo[SYNTH_CTR]   = -1.0;
            ap->hi[SYNTH_CTR]   = 1.0;
        } else if(mode == 2) {
            ap->lo[SYNTH_CHANS] = 1;
            ap->hi[SYNTH_CHANS] = 16.0;
            ap->lo[SYNTH_MAX]   = 1.0;
            ap->hi[SYNTH_MAX]   = 8.0;
            ap->lo[SYNTH_RATE]  = 0.004;
            ap->hi[SYNTH_RATE]  = 100;
            ap->lo[SYNTH_RISE]  = 0.0;
            ap->hi[SYNTH_RISE]  = 100;
            ap->lo[SYNTH_FALL]  = 0.0;
            ap->hi[SYNTH_FALL]  = 100;
            ap->lo[SYNTH_STDY]  = 0.0;
            ap->hi[SYNTH_STDY]  = 3600;
            ap->lo[SYNTH_SPLEN] = 2;
            ap->hi[SYNTH_SPLEN] = 50;
            ap->lo[SYNTH_NUM]   = 0;
            ap->hi[SYNTH_NUM]   = 1000;
            ap->lo[SYNTH_EFROM] = 0;
            ap->hi[SYNTH_EFROM] = 16.0;
            ap->lo[SYNTH_ETIME] = 0;
            ap->hi[SYNTH_ETIME] = 32767.0;
            ap->lo[SYNTH_CTO]   = 0;
            ap->hi[SYNTH_CTO]   = 10.0;
            ap->lo[SYNTH_CTIME] = 0;
            ap->hi[SYNTH_CTIME] = 32767.0;
            ap->lo[SYNTH_STYPE] = 0;
            ap->hi[SYNTH_STYPE] = 14;
            ap->lo[SYNTH_RSPEED] = -20;
            ap->hi[SYNTH_RSPEED] = 20;
        } else if(mode == 3) {
            ap->lo[SYNTH_ATK]   = 0;
            ap->hi[SYNTH_ATK]   = 16;
            ap->lo[SYNTH_EATK]  = 0.25;
            ap->hi[SYNTH_EATK]  = 4;
            ap->lo[SYNTH_DEC]   = 0;
            ap->hi[SYNTH_DEC]   = 64;
            ap->lo[SYNTH_EDEC]  = 0.25;
            ap->hi[SYNTH_EDEC]  = 4;
            ap->lo[SYNTH_ATOH]  = .1;
            ap->hi[SYNTH_ATOH]  = 1;
            ap->lo[SYNTH_GTOW]  = .1;
            ap->hi[SYNTH_GTOW]  = 1;
            ap->lo[SYNTH_RAND]  = 0;
            ap->hi[SYNTH_RAND]  = 1;
            ap->lo[SYNTH_FLEVEL]    = 0;
            ap->hi[SYNTH_FLEVEL]    = 256;
        }
        break;
    case(NEWTEX):
        ap->lo[NTEX_DUR]    = 0.0;
        ap->hi[NTEX_DUR]    = 32767.0;
        ap->lo[NTEX_CHANS]  = 2;
        ap->hi[NTEX_CHANS]  = 16.0;
        switch(mode) {
        case(0):
            ap->lo[NTEX_MAX] = 1.0;
            ap->hi[NTEX_MAX] = 8.0;
            break;
        case(1):
        case(2):
            ap->lo[NTEX_MAX] = 1.0;
            ap->hi[NTEX_MAX] = 8.0;
            break;
        }
        ap->lo[NTEX_RATE]    = 0.004;
        ap->hi[NTEX_RATE]    = 100;
        ap->lo[NTEX_SPLEN]   = 2;
        ap->hi[NTEX_SPLEN]   = 50;
        switch(mode) {
        case(0):
            ap->lo[NTEX_NUM] = 0;
            ap->hi[NTEX_NUM] = 32;
            break;
        case(1):
            ap->lo[NTEX_NUM] = 0;
            ap->hi[NTEX_NUM] = 132;
            ap->lo[NTEX_DEL] = 0;
            ap->hi[NTEX_DEL] = 32767;
            break;
        case(2):
            ap->lo[NTEX_NUM] = 0;
            ap->hi[NTEX_NUM] = 132;
            ap->lo[NTEX_LOC] = 0;
            ap->hi[NTEX_LOC] = 32767;
            ap->lo[NTEX_AMB] = 0;
            ap->hi[NTEX_AMB] = 32767;
            ap->lo[NTEX_GST] = 0;
            ap->hi[NTEX_GST] = 32767;
            break;
        }
        ap->lo[NTEX_EFROM]   = 0;
        ap->hi[NTEX_EFROM]   = 16.0;
        ap->lo[NTEX_ETIME]   = 0;
        ap->hi[NTEX_ETIME]   = 32767.0;
        ap->lo[NTEX_CTO]     = 0;
        ap->hi[NTEX_CTO]     = 16.0;
        ap->lo[NTEX_CTIME]   = 0;
        ap->hi[NTEX_CTIME]   = 32767.0;
        ap->lo[NTEX_STYPE]   = 0;
        ap->hi[NTEX_STYPE]   = 14;
        ap->lo[NTEX_RSPEED]  = -20;
        ap->hi[NTEX_RSPEED]  = 20;
        break;
    case(CERACU):
        ap->lo[CER_MINDUR] = 0.0;
        ap->hi[CER_MINDUR] = duration * 32;
        ap->lo[CER_OCHANS] = 1;
        ap->hi[CER_OCHANS] = 16;
        ap->lo[CER_CUTOFF] = 0.0;
        ap->hi[CER_CUTOFF] = 3600;
        ap->lo[CER_DELAY]   = 0.0;
        ap->hi[CER_DELAY]   = duration * 16;
        ap->lo[CER_DSTEP]   = -16;
        ap->hi[CER_DSTEP]   = 16;
        break;
    case(MADRID):
        ap->lo[MAD_DUR]   = duration;
        ap->hi[MAD_DUR]   = 32767;
        ap->lo[MAD_CHANS] = 2;
        ap->hi[MAD_CHANS] = 16;
        ap->lo[MAD_STRMS] = 2;
        ap->hi[MAD_STRMS] = 64;
        ap->lo[MAD_DELF]  = 0;
        ap->hi[MAD_DELF]  = 1000;
        ap->lo[MAD_STEP]  = 0;
        ap->hi[MAD_STEP]  = 60;
        ap->lo[MAD_RAND]  = 0;
        ap->hi[MAD_RAND]  = 1;
        ap->lo[MAD_SEED]  = 0;
        ap->hi[MAD_SEED]  = 256;
        break;
    case(SHIFTER):
        ap->lo[SHF_CYCDUR]  = 0.01;
        ap->hi[SHF_CYCDUR]  = 3600.0;
        ap->lo[SHF_OUTDUR]  = 0.0;
        ap->hi[SHF_OUTDUR]  = 32767.0;
        ap->lo[SHF_OCHANS]  = 1;
        ap->hi[SHF_OCHANS]  = 16;
        ap->lo[SHF_SUBDIV]  = 6;
        ap->hi[SHF_SUBDIV]  = 64;
        ap->lo[SHF_LINGER]  = 0.0;
        ap->hi[SHF_LINGER]  = 32767;
        ap->lo[SHF_TRNSIT]  = 0.0;
        ap->hi[SHF_TRNSIT]  = 32767;
        ap->lo[SHF_LBOOST]  = 0.0;
        ap->hi[SHF_LBOOST]  = 10.0;
        break;
    case(SUBTRACT):
        ap->lo[0] = 1;
        ap->hi[0] = channels;
        break;
    case(SPEKLINE):
        if(mode == 0) {
            ap->lo[SPEKPOINTS]  = 2;
            ap->hi[SPEKPOINTS]  = 32768;  //RWD 2025 was 16380
            ap->lo[SPEKHARMS]   = 0;
            ap->hi[SPEKHARMS]   = 64;
            ap->lo[SPEKBRITE]   = -96;
            ap->hi[SPEKBRITE]   = 0;
            ap->lo[SPEKMAX]     = 0.001;
            ap->hi[SPEKMAX]     = 1;
        }
        ap->lo[SPEKSRATE]   = 44100;
        ap->hi[SPEKSRATE]   = 96000;
        ap->lo[SPEKDUR]     = 0;
        ap->hi[SPEKDUR]     = 32767;
        ap->lo[SPEKDATLO]   = 0;
        ap->hi[SPEKDATLO]   = 48000;
        ap->lo[SPEKDATHI]   = 0;
        ap->hi[SPEKDATHI]   = 48000;
        ap->lo[SPEKSPKLO]   = 0;
        ap->hi[SPEKSPKLO]   = 48000;
        ap->lo[SPEKSPKHI]   = 0;
        ap->hi[SPEKSPKHI]   = 48000;
        ap->lo[SPEKWARP]    = 0.1;
        ap->hi[SPEKWARP]    = 10;
        ap->lo[SPEKAWARP]   = 1.0;
        ap->hi[SPEKAWARP]   = 100;
        break;
    case(FRACTURE):
        if(mode == 0)
            ap->lo[FRAC_CHANS] = 2;
        else
            ap->lo[FRAC_CHANS] = 4;
        ap->hi[FRAC_CHANS] = 16;
        ap->lo[FRAC_STRMS] = 4;
        ap->hi[FRAC_STRMS] = 512;
        ap->lo[FRAC_PULSE] = .05;
        ap->hi[FRAC_PULSE] = 8;
        ap->lo[FRAC_DEPTH] = 0;
        ap->hi[FRAC_DEPTH] = 8;
        ap->lo[FRAC_STACK] = 0;
        ap->hi[FRAC_STACK] = 12;
        ap->lo[FRAC_INRND] = 0;
        ap->hi[FRAC_INRND] = 1;
        ap->lo[FRAC_OUTRND] = 0;
        ap->hi[FRAC_OUTRND] = 1;
        ap->lo[FRAC_SCAT] = 0;
        ap->hi[FRAC_SCAT] = 1;
        ap->lo[FRAC_LEVRND] = 0;
        ap->hi[FRAC_LEVRND] = 1;
        ap->lo[FRAC_ENVRND] = 0;
        ap->hi[FRAC_ENVRND] = 32767;
        ap->lo[FRAC_STKRND] = 0;
        ap->hi[FRAC_STKRND] = 1;
        ap->lo[FRAC_PCHRND] = 0;
        ap->hi[FRAC_PCHRND] = 1200;
        ap->lo[FRAC_SEED] = 0;
        ap->hi[FRAC_SEED] = 32767;
        ap->lo[FRAC_MIN] = 0;
        ap->hi[FRAC_MIN] = 1;
        ap->lo[FRAC_MAX] = 0;
        ap->hi[FRAC_MAX] = 16;
        if(mode > 0) {
            ap->lo[FRAC_CENTRE]  = 0;
            ap->hi[FRAC_CENTRE]  = 8;
            ap->lo[FRAC_FRONT]   = -10;
            ap->hi[FRAC_FRONT]   = 2;
            ap->lo[FRAC_MDEPTH]  = 0;
            ap->hi[FRAC_MDEPTH]  = 1;
            ap->lo[FRAC_ROLLOFF] = 0.0;
            ap->hi[FRAC_ROLLOFF] = 1.0;
            ap->lo[FRAC_ATTEN]   = 1.0;
            ap->hi[FRAC_ATTEN]   = 10.0;
            ap->lo[FRAC_ZPOINT]  = 0.0;
            ap->hi[FRAC_ZPOINT]  = 1.0;
            ap->lo[FRAC_CONTRACT] = 0.0;
            ap->hi[FRAC_CONTRACT] = 1.0;
            ap->lo[FRAC_FPOINT]   = 0.0;
            ap->hi[FRAC_FPOINT]   = 1.0;
            ap->lo[FRAC_FFACTOR]  = 0.0;
            ap->hi[FRAC_FFACTOR]  = 1.0;
            ap->lo[FRAC_FFREQ]    = 10.0;
            ap->hi[FRAC_FFREQ]    = nyquist/2.0;
            ap->lo[FRAC_UP]       = 0.0;
            ap->hi[FRAC_UP]       = 1.0;
            ap->lo[FRAC_DN]       = 0.0;
            ap->hi[FRAC_DN]       = 1.0;
            ap->lo[FRAC_GAIN]     = 0.0;
            ap->hi[FRAC_GAIN]     = 1.0;
        }
        break;
    case(TAN_ONE):
        ap->lo[TAN_DUR]   = duration * 2.0;
        ap->hi[TAN_DUR]   = 32767;
        ap->lo[TAN_STEPS] = 2;
        ap->hi[TAN_STEPS] = 32767;
        if(mode==0) {
            ap->lo[TAN_MANG] = 90;
            ap->hi[TAN_MANG] = 135;
            ap->lo[TAN_SLOW] = 0;
            ap->hi[TAN_SLOW] = 1;
        } else {
            ap->lo[TAN_SKEW] = 0;
            ap->hi[TAN_SKEW] = 1;
        }
        ap->lo[TAN_DEC]    = 0;
        ap->hi[TAN_DEC]    = 1;
        ap->lo[TAN_FOCUS]  = 1;
        ap->hi[TAN_FOCUS]  = 8;
        ap->lo[TAN_JITTER] = 0;
        ap->hi[TAN_JITTER] = 1;
        break;
    case(TAN_TWO):
        ap->lo[TAN_DUR]   = duration * 2.0;
        ap->hi[TAN_DUR]   = 32767;
        ap->lo[TAN_STEPS] = 2;
        ap->hi[TAN_STEPS] = 32767;
        if(mode==0) {
            ap->lo[TAN_MANG] = 90;
            ap->hi[TAN_MANG] = 135;
            ap->lo[TAN_SLOW] = 0;
            ap->hi[TAN_SLOW] = 1;
        } else {
            ap->lo[TAN_SKEW] = 0;
            ap->hi[TAN_SKEW] = 1;
        }
        ap->lo[TAN_DEC]    = 0;
        ap->hi[TAN_DEC]    = 1;
        ap->lo[TAN_FBAL]   = 0;
        ap->hi[TAN_FBAL]   = 1;
        ap->lo[TAN_FOCUS]  = 1;
        ap->hi[TAN_FOCUS]  = 8;
        ap->lo[TAN_JITTER] = 0;
        ap->hi[TAN_JITTER] = 1;
        break;
    case(TAN_SEQ):
    case(TAN_LIST):
        ap->lo[TAN_DUR]       = duration * 2.0;
        ap->hi[TAN_DUR]       = 32767;
        if(mode==0) {
            ap->lo[TAN_MANG]  = 90;
            ap->hi[TAN_MANG]  = 135;
            ap->lo[TAN_SLOW]  = 0;
            ap->hi[TAN_SLOW]  = 1;
        } else {
            ap->lo[TAN_SKEW]  = 0;
            ap->hi[TAN_SKEW]  = 1;
        }
        ap->lo[TAN_DEC]       = 0;
        ap->hi[TAN_DEC]       = 1;
        ap->lo[TAN_FOCUS]     = 1;
        ap->hi[TAN_FOCUS]     = 8;
        ap->lo[TAN_JITTER]    = 0;
        ap->hi[TAN_JITTER]    = 1;
        break;
    case(SPECTWIN):
        ap->lo[0] = 0;
        ap->hi[0] = 1;
        ap->lo[1] = 0;
        ap->hi[1] = 1;
        ap->lo[2] = 0;
        ap->hi[2] = 16;
        ap->lo[3] = 0;
        ap->hi[3] = 48;
        ap->lo[4] = 0;
        ap->hi[4] = 1;
    break;
    case(TRANSIT):
    case(TRANSITF):
    case(TRANSITD):
    case(TRANSITFD):
    case(TRANSITS):
    case(TRANSITL):
        if(EVEN(mode)) {
            ap->lo[TRAN_FOCUS]    = 1;
            ap->hi[TRAN_FOCUS]    = 8;
        } else {
            ap->lo[TRAN_FOCUS]    = 1.5;
            ap->hi[TRAN_FOCUS]    = 8.5;
        }
        ap->lo[TRAN_DUR]      = duration * 2.0;
        ap->hi[TRAN_DUR]      = 32767;
        if(process < TRANSITS) {
            ap->lo[TRAN_STEPS]    = 2;
            ap->hi[TRAN_STEPS]    = 32767;
        }
        switch(mode) {
        case(GLANCING):
            ap->lo[TRAN_MAXA]   = 22.5;
            ap->hi[TRAN_MAXA]   = 90;
            break;
        case(EDGEWISE):
            ap->lo[TRAN_MAXA]   = 22.5;
            ap->hi[TRAN_MAXA]   = 90;
            break;
        case(CROSSING):
            ap->lo[TRAN_MAXA]   = 45.0;
            ap->hi[TRAN_MAXA]   = 90;
            break;
        case(CLOSE):
            ap->lo[TRAN_MAXA]   = 67.5;
            ap->hi[TRAN_MAXA]   = 90;
            break;
        case(CENTRAL):
            ap->lo[TRAN_MAXA]   = 1;
            ap->hi[TRAN_MAXA]   = 1000;
            break;
        }
        ap->lo[TRAN_DEC]      = 0;
        ap->hi[TRAN_DEC]      = 1;
        if(process == TRANSITF || process == TRANSITFD) {
            ap->lo[TRAN_FBAL] = 0;
            ap->hi[TRAN_FBAL] = 1;
        }
        if(process < TRANSITS) {
            ap->lo[TRAN_THRESH]   = 0;
            ap->hi[TRAN_THRESH]   = 1;
            ap->lo[TRAN_DECLIM]   = 0;
            ap->hi[TRAN_DECLIM]   = 1;
            ap->lo[TRAN_MINLEV]   = 0;
            ap->hi[TRAN_MINLEV]   = 1;
            ap->lo[TRAN_MAXDUR]   = ap->lo[TRAN_DUR];
            ap->hi[TRAN_MAXDUR]   = ap->hi[TRAN_DUR];
        }
        break;
    case(CANTOR):
        switch(mode) {
        case(0):
            ap->lo[CA_HOLEN]    = 0.0;
            ap->hi[CA_HOLEN]    = 99.0;
            ap->lo[CA_HOLEDIG]  = 0.001;
            ap->hi[CA_HOLEDIG]  = 1.0;
            ap->lo[CA_TRIGLEV]  = 0.001;
            ap->hi[CA_TRIGLEV]  = 1.0;
            ap->lo[CA_SPLEN]    = 3;
            ap->hi[CA_SPLEN]    = 50;
            break;
        case(1):
            ap->lo[CA_HOLEN]    = 0.0;
            ap->hi[CA_HOLEN]    = duration/3.0;
            ap->lo[CA_HOLEDIG]  = 0.001;
            ap->hi[CA_HOLEDIG]  = 1.0;
            ap->lo[CA_TRIGLEV]  = 0.001;
            ap->hi[CA_TRIGLEV]  = 1.0;
            ap->lo[CA_SPLEN]    = 3;
            ap->hi[CA_SPLEN]    = 50;
            break;
        case(2):
            ap->lo[CA_HOLEN]    = 0.0;
            ap->hi[CA_HOLEN]    = 0.99;
            ap->lo[CA_HOLEDIG]  = 2;
            ap->hi[CA_HOLEDIG]  = 256;
            ap->lo[CA_WOBDEC]   = 0.01;
            ap->hi[CA_WOBDEC]   = 1;
            ap->lo[CA_WOBBLES]  = 1;
            ap->hi[CA_WOBBLES]  = 100;
            break;
        }
        ap->lo[CA_MAXDUR]   = duration * 2;
        ap->hi[CA_MAXDUR]   = 32767;
        break;
    case(SHRINK):
        if(mode == SHRM_TIMED) {
            ap->lo[SHR_TIME] = 0;
            ap->hi[SHR_TIME] = duration;
        }
        ap->lo[SHR_INK]     = 0;
        ap->hi[SHR_INK]     = 1;
        if(mode >= SHRM_FINDMX) {
            ap->lo[SHR_WSIZE] = 1;
            ap->hi[SHR_WSIZE] = 128;
            ap->lo[SHR_AFTER] = 0.0;
            ap->hi[SHR_AFTER] = duration;
        } else {
            ap->lo[SHR_GAP]   = duration;
            ap->hi[SHR_GAP]   = 60;
            ap->lo[SHR_DUR]   = duration * 2.0;
            ap->hi[SHR_DUR]   = 32767;
        }
        ap->lo[SHR_CNTRCT] = 0;
        ap->hi[SHR_CNTRCT] = 1;
        ap->lo[SHR_SPLEN]  = 2;
        ap->hi[SHR_SPLEN]  = 50.0;
        ap->lo[SHR_SMALL]  = 0;
        ap->hi[SHR_SMALL]  = insams;
        ap->lo[SHR_MIN]    = 0;
        ap->hi[SHR_MIN]    = 32767;
        ap->lo[SHR_RAND]   = 0;
        ap->hi[SHR_RAND]   = 1;
        if(mode >= SHRM_FINDMX) {
            ap->lo[SHR_GATE] = 0;
            ap->hi[SHR_GATE] = 1;
            ap->lo[SHR_LEN]  = 0;
            ap->hi[SHR_LEN]  = duration;
        }
        if(mode == SHRM_FINDMX) {
            ap->lo[SHR_SKEW] = 0;
            ap->hi[SHR_SKEW] = 1;
        }
        break;
    case(NEWDELAY):
        if(mode == 0) {
            ap->lo[DELAY_DELAY] = -76;
            ap->hi[DELAY_DELAY] = 130;
            ap->lo[DELAY_MIX]       = 0.001;
            ap->hi[DELAY_MIX]       = 1.0;
            ap->lo[DELAY_FEEDBACK]  = -1.0;
            ap->hi[DELAY_FEEDBACK]  = 1.0;
        } else {
            hztomidi(&(ap->lo[0]),5.0);
            hztomidi(&(ap->hi[0]),6000);
            ap->lo[1]   = 0.016;
            ap->hi[1]   = duration;
            ap->lo[2]   = 1.1;
            ap->hi[2]   = 128;
            ap->lo[3]   = 0.0;
            ap->hi[3]   = 1.0;
            ap->lo[4]   = 1;
            ap->hi[4]   = 16;
            ap->lo[5]   = 0.1;
            ap->hi[5]   = 0.9;
        }
        break;
    case(ITERLINE):
    case(ITERLINEF):
        ap->lo[ITER_DUR]    = duration;
        ap->hi[ITER_DUR]    = BIG_TIME;
        ap->lo[ITER_DELAY]  = FLTERR;
        ap->hi[ITER_DELAY]  = ITER_MAX_DELAY;
        ap->lo[ITER_RANDOM] = 0.0;
        ap->hi[ITER_RANDOM] = 1.0;
        ap->lo[ITER_PSCAT]  = 0.0;
        ap->hi[ITER_PSCAT]  = ITER_MAXPSHIFT;
        ap->lo[ITER_ASCAT]  = 0.0;
        ap->hi[ITER_ASCAT]  = 1.0;
        ap->lo[ITER_GAIN]   = 0.0;
        ap->hi[ITER_GAIN]   = 1.0;
        ap->lo[ITER_RSEED]  = 0.0;
        ap->hi[ITER_RSEED]  = MAXSHORT;
        break;
    case(SPECRAND):
        ap->lo[0]           = frametime * 2;
        ap->hi[0]           = duration;
        ap->lo[1]           = 1;
        ap->hi[1]           = wlength/2;
        break;
    case(SPECSQZ):
        ap->lo[0]           = 0;
        ap->hi[0]           = nyquist;
        ap->lo[1]           = 2.0/(double)wlength;
        ap->hi[1]           = 1;
        break;
    case(FILTRAGE):
        ap->lo[FILTR_DUR]   = 0;
        ap->hi[FILTR_DUR]   = 32767;    // duration
        ap->lo[FILTR_CNT]   = 2;
        ap->hi[FILTR_CNT]   = 400;      // number of filters
        ap->lo[FILTR_MMIN]  = 0;
        ap->hi[FILTR_MMIN]  = 127;      // min MIDI value
        ap->lo[FILTR_MMAX]  = 0;
        ap->hi[FILTR_MMAX]  = 127;      // max MIDI value
        ap->lo[FILTR_DIS]   = 0.01;
        ap->hi[FILTR_DIS]   = 100;      // filter distribution over range
        ap->lo[FILTR_RND]   = 0;
        ap->hi[FILTR_RND]   = 1;        // filter randomisation
        ap->lo[FILTR_AMIN]  = 0;
        ap->hi[FILTR_AMIN]  = 1;        // minumum filter amplitude
        ap->lo[FILTR_ARND]  = 0;
        ap->hi[FILTR_ARND]  = 1;        // filter amplitude randomisation
        ap->lo[FILTR_ADIS]  = -1;
        ap->hi[FILTR_ADIS]  = 1;        // filter amp distrib (ascending,descending,random)
        if(mode == 1) {
            ap->lo[FILTR_STEP]  = .01;
            ap->hi[FILTR_STEP]  = 3600;     // time step between defined filter states
            ap->lo[FILTR_SRND]  = 0;
            ap->hi[FILTR_SRND]  = 1;        // randomisation of timestep
        }
        ap->lo[FILTR_SEED]  = 0;
        ap->hi[FILTR_SEED]  = 512;      // randomn seed value
        break;
    case(SELFSIM):
        ap->lo[0]   = 0;
        ap->hi[0]   = wlength - 1;      // self-similarity index
        break;
    case(ITERFOF):
        if(mode <2) {
            ap->lo[ITF_DEL]     = -24;
            ap->hi[ITF_DEL]     = 12;
        } else {
            ap->lo[ITF_DEL]     = 24;
            ap->hi[ITF_DEL]     = 96;
        }
        ap->lo[ITF_DUR]     = duration;
        ap->hi[ITF_DUR]     = BIG_TIME;
        ap->lo[ITF_PRND]    = 0.0;
        ap->hi[ITF_PRND]    = 2.0;
        ap->lo[ITF_AMPC]    = 0.0;
        ap->hi[ITF_AMPC]    = 1.0;
        ap->lo[ITF_TRIM]    = 0.0;
        ap->hi[ITF_TRIM]    = duration;
        ap->lo[ITF_TRBY]    = 0.0;
        ap->hi[ITF_TRBY]    = duration;
        ap->lo[ITF_SLOP]    = 1.0;
        ap->hi[ITF_SLOP]    = 4.0;
        ap->lo[ITF_RAND]    = 0.0;
        ap->hi[ITF_RAND]    = 1.0;
        ap->lo[ITF_VMIN]    = 0.0;
        ap->hi[ITF_VMIN]    = 20.0;
        ap->lo[ITF_VMAX]    = 0.0;
        ap->hi[ITF_VMAX]    = 20.0;
        ap->lo[ITF_DMIN]    = 0.0;
        ap->hi[ITF_DMIN]    = 2.0;
        ap->lo[ITF_DMAX]    = 0.0;
        ap->hi[ITF_DMAX]    = 2.0;
        if(EVEN(mode)) {
            ap->lo[ITF_SEED1]   = 0.0;
            ap->hi[ITF_SEED1]   = MAXSHORT;
        } else {
            ap->lo[ITF_GMIN]    = 0.0;
            ap->hi[ITF_GMIN]    = 1.0;
            ap->lo[ITF_GMAX]    = 0.0;
            ap->hi[ITF_GMAX]    = 1.0;
            ap->lo[ITF_UFAD]    = 0.0;
            ap->hi[ITF_UFAD]    = 10;
            ap->lo[ITF_FADE]    = 0.0;
            ap->hi[ITF_FADE]    = 10;
            ap->lo[ITF_GAPP]    = 0.0;
            ap->hi[ITF_GAPP]    = 1.0;
            ap->lo[ITF_PORT]    = -1;
            ap->hi[ITF_PORT]    = 2.0;
            ap->lo[ITF_PINT]    = 0.0;
            ap->hi[ITF_PINT]    = 2.0;
            ap->lo[ITF_SEED2]   = 0.0;
            ap->hi[ITF_SEED2]   = MAXSHORT;
        }
        break;
    case(PULSER):
    case(PULSER2):
    case(PULSER3):
        ap->lo[PLS_DUR]      = .02;
        ap->hi[PLS_DUR]      = 32767;
        if(process == PULSER3 || mode == 0) {
            ap->lo[PLS_PITCH]    = 24;
            ap->hi[PLS_PITCH]    = 96;
        }
        ap->lo[PLS_MINRISE]  = 0.002;
        ap->hi[PLS_MINRISE]  = 0.2;
        ap->lo[PLS_MAXRISE]  = 0.002;
        ap->hi[PLS_MAXRISE]  = 0.2;
        ap->lo[PLS_MINSUS]   = 0.0;
        ap->lo[PLS_MAXSUS]   = 0.0;
        if(process != PULSER3 && mode == 2) {
            ap->hi[PLS_MAXSUS]   = 1.0;
            ap->hi[PLS_MINSUS]   = 1.0;
        } else {
            ap->hi[PLS_MAXSUS]   = 0.2;
            ap->hi[PLS_MINSUS]   = 0.2;
        }
        ap->lo[PLS_MINDECAY] = 0.02;
        ap->hi[PLS_MINDECAY] = 2;
        ap->lo[PLS_MAXDECAY] = 0.02;
        ap->hi[PLS_MAXDECAY] = 2;
        ap->lo[PLS_SPEED]   = 0.05;
        ap->hi[PLS_SPEED]   = 1;
        ap->lo[PLS_SCAT]    = 0.0;
        ap->hi[PLS_SCAT]    = 1.0;
        ap->lo[PLS_EXP] = 0.25;
        ap->hi[PLS_EXP] = 4;
        ap->lo[PLS_EXP2]    = 0.25;
        ap->hi[PLS_EXP2]    = 4;
        ap->lo[PLS_PSCAT]   = 0;
        ap->hi[PLS_PSCAT]   = 1;
        ap->lo[PLS_ASCAT]   = 0;
        ap->hi[PLS_ASCAT]   = 1;
        ap->lo[PLS_OCT]     = 0;
        ap->hi[PLS_OCT]     = 1;
        ap->lo[PLS_BEND]    = 0;
        ap->hi[PLS_BEND]    = 24;
        ap->lo[PLS_SEED]    = 0.0;
        ap->hi[PLS_SEED]    = MAXSHORT;
        if(process == PULSER3) {
            ap->lo[PLS_SRATE]   = 16000;
            ap->hi[PLS_SRATE]   = 96000;
            ap->lo[PLS_CNT] = 0;
            ap->hi[PLS_CNT] = 64;
        }
        if(process != PULSER3 && mode == 2) {
            ap->lo[PLS_WIDTH] = 0;
            ap->hi[PLS_WIDTH] = 1;
        }
        break;
    case(CHIRIKOV):
        ap->lo[CHIR_DUR]    = 0.0;
        ap->hi[CHIR_DUR]    = 32767.0;
        ap->lo[CHIR_FRQ]    = .001;
        ap->hi[CHIR_FRQ]    = 10000;
        ap->lo[CHIR_DAMP]   = 0;
        ap->hi[CHIR_DAMP]   = 2.0 * TWOPI;
        if(mode < 2) {
            ap->lo[CHIR_SRATE]  = 16000;
            ap->hi[CHIR_SRATE]  = 96000;
            ap->lo[CHIR_SPLEN]  = 1;
            ap->hi[CHIR_SPLEN]  = 50;
        } else {
            ap->lo[CHIR_PMIN]   = 24;
            ap->hi[CHIR_PMIN]   = 96;
            ap->lo[CHIR_PMAX]   = 24;
            ap->hi[CHIR_PMAX]   = 96;
            ap->lo[CHIR_STEP]   = 0.02;
            ap->hi[CHIR_STEP]   = 2;
            ap->lo[CHIR_RAND]   = 0.0;
            ap->hi[CHIR_RAND]   = 1.0;
        }
        break;
    case(MULTIOSC):
        ap->lo[MOSC_DUR]    = 0.0;
        ap->hi[MOSC_DUR]    = 32767.0;
        ap->lo[MOSC_FRQ1]   = .001;
        ap->hi[MOSC_FRQ1]   = 10000;
        ap->lo[MOSC_FRQ2]   = .001;
        ap->hi[MOSC_FRQ2]   = 10000;
        ap->lo[MOSC_AMP2]   = 0;
        ap->hi[MOSC_AMP2]   = 1.0;
        if(mode >= 1) {
            ap->lo[MOSC_FRQ3]   = .001;
            ap->hi[MOSC_FRQ3]   = 10000;
            ap->lo[MOSC_AMP3]   = 0;
            ap->hi[MOSC_AMP3]   = 1.0;
        }
        if(mode == 2) {
            ap->lo[MOSC_FRQ4]   = .001;
            ap->hi[MOSC_FRQ4]   = 10000;
            ap->lo[MOSC_AMP4]   = 0;
            ap->hi[MOSC_AMP4]   = 1.0;
        }
        ap->lo[MOSC_SRATE]  = 16000;
        ap->hi[MOSC_SRATE]  = 96000;
        ap->lo[MOSC_SPLEN]  = 1;
        ap->hi[MOSC_SPLEN]  = 50;
        break;
    case(SYNFILT):
        ap->lo[SYNFLT_SRATE]   = 44100.0;
        ap->hi[SYNFLT_SRATE]   = 96000.0;
        ap->lo[SYNFLT_CHANS]   = 1;
        ap->hi[SYNFLT_CHANS]   = 2;
        ap->lo[SYNFLT_Q]       = MINQ;
        ap->hi[SYNFLT_Q]       = MAXQ;
        ap->lo[SYNFLT_HARMCNT] = 1.0;
        ap->hi[SYNFLT_HARMCNT] = FLT_MAXHARMS;
        ap->lo[SYNFLT_ROLLOFF] = MIN_DB_ON_16_BIT;
        ap->hi[SYNFLT_ROLLOFF] = 0.0;
        ap->lo[SYNFLT_SEED]    = 0;
        ap->hi[SYNFLT_SEED]    = 32767;
        break;
    case(STRANDS):
        ap->lo[STRAND_DUR]   = 0.0;
        ap->hi[STRAND_DUR]   = 32767.0;
        ap->lo[STRAND_BANDS] = 1;
        ap->hi[STRAND_BANDS] = 16;
        if(mode != 2) {
            ap->lo[STRAND_THRDS] = 2;   
            ap->hi[STRAND_THRDS] = 100;
        }
        ap->lo[STRAND_TSTEP] = 1;
        ap->hi[STRAND_TSTEP] = 500;
        ap->lo[STRAND_BOT]   = 0;
        ap->hi[STRAND_BOT]   = 127;
        ap->lo[STRAND_TOP]   = 0;
        ap->hi[STRAND_TOP]   = 127;
        ap->lo[STRAND_TWIST] = 0;
        ap->hi[STRAND_TWIST] = 10;
        ap->lo[STRAND_RAND]  = 0;
        ap->hi[STRAND_RAND]  = 1;
        ap->lo[STRAND_SCAT]  = 0;
        ap->hi[STRAND_SCAT]  = 1;
        ap->lo[STRAND_VAMP]  = 0;
        ap->hi[STRAND_VAMP]  = 1;
        ap->lo[STRAND_VMIN]  = 0;
        ap->hi[STRAND_VMIN]  = 4;
        ap->lo[STRAND_VMAX]  = 0;
        ap->hi[STRAND_VMAX]  = 4;
        ap->lo[STRAND_TURB]  = 0;
        ap->hi[STRAND_TURB]  = 2;
        ap->lo[STRAND_SEED]  = 0;
        ap->hi[STRAND_SEED]  = 256;
        ap->lo[STRAND_GAP]   = 0;
        ap->hi[STRAND_GAP]   = 12;
        ap->lo[STRAND_MINB]  = 1;
        ap->hi[STRAND_MINB]  = 24;
        ap->lo[STRAND_3D]    = -1;
        ap->hi[STRAND_3D]    = 1;
        break;
    case(REFOCUS):
        ap->lo[REFOC_DUR]    = 0.0;
        ap->hi[REFOC_DUR]    = 32767.0;
        ap->lo[REFOC_BANDS]  = 2;
        ap->hi[REFOC_BANDS]  = 900;
        ap->lo[REFOC_RATIO]  = 1;   
        ap->hi[REFOC_RATIO]  = 64;
        ap->lo[REFOC_TSTEP]  = .01;
        ap->hi[REFOC_TSTEP]  = 3600;
        ap->lo[REFOC_RAND]   = 0;
        ap->hi[REFOC_RAND]   = 1;
        ap->lo[REFOC_OFFSET] = 0;
        ap->hi[REFOC_OFFSET] = 32766;
        ap->lo[REFOC_END]    = 0;
        ap->hi[REFOC_END]    = 32766;
        ap->lo[REFOC_XCPT]   = -1;
        ap->hi[REFOC_XCPT]   = 1;
        ap->lo[REFOC_SEED]   = 0;
        ap->hi[REFOC_SEED]   = 256;
        break;
    case(CHANPHASE):
        ap->lo[0]   = 1;
        ap->hi[0]   = 16;
        break;
    case(SILEND):
        if(mode == 0) {
            ap->lo[0]   = FLTERR;
            ap->hi[0]   = 32767.0;
        } else {
            ap->lo[0]   = duration + FLTERR;
            ap->hi[0]   = duration +32767.0;
        }
        break;
    case(SPECULATE):
        ap->lo[0]   = 0.0;
        ap->hi[0]   = nyquist;
        ap->lo[1]   = 0.0;
        ap->hi[1]   = nyquist;
        break;
    case(SPECTUNE):
        ap->lo[ST_MATCH] = 1;
        ap->hi[ST_MATCH] = 8;
        ap->lo[ST_LOPCH] = 4;
        ap->hi[ST_LOPCH] = 127;
        ap->lo[ST_HIPCH] = 4;
        ap->hi[ST_HIPCH] = 127;
        ap->lo[ST_STIME] = 0;
        ap->hi[ST_STIME] = duration;
        ap->lo[ST_ETIME] = 0;
        ap->hi[ST_ETIME] = duration;
        ap->lo[ST_INTUN] = 0;
        ap->hi[ST_INTUN] = 6;
        ap->lo[ST_WNDWS] = 0;
        ap->hi[ST_WNDWS] = wlength;
        ap->lo[ST_NOISE] = 0.0;
        ap->hi[ST_NOISE] = SIGNOIS_MAX;
        break;
    case(REPAIR):
        ap->lo[0]    = 2;
        ap->hi[0]    = 16;
        break;
    case(DISTSHIFT):
        ap->lo[0]   = 1;
        ap->hi[0]   = 32767;
        if(mode==0) {
            ap->lo[1]   = 1;
            ap->hi[1]   = 32767;
        }
        break;
    case(QUIRK):
        ap->lo[0]   = 0.01;
        ap->hi[0]   = 100;
        break;
    case(ROTOR):
        ap->lo[ROT_CNT] = 3;
        ap->hi[ROT_CNT] = 127;
        ap->lo[ROT_PMIN] = 0;
        ap->hi[ROT_PMIN] = 127;
        ap->lo[ROT_PMAX] = 0;
        ap->hi[ROT_PMAX] = 127;
        ap->lo[ROT_NSTEP]   = 0;
        ap->hi[ROT_NSTEP]   = 4;
        ap->lo[ROT_PCYC]    = 4;
        ap->hi[ROT_PCYC]    = 256;
        ap->lo[ROT_TCYC]    = 4;
        ap->hi[ROT_TCYC]    = 256;
        ap->lo[ROT_PHAS]    = 0;
        ap->hi[ROT_PHAS]    = 1;
        ap->lo[ROT_DUR] = 1;
        ap->hi[ROT_DUR] = 32767;
        if(mode == 0) {
            ap->lo[ROT_GSTEP]   = .1;
            ap->hi[ROT_GSTEP]   = 60;
        }
        ap->lo[ROT_DOVE]    = 0;
        ap->hi[ROT_DOVE]    = 5;
        break;
    case(DISTCUT):
        ap->lo[DCUT_CNT]    = 1;
        ap->hi[DCUT_CNT]    = 1000;
        ap->lo[DCUT_STP]    = 1;
        ap->hi[DCUT_STP]    = 1000;
        ap->lo[DCUT_EXP]    = 0.02;
        ap->hi[DCUT_EXP]    = 50;
        ap->lo[DCUT_LIM]    = 0;
        ap->hi[DCUT_LIM]    = 96;
        break;
    case(ENVCUT):
        ap->lo[ECUT_CNT]    = .001;
        ap->hi[ECUT_CNT]    = 1000;
        ap->lo[ECUT_STP]    = .001;
        ap->hi[ECUT_STP]    = 1000;
        ap->lo[ECUT_ATT]    = 0.5;
        ap->hi[ECUT_ATT]    = (duration/2.0) * SECS_TO_MS;
        ap->lo[ECUT_EXP]    = 0.02;
        ap->hi[ECUT_EXP]    = 50;
        ap->lo[ECUT_LIM]    = 0;
        ap->hi[ECUT_LIM]    = 96;
        break;
    case(SPECFOLD):
        ap->lo[0] = 1;
        ap->hi[0] = (wanted/2) - 4;
        ap->lo[1] = 4;
        ap->hi[1] = wanted/2;
        switch(mode) {
        case(0):
            ap->lo[2] = 1;
            ap->hi[2] = wanted/2;
            break;
        case(2):
            ap->lo[2] = 1;
            ap->hi[2] = 64;
            break;
        }
        break;
    case(BROWNIAN):
        ap->lo[BRCHANS] = 1;
        ap->hi[BRCHANS] = 16;
        ap->lo[BRDUR] = duration;
        ap->hi[BRDUR] = 7200;
        if(mode == 0) {
            ap->lo[BRATT] = .002;
            ap->hi[BRATT] = 8;
            ap->lo[BRDEC]   = .002;
            ap->hi[BRDEC]   = 8;
        }
        ap->lo[BRPLO]   = 0;
        ap->hi[BRPLO]   = 127;
        ap->lo[BRPHI]   = 0;
        ap->hi[BRPHI]   = 127;
        ap->lo[BRPSTT]  = 0;
        ap->hi[BRPSTT]  = 127;
        ap->lo[BRSSTT]  = 1;
        ap->hi[BRSSTT]  = 16;
        ap->lo[BRPSTEP] = 0.125;
        ap->hi[BRPSTEP] = 24;
        ap->lo[BRSSTEP] = 0;
        ap->hi[BRSSTEP] = 1;
        ap->lo[BRTICK]  = 0.002;
        ap->hi[BRTICK]  = 4;
        ap->lo[BRSEED]  = 0;
        ap->hi[BRSEED]  = 255;
        ap->lo[BRASTEP] = 0;
        ap->hi[BRASTEP] = 96;
        ap->lo[BRAMIN]  = 0;
        ap->hi[BRAMIN]  = 96;
        if(mode == 0) {
            ap->lo[BRASLP]  = 0.1;
            ap->hi[BRASLP]  = 10;
            ap->lo[BRDSLP]  = 0.1;
            ap->hi[BRDSLP]  = 10;
        }
        break;
    case(SPIN):
        ap->lo[SPNRATE]  = -100;
        ap->hi[SPNRATE]  = 100.0;
        ap->lo[SPNBOOST] = 0;
        ap->hi[SPNBOOST] = 16;
        ap->lo[SPNATTEN] = 0;
        ap->hi[SPNATTEN] = 1;
        if(mode > 0) {
            ap->lo[SPNOCHNS] = 4;
            ap->hi[SPNOCHNS] = 16;
            ap->lo[SPNOCNTR] = 1;
            ap->hi[SPNOCNTR] = 16;
            ap->lo[SPNCMIN]  = 0;
            ap->hi[SPNCMIN]  = 1;
            if(mode == 1) {
                ap->lo[SPNCMAX]  = 0;
                ap->hi[SPNCMAX]  = 1;
            }
        }
        ap->lo[SPNDOPL]  = 0;
        ap->hi[SPNDOPL]  = 12;
        ap->lo[SPNXBUF]  = 1;
        ap->hi[SPNXBUF]  = 64;
        break;
    case(SPINQ):
        ap->lo[SPNRATE]  = -100;
        ap->hi[SPNRATE]  = 100.0;
        ap->lo[SPNBOOST] = 0;
        ap->hi[SPNBOOST] = 16;
        ap->lo[SPNATTEN] = 0;
        ap->hi[SPNATTEN] = 1;
        ap->lo[SPNOCHNS] = 5;
        ap->hi[SPNOCHNS] = 16;
        ap->lo[SPNOCNTR] = 1;
        ap->hi[SPNOCNTR] = 16;
        ap->lo[SPNDOPL]  = 0;
        ap->hi[SPNDOPL]  = 12;
        ap->lo[SPNXBUF]  = 1;
        ap->hi[SPNXBUF]  = 64;
        ap->lo[SPNCMIN]  = 0;
        ap->hi[SPNCMIN]  = 1;
        if(mode == 0) {
            ap->lo[SPNCMAX]  = 0;
            ap->hi[SPNCMAX]  = 1;
        }
        break;
    case(CRUMBLE):
        ap->lo[CRSTART] = 0;
        ap->hi[CRSTART] = duration;
        ap->lo[CRSTEP1] = 0;
        ap->hi[CRSTEP1] = duration;
        ap->lo[CRSTEP2] = 0;
        ap->hi[CRSTEP2] = duration;
        if(mode == 1) {
            ap->lo[CRSTEP3] = 0;
            ap->hi[CRSTEP3] = duration;
        }
        ap->lo[CRORIENT] = 1;
        if(mode == 1)
            ap->hi[CRORIENT] = 16;
        else
            ap->hi[CRORIENT] = 8;
        ap->lo[CRSIZE]  = 10.00001 * MS_TO_SECS;
        ap->hi[CRSIZE]  = duration;
        ap->lo[CRRAND]  = 0;
        ap->hi[CRRAND]  = 1;
        ap->lo[CRISCAT] = 0;
        ap->hi[CRISCAT] = 1;
        ap->lo[CROSCAT] = 0;
        ap->hi[CROSCAT] = 1;
        ap->lo[CROSTR]  = 1;
        ap->hi[CROSTR]  = 64;
        ap->lo[CRPSCAT] = 0;
        ap->hi[CRPSCAT] = 12;
        ap->lo[CRSEED]  = 1;
        ap->hi[CRSEED]  = 256;
        ap->lo[CRSPLICE]= 2;
        ap->hi[CRSPLICE]= 50;
        ap->lo[CRTAIL]  = 0;
        ap->hi[CRTAIL]  = 1000;
        ap->lo[CRDUR]   = 0;
        ap->hi[CRDUR]   = 3600;
        break;
    case(PHASOR):
        ap->lo[PHASOR_STREAMS]  = 2;
        ap->hi[PHASOR_STREAMS]  = 8;
        ap->lo[PHASOR_FRQ]      = .01;
        ap->hi[PHASOR_FRQ]      = 100;
        ap->lo[PHASOR_SHIFT]    = 0;
        ap->hi[PHASOR_SHIFT]    = 12;
        ap->lo[PHASOR_OCHANS]   = 1;
        ap->hi[PHASOR_OCHANS]   = 8;
        ap->lo[PHASOR_OFFSET]   = 0;
        ap->hi[PHASOR_OFFSET]   = 500;
        break;
    case(TESSELATE):
        ap->lo[TESS_CHANS]  = 2;
        ap->hi[TESS_CHANS]  = 16;
        ap->lo[TESS_PHRAS]  = FLTERR;
        ap->hi[TESS_PHRAS]  = 60;
        ap->lo[TESS_DUR]    = 1;
        ap->hi[TESS_DUR]    = 7200;
        ap->lo[TESS_TYP]    = 0;
        ap->hi[TESS_TYP]    = 16;
        break;
    case(CRYSTAL):
        ap->lo[CRY_ROTA]    = CRY_ROT_MIN;
        ap->hi[CRY_ROTA]    = CRY_ROT_MAX;
        ap->lo[CRY_ROTB]    = CRY_ROT_MIN;
        ap->hi[CRY_ROTB]    = CRY_ROT_MAX;
        ap->lo[CRY_TWIDTH]  = CRY_TW_MIN;
        ap->hi[CRY_TWIDTH]  = CRY_TW_MAX;
        ap->lo[CRY_TSTEP]   = CRY_TSTEP_MIN;
        ap->hi[CRY_TSTEP]   = CRY_TSTEP_MAX;
        ap->lo[CRY_DUR]     = 0.1;
        ap->hi[CRY_DUR]     = CRY_DUR_MAX;
        ap->lo[CRY_PLO]     = 0;
        ap->hi[CRY_PLO]     = 127;
        ap->lo[CRY_PHI]     = 0;
        ap->hi[CRY_PHI]     = 127;
        ap->lo[CRY_FPASS]   = 16;
        ap->hi[CRY_FPASS]   = 4000;
        ap->lo[CRY_FSTOP]   = 50;
        ap->hi[CRY_FSTOP]   = 8000;
        ap->lo[CRY_FATT]    = -96;
        ap->hi[CRY_FATT]    = 0;
        ap->lo[CRY_FPRESC]  = 0;
        ap->hi[CRY_FPRESC]  = 1;
        ap->lo[CRY_FSLOPE]  = 0.1;
        ap->hi[CRY_FSLOPE]  = 10;
        ap->lo[CRY_SSLOPE]  = 0.1;
        ap->hi[CRY_SSLOPE]  = 10;
        break;
    case(WAVEFORM):
        ap->lo[WF_TIME] = 0.0;
        ap->hi[WF_TIME] = duration;
        if(mode == 0) {
            ap->lo[WF_CNT]  = 1;
            ap->hi[WF_CNT]  = 256;
        } else {
            ap->lo[WF_DUR]  = 1;
            ap->hi[WF_DUR]  = 10000;
        }
        if(mode == 2) {
            ap->lo[WF_BAL]  = 0.001;
            ap->hi[WF_BAL]  = 1;
        }
        break;
    case(DVDWIND):
        ap->lo[0]   = 1;
        ap->hi[0]   = 3600.0;   
        ap->lo[1]   = 10;
        ap->hi[1]   = 2000;
        break;
    case(CASCADE):
        if(mode < 5) {
            ap->lo[CAS_CLIP]    = 0.005;
            ap->hi[CAS_CLIP]    = 60.0;
            ap->lo[CAS_MAXCLIP] = 0;
            ap->hi[CAS_MAXCLIP] = 60.0;
        }
        ap->lo[CAS_ECHO]    = 1;
        ap->hi[CAS_ECHO]    = 64;
        ap->lo[CAS_MAXECHO] = 0;
        ap->hi[CAS_MAXECHO] = 64;
        ap->lo[CAS_RAND]    = 0;
        ap->hi[CAS_RAND]    = 1;
        ap->lo[CAS_SEED]    = 0;
        ap->hi[CAS_SEED]    = 64;
        ap->lo[CAS_SHREDNO] = 0;
        ap->hi[CAS_SHREDNO] = 64;
        ap->lo[CAS_SHREDCNT]= 0;
        ap->hi[CAS_SHREDCNT]= 64;
        break;
    case(SYNSPLINE):
        ap->lo[SPLIN_SRATE] = 16000;
        ap->hi[SPLIN_SRATE] = 96000;
        ap->lo[SPLIN_DUR]   = 0.0;
        ap->hi[SPLIN_DUR]   = 32767.0;
        ap->lo[SPLIN_FRQ]   = .001;
        ap->hi[SPLIN_FRQ]   = 10000;
        ap->lo[SPLIN_CNT]   = 0;
        ap->hi[SPLIN_CNT]   = 64;
        ap->lo[SPLIN_INTP]  = 0;
        ap->hi[SPLIN_INTP]  = 4096;
        ap->lo[SPLIN_SEED]  = 0;
        ap->hi[SPLIN_SEED]  = 64;
        ap->lo[SPLIN_MCNT]  = 0;
        ap->hi[SPLIN_MCNT]  = 64;
        ap->lo[SPLIN_MINTP] = 0;
        ap->hi[SPLIN_MINTP] = 4096;
        ap->lo[SPLIN_DRIFT] = 0;
        ap->hi[SPLIN_DRIFT] = 12;
        ap->lo[SPLIN_DRVEL] = 0;
        ap->hi[SPLIN_DRVEL] = 1000;
        break;
    case(SPLINTER):
        ap->lo[SPL_TIME]    = 0.0;
        ap->hi[SPL_TIME]    = duration;
        ap->lo[SPL_WCNT]    = 0;
        ap->hi[SPL_WCNT]    = 256;
        ap->lo[SPL_SHRCNT]  = 2;
        ap->hi[SPL_SHRCNT]  = 256;
        ap->lo[SPL_OCNT]    = 0;
        ap->hi[SPL_OCNT]    = 256;
        ap->lo[SPL_PULS1]   = 0;
        ap->hi[SPL_PULS1]   = 50;
        ap->lo[SPL_PULS2]   = 0;
        ap->hi[SPL_PULS2]   = 50;
        ap->lo[SPL_ECNT]    = 0;
        ap->hi[SPL_ECNT]    = 10000;
        ap->lo[SPL_SCURVE]  = 0.1;
        ap->hi[SPL_SCURVE]  = 10.0;
        ap->lo[SPL_PCURVE]  = 0.1;
        ap->hi[SPL_PCURVE]  = 10.0;
        if(mode <= 1) {
            ap->lo[SPL_FRQ] = 1000.0;
            ap->hi[SPL_FRQ] = nyquist/2.0;
        } else {
            ap->lo[SPL_DUR] = 5.0;
            ap->hi[SPL_DUR] = 50.0;
        }
        ap->lo[SPL_RND] = 0.0;
        ap->hi[SPL_RND] = 1.0;
        ap->lo[SPL_SHRND] = 0.0;
        ap->hi[SPL_SHRND] = 1.0;
        break;
    case(REPEATER):
        if(mode >= 2) {
            ap->lo[REP_ACCEL] = 1.0;
            ap->hi[REP_ACCEL] = 10.0;
            ap->lo[REP_WARP] = 0.1;
            ap->hi[REP_WARP] = 10.0;
            ap->lo[REP_FADE] = 0.1;
            ap->hi[REP_FADE] = 10.0;
        }
        if(mode == 0 || mode == 2)
            ap->hi[REP_RAND] = 2.0;
        else
            ap->hi[REP_RAND] = 8.0;
        ap->lo[REP_TRNSP] = 0.0;
        ap->hi[REP_TRNSP] = 12.0;
        ap->lo[REP_SEED]  = 0;
        ap->hi[REP_SEED]  = 256;
        break;
    case(VERGES):
        ap->lo[VRG_TRNSP] = -24.0;
        ap->hi[VRG_TRNSP] =  24.0;
        ap->lo[VRG_CURVE] = 1.0;
        ap->hi[VRG_CURVE] = 8.0;
        ap->lo[VRG_DUR] = 20;
        ap->hi[VRG_DUR] = 1000;
        break;
    case(MOTOR):
        ap->lo[MOT_DUR]    = 1.0;
        ap->hi[MOT_DUR]    = 7200.0;
        ap->lo[MOT_FRQ]    = 2;
        ap->hi[MOT_FRQ]    = 100;
        ap->lo[MOT_PULSE]  = 0.1;
        ap->hi[MOT_PULSE]  = 10.0;
        ap->lo[MOT_FRATIO] = 0.0;
        ap->hi[MOT_FRATIO] = 1.0;
        ap->lo[MOT_PRATIO] = 0.0;
        ap->hi[MOT_PRATIO] = 1.0;
        ap->lo[MOT_SYM]    = 0.0;
        ap->hi[MOT_SYM]    = 1.0;
        ap->lo[MOT_FRND]   = 0.0;
        ap->hi[MOT_FRND]   = 1.0;
        ap->lo[MOT_PRND]   = 0.0;
        ap->hi[MOT_PRND]   = 1.0;
        ap->lo[MOT_JIT]    = 0.0;
        ap->hi[MOT_JIT]    = 3.0;
        ap->lo[MOT_TREM]   = 0.0;
        ap->hi[MOT_TREM]   = 1.0;
        ap->lo[MOT_SYMRND] = 0.0;
        ap->hi[MOT_SYMRND] = 1.0;
        ap->lo[MOT_EDGE]   = 0;
        ap->hi[MOT_EDGE]   = 20.0;
        ap->lo[MOT_BITE]   = 0.1;
        ap->hi[MOT_BITE]   = 10.0;
        ap->lo[MOT_VARY]   = 0;
        ap->hi[MOT_VARY]   = 1.0;
        ap->lo[MOT_SEED]   = 0;
        ap->hi[MOT_SEED]   = 256;
        break;
    case(STUTTER):
        ap->lo[STUT_DUR]    = 1;
        ap->hi[STUT_DUR]    = 7200;
        ap->lo[STUT_JOIN]   = 1;
        ap->hi[STUT_JOIN]   = STUT_MAX_JOIN;
        ap->lo[STUT_SIL]    = 0;
        ap->hi[STUT_SIL]    = 1;
        ap->lo[STUT_SILMIN] = 0;
        ap->hi[STUT_SILMIN] = 10;
        ap->lo[STUT_SILMAX] = 0;
        ap->hi[STUT_SILMAX] = 10;
        ap->lo[STUT_SEED]   = 0;
        ap->hi[STUT_SEED]   = 256;
        ap->lo[STUT_TRANS]  = 0;
        ap->hi[STUT_TRANS]  = 3;
        ap->lo[STUT_ATTEN]  = 0;
        ap->hi[STUT_ATTEN]  = 1;
        ap->lo[STUT_BIAS]   = -1.0;
        ap->hi[STUT_BIAS]   = 1.0;
        ap->lo[STUT_MINDUR] = STUT_SPLICE + STUT_DOVE;
        ap->hi[STUT_MINDUR] = 250;
        break;
    case(SCRUNCH):
        if(mode <= 1) {
            ap->lo[SCR_DUR]  = 1.0;
            ap->hi[SCR_DUR]  = 7200.0;
        }
        ap->lo[SCR_SEED] = 0;
        ap->hi[SCR_SEED] = 256;
        ap->lo[SCR_CNT]   = 1.0;
        ap->hi[SCR_CNT]   = 256.0;
        ap->lo[SCR_TRNS]  = 0.0;
        ap->hi[SCR_TRNS]  = 12.0;
        ap->lo[SCR_ATTEN] = .0;
        ap->hi[SCR_ATTEN] = 1.0;
        break;
    case(IMPULSE):
        ap->lo[IMP_DUR]   = 0;
        ap->hi[IMP_DUR]   = 7200;
        ap->lo[IMP_PICH]  = 1.0;
        ap->hi[IMP_PICH]  = 1000.0;
        ap->lo[IMP_CHIRP] = 0.0;
        ap->hi[IMP_CHIRP] = 30.0;
        ap->lo[IMP_SLOPE] = 1.0;
        ap->hi[IMP_SLOPE] = 20.0;
        ap->lo[IMP_CYCS]  = 1;
        ap->hi[IMP_CYCS]  = 200;
        ap->lo[IMP_LEV]   = 0;
        ap->hi[IMP_LEV]   = 1;
        ap->lo[IMP_GAP]   = -.99;
        ap->hi[IMP_GAP]   = 10;
        ap->lo[IMP_SRATE] = 16000;
        ap->hi[IMP_SRATE] = 96000;
        ap->lo[IMP_CHANS] = 1.0;
        ap->hi[IMP_CHANS] = 16.0;
        break;
    case(TWEET):
        ap->lo[TWT_PDAT]  = -2.0;           //  Pitch data
        ap->hi[TWT_PDAT]  = nyquist;
        ap->lo[TWT_MIN]   = -60.0;
        ap->hi[TWT_MIN]   = 0.0;
        if(mode != 2) {
            ap->lo[TWT_PKCNT] = 1;
            if(mode == 1)
                ap->hi[TWT_PKCNT] = 4000;
            else
                ap->hi[TWT_PKCNT] = 200;
            ap->lo[TWT_CHIRP] = 0;
            ap->hi[TWT_CHIRP] = 30;
        }
        break;
    case(RRRR_EXTEND):  //  version 8+
        if(mode == 1) {
            ap->lo[RRR_GATE]   = 0.0;
            ap->hi[RRR_GATE]   = 1.0;
            ap->lo[RRR_GRSIZ]  = LOW_RRR_SIZE;
            ap->hi[RRR_GRSIZ]  = SECS_TO_MS;
            ap->lo[RRR_SKIP]   = 0;
            ap->hi[RRR_SKIP]   = 8;
        } else {
            ap->lo[RRR_START]   = 0.0;
            ap->hi[RRR_START]   = duration;
            ap->lo[RRR_END]     = 0.0;
            ap->hi[RRR_END]     = duration;
        }
        ap->lo[RRR_SLOW]    = 1.0;
        ap->hi[RRR_SLOW]    = 100;
        ap->lo[RRR_REGU]    = 0.0;
        ap->hi[RRR_REGU]    = 1.0;
        ap->lo[RRR_RANGE]   = 0.0;
        ap->hi[RRR_RANGE]   = 4.0;
        ap->lo[RRR_GET]     = 2.0;
        ap->hi[RRR_GET]     = 100.0;
        if (mode != 2) {
            ap->lo[RRR_STRETCH] = 1.0;
            ap->hi[RRR_STRETCH] = 32767.0;
            ap->lo[RRR_REPET]   = 1.0;
            ap->hi[RRR_REPET]   = 32767.0;
            ap->lo[RRR_ASCAT]   = 0.0;
            ap->hi[RRR_ASCAT]   = 1.0;
            ap->lo[RRR_PSCAT]   = 0.0;
            ap->hi[RRR_PSCAT]   = 24.0;
        }
        break;
    case(SORTER):
        ap->lo[SORTER_SIZE]   = 0.0;
        ap->hi[SORTER_SIZE]   = 2000;
        if(mode == 4) {
            ap->lo[SORTER_SEED] = 0;
            ap->hi[SORTER_SEED] = 256;
        }
        ap->lo[SORTER_SMOOTH] = 0;
        ap->hi[SORTER_SMOOTH] = 50;
        ap->lo[SORTER_OMIDI]  = 0;
        ap->hi[SORTER_OMIDI]  = 128;
        ap->lo[SORTER_IMIDI]  = 0;
        ap->hi[SORTER_IMIDI]  = 128;
        ap->lo[SORTER_META]   = 0.0;
        ap->hi[SORTER_META]   = duration/3.0;
        break;
    case(SPECFNU):
        switch(mode) {
        case(F_NARROW):
            ap->lo[NARROWING] = 1.0;
            ap->hi[NARROWING] = 1000.0;
            ap->lo[NARSUPRES] = 0;
            ap->hi[NARSUPRES] = 234;
            ap->lo[FGAIN]     = 0.01;
            ap->hi[FGAIN]     = 10.0;
            break;
        case(F_SQUEEZE):
            ap->lo[SQZFACT] = 1.0;
            ap->hi[SQZFACT] = 10.0;
            ap->lo[SQZAT]   = 1;
            ap->hi[SQZAT]   = 4;
            ap->lo[FGAIN]   = 0.01;
            ap->hi[FGAIN]   = 10.0;
            break;
        case(F_INVERT):
            ap->lo[FVIB]  = 0.0;
            ap->hi[FVIB]  = 300.0;
            ap->lo[FGAIN] = 0.01;
            ap->hi[FGAIN] = 10.0;
            break;
        case(F_ROTATE):
            ap->lo[RSPEED] = -300.0;
            ap->hi[RSPEED] = 300.0;
            ap->lo[FGAIN]  = 0.01;
            ap->hi[FGAIN]  = 10.0;
            break;
        case(F_NEGATE):
            ap->lo[FGAIN] = 0.01;
            ap->hi[FGAIN] = 10.0;
            break;
        case(F_SUPPRESS):
            ap->lo[SUPRF] = 1;
            ap->hi[SUPRF] = 1234;
            ap->lo[FGAIN] = 0.01;
            ap->hi[FGAIN] = 10.0;
            break;
        case(F_MAKEFILT):
            ap->lo[FPKCNT] = 1;
            ap->hi[FPKCNT] = MAXFILTVALS;
            ap->lo[FBELOW] = 0;
            ap->hi[FBELOW] = 127;
            break;
        case(F_MOVE):
            ap->lo[FMOVE1]  = -8000;
            ap->hi[FMOVE1]  = 8000.0;
            ap->lo[FMOVE2]  = -8000;
            ap->hi[FMOVE2]  = 8000.0;
            ap->lo[FMOVE3]  = -8000;
            ap->hi[FMOVE3]  = 8000.0;
            ap->lo[FMOVE4]  = -8000;
            ap->hi[FMOVE4]  = 8000.0;
            ap->lo[FMVGAIN] = 0.01;
            ap->hi[FMVGAIN] = 10.0;
            break;
        case(F_MOVE2):
            ap->lo[FMOVE1]  = 10;
            ap->hi[FMOVE1]  = 8000.0;
            ap->lo[FMOVE2]  = 10;
            ap->hi[FMOVE2]  = 8000.0;
            ap->lo[FMOVE3]  = 10;
            ap->hi[FMOVE3]  = 8000.0;
            ap->lo[FMOVE4]  = 10;
            ap->hi[FMOVE4]  = 8000.0;
            ap->lo[FMVGAIN] = 0.01;
            ap->hi[FMVGAIN] = 10.0;
            break;
        case(F_SYLABTROF):
            ap->lo[FMINSYL] = 0.05;
            ap->hi[FMINSYL] = 0.5;
            ap->lo[FMINPKG] = 0.01;
            ap->hi[FMINPKG] = 0.2;
            break;
        case(F_ARPEG):
            ap->lo[FARPRATE] = -50.0;
            ap->hi[FARPRATE] = 50.0;
            ap->lo[FGAIN]    = 0.01;
            ap->hi[FGAIN]    = 10.0;
            break;
        case(F_OCTSHIFT):
            ap->lo[COLINT]  = -4.0;
            ap->hi[COLINT]  = 4.0;
            ap->lo[FGAIN]   = 0.01;
            ap->hi[FGAIN]   = 10.0;
            ap->lo[COL_LO]  = 0.0;
            ap->hi[COL_LO]  = 10000.0;
            ap->lo[COL_HI]  = 50.0;
            ap->hi[COL_HI]  = 10000.0;
            ap->lo[COLRATE] = -50.0;
            ap->hi[COLRATE] = 50.0;
            break;
        case(F_TRANS):
            ap->lo[COLFLT]  = -48.0;
            ap->hi[COLFLT]  = 48.0;
            ap->lo[FGAIN]   = 0.01;
            ap->hi[FGAIN]   = 10.0;
            ap->lo[COL_LO]  = 0.0;
            ap->hi[COL_LO]  = 10000.0;
            ap->lo[COL_HI]  = 50.0;
            ap->hi[COL_HI]  = 10000.0;
            ap->lo[COLRATE] = -50.0;
            ap->hi[COLRATE] = 50.0;
            break;
        case(F_FRQSHIFT):
            ap->lo[COLFLT]  = -1000.0;
            ap->hi[COLFLT]  = 1000.0;
            ap->lo[FGAIN]   = 0.01;
            ap->hi[FGAIN]   = 10.0;
            ap->lo[COL_LO]  = 0.0;
            ap->hi[COL_LO]  = 10000.0;
            ap->lo[COL_HI]  = 50.0;
            ap->hi[COL_HI]  = 10000.0;
            ap->lo[COLRATE] = -50.0;
            ap->hi[COLRATE] = 50.0;
            break;
        case(F_RESPACE):
            ap->lo[COLFLT]  = 1.0;
            ap->hi[COLFLT]  = 1000.0;
            ap->lo[FGAIN]   = 0.01;
            ap->hi[FGAIN]   = 10.0;
            ap->lo[COL_LO]  = 0.0;
            ap->hi[COL_LO]  = 10000.0;
            ap->lo[COL_HI]  = 50.0;
            ap->hi[COL_HI]  = 10000.0;
            ap->lo[COLRATE] = -50.0;
            ap->hi[COLRATE] = 50.0;
            break;
        case(F_PINVERT):
            ap->lo[COLFLT]   = 0.0;
            ap->hi[COLFLT]   = 127;
            ap->lo[FGAIN]    = 0.01;
            ap->hi[FGAIN]    = 10.0;
            ap->lo[COL_LO]   = 0.0;
            ap->hi[COL_LO]   = 10000.0;
            ap->lo[COL_HI]   = 50.0;
            ap->hi[COL_HI]   = 10000.0;
            ap->lo[COLRATE]  = -50.0;
            ap->hi[COLRATE]  = 50.0;
            ap->lo[COLLOPCH] = SPEC_MIDIMIN;
            ap->hi[COLLOPCH] = MIDIMAX;
            ap->lo[COLHIPCH] = SPEC_MIDIMIN;
            ap->hi[COLHIPCH] = MIDIMAX;
            break;
        case(F_PEXAGG):
            ap->lo[COLFLT]   = 0.0;
            ap->hi[COLFLT]   = MIDIMAX;
            ap->lo[EXAGRANG] = 0.0;
            ap->hi[EXAGRANG] = PEX_MAX_RANG;
            ap->lo[FGAIN]    = 0.01;
            ap->hi[FGAIN]    = 10.0;
            ap->lo[COL_LO]   = 0.0;
            ap->hi[COL_LO]   = 10000.0;
            ap->lo[COL_HI]   = 50.0;
            ap->hi[COL_HI]   = 10000.0;
            ap->lo[COLRATE]  = -50.0;
            ap->hi[COLRATE]  = 50.0;
            ap->lo[COLLOPCH] = SPEC_MIDIMIN;
            ap->hi[COLLOPCH] = MIDIMAX;
            ap->lo[COLHIPCH] = SPEC_MIDIMIN;
            ap->hi[COLHIPCH] = MIDIMAX;
            break;
        case(F_PQUANT):
            ap->lo[FGAIN]    = 0.01;
            ap->hi[FGAIN]    = 10.0;
            ap->lo[COL_LO]   = 0.0;
            ap->hi[COL_LO]   = 10000.0;
            ap->lo[COL_HI]   = 50.0;
            ap->hi[COL_HI]   = 10000.0;
            ap->lo[COLRATE]  = -50.0;
            ap->hi[COLRATE]  = 50.0;
            ap->lo[COLLOPCH] = SPEC_MIDIMIN;
            ap->hi[COLLOPCH] = MIDIMAX;
            ap->lo[COLHIPCH] = SPEC_MIDIMIN;
            ap->hi[COLHIPCH] = MIDIMAX;
            break;
        case(F_PCHRAND):
            ap->lo[FPRMAXINT] = 0.0;
            ap->hi[FPRMAXINT] = RANDPITCHMAX;
            ap->lo[FSLEW]     = 0.1;
            ap->hi[FSLEW]     = 10.0;
            ap->lo[FGAIN]     = 0.01;
            ap->hi[FGAIN]     = 10.0;
            ap->lo[COL_LO]    = 0.0;
            ap->hi[COL_LO]    = 10000.0;
            ap->lo[COL_HI]    = 50.0;
            ap->hi[COL_HI]    = 10000.0;
            ap->lo[COLRATE]   = -50.0;
            ap->hi[COLRATE]   = 50.0;
            ap->lo[COLLOPCH]  = SPEC_MIDIMIN;
            ap->hi[COLLOPCH]  = MIDIMAX;
            ap->lo[COLHIPCH]  = SPEC_MIDIMIN;
            ap->hi[COLHIPCH]  = MIDIMAX;
            break;
        case(F_RAND):
            ap->lo[COLFLT]  = 0.0;
            ap->hi[COLFLT]  = 1.0;
            ap->lo[FGAIN]   = 0.01;
            ap->hi[FGAIN]   = 10.0;
            ap->lo[COL_LO]  = 0.0;
            ap->hi[COL_LO]  = 10000.0;
            ap->lo[COL_HI]  = 50.0;
            ap->hi[COL_HI]  = 10000.0;
            ap->lo[COLRATE] = -50.0;
            ap->hi[COLRATE] = 50.0;
            break;
        case(F_SINUS):
            ap->lo[F_SINING] = 0.0;
            ap->hi[F_SINING] = 1.0;
            ap->lo[FGAIN]    = 0.01;
            ap->hi[FGAIN]    = 10.0;
            ap->lo[F_AMP1]   = 0.0;
            ap->hi[F_AMP1]   = 10.0;
            ap->lo[F_AMP2]   = 0.0;
            ap->hi[F_AMP2]   = 10.0;
            ap->lo[F_AMP3]   = 0.0;
            ap->hi[F_AMP3]   = 10.0;
            ap->lo[F_AMP4]   = 0.0;
            ap->hi[F_AMP4]   = 10.0;
            ap->lo[F_QDEP1]  = 0.0;
            ap->hi[F_QDEP1]  = 1.0;
            ap->lo[F_QDEP2]  = 0.0;
            ap->hi[F_QDEP2]  = 1.0;
            ap->lo[F_QDEP3]  = 0.0;
            ap->hi[F_QDEP3]  = 1.0;
            ap->lo[F_QDEP4]  = 0.0;
            ap->hi[F_QDEP4]  = 1.0;
            break;
        }
        break;
    case(FLATTEN):
        ap->lo[0]   = 0.001;
        ap->hi[0]   = 100.0;
        ap->lo[1]   = 20.0;
        ap->hi[1]   = 50000.0;
        ap->lo[2]   = 0.0;
        ap->hi[2]   = duration - FLTERR;
        break;
    case(BOUNCE):
        ap->lo[BNC_NUMBER]  = 1;
        ap->hi[BNC_NUMBER]  = 100;
        ap->lo[BNC_STTSTEP] = 0.04;
        ap->hi[BNC_STTSTEP] = 10.0;
        ap->lo[BNC_SHORTEN] = 0.1;
        ap->hi[BNC_SHORTEN] = 1.0;
        ap->lo[BNC_ENDLEV]  = 0.0;
        ap->hi[BNC_ENDLEV]  = 1.0;
        ap->lo[BNC_LEVWRP]  = .01;
        ap->hi[BNC_LEVWRP]  = 100.0;
        ap->lo[BNC_MINDUR]  = 0.0;
        ap->hi[BNC_MINDUR]  = 1.0;
        break;
    case(DISTMARK):
        ap->lo[0]   = .5;
        ap->hi[0]   = 1000;
        ap->lo[1]   = 1;
        ap->hi[1]   = 256;
        ap->lo[2]   = 0;
        ap->hi[2]   = 1;
        if(mode == 1) {
            ap->lo[3]   = 0;
            ap->hi[3]   = 1;
        }
        break;
    case(DISTREP):
        ap->lo[0]       = 2.0;
        ap->hi[0]       = BIG_VALUE;
        ap->lo[1]       = 1.0;
        ap->hi[1]       = MAX_CYCLECNT;
        ap->lo[2]       = 0.0;
        ap->hi[2]       = MAX_CYCLECNT;
        ap->lo[3]       = 0.0;
        ap->hi[3]       = 50.0;
        break;
    case(TOSTEREO):
        ap->lo[0]   = 0.0;
        ap->hi[0]   = duration;
        ap->lo[1]   = 0.0;
        ap->hi[1]   = duration;
        ap->lo[2]   = 2;
        ap->hi[2]   = 16;
        ap->lo[3]   = 1;
        ap->hi[3]   = 16;
        ap->lo[4]   = 1;
        ap->hi[4]   = 16;
        ap->lo[5]   = 0.0;
        ap->hi[5]   = 1;
        break;
    case(SUPPRESS):
        ap->lo[0]   = 0.0;
        ap->hi[0]   = nyquist;
        ap->lo[1]   = 0.0;
        ap->hi[1]   = nyquist;
        ap->lo[2]   = 1;
        ap->hi[2]   = channels/2;
        break;
    case(CALTRAIN):
        ap->lo[0]   = frametime;
        ap->hi[0]   = duration;
        ap->lo[1]   = 0;
        ap->hi[1]   = nyquist;
        ap->lo[2]   = 0;
        ap->hi[2]   = nyquist;
        break;
    case(SPECENV):
        ap->lo[0]           = 1;
        ap->hi[0]           = (channels/2) - 1;
        ap->lo[1]           = -1;
        ap->hi[1]           = 1;
        break;
    case(CLIP):
        ap->lo[0]           = 0;
        ap->hi[0]           = 1;
        break;
    case(SPECEX):
        ap->lo[0]           = 0;
        ap->hi[0]           = duration;
        ap->lo[1]           = frametime * 3;
        ap->hi[1]           = duration;
        ap->lo[2]           = 2.0;
        ap->hi[2]           = 32767.0;
        ap->lo[3]           = 1;
        ap->hi[3]           = 8;
        break;
    case(MATRIX):
        switch(mode) {
        case(MATRIX_USE):
            break;
        default:
            ap->lo[MATRIX_CHANS] = (double)2;
            ap->hi[MATRIX_CHANS] = (double)MAX_PVOC_CHANS;
            ap->lo[MATRIX_OVLAP] = (double)1;
            ap->hi[MATRIX_OVLAP] = (double)4;
            break;
        }
        break;
    case(TRANSPART):
        switch(mode) {
        case(0):    //  fall thro
        case(1):
        case(2):
        case(3):
            ap->lo[0]   = -48;
            ap->hi[0]   = 48;
            break;
        case(4):    //  fall thro
        case(5):
        case(6):
        case(7):
            ap->lo[0]   = -nyquist/4;
            ap->hi[0]   = nyquist/4;
            break;
        }
        ap->lo[1]   = 5.0;
        ap->hi[1]   = nyquist/2.0;
        ap->lo[2]   = 0.1;
        ap->hi[2]   = 1.0;
        break;
    case(SPECINVNU):
        ap->lo[0]   = 0.0;
        ap->hi[0]   = nyquist/2;
        ap->lo[1]   = 100.0;
        ap->hi[1]   = nyquist;
        ap->lo[2]   = 0;
        ap->hi[2]   = nyquist;
        ap->lo[3]   = 0.1;
        ap->hi[3]   = 1.0;
        break;
    case(SPECCONV):
        ap->lo[0]   = 0.1;
        ap->hi[0]   = 10.0;
        ap->lo[1]   = 1;
        if(mode == 0)
            ap->hi[1]   = 10;
        else
            ap->hi[1]   = 2;
        break;
    case(SPECSND):
        ap->lo[0]   = 0;
        ap->hi[0]   = 8;
        ap->lo[1]   = 1;
        ap->hi[1]   = 8;
        break;
    case(FRACTAL):
        if(mode == 1) {
            ap->lo[0]   = 1;
            ap->hi[0]   = FRAC_MAXDUR;
        }
        ap->lo[1]   = 0;
        ap->hi[1]   = FRAC_MAXFRACT;
        ap->lo[2]   = 1;
        ap->hi[2]   = 2;
        ap->lo[3]   = 0;
        ap->hi[3]   = 8;
        break;
    case(FRACSPEC):
        ap->lo[1]   = 0;
        ap->hi[1]   = FRAC_MAXFRACT;
        ap->lo[2]   = 1;
        ap->hi[2]   = 2;
        ap->lo[3]   = 0;
        ap->hi[3]   = 8;
        break;
    case(SPECFRAC):
        switch(mode) {
        case(6):    //  fall thro
        case(8):
            ap->lo[0]   = 2;
            break;
        default:
            ap->lo[0]   = 1;
            break;
        }
        ap->hi[0]   = 100;
        break;
    case(ENVSPEAK):
        if(mode < 12) {
            ap->lo[0]   = 5;            //  param 0     Windowsize
            ap->hi[0]   = 1000;
        }
        mode %= 12;
        ap->lo[1]   = 2;                //  param 1     Splicelen
        ap->hi[1]   = 100;
        if(mode < 9) {
            ap->lo[2]   = 0;            //  param 2     Offset
            ap->hi[2]   = 100;
            switch(mode) {
            case(2):
            case(3):
                ap->lo[4]   = -96;      //  param 4     //  gain on attenuated syllabs
                ap->hi[4]   = 0;
                //  fall thro
            case(0):    //  fall thro
            case(4):    //  fall thro
            case(5):    //  fall thro
            case(6):    //  fall thro
            case(7):    //  fall thro
            case(8):
                ap->lo[3]   = 1;        //  param 3     //  Repet_cnt = Attenuated_cnt
                ap->hi[3]   = 100;
                break;
            }
            if(!(mode == 1 || mode == 2 || mode == 3 || mode == 10 || mode == 11)) {
                ap->lo[4]   = 0;        //  param 4     //  Randomisation
                ap->hi[4]   = 1;
            }
            switch(mode) {
            case(6):
                ap->lo[3]   = 1;        //  param 3     //  Division Cnt
                ap->hi[3]   = 100;
                ap->lo[5]   = 1;        //  param 5     Which syllable-division to use
                ap->hi[5]   = 100;
                break;
            case(7):    //  fall thro
            case(8):
                ap->lo[5]   = 0.1;      //  param 5     Contraction ratio
                ap->hi[5]   = 1.0;
                break;
            }
        }
        switch(mode) {
        case(10):
            ap->lo[2]   = 0;            //  param 2     Random seed for perms
            ap->hi[2]   = 64;
            break;
        case(11):
            ap->lo[2]   = 1;            //  param 2     Group-size for reversal
            ap->hi[2]   = 100;
            break;
        }
        break;
    case(EXTSPEAK):
        if(mode < 6) {
            ap->lo[XSPK_WINSZ]  = 5;            //  param 0
            ap->hi[XSPK_WINSZ]  = 1000;
        }
        ap->lo[XSPK_SPLEN]  = 2;                //  param 1
        ap->hi[XSPK_SPLEN]  = 100;
        if(mode < 12) {
            ap->lo[XSPK_OFFST]  = 0;                //  param 2
            ap->hi[XSPK_OFFST]  = 100;
            ap->lo[XSPK_N]      = 0;                //  param 3
            ap->hi[XSPK_N]      = MAX_PATN;
        }
        ap->lo[XSPK_GAIN]   = -96;              //  param 4
        ap->hi[XSPK_GAIN]   = 0;
        if(mode != 2 && mode != 5 && mode != 8 && mode != 11 && mode != 14 && mode != 17) {
            ap->lo[XSPK_SEED]   = 0;            //  param 5
            ap->hi[XSPK_SEED]   = 64;
        }
        break;
    case(ENVSCULPT):
        if(mode != 2) {
            ap->lo[PKCH_WSIZE]  = 1.0;  //wsize for envelope extraction, mS
            ap->hi[PKCH_WSIZE]  = min(duration,1.0) * SECS_TO_MS;
        }
        ap->lo[PKCH_RISE]   = 1.0;  //risetime for attack, mS
        ap->hi[PKCH_RISE]   = 5;
        ap->lo[PKCH_DECDUR] = 0.001;    //time to reach zero after attack peak
        ap->hi[PKCH_DECDUR] = duration;
        ap->lo[PKCH_STEEP]  = 1;    //(min) steepness of attack.
        ap->hi[PKCH_STEEP]  = 10;
        if(mode == 1) {
            ap->lo[PKCH_ZSTT]   = 0.0;      //time to start fall to zero after consonant
            ap->hi[PKCH_ZSTT]   = duration;
            ap->lo[PKCH_ZEND]   = 0.0;      //time to start rise from zero before vowel
            ap->hi[PKCH_ZEND]   = duration;
        }
        if(mode != 0) {
            ap->lo[PKCH_RATIO]  = 0.1;      //ratio loudness of cons to loudness of vwl
            ap->hi[PKCH_RATIO]  = 10;
        }
        break;
    case(TREMENV):
        ap->lo[TREMOLO_FRQ]          = 0.0;
        ap->hi[TREMOLO_FRQ]          = 500;
        ap->lo[TREMOLO_DEP]          = 0.0;
        ap->hi[TREMOLO_DEP]          = 1.0;
        ap->lo[TREMOLO_WIN]          = 1.0; //wsize for envelope extraction
        ap->hi[TREMOLO_WIN]          = 40.0;
        ap->lo[TREMOLO_SQZ]          = 1.0;
        ap->hi[TREMOLO_SQZ]          = 100.0;
        break;
    case(DCFIX):
        ap->lo[0]   = 10.0;
        ap->hi[0]   = 100.0;
        break;
    default:
        sprintf(errstr,"Unknown case: get_param_ranges2()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** INITIALISE_PARAM_VALUES2 *********************************/

int initialise_param_values2(int process,int mode,int channels,double nyquist,float frametime,
    int insams,int srate,int wanted,int linecnt,double duration,double *default_val,int filetype,aplptr ap)
{
    double chwidth;
    switch(process) {
    case(TAPDELAY):
        default_val[0] = 0.25;
        default_val[1] = 0;
        default_val[2] = 0;
        default_val[3] = 0;
        break;
    case(RMRESP):
        default_val[0]  = 0.95; /* liveness */
        default_val[1]  = 4;    /* nrefs */
        default_val[2]  = 10;   /* roomL */
        default_val[3]  = 10;   /* roomW */
        default_val[4]  = 6;    /* roomH */
        default_val[5]  = 9;    /* srcL */
        default_val[6]  = 5;    /* srcW */
        default_val[7]  = 2;    /* srcH */
        default_val[8]  = 1;    /* listenerL */
        default_val[9]  = 5;    /* listenerW */
        default_val[10] = 2;    /* listenerH */
        default_val[11] = 1.0;  /* maxamp */
        default_val[12] = 0.1;  /* res */
        break;
    case(RMVERB):
        default_val[0]  = 3;            /* roomsize */
        default_val[1]  = 0.25;         /* dense_reverb_gain */
        default_val[2]  = 0.5;          /* source_in_mix */
        default_val[3]  = 0.1;          /* feedback */
        default_val[4]  = 2500;         /* air-absorption_cutoff */
        default_val[5]  = 0;            /* lopass_reverb-input_cutoff */
        default_val[6]  = 0;            /* decay_tail */
        default_val[7]  = 0;            /* lopass_input_cutoff */
        default_val[8]  = 0;            /* hipass_input_cutoff */
        default_val[9]  = 0;            /* predelay */
        default_val[10] = 2;            /* output_chans */
        break;
    case(MIXMULTI):
        default_val[MIX_START]  = 0.0;
        default_val[MIX_END]    = duration;
        default_val[MIX_ATTEN]  = 1.0;
        break;
    case(ANALJOIN):
        break;
    case(PTOBRK):
        default_val[0]  = min(duration * SECS_TO_MS,20.0);
        break;
    case(PSOW_STRETCH):
        default_val[0]  = 440.0;
        default_val[1]  = 1.0;
        default_val[2]  = 1;
        break;
    case(PSOW_DUPL):
        default_val[0]  = 0.0;
        default_val[1]  = 2;
        default_val[2]  = 1;
        break;
    case(PSOW_DEL):
        default_val[0]  = 0.0;
        default_val[1]  = 2;
        default_val[2]  = 1;
        break;
    case(PSOW_STRFILL):
        default_val[0]  = 440.0;
        default_val[1]  = 1.0;
        default_val[2]  = 1;
        default_val[3]  = 0;
        break;
    case(PSOW_FREEZE):
        default_val[0]       = 0.0;
        default_val[PS_TIME] = duration/2.0;
        default_val[PS_DUR]  = 1.0;
        default_val[PS_SEGS] = 1;
        default_val[PS_DENS] = 1.0;
        default_val[PS_TRNS] = 1.0;
        default_val[PS_RAND] = 0.0;
        default_val[PS_GAIN] = 1.0;
        break;
    case(PSOW_CHOP):
        default_val[0] = 0.0;
        default_val[1] = 0.0;
        break;
    case(PSOW_INTERP):
        default_val[PS_SDUR] = 1.0;
        default_val[PS_IDUR] = 1.0;
        default_val[PS_EDUR] = 1.0;
        default_val[PS_VIBFRQ]    = 6.5;
        default_val[PS_VIBDEPTH]  = 0.0;
        default_val[PS_TREMFRQ]   = 8.34712;
        default_val[PS_TREMDEPTH] = 0.2;
        break;
    case(PSOW_FEATURES):
        default_val[0]  = 440.0;
        default_val[1]  = 1.0;
        default_val[2]  = 0.0;
        default_val[3]  = 6.5;
        default_val[4]  = 0.0;
        default_val[5]  = 0;
        default_val[6]  = 0;
        default_val[7]  = 1;
        default_val[8]  = 0;
        default_val[9]  = 0.5;
        default_val[10] = 1;
        break;
    case(PSOW_SYNTH):
        default_val[0]  = 440.0;
        default_val[1]  = 1.0;
        break;
    case(PSOW_IMPOSE):
        default_val[0]  = 440.0;
        default_val[1]  = 1.0;
        default_val[2]  = 50.0;
        default_val[3]  = -60.0;
        break;
    case(PSOW_SPLIT):
        default_val[0]  = 440.0;
        default_val[1]  = 3.0;          /* subharmonic_no */
        default_val[2]  = 0.0;          /* upward_transposition_(semitones) */
        default_val[3]  = 1.0;          /* subharmonic_level */
        break;
    case(PSOW_SPACE):
        default_val[0] = 440.0;         /* pitch */
        default_val[1] = 2;             /* subharmno */
        default_val[2] = 1;             /* spatial separation */
        default_val[3] = 1;             /* LR relative level */
        default_val[4] = 0;             /* LOHI relative level */
        break;
    case(PSOW_INTERLEAVE):
        default_val[0] = 440.0;         /* pitch */
        default_val[1] = 440.0;         /* pitch */
        default_val[2] = 1.0;           /* fofs per chunk */
        default_val[3] = 0.0;           /* pitch biasing */
        default_val[4] = 1.0;           /* relative level */
        default_val[5] = 1.0;           /* weighting */
        break;
    case(PSOW_REPLACE):
        default_val[0] = 440.0;         /* pitch */
        default_val[1] = 440.0;         /* pitch */
        default_val[2] = 1.0;           /* fofs per chunk */
        break;
    case(PSOW_EXTEND):
        default_val[0] = 440.0;         /* pitch */
        default_val[PS_TIME]  = 0.0;            /* grabtime */
        default_val[PS_DUR]   = duration;   
        default_val[PS_SEGS]  = 1;
        default_val[PSE_VFRQ] = 6.5;
        default_val[PSE_VDEP] = 0.0;
        default_val[PSE_TRNS] = 0.0;
        default_val[PSE_GAIN] = 1.0;
        break;
    case(PSOW_EXTEND2):
        default_val[0] = 0.0;           /* pitch */
        default_val[1]  = 0.0;          /* grabtime */
        default_val[PS_DUR]   = duration;   
        default_val[PS2_VFRQ] = 6.5;
        default_val[PS2_VDEP] = 0.0;
        default_val[PS2_NUJ]  = 0.0;
        break;
    case(PSOW_LOCATE):
        default_val[0] = 440.0;         /* pitch */
        default_val[PS_TIME] = 0.0;     /* time */
        break;
    case(PSOW_CUT):
        default_val[0] = 440.0;         /* pitch */
        default_val[PS_TIME] = 0.0;     /* time */
        break;
    case(ONEFORM_GET):
        default_val[0]  = duration/2.0;
        break;
    case(ONEFORM_PUT):
        default_val[FORM_FTOP] = nyquist;
        default_val[FORM_FBOT] = PITCHZERO;
        default_val[FORM_GAIN] = 1.0;
        break;
    case(ONEFORM_COMBINE):
        break;
    case(NEWGATE):
        default_val[0] = -60.0;
        break;
    case(SPEC_REMOVE):
        default_val[0] = 60.0;
        default_val[1] = 60.0;
        default_val[2] = 6000.0;
        default_val[3] = 1.0;
        break;
    case(PREFIXSIL):
        default_val[0] = 0.0;
        break;
    case(STRANS):
        switch(mode) {
        case(0):
            default_val[VTRANS_TRANS] = 1.0;
            break;
        case(1):           
            default_val[VTRANS_TRANS] = 0.0;
            break;
        case(2):           
            default_val[ACCEL_ACCEL] = 1.0;
            default_val[ACCEL_GOALTIME] = min(1.0,duration);
            default_val[ACCEL_STARTTIME]= 0.0;
            break;
        case(3):           
            default_val[VIB_FRQ]    = DEFAULT_VIB_FRQ;
            default_val[VIB_DEPTH]  = DEFAULT_VIB_DEPTH;
            break;
        }
        break;
    case(PSOW_REINF):
        default_val[0] = 440.0;         /* pitch */
        if(mode == 0)
            default_val[1] = 0.0;
        else
            default_val[1] = 4.0;
        break;
    case(PARTIALS_HARM):  
        default_val[0] = CONCERT_A;
        default_val[1] = 0.01;
        if(mode > 1)
            default_val[2] = duration/2.0;
        break;
    case(SPECROSS):  
        default_val[PICH_RNGE]   = 1.0;
        default_val[PICH_VALID]  = (double)BLIPLEN;
        default_val[PICH_SRATIO] = SILENCE_RATIO;
        default_val[PICH_MATCH]  = (double)ACCEPTABLE_MATCH;
        default_val[PICH_HILM]   = nyquist/MAXIMI;
        default_val[PICH_LOLM]   = SPEC_MINFRQ;
        default_val[PICH_THRESH] = .02;
        default_val[SPCMPLEV]    = 1.0;
        default_val[SPECHINT]    = 1.0;
        break;
    case(LUCIER_GETF):
        default_val[LUCIER_CUT]         = MINPITCH;
        /* fall thro */
    case(LUCIER_GET):
        default_val[MIN_ROOM_DIMENSION] = 4.0;
        default_val[ROLLOFF_INTERVAL]   = 7.0;  /* 5th   */
        break;
    case(LUCIER_PUT):
        default_val[RESON_CNT]          = 1.0;
        default_val[RES_EXTEND_ATTEN]   = 0.0;
        break;
    case(LUCIER_DEL):
        default_val[SUPR_COEFF]         = 0.5;
        break;
    case(SPECTRACT):
    case(SPECLEAN):
        default_val[0]  = 8.0 * frametime * SECS_TO_MS;
        default_val[1]  = NEW_DEFAULT_NOISEGAIN;
        break;
    case(PHASE):
        if(mode == 1)
            default_val[0]  = 1.0;
        break;
    case(SPECSLICE):
        chwidth = nyquist/(double)channels;
        switch(mode) {
        case(0):
            default_val[0]  = 2;
            default_val[1]  = 1;
            break;
        case(1):
            default_val[0]  = 2;
            default_val[1]  = chwidth;
            break;
        case(2):
            default_val[0]  = 2;
            default_val[1]  = SEMITONES_PER_OCTAVE;
            break;
        case(4):
            default_val[0]  = nyquist/2.0;
            break;
        }
        break;
    case(FOFEX_CO):
        if(mode == FOF_MEASURE)
            break;
        default_val[0]  = 440.0;
        default_val[1]  = 1;
        default_val[2]  = 1;
        switch(mode) {
        case(FOF_SINGLE):
            default_val[3]  = 1;
            break;
        case(FOF_LOHI):
            default_val[3]  = 0;
            default_val[4]  = 0;
            default_val[5]  = 55;
            default_val[6]  = 880;
            break;
        case(FOF_TRIPLE):
            default_val[3]  = 0;
            default_val[4]  = 0;
            default_val[5]  = 0;
            default_val[6]  = 55;
            default_val[7]  = 880;
            default_val[8]  = 0;
            default_val[9]  = 1;
            break;
        }
        break;
    case(FOFEX_EX):
        default_val[0]  = 440.0;
        default_val[1]  = 0;
        default_val[2]  = 1;
        break;
    case(GREV_EXTEND):
        default_val[GREV_WSIZE]   = 5;
        default_val[GREV_TROFRAC] = .2;
        default_val[2]  = 1.0;
        default_val[3]  = 0.0;
        default_val[4]  = duration;
        break;
    case(PEAKFIND):
        default_val[0]  = 100;
        default_val[1] = 0.0;
        break;
    case(CONSTRICT):
        default_val[0]  = 50;
        break;
    case(EXPDECAY):
        default_val[0]  = 0.0;
        default_val[1] = duration;
        break;
    case(PEAKCHOP):
        default_val[PKCH_WSIZE]  = 50.0;
        default_val[PKCH_WIDTH]  = 2.0;
        default_val[PKCH_SPLICE] = 10.0;
        default_val[PKCH_GATE]   = 0.0;
        default_val[PKCH_SKEW]   = 0.25;
        if(mode == 0 || mode == 2) {
            default_val[PKCH_TEMPO]  = 120;
            default_val[PKCH_GAIN]   = 1.0;
            default_val[PKCH_SCAT]   = 0.0;
            default_val[PKCH_NORM]   = 0.0;
            default_val[PKCH_REPET]  = 0;
        }
        if(mode == 0)
            default_val[PKCH_MISS]   = 0;
        break;
    case(MCHANPAN):
        switch(mode) {
        case(9):
            default_val[3] = 1;
            /* fall thro */
        case(1):
            default_val[2] = SILMIN;
            /* fall thro */
        case(0):
            default_val[0]  = 8;
            default_val[1]  = 1.0;
            break;
        case(2):
            default_val[5]  = SILMIN;
            /* fall thro */
        case(3):
            default_val[0]  = 8;
            default_val[1]  = 1;
            default_val[2]  = 8;
            default_val[3]  = 4;
            default_val[4]  = 0.0;
            break;
        case(4):
            default_val[0]  = 8;
            default_val[1]  = SILMIN;
            break;
        case(5):
            default_val[0]  = 8;
            default_val[1]  = 0.5;
            default_val[2]  = 0.0;
            default_val[3]  = 15.0;
            break;
        case(6):
            default_val[0]  = 0.0;
            break;
        case(7):
            default_val[0]  = 1.0;
            default_val[1]  = 0.0;
            break;
        case(8):
            default_val[0]  = 8;
            default_val[1]  = 1;
            default_val[2]  = 1;
            default_val[3]  = 1;
            break;
        }
        break;
    case(TEX_MCHAN):
        default_val[TEXTURE_DUR]        = TEXTURE_DEFAULT_DUR;
        default_val[TEXTURE_PACK]       = DENSITY_DEFAULT;
        default_val[TEXTURE_SCAT]       = 0.0;
        default_val[TEXTURE_TGRID]      = 0.0;
        default_val[TEXTURE_INSLO]      = 1.0;
        default_val[TEXTURE_INSHI]      = 1.0;
        default_val[TEXTURE_MAXAMP]     = 64.0;
        default_val[TEXTURE_MINAMP]     = 64.0;
        default_val[TEXTURE_MAXDUR]     = min(ap->hi[TEXTURE_MAXDUR],TEXTURE_MAX_DUR);
        default_val[TEXTURE_MINDUR]     = max(ap->lo[TEXTURE_MINDUR],TEXTURE_MIN_DUR);
        default_val[TEXTURE_MAXPICH]    = DEFAULT_MIDI_PITCH;
        default_val[TEXTURE_MINPICH]    = DEFAULT_MIDI_PITCH;
        default_val[TEXTURE_OUTCHANS]   = 8;
        default_val[TEXTURE_ATTEN]      = 1.0;
        default_val[TEXTURE_POS]        = TEX_CENTRE;
        default_val[TEXTURE_SPRD]       = 1.0;
        default_val[TEXTURE_SEED]       = 0.0;
        break;
    case(MANYSIL):
        default_val[0]  = 15.0;
        break;
    case(RETIME):
        switch(mode) {
        case(0):
            default_val[MM] = 60.0;
            break;
        case(1):
            default_val[MM] = 60.0;
            default_val[RETIME_WIDTH]   = 40;
            default_val[RETIME_SPLICE]  = 20;
            break;
        case(2):       
            default_val[0] = (2.0/(double)srate) * SECS_TO_MS;
            default_val[1]  = 40;
            default_val[2]  = 10;
            default_val[3]  = 20;
            break;
        case(3):
            default_val[0]  = 60;
            default_val[1]  = (32.0/(double)srate) * SECS_TO_MS;
            default_val[2]  = 1.0;
            break;
        case(4):
            default_val[0]  = 1.0;
            default_val[1]  = (32.0/(double)srate) * SECS_TO_MS;
            default_val[2]  = 0.0;
            default_val[3]  = duration;
            default_val[4]  = 0.0;
            break;
        case(5):
            default_val[0]  = 60.0;
            default_val[1]  = 0.0;
            default_val[2] = (2.0/(double)srate) * SECS_TO_MS;
            default_val[3] = 1.0;
            break;
        case(6):
            default_val[0]  = 0.0;
            default_val[1] = (2.0/(double)srate) * SECS_TO_MS;
            default_val[2] = 1.0;
            break;
        case(7):
            default_val[MM]           = 60.0;
            default_val[BEAT_AT]      = 0.0;
            default_val[BEAT_CNT]     = 1.0;
            default_val[BEAT_REPEATS] = 1.0;
            default_val[BEAT_SILMIN] = (32.0/(double)srate) * SECS_TO_MS;
            break;
        case(8):
            default_val[0] = (2.0/(double)srate) * SECS_TO_MS;
            break;
        case(9):
            default_val[0] = (32.0/(double)srate) * SECS_TO_MS;
            default_val[1] = 0.5;
            default_val[2]  = 0;
            default_val[3]  = 1.0;
            break;
        case(10):
            default_val[0] = (32.0/(double)srate) * SECS_TO_MS;
            break;
        case(12):
            default_val[0]  = 0.0;
            break;
        case(13):
            default_val[0]  = 0.0;
            default_val[1]  = 0.0;
            break;
        }
        break;
    case(HOVER):
        default_val[0] = 440.0;
        default_val[1] = duration / 2.0;
        default_val[2] = 0.1;
        default_val[3] = 0.1;
        default_val[4] = 1.0;
        default_val[5] = 10.0;
        break;
    case(HOVER2):
        default_val[0] = 440.0;
        default_val[1] = duration / 2.0;
        default_val[2] = 0.1;
        default_val[3] = 0.1;
        default_val[4] = 10.0;
        break;
    case(MULTIMIX):
        switch(mode) {
        case(2):
            default_val[0]  = 0.0;
            break;
        case(3):
            default_val[0]  = 1.0;
            break;
        case(4):
            default_val[0]  = 1.0;
            default_val[1]  = 1.0;
            default_val[1]  = 1.0;
            default_val[3]  = 1.0;
            break;
        case(6):
            default_val[0]  = 8;
            default_val[1]  = 1;
            default_val[2]  = 1;
            default_val[3]  = 0;
            break;
        case(7):
            default_val[0]  = 8;
            break;
        }
        break;
    case(FRAME):
        switch(mode) {
        case(0):
            default_val[0]  = 1.0;
            default_val[1]  = 0.0;
            break;
        case(1):
            default_val[0]  = 1.0;
            default_val[1]  = -1.0;
            default_val[2]  = 0.0;
            break;
        case(2):
        case(4):
        case(7):
            break;
        case(3):
            default_val[0]  = 1.0;
            break;
        case(5):
            default_val[0]  = 1.0;
            default_val[1]  = 2.0;
            break;
        case(6):
            default_val[0]  = 0.0;
            break;
        }
        break;
    case(SEARCH):
        break;
    case(MCHANREV):
        default_val[STAD_PREGAIN] = STAD_PREGAIN_DFLT;
        default_val[STAD_ROLLOFF] = 1.0;
        default_val[STAD_SIZE]    = 1.0;
        default_val[STAD_ECHOCNT] = REASONABLE_ECHOCNT;
        default_val[REV_OCHANS]   = 2;
        default_val[REV_CENTRE]   = 1;
        default_val[REV_SPREAD]   = 2;
        break;
    case(WRAPPAGE):
        default_val[WRAP_OUTCHANS]  = 8.0;
        default_val[WRAP_SPREAD]    = 8.0;
        default_val[WRAP_DEPTH]     = 4.0;
        default_val[WRAP_VELOCITY]  = 1.0;
        default_val[WRAP_HVELOCITY] = 1.0;
        default_val[WRAP_DENSITY]   = WRAP_DEFAULT_DENSITY;
        default_val[WRAP_HDENSITY]  = WRAP_DEFAULT_DENSITY;
        default_val[WRAP_GRAINSIZE] = WRAP_DEFAULT_GRAINSIZE;
        default_val[WRAP_HGRAINSIZE]= WRAP_DEFAULT_GRAINSIZE;
        default_val[WRAP_PITCH]     = 0.0;
        default_val[WRAP_HPITCH]    = 0.0;
        default_val[WRAP_AMP]       = 1.0;
        default_val[WRAP_HAMP]      = 1.0;
        default_val[WRAP_BSPLICE]   = WRAP_DEFAULT_SPLICELEN;
        default_val[WRAP_HBSPLICE]  = WRAP_DEFAULT_SPLICELEN;
        default_val[WRAP_ESPLICE]   = WRAP_DEFAULT_SPLICELEN;
        default_val[WRAP_HESPLICE]  = WRAP_DEFAULT_SPLICELEN;
        default_val[WRAP_SRCHRANGE] = 0.0;
        default_val[WRAP_SCATTER]   = WRAP_DEFAULT_SCATTER;
        default_val[WRAP_OUTLEN]    = 0.0;
        default_val[WRAP_BUFXX]     = 0.0;
        break;
    case(MCHSTEREO):
        default_val[0] = 8;
        default_val[1] = 0.5;
        break;
    case(MTON):
        default_val[0] = 8;
        break;
    case(FLUTTER):
        default_val[0] = DEFAULT_VIB_FRQ;
        default_val[1] = 1.0;
        default_val[2] = 1.0;
        break;
    case(ABFPAN):
        default_val[0] = 0.0;
        default_val[1] = 1.0;
        default_val[2] = 4;
        break;
    case(ABFPAN2):
        default_val[0] = 0.0;
        default_val[1] = 1.0;
        default_val[2] = 1.0;
        break;
    case(ABFPAN2P):
        default_val[0] = 0.0;
        default_val[1] = 1.0;
        default_val[2] = 1.0;
        default_val[3] = 0.0;
        break;
    case(CHANNELX):
        break;
    case(CHORDER):
        break;
    case(FMDCODE):
        default_val[0] = 1.0;
        break;
    case(CHXFORMAT):
        default_val[0] = 0.0;
        break;
    case(CHXFORMATG):
    case(CHXFORMATM):
        break;
    case(INTERLX):
        default_val[0] = 0.0;
        break;
    case(COPYSFX):
        default_val[0] = 0;
        default_val[1] = -1;
        break;
    case(NJOIN):
        default_val[0] = -4;
        break;
    case(NJOINCH):
        break;
    case(NMIX):
        default_val[0] = 0.0;
        break;
    case(RMSINFO):
        default_val[0] = 0;
        default_val[1] = duration;
        break;
    case(SFEXPROPS):
        break;
    case(SETHARES):
        chwidth = nyquist/(double)((wanted/2)-1);
        default_val[0] = 12;
        default_val[1] = 1.5;
        default_val[2] = .001;
        default_val[3] = chwidth;
        default_val[4] = nyquist;
        default_val[5] = 1.0;
        break;
    case(MCHSHRED):
        default_val[0] = 1;
        default_val[1] = max((duration/8.0),((double)(MSHR_SPLICELEN * 3)/(double)srate));
        default_val[2] = 1.0;
        if(mode == 0)
            default_val[3]  = 8;
        break;
    case(MCHZIG):
        default_val[MZIG_START] = 0.0;
        default_val[MZIG_END]  = duration;
        default_val[MZIG_DUR] = duration * 2.0;
        default_val[MZIG_MIN] = ((MZIG_SPLICELEN * 2) + MZIG_MIN_UNSPLICED) * MS_TO_SECS;
        default_val[MZIG_OCHANS]      = 8;
        default_val[MZIG_SPLEN] = MZIG_SPLICELEN;
        if(mode==0) {
            default_val[MZIG_MAX] = min(2.0,duration - (2 * MZIG_SPLICELEN * MS_TO_SECS));
            default_val[MZIG_RSEED] = 0.0;
        }
        break;
    case(MCHITER):
        default_val[MITER_OCHANS] = 8;
        switch(mode) {
        case(0):
            default_val[MITER_DUR] = duration * 2.0;
            break;
        case(1):
            default_val[MITER_REPEATS] = 2.0;
            break;
        }
        default_val[MITER_DELAY] = duration;
        default_val[MITER_RANDOM]= 0.0;
        default_val[MITER_PSCAT] = 0.0;
        default_val[MITER_ASCAT] = 0.0;
        default_val[MITER_FADE]  = 0.0;
        default_val[MITER_GAIN]  = 1.0; /* 0.0 */ 
        default_val[MITER_RSEED] = 0.0;
        break;
    case(SPECSPHINX):
        switch(mode) {
        case(0):
            default_val[0] = 0.0;
            default_val[1] = 0.0;
            break;
        case(1):
            default_val[0] = 0.0;
            default_val[1] = 1.0;
            break;
        case(2):
            default_val[0]  = 1.0;
            default_val[1]  = 1.0;
            default_val[2]  = nyquist;
            break;
        }
        break;
    case(SPECMORPH):
        if(mode == 6) {
            default_val[NMPH_APKS] = 8;
            default_val[NMPH_OCNT] = 6;
        } else {
            default_val[NMPH_STAG] = 0.0;
            default_val[NMPH_ASTT] = 0.0;
            default_val[NMPH_AEND] = duration;
            default_val[NMPH_AEXP] = 1.0;
            default_val[NMPH_APKS] = 8;
            if(mode >= 4)
                default_val[NMPH_RAND] = 0;
        }
        break;
    case(SPECMORPH2):
        default_val[NMPH_APKS] = 8;
        if(mode > 0) {
            default_val[NMPH_ASTT] = 0.0;
            default_val[NMPH_AEND] = duration;
            default_val[NMPH_AEXP] = 1.0;
            default_val[NMPH_RAND] = 0;
        }
        break;
    case(SUPERACCU):
        default_val[0] = 0.01;
        default_val[1] = 0;
        break;
    case(PARTITION):
        default_val[0] = 8;
        switch(mode) {
        case(0):
            default_val[1] = 1;
            break;
        case(1):
            default_val[1] = 0.1;
            default_val[2] = 0.0;
            default_val[3] = 3.0;
            break;
        }
        break;
    case(SPECGRIDS):
        default_val[0]  = 2.0;
        default_val[1]  = 1.0;
        break;
    case(GLISTEN):
        default_val[0]  = 16;
        default_val[1]  = 64;
        default_val[2]  = 0;
        default_val[3]  = 0;
        default_val[4]  = 0;
        break;
    case(TUNEVARY):
        default_val[0]   = 1.0;
        default_val[1]  = 1.0;
        default_val[2]  = DEFAULT_TRACE;
        default_val[3]  = SPEC_MINFRQ;
        break;
    case(ISOLATE):
        switch(mode) {
        case(ISO_OVRLAP):
            default_val[ISO_SPL] = 15;
            default_val[ISO_DOV] = 5;
            break;
        case(ISO_THRESH):
            default_val[ISO_THRON]  = 0;
            default_val[ISO_THROFF] = -96;
            default_val[ISO_SPL]    = 15;
            default_val[ISO_MIN]    = 50;
            default_val[ISO_LEN]    = 0;
            break;
        default:
            default_val[ISO_SPL] = 15;
            break;
        }
        break;
    case(REJOIN):
        default_val[0]  = 1;
        break;
    case(PANORAMA):
        if(mode == 0) {
            default_val[PANO_LCNT] = 8;
            default_val[PANO_LWID] = 360;
        }
        default_val[PANO_SPRD]     = 360;
        default_val[PANO_OFST]     = 0;
        default_val[PANO_CNFG]     = 1;
        default_val[PANO_RAND]     = 0;
        break;
    case(TREMOLO):
        default_val[TREMOLO_FRQ] = 15;
        default_val[TREMOLO_DEP] = 1.0;
        default_val[TREMOLO_AMP] = 1.0;
        default_val[TREMOLO_SQZ] = 1.0;
        break;
    case(ECHO):
        default_val[ECHO_DELAY] = duration;
        default_val[ECHO_ATTEN] = 0.5;
        default_val[ECHO_DUR]   = duration * 2;
        default_val[ECHO_RAND]  = 0;
        default_val[ECHO_CUT]   = -96;
        break;
    case(PACKET):
        default_val[PAK_DUR] = 50.0;
        default_val[PAK_SQZ] = 1.0;
        default_val[PAK_CTR] = 0.0;
        break;
    case(SYNTHESIZER):
        default_val[SYNTHSRAT] = 44100.0;
        default_val[SYNTH_DUR] = 1.0;
        default_val[SYNTH_FRQ] = 440;
        if(mode == 1) {
            default_val[SYNTH_SQZ] = 1.0;
            default_val[SYNTH_CTR] = 0.0;
        } else if(mode == 2) {
            default_val[SYNTH_CHANS] = 1.0;
            default_val[SYNTH_MAX]   = 3.0;
            default_val[SYNTH_RATE]  = 0.1;
            default_val[SYNTH_RISE]  = 0;
            default_val[SYNTH_FALL]  = 0;
            default_val[SYNTH_STDY]  = 0;
            default_val[SYNTH_SPLEN] = 5;
            default_val[SYNTH_NUM] = 0;
            default_val[SYNTH_EFROM] = 0;
            default_val[SYNTH_ETIME] = 0;
            default_val[SYNTH_CTO] = 0;
            default_val[SYNTH_CTIME] = 0;
            default_val[SYNTH_STYPE] = 0;
            default_val[SYNTH_RSPEED] = 0;
        } else if(mode == 3) {
            default_val[SYNTH_ATK]    = 2;
            default_val[SYNTH_EATK]   = 2;
            default_val[SYNTH_DEC]    = 2;
            default_val[SYNTH_EDEC]   = 2;
            default_val[SYNTH_ATOH]   = .5;
            default_val[SYNTH_GTOW]   = 1;
            default_val[SYNTH_RAND]   = 0;
            default_val[SYNTH_FLEVEL] = 0;
        }
        break;
    case(NEWTEX):
        default_val[NTEX_DUR]    = 1.0;
        default_val[NTEX_CHANS]  = 1.0;
        default_val[NTEX_MAX]    = 3.0;
        default_val[NTEX_RATE]   = 0.1;
        default_val[NTEX_SPLEN]  = 5;
        default_val[NTEX_NUM]    = 1;
        switch(mode) {
        case(1):
            default_val[NTEX_DEL]= 0;
            break;
        case(2):
            default_val[NTEX_LOC] = 0;
            default_val[NTEX_AMB] = 0;
            default_val[NTEX_GST] = 0;
            break;
        }
        default_val[NTEX_EFROM]  = 0;
        default_val[NTEX_ETIME]  = 0;
        default_val[NTEX_CTO]    = 0;
        default_val[NTEX_CTIME]  = 0;
        default_val[NTEX_STYPE]  = 0;
        default_val[NTEX_RSPEED] = 0;
        break;
    case(CERACU):
        default_val[CER_MINDUR] = duration;
        default_val[CER_OCHANS] = 8;
        default_val[CER_CUTOFF] = 60;
        default_val[CER_DELAY]  = 0;
        default_val[CER_DSTEP]  = 1;
        break;
    case(MADRID):
        default_val[MAD_DUR]   = 20;
        default_val[MAD_CHANS] = 8;
        default_val[MAD_STRMS] = 3;
        default_val[MAD_DELF]  = 0.5;
        default_val[MAD_STEP]  = 0.2;
        default_val[MAD_RAND]  = 0;
        default_val[MAD_SEED]  = 0;
        break;
    case(SHIFTER):
        default_val[SHF_CYCDUR] = 2;
        default_val[SHF_OUTDUR] = 8;
        default_val[SHF_OCHANS] = 2;
        default_val[SHF_SUBDIV] = 16;
        default_val[SHF_LINGER] = 2;
        default_val[SHF_TRNSIT] = 2;
        default_val[SHF_LBOOST] = 1.0;
        break;
    case(SUBTRACT):
        default_val[0] = 1;
        break;
    case(SPEKLINE):
        if(mode == 0) {
            default_val[SPEKPOINTS] = 2048;
            default_val[SPEKHARMS]  = 0;
            default_val[SPEKBRITE]  = 0;
            default_val[SPEKMAX]    = 1;
        }
        default_val[SPEKSRATE]  = 44100;
        default_val[SPEKDUR]    = 10;
        default_val[SPEKDATLO]  = 0;
        default_val[SPEKDATHI]  = 22050;
        default_val[SPEKSPKLO]  = 0;
        default_val[SPEKSPKHI]  = 22050;
        default_val[SPEKWARP]   = 1;
        default_val[SPEKAWARP]  = 1;
        break;
    case(FRACTURE):
        if(mode == 0)
            default_val[FRAC_CHANS] = 2;
        else
            default_val[FRAC_CHANS] = 8;
        default_val[FRAC_STRMS]  = 16;
        default_val[FRAC_PULSE]  = 1;
        default_val[FRAC_DEPTH]  = 1;
        default_val[FRAC_STACK]  = 0;
        default_val[FRAC_INRND]  = 0;
        default_val[FRAC_OUTRND] = 0;
        default_val[FRAC_SCAT]   = 0;
        default_val[FRAC_LEVRND] = 0;
        default_val[FRAC_ENVRND] = 0;
        default_val[FRAC_STKRND] = 0;
        default_val[FRAC_PCHRND] = 0;
        default_val[FRAC_SEED]   = 0;
        default_val[FRAC_MIN]    = 0;
        default_val[FRAC_MAX]    = 0;
        if(mode > 0) {
            default_val[FRAC_CENTRE]  = 1;
            default_val[FRAC_FRONT]   = 0;
            default_val[FRAC_MDEPTH]  = 0.5;
            default_val[FRAC_ROLLOFF] = 0.0;
            default_val[FRAC_ATTEN]   = 3.0;
            default_val[FRAC_ZPOINT]   = 0.66;
            default_val[FRAC_CONTRACT] = 0.66;
            default_val[FRAC_FPOINT]   = 0.66;
            default_val[FRAC_FFACTOR]  = 0.66;
            default_val[FRAC_FFREQ]    = 500.0;
            default_val[FRAC_UP]       = 0.0;
            default_val[FRAC_DN]       = 0.0;
            default_val[FRAC_GAIN]     = 1.0;
        }
        break;
    case(TAN_ONE):
        default_val[TAN_DUR]   = 8;
        default_val[TAN_STEPS] = 24;
        if(mode==0) {
            default_val[TAN_MANG] = 130;
            default_val[TAN_SLOW] = 0.5;
        } else
            default_val[TAN_SKEW] = .75;
        default_val[TAN_DEC]    = .9;
        default_val[TAN_FOCUS]  = 1;
        default_val[TAN_JITTER] = 0;
        break;
    case(TAN_TWO):
        default_val[TAN_DUR]   = 8;
        default_val[TAN_STEPS] = 24;
        if(mode==0) {
            default_val[TAN_MANG] = 130;
            default_val[TAN_SLOW] = 0.5;
        } else
            default_val[TAN_SKEW] = .75;
        default_val[TAN_DEC]    = .5;
        default_val[TAN_FBAL]   = .9;
        default_val[TAN_FOCUS]  = 1;
        default_val[TAN_JITTER] = 0;
        break;
    case(TAN_SEQ):
    case(TAN_LIST):
        default_val[TAN_DUR] = 8;
        if(mode==0) {
            default_val[TAN_MANG] = 130;
            default_val[TAN_SLOW] = 0.5;
        } else
            default_val[TAN_SKEW] = .75;
        default_val[TAN_DEC]    = .5;
        default_val[TAN_FOCUS]  = 1;
        default_val[TAN_JITTER] = 0;
        break;
    case(SPECTWIN):
        default_val[0] = 1;
        default_val[1] = 1;
        default_val[2] = 0;
        default_val[3] = 12;
        default_val[4] = 1;
        break;
    case(TRANSIT):
    case(TRANSITF):
    case(TRANSITD):
    case(TRANSITFD):
    case(TRANSITS):
    case(TRANSITL):
        if(EVEN(mode))
            default_val[TRAN_FOCUS] = 1;
        else
            default_val[TRAN_FOCUS] = 1.5;
        default_val[TRAN_DUR]       = 8;
        if(process < TRANSITS)
            default_val[TRAN_STEPS] = 24;
        switch(mode) {
        case(GLANCING):
        case(EDGEWISE):
        case(CROSSING):
        case(CLOSE):
            default_val[TRAN_MAXA]  = 85.0;
            break;
        case(CENTRAL):
            default_val[TRAN_MAXA]  = 10;
            break;
        }
        default_val[TRAN_DEC]       = 0.9;
        if(process == TRANSITF || process == TRANSITFD)
            default_val[TRAN_FBAL]  = 0.9;
        if(process < TRANSITS) {
            default_val[TRAN_THRESH]    = 0.0;
            default_val[TRAN_DECLIM]    = 0.0;
            default_val[TRAN_MINLEV]    = 0.0;
            default_val[TRAN_MAXDUR]    = 0.0;
        }
        break;
    case(CANTOR):
        switch(mode) {
        case(0):
            default_val[CA_HOLEN]   = 0.333;
            default_val[CA_HOLEDIG] = 0.1;
            default_val[CA_TRIGLEV] = 0.5;
            default_val[CA_SPLEN] = 5;
            break;
        case(1):
            default_val[CA_HOLEN] = duration/3.0;
            default_val[CA_HOLEDIG] = 0.1;
            default_val[CA_TRIGLEV] = 0.5;
            default_val[CA_SPLEN] = 5;
            break;
        case(2):
            default_val[CA_HOLEN] = 0.0;
            default_val[CA_HOLEDIG] = 8;
            default_val[CA_WOBDEC]  = 0.5;
            default_val[CA_WOBBLES] = 4;
        }
        default_val[CA_MAXDUR]  = 60.0;
        break;
    case(SHRINK):
        if(mode == SHRM_TIMED)
            default_val[SHR_TIME]  = duration/2.0;
        default_val[SHR_INK]       = 0.9;
        if(mode >= SHRM_FINDMX) {
            default_val[SHR_WSIZE] = 50;
            default_val[SHR_AFTER] = 0.0;
        } else {
            default_val[SHR_GAP]   = duration * 2.0;
            default_val[SHR_DUR]   = duration * 8;
        }
        default_val[SHR_CNTRCT] = 1;
        default_val[SHR_SPLEN]  = 5;
        default_val[SHR_SMALL]  = 0.0;
        default_val[SHR_MIN]    = 0.0;
        default_val[SHR_RAND]   = 0.0;
        if(mode >= SHRM_FINDMX) {
            default_val[SHR_GATE] = 0;
            default_val[SHR_LEN]  = 0;
        }
        if(mode == SHRM_FINDMX)
            default_val[SHR_SKEW] = 0.25;
        break;
    case(NEWDELAY):
        if(mode==0) {
            default_val[DELAY_DELAY] = 30;
            default_val[DELAY_MIX] = 0.5;
            default_val[DELAY_FEEDBACK] = 0.7;
        } else {
            default_val[0] = 30;
            default_val[1] = duration/2.0;
            default_val[2] = 4;
            default_val[3] = 0.0;
            default_val[4] = 1;
            default_val[5] = 0.25;
        }
        break;
    case(ITERLINE):
    case(ITERLINEF):
        default_val[ITER_DUR]       = duration * 2.0;
        default_val[ITER_DELAY]     = duration;
        default_val[ITER_RANDOM]    = 0.0;
        default_val[ITER_PSCAT]     = 0.0;
        default_val[ITER_ASCAT]     = 0.0;
        default_val[ITER_RSEED]     = 0.0;
        default_val[ITER_GAIN]      = DEFAULT_ITER_GAIN; /* 0.0 */
        break;
    case(SPECRAND):
        default_val[0]  = duration;
        default_val[1]  = 1;
        break;
    case(SPECSQZ):
        default_val[0]  = 440.0;
        default_val[1]  = 0.5;
        break;
    case(FILTRAGE):
        default_val[FILTR_DUR] = 10;
        default_val[FILTR_CNT] = 64;
        default_val[FILTR_MMIN] = 0;
        default_val[FILTR_MMAX] = 0;
        default_val[FILTR_DIS] = 1;
        default_val[FILTR_RND] = 0;
        default_val[FILTR_AMIN] = .1;
        default_val[FILTR_ARND] = 0;
        default_val[FILTR_ADIS] = 0;
        if(mode == 1) {
            default_val[FILTR_STEP] = 1;
            default_val[FILTR_SRND] = 0;
        }
        default_val[FILTR_SEED] = 0;
        break;
    case(SELFSIM):
        default_val[0]  = 1;
        break;
    case(ITERFOF):
        if(mode <2)
            default_val[ITF_DEL] = 0;
        else
            default_val[ITF_DEL] = 60;
        default_val[ITF_DUR] = 20;  
        default_val[ITF_PRND] = 0.0;
        default_val[ITF_AMPC] = 0.0;
        default_val[ITF_TRIM] = 0.0;
        default_val[ITF_TRBY] = 0.0;
        default_val[ITF_SLOP] = 1.0;
        default_val[ITF_RAND] = 0.0;
        default_val[ITF_VMIN] = 0.0;
        default_val[ITF_VMAX] = 0.0;
        default_val[ITF_DMIN] = 0.0;
        default_val[ITF_DMAX] = 0.0;
        if(EVEN(mode))
            default_val[ITF_SEED1] = 0.0;
        else {
            default_val[ITF_GMIN] = 1.0;
            default_val[ITF_GMAX] = 1.0;
            default_val[ITF_UFAD] = 0.0;
            default_val[ITF_FADE] = 0.0;
            default_val[ITF_GAPP] = 0.0;
            default_val[ITF_PORT] = 0.0;
            default_val[ITF_PINT] = 0.0;
            default_val[ITF_SEED2] = 0.0;
        }
        break;
    case(PULSER):
    case(PULSER2):
    case(PULSER3):
        default_val[PLS_DUR]      = 4;
        if(process == PULSER3 || mode == 0)
            default_val[PLS_PITCH] = 60;
        default_val[PLS_MINRISE]  = 0.02;
        default_val[PLS_MAXRISE]  = 0.02;
        default_val[PLS_MINSUS]   = 0.0;
        default_val[PLS_MAXSUS]   = 0.0;
        default_val[PLS_MINDECAY] = 0.05;
        default_val[PLS_MAXDECAY] = 0.05;
        default_val[PLS_SPEED]    = 0.1;
        default_val[PLS_SCAT]     = 0.0;
        default_val[PLS_EXP]      = 1.0;
        default_val[PLS_EXP2]     = 1.0;
        default_val[PLS_PSCAT]    = 0;
        default_val[PLS_ASCAT]    = 0;
        default_val[PLS_OCT]      = 0;
        default_val[PLS_BEND]     = 0;
        default_val[PLS_SEED]     = 0.0;
        if(process == PULSER3) {
            default_val[PLS_SRATE]  = 44100;
            default_val[PLS_CNT]    = 0;
        } else if(mode == 2)
            default_val[PLS_WIDTH]  = 0;
        break;
    case(CHIRIKOV):
        default_val[CHIR_DUR]  = 1.0;
        default_val[CHIR_FRQ]  = 440;
        default_val[CHIR_DAMP] = 0;
        if(mode < 2) {
            default_val[CHIR_SRATE] = 44100;
            default_val[CHIR_SPLEN] = 50;
        } else {
            default_val[CHIR_PMIN]  = 60;
            default_val[CHIR_PMAX]  = 60;
            default_val[CHIR_STEP]  = 0.1;
            default_val[CHIR_RAND]  = 0;
        }
        break;
    case(MULTIOSC):
        default_val[MOSC_DUR]   = 1.0;
        default_val[MOSC_FRQ1] = 440;
        default_val[MOSC_FRQ2] = 440;
        default_val[MOSC_AMP2] = 0;
        if(mode >= 1) {
            default_val[MOSC_FRQ3] = 440;
            default_val[MOSC_AMP3] = 0;
        }
        if(mode == 2) {
            default_val[MOSC_FRQ4] = 440;
            default_val[MOSC_AMP4] = 0;
        }
        default_val[MOSC_SRATE] = 44100;
        default_val[MOSC_SPLEN] = 50;
        break;
    case(SYNFILT):
        default_val[SYNFLT_SRATE]   = 44100.0;
        default_val[SYNFLT_CHANS]   = 1;
        default_val[SYNFLT_Q]       = FLT_DEFAULT_Q;
        default_val[SYNFLT_HARMCNT] = FLT_DEFAULT_HCNT;
        default_val[SYNFLT_ROLLOFF] = FLT_DEFAULT_ROLLOFF;
        default_val[SYNFLT_SEED]    = 0;
        break;
    case(STRANDS):
        default_val[STRAND_DUR]   = 10.0;
        default_val[STRAND_BANDS] = 4;
        if(mode != 2)
            default_val[STRAND_THRDS] = 3;
        default_val[STRAND_TSTEP] = 100;
        default_val[STRAND_BOT]   = 36;
        default_val[STRAND_TOP]   = 84;
        default_val[STRAND_TWIST] = 3;
        default_val[STRAND_RAND]  = 0;
        default_val[STRAND_SCAT]  = 0;
        default_val[STRAND_VAMP]  = 0;
        default_val[STRAND_VMIN]  = 0;
        default_val[STRAND_VMAX]  = 0;
        default_val[STRAND_TURB]  = 0;
        default_val[STRAND_SEED]  = 0;
        default_val[STRAND_GAP]   = 0;
        default_val[STRAND_MINB]  = 12;
        default_val[STRAND_3D]    = 0;
        break;
    case(REFOCUS):
        default_val[REFOC_DUR]    = 10.0;
        default_val[REFOC_BANDS]  = 8;
        default_val[REFOC_RATIO]  = 2;
        default_val[REFOC_TSTEP]  = 1;
        default_val[REFOC_RAND]   = 0;
        default_val[REFOC_OFFSET] = 0;
        default_val[REFOC_END]    = 0;
        default_val[REFOC_XCPT]   = 0;
        default_val[REFOC_SEED]   = 0;
        break;
    case(CHANPHASE):
        default_val[0]   = 1;
        break;
    case(SILEND):
        if(mode == 0)
            default_val[0]   = 1.0;
        else
            default_val[0]   = duration + 1.0;
        break;
    case(SPECULATE):
        default_val[0]   = 0.0;
        default_val[1]   = nyquist;
        break;
    case(SPECTUNE):
        default_val[ST_MATCH]   = ST_ACCEPTABLE_MATCH;
        default_val[ST_LOPCH]   = 4;
        default_val[ST_HIPCH]   = 127;
        default_val[ST_STIME]   = 0;
        default_val[ST_ETIME]   = duration;
        default_val[ST_INTUN]   = 1;
        default_val[ST_WNDWS]   = BLIPLEN;
        default_val[ST_NOISE]   = SILENCE_RATIO;
        break;
    case(REPAIR):
        default_val[0]   = 2;
        break;
    case(DISTSHIFT):
        default_val[0] = 1;
        if(mode==0)
            default_val[1] = 1;
        break;
    case(QUIRK):
        default_val[0] = 1;
        break;
    case(ROTOR):
        default_val[ROT_CNT] = 7;
        default_val[ROT_PMIN] = 48;
        default_val[ROT_PMAX] = 72;
        default_val[ROT_NSTEP] = .1;
        default_val[ROT_PCYC] = 16;
        default_val[ROT_TCYC] = 16;
        default_val[ROT_PHAS] = 0;
        default_val[ROT_DUR] = 20;
        if(mode == 0)
            default_val[ROT_GSTEP] = 4;
        default_val[ROT_DOVE] = 0;
        break;
    case(DISTCUT):
        default_val[DCUT_CNT]   = 64;
        if(mode==1)
            default_val[DCUT_STP] = 64;
        default_val[DCUT_EXP] = 1.0;
        default_val[DCUT_LIM] = 40;
        break;
    case(ENVCUT):
        default_val[ECUT_CNT] = duration/4.0;
        if(mode==1)
            default_val[ECUT_STP] = duration/4.0;
        default_val[ECUT_ATT] = 1;
        default_val[ECUT_EXP] = 1.0;
        default_val[ECUT_LIM] = 40;
        break;
    case(SPECFOLD):
        default_val[0]  = 1;
        default_val[1]  = wanted/4;
        switch(mode) {
        case(0):
            default_val[2]  = 1;
            break;
        case(2):
            default_val[2]  = 1;
            break;
        }
        break;
    case(BROWNIAN):
        default_val[BRCHANS] = 8;
        default_val[BRDUR]   = 20;
        if(mode == 0) {
            default_val[BRATT] = .02;
            default_val[BRDEC] = .5;
        }
        default_val[BRPLO]   = 48;
        default_val[BRPHI]   = 72;
        default_val[BRPSTT]  = 60;
        default_val[BRSSTT]  = 1;
        default_val[BRPSTEP] = .5;
        default_val[BRSSTEP] = .0625;
        default_val[BRTICK]  = 0.04;
        default_val[BRSEED]  = 1;
        default_val[BRASTEP] = 0;
        default_val[BRAMIN]  = 0;
        if(mode == 0) {
            default_val[BRASLP] = 1;
            default_val[BRDSLP] = 1;
        }
        break;
    case(SPIN):
        default_val[SPNRATE]  = 1;
        default_val[SPNBOOST] = 2;
        default_val[SPNATTEN] = 0;
        if(mode > 0) {
            default_val[SPNOCHNS] = 8;
            default_val[SPNOCNTR] = 1;
            default_val[SPNCMIN]  = 0.0;
            if(mode==1)
                default_val[SPNCMAX]  = 0.5;
        }
        default_val[SPNDOPL]  = 0;
        default_val[SPNXBUF]  = 1;
        break;
    case(SPINQ):
        default_val[SPNRATE]  = 1;
        default_val[SPNBOOST] = 2;
        default_val[SPNATTEN] = 0;
        default_val[SPNOCHNS] = 8;
        default_val[SPNOCNTR] = 1;
        default_val[SPNDOPL]  = 0;
        default_val[SPNXBUF]  = 1;
        default_val[SPNCMIN]  = 0.0;
        if(mode == 0)
            default_val[SPNCMAX]  = 0.5;
        break;
    case(CRUMBLE):
        default_val[CRSTART]  = 0;
        default_val[CRSTEP1]  = 0;
        default_val[CRSTEP2]  = 0;
        if(mode == 1)
            default_val[CRSTEP3] = 0;
        default_val[CRORIENT] = 1;
        default_val[CRSIZE]   = min(.25,duration/2.0);
        default_val[CRRAND]   = 0;
        default_val[CRISCAT]  = 0;
        default_val[CROSCAT]  = 0;
        default_val[CROSTR]   = 1;
        default_val[CRPSCAT]  = 0;
        default_val[CRSEED]   = 1;
        default_val[CRSPLICE] = 5;
        default_val[CRTAIL]   = 0;
        default_val[CRDUR]    = 0;
        break;
    case(PHASOR):
        default_val[PHASOR_STREAMS] = 2;
        default_val[PHASOR_FRQ] = 1;
        default_val[PHASOR_SHIFT] = .5;
        default_val[PHASOR_OCHANS] = 1;
        default_val[PHASOR_OFFSET] = 0;
        break;
    case(TESSELATE):
        default_val[TESS_CHANS] = 8;
        default_val[TESS_PHRAS] = 1.0;
        default_val[TESS_DUR]   = 60;
        default_val[TESS_TYP]   = 0;
        break;
    case(CRYSTAL):
        default_val[CRY_ROTA]   = 0.1;
        default_val[CRY_ROTB]   = 0.1;
        default_val[CRY_TWIDTH] = 1;
        default_val[CRY_TSTEP]  = 1;
        default_val[CRY_DUR]    = 20;
        default_val[CRY_PLO]    = 36;
        default_val[CRY_PHI]    = 72;
        default_val[CRY_FPASS]  = CRY_PASSBAND;
        default_val[CRY_FSTOP]  = CRY_STOPBAND;
        default_val[CRY_FATT]   = CRY_FATT_DFLT;
        default_val[CRY_FPRESC] = CRY_FPRESC_DFLT;
        default_val[CRY_FSLOPE] = CRYS_DEPTH_ATTEN;
        default_val[CRY_SSLOPE] = CRYS_PROX_ATTEN;
        break;
    case(WAVEFORM):
        default_val[WF_TIME] = 0.0;
        if(mode == 0)
            default_val[WF_CNT]  = 1;
        else
            default_val[WF_DUR]  = 20;
        if(mode == 2)
            default_val[WF_BAL]  = 0.3;
        break;
    case(DVDWIND):
        default_val[0]  = 2;
        default_val[1]  = 500;
        break;
    case(CASCADE):
        if(mode < 5) {
            default_val[CAS_CLIP]    = .5;
            default_val[CAS_MAXCLIP] = 0;
        }
        default_val[CAS_ECHO]    = 8;
        default_val[CAS_MAXECHO] = 0;
        default_val[CAS_RAND]    = 0;
        default_val[CAS_SEED]    = 1;
        default_val[CAS_SHREDNO] = 0;
        default_val[CAS_SHREDCNT]= 0;
        break;
    case(SYNSPLINE):
        default_val[SPLIN_SRATE] = 44100.0;
        default_val[SPLIN_DUR] = 4.0;
        default_val[SPLIN_FRQ] = 440;
        default_val[SPLIN_CNT] = 2;
        default_val[SPLIN_INTP] = 24;
        default_val[SPLIN_SEED] = 1;
        default_val[SPLIN_MCNT] = 0;
        default_val[SPLIN_MINTP] = 0;
        default_val[SPLIN_DRIFT] = 0;
        default_val[SPLIN_DRVEL] = 0;
        break;
    case(SPLINTER):
        default_val[SPL_TIME]   = 0.0;
        default_val[SPL_WCNT]   = 1;
        default_val[SPL_SHRCNT] = SHRCNT_DFLT;
        default_val[SPL_OCNT]   = OCNT_DFLT;
        default_val[SPL_PULS1]  = PULS_DFLT;
        default_val[SPL_PULS2]  = PULS_DFLT;
        default_val[SPL_ECNT]   = 0;
        default_val[SPL_SCURVE] = 1;
        default_val[SPL_PCURVE] = 1;
        if(mode <= 1)
            default_val[SPL_FRQ]= FREQ_DFLT;
        else
            default_val[SPL_DUR]= 5.0;
        default_val[SPL_RND]    = 0;
        default_val[SPL_SHRND]  = 0;
        break;
    case(REPEATER):
        if(mode >= 2) {
            default_val[REP_ACCEL]  = 2.0;
            default_val[REP_WARP]   = 0.66;
            default_val[REP_FADE]   = 0.33;
        }
        default_val[REP_RAND]   = 1.0;
        default_val[REP_TRNSP]  = 0.0;
        default_val[REP_SEED]   = 0;
        break;
    case(VERGES):
        default_val[VRG_TRNSP] = VRG_DFLT_TRNSP;
        default_val[VRG_CURVE] = VRG_DFLT_CURVE;
        default_val[VRG_DUR]   = VRG_DFLT_DUR;
        break;
    case(MOTOR):
        default_val[MOT_DUR]    = 20.0;
        default_val[MOT_FRQ]    = MOT_FRQ_DFLT;
        default_val[MOT_PULSE]  = 1.0;
        default_val[MOT_FRATIO] = 0.5;
        default_val[MOT_PRATIO] = 1.0;
        default_val[MOT_SYM]    = 0.5;
        default_val[MOT_FRND]   = 0.0;
        default_val[MOT_PRND]   = 0.0;
        default_val[MOT_JIT]    = 0.0;
        default_val[MOT_TREM]   = 0.0;
        default_val[MOT_SYMRND] = 0.0;
        default_val[MOT_EDGE]   = 0;
        default_val[MOT_BITE]   = 3.0;
        default_val[MOT_VARY]   = 0.0;
        default_val[MOT_SEED]   = 0;
        break;
    case(STUTTER):
        default_val[STUT_DUR] = 20;
        default_val[STUT_JOIN] = 1;
        default_val[STUT_SIL]  = 0;
        default_val[STUT_SILMIN] = 0.1;
        default_val[STUT_SILMAX] = .5;
        default_val[STUT_SEED] = 1;
        default_val[STUT_TRANS] = 0;
        default_val[STUT_ATTEN] = 0;
        default_val[STUT_BIAS] = 0.0;
        default_val[STUT_MINDUR] = STUT_MIN;
        break;
    case(SCRUNCH):
        if(mode <= 1)
            default_val[SCR_DUR]  = 20.0;
        default_val[SCR_SEED] = 1;
        default_val[SCR_CNT]   = 1.0;
        default_val[SCR_TRNS]  = 0.0;
        default_val[SCR_ATTEN] = 0.0;
        break;
    case(IMPULSE):
        default_val[IMP_DUR]   = 0;
        default_val[IMP_PICH]  = 2.0;
        default_val[IMP_CHIRP] = 0;
        default_val[IMP_SLOPE] = 1.0;
        default_val[IMP_CYCS]  = 1;
        default_val[IMP_LEV]   = 1;
        default_val[IMP_GAP]   = 0;
        default_val[IMP_SRATE] = 44100;
        default_val[IMP_CHANS] = 1.0;
        break;
    case(TWEET):
        default_val[TWT_PDAT] = 440.0;
        default_val[TWT_MIN]  = 0.0;
        if(mode != 2) {
            default_val[TWT_PKCNT] = 4;
            default_val[TWT_CHIRP] = 0;
        }
        break;
    case(RRRR_EXTEND):  // version 8+
        if(mode == 1) {
            default_val[RRR_GATE]    = 0.0;
            default_val[RRR_GRSIZ]   = LOW_RRR_SIZE;
            default_val[RRR_SKIP]    = 0;
        } else {
            default_val[RRR_START]   = 0.0;
            default_val[RRR_END]     = duration;
        }
        default_val[RRR_SLOW]    = 1.0;
        default_val[RRR_REGU]    = 0.0;
        default_val[RRR_RANGE]   = 1.0;
        default_val[RRR_GET]     = 3.0;
        if(mode != 2) {
            default_val[RRR_STRETCH] = 2.0;
            default_val[RRR_REPET]   = 2.0;
            default_val[RRR_ASCAT]   = 0.0;
            default_val[RRR_PSCAT]   = 0.0;
        }
        break;
    case(SORTER):
        default_val[SORTER_SIZE] = 0;
        if(mode == 4) {
            default_val[SORTER_SEED] = 1;
        }
        default_val[SORTER_SMOOTH] = 5;
        default_val[SORTER_OMIDI]  = 0;
        default_val[SORTER_IMIDI]  = 0;
        default_val[SORTER_META]   = 0.0;
        break;
    case(SPECFNU):
        switch(mode) {
        case(F_NARROW):
            default_val[NARROWING] = 2.0;
            default_val[NARSUPRES] = 0;
            default_val[FGAIN]     = 1.0;
            break;
        case(F_SQUEEZE):
            default_val[SQZFACT] = 2.0;
            default_val[SQZAT]   = 1;
            default_val[FGAIN]   = 1.0;
            break;
        case(F_INVERT):
            default_val[FVIB]  = 0.0;
            default_val[FGAIN] = 1.0;
            break;
        case(F_ROTATE):
            default_val[RSPEED] = 1.0;
            default_val[FGAIN]  = 1.0;
            break;
        case(F_NEGATE):
            default_val[FGAIN]  = 1.0;
            break;
        case(F_SUPPRESS):
            default_val[SUPRF] = 1;
            default_val[FGAIN] = 1.0;
            break;
        case(F_MAKEFILT):
            default_val[FPKCNT] = 1;
            default_val[FBELOW] = 0;
            break;
        case(F_MOVE):
            default_val[FMOVE1]  = 0.0;
            default_val[FMOVE2]  = 0.0;
            default_val[FMOVE3]  = 0.0;
            default_val[FMOVE4]  = 0.0;
            default_val[FMVGAIN] = 1.0;
            break;
        case(F_MOVE2):
            default_val[FMOVE1]  = 200;
            default_val[FMOVE2]  = 1200;
            default_val[FMOVE3]  = 2000;
            default_val[FMOVE4]  = 3200;
            default_val[FMVGAIN] = 1.0;
            break;
        case(F_SYLABTROF):
            default_val[FMINSYL] = MIN_SYLLAB_DUR;      //  0.08
            default_val[FMINPKG] = MIN_PEAKTROF_GAP;    //  0.08
            break;
        case(F_ARPEG):
            default_val[FARPRATE] = 1.0;
            default_val[FGAIN]    = 1.0;
            break;
        case(F_OCTSHIFT):
            default_val[COLINT]  = 1.0;
            default_val[FGAIN]   = 1.0;
            default_val[COL_LO]  = 0.0;
            default_val[COL_HI]  = 2000.0;
            default_val[COLRATE] = 0.0;
            break;
        case(F_TRANS):
            default_val[COLFLT]  = 1.0;
            default_val[FGAIN]   = 1.0;
            default_val[COL_LO]  = 0.0;
            default_val[COL_HI]  = 2000.0;
            default_val[COLRATE] = 0.0;
            break;
        case(F_FRQSHIFT):
            default_val[COLFLT]  = 100.0;
            default_val[FGAIN]   = 1.0;
            default_val[COL_LO]  = 0.0;
            default_val[COL_HI]  = 2000.0;
            default_val[COLRATE] = 0.0;
            break;
        case(F_RESPACE):
            default_val[COLFLT]  = 10.0;
            default_val[FGAIN]   = 1.0;
            default_val[COL_LO]  = 0.0;
            default_val[COL_HI]  = 2000.0;
            default_val[COLRATE] = 0.0;
            break;
        case(F_PINVERT):
            default_val[COLFLT]   = 60;
            default_val[FGAIN]    = 1.0;
            default_val[COL_LO]   = 0.0;
            default_val[COL_HI]   = 2000.0;
            default_val[COLRATE]  = 0.0;
            default_val[COLLOPCH] = SPEC_MIDIMIN;
            default_val[COLHIPCH] = MIDIMAX;
            break;
        case(F_PEXAGG):
            default_val[COLFLT]   = 60;
            default_val[EXAGRANG] = 1.0;
            default_val[FGAIN]    = 1.0;
            default_val[COL_LO]   = 0.0;
            default_val[COL_HI]   = 2000.0;
            default_val[COLRATE]  = 0.0;
            default_val[COLLOPCH] = SPEC_MIDIMIN;
            default_val[COLHIPCH] = MIDIMAX;
            break;
        case(F_PQUANT):
            default_val[FGAIN]    = 1.0;
            default_val[COL_LO]   = 0.0;
            default_val[COL_HI]   = 2000.0;
            default_val[COLRATE]  = 0.0;
            default_val[COLLOPCH] = SPEC_MIDIMIN;
            default_val[COLHIPCH] = MIDIMAX;
            break;
        case(F_PCHRAND):
            default_val[FPRMAXINT] = 2.0;
            default_val[FSLEW]     = 1.0;
            default_val[FGAIN]     = 1.0;
            default_val[COL_LO]    = 0.0;
            default_val[COL_HI]    = 2000.0;
            default_val[COLRATE]   = 0.0;
            default_val[COLLOPCH]  = SPEC_MIDIMIN;
            default_val[COLHIPCH]  = MIDIMAX;
            break;
        case(F_RAND):
            default_val[COLFLT]  = 0.1;
            default_val[FGAIN]   = 1.0;
            default_val[COL_LO]  = 0.0;
            default_val[COL_HI]  = 2000.0;
            default_val[COLRATE] = 0.0;
            break;
        case(F_SINUS):
            default_val[F_SINING] = 1.0;
            default_val[FGAIN]    = 1.0;
            default_val[F_AMP1]   = 1.0;
            default_val[F_AMP2]   = 1.0;
            default_val[F_AMP3]   = 1.0;
            default_val[F_AMP4]   = 1.0;
            default_val[F_QDEP1]  = 0.0;
            default_val[F_QDEP2]  = 0.0;
            default_val[F_QDEP3]  = 0.0;
            default_val[F_QDEP4]  = 0.0;
            break;
        }
        break;
    case(FLATTEN):
        default_val[0]  = 0.1;
        default_val[1]  = 20.0;
        default_val[2]  = 0.0;
        break;
    case(BOUNCE):
        default_val[BNC_NUMBER]  = 8;
        default_val[BNC_STTSTEP] = 1.0;
        default_val[BNC_SHORTEN] = 0.92;
        default_val[BNC_ENDLEV]  = 0.05;
        default_val[BNC_LEVWRP]  = 1.0;
        default_val[BNC_MINDUR]  = 0.02;
        break;
    case(DISTMARK):
        default_val[0]  = 5;
        default_val[1]  = 1;
        default_val[2]  = 0;
        if(mode == 1)
            default_val[3]  = 1;
        break;
    case(DISTREP):
        default_val[0]  = 2.0;
        default_val[1]  = 1.0;
        default_val[2]  = 0.0;
        default_val[3]  = 15;
        break;
    case(TOSTEREO):
        default_val[0]  = 0.0;
        default_val[1]  = duration;
        default_val[2]  = 2;
        default_val[3]  = 0;
        default_val[4]  = 0;
        default_val[5]  = 0.0;
        break;
    case(SUPPRESS):
        default_val[0] = 0.0;
        default_val[1] = nyquist;
        default_val[2] = 1;
        break;
    case(CALTRAIN):
        default_val[0]  = 1;
        default_val[1]  = 440;
        default_val[2]  = 0;
        break;
    case(SPECENV):
        default_val[0]  = 60;
        default_val[1]  = 0;
        break;
    case(CLIP):
        switch(mode) {
        case(0):
            default_val[0]  = 0.1;
            break;
        case(1):
            default_val[0]  = 0.5;
            break;
        }
        break;
    case(SPECEX):
        default_val[0]  = 0;
        default_val[1]  = frametime * 3;
        default_val[2]  = 64.0;
        default_val[3]  = 1;
        break;
    case(MATRIX):
        switch(mode) {
        case(MATRIX_USE):
            break;
        default:
            default_val[MATRIX_CHANS]   = (double)DEFAULT_PVOC_CHANS;
            default_val[MATRIX_OVLAP]   = (double)DEFAULT_WIN_OVERLAP;
            break;
        }
        break;
    case(TRANSPART):
        switch(mode) {
        case(0):    //  fall thro
        case(1):
        case(2):
        case(3):
            default_val[0]  = 12;
            break;
        case(4):    //  fall thro
        case(5):
        case(6):
        case(7):
            default_val[0]  = 100;
            break;
        }
        default_val[1]  = 440;
        default_val[2]  = 1.0;
        break;
    case(SPECINVNU):
        default_val[0]  = 0.0;
        default_val[1]  = nyquist;
        default_val[2]  = 4;
        default_val[3]  = 1.0;
        break;
    case(SPECCONV):
        default_val[0]  = 1.0;
        default_val[1]  = 1;
        break;
    case(SPECSND):
        default_val[0]  = 0;
        default_val[1]  = 1;
        break;
    case(FRACTAL):
        if(mode == 1)
            default_val[0] = 10;
        default_val[1] = 0;
        default_val[2] = 1;
        default_val[3] = 0;
        break;
    case(FRACSPEC):
        default_val[1] = 0;
        default_val[2] = 1;
        default_val[3] = 0;
        break;
    case(SPECFRAC):
        default_val[0] = 8;
        break;
    case(ENVSPEAK):
        if(mode < 12)
            default_val[0]  = 50;               //  WINDOW SIZE
        mode %= 12;
        default_val[1]  = 5;                    //  SPLICELEN
        if(mode < 9) {
            default_val[2]  = 0;
            switch(mode) {
            case(2):    //  fall thro           //  ATTENUATION OF SEGMENT
            case(3):
                default_val[4]  = -96;
                //  fall thro
            case(0):    //  fall thro           //  REPETITION OF SEGMENTS, OR GROUPSIZE IN SEGMENT DELETIONS
            case(4):    //  fall thro
            case(5):    //  fall thro
            case(7):    //  fall thro
            case(8):    //  fall thro
            case(6):                            //  NO OF REPETS OF DIVISION OF SEGMENT = GROUPSIZE TO ATTENUATE
                default_val[3]  = 2;
                break;
            }
            if(!(mode == 1 || mode == 2 || mode == 3)) {
                default_val[4]  = 0;            //  RANDOMISATION
            }
            switch(mode) {
            case(6):
                default_val[5]  = 1;            //  WHICH SYLLABLE-DIVISION TO USE
                break;
            case(7):    //  fall thro
            case(8):
                default_val[5]  = .5;           //  CONTRACTION RATIO
                break;
            }
        }
        switch(mode) {
        case(10):
            default_val[2]  = 0;                //  RANDOM SEED
            break;
        case(11):
            default_val[2]  = 2;                // GROUP-SIZE FOR REVERSAL
            break;
        }
        break;
    case(EXTSPEAK):
        if(mode < 6)
            default_val[XSPK_WINSZ] = 50;
        default_val[XSPK_SPLEN] = 5;
        if(mode < 12) {
            default_val[XSPK_OFFST] = 0;
            default_val[XSPK_N]     = 1;
        }
        default_val[XSPK_GAIN]  = 0;
        if(mode != 2 && mode != 5 && mode != 8 && mode != 11 && mode != 14 && mode != 17)
            default_val[XSPK_SEED]  = 0;
        break;
    case(ENVSCULPT):
        if(mode != 2)
            default_val[PKCH_WSIZE]  = 20.0;
        default_val[PKCH_RISE]   = 2.0;
        default_val[PKCH_DECDUR] = min(0.3,duration);
        default_val[PKCH_STEEP]  = 3;
        if(mode == 1) {
            default_val[PKCH_ZSTT]  = 0.0;
            default_val[PKCH_ZEND]  = 0.0;
        }
        if(mode != 0)
            default_val[PKCH_RATIO] = 1.0;
        break;
    case(TREMENV):
        default_val[TREMOLO_FRQ] = 15;
        default_val[TREMOLO_DEP] = 1.0;
        default_val[TREMOLO_WIN] = 10.0;
        default_val[TREMOLO_SQZ] = 1.0;
        break;
    case(DCFIX):
        default_val[0]  = 10.0;
        break;
    default:
        sprintf(errstr,"Unknown case: initialise_param_values2()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/****************************** ESTABLISH_DISPLAY2 *********************************/

int establish_display2(int process,int mode,int total_params,float frametime,double duration,aplptr ap)
{
    if((ap->display_type = (char *)malloc((total_params)* sizeof(char)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: for display_type array\n");
        return(MEMORY_ERROR);
    }

    if((ap->has_subrange = (char *)malloc((total_params) * sizeof(char)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: for subrange array\n");
        return(MEMORY_ERROR);
    }

    if((ap->lolo = (double *)malloc((total_params) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: for lo subrange vals\n");
        return(MEMORY_ERROR);
    }

    if((ap->hihi = (double *)malloc((total_params) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: for hi subrange vals\n");
        return(MEMORY_ERROR);
    }
    switch(process) {
    case(TAPDELAY):
        setup_display2(0,LINEAR,SUBRANGE,ap->lo[0],2.0,ap); /* gain */
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);             /* feedback */
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);             /* mix */
        setup_display2(3,LINEAR,SUBRANGE,ap->lo[3],60.0,ap);/* decay tail */
        break;
    case(RMRESP):
        setup_display2(0, NUMERIC,0,0.0,0.0,ap);            /* liveness */
        setup_display2(1, LINEAR,SUBRANGE,ap->lo[0],5.0,ap);/* nrefs */
        setup_display2(2, LINEAR,SUBRANGE,2,25,ap);         /* roomL */
        setup_display2(3, LINEAR,SUBRANGE,2,25,ap);         /* roomW */
        setup_display2(4, LINEAR,SUBRANGE,1,10,ap);         /* roomH */
        setup_display2(5, LINEAR,SUBRANGE,1,25,ap);         /* srcL */
        setup_display2(6, LINEAR,SUBRANGE,1,25,ap);         /* srcW */
        setup_display2(7, LINEAR,SUBRANGE,1,10,ap);         /* srcH */
        setup_display2(8, LINEAR,SUBRANGE,1,25,ap);         /* listenerL */
        setup_display2(9, LINEAR,SUBRANGE,1,25,ap);         /* listenerW */
        setup_display2(10,LINEAR,SUBRANGE,1,10,ap);         /* listenerH */
        setup_display2(11,NUMERIC,0,0.0,0.0,ap);            /* maxamp */
        setup_display2(12,NUMERIC,0,0.0,0.0,ap);            /* res */
        break;
    case(RMVERB):
        setup_display2(0, NUMERIC,0,0.0,0.0,ap);            /* roomsize */
        setup_display2(1, NUMERIC,0,0.0,0.0,ap);            /* dense_reverb_gain */
        setup_display2(2, NUMERIC,0,0.0,0.0,ap);            /* source_in_mix */
        setup_display2(3, NUMERIC,0,0.0,0.0,ap);            /* feedback */
        setup_display2(4 ,LINEAR,SUBRANGE,2500,4200,ap);    /* air-absorption_cutoff */
        setup_display2(5 ,NUMERIC,0,0.0,0.0,ap);            /* lopass_reverb-input_cutoff */
        setup_display2(6,LINEAR,SUBRANGE,ap->lo[6],60.0,ap);/* decay tail */
        setup_display2(7 ,NUMERIC,0,0.0,0.0,ap);            /* lopass_input_cutoff */
        setup_display2(8 ,NUMERIC,0,0.0,0.0,ap);            /* hipass_input_cutoff */
        setup_display2(9 ,LINEAR,SUBRANGE,0,duration,ap);       /* predelay */
        setup_display2(10,NUMERIC,0,0.0,0.0,ap);            /* output_chans */
        break;
    case(MIXMULTI):
        setup_display2(0, NUMERIC,0,0.0,0.0,ap);
        setup_display2(1 ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2 ,NUMERIC,0,0.0,0.0,ap);
        break;
    case(ANALJOIN):
        break;
    case(PTOBRK):
        setup_display2(0, LOGNUMERIC,0,0.0,0.0,ap);
        break;
    case(PSOW_STRETCH):
        setup_display2(0, FILENAME,0,0.0,0.0,ap);
        setup_display2(1, LOGNUMERIC,0,0.0,0.0,ap);
        setup_display2(2, LINEAR,SUBRANGE,1,12,ap);
        break;
    case(PSOW_DUPL):
        setup_display2(0, FILENAME,0,0.0,0.0,ap);
        setup_display2(1, LINEAR,0,0.0,0.0,ap);
        setup_display2(2, LINEAR,SUBRANGE,1,12,ap);
        break;
    case(PSOW_DEL):
        setup_display2(0, FILENAME,0,0.0,0.0,ap);
        setup_display2(1, LINEAR,0,0.0,0.0,ap);
        setup_display2(2, LINEAR,SUBRANGE,1,12,ap);
        break;
    case(PSOW_STRFILL):
        setup_display2(0, FILENAME,0,0.0,0.0,ap);
        setup_display2(1, LOGNUMERIC,0,0.0,0.0,ap);
        setup_display2(2, LINEAR,SUBRANGE,1,12,ap);
        setup_display2(3, LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_FREEZE):
        setup_display2(0, FILENAME,0,0.0,0.0,ap);
        setup_display2(1, LINEAR,0,0.0,0.0,ap);
        setup_display2(2, LINEAR,SUBRANGE,0.0,120.0,ap);
        setup_display2(3, LINEAR,0,0.0,0.0,ap);
        setup_display2(4, LINEAR,SUBRANGE,0.05,2.0,ap);
        setup_display2(5, LINEAR,SUBRANGE,0.25,2.0,ap);
        setup_display2(6, LINEAR,0,0.0,0.0,ap);
        setup_display2(7, LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_CHOP):
        setup_display2(0, FILENAME,0,0.0,0.0,ap);
        setup_display2(1, FILENAME,0,0.0,0.0,ap);
        break;
    case(PSOW_INTERP):
        setup_display2(PS_SDUR,     LINEAR,SUBRANGE,0.0,120.0,ap);
        setup_display2(PS_IDUR,     LINEAR,SUBRANGE,0.0,120.0,ap);
        setup_display2(PS_EDUR,     LINEAR,SUBRANGE,0.0,120.0,ap);
        setup_display2(PS_VIBFRQ,   LINEAR,0,0.0,0.0,ap);
        setup_display2(PS_VIBDEPTH, LINEAR,0,0.0,0.0,ap);
        setup_display2(PS_TREMFRQ,  LINEAR,0,0.0,0.0,ap);
        setup_display2(PS_TREMDEPTH,LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_FEATURES):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,SUBRANGE,1,12,ap);
        setup_display2(2,LINEAR,SUBRANGE,-12,12,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        setup_display2(4,LINEAR,0,0.0,0.0,ap);
        setup_display2(5,LINEAR,0,0.0,0.0,ap);
        setup_display2(6,LINEAR,SUBRANGE,0.0,0.2,ap);
        setup_display2(7,LINEAR,0,0.0,0.0,ap);
        setup_display2(8,LINEAR,0,0.0,0.0,ap);
        setup_display2(9,LINEAR,0,0.0,0.0,ap);
        setup_display2(10,LINEAR,SUBRANGE,1,256,ap);
        break;
    case(PSOW_SYNTH):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_IMPOSE):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_SPLIT):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_SPACE):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LOG,0,0.0,0.0,ap);
        setup_display2(4,LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_INTERLEAVE):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        setup_display2(1,FILENAME,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        setup_display2(4,LOG,0,0.0,0.0,ap);
        setup_display2(5,LOG,0,0.0,0.0,ap);
        break;
    case(PSOW_REPLACE):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        setup_display2(1,FILENAME,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_EXTEND):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,SUBRANGE,ap->lo[PS_DUR],120.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        setup_display2(4,LINEAR,0,0.0,0.0,ap);
        setup_display2(5,LINEAR,0,0.0,0.0,ap);
        setup_display2(6,LINEAR,SUBRANGE,-7.0,7.0,ap);
        setup_display2(7,LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_EXTEND2):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,SUBRANGE,ap->lo[PS_DUR],120.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        setup_display2(4,LINEAR,0,0.0,0.0,ap);
        setup_display2(5,LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_LOCATE):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        break;
    case(PSOW_CUT):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        break;
    case(ONEFORM_GET):
        setup_display2(0, NUMERIC,0,0.0,0.0,ap);
        break;
    case(ONEFORM_PUT):
        setup_display2(FORM_FTOP,PLOG,0,0.0,0.0,ap);
        setup_display2(FORM_FBOT,PLOG,0,0.0,0.0,ap);
        setup_display2(FORM_GAIN,LINEAR,SUBRANGE,ap->lo[FORM_GAIN],16.0,ap);
        break;
    case(ONEFORM_COMBINE):
        break;
    case(NEWGATE):
        setup_display2(0, LINEAR,0,0.0,0.0,ap);
        break;
    case(SPEC_REMOVE):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        break;
    case(PREFIXSIL):
        setup_display2(0, LINEAR,SUBRANGE,0.0,1.0,ap);
        break;
    case(STRANS):
        switch(mode) {
        case(0):
            setup_display2(0,LOG,SUBRANGE,0.25,4.0,ap); 
            break;
        case(1):
            setup_display2(0,LINEAR,SUBRANGE,-24.0,24.0,ap);    
            break;
        case(2):
            setup_display2(ACCEL_ACCEL,LOG,SUBRANGE,0.5,2.0,ap);    
            setup_display2(ACCEL_GOALTIME,NUMERIC,0,0.0,0.0,ap);    
            setup_display2(ACCEL_STARTTIME,NUMERIC,0,0.0,0.0,ap);   
            break;
        case(3):
            setup_display2(VIB_FRQ,LINEAR,SUBRANGE,0.0,50.0,ap);    
            setup_display2(VIB_DEPTH,LINEAR,SUBRANGE,0.0,12.0,ap);  
            break;
        }
        break;
    case(PSOW_REINF):
        setup_display2(0,FILENAME,0,0.0,0.0,ap);
        if(mode == 0)
            setup_display2(1,LINEAR,SUBRANGE,0.0,200.0,ap);
        else
            setup_display2(1,LINEAR,SUBRANGE,1.0,64.0,ap);
        break;
    case(PARTIALS_HARM):  
        setup_display2(0, NUMERIC,0,0.0,0.0,ap);
        setup_display2(1, NUMERIC,0,0.0,0.0,ap);
        if(mode > 1)
            setup_display2(2, NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECROSS):  
        setup_display2(PICH_RNGE,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(PICH_VALID, LINEAR,SUBRANGE,ap->lo[PICH_VALID],min(ap->hi[PICH_VALID],8.0),ap);
        setup_display2(PICH_SRATIO,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PICH_MATCH, NUMERIC,0,0.0,0.0,ap);
        setup_display2(PICH_HILM,  PLOG,0,0.0,0.0,ap);
        setup_display2(PICH_LOLM,  PLOG,0,0.0,0.0,ap);
        setup_display2(PICH_THRESH,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPCMPLEV,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPECHINT,   LINEAR,0,0.0,0.0,ap);
        break;
    case(LUCIER_GETF):
        setup_display2(LUCIER_CUT,        NUMERIC,SUBRANGE,MINPITCH,200.0,ap);
        /* fall thro */
    case(LUCIER_GET):
        setup_display2(MIN_ROOM_DIMENSION,NUMERIC,0,0.0,0.0,ap);
        setup_display2(ROLLOFF_INTERVAL,  NUMERIC,0,0.0,0.0,ap);
        break;
    case(LUCIER_PUT):
        setup_display2(RESON_CNT,         LINEAR,0,0.0,0.0,ap);
        setup_display2(RES_EXTEND_ATTEN,  LINEAR,0,0.0,0.0,ap);
        break;
    case(LUCIER_DEL):
        setup_display2(SUPR_COEFF,        LINEAR,0,0.0,0.0,ap);
        break;
    case(SPECTRACT):
    case(SPECLEAN):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(PHASE):
        if(mode == 1)
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECSLICE):
        if(mode < 3) {
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,LINEAR,0,0.0,0.0,ap);
        }
        if(mode == 4)
            setup_display2(0,LINEAR,0,0.0,0.0,ap);
        break;
    case(FOFEX_CO):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        switch(mode) {
        case(FOF_MEASURE):
            break;
        case(FOF_SINGLE):
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            break;
        case(FOF_LOHI):
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            setup_display2(4,NUMERIC,0,0.0,0.0,ap);
            setup_display2(5,NUMERIC,0,0.0,0.0,ap);
            setup_display2(6,NUMERIC,0,0.0,0.0,ap);
            break;
        case(FOF_TRIPLE):
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            setup_display2(4,NUMERIC,0,0.0,0.0,ap);
            setup_display2(5,NUMERIC,0,0.0,0.0,ap);
            setup_display2(6,NUMERIC,0,0.0,0.0,ap);
            setup_display2(7,NUMERIC,0,0.0,0.0,ap);
            setup_display2(8,NUMERIC,0,0.0,0.0,ap);
            setup_display2(9,NUMERIC,0,0.0,0.0,ap);
            break;
        }
        break;
    case(FOFEX_EX):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        break;
    case(GREV_EXTEND):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        setup_display2(4,NUMERIC,0,0.0,0.0,ap);
        break;
    case(PEAKFIND):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(CONSTRICT):
        setup_display2(0,LINEAR,SUBRANGE,0.0,100.0,ap);
        break;
    case(EXPDECAY):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(PEAKCHOP):
        setup_display2(PKCH_WSIZE ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PKCH_WIDTH ,LINEAR,SUBRANGE,0.0,200.0,ap);
        setup_display2(PKCH_SPLICE,LINEAR,0,0.0,0.0,ap);
        setup_display2(PKCH_GATE  ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PKCH_SKEW  ,NUMERIC,0,0.0,0.0,ap);
        if(mode == 0 || mode == 2) {
            setup_display2(PKCH_TEMPO ,LOG,SUBRANGE,20.0,600.0,ap);
            setup_display2(PKCH_GAIN  ,LINEAR,0,0.0,0.0,ap);
            setup_display2(PKCH_SCAT  ,LINEAR,0,0.0,0.0,ap);
            setup_display2(PKCH_NORM  ,LINEAR,0,0.0,0.0,ap);
            setup_display2(PKCH_REPET ,LINEAR,0,0.0,0.0,ap);
            setup_display2(PKCH_MISS  ,NUMERIC,0,0.0,0.0,ap);
        }
        if(mode == 0)
            setup_display2(PKCH_MISS  ,NUMERIC,0,0.0,0.0,ap);
        break;
    case(MCHANPAN):
        switch(mode) {
        case(9):
            setup_display2(3,LINEAR,0,0.0,0.0,ap);
            /* fall thro */
        case(1):
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            /* fall thro */
        case(0):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            break;
        case(2):
            setup_display2(5,NUMERIC,0,0.0,0.0,ap);
            /* fall thro */
        case(3):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,LINEAR,0,0.0,0.0,ap);
            setup_display2(3,LINEAR,0,0.0,0.0,ap);
            setup_display2(4,LINEAR,0,0.0,0.0,ap);
            break;
        case(4):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            break;
        case(5):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,LINEAR,0,0.0,0.0,ap);
            setup_display2(2,LINEAR,0,0.0,0.0,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            break;
        case(6):
            setup_display2(0,LINEAR,0,0.0,0.0,ap);
            break;
        case(7):
            setup_display2(0,LINEAR,0,0.0,0.0,ap);
            setup_display2(1,LINEAR,0,0.0,0.0,ap);
            break;
        case(8):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,LINEAR,0,0.0,0.0,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            break;
        }
        break;
    case(TEX_MCHAN):
        setup_display2(TEXTURE_DUR,LINEAR,SUBRANGE,0.0,60.0,ap);
        setup_display2(TEXTURE_PACK,LOG,SUBRANGE,0.002,2.0,ap);
        setup_display2(TEXTURE_SCAT,LINEAR,SUBRANGE,ap->lo[TEXTURE_SCAT],2.0,ap);
        setup_display2(TEXTURE_TGRID,LINEAR,SUBRANGE,ap->lo[TEXTURE_TGRID],100.0,ap);
        setup_display2(TEXTURE_INSLO,LINEAR,SUBRANGE,ap->lo[TEXTURE_INSLO],64.0,ap);
        setup_display2(TEXTURE_INSHI,LINEAR,SUBRANGE,ap->lo[TEXTURE_INSHI],64.0,ap);
        setup_display2(TEXTURE_MAXAMP,LINEAR,0,0.0,0.0,ap);
        setup_display2(TEXTURE_MINAMP,LINEAR,0,0.0,0.0,ap);
        setup_display2(TEXTURE_MAXDUR,LINEAR,SUBRANGE,ap->lo[TEXTURE_MAXDUR],min(ap->hi[TEXTURE_MAXDUR],2.0),ap);
        setup_display2(TEXTURE_MINDUR,LINEAR,SUBRANGE,ap->lo[TEXTURE_MINDUR],min(ap->hi[TEXTURE_MINDUR],2.0),ap);
        setup_display2(TEXTURE_MAXPICH,LINEAR,0,0.0,0.0,ap);
        setup_display2(TEXTURE_MINPICH,LINEAR,0,0.0,0.0,ap);
        setup_display2(TEXTURE_OUTCHANS,LINEAR,0,0.0,0.0,ap);
        setup_display2(TEXTURE_ATTEN,LINEAR,0,0.0,0.0,ap);
        setup_display2(TEXTURE_POS,LINEAR,0,0.0,0.0,ap);
        setup_display2(TEXTURE_SPRD,LINEAR,0,0.0,0.0,ap);
        setup_display2(TEXTURE_SEED,LINEAR,SUBRANGE,ap->lo[TEXTURE_SEED],64.0,ap);
        break;
    case(MANYSIL):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(RETIME):
        switch(mode) {
        case(0):
            setup_display2(MM,NUMERIC,0,0.0,0.0,ap);
            break;
        case(1):
            setup_display2(MM,NUMERIC,0,0.0,0.0,ap);
            setup_display2(RETIME_WIDTH,LINEAR,0,0.0,0.0,ap);
            setup_display2(RETIME_SPLICE,NUMERIC,0,0.0,0.0,ap);
            break;
        case(2):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            break;
        case(3):
            setup_display2(MM,LINEAR,SUBRANGE,0.0,600.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            break;
        case(4):
            setup_display2(0,LOG,SUBRANGE,0.0625,16.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            setup_display2(4,NUMERIC,0,0.0,0.0,ap);
            break;
        case(5):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            break;
        case(6):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            break;
        case(7):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            setup_display2(4,NUMERIC,0,0.0,0.0,ap);
            break;
        case(8):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            break;
        case(9):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,LINEAR,SUBRANGE,0,64,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            break;
        case(10):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            break;
        case(12):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            break;
        case(13):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            break;
        }
        break;
    case(HOVER):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        setup_display2(4,NUMERIC,0,0.0,0.0,ap);
        setup_display2(5,NUMERIC,0,0.0,0.0,ap);
        break;
    case(HOVER2):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        setup_display2(4,NUMERIC,0,0.0,0.0,ap);
        break;
    case(MULTIMIX):
        switch(mode) {
        case(2):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            break;
        case(3):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            break;
        case(4):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            break;
        case(6):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            setup_display2(3,LINEAR,SUBRANGE,0,60.0,ap);
            break;
        case(7):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            break;
        }
        break;
    case(FRAME):
        switch(mode) {
        case(0):
            setup_display2(0,LINEAR,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            break;
        case(1):
            setup_display2(0,LINEAR,0,0.0,0.0,ap);
            setup_display2(1,LINEAR,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            break;
        case(2):
        case(4):
        case(7):
            break;
        case(3):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            break;
        case(5):
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            break;
        case(6):
            setup_display2(0,LINEAR,0,0.0,0.0,ap);
            break;
        }
        break;
    case(SEARCH):
        break;
    case(MCHANREV):
        setup_display2(STAD_PREGAIN,LINEAR,0,0.0,0.0,ap);
        setup_display2(STAD_ROLLOFF,LINEAR,0,0.0,0.0,ap);
        setup_display2(STAD_SIZE,LOG,SUBRANGE,0.1,10.0,ap);
        setup_display2(STAD_ECHOCNT,LINEAR,SUBRANGE,2,REASONABLE_ECHOCNT,ap);
        setup_display2(REV_OCHANS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(REV_CENTRE,NUMERIC,0,0.0,0.0,ap);
        setup_display2(REV_SPREAD,NUMERIC,0,0.0,0.0,ap);
        break;
    case(WRAPPAGE):
        setup_display2(WRAP_OUTCHANS,LINEAR,0,0.0,0.0,ap);
        setup_display2(WRAP_SPREAD,LINEAR,0,0.0,0.0,ap);
        setup_display2(WRAP_DEPTH,LINEAR,0,0.0,0.0,ap);
        setup_display2(WRAP_VELOCITY,LINEAR,SUBRANGE,ap->lo[WRAP_VELOCITY],8.0,ap);
        setup_display2(WRAP_HVELOCITY,LINEAR,SUBRANGE,ap->lo[WRAP_HVELOCITY],min(ap->hi[WRAP_HVELOCITY],8.0),ap);
        setup_display2(WRAP_DENSITY,LOG,SUBRANGE,0.125,min(ap->hi[WRAP_DENSITY],8.0),ap);
        setup_display2(WRAP_HDENSITY,LOG,SUBRANGE,0.125,8.0,ap);
        setup_display2(WRAP_GRAINSIZE,LINEAR,SUBRANGE,ap->lo[WRAP_GRAINSIZE],min(ap->hi[WRAP_GRAINSIZE],200.0),ap);
        setup_display2(WRAP_HGRAINSIZE,LINEAR,SUBRANGE,ap->lo[WRAP_GRAINSIZE],min(ap->hi[WRAP_GRAINSIZE],200.0),ap);
        setup_display2(WRAP_PITCH,LINEAR,SUBRANGE,-12.0,12.0,ap);
        setup_display2(WRAP_HPITCH,LINEAR,SUBRANGE,-12.0,12.0,ap);
        setup_display2(WRAP_AMP,LINEAR,0,0.0,0.0,ap);
        setup_display2(WRAP_HAMP,LINEAR,0,0.0,0.0,ap);
        setup_display2(WRAP_BSPLICE,LINEAR,SUBRANGE,ap->lo[WRAP_BSPLICE],min(ap->hi[WRAP_BSPLICE],WRAP_DEFAULT_SPLICELEN * 4.0),ap);
        setup_display2(WRAP_HBSPLICE,LINEAR,SUBRANGE,ap->lo[WRAP_BSPLICE],min(ap->hi[WRAP_BSPLICE],WRAP_DEFAULT_SPLICELEN * 4.0),ap);
        setup_display2(WRAP_ESPLICE,LINEAR,SUBRANGE,ap->lo[WRAP_ESPLICE],min(ap->hi[WRAP_ESPLICE],WRAP_DEFAULT_SPLICELEN * 4.0),ap);
        setup_display2(WRAP_HESPLICE,LINEAR,SUBRANGE,ap->lo[WRAP_ESPLICE],min(ap->hi[WRAP_ESPLICE],WRAP_DEFAULT_SPLICELEN * 4.0),ap);
        setup_display2(WRAP_SRCHRANGE,LINEAR,SUBRANGE,ap->lo[WRAP_SRCHRANGE],min(ap->hi[WRAP_SRCHRANGE],200.0),ap);
        setup_display2(WRAP_SCATTER,LINEAR,0,0.0,0.0,ap);
        setup_display2(WRAP_OUTLEN,LINEAR,SUBRANGE,0.0,60.0,ap);
        setup_display2(WRAP_BUFXX,NUMERIC,0,0.0,0.0,ap);
        break;
    case(MCHSTEREO):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(MTON):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(FLUTTER):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        break;
    case(ABFPAN):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,SUBRANGE,0.0,100.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        break;
    case(ABFPAN2):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,SUBRANGE,0.0,100.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        break;
    case(ABFPAN2P):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,SUBRANGE,0.0,100.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        break;
    case(CHANNELX):
        break;
    case(CHORDER):
        break;
    case(FMDCODE):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(CHXFORMAT):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(CHXFORMATG):
    case(CHXFORMATM):
        break;
    case(INTERLX):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(COPYSFX):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(NJOIN):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(NJOINCH):
        break;
    case(NMIX):
        setup_display2(0,LINEAR,SUBRANGE,0.0,60.0,ap);
        break;
    case(RMSINFO):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SFEXPROPS):
        break;
    case(SETHARES):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,LOGNUMERIC,0,0.0,0.0,ap);
        setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        setup_display2(4,NUMERIC,0,0.0,0.0,ap);
        setup_display2(5,NUMERIC,0,0.0,0.0,ap);
        break;
    case(MCHSHRED):
        setup_display2(0,LINEAR,SUBRANGE,1.0,2000.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,SUBRANGE,0.0,1.0,ap);
        if(mode == 0)
            setup_display2(3,LINEAR,SUBRANGE,2.0,16.0,ap);
        break;
    case(MCHZIG):
        if(mode==0) {
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        }
        setup_display2(4,NUMERIC,0,0.0,0.0,ap);
        setup_display2(5,NUMERIC,0,0.0,0.0,ap);
        if(mode==0) {
            setup_display2(6,NUMERIC,0,0.0,0.0,ap);
            setup_display2(7,NUMERIC,0,0.0,0.0,ap);
        }
        break;
    case(MCHITER):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        setup_display2(4,LINEAR,0,0.0,0.0,ap);
        setup_display2(5,LINEAR,0,0.0,0.0,ap);
        setup_display2(6,LINEAR,0,0.0,0.0,ap);
        setup_display2(7,NUMERIC,0,0.0,0.0,ap);
        setup_display2(8,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECSPHINX):
        switch(mode) {
        case(0):
            setup_display2(0,LINEAR,0,0.0,0.0,ap);
            setup_display2(1,LINEAR,0,0.0,0.0,ap);
            break;
        case(1):
            setup_display2(0,LINEAR,0,0.0,0.0,ap);
            setup_display2(1,LINEAR,0,0.0,0.0,ap);
            break;
        case(2):
            setup_display2(0,LINEAR,0,0.0,0.0,ap);
            setup_display2(1,LINEAR,0,0.0,0.0,ap);
            setup_display2(2,LINEAR,0,0.0,0.0,ap);
            break;
        }
        break;
    case(SPECMORPH):
        if(mode == 6) {
            setup_display2(NMPH_APKS,NUMERIC,0,0.0,0.0,ap);
            setup_display2(NMPH_OCNT,NUMERIC,0,0.0,0.0,ap);
        } else {
            setup_display2(NMPH_STAG,NUMERIC,0,0.0,0.0,ap);
            setup_display2(NMPH_ASTT,NUMERIC,0,0.0,0.0,ap);
            setup_display2(NMPH_AEND,NUMERIC,0,0.0,0.0,ap);
            setup_display2(NMPH_AEXP,NUMERIC,0,0.0,0.0,ap);
            setup_display2(NMPH_APKS,NUMERIC,0,0.0,0.0,ap);
            if(mode >= 4)
                setup_display2(NMPH_RAND,LINEAR,0,0.0,0.0,ap);
        }
        break;
    case(SPECMORPH2):
        setup_display2(NMPH_APKS,NUMERIC,0,0.0,0.0,ap);
        if(mode > 0) {
            setup_display2(NMPH_ASTT,NUMERIC,0,0.0,0.0,ap);
            setup_display2(NMPH_AEND,NUMERIC,0,0.0,0.0,ap);
            setup_display2(NMPH_AEXP,NUMERIC,0,0.0,0.0,ap);
            setup_display2(NMPH_RAND,NUMERIC,0,0.0,0.0,ap);
        }
        break;
    case(SUPERACCU):
        setup_display2(0,LOG,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        break;
    case(PARTITION):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        switch(mode) {
        case(0):
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            break;
        case(1):
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            break;
        }
        break;
    case(SPECGRIDS):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(GLISTEN):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        setup_display2(4,LINEAR,0,0.0,0.0,ap);
        break;
    case(TUNEVARY):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,PLOG,0,0.0,0.0,ap);
        break;
    case(ISOLATE):
        switch(mode) {
        case(ISO_OVRLAP):
            setup_display2(ISO_SPL,NUMERIC,0,0.0,0.0,ap);
            setup_display2(ISO_DOV,NUMERIC,0,0.0,0.0,ap);
            break;
        case(ISO_THRESH):
            setup_display2(ISO_THRON,LINEAR,0,0.0,0.0,ap);
            setup_display2(ISO_THROFF,LINEAR,0,0.0,0.0,ap);
            setup_display2(ISO_SPL,NUMERIC,0,0.0,0.0,ap);
            setup_display2(ISO_MIN,NUMERIC,0,0.0,0.0,ap);
            setup_display2(ISO_LEN,NUMERIC,0,0.0,0.0,ap);
            break;
        default:
            setup_display2(ISO_SPL,NUMERIC,0,0.0,0.0,ap);
            break;
        }
        break;
    case(REJOIN):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(PANORAMA):
        if(mode == 0) {
            setup_display2(PANO_LCNT,NUMERIC,0,0.0,0.0,ap);
            setup_display2(PANO_LWID,NUMERIC,0,0.0,0.0,ap);
        }
        setup_display2(PANO_SPRD,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PANO_OFST,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PANO_CNFG,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PANO_RAND,NUMERIC,0,0.0,0.0,ap);
        break;
    case(TREMOLO):
        setup_display2(0,LINEAR,SUBRANGE,ap->lo[TREMOLO_FRQ],64.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        break;
    case(ECHO):
        setup_display2(ECHO_DELAY,LINEAR,SUBRANGE,duration,30.0,ap);
        setup_display2(ECHO_ATTEN,LINEAR,0,0.0,0.0,ap);
        setup_display2(ECHO_DUR,  LINEAR,SUBRANGE,duration * 2,60.0,ap);
        setup_display2(ECHO_RAND, LINEAR,0,0.0,0.0,ap);
        setup_display2(ECHO_CUT,  NUMERIC,0,0.0,0.0,ap);
        break;
    case(PACKET):
        setup_display2(PAK_DUR,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PAK_SQZ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PAK_CTR,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SYNTHESIZER):
        setup_display2(SYNTHSRAT,SRATE,0,0.0,0.0,ap);
        setup_display2(SYNTH_DUR,NUMERIC,0,0.0,0.0,ap);
        if(mode == 0) {
            setup_display2(SYNTH_FRQ,LINEAR,SUBRANGE,16,4000,ap);
        } else if(mode == 1) {
            setup_display2(SYNTH_FRQ,LINEAR,SUBRANGE,16,4000,ap);
            setup_display2(SYNTH_SQZ,LINEAR,0,0.0,0.0,ap);
            setup_display2(SYNTH_CTR,LINEAR,0,0.0,0.0,ap);
        } else if(mode == 2) {
            setup_display2(SYNTH_FRQ,  LINEAR,SUBRANGE,16,4000,ap);
            setup_display2(SYNTH_CHANS,NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_MAX,  NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_RATE, LINEAR,0,0.0,0.0,ap);
            setup_display2(SYNTH_RISE, NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_FALL, NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_STDY, NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_SPLEN,NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_NUM,  NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_EFROM,NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_ETIME,NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_CTO,  NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_CTIME,NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_STYPE,NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_RSPEED,LINEAR,0,0.0,0.0,ap);
        } else if(mode == 3) {
            setup_display2(SYNTH_FRQ,   LOG,SUBRANGE,.01,440,ap);
            setup_display2(SYNTH_ATK,   NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_EATK,  LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_DEC,   NUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_EDEC,  LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(SYNTH_ATOH,  LINEAR,0,0.0,0.0,ap);
            setup_display2(SYNTH_GTOW,  LINEAR,0,0.0,0.0,ap);
            setup_display2(SYNTH_RAND,  LINEAR,0,0.0,0.0,ap);
            setup_display2(SYNTH_FLEVEL,NUMERIC,0,0.0,0.0,ap);
        }
        break;
    case(NEWTEX):
        setup_display2(NTEX_DUR,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(NTEX_CHANS,NUMERIC,0,0.0,0.0,ap);
        switch(mode) {
        case(0):
            setup_display2(NTEX_MAX,  LINEAR,0,0.0,0.0,ap);
            break;
        case(1):
        case(2):
            setup_display2(NTEX_MAX,  NUMERIC,0,0.0,0.0,ap);
            break;
        }
        setup_display2(NTEX_RATE, LINEAR,0,0.0,0.0,ap);
        setup_display2(NTEX_STYPE,NUMERIC,0,0.0,0.0,ap);
        setup_display2(NTEX_SPLEN,NUMERIC,0,0.0,0.0,ap);
        setup_display2(NTEX_NUM,  LINEAR,0,0.0,0.0,ap);
        switch(mode) {
        case(1):
            setup_display2(NTEX_DEL,LINEAR,SUBRANGE,0,60.0,ap);
            break;
        case(2):
            setup_display2(NTEX_LOC,LINEAR,SUBRANGE,0,60.0,ap);
            setup_display2(NTEX_AMB,LINEAR,SUBRANGE,0,60.0,ap);
            setup_display2(NTEX_GST,LINEAR,SUBRANGE,0,60.0,ap);
            break;
        }
        setup_display2(NTEX_EFROM,LINEAR,0,0.0,0.0,ap);
        setup_display2(NTEX_ETIME,NUMERIC,0,0.0,0.0,ap);
        setup_display2(NTEX_CTO,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(NTEX_CTIME,NUMERIC,0,0.0,0.0,ap);
        setup_display2(NTEX_STYPE,NUMERIC,0,0.0,0.0,ap);
        setup_display2(NTEX_RSPEED,LINEAR,0,0.0,0.0,ap);
        break;
    case(CERACU):
        setup_display2(CER_MINDUR,NUMERIC,0,0.0,0.0,ap);
        setup_display2(CER_OCHANS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(CER_CUTOFF,LINEAR,SUBRANGE,0,60.0,ap);
        setup_display2(CER_DELAY ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(CER_DSTEP ,NUMERIC,0,0.0,0.0,ap);
        break;
    case(MADRID):
        setup_display2(MAD_DUR,  LINEAR,SUBRANGE,0,60.0,ap);
        setup_display2(MAD_CHANS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(MAD_STRMS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(MAD_DELF ,LINEAR,SUBRANGE,0,100.0,ap);
        setup_display2(MAD_STEP ,LINEAR,0,0.0,0.0,ap);
        setup_display2(MAD_RAND ,LINEAR,0,0.0,0.0,ap);
        setup_display2(MAD_SEED ,LINEAR,0,0.0,0.0,ap);
        break;
    case(SHIFTER):
        setup_display2(SHF_CYCDUR,LINEAR,SUBRANGE,0,60.0,ap);
        setup_display2(SHF_OUTDUR,LINEAR,SUBRANGE,0,360.0,ap);
        setup_display2(SHF_OCHANS,LINEAR,0,0.0,0.0,ap);
        setup_display2(SHF_SUBDIV,LINEAR,0,0.0,0.0,ap);
        setup_display2(SHF_LINGER,LINEAR,SUBRANGE,0,256.0,ap);
        setup_display2(SHF_TRNSIT,LINEAR,SUBRANGE,0,256.0,ap);
        setup_display2(SHF_LBOOST,LINEAR,0,0.0,0.0,ap);
        break;
    case(SUBTRACT):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPEKLINE):
        if(mode == 0) {
            setup_display2(SPEKPOINTS,NUMERIC,0,0.0,0.0,ap);
            setup_display2(SPEKHARMS, NUMERIC,0,0.0,0.0,ap);
            setup_display2(SPEKBRITE, NUMERIC,0,0.0,0.0,ap);
            setup_display2(SPEKMAX,   NUMERIC,0,0.0,0.0,ap);
        }
        setup_display2(SPEKSRATE, NUMERIC,0,0.0,0.0,ap); 
        setup_display2(SPEKDUR,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPEKDATLO, NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPEKDATHI, NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPEKSPKLO, NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPEKSPKHI, NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPEKWARP,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPEKAWARP, NUMERIC,0,0.0,0.0,ap);
        break;
    case(FRACTURE):
        setup_display2(FRAC_CHANS ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(FRAC_STRMS ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(FRAC_PULSE, LOG,0,0.0,0.0,ap);
        setup_display2(FRAC_DEPTH, LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_STACK, LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_INRND, LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_OUTRND,LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_SCAT,  LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_LEVRND,LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_ENVRND,LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_STKRND,LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_PCHRND,LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_SEED,  LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_MIN,   LINEAR,0,0.0,0.0,ap);
        setup_display2(FRAC_MAX,   LINEAR,0,0.0,0.0,ap);
        if(mode > 0) {
            setup_display2(FRAC_CENTRE ,NUMERIC,0,0.0,0.0,ap);
            setup_display2(FRAC_FRONT  ,LINEAR,0,0.0,0.0,ap);
            setup_display2(FRAC_MDEPTH ,LINEAR,0,0.0,0.0,ap);
            setup_display2(FRAC_ROLLOFF,LINEAR,0,0.0,0.0,ap);
            setup_display2(FRAC_ATTEN  ,NUMERIC,0,0.0,0.0,ap);
            setup_display2(FRAC_ZPOINT  ,NUMERIC,0,0.0,0.0,ap);
            setup_display2(FRAC_CONTRACT,NUMERIC,0,0.0,0.0,ap);
            setup_display2(FRAC_FPOINT  ,NUMERIC,0,0.0,0.0,ap);
            setup_display2(FRAC_FFACTOR ,NUMERIC,0,0.0,0.0,ap);
            setup_display2(FRAC_FFREQ   ,LINEAR,SUBRANGE,300.0,ap->hi[FRAC_FFREQ],ap);
            setup_display2(FRAC_UP      ,NUMERIC,0,0.0,0.0,ap);
            setup_display2(FRAC_DN      ,NUMERIC,0,0.0,0.0,ap);
            setup_display2(FRAC_GAIN    ,NUMERIC,0,0.0,0.0,ap);
        }
        break;
    case(TAN_ONE):
        setup_display2(TAN_DUR,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_STEPS, NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_MANG,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_DEC,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_FOCUS, NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_JITTER,LINEAR,0,0.0,0.0,ap);
        if(mode==0)
            setup_display2(TAN_SLOW, NUMERIC,0,0.0,0.0,ap);
        break;
    case(TAN_TWO):
        setup_display2(TAN_DUR,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_STEPS, NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_MANG,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_DEC,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_FOCUS, NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_JITTER,LINEAR,0,0.0,0.0,ap);
        setup_display2(TAN_FBAL,  NUMERIC,0,0.0,0.0,ap);
        if(mode==0)
            setup_display2(TAN_SLOW, NUMERIC,0,0.0,0.0,ap);
        break;
    case(TAN_SEQ):
    case(TAN_LIST):
        setup_display2(TAN_DUR,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_MANG,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_DEC,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_FOCUS, NUMERIC,0,0.0,0.0,ap);
        setup_display2(TAN_JITTER,LINEAR,0,0.0,0.0,ap);
        if(mode==0)
            setup_display2(TAN_SLOW, NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECTWIN):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        setup_display2(4,NUMERIC,0,0.0,0.0,ap);
        break;
    case(TRANSIT):
    case(TRANSITF):
    case(TRANSITD):
    case(TRANSITFD):
    case(TRANSITS):
    case(TRANSITL):
        setup_display2(TRAN_FOCUS,LINEAR,0,0.0,0.0,ap);
        setup_display2(TRAN_DUR,LINEAR,SUBRANGE,duration * 2.0,60.0,ap);
        if(process < TRANSITS)
            setup_display2(TRAN_STEPS,LINEAR,SUBRANGE,2,256,ap);
        switch(mode) {
        case(GLANCING):
        case(EDGEWISE):
        case(CROSSING):
        case(CLOSE):
            setup_display2(TRAN_MAXA,LINEAR,0,0.0,0.0,ap);
            break;
        case(CENTRAL):
            setup_display2(TRAN_MAXA,LINEAR,SUBRANGE,1,20,ap);
            break;
        }
        setup_display2(TRAN_DEC,LINEAR,0,0.0,0.0,ap);
        if(process == TRANSITF || process == TRANSITFD)
            setup_display2(TRAN_FBAL,LINEAR,0,0.0,0.0,ap);
        if(process < TRANSITS) {
            setup_display2(TRAN_THRESH,LINEAR,0,0.0,0.0,ap);
            setup_display2(TRAN_DECLIM,LINEAR,0,0.0,0.0,ap);
            setup_display2(TRAN_MINLEV,LINEAR,0,0.0,0.0,ap);
            setup_display2(TRAN_MAXDUR,LINEAR,SUBRANGE,duration * 2.0,60.0,ap);
        }
        break;
    case(CANTOR):
        switch(mode) {
        case(0):
        case(1):
            setup_display2(CA_HOLEN,  LINEAR,0,0.0,0.0,ap);
            setup_display2(CA_HOLEDIG,LINEAR,0,0.0,0.0,ap);
            setup_display2(CA_TRIGLEV,LINEAR,0,0.0,0.0,ap);
            setup_display2(CA_SPLEN,  LINEAR,0,0.0,0.0,ap);
            if(duration * 2.0 < 60.0)
                setup_display2(CA_MAXDUR, LINEAR,SUBRANGE,duration * 2.0,60.0,ap);
            else
                setup_display2(CA_MAXDUR, LINEAR,0,0.0,0.0,ap);
            break;
        case(2):
            setup_display2(CA_HOLEN,  LINEAR,0,0.0,0.0,ap);
            setup_display2(CA_HOLEDIG,LINEAR,0,0.0,0.0,ap);
            setup_display2(CA_WOBBLES,LINEAR,0,0.0,0.0,ap);
            setup_display2(CA_WOBDEC ,LINEAR,0,0.0,0.0,ap);
            if(duration * 2.0 < 60.0)
                setup_display2(CA_MAXDUR, LINEAR,SUBRANGE,duration * 2.0,60.0,ap);
            else
                setup_display2(CA_MAXDUR, LINEAR,0,0.0,0.0,ap);
            break;
        }
        break;
    case(SHRINK):
        if(mode == SHRM_TIMED)
            setup_display2(SHR_TIME,LINEAR,0,0.0,0.0,ap);
        setup_display2(SHR_INK,   LINEAR,0,0.0,0.0,ap);
        if(mode >= SHRM_FINDMX) {
            setup_display2(SHR_WSIZE,LINEAR,0,0.0,0.0,ap);
            setup_display2(SHR_AFTER,LINEAR,0,0.0,0.0,ap);
        } else {
            setup_display2(SHR_GAP,   LINEAR,0,0.0,0.0,ap);
            setup_display2(SHR_DUR,   LINEAR,SUBRANGE,duration * 2.0,60.0,ap);
        }
        setup_display2(SHR_CNTRCT,LINEAR,0,0.0,0.0,ap);
        setup_display2(SHR_SPLEN, LINEAR,0,0.0,0.0,ap);
        setup_display2(SHR_SMALL, LINEAR,0,0.0,0.0,ap);
        setup_display2(SHR_MIN,   LINEAR,SUBRANGE,0,60.0,ap);
        setup_display2(SHR_RAND,  LINEAR,0,0.0,0.0,ap);
        if(mode >= SHRM_FINDMX) {
            setup_display2(SHR_GATE,LINEAR,0,0.0,0.0,ap);
            setup_display2(SHR_LEN, LINEAR,0,0.0,0.0,ap);
        }
        if(mode == SHRM_FINDMX)
            setup_display2(SHR_SKEW,LINEAR,0,0.0,0.0,ap);
        break;
    case(NEWDELAY):
        if(mode == 0) {
            setup_display2(DELAY_DELAY,LINEAR,0,0.0,0.0,ap);
            setup_display2(DELAY_MIX,NUMERIC,0,0.0,0.0,ap);
            setup_display2(DELAY_FEEDBACK,NUMERIC,0,0.0,0.0,ap);
        } else {
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            setup_display2(3,NUMERIC,0,0.0,0.0,ap);
            setup_display2(4,NUMERIC,0,0.0,0.0,ap);
            setup_display2(5,NUMERIC,0,0.0,0.0,ap);
        }
        break;
    case(ITERLINE):
    case(ITERLINEF):
        setup_display2(ITER_DUR,LINEAR,SUBRANGE,0,60.0,ap);     
        setup_display2(ITER_DELAY,LINEAR,SUBRANGE,ap->lo[ITER_DELAY],min(ap->hi[ITER_DELAY],duration),ap);
        setup_display2(ITER_RANDOM,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITER_PSCAT,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITER_ASCAT,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITER_RSEED,LINEAR,SUBRANGE,ap->lo[ITER_RSEED],min(ap->hi[ITER_RSEED],64.0),ap);
        setup_display2(ITER_GAIN,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECRAND):
        setup_display2(0,LOG,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        break;
    case(SPECSQZ):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        break;
    case(FILTRAGE):
        setup_display2(FILTR_DUR,NUMERIC,0,0.0,0.0,ap);
        setup_display2(FILTR_CNT,NUMERIC,0,0.0,0.0,ap);
        setup_display2(FILTR_MMIN,NUMERIC,0,0.0,0.0,ap);
        setup_display2(FILTR_MMAX,NUMERIC,0,0.0,0.0,ap);
        setup_display2(FILTR_DIS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(FILTR_RND,NUMERIC,0,0.0,0.0,ap);
        setup_display2(FILTR_AMIN,NUMERIC,0,0.0,0.0,ap);
        setup_display2(FILTR_ARND ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(FILTR_ADIS,NUMERIC,0,0.0,0.0,ap);
        if(mode == 1) {
            setup_display2(FILTR_STEP,NUMERIC,0,0.0,0.0,ap);
            setup_display2(FILTR_SRND,NUMERIC,0,0.0,0.0,ap);
        }
        setup_display2(FILTR_SEED,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SELFSIM):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(ITERFOF):
        setup_display2(ITF_DEL,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITF_DUR,NUMERIC,0,0.0,0.0,ap);
        setup_display2(ITF_PRND,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITF_AMPC,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITF_TRIM,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITF_TRBY,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITF_SLOP,NUMERIC,0,0.0,0.0,ap);
        setup_display2(ITF_RAND,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITF_VMIN,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITF_VMAX,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITF_DMIN,LINEAR,0,0.0,0.0,ap);
        setup_display2(ITF_DMAX,LINEAR,0,0.0,0.0,ap);
        if(EVEN(mode))
            setup_display2(ITF_SEED1,NUMERIC,0,0.0,0.0,ap);
        else {
            setup_display2(ITF_GMIN,LINEAR,0,0.0,0.0,ap);
            setup_display2(ITF_GMAX,LINEAR,0,0.0,0.0,ap);
            setup_display2(ITF_UFAD,LINEAR,0,0.0,0.0,ap);
            setup_display2(ITF_FADE,LINEAR,0,0.0,0.0,ap);
            setup_display2(ITF_GAPP,LINEAR,0,0.0,0.0,ap);
            setup_display2(ITF_PORT,LINEAR,0,0.0,0.0,ap);
            setup_display2(ITF_PINT,LINEAR,0,0.0,0.0,ap);
            setup_display2(ITF_SEED2,NUMERIC,0,0.0,0.0,ap);
        }
        break;
    case(PULSER):
    case(PULSER2):
    case(PULSER3):
        setup_display2(PLS_DUR     ,NUMERIC,SUBRANGE,0.0,60.0,ap);
        if(process == PULSER3 || mode == 0)
            setup_display2(PLS_PITCH,LINEAR,0,0.0,0.0,ap);
        setup_display2(PLS_MINRISE ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PLS_MAXRISE ,NUMERIC,0,0.0,0.0,ap);
        if(process != PULSER3 && mode == 2) {
            setup_display2(PLS_MINSUS  ,LINEAR,0,0.0,0.0,ap);
            setup_display2(PLS_MAXSUS  ,LINEAR,0,0.0,0.0,ap);
        } else {
            setup_display2(PLS_MINSUS  ,NUMERIC,0,0.0,0.0,ap);
            setup_display2(PLS_MAXSUS  ,NUMERIC,0,0.0,0.0,ap);
        }
        setup_display2(PLS_MINDECAY,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PLS_MAXDECAY,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PLS_SPEED   ,LINEAR,0,0.0,0.0,ap);
        setup_display2(PLS_SCAT    ,LINEAR,0,0.0,0.0,ap);
        setup_display2(PLS_EXP     ,LINEAR,0,0.0,0.0,ap);
        setup_display2(PLS_EXP2    ,LINEAR,0,0.0,0.0,ap);
        setup_display2(PLS_PSCAT   ,LINEAR,0,0.0,0.0,ap);
        setup_display2(PLS_ASCAT   ,LINEAR,0,0.0,0.0,ap);
        setup_display2(PLS_OCT     ,LINEAR,0,0.0,0.0,ap);
        setup_display2(PLS_BEND    ,LINEAR,0,0.0,0.0,ap);
        setup_display2(PLS_SEED    ,NUMERIC,0,0.0,0.0,ap);
        if(process == PULSER3) {
            setup_display2(PLS_SRATE,SRATE,0,0.0,0.0,ap);
            setup_display2(PLS_CNT,  LINEAR,0,0.0,0.0,ap);
        } else if(mode == 2)
            setup_display2(PLS_WIDTH,LINEAR,0,0.0,0.0,ap);
        break;
    case(CHIRIKOV):
        setup_display2(CHIR_DUR  ,NUMERIC,SUBRANGE,0.0,60.0,ap);
        setup_display2(CHIR_FRQ  ,LOG,0,0.0,0.0,ap);
        setup_display2(CHIR_DAMP ,LINEAR,0,0.0,0.0,ap);
        if(mode < 2) {
            setup_display2(CHIR_SRATE,SRATE,0,0.0,0.0,ap);
            setup_display2(CHIR_SPLEN,NUMERIC,0,0.0,0.0,ap);
        } else {
            setup_display2(CHIR_PMIN, LINEAR,0,0.0,0.0,ap);
            setup_display2(CHIR_PMAX, LINEAR,0,0.0,0.0,ap);
            setup_display2(CHIR_STEP, LINEAR,0,0.0,0.0,ap);
            setup_display2(CHIR_RAND, LINEAR,0,0.0,0.0,ap);
        }
        break;
    case(MULTIOSC):
        setup_display2(MOSC_DUR  ,NUMERIC,SUBRANGE,0.0,60.0,ap);
        setup_display2(MOSC_FRQ1 ,LOG,0,0.0,0.0,ap);
        setup_display2(MOSC_FRQ2 ,LOG,0,0.0,0.0,ap);
        setup_display2(MOSC_AMP2, LINEAR,0,0.0,0.0,ap);
        if(mode >= 1) {
            setup_display2(MOSC_FRQ3 ,LOG,0,0.0,0.0,ap);
            setup_display2(MOSC_AMP3, LINEAR,0,0.0,0.0,ap);
        }
        if(mode == 2) {
            setup_display2(MOSC_FRQ4 ,LOG,0,0.0,0.0,ap);
            setup_display2(MOSC_AMP4, LINEAR,0,0.0,0.0,ap);
        }
        setup_display2(MOSC_SRATE,SRATE,0,0.0,0.0,ap);
        setup_display2(MOSC_SPLEN,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SYNFILT):
        setup_display2(SYNFLT_SRATE  ,SRATE,0,0.0,0.0,ap);
        setup_display2(SYNFLT_CHANS  ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SYNFLT_Q      ,LINEAR,SUBRANGE,1.0,64.0,ap);
        setup_display2(SYNFLT_HARMCNT,LINEAR,SUBRANGE,ap->lo[SYNFLT_HARMCNT],64.0,ap);
        setup_display2(SYNFLT_ROLLOFF,LINEAR,0,0.0,0.0,ap);
        setup_display2(SYNFLT_SEED   ,NUMERIC,0,0.0,0.0,ap);
        break;
    case(STRANDS):
        setup_display2(STRAND_DUR   ,NUMERIC,SUBRANGE,0.0,60.0,ap);
        setup_display2(STRAND_BANDS ,NUMERIC,0,0.0,0.0,ap);
        if(mode != 2)
            setup_display2(STRAND_THRDS ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STRAND_TSTEP ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STRAND_BOT   ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STRAND_TOP   ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STRAND_TWIST ,LINEAR,0,0.0,0.0,ap);
        setup_display2(STRAND_RAND  ,LINEAR,0,0.0,0.0,ap);
        setup_display2(STRAND_SCAT  ,LINEAR,0,0.0,0.0,ap);
        setup_display2(STRAND_VAMP  ,LINEAR,0,0.0,0.0,ap);
        setup_display2(STRAND_VMIN  ,LINEAR,0,0.0,0.0,ap);
        setup_display2(STRAND_VMAX  ,LINEAR,0,0.0,0.0,ap);
        setup_display2(STRAND_TURB  ,LINEAR,0,0.0,0.0,ap);
        setup_display2(STRAND_SEED  ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STRAND_GAP   ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STRAND_MINB  ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STRAND_3D    ,NUMERIC,0,0.0,0.0,ap);
        break;
    case(REFOCUS):
        setup_display2(REFOC_DUR   ,NUMERIC,SUBRANGE,0.0,60.0,ap);
        setup_display2(REFOC_BANDS ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(REFOC_RATIO ,LINEAR,0,0.0,0.0,ap);
        setup_display2(REFOC_TSTEP ,LINEAR,0,0.0,0.0,ap);
        setup_display2(REFOC_RAND  ,LINEAR,0,0.0,0.0,ap);
        setup_display2(REFOC_OFFSET,NUMERIC,SUBRANGE,0.0,60.0,ap);
        setup_display2(REFOC_END   ,NUMERIC,SUBRANGE,0.0,60.0,ap);
        setup_display2(REFOC_XCPT  ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(REFOC_SEED  ,NUMERIC,0,0.0,0.0,ap);
        break;
    case(CHANPHASE):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SILEND):
        if(mode== 0)
            setup_display2(0,NUMERIC,SUBRANGE,FLTERR,20.0,ap);
        else
            setup_display2(0,NUMERIC,SUBRANGE,duration+FLTERR,duration+20.0,ap);
        break;
    case(SPECULATE):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECTUNE):
        setup_display2(ST_MATCH,NUMERIC,0,0.0,0.0,ap);
        setup_display2(ST_LOPCH,NUMERIC,0,0.0,0.0,ap);
        setup_display2(ST_HIPCH,NUMERIC,0,0.0,0.0,ap);
        setup_display2(ST_STIME,NUMERIC,0,0.0,0.0,ap);
        setup_display2(ST_ETIME,NUMERIC,0,0.0,0.0,ap);
        setup_display2(ST_INTUN,NUMERIC,0,0.0,0.0,ap);
        setup_display2(ST_WNDWS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(ST_NOISE,NUMERIC,0,0.0,0.0,ap);
        break;
    case(REPAIR):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(DISTSHIFT):
        setup_display2(0,NUMERIC,SUBRANGE,1,64,ap);
        if(mode==0)
            setup_display2(1,NUMERIC,SUBRANGE,1,256,ap);
        break;
    case(QUIRK):
        setup_display2(0,LOGNUMERIC,0,0.0,0.0,ap);
        break;
    case(ROTOR):
        setup_display2(ROT_CNT,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(ROT_PMIN, LINEAR,0,0.0,0.0,ap);
        setup_display2(ROT_PMAX, LINEAR,0,0.0,0.0,ap);
        setup_display2(ROT_NSTEP,LINEAR,0,0.0,0.0,ap);
        setup_display2(ROT_PCYC, LINEAR,0,0.0,0.0,ap);
        setup_display2(ROT_TCYC, LINEAR,0,0.0,0.0,ap);
        setup_display2(ROT_PHAS, NUMERIC,0,0.0,0.0,ap);
        setup_display2(ROT_DUR,  NUMERIC,SUBRANGE,1.0,60.0,ap);
        if(mode == 0)
            setup_display2(ROT_GSTEP,LINEAR,0,0.0,0.0,ap);
        setup_display2(ROT_DOVE, NUMERIC,0,0.0,0.0,ap);
        break;
    case(DISTCUT):
        setup_display2(DCUT_CNT,LINEAR,SUBRANGE,1,128,ap);
        if(mode==1)
            setup_display2(DCUT_STP,LINEAR,SUBRANGE,1,128,ap);
        setup_display2(DCUT_EXP,LOG,SUBRANGE,0.25,4.0,ap);
        setup_display2(DCUT_LIM,NUMERIC,0,0.0,0.0,ap);
        break;
    case(ENVCUT):
        setup_display2(ECUT_CNT,LINEAR,0,0.0,0.0,ap);
        if(mode==1)
            setup_display2(ECUT_STP,LINEAR,0,0.0,0.0,ap);
        setup_display2(ECUT_ATT,LINEAR,0,0.0,0.0,ap);
        setup_display2(ECUT_EXP,LOG,SUBRANGE,0.25,4.0,ap);
        setup_display2(ECUT_LIM,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECFOLD):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        if(mode != 1)
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        break;
    case(BROWNIAN):
        setup_display2(BRCHANS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(BRDUR,  NUMERIC,SUBRANGE,duration,60.0,ap);
        if(mode==0) {
            setup_display2(BRATT,LINEAR,0,0.0,0.0,ap);
            setup_display2(BRDEC,LINEAR,0,0.0,0.0,ap);
        }
        setup_display2(BRPLO,  LINEAR,0,0.0,0.0,ap);
        setup_display2(BRPHI,  LINEAR,0,0.0,0.0,ap);
        setup_display2(BRPSTT, NUMERIC,0,0.0,0.0,ap);
        setup_display2(BRSSTT, NUMERIC,0,0.0,0.0,ap);
        setup_display2(BRPSTEP,LINEAR,0,0.0,0.0,ap);
        setup_display2(BRSSTEP,LINEAR,0,0.0,0.0,ap);
        setup_display2(BRTICK, LINEAR,0,0.0,0.0,ap);
        setup_display2(BRSEED, NUMERIC,0,0.0,0.0,ap);
        setup_display2(BRASTEP,LINEAR,0,0.0,0.0,ap);
        setup_display2(BRAMIN, LINEAR,0,0.0,0.0,ap);
        if(mode==0) {
            setup_display2(BRASLP,LINEAR,0,0.0,0.0,ap);
            setup_display2(BRDSLP,LINEAR,0,0.0,0.0,ap);
        }
        break;
    case(SPIN):
        setup_display2(SPNRATE, LINEAR,0,0.0,0.0,ap);
        setup_display2(SPNBOOST,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPNATTEN,NUMERIC,0,0.0,0.0,ap);
        if(mode > 0) {
            setup_display2(SPNOCHNS,NUMERIC,0,0.0,0.0,ap);
            setup_display2(SPNOCNTR,NUMERIC,0,0.0,0.0,ap);
            setup_display2(SPNCMIN, NUMERIC,0,0.0,0.0,ap);
            if(mode==1)
                setup_display2(SPNCMAX, NUMERIC,0,0.0,0.0,ap);
        }
        setup_display2(SPNDOPL, NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPNXBUF, NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPINQ):
        setup_display2(SPNRATE, LINEAR,0,0.0,0.0,ap);
        setup_display2(SPNBOOST,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPNATTEN,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPNOCHNS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPNOCNTR,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPNDOPL, NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPNXBUF, NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPNCMIN, NUMERIC,0,0.0,0.0,ap);
        if(mode == 0)
            setup_display2(SPNCMAX, NUMERIC,0,0.0,0.0,ap);
        break;
    case(CRUMBLE):
        setup_display2(CRSTART, NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRSTEP1, NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRSTEP2, NUMERIC,0,0.0,0.0,ap);
        if(mode == 1)
            setup_display2(CRSTEP3, NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRORIENT,NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRSIZE,  LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRRAND,  LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRISCAT, LINEAR, 0,0.0,0.0,ap);
        setup_display2(CROSCAT, LINEAR, 0,0.0,0.0,ap);
        setup_display2(CROSTR,  LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRPSCAT, LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRSEED,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRSPLICE,NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRTAIL  ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRDUR   ,NUMERIC,0,0.0,0.0,ap);
        break;
    case(PHASOR):
        setup_display2(PHASOR_STREAMS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(PHASOR_FRQ,    LOG,0,0.0,0.0,ap);
        setup_display2(PHASOR_SHIFT,  LINEAR, 0,0.0,0.0,ap);
        setup_display2(PHASOR_OCHANS, NUMERIC,0,0.0,0.0,ap);
        setup_display2(PHASOR_OFFSET, NUMERIC,0,0.0,0.0,ap);
        break;
    case(TESSELATE):
        setup_display2(TESS_CHANS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(TESS_PHRAS,NUMERIC,0,0.0,0.0,ap);
        setup_display2(TESS_DUR  ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(TESS_TYP  ,NUMERIC,0,0.0,0.0,ap);
        break;
    case(CRYSTAL):
        setup_display2(CRY_ROTA,    LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRY_ROTB,    LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRY_TWIDTH,  LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRY_TSTEP,   LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRY_DUR,     NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRY_PLO,     LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRY_PHI,     LINEAR, 0,0.0,0.0,ap);
        setup_display2(CRY_FPASS,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRY_FSTOP,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRY_FATT,    NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRY_FPRESC,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRY_FSLOPE,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(CRY_SSLOPE,  NUMERIC,0,0.0,0.0,ap);
        break;
    case(WAVEFORM):
        setup_display2(WF_TIME,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(WF_CNT,    NUMERIC,0,0.0,0.0,ap);
        if(mode == 2)
            setup_display2(WF_BAL,NUMERIC,0,0.0,0.0,ap);
        break;
    case(DVDWIND):
        setup_display2(0,LINEAR, 0,0.0,0.0,ap);
        setup_display2(1,LINEAR, 0,0.0,0.0,ap);
        break;
    case(CASCADE):
        if(mode <5) {
            setup_display2(CAS_CLIP,    LINEAR, 0,0.0,0.0,ap);
            setup_display2(CAS_MAXCLIP, LINEAR, 0,0.0,0.0,ap);
        }
        setup_display2(CAS_ECHO,    LINEAR, 0,0.0,0.0,ap);
        setup_display2(CAS_MAXECHO, LINEAR, 0,0.0,0.0,ap);
        setup_display2(CAS_RAND,    LINEAR, 0,0.0,0.0,ap);
        setup_display2(CAS_SEED,    NUMERIC,0,0.0,0.0,ap);
        setup_display2(CAS_SHREDNO, LINEAR, 0,0.0,0.0,ap);
        setup_display2(CAS_SHREDCNT,LINEAR, 0,0.0,0.0,ap);
        break;
    case(SYNSPLINE):
        setup_display2(SPLIN_SRATE, SRATE,0,0.0,0.0,ap);
        setup_display2(SPLIN_DUR,   NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPLIN_FRQ,   LOG,SUBRANGE,16,4000,ap);
        setup_display2(SPLIN_CNT,   LINEAR, 0,0.0,0.0,ap);
        setup_display2(SPLIN_INTP,  LINEAR, 0,0.0,0.0,ap);
        setup_display2(SPLIN_SEED,  NUMERIC,0,0.0,0.0,ap);
        setup_display2(SPLIN_MCNT,  LINEAR, 0,0.0,0.0,ap);
        setup_display2(SPLIN_MINTP, LINEAR, 0,0.0,0.0,ap);
        setup_display2(SPLIN_DRIFT, LINEAR, 0,0.0,0.0,ap);
        setup_display2(SPLIN_DRVEL, LINEAR, 0,0.0,0.0,ap);
        break;
    case(SPLINTER):
        setup_display2(SPL_TIME,  NUMERIC,0,0.0,0.0,ap); 
        setup_display2(SPL_WCNT,  NUMERIC,0,0.0,0.0,ap); 
        setup_display2(SPL_SHRCNT,NUMERIC,0,0.0,0.0,ap); 
        setup_display2(SPL_OCNT  ,NUMERIC,0,0.0,0.0,ap); 
        setup_display2(SPL_PULS1 ,NUMERIC,0,0.0,0.0,ap); 
        setup_display2(SPL_PULS2 ,NUMERIC,0,0.0,0.0,ap); 
        setup_display2(SPL_ECNT  ,NUMERIC,0,0.0,0.0,ap); 
        setup_display2(SPL_SCURVE,NUMERIC,0,0.0,0.0,ap); 
        setup_display2(SPL_PCURVE,NUMERIC,0,0.0,0.0,ap); 
        setup_display2(SPL_FRQ   ,NUMERIC,0,0.0,0.0,ap);    //  and SPL_DUR
        setup_display2(SPL_RND   ,LINEAR, 0,0.0,0.0,ap); 
        setup_display2(SPL_SHRND ,LINEAR, 0,0.0,0.0,ap); 
        break;
    case(REPEATER):
        if(mode >= 2) {
            setup_display2(REP_ACCEL,NUMERIC,0,0.0,0.0,ap); 
            setup_display2(REP_WARP ,NUMERIC,0,0.0,0.0,ap); 
            setup_display2(REP_FADE ,NUMERIC,0,0.0,0.0,ap); 
        }
        setup_display2(REP_RAND ,LINEAR, 0,0.0,0.0,ap); 
        setup_display2(REP_TRNSP,LINEAR, 0,0.0,0.0,ap); 
        setup_display2(REP_SEED ,NUMERIC,0,0.0,0.0,ap); 
        break;
    case(VERGES):
        setup_display2(VRG_TRNSP,LINEAR,0,0.0,0.0,ap); 
        setup_display2(VRG_CURVE,LINEAR,0,0.0,0.0,ap); 
        setup_display2(VRG_DUR  ,LINEAR,0,0.0,0.0,ap); 
        break;
    case(MOTOR):
        setup_display2(MOT_DUR   ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(MOT_FRQ   ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_PULSE ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_FRATIO,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_PRATIO,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_SYM   ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_FRND  ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_PRND  ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_JIT   ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_TREM  ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_SYMRND,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_EDGE  ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_BITE  ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_VARY  ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(MOT_SEED  ,NUMERIC,0,0.0,0.0,ap);
        break;
    case(STUTTER):
        setup_display2(STUT_DUR   ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STUT_JOIN  ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STUT_SIL   ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STUT_SILMIN,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STUT_SILMAX,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STUT_SEED  ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(STUT_TRANS ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(STUT_ATTEN ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(STUT_BIAS  ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(STUT_MINDUR,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SCRUNCH):
        if(mode <= 1)
            setup_display2(SCR_DUR ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SCR_SEED,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SCR_CNT  ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SCR_TRNS ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(SCR_ATTEN,LINEAR, 0,0.0,0.0,ap);
        break;
    case(IMPULSE):
        setup_display2(IMP_DUR  ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(IMP_PICH ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(IMP_CHIRP,LINEAR, 0,0.0,0.0,ap);
        setup_display2(IMP_SLOPE,LINEAR, 0,0.0,0.0,ap);
        setup_display2(IMP_CYCS ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(IMP_LEV  ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(IMP_GAP  ,LINEAR, 0,0.0,0.0,ap);
        setup_display2(IMP_SRATE,SRATE,  0,0.0,0.0,ap);
        setup_display2(IMP_CHANS,NUMERIC,0,0.0,0.0,ap);
        break;
    case(TWEET):
        setup_display2(TWT_PDAT,FILENAME,0,0.0,0.0,ap);
        setup_display2(TWT_MIN,NUMERIC,0,0.0,0.0,ap);
        switch(mode) {
        case(0):
            setup_display2(TWT_PKCNT,LINEAR, 0,0.0,0.0,ap);
            setup_display2(TWT_CHIRP,LINEAR, 0,0.0,0.0,ap);;
            break;
        case(1):
            setup_display2(TWT_PKCNT,LOG   , 0,0.0,0.0,ap);
            setup_display2(TWT_CHIRP,LINEAR, 0,0.0,0.0,ap);
            break;
        }
        break;
    case(RRRR_EXTEND):  // Version 8+
        if(mode == 0) {
            setup_display2(RRR_START,NUMERIC,0,0.0,0.0,ap);
            setup_display2(RRR_END,NUMERIC,0,0.0,0.0,ap);
        } else {
            setup_display2(RRR_GATE,NUMERIC,0,0.0,0.0,ap);
            setup_display2(RRR_SKIP,NUMERIC,0,0.0,0.0,ap);
            setup_display2(RRR_GRSIZ,LINEAR,0,0.0,0.0,ap);
        }
        setup_display2(RRR_SLOW,LINEAR,0,0.0,0.0,ap);
        setup_display2(RRR_REGU,LINEAR,0,0.0,0.0,ap);
        setup_display2(RRR_STRETCH,LOGNUMERIC,0,0.0,0.0,ap);
        setup_display2(RRR_GET,NUMERIC,0,0.0,0.0,ap);
        setup_display2(RRR_RANGE,LINEAR,0,0.0,0.0,ap);
        setup_display2(RRR_REPET,LINEAR,SUBRANGE,1.0,3.0,ap);
        setup_display2(RRR_ASCAT,LINEAR,0,0.0,0.0,ap);
        setup_display2(RRR_PSCAT,LINEAR,SUBRANGE,0.0,1.0,ap);
        break;
    case(SORTER):
        setup_display2(SORTER_SIZE,  LINEAR,0,0.0,0.0,ap);
        if(mode == 4)
            setup_display2(SORTER_SEED,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SORTER_SMOOTH,NUMERIC,0,0.0,0.0,ap);
        setup_display2(SORTER_OMIDI, LINEAR,0,0.0,0.0,ap);
        setup_display2(SORTER_IMIDI, NUMERIC,0,0.0,0.0,ap);
        setup_display2(SORTER_META,  NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECFNU):
        switch(mode) {
        case(F_NARROW):
            setup_display2(NARROWING,LINEAR,0,0.0,0.0,ap);
            setup_display2(NARSUPRES,NUMERIC,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            break;
        case(F_SQUEEZE):
            setup_display2(SQZFACT,  LINEAR,0,0.0,0.0,ap);
            setup_display2(SQZAT,    NUMERIC,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            break;
        case(F_INVERT):
            setup_display2(FVIB,     LINEAR,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            break;
        case(F_ROTATE):
            setup_display2(RSPEED,   LINEAR,SUBRANGE,-4.0,4.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            break;
        case(F_NEGATE):
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            break;
        case(F_SUPPRESS):
            setup_display2(SUPRF,    NUMERIC,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            break;
        case(F_MAKEFILT):
            setup_display2(FPKCNT,   NUMERIC,0,0.0,0.0,ap);
            setup_display2(FBELOW,   NUMERIC,0,0.0,0.0,ap);
            break;
        case(F_MOVE):
            setup_display2(FMOVE1,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FMOVE2,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FMOVE3,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FMOVE4,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FMVGAIN,  LOGNUMERIC,0,0.0,0.0,ap);
            break;
        case(F_MOVE2):
            setup_display2(FMOVE1,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FMOVE2,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FMOVE3,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FMOVE4,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FMVGAIN,  LOGNUMERIC,0,0.0,0.0,ap);
            break;
        case(F_SYLABTROF):
            setup_display2(FMINSYL,  NUMERIC,0,0.0,0.0,ap);
            setup_display2(FMINPKG,  NUMERIC,0,0.0,0.0,ap);
            break;
        case(F_ARPEG):
            setup_display2(FARPRATE, LINEAR,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            break;
        case(F_OCTSHIFT):
            setup_display2(COLINT,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(COL_LO,   NUMERIC,SUBRANGE,0.0,200.0,ap);
            setup_display2(COL_HI,   NUMERIC,SUBRANGE,6000.0,10000.0,ap);
            setup_display2(COLRATE,  LINEAR,0,0.0,0.0,ap);
            break;
        case(F_TRANS):
            setup_display2(COLFLT,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(COL_LO,   NUMERIC,0,0.0,0.0,ap);
            setup_display2(COL_HI,   NUMERIC,0,0.0,0.0,ap);
            setup_display2(COLRATE,  LINEAR,0,0.0,0.0,ap);
            break;
        case(F_FRQSHIFT):
            setup_display2(COLFLT,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(COL_LO,   NUMERIC,SUBRANGE,0.0,200.0,ap);
            setup_display2(COL_HI,   NUMERIC,SUBRANGE,6000.0,10000.0,ap);
            setup_display2(COLRATE,  LINEAR,0,0.0,0.0,ap);
            break;
        case(F_RESPACE):
            setup_display2(COLFLT,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(COL_LO,   NUMERIC,SUBRANGE,0.0,200.0,ap);
            setup_display2(COL_HI,   NUMERIC,SUBRANGE,6000.0,10000.0,ap);
            setup_display2(COLRATE,  LINEAR,0,0.0,0.0,ap);
            break;
        case(F_PINVERT):
            setup_display2(COLFLT,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(COL_LO,   NUMERIC,SUBRANGE,0.0,200.0,ap);
            setup_display2(COL_HI,   NUMERIC,SUBRANGE,6000.0,10000.0,ap);
            setup_display2(COLRATE,  LINEAR,0,0.0,0.0,ap);
            setup_display2(COLLOPCH, NUMERIC,0,0.0,0.0,ap);
            setup_display2(COLHIPCH, NUMERIC,0,0.0,0.0,ap);
            break;
        case(F_PEXAGG):
            setup_display2(COLFLT,   LINEAR,0,0.0,0.0,ap);
            setup_display2(EXAGRANG, LINEAR,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(COL_LO,   NUMERIC,SUBRANGE,0.0,200.0,ap);
            setup_display2(COL_HI,   NUMERIC,SUBRANGE,6000.0,10000.0,ap);
            setup_display2(COLRATE,  LINEAR,0,0.0,0.0,ap);
            setup_display2(COLLOPCH, NUMERIC,0,0.0,0.0,ap);
            setup_display2(COLHIPCH, NUMERIC,0,0.0,0.0,ap);
            break;
        case(F_PQUANT):
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(COL_LO,   NUMERIC,SUBRANGE,0.0,200.0,ap);
            setup_display2(COL_HI,   NUMERIC,SUBRANGE,6000.0,10000.0,ap);
            setup_display2(COLRATE,  LINEAR,0,0.0,0.0,ap);
            setup_display2(COLLOPCH, NUMERIC,0,0.0,0.0,ap);
            setup_display2(COLHIPCH, NUMERIC,0,0.0,0.0,ap);
            break;
        case(F_PCHRAND):
            setup_display2(FPRMAXINT,LINEAR,0,0.0,0.0,ap);
            setup_display2(FSLEW,    LOG,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(COL_LO,   NUMERIC,SUBRANGE,0.0,200.0,ap);
            setup_display2(COL_HI,   NUMERIC,SUBRANGE,6000.0,10000.0,ap);
            setup_display2(COLRATE,  LINEAR,0,0.0,0.0,ap); 
            setup_display2(COLLOPCH, NUMERIC,0,0.0,0.0,ap);
            setup_display2(COLHIPCH, NUMERIC,0,0.0,0.0,ap);
            break;
        case(F_RAND):
            setup_display2(COLFLT,   LINEAR,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(COL_LO,   NUMERIC,SUBRANGE,0.0,200.0,ap);
            setup_display2(COL_HI,   NUMERIC,SUBRANGE,6000.0,10000.0,ap);
            setup_display2(COLRATE,  LINEAR,0,0.0,0.0,ap);
            break;
        case(F_SINUS):
            setup_display2(F_SINING, LINEAR,0,0.0,0.0,ap);
            setup_display2(FGAIN,    LOGNUMERIC,0,0.0,0.0,ap);
            setup_display2(F_AMP1,   LINEAR,0,0.0,0.0,ap);
            setup_display2(F_AMP2,   LINEAR,0,0.0,0.0,ap);
            setup_display2(F_AMP3,   LINEAR,0,0.0,0.0,ap);
            setup_display2(F_AMP4,   LINEAR,0,0.0,0.0,ap);
            setup_display2(F_QDEP1,  LINEAR,0,0.0,0.0,ap);
            setup_display2(F_QDEP2,  LINEAR,0,0.0,0.0,ap);
            setup_display2(F_QDEP3,  LINEAR,0,0.0,0.0,ap);
            setup_display2(F_QDEP4,  LINEAR,0,0.0,0.0,ap);
            break;
        }
        break;
    case(FLATTEN):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        break;
    case(BOUNCE):
        setup_display2(BNC_NUMBER, NUMERIC,0,0.0,0.0,ap);
        setup_display2(BNC_STTSTEP,NUMERIC,0,0.0,0.0,ap);
        setup_display2(BNC_SHORTEN,NUMERIC,0,0.0,0.0,ap);
        setup_display2(BNC_ENDLEV, NUMERIC,0,0.0,0.0,ap);
        setup_display2(BNC_LEVWRP, NUMERIC,0,0.0,0.0,ap);
        setup_display2(BNC_MINDUR, NUMERIC,0,0.0,0.0,ap);
        break;
    case(DISTMARK):
        setup_display2(0,LINEAR,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        if(mode == 1)
            setup_display2(3,LINEAR,0,0.0,0.0,ap);
        break;
    case(DISTREP):
        setup_display2(0,LINEAR,SUBRANGE,2,64,ap);
        setup_display2(1,LINEAR,SUBRANGE,1,64,ap);
        setup_display2(2,LINEAR,SUBRANGE,1,64,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        break;
    case(TOSTEREO):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        setup_display2(4,NUMERIC,0,0.0,0.0,ap);
        setup_display2(5,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SUPPRESS):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        break;
    case(CALTRAIN):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECENV):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(CLIP):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECEX):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,SUBRANGE,2,64,ap);
        setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        break;
    case(MATRIX):
        switch(mode) {
        case(MATRIX_USE):
            break;
        default:
            setup_display2(0,POWTWO,0,0.0,0.0,ap);
            setup_display2(1,NUMERIC,0,0.0,0.0,ap);
            break;
        }
        break;
    case(TRANSPART):
        if(mode < 4) {
            setup_display2(0,LINEAR,0,0.0,0.0,ap);
        } else {
            setup_display2(0,LINEAR,SUBRANGE,-1000,1000,ap);
        }
        setup_display2(1,LINEAR,SUBRANGE,-1000,1000,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        break;
    case(SPECINVNU):
        setup_display2(0,NUMERIC,SUBRANGE,100.0,1000.0,ap);
        setup_display2(1,NUMERIC,SUBRANGE,100.0,6000.0,ap);
        setup_display2(2,NUMERIC,SUBRANGE,2,64,ap);
        setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECCONV):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(SPECSND):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        break;
    case(FRACTAL):
        if(mode == 1)
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        break;
    case(FRACSPEC):
        setup_display2(1,LINEAR,0,0.0,0.0,ap);
        setup_display2(2,LINEAR,0,0.0,0.0,ap);
        setup_display2(3,LINEAR,0,0.0,0.0,ap);
        break;
    case(SPECFRAC):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    case(ENVSPEAK):
        if(mode < 12)
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        mode %= 12;
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        if(mode < 9) {                              //  mode 9 has no further params
            setup_display2(2,NUMERIC,0,0.0,0.0,ap);
            switch(mode) {                              
            case(6)://  fall thro                   //  all have 6 params
            case(7)://  fall thro
            case(8):
                setup_display2(5,LINEAR,0,0.0,0.0,ap);
                //  fall thro
            case(0)://  fall thro                   //  all have 5 params
            case(2)://  fall thro
            case(3)://  fall thro
            case(4)://  fall thro
            case(5):
                setup_display2(3,LINEAR,0,0.0,0.0,ap);
                setup_display2(4,LINEAR,0,0.0,0.0,ap);
                break;
            }
        }
        switch(mode) {
        case(10):   setup_display2(2,LINEAR,0,0.0,0.0,ap);  break;
        case(11):   setup_display2(2,NUMERIC,0,0.0,0.0,ap); break;
        }
        break;
    case(EXTSPEAK):
        if(mode < 6)
            setup_display2(XSPK_WINSZ,NUMERIC,0,0.0,0.0,ap);
        setup_display2(XSPK_SPLEN,NUMERIC,0,0.0,0.0,ap);
        if(mode < 12) {
            setup_display2(XSPK_OFFST,NUMERIC,0,0.0,0.0,ap);
            setup_display2(XSPK_N,    LINEAR,0,0.0,0.0,ap);
        }
        setup_display2(XSPK_GAIN, LINEAR,0,0.0,0.0,ap);
        if(mode != 2 && mode != 5 && mode != 8 && mode != 11 && mode != 14 && mode != 17)
            setup_display2(XSPK_SEED,NUMERIC,0,0.0,0.0,ap);
        break;
    case(ENVSCULPT):
        if(mode != 2)
            setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        if(mode == 1) {
            setup_display2(4,NUMERIC,0,0.0,0.0,ap);
            setup_display2(5,NUMERIC,0,0.0,0.0,ap);
        }
        if(mode != 0)
            setup_display2(6,NUMERIC,0,0.0,0.0,ap);
        break;
    case(TREMENV):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        setup_display2(1,NUMERIC,0,0.0,0.0,ap);
        setup_display2(2,NUMERIC,0,0.0,0.0,ap);
        setup_display2(3,NUMERIC,0,0.0,0.0,ap);
        break;
    case(DCFIX):
        setup_display2(0,NUMERIC,0,0.0,0.0,ap);
        break;
    }
    return(FINISHED);
}

/****************************** SETUP_DISPLAY2 *********************************/

void setup_display2(int paramno,int dtype,int subrang,double lo,double hi,aplptr ap)
{
    ap->display_type[paramno]   = (char)dtype;
    ap->has_subrange[paramno]   = (char)subrang;
    ap->lolo[paramno]           = lo;
    ap->hihi[paramno]           = hi;
}

/************************************ SETUP_FLAGNAMES2 *****************************/

int setup_flagnames2(int process,int mode,int total_flags,aplptr ap)
{
    if((ap->flagname = (char **)malloc((total_flags) * sizeof(char *)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY: to setup_flagnames2\n"); 
        return(MEMORY_ERROR);
    }
    switch(process) {
    case(TAPDELAY):
        ap->flagname[0] = "FLOATING_POINT_OUTPUT";
        break;
    case(RMRESP):
        break;
    case(RMVERB):
        ap->flagname[0] = "DOUBLE_AIR-ABSORPTION";
        ap->flagname[1] = "FLOATING_POINT_OUTPUT_FILE";
        break;
    case(PSOW_FEATURES):
        ap->flagname[0] = "ALTERNATIVE_FOF_STRETCH";
        break;
    case(STRANS):
        switch(mode) {
        case(0):
        case(1):
            ap->flagname[0] = "OUTFILE_TIMES";
            break;
        case(2):
        case(3):
            break;
        }
        break;
    case(PSOW_REINF):
        if(mode == 0)
            ap->flagname[0] = "DON'T_DUPLICATE_HARMONICS";
        break;
    case(PARTIALS_HARM):
        ap->flagname[0] = "OUTPUT_IN_VARIPARTIALS_FILTER_FORMAT";
        break;
    case(SPECROSS):
        ap->flagname[0] = "RETAIN_FILE2_CONTOUR_UNDER_FILE1_CONTOUR";
        ap->flagname[1] = "EXTEND_1ST_STABLE_PITCH_OF_FILE1_TO_START";
        break;
    case(LUCIER_GET):
        ap->flagname[0] = "RESOLVE_LOW_FRQS";
        break;
    case(FOFEX_EX):
        ap->flagname[0] = "NO_FOF_WINDOWING";
        break;
    case(FOFEX_CO):
        switch(mode) {
        case(FOF_SUM):
        case(FOF_LOSUM):
        case(FOF_MIDSUM):
        case(FOF_HISUM):
        case(FOF_LOHI):
        case(FOF_TRIPLE):
            ap->flagname[0] = "EQUALISE_FOF_LEVELS";
            break;
        case(FOF_SINGLE):
        case(FOF_MEASURE):
            break;
        }
        break;
    case(TEX_MCHAN):
        ap->flagname[0] = "PLAY_ALL_OF_INSOUND";
        ap->flagname[1] = "PLAY_FILES_CYCLICALLY";
        ap->flagname[2] = "RANDOMLY_PERMUTE_EACH_CYCLE";
        ap->flagname[3] = "FIXED_POSITIONS";
        break;
    case(MCHANPAN):
        switch(mode) {
        case(2):
            ap->flagname[0] = "STEPPED";
            break;
        case(8):
            ap->flagname[0] = "ANTICLOCKWISE";
            break;
        case(9):
            ap->flagname[0] = "NO_STEPS_TO_ADJACENT_CHANNELS";
            ap->flagname[1] = "RANDOMLY_VARY_GROUP_SIZE";
            break;
        default:
            break;
        }
        break;
    case(FRAME):
        switch(mode) {
        case(4):
        case(7):
            ap->flagname[0] = "TO_RING_FORMAT_(DEFAULT:_TO_BILATERAL)";
            break;
        default:
            break;
        }
        break;
    case(WRAPPAGE):
        ap->flagname[0] = "EXPONENTIAL_SPLICES";
        ap->flagname[1] = "RELATIVE_TO_OUTPUT_TIME";
        break;
    case(MCHSTEREO):
        ap->flagname[0] = "CENTRE_BETWEEN_OUTCHANS";
        break;
    case(FLUTTER):
        ap->flagname[0] = "RANDOMISE_CHANNEL-SETS_ORDER";
        break;
    case(ABFPAN):
        ap->flagname[0] = "OUTPUT_AS_STANDARD_WAV_AMBISONIC";
        ap->flagname[1] = "OUTPUT_AS_STANDARD_WAV";
        break;
    case(ABFPAN2):
    case(ABFPAN2P):
        ap->flagname[0] = "STANDARD_WAV_(AMBISONIC)_OUTPUT";
        break;
    case(FMDCODE):
        ap->flagname[0] = "WRITE_WAVEX_LSPKR_POSITIONS_TO_HEADER";
        ap->flagname[1] = "PLAIN_WAV_OUTFILE_FORMAT";
        break;
        break;
    case(COPYSFX):
        ap->flagname[0] = "ADD_DITHER_TO_16_BIT_OUTFILE";
        ap->flagname[1] = "WRITE_MINIMAL_HEADER_(NO_PEAKDATA)";
        break;
    case(NJOIN):
        ap->flagname[0] = "CD_COMPATIBLE_FILES_ONLY";
        ap->flagname[1] = "OUTPUT_A_TEXTFILE_OF_CUES";
        break;
    case(NJOINCH):
        ap->flagname[0] = "CD_COMPATIBLE_FILES_ONLY";
        break;
    case(NMIX):
        ap->flagname[0] = "APPLY_DITHER_TO_16bit_OUTPUT";
        ap->flagname[1] = "FLOAT_OUTPUT";
        break;
    case(RMSINFO):
        ap->flagname[0] = "INCLUDE_NORMALISED_LEVELS";
        break;
    case(CHORDER):
        ap->flagname[0] = "FORCE_'.amb'_FILENAME_EXTENSION_(CARE!!)";
        break;
    case(SETHARES):
        ap->flagname[0] = "NO_AMPLITUDES";
        ap->flagname[1] = "MIDI_OUTPUT";
        ap->flagname[2] = "QUANTISE_TO_QUARTERTONES";
        ap->flagname[3] = "MARK_ZEROS";
        if(mode > 0)
            ap->flagname[4] = "FILTER_FORMAT";
        break;
    case(MCHZIG):
        ap->flagname[0] = "NO_PANS_BETWEEN_ADJACENT_CHANNELS";
        break;
    case(PSOW_EXTEND):
        ap->flagname[0] = "SMOOTH_FOFS";
        break;
    case(SUPERACCU):
        ap->flagname[0] = "REASSIGN_CHANNELS_(GLISSING_PITCHES)";
        break;
    case(ISOLATE):
        ap->flagname[0] = "EXTEND_END-SILENCES_TO_ENDTIME_OF_SOURCE";
        ap->flagname[1] = "REVERSE_SEGMENT_OUTPUT";
        break;
    case(REJOIN):
        ap->flagname[0] = "REVERSE_SEGMENTS";
        break;
    case(PANORAMA):
        ap->flagname[0] = "FRONT-PAIR_LSPKRS";
        ap->flagname[1] = "FRONT-PAIR_SOUND_IMAGES";
        break;
    case(PACKET):
        ap->flagname[0] = "NORMALISE";
        ap->flagname[1] = "EXPAND";
        ap->flagname[2] = "SHAVE_SILENCE";
        break;
    case(SYNTHESIZER):
        if(mode==1)
            ap->flagname[0] = "STATIONARY_PACKET_PARAMS";
        else if(mode==2) {
            ap->flagname[0] = "FROM_FUNDAMENTAL";
            ap->flagname[1] = "TO_FUNDAMENTAL";
            ap->flagname[2] = "SPATIALISE";
            ap->flagname[3] = "MAXCHANGE_PARTIALS";
            ap->flagname[4] = "JUMP";
        } else if(mode==3)
            ap->flagname[0] = "SECOND_ON_SEG_AT_END_OF_ONOFFONOFF-GROUP";
        break;
    case(NEWTEX):
        if(mode==0)
            ap->flagname[0] = "MAXCHANGE_PARTIALS";
        else 
            ap->flagname[0] = "MAXCHANGE_COMPONENTS";
        ap->flagname[1] = "JUMP";
        break;
    case(CERACU):
        ap->flagname[0] = "OVERRIDE_LENGTH_LIMIT";
        ap->flagname[1] = "LINEAR_OUTPUT_ARRAY";
        break;
    case(MADRID):
        ap->flagname[MAD_GAPS]   = "ALLOW_GAPS_IN_OUTPUT";
        ap->flagname[MAD_LINEAR] = "LINEAR_OUTPUT_ARRAY";
        if(mode==0) {
            ap->flagname[MAD_INPERM] = "RANDOMLY_PERMUTE_INFILE_ORDER";
            ap->flagname[MAD_INRAND] = "RANDOMLY_SELECT_INFILE";
        }
        break;
    case(SHIFTER):
        ap->flagname[SHF_ZIG] = "READ_BACK_AND_FORTH_THROUGH_FOCUS";
        ap->flagname[SHF_RND] = "RANDOMLY_PERMUTE_FOCUS";
        ap->flagname[SHF_LIN] = "LINEAR_OUTPUT_ARRAY";
        break;
    case(FRACTURE):
        ap->flagname[0] = "PERMIT_SHORT_STACKS";
        if(mode==0)
            ap->flagname[1] = "LINEAR_OUTPUT_ARRAY";
        break;
    case(TAN_ONE):
    case(TAN_TWO):
    case(TAN_SEQ):
    case(TAN_LIST):
        ap->flagname[0] = "RECEDING";
        ap->flagname[1] = "AT_LEFT";
        break;
    case(TRANSIT):
    case(TRANSITF):
    case(TRANSITD):
    case(TRANSITFD):
    case(TRANSITS):
    case(TRANSITL):
        ap->flagname[0] = "LEFTWARDS_FROM_FOCUS";
        break;
    case(CANTOR):
        if(mode !=2)
            ap->flagname[0] = "EXTEND_TO_LIMIT";
        break;
    case(SHRINK):
        ap->flagname[0] = "EQUALISE_EVENT_LEVELS";
        ap->flagname[1] = "REVERSE_SEGMENTS";
        if(mode >= SHRM_FINDMX) {
            ap->flagname[2] = "SQUEEZE_EVENLY";
            ap->flagname[3] = "OUTPUT_GATE";
        }
        break;
    case(SPECMORPH):
        if(mode < 4 || mode == 6) {
            ap->flagname[0] = "RETAIN_LOUDNESS_ENVELOPE_OF_1st_SND";
            ap->flagname[1] = "INTERP_PEAKS_ONLY";
            ap->flagname[2] = "INTERP_PEAK_FREQUENCIES_ONLY";
        }
        break;
    case(ITERLINE):
    case(ITERLINEF):
        ap->flagname[0] = "NORMALISE_OUTPUT";
        break;
    case(HOVER2):
        ap->flagname[0] = "ADVANCE_LOCATIONS_STEPWISE";
        ap->flagname[1] = "NORMALISE_OUTPUT";
        break;
    case(PULSER2):
        ap->flagname[0] = "TOTALLY_RANDOM_SRC_SEQUENCE";
        break;
    case(SYNFILT):
        ap->flagname[0] = "DOUBLE_FILTERING";
        ap->flagname[1] = "DROP_OUT_ON_OVERFLOW";
        break;
    case(STRANDS):
        if(mode ==1)
            ap->flagname[0] = "OUTPUT_BANDS_SEQUENTIALLY";
        break;
    case(SPECULATE):
        ap->flagname[0] = "MASK_OTHER_CHANNELS";
        break;
    case(SPECTUNE):
        ap->flagname[0] = "IGNORE_PITCHED-WINDOW_RELATIVE-LOUDNESS";
        ap->flagname[1] = "SMOOTH_PITCH-CONTOUR";
        if(mode != 3)
            ap->flagname[2] = "FORMANT_ENVELOPE_NOT_PRESERVED";
        break;
    case(ROTOR):
        ap->flagname[0] = "STEREO_OUTPUT";
        break;
    case(SPECFOLD):
        ap->flagname[0] = "AMPLITUDES_ONLY";
        break;
    case(BROWNIAN):
        ap->flagname[0] = "LINEAR_LSPKR_ARRAY";
        break;
    case(PHASOR):
        ap->flagname[0] = "SOUND_SURROUND";
        ap->flagname[1] = "WARN_OF_ROUNDING_ERRORS";
        break;
    case(CASCADE):
        ap->flagname[0] = "ALSO_SHRED_SOURCE";
        ap->flagname[1] = "LINEAR_DECAY_ECHO_LEVELS";
        ap->flagname[2] = "NORMALISE_LOW_LEVEL_OUTPUT";
        break;
    case(SYNSPLINE):
        ap->flagname[0] = "NORMALISE_ALL_WAVECYCLES";
        break;
    case(SPLINTER):
        ap->flagname[0] = "RETAIN_ALL_SOURCE_SND";
        ap->flagname[1] = "RETAIN_NO_SOURCE_SND";
        break;
    case(VERGES):
        ap->flagname[0] = "DON'T_SEARCH_FOR_LOCAL_PEAKS";
        ap->flagname[1] = "BOOST_VERGE_LEVEL";
        ap->flagname[2] = "SUPPRESS_NON-VERGES";
        break;
    case(MOTOR):
        ap->flagname[MOT_FXDSTP] = "ADVANCE_BY_FIXED_STEP_IN_SRC_READS";
        if(mode % 3 == 2)
            ap->flagname[MOT_CYCLIC] = "USE_SRCS_CYCLICALLY";
        else
            ap->flagname[MOT_CYCLIC] = "USE_SEGMENTS_CYCLICALLY";
        break;
    case(STUTTER):
        ap->flagname[STUT_PERM] = "PERMUTE_SEGMENT_ORDER";
        break;
    case(TWEET):
        ap->flagname[0] = "COSIN_SMOOTH_FOFS";
        break;
    case(RRRR_EXTEND):  // version 8+
        switch(mode) {
        case(0):
        case(1):
            ap->flagname[0] = "DON'T_KEEP_SOUND_BEFORE_ITERATE";
            ap->flagname[1] = "DON'T_KEEP_SOUND_AFTER_ITERATE";
            break;
        }
        break;
    case(SORTER):
        ap->flagname[0] = "AS_FREQUENCY";
        break;
    case(SPECFNU):
        switch(mode) {
        case(0):
            ap->flagname[0] = "ZERO_TOP_OF_SPECTRUM";
            ap->flagname[1] = "FORCE_FUNDAMENTAL";
            ap->flagname[2] = "USE_SHORT_WINDOW";
            ap->flagname[3] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[4] = "EXCLUDE_HARMONIC_PARTIALS";
            ap->flagname[5] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            break;
        case(1):
            ap->flagname[0] = "SQUEEZEAROUND_TROUGH_ABOVE";
            ap->flagname[1] = "FORCE_FUNDAMENTAL";
            ap->flagname[2] = "USE_SHORT_WINDOW";
            ap->flagname[3] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[4] = "EXCLUDE_HARMONIC_PARTIALS";
            ap->flagname[5] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            break;
        case(2):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "EXCLUDE_HARMONIC_PARTIALS";
            ap->flagname[3] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            break;
        case(3):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "EXCLUDE_HARMONIC_PARTIALS";
            ap->flagname[3] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            break;
        case(4):
            ap->flagname[0] = "FLAT";
            break;
        case(5):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            break;
        case(6):
            ap->flagname[0] = "KEEP_PEAK_LOUDNESS";
            ap->flagname[1] = "INVERT_PEAK_LOUDNESS";
            ap->flagname[2] = "FORCE_FUNDAMENTAL";
            ap->flagname[3] = "USE_SHORT_WINDOW";
            break;
        case(7):
            ap->flagname[0] = "ZERO_TOP_OF_SPECTRUM";
            ap->flagname[1] = "USE_SHORT_WINDOW";
            ap->flagname[2] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[3] = "EXCLUDE_HARMONIC_PARTIALS";
            ap->flagname[4] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            break;
        case(8):
            ap->flagname[0] = "ZERO_TOP_OF_SPECTRUM";
            ap->flagname[1] = "USE_SHORT_WINDOW";
            ap->flagname[2] = "NARROW_FORMANT_BANDS";
            ap->flagname[3] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[4] = "EXCLUDE_HARMONIC_PARTIALS";
            ap->flagname[5] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            break;
        case(9):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "DOWNWARDS";
            ap->flagname[4] = "UP_AND_DOWN";
            break;
        case(10):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "DOWNWARDS";
            ap->flagname[4] = "UP_AND_DOWN";
            ap->flagname[5] = "FILL_TOP_OF_SPECTRUM";
            break;
        case(11):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "DOWNWARDS";
            ap->flagname[4] = "UP_AND_DOWN";
            ap->flagname[5] = "FILL_TOP_OF_SPECTRUM";
            break;
        case(12):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "DOWNWARDS";
            ap->flagname[4] = "UP_AND_DOWN";
            ap->flagname[5] = "FILL_TOP_OF_SPECTRUM";
            break;
        case(13):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "DOWNWARDS";
            ap->flagname[4] = "UP_AND_DOWN";
            ap->flagname[5] = "FILL_TOP_OF_SPECTRUM";
            break;
        case(14):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "DOWNWARDS";
            ap->flagname[4] = "UP_AND_DOWN";
            break;
        case(15):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "DOWNWARDS";
            ap->flagname[4] = "UP_AND_DOWN";
            ap->flagname[5] = "TIE_TO_TOP";
            ap->flagname[6] = "TIE_TO_FOOT";
            ap->flagname[7] = "TIE_TO_MIDDLE";
            ap->flagname[8] = "ABOVE_MEAN_ONLY";
            ap->flagname[9] = "BELOW_MEAN_ONLY";
            break;
        case(16):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "DOWNWARDS";
            ap->flagname[4] = "UP_AND_DOWN";
            ap->flagname[5] = "ALLOW ORNAMENTS";
            ap->flagname[6] = "NO_SMOOTHING";
            break;
        case(17):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "DOWNWARDS";
            ap->flagname[4] = "UP_AND_DOWN";
            ap->flagname[5] = "FAST_PITCH-CHANGE";
            ap->flagname[6] = "NO_SMOOTHING";
            ap->flagname[7] = "NO_FORMANT_RESHAPING";
            break;
        case(18):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "EXCLUDE_NON-HARMONIC_PARTIALS";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "DOWNWARDS";
            ap->flagname[4] = "UP_AND_DOWN";
            break;
        case(19):   //  fall thro
        case(20):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            break;
        case(21):
            ap->flagname[0] = "GET_PEAKS";
            ap->flagname[1] = "GET_TROUGHS_AND_PEAKS";
            break;
        case(22):
            ap->flagname[0] = "USE_SHORT_WINDOW";
            ap->flagname[1] = "FORCE_FUNDAMENTAL";
            ap->flagname[2] = "NONPITCH_&_LOLEVEL_WINDOWS_ZEROED";
            ap->flagname[3] = "SMOOTHING";
            break;
        }
        break;
    case(BOUNCE):
        ap->flagname[0] = "TRIM_TO_SIZE";
        ap->flagname[1] = "CUT_FROM_START";
        break;
    case(DISTMARK):
        ap->flagname[0] = "RETROGRADE_AND_PHASE_INVERT_ALTERNATE_WAVESETS";
        if(mode == 0)
            ap->flagname[1] = "RETAIN_ORIGINAL_TAIL";
        else
            ap->flagname[1] = "SWITCH_RETAINED_AND_DISTORTED_ELEMENTS";
        break;
    case(SPECSPHINX):
        switch(mode) {
        case(0): // fall thro
        case(1):    break;
        case(2):    
            ap->flagname[0] = "USE_LOUDNESS_ENVELOPE_OF_2nd_SPECTRUM";
            break;
        }
        break;
    case(SPECENV):
        ap->flagname[0] = "READ_WINDOWSIZE_AS_OCTAVES_(RATHER_THAN_ANAL_CHANS_CNT)";
        ap->flagname[1] = "IMPOSE_(RATHER_THAN_REPLACE)_SPECTRAL_ENVELOPE";
        ap->flagname[2] = "LOUDNESS_CONTOUR_FOLLOWS_FILE_2_(RATHER_THAN_FILE_1)";
        break;
    case(SPECEX):
        ap->flagname[0] = "INCLUDE_SOUND_BEFORE_STRETCHED_PORTION";
        ap->flagname[1] = "INCLUDE_SOUND_AFTER_STRETCHED_PORTION";
        break;
    case(MATRIX):
        switch(mode) {
        case(MATRIX_MAKE):  // fall thro
        case(MATRIX_USE):
            ap->flagname[0] = "APPLY_MATRIX_CYCLICALLY";
            break;
        default:
            break;
        }
        break;
    case(SPECCONV):
        if(mode == 1)
            ap->flagname[0] = "DON'T_TIMESTRETCH_FILE_2";
        break;
    case(SPECFRAC):
        ap->flagname[0] = "OUTPUT_ALL_INTERMEDIATE_FRACTALS";
        break;
    case(ENVSPEAK):
        if(mode == 6 || mode == 18)
            ap->flagname[0] = "REPEATED_ELEMENTS_DO_NOT_GROW_IN_SIZE";
        break;
    case(EXTSPEAK):
        switch(mode) {
        case(6):
        case(0):
            ap->flagname[XSPK_RAND]      = "SELECT_INSERTS_AT_RANDOM";
            //  fall thro
        case(1):    //  fall thro
        case(2):    //  fall thro
        case(7):    //  fall thro
        case(8):
            ap->flagname[XSPK_INJECT]    = "INJECT_BETWEEN_(RATHER_THAN_OVERWRITE)";
            ap->flagname[XSPK_ORIGSZ]    = "RETAIN_ORIGINAL_DURATION_OF_INSERTS";
            //  fall thro
        case(3):
        case(4):
        case(5):
        case(9):
        case(10):
        case(11):
            ap->flagname[XSPK_TRANSPOSE] = "TRANSPOSE_(RATHER_THAN_CUT_TO_SIZE)_INSERTS";
            ap->flagname[XSPK_ENV]       = "FOLLOW_ENVELOPE_OF_OVERWRITTEN_SYLLABLES";
            ap->flagname[XSPK_KEEP]      = "N_=_RETAIN_N_ORIG_SYLLABS_FOR_EVERY_1_OVERWRITTEN";
            break;
        case(12):
        case(13):
        case(14):
            ap->flagname[2]          = "RETAIN_ORIGINAL_DURATION_OF_INSERTS";
            //  fall thro
        case(15):
        case(16):
        case(17):
            ap->flagname[XSPK_TRANSPOSE] = "TRANSPOSE_(RATHER_THAN_CUT_TO_SIZE)_INSERTS";
            ap->flagname[XSPK_ENV]       = "FOLLOW_ENVELOPE_OF_OVERWRITTEN_SYLLABLES";
            break;
        }
        if(mode == 3 || mode == 9)
            ap->flagname[3] = "SELECT_INSERTS_AT_RANDOM";
        else if(mode == 12)
            ap->flagname[3] = "SELECT_INSERTS_AT_RANDOM";
        else if(mode == 15)
            ap->flagname[2] = "SELECT_INSERTS_AT_RANDOM";
        break;
    case(FRACTAL):
        ap->flagname[0]     = "SHRINK_PITCH_INTERVALS_AS_TIME_SCALES_SHRINK";
        if(mode == 0)
            ap->flagname[1] = "READ_BRKPNT_DATA_USING_TIME_IN_OUTFILE";
        break;
    case(FRACSPEC):
        ap->flagname[0] = "SHRINK_FRACTAL_INTERVALS_AS_TIMESCALE_SHRINKS";
        ap->flagname[1] = "FORMANTS_NOT_RETAINED";
        break;
    case(ENVSCULPT):
        if(mode == 2)
            ap->flagname[0] = "ATTACK_ON_2nd_PEAK_ONLY";
        break;
    case(PSOW_EXTEND2):
    case(MCHITER):
    case(MCHSHRED):
    case(SFEXPROPS):
    case(INTERLX):
    case(CHXFORMATM):
    case(CHXFORMAT):
    case(CHXFORMATG):
    case(CHANNELX):
    case(MIXMULTI):
    case(ANALJOIN):
    case(PTOBRK):
    case(PSOW_STRETCH):
    case(PSOW_DUPL):
    case(PSOW_DEL):
    case(PSOW_STRFILL):
    case(PSOW_FREEZE):
    case(PSOW_CHOP):
    case(PSOW_INTERP):
    case(PSOW_SYNTH):
    case(PSOW_IMPOSE):
    case(PSOW_SPLIT):
    case(PSOW_SPACE):
    case(PSOW_INTERLEAVE):
    case(PSOW_REPLACE):
    case(PSOW_LOCATE):
    case(PSOW_CUT):
    case(ONEFORM_GET):
    case(ONEFORM_PUT):
    case(ONEFORM_COMBINE):
    case(NEWGATE):
    case(SPEC_REMOVE):
    case(PREFIXSIL):
    case(LUCIER_GETF):
    case(LUCIER_PUT):
    case(LUCIER_DEL):
    case(SPECLEAN):
    case(SPECSLICE):
    case(GREV_EXTEND):
    case(PEAKFIND):
    case(CONSTRICT):
    case(EXPDECAY):
    case(PEAKCHOP):
    case(MANYSIL):
    case(RETIME):
    case(HOVER):
    case(MULTIMIX):
    case(SEARCH):
    case(MCHANREV):
    case(MTON):
    case(PARTITION):
    case(SPECGRIDS):
    case(GLISTEN):
    case(TUNEVARY):
    case(TREMOLO):
    case(ECHO):
    case(NEWDELAY):
    case(FILTRAGE):
    case(SELFSIM):
    case(ITERFOF):
    case(FLATTEN):
    case(CLIP):
    case(TRANSPART):
    case(SPECINVNU):
    case(SPECSND):
    case(TREMENV):
    case(DCFIX):        /*RWD March 2021 was ACFIX */
        break;
    default:
        sprintf(errstr,"Unknown case: setup_flagnames2()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}
