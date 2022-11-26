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



/* floatsam version TW */
#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <highlight.h>
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
//TW UPDATE
#include <pnames.h>
#include <string.h>

char errstr[2400];

/*extern*/ int	sloom = 0;
/*extern*/ int	sloombatch = 0;
/*extern*/ int anal_infiles = 1;
/*extern*/ int is_converted_to_stereo = -1;
const char* cdp_version = "7.1.0";

/**************************************** MAIN *********************************************/

int main(int argc,char *argv[])
{
	int exit_status;
	dataptr dz = NULL;
//	char *special_data_string = NULL;
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
//TW UPDATE
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
	/* MARCH 1999 */
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
//TW UPDATE
	if(dz->process == S_TRACE) {
		if(dz->mode == TRC_ABOVE)
			dz->param[TRAC_HIFRQ] = ap->default_val[TRAC_HIFRQ];
		else if(dz->mode == TRC_BELOW)
			dz->param[TRAC_LOFRQ] = ap->default_val[TRAC_LOFRQ];
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
	if((exit_status = spec_process_file(dz))<0) {
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
