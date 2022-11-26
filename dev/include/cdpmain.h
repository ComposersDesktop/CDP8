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
#define PRE_CMDLINE_DATACNT      (37)

int     establish_datastructure(dataptr *dz);
int     superfree(dataptr dz);
int     get_process_and_mode_from_cmdline(int *cmdlinecnt,char ***cmdline,dataptr dz);
int     usage(int argc,char *argv[]);
int     usage1(void);
int     usage2(char *);
int     usage3(char *str1,char *str2);

int     parse_tk_data(int argc,char *argv[],char ***cmdline,int *cmdlinecnt,dataptr dz);
int     allocate_filespace(dataptr dz);
int     setup_particular_application(dataptr dz);
int     allocate_filespace(dataptr dz);
int             set_legal_internalparam_structure(int process,int mode,aplptr ap);
int             read_parameters_and_flags(char ***cmdline,int *cmdlinecnt,dataptr dz);
int     read_special_data(char *str,dataptr dz);
int     read_and_test_pitch_or_transposition_brkvals(FILE *fp,char *filename,
                        double **brktable,int *brksize,int which_type,double minval,double maxval);
int     convert_brkpntdata_to_window_by_window_array
                        (double *brktable,int brksize,float **thisarray,int wlen,float timestep);
int     convert_dB_at_or_below_zero_to_gain(double *val);
void    out_of_range_in_brkfile(char *filename,double val,double loval,double hival);
int     interp_val(double *val,double ttime,double *startoftab,double *endoftab,double **p);
int     check_param_validity_and_consistency(dataptr dz);
int     get_maxvalue_in_brktable(double *brkmax,int paramno,dataptr dz);
int     get_minvalue_in_brktable(double *brkmin,int paramno,dataptr dz);
int     convert_pch_or_transpos_data_to_brkpnttable(int *brksize,float *floatbuf,float frametime,int array_no,dataptr dz);

int     groucho_process_file(dataptr dz);
int     spec_process_file(dataptr dz);

int     complete_output(dataptr dz);
int     truncate_outfile(dataptr dz);
int     write_exact_samps(float *buffer,int samps_to_write,dataptr dz);
int     write_samps(float *bbuf,int samps_to_write,dataptr dz);
int     write_samps_no_report(float *bbuf,int samps_to_write,int *samps_written,dataptr dz);
int     write_samps_to_elsewhere(int ofd, float *buffer,int samps_to_write,dataptr dz);
int     write_brkfile(FILE *fptext,int brklen,int array_no,dataptr dz);



int     convert_pch_or_transpos_data_to_brkpnttable(int *brksize,float *floatbuf,
                        float frametime,int array_no,dataptr dz);
int     headwrite(int ofd,dataptr dz);

int     create_sized_outfile(char *filename,dataptr dz);
/*RWD.7.98  for sfsys98 */
int     create_sized_outfile_formatted(const char *filename,int srate,int channels, int stype,dataptr dz);
void    display_virtual_time(int value,dataptr dz);

                /* groucho */
int     reset(int i,int samples,float *bbuf,int *lastzero,/*int*/float *cyclemax);
int     get_mixdata_in_line(int wordcnt,char **wordstor,int total_words,double *time,int *chans,
                        double *llevel,double *lpan,double *rlevel,double *rpan,int filecnt,dataptr dz);
int     finalise_and_check_mixdata_in_line(int wordcnt,int chans,
                        double llevel,double *lpan,double *rlevel,double *rpan);
int     create_sndbufs_for_envel(dataptr dz);
/* RWD was byte_windowsize */
int     generate_samp_windowsize(fileptr thisfile,dataptr dz);
void    upsort(double *scti,int scatcnt);
int     open_file_retrieve_props_open(int filecnt,char *filename,int *srate,dataptr dz);
int     force_value_at_zero_time(int paramno,dataptr dz);
int     establish_groucho_bufptrs_and_extra_buffers(dataptr dz);
int     pvoc_preprocess(dataptr dz);
int     pvoc_process(dataptr dz);
int     pvoc_process_addon(unsigned int samps_so_far,dataptr dz);        /* was ~bytes~ */

int     modspace_pconsistency(dataptr dz);
int     modpitch_pconsistency(dataptr dz);
int             modspace_preprocess(dataptr dz);
int             scaledpan_preprocess(dataptr dz);
int     create_delay_buffers(dataptr dz);
int     create_stadium_buffers(dataptr dz);
int             delay_preprocess(dataptr dz);
int             do_delay(dataptr dz);
int     stadium_pconsistency(dataptr dz);
int     do_stadium(dataptr dz);
int     create_shred_buffers(dataptr dz);
int     shred_pconsistency(dataptr dz);
int     shred_preprocess(dataptr dz);
int     shred_process(dataptr dz);

void    free_wordstors(dataptr dz);

#define INITIALISE_DEFAULT_VALUES       initialise_param_values

//int create_sized_outfile(char *filename,dataptr dz);
int setup_param_ranges_and_defaults(dataptr dz);
int handle_formants(int *cmdlinecnt,char ***cmdline,dataptr dz);
int handle_formant_quiksearch(int *cmdlinecnt,char ***cmdline,dataptr dz);
int handle_special_data(int *cmdlinecnt,char ***cmdline,dataptr dz);
int read_formant_qksrch_flag(char ***cmdline,int *cmdlinecnt,dataptr dz);
int do_housekeep_files(char *filename,dataptr dz);
int check_repitch_type(dataptr dz);
int exceptional_repitch_validity_check(int *is_valid,dataptr dz);
int setup_brktablesizes(infileptr infile_info,dataptr dz);
int store_filename(char *filename,dataptr dz);
int store_further_filename(int n,char *filename,dataptr dz);
int count_and_allocate_for_infiles(int argc,char *argv[],dataptr dz);
int count_infiles(int argc,char *argv[],dataptr dz);
int count_bundle_files(int argc,char *argv[],dataptr dz);
void put_default_vals_in_all_params(dataptr dz);
int make_initial_cmdline_check(int *argc,char **argv[]);
int test_application_validity(infileptr infile_info,int process,int *valid,dataptr dz);
int parse_infile_and_hone_type(char *filename,int *valid,dataptr dz);
int set_chunklens_and_establish_windowbufs(dataptr dz);
int setup_file_outname_where_ness(char *filename,dataptr dz);
int set_special_process_sizes(dataptr dz);
int copy_parse_info_to_main_structure(infileptr infile_info,dataptr dz);
int allocate_filespace(dataptr dz);
int handle_extra_infiles(char ***cmdline,int *cmdlinecnt,dataptr dz);
int handle_outfile(int *cmdlinecnt,char ***cmdline,int is_launched,dataptr dz);
int print_messages_and_close_sndfiles(int exit_status,int is_launched,dataptr dz);

int     x(int y); /* TESTING ONLY */

int pvoc_out(int floats_out,unsigned int *bytes_so_far,char *orig_outfilename,
        char *root_outname, int jj, dataptr dz);

int get_the_vowels(char *filename,double **times,int **vowels,int *vcnt,dataptr dz);
int get_vowel (char *str);

void insert_separator_on_sndfile_name(char *filename,int cnt);
