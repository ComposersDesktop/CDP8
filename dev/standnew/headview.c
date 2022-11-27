#include <stdio.h>
#ifdef unix
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#endif

#define BUFLEN 64
#define ENDOFSTR ('\0')

static void showbytes(int thisbuflen, int thisstart,char *buffer);

int main(int argc,char *argv[])
{
	int fh;
	int buflen = BUFLEN, bufcnt, bytescnt, partsect, n, thisbuflen, thisstart;
	char buffer[BUFLEN];
	if(argc != 3) {
		fprintf(stderr,"USAGE: headview filename bytescnt\n");
		return 0;
	}
	if(sscanf(argv[2],"%d",&bytescnt) < 1) {
		fprintf(stderr,"Cannot read bytescnt\n");
		return 0;
	}
	if(bytescnt <= 0) {
		fprintf(stderr,"Invalid bytescnt\n");
		return 0;
	}
#ifdef WIN32
	if((fh = open(argv[1],O_RDONLY|O_RAW,0))<0) {
#else
    if((fh = open(argv[1],O_RDONLY,0))<0) {    
#endif
		fprintf(stderr,"Cannot open file %s\n",argv[1]);
		return 0;
	}
	bufcnt = bytescnt/buflen;
	partsect = bytescnt - (bufcnt * buflen);
	thisstart = 0;
	for(n=0;n<bufcnt;n++) {
		thisbuflen = read(fh,buffer,BUFLEN);
		if(thisbuflen < 0) {
			fprintf(stderr,"Cannot read buffer %d\n",n+1);
			return 0;
		}
		showbytes(thisbuflen,thisstart,buffer);
		thisstart += thisbuflen;
		if(thisbuflen < BUFLEN) {
			partsect = 0;
			break;
		}
	}
	if(partsect > 0)
		showbytes(partsect,thisstart,buffer);
	return 1;
}

void showbytes(int thisbuflen, int thisstart,char *buffer)
{
	int thisend = thisstart + thisbuflen;
	int m = 0;
	char *p = buffer;
	char temp1[2000], temp2[2000], temp3[2], temp4[64];
	temp1[0] = ENDOFSTR;
	temp2[0] = ENDOFSTR;
	temp3[1] = ENDOFSTR;
	while(m < thisbuflen) {
		sprintf(temp4,"%d",*p);
		strcat(temp2,temp4);
		strcat(temp2," ");
		if(isprint(*p))
			temp3[0] = *p;
		else
			temp3[0] = '~';
		strcat(temp1,temp3);
		strcat(temp1," ");
		p++;
		m++;
	}
	fprintf(stdout,"CHARS %d to %d\n",thisstart + 1,thisend);
	fprintf(stdout,"%s\n",temp1);
	fprintf(stdout,"%s\n",temp2);
	fflush(stdout);
}