#include <stdio.h>
#include <stdlib.h>

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	FILE *fp;
	int c;
	if(argc != 2) {
		fprintf(stderr,"USAGE: getascii filename: where file is an ascii textfile\n");
		return 0;
	}
	if((fp = fopen(argv[1],"r"))==NULL) {
		fprintf(stderr,"Cannot open file %s for output.\n",argv[1]);
		return 0;
	}
	fseek(fp,0,0);
	do {
		c = fgetc(fp);
		fprintf(stderr,"%c %d\t",c,c);
	} while(c != EOF);
	fclose(fp);
	return 1;
}
