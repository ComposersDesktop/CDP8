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
#include <string.h>
#include <osbind.h>
#include <processno.h>
/*
#include <minmax.h>
*/                      /*RWD not allowed: not portable. incloude sfsys.h instead */
#include <sfsys.h>
#include "filetype.h"
#include <standalone.h>

/******* masking GOBOs for the list of programs: each contains 'maskcnt' bitflags, each of 32 bits ********/

#define NUMBER_OF_GOBOS (42)
#define ORIG_MASK_ARRAY_ELEMENTS_CNT (17)   /* No of masks on original release + 2 for experimental progs */

// ~~ ***** ADDING A NEW BLOCK OF 32 PROGRAMS : Need to INCREMENT "ORIG_MASK_ARRAY_ELEMENTS_CNT" *** ~~
// ~~ ***** that would be AFTER PROCESS 544 as at June 17th :2020  **** ~~
// ~~ ALSO FIX SPECIAL GOBO MASKS IN 
// ~~   _bulk.tcl:
// ~~   _progmask.tcl:
// ~~   _standalone.tcl:

#define INT_SIZE   (32)    /* Flags assumed to be in 32-bit ints : BUT DON'T CHANGE THIS EVEN IF ints ARE intER!! */
#define CHARBITSIZE (8)     /* bits in a char */

int gobo1[]  = {-1,-1,-1,-1,-1,-1,-16777217,-1,2147483647,-1,-1,-16778241,-1,-939532289,-164097,-19955569,15};         /* set to zero any process that changes srate */
int gobo2[]  = {0,0,0,1,2097152,2097152,0,557873152,0,1048592,4194816,-2147483646,262147,132121664,1075838976,0,0};    /* set to 1 if process works with zero files */
int gobo3[]  = {-1549,-402681857,-16779252,-1048580,-27787265,-14683905,-1310721,-558070023,1609791102,-340820177,-4444827,2063596061,-515137604,-1071658209,-1075876101,-20479857,14}; /* works with 1 files  1,1+  */
int gobo4[]  = {1548,2013294592,1073743859,-133169150,25690623,4198144,169083268,196871,537692545,340394432,8638618,1412432352,112222288,1048749,134369284,268959744,1};          /* works with 2 files  2,1+,2+ */
int gobo5[]  = {8,0,1073742208,-134217728,511,4195328,168821124,131075,1,273318336,8573064,1412431872,471883792,1052685,134365188,268435456,1};        /* works with 3 files  3,1+,2+ */
int gobo6[]  = {0,0,1073742208,-134217728,511,4195328,168821124,131075,1,273285568,8573064,1412431872,471883792,1052685,134365188,268435456,1};        /* works with any number of files (not 0) 1+,2+  */
int gobo7[]  = {-1,-536739841,134741995,1,2097152,2097152,-501218304,29391136,813695105,34750480,4194816,-2147479046,8654547,132302272,1084228099,2089680900,0};   /* File1 = ANAL: else 0  */
int gobo8[]  = {0,536739840,16252944,1,2097152,2097152,-501219328,21002528,258,3390992,6291968,-2147483646,262147,132121664,1075838976,0,0};   /* File1 = PICH (binary): else 0  */
int gobo9[]  = {0,402653184,2097152,1,2097152,2097152,-501219328,21002528,0,1179664,4194816,-2147483646,262147,132121664,1075838976,0,0};  /* File1 = TRNS (binary): else 0  */
int gobo10[] = {0,0,4,1,2097152,2097152,-501219328,21002528,64,1179664,4194816,-2147483646,262147,132121664,1075838976,0,0};                   /* File1 = FMNT : else 0 */
int gobo11[] = {0,0,-167772160,-1,-201326593,-10481917,-67113985,1564508159,1333788220,-35782152,-2154537,-16784889,-549457629,-939712897,-8422148,-2109636469,15};            /* File1 = SND : else 0 */
int gobo12[] = {0,0,0,1,69206016,2097164,-501219328,21002528,0,1179664,4194816,-2147483646,262147,132121664,1075838976,0,0};                   /* File1 = ENV (binary) : else 0 */
int gobo13[] = {0,0,0,1,2097152,2097152,606076928,4225024,0,1048592,4211200,-2147483646,262147,132121664,1075838976,0,0};          /* File1 = TextFile (NOT exclusively numbers): else 0 */
int gobo14[] = {0,402653184,0,1,2097152,2097152,572526592,21002240,0,1048599,4203008,-2147483646,262159,132121664,1075838976,0,0};     /* File1 = pitch or transpos textfile: else 0 */
int gobo15[] = {0,0,0,1,2097152,2097248,572526592,21002240,0,1048592,4194816,-2147483646,262147,132121664,1075838976,0,0};         /* File1 = dB-envelope textfile: else 0 */
int gobo16[] = {0,0,0,1,136314880,2097296,572526592,21002240,0,1048599,4203008,-2147483646,262159,132121664,1075838976,0,0};           /* File1 = normalised-envelope textfile: else 0 */
int gobo17[] = {0,0,0,1,2097152,2097152,572526592,21002240,0,1048599,4203008,-2147481598,262159,132121664,1075838976,0,0};         /* File1 = unranged brktable textfile: else 0 */
int gobo18[] = {0,0,0,1,2097152,2359296,639635456,16807936,0,1048592,4194816,-2147483646,541327363,132121664,1075838976,0,0};          /* File1 = list of sndfiles in a textfile: else 0 */
int gobo19[] = {0,0,0,1,2097152,2621440,35655680,21002240,0,1048592,4194816,-2147483646,262147,132121664,1075838976,0,0};          /* File1 = sndfile synchronisation list: else 0 */
int gobo20[] = {0,0,0,1,2097152,4190208,572526592,-2126481408,0,1048592,4227624,-2147483646,262147,132121664,1075838976,0,0};          /* File1 = mixfile: else 0 */
int gobo21[] = {0,0,0,1,2097152,2097152,572526592,58718208,0,1048599,4203008,-2147483646,262159,132121664,1075838976,0,0};         /* File1 = textfile of list-of-numbers: else 0 */
int gobo22[] = {-1,-1,-16777217,-1,-1,-12582913,-1,-603979777,2147483647,-1,-1,-16778241,-1,-939532289,-33025,-19955569,15};           /* Always 1 / non-functional */
int gobo23[] = {-1,-1,-16777217,-1,-1,-14675969,-1,-603979777,2147483647,-1,-1,-16778241,-541065217,-939532289,-33025,-19955569,15}; /* If file1=sndlist/synclist, if these must be same SR, set to 0*/
int gobo24[] = {-1,-1,-16777217,-1,-1,-8388609,-1,-641695745,2147483647,-1,-1,-16778241,-1,-939532289,-33025,-19955569,15};            /* Always 1 / non-functional */
int gobo25[] = {-1,-1,-1090519041,-1,-1,-8388609,-268435457,-536871489,2147483647,-1,-321,-16778241,-1,-939532289,-39169,-20021105,15};                            /* Reject progs not accepting mono sndsys files by setting to 0*/
int gobo26[] = {-1,-536739841,1544028139,-4194297,-201326593,-8389646,-35329,1610612735,914362497,-1747041840,785784791,-50392065,-1064845613,-941432864,1593318019,-53542907,7};  /* Reject progs not accepting stereo sndsys files by setting to 0: pitch, formants, transpos, and envbin get 0 */
int gobo27[] = {-1,-536739841,470286315,130023431,-201327104,-14677006,-402688569,1610612223,914362497,-1814674992,785784343,-52489734,-1064845613,-941432864,1593311875,-53608443,7};/* If process won't run with >2chas,sets to 0,all ANAL progs=1: sndprogs limited to stereo max get 0, pitch,formants,transpos,& envbin get 0 */
int gobo28[] = {-1541,-2013265921,-1090519058,-1048579,-25690113,-12586753,-135528837,-637730824,2146662015,-340296129,-8638619,-1429210113,-506486785,-940585006,-134402309,-19955569,14}; /* >1 file: to reject all lastfiles except =ANAL set to 1*/
int gobo29[] = {-1549,-2013294593,-1090521059,-1048579,-25690113,-12586753,-135528837,-637730824,1609791486,-340296129,-8638619,-1429210593,-514875473,-940585134,-134402309,-288915313,14}; /* >1 file: to reject all lastfiles except =FMNT set to 1*/
int gobo30[] = {-1549,-2013294593,-16779252,-1,-1,-8388609,-1,-637730817,1610612350,-67141633,-1,-16778721,-8388689,-939532417,-33025,-288915313,15};           /* >1 file: to reject all lastfiles except =SND   set to 1*/
int gobo31[] = {-9,-28673,-1090521076,-1048579,-25690113,-12586753,-135528837,-637730824,1609791102,-340296129,-8638619,-1429210593,-514875473,-940585134,-134402309,-288915313,14}; /* >1 file: to reject all lastfiles except =PCH or TRNSP binary   set to 1*/
int gobo32[] = {-1549,-1610641409,-1090521076,-1048579,-524289,-12586753,-135528837,-603980040,1609791102,-273318337,-8638619,-1429210593,-514875473,-940585134,-134402309,-288915313,14}; /* >1 file: to reject all lastfiles except =ENV(or dBENV) BRK  set to 1*/
int gobo33[] = {-1549,-1610641409,-1090521076,-1048579,-25690113,-12586753,-135528837,-637731080,1609791102,-340427201,-8638619,-1429210593,-514875473,-940585134,-134402309,-288915313,14}; /* if lastfile = textfile: if you want to reject all lastfiles except number lists, set to 0 */
int gobo34[] = {-9,-28673,-1090521059,133169151,-524800,-8391937,-135266693,-637730824,2147483646,-273154497,-8523905,-1429210113,-471883793,-940584961,-134398213,-288391025,14};         /* >1 file: if all files must be same type: set to 0 */
int gobo35[] = {-1549,-1610641409,-1090521076,133169149,-512,-12586753,-135528837,-637730819,1609791102,-273154241,-8589459,-1429210593,-514875473,-940585134,-134402309,-288915313,14};   /* >1 file: if all files must be same srate: set to 0 */
int gobo36[] = {-1549,-1610641409,-1090521076,133169151,-512,-8392193,-135528577,-637730819,1609791102,-272629953,-8589331,-1362101729,-514875473,-940585102,-134402309,-288915313,14};    /* >1 file: if all files must be same chancnt: set to 0 */
int gobo37[] = {-1549,-1610641409,-16779252,-1048577,-1,-8391937,-1,-637730824,1610612350,-1,-1,-16778721,-8388689,-939532417,-33025,-20479857,15};    /* >1 file: if all files must be same other props: set to 0 : but 1 for all sndfile only processes */
int gobo38[] = {-1,-134217729,-16777217,-1,-1,-8388609,-1,-603979777,2147483647,-1,-1,-16778241,-1,-939532289,-33025,-19955569,15};                    /* >1 file: if infiles are P and T, and output is binary, and hence 1 input must be binary: set to 0 */
int gobo39[] = {-1549,-2013294593,-1090521076,-3,-524289,-12586753,-135528837,-637731080,1609791102,-340394433,-8638619,-1429210593,-514875473,-940585134,-134402309,-288915313,14};   /* >1 file: to reject all lastfiles except =ENV binary   set to 1*/
int gobo40[] = {-1549,-1610641409,-1090521076,-1048579,-524289,-12586753,-135528837,-603980040,1609791102,-273318337,-8638619,-1429210593,-514875473,-940585134,-134402309,-288915313,14}; /* >1 file: if it can't use textfile as lastfile, set to 0 */
int gobo41[] = { -1,-1,-1,-1,-8388609,-1,-1,-1,2147483647,-67108865,-1,-16778241,-1,-939532289,-33025,-19955569,15};                                       /* always set to 1 for NEW progs */
int gobo42[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};                                         /* always set to 1 for NEW progs */
int gobo_any[] = {0,0,0,0,0,0,33554432,0,0,0,0,0,0,0,0,0,0};   /* processes which work with absolutely ANY files (i.e. including non-CDP valid files): set to 1, else set to 0 */
int gobozero[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};      /* set all gobos to 0 */

void    usage(void);
void    gobo_EQUALS(int** gobo_1,int gobo_2[]);
void    gobo_AND(int** gobo_1,int* gobo_2);
void    gobo_OR(int** gobo_1,int* gobo_2);
void    gobo_ZEROED(int** gobo_1);

/***** TESTING ONLY ******/
int     show(char *str,int *gobo_out,int progno,int valid_multifile);

#define FALSE   (0)
#define TRUE    (1)

#define ALL_INFILES_OF_SAME_TYPE    (16)
#define ALL_INFILES_OF_SAME_SRATE   (8)
#define ALL_INFILES_OF_SAME_CHANCNT (4)
#define ALL_INFILES_OF_SAME_PROPS   (2)
#define AT_LEAST_ONE_PFT_INFILE     (1)

/***** DATA COMING FROM INPUT FILES *****

[1] Count of input files
[2] Filetype of 1st infile
[3] Filetype of last infile
[4] Cumulative filetypes (those types which all files share)
[5] Bitflag re file compatibility etc. (16+8+4+2+1)
    #           16  = all infiles of same type
    #           8   = all infiles of same srate
    #           4   = all infiles of same channel count
    #           2   = all infiles have all other properties compatible
    #           1   = At least 1 of infiles is a binary PFT file
[6] Number of channels (of all files, if all have same channels: otherwise, of last file)

****/

int process_for_single_file = 0;
int maskcnt;
const char* cdp_version = "8.0.0";

/*******************************************************************/

int main(int argc, char *argv[])
{
    int     filecnt, n, m;
    int    firstfile_type=0,lastfile_type=0,compatibility=0,chancnt=0, mask;
    int     ismachine_create = 0, is_fixed_srate = 0;
    unsigned int start = hz1000();

    int pbrk            = FALSE;
    int tbrk            = FALSE;
    int tfile           = FALSE;
    int mixlist         = FALSE;
    int pfile2          = FALSE;
    int pbrk2           = FALSE;
    int tbrk2           = FALSE;
    int nlist2          = TRUE;
    int valid_multifile = TRUE;

    int *gobo_out, *gobo_accumulate=NULL;
    char *temp;
    int progno = -1;
    int cmdline_testing_mode = 0;
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    maskcnt = min(((MAX_PROCESS_NO - 1)/32) + 1,ORIG_MASK_ARRAY_ELEMENTS_CNT);
    if(argc == 1) {
        fprintf(stdout,"TO TEST THE GOBO SETTING FOR A NEW PROGRAM\n\n");
        fprintf(stdout,"1) In the soundloom code, MAIN, set gobo_testing to 1\n");
        fprintf(stdout,"2) restart the SOUND LOOM, choose zero or more files, & proceed to process.\n");
        fprintf(stdout,"3) When the gobo cmdline is displayed, note it down.\n");
        fprintf(stdout,"4) Now run gobo in command.tos, using the same cmdline PLUS\n");
        fprintf(stdout,"   THE PROCESS NUMBER, placed at the end of the cmdline.\n");
        fprintf(stdout,"   (The process number can be found in the file 'processno.h')\n\n");
        fprintf(stdout,"   The GOBO program will report at what stage it decides that\n");
        fprintf(stdout,"   the process will not run with the files entered.\n\n");
        fprintf(stdout,"   If you are developing a new application, and the process SHOULD run\n");
        fprintf(stdout,"   with the entered files, you need to correct the program's gobo settings.\n");
        return(0);
    }
    if(argc==5 || argc == 9) {
        if(sscanf(argv[argc-1],"%d",&progno)!=1) {
            fprintf(stderr,"Invalid program number given.\n");
            while (!(hz1000() - start))
                ;
            return(-1);
        } else if (progno < 1 || progno > MAX_PROCESS_NO) {
            fprintf(stderr,"Program number %d does not exist.\n",progno);
            while (!(hz1000() - start))
                ;
            return(-1);
        }
        cmdline_testing_mode = 1;
        argc--;
    }
    if(argc < 4) {
        fprintf(stdout,"ERROR: Incorrect call to gobo program: not enough args\n");
        while (!(hz1000() - start))
            ;
        return(-1);
    }

    if(sscanf(argv[1],"%d",&filecnt)!=1) {
        fprintf(stdout,"ERROR: Cannot read filecnt argument to gobo program.\n");
        while (!(hz1000() - start))
            ;
        return(-1);
    }
    if(sscanf(argv[2],"%d",&ismachine_create)!=1) {
        fprintf(stdout,"ERROR: Cannot read machinecreate argument to gobo program.\n");
        while (!(hz1000() - start))
            ;
        return(-1);
    }
    if(sscanf(argv[3],"%d",&is_fixed_srate)!=1) {
        fprintf(stdout,"ERROR: Cannot read is_fixed_srate argument to gobo program.\n");
        while (!(hz1000() - start))
            ;
        return(-1);
    }
    if(filecnt > 0) {                   /* if process has files, read the data on filetypes etc */
        if(argc!=8) {
            fprintf(stdout,"ERROR: Incorrect call to gobo program: too many args\n");
            while (!(hz1000() - start))
                ;
            return(-1);
        }
        if(sscanf(argv[4],"%d",&firstfile_type)!=1) {
            fprintf(stdout,"ERROR: Cannot read firstfile_type argument to [3] gobo program.\n");
            while (!(hz1000() - start))
                ;
            return(-1);
        }
        if(sscanf(argv[5],"%d",&lastfile_type)!=1) {
            fprintf(stdout,"ERROR: Cannot read lastfile_type argument to [4] gobo program.\n");
            while (!(hz1000() - start))
                ;
            return(-1);
        }
        if(sscanf(argv[6],"%d",&compatibility)!=1) {
            fprintf(stdout,"ERROR: Cannot read compatibility argument [5] to gobo program.\n");
            while (!(hz1000() - start))
                ;
            return(-1);
        }
        if(sscanf(argv[7],"%d",&chancnt)!=1) {
            fprintf(stdout,"ERROR: Cannot read channel count argument [6] to gobo program.\n");
            while (!(hz1000() - start))
                ;
            return(-1);
        }
    }

    if((gobo_out = malloc(maskcnt * sizeof(int)))==NULL) {
        fprintf(stdout,"ERROR: Can't allocate memory space 1 for gobos.\n");
        while (!(hz1000() - start))
            ;
        return(-1);
    }

    if(filecnt > 0) {
        if((gobo_accumulate = malloc(maskcnt * sizeof(int)))==NULL) {
            fprintf(stdout,"ERROR: Can't allocate memory space 2 for gobos.\n");
            while (!(hz1000() - start))
                ;
            return(-1);
        }
        for(n=0;n<maskcnt;n++)
            gobo_accumulate[n] = 0;
    }

                /* Initialise the process gobos for process or machine situations */

    if(filecnt == 0) {
        if(ismachine_create)
            gobo_EQUALS(&gobo_out,gobozero);    /* initialise gobo to accept all machine-compatible 0 file procs */
        else                                        
            gobo_EQUALS(&gobo_out,gobo2);   /* initialise gobo to accept all progs requiring 0 files */
    } else {
            /*  If there are input files, adjust processes-gobo according to file props */
        gobo_ZEROED(&gobo_out);             /* initialise gobo to accept nothing */

        switch(filecnt) {                   /* accept programs on basis of input filecnt */
        case(1): valid_multifile = FALSE;
                 process_for_single_file = 1;
                 gobo_OR(&gobo_out,gobo3);  /* all progs requiring 1 file, or 1-or-more files */                break;
        case(2): gobo_OR(&gobo_out,gobo4);  /* all progs requiring 2 files,1-or-more files,2-or-more files  */  break;
        case(3): gobo_OR(&gobo_out,gobo5);  /* all progs requiring 3 files,1-or-more files,2-or-more files  */  break;
        default: gobo_OR(&gobo_out,gobo6);  /* all progs requiring 1-or-more files, 2-or-more files  */         break;
        }
        if(cmdline_testing_mode && !show("AFTER FILECNT TEST (gobo6)",gobo_out,progno,valid_multifile))
            return(0);

        if(is_fixed_srate) 
            gobo_AND(&gobo_out,gobo1);          /* if interface uses fixed srate, disallow srate-changing progs */

        if(cmdline_testing_mode && !show("AFTER TEST FOR SYSTEM AT FIXED SRATE (gobo1)",gobo_out,progno,valid_multifile))
            return(0);
        switch(firstfile_type) {
        case(ANALFILE):     gobo_AND(&gobo_out,gobo7);  /* Accept only progs having ANALFILE as first file */       break;
        case(PITCHFILE):    gobo_AND(&gobo_out,gobo8);  /* Accept only progs having PITCHFILE as first file */      break;
        case(TRANSPOSFILE): tfile = TRUE;
                            gobo_AND(&gobo_out,gobo9);  /* Accept only progs having TRANSPOSFILE as first file */   break;
        case(FORMANTFILE):  gobo_AND(&gobo_out,gobo10); /* Accept only progs having FORMANTFILE as first file */    break;
        case(SNDFILE):      gobo_AND(&gobo_out,gobo11); /* Accept only progs having SNDFILE as first file */        break;
        case(ENVFILE):      gobo_AND(&gobo_out,gobo12); /* Accept only progs having ENVELOPE BINFILE as 1st file */ break;
        case(LINELIST_OR_WORDLIST):             
                            gobo_AND(&gobo_out,gobo13); /* Accept only progs of textlines(not just nos.) as file1*/ break;
        case(WORDLIST):     gobo_AND(&gobo_out,gobo13); /* Accept only prgs havng unspecific words */
                                                        /*(& not only numbers) as 1st file */                       break;

        default:                                    /* DEAL WITH ALL OTHER TEXTFILE TYPES */
            if(firstfile_type & IS_A_PITCH_BRKFILE)         pbrk = TRUE;
            if(firstfile_type & IS_A_TRANSPOS_BRKFILE)      tbrk = TRUE;
            if(pbrk || tbrk)            
                gobo_OR(&gobo_accumulate,gobo14);   /* Accept progs having pitch or transpos textfile as first file */
            if(firstfile_type & IS_A_DB_BRKFILE)
                gobo_OR(&gobo_accumulate,gobo15);   /* Accept progs having dB-envelope textfile as first file */
            if(firstfile_type & IS_A_NORMD_BRKFILE)
                gobo_OR(&gobo_accumulate,gobo16);   /* Accept progs having normalised-envelope textfile as first file */
            if(firstfile_type & IS_AN_UNRANGED_BRKFILE)
                gobo_OR(&gobo_accumulate,gobo17);   /* Accept progs having unranged brktable textfile as first file */
            if(firstfile_type & IS_A_SNDLIST)
                gobo_OR(&gobo_accumulate,gobo18);   /* Accept progs having list of sndfiles in a textfile as first file */
            if(firstfile_type & IS_A_SYNCLIST)
                gobo_OR(&gobo_accumulate,gobo19);   /* Accept progs having sndfile synchronisation list as first file */
            if((firstfile_type & IS_A_MIXFILE) && (firstfile_type != WORDLIST)) {
                mixlist = TRUE;
                gobo_OR(&gobo_accumulate,gobo20);   /* Accept progs having mixfile textfile as first file */
            }
            if(firstfile_type & IS_A_NUMLIST)
                gobo_OR(&gobo_accumulate,gobo21);   /* Accept progs having list-of-numbers textfile as first file */
            gobo_AND(&gobo_out,gobo_accumulate);    /* Accept all the accumulated progs (only) */
            if((firstfile_type & IS_A_SNDLIST_FIXEDSR) != IS_A_SNDLIST_FIXEDSR) {
                if(!mixlist)
                    gobo_AND(&gobo_out,gobo23);
            }
        }

        if(cmdline_testing_mode && !show("AFTER FILETYPE TESTS",gobo_out,progno,valid_multifile))
            return(0);

        switch(chancnt) {
        case(0):
            break;                          /* Ignore files with 0 chans (textfiles) */
        case(1):
            gobo_AND(&gobo_out,gobo25); /* Reject progs not accepting mono files */
            break;
        case(2):    
            gobo_AND(&gobo_out,gobo26); /* Reject progs not accepting stereo files */
            break;
        default:
            gobo_AND(&gobo_out,gobo27); /* Reject progs taking ONLY mono or stereo files */
            break;
        }

        if(cmdline_testing_mode && !show("AFTER CHANNEL CNT TEST (gobo27)",gobo_out,progno,valid_multifile))
            return(0);

        if(valid_multifile) {   /* i.e. More than 1 input file */
            if(cmdline_testing_mode && !show("AFTER VALID MULTIFILE TEST",gobo_out,progno,valid_multifile))
                return(0);

            switch(lastfile_type) {
            case(ANALFILE):     gobo_AND(&gobo_out,gobo28); /* Accept only progs taking analfile as last file */    break;
            case(FORMANTFILE):  gobo_AND(&gobo_out,gobo29); /* Accept only progs taking fmntfile as last file */    break;
            case(SNDFILE):      gobo_AND(&gobo_out,gobo30); /* Accept only progs taking sndfile as last file  */    break;
            case(PITCHFILE):    pfile2 = TRUE;
            /* fall thro */
            case(TRANSPOSFILE): gobo_AND(&gobo_out,gobo31); /* Accept only progs taking pitch */
                                                             /* or transpos binfile as last file */
                if(cmdline_testing_mode && !show("2nd FILE IS P or T BINARY",gobo_out,progno,valid_multifile))
                    return(0);
                break;
            case(ENVFILE):      gobo_AND(&gobo_out,gobo39);/* progs taking only env-binfiles as last file  */       break;

            default:                                /* DEAL WITH ALL TEXTFILE TYPES */
                if(!(lastfile_type & IS_A_TEXTFILE)) {

                    valid_multifile = FALSE;
                    if(cmdline_testing_mode && !show("2nd FILE IS NOT A TEXTFILE",gobo_out,progno,valid_multifile))
                        return(0);

                    gobo_EQUALS(&gobo_out,gobo_any);    /* i.e. Nothing else works, except HOUSE-BUNDLE options */
                } else {
                    gobo_AND(&gobo_out,gobo40);         /* Drop all files that DON'T use textfile as last file */


                    if(cmdline_testing_mode && !show("2nd FILE IS A TEXTFILE",gobo_out,progno,valid_multifile))
                        return(0);

                    if(lastfile_type & IS_A_PITCH_BRKFILE)      pbrk2 = TRUE;
                    if(lastfile_type & IS_A_TRANSPOS_BRKFILE)   tbrk2 = TRUE;

                    if(!((lastfile_type & IS_A_NORMD_BRKFILE) || (lastfile_type & IS_A_DB_BRKFILE)))
                        gobo_AND(&gobo_out,gobo32); /* Accept only progs taking envelope type brkfiles */


                    if(cmdline_testing_mode && !show("AFTER NORMD-BRKFILE TEST (gobo32)",gobo_out,progno,valid_multifile))
                        return(0);

                    if(!(lastfile_type & IS_A_DB_BRKFILE)
                    && !(lastfile_type & IS_A_NORMD_BRKFILE)
                    && !(lastfile_type & IS_A_TRANSPOS_BRKFILE)
                    && !(lastfile_type & IS_A_POSITIVE_BRKFILE))
                        gobo_AND(&gobo_out,gobo41); /* env-impose exception */


                    if(cmdline_testing_mode && !show("UNRANGED BRKFILE TEST",gobo_out,progno,valid_multifile))
                        return(0);

                    if((lastfile_type & IS_A_NUMBER_LINELIST) != IS_A_NUMBER_LINELIST)  {
                        nlist2 = FALSE;
                        gobo_AND(&gobo_out,gobo33);     /* Reject progs not taking list of numbers as lastfile */
                    }

                    if(cmdline_testing_mode && !show("NUMBER LIST TEST (gobo33)",gobo_out,progno,valid_multifile))
                        return(0);

                    if(!(pbrk2 || tbrk2 || nlist2)) {   /* Nothing else works, except 0-FILE options */
                        valid_multifile = FALSE;
                        gobo_EQUALS(&gobo_out,gobo_any);
                    }

                    if(cmdline_testing_mode && !show("ALL OTHER TEXTFILES TEST",gobo_out,progno,valid_multifile))
                        return(0);

                }
            }

            if(cmdline_testing_mode && !show("LAST FILETYPE",gobo_out,progno,valid_multifile))
                return(0);

            if(valid_multifile) {
                if((tfile||tbrk) && !pbrk && (pfile2||pbrk2) && !tbrk2) {
                    valid_multifile = FALSE;            /* Can't have Tfile followed by Pfile */
                    gobo_EQUALS(&gobo_out,gobo_any);
                } else {
                    if(!(compatibility & ALL_INFILES_OF_SAME_TYPE))     gobo_AND(&gobo_out,gobo34);
                    if(!(compatibility & ALL_INFILES_OF_SAME_SRATE))    gobo_AND(&gobo_out,gobo35);
                    if(!(compatibility & ALL_INFILES_OF_SAME_CHANCNT))  gobo_AND(&gobo_out,gobo36);
                    if(!(compatibility & ALL_INFILES_OF_SAME_PROPS))    gobo_AND(&gobo_out,gobo37);
                    if(!(compatibility & AT_LEAST_ONE_PFT_INFILE))      gobo_AND(&gobo_out,gobo38);
                }
            }

            if(cmdline_testing_mode && !show("AFTER COMPATIBILITY TEST",gobo_out,progno,valid_multifile))
                return(0);

        }
/* KLUJ for NEW MUTICHANNEL MIXFILES */
        if(firstfile_type == MIX_MULTI) {
            for(n=0;n<maskcnt;n++)
                gobo_out[n] = 0;
            gobo_out[7] = /* -2147483648 */ 0x80000000;  /* RWD: bypass gcc warning about signedness. NB: assumes 32bit int! */
        }
    }
    if(cmdline_testing_mode && !show("AFTER ALL TESTS",gobo_out,progno,valid_multifile))
        return(0);
    if((temp = malloc((maskcnt * ((sizeof(int) * CHARBITSIZE)+1)) + 2))==NULL) {
        fprintf(stdout,"ERROR: Can't allocate memory space 3 for gobos.\n");
        while (!(hz1000() - start))
            ;
        return(-1);
    }
    temp[0] = '\0';
    for(n=0;n<maskcnt;n++) {
        mask = 1;
        for(m=0;m<INT_SIZE;m++) {
            if(gobo_out[n] & mask)
                strcat(temp,"1");
            else
                strcat(temp,"0");
            mask <<= 1;
        }
    }
    strcat(temp,"\n");
    fprintf(stdout,"%s\n",temp);
    while (!(hz1000() - start))
        ;
    return(0);
}

/********************************* gobo_EQUALS *********************************/

void gobo_EQUALS(int** gobo_1,int gobo_2[])
{
    int n;
    for(n=0;n<maskcnt;n++)
        (*gobo_1)[n] = gobo_2[n];
}

/********************************* gobo_ZEROED *********************************/

void gobo_ZEROED(int** gobo_1)
{
    int n;
    for(n=0;n<maskcnt;n++)
        (*gobo_1)[n] = 0;
}

/********************************* gobo_AND *********************************/

void gobo_AND(int** gobo_1,int* gobo_2)
{
    int n;
    for(n=0;n<maskcnt;n++)
        (*gobo_1)[n] &= gobo_2[n];
}

/********************************* gobo_OR **********************************/

void gobo_OR(int** gobo_1,int* gobo_2)
{
    int n;
    for(n=0;n<maskcnt;n++)
        (*gobo_1)[n] |= gobo_2[n];
}

int show(char *str,int *gobo_out,int progno,int valid_multifile) {
    int mask = 1;
    int gobono, itemno, n = 0;
    progno--;
    gobono = progno/32;
    itemno = progno%32;
    if(itemno>0) {
        for(n=0;n<itemno;n++) {
            mask <<= 1;
        }
    }
    if(gobo_out[gobono] & mask) {
        if(process_for_single_file == 1)
            fprintf(stdout,"%s: process OK\n",str);
        else if(valid_multifile)
            fprintf(stdout,"%s: process OK: with these files\n",str);
        else
            fprintf(stdout,"%s: process OK: but NOT with these files\n",str);
        return(1);
    } else {
        fprintf(stdout,"%s: process NOT OK\n",str);
        return(0);
    }
}
