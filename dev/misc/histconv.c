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
#include <string.h>
#include <ctype.h>

#define CDP_PROPS_CNT (34)
#define CDP_PROGRAM_DIR	("cdprog")
#define	ENDOFSTR	('\0')
#define NUMERICVAL_MARKER ('@')

void	get_progname(int progno,char *p);
int 	skipwordandspace(char **q);
int 	getvalandskipspace(char **q,int *ival);
int 	getwordandskipspace(char **q,char *j);
void 	get_modename(int progno,int modeno,char *p);
void 	to_ucase(char *str);
const char* cdp_version = "7.1.0";

int main(int argc,char *argv[])
{
	char temp[1000], temp2[1000], temp3[32], c, *p, *q, *r;
	FILE *fpo, *fpi;
	int OK, i, isprog = 1, linecnt = 1, filecnt, progno, modeno;

	if(argc==2 && (strcmp(argv[1],"--version") == 0)) {
		fprintf(stdout,"%s\n",cdp_version);
		fflush(stdout);
		return 0;
	}
	if(argc != 3) {
		fprintf(stderr,"USAGE: histconv infile outfile\n");
		return 1;
	}
	if((fpi = fopen(argv[1],"r"))==NULL) {
		fprintf(stderr,"Cannot open file %s\n",argv[1]);
		return 1;
	}
	if((fpi = fopen(argv[1],"r"))==NULL) {
		fprintf(stderr,"Cannot open file %s to read\n",argv[1]);
		return 1;
	}
	if((fpo = fopen(argv[2],"w"))==NULL) {
		fprintf(stderr,"Cannot open file %s to write\n",argv[2]);
		return 1;
	}
	while(fgets(temp,1000,fpi)!=NULL) {
		if(strlen(temp) <= 0) {
			fprintf(fpo,"Insufficient information on line %d\n",linecnt);
			isprog = !isprog;
			linecnt++;
			continue;
		}
		if(isprog) {
			p = temp;
			if(*p == '#') {
				p++;
				p += strlen(CDP_PROGRAM_DIR);
				p++;
				if(!isprint(*p)) {
					fprintf(fpo,"Invalid program name on line %d\n",linecnt);
					isprog = !isprog;
					linecnt++;
					continue;
				}
				q = temp;	
				while(*p != ENDOFSTR) {		/* Get rid of directory name */
					*q = *p;
					q++;
					p++;
				}
				*q = ENDOFSTR;
				p = temp;
 				if(!skipwordandspace(&p)) {
					fprintf(fpo,"Not enough words on line %d\n",linecnt);
					isprog = !isprog;
					linecnt++;
					continue;
				}
				c = *p;
				*p = ENDOFSTR;
				strcpy(temp2,temp);
				q = temp2;
				to_ucase(temp2);
				*p = c;
				if(!getvalandskipspace(&p,&progno)) {
					fprintf(fpo,"Failed to get program number on line %d\n",linecnt);
					isprog = !isprog;
					linecnt++;
					continue;
				}

				r = temp3;
				get_progname(progno,r);
				to_ucase(temp3);								
				strcat(temp2,temp3);
				strcat(temp2," ");

				if(!getvalandskipspace(&p,&modeno)) {
					fprintf(fpo,"Failed to get mode number on line %d\n",linecnt);
					isprog = !isprog;
					linecnt++;
					continue;
				}
				get_modename(progno,modeno,r);
				to_ucase(temp3);								
				strcat(temp2,temp3);
				strcat(temp2," ");

				if(!getvalandskipspace(&p,&filecnt)) {
					fprintf(fpo,"Failed to get filecnt on line %d\n",linecnt);
					isprog = !isprog;
					linecnt++;
					continue;
				}
				if(*p == ENDOFSTR) {
					fprintf(fpo,"Insufficient information on line %d\n",linecnt);
					isprog = !isprog;
					linecnt++;
					continue;
				}
				if(filecnt > 0) {				
					OK = 1;
					for(i=0;i<CDP_PROPS_CNT;i++) {
						if(!skipwordandspace(&p)) {
							OK  = 0;
							break;
						}
					}
					if(!OK) {
						fprintf(fpo,"Not enough words on line %d\n",linecnt);
						isprog = !isprog;
						linecnt++;
						continue;
					}										
					r = temp3;
					for(i=0;i<filecnt;i++) {
						if(!getwordandskipspace(&p,r)) {
							fprintf(fpo,"Failed to get sufficient infile names on line %d\n",linecnt);
							isprog = !isprog;
							linecnt++;
							continue;
						}
						strcat(temp2,r);
						strcat(temp2," ");
					}
				}
   				skipwordandspace(&p);			/* skip outfilename */
				r = temp3;					/* remove numericval markers */
				while(*p != ENDOFSTR) {
					getwordandskipspace(&p,r);
					if(*r == NUMERICVAL_MARKER) {
						r++;
						strcat(temp2,r);
						strcat(temp2," ");
						r--;
					} else {
						strcat(temp2,r);
						strcat(temp2," ");
					}
				}
				*r++ = '\n';
				*r = ENDOFSTR;
				fprintf(fpo,"%s",temp2);
			} else {
				p = temp;
				r = temp3;					/* remove numericval markers */
				q = temp2;
				*q = ENDOFSTR;
				while(*p != ENDOFSTR) {
					getwordandskipspace(&p,r);
					if(*r == NUMERICVAL_MARKER) {
						r++;
						strcat(temp2,r);
						strcat(temp2," ");
						r--;
					} else {
						strcat(temp2,r);
						strcat(temp2," ");
					}
				}
				*r++ = '\n';
				*r = ENDOFSTR;
				fprintf(fpo,"%s",temp2);
			}
		} else {
			fprintf(fpo,"%s",temp);
		}
		isprog = !isprog;
		linecnt++;
	}
	return 0;
}


int skipwordandspace(char **q)
{
	char *p;
	p = *q;
	while(!isspace(*p) && *p != ENDOFSTR)
		p++;
	if(*p==ENDOFSTR)
		return 0;
	while(isspace(*p) && *p != ENDOFSTR)
		p++;
	*q = p;
	if(*p==ENDOFSTR)
		return 0;
	return 1;
}


int getvalandskipspace(char **q,int *ival)
{
	char *p, *r, c;
	p = *q;
	r = p;
	while(!isspace(*p))
		p++;
	c = *p;
	*p = ENDOFSTR;
	if(sscanf(r,"%d",ival)<1) {
		return 0;
	}
	*p = c;
	while(isspace(*p))
		p++;
	*q = p;
	return 1;
}


int getwordandskipspace(char **q,char *j)
{
	char *p, *r, c;
	p = *q;
	r = p;
	while(!isspace(*p))
		p++;
	c = *p;
	*p = ENDOFSTR;
	strcpy(j,r);
	*p = c;
	while(isspace(*p))
		p++;
	*q = p;
	return 1;
}


void get_progname(int progno,char *p)
{
	switch(progno){
	case(1):  	strcpy(p,"gain");	break;
	case(2):  	strcpy(p,"gate");	break;
	case(3):  	strcpy(p,"bare_partials");	break;
	case(4):  	strcpy(p,"clean");	break;
	case(5):  	strcpy(p,"cut");	break;
	case(6):  	strcpy(p,"grab_window");	break;
	case(7):  	strcpy(p,"magnify_window");	break;
	case(8):  	strcpy(p,"spectrum");	break;
	case(9):  	strcpy(p,"time");	break;
	case(10): 	strcpy(p,"alternate_harmonics");	break;
	case(11): 	strcpy(p,"octave_shift");	break;
	case(12): 	strcpy(p,"pitch_shift");	break;
	case(13): 	strcpy(p,"tune_spectrum");	break;
	case(14): 	strcpy(p,"choose_partials");	break;
	case(15): 	strcpy(p,"chord");	break;
	case(16): 	strcpy(p,"chord_(keep_fmnts)");	break;
	case(17): 	strcpy(p,"filter");	break;
	case(18): 	strcpy(p,"graphic_eq");	break;
	case(19): 	strcpy(p,"bands");	break;
	case(20): 	strcpy(p,"arpeggiate");	break;
	case(21): 	strcpy(p,"pluck");	break;
	case(22): 	strcpy(p,"tracery");	break;
	case(23): 	strcpy(p,"blur_&_trace");	break;
	case(24): 	strcpy(p,"accumulate");	break;
	case(25): 	strcpy(p,"exaggerate");	break;
	case(26): 	strcpy(p,"focus");	break;
	case(27): 	strcpy(p,"fold_in");	break;
	case(28): 	strcpy(p,"freeze");	break;
	case(29): 	strcpy(p,"step_through");	break;
	case(30): 	strcpy(p,"average");	break;
	case(31): 	strcpy(p,"blur");	break;
	case(32): 	strcpy(p,"supress");	break;
	case(33): 	strcpy(p,"chorus");	break;
	case(34): 	strcpy(p,"drunkwalk");	break;
	case(35): 	strcpy(p,"shuffle");	break;
	case(36): 	strcpy(p,"weave");	break;
	case(37): 	strcpy(p,"noise");	break;
	case(38): 	strcpy(p,"scatter");	break;
	case(39): 	strcpy(p,"spread");	break;
	case(40): 	strcpy(p,"linear_shift");	break;
	case(41): 	strcpy(p,"inner_glissando");	break;
	case(42): 	strcpy(p,"waver");	break;
	case(43): 	strcpy(p,"warp");	break;
	case(44): 	strcpy(p,"invert");	break;
	case(45): 	strcpy(p,"glide");	break;
	case(46): 	strcpy(p,"bridge");	break;
	case(47): 	strcpy(p,"morph");	break;
	case(48): 	strcpy(p,"extract_pitch");	break;
	case(49): 	strcpy(p,"track_pitch");	break;
	case(50): 	strcpy(p,"approximate_pitch");	break;
	case(51): 	strcpy(p,"exaggerate_contour");	break;
	case(52): 	strcpy(p,"invert_pitch_contour");	break;
	case(53): 	strcpy(p,"quantise_pitch");	break;
	case(54): 	strcpy(p,"randomise_pitch");	break;
	case(55): 	strcpy(p,"smooth_pitch_contour");	break;
	case(56): 	strcpy(p,"shift_pitch");	break;
	case(57): 	strcpy(p,"vibrato_pitch_data");	break;
	case(58): 	strcpy(p,"cut_pitchfile");	break;
	case(59): 	strcpy(p,"mend_pitchfile");	break;
	case(60): 	strcpy(p,"repitch_pitchdata");	break;
	case(61): 	strcpy(p,"repitch_(to_textfile)");	break;
	case(62): 	strcpy(p,"transpose");	break;
	case(63): 	strcpy(p,"transpose(keep_fmnts)");	break;
	case(64): 	strcpy(p,"extract");	break;
	case(65): 	strcpy(p,"impose");	break;
	case(66): 	strcpy(p,"vocode");	break;
	case(67): 	strcpy(p,"view");	break;
	case(68): 	strcpy(p,"get_&_view");	break;
	case(69): 	strcpy(p,"add_formants_to_pitch");	break;
	case(70): 	strcpy(p,"sum");	break;
	case(71): 	strcpy(p,"difference");	break;
	case(72): 	strcpy(p,"interleave");	break;
	case(73): 	strcpy(p,"windowwise_max");	break;
	case(74): 	strcpy(p,"mean");	break;
	case(75): 	strcpy(p,"cross_channels");	break;
	case(76): 	strcpy(p,"window_count");	break;
	case(77): 	strcpy(p,"channel");	break;
	case(78): 	strcpy(p,"get_frequency");	break;
	case(79): 	strcpy(p,"view_level_as_pseudo-sndfile");	break;
	case(80): 	strcpy(p,"print_octbands_to_file");	break;
	case(81): 	strcpy(p,"print_energy_centres_to_file");	break;
	case(82): 	strcpy(p,"print_freq_peaks_to_file");	break;
	case(83): 	strcpy(p,"print_analysis_data_to_file_only");	break;
	case(84): 	strcpy(p,"display_info_on_pitchfile");	break;
	case(85): 	strcpy(p,"check_for_pitch_zeros");	break;
	case(86): 	strcpy(p,"pitch_view_as_psuedo-sndfile");	break;
	case(87): 	strcpy(p,"pitch_to_testtone_spectrum");	break;
	case(88): 	strcpy(p,"print_pitch_data_to_file_only");	break;
	case(100): 	strcpy(p,"cyclecnt");	break;
	case(101): 	strcpy(p,"reshape");	break;
	case(102): 	strcpy(p,"envelope");	break;
	case(103): 	strcpy(p,"average");	break;
	case(104): 	strcpy(p,"omit");	break;
	case(105): 	strcpy(p,"multiply");	break;
	case(106): 	strcpy(p,"divide");	break;
	case(107): 	strcpy(p,"harmonic");	break;
	case(108): 	strcpy(p,"fractal");	break;
	case(109): 	strcpy(p,"reverse");	break;
	case(110): 	strcpy(p,"shuffle");	break;
	case(111): 	strcpy(p,"repeat");	break;
	case(112): 	strcpy(p,"interpolate");	break;
	case(113): 	strcpy(p,"delete");	break;
	case(114): 	strcpy(p,"replace");	break;
	case(115): 	strcpy(p,"telescope");	break;
	case(116): 	strcpy(p,"filter");	break;
	case(117): 	strcpy(p,"interact");	break;
	case(118): 	strcpy(p,"pitch");	break;
	case(119): 	strcpy(p,"zigzag");	break;
	case(120): 	strcpy(p,"loop");	break;
	case(121): 	strcpy(p,"scramble");	break;
	case(122): 	strcpy(p,"iterate");	break;
	case(123): 	strcpy(p,"drunkwalk");	break;
	case(124): 	strcpy(p,"simple");	break;
	case(125): 	strcpy(p,"of groups");	break;
	case(126): 	strcpy(p,"decorated");	break;
	case(127): 	strcpy(p,"pre-decorations");	break;
	case(128): 	strcpy(p,"post-decorations");	break;
	case(129): 	strcpy(p,"ornamented");	break;
	case(130): 	strcpy(p,"pre-ornate");	break;
	case(131): 	strcpy(p,"post-ornate");	break;
	case(132): 	strcpy(p,"of_motifs");	break;
	case(133): 	strcpy(p,"motifs_in_hf");	break;
	case(134): 	strcpy(p,"timed");	break;
	case(135): 	strcpy(p,"timed_groups");	break;
	case(136): 	strcpy(p,"timed_motifs");	break;
	case(137): 	strcpy(p,"timed_mtfs_in_hf");	break;
	case(138): 	strcpy(p,"count");	break;
	case(139): 	strcpy(p,"omit");	break;
	case(140): 	strcpy(p,"duplicate");	break;
	case(141): 	strcpy(p,"reorder");	break;
	case(142): 	strcpy(p,"repitch");	break;
	case(143): 	strcpy(p,"rerhythm");	break;
	case(144): 	strcpy(p,"remotif");	break;
	case(145): 	strcpy(p,"timewarp");	break;
	case(146): 	strcpy(p,"get");	break;
	case(147): 	strcpy(p,"position");	break;
	case(148): 	strcpy(p,"align");	break;
	case(149): 	strcpy(p,"reverse");	break;
	case(150): 	strcpy(p,"create");	break;
	case(151): 	strcpy(p,"extract");	break;
	case(152): 	strcpy(p,"impose");	break;
	case(153): 	strcpy(p,"replace");	break;
	case(154): 	strcpy(p,"warping");	break;
	case(155): 	strcpy(p,"reshaping");	break;
	case(156): 	strcpy(p,"replotting");	break;
	case(157): 	strcpy(p,"dovetailing");	break;
	case(158): 	strcpy(p,"curtailing");	break;
	case(159): 	strcpy(p,"swell");	break;
	case(160): 	strcpy(p,"attack");	break;
	case(161): 	strcpy(p,"pluck");	break;
	case(162): 	strcpy(p,"tremolo");	break;
	case(163): 	strcpy(p,"bin_to_brk");	break;
	case(164): 	strcpy(p,"bin_to_db-brk");	break;
	case(165): 	strcpy(p,"brk_to_bin");	break;
	case(166): 	strcpy(p,"db-brk_to_bin");	break;
	case(167): 	strcpy(p,"db-brk_to_brk");	break;
	case(168): 	strcpy(p,"brk_to_db-brk");	break;
	case(169): 	strcpy(p,"merge_two_sounds");	break;
	case(170): 	strcpy(p,"crossfade");	break;
	case(171): 	strcpy(p,"merge_channels");	break;
	case(172): 	strcpy(p,"inbetweening");	break;
	case(173): 	strcpy(p,"mix");	break;
	case(174): 	strcpy(p,"get_level");	break;
	case(175): 	strcpy(p,"attenuate");	break;
	case(176): 	strcpy(p,"shuffle");	break;
	case(177): 	strcpy(p,"timewarp");	break;
	case(178): 	strcpy(p,"spacewarp");	break;
	case(179): 	strcpy(p,"sync");	break;
	case(180): 	strcpy(p,"sync_attack");	break;
	case(181): 	strcpy(p,"test");	break;
	case(182): 	strcpy(p,"format");	break;
	case(183): 	strcpy(p,"dummy");	break;
	case(184): 	strcpy(p,"variable");	break;
	case(185): 	strcpy(p,"fixed");	break;
	case(186): 	strcpy(p,"lopass/hipass");	break;
	case(187): 	strcpy(p,"variable");	break;
	case(188): 	strcpy(p,"bank");	break;
	case(189): 	strcpy(p,"bank_frqs");	break;
	case(190): 	strcpy(p,"userbank");	break;
	case(191): 	strcpy(p,"varibank");	break;
	case(192): 	strcpy(p,"sweeping");	break;
	case(193): 	strcpy(p,"iterated");	break;
	case(194): 	strcpy(p,"phasing");	break;
	case(195): 	strcpy(p,"loudness");	break;
	case(196): 	strcpy(p,"spatialisation");	break;
	case(197): 	strcpy(p,"pitch");	break;
	case(198): 	strcpy(p,"rev/echo");	break;
	case(199): 	strcpy(p,"brassage");	break;
	case(200): 	strcpy(p,"sausage");	break;
	case(201): 	strcpy(p,"radical");	break;
	case(202): 	strcpy(p,"analysis");	break;
	case(203): 	strcpy(p,"synthesis");	break;
	case(204): 	strcpy(p,"extract");	break;
	case(206): 	strcpy(p,"cutout_&_keep");	break;
	case(207): 	strcpy(p,"cutend_&_keep");	break;
	case(208): 	strcpy(p,"cutout_at_zero-crossings");	break;
	case(209): 	strcpy(p,"remove_segment");	break;
	case(210): 	strcpy(p,"remove_many_segments");	break;
	case(211): 	strcpy(p,"insert_sound");	break;
	case(212): 	strcpy(p,"insert_silence");	break;
	case(213): 	strcpy(p,"join");	break;
	case(214): 	strcpy(p,"multiples");	break;
	case(215): 	strcpy(p,"extract/convert_channels");	break;
	case(216): 	strcpy(p,"&_clean");	break;
	case(217): 	strcpy(p,"change_specification");	break;
	case(218): 	strcpy(p,"bundle");	break;
	case(219): 	strcpy(p,"sort_files");	break;
	case(220): 	strcpy(p,"backup");	break;
	case(221): 	strcpy(p,"recover_from_dump");	break;
	case(222): 	strcpy(p,"diskspace");	break;
	case(223): 	strcpy(p,"properties");	break;
	case(224): 	strcpy(p,"duration");	break;
	case(225): 	strcpy(p,"list_sound_durations");	break;
	case(226): 	strcpy(p,"sum_durations");	break;
	case(227): 	strcpy(p,"subtract_durations");	break;
	case(228): 	strcpy(p,"sample_count_as_time");	break;
	case(229): 	strcpy(p,"time_as_sample_count");	break;
	case(230): 	strcpy(p,"maximum_sample");	break;
	case(231): 	strcpy(p,"loudest_channel");	break;
	case(232): 	strcpy(p,"largest_hole");	break;
	case(233): 	strcpy(p,"compare_files");	break;
	case(234): 	strcpy(p,"compare_channels");	break;
	case(235): 	strcpy(p,"print_sound_data");	break;
	case(236): 	strcpy(p,"convert_units");	break;
	case(237): 	strcpy(p,"waveform");	break;
	case(238): 	strcpy(p,"noise");	break;
	case(239): 	strcpy(p,"silent_file");	break;
	case(240): 	strcpy(p,"extract_column");	break;
	case(241): 	strcpy(p,"insert_column");	break;
	case(242): 	strcpy(p,"join_columns");	break;
	case(243): 	strcpy(p,"column_maths");	break;
	case(244): 	strcpy(p,"column_music");	break;
	case(245): 	strcpy(p,"column_rand");	break;
	case(246): 	strcpy(p,"column_sort");	break;
	case(247): 	strcpy(p,"column_create");	break;
	case(248): 	strcpy(p,"hold");	break;
	case(249): 	strcpy(p,"remove_copies");	break;
	case(250): 	strcpy(p,"vectors");	break;
	case(251): 	strcpy(p,"silence_masks");	break;
	default:	strcpy(p,"UNKNOWN_PROGRAM");	break;
	}
}

void get_modename(int progno,int modeno,char *p)
{
	switch(progno) {
	case(4):
		switch(modeno) {
		case(1):	strcpy(p,"from_specified_time");	break; 
		case(2):	strcpy(p,"anywhere");	break; 
		case(3):	strcpy(p,"above_specified_frq");	break; 
		case(4):	strcpy(p,"by_comparison_method");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(8):
		switch(modeno) {
		case(1):	strcpy(p,"above_given_frq");	break; 
		case(2):	strcpy(p,"below_given_frq");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(9):
		switch(modeno) {
		case(1):	strcpy(p,"do_time_stretc_");	break; 
		case(2):	strcpy(p,"get_output_length");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(10):
		switch(modeno) {
		case(1):	strcpy(p,"delete_odd_harmonics");	break; 
		case(2):	strcpy(p,"delete_even_harmonics");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(11):
		switch(modeno) {
		case(1):	strcpy(p,"up");	break; 
		case(2):	strcpy(p,"down");	break; 
		case(3):	strcpy(p,"down_with_bass_boost");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(12):
		switch(modeno) {
		case(1):	strcpy(p,"8va_shift_up");	break; 
		case(2):	strcpy(p,"8va_shift_down");	break; 
		case(3):	strcpy(p,"8va_shift_up_and_down");	break; 
		case(4):	strcpy(p,"shift_up");	break; 
		case(5):	strcpy(p,"shift_down");	break; 
		case(6):	strcpy(p,"shift_up_and_down");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(13):
		switch(modeno) {
		case(1):	strcpy(p,"tunings_as_frqs");	break; 
		case(2):	strcpy(p,"tunings_as_midi");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(14):
		switch(modeno) {
		case(1):	strcpy(p,"harmonic_series");	break; 
		case(2):	strcpy(p,"octaves");	break; 
		case(3):	strcpy(p,"odd_harmonics_only");	break; 
		case(4):	strcpy(p,"linear_frq_steps");	break; 
		case(5):	strcpy(p,"displaced_harmonics");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(17):
		switch(modeno) {
		case(1):	strcpy(p,"high_pass");	break; 
		case(2):	strcpy(p,"high_pass_normalised");	break; 
		case(3):	strcpy(p,"low_pass");	break; 
		case(4):	strcpy(p,"low_pass_normalised");	break; 
		case(5):	strcpy(p,"high_pass_with_gain");	break; 
		case(6):	strcpy(p,"low_pass_with_gain");	break; 
		case(7):	strcpy(p,"bandpass");	break; 
		case(8):	strcpy(p,"bandpass_normalised");	break; 
		case(9):	strcpy(p,"notch");	break; 
		case(10):	strcpy(p,"notch_normalised");	break; 
		case(11):	strcpy(p,"bandpass_with_gain");	break; 
		case(12):	strcpy(p,"notch_with_gain");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(18):
		switch(modeno) {
		case(1):	strcpy(p,"standard_bandwidth");	break; 
		case(2):	strcpy(p,"various_bandwidths");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(20):
		switch(modeno) {
		case(1):	strcpy(p,"on");	break; 
		case(2):	strcpy(p,"boost");	break; 
		case(3):	strcpy(p,"boost_below");	break; 
		case(4):	strcpy(p,"boost_above");	break; 
		case(5):	strcpy(p,"on_below");	break; 
		case(6):	strcpy(p,"on_above");	break; 
		case(7):	strcpy(p,"once_below");	break; 
		case(8):	strcpy(p,"once_above");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(22):
		switch(modeno) {
		case(1):	strcpy(p,"trace_all");	break; 
		case(2):	strcpy(p,"trace_above_frq");	break; 
		case(3):	strcpy(p,"trace_below_frq");	break; 
		case(4):	strcpy(p,"trace_between_frqs");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(28):
		switch(modeno) {
		case(1):	strcpy(p,"amplitudes");	break; 
		case(2):	strcpy(p,"frequencies");	break; 
		case(3):	strcpy(p,"amps_&_frqs");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(33):
		switch(modeno) {
		case(1):	strcpy(p,"scatter_amps");	break; 
		case(2):	strcpy(p,"scatter_frqs");	break; 
		case(3):	strcpy(p,"scatter_frqs_up");	break; 
		case(4):	strcpy(p,"scatter_frqs_down");	break; 
		case(5):	strcpy(p,"scatter_amps_&_frqs");	break; 
		case(6):	strcpy(p,"scatter_amp_&_frqs_up");	break; 
		case(7):	strcpy(p,"scatter_amp_&_frqs_down");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(40):
		switch(modeno) {
		case(1):	strcpy(p,"shift_all");	break; 
		case(2):	strcpy(p,"shift_above_frq");	break; 
		case(3):	strcpy(p,"shift_below_frq");	break; 
		case(4):	strcpy(p,"shift_between_frqs");	break; 
		case(5):	strcpy(p,"shift_outside_frqs");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(41):
		switch(modeno) {
		case(1):	strcpy(p,"shepard_tone_glis");	break; 
		case(2):	strcpy(p,"inharmonic_glis");	break; 
		case(3):	strcpy(p,"self_glis");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(42):
		switch(modeno) {
		case(1):	strcpy(p,"standard");	break; 
		case(2):	strcpy(p,"user_specified");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(44):
		switch(modeno) {
		case(1):	strcpy(p,"standard");	break; 
		case(2):	strcpy(p,"retain_src_envelope");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(46):
		switch(modeno) {
		case(1):	strcpy(p,"standard");	break; 
		case(2):	strcpy(p,"outlevel_follows_minimum");	break; 
		case(3):	strcpy(p,"outlevel_follows_file1");	break; 
		case(4):	strcpy(p,"outlevel_follows_file2");	break; 
		case(5):	strcpy(p,"outlevel_moves_from_1_to_2");	break; 
		case(6):	strcpy(p,"outlevel_moves_from_2_to_1");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(47):
		switch(modeno) {
		case(1):	strcpy(p,"linear_or_curved");	break; 
		case(2):	strcpy(p,"cosinusoidal");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(48):
		switch(modeno) {
		case(1):	strcpy(p,"to_binary_file");	break; 
		case(2):	strcpy(p,"to_textfile");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(49):
		switch(modeno) {
		case(1):	strcpy(p,"to_binary_file");	break; 
		case(2):	strcpy(p,"to_textfile");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(50):
		switch(modeno) {
		case(1):	strcpy(p,"pitch_data_out");	break; 
		case(2):	strcpy(p,"transposition_data_out");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(51):
		switch(modeno) {
		case(1):	strcpy(p,"range:_pitch_out");	break; 
		case(2):	strcpy(p,"range:_transposition_out");	break; 
		case(3):	strcpy(p,"contour:_pitch_out");	break; 
		case(4):	strcpy(p,"contour:_transpos_out");	break; 
		case(5):	strcpy(p,"range_&_contour:_pitch_out");	break; 
		case(6):	strcpy(p,"range_&_contour:_transpos_out");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(52):
		switch(modeno) {
		case(1):	strcpy(p,"pitch_data_out");	break; 
		case(2):	strcpy(p,"transposition_data_out");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(53):
		switch(modeno) {
		case(1):	strcpy(p,"pitch_data_out");	break; 
		case(2):	strcpy(p,"transposition_data_out");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(54):
		switch(modeno) {
		case(1):	strcpy(p,"pitch_data_out");	break; 
		case(2):	strcpy(p,"transposition_data_out");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(55):
		switch(modeno) {
		case(1):	strcpy(p,"pitch_data_out");	break; 
		case(2):	strcpy(p,"transposition_data_out");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(56):
		switch(modeno) {
		case(1):	strcpy(p,"shift_by_ratio");	break; 
		case(2):	strcpy(p,"shift_by_semitones");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(57):
		switch(modeno) {
		case(1):	strcpy(p,"pitch_data_out");	break; 
		case(2):	strcpy(p,"transposition_data_out");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(58):
		switch(modeno) {
		case(1):	strcpy(p,"from_starttime");	break; 
		case(2):	strcpy(p,"to_endtime");	break; 
		case(3):	strcpy(p,"between_times");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(60):
		switch(modeno) {
		case(1):	strcpy(p,"pitch+pitch_to_transpos");	break; 
		case(2):	strcpy(p,"pitch+transpos_to_pitch");	break; 
		case(3):	strcpy(p,"transpos+transpos_to_transpos");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(61):
		switch(modeno) {
		case(1):	strcpy(p,"pitch+pitch_to_transpos");	break; 
		case(2):	strcpy(p,"pitch+transpos_to_pitch");	break; 
		case(3):	strcpy(p,"transpos+transpos_to_transpos");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(62):
		switch(modeno) {
		case(1):	strcpy(p,"transpos_as_ratio");	break; 
		case(2):	strcpy(p,"transpos_in_octaves");	break; 
		case(3):	strcpy(p,"transpos_in_semitones");	break; 
		case(4):	strcpy(p,"transpos_as_binary_data");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(63):
		switch(modeno) {
		case(1):	strcpy(p,"transpos_as_ratio");	break; 
		case(2):	strcpy(p,"transpos_in_octaves");	break; 
		case(3):	strcpy(p,"transpos_in_semitones");	break; 
		case(4):	strcpy(p,"transpos_as_binary_data");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(65):
		switch(modeno) {
		case(1):	strcpy(p,"replace_formants");	break; 
		case(2):	strcpy(p,"superimpose_formants");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(74):
		switch(modeno) {
		case(1):	strcpy(p,"mean_amp_&_pitch");	break; 
		case(2):	strcpy(p,"mean_amp_&_frq");	break; 
		case(3):	strcpy(p,"amp_file1:_mean_pich");	break; 
		case(4):	strcpy(p,"amp_file1:_mean_frq");	break; 
		case(5):	strcpy(p,"amp_file2:_mean_pich");	break; 
		case(6):	strcpy(p,"amp_file2:_mean_frq");	break; 
		case(7):	strcpy(p,"max_amp:_mean_pitch");	break; 
		case(8):	strcpy(p,"max_amp:_mean_frq");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(82):
		switch(modeno) {
		case(1):	strcpy(p,"order_by_frq_&_time");	break; 
		case(2):	strcpy(p,"order_by_loudness_&_time");	break; 
		case(3):	strcpy(p,"order_by_frq_(untimed)");	break; 
		case(4):	strcpy(p,"order_by_loudness_(untimed)");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(86):
		switch(modeno) {
		case(1):	strcpy(p,"see_pitch");	break; 
		case(2):	strcpy(p,"see_transposition");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(101):
		switch(modeno) {
		case(1):	strcpy(p,"fixed_level_square");	break; 
		case(2):	strcpy(p,"square");	break; 
		case(3):	strcpy(p,"fixed_level_triangle");	break; 
		case(4):	strcpy(p,"triangle");	break; 
		case(5):	strcpy(p,"invert_halfcycles");	break; 
		case(6):	strcpy(p,"click");	break; 
		case(7):	strcpy(p,"sine");	break; 
		case(8):	strcpy(p,"exaggerate_contour");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(102):
		switch(modeno) {
		case(1):	strcpy(p,"rising");	break; 
		case(2):	strcpy(p,"falling");	break; 
		case(3):	strcpy(p,"troughed");	break; 
		case(4):	strcpy(p,"user_defined");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(113):
		switch(modeno) {
		case(1):	strcpy(p,"in_given_order");	break; 
		case(2):	strcpy(p,"retain_loudest");	break; 
		case(3):	strcpy(p,"delete_weakest");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(116):
		switch(modeno) {
		case(1):	strcpy(p,"high_pass");	break; 
		case(2):	strcpy(p,"low_pass");	break; 
		case(3):	strcpy(p,"band_pass");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(117):
		switch(modeno) {
		case(1):	strcpy(p,"interleave");	break; 
		case(2):	strcpy(p,"resize");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(119):
		switch(modeno) {
		case(1):	strcpy(p,"random");	break; 
		case(2):	strcpy(p,"user_specified");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(120):
		switch(modeno) {
		case(1):	strcpy(p,"loop_advances_to_end");	break; 
		case(2):	strcpy(p,"give_output_duration");	break; 
		case(3):	strcpy(p,"give_loop_repetitions");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(121):
		switch(modeno) {
		case(1):	strcpy(p,"completely_random");	break; 
		case(2):	strcpy(p,"scramble_src:_then_again..");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(122):
		switch(modeno) {
		case(1):	strcpy(p,"give_duration");	break; 
		case(2):	strcpy(p,"give_count");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(123):
		switch(modeno) {
		case(1):	strcpy(p,"completely_drunk");	break; 
		case(2):	strcpy(p,"sober_moments");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(124):
	case(125):
	case(126):
	case(127):
	case(128):
	case(129):
	case(130):
	case(131):
	case(132):
	case(134):
	case(135):
	case(136):
		switch(modeno) {
		case(1):	strcpy(p,"over_harmonic_field");	break; 
		case(2):	strcpy(p,"over_harmonic_fields");	break; 
		case(3):	strcpy(p,"over_harmonic_set");	break; 
		case(4):	strcpy(p,"over_harmonic_sets");	break; 
		case(5):	strcpy(p,"neutral");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(133):
	case(137):
		switch(modeno) {
		case(1):	strcpy(p,"over_harmonic_field");	break; 
		case(2):	strcpy(p,"over_harmonic_fields");	break; 
		case(3):	strcpy(p,"over_harmonic_set");	break; 
		case(4):	strcpy(p,"over_harmonic_sets");	break; 
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(142):
	case(143):
	case(144):
		switch(modeno) {
		case(1):	strcpy(p,"no_grain_repeats");	break; 
		case(2):	strcpy(p,"repeat_each_grain");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(150):
	case(151):
		switch(modeno) {
		case(1):	strcpy(p,"binary_output");	break; 
		case(2):	strcpy(p,"textfile_output");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(152):
	case(153):
		switch(modeno) {
		case(1):	strcpy(p,"env_from_other_sndfile");	break; 
		case(2):	strcpy(p,"env_in_binary_file");	break; 
		case(3):	strcpy(p,"env_in_textfile");	break; 
		case(4):	strcpy(p,"env_in_db_textfile");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(154):
	case(155):
	case(156):
		switch(modeno) {
		case(1):	strcpy(p,"normalise");	break; 
		case(2):	strcpy(p,"time_reverse");	break; 
		case(3):	strcpy(p,"exaggerate");	break; 
		case(4):	strcpy(p,"attenuate");	break; 
		case(5):	strcpy(p,"lift_all");	break; 
		case(6):	strcpy(p,"time-stretch");	break; 
		case(7):	strcpy(p,"flatten");	break; 
		case(8):	strcpy(p,"gate");	break; 
		case(9):	strcpy(p,"invert");	break; 
		case(10):	strcpy(p,"limit");	break; 
		case(11):	strcpy(p,"corrugate");	break; 
		case(12):	strcpy(p,"expand");	break; 
		case(13):	strcpy(p,"trigger_bursts");	break; 
		case(14):	strcpy(p,"to_ceiling");	break; 
		case(15):	strcpy(p,"ducked");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(158):
		switch(modeno) {
		case(1):	strcpy(p,"give_start_&_end_of_fade");	break; 
		case(2):	strcpy(p,"give_start_&_dur_of_fade");	break; 
		case(3):	strcpy(p,"give_start_of_fade-to-end");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(160):
		switch(modeno) {
		case(1):	strcpy(p,"where_gate_exceeded");	break; 
		case(2):	strcpy(p,"near_time_given");	break; 
		case(3):	strcpy(p,"at_exact_time_given");	break; 
		case(4):	strcpy(p,"at_max_level_in_file");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(162):
		switch(modeno) {
		case(1):	strcpy(p,"frqwise");	break; 
		case(2):	strcpy(p,"pitchwise");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(170):
		switch(modeno) {
		case(1):	strcpy(p,"linear");	break; 
		case(2):	strcpy(p,"cosinusoidal");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(172):
		switch(modeno) {
		case(1):	strcpy(p,"automatic");	break; 
		case(2):	strcpy(p,"give_mix_ratios");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(174):
		switch(modeno) {
		case(1):	strcpy(p,"maximum_level");	break; 
		case(2):	strcpy(p,"clipping_times");	break; 
		case(3):	strcpy(p,"maxlevel_&_cliptimes");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(176):
		switch(modeno) {
		case(1):	strcpy(p,"duplicate_lines");	break; 
		case(2):	strcpy(p,"reverse_order_filenames");	break; 
		case(3):	strcpy(p,"scatter_order_filenames");	break; 
		case(4):	strcpy(p,"first_filename_to_all");	break; 
		case(5):	strcpy(p,"omit_lines");	break; 
		case(6):	strcpy(p,"omit_alternate_lines");	break; 
		case(7):	strcpy(p,"dupl_lines,_new_filename");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(177):
		switch(modeno) {
		case(1):	strcpy(p,"sort_entry_times");	break; 
		case(2):	strcpy(p,"reverse_timing_pattern");	break; 
		case(3):	strcpy(p,"reverse_timing_&_names");	break; 
		case(4):	strcpy(p,"freeze_timegaps");	break; 
		case(5):	strcpy(p,"freeze_timegaps_&_names");	break; 
		case(6):	strcpy(p,"scatter_entry_times");	break; 
		case(7):	strcpy(p,"shuffle_up_entry_times");	break; 
		case(8):	strcpy(p,"add_to_timegaps");	break; 
		case(9):	strcpy(p,"create_timegap_1");	break; 
		case(10):	strcpy(p,"create_timegap_2");	break; 
		case(11):	strcpy(p,"create_timegap_3");	break; 
		case(12):	strcpy(p,"create_timegap_4");	break; 
		case(13):	strcpy(p,"enlarge_timegap_1");	break; 
		case(14):	strcpy(p,"enlarge_timegap_2");	break; 
		case(15):	strcpy(p,"enlarge_timegap_3");	break; 
		case(16):	strcpy(p,"enlarge_timegap_4");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(178):
		switch(modeno) {
		case(1):	strcpy(p,"fix_position");	break; 
		case(2):	strcpy(p,"narrow");	break; 
		case(3):	strcpy(p,"sequence_leftwards");	break; 
		case(4):	strcpy(p,"sequence_rightwards");	break; 
		case(5):	strcpy(p,"scatter");	break; 
		case(6):	strcpy(p,"scatter_alternating");	break; 
		case(7):	strcpy(p,"twist_whole_mix");	break; 
		case(8):	strcpy(p,"twist_a_line");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(179):
		switch(modeno) {
		case(1):	strcpy(p,"at_midtimes");	break; 
		case(2):	strcpy(p,"at_endtimes");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(185):
		switch(modeno) {
		case(1):	strcpy(p,"boost-or-cut_below_frq");	break; 
		case(2):	strcpy(p,"boost-or-cut_above_frq");	break; 
		case(3):	strcpy(p,"boost-or-cut_around_frq");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(186):
		switch(modeno) {
		case(1):	strcpy(p,"bands_as_frq_(hz)");	break; 
		case(2):	strcpy(p,"bands_as_midi");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(187):
		switch(modeno) {
		case(1):	strcpy(p,"high_pass");	break; 
		case(2):	strcpy(p,"low_pass");	break; 
		case(3):	strcpy(p,"band_pass");	break; 
		case(4):	strcpy(p,"notch");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(188):
	case(189):
		switch(modeno) {
		case(1):	strcpy(p,"harmonics");	break; 
		case(2):	strcpy(p,"alternate_harmonics");	break; 
		case(3):	strcpy(p,"subharmonics");	break; 
		case(4):	strcpy(p,"harmonics_with_offset");	break; 
		case(5):	strcpy(p,"fixed_number_of_bands");	break; 
		case(6):	strcpy(p,"fixed_interval_between");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(190):
		switch(modeno) {
		case(1):	strcpy(p,"bands_as_frq_(hz)");	break; 
		case(2):	strcpy(p,"bands_as_midi");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(191):
		switch(modeno) {
		case(1):	strcpy(p,"bands_as_frq_(hz)");	break; 
		case(2):	strcpy(p,"bands_as_midi");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(192):
		switch(modeno) {
		case(1):	strcpy(p,"high_pass");	break; 
		case(2):	strcpy(p,"low_pass");	break; 
		case(3):	strcpy(p,"band_pass");	break; 
		case(4):	strcpy(p,"notch");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(193):
		switch(modeno) {
		case(1):	strcpy(p,"bands_as_frq_(hz)");	break; 
		case(2):	strcpy(p,"bands_as_midi");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(194):
		switch(modeno) {
		case(1):	strcpy(p,"phase_shift_filter");	break; 
		case(2):	strcpy(p,"phasing_effect");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(195):
		switch(modeno) {
		case(1):	strcpy(p,"gain");	break; 
		case(2):	strcpy(p,"dBgain");	break; 
		case(3):	strcpy(p,"normalise");	break; 
		case(4):	strcpy(p,"force_level");	break; 
		case(5):	strcpy(p,"balance_srcs");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(196):
		switch(modeno) {
		case(1):	strcpy(p,"pan");	break; 
		case(2):	strcpy(p,"mirror");	break; 
		case(3):	strcpy(p,"mirror_panning_file");	break; 
		case(4):	strcpy(p,"narrow");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(197):
		switch(modeno) {
		case(1):	strcpy(p,"tape_transpose_by_time-ratio");	break; 
		case(2):	strcpy(p,"tape_transpose_by_semitones");	break; 
		case(3):	strcpy(p,"see_transposition_by_time-ratio");	break; 
		case(4):	strcpy(p,"see_transposition_by_semitones");	break; 
		case(5):	strcpy(p,"tape_accelerate");	break; 
		case(6):	strcpy(p,"tape_vibrato");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(198):
		switch(modeno) {
		case(1):	strcpy(p,"delay");	break; 
		case(2):	strcpy(p,"varying_delay");	break; 
		case(3):	strcpy(p,"stadium");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(199):
		switch(modeno) {
		case(1):	strcpy(p,"pitchshift");	break; 
		case(2):	strcpy(p,"timesqueeze");	break; 
		case(3):	strcpy(p,"reverb");	break; 
		case(4):	strcpy(p,"scramble");	break; 
		case(5):	strcpy(p,"granulate");	break; 
		case(6):	strcpy(p,"brassage");	break; 
		case(7):	strcpy(p,"full_monty");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(201):
		switch(modeno) {
		case(1):	strcpy(p,"reverse");	break; 
		case(2):	strcpy(p,"shred");	break; 
		case(3):	strcpy(p,"scrub_over_heads");	break; 
		case(4):	strcpy(p,"lower_resolution");	break; 
		case(5):	strcpy(p,"ring_modulate");	break; 
		case(6):	strcpy(p,"cross_modulate");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(202):
		switch(modeno) {
		case(1):	strcpy(p,"standard");	break; 
		case(2):	strcpy(p,"get_spec_envelope_only");	break; 
		case(3):	strcpy(p,"get_spec_magnitudes_only");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(206):
	case(207):
	case(208):
	case(209):
	case(210):
	case(211):
	case(212):
		switch(modeno) {
		case(1):	strcpy(p,"time_in_seconds");	break; 
		case(2):	strcpy(p,"time_as_sample_count");	break; 
		case(3):	strcpy(p,"time_as_grouped_samples");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(214):
		switch(modeno) {
		case(1):	strcpy(p,"make_a_copy");	break; 
		case(2):	strcpy(p,"make_multiple_copies");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(215):
		switch(modeno) {
		case(1):	strcpy(p,"extract_a_channel");	break; 
		case(2):	strcpy(p,"extract_all_channels");	break; 
		case(3):	strcpy(p,"zero_a_channel");	break; 
		case(4):	strcpy(p,"convert_stereo_to_mono");	break; 
		case(5):	strcpy(p,"convert_mono_to_stereo");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(216):
		switch(modeno) {
		case(1):	strcpy(p,"gated_extraction");	break; 
		case(2):	strcpy(p,"preview_extraction");	break; 
		case(3):	strcpy(p,"top_and_tail");	break; 
		case(4):	strcpy(p,"remove_dc");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(217):
		switch(modeno) {
		case(1):	strcpy(p,"change_sampling_rate");	break; 
		case(2):	strcpy(p,"convert_sample_format");	break; 
		case(3):	strcpy(p,"change_properties");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(218):
		switch(modeno) {
		case(1):	strcpy(p,"any_files");	break; 
		case(2):	strcpy(p,"non-text_files");	break; 
		case(3):	strcpy(p,"same_type");	break; 
		case(4):	strcpy(p,"same_properties");	break; 
		case(5):	strcpy(p,"same_channels");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(219):
		switch(modeno) {
		case(1):	strcpy(p,"by_filetype");	break; 
		case(2):	strcpy(p,"by_sampling_rate");	break; 
		case(3):	strcpy(p,"by_duration");	break; 
		case(4):	strcpy(p,"by_log_duration");	break; 
		case(5):	strcpy(p,"into_duration_order");	break; 
		case(6):	strcpy(p,"find_rogues");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(220):
		switch(modeno) {
		case(1):	strcpy(p,"string_sounds_together");	break; 
		case(2):	strcpy(p,"dump_sndfiles");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(236):
		switch(modeno) {
		case(1):	strcpy(p,"midi_to_frq");	break; 
		case(2):	strcpy(p,"frq_to_midi");	break; 
		case(3):	strcpy(p,"note_to_frq");	break; 
		case(4):	strcpy(p,"note_to_midi");	break; 
		case(5):	strcpy(p,"frq_to_note");	break; 
		case(6):	strcpy(p,"midi_to_note");	break; 
		case(7):	strcpy(p,"frq_ratio_to_semitones");	break; 
		case(8):	strcpy(p,"frq_ratio_to_interval");	break; 
		case(9):	strcpy(p,"interval_to_frq_ratio");	break; 
		case(10):	strcpy(p,"semitones_to_frq_ratio");	break; 
		case(11):	strcpy(p,"octaves_to_frq_ratio");	break; 
		case(12):	strcpy(p,"octaves_to_semitones");	break; 
		case(13):	strcpy(p,"frq_ratio_to_octaves");	break; 
		case(14):	strcpy(p,"semitones_to_octaves");	break; 
		case(15):	strcpy(p,"semitones_to_interval");	break; 
		case(16):	strcpy(p,"frq_ratio_to_time_ratio");	break; 
		case(17):	strcpy(p,"semitones_to_time_ratio");	break; 
		case(18):	strcpy(p,"octaves_to_time_ratio");	break; 
		case(19):	strcpy(p,"interval_to_time_ratio");	break; 
		case(20):	strcpy(p,"time_ratio_to_frq_ratio");	break; 
		case(21):	strcpy(p,"time_ratio_to_semitones");	break; 
		case(22):	strcpy(p,"time_ratio_to_octaves");	break; 
		case(23):	strcpy(p,"time_ratio_to_interval");	break; 
		case(24):	strcpy(p,"gain_factor_to_db_gain");	break; 
		case(25):	strcpy(p,"db_gain_to_gain_factor");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(237):
		switch(modeno) {
		case(1):	strcpy(p,"sine");	break; 
		case(2):	strcpy(p,"square");	break; 
		case(3):	strcpy(p,"saw");	break; 
		case(4):	strcpy(p,"ramp");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	case(251):
		switch(modeno) {
		case(1):	strcpy(p,"time_in_seconds");	break; 
		case(2):	strcpy(p,"time_as_sample_count");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
		break;
	default:
		switch(modeno) {
		case(0): strcpy(p,"");	break;
		default: strcpy(p,"UNKNOWN-MODE");	break;
		}
	}
}

void to_ucase(char *str)
{
	char *p = str, cc;
	while(*p != ENDOFSTR) {
		if(isalpha(*p) && *p > 96) {
			cc = (char)(*p - 32);
			*p = cc;
		}
		p++;
	}
}
