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
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <filters.h>
#include <filetype.h>
#include <processno.h>
#include <modeno.h>
#include <formants.h>
#include <cdpmain.h>
#include <special.h>
#include <logic.h>
#include <globcon.h>
#include <cdpmain.h>
#include <sfsys.h>
#include <ctype.h>
#include <string.h>
#include <arrays.h>

/* RWD Apr 2011 rebuilt to fix bug in filter bankfrqs (memory overrun) */
/* subject to revision from  TW etc */


char errstr[2400];

/*extern*/ int	sloom = 0;
/* TW May 2001 */
/*extern*/ int sloombatch = 0;	/*TW may 2001 */
/*extern*/ int anal_infiles = 0;
/*extern*/ int is_converted_to_stereo = -1;
const char* cdp_version = "7.1.0";

// TW MULTICHAN 2010
static int setup_internal_arrays_and_array_pointers_for_lphp(dataptr dz);

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
/*	FILE *fp   = NULL;*/
	dataptr dz = NULL;
/*	char *special_data_string = NULL;*/
	char **cmdline;
	int  cmdlinecnt;
	aplptr ap;
	int *valid = NULL;
	int is_launched = FALSE;
	int  validcnt;

	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
						/* CHECK FOR SOUNDLOOM */
	/* TW May 2001 */	
	if((sloom = sound_loom_in_use(&argc,&argv)) > 1) {
		sloom = 0;
		sloombatch = 1;
	}

	if(!sloom) {
		if((exit_status = allocate_and_initialise_validity_flags(&valid,&validcnt))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	}
	if(sflinit("cdp")){
		sfperror("cdp: initialisation\n");
		return(FAILED);
	}

						  /* SET UP THE PRINCIPLE DATASTRUCTURE */
	if((exit_status = establish_datastructure(&dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

	if(!sloom) {
							  /* INITIAL CHECK OF CMDLINE DATA */
		if((exit_status = make_initial_cmdline_check(&argc,&argv))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		cmdline    = argv;	/* GET PRE_DATA, ALLOCATE THE APPLICATION, CHECK FOR EXTRA INFILES */
		cmdlinecnt = argc;
		if((exit_status = get_process_and_mode_from_cmdline(&cmdlinecnt,&cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}		
		if((exit_status = setup_particular_application(dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
		if((exit_status = count_and_allocate_for_infiles(cmdlinecnt,cmdline,dz))<0) {
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);
			return(FAILED);
		}
	} else {
		if((exit_status = parse_tk_data(argc,argv,&cmdline,&cmdlinecnt,dz))<0) {  	/* includes setup_particular_application()      */
			exit_status = print_messages_and_close_sndfiles(exit_status,is_launched,dz);/* and cmdlinelength check = sees extra-infiles */
			return(exit_status);		 
		}
	}
	ap = dz->application;

/*********************************************************************************************************************
	   cmdline[0]				 		  2 vals					   		  ACTIVE		 
TK 		(infile) (more-infiles) (outfile) (flag val) (formantsqksrch) (special) params  options   variant-params  flags
CMDLINE	(infile) (more-infiles) (outfile) (formants) (formantsqksrch) (special) params  POSSIBLY  POSSIBLY	  	POSSIBLY
								 		  1 val
*********************************************************************************************************************/

	if((exit_status = parse_infile_and_hone_type(cmdline[0],valid,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

	if((exit_status = setup_param_ranges_and_defaults(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

					/* OPEN FIRST INFILE AND STORE DATA, AND INFORMATION, APPROPRIATELY */

	if(dz->input_data_type!=NO_FILE_AT_ALL) {
		if((exit_status = open_first_infile(cmdline[0],dz))<0) {	
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
			return(FAILED);
		}
//TW UPDATE
		cmdlinecnt--;
		cmdline++;
	}
// TW MULTICHAN 2010
	if(dz->process == LPHP) {
		if((exit_status = setup_internal_arrays_and_array_pointers_for_lphp(dz))<0) {	
			print_messages_and_close_sndfiles(exit_status,is_launched,dz);	
			return(FAILED);
		}
	}
	
/*********************************************************************************************************************
		cmdline[0]				   2 vals				   			   ACTIVE		 
TK 		(more-infiles) (outfile) (flag val) (formantsqksrch) (special) params  options   variant-params  flags
CMDLINE	(more-infiles) (outfile) (formants) (formantsqksrch) (special) params  POSSIBLY  POSSIBLY		  POSSIBLY
								   1 val
*********************************************************************************************************************/

	if((exit_status = handle_extra_infiles(&cmdline,&cmdlinecnt,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);		
		return(FAILED);
	}

/*********************************************************************************************************************
		cmdline[0]	  2					   			    ACTIVE		 
TK 		(outfile) (flag val) (formantsqksrch) (special) params  options   variant-params  flags
CMDLINE	(outfile) (formants) (formantsqksrch) (special) params  POSSIBLY  POSSIBLY		   POSSIBLY
					  1
*********************************************************************************************************************/

	if((exit_status = handle_outfile(&cmdlinecnt,&cmdline,is_launched,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

/****************************************************************************************
		cmdline[0]	  		   			       ACTIVE		 
TK 		(flag val) (formantsqksrch) (special) params  options   variant-params  flags
CMDLINE	(formants) (formantsqksrch) (special) params  POSSIBLY  POSSIBLY		POSSIBLY
*****************************************************************************************/

	if((exit_status = handle_formants(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = handle_formant_quiksearch(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = handle_special_data(&cmdlinecnt,&cmdline,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
 
/****************************************************************************************
		cmdline[0]	  		   			    
TK 		active_params  	options   		variant-params  flags
CMDLINE	active_params  	POSSIBLY  		POSSIBLY		POSSIBLY
*****************************************************************************************/

	if((exit_status = read_parameters_and_flags(&cmdline,&cmdlinecnt,dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

	if((exit_status = check_param_validity_and_consistency(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}

 	is_launched = TRUE;

	if((exit_status = allocate_large_buffers(dz))<0){
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = param_preprocess(dz))<0){
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = groucho_process_file(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	if((exit_status = complete_output(dz))<0) {
		print_messages_and_close_sndfiles(exit_status,is_launched,dz);
		return(FAILED);
	}
	exit_status = print_messages_and_close_sndfiles(FINISHED,is_launched,dz);
	free(dz);
	return(SUCCEEDED);
}

// TW MULTICHAN 2010
/***************************** SETUP_INTERNAL_ARRAYS_AND_ARRAY_POINTERS_FOR_LPHP **************************/

int setup_internal_arrays_and_array_pointers_for_lphp(dataptr dz)
{
	int n;		 
	dz->array_cnt = 3 + (FLT_LPHP_ARRAYS_PER_FILTER * dz->infile->channels); 
	if((dz->parray  = (double **)malloc(dz->array_cnt * sizeof(double *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY for internal double arrays.\n");
		return(MEMORY_ERROR);
	}
	for(n=0;n<dz->array_cnt;n++)
		dz->parray[n] = NULL;
	return(FINISHED);
}

