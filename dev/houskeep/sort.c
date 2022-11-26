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



/* RWD floats version */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <globcon.h>
#include <cdpmain.h>
#include <house.h>
#include <modeno.h>
#include <pnames.h>
#include <flags.h>
#include <filetype.h>
#include <sfsys.h>
#include <string.h>
/**
#include <logic.h>
#include <limits.h>
#include <math.h>
**/

#define MAXTYPES (512)

#define	IS_MONO		(1)
#define	IS_STEREO	(2)
#define	IS_QUAD		(3)
#define	IS_ANAL		(4)
#define	IS_PITCH	(5)
#define	IS_TRANSPOS	(6)
#define	IS_FMNT		(7)
#define	IS_ENV		(8)

static int  compare_properties_for_bundling(fileptr fp1,infileptr fp2);
		    
static int  read_and_sort_properties(char *infilename,dataptr dz);
static int  do_srates(char *infilename,char *thisfilename,
			char **file48,char **file44,char **file32,char **file24,char **file22,char **file16,
			int *is_file48,int *is_file44,int *is_file32,int *is_file24,int *is_file2,int *is_file16,
			FILE **fp48,FILE **fp44,FILE **fp32,FILE **fp24,FILE **fp22,FILE **fp16,dataptr dz);
static void do_rogues(char *infilename,int *a_srate,dataptr dz);
static int  read_srate(char *infilename,dataptr dz);
static int  read_channels(char *infilename,dataptr dz);
static void get_length(char *infilename,dataptr dz);
static int  do_lenths(char *infilename,char *thisfilename,char **namestore,char ***lstore,
			int *namestoresize,int *lcnt,double *sortlens,dataptr dz);
static int  create_lfile(char *infilename,char **otherfile,char ***lstore,double *sortlens,int *lcnt,dataptr dz);
static int  create_ofile(char *infilename,char **otherfile,int cnt,
			char ***lstore,double *lenstore,int *posstore,dataptr dz);
static int  do_types(char *infilename, char *thisfilename,char **monofile,char **stereofile,char **quadfile,
			char **analfile,char **pitchfile, char **transfile,char **formantfile,char **envfile,char **otherfile,
			int *is_mono_list,int *is_stereo_list,int *is_quad_list,int *is_anal_list,int *is_pitch_list,
			int *is_trans_list,int *is_fmnt_list,int *is_env_list,int *is_other_list,
			FILE **fpm,FILE **fps,FILE **fpa,FILE **fpp,FILE **fpt,FILE **fpf,FILE **fpe,FILE **fpo,FILE **fpq,dataptr dz);
//TW REVISED
static int  output_lenths(char *infilename,char **otherfile,
			char ***lstore,double *sortlens,int *lcnt,dataptr dz);
static void output_srates(int is_file48,int is_file44,int is_file32,int is_file24,int is_file22,int is_file16,
		   	char *file48,char *file44,char *file32,char *file24,char *file22,char *file16);
static void output_types(char *infilename,int is_mono_list,int is_stereo_list,int is_quad_list,
			int is_anal_list,int is_pitch_list,
			int is_trans_list,int is_fmnt_list,int is_env_list,int is_other_list,
			char *monofile,char *stereofile,char *quadfile,char *analfile,char *pitchfile,char *transfile,
			char *formantfile,char *envfile,char *otherfile);
static void strip_extension(char *filename);
static int  do_order(char *infilename,char *thisfilename,char **namestore,double **lenstore,int **posstore,
			char ****lstore,int *fileno,int *namestoresize,dataptr dz);
static int  output_order(char *infilename,char **otherfile,
			int *posstore,double *lenstore,char ***lstore,int cnt,dataptr dz);
static int  read_special_type(dataptr dz);
static int  filename_extension_is_not_sound(char *filename);

int outfcnt = 0;

/************************** DO_BUNDLE *****************************/

int do_bundle(dataptr dz)
{
	int exit_status;
	int got_sndfile = FALSE, n, OK;
	int filetype = dz->infile->filetype;
	fileptr fp1 = dz->infile;
	double maxamp, maxloc;
	int maxrep;
	int getmax = 0, getmaxinfo = 0;
	infileptr ifp;
	if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on files.\n");
		return(MEMORY_ERROR);
	}

	for(n=0;n<dz->infilecnt;n++) {
		if(!strcmp(dz->wordstor[n],dz->wordstor[dz->all_words-1])) {
			sprintf(errstr,"Name of out-listfile [%s] cannot be included in the listing!!\n",dz->wordstor[n]);
			return(DATA_ERROR);
		}
	}
	if(!is_a_text_input_filetype(filetype))
		got_sndfile = TRUE;
	if(got_sndfile || dz->mode==BUNDLE_ALL) {
		sprintf(errstr,"BUNDLED %s\n",dz->wordstor[0]);
		print_outmessage(errstr);
		fprintf(dz->fp,"%s\n",dz->wordstor[0]);
	}
	for(n=1;n<dz->infilecnt;n++) {
		for(;;) {
			OK = FALSE;
			if(dz->mode==BUNDLE_ALL) {
				OK = TRUE;					  /* list any file */
				break;
			} else if((dz->ifd[n] = sndopenEx(dz->wordstor[n],0,CDP_OPEN_RDONLY)) < 0)
				break;						  /* else if non-text file, break */
			if(dz->mode==BUNDLE_NONTEXT) {
				OK = TRUE;					  /* if any non-text file OK, break */
				break;				
			}
			if(!got_sndfile) {
				got_sndfile = TRUE;
				if((exit_status = readhead(ifp,dz->ifd[n],dz->wordstor[n],&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
					return(exit_status);
				filetype = ifp->filetype;
				copy_to_fileptr(ifp,fp1);
				OK = TRUE;					  /* 1st non-text file is always OK, break */
				break;
			}
			if((exit_status = readhead(ifp,dz->ifd[n],dz->wordstor[n],&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
				return(exit_status);
			if(ifp->filetype!=filetype) /* If files NOT of same type, break */
				break;	
			if(dz->mode==BUNDLE_TYPE) {
				OK = TRUE;					  /* if any file of same type OK, break */
				break;
			}
			if(compare_properties_for_bundling(fp1,ifp)==CONTINUE)
				break;					  	/* if properties incompatible, break */
			if(dz->mode==BUNDLE_SRATE) {
				OK = TRUE;					/* if any file with same props OK, break */
				break;
			}								/* if files have same chancnt, set OK for BUNDLE_CHAN */
			if(fp1->channels==ifp->channels)
				OK = TRUE;				
			break;	
		}
		if(OK) {
			sprintf(errstr,"BUNDLED %s\n",dz->wordstor[n]);
			print_outmessage(errstr);
			fprintf(dz->fp,"%s\n",dz->wordstor[n]);
		}
	}
	fflush(stdout);
	free(ifp);
	return(FINISHED);
}

/************************** COMPARE_PROPERTIES_FOR_BUNDLING *****************************/

int compare_properties_for_bundling(fileptr fp1,infileptr fp2)
{
	switch(fp1->filetype) {
 	case(PITCHFILE):	/* compare properties not in analfiles */
	case(FORMANTFILE):
	case(TRANSPOSFILE):
		if(fp2->origchans != fp1->origchans)
			return(CONTINUE);
		/* fall thro */ 
	case(ANALFILE):	/* compare properties common to all float files except envfiles */
		if(fp2->origstype != fp1->origstype)
			return(CONTINUE);
		if(fp2->origrate != fp1->origrate)
			return(CONTINUE);
		if(fp2->arate != fp1->arate)
			return(CONTINUE);
		if(fp2->Mlen != fp1->Mlen)
			return(CONTINUE);
		if(fp2->Dfac != fp1->Dfac)
			return(CONTINUE);
		/* fall thro */ 
	case(SNDFILE):
// TW MOVED THIS HERE : June 2004
		if(fp2->channels != fp1->channels)
			return(CONTINUE);
		if(fp2->srate != fp1->srate)
			return(CONTINUE);
		if(fp2->stype != fp1->stype)
			return(CONTINUE);
		break;
	case(ENVFILE):
		if(fp2->window_size != fp1->window_size)
			return(CONTINUE);
		break;
	default:
		sprintf(errstr,"Inaccessible case in compare_properties_for_bundling()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/**************************** DO_FILE_SORTING *****************************/

int do_file_sorting(dataptr dz)
{
	int exit_status;
	int  n;
	int a_srate = 0;
	double sum;
	int  infilecnt = dz->all_words-1;
	int  fileno = 0, namestoresize = 0;
	char *filename = dz->wordstor[infilecnt];	/* name of input list file */
	char *monofile=NULL,     *stereofile = NULL, *analfile = NULL, *quadfile = NULL;
	char *pitchfile = NULL,  *transfile = NULL,  *formantfile = NULL;
	char *envfile = NULL,    *otherfile  = NULL, *namestore  = NULL;
	int  *posstore   = NULL;
	int  *lcnt       = NULL;
	char ***lstore   = NULL;
 	double *lenstore = NULL;
 	double *sortlens = NULL;
#ifdef NOTDEF
    char *file48 = ENDOFSTR, *file44 = ENDOFSTR, *file32 = ENDOFSTR;
    char *file24 = ENDOFSTR, *file22 = ENDOFSTR, *file16 = ENDOFSTR;
#else
    char *file48 = NULL, *file44 = NULL, *file32 = NULL;
    char *file24 = NULL, *file22 = NULL, *file16 = NULL;
#endif
	int is_file48=FALSE, is_file44=FALSE, is_file32=FALSE, is_file24=FALSE, is_file22=FALSE, is_file16=FALSE;
	int is_mono_list=FALSE,  is_stereo_list=FALSE,is_quad_list=FALSE, is_anal_list=FALSE,is_pitch_list=FALSE;
	int is_trans_list=FALSE, is_fmnt_list=FALSE,  is_env_list=FALSE, is_other_list=FALSE;
	FILE *fp48 = NULL, *fp44 = NULL, *fp32 = NULL, *fp24 = NULL, *fp22 = NULL, *fp16 = NULL, 
	    *fpm = NULL, *fps = NULL, *fpa = NULL, *fpp = NULL, *fpt = NULL, *fpf = NULL, *fpe = NULL, *fpo = NULL, *fpq = NULL;
	char *p;
	int done_errmsg = 0;

	switch(dz->mode) {
	case(BY_DURATION):	
	case(BY_LOG_DUR):	
		if((lcnt = (int *)malloc((dz->iparam[SORT_LENCNT]+1) * sizeof(int)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for lcnt store.\n");
			return(MEMORY_ERROR);
		}
		if((sortlens  = (double *)malloc(dz->iparam[SORT_LENCNT] * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for lens store.\n");
			return(MEMORY_ERROR);
		}
		sum = dz->param[SORT_SMALL];
		for(n=0;n<dz->iparam[SORT_LENCNT];n++) {
			lcnt[n] = 0;				
			sortlens[n] = sum;
			if(dz->mode==BY_LOG_DUR)
				sum *= dz->param[SORT_STEP];
			else
				sum += dz->param[SORT_STEP];
		}
		lcnt[dz->iparam[SORT_LENCNT]] = 0;
		sortlens[dz->iparam[SORT_LENCNT]-1] = dz->param[SORT_LARGE];
		dz->iparam[SORT_LENCNT]++;
		/* fall thro */
	case(IN_DUR_ORDER):	
	   	if((lstore = (char ***)malloc((dz->iparam[SORT_LENCNT]+1) * sizeof(char **)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for length store.\n");
			return(MEMORY_ERROR);
		}
		for(n=0;n<dz->iparam[SORT_LENCNT]+1;n++) {
// AVOID realloc
			if((lstore[n] = (char **)malloc(infilecnt * sizeof(char *)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY for length store %d.\n",n+1);
				return(MEMORY_ERROR);
			}
		}
		break;
	}
	strip_extension(filename);
	if(sloom && (dz->mode == BY_FILETYPE || dz->mode == BY_SRATE)) {
		p = filename + strlen(filename) - 1;		/* Strip trailing zero from generic tempfilename */
		*p = ENDOFSTR;
	}
	for(n=0;n<infilecnt;n++) {
		if(!strcmp(dz->wordstor[infilecnt],dz->wordstor[n])) {
			sprintf(errstr,"The name of the listfile cannot be included in the listing!!\n");
			return(DATA_ERROR);
		}
	}
// AVOID realloc
	for(n=0;n<infilecnt;n++)
		namestoresize += strlen(dz->wordstor[n]) + 1;
	if((namestore = (char *)malloc(namestoresize * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for name store.\n");
		return(MEMORY_ERROR);
	}
	namestoresize = 0;
	for(n=0;n<infilecnt;n++) {
	    if((dz->ifd[0] = sndopenEx(dz->wordstor[n],0,CDP_OPEN_RDONLY)) < 0) 	{
			if(dz->mode!=BY_FILETYPE)  {
				if(!done_errmsg) {
					sprintf(errstr,"Some files are NOT soundfiles.\n");
					print_outmessage_flush(errstr);
					done_errmsg = 1;
				}
			}
			dz->ifd[0] = -1;
			continue;
		} else if((dz->mode!=BY_FILETYPE) && filename_extension_is_not_sound(dz->wordstor[n])) {
			if(!done_errmsg) {
				sprintf(errstr,"Some files are NOT soundfiles.\n");
				print_outmessage_flush(errstr);
				done_errmsg = 1;
			}
			continue;
		}
		switch(dz->mode) {
		case(BY_SRATE):		
			if((exit_status = do_srates(filename,dz->wordstor[n],&file48,&file44,&file32,&file24,&file22,&file16,
			&is_file48,&is_file44,&is_file32,&is_file24,&is_file22,&is_file16,
			&fp48,&fp44,&fp32,&fp24,&fp22,&fp16,dz))<0)
				return(exit_status);
			break;
		case(BY_DURATION):	
		case(BY_LOG_DUR):	
			if((exit_status = do_lenths(filename,dz->wordstor[n],&namestore,lstore,&namestoresize,lcnt,sortlens,dz))<0)
				return(exit_status);
			break;
		case(IN_DUR_ORDER):	
			if((exit_status = do_order
			(filename,dz->wordstor[n],&namestore,&lenstore,&posstore,&lstore,&fileno,&namestoresize,dz))<0)
				return(exit_status);
			break;
		case(BY_FILETYPE):	
			if((exit_status = do_types(filename,dz->wordstor[n],
			&monofile,&stereofile,&quadfile,&analfile,&pitchfile,&transfile,&formantfile,&envfile,&otherfile,
			&is_mono_list,&is_stereo_list,&is_quad_list,&is_anal_list,
			&is_pitch_list,&is_trans_list,&is_fmnt_list,&is_env_list,&is_other_list,
			&fpm,&fps,&fpa,&fpp,&fpt,&fpf,&fpe,&fpo,&fpq,dz))<0)
				return(exit_status);
			break;
		case(FIND_ROGUES):	
			do_rogues(dz->wordstor[n],&a_srate,dz);
			break;
		}

		if(dz->ifd[0]!=-1 && sndcloseEx(dz->ifd[0])<0)
			fprintf(stdout,"WARNING: Failed to close sndfile %s.\n",dz->wordstor[n]);
		dz->ifd[0] = -1;
	}
	switch(dz->mode) {
	case(BY_LOG_DUR):
	case(BY_DURATION):	
		if((exit_status = output_lenths(filename,&otherfile,lstore,sortlens,lcnt,dz))<0)
			return(exit_status);
		break;
 	case(BY_SRATE):		
 		output_srates(is_file48,is_file44,is_file32,is_file24,is_file22,is_file16,
		file48,	file44,file32,file24,file22,file16);						
		break;
 	case(IN_DUR_ORDER):	
 		if((exit_status = output_order(filename,&otherfile,posstore,lenstore,lstore,fileno,dz))<0)
			return(exit_status);
 		break;
	case(BY_FILETYPE):	
		output_types(filename,is_mono_list,is_stereo_list,is_quad_list,
		is_anal_list,is_pitch_list,is_trans_list,is_fmnt_list,is_env_list,
		is_other_list,monofile,stereofile,quadfile,analfile,pitchfile,
		transfile,formantfile,envfile,otherfile);							
		break;

	}
	fflush(stdout);
	return(FINISHED);
}

/********************* READ_AND_SORT_PROPERTIES **********************/

int read_and_sort_properties(char *infilename,dataptr dz)
{
	int ttype = IS_MONO, exit_status;
	double maxamp, maxloc;
	int maxrep;
	int getmax = 0, getmaxinfo = 0;
	infileptr ifp;
	if((ifp = (infileptr)malloc(sizeof(struct filedata)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY to store data on files.\n");
		return(MEMORY_ERROR);
	}
	if((exit_status = readhead(ifp,dz->ifd[0],infilename,&maxamp,&maxloc,&maxrep,getmax,getmaxinfo))<0)
		return(exit_status);
	copy_to_fileptr(ifp,dz->infile);

	if(dz->infile->filetype != SNDFILE)	{
		switch(dz->infile->filetype) {
		case(PITCHFILE):	return(IS_PITCH);
		case(TRANSPOSFILE):	return(IS_TRANSPOS);
		case(FORMANTFILE):	return(IS_FMNT);
		case(ENVFILE):		return(IS_ENV);
		case(ANALFILE):		return(IS_ANAL);
		default:
			sprintf(errstr,"This program only works with sound files or CDP-derived files.\n");
			return(DATA_ERROR);
		}
    }
	if(dz->infile->channels==2)
		ttype = IS_STEREO;
	else if(dz->infile->channels==4)
		ttype = IS_QUAD;
 	else if(dz->infile->channels != 1) {
		sprintf(errstr,"Program accepts sndfiles which are mono, stereo or quad, only.\n");
		return(DATA_ERROR);
    }
    return(ttype);
}

/********************* DO_SRATES *********************/ 

int do_srates(char *infilename,char *thisfilename,
char **file48,char **file44,char **file32,char **file24,char **file22,char **file16,
int *is_file48,int *is_file44,int *is_file32,int *is_file24,int *is_file22,int *is_file16,
FILE **fp48,FILE **fp44,FILE **fp32,FILE **fp24,FILE **fp22,FILE **fp16,dataptr dz)
{
	char temp[4];
	switch(read_srate(thisfilename,dz)) {
	case(48):		
		if(!*is_file48) {
			if((*file48 = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from srate 48000 filename.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*file48,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*file48,temp);				
				outfcnt++;
			} else
				strcat(*file48,".48");				
			if((*fp48 = fopen(*file48,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of 48000 files.\n",*file48);
				return(SYSTEM_ERROR);
			}
			*is_file48 = TRUE;
		}
		fprintf(*fp48,"%s\n",thisfilename);
		break;
	case(44):		
		if(!*is_file44) {
			if((*file44 = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from srate 44100 filename.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*file44,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*file44,temp);				
				outfcnt++;
			} else
				strcat(*file44,".44");				
			if((*fp44 = fopen(*file44,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of 44100 files.\n",*file44);
				return(SYSTEM_ERROR);
			}
			*is_file44 = TRUE;
		}
		fprintf(*fp44,"%s\n",thisfilename);
		break;
	case(32):			
		if(!*is_file32) {
			if((*file32 = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from srate 32000 filename.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*file32,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*file32,temp);				
				outfcnt++;
			} else
				strcat(*file32,".32");
			if((*fp32 = fopen(*file32,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of 32000 files.\n",*file32);
				return(SYSTEM_ERROR);
			}
			*is_file32 = TRUE;
		}
		fprintf(*fp32,"%s\n",thisfilename);
		break;
	case(24):			
		if(!*is_file24) {
			if((*file24 = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from srate 24000 filename.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*file24,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*file24,temp);				
				outfcnt++;
			} else
				strcat(*file24,".24");				
			if((*fp24 = fopen(*file24,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of 24000 files.\n",*file24);
				return(SYSTEM_ERROR);
			}
			*is_file24 = TRUE;
		}
		fprintf(*fp24,"%s\n",thisfilename);
		break;
	case(22):			
		if(!*is_file22) {
			if((*file22 = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from srate 22050 filename.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*file22,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*file22,temp);				
				outfcnt++;
			} else
				strcat(*file22,".22");				
			if((*fp22 = fopen(*file22,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of 22050 files.\n",*file22);
				return(SYSTEM_ERROR);
			}
			*is_file22 = TRUE;
		}
		fprintf(*fp22,"%s\n",thisfilename);
		break;
	case(16):		
		if(!*is_file16) {
			if((*file16 = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from srate 16000 filename.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*file16,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*file16,temp);				
				outfcnt++;
			} else
				strcat(*file16,".16");				
			if((*fp16 = fopen(*file16,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of 16000 files.\n",*file16);
				return(SYSTEM_ERROR);
			}
			*is_file16 = TRUE;
		}
		fprintf(*fp16,"%s\n",thisfilename);
		break;
	default:
		sprintf(errstr,"Ignoring file '%s'\n",thisfilename);
		print_outmessage_flush(errstr);
		break;		 
	}
	return(FINISHED);
}

/********************* DO_ROGUES **********************/

void do_rogues(char *infilename,int *a_srate,dataptr dz)
{
	if(read_special_type(dz)<0)
		return;
	if(read_srate(infilename,dz)<0)
		return;
	if(*a_srate==0)
		*a_srate = dz->infile->srate;
	else if(*a_srate!= dz->infile->srate) {
		sprintf(errstr,"%s has different sampling rate [%d] to earlier file [%d]\n",
		infilename,dz->infile->srate,*a_srate);
		print_outmessage_flush(errstr);
		return;
	}
	if(read_channels(infilename,dz)<0)
		return;
	get_length(infilename,dz);
}

/********************* READ_SRATE **********************/

int read_srate(char *infilename,dataptr dz)
{
	int k;
//TW CHANGED
//  if(sfgetprop(dz->ifd[0], "sample rate", (char *)&dz->infile->srate, sizeof(int)) < 0) {
    if(sndgetprop(dz->ifd[0], "sample rate", (char *)&dz->infile->srate, sizeof(int)) < 0) {
		fprintf(stdout,"WARNING: Can't read sample rate from input soundfile %s\n",infilename);
		return(-1);
    }
	k = (int)(dz->infile->srate/1000);
    return(k);
}

/********************* READ_SPECIAL_TYPE **********************/

int read_special_type(dataptr dz)
{
	int pppp = 0;
//TW CHANGED TO sndgetprop THROUGHOUT
    if(sndgetprop(dz->ifd[0], "is an envelope", (char *)&pppp, sizeof(int)) >=0) {
		sprintf(errstr,"%s is an envelope file\n",snd_getfilename(dz->ifd[0]));
		print_outmessage_flush(errstr);
		return(-1);
	}
    if(sndgetprop(dz->ifd[0], "is a pitch file", (char *)&pppp, sizeof(int)) >=0) {
		sprintf(errstr,"%s is a pitch file\n",snd_getfilename(dz->ifd[0]));
		print_outmessage_flush(errstr);
		return(-1);
	}
    if(sndgetprop(dz->ifd[0], "is a transpos file", (char *)&pppp, sizeof(int)) >=0) {
		sprintf(errstr,"%s is a transposition file\n",snd_getfilename(dz->ifd[0]));
		print_outmessage_flush(errstr);
		return(-1);
	}
    if(sndgetprop(dz->ifd[0], "is a formant file", (char *)&pppp, sizeof(int)) >=0)	{
		sprintf(errstr,"%s is a formant file\n",snd_getfilename(dz->ifd[0]));
		print_outmessage_flush(errstr);
		return(-1);
	}
    return(0);
}

/********************* READ_CHANNELS **********************/

int read_channels(char *infilename,dataptr dz)
{
	int channels;
//TW CHANGED 
    if(sndgetprop(dz->ifd[0], "channels", (char *)&channels, sizeof(int)) < 0) {
		fprintf(stdout,"WARNING: Can't read channels from input soundfile %s\n",infilename);
		return(-1);
    }
	if(channels!=1 && channels!=2 && channels != 4) {
		fprintf(stdout,"WARNING: Invalid channel count [%d] from input soundfile %s\n",channels,infilename);
		return(-1);
    }
    return(0);
}

/********************* GET_LENGTH **********************/

void get_length(char *infilename,dataptr dz)
{
	if((dz->insams[0] = sndsizeEx(dz->ifd[0]))<0) {
		fprintf(stdout, "WARNING: Can't read size of input file %s.\n",infilename);
		return;
	}
	if(dz->insams[0] <=0)
		fprintf(stdout, "WARNING: No data in file %s.\n",infilename);
}

/********************* DO_LENTHS *********************/ 

int do_lenths(char *infilename,char *thisfilename,char **namestore,char ***lstore,int *namestoresize,
				int *lcnt,double *sortlens,dataptr dz)
{
	int exit_status;
	int	k, thislen;
	double d;
	char *namestoreend;
	if((exit_status = read_and_sort_properties(thisfilename,dz))<0)
		return(exit_status);
	if((dz->insams[0] = sndsizeEx(dz->ifd[0]))<0) {
		sprintf(errstr, "Can't read size of input file %s.\n",infilename);
		return(PROGRAM_ERROR);
	}
	d = (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;
	for(k=0;k<dz->iparam[SORT_LENCNT]-1;k++) {
		if(d<sortlens[k])
			break;
	}
	thislen = strlen(thisfilename)+1;
	namestoreend = *namestore + *namestoresize;
	*namestoresize += thislen;
	strcpy(namestoreend,thisfilename);
	lstore[k][lcnt[k]] = namestoreend;
	lcnt[k]++;
	return(FINISHED);
}

/********************* CREATE_LFILE *********************/

int create_lfile(char *infilename,char **otherfile,char ***lstore,double *sortlens,int *lcnt,dataptr dz)
{
	int n, m;
	FILE *fpo;
	if((*otherfile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY from otherfile name.\n");
		return(MEMORY_ERROR);
	}
	strcpy(*otherfile,infilename);
	if(!sloom)
		strcat(*otherfile,".len");
	if((fpo = fopen(*otherfile,"w"))==NULL) {
		sprintf(errstr,"Cannot open file %s to write data\n",*otherfile);
		return(SYSTEM_ERROR);
	}
	for(n=0;n<dz->iparam[SORT_LENCNT]-1;n++) {
		if(lcnt[n]) {
			if(dz->vflag[DONT_SHOW_DURS]) {
				fprintf(fpo,   "\n");
				sprintf(errstr,"\n");
				print_outmessage(errstr);
			}else {
				fprintf(fpo,   "\n%d Files <= %.3lf secs\n",lcnt[n],sortlens[n]);
				sprintf(errstr,"%d Files <= %.3lf secs\n",lcnt[n],sortlens[n]);
				print_outmessage(errstr);
			} for(m=0;m<lcnt[n];m++) {
				fprintf(fpo,   "%s\n",lstore[n][m]);
				sprintf(errstr,"%s\n",lstore[n][m]);
				print_outmessage(errstr);
			}
		}
	}
	fflush(stdout);
	if(lcnt[n]) {
		if(dz->vflag[DONT_SHOW_DURS]) {
			fprintf(fpo,   "\n");
			sprintf(errstr,"\n");
			print_outmessage(errstr);
		} else {
			fprintf(fpo,   "\n%d Files > %.3lf secs\n",lcnt[n],sortlens[n-1]);
			sprintf(errstr,"%d Files > %.3lf secs\n",lcnt[n],sortlens[n-1]);
			print_outmessage(errstr);
		}
		for(m=0;m<lcnt[n];m++) {
			fprintf(fpo   ,"%s\n",lstore[n][m]);
			sprintf(errstr,"%s\n",lstore[n][m]);
			print_outmessage(errstr);
		}
	}
	fflush(stdout);
	if(fclose(fpo)<0) {
		fprintf(stdout,"WARNING: Failed to close output textfile.\n");
		fflush(stdout);
	} 
	return(FINISHED);
}

/********************* CREATE_OFILE *********************/

int create_ofile(char *infilename,char **otherfile,int cnt,char ***lstore,double *lenstore,int *posstore,dataptr dz)
{
	int n, m;
	int spacesize = 32, thisspace;
	char *p;
	FILE *fpo;
	if((*otherfile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY from otherfile name.\n");
		return(MEMORY_ERROR);
	}
	strcpy(*otherfile,infilename);
	if(!sloom)
		strcat(*otherfile,".len");
	if((fpo = fopen(*otherfile,"w"))==NULL) {
		sprintf(errstr,"Cannot open file %s to write data\n",*otherfile);
		return(SYSTEM_ERROR);
	}
	if(dz->vflag[DONT_SHOW_DURS]) {
		for(n=0;n<cnt;n++) {
			fprintf(fpo,   "%s\n",lstore[0][posstore[n]]);
			sprintf(errstr,"%s\n",lstore[0][posstore[n]]);
			print_outmessage(errstr);
		}
	} else {
		for(n=0;n<cnt;n++) {

			sprintf(errstr,"%.6lf secs",lenstore[n]);
			thisspace = spacesize - strlen(errstr);
			for(m=0;m<thisspace;m++)
				strcat(errstr," ");
			p = errstr + strlen(errstr);
			sprintf(p,"%s\n",lstore[0][posstore[n]]);
			fprintf(fpo,"%s",errstr);
			print_outmessage(errstr);
		}
	}
	fflush(stdout);
	if(fclose(fpo)<0) {
		fprintf(stdout,"WARNING: Failed to close output textfile.\n");
		fflush(stdout);
	}
	return(FINISHED);
}

/********************* DO_TYPES *********************/

int do_types(char *infilename, char *thisfilename,char **monofile,char **stereofile,char **quadfile,
			char **analfile,char **pitchfile, char **transfile,char **formantfile,char **envfile,char **otherfile,
			int *is_mono_list,int *is_stereo_list,int *is_quad_list,int *is_anal_list,int *is_pitch_list,
			int *is_trans_list,int *is_fmnt_list,int *is_env_list,int *is_other_list,
			FILE **fpm,FILE **fps,FILE **fpa,FILE **fpp,FILE **fpt,FILE **fpf,
			FILE **fpe,FILE **fpo,FILE **fpq,dataptr dz)
{
	char temp[4];
	if(dz->ifd[0]==-1) {
		if(!*is_other_list) {
			if((*otherfile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from forman file name.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*otherfile,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*otherfile,temp);				
				outfcnt++;
			} else
				strcat(*otherfile,".ott");				
			if((*fpo = fopen(*otherfile,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of other data files.\n",*otherfile);
				return(SYSTEM_ERROR);
			}
			*is_other_list = TRUE;
		}
		fprintf(*fpo,"%s\n",thisfilename);
		return(FINISHED);
	}
	switch(read_and_sort_properties(thisfilename,dz)) {
	case(IS_MONO):			/* Mono   sndfile */
		if(!*is_mono_list) {
			if((*monofile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from monofile name.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*monofile,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*monofile,temp);				
				outfcnt++;
			} else
				strcat(*monofile,".mot");				
			if((*fpm = fopen(*monofile,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of mono files.\n",*monofile);
				return(SYSTEM_ERROR);
			}
			*is_mono_list = TRUE;
		}
		fprintf(*fpm,"%s\n",thisfilename);
		break;
	case(IS_STEREO):		/* Stereo sndfile */
		if(!*is_stereo_list) {
			if((*stereofile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from stereofile name.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*stereofile,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*stereofile,temp);				
				outfcnt++;
			} else
				strcat(*stereofile,".stt");				
			if((*fps = fopen(*stereofile,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of stereo files.\n",*stereofile);
				return(SYSTEM_ERROR);
			}
			*is_stereo_list = TRUE;
		}
		fprintf(*fps,"%s\n",thisfilename);
		break;
	case(IS_QUAD):		/* Quad sndfile */
		if(!*is_quad_list) {
			if((*quadfile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from quadfile name.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*quadfile,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*quadfile,temp);				
				outfcnt++;
			} else
				strcat(*quadfile,".qdt");				
			if((*fpq = fopen(*quadfile,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of quad files.\n",*quadfile);
				return(SYSTEM_ERROR);
			}
			*is_quad_list = TRUE;
		}
		fprintf(*fpq,"%s\n",thisfilename);
		break;
	case(IS_ANAL):			/* Analysis file */
		if(!*is_anal_list) {
			if((*analfile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from analfile name.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*analfile,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*analfile,temp);				
				outfcnt++;
			} else
				strcat(*analfile,".ant");				
			if((*fpa = fopen(*analfile,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of analysis files.\n",*analfile);
				return(SYSTEM_ERROR);
			}
			*is_anal_list = TRUE;
		}
		fprintf(*fpa,"%s\n",thisfilename);
		break;
	case(IS_PITCH):			/* Pitch data file */
		if(!*is_pitch_list) {
			if((*pitchfile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from pitchfile name.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*pitchfile,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*pitchfile,temp);				
				outfcnt++;
			} else
				strcat(*pitchfile,".pct");				
			if((*fpp = fopen(*pitchfile,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of pitchdata files.\n",*pitchfile);
				return(SYSTEM_ERROR);
			}
			*is_pitch_list = TRUE;
		}
		fprintf(*fpp,"%s\n",thisfilename);
		break;
	case(IS_TRANSPOS):			/* Transposition data file */
		if(!*is_trans_list) {
			if((*transfile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from transposition file name.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*transfile,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*transfile,temp);				
				outfcnt++;
			} else
				strcat(*transfile,".trt");				
			if((*fpt = fopen(*transfile,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of transposition data files.\n",*transfile);
				return(SYSTEM_ERROR);
			}
			*is_trans_list = TRUE;
		}
		fprintf(*fpt,"%s\n",thisfilename);
		break;
	case(IS_FMNT):			/* Formant Data file */
		if(!*is_fmnt_list) {
			if((*formantfile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from forman file name.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*formantfile,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*formantfile,temp);				
				outfcnt++;
			} else
				strcat(*formantfile,".fot");				
			if((*fpf = fopen(*formantfile,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of formant data files.\n",*formantfile);
				return(SYSTEM_ERROR);
			}
			*is_fmnt_list = TRUE;
		}
		fprintf(*fpf,"%s\n",thisfilename);
		break;
	case(IS_ENV):			/* Formant Data file */
		if(!*is_env_list) {
			if((*envfile = (char *)malloc((strlen(infilename)+5) * sizeof(char)))==NULL) {
				sprintf(errstr,"INSUFFICIENT MEMORY from other file name.\n");
				return(MEMORY_ERROR);
			}
			strcpy(*envfile,infilename);
			if(sloom) {
				sprintf(temp,"%d",outfcnt);
				strcat(*envfile,temp);				
				outfcnt++;
			} else
				strcat(*envfile,".ett");				
			if((*fpe = fopen(*envfile,"w"))==NULL) {
				sprintf(errstr,"Cannot open file '%s' to write list of formant data files.\n",*envfile);
				return(SYSTEM_ERROR);
			}
			*is_env_list = TRUE;
		}
		fprintf(*fpe,"%s\n",thisfilename);
		break;
	default:
		sprintf(errstr,"Unknown case in do_types()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/********************* OUTPUT_LENTHS *********************/
//TW REVISED
int output_lenths(char *infilename,char **otherfile,char ***lstore,double *sortlens,int *lcnt,dataptr dz)
{
	int  exit_status;
	int n, sum = 0;
	for(n=0;n<dz->iparam[SORT_LENCNT];n++)
		sum += lcnt[n];
	if(sum==0) {
		sprintf(errstr,"NO VALID FILES FOUND.\n");
		print_outmessage(errstr);
	} else {
		if((exit_status = create_lfile(infilename,otherfile,lstore,sortlens,lcnt,dz))<0)
			return(exit_status);
//TW UPDATE
		if(!sloom && !sloombatch)
			fprintf(stdout,"\nCREATED SORT FILE '%s'\n",*otherfile);
		else {
			fprintf(stdout,"INFO: \n");
			fprintf(stdout,"INFO: CREATED SORT FILE '%s'\n",*otherfile);
		}
	}
	fflush(stdout);
 	return(FINISHED);
 }

/********************* OUTPUT_SRATES *********************/

void output_srates(int is_file48,int is_file44,int is_file32,int is_file24,int is_file22,int is_file16,
				   char *file48,char *file44,char *file32,char *file24,char *file22,char *file16)
{
	if(!is_file48 && !is_file44 && !is_file32 
	&& !is_file24 && !is_file22 && !is_file16) {
		sprintf(errstr,"NO SORT FILES CREATED.\n");
		print_outmessage(errstr);
	} else {
//TW UPDATE
		if(!sloom && !sloombatch)
			fprintf(stdout,"\nCREATED SORT FILES....\n");
		else {
			fprintf(stdout,"INFO: \n");
			fprintf(stdout,"INFO: CREATED SORT FILES....\n");
		}
//TW UPDATE
		if(!sloom && !sloombatch) {
			if(is_file48)   		fprintf(stdout,"%s\n",file48);
			if(is_file44)   		fprintf(stdout,"%s\n",file44);
			if(is_file32)   		fprintf(stdout,"%s\n",file32);
			if(is_file24)   		fprintf(stdout,"%s\n",file24);
			if(is_file22)   		fprintf(stdout,"%s\n",file22);
			if(is_file16)   		fprintf(stdout,"%s\n",file16);
		} else {
			if(is_file48)   		fprintf(stdout,"INFO: %s LISTING FILES OF SAMPLE RATE 48000\n",file48);
			if(is_file44)   		fprintf(stdout,"INFO: %s LISTING FILES OF SAMPLE RATE 44100\n",file44);
			if(is_file32)   		fprintf(stdout,"INFO: %s LISTING FILES OF SAMPLE RATE 32000\n",file32);
			if(is_file24)   		fprintf(stdout,"INFO: %s LISTING FILES OF SAMPLE RATE 24000\n",file24);
			if(is_file22)   		fprintf(stdout,"INFO: %s LISTING FILES OF SAMPLE RATE 22050\n",file22);
			if(is_file16)   		fprintf(stdout,"INFO: %s LISTING FILES OF SAMPLE RATE 16000\n",file16);
		}
	}
	fflush(stdout);
}

/********************* OUTPUT_TYPES *********************/

void output_types(char *infilename,int is_mono_list,int is_stereo_list,int is_quad_list,
				int is_anal_list,int is_pitch_list,
				int is_trans_list,int is_fmnt_list,int is_env_list,int is_other_list,
				char *monofile,char *stereofile,char *quadfile,char *analfile,char *pitchfile,char *transfile,
				char *formantfile,char *envfile,char *otherfile)
{
	if(!is_mono_list && !is_stereo_list && !is_quad_list && !is_anal_list 
	&& !is_pitch_list && !is_trans_list && !is_fmnt_list && !is_env_list) {
		if(!is_other_list) {
			sprintf(errstr,"NO SORT FILES CREATED.\n");
			print_outmessage(errstr);
		} else {
			sprintf(errstr,"NO VALID FILES FOUND : see '%s.ott' for files recognised.\n",infilename);
			print_outmessage(errstr);
		}			
	} else {
//TW UPDATE
		if(!sloom && !sloombatch)
			fprintf(stdout,"\nCREATED SORT FILES....\n");
		else {
			fprintf(stdout,"INFO: \n");
			fprintf(stdout,"INFO: CREATED SORT FILES....\n");
		}

//TW UPDATE
		if(!sloom && !sloombatch) {
			if(is_mono_list)  	fprintf(stdout,"%s\n",monofile);		
			if(is_stereo_list)	fprintf(stdout,"%s\n",stereofile);		
			if(is_quad_list)  	fprintf(stdout,"%s\n",quadfile);		
			if(is_anal_list)  	fprintf(stdout,"%s\n",analfile);		
			if(is_pitch_list) 	fprintf(stdout,"%s\n",pitchfile);			
			if(is_trans_list) 	fprintf(stdout,"%s\n",transfile);			
			if(is_fmnt_list)  	fprintf(stdout,"%s\n",formantfile);	
			if(is_env_list)   	fprintf(stdout,"%s\n",envfile);
			if(is_other_list) 	fprintf(stdout,"%s\n",otherfile);
		} else {
			if(is_mono_list)  	fprintf(stdout,"INFO: %s of MONO SNDFILES.\n",monofile);		
			if(is_stereo_list)	fprintf(stdout,"INFO: %s of STEREO SNDFILES.\n",stereofile);		
			if(is_quad_list)  	fprintf(stdout,"INFO: %s of QUAD SNDFILES.\n",quadfile);		
			if(is_anal_list)  	fprintf(stdout,"INFO: %s of ANALYSIS FILES.\n",analfile);		
			if(is_pitch_list) 	fprintf(stdout,"INFO: %s of BINARY PITCH FILES.\n",pitchfile);			
			if(is_trans_list) 	fprintf(stdout,"INFO: %s of BINARY TRANSPOSITION FILES.\n",transfile);			
			if(is_fmnt_list)  	fprintf(stdout,"INFO: %s of FORMANT FILES.\n",formantfile);	
			if(is_env_list)   	fprintf(stdout,"INFO: %s of BINARY ENVELOPE FILES.\n",envfile);
			if(is_other_list) 	fprintf(stdout,"INFO: %s of TEXT FILES.\n",otherfile);
		}
	}
	fflush(stdout);
}

/********************* STRIP_EXTENSION *********************/

void strip_extension(char *filename)
{
	char *p;
	int OK = 0;
	p = filename + strlen(filename) - 1;
	while(*p!='.') {
		if(--p==filename) {
			OK = TRUE;
			break;
		}
	}
	if(!OK)
		*p = ENDOFSTR;
}

/********************* TEST_LENVALS *********************/

int sort_preprocess(dataptr dz)
{
//	int cnt = 0;
	double sum, lastsum;
	switch(dz->mode) {
	case(BY_DURATION):
	case(BY_LOG_DUR):
		dz->iparam[SORT_LENCNT] = 0;
		if(flteq(dz->param[SORT_SMALL],dz->param[SORT_LARGE])) {
			sprintf(errstr,"Parameters for durations are the same: can't proceed.\n");
			return(DATA_ERROR);
		}
		if(dz->param[SORT_LARGE] < dz->param[SORT_SMALL]) {
			fprintf(stdout,"WARNING: Duration parameters inverted: swapping them round.\n");
			swap(&(dz->param[SORT_LARGE]),&(dz->param[SORT_SMALL]));
		}
		sum = dz->param[SORT_SMALL];
		do {
			lastsum = sum;
			if(++dz->iparam[SORT_LENCNT] > MAXTYPES) {
				sprintf(errstr,"Over %d length types: too many!!\n",MAXTYPES);
				return(GOAL_FAILED);
			}
			if(dz->mode==BY_LOG_DUR)
				sum *= dz->param[SORT_STEP];
			else
				sum += dz->param[SORT_STEP];
		} while(sum < dz->param[SORT_LARGE]);
		if(!flteq(lastsum,dz->param[SORT_LARGE])) {
			if(++dz->iparam[SORT_LENCNT] > MAXTYPES) {
				sprintf(errstr,"Over %d length types: too many!!\n",MAXTYPES);
				return(GOAL_FAILED);
			}
		}
		break;
	}
	return(FINISHED);
}

/********************* DO_ORDER *********************/ 

int do_order(char *infilename,char *thisfilename,char **namestore,double **lenstore,int **posstore,
			char ****lstore,int *fileno,int *namestoresize,dataptr dz)
{
	int  exit_status;
	int n, m, thislen;
	double d;
	char *namestoreend;
	if((exit_status = read_and_sort_properties(thisfilename,dz))<0)
		return(exit_status);
	if((dz->insams[0] = sndsizeEx(dz->ifd[0]))<0) {
		sprintf(errstr, "Can't read size of input file %s.\n",infilename);
		return(PROGRAM_ERROR);
	}
	d = (double)(dz->insams[0]/dz->infile->channels)/(double)dz->infile->srate;
	thislen = strlen(thisfilename)+1;
	namestoreend = *namestore + *namestoresize;
	*namestoresize += thislen;
	strcpy(namestoreend,thisfilename);

	(*lstore)[0][*fileno] = namestoreend;
	if(*fileno) {
		for(n = 0;n < *fileno; n++)	{
			if(d <= (*lenstore)[n])
				break;
		}
		if(((*posstore) = (int    *)realloc((char *)(*posstore),((*fileno)+1) * sizeof(int   )))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate posstore.\n");
			return(MEMORY_ERROR);
		}
		if(((*lenstore) = (double *)realloc((char *)(*lenstore),((*fileno)+1) * sizeof(double)))==NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY to reallocate lenstore.\n");
			return(MEMORY_ERROR);
		}
		if(n != *fileno) {
			for(m = *fileno; m > n; m--) {
				(*lenstore)[m] = (*lenstore)[m-1];
				(*posstore)[m] = (*posstore)[m-1];
			}
		}

	} else {
		if(((*lenstore) = (double *)malloc(sizeof(double))) == NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for lenstore.\n");
			return(MEMORY_ERROR);
		}
		if(((*posstore) = (int    *)malloc(sizeof(int))) == NULL) {
			sprintf(errstr,"INSUFFICIENT MEMORY for posstore.\n");
			return(MEMORY_ERROR);
		}
		n = 0;
	}		
	(*lenstore)[n] = d;
	(*posstore)[n] = *fileno;
	(*fileno)++;
	return(FINISHED);
}

/********************* OUTPUT_ORDER *********************/

int output_order(char *infilename,char **otherfile,int *posstore,double *lenstore,char ***lstore,int cnt,dataptr dz)
{
	int exit_status;
	if(cnt==0) {
		sprintf(errstr,"NO VALID FILES FOUND.\n");
		print_outmessage(errstr);
	} else {
		if((exit_status = create_ofile(infilename,otherfile,cnt,lstore,lenstore,posstore,dz))<0)
			return(exit_status);
//TW UPDATE
		if(!sloom && !sloombatch)
			fprintf(stdout,"\nCREATED LENGTH-ORDERED FILE '%s'\n",*otherfile);
		else {
			fprintf(stdout,"INFO: \n");
			fprintf(stdout,"INFO: CREATED LENGTH-ORDERED FILE '%s'\n",*otherfile);
		}
	}
 	fflush(stdout);
	return(FINISHED);
 }

/********************* FILENAME_EXTENSION_IS_NOT_SOUND *********************/

int filename_extension_is_not_sound(char *filename)
{
	char *q = filename;
	char *p = filename + strlen(filename);
	while(p > q) {
		p--;
		if(*p == '.') {
			if(!strcmp(p,".evl")
			|| !strcmp(p,".frq")
			|| !strcmp(p,".trn")
			|| !strcmp(p,".ana")
			|| !strcmp(p,".for")) {
				return 1;
			}
		}
	}
	return 0;
}

