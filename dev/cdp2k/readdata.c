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



/* floatsam version*/
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <processno.h>
#include <special.h>
#include <cdpmain.h>

#include <sfsys.h>
#include <string.h>         /*RWD*/

#if defined unix || defined _MSC_VER
#define round(x) lround((x))
#endif

static int  get_params(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int  get_options(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int  get_variants_and_flags(int *cmdlinecnt,char ***cmdline,dataptr dz);
static int  get_option_no(char *paramstr,int *option_no,aplptr ap);
static int  get_variant_no(char *paramstr,int *variant_no,aplptr ap);
static int  get_brkpnt_data_from_file_and_test_it(char *filename,int paramno,dataptr dz);
static int  read_param_as_value_or_brkfile_and_check_range(char *str,int paramno,dataptr dz);
static int  get_real_paramno(int paramno,dataptr dz);
static void out_of_range(int paramno,double val,double loval,double hival);

/************************ READ_PARAMETERS_AND_FLAGS **********************/

int read_parameters_and_flags(char ***cmdline,int *cmdlinecnt,dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    if(ap->param_cnt) {
        if(!sloom) {
            if(*cmdlinecnt <= 0) {
                sprintf(errstr,"Insufficient parameters on command line.\n");
                return(USAGE_ONLY);
            }
        }
        if((exit_status = get_params(cmdlinecnt,cmdline,dz))<0)
            return(exit_status);
    }
    if(ap->option_cnt) {
        if((exit_status = get_options(cmdlinecnt,cmdline,dz))<0)
            return(exit_status);
    }
    if(ap->vflag_cnt) {
        if((exit_status = get_variants_and_flags(cmdlinecnt,cmdline,dz))<0)
            return(exit_status);
    }
    if(!sloom) {
        if(*cmdlinecnt > 0) {
            if((*cmdline)[0][0]=='-') {
                if(strlen((*cmdline)[0])>1)
                    sprintf(errstr,"Unknown flag -%c on command line.\n",(*cmdline)[0][1]);
                else
                    sprintf(errstr,"Hanging '-' on command line.\n");
            } else
                sprintf(errstr,"Too many parameters on command line.\n");
            return(USAGE_ONLY);
        }
    }
    return(FINISHED);
}       
    
/************************ GET_PARAMS **********************/

int get_params(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int exit_status;
    int n;
    char *paramval;
    aplptr ap = dz->application;

    for(n=0;n<ap->max_param_cnt;n++) {
        if(dz->is_active[n]) {
            if(!sloom) {

                if(*cmdlinecnt<=0) {
                    sprintf(errstr,"Insufficient parameters on cmdline.\n");
                    return(USER_ERROR);
                }
            }
            paramval = (*cmdline)[0];
            if((exit_status = read_param_as_value_or_brkfile_and_check_range(paramval,n,dz)) < 0)
                return(exit_status);
            (*cmdline)++;
            (*cmdlinecnt)--;
        }
    }
    return(FINISHED);
}

/************************ GET_OPTIONS **********************/

int get_options(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int  exit_status;
    aplptr ap = dz->application;
    int  n, basecnt = ap->max_param_cnt;
    int m = 0;
    char *paramstr;
    int* options_got;
    int  option_no = 0, k = 0;
    int  options_remain = TRUE;

    if(!sloom) {
        if((options_got = (int *)malloc(ap->option_cnt * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for option checking array\n");
            return(MEMORY_ERROR);
        }
        for(n=0;n<ap->option_cnt;n++)
            options_got[n]=FALSE;
        while(*cmdlinecnt > 0 && options_remain) {
            paramstr = (*cmdline)[0];
            exit_status = get_option_no(paramstr,&option_no,dz->application);
            switch(exit_status) {
            case(CONTINUE):
                if(options_got[option_no]==TRUE) {
                    sprintf(errstr,"Duplicate option %c used on command line\n",*(paramstr+1));
                    return(USAGE_ONLY);
                }
                options_got[option_no] = TRUE;  
                paramstr += 2;
                k = basecnt + option_no;
                if((exit_status = read_param_as_value_or_brkfile_and_check_range(paramstr,k,dz))<0)
                    return(exit_status);
                (*cmdline)++;
                (*cmdlinecnt)--;
                break;
            case(FINISHED):
                options_remain = FALSE;
                break;
            default:
                return(exit_status);
            }
        }       
        free(options_got);
    } else {
        for(n=0,m=basecnt;n<ap->option_cnt;n++,m++) {
//TW JULY 2006
            if(dz->is_active[m]) {
                paramstr = (*cmdline)[0];
                if((exit_status = read_param_as_value_or_brkfile_and_check_range(paramstr,m,dz))<0)
                    return(exit_status);
                (*cmdline)++;
                (*cmdlinecnt)--;
            }
        }
    }
    return(FINISHED);
}

/************************ GET_VARIANTS_AND_FLAGS **********************/

int get_variants_and_flags(int *cmdlinecnt,char ***cmdline,dataptr dz)
{
    int exit_status;
    aplptr ap = dz->application;
    int basecnt = ap->max_param_cnt + ap->option_cnt;
    char *paramstr;                   
    int flagno, paramno;
    int* flags_got;
    int n = 0, rawflags = 0;
    if(!sloom) {
        if((flags_got = (int *)malloc(ap->vflag_cnt * sizeof(int)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for variant checking array\n");
            return(MEMORY_ERROR);
        }
        for(flagno=0;flagno<ap->vflag_cnt;flagno++)
            flags_got[flagno]=FALSE;
        while(*cmdlinecnt > 0) {
            paramstr = (*cmdline)[0];
            if((exit_status = get_variant_no(paramstr,&flagno,ap))<0)
                return(exit_status);
            if(flags_got[flagno]==TRUE) {
                sprintf(errstr,"Duplicate flag %c used on command line\n",*(paramstr+1));
                return(USER_ERROR);
            }
            flags_got[flagno] = TRUE;
            if(flagno < ap->variant_param_cnt) {    
                paramstr += 2;
                paramno = basecnt+flagno;   
                if((exit_status = read_param_as_value_or_brkfile_and_check_range(paramstr,paramno,dz))<0)
                    return(exit_status);
                if(dz->brksize[paramno] || !flteq(dz->param[paramno],ap->default_val[paramno]))
                    dz->vflag[flagno] = TRUE;
            } else
                dz->vflag[flagno] = TRUE;
            (*cmdline)++;
            (*cmdlinecnt)--;
        }
        free(flags_got);
    } else {
        rawflags = ap->vflag_cnt - ap->variant_param_cnt;

        for(flagno=0,paramno=basecnt;flagno<ap->variant_param_cnt;flagno++,paramno++) {
            paramstr = (*cmdline)[0];
            if((exit_status = read_param_as_value_or_brkfile_and_check_range(paramstr,paramno,dz))<0)
                return(exit_status);
            if(dz->brksize[paramno] || !flteq(dz->param[paramno],ap->default_val[paramno]))
                dz->vflag[flagno] = TRUE;
            (*cmdline)++;
            (*cmdlinecnt)--;
        }
        if(rawflags > 0) {
            basecnt = ap->variant_param_cnt;
            for(n=0,flagno = basecnt;n<rawflags;n++,flagno++) {
                paramstr = (*cmdline)[0];
    /* NEW JUNE 1999 ---> */
                if (strlen(paramstr) < 2) {
                    sprintf(errstr,"Unmarked numeric val for flag %s\n",paramstr);    /*RWD 4:12:2003 was $s */
                    return(DATA_ERROR);
                } 
                paramstr++;
    /* <--- NEW JUNE 1999 */
                if(!strcmp(paramstr,"0"))
                    ;   /* flag = FALSE, default */
                else if(!strcmp(paramstr,"1"))
                    dz->vflag[flagno] = TRUE;
                else {
                    sprintf(errstr,"Unknown flag value '%s' sent from TK\n",paramstr);
                    return(DATA_ERROR);
                }
                (*cmdline)++;
                (*cmdlinecnt)--;
            }
        } 
    }

    return(FINISHED);
}

/************************************* GET_OPTION_NO  ****************************************/

int get_option_no(char *paramstr,int *option_no,aplptr ap)
{
    char cmdline_flag, *p;
    if(*paramstr++!='-') {
        sprintf(errstr,"Unknown parameter '%s'\n",--paramstr);
        return(USAGE_ONLY);
    }
    cmdline_flag = *paramstr++;
    p = ap->option_flags;
    while(*p != ENDOFSTR) {
        if(*p == cmdline_flag) {
            *option_no = p - ap->option_flags;              
//TW JULY 2006
            if(ap->option_list[*option_no]=='0') {
                fprintf(stdout,"-%c is not a valid flag\n",cmdline_flag);
                fflush(stdout);
                return(USAGE_ONLY);
            }
            p++;
            if(strlen(paramstr) <=0) {
                sprintf(errstr,"option parameter missing with flag -%c\n",cmdline_flag);
                return(USAGE_ONLY);
            }
            return(CONTINUE);
        }
        p++;
    }
    return(FINISHED);
}

/************************************ GET_VARIANT_NO  ************************************/

int get_variant_no(char *paramstr,int *variant_no,aplptr ap)
{

    char cmdline_flag, *p;
    if(*paramstr++!='-') {
        sprintf(errstr,"Unknown parameter '%s'",--paramstr);
        return(USER_ERROR);
    }
    cmdline_flag = *paramstr++;
    p = ap->variant_flags;
    while(*p != ENDOFSTR) {
        if(*p == cmdline_flag) {                
            *variant_no = p - ap->variant_flags;
            p++;
            if(*variant_no < ap->variant_param_cnt && strlen(paramstr) <=0) {
                sprintf(errstr,"variant parameter missing with flag -%c\n",cmdline_flag);
                return(USER_ERROR);
            }
            return(CONTINUE);
        }
        p++;
    }
    if(ap->option_cnt) {
        p = ap->option_flags;
        while(*p != ENDOFSTR) {
            if(*p == cmdline_flag) {
                sprintf(errstr,"option flag -%c out of order on cmdline.\n",cmdline_flag);
                return(USER_ERROR);
            }
            p++;
        }
        sprintf(errstr,"Unknown variant flag -%c\n",cmdline_flag);
        return(USER_ERROR);
    } else {
        sprintf(errstr,"Unknown flag '-%c'\n",cmdline_flag);
        return(USER_ERROR);
    }
    return(FINISHED);
}

/***************** READ_PARAM_AS_VALUE_OR_BRKFILE_AND_CHECK_RANGE ********************/

int read_param_as_value_or_brkfile_and_check_range(char *str,int paramno,dataptr dz)
{
    aplptr ap = dz->application;
    int real_paramno;

    if(!sloom) {                /* CMDLINE convention: filenames can't begin with numbers : */
//TW Soundloom accepts numeric names, and extensions
//   'value_is_numeric' Commandline still looks for numeric value first, 
//   and if it finds it, assumes it's a number (if user uses filename "123.456" that's the user's problem)
//   New 'file_has_invalid_startchar' traps bad directory paths, but permits numeric names

        if(!value_is_numeric(str) && file_has_invalid_startchar(str)) {
            sprintf(errstr,"Cannot read parameter %d [%s]\n",paramno+1,str);
            return(USER_ERROR);
        }

        if(value_is_numeric(str)) {
            if(sscanf(str,"%lf",&(dz->param[paramno]))<1) {
                sprintf(errstr,"Cannot read parameter %d [%s].\n",paramno+1,str);
                return(USER_ERROR);
            }   
            if(dz->process != ENV_DOVETAILING && dz->process != ENV_CURTAILING) {
                if(flteq(dz->param[paramno],ap->lo[paramno]))
                    dz->param[paramno] = ap->lo[paramno];
                if(flteq(dz->param[paramno],ap->hi[paramno]))
                    dz->param[paramno] = ap->hi[paramno];
                if(dz->param[paramno] > ap->hi[paramno] || dz->param[paramno] < ap->lo[paramno]) {
                    real_paramno = get_real_paramno(paramno,dz);
                    out_of_range(real_paramno,dz->param[paramno],ap->lo[paramno],ap->hi[paramno]);
                    return(USER_ERROR);
                }
            }
            if(dz->is_int[paramno])
                dz->iparam[paramno] = round(dz->param[paramno]);
        } else {
            if(dz->no_brk[paramno]) {
                sprintf(errstr,"Cannot read parameter %d [%s]: brkpnt_files not permitted.\n",paramno+1,str);
                return(USER_ERROR);
            } else 
                return get_brkpnt_data_from_file_and_test_it(str,paramno,dz);
        }
    } else {                    /* TK convention, all numeric values are preceded by NUMERICVAL_MARKER */
        if(str[0]==NUMERICVAL_MARKER) {      
            str++;              
            if(strlen(str)<=0 || sscanf(str,"%lf",&(dz->param[paramno]))!=1) {
                sprintf(errstr,"Invalid parameter value encountered.\n");
                return(DATA_ERROR);
            }
            if(dz->process != ENV_DOVETAILING && dz->process != ENV_CURTAILING) {
                if(flteq(dz->param[paramno],ap->lo[paramno]))
                    dz->param[paramno] = ap->lo[paramno];
                if(flteq(dz->param[paramno],ap->hi[paramno]))
                    dz->param[paramno] = ap->hi[paramno];
                if(dz->param[paramno] > ap->hi[paramno] || dz->param[paramno] < ap->lo[paramno]) {
                    real_paramno = get_real_paramno(paramno,dz);
                    out_of_range(real_paramno,dz->param[paramno],ap->lo[paramno],ap->hi[paramno]);
                    return(USER_ERROR);
                }
            }

            if(dz->is_int[paramno])
                dz->iparam[paramno] = round(dz->param[paramno]);
        } else {
            if(dz->no_brk[paramno]) {
                sprintf(errstr,"Cannot read parameter %d [%s]: brkpnt_files not permitted.\n",paramno+1,str);
                return(USER_ERROR);
            }
            else 
                return get_brkpnt_data_from_file_and_test_it(str,paramno,dz);
        }
    }
    return(FINISHED);
}

/************************************* GET_REAL_PARAMNO ****************************************/

int get_real_paramno(int paramno,dataptr dz)
{
    int m = 0, n;
    for(n=0;n <= paramno;n++) {
        if(dz->is_active[n])
            m++;
    }
    if(dz->application->special_data)
        m++;
    return(m);
}

/*************************** GET_BRKPNT_DATA_FROM_FILE_AND_TEST_IT ***********************/

int get_brkpnt_data_from_file_and_test_it(char *filename,int paramno,dataptr dz)
{
    FILE *fp;
    aplptr ap = dz->application;
    double *p, lasttime = 0.0;
    int istime = 1;
    int arraysize = BIGARRAY;
    char temp[200], *q;
    int n = 0, dcount;
    if((fp = fopen(filename,"r"))==NULL) {
        sprintf(errstr, "Can't open brkpntfile %s to read data.\n",filename);
        return(DATA_ERROR);
    }
    if((dz->brk[paramno] = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for brkpnt data in file %s.\n",filename);
        return(MEMORY_ERROR);
    }
    p = dz->brk[paramno];
    while(fgets(temp,200,fp)==temp) {
        q = temp;
        while(get_float_from_within_string(&q,p)) {
            if(istime) {
                if(p!= dz->brk[paramno] && *p <= lasttime) {
                    sprintf(errstr,"Times (%lf & %lf) in brkpntfile %s are not in increasing order.\n",
                    lasttime,*p,filename);
                    return(DATA_ERROR);
                }
                lasttime = *p;
            } else {
                if(flteq(*p,ap->lo[paramno]))
                    *p = ap->lo[paramno];
                else if(flteq(*p,ap->hi[paramno]))
                    *p = ap->hi[paramno];
                if(*p < ap->lo[paramno] || *p > ap->hi[paramno]) {
                    out_of_range_in_brkfile(filename,*p,ap->lo[paramno],ap->hi[paramno]); 
                    return(DATA_ERROR);
                }
            }
            istime = !istime;
            p++;
            if(++n >= arraysize) {
                arraysize += BIGARRAY;
                if((dz->brk[paramno] = (double *)realloc((char *)(dz->brk[paramno]),arraysize * sizeof(double)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate brkpnt data from file %s.\n",filename);
                    return(MEMORY_ERROR);
                }
                p = dz->brk[paramno] + n;       
            }
        }
    }       
    if(n == 0) {
        sprintf(errstr,"No data in brkpnt file %s\n",filename);
        return(DATA_ERROR);
    }
    if((dz->brk[paramno] = (double *)realloc((char *)(dz->brk[paramno]),n * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate brkpnt data from file %s.\n",filename);
        return(MEMORY_ERROR);
    }
    if(((dcount = n/2) * 2) != n) {
        sprintf(errstr,"Data not paired correctly in file %s\n",filename);
        return(DATA_ERROR);
    }
    if(fclose(fp)<0) {
        fprintf(stdout,"WARNING: Failed to close file %s.\n",filename);
        fflush(stdout);
    }
    dz->brksize[paramno] = dcount;
    dz->brkptr[paramno]  = dz->brk[paramno];    /* initialise brkptr */
    return(FINISHED);
}

/**************************** OUT_OF_RANGE_IN_BRKFILE *************************/

void out_of_range_in_brkfile(char *filename,double val,double loval,double hival) 
{
    sprintf(errstr,"Value (%lf) out of range (%lf to %lf) in brkpntfile %s.\n",val,loval,hival,filename);
}

/**************************** OUT_OF_RANGE *************************/

void out_of_range(int paramno,double val,double loval,double hival) 
{
    sprintf(errstr,"Parameter[%d] Value (%lf) out of range (%lf to %lf)\n",paramno,val,loval,hival);
}

/************************* READ_AND_TEST_PITCH_OR_TRANSPOSITION_BRKVALS *****************************/

int read_and_test_pitch_or_transposition_brkvals
(FILE *fp,char *filename,double **brktable,int *brksize,int which_type,double minval,double maxval)
{
    int arraysize = BIGARRAY;
    double *p, lasttime = 0.0;
    int  istime = TRUE;
    int n = 0,m, final_size;
    char temp[200], *q;
    if((*brktable = (double *)malloc(arraysize * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY for pitch-or-transposition brktable.\n");
        return(MEMORY_ERROR);
    }
    p = *brktable;
    while(fgets(temp,200,fp)==temp) {    /* READ AND TEST BRKPNT VALS */
        q = temp;
        while(get_float_from_within_string(&q,p)) {
            if(istime) {
                if(p==*brktable) {
                    if(*p < 0.0) {
                        sprintf(errstr,"First timeval(%lf) in brkpntfile %s is less than zero.\n",*p,filename);
                        return(DATA_ERROR);
                    }
                } else {
                    if(*p <= lasttime) {
                        sprintf(errstr,"Times (%lf & %lf) in brkpntfile %s are not in increasing order.\n",
                        lasttime,*p,filename);
                        return(DATA_ERROR);
                    }
                }
                lasttime = *p;
            } else {
                switch(which_type) {
                case(TRANSPOS_SEMIT_OR_CONSTANT):
                    *p = *p/12.0;       /* semitone to octave */
                    /* fall thro */
                case(TRANSPOS_OCTAVE_OR_CONSTANT):
                    *p = pow(2.0,*p);   /* octave to frqratio */
                    break;
                case(NO_SPECIAL_TYPE):
                    break;
                }
                if(flteq(*p,minval))
                    *p = minval;
                else if(flteq(*p,maxval))
                    *p = maxval;
                if(*p < minval || *p > maxval) {
                    out_of_range_in_brkfile(filename,*p,minval,maxval); 
                    return(DATA_ERROR);
                }
            }
            istime = !istime;
            p++;
            if(++n >= arraysize) {
                arraysize += BIGARRAY;
                if((*brktable = (double *)realloc((char *)(*brktable),arraysize * sizeof(double)))==NULL) {
                    sprintf(errstr,"INSUFFICIENT MEMORY to reallocate pitch-or-transposition brktable.\n");
                    return(MEMORY_ERROR);
                }
                p = *brktable + n;      
            }
        }
    }       
    if(n < 2) {
        sprintf(errstr,"No data in brkpnt file %s\n",filename);
        return(DATA_ERROR);
    }
    if(ODD(n)) {
        sprintf(errstr,"Data not paired correctly in brkpntfile %s\n",filename);
        return(DATA_ERROR);
    }
    final_size = n;
    if((*brktable)[0] != 0.0)   /* Allow space for a value at time zero, if there isn't one */
        final_size = n + 2;
    if((*brktable = (double *)realloc((char *)(*brktable),final_size * sizeof(double)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate pitch-or-transposition brktable.\n");
        return(MEMORY_ERROR);
    }
    if(final_size != n) {       /* Force a value at time zero, if there isn't one */
        for(m=n-1;m>=0;m--)
            (*brktable)[m+2] = (*brktable)[m];
        (*brktable)[0] = 0.0;   
        (*brktable)[1] = (*brktable)[3];
    }
    *brksize = final_size/2;
    return(FINISHED);
}

/************************* CONVERT_TO_WINDOW_BY_WINDOW_ARRAY *****************************/

int convert_brkpntdata_to_window_by_window_array(double *brktable,int brksize,float **thisarray,int wlen,float timestep)
{
    int exit_status;
    double *p, val;
    int arraysize = BIGARRAY, n = 0;
    double ttime = 0.0;
    double firstval, lasttime, lastval;
    double *endpair, *endoftab;
    if((*thisarray = (float *)malloc(arraysize * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to create window array.\n");
        return(MEMORY_ERROR);
    }
    p = brktable;
    firstval = *(p+1);
    endoftab = brktable + (brksize*2);
    endpair  = endoftab - 2;
    lasttime = *endpair;
    lastval  = *(endpair+1);
    while(n < wlen) {
        if(ttime <= 0.0)            (*thisarray)[n] = (float)firstval;
        else if(ttime >= lasttime)  (*thisarray)[n] = (float)lastval;
        else {
            if((exit_status = interp_val(&val,ttime,brktable,endoftab,&p))<0)
                return(exit_status);
            (*thisarray)[n] = (float)val;
        }
        if(++n >= arraysize) {
            arraysize += BIGARRAY;
            if((*thisarray = (float *)realloc((char *)(*thisarray),arraysize*sizeof(float)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY to reallocate window array.\n");
                return(MEMORY_ERROR);
            }
        }
        ttime += timestep;
    }
    if((*thisarray = (float *)realloc((char *)(*thisarray),(wlen+1) * sizeof(float)))==NULL) {
        sprintf(errstr,"INSUFFICIENT MEMORY to reallocate window array.\n");
        return(MEMORY_ERROR);
    }
    (*thisarray)[wlen] = 0.0f;  /* SAFETY VALUE */
    return(FINISHED);
}

/********************** INTERP_VAL *******************/

int interp_val(double *val,double ttime,double *startoftab,double *endoftab,double **p)
{
    double hival, hiind, loval, loind, dval;
    if(ttime > **p) {
        while(**p<ttime) {
            if((*p += 2) >= endoftab) {
                sprintf(errstr,"Time off end of table :interp_val()\n");
                return(PROGRAM_ERROR);
            }
        }
    }
    if(*p < startoftab+2) {
        sprintf(errstr,"Pointer off start of table: Error in interp_val()\n");
        return(PROGRAM_ERROR);
    }
    hival  = *(*p+1);
    hiind  = **p;
    loval  = *(*p-1);
    loind  = *(*p-2);
    dval    = (ttime - loind)/(hiind - loind);
    dval   *= (hival - loval);
    dval   += loval;
    *val    = dval;
    return(FINISHED);
}

/**************************** CONVERT_DB_AT_OR_BELOW_ZERO_TO_GAIN ********************************/

int convert_dB_at_or_below_zero_to_gain(double *val)
{
    if (*val>0.0)  {
        sprintf(errstr,"dB value out of range (> 0dB)\n");
        return(DATA_ERROR);
    } else if(*val<=MIN_DB_ON_16_BIT)
        *val = 0.0;
    else if(flteq(*val,0.0))
        *val = 1.0;
    else /* *val<0.0 */ {
        *val  = -(*val);
        *val /= 20.0;
        *val  = pow(10.0,*val);
        *val  = 1.0/(*val);
    }
    return(FINISHED);
}

/*************************** GET_MAXVALUE_IN_BRKTABLE ************************/

int get_maxvalue_in_brktable(double *brkmax,int paramno,dataptr dz)
{
    double *p = dz->brk[paramno];
    int n;
    *brkmax = -DBL_MAX;
    if(dz->brksize[paramno] <= 0) {
        sprintf(errstr,"Brktable is empty.\n");
        return(DATA_ERROR);
    }
    p++;
    for(n=0;n<dz->brksize[paramno];n++) {
        if(*p > *brkmax)
            *brkmax = *p;
        p+=2;
    }
    if(*brkmax <= -DBL_MAX) {
        sprintf(errstr,"Invalid values in brktable.\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/*************************** GET_MINVALUE_IN_BRKTABLE ************************/

int get_minvalue_in_brktable(double *brkmin,int paramno,dataptr dz)
{
    double *p = dz->brk[paramno];
    int n;
    *brkmin = DBL_MAX;
    if(dz->brksize[paramno] <= 0) {
        sprintf(errstr,"Brktable is empty.\n");
        return(DATA_ERROR);
    }
    p++;
    for(n=0;n<dz->brksize[paramno];n++) {
        if(*p < *brkmin)
            *brkmin = *p;
        p+=2;
    }
    if(*brkmin >= DBL_MAX) {
        sprintf(errstr,"Invalid values in brktable.\n");
        return(DATA_ERROR);
    }
    return(FINISHED);
}

/********************* GET_MAXVALUE *********************/

int get_maxvalue(int paramno,double *maxval,dataptr dz)
{
    int exit_status;
    if(dz->brksize==NULL) {
        sprintf(errstr,"brksize array not initialised: get_maxvalue()\n");
        return(PROGRAM_ERROR);
    } 
    if(dz->brksize[paramno]) {
        if((exit_status = get_maxvalue_in_brktable(maxval,paramno,dz))<0)
            return(exit_status);
    } else {
        if(dz->is_int[paramno])
            *maxval = (double)dz->iparam[paramno];
        else
            *maxval = dz->param[paramno];
    }
    return(FINISHED);
}

