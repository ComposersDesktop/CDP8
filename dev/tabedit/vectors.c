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



/* Manipulate 2 columns of numbers (vectors) */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <osbind.h>
#include <time.h>

#define PROG    "VECTORS"

#define BIGARRAY      200
#define FLTERR        0.000002
#define ENDOFSTR        '\0'

#include <sfsys.h>
#ifdef unix
#define round lround
#endif


int     strgetfloat(char **,double  *);
double  miditohz(double);
char    *exmalloc(int), *exrealloc(char *,int);
void    logo(void), usage(void), help(void);
void    read_flags(int,int,char *[]);
int     do_infile(char *,double **), do_outfile(char *);
void    check_usage(int argc,char *argv[]), truncate_vectors(void);
void    do_add(void),        do_subtract(void),   do_multiply(void), do_mean(void);
void    do_divide(void),     do_overwrite(void),  do_insert(void);
void    do_interleave(void), do_power(void),      do_randadd(void);
void    do_patterned_interleave(void);
void    do_randmult(void),   do_randscat(void),   do_substitute(void);
void    do_quantise(void),      do_squeezed_pan(void);
void    quantise_sort(double *scti,int *scatcnt);
void    conjoina(void);
void    conjoinb(void);
void    conjoin_sort(void);
void    do_max(void);
void    do_min(void);

void    do_keep(void);
void    do_del(void);
void    compare(void);
void    del_copys(void);
void    insert_non_dupls(void);
void    partition_col(int atpos,int partition_outcol);
void    do_keep_in_spans(int is_del);
void    do_del_in_spans(void);
void    do_keep_cycles(int do_keep);
void    do_matches(void);
void    do_morph(void);
void    do_morphseq(void);

static void do_error(void);
static void do_valout(double val);
static int do_stringfile(char *argv,char ***stringptr);

double other_param;
double *other_params;
char **string0, **string1;

int     cnt = 0, cnt2, arraysize = BIGARRAY, ifactor, flagstart;
double  *number1, *number2, *diffs, scatter, factor, errorbnd = FLTERR;
FILE    *fp = NULL;
char    flag = 0, errstr[400];
int     ro = 0;
int sloom = 0;
int sloombatch = 0;
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
    char *p;
    fp = stdout;
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if(argc < 2) {
        fprintf(stdout,"ERROR: Invalid command\n");
        fflush(stdout);
        usage();
    }
    if(!strcmp(argv[1],"#")) {
        sloom = 1;
        argv++;
        argc--;
    } else if (!strcmp(argv[1],"##")) {
        p = argv[0];
        argc--;
        argv++;
        argv[0] = p;
        sloombatch = 1;
    }
    check_usage(argc,argv);
    if(sloom)
        flagstart = 3;
    initrand48();
    read_flags(flagstart,argc,argv);
    if(flag == 'I' && ro == 'P') {
        if(((string0 = (char **)exmalloc(arraysize * sizeof(char *)))==NULL)
           || ((string1 = (char **)exmalloc(arraysize * sizeof(char *)))==NULL)) {
            sprintf(errstr,"Memory allocation failed.\n");
            do_error();
        }
        if((cnt  = do_stringfile(argv[1],&string0)) < 1) {
            fprintf(stdout,"ERROR: No numeric data in 1st column.\n");
            fflush(stdout);
            exit(1);
        }
        if((cnt2 = do_stringfile(argv[2],&string1)) < 1) {
            fprintf(stdout,"ERROR: No numeric data in 2nd column.\n");
            fflush(stdout);
            exit(1);
        }
    } else {
        if(((number1 = (double *)exmalloc(arraysize * sizeof(double)))==NULL)
           || ((number2 = (double *)exmalloc(arraysize * sizeof(double)))==NULL)) {
            sprintf(errstr,"Memory allocation failed.\n");
            do_error();
        }
        if((cnt  = do_infile(argv[1],&number1)) < 1) {
            fprintf(stdout,"ERROR: No numeric data in 1st column.\n");
            fflush(stdout);
            exit(1);
        }
        if((cnt2 = do_infile(argv[2],&number2)) < 1) {
            fprintf(stdout,"ERROR: No numeric data in 2nd column.\n");
            fflush(stdout);
            exit(1);
        }
    }
    if((flag == 'C' || flag == 'y') && (cnt != cnt2)) {
        fprintf(stdout,"ERROR: Columns are not of the same length.\n");
        fflush(stdout);
        exit(1);
    }
    if(flag != 'l' && flag != 'K' && flag != 'k' && flag != 'M' && !(flag=='I' && ro=='P'))
        truncate_vectors();
    switch(flag) {
    case('a'): do_add();            break;
    case('A'): do_mean();           break;
    case('b'): do_max();            break;
    case('B'): do_min();            break;
    case('d'): do_divide();         break;
    case('i'): do_insert();         break;
    case('m'): do_multiply();       break;
    case('M'):
        switch(ro) {
        case('O'):do_morph();           break;
        case('o'):do_morphseq();        break;
        default:  do_matches();         break;
        }
        break;
    case('o'): do_overwrite();      break;
    case('p'):
        switch(ro) {
        case('y'): partition_col(1,1);          break;          /* partition, at given positions in outcol, the input col*/
        case('n'): partition_col(0,1);          break;          /* partition, at other than given positions, the input col */
        case('Y'): partition_col(1,0);          break;          /* partition, at given positions in incol, the output col*/
        case('N'): partition_col(0,0);          break;          /* partition, at other than given positions, the output col */
        }
        break;
    case('q'): do_quantise();       break;
    case('s'): do_subtract();       break;
    case('C'): compare();           break;
    case('I'):
        switch(ro) {
        case(ENDOFSTR): do_interleave();        break;
        case('P'):              do_patterned_interleave();      break;
        }
        break;
    case('P'): do_power();          break;
    case('S'): do_substitute();     break;
    case('l'):
        switch(ro) {
        case('a'): conjoina();          break;
        case('b'): conjoinb();          break;
        case('s'): conjoin_sort();      break;
        }
        break;
    case('K'):
        switch(ro) {
        case('k'): do_keep();                   break;
        case('d'): do_del();                    break;
        case('K'): do_keep_in_spans(0); break;
        case('D'): do_keep_in_spans(1); break;
        case('c'): del_copys();                 break;
        case('S'): insert_non_dupls();  break;
        }
        break;
    case('k'):
        switch(ro) {
        case('k'): do_keep_cycles(1);   break;
        case('d'): do_keep_cycles(0);   break;
        }
        break;
    case('R'):
        switch(ro) {
        case('a'): do_randadd();        break;
        case('m'): do_randmult();       break;
        case('s'): do_randscat();       break;
        }
        break;
    case('y'):
        switch(ro) {
        case('z'): do_squeezed_pan();   break;
        }
        break;
    }
    fflush(stdout);
    return 0;
}

/************************ CHECK_USAGE *************************/

void check_usage(int argc,char *argv[])
{
    if(argc==2 && !strncmp(argv[1],"-h",2))
        help();
    switch(argc) {
    case(8):
        flagstart = argc-4;
        break;
    case(6):
        flagstart = argc-2;
        if(!do_outfile(argv[3])) {
            fprintf(stdout,"ERROR: Don't understand.\n");
            fflush(stdout);
            usage();
        }
        break;
    case(5):
        flagstart = argc-1;
        if(!do_outfile(argv[3])) {
            flagstart--;
        }
        break;
    case(4):
        flagstart = argc-1;
        break;
    case(2):
        if(!strncmp(argv[1],"-h",2))
            help();
    default:
        usage();
    }
}

/**************************STRGETFLOAT **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

int  strgetfloat(char **str,double *val)
{
    char *p, *q, *end;
    double numero;
    int     point, valid;
    for(;;) {
        point = 0;
        p = *str;
        while(isspace(*p))
            p++;
        q = p;
        if(!isdigit(*p) && *p != '.' && *p!='-')
            return(0);
        if(*p == '.'|| *p == '-') {
            if(*p == '-')
                p++;
            else {
                point++;
                p++;
            }
        }
        for(;;) {
            if(*p == '.') {
                if(point)
                    return(0);
                else {
                    point++;
                    p++;
                    continue;
                }
            }
            if(isdigit(*p)) {
                p++;
                continue;
            } else {
                if(!isspace(*p) && *p!='\0')
                    return(0);
                else {
                    end = p;
                    p = q;
                    valid = 0;
                    while(p!=end) {
                        if(isdigit(*p))
                            valid++;
                        p++;
                    }
                    if(valid) {
                        if(sscanf(q,"%lf",&numero)!=1)
                            return(0);
                        *val = numero;
                        *str = end;
                        return(1);
                    }
                    return(0);
                }
            }
        }
    }
    return(0);                              /* NOTREACHED */
}

/****************************** USAGE ****************************/

void usage(void)
{
    if(!sloom && !sloombatch) {
        logo();
        fprintf(stdout,"USAGE: vectors column1 column2 [outfile] -flag [-eERROR]\n\n");
        fprintf(stdout,"\tcolumnN  textfile containing list of numbers.\n");
        fprintf(stdout,"\toutfile  receives output as text (Default: screen)\n");
        fprintf(stdout,"\tflag     determines operation.\n");
        fprintf(stdout,"\t         use 'vectors -h' to see flags.\n");
        fprintf(stdout,"\t-e       ERROR is acceptable error in checking equality of numbers.\n");
        fprintf(stdout,"\t         (Default: %lf).\n",FLTERR);
        fflush(stdout);
    }
    exit(1);
}

/****************************** HELP *******************************/

void help(void)
{
    fprintf(stdout,
            "---------------------- flags for VECTORS -----------------------\n");
    fprintf(stdout,"-a   ADD         add column1 val to column2 val.\n");
    fprintf(stdout,"-d   DIVIDE      divide column1 val by column2 val.\n");
    fprintf(stdout,"-q   QUANTISE    quantise column1 vals onto set of (ascending) vals in column2.\n");
    fprintf(stdout,"-o@  OVERWRITE   overwrite 1st & every @th val in column1 with column2 val.\n");
    fprintf(stdout,"-i@  INSERT      insert column2 vals at start & after every @th item\n");
    fprintf(stdout,"                 of column1.\n");
    fprintf(stdout,"-m   MULTIPLY    multiply column1 val by column2 val.\n");
    fprintf(stdout,"-s   SUBTRACT    subtract column2 val from column1 val.\n");
    fprintf(stdout,"-C   COMPARE     compare vals in column1 & column2.\n");
    fprintf(stdout,"-C@  COMPARE     within error range @.\n");
    fprintf(stdout,"-I   INTERLEAVE  column 2 vals with column 1 vals.\n");
    fprintf(stdout,"-IP x1 x2 x3 x4  Patterned Interleave, between x1 & x2 vals col1,\n");
    fprintf(stdout,"                 then between x3 & x4 vals col2, etc\n");
    fprintf(stdout,"-P   POWER       raise column1 vals (must be >=0) to powers in column2.\n");
    fprintf(stdout,"-Ra  RANDADD     add to column1 val a randval btwn +-val in column2.\n");
    fprintf(stdout,"-Rm  RANDMULT    multiply column1 val by randval between 0 and column2 val.\n");
    fprintf(stdout,"-Rs  RANDSCATTER random scatter (ascending) column1 vals\n");
    fprintf(stdout,"                 by degree of scatter(0-1) in column2.\n");
    fprintf(stdout,"-S@  SUBSTITUTE  substitute any val @ in column1 by next val in column2.\n");
    fflush(stdout);
    exit(1);
}

/********************** DO_OUTFILE *****************************/

int do_outfile(char *argv)
{
    if(*argv=='-')
        return(0);
    if((fp = fopen(argv,"w"))==NULL) {
        if(!sloom && !sloombatch)
            fprintf(stderr,"Cannot open file %s to write.\n",argv);
        else {
            fprintf(stdout,"ERROR: Cannot open file %s to write.\n",argv);
            fflush(stdout);
        }
        usage();
    }
    return(1);
}

/******************************** EXMALLOC ****************************/

char *exmalloc(int n)
{
    char *p;
    if((p = (char *)malloc(n))==NULL) {
        sprintf(errstr,"ERROR: memory allocation failed.\n");
        do_error();
    }
    return(p);
}

/********************************** READ_FLAGS *************************/

void read_flags(int flagstart,int argc,char *argv[])
{
    char dummy;

    int bas = flagstart, k;

    while(flagstart < argc) {
        if(sloom) {
            if(flagstart == bas) {
                if(*argv[flagstart]++!='-') {
                    fprintf(stdout,"ERROR: Cannot interpret program mode\n");
                    fflush(stdout);
                    usage();
                }
                if(sscanf(argv[flagstart]++,"%c",&dummy)!=1) {
                    fprintf(stdout,"ERROR: Cannot interpret program mode\n");
                    fflush(stdout);
                    usage();
                }
            } else {
                if(argc < 8)
                    dummy = 'z';
            }
        } else {
            if(*argv[flagstart]++!='-') {
                fprintf(stdout,"ERROR: Cannot read flag '%s'.\n",--argv[flagstart]);
                fflush(stdout);
                usage();
            }
            if(sscanf(argv[flagstart]++,"%c",&dummy)!=1) {
                fprintf(stdout,"ERROR: Cannot read flag '%s'\n",argv[flagstart]-2);
                fflush(stdout);
                usage();
            }
        }
        switch(dummy) {
        case('C'):
            flag = dummy;
            ro = (int)*argv[flagstart];
            if(ro == ENDOFSTR)
                factor = 0;
            else if(sscanf(argv[flagstart],"%lf",&factor)!=1) {
                fprintf(stdout,"ERROR: Cannot read error range.\n");
                fflush(stdout);
                usage();
            }
            break;
        case('M'):
            flag = dummy;
            ro = (int)*argv[flagstart];
            if((ro == 'o') || (ro == 'O')) {
                argv[flagstart]++;
                dummy = (int)*argv[flagstart];
                if(dummy == ENDOFSTR) {
                    fprintf(stdout,"ERROR: Cannot read start entry of morph.\n");
                    fflush(stdout);
                    usage();
                }
                if(sscanf(argv[flagstart],"%lf",&factor)!=1) {
                    fprintf(stdout,"ERROR: Cannot read start entry of morph.\n");
                    fflush(stdout);
                    usage();
                }
            } else {
                if(ro == ENDOFSTR) {
                    fprintf(stdout,"ERROR: Cannot read item to match.\n");
                    fflush(stdout);
                    usage();
                }
                if(sscanf(argv[flagstart],"%lf",&factor)!=1) {
                    fprintf(stdout,"ERROR: Cannot read item to match.\n");
                    fflush(stdout);
                    usage();
                }
            }
            break;
        case('d'): case('s'): case('m'): case('a'): case('A'):
        case('P'): case('q'): case('b'): case('B'):
            flag = dummy;
            break;
        case('R'):
            flag = dummy;
            ro = (int)*argv[flagstart];
            switch(ro) {
            case('a'): case('m'): case('s'):
                break;
            default:
                if(!sloom) {
                    fprintf(stdout,"ERROR: Unknown flag %s.\n",argv[flagstart]-2);
                    fflush(stdout);
                } else {
                    fprintf(stdout,"ERROR: Unknown program mode.\n");
                    fflush(stdout);
                }
                usage();
            }
            break;
        case('k'):
            flag = dummy;
            ro = (int)*argv[flagstart]++;
            switch(ro) {
            case('k'):      case('d'):
                if(sscanf(argv[flagstart],"%d",&ifactor)!=1) {
                    fprintf(stdout,"ERROR: Cannot read cycle count.\n");
                    fflush(stdout);
                    usage();
                }
                break;
            default:
                fprintf(stdout,"ERROR: Unknown program mode.\n");
                fflush(stdout);
                usage();
            }
            break;
        case('K'):
            flag = dummy;
            ro = (int)*argv[flagstart];
            switch(ro) {
            case('k'):      case('d'):      case('K'):      case('D'):      case('c'):      case('S'):
                break;
            default:
                if(!sloom) {
                    fprintf(stdout,"ERROR: Unknown flag %s.\n",argv[flagstart]-2);
                    fflush(stdout);
                } else {
                    fprintf(stdout,"ERROR: Unknown program mode.\n");
                    fflush(stdout);
                }
                usage();
            }
            break;
        case('l'):
            flag = dummy;
            ro = (int)*argv[flagstart];
            switch(ro) {
            case('a'):      case('b'):      case('s'):
                break;
            default:
                fprintf(stdout,"ERROR: Unknown program mode.\n");
                fflush(stdout);
                usage();
            }
            break;
        case('p'):
            flag = dummy;
            ro = (int)*argv[flagstart]++;
            switch(ro) {
            case('n'):      case('y'):      case('N'):      case('Y'):
                if(sscanf(argv[flagstart],"%lf",&factor)!=1) {
                    fprintf(stdout,"ERROR: Cannot read parameter value.\n");
                    fflush(stdout);
                    usage();
                }
                errorbnd = 0.0;
                break;
            default:
                fprintf(stdout,"ERROR: Unknown program mode.\n");
                fflush(stdout);
                usage();
            }
            break;
        case('S'): case('i'): case('o'): case('c'):
            flag = dummy;
            if(sscanf(argv[flagstart],"%lf",&factor)!=1) {
                if(!sloom) {
                    fprintf(stdout,"Cannot read numerical value with flag 'S'|'i'|'o'|'c'.\n");
                    fflush(stdout);
                } else {
                    fprintf(stdout,"ERROR: Cannot read parameter value.\n");
                    fflush(stdout);
                }
                usage();
            }
            ifactor = round(factor);
            break;
        case('y'):
            if(flagstart == bas) {
                flag = dummy;
                ro = (int)*argv[flagstart];
                switch(ro) {
                case('z'):
                    if((other_params = (double *)malloc(4 * sizeof(double)))==NULL) {
                        fprintf(stdout,"ERROR: Out of memory.\n");
                        fflush(stdout);
                        exit(1);
                    }
                    break;
                default:
                    fprintf(stdout,"ERROR: Unknown program mode.\n");
                    fflush(stdout);
                    usage();
                }
            } else {
                k = flagstart - bas - 1;
                if(sscanf(argv[flagstart],"%lf",other_params+k)!=1) {
                    fprintf(stdout,"ERROR: Cannot read parameter %d\n",k+1);
                    fflush(stdout);
                    usage();
                }
            }
            break;
        case('I'):
            if(flagstart == bas) {
                flag = dummy;
                ro = (int)*argv[flagstart]++;
                switch(ro) {
                case(ENDOFSTR):
                    break;
                case('P'):
                    if((other_params = (double *)malloc(4 * sizeof(double)))==NULL) {
                        fprintf(stdout,"ERROR: Out of memory.\n");
                        fflush(stdout);
                        exit(1);
                    }
                    break;
                default:
                    fprintf(stdout,"ERROR: Unknown program mode.\n");
                    fflush(stdout);
                    usage();
                }
            } else {
                k = flagstart - bas - 1;
                if(sscanf(argv[flagstart],"%lf",other_params+k)!=1) {
                    fprintf(stdout,"ERROR: Cannot read parameter %d\n",k+1);
                    fflush(stdout);
                    usage();
                }
            }
            break;
        case('e'):
            if(!sloom) {
                if(sscanf(argv[flagstart],"%lf",&errorbnd)!=1) {
                    fprintf(stdout,"ERROR: Cannot read error bound with flag 'e'.\n");
                    fflush(stdout);
                    usage();
                }
                if(errorbnd <= 0.0) {
                    fprintf(stdout,"ERROR: Error bound impossible.\n");
                    fflush(stdout);
                    usage();
                }
                break;
            }
            /* fall thro */
        default:
            if(!sloom) {
                fprintf(stdout,"Unknown flag %s.\n",--argv[flagstart]);
                fflush(stdout);
                usage();
            } else {
                if(sscanf(argv[flagstart],"%lf",&other_param)!=1) {
                    fprintf(stdout,"ERROR: Unknown program parameter '%s'.\n",argv[flagstart]);
                    fflush(stdout);
                    usage();
                }
                if(flag == 'S' || flag == 'p')
                    errorbnd = other_param;
                if(flag == 'M')
                    ifactor = round(other_param);
            }
        }
        flagstart++;
    }
    if(flag==0) {
        if(!sloom) {
            fprintf(stdout,"ERROR: No action specified.\n");
            fflush(stdout);
        } else {
            fprintf(stdout,"ERROR: No program mode specified.\n");
            fflush(stdout);
        }
        usage();
    }
}

/************************* DO_INFILE ***************************/

int do_infile(char *argv,double **number)
{
    int ccnt = 0;
    char *p, temp[200];
    FILE *fpi;
    arraysize = BIGARRAY;
    if((fpi = fopen(argv,"r"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open infile %s\n",argv);
        fflush(stdout);
        usage();
    }
    while(fgets(temp,200,fpi)!=NULL) {
        p = temp;
        while(strgetfloat(&p,&((*number)[ccnt]))) {
            if(++ccnt >= arraysize) {
                arraysize += BIGARRAY;
                *number = (double *)exrealloc((char *)*number,arraysize*sizeof(double));
            }
        }
    }
    *number=(double *)exrealloc((char *)*number,ccnt*sizeof(double));
    fclose(fpi);
    return(ccnt);
}

/************************* DO_STRINGFILE ***************************/

int do_stringfile(char *argv,char ***stringptr)
{
    int ccnt = 0;
    char temp[200];
    FILE *fpi;
    arraysize = BIGARRAY;
    if((fpi = fopen(argv,"r"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open infile %s\n",argv);
        fflush(stdout);
        usage();
    }
    while(fgets(temp,200,fpi)!=NULL) {
        if(((*stringptr)[ccnt] = (char *)malloc(strlen(temp)+1)) == NULL) {
            fprintf(stdout,"ERROR: No more memory for strings\n");
            fflush(stdout);
            exit(1);
        }
        strcpy((*stringptr)[ccnt],temp);
        if(++ccnt >= arraysize) {
            arraysize += BIGARRAY;
            if((*stringptr = (char **)exrealloc((char *)*stringptr,arraysize*sizeof(char *)))== NULL) {
                fprintf(stdout,"ERROR: No more memory for strings\n");
                fflush(stdout);
                exit(1);
            }
        }
    }
    fclose(fpi);
    return(ccnt);
}

/**************************** FLTEQ *******************************/

int flteq(double f1,double f2,double errorbnd)
{
    double upperbnd, lowerbnd;
    upperbnd = f2 + errorbnd;
    lowerbnd = f2 - errorbnd;
    if((f1>upperbnd) || (f1<lowerbnd))
        return(0);
    return(1);
}

/******************************** LOGO **********************************/

void logo(void)
{
    printf("\t    ***************************************************\n");
    printf("\t    *           COMPOSERS DESKTOP PROJECT             *\n");
    printf("\t                   %s $Revision: 1.1 $\n",PROG);
    printf("\t    *                                                 *\n");
    printf("\t    *     Manipulate 2 columns of numbers (vectors)   *\n");
    printf("\t    *                                                 *\n");
    printf("\t    *                by TREVOR WISHART                *\n");
    printf("\t    ***************************************************\n\n");
}

/***************************** EXREALLOC *****************************/

char *exrealloc(char *p,int k)
{
    char *q;
    if((q = malloc(k))==NULL) {
        sprintf(errstr,"ERROR: reallocation of memory failed.\n");
        do_error();
    }
    memcpy(q,p,k);
    /*
      if((q = realloc(p,k))==NULL) {
      sprintf(errstr,"ERROR: reallocation of memory failed.\n");
      do_error();
      }
    */
    return(q);
}

/******************************** DO_ADD ****************************/

void do_add(void)
{
    int n;
    for(n=0;n<cnt;n++)
        do_valout(number1[n]+number2[n]);
}

/******************************** DO_MEAN ****************************/

void do_mean(void)
{
    int n;
    for(n=0;n<cnt;n++)
        do_valout((number1[n]+number2[n])/2.0);
}

/******************************** DO_MAX ****************************/

void do_max(void)
{
    int n;
    for(n=0;n<cnt;n++)
        do_valout(max(number1[n],number2[n]));
}

/******************************** DO_MIN ****************************/

void do_min(void)
{
    int n;
    for(n=0;n<cnt;n++)
        do_valout(min(number1[n],number2[n]));
}

/******************************** DO_SUBTRACT ****************************/

void do_subtract(void)
{
    int n;
    for(n=0;n<cnt;n++)
        do_valout(number1[n]-number2[n]);
}

/******************************** DO_MULTIPLY ****************************/

void do_multiply(void)
{
    int n;
    for(n=0;n<cnt;n++)
        do_valout(number1[n]*number2[n]);
}

/******************************** DO_DIVIDE ****************************/

void do_divide(void)
{
    int n;
    for(n=0;n<cnt;n++) {
        if(flteq(number2[n],0.0,FLTERR)) {
            if(!sloom) {
                fprintf(fp,"INF\n");
                fflush(stdout);
            } else {
                sprintf(errstr,"Division by zero encountered at item %d in 2nd column: Impossible\n",n+1);
                do_error();
            }
        } else
            do_valout(number1[n]/number2[n]);
    }
}

/******************************** DO_OVERWRITE ****************************/

void do_overwrite(void)
{
    int n, m = -1, k = 1;
    for(n=0;n<cnt;n++) {
        if(n%ifactor)
            do_valout(number1[n]);
        else {
            if(k) {
                if(++m >= cnt2) {
                    fprintf(stdout,"WARNING: Out of values in column2\n");
                    fflush(stdout);
                    k = 0;
                } else
                    do_valout(number2[m]);
            }
        }
    }
}

/******************************** DO_INSERT ****************************/

void do_insert(void)
{
    int n, m = -1, k = 1;
    for(n=0;n<cnt;n++) {
        if(k && !(n%ifactor)) {
            if(++m >= cnt2) {
                fprintf(stdout,"WARNING: Out of values in column2\n");
                fflush(stdout);
                k = 0;
            } else
                do_valout(number2[m]);
        }
        do_valout(number1[n]);
    }
}

/******************************** DO_INTERLEAVE ****************************/

void do_interleave(void)
{
    int n;
    for(n=0;n<cnt;n++) {
        if(!sloom) {
            fprintf(fp,"%lf\n",number1[n]);
            fprintf(fp,"%lf\n",number2[n]);
            fflush(stdout);
        } else {
            fprintf(stdout,"INFO: %lf\n",number1[n]);
            fprintf(stdout,"INFO: %lf\n",number2[n]);
        }
    }
}

/******************************** DO_INTERLEAVE ****************************/

void do_patterned_interleave(void)
{
    int n, outcnt[2], incnt[2], range[2], lims[4], which = 0, thiscnt;
    char **strs[2];
    strs[0] = string0;
    strs[1] = string1;
    incnt[0] = cnt;
    incnt[1] = cnt2;
    outcnt[0] = 0;
    outcnt[1] = 0;
    for(n=0;n<4;n++) {
        lims[n] = round(other_params[n]);
        if(lims[n] < 0) {
            fprintf(stdout,"ERROR: Parameter %d is less than zero: Cannot proceed\n",n+1);
            fflush(stdout);
            exit(1);
        }
    }
    n = lims[2];            /* change order of params from 01,23 -> 02,13 */
    lims[2] = lims[1];      /* so bottom of ranges are 0 and 1 */
    lims[1] = n;

    range[0] = lims[2] - lims[0];   /* drand over range 0 2, followed by floor, gives ... */
    range[1] = lims[3] - lims[1];   /* nums in range 0to0.999 -> 0, nums in range 1to1.999 ->1, and val 2.0 only -> 2 */
    /* so vanishingly small probability of generating 2.0 */
    range[0]++;                                             /* adding 1 to range, makes equally probable 0,1,or2 will be generated */
    range[1]++;                                             /* with vanishingly small probability of generating 3.0 (outside range) */
    for(;;) {
        thiscnt = (int)(floor(drand48() * range[which]));
        if(thiscnt > range[which])      /* safety only : vanishingly small probability of generating number outside range */
            thiscnt--;
        thiscnt +=  lims[which];
        for(n = 0;n < thiscnt; n++) {
            if(outcnt[which] >= incnt[which])
                return;
            if(!sloom)
                fprintf(fp,"%s\n",strs[which][outcnt[which]++]);
            else
                fprintf(fp,"INFO: %s\n",strs[which][outcnt[which]++]);
            fflush(stdout);
        }
        which = !which;
    }
}

/******************************** DO_POWER ****************************/

void do_power(void)
{
    int n, is_neg;
    double output;
    for(n=0;n<cnt;n++) {
        is_neg = 0;
        if(number1[n]<0.0) {
            number1[n] = -number1[n];
            is_neg = 1;
        }
        output = pow(number1[n],number2[n]);
        if(is_neg)
            output = -output;
        do_valout(output);
    }
}

/******************************** DO_RANDADD ****************************/

void do_randadd(void)
{
    int n;
    double sum;
    for(n=0;n<cnt;n++) {
        sum =  ((drand48() * 2.0) - 1.0) * number2[n];
        do_valout(number1[n] + sum);
    }
}

/******************************** DO_RANDMULT ****************************/

void do_randmult(void)
{
    int n;
    double sum;
    for(n=0;n<cnt;n++) {
        sum = drand48() * number2[n];
        do_valout(number1[n] * sum);
    }
}

/******************************** DO_RANDSCAT ****************************/

void do_randscat(void)
{
    int n;
    double *diffs, scatter;
    if(number2[0]<0.0 || number2[0]>1.0) {
        sprintf(errstr,"1st nuber in 2nd column (%lf) is out of range for this option.\n",number2[0]);
        do_error();
    }
    diffs = (double *)exmalloc((cnt-1)*sizeof(double));
    for(n=0;n<cnt-1;n++) {
        diffs[n] = number1[n+1] - number1[n];
        if(number2[n]<0.0 || number2[n]>1.0) {
            sprintf(errstr,"COLUMN2[%d] = %lf is out of range for this option.\n",n+1,number2[n]);
            do_error();
        }
        diffs[n] /= 2.0;
    }
    for(n=1;n<cnt-1;n++) {
        scatter = ((drand48() * 2.0) - 1.0) * number2[n-1];
        if(scatter > 0.0)
            number1[n] += diffs[n] * scatter;
        else
            number1[n] += diffs[n-1] * scatter;
    }
    for(n=0;n<cnt;n++)
        do_valout(number1[n]);
}


/******************************** DO_SUBSTITUTE ****************************/

void do_substitute(void)
{
    int n, m = 0;
    for(n=0;n<cnt;n++) {
        if(flteq(number1[n],factor,errorbnd)) {
            number1[n] = number2[m];
            if(++m >= cnt2) {
                fprintf(stdout,"WARNING: out of numbers in column2\n");
                fflush(stdout);
                break;
            }
        }
    }
    for(n=0;n<cnt;n++)
        do_valout(number1[n]);
}

/******************************** DO_QUANTISE ****************************/

void do_quantise(void)
{
    int n, m;

    quantise_sort(number2,&cnt2);
    for(n=0;n<cnt;n++) {
        m = 0;
        while(number1[n] > number2[m]) {
            if(++m>=cnt2)
                break;
        }
        if(m==0)        { number1[n] = number2[0];       continue; }
        if(m==cnt2) { number1[n] = number2[cnt2-1]; continue; }
        if(number2[m]-number1[n]<number1[n]-number2[m-1])
            number1[n] = number2[m];
        else
            number1[n] = number2[m-1];
    }
    for(n=0;n<cnt;n++)
        do_valout(number1[n]);
}

/*************************** TRUNCATE_VECTORS ************************/

void truncate_vectors(void)
{
    int cntdiff;
    if(flag!='S' && flag!='q' && flag!='i'&& flag!='o') {
        if((cntdiff = cnt2 - cnt)>0) {
            if(!sloom && !sloombatch)
                fprintf(stdout,"column2 > column1 : truncating column2.\n");
            else
                fprintf(stdout,"WARNING: column2 > column1 : truncating column2.\n");
            fflush(stdout);
        } else if(cntdiff<0)  {
            if(!sloom && !sloombatch)
                fprintf(stdout,"column1 > column2 : truncating column1.\n");
            else
                fprintf(stdout,"WARNING: column1 > column2 : truncating column1.\n");
            fflush(stdout);
            cnt = cnt2;
        }
    }
}

/*********************** QUANTISE_SORT *******************************
 *
 * Sort set of doubles into ascending order.
 */

void quantise_sort(double *scti,int *scatcnt)
{
    double sct;
    int n, m;
    for(n=0;n<((*scatcnt)-1);n++) {
        for(m=n+1;m<(*scatcnt);m++) {
            if(*(scti+m)<*(scti+n)) {
                sct       = *(scti+m);
                *(scti+m) = *(scti+n);
                *(scti+n) = sct;
            }
        }
    }
    for(n=0;n<((*scatcnt)-1);n++) {
        if(flteq(*(scti+n+1),*(scti+n),FLTERR)) {
            for(m = n; m < ((*scatcnt)-1); m++)
                *(scti+m) = *(scti+m+1);
            (*scatcnt)--;
            n--;
        }
    }
}

/******************************** CONJOINA ****************************/

void conjoina(void)
{
    int n;
    for(n=0;n<cnt;n++)
        fprintf(stdout,"INFO: %lf\n",number1[n]);
    for(n=0;n<cnt2;n++)
        fprintf(stdout,"INFO: %lf\n",number2[n]);
}

/******************************** CONJOINB ****************************/

void conjoinb(void)
{
    int n;
    for(n=0;n<cnt2;n++)
        fprintf(stdout,"INFO: %lf\n",number2[n]);
    for(n=0;n<cnt;n++)
        fprintf(stdout,"INFO: %lf\n",number1[n]);
}

/******************************** CONJOIN_SORT ****************************/

void conjoin_sort(void)
{
    int n = 0,m = 0, done = 0;
    while(!done) {
        if(number1[n] <= number2[m]) {
            fprintf(stdout,"INFO: %lf\n",number1[n]);
            if(++n >= cnt)
                done = 1;
        } else {
            fprintf(stdout,"INFO: %lf\n",number2[m]);
            if(++m >= cnt2)
                done = 1;
        }
    }                               /* Either 1 list is unfinished, or neither list is unfinished */
    /* so print elements in whichever list is still unfinished */
    while(n < cnt) {
        fprintf(stdout,"INFO: %lf\n",number1[n]);
        n++;
    }
    while(m < cnt2) {
        fprintf(stdout,"INFO: %lf\n",number2[m]);
        m++;
    }
    fflush(stdout);
}

/******************************** DO_KEEP ****************************/

void do_keep(void)
{
    int n, i, warned = 0;
    for(n=0;n<cnt2;n++) {
        i = (int)(round(number2[n]));
        if(i < 1) {
            fprintf(stdout,"ERROR: There is no such position as %lf (%d)\n",number2[n],i);
            fflush(stdout);
            exit(1);
        }
        i--;
        if(i < cnt)
            fprintf(stdout,"INFO: %lf\n",number1[i]);
        else if(!warned) {
            fprintf(stdout,"WARNING: There is no such position as %lf (%d)\n",number2[n],i);
            fflush(stdout);
            warned = 1;
        }
    }
    fflush(stdout);
}

/******************************** DO_DEL ****************************/

void do_del(void)
{
    int n, m, elimcnt, i, j, *delpos, warned = 0;
    if((delpos = (int *)malloc(cnt2 * sizeof(int)))==NULL) {
        fprintf(stdout,"ERROR: Out of memory.\n");
        fflush(stdout);
        exit(1);
    }
    elimcnt = 0;
    for(n=0;n<cnt2;n++) {
        i = (int)(round(number2[n]));
        if(i < 1) {
            fprintf(stdout,"ERROR: There is no such position as %lf (%d)\n",number2[n],i);
            fflush(stdout);
            exit(1);
        }
        i--;
        if(i < cnt)
            delpos[elimcnt++] = i;
        else if(!warned) {
            fprintf(stdout,"WARNING: There is no such position as %lf (%d)\n",number2[n],i);
            fflush(stdout);
            warned = 1;
        }
    }
    for(n=0;n<elimcnt-1;n++) {                                              /* eliminate duplicates */
        for(m=n+1;m<elimcnt;m++) {
            if(delpos[m] == delpos[n]) {    /* if values are equal */
                i = m;
                j = m+1;
                while(j < elimcnt) {            /* If there are vals beyond here, shuffle down to eliminate delpos[m] */
                    delpos[i] = delpos[j];
                    i++;
                    j++;
                }
                elimcnt--;                                      /* total number of items reduced by 1 */
                m--;                                            /* comparison cnt stays where it is */
            }
        }
    }
    for(n=0;n<elimcnt-1;n++) {                                              /* sort into ascending order */
        for(m=n+1;m<elimcnt;m++) {
            if(delpos[m] < delpos[n]) {
                i                 = delpos[m];
                delpos[m] = delpos[n];
                delpos[n] = i;
            }
        }
    }
    i = 0;
    for(n=0;n<cnt;n++) {                                    /* print all values not listed for elimination */
        if(i < elimcnt) {
            if(n != delpos[i])
                fprintf(stdout,"INFO: %lf\n",number1[n]);
            else
                i++;
        } else
            fprintf(stdout,"INFO: %lf\n",number1[n]);
    }
    fflush(stdout);
}

/******************************** COMPARE ****************************/

void compare(void)
{
    int n;
    double upbnd, lobnd;
    if(factor == 0) {
        for(n = 0;n<cnt;n++) {
            if(number1[n] != number2[n]) {
                if(!sloom && !sloombatch)
                    fprintf(stdout,"Columns differ at item %d.\n",n+1);
                else
                    fprintf(stdout,"WARNING: Columns differ at item %d.\n",n+1);
                fflush(stdout);
                exit(1);
            }
        }
        if(!sloom && !sloombatch)
            fprintf(stdout,"Columns are the same.\n");
        else
            fprintf(stdout,"WARNING: Columns are the same.\n");
    } else {
        for(n = 0;n<cnt;n++) {
            upbnd = number1[n] + factor;
            lobnd = number1[n] - factor;
            if(number2[n] > upbnd || number2[n] < lobnd) {
                if(!sloom && !sloombatch)
                    fprintf(stdout,"Columns differ by more than permitted error at item %d.\n",n+1);
                else
                    fprintf(stdout,"WARNING: Columns differ by more than permitted error at item %d.\n",n+1);
                fflush(stdout);
                exit(1);
            }
        }
        if(!sloom && !sloombatch)
            fprintf(stdout,"Columns are equivalent within error range %lf\n",factor);
        else
            fprintf(stdout,"WARNING: Columns are equivalent within error range %lf\n",factor);
    }
    fflush(stdout);
}

/******************************** DEL_COPYS ****************************/

void del_copys(void)
{
    int n, m, dupl;
    for(n=0;n<cnt;n++) {
        dupl = 0;
        for(m=0;m<cnt2;m++) {
            if(flteq(number1[n],number2[m],FLTERR)) {
                dupl = 1;
                break;
            }
        }
        if(!dupl)
            fprintf(stdout,"INFO: %lf\n",number1[n]);
    }
    fflush(stdout);
}

/******************************** INSERT_NON_DUPLS ****************************/

void insert_non_dupls(void)
{
    int n, m, k, dupl;
    double temp;
    if((number1 = (double *)realloc((char *)number1,(cnt + cnt2) * sizeof(double))) == NULL) {
        fprintf(stdout,"ERROR: Out of memory.\n");
        fflush(stdout);
        exit(1);
    }
    k = cnt;
    for(m=0;m<cnt2;m++) {
        dupl = 0;
        for(n=0;n<cnt;n++) {
            if(flteq(number1[n],number2[m],FLTERR)) {
                dupl = 1;
                break;
            }
        }
        if(!dupl)
            number1[k++] = number2[m];
    }
    cnt = k;
    for(n=0;n<cnt-1;n++) {
        for(m = n; m<cnt; m++) {
            if(number1[m] < number1[n]) {
                temp = number1[n];
                number1[n] = number1[m];
                number1[m] = temp;
            }
        }
    }
    for(n=0;n<cnt;n++)
        fprintf(stdout,"INFO: %lf\n",number1[n]);
    fflush(stdout);
}

/******************************** INSERT_NON_DUPLS ****************************/

void partition_col(int atpos,int partition_incol)
{
    double *numbera, *numberb;
    int n;
    if(partition_incol) {
        numbera = number2;
        numberb = number1;
    } else {
        numbera = number1;
        numberb = number2;
    }
    if(atpos) {
        for(n = 0;n < cnt;n++) {
            if(flteq(numbera[n],factor,errorbnd))
                fprintf(stdout,"INFO: %lf\n",numberb[n]);
        }
    } else {
        for(n = 0;n < cnt;n++) {
            if(!flteq(numbera[n],factor,errorbnd))
                fprintf(stdout,"INFO: %lf\n",numberb[n]);
        }
    }
    fflush(stdout);
}

/******************************** DO_KEEP_IN_SPANS ****************************/

void do_keep_in_spans(int is_del)
{
    int n,m;
    double lo, hi;
    if((cnt2<2) || (cnt2&1)) {
        fprintf(stdout,"ERROR: Values in column two are not paired correctly.\n");
        fflush(stdout);
        exit(1);
    }
    if(!cnt) {
        fprintf(stdout,"ERROR: No Values in column one.\n");
        fflush(stdout);
        exit(1);
    }
    lo = number1[0];
    for(n=1;n<cnt;n++) {
        if(number1[n] < lo) {
            fprintf(stdout,"ERROR: Values in column one are not in ascending order.\n");
            fflush(stdout);
            exit(1);
        }
        lo = number1[n];
    }
    lo = number2[0];
    for(n=1;n<cnt2;n++) {
        if(number2[n] < lo) {
            fprintf(stdout,"ERROR: Values in column two are not in ascending order.\n");
            fflush(stdout);
            exit(1);
        }
        lo = number2[n];
    }
    n = 0;
    m = 0;
    lo = number2[m++];
    hi = number2[m++];
    switch(is_del) {
    case(0):
        do {
            if(number1[n] < lo)                                                             /* If below value pair range, ignore */
                ;
            else if(number1[n] <= hi)                                               /* If within value pair range, print */
                fprintf(stdout,"INFO: %lf\n",number1[n]);
            else if(m >= cnt2)                                                              /* if above value pair range */
                break;                                                                          /* if there are no more pairs, finished */
            else {
                lo = number2[m++];                                                      /* else, get next pair */
                hi = number2[m++];
            }
            n++;
        } while(n < cnt);
        break;
    case(1):
        do {
            if(number1[n] < lo)                                                             /* If below value pair range, print */
                fprintf(stdout,"INFO: %lf\n",number1[n]);
            else if(number1[n] <= hi)                                               /* If within value pair range, ignore */
                ;
            else if(m >= cnt2)                                                              /* if above value pair range */
                break;                                                                          /* if there are no more pairs, finished with pairs */
            else {
                lo = number2[m++];                                                      /* else, get next pair */
                hi = number2[m++];
                n--;                                                                            /* but stay where we are in input numbers */
            }
            n++;
        } while(n < cnt);

        while(n < cnt)                                                                          /* If any vals above last pair, print */
            fprintf(stdout,"INFO: %lf\n",number1[n++]);
        break;
    }
    fflush(stdout);
}

/******************************** DO_KEEP_CYCLES ****************************/

void do_keep_cycles(int do_keep)
{
    int n, j, m, k;
    int OK = 0, diff, *pos;

    pos = (int *)exmalloc(cnt2 * sizeof(int));
    for(n=0;n<cnt2;n++) {
        if((pos[n] = (int)round(number2[n])) < 1) {
            fprintf(stdout,"ERROR: List positions below 1 do not exist.\n");
            fflush(stdout);
            exit(1);
        }
        if(pos[n] > ifactor) {
            fprintf(stdout,"ERROR: Position %d is outside the cycle length (%d).\n",pos[n],ifactor);
            fflush(stdout);
            exit(1);
        }
        if(n > 0) {
            if((diff = pos[n] - pos[n-1]) < 1) {
                fprintf(stdout,"ERROR: Positions must be in ascending order.\n");
                fflush(stdout);
                exit(1);
            }
            else if(!OK && (diff > 1))              /* if no previous step is greater than 1 */
                OK = 1;                                         /* if this step is > 1, set as OK */
        }
    }
    if(!OK && (pos[0] == 1) && (pos[n-1] == ifactor)) {
        if(do_keep)
            fprintf(stdout,"ERROR: All items will be retained.\n");
        else
            fprintf(stdout,"ERROR: All items will be deleted.\n");
        fflush(stdout);
        exit(1);
    }
    for(n=0;n<cnt2;n++)
        pos[n]--;

    if(do_keep) {
        for(n=0;n<cnt;n+=ifactor) {                     /* KEEP */
            j = 0;
            for(m=n+pos[0],k=pos[0];k<ifactor;k++,m++) {
                if(m >= cnt)
                    break;
                if(k == pos[j]) {
                    fprintf(stdout,"INFO: %lf\n",number1[m]);
                    if(++j >= cnt2)
                        break;
                }
            }
        }
    } else {
        for(n=0;n<cnt;n+=ifactor) {                     /* DELETE */
            j = 0;
            for(m=n,k=0;k<ifactor;k++,m++) {
                if(m >= cnt)
                    break;
                if(j < cnt2) {
                    if(k != pos[j])
                        fprintf(stdout,"INFO: %lf\n",number1[m]);
                    else
                        j++;
                } else {
                    fprintf(stdout,"INFO: %lf\n",number1[m]);
                }
            }
        }
    }
    fflush(stdout);
}

/******************************** DO_MATCHES ****************************/

void do_matches(void) {

    int n, m, k;
    for(n=0;n<cnt;n++) {
        if(flteq(number1[n],factor,FLTERR)) {
            for(k=0,m=n;k<ifactor;m++,k++)
                fprintf(stdout,"INFO: %lf\n",number2[m]);
        }
    }
    fflush(stdout);
}

/******************************** DO_ERROR ****************************/

void do_error(void) {
    if(!sloom && !sloombatch)
        fprintf(stderr,"%s\n",errstr);
    else {
        fprintf(stdout,"ERROR: %s\n",errstr);
        fflush(stdout);
    }
    exit(1);
}

/******************************** DO_VALOUT ****************************/

void do_valout(double val) {
    if(!sloom && !sloombatch)
        fprintf(fp,"%lf\n",val);
    else
        fprintf(stdout,"INFO: %lf\n",val);
}

/******************************** DO_SQUEEZED_PAN ****************************/

void do_squeezed_pan(void) {
    int n, neg = 1;
    double time_shoulder = other_params[0];
    double start_panpos  = other_params[1];
    double end_panpos    = other_params[2];
    double end_pantime   = other_params[3];
    double lastime;
    double pan_subedj;

    if(time_shoulder < 0.0) {
        fprintf(stdout,"ERROR: half lingertime (%lf) cannot be less than zero.\n",time_shoulder);
        fflush(stdout);
        exit(1);
    }
    if(number1[0] < 0.0) {
        fprintf(stdout,"ERROR: First time value less than zero encountered.\n");
        fflush(stdout);
        exit(1);
    } else if(number1[0] <= time_shoulder) {
        fprintf(stdout,"ERROR: First time (%lf) is too close to zero for half-lingertime %lf\n",number1[0],time_shoulder);
        fflush(stdout);
        exit(1);
    }
    lastime = number1[0];
    for(n = 1; n < cnt; n++) {
        if(number1[n] <= lastime + (2 * time_shoulder)) {
            fprintf(stdout,"ERROR: Too small time step encountered (between %lf and %lf).\n",number1[n],lastime);
            fflush(stdout);
            exit(1);
        }
    }
    if(end_pantime - time_shoulder <= number1[cnt-1]) {
        fprintf(stdout,"ERROR: Too small time step encountered before end pantime (%lf to %lf).\n",number1[cnt -1],end_pantime);
        fflush(stdout);
        exit(1);
    }
    if(flteq(number1[0],0.0,FLTERR))
        fprintf(stdout,"INFO: 0.0  %lf\n",number2[0]);
    else {
        pan_subedj = number2[0] * (7.5/9.0);
        fprintf(stdout,"INFO: 0.0  %lf\n",start_panpos);
        fprintf(stdout,"INFO: %lf  %lf\n",number1[0] - time_shoulder,pan_subedj);
        fprintf(stdout,"INFO: %lf  %lf\n",number1[0],number2[0]);
        fprintf(stdout,"INFO: %lf  %lf\n",number1[0] + time_shoulder,pan_subedj);
    }
    if(number2[0] > 0.0)
        neg = -1;
    for(n = 1; n < cnt; n++) {
        number2[n] *= neg;
        pan_subedj = number2[n] * (7.5/9.0);
        fprintf(stdout,"INFO: %lf  %lf\n",number1[n] - time_shoulder,pan_subedj);
        fprintf(stdout,"INFO: %lf  %lf\n",number1[n],number2[n]);
        fprintf(stdout,"INFO: %lf  %lf\n",number1[n] + time_shoulder,pan_subedj);
        lastime = number1[n];
        neg = -neg;
    }
    fprintf(stdout,"INFO: %lf  %lf\n",end_pantime,end_panpos);
    fflush(stdout);
}

/************************************** GENERAL *****************************************/

#ifdef NOTDEF

void
initrand48()
{
    srand(time((time_t *)0));
}

double
drand48()
{
    return (double)rand()/(double)RAND_MAX;
}
#endif
/******************************** DO_MORPHSEQ ****************************/

void do_morphseq(void) {

    int n = 0, morphcnt;
    int minlen = min(cnt,cnt2);
    int startmorph = (int)round(factor) - 1;
    int endmorph = ifactor - 1;
    int morphlen = endmorph - startmorph + 1;
    double last1val, last2val, outval, diff1, diff2, trudiff;
    while(n <= startmorph) {
        fprintf(stdout,"INFO: %lf\n",number1[n]);
        n++;
    }
    n--;
    last1val = number1[n];
    last2val = number2[n];
    outval   = number1[n];
    n++;
    morphcnt = 1;
    while(n <= endmorph) {
        diff1 = number1[n] - last1val;
        diff2 = number2[n] - last2val;
        last1val = number1[n];
        last2val = number2[n];
        trudiff = ((diff2 - diff1) * ((double)morphcnt/(double)morphlen)) + diff1;
        outval += trudiff;
        fprintf(stdout,"INFO: %lf\n",outval);
        morphcnt++;
        n++;
    }
    while(n < minlen) {
        diff2 = number2[n] - last2val;
        last2val = number2[n];
        outval += diff2;
        fprintf(stdout,"INFO: %lf\n",outval);
        n++;
    }
    fflush(stdout);
}

/******************************** DO_MORPH ****************************/

void do_morph(void) {

    int n = 0, morphcnt;
    int minlen = min(cnt,cnt2);
    int startmorph = (int)round(factor) - 1;
    int endmorph = ifactor - 1;
    int morphlen = endmorph - startmorph + 1;
    while(n <= startmorph) {
        fprintf(stdout,"INFO: %lf\n",number1[n]);
        n++;
    }
    morphcnt = 1;
    while(n < endmorph) {
        fprintf(stdout,"INFO: %lf\n",((number2[n] - number1[n]) * ((double)morphcnt/(double)morphlen)) + number1[n]);
        morphcnt++;
        n++;
    }
    while(n < minlen) {
        fprintf(stdout,"INFO: %lf\n",number2[n]);
        n++;
    }
    fflush(stdout);
}
