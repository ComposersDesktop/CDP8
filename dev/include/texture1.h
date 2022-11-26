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



//REMOVED functions that are now LOCAL STATIC
//int  initialise_texture_structure(dataptr dz);
//int  set_up_and_fill_insample_buffers(insamptr **insound,dataptr dz);
//int  setup_texflag(texptr tex,dataptr dz);
//int  preset_some_internal_texture_params(dataptr dz);
//int  install_the_internal_flags(int total_flags,int internal_flags,dataptr dz);
//int  get_the_notedata(texptr tex,dataptr dz);
int  attenuate_input_sounds(dataptr dz);
//int  extend_timeset(dataptr dz);
//int  generate_timeset(dataptr dz);
//int  do_prespace(motifptr tset,dataptr dz);
//int  pre_space(noteptr thisnote,dataptr dz);
//int  assign_timeset_hfset_motifsets(dataptr dz);
//int  massage_params(dataptr dz);

int  prepare_texture_parameters(dataptr dz);
int  make_texture(dataptr dz);
void upsort(double *scti,int scatcnt);
double quantise(double thistime,double timegrid);
int  initperm(int ***permm,dataptr dz);
int  make_new_note(noteptr *thisnote);

void swap(double *a,double *b);
void iswap(int *a,int *b);
//int  get_the_notedatafile(char *str,dataptr dz);
double octadjust(double thispitch);
int  do_amp_instr_dur(double *thisamp,unsigned char *thisinstr,double *thisdur,
         noteptr tsetnote,double thistime,dataptr dz);
int  make_shadow(motifptr tset,int *shadowsize,noteptr **shadow);
int  erase_shadow(int shadowsize,noteptr *shadow,motifptr tset);
void setup_decor(double *pptop,double *ppbot,int *shadindex,noteptr *tsetnote,dataptr dz);
int  setup_ornament
         (double *timeadjust,double *thistime,int *gpsize,noteptr *phrlastnote,
         double multiplier,noteptr *phrasenote,int phrno,dataptr dz);
int  set_motif_amp
         (noteptr tsetnote,double *thisamp,int gpsize,double ampstep,noteptr phrasenote,
         double rangemult,double *phraseamp,int phrno,unsigned char amptype);
int  set_ornament_amp
         (double *phraseamp,noteptr *phrlastnote,double *thisamp,noteptr phrasenote,int phrno,
         noteptr tsetnote,double ampstep,double rangemult,int gpsize,dataptr dz);
int  set_group_params(noteptr tsetnote,noteptr thisnote,double gpdense,double ampstep,double *thisamp,
        double *thistime,double thisdur,dataptr dz);
int  check_next_phrasenote_exists(noteptr *phrasenote,int texflag,dataptr dz);
noteptr getnextevent_to_decorate(noteptr tsetnote,int *shaddoindex,dataptr dz);
double  getnotetime(noteptr phrasenote,double time,double multiplier,double timeadjust,dataptr dz);
int     getmtfdur(noteptr tsetnote,noteptr phrasenote,double *dur,double multiplier,dataptr dz);
int  orn_or_mtf_amp_setup
        (int ampdirected,double *phrange,int phrno,double thisamp,int gpsize,double *rangemult,
        double *ampstep,unsigned char *amptype,unsigned char amptypestor,unsigned char amptypecnt,dataptr dz);
int  init_group_spatialisation
        (noteptr tsetnote,int shaddoindex,noteptr *shadow,int shadowsize,dataptr dz);
int  setspace(noteptr tsetnote,noteptr thisnote,int gpsize,dataptr dz);
int  do_mtf_params
         (noteptr thisnote,double thisamp,noteptr phrasenote,noteptr tsetnote,
         double ampdif,double notetime,double multiplier,dataptr dz);
int  setup_motif_or_ornament
         (double thistime,double *multiplier,int *phrno,noteptr *phrasenote,
         motifptr *phrase,dataptr dz);
int  position_and_size_decoration
         (double *thistime,double tsettime,double gpdense,int *gpsize,dataptr dz);
int  set_decor_amp
         (int ampdirected,double *thisamp,double *ampstep,int gpsize,unsigned char *amptype,
         unsigned char amptypecnt,unsigned char amptypestor,dataptr dz);
int  set_group_amp
         (noteptr tsetnote,double *thisamp,unsigned char *amptype, double *ampstep,int gpsize,
         unsigned char amptypecnt,unsigned char amptypestor,dataptr dz);
int  getvalue(int paramhi,int paramlo,double time,int z,double *val,dataptr dz);
int  igetvalue(int paramhi,int paramlo,double time,int z,int *ival,dataptr dz);
int  doperm(int k,int pindex,int *val,dataptr dz);
noteptr gethipitch(noteptr tsetnote,int *shaddoindex);
int  do_grp_ins(unsigned char thisinstr,unsigned char *val,dataptr dz);
int  pscatx(double range,double bottom,int pindex,double *val,dataptr dz);
int  gettritype(int k,unsigned stor,unsigned char *val);
int  get_density_val(double thistime,double *gpdense,dataptr dz);
int  set_motifs(int phrcount,motifptr *phrase,int *phrnotecnt,double *phraseamp,
                double *phrange,noteptr *phrlastnote);

int  make_new_note(noteptr *thisnote);
void del_note(noteptr thisnote,motifptr thismotif);
int  arrange_notes_in_timeorder(motifptr mtf);

int  do_texture(dataptr dz);
int  do_simple_hftexture(dataptr dz);
int  do_clumped_hftexture(dataptr dz);
int  produce_texture_sound(dataptr dz);

//int texture_pconsistency(dataptr dz);
