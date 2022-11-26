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



/* floatsam version*/
#include <stdio.h>
#include <stdlib.h>
/*RWD*/
#include <memory.h>

#include <structures.h>
#include <tkglobals.h>
#include <cdpmain.h>
#include <globcon.h>
#include <distort.h>
//TW UPDATE
#include <distort1.h>
#include <modeno.h>
#include <arrays.h>

#include <sfsys.h>




static int  gen_distort_sin_table(dataptr dz);
static int  convert_frq_to_cyclelen(int paramno,dataptr dz);
static int  reduce_brkvals_by_one(int paramno,dataptr dz);
static int  frequencies_cross(dataptr dz);
//TW UPDATE
static int  make_distorter_tab(dataptr dz);

/****************************** DISTORT_PREPROCESS *********************************/

int distort_preprocess(dataptr dz)
{
	int exit_status;
    dz->iparam[DISTORT_PULSE] = (int)round(PULSEWIDTH * (double)dz->infile->srate);
	if(dz->mode==DISTORT_SINE && (exit_status = gen_distort_sin_table(dz))<0)
		return(exit_status);
	return(FINISHED);
}

/****************************** GEN_DISTORT_SIN_TABLE *********************************/

int gen_distort_sin_table(dataptr dz)
{
	int n;
	if((dz->parray[DISTORT_SIN] = (double *)malloc(SINETABLEN * sizeof(double)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for sinewave table.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<SINETABLEN;n++)
		dz->parray[DISTORT_SIN][n] = sin(PI * (double)n/(double)SINETABLEN);
	return(FINISHED);
}

/****************************** DISTORTENV_PREPROCESS *********************************/

int distortenv_preprocess(dataptr dz)
{
	int exit_status;
	if((exit_status = reset_distorte_modes(dz))<0)
		return(exit_status);
	dz->param[ONE_LESS_TROF] = 1.0 - dz->param[DISTORTE_TROF];
	return(FINISHED);
}

/**************************** RESET_DISTORTE_MODES ************************/

int reset_distorte_modes(dataptr dz)
{
	if(dz->mode == DISTORTE_USERDEF)
		return(FINISHED);
	if(dz->mode == DISTORTE_TROFFED) {
		if(!dz->vflag[0]) 			/* Only 1 flag exists in this mode, it is for EXPON, not TROF as in other cases!! */
			dz->mode = DISTORTE_LINTROF;
	} else {
		if(!dz->vflag[DISTORTE_IS_EXPON]) {	/* flag 1 */
	    	switch(dz->mode) {
	    	case(DISTORTE_RISING):   dz->mode = DISTORTE_LINRISE;  	break;
	    	case(DISTORTE_FALLING):  dz->mode = DISTORTE_LINFALL;  	break;
			default:
				sprintf(errstr,"Bad case in reset_distorte_modes()\n");
				return(PROGRAM_ERROR);
			}
	    }
		if(dz->vflag[DISTORTE_IS_TROF]) {		/* flag 0 */
		    switch(dz->mode) {
	    	case(DISTORTE_RISING):	dz->mode = DISTORTE_RISING_TR;	break;
			case(DISTORTE_LINRISE):	dz->mode = DISTORTE_LINRISE_TR;	break;
	    	case(DISTORTE_FALLING):	dz->mode = DISTORTE_FALLING_TR;	break;
			case(DISTORTE_LINFALL):	dz->mode = DISTORTE_LINFALL_TR;	break;
			}
	    }
	}
	return(FINISHED);
}

/****************************** DISTORTMLT_PREPROCESS *********************************/

int distortmlt_preprocess(dataptr dz)
{
	memset((char *)dz->sampbuf[2],0,dz->buflen * sizeof(float));
	memset((char *)dz->sampbuf[3],0,dz->buflen * sizeof(float));
	memset((char *)dz->sampbuf[4],0,dz->buflen * sizeof(float));
	memset((char *)dz->sampbuf[5],0,dz->buflen * sizeof(float));
	return(FINISHED);
}

/****************************** DISTORTDIV_PREPROCESS *********************************/

int distortdiv_preprocess(dataptr dz)
{
	memset((char *)dz->sampbuf[2],0,dz->buflen * sizeof(float));
	memset((char *)dz->sampbuf[3],0,dz->buflen * sizeof(float));
	return(FINISHED);
}

/****************************** DISTORTER_PREPROCESS *********************************/

int distorter_preprocess(int param1,int param2,int param3,dataptr dz)
{
	int exit_status;
	int   brklen;
	double brkmax;
	if(dz->brksize[param1]) {
		if((exit_status = get_maxvalue_in_brktable(&brkmax,DISTORTA_CYCLECNT,dz))<0)
			return(exit_status);
		brklen = round(brkmax);
	} else
		brklen = dz->iparam[param1];
   	if((dz->lparray[param2] = (int *)malloc((brklen+1) * sizeof(int)))==NULL
   	|| (dz->lparray[param3] = (int *)malloc(brklen     * sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for cycle information arrays.\n");
		return(MEMORY_ERROR);
	}

	/*RWD 4:2002 we shadow this with lfarray */
	if((dz->lfarray[param2] = (float *)malloc((brklen+1) * sizeof(float)))==NULL
   	|| (dz->lfarray[param3] = (float *)malloc(brklen     * sizeof(float)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for cycle information arrays.\n");
		return(MEMORY_ERROR);
	}

	return(FINISHED);
}

/****************************** DISTORTSHUF_PREPROCESS *********************************/

int distortshuf_preprocess(dataptr dz)
{
	int exit_status;
	int maxval;
	double brkmax;
	if(dz->brksize[DISTORTS_CYCLECNT]) {
		if((exit_status = get_maxvalue_in_brktable(&brkmax,DISTORTS_CYCLECNT,dz))<0)
			return(exit_status);
		maxval = round(brkmax);
	} else
		maxval = dz->iparam[DISTORTS_CYCLECNT];
	if((dz->lparray[DISTORTS_STARTCYC] = (int *)malloc((maxval+1)*dz->iparam[DISTORTS_DMNCNT]*sizeof(int)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for cycle information array.\n");
		return(MEMORY_ERROR);
	}
	return(FINISHED);
}

/****************************** DISTORTDEL_PREPROCESS *********************************/

int distortdel_preprocess(dataptr dz)
{
	switch(dz->mode) {
	case(DELETE_IN_STRICT_ORDER):
		if(dz->brksize[DISTDEL_CYCLECNT])
			reduce_brkvals_by_one(DISTDEL_CYCLECNT,dz);	/* becomes number of cycles to delete */
		else 
			dz->iparam[DISTDEL_CYCLECNT]--; 
		break;
	case(KEEP_STRONGEST):
	case(DELETE_WEAKEST):
		return distorter_preprocess(DISTDEL_CYCLECNT,DISTDEL_STARTCYC,DISTDEL_CYCLEVAL,dz);
	}
	return(FINISHED);
}

/****************************** DISTORTFLT_PREPROCESS *********************************/

int distortflt_preprocess(dataptr dz)
{
	int exit_status;
	switch(dz->mode) {
	case(DISTFLT_HIPASS):
		convert_frq_to_cyclelen(DISTFLT_LOFRQ_CYCLELEN,dz);
		break;
	case(DISTFLT_LOPASS):
		convert_frq_to_cyclelen(DISTFLT_HIFRQ_CYCLELEN,dz);
		break;
	case(DISTFLT_BANDPASS):
		if(dz->brksize[DISTFLT_LOFRQ_CYCLELEN] && dz->brksize[DISTFLT_HIFRQ_CYCLELEN]) {
		/* Force both brktables to begin at time zero */
			if((exit_status = force_value_at_zero_time(DISTFLT_LOFRQ_CYCLELEN,dz))<0)
				return(exit_status);
			if((exit_status = force_value_at_zero_time(DISTFLT_HIFRQ_CYCLELEN,dz))<0)
				return(exit_status);
		}
		/* Check brktables against each other: If hifrq < lofrq: reject */
		if(frequencies_cross(dz)) {
			sprintf(errstr,"Filter frqs cross: cannot proceed\n");
			return(DATA_ERROR);
		}
		convert_frq_to_cyclelen(DISTFLT_LOFRQ_CYCLELEN,dz);
		convert_frq_to_cyclelen(DISTFLT_HIFRQ_CYCLELEN,dz);
		break;
	default:
		sprintf(errstr,"Unknown mode for DISTORT_FLT in param_preprocess()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

/***************************** CONVERT_FRQ_TO_CYCLELEN ******************************/

int convert_frq_to_cyclelen(int paramno,dataptr dz)
{
	double *p, *pend;
	if(dz->brksize[paramno]) {
		p = dz->brk[paramno] + 1;
		pend = p + (dz->brksize[paramno] * 2);
		while(p < pend) {
			*p = dz->infile->srate/(*p);
			p += 2;
		}
		dz->is_int[paramno] = TRUE;	/* Force rounding in brktable reads */
	} else 
		dz->iparam[paramno] = (int)round(dz->infile->srate/dz->param[paramno]);
	return(FINISHED);
}

/**************************** REDUCE_BRKVALS_BY_ONE ************************/

int reduce_brkvals_by_one(int paramno,dataptr dz)
{
	double *p = dz->brk[paramno];
	int n;
	if(dz->brksize[paramno] <= 0) {
		sprintf(errstr,"Brktable is empty.\n");
		return(DATA_ERROR);
	}
	p++;
	for(n=0;n<dz->brksize[paramno];n++) {
		*p -= 1.0;
		p += 2;
	}
	return(FINISHED);
}	

/**************************** FREQUENCIES_CROSS ************************/

int frequencies_cross(dataptr dz)
{
	int lo_finished = FALSE;
	int hi_finished = FALSE;
	double *flo, *fhi, *tlo, *thi, *loend, *hiend;
	double lasttime,thistime,timediff,timeratio,lastval,thisval,valdiff,val;
	if(dz->brksize[DISTFLT_LOFRQ_CYCLELEN]) {
		flo   = dz->brk[DISTFLT_LOFRQ_CYCLELEN] + 1;
		loend = dz->brk[DISTFLT_LOFRQ_CYCLELEN] + (dz->brksize[DISTFLT_LOFRQ_CYCLELEN] * 2);
		if(dz->brksize[DISTFLT_HIFRQ_CYCLELEN]) {
			fhi   = dz->brk[DISTFLT_HIFRQ_CYCLELEN] + 1;
			hiend = dz->brk[DISTFLT_HIFRQ_CYCLELEN] + (dz->brksize[DISTFLT_HIFRQ_CYCLELEN] * 2);
			tlo = flo - 1;
			thi = fhi - 1;
			while(lo_finished==FALSE && hi_finished==FALSE) {
				if(*tlo < *thi) {
					lasttime  = *(thi - 2);
					thistime  = *thi;
					timediff  = thistime - lasttime;
					timeratio = (*tlo - lasttime)/timediff;
					lastval   = *(fhi - 2);
					thisval   = *fhi;
					valdiff   = thisval - lastval;
					val       = lastval + (valdiff * timeratio);
					if(*flo >= val)
						return(TRUE);
					if((flo += 2) < loend)
						tlo += 2;
					else {
						flo -= 2;
						lo_finished = TRUE;
					}
				} else if (*tlo > *thi) {
					lasttime  = *(tlo - 2);
					thistime  = *tlo;
					timediff  = thistime - lasttime;
					timeratio = (*thi - lasttime)/timediff;
					lastval   = *(flo - 2);
					thisval   = *flo;
					valdiff   = thisval - lastval;
					val       = lastval + (valdiff * timeratio);
					if(val >= *fhi)
						return(TRUE);
					if((fhi += 2) < hiend)
						thi += 2;
					else {
						fhi -= 2;
						hi_finished = TRUE;
					}
				} else /* (*tlo == *thi) */ {
					if(*flo >= *fhi)
						return(TRUE);
					if((flo += 2) < loend)
						tlo += 2;
					else {
						flo -= 2;
						lo_finished = TRUE;
					}
					if((fhi += 2) < hiend)
						thi += 2;
					else {
						fhi -= 2;
						hi_finished = TRUE;
					}
				}
			}
		} else {
			while(flo < loend) {
				if(*flo >= dz->param[DISTFLT_HIFRQ_CYCLELEN])
					return(TRUE);
				flo += 2;
			}
		}
	} else if(dz->brksize[DISTFLT_HIFRQ_CYCLELEN]) {
		fhi   = dz->brk[DISTFLT_HIFRQ_CYCLELEN] + 1;
		hiend = dz->brk[DISTFLT_HIFRQ_CYCLELEN] + (dz->brksize[DISTFLT_HIFRQ_CYCLELEN] * 2);
		while(fhi < hiend) {
			if(*fhi <= dz->param[DISTFLT_LOFRQ_CYCLELEN])
				return(TRUE);
			fhi += 2;
		}
	}
	return(FALSE);
}

//TW UPDATE : NEW FUNCTIONS
/**************************** OVERLOAD_PREPROCESS ************************/

int overload_preprocess(dataptr dz)
{
	int exit_status;
	if((dz->mode==OVER_SINE) && ((exit_status = make_distorter_tab(dz))<0))
		return(exit_status);
//TW NEW CODE UPDATE TO FLOATS
//	dz->param[DISTORTER_MULT] *= (double)MAXSAMP;
	return(FINISHED);
}

/******************************** MAKE_DISTORTER_TAB ********************************
 *
 *  cosin table shifted to lie between 0.0 and -1.0
 */
 
int make_distorter_tab(dataptr dz)
{
	int n;
	double convertor = (double)TWOPI/(double)DISTORTER_TABLEN;
	double *tab;
	if((dz->parray[0] = (double *)malloc(DISTORTER_TABLEN * sizeof(double)))==NULL) {
		sprintf(errstr,"Out of memory for cosine table.");
		return(MEMORY_ERROR);
	}
	tab = dz->parray[0];
	for(n = 0; n <DISTORTER_TABLEN; n++)
		tab[n] = (cos((double)n * convertor) - 1.0)/2.0;
	return(FINISHED);
}

