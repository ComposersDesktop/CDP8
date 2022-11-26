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
#include <string.h>
#include <ctype.h>
#include <osbind.h>
#include <time.h>
#define ENDOFSTR ('\0')
const char* cdp_version = "7.1.0";

int main(int argc, char *argv[]) {
//TW UPDATE
	unsigned long tickdur = 0, start = hz1000();
	time_t now = time(0);
	char *q, *p, *z, temp[64];
	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(now < 0)
		return 0;
	z = temp;
	z = ctime((const time_t *)&now);
	p = z;

	while(isspace(*p))	/* SAFETY */
		p++;

	q = p;
	while(!isspace(*p))
		p++;

	while(isspace(*p))	/* SAFETY */
		p++;
	q = p;
	while(!isspace(*p))
		p++;
	*p = ENDOFSTR;
    
	strcpy(temp,q);	/* Aug */
	p++;

	while(isspace(*p))	/* SAFETY */
		p++;

	q = p;
	while(!isspace(*p))
		p++;
	*p = ENDOFSTR;
    // RWD  if just one digit, addd leading zero for Mac and Linux Windows-style - expected by Soundloom
#ifndef WIN32
    if(strlen(q)==1){
        strcat(temp,"0");
    }
#endif
	strcat(temp,q);	/* Aug25 */
	strcat(temp,"_");	/* Aug25_ */
	p++;

	while(isspace(*p))	/* SAFETY */
		p++;

	q = p;
	while(*p != ':')
		p++;
	*p = ENDOFSTR;
	strcat(temp,q);	/* Aug25_10 */
	strcat(temp,"-");	/* Aug25_10- */
	p++;
	q = p;
	while(*p != ':')
		p++;
	*p = ENDOFSTR;
	strcat(temp,q);	/* Aug25_10- */
	strcat(temp,"-");	/* Aug25_10-10- */
	p++;
	q = p;
	while(!isspace(*p))
		p++;
	*p = ENDOFSTR;
	strcat(temp,q);	/* Aug25_10-10- */
	strcat(temp,".");	/* Aug25_10-10-53. */
	p++;
	q = p;
	while(isspace(*p))
		p++;
	q = p;
	while(!isspace(*p))
		p++;
	*p = ENDOFSTR;	
	strcat(temp,q);	/* Wed_Aug25_10_10_53.1999 */
	fprintf(stdout,"%s",temp);
	fflush(stdout);
//TW UPDATE: waiting
	while(tickdur == 0)
		tickdur = hz1000() - start;
	return 0;
}

