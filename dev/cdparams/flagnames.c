#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <tkglobals.h>
#include <logic.h>
#include <cdparams.h>
#include <globcon.h>
#include <processno.h>
#include <modeno.h>

/************************************ SETUP_FLAGNAMES *****************************/

int setup_flagnames(int process,int mode,int total_flags,aplptr ap)
{
	if((ap->flagname = (char **)malloc((total_flags
	) * sizeof(char *)))==NULL) {
		sprintf(errstr,"INSUFFICIENT MEMORY: to setup_flagnames\n"); 
		return(MEMORY_ERROR);
	}
	switch(process) {

			/******************** SPEC *******************/

	case(ALT):
		ap->flagname[0] 	= "LESS_BODY_IN_SPECTRUM";
		break;				   
	case(ARPE):
		if(mode==ON 
		|| mode==BOOST
		|| mode==ABOVE_BOOST
		|| mode==BELOW_BOOST) {
			ap->flagname[0] = "TRACK_FRQ_CHANGE";
			ap->flagname[1] = "SUSTAINS_RUN_TO_ZERO";
		}
		break;
	case(BARE):
	case(CHORD):
	case(MULTRANS):
		ap->flagname[0] 	= "LESS_BODY_IN_SPECTRUM";
		break;
	case(TRNSF):
	case(TRNSP):
	case(FOLD):
		ap->flagname[0] 	= "MORE_BODY_IN_SPECTRUM";
		break;
	case(DIFF):
		ap->flagname[0] 	= "RETAIN_SUBZERO_AMPLITUDES";
		break;				   
	case(DRUNK):
		ap->flagname[0] 	= "REJECT_ZERO_STEPS";
		break;
	case(FMNTSEE):
		ap->flagname[0] 	= "DISPLAY_FORMANTBAND_DATA";
		break;
	case(FORMSEE):
		ap->flagname[0] 	= "DISPLAY_IN_SEMITONE_BANDS";
		break;
	case(GREQ):
		ap->flagname[0] 	= "NOTCH_FILTER";
		break;
	case(MEAN):
		ap->flagname[0] 	= "ZERO_CHANNELS_OUTSIDE_RANGE";
		break;				   
	case(P_FIX):
		ap->flagname[0] 	= "REMOVE_2-WINDOW_GLITCHES";
		ap->flagname[1] 	= "INTERPOLATE";
		break;
	case(P_QUANTISE):
		ap->flagname[0] 	= "DUPLICATE_Q-SET_IN_ALL_OCTAVES";
		break;				   
	case(P_SMOOTH):
		ap->flagname[0] 	= "HOLD_LAST_CALCULATED_PITCH";
		break;
	case(P_SYNTH):	
	case(P_VOWELS):
	case(VFILT):
	case(P_GEN):
	case(P_INSERT):	
	case(P_SINSERT):	
	case(P_PTOSIL):	
	case(P_NTOSIL):	
	case(ANALENV):	
	case(P_BINTOBRK):	
	case(MAKE2):
	case(P_INTERP):
		break;
	case(PEAK):
		ap->flagname[0] 	= "ADJUST_FOR_SENSITIVITY_OF_EAR";
		break;
	case(PITCH):
		ap->flagname[0] = "USE_OTHER_ALGORITHM";
		if(mode==PICH_TO_BIN) {
			ap->flagname[1] = "KEEP_PITCH_ZEROS";
		}
		break;
	case(SCAT):
		ap->flagname[0] 	= "KEEP_RANDOM_SET_ONLY";
		ap->flagname[1] 	= "DON'T_NORMALISE";
		break;
	case(SHIFT):
		ap->flagname[0] 	= "LOG_INTERP_CHANGING_FRQ";
		break;
	case(S_TRACE):
		if(mode!=TRC_ALL) {
			ap->flagname[0] = "RETAIN_OUT-OF-BAND_CHANS";
		}
		break;

			/******************** GROUCHO *****************/

	case(DISTORT_MLT):
		ap->flagname[0] = "SMOOTHED";
		break;
	case(DISTORT_DIV):
		ap->flagname[0] = "INTERPOLATED";
		break;
	case(DISTORT_TEL):
		ap->flagname[0] = "TO_AVERAGE_CYCLELEN";
		break;
	case(DISTORT_PULSED):
		ap->flagname[0] = "KEEP_START_OF_SRC_BEFORE_IMPULSES_BEGIN";
		ap->flagname[1] = "KEEP_END_OF_SRC_AFTER_IMPULSES_FINISH";
		break;
	case(LOOP):
		ap->flagname[0] = "FROM_INFILE_START";
		break;
	case(SCRAMBLE):
		ap->flagname[0] = "RUN_FROM_SND_START";
		ap->flagname[1] = "RUN_TO_SND_END";
		break;
	case(SIMPLE_TEX):
	case(TIMED):
		ap->flagname[0] = "PLAY_ALL_OF_INSOUND";
		ap->flagname[1] = "PLAY_FILES_CYCLICALLY";
		ap->flagname[2] = "RANDOMLY_PERMUTE_EACH_CYCLE";
		break;
	case(DECORATED):
	case(PREDECOR):
	case(POSTDECOR):
		ap->flagname[0] = "PLAY_ALL_OF_INSOUND";
		ap->flagname[1] = "FIXED_TIMESTEP_IN_GROUP";
		ap->flagname[2] = "SCATTER_DECOR_INSTRS";
		ap->flagname[3] = "DECORS_TO_HIGHEST_NOTE";
		ap->flagname[4] = "DECORS_ON_EVERY_NOTE";
		ap->flagname[5] = "DISCARD_ORIGINAL_LINE";
		break;
	case(ORNATE):
	case(PREORNATE):
	case(POSTORNATE):
		ap->flagname[0] = "PLAY_ALL_OF_INSOUND";
		ap->flagname[1] = "FIXED_NOTE-SUSTAIN_IN_MOTIFS";
		ap->flagname[2] = "SCATTER_ORNAMENT_INSTRS";
		ap->flagname[3] = "ORNAMENTS_TO_HIGHEST_NOTE";
		ap->flagname[4] = "ORNAMENTS_ON_EVERY_NOTE";
		break;
	case(GROUPS):
	case(TGROUPS):
		ap->flagname[0] = "PLAY_ALL_OF_INSOUND";
		ap->flagname[1] = "FIXED_TIMESTEP_IN_GROUP";
		ap->flagname[2] = "SCATTER_DECOR_INSTRS";
		break;
	case(MOTIFSIN):
	case(MOTIFS):
	case(TMOTIFS):
	case(TMOTIFSIN):
		ap->flagname[0] = "PLAY_ALL_OF_INSOUND";
		ap->flagname[1] = "FIXED_NOTE-SUSTAIN_IN_MOTIFS";
		ap->flagname[2] = "SCATTER_MOTIF_INSTRS";
		break;
	case(GRAIN_COUNT):		
	case(GRAIN_OMIT):		
	case(GRAIN_DUPLICATE):
	case(GRAIN_REORDER):	
	case(GRAIN_REPITCH):	
	case(GRAIN_RERHYTHM):
	case(GRAIN_REMOTIF):	
	case(GRAIN_TIMEWARP):	
	case(GRAIN_POSITION):
	case(GRAIN_ALIGN):		
	case(GRAIN_GET):		
	case(GRAIN_REVERSE):
		ap->flagname[0] = "IGNORE_LAST_GRAIN";
		break;
	case(MIX):
		ap->flagname[0] = "ALTERNATIVE_MIX";
		break;
	case(MIXSYNCATT):
		ap->flagname[0] = "POWER_METHOD";
		break;
	case(MIXSHUFL):	   	   
		if(mode==MSH_DUPL_AND_RENAME) {
			ap->flagname[0] = "NO_SNDNAME_CHECK";
		}
		break;
	case(FLTBANKN):		
	case(FLTBANKU):		
	case(FLTBANKV2):
		ap->flagname[0] = "DOUBLE_FILTERING";
		ap->flagname[1] = "NORMALISE";
		break;
	case(FLTBANKV):
		ap->flagname[0] = "DOUBLE_FILTERING";
		ap->flagname[1] = "DROP_OUT_IF_FILTER_OVERFLOWS";
		ap->flagname[2] = "NORMALISE";
		break;
	case(FLTITER):
		ap->flagname[0] = "DOUBLE_FILTERING";
		ap->flagname[1] = "PITCH_NOT_INTERPOLATED";
		ap->flagname[2] = "EXPONENTIAL_DECAY";
		ap->flagname[3] = "NO_NORMALISATION";
		break;
	case(ALLPASS):		
		ap->flagname[0] = "LINEAR_INTERP_DELAY";
		break;

	case(MOD_LOUDNESS):		
		break;

	case(MOD_SPACE):		
	case(SCALED_PAN):		
		break;

	case(MOD_PITCH):

		switch(mode) {
		case(MOD_ACCEL):
		case(MOD_VIBRATO):
			break;
		default:
			ap->flagname[0] = "OUTFILE_TIMES";
			break;
		}
		break;
	case(MOD_REVECHO):

		switch(mode) {
		case(MOD_DELAY):
			ap->flagname[0] = "INVERT_DELAYED_SIGNAL";
			break;
		case(MOD_VDELAY):
			break;
		case(MOD_STADIUM):
			ap->flagname[0] = "NORMALISE_OUTPUT";
			break;
		}
		break;
	case(MOD_RADICAL):
		if(mode == MOD_SHRED)
			ap->flagname[0] = "SMOOTHER";
		else if(mode == MOD_SCRUB)
			ap->flagname[0] = "SINGLE_SCRUB_(version_7+_only)";
		break;
	case(BRASSAGE):		
		switch(mode) {
		case(GRS_BRASSAGE):
		case(GRS_FULL_MONTY):
 		    ap->flagname[0] = "NO_DECCELERATION";
	    	ap->flagname[1] = "EXPONENTIAL_SPLICES";
	    	ap->flagname[2] = "PITCH_NOT_INTERPOLATED";
			break;
		case(GRS_GRANULATE):
		    ap->flagname[0] = "NO_DECCELERATION";
			break;
		}
		break;

	case(SAUSAGE):		
	    ap->flagname[0] = "NO_DECCELERATION";
    	ap->flagname[1] = "EXPONENTIAL_SPLICES";
    	ap->flagname[2] = "PITCH_NOT_INTERPOLATED";
		break;

	case(EDIT_CUT):
	case(EDIT_CUTEND):
	case(EDIT_ZCUT):
	case(EDIT_EXCISE):
	case(EDIT_EXCISEMANY):
	case(INSERTSIL_MANY):
		break;
	case(EDIT_CUTMANY):
		break;
	case(STACK):
	    ap->flagname[0] = "SEE_STACK_LEVELS";
	    ap->flagname[1] = "NORMALISE";
		break;
	case(EDIT_INSERT):
	case(EDIT_INSERTSIL):
	    ap->flagname[0] = "OVERWRITE";
	    ap->flagname[1] = "RETAIN_ANY_TRAILING_SILENCE";
		break;

	case(EDIT_JOIN):
	case(JOIN_SEQ):
	case(JOIN_SEQDYN):
	    ap->flagname[0] = "SPLICE_START";
	    ap->flagname[1] = "SPLICE_END";
		break;

	case(HOUSE_COPY):
		break;

	case(HOUSE_DEL):
   	ap->flagname[0] = "SEARCH_FOR_ALL_COPIES";
		break;
	case(HOUSE_CHANS):
		switch(mode) {
		case(STOM):
		    ap->flagname[0] = "INVERT_CHANNEL2_PHASE";
			break;
		default:
			break;			
		}
		break;

	case(HOUSE_BUNDLE):
		break;

	case(HOUSE_SORT):
		switch(mode) {
		case(BY_DURATION):
		case(BY_LOG_DUR):
		case(IN_DUR_ORDER):
		    ap->flagname[0] = "DON'T_DISPLAY_TIMINGS";
			break;
		default:
			break;			
		}
		break;

	case(HOUSE_SPEC):
		break;

	case(HOUSE_EXTRACT):
		switch(mode) {
		case(HOUSE_TOPNTAIL):
		    ap->flagname[0] = "NO_START_TRIM";
		    ap->flagname[1] = "NO_END_TRIM";
			break;
		default:
			break;			
		}
		break;

	case(TOPNTAIL_CLICKS):
	    ap->flagname[0] = "TRIM_START";
	    ap->flagname[1] = "TRIM_END";
		break;
	case(HOUSE_BAKUP):
	case(HOUSE_DUMP):
	case(HOUSE_GATE):
		break;
	case(HOUSE_RECOVER):
	    ap->flagname[0] = "INVERT_PHASE";
		break;

	case(HOUSE_DISK):
		break;

	case(INFO_SAMPTOTIME):
	case(INFO_TIMETOSAMP):
	    ap->flagname[0] = "COUNT_SAMPLES_IN_GROUPS";
		break;

	case(INFO_DIFF):
	    ap->flagname[0] = "IGNORE_LENGTH_DIFFERENCE";
	    ap->flagname[1] = "IGNORE_CHANNEL_COUNT_DIFFERENCE";
		break;

	case(INFO_MAXSAMP):
	    ap->flagname[0] = "FORCE_SEARCH_IN_SOUND";
		break;
	case(INFO_MAXSAMP2):
	case(INFO_TIMESUM):
	case(INFO_PROPS):
	case(INFO_SFLEN):
	case(INFO_TIMELIST):
	case(INFO_LOUDLIST):
	case(INFO_TIMEDIFF):
	case(INFO_LOUDCHAN):
	case(INFO_FINDHOLE):
	case(INFO_CDIFF):
	case(INFO_PRNTSND):
	case(INFO_MUSUNITS):
		break;
	case(SYNTH_WAVE):
	case(SYNTH_NOISE):
	case(SYNTH_SIL):
	    ap->flagname[0] = "CUBIC_SPLINE_INTERP_FRQ_DATA";
		break;
	case(MULTI_SYN):
		break;
	case(SYNTH_SPEC):
	    ap->flagname[0] = "SPREAD_AS_TRANSPOSITION_RATIO";
		break;
	case(RANDCUTS):
		break;
	case(RANDCHUNKS):
		ap->flagname[0] 	= "LINEAR_DISTRIBUTION";
		ap->flagname[1] 	= "ALL_BEGIN_AT_SOUND_START";
		break;
	case(SIN_TAB):
		break;
	case(ACC_STREAM):
		break;
 	case(HF_PERM1):
 	case(HF_PERM2):
		ap->flagname[0] 	= "CHORDS_ONLY_OF_SPECIFIED_MIN_NO_OF_NOTES";
		ap->flagname[1] 	= "SMALLEST_SPAN_FIRST_(DEFAULT:_LARGEST_FIRST)";
		ap->flagname[2] 	= "FINAL_SORT_BY_ALTERNATIVE_METHOD_(DEFAULT:_BY_DENSITY)";
		ap->flagname[3] 	= "ELIMINATE_CHORDS_DUPLICATED_AT_OCTAVES";
		break;
	case(DEL_PERM): 
	case(DEL_PERM2):
		break;
 	case(TWIXT):
 	case(SPHINX):
		if(process != TWIXT || mode != TRUE_EDIT)
			ap->flagname[0] 	= "RANDOMLY_PERMUTE_FILE_ORDER";
		break;
 	case(NOISE_SUPRESS):
		ap->flagname[0] 	= "RETAIN_NOISE_RATHER_THAN_TONE";
		break;
 	case(TIME_GRID):
 	case(SEQUENCER):
	case(SEQUENCER2):
	case(CONVOLVE):
	case(BAKTOBAK):
	case(MIX_PAN):
	case(MIX_AT_STEP):
	case(FIND_PANPOS):
		break;
	case(CLICK):
		ap->flagname[0] 	= "SHOW_LINE_TIMES";
		break;
	break;
	case(SHUDDER):
		ap->flagname[0] 	= "BALANCE_CHANNEL_LEVELS";
		break;
	case(DOUBLETS):
		ap->flagname[0] 	= "RETAIN_ORIGINAL_DURATION";
		break;
	case(SYLLABS):
		ap->flagname[0] 	= "CUT_PAIRS_OF_SYLLABLES";
		break;
	case(MAKE_VFILT):
	case(BATCH_EXPAND):
	case(MIX_MODEL):
	case(ENVSYN):
	case(GRAIN_ASSESS):
	case(GREV):
		break;
	case(HOUSE_GATE2):
		ap->flagname[0] 	= "SEE_DETAILS_OF_GLITCHES";
		break;
	case(RRRR_EXTEND):
		switch(mode) {
		case(0):
			ap->flagname[0]	= "KEEP_EXTENDED_ITERATE_ONLY";
			break;
		case(1):
			ap->flagname[0]	= "KEEP_SOUND_BEFORE_ITERATE";
			ap->flagname[1]	= "KEEP_SOUND_AFTER_ITERATE";
			break;
		}
		break;
	case(ZIGZAG):
		if(mode == 0)
			ap->flagname[0]	= "DON'T_RUN_TO_END_OF_FILE";
		break;
	case(SSSS_EXTEND):
		ap->flagname[0] 	= "KEEP_EXTENDED_NOISE_ONLY";
		break;
	case(MANY_ZCUTS):
		break;
	default:
		sprintf(errstr,"Unknown case: setup_flagnames()\n");
		return(PROGRAM_ERROR);
	}
	return(FINISHED);
}

