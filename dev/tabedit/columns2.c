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



#include <columns.h>

void 	helpm(void),helpM(void),helpg(void),helpr(void),helpl(void),help(void);

/********************************** HELPM **********************************/

void helpm(void)
{
fprintf(stderr,
" ------------------------ MATHEMATICAL OPERATIONS ---------------------------\n"
"| g     find Greatest                | i    find Intervals (MIDI)            |\n"
"| l     find Least                   | im@  Multiply Intervals by @          |\n"
"| t     find Total                   | ia@  Add @ to Intervals               |\n"
"| p     find Product                 | ra   RAtios btwn succesive values     |\n"
"| M     find Mean                    | iv   generate Intermediate Values     |\n"
"| R[@]  find Reciprocals (1/N) (<|>) | iL@  Limit Intervals to < max @       |\n"
"|       with @, find @/N             | il@  Limit Intervals to > min @       |\n"
"| a@    Add @                  (<|>) |                                       |\n"
"| m@    Multiply by @          (<|>) | Ra@  Add Random value between +-@     |\n"
"| d@    Divide by @            (<|>) | RA@  Add Random value between 0 & @   |\n"
"| P@    +ve values to Power @  (<|>) | Rm@  Multiply by Randval between 0-@  |\n"
"|                                    | Rs@  Random Scatter[@=0-1] ascending  |\n"
"|(<|>) means these can apply to vals |      vals over intervals between vals |\n"
"|  < or > threshold on command-line. |                                       |\n"
"|                                    | s[@] Stack values from 0 (overlap @)  |\n"
"| A@  Approximate to multiplies of @ | sl@  Change Slope by factor @         |\n"
"| fl@ ensure all vals >= floor-val @ | sd[@] Sum abs Differences (zigzag)    |\n"
"| li@ ensure all vals <= limit-val @ |       (@=overlap e.g.splicelen)       |\n"
"|                                    | so@   Sum, minus Overlaps             |\n"
"| THRESHOLD VALUE IS INDICATED BY an |       @=overlap (e.g.splicelen)       |\n"
"| extra parameter on command line..  | sn@   Sum N-wise. N = @               |\n"
"|  e.g. {3 means <3 ;  }7 means >7   |                                       |\n"
" -------------------- FOR FURTHER HELP TRY -l -M -g -R ----------------------\n");
exit(1);
}

/************************************ HELPL ***********************************/

void helpl(void)
{
fprintf(stderr,
" ------------------------ LIST REORDER OR EDIT --------------------------\n"
"| c     Count vals (<||>threshold) | e@   Eliminate value @ (+-error)    |\n"
"| mg@   Mark values > @            | eg@  Eliminate vals Greater than @  |\n"
"| ml@   Mark values < @            | el@  Eliminate vals Less than @     |\n"
"| cl    Count lines                | ed@  Eliminate Duplicates           |\n"
"| o     Order list                 |      within range @                 |\n"
"| rr    Reverse list               | ee   Eliminate Even items           |\n"
"| Ro[@] Randomise Order (X@)       | Re@  Eliminate @ items at Random    |\n"
"| dl@   Duplicate List @ times     | bg@  Vals > @ reduced to Bound @    |\n"
"| dv@   Duplicate each Val @ times | bl@  Vals < @ raised to Bound @     |\n"
"|                                  | N@   partitioN to @ files,in blocks |\n"
"| sk@   get 1 value & SKip @       | Nr@  partitioN to @ fils,in rotation|\n"
"| sK@   get @ values, SKip 1       | C    Concatenate files              |\n"
"| cc    columnate                  |                                     |\n");
fprintf(stderr,
"| As    Alphabetic Sort            | S@   Separate @ columns to @ files  |\n"
"| F@    Format vals in @ cols      | J    Join as columns in 1 file      |\n"
"| s[@]  Stack values from 0        | I    Interleave: columns->list      |\n"
"|       (overlap @)                | E[@] vals End to End in 2 columns   |\n"
"| Mr[@] Avoid midi-pitch-class     |      (@ = column2 overshoot of      |\n"
"|       repetition (within @ notes)|      next value in column 1)        |\n"
"| fr[@] Avoid  frq-pitch-class     | v[@] rank by VOTE: ie. by no. of    |\n"
"|       repetition (within @ notes)|      times each number(+-@) occurs  |\n"
"| ir@   Repeat Intervals @ times   | V@   rank frqs by VOTE              |\n"
"|       starting from last entry   |      @ = semitone range for equalfrq|\n"
"| F@    format in @ columns        | G@   reGROUP: take every @th item   |\n"
"|                                  |      from 0, then ditto from 1 etc. |\n"
"| THRESHOLD IS EXTRA PARAM ON CMDLINE... E.G. {3 MEANS <3 ;  }7 MEANS >7 |\n"
" ------------------ FOR FURTHER HELP TRY -m -M -g -R --------------------\n");
exit(1);
}

/*********************************** HELPM ***********************************/

void helpM(void)
{
fprintf(stderr,
" ----------------------------  MUSICAL OPERATIONS ----------------------------\n"
"| Mh    MIDI to Hz                    | sd[@] Sum abs Differences (zigzag)    |\n"
"| Mt    MIDI to Text                  |       (@=overlap e.g. splicelen)      |\n"
"| hM    hz to MIDI                    | sl@   Change slope by factor @        |\n"
"| tM    Text to MIDI                  | so@   Sum, minus Overlaps: @=overlap  |\n"
"| th    Text to Hz                    |       (e.g.splicelen)                 |\n"
"|                                     | Mm@   Major->Minor,(MIDI) @ = key     |\n"
"| q@    Quantise over @               | mM@   Minor->Major,(MIDI) @ = key     |\n"
"| i     get Intervals (e.g. MIDI)     | TM[@] Temper MIDI data. (With @, to a |\n"
"| Ir    Interval(semitones)-> frqratio|       @-note equal-tempered scale     |\n"
"| ra    RAtios between succesive vals |       based on concert-C)             |\n"
"| iM    motivically-Invert MIDI       | Th[@] Temper Hz data. (With @, to a   |\n"
"| ih    motivically-Invert Hz         |       @-note equal-tempered scale     |\n"
"| rm@   motivically-ROtate by @       |       based on concert-C)             |\n");
fprintf(stderr,
"| B     plain Bob : 8 bells           | do@   Duplicate vals at @ Octaves     |\n"    
"| At@   Accel Time-seq from event-    |       (MIDI)                          |\n"
"|       -separation val1 in file      | dO@   Duplicate vals at @ Octaves     |\n"
"|       to val2 in file, total dur @  |       (FREQUENCIES)                   |\n"
"|       (optional: starttime val3)    | Mr[@] Avoid midi-pitchclass repeats   |\n"
"| Tc@   times from crotchet count     |       (within @ notes)                |\n"
"|       in file, and tempo @          | fr[@] Avoid frq-pitchclass repeats    |\n"
"| Tl@   Times from crotchet lengths   |       (within @ notes)                |\n"
"| H@    Generate @ Harmonics          | V@    rank frqs by VOTE               |\n"
"| Hr@   Generate @ Subharmonics       |       (no. of occcurences)            |\n"
"| Hg[@] Group frqs as Harmonics:      |       @ = semitone range within which |\n"
"|       @=tuning tolerance (semitones)|       frqs judged to be equivalent    |\n"
"|       (default .01)                 | td@   Take set of snd-durations and   |\n"    
"| Ad@   Accel Durations from val1 to  |       produce list of start-times, to |\n"    
"|       val2 in file, with total dur @|       give density of @ snds always.  |\n"    
"| DB    convert dB levels to gain vals| st@   sample to time (@ = srate)      |\n"    
"| db    convert gain levels to dB     |                                       |\n"    
" -------------------- FOR FURTHER HELP TRY -m -l -g -R ----------------------\n");
exit(1);
}

/**************************************** HELPG *****************************/

void helpg(void)
{
fprintf(stderr,"%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
" -------------------------- GENERATIVE OPERATIONS --------------------------\n",
"| Rg    Generate Random 0s & 1s      |                                      |\n",
"|       (number of items, in infile) | ic@   Create @ Intervals, size of    |\n",
"| Rv@   generate @ Random Values     |       (1) value in file              |\n",
"|       (maxmin values in infile)    | i=@   Equal Intervals of @ between   |\n",
"| Rc@   Chop @ to Randomsize chunks  |       2 values in file               |\n",
"|       (maxmin chunklens in infile) | D@    @ equal Divisions between      |\n",
"|                                    |       2 values in file               |\n",
"| iv    gen Intermediate Values      | L@    @ Log-equal divisions between  |\n",
"|                                    |       2 +ve values in file           |\n",
"| dv@   Duplicate each Val @ times   | Q@    @ Quadratic-curve steps btwn   |\n",
"| dl@   Duplicate List @ times       |       2 vals in file, where val3 is  |\n",
"| ir@   Repeat Intervals @ times,    |       curvature(>0): <1 down: >1 up  |\n",
"|       starting from last entry     |                                      |\n",
"|                                    |                                      |\n",
"| At@   Accel Time-seq from event-   | B     plain Bob : 8 bells            |\n",
"|       -separation val1 in file, to |                                      |\n",
"|       val2 in file, total dur @    |                                      |\n",
"|       (optional: starttime val3)   |                                      |\n",
"|                                    |                                      |\n",
"| Ad@   Accel Durations from val1 to |                                      |\n",
"|       val2 in file, in total dur @ |                                      |\n",
" ------------------- FOR FURTHER HELP TRY -m -M -l -R ---------------------\n");
exit(1);
}

/********************************** HELPR ***********************************/

void helpr(void)
{
fprintf(stderr,"%s%s%s%s%s%s%s%s%s%s%s%s",
" ---------------------------- RANDOM OPERATIONS ---------------------------\n",
"|                                     |                                    |\n",
"| Ro[@] Randomise Order               | Rc@   Chop @ to Randchunks         |\n",
"|       (& again, @ times)            |       (maxmin chunklens in infile) |\n",
"|                                     | Rg    Generate random 0s & 1s      |\n",
"| Ra@   Add Randval between +-@       |       (number of items, in infile) |\n",
"| RA@   Add Randval between 0 & @     | Rv@   Gen @ random Values          |\n",
"| Rm@   Multiply by Randval btwn 0-@  |       (maxmin vals in infile)      |\n",
"| Rs@   Scatter[@=0-1] ascending vals |                                    |\n",
"|       over intervals between vals   | Re@   Eliminate @ items at Random  |\n",
"|                                     |                                    |\n",
" -------------------- FOR FURTHER HELP TRY -m -M -l -g --------------------\n");
exit(1);
}

/******************************* HELP *******************************/

void help(void)
{
fprintf(stderr,
"a@ .... Add @                          l ..... find LEAST\n"
"A@ .... Approx to multiples of @       li@ ....LIMIT values to <=@\n"
"At@ ... ACCELERATING TIME-seq          M ..... find MEAN\n"
"Ad@ ... ACCELERATING DURATIONS         m@ .... MULTIPLY by @\n"
"As .... ALPHABETIC SORT                mg@ ... MARK values > @\n"
"B ..... plain BOB (bell-ringing)       Mr[@].. MIDI-pitchclass-REPETS avoided.\n"
"bg@ ... Vals >@ reduced to BOUND @     Mh .... MIDI to HZ\n"
"bl@ ... Vals <@ increased to BOUND @   ml@ ... MARK values < @\n"
"C ..... CONCATENATE                    Mm@ ... MAJOR->MINOR (MIDI) in key @\n"
"c ..... COUNT vals    cc...COLUMNATE   Mt .... MIDI to TEXT\n"
"cl .... COUNT LINES                    mM@ ... MINOR->MAJOR (MIDI) in key @\n"
"D@ .... @ equal DIVISIONS btwn 2 vals  N@ .... partitioN -> @ files, in blocks\n"
"DB .... DB values to gain vals         Nr@ ....partN-> @ files, in rotation\n"
"d@ .... DIVIDE  by @                   o ..... ORDER list\n"
"db .... gain vals to DB values         P@ .... +ve vals raised to POWER\n"
"dl@ ... DUPLICATE LIST @ times         p ..... find PRODUCT\n"
"do@ ... DUPL LIST at @ OCTAVES (MIDI)  Q@ ..@ QUADRATIC-curve steps btwn vals.\n"
"dO@ ... DUPL LIST at @ OCTAVES (FRQS)  q@ .... QUANTISE over @\n"
"dv@ ... DUPLICATE each VALUE @ times   R[@]... find RECIPROCALS\n"
"E[@] .. vals END to END in 2 cols      RA@ ... RANDVAL ADDED\n"
"e@ .... ELIMINATE value                Ra@ ... RANDVAL +- ADDED\n"
"ed@ ... ELIMINATE DUPLICATES           ra .... RATIOS betwn vals\n"
"ee .... ELIMINATE EVEN items           Rc@ ... cut @ into RANDOM CHUNKS\n"
"eg@ ... ELIMINATE vals > @             Re@ ... RANDOMLY ELIMINATE items\n"
"el@ ... ELIMINATE vals < @             Rg  ... RANDOM GEN of 0s & 1s\n"
"fr[@].. FRQ pitchclass REPETS avoided  Rm@ ... MULTIPLY by RAND\n"
"F@ .... FORMAT in @ columns            rm@ ... motivic-ROTATE\n"
"fl@ ... FLOOR vals to >=@              Ro[@].. RANDOMISE ORDER\n"
"g ..... find GREATEST                  rr..... REVERSE list\n"
"G@ .... GROUP @th items, cyclically    Rs@ ... RANDOM SCATTER vals\n");
fprintf(stderr,
"hM .... HZ to MIDI                     Rv@ ... generate RAND VALS\n"
"H@ .... Generate HARMONICS             S@ .....SEPARATE cols->files\n"
"Hr@ ... Gen HARMONIC ROOTS (subharms)  s[@] .. STACK vals from 0\n"
"Hg[@].. GROUP freqs as HARMONICS.      sd[@]...SUM abs DIFFERENCES\n"
"I ..... INTERLEAVE                     sK@ ... get @ vals,SKIP 1\n"
"Ir .... INTERVAL(semitones)->frqRATIO  sk@ ... get 1 val, SKIP @\n"
"i ..... get INTERVALS                  sl@ ... change slope by factor @\n"
"i=@ ... create EQUAL INTVLS of @       sn@ ... SUM N-WISE. N = @\n"
"ia@ ... ADD @ to INTERVALS             so@ ... SUM,minus OVERLAPS\n"
"ic@ ... CREATE @ INTERVALS             st@ ... SAMPLE-CNT to TIME\n"
"ih .... motiv-INVERT HZ                t ..... find TOTAL\n"
"iL@ ....LIMIT INTERVALS to < max @     Tc@ ... TEMPO with beatCOUNT ->time\n"
"il@ ....LIMIT INTERVALS to > min @     td@ ... Time density in @ layers.\n"
"iM .... motiv-INVERT:MIDI              Th[@].. TEMPER HZ data\n"
"im@ ... INTERVALS, MULTIPLIED          th .... TEXT to HZ\n"
"ir@ ... INTERVS, REPEATED              Tl@ ... TEMPO & beatLENGTHS ->time\n"
"iv .... INTERMEDIATE VALS              TM[@].. TEMPER MIDI data\n"
"J ..... JOIN as columns                tM .... TEXT to MIDI\n"
"L@ .... LOG-equal divisions            v[@] .. rank by VOTE(no of times occurs)\n"
"                                       V@ ..rank frqs by VOTE(@-semitone steps)\n");
exit(1);
}
																	  

