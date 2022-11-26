#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <standalone.h>

/******* masking GOBOs for the list of programs: each contains 'maskcnt' bitflags, each of 32 bits ********/

#define ENDOFSTR '\0'
#define NUMBER_OF_GOBOS (42)

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

const char* cdp_version = "8.0.0";

/*******************************************************************/

int main(int argc, char *argv[])
{
    char temp[200];
    int  progno, gobindex, charindex;
    int mask;
    if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
        fprintf(stdout,"%s\n",cdp_version);
        fflush(stdout);
        return 0;
    }
    if(argc != 2) {
        fprintf(stdout,"ERROR: Bad call 1 to gobosee\n");
        fflush(stdout);
        return 1;
    }
    if (sscanf(argv[1],"%d",&progno) !=1) {
        fprintf(stdout,"ERROR: Cannot read program number.\n");
        fflush(stdout);
        return 1;
    }
    temp[0] = ENDOFSTR;
    if(progno == MIX_MULTI) {
        fprintf(stdout,"INFO: Multichannel_mixing is a special case.\n");
        return 1;
    }
    progno--;
    gobindex = progno/INT_SIZE;
    charindex = progno % INT_SIZE;
    mask = 1;
    if (charindex > 0)
        mask <<= charindex;
    if(gobo1[gobindex] & mask)      strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo2[gobindex] & mask)      strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo3[gobindex] & mask)      strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo4[gobindex] & mask)      strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo5[gobindex] & mask)      strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo6[gobindex] & mask)      strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo7[gobindex] & mask)      strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo8[gobindex] & mask)      strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo9[gobindex] & mask)      strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo10[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo11[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo12[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo13[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo14[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo15[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo16[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo17[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo18[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo19[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo20[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo21[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo22[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo23[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo24[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo25[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo26[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo27[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo28[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo29[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo30[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo31[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo32[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo33[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo34[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo35[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo36[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo37[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo38[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo39[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo40[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo41[gobindex] & mask)     strcat(temp,"1");   else    strcat(temp,"0");
    if(gobo_any[gobindex] & mask)   strcat(temp,"1");   else    strcat(temp,"0");
    fprintf(stdout,"INFO: %s\n",temp);
    return 1;
}
