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

/*
 *  CONVERT TIME-SERIES DATA TO A SPECIFIED RANGE (and possibly a breaktable output)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <sfsys.h>  //needed for max/min
#define NEWLINE  ('\n')
#define ENDOFSTR ('\0')
#define FLTERR  (0.000002)

#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

const char* cdp_version = "6.1.0";

//CDP LIB REPLACEMENTS
static int usage(void);
static int  get_float_from_within_string(char **str,double *val);
static int rround(double val);
static int flteq(double f1,double f2);
static double read_the_value_from_brktable(double *brk,int brksize,double time);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
    char durstr[200], temp[400], fixfnam[400], *q;
    int  isoutdur = 0, rectified = 0, logarithmic = 0, compact = 0, cutoff = 0, quantise = 0, qcompact = 0, fixed = 0, k;
    double *intable, *outtable, *fixtable = NULL, *p, d;
    double outminval, outmaxval, outrange, outdur, minstep, lastval, maxoutdur = 0.0;
    double inminval, inmaxval, inrange, rconvert, timestep, time, val;
    int inarraysize, outarraysize, fixarraysize = 0, n, t, m;
    double val1, val2, step;
    int laststep, thisstep;
    FILE *fp;

    time = 0.0;
    outdur = 1.0;
    timestep = 1.0;
    minstep = 0.0;

    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stderr,"%s\n",cdp_version);
        return 0;
    }
    if(argc <= 1) {
        usage();    
        return 0;
    } else if(argc < 5 || argc > 9) {
        usage();    
        return 0;
    }
    if(sscanf(argv[3],"%lf",&outminval) != 1) {
        fprintf(stderr, "Can'read minimum output value %s\n",argv[3]);
        return 0;
    }
    if(sscanf(argv[4],"%lf",&outmaxval) != 1) {
        fprintf(stderr, "Can'read maximum output value %s\n",argv[4]);
        return 0;
    }
    outrange = outmaxval - outminval;
    if(argc > 5) {
        k = 5;
        while(k < argc) {
            if(argv[k][0] == '-' && argv[k][1] == 'r')
                rectified = 1;
            else if(argv[k][0] == '-' && argv[k][1] == 'l')
                logarithmic = 1;
            else if(argv[k][0] == '-' && argv[k][1] == 'q')
                quantise = 1;
            else if(argv[k][0] == '-' && argv[k][1] == 'Q') {
                quantise = 1;
                qcompact = 1;
            }
            else {
                if(sscanf(argv[k],"%s",durstr) != 1) {
                    fprintf(stderr, "Can'read flag %s\n",argv[k]);  //RWD added %s
                    return 0;
                }
                if(durstr[0] != '-') {
                    fprintf(stderr, "Invalid flag %s\n",argv[k]);
                    return 0;
                }
                if(durstr[1] == 'd') {
                    if(strlen(durstr) < 3) {
                        fprintf(stderr, "No duration value\n");
                        return 0;
                    }
                    if(sscanf(&(durstr[2]),"%lf",&outdur) != 1) {
                        fprintf(stderr, "Can't read output duration (-d)\n");
                        return 0;
                    }
                    if(outdur <= 0.0) {
                        fprintf(stderr, "Invalid output duration specified (-d).\n");
                        return 0;
                    }
                    isoutdur = 1;
                } else if(durstr[1] == 'm') {
                    if(strlen(durstr) < 3) {
                        fprintf(stderr, "No maximum duration value (-m)\n");
                        return 0;
                    }
                    if(sscanf(&(durstr[2]),"%lf",&maxoutdur) != 1) {
                        fprintf(stderr, "Can'read maximum duration (-m)\n");
                        return 0;
                    }
                    if(maxoutdur <= 0.0) {
                        fprintf(stderr, "Invalid maximum duration specified (-m)\n");
                        return 0;
                    }
                    cutoff = 1;
                } else if(durstr[1] == 'c') {
                    if(strlen(durstr) < 3) {
                        fprintf(stderr, "No minimum step info\n");
                        return 0;
                    }
                    if(sscanf(&(durstr[2]),"%lf",&minstep) != 1) {
                        fprintf(stderr, "Can'read minimum step\n");
                        return 0;
                    }
                    if(minstep <= 0.0) {
                        fprintf(stderr, "Invalid minimum step specified.\n");
                        return 0;
                    }
                    compact = 1;
                } else if(durstr[1] == 'f') {
                    if(strlen(durstr) < 3) {
                        fprintf(stderr, "No fixed times file\n");
                        return 0;
                    }
                    if(sscanf(&(durstr[2]),"%s",fixfnam) != 1) {
                        fprintf(stderr, "Can'read filename for fixed times\n");
                        return 0;
                    }
                    if((fp = fopen(fixfnam,"r"))==NULL) {
                        fprintf(stderr, "Can't open file %s to read time data.\n",fixfnam);
                        return 0;
                    }
                    while(fgets(temp,200,fp)==temp) {    /* COUNT VALS */
                        q = temp;
                        if(*q == ';')   //  Allow comments in file
                            continue;
                        while(get_float_from_within_string(&q,&d))
                            fixarraysize++;
                    }       
                    if((fixtable = (double *)malloc((fixarraysize+2) * sizeof(double)))==NULL) {
                        fprintf(stderr,"INSUFFICIENT MEMORY to store fixed time data.\n");
                        return 0;
                    }
                    lastval = -1.0;
                    p = fixtable;
                    rewind(fp);
                    while(fgets(temp,200,fp)==temp) {    /* READ AND TEST VALS */
                        q = temp;
                        if(*q == ';')   //  Allow comments in file
                            continue;
                        while(get_float_from_within_string(&q,&d)) {
                            if(d < 0.0) {
                                fprintf(stderr,"Invalid subzero time in fixed time data.\n");
                                fclose(fp);
                                return 0;
                            } else if(d <= lastval) {
                                fprintf(stderr,"Times do not all advance in fixed time data.\n");
                                fclose(fp);
                                return 0;
                            }
                            lastval = d;
                            *p = d;
                            p++;
                        }
                    }
                    fclose(fp);
                    fixed = 1;
                } else {
                    fprintf(stderr, "Invalid flag %s\n",argv[k]);
                    return 0;
                }
            }
            k++;
        }
    }
    if(rectified && compact) {
        fprintf(stderr, "Can't specify rectification and also do compacting.\n");
        return 0;
    }
    if(rectified && quantise) {
        fprintf(stderr, "Can't specify rectification and quantisation.\n");
        return 0;
    }
    if(compact && quantise) {
        fprintf(stderr, "Can't specify compacting and quantisation.\n");
        return 0;
    }
    if(fixed && !isoutdur) {
        fprintf(stderr, "Can't specify output times without specifying output dur.\n");
        return 0;
    }
    if(isoutdur && compact) {
        fprintf(stderr, "output dur (-d) not applicable when compacting: use -m flag.\n");
        return 0;
    }
    if((fp = fopen(argv[1],"r"))==NULL) {
        fprintf(stderr, "Can't open file %s to read data.\n",argv[1]);
        return 0;
    }
    inarraysize = 0;
    while(fgets(temp,200,fp)==temp) {    /* COUNT VALS */
        q = temp;
        while(get_float_from_within_string(&q,&d))
            inarraysize++;
    }       
    if((intable = (double *)malloc((inarraysize+2) * sizeof(double)))==NULL) {
        fprintf(stderr,"INSUFFICIENT MEMORY to store input data.\n");
        return 0;
    }
    inminval = HUGE;
    inmaxval = -HUGE;
    p = intable;
    rewind(fp);
    while(fgets(temp,200,fp)==temp) {    /* READ AND TEST VALS */
        q = temp;
        while(get_float_from_within_string(&q,&d)) {
            if(d < inminval)
                inminval = d;
            if(d > inmaxval)
                inmaxval = d;
            *p = d;
            p++;
        }
    }
    fclose(fp);
    inrange = inmaxval - inminval;
    if(inrange <= 0.0) {
        fprintf(stderr,"Input data does not vary: cannot proceed.\n");
        return 0;
    }
    rconvert = outrange/inrange;
    
    outarraysize = inarraysize;
    if(isoutdur) {
        outarraysize *= 2;
        timestep = outdur/(double)inarraysize;
        time = 0.0;
    }
    if((outtable = (double *)malloc((outarraysize+2) * sizeof(double)))==NULL) {
        fprintf(stderr,"INSUFFICIENT MEMORY to store output data.\n");
        return 0;
    }
    n = 0;
    if(logarithmic) {
        for(n=0;n < inarraysize;n++) {
            val = intable[n] - inminval;    //  Range 0 to inrange
            val /= inrange;                 //  Range 0 to 1
            val = exp(val);                 //  Range 1 to e
            val -= 1.0;                     //  Range 0 to (e-1)
            intable[n] = val;
        }
        inminval = HUGE;
        inmaxval = -HUGE;
        for(n=0;n < inarraysize;n++) {
            inmaxval = max(intable[n],inmaxval);
            inminval = min(intable[n],inminval);
        }
        inrange = inmaxval - inminval;
        rconvert = outrange/inrange;
    } 
    if(rectified) {
        for(n=0;n < inarraysize;n++) {
            val = intable[n] - inminval;    //  Range 0 to inrange
            val /= inrange;                 //  Range 0 to 1
            val *= 2.0;                     //  Range 0 to 2
            val -= 1.0;                     //  Range -1 to 1
            intable[n] = fabs(val);         //  Range 0 to 1
        }
        inminval = 0.0;
        rconvert = outrange;
    }
    if(isoutdur) {
        for(n=0,t=0,m=1;n < inarraysize;n++,t+=2,m+=2) {
            val = intable[n] - inminval;
            val *= rconvert;
            val += outminval;
            outtable[t] = time;
            if(quantise)
                outtable[m] = rround(val);
            else 
                outtable[m] = val;
            time += timestep;
            if(cutoff && (time > maxoutdur))
                break;
        }
        outarraysize = t;
    } else {
        for(n=0;n < inarraysize;n++) {
            val = intable[n] - inminval;
            val *= rconvert;
            val += outminval;
            if(quantise)
                outtable[n] = rround(val);
            else 
            outtable[n] = val;
        }
    }
    if(compact) {
        val1 = outtable[0];
        val2 = outtable[1];
        step = val2 - val1;
        if(step >= 0)
            laststep = 1;
        else
            laststep = -1;
        n = 2;
        while(n < outarraysize) {   //  eliminate steps in same direction
            val1 = outtable[n-1];
            val2 = outtable[n];
            step = val2 - val1;
            if(step >= 0)
                thisstep = 1;
            else
                thisstep = -1;
            if(thisstep == laststep) {
                m = n;
                while(m <outarraysize) {
                    outtable[m-1] = outtable[m];
                    m++;
                }
                outarraysize--;
            } else {
                laststep = thisstep;
                n++;
            }
        }
        n = 1;
        while(n < outarraysize) {   //  eliminate too small steps
            val1 = outtable[n-1];
            val2 = outtable[n];
            step = val2 - val1;
            if(fabs(step) < minstep) {
                m = n + 1;
                if(m <outarraysize) {
                    while(m <outarraysize) {
                        outtable[m-1] = outtable[m];
                        m++;
                    }
                }
                outarraysize--;
            } else {
                n++;
            }
        }
        if(cutoff) {
            n = 1;
            val = 0;
            while(n < outarraysize) {   //  curtail total duration
                val += fabs(outtable[n] - outtable[n-1]);
                if(val >= outdur) {
                    outarraysize = n + 1;
                    break;
                }
                n++;
            }
        }
    } else if(qcompact) {
        if(isoutdur) {
            n = 3;
            while(n < outarraysize) {
                if(flteq(outtable[n],outtable[n-2])) {
                    m = n;
                    while(m < outarraysize) {
                        outtable[m-2] = outtable[m];
                        m++;
                    }
                    outarraysize -= 2;
                } else
                    n += 2;
            }
        } else {
            n = 1;
            while(n < outarraysize) {
                if(flteq(outtable[n],outtable[n-1])) {
                    m = n;
                    while(m < outarraysize) {
                        outtable[m-1] = outtable[m];
                        m++;
                    }
                    outarraysize--;
                } else
                    n++;
            }
        }
    }
    if(fixed) {
        n = 0;
        while(n < fixarraysize) {
            val = read_the_value_from_brktable(outtable,outarraysize,fixtable[n]);
            fixtable[n] = val;
            n++;
        }
    }
    if((fp = fopen(argv[2],"w"))==NULL) {
        fprintf(stderr, "Can't open file %s to write data.\n",argv[2]);
        return 0;
    }
    if(fixed) {
        for(n=0;n < fixarraysize;n++) {
            sprintf(temp,"%lf\n",fixtable[n]);
            fputs(temp,fp);
        }
    } else if(isoutdur) {
        for(n=0;n < outarraysize;n+=2) {
            sprintf(temp,"%lf  %lf\n",outtable[n],outtable[n+1]);
            fputs(temp,fp);
        }
    } else {
        for(n=0;n < outarraysize;n++) {
            sprintf(temp,"%lf\n",outtable[n]);
            fputs(temp,fp);
        }
    }
    fclose(fp);
    return(1);
}

/******************************** USAGE2 ********************************/

int usage(void)
{
    fprintf(stderr,
    "USAGE: tsconvert indata outdata min max [-cminstep|-r|-q|-Q] [-ddur [-ftimes]] [-mmaxoutdur] [-l]\n"
    "\n"
    "Convert input data to specified range and format.\n"
    "\n"
    "INDATA  Text data as a list of numerical values.\n"
    "MIN     Minimum value in output data values.\n"
    "MAX     Maximum value in output data values.\n"
    "        In minimum > maximum, data contour is inverted in output.\n"
    "-d      Generate brkpnt file of time-val pairs, duration 'dur'\n"
    "        thus determining the timestep between output values,\n"
    "-m      Curtail total outlength to maximum of 'outdur'.\n"
    "        With brkpnt outputs, values generated beyond maxoutdur are ignored.\n"
    "        with '-c' flag, absolute size of values are summed.\n"
    "-c      compact data, so no (absolute)value is less than minstep,\n"
    "        and steps in same direction are joined into same step\n"
    "-r      Rectify the data around the mean.\n"
    "        Mean value = minimum: other vals become deviations from mean.\n"
    "-q      Quantise the data to whole-number values only.\n"
    "-Q      Quantise the data and suppress adjacent equal-values.\n"
    "-f      Output (untimed) vals at times specified in 'times' file (needs -d)\n"
    "-l      Output values vary as log of input values.\n");
    return 0;
}

/************************** GET_FLOAT_FROM_WITHIN_STRING **************************
 * takes a pointer TO A POINTER to a string. If it succeeds in finding 
 * a float it returns the float value (*val), and it's new position in the
 * string (*str).
 */

int  get_float_from_within_string(char **str,double *val)
{
    char   *p, *valstart;
    int    decimal_point_cnt = 0, has_digits = 0;
    p = *str;
    while(isspace(*p))
        p++;
    valstart = p;   
    switch(*p) {
    case('-'):                      break;
    case('.'): decimal_point_cnt=1; break;
    default:
        if(!isdigit(*p))
            return 0;
        has_digits = 1;
        break;
    }
    p++;        
    while(!isspace(*p) && *p!=NEWLINE && *p!=ENDOFSTR) {
        if(isdigit(*p))
            has_digits = 1;
        else if(*p == '.') {
            if(++decimal_point_cnt>1)
                return 0;
        } else
            return 0;
        p++;
    }
    if(!has_digits || sscanf(valstart,"%lf",val)!=1)
        return 0;
    *str = p;
    return 1;
}

/**************************** RROUND *******************************/

int rround(double val)
{
    int k;
    k = (int)(fabs(val)+0.5);
    if(val < 0.0)
        k = -k;
    return k;
}

/**************************** FLTEQ *******************************/

int flteq(double f1,double f2)
{
    double upperbnd, lowerbnd;
    upperbnd = f2 + FLTERR;     
    lowerbnd = f2 - FLTERR;     
    if((f1>upperbnd) || (f1<lowerbnd))
        return(0);
    return(1);
}

/**************************** READ_THE_VALUE_FROM_BRKTABLE *****************************/

double read_the_value_from_brktable(double *brk,int brksize,double time)
{
    int n;
    double lotime, loval, hitime, hival, diff, tratio, val;

    n = 0;
    while(n < brksize) {
        if(brk[n] >= time)
            break;
        n += 2;
    }
    if(n >= brksize)
        return brk[n-1];
    else if(n == 0)
        return brk[1];
    lotime = brk[n-2];
    loval  = brk[n-1];
    hitime = brk[n];
    hival  = brk[n+1];
    diff = hival - loval;
    tratio = (time - lotime)/(hitime - lotime);
    diff *= tratio;
    val = loval + diff;
    return val;
}
