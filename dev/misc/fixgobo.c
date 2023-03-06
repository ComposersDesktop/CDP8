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
#include <string.h>
#include <ctype.h>
#include <processno.h>
#include <standalone.h>

#define NUMBER_OF_GOBOS (41)
#define ENDOFSTR    ('\0')

#define MAX_TEMPROC_AREA    (288)
#define MIN_TEMPROC_AREA    (287)

static int check_flags(char *str,int **flags,int max_gobos);
static int check_progno(int *progno, int system_update);
static int make_next_gobo_name_and_number(int *gobo_number, int *goboname_len, char **comp, char *pretext, int max_gobos);
static int count_gobo_array_elements_in_first_gobo(char *gobo_line, char *comp, int progno, int new_element_cnt);
static int altergobo(char **gobo_line,int existing_element_cnt,int make_new_array_element,int gobo_array_element,
            int gobo_element_mask,int substitute,int setval);
static int count_gobo_array_elements(char *gobo_line);

//TW UPDATE
int deleter = 0;
int substitute = 0;
char pretext[] = "int ";    /* text occuring before goboname, on gobo defining lines in gobo.c */
const char* cdp_version = "7.1.0";

int main(int argc, char *argv[])
{
    FILE *fpi, *fpo;
    char *gobo_line, *comp;
//TW UPDATE
    int gobo_number = 0, goboname_len = 10, setval = 0, *flags;
    int max_gobos = NUMBER_OF_GOBOS + 2;
    int finished_fixing_gobos = 0, progno;
    int gobo_array_element, gobo_array_element_index, gobo_element_mask, first_gobostring = 1;
    int existing_element_cnt = 0, new_element_cnt, make_new_array_element = 0;
    int system_update = 0;
        
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if(argc==4) {
        if(strcmp(argv[3],"SYSTEM_UPDATE")) {
            fprintf(stdout,"ERROR: Bad call to fixgobo.\n");
            fflush(stdout);
            exit(1);
        } else {
            system_update = 1;
        }
    } else if(argc!=3) {
        fprintf(stdout,"ERROR: Bad call to fixgobo.\n");
        fflush(stdout);
        exit(1);
    }
    if(sscanf(argv[2],"%d",&progno)!=1) {
        fprintf(stdout,"ERROR: Cannot read process number.\n");
        fflush(stdout);
        exit(1);
    }
    if((substitute = check_progno(&progno,system_update)) < 0)
        exit(1);
    if(check_flags(argv[1],&flags,max_gobos) < 0)
        exit(1);
    progno--;                   /* progs count from 1, gob flags indexed from 0 */
    gobo_array_element       = progno/32;
    gobo_array_element_index = progno%32;
    progno++;
    gobo_element_mask = 1 << gobo_array_element_index;
    if((fpi = fopen("gobo.c","r"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open the existing gobo file, to update it.\n");
        fflush(stdout);
        exit(1);
    }
    if((fpo = fopen("temp.c","w"))==NULL) {
        fprintf(stdout,"ERROR: Cannot open temporary file 'temp.c', to write revised gobo.\n");
        fclose(fpi);
        fflush(stdout);
        exit(1);
    }
    if((gobo_line = malloc(512 * sizeof(char)))==NULL) {
        fprintf(stdout,"ERROR: Out of memory to create gobo line store, in fixgobo.\n");
        fflush(stdout);
        exit(1);
    }
    if((comp = malloc(24 * sizeof(char)))==NULL) {
        fprintf(stdout,"ERROR: Out of memory to create gobo name comaparator, in fixgobo.\n");
        fflush(stdout);
        exit(1);
    }
    new_element_cnt = ((MAX_PROCESS_NO - 1)/32) + 1;
    make_next_gobo_name_and_number(&gobo_number,&goboname_len,&comp,pretext,max_gobos);
    setval = flags[gobo_number];
    while(fgets(gobo_line,200,fpi)!=NULL) {
        if(!finished_fixing_gobos && !strncmp(gobo_line,comp,goboname_len)) {
            if(first_gobostring) {
                existing_element_cnt = count_gobo_array_elements_in_first_gobo(gobo_line,comp,progno,new_element_cnt);
                if(existing_element_cnt <= 0)
                    exit(1);
                if(new_element_cnt > existing_element_cnt)
                    make_new_array_element = 1;
                first_gobostring = 0;
            } else if(count_gobo_array_elements(gobo_line) != existing_element_cnt) {
                fprintf(stdout, "ERROR: Anomalous count of array members in gobo %s.\n",comp);
                fflush(stdout);
                exit(1);
            }
            if(!altergobo
            (&gobo_line,existing_element_cnt,make_new_array_element,gobo_array_element,gobo_element_mask,substitute,setval)) {
                exit(1);
            }
            if(fputs(gobo_line,fpo)==EOF) {
                fprintf(stdout,"ERROR: Failed to write new line of gobo data to the new temporary file.\n");
                fflush(stdout);
                exit(1);
            }
            finished_fixing_gobos = make_next_gobo_name_and_number(&gobo_number,&goboname_len,&comp,pretext,max_gobos);
            setval = flags[gobo_number];
        } else {
            if(fputs(gobo_line,fpo)==EOF) {
                fprintf(stdout,"ERROR: Failed to write original line of gobo data to the new temporary file.\n");
                fflush(stdout);
                exit(1);
            }
        }
    }
    fclose(fpi);
    fclose(fpo);
    fprintf(stdout,"END: \n");
    fflush(stdout);
    return 0;
}

/*********************************** COUNT_GOBO_ARRAY_ELEMENTS ******************************/

int count_gobo_array_elements(char *gobo_line)
{
    char *p;
    int existing_element_cnt = 0, OK = 0;   

    p = gobo_line;
    while(*p != '{') {              /* search for start of array */
        p++;
        if(*p==ENDOFSTR) {
            fprintf(stdout,"ERROR: Failed to find gobo array in string.\n");
            fflush(stdout);
            return(0);
        }
    }       
    p++;
    while(isspace(*p))
        p++;                        /* check for inappropriate end of array */
    if(*p=='}') {
        fprintf(stdout,"ERROR: First gobo array is empty.\n");
        fflush(stdout);
        return(0);
    }
    while(*p != '}') {              /* until array end is found */
        if(isdigit(*p))
            OK = 1;                 /* check that each array element contains numeric data */
        if(*p == ',') {             /* count array elements, by counting separating commas */
            existing_element_cnt++;
            if(!OK) {
                fprintf(stdout,"ERROR: First-gobo array-element %d is missing.\n",existing_element_cnt);
                fflush(stdout);
                return(0);
            }
            OK = 0;
        }
        p++;
        if(*p==ENDOFSTR) {
            fprintf(stdout,"ERROR: Failed to find gobo element in array, in string.\n");
            fflush(stdout);
            return(0);
        }
    }
    existing_element_cnt++;         /* count last array elements */
    if(!OK) {                       /* and check that it contains numeric data */
        fprintf(stdout,"ERROR: First-gobo, last array-element is missing.\n");
        fflush(stdout);
        return(0);
    }
    return(existing_element_cnt);
}

/*********************************** ALTERGOBO ******************************/

int altergobo
(char **gobo_line,int existing_element_cnt,int make_new_array_element,
int gobo_array_element,int gobo_element_mask,int substitute, int setval)
{
    char *p, *gobo_pointer, gobo_string[24], end_part[512];
    int goboval, n, zero_mask;

    p = *gobo_line;
    while(*p != '{') {              /* search for start of array */
        p++;
        if(*p==ENDOFSTR) {
            fprintf(stdout,"ERROR: Failed to find gobo array in string.\n");
            fflush(stdout);
            return(0);
        }
    }                               /* CREATING NEW ARRAY ELEMENT */
    if(make_new_array_element) {
        while(*p != '}') {          /* search for end of array */
            p++;
            if(*p==ENDOFSTR) {
                fprintf(stdout,"ERROR: Failed to find gobo array in string.\n");
                fflush(stdout);
                return(0);
            }
        }
        strcpy(end_part,p);         /* store characters at end of array */
        *p++ = ',';                 /* enter new separating comma */
        *p = ENDOFSTR;              /* cut array at this point */

        goboval = 0;                /* PRESET NEW ARRAY ELEMENT TO 0 */
//TW UPDATE
        if(!deleter && (setval==1)) /* put in value 1, only if flag set */
            goboval |= gobo_element_mask;       
                                    /* & store the new value in a string */
        sprintf(gobo_string,"%d",goboval);

    } else {                        /* ELSE, SUBSTITUTING NEW VALUE IN EXISTING LOCATION */

        if(gobo_array_element>0) {
            for(n=0;n<gobo_array_element;n++) {
                while(*p != ',') {      /* search for the array element by counting separating commas */
                    p++;
                    if(*p==ENDOFSTR || *p == '}') {
                        fprintf(stdout,"ERROR: Failed to find gobo element in array, in string.\n");
                        fflush(stdout);
                        return(0);
                    }
                }
                p++;
            }
        } else
            p++;
        gobo_pointer = p;           /* point to the start of the array element wanted */
                                
        if(gobo_array_element == existing_element_cnt-1) {
            while(*p != '}') {      /* if we're working on the last element in the array */
                p++;                /* look for the array end */
                if(*p==ENDOFSTR) {
                    fprintf(stdout,"ERROR: Failed to find last gobo element in array, in string.\n");
                    fflush(stdout);
                    return(0);
                }
            }
        } else {                    /* else */
            while(*p != ',') {      /* look for the next separating comma */
                p++;
                if(*p==ENDOFSTR || *p == '}') {
                    fprintf(stdout,"ERROR: Failed to find gobo element in array, in string.\n");
                    fflush(stdout);
                    return(0);
                }
            }
        }
        strcpy(end_part,p);         /* keep the remaining characters of the array */
        *p = ENDOFSTR;              /* cut the array at this point */
                                    /* read the existing array element */
        if(sscanf(gobo_pointer,"%d",&goboval)!=1) {
            fprintf(stdout,"ERROR: Failed to read value of existing gobo element in array %s, in string.\n",gobo_pointer);
            fflush(stdout);
            return(0);
        }
        *gobo_pointer = ENDOFSTR;
                                    /* If existing value to be changed */
        if(substitute) {            /* zero the value at gobo_element_mask position */
            zero_mask = ~gobo_element_mask;     
            goboval &= zero_mask;   
        }                           /* put in value 1, only if flag set */
        if(!deleter && (setval==1)) /* & store the new value in a string */
            goboval |= gobo_element_mask;
        sprintf(gobo_string,"%d",goboval);
    }
    strcat(*gobo_line,gobo_string); /* put new value on end of (cut) original string */
    strcat(*gobo_line,end_part);    /* replace end characters of array, at end of string */
    return(1);
}

/******************************** COUNT_GOBO_ARRAY_ELEMENTS_IN_FIRST_GOBO ******************************/

int count_gobo_array_elements_in_first_gobo(char *gobo_line, char *comp, int progno, int new_element_cnt)
{
    int existing_element_cnt, next_available_progno;

    if((existing_element_cnt = count_gobo_array_elements(gobo_line))<=0)
        return(0);
    if(new_element_cnt > existing_element_cnt) {
        if(new_element_cnt > existing_element_cnt + 1) {
            fprintf(stdout,
            "ERROR: MAX_PROCESS_NO is TOO HIGH to tally with the existing gobo count in gobo %s.\n",comp);
            fflush(stdout);
            return(0);
        }
        next_available_progno = ((new_element_cnt - 1) * 32) + 1;
        if(progno < next_available_progno) {
            fprintf(stdout,
            "ERROR: MAX_PROCESS_NO is TOO HIGH to tally with the existing gobo count in gobo %s.\n",comp);
            return(0);
        }
        if(progno > next_available_progno) {
            fprintf(stdout,
            "ERROR: The process-number being entered creates a gap in numbering of processes on the system.\n");
            fflush(stdout);
            return(0);
        }
    } else if(new_element_cnt < existing_element_cnt) {
        fprintf(stdout, 
        "ERROR: new_element_cnt %d existing_element_cnt %d\n",new_element_cnt,existing_element_cnt);
        fprintf(stdout, 
        "ERROR: MAX_PROCESS_NO is TOO LOW to tally with the existing gobo count in gobo %s.\n",comp);
        fflush(stdout);
        return(0);
    }
    return(existing_element_cnt);
}

/******************************** MAKE_NEXT_GOBO_NAME_AND_NUMBER ******************************/

int make_next_gobo_name_and_number(int *gobo_number, int *goboname_len, char **comp, char *pretext, int max_gobos)
{
    int finished_fixing_gobos = 0;
    char num[4];
    (*gobo_number)++;
    (*comp)[0] = ENDOFSTR;
    strcat(*comp,pretext);
    strcat(*comp,"gobo");
    if(*gobo_number == max_gobos-1) {
        *goboname_len +=2;
        strcat(*comp,"_any");
    } else if(*gobo_number == max_gobos) {
        strcat(*comp,"zero");
    } else if(*gobo_number > max_gobos) {
        finished_fixing_gobos = 1;
    } else {
        if(*gobo_number == 10)
            (*goboname_len)++;  
        sprintf(num,"%d",*gobo_number);
        strcat(*comp,num);
    }
    return finished_fixing_gobos;
}

/******************************** CHECK_PROGNO ******************************/

int check_progno(int *progno, int system_update)
{
//    int is_new = 0;
    if(system_update) {

/* TEST */
        if(*progno == 0) {
            *progno = MAX_PROCESS_NO;
//TW UPDATES
            fprintf(stdout,"INFO: using the value %d for the MAX_PROCESS_NO: If this is incorrect, recompile fixgobo, and start again.\n",MAX_PROCESS_NO);
            fprintf(stdout,"INFO: If you do not do this, the gobo data for program %d will be overwritten!!\n",MAX_PROCESS_NO);
            fflush(stdout);

//            is_new = 1;
        }
//      if((*progno >= MIN_TEMPROC_AREA) && (*progno <= MAX_TEMPROC_AREA)) {
//          if(is_new)
//              fprintf(stdout,"ERROR: Invalid MAX_PROCESS_NO for new system processes.\n");
//          else
//              fprintf(stdout,"ERROR: Invalid process number for new system processes.\n");
//          fprintf(stdout,"Cannot be between %d  and %d inclusive\n",MIN_TEMPROC_AREA,MAX_TEMPROC_AREA);
//          fprintf(stdout,"as these are reserved for private user processes.\n");
//          fflush(stdout);
//          return(-1);
//      } else 
        if(*progno > MAX_PROCESS_NO) {
            fprintf(stdout,"ERROR: The process number entered is above MAX_PROCESS_NO\n");
            fflush(stdout);
            return(-1);
//TW UPDATE
        } else if(*progno <= MAX_PROCESS_NO) {
            fprintf(stdout,"WARNING: REDEFINING THE GOBO FOR AN EXISTING PROCESS.\n");
            fflush(stdout);
            return(1);
        }
    } else {
        if(*progno == 0) {
            *progno = MAX_TEMP_PROCESS_NO;
//            is_new = 1;
        }
//      if((*progno > MAX_TEMPROC_AREA) || (*progno < MIN_TEMPROC_AREA)) {
//          if(is_new)
//              fprintf(stdout,"ERROR: Invalid MAX_TEMP_PROCESS_NO for new processes.\n");
//          else
//              fprintf(stdout,"ERROR: Invalid process number for new processes.\n");
//          fprintf(stdout,"Private user processes must lie between %d and %d\n",MIN_TEMPROC_AREA,MAX_TEMPROC_AREA);
//          fprintf(stdout,"If you have run out of space for new processes,\n");
//          fprintf(stdout,"perhaps you should install some of them as common CDP system processes\n");
//          fprintf(stdout,"by contacting the CDP.\n");
//          fflush(stdout);
//          return(-1);
//      } else 
        if(*progno < MAX_TEMP_PROCESS_NO) {
            fprintf(stdout,"WARNING: OVERWRITING AN EXISTING PROCESS.\n");
            fflush(stdout);
            return(1);
        }
    }
    return(0);
}

/******************************** CHECK_FLAGS ******************************/

int check_flags(char *str,int **flags,int max_gobos)
{
    char *p;
    int n;
//TW UPDATE
    if(!strcmp(str,"0")) {
        deleter = 1;
    } else if(strlen(str) != (unsigned int)max_gobos) {
        fprintf(stdout,"ERROR: Bad data string sent to fixgobo.\n");
        fflush(stdout);
        return(-1);
    }
    if((*flags = malloc((max_gobos+2) * sizeof(int)))==NULL) {
        fprintf(stdout,"ERROR: Out of memory to store flags, in fixgobo.\n");
        fflush(stdout);
        return(-1);
    }
    p = str;
    for(n=1;n<=max_gobos;n++) {
        switch(*p) {
        case('0'):  (*flags)[n] = 0;    break;
        case('1'):  (*flags)[n] = 1;    break;
        default:
            fprintf(stdout,"ERROR: Bad character (ascii %d) in gobo-flag string sent to fixgobo.\n",*p);
            fflush(stdout);
            return(-1);
        }
//TW UPDATE
        if(!deleter)
            p++;
    }
    return(1);
}

