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
/*
 * UPDATE MAR 23 : 2008
 *
 */

/* EXPECTS TO RECEIVE FROM TK 
 * --------------------------
 * 1)  process_no
 * 2)  mode_no
 * 3)  filetype             %d
 * 4)  infilesize           %ld
 * 5)  insams               %ld
 * 6)  srate                %ld
 * 7)  channels             %d
 * 8)  wanted               %ld
 * 9)  wlength              %ld
 * 10) linecnt              %d
 * 11) arate                %f
 * 12) frametime            %f
 * 13) nyquist              %lf
 * 14) duration             %lf
 *
 *
 * DELIVERS TO TK
 * --------------
 *  DURATION FLAG               (can duration for creation of brkpntfiles be used AS IS)
 *  N PARAMETER SPECIFICATIONS  (various)
 *
 *  All on separate lines!!
 */

#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <logic.h>
#include <formants.h>
#include <cdparams.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>
#include <filetype.h>
#include <string.h>    /* RWD */

#define PROCESS_NOT_SET (-100)

static int  establish_application(aplptr *ap);
static void initialise_application_vals(aplptr ap);
static int  set_legal_application_structure(aplptr ap, int process,int mode);
static int  print_dialog_box_info(int process,int mode,int total_params,int bare_flags,aplptr ap);
static int  print_info_on_formants(aplptr ap);
static int  names_check(int bare_flags,aplptr ap);
static int  print_special_data_info(aplptr ap);
static int  print_param_info(int total_params,aplptr ap);
static int  print_flag_info(int bare_flags,aplptr ap);
static int  init_param_default_array(int total_params,aplptr ap);

static void superfree_application(aplptr ap);

static int  can_specify_brkpntfile_len(int process,int mode);
static int  do_the_parameter_display(int display_type,char *pname,char *pname2,char *pname3,char ptype,
            double ranglo,double ranghi,double dflt,double r2lo,double r2hi,double dflt2);
static int  get_subrange(double *lolo, double *hihi,int n,aplptr ap);

char errstr[1000];
char paramstr[6000];

int sloom = 1;
int sloombatch = 0;
const char* cdp_version = "8.0.0";

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

    if((exit_status = establish_application(&ap))<0)
        return(exit_status);
    ap->accepts_conflicting_srates = does_process_accept_conflicting_srates(process);
/* KLUDGE */
    fprintf(stderr,"\n");
/* KLUDGE */
    if((exit_status = set_legal_application_structure(ap,process,mode))<0) {
        superfree_application(ap);
        fprintf(stdout,"ERROR: %s\n",errstr);
        fflush(stdout);
        return(FAILED);
    }

    total_params  = ap->max_param_cnt + ap->option_cnt + ap->variant_param_cnt;
    user_paramcnt = ap->param_cnt + ap->option_cnt + ap->variant_param_cnt;
    bare_flags    = ap->vflag_cnt - ap->variant_param_cnt;

    if((exit_status = deal_with_special_data(process,mode,srate,duration,nyquist,wlength,channels,ap))<0) {
        superfree_application(ap);
        fprintf(stdout,"ERROR: %s\n",errstr);
        fflush(stdout);
        return(exit_status);
    }
    if((exit_status = deal_with_formants(process,mode,channels,ap))<0) {
        superfree_application(ap);
        fprintf(stdout,"ERROR: %s\n",errstr);
        fflush(stdout);
        return(exit_status);
    }
    if(total_params > 0) {
        if((exit_status = get_param_names(process,mode,total_params,ap))<0) {
            superfree_application(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
        if((exit_status = get_param_ranges(process,mode,total_params,nyquist,frametime,arate,srate,wlength,
                                        insams,channels,wanted,filetype,linecnt,duration,ap))<0) {
            superfree_application(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
        if((exit_status = init_param_default_array(total_params,ap))<0) {
            superfree_application(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
        if((exit_status = initialise_param_values(process,mode,channels,nyquist,frametime,insams,srate,
                                                wanted,linecnt,duration,ap->default_val,filetype,ap))<0) {
            superfree_application(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
        if((exit_status = establish_display(process,mode,total_params,frametime,duration,ap))<0) {
            superfree_application(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }

    }
    if(bare_flags > 0) {
        if((exit_status = setup_flagnames(process,mode,bare_flags,ap))<0) {
            superfree_application(ap);
            fprintf(stdout,"ERROR: %s\n",errstr);
            fflush(stdout);
            return(FAILED);
        }
    }
    if((exit_status = print_dialog_box_info(process,mode,user_paramcnt,bare_flags,ap))<0) {
        superfree_application(ap);
        fprintf(stdout,"ERROR: %s",errstr);
        fflush(stdout);
        return(FAILED);
    }
    fflush(stdout);
    return(SUCCEEDED);
}

/******************************* ESTABLISH_APPLICATION *******************************/

int establish_application(aplptr *ap)
{
    if((*ap = (aplptr)malloc(sizeof(struct applic)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to establish_application\n");
        return(MEMORY_ERROR);
    }
    initialise_application_vals(*ap);
    return(FINISHED);
}

/************************* INITIALISE_APPLICATION_VALS *************************/

void initialise_application_vals(aplptr ap)
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
    ap->special_data_name     = (char*)0;
    ap->special_data_name2    = (char*)0;
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

/************************** SET_LEGAL_APPLICATION_STRUCTURE *****************************/

int set_legal_application_structure(aplptr ap,int process,int mode)
{
    int exit_status;
    if((exit_status = set_legal_param_structure(process,mode,ap))<0)
        return(exit_status);

    if((exit_status = set_legal_option_and_variant_structure(process,mode,ap))<0)
        return(exit_status);
    return(FINISHED);
}

/******************************* SUPERFREE_APPLICATION *******************************/

void superfree_application(aplptr ap)
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

/******************************* PRINT_DIALOG_BOX_INFO (Feb 1999) *******************************/

int print_dialog_box_info(int process,int mode,int user_paramcnt,int bare_flags,aplptr ap)
{
    int exit_status;

    if((exit_status = names_check(bare_flags,ap))<0)
        return(exit_status);

    if((exit_status = can_specify_brkpntfile_len(process,mode))<0)
        return(exit_status);
    fprintf(stdout,"%d\n",exit_status);  /* TELLS WHETHER BRKPNTFILE DATA CAN HAVE SPECIFIC LENGTH */

    if((exit_status = print_info_on_formants(ap))<0)
        return(exit_status);

    if((exit_status = print_special_data_info(ap))<0)
        return(exit_status);

    if((exit_status = print_param_info(user_paramcnt,ap))<0)
        return(exit_status);

    if((exit_status = print_flag_info(bare_flags,ap))<0)
        return(exit_status);
    fprintf(stdout,"ENDPARAMS\n");
    return(FINISHED);
}

/******************************* NAMES_CHECK *******************************/

int names_check(int bare_flags,aplptr ap)
{
    int array_checked = FALSE;
    int n, m;
    int checkcnt      = 0;

    for(n=0;n<ap->max_param_cnt;n++) {
        if(ap->param_list[n]!='0') {
            if(!array_checked) {
                if(ap->param_name==NULL) {
                    sprintf(errstr,"Parameter names array not initialised: names_check()\n");
                    return(PROGRAM_ERROR);
                } else
                    array_checked = TRUE;
            }
            if(ap->param_name[n]==NULL) {
                sprintf(errstr,"Parameter name %d not initialised: names_check()\n",checkcnt+1);
                return(PROGRAM_ERROR);
            }
            checkcnt++;
        }
    }
    for(n=ap->max_param_cnt,m=0;m<ap->option_cnt;n++,m++) {
        if(!array_checked) {
            if(ap->param_name==NULL) {
                sprintf(errstr,"Option-Parameter names array not initialised: names_check()\n");
                return(PROGRAM_ERROR);
            } else
                array_checked = TRUE;
        }
        if(ap->param_name[n]==NULL) {
            sprintf(errstr,"Option-Parameter names %d not initialised: names_check()\n",ap->param_cnt+m+1);
            return(PROGRAM_ERROR);
        }
    }
    for(n=ap->max_param_cnt+ap->option_cnt,m=0;m<ap->variant_param_cnt;n++,m++) {
        if(!array_checked) {
            if(ap->param_name==NULL) {
                sprintf(errstr,"Variant-Parameter names array not initialised: names_check()\n");
                return(PROGRAM_ERROR);
            } else
                array_checked = TRUE;
        }
        if(ap->param_name[n]==NULL) {
            sprintf(errstr,"Variant-Parameter names %d not initialised: names_check()\n",ap->param_cnt+ap->option_cnt+m+1);
            return(PROGRAM_ERROR);
        }
    }
    array_checked = FALSE;
    for(n=ap->max_param_cnt+ap->option_cnt+ap->variant_param_cnt,m=0;m<bare_flags;n++,m++) {
        if(!array_checked) {
            if(ap->flagname==NULL) {
                sprintf(errstr,"Flag names array not initialised: names_check()\n");
                return(PROGRAM_ERROR);
            } else
                array_checked = TRUE;
        }
        if(ap->flagname[m]==NULL) {
            sprintf(errstr,"Flag name %d not initialised: names_check()\n",m+1);
            return(PROGRAM_ERROR);
        }
    }
    return(FINISHED);
}

/**************************** PRINT_INFO_ON_FORMANTS *************************/

int print_info_on_formants(aplptr ap)
{
    int exit_status;
    if(ap->formant_flag) {

        if(ap->no_pichwise_formants) {
            if((exit_status = do_the_parameter_display
            (LINEAR,"FRQWISE_FMNT_BANDS","","",'i',ap->min_fbands,ap->max_freqwise_fbands,FBAND_DEFAULT,
                                     ap->min_fbands,ap->max_freqwise_fbands,0.0))<0)
                return(exit_status);
        } else {
            if((exit_status = do_the_parameter_display
            (SWITCHED,"FORMANT_BANDS","PITCHWISE","FRQWISE",'i',ap->min_fbands,ap->max_pichwise_fbands,FBAND_DEFAULT,
                                     ap->min_fbands,ap->max_freqwise_fbands,FBAND_DEFAULT))<0)
                return(exit_status);
        }
    }

    if(ap->formant_qksrch) {
        if((exit_status = do_the_parameter_display(CHECKBUTTON,"FORMANT_QUICKSEARCH","","",'i',0.0,0.0,0.0,0.0,0.0,0.0))<0)
            return(exit_status);
    }
    return(FINISHED);
}

/******************************* PRINT_SPECIAL_DATA_INFO *******************************/

int print_special_data_info(aplptr ap)
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
                if((exit_status = do_the_parameter_display(FILENAME,ap->special_data_name,"","",(char)0,
                0.0,0.0,0.0,0.0,0.0,0.0))<0)
                    return(exit_status);
                break;
            case(1):
                if((exit_status = do_the_parameter_display(FILENAME,ap->special_data_name,"","",(char)1,
                ap->min_special,ap->max_special,0.0,0.0,0.0,0.0))<0)
                    return(exit_status);
                break;
            case(2):
                if((exit_status = do_the_parameter_display(FILENAME,ap->special_data_name,"","",(char)2,
                ap->min_special,ap->max_special,0.0,ap->min_special2,ap->max_special2,0.0))<0)
                    return(exit_status);
                break;
            }
            break;
        case(FALSE):
            if((exit_status = do_the_parameter_display(FILE_OR_VAL,ap->special_data_name,"","",'D',
            ap->min_special,ap->max_special,ap->default_special,0.0,0.0,0.0))<0)
                return(exit_status);
            break;
        case(VOWEL_STRING):
            if((exit_status = do_the_parameter_display(FILE_OR_VOWELS,ap->special_data_name,"","",(char)0,
            0.0,0.0,0.0,0.0,0.0,0.0))<0)
                return(exit_status);
            break;
        case(SNDFILENAME_STRING):   /* this is a generic outfilename, not an existing file */
            if((exit_status = do_the_parameter_display(GENERIC_FILENAME,ap->special_data_name,"","",(char)0,
            0.0,0.0,0.0,0.0,0.0,0.0))<0)
                return(exit_status);
            break;
        case(SHUFFLE_STRING):
            if((exit_status = do_the_parameter_display(STRING_A,ap->special_data_name,"","",(char)0,
            0.0,0.0,0.0,0.0,0.0,0.0))<0)
                return(exit_status);
            break;
        case(REORDER_STRING):
            if((exit_status = do_the_parameter_display(STRING_B,ap->special_data_name,"","",(char)0,
            0.0,0.0,0.0,0.0,0.0,0.0))<0)
                return(exit_status);
            break;
        case(NOTE_STRING):             
            if((exit_status = do_the_parameter_display(STRING_C,ap->special_data_name,"","",(char)0,
            0.0,0.0,0.0,0.0,0.0,0.0))<0)
                return(exit_status);
            break;
        case(INTERVAL_STRING):         
            if((exit_status = do_the_parameter_display(STRING_D,ap->special_data_name,"","",(char)0,
            0.0,0.0,0.0,0.0,0.0,0.0))<0)
                return(exit_status);
            break;
        }
    }
    return(FINISHED);
}

/******************************* PRINT_PARAM_INFO *******************************/

int print_param_info(int user_paramcnt,aplptr ap)
{
    int exit_status;
    int checkcnt = 0, n, m;
    double lolo, hihi;
    if(user_paramcnt<=0)
        return(FINISHED);
    if(ap->max_param_cnt > 0) {
        for(n=0;n<ap->max_param_cnt;n++) {
            if(ap->param_list[n]!='0') {

                get_subrange(&lolo,&hihi,n,ap);
                if((exit_status = do_the_parameter_display(ap->display_type[n],ap->param_name[n],"","",
                ap->param_list[n],ap->lo[n],ap->hi[n],ap->default_val[n],lolo,hihi,0.0))<0)
                    return(exit_status);
                checkcnt++;
            }
        }
        if(checkcnt!=ap->param_cnt) {
            sprintf(errstr,"parameter accounting problem: print_param_info()\n");
            return(PROGRAM_ERROR);
        }
    }
    if(ap->option_cnt > 0) {
        for(n=ap->max_param_cnt,m=0;m<ap->option_cnt;n++,m++) {
//TW JULY 2006
            if(ap->option_list[m]!='0') {
                get_subrange(&lolo,&hihi,n,ap);
                if((exit_status = do_the_parameter_display(ap->display_type[n],ap->param_name[n],"","",
                ap->option_list[m],ap->lo[n],ap->hi[n],ap->default_val[n],lolo,hihi,0.0))<0)
                    return(exit_status);
            }
        }
    }
    if(ap->variant_param_cnt > 0) {
        for(n=ap->max_param_cnt+ap->option_cnt,m=0;m<ap->variant_param_cnt;n++,m++) {
            get_subrange(&lolo,&hihi,n,ap);
            if((exit_status = do_the_parameter_display(ap->display_type[n],ap->param_name[n],"","",
            ap->variant_list[m],ap->lo[n],ap->hi[n],ap->default_val[n],lolo,hihi,0.0))<0)
                return(exit_status);
        }
    }
    return(FINISHED);
}

/******************************* PRINT_FLAG_INFO *******************************/

int print_flag_info(int bare_flags,aplptr ap)
{
    int n, m;

    if(bare_flags>0) {
        for(n=ap->max_param_cnt+ap->option_cnt+ap->variant_param_cnt,m=0;m<bare_flags;n++,m++) {
            do_the_parameter_display(CHECKBUTTON,ap->flagname[m],"","",0,0,0,0,0,0,0);
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

/******************************* INIT_PARAM_DEFAULT_ARRAY *******************************/

int init_param_default_array(int total_params,aplptr ap)
{
    if((ap->default_val = (double *)malloc((total_params) * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for param_default_array\n");
        return(MEMORY_ERROR);
    }
    return(FINISHED);
}

/****************************** DO_THE_PARAMETER_DISPLAY *********************************/

int do_the_parameter_display
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
    case(FILE_OR_VAL):  
        fprintf(stdout,"FILE_OR_VAL %s %c %lf %lf %lf\n",
            quoted_pname,   /* param name           */
            ptype,          /* data type            */
            ranglo,         /* bottom of range      */
            ranghi,         /* top of range         */
            dflt);          /* default val          */
        break;
    case(FILE_OR_VOWELS):   fprintf(stdout,"VOWELS %s\n",quoted_pname);         break;
    case(CHECKBUTTON):      fprintf(stdout,"CHECKBUTTON %s\n",quoted_pname);    break;
    case(GENERIC_FILENAME): fprintf(stdout,"GENERICNAME %s\n",quoted_pname);    break;
    case(STRING_A):         fprintf(stdout,"STRING_A %s\n",quoted_pname);       break;
    case(STRING_B):         fprintf(stdout,"STRING_B %s\n",quoted_pname);       break;
    case(STRING_C):         fprintf(stdout,"STRING_C %s\n",quoted_pname);       break;
    case(STRING_D):         fprintf(stdout,"STRING_D %s\n",quoted_pname);       break;

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
    case(TIMETYPE):
        fprintf(stdout,"TIMETYPE %s %lf\n",
            quoted_pname,   /* param name       */
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
    case(CHORDSORT):
        fprintf(stdout,"CHORD_GADGET %s %lf\n",
            quoted_pname,   /* param name       */
            dflt);          /* default value    */
        break;
    case(TWOFAC):
        fprintf(stdout,"TWOFAC %s %lf\n",
            quoted_pname,   /* param name       */
            dflt);          /* default value    */
        break;
    case(WAVETYPE):
        fprintf(stdout,"WAVETYPE %s %lf\n",
            quoted_pname,   /* param name       */
            dflt);          /* default value    */
        break;
    default:
        sprintf(errstr,"Unknown parameter type: do_the_parameter_display()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
} 

/******************************* GET_SUBRANGE *******************************/

int get_subrange(double *lolo, double *hihi,int n,aplptr ap)
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

/****************************** CAN_SPECIFY_BRKPNTFILE_LEN *********************************/

int can_specify_brkpntfile_len(int process,int mode)
{                        
    switch(process) {
    case(GAIN):     case(LIMIT):    case(BARE):     case(CLEAN):    case(STRETCH):  case(ALT):
    case(OCT):      case(SHIFTP):   case(TUNE):     case(PICK):     case(MULTRANS): case(CHORD):
    case(FILT):     case(GREQ):     case(SPLIT):    case(ARPE):     case(PLUCK):    case(S_TRACE):
    case(BLTR):     case(ACCU):     case(EXAG):     case(FOCUS):    case(FOLD):     case(FREEZE):
    case(STEP):     case(AVRG):     case(BLUR):     case(SUPR):     case(CHORUS):   case(NOISE):
    case(SCAT):     case(SPREAD):   case(SHIFT):    case(GLIS):     case(WAVER):    case(INVERT):
    case(FMNTSEE):  case(FORMSEE):  case(LEVEL):    case(MAKE):     case(PITCH):    case(TRACK):
    case(TRNSP):    case(TRNSF):    case(DIFF):     case(REPITCHB): case(OCTVU):    case(PEAK):
    case(REPORT):   case(PRINT):    case(P_WRITE):  case(P_APPROX): case(P_CUT):    case(REPITCH):
    case(FORMANTS): case(VOCODE):   case(LEAF):     case(MEAN):     case(CROSS):    case(P_EXAG):
    case(P_INVERT): case(P_SEE):    case(P_HEAR):   case(P_SMOOTH): case(P_FIX):    case(TSTRETCH):
    case(P_QUANTISE):       case(P_RANDOMISE):      case(P_TRANSPOSE):      case(P_VIBRATO):
    case(DISTORT):          case(DISTORT_ENV):      case(DISTORT_OMT):      case(GRAIN_GET):
    case(ENV_DOVETAILING):  case(ENV_SWELL):        case(ENV_ATTACK):       case(ENV_ENVTOBRK):
    case(ENV_ENVTODBBRK):   case(ENV_WARPING):      case(ENV_RESHAPING):    case(ENV_REPLOTTING):
    case(ENV_DBBRKTOENV):   case(ENV_DBBRKTOBRK):   case(ENV_BRKTODBBRK):   case(ENV_BRKTOENV):
    case(ENV_EXTRACT):      case(ENV_TREMOL):       case(ENV_PLUCK):        case(MIXDUMMY):
    case(MIXSYNC):          case(MIXSYNCATT):       case(MIXTWARP):         case(MIXSWARP):
    case(MIXGAIN):          case(MIXSHUFL):         case(MIXMAX):           case(EQ):
    case(LPHP):             case(FSTATVAR):         case(FLTBANKN):         case(FLTBANKC):
    case(FLTBANKU):         case(FLTBANKV):         case(FLTITER):          case(FLTSWEEP):
    case(ALLPASS):          case(BRASSAGE):         case(PVOC_ANAL):        case(PVOC_SYNTH):
    case(PVOC_EXTRACT):     case(MOD_SPACE):        case(FREEZE2):          case(FLTBANKV2):
    case(ACC_STREAM):       case(DISTORT_OVERLOAD): case(SCALED_PAN):       case(TIME_GRID):
    case(MIX_PAN):          case(SHUDDER):          case(DOUBLETS):         case(ENVSYN):
    case(GREV):
        return(TRUE);
    case(MOD_LOUDNESS):
        switch(mode) {
        case(LOUD_PROPOR):  case(LOUD_DB_PROPOR):
            return(FALSE);
        default:
            return(TRUE);
        }
        break;
    case(CONVOLVE):
        switch(mode) {
        case(CONV_NORMAL):  return(FALSE);
        case(CONV_TVAR):    return(TRUE);
        }
        break;
    case(MOD_RADICAL):
        switch(mode) {
        case(MOD_REVERSE):  return(TRUE);
        case(MOD_SHRED):    return(TRUE);
        case(MOD_SCRUB):    return(FALSE);
        case(MOD_LOBIT):    return(TRUE);
        case(MOD_LOBIT2):   return(TRUE);
        case(MOD_RINGMOD):  return(TRUE);
        case(MOD_CROSSMOD): return(FALSE);
        }
        break;
    case(MOD_PITCH):
        switch(mode) {
        case(MOD_VIBRATO):  return(TRUE);
        default:            return(FALSE);
        }
        break;
    case(CHANNEL):      case(FREQUENCY):    case(P_INFO):       case(P_ZEROS):
    case(CUT):          case(GRAB):         case(MAGNIFY):      case(DRUNK):        case(SHUFFLE):
    case(WEAVE):        case(GLIDE):        case(BRIDGE):       case(MORPH):        case(WARP):
    case(FORM):         case(SUM):          case(MAX):          case(WINDOWCNT):    case(DISTORT_RPTFL):
    case(DISTORT_AVG):  case(DISTORT_MLT):  case(DISTORT_DIV):  case(DISTORT_HRM):  case(DISTORT_FRC):
    case(DISTORT_REV):  case(DISTORT_SHUF): case(DISTORT_RPT):  case(DISTORT_INTP): case(DISTORT_DEL):
    case(DISTORT_RPL):  case(DISTORT_TEL):  case(DISTORT_FLT):  case(DISTORT_INT):  case(DISTORT_CYCLECNT):
    case(ZIGZAG):       case(LOOP):         case(SCRAMBLE):     case(ITERATE):      case(DRUNKWALK):
    case(DISTORT_PCH):  case(SIMPLE_TEX):   case(TIMED):        case(GROUPS):       case(TGROUPS):
    case(DECORATED):    case(PREDECOR):     case(POSTDECOR):    case(ORNATE):       case(PREORNATE):
    case(POSTORNATE):   case(MOTIFS):       case(TMOTIFS):      case(MOTIFSIN):     case(TMOTIFSIN):
    case(MIX):          case(MIXCROSS):     case(MIXINTERL):    case(MIXINBETWEEN): case(ITERATE_EXTEND):
    case(MIXTEST):      case(MIXFORMAT):    case(WORDCNT):      case(GRAIN_COUNT):  case(GRAIN_REORDER):
    case(GRAIN_ALIGN):  case(ENV_CREATE):   case(GRAIN_OMIT):   case(GRAIN_REMOTIF):
    case(ENV_CURTAILING):   case(GRAIN_POSITION):   case(GRAIN_REPITCH):    case(GRAIN_DUPLICATE):
    case(GRAIN_RERHYTHM):   case(GRAIN_TIMEWARP):   case(GRAIN_REVERSE):    case(BATCH_EXPAND):
    case(MOD_REVECHO):      case(SAUSAGE):          case(HOUSE_COPY):       case(HOUSE_DISK):
    case(HOUSE_EXTRACT):    case(HOUSE_BAKUP):      case(HOUSE_DEL):        case(MAKE_VFILT):
    case(HOUSE_CHANS):      case(HOUSE_BUNDLE):     case(HOUSE_SORT):       case(HOUSE_SPEC):
    case(EDIT_CUT):         case(EDIT_CUTEND):      case(EDIT_ZCUT):        case(EDIT_EXCISE):
    case(EDIT_EXCISEMANY):  case(EDIT_INSERT):      case(EDIT_INSERTSIL):   case(EDIT_JOIN):
    case(INFO_PROPS):       case(INFO_SFLEN):       case(INFO_TIMELIST):    case(INFO_TIMESUM):
    case(INFO_TIMEDIFF):    case(INFO_SAMPTOTIME):  case(INFO_TIMETOSAMP):  case(INFO_MAXSAMP):
    case(INFO_LOUDCHAN):    case(INFO_FINDHOLE):    case(INFO_DIFF):        case(INFO_CDIFF):
    case(INFO_PRNTSND):     case(INFO_MUSUNITS):    case(INSERTSIL_MANY):   case(INFO_MAXSAMP2):
    case(SYNTH_WAVE):       case(SYNTH_NOISE):      case(SYNTH_SIL):        case(RANDCUTS):
    case(RANDCHUNKS):       case(SIN_TAB):          case(HF_PERM1):         case(HF_PERM2):
    case(DEL_PERM):         case(DEL_PERM2):        case(SYNTH_SPEC):       case(TWIXT):
    case(SPHINX):           case(INFO_LOUDLIST):    case(NOISE_SUPRESS):    case(JOIN_SEQ):
    case(P_SYNTH):          case(P_INSERT):         case(P_PTOSIL):         case(P_NTOSIL):
    case(P_SINSERT):        case(ANALENV):          case(MAKE2):            case(P_VOWELS):
    case(HOUSE_DUMP):       case(HOUSE_RECOVER):    case(HOUSE_GATE):       case(MIX_ON_GRID):
    case(P_GEN):            case(P_INTERP):         case(AUTOMIX):          case(EDIT_CUTMANY):
    case(STACK):            case(VFILT):            case(ENV_PROPOR):       case(DISTORT_PULSED):
    case(SEQUENCER):        case(BAKTOBAK):         case(ADDTOMIX):         case(EDIT_INSERT2):
    case(MIX_AT_STEP):      case(FIND_PANPOS):      case(CLICK):            case(SYLLABS):
    case(MIX_MODEL):        case(CYCINBETWEEN):     case(JOIN_SEQDYN):      case(TOPNTAIL_CLICKS):
    case(P_BINTOBRK):       case(SEQUENCER2):       case(RRRR_EXTEND):      case(HOUSE_GATE2):
    case(GRAIN_ASSESS):     case(MULTI_SYN):        case(DISTORT_RPT2):     case(ZCROSS_RATIO):
    case(SSSS_EXTEND):      case(MANY_ZCUTS):
        return(FALSE);

    case(ENV_IMPOSE):
    case(ENV_REPLACE):
        switch(mode) {
        case(ENV_BRKFILE_IN):
        case(ENV_DB_BRKFILE_IN):
            return(FALSE);
        case(ENV_ENVFILE_IN):
        case(ENV_SNDFILE_IN):
            return(TRUE);
        }
        break;
    case(MIXTWO):
    case(MIXMANY):
        return(FALSE);
        break;
    case(MIXBALANCE):
        return(TRUE);
        break;
    default:
        sprintf(errstr,"Unknown case in specify_brkpntfile_len()\n");
        return(PROGRAM_ERROR);
    }
    return(FALSE);
}

