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
#include <sndinfo.h>
#include <globcon.h>
#include <modeno.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <sfsys.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

#define NOTEMIN                 ("A-4")
#define NOTEMAX                 ("G5")
#define FRQMAX                  (12543)    /* Approx frequency at MIDI 127 */
#define CONVERT_LOG10_TO_LOG2   (3.321928)
#define my_LOG2(x)              (log10(x) * CONVERT_LOG10_TO_LOG2)
#define my_ANTILOG2(x)          (pow(2.0,x))
#define LOG_SEMITONE_INTERVAL   (0.0250858329719984)
#define QTONE_PER_OCT           (24)

//static double readnote(char *note,dataptr dz);
static int  midi_to_frq(double midi);
static int  frq_to_midi(double frq);
static void note_to_frq(double midi);
//static void note_to_midi(int argc,char *argv[]);
static int  ratio_to_semitones(double ratio);
static int  ratio_to_interval(double ratio);
static int  r_to_i(double ratio);
static void semitones_to_ratio(double semitone_cnt);
//static int  interval_to_semitones(char *str,double *semitiones);
static void semitones_to_oct(double semitone_cnt);
static void oct_to_semitones(double oct);
static void oct_to_ratio(double oct);
static int  ratio_to_oct(double ratio);
static int  do_fraction(double val);
static int  ratio_to_time(double ratio);
static int  time_to_ratio(double timestr);
static void semitones_to_time(double timestr);
static void oct_to_time(double oct);
static int  time_to_semitones(double timestr);
static int  time_to_intervals(double timestr);
static int  time_to_oct(double timestr);
static int  semitones_to_interval(double sc);
static void gain_to_db(double gain);
static void db_to_gain(double dbgain);
static int  midi_to_note(double val,dataptr dz);
static void delay_to_frq(double val);
static int  delay_to_midi(double val);
static void frq_to_delay(double val);
static int  midi_to_delay(double val);
static int  tempo_to_delay(double val);
static int  delay_to_tempo(double val);

/************************ DO_MUSUNITS *************************/

int do_musunits(dataptr dz)
{

    int exit_status = FINISHED, k;
    switch(dz->mode) {
    case(MU_MIDI_TO_FRQ):           exit_status = midi_to_frq(dz->param[MUSUNIT]);              break;
    case(MU_FRQ_TO_MIDI):           exit_status = frq_to_midi(dz->param[MUSUNIT]);              break;
    case(MU_NOTE_TO_FRQ):           note_to_frq(dz->scalefact);                                 break;
    case(MU_NOTE_TO_MIDI):
        k = round(dz->scalefact);
        if(!flteq((double)k,dz->scalefact))
            sprintf(errstr,"MIDI value = %lf or approx %d\n",dz->scalefact,k);      
        else
            sprintf(errstr,"MIDI value = %d\n",k);      
        break;
    case(MU_FRQ_TO_NOTE):
    case(MU_MIDI_TO_NOTE):          midi_to_note(dz->param[MUSUNIT],dz);                        break;
    case(MU_FRQRATIO_TO_SEMIT):     exit_status = ratio_to_semitones(dz->param[MUSUNIT]);       break;
    case(MU_FRQRATIO_TO_INTVL):     exit_status = ratio_to_interval(dz->param[MUSUNIT]);        break;
    case(MU_INTVL_TO_FRQRATIO):     semitones_to_ratio(dz->scalefact);                          break;
    case(MU_SEMIT_TO_FRQRATIO):     semitones_to_ratio(dz->param[MUSUNIT]);                     break;
    case(MU_OCTS_TO_FRQRATIO):      oct_to_ratio(dz->param[MUSUNIT]);                           break;
    case(MU_OCTS_TO_SEMIT):         oct_to_semitones(dz->param[MUSUNIT]);                       break;
    case(MU_FRQRATIO_TO_OCTS):      exit_status = ratio_to_oct(dz->param[MUSUNIT]);             break;
    case(MU_SEMIT_TO_OCTS):         semitones_to_oct(dz->param[MUSUNIT]);                       break;
    case(MU_SEMIT_TO_INTVL):        exit_status = semitones_to_interval(dz->param[MUSUNIT]);    break;
    case(MU_FRQRATIO_TO_TSTRETH):   exit_status = ratio_to_time(dz->param[MUSUNIT]);            break;
    case(MU_SEMIT_TO_TSTRETCH):     semitones_to_time(dz->param[MUSUNIT]);                      break;
    case(MU_OCTS_TO_TSTRETCH):      oct_to_time(dz->param[MUSUNIT]);                            break;
    case(MU_INTVL_TO_TSTRETCH):     semitones_to_time(dz->scalefact);                           break;
    case(MU_TSTRETCH_TO_FRQRATIO):  exit_status = time_to_ratio(dz->param[MUSUNIT]);            break;
    case(MU_TSTRETCH_TO_SEMIT):     exit_status = time_to_semitones(dz->param[MUSUNIT]);        break;
    case(MU_TSTRETCH_TO_OCTS):      exit_status = time_to_oct(dz->param[MUSUNIT]);              break;
    case(MU_TSTRETCH_TO_INTVL):     exit_status = time_to_intervals(dz->param[MUSUNIT]);        break;
    case(MU_GAIN_TO_DB):            gain_to_db(dz->param[MUSUNIT]);                             break;
    case(MU_DB_TO_GAIN):            db_to_gain(dz->param[MUSUNIT]);                             break;
    case(MU_DELAY_TO_FRQ):          delay_to_frq(dz->param[MUSUNIT]);                           break;
    case(MU_DELAY_TO_MIDI):         exit_status = delay_to_midi(dz->param[MUSUNIT]);            break;
    case(MU_FRQ_TO_DELAY):          frq_to_delay(dz->param[MUSUNIT]);                           break;
    case(MU_MIDI_TO_DELAY):         exit_status = midi_to_delay(dz->param[MUSUNIT]);            break;
    case(MU_NOTE_TO_DELAY):         exit_status = midi_to_delay(dz->scalefact);                 break;
    case(MU_TEMPO_TO_DELAY):        exit_status = tempo_to_delay(dz->param[MUSUNIT]);           break;
    case(MU_DELAY_TO_TEMPO):        exit_status = delay_to_tempo(dz->param[MUSUNIT]);           break;
    default:
        sprintf(errstr,"Unknown mode in do_musunits()\n");
        return(PROGRAM_ERROR);
    }
    sprintf(errstr,"%s",errstr);
    print_outmessage_flush(errstr);
    return(exit_status);
}

/**************************** MIDI_TO_FRQ **************************/

int midi_to_frq(double midi)
{
    double frq;
    if(midi<(double)MIDIMIN || midi >(double)MIDIMAX) {
        sprintf(errstr,"MIDI value out of range %d - %d\n",MIDIMIN,MIDIMAX);
        return(GOAL_FAILED);
    }
    frq  = midi;
    frq += 3.0;
    frq /= SEMITONES_PER_OCTAVE;
    frq  = pow((double)2,frq);
    frq *= LOW_A;
    sprintf(errstr,"frequency = %lf\n",frq);
    return(FINISHED);
}

/************************ FRQ_TO_MIDI *****************/

int frq_to_midi(double frq)
{
    double midi;
    if(frq<MINPITCH || frq >(double)FRQMAX) {
        sprintf(errstr,"Frq value out of range %ld - %d\n",round(MINPITCH),FRQMAX);
        return(GOAL_FAILED);
    }
    midi  = frq;
    midi /= LOW_A;
    midi  = my_LOG2(midi);
    midi *= SEMITONES_PER_OCTAVE;
    midi -= 3.0;
    sprintf(errstr,"MIDI value = %lf or approx %ld\n",midi,round(midi));
    return(FINISHED);
}

/************************** NOTE_TO_FRQ *************************/

void note_to_frq(double midi)
{
    double frq;
    frq  = midi;
    frq += 3.0;
    frq /= SEMITONES_PER_OCTAVE;
    frq  = pow((double)2,frq);
    frq *= LOW_A;
    sprintf(errstr,"Frequency = %lf\n",frq);
}

/************************* RATIO_TO_SEMITONES **********************/

int ratio_to_semitones(double ratio)
{  
    double semitone_cnt;
    if(ratio <= 0.0) {
        sprintf(errstr,"Freq ratio <= 0.0 : impossible.\n");
        return(GOAL_FAILED);
    }
    semitone_cnt = fabs(log10(ratio)/(double)LOG_SEMITONE_INTERVAL);
//TW UPDATE
    if(ratio < 1.0)
        semitone_cnt = -semitone_cnt;
    sprintf(errstr,"%lf semitones\n",semitone_cnt);
    return(FINISHED);
}

/************************* RATIO_TO_SEMITONES **********************/

int ratio_to_interval(double ratio)
{
    if(ratio <= 0.0) {
        sprintf(errstr,"Freq ratio <= 0.0 : impossible.\n");
        return(GOAL_FAILED);
    }
    return r_to_i(ratio);
}

/************************* R_TO_I **********************/

int r_to_i(double ratio)
{   
    double semitone_cnt;
    int oct_cnt, qtone_cnt;
    semitone_cnt = fabs(log10(ratio)/(double)LOG_SEMITONE_INTERVAL);
    qtone_cnt = round(semitone_cnt * 2);
    oct_cnt = qtone_cnt/QTONE_PER_OCT;
    qtone_cnt -= oct_cnt * QTONE_PER_OCT;
    switch(oct_cnt) {
    case(1): sprintf(errstr,"1 octave");          break;
    case(0):                                      break;
    default: 
        sprintf(errstr,"%d octaves",oct_cnt);           
        break;
    }
    if(qtone_cnt && oct_cnt)
        strcat(errstr," and a ");
    switch(qtone_cnt) {
    case(0):
        if(oct_cnt==0) {
            sprintf(errstr,"Unison.\n");
            return(FINISHED);
        }
        break;
    case(1):    strcat(errstr,"minor 2nd-\n");      break;
    case(2):    strcat(errstr,"minor 2nd\n");       break;
    case(3):    strcat(errstr,"minor 2nd+\n");      break;
    case(4):    strcat(errstr,"major 2nd\n");       break;
    case(5):    strcat(errstr,"major 2nd+\n");      break;
    case(6):    strcat(errstr,"minor 3rd\n");       break;
    case(7):    strcat(errstr,"minor 3rd+\n");      break;
    case(8):    strcat(errstr,"major 3rd\n");       break;
    case(9):    strcat(errstr,"major 3rd+\n");      break;
    case(10):   strcat(errstr,"4th\n");             break;
    case(11):   strcat(errstr,"4th+\n");            break;
    case(12):   strcat(errstr,"augmented 4th\n");   break;
    case(13):   strcat(errstr,"augmented 4th+\n");  break;
    case(14):   strcat(errstr,"5th\n");             break;
    case(15):   strcat(errstr,"5th+\n");            break;
    case(16):   strcat(errstr,"minor 6th\n");       break;
    case(17):   strcat(errstr,"minor 6th+\n");      break;
    case(18):   strcat(errstr,"major 6th\n");       break;
    case(19):   strcat(errstr,"major 6th+\n");      break;
    case(20):   strcat(errstr,"minor 7th\n");       break;
    case(21):   strcat(errstr,"minor 7th+\n");      break;
    case(22):   strcat(errstr,"major 7th\n");       break;
    case(23):   strcat(errstr,"major 7th+\n");      break;
    }
    return(FINISHED);
}

/************************ SEMITONES_TO_RATIO *******************/

void semitones_to_ratio(double semitone_cnt)
{
    double ratio;
    ratio = pow((double)SEMITONE_INTERVAL,semitone_cnt);
    sprintf(errstr,"frequency ratio is %.8lf\n",ratio);
}
    
/******************************** SEMITONES_TO_OCT ***********************/

void semitones_to_oct(double semitone_cnt)
{
    double oct;
    oct = (double)semitone_cnt/SEMITONES_PER_OCTAVE;
    sprintf(errstr,"octave_distance is %.8lf\n",oct);
}

/******************************** OCT_TO_SEMITONES ***********************/

void oct_to_semitones(double oct)
{
    double semitone_cnt;
    semitone_cnt = oct * SEMITONES_PER_OCTAVE;
    sprintf(errstr,"semitone count  is %.8lf\n",semitone_cnt);
}

/******************************** OCT_TO_RATIO ***********************/

void oct_to_ratio(double oct)
{
    double ratio = my_ANTILOG2(oct);
    sprintf(errstr,"Frequency ratio is %.8lf\n",ratio);
}

/******************************** RATIO_TO_OCT ***********************/

int ratio_to_oct(double ratio)
{
    int exit_status;
    double oct;
    if(ratio <= FLTERR) {
        sprintf(errstr,"Interval ratio too small or negative (Impossible).\n");
        return(GOAL_FAILED);
    }
    oct = my_LOG2(ratio);
    sprintf(errstr,"Octave distance is ");
    if((exit_status = do_fraction(oct))<0)
        return(exit_status);
    strcat(errstr,"\n");
    return(FINISHED);
}

/*********************************** DO_FRACTION ******************************/

int do_fraction(double val)
{
    int n, m, k = 0;
    int is_neg = 0;
    double d, test, thisval = val;
    char temp[64];
    if(val <0.0)  {
        thisval = -thisval;
        is_neg = 1;
    }
    for(n=1;n<5;n*=2) {
        d = 1.0/(double)n;
        if((test = fmod(thisval,d))< FLTERR || d - test < FLTERR) {
            m = (int)(thisval/d);   /* truncate */
            if(!flteq((double)m * d,thisval))
                m++;            
            if(!flteq((double)m * d,thisval)) {
                m-=2;           
                if(!flteq((double)m * d,thisval)) {
                    sprintf(errstr,"Problem in fractions\n");
                    return(PROGRAM_ERROR);
                }
            }
            k = 0L;         
            if(m >= n) {
                k  = m/n;       /* truncate */
                m -= k * n;
            }
            if(is_neg)
                strcat(errstr,"-");
            if(k) {
                sprintf(temp,"%d",k);
                strcat(errstr,temp);
            }
            switch(n) {
            case(2):
                if(m)   strcat(errstr,".5");
                break;  
            case(4):
                switch(m) {
                case(1): strcat(errstr,".25");  break;
                case(2): strcat(errstr,".5");   break;
                case(3): strcat(errstr,".75");  break;
                }
            }
            return(FINISHED);
        }
    }
    sprintf(temp,"%lf",val);
    strcat(errstr,temp);
    return(FINISHED);
}

/******************************** RATIO_TO_TIME ********************************/ 

int ratio_to_time(double ratio)
{
    if(ratio <= 0.0) {
        sprintf(errstr,"Freq ratio <= 0.0 : impossible.\n");
        return(GOAL_FAILED);
    }
    sprintf(errstr,"Timestretch is %lf\n",1.0/ratio);
    return(FINISHED);
}

/******************************** TIME_TO_RATIO ********************************/ 

int time_to_ratio(double timestr)
{
    if(timestr <= 0.0) {
        sprintf(errstr,"timestretch <= 0.0 : impossible.\n");
        return(GOAL_FAILED);
    }
    sprintf(errstr,"Timestretch is %lf\n",1.0/timestr);
    return(FINISHED);
}

/******************************** SEMITONES_TO_TIME ********************************/ 

void semitones_to_time(double semitone_cnt)
{
    double ratio;
    ratio = pow((double)SEMITONE_INTERVAL,semitone_cnt);
    sprintf(errstr,"Timestretch is %lf\n",1.0/ratio);
}

/******************************** OCT_TO_TIME ********************************/ 

void oct_to_time(double oct)
{
    double ratio;
    ratio = my_ANTILOG2(oct);
    sprintf(errstr,"Timestretch is %lf\n",1.0/ratio);
}

/******************************** TIME_TO_SEMITONES ********************************/ 

int time_to_semitones(double timestr)
{
    double semitone_cnt;
    double ratio;
    if(timestr <= 0.0) {
        sprintf(errstr,"timestretch <= 0.0 : impossible.\n");
        return(GOAL_FAILED);
    }
    ratio = 1.0/timestr;
    semitone_cnt = log10(ratio)/(double)LOG_SEMITONE_INTERVAL;
    sprintf(errstr,"%lf semitones\n",semitone_cnt);
    return(FINISHED);
}


/******************************** TIME_TO_INTERVALS ********************************/ 

int time_to_intervals(double timestr)
{
    int exit_status;
    double ratio;
    char *p;
    if(timestr <= 0.0) {
        sprintf(errstr,"timestretch <= 0.0 : impossible.\n");
        return(GOAL_FAILED);
    }
    ratio = 1.0/timestr;
    if((exit_status = r_to_i(ratio))<0)
        return(exit_status);
    if(ratio<1.0) { 
        p = errstr;
        while(*p != ENDOFSTR) {
            if(*p == NEWLINE) {
                *p = ENDOFSTR;
                break;
            }
            p++;
        }
        strcat(errstr," Downwards.\n");
    }
    return(FINISHED);
}

/******************************** TIME_TO_OCT ********************************/ 

int time_to_oct(double timestr)
{
    int exit_status;
    double oct;
    if(timestr <= 0.0) {
        sprintf(errstr,"timestretch <= 0.0 : impossible.\n");
        return(GOAL_FAILED);
    }
//TW UPDATE
    if(flteq((oct = my_LOG2(1.0/timestr)),0.0)) {
        sprintf(errstr,"Octave distance is 0\n");
        return(FINISHED);
    }
    sprintf(errstr,"Octave distance is ");
    if((exit_status = do_fraction(oct))<0)
        return(exit_status);
    strcat(errstr,"\n");
    return(FINISHED);
}

/********************** SEMITONES_TO_INTERVAL **********************/

int semitones_to_interval(double sc)
{
    int ocnt = 0, qcnt, scnt, neg = 0;
    char temp[200];
    errstr[0] = ENDOFSTR;
    if(sc < 0.0)
        neg = 1;
    sc   = fabs(sc);
    ocnt = round(floor(sc/SEMITONES_PER_OCTAVE));
    sc   = fmod(sc,SEMITONES_PER_OCTAVE);
    qcnt = round(sc * 2.0);
    scnt = qcnt/2;
    if(scnt*2 != qcnt)
        qcnt = 1;
    else
        qcnt = 0;
    switch(scnt) {
    case(0):                                break;
    case(1):  sprintf(errstr,"minor 2nd");  break;  
    case(2):  sprintf(errstr,"2ns");        break; 
    case(3):  sprintf(errstr,"minor 3rd");  break;
    case(4):  sprintf(errstr,"3rd");        break;
    case(5):  sprintf(errstr,"4th");        break;
    case(6):  sprintf(errstr,"#4th");       break;
    case(7):  sprintf(errstr,"5th");        break;
    case(8):  sprintf(errstr,"minor 6th");  break;
    case(9):  sprintf(errstr,"6th");        break;
    case(10): sprintf(errstr,"minor 7th");  break;
    case(11): sprintf(errstr,"7th");        break;
    default:
        sprintf(errstr,"Unknown interval.\n");
        exit(PROGRAM_ERROR);
    }
    if(qcnt) {
        if(scnt==0)
            strcat(errstr,"1 quartertone ");
        else
            strcat(errstr," + quartertone");
    }
    if(ocnt) {
        if(scnt > 0 || qcnt > 0) {
            sprintf(temp," plus %d octave",ocnt);
            strcat(errstr,temp);
        } else {
            sprintf(temp,"%d octave",ocnt);
            strcat(errstr,temp);
        }
        if(ocnt > 1) {
            sprintf(temp,"s");
            strcat(errstr,temp);
        }
    }
    if(strlen(errstr) <= 0)
        sprintf(errstr,"Less than a quarter-tone");
    if(neg)
        strcat(errstr," down");
    strcat(errstr,"\n");
    return(FINISHED);
}

/********************** GAIN_TO_DB **********************/

void gain_to_db(double gain)
{
    double dbgain = log10(gain) * 20.0;
    if(dbgain <= MIN_DB_ON_16_BIT)
        sprintf(errstr,"-96dB:  i.e. zero level\n");
    else
        sprintf(errstr,"%.3lfdB\n",dbgain);
}

/********************** DB_TO_GAIN **********************/

void db_to_gain(double dbgain)
{
    double gain = dbtogain(dbgain);
    sprintf(errstr,"Gain = %lf\n",gain);
}

/************************ MIDI_TO_NOTE *************************/

int midi_to_note(double val,dataptr dz)
{
    int oct, imidi, midinote, is_qtone = FALSE, is_approx = FALSE;
    char temp[6];
    double midival;

    if(dz->mode==MU_FRQ_TO_NOTE) {
        if(val<MINPITCH || val >(double)FRQMAX) {
            sprintf(errstr,"Frq value out of range %ld - %d\n",round(MINPITCH),FRQMAX);
            return(GOAL_FAILED);
        }
        midival  = val;
        midival /= LOW_A;
        midival  = my_LOG2(midival);
        midival *= SEMITONES_PER_OCTAVE;
        midival -= 3.0;
    } else
        midival = val;

    midival *= 2.0;
    imidi = round(midival); 
    if(!flteq((double)imidi,midival))
        is_approx = TRUE;
    if(ODD(imidi))
        is_qtone = TRUE;    
    imidi /= 2;

    midinote = imidi%12;
    oct = imidi/12;
    oct -= 5;
    sprintf(errstr,"Note is ");     
    if(is_approx)
        strcat(errstr,"(approx) ");     
    switch(midinote) {
    case(0):    strcat(errstr,"C");     break;
    case(1):    strcat(errstr,"C#");    break;
    case(2):    strcat(errstr,"D");     break;
    case(3):    strcat(errstr,"Eb");    break;
    case(4):    strcat(errstr,"E");     break;
    case(5):    strcat(errstr,"F");     break;
    case(6):    strcat(errstr,"F#");    break;
    case(7):    strcat(errstr,"G");     break;
    case(8):    strcat(errstr,"Ab");    break;
    case(9):    strcat(errstr,"A");     break;
    case(10):   strcat(errstr,"Bb");    break;
    case(11):   strcat(errstr,"B");     break;
    }
    if(is_qtone)
        strcat(errstr,"u");
    sprintf(temp,"%d",oct);
    strcat(errstr,temp);
    strcat(errstr,"\n");
    return(FINISHED);
}

void delay_to_frq(double val) {
    sprintf(errstr,"Frequency is %.5lf Hz\n",SECS_TO_MS/val);
}

int delay_to_midi(double val) {
    int exit_status = FINISHED;
    val = SECS_TO_MS/val;
    exit_status = frq_to_midi(val);
    return(exit_status);
}

void frq_to_delay(double frq) {
    sprintf(errstr,"Delay is %.5lf milliseconds\n",SECS_TO_MS/frq);
}

int midi_to_delay(double midi) {
    double frq, del;
    if(midi<(double)MIDIMIN || midi >(double)MIDIMAX) {
        sprintf(errstr,"MIDI value out of range %d - %d\n",MIDIMIN,MIDIMAX);
        return(GOAL_FAILED);
    }
    frq  = midi;
    frq += 3.0;
    frq /= SEMITONES_PER_OCTAVE;
    frq  = pow((double)2,frq);
    frq *= LOW_A;
    del = SECS_TO_MS/frq;
    sprintf(errstr,"Delay is %lf milliseconds\n",del);

    return(FINISHED);
}

int tempo_to_delay(double tempo) {
    double del = (60 * SECS_TO_MS)/(double)tempo;
    sprintf(errstr,"Delay is %lf milliseconds\n",del);
    return(FINISHED);
}

int delay_to_tempo(double del) {
    double tempo = (60 * SECS_TO_MS)/(double)del;
    sprintf(errstr,"Tempo : crotchet =  %lf\n",tempo);
    return(FINISHED);
}

