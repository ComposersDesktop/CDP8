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



/* Manipulate or Generate columns of numbers */

#include <columns.h>
#include <srates.h>
#include <string.h>

#define STACK_WITH_OVERLAP 1111

void	usage(void), logo(void);

static void read_flags(char *,char *);
static void do_string_params(char *argv[],int pcnt);
static void read_data(char *startarg,char *endarg,int *ro);
static void do_help(char *str);
static int option_takes_NO_data_but_has_extra_params(char *agv);
static int option_takes_data_and_has_extra_params(char *agv);
static void check_exceptional_processes(int ro,int argc);
static void handle_multifile_processes_and_the_output_file(int argc,char flag,int ro,char **argv);

static void cant_read_numeric_value(char flag,int ro);
static void cant_read_numeric_value_with_flag_only(char flag);
static void unknown_flag_only(char flag);
static void unknown_flag(char flag,int ro);
static void cannot_read_flags(char *flagstr);
static void flag_takes_no_params(char flag,int ro);
static void flag_only_takes_no_params(char flag);
static void unknown_flag_or_bad_param(void);
static void no_value_with_flag(char flag,int ro);
static void no_value_with_flag_only(char flag);
static void no_value_required_with_flag(char flag,int ro);
static void no_value_required_with_flag_only(char flag);
static void read_flags_data_error(char *str);
static void unknown_flag_string(char *flagstr);

static void check_for_columnextract(char *argv1,int *argc,char *argv[]);
static int valid_call_for_multicol_input(char flag,int ro);
static int noro(int ro);
static void output_multicol_data();

int	cnt = 0, firstcnt, arraysize = BIGARRAY, infilecnt, outfilecnt, colcnt, thecol, allcols = 1;
double 	*number, *permm, *permmm, factor, scatter, *pos, *outcoldata;
int outcoldatacnt = 0;
FILE 	**fp;
char 	temp[20000], flag, *filename, *thisfilename, string[200];
int	ro = 0, ifactor = 0, len, condit = 0;
double thresh;
int *file_cnt; 
int sloom = 0;
int sloombatch = 0;

char errstr[400];
char srcfile[2000],goalfile[2000];

#define TOO_BIG_ARRAY (100000)
void reorganise_extra_params(char *agv,int argc,char *argv[]);
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
	int extra_params = 0, k = 0, *perm = (int *)0;
	char *agv = (char *)0;
	char *p;
	goalfile[0] = ENDOFSTR;
	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(argc < 2)
		usage();
	if(!strcmp(argv[1],"#")) {
		sloom = 1;
		argv++;
		argc--;
	} else if (!strcmp(argv[1],"##")) {
		p = argv[0];
		argc--;
		argv++;
		argv[0] = p;
		sloombatch = 1;
	}
    initrand48();
    number = (double *)exmalloc(arraysize * sizeof(double));
    fp     = (FILE **)exmalloc(2 * sizeof(FILE *));
    fp[1]  = stderr;
	if(!sloom && !sloombatch) {
    	if(argc==2)
			do_help(argv[1]);
	}
    if(argc<3)
		usage();
    check_for_columnextract(argv[1],&argc,argv);
    check_for_conditional(&argc,argv);
	agv = argv[argc-1];
	if(!sloom) {
		if(allcols > 1)
			read_data("__temp.txt",argv[argc-1],&ro);
		else
			read_data(argv[1],argv[argc-1],&ro);
	    read_flags(agv,argv[2]);
	} else {
		if(option_takes_NO_data_but_has_extra_params(agv))
			extra_params = 1;
		else {
			if(allcols > 1)
				read_data("__temp.txt",argv[argc-1],&ro);
			else
				read_data(argv[1],argv[argc-1],&ro);
		}
		if(option_takes_data_and_has_extra_params(agv)) {	 
			extra_params = 1;
			read_flags(agv,argv[2]);
		}
		if(!extra_params)
		    read_flags(agv,argv[2]);
	}
	check_exceptional_processes(ro,argc);

	if(!sloom) {
	    if(argc>=4)
			handle_multifile_processes_and_the_output_file(argc,flag,ro,argv);
	    if((number=(double *)exrealloc((char *)number,(cnt+1)*sizeof(double)))==NULL) {
			fprintf(stdout, "ERROR: Out of memory\n");
			fflush(stdout);
			exit(1);		/* EXTRA ONE IS TEMPORARY FOR QIKSORT INDEXING PROBLEM !! */
		}
	} else {
	    if(argc>=4 && !extra_params)
			handle_multifile_processes_and_the_output_file(argc,flag,ro,argv);
		else if(flag =='K' && ro == 'b') {
			handle_multifile_processes_and_the_output_file(argc,flag,ro,argv);
		}
		if(extra_params)
			reorganise_extra_params(agv,argc,argv);
		else if((number=(double *)exrealloc((char *)number,(cnt+1)*sizeof(double)))==NULL) {
			fprintf(stdout, "ERROR: Out of memory\n");
			fflush(stdout);
			exit(1);		/* EXTRA ONE IS TEMPORARY FOR QIKSORT INDEXING PROBLEM !! */
		}
	}
    switch(flag) {
    case('A'): 
		switch(ro) {
		case('d'):	accel_durations();			break; /* Ad */
		case('e'):	abutt();					break; /* Ae */
		case('s'):	alphabetic_sort();			break; /* As */
		case('t'):	accel_time_seq();			break; /* At */
		default:	approx_vals();				break; /* A */
		}
		break;
    case('a'):	
		switch(ro) {
		case(0):	add();						break; /* a */
		case('o'):	ascending_order();			break; /* ao */
		case('T'):	append_text(0);				break; /* aT */
		case('X'):	append_text(1);				break; /* aX */
		}
		break;
    case('B'):		plain_bob();				break; /* B */
    case('b'):
		switch(ro) {
		case('b'):	band_cut();									break; /* bb */
		case('g'):	reduce_to_bound();		print_numbers();	break; /* bg */
		case('l'):	increase_to_bound();	print_numbers();	break; /* bl */
		}
	   	break;
    case('C'):
    	switch(ro) {
		case(ENDOFSTR): concatenate_files(argv[1]);		break; /* C */
		case('c'):		concatenate_files_cyclically(argv[1]);		break; /* Cc */
		case('a'): 		cyclic_select('a');				break; /* Ca */
		case('m'): 		cyclic_select('m');				break; /* Cm */
		}
		break;
    case('c'):
	    switch(ro) {
		case(ENDOFSTR):	count_items(argv[1]);			break; /* c  */
		case('c'): 		columnate();					break; /* cc */
		case('l'): 		count_items(argv[1]);			break; /* cl */
		case('s'): 		cosin_spline();					break; /* cs */
		case('t'): 		cut_table_at_time();			break; /* ct */
		}
		break;
    case('D'):
		switch(ro) {
	    case('B'):		db_to_level(0);				break; /* DB */
	    case('s'):		db_to_level(1);				break; /* Ds */
		}
		break;
    case('d'):
		switch(ro) {
		case(ENDOFSTR): divide_list();			break; /* d */
		case('b'):	level_to_db(0);				break; /* db */
		case('d'):	duplicate_values_stepped();	break; /* dd */
		case('g'):	distance_from_grid();		break; /* dg */
		case('h'):	delay_to_pitch(0);			break; /* dh */
		case('i'):	delete_small_intervals();	break; /* di */
		case('l'):	duplicate_list();			break; /* dl */
		case('L'):	duplicate_list_at_step();	break; /* dL */
		case('M'):	delay_to_pitch(1);			break; /* dM */
		case('o'):	duplicate_octaves();		break; /* do */
		case('O'):	duplicate_octfrq();			break; /* dO */
		case('v'):	duplicate_values();			break; /* dv */
		}
        break;
    case('E'):	vals_end_to_end_in_2_cols();	break; /* E */
	case('e'):
		switch(ro) {
		case(ENDOFSTR):	eliminate_equivalents();	print_numbers();	break; /* e */
		case('a'):	env_append(); 					break; /* ea */
		case('b'):	eliminate_dupltext();			break; /* eb */
		case('d'):	eliminate_duplicates();			print_numbers();	break; /* ed */
		case('e'):	eliminate_even_items();			print_numbers();	break; /* ee */
		case('f'):	expand_table_dur_by_factor();						break; /* ef */
		case('g'):	eliminate_greater_than();		print_numbers();	break; /* eg */
		case('i'):	env_superimpos(INVERT,MULT); 	break; /* ei */
		case('l'):	eliminate_less_than();			print_numbers();	break; /* el */
		case('L'):	list_warp();					break; /* eL */
		case('m'):	env_superimpos(0,ENVMAX);		break; /* em */
		case('Q'):	seqtime_warp();					break; /* eQ */
		case('s'):	env_superimpos(0,MULT);			break; /* es */
		case('t'):	expand_table_to_dur();			break; /* et */
		case('v'):	expand_table_vals_by_factor();	break; /* ev */
		case('w'):	time_warp();					break; /* ew */
		case('W'):	brktime_warp();					break; /* eW */
		case('x'):	extend_table_to_dur();			break; /* ex */
		case('X'):	brkval_warp();					break; /* eX */
		case('Y'):	brktime_owarp();				break; /* eY */
		case('A'):	env_superimpos(0,ADD);			break; /* eA */
		case('D'):	env_del_inv();					break; /* eD */
		case('P'):	env_plateau();					break; /* eP */
		case('S'):	env_superimpos(0,SUBTRACT);		break; /* eS */
		}
	   	break;
    case('F'):	
		switch(ro) {
		case(ENDOFSTR): format_vals();			break; /* F */	 /* RWD */
		case('r'):      column_format_vals();	break; /* Fr */	
		case('f'):      format_strs();			break; /* Ff */	
		case('R'):      column_format_strs();	break; /* FR */	
		}
		break;
	case('f'):
	    switch(ro) {
	    case('r'):		remove_frq_pitchclass_duplicates();    	break; /* fr */
	    case('l'):		floor_vals();    						break; /* fl */
	    case('t'):		pitchtotext(0);    						break; /* ft */
		}
		break;
    case('G'):		group();				break; /* G */
    case('g'):
		switch(ro) {
		case(ENDOFSTR):	greatest();				break; /* g */
		case('s'):		multiply(1);			break; /* gs */
		case('a'):		cumuladd();				break; /* ga */
		}
		break;
	case('H'):
		switch(ro) {
		case(ENDOFSTR):	generate_harmonics();	break;	/* H */
		case('g'):		group_harmonics();		break;	/* Hg */
 		case('r'):		get_harmonic_roots();	break;  /* Hr */
		}
		break;
    case('h'):
		switch(ro) {
		case('d'):		pitch_to_delay(0);		break; /* hd */
		case('M'):		hz_to_midi();			break; /* hM */
		}
		break;
    case('I'):
		switch(ro) {
		case(ENDOFSTR):	print_numbers();			break; /* I */
		case('r'):		interval_to_ratio(1,0);		break; /* Ir */
		case('v'):		invert_normd_env();			break; /* Iv */
		case('V'):		interp_n_vals();			break; /* IV */
		}
		break;
    case('i'):
		switch(ro) {
		case(ENDOFSTR):	get_intervals();			break; /* i */
		case('r'):	repeat_intervals();				break; /* ir */
		case('='):	make_equal_intevals_btwn_given_vals();	break; /* i= */
		case('e'):	create_equal_vals();			break; /* ie */
		case('c'):	create_intervals();				break; /* ic */
		case('C'):	create_intervals_from_base();	break; /* iC */
		case('R'):	create_ratios_from_base();		break; /* iR */
		case('p'):	insert_intermediate_valp();		break; /* ip */
		case('Q'):	create_equal_steps();			break; /* iQ */
		case('a'):	change_value_of_intervals();	break; /* ia */
		case('m'):	change_value_of_intervals();	break; /* im */
		case('v'):	get_intermediate_values();		break; /* iv */
		case('V'):	insert_intermediate_values();	break; /* iV */
		case('M'):	motivically_invert_midi();		break; /* iM */
		case('h'):	motivically_invert_hz();		break; /* ih */
		case('l'):										   /* il */
		case('L'):	interval_limit();				break; /* iL */
		}
		break;
    case('J'): 
    	switch(ro) {
		case(0):	join_many_files_as_columns(argv[1],0);	break; /* J */
		case('j'):	join_many_files_as_columns(argv[1],1);	break; /* Jj */
		}
		break;
    case('j'):
    	switch(ro) {
		case(0):	join_files_as_rows();			break; /* j */
		case('j'):	join_many_files_as_rows();		break; /* jj */
		}
		break;
    case('k'):		
	    switch(ro) {
		case('A'):		kill_path();				break;	/* kA */
		case('B'):		kill_extension();			break;	/* kB */
		case('C'):		kill_path_and_extension();	break;	/* kC */
		case('T'):		kill_text(0);				break;	/* kT */
		case('X'):		kill_text(1);				break;	/* kX */
		case('Y'):		kill_text(2);				break;	/* kY */
		}
		break;
    case('K'):		
		switch(ro) {
		case('a'): ta_reverse_sequence(1);		break;	/* Ka */
		case('A'): a_reverse_sequence(1);		break;	/* KA */
		case('b'): abut_sequences(1);			break;	/* Kb */
		case('c'): compress_sequence(1);		break;	/* Kc */
		case('E'): p_expandset_sequence(1);		break;	/* KE */
		case('I'): p_invertset_sequence(1);		break;	/* KI */
		case('i'): p_invert_sequence(1);		break;	/* Ki */
		case('l'): loop_sequence(1);			break;	/* Kl */
		case('m'): uptempo_sequence(1);			break;	/* Km */
		case('M'): accel_sequence(1);			break;	/* KM */
		case('P'): p_reverse_sequence(1);		break;	/* KP */
		case('r'): t_reverse_sequence(1);		break;	/* Kr */
		case('R'): pa_reverse_sequence(1);		break;	/* KR */
		case('t'): transpose_sequence(1);		break;	/* Kt */
		case('T'): tp_reverse_sequence(1);		break;	/* KT */
		case('Z'): tpa_reverse_sequence(1);		break;	/* KZ */
		}
		break;
    case('L'):		log_equal_divisions();			break; /* L */
    case('l'):
	    switch(ro) {
	    case(ENDOFSTR):	least();					break; /* l */
		case('i'):		limit_vals();				break;	/* li */
		case('v'):		limit_table_val_range();	break;	/* lv */
		}
		break;
    case('M'):
		switch(ro) {
		case(ENDOFSTR):	find_mean();				break; /* M */
		case('d'):		pitch_to_delay(1);			break; /* Md */
		case('h'):		midi_to_hz();				break; /* Mh */
		case('m'):	major_to_minor();				break; /* Mm */
		case('r'):	remove_midi_pitchclass_duplicates();	break; /* Mr */
		case('t'):	pitchtotext(1);					break; /* Mt */
		}
    	break;
    case('m'):		
		switch(ro) {
		case(0):	multiply(0);				break; /* m */
		case('g'):	mark_greater_than();		break; /* mg */
		case('l'):	mark_less_than();			break; /* ml */
		case('m'):	mark_multiples();			break; /* mm */
		case('M'):	minor_to_major();			break; /* mM */
		case('e'):	mark_event_groups();		break; /* me */
		case('i'):	min_interval(0);			break; /* mi */
		case('I'):	min_interval(1);			break; /* mI */
		}
		break;
    case('N'):
    	switch(ro) {
		case('r'):	rotate_partition_values_to_files();	break; /* Nr */
    	default:	partition_values_to_files();		break; /* N */
		}
		break;
    case('n'):
    	switch(ro) {
		case('F'):	note_to_midi(2);					break; /* nF */
		case('M'):	note_to_midi(0);					break; /* nM */
		case('T'):	note_to_midi(1);					break; /* nT */
		}
		break;
    case('O'):
		switch(ro) {
	    case('s'):		multiply(0);				break; /* Os */
	    case('T'):		interval_to_ratio(0,1);		break; /* OT */
		}
		break;
    case('o'):
		switch(ro) {
		case(ENDOFSTR): qiksort(); print_numbers(); break; /* o */
	 	case('r'):		interval_to_ratio(0,0);		break; /* or */
		}
		break;
    case('p'):
		switch(ro) {
		case(ENDOFSTR): product();					break; /* p */
		case('p'):		tw_psuedopan();				break; /* pp */
		}
		break;
    case('P'):		take_power();			break; /* P */
    case('Q'):		quadratic_curve_steps();	break; /* Q */
    case('q'):		
    	switch(ro) {
		case(ENDOFSTR): quantise();					break; /* q */
		case('t'): quantise_time();					break; /* qt */
		case('v'): quantise_val();					break; /* qv */
		case('z'): rand_zigs();						break; /* qz */
		}
		break;
    case('R'):
		switch(ro) {
		case(ENDOFSTR):	reciprocals(0);				break; /* R */
		case('o'):	
		case('x'):	
			if((perm=(int *)malloc(stringscnt * sizeof(int)))==NULL) {
				fprintf(stdout, "ERROR: Out of memory\n");
				fflush(stdout);
				exit(1);
			}
			for(k=0;k<stringscnt;k++)
				perm[k] = k;
			if(ro=='o')
				randomise_order(perm);					   /* Ro */
			else
				randomise_Ntimes(perm);				break; /* Rx */
		case('a'):	add_randval_plus_or_minus();	break; /* Ra */
		case('A'):	add_randval();					break; /* RA */
		case('c'):	randchunks();					break; /* Rc */
		case('e'):	random_elimination();			break; /* Re */
		case('g'):	random_0s_and_1s();				break; /* Rg */
		case('G'):	random_0s_and_1s_restrained();	break; /* RG */
		case('i'):	random_integers();				break; /* Ri */
		case('I'): random_integers_evenly_spread(); break; /* RI */
		case('m'):	multiply_by_randval();			break; /* Rm */
		case('r'):	generate_randomised_vals();		break; /* Rr */
		case('s'):	random_scatter();				break; /* Rs */
		case('v'):	generate_random_values();		break; /* Rv */
		}
		break;
    case('r'):
		switch(ro) {
		case(ENDOFSTR):	reverse_order_of_brkvals();	break; /* r  */
		case('a'):	ratios();						break; /* ra */
		case('I'):	ratio_to_interval(1,0);			break; /* rI */
		case('o'):	ratio_to_interval(0,0);			break; /* ro */
		case('r'):	reverse_list();					break; /* rr */
		case('G'):	random_pairs();					break; /* rG */
		case('J'):	random_pairs_restrained();		break; /* rJ */
		case('R'):	rotate_list(0);					break; /* rR */
		case('X'):	rotate_list(1);					break; /* rX */
		case('m'):	rotate_motif();					break; /* rm */
		case('p'):	mean_of_reversed_pairs();		break; /* rp */
		case('T'):	condit = 0; reciprocals(1);		break; /* rT */
		case('z'): rand_ints_with_fixed_ends();		break; /* rz */
		}
		break;
    case('S'):
		switch(ro) {
		case(ENDOFSTR):	separate_columns_to_files();	break; /* S */
		case('M'):		splice_pos();					break; /* SM */
		}
		break;
    case('s'):
		switch(ro) {
		case(ENDOFSTR):	stack(1);				break; /* s */
		case('a'):	scale_above();				break; /* sa */
		case('b'):	scale_below();				break; /* sb */
		case('c'):	sinjoin('c');				break; /* sc */
		case('d'):	sum_absolute_differences();	break; /* sd */
		case('D'):	level_to_db(1);				break; /* sD */
		case('E'):	convert_to_edits();			break; /* sE */
		case('f'):	scale_from();				break; /* sf */
		case('g'):	multiply(0);				break; /* sg */
		case('k'):	get_one_skip_n();			break; /* sk */
		case('K'):	get_n_skip_one();			break; /* sK */
		case('l'):	do_slope();					break; /* sl */
		case('n'):	sum_nwise();				break; /* sn */
		case('o'):	sum_minus_overlaps();		break; /* so */
		case('u'):	substitute();				break; /* su */
		case('U'):	substitute_all();			break; /* sU */
		case('O'):	multiply(0);				break; /* sO */
		case('p'):	spanpair();					break; /* sp */
		case('s'):	sinusoid();					break; /* ss */
		case('t'):	samp_to_time();				break; /* st */
		case('v'):	sinjoin('v');				break; /* sv */
		case('x'):	sinjoin('x');				break; /* sx */
		case('A'):	span_all();					break; /* sA */
		case('F'):	span_fall();				break; /* sF */
		case('X'):	span_xall();				break; /* sX */
		case('P'):	span();						break; /* sP */
		case('R'):	span_rise();				break; /* sR */
		case('T'):	interval_to_ratio(1,1);		break; /* sT */
		case('z'):	stack(0);					break; /* sz */
		}
		break;
    case('T'):
		switch(ro) {
		case('B'):	time_from_bar_beat_metre_tempo(1);	break; /* TB */
		case('b'):	time_from_bar_beat_metre_tempo(0);	break; /* Tb */
		case('c'):	time_from_crotchet_count();			break; /* Tc */
		case('h'):	temper_hz_data();					break; /* Th */
		case('l'):	time_from_beat_lengths();			break; /* Tl */
		case('M'):	temper_midi_data();					break; /* TM */
		case('O'):	ratio_to_interval(0,1);				break; /* TO */
		case('p'):	time_from_beat_position(0);			break; /* Tp */
		case('P'):	time_from_beat_position(1);			break; /* TP */
		case('r'):	condit = 0; reciprocals(1);			break; /* Tr */
		case('s'):	ratio_to_interval(1,1);				break; /* Ts */
		}
    	break;
    case('t'):
		switch(ro) {
		case(ENDOFSTR):	total();				break; /* t */
		case('B'):	time_to_crotchets(1);		break; /* tB */
		case('c'):	thresh_cut();				break; /* tc */
		case('C'):	time_to_crotchets(0);		break; /* tC */
		case('d'):	time_density();		    	break; /* td */
		case('e'):	mean_tempo();		    	break; /* te */
		case('h'):	text_to_hz();		    	break; /* th */
		case('M'):	print_numbers();	    	break; /* tM */
		case('r'):	reverse_time_intervals();	break; /* tr */
		case('R'):	reverse_time_intervals2();	break; /* tR */
		case('s'):	time_to_samp();				break; /* ts */
		}
	    break;
	case('v'):	rank_vals();					break;	/* v */
	case('V'):	rank_frqs();					break;	/* V */
	case('w'):
		switch(ro) {
		case('o'): warped_times(1);				break;	/* wo */
		case('t'): warped_times(0);				break;	/* wt */
		}
		break;
	case('W'):
		switch(ro) {
		case('a'): ta_reverse_sequence(0);		break;	/* Wa */
		case('A'): a_reverse_sequence(0);		break;	/* WA */
		case('b'): abut_sequences(0);			break;	/* Wb */
		case('c'): compress_sequence(0);		break;	/* Wc */
		case('E'): p_expandset_sequence(0);		break;	/* WE */
		case('I'): p_invertset_sequence(0);		break;	/* WI */
		case('i'): p_invert_sequence(0);		break;	/* Wi */
		case('l'): loop_sequence(0);			break;	/* Wl */
		case('m'): uptempo_sequence(0);			break;	/* Wm */
		case('M'): accel_sequence(0);			break;	/* WM */
		case('P'): p_reverse_sequence(0);		break;	/* WP */
		case('r'): t_reverse_sequence(0);		break;	/* Wr */
		case('R'): pa_reverse_sequence(0);		break;	/* WR */
		case('t'): transpose_sequence(0);		break;	/* Wt */
		case('T'): tp_reverse_sequence(0);		break;	/* WT */
		case('Z'): tpa_reverse_sequence(0);		break;	/* WZ */
		}
		break;
	case('y'):
		switch(ro) {
		case('a'):	replace_with_rand(0);		break; /* ya */
		case('A'):	insert_after_val();			break; /* yA */
		case('b'):	replace_with_rand(-1);		break; /* yb */
		case('B'):	insert_at_start();			break; /* yB */
		case('c'):	replace_with_rand(1);		break; /* yc */
		case('C'):	cyclic_select('s');			break; /* yC */
		case('d'):	delete_vals();				break; /* yd */
		case('e'):	replace_equal();			break; /* ye */
		case('E'):	insert_at_end();			break; /* yE */
		case('g'):	replace_greater();			break; /* yg */
		case('i'):	insert_val();				break; /* yi */
		case('I'):	invert_over_range();		break; /* yI */
		case('l'):	replace_less();				break; /* yl */
		case('o'):	insert_in_order();			break; /* yo */
		case('p'):	invert_around_pivot();		break; /* yp */
		case('P'):	targeted_pan();				break; /* yP */
		case('q'):	randquanta_in_gaps();		break; /* yq */
		case('Q'):	do_squeezed_pan();			break; /* yQ */
		case('r'):	replace_val();				break; /* yr */
		case('R'):	randdel_not_adjacent();		break; /* yR */
		case('s'):	grid(1);					break; /* ys */
		case('S'):	grid(0);					break; /* yS */
		case('x'):	targeted_stretch();			break; /* yx */
		}
		break;
	case('Y'):
		switch(ro) {
		case('s'):	convert_space_pan_to_tex();	break; /* Ys */
		case('S'):	convert_space_tex_to_pan();	break; /* YS */
		}
		break;
	case('z'):
		switch(ro) {
		case('q'):
			if(number[2] > 1.0)  {
				if(number[0] < .001) {
					fprintf(stdout, "ERROR: Quantisation too small for scatter > 1.0\n");
					fflush(stdout);
					exit(1);
				}
				k = (int)floor(number[1]/number[0]) + 1;
				k = (int)ceil(k/(cnt-1));
				k *= round(number[2]);
				if(k > TOO_BIG_ARRAY) {
					fprintf(stdout, "ERROR: Combination of scatter > 1 with small quantisation: requires too much memory\n");
					fflush(stdout);
					exit(1);
				}
				if((perm=(int *)malloc(k * sizeof(int)))==NULL) {
					fprintf(stdout, "ERROR: Out of memory\n");
					fflush(stdout);
					exit(1);
				}
			}
			quantised_scatter(perm,k);    		break; /* za */
		case('X'):	alt0101();					break; /* zX */
		case('Y'):	alt0011();					break; /* zY */
		case('Z'):	alt01100();					break; /* zZ */
		case('A'):	alt0r0r();					break; /* zA */
		case('B'):	altr0r0();					break; /* zB */
		case('C'):	alt00rr();					break; /* zC */
		case('D'):	altrr00();					break; /* zD */
		case('e'):	tw_pseudo_exp();			break; /* ze */
		case('E'):	alt0rr00r();				break; /* zE */
		case('F'):	altr00rr0();				break; /* zF */
		case('G'):	alt00RR();					break; /* zG */
		case('H'):	altrr00();					break; /* zH */
		case('I'):	alt0RR00R();				break; /* zI */
		case('J'):	altR00RR0();				break; /* zJ */
		}
		break;
 	case('Z'):
		switch(ro) {
		case('h'):	just_intonation_Hz();		break; /* Zh */
		case('M'):	just_intonation_midi();		break; /* ZM */
		case('H'):	create_just_intonation_Hz();	break; /* ZH */
		case('m'):	create_just_intonation_midi();	break; /* Zm */
		case('s'):	random_warp();					break; /* Zs */
		}
		break;
    }
	if(allcols > 1)
		output_multicol_data();
	return 0;
}

/****************************** USAGE ****************************/

void usage(void)
{
	if(!sloom) { 
	    logo();
	    fprintf(stderr,
	    "USAGE: columns infile [outfile] -flag[@] [{threshold|}threshold] [--cCOLCNT]\n\n");
	    fprintf(stderr,  "     { = less than:     } = greater than      THRESHOLD is numeric value.\n");
	    fprintf(stderr,  "     threshold can only be used with R,a,m,d,P,c.\n");
	    fprintf(stderr,  "     and specified operation then applies ONLY to vals > OR < threshold value.\n\n");
	    fprintf(stderr,  "     If multicolumn data is being processed\n");
	    fprintf(stderr,  "     COLCNT is the column of the input data to which process is applied.\n");
	    fprintf(stderr,  "     Not all processes work with multicolumn data.\n");
	    fprintf(stderr,  "FOR MORE INFORMATION:\n");
	    fprintf(stderr,  "       columns -f   to see all available flags.\n");
	    fprintf(stderr,  "       columns -g   for generative operations.\n");
	    fprintf(stderr,  "       columns -l   for list manipulation and editing.\n");
	    fprintf(stderr,  "       columns -m   for mathematical operations.\n");
	    fprintf(stderr,  "       columns -M   for musical operations.\n");
	    fprintf(stderr,  "       columns -R   for random operations.\n");
	    fprintf(stderr,  "ALSO\n");
	    fprintf(stderr,  "columns inoutfile1 infile2 [infile3....] -J      to juxtapose cols in one file\n");
	    fprintf(stderr,  "columns infile1 outfiles -SX   to separate cols to X distinct files.\n");
	    fprintf(stderr,  "columns infile1 outfiles -NX   to separate vals to X distinct files, in blocks.\n");
	    fprintf(stderr,  "columns infile1 outfiles -NrX  to separate vals to X distinct files, in rotation\n");
	}
    exit(1);
}

/********************************** READ_FLAGS *************************/

void read_flags(char *argv,char *argv2)
{
	char *p, *q;
	double qtone = 0.0;
	int maxpos;

	if(*argv++!='-')
		cannot_read_flags(argv-1);
	if(sscanf(argv++,"%c",&flag)!=1)
		cannot_read_flags(argv-2);
	if(allcols > 1) {
		ro = *argv;
		if(!valid_call_for_multicol_input(flag,ro)) {
			fprintf(stdout,"ERROR: Operation invalid with multicolumn input.\n");
			fflush(stdout);
			exit(1);
		}
	}
	switch(flag) {
	case('A'):
		ro = *argv++;
		switch(ro) {
			case('b'):		/* Ab */
			case('e'):		/* Ae */
			case('s'):		/* As */
				if(strlen(argv))
					no_value_required_with_flag(flag,ro);
				break;
			case('d'):		/* Ad */
			case('t'):		/* At */
				if(sscanf(argv,"%lf",&factor)!=1)
					cant_read_numeric_value(flag,ro);
				break;
			default:
				if(sscanf(--argv,"%lf",&factor)!=1)  				/* A */
					unknown_flag(flag,ro);

		}
		break;
	case('a'):	 		/* a */
		ro = *argv++;
		switch(ro) {
		case('o'):		/* ao */
			break;
		case('T'):		/* aT */
		case('X'):		/* aX */
			if(sscanf(argv,"%s",temp)!=1) {
				fprintf(stdout,"ERROR: Cannot read string text parameter.\n");
				fflush(stdout);
				exit(1);
			}
			break;
		default:		/* a */
			if(sscanf(--argv,"%lf",&factor)!=1) {
				cant_read_numeric_value_with_flag_only(flag);
			}
			ro = 0;
			ifactor = round(factor);
			break;
		}
		break;
	case('B'):			/* B */
		if((cnt & 1))
			read_flags_data_error("Must have even number of 'bells' for bell-ringing, at present.");
		if(cnt > 32)
			read_flags_data_error("Too many bells (max 32, at present).");
		if(*argv!=ENDOFSTR)
			flag_only_takes_no_params(flag);
		break;
	case('b'):
		ro = (int)*argv++;
		switch(ro) {
			case('b'):		/* bb */
				break;
			case('g'):	 	/* bg */
			case('l'):		/* bl */
				if(sscanf(argv,"%lf",&factor)!=1)
					cant_read_numeric_value(flag,ro);
				break;
			default:		
				unknown_flag_string(argv-2);
		}
		break;
	case('C'):   			/* C */
		ro =*argv++;
		switch(ro) {
		case(ENDOFSTR):	case('a'):	case('m'):	case('c'):
			break;
		}
		break;
	case('c'):
		ro = *argv++;
		switch(ro) {
		case(ENDOFSTR):
			break;			/* c */
		case('l'):		 	/* cl */
		case('c'):			/* cc */
			if(*argv!=ENDOFSTR)
				flag_takes_no_params(flag,ro);
			break;
		case('s'):			/* cs */
		case('t'):			/* ct */
			if(sscanf(argv,"%lf",&factor)!=1)
				no_value_with_flag(flag,ro);
			break;
		default:
			if(isdigit(ro) || ro=='.' || ro=='-') {
				--argv;
				if(sscanf(argv,"%lf",&thresh)!=1)
					cant_read_numeric_value(flag,ro);
				else {
					ro = 0;
					condit = '>';
				}
				break;
			}
			unknown_flag(flag,ro);
		}
		break;
	case('D'):
		ro = (int)*argv++;
		switch(ro) {
			case('B'):	/* DB */
			case('s'):	/* Ds */
				break;
			default:
				unknown_flag(flag,ro);
		}
		break;
	case('d'):
		ro = (int)*argv++;
		switch(ro) {
			case('v'):	 	/* dv */
			case('l'):	 	/* dl */
			case('o'):	 	/* do */
			case('O'):	 	/* dO */
				if(sscanf(argv,"%d",&ifactor)!=1)
					cant_read_numeric_value(flag,ro);
				break;
			case('i'):	 	/* di */
				if(sscanf(argv,"%lf",&factor)!=1)
					cant_read_numeric_value(flag,ro);
				break;
			case('b'):	 	/* db */
			case('d'):	 	/* dd */
			case('g'):	 	/* dg */
			case('h'):	 	/* dh */
			case('L'):	 	/* dL */
			case('M'):	 	/* dM */
				break;
			default:
				if(isalpha(ro))
					unknown_flag(flag,ro);
				else {
					if(sscanf(argv-1,"%lf",&factor)!=1)
						cant_read_numeric_value_with_flag_only(flag);
					ro = ENDOFSTR; 	/* d */
				}
				break;
		}
		break;
	case('E'):	 		/* E */
		if(sscanf(argv,"%lf",&factor)!=1)
		   factor = 0.0;
		break;
	case('e'):
		ro = (int)*argv;
		if(ro==ENDOFSTR)
			no_value_with_flag_only(flag);
		if(isdigit(ro) || ro=='.' || ro=='-') {
			if(sscanf(argv,"%lf",&factor)!=1)
				cant_read_numeric_value_with_flag_only(flag);
			ro = ENDOFSTR; 	/* e */
			break;
		}
		argv++;
		switch(ro) {
			case('e'):	 	/* ee */
			case('b'):	 	/* eb */
			case('Q'):		/* eQ */
			case('L'):	 	/* eL */
			case('w'):	 	/* ew */
			case('W'):	 	/* eW */
			case('X'):	 	/* eX */
			case('Y'):	 	/* eY */
				if(*argv!=ENDOFSTR)
					read_flags_data_error("No parameter required with this option.");
				break;
			case('a'):	 	/* ea */
			case('d'):	 	/* ed */
			case('g'):	 	/* eg */
			case('l'): 	 	/* el */
			case('f'):	 	/* ef */
			case('t'):	 	/* et */
			case('x'):	 	/* ex */
			case('v'):	 	/* ev */
				if(sscanf(argv,"%lf",&factor)!=1)
					cant_read_numeric_value(flag,ro);
				break;
			case('i'):	 	/* ei */
			case('m'):	 	/* em */
			case('s'): 	 	/* es */
			case('A'): 	 	/* eA */
			case('S'): 	 	/* eS */
				if(*argv == ENDOFSTR)
					factor = 0.0;
				else {
					if(sscanf(argv,"%lf",&factor)!=1) {
						fprintf(stdout,"ERROR: Cannot read numerical value.\n");
						fflush(stdout);
						exit(1);
					}
					if(factor < 0.0) {
						fprintf(stdout,"ERROR: Cannot insert at a time before zero.\n");
						fflush(stdout);
						exit(1);
					}
				}
				break;
			case('D'): 	 	/* eD */
				if(sscanf(argv,"%lf",&factor)!=1) {
					fprintf(stdout,"ERROR: Cannot read numerical value.\n");
					fflush(stdout);
					exit(1);
				}
				if(factor < 0.0) {
					fprintf(stdout,"ERROR: Cannot delay to a time before zero.\n");
					fflush(stdout);
					exit(1);
				}
				break;
			case('P'): 	 	/* eP */
				if(sscanf(argv,"%d",&ifactor)!=1) {
					fprintf(stdout,"ERROR: Cannot read line number.\n");
					fflush(stdout);
					exit(1);
				}
				if(ifactor < 1 || ifactor > cnt) {
					fprintf(stdout,"ERROR: Line %d does not exist.\n",ifactor);
					fflush(stdout);
					exit(1);
				}
				if(ifactor == 1) {
					fprintf(stdout,"ERROR: Cannot plateau at line 1 (No change to file).\n");
					fflush(stdout);
					exit(1);
				}
				break;
			default:		
				unknown_flag_string(argv-2);
		}
		break;
/*RWD new Format option */
	case('F'):
		ro = (int)*argv;
		if(ro=='r' || ro == 'f' || ro == 'R')
			argv++;
		else
			ro = ENDOFSTR;
		if(((int)*argv) == ENDOFSTR)
			no_value_with_flag_only(flag);
		if(sscanf(argv,"%d",&ifactor)!=1)
			cant_read_numeric_value_with_flag_only(flag);
		if(ifactor <= 0)
			read_flags_data_error("Value must be > zero.");
		if(!sloom) {
			if(ifactor > 8)
				read_flags_data_error("Maximum of 8 values possible on a line.");
		}
		break;
	case('f'):   
		ro = *argv++;
		switch(ro) {
		case('t'):	 		/* ft */
			break;
		case('r'):	 		/* fr */
			if(sscanf(argv,"%d",&ifactor)!=1)
				ifactor = 1;
			break;
		case('l'):	 		/* fl */
			if(sscanf(argv,"%lf",&factor)!=1)
				no_value_with_flag(flag,ro);
			break;
		default:
			unknown_flag(flag,ro);
		}
		break;
	case('G'):				/* G */
		if(sscanf(argv,"%d",&ifactor)!=1)
			no_value_with_flag_only(flag);
		if(ifactor >= stringscnt) {
			sprintf(errstr,"ERROR: regrouping value (%d) is too big for list of numbers (only %d in list).\n",ifactor,cnt);
			do_error();
		}
		break;
	case('g'):
		ro = *argv++;
		switch(ro) {
		case(ENDOFSTR):		/* g  */
		case('a'):			/* ga */
			break;
		case('s'):			/* gs */
			factor = (double)MAXSAMP;
			break;
		default:
			unknown_flag_or_bad_param();
		}
		break;
 	case('H'):
		if((ro = (int)*argv++)!='g' && ro !='r') {
			if(isalpha(ro))
				unknown_flag(flag,ro);
			if(sscanf(--argv,"%d",&ifactor)!=1)
				no_value_with_flag_only(flag);
			ro = ENDOFSTR;
		} else {
			switch(ro) {
				case('r'):
					if(sscanf(argv,"%d",&ifactor)!=1)
						no_value_with_flag(flag,ro);
					break;
				case('g'):
					if(sscanf(argv,"%lf",&factor)!=1)
						factor = HARMONIC_TUNING_TOLERANCE;
					break;
				default:
					unknown_flag(flag,ro);
			}
		}
		break;
	case('h'):	 		
		ro = *argv++;
		switch(ro) {
		case('d'):		/* hd */
		case('M'):		/* hM */
			break;
		default:
			unknown_flag_string(argv-1);
		}
		if(*argv!=ENDOFSTR)
			read_flags_data_error("No parameter required with this flag.");
		break;
	case('I'):
		ro =*argv++;
		switch(ro) {
			case(ENDOFSTR):			break;   			/* I */
			case('r'):				break;   			/* Ir */
			case('v'):				break;   			/* Iv */
			case('V'):									/* IV */
				if(sscanf(argv,"%d",&ifactor)!=1)
					cant_read_numeric_value(flag,ro);
				break;   								
		}
		break;
	case('i'):
		ro = (int)*argv++;
		switch(ro) {
			case(ENDOFSTR):		/* i */
				break;
			case('M'):		/* iM */
			case('p'):		/* ip */
			case('v'):		/* iv */
			case('V'):		/* iV */
			case('h'):		/* ih */
				if(*argv!=ENDOFSTR)
					no_value_required_with_flag(flag,ro);
				break;
			case('='):		/* i= */
				if(cnt!=2) {
					if(!sloom)
						read_flags_data_error("Must have two values in infile with flag i=.");
					exit(1);
				}
				if(flteq(number[0],number[1])) {
					if(!sloom)
						read_flags_data_error("Two values in infile must be different.");
					exit(1);
				}
				if(sscanf(argv,"%lf",&factor)!=1)
					cant_read_numeric_value(flag,ro);
				if(flteq(factor,0.0))
					read_flags_data_error("Parameter cannot be zero.");
				break;
			case('r'):		/* ir */
			case('c'):		/* ic */
				if(sscanf(argv,"%d",&ifactor)!=1)
					cant_read_numeric_value(flag,ro);
				if(flag=='c' && cnt!=1)
					read_flags_data_error("Must have only one value in infile.");
				break;
			case('a'):		/* ia */
			case('m'):		/* im */
			case('l'):		/* il */
			case('L'):		/* iL */
				if(sscanf(argv,"%lf",&factor)!=1)
					cant_read_numeric_value(flag,ro);
				break;
			default:
				if(isdigit(ro) || ro=='.' || ro=='-')
					flag_only_takes_no_params(flag);
				unknown_flag_string(argv-2);
		}
		break;
	case('J'):
		ro = *argv;
		if(ro=='j') {							/*Jj */	
			if(sscanf(++argv,"%d",&ifactor)!=1)
				no_value_with_flag_only(flag);
		} else if(*argv!=ENDOFSTR)   			/* J */
			flag_only_takes_no_params(flag);
		break;
	case('j'):
		ro = *argv;
		if(ro == 'j')		/* jj */
			break;
		else {				/* j */
			ro = 0;
			maxpos = (cnt/colcnt) + 1;
			if(sscanf(argv,"%d",&ifactor)!=1)
				no_value_with_flag_only(flag);
		   	else if(ifactor > maxpos) {
				fprintf(stdout,"ERROR: Insertion point is beyond end of file: reverting to end of file.\n");
				fflush(stdout);
				ifactor = maxpos;
			}
		}
		break;
   case('L'):			/* L */
		if(cnt!=2)
			read_flags_data_error("Must have two values in infile.");
		if(flteq(number[0],number[1]))
			read_flags_data_error("Two values in infile must be different.");
		if(number[0]<=0 || number[1]<=0)
			read_flags_data_error("This option only works on values >0.\n");
		if(sscanf(argv,"%lf",&factor)!=1)
			cant_read_numeric_value_with_flag_only(flag);
		if(flteq(factor,0.0))
			read_flags_data_error("Parameter cannot be zero.");
		break;
	case('l'):
		ro =*argv++;
		switch(ro) {
		case(ENDOFSTR):			break;   			/* l */
		case('i'):									/* li */
		case('v'):									/* lv */
			if(sscanf(argv,"%lf",&factor)!=1)
				no_value_with_flag(flag,ro);
			break;
		default:
			unknown_flag(flag,ro);
		}
		break;
	case('k'):
		ro =*argv++;
		switch(ro) {
		case('A'):									/* kA */
		case('B'):									/* kB */
		case('C'):									/* kC */
		case('T'):									/* kT */
		case('X'):									/* kX */
		case('Y'):									/* kY */
			break;
		default:
			unknown_flag(flag,ro);
		}
		break;
	case('K'):
		ro =*argv++;
		switch(ro) {
		case('c'): case('t'): case('E'): 
		case('l'): case('b'): case('m'): case('M'):
			if(sscanf(argv,"%lf",&factor)!=1)
				read_flags_data_error("Cannot read parameter value.");
			break;
		case('I'): case('i'): case('P'): case('A'): case('R'):	case('r'):
		case('Z'): case('T'): case('a'): 
			break;
		default:
			fprintf(stdout,"ERROR: Unknown option\n");
			fflush(stdout);
			exit(1);
		}
		break;
	case('M'):
		ro = *argv++;

		switch(ro) {
			case(ENDOFSTR):	/* M  */
			case('d'):	 	/* Md */
				break;
			case('h'):	 	/* Mh */
				if(*argv!=ENDOFSTR)
					no_value_required_with_flag(flag,ro);
				break;
			case('m'): 	 	/* Mm */
				if((p=get_pitchclass(argv,&ifactor,0))==(char *)0) {
					sprintf(errstr,"key not specified, or unknown (%s)\n",argv);
					do_error();
				}
				break;
			case('t'): 	 	/* Mt */
				break;
			case('r'): 	 	/* Mr */
				if(sscanf(argv,"%d",&ifactor)!=1)
					ifactor = 1;
				break;
			default:
				if(isdigit(ro) || ro=='.' || ro=='-')
					no_value_required_with_flag_only(flag);
				unknown_flag(flag,ro);
		}
		break;
	case('m'):	   
		ro = *argv++;
		switch(ro) {
			case('i'):		/* mi */
			case('I'):		/* mI */
				break;
			case('M'):		/* mM */
				if((p=get_pitchclass(argv,&ifactor,0))==(char *)0) {
					sprintf(errstr,"key not specified, or unknown (%s)\n",argv);
					do_error();
				}
				break;
			case('g'): 		/* mg */
			case('l'): 		/* ml */
			case('e'): 		/* me */
			case('m'): 		/* mm */
				if(sscanf(argv,"%lf",&factor)!=1) {
					cant_read_numeric_value(flag,ro);
				}
				if(ro == 'm' && flteq(factor,0.0)) {
					fprintf(stdout,"ERROR: Mutiples of numbers very close to zero cannot be handled.\n");
					fflush(stdout);
					exit(1);
				}	
				ifactor = round(factor);
				break;
			default: 		/* m */
				if(sscanf(--argv,"%lf",&factor)!=1) {
					cant_read_numeric_value_with_flag_only(flag);
				}
				ro = 0;
				break;
		}
		break;
	case('N'):	 		/* N */
		ro = *argv;
		if(ro == 'r')		/* Nr */
			argv++;
		if(*argv==ENDOFSTR) {
			if(ro=='r')
				cant_read_numeric_value(flag,ro);
			else
				cant_read_numeric_value_with_flag_only(flag);
		}
		if(ro!='r' && !isdigit(ro))
			unknown_flag(flag,ro);
		if(sscanf(argv,"%d",&outfilecnt)!=1) {
			if(ro=='r')
				cant_read_numeric_value(flag,ro);
			else
				cant_read_numeric_value_with_flag_only(flag);
		}
		if(outfilecnt<2)
			read_flags_data_error("Number of partitioning sets must be >1.");
		if(outfilecnt > 99)
			read_flags_data_error("You can't have more than 99 outfiles (at present).");
		len = strlen(argv2);
		filename = (char *)exmalloc(len+1);
		*filename = ENDOFSTR;
		strcpy(filename,argv2);
		len+=2;
		if(factor>9)
			len++;
		thisfilename = (char *)exmalloc(len+1);
		break;
	case('n'):	 		/* n */
		ro = *argv++;
		switch(ro) {
		case('F'):		/* nF */
		case('M'):		/* nM */
			break;
		case('T'):		/* nT */
			switch(*argv++) {
			case('c'): case('C'): factor = 60.0;	break;
			case('d'): case('D'): factor = 62.0;	break;
			case('e'): case('E'): factor = 64.0;	break;
			case('f'): case('F'): factor = 65.0;	break;
			case('g'): case('G'): factor = 67.0;	break;
			case('a'): case('A'): factor = 69.0;	break;
			case('b'): case('B'): factor = 71.0;	break;
			default: 
				fprintf(stdout,"ERROR: Cannot read note parameter.\n");
				fflush(stdout);
				exit(1);
			}
			switch(*argv) {
			case('b'): factor -= 1.0; argv++; break;
			case('#'): factor += 1.0; argv++; break;
			case(ENDOFSTR): break;
			}
			ifactor = 0;
			switch(*argv) {
			case(ENDOFSTR):	break;
			default:
				qtone = 0.0;
				q = argv + strlen(argv) - 1;
				if(*q == '-') {
					qtone = -.5;
					*q = ENDOFSTR;
				} else if(*q == '+') {
					qtone = .5;
					*q = ENDOFSTR;
				}
				if(sscanf(argv,"%d",&ifactor)!=1) {
					fprintf(stdout,"ERROR: Invalid octave data in note parameter.\n");
					fflush(stdout);
					exit(1);
				}
				break;
			}
			factor += (double)(12 * ifactor);
			factor += qtone;
			break;
		default:
			unknown_flag_string(argv-1);
		}
		break;		
	case('O'):		
		ro = *argv++;
		switch(ro) {
		case('s'):		/* Os */
			factor = 12;
			break;
		case('T'):		/* OT */
			break;
		default:
			unknown_flag(flag,ro);
		}
		break;
	case('o'):   			
		ro = *argv++;
		switch(ro) {
		case(ENDOFSTR):		/* o  */
		case('r'):			/* or */
			break;
		default:
			unknown_flag_or_bad_param();
		}
		break;
	case('p'):   			/* p */
		ro = *argv++;
		switch(ro) {
		case(ENDOFSTR):		/* p  */
		case('p'):			/* pp */
			break;
		default:
			unknown_flag_string(argv-1);
		}
		break;
	case('P'):	 		/* P */
	case('q'):	 		/* q */
		ro = (int)*argv;
		if(ro==ENDOFSTR)
			no_value_with_flag_only(flag);
		if(isdigit(ro) || ro=='.' || ro=='-') {
			if(sscanf(argv,"%lf",&factor)!=1)
				cant_read_numeric_value_with_flag_only(flag);
			ro = ENDOFSTR; 	/* q */
			break;
		}
		switch(ro) {
		case('t'):
		case('v'):
			argv++;
			if(sscanf(argv,"%lf",&factor)!=1) {
				cant_read_numeric_value(flag,ro);
				exit(1);
			}
			break;
		default:
			unknown_flag_string(argv-1);
			break;
		}
		break;
	case('Q'):			/* Q */
		if(cnt!=3)
			read_flags_data_error("Must have three values in infile.");
		if(flteq(number[0],number[1]))
			read_flags_data_error("First two values in infile must be different.");
		if(number[2]<=0.0)
			read_flags_data_error("Curvature (third value in infile) must be >0.0.");
		if(sscanf(argv,"%lf",&factor)!=1)
			cant_read_numeric_value_with_flag_only(flag);
		if(flteq(factor,0.0))
			read_flags_data_error("Parameter cannot be zero.");
		break;
	case('R'):
		ro = (int)*argv++;
		switch(ro) {
			case(ENDOFSTR):	/* R */
				factor = 1.0;
				break;
			case('g'):		/* Rg */
			case('G'):		/* RG */
				if(*argv!=ENDOFSTR)
					read_flags_data_error("No parameter required with this option.");
				break;
		case('o'):		/* Ro */
		case('i'):		/* Ri */
		case('I'):		/* RI */
		case('r'):		/* Rr */
			if(*argv!=ENDOFSTR) {
				fprintf(stdout,"ERROR: No parameter required with this option\n");
				fflush(stdout);
				exit(1);
			}
			break;
		case('e'):		/* Re */
			if(sscanf(argv,"%d",&ifactor)!=1)
				no_value_with_flag(flag,ro);
			break;
		case('a'):		/* Ra */
		case('A'):		/* RA */
		case('c'):		/* Rc */
		case('m'):		/* Rm */
		case('s'):		/* Rs */
		case('v'):		/* Rv */
		case('x'):		/* Rx */
			if(sscanf(argv,"%lf",&factor)!=1)
				cant_read_numeric_value(flag,ro);
			if(ro=='s' && (factor > 1.0 || factor < 0.0))
				read_flags_data_error("Numerical value out of range [0-1].");
			else if(ro == 'x') {
				ifactor = round(factor);
				if(ifactor < 1)
					read_flags_data_error("Numerical value out of range [must be >= 1].");
			}
			break;
		default:
			if(sscanf(argv-1,"%lf",&factor)!=1)
				unknown_flag_string(argv-2);
			ro = ENDOFSTR;
		}
		break;
	case('r'):   
		ro = *argv++;
		switch(ro) {
		case(ENDOFSTR):	/* r */
			break;
		case('m'):		/* rm */
			if(sscanf(argv,"%d",&ifactor)!=1)
				no_value_with_flag(flag,ro);
			break;
		case('T'):			/* rT */
			factor = 1.0;
			/* fall thro */
		case('a'):			/* ra */
		case('G'):			/* rG */
		case('I'):			/* rI */
		case('J'):			/* rJ */
		case('o'):			/* ro */
		case('p'):			/* rp */
		case('r'):			/* rr */
		case('R'):			/* rR */
		case('X'):			/* rX */
		case('z'):			/* rz */
			if(*argv!=ENDOFSTR)
				no_value_required_with_flag(flag,ro);
			break;
		default:
			unknown_flag(flag,ro);
		}
		break;
	case('S'):	 			/* S */
		ro = *argv;
		switch(ro) {
		case('M'):			/* SM */
			argv++;		/* NB need to do this here if there are more 'S' flags!! */
			if(*argv!=ENDOFSTR)
				no_value_required_with_flag(flag,ro);
			break;
		default:
			if(*argv==ENDOFSTR) {
				cant_read_numeric_value_with_flag_only(flag);
				exit(1);
			}
			if(sscanf(argv,"%d",&outfilecnt)!=1) {
				cant_read_numeric_value_with_flag_only(flag);
				exit(1);
			}
			if(outfilecnt<2)
				read_flags_data_error("Number of partitioning sets must be >1.");
			if(outfilecnt > 99)
				read_flags_data_error("You can't have more than 99 outfiles (at present).");
			len = strlen(argv2);
			filename = (char *)exmalloc(len+1);
			*filename = ENDOFSTR;
			strcpy(filename,argv2);
			len+=2;
			if(factor>9)
				len++;
			thisfilename = (char *)exmalloc(len+1);
			ro = ENDOFSTR;
			break;
		}
		break;
	case('s'):
		ro = *argv++;
		switch(ro) {
		case('R'):		/* sR */
		case('T'):		/* sT */
		case('F'):		/* sF */
		case('X'):		/* sX */
		case('a'):		/* sa */	
		case('b'):		/* sb */
		case('f'):		/* sf */
			break;
		case('O'):		/* sO */
			factor = .083333333333;		/* i.e. 1/12 */
			break;
		case('g'):		/* sg */
			factor = 1.0/MAXSAMP;
			break;
		case('d'):		/* sd */
			if(*argv == ENDOFSTR) {
				factor = 0.0;
				break;
			}
			/* fall thro */
						/* sd with param */
		case('A'):		/* sA */
			if(sscanf(argv,"%lf",&factor)!=1) 
				cant_read_numeric_value(flag,ro);
			break;
		case('t'):		/* st */
			if(*argv == ENDOFSTR)
				no_value_with_flag(flag,ro);
			else if(sscanf(argv,"%d",&ifactor)!=1)
				cant_read_numeric_value(flag,ro);
			else if(BAD_SR(ifactor))
				read_flags_data_error("Invalid sampling rate.");
			break;
		case('P'):		/* sP */
		case('p'):		/* sp */
		case('o'):		/* so */
		case('u'):		/* su */
		case('k'):		/* sk */
		case('l'):		/* sl */
		case('K'):		/* sK */
		case('n'):		/* sn */
		case('E'):		/* sE */
			if(*argv == ENDOFSTR)
				no_value_with_flag(flag,ro);
			else if(sscanf(argv,"%lf",&factor)!=1)
				cant_read_numeric_value(flag,ro);
			break;
		case('U'):		/* sn */
			if(*argv == ENDOFSTR)
				no_value_with_flag(flag,ro);
			else if(sscanf(argv,"%s",string)!=1) {
				fprintf(stdout,"ERROR: Cannot read string text parameter.\n");
				fflush(stdout);
				exit(1);
			}
			break;
		case('z'):		/* sz */
			ifactor = *argv;
			if(ifactor == ENDOFSTR)
				factor = 0.0;
			else {
				if(sscanf(argv,"%lf",&factor)!=1) {
					cant_read_numeric_value(flag,ro);
					exit(1);
				}
			}
			break;
		case(ENDOFSTR):	 /* s */
			factor = 0.0;
			break;
		default:		 /* s with parameter */
			if(isdigit(ro) || ro=='.' || ro=='-') {	  
				argv--;
				if(sscanf(argv,"%lf",&factor)!=1) {
					cant_read_numeric_value(flag,ro);
					exit(1);
				}
				ro = ENDOFSTR;
			} else
				unknown_flag(flag,ro);
		}
		break;
	case('T'):
		if((ro = *argv++)==ENDOFSTR)
			unknown_flag_only(flag);
		switch(ro) {
			case('M'): 		/* TM */
			case('h'): 		/* Th */
				if(sscanf(argv,"%lf",&factor)!=1)
					factor = TWELVE;	/* Normal tempering */
				break;
			case('c'): 		/* Tc */
			case('l'): 		/* Tl */
				if(sscanf(argv,"%lf",&factor)!=1)
					read_flags_data_error("Cannot read tempo value.");
				break;
			case('p'): 		/* Tp */
			case('P'): 		/* TP */
				if(sscanf(argv,"%lf",&factor)!=1)
					read_flags_data_error("Cannot read beat duration.");
				break;
			case('r'): 		/* Tr */
				factor = 1.0;
				/* fall thro */
			case('b'):		/* Tb */
			case('B'): 		/* TB */
			case('O'): 		/* TO */
			case('s'): 		/* Ts */
				break;
			default:
				unknown_flag(flag,ro);
		}
		break;
	case('t'):
		ro = *argv++;
		switch(ro) {
		case(ENDOFSTR):		/* t */
			break;
		case('s'):			/* ts */
			if(*argv == ENDOFSTR)
				no_value_with_flag(flag,ro);
			else if(sscanf(argv,"%d",&ifactor)!=1) {
				cant_read_numeric_value(flag,ro);
			} else if(BAD_SR(ifactor))
				read_flags_data_error("Invalid sampling rate.");
			break;
 		case('e'):		/* te */
		case('M'): 		/* tM */
		case('h'): 		/* th */
		case('r'): 		/* tr */
		case('R'): 		/* tR */
			if(*argv!=ENDOFSTR)
				no_value_required_with_flag(flag,ro);
			break;
		case('d'):
			if(sscanf(argv,"%d",&ifactor)!=1)
				read_flags_data_error("Cannot read density value.");
			break;
 		case('B'):			/* tB */
 		case('c'):			/* tc */
 		case('C'):			/* tC */
			if(sscanf(argv,"%lf",&factor)!=1)
				read_flags_data_error("Cannot read parameter value.");
			break;
		default:
			if(isdigit(ro) || ro=='.' || ro=='-')
				no_value_required_with_flag_only(flag);
			unknown_flag(flag,ro);
		}
		break;
	case('V'):
		if(sscanf(argv,"%lf",&factor)!=1)
			no_value_with_flag_only(flag);
		break;
   	case('v'):
		if(sscanf(argv,"%lf",&factor)!=1)
			factor = 0.0;
		break;	
	case('w'):
		ro =*argv++;
		switch(ro) {
		case('t'): case('o'):
			break;
		default:
			fprintf(stdout,"ERROR: Unknown option\n");
			fflush(stdout);
			exit(1);
		}
		break;
	case('W'):
		ro =*argv++;
		switch(ro) {
		case('c'): case('t'): case('E'): 
		case('l'): case('b'): case('m'): case('M'):
			if(sscanf(argv,"%lf",&factor)!=1)
				read_flags_data_error("Cannot read parameter value.");
			break;
		case('I'): case('i'): case('P'): case('A'): case('R'):	case('r'):
		case('Z'): case('T'): case('a'): 
			break;
		default:
			fprintf(stdout,"ERROR: Unknown option\n");
			fflush(stdout);
			exit(1);
		}
		break;
	case('y'):
		ro =*argv++;
		switch(ro) {
		case('a'):	case('A'):	case('b'):	case('B'):	case('c'):	case('C'):	
		case('d'):	case('e'):	case('E'):	case('g'):	case('i'):	case('I'):
		case('l'):	case('o'):	case('p'):	case('q'):	case('Q'):	case('r'):	case('R'):
		case('P'):	case('s'):	case('S'):	case('x'):	
			break;
		default:
			fprintf(stdout,"ERROR: Unknown option\n");
			fflush(stdout);
			exit(1);
		}
		break;
	case('Y'):
		ro =*argv++;
		switch(ro) {
		case('s'):	case('S'):	
			break;
		default:
			fprintf(stdout,"ERROR: Unknown option\n");
			fflush(stdout);
			exit(1);
		}
		break;
	case('z'):
		ro =*argv++;
		switch(ro) {
		case('A'):	case('B'):	case('C'):	case('D'):	case('e'):	case('E'):	case('F'):	case('G'):
		case('H'):	case('I'):	case('J'):	case('X'):	case('Y'):	case('Z'):	
		case('q'):
			break;
		default:
			fprintf(stdout,"ERROR: Unknown option\n");
			fflush(stdout);
			exit(1);
		}
		break;
	case('Z'):
		ro =*argv++;
		switch(ro) {
		case('h'):	case('M'):	
			if(sscanf(argv,"%lf",&factor)!=1)
				read_flags_data_error("Cannot read parameter value.");
			break;
		case('s'):	
			if(sscanf(argv,"%s",string)!=1)
				read_flags_data_error("Cannot read parameter value.");
			break;
		default:
			fprintf(stdout,"ERROR: Unknown option\n");
			fflush(stdout);
			exit(1);
		}
		break;
	default:
		unknown_flag_string(argv-1);
	}

	if(condit) {
		if(!(flag=='s' && (ro=='p' || ro=='P' || ro=='F' || ro=='R' || ro=='u' || ro=='X'))
		&& !(flag=='W' && (ro=='l' || ro=='M'))
		&& !(flag=='K' && (ro=='l' || ro=='M'))
		&& !(flag == 'm' && ro == 'm') && !(flag =='T' && ro == 'P')
		&& !(flag == 'l' && ro == 'v') 
		&& !(flag == 'd' && ro == 'L') && !(flag == 'T' && (ro == 'M' || ro == 'h')) 
		&& !(flag == 'c' && ro == 0)
		&& (ro!=0 || !(flag=='R'||flag=='a'||flag=='m'||flag=='P'||flag=='c'||flag=='d')))
			read_flags_data_error("Threshold cannot be used with this option.");
	}
}

/***************************** GET_EXTRA_PARAMS *********************************/

void get_extra_params(int no,int argc,char *argv[]) {
	int n, m;
	cnt = no;
	number=(double *)exrealloc((char *)number,(cnt+1)*sizeof(double));
	for(n=2,m=0;n < argc-1;n++,m++) {
		if(sscanf(argv[n],"%lf",&number[m])!=1) {
			fprintf(stdout,"ERROR: Insufficient parameters for this process.\n");
			fflush(stdout);
			exit(1);
		}
	}			
}

/***************************** GET_FUNNY_PARAMS *********************************/

void get_funny_params(int no,int argc,char *argv[]) {
	int n, m;
	number=(double *)exrealloc((char *)number,(cnt+no)*sizeof(double));
	for(n=2,m=cnt;n < argc-1;n++,m++) {
		if(sscanf(argv[n],"%lf",&number[m])!=1) {
			fprintf(stdout,"ERROR: Insufficient parameters for this process.\n");
			fflush(stdout);
			exit(1);
		}
	}			
}

/***************************** REORGANISE_EXTRA_PARAMS *********************************/

void reorganise_extra_params(char *agv,int argc,char *argv[])
{
	double range, step, scat, quantisation, dur;
	int tq, ql;

	agv++;
	flag = *agv;
	agv++;
	ro = *agv;
	switch(flag) {
	case('A'):
		switch(ro) {
		case('d'):	
		case('t'):	get_extra_params(3,argc,argv); 
					factor = number[0];
					number[0] = number[1];
					number[1] = number[2];
					cnt--;
					if(factor <= 0.0) {
						fprintf(stdout,"ERROR: Total duration must be greater than zero.\n");
						fflush(stdout);
						exit(1);
					}
					if((number[0]<=0) || (number[1]<=0)) {
						fprintf(stdout,"ERROR: Time steps must be greater than zero.\n");
						fflush(stdout);
						exit(1);
					}
					if(factor < number[0] + number[1]) {
						fprintf(stdout,"ERROR: Total duration too small to divide thus.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		case('T'):	get_extra_params(4,argc,argv); 
					factor = number[0];
					number[0] = number[1];
					number[1] = number[2];
					number[2] = number[3];
					cnt--;
					if((number[0]<=0) || (number[1]<=0)) {
						fprintf(stdout,"ERROR: Time steps must be greater than zero.\n");
						fflush(stdout);
						exit(1);
					}
					if(factor <= 0.0) {
						fprintf(stdout,"ERROR: Total duration must be greater than zero.\n");
						fflush(stdout);
						exit(1);
					}
					if(factor < number[0] + number[1]) {
						fprintf(stdout,"ERROR: Total duration too small to divide thus.\n");
						fflush(stdout);
						exit(1);
					}
					ro = 't';
					break;
		default: 
			fprintf(stdout,"ERROR: Unknown process mode.\n");
			fflush(stdout);
			exit(1);
		}
		break;
	case('b'):
		switch(ro) {
		case('b'):
			get_funny_params(2,argc,argv);
			break;
		}
		break;
	case('C'):
		switch(ro) {
		case('a'):	case('m'):
			get_funny_params(3,argc,argv);
			break;
		}
		break;
	case('c'):
		switch(ro) {
		case('s'):	
			get_extra_params(4,argc,argv);
			cnt = round(number[0]);
			if(cnt < 2) {
				fprintf(stdout,"ERROR: Number of output values must be more than 1.\n");
				fflush(stdout);
				exit(1);
			}
			if(number[3] < FLTERR) {
				fprintf(stdout,"ERROR: skew value must be greater than 0 (1.0 == NO skew).\n");
				fflush(stdout);
				exit(1);
			}
			break;
		}
		break;
	case('d'):
		switch(ro) {
		case('d'):
		case('L'):
			get_funny_params(2,argc,argv);
			break;
		}
		break;
	case('g'):
		switch(ro) {
		case('a'):
			get_funny_params(1,argc,argv);
			break;
		}
		break;
	case('i'):
		switch(ro) {
		case('e'):
		case('c'):	get_extra_params(2,argc,argv);
					ifactor = round(number[0]);
					number[0] = number[1];
					cnt--;
					if(ifactor <= 1) {
						fprintf(stdout,"ERROR: Number of values must be more than 1.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		case('R'):	
		case('Q'):	
		case('C'):	get_extra_params(3,argc,argv);
					ifactor = round(number[0]);
					number[0] = number[1];
					number[1] = number[2];
					cnt--;
					if(ifactor <= 1) {
						fprintf(stdout,"ERROR: Number of values must be more than 1.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		case('='):	get_extra_params(3,argc,argv);
					factor = number[0];
					number[0] = number[1];
					number[1] = number[2];
					cnt--;
					range = fabs(number[1] - number[0]);
					step  = fabs(factor);
					if(step > range) {
						fprintf(stdout,"ERROR: Range not large enough to accomodate the steps.\n");
						fflush(stdout);
						exit(1);
					}
					if(flteq(step,0.0)) {
						fprintf(stdout,"ERROR: Steps are too small to work with.\n");
						fflush(stdout);
						exit(1);
					}
					if(range/step >= 10000.0) {
						fprintf(stdout,"ERROR: More than 10000 steps: can't handle this.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		default: 
			fprintf(stdout,"ERROR: Unknown process mode.\n");
			fflush(stdout);
			exit(1);
		}
		break;
	case('L'):
		if(ro != ENDOFSTR) {
			fprintf(stdout,"ERROR: Unknown process mode.\n");
			fflush(stdout);
			exit(1);
		}
		get_extra_params(3,argc,argv);
		factor = (double)round(number[0]);
		if(factor <= 0.0) {
			fprintf(stdout,"ERROR: Number of divisions must be greater than zero.\n");
			fflush(stdout);
			exit(1);
		}
		number[0] = number[1];
		number[1] = number[2];
		if(flteq(number[0],number[1])) {
			fprintf(stdout,"ERROR: The range to divide up is too small.\n");
			fflush(stdout);
			exit(1);
		}
		if(number[0] <= 0.0 || number[1] <= 0.0) {
			fprintf(stdout,"ERROR: The range must be between two positive numbers.\n");
			fflush(stdout);
			exit(1);
		}
		cnt--;
		break;
	case('K'):
		tq = 2;
		if(ro == 'b')
			tq = 3;
		if(sscanf(argv[tq],"%d",&ifactor)!=1) {
			fprintf(stdout,"ERROR: Failed to get number of soundfiles in sequence data.\n");
			fflush(stdout);
			exit(1);
		}			
		break;
	case('p'):
		if (ro == 'p') {
			get_extra_params(7,argc,argv);
			break;
		}	
		break;
	case('q'):
		if (ro == 'z') {
			get_extra_params(4,argc,argv);
			break;
		}	
		break;
	case('Q'):
		if(ro != ENDOFSTR) {
			fprintf(stdout,"ERROR: Unknown process mode.\n");
			fflush(stdout);
			exit(1);
		}
		get_extra_params(4,argc,argv);
		factor = (double)round(number[0]);
		number[0] = number[1];
		number[1] = number[2];
		number[2] = number[3];
		cnt--;
		if(flteq(number[0],number[1])) {
			fprintf(stdout,"ERROR: Parameters 2 and 3 must be different.\n");
			fflush(stdout);
			exit(1);
		}
		if(number[2]<=0.0) {
			fprintf(stdout,"ERROR: Curvature (4th parameter) must be > 0.0.\n");
			fflush(stdout);
			exit(1);
		}
		if(factor <= 0.0) {
			fprintf(stdout,"ERROR: Number of steps cannot be zero\n");
			fflush(stdout);
			exit(1);
		}
		break;
	case('R'):
		switch(ro) {
		case('g'):	get_extra_params(1,argc,argv);
					if(round(number[0])<=0) {
						fprintf(stdout,"ERROR: Number of items to generate must be 1 or more.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		case('G'):	get_extra_params(1,argc,argv);
					if(round(number[0]) <= 0) {
						fprintf(stdout,"ERROR: Number of items to generate must be 1 or more.\n");
						fflush(stdout);
						exit(1);
					}
					if(round(number[1]) <= 1) {
						fprintf(stdout,"ERROR: Number of consecutive repeats must be 2 or more.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		case('i'):	get_extra_params(2,argc,argv);
					if(round(number[0])<=0) {
						fprintf(stdout,"ERROR: Number of items to generate must be 1 or more.\n");
						fflush(stdout);
						exit(1);
					}
					if(round(number[1])==1) {
						fprintf(stdout,"ERROR: All numbers will be the same (1).\n");
						fflush(stdout);
						exit(1);
					}
					break;
		case('I'):	get_extra_params(3,argc,argv);
					if(round(number[0])<=0) {
						fprintf(stdout,"ERROR: Number of items to generate must be 1 or more.\n");
						fflush(stdout);
						exit(1);
					}
					if(round(number[1])==1) {
						fprintf(stdout,"ERROR: All numbers will be the same (1).\n");
						fflush(stdout);
						exit(1);
					}
					if(round(number[2])<1) {
						fprintf(stdout,"ERROR: Number of allowable repetitions must be 1 or more.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		case('r'):	get_extra_params(3,argc,argv);
					if(round(number[0])<=0) {
						fprintf(stdout,"ERROR: TOtal duratihn must be > 0.\n");
						fflush(stdout);
						exit(1);
					}
					if(round(number[1])<1) {
						fprintf(stdout,"ERROR: Must be more than one number to generate.\n");
						fflush(stdout);
						exit(1);
					}
					if(round(number[2])<=0) {
						fprintf(stdout,"ERROR: Randomisation value must be > 0.\n");
						fflush(stdout);
						exit(1);
					}
					if(round(number[2])>round(number[1])) {
						fprintf(stdout,"ERROR: Randomisation Value cannot be greater than number of values to generate.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		case('c'):
		case('v'):	get_extra_params(3,argc,argv); 
					factor = number[0];
					number[0] = number[1];
					number[1] = number[2];
					cnt--;
					if(ro == 'v') {
						if(round(factor) < 1.0) {
							fprintf(stdout,"ERROR: Number of items to generate must be 1 or more.\n");
							fflush(stdout);
							exit(1);
						}
					} else {	/* case 'c' */
						if (number[0] <= 0.0 || number[1] <= 0.0) {
							fprintf(stdout,"ERROR: Chunk sizes must all be larger than zero.\n");
							fflush(stdout);
							exit(1);
						}

						if (number[1] < number[0]) {
							number[2] = number[0];
							number[0] = number[1];
							number[1] = number[2];
						}
						if(factor <= 0.0) {
							fprintf(stdout,"ERROR: Item to cut must be greater than zero.\n");
							fflush(stdout);
							exit(1);
						}
						if(factor < number[0]) {
							fprintf(stdout,"ERROR: Chunk sizes are too large.\n");
							fflush(stdout);
							exit(1);
						}
						if(factor < number[1]) {
							number[1] = factor;
						}
					}
					break;
		default: 
			fprintf(stdout,"ERROR: Unknown process mode.\n");
			fflush(stdout);
			exit(1);
		}
		break;
	case('r'):
		switch(ro) {
		case('G'):	get_extra_params(3,argc,argv);
					if(round(number[0]) <= 0) {
						fprintf(stdout,"ERROR: Number of items to generate must be 1 or more.\n");
						fflush(stdout);
						exit(1);
					}
					if((int)round(number[1]) == (int)round(number[2])) {
						fprintf(stdout,"ERROR: Two numbers to randomise must be different from each other.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		case('J'):	get_extra_params(4,argc,argv);
					if(round(number[0]) <= 0) {
						fprintf(stdout,"ERROR: Number of items to generate must be 1 or more.\n");
						fflush(stdout);
						exit(1);
					}
					if((int)round(number[1]) == (int)round(number[2])) {
						fprintf(stdout,"ERROR: Two numbers to randomise must be different from each other.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		case('z'):	get_extra_params(6,argc,argv);
					if(round(number[0])<3) {
						fprintf(stdout,"ERROR: Number of items to generate must be 3 or more.\n");
						fflush(stdout);
						exit(1);
					}
					if(round(number[1]) == round(number[2])) {
						fprintf(stdout,"ERROR: All 'random' numbers will be the same (%d).\n",(int) round(number[1]));
						fflush(stdout);
						exit(1);
					}
					if(round(number[5])<1) {
						fprintf(stdout,"ERROR: Number of allowable repetitions must be 1 or more.\n");
						fflush(stdout);
						exit(1);
					}
					break;
		default: 
			fprintf(stdout,"ERROR: Unknown process mode.\n");
			fflush(stdout);
			exit(1);
		}
		break;

	case('s'):
		switch(ro) {
		case('a'):	case('b'):	case('f'):
			get_funny_params(2,argc,argv);
			break;
		case('s'):
			get_extra_params(5,argc,argv);
			if(flteq(number[0],number[1])) {
				fprintf(stdout,"WARNING: Maximum and minimum vals are equal: no sinusoidal movement.\n");
				fflush(stdout);
			}
			if(number[3] <= 0.0) {
				fprintf(stdout,"ERROR: No of periods is <= 0. Cannot proceed\n");
				fflush(stdout);
				exit(1);
			}
			if(number[4] <= 2.0) {
				fprintf(stdout,"ERROR: No of values per period is <= 2. No sinusoid will be produced\n");
				fflush(stdout);
				exit(1);
			}
			if (number[3] * number[4] < 1.0) { 
				fprintf(stdout,"ERROR: No of cycles too short to produce any output with this no. of vals per cycle.\n");
				fflush(stdout);
				exit(1);
			}
			break;
		case('c'):	case('v'):	case('x'):
			get_extra_params(5,argc,argv);
			if(flteq(number[0],number[1])) {
				fprintf(stdout,"ERROR: Maximum and minimum vals are equal: no sinusoidal movement.\n");
				fflush(stdout);
				exit(1);
			}
			if(flteq(number[2],number[3])) {
				fprintf(stdout,"ERROR: Start and end times equal .. no sinusoidal movement.\n");
				fflush(stdout);
				exit(1);
			}
			if(number[2] > number[3]) {
				dur = number[2];
				number[2] = number[3];
				number[3] = dur;
			}
			if(number[2] < 0.0) {
				fprintf(stdout,"ERROR: Time value less than zero.\n");
				fflush(stdout);
				exit(1);
			}
			if (ro == 'c') {
				if(number[4] <= 3) {
					fprintf(stdout,"ERROR: No of points < 3. Cannot define sinusoidal motion.\n");
					fflush(stdout);
					exit(1);
				}
			} else {
				if(number[4] <= 5) {
					fprintf(stdout,"ERROR: No of points < 5. Cannot define cosinusoidal motion.\n");
					fflush(stdout);
					exit(1);
				}
			}
			break;
		}
		break;
	case('S'):
		switch(ro) {
		case('M'):
			get_funny_params(4,argc,argv);
			break;
		}
		break;
	case('T'):
		switch(ro) {
		case('b'):	
			do_string_params(argv,2);
			break;
		case('B'):	
			do_string_params(argv,3);
			break;
		}
		break;
	case('y'):
		switch(ro) {
		case('o'):	case('p'):	case('s'):	case('B'):	case('E'):	case('S'):
			get_funny_params(1,argc,argv);
			break;
		case('e'):	case('l'):	case('g'):	case('I'):	case('A'):	case('C'): case('x'):
			get_funny_params(2,argc,argv);
			break;
		case('R'):	case('a'):	case('b'):	case('c'):
			get_funny_params(3,argc,argv);
			break;
		case('q'):	case('Q'):
			get_funny_params(4,argc,argv);
			break;
		case('P'):
			get_funny_params(5,argc,argv);
			break;
		case('i'):	
			get_funny_params(2,argc,argv);
			ifactor = (int)round(number[cnt]);
			factor = number[cnt+1];
			if(ifactor < 1 || ifactor > cnt+1) {
				fprintf(stdout,"ERROR: Position must be within or at end of the column (1 to %d).\n",cnt+1);
				fflush(stdout);
				exit(1);
			}
			break;
		case('d'):
			get_funny_params(2,argc,argv);
			ifactor = (int)round(number[cnt]);
			factor = round(number[cnt+1]);
			if((ifactor < 1) || (factor < 1.0)) {
				fprintf(stdout,"ERROR: Positions must be 1 or greater.\n");
				fflush(stdout);
				exit(1);
			}
			if((ifactor > cnt) && (factor > (double)cnt)) {
				fprintf(stdout,"ERROR: Positions must be inside the list you are editing.\n");
				fflush(stdout);
				exit(1);
			}
			if(((ifactor >= cnt) && (factor <= 1.0)) || ((ifactor == 1) && (factor >= (double)cnt))) {
				fprintf(stdout,"ERROR: Entire column will be deleted.\n");
				fflush(stdout);
				exit(1);
			}
			break;
		case('r'):
			get_funny_params(2,argc,argv);
			ifactor = (int)round(number[cnt]);
			factor = number[cnt+1];
			if(ifactor < 1 || ifactor > cnt) {
				fprintf(stdout,"ERROR: Position must be within the column (1 to %d).\n",cnt);
				fflush(stdout);
				exit(1);
			}
			break;
		}
		break;
	case('z'):
		switch(ro) {
		case('X'):	case('Y'):	case('Z'):
			get_extra_params(3,argc,argv);
			cnt = round(number[0]);
			number[0] = number[1];
			number[1] = number[2];
			break;
		case('e'):
			get_extra_params(5,argc,argv);
			ifactor = round(number[0]);
			number[0] = number[1];
			number[1] = number[2];
			number[2] = number[3];
			number[3] = number[4];
			break;
		case('A'):	case('B'):	case('C'):	case('D'):	case('E'):
		case('F'):	case('G'):	case('H'):	case('I'):	case('J'):
			get_extra_params(4,argc,argv);
			cnt = round(number[0]);
			number[0] = number[1];
			number[1] = number[2];
			number[2] = number[3];
			if(cnt <= 0) {
				fprintf(stdout,"ERROR: Number of steps cannot be less than 1.\n");
				fflush(stdout);
				exit(1);
			}
			break;
		case('q'):
			get_extra_params(4,argc,argv);
			cnt = round(number[0]);
			number[0] = number[1];
			number[1] = number[2];
			number[2] = number[3];
			if(cnt<=1) {
				fprintf(stdout,"ERROR: Can't randomise less than 2 elements.\n");
				fflush(stdout);
				exit(1);
			}
			quantisation = number[0];
			dur  		 = number[1]; 	
			scat 		 = number[2];
			tq = (int)floor(dur/quantisation) + 1; 			/* total quantised positions */
			ql = (int)ceil(tq/(cnt-1));				 		/* average number of quanta per placed item */
			if(ql <= 1) {
				fprintf(stdout,"ERROR: Quantisation is too coarse to accomodate all these elements.\n");
				fflush(stdout);
				exit(1);
			}
			if(scat <= 1.0)  {								/* for scatter less than 1 */
				ql = (int)round(ql * scat);
				if(ql <= 1) {
					fprintf(stdout,"ERROR: Scatter is too small to achieve randomisation of position.\n");
					fflush(stdout);
					exit(1);
				}
			}
			if((cnt+1 > 4) && (number = (double *)realloc((char *)number,(cnt+1) * sizeof(double)))==NULL) {
				fprintf(stdout,"ERROR: Out of memory.\n");
				fflush(stdout);
				exit(1);
			}
			break;
		}
		break;
	case('Z'):
		switch(ro) {
		case('H'):	case('m'):
			get_extra_params(3,argc,argv);
			break;
		}
		break;
	default:
		fprintf(stdout,"ERROR: Unknown process mode.\n");
		fflush(stdout);
		exit(1);
	}
}

/************************* DO_STRING_PARAMS ***************************
 *
 * Reads parameters which are strings
 *
 * NB stringscnt will then include count of these parameters,
 * hence count of any strings values read from file gets remembered as 'cnt'.
 */

void do_string_params(char *argv[],int pcnt)
{
	int  strspace, n, k, nn;
	int space_step = 200, old_stringstoresize, startstringscnt = stringscnt;
	char temp2[200], *p;
	int total_space = space_step;
	cnt = stringscnt;		/* remember count of any existing strings */
	if(stringstoresize == 0) {
		if((stringstore = (char *)exmalloc(total_space))==NULL) {
			sprintf(errstr,"Out of Memory\n");
			do_error();
		}
	}
	for(n=2,k=0;k<pcnt;k++,n++) {
		strcpy(temp2,argv[n]);
		strspace = strlen(temp2)+1;
		old_stringstoresize = stringstoresize;
		if((stringstoresize += strspace) >= total_space) {
			while(stringstoresize  >= total_space)
				total_space += space_step;
			if((stringstore = (char *)exrealloc((char *)stringstore,total_space))==NULL) {
				sprintf(errstr,"Out of Memory\n");
				do_error();
			}
		}
		strcpy(stringstore + stringstart,temp2);
		stringstart += strspace;
		stringscnt++;
    }
	if(stringscnt <= cnt) {
		fprintf(stdout,"ERROR: Invalid or missing data.\n");
		fflush(stdout);
		exit(1);
	}
	if(strings == 0) {
		if((strings = (char **)malloc(stringscnt * sizeof(char *)))==NULL) {
			sprintf(errstr,"Out of Memory\n");
			do_error();
		}
	} else {
		if((strings = (char **)realloc((char *)strings,stringscnt * sizeof(char *)))==NULL) {
			sprintf(errstr,"Out of Memory\n");
			do_error();
		}
	}
	p = stringstore;
	nn = 0;
	while(nn < startstringscnt) {
		while(*p != ENDOFSTR)
			p++;
		p++;
		nn++;
	}
	while(nn < stringscnt) {
		strings[nn] = p;
		while(*p != ENDOFSTR)
			p++;
		p++;
		nn++;
	}
	fclose(fp[0]);
}

/********************************* READ_DATA *********************************/

void read_data(char *startarg,char *endarg,int *ro)
{
									/* READ THE (FIRST) FILE DATA */
	if(!strncmp(endarg,"-th",3) || !strncmp(endarg,"-tM",3))
		do_pitchtext_infile(startarg);				/* PITCH-REPRESENTING TEXTS in file */
	else if(!strncmp(endarg,"-As",3) || !strncmp(endarg,"-dl",3) || !strncmp(endarg,"-rr",3)
	 || !strncmp(endarg,"-rR",3) || !strncmp(endarg,"-rX",3)
	 || !strncmp(endarg,"-dv",3) || !strncmp(endarg,"-G",2)	 || !strncmp(endarg,"-sK",3)
	 || !strncmp(endarg,"-sk",3) || !strncmp(endarg,"-Ro",3) || !strncmp(endarg,"-Ff",3)
	 || !strncmp(endarg,"-FR",3) || !strncmp(endarg,"-Tb",3) || !strncmp(endarg,"-TB",3)
	 || !strncmp(endarg,"-nM",3) || !strcmp(endarg,"-kT")	 || !strcmp(endarg,"-kX")
	 || !strcmp(endarg,"-kA")    || !strcmp(endarg,"-kB")    || !strcmp(endarg,"-kC")
	 || !strcmp(endarg,"-kY")    || !strncmp(endarg,"-aT",3) || !strncmp(endarg,"-aX",3)
	 || !strncmp(endarg,"-nT",3) || !strncmp(endarg,"-sU",3) || !strncmp(endarg,"-Rx",3)
	 || !strncmp(endarg,"-eb",3) || !strncmp(endarg,"-nF",3)) {
		do_string_infile(startarg);					/* STRINGS in file */
	} else if(!strncmp(endarg,"-J",2) || !strncmp(endarg,"-j",2) || !strncmp(endarg,"-jj",3) 
		|| !strncmp(endarg,"-Jj",3) || !strncmp(endarg,"-Wb",3) ) {
		colcnt = do_stringline_infile(startarg,0);	/* LINES OF STRINGS in file */
		if(!strncmp(endarg,"-Wb",3) && (colcnt != 3)) {
			fprintf(stdout,"ERROR: THis process only works with sequence files\n");
			fflush(stdout);
			exit(1);
		}
		cnt = stringscnt;
	} else if(!strncmp(endarg,"-DB",3))
		do_DB_infile(startarg);						/* DB info in file */
	else if(!strcmp(endarg,"-c"))
		*ro = ENDOFSTR;								/* COUNT only, option */
	else if(!strcmp(endarg,"-cl"))
		*ro = 'l';									/* COUNT lines, option */
	else
		do_infile(startarg);						/* read numeric values */
}

/********************************* DO_HELP *********************************/

void do_help(char *str)
{
	if(!strcmp(str,"-f"))    help();
	if(!strcmp(str,"-m"))    helpm();
    if(!strcmp(str,"-M"))    helpM();
    if(!strcmp(str,"-R"))    helpr();
    if(!strcmp(str,"-l"))    helpl();
    if(!strcmp(str,"-g"))    helpg();
}

/********************************* OPTION_TAKES_NO_DATA_BUT_HAS_EXTRA_PARAMS *********************************/

int option_takes_NO_data_but_has_extra_params(char *agv)
{
	if(!strcmp(agv,"-Rg") || !strcmp(agv,"-Rv") || !strcmp(agv,"-Rc") || !strcmp(agv,"-Ri") || !strcmp(agv,"-RI")
	|| !strcmp(agv,"-At") || !strcmp(agv,"-AT") || !strcmp(agv,"-Ad") || !strcmp(agv,"-ic") || !strcmp(agv,"-i=") 
	|| !strcmp(agv,"-L")  || !strcmp(agv,"-Q")  || !strcmp(agv,"-iC") || !strcmp(agv,"-ie") || !strcmp(agv,"-rz") || !strcmp(agv,"-qz") 
 	|| !strcmp(agv,"-Rr") || !strcmp(agv,"-iR") || !strcmp(agv,"-iQ") || !strcmp(agv,"-cs") || !strcmp(agv,"-ss")
	|| !strcmp(agv,"-sc") || !strcmp(agv,"-sv") || !strcmp(agv,"-sx") || !strncmp(agv,"-Zm",3) || !strncmp(agv,"-ZH",3)
	|| !strncmp(agv,"-z",2) || !strcmp(agv,"-RG") || !strcmp(agv,"-rG") || !strcmp(agv,"-rJ") || !strcmp(agv,"-pp"))	
		return 1;			/* all flags starting with 'z', don't read data, but have extra-params */
	return 0;
}

/********************************* OPTION_TAKES_DATA_AND_HAS_EXTRA_PARAMS *********************************/

int option_takes_data_and_has_extra_params(char *agv)
{
	if(!strncmp(agv,"-y",2) 	/* all flags starting with 'y', read data, and have extra-params */
	|| !strncmp(agv,"-Tb",3) || !strncmp(agv,"-TB",3) || !strncmp(agv,"-Ca",3) || !strncmp(agv,"-Cm",3)
	|| !strcmp(agv,"-sa")    || !strcmp(agv,"-sb")	  || !strcmp(agv,"-sf")	   || !strncmp(agv,"-bb",3)
	|| !strcmp(agv,"-dd")    || !strcmp(agv,"-dL")    || !strcmp(agv,"-SM") || !strcmp(agv,"-ga")
	|| !strncmp(agv,"-K",2))	/* all flags starting with 'K', read data, and have extra-params */
		return 1;
	return 0;
}

/********************************* CHECK_EXCEPTIONAL_PROCESSES ********************************/

void check_exceptional_processes(int ro,int argc)
{
	if(!sloom && !sloombatch) {
		if(flag=='H' && (ro==ENDOFSTR || ro=='r') && cnt>1) {
			fprintf(stderr,"Too many values in file for H");
			if(ro=='r')
				fprintf(stderr,"r");
			fprintf(stderr," flag.\n");
			exit(1);
		}

		if((flag=='J' || (flag=='C' && (ro == ENDOFSTR || ro == 'c'))) && argc<4) {
			fprintf(stderr,"Flags J & C need at least 2 input files.\n");
			exit(1);
	    }
	    if(((flag=='S' && ro==ENDOFSTR) || flag=='N') && argc!=4) {
			fprintf(stderr,"flags S & N or Nr need 1 infilename & 1 generic outfilename.\n");
			exit(1);
	    }
	} else {
		if(flag=='H' && (ro==ENDOFSTR || ro=='r') && cnt>1) {
			fprintf(stdout,"ERROR: Too many values in file this option.\n");
			fflush(stdout);
			exit(1);
		}
	    if((flag=='J' || flag=='j'  
		|| (flag == 'W' && ro == 'b')
	    || (flag=='e' && (ro == 's' || ro == 'i' || ro == 'A' || ro == 'S' || ro == 'w' || ro == 'W' || ro == 'L' || ro == 'Q' || ro == 'X' || ro == 'Y'))
	    || (flag == 'A' && ro == 'e')) && argc<4) {
			fprintf(stdout,"ERROR: This option needs at least 2 input files.\n");
			fflush(stdout);
			exit(1);
	    }
	    if(((flag=='S' && ro==ENDOFSTR) || flag=='N') && argc!=4) {
			fprintf(stdout,"ERROR: These options need 1 infilename & 1 generic outfilename.\n");
			fflush(stdout);
			exit(1);
	    }
	}
}

/********************************* HANDLE_MULTIFILE_PROCESSES_AND_THE_OUTPUT_FILE ********************************/

void handle_multifile_processes_and_the_output_file(int argc, char flag,int ro,char **argv)
{
	int joico = 0, colcnt2 = 0;

	if(!sloom) {
		if(flag=='J') {
			joico = 1;
		    firstcnt = cnt;
		    infilecnt = argc-2;		
	   	    do_other_stringline_infiles(argv,'J');
		} else if(flag=='C' && (ro == ENDOFSTR || ro == 'c')) {
			joico = 1;
		    firstcnt = cnt;
		    infilecnt = argc-2;		
			do_other_infiles(argv);
		} else if(flag =='j') {
			if(ro == 0) {
			    infilecnt = 2;	
				colcnt2 = do_other_stringline_infile(argv[2]);
				if(colcnt != colcnt2) {
					fprintf(stdout,"ERROR: Incompatible column counts (%d and %d) in these two files.\n",colcnt,colcnt2);
					fflush(stdout);
					exit(1);
				}
			} else if(ro =='j') {
				infilecnt = argc-2;		
		   	    do_other_stringline_infiles(argv,'j');
			}
		} else if(flag =='W' && ro == 'b') {
			infilecnt = argc-2;		
	   	    do_other_stringline_infiles(argv,'W');
		} else if(flag =='K' && ro == 'b') {
			infilecnt = argc-3;		
	   	    do_other_infiles(argv);
		} else {
		    if(argc>4 && !joico) {
				fprintf(stdout,"Too many arguments.\n");
				fflush(stdout);
				exit(1);
		    }
	    	if(flag!='S' && flag!='N') {
	/*RWD*/		if(!strcmp(argv[2],"con") || !strcmp(argv[2],"CON"))
		    		fp[1]=stdout;
				else if(allcols == 1)
	 	           	do_outfile(argv[2]);
				else
					strcpy(goalfile,argv[2]);
		   	}
		}
	} else {
		if(flag=='A' && ro == 'e') {
			joico = 1;
		    infilecnt = argc-2;		
			if((file_cnt = (int *)malloc(infilecnt * sizeof(int)))==NULL) {
				fprintf(stdout,"ERROR: Out of memory.\n");
				fflush(stdout);
				exit(1);
			}
	 		file_cnt[0] = cnt;
	   	    do_other_infiles(argv);
		} else if(flag=='J') {
			joico = 1;
		    firstcnt = cnt;
		    infilecnt = argc-2;		
	   	    do_other_stringline_infiles(argv,'J');
		} else if(flag=='C' && (ro == ENDOFSTR || ro == 'c')) {
			joico = 1;
		    firstcnt = cnt;
		    infilecnt = argc-2;		
	   	    do_other_infiles(argv);
		} else if(flag =='e' 
			&& (ro == 's' || ro == 'i' || ro == 'a' || ro == 'A' || ro == 'S' || ro == 'w' || ro == 'W' || ro == 'L' || ro == 'Q' 
			|| ro == 'X' || ro == 'm' || ro =='Y')) {
		    firstcnt = cnt;
		    infilecnt = 2;	
	   	    do_other_infiles(argv);
		} else if(flag =='w' && (ro == 't' || ro == 'o')) {
		    firstcnt = cnt;
		    infilecnt = 2;	
	   	    do_other_infiles(argv);
		} else if(flag =='j') {
			if(ro == 0) {
			    infilecnt = 2;	
		   	    colcnt2 = do_other_stringline_infile(argv[2]);
				if(colcnt != colcnt2) {
					fprintf(stdout,"ERROR: Incompatible column counts (%d and %d) in these two files.\n",colcnt,colcnt2);
					fflush(stdout);
					exit(1);
				}
			} else if(ro =='j') {
			    infilecnt = argc-2;		
				do_other_stringline_infiles(argv,'j');
			}
		} else if(flag =='W' && ro == 'b') {
			infilecnt = argc-2;		
	   	    do_other_stringline_infiles(argv,'W');
		} else if(flag =='K' && ro == 'b') {
			infilecnt = argc-3;		
	   	    do_other_infiles(argv);
		} else if(!joico) {
		    if(argc>4) {
				fprintf(stdout,"ERROR: Too many arguments.\n");
				fflush(stdout);
				exit(1);
		    }
			if(flag!='S' && flag!='N') {
				if(allcols == 1)
					do_outfile(argv[2]);
				else
					strcpy(goalfile,argv[2]);
			}
		}
	}
}

/********************************* ERROR_REPORTING ********************************/


void cant_read_numeric_value(char flag,int ro) {
	if(!sloom && !sloombatch)
		fprintf(stderr,"Cannot read numerical value with flag %c%c\n",flag,ro);
	else {
		fprintf(stdout,"ERROR: Cannot read numerical value.\n");
		fflush(stdout);
	}
	exit(1);
}

void cant_read_numeric_value_with_flag_only(char flag) {
	if(!sloom && !sloombatch)
		fprintf(stderr,"Cannot read numerical value with flag %c\n",flag);
	else {
		fprintf(stdout,"ERROR: Cannot read numerical value.\n");
		fflush(stdout);
	}
	exit(1);
}

void unknown_flag_only(char flag)
{
	if(!sloom && !sloombatch)
		fprintf(stderr,"Unknown flag -%c\n",flag);
	else {
		fprintf(stdout,"ERROR: Unknown option.\n");
		fflush(stdout);
	}
	exit(1);
}

void unknown_flag(char flag,int ro)
{
	if(!sloom && !sloombatch)
		fprintf(stderr,"Unknown flag -%c%c\n",flag,ro);
	else {
		fprintf(stdout,"ERROR: Unknown option.\n");
		fflush(stdout);
	}
	exit(1);
}

void cannot_read_flags(char *flagstr)
{
	if(!sloom && !sloombatch)
		fprintf(stderr,"Cannot read flag %s.\n",flagstr);
	else {
		fprintf(stdout,"ERROR: Cannot read the option type\n");
		fflush(stdout);
	}
	exit(1);
}

void flag_takes_no_params(char flag,int ro)
{
	if(!sloom && !sloombatch)
		fprintf(stderr,"flag -%c%c does not take a parameter\n",flag,ro);
	else {
		fprintf(stdout,"ERROR: This option does not take a parameter\n");
		fflush(stdout);
	}
	exit(1);
}

void flag_only_takes_no_params(char flag)
{
	if(!sloom && !sloombatch)
		fprintf(stderr,"flag -%c does not take a parameter\n",flag);
	else {
		fprintf(stdout,"ERROR: This option does not take a parameter\n");
		fflush(stdout);
	}
	exit(1);
}

void unknown_flag_or_bad_param(void)
{
	if(!sloom && !sloombatch)
		fprintf(stderr,"Unknown flag or inappropriate use of parameter.\n");
	else {
		fprintf(stdout,"ERROR: Unknown flag or inappropriate use of parameter.\n");
		fflush(stdout);
	}
	exit(1);
}

void no_value_with_flag(char flag,int ro)
{
	if(!sloom && !sloombatch)
	   	fprintf(stderr,"No value supplied with flag -%c%c\n",flag,ro);
	else {
	   	fprintf(stdout,"ERROR: No value supplied with this option\n");
		fflush(stdout);
	}
   	exit(1);
}

void no_value_with_flag_only(char flag)
{
	if(!sloom && !sloombatch)
	   	fprintf(stderr,"No value supplied with flag -%c\n",flag);
	else {
	   	fprintf(stdout,"ERROR: No value supplied with this option\n");
		fflush(stdout);
	}
   	exit(1);
}

void no_value_required_with_flag(char flag,int ro)
{
	if(!sloom && !sloombatch)
		fprintf(stderr,"No parameter required with flag %c%c.\n",flag,ro);
	else {
		fprintf(stdout,"ERROR: No parameter required with this option.\n");
		fflush(stdout);
	}
	exit(1);
}

void no_value_required_with_flag_only(char flag)
{
	if(!sloom && !sloombatch)
		fprintf(stderr,"No parameter required with flag %c\n",flag);
	else {
		fprintf(stdout,"ERROR: No parameter required with this option.\n");
		fflush(stdout);
	}
	exit(1);
}

void read_flags_data_error(char *str)
{
	if(!sloom && !sloombatch)
		fprintf(stderr,"%s\n",str);
	else {
		fprintf(stdout,"ERROR: %s\n",str);
		fflush(stdout);
	}
	exit(1);
}

void unknown_flag_string(char *flagstr)
{
	if(!sloom && !sloombatch)
		fprintf(stderr,"Unknown flag %s.\n",flagstr);
	else {
		fprintf(stdout,"ERROR: Unknown option\n");
		fflush(stdout);
	}
	exit(1);
}

void do_error(void) {
	if(!sloom && !sloombatch)
		fprintf(stderr,"%s\n",errstr);
	else {
		fprintf(stdout,"ERROR: %s\n",errstr);
		fflush(stdout);
	}
	exit(1);
}

void do_valout(double val) {
	if(!sloom && !sloombatch) {
		if(allcols > 1)
			outcoldata[outcoldatacnt++] = val;
		else
	        fprintf(fp[1],"%lf\n",val);
	} else
		fprintf(stdout,"INFO: %lf\n",val);
}

void do_valout_as_message(double val) {
	if(!sloom && !sloombatch)
        fprintf(fp[1],"%lf\n",val);
	else
		fprintf(stdout,"WARNING: %lf\n",val);
}

void do_valout_flush(double val) {
	if(!sloom && !sloombatch) {
		if(allcols > 1)
			outcoldata[outcoldatacnt++] = val;
		else
			fprintf(fp[1],"%lf\n",val);
	} else {
		fprintf(stdout,"INFO: %lf\n",val);
		fflush(stdout);
	}
}

void do_stringout(char *str) {
	if(!sloom && !sloombatch) {
		if(allcols > 1)
			outcoldata[outcoldatacnt++] = atof(str);
		else
			fprintf(fp[1],"%s\n",str);
	}
	else
		fprintf(stdout,"INFO: %s\n",str);
}

void do_valpair_out(double val0,double val1) {
	if(!sloom && !sloombatch)
		fprintf(fp[1],"%lf  %lf\n",val0,val1);
	else
		fprintf(stdout,"INFO: %lf  %lf\n",val0,val1);
}

/************************* CHECK_FOR_COLUMNEXTRACT ************************/

void check_for_columnextract(char *argv1,int *argc,char *argv[])
{
	int this_column, linecnt;
	double dummy;
	char temp2[200], *p = argv[*argc-1];
	if(*p != '-')
		return;
	p++;
	if(*p != '-')
		return;
	p++;
	if(*p != 'c')
		return;
	p++;
	if(sscanf(p,"%d",&thecol)!=1)
		return;
	thecol--;
	strcpy(srcfile,argv1);
	(*argc)--;
	if((fp[0] = fopen(argv1,"r"))==NULL) {
		sprintf(errstr,"Cannot open infile %s\n",argv1);
		do_error();
	}
	if((fp[2] = fopen("__temp.txt","w"))==NULL) {
		sprintf(errstr,"Cannot open temporary file (__temp.txt) to store extracted column\n");
		do_error();
	}
	this_column = 0;
	linecnt = 0;
	while(fgets(temp,200,fp[0])!=NULL) {
		this_column = 0;
		p = temp;
		while(strgetstr(&p,temp2)) {
			if(this_column == thecol) {
				if(sscanf(temp2,"%lf",&dummy) != 1) {
					sprintf(errstr,"Numeric values only for multicolumn data.\n");
					do_error();
				}
		        fprintf(fp[2],"%s\n",temp2);
			}
			this_column++;
		}
		if(linecnt == 0) {
			if(this_column <= thecol) {
				sprintf(errstr,"There is no column %d in the input data.\n",thecol+1);
				do_error();
			}
			allcols = this_column;
		} else if(allcols != this_column) {
			sprintf(errstr,"Inconsistent number of columns in data.\n");
			do_error();
		}
		linecnt++;
	}
	fclose(fp[0]);
	fclose(fp[2]);
	if((outcoldata = (double *)malloc(linecnt * sizeof(double))) == NULL) {
		sprintf(errstr,"Failed to allocate memory to store column data.\n");
		do_error();
	}
}

/********************************** VALID_CALL_FOR_MULTICOL_INPUT **********************************/

int valid_call_for_multicol_input(char flag,int ro)
{
	switch(flag) {
	case('A'):
		if(ro == 's')
			return 1;
		if(noro(ro))
			return 1;
		break;
	case('D'):
		if(ro == 'B')
			return 1;
		break;
	case('I'):
		if(ro == 'r')
			return 1;
		break;
	case('M'):
		if(ro == 'h')
			return 1;
		if(ro == 'm')
			return 1;
		if(ro == 't')
			return 1;
		break;
	case('P'):
		if(noro(ro))
			return 1;
		break;
	case('R'):
		if(noro(ro))
			return 1;
		if(ro == 'A' || ro == 'a' || ro == 'm' || ro == 'o' || ro == 's')
			return 1;
		break;
	case('S'):
		if(ro == 'L')
			return 1;
		break;
	case('T'):
		if(ro == 'c' || ro == 'h' || ro == 'M' || ro == 'l')
			return 1;
		break;
	case('a'):
		if(noro(ro))
			return 1;
		break;
	case('b'):
		if(ro == 'g' || ro == 'l')
			return 1;
		break;
	case('d'):
		if(noro(ro))
			return 1;
		if(ro == 'b')
			return 1;
		break;
	case('f'):
		if(ro == 'l')
			return 1;
		break;
	case('h'):
		if(ro == 'M')
			return 1;
		break;
	case('i'):
		if(ro == 'L' || ro == 'M' || ro == 'a' || ro == 'h' || ro == 'm' || ro == 'l')
			return 1;
		break;
	case('l'):
		if(ro == 'i')
			return 1;
		break;
	case('m'):
		if(noro(ro))
			return 1;
		if(ro == 'M' || ro == 'g' || ro == 'l')
			return 1;
		break;
	case('o'):
	case('q'):
		if(noro(ro))
			return 1;
		break;
	case('r'):
		if(ro == 'S' || ro == 'm' || ro == 'r')
			return 1;
		break;
	case('s'):
		if(ro == 'l' || ro == 't')
			return 1;
		break;
	case('t'):
		if(ro == 'M' || ro == 'h')
			return 1;
		break;
	}
	return 0;
}

int noro(int ro)
{
	if(ro == ENDOFSTR || ro == '.' || ro == '-' || isdigit(ro))
		return 1;
	return 0;
}

void output_multicol_data()
{
	int xcnt;
	int this_column;
	char *p, temp2[200];
	if(strlen(goalfile) > 0) {
		if((fp[0] = fopen(srcfile,"r"))==NULL) {
			sprintf(errstr,"Cannot reopen source file %s\n",srcfile);
			do_error();
		}
		do_outfile(goalfile);
		xcnt = 0;
		while(fgets(temp,200,fp[0])!=NULL) {
			this_column = 0;
			p = temp;
			while(strgetstr(&p,temp2)) {
				if(this_column == thecol)
					fprintf(fp[1],"%lf",outcoldata[xcnt++]);
				else
					fprintf(fp[1],"%s",temp2);
				fprintf(fp[1],"\t");
				this_column++;
			}
			fprintf(fp[1],"\n");
		}
		fclose(fp[0]);
		fclose(fp[1]);
	} else {
		for(xcnt = 0;xcnt < outcoldatacnt;xcnt++)
			fprintf(stdout,"%lf\n",outcoldata[xcnt]);
		fflush(stdout);
	}
}
