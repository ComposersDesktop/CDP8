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

void sort_numbers(int *);

/***************************** PRINT_NUMBERS() ************************/

void print_numbers(void)
{
	int n;
	for(n=0;n<cnt;n++)
		do_valout_flush(number[n]);
}

/********************** REMOVE_FRQ_PITCHCLASS_DUPLICATES ******************/

void remove_frq_pitchclass_duplicates(void)
{
	int n, m, k, z, failed = 0, move;
	double interval;
	k = cnt-ifactor;
	z = 1;
	for(n=0;n<k;n++) {
		for(m=1;m<=ifactor;m++) {
			if(n+m >= cnt)
				break;
			interval = log(number[n]/number[n+m])/LOG_2_TO_BASE_E;
			interval = fmod(fabs(interval),1.0);
			if(interval<ONEMAX || interval>ONEMIN) {
				if((move = f_repos(n+m))<0) {
					failed++;
				} else {
					n += move;	/* list shuffled forward, or not */
					m--;		/* m+1th item now at m */
				}
			}
		}
	}
	if(failed)
		fprintf(stdout,"WARNING: %d Items failed to be separated.\n",failed);
	print_numbers();
}


/***************************** COUNT_ITEMS ****************************/

void count_items(char *filename)
{
	char *p;
 	char temp[10000];
	cnt = 0;
	if((fp[0] = fopen(filename,"r"))==NULL) {
		fprintf(stdout,"ERROR: Cannot open infile %s\n",filename);
		fflush(stdout);
		exit(1);
	}
	while(fgets(temp,200,fp[0])!=NULL) {
		p = temp;
		if(ro=='l') {
			cnt++;
		} else {
 		   while(strgetfloat(&p,&factor)) {
				switch(condit) {
				case(0):   cnt++;   break;
				case('>'): if(factor>thresh) cnt++; break;
				case('<'): if(factor<thresh) cnt++; break;
 				}
			}
		}
	}
	fclose(fp[0]);
	sprintf(errstr,"%d items",cnt);
	if(condit) {
		sprintf(temp," %c %lf",condit,thresh);
		strcat(errstr,temp);
	}
	strcat(errstr,"\n");
	if(!sloom && !sloombatch)
		fprintf(stdout,"%s\n",errstr);
	else
		fprintf(stdout,"WARNING: %s\n",errstr);
	fflush(stdout);
}

/**************************** ACCEL_TIME_SEQ ******************/

void accel_time_seq(void)
{
	int n; 
	double q;
	if(cnt<2 || cnt>3) {
		fprintf(stdout,"ERROR: Input file must have either 2 or 3 vals.\n");
		fflush(stdout);
		exit(1);
	}
	if(cnt==3)	q = number[2];			/* q = starttime */
	else		q = 0.0;
	cnt = timevents(0.0,factor,number[0],number[1]);
	for(n=0;n<cnt;n++)
		do_valout(pos[n] + q);
	fflush(stdout);
}

/**************************** ACCEL_DURATIONS *****************************/

void accel_durations(void)
{
	int n;
	if(cnt!=2) {
		fprintf(stdout,"ERROR: Input file must have 2 vals.\n");
		fflush(stdout);
		exit(1);
	}
	cnt = timevents(0.0,factor,number[0],number[1]);
	do_valout(number[0]);
	for(n=1;n<cnt;n++)
		do_valout(pos[n]-pos[n-1]);
	fflush(stdout);
}


/************************ SEPARATE_COLUMNS_TO_FILES *********************/

void separate_columns_to_files(void)
{
	int n, m, k;
	if((firstcnt = cnt/outfilecnt)*outfilecnt!=cnt) {
		fprintf(stdout,
		"ERROR: Number of vals in input file does not divide exactly %d outfiles\n",outfilecnt);
		fflush(stdout);
		exit(1);
	}
	for(n=0;n<outfilecnt;n++) {
		sprintf(temp,"%d",n);
		strcpy(thisfilename,filename);
		strcat(thisfilename,"_");
		strcat(thisfilename,temp);
		strcat(thisfilename,".txt");
		if(!sloom) {
			if((fp[1] = fopen(thisfilename,"w"))==NULL) {
				fprintf(stdout,"ERROR: Cannot open file %s to write.\n",thisfilename);
				fflush(stdout);
				exit(1);
			} else {
				fprintf(stdout,"Writing to file %s\n",thisfilename);
	 		}
		}
		k = n;
		for(m=0;m<firstcnt;m++) {
			do_valout(number[k]);
			k += outfilecnt;
		}
		if(!sloom)
			fclose(fp[1]);
	}
	fflush(stdout);
}


/************************* PARTITION_VALUES_TO_FILES ********************/

void partition_values_to_files(void)
{
	int n,m,k;
	firstcnt   = cnt/outfilecnt;
	if(firstcnt * outfilecnt!= cnt)
		firstcnt++;
	k = 0;
	for(n=0;n<outfilecnt;n++) {
		sprintf(temp,"%d",n);
		strcpy(thisfilename,filename);
		strcat(thisfilename,".");
		strcat(thisfilename,temp);
		if(!sloom) {
			if((fp[1] = fopen(thisfilename,"w"))==NULL) {
				fprintf(stdout,"ERROR: Cannot open file %s to write.\n",thisfilename);
				fflush(stdout);
				exit(1);
			} else {
				fprintf(stdout,"Writing to file %s\n",thisfilename);
			}
		}
		if(n==outfilecnt-1)
		firstcnt = cnt - k;
		for(m=0;m<firstcnt;m++,k++)
			do_valout(number[k]);
		fflush(stdout);
		if(!sloom)
			fclose(fp[1]);
	}
}

/***************** ROTATE_PARTITION_VALUES_TO_FILES ********************/

void rotate_partition_values_to_files(void)
{	
	int n,m,k;
	firstcnt   = cnt/outfilecnt;
	if(firstcnt * outfilecnt!= cnt)
		firstcnt++;
	k = 0;
	for(n=0;n<outfilecnt;n++) {
		sprintf(temp,"%d",n);
		strcpy(thisfilename,filename);
		strcat(thisfilename,".");
		strcat(thisfilename,temp);
		if(!sloom) {
			if((fp[1] = fopen(thisfilename,"w"))==NULL) {
				fprintf(stdout,"ERROR: Cannot open file %s to write.\n",thisfilename);
				fflush(stdout);
				exit(1);
			} else {
				fprintf(stdout,"Writing to file %s\n",thisfilename);
			}
		}
		if(n==outfilecnt-1)
			firstcnt = cnt - k;
		for(m=n;m<cnt;m+=outfilecnt)
			do_valout(number[m]);
		fflush(stdout);
		if(!sloom)		
			fclose(fp[1]);
	}
}

/************************* JOIN_FILES_AS_COLUMNS ************************/

void join_files_as_columns(char *filename)
{
	int n, m;
	char temp[64];
	if(!sloom) {
		if((fp[0] = fopen(filename,"w"))==NULL) {
			fprintf(stdout,"Cannot reopen infile1 to write data.\n");
			fflush(stdout);
			exit(1);
		}
	}
	errstr[0] = ENDOFSTR;
	for(n=0;n<firstcnt;n++) {
		for(m=0;m<infilecnt-1;m++) {
	 		sprintf(temp,"%.5lf ",number[n + (firstcnt * m)]);
			strcat(errstr,temp);
		}
   		sprintf(temp,"%.5lf ",number[n + (firstcnt * m)]);
		strcat(errstr,temp);
		if(!sloom)
			fprintf(fp[0],"%s\n",errstr);
		else
			fprintf(stdout,"INFO: %s\n",errstr);
		errstr[0] = ENDOFSTR;
	}
	fflush(stdout);
}

/************************* JOIN_FILES_AS_ROWS ************************/

void join_files_as_rows(void)
{
	int n, m, locnt = cnt/colcnt, totcnt = (cnt + stringscnt)/colcnt;
	char temp[64];
	errstr[0] = ENDOFSTR;
	for(n=0;n<ifactor;n++) {
		sprintf(errstr,"INFO: ");
		for(m = 0; m < colcnt-1; m++) {
	   		sprintf(temp,"%s ",strings[(n * colcnt) + m]);
			strcat(errstr,temp);
		}
   		sprintf(temp,"%s",strings[(n * colcnt) + m]);
		strcat(errstr,temp);
		fprintf(stdout,"%s\n",errstr);
	}
	for(n=locnt;n<totcnt;n++) {
		sprintf(errstr,"INFO: ");
		for(m = 0; m < colcnt-1; m++) {
	   		sprintf(temp,"%s ",strings[(n * colcnt) + m]);
			strcat(errstr,temp);
		}
   		sprintf(temp,"%s",strings[(n * colcnt) + m]);
		strcat(errstr,temp);
		fprintf(stdout,"%s\n",errstr);
	}

	for(n=ifactor;n<locnt;n++) {
		sprintf(errstr,"INFO: ");
		for(m = 0; m < colcnt-1; m++) {
	   		sprintf(temp,"%s ",strings[(n * colcnt) + m]);
			strcat(errstr,temp);
		}
   		sprintf(temp,"%s",strings[(n * colcnt) + m]);
		strcat(errstr,temp);
		fprintf(stdout,"%s\n",errstr);
	}
	fflush(stdout);
}

/************************* JOIN_MANY_FILES_AS_ROWS ************************/

void join_many_files_as_rows(void)
{
	int i, n, m, rowcnt, bas;
	char temp[200];

	bas = 0;
	for(i = 0;i < infilecnt; i++) {			/* for each file */
		rowcnt = cntr[i]/colcnt;			/*	Number of rows in file */
		for(n=0;n<rowcnt;n++) {				/*	for each row in file */
			sprintf(errstr,"INFO: ");
			for(m=0;m<colcnt;m++) {			/* foreach column in row */
				sprintf(temp,"%s ",strings[bas + (n * colcnt) + m]);
				strcat(errstr,temp);
			}
			fprintf(stdout,"%s\n",errstr);
		}
		bas += cntr[i];						/* set bas to start of next file's numbers */
	}
	fflush(stdout);
}

/************************* JOIN_MANY_FILES_AS_COLUMNS ************************/


void join_many_files_as_columns(char *filename,int insert)
{
	int i, n, m, rowcnt, bas;
	char temp[200], *p;

	if(!sloom) {
		if((fp[0] = fopen(filename,"w"))==NULL) {
			fprintf(stdout,"Cannot reopen infile1 to write data.\n");
			fflush(stdout);
			exit(1);
		}
	}

	rowcnt = colcnt; 	/* i.e. 'colcnt' has been used to store count-of-rows, not of columns, in this case */	

	if(insert) {
		if(ifactor > cntr[0]/rowcnt) {
			fprintf(stdout,"ERROR: Only %d columns in the first file. Can't do insert after column %d\n",cntr[0]/colcnt,ifactor);
			fflush(stdout);
			exit(1);
		}
		for(n=0;n<rowcnt;n++) {					/*	for each row in files */
			p = errstr;
			if(sloom)
				sprintf(errstr,"INFO: ");
			else
				sprintf(errstr,"");

			colcnt = cntr[0]/rowcnt;		/*	Number of columns in file */
			for(m=0;m<ifactor;m++) {			/* foreach column in row */
				sprintf(temp,"%s ",strings[(n * colcnt) + m]);
				strcat(errstr,temp);
			}
			colcnt = cntr[1]/rowcnt;		/*	Number of columns in file1 */
			for(m=0;m<colcnt;m++) {			/* foreach column in row */
				sprintf(temp,"%s ",strings[cntr[0] + (n * colcnt) + m]);
				strcat(errstr,temp);
			}
			colcnt = cntr[0]/rowcnt;		/*	Number of columns in file */

			for(m=ifactor;m<colcnt;m++) {			/* foreach column in row */
				sprintf(temp,"%s ",strings[(n * colcnt) + m]);
				strcat(errstr,temp);
			}

			fprintf(stdout,"%s\n",p);
			if(!sloom)
				fprintf(fp[0],"%s\n",p);
		}
	} else {
		for(n=0;n<rowcnt;n++) {					/*	for each row in files */
			bas = 0;
			p = errstr;
			if(sloom)
				sprintf(errstr,"INFO: ");
			else
				sprintf(errstr,"");
			for(i = 0;i < infilecnt; i++) {		/* for each file */
				colcnt = cntr[i]/rowcnt;		/*	Number of columns in file */
				for(m=0;m<colcnt;m++) {			/* foreach column in row */
					sprintf(temp,"%s ",strings[bas + (n * colcnt) + m]);
					strcat(errstr,temp);
				}
				bas += cntr[i];					/* set bas to start of next file's numbers */
			}
			fprintf(stdout,"%s\n",p);
			if(!sloom)
				fprintf(fp[0],"%s\n",p);
		}
	}
	fflush(stdout);
}

/************************ CONCATENATE_FILES ****************************/

void concatenate_files(char *filename)
{
	int n;
	if(!sloom) {
		if((fp[0] = fopen(filename,"w"))==NULL) {
			fprintf(stdout,"Cannot reopen infile1 to write data.\n");
			fflush(stdout);
			exit(1);
		}
	}
	for(n=0;n<cnt;n++) {
		if(!sloom)
			fprintf(fp[0],"%lf\n",number[n]);
		else
			fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
} 

/************************ CONCATENATE_FILES_CYCLICALLY ****************************/

void concatenate_files_cyclically(char *filename)
{
	int n, m;
	if(!sloom) {
		if((fp[0] = fopen(filename,"w"))==NULL) {
			fprintf(stdout,"Cannot reopen infile1 to write data.\n");
			fflush(stdout);
			exit(1);
		}
	}
	for(n=0;n<firstcnt;n++) {
		for(m=n;m<cnt;m+=firstcnt) {
			if(!sloom)
				fprintf(fp[0],"%lf\n",number[m]);
			else
				fprintf(stdout,"INFO: %lf\n",number[m]);
		}
	}
	fflush(stdout);
} 

/*********************** VALS_END_TO_END_IN_2_COLS *******************/

void vals_end_to_end_in_2_cols(void)
{
	int n;
	for(n=1;n<cnt;n++) {
		if(n==cnt-1)
		factor = 0.0;
		if(!sloom && !sloombatch)
			fprintf(fp[1],"%lf   %lf\n",number[n-1],number[n] + factor);
		else
			fprintf(stdout,"INFO: %lf   %lf\n",number[n-1],number[n] + factor);
	}
	fflush(stdout);
}

/****************************** QUANTISE ****************************/

void quantise(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		if(number[n]>0.0)
			ifactor = round(number[n]/factor);
		else
/* IS TRUNCATION OK */
	 	   ifactor = (int)((number[n]/factor) - 0.5 + VERY_TINY); /* TRUNCATION */
		number[n] = (double)ifactor * factor;
		if(!sloom && !sloombatch)		
			fprintf(fp[1],"%lf\n",number[n]);
		else
			fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}

/************************* ELIMINATE_EQUIVALENTS **********************/

void eliminate_equivalents(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		if(flteq(number[n],factor)) {
			eliminate(n);
			n--;
		}
	}
}

/************************** ELIMINATE_EVEN_ITEMS *********************/

void eliminate_even_items(void)
{
	int n,m;
	if(cnt==2) {
		cnt--;
		return;
	}
	for(n=2,m=1;n<cnt;n+=2,m++)
		number[m] = number[n];
	cnt = m;
}

/************************** ELIMINATE_GREATER_THAN ********************/

void eliminate_greater_than(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		if(number[n] > factor) {
			eliminate(n);
			n--;
		}
	}
}

/************************** ELIMINATE_LESS_THAN ***********************/

void eliminate_less_than(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		if(number[n] < factor) {
			eliminate(n);
			n--;
		}
	}
}

/*************************** ELIMINATE_DUPLICATES *************************/

void eliminate_duplicates(void)
{
	int m,n;
	for(n=0;n<cnt-1;n++) {
		for(m=n+1;m<cnt;m++) {
			if(number[m] >= (number[n] - factor) && number[m] <= (number[n] + factor)) {
				eliminate(m);
				m--;
			}
		}
	}
}

/**************************** REDUCE_TO_BOUND ***************************/

void reduce_to_bound(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		if(number[n] > factor)
			number[n] = factor;
	}
}

/************************* INCREASE_TO_BOUND *******************************/

void increase_to_bound(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		if(number[n] < factor)
			number[n] = factor;
	}
}

/***************************** GREATEST *******************************/

void greatest(void)
{
	int n;
	factor = number[0];
	for(n=1;n<cnt;n++) {
		if(number[n] > factor)
			factor = number[n];
	}
	if(!sloom && !sloombatch)		
		fprintf(fp[1],"%lf\n",factor);
	else
		fprintf(stdout,"ERROR: %lf\n",factor);
	fflush(stdout);
}

/****************************** LEAST *****************************/

void least(void)
{
	int n;
	factor = number[0];
	for(n=1;n<cnt;n++) {
		if(number[n] < factor)
		factor = number[n];
	}
	if(!sloom && !sloombatch)
		fprintf(fp[1],"%lf\n",factor);
	else
		fprintf(stdout,"WARNING: %lf\n",factor);
	fflush(stdout);
}

/***************************** MULTIPLY ************************/

void multiply(int rounded)
{
	int n;
	for(n=0;n<cnt;n++) {
		switch(condit) {
		case(0):
			number[n] *= factor;
			break;
		case('>'):
			if(number[n]>thresh)
				number[n] *= factor;
			break;
		case('<'):
			if(number[n]<thresh)
				number[n] *= factor;
			break;
		}
		if(rounded)
			number[n] = (double)round(number[n]);
		if(!sloom && !sloombatch)
			fprintf(fp[1],"%lf\n",number[n]);
		else
			fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}

/**************************** MARK_GREATER_THAN ************************/

void mark_greater_than(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		if(number[n] > factor) {
			if(!sloom && !sloombatch)
				fprintf(fp[1],"*%lf\n",number[n]);
			else
				fprintf(stdout,"INFO: *%lf\n",number[n]);
		} else {
			if(!sloom && !sloombatch)
				fprintf(fp[1],"%lf\n",number[n]);
			else
				fprintf(stdout,"INFO: %lf\n",number[n]);
		}
	}
	fflush(stdout);
}

/**************************** MARK_LESS_THAN ************************/

void mark_less_than(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		if(number[n] < factor) {
			if(!sloom && !sloombatch)
				fprintf(fp[1],"*%lf\n",number[n]);
			else
				fprintf(stdout,"INFO: *%lf\n",number[n]);
		} else {
			if(!sloom && !sloombatch)
				fprintf(fp[1],"%lf\n",number[n]);
			else
				fprintf(stdout,"INFO: %lf\n",number[n]);
		}
	}
	fflush(stdout);
}

/**************************** MARK_MULTIPLES ************************/

void mark_multiples(void)
{
	int n;
	int k;
	double z;
	if(!condit)
		thresh = 0.0;
	if(thresh < 0.0)
		thresh = -thresh;
	for(n=0;n<cnt;n++) {
		k = (int)round(number[n]/factor);
		z = (double)k * factor;
		if((z <= (number[n] + thresh)) && (z >= (number[n] - thresh)))
			fprintf(stdout,"INFO: *%lf\n",number[n]);
		else
			fprintf(stdout,"INFO: %lf\n",number[n]);
	}
	fflush(stdout);
}

/****************************** MINOR_TO_MAJOR **************************/

void minor_to_major(void)
{
	int n;
	int m3 = (3 + ifactor)%12;	/* MIDI location of minor 3rd */
	int m6 = (8 + ifactor)%12;	/* MIDI location of minor 6th */
	int m7 = (10 + ifactor)%12;   /* MIDI location of minor 7th */
	for(n=0;n<cnt;n++) {
		factor = fmod(number[n],TWELVE);
		if(flteq(factor,(double)m3)
		|| flteq(factor,(double)m6)
		|| flteq(factor,(double)m7))
			number[n] += 1.0;
		do_valout(number[n]);
	}
	fflush(stdout);
}

/****************************** ADD ******************************/

void add(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		switch(condit) {
		case(0):
			number[n] += factor;
			break;
		case('>'):
			if(number[n]>thresh)
			   	number[n] += factor;
			break;
		case('<'):
			if(number[n]<thresh)
			   	number[n] += factor;
			break;
		}
		do_valout(number[n]);
	}
	fflush(stdout);
}

/**************************** TAKE_POWER *************************/

void take_power(void)
{
	int n, is_neg;
	for(n=0;n<cnt;n++) {
		is_neg = 0;
		switch(condit) {
		case(0):
			if(fabs(number[n]) < FLTERR)
				number[n] = 0;
			else {
				if (number[n] < 0) {
					is_neg = 1;
					number[n] = -number[n];
				}
		 	   	number[n] = pow(number[n],factor);
			}
			break;
		case('>'):
			if(number[n]>thresh) {
				if(fabs(number[n]) < FLTERR)
					number[n] = 0;
				else {
					if (number[n] < 0) {
						is_neg = 1;
						number[n] = -number[n];
					}
					number[n] = pow(number[n],factor);
				}
			}
			break;
		case('<'):
			if(number[n]<thresh) {
				if(fabs(number[n]) < FLTERR)
					number[n] = 0;
				else {
					if (number[n] < 0) {
						is_neg = 1;
						number[n] = -number[n];
					}
					number[n] = pow(number[n],factor);
				}
			}
			break;
		}
		if(is_neg)
			number[n] = -number[n];
		do_valout(number[n]);
	}
	fflush(stdout);
}

/*********************** TEMPER_MIDI_DATA *********************/

void temper_midi_data(void)
{
	int n, octaves;
	double step, q, offset = 0.0, maxm, thisval, outval=0.0, diff, thisdiff;
	if(condit) {
		offset = thresh - floor(thresh);
		if(offset > 0.5)
			offset = 1.0 - offset;
	}
	if(flteq(factor,TWELVE) && offset == 0.0) {
		for(n=0;n<cnt;n++) {
			ifactor = round(number[n]);
			number[n] = ifactor;
			do_valout(number[n]);
		}
	} else {
		if(offset == 0.0) {
			step = TWELVE/factor;		
			for(n=0;n<cnt;n++) {
				q = fmod(number[n],TWELVE);	/* which semitone in 8va */
				q /= step;			/* which newtempered step */
				ifactor = round(q);		/* round */
				q = ifactor * step;		/* which semitone is this */
				octaves = (int)(number[n]/12.0);   /* TRUNCATE */	/* which octave */
				number[n] = (octaves * TWELVE) + q;
				do_valout(number[n]);
			}
		} else {
			step = TWELVE/factor;
			maxm = -1.0;
			for(n=0;n<cnt;n++) {
				if(number[n] > maxm)
					maxm = number[n];
			}
			for(n=0;n<cnt;n++) {
				thisval = offset;
				diff = HUGE;
				while(thisval <= maxm) {
					thisdiff = fabs(number[n] - thisval);
					if(thisdiff < diff) {
						diff = thisdiff;
						outval = thisval;
					}
					thisval += step;
				}
				do_valout(outval);
			}
		}
	}
	fflush(stdout);
}

/*************************** TEMPER_HZ_DATA ******************************/

void temper_hz_data(void)
{
	int n, octaves;
	double q, this_reffrq, minf = HUGE, reffrq;
	if(condit) {
		reffrq = thresh;
		for(n=0;n<cnt;n++) {
			if(number[n] < minf)
				minf = number[n];
		}
		while(reffrq > minf)
			reffrq /= 2.0;
	} else
		reffrq = C_HZ;
	for(n=0;n<cnt;n++) {		   /* factor is tempering number */
		if(number[n] < 0.0) {
			do_valout(number[n]);	/* Retain any (subzero) flags that arrive */
			continue;
		}
		q		= number[n]/reffrq;		 /* frq ratio withref frq */
		octaves  = (int)floor(log(q)/LOG_2_TO_BASE_E); /* Number of 8vas (trunc) */
		this_reffrq   = reffrq*pow(2.0,(double)octaves); /* C blw actualpch */
		ifactor  = round(factor*((log(number[n])-log(this_reffrq))/LOG_2_TO_BASE_E));
	  		  				 /* No. tempered steps, rounded */
		number[n]= this_reffrq * pow(2.0,(double)ifactor/factor);
		do_valout(number[n]);
	}
	fflush(stdout);
}

/*************************** JUST_INTONATION_HZ ******************************/

void just_intonation_Hz(void)
{
	double *refset, reffrq, minf, maxf, minratio, oct, refval, outval = 0.0, q;
	int n, m;
	refset = (double *)exmalloc(12 * sizeof(double));
	minf  = number[0];
	maxf  = number[0];
	for(n=1;n<cnt;n++) {
		minf = min(minf,number[n]);
		maxf = max(maxf,number[n]);
	}
	reffrq = factor;
	while(reffrq > minf)
		reffrq /= 2.0;
	refset[0] =  reffrq;
	refset[1] =  reffrq * 135.0 / 128.0;
	refset[2] =  reffrq *   9.0 / 8.0;
	refset[3] =  reffrq *   6.0 / 5.0;
	refset[4] =  reffrq *   5.0 / 4.0;
	refset[5] =  reffrq *   4.0 / 3.0;
	refset[6] =  reffrq *  45.0 / 32.0;
	refset[7] =  reffrq *   3.0 / 2.0;
	refset[8] =  reffrq *   5.0 / 3.0;
	refset[9] =  reffrq *  27.0 / 16.0;
	refset[10] = reffrq *   9.0 / 5.0;
	refset[11] = reffrq *  15.0 / 8.0;
	for(n=0;n<cnt;n++) {
		minratio = HUGE;
		oct = 1.0;
		while(refset[0] * oct < maxf * 2.0) {
			for(m = 0; m < 12; m ++) {
				refval = refset[m] * oct;
				q = number[n]/refval;
				if(q < 1.0)
					q = 1.0/q;
				if(q < minratio) {
					minratio = q;
					outval = refval;
				}
			}
			oct *= 2.0;
		}
		do_valout(outval);
	}
}

/*************************** CREATE_JUST_INTONATION_HZ ******************************/

void create_just_intonation_Hz(void)
{
	double *refset, reffrq, minf, maxf, oct, outval;
	int m, OK;
	refset = (double *)exmalloc(12 * sizeof(double));
	minf  = number[0];
	maxf  = number[0];
	minf = min(number[1],number[2]);
	maxf = max(number[1],number[2]);
	reffrq = number[0];
	while(reffrq > minf)
		reffrq /= 2.0;
	refset[0] =  reffrq;
	refset[1] =  reffrq * 135.0 / 128.0;
	refset[2] =  reffrq *   9.0 / 8.0;
	refset[3] =  reffrq *   6.0 / 5.0;
	refset[4] =  reffrq *   5.0 / 4.0;
	refset[5] =  reffrq *   4.0 / 3.0;
	refset[6] =  reffrq *  45.0 / 32.0;
	refset[7] =  reffrq *   3.0 / 2.0;
	refset[8] =  reffrq *   5.0 / 3.0;
	refset[9] =  reffrq *  27.0 / 16.0;
	refset[10] = reffrq *   9.0 / 5.0;
	refset[11] = reffrq *  15.0 / 8.0;
	OK = 1;
	oct = 1.0;
	while(OK) {
		for(m = 0; m < 12; m ++) {
			outval = refset[m] * oct;
			if(outval >= minf) {
				if(outval <= maxf) 
					do_valout(outval);
				else
					OK = 0;
			}
		}
		if(!OK)
			break;
		oct *= 2.0;
	}
}

/*************************** CREATE_JUST_INTONATION_MIDI ******************************/

void create_just_intonation_midi(void)
{
	double *refset, refmidi, reffrq, minf, maxf, oct, outval;
	int m, OK;
	refset = (double *)exmalloc(12 * sizeof(double));
	minf  = number[0];
	maxf  = number[0];
	minf = min(number[1],number[2]);
	maxf = max(number[1],number[2]);
	refmidi = number[0];
	reffrq = miditohz(refmidi);
	while(refmidi > minf) {
		refmidi -= 12.0;
		reffrq /= 2.0;
	}
	refset[0] =  refmidi;
	refset[1] =  hztomidi(reffrq * 135.0 / 128.0);
	refset[2] =  hztomidi(reffrq *   9.0 / 8.0);
	refset[3] =  hztomidi(reffrq *   6.0 / 5.0);
	refset[4] =  hztomidi(reffrq *   5.0 / 4.0);
	refset[5] =  hztomidi(reffrq *   4.0 / 3.0);
	refset[6] =  hztomidi(reffrq *  45.0 / 32.0);
	refset[7] =  hztomidi(reffrq *   3.0 / 2.0);
	refset[8] =  hztomidi(reffrq *   5.0 / 3.0);
	refset[9] =  hztomidi(reffrq *  27.0 / 16.0);
	refset[10] = hztomidi(reffrq *   9.0 / 5.0);
	refset[11] = hztomidi(reffrq *  15.0 / 8.0);
	OK = 1;
	oct = 0.0;
	while(OK) {
		for(m = 0; m < 12; m ++) {
			outval = refset[m] + oct;
			if(outval >= minf) {
				if(outval <= maxf) 
					do_valout(outval);
				else
					OK = 0;
			}
		}
		if(!OK)
			break;
		oct += 12.0;
	}
}

/*************************** just_intonation_midi ******************************/

void just_intonation_midi(void)
{
	double *refset, refmidi, reffrq, minmidi, maxmidi, minstep, oct, refval, outval = 0.0, q;
	int n, m;
	refset = (double *)exmalloc(12 * sizeof(double));
	minmidi  = number[0];
	maxmidi  = number[0];
	for(n=1;n<cnt;n++) {
		minmidi = min(minmidi,number[n]);
		maxmidi = max(maxmidi,number[n]);
	}
	refmidi = factor;
	while(refmidi > minmidi)
		refmidi -= 12.0;
	reffrq = miditohz(refmidi);
	refset[0] =  refmidi;
	refset[1] =  hztomidi(reffrq * 135.0 / 128.0);
	refset[2] =  hztomidi(reffrq *   9.0 / 8.0);
	refset[3] =  hztomidi(reffrq *   6.0 / 5.0);
	refset[4] =  hztomidi(reffrq *   5.0 / 4.0);
	refset[5] =  hztomidi(reffrq *   4.0 / 3.0);
	refset[6] =  hztomidi(reffrq *  45.0 / 32.0);
	refset[7] =  hztomidi(reffrq *   3.0 / 2.0);
	refset[8] =  hztomidi(reffrq *   5.0 / 3.0);
	refset[9] =  hztomidi(reffrq *  27.0 / 16.0);
	refset[10] = hztomidi(reffrq *   9.0 / 5.0);
	refset[11] = hztomidi(reffrq *  15.0 / 8.0);
	for(n=0;n<cnt;n++) {
		minstep = HUGE;
		oct = 0.0;
		while(refset[0] + oct < maxmidi + 12.0) {
			for(m = 0; m < 12; m ++) {
				refval = refset[m] + oct;
				q = fabs(number[n] - refval);
				if(q < minstep) {
					minstep = q;
					outval = refval;
				}
			}
			oct += 12.0;
		}
		do_valout(outval);
	}
}

/************************* TIME_FROM_CROTCHET_COUNT *********************/

void time_from_crotchet_count(void)
{
	int n;
	factor = 60.0/factor; /* factor becomes duration of crotchet */
	for(n=0;n<cnt;n++)
		do_valout(number[n]*factor);
	fflush(stdout);
}

/********************* TIME_FROM_BEAT_LENGTHS ***********************/

void time_from_beat_lengths(void)
{
	int n;
	double sum;
	factor = 60.0/factor; /* factor becomes duration of crotchet */
	sum = 0.0;
	for(n=0;n<cnt;n++) {
		do_valout(sum);
		sum += number[n] * factor;		
	}
	fflush(stdout);
}

/****************************** TOTAL ******************************/

void total(void)
{   
	int n;
	double sum = 0.0;
	for(n=0;n<cnt;n++)
		sum += number[n];
	do_valout_as_message(sum);
	fflush(stdout);
}

/***************************** TEXT_TO_HZ **************************/

void text_to_hz(void)
{
	int n;
	for(n=0;n<cnt;n++)
		do_valout(miditohz(number[n]));
	fflush(stdout);
}

/***************************** GENERATE_HARMONICS **************************/

void generate_harmonics(void)
{
	int n;
	for(n=1;n<=ifactor;n++)
		do_valout(number[n] * (double)n);
	fflush(stdout);
}

/***************************** GROUP_HARMONICS **************************/

#define SEMIT_UP	(1.05946309436)	
#define LOG2(x)		(log(x)/log(2))

void group_harmonics(void)
{
	int n, m, got_it, samecnt;
	int j, hno, bigg, smal, k;
	double thisintv, thisnum;
	double **hgrp, semit_up, semit_dn;
	int	   *hgrpcnt;
	if(factor<0.0)
		factor = -factor;
	semit_up = pow(SEMIT_UP,factor);
	semit_dn = 1.0/semit_up;

  	for(n=0;n<cnt-1;n++) {	   /* eliminate zeros */
		if(number[n]<=0.0) {
			fprintf(stdout,"ERROR: zero or subzero frquency in list: cannot proceed.\n");
			fflush(stdout);
			exit(1);
		}
	}
  	for(n=1;n<cnt;n++) {	   /* Sort list into ascending order */
		thisnum  = number[n];
		m = n-1;
		while(m >= 0 && number[m] > thisnum) {
			number[m+1]  = number[m];
			m--;
		}
		number[m+1]  = thisnum;
	}
 	for(n=0;n<cnt-1;n++) {	   /* Eliminate duplicate frequencies */
		for(m=n+1;m<cnt;m++) {
			if(flteq(number[n],number[m])) {
				for(j=m+1;j<cnt;j++)
					number[j-1] = number[j];
				cnt--;
				m--;
			}
		}
	}
	hgrp	= (double **)exmalloc(cnt * sizeof(double *));
	hgrpcnt = (int *)exmalloc(cnt * sizeof(int));
	for(n=0;n<cnt;n++) {	 	/* For all remaining numbers */
		hgrp[n]  = (double *)exmalloc(sizeof(double));
		hgrp[n][0] = number[n];			/* Establish space to store, and to count, harmonic group */
		hgrpcnt[n] = 1;
		for(m=n+1;m<cnt;m++) {   			/* Take each higher frq in list */
   			got_it = 0;
			thisintv = number[m]/number[n];
			hno	  = round(thisintv);
			thisintv = number[n]/(number[m]/(double)hno);
			if(thisintv < semit_up && thisintv > semit_dn) {
				hgrpcnt[n]++;
				hgrp[n] = (double *)exrealloc((char *)hgrp[n],hgrpcnt[n] * sizeof(double));
				hgrp[n][hgrpcnt[n]-1] = number[m];
			}
		}
	}

	for(n=0;n<cnt-1;n++) {
  		for(m=n+1;m<cnt;m++) {
			samecnt = 0;
			if(hgrpcnt[n] >= hgrpcnt[m]) {
				bigg = n;
				smal = m;
			} else {
				bigg = m;
				smal = n;
			}
			for(k=0;k<hgrpcnt[smal];k++) {
				j = 0;
				for(;;) {
					if(hgrp[smal][k] == hgrp[bigg][j]) {
						samecnt++;
						break;
					} else {
						if(++j >=hgrpcnt[bigg])
							break;
					}
				}
			}
			if(samecnt==hgrpcnt[smal]) {
				if(bigg==n) {
					free(hgrp[m]);
					for(j = m+1;j<cnt;j++) {
						hgrp[j-1]	= hgrp[j];
						hgrpcnt[j-1] = hgrpcnt[j];
					}
					cnt--;
					m--;
				} else {
					free(hgrp[n]);
					for(j = n+1;j<cnt;j++) {
						hgrp[j-1]	= hgrp[j];
						hgrpcnt[j-1] = hgrpcnt[j];
					}
					cnt--;
					n--;
					break;
				}
			}
		}
	}
	for(n=0;n<cnt;n++) {
		for(m=0;m<hgrpcnt[n];m++)
			do_valout(hgrp[n][m]);
		if(!sloom && !sloombatch)
			fprintf(fp[1],"\n");
		else
			fprintf(stdout,"INFO: \n");
		free(hgrp[n]);
	}
	fflush(stdout);
	free(hgrp);
	free(hgrpcnt);
}


/***************************** GET_HARMONIC_ROOTS **************************/

void get_harmonic_roots(void)
{
	int n;
	for(n=1;n<=ifactor;n++)
		do_valout(number[n]/(double)n);
	fflush(stdout);
}

 /************************* RANK_VALS *****************************/

 void rank_vals(void)
 {
 	int m, n;
	double hibnd, lobnd;
 	int *poll = (int *)exmalloc(cnt * sizeof(int));
	for(n=0;n<cnt;n++)
		poll[n] = 1;
	for(n=0;n<cnt-1;n++) {
		if(poll[n] > 0) {
			hibnd = number[n] + factor;
			lobnd = number[n] - factor;
			for(m=n+1;m<cnt;m++) {
				if(poll[m] > 0) {
					if(number[m] <= hibnd && number[m] >= lobnd) {
						poll[n]++;
						poll[m] = -1;
					}
				}
			}
		}
	}
	for(n=0;n<cnt;n++) {
		if(poll[n]<=0) {
			for(m=n;m<cnt-1;m++) {
				number[m] = number[m+1];
				poll[m]   = poll[m+1];
			}
			n--;
			cnt--;
		}
	}
	sort_numbers(poll);
	for(n=0;n<cnt;n++)
		do_valout(number[n]);
 	fflush(stdout);
 }

 /************************* RANK_FRQS *****************************/

 #define TWELFTH_ROOT_OF_2 (1.059463094)

 void rank_frqs(void)
 {
 	int m, n;
	double hibnd, lobnd, one_over_factor;
 	int *poll = (int *)exmalloc(cnt * sizeof(int));
	for(n=0;n<cnt;n++)
		poll[n] = 1;
	factor = pow(TWELFTH_ROOT_OF_2,factor);
	one_over_factor = 1.0/factor;
	for(n=0;n<cnt-1;n++) {
		if(poll[n] > 0) {
			hibnd = number[n] * factor;
			lobnd = number[n] * one_over_factor;
			for(m=n+1;m<cnt;m++) {
				if(poll[m] > 0) {
					if(number[m] <= hibnd && number[m] >= lobnd) {
						poll[n]++;
						poll[m] = -1;
					}
				}
			}
		}
	}
	for(n=0;n<cnt;n++) {
		if(poll[n]<=0) {
			for(m=n;m<cnt-1;m++) {
				number[m] = number[m+1];
				poll[m]   = poll[m+1];
			}
			n--;
			cnt--;
		}
	}
	sort_numbers(poll);

	for(n=0;n<cnt;n++)
		do_valout(number[n]);
	fflush(stdout);
}

/************************* SORT_NUMBERS *****************************/

void sort_numbers(int *poll)
{
	int n, m, thispoll;
	double thisnum;
	for(n=1;n<cnt;n++) {
		thispoll = poll[n];
		thisnum  = number[n];
		m = n-1;
		while(m >= 0 && poll[m] < thispoll) {
			number[m+1]  = number[m];
			poll[m+1]	= poll[m];
			m--;
		}
		number[m+1]  = thisnum;
		poll[m+1]	= thispoll;
	}
}


/*********************** APPROX_VALS ********************/

void approx_vals(void)
{
	int z;
	int n;
	for(n=0;n<cnt;n++) {
		z = round(number[n]/factor);
		number[n] = (double)z * factor;
		do_valout(number[n]);
	}
	fflush(stdout);
}

/*********************** FLOOR_VALS ********************/

void floor_vals(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		if(number[n]<factor)
			number[n] = factor;
		do_valout(number[n]);
	}
	fflush(stdout);
}

/*********************** LIMIT_VALS ********************/

void limit_vals(void)
{
	int n;
	for(n=0;n<cnt;n++) {
		if(number[n]>factor)
			number[n] = factor;
		do_valout(number[n]);
	}
	fflush(stdout);
}

/****************************** NOTE_TO_MIDI ******************************/

void note_to_midi(int is_transpos)
{
	int n, oct, midi;
	char *p, *q;
	double qtone = 0.0, midiflt;
	for(n=0;n< stringscnt;n++) {
		p = strings[n];	
		switch(*p) {
		case('c'):	case('C'):	midi = 0;	break;
		case('d'):	case('D'):	midi = 2;	break;
		case('e'):	case('E'):	midi = 4;	break;
		case('f'):	case('F'):	midi = 5;	break;
		case('g'):	case('G'):	midi = 7;	break;
		case('a'):	case('A'):	midi = 9;	break;
		case('b'):	case('B'):  midi = 11;	break;
		default:
			fprintf(stdout,"ERROR: Unknown pitch value '%c' at note %d\n",*p,n+1);
			fflush(stdout);
			exit(1);
		}
		p++;
		switch(*p) {
		case('#'): midi = (midi+1)%12;	p++;	break;
		case('b'): midi = (midi-1)%12;	p++;	break;
		}
		q = p + strlen(p) - 1;
		qtone = 0.0;
		if(*q == '-') {
			qtone = -.5;
			*q = ENDOFSTR;
		} else if(*q == '+') {
			qtone = .5;
			*q = ENDOFSTR;
		}		
		if(sscanf(p,"%d",&oct)!=1) {
			fprintf(stdout,"ERROR: No octave value given at note %d\n",n+1);
			fflush(stdout);
			exit(1);
		}
		if(oct > 5 || oct < -5) {
			fprintf(stdout,"ERROR: octave value out of range (-5 to +5) at note %d\n",n+1);
			fflush(stdout);
			exit(1);
		}
		oct += 5;
		midi += (oct * 12);
		midiflt = (double)midi + qtone;
		if(is_transpos == 1)
			midiflt -= factor;
		else if(is_transpos == 2) /* to frq */
			midiflt = miditohz(midiflt);
		fprintf(stdout,"INFO: %lf\n",midiflt);			
	}
	fflush(stdout);
}

/****************************** APPEND_TEXT ******************************/

void append_text(int after)
{
	int n, k = strlen(temp);
	char temp2[64], *p;
	if(after) {
		for(n=0;n<stringscnt;n++) {
			sprintf(temp2,"%s",strings[n]);
			p = temp2 + strlen(temp2);
			sprintf(p,"%s",temp);
			fprintf(stdout,"INFO: %s\n",temp2);			
		}
	} else {
		sprintf(temp2,"%s",temp);
		p = temp2 + k;
		for(n=0;n<stringscnt;n++) {
			sprintf(p,"%s",strings[n]);
			fprintf(stdout,"INFO: %s\n",temp2);			
		}
	}

	fflush(stdout);
}

/****************************** KILL_TEXT ******************************/

void kill_text(int where)
{
	int n;
	char *p;
	switch(where) {
	case(0):	/* Before */
		for(n=0;n<stringscnt;n++) {
			p = strings[n];
			p += strlen(strings[n]);
			p--;
			while(isdigit(*p) || (*p == '.') || (*p == '-')) {
				p--;
				if(p < strings[n])
					break;
			}
			p++;
			if(p == strings[n] + strlen(strings[n])) {
				fprintf(stdout,"ERROR: Cannot find numeric ending of value.\n");
				fflush(stdout);
				exit(1);
			}
			fprintf(stdout,"INFO: %s\n",p);			
		}				
		break;
	case(1):	/* After */
		for(n=0;n<stringscnt;n++) {
			p = strings[n];
			p += strlen(strings[n]);
			p--;
			while(!isdigit(*p)) {
				p--;
				if(p < strings[n]) {
					fprintf(stdout,"ERROR: Cannot find numeric part of value.\n");
					fflush(stdout);
					exit(1);
				}
			}
			if(*(p+1) =='.')
				p++;
			p++;
			*p = ENDOFSTR;
			fprintf(stdout,"INFO: %s\n",strings[n]);			
		}				
		break;
	case(2):	/* Before & After */
		for(n=0;n<stringscnt;n++) {
			p = strings[n];
			p += strlen(strings[n]);
			p--;
			while(!isdigit(*p)) {
				p--;
				if(p < strings[n]) {
					fprintf(stdout,"ERROR: Cannot find numeric part of value.\n");
					fflush(stdout);
					exit(1);
				}
			}
			if(*(p+1) =='.')
				p++;
			p++;
			*p = ENDOFSTR;
			p--;
			while(isdigit(*p) || (*p == '.') || (*p == '-')) {
				p--;
				if(p < strings[n])
					break;
			}
			p++;
			fprintf(stdout,"INFO: %s\n",p);			
		}				
		break;
	}
	fflush(stdout);
}

/****************************** TARGETED_STRETCH ******************************/

void targeted_stretch(void)
{
	int n;
	double time_shoulder = number[cnt];
	double stretch = number[cnt+1], lastime;
	if(time_shoulder < 0.0) {
		fprintf(stdout,"ERROR: timestep (%lf) cannot be less than zero.\n",time_shoulder);
		fflush(stdout);
		exit(1);
	}
	if(number[0] < 0.0) {
		fprintf(stdout,"ERROR: First time value less than zero encountered.\n");
		fflush(stdout);
		exit(1);
	} else if(number[0] <= time_shoulder) {
		fprintf(stdout,"ERROR: First time is too close to zero\n");
		fflush(stdout);
		exit(1);
	}
	lastime = number[0];
	for(n = 1; n < cnt; n++) {
		if(ODD(n)) {
			if(number[n] <= lastime) {
				fprintf(stdout,"ERROR: Times out of sequence (between %lf and %lf).\n",number[n],lastime);
				fflush(stdout);
				exit(1);
			}
		} else {
			if(number[n] <= lastime + (2 * time_shoulder)) {
				fprintf(stdout,"ERROR: Too small time step encountered (between %lf and %lf).\n",number[n],lastime);
				fflush(stdout);
				exit(1);
			}
		}
		lastime = number[n];
	}

	if(flteq(number[n],0.0)) {
		fprintf(stdout,"INFO: 0.0  %lf\n",stretch);
	} else {
		fprintf(stdout,"INFO: 0.0  1.0\n");
		fprintf(stdout,"INFO: %lf  1.0\n",number[0] - time_shoulder);
		fprintf(stdout,"INFO: %lf  %lf\n",number[0],stretch);
	}
	for(n = 1; n < cnt; n++) {
		if(ODD(n)) {	/* already stretched */
			fprintf(stdout,"INFO: %lf  %lf\n",number[n],stretch);
			fprintf(stdout,"INFO: %lf  1.0\n",number[n] + time_shoulder);
		} else {		/* not stretched at the moment */
			fprintf(stdout,"INFO: %lf  1.0\n",number[n] - time_shoulder);
			fprintf(stdout,"INFO: %lf  %lf\n",number[n],stretch);
		}
	}
	lastime = max(100.0,2.0 * number[cnt-1]);
	if(ODD(cnt))
		fprintf(stdout,"INFO: %lf  %lf\n",lastime,stretch);
	else
		fprintf(stdout,"INFO: %lf  1.0\n",lastime);
	fflush(stdout);
}

/****************************** TARGETED_PAN ******************************/

void targeted_pan(void)
{
	int n;
	double time_shoulder = number[cnt];
	double panedj        = number[cnt+1];
	double start_panpos  = number[cnt+2];
	double end_panpos    = number[cnt+3];
	double end_pantime   = number[cnt+4];
	double lastime;
	double pan_subedj = panedj * (7.5/9.0);

	if(time_shoulder < 0.0) {
		fprintf(stdout,"ERROR: half lingertime (%lf) cannot be less than zero.\n",time_shoulder);
		fflush(stdout);
		exit(1);
	}
	if(number[0] < 0.0) {
		fprintf(stdout,"ERROR: First time value less than zero encountered.\n");
		fflush(stdout);
		exit(1);
	} else if(number[0] <= time_shoulder) {
		fprintf(stdout,"ERROR: First time is too close to zero\n");
		fflush(stdout);
		exit(1);
	}
	lastime = number[0];
	for(n = 1; n < cnt; n++) {
		if(number[n] <= lastime + (2 * time_shoulder)) {
			fprintf(stdout,"ERROR: Too small time step encountered (between %lf and %lf).\n",number[n],lastime);
			fflush(stdout);
			exit(1);
		}
	}
	if(end_pantime - time_shoulder <= number[cnt-1]) {
		fprintf(stdout,"ERROR: Too small time step encountered before end pantime (%lf to %lf).\n",number[cnt -1],end_pantime);
		fflush(stdout);
		exit(1);
	}
	if(flteq(number[0],0.0))
		fprintf(stdout,"INFO: 0.0  %lf\n",panedj);
	else {
		fprintf(stdout,"INFO: 0.0  %lf\n",start_panpos);
		fprintf(stdout,"INFO: %lf  %lf\n",number[0] - time_shoulder,pan_subedj);
		fprintf(stdout,"INFO: %lf  %lf\n",number[0],panedj);
		fprintf(stdout,"INFO: %lf  %lf\n",number[0] + time_shoulder,pan_subedj);
	}
	panedj = -panedj;
	pan_subedj = -pan_subedj;
	for(n = 1; n < cnt; n++) {
		fprintf(stdout,"INFO: %lf  %lf\n",number[n] - time_shoulder,pan_subedj);
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],panedj);
		fprintf(stdout,"INFO: %lf  %lf\n",number[n] + time_shoulder,pan_subedj);
		lastime = number[n];
		panedj = -panedj;
		pan_subedj = -pan_subedj;
	}
	fprintf(stdout,"INFO: %lf  %lf\n",end_pantime,end_panpos);
	fflush(stdout);
}

/******************************** DO_SQUEEZED_PAN ****************************/

void do_squeezed_pan(void) {
	int n, neg = 1;
	double time_shoulder = number[cnt];
	double start_panpos  = number[cnt+1];
	double end_panpos    = number[cnt+2];
	double end_pantime   = number[cnt+3];
	double lastime;
	double pan_subedj;

	if(time_shoulder < 0.0) {
		fprintf(stdout,"ERROR: half lingertime (%lf) cannot be less than zero.\n",time_shoulder);
		fflush(stdout);
		exit(1);
	}
	if(number[0] < 0.0) {
		fprintf(stdout,"ERROR: First time value less than zero encountered.\n");
		fflush(stdout);
		exit(1);
	} else if(number[0] <= time_shoulder) {
		fprintf(stdout,"ERROR: First time (%lf) is too close to zero for half-lingertime %lf\n",number[0],time_shoulder);
		fflush(stdout);
		exit(1);
	}
	lastime = number[0];
	for(n = 2; n < cnt; n+=2) {
		if(number[n] <= lastime + (2 * time_shoulder)) {
			fprintf(stdout,"ERROR: Too small time step encountered (between %lf and %lf).\n",number[n],lastime);
			fflush(stdout);
			exit(1);
		}
	}
	if(end_pantime - time_shoulder <= number[cnt-2]){
		fprintf(stdout,"ERROR: Too small time step encountered before end pantime (%lf to %lf).\n",number[cnt-2],end_pantime);
		fflush(stdout);
		exit(1);
	}
	if(flteq(number[0],0.0))
		fprintf(stdout,"INFO: 0.0  %lf\n",number[1]);
	else {
		pan_subedj = number[1] * (7.5/9.0); 
		fprintf(stdout,"INFO: 0.0  %lf\n",start_panpos);
		fprintf(stdout,"INFO: %lf  %lf\n",number[0] - time_shoulder,pan_subedj);
		fprintf(stdout,"INFO: %lf  %lf\n",number[0],number[1]);
		fprintf(stdout,"INFO: %lf  %lf\n",number[0] + time_shoulder,pan_subedj);
	}
	if(number[1] > 0.0)
		neg = -1;
	for(n = 2; n < cnt; n+=2) {
		number[n+1] *= neg;
		pan_subedj = number[n+1] * (7.5/9.0); 
		fprintf(stdout,"INFO: %lf  %lf\n",number[n] - time_shoulder,pan_subedj);
		fprintf(stdout,"INFO: %lf  %lf\n",number[n],number[n+1]);
		fprintf(stdout,"INFO: %lf  %lf\n",number[n] + time_shoulder,pan_subedj);
		neg = -neg;
	}
	fprintf(stdout,"INFO: %lf  %lf\n",end_pantime,end_panpos);
	fflush(stdout);
}	

/****************************** KILL_PATH ******************************/

void kill_path(void)
{
	int n;
	char *p;
	for(n=0;n<stringscnt;n++) {
		p = strings[n];
		p += strlen(strings[n]);
		p--;
		while((*p != '\\') && (*p != '/')) {
			p--;
			if(p < strings[n])
				break;
		}
		p++;
		fprintf(stdout,"INFO: %s\n",p);			
	}				
	fflush(stdout);
}

/****************************** KILL_EXTENSION ******************************/

void kill_extension(void)
{
	int n;
	char *p;
	for(n=0;n<stringscnt;n++) {
		p = strings[n];
		p += strlen(strings[n]);
		p--;
		while(*p != '.') {
			p--;
			if(p < strings[n]) {
				p++;
				break;
			}
		}
		if(p >strings[n])
			*p = ENDOFSTR;
		fprintf(stdout,"INFO: %s\n",strings[n]);			
	}				
	fflush(stdout);
}

/****************************** KILL_PATH ******************************/

void kill_path_and_extension(void)
{
	int n, gotext;
	char *p;
	for(n=0;n<stringscnt;n++) {
		p = strings[n];
		p += strlen(strings[n]);
		p--;
		gotext = 0;
		while(p >= strings[n]) {
			if(!gotext && (*p == '.')) {
				*p = ENDOFSTR;
				gotext = 1;
			}
			if(*p == '\\' || *p == '/') {
				p++;
				break;
			}
			p--;
		}
		if(strlen(p) <=0) {
			sprintf(errstr,"Invalid filename (%s) encountered: cannot complete this process\n",strings[n]);
			do_error();
		}
		fprintf(stdout,"INFO: %s\n",p);			
	}				
	fflush(stdout);
}

