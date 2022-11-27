/*
 * Copyright (c) 1983-2013 Richard Dobson and Composers Desktop Project Ltd
 * http://people.bath.ac.uk/masrwd
 * http://www.composersdesktop.com
 * This file is part of the CDP System.
 * The CDP System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *
 * The CDP System is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with the CDP System; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
 
/* chxformat.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <limits.h>
#include <portsf.h>

#ifdef unix
/* in portsf.lib */
extern int stricmp(const char *a, const char *b);
#endif

enum {ARG_PROGNAME, ARG_INFILE,ARG_NARGS};
enum {GUID_PCM,GUID_IEEE,GUID_AMB_PCM,GUID_AMB_IEEE};

char* guidnames[] = {"PCM","PCM FLOAT","AMB PCM","AMB FLOAT"};


#define REVDWBYTES(t)	( (((t)&0xff) << 24) | (((t)&0xff00) << 8) | (((t)&0xff0000) >> 8) | (((t)>>24) & 0xff) )
#define REVWBYTES(t)	( (((t)&0xff) << 8) | (((t)>>8) &0xff) )
#define TAG(a,b,c,d)	( ((a)<<24) | ((b)<<16) | ((c)<<8) | (d) )

#ifdef linux
#define POS64(x) (x.__pos)
#else
#define POS64(x) (x)
#endif

#define WAVE_FORMAT_PCM			(0x0001)
#define sizeof_WFMTEX  (40)
typedef struct _GUID 
{ 
    unsigned int        Data1; 
    unsigned short       Data2; 
    unsigned short       Data3; 
    unsigned char        Data4[8]; 
} GUID; 


typedef struct  {
	WORD  wFormatTag; 
    WORD  nChannels; 
    DWORD nSamplesPerSec; 
    DWORD nAvgBytesPerSec; 
    WORD  nBlockAlign; 
    WORD  wBitsPerSample; 

} WAVEFORMAT;


typedef struct { 
    WORD  wFormatTag; 
    WORD  nChannels; 
    DWORD nSamplesPerSec; 
    DWORD nAvgBytesPerSec; 
    WORD  nBlockAlign; 
    WORD  wBitsPerSample; 
    WORD  cbSize; 
} WAVEFORMATEX; 

typedef struct {
    WAVEFORMATEX    Format;				/* 18 bytes */
    union {
        WORD wValidBitsPerSample;       /* bits of precision  */
        WORD wSamplesPerBlock;          /* valid if wBitsPerSample==0 */
        WORD wReserved;                 /* If neither applies, set to */
                                        /* zero. */
    } Samples;
    DWORD    dwChannelMask;				/* which channels are */
                                        /* present in stream  */
    GUID     SubFormat;
} WAVEFORMATEXTENSIBLE;

static const GUID  KSDATAFORMAT_SUBTYPE_PCM = {0x00000001,0x0000,0x0010,
								{0x80,
								0x00,
								0x00,
								0xaa,
								0x00,
								0x38,
								0x9b,
								0x71}};

static const GUID  KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {0x00000003,0x0000,0x0010,
								{0x80,
								0x00,
								0x00,
								0xaa,
								0x00,
								0x38,
								0x9b,
								0x71}};

static const GUID SUBTYPE_AMBISONIC_B_FORMAT_PCM = { 0x00000001, 0x0721, 0x11d3, 
												{ 0x86,
												0x44,
												0xc8,
												0xc1,
												0xca,
												0x0,
												0x0,
												0x0 } };


static const GUID SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT = { 0x00000003, 0x0721, 0x11d3, 
												{ 0x86,
												0x44,
												0xc8,
												0xc1,
												0xca,
												0x0,
												0x0,
												0x0 } };


#define WAVE_FORMAT_IEEE_FLOAT	(0x0003)
#define WAVE_FORMAT_EXTENSIBLE	(0xfffe)

static int compare_guids(const GUID *gleft, const GUID *gright)
{
	const char *left = (const char *) gleft, *right = (const char *) gright;
	return !memcmp(left,right,sizeof(GUID));
}


static int wavDoRead(FILE* fp, void* buf, DWORD nBytes)
{
	DWORD got = 0;	
	if((got = fread(buf,sizeof(char),nBytes,fp)) != nBytes) {
        return 1;
    }
	return 0;
}

static int byte_order()					
{						    
  int   one = 1;
  char* endptr = (char *) &one;
  return (*endptr);
}

static void fmtSwapBytes(WAVEFORMATEX  *pfmt)
{	
	pfmt->wFormatTag	= (WORD) REVWBYTES(pfmt->wFormatTag);
	pfmt->nChannels		= (WORD) REVWBYTES(pfmt->nChannels);
	pfmt->nSamplesPerSec	= REVDWBYTES(pfmt->nSamplesPerSec);
	pfmt->nAvgBytesPerSec	= REVDWBYTES(pfmt->nAvgBytesPerSec);
	pfmt->nBlockAlign	= (WORD) REVWBYTES(pfmt->nBlockAlign);
	pfmt->wBitsPerSample	= (WORD) REVWBYTES(pfmt->wBitsPerSample);	
}



void printmaskinfo()
{
    printf("WAVEX Speaker positions:\n");

    
printf(
"FRONT LEFT             0x1\n"
"FRONT RIGHT            0x2\n"
"FRONT CENTER           0x4\n"
"LOW FREQUENCY          0x8\n"
"BACK LEFT              0x10\n"
"BACK RIGHT             0x20\n"
"FRONT LEFT OF CENTER   0x40\n"
"FRONT RIGHT OF CENTER  0x80\n"
"BACK CENTER            0x100\n"
"SIDE LEFT              0x200\n"
"SIDE RIGHT             0x400\n"
"TOP CENTER             0x800\n"
"TOP FRONT LEFT         0x1000\n"
"TOP FRONT CENTER       0x2000\n"
"TOP FRONT RIGHT        0x4000\n"
"TOP BACK LEFT          0x8000\n"
"TOP BACK CENTER        0x10000\n"
"TOP BACK RIGHT         0x20000\n"
       );
printf("The value 0x80000000 is reserved.\n");
    
printf("Standard layouts:\n"

"Mono       0x40        (64)\n"
"Stereo     0x03         (3)\n"
"Quad       0x33        (51)\n"
"LCRS       0x107      (263)\n"
"5.0        0x37        (55)\n"
"5.1        0x3f        (63)\n"
"7.1        0xff       (255)\n"
"Cube       0x2d033 (184371)\n"
       ); 
}

void usage(void)
{
    printf("CDP MCTOOLS: CHXFORMAT v1.0.1beta (c) RWD,CDP 2009\n");
    printf("change GUID and/or speaker mask in WAVEX header of infile.\n");
    printf("usage: chxformat [-m | [[-t] [-gGUID] [-sMASK]] filename\n");
    printf(
           " -gGUID : change GUID type between PCM and AMB.\n"
           "          Plain WAVEX: GUID = 1\n"
           "          AMB:         GUID = 2\n"
           " -sMASK : change speaker position mask.\n"
           "          MASK = 0:  unset channel mask\n"
           "             else set to MASK\n"
           "          MASK may be given in decimal or hex (prefix '0x').\n"
           " -m     : (not in combination with other options)\n"
           " NOTE: speaker positions are only supported for WAVEX PCM files.\n"
           "       If GUID is set to 2, the -s option should not be used. Any existing\n"
           "       speaker positions will be set to zero.\n"
           " Type chxformat -m to see list of WAVEX mask values.\n"
           " -t     : Test only: do not modify file.\n"
           "  If only infile given, program prints current GUID type and speaker mask.\n"
           "  In test mode, program checks file, prints current channel mask as binary,\n"
           "   and new mask in binary, if -s option set.\n"
           "  Otherwise: program modifies infile (\"destructively\") - use with care!\n"
           );
    
}

void binprintf(int val,int skipzeroes)
{
    unsigned int nbits = sizeof(int) * CHAR_BIT;
    unsigned int i = 0;
    unsigned int mask = 1 << (nbits-1); /* i.e. 0x80000000; */
    // skip leading zeroes
    if(skipzeroes){
        for(;i < nbits;i++){
            if(val&mask)
                break;
            mask>>=1;
        }
    }
    for(;i < nbits;i++){
        if(i > 0 && i%8 == 0)
            printf(" ");
        printf("%d",(val & mask) ? 1 : 0);
        mask >>= 1;
    }
}

int countbits(int val)
{
    unsigned int nbits = sizeof(int) * CHAR_BIT;
    unsigned int i = 0;
    int count = 0;
    unsigned int mask = 1 << (nbits-1); /* i.e. 0x80000000; */
    for(;i< nbits;i++){
        if(val & mask)
            count++;
        mask >>= 1;
    }
    return count;
}

int isdec(int ch){
    if(ch >= '0' && ch <= '9')
        return 1;
//    printf("isdec fail: %d\n",ch);
    return 0;
}

int ishex(int ch){
    
    if((ch >='A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
        return 1;
//     printf("ishex fail: %c\n",ch);
    return 0;
}

int validate_mask(const char* mask)
{
    int is_dec = 1,is_hex = 1;
    size_t i,len;
    char* pch;
    if(mask==NULL)
        return 0;
    len = strlen(mask);
    if(len==0)
        return 0;
    
    pch = (char*) mask;
    /*try hex */
    if(len > 2 && *pch =='0'){
        pch++;
        if(*pch=='x' || *pch =='X'){ // prefix found
            pch++;
//            printf("got hex prefix\n");
            for(i = 0;i < len-2;i++){
//                printf("testing %d \n",pch[i]);
                if(!(isdec(pch[i]) || ishex(pch[i]))){
                    is_hex = 0;
                    break;
                }
            }
            if(is_hex)
                return 1;
        }
        // not hex, maybe decimal
    }
    pch = (char*) mask;
    for(i=0;i < len;i++){
        if(!isdec(pch[i])){
            is_dec = 0;
            break;
        }
    }
    return is_dec;
}



int main (int argc, char**argv)
{
    int do_changeguid = 0;
    int in_guidtype =0;
    int guidtoset = 0;
    int do_changemask = 0;
    unsigned int speakermask = 0;
    int fmtfound = 0;
    int test = 0;
    int src_is_amb = 0;
    FILE* fp  = NULL;
    char* maskstring = NULL;
    psf_format outformat;
    psf_format new_outtype;
    DWORD tag;
    DWORD size;
    fpos_t bytepos;
    DWORD fmtoffset = 0;
    DWORD guidoffset = 0;
    DWORD maskoffset = 0;
	WORD cbSize;
    WORD validbits;
    DWORD chmask;
    WAVEFORMATEXTENSIBLE fmt;
    int is_little_endian = byte_order();
    
    /* CDP version number */
    if(argc==2 && (stricmp(argv[1],"--version")==0)){
        printf("1.0.1b\n");
        return 0;
    }
    
    if(argc<2){
        usage();
        return 1;
    }
    while(argv[1][0]=='-'){
		switch(argv[1][1]){
            case 'm':
                if(argc > 2){
                    printf("cannot use -m with other options\n");
                    return 1;
                }
                else{
                    printmaskinfo();
                    return 0;
                }
                break;
            case 'g':
                if(do_changeguid){
                    printf("Cannot use -g option more than once!\n");
                    return 1;
                }
                guidtoset = atoi(&argv[1][2]);
                if(guidtoset < 1 || guidtoset > 2) {
                    printf("bad value for -g option - use 1 or 2 only.\n");
                    return 1;
                }
                do_changeguid = 1;
                break;
            case 's':
                if(do_changemask){
                    printf("Cannot use -s option more than once!\n");
                    return 1;
                }
                maskstring =  &argv[1][2];
                if(validate_mask(maskstring)==0){
                    printf("Bad format for mask argument.\n"
                           "Value must be decimal, or hex with 0x prefix.\n");
                    return 1;
                }
                    
                do_changemask = 1;
                break;
            case 't':
                if(test==1){
                    printf("You only need to set -t once!\n");
                }
                else
                    printf("Test mode: no changes will be made.\n");
                test = 1;
                
                break;
            default:
                printf("illegal option %s\n",argv[1]);
                return 1;
        }
        argc--;
        argv++;
    }
    if(argc != 2){
        printf("infile argument required.\n");
        usage();
        return 1;
    }
    psf_init();
    /* nitty-gritty to read header */
    outformat = psf_getFormatExt(argv[ARG_INFILE]);
	if(!(outformat == PSF_STDWAVE|| outformat==PSF_WAVE_EX)){
		printf("file mustbe WAVEX format with .wav or .amb extension.\n");
        return 1;
	}
    
    fp = fopen(argv[ARG_INFILE],"rb+");
    if(fp==NULL){
        printf("unable to open infile %s\n",argv[ARG_INFILE]);
        return 1;
    }
    if(wavDoRead(fp,(char *)&tag,sizeof(DWORD))
		|| wavDoRead(fp,(char *) &size,sizeof(DWORD))) {
		printf("read error 1\n");
        return 1;
    }
	if(!is_little_endian)
		size = REVDWBYTES(size);
	else
		tag = REVDWBYTES(tag);
	if(tag != TAG('R','I','F','F')){
		printf("not a RIFF file\n");
        return 1;
    }
	if(size < (sizeof(WAVEFORMAT) + 3 * sizeof(WORD))){
		printf("file has bad header.\n");
        return 1;
    }

	if(wavDoRead(fp,(char *)&tag,sizeof(DWORD))) {
		printf("read error 2\n");
        return 1;
    }
	if(is_little_endian)
		tag = REVDWBYTES(tag);
	if(tag != TAG('W','A','V','E')){
		printf("Not a WAVE file.\n");
        return 1;    
    }
	for(;;){
        if(fmtfound) 
            break;
		if(wavDoRead(fp,(char *)&tag,sizeof(DWORD))
				|| wavDoRead(fp,(char *) &size,sizeof(DWORD))) {
			printf("read error 3\n");
            return 1;
        }
		if(!is_little_endian)
			size = REVDWBYTES(size);
		else
			tag = REVDWBYTES(tag);
		switch(tag){
		case(TAG('f','m','t',' ')):
			if( size < sizeof(WAVEFORMAT)){
				printf("file has bad format.\n");
                return 1;
            }
            
			if(size > sizeof_WFMTEX) {
				printf("file has unsupported WAVE format.\n");
                return 1;
            }
			if(fgetpos(fp,&bytepos)) {
			    printf("seek error\n");
                return 1;
            }
			fmtoffset = (DWORD) POS64(bytepos);
			if(wavDoRead(fp,(char *) &fmt.Format,sizeof(WAVEFORMAT))){				
				printf("read error 4\n");
                return 1;
			}
			if(!is_little_endian)		
				fmtSwapBytes(&fmt.Format);	
			/* calling function decides if format is supported*/
			if(size > sizeof(WAVEFORMAT)) {
				if(wavDoRead(fp,(char*)&cbSize,sizeof(WORD))){
					printf("read error 5\n");
                    return 1;
                }
				if(!is_little_endian)		
					cbSize = (WORD) REVWBYTES(cbSize);
                if(cbSize==0){
                    printf("file is plain WAVE format.\n");
                    return 1;
                }
				if(fmt.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE){
					if(cbSize != 22) {
						printf("not a recognized WAVEX file.\n");
                        return 1;
					}
				}				
				if(wavDoRead(fp,(char *) &validbits,sizeof(WORD))){
					printf("read error 6\n");
                    return 1;
                }
                
				if(!is_little_endian)
                    validbits = REVWBYTES(validbits);
                fmt.Samples.wValidBitsPerSample = (WORD) validbits;
				if(wavDoRead(fp,(char *) &chmask,sizeof(DWORD))) {
					printf("read error 7\n");
                    return 1;
                }
				if(!is_little_endian)
                    chmask = REVDWBYTES(chmask);
                fmt.dwChannelMask = chmask;
				if(wavDoRead(fp,(char *) &(fmt.SubFormat),sizeof(GUID))) {
					printf("read error 8 \n");
                    return 1;
                }
				if(!is_little_endian){
					fmt.SubFormat.Data1 = REVDWBYTES(fmt.SubFormat.Data1);
					fmt.SubFormat.Data2 = (WORD) REVWBYTES(fmt.SubFormat.Data2);
					fmt.SubFormat.Data3 = (WORD) REVWBYTES(fmt.SubFormat.Data3);	
				}
				/* if we get a good GUID, we are ready to make changes! */
                
				if(compare_guids(&(fmt.SubFormat),&(KSDATAFORMAT_SUBTYPE_PCM))) {
                    in_guidtype = GUID_PCM;
                    if(test)
                        printf("Current GUID: KSDATAFORMAT_SUBTYPE_PCM.\n");
                }
                else if(compare_guids(&(fmt.SubFormat),&(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))) {
                    in_guidtype = GUID_IEEE;
                    if(test)
                        printf("Current GUID: KSDATAFORMAT_SUBTYPE_IEEE_FLOAT.\n");
                }
                else if(compare_guids(&(fmt.SubFormat),&(SUBTYPE_AMBISONIC_B_FORMAT_PCM))) {
                    in_guidtype = GUID_AMB_PCM;
                    src_is_amb = 1;
                    if(test)
                        printf("Current GUID: SUBTYPE_AMBISONIC_B_FORMAT_PCM.\n");
                }
                else if(compare_guids(&(fmt.SubFormat),&(SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT))){
                    in_guidtype = GUID_AMB_IEEE;
                    src_is_amb = 1;
                    if(test)
                        printf("Current GUID: SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT.\n");
                }
                else {
                    printf("unrecognized WAVE_EX GUID.\n");
                    return 1;
                }				
			}
            else {
                printf("WAVEX format required.\n"
                       "Use copysfx to convert to WAVEX format.\n");
                return 1;
            }
            fmtfound = 1;			
			break;
		case TAG('d','a','t','a'):
            if(!fmtfound){
                printf("bad WAVE file: no fmt chunk found!\n");
                return 1;
            }                 
        default:
			/* unknown chunk - skip */
            if(!fmtfound) {
                if(fseek(fp,size,SEEK_CUR)){
                    printf("seek error\n");
                    return 1;
                }
            }
			break;
		}
	}
    if(!fmtfound){
         printf("no fmt chunk found!\n");
         return 1;
    }
    
    maskoffset = fmtoffset + sizeof(WAVEFORMAT) + sizeof(WORD)*2;
    guidoffset = maskoffset + sizeof(DWORD);
    
    if(!(do_changeguid || do_changemask)){
        /* display what we know */
        printf("Wordsize     : %d\n",validbits);
        printf("Channels     : %d\n",fmt.Format.nChannels);
        printf("GUID         : %s\n",guidnames[in_guidtype]);
        if(chmask==0)
            printf("Channel mask : 0\n");
        else
            printf("Channel mask : 0x%06x (%d)\n",(unsigned int) chmask,(int) chmask);
        if(chmask){
            printf("Channel mask (binary): ");
            binprintf(chmask,1);
            printf("There are %d speaker positions set.\n",countbits(chmask));
            
        }
        printf("\n");
    }
    else {
        int bits,is_same = 0;

        /* check all settings before making any changes */
        if(do_changemask){
            // be strict!
            if(src_is_amb && guidtoset != 1){
                printf("-s flag only supported for PCM files (AMB speaker mask must be zero).\n"
                       "Exiting.\n");
                return 1;
            }
            if(strncmp(maskstring,"0x",2)==0)
                sscanf(maskstring,"%x", &speakermask);
            else
                sscanf(maskstring,"%u", &speakermask);
            bits = countbits(speakermask);
            /* can't do much if a silly huge value given */
            if(speakermask < 0 || speakermask >= 0x40000){
                printf("Mask value out of range!\n");
                return 1;
            }
            if(bits > fmt.Format.nChannels){
                printf("Error: %d positions requested for %d channels: \n",bits,fmt.Format.nChannels);
                binprintf(speakermask,1);
                printf("\n");
                return 1;
            }
            if(bits && bits < fmt.Format.nChannels){
                printf("Warning: %d requested positions less than channels in file.\n",bits);
            }
        }
                
        if(do_changeguid){
            GUID nuguid;
            
            // set float or pcm GUID as required.
            // no copy to same: only convert if types are different
            switch(guidtoset){
            case 1:       // plain wavex - the easy iption
                if(in_guidtype == GUID_AMB_PCM){
                    nuguid = KSDATAFORMAT_SUBTYPE_PCM;
                }
                else if(in_guidtype == GUID_AMB_IEEE) {
                    nuguid = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
                }
                else
                    is_same = 1;
                new_outtype = PSF_STDWAVE;
                break;
            case 2:  // convert to amb - must zero speaker flags
                if(in_guidtype == GUID_PCM)
                    nuguid = SUBTYPE_AMBISONIC_B_FORMAT_PCM;
                else if(in_guidtype == GUID_IEEE)
                    nuguid = SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT;
                else
                    is_same = 1;
                // set mask to zero if setting AMB format
                if(is_same==0) {
                    do_changemask = 1;
                    maskstring = "0";
                    speakermask = 0;
                }
                new_outtype = PSF_WAVE_EX;    
                break;
            }
            if(is_same){
                printf("new GUID identical - no change made.\n");
            }
            else {
                int err;
                //make the change!
                if(!is_little_endian){
                    nuguid.Data1 = REVDWBYTES(nuguid.Data1);
					nuguid.Data2 = (WORD) REVWBYTES(nuguid.Data2);
					nuguid.Data3 = (WORD) REVWBYTES(nuguid.Data3);
                }
                if(!test){
                    if(fseek(fp,guidoffset,SEEK_SET) !=0){
                        printf("seek error updating channelmask. exiting.\n");
                        return 1;
                    }
                    err = fwrite((char*) &nuguid,1,sizeof(GUID),fp);
                    if(err != sizeof(GUID)){
                        printf("Error updating GUID. File may have inconsistent header.\n");
                        return 1;
                    }
                    printf("new GUID set.\n");
                    
                }
            }
            
        }
        if(do_changemask){
            /* read user mask value */
            /* TODO: full parse, trap bad characters */
            if(chmask==speakermask){
                if(speakermask > 0)
                    printf("Requested mask is already set. Data not modified.\n");
            }
            else {
                DWORD writemask;
                int err;
                
                if(speakermask > 0){
                    printf("new mask = %d (",speakermask);
                    binprintf(speakermask,1);
                    printf(")\n");
                }
                if(fseek(fp,maskoffset,SEEK_SET) !=0){
                    printf("seek error updating channelmask. exiting.\n");
                    return 1;
                }
                writemask = speakermask;
                if(!is_little_endian)
                    writemask = REVDWBYTES(speakermask);
                if(!test){
                    err = fwrite((char*) &writemask,1,sizeof(DWORD),fp);
                    if(err != sizeof(DWORD)){
                        printf("Error updating mask. File may have inconsistent header.\n");
                        return 1;
                    }
                    printf("New mask set.\n");
                }
            }
                
        }
        if(outformat == PSF_STDWAVE && new_outtype==PSF_WAVE_EX)
            printf("File extension should be changed to amb.\n");
        else if(outformat == PSF_WAVE_EX && new_outtype == PSF_STDWAVE)
            printf("File extension should be changed to wav.\n");
    }
    fseek(fp,0,SEEK_END);
    fclose(fp);
    return 0;
}


    
