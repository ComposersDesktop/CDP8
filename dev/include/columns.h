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



/* header for columns */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sfsys.h>
#include <osbind.h>

#if defined unix || defined __GNUC__
#define round(x) lround((x))
#endif

#ifndef HUGE
#define HUGE 3.40282347e+38F
#endif

#define PROG    "COLUMNS"

#define BIGARRAY  200
#define ODD(x)    ((x)&1)
#define ENDOFSTR  '\0'
#define FLTERR    0.000002
#define ONEMAX    0.001         /* Upper limit on log(x)==1 */
#define ONEMIN    0.999         /* Lower limit on log(x)==1 */
#define TWELVE  12.0
#define VERY_TINY (0.0000000000000000000000000000001)

#define C_HZ              (8.175799)
#define LOG_2_TO_BASE_E  (.69314718)
#define HARMONIC_TUNING_TOLERANCE (.01) /* semitones */
#define ONE_OVER_LN2  (1.442695)
#define INVERT 1
#define MULT 0
#define ADD 1
#define SUBTRACT 2
#define ENVMAX   3
#define MIDIMINFRQ (8.175799)
#define MIDIMAXFRQ (12543.853951)

#define MIDIMIN (0)
#define MIDIMAX (127)

#define MAXSAMP (32767.0)
#define F_MAXSAMP (1.0)
#define MAXOCTTRANS (12.0)

extern int      cnt, firstcnt, arraysize, infilecnt, outfilecnt;
extern double   *number, *permm, *permmm, factor, scatter, *pos;
extern double   thresh;
extern FILE     **fp;
extern char     temp[20000], flag, *filename, *thisfilename, string[200];
extern int      ro, ifactor, len, condit;

extern char **strings;
extern char     *stringstore;
extern int      stringscnt;
extern int      stringstoresize, stringstart;

int     strgetfloat(char **,double  *), strgetstr(char **,char *);
int     flteq(double,double);
void    logo(void), qiksort(void), eliminate(int), usage(void);
double  hztomidi(double), miditohz(double);
char    *exmalloc(int), *exrealloc(char *,int);
void    rndperm(double *), insert(double *,int, int), prefix(double *,int);
void    multrndperm(double *), rndperm2(double *), shuffle(int,int);
void    do_infile(char *), do_pitchtext_infile(char *), do_text_read(char *);
void    do_outfile(char *), do_other_infiles(char *[]);
void    bellperm1(void), bellperm2(void), bellperm3(void);
void    swap(double *,double *);
double  texttomidi(char *);
char    *get_pitchclass(char *,int *,int);
int     timevents(double,double,double,double);
int     m_repos(int), f_repos(int);
void    check_for_conditional(int *,char *[]);
void do_stringline_infile_irreg(char *argv,int n);
void    remove_frq_pitchclass_duplicates(void);
void    count_items(char *);
void    accel_time_seq(void);
void    accel_durations(void);
void    separate_columns_to_files(void);
void    partition_values_to_files(void);
void    join_files_as_columns(char *);
void    join_many_files_as_columns(char *,int);
void    concatenate_files(char *);
void    concatenate_files_cyclically(char *);
void    vals_end_to_end_in_2_cols(void);
void    quantise(void);
void    eliminate_equivalents(void);
void    eliminate_even_items(void);
void    eliminate_greater_than(void);
void    eliminate_less_than(void);
void    eliminate_duplicates(void);
void    print_numbers(void);
void    reduce_to_bound(void);
void    increase_to_bound(void);
void    greatest(void);
void    least(void);
void    mark_greater_than(void);
void    mark_less_than(void);
void    minor_to_major(void);
void    add(void);
void    take_power(void);
void    temper_midi_data(void);
void    temper_hz_data(void);
void    time_from_crotchet_count(void);
void    time_from_beat_lengths(void);
void    total(void);
void    text_to_hz(void);
void    product(void);
void    hz_to_midi(void);
void    find_mean(void);
void    midi_to_hz(void);
void    major_to_minor(void);
void    remove_midi_pitchclass_duplicates(void);
void    reverse_list(void);
void    rotate_motif(void);
void    ratios(void);
void    qiksort(void);
void    reciprocals(int positive_vals_only);
void    randomise_order(int *perm);
void    add_randval_plus_or_minus(void);
void    add_randval(void);
void    multiply_by_randval(void);
void    randchunks(void);
void    generate_random_values(void);
void    random_0s_and_1s(void);
void    random_0s_and_1s_restrained(void);         /*TW march 2004 */
void    random_scatter(void);
void    random_elimination(void);
void    equal_divisions(void);
void    log_equal_divisions(void);
void    quadratic_curve_steps(void);
void    plain_bob(void);
void    get_intervals(void);
void    repeat_intervals(void);
void    make_equal_intevals_btwn_given_vals(void);
void    create_intervals(void);
void    change_value_of_intervals(void);
void    change_value_of_intervals(void);
void    get_intermediate_values(void);
void    insert_intermediate_values(void);
void    motivically_invert_midi(void);
void    motivically_invert_hz(void);
void    stack(int with_last);              /*TW March 2004 */
void    get_one_skip_n(void);
void    get_n_skip_one(void);
void    sum_nwise(void);
void    sum_minus_overlaps(void);
void    sum_absolute_differences(void);
void    duplicate_values(void);
void    duplicate_list(void);
void    multiply(int rounded);
void    format_vals(void);
void    rotate_partition_values_to_files(void);
void    generate_harmonics(void);
void    group_harmonics(void);
void    get_harmonic_roots(void);
void    rank_vals(void);
void    rank_frqs(void);
void    approx_vals(void);
void    pitchtotext(int midi);
void    miditotext(void);
void    floor_vals(void);
void    limit_vals(void);

void    help(void),helpm(void),helpM(void),helpr(void);
void    helpl(void),helpg(void);
void    interval_limit(void);
void    time_density(void), divide_list(void), group(void);
void    duplicate_octfrq(void);
void    duplicate_octaves(void);
void    interval_to_ratio(int semitones,int tstretch);
void    do_slope(void);
void    do_string_infile(char *);
void    alphabetic_sort(void);
void    db_to_level(int sampleval);
void    level_to_db(int sampleval);
void    do_DB_infile(char *);
void    columnate(void);
void    samp_to_time(void);
void    time_to_samp(void);
void    column_format_vals(void);
void    delete_small_intervals(void);
void    mark_event_groups(void);
void    span(void);
void    spanpair(void);
void    alt0101(void);
void    alt0011(void);
void    alt01100(void);
void    delete_vals(void);
void    insert_val(void);
void    replace_val(void);
void    ascending_order(void);
void    alt0r0r(void);
void    altr0r0(void);
void    alt00rr(void);
void    altrr00(void);
void    alt0rr00r(void);
void    altr00rr0(void);
void    alt00RR(void);
void    altRR00(void);
void    alt0RR00R(void);
void    altR00RR0(void);
void    create_intervals_from_base(void);
void    create_ratios_from_base(void);
void    create_equal_vals(void);
void    quantised_scatter(int *perm,int permlen);
void    replace_equal(void);
void    replace_less(void);
void    replace_greater(void);
void    grid(int isoutside);
void    randdel_not_adjacent(void);
void    replace_with_rand(int i);
void    randquanta_in_gaps(void);
void    reverse_time_intervals(void);
void    invert_normd_env(void);
void    invert_over_range(void);
void    invert_around_pivot(void);
void    join_files_as_rows(void);
void    join_many_files_as_rows(void);
void    env_superimpos(int inverse,int typ);
void    env_del_inv(void);
void    env_plateau(void);
void    time_from_beat_position(int has_offset);
void    insert_after_val(void);
void    min_interval(int ismax);
void    mark_multiples(void);
void    insert_in_order(void);
void    insert_at_start(void);
void    insert_at_end(void);
void    format_strs(void);
void    column_format_strs(void);
void    random_integers(void);
void    random_integers_evenly_spread(void);
void    time_from_bar_beat_metre_tempo(int has_offset);
void    brk_add(void);
void    brk_subtract(void);
void    span_rise(void);
void    span_fall(void);
void    span_all(void);
void    span_xall(void);
void    cyclic_select(char c);
void    generate_randomised_vals(void);
void    ratio_to_interval(int semitones,int tstretch);
void    pitch_to_delay(int midi);
void    delay_to_pitch(int midi);
void    reverse_order_of_brkvals(void);
void    thresh_cut(void);
void    band_cut(void);

int     do_stringline_infile(char *argv,int i);
int     do_other_stringline_infile(char *argv);
void    do_other_stringline_infiles(char *argv[],char c);
void    do_inline_cnt(int n,char *argv);
void    scale_above(void);
void    scale_below(void);
void    scale_from(void);
void    env_append(void);
void    abutt(void);
void    quantise_time(void);
void    quantise_val(void);
void    expand_table_dur_by_factor(void);
void    expand_table_dur_to_time(void);
void    expand_table_vals_by_factor(void);
void    cut_table_at_time(void);
void    limit_table_val_range(void);
void    expand_table_to_dur(void);
void    extend_table_to_dur(void);

void    do_error(void);
void    do_valout(double val);
void    do_stringout(char *str);
void    do_valout_as_message(double val);
void    do_valout_flush(double val);
void    do_valpair_out(double val0,double val1);
void    note_to_midi(int is_transpos);
void    substitute(void);
void    substitute_all(void);   /*TW March 2004 */
void    mean_of_reversed_pairs(void);
void    duplicate_values_stepped(void);
void    kill_text(int after);
void    append_text(int where);

void    targeted_pan(void);
void    targeted_stretch(void);
void    do_squeezed_pan(void);
void    kill_path(void);
void    kill_extension(void);
void    kill_path_and_extension(void);
void    convert_space_pan_to_tex(void);
void    convert_space_tex_to_pan(void);

extern char     errstr[];
extern int colcnt, *cntr;
extern int *file_cnt;
extern int sloom;
extern int sloombatch;
extern int hasoutfile;
extern char outfilename[];

void compress_sequence(int multi);              /* Wc */
void transpose_sequence(int multi);             /* Wt */
void p_invertset_sequence(int multi);   /* WI */
void p_expandset_sequence(int multi);   /* WE */
void p_invert_sequence(int multi);              /* Wi */
void t_reverse_sequence(int multi);             /* Wr */
void p_reverse_sequence(int multi);             /* WP */
void a_reverse_sequence(int multi);             /* WA */
void pa_reverse_sequence(int multi);    /* WR */
void tp_reverse_sequence(int multi);    /* WT */
void ta_reverse_sequence(int multi);    /* Wa */
void tpa_reverse_sequence(int multi);   /* WZ */
void loop_sequence(int);                /* Wl */
void abut_sequences(int);               /* Wb */
void uptempo_sequence(int);     /* Wm */
void accel_sequence(int);               /* WM */
void create_equal_steps(void);  /* iQ */
void mean_tempo(void);                  /* te */
void time_to_crotchets(int beatvals);   /* tB,tC */
void rotate_list(int reversed); /* rR, rX */
void splice_pos(void);                  /* SM */
void time_warp(void);                   /* ew */
void brkval_warp(void);                 /* eX */
void brktime_warp(void);                /* eW */
void duplicate_list_at_step(void); /* dL */
void list_warp(void);                   /* eL */
void seqtime_warp(void);                /* eQ */
void tw_pseudo_exp(void);               /* ze */
void reverse_time_intervals2(void);     /* tR */
void interp_n_vals(void);               /* IV */
void convert_to_edits(void);    /* sE */
void cosin_spline(void);                /* cs */
void distance_from_grid(void);  /* dg */
void sinusoid(void);                    /* ss */
void warped_times(int isdiff);  /* wt, wo */
void random_pairs(void);                        /* rG */
void random_pairs_restrained(void);     /* rJ */
void cumuladd(void);                            /* ga */
void randomise_Ntimes(int *perm);       /* Rx */
void tw_psuedopan(void);                        /* pp */

//double drand48(void);
//void initrand48(void);
void rand_ints_with_fixed_ends(void);   /* ri */
void rand_zigs(void);                                   /* zz */
void eliminate_dupltext(void);                  /* eb */
void sinjoin(char val);         /* sc, sv, sx */
void brktime_owarp(void);               /* eY */
void just_intonation_Hz(void);          /* Zh */
void just_intonation_midi(void);        /* ZM */
void create_just_intonation_Hz(void);   /* ZH */
void create_just_intonation_midi(void); /* Zm */
void insert_intermediate_valp(void);    /* ip */
void random_warp(void); /* Zs */
